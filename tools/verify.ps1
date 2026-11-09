$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
function Run-Dotnet([string[]]$Arguments) { & dotnet @Arguments; if ($LASTEXITCODE -ne 0) { throw "dotnet failed: $Arguments" } }
Run-Dotnet @('run','--project','tests/IDCGames.Integration.Tests','-c','Release')
Run-Dotnet @('run','--project','examples/LauncherDemo','-c','Release','--','--demo')
Run-Dotnet @('run','--project','examples/LauncherDemo','-c','Release','--','--check-native')
$expected = (Get-Content 'runtimes/win-x64/native/manifest.json' -Raw | ConvertFrom-Json).sha256
$actual = (Get-FileHash 'runtimes/win-x64/native/IDCLib2.dll' -Algorithm SHA256).Hash
if ($actual -ne $expected) { throw 'Native DLL hash mismatch' }
Write-Host 'Offline verification complete. Live Launcher/payment/ad acceptance remains game-specific.'
