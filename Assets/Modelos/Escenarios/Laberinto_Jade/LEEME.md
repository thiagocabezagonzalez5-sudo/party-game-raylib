# Laberinto Jade · modelos GLB v1

Quince modelos GLB 2.0 originales para `Minigames/MinijuegoLaberintoInclinado.cpp` en la rama `claude/expansion-party`, referencia artística `ea5472c`. **Los 15 modelos están integrados en el minijuego real.** El visor incluido sigue siendo independiente del juego.

## Contenido

- `GLB/`: piezas para importar.
- `manifest.json`: límites, triángulos, materiales, pivotes y colocación.
- `Vistas/`: 19 capturas raylib, incluidas la cámara original de cuatro jugadores, la de dos jugadores y vistas de cada pieza.
- `Vista_previa.png`: resumen visual.
- `generar_modelos.py` y `geometria_glb.py`: regeneran todo con Python 3, sin dependencias externas.
- `visor_raylib.cpp`: visor independiente; carga y descarga todos los GLB.

## Medidas y reglas de integración

Una unidad GLB equivale a una unidad del mundo raylib. La losa mide 12×12, con el suelo jugable en Y=0. La celda lógica (c,r) se dibuja en **X=c−5, Z=r−5**, respecto del centro de cada tablero. El bloque de muro ocupa menos de una celda en planta y llega a Y≈0,61; la colisión debe seguir usando las celdas del código. Los tres diseños se obtienen colocando las mismas piezas de acuerdo con el mapa lógico actual, también cuando este se transpone o rota.

El origen de `esfera_jade.glb` está en el **centro** de la esfera; se coloca con el radio actual y se escala durante una caída. `banda_jugador.glb`, `checkpoint.glb`, `flecha_trampa.glb` y `boquilla_trampa.glb` usan `COLOR_DINAMICO` para tintar el estado. El cambio de color debe aplicarse a ese material y no a toda la pieza. `altar.glb` y `agujero.glb` son decoración; el radio de meta y el de caída permanecen en la lógica.

`losa_marco`, los bloques y las piezas interactivas se dibujan entre `rlTranslatef(centro...)` y las dos rotaciones del tablero. `templo_fondo`, `columna_templo`, `antorcha` y `llama` se colocan en coordenadas de mundo, antes de dibujar los tableros. La antorcha está anclada en (x,2.4,−21); la llama tiene el pivote centrado en (x,4.3,−20.9) y admite escala/oscilación independientes.

## Visor

En Windows con MSYS2 UCRT64 y raylib configurados, dentro de esta carpeta:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorLaberintoJade.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorLaberintoJade.exe
```

Teclas: 1 cuatro tableros, 2 dos tableros, 3 un tablero, flechas izquierda/derecha giran la cámara, Escape cierra. `--capturar` regenera las capturas. Las vistas son del visor independiente, no del juego integrado.

## Archivos del juego

`Core/RecursosJuego.h` centraliza las rutas reales `Assets/Modelos/Escenarios/Laberinto_Jade/GLB/`. No mover ni regenerar los recursos. `ModelosEscenariosRetro3D.h` carga el paquete al activar el minijuego y comparte un modelo por recurso entre todos los tableros. Reiniciar o reentrar desde el selector/tablero reutiliza esas mallas; la descarga compartida ocurre antes de `CloseWindow`.

`DibujarEscenaTemplo`, `DibujarTrampaVisual` y `DibujarTableroVisual` conservan las primitivas originales como alternativa por pieza. Una carga fallida se registra una sola vez. Los muros, agujeros, salida, checkpoints, altar y trampas se colocan según las celdas del mapa lógico actual, incluidos los diseños transpuesto y girado. Las flechas apuntan hacia la dirección lógica de los dardos; las boquillas miran hacia el interior del pasillo.

Los modelos del tablero reciben posiciones locales y heredan las dos rotaciones existentes de `rlgl`. No se vuelve a sumar `centrosTableros` ni a componer la inclinación en `DrawModelEx`. La esfera conserva el radio 0,28, el pivote central y la escala uniforme de caída. Permanecen su sombra de color, los indicadores y los dardos en vuelo. Los modelos mantienen sus colores de vértice y materiales; únicamente se cambia y restaura `COLOR_DINAMICO` en banda (primitiva 0), checkpoint (0), flecha (0) y boquilla (2), resolviendo el material con `meshMaterial`. No se aplican las sombras automáticas de personajes a estas piezas del escenario.

La física, colisión por celdas, IA, cámara, controles, trampas y reglas permanecen en `MinijuegoLaberintoInclinado.cpp`, independientes de las mallas. El encabezado conserva sus firmas actuales.

Prueba reproducible desde la raíz con MSYS2 UCRT64:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarLaberintoJade.ps1
```

La prueba ejecuta `ZonaPruebas` y el minijuego real con OpenGL en ventana oculta, para los tres diseños y 2/3/4 participantes. Comprueba las matrices enviadas a GPU, inclinación con entrada de teclado simulada, materiales, trampas, checkpoints, caída/respawn, meta/final, IA, R, ESC, carga compartida, alternativas por pieza y descarga. Las posiciones de esfera para alcanzar estados específicos se preparan en la prueba; sus transiciones usan la actualización real. Las capturas `build/jade-*.png` son del juego integrado. `Vistas/` y `Vista_previa.png` siguen siendo imágenes del visor. Falta una partida manual con mandos y audio.

Para observar los envíos de matrices internos, el script extrae `rmodels.c.obj` de la biblioteca raylib ya instalada y lo enlaza solamente en la prueba, junto con la DLL habitual. El objeto queda en `build/`; no cambia la instalación, las dependencias del juego ni `.vscode/tasks.json`.
