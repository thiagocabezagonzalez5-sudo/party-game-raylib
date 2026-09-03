#pragma once

#include "raylib.h"


//==================================================
// TIPOS DE CASILLA
//==================================================

enum TipoCasilla
{
    CASILLA_NEUTRA = 0,
    CASILLA_POSITIVA,
    CASILLA_NEGATIVA,
    CASILLA_ESPECIAL
};


inline constexpr int MAX_CONEXIONES_CASILLA =
    2;


//==================================================
// CONEXION
//==================================================

struct ConexionCasilla
{
    int destino = -1;
};


//==================================================
// CASILLA
//==================================================

struct Casilla
{
    int indice = -1;

    Vector3 posicion{};

    TipoCasilla tipo =
        CASILLA_NEUTRA;

    ConexionCasilla conexiones[
        MAX_CONEXIONES_CASILLA
    ];

    int cantidadConexiones =
        0;
};


Color ObtenerColorCasilla(
    TipoCasilla tipo
);


const char* ObtenerNombreTipoCasilla(
    TipoCasilla tipo
);
