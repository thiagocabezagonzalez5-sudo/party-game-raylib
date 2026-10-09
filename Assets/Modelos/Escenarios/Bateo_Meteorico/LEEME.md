# Bateo Meteórico · modelos GLB v1

Quince GLB 2.0 originales para `Minigames/MinijuegoBateoMeteorico.cpp` de `claude/expansion-party`, referencia artística `ea5472c`. **Integrados en el minijuego real**, con carga compartida diferida, pivotes originales y fallback independiente por pieza.

`GLB/` contiene los modelos; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 19 capturas de raylib; `Vista_previa.png` resume el conjunto. `generar_modelos.py` y `geometria_glb.py` regeneran las mallas con Python 3 sin dependencias externas. `visor_raylib.cpp` carga cada malla una vez y la descarga al salir.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; el jugador mira hacia Z negativo.

- Los centros de carril están separados 6 unidades: `carrilX=(carril-(total-1)/2)*6`. `carril_plataforma` se coloca en `(carrilX,0,0)` y ocupa Z=−15..+5; la roca base de `campo_puntaje` ocupa Z=−30..−14 y la franja azul se extiende a −12,5, como en el dibujo actual. Ambos comparten pivote. Los bordes `COLOR_DINAMICO` admiten color por jugador. La zona de 100 puntos permanece centrada en Z=−20,5.
- `canon_base` va en `(carrilX,0,-12.8)`. El tubo tiene el origen en su unión con el cañón y su eje local +Y mide 2,2. Se coloca en `(carrilX,4.7,-12.4)` usando la dirección original del juego: desde `(carrilX,5,-12)` hacia el punto dulce `(carrilX,1.4,-0.7)`.
- Los tres meteoritos tienen origen en el centro y se colocan con `PosicionMeteoritoEntrante` o `PosicionMeteoritoGolpeado`. Normal ≈0,45 de radio, dorado ≈0,50, rojo ≈0,50. Conservar su tipo, ventana de golpe, estela, trayectoria, castigo y puntaje en C++.
- El bate apunta hacia Z negativo desde su origen; colocar en `(carrilX+0.4,1.05,0.7)` y aplicar el ángulo Y del swing existente. El aro va en `(carrilX,1.4,-0.7)`.
- `cumbre` va en `(0,0,0)`; `observatorio_cupula` y `telescopio` comparten `(-36,0,-64)`; la cúpula secundaria va en `(44,0,-76)`; el planeta con anillos en `(34,34,-112)`. El plano lejano y las estrellas del visor son decoración.

Son mallas estáticas modulares sin rig ni animaciones GLB. El juego usa directamente `Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/`, sin copiar ni regenerar archivos. Las rutas están centralizadas en `Core/RecursosJuego.h` y el almacén está en `Minigames/ModelosEscenariosRetro3D.h`.

## Integración en el juego

El paquete se solicita al activar una partida válida de Bateo, desde `Reiniciar`, y conserva sus recursos entre rondas, reinicios y entradas desde el selector o el tablero. Cada GLB se carga una vez y se comparte entre carriles. `ZonaPruebas::Descargar` libera el almacén antes de `CloseWindow`; no se libera al volver al menú para evitar recargas innecesarias.

| GLB | Elemento reemplazado y estado usado |
| --- | --- |
| `cumbre` | Las dos bases rocosas del mundo. |
| `observatorio_cupula` | Cilindro, esfera y ranura del observatorio principal. |
| `telescopio` | Los dos tubos del telescopio; comparte origen con la cúpula. |
| `observatorio_secundario` | Base, cúpula y ranura secundarias. |
| `planeta_anillado` | Esferas y cuatro anillos del planeta lejano. |
| `farol` | Poste y bombilla de los cinco faroles; mantiene `tiempoAnimacion`. |
| `carril_plataforma` | Plataforma y bordes de Z=−15..+5 por `carrilX`. |
| `campo_puntaje` | Base, bordes y franjas de puntuación del campo. |
| `canon_base` | Pedestal y articulación del cañón. |
| `canon_tubo` | Tubo dirigido desde su unión siguiendo la dirección original. |
| `meteorito_normal` | Superficie y detalle del meteorito normal. |
| `meteorito_dorado` | Superficie dorada; conserva su halo y puntaje doble. |
| `meteorito_rojo` | Superficie inestable; conserva pulso, alambre exterior y explosión. |
| `bate` | Mango y cabeza del bate con el ángulo Y de `tiempoSwing`. |
| `aro_punto_dulce` | Aro original; pulsa en XY alrededor del punto dulce. |

No queda ningún GLB sin usar. Todas las posiciones y trayectorias proceden del minijuego actual. Los meteoritos desaparecen al aterrizar/explotar y los pasados conservan la ventana visual existente. El tubo usa su eje local +Y y longitud 2,2 sin agregar otra transformación de rlgl. El bate GLB recibe su posición y giro una sola vez; solamente su fallback usa la matriz local antigua.

Los índices de primitivas de color se contrastaron con los GLB reales: `carril_plataforma` 3, `campo_puntaje` 4 y `bate` 1 corresponden a `COLOR_DINAMICO`. El cargador resuelve cada material con `meshMaterial`, modifica exclusivamente ese material y lo restaura después del dibujo. Los demás materiales y los colores de vértice permanecen originales; no se aplica tinte global.

En `farol`, la primitiva 2 es `BOMBILLAS`: comparte malla y materiales mediante una vista sin propiedad, pulsa alrededor de su centro local Y=1,97 y recibe el brillo original. Poste, techo y herrajes quedan fijos. No se crea ni descarga geometría por frame.

Si falta una pieza, solamente vuelven sus primitivas. Los bordes originales se dividen entre plataforma y campo para permitir fallos independientes sin dibujarlos dos veces. El cargador recuerda el fallo y avisa una sola vez. El helper evita la sombra automática de personaje sobre las piezas GLB; el meteorito recibe una sola sombra de superficie, y el cielo, avisos, halos, estelas e impactos no proyectan manchas sobre el suelo. Las sombras de jugadores permanecen originales.

Se conservan estrellas, constelaciones, luna, jugadores, estelas, halos, avisos de carga, impactos, explosiones, resplandores de aterrizaje, indicadores y HUD. No se modifican colisiones, cámara, controles, reglas, tiempos ni puntuación.

## Prueba integrada

Desde la raíz del repositorio, con raylib y UCRT64 instalados:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarBateoMeteorico.ps1
```

La prueba compila los fuentes reales y ejecuta `ZonaPruebas` con OpenGL para 2, 3 y 4 participantes. Verifica fases, trayectorias, swing, pulso, materiales, puntos normales/dobles, castigo rojo, fallo, IA por mando ausente, reinicio, regreso al selector, entrada de tablero, carga única, cada fallback y descarga simétrica. Los tipos/tiempos de algunos lanzamientos se fijan como estímulos deterministas; las transiciones, entradas y asignación de puntos usan el código del juego.

Las capturas `build/bateo-*.png` proceden del minijuego integrado, no del visor. La prueba automatizada no sustituye una sesión manual de mandos físicos y audio.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorBateoMeteorico.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorBateoMeteorico.exe
```

Teclas 1: cuatro carriles, 2: cámara del juego, 3: dos carriles; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los personajes cúbicos, estrellas y luna en las vistas son referencias temporales del visor, no modelos GLB de este paquete.
