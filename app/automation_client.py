"""Compatibility Python client for the application's compiled CyMCPSDK host.
There is no Python scheduler or MCP server here. Jobs execute in the native host.
"""
import hashlib,json,os,secrets,shutil,subprocess,sys,threading,time
from pathlib import Path
from job_contract import CyToolError

ROOT=Path(__file__).resolve().parents[1]
def load_manifest(path):return json.loads(Path(path).read_text(encoding="utf8"))
def validate_parameters(parameters,schema):
    from jsonschema import Draft202012Validator
    errors=list(Draft202012Validator(schema).iter_errors(parameters))
    if errors:raise CyToolError("InvalidParameters",errors[0].message)
    return json.loads(json.dumps(parameters))
def cli_path():
    path=Path(os.environ.get("CYGAME_CLI_EXE",str(ROOT/"CyGameCLI.exe")))
    if not path.is_file():raise CyToolError("MissingRuntime","Build CyGameCLI from the pinned CyMCPSDK sources first.")
    return path
class Runtime:
    def __init__(self,manifest,data_dir,*,editor_root=None,**options):
        self.descriptor=manifest;self.root=Path(data_dir).resolve();self.root.mkdir(parents=True,exist_ok=True)
        if editor_root is None:
            import tool
            editor_root=tool.ROOT
        self.editor_root=Path(editor_root).resolve();self._process=None;self._lock=threading.Lock();self._clients={};self._sessions={};self._jobs={};self._config={};self._closed=False;self._allow_changes=options.pop("allow_changes",True)
        from legacy_jobs import records,migrate
        legacy_clients=records(self.root,"clients")
        try:
            self._clients={r["tokenHash"]:r for r in json.loads((self.root/"native-clients.json").read_text(encoding="utf8"))}
        except FileNotFoundError:self._clients={r["tokenHash"]:r for r in legacy_clients}
        try:self._sessions=json.loads((self.root/"native-sessions.json").read_text(encoding="utf8"))
        except FileNotFoundError:self._sessions={r["sessionId"]:r for r in records(self.root,"sessions")}
        migrate(self.root,self.root)
        for key in ("build_providers","import_providers","game_codecs"):
            if options.get(key) is not None:
                self._config[key]={k:{"command":v.worker.command,**({"descriptor":v.descriptor} if hasattr(v,"descriptor") else {})} for k,v in options[key].items()}
        if options.get("shader_worker") is not None:self._config["shader_worker"]={"command":options["shader_worker"].command}
    def _request(self,request):
        if self._closed:raise CyToolError("Closed","Native client is closed")
        with self._lock:
            if self._process is None:
                manifest_file=self.root/"native-manifest.json";manifest_file.write_text(json.dumps(self.descriptor),encoding="utf8")
                args=[str(cli_path()),"--root",str(self.editor_root),"--data-dir",str(self.root),"--bridge","--manifest",str(manifest_file),"jsonl"]
                if self._allow_changes:args.insert(1,"--allow-write")
                self._process=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,"CREATE_NO_WINDOW",0))
                if self._config:
                    self._process.stdin.write((json.dumps({"bridge":"configure","config":self._config})+"\n").encode("utf8"));self._process.stdin.flush()
                    first=self._process.stdout.readline()
                    if not first:raise CyToolError("NativeHostFailed","Native client failed to configure")
            self._process.stdin.write((json.dumps(request,ensure_ascii=False)+"\n").encode("utf8"));self._process.stdin.flush()
            line=self._process.stdout.readline(64*1024*1024+1)
            if not line:
                error=self._process.stderr.read(4096).decode("utf8",errors="replace")
                raise CyToolError("NativeHostFailed",error or "Native SDK host exited")
            response=json.loads(line)
            if "error" in response:raise CyToolError(response["error"]["code"],response["error"]["message"],details=response["error"].get("details"))
            return response["result"]
    def _persist_identity(self):
        # Start the native host first: it applies an owner-only ACL to this store.
        self._request({"bridge":"configure","config":self._config})
        for name,value in (("native-clients.json",list(self._clients.values())),("native-sessions.json",self._sessions)):
            path=self.root/name;temp=path.with_suffix(".tmp");temp.write_text(json.dumps(value,indent=2),encoding="utf8");temp.replace(path)
    @staticmethod
    def _public(record):return {k:v for k,v in record.items() if k!="tokenHash"}
    def create_client(self,name,**options):
        token=secrets.token_hex(32);digest=hashlib.sha256(token.encode()).hexdigest()
        client={"clientId":secrets.token_hex(16),"name":name,"permissions":list(options.get("permissions",())),"tokenHash":digest}
        self._clients[digest]=client;self._persist_identity();return {**self._public(client),"token":token}
    def authenticate(self,token):
        digest=hashlib.sha256(token.encode()).hexdigest()
        for known,record in self._clients.items():
            if secrets.compare_digest(digest,known):return self._public(record)
        raise CyToolError("AuthenticationRequired","Invalid client token")
    def _owner(self,actor):
        if not isinstance(actor,dict) or not any(self._public(x)==actor for x in self._clients.values()):raise CyToolError("Unauthorized","Authenticate this client first")
        return actor["clientId"]
    def create_session(self,actor,**options):
        owner=self._owner(actor);session={"sessionId":secrets.token_hex(16),"clientId":owner,"closed":False,"temporary":bool(options.get("temporary",True))}
        self._sessions[session["sessionId"]]=session;self._persist_identity();return session
    def submit(self,actor,session_id,operation,parameters,**options):
        owner=self._owner(actor)
        if session_id not in self._sessions or self._sessions[session_id]["clientId"]!=owner or self._sessions[session_id].get("closed",False):raise CyToolError("PermissionDenied","Session belongs to another client")
        descriptor=next((x for x in self.descriptor["operations"] if x["id"]==operation),None)
        if descriptor is None:raise CyToolError("UnknownOperation","Unknown application operation")
        validate_parameters(parameters,descriptor["inputSchema"])
        job=self._request({"bridge":"submit","owner":owner,"operation":operation,"parameters":parameters});self._jobs[job["jobId"]]=session_id;return job
    def get_job(self,actor,job_id):return self._request({"bridge":"get","owner":self._owner(actor),"jobId":job_id})
    def list_jobs(self,actor):return self._request({"bridge":"list","owner":self._owner(actor)})
    def cancel(self,actor,job_id):return self._request({"bridge":"cancel","owner":self._owner(actor),"jobId":job_id})
    def wait(self,actor,job_id,timeout=300,**options):
        until=time.monotonic()+timeout
        while True:
            job=self.get_job(actor,job_id)
            if job["state"] in ("Completed","Failed","Cancelled","Interrupted"):return job
            if time.monotonic()>until:raise CyToolError("Timeout","Job continues; inspect its durable state before retrying")
            time.sleep(.02)
    def finish_task(self,actor,session_id=None,**options):
        owner=self._owner(actor)
        if session_id is not None and (session_id not in self._sessions or self._sessions[session_id]["clientId"]!=owner):raise CyToolError("PermissionDenied","Session belongs to another client")
        if any(j["state"] in ("Created","Running") for j in self.list_jobs(actor)):raise CyToolError("Busy","Owned jobs are still active")
        if session_id is not None:
            self._sessions[session_id]["closed"]=True;self._persist_identity()
        return {"finished":True,"resultsRetained":True}
    def export_job(self,actor,job_id,destination):
        job=self.get_job(actor,job_id)
        if job["state"] in ("Created","Running"):raise CyToolError("Busy","Wait for the job to finish")
        dest=Path(destination).resolve()
        if dest.exists():raise CyToolError("AlreadyExists","Export destination already exists")
        dest.mkdir(parents=True)
        def rewrite(v):
            if isinstance(v,dict):return {k:rewrite(x) for k,x in v.items()}
            if isinstance(v,list):return [rewrite(x) for x in v]
            if isinstance(v,str):
                p=Path(v)
                if p.is_absolute() and p.is_file() and p.resolve().is_relative_to(self.root):
                    target=dest/p.resolve().relative_to(self.root);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target);return str(target)
            return v
        exported=rewrite(job);(dest/"job.json").write_text(json.dumps(exported,indent=2),encoding="utf8");return exported
    def register_worker(self,name,worker):pass
    def register(self,name,handler):
        if not name.startswith("portable_"):raise CyToolError("Unsupported","Register operations in the native application catalogue and trusted source runner")
    def configure_portable(self,path):
        self._config["portable_input_root"]=str(Path(path).resolve())
        if self._process:self._request({"bridge":"configure","config":self._config})
    def configure_inputs(self,*args,**kwargs):pass
    def close(self):
        if self._closed:return
        self._closed=True
        if self._process:
            self._process.stdin.close()
            try:self._process.wait(timeout=15)
            except subprocess.TimeoutExpired:self._process.terminate();self._process.wait(timeout=5)
            self._process.stdout.close();self._process.stderr.close()
    def __enter__(self):return self
    def __exit__(self,*args):self.close()

def run_cli(factory,argv):
    args=list(argv);root=os.environ.get("CYGAME_EDITOR_ROOT",str(ROOT))
    legacy=("describe","list-operations","describe-operation","list-backends","list-models","diagnose","estimate","can-run","create-client","invoke","stdio")
    if any(a in legacy for a in args):
        from legacy_cli import main
        return main(factory,args)
    if "serve" in args and "--port" not in args:args.extend(["--port","8766"])
    if "mcp" in args and "--compact" not in args and "--full" not in args:args.append("--full")
    if os.environ.get("CYGAME_ALLOW_WRITES")=="1" and "--allow-write" not in args:args.insert(0,"--allow-write")
    try:return subprocess.call([str(cli_path()),"--root",root,*args])
    except CyToolError as e:
        print(json.dumps({"error":{"code":e.code,"message":str(e)}}),file=sys.stderr);return 1
