param(
    # Solo compila (no abre la demo interactiva).
    [switch]$SoloCompilar
)
$ErrorActionPreference = 'Stop'
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
New-Item -ItemType Directory -Force -Path 'build' | Out-Null
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
# Demo del estilo UI: solo necesita EstiloUI y DemoEstiloUI.
& 'C:/msys64/ucrt64/bin/g++.exe' 'Tests/DemoEstiloUI.cpp' 'UI/EstiloUI.cpp' 'UI/DemoEstiloUI.cpp' `
    '-std=c++17' '-Wall' '-Wextra' '-g' '-I' $raizParty '-o' 'build/DemoEstiloUI.exe' `
    '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($SoloCompilar) { exit 0 }
# Interactivo. Para capturas: ./build/DemoEstiloUI.exe --captura 1920 1080 build/capturas/estiloui_v2
& './build/DemoEstiloUI.exe'
exit $LASTEXITCODE
