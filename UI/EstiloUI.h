#pragma once

#include "raylib.h"


//==================================================
// ESTILO UI: "CARTELES DE PLAYA"
//==================================================
//
// Modulo compartido de interfaz (ver docs/ui_style.md). Todo se dibuja
// con madera y cuerda que se GENERAN UNA VEZ por codigo como texturas
// (sin archivos) y se dibujan con 9-slice: las pantallas no dibujan
// tablones a mano, solo usan estos componentes.
//
// Reglas del estilo:
//  - El texto SIEMPRE va en tinta oscura sobre una placa clara.
//  - Un solo color de foco: UI_FOCO (el dorado del HUB).
//  - Un solo sistema de tamanos: unidades de referencia de 1280x720
//    (ver RectRef / Px); todo escala con EscalaUI().
//  - El balanceo ocurre solo al aparecer y al recibir foco; en reposo
//    todo queda quieto. EstiloUI::reducirMovimiento lo desactiva por
//    completo (tambien la caida de entrada).
//==================================================


//--------------------------------------------------
// TOKENS DE COLOR
//--------------------------------------------------

inline constexpr Color UI_FOCO =
    { 255, 200, 70, 255 };              // dorado del HUB (COLOR_ACENTO)

inline constexpr Color UI_TINTA =
    { 58, 36, 24, 255 };                // texto sobre placa clara

inline constexpr Color UI_TINTA_SUAVE =
    { 112, 82, 58, 255 };               // texto secundario sobre placa clara

inline constexpr Color UI_PLACA_CLARA =
    { 232, 207, 160, 255 };

inline constexpr Color UI_MADERA_OSCURA =
    { 126, 82, 56, 255 };

inline constexpr Color UI_MADERA_MEDIA =
    { 170, 112, 78, 255 };

inline constexpr Color UI_CUERDA =
    { 210, 180, 120, 255 };

inline constexpr Color UI_POSITIVO =
    { 90, 190, 130, 255 };

inline constexpr Color UI_PELIGRO =
    { 214, 78, 66, 255 };

// Texto que va directo sobre el mundo (titulos, ayudas). Se dibuja con
// sombra dura de 2 px.
inline constexpr Color UI_TEXTO_MUNDO =
    { 255, 240, 210, 255 };


//--------------------------------------------------
// TOKENS DE TAMANO (unidades de referencia a 1280x720)
//--------------------------------------------------

inline constexpr float UI_REF_ANCHO = 1280.0f;
inline constexpr float UI_REF_ALTO = 720.0f;

inline constexpr int UI_TEXTO_PEQUENO = 16;
inline constexpr int UI_TEXTO = 20;
inline constexpr int UI_TEXTO_GRANDE = 30;      // elemento con foco en el HUB
inline constexpr int UI_TEXTO_TITULO = 54;      // titulo del HUB

inline constexpr float UI_RELLENO = 12.0f;
inline constexpr float UI_SEPARACION = 10.0f;
inline constexpr float UI_ALTO_FILA = 48.0f;
inline constexpr float UI_ALTO_BOTON = 52.0f;
inline constexpr float UI_LARGO_CUERDA = 26.0f;
inline constexpr float UI_BORDE_FOCO = 3.0f;


//--------------------------------------------------
// TOKENS DE TIEMPO (segundos)
//--------------------------------------------------

inline constexpr float UI_T_CAIDA_ENTRADA = 0.55f;
inline constexpr float UI_T_ESCALONADO = 0.07f;   // retraso entre elementos
inline constexpr float UI_T_PULSACION = 0.14f;
inline constexpr float UI_T_ASENTAR_PALANCA = 0.10f;


//==================================================
// ESTILO (recursos compartidos)
//==================================================

struct EstiloUI
{
    // Texturas 9-slice y tile de cuerda (generadas por codigo).
    Texture2D madera = {};      // marco oscuro de paneles
    Texture2D placa = {};       // zona clara donde va el texto
    Texture2D cartel = {};      // boton: marco de madera + cara clara
    Texture2D cuerda = {};      // tile vertical que se repite

    bool cargado = false;

    // Desactiva balanceos y caidas de entrada (accesibilidad).
    bool reducirMovimiento = false;

    float tiempo = 0.0f;
};

// Genera las texturas. Seguro llamar varias veces; requiere ventana abierta.
void CargarEstiloUI(EstiloUI& estilo);

// Descarga simetrica de CargarEstiloUI.
void DescargarEstiloUI(EstiloUI& estilo);

void ActualizarEstiloUI(EstiloUI& estilo, float deltaTime);


//==================================================
// ESCALA Y LAYOUT
//==================================================

// Escala unica: min(ancho / 1280, alto / 720).
float EscalaUI();

// Unidades de referencia -> pixeles.
float Px(float unidades);

// Rectangulo en unidades de referencia -> pixeles, centrando el area
// 16:9 en pantallas de otra proporcion.
Rectangle RectRef(float x, float y, float ancho, float alto);

// Tamano de fuente en pixeles para un token (UI_TEXTO, etc.).
int TamanoTexto(int tokenTexto);


//==================================================
// ENTRADA (teclado + mando, un solo modelo)
//==================================================

struct EntradaUI
{
    bool arriba = false;
    bool abajo = false;
    bool izquierda = false;
    bool derecha = false;
    bool aceptar = false;
    bool volver = false;
    bool pestanaAnterior = false;    // Q / LB
    bool pestanaSiguiente = false;   // E / RB
};

// Lee teclado (flechas/WASD, Enter/Espacio, Esc, Q/E) y los mandos 0..3
// (cruceta, stick izquierdo con flanco, A, B, LB/RB). Las pantallas que
// ya tienen su lector (LectorEntradaFlujo) pueden rellenar EntradaUI
// directamente en lugar de usar esta funcion.
struct LectorEntradaUI
{
    bool stickPrevio[4][4] = {};
};

EntradaUI LeerEntradaUI(LectorEntradaUI& lector);

// Indice con foco dentro de una lista vertical de elementos.
struct EnfoqueUI
{
    int indice = 0;
    int cantidad = 0;

    void Mover(int delta);
};


//==================================================
// ANIMACION
//==================================================

// Muelle amortiguado (1 dimension).
struct ResorteUI
{
    float valor = 0.0f;
    float velocidad = 0.0f;

    void Actualizar(
        float objetivo,
        float deltaTime,
        float rigidez,
        float amortiguacion
    );
};

// Cuerpo colgado de una cuerda: cae al aparecer (con retraso) y se
// balancea como pendulo SOLO al aparecer y al recibir foco. Despues
// vuelve al angulo de reposo y queda quieto.
struct ColgadoUI
{
    float angulo = 0.0f;        // grados respecto de la vertical
    float velocidad = 0.0f;     // grados / s
    float reposo = 0.0f;        // angulo fijo en reposo (+-1,5 deg)

    ResorteUI caida;            // desplazamiento vertical (unidades ref)
    float retraso = 0.0f;       // segundos hasta empezar a caer
    bool aterrizo = false;

    bool focoPrevio = false;
    float hundimiento = 0.0f;   // 1 = recien pulsado, baja a 0
    float signo = 1.0f;         // alterna el lado del balanceo
    float intensidad = 1.0f;    // 0 = sin balanceo (solo cae), 1 = normal
    float distanciaCaida = 170.0f;  // unidades de referencia

    void Iniciar(
        float reposoGrados,
        float retrasoEntrada,
        float distanciaCaidaRef = 170.0f,
        float intensidadBalanceo = 1.0f
    );

    void Actualizar(
        const EstiloUI& estilo,
        float deltaTime,
        bool foco
    );

    // Animacion de pulsacion (el elemento se hunde un instante).
    void Pulsar();

    bool EnMovimiento() const;
};


//==================================================
// DIBUJO BASE (reutilizable por los componentes)
//==================================================

// 9-slice: dibuja la textura en destino con bordes de `borde` pixeles
// de textura escalados por `escala`.
void DibujarNueveCortes(
    Texture2D textura,
    int borde,
    Rectangle destino,
    float escala,
    Color tinte
);

// Cuerda estirada entre dos puntos de pantalla.
void DibujarCuerdaUI(
    const EstiloUI& estilo,
    Vector2 a,
    Vector2 b
);

// Texto en tinta oscura (para placas claras).
void DibujarTextoTinta(
    const char* texto,
    float x,
    float y,
    int tokenTexto,
    Color color
);

// Texto sobre el mundo, con sombra dura.
void DibujarTextoMundo(
    const char* texto,
    float centroX,
    float y,
    int tokenTexto,
    Color color
);

// Marco dorado de foco alrededor de un rectangulo en pixeles.
void DibujarMarcoFoco(Rectangle rect);

// Flecha triangular centrada en (cx, cy), en pixeles. `medio` es la mitad
// de su ancho; direccion: -1 izquierda, +1 derecha.
void DibujarFlechaUI(
    float cx,
    float cy,
    float medio,
    int direccion,
    Color color
);


//==================================================
// TEXTO AJUSTADO AL ANCHO
//==================================================

// Tamano de fuente (pixeles) del token, reducido de a 1 px hasta que el
// texto quepa en anchoMaximo pixeles. Nunca baja de 10 px: si ni asi
// cabe, el texto se dibuja entero (nunca se corta).
int TamanoTextoAjustadoUI(
    const char* texto,
    int tokenTexto,
    float anchoMaximo
);

// Como DibujarTextoTinta, pero reduce la fuente si el texto no cabe en
// anchoMaximo pixeles. Con centrado = true, x es el centro del texto.
void DibujarTextoAjustadoUI(
    const char* texto,
    float x,
    float y,
    int tokenTexto,
    float anchoMaximo,
    Color color,
    bool centrado
);

// Ancho en unidades de referencia que necesita un boton o pestana para
// mostrar su texto sin reducirlo (texto + marco de madera + relleno).
float AnchoNecesarioBotonUI(
    const char* texto,
    int tokenTexto = UI_TEXTO
);


//==================================================
// COMPONENTES
//==================================================

// ---- Panel: tablon grande colgado de dos cuerdas ----

struct PanelUI
{
    Rectangle ref = {};         // unidades de referencia
    ColgadoUI colgado;

    void Iniciar(Rectangle rectRef, float retraso);
};

void ActualizarPanelUI(PanelUI& panel, const EstiloUI& estilo, float dt);

void DibujarPanelUI(const EstiloUI& estilo, const PanelUI& panel);

// Zona clara interior, en unidades de referencia (donde va el contenido).
Rectangle AreaInteriorPanelUI(const PanelUI& panel);


// ---- Titulo: cartel con texto ----

void DibujarTituloUI(
    const EstiloUI& estilo,
    const ColgadoUI& colgado,
    const char* texto,
    Rectangle ref
);


// ---- Boton / pestana: cartel colgado ----
// Si el texto no cabe en el cartel, la fuente se reduce sola (sin cortar).
// Para dimensionar el cartel segun su texto: AnchoNecesarioBotonUI.

struct BotonUI
{
    Rectangle ref = {};         // unidades de referencia
    const char* texto = "";
    bool habilitado = true;
    bool activo = false;        // pestana seleccionada (queda mas abajo)
    ColgadoUI colgado;

    void Iniciar(
        Rectangle rectRef,
        const char* textoBoton,
        float reposoGrados,
        float retraso
    );
};

void ActualizarBotonUI(
    BotonUI& boton,
    const EstiloUI& estilo,
    float dt,
    bool foco
);

void DibujarBotonUI(
    const EstiloUI& estilo,
    const BotonUI& boton,
    bool foco
);


// ---- Filas del panel: selector, palanca, interruptor ----
// Son placas claras que aparecen cayendo un poco (escalonadas) y despues
// quedan fijas, sin balanceo. Etiqueta a la izquierda, control a la derecha.

struct SelectorUI
{
    Rectangle ref = {};
    const char* etiqueta = "";
    float golpeIzquierda = 0.0f;    // flecha que "golpea" al cambiar
    float golpeDerecha = 0.0f;

    // false = deshabilitado: placa atenuada, flechas solo contorneadas
    // y GolpearSelectorUI no hace nada. Iniciar lo vuelve a true. La
    // pantalla decide si deja enfocarlo (el marco de foco se dibuja igual).
    bool habilitado = true;

    ColgadoUI colgado;

    void Iniciar(Rectangle rectRef, const char* textoEtiqueta, float retraso);
};

void ActualizarSelectorUI(
    SelectorUI& selector,
    const EstiloUI& estilo,
    float dt
);

// direccion: -1 izquierda, +1 derecha. Ignorado si no esta habilitado.
void GolpearSelectorUI(SelectorUI& selector, int direccion);

void DibujarSelectorUI(
    const EstiloUI& estilo,
    const SelectorUI& selector,
    const char* valorTexto,
    bool foco
);


struct PalancaUI
{
    Rectangle ref = {};
    const char* etiqueta = "";
    float valor = 0.5f;             // 0..1
    float valorMostrado = 0.5f;     // la perilla llega suave
    float paso = 0.05f;

    // Texto del valor (opcional). Por defecto se muestra 0..100 %. Con
    // formatoValor != nullptr se muestra ValorRealPalancaUI() con ese
    // formato printf de UN float, p. ej. "%.0f %%" con minimo 5 y
    // maximo 35 muestra "5 %".."35 %". Iniciar NO toca estos campos.
    float minimo = 0.0f;
    float maximo = 100.0f;
    const char* formatoValor = nullptr;

    // Zona reservada a la derecha para el texto del valor (unidades de
    // referencia). Agrandarla acorta el riel; ver RielPalancaUI.
    float anchoValor = 84.0f;

    ColgadoUI colgado;

    void Iniciar(
        Rectangle rectRef,
        const char* textoEtiqueta,
        float valorInicial,
        float retraso
    );
};

void ActualizarPalancaUI(
    PalancaUI& palanca,
    const EstiloUI& estilo,
    float dt
);

// direccion: -1 / +1.
void AjustarPalancaUI(PalancaUI& palanca, int direccion);

// minimo + valor * (maximo - minimo).
float ValorRealPalancaUI(const PalancaUI& palanca);

// Extremos del riel en pixeles (x0 = valor 0, x1 = valor 1), por ejemplo
// para arrastrar la perilla con el mouse.
void RielPalancaUI(
    const PalancaUI& palanca,
    float& x0,
    float& x1
);

// textoValor (opcional) reemplaza el texto del valor, p. ej. "60 FPS";
// se reduce si no cabe en anchoValor. Prioridad: textoValor,
// formatoValor y por ultimo el 0..100 % por defecto.
void DibujarPalancaUI(
    const EstiloUI& estilo,
    const PalancaUI& palanca,
    bool foco,
    const char* textoValor = nullptr
);


struct InterruptorUI
{
    Rectangle ref = {};
    const char* etiqueta = "";
    bool activo = false;
    float posicion = 0.0f;          // 0..1, animada
    ColgadoUI colgado;

    void Iniciar(
        Rectangle rectRef,
        const char* textoEtiqueta,
        bool valorInicial,
        float retraso
    );
};

void ActualizarInterruptorUI(
    InterruptorUI& interruptor,
    const EstiloUI& estilo,
    float dt
);

void DibujarInterruptorUI(
    const EstiloUI& estilo,
    const InterruptorUI& interruptor,
    bool foco
);


// ---- Tarjeta: bloque de informacion (sin sogas) ----
// Placa clara con marco fino de madera (9-slice) y titulo opcional. Como
// las filas, aparece cayendo un poco (escalonada) y despues queda fija.
// No es interactiva: no tiene foco.

struct TarjetaUI
{
    Rectangle ref = {};             // unidades de referencia
    const char* titulo = nullptr;   // nullptr = sin titulo
    ColgadoUI colgado;

    void Iniciar(Rectangle rectRef, const char* textoTitulo, float retraso);
};

void ActualizarTarjetaUI(
    TarjetaUI& tarjeta,
    const EstiloUI& estilo,
    float dt
);

void DibujarTarjetaUI(const EstiloUI& estilo, const TarjetaUI& tarjeta);

// Zona clara bajo el titulo, en unidades de referencia (sin la caida de
// entrada: sumar colgado.caida.valor si se dibuja a mano).
Rectangle AreaContenidoTarjetaUI(const TarjetaUI& tarjeta);

// Linea "etiqueta   valor" numero `linea` (0, 1, ...) del contenido.
// Etiqueta en tinta suave, valor en tinta; ambos se reducen si no caben.
void DibujarLineaTarjetaUI(
    const TarjetaUI& tarjeta,
    int linea,
    const char* etiqueta,
    const char* valor
);


// ---- Modal / aviso: cartel colgado sobre un velo oscuro ----
// Cuelga de dos ganchos clavados `largoSogas` unidades por encima del
// cartel (sogas cortas: no cruzan el contenido de arriba). Tiene un
// mensaje, un detalle opcional y dos botones con foco propio:
// izquierda/derecha van al boton de ese lado, arriba/abajo alternan,
// aceptar ejecuta el boton enfocado y volver ejecuta botonCancelar. Respeta
// reducirMovimiento (sin caida ni balanceo).

struct ModalUI
{
    Rectangle ref = {};             // cartel, unidades de referencia
    const char* mensaje = "";
    BotonUI botones[2];
    int foco = 0;                   // boton enfocado (0 o 1)
    int botonCancelar = 1;          // el que ejecuta "volver"

    float largoSogas = 22.0f;       // 0 = sin ganchos ni sogas
    bool abierto = false;
    float aparicion = 0.0f;         // 0..1, opacidad del velo
    ColgadoUI colgado;

    // Abre el modal y coloca los botones al pie del cartel, con el
    // ancho que pidan sus textos (sin pasarse del cartel).
    void Abrir(
        Rectangle rectRef,
        const char* textoMensaje,
        const char* textoBoton0,
        const char* textoBoton1,
        int focoInicial = 0
    );

    void Cerrar();
};

// Devuelve el indice del boton elegido (0 o 1) en el frame en que se
// acepta o se vuelve, y -1 si no hubo respuesta. Al responder el modal se
// cierra solo. Con el modal cerrado no hace nada (devuelve -1).
int ActualizarModalUI(
    ModalUI& modal,
    const EstiloUI& estilo,
    float dt,
    const EntradaUI& entrada
);

// detalle (opcional): texto secundario, se parte en lineas. progreso
// (opcional, 0..1): riel de cuenta atras; < 0 no se dibuja.
void DibujarModalUI(
    const EstiloUI& estilo,
    const ModalUI& modal,
    const char* detalle = nullptr,
    float progreso = -1.0f
);

// Boton del modal bajo el puntero del mouse (0 o 1), o -1.
int BotonModalBajoMouseUI(const ModalUI& modal);


// ---- Ayuda de controles: tira clara al pie de la pantalla ----

void DibujarAyudaUI(const EstiloUI& estilo, const char* texto);
