"""Official Core ProcessWorker for bounded model processing."""
import json,sys
from pathlib import Path
from portable_assets.glb import strict_json
from model_authoring import process

def run(request):
 if request.get('protocolVersion')!='1.0':raise ValueError('Unsupported worker protocol')
 root=Path(request['workspace']).resolve();p=request['parameters']
 def owned(key):
  path=Path(p[key]).resolve()
  if path==root or not path.is_relative_to(root):raise ValueError('Worker path escapes its job')
  return path
 snapshot=owned('snapshot');output=owned('report')
 if snapshot.stat().st_size>3*1024*1024:raise ValueError('Authoring snapshot exceeds 3 MiB')
 plan=strict_json(snapshot.read_bytes());parameters=plan['parameters'];model,summary=process(plan['model'],plan.get('source'),parameters['action'],parameters.get('options',{}))
 output.parent.mkdir(parents=True,exist_ok=True)
 report={'format':'cygame-model-process','schemaVersion':1,'modelId':parameters['modelId'],'baseHash':plan['baseHash'],'sourceId':parameters.get('sourceModelId',''),'sourceHash':plan.get('sourceHash',''),'model':model,'summary':summary}
 with output.open('x',encoding='utf-8') as stream:json.dump(report,stream,ensure_ascii=False,allow_nan=False,separators=(',',':'))
 return {'reportPath':str(output),'summary':summary}
if __name__=='__main__':
 try:print(json.dumps(run(json.load(sys.stdin))))
 except Exception as exc:print(str(exc),file=sys.stderr);raise SystemExit(1)
