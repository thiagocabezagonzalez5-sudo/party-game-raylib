#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "raylib.h"


inline constexpr int MAX_HUELLAS_TRAZO_PERFECTO = 240;


enum FormaTrazoPerfecto
{
    FORMA_TRAZO_CRISTAL = 0,
    FORMA_TRAZO_RAYO,
    FORMA_TRAZO_COCHETE,
    CANTIDAD_FORMAS_TRAZO
};


enum FaseTrazoPerfecto
{
    FASE_TRAZO_PREPARACION = 0,
    FASE_TRAZO_DIBUJANDO,
    FASE_TRAZO_TERMINADO
};


struct EstadoJugadorTrazoPerfecto
{
    Vector2 cursor{};
    Vector2 huellas[MAX_HUELLAS_TRAZO_PERFECTO];

    int cantidadHuellas = 0;
    int puntuacionFinal = 0;

    float precisionAcumulada = 0.0f;
    float tiempoEvaluado = 0.0f;
    float tiempoNuevaHuella = 0.0f;
    float desfaseBot = 0.0f;
    float errorBot = 0.0f;
};


struct MinijuegoTrazoPerfecto
{
    ResultadoMinijuego resultado{};
    EstadoJugadorTrazoPerfecto jugadores[MAX_PARTICIPANTES];

    FaseTrazoPerfecto fase = FASE_TRAZO_PREPARACION;
    FormaTrazoPerfecto forma = FORMA_TRAZO_CRISTAL;

    float tiempoPreparacion = 0.0f;
    float tiempoTrazo = 0.0f;
    float tiempoAnimacion = 0.0f;
    float progresoObjetivo = 0.0f;

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
