#pragma once

#include "raylib.h"

//==================================================
// RESOLUCION
//==================================================

struct Resolucion
{
    int ancho;
    int alto;
};


//==================================================
// MODOS DE VENTANA
//==================================================

enum ModoVentana
{
    MODO_VENTANA = 0,
    MODO_PANTALLA_COMPLETA,
    MODO_SIN_BORDES
};


//==================================================
// FUNCIONES
//==================================================

const char* NombreModoVentana(
    ModoVentana modo
);

void AgregarResolucion(
    Resolucion resoluciones[],
    int& cantidad,
    int maximo,
    int ancho,
    int alto
);

void CentrarVentana(
    int ancho,
    int alto
);

void AplicarModoVentana(
    ModoVentana& modoActual,
    ModoVentana nuevoModo,
    Resolucion resolucion
);