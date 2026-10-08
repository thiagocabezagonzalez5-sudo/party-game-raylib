#pragma once

#include "Board/Casilla.h"


inline constexpr int MAX_CASILLAS_TABLERO =
    112;


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


    // Plano base + conexiones + casillas (usado por el prototipo).
    void Dibujar() const;

    // Solo conexiones + casillas: cada tablero final dibuja
    // su propio suelo en su decoracion.
    void DibujarRuta() const;

    // Igual, con el estilo de losa/sendero del tablero y el tiempo
    // para animar el brillo de las incrustaciones.
    void DibujarRuta(
        const EstiloCasilla& estilo,
        float tiempo
    ) const;
};


// Libera las mallas en GPU de senderos y losas (al cerrar el juego).
void DescargarMallasRutaTablero();
