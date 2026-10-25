"""Isolated native preview and real ImGui apply/acknowledgement gestures."""
import os,json,shutil,subprocess,sys,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];APP=ROOT/'app';REPORTS=ROOT/'data/reports'
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'));import tool
from test_interface import stop_owned
requested=set(filter(None,os.environ.get('CYGAME_MIGRATION_CAPTURES','').split(',')))
results=json.loads((REPORTS/'migrations-captures.json').read_text('utf-8')) if requested and (REPORTS/'migrations-captures.json').exists() else []
results=[r for r in results if r['name'] not in requested]
with tempfile.TemporaryDirectory(prefix='migrations-capture-',dir=REPORTS) as temporary:
 root=Path(temporary);assert root.resolve().is_relative_to(REPORTS.resolve());(root/'app').mkdir();(root/'data/assets/Examples').mkdir(parents=True)
 for name in ('CyTool.json','interface_features.json'):shutil.copyfile(APP/name,root/'app'/name)
 shutil.copytree(APP/'locales',root/'app/locales');shutil.copyfile(APP/'examples/Example_BlueprintMigration.cyasset',root/'data/assets/Examples/Example_BlueprintMigration.cyasset')
 def capture(name,lang,theme='unreal-dark',phase=190,example=False):
  if requested and name not in requested:return
  output=REPORTS/('migrations-'+name+'.bmp');output.unlink(missing_ok=True);report=root/'gesture.json';report.unlink(missing_ok=True);marker=root/'ready';marker.unlink(missing_ok=True)
  args=[str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--lang',lang,'--theme',theme,'--capture',str(output),'--capture-after',str(phase),'--exit-after','45']
  if example:args+=['--asset','Examples/Example_BlueprintMigration.cyasset','--capture-ready',str(marker)]
  else:args+=['--test-migration',str(report)]
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
    g=tool.editor_call('asset_open',{'path':'Examples/Example_BlueprintMigration.cyasset'})['graphId'];before=tool.editor_call('graph_get',{'graphId':g})['graph']
    p={'graphId':g,'target':'function','name':'Echo','inputs':{'data':{'type':'float','default':2}},'outputs':{'answer':{'type':'float','default':0}},'renames':{'inputs':{'x':'data'},'outputs':{'result':'answer'}}}
    with tool.create_runtime(root/('sdk-'+name)) as runtime:
     actor=runtime.authenticate(runtime.create_client('Migration capture')['token']);session=runtime.create_session(actor)
     def run(op,params):
      job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],op,params)['jobId']);assert job['state']=='Completed',job;return job['outputs']
     plan=run('graph_signature_plan',p);assert plan['valid'] and not plan['requiresLossAcceptance'];applied=run('graph_signature_apply',{**p,'planHash':plan['planHash']});assert applied['applied'];result=run('graph_run',{'graphId':g});assert result['messages']==['9'];runtime.finish_task(actor,session['sessionId'])
    after=tool.editor_call('graph_get',{'graphId':g})['graph'];assert before['metadata']==after['metadata'];assert {l['id'] for l in before['links']}=={l['id'] for l in after['links']};proof={'execution':result['messages'],'officialCorePlanAndApply':True,'sourceMetadataAndLinkIdsPreserved':True};marker.touch()
   code=process.wait(timeout=60)
   if report.exists():shutil.copyfile(report,REPORTS/('migrations-'+name+'-gestures.json'))
   fatal=root/'data/logs/native-fatal.json'
   if fatal.exists():shutil.copyfile(fatal,REPORTS/('migrations-'+name+'-fatal.json'))
   assert code==0,(name,code);assert output.exists()
   if not example and phase==190:proof=json.loads(report.read_text('utf-8'));assert proof['passed'],proof
   if not example and phase!=190:proof={'previewFrame':phase,'partialGestureCapture':True}
   results.append({'name':name,'path':str(output),'language':lang,'theme':theme,'proof':proof});(REPORTS/'migrations-captures.json').write_text(json.dumps(results,indent=2),'utf-8');print(name,'passed',flush=True)
  finally:stop_owned(process);tool.ROOT=previous
 for lang in ('en','fr'):
  capture('example-'+lang,lang,example=True);capture('gestures-'+lang,lang);capture('rename-preview-'+lang,lang,phase=44);capture('loss-preview-'+lang,lang,phase=122)
 for theme in ('unreal-dark','mid-grey','light-grey','amber-night','violet','navy'):capture('loss-theme-'+theme,'en',theme,phase=122)
 assert not (root/'data/settings/ui-preferences.json').exists();assert not (root/'data/settings/editor.ini').exists()
print(len(results),'captures passed',flush=True)
