# Parejas Glaciares — modelos v1

24 GLB 2.0 originales para `MinijuegoParejasGlaciar.cpp`. Hielo azul, fauna del lago y bandas de aurora. Las identidades de los bloques se mantienen en `bloques[]`; los modelos de símbolos no contienen un orden de parejas fijo.

| Grupo | GLB |
| --- | --- |
| Tablero | `mar_frio`, `lago_helado`, `tablero_4x4`, `bloque_oculto`, `bloque_emparejado`, `cursor_seleccion`, `fragmento_hielo` |
| Símbolos | `simbolo_0_esfera`, `simbolo_1_cubo`, `simbolo_2_cono`, `simbolo_3_cilindro`, `simbolo_4_rombo`, `simbolo_5_nieve`, `simbolo_6_copo`, `simbolo_7_aurora` |
| Paisaje | `tempano_jugador`, `iceberg`, `montana_nevada`, `cueva_hielo`, `pinguino`, `foca`, `aurora_verde`, `aurora_violeta`, `aurora_cian` |

## Colocación

Y hacia arriba, unidades raylib. `tablero_4x4` se coloca en el origen. Para el bloque de índice `i`: `x=(i%4−1.5)×1.9`, `z=(i/4−1.5)×1.9`. `bloque_oculto` va en `(x,0,z)` y ocupa 1.65×0.9×1.65. Mostrar un `simbolo_N` únicamente cuando la animación del bloque tenga `derretido >= 0.5`, colocado en `(x,altura_bloque+.05,z)`, como hace el dibujo actual. El símbolo 7 es aurora y corresponde a la pareja especial. Para un bloque emparejado o derretido, escalar la altura visual sin alterar la identidad; `bloque_emparejado` es una variante baja y la marca de dueño se puede teñir en el motor.

`cursor_seleccion` sigue `cursor` durante la fase de elección. En `FASE_GLACIAR_CRUJIDO`, mover/temblar solo `crujidoA` y `crujidoB`; el intercambio de los símbolos sigue siendo responsabilidad del estado del juego. `tempano_jugador` se instancia en `posicionTempanoX/Z` de cada participante. `cueva_hielo` va en `(0,0,-11)`, los icebergs cerca de `(-12,-.5,-9)` y `(11,-.5,-10)`, montañas en Z=-20, y las bandas de aurora detrás en Z=-26. Sus materiales translúcidos admiten movimiento en Y/X al integrarse.

## Reproducir

`manifest.json` enumera límites, pivotes y usos. `python3 generar_modelos.py` reconstruye los GLB sin dependencias. `visor_raylib.cc` carga cada GLB y `--capturar RUTA_ABSOLUTA_DE_ESTA_CARPETA` crea las vistas. La extensión `.cc` evita sumar otro `main` a la compilación recursiva `*.cpp`. Los recursos todavía no sustituyen las primitivas de dibujo existentes.
