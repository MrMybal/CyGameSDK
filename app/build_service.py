"""Source build providers. Scheduling, cancellation and workers belong to CyToolsCore."""
import json,sys
from pathlib import Path
from cytools_core import CyToolError
from cytools_core.workers import ProcessWorker

class BuildProvider:
    def __init__(self,provider_id,command):
        self.id=provider_id
        self.worker=ProcessWorker(command,idle_timeout=120)


def default_providers():
    return {'sandbox.archive':BuildProvider('sandbox.archive',[sys.executable,str(Path(__file__).with_name('package_worker.py'))])}


def register_build_operation(runtime,editor_call,providers=None):
    providers=default_providers() if providers is None else dict(providers)
    for id,provider in providers.items():runtime.register_worker('build:'+id,provider.worker)
    def execute(context,parameters):
        context.progress(5,currentStep='Validating build request')
        plan=editor_call('build_prepare',parameters,context.cancellation)
        context.path('diagnostics/plan.json').write_text(json.dumps(plan,ensure_ascii=False,indent=2),encoding='utf-8')
        if not plan['valid']:raise CyToolError('InvalidBuild','The document has build errors.',details=plan['diagnostics'])
        provider_id=plan['target']['providerId'];provider=providers.get(provider_id)
        if provider is None:raise CyToolError('MissingImplementation','Register a trusted source provider for '+provider_id)
        snapshot=context.path('inputs/build-snapshot.json')
        snapshot.write_text(json.dumps(plan['snapshot'],ensure_ascii=False,indent=2),encoding='utf-8')
        context.progress(40,currentStep='Preparing immutable document snapshot')
        request={'snapshotPath':str(snapshot),'targetId':plan['target']['id'],'configuration':plan['configuration'],'options':plan['options'],
            'archivePath':str(context.path('outputs/package.zip')),'reportPath':str(context.path('outputs/package-report.json'))}
        context.progress(60,currentStep='Running build provider')
        result=provider.worker.run(context,request)
        context.cancellation.check();context.progress(100,currentStep='Package complete')
        return result
    runtime.register('build_execute',execute)
