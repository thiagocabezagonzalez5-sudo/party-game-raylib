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


struct InputMinijuegoParticipante
{
    bool izquierda = false;
    bool derecha = false;
    bool adelante = false;
    bool atras = false;
    bool saltar = false;
    bool golpear = false;
};


enum AccionDireccionalControl
{
    CONTROL_DIRECCION_ARRIBA = 0,
    CONTROL_DIRECCION_ABAJO,
    CONTROL_DIRECCION_IZQUIERDA,
    CONTROL_DIRECCION_DERECHA
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


InputMinijuegoParticipante LeerInputMinijuegoParticipante(
    const Participante& participante
);


bool AccionDireccionalControlPresionada(
    const Participante& participante,
    AccionDireccionalControl accion
);


const char* ObtenerTextoAccionDireccionalControl(
    const Participante& participante,
    AccionDireccionalControl accion
);


const char* ObtenerTextoBotonPrincipal(
    const Participante& participante
);


const char* ObtenerNombreControlParticipante(
    const Participante& participante
);


// Puente temporal para Entities/Jugador.
InputJugador LeerInputJugador(
    int jugador
);
