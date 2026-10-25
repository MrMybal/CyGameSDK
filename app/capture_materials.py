import json,shutil,subprocess,tempfile,time,sys,ctypes
from contextlib import contextmanager
from pathlib import Path
root=Path(__file__).resolve().parents[1];reports=root/'data/reports';exe=str(root/'CyGameEditor.exe');sys.path.insert(0,str(root/'app'));import tool
@contextmanager
def owned_workspace():
 workspace=Path(tempfile.mkdtemp(prefix='capture-m15-',dir=reports))
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

cases=[]
for lang in ['en','fr']:
 for mode in ['uv-sphere','uv-plane','shader-source','uv-instance','shader-error','image']:cases.append((mode+'-'+lang,lang,'navy',mode,'RGBA'))
for channel in ['RGB','R','G','B','A']:cases.append(('image-channel-'+channel,'fr','navy','image',channel))
for theme in ['mid-grey','light-grey','amber-night','violet','navy']:
 cases.append(('uv-theme-'+theme,'en',theme,'uv-sphere','RGBA'))
 cases.append(('image-theme-'+theme,'en',theme,'image','RGBA'))
if len(sys.argv)>1:cases=[case for case in cases if case[0] in sys.argv[1:]]
results=[]
for name,lang,theme,mode,channel in cases:
 with owned_workspace() as temporary:
  workspace=Path(temporary);assert workspace.resolve().is_relative_to(reports.resolve())
  (workspace/'app').mkdir();shutil.copytree(root/'app/locales',workspace/'app/locales');shutil.copyfile(root/'app/CyTool.json',workspace/'app/CyTool.json')
  target=workspace/'data/assets/Examples';shutil.copytree(root/'app/examples',target)
  asset='Example_Image.png' if mode=='image' else 'Example_UVInstance.cyasset' if mode=='uv-instance' else 'Example_UVMaterial.cyasset'
  key='asset:Examples/'+asset;image=reports/('m15-'+name+'.bmp')
  args=[exe,'--root',str(workspace),'--workspace','Materials','--asset','Examples/'+asset,'--lang',lang,'--theme',theme,'--capture',str(image),'--capture-after','60','--capture-ready',str(workspace/'capture.ready'),'--exit-after','45']
  if mode=='image':args+=['--panel','texture']
  elif mode.startswith('shader-'):args+=['--panel','compiler']
  started=time.monotonic();process=subprocess.Popen(args);original=tool.ROOT;tool.ROOT=workspace
  try:
   for attempt in range(250):
    if process.poll() is not None:raise RuntimeError('Capture host exited early')
    try:
     tool.editor_call('editor_status',{},timeout=1);break
    except tool.CyToolError:time.sleep(.02)
   else:raise RuntimeError('Capture editor did not become ready')
   if mode=='image':tool.editor_call('texture_view_settings',{'path':'Examples/'+asset,'channel':channel})
   else:
    if mode=='shader-error':
     tool.editor_call('graph_function_create',{'graphId':key,'name':'Unsupported'})
     body=tool.editor_call('graph_get',{'graphId':key,'functionName':'Unsupported'})['graph']
     tool.editor_call('graph_link_remove',{'graphId':key,'functionName':'Unsupported','linkId':body['links'][0]['id']})
     tool.editor_call('graph_function_signature',{'graphId':key,'name':'Unsupported','inputs':{'label':{'type':'string','default':'Example'}},'outputs':{'result':{'type':'float','default':.5}}})
     n=tool.editor_call('graph_node_add',{'graphId':key,'type':'function.call','functionRef':'Unsupported','position':[400,300]})['node']['id']
     tool.editor_call('graph_link_add',{'graphId':key,'fromNode':n,'fromPin':'result','toNode':'surface','toPin':'metallic'})
    tool.editor_call('material_preview_settings',{'graphId':key,'shape':'Plane' if mode=='uv-plane' else 'Sphere'})
    tool.editor_call('material_compile_request',{'graphId':key})
    deadline=time.monotonic()+30
    while time.monotonic()<deadline and process.poll() is None:
     status=tool.editor_call('material_compile_status',{'graphId':key})
     if status['state'] in ('Completed','Failed','Cancelled'):break
     time.sleep(.025)
    if mode=='shader-error':assert status['state']=='Failed' and status['job']['error']['technicalDetails']['diagnostics'],status
    else:assert status['state']=='Completed' and status['current'],status
    assert process.poll() is None,'Capture completed before compiler result'
   ready_at=time.monotonic();(workspace/'capture.ready').write_text('ready','utf-8')
   code=process.wait(timeout=35)
   assert code==0 and image.stat().st_size>0,(name,code)
   results.append({'name':name,'language':lang,'theme':theme,'mode':mode,'shaderState':status['state'] if mode!='image' else None,'passed':True,'elapsedSeconds':round(time.monotonic()-started,3),'captureAndExitSeconds':round(time.monotonic()-ready_at,3)})
  finally:
   tool.ROOT=original
   if process.poll() is None:
    user=ctypes.WinDLL('user32',use_last_error=True)
    user.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)];user.GetWindowThreadProcessId.restype=ctypes.c_uint32
    user.PostThreadMessageW.argtypes=[ctypes.c_uint32,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_ssize_t]
    callback_type=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_ssize_t)
    @callback_type
    def close_owned(hwnd,param):
     pid=ctypes.c_uint32();thread=user.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
     if pid.value==process.pid:user.PostThreadMessageW(thread,0x0012,0,0)
     return 1
    user.EnumWindows.argtypes=[callback_type,ctypes.c_ssize_t];user.EnumWindows(close_owned,0)
    try:process.wait(timeout=15)
    except subprocess.TimeoutExpired:process.kill();process.wait(timeout=30)
 print(name,'passed',flush=True)
(reports/'m15-capture-results.json').write_text(json.dumps(results,indent=2),'utf-8')
