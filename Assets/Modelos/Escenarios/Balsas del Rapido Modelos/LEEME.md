# Balsas del Rápido — modelos v1

24 modelos GLB 2.0 originales de geometría simple y colores planos, adaptados a `MinijuegoBalsasRapido.cpp` de la rama `claude/expansion-party` (`ea5472c`). Y es vertical; Z negativo indica avance. Una unidad GLB equivale a una unidad raylib. La lógica de carrera, obstáculos y puntuación permanece independiente del visual.

## Contenido

| Grupo | Archivos |
| --- | --- |
| Río y selva | `rio_tramo`, `isla_tramo`, `orilla_tramo`, `arbol_selva`, `liana`, `palmera_isla`, `ruina_musgosa`, `cascada_orilla`, `espuma_rapido` |
| Interacciones | `roca_obstaculo`, `tronco_flotante`, `divisor_rapido`, `remolino`, `banana_boost` |
| Carrera | `balsa`, `remo`, `arco_meta`, `linea_meta` |
| Pájaros | `loro_rojo`, `loro_azul`, `loro_amarillo` y sus tres alas correspondientes |

`manifest.json` da el pivote, los límites, materiales y el uso previsto de cada GLB. El material `COLOR_DINAMICO` permite teñir la bandera de cada equipo.

## Montaje y movimiento

- Repetir `rio_tramo` en los centros X=−8 y +8, `isla_tramo` en X=0 y `orilla_tramo` en X=±17.6, cada 20 unidades de Z desde −10 hasta −130. Girar 180° la orilla izquierda. El río tiene dos carriles de 11.2 unidades de ancho y una isla central de 4.8.
- Situar la balsa en `(b.x, 0.12+oscilación, −b.p)` y girarla `−b.rumbo` alrededor de Y. Los dos `remo` se animan individualmente según `tiempoPalada[0]` y `[1]`. Los jugadores se colocan mediante la lógica actual.
- Para cada obstáculo, calcular X como `centroCarril + desvio` y Z como `−p`. Conservar los tamaños lógicos `o.a` y `o.b` para colisiones; escalar el modelo visual si corresponde. `divisor_rapido` tiene 30 unidades de largo y va centrado en p=67 en ambos carriles. La espuma marca la ruta corta entre p≈56 y 80.
- El `remolino` se puede girar como modelo completo; ocultar `banana_boost` tras su recogida. Las cascadas de orilla van cerca de p=30, 95 y 113. El `arco_meta` cruza ambos carriles en Z=−120; poner dos `linea_meta` en X=±8.
- Los loros tienen alas separadas para el aleteo. Instanciarlos en el vuelo decorativo del código, aproximadamente a Y=5, sin crear colisiones.

## Revisión

`python3 generar_modelos.py` recrea los GLB sin bibliotecas externas. `visor_raylib.cpp` los carga y genera las capturas de `Vistas/`; necesita raylib y SDL para compilar. Las vistas `Inicio_camara_del_juego`, `Rapidos_y_divisor` y `Meta` usan la fórmula de cámara del juego con focos de avance 20, 67 y 120. `VALIDACION.txt` resume los controles técnicos.

El paquete aún no sustituye las llamadas `Draw*` del repositorio; se entrega como conjunto de assets modular.
