import json,shutil,subprocess,tempfile,time,sys,ctypes
from contextlib import contextmanager
from pathlib import Path
root=Path(__file__).resolve().parents[1];reports=root/'data/reports';exe=str(root/'CyGameEditor.exe');sys.path.insert(0,str(root/'app'));import tool
@contextmanager
def owned_workspace():
 workspace=Path(tempfile.mkdtemp(prefix='capture-m18-',dir=reports))
 assert workspace.resolve().is_relative_to(reports.resolve())
 try:yield str(workspace)
 finally:
  # A terminal Core job can precede SDK-client shutdown; let its SQLite handles close.
  kernel=ctypes.WinDLL('kernel32',use_last_error=True)
  kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p];kernel.CreateFileW.restype=ctypes.c_void_p
  kernel.CloseHandle.argtypes=[ctypes.c_void_p]
  for _ in range(150):
   busy=False
   for path in workspace.rglob('runtime.lock.sqlite3'):
    handle=kernel.CreateFileW(str(path),0x80000000,0,None,3,0,None)
    if handle==ctypes.c_void_p(-1).value:busy |= ctypes.get_last_error() in (32,33)
    else:kernel.CloseHandle(handle)
   if not busy:break
   time.sleep(.1)
  shutil.rmtree(workspace)


import struct
cases=[(mode+'-'+lang,mode,lang,'navy') for lang in ['en','fr'] for mode in ['ready','changed','local','browser']]
cases += [('theme-'+theme,'ready','en',theme) for theme in ['mid-grey','light-grey','amber-night','violet','navy']]
if len(sys.argv)>1:cases=[c for c in cases if c[0] in sys.argv[1:]]
results=[]
for name,mode,lang,theme in cases:
 with owned_workspace() as temporary:
  workspace=Path(temporary);(workspace/'app').mkdir();shutil.copyfile(root/'app/CyTool.json',workspace/'app/CyTool.json');shutil.copytree(root/'app/locales',workspace/'app/locales')
  sources=workspace/'sources';sources.mkdir();source=sources/'Photo.png';shutil.copyfile(root/'app/examples/Example_Image.png',source)
  content=workspace/'data/assets/Imported';content.mkdir(parents=True);shutil.copyfile(root/'app/tests/fixtures/tiny.jpg',content/'Photo.jpg')
  raw=(b'\xff\x40\x20'*4)*4
  (content/'Colors.bmp').write_bytes(b'BM'+struct.pack('<IHHI',54+len(raw),0,0,54)+struct.pack('<IiiHHIIiiII',40,4,4,1,24,0,len(raw),2835,2835,0,0)+raw)
  image=reports/('m18-'+name+'.bmp');marker=workspace/'capture.ready'
  args=[exe,'--root',str(workspace),'--workspace','Materials','--panel','imports','--theme',theme,'--capture',str(image),'--capture-after','90','--capture-ready',str(marker),'--exit-after','60']
  if lang=='fr':args+=['--lang','fr']
  process=subprocess.Popen(args);original=tool.ROOT;tool.ROOT=workspace;started=time.monotonic()
  try:
   deadline=time.monotonic()+10
   while time.monotonic()<deadline:
    if process.poll() is not None:raise RuntimeError('Capture editor exited early')
    try:tool.editor_call('editor_status',{},timeout=1);break
    except tool.CyToolError:time.sleep(.02)
   tool.editor_call('asset_import_request',{'sourcePath':str(source),'destination':'Imported/Photo.png'})
   deadline=time.monotonic()+30
   while time.monotonic()<deadline:
    state=tool.editor_call('asset_import_status')
    if state['state'] in ('Completed','Failed','Cancelled'):break
    time.sleep(.05)
   assert state['state']=='Completed',state
   if mode=='changed':source.write_bytes(source.read_bytes()+b'changed')
   elif mode=='local':(content/'Photo.png').write_bytes((content/'Photo.png').read_bytes()+b'local')
   info=tool.editor_call('asset_import_check',{'path':'Imported/Photo.png'})
   assert info['state']=={'changed':'SourceChanged','local':'LocalChanges'}.get(mode,'UpToDate'),info
   tool.editor_call('asset_import_view',{'path':'Imported/Photo.png'})
   tool.editor_call('asset_browser_settings',{'type':'Texture','grid':True,'folder':'Imported'})
   if mode=='browser':tool.editor_call('texture_open',{'path':'Imported/Photo.png'})
   marker.write_text('ready','utf-8');code=process.wait(timeout=35)
   assert code==0 and image.stat().st_size>0,(name,code)
   results.append({'name':name,'mode':mode,'language':lang,'theme':theme,'sourceState':info['state'],'passed':True,'seconds':round(time.monotonic()-started,3)})
  finally:
   tool.ROOT=original
   if process.poll() is None:
    process.terminate();process.wait(timeout=30)
 print(name,'passed',flush=True)
(reports/'m18-capture-results.json').write_text(json.dumps(results,indent=2),'utf-8')
