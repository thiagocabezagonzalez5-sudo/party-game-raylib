#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


enum AccionTronco
{
    ACCION_TRONCO_TIRAR = 0,
    ACCION_TRONCO_EMPUJAR
};


enum EstadoPartidaTronco
{
    TRONCO_ESPERANDO_JUGADORES = 0,
    TRONCO_PREPARANDO,
    TRONCO_JUGANDO,
    TRONCO_FINALIZADO
};


struct EstadoJugadorTronco
{
    bool respondio = false;
    bool acerto = false;

    float animacionGolpe = 0.0f;
};


struct EstadoEquipoTronco
{
    float progresoCorte = 0.0f;

    float posicionSierra = -0.72f;
    float objetivoSierra = -0.72f;
    int direccionSierra = 1;

    float tiempoCoordinacion = 0.0f;
    float tiempoBloqueo = 0.0f;
    float tiempoFeedback = 0.0f;

    bool golpeEnCurso = false;
    bool ultimoGolpeCorrecto = false;
};


struct MinijuegoTronco
{
    ResultadoMinijuego resultado;

    Camera3D camara{};

    EstadoJugadorTronco estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoEquipoTronco equipos[2];

    int equipoPorJugador[
        MAX_JUGADORES_PRUEBA
    ] = { -1, -1, -1, -1 };

    int ordenEnEquipoPorJugador[
        MAX_JUGADORES_PRUEBA
    ] = { -1, -1, -1, -1 };

    int cantidadJugadoresEquipo[2] =
    {
        0,
        0
    };

    EstadoPartidaTronco estado =
        TRONCO_ESPERANDO_JUGADORES;

    int jugadoresEnPartida = 0;
    int equipoGanador = -1;

    float tiempoPreparacion = 0.0f;
    float tiempoPartida = 0.0f;

    bool partidaValida = false;
    bool empate = false;

    void Inicializar();

    void ConfigurarJugadores(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    ) const;

    void Reiniciar(
        const Participante participantes[],
        int cantidadMaxima
    );

    void PrepararEquipos(
        const Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        int cantidadMaxima,
        const Participante participantes[]
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
