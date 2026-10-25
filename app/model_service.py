import json,sys
from pathlib import Path
from cytools_core.workers import ProcessWorker

def register_model_operations(runtime,editor_call):
    worker=ProcessWorker([sys.executable,str(Path(__file__).with_name('model_worker.py'))],idle_timeout=120)
    runtime.register_worker('scene3d-export',worker)
    def execute(context,p):
        context.progress(5,currentStep='Preparing immutable 3D snapshot')
        model=editor_call('model_export_prepare',{'modelId':p['modelId']},context.cancellation)['model']
        snapshot=context.path('inputs/model.cy3d');snapshot.write_text(json.dumps(model,ensure_ascii=False,separators=(',',':')),'utf-8')
        output=context.path('outputs/asset.'+p['format'])
        context.progress(20,currentStep='Exporting portable 3D asset')
        result=worker.run(context,{'snapshot':str(snapshot),'output':str(output),'lod':p.get('lod',0)})
        context.progress(100,currentStep='3D asset ready');return result
    runtime.register('model_export',execute)
