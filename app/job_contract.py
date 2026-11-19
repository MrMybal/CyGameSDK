"""Host-owned worker contract. Scheduling/transport belong to native CyMCPSDK."""
import json,subprocess,threading,time
from pathlib import Path

class CyToolError(RuntimeError):
    def __init__(self,code,message,*,details=None,recoverable=False):
        super().__init__(message);self.code=code;self.details=details;self.recoverable=recoverable
class Cancellation:
    def __init__(self,path=None):self.file=Path(path) if path else None
    def check(self):
        if self.file and self.file.exists():raise CyToolError("Cancelled","Job cancelled")
    def sleep(self,seconds):
        until=time.monotonic()+seconds
        while time.monotonic()<until:self.check();time.sleep(min(.02,max(0,until-time.monotonic())))
class JobContext:
    def __init__(self,workspace):
        self.workspace=Path(workspace).resolve();self.cancellation=Cancellation(self.workspace/"cancel.request")
    def path(self,name):
        p=(self.workspace/name).resolve()
        if p==self.workspace or not p.is_relative_to(self.workspace):raise CyToolError("InvalidPath","Job path must stay inside its workspace")
        p.parent.mkdir(parents=True,exist_ok=True);return p
    def progress(self,value,**extra):
        if value<100:self.cancellation.check()
        print(json.dumps({"progress":value,**extra}),flush=True)
class ProcessWorker:
    """One trusted Python compute worker under the SDK-owned process tree.
    No scheduler, transport, arbitrary asset commands, or persistent worker runtime.
    """
    def __init__(self,command,**options):self.command=list(command);self.options=options
    def run(self,context,parameters):
        context.cancellation.check()
        request={"protocolVersion":"1.0","workspace":str(context.workspace),"parameters":parameters}
        process=subprocess.Popen(self.command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,"CREATE_NO_WINDOW",0))
        print(json.dumps({"workerPid":process.pid}),flush=True)
        result={}
        def communicate():
            try:result["out"],result["err"]=process.communicate(json.dumps(request).encode("utf8"))
            except BaseException as e:result["error"]=e
        thread=threading.Thread(target=communicate);thread.start();started=time.monotonic()
        idle_timeout=float(self.options.get("idle_timeout",120) or 0)
        try:
            while thread.is_alive():
                context.cancellation.check()
                if idle_timeout>0 and time.monotonic()-started>idle_timeout:raise CyToolError("WorkerTimeout","Compute worker exceeded its bounded one-shot response timeout")
                thread.join(.02)
        except BaseException:
            process.terminate();thread.join(3)
            if thread.is_alive():process.kill();thread.join()
            raise
        if "error" in result:raise result["error"]
        if len(result["out"])>16*1024*1024:raise CyToolError("InvalidOutput","Worker output exceeds 16 MiB")
        if process.returncode:raise CyToolError("WorkerFailed",result["err"][:4096].decode("utf8",errors="replace") or "Worker failed")
        try:return json.loads(result["out"])
        except ValueError as e:raise CyToolError("InvalidOutput","Worker did not return valid JSON") from e
