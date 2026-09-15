#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"


enum FaseCircuitoVoltaje
{
    FASE_CIRCUITO_PREPARACION = 0,
    FASE_CIRCUITO_CARRERA,
    FASE_CIRCUITO_TERMINADO
};


struct EstadoJugadorCircuitoVoltaje
{
    float avance = 0.0f;
    float velocidad = 0.0f;
    float tiempoExcesoCurva = 0.0f;
    float tiempoTrompo = 0.0f;

    int trompos = 0;
    bool llegoMeta = false;
};


struct MinijuegoCircuitoVoltaje
{
    ResultadoMinijuego resultado{};
    EstadoJugadorCircuitoVoltaje jugadores[MAX_PARTICIPANTES];

    FaseCircuitoVoltaje fase = FASE_CIRCUITO_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoCarrera = 0.0f;
    float tiempoAnimacion = 0.0f;

    void Inicializar();

    void Reiniciar(
        Participante participantes[]
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
