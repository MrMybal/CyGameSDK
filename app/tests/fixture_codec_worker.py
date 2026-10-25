"""Synthetic codec fixture only: not a real game's file format."""
import hashlib,json,struct,sys
from pathlib import Path
p=json.load(sys.stdin);args=p['parameters'];root=Path(p['workspace']).resolve()
def owned(key):
 path=Path(args[key]).resolve()
 if not path.is_relative_to(root):raise ValueError('Path escapes owned fixture job')
 return path
model=json.loads(owned('snapshot').read_text('utf-8'));payload=b'CYSDKTEST'+struct.pack('<I',len(model['nodes']))+model['assetId'].encode('ascii')
# Real decode/roundtrip check for this synthetic test format only.
assert payload[:9]==b'CYSDKTEST' and struct.unpack_from('<I',payload,9)[0]==len(model['nodes']) and payload[13:].decode()==model['assetId']
output=owned('output');output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(payload)
report={'format':'cygame-codec-result','schemaVersion':1,'codecId':args['codecId'],'gameId':args['gameId'],'gameVersion':args['gameVersion'],'platform':args['platform'],'assetId':model['assetId'],'sha256':hashlib.sha256(payload).hexdigest(),'bytes':len(payload),'losses':['Synthetic fixture stores only node count and identity'],'validation':{'status':'verified','method':'Synthetic fixture decoder roundtrip; no game compatibility claim'}}
if args.get('options',{}).get('corruptReport'):report['sha256']='0'*64
if args.get('options',{}).get('unverified'):report['validation']['status']='unverified'
owned('report').write_text(json.dumps(report),'utf-8');print(json.dumps({'outputPath':str(output),'reportPath':str(owned('report'))}))
