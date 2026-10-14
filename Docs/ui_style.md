# Lenguaje visual de la UI: dos direcciones

Estado: **propuesta, pendiente de elección**. No se ha tocado código del juego.

## 1. Punto de partida (capturas reales)

Capturas en `docs/img/ui_actual_*.png` (1920x1080).

- **Mundo del juego:** isla low-poly de atardecer: cielo degradado violeta a naranja,
  sol bajo, mar azul, nubes rosas, banderines de colores, madera, engranajes, un
  dirigible y un barco. Colores planos, sin texturas, formas simples.
- **HUB (menú principal):** ya es diegético: se elige modo apuntando a objetos 3D
  (dirigible = partida, barco = minijuegos). Es lo mejor del juego actual.
- **Resto de pantallas:** paneles rectangulares azul marino con borde fino
  (naranja = foco, rojo = foco en el catálogo, verde = foco en rondas), botones
  planos, toggles tipo iOS, sliders y flechas `<` `>`. Se sienten genéricas y
  **desconectadas del HUB**.
- **Tipografía:** fuente pixel de raylib (por defecto) en todos lados; funciona, pero
  los títulos no tienen personalidad. Texto de ayuda pequeño y en mayúsculas.
- **Inconsistencias:** cada pantalla elige su propio color de foco; tamaños y
  espaciados se calculan por pantalla (`Px()`, `EscalaUI()` locales).

Objetivo: que menús, configuración, catálogo, HUD y tableros parezcan **objetos del
mismo mundo** y no una capa de formularios encima.

Reglas comunes a ambas direcciones:

- Todo con primitivas 2D de raylib (rectángulos, círculos, líneas, anillos). Cero
  texturas, cero dependencias nuevas, originales (nada de Nintendo).
- Teclado y mando con el mismo modelo: un único **foco** + acciones
  (arriba/abajo/izq/der, aceptar, volver, pestaña anterior/siguiente). El mouse es
  opcional.
- Tokens únicos: paleta, tamaños en unidades relativas a 720p, duraciones.
- Respetar `reducirMovimiento` de la configuración: sin balanceo ni sacudidas.
- Contraste mínimo del texto 4.5:1 sobre su fondo.
- Textos de UI en español rioplatense con voseo (elegí, confirmá, volvé); nunca tuteo.
  En mayúsculas se escriben las tildes de Á, É, Ó, Ú («CONFIRMÁ», «VOLVÉ»), pero nunca
  la Í mayúscula, porque la fuente pixel la dibuja como una í minúscula («ELEGI»).

---

## Dirección A: «Carteles de playa» (madera y cuerda)

Imagen: `docs/img/ui_direccion_A.png`

Idea: la UI es **utilería de la isla**: tablones, carteles colgados de sogas, un
timón, palancas. Continúa lo que ya hace el HUB.

**Paleta**

| Rol | Color |
|---|---|
| Madera base / tablón / cartel | `#8C5C40` / `#AA704E` / `#A66C4C` |
| Foco (madera clara) + marco | `#DE9660` + `#FFD65A` |
| Cuerda | `#D2B478` |
| Texto sobre madera | `#FFF0D2` (sombra `#000` 35 %) |
| Acento / título | `#FFE28C` |
| Positivo / alerta / peligro | `#5ACB8C` / `#FFC446` / `#E65046` |

**Tipografía:** fuente pixel actual. Títulos grandes en amarillo con sombra dura de 2 px,
como letras pintadas. Texto de cuerpo en crema.

**Formas:** rectángulos con clavos en las esquinas, vetas de líneas finas, sombra
proyectada desplazada (+4, +6 px). Nada de esquinas muy redondeadas.

**Componentes**

- **Panel:** tablón grande colgado de dos sogas; entra cayendo desde arriba y se
  asienta con rebote.
- **Botón:** cartel colgado de dos sogas. En reposo cuelga con un ángulo propio
  (±3°); con foco se ilumina, se agrega marco amarillo y se balancea con un péndulo
  amortiguado; al aceptar se «hunde» 4 px.
- **Slider:** palanca con perilla roja sobre una regla con marcas; el relleno es
  amarillo.
- **Selector de opción:** cartel con `< valor >`; las flechas se «golpean» al cambiar.
- **Dial / toggle:** timón que gira al cambiar; el toggle es una palanca de dos
  posiciones.
- **Pestañas:** carteles colgados a distinta altura; el activo queda más abajo.

**Animación:** muelle amortiguado (frecuencia ~6 Hz, amortiguación 0,45) para
balanceo y entrada; transición de pantalla: los carteles caen y se recogen.
**Sonido de UI:** toc de madera (mover), golpe de martillo (aceptar), cuerda que se
tensa (volver), campanita (confirmar de partida).

**Pros:** coherente con el mundo; mucha personalidad; fácil de leer.
**Contras:** más trabajo de dibujo por componente; el balanceo constante puede
cansar, por eso solo se mueve el elemento con foco.

---

## Dirección B: «Álbum de pegatinas» (cartulina, cinta y sellos)

Imagen: `docs/img/ui_direccion_B.png`

Idea: la UI es **papel sobre la mesa**: cartas inclinadas, cinta adhesiva,
perillas de radio y sellos de goma. Más ligera y juguetona, no depende de la
isla.

**Paleta**

| Rol | Color |
|---|---|
| Cartulina / carta blanca | `#FAEED2` / `#FFFFFA` |
| Tinta (texto y bordes) | `#3C2846` |
| Foco (amarillo) | `#FFBE5A` |
| Perillas / acento | `#FF7858` |
| Positivo / peligro | `#5AC88C` / `#DC3C3C` |
| Cintas | `#78C8E6`, `#FFC85A` |

**Tipografía:** fuente pixel actual en tinta oscura sobre papel; títulos en amarillo
con sombra, sobre el mundo.

**Formas:** rectángulos con borde de tinta de 2 px, sombra desplazada, inclinación
leve por elemento (±2°), cintas semitransparentes en las esquinas.

**Componentes**

- **Panel:** carta grande inclinada -1° con dos cintas; entra deslizándose y
  enderezándose.
- **Botón:** carta blanca inclinada; con foco se levanta 14 px, se pone amarilla y se
  endereza hacia el cursor; al aceptar, un **sello** rojo cae sobre ella.
- **Slider:** perilla giratoria con marcas (270°); con teclado/mando, izq/der gira la
  perilla paso a paso.
- **Selector de opción:** carta con `‹ valor ›`.
- **Toggle:** interruptor verde/gris tipo pegatina.
- **Pestañas:** fichas de archivador; la activa sube.

**Animación:** resorte con inclinación (la carta rota al moverse y se asienta en
~0,25 s); transición de pantalla: pasar página.
**Sonido de UI:** papel (mover), clic de perilla (cambiar valor), sello de goma
(aceptar), cinta que se despega (volver).

**Pros:** barata de dibujar, muy legible, buen contraste; funciona igual sobre
cualquier fondo 3D.
**Contras:** menos ligada al mundo de la isla; riesgo de verse «de papelería» si no se
cuida la paleta.

---

## Comparación rápida

| | A. Carteles de playa | B. Álbum de pegatinas |
|---|---|---|
| Conexión con el HUB 3D | Alta | Media |
| Legibilidad | Buena (texto claro sobre madera) | Muy buena (tinta sobre papel) |
| Costo de implementación | Medio | Bajo-medio |
| Movimiento | Péndulos y sogas | Inclinaciones y sellos |
| Riesgo | Cansancio por balanceo | Aspecto genérico |

Recomendación: **A** si se prioriza identidad y coherencia con el mundo; **B** si se
prioriza claridad y rapidez de implementación.

## Qué sigue (tras la elección)

Módulo compartido mínimo `UI/Estilo*` (tokens + componentes reutilizables), una
pantalla de demo con todos los componentes capturada en 1280x720 y 1920x1080, y
documentación de la API. No se tocará lógica de juego ni pantallas concretas.

---

# Direccion elegida: A, «Carteles de playa» (implementada)

Ajustes pedidos e implementados:

1. **Balanceo solo al aparecer y al recibir foco.** En reposo todo queda quieto
   (cada cartel mantiene un angulo fijo de ±1,5°). El pendulo se amortigua en ~1 s.
2. **Se puede desactivar.** `EstiloUI::reducirMovimiento = true` elimina la caida
   de entrada y todos los balanceos (los elementos aparecen y se quedan en su sitio).
   Conviene enlazarlo con la opcion existente `reducirMovimiento` de
   `ConfiguracionJuego`. La demo lo muestra en la pestana JUEGO («Reducir movimiento»).
3. **Texto solo sobre zonas claras.** Todo el texto va en tinta oscura
   (`UI_TINTA`, contraste ~9:1) sobre la placa clara; la madera oscura solo es marco.
   Las placas son de color plano, sin veta. Las unicas excepciones son los titulos
   sobre el mundo (`DibujarTextoMundo`), con sombra dura.
4. **Madera y cuerda como recursos 9-slice reutilizables.** Se generan una vez
   por codigo (`CargarEstiloUI`) y se dibujan con `DibujarNueveCortes`. Ninguna
   pantalla dibuja tablones a mano.
5. **Un color de foco y un sistema de tamanos.** `UI_FOCO` = dorado del HUB
   (`COLOR_ACENTO` de `MenuPrincipal.cpp`, 255/200/70). Tamanos en unidades de
   1280x720 con `EscalaUI()` = min(ancho/1280, alto/720); tamanos de texto del HUB
   (20, 30 con foco, 54 titulo) como tokens.

No incluido en esta etapa: el dial/timon (las palancas cubren los valores
continuos) y el sonido de UI (solo se documenta; lo reproduce cada pantalla con
el audio existente).

## Como ejecutar la demo

```
powershell -File Tests/DemoEstiloUI.ps1                 # compila y abre la demo
powershell -File Tests/DemoEstiloUI.ps1 -SoloCompilar   # solo compila
build/DemoEstiloUI.exe --captura 1920 1080 build/capturas/estiloui_v2
build/DemoEstiloUI.exe --captura 1280 720 build/capturas/estiloui_v2/reducido --reducir-movimiento
```

La demo tiene 5 pestanas: VIDEO (palanca con texto propio «60 FPS»), AUDIO
(palancas 0..100 % por defecto), JUEGO (interruptor «Reducir movimiento»),
CONTROLES (palanca con rango 5..35 %, selector deshabilitado y tarjeta) y EXTRAS
(segunda tanda: textos con contorno, banderines, marco con recorte, parrafo,
sellos, etiquetas y velo; izquierda/derecha cambia la alineacion del parrafo y
aceptar muestra u oculta el velo). VOLVER o Esc abren el aviso «¿Salir sin
aplicar?»; APLICAR abre un aviso con cuenta atras.

## Componentes y API (`UI/EstiloUI.h`)

Todas las posiciones se pasan en **unidades de referencia** (1280x720); el modulo
las convierte con `RectRef`. Los componentes con animacion llevan su estado dentro
del propio struct.

| Componente | Struct | Funciones |
|---|---|---|
| Recursos | `EstiloUI` | `CargarEstiloUI`, `DescargarEstiloUI`, `ActualizarEstiloUI`; campo `reducirMovimiento` |
| Escala | — | `EscalaUI()`, `Px(u)`, `RectRef(x,y,w,h)`, `TamanoTexto(token)` |
| Entrada | `EntradaUI`, `LectorEntradaUI`, `EnfoqueUI` | `LeerEntradaUI(lector)`, `EnfoqueUI::Mover(delta)` |
| Animacion | `ResorteUI`, `ColgadoUI` | `ResorteUI::Actualizar`, `ColgadoUI::Iniciar/Actualizar/Pulsar/EnMovimiento` |
| Panel | `PanelUI` | `Iniciar(rect, retraso)`, `ActualizarPanelUI`, `DibujarPanelUI`, `AreaInteriorPanelUI` |
| Titulo | — | `DibujarTituloUI(estilo, colgado, texto, rect)` |
| Boton / pestana | `BotonUI` | `Iniciar(rect, texto, reposo, retraso)`, `ActualizarBotonUI(b, estilo, dt, foco)`, `DibujarBotonUI(estilo, b, foco)`; `colgado.Pulsar()` al aceptar; `activo` marca la pestana actual. Si el texto no cabe, la fuente se reduce sola (sin cortar) |
| Ancho segun texto | — | `AnchoNecesarioBotonUI(texto, token = UI_TEXTO)`: ancho en unidades de referencia (texto + marco + aire) para dimensionar botones y pestanas |
| Selector `< valor >` | `SelectorUI` | `Iniciar`, `ActualizarSelectorUI`, `GolpearSelectorUI(dir)`, `DibujarSelectorUI(estilo, s, textoValor, foco)`; `habilitado = false`: placa rayada, flechas solo contorneadas, texto en `UI_TINTA_SUAVE` (~4,7:1) y `GolpearSelectorUI` no hace nada. `Iniciar` lo repone a `true` |
| Palanca (slider) | `PalancaUI` | `Iniciar(rect, etiqueta, valor, retraso)`, `ActualizarPalancaUI`, `AjustarPalancaUI(dir)`, `DibujarPalancaUI(estilo, p, foco, textoValor = nullptr)`; `valor` 0..1, `paso`. Valor mostrado: `textoValor` (p. ej. «60 FPS») o `formatoValor` printf sobre `ValorRealPalancaUI` (= `minimo + valor * (maximo - minimo)`, p. ej. `"%.0f %%"` con 5..35) o, por defecto, 0..100 %. `anchoValor` reserva la zona del texto; `RielPalancaUI(p, x0, x1)` da el riel en pixeles. `Iniciar` no toca `minimo/maximo/formatoValor/anchoValor` |
| Interruptor (toggle) | `InterruptorUI` | `Iniciar`, `ActualizarInterruptorUI`, `DibujarInterruptorUI`; campo `activo` |
| Tarjeta (bloque de informacion) | `TarjetaUI` | `Iniciar(rect, titulo o nullptr, retraso)`, `ActualizarTarjetaUI`, `DibujarTarjetaUI`, `AreaContenidoTarjetaUI`, `DibujarLineaTarjetaUI(t, linea, etiqueta, valor)`. Placa clara con marco fino de madera, sin sogas ni foco |
| Modal / aviso | `ModalUI` | `Abrir(rect, mensaje, boton0, boton1, focoInicial)`, `Cerrar()`, `ActualizarModalUI(m, estilo, dt, entrada)` → indice del boton elegido o -1 (al responder se cierra solo), `DibujarModalUI(estilo, m, detalle = nullptr, progreso = -1)`, `BotonModalBajoMouseUI`. Campos: `botonCancelar` (lo ejecuta volver), `largoSogas` (ganchos fijos a esa distancia sobre el cartel; 0 = sin sogas). Velo oscuro; izq/der eligen boton, arriba/abajo alternan; respeta `reducirMovimiento` |
| Ayuda de controles | — | `DibujarAyudaUI(estilo, texto)` |
| Texto ajustado | — | `TamanoTextoAjustadoUI(texto, token, anchoMax)`, `DibujarTextoAjustadoUI(texto, x, y, token, anchoMax, color, centrado)`: reducen la fuente de a 1 px (minimo 10 px) hasta que el texto quepa; nunca lo cortan |
| Bajo nivel | — | `DibujarNueveCortes`, `DibujarCuerdaUI`, `DibujarMarcoFoco`, `DibujarFlechaUI(cx, cy, medio, dir, color)`, `DibujarTextoTinta`, `DibujarTextoMundo` |

**Tokens** (todos en `EstiloUI.h`): colores `UI_FOCO`, `UI_TINTA`, `UI_TINTA_SUAVE`,
`UI_PLACA_CLARA`, `UI_MADERA_OSCURA`, `UI_MADERA_MEDIA`, `UI_CUERDA`, `UI_POSITIVO`,
`UI_PELIGRO`, `UI_TEXTO_MUNDO`; tamanos `UI_TEXTO_PEQUENO/UI_TEXTO/UI_TEXTO_GRANDE/
UI_TEXTO_TITULO`, `UI_RELLENO`, `UI_SEPARACION`, `UI_ALTO_FILA`, `UI_ALTO_BOTON`,
`UI_LARGO_CUERDA`, `UI_BORDE_FOCO`; tiempos `UI_T_CAIDA_ENTRADA`, `UI_T_ESCALONADO`,
`UI_T_PULSACION`, `UI_T_ASENTAR_PALANCA`.

**Uso tipico en una pantalla**

```cpp
EstiloUI estilo;  CargarEstiloUI(estilo);            // una vez
PanelUI panel;    panel.Iniciar(Rectangle{240,100,800,520}, 0.0f);
BotonUI volver;   volver.Iniciar(Rectangle{380,468,220,52}, "VOLVER", -1.2f, 0.8f);

// cada frame
ActualizarPanelUI(panel, estilo, dt);
ActualizarBotonUI(volver, estilo, dt, enfoque.indice == 0);
...
DibujarPanelUI(estilo, panel);
DibujarBotonUI(estilo, volver, enfoque.indice == 0);

// al cerrar
DescargarEstiloUI(estilo);
```

## Segunda tanda de componentes

Solo se agregaron funciones, structs y un token (`UI_VELO`); ninguna firma,
valor por defecto ni aspecto de lo anterior cambio. Ninguno carga recursos
propios: usan las texturas de `EstiloUI` (o ninguna).

**Convencion de unidades.** Las *posiciones* que suelen salir de otro calculo
(un centro obtenido con `GetWorldToScreen`, un rectangulo ya pasado por
`RectRef`) van en **pixeles**; los *tamanos* (radios, grosores, tokens de
texto) van en **unidades de referencia** y escalan con `EscalaUI()`. Los
componentes que ocupan un lugar fijo de la pantalla (banderin, marco) reciben
su rectangulo en unidades de referencia, como el resto del modulo. Para pasar
un punto de referencia a pixeles: `RectRef(x, y, 0, 0)`.

### 1. Texto sobre el mundo con contorno

```cpp
void DibujarTextoMundoContornoUI(const char* texto, float centroX, float y,
    int tokenTexto, Color color, float alfa = 1.0f, Color contorno = UI_TINTA);
```

Como `DibujarTextoMundo` (que no cambia), pero con contorno oscuro y sombra dura
**proporcionales al tamano** (≈ 1/16 y 1/10 de la fuente: 16 px → 1 y 2 px;
54 px → 3 y 5 px). Usarlo para textos que van directo sobre la escena 3D y deben
leerse sobre cualquier fondo (cielo claro, nieve, lava): «¡YA!», «RONDA 3»,
puntos que saltan. `alfa` funde todo junto, pensado para aparecer/desaparecer
(a mitad del fundido el contorno se ve algo mas denso que el texto).

```cpp
DibujarTextoMundoContornoUI("¡YA!", GetScreenWidth() * 0.5f, Px(120), UI_TEXTO_TITULO, UI_FOCO);
```

### 2. Etiqueta compacta rotable

```cpp
struct OpcionesEtiquetaUI { int tokenTexto = UI_TEXTO_PEQUENO; Color borde = UI_MADERA_MEDIA;
                            float alfa = 1.0f; bool pico = false; };
Rectangle DibujarEtiquetaUI(const char* texto, Vector2 centro, float rotacionGrados = 0.0f,
    const OpcionesEtiquetaUI& opciones = OpcionesEtiquetaUI{});
```

Cartelito de placa clara con borde de color (p. ej. el del jugador) y texto en
tinta. `centro` en pixeles; rotacion en grados, sentido horario. `pico = true`
agrega un triangulo abajo que senala el objetivo. Devuelve el rectangulo de la
placa en pixeles **sin rotar** (para apilar etiquetas). Sin foco ni animacion.
Usarlo para rotulos sobre jugadores, fichas o postales.

```cpp
OpcionesEtiquetaUI op;  op.borde = colorJugador;  op.pico = true;
DibujarEtiquetaUI("J1", Vector2{ pantalla.x, pantalla.y - Px(40) }, -5.0f, op);
```

### 3. Parrafo con ajuste de linea

```cpp
enum AlineacionUI { UI_ALINEAR_IZQUIERDA, UI_ALINEAR_CENTRO, UI_ALINEAR_DERECHA };
struct MedidaParrafoUI { float alto; int lineas; int tamano; bool cabe; };
MedidaParrafoUI MedirParrafoUI(const char* texto, int tokenTexto, float anchoMaximo,
    float altoMaximo = 0.0f);
MedidaParrafoUI DibujarParrafoUI(const char* texto, float x, float y, int tokenTexto,
    float anchoMaximo, Color color, AlineacionUI alineacion = UI_ALINEAR_IZQUIERDA,
    float altoMaximo = 0.0f);
```

Parte el texto por palabras (y en cada `\n`) para que quepa en `anchoMaximo`
pixeles; una palabra mas larga que el ancho queda sola en su linea (nunca se
corta). Con `altoMaximo > 0` la fuente baja de a 1 px (minimo 10) hasta que el
bloque entre; si ni asi entra, `cabe = false` y se dibuja igual a 10 px. El
interlineado es proporcional a la fuente. Devuelve el alto usado (primera a
ultima linea), las lineas y la fuente final. `MedirParrafoUI` mide sin dibujar,
para reservar espacio. Usarlo para descripciones de minijuegos, reglas y
avisos largos (en lugar de varias `DrawText`).

```cpp
Rectangle caja = RectRef(560, 260, 260, 120);
MedidaParrafoUI m = DibujarParrafoUI(descripcion, caja.x, caja.y, UI_TEXTO,
    caja.width, UI_TINTA, UI_ALINEAR_CENTRO, caja.height);
float ySiguiente = caja.y + m.alto + Px(UI_SEPARACION);
```

### 4. Sello / insignia

```cpp
enum FormaSelloUI { UI_SELLO_CIRCULO, UI_SELLO_ESTRELLA };
void DibujarSelloUI(const char* texto, Vector2 centro, float radioRef,
    FormaSelloUI forma = UI_SELLO_CIRCULO, Color color = UI_PELIGRO,
    float rotacionGrados = 0.0f, float alfa = 1.0f);
```

Sello de goma: forma de color (circulo o estrella de 12 puntas) con contorno,
disco claro al centro y el texto en tinta; la fuente se elige segun el radio y
se reduce hasta entrar en el disco. `centro` en pixeles, `radioRef` en unidades
de referencia. Para textos muy cortos: «x2», «NUEVO», «1º», «+5».

```cpp
DibujarSelloUI("x2", Vector2{ r.x + r.width, r.y }, 30.0f, UI_SELLO_ESTRELLA, UI_FOCO, -10.0f);
```

### 5. Velo de pantalla completa

```cpp
inline constexpr Color UI_VELO = { 10, 12, 18, 255 };
struct VeloUI { bool visible = false; float aparicion = 0.0f; float opacidad = 0.55f;
                float duracion = 0.18f; Color color = UI_VELO; void Iniciar(bool visibleInicial); };
void ActualizarVeloUI(VeloUI& velo, const EstiloUI& estilo, float dt);
void DibujarVeloUI(const VeloUI& velo);
void DibujarVeloUI(float alfa, Color color = UI_VELO);
```

Oscurece toda la pantalla para dejar la escena en segundo plano detras de una
pausa, un resultado o un aviso propio. Se cambia `visible` y `ActualizarVeloUI`
anima `aparicion` en `duracion` segundos; con `reducirMovimiento` aparece y
desaparece al instante. `Iniciar` fija el estado sin fundido. La version sin
estado dibuja el rectangulo con ese alfa. Mismo color y opacidad que el velo
de `ModalUI` (que sigue dibujando el suyo: no hace falta otro debajo).

```cpp
velo.visible = enPausa;                 // cada frame
ActualizarVeloUI(velo, estilo, dt);
... escena ...
DibujarVeloUI(velo);                    // despues de la escena, antes del menu de pausa
```

### 6. Banderin con mastil

```cpp
void DibujarBanderinUI(const EstiloUI& estilo, const char* texto, Rectangle refBandera,
    float altoMastilRef, Color color = UI_PELIGRO, bool ondear = true, float fase = 0.0f);
```

Mastil de madera con perilla dorada y una bandera de color con cola en V; el
texto va en tinta sobre una franja clara de la tela. `refBandera` es la tela
(unidades de referencia); el mastil va pegado a su borde izquierdo y mide
`altoMastilRef` desde el borde superior hacia abajo. Ondea con `estilo.tiempo`
(requiere `ActualizarEstiloUI`); con `ondear = false` o `reducirMovimiento`
queda quieta. `fase` desfasa banderines vecinos. Usarlo para marcar salida/meta,
equipos o la casilla de un evento.

```cpp
DibujarBanderinUI(estilo, "META", Rectangle{ 1080, 268, 150, 58 }, 330.0f, UI_POSITIVO, true, 1.7f);
```

### 7. Marco de ilustracion con recorte

```cpp
Rectangle ComenzarMarcoIlustracionUI(const EstiloUI& estilo, Rectangle ref,
    float grosorMarcoRef = 10.0f, Color fondo = UI_PLACA_CLARA);
void TerminarMarcoIlustracionUI(const EstiloUI& estilo, Rectangle ref, float grosorMarcoRef = 10.0f);
Rectangle AreaMarcoIlustracionUI(Rectangle ref, float grosorMarcoRef = 10.0f);
```

Marco de madera tipo cartel cuyo interior recorta lo que se dibuje entre
`Comenzar` y `Terminar` (con `BeginScissorMode` sobre el area interior en
pixeles enteros, ya escalada). `Comenzar` dibuja sombra, marco y fondo (alfa 0 =
sin fondo) y devuelve el area interior en pixeles; `Terminar` cierra el recorte
y dibuja el canto interior encima del contenido. Usarlo para vistas previas de
minijuegos, retratos o mapas. Limitaciones: no se anida, no rota, llamar ambas
con el mismo `ref` y grosor, y al terminar el recorte queda desactivado (si la
pantalla tenia uno propio, debe volver a activarlo).

```cpp
Rectangle area = ComenzarMarcoIlustracionUI(estilo, Rectangle{ 282, 222, 250, 172 });
DrawTexturePro(vistaPrevia, origen, area, Vector2{}, 0.0f, WHITE);  // lo que sobre, no se ve
TerminarMarcoIlustracionUI(estilo, Rectangle{ 282, 222, 250, 172 });
```

## Capturas de la demo

`docs/img/ui_demo_{1280x720,1920x1080}_*.png` (primera version, 3 pestanas): 01 mitad
de caida del panel, 02 rebote, 03 reposo, 04 palanca ajustada, 05 cambio de pestana a
mitad de balanceo, 06 pestana en reposo, 07 boton con foco a mitad de balanceo,
08 boton pulsado, 09 final.

Version 2 (componentes nuevos), en `build/capturas/estiloui_v2/` (no versionadas;
se regeneran con los comandos de arriba): 01 mitad de caida, 02 rebote,
03 reposo con pestanas dimensionadas por su texto, 04 palanca con texto propio
(120 FPS), 05 cambio de pestana a mitad, 06 palancas 0..100 %, 07 pestana CONTROLES
con tarjeta, 08 palanca con rango (17 %) y selector deshabilitado con foco,
09 entrada del aviso, 10 aviso en reposo, 11 foco movido a SALIR a mitad de
balanceo, 12 aviso cancelado con volver, 13 aviso con cuenta atras. La carpeta
`reducido/` repite la secuencia con `reducirMovimiento` activado.

Segunda tanda, en `build/capturas/estiloui_tanda2/` (y `reducido/`), con la
misma secuencia mas: 14 pestana EXTRAS en reposo, 15 parrafo centrado y
banderines ondeando, 16 velo a mitad del fundido, 17 velo completo. Se
regeneran con `build/DemoEstiloUI.exe --captura 1280 720 build/capturas/estiloui_tanda2`
(y 1920 1080, y `--reducir-movimiento` hacia `reducido/`).
