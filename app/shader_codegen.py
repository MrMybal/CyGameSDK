"""Trusted source lowering for the bounded Sandbox material expression language."""
import math
from pathlib import Path

class ShaderGraphError(ValueError):
    def __init__(self,message,node='',function=''):
        super().__init__(message);self.diagnostics=[{'severity':'Error','nodeId':node,'functionName':function,'message':message}]

class Frame:
    def __init__(self,graph,name='',arguments=None):
        self.graph=graph;self.name=name;self.arguments=arguments or {};self.nodes={n['id']:n for n in graph['nodes']}
        self.links={(l['toNode'],l['toPin']):(l['fromNode'],l['fromPin']) for l in graph['links']};self.cache={}

class Compiler:
    def __init__(self,graph):
        self.graph=graph;self.lines=[];self.nodes=[];self.budget=10000;self.counter=0;self.calls=0
        self.parameters={name:i for i,name in enumerate(sorted(graph.get('variables',{})))}
    def literal(self,value):
        if type(value) is bool:return 'true' if value else 'false'
        if isinstance(value,(int,float)) and math.isfinite(value):
            result=format(value,'.9g');return result if any(c in result for c in '.eE') else result+'.0'
        if isinstance(value,list) and len(value)==3:return 'float3('+','.join(self.literal(x) for x in value)+')'
        raise ShaderGraphError('This value type is unavailable in the preview shader')
    def input(self,f,n,pin):
        self.budget-=1
        if self.budget<0:raise ShaderGraphError('Shader lowering budget exceeded',n['id'],f.name)
        link=f.links.get((n['id'],pin))
        try:return self.output(f,*link) if link else self.literal(n['values'][pin])
        except ShaderGraphError as exc:
            if not exc.diagnostics[0]['nodeId']:exc.diagnostics[0].update(nodeId=n['id'],functionName=f.name)
            raise
    def output(self,f,id,pin):
        if (id,pin) in f.cache:return f.cache[id,pin]
        n=f.nodes[id];t=n['type'];a=lambda p:self.input(f,n,p)
        self.budget-=1
        if self.budget<0:raise ShaderGraphError('Shader lowering budget exceeded',id,f.name)
        kind='float'
        if t=='function.input':return f.arguments[pin]
        if t=='function.call':
            self.calls+=1
            if self.calls>2000:raise ShaderGraphError('Shader function expansion limit exceeded',id,f.name)
            function=self.graph['functions'][n['function']]
            if function.get('mode','pure')!='pure':raise ShaderGraphError('Execution calls are not material expressions',id,f.name)
            child=Frame(function,n['function'],{p:a(p) for p in function['inputs']})
            result=next(node for node in function['nodes'] if node['type']=='function.output')
            for p in function['outputs']:f.cache[id,p]=self.input(child,result,p)
            return f.cache[id,pin]
        if t.startswith('constant.'):
            expression=self.literal(n['values']['value']);kind=t.split('.')[1]
        elif t.startswith('variable.get.'):
            name=n['variable'];kind=t.split('.')[-1];value='cy_params['+str(self.parameters[name])+']'
            expression=value+'.xyz' if kind=='color' else '('+value+'.x != 0)' if kind=='bool' else value+'.x'
        elif t.startswith('route.'):
            kind=t.split('.')[-1];expression=a('value')
        elif t=='material.texture':
            if (n['id'],'path') in f.links:raise ShaderGraphError('Texture path must be a literal asset property',id,f.name)
            paths=set()
            def scan(body):
                for node in body['nodes']:
                    if node['type']=='material.texture':paths.add(node['values']['path'])
                for body in body.get('functions',{}).values():scan(body)
            scan(self.graph)
            slot=sorted(paths).index(n['values']['path']);sample='cy_tex_'+str(slot)+'.Sample(cy_sampler,float2('+a('u')+','+a('v')+'))'
            kind='color' if pin=='color' else 'float';expression=sample+('.rgb' if pin=='color' else '.a')
        elif t=='material.uv':expression='uv.'+('x' if pin=='u' else 'y')
        elif t=='uv.transform':expression='('+a(pin)+'*'+a('tileU' if pin=='u' else 'tileV')+'+'+a('offsetU' if pin=='u' else 'offsetV')+')'
        elif t in ('math.sine','math.fract','math.clamp'):expression={'math.sine':'sin','math.fract':'frac','math.clamp':'saturate'}[t]+'('+a('value')+')'
        elif t in ('math.add','math.subtract','math.multiply','math.divide','math.less','math.greater','math.equal'):
            left,right=a('a'),a('b');op={'math.add':'+','math.subtract':'-','math.multiply':'*','math.divide':'/','math.less':'<','math.greater':'>','math.equal':'=='}[t]
            kind='bool' if t in ('math.less','math.greater','math.equal') else 'float'
            expression='(abs('+right+')<1e-12?0:('+left+'/'+right+'))' if t=='math.divide' else '('+left+op+right+')'
        elif t=='color.compose':kind='color';expression='saturate(float3('+','.join(a(p) for p in ('r','g','b'))+'))'
        elif t=='color.multiply':kind='color';expression='saturate('+a('color')+'*'+a('factor')+')'
        elif t=='color.mix':kind='color';expression='lerp('+a('a')+','+a('b')+',saturate('+a('factor')+'))'
        elif t=='pattern.gradient':kind='color';expression='lerp('+a('a')+','+a('b')+',saturate('+a('u')+'))'
        elif t=='pattern.checker':
            kind='color';u,v,scale=a('u'),a('v'),a('scale');expression='lerp('+a('a')+','+a('b')+',step(.5,abs(fmod(floor('+u+'*'+scale+')+floor('+v+'*'+scale+'),2.0))))'
        else:raise ShaderGraphError('No preview shader lowering for node type '+t,id,f.name)
        if kind not in ('float','bool','color'):raise ShaderGraphError('No preview shader value type '+kind,id,f.name)
        index=self.counter;self.counter+=1;name='cy_value_'+str(index);file='node_'+str(index)
        self.nodes.append({'file':file,'nodeId':id,'functionName':f.name,'pin':pin})
        self.lines += ['#line 1 "'+file+'"',('float3' if kind=='color' else kind)+' '+name+' = '+expression+';']
        f.cache[id,pin]=name;return name
    def compile(self):
        frame=Frame(self.graph);outputs=[n for n in self.graph['nodes'] if n['type']=='material.output']
        if len(outputs)!=1:raise ShaderGraphError('Material preview requires exactly one output')
        n=outputs[0];color,rough,metal=(self.input(frame,n,p) for p in ('baseColor','roughness','metallic'))
        code='void cy_material(float2 uv,out float3 albedo,out float rough,out float metal){\n'+'\n'.join(self.lines)+'\n#line 1 "material_output"\nalbedo=saturate('+color+');rough='+rough+';metal='+metal+';\n}\n'
        paths=set()
        def scan(body):
            for node in body['nodes']:
                if node['type']=='material.texture':paths.add(node['values']['path'])
            for body in body.get('functions',{}).values():scan(body)
        scan(self.graph)
        if len(paths)>8:raise ShaderGraphError('A material supports up to eight textures')
        declarations=''.join('Texture2D cy_tex_'+str(i)+' : register(t'+str(i)+');\n' for i in range(len(paths)))
        if paths:declarations+='SamplerState cy_sampler : register(s0);\n'
        code=declarations+code
        source=Path(__file__).with_name('shaders').joinpath('preview_template.hlsl').read_text('utf-8').replace('// CY_MATERIAL',code)
        if len(source.encode('utf-8'))>1024*1024:raise ShaderGraphError('Generated preview shader exceeds 1 MiB')
        return source,self.nodes

def generate(graph):return Compiler(graph).compile()
