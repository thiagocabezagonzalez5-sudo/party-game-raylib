# Bateo Meteórico · modelos GLB v1

Quince GLB 2.0 originales para `Minigames/MinijuegoBateoMeteorico.cpp` de `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib con el visor incluido. **Aún no integrados en el juego.**

`GLB/` contiene los modelos; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 19 capturas de raylib; `Vista_previa.png` resume el conjunto. `generar_modelos.py` y `geometria_glb.py` regeneran las mallas con Python 3 sin dependencias externas. `visor_raylib.cpp` carga cada malla una vez y la descarga al salir.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; el jugador mira hacia Z negativo.

- Los centros de carril están separados 6 unidades: `carrilX=(carril-(total-1)/2)*6`. `carril_plataforma` se coloca en `(carrilX,0,0)` y ocupa Z=−15..+5; la roca base de `campo_puntaje` ocupa Z=−30..−14 y la franja azul se extiende a −12,5, como en el dibujo actual. Ambos comparten pivote. Los bordes `COLOR_DINAMICO` admiten color por jugador. La zona de 100 puntos permanece centrada en Z=−20,5.
- `canon_base` va en `(carrilX,0,-12.8)`. El tubo tiene el origen en su unión con el cañón y su eje local +Y mide 2,2. Orientarlo desde `(carrilX,4.7,-12.4)` hacia el punto dulce `(carrilX,1.4,-0.7)`.
- Los tres meteoritos tienen origen en el centro y se colocan con `PosicionMeteoritoEntrante` o `PosicionMeteoritoGolpeado`. Normal ≈0,45 de radio, dorado ≈0,50, rojo ≈0,50. Conservar su tipo, ventana de golpe, estela, trayectoria, castigo y puntaje en C++.
- El bate apunta hacia Z negativo desde su origen; colocar en `(carrilX+0.4,1.05,0.7)` y aplicar el ángulo Y del swing existente. El aro va en `(carrilX,1.4,-0.7)`.
- `cumbre` va en `(0,0,0)`; `observatorio_cupula` y `telescopio` comparten `(-36,0,-64)`; la cúpula secundaria va en `(44,0,-76)`; el planeta con anillos en `(34,34,-112)`. El plano lejano y las estrellas del visor son decoración.

Son mallas estáticas modulares sin rig ni animaciones GLB. Para integrar, copiar `GLB/` a `Assets/Modelos/Escenarios/BateoMeteorico/`, conservar todas las reglas y sustituir solamente las primitivas de dibujo. Cargar una vez cada modelo y descargar simétricamente. Esta entrega no modifica el repositorio del juego.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorBateoMeteorico.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorBateoMeteorico.exe
```

Teclas 1: cuatro carriles, 2: cámara del juego, 3: dos carriles; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los personajes cúbicos, estrellas y luna en las vistas son referencias temporales del visor, no modelos GLB de este paquete.
