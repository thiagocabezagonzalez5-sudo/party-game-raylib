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

## Integración en el juego

Los 22 GLB se usan en `Minigames/MinijuegoGruaChatarra.cpp`. Las rutas reales
de esta carpeta están centralizadas en `Core/RecursosJuego.h`. El paquete se
carga una vez al iniciar una ronda válida; no se carga al inicializar el
catálogo ni se duplica al reiniciar o cambiar la cantidad de participantes.

| GLB | Elemento sustituido y montaje |
| --- | --- |
| `suelo_desguace` | Dos cubos del patio/pozo, en el origen global. Se dibujan sus primitives 0, 1 y 2. |
| `escombros` | Los 46 cubos bajos, en sus puntos procedurales actuales. Escala explícita de cada instancia al ancho, alto y largo del cubo sustituido. |
| `muro_fondo` | Cubo del muro, con pie en `(0,0,-9.2)`. |
| `engranaje_gigante` | Dos cilindros y doce dientes por engranaje; centros `(-7,3.4,-8.2)` y `(7.5,3.4,-8.2)`, giro Z=`t*0.6` y `t*(-0.5)` radianes. |
| `prensa_estructura` | Columna, travesaño y base de la prensa, pie en `(10.6,0,-3.5)`. |
| `prensa_plato` | Plato móvil, centro en `(10.6,4-1.5*prensa,-3.5)`; conserva el seno procedural. |
| `auto_aplastado_rojo`, `auto_aplastado_azul`, `auto_aplastado_amarillo` | Los dos cubos por auto; pies en Y=`0.05+0.5*k`, conservando X/Z y el orden de apilado. |
| `cinta_transportadora` | Base y ocho franjas por cinta, en X=±9.9, Z=1.6. Conserva el desplazamiento original de franjas en ambos sentidos. |
| `pila_chatarra` | Nueve cubos del frente, en X=`-10+2.5*i`, Z=6.6; escala Y=`alto` actual, X/Z=1. |
| `foco_industrial` | Poste y bombilla de los cuatro focos, pie en X=`-8+5.4*k`, Z=8. El halo se conserva y se alinea con la bombilla del GLB. |
| `tolva_equipo` | Tolva, embudo, plataforma y respaldo del operador; X depende de `puesto` y participantes, Z=-6.3. |
| `garra_iman` | Dos cilindros del imán, con pivote central en `(estado.x,AlturaGarraGrua(estado),estado.z)`. |
| `garra_pinza` | Cuatro cubos por garra; unión superior en Y=`altura-0.02`, radio=`apertura*0.7`, giro Y=`90-a*RAD2DEG` para orientar cada punta hacia dentro. |
| `carro_grua` | Cubo del carro en `(estado.x,8,estado.z)`; conserva el cable procedural hasta Y=`altura+0.35`. |
| `marca_garra` | Dos círculos del suelo; sigue X/Z y alpha/color del jugador. |
| `objeto_tuerca`, `objeto_engranaje`, `objeto_motor`, `objeto_bateria`, `objeto_cartucho` | Cuerpos de los cinco objetivos, pie en la posición actual y giro Y=`objeto.giro`. Al transportarlos, Y=`altura-0.75` y giro=`tiempoAnimacion*40`. |

El suelo incluye escombro estático en sus primitives 3/4: se omiten para
reemplazarlo por las instancias modulares en las posiciones originales y
con fallback independiente. Las vistas de mallas no poseen ni duplican
recursos. Todos los archivos conservan sus pivotes; no se normalizan a la base
y cada instancia recibe una única transformación explícita, sin jerarquías
adicionales de `rlgl`. Las colisiones y tolerancias de captura siguen siendo
las originales, independientes de las dimensiones de los GLB.

La cinta anima únicamente los vértices Z de la primitive 3 (`amarillo`), con
una copia CPU de reposo y un solo VBO compartido. Mantiene exactamente el
`fmod(k+t*1.2*lado,8)` original, incluido el recorrido asimétrico del lado
negativo. Cada instancia restaura CPU y GPU después de dibujar; materiales,
colores de vértice, normales, base y herrajes permanecen intactos.

Solo se tiñe `COLOR_DINAMICO`: primitive 3 de la tolva y primitive 0 del imán
y marca, resueltas mediante `meshMaterial`. Los materiales se restauran tras
cada instancia, con tinte global blanco. Se conservan los halos animados de
baterías, halos de focos, cable, jugadores, indicadores de captura/debug,
mensajes y HUD. Las llamadas directas evitan las manchas automáticas de
`SombrasRetro.h` sobre marcas, halos o piezas del suelo; los personajes
conservan su sistema de sombras.

Si falta o falla una pieza, solo esa pieza usa sus primitivas originales y
registra un diagnóstico por intento de carga, sin repetirlo cada frame.
`ZonaPruebas::Descargar()` descarga los modelos y la copia CPU de la cinta
mediante el almacén compartido antes de `CloseWindow()`, también en partidas
oficiales de tablero. No quedan modelos sin usar.

## Verificación de integración

Compilar con la tarea existente de Windows UCRT64. La prueba siguiente enlaza
el juego real y ejecuta ZonaPruebas en una ventana OpenGL oculta:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/VerificarGruaChatarra.ps1
```

Comprueba 2, 3 y 4 participantes, cuenta regresiva, movimiento, bajada,
captura de los cinco tipos, transporte, depósito, retorno, fallo, disputa,
cartucho, reaparición, IA, final, reinicio y salida/protección de ronda
oficial. Verifica carga única, materiales restaurados, pivotes, animación
de cintas, vértices intactos, los 22 fallbacks y descarga simétrica. Las
capturas `build/grua-*.png` corresponden al juego integrado con su cámara
y HUD. Los gamepads físicos requieren una comprobación manual adicional.
