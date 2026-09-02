#pragma once

#include "raylib.h"

struct PantallaLogo
{
    Texture2D logo{};

    bool logoCargado = false;

    float tiempo = 0.0f;


    const float DURACION_TOTAL =
        5.0f;


    const float DURACION_FADE_ENTRADA =
        0.8f;


    const float INICIO_FADE_SALIDA =
        4.0f;


    void Inicializar(
        const char* rutaLogo
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    bool Termino() const;

    void Descargar();
};