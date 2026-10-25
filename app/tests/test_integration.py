"""Integration checks against the real native editor model and CyToolsCore MCP."""
import asyncio
import hashlib
import zipfile
import ctypes
import json
import os
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
from cytools_core import CyToolError
from cytools_core.schema import load_manifest
from mcp import ClientSession,StdioServerParameters
from mcp.client.stdio import stdio_client

class EditorIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(prefix="editor-contract-",dir=ROOT/"data/reports")
        cls.workspace=Path(cls.temp.name)
        (cls.workspace/"app").mkdir()
        shutil.copyfile(APP/"CyTool.json",cls.workspace/"app/CyTool.json")
        cls.old_root=tool.ROOT;tool.ROOT=cls.workspace
        cls.gui=os.environ.get("CYGAME_TEST_GUI")=="1"
        native_args=[str(ROOT/"CyGameEditor.exe"),"--root",str(cls.workspace),"--exit-after","240"] if cls.gui else [str(ROOT/"runtime/build/CyGameSDKTests.exe"),"--serve",str(cls.workspace),"240"]
        cls.native=subprocess.Popen(native_args,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        for _ in range(200):
            if (cls.workspace/"data/ipc/editor.json").exists():break
            if cls.native.poll() is not None:raise RuntimeError(cls.native.communicate())
            time.sleep(.02)
        else:raise RuntimeError("Native editor test host did not start")
    @classmethod
    def tearDownClass(cls):
        if cls.gui and cls.native.poll() is None:
            # Ask only this owned test host to exit through its normal message loop.
            # TerminateProcess can leave graphics-driver I/O pending on Windows.
            user=ctypes.WinDLL("user32",use_last_error=True)
            user.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
            user.GetWindowThreadProcessId.restype=ctypes.c_uint32
            user.PostThreadMessageW.argtypes=[ctypes.c_uint32,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_ssize_t]
            callback_type=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_ssize_t)
            @callback_type
            def close_owned(hwnd,param):
                pid=ctypes.c_uint32();thread=user.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
                if pid.value==cls.native.pid:user.PostThreadMessageW(thread,0x0012,0,0)
                return 1
            user.EnumWindows.argtypes=[callback_type,ctypes.c_ssize_t]
            user.EnumWindows(close_owned,0)
        elif cls.native.poll() is None:cls.native.terminate()
        try:cls.native.wait(timeout=15)
        except subprocess.TimeoutExpired:
            cls.native.kill();cls.native.wait(timeout=15)
        if cls.native.stdout:cls.native.stdout.close()
        if cls.native.stderr:cls.native.stderr.close()
        fatal=cls.workspace/"data/logs/native-fatal.json"
        if fatal.exists():shutil.copyfile(fatal,ROOT/"data/reports/m18-native-fatal.json")
        tool.ROOT=cls.old_root
        assert cls.workspace.resolve().is_relative_to((ROOT/"data/reports").resolve())
        cls.temp.cleanup()
    def call(self,op,params=None):
        return tool.editor_call(op,params or {})
    def test_01_properties_undo_and_native_validation(self):
        before=self.call("scene_list")
        self.call("property_set",{"entityId":"hero","propertyId":"name","value":"Cube épreuve"})
        self.assertEqual(self.call("scene_list")["entities"][1]["properties"]["name"],"Cube épreuve")
        self.call("edit_undo")
        self.assertEqual(self.call("scene_list")["entities"],before["entities"])
        with self.assertRaises(CyToolError):self.call("property_set",{"entityId":"hero","propertyId":"scale","value":[-1,1,1]})
        self.assertEqual(self.call("scene_list")["entities"],before["entities"])
        self.assertTrue(self.call("editor_search",{"query":"position"})["results"])
    def test_02_graph_validation_execution_and_assets(self):
        self.assertEqual(self.call("graph_run",{"graphId":"logic"})["messages"],["Hello from the graph"])
        before=self.call("graph_get",{"graphId":"logic"})["graph"]
        for params in [
            {"fromNode":"begin","fromPin":"then","toNode":"print","toPin":"text"},
            {"fromNode":"begin","fromPin":"then","toNode":"print","toPin":"exec"},
            {"fromNode":"print","fromPin":"then","toNode":"print","toPin":"exec"}]:
            with self.assertRaises(CyToolError):self.call("graph_link_add",{"graphId":"logic",**params})
        self.assertEqual(self.call("graph_get",{"graphId":"logic"})["graph"],before)
        color=[.8,.25,.1]
        self.call("graph_node_value",{"graphId":"material","nodeId":"color","key":"value","value":color})
        self.assertEqual(self.call("graph_run",{"graphId":"material"})["baseColor"],color)
        a=self.call("asset_new_graph",{"name":"TestMaterial","domain":"material"})
        b=self.call("asset_new_graph",{"name":"SecondMaterial","domain":"material"})
        self.call("graph_node_value",{"graphId":a["graphId"],"nodeId":"color","key":"value","value":color})
        self.assertNotEqual(self.call("graph_run",{"graphId":b["graphId"]})["baseColor"],color)
        self.call("asset_save_graph",{"graphId":a["graphId"]})
        files=self.call("assets_list",{"query":"Test","type":"Graph"})["assets"]
        self.assertEqual(len(files),1)
        self.call("asset_open",{"path":"graphs/TestMaterial.cyasset"})
        self.assertEqual(self.call("graph_run",{"graphId":a["graphId"]})["baseColor"],color)
        with self.assertRaises(CyToolError):self.call("asset_open",{"path":"../ipc/editor.json"})
        with self.assertRaises(CyToolError):self.call("asset_new_graph",{"name":"TestMaterial","domain":"logic"})
    def test_03_project_roundtrip_and_invalid_load(self):
        self.call("project_save")
        previous=self.call("scene_list")["entities"]
        self.call("scene_create",{"name":"Roundtrip"})
        self.call("project_load")
        self.assertEqual(self.call("scene_list")["entities"],previous)
        project=self.workspace/"data/projects/Sandbox.cyproject.json"
        saved=project.read_text("utf-8")
        project.write_text('{"format":"wrong"}',"utf-8")
        try:
            with self.assertRaises(CyToolError):self.call("project_load")
            self.assertEqual(self.call("scene_list")["entities"],previous)
        finally:project.write_text(saved,"utf-8")
    def test_04_sdk_jobs_validation_and_privacy(self):
        with tool.create_runtime(self.workspace/"sdk-jobs") as runtime:
            alice=runtime.authenticate(runtime.create_client("Alice")["token"])
            bob=runtime.authenticate(runtime.create_client("Bob")["token"])
            session=runtime.create_session(alice,temporary=False)
            job=runtime.submit(alice,session["sessionId"],"scene_validate",{})
            result=runtime.wait(alice,job["jobId"])
            self.assertEqual(result["state"],"Completed",result)
            self.assertTrue(result["outputs"]["valid"])
            self.assertEqual(runtime.list_jobs(bob),[])
            with self.assertRaises(CyToolError):runtime.submit(alice,session["sessionId"],"scene_create",{"name":""})
    def test_05_official_mcp_stdio_roundtrip(self):
        async def run():
            env=dict(os.environ,CYGAME_EDITOR_ROOT=str(self.workspace))
            server=StdioServerParameters(command=sys.executable,args=[str(APP/"tool.py"),"--data-dir",str(self.workspace/"mcp-jobs"),"mcp"],env=env)
            async with stdio_client(server) as (reader,writer):
                async with ClientSession(reader,writer) as client:
                    await client.initialize()
                    catalogue=await client.list_tools()
                    expected={"op_"+op["id"] for op in load_manifest(APP/"CyTool.json")["operations"]}
                    self.assertTrue(expected.issubset({t.name for t in catalogue.tools}))
                    response=await client.call_tool("op_property_set",{"parameters":{"entityId":"hero","propertyId":"position","value":[3,2,1]}})
                    self.assertFalse(response.isError,response)
                    job=response.structuredContent["result"]
                    for _ in range(200):
                        status=await client.call_tool("cy_get_job",{"jobId":job["jobId"]})
                        result=status.structuredContent["result"]
                        if result["state"] in ("Completed","Failed","Cancelled"):break
                        await asyncio.sleep(.02)
                    self.assertEqual(result["state"],"Completed",result)
                    self.assertEqual(self.call("scene_list")["entities"][1]["properties"]["position"],[3,2,1])
                    async def operation(id,parameters):
                        call=await client.call_tool("op_"+id,{"parameters":parameters})
                        self.assertFalse(call.isError,call)
                        key=call.structuredContent["result"]["jobId"]
                        for _ in range(200):
                            poll=await client.call_tool("cy_get_job",{"jobId":key})
                            state=poll.structuredContent["result"]
                            if state["state"] in ("Completed","Failed","Cancelled"):break
                            await asyncio.sleep(.02)
                        self.assertEqual(state["state"],"Completed",state)
                        return state["outputs"]
                    self.assertEqual((await operation("graph_debug_start",{"graphId":"logic"}))["state"],"Paused")
                    self.assertEqual((await operation("graph_debug_continue",{"graphId":"logic"}))["messages"],["Hello from the graph"])
                    await operation("graph_debug_stop",{"graphId":"logic"})
                    graph=(await operation("asset_new_graph",{"name":"McpFunction","domain":"logic"}))["graphId"]
                    await operation("graph_function_create",{"graphId":graph,"name":"Identity"})
                    self.assertEqual((await operation("graph_function_evaluate",{"graphId":graph,"name":"Identity","arguments":{"x":6.5}}))["values"],{"result":6.5})
                    await operation("graph_function_create",{"graphId":graph,"name":"WithExecution","mode":"execution"})
                    self.assertEqual((await operation("graph_function_evaluate",{"graphId":graph,"name":"WithExecution","arguments":{"x":12}}))["values"],{"result":12})
                    diff=await operation("graph_diff",{"graphId":graph})
                    self.assertTrue(diff["changed"])
                    package=await operation("build_execute",{"targetId":"sandbox.document"})
                    self.assertTrue(Path(package["archivePath"]).is_file())
                    sample=await operation("material_sample",{"graphId":"material","u":.1,"v":.2})
                    self.assertIn("baseColor",sample)
                    shader=await operation("material_compile",{"graphId":"material"})
                    self.assertTrue(shader["success"]);self.assertTrue(Path(shader["shaderPath"]).read_bytes().startswith(b"DXBC"))
                    imported=await operation("asset_import",{"sourcePath":str(APP/"examples/Example_Image.png"),"destination":"McpImports/Image.png"})
                    await client.call_tool("cy_finish_task",{})
                    self.assertTrue((self.workspace/"data/assets"/imported["path"]).exists())
            return len(catalogue.tools)
        count=asyncio.run(asyncio.wait_for(run(),30))
        report={"nativeHost":"CyGameEditor.exe graphical host" if self.gui else "same EditorCore and EditorIpc; headless test host","mcp":"official CyToolsCore 0.10.1 / MCP 1.30.0 stdio","toolCount":count,"result":"passed","uiInteraction":"panel/plugin operations exercised" if self.gui else "not exercised by the headless suite"}
        (ROOT/"data/reports/integration.json").write_text(json.dumps(report,indent=2),"utf-8")

    def test_06_configuration_roundtrip_and_replay(self):
        import generation_config
        manifest=load_manifest(APP/"CyTool.json")
        params={"entityId":"hero","propertyId":"position","value":[4,1,2]}
        before=self.call("scene_list")["entities"]
        path=self.workspace/"configuration.json"
        generation_config.save(path,generation_config.recipe(manifest,"property_set",params))
        loaded=generation_config.load(path,manifest)
        self.assertEqual(loaded["parameters"],params)
        self.assertEqual(self.call("scene_list")["entities"],before)
        with tool.create_runtime(self.workspace/"recipe-jobs") as runtime:
            actor=runtime.authenticate(runtime.create_client("Replay")["token"])
            session=runtime.create_session(actor,temporary=False)
            job=generation_config.submit_config(runtime,actor,session["sessionId"],path)
            self.assertEqual(runtime.wait(actor,job["jobId"])["state"],"Completed")
        self.assertEqual(self.call("scene_list")["entities"][1]["properties"]["position"],[4,1,2])
    def test_07_graphical_host_panels_and_plugin(self):
        if not self.gui:self.skipTest("Set CYGAME_TEST_GUI=1 to exercise the graphical host")
        viewport=self.call("panel_open",{"panelId":"viewport","newInstance":True})["panelId"]
        self.assertTrue(viewport.startswith("viewport."))
        panels=self.call("editor_status")["panels"]
        self.assertIn(viewport,{p["id"] for p in panels})
        self.call("plugin_set_enabled",{"pluginId":"example.diagnostics","enabled":False})
        with self.assertRaises(CyToolError):self.call("panel_open",{"panelId":"diagnostics"})
        self.call("plugin_set_enabled",{"pluginId":"example.diagnostics","enabled":True})
        self.call("panel_open",{"panelId":"diagnostics"})
        self.call("panel_open",{"panelId":"logic"})
        self.call("panel_open",{"panelId":"viewport"})

    def test_08_asset_lifecycle_and_dirty_protection(self):
        asset=self.call("asset_new_graph",{"name":"WorkspaceMaterial","domain":"material","folder":"Library"})
        key=asset["graphId"]
        self.call("graph_node_value",{"graphId":key,"nodeId":"color","key":"value","value":[.1,.2,.3]})
        with self.assertRaises(CyToolError):self.call("asset_move",{"path":asset["path"],"destination":"Library/Renamed.cyasset"})
        self.call("asset_save_graph",{"graphId":key})
        copy=self.call("asset_copy",{"path":asset["path"],"destination":"Library/FX/Copy.cyasset"})
        with self.assertRaises(CyToolError):self.call("asset_copy",{"path":asset["path"],"destination":copy["path"]})
        self.call("property_set",{"entityId":"hero","propertyId":"name","value":"Before asset move"})
        moved=self.call("asset_move",{"path":asset["path"],"destination":"Library/Renamed.cyasset"})
        self.call("edit_undo")
        self.assertEqual(self.call("graph_get",{"graphId":key})["graph"]["assetPath"],moved["path"])
        self.assertEqual(self.call("asset_open",{"path":moved["path"]})["graphId"],key)
        filtered=self.call("assets_list",{"query":"type:Graph domain:material name:Renamed","type":"All"})
        self.assertEqual([a["path"] for a in filtered["assets"]],[moved["path"]])
        ticket=self.call("asset_trash",{"path":moved["path"]})
        self.assertIn(ticket,self.call("asset_trash_list")["tickets"])
        self.assertFalse((self.workspace/"data/assets"/moved["path"]).exists())
        with self.assertRaises(CyToolError):self.call("asset_save_graph",{"graphId":key})
        self.call("asset_restore",{"ticket":ticket["ticket"]})
        self.assertTrue((self.workspace/"data/assets"/moved["path"]).exists())
        self.call("asset_folder_create",{"path":"Library/Empty"})
        with self.assertRaises(CyToolError):self.call("asset_folder_create",{"path":"../outside"})
        with self.assertRaises(CyToolError):self.call("asset_move",{"path":copy["path"],"destination":"../outside.cyasset"})
        self.assertIn("Library/Empty",self.call("assets_list")["folders"])
    def test_09_workspace_roundtrip(self):
        if not self.gui:self.skipTest("Graphical host required")
        self.call("workspace_layout",{"preset":"Scripting"})
        self.call("workspace_save",{"name":"Integration"})
        self.call("workspace_layout",{"preset":"Materials"})
        self.call("workspace_load",{"name":"Integration"})
        panels={p["id"]:p for p in self.call("editor_status")["panels"]}
        self.assertTrue(panels["logic"]["open"])
        self.assertFalse(panels["outliner"]["open"])
        self.assertTrue(self.call("content_drawer_toggle")["open"])
        self.assertFalse(self.call("content_drawer_toggle")["open"])
        self.call("workspace_layout",{"preset":"Level"})

    def test_10_selection_transforms_locks_and_revisions(self):
        self.call("scene_select_many",{"entityIds":["hero","pillar"]})
        self.assertEqual(self.call("scene_list")["selection"],["hero","pillar"])
        before=self.call("scene_list")["entities"]
        revision=self.call("editor_status")["revision"]
        batch=[{"entityId":"hero","position":[1,2,3]},{"entityId":"pillar","rotation":[0,0,45]}]
        self.call("scene_transform_batch",{"transforms":batch,"expectedRevision":revision})
        self.call("edit_undo")
        self.assertEqual(self.call("scene_list")["entities"],before)
        with self.assertRaises(CyToolError):self.call("scene_transform_batch",{"transforms":batch,"expectedRevision":revision})
        self.assertEqual(self.call("scene_list")["entities"],before)
        self.call("scene_lock",{"entityIds":["pillar"],"locked":True})
        locked=self.call("scene_list")["entities"]
        with self.assertRaises(CyToolError):self.call("scene_transform_batch",{"transforms":batch})
        self.assertEqual(self.call("scene_list")["entities"],locked)
        with self.assertRaises(CyToolError):self.call("scene_delete_many",{"entityIds":["hero","pillar"]})
        self.call("scene_lock",{"entityIds":["pillar"],"locked":False})
        self.call("scene_folder_set",{"entityIds":["hero","pillar"],"folder":"Architecture/Props"})
        copied=self.call("scene_duplicate",{"entityIds":["hero","pillar"],"offset":[0,3,0]})["entities"]
        self.assertEqual(len(copied),2)
        self.assertTrue(all(e["folder"]=="Architecture/Props" for e in copied))
        self.assertEqual(self.call("scene_list")["selection"],[e["id"] for e in copied])
        self.call("edit_undo")
        self.assertEqual(self.call("scene_list")["selection"],["hero","pillar"])
        self.assertTrue(self.call("edit_history")["undo"])
        self.call("scene_select",{"entityId":"hero","mode":"replace"})


    def new_graph(self,name,domain="logic"):
        return self.call("asset_new_graph",{"name":name,"domain":domain})["graphId"]
    def node(self,g,type,**extra):
        return self.call("graph_node_add",{"graphId":g,"type":type,"position":[0,0],**extra})["node"]["id"]
    def wire(self,g,a,ap,b,bp):
        return self.call("graph_link_add",{"graphId":g,"fromNode":a,"fromPin":ap,"toNode":b,"toPin":bp})["link"]["id"]
    def test_11_typed_variables_debugger_and_program_changes(self):
        g=self.new_graph("DebugVariables")
        self.call("graph_variable_add",{"graphId":g,"name":"Counter","type":"float","default":1})
        before=self.call("graph_get",{"graphId":g})["graph"]
        for name,type,default in [("Bad","float","string"),("BadColor","color",[1,2,0])]:
            with self.assertRaises(CyToolError):self.call("graph_variable_add",{"graphId":g,"name":name,"type":type,"default":default})
        with self.assertRaises(CyToolError):self.node(g,"variable.get.bool",variableName="Counter")
        self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],before)
        setter=self.node(g,"variable.set.float",variableName="Counter")
        getter=self.node(g,"variable.get.float",variableName="Counter")
        convert=self.node(g,"convert.float_string")
        self.call("graph_link_remove",{"graphId":g,"linkId":"start"})
        self.wire(g,"begin","then",setter,"exec");self.wire(g,setter,"then","print","exec")
        self.wire(g,getter,"value",convert,"value");self.wire(g,convert,"value","print","text")
        self.call("graph_node_value",{"graphId":g,"nodeId":setter,"key":"value","value":7})
        self.call("graph_breakpoint_set",{"graphId":g,"nodeId":setter,"enabled":True})
        self.assertEqual(self.call("graph_debug_start",{"graphId":g})["state"],"Paused")
        stop=self.call("graph_debug_continue",{"graphId":g})
        self.assertEqual(stop["state"],"Breakpoint");self.assertEqual(stop["nextNode"],setter)
        self.assertEqual(stop["variables"]["Counter"],1)
        self.call("graph_node_move",{"graphId":g,"nodeId":setter,"position":[360,90]})
        self.call("graph_node_comment",{"graphId":g,"nodeId":setter,"text":"Assign the runtime counter"})
        self.assertEqual(self.call("graph_debug_status",{"graphId":g})["state"],"Breakpoint")
        step=self.call("graph_debug_step",{"graphId":g})
        self.assertEqual(step["state"],"Paused");self.assertEqual(step["variables"]["Counter"],7)
        complete=self.call("graph_debug_continue",{"graphId":g})
        self.assertEqual(complete["state"],"Completed");self.assertEqual(complete["messages"],["7"])
        self.assertEqual([t["nodeId"] for t in complete["trace"]],["begin",setter,"print"])
        self.assertEqual(complete["watches"][getter+".value"],7)
        self.assertEqual(self.call("graph_run",{"graphId":g})["variables"]["Counter"],7)
        self.assertEqual(self.call("graph_get",{"graphId":g})["graph"]["variables"]["Counter"]["default"],1)
        self.call("graph_debug_start",{"graphId":g})
        self.call("graph_node_value",{"graphId":g,"nodeId":setter,"key":"value","value":8})
        self.assertEqual(self.call("graph_debug_step",{"graphId":g})["state"],"Stale")
        self.call("edit_undo")
        self.assertEqual(self.call("graph_debug_status",{"graphId":g})["state"],"Paused")
        with self.assertRaises(CyToolError):self.call("graph_variable_remove",{"graphId":g,"name":"Counter"})
        self.call("graph_variable_rename",{"graphId":g,"name":"Counter","newName":"Score"})
        graph=self.call("graph_get",{"graphId":g})["graph"]
        self.assertTrue(all(n["variable"]=="Score" for n in graph["nodes"] if "variable" in n))
        self.call("graph_debug_stop",{"graphId":g})
        self.assertEqual(self.call("graph_debug_status",{"graphId":g})["state"],"Idle")

    def test_12_graph_diagnostics_duplication_and_runtime_failures(self):
        g=self.new_graph("Diagnostics")
        copies=self.call("graph_nodes_duplicate",{"graphId":g,"nodeIds":["begin","print"],"offset":[40,80]})["nodes"]
        ids={n["id"] for n in copies}
        graph=self.call("graph_get",{"graphId":g})["graph"]
        self.assertEqual(len([l for l in graph["links"] if l["fromNode"] in ids and l["toNode"] in ids]),1)
        self.assertEqual(self.call("graph_run",{"graphId":g})["messages"],["Hello from the graph"]*2)
        self.call("edit_undo")
        self.call("graph_node_remove",{"graphId":g,"nodeId":"begin"})
        diag=self.call("graph_diagnose",{"graphId":g})
        self.assertFalse(diag["valid"]);self.assertTrue(any(d["severity"]=="Error" for d in diag["diagnostics"]))
        with self.assertRaises(CyToolError):self.call("graph_debug_start",{"graphId":g})
        self.call("edit_undo")
        divide=self.node(g,"math.divide");convert=self.node(g,"convert.float_string")
        self.call("graph_node_value",{"graphId":g,"nodeId":divide,"key":"b","value":0})
        self.wire(g,divide,"value",convert,"value");self.wire(g,convert,"value","print","text")
        self.call("graph_debug_start",{"graphId":g})
        failed=self.call("graph_debug_continue",{"graphId":g})
        self.assertEqual(failed["state"],"Failed");self.assertIn("Division by zero",failed["error"])
        with self.assertRaises(CyToolError):self.call("graph_run",{"graphId":g})
        material=self.new_graph("ParameterSurface","material")
        self.call("graph_variable_add",{"graphId":material,"name":"Tint","type":"color","default":[.8,.15,.05]})
        getter=self.node(material,"variable.get.color",variableName="Tint")
        self.call("graph_link_remove",{"graphId":material,"linkId":"color_link"})
        self.wire(material,getter,"value","surface","baseColor")
        self.assertEqual(self.call("graph_run",{"graphId":material})["baseColor"],[.8,.15,.05])
        self.call("asset_save_graph",{"graphId":material})
        self.call("graph_variable_default",{"graphId":material,"name":"Tint","value":[.1,.2,.3]})
        with self.assertRaises(CyToolError):self.call("asset_reload",{"path":"graphs/ParameterSurface.cyasset"})
        self.call("asset_reload",{"path":"graphs/ParameterSurface.cyasset","discardChanges":True})
        self.assertEqual(self.call("graph_run",{"graphId":material})["baseColor"],[.8,.15,.05])
        self.call("graph_node_remove",{"graphId":material,"nodeId":"surface"})
        self.assertFalse(self.call("graph_diagnose",{"graphId":material})["valid"])


    def test_13_gizmo_mouse_drag_one_transaction_and_undo(self):
        if not self.gui:self.skipTest("Graphical host required")
        root=self.workspace/"gizmo-smoke";(root/"app").mkdir(parents=True)
        shutil.copyfile(APP/"CyTool.json",root/"app/CyTool.json")
        report=ROOT/"data/reports/gizmo-gesture.json"
        process=subprocess.run([str(ROOT/"CyGameEditor.exe"),"--root",str(root),"--workspace","Level","--test-gizmo",str(report),"--exit-after","15"],timeout=25)
        self.assertEqual(process.returncode,0,report.read_text("utf-8") if report.exists() else "No gesture report")
        self.assertTrue(json.loads(report.read_text("utf-8"))["passed"])


    def test_14_shipped_variable_examples(self):
        target=self.workspace/"data/assets/Examples";target.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(APP/"examples/Example_Image.png",target/"Example_Image.png")
        sources=list((APP/"examples").glob("*.cyasset"))
        for source in sources:shutil.copyfile(source,target/source.name)
        for source in sources:
            key=self.call("asset_open",{"path":"Examples/"+source.name})["graphId"]
            self.assertTrue(self.call("graph_diagnose",{"graphId":key})["valid"])
            if source.stem=="Example_BlueprintLatent":
                self.call("graph_debug_start",{"graphId":key});self.assertEqual(self.call("graph_debug_continue",{"graphId":key})["state"],"Waiting");self.assertEqual(self.call("graph_debug_tick",{"graphId":key,"delta":1})["state"],"Completed");continue
            result=self.call("graph_run",{"graphId":key})
            if source.stem=="Example_FunctionLogic":self.assertEqual(result["messages"],["6"])
            if source.stem=="Example_FunctionMaterial":self.assertAlmostEqual(result["baseColor"][0],.56)
            if source.stem=="Example_DebugCounter":self.assertEqual(result["messages"],["7"])
            if source.stem=="Example_ParameterMaterial":
                self.assertEqual(result["baseColor"],[.7,.12,.035]);self.assertEqual(result["roughness"],.24)


    def test_15_pure_functions_nested_calls_signatures_and_undo(self):
        g=self.new_graph("ReusableFunctions")
        self.call("graph_function_create",{"graphId":g,"name":"Double"})
        def f(op,**params):return self.call(op,{"graphId":g,"functionName":"Double",**params})
        function=f("graph_get")["graph"]
        f("graph_link_remove",linkId=function["links"][0]["id"])
        multiply=f("graph_node_add",type="math.multiply",position=[200,200])["node"]["id"]
        f("graph_node_value",nodeId=multiply,key="b",value=2)
        f("graph_link_add",fromNode="arguments",fromPin="x",toNode=multiply,toPin="a")
        f("graph_link_add",fromNode=multiply,fromPin="value",toNode="return",toPin="result")
        self.assertEqual(self.call("graph_function_evaluate",{"graphId":g,"name":"Double","arguments":{"x":3}})["values"],{"result":6})
        with self.assertRaises(CyToolError):self.call("graph_function_evaluate",{"graphId":g,"name":"Double","arguments":{"x":"bad"}})
        with self.assertRaises(CyToolError):f("graph_node_add",type="debug.print",position=[0,0])
        with self.assertRaises(CyToolError):f("graph_nodes_remove",nodeIds=["arguments"])
        call=self.node(g,"function.call",functionRef="Double")
        self.call("graph_node_value",{"graphId":g,"nodeId":call,"key":"x","value":4})
        convert=self.node(g,"convert.float_string");self.wire(g,call,"result",convert,"value");self.wire(g,convert,"value","print","text")
        self.assertEqual(self.call("graph_run",{"graphId":g})["messages"],["8"])
        self.call("graph_debug_start",{"graphId":g})
        f("graph_node_move",nodeId=multiply,position=[250,200])
        self.assertEqual(self.call("graph_debug_status",{"graphId":g})["state"],"Paused")
        original=self.call("graph_get",{"graphId":g})["graph"]
        with self.assertRaises(CyToolError):self.call("graph_function_signature",{"graphId":g,"name":"Double","inputs":{"x":{"type":"bool","default":False}},"outputs":{"result":{"type":"float","default":0}}})
        self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],original)
        self.call("graph_function_signature",{"graphId":g,"name":"Double","inputs":{"x":{"type":"float","default":0}},"outputs":{"result":{"type":"float","default":0},"flag":{"type":"bool","default":True}}})
        self.assertTrue(self.call("graph_function_evaluate",{"graphId":g,"name":"Double","arguments":{"x":3}})["values"]["flag"])
        self.call("graph_function_create",{"graphId":g,"name":"Quad"})
        q=self.call("graph_get",{"graphId":g,"functionName":"Quad"})["graph"]
        self.call("graph_link_remove",{"graphId":g,"functionName":"Quad","linkId":q["links"][0]["id"]})
        a=self.node(g,"function.call",functionName="Quad",functionRef="Double");b=self.node(g,"function.call",functionName="Quad",functionRef="Double")
        for source,pin,target,targetpin in [("arguments","x",a,"x"),(a,"result",b,"x"),(b,"result","return","result")]:
            self.call("graph_link_add",{"graphId":g,"functionName":"Quad","fromNode":source,"fromPin":pin,"toNode":target,"toPin":targetpin})
        self.assertEqual(self.call("graph_function_evaluate",{"graphId":g,"name":"Quad","arguments":{"x":3}})["values"]["result"],12)
        before=self.call("graph_get",{"graphId":g})["graph"]
        with self.assertRaises(CyToolError):self.node(g,"function.call",functionName="Double",functionRef="Quad")
        self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],before)
        with self.assertRaises(CyToolError):self.call("graph_function_remove",{"graphId":g,"name":"Double"})
        self.call("graph_function_rename",{"graphId":g,"name":"Double","newName":"Twice"})
        self.assertEqual(self.call("graph_function_evaluate",{"graphId":g,"name":"Quad","arguments":{"x":5}})["values"]["result"],20)
        self.call("asset_save_graph",{"graphId":g})
        self.call("asset_reload",{"path":"graphs/ReusableFunctions.cyasset"})
        self.assertEqual(self.call("graph_run",{"graphId":g})["messages"],["8"])
        self.function_graph=g
        if self.gui:
            panel=self.call("panel_open",{"panelId":"fn:"+g+"#Twice"})
            self.assertTrue(panel["panelId"].startswith("fn:"))
            time.sleep(.1)
            self.assertIn(panel["panelId"],{p["id"] for p in self.call("editor_status")["panels"]})

    def test_16_graph_batches_clipboard_and_comment_groups(self):
        g=self.new_graph("BatchGraph")
        original=self.call("graph_get",{"graphId":g})["graph"]
        self.call("graph_nodes_move",{"graphId":g,"positions":[{"nodeId":"begin","position":[80,120]},{"nodeId":"print","position":[420,120]}]})
        self.call("edit_undo");self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],original)
        with self.assertRaises(CyToolError):self.call("graph_nodes_move",{"graphId":g,"positions":[{"nodeId":"begin","position":[80,120]},{"nodeId":"missing","position":[420,120]}]})
        self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],original)
        fragment=self.call("graph_fragment_copy",{"graphId":g,"nodeIds":["begin","print"]})["fragment"]
        pasted=self.call("graph_fragment_paste",{"graphId":g,"fragment":fragment,"offset":[0,300]})["nodes"]
        self.assertEqual(len(pasted),2);self.assertEqual(self.call("graph_run",{"graphId":g})["messages"],["Hello from the graph"]*2)
        self.call("graph_nodes_remove",{"graphId":g,"nodeIds":[n["id"] for n in pasted]})
        self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],original)
        self.call("graph_comment_add",{"graphId":g,"text":"Startup","position":[10,10],"size":[650,220]})
        graph=self.call("graph_get",{"graphId":g})["graph"];self.assertEqual(graph["comments"][0]["text"],"Startup")
        self.call("asset_save_graph",{"graphId":g})
        with self.assertRaises(CyToolError):self.call("graph_comment_add",{"graphId":g,"text":"Bad","position":[0,0],"size":[-1,50]})
        self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],graph)
        functions=self.new_graph("TransferLibrary")
        self.call("graph_function_create",{"graphId":functions,"name":"Twice"})
        fn=self.call("graph_get",{"graphId":functions,"functionName":"Twice"})["graph"]
        self.call("graph_link_remove",{"graphId":functions,"functionName":"Twice","linkId":fn["links"][0]["id"]})
        multiply=self.node(functions,"math.multiply",functionName="Twice")
        for a,ap,b,bp in [("arguments","x",multiply,"a"),(multiply,"value","return","result")]:
            self.call("graph_link_add",{"graphId":functions,"functionName":"Twice","fromNode":a,"fromPin":ap,"toNode":b,"toPin":bp})
        self.node(functions,"function.call",functionRef="Twice")
        source=self.call("graph_get",{"graphId":functions})["graph"]
        call=next(n["id"] for n in source["nodes"] if n["type"]=="function.call")
        clip=self.call("graph_fragment_copy",{"graphId":functions,"nodeIds":[call]})["fragment"]
        copied=self.call("graph_fragment_paste",{"graphId":g,"fragment":clip})["nodes"][0]
        self.assertEqual(copied["function"],"Twice")
        self.assertEqual(self.call("graph_function_evaluate",{"graphId":g,"name":"Twice","arguments":{"x":9}})["values"]["result"],18)
        other=self.new_graph("ClipboardConflict")
        self.call("graph_function_create",{"graphId":other,"name":"Twice"})
        before=self.call("graph_get",{"graphId":other})["graph"]
        with self.assertRaises(CyToolError):self.call("graph_fragment_paste",{"graphId":other,"fragment":clip})
        self.assertEqual(self.call("graph_get",{"graphId":other})["graph"],before)


    def test_17_asset_references_missing_cycles_and_file_protection(self):
        target=self.call("asset_new_graph",{"name":"DependencyTarget","domain":"material"})
        consumer=self.call("asset_new_graph",{"name":"DependencyConsumer","domain":"material"})
        self.call("asset_references_set",{"graphId":consumer["graphId"],"paths":[target["path"],"textures/Missing.png"]})
        report=self.call("asset_dependencies",{"path":consumer["path"]})
        self.assertEqual(report["missing"],["textures/Missing.png"])
        self.assertEqual([a["path"] for a in self.call("asset_dependencies",{"path":target["path"]})["referencers"]],[consumer["path"]])
        before=self.call("graph_get",{"graphId":consumer["graphId"]})["graph"]
        with self.assertRaises(CyToolError):self.call("asset_references_set",{"graphId":consumer["graphId"],"paths":["../outside"]})
        self.assertEqual(self.call("graph_get",{"graphId":consumer["graphId"]})["graph"],before)
        for op in ["asset_move","asset_trash"]:
            params={"path":target["path"]}
            if op=="asset_move":params["destination"]="graphs/RenamedTarget.cyasset"
            with self.assertRaises(CyToolError):self.call(op,params)
        self.call("asset_save_graph",{"graphId":consumer["graphId"]})
        self.call("asset_references_set",{"graphId":target["graphId"],"paths":[consumer["path"]]})
        cyclic=self.call("asset_dependencies",{"path":consumer["path"]})
        self.assertIn(target["path"],cyclic["transitive"])
        self.call("asset_references_set",{"graphId":consumer["graphId"],"paths":[]})
        with self.assertRaises(CyToolError):self.call("asset_move",{"path":target["path"],"destination":"graphs/RenamedTarget.cyasset"})
        self.call("asset_save_graph",{"graphId":consumer["graphId"]})
        self.call("asset_save_graph",{"graphId":target["graphId"]})
        self.call("asset_move",{"path":target["path"],"destination":"graphs/RenamedTarget.cyasset"})
        self.call("edit_undo")
        restored=self.call("graph_get",{"graphId":consumer["graphId"]})["graph"]
        self.assertIn("graphs/RenamedTarget.cyasset",restored["references"])
        self.assertNotIn(target["path"],restored["references"])
        broken=self.workspace/"data/assets/Broken.cyasset";broken.write_text("{",encoding="utf-8")
        try:
            self.assertTrue(self.call("asset_dependencies",{"path":consumer["path"]})["incomplete"])
            with self.assertRaises(CyToolError):self.call("asset_trash",{"path":"graphs/RenamedTarget.cyasset"})
        finally:broken.unlink()
        if self.gui:self.call("asset_reference_view",{"path":consumer["path"]})


    def test_18_marquee_and_group_node_drag(self):
        if not self.gui:self.skipTest("Graphical host required")
        root=self.workspace/"graph-smoke";(root/"app").mkdir(parents=True)
        shutil.copyfile(APP/"CyTool.json",root/"app/CyTool.json")
        report=ROOT/"data/reports/graph-gesture.json"
        process=subprocess.run([str(ROOT/"CyGameEditor.exe"),"--root",str(root),"--workspace","Scripting","--test-graph",str(report),"--exit-after","15"],timeout=25)
        self.assertEqual(process.returncode,0,report.read_text("utf-8") if report.exists() else "No graph gesture report")
        self.assertTrue(json.loads(report.read_text("utf-8"))["passed"])


    def test_19_graph_navigation_reroutes_bookmarks_and_comments(self):
        g=self.new_graph("Navigation")
        before=self.call("graph_get",{"graphId":g})["graph"]
        reroute=self.call("graph_link_reroute",{"graphId":g,"linkId":"start","position":[280,90]})["node"]
        self.assertEqual(reroute["type"],"route.exec")
        self.assertEqual(self.call("graph_run",{"graphId":g})["messages"],["Hello from the graph"])
        self.call("edit_undo");self.assertEqual(self.call("graph_get",{"graphId":g})["graph"],before)
        self.call("graph_debug_start",{"graphId":g})
        mark=self.call("graph_bookmark_add",{"graphId":g,"name":"Startup","pan":[20,30],"zoom":.75})["bookmark"]
        self.assertEqual(self.call("graph_debug_status",{"graphId":g})["state"],"Paused")
        with self.assertRaises(CyToolError):self.call("graph_bookmark_add",{"graphId":g,"name":"Bad","pan":[0,0],"zoom":99})
        comment=self.call("graph_comment_add",{"graphId":g,"text":"Section","position":[0,0],"size":[600,250]})["comment"]
        self.call("graph_comment_edit",{"graphId":g,"commentId":comment["id"],"text":"Gameplay initialization","position":[5,10],"size":[700,280]})
        self.call("graph_node_comment",{"graphId":g,"nodeId":"print","text":"Inspect the greeting"})
        found=self.call("graph_find",{"graphId":g,"query":"greeting"})["results"]
        self.assertEqual([r["nodeId"] for r in found],["print"])
        self.call("graph_function_create",{"graphId":g,"name":"Identity"})
        results=self.call("graph_find",{"graphId":g,"query":"function.input"})["results"]
        self.assertEqual(results[0]["functionName"],"Identity")
        local={"graphId":g,"functionName":"Identity"}
        function=self.call("graph_get",local)["graph"]
        self.call("graph_link_reroute",{**local,"linkId":function["links"][0]["id"],"position":[200,90]})
        self.call("graph_bookmark_add",{**local,"name":"Function result","pan":[10,15],"zoom":1})
        self.assertEqual(self.call("graph_function_evaluate",{"graphId":g,"name":"Identity","arguments":{"x":8.25}})["values"]["result"],8.25)
        bookmark=self.call("graph_get",local)["graph"]["bookmarks"][0]
        self.call("graph_bookmark_remove",{**local,"bookmarkId":bookmark["id"]})
        self.assertEqual(self.call("graph_get",local)["graph"]["bookmarks"],[])
        self.call("asset_save_graph",{"graphId":g});self.call("asset_reload",{"path":"graphs/Navigation.cyasset"})
        graph=self.call("graph_get",{"graphId":g})["graph"]
        self.assertEqual(graph["bookmarks"][0]["name"],"Startup");self.assertEqual(graph["comments"][0]["size"],[700,280])
        material=self.new_graph("RouteMaterial","material")
        route=self.call("graph_link_reroute",{"graphId":material,"linkId":"color_link","position":[270,90]})["node"]
        self.assertEqual(route["type"],"route.color")
        self.assertEqual(self.call("graph_run",{"graphId":material})["baseColor"],[.2,.45,.8])
        if self.gui:self.call("panel_open",{"panelId":"navigator"})

    def test_20_material_instance_inheritance_and_validation(self):
        parent=self.call("asset_new_graph",{"name":"InstanceParent","domain":"material"});g=parent["graphId"]
        self.call("graph_variable_add",{"graphId":g,"name":"Tint","type":"color","default":[.2,.4,.7]})
        get=self.node(g,"variable.get.color",variableName="Tint")
        self.call("graph_link_remove",{"graphId":g,"linkId":"color_link"});self.wire(g,get,"value","surface","baseColor")
        self.call("asset_save_graph",{"graphId":g})
        child=self.call("asset_new_material_instance",{"name":"Child","parent":parent["path"]})
        self.assertEqual(self.call("material_get",{"graphId":child["graphId"]})["preview"]["baseColor"],[.2,.4,.7])
        self.call("material_override_set",{"graphId":child["graphId"],"name":"Tint","value":[.7,.2,.1]})
        second=self.call("asset_new_material_instance",{"name":"Grandchild","parent":child["path"]})
        self.assertEqual(self.call("graph_run",{"graphId":second["graphId"]})["baseColor"],[.7,.2,.1])
        self.assertEqual(self.call("graph_run",{"graphId":g})["baseColor"],[.2,.4,.7])
        before=self.call("graph_get",{"graphId":child["graphId"]})["graph"]
        for name,value in [("Tint",True),("Missing",1)]:
            with self.assertRaises(CyToolError):self.call("material_override_set",{"graphId":child["graphId"],"name":name,"value":value})
        with self.assertRaises(CyToolError):self.call("material_parent_set",{"graphId":child["graphId"],"parent":second["path"]})
        with self.assertRaises(CyToolError):self.call("material_parent_set",{"graphId":child["graphId"],"parent":"../outside"})
        self.assertEqual(self.call("graph_get",{"graphId":child["graphId"]})["graph"],before)
        with self.assertRaises(CyToolError):self.node(child["graphId"],"constant.float")
        self.call("material_override_remove",{"graphId":child["graphId"],"name":"Tint"})
        self.assertEqual(self.call("graph_run",{"graphId":second["graphId"]})["baseColor"],[.2,.4,.7])
        self.call("material_override_set",{"graphId":second["graphId"],"name":"Tint","value":[.1,.9,.3]})
        self.call("graph_variable_rename",{"graphId":g,"name":"Tint","newName":"Color"})
        with self.assertRaises(CyToolError):self.call("material_get",{"graphId":second["graphId"]})
        self.call("material_override_remove",{"graphId":second["graphId"],"name":"Tint"})
        self.assertEqual(self.call("material_get",{"graphId":second["graphId"]})["preview"]["baseColor"],[.2,.4,.7])
        with self.assertRaises(CyToolError):self.call("asset_move",{"path":parent["path"],"destination":"RenamedParent.cyasset"})
        self.call("asset_save_graph",{"graphId":second["graphId"]})
        if self.gui:self.call("panel_open",{"panelId":second["graphId"]})

    def test_21_build_plan_sdk_archive_and_privacy(self):
        self.call("project_load")
        before=self.call("editor_status")["revision"]
        descriptor=self.call("build_describe");self.assertEqual(descriptor["targets"][0]["providerId"],"sandbox.archive")
        plan=self.call("build_prepare",{"targetId":"sandbox.document","expectedRevision":before})
        self.assertTrue(plan["valid"],plan["diagnostics"])
        self.assertEqual(self.call("editor_status")["revision"],before)
        for parameters in [{"targetId":"unknown"},{"targetId":"sandbox.document","configuration":"Shipping"},
            {"targetId":"sandbox.document","options":{"compression":"Invalid"}},
            {"targetId":"sandbox.document","options":{"other":True}},
            {"targetId":"sandbox.document","expectedRevision":before-1}]:
            with self.assertRaises(CyToolError):self.call("build_prepare",parameters)
        with tool.create_runtime(self.workspace/"build-jobs") as runtime:
            alice=runtime.authenticate(runtime.create_client("Builder")["token"])
            bob=runtime.authenticate(runtime.create_client("Other")["token"])
            session=runtime.create_session(alice,temporary=False)
            for options in [{},{"compression":"Stored","includeMaterialValues":False}]:
                job=runtime.submit(alice,session["sessionId"],"build_execute",{"targetId":"sandbox.document","expectedRevision":before,"options":options})
                result=runtime.wait(alice,job["jobId"])
                self.assertEqual(result["state"],"Completed",result)
                output=result["outputs"];archive=Path(output["archivePath"])
                self.assertTrue(archive.resolve().is_relative_to(Path(result["workspace"]).resolve()))
                self.assertEqual(output["sha256"],hashlib.sha256(archive.read_bytes()).hexdigest())
                self.assertEqual(output["snapshotRevision"],before);self.assertFalse(output["gameExecutable"])
                with zipfile.ZipFile(archive) as packaged:
                    document=json.loads(packaged.read("document.cyproject.json"))
                    self.assertEqual(document["document"],plan["snapshot"]["document"])
                    self.assertEqual("material-values.json" in packaged.namelist(),options.get("includeMaterialValues",True))
                    expected=zipfile.ZIP_STORED if options.get("compression")=="Stored" else zipfile.ZIP_DEFLATED
                    self.assertTrue(all(entry.compress_type==expected for entry in packaged.infolist()))
                self.assertEqual(runtime.list_jobs(bob),[])
                with self.assertRaises(CyToolError):runtime.get_job(bob,job["jobId"])

    def test_22_build_diagnostics_and_worker_cancellation(self):
        from build_service import BuildProvider
        g=self.new_graph("BuildInvalid")
        self.call("graph_node_remove",{"graphId":g,"nodeId":"begin"})
        plan=self.call("build_prepare",{"targetId":"sandbox.document"})
        self.assertFalse(plan["valid"])
        with tool.create_runtime(self.workspace/"invalid-build-jobs") as runtime:
            actor=runtime.authenticate(runtime.create_client("Invalid build")["token"])
            session=runtime.create_session(actor,temporary=False)
            job=runtime.submit(actor,session["sessionId"],"build_execute",{"targetId":"sandbox.document"})
            result=runtime.wait(actor,job["jobId"])
            self.assertEqual(result["state"],"Failed",result);self.assertEqual(result["error"]["code"],"InvalidBuild")
            self.assertTrue((Path(result["workspace"])/"diagnostics/plan.json").exists())
            self.assertFalse(list(Path(result["workspace"]).rglob("*.zip")))
        self.call("project_load")
        provider=BuildProvider("sandbox.archive",[sys.executable,str(APP/"tests/slow_build_worker.py")])
        with tool.create_runtime(self.workspace/"cancel-build-jobs",build_providers={provider.id:provider}) as runtime:
            actor=runtime.authenticate(runtime.create_client("Cancel build")["token"])
            session=runtime.create_session(actor,temporary=False)
            job=runtime.submit(actor,session["sessionId"],"build_execute",{"targetId":"sandbox.document"})
            marker=Path(job["workspace"])/"worker-started.txt"
            deadline=time.monotonic()+10
            while not marker.exists() and time.monotonic()<deadline:time.sleep(.02)
            self.assertTrue(marker.exists(),runtime.get_job(actor,job["jobId"]))
            runtime.cancel(actor,job["jobId"])
            result=runtime.wait(actor,job["jobId"])
            self.assertEqual(result["state"],"Cancelled",result)
            self.assertIsNone(provider.worker.process);self.assertFalse(provider.worker.health()["alive"])
            self.assertFalse(list(Path(result["workspace"]).rglob("*.zip")))

    def test_23_graphical_build_client(self):
        if not self.gui:self.skipTest("Graphical build client required")
        self.call("project_load");self.call("panel_open",{"panelId":"build"})
        state=self.call("build_execute",{"targetId":"sandbox.document"})
        self.assertEqual(state["state"],"Created")
        with self.assertRaises(CyToolError):self.call("build_request",{"targetId":"sandbox.document"})
        deadline=time.monotonic()+20
        while time.monotonic()<deadline:
            state=self.call("build_status")
            if state["state"] in ("Completed","Failed","Cancelled"):break
            time.sleep(.05)
        self.assertEqual(state["state"],"Completed",state)
        self.assertTrue(Path(state["job"]["outputs"]["archivePath"]).is_file())
        self.call("build_request",{"targetId":"sandbox.document"});self.call("build_cancel")
        deadline=time.monotonic()+20
        while time.monotonic()<deadline:
            state=self.call("build_status")
            if state["state"] in ("Completed","Failed","Cancelled"):break
            time.sleep(.05)
        self.assertEqual(state["state"],"Cancelled",state)

    def execution_example(self,name):
        source=APP/"examples/Example_ExecutionFunctions.cyasset"
        target=self.workspace/"data/assets/graphs"/(name+".cyasset");target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
        return self.call("asset_open",{"path":"graphs/"+target.name})["graphId"]

    def test_24_execution_functions_locals_return_values_and_domains(self):
        g=self.new_graph("ExecutionAPI")
        self.call("graph_function_create",{"graphId":g,"name":"Identity","mode":"execution"})
        body=self.call("graph_get",{"graphId":g,"functionName":"Identity"})["graph"]
        self.assertEqual(body["mode"],"execution")
        self.assertEqual(self.call("graph_function_evaluate",{"graphId":g,"name":"Identity","arguments":{"x":6.5}})["values"],{"result":6.5})
        with self.assertRaises(CyToolError):self.call("graph_function_evaluate",{"graphId":g,"name":"Identity","arguments":{"x":True}})
        self.call("graph_function_create",{"graphId":g,"name":"Pure"})
        with self.assertRaises(CyToolError):self.call("graph_node_add",{"graphId":g,"functionName":"Pure","type":"function.call","functionRef":"Identity","position":[50,200]})
        material=self.new_graph("ExecForbiddenMaterial","material")
        before=self.call("graph_get",{"graphId":material})["graph"]
        with self.assertRaises(CyToolError):self.call("graph_function_create",{"graphId":material,"name":"Bad","mode":"execution"})
        self.assertEqual(self.call("graph_get",{"graphId":material})["graph"],before)
        g=self.execution_example("NestedExecution")
        result=self.call("graph_run",{"graphId":g})
        self.assertEqual(result["messages"],["Inside Report","6"]);self.assertEqual(result["variables"]["Counter"],900)
        value=self.call("graph_function_evaluate",{"graphId":g,"name":"Compute","arguments":{"x":10}})
        self.assertEqual(value["values"],{"result":11});self.assertEqual(value["variables"]["Counter"],10)
        self.assertEqual(self.call("graph_get",{"graphId":g,"functionName":"Compute"})["graph"]["variables"]["Counter"]["default"],100)
        self.assertEqual(self.call("graph_run",{"graphId":g})["messages"],["Inside Report","6"])
        return_graph=self.call("graph_get",{"graphId":g,"functionName":"Report"})["graph"]
        return_link=next(l for l in return_graph["links"] if l["toPin"]=="exec" and l["toNode"]=="return")
        self.call("graph_link_remove",{"graphId":g,"functionName":"Report","linkId":return_link["id"]})
        self.assertFalse(self.call("graph_diagnose",{"graphId":g})["valid"])
        with self.assertRaises(CyToolError):self.call("graph_run",{"graphId":g})
        self.call("edit_undo");self.assertTrue(self.call("graph_diagnose",{"graphId":g})["valid"])
        # A result wire cannot silently run an execution function before its exec input.
        self.call("graph_link_remove",{"graphId":g,"linkId":"begin"});self.call("graph_link_remove",{"graphId":g,"linkId":"print"})
        self.wire(g,"begin","then","print","exec")
        with self.assertRaises(CyToolError):self.call("graph_run",{"graphId":g})

    def test_25_debugger_nested_stack_step_modes_and_breakpoints(self):
        g=self.execution_example("StackDebug")
        def debug(suffix):return self.call("graph_debug_"+suffix,{"graphId":g})
        self.assertEqual(debug("start")["nextNode"],"begin")
        self.assertEqual(debug("step")["nextNode"],"compute")
        state=debug("step_into");self.assertEqual(state["activeFunction"],"Compute");self.assertEqual(len(state["callStack"]),2)
        self.assertEqual(state["callStack"][-1]["arguments"],{"x":5});self.assertEqual(state["variables"]["Counter"],100)
        self.assertEqual(debug("step")["nextNode"],"set")
        self.assertEqual(debug("step")["variables"]["Counter"],5)
        self.assertEqual(debug("step_into")["activeFunction"],"Report")
        state=debug("step_out");self.assertEqual(state["activeFunction"],"Compute");self.assertEqual(state["nextNode"],"return")
        state=debug("step_out");self.assertEqual(state["activeFunction"],"");self.assertEqual(state["nextNode"],"print")
        self.assertEqual(state["rootVariables"]["Counter"],900)
        self.assertEqual(debug("continue")["messages"],["Inside Report","6"])
        debug("start");debug("step")
        self.call("graph_breakpoint_set",{"graphId":g,"functionName":"Report","nodeId":"log","enabled":True})
        state=debug("step");self.assertEqual(state["state"],"Breakpoint");self.assertEqual(state["activeFunction"],"Report")
        self.assertEqual(len(state["callStack"]),3)
        self.assertEqual(debug("continue")["state"],"Completed")
        debug("start");debug("step");debug("step_into")
        self.call("graph_node_move",{"graphId":g,"functionName":"Compute","nodeId":"set","position":[290,60]})
        self.assertEqual(debug("status")["state"],"Paused")
        self.call("graph_node_value",{"graphId":g,"functionName":"Compute","nodeId":"add","key":"b","value":2})
        self.assertEqual(debug("status")["state"],"Stale")
        self.call("edit_undo");self.assertEqual(debug("status")["state"],"Paused")
        if self.gui:self.call("panel_open",{"panelId":"debugger"})

    def test_26_graph_diff_disk_function_changes_layout_filter_and_readonly(self):
        g=self.execution_example("DiffReview")
        before=self.call("editor_status")["revision"]
        self.assertFalse(self.call("graph_diff",{"graphId":g})["changed"])
        self.call("graph_node_move",{"graphId":g,"nodeId":"compute","position":[315,65]})
        self.assertTrue(self.call("graph_diff",{"graphId":g})["changed"])
        self.assertFalse(self.call("graph_diff",{"graphId":g,"ignorePresentation":True})["changed"])
        self.call("graph_node_value",{"graphId":g,"functionName":"Compute","nodeId":"add","key":"b","value":2})
        revision=self.call("editor_status")["revision"]
        diff=self.call("graph_diff",{"graphId":g,"ignorePresentation":True})
        self.assertEqual(len(diff["entries"]),1);entry=diff["entries"][0]
        self.assertEqual((entry["kind"],entry["key"],entry["functionName"],entry["change"]),("nodes","add","Compute","Changed"))
        self.assertEqual(entry["before"]["values"]["b"],1);self.assertEqual(entry["after"]["values"]["b"],2)
        self.assertEqual(self.call("editor_status")["revision"],revision)
        self.call("edit_undo");self.call("edit_undo")
        self.assertFalse(self.call("graph_diff",{"graphId":g})["changed"])
        path=self.workspace/"data/assets/graphs/DiffReview.cyasset";saved=path.read_text("utf-8");disk=json.loads(saved)
        disk["graph"]["nodes"][1]["values"]["x"]=42;path.write_text(json.dumps(disk),"utf-8")
        try:
            report=self.call("graph_diff",{"graphId":g})
            self.assertTrue(report["externalChanges"]);self.assertTrue(report["changed"])
            self.assertEqual(self.call("graph_get",{"graphId":g})["graph"]["nodes"][1]["values"]["x"],5)
            reordered=json.loads(saved);reordered["graph"]["nodes"].reverse();path.write_text(json.dumps(reordered),"utf-8")
            self.assertTrue(any(e["kind"]=="ordering" for e in self.call("graph_diff",{"graphId":g})["entries"]))
            path.write_text("{","utf-8")
            with self.assertRaises(CyToolError):self.call("graph_diff",{"graphId":g})
        finally:path.write_text(saved,"utf-8")
        self.call("graph_node_value",{"graphId":g,"nodeId":"compute","key":"x","value":7})
        if self.gui:self.call("graph_diff_view",{"graphId":g})
        self.call("asset_save_graph",{"graphId":g});self.assertFalse(self.call("graph_diff",{"graphId":g})["changed"])

    def test_27_execution_limits_void_returns_recursion_and_unicode_diff(self):
        template=json.loads((APP/"examples/Example_ExecutionFunctions.cyasset").read_text("utf-8"))
        graph=template["graph"]
        # A short call tree expands into more than 4,096 execution steps without a cycle.
        def node(id,type,values=None,**extra):return {"id":id,"type":type,"position":[0,0],"values":values or {},**extra}
        def wire(id,a,ap,b,bp):return {"id":id,"fromNode":a,"fromPin":ap,"toNode":b,"toPin":bp}
        graph["variables"]={};graph["functions"]={}
        for i in range(8):
            nodes=[node("arguments","function.input"),node("return","function.output")]
            links=[wire("return","arguments","then","return","exec")]
            if i<7:
                nodes += [node("sequence","flow.sequence")]+[node("call"+str(j),"function.call",function="F"+str(i+1)) for j in range(3)]
                links=[wire("sequence","arguments","then","sequence","exec"),wire("return","call2","then","return","exec")]+[wire("branch"+str(j),"sequence","then"+str(j),"call"+str(j),"exec") for j in range(3)]
            graph["functions"]["F"+str(i)]={"domain":"function","mode":"execution","inputs":{},"outputs":{},"variables":{},"nodes":nodes,"links":links}
        graph["nodes"]=[node("begin","event.begin"),node("call","function.call",function="F0")];graph["links"]=[wire("begin","begin","then","call","exec")]
        path=self.workspace/"data/assets/BoundedExecution.cyasset";path.write_text(json.dumps(template),"utf-8")
        g=self.call("asset_open",{"path":path.name})["graphId"]
        self.assertEqual(self.call("graph_function_evaluate",{"graphId":g,"name":"F7"})["values"],{})
        self.call("graph_debug_start",{"graphId":g});state=self.call("graph_debug_continue",{"graphId":g})
        self.assertEqual(state["state"],"Failed");self.assertIn("step limit",state["error"])
        self.assertLessEqual(len(state["trace"]),256);self.assertLessEqual(len(state["callStack"]),17)
        with self.assertRaises(CyToolError):self.call("graph_node_add",{"graphId":g,"functionName":"F7","type":"function.call","functionRef":"F0","position":[0,0]})
        g=self.new_graph("UnicodeDiff")
        self.call("graph_node_value",{"graphId":g,"nodeId":"print","key":"text","value":"é漢"*600})
        self.call("graph_function_create",{"graphId":g,"name":"Unicode"})
        fn=self.call("graph_get",{"graphId":g,"functionName":"Unicode"})["graph"]
        self.call("graph_link_remove",{"graphId":g,"functionName":"Unicode","linkId":fn["links"][0]["id"]})
        self.call("graph_function_signature",{"graphId":g,"name":"Unicode","inputs":{name:{"type":"string","default":"é漢"*600} for name in ("a","b","c")},"outputs":{"result":{"type":"float","default":0}}})
        report=self.call("graph_diff",{"graphId":g});self.assertTrue(report["changed"])
        self.assertTrue(next(e for e in report["entries"] if e["kind"]=="functions")["afterPreview"].endswith("..."))

    def test_28_function_keyboard_navigation_in_real_imgui(self):
        if not self.gui:self.skipTest("Graphical keyboard host required")
        root=self.workspace/"function-smoke";(root/"app").mkdir(parents=True);(root/"data/assets/Examples").mkdir(parents=True)
        shutil.copyfile(APP/"CyTool.json",root/"app/CyTool.json")
        shutil.copyfile(APP/"examples/Example_ExecutionFunctions.cyasset",root/"data/assets/Examples/Example_ExecutionFunctions.cyasset")
        report=ROOT/"data/reports/function-gesture.json"
        process=subprocess.run([str(ROOT/"CyGameEditor.exe"),"--root",str(root),"--workspace","Scripting","--asset","Examples/Example_ExecutionFunctions.cyasset","--test-functions",str(report),"--exit-after","15"],timeout=25)
        self.assertEqual(process.returncode,0,report.read_text("utf-8") if report.exists() else "No function gesture report")
        self.assertTrue(json.loads(report.read_text("utf-8"))["passed"])

    def uv_example(self,name):
        target=self.workspace/"data/assets/Examples";target.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(APP/"examples/Example_UVMaterial.cyasset",target/(name+".cyasset"))
        return self.call("asset_open",{"path":"Examples/"+name+".cyasset"})["graphId"]

    def singular_material(self,name):
        g=self.new_graph(name,"material");uv=self.node(g,"material.uv");subtract=self.node(g,"math.subtract");divide=self.node(g,"math.divide")
        self.call("graph_node_value",{"graphId":g,"nodeId":subtract,"key":"b","value":.5});self.call("graph_node_value",{"graphId":g,"nodeId":divide,"key":"a","value":1})
        self.wire(g,uv,"u",subtract,"a");self.wire(g,subtract,"value",divide,"b");self.wire(g,divide,"value","surface","roughness")
        return g

    def test_29_uv_sampling_nodes_functions_and_uniform_signatures(self):
        g=self.uv_example("UVSampling")
        sample=lambda u,v:self.call("material_sample",{"graphId":g,"u":u,"v":v})["baseColor"]
        self.assertEqual(sample(.01,.01),[.025,.08,.18]);self.assertEqual(sample(.15,.01),[.95,.42,.045])
        self.assertEqual(sample(-.01,.01),[.95,.42,.045])
        with self.assertRaises(CyToolError):sample(10001,0)
        initial=self.call("material_prepare",{"graphId":g})
        self.call("graph_variable_default",{"graphId":g,"name":"Tile","value":4})
        changed=self.call("material_prepare",{"graphId":g})
        self.assertEqual(initial["signature"],changed["signature"]);self.assertNotEqual(initial["parameters"],changed["parameters"])
        self.call("graph_node_move",{"graphId":g,"nodeId":"uv","position":[50,60]})
        self.assertEqual(self.call("material_prepare",{"graphId":g})["signature"],initial["signature"])
        self.call("graph_node_value",{"graphId":g,"nodeId":"surface","key":"metallic","value":.7})
        self.assertNotEqual(self.call("material_prepare",{"graphId":g})["signature"],initial["signature"])
        # Compose the UV source inside a pure function: the caller's UV context survives.
        self.call("graph_function_create",{"graphId":g,"name":"SampleU"})
        f=lambda op,**p:self.call(op,{"graphId":g,"functionName":"SampleU",**p})
        body=f("graph_get")["graph"];f("graph_link_remove",linkId=body["links"][0]["id"])
        uv=f("graph_node_add",type="material.uv",position=[100,100])["node"]["id"]
        f("graph_link_add",fromNode=uv,fromPin="u",toNode="return",toPin="result")
        call=self.node(g,"function.call",functionRef="SampleU")
        transform=self.node(g,"uv.transform");self.wire(g,call,"result",transform,"u")
        gradient=self.node(g,"pattern.gradient");self.wire(g,transform,"u",gradient,"u")
        link=next(l for l in self.call("graph_get",{"graphId":g})["graph"]["links"] if l["toPin"]=="baseColor")
        self.call("graph_link_remove",{"graphId":g,"linkId":link["id"]});self.wire(g,gradient,"value","surface","baseColor")
        self.assertEqual(sample(0,.7),[.05,.1,.2]);self.assertEqual(sample(1,.7),[.8,.5,.1])
        # Unary and compose paths are evaluated in the same interpreter.
        nodes=[self.node(g,t) for t in ("math.sine","math.fract","math.clamp","color.compose")]
        for n,value in zip(nodes[:3],(0,-.25,2)):
            self.call("graph_node_value",{"graphId":g,"nodeId":n,"key":"value","value":value})
        for n,pin in zip(nodes[:3],("r","g","b")):self.wire(g,n,"value",nodes[3],pin)
        l=next(l for l in self.call("graph_get",{"graphId":g})["graph"]["links"] if l["toPin"]=="baseColor")
        self.call("graph_link_remove",{"graphId":g,"linkId":l["id"]});self.wire(g,nodes[3],"value","surface","baseColor")
        self.assertEqual(sample(.7,.2),[0,.75,1])
        singular=self.singular_material("SingularSample")
        self.assertIn("sampleError",self.call("material_prepare",{"graphId":singular}))
        self.assertEqual(self.call("material_sample",{"graphId":singular,"u":.75,"v":.5})["roughness"],1)
        with self.assertRaises(CyToolError):self.call("material_sample",{"graphId":singular,"u":.5,"v":.5})

    def test_30_real_preview_compilation_reports_functions_and_privacy(self):
        g=self.uv_example("ShaderCompilation")
        target=self.workspace/"data/assets/Examples/ShaderFunction.cyasset";shutil.copyfile(APP/"examples/Example_FunctionMaterial.cyasset",target)
        fn=self.call("asset_open",{"path":"Examples/ShaderFunction.cyasset"})["graphId"]
        procedural=self.uv_example("ShaderProcedural")
        self.call("graph_function_create",{"graphId":procedural,"name":"Coordinate"})
        body=self.call("graph_get",{"graphId":procedural,"functionName":"Coordinate"})["graph"]
        self.call("graph_link_remove",{"graphId":procedural,"functionName":"Coordinate","linkId":body["links"][0]["id"]})
        source=self.call("graph_node_add",{"graphId":procedural,"functionName":"Coordinate","type":"material.uv","position":[100,100]})["node"]["id"]
        self.call("graph_link_add",{"graphId":procedural,"functionName":"Coordinate","fromNode":source,"fromPin":"u","toNode":"return","toPin":"result"})
        call=self.node(procedural,"function.call",functionRef="Coordinate");transform=self.node(procedural,"uv.transform")
        self.wire(procedural,call,"result",transform,"u");self.wire(procedural,"uv","v",transform,"v")
        for link in self.call("graph_get",{"graphId":procedural})["graph"]["links"]:
            if (link["toNode"]=="checker" and link["toPin"] in ("u","v")) or link["toPin"]=="baseColor":self.call("graph_link_remove",{"graphId":procedural,"linkId":link["id"]})
        self.wire(procedural,transform,"u","checker","u");self.wire(procedural,transform,"v","checker","v")
        compose=self.node(procedural,"color.compose");gradient=self.node(procedural,"pattern.gradient");multiply=self.node(procedural,"color.multiply")
        self.wire(procedural,transform,"u",compose,"r");self.wire(procedural,transform,"v",compose,"g")
        self.wire(procedural,transform,"u",gradient,"u");self.wire(procedural,"checker","value",gradient,"a");self.wire(procedural,compose,"value",gradient,"b")
        previous="uv";pin="v"
        for type in ("math.sine","math.fract","math.clamp"):
            n=self.node(procedural,type);self.wire(procedural,previous,pin,n,"value");previous=n;pin="value"
        self.wire(procedural,previous,pin,multiply,"factor");self.wire(procedural,gradient,"value",multiply,"color");self.wire(procedural,multiply,"value","surface","baseColor")
        with tool.create_runtime(self.workspace/"shader-jobs") as runtime:
            actor=runtime.authenticate(runtime.create_client("Shader author")["token"]);other=runtime.authenticate(runtime.create_client("Private")["token"])
            session=runtime.create_session(actor,temporary=False)
            for graph in (g,fn,procedural,self.singular_material("SingularShader")):
                job=runtime.submit(actor,session["sessionId"],"material_compile",{"graphId":graph})
                result=runtime.wait(actor,job["jobId"]);self.assertEqual(result["state"],"Completed",result)
                report=result["outputs"];self.assertTrue(report["success"])
                for key,hashkey in (("sourcePath","sourceSha256"),("shaderPath","bytecodeSha256")):
                    path=Path(report[key]);self.assertTrue(path.resolve().is_relative_to(Path(result["workspace"]).resolve()))
                    self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),report[hashkey])
                self.assertTrue(Path(report["shaderPath"]).read_bytes().startswith(b"DXBC"))
                self.assertIn("cy_params[",Path(report["sourcePath"]).read_text("utf-8"))
                if graph==fn:self.assertTrue(any(n["functionName"]=="TintScale" for n in report["nodeMap"]))
                self.assertEqual(runtime.list_jobs(other),[])
                with self.assertRaises(CyToolError):runtime.get_job(other,job["jobId"])
            revision=self.call("editor_status")["revision"]
            stale=runtime.submit(actor,session["sessionId"],"material_compile",{"graphId":g,"expectedRevision":revision-1})
            self.assertEqual(runtime.wait(actor,stale["jobId"])["state"],"Failed")
            # A valid material can contain a string-valued function parameter, unsupported by the GPU backend.
            self.call("graph_function_create",{"graphId":g,"name":"Unsupported"})
            body=self.call("graph_get",{"graphId":g,"functionName":"Unsupported"})["graph"]
            self.call("graph_link_remove",{"graphId":g,"functionName":"Unsupported","linkId":body["links"][0]["id"]})
            self.call("graph_function_signature",{"graphId":g,"name":"Unsupported","inputs":{"label":{"type":"string","default":"hello"}},"outputs":{"result":{"type":"float","default":.5}}})
            call=self.node(g,"function.call",functionRef="Unsupported");self.wire(g,call,"result","surface","metallic")
            job=runtime.submit(actor,session["sessionId"],"material_compile",{"graphId":g})
            result=runtime.wait(actor,job["jobId"]);self.assertEqual(result["state"],"Failed",result)
            self.assertEqual(result["error"]["code"],"ShaderCompileFailed")
            self.assertEqual(result["error"]["technicalDetails"]["diagnostics"][0]["nodeId"],call)

    def test_31_shader_worker_cancellation(self):
        from cytools_core.workers import ProcessWorker
        worker=ProcessWorker([sys.executable,str(APP/"tests/slow_build_worker.py")])
        g=self.uv_example("CancelShader")
        with tool.create_runtime(self.workspace/"cancel-shader-jobs",shader_worker=worker) as runtime:
            actor=runtime.authenticate(runtime.create_client("Cancel shader")["token"]);session=runtime.create_session(actor,temporary=False)
            job=runtime.submit(actor,session["sessionId"],"material_compile",{"graphId":g})
            marker=Path(job["workspace"])/"worker-started.txt";deadline=time.monotonic()+10
            while not marker.exists() and time.monotonic()<deadline:time.sleep(.02)
            self.assertTrue(marker.exists(),runtime.get_job(actor,job["jobId"]))
            runtime.cancel(actor,job["jobId"]);result=runtime.wait(actor,job["jobId"])
            self.assertEqual(result["state"],"Cancelled",result);self.assertIsNone(worker.process)
            self.assertFalse(worker.health()["alive"]);self.assertFalse(list(Path(result["workspace"]).rglob("*.dxbc")))

    def test_32_graphical_shader_application_live_parameters_and_stale_reports(self):
        if not self.gui:self.skipTest("Native DX11 renderer required")
        g=self.uv_example("GPUShader")
        self.call("panel_open",{"panelId":g});self.call("material_compiler_view",{"graphId":g})
        self.call("material_compile_request",{"graphId":g})
        deadline=time.monotonic()+25
        while time.monotonic()<deadline:
            state=self.call("material_compile_status",{"graphId":g})
            if state["state"] in ("Completed","Failed","Cancelled"):break
            time.sleep(.05)
        self.assertEqual(state["state"],"Completed",state);self.assertTrue(state["current"],state)
        revision=self.call("editor_status")["revision"]
        self.assertEqual(self.call("material_preview_settings",{"graphId":g,"shape":"Plane"})["shape"],"Plane")
        self.assertEqual(self.call("editor_status")["revision"],revision)
        report=state["job"]["outputs"];path=Path(report["reportPath"])
        self.call("graph_variable_default",{"graphId":g,"name":"Tile","value":12})
        self.assertTrue(self.call("material_compile_status",{"graphId":g})["current"])
        # Explicit SDK reports use the same native installer and verified bytecode.
        self.assertTrue(self.call("material_preview_apply",{"graphId":g,"reportPath":str(path)})["applied"])
        original=path.read_bytes();tampered=json.loads(original);tampered["bytecodeSha256"]="0"*64;path.write_text(json.dumps(tampered),"utf-8")
        try:
            with self.assertRaises(CyToolError):self.call("material_preview_apply",{"graphId":g,"reportPath":str(path)})
        finally:path.write_bytes(original)
        outside=Path(report["shaderPath"]).parent.parent/"outside.dxbc";outside.write_bytes(Path(report["shaderPath"]).read_bytes())
        tampered=json.loads(original);tampered["shaderPath"]=str(outside);path.write_text(json.dumps(tampered),"utf-8")
        try:
            with self.assertRaises(CyToolError):self.call("material_preview_apply",{"graphId":g,"reportPath":str(path)})
        finally:path.write_bytes(original);outside.unlink()
        self.call("graph_node_value",{"graphId":g,"nodeId":"surface","key":"metallic","value":.8})
        self.assertFalse(self.call("material_compile_status",{"graphId":g})["current"])
        with self.assertRaises(CyToolError):self.call("material_preview_apply",{"graphId":g,"reportPath":str(path)})
        self.call("edit_undo");self.assertTrue(self.call("material_compile_status",{"graphId":g})["current"])
        for key in ("sourcePath","shaderPath"):Path(report[key]).unlink()
        self.assertTrue(self.call("material_compile_status",{"graphId":g})["current"])
        with self.assertRaises(CyToolError):self.call("material_preview_settings",{"graphId":g,"shape":"Other"})

    def test_33_image_metadata_formats_limits_and_graphical_editor(self):
        import struct
        target=self.workspace/"data/assets/Images";target.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(APP/"examples/Example_Image.png",target/"épreuve.png")
        info=self.call("texture_info",{"path":"Images/épreuve.png"})
        self.assertEqual((info["format"],info["width"],info["height"],info["decodedBytes"]),("PNG",256,128,256*128*4))
        shutil.copyfile(APP/"tests/fixtures/tiny.jpg",target/"tiny.jpg")
        jpeg=self.call("texture_info",{"path":"Images/tiny.jpg"});self.assertEqual((jpeg["format"],jpeg["width"],jpeg["height"]),("JPEG",8,8))
        def bmp(width,height,body=b''):
            return b'BM'+struct.pack('<IHHI',54+len(body),0,0,54)+struct.pack('<IiiHHIIiiII',40,width,height,1,24,0,len(body),2835,2835,0,0)+body
        (target/"tiny.bmp").write_bytes(bmp(2,2,b'\xff\x20\x00'*2+b'\0\0'+b'\x00\xff\x00'*2+b'\0\0'))
        self.assertEqual(self.call("texture_info",{"path":"Images/tiny.bmp"})["format"],"BMP")
        self.assertIn("Images/tiny.bmp",{a["path"] for a in self.call("assets_list",{"type":"Texture"})["assets"]})
        (target/"too-wide.bmp").write_bytes(bmp(8193,1));(target/"too-many.bmp").write_bytes(bmp(4096,4096));(target/"broken.png").write_bytes(b'\x89PNG\r\n\x1a\n')
        for path in ("Images/too-wide.bmp","Images/too-many.bmp","Images/broken.png","../ipc/editor.json",str(APP/"examples/Example_Image.png")):
            with self.assertRaises(CyToolError):self.call("texture_info",{"path":path})
        if self.gui:
            revision=self.call("editor_status")["revision"]
            opened=self.call("texture_open",{"path":"Images/épreuve.png"});self.assertTrue(opened["panelId"].startswith("texture:"))
            for channel in ("RGBA","RGB","R","G","B","A"):
                view=self.call("texture_view_settings",{"path":"Images/épreuve.png","channel":channel,"fit":False,"zoom":2})
                self.assertEqual(view["channel"],channel)
            self.assertEqual(self.call("editor_status")["revision"],revision)
            with self.assertRaises(CyToolError):self.call("texture_view_settings",{"path":"Images/épreuve.png","zoom":0})

    def import_source(self,name='image.png',color=40):
        import struct,zlib
        folder=self.workspace/"sources";folder.mkdir(exist_ok=True);source=folder/name
        def chunk(type,data):return struct.pack('>I',len(data))+type+data+struct.pack('>I',zlib.crc32(type+data))
        raw=b''.join(b'\0'+bytes([color,120,200,255])*4 for _ in range(3))
        source.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',4,3,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''));return source

    def import_job(self,source,destination,runtime=None,actor=None,session=None):
        if runtime is None:
            with tool.create_runtime(self.workspace/(destination.replace('/','_')+'-jobs')) as rt:
                who=rt.authenticate(rt.create_client('Importer')['token']);scope=rt.create_session(who,temporary=False)
                return self.import_job(source,destination,rt,who,scope)
        job=runtime.submit(actor,session['sessionId'],'asset_import',{'sourcePath':str(source),'destination':destination})
        result=runtime.wait(actor,job['jobId']);self.assertEqual(result['state'],'Completed',result);return result

    def test_34_import_provider_sdk_persistence_filters_and_privacy(self):
        self.assertEqual(self.call('asset_importers')['importers'][0]['id'],'image.copy')
        source=self.import_source('épreuve.PnG');destination='Imported/épreuve.PnG';revision=self.call('editor_status')['revision']
        with tool.create_runtime(self.workspace/'import-jobs') as runtime:
            actor=runtime.authenticate(runtime.create_client('Importer')['token']);other=runtime.authenticate(runtime.create_client('Other')['token']);session=runtime.create_session(actor,temporary=False)
            result=self.import_job(source,destination,runtime,actor,session)
            self.assertEqual(runtime.list_jobs(other),[])
            with self.assertRaises(CyToolError):runtime.get_job(other,result['jobId'])
        asset=self.workspace/'data/assets'/destination
        self.assertEqual(asset.read_bytes(),source.read_bytes());self.assertTrue(Path(str(asset)+'.cyimport').exists())
        info=self.call('asset_import_info',{'path':destination});self.assertTrue(info['imported'])
        self.assertEqual(info['metadata']['importedSha256'],hashlib.sha256(source.read_bytes()).hexdigest())
        self.assertEqual(self.call('asset_import_check',{'path':destination})['state'],'UpToDate')
        results=self.call('assets_list',{'query':'imported:true'})['assets'];self.assertIn(destination,{a['path'] for a in results});self.assertTrue(all(a['imported'] for a in results))
        self.assertFalse(any(a['path'].endswith('.cyimport') for a in self.call('assets_list')['assets']))
        self.assertEqual(self.call('editor_status')['revision'],revision)
        for params in [{'sourcePath':str(source),'destination':destination},{'sourcePath':str(source),'destination':'../bad.png'},{'sourcePath':str(source),'destination':'Imported/bad.jpg'},{'sourcePath':str(source),'destination':'Imported/missing.png','importerId':'missing'},{'sourcePath':str(asset),'destination':'Imported/inside.PnG'}]:
            with self.assertRaises(CyToolError):self.call('asset_import_prepare',params)
        with self.assertRaises(CyToolError):self.call('asset_copy',{'path':destination+'.cyimport','destination':'metadata.json'})

    def test_35_reimport_source_tracking_local_edit_guard_and_lifecycle(self):
        source=self.import_source('tracking.png',20);path='Imported/Tracking.png';self.import_job(source,path);asset=self.workspace/'data/assets'/path;original=asset.read_bytes()
        self.import_source('tracking.png',220)
        self.assertEqual(self.call('asset_import_check',{'path':path})['state'],'SourceChanged')
        with tool.create_runtime(self.workspace/'reimport-jobs') as runtime:
            actor=runtime.authenticate(runtime.create_client('Reimport')['token']);session=runtime.create_session(actor,temporary=False)
            job=runtime.submit(actor,session['sessionId'],'asset_reimport',{'path':path});result=runtime.wait(actor,job['jobId']);self.assertEqual(result['state'],'Completed',result)
        self.assertNotEqual(asset.read_bytes(),original);self.assertEqual(asset.read_bytes(),source.read_bytes())
        committed=asset.read_bytes();asset.write_bytes(original)
        self.assertEqual(self.call('asset_import_check',{'path':path})['state'],'LocalChanges')
        with self.assertRaises(CyToolError):self.call('asset_import_prepare',{'path':path,'reimport':True})
        self.assertEqual(asset.read_bytes(),original);asset.write_bytes(committed)
        hidden=source.with_suffix('.hidden');source.rename(hidden)
        try:self.assertEqual(self.call('asset_import_check',{'path':path})['state'],'SourceMissing')
        finally:hidden.rename(source)
        copied=self.call('asset_copy',{'path':path,'destination':'Imported/Copy.png'})['path'];self.assertTrue(self.call('asset_import_info',{'path':copied})['imported'])
        moved=self.call('asset_move',{'path':copied,'destination':'Imported/Moved.png'})['path'];self.assertFalse((self.workspace/'data/assets/Imported/Copy.png.cyimport').exists())
        self.assertEqual(self.call('asset_import_check',{'path':moved})['state'],'UpToDate')
        trash=self.call('asset_trash',{'path':moved});self.assertFalse((self.workspace/'data/assets'/moved).exists());self.assertFalse((self.workspace/'data/assets'/(moved+'.cyimport')).exists())
        self.call('asset_restore',{'ticket':trash['ticket']});self.assertTrue(self.call('asset_import_info',{'path':moved})['imported'])

    def test_36_import_commit_rejects_stale_corrupt_and_escaping_reports(self):
        import import_worker
        source=self.import_source('staged.png');destination='Imported/Staged.png';plan=self.call('asset_import_prepare',{'sourcePath':str(source),'destination':destination})
        stage=self.workspace/'staged-job';stage.mkdir();input=stage/'plan.json';input.write_text(json.dumps(plan),'utf-8')
        result=import_worker.run({'protocolVersion':'1.0','workspace':str(stage),'parameters':{'planPath':str(input),'stagedPath':str(stage/'staged.png'),'reportPath':str(stage/'report.json')}})
        report=Path(result['reportPath']);saved=report.read_bytes();record=json.loads(saved);asset=self.workspace/'data/assets'/destination;asset.parent.mkdir(exist_ok=True)
        asset.write_bytes(b'collision')
        with self.assertRaises(CyToolError):self.call('asset_import_commit',{'reportPath':str(report)})
        self.assertEqual(asset.read_bytes(),b'collision');asset.unlink()
        record['importedSha256']='0'*64;report.write_text(json.dumps(record),'utf-8')
        with self.assertRaises(CyToolError):self.call('asset_import_commit',{'reportPath':str(report)})
        record=json.loads(saved);record['stagedPath']=str(source);report.write_text(json.dumps(record),'utf-8')
        with self.assertRaises(CyToolError):self.call('asset_import_commit',{'reportPath':str(report)})
        self.assertFalse(asset.exists());self.assertFalse(Path(str(asset)+'.cyimport').exists())
        report.write_bytes(saved);self.assertTrue(self.call('asset_import_commit',{'reportPath':str(report)})['imported'])
        with self.assertRaises(CyToolError):self.call('asset_import_commit',{'reportPath':str(report)})

    def test_37_import_cancellation_keeps_persistent_destination_absent(self):
        from import_service import ImportProvider
        provider=ImportProvider('image.copy',[sys.executable,str(APP/'tests/slow_build_worker.py')]);source=self.import_source('cancel.png')
        with tool.create_runtime(self.workspace/'cancel-import',import_providers={provider.id:provider}) as runtime:
            actor=runtime.authenticate(runtime.create_client('Cancel import')['token']);session=runtime.create_session(actor,temporary=False)
            job=runtime.submit(actor,session['sessionId'],'asset_import',{'sourcePath':str(source),'destination':'Imported/Cancelled.png'})
            marker=Path(job['workspace'])/'worker-started.txt';deadline=time.monotonic()+10
            while not marker.exists() and time.monotonic()<deadline:time.sleep(.02)
            self.assertTrue(marker.exists());runtime.cancel(actor,job['jobId']);self.assertEqual(runtime.wait(actor,job['jobId'])['state'],'Cancelled')
            self.assertIsNone(provider.worker.process);self.assertFalse((self.workspace/'data/assets/Imported/Cancelled.png').exists())
            self.assertFalse((self.workspace/'data/assets/Imported/Cancelled.png.cyimport').exists())

    def test_38_graphical_import_job_and_reimport_updates_image_view(self):
        if not self.gui:self.skipTest('Native graphical import client required')
        source=self.import_source('gui.png',60);path='Imported/GUI.png';self.call('asset_import_view');self.call('asset_import_request',{'sourcePath':str(source),'destination':path})
        def wait():
            deadline=time.monotonic()+30
            while time.monotonic()<deadline:
                state=self.call('asset_import_status')
                if state['state'] in ('Completed','Failed','Cancelled'):return state
                time.sleep(.05)
            self.fail('Desktop import did not finish')
        result=wait();self.assertEqual(result['state'],'Completed',result)
        self.call('texture_open',{'path':path});self.call('asset_import_view',{'path':path});self.import_source('gui.png',200)
        self.call('asset_import_request',{'path':path,'reimport':True});reimported=wait();self.assertEqual(reimported['state'],'Completed',reimported)
        self.assertEqual((self.workspace/'data/assets'/path).read_bytes(),source.read_bytes())
        self.assertEqual(self.call('asset_import_check',{'path':path})['state'],'UpToDate')

    def test_39_windows_file_locks_and_metadata_failure_rollback(self):
        source=self.import_source('locked.png',20);path='Imported/Locked.png';self.import_job(source,path);asset=self.workspace/'data/assets'/path;meta=Path(str(asset)+'.cyimport');old=asset.read_bytes();oldmeta=meta.read_bytes();self.import_source('locked.png',180)
        kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p];kernel.CreateFileW.restype=ctypes.c_void_p;kernel.CloseHandle.argtypes=[ctypes.c_void_p]
        with tool.create_runtime(self.workspace/'rollback-jobs') as runtime:
            actor=runtime.authenticate(runtime.create_client('Rollback')['token']);session=runtime.create_session(actor,temporary=False)
            def run():
                job=runtime.submit(actor,session['sessionId'],'asset_reimport',{'path':path});return runtime.wait(actor,job['jobId'])
            handle=kernel.CreateFileW(str(asset),0x80000000,3,None,3,0,None);self.assertNotEqual(handle,ctypes.c_void_p(-1).value)
            try:
                result=run();self.assertEqual(result['state'],'Failed',result);self.assertEqual(result['error']['code'],'EditorCommandFailed')
                self.assertEqual(asset.read_bytes(),old);self.assertEqual(meta.read_bytes(),oldmeta);self.assertEqual(self.call('editor_status')['adapter']['version'],'0.28.0')
            finally:kernel.CloseHandle(handle)
            temporary=Path(str(meta)+'.tmp');temporary.mkdir()
            try:
                result=run();self.assertEqual(result['state'],'Failed',result);self.assertEqual(asset.read_bytes(),old);self.assertEqual(meta.read_bytes(),oldmeta)
            finally:
                if temporary.exists():temporary.rmdir()
            result=run();self.assertEqual(result['state'],'Completed',result);self.assertEqual(asset.read_bytes(),source.read_bytes())
        self.assertFalse(list((self.workspace/'data/import-recovery').glob('*/transaction.json')))

if __name__=="__main__":unittest.main(verbosity=2)
