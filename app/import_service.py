"""Trusted importer registry; CyToolsCore owns workers, progress and cancellation."""
import json,sys
from pathlib import Path
from cytools_core import CyToolError
from cytools_core.workers import ProcessWorker

class ImportProvider:
    def __init__(self,id,command):self.id=id;self.worker=ProcessWorker(command,idle_timeout=120)

def register_import_operations(runtime,editor_call,providers=None):
    providers={key:ImportProvider(key,[sys.executable,str(Path(__file__).with_name('import_worker.py'))]) for key in ('image.copy','scene3d.glb')} if providers is None else dict(providers)
    for id,provider in providers.items():runtime.register_worker('import:'+id,provider.worker)
    def execute(context,parameters,reimport=False):
        context.progress(5,currentStep='Validating import')
        plan=editor_call('asset_import_prepare',{'path':parameters['path'],'reimport':True} if reimport else parameters,context.cancellation)
        provider=providers.get(plan['importerId'])
        if provider is None:raise CyToolError('MissingImplementation','Register a trusted source import worker for '+plan['importerId'])
        plan_path=context.path('inputs/import-plan.json');plan_path.write_text(json.dumps(plan,ensure_ascii=False),'utf-8')
        context.progress(25,currentStep='Staging asset')
        output=provider.worker.run(context,{'planPath':str(plan_path),'stagedPath':str(context.path('outputs/asset'+Path(plan['destination']).suffix)),'reportPath':str(context.path('outputs/import-report.json'))})
        context.cancellation.check();context.progress(85,currentStep='Committing import')
        # The commit is a short persistent transaction. A cancellation after this
        # boundary may leave the explicit user import committed; inspect destination.
        result=editor_call('asset_import_commit',{'reportPath':output['reportPath']},context.cancellation)
        context.progress(100,currentStep='Asset ready');return result
    runtime.register('asset_import',lambda c,p:execute(c,p))
    runtime.register('asset_reimport',lambda c,p:execute(c,p,True))
