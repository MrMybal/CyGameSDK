"""Optional operations for an existing CyToolsCore runtime/manifest, not a server."""
from pathlib import Path
import sys
from cytools_core import CyToolError
from cytools_core.workers import ProcessWorker

OPERATION_IDS = frozenset(['portable_pack', 'portable_inspect', 'portable_export_glb'])

def operation_descriptors():
    def obj(properties, required):
        return {'type': 'object', 'properties': properties, 'required': required, 'additionalProperties': False}
    source = {'type': 'string', 'minLength': 1, 'maxLength': 1024}
    definitions = [('portable_pack', 'Package portable 3D asset', {'sourcePath': source, 'name': {'type': 'string', 'minLength': 1, 'maxLength': 128}, 'assetId': {'type': 'string', 'format': 'uuid'}, 'extensions': {'type': 'object'}, 'provenance': {'type': 'object'}}, ['sourcePath', 'name']),
                   ('portable_inspect', 'Inspect portable 3D asset', {'sourcePath': source}, ['sourcePath']),
                   ('portable_export_glb', 'Export portable GLB payload', {'sourcePath': source}, ['sourcePath'])]
    return [{'id': identity, 'name': name, 'description': name + '. Reads a configured shared input root; outputs are private Core job files.',
             'inputSchema': obj(properties, required), 'outputSchema': {'type': 'object'},
             'supportedBackends': [], 'resourceRequirements': {'ramBytes': 512 * 1024 * 1024},
             'permissions': [], 'canRunAsync': True, 'canCancel': True, 'supportsProgress': True,
             'exclusiveGroups': ['portable-assets']} for identity, name, properties, required in definitions]

def register_operations(runtime, input_root):
    """Declare operation_descriptors first; exclude these IDs from native proxies."""
    root = Path(input_root).resolve(strict=True)
    runtime_root = runtime.root.resolve()
    if root.is_relative_to(runtime_root) or runtime_root.is_relative_to(root):
        raise ValueError('Keep the shared input root disjoint from private Core runtime storage')
    if not root.is_dir():
        raise ValueError('Configure a dedicated portable input directory')
    worker = ProcessWorker([sys.executable, str(Path(__file__).resolve().parents[1] / 'portable_asset_worker.py')], idle_timeout=120)
    runtime.register_worker('portable-assets', worker)
    def execute(context, parameters, operation):
        context.progress(5, currentStep='Checking portable input')
        source = (root / parameters['sourcePath']).resolve()
        if not source.is_relative_to(root) or source == root or not source.is_file():
            raise CyToolError('InvalidParameters', 'Portable source must be a file inside the configured input directory')
        request = {'action': operation, 'sourcePath': str(source), 'inputRoot': str(root),
                   'parameters': {key: value for key, value in parameters.items() if key != 'sourcePath'},
                   'reportPath': str(context.path('outputs/portable-report.json'))}
        if operation != 'portable_inspect':
            request['outputPath'] = str(context.path('outputs/asset.cypack' if operation == 'portable_pack' else 'outputs/asset.glb'))
        context.progress(15, currentStep='Processing portable asset')
        result = worker.run(context, request)
        context.cancellation.check()
        context.progress(100, currentStep='Portable result ready')
        return result
    for operation in sorted(OPERATION_IDS):
        runtime.register(operation, lambda context, parameters, operation=operation: execute(context, parameters, operation))
