"""Neutral editable 3D documents, preserving original GLB and portable identity.
All external codecs run in the existing Core import/export workers.
"""
import base64
import hashlib
import copy
import json
import math
import struct
import uuid
from pathlib import Path
from portable_assets.glb import AssetError, inspect_glb, strict_json
from portable_assets.package import bounded_read, read_package, package_glb
MAX_DOCUMENT=2*1024*1024
MAX_VERTICES=20000
MAX_KEYS=20000

def require(value,message):
    if not value:raise AssetError('UnsupportedScene3D',message)

def unpack_glb(data):
    inspect_glb(data)
    length=struct.unpack_from('<I',data,12)[0]
    doc=strict_json(data[20:20+length])
    offset=20+length
    binary=data[offset+8:] if offset<len(data) else b''
    return doc,binary

def decode(source):
    path=Path(source)
    manifest,data=read_package(path) if path.suffix.lower()=='.cypack' else (None,bounded_read(path,64*1024*1024))
    doc,binary=unpack_glb(data)
    require(not doc.get('extensionsRequired'),'Required GLB extensions need a registered decoder')
    budget=0
    def accessor(index):
        nonlocal budget
        a=doc['accessors'][index];typ=a['type'];component=a['componentType'];count=a['count']
        formats={5120:('b',1),5121:('B',1),5122:('h',2),5123:('H',2),5125:('I',4),5126:('f',4)}
        sizes={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
        require(component in formats and typ in sizes and type(count) is int and 0<count<=100000,'Unsupported accessor shape')
        fmt,width=formats[component];n=sizes[typ];budget+=count*n
        require(budget<=2000000,'Decoded accessor budget exceeded')
        result=[[0]*n for _ in range(count)]
        def read_view(view_index,offset,amount,components,code,component_width,strided=False):
            view=doc['bufferViews'][view_index];require(view.get('buffer',0)==0,'Only embedded buffer zero is supported')
            stride=view.get('byteStride',components*component_width) if strided else components*component_width
            require(type(stride) is int and stride>=components*component_width and stride%component_width==0,'Invalid accessor stride')
            require(type(offset) is int and offset>=0 and offset%component_width==0,'Invalid accessor offset')
            require(offset+(amount-1)*stride+components*component_width<=view['byteLength'],'Accessor exceeds bufferView')
            start=view.get('byteOffset',0)+offset
            return [list(struct.unpack_from('<'+code*components,binary,start+i*stride)) for i in range(amount)]
        if 'bufferView' in a:result=read_view(a['bufferView'],a.get('byteOffset',0),count,n,fmt,width,True)
        if 'sparse' in a:
            sparse=a['sparse'];k=sparse['count'];indices=sparse['indices'];values=sparse['values']
            require(type(k) is int and 0<k<=count and indices['componentType'] in (5121,5123,5125),'Invalid sparse accessor')
            code,w=formats[indices['componentType']];ids=read_view(indices['bufferView'],indices.get('byteOffset',0),k,1,code,w)
            vals=read_view(values['bufferView'],values.get('byteOffset',0),k,n,fmt,width)
            require(all(0<=i[0]<count for i in ids) and all(ids[i][0]<ids[i+1][0] for i in range(k-1)),'Sparse indices must increase within bounds')
            for i,value in zip(ids,vals):result[i[0]]=value
        if a.get('normalized',False):
            require(component in (5120,5121,5122,5123),'Invalid normalized component')
            divisor={5120:127,5121:255,5122:32767,5123:65535}[component]
            result=[[max(-1,v/divisor) for v in row] for row in result]
        require(all(math.isfinite(v) for row in result for v in row),'Accessor contains non-finite values')
        return result
    nodes=copy.deepcopy(doc.get('nodes',[]));require(len(nodes)<=512,'Scene node limit is 512')
    parents=[-1]*len(nodes)
    for i,n in enumerate(nodes):
        for child in n.get('children',[]):
            require(type(child) is int and 0<=child<len(nodes) and parents[child]==-1,'Invalid shared node parent')
            parents[child]=i
    result_nodes=[]
    for i,n in enumerate(nodes):
        seen={i};parent=parents[i]
        while parent!=-1:
            require(parent not in seen,'Node hierarchy contains a cycle');seen.add(parent);parent=parents[parent]
        result_nodes.append({'id':'node:'+str(i),'name':n.get('name','Node '+str(i)),'parent':parents[i],
            'translation':n.get('translation',[0,0,0]),'rotation':n.get('rotation',[0,0,0,1]),'scale':n.get('scale',[1,1,1]),
            **({'matrix':n['matrix']} if 'matrix' in n else {}),**{k:n[k] for k in ('mesh','skin') if k in n}})
    meshes=[];vertices=0
    for mi,mesh in enumerate(doc.get('meshes',[])):
        primitives=[]
        for pi,p in enumerate(mesh['primitives']):
            require(p.get('mode',4)==4 and not p.get('targets') and not p.get('extensions'),'Only uncompressed triangle primitives without morph targets are editable')
            a=p['attributes'];require('POSITION' in a,'Mesh requires POSITION')
            positions=accessor(a['POSITION']);vertices+=len(positions);require(vertices<=MAX_VERTICES,'Editable mesh exceeds 20,000 vertices')
            require(doc['accessors'][a['POSITION']]['type']=='VEC3' and doc['accessors'][a['POSITION']]['componentType']==5126,'Positions must be FLOAT VEC3')
            indices=[v[0] for v in accessor(p['indices'])] if 'indices' in p else list(range(len(positions)))
            require(len(indices)<=60000 and len(indices)%3==0 and all(type(v) is int and 0<=v<len(positions) for v in indices),'Invalid triangle indices')
            primitive={'sourceMesh':mi,'sourcePrimitive':pi,'positions':positions,'indices':indices,'material':p.get('material',-1)}
            for key,attribute,size in [('normals','NORMAL',3),('uv','TEXCOORD_0',2),('joints','JOINTS_0',4),('weights','WEIGHTS_0',4)]:
                if attribute in a:
                    value=accessor(a[attribute]);require(len(value)==len(positions) and all(len(v)==size for v in value),'Invalid vertex stream: '+attribute);primitive[key]=value
            require(('joints' in primitive)==('weights' in primitive),'Skinning requires joint indices and weights')
            primitives.append(primitive)
        meshes.append({'name':mesh.get('name','Mesh '+str(mi)),'primitives':primitives})
    skins=[]
    for skin in doc.get('skins',[]):
        joints=skin['joints'];require(0<len(joints)<=256 and len(set(joints))==len(joints) and all(type(j) is int and 0<=j<len(nodes) for j in joints),'Invalid skin joints')
        inverse=accessor(skin['inverseBindMatrices']) if 'inverseBindMatrices' in skin else [[1 if i%5==0 else 0 for i in range(16)] for _ in joints]
        require(len(inverse)==len(joints) and all(len(m)==16 for m in inverse),'Invalid inverse bind matrices')
        skins.append({'name':skin.get('name','Skeleton'),'joints':joints,'inverseBindMatrices':inverse})
    animations=[];keys=0
    for ai,animation in enumerate(doc.get('animations',[])):
        tracks=[]
        for ci,channel in enumerate(animation['channels']):
            sampler=animation['samplers'][channel['sampler']];node=channel['target']['node'];target=channel['target']['path'];interpolation=sampler.get('interpolation','LINEAR')
            require(type(node) is int and 0<=node<len(nodes) and 'matrix' not in nodes[node] and target in ('translation','rotation','scale'),'Unsupported animation target')
            require(interpolation in ('LINEAR','STEP','CUBICSPLINE'),'Unsupported interpolation')
            times=[v[0] for v in accessor(sampler['input'])];values=accessor(sampler['output']);keys+=len(times)
            require(keys<=MAX_KEYS and all(t>=0 for t in times) and all(a<b for a,b in zip(times,times[1:])),'Animation times must increase; key limit is 20,000')
            require(len(values)==len(times)*(3 if interpolation=='CUBICSPLINE' else 1) and all(len(v)==(4 if target=='rotation' else 3) for v in values),'Animation output shape mismatch')
            tracks.append({'sourceChannel':ci,'node':node,'path':target,'times':times,'values':values,'interpolation':interpolation})
        animations.append({'sourceAnimation':ai,'name':animation.get('name','Animation '+str(ai)),'tracks':tracks})
    extensions=copy.deepcopy(manifest.get('extensions',{})) if manifest else {}
    extras=doc.get('extras',{})
    reference=extensions.get('org.cygame.editor')
    if isinstance(reference,dict) and reference.get('storage')=='glTF.extras':require(isinstance(extras,dict) and 'org.cygame.editor' in extras,'Authoring payload reference is missing')
    if isinstance(extras,dict) and 'org.cygame.metadata' in extras:
        siblings=extras['org.cygame.metadata'];require(isinstance(siblings,dict) and 'org.cygame.editor' not in siblings,'Invalid portable metadata namespaces')
        for namespace,value in siblings.items():
            require(namespace not in extensions or extensions[namespace]==value,'Portable metadata namespace conflict')
            extensions[namespace]=copy.deepcopy(value)
    if isinstance(extras,dict) and 'org.cygame.editor' in extras:
        meta=extras['org.cygame.editor']
        envelope=extensions.get('org.cygame.editor')
        if isinstance(envelope,dict) and envelope.get('storage')=='glTF.extras':
            digest=hashlib.sha256(json.dumps(meta,ensure_ascii=False,sort_keys=True,separators=(',',':'),allow_nan=False).encode()).hexdigest()
            require(envelope.get('sha256')==digest,'Authoring payload reference hash mismatch')
        else:require(envelope is None or envelope==meta,'Portable/editor metadata conflict')
        extensions['org.cygame.editor']=copy.deepcopy(meta)
    model={'format':'cygame-scene3d','schemaVersion':1,'assetId':manifest['assetId'] if manifest else str(uuid.uuid4()),'name':manifest['name'] if manifest else path.stem,
        'space':{'handedness':'right','upAxis':'+Y','linearUnit':'meter','rotation':'quaternion-xyzw','timeUnit':'second'},
        'nodes':result_nodes,'meshes':meshes,'skins':skins,'animations':animations,
        'materials':[{'name':m.get('name','Material '+str(i)),'baseColor':m.get('pbrMetallicRoughness',{}).get('baseColorFactor',[.65,.65,.65,1])} for i,m in enumerate(doc.get('materials',[]))],
        'extensions':extensions,
        'source':{'glbBase64':base64.b64encode(data).decode(),**({'manifest':manifest} if manifest else {})}}
    require(len(json.dumps(model,ensure_ascii=False,separators=(',',':')).encode())<=MAX_DOCUMENT,'Decoded 3D document exceeds 2 MiB; use a source provider for larger assets')
    return model

def encode(model):
    require(model.get('format')=='cygame-scene3d' and model.get('schemaVersion')==1,'Unsupported native 3D document')
    doc,binary=unpack_glb(base64.b64decode(model['source']['glbBase64'],validate=True));binary=bytearray(binary[:(doc.get('buffers') or [{'byteLength':0}])[0]['byteLength']])
    def append(values,typ,code='f',component=5126,bounds=False):
        while len(binary)%4:binary.append(0)
        count=len(values);width=len(values[0]);offset=len(binary)
        for row in values:binary.extend(struct.pack('<'+code*len(row),*row))
        views=doc.setdefault('bufferViews',[]);view=len(views);views.append({'buffer':0,'byteOffset':offset,'byteLength':len(binary)-offset})
        accessor={'bufferView':view,'componentType':component,'count':count,'type':typ}
        if bounds:accessor.update(min=[min(v[j] for v in values) for j in range(width)],max=[max(v[j] for v in values) for j in range(width)])
        accessors=doc.setdefault('accessors',[]);index=len(accessors);accessors.append(accessor);return index
    memberships=[]
    for scene in doc.get('scenes',[]):
        members=set();pending=list(scene.get('nodes',[]))
        while pending:
            node=pending.pop()
            if node in members:continue
            members.add(node);pending.extend(doc['nodes'][node].get('children',[]))
        memberships.append(members)
    for i,node in enumerate(model['nodes']):
        original=doc['nodes'][i];original['name']=node['name'];original.pop('children',None)
        if 'matrix' not in node:
            original.pop('matrix',None)
            for key in ('translation','rotation','scale'):original[key]=node[key]
    for i,node in enumerate(model['nodes']):
        if node['parent']!=-1:doc['nodes'][node['parent']].setdefault('children',[]).append(i)
    oldroots={i for i,node in enumerate(model['nodes']) if node['parent']==-1}
    for scene,members in zip(doc.get('scenes',[]),memberships):
        scene['nodes']=sorted(oldroots & members)
    for mesh in model['meshes']:
        for primitive in mesh['primitives']:
            p=doc['meshes'][primitive['sourceMesh']]['primitives'][primitive['sourcePrimitive']]
            for key,attribute,typ,code,component in [('positions','POSITION','VEC3','f',5126),('normals','NORMAL','VEC3','f',5126),('uv','TEXCOORD_0','VEC2','f',5126),('joints','JOINTS_0','VEC4','H',5123),('weights','WEIGHTS_0','VEC4','f',5126)]:
                if key in primitive:p['attributes'][attribute]=append(primitive[key],typ,code,component,attribute=='POSITION')
            p['indices']=append([[i] for i in primitive['indices']],'SCALAR','I',5125)
    for si,skin in enumerate(model['skins']):
        original=doc['skins'][si];original['name']=skin['name'];original['joints']=skin['joints'];original['inverseBindMatrices']=append(skin['inverseBindMatrices'],'MAT4')
    original_animations=copy.deepcopy(doc.get('animations',[]));doc['animations']=[]
    for ai,animation in enumerate(model['animations']):
        require(bool(animation['tracks']),'Empty animation clips need a track before export')
        source_index=animation.get('sourceAnimation',ai if ai<len(original_animations) else -1)
        original=original_animations[source_index] if 0<=source_index<len(original_animations) else {}
        target=copy.deepcopy(original);target.update(name=animation['name'],channels=[],samplers=[])
        for ci,track in enumerate(animation['tracks']):
            source_channel=track.get('sourceChannel',ci if ci<len(original.get('channels',[])) else -1)
            channel=copy.deepcopy(original.get('channels',[])[source_channel]) if 0<=source_channel<len(original.get('channels',[])) else {}
            old=copy.deepcopy(original['samplers'][channel['sampler']]) if 'sampler' in channel else {}
            old.update(input=append([[t] for t in track['times']],'SCALAR',bounds=True),output=append(track['values'],'VEC4' if track['path']=='rotation' else 'VEC3'),interpolation=track['interpolation'])
            target_node=copy.deepcopy(channel.get('target',{}));target_node.update(node=track['node'],path=track['path'])
            channel.update(sampler=len(target['samplers']),target=target_node)
            target['samplers'].append(old);target['channels'].append(channel)
        if target['channels']:doc['animations'].append(target)
    if not doc['animations']:doc.pop('animations')
    if binary:doc.setdefault('buffers',[{}])[0]['byteLength']=len(binary)
    else:doc.pop('buffers',None)
    namespaces=model.get('extensions',{});require(isinstance(namespaces,dict),'Model extensions must be namespaced metadata')
    if isinstance(doc.get('extras'),dict):
        doc['extras'].pop('org.cygame.metadata',None);doc['extras'].pop('org.cygame.editor',None)
    siblings={key:copy.deepcopy(value) for key,value in namespaces.items() if key!='org.cygame.editor'}
    if siblings:
        require(isinstance(doc.get('extras',{}),dict),'Cannot add portable metadata over non-object glTF extras')
        doc.setdefault('extras',{})['org.cygame.metadata']=siblings
    metadata=namespaces.get('org.cygame.editor')
    if metadata is not None:
        require(isinstance(doc.get('extras',{}),dict),'Cannot add authoring metadata over non-object glTF extras')
        doc.setdefault('extras',{})['org.cygame.editor']=copy.deepcopy(metadata)
    encoded=json.dumps(doc,ensure_ascii=False,separators=(',',':'),allow_nan=False).encode();encoded+=b' '*((-len(encoded))%4);binary.extend(b'\0'*((-len(binary))%4))
    result=struct.pack('<4sII',b'glTF',2,12+8+len(encoded)+(8+len(binary) if binary else 0))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+(struct.pack('<II',len(binary),0x004e4942)+binary if binary else b'')
    inspect_glb(result);return result

def export(model,destination):
    destination=Path(destination);require(not destination.exists(),'Export refuses existing files')
    data=encode(model)
    if destination.suffix.lower()=='.glb':
        with destination.open('xb') as stream:stream.write(data)
        return {'outputPath':str(destination),'assetId':model['assetId']}
    require(destination.suffix.lower()=='.cypack','Export supports GLB or .cypack')
    temporary=destination.with_name(destination.name+'.source.glb')
    created=False
    try:
        with temporary.open('xb') as stream:
            created=True;stream.write(data)
        manifest=model['source'].get('manifest',{})
        extensions=copy.deepcopy(model.get('extensions',{}))
        if 'org.cygame.editor' in extensions:
            digest=hashlib.sha256(json.dumps(extensions['org.cygame.editor'],ensure_ascii=False,sort_keys=True,separators=(',',':'),allow_nan=False).encode()).hexdigest()
            extensions['org.cygame.editor']={'storage':'glTF.extras','schemaVersion':1,'sha256':digest}
        return package_glb(temporary,destination,name=model['name'],asset_id=model['assetId'],extensions=extensions,provenance=manifest.get('provenance',{}))
    finally:
        if created:temporary.unlink(missing_ok=True)
