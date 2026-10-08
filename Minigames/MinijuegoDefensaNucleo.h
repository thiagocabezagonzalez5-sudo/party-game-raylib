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
