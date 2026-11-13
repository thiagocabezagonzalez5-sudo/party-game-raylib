$ErrorActionPreference = 'Stop'
# Corre todos los Tests/Verificar*.cpp de una vez. Compila cada fuente del
# juego una sola vez (incremental, con dependencias -MMD) en build/obj_tests
# y enlaza cada test con los -Wl,--wrap=... de su propio .ps1.
# Uso: powershell -ExecutionPolicy Bypass -File Tests/VerificarTodo.ps1
$raizParty = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $raizParty
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
$gpp = 'C:/msys64/ucrt64/bin/g++.exe'
$dirObj = 'build/obj_tests'
New-Item -ItemType Directory -Force -Path $dirObj | Out-Null

function Desactualizado([string]$objeto, [string]$deps) {
    if (-not (Test-Path $objeto) -or -not (Test-Path $deps)) { return $true }
    $fecha = (Get-Item $objeto).LastWriteTimeUtc
    $texto = (Get-Content -Raw $deps) -replace '\\\r?\n', ' '
    $lista = ($texto -split ':', 2)[1] -split '\s+' | Where-Object { $_ -ne '' }
    foreach ($d in $lista) {
        if (-not (Test-Path $d) -or (Get-Item $d).LastWriteTimeUtc -gt $fecha) { return $true }
    }
    return $false
}

$directoriosParty = @('Core', 'UI', 'Gameplay', 'Minigames', 'Systems', 'Board', 'Entities')
$fuentes = @(Get-ChildItem -Path $directoriosParty -Recurse -Filter '*.cpp' |
    Where-Object { $_.FullName -notmatch '\.bak\.cpp$' -and $_.FullName -notmatch '\.conflicto\..*\.cpp$' })
$objetos = @()
foreach ($f in $fuentes) {
    $relativo = Resolve-Path -Relative $f.FullName
    $base = Join-Path $dirObj (($relativo -replace '^\.[\\/]', '') -replace '[\\/]', '_' -replace '\.cpp$', '')
    $objeto = "$base.o"
    $objetos += $objeto
    if (Desactualizado $objeto "$base.d") {
        Write-Host "Compilando $relativo"
        & $gpp '-std=c++17' '-g' '-w' '-I' $raizParty '-MMD' '-MF' "$base.d" '-c' $f.FullName '-o' $objeto
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

Push-Location -LiteralPath 'build'
try {
    if (-not (Test-Path 'rmodels.c.obj')) {
        & 'C:/msys64/ucrt64/bin/ar.exe' 'x' 'C:/msys64/ucrt64/lib/libraylib.a' 'rmodels.c.obj'
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
} finally { Pop-Location }

$fallidos = @()
foreach ($t in Get-ChildItem -Path 'Tests' -Filter 'Verificar*.cpp') {
    $nombre = $t.BaseName
    $ps1 = "Tests/$nombre.ps1"
    $extra = @()
    if (Test-Path $ps1) {
        $contenido = Get-Content -Raw $ps1
        $wrap = [regex]::Match($contenido, "-Wl,--wrap=[^' ]+")
        if ($wrap.Success) { $extra += $wrap.Value }
        if ($contenido -match 'rmodels\.c\.obj') { $extra += 'build/rmodels.c.obj' }
    }
    & $gpp $t.FullName @objetos '-std=c++17' '-g' '-w' '-I' $raizParty '-o' "build/$nombre.exe" @extra `
        '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm'
    if ($LASTEXITCODE -ne 0) { $fallidos += "$nombre (no enlaza)"; continue }
    $salida = & "./build/$nombre.exe" 2>&1 | Out-String
    $codigo = $LASTEXITCODE
    $resumen = ($salida -split "`n" | Where-Object { $_ -match 'errores' } | Select-Object -Last 1)
    Write-Host ("{0}: {1}" -f $nombre, $resumen.Trim())
    if ($codigo -ne 0) { $fallidos += $nombre }
}

if ($fallidos.Count -gt 0) {
    Write-Host ("FALLARON: " + ($fallidos -join ', '))
    exit 1
}
Write-Host 'Todos los tests terminaron con 0 errores.'
exit 0
