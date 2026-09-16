#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"


enum FaseCargaInestable
{
    FASE_CARGA_PREPARACION = 0,
    FASE_CARGA_ACTIVA,
    FASE_CARGA_EXPLOSION,
    FASE_CARGA_TERMINADO
};


struct EstadoJugadorCargaInestable
{
    bool eliminado = false;
    int posicionFinal = 0;
    int cantidadPases = 0;
    float tiempoDecisionBot = 0.0f;
};


struct MinijuegoCargaInestable
{
    ResultadoMinijuego resultado{};
    EstadoJugadorCargaInestable jugadores[MAX_PARTICIPANTES];

    FaseCargaInestable fase = FASE_CARGA_PREPARACION;

    int portador = -1;
    int portadorExplosion = -1;
    int numeroRonda = 0;

    float tiempoPreparacion = 0.0f;
    float tiempoCarga = 0.0f;
    float duracionCarga = 0.0f;
    float bloqueoPase = 0.0f;
    float tiempoExplosion = 0.0f;
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
