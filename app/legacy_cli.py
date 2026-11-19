"""Compatibility for existing tool.py commands, delegated to native CyMCPSDK.
The native CyGameCLI exposes direct invoke; this entry retains job-shaped invoke output.
"""
import argparse,json,os,sys,time,urllib.request,urllib.error
from pathlib import Path
from job_contract import CyToolError
def main(factory,argv):
    p=argparse.ArgumentParser(description="CyGameSDK compatibility CLI / native CyMCPSDK")
    p.add_argument("--data-dir",default=str(Path(__file__).resolve().parents[1]/"data/jobs"));p.add_argument("--connect");p.add_argument("--allow-write",action="store_true")
    commands=p.add_subparsers(dest="command",required=True)
    for name in ("describe","list-operations","list-backends","list-models","diagnose","stdio"):commands.add_parser(name)
    d=commands.add_parser("describe-operation");d.add_argument("operation")
    for name in ("invoke","estimate","can-run"):
        c=commands.add_parser(name);c.add_argument("operation");c.add_argument("--params",default="{}");c.add_argument("--params-file");c.add_argument("--backend",default="");c.add_argument("--model",default="");c.add_argument("--timeout",type=float,default=300)
    c=commands.add_parser("create-client");c.add_argument("--name",required=True);c.add_argument("--permission",action="append",default=[])
    a=p.parse_args(argv);runtime=None
    try:
        from automation_client import ROOT
        root=Path(os.environ.get("CYGAME_EDITOR_ROOT",str(ROOT)))
        allow=a.allow_write or os.environ.get("CYGAME_ALLOW_WRITES")=="1"
        try:allow|=json.loads((root/"data/settings/automation.json").read_text(encoding="utf8")).get("allowChanges",False)
        except FileNotFoundError:pass
        runtime=factory(a.data_dir,allow_changes=allow)
        manifest=runtime.descriptor
        def print_json(value):print(json.dumps(value,ensure_ascii=False),flush=True)
        if a.command in ("describe","list-operations","describe-operation","list-backends","list-models","diagnose","estimate","can-run"):
            if a.command=="describe":value=manifest
            elif a.command=="list-operations":value=manifest["operations"]
            elif a.command=="describe-operation":
                value=next((x for x in manifest["operations"] if x["id"]==a.operation),None)
                if value is None:raise CyToolError("UnknownOperation","Unknown operation")
            elif a.command in ("list-backends","list-models"):value=[]
            elif a.command=="diagnose":value={"sdk":"CyMCPSDK","sdkVersion":"0.7.0","legacyRuntimeInstalled":False,"nativeEditorAvailable":(root/"data/ipc/editor.json").exists()}
            else:value={"operation":a.operation,"canRun":allow and (root/"data/ipc/editor.json").exists(),"validation":"The actual invocation validates live state; no automatic retries."}
            print_json(value);return 0
        if a.command=="create-client":print_json(runtime.create_client(a.name,permissions=a.permission));return 0
        token=os.environ.get("CYGAME_CLIENT_TOKEN",os.environ.get("CYTOOLS_TOKEN",""))
        actor=runtime.authenticate(token) if token and not a.connect else runtime.authenticate(runtime.create_client("Compatibility CLI")["token"])
        def remote(name,args):
            key=os.environ.get("CYGAME_MCP_TOKEN",os.environ.get("CYTOOLS_TOKEN",""))
            if not key:raise CyToolError("AuthenticationRequired","Set CYGAME_MCP_TOKEN (or legacy CYTOOLS_TOKEN)")
            request=urllib.request.Request(a.connect,data=json.dumps({"method":"invoke","name":name,"arguments":args}).encode(),headers={"Authorization":"Bearer "+key,"Content-Type":"application/json"})
            try:
                with urllib.request.urlopen(request,timeout=300) as response:value=json.load(response)
            except urllib.error.HTTPError as e:raise CyToolError("TransportError","HTTP "+str(e.code)) from e
            if "error" in value:raise CyToolError(value["error"]["code"],value["error"]["message"])
            return value.get("result",value)
        if a.command=="invoke":
            if a.backend or a.model:raise CyToolError("InvalidParameters","No business compute backend/model IDs are declared")
            params=json.loads(Path(a.params_file).read_text(encoding="utf8-sig") if a.params_file else a.params)
            if a.connect:
                job=remote("op_"+a.operation,{"parameters":params});until=time.monotonic()+a.timeout
                while job["state"] in ("Created","Running"):
                    if time.monotonic()>until:raise CyToolError("Timeout","Inspect the existing job; do not replay the operation")
                    time.sleep(.03);job=remote("cy_get_job",{"jobId":job["jobId"]})
            else:
                session=runtime.create_session(actor,temporary=False)
                submitted=runtime.submit(actor,session["sessionId"],a.operation,params)
                job=runtime.wait(actor,submitted["jobId"],timeout=a.timeout)
            print_json(job);return 0 if job["state"]=="Completed" else 3 if job.get("error",{}).get("code") in ("AccessDenied","PermissionDenied") else 1
        # Legacy protocol 1.0 envelopes on local stdio; the native JSONL mode stays available.
        session=runtime.create_session(actor,temporary=False)
        for line in sys.stdin:
            identity=None
            try:
                request=json.loads(line);identity=request.get("id");method=request["method"];args=request.get("params",{})
                if method=="DescribeTool":value=manifest
                elif method=="ListOperations":value=manifest["operations"]
                elif method=="DescribeOperation":value=next(x for x in manifest["operations"] if x["id"]==args["operationId"])
                elif method=="GetCapabilities":value=manifest.get("capabilities",[])
                elif method in ("ListBackends","ListModels"):value=[]
                elif method=="CreateSession":value=runtime.create_session(actor,**args)
                elif method=="SubmitJob":value=runtime.submit(actor,args.get("sessionId",session["sessionId"]),args["operationId"],args.get("parameters",{}))
                elif method=="GetJob":value=runtime.get_job(actor,args["jobId"])
                elif method=="ListJobs":value=runtime.list_jobs(actor)
                elif method=="CancelJob":value=runtime.cancel(actor,args["jobId"])
                elif method=="ExportJob":value=runtime.export_job(actor,args["jobId"],args["destination"])
                elif method in ("FinishTask","CloseSession"):value=runtime.finish_task(actor,args.get("sessionId"))
                elif method=="GetRuntimeStatus":value={"sdk":"CyMCPSDK","jobs":runtime.list_jobs(actor)}
                else:raise CyToolError("UnsupportedMethod","Use native SDK discovery for supported operations")
                print_json({"protocolVersion":"1.0","id":identity,"result":value})
            except Exception as e:print_json({"protocolVersion":"1.0","id":identity,"error":{"code":getattr(e,"code","InvalidRequest"),"message":str(e)}})
        return 0
    except Exception as e:
        print(json.dumps({"error":{"code":getattr(e,"code","ConfigurationError"),"message":str(e)}}),file=sys.stderr);return 3 if getattr(e,"code","") in ("AccessDenied","PermissionDenied") else 1
    finally:
        if runtime:runtime.close()
