"""Bounded self-contained GLB profile checks, not a full glTF semantic validator."""
import json
import math
import struct

MAX_GLB = 64 * 1024 * 1024
MAX_JSON = 4 * 1024 * 1024

class AssetError(ValueError):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code

def fail(code, message):
    raise AssetError(code, message)

def strict_json(data):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                fail('InvalidJson', 'Duplicate JSON property: ' + key)
            result[key] = value
        return result
    def constant(value):
        fail('InvalidJson', 'Non-finite JSON number: ' + value)
    try:
        value = json.loads(data.decode('utf-8'), object_pairs_hook=pairs, parse_constant=constant)
        pending = [(value, 0)]
        while pending:
            item, depth = pending.pop()
            if depth > 64:
                fail('InvalidJson', 'JSON nesting exceeds 64 levels')
            if isinstance(item, float) and not math.isfinite(item):
                fail('InvalidJson', 'Non-finite JSON number')
            if isinstance(item, dict):
                pending.extend((v, depth + 1) for v in item.values())
            elif isinstance(item, list):
                pending.extend((v, depth + 1) for v in item)
        return value
    except (UnicodeError, json.JSONDecodeError, RecursionError) as exc:
        raise AssetError('InvalidJson', 'Invalid bounded UTF-8 JSON') from exc

def integer(value, minimum=0):
    return isinstance(value, int) and not isinstance(value, bool) and value >= minimum

def inspect_glb(data):
    if not 20 <= len(data) <= MAX_GLB:
        fail('SizeLimit', 'GLB must be between 20 bytes and 64 MiB')
    magic, version, size = struct.unpack_from('<4sII', data)
    if magic != b'glTF' or version != 2 or size != len(data):
        fail('InvalidGlb', 'Expected a complete glTF 2 GLB header')
    offset = 12
    chunks = []
    while offset < len(data):
        if offset + 8 > len(data):
            fail('InvalidGlb', 'Truncated GLB chunk header')
        length, kind = struct.unpack_from('<II', data, offset)
        offset += 8
        if length % 4 or offset + length > len(data):
            fail('InvalidGlb', 'Unaligned or truncated GLB chunk')
        chunks.append((kind, offset, length))
        offset += length
    if not chunks or chunks[0][0] != 0x4E4F534A or len(chunks) > 2:
        fail('UnsupportedGlbProfile', 'Profile requires JSON followed by at most one BIN chunk')
    if len(chunks) == 2 and chunks[1][0] != 0x004E4942:
        fail('UnsupportedGlbProfile', 'Unsupported GLB chunk type')
    if chunks[0][2] > MAX_JSON:
        fail('SizeLimit', 'GLB JSON exceeds 4 MiB')
    _, start, length = chunks[0]
    doc = strict_json(data[start:start + length])
    if not isinstance(doc, dict) or not isinstance(doc.get('asset'), dict) or doc['asset'].get('version') != '2.0':
        fail('UnsupportedVersion', 'Expected glTF asset.version 2.0')
    if doc['asset'].get('minVersion', '2.0') != '2.0':
        fail('UnsupportedVersion', 'Unsupported glTF minimum version')
    # This profile resolves no URIs, including data URIs or extension-specific URIs.
    pending = [doc]
    while pending:
        item = pending.pop()
        if isinstance(item, dict):
            if 'uri' in item:
                fail('ExternalResource', 'Portable profile forbids URI properties; embed resources in BIN bufferViews')
            pending.extend(item.values())
        elif isinstance(item, list):
            pending.extend(item)
    def objects(key):
        items = doc.get(key, [])
        if not isinstance(items, list) or len(items) > 100000 or any(not isinstance(v, dict) for v in items):
            fail('InvalidGlb', 'Invalid or oversized glTF array: ' + key)
        return items
    arrays = {key: objects(key) for key in ['buffers', 'bufferViews', 'accessors', 'nodes', 'scenes', 'meshes', 'skins', 'animations', 'materials', 'textures', 'images']}
    buffers = arrays['buffers']
    binary_size = chunks[1][2] if len(chunks) == 2 else 0
    if len(buffers) > 1:
        fail('UnsupportedGlbProfile', 'Only one embedded buffer is supported')
    if buffers:
        size = buffers[0].get('byteLength')
        if not integer(size, 1) or not size <= binary_size <= size + 3:
            fail('InvalidGlb', 'Embedded buffer length does not match BIN chunk')
    elif binary_size:
        fail('InvalidGlb', 'BIN chunk has no declared buffer')
    for view in arrays['bufferViews']:
        begin, length = view.get('byteOffset', 0), view.get('byteLength')
        if not integer(view.get('buffer')) or view.get('buffer') != 0 or not buffers or not integer(begin) or not integer(length, 1) or begin + length > buffers[0]['byteLength']:
            fail('InvalidGlb', 'bufferView escapes the embedded buffer')
    required = doc.get('extensionsRequired', [])
    used = doc.get('extensionsUsed', [])
    for extensions in [required, used]:
        if not isinstance(extensions, list) or len(extensions) > 64 or any(not isinstance(v, str) or not 1 <= len(v) <= 128 for v in extensions) or len(set(extensions)) != len(extensions):
            fail('InvalidGlb', 'Invalid extension declarations')
    if not set(required) <= set(used):
        fail('InvalidGlb', 'Required extensions must also be declared used')
    features = []
    for key, feature in [('meshes', 'geometry'), ('skins', 'skinning'), ('animations', 'animation'), ('materials', 'materials'), ('textures', 'textures')]:
        if arrays[key]:
            features.append(feature)
    primitives = []
    for mesh in arrays['meshes']:
        values = mesh.get('primitives', [])
        if not isinstance(values, list) or any(not isinstance(v, dict) for v in values):
            fail('InvalidGlb', 'Invalid primitive declaration')
        primitives.extend(values)
    if any(p.get('targets') for p in primitives):
        features.append('morphTargets')
    return {'profile': 'self-contained-glb-v1', 'meshes': len(arrays['meshes']),
            'skins': len(arrays['skins']), 'animations': len(arrays['animations']),
            'nodes': len(arrays['nodes']), 'primitives': len(primitives),
            'features': sorted(features), 'requiredExtensions': sorted(required), 'usedExtensions': sorted(used),
            'validationScope': 'glb-framing-resource-bounds-and-declarations',
            'semanticValidation': 'not-performed'}
