"""Trusted source codec registry. This foundation ships no real game codec."""
import hashlib,json,sys
from pathlib import Path
from jsonschema import Draft202012Validator
from cytools_core import CyToolError
from cytools_core.workers import ProcessWorker

class GameCodecProvider:
 def __init__(self,descriptor,command):
  required={'id','gameId','versions','platforms','extension','optionsSchema','maxBytes'}
  if not isinstance(descriptor,dict) or not required<=descriptor.keys():raise ValueError('Codec descriptor is incomplete')
  if not descriptor['versions'] or not descriptor['platforms']:raise ValueError('Declare exact game versions and platforms')
  extension=descriptor['extension']
  if not isinstance(extension,str) or not extension.startswith('.') or len(extension)>16 or not extension[1:].isalnum():raise ValueError('Codec extension must be a simple suffix')
  if not isinstance(descriptor['maxBytes'],int) or not 0<descriptor['maxBytes']<=64*1024*1024:raise ValueError('Codec export cap is 64 MiB')
  Draft202012Validator.check_schema(descriptor['optionsSchema']);self.descriptor=json.loads(json.dumps(descriptor));self.worker=ProcessWorker(command,idle_timeout=120)

# Extend this source factory in a derived SDK, or pass game_codecs to create_runtime.
# Never load commands/modules from user asset JSON or project metadata.
def default_codecs():return {}

def register_game_codec_operations(runtime,editor_call,providers=None):
 codecs=default_codecs() if providers is None else dict(providers)
 for key,provider in codecs.items():
  if key!=provider.descriptor['id']:raise ValueError('Codec registry identity mismatch')
  runtime.register_worker('game-codec:'+key,provider.worker)
 runtime.register('game_codec_list',lambda context,p:{'codecs':[c.descriptor for _,c in sorted(codecs.items())],'gameCompatibility':'Only registered, verified providers implement real game formats'})
 def execute(context,p):
  if p['codecId'] not in codecs:raise CyToolError('MissingImplementation','No trusted source codec registered for '+p['codecId'])
  codec=codecs[p['codecId']];d=codec.descriptor
  if p['gameVersion'] not in d['versions'] or p['platform'] not in d['platforms']:raise CyToolError('UnsupportedTarget','Codec does not support this exact game version/platform')
  options=p.get('options',{});errors=list(Draft202012Validator(d['optionsSchema']).iter_errors(options))
  if errors:raise CyToolError('InvalidParameters',errors[0].message)
  model=editor_call('model_export_prepare',{'modelId':p['modelId']},context.cancellation)['model']
  meta=model.get('extensions',{}).get('org.cygame.editor',{});required_losses=[]
  if any(level.get('stale',False) for levels in meta.get('lods',{}).values() for level in levels):required_losses.append('Stale generated LODs require review')
  if any(shape.get('needsReview',False) for shape in meta.get('collisions',[])):required_losses.append('Collision fit requires review')
  if set(required_losses)-set(p.get('acceptedLosses',[])):raise CyToolError('ConversionLoss','Explicit review required: '+', '.join(required_losses))
  context.progress(10,currentStep='Preparing game codec snapshot')
  snapshot=context.path('inputs/asset.cy3d');snapshot.write_text(json.dumps(model,ensure_ascii=False,separators=(',',':')),'utf-8')
  output=context.path('outputs/asset'+d['extension']);report=context.path('outputs/codec-report.json')
  result=codec.worker.run(context,{'snapshot':str(snapshot),'output':str(output),'report':str(report),'codecId':d['id'],'gameId':d['gameId'],'gameVersion':p['gameVersion'],'platform':p['platform'],'options':options})
  context.cancellation.check();context.progress(85,currentStep='Checking codec output and loss report')
  root=context.path('outputs').resolve()
  for name,path in [('outputPath',output),('reportPath',report)]:
   candidate=Path(result[name]).resolve()
   if candidate!=path.resolve() or not candidate.is_relative_to(root) or candidate.is_symlink():raise CyToolError('InvalidOutput','Codec output/report must be owned by this job')
  if not output.is_file() or output.stat().st_size>d['maxBytes'] or not report.is_file() or report.stat().st_size>128*1024:raise CyToolError('InvalidOutput','Codec output/report exceeds its limits')
  value=json.loads(report.read_text('utf-8-sig'));digest=hashlib.sha256(output.read_bytes()).hexdigest()
  expected={'format':'cygame-codec-result','schemaVersion':1,'codecId':d['id'],'gameId':d['gameId'],'gameVersion':p['gameVersion'],'platform':p['platform'],'assetId':model['assetId'],'sha256':digest,'bytes':output.stat().st_size}
  if any(value.get(k)!=v for k,v in expected.items()):raise CyToolError('InvalidOutput','Codec report does not match target, identity or output bytes')
  losses=value.get('losses');validation=value.get('validation')
  if not isinstance(losses,list) or any(not isinstance(x,str) or len(x)>1024 for x in losses) or len(losses)>64:raise CyToolError('InvalidOutput','Codec must declare bounded conversion losses')
  accepted=p.get('acceptedLosses',[])
  if set(losses)-set(accepted):raise CyToolError('ConversionLoss','Explicitly acknowledge the reported losses before exporting: '+', '.join(losses))
  if not isinstance(validation,dict) or validation.get('status') not in ('verified','unverified') or not isinstance(validation.get('method'),str) or not validation['method']:raise CyToolError('InvalidOutput','Codec must describe its actual target validation')
  if p.get('requireVerified',True) and validation['status']!='verified':raise CyToolError('UnverifiedOutput','This codec has not verified its output for the requested target')
  context.progress(100,currentStep='Codec output ready')
  return {**value,'outputPath':str(output),'reportPath':str(report),'verificationAuthority':'trusted source provider; generic framework checks ownership/identity/hashes, not the game'}
 runtime.register('game_codec_export',execute)
