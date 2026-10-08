#pragma once

#include "raylib.h"

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


//==================================================
// ZONA MUERTA DEL STICK
//==================================================
//
// Unica zona muerta para el stick izquierdo de todos los mandos (la elige
// el jugador en Configuracion > Controles). Es RADIAL: se mide la magnitud
// del vector (x, y), no cada eje por separado, asi las diagonales se
// comportan igual que los ejes. Fuera de la zona el rango restante se
// reescala a 0..1 para que el movimiento empiece suave y llegue al maximo.
// No afecta a gatillos, cruceta ni botones.

// fraccion: 0.05 .. 0.35 (ver ZONA_MUERTA_STICK_* en ConfiguracionJuego.h).
void EstablecerZonaMuertaStick(
    float fraccion
);

float ObtenerZonaMuertaStick();

// Stick izquierdo ya filtrado y reescalado (magnitud 0..1). Devuelve
// (0, 0) si el mando no esta conectado o el stick esta dentro de la zona.
Vector2 LeerStickIzquierdo(
    int indiceGamepad
);

// Aplica la zona muerta radial actual a un vector de stick crudo.
Vector2 FiltrarZonaMuertaStick(
    Vector2 eje
);

// Stick izquierdo sin filtrar (solo para la vista previa de Configuracion).
Vector2 LeerStickIzquierdoCrudo(
    int indiceGamepad
);


// Puente temporal para Entities/Jugador.
InputJugador LeerInputJugador(
    int jugador
);
