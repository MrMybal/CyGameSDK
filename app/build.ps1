param(
    [string]$CyUIRoot = '',
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '..\runtime\build\modular'),
    [string]$Configuration = 'Release',
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..')
)
$ErrorActionPreference = 'Stop'
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmake = $cmakeCommand.Source }
else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install CMake or the Visual Studio CMake tools.' }
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (-not (Test-Path -LiteralPath $cmake)) { throw 'Install the Visual Studio CMake tools.' }
}
$arguments = @('-S', $PSScriptRoot, '-B', $BuildDirectory, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCYGAME_OUTPUT_DIR=$OutputDirectory")
if ($CyUIRoot) { $arguments += "-DCYUI_ROOT=$((Resolve-Path -LiteralPath $CyUIRoot).Path)" }
& $cmake @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $cmake --build $BuildDirectory --config $Configuration --parallel 4
exit $LASTEXITCODE
