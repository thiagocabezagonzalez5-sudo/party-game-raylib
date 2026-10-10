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

`manifest.json` enumera límites, pivotes y usos. `python3 generar_modelos.py` reconstruye los GLB sin dependencias. `visor_raylib.cc` carga cada GLB y `--capturar RUTA_ABSOLUTA_DE_ESTA_CARPETA` crea las vistas. La extensión `.cc` evita sumar otro `main` a la compilación recursiva `*.cpp`. Los 24 recursos están integrados en el minijuego real; el visor permanece como referencia del arte.

## Integración en el juego

Las rutas usan la carpeta real `Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB` y están centralizadas en `Core/RecursosJuego.h`. El paquete se solicita al reiniciar una ronda válida, comparte un único modelo por archivo y no se carga al inicializar el catálogo. Ningún GLB se mueve, regenera ni normaliza. Un fallo conserva el fallback individual y emite un diagnóstico una sola vez. `ZonaPruebas::Descargar()` libera el almacén compartido antes de `CloseWindow`, también después de una partida de tablero.

| GLB | Sustitución y estado |
| --- | --- |
| `mar_frio` | Plano del mar; origen mundial, geometría local con Z=-6/Y=-.63. |
| `lago_helado` | Cubo del lago, origen mundial y escala unitaria. |
| `tablero_4x4` | Losa central, origen mundial y escala unitaria. |
| `bloque_oculto` | Cubo, wireframe y talla de cada celda; altura `.9*(1-.8*derretido)`. La primitive 3 de la talla se omite desde `derretido>=.5`. |
| `bloque_emparejado` | Variante y marca de dueño al emparejar; escala Y=`altura/.26`, conservando la misma altura animada del cuerpo. |
| `cursor_seleccion` | Wireframe del cursor, solo durante `FASE_GLACIAR_ELEGIR`; conserva el disco de color del jugador. |
| `simbolo_0_esfera` | Esfera roja de identidad 0. |
| `simbolo_1_cubo` | Cubo azul de identidad 1. |
| `simbolo_2_cono` | Cono amarillo de identidad 2. |
| `simbolo_3_cilindro` | Cilindro verde de identidad 3. |
| `simbolo_4_rombo` | Dos conos violetas de identidad 4. |
| `simbolo_5_nieve` | Dos esferas naranjas de identidad 5. |
| `simbolo_6_copo` | Seis esferas turquesas de identidad 6. |
| `simbolo_7_aurora` | Estrella especial de identidad 7; mantiene el pulso de color y halo procedural. |
| `tempano_jugador` | Base cilíndrica de cada participante en `posicionTempanoX/Z`; el disco del turno sigue pulsando. |
| `iceberg` | Los tres conos originales; mismas posiciones X/Z, radio y altura adaptados con escala explícita. |
| `montana_nevada` | Conos de montaña y nieve; misma altura `9+(i*7)%5`, escala Y=`alto/11`. |
| `cueva_hielo` | Cuatro cubos de cueva en `(0,0,-11)`. |
| `pinguino` | Cuerpo, cabeza, vientre y pico de los tres pingüinos; posiciones originales. |
| `foca` | Cuerpo, cabeza, cola y ojos; posición original `(12.5,-.4,-3)`. |
| `aurora_verde`, `aurora_violeta`, `aurora_cian` | Nueve bandas detrás de las montañas, alternadas por `i%3`. Altura y centro siguen el seno original de `tiempoAnimacion`. |
| `fragmento_hielo` | Efecto visual de crujido en los bloques que tienen `temblor`; tres fragmentos por bloque, sin física ni identidad nueva. |

Todos los símbolos respetan `bloques[i].simbolo`, el umbral `.5` y `(x,altura+.05,z)`, también al volver a ocultar un fallo. El crujido no introduce un revelado adicional. Bloques y símbolos comparten el desplazamiento de temblor original, sin matrices adicionales de rlgl.

El paquete usa `COLOR_JUGADOR`, no `COLOR_DINAMICO`: únicamente la primitive 3 de `bloque_emparejado` recibe el color del dueño, resuelto mediante `meshMaterial`. La primitive 0 (`aurora`) de `simbolo_7_aurora` recibe el pulso existente; `nieve` permanece intacto. Las bandas y fragmentos conservan RGB y colores de vértice, con reducción de alpha para su efecto. Todos los materiales se restauran después de cada dibujo y `DrawModelEx` recibe `WHITE`.

Las montañas y los icebergs tienen el pie local en Y=-.5: la posición Y explícita compensa el escalado para conservar el pie mundial en -.5, sin editar su pivote. Las bandas tienen límites locales Y=6.1906..18.0973: la escala Y y traslación de instancia reproducen la altura y centro animados originales. El disco del turno se eleva a Y=.12 sobre la tapa del témpano (.115), y el disco del cursor a .048 sobre la cuadrícula (.043), para mantener visibles estos indicadores. Los jugadores, cámara y hitboxes no se desplazan.

Los GLB y el fallback del escenario omiten las macros de sombra automática para evitar manchas de tamaño humano y sombras en Y=0 sobre objetos elevados. Las sombras existentes de los personajes se conservan. No quedan modelos sin usar.

## Verificación del juego integrado

Desde la raíz, ejecutar `powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarParejasGlaciar.ps1`. Compila con el UCRT64 existente y ejecuta `ZonaPruebas`/`GestorMinijuegos` reales con OpenGL y entradas simuladas para 2, 3 y 4 participantes. Comprueba fases, revelado, pareja aurora, fallos, crujido, IA, tiempo, resultado, reinicio, salida, materiales, pivotes, cargas compartidas, fallback individual y descarga. Las capturas `build/glaciar-*.png` corresponden al minijuego integrado.
