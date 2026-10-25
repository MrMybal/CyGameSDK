"""Collect local installed dependency license metadata without importing their code."""
import importlib.metadata as md
import json,shutil,sys
from pathlib import Path
app=Path(__file__).resolve().parent
notices=app/"LICENCES/python";notices.mkdir(parents=True,exist_ok=True)
components=[]
for dist in sorted(md.distributions(),key=lambda d:d.metadata["Name"].lower()):
    name=dist.metadata["Name"]
    license=dist.metadata.get("License-Expression") or dist.metadata.get("License") or "Not declared in package metadata"
    if len(license)>200:license=license.splitlines()[0][:200]+" (see full notices)"
    paths=[]
    for item in dist.files or []:
        lower=str(item).lower()
        if ".dist-info/" not in lower or not ("license" in item.name.lower() or "/licenses/" in lower or item.name.lower()=="notice"):continue
        source=Path(dist.locate_file(item))
        if source.is_file() and source.stat().st_size<1024**2:
            target=notices/name/item.name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
            paths.append(target.relative_to(app).as_posix())
    components.append({"name":name,"version":dist.version,"license":license,"notices":paths})
(app/"licenses-python.json").write_text(json.dumps(components,indent=2,ensure_ascii=False)+"\n","utf-8")
source=Path(sys.base_prefix)/"LICENSE.txt"
if source.is_file():shutil.copyfile(source,app/"LICENCES/Python.txt")
print("Dependency notices:",len(components))
