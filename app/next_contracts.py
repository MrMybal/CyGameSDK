"""Milestones 7-12 contracts, appended to the single editor operation catalogue."""
def extend_next(operations,obj):
    text={"type":"string","minLength":1,"maxLength":1024}
    name={"type":"string","minLength":1,"maxLength":128}
    g={"graphId":text};ctx={**g,"functionName":{"type":"string","minLength":1,"maxLength":64}}
    point={"type":"array","minItems":2,"maxItems":2,"items":{"type":"number","minimum":-100000,"maximum":100000}}
    def add(id,title,props,required=(),undo=False):
        props=dict(props)
        if undo:props['expectedRevision']={"type":"integer","minimum":0}
        operations.append({"id":id,"name":title,"description":title+". Uses the running editor and its current project.","inputSchema":obj(props,required),"outputSchema":{"type":"object"},"supportedBackends":[],"resourceRequirements":{"ramBytes":16777216},"permissions":[],"canRunAsync":True,"canCancel":True,"supportsProgress":False,"exclusiveGroups":["editor"],"x-editor-undoable":undo})
    add('graph_find','Find graph nodes',{'graphId':text,'query':{'type':'string','maxLength':256}},['query'])
    add('graph_link_reroute','Insert reroute',{**ctx,'linkId':text,'position':point},['graphId','linkId','position'],True)
    add('graph_bookmark_add','Bookmark graph view',{**ctx,'name':name,'pan':point,'zoom':{'type':'number','minimum':.4,'maximum':1.8}},['graphId','name','pan','zoom'],True)
    add('graph_bookmark_remove','Remove graph bookmark',{**ctx,'bookmarkId':text},['graphId','bookmarkId'],True)
    add('graph_comment_edit','Edit comment group',{**ctx,'commentId':text,'text':{'type':'string','maxLength':2048},'position':point,'size':{'type':'array','minItems':2,'maxItems':2,'items':{'type':'number','minimum':40,'maximum':10000}}},['graphId','commentId'],True)
    add('asset_new_material_instance','Create material instance',{'name':{'type':'string','minLength':1,'maxLength':128},'folder':text,'parent':text},['name','parent'],True)
    add('material_get','Inspect material parameters',g,['graphId'])
    add('material_override_set','Override material parameter',{**g,'name':name,'value':{}},['graphId','name','value'],True)
    add('material_override_remove','Inherit material parameter',{**g,'name':name},['graphId','name'],True)
    add('material_parent_set','Change material parent',{**g,'parent':text},['graphId','parent'],True)

    build={'targetId':text,'configuration':name,'options':{'type':'object'},'expectedRevision':{'type':'integer','minimum':0}}
    add('build_describe','List adapter build targets',{})
    add('build_prepare','Validate build request',build,['targetId'])
    add('build_execute','Export adapter package',build,['targetId'])
    operations[-1]['exclusiveGroups']=['build'];operations[-1]['supportsProgress']=True
    add('build_request','Start editor build job',build,['targetId'])
    add('build_status','Inspect editor build job',{})
    add('build_cancel','Cancel editor build job',{})

    # Milestones 10-12 extend existing contracts without breaking pure assets.
    for op in operations:
        if op['id']=='graph_function_create':op['inputSchema']['properties']['mode']={'type':'string','enum':['pure','execution'],'default':'pure'}
    add('graph_debug_step_into','Step into function',g,['graphId'])
    add('graph_debug_step_out','Step out of function',g,['graphId'])
    add('graph_diff','Compare saved graph',{'graphId':text,'ignorePresentation':{'type':'boolean','default':False}},['graphId'])
    add('graph_diff_view','Open Graph Changes',g,['graphId'])

    add('material_prepare','Prepare preview shader',{'graphId':text,'expectedRevision':{'type':'integer','minimum':0}},['graphId'])
    add('material_sample','Sample material at UV',{**g,'u':{'type':'number','minimum':-10000,'maximum':10000},'v':{'type':'number','minimum':-10000,'maximum':10000}},['graphId'])
    add('material_compile','Compile preview shader',{'graphId':text,'expectedRevision':{'type':'integer','minimum':0}},['graphId'])
    operations[-1]['exclusiveGroups']=['shader'];operations[-1]['supportsProgress']=True
    add('material_compile_request','Start preview shader job',{'graphId':text,'expectedRevision':{'type':'integer','minimum':0}},['graphId'])
    add('material_compile_status','Inspect preview shader job',g,['graphId'])
    add('material_compile_cancel','Cancel preview shader job',g,['graphId'])
    add('material_preview_apply','Use compiled preview shader',{**g,'reportPath':text,'expectedRevision':{'type':'integer','minimum':0}},['graphId','reportPath'])
    add('material_compiler_view','Open Material Compiler',g,['graphId'])
    add('texture_info','Inspect image asset',{'path':text},['path'])
    add('texture_open','Open image asset editor',{'path':text},['path'])

    add('material_preview_settings','Set preview surface',{**g,'shape':{'type':'string','enum':['Sphere','Plane']}},['graphId','shape'])
    add('texture_view_settings','Set image view',{'path':text,'channel':{'type':'string','enum':['RGBA','RGB','R','G','B','A']},'fit':{'type':'boolean'},'zoom':{'type':'number','minimum':0.05,'maximum':8}},['path'])

    imp={'sourcePath':text,'destination':text,'importerId':text,'reimport':{'type':'boolean','default':False},'path':text}
    add('asset_importers','List asset importers',{})
    add('asset_import_prepare','Prepare asset import',imp)
    add('asset_import_commit','Commit prepared import',{'reportPath':text},['reportPath'])
    add('asset_import','Import asset',imp,['sourcePath','destination'])
    operations[-1]['supportsProgress']=True;operations[-1]['exclusiveGroups']=['import']
    add('asset_reimport','Reimport asset',{'path':text},['path'])
    operations[-1]['supportsProgress']=True;operations[-1]['exclusiveGroups']=['import']
    add('asset_import_info','Inspect import source',{'path':text},['path'])
    add('asset_import_check','Check import source',{'path':text},['path'])
    add('asset_import_request','Start asset import job',imp)
    add('asset_import_status','Inspect asset import job',{})
    add('asset_import_cancel','Cancel asset import job',{})
    add('asset_import_view','Open Asset Import',{'path':text})

    add('asset_browser_settings','Set Content Browser view',{'query':{'type':'string','maxLength':256},'type':{'type':'string','enum':['All','Graph','Texture','Mesh','Audio','Other']},'grid':{'type':'boolean'},'folder':{'type':'string','maxLength':1024}})
    for op in operations:
        if op['id']=='assets_list':op['outputSchema']['properties']['assets']['items']['properties']['imported']={'type':'boolean'}

    m={'modelId':text}
    index={'type':'integer','minimum':0,'maximum':20000}
    vec3={'type':'array','minItems':3,'maxItems':3,'items':{'type':'number','minimum':-1e8,'maximum':1e8}}
    vec4={'type':'array','minItems':4,'maxItems':4,'items':{'type':'number','minimum':-1e8,'maximum':1e8}}
    add('model_open','Open 3D document',{'path':text},['path'],True)
    add('model_get','Describe 3D document',m,['modelId'])
    add('model_export_prepare','Prepare 3D export',m,['modelId'])
    add('model_node_set','Edit skeleton or scene node',{**m,'node':index,'name':text,'parent':{'type':'integer','minimum':-1,'maximum':511},'translation':vec3,'rotation':vec4,'scale':vec3},['modelId','node'],True)
    add('model_vertex_set','Edit mesh vertex',{**m,'mesh':index,'primitive':index,'vertex':index,'positions':vec3,'normals':vec3,'uv':{'type':'array','minItems':2,'maxItems':2,'items':{'type':'number'}},'joints':{'type':'array','minItems':4,'maxItems':4,'items':index},'weights':vec4},['modelId','mesh','primitive','vertex'],True)
    add('model_key_set','Edit animation key',{**m,'animation':index,'track':index,'key':index,'time':{'type':'number','minimum':0,'maximum':1e6},'value':{'type':'array','minItems':3,'maxItems':4,'items':{'type':'number'}}},['modelId','animation','track','key'],True)
    add('model_sample','Sample mesh and animated pose',{**m,'animation':{'type':'integer','minimum':-1,'maximum':127},'time':{'type':'number','minimum':0,'maximum':1e6}},['modelId'])
    add('model_save','Save 3D document',m,['modelId'])
    add('model_close','Close saved 3D document',m,['modelId'],True)
    add('model_export','Export GLB or CyPack',{**m,'format':{'type':'string','enum':['glb','cypack']}},['modelId','format'])
    operations[-1]['supportsProgress']=True

    jd={'jsonId':text}
    ptr={'type':'string','maxLength':2048}
    add('json_open','Open JSON document',{'path':text},['path'],True)
    add('json_get','Describe JSON document',jd,['jsonId'])
    add('json_value_set','Edit JSON value',{**jd,'pointer':ptr,'value':{}},['jsonId','pointer','value'],True)
    add('json_add','Add JSON property or array item',{**jd,'pointer':ptr,'name':text,'value':{}},['jsonId','pointer','value'],True)
    add('json_remove','Remove JSON value',{**jd,'pointer':ptr},['jsonId','pointer'],True)
    add('json_save','Save JSON document',jd,['jsonId'])

    add('model_reload','Reload saved 3D document',m,['modelId'],True)
    add('model_view_settings','Set 3D preview playback',{**m,'animation':{'type':'integer','minimum':-1,'maximum':127},'time':{'type':'number','minimum':0,'maximum':1e6},'playing':{'type':'boolean'},'tab':{'type':'string','enum':['Skeleton','Mesh','Animation']}},['modelId'])
    for operation in operations:
        if operation['id'] in ('asset_import','asset_reimport','model_export'):operation['resourceRequirements']={'ramBytes':512*1024**2}

    add('model_clip_create','Create animation clip',{**m,'name':text,'copyFrom':index},['modelId','name'],True)
    add('model_track_add','Add animation track',{**m,'animation':index,'node':index,'path':{'type':'string','enum':['translation','rotation','scale']}},['modelId','animation','node','path'],True)
    add('model_key_add','Add animation key',{**m,'animation':index,'track':index,'time':{'type':'number','minimum':0,'maximum':1e6},'value':{'type':'array','minItems':3,'maxItems':4,'items':{'type':'number'}}},['modelId','animation','track','time','value'],True)
    add('model_key_remove','Remove animation key',{**m,'animation':index,'track':index,'key':index},['modelId','animation','track','key'],True)

    add("json_close","Close saved JSON document",jd,["jsonId"],True)

    add("model_list","List loaded 3D documents",{})

    process={**m,'action':{'type':'string','enum':['lod','collision','retarget','ik','root_motion','clip_edit']},'sourceModelId':text,'options':{'type':'object','maxProperties':16}}
    add('model_process_prepare','Prepare model authoring job',process,['modelId','action'])
    add('model_process','Process model authoring job',process,['modelId','action'])
    operations[-1]['supportsProgress']=True;operations[-1]['resourceRequirements']={'ramBytes':512*1024**2}
    add('model_process_commit','Apply model authoring result',{'reportPath':text},['reportPath'],True)
    add('model_lod_remove','Remove generated LOD',{**m,'mesh':index,'lod':{'type':'integer','minimum':1,'maximum':4}},['modelId','mesh','lod'],True)
    add('model_collision_remove','Remove collision shape',{**m,'collisionId':text},['modelId','collisionId'],True)
    add('model_collision_set','Edit collision shape',{**m,'collisionId':text,'name':name,'center':{'type':'array','minItems':3,'maxItems':3,'items':{'type':'number'}},'rotation':{'type':'array','minItems':4,'maxItems':4,'items':{'type':'number'}},'halfExtents':{'type':'array','minItems':3,'maxItems':3,'items':{'type':'number','exclusiveMinimum':0}},'radius':{'type':'number','exclusiveMinimum':0},'halfHeight':{'type':'number','minimum':0},'enabled':{'type':'boolean'}},['modelId','collisionId'],True)
    for operation in operations:
        if operation['id'] in ('model_sample','model_view_settings','model_export'):operation['inputSchema']['properties']['lod']={'type':'integer','minimum':0,'maximum':4}
        if operation['id']=='model_view_settings':operation['inputSchema']['properties']['tab']['enum']+=['LODs','Collision','Retarget']

    types=['float','bool','string','color','int','vector3','object','array.float','array.int','array.string','array.bool','array.object','array.vector3','map.float','map.string','map.object','set.string','set.int']
    for operation in operations:
        if operation['id']=='graph_variable_add':operation['inputSchema']['properties']['type']['enum']=types
        if operation['id']=='graph_function_create':operation['inputSchema']['properties']['kind']={'type':'string','enum':['function','macro']}
        if operation['id']=='graph_node_add':operation['inputSchema']['properties'].update(classRef=name,methodRef=name)
    add('graph_class_define','Define Blueprint class',{**g,'name':name,'parent':{'type':'string','maxLength':64},'fields':{'type':'object','maxProperties':64},'methods':{'type':'object','maxProperties':64,'additionalProperties':name}},['graphId','name','fields'],True)
    add('graph_class_remove','Remove Blueprint class',{**g,'name':name},['graphId','name'],True)
    add('graph_debug_tick','Advance graph simulation time',{**g,'delta':{'type':'number','minimum':0,'maximum':60}},['graphId','delta'])
    add('graph_debug_event','Emit graph custom event',{**g,'event':name},['graphId','event'])

    add('game_codec_describe','Describe adapter codec declarations',{})
    add('game_codec_list','List executable game codecs',{})
    add('game_codec_export','Export asset through game codec',{**m,'codecId':name,'gameVersion':name,'platform':name,'options':{'type':'object','maxProperties':32},'acceptedLosses':{'type':'array','maxItems':64,'uniqueItems':True,'items':text},'requireVerified':{'type':'boolean','default':True}},['modelId','codecId','gameVersion','platform'])
    operations[-1]['supportsProgress']=True;operations[-1]['resourceRequirements']={'ramBytes':512*1024**2}

    add('graph_debug_object_set','Edit runtime object field',{**g,'object':text,'field':name,'value':{}},['graphId','object','field','value'])

    for operation in operations:
        if operation['id']=='model_view_settings':operation['inputSchema']['properties'].update(wireframe={'type':'boolean'},collisions={'type':'boolean'},sourceModelId=text,mapping={'type':'object','maxProperties':256,'additionalProperties':index})

    for operation in operations:
        if operation['id']=='model_collision_set':operation['inputSchema']['properties']['needsReview']={'type':'boolean'}

    add('graph_debug_view','Open graph runtime debugger',g,['graphId'])

    for operation in operations:
        if operation['id']=='model_view_settings':
            operation['inputSchema']['properties']['tab']['enum'].append('Motion tools')
            operation['inputSchema']['properties'].update(motionTool={'type':'string','enum':['IK','RootMotion','Clip']},ikRoot=index,ikTip=index,motionRoot=index,ikTarget={'type':'array','minItems':3,'maxItems':3,'items':{'type':'number'}},ikPole={'type':'array','minItems':3,'maxItems':3,'items':{'type':'number'}})

    add('graph_types_get','Describe named graph types',g,['graphId'])
    add('graph_type_define','Define enum or structure',{**g,'typeId':{'type':'string','minLength':6,'maxLength':71},'definition':{'type':'object','maxProperties':16}},['graphId','typeId','definition'],True)
    add('graph_type_remove','Remove named graph type',{**g,'typeId':{'type':'string','minLength':6,'maxLength':71}},['graphId','typeId'],True)
    for operation in operations:
        if operation['id']=='graph_variable_add':operation['inputSchema']['properties']['type']={'type':'string','minLength':1,'maxLength':71}
        if operation['id']=='graph_node_add':operation['inputSchema']['properties'].update(typeRef={'type':'string','minLength':1,'maxLength':71},fieldRef={'type':'string','minLength':1,'maxLength':64})

    add('graph_interfaces_get','Describe Blueprint interfaces',g,['graphId'])
    add('graph_interface_define','Define Blueprint interface',{**g,'name':name,'definition':{'type':'object','maxProperties':16}},['graphId','name','definition'],True)
    add('graph_interface_remove','Remove Blueprint interface',{**g,'name':name},['graphId','name'],True)
    for operation in operations:
        if operation['id']=='graph_node_add':operation['inputSchema']['properties']['interfaceRef']=name
        if operation['id']=='graph_class_define':operation['inputSchema']['properties']['interfaces']={'type':'array','maxItems':32,'uniqueItems':True,'items':name}

    migration={**g,'target':{'type':'string','enum':['function','interface']},'name':name,'method':{'type':'string','minLength':1,'maxLength':64},'inputs':{'type':'object','maxProperties':16},'outputs':{'type':'object','maxProperties':16},'renames':{'type':'object','properties':{side:{'type':'object','maxProperties':16,'additionalProperties':{'type':'string','minLength':1,'maxLength':64}} for side in ('inputs','outputs')},'additionalProperties':False}}
    required=['graphId','target','name','inputs','outputs']
    add('graph_signature_plan','Preview signature migration',migration,required)
    add('graph_signature_apply','Apply signature migration',{**migration,'planHash':{'type':'string','pattern':'^[0-9a-f]{64}$'},'acceptLoss':{'type':'boolean'}},required+['planHash'],True)

    type_migration={**g,'typeId':{'type':'string','minLength':6,'maxLength':71},'definition':{'type':'object','maxProperties':16},'renames':{'type':'object','maxProperties':64,'additionalProperties':{'type':'string','minLength':1,'maxLength':64}}}
    add('graph_type_migration_plan','Preview data type migration',type_migration,['graphId','typeId','definition'])
    add('graph_type_migration_apply','Apply data type migration',{**type_migration,'planHash':{'type':'string','pattern':'^[0-9a-f]{64}$'},'acceptLoss':{'type':'boolean'}},['graphId','typeId','definition','planHash'],True)

    collapse={**ctx,'nodeIds':{'type':'array','minItems':1,'maxItems':128,'uniqueItems':True,'items':{'type':'string','minLength':1,'maxLength':128}},'name':{'type':'string','minLength':1,'maxLength':64},'kind':{'type':'string','enum':['function','macro']}}
    add('graph_collapse_plan','Preview collapse selection',collapse,['graphId','nodeIds','name'])
    add('graph_collapse_apply','Apply collapse selection',{**collapse,'planHash':{'type':'string','pattern':'^[0-9a-f]{64}$'}},['graphId','nodeIds','name','planHash'],True)

    expand={**ctx,"nodeId":{"type":"string","minLength":1,"maxLength":128}}
    add("graph_expand_plan","Preview expand call",expand,["graphId","nodeId"])
    add("graph_expand_apply","Apply expand call",{**expand,"planHash":{"type":"string","pattern":"^[0-9a-f]{64}$"}},["graphId","nodeId","planHash"],True)
