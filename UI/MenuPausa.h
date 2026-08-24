#pragma once

#include "raylib.h"
#include "Core/WindowUtils.h"
#include "Entities/Jugador.h"

enum OpcionPausa
{
    OPCION_RESOLUCION = 0,
    OPCION_MODO_VENTANA,
    OPCION_MOSTRAR_FPS,
    OPCION_LIMITE_FPS,
    OPCION_REINICIAR,
    OPCION_VOLVER,
    CANTIDAD_OPCIONES_PAUSA
};

void ActualizarMenuPausa(
    bool& menuAbierto,
    int& opcionSeleccionada,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int& indiceResolucion,
    ModoVentana& modoVentana,
    bool& mostrarFPS,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    int& indiceFPS,
    Jugador& jugador
);

void DibujarMenuPausa(
    int opcionSeleccionada,
    Resolucion resoluciones[],
    int indiceResolucion,
    ModoVentana modoVentana,
    bool mostrarFPS,
    int opcionesFPS[],
    int indiceFPS
);