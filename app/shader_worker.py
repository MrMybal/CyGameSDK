"""One-shot D3DCompiler worker, owned by CyToolsCore ProcessWorker."""
import ctypes as c,hashlib,json,sys
from pathlib import Path

def compile_shader(source):
    dll=c.WinDLL('d3dcompiler_47.dll');compile=dll.D3DCompile
    compile.argtypes=[c.c_void_p,c.c_size_t,c.c_char_p,c.c_void_p,c.c_void_p,c.c_char_p,c.c_char_p,c.c_uint,c.c_uint,c.POINTER(c.c_void_p),c.POINTER(c.c_void_p)];compile.restype=c.c_long
    binary,error=c.c_void_p(),c.c_void_p();data=source.encode('utf-8')
    def blob(pointer):
        if not pointer.value:return b''
        table=c.cast(pointer,c.POINTER(c.POINTER(c.c_void_p))).contents
        address=c.WINFUNCTYPE(c.c_void_p,c.c_void_p)(table[3])(pointer)
        size=c.WINFUNCTYPE(c.c_size_t,c.c_void_p)(table[4])(pointer)
        return c.string_at(address,size)
    def release(pointer):
        if pointer.value:
            table=c.cast(pointer,c.POINTER(c.POINTER(c.c_void_p))).contents;c.WINFUNCTYPE(c.c_ulong,c.c_void_p)(table[2])(pointer)
    try:
        result=compile(data,len(data),b'CyGameSDK_Preview',None,None,b'ps',b'ps_5_0',(1<<11)|(1<<15),0,c.byref(binary),c.byref(error))
        diagnostics=blob(error).decode('utf-8',errors='replace').rstrip('\x00')
        return result>=0,blob(binary),diagnostics
    finally:release(binary);release(error)

def run(payload):
    from shader_codegen import generate,ShaderGraphError
    if payload.get('protocolVersion')!='1.0':raise ValueError('Unsupported worker protocol')
    workspace=Path(payload['workspace']).resolve();p=payload['parameters']
    def owned(name):
        path=Path(p[name]).resolve()
        if path==workspace or not path.is_relative_to(workspace):raise ValueError('Shader files must remain in the job workspace')
        path.parent.mkdir(parents=True,exist_ok=True);return path
    plan=json.loads(owned('planPath').read_text('utf-8'));source_path=owned('sourcePath');shader_path=owned('shaderPath');report_path=owned('reportPath')
    report={'format':'cygame-preview-shader','schemaVersion':1,'graphId':plan['graphId'],'signature':plan['signature'],'parameterNames':plan['parameterNames'],'sourcePath':str(source_path),'shaderPath':str(shader_path),'reportPath':str(report_path),'profile':'ps_5_0','diagnostics':[],'success':False}
    textures=plan.get('textures',[])
    if not isinstance(textures,list) or len(textures)>8:raise ValueError('A material supports up to eight textures')
    report['textures']=[]
    for i,binding in enumerate(textures):
        original=Path(binding['sourcePath']);target=report_path.parent/('texture_'+str(i)+original.suffix.lower())
        if original.stat().st_size>32*1024*1024:raise ValueError('Texture exceeds 32 MiB')
        data=original.read_bytes()
        if len(data)>32*1024*1024 or hashlib.sha256(data).hexdigest()!=binding['sha256']:raise ValueError('Texture changed after material preparation')
        with target.open('xb') as stream:stream.write(data)
        report['textures'].append({'assetPath':binding['assetPath'],'sha256':binding['sha256'],'filePath':str(target)})
    try:source,node_map=generate(plan['graph'])
    except ShaderGraphError as exc:report['diagnostics']=exc.diagnostics
    else:
        source_path.write_bytes(source.encode('utf-8'));success,bytecode,diagnostics=compile_shader(source)
        report.update(success=success,nodeMap=node_map,sourceSha256=hashlib.sha256(source.encode('utf-8')).hexdigest(),compilerOutput=diagnostics)
        if diagnostics:
            for line in diagnostics.splitlines():
                location=next((n for n in node_map if n['file']+'(' in line),{})
                report['diagnostics'].append({'severity':'Warning' if success else 'Error','nodeId':location.get('nodeId',''),'functionName':location.get('functionName',''),'message':line})
        if success:shader_path.write_bytes(bytecode);report.update(bytecodeSha256=hashlib.sha256(bytecode).hexdigest(),bytes=len(bytecode))
    report_path.write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8');return report
if __name__=='__main__':
    try:print(json.dumps(run(json.load(sys.stdin)),ensure_ascii=False))
    except Exception as exc:print(str(exc),file=sys.stderr);raise SystemExit(1)
