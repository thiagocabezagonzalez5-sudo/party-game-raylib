#pragma once

#include "Core/WindowUtils.h"

struct ConfiguracionJuego
{
    ModoVentana modoVentana = MODO_PANTALLA_COMPLETA;

    int indiceResolucion = 0;
    int indiceFPS = 1;

    bool mostrarFPS = true;

    float volumenMusica = 0.35f;
    float volumenSonidos = 0.60f;
};

bool CargarConfiguracion(
    const char* ruta,
    ConfiguracionJuego& config
);

bool GuardarConfiguracion(
    const char* ruta,
    const ConfiguracionJuego& config
);