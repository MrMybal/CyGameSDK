"""Interface audit renders, isolated fixtures and no user preferences/layout writes."""
import os
import json
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
APP=ROOT/"app"
REPORTS=ROOT/"data/reports"
PREFIX=os.environ.get('CYGAME_CAPTURE_PREFIX','completion')
results=[]
with tempfile.TemporaryDirectory(prefix="interface-captures-",dir=REPORTS) as temp:
    workspace=Path(temp);(workspace/"app").mkdir()
    for name in ("CyTool.json","interface_features.json"):shutil.copyfile(APP/name,workspace/"app"/name)
    shutil.copytree(APP/"locales",workspace/"app/locales")
    shutil.copytree(APP/"examples",workspace/"data/assets/Examples",ignore=shutil.ignore_patterns("adapters","portable","*.md"))
    def capture(name,*args):
        path=REPORTS/((PREFIX+"-interface-")+name+".bmp")
        if path.exists():path.unlink()
        run=subprocess.run([str(ROOT/"CyGameEditor.exe"),"--root",str(workspace),"--capture",str(path),*args],timeout=25)
        if run.returncode or not path.exists():raise RuntimeError((name,run.returncode))
        data=path.read_bytes();width,height=struct.unpack_from("<ii",data,18)
        assert width>=1000 and abs(height)>=600 and len(set(data[54::127]))>10,name
        results.append({"name":name,"path":str(path),"width":width,"height":abs(height),"exitCode":run.returncode})
        (REPORTS/(PREFIX+"-interface-captures.json")).write_text(json.dumps(results,indent=2),"utf-8")
        print(name,"passed",flush=True)
    for lang in ("en","fr"):
        for workspace_name in ("Level","Scripting","Materials"):
            capture(workspace_name.lower()+"-"+lang,"--workspace",workspace_name,"--lang",lang)
        for panel in ("options","interface","build","references","navigator","compiler","diagnostics"):
            capture(panel+"-"+lang,"--workspace","Level","--panel",panel,"--lang",lang)
        capture("texture-"+lang,"--panel","texture","--asset","Examples/Example_Image.png","--lang",lang)
        capture("function-"+lang,"--workspace","Scripting","--asset","Examples/Example_ExecutionFunctions.cyasset","--function","Report","--lang",lang)
    for theme in ("unreal-dark","mid-grey","light-grey","amber-night","violet","navy"):
        for workspace_name in ("Level","Materials"):
            capture(theme+"-"+workspace_name.lower(),"--workspace",workspace_name,"--theme",theme,"--lang","en")
    assert not (workspace/"data/settings/ui-preferences.json").exists()
    assert not (workspace/"data/settings/editor.ini").exists()
print(len(results),"captures passed; user settings untouched",flush=True)

