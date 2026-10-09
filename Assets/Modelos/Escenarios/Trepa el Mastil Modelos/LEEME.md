# Trepa el Mástil — modelos v1

25 GLB 2.0 originales para `MinijuegoTrepaMastil.cpp`. Barco explorador de madera con velas claras, franjas y banderas de cuatro colores, agua azul y fauna sencilla. Cada pieza visual es independiente de los agarres, las olas y las colisiones del minijuego.

| Grupo | Archivos |
| --- | --- |
| Barco | `casco_barco`, `cubierta`, `barandilla`, `cofre`, `barril`, `ancla`, `timon` |
| Escalada | `mastil`, `vela`, `cofa`, `jarcias`, `franja_roja`, `franja_azul`, `franja_verde`, `franja_amarilla`, `bandera_roja`, `bandera_azul`, `bandera_verde`, `bandera_amarilla` |
| Mar y aves | `mar`, `espuma_ola`, `isla_lejana`, `cuervo`, `ala_cuervo`, `gaviota` |

## Colocación

Y hacia arriba, unidades raylib. `casco_barco` y `cubierta` van en el origen; la superficie está en Y≈0 y el casco baja hasta Y=-3.15. `barandilla` se instancia en Z=2.25 y Z=-2.25. Para `cantidad` jugadores, cada mástil se sitúa en `x=(k−(cantidad−1)/2)×4.4`, Z=0; `mastil`, `vela`, `cofa`, `jarcias`, franja y bandera comparten ese pivote. La cofa está en Y=10 y el asta alcanza Y≈11.7. Si cambia el orden o color de participantes, seleccionar la franja y bandera correspondientes al color, no solo al índice.

Todo lo que pertenece al barco, incluidos jugadores, cuervos y mástiles, debe recibir el mismo giro de `AnguloBarcoMastil(inclinacion)` alrededor del origen. `mar` y `isla_lejana` se dibujan fuera de ese giro; el mar está a Y≈-4.5 y la isla se coloca en `(-22,-4.5,-42)`. `cuervo` sigue `(posicionX[i]+c.x,c.altura,.75)` y puede usar `ala_cuervo` como pieza animada. `manifest.json` recoge los límites y pivotes de las 25 piezas.

## Generación y comprobación

`python3 generar_modelos.py` reconstruye los GLB sin dependencias. `visor_raylib.cc` comprueba la carga en raylib; `--capturar RUTA_ABSOLUTA_DE_ESTA_CARPETA` produce imágenes en `Vistas/`. La extensión `.cc` evita que la tarea recursiva `*.cpp` añada el `main` del visor al juego. `VALIDACION.txt` documenta la prueba. Estos GLB todavía no sustituyen las primitivas de dibujo del minijuego.
