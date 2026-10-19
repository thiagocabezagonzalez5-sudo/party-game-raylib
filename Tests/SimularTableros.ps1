# Simula partidas completas de tablero (todos bots, sin ventana) en cada tablero del catalogo.
# Uso:
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/SimularTableros.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/SimularTableros.ps1 -Tablero 0
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tests/SimularTableros.ps1 -Tablero volcan -Semillas 10
#   -SinCompilar reutiliza build/qa/SimularTableros.exe.
param(
    [string]$Tablero = '',
    [int]$Semillas = 5,
    [switch]$SinCompilar
)

$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build/qa' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$exe = 'build/qa/SimularTableros.exe'

if (-not $SinCompilar) {
    $directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
    $fuentesParty = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
        Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' } |
        Select-Object -ExpandProperty FullName)
    & 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/SimularTableros.cpp' @fuentesParty `
        '-std=c++17' '-Wall' '-Wextra' '-g' '-I' $raizParty '-o' $exe `
        '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FALLA: no compila (codigo $LASTEXITCODE)."
        exit $LASTEXITCODE
    }
}

$argumentos = @('--semillas', "$Semillas")
if ($Tablero -ne '') { $argumentos += @('--tablero', $Tablero) }
& "./$exe" @argumentos
exit $LASTEXITCODE
