# Cajas del Puerto — paquete v1

16 GLB originales modulares para el minijuego de la rama `claude/expansion-party`, referencia `ea5472c`.
**Modelos comprobados en raylib. No están integrados en el juego todavía.**

Contenido: `GLB/` son los archivos para importar; `Vistas/` incluye 19 capturas reales; `Vista_previa.png` muestra la estética del conjunto; `generar_modelos.py` y `geometria_glb.py` generan todo con Python 3 sin paquetes externos; `visor_raylib.cpp` abre el conjunto independientemente del juego; `manifest.json` contiene límites, pivotes y materiales.

Todas las medidas usan 1 unidad GLB = 1 unidad de raylib. Eje Y hacia arriba, frontal de contenedores y barco hacia +Z. Las posiciones abajo son valores de **mundo**; las piezas se instancian desde el código del juego.

| Archivo | Triángulos | Uso / posición inicial |
| --- | ---: | --- |
| `muelle.glb` | 10,920 | mundo (0,0,0.75); borde Z=-6.25 a 7.75. |
| `grua_portico.glb` | 1,152 | mundo (0,0,-5), columnas X+-10.3 y altura de traviesa 5.35. |
| `grua_carro.glb` | 452 | mundo (ganchoX,5.7,-5); piso bajo jugador del equipo solo. |
| `grua_cabina.glb` | 744 | mundo (ganchoX,5.7,-5); arco abierto para visualizar jugador. |
| `grua_brazo.glb` | 120 | mundo (ganchoX,5.55,-5); punta en Z=-2.5. |
| `grua_cable_unidad.glb` | 96 | instanciar en (ganchoX,5.55,-2.5), escalar SOLO Y hasta 5.55-yGancho. |
| `grua_gancho.glb` | 360 | mundo (ganchoX,yGancho,-2.5); yGancho=4.2+bob. |
| `contenedor_cuerpo.glb` | 540 | mundo (2.1*(indice-3.5),0,-2.5). Interior abierto. |
| `contenedor_puerta_izquierda.glb` | 268 | puerta izquierda: (-.85,1.1,1.08) relativo a contenedor_cuerpo. |
| `contenedor_puerta_derecha.glb` | 268 | puerta derecha: (+.85,1.1,1.08). Apertura logica 0..1. |
| `contenedor_decoracion.glb` | 1,076 | cajas de fondo: (X+/-12.5,0,-5) y escalon Y+2.2. |
| `farol.glb` | 460 | (-10.5|10.5,0,1.0|6.5). Luz en Y=3.3. |
| `bolardo.glb` | 592 | X=-10,-6,-2,2,6,10; Z=7.8. |
| `ancla_dorada.glb` | 568 | refuerzo si contenedor.reforzado; centro del techo Y=2.26. |
| `barco_fondo.glb` | 2,040 | mundo (0,-0.9,-17). 34.5 x 6.2. |
| `marca_golpe.glb` | 400 | solo si contenedor.marcas>0; colocar sobre Y=2.32. |

## Pivotes y animación

- Cada contenedor se instancia una vez con cuerpo en `(2.1*(i-3.5),0,-2.5)`. La chapa es hueca, y ocupa como máximo 1,96 de ancho; los ocho puestos separados 2,1 quedan libres. El techo llega a Y=2,30. Los GLB no añaden colisión: conservar las reglas de entrada actuales.
- La puerta izquierda gira o se escala desde la bisagra local `(-0.85,1.1,1.08)`; la derecha desde `(+0.85,1.1,1.08)`. Para repetir exactamente el dibujo actual, aplicar escala X `1-0.88*apertura`, con `apertura` en 0..1. Dibujar las puertas antes o después del cuerpo según el orden que mejor preserve visibilidad.
- El pórtico es fijo. Carro y cabina comparten `(ganchoX,5.7,-5)`, brazo `(ganchoX,5.55,-5)`. El cable unitario sale de `(ganchoX,5.55,-2.5)` y se escala Y a `5.55-yGancho`; el gancho se ancla en `(ganchoX,yGancho,-2.5)`.
- El jugador solitario permanece en `(ganchoX,6.5,-5)`, dentro de la cabina abierta. El gancho sigue `yGancho=4.2+sin(tiempoAnimacion*3)*0.06`. Las marcas negras y el ancla dorada son piezas independientes para los estados `marcas` y `reforzado`.
- Seis bolardos se sitúan en X=-10,-6,-2,2,6,10; Z=7.8. Los cuatro faroles en X=±10.5 y Z=1 o 6.5. El barco se sitúa en `(0,-0.9,-17)`.

`COLOR_DINAMICO` es un material blanco que se puede teñir con los ocho colores de `ColorContenedorCajas`. En `contenedor_cuerpo`, la primitiva es 1; en ambas puertas, 0; y en `contenedor_decoracion`, 1. Si se usa `DrawModel(..., color)`, raylib tiñe todos los materiales incluidos los refuerzos; `visor_raylib.cpp` muestra cómo cambiar solo ese material con `meshMaterial` y restaurarlo después de cada dibujo.

Son **mallas estáticas modulares**, sin rig ni clips GLB; `ganchoX`, `apertura` y la altura del gancho ya viven en el estado del minijuego. No duplicar las mallas en memoria por contenedor. Cargar una sola vez y descargar simétricamente, sin alterar la lógica de juego.

## Visor para Windows / MSYS2 UCRT64

Descomprimir el ZIP y abrir PowerShell **en la carpeta `Cajas_del_Puerto`**, junto a `GLB/`:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorCajasPuerto.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorCajasPuerto.exe
```

Teclas 1: puerto completo; 2: grúa; 3: contenedores; 4: barco; flechas izquierda/derecha: rotar cámara; Escape: salir. El programa descarga los 16 modelos al cerrar. Usa las funciones `raylib.h`, `raymath.h`, `rlgl.h` instaladas junto a tu proyecto.

## Integración posterior

Copiar `GLB/` a `Assets/Modelos/Escenarios/CajasPuerto/`. Sustituir solo las primitivas usadas para dibujar escenario, contenedores y grúa, manteniendo sus coordenadas, fases y lógica de 1 contra 3. Las vistas son del visor autónomo; no representan una integración aplicada al código del minijuego. El binario Windows no se ejecutó aquí.

Los GLB no contienen recursos de Nintendo, texturas externas ni dependencias adicionales. La luz cálida está representada por geometría y color, sin bloom ni shader extra.
