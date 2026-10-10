# Pesca Isleña · modelos GLB v1

18 GLB 2.0 originales para `Minigames/MinijuegoPescaIsla.cpp` en `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib. **Integrados en el minijuego real mediante la caché compartida de escenarios.**

`GLB/` contiene los modelos; `manifest.json`, sus dimensiones, pivotes y usos. `Vistas/` tiene 23 capturas y `Vista_previa.png` una selección. `generar_modelos.py` y `geometria_glb.py` regeneran los modelos con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y libera los 18 modelos.

## Escala y ubicación

Una unidad GLB equivale a una unidad raylib. Y apunta hacia arriba. `oceano`, `isla_laguna` y `superficie_laguna` se colocan en `(0,0,0)`. La isla tiene radio 15, la laguna radio 8,5, el fondo Y=−1,8 y ondas sobre Y=0. El océano tiene una abertura circular para que no oculte los peces bajo la laguna. El modelo `superficie_laguna` representa el fondo de agua turquesa y las ondas: no es una tapa opaca sobre los peces.

- Instanciar `muelle_bambu` en `(0,0,10.5)`, `(0,0,−10.5)`, `(−10.5,0,0)` y `(10.5,0,0)`. Girar 0°, 180°, −90° y 90°, respectivamente. Su tarima superior ronda Y=0,4; la posición lógica del jugador sigue siendo Y=0,35 + 0,72.
- `cana_pescar` tiene origen en el mango: colocarla a Y=1,45 sobre cada muelle y girar como ese muelle. Su extremo se dirige hacia el centro. La línea de pesca sigue calculándose en C++ entre la punta y el anzuelo; `corcho` se centra en ese anzuelo y `cursor_lanzamiento` en el cursor a Y=0,09.
- Ocho `palmera` van a radio 13,2 y ángulos `22,5° + 45°*k`. `barca` se coloca en `(−12,0,−11)`, `volcan` en `(0,0,−21)` y `humo_volcan` comienza en `(0,7,−21)`. `gaviota` puede moverse por el vuelo orbital existente.
- Distribuir 16 instancias de `coral_1`/`coral_2` dentro de la laguna, en Y=−1,8. `pez_pequeno`, `pez_mediano` y `pez_dorado` tienen pivote central; colocar su centro en `(pez.x,−0.9,pez.z)` y orientar el eje +X hacia `(dirX,dirZ)`. `bota_vieja` usa la misma posición lógica. Los valores del juego son 1, 2, 5 y 0 puntos, respectivamente.

Son modelos estáticos sin rig, animaciones ni colisiones embebidas. Conservar los estados de pique, tensión, capturas y colisiones de C++. Cargar una vez cada GLB y descargarlo al salir. La integración mantiene los archivos GLB originales, sin moverlos ni regenerarlos.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorPescaIslena.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorPescaIslena.exe
```

Teclas 1 a 4: escenas; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las vistas. Los bloques de colores son referencias temporales de jugadores.

## Integración en el juego

Los 18 GLB tienen uso. Las rutas reales de `Pesca_Islena/GLB` están centralizadas
en `Core/RecursosJuego.h`. `ModelosEscenariosRetro3D.h` guarda una única copia de
cada modelo y prepara las animaciones al iniciar una ronda válida. No carga Pesca
al inicializar el catálogo. La caché sobrevive al reinicio y al regreso desde selector
o tablero. `ZonaPruebas::Descargar()`, incluido el cierre de `Juego`, libera modelos
y copias CPU de animación antes de `CloseWindow`. El minijuego no posee esos recursos
y no necesita un `Descargar()` propio en su encabezado.

| GLB | Elemento reemplazado y conexión al juego |
|---|---|
| `oceano` | Plano marino; origen global y abertura que permite ver la laguna. |
| `isla_laguna` | Arena, fondo y paredes de la laguna; origen global. |
| `superficie_laguna` | Agua y ondas fijas del paquete; sin tapa opaca sobre los peces. Las ondas expansivas animadas continúan. |
| `muelle_bambu` | Tarima, seis tablas y cuatro pilares de cada uno de los cuatro muelles, con pivote en Y=0. |
| `cana_pescar` | Caña entre la mano y `PuntaCanaPesca`, visible para cada participante activo. |
| `corcho` | Dos esferas del anzuelo; conserva trayectoria de lanzamiento, espera, hundimiento al pique y acercamiento por tensión. |
| `palmera` | Ocho troncos, hojas y cocos; mismas posiciones y balanceo alrededor del pie. |
| `barca` | Casco, mástil y vela en `(-12,0,-11)`. |
| `gaviota` | Cuatro parejas de líneas de alas; conserva órbita, altura oscilante y aleteo. |
| `volcan` | Cono y boca de lava en `(0,0,-21)`. |
| `humo_volcan` | Cuatro partículas de humo, con posición, tamaño y desvanecimiento actuales. |
| `coral_1`, `coral_2` | Dieciséis corales; mismos datos de `HashPesca`, base Y=-1,8 y altura variable. |
| `pez_pequeno`, `pez_mediano`, `pez_dorado` | Silueta y cola de cada tipo, según posición, rumbo, semilla y estado de `peces[]`. |
| `bota_vieja` | Dos cubos de la bota; sigue posición y visibilidad del tipo bota. |
| `cursor_lanzamiento` | Círculos y cruz de apuntado; posición, pulso y color de `estados[]`/`coloresJugadores[]`. |

Todas las piezas conservan sus pivotes y `Model.transform`; no se centran en la base.
El dibujo usa posiciones, ejes, rotaciones y escalas explícitas, sin aplicar otra
transformación de rlgl. El eje +X de cada pez apunta a `(dirX,dirZ)` con el signo de yaw
de raylib. Muelles y cañas giran 0°, 180°, -90° y 90°. La caña se coloca a Y=1,45 y
escala únicamente Y por `1,25/1,6` para conectar su extremo con la punta original a
Y=2,7, manteniendo la línea de pesca y la posición del jugador. La escala Y de los
corales mantiene la altura procedural de sus ramas.

La gaviota y las colas animan únicamente sus vértices móviles sobre copias de reposo;
conservan normales válidas, materiales y RGB. El humo usa los cuatro grupos de vértices
del GLB para reproducir las cuatro partículas actuales: varía posición, escala y solo
alpha de vértice. Cada instancia restaura vértices, normales y colores antes de la
siguiente. Comparten las mallas y VBO originales; las copias CPU se preparan una vez,
sin modelos duplicados ni clips GLB inventados.

El cursor modifica solo su `COLOR_DINAMICO`. La tapa `rojo` del corcho reproduce el
color de jugador que ya tenía la esfera superior; su uso está permitido en el manifest.
Ambos materiales se resuelven mediante `meshMaterial` y se restauran tras cada dibujo.
El cuerpo blanco y el metal del corcho, la cruz blanca del cursor y los demás materiales
permanecen originales. Se usa tinte global blanco para conservar colores de vértice.

Se mantienen ondas animadas, línea de pesca, indicador de pique, destellos del pez
raro, jugadores, HUD y popups. No cambia física, hitboxes, IA, reglas, controles,
cámara, puntuación ni tiempos. Cada GLB tiene fallback de sus propias primitivas y
un diagnóstico único si falta o no puede prepararse. El dibujo de escenario y sus
fallbacks evitan las sombras humanas automáticas de `SombrasRetro.h` en Y=0, que no
corresponden a peces/corales sumergidos ni al humo y aves elevados.

## Verificación de la integración

Desde la raíz del repositorio:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarPescaIsla.ps1
```

La prueba compila con C++17, UCRT64 y la misma raylib del proyecto. Ejecuta
`ZonaPruebas` real con contexto OpenGL, cámara y HUD del juego, entradas de teclado
simuladas y 2, 3 y 4 participantes. Comprueba preparación, apuntado, lanzamiento,
espera, pique compartido, tensión, captura de los tres valores, botas, fallo de pique,
exceso de tensión, clasificación, IA, reinicio y salida. Observa transformaciones,
materiales, animaciones y restauración de buffers. Simula el fallo de cada uno de los
18 archivos sin moverlos; comprueba fallback, aviso único, carga única y descarga
simétrica de modelos y memoria CPU.

Las capturas `build/pesca-preparacion.png`, `pesca-lanzamiento.png`, `pesca-pique.png`,
`pesca-tension.png`, `pesca-captura.png` y `pesca-final.png` proceden del juego integrado.
El visor del paquete permanece como referencia de arte. La prueba no verifica
gamepads físicos ni conexión/desconexión de dispositivos.
