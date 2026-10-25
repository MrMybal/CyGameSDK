"""CyToolsCore remains the sole MCP server, SDK, scheduler and job runtime."""
import ctypes
import json
import os
import struct
import sys
import time
from pathlib import Path
from cytools_core import Runtime, load_manifest, CyToolError
from cytools_core.transports.cli import run_cli

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

def create_runtime(data_dir, *, build_providers=None, shader_worker=None, import_providers=None, game_codecs=None, **options):
    runtime=Runtime(load_manifest(Path(__file__).with_name("CyTool.json")),data_dir,**options)
    runtime.configure_inputs(ROOT/"data/inputs",fields={})
    for operation in runtime.descriptor["operations"]:
        operation_id=operation["id"]
        if operation_id in ("build_execute","material_compile","asset_import","asset_reimport","model_export","model_process","game_codec_list","game_codec_export"):continue
        def handler(context,parameters,operation_id=operation_id):
            context.cancellation.check()
            result=editor_call(operation_id,parameters,context.cancellation)
            # Job-owned diagnostic result; explicit project/asset saves are persistent by design.
            context.path("outputs/result.json").write_text(json.dumps(result,indent=2,ensure_ascii=False),"utf-8")
            return result
        runtime.register(operation_id,handler)
    from build_service import register_build_operation
    register_build_operation(runtime,editor_call,build_providers)
    from shader_service import register_shader_operation
    register_shader_operation(runtime,editor_call,shader_worker)
    from import_service import register_import_operations
    register_import_operations(runtime,editor_call,import_providers)
    from model_service import register_model_operations
    register_model_operations(runtime,editor_call)
    from model_process_service import register_authoring_operations
    register_authoring_operations(runtime,editor_call)
    from game_codecs import register_game_codec_operations
    register_game_codec_operations(runtime,editor_call,game_codecs)
    return runtime

if __name__=="__main__":
    argv=sys.argv[1:]
    if "--data-dir" not in argv:
        # Each stdio MCP connection owns its runtime; editor mutations are serialized
        # by the editor thread. Use Core's HTTP service for a shared scheduler.
        default=ROOT/"data/jobs"
        if "mcp" in argv or "stdio" in argv:default=default/f"connection-{os.getpid()}"
        argv=["--data-dir",str(default),*argv]
    raise SystemExit(run_cli(create_runtime,argv))
