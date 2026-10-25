param([string]$CyToolsCoreRoot = (Join-Path $PSScriptRoot '..\..\CyToolsCore'))
$ErrorActionPreference='Stop'
$projectRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$privatePython=Join-Path $projectRoot 'runtime\python\Scripts\python.exe'
if(-not (Test-Path $privatePython)){
    python -m venv (Join-Path $projectRoot 'runtime\python')
    if($LASTEXITCODE){exit $LASTEXITCODE}
}
$wheel=Join-Path $CyToolsCoreRoot 'dist\cytools_core-0.10.1-py3-none-any.whl'
if(-not (Test-Path $wheel)){throw "Build the CyToolsCore 0.10.1 wheel in $CyToolsCoreRoot first."}
& $privatePython -m pip --isolated --disable-pip-version-check --no-cache-dir install -r "$PSScriptRoot\requirements-lock.txt"
if($LASTEXITCODE){exit $LASTEXITCODE}
& $privatePython -m pip --isolated --disable-pip-version-check --no-cache-dir install --no-deps $wheel
exit $LASTEXITCODE
