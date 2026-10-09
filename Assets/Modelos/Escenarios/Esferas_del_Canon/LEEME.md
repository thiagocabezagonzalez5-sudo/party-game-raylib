# Esferas del Cañón · modelos GLB v1

26 modelos GLB 2.0 originales para `Minigames/MinijuegoEsferasCanon.cpp` en la rama `claude/expansion-party` (referencia `ea5472c`). Se comprobaron cargándolos en raylib. **No están integrados aún en el juego.**

`GLB/` contiene las piezas; `manifest.json` registra límites, pivote, materiales y uso. `Vistas/` ofrece 31 capturas y `Vista_previa.png` presenta la colección. `generar_modelos.py` y `geometria_glb.py` permiten regenerar los archivos con Python 3. `visor_raylib.cpp` es un visor C++17 que carga y descarga todas las mallas.

## Escala y ensamblado

Una unidad GLB equivale a una unidad raylib; Y apunta hacia arriba. La curva reproduce los 14 puntos de control, la interpolación Catmull-Rom, la pendiente `Y = -0.12*S` y la pista de ancho 8,4 del código. La longitud de la curva es 142,23; la meta está en S=120. Las piezas cuyo pivote dice **origen global** se colocan todas en `(0,0,0)`: cuatro `tramo_pista_*`, ocho `pared_*`, tres `puente_roto_*`, `rampa_atajo`, `arco_natural` y `suelo_desertico`. No hay que trasladarlas individualmente a sus respectivos valores S.

- Grieta 1: S=40..43,5, centro de paso lateral +1,5 y semiancho 1. Grieta 2: S=74..77,5, centro −1,5 y semiancho 1. Grieta 3: S=96..102, centro 0 y semiancho 0,8. Los puentes de madera marcan estas franjas seguras; conservar la física y las penalizaciones originales.
- La rampa ocupa S=90..93,5 y lateral +2..+3,6. Los tramos muestran arena suelta en S=18..26, 64..71 y 108..116. La malla no reemplaza la lógica de rozamiento o salto.
- Colocar parejas de `banderin_checkpoint` en S=28, 58 y 88, lateral ±3,9. `arco_salida` va en S=0,8 y `arco_meta` en S=120. Girar sus ejes X para dejarlos perpendiculares a la tangente local. Sus pivotes están al pie y en el centro; ver `visor_raylib.cpp`.
- `mesa_lejana` se instancia fuera de la pista, X≈±20..30. `cactus` y `roca_caida` se instancian con los datos de obstáculo existentes. En el visor se añaden también cactus sobre las paredes.
- `esfera_piedra` tiene pivote en su centro y radio visual ≈1; centro en `Y = -0.12*S + 1`, más la elevación del salto si corresponde. Rotarla con `estado.giro`, `estado.ejeX` y `estado.ejeZ`. `aro_jugador` se coloca en el suelo y puede teñirse con `participantes[i].color`.

Los modelos son estáticos, sin animaciones GLB ni colisiones embebidas. Al integrar, conservar cálculos de pista, proyección, IA, física, controles y cámara. Cargar cada modelo una sola vez y descargarlo al cerrar. El suelo lejano es decorativo y no colisiona. Esta entrega no modifica el repositorio del juego.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorEsferasCanon.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorEsferasCanon.exe
```

Teclas `1` a `4`: escenas; flechas izquierda/derecha: recorrer el cañón; Escape: salir. `--capturar` regenera las vistas. Los bloques pequeños de color en el visor son referencias temporales de jugadores.
