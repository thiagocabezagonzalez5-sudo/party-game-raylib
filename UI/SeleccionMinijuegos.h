#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/Participante.h"
#include "UI/EstiloUI.h"
#include "raylib.h"


//==================================================
// PREVIA COMPARTIDA
//==================================================
//
// Cartel "PRÓXIMO MINIJUEGO" (nombre, descripcion, formato, controles)
// en el que cada humano iza su bandera para decir "listo". Los bots
// quedan listos solos y un mando desconectado no deja la previa
// esperando. La usan el selector libre (SeleccionMinijuegos) y la
// ruleta del tablero (RuletaMinijuegos).
enum ResultadoPreviaMinijuego
{
    PREVIA_ESPERANDO = 0,
    PREVIA_CONFIRMADA,      // todos listos: arranca el minijuego
    PREVIA_VOLVER           // solo si permiteVolver (ESC o J1 sin bandera)
};


struct PreviaMinijuego
{
    IdMinijuego id = MINIJUEGO_COLOR_SEGURO;

    // Selector libre: ESC o cancelar sin bandera vuelven al tablon.
    // Ruleta: no hay a donde volver; cancelar solo baja la bandera.
    bool permiteVolver = true;

    PanelUI panel;

    // Listo e izado (0 abajo, 1 arriba) de cada puesto.
    bool listo[MAX_PARTICIPANTES] = {};
    ResorteUI izado[MAX_PARTICIPANTES];

    float tiempo = 0.0f;
    float tiempoTodosListos = 0.0f;
};

void IniciarPreviaMinijuego(
    PreviaMinijuego& previa,
    IdMinijuego id,
    bool permiteVolver
);

ResultadoPreviaMinijuego ActualizarPreviaMinijuego(
    PreviaMinijuego& previa,
    float deltaTime,
    const Participante participantes[],
    int cantidad
);

// Que partes de una pantalla se dibujan en una pasada. Con todo quieto,
// lo fijo se guarda una vez en una textura (cache) y cada frame solo se
// compone la textura y encima lo vivo. Mientras algo se mueve se dibuja
// todo junto, como siempre.
enum CapaDibujoMinijuegos
{
    CAPA_TODO = 0,
    CAPA_FIJA,
    CAPA_VIVA
};

// Velo, titulo y cartel; la barra de ayuda va con CAPA_TODO y CAPA_VIVA.
void DibujarPreviaMinijuego(
    const PreviaMinijuego& previa,
    const Participante participantes[],
    int cantidad,
    CapaDibujoMinijuegos capa = CAPA_TODO
);

// true si el cartel ya cayo y su velo y titulo terminaron de aparecer.
bool PreviaMinijuegoQuieta(
    const PreviaMinijuego& previa
);


//==================================================
// CACHE DE PANTALLA
//==================================================
//
// Dibuja en `cache` (la crea o la recrea al tamano de la ventana) lo
// que hace `dibujar(datos)`, de modo que ComponerCacheMinijuegos de el
// mismo resultado que dibujarlo directo en pantalla. Devuelve false si
// no se pudo crear la textura. Quien la usa la libera con
// UnloadRenderTexture.
bool DibujarEnCacheMinijuegos(
    RenderTexture2D& cache,
    void (*dibujar)(const void* datos),
    const void* datos
);

void ComponerCacheMinijuegos(
    const RenderTexture2D& cache
);

// Firma de los participantes (activo, bot, conexion y control) para
// las claves de cache: los textos de controles dependen de ellos.
unsigned int FirmaParticipantesMinijuegos(
    const Participante participantes[],
    int cantidad
);


//==================================================
// SELECTOR DEL MODO LIBRE
//==================================================

// Selector de minijuegos del modo libre (HUB > barco), estilo
// "Carteles de playa":
//  - TABLON: el catalogo es un tablon del puerto con una postal por
//    minijuego; J1 elige.
//  - PREVIA: al confirmar cae el cartel del minijuego elegido (ver
//    PreviaMinijuego). Con todos listos se activa `confirmado`.
//
// Juego solo lee `confirmado`, `volver` e `indiceSeleccionado`.
enum VistaSeleccionMinijuegos
{
    VISTA_TABLON_MINIJUEGOS = 0,
    VISTA_PREVIA_MINIJUEGO
};


// Lo que muestra la cache de la pantalla; si algo cambia se redibuja.
struct ClaveCacheSeleccion
{
    int ancho = 0;
    int alto = 0;
    int vista = -1;
    int indice = -1;
    int fila = -1;
    int dyTablon = 0;           // centesimas de unidad de referencia
    int dyPrevia = 0;
    int idPrevia = -1;
    int cantidad = 0;
    unsigned int participantes = 0;
};


struct SeleccionMinijuegos
{
    int indiceSeleccionado = 0;
    int indiceAnterior = -1;

    bool confirmado = false;
    bool volver = false;

    // 0..1: entrada de la ficha del minijuego con foco al cambiarlo.
    float progresoPanel = 0.0f;

    // Primera fila visible de la grilla; permite mas minijuegos que filas.
    int filaInicial = 0;

    // Con mando, la direccion se lee mantenida (cruceta o stick): estos
    // bloqueos hacen que cada empuje mueva el foco un solo paso y se
    // sueltan al volver al centro. El teclado ya dispara por pulsacion.
    bool bloqueoHorizontal = true;
    bool bloqueoVertical = true;

    VistaSeleccionMinijuegos vista = VISTA_TABLON_MINIJUEGOS;

    PanelUI panelTablon;

    // Elevacion de cada postal (0 reposo, 1 con foco), con muelle.
    ResorteUI elevacion[CANTIDAD_MINIJUEGOS];

    // Al desplazar la grilla las postales entran desde ese lado
    // (-1 arriba, +1 abajo); aparicionFilas va de 0 a 1.
    float aparicionFilas = 1.0f;
    int direccionDesplazamiento = 0;

    float tiempo = 0.0f;

    PreviaMinijuego previa;

    // Con todo quieto, lo que no cambia (velo, tablon, postales, ficha
    // y, en la previa, su cartel) se dibuja una vez en esta textura y se
    // reutiliza mientras la clave no cambie: dibujarlo cada frame bajaba
    // el FPS a 1920x1080. Se libera al salir de la pantalla y en
    // Descargar(); se regenera sola al volver o si cambia la ventana.
    mutable RenderTexture2D cacheFija = {};
    mutable bool cacheValida = false;
    mutable ClaveCacheSeleccion cacheClave;

    void Inicializar();

    // Solo J1 (compatibilidad): la previa espera unicamente a J1.
    void Actualizar(
        float deltaTime,
        const Participante& jugadorUno
    );

    // Con todos los participantes: la previa espera a cada humano.
    void Actualizar(
        float deltaTime,
        const Participante participantes[],
        int cantidad
    );

    void Dibujar(
        const Participante& jugadorUno
    ) const;

    void Dibujar(
        const Participante participantes[],
        int cantidad
    ) const;

    // Libera la textura de la cache. Seguro llamar varias veces.
    void Descargar();
};
