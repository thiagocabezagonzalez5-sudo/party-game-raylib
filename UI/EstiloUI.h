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
    ColgadoUI colgado;

    void Iniciar(Rectangle rectRef, const char* textoEtiqueta, float retraso);
};

void ActualizarSelectorUI(
    SelectorUI& selector,
    const EstiloUI& estilo,
    float dt
);

// direccion: -1 izquierda, +1 derecha.
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

void DibujarPalancaUI(
    const EstiloUI& estilo,
    const PalancaUI& palanca,
    bool foco
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


// ---- Ayuda de controles: tira clara al pie de la pantalla ----

void DibujarAyudaUI(const EstiloUI& estilo, const char* texto);
