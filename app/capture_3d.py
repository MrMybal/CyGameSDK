"""Real native 3D/JSON/texture-material renderer QA in isolated roots."""
import os
import json,shutil,subprocess,sys,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];APP=ROOT/'app';REPORTS=ROOT/'data/reports'
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'));import tool
from test_interface import stop_owned
PREFIX=os.environ.get('CYGAME_CAPTURE_PREFIX','completion')
results=[]
with tempfile.TemporaryDirectory(prefix='completion-captures-',dir=REPORTS) as temp:
 root=Path(temp);(root/'app').mkdir();shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copytree(APP/'locales',root/'app/locales');shutil.copyfile(APP/'interface_features.json',root/'app/interface_features.json');shutil.copytree(APP/'examples',root/'data/assets/Examples')
 def capture(name,*args):
  output=REPORTS/((PREFIX+'-')+name+'.bmp')
  if output.exists():output.unlink()
  process=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--capture',str(output),*args],timeout=30)
  if process.returncode or not output.is_file():raise RuntimeError((name,process.returncode))
  results.append({'name':name,'path':str(output),'passed':True});print(name,'passed',flush=True)
 for language in ('en','fr'):
  for tab in ('Skeleton','Mesh','Animation'):
   capture('3d-'+tab.lower()+'-'+language,'--asset','Examples/AnimatedTriangle.cy3d','--model-tab',tab,'--model-time','.5','--lang',language)
  capture('static-'+language,'--asset','Examples/StaticTriangle.cy3d','--model-tab','Mesh','--lang',language)
  capture('json-'+language,'--asset','Examples/Example_Data.json','--lang',language)
  capture('import-'+language,'--panel','imports','--lang',language)
 for theme in ('unreal-dark','mid-grey','light-grey','amber-night','violet','navy'):
  capture('3d-theme-'+theme,'--asset','Examples/AnimatedTriangle.cy3d','--model-tab','Animation','--model-time','.5','--theme',theme,'--lang','en')
 # Compile, apply and retain actual image resources after the Core task is finished.
 for language in ('en','fr'):
  output=REPORTS/((PREFIX+'-textured-material-')+language+'.bmp');marker=root/'capture.ready';marker.unlink(missing_ok=True)
  process=subprocess.Popen([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Materials','--asset','Examples/Example_TexturedMaterial.cyasset','--lang',language,'--capture',str(output),'--capture-after','90','--capture-ready',str(marker),'--exit-after','45'])
  previous=tool.ROOT;tool.ROOT=root
  try:
   deadline=time.monotonic()+10
   while time.monotonic()<deadline:
    try:tool.editor_call('editor_status',{},timeout=1);break
    except tool.CyToolError:
     if process.poll() is not None:raise RuntimeError('Texture capture exited early')
     time.sleep(.03)
   graph='asset:Examples/Example_TexturedMaterial.cyasset'
   with tool.create_runtime(root/('shader-'+language)) as runtime:
    owner=runtime.authenticate(runtime.create_client('Capture')['token']);session=runtime.create_session(owner)
    job=runtime.wait(owner,runtime.submit(owner,session['sessionId'],'material_compile',{'graphId':graph})['jobId'])
    assert job['state']=='Completed',job
    tool.editor_call('material_preview_apply',{'graphId':graph,'reportPath':job['outputs']['reportPath']})
    tool.editor_call('material_preview_settings',{'graphId':graph,'shape':'Plane'})
    deadline=time.monotonic()+5
    while True:
     try:runtime.finish_task(owner,session['sessionId']);break
     except tool.CyToolError as error:
      if error.code!='Busy' or time.monotonic()>deadline:raise
      time.sleep(.02)
    assert not Path(job['outputs']['shaderPath']).exists()
    marker.write_text('ready','utf-8');process.wait(timeout=20)
    assert process.returncode==0 and output.exists()
    results.append({'name':'textured-material-'+language,'path':str(output),'passed':True,'shaderInstalled':True,'privateTaskFilesRemovedBeforeCapture':True})
    print('textured-material-'+language,'passed',flush=True)
  finally:stop_owned(process);tool.ROOT=previous
 assert not (root/'data/settings/ui-preferences.json').exists()
(REPORTS/(PREFIX+'-captures.json')).write_text(json.dumps(results,indent=2),'utf-8')
print(len(results),'captures passed',flush=True)
