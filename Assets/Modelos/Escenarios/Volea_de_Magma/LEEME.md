# Volea de Magma · modelos GLB v1

Diecisiete GLB 2.0 originales para `Minigames/MinijuegoVoleaMagma.cpp` de `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib con el visor incluido. **Aún no integrados en el juego.**

`GLB/` contiene las piezas; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 21 capturas de raylib; `Vista_previa.png` muestra el escenario y ocho piezas. `generar_modelos.py` y `geometria_glb.py` regeneran las mallas con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga todos los modelos.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; la red divide los equipos en X=0. La cámara existente está en `(0,9,14.5)`, mirando a `(0,1.6,0)`.

- `cancha_obsidiana` y `borde_cancha` se colocan en `(0,0,0)`. La plataforma completa mide 20×12 y tiene superficie jugable 18×10. El suelo lógico conserva su bloque de colisión original; el borde no debe cambiar los límites de jugador.
- `red_cadenas` se coloca en `(0,0,0)`, con plano X=0, de Z=−5,4 a +5,4 y cadena principal hasta Y=2,4. Repetir `poste_red` en `(0,0,−5.4)` y `(0,0,5.4)`. Desde la cámara frontal actual la red queda casi de canto; `Escena_general.png` muestra su volumen desde un ángulo oblicuo.
- `lago_lava` se coloca en `(0,0,0)`. `volcan_menor` va en `(−28,−1.5,−38)` y `volcan_mayor` en `(26,−1.5,−44)`, con alturas respectivas 20 y 26. `columna_basalto` tiene altura base 4: colocar su pie en Y=−1,5 y escalar **solo Y** con `altura/4`. Las columnas con llama van en X=±12,2, Z=±3,5.
- `roca_magma` y `roca_sobrecalentada` tienen pivote central y radio visual aproximado 0,45. Instanciar una u otra en `pelota.posicion` según `pelota.temperatura>=0.85`. `estela_ascua`, `sombra_pelota` e `indicador_caida` son piezas separadas para animar con los estados existentes.
- `charco_lava` va en `(charco.x,0,charco.z)` únicamente mientras `charco.activo`; su radio es 1,8. El tiempo de vida de 4 segundos y el efecto de lentitud continúan en C++. `burbuja_lava` y `ceniza` se repiten como decoración sin colisiones.

Son mallas estáticas modulares sin rig ni animaciones GLB. Para integrarlas, copiar `GLB/` a `Assets/Modelos/Escenarios/VoleaMagma/`, conservar las reglas y reemplazar solamente las primitivas de dibujo. Cargar una vez cada pieza y descargarla al cerrar. Esta entrega no modifica el repositorio del juego.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorVoleaMagma.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorVoleaMagma.exe
```

Teclas 1: escena; 2: cámara del juego; 3: roca sobrecalentada; 4: cancha; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los cuerpos cúbicos de colores del visor son referencias de escala para los jugadores y no son GLB del paquete.
