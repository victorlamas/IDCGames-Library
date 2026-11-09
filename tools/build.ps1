$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
& "$PSScriptRoot/verify.ps1"
dotnet pack src/IDCGames.Integration -c Release -o artifacts
if ($LASTEXITCODE -ne 0) { throw 'Package failed' }
dotnet publish examples/LauncherDemo -c Release -r win-x64 --self-contained false -o artifacts/LauncherDemo
if ($LASTEXITCODE -ne 0) { throw 'Publish failed' }
Compress-Archive -Path artifacts/LauncherDemo/* -DestinationPath artifacts/LauncherDemo-win-x64.zip -Force
Get-FileHash artifacts/LauncherDemo-win-x64.zip -Algorithm SHA256 | Format-List
