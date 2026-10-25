"""File utilities for the portable contract; not a second MCP or job runtime."""
import argparse
import json
import sys
from .package import AssetError, bounded_read, inspect_package, package_glb, export_glb
from .glb import strict_json
from .converters import ConversionRegistry

def main(argv=None):
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description='CyGame portable GLB packages, schema v1 (experimental)')
    commands = parser.add_subparsers(dest='action', required=True)
    pack = commands.add_parser('pack', help='Package an embedded-resource GLB without changing its bytes')
    pack.add_argument('source'); pack.add_argument('destination'); pack.add_argument('--name', required=True)
    pack.add_argument('--asset-id'); pack.add_argument('--metadata', help='JSON object with optional extensions/provenance')
    inspect = commands.add_parser('inspect'); inspect.add_argument('source')
    export = commands.add_parser('export-glb'); export.add_argument('source'); export.add_argument('destination')
    plan = commands.add_parser('plan'); plan.add_argument('source'); plan.add_argument('--profile', help='A declarative game converter JSON profile')
    args = parser.parse_args(argv)
    try:
        if args.action == 'pack':
            metadata = strict_json(bounded_read(args.metadata, 128 * 1024)) if args.metadata else {}
            if not isinstance(metadata, dict) or set(metadata) - {'extensions', 'provenance'}:
                raise AssetError('InvalidManifest', 'Metadata accepts only extensions and provenance')
            result = package_glb(args.source, args.destination, name=args.name, asset_id=args.asset_id, **metadata)
        elif args.action == 'inspect':
            result = inspect_package(args.source)
        elif args.action == 'export-glb':
            result = export_glb(args.source, args.destination)
        else:
            registry = ConversionRegistry()
            converter_id = 'gltf.glb.identity'
            if args.profile:
                descriptor = strict_json(bounded_read(args.profile, 128 * 1024)); registry.register(descriptor); converter_id = descriptor['id']
            result = registry.plan_export(args.source, converter_id)
        print(json.dumps({'ok': True, 'result': result}, ensure_ascii=False, allow_nan=False))
        return 0
    except AssetError as exc:
        print(json.dumps({'ok': False, 'error': {'code': exc.code, 'message': str(exc)}}, ensure_ascii=False))
        return 1
