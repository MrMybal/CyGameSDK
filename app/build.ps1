param([string]$CyUIRoot = (Join-Path $PSScriptRoot '..\..\CyImgui_Interface'))
$ErrorActionPreference='Stop'
$env:CYUI_ROOT=(Resolve-Path $CyUIRoot).Path
& "$PSScriptRoot\build_native.cmd"
exit $LASTEXITCODE
