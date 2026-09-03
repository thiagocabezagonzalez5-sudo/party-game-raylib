#pragma once

#include "Board/Tablero.h"
#include "Core/Participante.h"


//==================================================
// FASES DEL TURNO
//==================================================

enum FaseTurnoTablero
{
    FASE_TABLERO_ESPERANDO_DADO = 0,
    FASE_TABLERO_MOSTRANDO_DADO,
    FASE_TABLERO_ELIGIENDO_RUTA,
    FASE_TABLERO_MOVIENDO,
    FASE_TABLERO_EVENTO_CASILLA,
    FASE_TABLERO_FIN_TURNO,
    FASE_TABLERO_PARTIDA_TERMINADA
};


//==================================================
// ESTADO DE CADA JUGADOR
//==================================================

struct EstadoJugadorTablero
{
    bool participa =
        false;

    int casillaActual =
        0;

    int puntos =
        0;

    Vector3 posicionVisual{};
};


//==================================================
// PROTOTIPO JUGABLE
//==================================================

struct PrototipoTablero
{
    Tablero tablero;

    Participante* participantes =
        nullptr;

    EstadoJugadorTablero jugadores[
        MAX_PARTICIPANTES
    ];

    int ordenParticipantes[
        MAX_PARTICIPANTES
    ] = {};

    int cantidadJugadores =
        0;

    int indiceOrdenTurno =
        0;

    int rondaActual =
        1;

    int cantidadRondas =
        5;

    int valorDado =
        0;

    int pasosPendientes =
        0;

    int opcionRuta =
        0;

    bool direccionRutaBloqueada =
        false;

    int casillaDestinoMovimiento =
        -1;

    Vector3 posicionInicioMovimiento{};
    Vector3 posicionFinMovimiento{};

    float progresoMovimiento =
        0.0f;

    float tiempoFase =
        0.0f;

    bool eventoCasillaResuelto =
        false;

    FaseTurnoTablero fase =
        FASE_TABLERO_ESPERANDO_DADO;

    Camera3D camara{};

    bool inicializado =
        false;


    void Inicializar(
        Participante participantesJuego[],
        int cantidadParticipantesJuego
    );


    void Reiniciar();


    void Actualizar(
        float deltaTime
    );


    void Dibujar(
        bool mostrarDebug
    ) const;
};
