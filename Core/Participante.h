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

    // Los bots ocupan un puesto real de la partida, pero no leen
    // ningun dispositivo. Por ahora son deliberadamente inmoviles.
    bool esBot = false;

    int numeroJugador = 1;

    TipoControl control =
        CONTROL_NINGUNO;

    int indiceGamepad = -1;

    int personajeSeleccionado = -1;

    Color color = LIGHTGRAY;

    bool conectado = false;
};


inline int ObtenerIndicesParticipantesActivos(
    const Participante participantes[],
    int indicesActivos[],
    int capacidad
)
{
    int cantidad = 0;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES && cantidad < capacidad;
        i++
    )
    {
        if (participantes[i].activo)
        {
            indicesActivos[cantidad] = i;
            cantidad++;
        }
    }

    return cantidad;
}


inline int ContarParticipantesHumanos(
    const Participante participantes[]
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            !participantes[i].esBot
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


inline void CompletarParticipantesConBots(
    Participante participantes[],
    int cantidadMaxima
)
{
    int limite =
        cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        Participante& participante =
            participantes[i];

        if (participante.activo)
        {
            participante.esBot = false;
            continue;
        }

        participante.activo = true;
        participante.esBot = true;
        participante.conectado = true;
        participante.control = CONTROL_NINGUNO;
        participante.indiceGamepad = -1;
        participante.personajeSeleccionado = i;
        participante.color = Color{ 145, 145, 152, 255 };
    }
}
