#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Input.h"


enum FaseCapitanManda
{
    FASE_CAPITAN_PREPARACION = 0,
    FASE_CAPITAN_MOSTRANDO,
    FASE_CAPITAN_RESPONDIENDO,
    FASE_CAPITAN_RESOLVIENDO,
    FASE_CAPITAN_TERMINADO
};


struct EstadoJugadorCapitanManda
{
    bool eliminado = false;
    bool respondio = false;
    bool acerto = false;

    int posicionFinal = 0;
    int rondasSuperadas = 0;

    float tiempoFeedback = 0.0f;
};


struct MinijuegoCapitanManda
{
    ResultadoMinijuego resultado;

    EstadoJugadorCapitanManda jugadores[
        MAX_PARTICIPANTES
    ];

    FaseCapitanManda fase =
        FASE_CAPITAN_PREPARACION;

    AccionDireccionalControl ordenActual =
        CONTROL_DIRECCION_IZQUIERDA;

    int numeroRonda = 0;

    float tiempoPreparacion = 3.0f;
    float tiempoFase = 0.0f;
    float tiempoRespuesta = 1.55f;

    bool resultadoInicializado = false;

    void Inicializar();

    void Reiniciar(
        const Participante participantes[]
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[]
    );

    void Dibujar(
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
