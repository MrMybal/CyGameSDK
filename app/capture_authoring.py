"""Actual native renderer QA of 0.21 authoring workflows in isolated roots."""
import os
import json,shutil,subprocess,sys,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];APP=ROOT/'app';REPORTS=ROOT/'data/reports'
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool
from test_interface import stop_owned
requested=set(filter(None,os.environ.get('CYGAME_AUTHORING_CAPTURES','').split(',')))
results=json.loads((REPORTS/'authoring-captures.json').read_text('utf-8')) if requested and (REPORTS/'authoring-captures.json').exists() else []
results=[r for r in results if r['name'] not in requested]
with tempfile.TemporaryDirectory(prefix='authoring-capture-',dir=REPORTS) as temporary:
 root=Path(temporary);(root/'app').mkdir();shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copyfile(APP/'interface_features.json',root/'app/interface_features.json');shutil.copytree(APP/'locales',root/'app/locales');shutil.copytree(APP/'examples',root/'data/assets/Examples')
 def capture(name,language,asset=None,action=None,tab=None,theme=None,workspace='Level',debug=False,codec=False):
  if requested and name not in requested:return
  output=REPORTS/('authoring-'+name+'.bmp');marker=root/'ready';marker.unlink(missing_ok=True);output.unlink(missing_ok=True)
  args=[str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace',workspace,'--lang',language,'--capture',str(output),'--capture-after','90','--capture-ready',str(marker),'--exit-after','60']
  if asset:args+=['--asset','Examples/'+asset]
  if tab:args+=['--model-tab',tab]
  if theme:args+=['--theme',theme]
  process=subprocess.Popen(args);previous=tool.ROOT;tool.ROOT=root
  try:
   deadline=time.monotonic()+15
   while True:
    try:tool.editor_call('editor_status',{},timeout=1);break
    except tool.CyToolError:
     if process.poll() is not None:raise RuntimeError('Capture exited before IPC: '+name)
     if time.monotonic()>deadline:raise
     time.sleep(.03)
   proof={}
   if action:
    target=tool.editor_call('model_open',{'path':'Examples/'+asset})['modelId'];p={'modelId':target,'action':action,'options':{}}
    if action=='lod':p['options']={'ratio':.5}
    elif action=='collision':p['options']={'shape':'capsule' if 'capsule' in name else 'box'}
    else:
     source=tool.editor_call('model_open',{'path':'Examples/AnimatedTriangle.cy3d'})['modelId'];p.update(sourceModelId=source,options={'mapping':{'1':1,'2':2},'fps':30})
    with tool.create_runtime(root/('jobs-'+name)) as runtime:
     actor=runtime.authenticate(runtime.create_client('Renderer QA')['token']);session=runtime.create_session(actor)
     job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'model_process',p)['jobId']);assert job['state']=='Completed',job;proof=job['outputs']['summary'];runtime.finish_task(actor,session['sessionId'])
    settings={'modelId':target,'tab':tab or 'Mesh'}
    if action=='lod':settings.update(lod=1,wireframe=True)
    if action=='retarget':settings.update(animation=0,time=.5,sourceModelId=source,mapping={'1':1,'2':2})
    tool.editor_call('model_view_settings',settings)
   if debug:
    graph=tool.editor_call('asset_open',{'path':'Examples/'+asset})['graphId'];tool.editor_call('graph_debug_start',{'graphId':graph});state=tool.editor_call('graph_debug_continue',{'graphId':graph})
    if state['state']=='Waiting':state=tool.editor_call('graph_debug_tick',{'graphId':graph,'delta':.25})
    tool.editor_call('graph_debug_view',{'graphId':graph});proof={'state':state['state'],'time':state['time'],'objects':len(state['objects'])}
   if codec:
    tool.editor_call('panel_open',{'panelId':'codecs'});request=tool.editor_call('game_codec_list',{});state_file=root/'data/build-runs'/request['runId']/'state.json';deadline=time.monotonic()+20
    while True:
     try:state=json.loads(state_file.read_text('utf-8'))
     except (OSError,ValueError):state={}
     if state.get('state') in ('Completed','Failed','Cancelled'):break
     if time.monotonic()>deadline:raise TimeoutError('Codec provider listing')
     time.sleep(.05)
    assert state['state']=='Completed',state;assert state['job']['outputs']['codecs']==[];proof={'defaultGameCodecCount':0,'officialCoreJob':True}
   marker.touch();assert process.wait(timeout=40)==0,name;assert output.is_file(),name
   results.append({'name':name,'path':str(output),'language':language,'theme':theme or 'unreal-dark','proof':proof});(REPORTS/'authoring-captures.json').write_text(json.dumps(results,indent=2),'utf-8');print(name,'passed',flush=True)
  finally:stop_owned(process);tool.ROOT=previous
 for language in ('en','fr'):
  capture('lod-'+language,language,'Grid_LOD.cy3d','lod','LODs')
  capture('collision-'+language,language,'Grid_LOD.cy3d','collision','Collision')
  capture('capsule-'+language,language,'Grid_LOD.cy3d','collision','Collision')
  capture('retarget-mapping-'+language,language,'TallTarget.cy3d','retarget','Retarget')
  capture('retarget-animation-'+language,language,'TallTarget.cy3d','retarget','Animation')
  capture('blueprint-objects-'+language,language,'Example_BlueprintObjects.cyasset',workspace='Scripting',debug=True)
  capture('blueprint-containers-'+language,language,'Example_BlueprintContainers.cyasset',workspace='Scripting')
  capture('blueprint-latent-'+language,language,'Example_BlueprintLatent.cyasset',workspace='Scripting',debug=True)
  capture('codecs-'+language,language,codec=True)
 for theme in ('unreal-dark','mid-grey','light-grey','amber-night','violet','navy'):
  capture('lod-theme-'+theme,'en','Grid_LOD.cy3d','lod','LODs',theme=theme)
  capture('blueprint-theme-'+theme,'en','Example_BlueprintContainers.cyasset',theme=theme,workspace='Scripting')
 assert not (root/'data/settings/ui-preferences.json').exists();assert not (root/'data/settings/editor.ini').exists()
print(len(results),'authoring captures passed',flush=True)
