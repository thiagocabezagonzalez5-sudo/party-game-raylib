# Bolas de Azúcar · modelos GLB v1

20 GLB 2.0 originales para `Minigames/MinijuegoBolasAzucar.cpp` de `claude/expansion-party`, referencia del arte `ea5472c`. Integrados en el minijuego real mediante el almacén compartido de escenarios.

`GLB/` contiene las piezas; `manifest.json` registra dimensiones, materiales, pivote y uso. `Vistas/` incluye 25 capturas y `Vista_previa.png` presenta la colección. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB usando Python 3 estándar. `visor_raylib.cpp` carga y libera los modelos.

## Escala y ubicación

Una unidad GLB equivale a una unidad raylib; Y apunta arriba. Colocar `fondo_rosa` y `suelo_galleta` en `(0,0,0)`. La arena tiene 18×18 unidades, parte superior Y=0. Los cuatro `muro_galleta` se colocan en Z=±9,2 con eje longitudinal X, y en X=±9,2 girados 90°; mantienen el alto de colisión 2 del C++.

- `azucar_lateral` mide 4×9; repetir en `(−6,0,0)` y `(6,0,0)`. `azucar_extremo` mide 6×2,5; repetir en `(0,0,−6,75)` y `(0,0,6,75)`. El crecimiento de la bola debe seguir los rectángulos de la lógica existente.
- `chocolate_grande` va en el origen con radio 2,2. Dos `chocolate_pequeno` de radio 1,4 van en `(−5,5,0,−6,5)` y `(5,5,0,6,5)`. Conservar la pérdida de tamaño y velocidad que calcula el juego.
- `gominola_roja`, `gominola_verde`, `gominola_amarilla` y `gominola_azul` se centran respectivamente en X/Z `(−3,2,−3,2)`, `(3,2,−3,2)`, `(−3,2,3,2)` y `(3,2,3,2)`. El radio lógico es 0,8; conservar los rebotes y colisiones del C++.
- Instanciar siete `piruleta_*` al fondo, X=`−20+6,5*i`, Z=−16, y otras en `(−16,0,4)`, `(16,0,−3)`, `(17,0,9)`. Cuatro `columna_caramelo` van en X=±12,5, Z=±12. Tres `montana_nata` van en X=−22, −2 y 18, Z=−30, Y=−0,5.
- `bola_azucar` tiene radio base 1 y pivote central: colocar su centro en `(bola.x, bola.radio, bola.z)` y escalar uniformemente por `bola.radio` entre 0,3 y 1,4. Mantener el material de la bola blanco; el aro de cada jugador puede teñirse por separado o dibujarse con raylib. `pepita_chocolate` es un detalle opcional repetible.

Son mallas estáticas sin rig ni animación GLB. La física, vidas, crecimiento, derretimiento, proyectiles y jugadores permanecen en C++.

## Integración en el juego

Las rutas reales `Assets/Modelos/Escenarios/Bolas_de_Azucar/GLB/` están
centralizadas en `Core/RecursosJuego.h`. El paquete se carga al reiniciar una
partida válida, después de comprobar sus participantes. El catálogo no lo
carga al arrancar. Las instancias comparten las mallas/materiales del almacén;
reiniciar con R o entrar otra vez desde el selector no duplica recursos.

| GLB | Elemento sustituido y montaje |
| --- | --- |
| `fondo_rosa` | Plano de fondo, con origen global (0,0,0). |
| `suelo_galleta` | Cubo del suelo; conserva base, trama y detalles de esquinas. |
| `azucar_lateral`, `azucar_extremo` | Cuatro cubos de azúcar, en los centros de `ZONAS_AZUCAR_BOLAS`. |
| `chocolate_grande`, `chocolate_pequeno` | Dos cilindros por charco, usando `CHARCOS_BOLAS` y sus variantes. |
| `muro_galleta` | Cubo y glaseado de los cuatro muros; pivote al pie de `bloques[1..4]`, giro Y=90° en los laterales. |
| `gominola_roja`, `gominola_verde`, `gominola_amarilla`, `gominola_azul` | Cilindro, esfera y brillo de cada gominola, en `GOMINOLAS_BOLAS`. |
| `piruleta_rosa`, `piruleta_azul`, `piruleta_amarilla`, `piruleta_violeta`, `piruleta_verde` | Tronco y tres piezas de copa de las diez piruletas; mantienen posiciones y variantes actuales. |
| `columna_caramelo` | Dos cilindros por cada una de las cuatro columnas. |
| `montana_nata` | Cono, cereza y franja de chocolate de las tres montañas; pie en Y=-0,5. |
| `bola_azucar` | Esfera y wireframe del dueño; centro `(x,radio,z)`, escala uniforme `radio`, giro Z=`giro*RAD2DEG`. |
| `pepita_chocolate` | Cilindro de cada pepita, en los puntos procedurales actuales fuera de azúcar y chocolate. |

El suelo ya incluye pepitas en sus primitives 2 y 3. La integración omite
esas dos mallas al dibujarlo y usa `pepita_chocolate` en su lugar. Las vistas
de las otras tres mallas no cargan ni poseen recursos nuevos. Así el suelo
y la pieza modular conservan fallback independiente sin pepitas superpuestas.

Todos los GLB conservan sus pivotes y una unidad por unidad del juego. No se
normalizan a la base ni se alteran las hitboxes. No se usa una jerarquía extra
de `rlgl`: cada instancia recibe una única transformación explícita. Se evita
la macro de sombras de personajes al dibujar GLB y fallbacks, para no añadir
manchas incorrectas a suelo, azúcar, charcos, copas o indicadores. Las sombras
de los jugadores permanecen en el sistema compartido existente.

La bola usa exclusivamente el material `COLOR_DINAMICO` de su aro (primitive
2, resuelto con `meshMaterial`) para el color del dueño. El cuerpo conserva
sus colores originales; durante reposo solo cambia el alpha de sus dos
materiales, conforme a `inactiva`, y el aro también se desvanece. Todos los
colores se restauran tras cada instancia y el tinte global es blanco. Los
vértices y sus colores no se modifican. El giro sigue el marcador procedural
original que orbita en XY. Se mantienen ese marcador, las hitboxes de debug,
partículas, inmunidad, gestos de creación/aturdimiento y HUD.

Si una pieza falta o resulta incompatible, conserva únicamente sus primitivas
originales y registra el problema una sola vez. `ZonaPruebas::Descargar()`
libera el paquete mediante `DescargarModelosEscenariosRetro3D()` antes de
`CloseWindow()`, tanto desde pruebas/selector como en partidas de tablero.
No quedan modelos sin usar.

## Verificación de integración

Compilar con la tarea existente de Windows UCRT64. La prueba siguiente enlaza
el juego real e inicia una ventana OpenGL oculta, usando entradas simuladas:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarBolasAzucar.ps1
```

Comprueba 2, 3 y 4 participantes, preparación, creación, crecimiento,
lanzamiento, reposo, derretimiento, rebotes, impactos pequeños/grandes,
eliminación, final, IA, reinicio y salida, incluida la protección de ronda
oficial. Verifica materiales restaurados, vértices intactos, pivotes, escala,
carga única, ausencia de cada uno de los 20 archivos y descarga simétrica.
Las capturas `build/azucar-*.png` son del juego integrado con su cámara y HUD.
Los gamepads físicos requieren una comprobación manual adicional.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorBolasAzucar.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorBolasAzucar.exe
```

Teclas 1 a 4: cuatro jugadores, arena, dos jugadores y paisaje. Escape: salir. `--capturar` regenera las vistas. Los cubos de colores del visor son jugadores temporales.
