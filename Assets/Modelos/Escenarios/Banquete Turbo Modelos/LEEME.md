# Banquete Turbo — modelos v1

15 GLB 2.0 originales para el comedor orbital de `MinijuegoBanqueteTurbo.cpp`. Estilo low poly con metal azul, luces cian y acentos de color de cada participante. Son recursos visuales separados de la lógica existente.

## Contenido

| Escenario | Personajes y objetos |
| --- | --- |
| `comedor_orbital`, `ventanal_tierra`, `tierra`, `cielo_estrellado`, `techo_comedor`, `tuberias_techo`, `lampara_colgante`, `consola_lateral` | `mesa_magnetica`, `bandeja`, `taburete`, `robot_camarero`, `tubo_comida_normal`, `tubo_comida_picante`, `tubo_comida_dorada` |

## Colocación

`manifest.json` incluye límites, pivotes y uso sugerido de cada GLB. Y apunta hacia arriba. El modelo `comedor_orbital` ya sitúa su suelo entre Z=-12 y Z=10, con la cara superior en Y=0; colocarlo en el origen. `ventanal_tierra` va en `(0,0,-8)`, `tierra` en `(-7,7,-24)` y `cielo_estrellado` en `(0,6,-36)`. Este fondo se compone de una placa opaca: debe dibujarse antes de la Tierra y el ventanal. `techo_comedor` va en `(0,11,-1)` y `tuberias_techo` en `(0,9,-4)`.

Por cada puesto, `x = (k−(cantidad−1)/2)×4.2`, `mesa_magnetica` va en `(x,0,1.3)`, `bandeja` en `(x,.99,1.3)` y `taburete` en `(x,0,-.3)`. La tapa de mesa está en Y=.86; la bandeja descansa a Y≈.96. El `robot_camarero` tiene pivote en el centro del torso y se coloca en `(robotX,2.5,-3)`; el movimiento y la extensión del brazo corresponden al estado del minijuego.

Los tres tubos comparten centro y medida: `(x,1.18,1.3)` en la ración activa. El progreso del alimento y el deslizamiento de entrada se aplican al dibujarlos; si se sustituye la geometría actual, hay que mantener la lectura del tipo picante y dorado. El material `COLOR_JUGADOR` de taburetes y mesas sirve para teñir el borde según el participante. `EMISION` señala luces cian que pueden configurarse como emisivas en el motor.

## Generación y revisión

Ejecutar `python3 generar_modelos.py` para reconstruir los 15 GLB con la biblioteca estándar. `visor_raylib.cc` carga todos los modelos, y con `--capturar RUTA_ABSOLUTA_DE_ESTA_CARPETA` genera las vistas. Tiene extensión `.cc` para que la tarea recursiva de compilación `*.cpp` del juego no lo incluya. Las capturas y `VALIDACION.txt` registran la revisión. Estos GLB aún no sustituyen las primitivas del minijuego.
