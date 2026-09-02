#pragma once

#include "raylib.h"

struct FondoAnimado
{
    Image imagen{};

    Texture2D textura{};

    int cantidadFrames = 0;

    int frameActual = 0;

    float tiempoFrame = 0.0f;

    float fpsAnimacion = 15.0f;

    bool cargado = false;


    void Cargar(
        const char* ruta,
        float fps
    );

    void Actualizar(
        float deltaTime
    );

    void DibujarPantallaCompleta() const;

    void Descargar();
};