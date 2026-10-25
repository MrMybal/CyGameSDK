"""Bounded neutral authoring algorithms; trusted Core worker code, no engine codec."""
import copy,math,uuid
NS='org.cygame.editor'
def require(ok,message):
 if not ok:raise ValueError(message)
def add(a,b):return [x+y for x,y in zip(a,b)]
def sub(a,b):return [x-y for x,y in zip(a,b)]
def mul(a,s):return [x*s for x in a]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def length(a):return math.sqrt(dot(a,a))
def unit(a):
 n=length(a);require(n>1e-12,'Cannot normalize a zero vector');return mul(a,1/n)
def qm(a,b):
 x,y,z,w=a;u,v,t,s=b
 return [w*u+x*s+y*t-z*v,w*v-x*t+y*s+z*u,w*t+x*v-y*u+z*s,w*s-x*u-y*v-z*t]
def qi(q):return [-q[0],-q[1],-q[2],q[3]]
def rotate(q,v):return qm(qm(q,[*v,0]),qi(q))[:3]
def slerp(a,b,t):
 a=unit(a);b=unit(b);c=dot(a,b)
 if c<0:b=mul(b,-1);c=-c
 if c>.9995:return unit(add(mul(a,1-t),mul(b,t)))
 theta=math.acos(max(-1,min(1,c)));return add(mul(a,math.sin((1-t)*theta)/math.sin(theta)),mul(b,math.sin(t*theta)/math.sin(theta)))
def sample(track,time):
 times=track['times'];values=track['values'];a=0
 while a+1<len(times) and times[a+1]<=time:a+=1
 b=min(a+1,len(times)-1);cubic=track['interpolation']=='CUBICSPLINE';va=values[a*3+1 if cubic else a]
 if a==b or track['interpolation']=='STEP':return va
 span=times[b]-times[a];t=max(0,min(1,(time-times[a])/span));vb=values[b*3+1 if cubic else b]
 if cubic:return [(2*t**3-3*t*t+1)*x+(t**3-2*t*t+t)*span*out+(-2*t**3+3*t*t)*y+(t**3-t*t)*span*incoming for x,y,out,incoming in zip(va,vb,values[a*3+2],values[b*3])]
 return slerp(va,vb,t) if track['path']=='rotation' else add(mul(va,1-t),mul(vb,t))
def pose(model,clip=None,time=0):
 nodes=copy.deepcopy(model['nodes'])
 if clip:
  for track in clip['tracks']:nodes[track['node']][track['path']]=sample(track,time)
 world={}
 def visit(i):
  if i in world:return world[i]
  n=nodes[i];require('matrix' not in n,'Retargeting currently requires decomposed TRS nodes')
  q=unit(n['rotation']);p=n['translation'];s=n['scale'];parent=n['parent']
  if parent>=0:
   pp,pq,ps=visit(parent);p=add(pp,rotate(pq,[x*y for x,y in zip(p,ps)]));q=unit(qm(pq,q));s=[x*y for x,y in zip(ps,s)]
  world[i]=(p,q,s);return world[i]
 for i in range(len(nodes)):visit(i)
 return nodes,world

def retarget(target,source,options):
 animation=options.get('animation',0);require(type(animation) is int and 0<=animation<len(source['animations']),'Select a source animation')
 clip=source['animations'][animation];require(clip['tracks'],'Source clip is empty')
 mapping=options.get('mapping',{})
 if not mapping:
  def names(model):
   found={}
   for i,n in enumerate(model['nodes']):found.setdefault(n['name'].rsplit(':',1)[-1].lower(),[]).append(i)
   return found
  sn,tn=names(source),names(target);mapping={str(a[0]):tn[name][0] for name,a in sn.items() if len(a)==1 and name in tn and len(tn[name])==1}
 require(isinstance(mapping,dict) and 0<len(mapping)<=256,'Provide an unambiguous source-node to target-node mapping')
 pairs={int(a):b for a,b in mapping.items()};require(len(set(pairs.values()))==len(pairs),'A target bone cannot be mapped twice')
 for a,b in pairs.items():require(type(b) is int and 0<=a<len(source['nodes']) and 0<=b<len(target['nodes']),'Mapping index is outside the skeleton')
 fps=options.get('fps',30);require(type(fps) in (int,float) and math.isfinite(fps) and 1<=fps<=120,'Bake rate must be 1..120 FPS')
 mode=options.get('translationMode','bind');require(mode in ('bind','root_scaled','scaled'),'Unknown translation mode')
 factor=options.get('translationScale',1);require(type(factor) in (int,float) and math.isfinite(factor) and 0<factor<=1000,'Translation scale must be positive and bounded')
 start=min(t['times'][0] for t in clip['tracks']);end=max(t['times'][-1] for t in clip['tracks']);count=max(1,math.ceil((end-start)*fps))
 times=[start+(end-start)*i/count for i in range(count+1)] if end>start else [start]
 channels=1+(mode!='bind');require(len(times)*len(pairs)*channels<=20000,'Retarget bake exceeds 20,000 keys; reduce FPS or mapped bones')
 rest_s,ws=pose(source);rest_t,wt=pose(target);tracks=[];rot={b:[] for b in pairs.values()};translations={b:[] for b in pairs.values()}
 reverse={b:a for a,b in pairs.items()};ordered=sorted(reverse,key=lambda i:depth(target,i))
 for time in times:
  current,world=pose(source,clip,time);out={i:wt[i][1] for i in wt}
  for b in ordered:
   a=reverse[b];world_q=unit(qm(qm(wt[b][1],qi(ws[a][1])),world[a][1]));parent=target['nodes'][b]['parent']
   # Unmapped ancestors follow their target local bind orientation and animated parents.
   def parent_q(i):
    if i<0:return [0,0,0,1]
    if i in reverse:return out[i]
    p=target['nodes'][i]['parent'];return unit(qm(parent_q(p),rest_t[i]['rotation']))
   local=unit(qm(qi(parent_q(parent)),world_q));out[b]=world_q
   if rot[b] and dot(rot[b][-1],local)<0:local=mul(local,-1)
   rot[b].append(local)
   translate=rest_t[b]['translation']
   root=not any(ancestor in reverse for ancestor in ancestors(target,b))
   if mode=='scaled' or (mode=='root_scaled' and root):translate=add(translate,mul(sub(current[a]['translation'],rest_s[a]['translation']),factor))
   translations[b].append(translate)
 for b in ordered:
  tracks.append({'node':b,'path':'rotation','interpolation':'LINEAR','times':times,'values':rot[b]})
  if mode!='bind':tracks.append({'node':b,'path':'translation','interpolation':'LINEAR','times':times,'values':translations[b]})
 target['animations'].append({'name':options.get('name',clip['name']+'_Retargeted'),'tracks':tracks})
 return {'animation':len(target['animations'])-1,'mapping':{str(a):b for a,b in pairs.items()},'sampleCount':len(times),'translationMode':mode,'losses':['Baked to LINEAR samples','Source scale animation is not transferred','No IK or end-effector contacts']}
def ancestors(model,node):
 result=[];parent=model['nodes'][node]['parent']
 while parent>=0:result.append(parent);parent=model['nodes'][parent]['parent']
 return result
def depth(model,node):return len(ancestors(model,node))

def simplify(primitive,ratio):
 p=primitive;positions=p['positions'];faces=[p['indices'][i:i+3] for i in range(0,len(p['indices']),3)];require(not p.get('joints'),'Automatic LOD generation currently supports static meshes only')
 edges={}
 for face in faces:
  for a,b in zip(face,face[1:]+face[:1]):edge=tuple(sorted((a,b)));edges[edge]=edges.get(edge,0)+1
 require(all(n<=2 for n in edges.values()),'Non-manifold source requires an adapter simplifier')
 locked={i for edge,n in edges.items() if n==1 for i in edge};coincident={}
 for i,v in enumerate(positions):coincident.setdefault(tuple(round(x,8) for x in v),[]).append(i)
 for group in coincident.values():
  if len(group)>1:locked.update(group)
 minimum=[min(v[j] for v in positions) for j in range(3)];span=max(max(v[j] for v in positions)-minimum[j] for j in range(3));require(span>1e-9,'Mesh has zero extent')
 best=None;desired=max(1,int(len(faces)*ratio));closed=all(n==2 for n in edges.values())
 for divisions in (96,64,48,32,24,20,16,12,10,8,6,4,3,2,1):
  cells={};groups=[];remap={}
  for i,v in enumerate(positions):
   cell=('locked',i) if i in locked else tuple(math.floor((v[j]-minimum[j])*divisions/span) for j in range(3))
   if cell not in cells:cells[cell]=len(groups);groups.append([])
   remap[i]=cells[cell];groups[remap[i]].append(i)
  out=copy.deepcopy(p)
  for stream in ('positions','normals','uv'):
   if stream in p:
    out[stream]=[[sum(p[stream][i][j] for i in group)/len(group) for j in range(len(p[stream][0]))] for group in groups]
    if stream=='normals':out[stream]=[unit(n) if length(n)>1e-10 else [0,1,0] for n in out[stream]]
  indices=[];seen=set();bad=False
  for face in faces:
   f=[remap[i] for i in face]
   if len(set(f))<3:continue
   key=tuple(sorted(f))
   if key in seen:continue
   seen.add(key);a,b,c=[out['positions'][i] for i in f];x,y,z=[positions[i] for i in face]
   normal=cross(sub(b,a),sub(c,a))
   if length(normal)<1e-12:continue
   if dot(normal,cross(sub(y,x),sub(z,x)))<=0:bad=True;break
   indices.extend(f)
  if bad or not indices or len(indices)>=len(p['indices']):continue
  candidate_edges={}
  for k in range(0,len(indices),3):
   f=indices[k:k+3]
   for a,b in zip(f,f[1:]+f[:1]):e=tuple(sorted((a,b)));candidate_edges[e]=candidate_edges.get(e,0)+1
  if any(n>2 for n in candidate_edges.values()) or (closed and any(n!=2 for n in candidate_edges.values())):continue
  used=sorted(set(indices));compact={old:new for new,old in enumerate(used)}
  for stream in ('positions','normals','uv'):
   if stream in out:out[stream]=[out[stream][i] for i in used]
  out['indices']=[compact[i] for i in indices];triangles=len(indices)//3
  error=max(length(sub(positions[i],out['positions'][compact[remap[i]]])) for i in range(len(positions)) if remap[i] in compact)
  candidate=(abs(triangles-desired),out,error)
  if best is None or candidate[0]<best[0]:best=candidate
 require(best is not None,'No safe reduction found while preserving boundaries/seams; use another source or a trusted simplifier provider')
 return best[1],best[2]
def generate_lod(model,options):
 mesh=options.get('mesh',0);require(type(mesh) is int and 0<=mesh<len(model['meshes']),'Choose a mesh');require(not any(n.get('mesh')==mesh and 'skin' in n for n in model['nodes']),'Skinned LOD generation needs a skin-aware provider')
 ratio=options.get('ratio',.5);require(type(ratio) in (int,float) and math.isfinite(ratio) and .05<=ratio<=.95,'Reduction ratio must be .05..95')
 levels=model.setdefault('extensions',{}).setdefault(NS,{}).setdefault('lods',{}).setdefault(str(mesh),[]);replacement=options.get('replace',0);require(type(replacement) is int and 0<=replacement<=len(levels),'Replacement LOD does not exist');require(replacement>0 or len(levels)<4,'At most four generated LODs per mesh')
 import base64
 from scene3d import unpack_glb
 original,_=unpack_glb(base64.b64decode(model['source']['glbBase64'],validate=True))
 for primitive in original.get('meshes',[])[mesh]['primitives']:require(set(primitive['attributes'])<={'POSITION','NORMAL','TEXCOORD_0'},'LOD generation requires editable position/normal/UV streams; other attributes need a provider')
 source=model['meshes'][mesh];primitives=[];maximum=0
 for p in source['primitives']:
  result,error=simplify(p,ratio);primitives.append(result);maximum=max(maximum,error)
 source_count=sum(len(p['indices'])//3 for p in source['primitives']);result_count=sum(len(p['indices'])//3 for p in primitives)
 level={**(levels[replacement-1] if replacement else {}),'stale':False,'name':'LOD '+str(replacement or len(levels)+1),'primitives':primitives,'requestedRatio':ratio,'achievedRatio':result_count/source_count,'maximumVertexDisplacement':maximum,'algorithm':'bounded-vertex-clustering-v1'}
 if replacement:levels[replacement-1]=level
 else:levels.append(level)
 return {'mesh':mesh,'lod':replacement or len(levels),'sourceTriangles':source_count,'triangles':result_count,'maximumVertexDisplacement':maximum,'losses':['Approximate static geometry','Preserves source boundary/seam vertices; not a production QEM simplifier']}
def collision(model,options):
 kind=options.get('shape','box');require(kind in ('box','sphere','capsule'),'Unknown collision shape');require(not model['skins'],'Automatic collision fitting currently supports static reference geometry')
 _,world=pose(model);points=[]
 for i,n in enumerate(model['nodes']):
  if 'mesh' not in n:continue
  p,q,s=world[i]
  for primitive in model['meshes'][n['mesh']]['primitives']:
   points.extend(add(p,rotate(q,[x*y for x,y in zip(v,s)])) for v in primitive['positions'])
 require(points,'Model has no geometry');minimum=[min(v[j] for v in points) for j in range(3)];maximum=[max(v[j] for v in points) for j in range(3)];center=mul(add(minimum,maximum),.5);extent=mul(sub(maximum,minimum),.5)
 shapes=model.setdefault('extensions',{}).setdefault(NS,{}).setdefault('collisions',[]);require(len(shapes)<64,'Collision shape limit is 64')
 value={'id':str(uuid.uuid4()),'name':kind.title()+' '+str(len(shapes)+1),'shape':kind,'center':center,'rotation':[0,0,0,1],'enabled':True,'needsReview':False}
 if kind=='box':value['halfExtents']=[max(x,1e-5) for x in extent]
 elif kind=='sphere':value['radius']=max(max(length(sub(v,center)) for v in points),1e-5)
 else:
  value['radius']=max(max(math.hypot(v[0]-center[0],v[2]-center[2]) for v in points),1e-5);value['halfHeight']=extent[1]
 shapes.append(value);return {'collisionId':value['id'],'shape':kind,'space':'asset reference-pose world','losses':['Editor collision geometry; no engine physics cooking']}
def process(target,source,action,options):
 allowed={'lod':{'mesh','ratio','replace'},'collision':{'shape'},'retarget':{'animation','mapping','fps','translationMode','translationScale','name'},'ik':{'animation','root','tip','target','pole','iterations','tolerance','fps','name'},'root_motion':{'animation','root','mode','recordId','axes','rotation','fps','name'},'clip_edit':{'animation','start','end','fps','name'}}
 require(action in allowed and isinstance(options,dict) and set(options)<=allowed[action],'Unknown authoring action or option')
 result=copy.deepcopy(target)
 if action=='lod':summary=generate_lod(result,options)
 elif action=='collision':summary=collision(result,options)
 elif action=='retarget':require(source is not None,'Retargeting needs a loaded source model');summary=retarget(result,source,options)
 elif action in ('ik','root_motion','clip_edit'):
  import animation_tools
  summary=getattr(animation_tools,action)(result,options)
 else:raise ValueError('Unknown authoring action')
 history=result.setdefault('extensions',{}).setdefault(NS,{}).setdefault('history',[]);history.append({'action':action,'sourceAssetId':source['assetId'] if source else target['assetId'],'summary':summary});del history[:-16]
 require(len(__import__('json').dumps(result,ensure_ascii=False,separators=(',',':')).encode())<=2*1024*1024,'Processed model exceeds the 2 MiB profile')
 return result,summary

def select_lod(model,lod):
 require(type(lod) is int and 0<=lod<=4,'LOD index must be 0..4')
 model=copy.deepcopy(model)
 if lod:
  available=model.get('extensions',{}).get(NS,{}).get('lods',{});changed=False
  for i,mesh in enumerate(model['meshes']):
   levels=available.get(str(i),[])
   if len(levels)>=lod:
    require(not levels[lod-1].get('stale',False),'Generated LOD is stale; regenerate it before export');mesh['primitives']=copy.deepcopy(levels[lod-1]['primitives']);changed=True
  require(changed,'Requested LOD is not available')
  metadata=model['extensions'][NS];metadata.pop('lods',None);metadata['exportedLod']=lod
 return model
