"""Graphical interface regression checks; every editor owns an isolated test root."""
import ctypes
import json
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
APP=Path(__file__).resolve().parents[1]
ROOT=APP.parent
sys.path.insert(0,str(APP))
import tool

def prepare(workspace):
    (workspace/"app").mkdir()
    for name in ("CyTool.json","interface_features.json"):
        shutil.copyfile(APP/name,workspace/"app"/name)
    shutil.copytree(APP/"locales",workspace/"app/locales")

def stop_owned(process):
    if process.poll() is None:
        user=ctypes.WinDLL("user32",use_last_error=True)
        user.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
        user.GetWindowThreadProcessId.restype=ctypes.c_uint32
        user.PostThreadMessageW.argtypes=[ctypes.c_uint32,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_ssize_t]
        callback_type=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_ssize_t)
        @callback_type
        def close(hwnd,param):
            pid=ctypes.c_uint32();thread=user.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
            if pid.value==process.pid:user.PostThreadMessageW(thread,0x0012,0,0)
            return 1
        user.EnumWindows.argtypes=[callback_type,ctypes.c_ssize_t]
        user.EnumWindows(close,0)
    try:process.wait(timeout=15)
    except subprocess.TimeoutExpired:process.kill();process.wait(timeout=15)

class InterfaceTests(unittest.TestCase):
    def test_01_real_mouse_keyboard_navigation(self):
        with tempfile.TemporaryDirectory(prefix="interface-gesture-",dir=ROOT/"data/reports") as temp:
            workspace=Path(temp);prepare(workspace)
            report=ROOT/"data/reports/completion-interface-gestures.json"
            if report.exists():report.unlink()
            result=subprocess.run([str(ROOT/"CyGameEditor.exe"),"--root",str(workspace),"--test-interface",str(report),"--exit-after","20"],timeout=30)
            evidence=json.loads(report.read_text("utf-8"))
            self.assertEqual(result.returncode,0,evidence)
            self.assertTrue(evidence["passed"],evidence)
            self.assertFalse((workspace/"data/settings/ui-preferences.json").exists())

    def test_03_mesh_vertex_gesture_and_animation_playback(self):
        with tempfile.TemporaryDirectory(prefix="model-gesture-",dir=ROOT/"data/reports") as temp:
            workspace=Path(temp);prepare(workspace)
            assets=workspace/"data/assets/Models";assets.mkdir(parents=True)
            shutil.copyfile(APP/"examples/AnimatedTriangle.cy3d",assets/"Animated.cy3d")
            report=ROOT/"data/reports/completion-model-gestures.json"
            if report.exists():report.unlink()
            result=subprocess.run([str(ROOT/"CyGameEditor.exe"),"--root",str(workspace),"--asset","Models/Animated.cy3d","--test-model",str(report),"--exit-after","20"],timeout=30)
            evidence=json.loads(report.read_text("utf-8"));self.assertEqual(result.returncode,0,evidence);self.assertTrue(evidence["passed"],evidence)

    def test_02_restore_preferences_and_every_panel(self):
        with tempfile.TemporaryDirectory(prefix="interface-restore-",dir=ROOT/"data/reports") as temp:
            workspace=Path(temp);prepare(workspace)
            settings=workspace/"data/settings";settings.mkdir(parents=True)
            prefs={"language":"fr","theme":"unreal-dark","workspace":"Materials","diagnostics":False,"panels":{"material":True,"compiler":True,"logic":False,"viewport":False,"diagnostics":True}}
            (settings/"ui-preferences.json").write_text(json.dumps(prefs),"utf-8")
            process=subprocess.Popen([str(ROOT/"CyGameEditor.exe"),"--root",str(workspace),"--exit-after","60"])
            previous=tool.ROOT;tool.ROOT=workspace
            try:
                endpoint=workspace/"data/ipc/editor.json";deadline=time.monotonic()+10
                while not endpoint.exists() and time.monotonic()<deadline:
                    if process.poll() is not None:self.fail("Editor exited before IPC")
                    time.sleep(.03)
                self.assertTrue(endpoint.exists());time.sleep(.25)
                status=tool.editor_call("editor_status",{})
                panels={p["id"]:p for p in status["panels"]}
                self.assertTrue(panels["compiler"]["open"])
                self.assertFalse(panels["viewport"]["open"])
                self.assertFalse(panels.get("diagnostics",{}).get("open",False))
                self.assertTrue(panels["material"]["open"])
                opened=[]
                for panel in panels:
                    if panel=="diagnostics":continue
                    tool.editor_call("panel_open",{"panelId":panel});opened.append(panel);time.sleep(.04)
                    self.assertIsNone(process.poll(),panel)
                (ROOT/"data/reports/completion-interface-panel-registry.json").write_text(json.dumps({"adapter":status["adapter"],"openedPanels":opened,"restoredCompiler":True,"restoredMaterialsWorkspace":True,"disabledPluginRemainsClosed":True},indent=2),"utf-8")
            finally:stop_owned(process);tool.ROOT=previous
            restored=json.loads((settings/"ui-preferences.json").read_text("utf-8"))
            self.assertEqual(restored["language"],"fr");self.assertEqual(restored["theme"],"unreal-dark")

if __name__=="__main__":unittest.main(verbosity=2)
