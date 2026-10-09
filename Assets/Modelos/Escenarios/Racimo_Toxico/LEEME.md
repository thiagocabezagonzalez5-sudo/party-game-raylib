# Racimo Tóxico · modelos GLB v1

Catorce GLB 2.0 originales integrados en `Minigames/MinijuegoRacimoToxico.cpp` de `claude/expansion-party`. La referencia de creación del paquete fue `ea5472c`; el dibujo del juego usa ahora estos archivos mediante la base compartida de escenarios.

`GLB/` contiene las mallas; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 18 capturas de raylib; `Vista_previa.png` muestra la escena y ocho piezas. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga todos los modelos.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; el jugador mira hacia Z negativo.

- `agua_pantano` y `enredadera_central` se colocan en `(0,0,0)`; `arbol_podrido`, en `(-4.8,0,-1)`. La enredadera tiene ramitas a la altura de los 22 frutos.
- Los frutos tienen pivote en el centro. Colocar `fruto_normal`, `fruto_toxico` (índices 5, 11 y 17) o `fruto_dorado` en `PosicionFrutaRacimo(i,t)` según el estado del juego. Conservar la selección, el veneno y el premio en C++.
- Colocar una `balsa` en `(posicionBalsaX, 0.08+hundimiento+bamboleo, 3.5)`. La balsa mide 3.25 de ancho, frente a una separación de 5 entre centros. `aro_turno` se puede mostrar en `(posicionBalsaX,.02,3.5)` mientras juega esa balsa.
- `tronco_flotante` tiene pivote en un extremo y eje local +Z de longitud 1. Orientar ese eje entre los extremos originales y escalarlo a su longitud. Conservar los movimientos, colisiones y hundimiento existentes.
- Repetir `cabana_pilotes` en `(-10.5,0,-8)` y `(9.5,0,-9)`, y `arbol_fondo` alrededor de Z=−15. Repartir `nenufar` y `roca_pantano` fuera de las balsas; instanciar y animar `luciernaga` con las posiciones actuales.

Son mallas estáticas modulares sin rig ni animaciones GLB. Se usan directamente desde `Assets/Modelos/Escenarios/Racimo_Toxico/GLB/`, sin copiar, mover ni regenerar recursos. Las rutas se centralizan en `Core/RecursosJuego.h`.

## Integración en el juego

| GLB | Elemento y estado real |
| --- | --- |
| `agua_pantano` | Reemplaza los dos planos de agua, con origen compartido `(0,0,0)`. |
| `arbol_podrido` | Reemplaza tronco y dos ramas; conserva las seis lianas procedurales que oscilan. |
| `enredadera_central` | Reemplaza tallo, ramitas y hojas del racimo. Sus ramitas quedan como decoración estática al consumir frutos. |
| `fruto_normal` | Reemplaza la esfera normal; sigue índice, frente, vaivén y vuelo real. |
| `fruto_toxico` | Reemplaza la esfera tóxica y conserva su halo pulsante, daño y eliminación. |
| `fruto_dorado` | Reemplaza la esfera dorada y conserva su halo y salto de turno. |
| `balsa` | Reemplaza las cuatro tablas; conserva posición, temblor, bamboleo y hundimiento. |
| `tronco_flotante` | Reemplaza los dos cilindros de cada tronco; pivote en el extremo inicial, eje +Z orientado al extremo final, escala únicamente longitudinal. |
| `cabana_pilotes` | Reemplaza pilotes, paredes, ventana y techo en las dos posiciones originales. |
| `arbol_fondo` | Reemplaza tronco y copa en las siete posiciones originales. |
| `luciernaga` | Reemplaza la esfera central de cada insecto; conserva vuelo y halo. El brillo pulsa solo en `BOMBILLAS`. |
| `nenufar` | Añade decoración fuera de las balsas en las cuatro posiciones documentadas por el visor; no agrega colisiones. |
| `roca_pantano` | Añade decoración fuera de las balsas en las cuatro posiciones documentadas por el visor; no agrega colisiones. |
| `aro_turno` | Reemplaza el disco del turno activo; el pulso modifica solo `BOMBILLAS`. |

Los 14 modelos se usan. Se mantienen indicadores de selección, señal sobre el jugador activo, halos y niebla. Los GLB conservan pivotes, materiales y colores de vértice, sin normalización a la base ni transformaciones acumuladas de rlgl. El paquete no tiene `COLOR_DINAMICO`: las dos piezas con brillo variable resuelven su material `BOMBILLAS` por `meshMaterial` y restauran el color después de cada instancia. La ventana de la cabaña permanece constante. La madera de la balsa conserva su material original al eliminar al jugador; el estado se expresa mediante el hundimiento bajo el agua.

El paquete se carga al activar una ronda válida, una sola vez por archivo y compartido por todas las instancias. Reinicio, reposición y reentrada reutilizan la caché. Cada pieza fallida conserva sus primitivas de respaldo y registra el problema una sola vez. `DescargarModelosEscenariosRetro3D()` libera el paquete en los caminos existentes de ZonaPruebas/CoreJuego antes de `CloseWindow`; salir al selector mantiene la caché disponible. No se crean recursos propios del struct del minijuego, por lo que su encabezado no necesita nuevos métodos.

El helper dibuja los GLB sin la sombra automática destinada a personajes. Los efectos transparentes y los respaldos de balsas/aros tampoco proyectan esas manchas sobre el agua. El dibujo y las sombras existentes de jugadores se conservan. Física, hitboxes, IA, cámara, reglas, controles y tiempos permanecen intactos.

## Verificación integrada

Desde la raíz del repositorio, con MSYS2 UCRT64 instalado:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarRacimoToxico.ps1
```

La prueba compila el código real del juego, crea un contexto OpenGL y usa `ZonaPruebas` con 2, 3 y 4 participantes. Simula selección y confirmación, frutos seguros, veneno, dorado, vuelos, timeout, reposición, IA, mando ausente, eliminación, final, reinicio, salida y reentrada de tablero. Comprueba recursos compartidos, materiales restaurados, transformaciones, estado e hitboxes intactos durante el dibujo, fallback individual de los 14 archivos y descarga simétrica. Las capturas `build/racimo-*.png` corresponden al juego integrado, no al visor. La prueba no sustituye la revisión manual de audio y mandos físicos.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorRacimoToxico.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorRacimoToxico.exe
```

Teclas 1: cuatro balsas; 2: cámara del juego; 3: dos balsas; 4: pantano; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los personajes de prueba y los efectos del visor son referencias temporales, no GLB de este paquete.
