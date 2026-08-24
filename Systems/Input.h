#pragma once

struct InputJugador{
    float moverX;
    float moverZ;

    bool saltar;
    bool accion;
};

InputJugador LeerInputJugador(
    int jugador
);