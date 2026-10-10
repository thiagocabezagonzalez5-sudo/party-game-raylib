# Volea de Magma · modelos GLB v1

Diecisiete GLB 2.0 originales para `Minigames/MinijuegoVoleaMagma.cpp` de `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib con el visor incluido. **Integrados en el minijuego real mediante el almacén compartido de escenarios.**

`GLB/` contiene las piezas; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 21 capturas de raylib; `Vista_previa.png` muestra el escenario y ocho piezas. `generar_modelos.py` y `geometria_glb.py` regeneran las mallas con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga todos los modelos.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; la red divide los equipos en X=0. La cámara existente está en `(0,9,14.5)`, mirando a `(0,1.6,0)`.

- `cancha_obsidiana` y `borde_cancha` se colocan en `(0,0,0)`. La plataforma completa mide 20×12 y tiene superficie jugable 18×10. El suelo lógico conserva su bloque de colisión original; el borde no debe cambiar los límites de jugador.
- `red_cadenas` se coloca en `(0,0,0)`, con plano X=0, de Z=−5,4 a +5,4 y cadena principal hasta Y=2,4. Repetir `poste_red` en `(0,0,−5.4)` y `(0,0,5.4)`. Desde la cámara frontal actual la red queda casi de canto; `Escena_general.png` muestra su volumen desde un ángulo oblicuo.
- `lago_lava` se coloca en `(0,0,0)`. `volcan_menor` va en `(−28,−1.5,−38)` y `volcan_mayor` en `(26,−1.5,−44)`, con alturas respectivas 20 y 26. `columna_basalto` tiene altura base 4: colocar su pie en Y=−1,5 y escalar **solo Y** con `altura/4`. Las columnas con llama van en X=±12,2, Z=±3,5.
- `roca_magma` y `roca_sobrecalentada` tienen pivote central y radio visual aproximado 0,45. Instanciar una u otra en `pelota.posicion` según `pelota.temperatura>=0.85`. `estela_ascua`, `sombra_pelota` e `indicador_caida` son piezas separadas para animar con los estados existentes.
- `charco_lava` va en `(charco.x,0,charco.z)` únicamente mientras `charco.activo`; su radio es 1,8. El tiempo de vida de 4 segundos y el efecto de lentitud continúan en C++. `burbuja_lava` y `ceniza` se repiten como decoración sin colisiones.

Son mallas estáticas modulares sin rig ni animaciones GLB. La integración usa la carpeta real `Assets/Modelos/Escenarios/Volea_de_Magma/GLB/`, con rutas centralizadas en `Core/RecursosJuego.h`, sin mover ni regenerar los recursos.

## Integración en el juego

Se usan los 17 GLB; no quedan modelos pendientes. El paquete se carga al reiniciar una ronda válida de Volea de Magma, una vez por recurso, y comparte mallas entre instancias. Inicializar el gestor, reiniciar con `R` o reentrar no duplica cargas. `DescargarModelosEscenariosRetro3D`, invocado por `ZonaPruebas::Descargar` y el cierre de `Juego`, libera cada modelo antes de `CloseWindow`. El minijuego no posee modelos propios y no requiere un nuevo método de descarga.

| Modelos | Elemento visual integrado |
|---|---|
| `lago_lava` | Reemplaza el bloque de lava, en `(0,0,0)` con alturas locales originales. |
| `cancha_obsidiana` | Losa, superficie y cinco líneas del campo. Conserva el bloque lógico de colisión 20×1×12. Las dos marcas translúcidas de equipo permanecen como indicadores. |
| `borde_cancha` | Completa el perímetro de la plataforma; respaldo propio de ocho cubos de borde y luz, sin volver a dibujar la cancha. |
| `poste_red`, `red_cadenas` | Postes, luces, dieciocho segmentos de cadena y dintel. Pivotes originales en `(0,0,±5.4)` y `(0,0,0)`; pulso `sin(t*4)`. |
| `volcan_menor`, `volcan_mayor` | Conos, cráteres y coladas, en las posiciones originales `(-28,-1.5,-38)` y `(26,-1.5,-44)`. Conserva las doce esferas de humo con movimiento y alpha originales. |
| `columna_basalto`, `columna_con_llama` | Diez columnas posteriores y cuatro laterales. Pie en Y=-1,5 y escala `(1,altura/4,1)`; conserva distribución y alturas del código, no las distribuciones demostrativas del visor. |
| `roca_magma`, `roca_sobrecalentada` | Dos esferas del cuerpo y detalle de la pelota, con radio visual aproximado 0,45, pivote central y posición actual. Variante según `temperatura>=0.85`. Conserva el halo procedural sobrecalentado. |
| `charco_lava` | Dos cilindros por charco activo; origen `(x,0,z)`, radio 1,8, pulso `sin(t*7+i)` y desvanecimiento del último segundo de sus cuatro segundos. |
| `sombra_pelota` | Cilindro de sombra. Sigue X/Z de la pelota; instancia Y=0,03 para dejar el disco local Y=0,015..0,027 por encima de las losas Y=0,031 y conservar la altura del cilindro original. Escala solo X/Z por `max(0.3,0.45*(1.25-0.08*clamp(y,0,9)))/0.48`. |
| `indicador_caida` | Dos círculos de predicción; únicamente en vuelo y dentro del filtro visual actual. Aro exterior y luces pulsan en X/Z; aro interior fijo. |
| `burbuja_lava` | Esferas fuera de la cancha, con posiciones, vaivén, radio y variación de color originales. Escala uniforme `radio/0.48`. |
| `ceniza` | Cuarenta cubos, con trayectorias actuales, tamaño 0,09 y alpha 200. |
| `estela_ascua` | Siete esferas de trayectoria. Usa `pelota.estela[k]`, escala uniforme `radio/0.18` y alpha `0.5*(1-k/8)`. |

Todas las piezas conservan sus pivotes, materiales y colores de vértice. Este paquete **no contiene COLOR_DINAMICO**; las variaciones existentes se resuelven por `meshMaterial` y se restauran después de cada dibujo:

- `roca` de roca_magma y `lava` de roca_sobrecalentada: `ColorRoca(temperatura,t)`; detalles, grietas y reflejos conservan sus colores originales.
- `cadena` de red_cadenas y `lava_clara` de poste_red: brillo actual de las cadenas; metal, herrajes y restantes materiales intactos.
- `lava_clara` de burbuja_lava: color procedural de la burbuja; BOMBILLAS y detalle amarillo intactos.
- `lava_roja` y `lava` de charco_lava: colores de sus dos cilindros originales. `lava_clara` y `lava_amarilla` conservan RGB. Solo la opacidad se desvanece en las cuatro mallas.
- `ceniza`: conserva RGB y aplica alpha 200. `lava_clara` de estela_ascua usa el color actual de la roca; `lava_amarilla` conserva RGB. Ambas mallas de estela se desvanecen.

Las vistas por malla comparten VBO y materiales sin adquirir propiedad. Cada pieza fallida mantiene su fallback y registra el problema una sola vez. El dibujo de escenario y sus efectos evita la macro de `SombrasRetro.h`: no agrega sombras humanas a humo, lava, ceniza ni halo, y la pelota conserva exclusivamente su sombra dedicada. No se combinan matrices locales `rlTranslatef`/`rlRotatef` con `DrawModelEx`.

Física, cámara, controles, IA, equipos 1vs1/2vs1/2vs2, ventaja de minoría, reglas de toques, tiempos, puntuación y colisiones siguen en el código original.

## Verificación de integración

Compilar con la tarea UCRT64 existente. La prueba adicional compila los archivos reales del proyecto y ejecuta `ZonaPruebas` y `GestorMinijuegos` con contexto OpenGL:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarVoleaMagma.ps1
```

Cubre 2, 3 y 4 participantes, preparación, saque, remate, toques, red, temperatura, charcos, ralentización, enfriamiento, siete puntos, final por tiempo, punto de oro, IA, reinicio, regreso al catálogo y bloqueo de abandono/reinicio del modo tablero. Observa materiales durante el dibujo, pivotes, transformaciones, visibilidad, ausencia de primitivas duplicadas, fallback de los 17 GLB, carga única y descarga simétrica.

Las capturas `build/volea-*.png` provienen del minijuego integrado. Entradas y situaciones específicas se simulan; no sustituyen una prueba con gamepads físicos. El visor y `Vistas/` son referencias del paquete, no pruebas de esta integración.

## Visor independiente (referencia del paquete)

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorVoleaMagma.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorVoleaMagma.exe
```

Teclas 1: escena; 2: cámara del juego; 3: roca sobrecalentada; 4: cancha; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los cuerpos cúbicos de colores del visor son referencias de escala para los jugadores y no son GLB del paquete.
