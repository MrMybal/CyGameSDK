"""Model authoring uses the existing Core jobs, identities and cancellation."""
import json,sys
from pathlib import Path
from cytools_core.workers import ProcessWorker

def register_authoring_operations(runtime,editor_call):
 worker=ProcessWorker([sys.executable,str(Path(__file__).with_name('model_process_worker.py'))],idle_timeout=120)
 runtime.register_worker('model-authoring',worker)
 def execute(context,p):
  context.progress(5,currentStep='Validating model snapshot')
  plan=editor_call('model_process_prepare',p,context.cancellation)
  snapshot=context.path('inputs/authoring.json');snapshot.write_text(json.dumps(plan,ensure_ascii=False,separators=(',',':')),'utf-8')
  context.progress(20,currentStep='Processing neutral asset')
  result=worker.run(context,{'snapshot':str(snapshot),'report':str(context.path('outputs/authoring-result.json'))})
  context.cancellation.check();context.progress(85,currentStep='Applying validated authoring result')
  applied=editor_call('model_process_commit',{'reportPath':result['reportPath']},context.cancellation)
  context.progress(100,currentStep='Authoring edit ready');return {**applied,'summary':result['summary'],'reportPath':result['reportPath']}
 runtime.register('model_process',execute)
