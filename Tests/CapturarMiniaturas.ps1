# Regenera las miniaturas del menu de minijuegos: juega cada minijuego con
# bots en una ventana real y guarda una captura de gameplay sin HUD, tomada
# en un instante aleatorio de la fase de juego, reducida a 320x180 en
# Assets/UI/Miniaturas/<minijuego>.png.
# Uso:
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/CapturarMiniaturas.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/CapturarMiniaturas.ps1 -Minijuego 12
#   -Semilla N repite los mismos instantes; -SinCompilar reutiliza build/qa/CapturarMiniaturas.exe.
param(
    [int]$Minijuego = -1,
    [long]$Semilla = -1,
    [switch]$SinCompilar
)

$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build/qa' | Out-Null
New-Item -ItemType Directory -Force -Path 'Assets/UI/Miniaturas' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$exe = 'build/qa/CapturarMiniaturas.exe'

if (-not $SinCompilar) {
    $directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
    $fuentesParty = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
        Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' } |
        Select-Object -ExpandProperty FullName)
    & 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/CapturarMiniaturas.cpp' @fuentesParty `
        '-std=c++17' '-Wall' '-Wextra' '-g' '-I' $raizParty '-o' $exe `
        '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
    if ($LASTEXITCODE -ne 0) { Write-Host "FALLA: no compila (codigo $LASTEXITCODE)."; exit $LASTEXITCODE }
}

$argumentos = @()
if ($Minijuego -ge 0) { $argumentos += @('--minijuego', "$Minijuego") }
if ($Semilla -ge 0) { $argumentos += @('--semilla', "$Semilla") }
& "./$exe" @argumentos
exit $LASTEXITCODE
