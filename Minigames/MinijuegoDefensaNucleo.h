#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Referencia de reglas: Mario Party 2 - Speed Hockey (2 vs 2).
// Aqui se transforma en una arena tematizada donde cada jugador se mueve
// libremente por su mitad y devuelve un nucleo que rebota y acelera.


enum FaseDefensaNucleo
{
    FASE_DEFENSA_NUCLEO_PREPARACION = 0,
    FASE_DEFENSA_NUCLEO_JUGANDO,
    FASE_DEFENSA_NUCLEO_PAUSA_GOL,
    FASE_DEFENSA_NUCLEO_TERMINADO
};


struct EstadoJugadorDefensaNucleo
{
    int equipo = -1;

    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;

    // Golpe cargado con el boton principal.
    float cooldownGolpe = 0.0f;
    float tiempoGolpeActivo = 0.0f;
    float cooldownContacto = 0.0f;
    bool botonAnterior = false;

    // Inteligencia de bot.
    float tiempoDecisionBot = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;
    bool quiereGolpear = false;
    bool botGolpeaEnEsteContacto = false;
};


struct NucleoDefensaNucleo
{
    float x = 0.0f;
    float z = 0.0f;
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;

    // Tiempo restante de brillo tras un golpe cargado.
    float tiempoCarga = 0.0f;

    bool enJuego = false;
};


// Chispas, confeti y destellos del estadio (solo visuales).
struct ParticulaDefensaNucleo
{
    bool activa = false;
    bool brillo = false;            // se dibuja en la pasada aditiva
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
};


static const int MAX_PARTICULAS_DEFENSA_NUCLEO = 260;
static const int LARGO_ESTELA_DEFENSA_NUCLEO = 12;


struct MinijuegoDefensaNucleo
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr
    EstadoJugadorDefensaNucleo estadosJugadores[MAX_PARTICIPANTES];
    NucleoDefensaNucleo nucleo{};

    Camera3D camara{};
    FaseDefensaNucleo fase = FASE_DEFENSA_NUCLEO_PREPARACION;

    int goles[2] = { 0, 0 };
    int cantidadJugadoresEquipo[2] = { 0, 0 };
    int indiceTema = 0;
    int equipoUltimoGol = -1;
    int equipoGanador = -1;

    bool golDeOro = false;
    bool empate = false;
    bool partidaValida = false;

    float anguloObstaculo = 0.0f;
    float direccionGiroObstaculo = 1.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoOro = 0.0f;
    float tiempoPausaGol = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: anuncios, festejos y particulas. Nada de esto toca la
    // logica ni consume el GetRandomValue de los bots.
    unsigned int semillaVisual = 2654435761u;
    float tiempoAnuncio = -1.0f;        // «¡A JUGAR!» al primer saque
    float tiempoAnuncioOro = -1.0f;     // «¡GOL DE ORO!»
    float tiempoGolVisual = -1.0f;      // desde el ultimo gol
    float festejo = 0.0f;               // euforia de la tribuna (0..1)
    float destelloArco[2] = { 0.0f, 0.0f };   // 0 = arco izquierdo, 1 = derecho
    float destelloObstaculo = 0.0f;
    float destelloGolpe[MAX_PARTICIPANTES] = {};
    float golpeVisto[MAX_PARTICIPANTES] = {};      // tiempoGolpeActivo previo
    float contactoVisto[MAX_PARTICIPANTES] = {};   // cooldownContacto previo
    float destelloSaque = 0.0f;
    int golesVistos[2] = { 0, 0 };
    int equipoGolVisual = -1;
    bool juegoVisto = false;
    bool oroVisto = false;
    bool finVisto = false;
    bool nucleoVisto = false;
    float velocidadVistaX = 0.0f;
    float velocidadVistaZ = 0.0f;
    float cargaVista = 0.0f;
    Vector3 estela[LARGO_ESTELA_DEFENSA_NUCLEO] = {};
    int cantidadEstela = 0;
    float relojEstela = 0.0f;
    ParticulaDefensaNucleo particulas[MAX_PARTICULAS_DEFENSA_NUCLEO];

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

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
