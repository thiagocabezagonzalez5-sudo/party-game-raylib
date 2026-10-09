# Pisotón de Plagas — modelos v1

22 modelos originales GLB 2.0 de geometría simple y colores planos, adaptados a `MinijuegoPisotonPlagas.cpp` de la rama `claude/expansion-party` (`ea5472c`). Una unidad GLB equivale a una unidad raylib, con Y hacia arriba. Cada archivo se puede sustituir por arte definitivo conservando la lógica.

## Archivos

| Conjunto | Modelos |
| --- | --- |
| Jardín | `suelo_jardin`, `brizna_gigante`, `flor_fondo_rosa`, `flor_fondo_amarilla`, `flor_fondo_violeta`, `seta`, `regadera`, `piedra`, `cerca_fondo`, `cerca_lateral`, `gota_rocio` |
| Eventos | `flor_brote`, `flor_abierta`, `aro_flor`, `madriguera`, `aro_aviso` |
| Plagas | `escarabajo`, `oruga_cabeza`, `oruga_segmento`, `babosa_dorada`, `avispa_cuerpo`, `avispa_ala` |

`manifest.json` detalla pivotes, límites, materiales y colocación por archivo. El material `COLOR_DINAMICO` permite teñir el aviso de madriguera; las superficies `BOMBILLAS` identifican los reflejos. El aro de flor es geometría abierta para conservar visibles a las plagas.

## Montaje

- Arena jugable X=±9 y Z=±5.6. Cerca del fondo en `(0,0,-6.9)` y laterales en X=±10.4, con rotación Y=90°. Vegetación alta detrás de la cerca; las flores grandes están a X=-7, 1.5 y 8, con Z=-9.4, -10.6 y -9.4.
- Seis madrigueras: X=(-7.2,0,7.2), Z=(-4.3,-4.7,-4.3) y X=(-7.2,0,7.2), Z=(4.3,4.7,4.3). Durante `aviso`, animar el montículo según la elevación existente y activar `aro_aviso` con color naranja.
- `oruga_cabeza` representa el primer segmento vivo y `oruga_segmento` se instancia para los demás `segVivo[s]`, por separado, en `segX[s]`/`segZ[s]`. Orientar el eje +X según `dirX,dirZ`. Aplicar `aparicion*escala` visual a las plagas y el tinte del destello cuando estén aturdidas.
- `avispa_cuerpo` tiene origen a la altura de vuelo (`altura≈0.95`). Instanciar dos `avispa_ala` en los lados ±Z del cuerpo y animarlas con `sin(t*55)*0.12`; conservar movimiento y detección de golpes existentes.
- La flor de disputa usa `flor_brote` durante el aviso, `flor_abierta` al florecer y `aro_flor` de radio 1.7 en `(florX,0,florZ)`. El aro no determina la puntuación; la regla sigue en el código del minijuego.

## Reconstruir y revisar

Ejecutar `python3 generar_modelos.py` para recrear los GLB con Python estándar. `visor_raylib.cpp` carga los 22 modelos y captura `Vistas/`; requiere raylib y SDL para compilar. `Camara_del_juego.png` usa la cámara `(0,15,10.5)` hacia `(0,0,0.6)` con FOV 50°. `VALIDACION.txt` recoge la inspección técnica.

Este paquete contiene assets y visor; aún no sustituye las primitivas `Draw*` del repositorio.
