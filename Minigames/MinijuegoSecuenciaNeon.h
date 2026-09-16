#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"


inline constexpr int MAX_PULSOS_SECUENCIA_NEON = 8;


enum PulsoSecuenciaNeon
{
    PULSO_NEON_ARRIBA = 0,
    PULSO_NEON_DERECHA,
    PULSO_NEON_ABAJO,
    PULSO_NEON_IZQUIERDA,
    CANTIDAD_PULSOS_NEON
};


enum FaseSecuenciaNeon
{
    FASE_SECUENCIA_PREPARACION = 0,
    FASE_SECUENCIA_MOSTRANDO,
    FASE_SECUENCIA_RESPONDIENDO,
    FASE_SECUENCIA_RESOLUCION,
    FASE_SECUENCIA_TERMINADO
};


struct EstadoJugadorSecuenciaNeon
{
    bool eliminado = false;
    bool completoRonda = false;
    bool falloEstaRonda = false;

    int indiceRespuesta = 0;
    int aciertosTotales = 0;
    int rondasSuperadas = 0;
    int puntuacionFinal = 0;

    float tiempoRespuestaBot = 0.0f;
    int indiceFalloBot = -1;
};


struct MinijuegoSecuenciaNeon
{
    ResultadoMinijuego resultado{};
    EstadoJugadorSecuenciaNeon jugadores[MAX_PARTICIPANTES];

    PulsoSecuenciaNeon secuencia[MAX_PULSOS_SECUENCIA_NEON];
    int cantidadPulsos = 0;
    int indiceMuestra = 0;
    int numeroRonda = 0;

    FaseSecuenciaNeon fase = FASE_SECUENCIA_PREPARACION;
    bool pulsoVisible = false;

    float tiempoPreparacion = 0.0f;
    float tiempoPasoMuestra = 0.0f;
    float tiempoRespuesta = 0.0f;
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
