param([string]$CyToolsCoreRoot = '')
$ErrorActionPreference = 'Stop'
# Keep the former option accepted by existing scripts. It no longer selects a dependency.
if ($CyToolsCoreRoot) { Write-Warning 'The legacy source-root option is retained for script compatibility; only compute-worker dependencies are installed.' }
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$privatePython = Join-Path $projectRoot 'runtime\python\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $privatePython)) {
    python -m venv (Join-Path $projectRoot 'runtime\python')
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}
& $privatePython -m pip --isolated --disable-pip-version-check --no-cache-dir install -r "$PSScriptRoot\requirements-lock.txt"
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $privatePython -m pip --isolated --disable-pip-version-check uninstall --yes cytools-core
exit $LASTEXITCODE
