# Esferas del Cañón · modelos GLB v1

26 modelos GLB 2.0 originales para `Minigames/MinijuegoEsferasCanon.cpp` en la rama `claude/expansion-party` (referencia `ea5472c`). Se comprobaron cargándolos en raylib. **Integrados en el minijuego real mediante la caché compartida de escenarios.**

`GLB/` contiene las piezas; `manifest.json` registra límites, pivote, materiales y uso. `Vistas/` ofrece 31 capturas y `Vista_previa.png` presenta la colección. `generar_modelos.py` y `geometria_glb.py` permiten regenerar los archivos con Python 3. `visor_raylib.cpp` es un visor C++17 que carga y descarga todas las mallas.

## Escala y ensamblado

Una unidad GLB equivale a una unidad raylib; Y apunta hacia arriba. La curva reproduce los 14 puntos de control, la interpolación Catmull-Rom, la pendiente `Y = -0.12*S` y la pista de ancho 8,4 del código. La longitud de la curva es 142,23; la meta está en S=120. Las piezas cuyo pivote dice **origen global** se colocan todas en `(0,0,0)`: cuatro `tramo_pista_*`, ocho `pared_*`, tres `puente_roto_*`, `rampa_atajo`, `arco_natural` y `suelo_desertico`. No hay que trasladarlas individualmente a sus respectivos valores S.

- Grieta 1: S=40..43,5, centro de paso lateral +1,5 y semiancho 1. Grieta 2: S=74..77,5, centro −1,5 y semiancho 1. Grieta 3: S=96..102, centro 0 y semiancho 0,8. Los puentes de madera marcan estas franjas seguras; conservar la física y las penalizaciones originales.
- La rampa ocupa S=90..93,5 y lateral +2..+3,6. Los tramos muestran arena suelta en S=18..26, 64..71 y 108..116. La malla no reemplaza la lógica de rozamiento o salto.
- Colocar parejas de `banderin_checkpoint` en S=28, 58 y 88, lateral ±3,9. `arco_salida` va en S=0,8 y `arco_meta` en S=120. Girar sus ejes X para dejarlos perpendiculares a la tangente local. Sus pivotes están al pie y en el centro; ver `visor_raylib.cpp`.
- `mesa_lejana` se instancia fuera de la pista, X≈±20..30. `cactus` y `roca_caida` se instancian con los datos de obstáculo existentes. En el visor se añaden también cactus sobre las paredes.
- `esfera_piedra` tiene pivote en su centro y radio visual ≈1; centro en `Y = -0.12*S + 1`, más la elevación del salto si corresponde. Rotarla con `estado.giro`, `estado.ejeX` y `estado.ejeZ`. `aro_jugador` se coloca en el suelo y puede teñirse con `participantes[i].color`.

Los modelos son estáticos, sin animaciones GLB ni colisiones embebidas. Al integrar, conservar cálculos de pista, proyección, IA, física, controles y cámara. Cargar cada modelo una sola vez y descargarlo al cerrar. El suelo lejano es decorativo y no colisiona. La integración conserva los archivos GLB originales y añade únicamente su conexión al dibujo del juego.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorEsferasCanon.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorEsferasCanon.exe
```

Teclas `1` a `4`: escenas; flechas izquierda/derecha: recorrer el cañón; Escape: salir. `--capturar` regenera las vistas. Los bloques pequeños de color en el visor son referencias temporales de jugadores.

## Integración en el juego

Los 26 GLB tienen uso; no quedan modelos pendientes. Las rutas reales están en
`Core/RecursosJuego.h`. `Minigames/ModelosEscenariosRetro3D.h` conserva una sola
copia de cada recurso, cargada al iniciar una ronda válida de Esferas. Reiniciar,
regresar desde el selector y activar una partida de tablero reutilizan esa copia.
`ZonaPruebas::Descargar()` libera la caché, también en el cierre de `Juego`, antes
de `CloseWindow`. El minijuego no posee modelos y no necesita un `Descargar()` propio.

| Archivos GLB | Sustitución y estado real |
|---|---|
| `tramo_pista_1..4` | Lecho, arena y grietas visuales de S=0..40, 40..74, 74..96 y 96..130. |
| `pared_izquierda_1..4`, `pared_derecha_1..4` | Bandas de roca de cada lado, con origen global. |
| `puente_roto_1..3` | Tablones y cuerda de las tres franjas seguras. |
| `rampa_atajo` | Superficie y bordes de la rampa, con la lógica original de salto. |
| `arco_natural` | Dos pilares y dintel cerca de S=12, globales. |
| `mesa_lejana` | Diez mesas; posición original y escala solo Y según su altura procedural. |
| `cactus` | Diez obstáculos reales y dieciocho cactus sobre las paredes; escala explícita para la decoración. |
| `roca_caida` | Diez obstáculos reales; posición y radio visual derivados de `obstaculos[]`. |
| `banderin_checkpoint` | Seis postes y banderas en S=28/58/88, lateral ±3,9. |
| `arco_salida`, `arco_meta` | Cuadrícula, postes y dintel en S=0,8 y S=120. |
| `esfera_piedra` | Cuerpo y manchas; centro de `jugador.posicion`, radio 1, giro y eje de `estado`. |
| `aro_jugador` | Indicador circular al pie de cada esfera, con color del participante y alpha original. |
| `suelo_desertico` | Plano lejano decorativo incluido en el paquete; sin colisiones. |

No se centra ninguna pieza ni se modifica su `Model.transform`. Las piezas globales
se dibujan en `(0,0,0)` con escala 1; los elementos locales usan `DrawModelEx` sin
combinarlo con traslaciones o rotaciones adicionales de rlgl. El yaw de arcos y
banderas usa el signo de rotación de raylib: el eje X del arco queda perpendicular
a la tangente y ambas banderas apuntan hacia el interior. Esto corrige el signo
usado por el visor, sin cambiar sus posiciones ni el mapa lógico.

El arte de tramos y paredes acaba en S=130; la curva lógica termina en S≈142,23.
Ese tramo final conserva sus primitivas. Si falla una pieza, únicamente su intervalo
o instancia vuelve al dibujo original, recortado en las fronteras del paquete para
no superponerlo a los GLB vecinos. El suelo lejano tiene un cubo plano como alternativa.
El diagnóstico aparece una vez por recurso, sin reintentar la carga cada frame.
Los GLB y las primitivas de fallback evitan los macros de sombras humanas en Y=0 de
`SombrasRetro.h`, que no corresponden al suelo inclinado del cañón.

Solo `aro_jugador` tiene `COLOR_DINAMICO`. Se obtiene su material mediante
`meshMaterial[0]`, se cambia temporalmente su diffuse y se restaura después de cada
dibujo. Todos los dibujos usan tinte blanco y conservan los demás materiales y los
colores de vértice. La esfera sigue el salto, caída y parpadeo originales; jugadores,
HUD, indicadores, reglas, cámara, IA, hitboxes y física siguen siendo los del minijuego.

## Verificación de la integración

Desde la raíz del repositorio:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarEsferasCanon.ps1
```

La prueba compila C++17 con el mismo UCRT64/raylib del proyecto y ejecuta
`ZonaPruebas` real con contexto OpenGL y entradas de teclado simuladas. Comprueba
2, 3 y 4 participantes; preparación, impulso, giro, choques, checkpoints, puentes,
caída, reaparición, parpadeo, salto, meta, cierre, clasificación, IA, reinicio y salida.
También observa transformaciones, colores restaurados, ausencia de cargas de otros
mapas y ausencia de primitivas duplicadas. Simula por separado el fallo de los 26
archivos sin mover recursos, y verifica diagnóstico único y descarga simétrica.

Las capturas `build/esferas-preparacion.png`, `esferas-caida.png`, `esferas-rampa.png`
y `esferas-final.png` proceden del juego integrado, con su cámara y HUD. El visor
independiente del paquete sigue disponible únicamente como referencia de los modelos.
Esta prueba no verifica gamepads físicos ni conexión/desconexión de dispositivos.
