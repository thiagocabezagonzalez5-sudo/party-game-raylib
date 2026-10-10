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
- `oruga_cabeza` representa el primer segmento vivo y `oruga_segmento` se instancia para los demás `segVivo[s]`, por separado, en `segX[s]`/`segZ[s]`. Orientar el eje +X según `dirX,dirZ`. Aplicar `aparicion*escala` visual a las plagas. El destello de aturdimiento usa alambre y las estrellas originales, porque los cuerpos no contienen `COLOR_DINAMICO`.
- `avispa_cuerpo` tiene origen a la altura de vuelo (`altura≈0.95`). Instanciar dos `avispa_ala` en los lados ±Z del cuerpo y animarlas con `sin(t*55)*0.12`; conservar movimiento y detección de golpes existentes.
- La flor de disputa usa `flor_brote` durante el aviso, `flor_abierta` al florecer y `aro_flor` de radio 1.7 en `(florX,0,florZ)`. El aro no determina la puntuación; la regla sigue en el código del minijuego.

## Reconstruir y revisar

Ejecutar `python3 generar_modelos.py` para recrear los GLB con Python estándar. `visor_raylib.cpp` carga los 22 modelos y captura `Vistas/`; requiere raylib y SDL para compilar. `Camara_del_juego.png` usa la cámara `(0,15,10.5)` hacia `(0,0,0.6)` con FOV 50°. `VALIDACION.txt` recoge la inspección técnica.

## Integración en el juego

Los 22 GLB se dibujan desde `MinijuegoPisotonPlagas.cpp`, con rutas en `Core/RecursosJuego.h` y una sola copia compartida de cada recurso en `ModelosEscenariosRetro3D.h`. No hay modelos pendientes. Se cargan al iniciar la primera ronda válida; reiniciar conserva la caché. La descarga global de Zona de Pruebas libera modelos y copias CPU de reposo antes de `CloseWindow`, también en selector y partida de tablero.

| GLB | Primitivas reemplazadas y estado conectado |
| --- | --- |
| `suelo_jardin` | Césped lejano, bloque visual, parches y mechones cortos. El bloque físico continúa intacto. |
| `brizna_gigante` | 34 briznas con las posiciones, alturas y vaivén originales. |
| `flor_fondo_rosa`, `flor_fondo_amarilla`, `flor_fondo_violeta` | Tallo, pétalos y centro de cada flor del fondo. |
| `seta`, `regadera`, `piedra` | Decoraciones en sus posiciones originales; piedras con tamaños alternados. |
| `cerca_fondo`, `cerca_lateral` | Postes y travesaños; laterales rotados 90° en Y. |
| `gota_rocio` | Diez gotas, con opacidad pulsante y posiciones procedurales originales. |
| `madriguera`, `aro_aviso` | Montículo y aviso; conservan temblor, elevación y aviso naranja. |
| `flor_brote`, `flor_abierta`, `aro_flor` | Tallo, brote, ocho pétalos, centro y aro. El tallo crece y los pétalos se separan gradualmente y giran con el tiempo actual. El aro aparece durante la floración; durante el aviso permanece el indicador lleno de disputa. |
| `escarabajo` | Cuerpo, cara, patas y antenas; seis patas con movimiento alternado según `edad`, suspendido al aturdir. |
| `oruga_cabeza`, `oruga_segmento` | Segmentos vivos y cara. La cabeza pasa al primer segmento vivo; cada pieza sigue posición, orientación, aparición, escala y bote. |
| `babosa_dorada` | Cuerpo, cola, tentáculos y ojos, conservando el estiramiento procedural de la cola y los brillos. |
| `avispa_cuerpo`, `avispa_ala` | Cuerpo y dos alas por avispa activa, con posición, dirección, altura y aleteo reales. La huida conserva un destello naranja como efecto y oculta la marca de peligro. |

Todos conservan el pivote nativo, sin centrar automáticamente ni aplicar dos veces transformaciones de instancia. El único material de estado es `COLOR_DINAMICO` en `aro_aviso`, resuelto mediante `meshMaterial`. Rocío y aro de flor pulsan la opacidad de instancia con RGB blanco en `DrawModelEx`, sin editar sus materiales. Se conservan RGB y colores de vértice; materiales y mallas deformadas quedan restaurados después de cada instancia. No se cargan clips de animación.

Patas, cola y pétalos usan los VBO compartidos del GLB y copias CPU de reposo, sin duplicar modelos por plaga. Se valida la estructura esperada del paquete al cargar. Cada pieza mantiene su fallback original y un diagnóstico único si falta o falla su recurso. El dibujo directo evita las sombras automáticas de `SombrasRetro.h` sobre decoraciones; permanecen sombras explícitas de plagas, sombras compartidas de jugadores, indicadores, partículas y popups.

## Prueba integrada

Desde la raíz del repositorio, con Windows UCRT64:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarPisotonPlagas.ps1
```

El script compila con C++17 el código real del juego y ejecuta Zona de Pruebas con OpenGL y entradas simuladas. Comprueba 2, 3 y 4 participantes; preparación, movimiento, ground pound de las tres plagas, pérdida parcial de oruga, aturdimiento, picadura y huida de avispas, floración, aparición y escape, IA, final, R, ESC y protección de ronda oficial. Verifica carga única, restauración de materiales y mallas, transformaciones de alas, fallback individual de los 22 archivos y descarga simétrica. Las capturas `build/pisoton-*.png` corresponden al juego integrado. No reemplaza una prueba manual de gamepads físicos ni de audio.
