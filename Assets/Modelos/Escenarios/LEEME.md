# Grúa de Chatarra — paquete de modelos v1

22 modelos originales GLB 2.0, de geometría simple y colores planos, adaptados al minijuego de la rama `claude/expansion-party` (`ea5472c`). Las unidades coinciden con raylib; Y es el eje vertical. Son archivos independientes del código del juego, listos para sustituir la geometría visual sin modificar la lógica.

## Contenido

| Grupo | Archivos |
| --- | --- |
| Arena y maquinaria | `suelo_desguace`, `escombros`, `muro_fondo`, `engranaje_gigante`, `prensa_estructura`, `prensa_plato`, `cinta_transportadora` |
| Decorado | `auto_aplastado_rojo`, `auto_aplastado_azul`, `auto_aplastado_amarillo`, `pila_chatarra`, `foco_industrial` |
| Juego | `tolva_equipo`, `garra_iman`, `garra_pinza`, `carro_grua`, `marca_garra`, `objeto_tuerca`, `objeto_engranaje`, `objeto_motor`, `objeto_bateria`, `objeto_cartucho` |

`manifest.json` contiene los límites, pivotes, materiales y sugerencias de colocación por archivo. Las variantes de equipo usan el material blanco `COLOR_DINAMICO` para aplicar el color del jugador. `BOMBILLAS` identifica las superficies luminosas, aunque la emisión e iluminación real deben configurarse en el motor.

## Montaje y animación

- Instanciar el suelo en el origen, el muro en `(0,0,-9.2)`, dos engranajes en `(-7,3.4,-8.2)` y `(7.5,3.4,-8.2)`, y la prensa en `(10.6,0,-3.5)`. El plato separado se traslada a `(10.6, 4-1.5*prensa, -3.5)`, con `prensa=0.5+0.5*sin(t*1.4)`.
- Las tolvas van en `(x,0,-6.3)`, con `x=(puesto-(N-1)/2)*4.6`, y se tiñen por equipo. Los objetos tienen pivote al pie y pueden desplazarse entre el pozo y la garra conservando la lógica de captura y puntuación actual.
- El imán tiene pivote central en `(estado.x, altura, estado.z)`. Instanciar cuatro `garra_pinza` a un radio `apertura*0.7`, con ángulos `π/4+k*π/2`; `apertura` varía de `0.6` a `0.25` según el estado. El `carro_grua` va en Y=8; dibujar el cable como línea dinámica entre el carro y `(x,altura+0.35,z)`.
- Las cintas pueden repetir el GLB tal como está o animar las franjas en el motor. Los engranajes grandes giran alrededor de su eje Z. Mantener la detección de agarre y los radios lógicos existentes: los volúmenes GLB son solo visuales.

## Reproducción y revisión

`python3 generar_modelos.py` reconstruye todos los GLB sin dependencias externas. `visor_raylib.cpp` los carga y captura las vistas de `Vistas/`; requiere raylib y SDL para compilar. La vista `Camara_del_juego.png` usa la cámara del minijuego `(0,16,11.5)` hacia `(0,0,-0.3)` a 50 grados. `VALIDACION.txt` resume la inspección técnica.

El paquete no reemplaza todavía los `DrawCube` y otras primitivas del repositorio. Es la entrega de assets para la futura integración.
