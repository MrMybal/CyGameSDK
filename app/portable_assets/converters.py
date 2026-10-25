"""Declarative game conversion planning. Only GLB identity export executes today."""
import copy
import json
import re
from .glb import fail, strict_json
from .package import inspect_package

FEATURES = frozenset(['geometry', 'skinning', 'animation', 'materials', 'textures', 'morphTargets'])
IDENTITY = 'gltf.glb.identity'

class ConversionRegistry:
    def __init__(self):
        self._profiles = {IDENTITY: {'id': IDENTITY, 'name': 'Original GLB payload',
            'targetFormat': 'model/gltf-binary', 'supportedFeatures': sorted(FEATURES),
            'supportedExtensions': [], 'preservesOpaquePayload': True}}

    def register(self, descriptor):
        try:
            value = strict_json(json.dumps(descriptor, allow_nan=False).encode('utf-8'))
        except (ValueError, TypeError, RecursionError) as exc:
            fail('InvalidConverter', 'Converter descriptor must be finite JSON')
        keys = {'id', 'name', 'targetFormat', 'supportedFeatures', 'supportedExtensions'}
        if not isinstance(value, dict) or set(value) != keys:
            fail('InvalidConverter', 'Declare id/name/targetFormat and supported feature/extension lists')
        if any(not isinstance(value.get(key), str) or not 1 <= len(value[key]) <= 128 for key in ['id', 'name', 'targetFormat']):
            fail('InvalidConverter', 'Invalid converter identity')
        if not re.fullmatch(r'[a-z][a-z0-9_.-]*', value['id']) or value['id'] in self._profiles:
            fail('InvalidConverter', 'Converter ID is invalid or already registered')
        for key in ['supportedFeatures', 'supportedExtensions']:
            values = value[key]
            if not isinstance(values, list) or len(values) > 64 or any(not isinstance(v, str) or not 1 <= len(v) <= 128 for v in values) or len(values) != len(set(values)):
                fail('InvalidConverter', 'Invalid converter feature/extension list')
        if not set(value['supportedFeatures']) <= FEATURES:
            fail('InvalidConverter', 'Unknown converter feature')
        value['preservesOpaquePayload'] = False
        self._profiles[value['id']] = value

    def descriptors(self):
        return copy.deepcopy(list(self._profiles.values()))

    def plan_export(self, source, converter_id=IDENTITY):
        if converter_id not in self._profiles:
            fail('UnknownConverter', 'Register the target converter capability profile first')
        info = inspect_package(source)
        summary = info['manifest']['summary']
        profile = self._profiles[converter_id]
        missing = sorted(set(summary['features']) - set(profile['supportedFeatures']))
        extensions = [] if profile['preservesOpaquePayload'] else sorted(set(summary['requiredExtensions']) - set(profile['supportedExtensions']))
        optional = [] if profile['preservesOpaquePayload'] else sorted(set(summary['usedExtensions']) - set(summary['requiredExtensions']) - set(profile['supportedExtensions']))
        compatible = not missing and not extensions
        identity = converter_id == IDENTITY
        return {'assetId': info['manifest']['assetId'], 'converterId': converter_id,
            'targetFormat': profile['targetFormat'], 'declaredCompatible': compatible,
            'unsupportedFeatures': missing, 'unsupportedRequiredExtensions': extensions,
            'unsupportedOptionalExtensions': optional, 'requiresLossReview': bool(missing or extensions or optional),
            'status': 'blocked' if not compatible else 'review-required' if optional else 'ready' if identity else 'provider-required',
            'executionAvailable': identity and compatible and not optional,
            'requiresProviderValidation': not identity, 'silentLossAllowed': False,
            'warnings': info['warnings']}
