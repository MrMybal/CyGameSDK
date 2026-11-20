"""Actual editor/HTTP profile checks; no provider account or GitHub workflow."""
import asyncio,json,os,shutil,socket,subprocess,sys,tempfile,time,unittest,urllib.request,urllib.error
from pathlib import Path
import httpx
from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client
ROOT=Path(__file__).resolve().parents[2];APP=ROOT/"app";REPORTS=ROOT/"data/reports"
sys.path[:0]=[str(APP),str(APP/"tests")]
from test_interface import prepare,stop_owned
EDITOR=Path(os.environ.get("CYGAME_EDITOR_EXE",str(ROOT/"CyGameEditor.exe")))
CLI=Path(os.environ.get("CYGAME_CLI_EXE",str(ROOT/"CyGameCLI.exe")))
def endpoint_call(url,name,args=None,token=None):
 request=urllib.request.Request(url,data=json.dumps({"method":"invoke","name":name,"arguments":args or {}}).encode(),headers={"Content-Type":"application/json",**({"Authorization":"Bearer "+token} if token else {})})
 try:
  with urllib.request.urlopen(request,timeout=8) as response:return response.status,json.load(response)
 except urllib.error.HTTPError as e:
  data=e.read()
  try:value=json.loads(data)
  except ValueError:value={}
  return e.code,value
class AccessProfiles(unittest.TestCase):
 def test_01_real_http_profiles_default_migration_and_cli(self):
  evidence=[]
  for profile,require_token,controlled in [(None,False,False),("free",False,False),("local_controlled",False,True),("token_only",True,False),("protected",True,True)]:
   with self.subTest(profile=profile),tempfile.TemporaryDirectory(prefix="mcp-access-",dir=REPORTS) as td:
    root=Path(td);prepare(root);settings=root/"data/settings";settings.mkdir(parents=True)
    config={"allowChanges":False,"unknownMetadata":{"keep":[1,"original"]}}
    if profile is not None:config["access_profile"]=profile
    path=settings/"automation.json";path.write_text(json.dumps(config),encoding="utf8")
    process=subprocess.Popen([str(EDITOR),"--root",str(root),"--panel","assistant","--exit-after","45"])
    try:
     until=time.monotonic()+15;endpoint_file=root/"data/ipc/automation.json"
     while not endpoint_file.exists():
      self.assertIsNone(process.poll());self.assertLess(time.monotonic(),until);time.sleep(.03)
     endpoint=json.loads(endpoint_file.read_text(encoding="utf8"));canonical=profile or "free";url=endpoint["url"]+"/v1";token=endpoint.get("token")
     self.assertEqual(endpoint["accessProfile"],canonical);self.assertEqual(endpoint["requireToken"],require_token)
     self.assertEqual(bool(token),require_token);self.assertTrue(endpoint["url"].startswith("http://127.0.0.1:"))
     saved=json.loads(path.read_text(encoding="utf8"));self.assertEqual(saved["access_profile"],canonical);self.assertEqual(saved["unknownMetadata"],config["unknownMetadata"]);self.assertFalse(saved["allowChanges"])
     code,scene=endpoint_call(url,"scene_list")
     self.assertEqual(code,401 if require_token else 200)
     if require_token:
      self.assertEqual(endpoint_call(url,"scene_list",token="wrong-local-test-token")[0],401)
     code,info=endpoint_call(url,"automation_info",token=token)
     self.assertEqual(code,200);self.assertEqual(info["sdkRevision"],"34d4bcae126bc83d1a2925b5047dac36e3d09d92");self.assertEqual(info["sdkVersion"],"0.7.1");self.assertEqual(info["controlOperations"],controlled)
     async def mcp():
      async with httpx.AsyncClient(headers={"Authorization":"Bearer "+token} if token else {},timeout=10) as http:
       async with streamable_http_client(endpoint["mcpUrl"],http_client=http) as (r,w,_):
        async with ClientSession(r,w) as client:
         await client.initialize();tools=await client.list_tools();self.assertEqual(len(tools.tools),5)
         self.assertFalse((await client.call_tool("cy_tools_describe",{"name":"property_set"})).isError)
         self.assertFalse((await client.call_tool("cy_tools_call",{"name":"scene_list","arguments":{}})).isError)
         name="HTTP "+canonical
         changed=await client.call_tool("cy_tools_call",{"name":"property_set","arguments":{"entityId":"hero","propertyId":"name","value":name}})
         self.assertEqual(changed.isError,controlled)
         if not controlled:
          self.assertEqual(endpoint_call(url,"scene_list",token=token)[1]["entities"][1]["properties"]["name"],name)
          self.assertFalse((await client.call_tool("cy_tools_call",{"name":"edit_undo","arguments":{}})).isError)
          invalid=await client.call_tool("cy_tools_call",{"name":"property_set","arguments":{"entityId":"hero","propertyId":"position","value":[1]}})
          self.assertTrue(invalid.isError)
     asyncio.run(mcp())
     # Same HTTP policy from native CLI; --allow-write cannot bypass a remote controlled server.
     env={k:v for k,v in os.environ.items() if k not in ("CYGAME_MCP_TOKEN","CYTOOLS_TOKEN")}
     args=[str(CLI),"--root",str(root),"--connect",url]
     if token:env["CYGAME_PROFILE_TEST_TOKEN"]=token;args+=["--token-env","CYGAME_PROFILE_TEST_TOKEN"]
     value=root/"change.json";value.write_text('{"entityId":"hero","propertyId":"name","value":"CLI through HTTP"}',encoding="utf8")
     completed=subprocess.run(args+["--allow-write","invoke","property_set","--params-file",str(value)],capture_output=True,env=env,timeout=12)
     self.assertEqual(completed.returncode,3 if controlled else 0,completed.stderr)
     if require_token:
      denied=subprocess.run([str(CLI),"--root",str(root),"--connect",url,"invoke","scene_list"],capture_output=True,env={k:v for k,v in env.items() if k!="CYGAME_PROFILE_TEST_TOKEN"},timeout=12)
      self.assertNotEqual(denied.returncode,0)
     if controlled:
      saved["allowChanges"]=True;path.write_text(json.dumps(saved),encoding="utf8")
      self.assertEqual(endpoint_call(url,"property_set",{"entityId":"hero","propertyId":"name","value":"Controlled grant"},token)[0],200)
     if profile is None:
      # Existing Python CLI also reaches this URL with no token or extra write grant.
      env.update({"CYGAME_CLI_EXE":str(CLI),"CYGAME_EDITOR_ROOT":str(root)})
      legacy=subprocess.run([sys.executable,str(APP/"tool.py"),"--data-dir",str(root/"data/legacy-client"),"--connect",url,"invoke","scene_list"],capture_output=True,env=env,timeout=15)
      self.assertEqual(legacy.returncode,0,legacy.stderr);self.assertEqual(json.loads(legacy.stdout)["state"],"Completed")
      direct=subprocess.run([str(CLI),"--root",str(root),"invoke","property_set","--params-file",str(value)],capture_output=True,env=env,timeout=12)
      self.assertEqual(direct.returncode,0,direct.stderr)
      sock=socket.socket();sock.bind(("127.0.0.1",0));port=sock.getsockname()[1];sock.close()
      server=subprocess.Popen([str(CLI),"--root",str(root),"serve","--port",str(port)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
      try:
       until=time.monotonic()+8
       while True:
        try:status,reply=endpoint_call(f"http://127.0.0.1:{port}/v1","scene_list");break
        except (urllib.error.URLError,TimeoutError):
         self.assertIsNone(server.poll());self.assertLess(time.monotonic(),until);time.sleep(.03)
       self.assertEqual(status,200)
       self.assertEqual(endpoint_call(f"http://127.0.0.1:{port}/v1","property_set",{"entityId":"hero","propertyId":"name","value":"Standalone Free"})[0],200)
      finally:server.terminate();server.communicate(timeout=8)
     evidence.append({"profile":canonical,"migrated":profile is None,"requireToken":require_token,"controlled":controlled,"realRead":True,"writeAllowedWithoutExtraGrant":not controlled,"validationKept":True})
    finally:stop_owned(process)
  (REPORTS/"cymcp-access-profiles.json").write_text(json.dumps({"passed":True,"profiles":evidence,"standaloneServeWithoutToken":True,"nativeAndLegacyCLI":True},indent=2),encoding="utf8")
 def test_02_explicit_profiles_aliases_invalid_config_and_disable(self):
  with tempfile.TemporaryDirectory(prefix="mcp-profile-config-",dir=REPORTS) as td:
   root=Path(td);prepare(root);folder=root/"data/settings";folder.mkdir(parents=True);path=folder/"automation.json"
   for key,value in [("access_profile","protected_access"),("accessProfile","token_only"),("localAccessProfile","local_controlled")]:
    config={key:value,"unrelated":{"keep":True},"allowChanges":False};path.write_text(json.dumps(config),encoding="utf8")
    p=subprocess.run([str(CLI),"--root",str(root),"diagnose"],capture_output=True,timeout=10);self.assertEqual(p.returncode,0,p.stderr)
    saved=json.loads(path.read_text(encoding="utf8"));self.assertEqual(saved[key],value);self.assertEqual(saved["unrelated"],config["unrelated"])
   for content in ('{"access_profile":"unknown","allowChanges":true}', '{"access_profile":"protected","accessProfile":"free"}','{"access_profile":'):
    path.write_text(content,encoding="utf8")
    p=subprocess.run([str(CLI),"--root",str(root),"diagnose"],capture_output=True,timeout=10)
    self.assertNotEqual(p.returncode,0);self.assertEqual(path.read_text(encoding="utf8"),content)
   path.write_text('{"access_profile":"protected","localMcpEnabled":false}',encoding="utf8")
   p=subprocess.run([str(CLI),"--root",str(root),"--access-profile","free","diagnose"],capture_output=True,timeout=10)
   self.assertEqual(json.loads(p.stdout)["accessProfile"],"free");self.assertEqual(json.loads(path.read_text(encoding="utf8"))["access_profile"],"protected")
   process=subprocess.Popen([str(EDITOR),"--root",str(root),"--panel","assistant","--exit-after","15"])
   try:
    until=time.monotonic()+10
    while not (root/"data/ipc/editor.json").exists():
     self.assertIsNone(process.poll());self.assertLess(time.monotonic(),until);time.sleep(.03)
    time.sleep(.3);self.assertFalse((root/"data/ipc/automation.json").exists())
   finally:stop_owned(process)
 def test_03_free_chat_tool_without_write_grant(self):
  from cymcp_provider_fixture import ProviderFixture
  with tempfile.TemporaryDirectory(prefix="mcp-free-chat-",dir=REPORTS) as td,ProviderFixture() as fixture:
   root=Path(td);prepare(root);settings=root/"data/settings";settings.mkdir(parents=True)
   (settings/"automation.json").write_text('{"allowChanges":false}',encoding="utf8")
   report=REPORTS/"cymcp-free-chat.json";plan=root/"plan.json"
   plan.write_text(json.dumps({"report":str(report),"steps":[
    {"action":"connect","provider":"openai-compatible","model":"cygame-fixture","settings":{"base_url":fixture.url}},
    {"action":"rights","allowChanges":False},
    {"action":"send","text":"TOOL_EDIT","contains":"EDIT_DONE","entityName":"Assistant fixture edit","streaming":True},
    {"action":"local_mcp","enabled":False},{"action":"local_mcp","enabled":True},
    {"action":"profile","profile":"protected"},{"action":"wait","milliseconds":1000}
   ]}),encoding="utf8")
   completed=subprocess.run([str(EDITOR),"--root",str(root),"--panel","assistant","--test-assistant",str(plan),"--capture",str(REPORTS/"cymcp-free-chat.bmp"),"--exit-after","30"],timeout=40)
   self.assertEqual(completed.returncode,0,report.read_text(encoding="utf8"));evidence=json.loads(report.read_text(encoding="utf8"))
   self.assertTrue(evidence["passed"]);self.assertEqual(evidence["entityName"],"Assistant fixture edit")
   saved=json.loads((settings/"automation.json").read_text(encoding="utf8"))
   self.assertEqual(saved["access_profile"],"protected");self.assertFalse(saved["allowChanges"])
   self.assertTrue(fixture.requests);self.assertTrue(all(x["toolGroupsValid"] for x in fixture.requests))
   evidence.update({"activeProfileDuringToolCall":"free","allowChanges":False,"storedNextProfile":"protected","provider":"offline API fixture; no real account/model"})
   report.write_text(json.dumps(evidence,indent=2),encoding="utf8")

if __name__=="__main__":unittest.main(verbosity=2)
