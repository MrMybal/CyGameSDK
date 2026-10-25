"""Neutral 3D codec and native transaction/worker roundtrips."""
import copy,json,math,shutil,sys,tempfile,time,unittest,uuid
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import scene3d,tool
import test_integration as integration
from cytools_core import CyToolError
from portable_assets.package import read_package

class CodecTests(unittest.TestCase):
 def test_01_static_and_skinned_decode_export_identity_metadata(self):
  with tempfile.TemporaryDirectory(prefix='codec-',dir=ROOT/'data/reports') as temp:
   for name in ('StaticTriangle','AnimatedTriangle'):
    source=APP/'examples/portable'/(name+'.cypack');manifest,_=read_package(source);model=scene3d.decode(source)
    self.assertEqual(model['assetId'],manifest['assetId']);target=Path(temp)/(name+'.cypack');scene3d.export(model,target)
    result=scene3d.decode(target);self.assertEqual(result['assetId'],model['assetId']);self.assertEqual(result['extensions'],model['extensions']);self.assertEqual(result['meshes'],model['meshes']);self.assertEqual(result['skins'],model['skins']);self.assertEqual(result['animations'],model['animations'])
 def test_02_vertex_node_and_animation_edits_survive_real_glb(self):
  with tempfile.TemporaryDirectory(prefix='codec-edit-',dir=ROOT/'data/reports') as temp:
   model=scene3d.decode(APP/'examples/portable/AnimatedTriangle.cypack');model['meshes'][0]['primitives'][0]['positions'][0]=[-2,0,0];model['nodes'][2]['name']='Tip_模型';model['animations'][0]['tracks'][0]['times'][1]=.6
   target=Path(temp)/'edited.glb';scene3d.export(model,target);result=scene3d.decode(target)
   self.assertEqual(result['meshes'][0]['primitives'][0]['positions'][0],[-2,0,0]);self.assertEqual(result['nodes'][2]['name'],'Tip_模型');self.assertAlmostEqual(result['animations'][0]['tracks'][0]['times'][1],.6,places=6)
   shutil.copyfile(target,ROOT/'data/reports/completion-edited.glb')
 def test_03_export_never_removes_preexisting_temporary_or_destination(self):
  with tempfile.TemporaryDirectory(prefix='codec-existing-',dir=ROOT/'data/reports') as temp:
   model=scene3d.decode(APP/'examples/portable/StaticTriangle.cypack');target=Path(temp)/'existing.cypack';temporary=target.with_name(target.name+'.source.glb');temporary.write_bytes(b'user source')
   with self.assertRaises(FileExistsError):scene3d.export(model,target)
   self.assertEqual(temporary.read_bytes(),b'user source');self.assertFalse(target.exists());target.write_bytes(b'user destination')
   with self.assertRaises(ValueError):scene3d.export(model,target)
   self.assertEqual(target.read_bytes(),b'user destination')
 def test_04_external_uri_corruption_and_unsupported_topology_reject(self):
  from portable_assets.glb import AssetError
  for source in (b'not glb',b'glTF'+b'\0'*32):
   with self.assertRaises(AssetError):scene3d.unpack_glb(source)
  model=scene3d.decode(APP/'examples/portable/StaticTriangle.cypack');self.assertEqual(model['space']['upAxis'],'+Y')

class Native3DTests(unittest.TestCase):
 setUpClass=classmethod(integration.EditorIntegration.setUpClass.__func__)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call
 def model(self,name):
  model=json.loads((APP/'examples/AnimatedTriangle.cy3d').read_text('utf-8'));model['assetId']=str(uuid.uuid4());path=self.workspace/'data/assets/Models'/(name+'.cy3d');path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(model,separators=(',',':')),'utf-8');return self.call('model_open',{'path':'Models/'+path.name})['modelId'],path
 def test_01_native_skinning_animation_vertex_edit_undo_and_validation(self):
  mid,path=self.model('Editable');rest=self.call('model_sample',{'modelId':mid,'animation':-1,'time':0});pose=self.call('model_sample',{'modelId':mid,'animation':0,'time':.5})
  self.assertEqual(rest['triangleCount'],1);self.assertNotEqual(rest['positions'],pose['positions'])
  before=self.call('model_get',{'modelId':mid})['model'];revision=self.call('editor_status')['revision'];self.call('model_vertex_set',{'modelId':mid,'mesh':0,'primitive':0,'vertex':0,'positions':[-2,0,0],'expectedRevision':revision});self.call('edit_undo');self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before)
  for p in ({'node':1,'parent':2},{'node':2,'rotation':[0,0,0,0]}):
   with self.assertRaises(CyToolError):self.call('model_node_set',{'modelId':mid,**p})
  with self.assertRaises(CyToolError):self.call('model_key_set',{'modelId':mid,'animation':0,'track':0,'key':1,'time':2})
  self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before)
  self.call('model_vertex_set',{'modelId':mid,'mesh':0,'primitive':0,'vertex':0,'normals':[0,0,2],'weights':[1,1,0,0],'joints':[0,1,0,0]});normalized=self.call('model_get',{'modelId':mid})['model']['meshes'][0]['primitives'][0];self.assertEqual(normalized['normals'][0],[0,0,1]);self.assertEqual(normalized['weights'][0],[.5,.5,0,0])
  self.call('model_key_set',{'modelId':mid,'animation':0,'track':0,'key':1,'value':[0,0,.5,1]});self.call('model_save',{'modelId':mid});self.assertEqual(json.loads(path.read_text())['assetId'],mid)
 def test_02_real_core_glb_and_cypack_import_and_export_privacy(self):
  with tool.create_runtime(self.workspace/'model-jobs') as runtime:
   actor=runtime.authenticate(runtime.create_client('Model owner')['token']);other=runtime.authenticate(runtime.create_client('Other')['token']);session=runtime.create_session(actor)
   submitted=runtime.submit(actor,session['sessionId'],'asset_import',{'sourcePath':str(APP/'examples/portable/AnimatedTriangle.cypack'),'destination':'Models/Imported.cy3d'})
   job=runtime.wait(actor,submitted['jobId']);self.assertEqual(job['state'],'Completed',job)
   mid=self.call('model_open',{'path':'Models/Imported.cy3d'})['modelId'];self.call('model_node_set',{'modelId':mid,'node':2,'name':'Edited Tip'})
   submitted=runtime.submit(actor,session['sessionId'],'model_export',{'modelId':mid,'format':'cypack'});job=runtime.wait(actor,submitted['jobId']);self.assertEqual(job['state'],'Completed',job)
   with self.assertRaises(CyToolError):runtime.get_job(other,job['jobId'])
   output=Path(job['outputs']['outputPath']);result=scene3d.decode(output);self.assertEqual(result['assetId'],mid);self.assertEqual(result['nodes'][2]['name'],'Edited Tip')
   deadline=time.monotonic()+5
   while True:
    try:exported=runtime.export_job(actor,job['jobId'],self.workspace/'model-export');break
    except CyToolError as error:
     if error.code!='Busy' or time.monotonic()>deadline:raise
     time.sleep(.02)
   runtime.finish_task(actor,session['sessionId']);self.assertFalse(output.exists());self.assertTrue(Path(exported['outputs']['outputPath']).exists())
 def test_03_identity_conflicts_and_path_confinement(self):
  mid,path=self.model('Identity');duplicate=path.with_name('Conflict.cy3d');duplicate.write_bytes(path.read_bytes())
  for target in ('Models/Conflict.cy3d','../ipc/editor.json',str(APP/'examples/AnimatedTriangle.cy3d')):
   with self.assertRaises(CyToolError):self.call('model_open',{'path':target})
 def test_04_json_tree_transactions_external_edits_and_save_after_undo(self):
  path=self.workspace/'data/assets/Example.json';path.write_text('{"nested":{"v":1},"items":[true,2],"a/b":{"~":0}}','utf-8');jid=self.call('json_open',{'path':'Example.json'})['jsonId']
  self.call('json_value_set',{'jsonId':jid,'pointer':'/nested/v','value':4});self.call('json_save',{'jsonId':jid});self.call('edit_undo');self.assertEqual(self.call('json_get',{'jsonId':jid})['value']['nested']['v'],1);self.call('json_save',{'jsonId':jid})
  self.call('json_add',{'jsonId':jid,'pointer':'/items','value':'item'});self.call('json_value_set',{'jsonId':jid,'pointer':'/a~1b/~0','value':5});self.call('json_remove',{'jsonId':jid,'pointer':'/items/0'});self.assertEqual(self.call('json_get',{'jsonId':jid})['value']['items'],[2,'item'])
  before=self.call('json_get',{'jsonId':jid})['value']
  with self.assertRaises(CyToolError):self.call('json_value_set',{'jsonId':jid,'pointer':'/missing/path','value':1})
  self.assertEqual(self.call('json_get',{'jsonId':jid})['value'],before);path.write_text('{"external":true}','utf-8')
  with self.assertRaises(CyToolError):self.call('json_save',{'jsonId':jid})
  self.assertEqual(path.read_text(),'{"external":true}')

 def test_05_new_clips_tracks_key_insertion_and_portable_export(self):
  mid,path=self.model('Timeline');clip=self.call('model_clip_create',{'modelId':mid,'name':'New action'})['animation'];track=self.call('model_track_add',{'modelId':mid,'animation':clip,'node':2,'path':'translation'})['track']
  self.call('model_key_add',{'modelId':mid,'animation':clip,'track':track,'time':1,'value':[1,1,0]})
  sample=self.call('model_sample',{'modelId':mid,'animation':clip,'time':.5});self.assertAlmostEqual(sample['nodes'][2][0],.5,places=5)
  before=self.call('model_get',{'modelId':mid})['model']
  with self.assertRaises(CyToolError):self.call('model_key_add',{'modelId':mid,'animation':clip,'track':track,'time':1,'value':[0,1,0]})
  self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before)
  target=self.workspace/'timeline.glb';scene3d.export(before,target);decoded=scene3d.decode(target);self.assertEqual(decoded['animations'][-1]['name'],'New action');self.assertEqual(decoded['animations'][-1]['tracks'][0]['values'][-1],[1,1,0])
  self.call('model_key_remove',{'modelId':mid,'animation':clip,'track':track,'key':1});self.call('edit_undo');self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before)
  shutil.copyfile(target,ROOT/'data/reports/completion-timeline.glb')
 def textured_graph(self,name):
  import struct,zlib
  directory=self.workspace/'data/assets/Images';directory.mkdir(parents=True,exist_ok=True)
  def chunk(kind,value):return struct.pack('>I',len(value))+kind+value+struct.pack('>I',zlib.crc32(kind+value))
  png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',2,2,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b'\0'+bytes([255,0,0,255,0,255,0,255])+b'\0'+bytes([0,0,255,255,255,255,255,255])))+chunk(b'IEND',b'')
  (directory/'Sampler.png').write_bytes(png)
  graph=self.call('asset_new_graph',{'name':name,'domain':'material'})['graphId']
  self.call('graph_link_remove',{'graphId':graph,'linkId':'color_link'});self.call('graph_node_remove',{'graphId':graph,'nodeId':'color'})
  node=self.call('graph_node_add',{'graphId':graph,'type':'material.texture','position':[50,80]})['node']['id'];uv=self.call('graph_node_add',{'graphId':graph,'type':'material.uv','position':[0,0]})['node']['id']
  self.call('graph_node_value',{'graphId':graph,'nodeId':node,'key':'path','value':'Images/Sampler.png'})
  for pin in ('u','v'):self.call('graph_link_add',{'graphId':graph,'fromNode':uv,'fromPin':pin,'toNode':node,'toPin':pin})
  self.call('graph_link_add',{'graphId':graph,'fromNode':node,'fromPin':'color','toNode':'surface','toPin':'baseColor'})
  return graph,node
 def test_06_texture_sampling_srgb_repeat_and_references(self):
  graph,node=self.textured_graph('TextureCPU')
  for u,v,expected in ((.25,.25,[1,0,0]),(.75,.25,[0,1,0]),(-.25,.25,[0,1,0]),(.5,.5,[.5,.5,.5])):
   color=self.call('material_sample',{'graphId':graph,'u':u,'v':v})['baseColor']
   for a,b in zip(color,expected):self.assertAlmostEqual(a,b,places=5)
  plan=self.call('material_prepare',{'graphId':graph});self.assertEqual(plan['textures'][0]['assetPath'],'Images/Sampler.png')
  references=self.call('asset_dependencies',{'path':'Images/Sampler.png'})
  self.assertTrue(references['referencers'])
  with self.assertRaises(CyToolError):self.call('asset_move',{'path':'Images/Sampler.png','destination':'Moved.png'})
  self.call('graph_function_create',{'graphId':graph,'name':'TextureAlpha'})
  body=self.call('graph_get',{'graphId':graph,'functionName':'TextureAlpha'})['graph']
  self.call('graph_link_remove',{'graphId':graph,'functionName':'TextureAlpha','linkId':body['links'][0]['id']})
  sampler=self.call('graph_node_add',{'graphId':graph,'functionName':'TextureAlpha','type':'material.texture','position':[10,10]})['node']['id']
  self.call('graph_node_value',{'graphId':graph,'functionName':'TextureAlpha','nodeId':sampler,'key':'path','value':'Images/Sampler.png'})
  self.call('graph_link_add',{'graphId':graph,'functionName':'TextureAlpha','fromNode':sampler,'fromPin':'alpha','toNode':'return','toPin':'result'})
  self.assertEqual(self.call('graph_function_evaluate',{'graphId':graph,'name':'TextureAlpha','arguments':{'x':0}})['values']['result'],1)
  self.call('graph_node_value',{'graphId':graph,'nodeId':node,'key':'path','value':'../ipc/editor.json'})
  with self.assertRaises(CyToolError):self.call('material_prepare',{'graphId':graph})
 def test_07_texture_shader_worker_hashes_and_gpu_install(self):
  graph,node=self.textured_graph('TextureGPU')
  with tool.create_runtime(self.workspace/'texture-jobs') as runtime:
   actor=runtime.authenticate(runtime.create_client('Texture owner')['token']);session=runtime.create_session(actor)
   job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'material_compile',{'graphId':graph})['jobId']);self.assertEqual(job['state'],'Completed',job)
   report=job['outputs'];self.assertEqual(len(report['textures']),1);source=Path(report['sourcePath']).read_text('utf-8');self.assertIn('Texture2D cy_tex_0',source);self.assertIn('.Sample(cy_sampler',source)
   if self.gui:
    self.call('material_preview_apply',{'graphId':graph,'reportPath':report['reportPath']});self.assertTrue(self.call('material_compile_status',{'graphId':graph})['current'])
   runtime.finish_task(actor,session['sessionId'])
   self.assertFalse(Path(report['shaderPath']).exists())
   if self.gui:self.assertTrue(self.call('material_compile_status',{'graphId':graph})['current'])
 def test_08_model_saved_reload_guard_and_uuid_reimport(self):
  source=self.workspace/'Original.glb';shutil.copyfile(APP/'examples/portable/StaticTriangle.glb',source)
  with tool.create_runtime(self.workspace/'reimport-model-jobs') as runtime:
   actor=runtime.authenticate(runtime.create_client('Reimport model')['token']);session=runtime.create_session(actor,temporary=False)
   job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'asset_import',{'sourcePath':str(source),'destination':'Models/Reimport.cy3d'})['jobId']);self.assertEqual(job['state'],'Completed',job)
   model=self.call('model_open',{'path':'Models/Reimport.cy3d'})['modelId'];job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'asset_reimport',{'path':'Models/Reimport.cy3d'})['jobId']);self.assertEqual(job['state'],'Completed',job)
   self.assertEqual(json.loads((self.workspace/'data/assets/Models/Reimport.cy3d').read_text())['assetId'],model)
   self.call('model_reload',{'modelId':model});self.call('model_node_set',{'modelId':model,'node':0,'name':'Unsaved'})
   with self.assertRaises(CyToolError):self.call('model_reload',{'modelId':model})
   with self.assertRaises(CyToolError):self.call('asset_import_prepare',{'reimport':True,'path':'Models/Reimport.cy3d'})
   self.call('edit_undo');self.call('model_save',{'modelId':model})

 def test_09_loaded_document_listing_canonical_paths_and_native_panels(self):
  status=self.call('editor_status');loaded=self.call('model_list')['models'];self.assertTrue(loaded);self.assertEqual(self.call('editor_status')['revision'],status['revision'])
  mid,path=self.model('Routing');self.assertEqual(self.call('model_open',{'path':'Models/../Models/Routing.cy3d'})['modelId'],mid)
  with self.assertRaises(CyToolError):self.call('asset_move',{'path':'Models/../Models/Routing.cy3d','destination':'Models/Moved.cy3d'})
  copied=self.call('asset_copy',{'path':'Models/Routing.cy3d','destination':'Models/Independent.cy3d'})
  duplicate=self.call('model_open',{'path':'Models/Independent.cy3d'})['modelId'];self.assertNotEqual(duplicate,mid)
  file=self.workspace/'data/assets/Routing.json';file.write_text('{"large":18446744073709551615,"precise":0.12345678901234566}',encoding='utf-8')
  jid=self.call('json_open',{'path':'Models/../Routing.json'})['jsonId'];self.assertEqual(jid,'Routing.json')
  self.call('json_save',{'jsonId':jid});self.assertEqual(json.loads(file.read_text())['large'],18446744073709551615)
  with self.assertRaises(CyToolError):self.call('asset_move',{'path':'Models/../Routing.json','destination':'Moved.json'})
  self.call('json_close',{'jsonId':jid});file.write_text('{"external":2}',encoding='utf-8');jid=self.call('json_open',{'path':'Routing.json'})['jsonId'];self.call('json_save',{'jsonId':jid})
  big=self.workspace/'data/assets/Large.json';value={'k'+str(i).zfill(5):0 for i in range(40000)};big.write_text(json.dumps(value,separators=(',',':')),encoding='utf-8')
  large_id=self.call('json_open',{'path':'Large.json'})['jsonId'];self.call('json_save',{'jsonId':large_id});self.assertLessEqual(big.stat().st_size,512*1024);self.assertEqual(json.loads(big.read_text()),value)
  project=Path(self.call('project_save')['path']);saved=project.read_bytes();original=self.call('model_get',{'modelId':mid})['model']
  self.call('model_node_set',{'modelId':mid,'node':0,'name':'Temporary project edit'});self.call('project_load');self.assertEqual(self.call('model_get',{'modelId':mid})['model'],original);self.call('model_reload',{'modelId':mid})
  damaged=json.loads(saved);nested=0
  for _ in range(66):nested={'nested':nested}
  damaged['document']['jsonDocuments'][jid]['value']=nested;project.write_text(json.dumps(damaged),encoding='utf-8')
  with self.assertRaises(CyToolError):self.call('project_load')
  self.assertEqual(self.call('model_get',{'modelId':mid})['model'],original);project.write_bytes(saved)
  if self.gui:
   self.assertEqual(self.call('panel_open',{'panelId':'model:'+mid})['panelId'],'model:'+mid)
   self.assertEqual(self.call('panel_open',{'panelId':'json:'+jid})['panelId'],'json:'+jid)

if __name__=='__main__':unittest.main(verbosity=2)

