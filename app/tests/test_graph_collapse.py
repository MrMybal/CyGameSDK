"""Pure graph region collapse through the native owner and official Core."""
import copy,json,os,shutil,subprocess,sys,tempfile,unittest,uuid
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool,test_integration as integration,test_authoring as authoring,test_type_migration as typed
from cytools_core import CyToolError
class Collapse(unittest.TestCase):
 @classmethod
 def setUpClass(cls):typed.TypeMigrations.setUpClass.__func__(cls)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call;node=typed.TypeMigrations.node;get=typed.TypeMigrations.get;wire=authoring.NativeAuthoring.wire;unlink=authoring.NativeAuthoring.unlink;function=authoring.NativeAuthoring.function;types=typed.TypeMigrations.types
 def graph(self,name,domain='logic'):return self.call('asset_new_graph',{'name':name,'domain':domain})['graphId']
 def simple(self,name='Collapse'):
  g=self.graph(name);outside=self.node(g,'constant.float',value=2);a=self.node(g,'math.add',b=1);b=self.node(g,'math.multiply',b=3);text=self.node(g,'convert.float_string');self.wire(g,outside,'value',a,'a');self.wire(g,a,'value',b,'a');self.wire(g,b,'value',text,'value');self.wire(g,text,'value','print','text');return g,a,b
 def request(self,g,ids,**kw):return {'graphId':g,'nodeIds':ids,'name':'Extracted','kind':'function',**kw}
 def plan(self,r):return self.call('graph_collapse_plan',r)
 def apply(self,r,plan,**kw):return self.call('graph_collapse_apply',{**r,'planHash':plan['planHash'],**kw})
 def load(self,source):
  source=copy.deepcopy(source);source.pop('assetPath',None);name='collapse-'+uuid.uuid4().hex+'.cyasset';path=self.workspace/'data/assets'/name;path.write_text(json.dumps({'format':'cygame-graph','schemaVersion':1,'graph':source}),'utf-8');return self.call('asset_open',{'path':name})['graphId']
 def test_01_preview_preserves_all_ids_metadata_fallbacks_positions_and_one_undo(self):
  g,a,b=self.simple();source=self.get(g);source['metadata']={'keep':7};source['comments']=[{'id':'group','text':'Keep at the caller','position':[0,0],'size':[400,400]}]
  for n in source['nodes']:n['metadata']={'keep':n['id']}
  for l in source['links']:l['metadata']={'keep':l['id']}
  g=self.load(source);r=self.request(g,[a,b]);before=self.get(g);history=self.call('edit_history');revision=self.call('editor_status')['revision'];plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertEqual(self.get(g),before);self.assertEqual(self.call('edit_history'),history);self.assertEqual(self.call('editor_status')['revision'],revision);self.assertEqual(plan,self.plan(r));self.assertEqual(plan['planHash'],self.plan({**r,'nodeIds':[b,a]})['planHash']);self.apply(r,plan);after=self.get(g);helper=after['functions']['Extracted'];self.assertEqual(after['metadata'],before['metadata']);self.assertEqual(after['comments'],before['comments'])
  for n in before['nodes']:
   actual=next(x for x in (helper['nodes'] if n['id'] in (a,b) else after['nodes']) if x['id']==n['id']);self.assertEqual(actual,n)
  all_links={l['id']:l for l in after['links']+helper['links']}
  for l in before['links']:self.assertEqual(all_links[l['id']]['metadata'],l['metadata'])
  self.assertEqual(len(all_links),len(before['links'])+2);self.assertEqual(helper['inputs']['in_1']['default'],0);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);self.assertEqual(len(self.call('edit_history')['undo']),len(history['undo'])+1);self.call('edit_undo');self.assertEqual(self.get(g),before);self.call('edit_redo');self.assertEqual(self.get(g),after)
 def test_02_macro_preserves_execution_and_debugger_is_stale(self):
  g,a,b=self.simple('MacroCollapse');self.call('graph_debug_start',{'graphId':g});r=self.request(g,[a,b],kind='macro');plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Paused');self.apply(r,plan);self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Stale');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);self.assertEqual(next(n for n in self.get(g)['nodes'] if n['id']==plan['callNodeId'])['type'],'macro.call')
 def test_03_extract_inside_pure_function_keeps_owner_libraries(self):
  g=self.graph('NestedCollapse');self.function(g,'Outer',{'x':('float',2)},{'result':('float',0)});a=self.node(g,'math.add',b=1,functionName='Outer');b=self.node(g,'math.multiply',b=3,functionName='Outer');self.wire(g,'arguments','x',a,'a','Outer');self.wire(g,a,'value',b,'a','Outer');self.wire(g,b,'value','return','result','Outer');r=self.request(g,[a,b],functionName='Outer');plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Outer'})['values']['result'],9);self.assertNotIn('functions',self.get(g)['functions']['Outer']);self.assertNotIn('types',self.get(g)['functions']['Extracted'])
 def test_04_pure_region_inside_execution_function_keeps_control_flow(self):
  g=self.graph('ExecutionRegion');self.function(g,'Work',{'x':('float',2)},{'result':('float',0)},mode='execution');a=self.node(g,'math.add',b=1,functionName='Work');b=self.node(g,'math.multiply',b=3,functionName='Work');self.wire(g,'arguments','x',a,'a','Work');self.wire(g,a,'value',b,'a','Work');self.wire(g,b,'value','return','result','Work');self.wire(g,'arguments','then','return','exec','Work');r=self.request(g,[a,b],functionName='Work');plan=self.plan(r);self.assertTrue(plan['valid'],plan);before=self.get(g)['functions']['Work']['links'];self.apply(r,plan);self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Work'})['values']['result'],9);self.assertTrue(all(l in self.get(g)['functions']['Work']['links'] for l in before if l['fromPin']=='then'))
 def test_05_material_function_cpu_and_real_shader_compile(self):
  g=self.graph('MaterialCollapse','material');self.unlink(g,'color_link');a=self.node(g,'math.multiply');b=self.node(g,'color.multiply');self.wire(g,a,'value',b,'factor');self.wire(g,'color','value',b,'color');self.wire(g,b,'value','surface','baseColor');before=self.call('graph_run',{'graphId':g});r=self.request(g,[a,b]);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g}),before)
  with tool.create_runtime(self.workspace/'collapse-shader') as runtime:
   actor=runtime.authenticate(runtime.create_client('Collapse shader')['token']);session=runtime.create_session(actor);job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'material_compile',{'graphId':g})['jobId']);self.assertEqual(job['state'],'Completed',job);self.assertTrue(job['outputs']['success']);self.assertTrue(Path(job['outputs']['shaderPath']).read_bytes().startswith(b'DXBC'));runtime.finish_task(actor,session['sessionId'])
 def test_06_named_records_enums_and_multi_output_boundaries(self):
  g=self.graph('NamedCollapse');self.types(g);member=self.node(g,'typed.literal',typeRef='enum.Team',value='B');a=self.node(g,'struct.make',typeRef='struct.Stats',health=9);b=self.node(g,'struct.break',typeRef='struct.Stats');text=self.node(g,'convert.float_string');enumtext=self.node(g,'enum.to_string',typeRef='enum.Team');self.wire(g,member,'value',a,'team');self.wire(g,a,'value',b,'value');self.wire(g,b,'health',text,'value');self.wire(g,text,'value','print','text');self.wire(g,b,'team',enumtext,'value');before=self.get(g);r=self.request(g,[a,b]);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertEqual({p['type'] for p in plan['outputs'].values()},{'float','enum.Team'});self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);after=self.get(g);self.assertEqual(after['types'],before['types']);self.assertNotIn('types',after['functions']['Extracted'])
 def test_07_fanout_and_distinct_authored_input_fallbacks_survive(self):
  g,a,b=self.simple('FanoutCollapse');source=self.get(g);outside=next(l['fromNode'] for l in source['links'] if l['toNode']==a);self.wire(g,outside,'value',a,'b');extra=self.node(g,'convert.float_string');self.wire(g,b,'value',extra,'value');r=self.request(g,[a,b]);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertEqual(len(plan['inputs']),2);self.assertEqual(len(plan['outputs']),1);self.assertEqual(next(iter(plan['outputs'].values()))['connections'],2);self.apply(r,plan);self.assertEqual({p['default'] for p in self.get(g)['functions']['Extracted']['inputs'].values()},{0,1});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['12'])
 def test_08_stale_tampered_revision_and_duplicate_name_are_atomic(self):
  g,a,b=self.simple('CollapseGuards');r=self.request(g,[a,b]);plan=self.plan(r);before=self.get(g)
  for request in ({**r,'name':'Different'},{**r,'kind':'macro'},{**r,'nodeIds':[a]}):
   with self.assertRaises(CyToolError):self.apply(request,plan)
   self.assertEqual(self.get(g),before)
  with self.assertRaises(CyToolError):self.apply(r,plan,expectedRevision=max(0,self.call('editor_status')['revision']-1))
  self.call('graph_node_comment',{'graphId':g,'nodeId':a,'text':'stale'});before=self.get(g)
  with self.assertRaises(CyToolError):self.apply(r,plan)
  self.assertEqual(self.get(g),before);self.apply(r,self.plan(r));self.assertFalse(self.plan(r)['valid'])
 def test_09_boundaries_execution_variables_dead_regions_and_bad_identifiers_reject(self):
  g,a,b=self.simple('BadCollapse');self.call('graph_variable_add',{'graphId':g,'name':'Value','type':'float','default':2});variable=self.node(g,'variable.get.float');dead=self.node(g,'constant.float');before=self.get(g)
  for ids in (['begin'],['print'],[variable],[a,b,dead],['missing']):
   r=self.request(g,ids);plan=self.plan(r);self.assertFalse(plan['valid'],plan)
   with self.assertRaises(CyToolError):self.apply(r,plan)
   self.assertEqual(self.get(g),before)
  self.assertFalse(self.plan(self.request(g,[a,b],name='Bad name'))['valid']);self.function(g,'Outer',{}, {'result':('float',0)});self.assertFalse(self.plan(self.request(g,['arguments'],functionName='Outer'))['valid'])
 def test_10_lazy_uv_output_does_not_promote_an_unused_failing_input(self):
  g=self.graph('LazyCollapse','material');self.unlink(g,'color_link');bad=self.node(g,'math.divide',b=0);uv=self.node(g,'uv.transform');gradient=self.node(g,'pattern.gradient');self.wire(g,bad,'value',uv,'v');self.wire(g,uv,'u',gradient,'u');self.wire(g,gradient,'value','surface','baseColor');before=self.get(g);original=self.call('graph_run',{'graphId':g});r=self.request(g,[uv]);plan=self.plan(r);self.assertFalse(plan['valid'],plan)
  with self.assertRaises(CyToolError):self.apply(r,plan)
  self.assertEqual(self.get(g),before);self.assertEqual(self.call('graph_run',{'graphId':g}),original)
 def test_11_function_and_port_budgets_are_not_expanded(self):
  g,a,b=self.simple('CollapseBudget')
  for i in range(32):self.call('graph_function_create',{'graphId':g,'name':'F'+str(i)})
  self.assertFalse(self.plan(self.request(g,[a,b]))['valid']);source=self.get(g);external=next(n['id'] for n in source['nodes'] if n['type']=='constant.float');long_id='X'*270000
  for n in source['nodes']:
   if n['id']==external:n['id']=long_id
  for link in source['links']:
   for key in ('fromNode','toNode'):
    if link[key]==external:link[key]=long_id
  owner=self.load(source);bounded=self.plan(self.request(owner,[a,b]));self.assertFalse(bounded['valid']);self.assertLessEqual(len(json.dumps(bounded).encode()),256*1024);g=self.graph('CollapsePorts');outside=self.node(g,'constant.float');ids=[]
  for i in range(17):
   n=self.node(g,'math.add');self.wire(g,outside,'value',n,'a')
   if ids:self.wire(g,ids[-1],'value',n,'b')
   ids.append(n)
  text=self.node(g,'convert.float_string');self.wire(g,ids[-1],'value',text,'value');self.assertFalse(self.plan(self.request(g,ids))['valid'])
 def test_12_official_core_persistence_and_no_migration_on_load(self):
  g,a,b=self.simple('CollapseCore');r=self.request(g,[a,b]);before=self.get(g)
  with tool.create_runtime(self.workspace/'collapse-core') as runtime:
   actor=runtime.authenticate(runtime.create_client('Collapse author')['token']);session=runtime.create_session(actor)
   def run(op,params):
    job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],op,params)['jobId']);self.assertEqual(job['state'],'Completed',job);return job['outputs']
   plan=run('graph_collapse_plan',r);self.assertEqual(self.get(g),before);self.assertTrue(run('graph_collapse_apply',{**r,'planHash':plan['planHash']})['applied']);runtime.finish_task(actor,session['sessionId'])
  self.call('asset_save_graph',{'graphId':g});after=self.get(g);self.call('asset_open',{'path':after['assetPath']});self.assertEqual(self.get(g),after);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_13_shared_runtime_object_fields_remain_live_in_extracted_function(self):
  g=self.graph('ObjectCollapse');self.call('graph_class_define',{'graphId':g,'name':'Entity','fields':{'health':{'type':'float','default':4.5}}});obj=self.node(g,'object.new',classRef='Entity');a=self.node(g,'object.get.float',field='health');b=self.node(g,'math.multiply',b=2);text=self.node(g,'convert.float_string');self.unlink(g,'start');self.wire(g,'begin','then',obj,'exec');self.wire(g,obj,'then','print','exec');self.wire(g,obj,'object',a,'object');self.wire(g,a,'value',b,'a');self.wire(g,b,'value',text,'value');self.wire(g,text,'value','print','text');r=self.request(g,[a,b]);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_14_actual_canvas_selection_modal_apply_undo_redo_function_macro_en_fr(self):
  if not self.gui:self.skipTest('Graphical host required')
  for lang in ('en','fr'):
   with tempfile.TemporaryDirectory(prefix='collapse-ui-',dir=ROOT/'data/reports') as tmp:
    root=Path(tmp);(root/'app').mkdir();shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copytree(APP/'locales',root/'app/locales');report=root/'report.json';process=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--lang',lang,'--test-collapse',str(report),'--exit-after','45'],timeout=60);self.assertEqual(process.returncode,0);self.assertTrue(report.exists());result=json.loads(report.read_text('utf-8'));self.assertTrue(result['passed'],result);self.assertEqual(len(result['checks']),14);shutil.copyfile(report,ROOT/'data/reports'/('collapse-gestures-'+lang+'.json'));self.assertFalse((root/'data/settings/ui-preferences.json').exists());self.assertFalse((root/'data/settings/editor.ini').exists())
if __name__=='__main__':unittest.main(verbosity=2)
