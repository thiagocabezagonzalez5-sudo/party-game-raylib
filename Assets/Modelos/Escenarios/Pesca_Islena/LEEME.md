# Pesca Isleña · modelos GLB v1

18 GLB 2.0 originales para `Minigames/MinijuegoPescaIsla.cpp` en `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib. **Aún no integrados en el juego.**

`GLB/` contiene los modelos; `manifest.json`, sus dimensiones, pivotes y usos. `Vistas/` tiene 23 capturas y `Vista_previa.png` una selección. `generar_modelos.py` y `geometria_glb.py` regeneran los modelos con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y libera los 18 modelos.

## Escala y ubicación

Una unidad GLB equivale a una unidad raylib. Y apunta hacia arriba. `oceano`, `isla_laguna` y `superficie_laguna` se colocan en `(0,0,0)`. La isla tiene radio 15, la laguna radio 8,5, el fondo Y=−1,8 y ondas sobre Y=0. El océano tiene una abertura circular para que no oculte los peces bajo la laguna. El modelo `superficie_laguna` representa el fondo de agua turquesa y las ondas: no es una tapa opaca sobre los peces.

- Instanciar `muelle_bambu` en `(0,0,10.5)`, `(0,0,−10.5)`, `(−10.5,0,0)` y `(10.5,0,0)`. Girar 0°, 180°, −90° y 90°, respectivamente. Su tarima superior ronda Y=0,4; la posición lógica del jugador sigue siendo Y=0,35 + 0,72.
- `cana_pescar` tiene origen en el mango: colocarla a Y=1,45 sobre cada muelle y girar como ese muelle. Su extremo se dirige hacia el centro. La línea de pesca sigue calculándose en C++ entre la punta y el anzuelo; `corcho` se centra en ese anzuelo y `cursor_lanzamiento` en el cursor a Y=0,09.
- Ocho `palmera` van a radio 13,2 y ángulos `22,5° + 45°*k`. `barca` se coloca en `(−12,0,−11)`, `volcan` en `(0,0,−21)` y `humo_volcan` comienza en `(0,7,−21)`. `gaviota` puede moverse por el vuelo orbital existente.
- Distribuir 16 instancias de `coral_1`/`coral_2` dentro de la laguna, en Y=−1,8. `pez_pequeno`, `pez_mediano` y `pez_dorado` tienen pivote central; colocar su centro en `(pez.x,−0.9,pez.z)` y orientar el eje +X hacia `(dirX,dirZ)`. `bota_vieja` usa la misma posición lógica. Los valores del juego son 1, 2, 5 y 0 puntos, respectivamente.

Son modelos estáticos sin rig, animaciones ni colisiones embebidas. Conservar los estados de pique, tensión, capturas y colisiones de C++. Cargar una vez cada GLB y descargarlo al salir. Esta entrega no modifica el repositorio.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorPescaIslena.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorPescaIslena.exe
```

Teclas 1 a 4: escenas; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las vistas. Los bloques de colores son referencias temporales de jugadores.
