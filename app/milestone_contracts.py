"""Versioned milestone operation contracts appended to the common catalogue."""
def extend(operations,obj):
    text={"type":"string","minLength":1,"maxLength":1024}
    def add(id,name,props=None,required=(),output=None,undo=False):
        props=dict(props or {})
        if undo:props["expectedRevision"]={"type":"integer","minimum":0}
        operations.append({"id":id,"name":name,"description":name+". Uses the running editor and its current content root.",
            "inputSchema":obj(props,required),"outputSchema":output or {"type":"object"},"supportedBackends":[],
            "resourceRequirements":{"ramBytes":16777216},"permissions":[],"canRunAsync":True,"canCancel":True,
            "supportsProgress":False,"exclusiveGroups":["editor"],"x-editor-undoable":undo})
    for op in operations:
        if op["id"]=="asset_new_graph":op["inputSchema"]["properties"]["folder"]=text
        if op["id"]=="assets_list":
            op["outputSchema"]["properties"]["assets"]["items"]["properties"].update(domain={"type":"string"},extension={"type":"string"})
            op["outputSchema"]["properties"].update(folders={"type":"array","items":{"type":"string"}},truncated={"type":"boolean"})
    for suffix,name in [("copy","Duplicate asset"),("move","Move or rename asset")]:
        add("asset_"+suffix,name,{"path":text,"destination":text},["path","destination"],obj({"path":text},["path"]))
    add("asset_trash","Move asset to Trash",{"path":text},["path"],obj({"ticket":text,"path":text},["ticket","path"]))
    add("asset_restore","Restore asset",{"ticket":text},["ticket"],obj({"path":text},["path"]))
    add("asset_trash_list","List recoverable assets",output=obj({"tickets":{"type":"array","items":obj({"ticket":text,"path":text},["ticket","path"])}},["tickets"]))
    add("asset_folder_create","Create content folder",{"path":text},["path"],obj({"path":text},["path"]))
    add("asset_reload","Reload asset",{"path":text,"discardChanges":{"type":"boolean","default":False}},["path"],undo=True)
    add("asset_favorite","Favorite asset",{"path":text,"enabled":{"type":"boolean"}},["path","enabled"],obj({"path":text,"enabled":{"type":"boolean"}},["path","enabled"]))
    add("workspace_layout","Switch workspace",{"preset":{"type":"string","enum":["Level","Scripting","Materials"]}},["preset"],obj({"preset":text},["preset"]))
    for suffix in ["save","load"]:add("workspace_"+suffix,suffix.title()+" workspace layout",{"name":{"type":"string","minLength":1,"maxLength":64}},["name"],obj({"name":text},["name"]))
    add("content_drawer_toggle","Toggle Content Drawer",output=obj({"open":{"type":"boolean"}},["open"]))

    ids={"type":"array","maxItems":2000,"uniqueItems":True,"items":{"type":"string","minLength":1,"maxLength":128}}
    vec={"type":"array","minItems":3,"maxItems":3,"items":{"type":"number","minimum":-10000,"maximum":10000}}
    transform=obj({"entityId":{"type":"string","minLength":1,"maxLength":128},"position":vec,"rotation":vec,"scale":vec},["entityId"])
    for op in operations:
        if op["x-editor-undoable"]:op["inputSchema"]["properties"]["expectedRevision"]={"type":"integer","minimum":0}
        if op["id"]=="scene_select":op["inputSchema"]["properties"]["mode"]={"type":"string","enum":["replace","add","toggle"],"default":"replace"}
        if op["id"]=="scene_list":op["outputSchema"]["properties"].update(selection=ids,revision={"type":"integer"})
        if op["id"]=="editor_status":op["outputSchema"]["properties"]["revision"]={"type":"integer"}
    def entity_metadata(schema):
        if not isinstance(schema,dict):return
        if schema.get("type")=="object" and set(schema.get("properties",{}))=={"id","type","properties"}:
            schema["properties"].update(folder={"type":"string"},editorLocked={"type":"boolean"})
        for value in list(schema.values()):
            if isinstance(value,dict):entity_metadata(value)
            if isinstance(value,list):
                for item in value:entity_metadata(item)
    for op in operations:entity_metadata(op["outputSchema"])
    add("scene_select_many","Select objects",{"entityIds":ids},["entityIds"],obj({"selection":ids},["selection"]))
    add("scene_delete_many","Delete selected objects",{"entityIds":ids},output=obj({"deletedCount":{"type":"integer"}},["deletedCount"]),undo=True)
    add("scene_duplicate","Duplicate selected objects",{"entityIds":ids,"offset":vec},output=obj({"entities":{"type":"array","items":{"type":"object"}}},["entities"]),undo=True)
    add("scene_folder_set","Move objects to outliner folder",{"entityIds":ids,"folder":{"type":"string","maxLength":256}},["folder"],obj({"updatedCount":{"type":"integer"}},["updatedCount"]),True)
    add("scene_lock","Lock objects",{"entityIds":ids,"locked":{"type":"boolean"}},["locked"],obj({"updatedCount":{"type":"integer"}},["updatedCount"]),True)
    add("scene_transform","Set object transform",{**transform["properties"],"expectedRevision":{"type":"integer","minimum":0}},["entityId"],obj({"entity":{"type":"object"}},["entity"]),True)
    add("scene_transform_batch","Transform objects atomically",{"transforms":{"type":"array","minItems":1,"maxItems":2000,"items":transform},"expectedRevision":{"type":"integer","minimum":0}},["transforms"],obj({"updatedCount":{"type":"integer"}},["updatedCount"]),True)
    add("edit_history","Inspect edit history",output=obj({"undo":{"type":"array","items":{"type":"string"}},"redo":{"type":"array","items":{"type":"string"}}},["undo","redo"]))

    g={"graphId":{"type":"string","minLength":1,"maxLength":512}}
    varname={"type":"string","minLength":1,"maxLength":64,"pattern":"^[A-Za-z_][A-Za-z0-9_]*$"}
    for op in operations:
        if op["id"]=="graph_node_add":op["inputSchema"]["properties"]["variableName"]=varname
        if op["id"]=="graph_run":
            op["outputSchema"]["oneOf"][0]["properties"].update(trace={"type":"array","items":{"type":"object"}},variables={"type":"object"})
    add("graph_variable_add","Add graph variable",{**g,"name":varname,"type":{"type":"string","enum":["float","bool","string","color"]},"default":{}},["graphId","name","type","default"],undo=True)
    add("graph_variable_default","Set variable default",{**g,"name":varname,"value":{}},["graphId","name","value"],undo=True)
    add("graph_variable_remove","Remove graph variable",{**g,"name":varname},["graphId","name"],undo=True)
    add("graph_variable_rename","Rename graph variable",{**g,"name":varname,"newName":varname},["graphId","name","newName"],undo=True)
    add("graph_breakpoint_set","Set graph breakpoint",{**g,"nodeId":text,"enabled":{"type":"boolean"}},["graphId","nodeId","enabled"],undo=True)
    add("graph_node_comment","Comment graph node",{**g,"nodeId":text,"text":{"type":"string","maxLength":2048}},["graphId","nodeId","text"],undo=True)
    add("graph_nodes_duplicate","Duplicate graph nodes",{**g,"nodeIds":{"type":"array","minItems":1,"maxItems":500,"uniqueItems":True,"items":text},"offset":{"type":"array","minItems":2,"maxItems":2,"items":{"type":"number","minimum":-100000,"maximum":100000}}},["graphId","nodeIds"],undo=True)
    add("graph_diagnose","Check graph",{**g},["graphId"])
    for action in ["start","step","continue","stop","status"]:add("graph_debug_"+action,action.title()+" graph debugging",g,["graphId"])

    # Milestones 4-6: reusable pure functions, graph authoring and references.
    point={"type":"array","minItems":2,"maxItems":2,"items":{"type":"number","minimum":-100000,"maximum":100000}}
    nodes={"type":"array","minItems":1,"maxItems":500,"uniqueItems":True,"items":text}
    for op in operations:
        if op["id"].startswith("graph_") and not op["id"].startswith("graph_debug_") and op["id"]!="graph_run":op["inputSchema"]["properties"]["functionName"]=varname
        if op["id"]=="graph_node_add":op["inputSchema"]["properties"]["functionRef"]=varname
        if op["id"]=="graph_get":op["outputSchema"]["properties"]["nodeDefinitions"]={"type":"object"}
    for suffix,name,extra,required in [
        ("create","Create function",{},[]),("signature","Edit function signature",{"inputs":{"type":"object"},"outputs":{"type":"object"}},["inputs","outputs"]),
        ("rename","Rename function",{"newName":varname},["newName"]),("remove","Remove function",{},[])]:
        add("graph_function_"+suffix,name,{**g,"name":varname,**extra},["graphId","name",*required],undo=True)
    add("graph_function_evaluate","Evaluate function",{**g,"name":varname,"arguments":{"type":"object"}},["graphId","name"])
    add("graph_nodes_move","Move graph nodes",{**g,"functionName":varname,"positions":{"type":"array","minItems":1,"maxItems":500,"items":obj({"nodeId":text,"position":point},["nodeId","position"])}},["graphId","positions"],undo=True)
    add("graph_nodes_remove","Remove graph nodes",{**g,"functionName":varname,"nodeIds":nodes},["graphId","nodeIds"],undo=True)
    add("graph_fragment_copy","Copy graph selection",{**g,"functionName":varname,"nodeIds":nodes},["graphId","nodeIds"])
    add("graph_fragment_paste","Paste graph selection",{**g,"functionName":varname,"fragment":{"type":"object"},"offset":point},["graphId","fragment"],undo=True)
    add("graph_comment_add","Create comment group",{**g,"functionName":varname,"text":{"type":"string","maxLength":2048},"position":point,"size":{"type":"array","minItems":2,"maxItems":2,"items":{"type":"number","minimum":40,"maximum":10000}}},["graphId","text","position","size"],undo=True)
    add("graph_comment_remove","Remove comment group",{**g,"functionName":varname,"commentId":text},["graphId","commentId"],undo=True)

    add("asset_dependencies","Inspect asset references",{"path":text},["path"])
    add("asset_references_set","Edit asset references",{**g,"paths":{"type":"array","maxItems":256,"uniqueItems":True,"items":text}},["graphId","paths"],undo=True)

    add("asset_reference_view","Open Reference Viewer",{"path":text},["path"])

    from next_contracts import extend_next
    extend_next(operations,obj)
