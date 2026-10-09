# Tuberías Desierto — modelos v1

20 GLB 2.0 originales para `MinijuegoTuberiasDesierto.cpp`. Estilo de arenisca cálida, cerámica, agua turquesa y oasis. La ruta y los puntos de cada ronda continúan en el estado C++; las piezas no codifican una solución fija.

| Grupo | Archivos GLB |
| --- | --- |
| Oasis y ruinas | `suelo_desierto`, `pared_arenisca`, `cisterna_oasis`, `agua_cisterna`, `columna_ruina`, `repisa_cantaros`, `plataforma_jugadores`, `palmera_oasis`, `cactus`, `duna`, `sol` |
| Acueducto dinámico | `colector`, `tubo_piedra`, `tubo_agua`, `codo_tuberia`, `compuerta_base`, `compuerta_hoja` |
| Salidas | `cantaro_barro`, `cantaro_oro`, `cantaro_miraje` |

## Escala y pivotes

Y hacia arriba, dimensiones en unidades de raylib. La pared se centra en `(0,0,-3.2)`, la cisterna en `(0,15.2,-3.2)` y su agua en `(0,16.74,-3.2)`. La repisa va en `(0,0,-1.9)` y la plataforma en `(0,0,3)`. `manifest.json` detalla dimensiones y posición de cada pieza.

`tubo_piedra` y `tubo_agua` miden una unidad a lo largo de +Y; para cada par consecutivo de `PuntoTuberia(m,e,u)`, situar el pivote en el primer punto, orientar +Y hacia el segundo y escalar su longitud. El canal frontal del tubo de piedra está abierto para que el agua interior sea visible. Se puede instanciar `codo_tuberia` en las curvas. Dibujar `tubo_agua` hasta `progresoAgua` y conservar el color de aviso del recorrido ganador. El colector se centra en `(0,13.7,-2.3)` y se ajusta horizontalmente para cuatro, cinco o seis entradas. Las compuertas se colocan en `XCarril(cantidadEntradas,e)`; la hoja sube `0.9 × compuerta`.

La identidad de los cántaros debe seguir el estado de la ronda. Si la regla exige ocultar el oro o el espejismo durante la observación, usar el cántaro común y cambiarlo al revelarlos. `cantaro_miraje` se retira y anima como partículas cuando se disipa. Son recursos visuales que todavía no sustituyen las primitivas del minijuego.

## Reproducir

`python3 generar_modelos.py` reconstruye los GLB sin dependencias. `visor_raylib.cc` comprueba la carga con raylib; `--capturar RUTA_ABSOLUTA_DE_ESTA_CARPETA` crea las capturas. La extensión `.cc` evita añadir otro `main` a la tarea de compilación `*.cpp`. `Vista_previa.png`, `Vistas/` y `VALIDACION.txt` documentan la revisión.
