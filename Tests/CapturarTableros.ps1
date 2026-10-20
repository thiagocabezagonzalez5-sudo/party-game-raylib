# Abre una ventana real, juega unos turnos de bots en cada tablero y guarda
# capturas PNG tal como se ven en partida (escena 3D + HUD + contador de FPS).
# Tomas: general, recorrido, bifurcacion (si hay), gimmick y fondo.
# Salida: build/capturas/tableros/<tablero>_<toma>_<ancho>x<alto>.png
# Uso:
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/CapturarTableros.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/CapturarTableros.ps1 -Tablero 0
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/CapturarTableros.ps1 -Tablero volcan -Resolucion 1920x1080
#   -Calidad baja|media|alta (por defecto alta), -FpsObjetivo N (0 = sin limite, para medir).
#   -Optimizado compila con -O2 (por defecto -g, igual que build/Party.exe).
#   -SinCompilar reutiliza build/qa/CapturarTableros.exe.
param(
    [string]$Tablero = '',
    [string]$Resolucion = '1280x720',
    [string]$Calidad = 'alta',
    [int]$FpsObjetivo = 0,
    [switch]$Optimizado,
    [switch]$SinCompilar
)

$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build/qa' | Out-Null
New-Item -ItemType Directory -Force -Path 'build/capturas/tableros' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$exe = 'build/qa/CapturarTableros.exe'

if ($Resolucion -notmatch '^(\d+)x(\d+)$') {
    Write-Host "FALLA: -Resolucion debe tener la forma ANCHOxALTO (p. ej. 1920x1080)."
    exit 2
}
$ancho = $Matches[1]
$alto = $Matches[2]

if (-not $SinCompilar) {
    # Sin -Optimizado se compila como build/Party.exe (CMake Debug: -g sin -O),
    # para que el FPS medido sea el del juego.
    $banderasOptimizacion = @('-g')
    if ($Optimizado) { $banderasOptimizacion = @('-O2', '-g') }
    $directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
    $fuentesParty = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
        Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' } |
        Select-Object -ExpandProperty FullName)
    & 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/CapturarTableros.cpp' @fuentesParty `
        '-std=c++17' '-Wall' '-Wextra' @banderasOptimizacion '-I' $raizParty '-o' $exe `
        '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FALLA: no compila (codigo $LASTEXITCODE)."
        exit $LASTEXITCODE
    }
}

$argumentos = @('--resolucion', $ancho, $alto, '--calidad', $Calidad, '--fps', "$FpsObjetivo",
    '--salida', 'build/capturas/tableros')
if ($Tablero -ne '') { $argumentos += @('--tablero', $Tablero) }
& "./$exe" @argumentos
exit $LASTEXITCODE
