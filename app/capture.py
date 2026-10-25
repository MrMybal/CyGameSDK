"""Renderer QA. Capture mode does not persist editor settings or layouts."""
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def capture(name,*args):
    path=root/f"data/reports/{name}.bmp"
    result=subprocess.run([str(root/"CyGameEditor.exe"),"--capture",str(path),*args],timeout=25)
    if result.returncode or not path.is_file():raise RuntimeError(f"Capture failed: {name} / {result.returncode}")
    print(name,"passed",flush=True)
for workspace in ["Level","Scripting","Materials"]:
    for lang in ["en","fr"]:capture(f"{workspace.lower()}-{lang}","--workspace",workspace,"--lang",lang)
for panel in ["viewport","logic","material","options"]:
    capture(panel,"--panel",panel,"--lang","fr" if panel=="options" else "en")
for theme in ["unreal-dark","mid-grey","light-grey","amber-night","violet","navy"]:
    capture(f"theme-{theme}","--workspace","Level","--theme",theme,"--lang","en")
