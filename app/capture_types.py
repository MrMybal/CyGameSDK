"""Native graph/type editors; real ImGui designer/property gestures in isolated roots."""
import os
import json,shutil,subprocess,sys,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];APP=ROOT/'app';REPORTS=ROOT/'data/reports'
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'));import tool
from test_interface import stop_owned
requested=set(filter(None,os.environ.get('CYGAME_TYPES_CAPTURES','').split(',')))
results=json.loads((REPORTS/'types-captures.json').read_text('utf-8')) if requested and (REPORTS/'types-captures.json').exists() else []
results=[r for r in results if r['name'] not in requested]
with tempfile.TemporaryDirectory(prefix='types-capture-',dir=REPORTS) as temporary:
 root=Path(temporary);assert root.resolve().is_relative_to(REPORTS.resolve()) and root.resolve()!=REPORTS.resolve();(root/'app').mkdir();(root/'data/assets/Examples').mkdir(parents=True)
 for name in ('CyTool.json','interface_features.json'):shutil.copyfile(APP/name,root/'app'/name)
 shutil.copytree(APP/'locales',root/'app/locales');shutil.copyfile(APP/'examples/Example_BlueprintTypes.cyasset',root/'data/assets/Examples/Example_BlueprintTypes.cyasset')
 def capture(name,lang,theme='unreal-dark',phase=144,example=False):
  if requested and name not in requested:return
  output=REPORTS/('types-'+name+'.bmp');output.unlink(missing_ok=True);report=root/'gesture.json';report.unlink(missing_ok=True);marker=root/'ready';marker.unlink(missing_ok=True)
  args=[str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--lang',lang,'--theme',theme,'--capture',str(output),'--capture-after',str(phase),'--exit-after','45']
  if example:args+=['--asset','Examples/Example_BlueprintTypes.cyasset','--capture-ready',str(marker)]
  else:args+=['--test-types',str(report)]
  process=subprocess.Popen(args);previous=tool.ROOT;tool.ROOT=root;proof={}
  try:
   if example:
    deadline=time.monotonic()+15
    while True:
     try:tool.editor_call('editor_status',{},timeout=1);break
     except tool.CyToolError:
      if process.poll() is not None:raise RuntimeError('Capture exited early')
      if time.monotonic()>deadline:raise
      time.sleep(.03)
    g=tool.editor_call('asset_open',{'path':'Examples/Example_BlueprintTypes.cyasset'})['graphId'];result=tool.editor_call('graph_run',{'graphId':g});assert result['messages']==['88'],result
    with tool.create_runtime(root/('sdk-'+name)) as runtime:
     actor=runtime.authenticate(runtime.create_client('Type capture')['token']);session=runtime.create_session(actor);job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'graph_types_get',{'graphId':g})['jobId']);assert job['state']=='Completed',job;assert len(job['outputs']['types'])==2;proof={'execution':result['messages'],'officialCoreTypeDiscovery':True};runtime.finish_task(actor,session['sessionId'])
    marker.touch()
   code=process.wait(timeout=55)
   if report.exists():shutil.copyfile(report,REPORTS/('types-'+name+'-gestures.json'))
   fatal=root/'data/logs/native-fatal.json'
   if fatal.exists():shutil.copyfile(fatal,REPORTS/('types-'+name+'-fatal.json'))
   assert code==0,name;assert output.exists()
   if not example and phase==144:proof=json.loads(report.read_text('utf-8'));assert proof['passed'],proof
   if not example and phase!=144:proof={'designerFrame':phase,'partialGestureCapture':True}
   results.append({'name':name,'path':str(output),'language':lang,'theme':theme,'proof':proof});(REPORTS/'types-captures.json').write_text(json.dumps(results,indent=2),'utf-8');print(name,'passed',flush=True)
  finally:stop_owned(process);tool.ROOT=previous
 for lang in ('en','fr'):
  capture('example-'+lang,lang,example=True)
  capture('record-properties-'+lang,lang)
  capture('enum-designer-'+lang,lang,phase=60)
  capture('struct-designer-'+lang,lang,phase=80)
 for theme in ('unreal-dark','mid-grey','light-grey','amber-night','violet','navy'):capture('properties-theme-'+theme,'en',theme)
 assert not (root/'data/settings/ui-preferences.json').exists();assert not (root/'data/settings/editor.ini').exists()
print(len(results),'captures passed',flush=True)
