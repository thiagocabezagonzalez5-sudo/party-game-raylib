#pragma once

#include "raylib.h"


enum OpcionModoJuego
{
    MODO_JUEGO_MINIJUEGOS = 0,
    MODO_JUEGO_TABLERO
};


struct MenuModoJuego
{
    int opcionSeleccionada =
        MODO_JUEGO_MINIJUEGOS;

    bool confirmar = false;
    bool volver = false;

    float alphaEntrada = 0.0f;

    void Inicializar();

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;
};
