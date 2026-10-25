"""Real package roundtrips, malformed inputs, publication safety and converter plans."""
import hashlib
import json
from pathlib import Path
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
import uuid
import warnings
import zipfile

APP = Path(__file__).resolve().parents[1]
ROOT = APP.parent
sys.path.insert(0, str(APP))
from portable_assets import AssetError, package_glb, export_glb, inspect_package, read_package, ConversionRegistry
from portable_assets.glb import inspect_glb, strict_json

FIXTURES = APP / 'examples/portable'

def alter_glb(data, edit):
    length, kind = struct.unpack_from('<II', data, 12)
    doc = json.loads(data[20:20 + length]); edit(doc)
    content = json.dumps(doc, separators=(',', ':')).encode('utf-8'); content += b' ' * (-len(content) % 4)
    remainder = data[20 + length:]
    return struct.pack('<4sII', b'glTF', 2, 20 + len(content) + len(remainder)) + struct.pack('<II', len(content), kind) + content + remainder

class PortableContracts(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='portable-contract-', dir=ROOT / 'data/reports')
        self.workspace = Path(self.temporary.name)
        self.source = FIXTURES / 'StaticTriangle.glb'
        self.target = self.workspace / 'asset.cypack'
    def tearDown(self):
        assert self.workspace.resolve().is_relative_to((ROOT / 'data/reports').resolve())
        self.temporary.cleanup()
    def error(self, code, callback):
        with self.assertRaises(AssetError) as raised: callback()
        self.assertEqual(raised.exception.code, code)
    def pack(self, source=None, **options):
        return package_glb(source or self.source, self.target, name='Example', **options)
    def rewrite(self, edit_manifest=None, edit_data=None, extra=None):
        with zipfile.ZipFile(self.target) as source:
            manifest = json.loads(source.read('manifest.json')); data = source.read('asset.glb')
        if edit_manifest: edit_manifest(manifest)
        if edit_data: data = edit_data(data)
        with zipfile.ZipFile(self.target, 'w', compression=zipfile.ZIP_DEFLATED) as target:
            target.writestr('manifest.json', json.dumps(manifest)); target.writestr('asset.glb', data)
            if extra: target.writestr(*extra)
    def test_01_static_package_schema_identity_and_metadata(self):
        identity = str(uuid.uuid4()); ext = {'org.example.game': {'collision': 'box', 'unknownFields': {'x': [1, 2, 3]}}}
        result = self.pack(asset_id=identity, extensions=ext, provenance={'sourceGame': 'ExampleA'})
        info = inspect_package(self.target); manifest = info['manifest']
        self.assertEqual(manifest['assetId'], identity); self.assertEqual(manifest['extensions'], ext)
        self.assertEqual(manifest['summary']['features'], ['geometry']); self.assertEqual(manifest['space']['linearUnit'], 'meter')
        self.assertEqual(manifest['payload']['sha256'], hashlib.sha256(self.source.read_bytes()).hexdigest())
        self.assertEqual(info['integrity'], 'verified'); self.assertEqual(info['semanticValidation'], 'not-performed'); self.assertTrue(info['warnings'])
        self.assertNotIn(str(self.source), json.dumps(manifest)); self.assertEqual(result['manifest'], manifest)
    def test_02_animation_transfer_between_sdk_roots_preserves_rig_and_bytes(self):
        source = FIXTURES / 'AnimatedTriangle.glb'; a = self.workspace / 'SDK_A/Content'; b = self.workspace / 'SDK_B/Content'; a.mkdir(parents=True); b.mkdir(parents=True)
        original = a / 'Character.cypack'; result = package_glb(source, original, name='Character', extensions={'org.game.a': {'opaque': True}})
        copied = b / original.name; shutil.copyfile(original, copied); output = b / 'Character.glb'; exported = export_glb(copied, output)
        self.assertEqual(output.read_bytes(), source.read_bytes()); self.assertTrue(exported['payloadPreserved']); self.assertEqual(exported['losses'], [])
        manifest = inspect_package(copied)['manifest']; self.assertEqual(manifest['assetId'], result['manifest']['assetId'])
        self.assertEqual(manifest['summary']['skins'], 1); self.assertEqual(manifest['summary']['animations'], 1)
        self.assertEqual(manifest['summary']['features'], ['animation', 'geometry', 'skinning'])
        self.assertEqual(manifest['extensions']['org.game.a'], {'opaque': True})
    def test_03_no_overwrite_and_no_temporary_leaks(self):
        self.target.write_bytes(b'keep'); self.error('DestinationExists', lambda: self.pack()); self.assertEqual(self.target.read_bytes(), b'keep')
        self.target.unlink(); self.pack(); destination = self.workspace / 'existing.glb'; destination.write_bytes(b'keep')
        self.error('DestinationExists', lambda: export_glb(self.target, destination)); self.assertEqual(destination.read_bytes(), b'keep')
        self.assertFalse(list(self.workspace.glob('.cypack-*.tmp')))
    def test_04_racing_destination_is_preserved(self):
        import os
        real_link = os.link
        def race(source, destination):
            Path(destination).write_bytes(b'racing user file'); real_link(source, destination)
        with patch('portable_assets.package.os.link', side_effect=race):
            self.error('DestinationExists', lambda: self.pack())
        self.assertEqual(self.target.read_bytes(), b'racing user file'); self.assertFalse(list(self.workspace.glob('.cypack-*.tmp')))
    def test_05_publication_failure_leaves_no_partial_destination(self):
        with patch('portable_assets.package.os.link', side_effect=OSError('No hard links')):
            self.error('FileAccess', lambda: self.pack())
        self.assertFalse(self.target.exists()); self.assertFalse(list(self.workspace.glob('.cypack-*.tmp')))
    def test_06_corrupt_payload_and_forged_summary_are_rejected(self):
        self.pack(); self.rewrite(edit_data=lambda data: data[:-1] + bytes([data[-1] ^ 1]))
        self.error('ChecksumMismatch', lambda: inspect_package(self.target)); self.target.unlink(); self.pack()
        self.rewrite(edit_manifest=lambda m: m['summary'].update(meshes=99)); self.error('ManifestMismatch', lambda: read_package(self.target))
    def test_07_unknown_versions_boolean_version_and_uuid_rejected(self):
        for version in [2, True, '1']:
            with self.subTest(version=version):
                self.pack(); self.rewrite(edit_manifest=lambda m: m.update(schemaVersion=version)); self.error('UnsupportedVersion', lambda: read_package(self.target)); self.target.unlink()
        self.error('InvalidManifest', lambda: self.pack(asset_id='bad-id'))
        self.assertFalse(self.target.exists())
        self.error('InvalidManifest', lambda: self.pack(asset_id=''))
    def test_08_unexpected_traversal_duplicate_and_symlink_entries_rejected(self):
        for name in ['../escape.glb', '/escape.glb', 'textures/image.png']:
            self.pack(); self.rewrite(extra=(name, b'payload')); self.error('InvalidArchive', lambda: inspect_package(self.target)); self.target.unlink()
        self.pack()
        with warnings.catch_warnings():
            warnings.simplefilter('ignore'); self.rewrite(extra=('manifest.json', b'{}'))
        self.error('InvalidArchive', lambda: inspect_package(self.target)); self.target.unlink(); self.pack()
        manifest, data = read_package(self.target)
        with zipfile.ZipFile(self.target, 'w') as archive:
            archive.writestr('manifest.json', json.dumps(manifest)); info = zipfile.ZipInfo('asset.glb'); info.create_system = 3; info.external_attr = (stat.S_IFLNK | 0o777) << 16; archive.writestr(info, data)
        self.error('InvalidArchive', lambda: inspect_package(self.target)); self.assertFalse((self.workspace.parent / 'escape.glb').exists())
    def test_09_truncated_archive_and_glb_framing_rejected(self):
        self.pack(); self.target.write_bytes(self.target.read_bytes()[:-12]); self.error('InvalidArchive', lambda: read_package(self.target))
        data = self.source.read_bytes()
        for broken in [data[:-1], b'xxxx' + data[4:], data[:4] + struct.pack('<I', 1) + data[8:]]:
            self.error('InvalidGlb', lambda: inspect_glb(broken))
    def test_10_uri_policy_and_buffer_bounds(self):
        data = self.source.read_bytes()
        for uri in ['https://example.invalid/buffer.bin', '../outside.bin', 'data:application/octet-stream;base64,AA==']:
            broken = alter_glb(data, lambda doc: doc['buffers'][0].update(uri=uri)); self.error('ExternalResource', lambda: inspect_glb(broken))
        broken = alter_glb(data, lambda doc: doc['bufferViews'][0].update(byteLength=99999999)); self.error('InvalidGlb', lambda: inspect_glb(broken))
        broken = alter_glb(data, lambda doc: doc['bufferViews'][0].update(buffer=False)); self.error('InvalidGlb', lambda: inspect_glb(broken))
    def test_11_size_limits_before_parsing(self):
        big = self.workspace / 'big.glb'
        with big.open('wb') as stream: stream.truncate(64 * 1024 * 1024 + 1)
        self.error('SizeLimit', lambda: self.pack(big))
        with zipfile.ZipFile(self.target, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            archive.writestr('manifest.json', b' ' * (128 * 1024 + 1)); archive.writestr('asset.glb', b'')
        self.error('SizeLimit', lambda: inspect_package(self.target))
    def test_12_json_duplicates_non_finite_and_namespaced_metadata(self):
        for value in [b'{"x":1,"x":2}', b'{"x":NaN}', b'{"x":1e999}']:
            self.error('InvalidJson', lambda: strict_json(value))
        self.error('InvalidManifest', lambda: self.pack(extensions={'game': {'x': 1}}))
        self.error('InvalidManifest', lambda: self.pack(extensions={'org.game': {'x': float('nan')}}))
        self.error('InvalidManifest', lambda: self.pack(provenance={'sourcePath': 'private/path'}))
    def test_13_unknown_required_extensions_preserved_but_game_plan_blocks(self):
        source = self.workspace / 'extended.glb'; source.write_bytes(alter_glb(self.source.read_bytes(), lambda doc: doc.update(extensionsUsed=['VENDOR_test'], extensionsRequired=['VENDOR_test'])))
        self.pack(source); registry = ConversionRegistry(); plan = registry.plan_export(self.target)
        self.assertEqual(plan['status'], 'ready'); self.assertTrue(plan['executionAvailable'])
        registry.register({'id': 'game.static', 'name': 'Game Static', 'targetFormat': 'game.mesh', 'supportedFeatures': ['geometry'], 'supportedExtensions': []})
        plan = registry.plan_export(self.target, 'game.static'); self.assertEqual(plan['status'], 'blocked'); self.assertEqual(plan['unsupportedRequiredExtensions'], ['VENDOR_test'])
        output = self.workspace / 'extended-output.glb'; export_glb(self.target, output); self.assertEqual(output.read_bytes(), source.read_bytes())
    def test_14_static_target_blocks_skinning_and_animation(self):
        self.pack(FIXTURES / 'AnimatedTriangle.glb'); registry = ConversionRegistry(); registry.register({'id': 'game.static', 'name': 'Static only', 'targetFormat': 'game.mesh', 'supportedFeatures': ['geometry'], 'supportedExtensions': []})
        plan = registry.plan_export(self.target, 'game.static'); self.assertEqual(plan['unsupportedFeatures'], ['animation', 'skinning']); self.assertFalse(plan['executionAvailable']); self.assertFalse(plan['silentLossAllowed'])
    def test_15_declaration_does_not_implement_a_game_converter(self):
        self.pack(); registry = ConversionRegistry(); profile = {'id': 'game.static', 'name': 'Static', 'targetFormat': 'game.mesh', 'supportedFeatures': ['geometry'], 'supportedExtensions': []}; registry.register(profile)
        profile['supportedFeatures'].clear(); plan = registry.plan_export(self.target, 'game.static')
        self.assertTrue(plan['declaredCompatible']); self.assertEqual(plan['status'], 'provider-required'); self.assertFalse(plan['executionAvailable']); self.assertTrue(plan['requiresProviderValidation'])
        self.error('InvalidConverter', lambda: registry.register(registry.descriptors()[0])); self.error('UnknownConverter', lambda: registry.plan_export(self.target, 'missing'))
    def test_16_utf8_cli_roundtrip_and_structured_errors(self):
        cli = [sys.executable, str(APP / 'portable_asset_cli.py')]
        def run(*arguments):
            result = subprocess.run([*cli, *map(str, arguments)], capture_output=True, encoding='utf-8', timeout=10); return result.returncode, json.loads(result.stdout)
        code, result = run('pack', self.source, self.target, '--name', 'Piece_模型'); self.assertEqual(code, 0); self.assertEqual(result['result']['manifest']['name'], 'Piece_模型')
        code, result = run('inspect', self.target); self.assertEqual(code, 0); self.assertEqual(result['result']['integrity'], 'verified')
        output = self.workspace / 'export.glb'; code, result = run('export-glb', self.target, output); self.assertEqual(code, 0); self.assertEqual(output.read_bytes(), self.source.read_bytes())
        code, result = run('export-glb', self.target, output); self.assertEqual(code, 1); self.assertEqual(result['error']['code'], 'DestinationExists')
        code, result = run('plan', self.target); self.assertEqual(code, 0); self.assertTrue(result['result']['executionAvailable'])

    def portable_runtime(self):
        from cytools_core import Runtime
        from portable_assets.service import operation_descriptors, register_operations
        sources = self.workspace / 'shared-inputs'; sources.mkdir(exist_ok=True)
        manifest = json.loads((APP / 'CyTool.json').read_text('utf-8')); manifest['operations'] = operation_descriptors()
        runtime = Runtime(manifest, self.workspace / 'private-jobs'); register_operations(runtime, sources)
        return runtime, sources
    def test_17_real_core_worker_pack_privacy_export_and_task_cleanup(self):
        from cytools_core import CyToolError
        runtime, sources = self.portable_runtime(); shutil.copyfile(self.source, sources / 'input.glb')
        with runtime:
            actor = runtime.authenticate(runtime.create_client('Portable owner')['token']); other = runtime.authenticate(runtime.create_client('Other client')['token']); session = runtime.create_session(actor)
            submitted = runtime.submit(actor, session['sessionId'], 'portable_pack', {'sourcePath': 'input.glb', 'name': 'SDK Mesh'})
            job = runtime.wait(actor, submitted['jobId']); self.assertEqual(job['state'], 'Completed', job)
            output = Path(job['outputs']['outputPath']); self.assertTrue(output.is_relative_to(Path(job['workspace']))); self.assertTrue(output.is_file())
            with self.assertRaises(CyToolError): runtime.get_job(other, job['jobId'])
            deadline = time.monotonic() + 5
            while True:
                try: exported = runtime.export_job(actor, job['jobId'], self.workspace / 'persistent-export'); break
                except CyToolError as error:
                    if error.code != 'Busy' or time.monotonic() > deadline: raise
                    time.sleep(.02)
            runtime.finish_task(actor, session['sessionId']); self.assertFalse(output.exists())
            preserved = Path(exported['outputs']['outputPath']); self.assertTrue(preserved.is_file()); self.assertEqual(inspect_package(preserved)['manifest']['name'], 'SDK Mesh')
    def test_18_real_core_inspection_and_glb_export(self):
        runtime, sources = self.portable_runtime(); shutil.copyfile(FIXTURES / 'AnimatedTriangle.cypack', sources / 'input.cypack')
        with runtime:
            actor = runtime.authenticate(runtime.create_client('Portable')['token']); session = runtime.create_session(actor, temporary=False)
            for operation in ['portable_inspect', 'portable_export_glb']:
                submitted = runtime.submit(actor, session['sessionId'], operation, {'sourcePath': 'input.cypack'}); job = runtime.wait(actor, submitted['jobId']); self.assertEqual(job['state'], 'Completed', job)
                if operation == 'portable_inspect': self.assertEqual(job['outputs']['manifest']['summary']['animations'], 1)
                else: self.assertEqual(Path(job['outputs']['outputPath']).read_bytes(), (FIXTURES / 'AnimatedTriangle.glb').read_bytes())
    def test_19_real_core_rejects_input_escape_and_invalid_glb(self):
        runtime, sources = self.portable_runtime(); (self.workspace / 'outside.glb').write_bytes(self.source.read_bytes()); (sources / 'invalid.glb').write_bytes(b'not glb')
        with runtime:
            actor = runtime.authenticate(runtime.create_client('Portable')['token']); session = runtime.create_session(actor, temporary=False)
            for source in ['../outside.glb', 'invalid.glb']:
                submitted = runtime.submit(actor, session['sessionId'], 'portable_pack', {'sourcePath': source, 'name': 'Invalid'}); job = runtime.wait(actor, submitted['jobId']); self.assertEqual(job['state'], 'Failed', job)
                self.assertFalse((Path(job['workspace']) / 'outputs/asset.cypack').exists())
    def test_20_worker_rejects_output_escape_and_alias(self):
        from portable_asset_worker import run
        workspace = self.workspace / 'worker'; workspace.mkdir(); sources = self.workspace / 'inputs'; sources.mkdir(); source = sources / 'source.glb'; shutil.copyfile(self.source, source)
        parameters = {'action': 'portable_pack', 'sourcePath': str(source), 'inputRoot': str(sources), 'parameters': {'name': 'Example'}, 'reportPath': str(workspace / 'report.json'), 'outputPath': str(self.workspace / 'escape.cypack')}
        payload = {'protocolVersion': '1.0', 'workspace': str(workspace), 'parameters': parameters}
        self.error('InvalidOutput', lambda: run(payload)); self.assertFalse((self.workspace / 'escape.cypack').exists())
        parameters['outputPath'] = parameters['reportPath']; self.error('InvalidOutput', lambda: run(payload)); self.assertEqual(source.read_bytes(), self.source.read_bytes())
    def test_21_shared_inputs_cannot_expose_private_runtime_storage(self):
        from cytools_core import Runtime
        from portable_assets.service import operation_descriptors, register_operations
        manifest = json.loads((APP / 'CyTool.json').read_text('utf-8')); manifest['operations'] = operation_descriptors(); runtime = Runtime(manifest, self.workspace / 'private-jobs')
        try:
            with self.assertRaises(ValueError): register_operations(runtime, self.workspace)
        finally: runtime.close()

    def test_22_optional_extensions_require_explicit_loss_review(self):
        source = self.workspace / 'optional.glb'; source.write_bytes(alter_glb(self.source.read_bytes(), lambda doc: doc.update(extensionsUsed=['VENDOR_optional'])))
        self.pack(source); registry = ConversionRegistry(); registry.register({'id': 'game.static', 'name': 'Static', 'targetFormat': 'game.mesh', 'supportedFeatures': ['geometry'], 'supportedExtensions': []})
        plan = registry.plan_export(self.target, 'game.static'); self.assertEqual(plan['unsupportedRequiredExtensions'], []); self.assertEqual(plan['unsupportedOptionalExtensions'], ['VENDOR_optional']); self.assertEqual(plan['status'], 'review-required'); self.assertTrue(plan['requiresLossReview']); self.assertFalse(plan['executionAvailable'])
        self.assertEqual(registry.plan_export(self.target)['status'], 'ready')

if __name__ == '__main__': unittest.main(verbosity=2)
