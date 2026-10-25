"""Build entry point; use MSVC directly to avoid MSBuild Path/PATH collisions."""
import subprocess
from pathlib import Path
raise SystemExit(subprocess.call(["cmd.exe","/d","/c",str(Path(__file__).with_name("build_native.cmd"))]))
