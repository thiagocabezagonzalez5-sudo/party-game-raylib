#pragma once

#include "Core/Participante.h"
#include "Minigames/TiposMinijuegos.h"


enum AccionTronco
{
    ACCION_TRONCO_ARRIBA = 0,
    ACCION_TRONCO_ABAJO,
    ACCION_TRONCO_IZQUIERDA,
    ACCION_TRONCO_DERECHA
};


enum EstadoPartidaTronco
{
    TRONCO_PREPARANDO = 0,
    TRONCO_JUGANDO,
    TRONCO_GANADO,
    TRONCO_PERDIDO
};


struct EstadoJugadorTronco
{
    AccionTronco accion =
        ACCION_TRONCO_ARRIBA;

    bool respondio = false;
    bool acerto = false;

    float animacionGolpe = 0.0f;
};


struct MinijuegoTronco
{
    Camera3D camara{};

    EstadoJugadorTronco estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoPartidaTronco estado =
        TRONCO_PREPARANDO;

    int rondasCompletadas = 0;
    int rondasObjetivo = 10;
    int jugadoresEnRonda = 0;

    float tiempoPreparacion = 0.0f;
    float tiempoPartida = 0.0f;
    float tiempoRonda = 0.0f;
    float tiempoPausaRonda = 0.0f;

    bool rondaEnPausa = false;
    bool ultimaRondaCorrecta = false;

    void Inicializar();

    void ConfigurarJugadores(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    ) const;

    void Reiniciar(
        const Participante participantes[],
        int cantidadMaxima
    );

    void PrepararNuevaRonda(
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
};
