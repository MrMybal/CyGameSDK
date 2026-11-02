"""Verify the modular native host without opening the user's project."""
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SUITES=("graph_expand","graph_collapse","type_migration","signature_migration","graph_interfaces","named_types","authoring","animation_tools","integration","scene3d","interface","portable_assets","modules")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--captures",action="store_true",help="also render the isolated interface, asset and density/DPI matrices")
    args=parser.parse_args()
    reports=ROOT/"data/reports";reports.mkdir(parents=True,exist_ok=True)
    env={**os.environ,"CYGAME_TEST_GUI":"1","CYGAME_CAPTURE_PREFIX":"cyui"}
    checks=[("native",[str(ROOT/"runtime/build/CyGameSDKTests.exe"),str(ROOT)])]
    checks += [(name,[sys.executable,str(ROOT/"app/tests"/f"test_{name}.py")]) for name in SUITES]
    if args.captures:
        checks += [(name,[sys.executable,str(ROOT/"app"/f"capture_{name}.py")]) for name in ("interface","3d","cyui")]
    for name,command in checks:
        log=reports/("cyui-"+name+("-captures" if "capture_" in command[-1] else "")+".log")
        print(name,"started",flush=True)
        with log.open("w",encoding="utf-8") as stream:
            result=subprocess.run(command,cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=360)
        if result.returncode:
            print(f"{name} failed; see {log}",flush=True);return result.returncode
        print(name,"passed",flush=True)
    return 0

if __name__=="__main__":raise SystemExit(main())
