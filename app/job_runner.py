"""Single trusted business job, supervised by the native CyMCPSDK process adapter."""
import json,os,sys
from pathlib import Path
from job_contract import JobContext,ProcessWorker,CyToolError

class Handlers:
    def __init__(self):self.handlers={}
    def register(self,name,handler):self.handlers[name]=handler
    def register_worker(self,name,worker):pass

def main():
    request=json.load(sys.stdin);print(json.dumps({"workerPid":os.getpid()}),flush=True);context=JobContext(request["workspace"]);config=request.get("workerConfig",{})
    import tool
    r=Handlers()
    from build_service import register_build_operation,BuildProvider
    from shader_service import register_shader_operation
    from import_service import register_import_operations,ImportProvider
    from model_service import register_model_operations
    from model_process_service import register_authoring_operations
    from game_codecs import register_game_codec_operations,GameCodecProvider
    builds={k:BuildProvider(k,v["command"]) for k,v in config["build_providers"].items()} if "build_providers" in config else None
    imports={k:ImportProvider(k,v["command"]) for k,v in config["import_providers"].items()} if "import_providers" in config else None
    codecs={k:GameCodecProvider(v["descriptor"],v["command"]) for k,v in config["game_codecs"].items()} if "game_codecs" in config else None
    shader=ProcessWorker(config["shader_worker"]["command"]) if "shader_worker" in config else None
    register_build_operation(r,tool.editor_call,builds);register_shader_operation(r,tool.editor_call,shader)
    register_import_operations(r,tool.editor_call,imports);register_model_operations(r,tool.editor_call)
    register_authoring_operations(r,tool.editor_call);register_game_codec_operations(r,tool.editor_call,codecs)
    if request["operation"].startswith("portable_"):
        from portable_assets.service import register_operations
        r.root=context.workspace
        register_operations(r,config["portable_input_root"])
    if request["operation"] not in r.handlers:raise CyToolError("UnknownOperation","No trusted worker handler")
    result=r.handlers[request["operation"]](context,request["parameters"])
    print(json.dumps({"result":result},ensure_ascii=False),flush=True)
if __name__=="__main__":
    try:main()
    except Exception as e:
        print(json.dumps({"error":{"code":getattr(e,"code","WorkerFailed"),"message":str(e),"details":getattr(e,"details",None)}}),flush=True);raise SystemExit(1)
