# Rodillos Neón · modelos GLB v1

21 GLB 2.0 originales para `Minigames/MinijuegoRodillosNeon.cpp` de `claude/expansion-party`, referencia del arte `ea5472c`. Integrados en el minijuego real mediante el almacén compartido de escenarios.

`GLB/` contiene las piezas; `manifest.json`, sus dimensiones, pivotes y uso. `Vistas/` tiene 26 capturas y `Vista_previa.png` muestra la colección. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y libera las 21 mallas.

## Escala y montaje

Una unidad GLB equivale a una unidad raylib. Eje Y hacia arriba y frente de las máquinas hacia +Z. `suelo_arcade` y `pared_arcade` se colocan en el origen. Para N participantes, centro X de cada máquina `((i-(N-1)/2)*5.8)`, Z=0. `gabinete_arcade` y `marco_jugador` comparten ese origen; el marco se puede teñir por participante. `boton_detener` se ubica en `(X,1.2,1.9)`.

Cada máquina lleva tres instancias de `tambor_rodillo`, centradas en `(X-1.05,3,0)`, `(X,3,0)` y `(X+1.05,3,0)`. Radio de tambor 1,7; gira sobre X en grados `-rodillo.angulo*RAD2DEG`. Para cada uno de los diez valores `k` de la tira aleatoria, colocar el `simbolo_*` de tipo `rodillo.tira[k]` en el espacio local del tambor: `Y=1.73*sin(k*2π/10)`, `Z=1.73*cos(k*2π/10)` y orientación X=`-k*36°`. El giro del tambor debe heredarse por sus símbolos. El visor muestra esa jerarquía y una tira de ejemplo; no guarda la baraja ni el estado del juego en un GLB. La `linea_comodin` va en `(X,3,1.84)`; conservar la ventana temporal y puntuación del C++.

Instanciar `columna_led_cian`/`columna_led_rosa` alternadas en `X=-24+6*c`, Z=−8,5. Colocar tres hologramas en `X=-12+12*k`, Y≈9,5, Z=−6 y rotarlos si se desea. Los cuatro `letrero_*` van en `X=-18+12*k`, Y=12,3, Z=−8,4. El brillo se representa mediante materiales de color intenso; para un halo real se requiere el posprocesado de iluminación del proyecto.

Las mallas son estáticas y no traen rig ni animaciones GLB. El juego conserva el giro, el frenado, la línea de comodín, los glitches, las cinco rondas, la IA y la cámara existentes.

## Integración en el juego

Las rutas reales `Assets/Modelos/Escenarios/Rodillos_Neon/GLB/` se centralizan en
`Core/RecursosJuego.h`. El paquete se carga al reiniciar una partida válida de
Rodillos Neón, nunca al inicializar el catálogo ni por frame. Todas las máquinas
comparten las 21 mallas; reiniciar o volver desde el selector conserva la caché.
`ZonaPruebas::Descargar()` libera modelos y copias CPU mediante el almacén
compartido antes de `CloseWindow()`, también en el camino de tablero.

| GLB | Primitivas reemplazadas y estado utilizado |
| --- | --- |
| `suelo_arcade` | Base y rejilla del suelo; pulsos por columna según `tiempoAnimacion`. |
| `pared_arcade` | Pared y líneas verticales de fondo, en origen global. |
| `gabinete_arcade` | Seis bloques del gabinete y los tres aros superiores. |
| `marco_jugador` | Cuatro bordes; `COLOR_DINAMICO` recibe el color del participante. |
| `boton_detener` | Base y esfera; cúpula dinámica con pulso/opacidad según rodillos pendientes y etapa. |
| `tambor_rodillo` | Cilindro horizontal; rotación X real de giro, frenado o glitch. |
| `simbolo_triangulo`, `simbolo_circulo`, `simbolo_cuadrado`, `simbolo_rombo`, `simbolo_estrella` | Polígonos de cada tipo; usan `tira[k]` y `angulo`, filtrados a la cara frontal como antes. |
| `linea_comodin` | Línea blanca y flechas; sus dos bordes amarillos siguen la ventana temporal de la ronda. |
| `columna_led_cian`, `columna_led_rosa` | Base y diez luces por columna; pulsos por luz, columna y tiempo. |
| `holograma_cubo`, `holograma_esfera`, `holograma_piramide` | Tres formas wireframe; conservan flotación, giro Y e inclinación X. |
| `letrero_cian`, `letrero_rosa`, `letrero_verde`, `letrero_naranja` | Cuatro marcos con símbolo, en sus coordenadas actuales. |

Todos conservan el pivote importado y escala 1. El gabinete y marco comparten
la traslación de la máquina; los tambores reciben además Y=3. Los símbolos
componen el ángulo total una sola vez, con radio documentado 1,73. Los
hologramas heredan las dos rotaciones procedurales de `rlgl`; su instancia no
repite esas transformaciones. Los GLB y fallbacks evitan las macros de sombras
para personajes: no generan manchas de suelo incorrectas dentro de las máquinas.

Los únicos tintes de participante afectan `COLOR_DINAMICO`: primitive 0 del
marco y primitive 2 del botón, resueltos con `meshMaterial`. Durante el glitch
solo la cara de cada símbolo (primitive 0) actúa como material dinámico; los
contornos y detalles conservan sus materiales. Se restaura cada color después
del dibujo y se usa tinte global blanco. Los pulsos del suelo y LEDs modifican
solo el alpha de los vértices de las luces, manteniendo RGB y materiales.
Las copias de reposo y VBO se restauran entre instancias. La línea mueve
únicamente sus dos bordes, conserva flechas y normales y restaura posiciones.

Se mantiene la banda translúcida procedural de comodín, que expresa su ventana
temporal; el GLB aporta línea y bordes, sin reemplazar esa transparencia.
También se conservan halos al detener, halo animado de comodín, aviso de
glitch, personaje, gestos y HUD. Si falla una pieza, se dibujan solo sus
primitivas originales y se registra un diagnóstico una vez. No quedan GLB sin usar.

## Verificación de integración

Compilar con la tarea existente de Windows UCRT64. La prueba siguiente enlaza
los archivos del juego, inicia una ventana OpenGL oculta y ejecuta
`ZonaPruebas` y el minijuego reales con entradas simuladas:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarRodillosNeon.ps1
```

Comprueba 2, 3 y 4 participantes, carga única, pivotes y giro, pulsos, restauración
de materiales y buffers, preparación, ambos glitches, frenado, comodines,
puntuación, cinco rondas con IA, final, reinicio, salida y protección de ronda
oficial. Simula la ausencia de cada archivo sin mover recursos y verifica el
fallback local y descarga simétrica. Las capturas `build/rodillos-*.png` provienen
del juego integrado con su cámara y HUD. Los gamepads físicos requieren una
comprobación manual adicional.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorRodillosNeon.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorRodillosNeon.exe
```

Teclas 1 a 4: cuatro máquinas, giro de ejemplo, dos máquinas y sala. Escape: salir. `--capturar` regenera las vistas. Los cubos de colores del visor representan jugadores temporales.
