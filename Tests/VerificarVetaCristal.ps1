$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
$fuentesParty = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
    Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' } |
    Select-Object -ExpandProperty FullName)
& 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/VerificarVetaCristal.cpp' @fuentesParty `
    '-std=c++17' '-Wall' '-Wextra' '-g' '-I' $raizParty '-o' 'build/VerificarVetaCristal.exe' `
    '-Wl,--wrap=DrawModelEx,--wrap=IsKeyPressed,--wrap=LoadModel,--wrap=UnloadModel,--wrap=FileExists,--wrap=DrawCube,--wrap=DrawSphereEx,--wrap=DrawSphere,--wrap=DrawCircle3D' `
    '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& './build/VerificarVetaCristal.exe'
exit $LASTEXITCODE
