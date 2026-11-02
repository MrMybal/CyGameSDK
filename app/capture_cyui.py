"""Additional modular UI captures: density/DPI, generated Code and graph Diff."""
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1];APP=ROOT/"app";REPORTS=ROOT/"data/reports"
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/"tests"))
import tool
from test_interface import prepare,stop_owned
EDITOR=Path(os.environ.get("CYGAME_EDITOR_EXE",str(ROOT/"CyGameEditor.exe")))
results=[]
with tempfile.TemporaryDirectory(prefix="cyui-captures-",dir=REPORTS) as temporary:
    root=Path(temporary);prepare(root)
    shutil.copytree(APP/"examples",root/"data/assets/Examples")
    def record(name,path,**proof):
        data=path.read_bytes();w,h=struct.unpack_from("<ii",data,18)
        assert w>=1000 and abs(h)>=600 and len(set(data[54::127]))>10,name
        results.append({"name":name,"path":str(path),"width":w,"height":abs(h),"passed":True,**proof})
        (REPORTS/"cyui-module-captures.json").write_text(json.dumps(results,indent=2),"utf-8")
        print(name,"passed",flush=True)
    def capture(name,*args):
        path=REPORTS/("cyui-"+name+".bmp")
        result=subprocess.run([str(EDITOR),"--root",str(root),"--capture",str(path),*args],timeout=30)
        assert result.returncode==0,(name,result.returncode)
        record(name,path)
    for density in ("compact","comfortable"):
        for dpi in ("1","1.5","2"):
            for language in ("en","fr"):
                capture(f"options-{density}-dpi{dpi}-{language}","--panel","options","--density",density,"--capture-dpi",dpi,"--lang",language)
        for dpi in ("1","1.5"):
            capture(f"nodes-{density}-dpi{dpi}","--workspace","Scripting","--density",density,"--capture-dpi",dpi)
        capture(f"image-{density}-dpi1.5","--asset","Examples/Example_Image.png","--density",density,"--capture-dpi","1.5")
        capture(f"animation-{density}-dpi1.5","--asset","Examples/AnimatedTriangle.cy3d","--model-tab","Animation","--model-time",".5","--density",density,"--capture-dpi","1.5")
    for language in ("en","fr"):
        for mode in ("code","diff"):
            name=f"{mode}-{language}";output=REPORTS/("cyui-"+name+".bmp");marker=root/"ready";marker.unlink(missing_ok=True)
            process=subprocess.Popen([str(EDITOR),"--root",str(root),"--panel","compiler" if mode=="code" else "changes","--lang",language,"--capture",str(output),"--capture-after","80","--capture-ready",str(marker),"--exit-after","45"])
            previous=tool.ROOT;tool.ROOT=root
            try:
                deadline=time.monotonic()+12
                while True:
                    try:status=tool.editor_call("editor_status",{},timeout=1);break
                    except tool.CyToolError:
                        if process.poll() is not None or time.monotonic()>deadline:raise
                        time.sleep(.03)
                assert status["adapter"]["version"]=="0.29.0",status
                if mode=="code":
                    with tool.create_runtime(root/("code-job-"+language)) as runtime:
                        actor=runtime.authenticate(runtime.create_client("CyUI capture")["token"]);session=runtime.create_session(actor)
                        job=runtime.wait(actor,runtime.submit(actor,session["sessionId"],"material_compile",{"graphId":"material"})["jobId"])
                        assert job["state"]=="Completed",job
                        tool.editor_call("material_preview_apply",{"graphId":"material","reportPath":job["outputs"]["reportPath"]})
                        tool.editor_call("panel_open",{"panelId":"compiler"})
                        deadline=time.monotonic()+5
                        while True:
                            try:runtime.finish_task(actor,session["sessionId"]);break
                            except tool.CyToolError as error:
                                if error.code!="Busy" or time.monotonic()>deadline:raise
                                time.sleep(.02)
                else:
                    tool.editor_call("graph_node_value",{"graphId":"logic","nodeId":"print","key":"text","value":"Modular diff preview"})
                    tool.editor_call("graph_diff_view",{"graphId":"logic"})
                marker.write_text("ready","utf-8");process.wait(timeout=25)
                assert process.returncode==0 and output.is_file(),name
                record(name,output,liveVersion=status["adapter"]["version"],compiledSource=mode=="code")
            finally:stop_owned(process);tool.ROOT=previous
    assert not (root/"data/settings/ui-preferences.json").exists()
    assert not (root/"data/settings/editor.ini").exists()
print(len(results),"module captures passed; user data/settings untouched",flush=True)
