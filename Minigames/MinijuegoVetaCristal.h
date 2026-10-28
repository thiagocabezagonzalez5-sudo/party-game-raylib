#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Referencia de reglas: Mario Party 1 - Coin Block Blitz (2 vs 2).
// Aqui se transforma en una mina: cada equipo golpea con la cabeza geodas de
// cristal que flotan sobre su mitad, recoge las gemas que caen y evita la
// vagoneta que cruza el riel central.


inline constexpr int MAX_BLOQUES_VETA = 12;
inline constexpr int MAX_GEODAS_VETA = 10;
inline constexpr int MAX_GEMAS_VETA = 48;


enum FaseVetaCristal
{
    FASE_VETA_PREPARACION = 0,
    FASE_VETA_JUGANDO,
    FASE_VETA_TERMINADO
};


struct EstadoJugadorVeta
{
    int equipo = -1;
    int gemas = 0;

    // Aturdimiento por la vagoneta e inmunidad posterior.
    float tiempoAturdido = 0.0f;
    float tiempoInmune = 0.0f;

    // Inteligencia de bot.
    float tiempoDecision = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;
    int objetivoGeoda = -1;
    float retardoSalto = -1.0f;
    float esperaGrande = 0.0f;
    float cooldownSaltoBot = 0.0f;
};


struct GeodaVeta
{
    float x = 0.0f;
    float z = 0.0f;
    bool grande = false;

    bool cargada = true;
    float tiempoRecarga = 0.0f;
    float tiempoSacudida = 0.0f;

    // Coordinacion de la geoda grande: ventana tras el primer golpe.
    float ventanaGolpe = 0.0f;
    int golpeadorPrevio = -1;
};


struct GemaVeta
{
    bool activa = false;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float velocidadX = 0.0f;
    float velocidadY = 0.0f;
    float velocidadZ = 0.0f;
    int valor = 1;
    float tiempoSinRecoger = 0.0f;
    float edad = 0.0f;
};


// Solo visual: esquirlas, chispas y confeti (no afectan la partida).
inline constexpr int MAX_CHISPAS_VETA = 180;
inline constexpr int MAX_AVISOS_VETA = 10;


struct ChispaVeta
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 9.0f;
    float giro = 0.0f;
    bool brilla = false;            // halo aditivo en lugar de esquirla
};


// Rotulos que suben sobre el mundo: «+3», «¡TESORO!», «-2».
struct AvisoVeta
{
    bool activo = false;
    Vector3 posicion{};
    float vida = 0.0f;
    int tipo = 0;                   // 0 suma, 1 tesoro, 2 perdida
    int valor = 0;
    int equipo = -1;
};


struct MinijuegoVetaCristal
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorVeta estadosJugadores[MAX_PARTICIPANTES];
    GeodaVeta geodas[MAX_GEODAS_VETA];
    GemaVeta gemas[MAX_GEMAS_VETA];
    BloquePrueba bloques[MAX_BLOQUES_VETA];
    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];
    int cantidadBloques = 0;

    Camera3D camara{};
    FaseVetaCristal fase = FASE_VETA_PREPARACION;

    int cantidadJugadoresEquipo[2] = { 0, 0 };
    int ultimaDoradaEquipo = -1;
    int equipoGanador = -1;
    bool empate = false;
    bool partidaValida = false;

    // Vagoneta del riel central.
    bool vagonetaActiva = false;
    bool avisoVagoneta = false;
    float vagonetaZ = 0.0f;
    float direccionVagoneta = 1.0f;
    float tiempoHastaVagoneta = 0.0f;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Estado solo visual. Lo actualiza Actualizar comparando la partida
    // antes y despues de cada paso; el azar sale de semillaVisual.
    ChispaVeta chispas[MAX_CHISPAS_VETA];
    AvisoVeta avisos[MAX_AVISOS_VETA];
    unsigned int semillaVisual = 1u;
    float destelloGeoda[MAX_GEODAS_VETA] = {};
    float grietaGeoda[MAX_GEODAS_VETA] = {};
    float destelloJugador[MAX_PARTICIPANTES] = {};
    float marcadorVisual[2] = { 0.0f, 0.0f };
    float tiempoAnuncioInicio = -1.0f;
    float tiempoFin = -1.0f;
    bool vagonetaVisible = false;
    float vagonetaVisualZ = 0.0f;
    float vagonetaVisualDireccion = 1.0f;
    float giroRuedas = 0.0f;

    void Inicializar();

    void Reiniciar(
        JugadorPrueba jugadores[],
        Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[]
    );

    // Reglas de la partida (Actualizar la envuelve con el estado visual).
    void ActualizarPartida(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[]
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
