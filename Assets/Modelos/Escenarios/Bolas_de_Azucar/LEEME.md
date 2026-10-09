# Bolas de Azúcar · modelos GLB v1

20 GLB 2.0 originales para `Minigames/MinijuegoBolasAzucar.cpp` de `claude/expansion-party`, referencia `ea5472c`. Se cargaron en raylib y se comprobaron. **Aún no están integrados en el juego.**

`GLB/` contiene las piezas; `manifest.json` registra dimensiones, materiales, pivote y uso. `Vistas/` incluye 25 capturas y `Vista_previa.png` presenta la colección. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB usando Python 3 estándar. `visor_raylib.cpp` carga y libera los modelos.

## Escala y ubicación

Una unidad GLB equivale a una unidad raylib; Y apunta arriba. Colocar `fondo_rosa` y `suelo_galleta` en `(0,0,0)`. La arena tiene 18×18 unidades, parte superior Y=0. Los cuatro `muro_galleta` se colocan en Z=±9,2 con eje longitudinal X, y en X=±9,2 girados 90°; mantienen el alto de colisión 2 del C++.

- `azucar_lateral` mide 4×9; repetir en `(−6,0,0)` y `(6,0,0)`. `azucar_extremo` mide 6×2,5; repetir en `(0,0,−6,75)` y `(0,0,6,75)`. El crecimiento de la bola debe seguir los rectángulos de la lógica existente.
- `chocolate_grande` va en el origen con radio 2,2. Dos `chocolate_pequeno` de radio 1,4 van en `(−5,5,0,−6,5)` y `(5,5,0,6,5)`. Conservar la pérdida de tamaño y velocidad que calcula el juego.
- `gominola_roja`, `gominola_verde`, `gominola_amarilla` y `gominola_azul` se centran respectivamente en X/Z `(−3,2,−3,2)`, `(3,2,−3,2)`, `(−3,2,3,2)` y `(3,2,3,2)`. El radio lógico es 0,8; conservar los rebotes y colisiones del C++.
- Instanciar siete `piruleta_*` al fondo, X=`−20+6,5*i`, Z=−16, y otras en `(−16,0,4)`, `(16,0,−3)`, `(17,0,9)`. Cuatro `columna_caramelo` van en X=±12,5, Z=±12. Tres `montana_nata` van en X=−22, −2 y 18, Z=−30, Y=−0,5.
- `bola_azucar` tiene radio base 1 y pivote central: colocar su centro en `(bola.x, bola.radio, bola.z)` y escalar uniformemente por `bola.radio` entre 0,3 y 1,4. Mantener el material de la bola blanco; el aro de cada jugador puede teñirse por separado o dibujarse con raylib. `pepita_chocolate` es un detalle opcional repetible.

Son mallas estáticas sin rig ni animación GLB. La física, vidas, crecimiento, derretimiento, proyectiles y jugadores permanecen en C++. Cargar cada modelo una sola vez y descargarlo al salir. Esta entrega no modifica el repositorio.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorBolasAzucar.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorBolasAzucar.exe
```

Teclas 1 a 4: cuatro jugadores, arena, dos jugadores y paisaje. Escape: salir. `--capturar` regenera las vistas. Los cubos de colores del visor son jugadores temporales.
