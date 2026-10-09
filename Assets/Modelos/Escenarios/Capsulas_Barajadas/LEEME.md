# Cápsulas Barajadas · modelos GLB v1

Dieciséis GLB 2.0 originales para `Minigames/MinijuegoCapsulasBarajadas.cpp` en `claude/expansion-party`, referencia artística `ea5472c`. **Los 16 están integrados en el minijuego real**, mediante la base compartida de escenarios.

`GLB/` contiene los modelos. `manifest.json` registra límites, pivotes, materiales y uso. `Vistas/` reúne 20 capturas raylib; `Vista_previa.png` resume la estética. `generar_modelos.py` y `geometria_glb.py` permiten regenerarlos con Python 3; `visor_raylib.cpp` carga y descarga todas las mallas.

## Escala y animación

Una unidad GLB equivale a una unidad raylib, con Y hacia arriba y frente de monitores hacia +Z.

- `mesa_acero` va en `(0, 0, 0)`, superficie a Y=1,01, ancho 11,4 y fondo 3,6. `pasarela` va en `(0, 0, 4.8)` y llega a Y=0,3.
- Cada cápsula usa el mismo cuerpo hueco, banda de color y tapa independiente. `PosicionCapsulaCapsulas` devuelve el pivote del cuerpo y de la banda. La tapa se coloca en `(p.x, p.y+1.5+TapaAbiertaCapsulas(m)*1.3, p.z)`; aplicar además la elevación de revelado actual a las tres piezas. Cinco posiciones usan separación 2,1; tres usan 2,8.
- `COLOR_DINAMICO` identifica el material tintable en banda, tapa, tubo, baliza y marcador. Tintar ese material según jugador/cápsula o subfase, preservando los materiales metálicos.
- `brazo_base` va en `(0, 0, -3.4)`; su hombro está en `(0, 4.2, -3.4)`. `brazo_segmento` mide una unidad apuntando a +Y desde su origen: orientar y escalar solo Y entre hombro y codo, y entre codo y mano. Colocar `brazo_articulacion` en el codo y `brazo_pinza` en `PosicionManoCapsulas`. El núcleo usa el mismo cálculo de posición y pulso del minijuego.
- La pared posterior, el suelo y los monitores son decoración. Mostrador en `(0, 0, -6.4)`; ocho tubos en X=−8,4+2,4·k, Y=1, Z=−6,4. Balizas en `(±10.5, 5.6, -7.8)`. Monitores en X=−7,5+5·k, Y=4,6, Z=−8,1.

Los GLB son mallas estáticas modulares; no incluyen rig ni animaciones. Barajado, subfases, selección y puntaje siguen en C++. El paquete se solicita al activar este minijuego, sin cargarlo al inicializar todo el catálogo. Reinicio, nuevas rondas, selector y modo tablero reutilizan las mallas. `ZonaPruebas::Descargar` libera el almacén compartido antes de `CloseWindow`.

## Integración en el juego

Las rutas se centralizan en `Core/RecursosJuego.h` y usan esta carpeta real: `Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/`. No se copian ni regeneran los GLB. Todos conservan escala 1, salvo la longitud Y de los segmentos y el pulso uniforme del núcleo.

| GLB | Primitivas reemplazadas y estado actual |
| --- | --- |
| `sala_laboratorio` | Suelo, juntas y pared posterior, origen común (0,0,0). |
| `monitor` | Marco y pantalla de los cuatro monitores. La malla 2 contiene barras estáticas: se omite para mantener las seis barras animadas originales por monitor. |
| `mostrador` | Mostrador del fondo en (0,0,-6,4). |
| `tubo_ensayo` | Recipiente, líquido y contorno de ocho tubos. Malla 1 es `COLOR_DINAMICO`; la malla 4 de burbujas fijas se omite para conservar las dos burbujas que suben por tubo. |
| `baliza` | Base y lámpara de las dos balizas; `COLOR_DINAMICO` sigue parpadeo de barajado y verde de elección. |
| `mesa_acero` | Encimera, bordes y cuatro patas de la mesa; origen (0,0,0). |
| `pasarela` | Plataforma y franjas del público, en (0,0,4,8). |
| `brazo_base` | Base y hombro fijos del robot, en (0,0,-3,4). |
| `brazo_segmento` | Ambos cilindros entre hombro/codo/mano, una sola malla compartida. Se orienta +Y hacia cada unión y se escala solo Y por su longitud. |
| `brazo_articulacion` | Esfera del codo, en el cálculo existente. |
| `brazo_pinza` | Dos dedos, sigue `PosicionManoCapsulas`. |
| `capsula_cuerpo` | Cilindro y contorno de cada cápsula; pivote de base, cuerpo hueco. |
| `capsula_banda` | Banda de cada cápsula; mismo pivote/elevación del cuerpo, color por ID lógico. |
| `capsula_tapa` | Tapa e indicador superior; sigue apertura y elevación actuales, color por cápsula. |
| `nucleo` | Esfera y aros; sigue la mano antes de entregar, después queda bajo la cápsula premiada, con el pulso existente. |
| `marcador` | Cono y esfera de cada marcador; sigue slot seleccionado, separación entre jugadores y flotación existentes. Color de jugador al elegir y acierto/error al revelar. |

Para cuatro cápsulas la separación actual es 2,5; para tres es 2,8 y para cinco 2,1. Cuerpo/banda/tapa siguen los arcos del barajado y comparten la elevación de revelado. El núcleo permanece sobre la mesa al elevarse las cápsulas. Los marcadores conservan las coordenadas reales del juego (Z=1,5, Y=3,5 más flotación), distintas de la colocación de muestra del visor.

El material dinámico se resuelve mediante `meshMaterial`: tubo 1, baliza 2, banda 0, tapa 3 y marcador 0 son índices de malla, no de material. Se restaura después de cada dibujo y el resto conserva materiales y colores de vértice. Los colores de cada participante también permanecen en HUD, personajes y líneas de selección; los aros de confirmación y el destello de entrega se conservan.

Las vistas que omiten barras/burbujas comparten las mallas y VBO originales, sin nuevas cargas, copias de geometría ni descargas parciales. No se usan matrices locales de rlgl: instancia y pivote se aplican una sola vez. Los GLB omiten la sombra automática de tamaño humano; las cápsulas tienen una única sombra explícita sobre la mesa, también con fallback. Las barras y los marcadores flotantes no proyectan sombras incorrectas en el suelo.

Cada pieza mantiene fallback independiente y un diagnóstico de carga fallida por recurso. No quedan modelos sin usar. Solo se omiten las dos mallas estáticas indicadas para preservar sus animaciones procedurales.

Desde la raíz del repositorio, la prueba integrada se ejecuta con:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarCapsulasBarajadas.ps1
```

Compila el código real con UCRT64 y ejecuta `ZonaPruebas` con OpenGL para 2/3/4 participantes y cinco rondas. Comprueba subfases, tres/cuatro/cinco cápsulas, brazo, tapas, núcleo, barajado, materiales, selección, apuesta doble, errores, timeout, IA, desempate, reinicio, regreso al menú, modo tablero, fallos por pieza y descarga. Exporta capturas del juego en `build/capsulas-*.png`. No sustituye una sesión manual con mandos y audio.

## Visor

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de esta carpeta:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorCapsulasBarajadas.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorCapsulasBarajadas.exe
```

Teclas: 1 cinco cápsulas; 2 cámara original; 3 tres cápsulas; 4 movimiento; flechas izquierda/derecha giran la cámara; Escape cierra. `--capturar` regenera las vistas.

El visor permanece como herramienta de inspección del arte. Sus capturas originales no se presentan como pruebas de la integración del minijuego.
