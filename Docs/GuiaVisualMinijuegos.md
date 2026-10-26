# Guía visual de los minijuegos

Lo que todo minijuego (actual y futuro) debe cumplir para verse al nivel de los **duelos** (Cables Cruzados, Vagonetas de Mina, Gemas Deslizantes) y de la cueva de **Refugio Taladros**. Es el equivalente de `ReglasTableros.md` para los minijuegos: la decisión de diseño es del usuario; esta guía la baja a reglas concretas.

**Referencias en el código** (leer antes de rehacer un escenario):

| Qué mirar | Dónde | Por qué |
|---|---|---|
| Mundo por segmentos con niebla de color, luces aditivas y lava animada | `MinijuegoVagonetasDuelo.cpp` (`TinteVagonetas`, `DibujarSegmentoVagonetas`, `DibujarVistaVagonetas`) | Profundidad y atmósfera |
| Mesa de trabajo con utilería, cubos con caras sombreadas, aparición en ola | `MinijuegoCablesCruzados.cpp` (`CuboCables`, `DibujarEscenarioCables`, `EscalaAparicionCables`) | Densidad de props y animación de entrada |
| Escenario + HUD común + anuncios sobre el mundo | `MinijuegoCablesCruzados.cpp::Dibujar` | Plantilla del `Dibujar` final |
| Nave industrial completa, luces falsas y pantalla dividida sin RenderTexture | `Minijuego67.cpp` (`DibujarLuces67`, `DibujarUtileria67`) | Iluminación y pantalla dividida |
| Barco en alta mar con horizonte, islas y faro | `MinijuegoCapitanManda.cpp` | Exterior con agua y cielo |
| Cueva con taladros, rocas y polvo | `MinijuegoRefugioPinchos.cpp`, `EfectosVisualesMinijuegos.h` (`DibujarTaladrosCueva`) | Interior cerrado |
| Etiquetas, textos con contorno, velo, banderines | `UI/EstiloUI.h`, `Docs/ui_style.md` | Estilo «Carteles de playa» |
| HUD común «El tendedero» | `Minigames/HudMinijuego.h` | Interfaz obligatoria |

Escenarios ya rehechos con esta guía (sirven de ejemplo): Color Seguro, Pelotas, Tronco 2V2, Fábrica 67, Isla Bajo Fuego, Capitán Manda, Núcleos de Energía, Conteo Explosivo, Carga Inestable, Secuencia Neón, Cantera en Fuga, Cajas del Puerto, Cápsulas Barajadas.

---

## 1. Regla principal: el escenario se rehace entero

Un minijuego viejo **no se retoca**: se borra todo su código de dibujo del mundo (suelo, fondo, decoración, colores sueltos) y se construye de nuevo, **conservando la idea del lugar** (isla, volcán, montaña nevada, etc.; ver la ficha de cada uno en la sección 9) y **sin tocar la jugabilidad**.

- La lógica, las medidas de la arena, las colisiones, los tiempos, los bots y el azar de la partida quedan **idénticos**. Si un cambio visual exige mover una pared o una plataforma, el dibujo se adapta a la zona real, nunca al revés (el borde dibujado tiene que coincidir con el borde que mata o empuja).
- El azar visual usa su propia semilla (`semillaVisual`, hash determinista por índice). **Nunca** consume `GetRandomValue` de la lógica.
- Lo que antes se mostraba con texto en pantalla y sigue haciendo falta (por ejemplo «MURO 2», «SALVÁ: IZQUIERDA») pasa al HUD común o a un anuncio sobre el mundo (sección 6).
- Los `Tests/Verificar*.ps1` existentes deben seguir pasando.

## 2. Composición en capas

Toda escena se arma con cinco capas, de adelante hacia atrás. Una escena que no tenga las cinco no está terminada.

1. **Arena jugable.** Donde están los personajes. Suelo con material (tablones con vetas, losas con juntas, hielo con grietas, metal con remaches), nunca un color plano. Variación por baldosa o tablón con hash (±6–12 % de brillo). Los límites se ven: bordillo, baranda, cornisa con grosor, desnivel o agua.
2. **Anillo cercano.** Lo que rodea la arena y está a 1–6 unidades: utilería del lugar (barriles, cajas, faroles, rocas, plantas, máquinas). Es lo que hace que «se sienta un lugar». Mínimo **5 tipos de prop** distintos y temáticos.
3. **Media distancia.** Estructuras grandes que encuadran: muros, acantilados, tribunas, edificios, árboles, columnas, paredes de la cueva. Siluetas variadas en altura, no una pared lisa.
4. **Horizonte.** Montañas, mar, ciudad, dunas, volcanes o paredes lejanas, con **al menos dos profundidades** de silueta (la de atrás más clara y desaturada por la niebla).
5. **Cielo o techo.** Degradé (`DrawRectangleGradientV` antes de `BeginMode3D`, o cúpula), con elementos: nubes, sol o luna, estrellas, aurora, estalactitas, focos del techo. Nunca `ClearBackground` de un solo color como fondo visible.

**Encuadre:** la arena ocupa entre el 55 y el 75 % del ancho de la pantalla; no quedan franjas vacías de color liso en los bordes del cuadro. Lo que la cámara ve detrás de la arena tiene que estar construido.

## 3. Materiales, color y luz

- **Volumen por caras.** Cada caja se dibuja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70, derecha 0.62), como `CuboCables` o `CuboPistaVagonetas`. Nada de cubos de un solo tono ni `DrawCubeWires` como decoración.
- **Paleta por escena:** 5–7 colores base más **un acento** que se reserva para lo importante (lo que el jugador debe mirar: la bandera, la carga, la meta). Los colores de los jugadores (rojo, azul, verde, amarillo) no se usan en el escenario salvo para marcar zonas de cada jugador.
- **Contraste de lectura:** suelo de valor medio, personajes y objetos jugables más saturados o más claros que el suelo. Ninguna textura del suelo compite con los jugadores.
- **Niebla de color:** función de tinte por distancia (como `TinteVagonetas`) que lleva los colores lejanos hacia el color del cielo o de la niebla del lugar. Da profundidad sin costo.
- **Luces falsas aditivas** al final del 3D, con `rlDrawRenderBatchActive(); rlDisableDepthMask(); BeginBlendMode(BLEND_ADDITIVE)`: halo en la fuente (dos esferas), cono de luz (`DrawCylinderEx` muy transparente) y charco de luz en el suelo (discos concéntricos con alfa 0.02–0.05). Cada escena tiene **al menos una fuente de luz con su charco** (farol, lava, sol que entra, monitor, antorcha).
- **Hora del día con intención:** atardecer, noche, mediodía o tormenta; el color del cielo, la niebla y las luces cuentan lo mismo.

## 4. Vida y animación ambiental

- **Al menos dos animaciones ambientales** que no dependan del jugador: agua con olas o brillo, banderas o telas que ondean, nubes que se desplazan, luces que titilan, máquinas que giran, lava que late, hojas o polvo que cruzan.
- **Partículas ambientales** de la escena (brasas, nieve, polvo, luciérnagas, burbujas, chispas), escaladas con `FactorCalidadGrafica(CalidadParticulas())`.
- **Entrada:** durante la cuenta regresiva la cámara hace un movimiento suave de acercamiento o barrido (como Vagonetas, que baja desde más lejos), multiplicado por `ObtenerFactorMovimientoCamaraMinijuegos()`. Los elementos jugables pueden aparecer en ola con rebote (`EscalaAparicionCables`).
- Todo movimiento de cámara (temblor incluido) respeta **Reducir movimiento**.

## 5. Personajes y feedback

- Personajes con el **modelo compartido** (`DibujarJugadorModeloCompartido` o `DibujarModeloJugadorEnPosicion`) y su sombra (`SombrasRetro.h`). Si el minijuego usa vehículos u objetos propios (tanques, autos, planeadores, balsas), el jugador va visible encima o al costado.
- Cada jugador lleva un **cartelito «J1..J4»** sobre la cabeza con `DibujarEtiquetaUI` (`borde` = color del jugador, `pico = true`) y, cuando ayuda a leer la escena, un **anillo de su color** en el suelo.
- **Feedback de cada acción importante:**
  - acierto / punto: destello, pequeño salto de escala, partículas del color del jugador y `ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO)`;
  - error / golpe: sacudida corta, partículas grises o de impacto, `SONIDO_ERROR` / `SONIDO_GOLPE`, y `ActivarTemblorCamaraGeneral` solo para golpes fuertes;
  - eliminación o caída: animación de salida (vuela, cae al agua, se hunde), `SONIDO_CAIDA`;
  - fin: confeti sobre el ganador.
- Audio de estructura en todos: `ActualizarAudioCuentaRegresiva` y `ActualizarAudioAlertaTiempo` (si hay reloj).

## 6. Interfaz: solo el HUD común

- **Obligatorio:** cada frame se llena `DatosHudMinijuego` (`PrepararDatosHudMinijuego` + jugadores) y se llama a `DibujarHudMinijuego` al final de `Dibujar`, después de `EndMode3D`. Fases `HUD_MINIJUEGO_CUENTA` / `JUGANDO` / `FIN`, con `resultado` para el tablón final.
- **Se borra toda la interfaz vieja:** rectángulos oscuros con título arriba a la izquierda, barras de jugadores al pie, cajas de «TIEMPO», párrafos de controles, listas de «J2 BOT EN JUEGO». Nada de `DrawRectangle` translúcido como panel.
- Qué va en cada lugar:
  - **Título y objetivo** → `titulo` y `objetivo` (una línea en el cartel de presentación).
  - **Tiempo** → `HUD_RELOJ_CUENTA_ATRAS`; si no hay tiempo, `HUD_RELOJ_MEDIDOR` (ronda, muro, turno, pila…).
  - **Puntaje o estado por jugador** → `jugadores[i].valor` / `valorNumerico` / `progreso` / `estado` / `eliminado` / `rol` / `equipo`.
  - **Controles** → `jugadores[i].controles`, solo humanos, máximo dos líneas cortas (usar `TextoMoverHud`, `TextoBotonGolpeHud`, `ObtenerTextoBotonPrincipal`).
  - **Anuncios grandes** («¡RONDA 2!», «¡IZQUIERDA!», «¡PUNTO EQUIPO 1!») → `DibujarTextoMundoContornoUI`, centrados, en el tercio superior o sobre el objeto que lo provoca, breves y animados.
  - **Marcador de equipos** (2 vs 2) → `equipo` en cada jugador; si hace falta un marcador grande, un único cartel de madera arriba al centro con el estilo de `EstiloUI`, nunca un panel oscuro.
- Nada se dibuja sobre la franja de ayuda inferior: respetar `MargenBarraRondaMinijuegos()`.
- Textos con voseo, tildes y signos de apertura («¡», «¿»).

## 7. Rendimiento y calidad gráfica

- Objetivo: 60 FPS estables a 1920x1080 en calidad ALTA con 4 jugadores.
- `Systems/CalidadGrafica.h`:
  - `decoracion == CALIDAD_BAJA`: solo arena, bordes y cielo/fondo simple;
  - `CALIDAD_MEDIA`: props a la mitad (paso 2 en bucles de utilería, horizonte simplificado);
  - `efectos == CALIDAD_BAJA`: sin luces aditivas ni agua animada; `sombras` controla `SombrasRetro`.
- Escenarios grandes y estáticos: preferir pocos cubos grandes con buen sombreado a miles de cubos chicos; agrupar con `rlBegin(RL_TRIANGLES)` por material (como `CuboCables`) o hornear una malla una sola vez si el escenario supera unos miles de piezas.
- **Pantalla dividida** con `BeginScissorMode` + `rlViewport` + proyección propia (Vagonetas, Fábrica 67). Sin `RenderTexture`.
- Sin texturas externas nuevas: todo se construye con primitivas. Las piezas que algún día serán modelos GLB se marcan con un comentario `// MODELO FUTURO: ...`.

## 8. Lista de comprobación de un minijuego terminado

1. Escena vieja borrada; las cinco capas de la sección 2 presentes.
2. ≥ 5 tipos de prop temáticos, ≥ 2 animaciones ambientales, ≥ 1 fuente de luz con charco, niebla de color.
3. Suelo con material y bordes con grosor coincidentes con la zona real.
4. Personajes con modelo, sombra, cartelito J# y feedback (partículas + sonido) en las acciones importantes.
5. HUD común como única interfaz; anuncios con texto de contorno; controles en la etiqueta del jugador.
6. Calidad BAJA, MEDIA y ALTA respetadas; Reducir movimiento respetado.
7. Jugabilidad intacta: mismas reglas, medidas y bots; los `Tests/Verificar*` del minijuego pasan.
8. Capturas revisadas a 1280x720 (cuenta regresiva, juego en curso y tablón final) con 2 y 4 jugadores.
9. Compila sin errores ni advertencias nuevas.

## 9. Ficha de escenario por minijuego

Cada ficha conserva la **idea del lugar** actual y fija la dirección del rediseño. Cuando dos minijuegos comparten idea, se separan por hora del día y paleta.

| # | Minijuego | Idea que se conserva | Escenario nuevo (dirección) | Estado |
|---|---|---|---|---|
| 1 | Color Seguro | Islas sobre lava | Cráter volcánico | Rehecho |
| 2 | Pelotas | Cumbre nevada | Cima alpina | Rehecho |
| 3 | Tronco 2V2 | Carrera de troncos | Aserradero al atardecer | Rehecho |
| 4 | Fábrica 67 | Fábrica con cintas | Nave industrial | Rehecho |
| 5 | Isla Bajo Fuego | Isla bombardeada | Isla tropical con barco de cañones | Rehecho |
| 6 | Capitán Manda | Barco y capitán | Barco en alta mar de día | Rehecho |
| 7 | Barra Giratoria | Plataforma alta en el cielo | Templo celeste: plataforma de piedra tallada sobre un mar de nubes al amanecer, eje con engranajes de bronce, banderolas, islas flotantes con cascadas, aves | Rehecho |
| 8 | Núcleos de Energía | Recolección de energía | Sala de energía | Rehecho |
| 9 | Refugio Taladros | Cueva con taladros | Cueva (referencia) | Referencia |
| 10 | Miradas Cruzadas | Escenario con arcoíris y cabezas gigantes | Teatro de títeres en una feria de campo: tablado de madera con telón, cabezas-títere gigantes, arcoíris pintado de utilería, guirnaldas, carpas y colinas | Rehecho |
| 11 | Muros Locos | Ruinas del desierto, muros de adobe | Ruinas de adobe al mediodía: patio de baldosas gastadas, columnas rotas, arcos, vasijas, palmeras secas, dunas en dos planos, calima | Rehecho |
| 12 | Tormenta Magnética | Núcleo que atrae y repele | Plataforma de pruebas en una tormenta eléctrica: bobinas Tesla, pararrayos, nubes bajas con relámpagos, chispas, lluvia | Pendiente |
| 13 | Conteo Explosivo | Drones y conteo | Hangar | Rehecho |
| 14 | Paso Silencioso | Centinela que duerme | Salón de un templo nocturno con un gigante de piedra dormido: pasillos de losas, columnas, braseros, tapices, luz de luna por los ventanales | Pendiente |
| 15 | Circuito Voltaje | Circuito de carreras | Autódromo nocturno de autos eléctricos: asfalto con pianos, tribunas con público, torres de luz, boxes, semáforo y arco de meta | Pendiente |
| 16 | Trazo Perfecto | Figuras sobre lienzos | Atelier de pintor: caballetes con lienzos, mesa con frascos de pintura y pinceles, ventanal con ciudad al atardecer, salpicaduras en el piso | Pendiente |
| 17 | Carga Inestable | Carga que explota | Forja circular con núcleo de lava | Rehecho |
| 18 | Secuencia Neón | Memoria con pulsos | Sala neón | Rehecho |
| 19 | Interruptores | Palancas y sobrecarga | Sala de control de una central eléctrica: consolas con monitores, tableros de palancas, turbinas, cables en bandejas, luces de alarma | Pendiente |
| 20 | Tanques Plasma | Arena con cristales | Cañón de cristal alienígena: suelo de roca violeta, cristales luminosos, planeta y lunas en el cielo, restos de naves | Pendiente |
| 21 | Pasarelas del Vacío | Plataformas sobre el vacío | Pasarelas en una nebulosa: losas de piedra flotante con runas, islas suspendidas, cadenas, nebulosa de colores y estrellas | Pendiente |
| 22 | Cantera en Fuga | Pendiente con rocas | Cantera | Rehecho |
| 23 | Territorio | Mundo dulce | Pastelería gigante: mesa de mantel a cuadros, tortas, macarons, cupcakes, glaseado que gotea, ventana con cocina de fondo | Pendiente |
| 24 | Defensa Núcleo | Ciudad futurista | Estadio en la azotea de un rascacielos de noche: cancha con líneas de luz, gradas, arcos de energía, ciudad con letreros y autos voladores | Pendiente |
| 25 | Lluvia Apilada | Observatorio | Cima del observatorio de noche: cúpula con telescopio, bases de piedra, cielo estrellado con estrellas fugaces, montañas | Pendiente |
| 26 | Cuerda Acantilado | Acantilado, faro y mar | Dos acantilados con faro sobre el mar al atardecer: rocas estratificadas, olas que rompen, gaviotas, pasto y flores | Pendiente |
| 27 | Último Asiento | Carrusel de feria | Feria de día: carrusel con caballitos y tazas, noria, puestos, guirnaldas de bombillas, globos | Pendiente |
| 28 | Cajas del Puerto | Puerto y grúa | Muelle | Rehecho |
| 29 | Laberinto Jade | Templo antiguo de jade | Cámara interior de un templo de jade: losas talladas, altar, columnas con relieves, antorchas, estatuas | Pendiente |
| 30 | Veta de Cristal | Mina y geodas | Mina de cristales azules y violetas (sin lava, para separarla de Vagonetas): vigas, lámparas, rieles, geodas | Pendiente |
| 31 | Cápsulas Barajadas | Laboratorio | Laboratorio | Rehecho |
| 32 | Bateo Meteórico | Espacio y meteoritos | Plataforma en órbita: planeta anillado, cúpula con telescopio, lluvia de meteoritos, estrellas | Pendiente |
| 33 | Racimo Tóxico | Árbol y cabañas en el pantano | Pantano de noche: árbol gigante con racimo, cabañas sobre pilotes, agua con nenúfares, luciérnagas, niebla verde | Pendiente |
| 34 | Tesorero Cercado | Patio de castillo con rejas | Patio de castillo medieval: murallas con almenas, torres, estandartes, antorchas, foso, carros y barriles | Pendiente |
| 35 | Descenso en Nubes | Cielo con nubes | Cielo de tarde con islas flotantes: nubes en capas, globos, molinos, aves, arcoíris, isla diana | Pendiente |
| 36 | Volea de Magma | Cancha volcánica | Cancha de obsidiana junto a un lago de lava: volcanes, columnas de basalto, red de cadenas, brasas | Pendiente |
| 37 | Parejas Glaciares | Glaciar | Glaciar con aurora: témpanos, icebergs, pingüinos, cueva de hielo, nieve que cae | Pendiente |
| 38 | Esferas del Cañón | Cañón del desierto | Cañón de roca roja: paredes estratificadas, arco natural, cactus, puentes rotos, mesetas lejanas | Pendiente |
| 39 | Pesca Isleña | Laguna en una isla | Laguna tropical: muelles de bambú, palmeras, arrecife visible bajo el agua, barca, volcán lejano | Pendiente |
| 40 | Rodillos Neón | Máquinas tragamonedas neón | Sala arcade retro: máquinas con marquesinas, alfombra estampada, letreros neón, columnas de LEDs | Pendiente |
| 41 | Bolas de Azúcar | Mundo de dulces | Montaña de dulces nevada de azúcar glas: suelo de galleta, piruletas, columnas de caramelo, ríos de chocolate | Pendiente |
| 42 | Grúa de Chatarra | Depósito de chatarra | Desguace al atardecer: pilas de autos aplastados, prensa, cintas, focos, grúas de fondo | Pendiente |
| 43 | Pisotón de Plagas | Jardín gigante | Jardín visto desde el tamaño de un bicho: pasto gigante, flores, setas, regadera, cerca de madera | Pendiente |
| 44 | Balsas del Rápido | Río de selva | Río selvático: paredes de selva, lianas, ruinas, cascadas, loros, niebla de agua | Pendiente |
| 45 | Autos de Globo | Ciudad futurista | Plaza flotante entre rascacielos de día: jardines verticales, autopistas aéreas, hologramas, drones | Pendiente |
| 46 | Sendero Invisible | Cementerio | Cementerio de noche: losas agrietadas, lápidas, mausoleos, verja, faroles de llama verde, luna llena, niebla | Pendiente |
| 47 | Banquete Turbo | Comedor espacial | Comedor orbital: ventanal a la Tierra, mesas magnéticas, tubos de comida, robot camarero, luces de servicio | Pendiente |
| 48 | Tuberías Desierto | Oasis y tuberías | Oasis al atardecer: acueducto de arenisca, cisterna con palmeras, cántaros, dunas | Pendiente |
| 49 | Trepá el Mástil | Barco y mástiles | Barco amarrado en una caleta al atardecer (distinto de Capitán Manda): fuerte en la costa, velas recogidas, jarcias, cofres, gaviotas | Pendiente |
| 50 | Guardián de Ruinas | Ruinas con portal | Ruinas exteriores tomadas por la selva: portal rúnico, columnas rotas con enredaderas, estatuas caídas, rayos de sol entre árboles | Pendiente |
| 51–53 | Duelos | — | Referencia | Referencia |

Al terminar un minijuego se cambia su estado a **Rehecho** en esta tabla.

## 10. Flujo de trabajo para rehacer un minijuego

1. Leer el `.h` y el `.cpp` por tramos: separar qué es lógica (no se toca) y qué es dibujo (se reemplaza).
2. Sacar una captura «antes» para ver cámara y encuadre.
3. Borrar el dibujo viejo del mundo y la interfaz vieja; dejar `Dibujar` con la estructura de `MinijuegoCablesCruzados::Dibujar` (fondo → 3D por capas → luces aditivas → `EndMode3D` → anuncios → HUD común).
4. Construir capa por capa (sección 2), luego materiales y luz (3), animación (4), feedback (5) y HUD (6).
5. Compilar, capturar y corregir hasta cumplir la lista de la sección 8.
6. Actualizar la ficha de la sección 9 y, si aparecen piezas nuevas para modelos, la lista «Modelo futuro» de `CatalogoMinijuegos.md`.
