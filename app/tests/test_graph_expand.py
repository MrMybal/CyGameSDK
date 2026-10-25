"""Reversible authored call expansion through native commands and official Core."""
import copy,json,shutil,subprocess,sys,tempfile,unittest,uuid
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool,test_integration as integration,test_authoring as authoring,test_type_migration as typed
from cytools_core import CyToolError
class Expand(unittest.TestCase):
 @classmethod
 def setUpClass(cls):typed.TypeMigrations.setUpClass.__func__(cls)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call;node=typed.TypeMigrations.node;get=typed.TypeMigrations.get;wire=authoring.NativeAuthoring.wire;unlink=authoring.NativeAuthoring.unlink;function=authoring.NativeAuthoring.function;types=typed.TypeMigrations.types
 def graph(self,name,domain='logic'):return self.call('asset_new_graph',{'name':name+'_'+uuid.uuid4().hex[:8],'domain':domain})['graphId']
 def load(self,source):
  source=copy.deepcopy(source);source.pop('assetPath',None);name='expand-'+uuid.uuid4().hex+'.cyasset';(self.workspace/'data/assets'/name).write_text(json.dumps({'format':'cygame-graph','schemaVersion':1,'graph':source}),'utf-8');return self.call('asset_open',{'path':name})['graphId']
 def request(self,g,n,**kw):return {'graphId':g,'nodeId':n,**kw}
 def plan(self,r):return self.call('graph_expand_plan',r)
 def apply(self,r,plan,**kw):return self.call('graph_expand_apply',{**r,'planHash':plan['planHash'],**kw})
 def print_float(self,g,n,pin='result'):
  t=self.node(g,'convert.float_string');self.wire(g,n,pin,t,'value');self.wire(g,t,'value','print','text')
 def simple(self,kind='function'):
  g=self.graph('Expand_'+kind);self.function(g,'Calculate',{'x':('float',2)},{'result':('float',0)},kind=kind);a=self.node(g,'math.add',functionName='Calculate',b=1);b=self.node(g,'math.multiply',functionName='Calculate',b=3);self.wire(g,'arguments','x',a,'a','Calculate');self.wire(g,a,'value',b,'a','Calculate');self.wire(g,b,'value','return','result','Calculate');n=self.node(g,'macro.call' if kind=='macro' else 'function.call',functionRef='Calculate',x=2);self.print_float(g,n);return g,n,a,b
 def test_01_plan_metadata_determinism_clones_wire_ids_and_one_undo(self):
  g,n,a,b=self.simple();source=self.get(g);source['metadata']={'keep':7};source['functions']['Calculate']['comments']=[{'id':'inside','text':'Author note','position':[50,50],'size':[400,400],'metadata':{'keep':3}}]
  for body in (source,source['functions']['Calculate']):
   for node in body['nodes']:node['metadata']={'keep':node['id']}
   for link in body['links']:link['metadata']={'keep':link['id']}
  g=self.load(source);r=self.request(g,n);before=self.get(g);history=self.call('edit_history');revision=self.call('editor_status')['revision'];plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertEqual(plan,self.plan(r));self.assertEqual(self.get(g),before);self.assertEqual(self.call('edit_history'),history);self.assertEqual(self.call('editor_status')['revision'],revision);self.apply(r,plan);after=self.get(g);self.assertEqual(after['functions'],before['functions']);self.assertEqual(after['metadata'],before['metadata']);self.assertEqual(len(self.call('edit_history')['undo']),len(history['undo'])+1)
  for node in before['functions']['Calculate']['nodes']:
   if node['id'] not in plan['nodeMap']:continue
   actual=next(x for x in after['nodes'] if x['id']==plan['nodeMap'][node['id']]);self.assertEqual({k:v for k,v in actual.items() if k not in ('id','position')},{k:v for k,v in node.items() if k not in ('id','position')})
  for link in before['links']:self.assertEqual(next(x for x in after['links'] if x['id']==link['id'])['metadata'],link['metadata'])
  self.assertEqual(next(c for c in after['comments'] if c['id']==plan['provenanceCommentId'])['expandedCall']['node'],next(x for x in before['nodes'] if x['id']==n));self.assertTrue(any(c.get('metadata')=={'keep':3} for c in after['comments']));self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);self.call('edit_undo');self.assertEqual(self.get(g),before);self.call('edit_redo');self.assertEqual(self.get(g),after)
 def test_02_passthrough_defaults_fanout_and_disconnect(self):
  g=self.graph('Passthrough');self.function(g,'Echo',{'x':('float',2)},{'result':('float',0)});self.wire(g,'arguments','x','return','result','Echo');n=self.node(g,'function.call',functionRef='Echo',x=7);external=self.node(g,'constant.float',value=9);wire=self.wire(g,external,'value',n,'x');self.print_float(g,n);other=self.node(g,'convert.float_string');self.wire(g,n,'result',other,'value');r=self.request(g,n);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);link=next(l for l in self.get(g)['links'] if l['fromNode']==external);self.unlink(g,link['id']);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['7']);self.assertEqual(len(plan['addedNodeIds']),2)
 def test_03_macro_lazy_arguments_and_unwired_output_declaration_default(self):
  g=self.graph('MacroFallback');self.function(g,'Ignore',{'unused':('float',2)},{'result':('float',3)},kind='macro');self.call('graph_node_value',{'graphId':g,'functionName':'Ignore','nodeId':'return','key':'result','value':7});bad=self.node(g,'math.divide',b=0);n=self.node(g,'macro.call',functionRef='Ignore');self.wire(g,bad,'value',n,'unused');self.print_float(g,n);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['3']);r=self.request(g,n);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['3']);self.assertTrue(any(l['fromNode']==bad for l in self.get(g)['links']))
  g=self.graph('FunctionFallback');self.function(g,'Fallback',{}, {'result':('float',3)});self.call('graph_node_value',{'graphId':g,'functionName':'Fallback','nodeId':'return','key':'result','value':7});n=self.node(g,'function.call',functionRef='Fallback');self.print_float(g,n);r=self.request(g,n);plan=self.plan(r);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['7'])
 def test_04_ignored_wired_function_argument_blocks_error_elision(self):
  g=self.graph('EagerGuard');self.function(g,'Ignore',{'x':('float',2)},{'result':('float',3)});n=self.node(g,'function.call',functionRef='Ignore');bad=self.node(g,'math.divide',b=0);self.wire(g,bad,'value',n,'x');self.print_float(g,n);before=self.get(g);plan=self.plan(self.request(g,n));self.assertFalse(plan['valid']);self.assertIn('every wired argument',plan['diagnostic']);self.assertEqual(self.get(g),before)
  with self.assertRaises(CyToolError):self.apply(self.request(g,n),plan)
  with self.assertRaises(CyToolError):self.call('graph_run',{'graphId':g})
 def test_05_every_connected_output_checks_eager_arguments(self):
  g=self.graph('OutputGuard');self.function(g,'Split',{'x':('float',2)},{'a':('float',3),'b':('float',4)});self.wire(g,'arguments','x','return','a','Split');n=self.node(g,'function.call',functionRef='Split');outside=self.node(g,'constant.float');self.wire(g,outside,'value',n,'x');self.print_float(g,n,'a');self.assertTrue(self.plan(self.request(g,n))['valid']);t=self.node(g,'convert.float_string');self.wire(g,n,'b',t,'value');self.assertFalse(self.plan(self.request(g,n))['valid'])
 def test_06_nested_lazy_macro_does_not_hide_unused_function_argument(self):
  g=self.graph('NestedLazy');self.function(g,'Ignore',{'x':('float',2)},{'result':('float',3)},kind='macro');self.function(g,'Outer',{'x':('float',2)},{'result':('float',0)});m=self.node(g,'macro.call',functionName='Outer',functionRef='Ignore');self.wire(g,'arguments','x',m,'x','Outer');self.wire(g,m,'result','return','result','Outer');n=self.node(g,'function.call',functionRef='Outer');bad=self.node(g,'math.divide',b=0);self.wire(g,bad,'value',n,'x');self.print_float(g,n);self.assertFalse(self.plan(self.request(g,n))['valid'])
 def test_07_nested_calls_are_retained_at_one_level(self):
  g=self.graph('OneLevel');self.function(g,'Inner',{'x':('float',2)},{'result':('float',0)});self.wire(g,'arguments','x','return','result','Inner');self.function(g,'Outer',{'x':('float',2)},{'result':('float',0)});inner=self.node(g,'function.call',functionName='Outer',functionRef='Inner');self.wire(g,'arguments','x',inner,'x','Outer');self.wire(g,inner,'result','return','result','Outer');n=self.node(g,'function.call',functionRef='Outer',x=9);self.print_float(g,n);r=self.request(g,n);plan=self.plan(r);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);self.assertTrue(any(x.get('function')=='Inner' for x in self.get(g)['nodes']))
 def test_08_named_and_builtin_container_relays(self):
  values={'int':7,'vector3':[1,2,3],'array.float':[1,2],'array.int':[3],'array.string':['hi'],'array.bool':[True],'array.object':[''],'array.vector3':[[1,2,3]],'map.float':{'x':2},'map.string':{'x':'a'},'map.object':{'x':''},'set.int':[1,2],'set.string':['hi'],'enum.Team':'B','struct.Stats':{'health':9,'team':'B'}}
  for i,(t,v) in enumerate(values.items()):
   g=self.graph('Relay'+str(i));self.types(g);self.function(g,'Echo',{'x':(t,v)},{'result':(t,v)});self.wire(g,'arguments','x','return','result','Echo');self.function(g,'Outer',{}, {'result':(t,v)});n=self.node(g,'function.call',functionName='Outer',functionRef='Echo',x=v);self.wire(g,n,'result','return','result','Outer');r=self.request(g,n,functionName='Outer');plan=self.plan(r);self.assertTrue(plan['valid'],(t,plan));self.apply(r,plan);self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Outer'})['values']['result'],v)
 def test_09_pure_region_in_execution_function_and_libraries_stay_local(self):
  g,n,a,b=self.simple();self.function(g,'Work',{}, {'result':('float',0)},mode='execution');nested=self.node(g,'function.call',functionName='Work',functionRef='Calculate',x=2);self.wire(g,nested,'result','return','result','Work');self.wire(g,'arguments','then','return','exec','Work');r=self.request(g,nested,functionName='Work');plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);after=self.get(g);self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Work'})['values']['result'],9);self.assertNotIn('functions',after['functions']['Work']);self.assertTrue(any(x['id']==n for x in after['nodes']));execution=self.node(g,'function.call',functionRef='Work');self.assertFalse(self.plan(self.request(g,execution))['valid']);self.assertFalse(self.plan(self.request(g,'print'))['valid'])
 def test_10_material_values_and_actual_shader_compile(self):
  g=self.graph('MaterialExpand','material');self.function(g,'Tint',{'color':('color',[.2,.4,.6])},{'result':('color',[0,0,0])});self.wire(g,'arguments','color','return','result','Tint');n=self.node(g,'function.call',functionRef='Tint');self.unlink(g,'color_link');self.wire(g,'color','value',n,'color');self.wire(g,n,'result','surface','baseColor');before=self.call('graph_run',{'graphId':g});r=self.request(g,n);plan=self.plan(r);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g}),before)
  with tool.create_runtime(self.workspace/'expand-shader') as runtime:
   actor=runtime.authenticate(runtime.create_client('Expand shader')['token']);session=runtime.create_session(actor);job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'material_compile',{'graphId':g})['jobId']);self.assertEqual(job['state'],'Completed',job);self.assertTrue(job['outputs']['success']);self.assertTrue(Path(job['outputs']['shaderPath']).read_bytes().startswith(b'DXBC'));runtime.finish_task(actor,session['sessionId'])
 def test_11_uv_outputs_remain_independent_and_sampling_context_survives(self):
  g=self.graph('UVExpand','material');self.function(g,'Coord',{'u':('float',.25),'v':('float',.75)},{'result':('float',0)});uv=self.node(g,'uv.transform',functionName='Coord');self.wire(g,'arguments','u',uv,'u','Coord');self.wire(g,'arguments','v',uv,'v','Coord');self.wire(g,uv,'u','return','result','Coord');n=self.node(g,'function.call',functionRef='Coord');bad=self.node(g,'math.divide',b=0);self.wire(g,bad,'value',n,'v');self.wire(g,n,'result','surface','roughness');self.assertFalse(self.plan(self.request(g,n))['valid'])
  link=next(l for l in self.get(g)['links'] if l['toNode']==n);self.unlink(g,link['id']);before=self.call('graph_run',{'graphId':g});r=self.request(g,n);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g}),before)
  g=self.graph('UVContext','material');self.function(g,'SampleU',{}, {'result':('float',0)});uv=self.node(g,'material.uv',functionName='SampleU');self.wire(g,uv,'u','return','result','SampleU');n=self.node(g,'function.call',functionRef='SampleU');self.wire(g,n,'result','surface','roughness');samples=[self.call('material_sample',{'graphId':g,'u':u,'v':v}) for u,v in ((.125,.875),(.75,.2))];r=self.request(g,n);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan)
  for (u,v),before in zip(((.125,.875),(.75,.2)),samples):self.assertEqual(self.call('material_sample',{'graphId':g,'u':u,'v':v}),before);self.assertAlmostEqual(before['roughness'],u)
 def test_12_stale_tampered_missing_and_revision_are_atomic(self):
  g,n,a,b=self.simple();r=self.request(g,n);plan=self.plan(r);before=self.get(g)
  for changed in ({**r,'nodeId':'missing'},{**r,'functionName':'Calculate'}):
   with self.assertRaises(CyToolError):self.apply(changed,plan)
   self.assertEqual(self.get(g),before)
  with self.assertRaises(CyToolError):self.apply(r,plan,expectedRevision=max(0,self.call('editor_status')['revision']-1))
  self.call('graph_node_comment',{'graphId':g,'nodeId':n,'text':'stale'});before=self.get(g)
  with self.assertRaises(CyToolError):self.apply(r,plan)
  self.assertEqual(self.get(g),before);self.apply(r,self.plan(r));self.assertFalse(self.plan(r)['valid'])
 def test_13_node_comment_and_layout_budgets_reject_without_mutation(self):
  g,n,a,b=self.simple();source=self.get(g);source['comments']=[{'id':str(i),'text':'x','position':[0,0],'size':[40,40]} for i in range(128)];g2=self.load(source);self.assertFalse(self.plan(self.request(g2,n))['valid']);source.pop('comments');body=source['functions']['Calculate'];template=next(x for x in body['nodes'] if x['id']==a)
  for i in range(127):body['nodes'].append({**copy.deepcopy(template),'id':'dead'+str(i)})
  g3=self.load(source);self.assertFalse(self.plan(self.request(g3,n))['valid']);source=self.get(g);next(x for x in source['nodes'] if x['id']==n)['position']=[99990,99990];g4=self.load(source);self.assertFalse(self.plan(self.request(g4,n))['valid'])
 def test_14_oversize_preview_is_bounded_even_when_invalid(self):
  g=self.graph('BoundedExpand');self.function(g,'Long',{}, {'result':('float',0)});value=self.node(g,'constant.float',functionName='Long',value=9);self.wire(g,value,'value','return','result','Long');n=self.node(g,'function.call',functionRef='Long');source=self.get(g);body=source['functions']['Long'];long='x'*270000;next(x for x in body['nodes'] if x['id']==value)['id']=long;body['links'][0]['fromNode']=long;g=self.load(source);plan=self.plan(self.request(g,n));self.assertFalse(plan['valid']);self.assertLessEqual(len(json.dumps(plan).encode()),256*1024);self.assertIn('256 KiB',plan['diagnostic'])
 def test_15_official_core_jobs_persistence_and_no_implicit_expansion(self):
  g,n,a,b=self.simple();r=self.request(g,n);before=self.get(g)
  with tool.create_runtime(self.workspace/'expand-core') as runtime:
   actor=runtime.authenticate(runtime.create_client('Expand author')['token']);session=runtime.create_session(actor)
   def run(op,p):
    job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],op,p)['jobId']);self.assertEqual(job['state'],'Completed',job);return job['outputs']
   plan=run('graph_expand_plan',r);self.assertEqual(self.get(g),before);self.assertTrue(run('graph_expand_apply',{**r,'planHash':plan['planHash']})['applied']);runtime.finish_task(actor,session['sessionId'])
  self.call('asset_save_graph',{'graphId':g});after=self.get(g);self.call('asset_open',{'path':after['assetPath']});self.assertEqual(self.get(g),after);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_16_shared_object_field_reads_survive(self):
  g=self.graph('ObjectExpand');self.call('graph_class_define',{'graphId':g,'name':'Entity','fields':{'health':{'type':'float','default':9}}});self.function(g,'Health',{'self':('object','')},{'result':('float',0)});read=self.node(g,'object.get.float',functionName='Health',field='health');self.wire(g,'arguments','self',read,'object','Health');self.wire(g,read,'value','return','result','Health');obj=self.node(g,'object.new',classRef='Entity');n=self.node(g,'function.call',functionRef='Health');self.unlink(g,'start');self.wire(g,'begin','then',obj,'exec');self.wire(g,obj,'then','print','exec');self.wire(g,obj,'object',n,'self');self.print_float(g,n);r=self.request(g,n);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_17_debugger_becomes_stale_and_dormant_calls_keep_dead_nodes(self):
  g,n,a,b=self.simple('macro');self.call('graph_debug_start',{'graphId':g});r=self.request(g,n);plan=self.plan(r);self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Paused');self.apply(r,plan);self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Stale');self.function(g,'Dormant',{}, {'result':('float',0)});dead=self.node(g,'math.divide',functionName='Dormant',b=0);n=self.node(g,'function.call',functionRef='Dormant');r=self.request(g,n);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertIn(dead,plan['nodeMap']);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_18_actual_canvas_modal_undo_redo_en_fr(self):
  if not self.gui:self.skipTest('Graphical host required')
  for lang in ('en','fr'):
   with tempfile.TemporaryDirectory(prefix='expand-ui-',dir=ROOT/'data/reports') as tmp:
    root=Path(tmp);(root/'app').mkdir();shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copytree(APP/'locales',root/'app/locales');report=root/'report.json';process=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--lang',lang,'--test-expand',str(report),'--exit-after','45'],timeout=60);self.assertEqual(process.returncode,0);self.assertTrue(report.exists());result=json.loads(report.read_text('utf-8'));self.assertTrue(result['passed'],result);self.assertEqual(len(result['checks']),16);shutil.copyfile(report,ROOT/'data/reports'/('expand-gestures-'+lang+'.json'));self.assertFalse((root/'data/settings/ui-preferences.json').exists());self.assertFalse((root/'data/settings/editor.ini').exists())
if __name__=='__main__':unittest.main(verbosity=2)
