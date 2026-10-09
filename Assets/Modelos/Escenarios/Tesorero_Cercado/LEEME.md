# Tesorero Cercado · modelos GLB v1

Dieciséis GLB 2.0 originales para `Minigames/MinijuegoTesoreroAcorralado.cpp` de `claude/expansion-party`, referencia `ea5472c`. El nombre de archivo del minijuego es **Tesorero Acorralado**; este paquete usa el nombre de la lista de modelos. Cargados y comprobados en raylib con el visor incluido. **Aún no integrados en el juego.**

`GLB/` contiene las piezas; `manifest.json` registra límites, triángulos, materiales, pivotes y colocación. `Vistas/` contiene 20 capturas de raylib; `Vista_previa.png` muestra el conjunto. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y descarga cada modelo.

## Escala y uso

Una unidad GLB equivale a una unidad raylib. Y apunta arriba; Z positivo apunta hacia la cámara de juego.

- `foso_agua`, `suelo_losas` y `torre_homenaje` se colocan en `(0,0,0)`. El suelo mide 18×14 y termina en Y=0. El cuerpo de la torre ocupa 2,6×2,6 y Y=0..2,2, como el obstáculo actual.
- `muro_fondo` va en `(0,0,-7.6)`, `muro_lateral` se repite en `(±9.6,0,0)`, `parapeto_frontal` en `(0,0,7.6)` y `portal_fondo` en `(0,0,-6.95)`. Las torres altas van en `(±10.4,0,-8.2)` y las bajas en `(±10.4,0,8.2)`.
- Los cinco estandartes parten de `x=-7.2+3.6*k`, `y=2.8`, `z=-7.2`, alternando rojo y dorado. Las seis antorchas van en `x=±8.8`, `z=-4.5+4.5*j`, con `j=0..2`. La llama puede recibir partículas o luz dinámica de C++.
- Cada reja se compone de `marco_reja` fijo y `reja_levadiza` móvil, ambos con pivote XZ en su centro. Colocarlos en `(−5,0,0)`, `(5,0,0)`, `(0,0,−4.2)` y `(0,0,4.2)`. Girar **90° en Y** las rejas cuyo `mitadZ>mitadX` (las dos de X=±5). La hoja móvil lleva `Y=(1-reja.altura)*2.6`; el marco queda en Y=0. Mostrar `aviso_reja` en el suelo solo cuando el estado sea `REJA_TESORERO_AVISO`, con la misma rotación.
- `moneda_tesoro` tiene origen en el centro. Situarla en `moneda.y+.12` y conservar el giro, la caída, el tiempo de recogida y el puntaje existentes.

Son mallas estáticas modulares sin rig ni animaciones GLB. Para integrarlas, copiar `GLB/` a `Assets/Modelos/Escenarios/TesoreroAcorralado/`, conservar las reglas y sustituir solo las primitivas de dibujo. Cargar una vez cada modelo y descargarlo al cerrar. Esta entrega no modifica el repositorio del juego.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, abrir PowerShell dentro de la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorTesoreroCercado.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorTesoreroCercado.exe
```

Teclas 1: patio; 2: cámara del juego; 3: rejas cerradas; 4: castillo; flechas izquierda/derecha: girar; Escape: salir. `--capturar` regenera las capturas. Los cuatro cubos de colores del visor son referencias temporales de escala para jugadores, no GLB del paquete.
