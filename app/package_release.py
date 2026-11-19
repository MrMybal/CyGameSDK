"""Build source, Windows folder and offline-help ZIPs from the reviewed working tree."""
import argparse,hashlib,json,subprocess,zipfile
from pathlib import Path,PurePosixPath

ROOT=Path(__file__).resolve().parents[1]
def selected_sources():
 raw=subprocess.check_output(["git","-c","safe.directory="+ROOT.as_posix(),"ls-files","--cached","--others","--exclude-standard","-z"],cwd=ROOT)
 names=sorted(set(x.decode("utf8") for x in raw.split(b"\0") if x))
 result={}
 for name in names:
  path=ROOT/name;relative=PurePosixPath(name)
  if not path.is_file():continue
  if relative.parts[0] in ("data",".git") or (relative.parts[0]=="runtime" and name!="runtime/active-release.json"):continue
  if path.suffix.lower() in (".exe",".dll",".pdb",".obj",".lib",".db",".sqlite",".sqlite3",".log"):raise ValueError("Unexpected build/private file in source selection: "+name)
  if path.name.startswith(".env") and path.name!=".env.example":raise ValueError("Local environment file in source selection")
  result[name]=path
 required=("AGENTS.md","AI_GUIDE.md","LICENSE","README.md","README.fr.md","app/CyTool.json","app/dependencies.lock.json","Documentation/Documentation.html","Documentation/DocData/data.js")
 for name in required:
  if name not in result:raise ValueError("Required distribution file missing: "+name)
 return result

def write_archive(path,files,prefix):
 with zipfile.ZipFile(path,"w",zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
  for name,source in sorted(files.items()):
   info=zipfile.ZipInfo(prefix+name,date_time=(1980,1,1,0,0,0))
   info.compress_type=zipfile.ZIP_DEFLATED;info.create_system=3
   info.external_attr=(0o100755 if name.endswith((".exe",".cmd")) else 0o100644)<<16
   archive.writestr(info,source.read_bytes())
 with zipfile.ZipFile(path) as archive:
  if archive.testzip() is not None:raise ValueError("Archive integrity check failed")
 return {"name":path.name,"bytes":path.stat().st_size,"sha256":hashlib.sha256(path.read_bytes()).hexdigest(),"files":len(files)}

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument("--output",type=Path,default=ROOT/"runtime/release-0.31.0")
 parser.add_argument("--binary-dir",type=Path,default=ROOT)
 args=parser.parse_args()
 version=json.loads((ROOT/"app/CyTool.json").read_text(encoding="utf8"))["version"]
 source=selected_sources();windows=dict(source)
 for name in ("CyGameEditor.exe","CyGameCLI.exe"):
  path=args.binary_dir/name
  if not path.is_file() or path.read_bytes()[:2]!=b"MZ":raise ValueError("Missing rebuilt native executable: "+name)
  windows[name]=path
 documentation={name.removeprefix("Documentation/"):path for name,path in source.items() if name.startswith("Documentation/")}
 documentation["LICENSE"]=ROOT/"LICENSE"
 output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
 records=[]
 for variant,files,prefix in (("source",source,f"CyGameSDK-{version}/"),("windows-x64",windows,f"CyGameSDK-{version}/"),("documentation",documentation,f"CyGameSDK-{version}-Documentation/")):
  records.append(write_archive(output/f"CyGameSDK-{version}-{variant}.zip",files,prefix))
 (output/"SHA256SUMS.txt").write_text("".join(x["sha256"]+"  "+x["name"]+"\n" for x in records),encoding="utf8")
 print(json.dumps({"version":version,"artifacts":records},indent=2))
 return 0
if __name__=="__main__":raise SystemExit(main())
