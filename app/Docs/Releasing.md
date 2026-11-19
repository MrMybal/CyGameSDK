# Manual releases

Release builds and checks run locally. No GitHub Actions workflow is required by
this repository's build or packaging scripts.

1. Keep source/catalogue/executable versions matched and read the exact dependency lock.
   Compile changed native code with **app/build.ps1**. Reuse a verified current binary
   when only documentation/packaging changes; never rename an older executable.
2. Update English/French README, release notes and CyDoc source. Regenerate the complete
   offline reader with **app/build_documentation.ps1**. Preserve stable page IDs/anchors.
3. Run the tests affected by the actual change and a smoke test from an extracted ZIP.
   The Windows distribution must launch with no sibling checkout, Python venv or Node
   installation. Python compute workers remain optional and configured at their final path.
4. Build all three archives and checksums:

~~~powershell
.\runtime\python\Scripts\python.exe app\package_release.py --output runtime\release-0.31.0
~~~

The source ZIP contains distributable sources, examples, documentation and notices.
The Windows ZIP adds the two native executables. The documentation ZIP contains the
complete offline reader and its resources. No installed virtual environment is copied.

5. Review selected files, embedded archives, screenshots/metadata, executable strings,
   commit identities and the unpublished Git history. Run Gitleaks on the exact
   distribution and history. Keep review reports under ignored local data/reports.
6. Push only when the user authorizes publication. Use their GitHub noreply identity.
   Preserve all unrelated work and existing releases. Create the version tag at the
   reviewed commit; upload the three ZIPs and SHA256SUMS.txt through the release API.

If workflows must not run, disable GitHub Actions for this repository before pushing
or creating a tag/release, verify the setting, and use a **[skip ci]** commit annotation.
Do not dispatch workflows or request a Pages build. GitHub Pages remains on its previous
deployment while Actions is disabled; the current manual is included in release assets.
Re-enable Actions only when authorized. Update static-site asset query hashes with
**python docs/tools/stamp_assets.py** whenever its scripts or styles change.
