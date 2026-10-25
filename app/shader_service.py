"""Preview compiler jobs use the installed CyToolsCore scheduler and worker contract."""
import json,sys
from pathlib import Path
from cytools_core import CyToolError
from cytools_core.workers import ProcessWorker

def register_shader_operation(runtime,editor_call,worker=None):
    worker=worker or ProcessWorker([sys.executable,str(Path(__file__).with_name('shader_worker.py'))],idle_timeout=120)
    runtime.register_worker('material:preview',worker)
    def execute(context,parameters):
        context.progress(5,currentStep='Resolving material')
        plan=editor_call('material_prepare',parameters,context.cancellation)
        plan_path=context.path('inputs/material-plan.json');plan_path.write_text(json.dumps(plan,ensure_ascii=False),encoding='utf-8')
        context.progress(30,currentStep='Compiling preview shader')
        result=worker.run(context,{'planPath':str(plan_path),'sourcePath':str(context.path('outputs/material.hlsl')),'shaderPath':str(context.path('outputs/material.dxbc')),'reportPath':str(context.path('outputs/shader-report.json'))})
        context.cancellation.check()
        if not result['success']:raise CyToolError('ShaderCompileFailed','Preview shader compilation failed.',details=result)
        context.progress(100,currentStep='Preview shader ready');return result
    runtime.register('material_compile',execute)
