"""Independent pose/length/roundtrip checks and real native/Core animation jobs."""
import copy,json,math,os,shutil,sys,tempfile,unittest
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import animation_tools as motion,model_authoring as authoring,scene3d,tool
import test_integration as integration
from cytools_core import CyToolError

def fixture():return json.loads((APP/'examples/ChainMotion.cy3d').read_text('utf-8-sig'))

def compare_poses(test,a,ca,b,cb,times,epsilon=1e-5):
 for time in times:
  _,pa=authoring.pose(a,a['animations'][ca],time);_,pb=authoring.pose(b,b['animations'][cb],time)
  for i in pa:
   test.assertLess(authoring.length(authoring.sub(pa[i][0],pb[i][0])),epsilon)
   test.assertAlmostEqual(abs(authoring.dot(pa[i][1],pb[i][1])),1,places=5)

class Algorithms(unittest.TestCase):
 def test_01_ik_preserves_lengths_root_source_and_reaches_target_after_glb(self):
  source=fixture();original=copy.deepcopy(source);out,report=authoring.process(source,None,'ik',{'root':1,'tip':3,'target':[1,1,0],'pole':[0,0,1],'animation':0,'fps':10})
  self.assertEqual(source,original);self.assertEqual(out['nodes'],source['nodes']);self.assertEqual(out['animations'][0],source['animations'][0]);self.assertEqual(report['unreachableSamples'],0);self.assertEqual(report['unconvergedSamples'],0)
  for time in [i/10 for i in range(21)]:
   _,current=authoring.pose(out,out['animations'][1],time);_,reference=authoring.pose(source,source['animations'][0],time)
   self.assertLess(authoring.length(authoring.sub(current[3][0],[1,1,0])),.0011);self.assertEqual(current[1][0],reference[1][0])
   for a,b in [(1,2),(2,3)]:self.assertAlmostEqual(authoring.length(authoring.sub(current[b][0],current[a][0])),1,places=6)
  target=ROOT/'data/reports/motion-ik.glb';target.unlink(missing_ok=True);scene3d.export(out,target);decoded=scene3d.decode(target);compare_poses(self,out,1,decoded,1,[0,.5,1,1.5,2])
 def test_02_ik_unreachable_straight_degenerate_antiparallel_and_scale_bounds(self):
  points=[[0,0,0],[0,1,0],[0,2,0]]
  for target in [[0,1,0],[0,0,0],[-1,0,0],[0,4,0]]:
   result,reachable,_=motion.fabrik(points,target,128,.00001,[0,0,1])
   for a,b in zip(result,result[1:]):self.assertAlmostEqual(authoring.length(authoring.sub(a,b)),1,places=6)
   self.assertLess(authoring.length(authoring.sub(result[-1],target)),.00002 if reachable else 2.00001)
  out,r=authoring.process(fixture(),None,'ik',{'root':1,'tip':3,'target':[0,9,0]});self.assertEqual(r['unreachableSamples'],1);self.assertGreater(r['maximumError'],6)
  folded,reached,_=motion.fabrik([[0,0,0],[0,3,0],[0,4,0]],[0,.5,0],128,.00001);self.assertFalse(reached);self.assertAlmostEqual(authoring.length(authoring.sub(folded[1],folded[0])),3,places=6);self.assertAlmostEqual(authoring.length(authoring.sub(folded[2],folded[1])),1,places=6)
  q=motion.from_to([0,1,0],[0,-1,0]);self.assertLess(authoring.length(authoring.sub(authoring.rotate(q,[0,1,0]),[0,-1,0])),1e-6)
  for options in [{'root':2,'tip':1,'target':[1,1,0]},{'root':1,'tip':3,'target':[math.nan,0,0]},{'root':1,'tip':3,'target':[1,1,0],'iterations':True}]:
   with self.assertRaises(ValueError):authoring.process(fixture(),None,'ik',options)
  scaled=fixture();scaled['nodes'][1]['scale']=[2,2,2]
  with self.assertRaises(ValueError):authoring.process(scaled,None,'ik',{'root':1,'tip':3,'target':[1,1,0]})
  zero=fixture();zero['nodes'][3]['translation']=[0,0,0]
  with self.assertRaises(ValueError):authoring.process(zero,None,'ik',{'root':1,'tip':3,'target':[1,1,0]})
 def test_03_root_motion_partial_axes_rotation_and_sampled_reconstruction(self):
  original=fixture();original['animations'][0]['tracks'][0]['values']=[[.5,.1,.3],[1,.2,0],[2,0,.5]]
  for axes,rotation in [([True,False,True],False),([True,True,True],True),([False,False,False],True),([False,True,False],True)]:
   out,r=authoring.process(original,None,'root_motion',{'root':1,'animation':0,'axes':axes,'rotation':rotation,'fps':10})
   self.assertEqual(out['animations'][0],original['animations'][0]);self.assertEqual(out['nodes'],original['nodes'])
   for t in [0,.5,1,1.5,2]:
    nodes,_=authoring.pose(out,out['animations'][r['animation']],t)
    for i,selected in enumerate(axes):
     if selected:self.assertAlmostEqual(nodes[1]['translation'][i],original['animations'][0]['tracks'][0]['values'][0][i])
   restored,summary=authoring.process(out,None,'root_motion',{'mode':'restore','recordId':r['recordId']});compare_poses(self,original,0,restored,summary['animation'],[i/10 for i in range(21)])
 def test_04_root_motion_cypack_glb_and_fingerprint_roundtrip(self):
  source=fixture();out,r=authoring.process(source,None,'root_motion',{'root':1,'rotation':True,'fps':15})
  with tempfile.TemporaryDirectory(dir=ROOT/'data/reports') as temporary:
   for extension in ['glb','cypack']:
    path=Path(temporary)/('root.'+extension);scene3d.export(out,path);decoded=scene3d.decode(path);self.assertEqual(decoded['extensions'],out['extensions'])
    restored,summary=authoring.process(decoded,None,'root_motion',{'mode':'restore','recordId':r['recordId']});compare_poses(self,source,0,restored,summary['animation'],[0,.4,1,1.8,2])
    if extension=='glb':
     target=ROOT/'data/reports/motion-root-restored.glb';target.unlink(missing_ok=True);scene3d.export(restored,target)
 def test_05_changed_motion_refuses_reconstruction_and_invalid_policies_reject(self):
  source=fixture();out,r=authoring.process(source,None,'root_motion',{'root':1});out['animations'][r['motionAnimation']]['tracks'][0]['values'][0][0]+=1
  with self.assertRaisesRegex(ValueError,'changed'):authoring.process(out,None,'root_motion',{'mode':'restore','recordId':r['recordId']})
  for options in [{'root':2},{'root':1,'axes':[False,False,False]},{'root':1,'axes':[1,False,True]},{'root':1,'rotation':1}]:
   with self.assertRaises(ValueError):authoring.process(source,None,'root_motion',options)
  self.assertEqual(len(source['animations']),1)
 def test_06_trim_resample_range_values_zero_time_and_cubic(self):
  source=fixture();t=source['animations'][0]['tracks'][0];t['interpolation']='CUBICSPLINE';t['values']=[v for p in t['values'] for v in ([0,0,0],p,[0,0,0])]
  out,r=authoring.process(source,None,'clip_edit',{'animation':0,'start':.4,'end':1.6,'fps':10});clip=out['animations'][r['animation']];self.assertAlmostEqual(clip['tracks'][0]['times'][0],0);self.assertAlmostEqual(clip['tracks'][0]['times'][-1],1.2)
  for time in clip['tracks'][0]['times']:
   a,_=authoring.pose(source,source['animations'][0],time+.4);b,_=authoring.pose(out,clip,time);self.assertLess(authoring.length(authoring.sub(a[1]['translation'],b[1]['translation'])),1e-6)
  target=ROOT/'data/reports/motion-trimmed.glb';target.unlink(missing_ok=True);scene3d.export(out,target);decoded=scene3d.decode(target);compare_poses(self,out,r['animation'],decoded,r['animation'],[0,.6,1.2])
  for options in [{'start':2,'end':1},{'start':-1},{'end':3},{'fps':0}]:
   with self.assertRaises(ValueError):authoring.process(source,None,'clip_edit',options)

 def test_07_portable_namespace_conflict_and_missing_payload_reference_reject(self):
  from portable_assets.package import package_glb
  from portable_assets.glb import AssetError
  with tempfile.TemporaryDirectory(dir=ROOT/'data/reports') as temporary:
   root=Path(temporary);source=fixture();glb=root/'source.glb';scene3d.export(source,glb)
   conflict=root/'conflict.cypack';package_glb(glb,conflict,name='Conflict',extensions={'org.cygame.example':{'unexpected':True}})
   with self.assertRaisesRegex(AssetError,'namespace conflict'):scene3d.decode(conflict)
   absent=root/'absent.cypack';package_glb(glb,absent,name='Absent',extensions={'org.cygame.editor':{'storage':'glTF.extras','schemaVersion':1,'sha256':'0'*64}})
   with self.assertRaisesRegex(AssetError,'reference is missing'):scene3d.decode(absent)

class NativeMotion(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  integration.EditorIntegration.setUpClass.__func__(cls);(cls.workspace/'data/assets/Examples').mkdir(parents=True,exist_ok=True);shutil.copyfile(APP/'examples/ChainMotion.cy3d',cls.workspace/'data/assets/Examples/ChainMotion.cy3d')
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call
 def fresh(self):
  # Give every test a distinct portable identity without overwriting prior documents.
  value=fixture();import uuid;value['assetId']=str(uuid.uuid4());path='Motion/'+value['assetId']+'.cy3d';p=self.workspace/'data/assets'/path;p.parent.mkdir(exist_ok=True);p.write_text(json.dumps(value),'utf-8');return self.call('model_open',{'path':path})['modelId']
 def job(self,runtime,actor,session,mid,action,options):return runtime.wait(actor,runtime.submit(actor,session['sessionId'],'model_process',{'modelId':mid,'action':action,'options':options})['jobId'])
 def runtime(self):return tool.create_runtime(self.workspace/'motion-jobs')
 def test_01_real_core_ik_native_skinning_single_undo_and_private_cleanup(self):
  mid=self.fresh();before=self.call('model_get',{'modelId':mid})['model'];history=len(self.call('edit_history',{})['undo'])
  with self.runtime() as runtime:
   actor=runtime.authenticate(runtime.create_client('Motion test')['token']);session=runtime.create_session(actor);job=self.job(runtime,actor,session,mid,'ik',{'animation':0,'root':1,'tip':3,'target':[1,1,0],'pole':[0,0,1],'fps':15});self.assertEqual(job['state'],'Completed',job)
   after=self.call('model_get',{'modelId':mid})['model'];self.assertEqual(len(self.call('edit_history',{})['undo']),history+1)
   for time in [0,.4,1,1.8,2]:
    frame=self.call('model_sample',{'modelId':mid,'animation':1,'time':time});self.assertLess(authoring.length(authoring.sub(frame['nodes'][3],[1,1,0])),.0012);self.assertEqual(frame['triangleCount'],4)
   self.call('edit_undo',{});self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before);self.call('edit_redo',{});self.assertEqual(self.call('model_get',{'modelId':mid})['model'],after)
   path=Path(job['outputs']['reportPath']);self.assertTrue(path.exists());runtime.finish_task(actor,session['sessionId']);self.assertFalse(path.exists())
 def test_02_motion_pair_native_roundtrip_changed_keys_guard_and_save(self):
  mid=self.fresh()
  with self.runtime() as runtime:
   actor=runtime.authenticate(runtime.create_client('Root motion')['token']);session=runtime.create_session(actor);job=self.job(runtime,actor,session,mid,'root_motion',{'root':1,'rotation':True,'fps':15});self.assertEqual(job['state'],'Completed',job);report=job['outputs']['summary'];record=report['recordId']
   restored=self.job(runtime,actor,session,mid,'root_motion',{'mode':'restore','recordId':record});self.assertEqual(restored['state'],'Completed',restored)
   for t in [0,.4,1,1.8,2]:
    a=self.call('model_sample',{'modelId':mid,'animation':0,'time':t});b=self.call('model_sample',{'modelId':mid,'animation':3,'time':t})
    for x,y in zip(a['positions'],b['positions']):self.assertLess(authoring.length(authoring.sub(x,y)),1e-5)
   self.call('model_key_set',{'modelId':mid,'animation':2,'track':0,'key':0,'value':[4,0,0]});before=self.call('model_get',{'modelId':mid})['model'];failed=self.job(runtime,actor,session,mid,'root_motion',{'mode':'restore','recordId':record});self.assertEqual(failed['state'],'Failed');self.assertEqual(self.call('model_get',{'modelId':mid})['model'],before)
   self.call('edit_undo',{});self.call('model_save',{'modelId':mid});self.call('model_close',{'modelId':mid});self.call('model_open',{'path':before['assetPath']});self.assertEqual(len(self.call('model_get',{'modelId':mid})['model']['animations']),4)
   runtime.finish_task(actor,session['sessionId'])
 def test_03_trim_native_and_worker_failure_leave_document_unchanged(self):
  mid=self.fresh()
  with self.runtime() as runtime:
   actor=runtime.authenticate(runtime.create_client('Trim test')['token']);session=runtime.create_session(actor);job=self.job(runtime,actor,session,mid,'clip_edit',{'animation':0,'start':.5,'end':1.5,'fps':20});self.assertEqual(job['state'],'Completed',job)
   model=self.call('model_get',{'modelId':mid})['model'];self.assertEqual(model['animations'][1]['tracks'][0]['times'][0],0);self.assertEqual(model['animations'][1]['tracks'][0]['times'][-1],1)
   failed=self.job(runtime,actor,session,mid,'ik',{'root':3,'tip':1,'target':[0,0,0]});self.assertEqual(failed['state'],'Failed');self.assertEqual(self.call('model_get',{'modelId':mid})['model'],model)
   if os.environ.get('CYGAME_TEST_GUI')=='1':self.call('panel_open',{'panelId':'model:'+mid});self.call('model_view_settings',{'modelId':mid,'tab':'Motion tools','motionTool':'Clip','animation':1})
   runtime.finish_task(actor,session['sessionId'])

 def test_04_actual_motion_buttons_run_three_core_jobs_and_undo(self):
  if not self.gui:self.skipTest('Graphical host required')
  import subprocess
  root=self.workspace/'motion-gestures';(root/'app').mkdir(parents=True);shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copyfile(APP/'interface_features.json',root/'app/interface_features.json');shutil.copytree(APP/'locales',root/'app/locales');(root/'data/assets/Examples').mkdir(parents=True);shutil.copyfile(APP/'examples/ChainMotion.cy3d',root/'data/assets/Examples/ChainMotion.cy3d')
  report=ROOT/'data/reports/motion-gestures.json';report.unlink(missing_ok=True)
  run=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--asset','Examples/ChainMotion.cy3d','--model-tab','Motion tools','--test-motion',str(report),'--exit-after','45'],timeout=60)
  fatal=root/'data/logs/native-fatal.json'
  if fatal.exists():shutil.copyfile(fatal,ROOT/'data/reports/motion-gesture-fatal.json')
  states={p.parent.name:json.loads(p.read_text('utf-8')) for p in (root/'data/build-runs').glob('*/state.json')}
  self.assertTrue(report.exists(),'Motion gesture host ended before its report: '+json.dumps(states))
  self.assertEqual(run.returncode,0,report.read_text('utf-8'));self.assertTrue(json.loads(report.read_text('utf-8'))['passed'])
  self.assertFalse((root/'data/settings/ui-preferences.json').exists());self.assertFalse((root/'data/settings/editor.ini').exists())

 def test_05_sequential_desktop_jobs_observe_closed_core_without_cached_busy(self):
  if not self.gui:self.skipTest('Graphical host required')
  import time
  mid=self.fresh()
  def desktop(options):
   request=self.call('model_process',{'modelId':mid,'action':'root_motion','options':options});path=self.workspace/'data/build-runs'/request['runId']/'state.json';deadline=time.monotonic()+20
   while True:
    try:state=json.loads(path.read_text('utf-8'))
    except (OSError,ValueError):state={}
    if state.get('state') in ('Completed','Failed','Cancelled'):break
    if time.monotonic()>deadline:self.fail('Desktop client did not close')
    time.sleep(.005)
   self.assertEqual(state['state'],'Completed',state);self.assertTrue(state['clientClosed']);return state['job']['outputs']['summary']
  pair=desktop({'animation':0,'root':1,'rotation':True});restored=desktop({'mode':'restore','recordId':pair['recordId']});self.assertEqual(restored['animation'],3)

if __name__=='__main__':unittest.main(verbosity=2)
