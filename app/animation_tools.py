"""Bounded offline animation tools. No engine runtime or physics bindings."""
import copy,hashlib,json,math,struct,uuid
from model_authoring import NS,require,add,sub,mul,dot,cross,length,unit,qm,qi,rotate,pose,sample

def digest(clip):
 # Fingerprint the supported pose data after float32 quantization, as in GLB.
 # Display names/unknown metadata do not change the extracted motion semantics.
 f32=lambda value:struct.unpack('<f',struct.pack('<f',value))[0]
 value=[{'node':t['node'],'path':t['path'],'interpolation':t['interpolation'],'times':[f32(x) for x in t['times']],'values':[[f32(x) for x in v] for v in t['values']]} for t in clip['tracks']]
 return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':'),allow_nan=False).encode()).hexdigest()
def finite(value,minimum,maximum,label):
 require(type(value) in (int,float) and math.isfinite(value) and minimum<=value<=maximum,label)
 return value

def vector(value,label):
 require(isinstance(value,list) and len(value)==3,label)
 return [finite(x,-1e6,1e6,label) for x in value]

def clip_at(model,index,reference=False):
 require(type(index) is int and ((reference and index==-1) or 0<=index<len(model['animations'])),'Choose an existing clip or reference pose')
 if index<0:return None
 clip=model['animations'][index];require(clip['tracks'],'Source clip has no tracks');return clip

def time_grid(clip,options,channels):
 fps=finite(options.get('fps',30),1,120,'Bake rate must be 1..120 FPS')
 low=min(t['times'][0] for t in clip['tracks']) if clip else 0
 high=max(t['times'][-1] for t in clip['tracks']) if clip else 0
 start=finite(options.get('start',low),low,high,'Start must be within source range')
 end=finite(options.get('end',high),start,high,'End must follow start within source range')
 count=math.ceil((end-start)*fps) if end>start else 0
 require((count+1)*channels<=20000,'Bake exceeds 20,000 keys; reduce range, FPS or chain length')
 return ([start+(end-start)*i/count for i in range(count+1)] if count else [start]),fps

def track(node,path,times,values):return {'node':node,'path':path,'interpolation':'LINEAR','times':times,'values':values}
def continuous(values):
 out=[]
 for q in values:
  q=unit(q)
  if out and dot(out[-1],q)<0:q=mul(q,-1)
  out.append(q)
 return out

def append_clip(model,clip):
 require(len(model['animations'])<128,'Animation clip limit is 128')
 require(isinstance(clip['name'],str) and 0<len(clip['name'].encode())<=512,'Clip name must be 1..512 UTF-8 bytes')
 total=sum(len(t['times']) for c in model['animations'] for t in c['tracks'])+sum(len(t['times']) for t in clip['tracks'])
 require(total<=20000,'Loaded animation keys exceed 20,000')
 model['animations'].append(clip);return len(model['animations'])-1

def clip_edit(model,options):
 clip=clip_at(model,options.get('animation',0));times,fps=time_grid(clip,options,len(clip['tracks']));relative=[t-times[0] for t in times];out=copy.deepcopy(clip)
 out['name']=options.get('name',clip['name'][:100]+'_Trimmed');out['tracks']=[]
 for t in clip['tracks']:
  values=[sample(t,time) for time in times]
  if t['path']=='rotation':values=continuous(values)
  replacement={**copy.deepcopy(t),**track(t['node'],t['path'],relative,values)};out['tracks'].append(replacement)
 index=append_clip(model,out)
 return {'animation':index,'samples':len(times),'fps':fps,'sourceRange':[times[0],times[-1]],'duration':relative[-1],'losses':['Selected range resampled to LINEAR keys; original clip retained']}

def from_to(a,b):
 a=unit(a);b=unit(b);c=max(-1,min(1,dot(a,b)))
 if c>1-1e-12:return [0,0,0,1]
 if c<-1+1e-10:
  basis=min(([1,0,0],[0,1,0],[0,0,1]),key=lambda v:abs(dot(v,a)))
  return [*unit(cross(a,basis)),0]
 return unit([*cross(a,b),1+c])

def fabrik(points,target,iterations,tolerance,pole=None):
 """Single unconstrained fixed-root chain; lengths never stretch."""
 points=copy.deepcopy(points);root=points[0][:];segments=[length(sub(b,a)) for a,b in zip(points,points[1:])]
 require(all(x>1e-7 for x in segments),'IK chain contains a zero-length bone')
 distance=length(sub(target,root));minimum=max(0,2*max(segments)-sum(segments));reachable=minimum-tolerance<=distance<=sum(segments)+tolerance
 if distance>=sum(segments):
  direction=unit(sub(target,root))
  for i,span in enumerate(segments):points[i+1]=add(points[i],mul(direction,span))
  return points,reachable,0
 # A perfectly straight chain has no bending direction. Seed a deterministic plane.
 axis=unit(sub(points[-1],root)) if length(sub(points[-1],root))>1e-8 else [0,1,0]
 perpendicular=sub(pole,root) if pole is not None else min(([1,0,0],[0,1,0],[0,0,1]),key=lambda v:abs(dot(v,axis)))
 perpendicular=sub(perpendicular,mul(axis,dot(perpendicular,axis)))
 if length(perpendicular)<1e-8:perpendicular=cross(axis,[1,0,0] if abs(axis[0])<.9 else [0,1,0])
 if max(length(cross(sub(p,root),axis)) for p in points)<1e-8:
  for i in range(1,len(points)-1):points[i]=add(points[i],mul(unit(perpendicular),sum(segments)*.03*math.sin(math.pi*i/(len(points)-1))))
 def direction(a,b,fallback):
  v=sub(b,a);return unit(v) if length(v)>1e-12 else fallback
 for iteration in range(iterations):
  points[-1]=target[:]
  for i in reversed(range(len(segments))):points[i]=add(points[i+1],mul(direction(points[i+1],points[i],[0,1,0]),segments[i]))
  points[0]=root[:]
  for i,span in enumerate(segments):points[i+1]=add(points[i],mul(direction(points[i],points[i+1],[0,1,0]),span))
  if pole is not None:
   for i in range(1,len(points)-1):
    axis_vector=sub(points[i+1],points[i-1])
    if length(axis_vector)<1e-10:continue
    ax=unit(axis_vector);base=points[i-1];center=add(base,mul(ax,dot(sub(points[i],base),ax)));v=sub(points[i],center);aim=sub(sub(pole,center),mul(ax,dot(sub(pole,center),ax)))
    if length(v)>1e-10 and length(aim)>1e-10:points[i]=add(center,mul(unit(aim),length(v)))
  if length(sub(points[-1],target))<=tolerance:break
 return points,reachable,iteration+1

def ik(model,options):
 root=options.get('root');tip=options.get('tip');nodes=model['nodes']
 require(type(root) is int and type(tip) is int and 0<=root<len(nodes) and 0<=tip<len(nodes),'Choose valid chain root and tip nodes')
 chain=[tip]
 while chain[-1]!=root and nodes[chain[-1]]['parent']>=0 and len(chain)<33:chain.append(nodes[chain[-1]]['parent'])
 require(chain[-1]==root and 3<=len(chain)<=32,'IK requires a direct ancestor chain of 3..32 nodes');chain.reverse()
 target=vector(options.get('target'),'IK target must be a finite world-space vector');pole=vector(options['pole'],'IK pole must be a finite world-space vector') if 'pole' in options else None
 iterations=options.get('iterations',64);require(type(iterations) is int and 1<=iterations<=128,'IK iterations must be 1..128');tolerance=finite(options.get('tolerance',.001),1e-6,.1,'IK tolerance must be 0.000001..0.1')
 clip=clip_at(model,options.get('animation',-1),True);times,fps=time_grid(clip,options,len(chain)-1);ancestors=set(chain)
 parent=nodes[root]['parent']
 while parent>=0:ancestors.add(parent);parent=nodes[parent]['parent']
 values={i:[] for i in chain[:-1]};max_error=0;unreachable=0;unconverged=0;maximum_iterations=0
 for time in times:
  current,world=pose(model,clip,time)
  require(all(all(abs(s-1)<1e-6 for s in current[i]['scale']) for i in ancestors),'IK chain and ancestors require unit scale, including animated scale')
  solved,reachable,used=fabrik([world[i][0] for i in chain],target,iterations,tolerance,pole);maximum_iterations=max(maximum_iterations,used);unreachable+=not reachable
  parent_q=world[nodes[root]['parent']][1] if nodes[root]['parent']>=0 else [0,0,0,1]
  for a,b,pa,pb in zip(chain,chain[1:],solved,solved[1:]):
   starting_q=unit(qm(parent_q,current[a]['rotation']));original_direction=rotate(starting_q,current[b]['translation']);rotation=unit(qm(from_to(original_direction,sub(pb,pa)),starting_q));local=unit(qm(qi(parent_q),rotation));current[a]['rotation']=local;values[a].append(local);parent_q=rotation
  _,actual=pose({'nodes':current});error=length(sub(actual[tip][0],target));max_error=max(max_error,error);unconverged+=reachable and error>tolerance*1.01
 output=copy.deepcopy(clip) if clip else {'tracks':[]};output['name']=options.get('name',(clip['name'][:100] if clip else 'Reference')+'_IK')
 # Bake all original channels when trimming; preserve untouched originals otherwise.
 replacing={(i,'rotation') for i in values};trimmed='start' in options or 'end' in options
 if trimmed:
  output['tracks']=[{**copy.deepcopy(t),**track(t['node'],t['path'],times,[sample(t,x) for x in times])} for t in output['tracks']]
 output['tracks']=[t for t in output['tracks'] if (t['node'],t['path']) not in replacing]
 for i in chain[:-1]:output['tracks'].append(track(i,'rotation',times,continuous(values[i])))
 index=append_clip(model,output)
 return {'animation':index,'algorithm':'FABRIK fixed-root position-only','chain':chain,'target':target,'samples':len(times),'fps':fps,'maximumError':max_error,'unreachableSamples':unreachable,'unconvergedSamples':unconverged,'maximumIterations':maximum_iterations,'losses':['Unconstrained position-only IK; no joint limits, contacts or physics; original clip retained']}

def motion_records(model):return model.setdefault('extensions',{}).setdefault(NS,{}).setdefault('rootMotion',[])
def root_motion(model,options):
 mode=options.get('mode','extract');require(mode in ('extract','restore'),'Unknown root-motion mode')
 require(set(options)<=({'mode','recordId','name'} if mode=='restore' else {'mode','animation','root','axes','rotation','fps','name'}),'Options do not match root-motion mode')
 if mode=='restore':
  record=next((r for r in motion_records(model) if r['id']==options.get('recordId')),None);require(record is not None,'Choose an extracted motion record')
  inplace=clip_at(model,record['inPlace']);motion=clip_at(model,record['motion']);require(digest(inplace)==record['inPlaceHash'] and digest(motion)==record['motionHash'],'Motion clips changed; extract a new pair before reconstructing')
  root=record['root'];require(model['nodes'][root]['parent']==-1,'Motion root was reparented; extract again')
  times=motion['tracks'][0]['times'];translations=[];rotations=[]
  for time in times:
   locked,_=pose(model,inplace,time);delta,_=pose(model,motion,time);q=unit(qm(delta[root]['rotation'],locked[root]['rotation']));rotations.append(q);translations.append(add(delta[root]['translation'],rotate(delta[root]['rotation'],locked[root]['translation'])))
  out=copy.deepcopy(inplace);out['name']=options.get('name',inplace['name'][:100]+'_Restored');out['tracks']=[t for t in out['tracks'] if not(t['node']==root and t['path'] in ('translation','rotation'))]
  out['tracks'] += [track(root,'translation',times,translations),track(root,'rotation',times,continuous(rotations))];index=append_clip(model,out)
  return {'animation':index,'recordId':record['id'],'samples':len(times),'losses':['Reconstructs baked root motion samples; no engine movement binding']}
 clip=clip_at(model,options.get('animation',0));root=options.get('root',0);require(type(root) is int and 0<=root<len(model['nodes']) and model['nodes'][root]['parent']==-1,'Extraction requires a top-level TRS root node')
 axes=options.get('axes',[True,False,True]);require(isinstance(axes,list) and len(axes)==3 and all(type(x) is bool for x in axes),'Axes must be three booleans')
 extract_rotation=options.get('rotation',False);require(type(extract_rotation) is bool and (any(axes) or extract_rotation),'Select translation axes or rotation to extract')
 times,fps=time_grid(clip,options,5);first,_=pose(model,clip,times[0]);lock=first[root];locked_t=[];locked_q=[];deltas=[];delta_q=[];scales=[]
 for time in times:
  nodes,_=pose(model,clip,time);n=nodes[root];p=[lock['translation'][i] if axes[i] else n['translation'][i] for i in range(3)];q=lock['rotation'] if extract_rotation else n['rotation'];dq=unit(qm(unit(n['rotation']),qi(unit(q))));dp=sub(n['translation'],rotate(dq,p));locked_t.append(p);locked_q.append(q);deltas.append(dp);delta_q.append(dq);scales.append(n['scale'])
 inplace=copy.deepcopy(clip);inplace['name']=options.get('name',clip['name'][:100]+'_InPlace');inplace['tracks']=[t for t in inplace['tracks'] if t['node']!=root]
 inplace['tracks'] += [track(root,'translation',times,locked_t),track(root,'rotation',times,continuous(locked_q)),track(root,'scale',times,scales)]
 curve={'name':inplace['name'][:100]+'_Motion','tracks':[track(root,'translation',times,deltas),track(root,'rotation',times,continuous(delta_q))]}
 records=motion_records(model);require(len(records)<32,'At most 32 extracted motion pairs')
 a=append_clip(model,inplace);b=append_clip(model,curve);record={'id':str(uuid.uuid4()),'source':options.get('animation',0),'root':root,'inPlace':a,'motion':b,'inPlaceHash':digest(inplace),'motionHash':digest(curve),'axes':axes,'rotation':extract_rotation,'space':'asset world rigid delta','fps':fps}
 records.append(record);distance=sum(length(sub(b,a)) for a,b in zip(deltas,deltas[1:]))
 return {'animation':a,'motionAnimation':b,'recordId':record['id'],'samples':len(times),'distance':distance,'losses':['Root channels baked to LINEAR; extracted rigid delta is editor data, not engine movement; original clip retained']}
