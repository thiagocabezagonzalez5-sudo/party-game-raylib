# Juega TODOS los minijuegos con 2, 3 y 4 bots en una ventana real (flujo de
# Zona de Pruebas en modo catalogo) e informa estado final, desenlace,
# duracion y costo por frame. Capturas en build/capturas/minijuegos.
# Uso:
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RecorrerMinijuegos.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RecorrerMinijuegos.ps1 -Minijuego 45 -Jugadores 4
param(
    [int]$Minijuego = -1,
    [int]$Jugadores = 0
)

$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build/qa' | Out-Null
New-Item -ItemType Directory -Force -Path 'build/capturas/minijuegos' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$exe = 'build/qa/RecorrerMinijuegos.exe'

$directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
$fuentesParty = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
    Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' } |
    Select-Object -ExpandProperty FullName)
& 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/RecorrerMinijuegos.cpp' @fuentesParty `
    '-std=c++17' '-Wall' '-Wextra' '-g' '-I' $raizParty '-o' $exe `
    '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
if ($LASTEXITCODE -ne 0) { Write-Host "FALLA: no compila (codigo $LASTEXITCODE)."; exit $LASTEXITCODE }

$argumentos = @()
if ($Minijuego -ge 0) { $argumentos += @('--minijuego', "$Minijuego") }
if ($Jugadores -gt 0) { $argumentos += @('--jugadores', "$Jugadores") }
& "./$exe" @argumentos
exit $LASTEXITCODE
