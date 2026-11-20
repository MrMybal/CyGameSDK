"""End-to-end CyMCPSDK CLI/MCP/ChatUI integration; no paid or real-provider generation."""
import asyncio,base64,hashlib,json,os,shutil,subprocess,sys,tempfile,time,unittest
from pathlib import Path
from test_interface import prepare,stop_owned
from cymcp_provider_fixture import ProviderFixture
from mcp import ClientSession,StdioServerParameters
from mcp.client.stdio import stdio_client
ROOT=Path(__file__).resolve().parents[2];REPORTS=ROOT/"data/reports"
EDITOR=Path(os.environ.get("CYGAME_EDITOR_EXE",str(ROOT/"CyGameEditor.exe")))
CLI=Path(os.environ.get("CYGAME_CLI_EXE",str(ROOT/"CyGameCLI.exe")))
sys.path.insert(0,str(ROOT/"app"))
import tool
class NativeAutomation(unittest.TestCase):
    def test_01_cli_mcp_live_permissions_and_results(self):
        with tempfile.TemporaryDirectory(prefix="cymcp-live-",dir=REPORTS) as td:
            root=Path(td);prepare(root);settings=root/"data/settings";settings.mkdir(parents=True);(settings/"automation.json").write_text('{"access_profile":"protected"}',encoding="utf8");marker=root/"ready"
            process=subprocess.Popen([str(EDITOR),"--root",str(root),"--capture",str(root/"end.bmp"),"--capture-ready",str(marker),"--exit-after","50"])
            previous=tool.ROOT;tool.ROOT=root
            try:
                until=time.monotonic()+12
                while not (root/"data/ipc/editor.json").exists():
                    self.assertIsNone(process.poll());self.assertLess(time.monotonic(),until);time.sleep(.03)
                def cli(*args,code=0):
                    p=subprocess.run([str(CLI),"--root",str(root),*args],capture_output=True,timeout=20)
                    self.assertEqual(p.returncode,code,p.stderr.decode("utf8",errors="replace"))
                    if code:self.assertTrue(p.stderr);return json.loads(p.stderr)
                    self.assertFalse(p.stderr,p.stderr);return json.loads(p.stdout)
                self.assertEqual(cli("diagnose")["sdkVersion"],"0.7.1")
                self.assertTrue(cli("describe-operation","property_set")["inputSchema"]["properties"])
                args=root/"args.json";args.write_text(json.dumps({"entityId":"hero","propertyId":"name","value":"CLI épreuve"}),encoding="utf8")
                denied=cli("invoke","property_set","--params-file",str(args),code=3);self.assertIn(denied["error"]["code"],("PermissionDenied","AccessDenied"))
                changed=cli("--allow-write","invoke","property_set","--params-file",str(args));self.assertEqual(changed["entity"]["properties"]["name"],"CLI épreuve")
                self.assertEqual(cli("invoke","scene_list")["entities"][1]["properties"]["name"],"CLI épreuve")
                cli("--allow-write","invoke","edit_undo");self.assertNotEqual(cli("invoke","scene_list")["entities"][1]["properties"]["name"],"CLI épreuve")
                bad=root/"bad.json";bad.write_text('{"entityId":"hero","propertyId":"position","value":[1]}',encoding="utf8");cli("--allow-write","invoke","property_set","--params-file",str(bad),code=1)
                async def mcp(compact,allow):
                    args=["--root",str(root),"mcp","--compact" if compact else "--full"]
                    if allow:args.insert(0,"--allow-write")
                    async with stdio_client(StdioServerParameters(command=str(CLI),args=args)) as (r,w):
                        async with ClientSession(r,w) as client:
                            await client.initialize();names=[];page=await client.list_tools();names.extend(x.name for x in page.tools)
                            while page.nextCursor:page=await client.list_tools(cursor=page.nextCursor);names.extend(x.name for x in page.tools)
                            if compact:
                                self.assertEqual(len(names),5)
                                search=await client.call_tool("cy_tools_search",{"query":"property_set"});self.assertFalse(search.isError)
                                desc=await client.call_tool("cy_tools_describe",{"name":"property_set"});self.assertFalse(desc.isError)
                                result=await client.call_tool("cy_tools_call",{"name":"editor_status","arguments":{}});self.assertFalse(result.isError);value=result.structuredContent
                                self.assertIn("resource",value)
                                handle=value["resource"]["uri"];resource=await client.call_tool("cy_resources_read",{"uri":handle});self.assertFalse(resource.isError)
                                manifest=json.loads(resource.structuredContent["contents"][0]["text"]);cursor=manifest["firstUri"];chunks=[]
                                while cursor:
                                    response=await client.call_tool("cy_resources_read",{"uri":cursor});self.assertFalse(response.isError)
                                    page=response.structuredContent;chunks.append(page["contents"][0]["text"]);cursor=page.get("_meta",{}).get("com.cymcp/chunk",{}).get("nextUri")
                                recovered=json.loads("".join(chunks));self.assertIn("adapter",recovered);self.assertEqual(len(recovered["commands"]),177)
                                self.assertTrue(any(p.is_file() for p in (root/"data/automation/spool").rglob("*")))
                                revision=tool.editor_call("editor_status",{})["revision"];await asyncio.sleep(2.1)
                                expired=await client.call_tool("cy_resources_read",{"uri":handle});self.assertTrue(expired.isError)
                                self.assertEqual(tool.editor_call("editor_status",{})["revision"],revision)
                                return {"compact":True,"tools":len(names),"largeResultHandle":True,"completeRecovery":True,"diskSpooling":True,"expiredWithoutReplay":True}
                            self.assertIn("scene_list",names);self.assertIn("op_property_set",names)
                            result=await client.call_tool("scene_list",{});self.assertFalse(result.isError)
                            result=await client.call_tool("property_set",{"entityId":"hero","propertyId":"name","value":"External MCP edit"})
                            self.assertEqual(result.isError,not allow)
                            if allow:
                                self.assertEqual(tool.editor_call("scene_list",{})["entities"][1]["properties"]["name"],"External MCP edit")
                                undone=await client.call_tool("edit_undo",{});self.assertFalse(undone.isError)
                            return {"compact":False,"tools":len(names),"writesAllowed":allow}
                evidence=asyncio.run(mcp(False,False));allowed=asyncio.run(mcp(False,True))
                (root/"data/settings/automation.json").write_text(json.dumps({"access_profile":"protected","resultTtlMs":2000,"spoolThresholdBytes":1024}),encoding="utf8")
                compact=asyncio.run(mcp(True,False))
                probe=subprocess.run([str(CLI.parent/"CyGameMCPProbe.exe"),str(CLI),str(root)],capture_output=True,timeout=15)
                self.assertEqual(probe.returncode,0,probe.stderr.decode("utf8",errors="replace"));probe_evidence=json.loads(probe.stdout)
                (REPORTS/"cymcp-live.json").write_text(json.dumps({"passed":True,"cliReadWriteUndo":True,"cliDeniedExit":3,"mcpDenied":evidence,"mcpAllowed":allowed,"compact":compact,"cacheProbe":probe_evidence},indent=2),encoding="utf8")
            finally:marker.write_text("done",encoding="utf8");stop_owned(process);tool.ROOT=previous
    def test_02_integrated_chat_stream_tool_cancel_restore_and_context(self):
        with tempfile.TemporaryDirectory(prefix="cymcp-chat-",dir=REPORTS) as td,ProviderFixture() as fixture:
            root=Path(td);prepare(root);settings=root/"data/settings";settings.mkdir(parents=True)
            (settings/"automation.json").write_text(json.dumps({"access_profile":"protected","allowChanges":True,"performance":True,"contextWindow":True,"contextKiB":16,"compactDiscovery":True,"saveHistory":True}),encoding="utf8")
            connection={"action":"connect","provider":"openai-compatible","model":"cygame-fixture","settings":{"base_url":fixture.url}}
            report=REPORTS/"cymcp-chat.json";capture=REPORTS/"cymcp-chat.bmp"
            steps=[connection,{"action":"send","text":"TOOL_EDIT","contains":"EDIT_DONE","streaming":True,"entityName":"Assistant fixture edit"},{"action":"rights","allowChanges":False},{"action":"send","text":"TOOL_DENY","contains":"REFUSED","entityName":"Assistant fixture edit"},{"action":"send","text":"CANCEL_SILENT","cancelAfterMs":220,"cancelled":True}]
            steps += [{"action":"send","text":"LONG_"+str(i)+" SensitiveFixtureCanary","contains":"LONG_REPLY"} for i in range(7)]
            steps += [{"action":"save"},{"action":"disconnect"},{**connection,"restore":True,"minimumMessages":20}]
            plan=root/"plan.json";plan.write_text(json.dumps({"report":str(report),"steps":steps}),encoding="utf8")
            p=subprocess.run([str(EDITOR),"--root",str(root),"--test-assistant",str(plan),"--capture",str(capture),"--panel","assistant","--exit-after","50"],timeout=60)
            evidence=json.loads(report.read_text(encoding="utf8"));self.assertEqual(p.returncode,0,evidence);self.assertTrue(evidence["passed"],evidence)
            self.assertTrue(all(x["toolGroupsValid"] for x in fixture.requests))
            self.assertTrue(any(x["bytes"]>5000 for x in fixture.requests));self.assertTrue(all(x["bytes"]<20000 for x in fixture.requests),fixture.requests)
            history=list((root/"data/automation/conversations").glob("*.json"));self.assertTrue(history)
            self.assertGreater(sum(x.stat().st_size for x in history),40000)
            metrics=list((root/"data/automation/metrics").glob("*.jsonl"));self.assertTrue(metrics);raw="\n".join(p.read_text(encoding="utf8") for p in metrics)
            self.assertNotIn("SensitiveFixtureCanary",raw);self.assertNotIn("Assistant fixture edit",raw);self.assertNotIn("session_id",raw);self.assertNotIn("Bearer",raw)
            evidence.update({"fixture":"offline API, not a real model","apiContextRequests":fixture.requests,"historyBytes":sum(x.stat().st_size for x in history),"metricsMetadataOnly":True,"capture":str(capture)})
            report.write_text(json.dumps(evidence,indent=2),encoding="utf8")

    def test_03_authenticated_http_durable_files_and_interruption(self):
        import secrets,urllib.request,urllib.error
        with tempfile.TemporaryDirectory(prefix="cymcp-recovery-",dir=REPORTS) as td:
            root=Path(td);prepare(root);settings=root/"data/settings";settings.mkdir(parents=True)
            (settings/"automation.json").write_text('{"access_profile":"protected","allowChanges":false}',encoding="utf8")
            marker=root/"end";process=subprocess.Popen([str(EDITOR),"--root",str(root),"--panel","assistant","--capture",str(root/"end.bmp"),"--capture-ready",str(marker),"--exit-after","45"])
            try:
                endpoint_file=root/"data/ipc/automation.json";until=time.monotonic()+12
                while not endpoint_file.exists():self.assertIsNone(process.poll());self.assertLess(time.monotonic(),until);time.sleep(.03)
                endpoint=json.loads(endpoint_file.read_text(encoding="utf8"));url=endpoint["url"]+"/v1"
                request=urllib.request.Request(url,data=b'{"method":"invoke","name":"scene_list","arguments":{}}',headers={"Authorization":"Bearer "+("x"*64),"Content-Type":"application/json"})
                with self.assertRaises(urllib.error.HTTPError) as denied:urllib.request.urlopen(request,timeout=5)
                self.assertEqual(denied.exception.code,401)
                env={**os.environ,"CYGAME_MCP_TOKEN":endpoint["token"]}
                p=subprocess.run([str(CLI),"--root",str(root),"--connect",url,"invoke","scene_list"],env=env,capture_output=True,timeout=10);self.assertEqual(p.returncode,0,p.stderr);self.assertIn("entities",json.loads(p.stdout))
                args=root/"change.json";args.write_text('{"entityId":"hero","propertyId":"name","value":"Denied remote"}',encoding="utf8")
                p=subprocess.run([str(CLI),"--root",str(root),"--allow-write","--connect",url,"invoke","property_set","--params-file",str(args)],env=env,capture_output=True,timeout=10);self.assertEqual(p.returncode,3,p.stderr)
                output=subprocess.run([str(CLI),"--root",str(root),"--allow-write","invoke","build_execute","--params",'{"targetId":"sandbox.document"}'],capture_output=True,timeout=15)
                self.assertEqual(output.returncode,0,output.stderr);business=json.loads(output.stdout);expected=Path(business["archivePath"]).read_bytes()
                jobs=list((root/"data/automation/jobs").glob("*/job.json"));self.assertEqual(len(jobs),1);job=json.loads(jobs[0].read_text(encoding="utf8"));self.assertEqual(job["state"],"Completed")
                slow=root/"worker.json";slow.write_text(json.dumps({"build_providers":{"sandbox.archive":{"command":[sys.executable,str(ROOT/"app/tests/slow_build_worker.py")]}}}),encoding="utf8")
                child=subprocess.Popen([str(CLI),"--root",str(root),"--allow-write","--worker-config",str(slow),"invoke","build_execute","--params",'{"targetId":"sandbox.document"}'],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
                try:
                    until=time.monotonic()+10
                    while True:
                        markers=list((root/"data/automation/jobs").glob("*/worker-started.txt"))
                        if markers:break
                        self.assertIsNone(child.poll());self.assertLess(time.monotonic(),until);time.sleep(.03)
                    interrupted_id=markers[0].parent.name;child.terminate();child.wait(timeout=5)
                finally:
                    if child.poll() is None:child.kill();child.wait()
                    child.stdout.close();child.stderr.close()
            finally:marker.write_text("done",encoding="utf8");stop_owned(process)
            def invoke(name,args):
                p=subprocess.run([str(CLI),"--root",str(root),"invoke",name,"--params",json.dumps(args)],capture_output=True,timeout=10);self.assertEqual(p.returncode,0,p.stderr);return json.loads(p.stdout)
            self.assertEqual(invoke("job_get",{"jobId":interrupted_id})["state"],"Interrupted")
            self.assertEqual(invoke("job_get",{"jobId":job["jobId"]})["state"],"Completed")
            offset=0;recovered=bytearray()
            while True:
                chunk=invoke("job_output_read",{"jobId":job["jobId"],"path":"outputs/package.zip","offset":offset,"length":257});recovered+=base64.b64decode(chunk["base64"]);offset=chunk["nextOffset"]
                if chunk["complete"]:break
            self.assertEqual(bytes(recovered),expected)
            self.assertEqual(len(list((root/"data/automation/jobs").glob("*/worker-started.txt"))),1)
            (REPORTS/"cymcp-recovery.json").write_text(json.dumps({"passed":True,"unauthenticatedHttpDenied":True,"remotePolicyCannotBeOverridden":True,"durableAfterEditorExit":True,"completeFileRecovered":True,"bytes":len(recovered),"interruptedWithoutReplay":True},indent=2),encoding="utf8")

if __name__=="__main__":unittest.main(verbosity=2)
