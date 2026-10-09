# Laberinto Jade · modelos GLB v1

Quince modelos GLB 2.0 originales para `Minigames/MinijuegoLaberintoInclinado.cpp` en la rama `claude/expansion-party`, referencia `ea5472c`. Los modelos se comprobaron en raylib mediante el visor incluido. **Todavía no se incorporaron al juego.**

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

Para integrar más adelante, copiar `GLB/` a `Assets/Modelos/Escenarios/LaberintoJade/` y sustituir únicamente el dibujo de primitivas en las funciones visuales. La física, IA, celdas y trampas siguen en `MinijuegoLaberintoInclinado.cpp`. Esta entrega no cambia los archivos del repositorio.
