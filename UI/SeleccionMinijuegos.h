#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/Participante.h"
#include "raylib.h"



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
