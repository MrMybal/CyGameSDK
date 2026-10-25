# Portable asset examples

These are original generated data, not files from a game:

- StaticTriangle.glb / .cypack: one static triangle with normals.
- AnimatedTriangle.glb / .cypack: one skinned triangle, two joints and one quaternion clip.
- Example_StaticOnlyConverter.json: declarative target profile, with no game encoder.

Read [CommonAssetFormat.md](../../Docs/CommonAssetFormat.md). The packages preserve
GLB bytes and namespaced metadata. They are CLI/SDK contract fixtures. Native 0.21 and later import the bounded
GLB/CyPack profile into editable .cy3d documents with animation playback; see
[Native3DAssets.md](../../Docs/Native3DAssets.md) for the supported profile and limits.

Both GLBs passed Khronos validator 2.0.0-dev.3.10 with zero errors/warnings. Verify
with `node app/tests/validate_portable_fixtures.cjs` after installing that optional
QA package as documented in the format guide. Regenerate fixtures to a new folder:

```powershell
.\runtime\python\Scripts\python.exe app\examples\portable\generate_examples.py --output <NEW_DIRECTORY>
```

The generator refuses existing output names. Stable example UUIDs and deterministic
ZIP metadata keep the generated packages reproducible.
