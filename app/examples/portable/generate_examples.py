"""Generate original minimal GLB fixtures; no third-party game data."""
import argparse
import json
import math
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from portable_assets import package_glb

def glb_fixture(rigged=False):
    binary = bytearray()
    views, accessors = [], []
    def add(values, component_type, kind, count, bounds=None, target=None):
        while len(binary) % 4: binary.append(0)
        offset = len(binary)
        formats = {5126: 'f', 5123: 'H', 5121: 'B'}
        block = struct.pack('<' + formats[component_type] * len(values), *values)
        binary.extend(block)
        view = {'buffer': 0, 'byteOffset': offset, 'byteLength': len(block)}
        if target is not None: view['target'] = target
        views.append(view)
        accessor = {'bufferView': len(views) - 1, 'componentType': component_type, 'count': count, 'type': kind}
        if bounds: accessor.update(min=bounds[0], max=bounds[1])
        accessors.append(accessor)
        return len(accessors) - 1
    positions = add([-.5, 0, 0, .5, 0, 0, 0, 1, 0], 5126, 'VEC3', 3, ([-.5, 0, 0], [.5, 1, 0]), 34962)
    normals = add([0, 0, 1] * 3, 5126, 'VEC3', 3, target=34962)
    indices = add([0, 1, 2], 5123, 'SCALAR', 3, target=34963)
    primitive = {'attributes': {'POSITION': positions, 'NORMAL': normals}, 'indices': indices}
    doc = {'asset': {'version': '2.0', 'generator': 'CyGameSDK portable contract fixtures'},
           'scene': 0, 'scenes': [{'nodes': [0]}], 'nodes': [{'name': 'Triangle', 'mesh': 0}],
           'meshes': [{'name': 'Triangle', 'primitives': [primitive]}]}
    if rigged:
        joints = add([0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0], 5121, 'VEC4', 3, target=34962)
        weights = add([1, 0, 0, 0] * 3, 5126, 'VEC4', 3, target=34962)
        primitive['attributes'].update(JOINTS_0=joints, WEIGHTS_0=weights)
        identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
        inverse_tip = list(identity); inverse_tip[13] = -1
        binds = add(identity + inverse_tip, 5126, 'MAT4', 2)
        times = add([0, .5, 1], 5126, 'SCALAR', 3, ([0], [1]))
        rotations = add([0, 0, 0, 1, 0, 0, math.sin(math.pi / 8), math.cos(math.pi / 8), 0, 0, 0, 1], 5126, 'VEC4', 3)
        doc['nodes'][0]['skin'] = 0
        doc['nodes'] += [{'name': 'Root', 'children': [2]}, {'name': 'Tip', 'translation': [0, 1, 0]}]
        doc['scenes'][0]['nodes'].append(1)
        doc['skins'] = [{'name': 'TwoBoneRig', 'skeleton': 1, 'joints': [1, 2], 'inverseBindMatrices': binds}]
        doc['animations'] = [{'name': 'Swing', 'samplers': [{'input': times, 'output': rotations, 'interpolation': 'LINEAR'}],
                              'channels': [{'sampler': 0, 'target': {'node': 2, 'path': 'rotation'}}]}]
    doc.update(buffers=[{'byteLength': len(binary)}], bufferViews=views, accessors=accessors)
    content = json.dumps(doc, separators=(',', ':'), allow_nan=False).encode('utf-8')
    content += b' ' * (-len(content) % 4)
    binary += b'\0' * (-len(binary) % 4)
    size = 12 + 8 + len(content) + 8 + len(binary)
    return struct.pack('<4sII', b'glTF', 2, size) + struct.pack('<II', len(content), 0x4E4F534A) + content + struct.pack('<II', len(binary), 0x004E4942) + binary

def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--output', required=True); args = parser.parse_args()
    output = Path(args.output); output.mkdir(parents=True, exist_ok=True)
    for name, rigged, identity in [('StaticTriangle', False, 'dd87a798-a9e9-4d22-b099-d920cc5d0319'), ('AnimatedTriangle', True, 'ef7603e1-84f8-44b2-87da-a677412c462a')]:
        glb = output / (name + '.glb')
        with glb.open('xb') as stream: stream.write(glb_fixture(rigged))
        package_glb(glb, output / (name + '.cypack'), name=name, asset_id=identity,
                    extensions={'org.cygame.examples': {'originalFixture': True}},
                    provenance={'sourceFormat': 'glTF 2 GLB', 'converterId': 'gltf.glb.identity'})
        print(name)
if __name__ == '__main__': main()
