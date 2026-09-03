#pragma once

#include "Board/Casilla.h"


inline constexpr int MAX_CASILLAS_TABLERO =
    32;


//==================================================
// TABLERO
//==================================================

struct Tablero
{
    Casilla casillas[
        MAX_CASILLAS_TABLERO
    ];

    int cantidadCasillas =
        0;

    bool recorridoValido =
        false;


    void InicializarPrototipo();


    int AgregarCasilla(
        Vector3 posicion,
        TipoCasilla tipo
    );


    bool ConectarCasillas(
        int origen,
        int destino
    );


    bool ValidarRecorrido();


    const Casilla* ObtenerCasilla(
        int indice
    ) const;


    void Dibujar() const;
};
