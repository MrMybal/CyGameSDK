"""Official Core one-shot worker protocol for portable asset file operations."""
import json
from pathlib import Path
import sys
from portable_assets import AssetError, package_glb, inspect_package, export_glb

def run(payload):
    if payload.get('protocolVersion') != '1.0':
        raise AssetError('InvalidProtocol', 'Expected ProcessWorker protocol 1.0')
    workspace = Path(payload['workspace']).resolve()
    request = payload['parameters']; action = request['action']
    root = Path(request['inputRoot']).resolve(strict=True)
    if root.is_relative_to(workspace) or workspace.is_relative_to(root):
        raise AssetError('InvalidSource', 'Keep shared inputs disjoint from the job workspace')
    source = Path(request['sourcePath']).resolve(strict=True)
    if not source.is_relative_to(root) or source == root or not source.is_file():
        raise AssetError('InvalidSource', 'Source escapes the configured input directory')
    def owned(key):
        path = Path(request[key]).resolve()
        if not path.is_relative_to(workspace) or path == workspace:
            raise AssetError('InvalidOutput', 'Portable outputs must belong to the Core job workspace')
        return path
    report = owned('reportPath')
    if report == source or ('outputPath' in request and (owned('outputPath') == report or owned('outputPath') == source)):
        raise AssetError('InvalidOutput', 'Input, output and report paths must be independent')
    if action == 'portable_pack':
        parameters = request['parameters']; options = {key: value for key, value in parameters.items() if key in ('name', 'extensions', 'provenance')}
        if 'assetId' in parameters: options['asset_id'] = parameters['assetId']
        result = package_glb(source, owned('outputPath'), **options)
    elif action == 'portable_inspect':
        result = inspect_package(source)
    elif action == 'portable_export_glb':
        result = export_glb(source, owned('outputPath'))
    else:
        raise AssetError('InvalidOperation', 'Unsupported portable worker operation')
    report.parent.mkdir(parents=True, exist_ok=True)
    with report.open('x', encoding='utf-8') as stream:
        stream.write(json.dumps(result, ensure_ascii=False, allow_nan=False, indent=2))
    return {**result, 'reportPath': str(report)}

if __name__ == '__main__':
    try:
        print(json.dumps(run(json.load(sys.stdin)), allow_nan=False))
    except Exception as exc:
        print(str(exc).encode('ascii', 'backslashreplace').decode('ascii'), file=sys.stderr)
        raise SystemExit(1)
