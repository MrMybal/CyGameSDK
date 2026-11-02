"""Regression checks through the actual migrated modules and app transactions."""
import json
import os
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path
from test_interface import prepare

ROOT=Path(__file__).resolve().parents[2]
EDITOR=Path(os.environ.get("CYGAME_EDITOR_EXE",str(ROOT/"CyGameEditor.exe")))

class ModularInterface(unittest.TestCase):
    def test_interactive_module_bridges(self):
        for density,language in (("compact","en"),("comfortable","fr")):
            with self.subTest(density=density,language=language), tempfile.TemporaryDirectory(prefix="cyui-modules-",dir=ROOT/"data/reports") as temp:
                workspace=Path(temp);prepare(workspace)
                assets=workspace/"data/assets";assets.mkdir(parents=True)
                width=height=16;pixels=bytes((64,128,224))*width*height
                bmp=struct.pack("<2sIHHI",b"BM",54+len(pixels),0,0,54)+struct.pack("<IiiHHIIiiII",40,width,height,1,24,0,len(pixels),0,0,0,0)+pixels
                (assets/"Module.bmp").write_bytes(bmp)
                report=ROOT/"data/reports"/f"cyui-modules-{density}-{language}.json"
                result=subprocess.run([str(EDITOR),"--root",str(workspace),"--density",density,"--lang",language,"--test-modules",str(report),"--exit-after","25"],timeout=35)
                evidence=json.loads(report.read_text("utf-8"))
                self.assertEqual(result.returncode,0,evidence)
                self.assertTrue(evidence["passed"],evidence)
                self.assertTrue(evidence["checks"].get("commandProtocolIdSearch"),evidence)
                self.assertTrue(evidence["checks"].get("newScalarTypeEditable"),evidence)
                self.assertEqual((assets/"Module.bmp").read_bytes(),bmp)
                self.assertFalse((workspace/"data/settings/ui-preferences.json").exists())

if __name__=="__main__":unittest.main(verbosity=2)
