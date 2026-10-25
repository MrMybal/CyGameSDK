"""Build the one operation catalogue consumed by CyToolsCore and the native editor."""
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parent
def obj(props=None, required=()):
    return {"type":"object","properties":props or {},"required":list(required),"additionalProperties":False}
string = {"type":"string","minLength":1,"maxLength":128}
entity = {"entityId":string}
graph = {"graphId":{"type":"string","minLength":1,"maxLength":512}}
position = {"type":"array","minItems":2,"maxItems":2,"items":{"type":"number","minimum":-100000,"maximum":100000}}
operations=[]
def add(id,name,props=None,required=(),undo=False,output=None,description=""):
    operations.append({"id":id,"name":name,"description":description or name+". Operates on the running editor.",
        "inputSchema":obj(props,required),"outputSchema":output or {"type":"object"},"supportedBackends":[],
        "resourceRequirements":{"ramBytes":16*1024**2},"permissions":[],"canRunAsync":True,"canCancel":True,
        "supportsProgress":False,"exclusiveGroups":["editor"],"x-editor-undoable":undo})
add("editor_status","Editor status")
add("scene_list","List scene objects")
add("viewport_new","New viewport")
add("window_reset_layout","Reset layout")
add("editor_close","Exit",description="Request editor close. The native UI asks to save or discard unsaved work; this does not force termination.")
add("scene_select","Select object",entity,["entityId"])
add("scene_create","Create cube",{"name":string},["name"],True)
add("scene_delete","Delete object",entity,["entityId"],True)
add("property_set","Set property",{**entity,"propertyId":string,"value":{}},["entityId","propertyId","value"],True)
add("edit_undo","Undo")
add("edit_redo","Redo")
add("project_save","Save project",description="Explicitly save the live editor project to data/projects/Sandbox.cyproject.json. Persistent user project, not a temporary job artifact.")
add("project_load","Load project",undo=True,description="Load the saved Sandbox project into the editor. Current changes remain recoverable through Undo.")
add("scene_validate","Validate scene")
add("viewport_focus","Focus object",entity,["entityId"])
add("editor_search","Search editor",{"query":{"type":"string","maxLength":256}},["query"])
add("panel_open","Open panel",{"panelId":string,"newInstance":{"type":"boolean","default":False}},["panelId"])
add("plugin_set_enabled","Enable editor plugin",{"pluginId":string,"enabled":{"type":"boolean"}},["pluginId","enabled"])
add("assets_list","List assets",{"query":{"type":"string","default":"","maxLength":256},"type":{"type":"string","enum":["All","Graph","Texture","Mesh","Audio","Other"],"default":"All"}},
    output=obj({"assets":{"type":"array","items":obj({"path":{"type":"string"},"type":{"type":"string"},"bytes":{"type":"integer"}},["path","type","bytes"])}},["assets"]))
add("asset_save_graph","Save graph asset",graph,["graphId"],description="Explicitly save a graph in data/assets/graphs. Persistent project asset.")
add("asset_new_graph","Create graph asset",{"name":string,"domain":{"type":"string","enum":["logic","material"]}},["name","domain"])
add("asset_open","Open asset",{"path":{"type":"string","minLength":1,"maxLength":1024}},["path"],True)
for id,name in [("graph_get","Describe graph"),("graph_validate","Validate graph"),("graph_run","Run sandbox graph")]:
    add(id,name,graph,["graphId"])
add("graph_node_add","Add graph node",{**graph,"type":string,"position":position},["graphId","type","position"],True)
add("graph_node_move","Move graph node",{**graph,"nodeId":string,"position":position},["graphId","nodeId","position"],True)
add("graph_node_remove","Remove graph node",{**graph,"nodeId":string},["graphId","nodeId"],True)
add("graph_node_value","Set node value",{**graph,"nodeId":string,"key":string,"value":{}},["graphId","nodeId","key","value"],True)
add("graph_link_add","Connect graph pins",{**graph,**{k:string for k in ["fromNode","fromPin","toNode","toPin"]}},["graphId","fromNode","fromPin","toNode","toPin"],True)
add("graph_link_remove","Remove graph connection",{**graph,"linkId":string},["graphId","linkId"],True)
# Output contracts describe business results; Core wraps these in asynchronous jobs.
str_value={"type":"string"}
entity_value=obj({"id":str_value,"type":str_value,"properties":{"type":"object"}},["id","type","properties"])
outputs={
 "editor_status":obj({"adapter":{"type":"object"},"selected":str_value,"dirty":{"type":"boolean"},"canUndo":{"type":"boolean"},"canRedo":{"type":"boolean"},"commands":{"type":"array","items":{"type":"object"}},"panels":{"type":"array","items":obj({"id":str_value,"title":str_value,"owner":str_value,"open":{"type":"boolean"}},["id","title","owner","open"])}},["adapter","selected","dirty","canUndo","canRedo","commands"]),
 "scene_list":obj({"entities":{"type":"array","items":entity_value},"selected":str_value,"propertySchema":{"type":"object"}},["entities","selected","propertySchema"]),
 "viewport_new":obj({"panelId":str_value},["panelId"]),
 "window_reset_layout":obj({"reset":{"type":"boolean"}},["reset"]),
 "editor_close":obj({"requested":{"type":"boolean"}},["requested"]),
 "scene_select":obj({"selected":str_value},["selected"]),
 "scene_create":obj({"entity":entity_value},["entity"]),
 "property_set":obj({"entity":entity_value},["entity"]),
 "scene_delete":obj({"deleted":str_value},["deleted"]),
 "edit_undo":obj({"applied":{"type":"boolean"}},["applied"]),
 "edit_redo":obj({"applied":{"type":"boolean"}},["applied"]),
 "project_save":obj({"path":str_value},["path"]),
 "project_load":obj({"loaded":{"type":"boolean"}},["loaded"]),
 "scene_validate":obj({"valid":{"type":"boolean"},"entityCount":{"type":"integer"},"adapterId":str_value},["valid","entityCount","adapterId"]),
 "viewport_focus":obj({"focused":str_value},["focused"]),
 "editor_search":obj({"results":{"type":"array","items":obj({"kind":str_value,"id":str_value,"label":str_value},["kind","id","label"])}},["results"]),
 "panel_open":obj({"panelId":str_value},["panelId"]),
 "plugin_set_enabled":obj({"pluginId":str_value,"enabled":{"type":"boolean"}},["pluginId","enabled"]),
 "graph_get":obj({"graph":{"type":"object"},"nodeTypes":{"type":"object"}},["graph","nodeTypes"]),
 "graph_validate":obj({"valid":{"type":"boolean"},"nodeCount":{"type":"integer"},"linkCount":{"type":"integer"}},["valid","nodeCount","linkCount"]),
 "graph_run":{"oneOf":[obj({"messages":{"type":"array","items":str_value},"executed":{"type":"boolean"}},["messages","executed"]),obj({"baseColor":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number","minimum":0,"maximum":1}},"roughness":{"type":"number","minimum":0,"maximum":1},"metallic":{"type":"number","minimum":0,"maximum":1}},["baseColor","roughness","metallic"])]},
 "graph_node_add":obj({"node":{"type":"object"}},["node"]),
 "graph_node_value":obj({"node":{"type":"object"}},["node"]),
 "graph_node_move":obj({"moved":{"type":"boolean"}},["moved"]),
 "graph_node_remove":obj({"removed":str_value},["removed"]),
 "graph_link_add":obj({"link":{"type":"object"}},["link"]),
 "graph_link_remove":obj({"removed":str_value},["removed"]),
 "asset_save_graph":obj({"path":str_value},["path"]),
 "asset_new_graph":obj({"graphId":str_value,"path":str_value},["graphId","path"]),
 "asset_open":obj({"graphId":str_value,"path":str_value,"extension":str_value,"bytes":{"type":"integer"},"editor":str_value,"modelId":str_value,"jsonId":str_value},["path"])
}
for operation in operations:
    if operation["id"] in outputs:operation["outputSchema"]=outputs[operation["id"]]
from milestone_contracts import extend
extend(operations,obj)
manifest=json.loads((ROOT/"CyTool.json").read_text("utf-8"))
manifest.update(description="Modular game editor foundation. CyUI native desktop and CyToolsCore MCP/SDK.",version="0.28.0",
    categories=["development"],platforms=[{"os":"Windows","architecture":"x64","status":"Experimental","computeBackends":["CPU"]}],
    executable="runtime/python/Scripts/python.exe app/tool.py",standaloneExecutable="CyGameEditor.exe",
    distributionStatus="Development foundation; Windows DX11; source plugins; Sandbox adapter only",operations=operations)
(ROOT/"CyTool.json").write_text(json.dumps(manifest,indent=2)+"\n","utf-8")
