# Sendero Invisible — modelos v1

19 GLB 2.0 originales para `MinijuegoSenderoInvisible.cpp`. La escena mantiene un estilo de geometría simple, piedra gris, hierro oscuro y llama verde. Cada GLB usa unidades raylib con Y hacia arriba. Son recursos visuales separados de la lógica actual; la ruta segura permanece oculta hasta que el juego la ilumine.

## Archivos

| Grupo | GLB |
| --- | --- |
| Cuadrícula | `losa_normal`, `losa_agrietada`, `losa_salida`, `losa_meta`, `fragmento_losa`, `brillo_losa` |
| Cementerio | `terreno_cementerio`, `borde_cementerio`, `lapida`, `lapida_cruz`, `mausoleo`, `arbol_seco`, `verja_tramo` |
| Luz y ambiente | `farol_verde`, `farol_recogible`, `fuego_fatuo`, `arco_meta`, `luna`, `niebla_abismo` |

`manifest.json` detalla pivotes, límites y materiales. `COLOR_DINAMICO` se usa en las marcas de la losa de salida; `BOMBILLAS` identifica llamas o núcleos luminosos, que pueden recibir emisión al integrar el modelo.

## Colocación y estados

- Cada jugador recibe 6 columnas × 10 filas. Centro de la losa: `x = XCarril(carril) + (columna−2.5)*1.5`, `z = 6−fila*1.5`, `y = 0`. La losa mide 1.36 × 0.4 × 1.36 y su cara superior está a Y=0. Los carriles están separados 10 unidades.
- Instanciar `losa_normal` en las casillas intermedias, `losa_salida` en la fila 0 y `losa_meta` en la 9. `losa_agrietada` reemplaza la normal únicamente cuando `grieta[indice]` sea verdadera. La ruta segura no lleva un modelo diferente. Omitir la losa cuando `estado==LOSA_SENDERO_CAIDA`; instanciar temporalmente cuatro `fragmento_losa` con las posiciones y la caída calculadas en C++.
- `brillo_losa` se superpone solo a las casillas cuya luz temporal devuelva `LuzLosa>0`, durante memorización o uso del farol. `fuego_fatuo` recorre `secuencia` en Y≈0.9 y `farol_recogible` va sobre la casilla de `farolIndice` hasta que sea recogido. Cada carril tiene su `arco_meta` en la última fila, Z≈−7.9.
- `terreno_cementerio` se coloca en `(0,0,-28)`, `borde_cementerio` en `(0,0,-9.6)` y cinco `verja_tramo` en Z=−9.4. Las tumbas, mausoleos, árboles y faroles se instancian detrás de la verja. `niebla_abismo` está por debajo de las losas; sus bancos son geometría opaca dispersa para no tapar el juego. `luna` es independiente y se posiciona respecto a la cámara al integrarla.

## Generación y revisión

`python3 generar_modelos.py` reconstruye los GLB sin dependencias. `visor_raylib.cc` se compila por separado con raylib y SDL y genera las vistas en `Vistas/`; la extensión `.cc` evita que la tarea recursiva `*.cpp` de VS Code lo incluya al compilar el juego. Las vistas de dos y cuatro carriles usan la fórmula de cámara real del minijuego. `VALIDACION.txt` resume la verificación técnica.

Los recursos están en GitHub, pero todavía no sustituyen las primitivas de dibujo del minijuego.
