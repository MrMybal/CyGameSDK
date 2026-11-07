---
id: sdk-contracts
title: SDK schemas and failures
description: SDK schemas and failures
section: SDK
order: 52
revision: 1
parent: automation
---

## Inputs and outputs {#schemas}
Operation inputs and outputs use JSON Schema Draft 2020-12. JSON types are `string`, `boolean`, `integer`, `number`, `object` and `array`; `file` and `float` are not JSON types. `x-cy-kind` describes editor controls (File, Directory, Files, Image, Enum, Object, Array); it does not implement a decoder. `x-cy-ui` help/advanced/hidden/runtimeGenerated/step/visibleWhen are presentation hints. Use schema constraints or handler checks for validity; `multipleOf` validates a step, `x-cy-ui.step` only suggests one.

Defaults are copied through properties/items/allOf and the selected if branch before validation. No arbitrary default is selected between anyOf/oneOf. Bundle local `$defs`; remote references are refused. `format` is an annotation, not proof that a date, URL or file exists. A malformed handler result fails with `InvalidOutput`; descriptor `outputs` adds file/media metadata without replacing `outputSchema`. Write artifacts with `context.path('outputs/result.ext')`, within the job workspace. Server paths do not automatically transfer bytes to a remote client.

## Structured failures {#errors}
`CyToolError` exposes code, message, technicalDetails, recoverable and suggestedAction. Preserve unfamiliar error codes. A rejected SubmitJob creates no job; a failing handler produces a Failed job with error. A successful GetJob request for that Failed job does not mean the operation succeeded. Unexpected handler exceptions are InternalError.

Common codes include InvalidDescriptor, IncompatibleVersion, InvalidParameters, InvalidOutput, MissingImplementation, UnknownOperation, MissingDependency, MissingRuntime, UnsupportedPlatform, AccessDenied, AuthenticationRequired, Busy, Timeout, Cancelled, Interrupted, WorkerCrashed, ProtocolError, IntegrityFailure and InternalError. Resource/backend/model codes describe optional providers only when those are actually installed. Logs and technicalDetails may contain user data: redact secrets at the provider boundary.

## Manifest identity {#manifest}
The shipped `app/CyTool.json` is the operation catalogue for this version. Its schemaVersion, protocolVersion, id, version, operations, platforms and concurrency describe the Tool. IDs start with an ASCII letter, then letters/digits/underscore/dot/dash, at most 128 characters. Unknown manifest fields are preserved; business parameters may prohibit unknown fields. An incompatible major version fails. Optional executable metadata does not launch that file. Keep developer paths, tokens and accounts out of distributed catalogues.

Read [all editor operations](doc:operations), [temporary tasks](doc:jobs) and [transport protocol](doc:protocol). Native document Undo is separate from job cancellation.
