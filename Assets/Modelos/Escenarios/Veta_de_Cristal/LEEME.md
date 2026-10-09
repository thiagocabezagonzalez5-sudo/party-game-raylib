# Veta de Cristal · modelos GLB v1

Quince modelos GLB 2.0 originales para `Minigames/MinijuegoVetaCristal.cpp` en la rama `claude/expansion-party`, referencia `ea5472c`. Comprobados cargándolos en raylib con el visor incluido. **No integrados todavía en el juego.**

`GLB/` contiene las mallas, `manifest.json` registra límites, pivotes, materiales y posiciones, `Vistas/` incluye 19 capturas reales, `Vista_previa.png` muestra el conjunto. `generar_modelos.py` y `geometria_glb.py` permiten regenerarlos con Python 3. `visor_raylib.cpp` carga y descarga las 15 mallas.

## Colocación

- Una unidad GLB = una unidad raylib, eje Y hacia arriba. Suelo de la mina 24×16, con plano jugable Y=0; límites lógicos de jugadores X=±10,6 y Z=±7,2.
- `paredes_tunel`, `portico_madera` y `riel_central` se colocan en (0,0,0); el riel mide 16 y avanza por Z. Los postes van en X=±11,7, Z=−5/0/5. Las lámparas en X=±11, Y=3,7 y el mismo Z.
- Cinco geodas por equipo: cuatro pequeñas en X=±3,8/±7,6 y Z=±3,6; una grande en X=±6 y Z=0. Sus centros Y=2,85 o 3,10, respectivamente. Geoda descargada: escala 0,9/0,65 para la grande. Son visuales y quedan dentro de sus bloques lógicos.
- La vagoneta se coloca en (0,0,`vagonetaZ`) y sigue la lógica actual; las ruedas permanecen dentro de X=±0,95 y la carga dentro del cuerpo. El movimiento y aturdimiento siguen en C++.
- Las gemas azules, doradas y violetas tienen pivote en el centro; usar `GemaVeta.x/y/z` y la oscilación existente. La marca se sitúa en (geoda.x,0,04,geoda.z) y `COLOR_DINAMICO` permite tintarla. Conservar la recogida, la caída y la puntuación actuales.

Estas piezas son mallas estáticas modulares, sin clips ni shaders. La lámpara y los cristales brillan por color de material, sin crear una luz real. Cargar cada GLB una sola vez y descargar simétricamente al salir del minijuego.

## Visor independiente

Desde PowerShell, dentro de esta carpeta en Windows con MSYS2 UCRT64 y raylib:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorVetaCristal.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorVetaCristal.exe
```

Teclas 1: mina completa; 2: cámara del juego; 3: túnel; flechas izquierda/derecha: girar la cámara; Escape: cerrar. `--capturar` genera las vistas.

Para una integración posterior, copiar `GLB/` a `Assets/Modelos/Escenarios/VetaCristal/` y reemplazar solo el dibujo de primitivas en las funciones visuales. Esta entrega no modifica el repositorio del juego.
