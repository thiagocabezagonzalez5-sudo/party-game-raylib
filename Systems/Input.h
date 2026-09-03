#pragma once

#include "Core/ConfiguracionJuego.h"
#include "Core/Participante.h"

struct InputJugador{
    float moverX;
    float moverZ;

    bool saltar;
    bool accion;
};


struct InputSeleccionParticipante
{
    bool izquierda = false;
    bool derecha = false;
    bool arriba = false;
    bool abajo = false;
    bool confirmar = false;
    bool cancelar = false;
};


void ConfigurarControlesParticipantes(
    Participante participantes[],
    int cantidadMaxima,
    ModoTeclado modoTeclado
);


void ActualizarConexionParticipante(
    Participante& participante
);


void ActualizarConexionesParticipantes(
    Participante participantes[],
    int cantidadMaxima
);


InputJugador LeerInputParticipante(
    const Participante& participante
);


InputSeleccionParticipante LeerInputSeleccionParticipante(
    const Participante& participante
);


const char* ObtenerNombreControlParticipante(
    const Participante& participante
);


// Puente temporal para Entities/Jugador.
InputJugador LeerInputJugador(
    int jugador
);
