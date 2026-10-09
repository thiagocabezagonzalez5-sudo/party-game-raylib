# Veta de Cristal · modelos GLB v1

Quince modelos GLB 2.0 originales para `Minigames/MinijuegoVetaCristal.cpp` en la rama `claude/expansion-party`, referencia artística `ea5472c`. **Los 15 están integrados en el minijuego real**, mediante el almacén compartido de escenarios.

`GLB/` contiene las mallas, `manifest.json` registra límites, pivotes, materiales y posiciones, `Vistas/` incluye 19 capturas reales, `Vista_previa.png` muestra el conjunto. `generar_modelos.py` y `geometria_glb.py` permiten regenerarlos con Python 3. `visor_raylib.cpp` carga y descarga las 15 mallas.

## Colocación

- Una unidad GLB = una unidad raylib, eje Y hacia arriba. Suelo de la mina 24×16, con plano jugable Y=0; límites lógicos de jugadores X=±10,6 y Z=±7,2.
- `paredes_tunel`, `portico_madera` y `riel_central` se colocan en (0,0,0); el riel mide 16 y avanza por Z. Los postes van en X=±11,7, Z=−5/0/5. Las lámparas en X=±11, Y=3,7 y el mismo Z.
- Cinco geodas por equipo: cuatro pequeñas en X=±3,8/±7,6 y Z=±3,6; una grande en X=±6 y Z=0. Sus centros Y=2,85 o 3,10, respectivamente. Geoda descargada: escala 0,9/0,65 para la grande. Son visuales y quedan dentro de sus bloques lógicos.
- La vagoneta se coloca en (0,0,`vagonetaZ`) y sigue la lógica actual; las ruedas permanecen dentro de X=±0,95 y la carga dentro del cuerpo. El movimiento y aturdimiento siguen en C++.
- Las gemas azules, doradas y violetas tienen pivote en el centro; usar `GemaVeta.x/y/z` y la oscilación existente. La marca se sitúa en (geoda.x,0,04,geoda.z) y `COLOR_DINAMICO` permite tintarla. Conservar la recogida, la caída y la puntuación actuales.

Estas piezas son mallas estáticas modulares, sin clips ni shaders. La lámpara y los cristales brillan por color de material, sin crear una luz real. El juego carga el paquete al activar Veta (no al inicializar todos los minijuegos), conserva las mallas al reiniciar/regresar al selector y las descarga desde `ZonaPruebas::Descargar`, antes de `CloseWindow`.

## Integración en el juego

Las rutas se centralizan en `Core/RecursosJuego.h` y apuntan a esta carpeta real, `Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/`. No se copian ni regeneran los archivos.

| GLB | Primitivas reemplazadas y estado |
| --- | --- |
| `suelo_mina` | Suelo, colores de las dos mitades y manchas de roca. Origen (0,0,0), escala 1. |
| `paredes_tunel` | Paredes del fondo, laterales y borde frontal. Origen (0,0,0), escala 1. |
| `veta_pared` | 26 grupos minerales del fondo y 8 laterales, en las posiciones procedurales actuales. Laterales girados ±90° hacia la mina; se conserva el pulso como halo. |
| `portico_madera` | Cuatro postes y traviesa del fondo, desde su origen común (0,0,0). |
| `poste_lateral` | Seis postes con su traviesa, desde el pivote del pie. |
| `lampara_colgante` | Seis luces sólidas; conserva los halos transparentes existentes. |
| `riel_central` | Lecho, quince travesaños y ambos rieles. Origen (0,0,0), dirección Z. |
| `vagoneta` | Cuerpo, refuerzos, ruedas, carga y faros. Visible según `vagonetaActiva`, sigue `vagonetaZ` y orientación según `direccionVagoneta`. |
| `geoda_pequena` | Cubo, aristas y cristales de las ocho geodas pequeñas cargadas. Conserva centro Y=2,85 y sacudida. |
| `geoda_grande` | Lo mismo para las dos grandes cargadas, centro Y=3,10. |
| `geoda_agotada` | Cubo/aristas de cualquiera de las geodas descargadas. Escala 1 pequeña o 0,9/0,65 grande; misma sacudida. |
| `gema_azul` | Coleccionable de valor 1, radio visual 0,20. |
| `gema_dorada` | Coleccionable de valor 3, radio visual 0,28. |
| `gema_violeta` | Coleccionables de valor 4/6, radio visual 0,38. Las tres gemas siguen posición, flotación y parpadeo existentes. |
| `marca_geoda` | Aro de salto de cada geoda cargada y segundo aro de la grande. Escala X/Z conserva el pulso/radio de cada aro. Únicamente `COLOR_DINAMICO` recibe su color y transparencia. |

Cada pieza mantiene fallback independiente con sus primitivas originales. El diagnóstico de carga fallida se emite una vez. Se resuelve el material dinámico mediante `meshMaterial[0]` y se restaura después de cada instancia; los demás materiales y colores de vértice se conservan.

No se combinan matrices locales de rlgl con transformaciones de instancia: todo se dibuja en coordenadas del mundo. El helper omite la sombra automática de tamaño humano para los GLB; geodas y vagoneta usan sombras explícitas acordes a sus dimensiones. Los halos no generan otra sombra y las gemas conservan su sombra explícita, elevada sobre los detalles del suelo. Permanecen el aviso del carril, los indicadores de equipo, partículas, estrellas de aturdimiento, debug y HUD. No quedan modelos sin usar.

La prueba automatizada usa el minijuego y `ZonaPruebas` reales, sin ejecutar el visor:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarVetaCristal.ps1
```

Ejecutarla desde la raíz del repositorio. Compila con UCRT64 y verifica 2/3/4 participantes, estados visuales, golpes, coordinación, recogida, puntuación, vagoneta, aturdimiento, recarga, IA, final, reinicio, salida, entrada desde tablero, carga única, fallos por pieza y descarga. Exporta capturas del juego integrado en `build/veta-*.png`. La prueba automatizada no sustituye una sesión manual con mandos y audio.

## Visor independiente

Desde PowerShell, dentro de esta carpeta en Windows con MSYS2 UCRT64 y raylib:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorVetaCristal.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorVetaCristal.exe
```

Teclas 1: mina completa; 2: cámara del juego; 3: túnel; flechas izquierda/derecha: girar la cámara; Escape: cerrar. `--capturar` genera las vistas.

El visor queda como herramienta independiente para inspeccionar el arte; sus capturas originales no son evidencia de la integración del minijuego.
