"""Compatibility launcher for development; the editor itself is native."""
import subprocess
from pathlib import Path
raise SystemExit(subprocess.call([str(Path(__file__).resolve().parents[1]/"CyGameEditor.exe")]))
