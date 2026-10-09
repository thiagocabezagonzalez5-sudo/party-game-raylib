$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
$fuentesParty = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
    Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' } |
    Select-Object -ExpandProperty FullName)
# Enlazar solo el objeto de modelos de la raylib instalada permite observar
# su envio a rlSetUniformMatrix. El resto usa la DLL habitual, sin sumar libs.
Push-Location -LiteralPath 'build'
try {
    & 'C:/msys64/ucrt64/bin/ar.exe' 'x' 'C:/msys64/ucrt64/lib/libraylib.a' 'rmodels.c.obj'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally { Pop-Location }
& 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/VerificarLaberintoJade.cpp' @fuentesParty `
    '-std=c++17' '-Wall' '-Wextra' '-g' '-I' $raizParty '-o' 'build/VerificarLaberintoJade.exe' `
    '-Wl,--wrap=DrawModelEx,--wrap=rlSetUniformMatrix,--wrap=IsKeyPressed,--wrap=IsKeyDown' `
    'build/rmodels.c.obj' '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& './build/VerificarLaberintoJade.exe'
exit $LASTEXITCODE
