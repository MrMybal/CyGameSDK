param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\Documentation'),
    [switch]$Check
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$work = Join-Path $projectRoot '.CyDocBuild'
$dependency = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'dependencies.lock.json') -Raw | ConvertFrom-Json).CyDoc
$checkout = Join-Path $projectRoot 'runtime\dependencies\cydoc'
$gitSafe = $checkout.Replace('\','/')
$archive = Join-Path $projectRoot "runtime\cydoc-$($dependency.version).tgz"
$node = (Get-Command node -ErrorAction Stop).Source
$npm = (Get-Command npm.cmd -ErrorAction Stop).Source
if ([int]((& $node -p 'Number.parseInt(process.versions.node)') | Select-Object -Last 1) -lt 22) { throw 'CyDoc generation requires Node.js 22+. Reading needs no Node.js.' }
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE) { throw "$Program failed with exit code $LASTEXITCODE" }
}
if (-not (Test-Path -LiteralPath (Join-Path $checkout '.git'))) {
    New-Item -ItemType Directory -Force -Path (Split-Path $checkout) | Out-Null
    Invoke-Checked 'git' @('clone', '--no-checkout', $dependency.repository, $checkout)
    Invoke-Checked 'git' @('-c', "safe.directory=$gitSafe", '-C', $checkout, 'checkout', '--detach', $dependency.revision)
}
$actual = & git -c "safe.directory=$gitSafe" -C $checkout rev-parse HEAD
if ($LASTEXITCODE -or $actual -ne $dependency.revision) { throw "Use the clean locked CyDoc revision $($dependency.revision) at $checkout" }
$dirty = & git -c "safe.directory=$gitSafe" -C $checkout status --porcelain
if ($LASTEXITCODE -or $dirty) { throw 'The private CyDoc dependency checkout must remain clean.' }
if (-not (Test-Path -LiteralPath $archive)) {
    Push-Location $checkout
    try {
        Invoke-Checked $npm @('ci', '--no-audit', '--no-fund')
        Invoke-Checked $npm @('pack', '--pack-destination', (Split-Path $archive))
    } finally { Pop-Location }
}
# Verify the archive even when npm's installed-package cache is already warm.
Invoke-Checked $node @((Join-Path $work 'check-package.mjs'), $archive)
# npm ci also verifies each installed dependency against package-lock.
$installStamp = Join-Path $work 'internal\install-lock.sha256'
$lockHash = (Get-FileHash -LiteralPath (Join-Path $work 'package-lock.json') -Algorithm SHA256).Hash
$stamp = if (Test-Path -LiteralPath $installStamp) { Get-Content -LiteralPath $installStamp -Raw } else { '' }
if ($stamp.Trim() -ne $lockHash -or -not (Test-Path -LiteralPath (Join-Path $work 'node_modules\cydoc\tools\cli.mjs'))) {
    Invoke-Checked $npm @('ci', '--prefix', $work, '--omit=optional', '--no-audit', '--no-fund')
    New-Item -ItemType Directory -Force -Path (Split-Path $installStamp) | Out-Null
    Set-Content -LiteralPath $installStamp -Value $lockHash -Encoding ASCII
}
$buildArgs = @((Join-Path $work 'build.mjs'))
if ($Check) { $buildArgs += '--check' }
Invoke-Checked $node $buildArgs
$export = Join-Path $projectRoot 'Documentation'
$destination = [IO.Path]::GetFullPath($OutputDirectory)
if ($destination.TrimEnd('\','/') -ne $export.TrimEnd('\','/')) {
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Copy-Item -LiteralPath (Join-Path $export 'Documentation.html') -Destination $destination -Force
    Copy-Item -LiteralPath (Join-Path $export 'DocAI.md') -Destination $destination -Force
    Copy-Item -LiteralPath (Join-Path $export 'DocData') -Destination $destination -Recurse -Force
}
Write-Host "Local help: $destination\Documentation.html"
