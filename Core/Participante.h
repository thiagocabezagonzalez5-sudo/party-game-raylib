#pragma once

#include "raylib.h"


inline constexpr int MAX_PARTICIPANTES = 4;


enum TipoControl
{
    CONTROL_NINGUNO = 0,
    CONTROL_TECLADO_COMPLETO,
    CONTROL_TECLADO_WASD,
    CONTROL_TECLADO_FLECHAS,
    CONTROL_GAMEPAD
};


struct Participante
{
    bool activo = false;

    int numeroJugador = 1;

    TipoControl control =
        CONTROL_NINGUNO;

    int indiceGamepad = -1;

    int personajeSeleccionado = -1;

    Color color = LIGHTGRAY;

    bool conectado = false;
};
