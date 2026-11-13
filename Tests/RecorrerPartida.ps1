# Partida de tablero completa con 4 bots y el flujo real del juego
# (orden de turno, tablero, ruleta, minijuego real, resultados) en ventana
# real. Registra cada estado y guarda capturas en build/capturas/partida.
# Uso:
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RecorrerPartida.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RecorrerPartida.ps1 -Tablero 3 -Rondas 2
param(
    [int]$Tablero = 0,
    [int]$Rondas = 2
)

$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build/qa' | Out-Null
New-Item -ItemType Directory -Force -Path 'build/capturas/partida' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$exe = 'build/qa/RecorrerPartida.exe'

$directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
$fuentesParty = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
    Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' } |
    Select-Object -ExpandProperty FullName)
& 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/RecorrerPartida.cpp' @fuentesParty `
    '-std=c++17' '-Wall' '-Wextra' '-g' '-I' $raizParty '-o' $exe `
    '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
if ($LASTEXITCODE -ne 0) { Write-Host "FALLA: no compila (codigo $LASTEXITCODE)."; exit $LASTEXITCODE }

& "./$exe" '--tablero' "$Tablero" '--rondas' "$Rondas"
exit $LASTEXITCODE
