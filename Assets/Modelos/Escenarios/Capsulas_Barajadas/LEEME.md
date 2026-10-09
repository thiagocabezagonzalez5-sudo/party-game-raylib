# Cápsulas Barajadas · modelos GLB v1

Dieciséis GLB 2.0 originales para `Minigames/MinijuegoCapsulasBarajadas.cpp` en `claude/expansion-party`, referencia `ea5472c`. Comprobados cargándolos en raylib con el visor incluido. **Aún no integrados en el juego.**

`GLB/` contiene los modelos. `manifest.json` registra límites, pivotes, materiales y uso. `Vistas/` reúne 20 capturas raylib; `Vista_previa.png` resume la estética. `generar_modelos.py` y `geometria_glb.py` permiten regenerarlos con Python 3; `visor_raylib.cpp` carga y descarga todas las mallas.

## Escala y animación

Una unidad GLB equivale a una unidad raylib, con Y hacia arriba y frente de monitores hacia +Z.

- `mesa_acero` va en `(0, 0, 0)`, superficie a Y=1,01, ancho 11,4 y fondo 3,6. `pasarela` va en `(0, 0, 4.8)` y llega a Y=0,3.
- Cada cápsula usa el mismo cuerpo hueco, banda de color y tapa independiente. `PosicionCapsulaCapsulas` devuelve el pivote del cuerpo y de la banda. La tapa se coloca en `(p.x, p.y+1.5+TapaAbiertaCapsulas(m)*1.3, p.z)`; aplicar además la elevación de revelado actual a las tres piezas. Cinco posiciones usan separación 2,1; tres usan 2,8.
- `COLOR_DINAMICO` identifica el material tintable en banda, tapa, tubo, baliza y marcador. Tintar ese material según jugador/cápsula o subfase, preservando los materiales metálicos.
- `brazo_base` va en `(0, 0, -3.4)`; su hombro está en `(0, 4.2, -3.4)`. `brazo_segmento` mide una unidad apuntando a +Y desde su origen: orientar y escalar solo Y entre hombro y codo, y entre codo y mano. Colocar `brazo_articulacion` en el codo y `brazo_pinza` en `PosicionManoCapsulas`. El núcleo usa el mismo cálculo de posición y pulso del minijuego.
- La pared posterior, el suelo y los monitores son decoración. Mostrador en `(0, 0, -6.4)`; ocho tubos en X=−8,4+2,4·k, Y=1, Z=−6,4. Balizas en `(±10.5, 5.6, -7.8)`. Monitores en X=−7,5+5·k, Y=4,6, Z=−8,1.

Los GLB son mallas estáticas modulares; no incluyen rig ni animaciones. Mantener barajado, subfases, selección y puntaje del minijuego. Cargar cada malla una sola vez y descargarla simétricamente al salir.

## Visor

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de esta carpeta:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorCapsulasBarajadas.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorCapsulasBarajadas.exe
```

Teclas: 1 cinco cápsulas; 2 cámara original; 3 tres cápsulas; 4 movimiento; flechas izquierda/derecha giran la cámara; Escape cierra. `--capturar` regenera las vistas.

Para integrar más adelante, copiar `GLB/` a `Assets/Modelos/Escenarios/CapsulasBarajadas/` y reemplazar las primitivas en las funciones de dibujo. Esta entrega no modifica los archivos del repositorio.
