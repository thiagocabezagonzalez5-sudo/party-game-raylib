#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"


inline constexpr int MAX_INTERRUPTORES_CAOS = 5;


enum FaseInterruptoresCaos
{
    FASE_INTERRUPTORES_PREPARACION = 0,
    FASE_INTERRUPTORES_ELECCION,
    FASE_INTERRUPTORES_SEGURO,
    FASE_INTERRUPTORES_EXPLOSION,
    FASE_INTERRUPTORES_TERMINADO
};


struct EstadoJugadorInterruptoresCaos
{
    bool eliminado = false;
    int posicionFinal = 0;
    int turnosSuperados = 0;
    float tiempoDecisionBot = 0.0f;
};


struct MinijuegoInterruptoresCaos
{
    ResultadoMinijuego resultado{};
    EstadoJugadorInterruptoresCaos jugadores[MAX_PARTICIPANTES];

    bool interruptoresUsados[MAX_INTERRUPTORES_CAOS]{};
    int cantidadInterruptores = MAX_INTERRUPTORES_CAOS;
    int interruptorPeligroso = 0;
    int interruptorSeleccionado = 0;

    int jugadorTurno = -1;
    int jugadorExplosion = -1;
    int numeroTurno = 0;

    FaseInterruptoresCaos fase = FASE_INTERRUPTORES_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoTurno = 0.0f;
    float tiempoResolucion = 0.0f;
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
