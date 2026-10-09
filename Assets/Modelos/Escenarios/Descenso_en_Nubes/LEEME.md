# Descenso en Nubes · modelos GLB v1

Diecinueve GLB 2.0 originales para `Minigames/MinijuegoDescensoNubes.cpp` de `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib con el visor incluido. **Integrados en el minijuego real mediante el almacén compartido de escenarios.**

`GLB/` contiene las piezas; `manifest.json` registra dimensiones, materiales, pivotes y ubicación. `Vistas/` contiene 23 capturas; `Vista_previa.png` muestra descenso y aterrizaje. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga cada modelo.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; la caída comienza en Y=100 y termina en Y=0. La cámara acompaña `alturaCamara` desde `(0,alturaCamara+13,7.5)` hacia `(0,alturaCamara-3,.8)`.

- `planeador` se coloca en `jugador.posicion`, con el ala a Y=+1,3 y ancho 2,2. Usar `planeador_frenado` mientras `estado.frenando>0` (ancho 3,4). Su material `COLOR_DINAMICO` se puede teñir con el color del jugador; la integración resuelve la primitive 0 mediante `meshMaterial`, modifica exclusivamente `COLOR_DINAMICO` y restaura el material después de dibujar. Conservar el cuerpo del personaje por separado.
- `anillo_blanco` y `anillo_dorado` tienen pivote central y radios 0,85 y 1,0. Colocar en `anillo.posicion` según `valor`. `estrella` reemplaza las tres esferas que orbitan al jugador aturdido, conservando su radio visual 0,1 y recorrido. Mantener la recogida, la puntuación y el estado `recogidoPor` en C++.
- `nube_tormenta` va en `tormenta.posicion`, con tamaño visual cercano al radio base 1,6. Mantener la colisión, el aturdimiento y el descuento de puntos. `banda_viento` tiene origen central, radio 6 y Y=±1,6 respecto de `viento.altura`. Repetir `flecha_viento` en esa altura y orientarla desde +X hacia `dirX,dirZ`; las fuerzas siguen en C++.
- Mostrar `mar_de_nubes`, `isla_principal` y `diana_aterrizaje` juntos en `(0,0,0)` cuando `alturaCamara<=30`. La isla tiene radio 7 y diana de radio 1,8, con superficie de aterrizaje en Y=0.
- `isla_flotante` se repite fuera del cilindro a radio aproximado 10,5..13,5 y alturas `12+13*k`; `globo_azul` y `globo_rojo` se alternan a X=±9,5 y alturas `20+17*k`. `nube_blanca` forma el fondo. `molino_torre` va en `(-5.2,0,-3)` y sus `aspas_molino` en `(-5.2,4.15,-2.12)` con giro sobre Z; hay otros molinos reducidos en las nubes. `ave` se repite cerca de radio 8. `arcoiris` va en `(0,62,-16)`.

Son mallas estáticas modulares sin rig ni animaciones GLB. El juego usa los archivos existentes en `Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/`, sin copiarlos, normalizar sus pivotes ni regenerarlos. Las rutas están centralizadas en `Core/RecursosJuego.h`.

## Integración en el juego

Se usan los 19 GLB; no quedan modelos pendientes. Cada recurso se solicita al iniciar una ronda válida de Descenso en Nubes y se comparte entre participantes e instancias. `R` y la reentrada no recargan las mallas. La descarga global, utilizada por `ZonaPruebas::Descargar` y `Juego::Descargar`, libera los modelos y las copias CPU de las alas antes de `CloseWindow`.

| Modelos | Primitivas reemplazadas y estado |
|---|---|
| `planeador`, `planeador_frenado` | Dos cubos y dos soportes; variante según `frenando`, posición y color del jugador. |
| `anillo_blanco`, `anillo_dorado` | Ocho círculos; posición, valor y máscara `recogidoPor`. La primitive BOMBILLAS dorada reemplaza la esfera central y pulsa con `sin(t*6)` sin teñir materiales. |
| `estrella` | Esferas de aturdimiento; tres instancias en las mismas órbitas, escala `0.1/0.29`. |
| `nube_blanca` | Tres esferas por nube, mismas posiciones, escala y filtros por altura. |
| `nube_tormenta` | Tres esferas y rayos; escala `radio/1.6`, primitive de rayos visible durante el parpadeo actual. Se omite su aro rojo para conservar el círculo de riesgo con radio `radio+0.35`. |
| `flecha_viento`, `banda_viento` | Dos cilindros por flecha y cilindro de alambre; mismas alturas y avance. +X local gira sobre Y con `-atan2(dirZ,dirX)`. La banda conserva RGB y usa alpha 0,22, restaurado por material. |
| `mar_de_nubes`, `isla_principal`, `diana_aterrizaje` | Plano, dos cilindros de isla y tres de diana; origen `(0,0,0)`, visibles cuando `alturaCamara<=30`. |
| `isla_flotante` | Disco, roca, tronco y copa; posiciones actuales fuera del cilindro de caída. |
| `globo_azul`, `globo_rojo` | Envolvente, cesta y cuerdas; variantes alternadas con el vaivén actual. El pivote central de la envolvente se coloca en `c+(0,1.8,0)`. |
| `molino_torre`, `aspas_molino` | Torre, techo y cuatro aspas; unidad en la isla, escala uniforme 0,65 sobre las nubes. Offset del eje `(0,4.15,0.88)` escalado con la torre; giro sobre Z a `t*1.2`. |
| `ave` | Dos líneas por ave; misma órbita y aleteo `sin(t*9+k)*0.35`. La primitive 0 agrupa cuerpo y alas interiores: se conserva el prefijo del cuerpo, se deforman únicamente alas y se actualizan sus normales. Se restauran vértices, normales y VBO después de cada instancia. |
| `arcoiris` | Setenta segmentos; posición `(0,62,-16)` y filtro de altura original. |

Cada pieza conserva su fallback procedural y emite un solo diagnóstico si falta o falla. El GLB exitoso suprime exclusivamente sus primitivas sustituidas. Permanecen personajes, indicadores, líneas de descenso, HUD, sombras de personajes, física, colisiones, IA, tiempos y puntuación. Los escenarios evitan la macro de sombras humanas de `SombrasRetro.h`; no se combinan transformaciones `rlTranslatef`/`rlRotatef` con las instancias `DrawModelEx`.

## Verificación de integración

Compilar el juego con la tarea UCRT64 existente. La prueba automatizada compila el código real del proyecto y ejecuta `ZonaPruebas`, `GestorMinijuegos`, entrada, actualización y dibujo con contexto OpenGL:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarDescensoNubes.ps1
```

Cubre 2, 3 y 4 participantes, preparación, acciones de frenado y recarga, movimiento, anillos y máscaras, tormentas y aturdimiento, viento en cuatro direcciones, filtros por altura, aterrizaje central y exterior, IA hasta el final, límite de tiempo, reinicio, salida y modo tablero. Observa transformaciones y materiales durante el dibujo, geometría de alas, carga única, fallback individual de los 19 GLB y descarga simétrica. Las capturas `build/nubes-*.png` son del minijuego integrado; entradas y casos específicos se simulan, sin requerir gamepads físicos.

## Visor independiente (referencia del paquete)

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorDescensoNubes.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorDescensoNubes.exe
```

Teclas 1: descenso; 2: cámara del juego; 3: aterrizaje; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los cuerpos cúbicos del visor solo sirven de referencia de escala y no forman parte de los GLB.
