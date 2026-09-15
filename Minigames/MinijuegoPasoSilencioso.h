#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "raylib.h"


enum FasePasoSilencioso
{
    FASE_PASO_PREPARACION = 0,
    FASE_PASO_CARRERA,
    FASE_PASO_TERMINADO
};


struct EstadoJugadorPasoSilencioso
{
    float progreso = 0.0f;
    int penalizaciones = 0;
    bool llegoMeta = false;
    bool castigadoEnAlerta = false;

    bool botAvanzando = false;
    float reaccionBot = 0.0f;
};


struct MinijuegoPasoSilencioso
{
    ResultadoMinijuego resultado{};
    EstadoJugadorPasoSilencioso jugadores[MAX_PARTICIPANTES];

    Camera3D camara{};
    FasePasoSilencioso fase = FASE_PASO_PREPARACION;

    bool centinelaAlerta = false;
    float tiempoPreparacion = 0.0f;
    float tiempoEstadoCentinela = 0.0f;
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
