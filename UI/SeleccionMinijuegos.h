#pragma once

#include "Core/Participante.h"
#include "raylib.h"


enum IdMinijuegoCatalogo
{
    CATALOGO_COLOR_SEGURO = 0,
    CATALOGO_PELOTAS,
    CATALOGO_TRONCO,
    CATALOGO_FABRICA_67,
    CATALOGO_ISLA_FUEGO,
    CATALOGO_CAPITAN_MANDA,
    CATALOGO_BARRA_GIRATORIA,
    CANTIDAD_MINIJUEGOS_CATALOGO
};


struct SeleccionMinijuegos
{
    int indiceSeleccionado = 0;
    int indiceAnterior = -1;

    bool confirmado = false;
    bool volver = false;

    float progresoPanel = 0.0f;

    void Inicializar();

    void Actualizar(
        float deltaTime,
        const Participante& jugadorUno
    );

    void Dibujar(
        const Participante& jugadorUno
    ) const;
};
