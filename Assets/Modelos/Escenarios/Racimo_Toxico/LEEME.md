# Racimo Tóxico · modelos GLB v1

Catorce GLB 2.0 originales para `Minigames/MinijuegoRacimoToxico.cpp` de `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib con el visor incluido. **Aún no integrados en el juego.**

`GLB/` contiene las mallas; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 18 capturas de raylib; `Vista_previa.png` muestra la escena y ocho piezas. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga todos los modelos.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; el jugador mira hacia Z negativo.

- `agua_pantano` y `enredadera_central` se colocan en `(0,0,0)`; `arbol_podrido`, en `(-4.8,0,-1)`. La enredadera tiene ramitas a la altura de los 22 frutos.
- Los frutos tienen pivote en el centro. Colocar `fruto_normal`, `fruto_toxico` (índices 5, 11 y 17) o `fruto_dorado` en `PosicionFrutaRacimo(i,t)` según el estado del juego. Conservar la selección, el veneno y el premio en C++.
- Colocar una `balsa` en `(posicionBalsaX, 0.08+hundimiento+bamboleo, 3.5)`. La balsa mide 3.25 de ancho, frente a una separación de 5 entre centros. `aro_turno` se puede mostrar en `(posicionBalsaX,.02,3.5)` mientras juega esa balsa.
- `tronco_flotante` tiene pivote en un extremo y eje local +Z de longitud 1. Orientar ese eje entre los extremos originales y escalarlo a su longitud. Conservar los movimientos, colisiones y hundimiento existentes.
- Repetir `cabana_pilotes` en `(-10.5,0,-8)` y `(9.5,0,-9)`, y `arbol_fondo` alrededor de Z=−15. Repartir `nenufar` y `roca_pantano` fuera de las balsas; instanciar y animar `luciernaga` con las posiciones actuales.

Son mallas estáticas modulares sin rig ni animaciones GLB. Para integrarlas, copiar `GLB/` a `Assets/Modelos/Escenarios/RacimoToxico/`, conservar las reglas y reemplazar las primitivas de dibujo. Cargar una vez cada modelo y descargarlo al cerrar el minijuego. Esta entrega no modifica el repositorio del juego.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorRacimoToxico.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorRacimoToxico.exe
```

Teclas 1: cuatro balsas; 2: cámara del juego; 3: dos balsas; 4: pantano; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los personajes de prueba y los efectos del visor son referencias temporales, no GLB de este paquete.
