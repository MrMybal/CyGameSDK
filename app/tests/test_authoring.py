"""Native Blueprint semantics and official Core authoring/codec jobs."""
import base64,copy,hashlib,json,math,os,shutil,sys,tempfile,time,unittest,uuid
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool,scene3d,model_authoring as authoring
import test_integration as integration
from cytools_core import CyToolError
from game_codecs import GameCodecProvider

class Algorithms(unittest.TestCase):
 def test_01_lod_reduces_geometry_preserves_boundary_and_portable_metadata(self):
  model=json.loads((APP/'examples/Grid_LOD.cy3d').read_text('utf-8'));before=copy.deepcopy(model)
  out,summary=authoring.process(model,None,'lod',{'ratio':.5});self.assertEqual(model,before);self.assertLess(summary['triangles'],800);self.assertGreater(summary['triangles'],0)
  p=out['extensions'][authoring.NS]['lods']['0'][0]['primitives'][0]
  self.assertEqual(len(p['positions']),len(p['normals']));self.assertTrue(all(0<=i<len(p['positions']) for i in p['indices']))
  original=model['meshes'][0]['primitives'][0]['positions'];edge={tuple(v) for v in original if abs(v[0])==2 or abs(v[2])==2};self.assertTrue(edge<={tuple(v) for v in p['positions']})
  with tempfile.TemporaryDirectory(dir=ROOT/'data/reports') as temp:
   path=Path(temp)/'lod.glb';scene3d.export(authoring.select_lod(out,1),path);decoded=scene3d.decode(path);self.assertEqual(len(decoded['meshes'][0]['primitives'][0]['indices'])//3,summary['triangles']);shutil.copyfile(path,ROOT/'data/reports/authoring-lod.glb')
   full=Path(temp)/'full.cypack';scene3d.export(out,full);self.assertEqual(scene3d.decode(full)['extensions'],out['extensions'])
 def test_02_collision_conservative_fits_and_unsupported_inputs(self):
  model=json.loads((APP/'examples/Grid_LOD.cy3d').read_text('utf-8'))
  for kind in ('box','sphere','capsule'):
   out,_=authoring.process(model,None,'collision',{'shape':kind});shape=out['extensions'][authoring.NS]['collisions'][0]
   for point in model['meshes'][0]['primitives'][0]['positions']:
    relative=authoring.sub(point,shape['center'])
    if kind=='box':self.assertTrue(all(abs(x)<=e+1e-6 for x,e in zip(relative,shape['halfExtents'])))
    elif kind=='sphere':self.assertLessEqual(authoring.length(relative),shape['radius']+1e-6)
    else:self.assertLessEqual(math.sqrt(relative[0]**2+relative[2]**2+max(0,abs(relative[1])-shape['halfHeight'])**2),shape['radius']+1e-6)
  animated=json.loads((APP/'examples/AnimatedTriangle.cy3d').read_text('utf-8'))
  with self.assertRaises(ValueError):authoring.process(animated,None,'lod',{'ratio':.5})
  with self.assertRaises(ValueError):authoring.process(animated,None,'collision',{})
 def test_03_retarget_preserves_lengths_reference_identity_and_normalized_rotations(self):
  source=json.loads((APP/'examples/AnimatedTriangle.cy3d').read_text('utf-8'));target=json.loads((APP/'examples/TallTarget.cy3d').read_text('utf-8'));original=copy.deepcopy(target)
  out,report=authoring.process(target,source,'retarget',{'mapping':{'1':1,'2':2},'fps':30})
  self.assertEqual(target,original);self.assertEqual(out['assetId'],target['assetId']);self.assertEqual(out['nodes'],target['nodes'])
  for time in (0,.25,.5,1):
   _,rest=authoring.pose(target);_,pose=authoring.pose(out,out['animations'][0],time);self.assertAlmostEqual(authoring.length(authoring.sub(pose[2][0],pose[1][0])),2,places=5)
   for track in out['animations'][0]['tracks']:
    for value in track['values']:self.assertAlmostEqual(authoring.length(value),1,places=5)
  self.assertNotEqual(out['animations'][0]['tracks'][1]['values'][0],out['animations'][0]['tracks'][1]['values'][15])
  rotated=copy.deepcopy(target);rotated['nodes'][2]['rotation']=[math.sqrt(.5),0,0,math.sqrt(.5)];turned,_=authoring.process(rotated,source,'retarget',{'mapping':{'1':1,'2':2}});_,rest_world=authoring.pose(rotated);_,source_world=authoring.pose(source,source['animations'][0],.5);_,turned_world=authoring.pose(turned,turned['animations'][0],.5);expected=authoring.qm(rest_world[2][1],source_world[2][1]);self.assertAlmostEqual(abs(authoring.dot(expected,turned_world[2][1])),1,places=5)
  with tempfile.TemporaryDirectory(dir=ROOT/'data/reports') as temp:
   path=Path(temp)/'retarget.glb';scene3d.export(out,path);decoded=scene3d.decode(path);self.assertEqual(len(decoded['animations']),1);self.assertEqual(decoded['skins'],out['skins']);shutil.copyfile(path,ROOT/'data/reports/authoring-retarget.glb')
  with self.assertRaises(ValueError):authoring.process(target,source,'retarget',{'mapping':{'1':1,'2':1}})

class NativeAuthoring(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  integration.EditorIntegration.setUpClass.__func__(cls);shutil.copytree(APP/'examples',cls.workspace/'data/assets/Examples',dirs_exist_ok=True)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call
 def graph(self,name):return self.call('asset_new_graph',{'name':name,'domain':'logic'})['graphId']
 def node(self,g,type,**values):
  p={'graphId':g,'type':type,'position':[100,100]};p.update({k:values.pop(k) for k in ('functionRef','classRef','methodRef','functionName') if k in values});id=self.call('graph_node_add',p)['node']['id']
  for key,value in values.items():self.call('graph_node_value',{'graphId':g,'nodeId':id,'key':key,'value':value,**({'functionName':p['functionName']} if 'functionName' in p else {})})
  return id
 def wire(self,g,a,ap,b,bp,fn=''):
  p={'graphId':g,'fromNode':a,'fromPin':ap,'toNode':b,'toPin':bp}
  if fn:p['functionName']=fn
  return self.call('graph_link_add',p)
 def unlink(self,g,id,fn=''):
  return self.call('graph_link_remove',{'graphId':g,'linkId':id,**({'functionName':fn} if fn else {})})
 def function(self,g,name,inputs,outputs,mode='pure',kind='function'):
  self.call('graph_function_create',{'graphId':g,'name':name,'mode':mode,'kind':kind})
  body=self.call('graph_get',{'graphId':g,'functionName':name})['graph']
  for l in body['links']:self.unlink(g,l['id'],name)
  self.call('graph_function_signature',{'graphId':g,'name':name,'inputs':{k:{'type':v[0],'default':v[1]} for k,v in inputs.items()},'outputs':{k:{'type':v[0],'default':v[1]} for k,v in outputs.items()}})
 def print_value(self,g,node,pin,type='float'):
  convert=self.node(g,'convert.'+type+'_string');self.wire(g,node,pin,convert,'value');self.wire(g,convert,'value','print','text')
 def model(self,name):return self.call('model_open',{'path':'Examples/'+name+'.cy3d'})['modelId']
 def process(self,runtime,actor,session,parameters):
  result=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'model_process',parameters)['jobId']);self.assertEqual(result['state'],'Completed',result);return result
 def test_01_typed_containers_functions_and_invalid_values_rollback(self):
  g=self.graph('Containers');literal=self.node(g,'constant.array.float',value=[1,2]);append=self.node(g,'array.add.float',item=3);length=self.node(g,'array.length.float');self.wire(g,literal,'value',append,'container');self.wire(g,append,'value',length,'container');self.print_value(g,length,'value','int');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['3'])
  before=self.call('graph_get',{'graphId':g})['graph']
  for typ,value in [('set.string',['a','a']),('array.int',[1.5]),('map.float',{'a':'bad'})]:
   with self.assertRaises(CyToolError):self.call('graph_variable_add',{'graphId':g,'name':'Bad','type':typ,'default':value})
  self.assertEqual(self.call('graph_get',{'graphId':g})['graph'],before)
  self.function(g,'Echo',{'items':('array.float',[])},{'result':('array.float',[])})
  self.wire(g,'arguments','items','return','result','Echo');self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Echo','arguments':{'items':[3,4]}})['values']['result'],[3,4])
  self.call('graph_variable_add',{'graphId':g,'name':'Dictionary','type':'map.float','default':{'health':100}})
 def test_02_classes_shared_references_inheritance_and_virtual_method_dispatch(self):
  g=self.graph('Objects')
  for name in ('ReadBase','ReadDerived'):
   self.function(g,name,{'self':('object','')},{'result':('float',0)})
  getter=self.node(g,'object.get.float',functionName='ReadBase',field='health');self.wire(g,'arguments','self',getter,'object','ReadBase');self.wire(g,getter,'value','return','result','ReadBase')
  constant=self.node(g,'constant.float',functionName='ReadDerived',value=500);self.wire(g,constant,'value','return','result','ReadDerived')
  self.call('graph_class_define',{'graphId':g,'name':'Base','fields':{'health':{'type':'float','default':100}},'methods':{'Read':'ReadBase'}})
  self.call('graph_class_define',{'graphId':g,'name':'Derived','parent':'Base','fields':{'health':{'type':'float','default':250}},'methods':{'Read':'ReadDerived'}})
  obj=self.node(g,'object.new',classRef='Derived');method=self.node(g,'object.call',classRef='Base',methodRef='Read');self.wire(g,obj,'object',method,'self');self.print_value(g,method,'result');self.unlink(g,'start');self.wire(g,'begin','then',obj,'exec');self.wire(g,obj,'then','print','exec');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['500'])
  self.call('graph_debug_start',{'graphId':g});state=self.call('graph_debug_continue',{'graphId':g});self.assertEqual(next(iter(state['objects'].values()))['fields']['health'],250)
  handle=next(iter(state['objects']));self.call('graph_debug_object_set',{'graphId':g,'object':handle,'field':'health','value':300});self.assertEqual(self.call('graph_debug_status',{'graphId':g})['objects'][handle]['fields']['health'],300)
  self.call('graph_debug_stop',{'graphId':g});self.call('graph_debug_start',{'graphId':g});fresh=self.call('graph_debug_continue',{'graphId':g});self.assertNotIn(handle,fresh['objects'])
  with self.assertRaises(CyToolError):self.call('graph_debug_object_set',{'graphId':g,'object':handle,'field':'health','value':1})
  before=self.call('graph_get',{'graphId':g})['graph']
  with self.assertRaises(CyToolError):self.call('graph_class_define',{'graphId':g,'name':'Base','parent':'Derived','fields':{}})
  with self.assertRaises(CyToolError):self.call('graph_class_remove',{'graphId':g,'name':'Derived'})
  self.assertEqual(self.call('graph_get',{'graphId':g})['graph'],before)
  fragment=self.call('graph_fragment_copy',{'graphId':g,'nodeIds':[obj,method]})['fragment'];other=self.graph('PastedClasses');self.call('graph_fragment_paste',{'graphId':other,'fragment':fragment});self.assertIn('Derived',self.call('graph_get',{'graphId':other})['graph']['classes'])
 def test_03_macros_expand_inline_and_debugger_matches_source(self):
  g=self.graph('Macros');self.function(g,'Twice',{'x':('float',0)},{'result':('float',0)},kind='macro')
  multiply=self.node(g,'math.multiply',functionName='Twice',b=2);self.wire(g,'arguments','x',multiply,'a','Twice');self.wire(g,multiply,'value','return','result','Twice')
  call=self.node(g,'macro.call',functionRef='Twice',x=4);self.print_value(g,call,'result');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['8'])
  self.call('graph_debug_start',{'graphId':g});self.assertEqual(self.call('graph_debug_continue',{'graphId':g})['state'],'Completed');self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Twice','arguments':{'x':3}})['values']['result'],6)
  self.call('asset_save_graph',{'graphId':g});self.call('edit_undo')
 def test_04_latent_time_and_custom_events_are_nonblocking(self):
  g=self.graph('Latent');delay=self.node(g,'flow.delay',seconds=1);self.unlink(g,'start');self.wire(g,'begin','then',delay,'exec');self.wire(g,delay,'then','print','exec')
  event=self.node(g,'event.custom',name='Interact');printer=self.node(g,'debug.print',text='event');self.wire(g,event,'then',printer,'exec')
  self.call('graph_debug_start',{'graphId':g});start=time.monotonic();self.assertEqual(self.call('graph_debug_continue',{'graphId':g})['state'],'Waiting');self.assertLess(time.monotonic()-start,1)
  self.assertEqual(self.call('graph_debug_tick',{'graphId':g,'delta':.4})['state'],'Waiting');state=self.call('graph_debug_tick',{'graphId':g,'delta':.6});self.assertEqual(state['state'],'Completed');self.assertEqual(len(state['messages']),1)
  self.call('graph_debug_event',{'graphId':g,'event':'Interact'});state=self.call('graph_debug_continue',{'graphId':g});self.assertEqual(state['messages'][-1],'event');self.assertEqual(len(state['messages']),2)
 def test_05_foreach_function_loop_and_early_break(self):
  g=self.graph('Loop');self.function(g,'Visit',{'item':('float',0),'index':('int',0)},{'break':('bool',False)},mode='execution')
  convert=self.node(g,'convert.float_string',functionName='Visit');printer=self.node(g,'debug.print',functionName='Visit');self.wire(g,'arguments','item',convert,'value','Visit');self.wire(g,convert,'value',printer,'text','Visit');self.wire(g,'arguments','then',printer,'exec','Visit');self.wire(g,printer,'then','return','exec','Visit')
  loop=self.node(g,'flow.foreach',functionRef='Visit',values=[3,5,8]);self.unlink(g,'start');self.wire(g,'begin','then',loop,'exec');self.wire(g,loop,'then','print','exec');self.print_value(g,loop,'count','int');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['3','5','8','3'])
  self.call('graph_node_value',{'graphId':g,'functionName':'Visit','nodeId':'return','key':'break','value':True});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['3','1'])
  self.call('graph_node_value',{'graphId':g,'functionName':'Visit','nodeId':'return','key':'break','value':False});self.call('graph_node_value',{'graphId':g,'nodeId':loop,'key':'values','value':list(range(256))})
  link=next(l for l in self.call('graph_get',{'graphId':g})['graph']['links'] if l['fromNode']=='begin' and l['toNode']==loop);self.unlink(g,link['id']);event=self.node(g,'event.custom',name='Repeat');self.wire(g,event,'then',loop,'exec');self.call('graph_debug_start',{'graphId':g});self.call('graph_debug_continue',{'graphId':g})
  for _ in range(8):
   self.call('graph_debug_event',{'graphId':g,'event':'Repeat'});state=self.call('graph_debug_continue',{'graphId':g});self.assertEqual(state['state'],'Completed',state)
  self.assertGreater(state['steps'],4096)
  words=self.graph('StringLoop');self.function(words,'PrintWord',{'item':('string',''),'index':('int',0)},{},mode='execution');printer=self.node(words,'debug.print',functionName='PrintWord');self.wire(words,'arguments','item',printer,'text','PrintWord');self.wire(words,'arguments','then',printer,'exec','PrintWord');self.wire(words,printer,'then','return','exec','PrintWord');loop=self.node(words,'flow.foreach',functionRef='PrintWord',values=['alpha','beta']);self.unlink(words,'start');self.wire(words,'begin','then',loop,'exec');self.assertEqual(self.call('graph_run',{'graphId':words})['messages'],['alpha','beta'])
 def test_06_official_lod_collision_jobs_undo_hash_guards_and_export(self):
  mid=self.model('Grid_LOD')
  with tool.create_runtime(self.workspace/'authoring-jobs') as runtime:
   actor=runtime.authenticate(runtime.create_client('Author')['token']);other=runtime.authenticate(runtime.create_client('Other')['token']);session=runtime.create_session(actor)
   job=self.process(runtime,actor,session,{'modelId':mid,'action':'lod','options':{'ratio':.5}})
   with self.assertRaises(CyToolError):runtime.get_job(other,job['jobId'])
   frame=self.call('model_sample',{'modelId':mid,'lod':1});self.assertLess(frame['triangleCount'],800)
   self.call('edit_undo');self.assertEqual(self.call('model_sample',{'modelId':mid,'lod':1})['triangleCount'],800);self.call('edit_redo')
   self.process(runtime,actor,session,{'modelId':mid,'action':'collision','options':{'shape':'capsule'}})
   model=self.call('model_get',{'modelId':mid})['model'];shape=model['extensions'][authoring.NS]['collisions'][0];self.call('model_collision_set',{'modelId':mid,'collisionId':shape['id'],'radius':3})
   before=self.call('model_get',{'modelId':mid})['model']
   with self.assertRaises(CyToolError):self.call('model_collision_set',{'modelId':mid,'collisionId':shape['id'],'radius':0})
   self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before)
   self.call('model_vertex_set',{'modelId':mid,'mesh':0,'primitive':0,'vertex':0,'positions':[-2,.05,-2]})
   with self.assertRaises(CyToolError):self.call('model_sample',{'modelId':mid,'lod':1})
   self.assertTrue(self.call('model_get',{'modelId':mid})['model']['extensions'][authoring.NS]['collisions'][0]['needsReview']);self.call('edit_undo');self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before)
   self.call('model_save',{'modelId':mid});job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'model_export',{'modelId':mid,'format':'glb','lod':1})['jobId']);self.assertEqual(job['state'],'Completed',job)
   output=Path(job['outputs']['outputPath']);self.assertEqual(len(scene3d.decode(output)['meshes'][0]['primitives'][0]['indices'])//3,frame['triangleCount']);runtime.finish_task(actor,session['sessionId']);self.assertFalse(output.exists())
  plan=self.call('model_process_prepare',{'modelId':mid,'action':'collision','options':{}});result,summary=authoring.process(plan['model'],None,'collision',{})
  report=self.workspace/'stale-authoring.json';report.write_text(json.dumps({'format':'cygame-model-process','schemaVersion':1,'modelId':mid,'baseHash':plan['baseHash'],'sourceId':'','sourceHash':'','model':result,'summary':summary}),encoding='utf-8')
  self.call('model_node_set',{'modelId':mid,'node':0,'name':'Changed during work'})
  with self.assertRaises(CyToolError):self.call('model_process_commit',{'reportPath':str(report)})
  if self.gui:self.call('panel_open',{'panelId':'model:'+mid});self.call('model_view_settings',{'modelId':mid,'tab':'LODs','lod':1})
 def test_07_official_retarget_job_and_skinning_on_target(self):
  source=self.model('AnimatedTriangle');target=self.model('TallTarget')
  with tool.create_runtime(self.workspace/'retarget-jobs') as runtime:
   actor=runtime.authenticate(runtime.create_client('Retarget')['token']);session=runtime.create_session(actor)
   self.process(runtime,actor,session,{'modelId':target,'sourceModelId':source,'action':'retarget','options':{'mapping':{'1':1,'2':2},'fps':30}})
   self.assertNotEqual(self.call('model_sample',{'modelId':target,'animation':0,'time':0})['positions'],self.call('model_sample',{'modelId':target,'animation':0,'time':.5})['positions'])
   self.call('model_save',{'modelId':target})
   if self.gui:self.call('panel_open',{'panelId':'model:'+target});self.call('model_view_settings',{'modelId':target,'tab':'Retarget'})
 def test_08_game_codec_registry_missing_provider_and_verified_fixture_worker(self):
  mid=self.model('Grid_LOD')
  descriptor={'id':'test.fixture','gameId':'test-only-not-a-game','versions':['fixture-v1'],'platforms':['Test'],'extension':'.testmesh','optionsSchema':{'type':'object','properties':{'corruptReport':{'type':'boolean'},'unverified':{'type':'boolean'}},'additionalProperties':False},'maxBytes':1024}
  provider=GameCodecProvider(descriptor,[sys.executable,str(APP/'tests/fixture_codec_worker.py')])
  with tool.create_runtime(self.workspace/'codec-jobs',game_codecs={descriptor['id']:provider}) as runtime:
   actor=runtime.authenticate(runtime.create_client('Codec')['token']);session=runtime.create_session(actor)
   listing=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'game_codec_list',{})['jobId']);self.assertEqual(len(listing['outputs']['codecs']),1)
   p={'modelId':mid,'codecId':descriptor['id'],'gameVersion':'fixture-v1','platform':'Test','acceptedLosses':['Synthetic fixture stores only node count and identity']}
   for extra in ({'acceptedLosses':[]},{'codecId':'missing'},{'options':{'corruptReport':True}},{'options':{'unverified':True}}):
    denied=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'game_codec_export',{**p,**extra})['jobId']);self.assertEqual(denied['state'],'Failed',denied)
   job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'game_codec_export',p)['jobId']);self.assertEqual(job['state'],'Completed',job);path=Path(job['outputs']['outputPath']);self.assertTrue(path.read_bytes().startswith(b'CYSDKTEST'))
   bad=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'game_codec_export',{**p,'gameVersion':'unknown'})['jobId']);self.assertEqual(bad['state'],'Failed');runtime.finish_task(actor,session['sessionId']);self.assertFalse(path.exists())

 def test_09_graphical_type_switch_undo_and_macro_creation(self):
  if not self.gui:self.skipTest('Graphical host required')
  import subprocess
  root=self.workspace/'blueprint-gestures';(root/'app').mkdir(parents=True);shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copytree(APP/'locales',root/'app/locales');shutil.copyfile(APP/'interface_features.json',root/'app/interface_features.json')
  report=ROOT/'data/reports/authoring-blueprint-gestures.json';report.unlink(missing_ok=True)
  result=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--test-blueprint',str(report),'--exit-after','25'],timeout=40)
  fatal=root/'data/logs/native-fatal.json'
  if fatal.exists():shutil.copyfile(fatal,ROOT/'data/reports/authoring-blueprint-fatal.json')
  self.assertEqual(result.returncode,0,report.read_text('utf-8') if report.exists() else fatal.read_text('utf-8') if fatal.exists() else 'No gesture report');self.assertTrue(json.loads(report.read_text('utf-8'))['passed'])

if __name__=='__main__':unittest.main(verbosity=2)
