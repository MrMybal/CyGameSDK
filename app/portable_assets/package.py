"""CyGame .cypack v1: a bounded manifest plus an unchanged, self-contained GLB."""
import hashlib
import json
import os
from pathlib import Path
import stat
import tempfile
import uuid
import zipfile
import zlib
from jsonschema import Draft202012Validator, FormatChecker
from .glb import AssetError, fail, inspect_glb, integer, strict_json, MAX_GLB

MAX_MANIFEST = 128 * 1024
MAX_PACKAGE = 80 * 1024 * 1024
SPACE = {'handedness': 'right', 'upAxis': '+Y', 'linearUnit': 'meter', 'rotation': 'quaternion-xyzw', 'timeUnit': 'second'}
SCHEMA = Path(__file__).resolve().parents[1] / 'schemas/portable-asset.schema.json'
_VALIDATOR = Draft202012Validator(json.loads(SCHEMA.read_text('utf-8-sig')), format_checker=FormatChecker())

def validate_manifest(value):
    if not isinstance(value, dict) or value.get('format') != 'cygame-portable-asset':
        fail('InvalidManifest', 'Expected a CyGame portable asset manifest')
    if not integer(value.get('schemaVersion'), 1) or value['schemaVersion'] != 1:
        fail('UnsupportedVersion', 'Only portable asset schema version 1 is supported')
    errors = sorted(_VALIDATOR.iter_errors(value), key=lambda e: str(list(e.path)))
    if errors:
        fail('InvalidManifest', errors[0].message)
    if str(uuid.UUID(value['assetId'])) != value['assetId']:
        fail('InvalidManifest', 'assetId must be a canonical lowercase UUID')
    return value

def bounded_read(path, limit):
    try:
        with Path(path).open('rb') as stream:
            if os.fstat(stream.fileno()).st_size > limit:
                fail('SizeLimit', 'Input file exceeds its size limit')
            data = stream.read(limit + 1)
        if len(data) > limit:
            fail('SizeLimit', 'Input file exceeds its size limit')
        return data
    except OSError as exc:
        raise AssetError('FileAccess', 'Cannot read input file: ' + str(path)) from exc

def publish(destination, write):
    """Publish with a same-directory hard link: atomic creation, never replace."""
    destination = Path(destination).absolute()
    if destination.exists() or destination.is_symlink():
        fail('DestinationExists', 'Destination already exists')
    temporary = None
    try:
        destination.parent.mkdir(parents=True, exist_ok=True)
        handle, name = tempfile.mkstemp(prefix='.cypack-', suffix='.tmp', dir=destination.parent)
        os.close(handle)
        temporary = Path(name)
        write(temporary)
        # The exclusive link also protects against a destination created while writing.
        os.link(temporary, destination)
    except FileExistsError as exc:
        raise AssetError('DestinationExists', 'Destination appeared during publication') from exc
    except OSError as exc:
        raise AssetError('FileAccess', 'Cannot publish file; same-volume hard links must be supported') from exc
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return destination

def package_glb(source, destination, *, name, asset_id=None, extensions=None, provenance=None):
    data = bounded_read(source, MAX_GLB)
    summary = inspect_glb(data)
    manifest = {'format': 'cygame-portable-asset', 'schemaVersion': 1,
                'assetId': str(uuid.uuid4()) if asset_id is None else asset_id, 'name': name, 'kind': 'scene3d',
                'space': dict(SPACE),
                'payload': {'path': 'asset.glb', 'mediaType': 'model/gltf-binary',
                            'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()},
                'summary': summary, 'extensions': {} if extensions is None else extensions,
                'provenance': {} if provenance is None else provenance}
    # Validate JSON-compatible metadata and finite numbers, as well as schema fields.
    try:
        encoded = json.dumps(manifest, ensure_ascii=False, allow_nan=False, sort_keys=True, separators=(',', ':')).encode('utf-8')
    except (ValueError, TypeError, RecursionError) as exc:
        raise AssetError('InvalidManifest', 'Manifest metadata is not finite JSON') from exc
    if len(encoded) > MAX_MANIFEST:
        fail('SizeLimit', 'Manifest metadata exceeds 128 KiB')
    manifest = validate_manifest(strict_json(encoded))
    def write(path):
        with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for member, value in [('manifest.json', encoded), ('asset.glb', data)]:
                info = zipfile.ZipInfo(member, date_time=(1980, 1, 1, 0, 0, 0))
                info.create_system = 3
                info.external_attr = (stat.S_IFREG | 0o600) << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, value)
    publish(destination, write)
    return {'manifest': manifest, 'outputPath': str(Path(destination).absolute()),
            'warnings': ['Full glTF semantic validation has not been performed.']}

def read_package(source):
    try:
        with Path(source).open('rb') as stream:
            if os.fstat(stream.fileno()).st_size > MAX_PACKAGE:
                fail('SizeLimit', 'Portable package exceeds 80 MiB')
            with zipfile.ZipFile(stream) as archive:
                entries = archive.infolist()
                if len(entries) != 2 or {e.filename for e in entries} != {'manifest.json', 'asset.glb'}:
                    fail('InvalidArchive', 'Package must contain exactly manifest.json and asset.glb')
                for entry in entries:
                    mode = entry.external_attr >> 16
                    if entry.is_dir() or stat.S_ISLNK(mode) or stat.S_IFMT(mode) not in (0, stat.S_IFREG) or entry.flag_bits & 1 or entry.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
                        fail('InvalidArchive', 'Unsupported archive entry type or compression')
                    limit = MAX_MANIFEST if entry.filename == 'manifest.json' else MAX_GLB
                    if not 0 <= entry.file_size <= limit:
                        fail('SizeLimit', 'Archive entry exceeds its size limit')
                manifest = validate_manifest(strict_json(archive.read('manifest.json')))
                data = archive.read('asset.glb')
    except AssetError:
        raise
    except (OSError, zipfile.BadZipFile, RuntimeError, NotImplementedError, EOFError, zlib.error) as exc:
        raise AssetError('InvalidArchive', 'Cannot read a complete portable asset package') from exc
    payload = manifest['payload']
    if len(data) != payload['bytes'] or hashlib.sha256(data).hexdigest() != payload['sha256']:
        fail('ChecksumMismatch', 'Portable payload size or SHA-256 does not match')
    if inspect_glb(data) != manifest['summary']:
        fail('ManifestMismatch', 'Portable summary does not match the GLB declarations')
    return manifest, data

def inspect_package(source):
    manifest, _ = read_package(source)
    return {'manifest': manifest, 'integrity': 'verified',
            'validationScope': manifest['summary']['validationScope'],
            'semanticValidation': 'not-performed',
            'warnings': ['Container checks do not establish full glTF or game compatibility.']}

def export_glb(source, destination):
    manifest, data = read_package(source)
    publish(destination, lambda path: path.write_bytes(data))
    return {'assetId': manifest['assetId'], 'outputPath': str(Path(destination).absolute()),
            'bytes': len(data), 'sha256': manifest['payload']['sha256'], 'payloadPreserved': True,
            'losses': [], 'conversion': 'byte-preserving-glb-export'}
