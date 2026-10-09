# Tesorero Cercado · modelos GLB v1

Dieciséis GLB 2.0 originales integrados en `Minigames/MinijuegoTesoreroAcorralado.cpp` de `claude/expansion-party`. La referencia de creación fue `ea5472c`. El nombre de archivo del minijuego es **Tesorero Acorralado**; este paquete conserva su nombre original **Tesorero Cercado**.

`GLB/` contiene las piezas; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 20 capturas de raylib; `Vista_previa.png` muestra el conjunto. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga cada modelo.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; Z positivo apunta hacia la cámara de juego.

- `foso_agua`, `suelo_losas` y `torre_homenaje` se colocan en `(0,0,0)`. El suelo mide 18×14 y termina en Y=0. El cuerpo de la torre ocupa 2,6×2,6 y Y=0..2,2, como el obstáculo actual.
- `muro_fondo` va en `(0,0,-7.6)`, `muro_lateral` se repite en `(±9.6,0,0)`, `parapeto_frontal` en `(0,0,7.6)` y `portal_fondo` en `(0,0,-6.95)`. Las torres altas van en `(±10.4,0,-8.2)` y las bajas en `(±10.4,0,8.2)`.
- Los cinco estandartes parten de `x=-7.2+3.6*k`, `y=2.8`, `z=-7.2`, alternando rojo y dorado. Las seis antorchas van en `x=±8.8`, `z=-4.5+4.5*j`, con `j=0..2`. La llama puede recibir partículas o luz dinámica de C++.
- Cada reja se compone de `marco_reja` fijo y `reja_levadiza` móvil, ambos con pivote XZ en su centro. Colocarlos en `(−5,0,0)`, `(5,0,0)`, `(0,0,−4.2)` y `(0,0,4.2)`. Girar **90° en Y** las rejas cuyo `mitadZ>mitadX` (las dos de X=±5). La hoja móvil lleva `Y=(1-reja.altura)*2.6`; el marco queda en Y=0. Mostrar `aviso_reja` en el suelo solo cuando el estado sea `REJA_TESORERO_AVISO`, con la misma rotación.
- `moneda_tesoro` tiene origen en el centro. Situarla en `moneda.y+.12` y conservar el giro, la caída, el tiempo de recogida y el puntaje existentes.

Son mallas estáticas modulares sin rig ni animaciones GLB. El juego usa directamente `Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/`, sin mover ni regenerar recursos. `Core/RecursosJuego.h` centraliza las rutas.

## Integración y reemplazos

| GLB | Primitivas reemplazadas y conexión al juego |
| --- | --- |
| `foso_agua` | Volumen del foso; conserva la espuma que se mueve proceduralmente. |
| `suelo_losas` | Base del patio y losas, en el origen y escala originales. |
| `muro_fondo` | Muro del fondo y sus 17 almenas. |
| `muro_lateral` | Ambos muros laterales y sus 12 almenas por lado. |
| `parapeto_frontal` | Muro bajo del frente, en `(0,0,7.6)`. |
| `torre_esquina_alta` | Cuerpo y techo de las dos torres del fondo. |
| `torre_esquina_baja` | Cuerpo y techo de las dos torres frontales. |
| `torre_homenaje` | Obstáculo visual central, almenas y estandarte; conserva la caja lógica de 2.6×2.2×2.6. |
| `portal_fondo` | Portón, postes y dintel del fondo. |
| `estandarte_rojo` | Asta y tela de los tres estandartes pares; solo la tela y sus adornos siguen el vaivén. |
| `estandarte_dorado` | Asta y tela de los dos impares; asta y travesaño fijos. La malla `oro` agrupa tela y remate, que comparten el pequeño vaivén. |
| `antorcha` | Soporte y llama central de seis antorchas; solo el fuego pulsa alrededor de su centro local. El halo se conserva. |
| `marco_reja` | Postes de las cuatro rejas, con marco y poleas fijos. |
| `reja_levadiza` | Barras y travesaños; altura y rotación proceden de cada `RejaTesorero`. |
| `aviso_reja` | Marca del suelo visible solo durante `REJA_TESORERO_AVISO`, con opacidad pulsante. |
| `moneda_tesoro` | Cilindro de moneda activa; conserva centro, giro, vuelo, sombra circular y parpadeo de expiración. |

Los 16 modelos se usan. Se conservan partículas de impacto, marcadores de equipo, bolsa indicadora del tesorero, HUD y efectos procedurales. El GLB de moneda tiene normal local +Z; el ángulo Y `90-(tiempoAnimacion*6+moneda.x*3)*RAD2DEG` conserva la normal y velocidad del cilindro anterior. Todas las transformaciones se pasan una sola vez a `DrawModelEx`, sin sumar otra matriz de rlgl ni centrar los pivotes.

El paquete no usa `COLOR_DINAMICO` ni `BOMBILLAS`. Durante el aviso, solo `hierro` e `hierro_oscuro` de la hoja móvil reciben el rojo del estado; puntas y herrajes permanecen intactos. Los tres materiales de la marca conservan RGB y reciben únicamente alpha. Se resuelven por `meshMaterial` y se restauran después de cada dibujo. El resto conserva materiales y colores de vértice originales. Las vistas de una malla comparten VBO y materiales con el modelo cargado; no crean recursos ni duplican cargas por instancia.

La carga diferida se solicita desde `Reiniciar` después de validar al menos dos participantes. Reinicios y reentradas desde selector o tablero conservan la caché compartida. Un fallo de carga o una estructura de primitives incompatible deja disponible el fallback de esa pieza y emite un solo diagnóstico por recurso. `ZonaPruebas::Descargar()` libera el paquete mediante `DescargarModelosEscenariosRetro3D()`. `Juego::Descargar()` llama ese camino antes de `CloseWindow`; la descarga repetida es segura. El struct del minijuego no es propietario de modelos y su encabezado no requiere métodos nuevos.

El helper evita las sombras automáticas de tamaño humano de `SombrasRetro.h` para los GLB; espuma, halos, bolsa indicadora y aviso tampoco generan esas manchas. Las sombras existentes de jugadores y monedas se mantienen. No se modifican física, hitboxes, reglas, IA, controles, cámara, tiempos ni puntuación.

## Prueba del juego integrado

Desde la raíz, con el compilador UCRT64 existente:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarTesoreroAcorralado.ps1
```

Compila el código real con C++17 y ejecuta `ZonaPruebas` en OpenGL con 2, 3 y 4 participantes. Verifica preparación, aviso, descenso y reapertura, protección de jugadores bajo una reja, golpe horizontal, golpe al suelo, caída/recogida/recuperación/expiración de monedas, IA, mando ausente, victoria de ambos equipos, reinicio, salida y reentrada desde tablero. También comprueba transformaciones, colores/restauración, caché compartida, fallback de los 16 archivos y descarga antes del cierre del contexto. Las capturas `build/tesorero-*.png` pertenecen al minijuego integrado. Queda aparte la revisión manual de audio y mandos físicos.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorTesoreroCercado.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorTesoreroCercado.exe
```

Teclas 1: patio; 2: cámara del juego; 3: rejas cerradas; 4: castillo; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los cuatro cubos de colores del visor son referencias temporales de escala para jugadores, no GLB del paquete.
