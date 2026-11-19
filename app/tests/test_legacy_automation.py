"""Legacy data and script compatibility through the compiled native SDK."""
import hashlib,json,os,sqlite3,subprocess,sys,tempfile,time,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];APP=ROOT/"app"
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/"tests"))
from automation_client import Runtime,load_manifest
from legacy_jobs import migrate
from test_interface import prepare,stop_owned
EDITOR=Path(os.environ.get("CYGAME_EDITOR_EXE",str(ROOT/"CyGameEditor.exe")))

class LegacyCompatibility(unittest.TestCase):
 def test_legacy_database_ownership_durability_and_script_protocol(self):
  with tempfile.TemporaryDirectory(prefix="cymcp-legacy-",dir=ROOT/"data/reports") as td:
   root=Path(td);prepare(root);store=root/"data/jobs";store.mkdir(parents=True)
   old=store/"artifact.bin";old.write_bytes(b"preserved legacy result")
   token="local-fixture-token-not-an-account";client={"clientId":"legacy-client","name":"Fixture","permissions":[],"tokenHash":hashlib.sha256(token.encode()).hexdigest()}
   session={"sessionId":"legacy-session","clientId":client["clientId"],"closed":False,"temporary":False}
   records=[("clients",client["clientId"],client),("sessions",session["sessionId"],session)]
   for identity,state in (("legacy-complete","Completed"),("legacy-running","Running")):
    job={"jobId":identity,"clientId":client["clientId"],"sessionId":session["sessionId"],"state":state,"operationId":"build_execute","outputs":{"path":str(old)},"workspace":str(store)}
    records.append(("jobs",identity,job))
   database=store/"state.sqlite3"
   with sqlite3.connect(database) as db:
    db.execute("CREATE TABLE records(kind TEXT,id TEXT,value TEXT,PRIMARY KEY(kind,id))")
    db.executemany("INSERT INTO records VALUES(?,?,?)",[(k,i,json.dumps(v)) for k,i,v in records])
   db.close()
   original=hashlib.sha256(database.read_bytes()).hexdigest()
   with Runtime(load_manifest(APP/"CyTool.json"),store,editor_root=root,allow_changes=False) as runtime:
    actor=runtime.authenticate(token)
    self.assertEqual(actor["clientId"],client["clientId"])
    self.assertEqual(runtime.get_job(actor,"legacy-complete")["outputs"]["path"],str(old))
    self.assertEqual(runtime.get_job(actor,"legacy-running")["state"],"Interrupted")
    self.assertTrue(runtime.finish_task(actor,session["sessionId"])["resultsRetained"])
    self.assertTrue(runtime._sessions[session["sessionId"]]["closed"])
    self.assertEqual(runtime.get_job(actor,"legacy-complete")["state"],"Completed")
    outsider=runtime.authenticate(runtime.create_client("other")["token"])
    with self.assertRaisesRegex(Exception,"another authenticated identity"):runtime.get_job(outsider,"legacy-complete")
   self.assertEqual(hashlib.sha256(database.read_bytes()).hexdigest(),original)
   self.assertEqual(old.read_bytes(),b"preserved legacy result")
   self.assertEqual(migrate(store,store)["importedJobs"],0)
   env={**os.environ,"CYGAME_EDITOR_ROOT":str(root),"PYTHONUTF8":"1"}
   def script(*args,input=None):
    return subprocess.run([sys.executable,str(APP/"tool.py"),"--data-dir",str(store),*args],input=input,capture_output=True,env=env,timeout=25)
   result=script("describe-operation","scene_list")
   self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(json.loads(result.stdout)["id"],"scene_list")
   result=script("stdio",input=b'{"protocolVersion":"1.0","id":"fixture","method":"ListOperations"}\n')
   self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(len(json.loads(result.stdout)["result"]),177)
   process=subprocess.Popen([str(EDITOR),"--root",str(root),"--exit-after","45"])
   try:
    until=time.monotonic()+12
    while not (root/"data/ipc/editor.json").exists():
     self.assertIsNone(process.poll());self.assertLess(time.monotonic(),until);time.sleep(.03)
    result=script("invoke","editor_status")
    self.assertEqual(result.returncode,0,result.stderr);job=json.loads(result.stdout);self.assertEqual(job["state"],"Completed");self.assertEqual(job["outputs"]["adapter"]["version"],"0.31.0")
    result=script("invoke","property_set","--params",'{"entityId":"hero","propertyId":"name","value":"Denied legacy edit"}')
    self.assertEqual(result.returncode,3,result.stdout+result.stderr)
    result=script("--allow-write","invoke","property_set","--params",'{"entityId":"hero","propertyId":"name","value":"Legacy CLI edit"}')
    self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(json.loads(result.stdout)["state"],"Completed")
   finally:stop_owned(process)
   (ROOT/"data/reports/cymcp-legacy.json").write_text(json.dumps({"passed":True,"originalDatabaseUnchanged":True,"tokenOwnershipRetained":True,"interruptedNotReplayed":True,"resultsRetained":True,"oldCLIAndProtocol":True,"legacyDenialExit":3,"nativeVersion":"0.31.0"},indent=2),encoding="utf8")
if __name__=="__main__":unittest.main(verbosity=2)
