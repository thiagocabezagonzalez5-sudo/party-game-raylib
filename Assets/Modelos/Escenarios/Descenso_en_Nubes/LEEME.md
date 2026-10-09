# Descenso en Nubes · modelos GLB v1

Diecinueve GLB 2.0 originales para `Minigames/MinijuegoDescensoNubes.cpp` de `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib con el visor incluido. **Aún no integrados en el juego.**

`GLB/` contiene las piezas; `manifest.json` registra dimensiones, materiales, pivotes y ubicación. `Vistas/` contiene 23 capturas; `Vista_previa.png` muestra descenso y aterrizaje. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga cada modelo.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; la caída comienza en Y=100 y termina en Y=0. La cámara acompaña `alturaCamara` desde `(0,alturaCamara+13,7.5)` hacia `(0,alturaCamara-3,.8)`.

- `planeador` se coloca en `jugador.posicion`, con el ala a Y=+1,3 y ancho 2,2. Usar `planeador_frenado` mientras `estado.frenando>0` (ancho 3,4). Su material `COLOR_DINAMICO` se puede teñir con el color del jugador; el visor aplica un tinte al modelo entero como demostración. Conservar el cuerpo del personaje por separado.
- `anillo_blanco` y `anillo_dorado` tienen pivote central y radios 0,85 y 1,0. Colocar en `anillo.posicion` según `valor`. `estrella` es un efecto decorativo opcional. Mantener la recogida, la puntuación y el estado `recogidoPor` en C++.
- `nube_tormenta` va en `tormenta.posicion`, con tamaño visual cercano al radio base 1,6. Mantener la colisión, el aturdimiento y el descuento de puntos. `banda_viento` tiene origen central, radio 6 y Y=±1,6 respecto de `viento.altura`. Repetir `flecha_viento` en esa altura y orientarla desde +X hacia `dirX,dirZ`; las fuerzas siguen en C++.
- Mostrar `mar_de_nubes`, `isla_principal` y `diana_aterrizaje` juntos en `(0,0,0)` cuando `alturaCamara<=30`. La isla tiene radio 7 y diana de radio 1,8, con superficie de aterrizaje en Y=0.
- `isla_flotante` se repite fuera del cilindro a radio aproximado 10,5..13,5 y alturas `12+13*k`; `globo_azul` y `globo_rojo` se alternan a X=±9,5 y alturas `20+17*k`. `nube_blanca` forma el fondo. `molino_torre` va en `(-5.2,0,-3)` y sus `aspas_molino` en `(-5.2,4.15,-2.12)` con giro sobre Z; hay otros molinos reducidos en las nubes. `ave` se repite cerca de radio 8. `arcoiris` va en `(0,62,-16)`.

Son mallas estáticas modulares sin rig ni animaciones GLB. Para integrarlas, copiar `GLB/` a `Assets/Modelos/Escenarios/DescensoNubes/`, conservar la lógica y sustituir las primitivas de dibujo. Cargar una vez cada pieza y descargarla al cerrar. Esta entrega no modifica el repositorio del juego.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorDescensoNubes.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorDescensoNubes.exe
```

Teclas 1: descenso; 2: cámara del juego; 3: aterrizaje; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los cuerpos cúbicos del visor solo sirven de referencia de escala y no forman parte de los GLB.
