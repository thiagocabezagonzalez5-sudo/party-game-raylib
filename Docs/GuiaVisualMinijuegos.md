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
10. Verificado con el flujo real del juego (ventana y `config.ini` reales, en calidad BAJA, MEDIA y ALTA): el bucle principal no limpia la pantalla, así que un fondo hecho con un degradado 2D no reemplaza a `ClearBackground` (desde ahora lo hace `ZonaPruebas::Dibujar`, pero el minijuego no debe depender de eso para verse bien en otros flujos).

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
| 12 | Tormenta Magnética | Núcleo que atrae y repele | Plataforma de pruebas en una tormenta eléctrica de noche: cubierta de chapa de lágrimas con emisores de cobre y franja de peligro, torre sobre un mar de nubes, bobinas Tesla con arcos, pararrayos, mástiles de focos, torre de control, tendido de alta tensión, cordillera en dos planos, lluvia y relámpagos (uno cae sobre el núcleo en cada cambio de campo) | Rehecho |
| 13 | Conteo Explosivo | Drones y conteo | Hangar | Rehecho |
| 14 | Paso Silencioso | Centinela que duerme | Salón de un templo de noche: cuatro pasillos de losas con grecas y tachas de bronce, columnatas, ventanales y óculos con luz de luna, braseros, velas, lámparas colgantes, tapices que ondean, guardianes, gong y urnas; al fondo un gigante de piedra en su trono (respira dormido, ojos ámbar al despertar y mirada roja que barre los pasillos) recortado contra una logia abierta a la luna, montañas y una pagoda | Rehecho |
| 15 | Circuito Voltaje | Circuito de carreras | Autódromo nocturno de autos eléctricos en una meseta: óvalo peraltado de cuatro carriles con pianos rojos y blancos en las curvas, muro con publicidad y chevrones, alambrado, calle de boxes con ocho garajes iluminados y terraza VIP, tribuna principal con visera y laterales con público que festeja, seis torres de luz, pórtico de meta a cuadros con semáforo y contador de vueltas, pantalla gigante con las posiciones, infield con podio, cargadores, carpas y auto de seguridad, ciudad en el valle, sierras con molinos, luna y fuegos artificiales al final | Rehecho |
| 16 | Trazo Perfecto | Figuras sobre lienzos | Atelier de pintor al atardecer: una fila de caballetes con lienzos inclinados (boceto a carbonilla que se dibuja solo en la cuenta, trazo de pintura espesa con contorno, estrella dorada y pincel), cada pintor de espaldas frente al suyo; piso de tablones con lona arrugada y salpicaduras (las gotas del pincel dejan manchas nuevas), ventanal de hierro con cortinas que ondean y la ciudad vieja detrás (techos con ventanas que se encienden, cúpula, campanario, torres en la calima, sol bajo, nubes y pájaros), haces de sol con polvo, mesa con frascos, pinceles y paleta, carrito, busto de yeso, taburete, lienzos apilados, plantas, escalera y lámparas colgantes | Rehecho |
| 17 | Carga Inestable | Carga que explota | Forja circular con núcleo de lava | Rehecho |
| 18 | Secuencia Neón | Memoria con pulsos | Sala neón | Rehecho |
| 19 | Interruptores | Palancas y sobrecarga | Sala de control de una central eléctrica de noche: piso técnico de baldosas con rejillas y franja de precaución, tarimas de chapa semilla con banda del color de cada operador; consola verde menta con botoneras, rejillas y medidores, tablero de instrumentos por palanca (dial con aguja y lámpara de estado: crema libre, ámbar elegida con flecha y halo, verde segura, roja fatal) y palancas que aparecen en ola en cada ronda; muro de monitores con medidor de sobrecarga que sube con cada palanca segura, osciloscopios, radares y esquemas; aislador colgante de barra colectora con arcos que trepan (de ahí salta la descarga al operador, que vuela y queda tendido y chamuscado); racks con LEDs, armario de cortes con empalme que chispea, casilleros, escritorios con monitor de tubo, sillas, bobinas de cable, pizarra, carro y conos; tubos fluorescentes, bandejas de cables y balizas giratorias que se aceleran y pasan a rojo en la sobrecarga; ventanal a la nave de turbinas (tres turbogeneradores con volante que gira, puente grúa que va y viene, luces de sodio) y, por sus ventanales, torres de enfriamiento con vapor, tendido de alta tensión con balizas, sierra en dos planos, luna y estrellas | Rehecho |
| 20 | Tanques Plasma | Arena con cristales | Cañón de cristal alienígena al anochecer: arena de losas de roca violeta con juntas irregulares, grietas y vetas, bordillo de roca con franja de energía y emisores de esquina con orbe y anillo que gira; tres racimos de cristal cian sobre bases de roca del radio exacto de la colisión (laten, destellan al recibir plasma y dejan charco de luz); cornisa con cápsula de escape que parpadea, contenedores, planchas de casco, rocas, racimos rosa y lila y hongos-tallo luminosos; nave estrellada con el motor ardiendo sobre el precipicio, baliza alienígena con haz al cielo y agujas de cristal gigantes; abismo con resplandor y rocas flotantes, el otro lado del cañón estratificado, mesetas y agujas en dos planos, anillo de una nave antigua; cielo con planeta anillado, dos lunas, nebulosa, estrellas y estrellas fugaces; tanques con piloto en la escotilla, celdas de vida, culatazo y fogonazo, plasma con estela y luz, explosión con torreta que salta y piloto eyectado | Rehecho |
| 21 | Pasarelas del Vacío | Plataformas sobre el vacío | Pasarelas en una nebulosa: losas de piedra flotante del tamaño exacto de su colisión, con hiladas, bordillo claro, baldosas a soga y raíz de roca con cristales colgantes; el círculo de runas cuenta el desgaste (cian, naranja, rojo y titila al pisarla), aparecen grietas, la losa tiembla y al caer se parte en dos mitades con cascotes; cadenas a rocas ancla que quedan colgando, salida con pilares y braseros de fuego astral, meta con arco de dovelas, clave y línea doradas, portal de chispas y columna de luz; isletas con braseros, faroles, cristales, árboles astrales y ruinas, monolitos rúnicos que giran; islas suspendidas con templos, torres y cascadas de polvo estelar, anillo rúnico gigante roto; islas hundidas en dos profundidades entre nubes de nebulosa; cielo de nebulosa con franjas de polvo, estrellas, galaxia espiral y estrellas fugaces; vista en perspectiva que baja en la cuenta, losas que suben en ola, caída de los eliminados al vacío | Rehecho |
| 22 | Cantera en Fuga | Pendiente con rocas | Cantera | Rehecho |
| 23 | Territorio | Mundo dulce | Pastelería gigante al atardecer: la arena es una torta cuadrada sobre bandeja plateada, cercada de barritas de chocolate (bajas al frente para ver la última fila) con moño lila; cada cuadro es una baldosa de glaseado del color de su dueño con chispitas (crema si está libre) que se hunde al pintarse y suelta chispitas; torta de chocolate central con merengues, macarons, vela y chocolate que gotea; el sector sellado lo marca un colador que baja, tamiza azúcar glas y se va; mesa con mantel a cuadros y puntilla, torta de pisos, torres de macarons, cupcakes, donas, galletas, palote, batidor, manga, boles, batidora, taza, huevos y harina; pared de azulejos con estantes de frascos y horno encendido, ventana con cortinas a cuadros que ondean, macetas, jardín con cerco, lomas y sol bajo; lámparas colgantes, migas y confeti sobre el ganador | Rehecho |
| 24 | Defensa Núcleo | Ciudad futurista | Estadio en la azotea de un rascacielos de noche: cancha octogonal de paneles de resina azul (cada mitad teñida por su equipo, franja neutral rayada) que sigue el chaflán sorteado, líneas de luz cian con un pulso que corre por el borde, áreas y líneas límite del color de cada equipo, círculo del barrido del obstáculo; vallas con banda de LEDs, plataforma giratoria bajo la barra (cápsula del radio exacto, bandas rosas, puntas emisoras); arcos de energía con postes del radio de su colisión, arco luminoso, cortina con barrido, bolsillo con rejilla y receptor que absorbe el núcleo tras el gol; núcleo dorado con anillos de contención, estela, halo y charco de luz (naranja al cargar); grada con cinco filas de hinchas por color de equipo que saltan y levantan los brazos, barritas de luz y cinta de LEDs, cercha de focos y pantallas gigantes con el marcador; bancos de suplentes con techo de vidrio, bandera y conservadora, aires acondicionados con ventilador, celdas de energía, cámara de TV, plataforma y drones cámara, torres de iluminación con cono y charco, pilones, balizas, antena y tanque; barandas de vidrio al borde de la azotea; ciudad de torres con ventanas, letreros de neón en las terrazas, antenas con balizas y autos voladores en carriles, en dos profundidades de bruma violeta; cielo con luna, estrellas, nubes y reflectores que barren; entrada de cámara desde el horizonte, chispas en cada toque, estallido en el golpe cargado, «¡GOL!» sobre el arco con papelitos, luces que laten con el color del equipo y fuegos artificiales | Rehecho |
| 25 | Lluvia Apilada | Observatorio | Cima nevada del observatorio de noche: la arena es la azotea de losas a soga con rosa celeste de bronce embutida, muros de sillares con albardilla, balaustrada calada al frente (deja ver la última fila) y pilastras con faroles que dejan charcos tibios; fachada con ventanas encendidas; bases de piedra con anillo y estrella del color de cada jugador, mojones con luz, columna de luz y «+N» al asegurar, y un frasco de estrellas sobre el muro que se llena con lo asegurado; obstáculos vestidos sobre su caja exacta (columnas con remate de bronce, pedestal con telescopio que barre el cielo, esferas armilares que giran, globo celeste, obeliscos, consolas con carta celeste y diales, cajas de instrumental); lo que cae son fragmentos de cristal cian, estrellas doradas y meteoros con grietas al rojo, todos con estela de estrella fugaz y blanco en el piso que crece (cruz roja para el meteoro, estrella para la dorada); esquirlas al romperse, quemaduras de meteoro, pila que se tambalea tras un golpe; afuera, cúpula grande con ranura y telescopio que sigue una estrella, radiotelescopio que gira, telescopio en trípode, mástil meteorológico con anemómetro, farolas, baranda al precipicio, pinos y rocas nevadas; cima con parches de nieve y acantilados que se hunden en un mar de nubes; dos cordilleras con un observatorio vecino y su baliza, Vía Láctea, luna, estrellas que titilan y estrellas fugaces; escarcha que vuela; cámara que baja en la cuenta y confeti sobre el ganador | Rehecho |
| 26 | Cuerda Acantilado | Acantilado, faro y mar | Dos acantilados con faro sobre el mar al atardecer: rocas estratificadas, olas que rompen, gaviotas, pasto y flores | Rehecho |
| 27 | Último Asiento | Carrusel de feria | Feria de día: carrusel con caballitos y tazas, noria, puestos, guirnaldas de bombillas, globos | Rehecho |
| 28 | Cajas del Puerto | Puerto y grúa | Muelle | Rehecho |
| 29 | Laberinto Jade | Templo antiguo de jade | Cámara interior de un templo de jade: losas talladas, altar, columnas con relieves, antorchas, estatuas | Rehecho |
| 30 | Veta de Cristal | Mina y geodas | Mina de cristales azules y violetas (sin lava, para separarla de Vagonetas): vigas, lámparas, rieles, geodas | Rehecho |
| 31 | Cápsulas Barajadas | Laboratorio | Laboratorio | Rehecho |
| 32 | Bateo Meteórico | Espacio y meteoritos | Plataforma en órbita: planeta anillado, cúpula con telescopio, lluvia de meteoritos, estrellas | Rehecho |
| 33 | Racimo Tóxico | Árbol y cabañas en el pantano | Pantano de noche: árbol gigante con racimo, cabañas sobre pilotes, agua con nenúfares, luciérnagas, niebla verde | Rehecho |
| 34 | Tesorero Cercado | Patio de castillo con rejas | Patio de castillo medieval: murallas con almenas, torres, estandartes, antorchas, foso, carros y barriles | Rehecho |
| 35 | Descenso en Nubes | Cielo con nubes | Cielo de tarde con islas flotantes: nubes en capas, globos, molinos, aves, arcoíris, isla diana | Rehecho |
| 36 | Volea de Magma | Cancha volcánica | Cancha de obsidiana junto a un lago de lava: volcanes, columnas de basalto, red de cadenas, brasas | Rehecho |
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
