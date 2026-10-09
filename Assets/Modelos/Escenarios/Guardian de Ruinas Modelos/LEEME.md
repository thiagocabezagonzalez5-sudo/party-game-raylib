# Guardián de Ruinas — modelos v1

20 GLB 2.0 originales para `MinijuegoGuardianRuinas.cpp`. Ruinas de piedra gris, vegetación y energía turquesa. Los modelos visuales se alinean con las coordenadas y estados existentes, sin contener lógica de colisiones.

| Grupo | GLB |
| --- | --- |
| Plaza y templo | `terreno_selva`, `plaza_losas`, `muro_lateral`, `escalinata`, `templo_fondo`, `muro_fondo`, `portal_marco`, `runa`, `velo_portal`, `linea_meta` |
| Obstáculos y vegetación | `columna_entera`, `aviso_columna`, `columna_caida`, `estatua_caida`, `arbol_selva`, `arbusto` |
| Acciones | `orbe_azul`, `orbe_devuelto`, `escudo_guardian`, `escudo_embestida` |

## Pivotes y estados

Y hacia arriba y unidades raylib. `plaza_losas` va en el origen, con X±8.6 y Z≈-14..10; `templo_fondo` en `(0,0,-18)`, `escalinata` en `(0,0,-13.3)` y `portal_marco` en `(0,0,-12.7)`. El hueco del portal mide cinco unidades de ancho. `velo_portal` ocupa `(0,0,-12.5)` y usa un material translúcido `VELO_PORTAL`; las runas se colocan sobre las jambas del marco. La línea de gol sigue la posición lógica Z=-12.3 (el recurso se sitúa en Z=-12.1, como el dibujo actual).

Las seis `columna_entera` van exactamente en `POSICIONES_COLUMNAS`, y su radio central coincide con `RADIO_COLUMNA=.6`. Mostrar `aviso_columna` solo en `COLUMNA_GUARDIAN_AVISO`; reemplazar por `columna_caida` al cambiar a `COLUMNA_GUARDIAN_CAIDA` y conservar que la caída deja de ser sólida. Los orbes se dibujan con centro `(orbe.x,.9,orbe.z)`; usar `orbe_devuelto` solo con `orbe.devuelto`. Su halo y escalado no cambian el radio lógico de .35.

El escudo tiene su pivote en el centro `(guardianX,1,ZEscudo(m))`, mide aproximadamente 3.8×1.5 y lleva el pie de piedra incluido. Sustituir `escudo_guardian` por `escudo_embestida` durante la embestida; ambas piezas usan un panel translúcido y el mismo punto de colocación. `manifest.json` documenta límites, materiales y pivotes de cada GLB. Los recursos todavía no sustituyen las primitivas de dibujo del juego.

## Reproducir

`python3 generar_modelos.py` regenera los modelos sin dependencias. `visor_raylib.cc` verifica la carga en raylib y `--capturar RUTA_ABSOLUTA_DE_ESTA_CARPETA` produce las vistas. Su extensión `.cc` evita añadir otro `main` a la tarea recursiva `*.cpp`. `Vista_previa.png` y `VALIDACION.txt` documentan la comprobación.
