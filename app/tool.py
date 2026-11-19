"""CyMCPSDK native transport; compatibility Python entry point and editor IPC adapter."""
import ctypes
import json
import os
import struct
import sys
import time
from pathlib import Path
from automation_client import Runtime, load_manifest
from job_contract import CyToolError
from automation_client import run_cli

ROOT = Path(os.environ.get("CYGAME_EDITOR_ROOT", Path(__file__).resolve().parents[1])).resolve()
MAX_RESPONSE = 4 * 1024**2

def editor_call(operation, parameters, cancellation=None, *, endpoint=None, timeout=12):
    if os.name != "nt":
        raise CyToolError("UnsupportedPlatform", "The native editor currently requires Windows.")
    try:
        descriptor = json.loads((ROOT/"data/ipc/editor.json").read_text("utf-8-sig")) if endpoint is None else {"pipe":endpoint}
    except (OSError, ValueError) as exc:
        raise CyToolError("EditorUnavailable", "Start CyGameEditor.exe before invoking editor operations.", recoverable=True) from exc
    pipe = descriptor["pipe"]
    if not isinstance(pipe,str) or not pipe.startswith("\\\\.\\pipe\\CyGameSDK-"):
        raise CyToolError("InvalidConfiguration", "Invalid local editor endpoint.")
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.WaitNamedPipeW.argtypes = [ctypes.c_wchar_p,ctypes.c_uint32]
    kernel.WaitNamedPipeW.restype = ctypes.c_int
    kernel.PeekNamedPipe.argtypes = [ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_void_p]
    kernel.PeekNamedPipe.restype = ctypes.c_int
    deadline = time.monotonic()+timeout
    def check():
        if cancellation: cancellation.check()
        if time.monotonic()>deadline:
            raise CyToolError("EditorTimeout", "Editor response timed out. Inspect the live state before retrying a mutation.",recoverable=True)
    while True:
        check()
        if kernel.WaitNamedPipeW(pipe,50):
            try: stream=open(pipe,"r+b",buffering=0); break
            except OSError: pass
        if cancellation: cancellation.sleep(.02)
        else: time.sleep(.02)
    import msvcrt
    with stream:
        payload=json.dumps({"protocolVersion":1,"operation":operation,"parameters":parameters},allow_nan=False).encode("utf-8")
        if len(payload)>1024**2:raise CyToolError("InvalidParameters","Editor request exceeds 1 MiB.")
        packet=struct.pack("<I",len(payload))+payload
        # One write: native operations are short, atomic editor-thread actions.
        written=stream.write(packet)
        if written!=len(packet):raise CyToolError("EditorUnavailable","Incomplete editor request.")
        handle=msvcrt.get_osfhandle(stream.fileno())
        def read_exact(size):
            result=bytearray()
            while len(result)<size:
                check();available=ctypes.c_uint32()
                if not kernel.PeekNamedPipe(handle,None,0,None,ctypes.byref(available),None):
                    raise CyToolError("EditorUnavailable","Editor connection closed.")
                if available.value:
                    data=stream.read(min(available.value,size-len(result)))
                    if not data:raise CyToolError("EditorUnavailable","Editor connection closed.")
                    result.extend(data)
                elif cancellation:cancellation.sleep(.01)
                else:time.sleep(.01)
            return bytes(result)
        size=struct.unpack("<I",read_exact(4))[0]
        if size>MAX_RESPONSE:raise CyToolError("InvalidOutput","Editor response exceeds 4 MiB.")
        response=json.loads(read_exact(size))
        if "error" in response:
            raise CyToolError(response["error"]["code"],response["error"]["message"],recoverable=True)
        return response["result"]

def create_runtime(data_dir, **options):
    return Runtime(load_manifest(Path(__file__).with_name("CyTool.json")),data_dir,editor_root=ROOT,**options)

if __name__=="__main__":
    raise SystemExit(run_cli(create_runtime,sys.argv[1:]))
