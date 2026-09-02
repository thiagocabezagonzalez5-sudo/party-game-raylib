#pragma once

#include "Core/WindowUtils.h"


enum ModoTeclado
{
    TECLADO_COMPLETO = 0,
    TECLADO_DIVIDIDO
};


struct ConfiguracionJuego
{
    ModoVentana modoVentana = MODO_PANTALLA_COMPLETA;

    int indiceResolucion = 0;
    int indiceFPS = 1;

    bool mostrarFPS = true;

    float volumenMusica = 0.35f;
    float volumenSonidos = 0.60f;

    ModoTeclado modoTeclado = TECLADO_DIVIDIDO;
};


bool CargarConfiguracion(
    const char* ruta,
    ConfiguracionJuego& config
);


bool GuardarConfiguracion(
    const char* ruta,
    const ConfiguracionJuego& config
);
