# Autos de Globo — modelos v1

23 GLB 2.0 originales y modulares para `MinijuegoAutosGlobo.cpp`. Son geometría de colores planos para la estética actual del juego. Una unidad GLB equivale a una unidad raylib, Y es vertical y el frente local del auto es +X. Se dejan como recursos; la lógica y el dibujo existente siguen funcionando hasta que se integren los modelos.

## Archivos

| Grupo | GLB |
| --- | --- |
| Arena | `plaza_elevada`, `anillo_central`, `barrera_frontal`, `barrera_lateral` |
| Ciudad | `rascacielos_bajo`, `rascacielos_medio`, `rascacielos_alto`, `jardin_vertical`, `autopista_trasera`, `autopista_lateral`, `vehiculo_trafico` |
| Decoración móvil | `holograma_pedestal`, `holograma_anillos`, `dron_cuerpo`, `dron_helice` |
| Juego | `placa_aviso`, `placa_recarga`, `orbe_recarga`, `auto_flotante`, `globo_energia`, `cuerda_globo`, `llama_turbo`, `halo_auto` |

`manifest.json` detalla el pivote, los límites, materiales y uso de cada archivo. `COLOR_DINAMICO` marca las partes que corresponden al color del participante: chasis y globos. Las otras superficies conservan los acentos y el contraste. `BOMBILLAS` marca reflejos luminosos; la emisión real se puede asignar al cargar el modelo.

## Colocación

- La arena mide 18,3 × 12,9. Las barreras van en Z=±6,45 y X=±9,15; las laterales giran 90° alrededor de Y. El centro del auto se limita a X=±8,3 y Z=±5,6 con la lógica actual.
- El chasis se dibuja en `(a.x,0,a.z)` y gira `−a.angulo` alrededor de Y. `halo_auto` queda debajo. Instanciar exactamente `a.globos` unidades de `globo_energia` (0 a 3) detrás del auto, a Y≈1,9–2,1; conectar con línea dinámica o `cuerda_globo` transformada. `llama_turbo` solo aparece cuando `turboActivo>0`.
- La placa de aviso y la de recarga comparten radio lógico 1,1. Mostrar una u otra según `placa.enAviso`, con el orbe solo en fase activa. Mantener su temporizador y regeneración de globo en C++.
- Los edificios se sitúan fuera de la plaza con base Y=−6. Las autopistas y el tráfico quedan detrás o a un lado. Los cuatro hologramas van en `(±10,6,0,±7,6)`, con los anillos a Y≈2,4. Las hélices de drones son piezas separadas para giro independiente.

## Generación y revisión

`python3 generar_modelos.py` recrea los 23 GLB con Python estándar. `visor_raylib.cc` es el visor de prueba y genera `Vistas/`; compilarlo por separado con raylib y SDL. Usa extensión `.cc` para quedar fuera de la tarea recursiva `*.cpp` de VS Code. `Camara_del_juego.png` usa exactamente `(0,17.5,9.5)` hacia `(0,0,0.4)`, FOV 50°. `VALIDACION.txt` resume los controles.
