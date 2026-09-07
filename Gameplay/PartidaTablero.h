#pragma once

#include "Board/Tablero.h"
#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"

#include "raylib.h"


enum FasePartidaTablero
{
    FASE_PARTIDA_TABLERO_ESPERANDO_DADO = 0,
    FASE_PARTIDA_TABLERO_MOSTRANDO_DADO,
    FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA,
    FASE_PARTIDA_TABLERO_MOVIENDO,
    FASE_PARTIDA_TABLERO_DECISION_TROFEO,
    FASE_PARTIDA_TABLERO_EVENTO_CASILLA,
    FASE_PARTIDA_TABLERO_FIN_TURNO,
    FASE_PARTIDA_TABLERO_FIN_RONDA,
    FASE_PARTIDA_TABLERO_TERMINADA
};


enum EventoEspecialTablero
{
    EVENTO_TABLERO_NINGUNO = 0,
    EVENTO_TABLERO_BONIFICACION,
    EVENTO_TABLERO_MULTA,
    EVENTO_TABLERO_INTERCAMBIO
};


struct EstadoJugadorPartidaTablero
{
    bool participa = false;

    int casillaActual = 0;
    int monedas = 10;
    int trofeos = 0;

    int premioUltimoMinijuego = 0;

    Vector3 posicionVisual{};
};


struct PartidaTablero
{
    Tablero tablero;

    Participante* participantes = nullptr;
    AudioJuego* audio = nullptr;

    EstadoJugadorPartidaTablero jugadores[MAX_PARTICIPANTES];

    int ordenParticipantes[MAX_PARTICIPANTES] =
    {
        -1,
        -1,
        -1,
        -1
    };

    int cantidadJugadores = 0;
    int indiceOrdenTurno = 0;

    int rondaActual = 1;
    int cantidadRondas = 5;

    int valorDado = 0;
    int pasosPendientes = 0;

    int opcionRuta = 0;
    bool direccionRutaBloqueada = false;

    int casillaDestinoMovimiento = -1;

    Vector3 posicionInicioMovimiento{};
    Vector3 posicionFinMovimiento{};

    float progresoMovimiento = 0.0f;
    float tiempoFase = 0.0f;

    int casillaTrofeo = -1;
    int costoTrofeo = 20;

    bool compraTrofeoDisponible = false;
    bool compraTrofeoResuelta = false;

    EventoEspecialTablero ultimoEventoEspecial =
        EVENTO_TABLERO_NINGUNO;

    int variacionMonedasEvento = 0;
    int jugadorIntercambioEvento = -1;

    bool minijuegoSolicitado = false;
    bool salidaSolicitada = false;
    bool inicializado = false;

    FasePartidaTablero fase =
        FASE_PARTIDA_TABLERO_ESPERANDO_DADO;

    Camera3D camara{};

    void Inicializar(
        Participante participantesJuego[],
        int cantidadParticipantesJuego,
        AudioJuego* audioJuego
    );

    void Reiniciar();

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    bool SolicitaMinijuego() const;

    void AplicarResultadoMinijuego(
        const ResultadoMinijuego& resultado
    );

    void ContinuarTrasMinijuego();

    bool SolicitaSalida() const;
};
