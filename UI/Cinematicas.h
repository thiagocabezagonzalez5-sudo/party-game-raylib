#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/Participante.h"
#include "Entities/PersonajesBloque.h"
#include "Gameplay/PartidaTablero.h"
#include "UI/EstiloUI.h"

#include "raylib.h"


//==================================================
// CINEMATICAS
//==================================================
//
// Presentaciones breves con camara animada, los personajes de bloques y
// franjas de cine. Todas se pueden omitir (Enter/Espacio/Esc o A/B del
// mando) y respetan "reducir movimiento" (camara quieta, sin temblores).
// No cargan recursos: escena procedural + EstiloUI.
//
// - Apertura: al iniciar el juego, los cuatro personajes llegan al
//   escenario y aparece el titulo (antes del menu).
// - Minijuego: entre la ruleta y el minijuego de una ronda de tablero, el
//   nombre del minijuego y quienes lo juegan.
// - Campeon: al terminar la partida, el ganador festeja en el podio antes
//   de la pantalla de resultados.
// - Tablero: antes de cada partida, un personaje guia presenta el tablero
//   real (historia, trofeo, gimmick y casillas especiales). El codigo es
//   UNO solo para los cinco tableros: cada tablero aporta solamente su
//   guion (personaje guia y lineas de dialogo, ver
//   UI/GuionesCinematicaTablero.cpp).
//==================================================

struct EstadoCinematica
{
    float tiempo = 0.0f;
    float duracion = 1.0f;
    bool terminada = false;
    bool omitida = false;

    LectorEntradaUI lector;
};


// Un participante tal como lo muestra una cinematica.
struct FiguraCinematica
{
    bool activo = false;
    int personaje = 0;
    int numeroJugador = 1;
    bool esBot = false;
    Color color = WHITE;
};


struct CinematicaApertura
{
    static constexpr float DURACION = 7.2f;

    EstadoCinematica estado;

    void Inicializar();
    void Actualizar(float deltaTime);
    void Dibujar() const;
};


struct CinematicaMinijuego
{
    static constexpr float DURACION = 2.8f;

    EstadoCinematica estado;

    IdMinijuego minijuego = MINIJUEGO_COLOR_SEGURO;
    bool duelo = false;

    FiguraCinematica figuras[MAX_PARTICIPANTES];
    int cantidad = 0;

    void Inicializar(
        IdMinijuego id,
        const Participante participantes[],
        bool esDuelo
    );

    void Actualizar(float deltaTime);
    void Dibujar() const;
};


struct CinematicaCampeon
{
    static constexpr float DURACION = 5.6f;

    EstadoCinematica estado;

    // ranking[0] es el ganador; puesto[k] es 1..4 (se repite si empatan).
    FiguraCinematica figuras[MAX_PARTICIPANTES];
    int puesto[MAX_PARTICIPANTES] = {};
    int cantidad = 0;
    bool empateEnPrimero = false;

    void Inicializar(
        const int ranking[],
        const int puestos[],
        int cantidadJugadores,
        const Participante participantes[]
    );

    void Actualizar(float deltaTime);
    void Dibujar() const;
};


//==================================================
// PRESENTACION DEL TABLERO
//==================================================

// Que encuadra cada linea del guion. La camara, la posicion del guia y la
// casilla resaltada se calculan con el tablero real al inicializar.
enum TipoPlanoCinematica
{
    PLANO_PANORAMA = 0,     // vista general del tablero (titulo)
    PLANO_GUIA,             // primer plano del guia donde este parado
    PLANO_CASILLA,          // una casilla del tipo indicado, el guia al lado
    PLANO_TROFEO,           // la casilla actual del trofeo
    PLANO_LANDMARK,         // el landmark del tablero (DefinicionTablero)
    PLANO_DESPEDIDA         // el guia festeja y la camara se abre al tablero
};


struct LineaGuionTablero
{
    TipoPlanoCinematica plano = PLANO_GUIA;
    TipoCasilla casilla = CASILLA_NEUTRA;    // solo PLANO_CASILLA
    GestoPersonajeBloque gesto = GESTO_NINGUNO;
    const char* texto = nullptr;             // nullptr = fin del guion
};


inline constexpr int MAX_LINEAS_GUION_TABLERO = 14;


// Datos de un tablero: quien presenta y que dice. Nada mas.
struct GuionCinematicaTablero
{
    int guia = PERSONAJE_PIXI;
    LineaGuionTablero lineas[MAX_LINEAS_GUION_TABLERO];
};


const GuionCinematicaTablero& ObtenerGuionCinematicaTablero(
    IdTablero id
);


// Plano ya resuelto sobre el tablero real.
struct PlanoCinematicaTablero
{
    float inicio = 0.0f;
    float duracion = 2.5f;

    Vector3 objetivoInicio{};
    Vector3 objetivoFin{};
    Vector3 posicionInicio{};
    Vector3 posicionFin{};

    Vector3 guia{};           // pies del guia
    float anguloGuia = 0.0f;  // grados (0 = mira a +Z)

    int casilla = -1;         // casilla resaltada (-1 = ninguna)
    bool levantaCasilla = false;
};


struct CinematicaTablero
{
    // Cada cambio de plano: la camara viaja y el guia salta hasta su lugar.
    static constexpr float TRANSICION = 0.7f;
    static constexpr float FUNDIDO_FINAL = 0.45f;

    EstadoCinematica estado;

    // La partida ya preparada (fichas en la salida, trofeo sorteado). Solo
    // se avanza su reloj visual (tiempoTablero) para que el agua, las
    // luces y el gimmick sigan animados; la logica no se toca.
    PartidaTablero* partida = nullptr;
    const GuionCinematicaTablero* guion = nullptr;
    int rondas = 5;

    PlanoCinematicaTablero planos[MAX_LINEAS_GUION_TABLERO];
    int cantidad = 0;

    void Inicializar(PartidaTablero& partidaTablero, int cantidadRondas);
    void Actualizar(float deltaTime);
    void Dibujar() const;

    // Indice del plano que se ve en este instante (para pruebas).
    int PlanoActual() const;
};
