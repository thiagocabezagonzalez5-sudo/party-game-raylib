#pragma once

#include "raylib.h"


//==================================================
// ESCENA 3D DE LA SELECCION DE PERSONAJES
//==================================================
//
// Escenario festivo al atardecer (plaza del HUB): una tarima con un
// pedestal por personaje, banderines, luces y confeti. Este modulo solo
// DIBUJA: el estado (quien apunta a quien, quien esta listo) lo mantiene
// SeleccionPersonajes y se lo entrega en un arreglo de PersonajeEscena3D.
//
// El modelo 3D NO se carga aqui: se reutiliza el modelo compartido
// (Minigames/ModeloJugadorCompartido) que ya posee main.cpp.
//==================================================

inline constexpr int ESCENA_SELECCION_PEDESTALES =
    4;

// Cantidad maxima de jugadores que pueden apuntar al mismo pedestal.
inline constexpr int ESCENA_SELECCION_MAX_AROS =
    4;


struct PersonajeEscena3D
{
    // Color propio del personaje (pedestal y tinte del modelo).
    Color color = LIGHTGRAY;

    // 0 = personaje en reposo, 1 = apuntado (escala, luz, giro suave).
    float foco = 0.0f;

    // Salto de confirmacion: 0 = sin salto, 0..1 = progreso del salto.
    float progresoSalto = 0.0f;

    // Giro extra en grados (confirmacion).
    float giroExtra = 0.0f;

    // Hay un jugador listo con este personaje (brillo dorado).
    bool listo = false;

    // Un bot ocupa este personaje (se dibuja una placa gris).
    bool bot = false;

    // Colores de los jugadores que apuntan aqui (aros concentricos).
    Color aros[ESCENA_SELECCION_MAX_AROS] = {};
    int cantidadAros = 0;
};


Camera3D ObtenerCamaraEscenaSeleccion(
    float tiempo
);

Vector3 ObtenerPosicionPedestalSeleccion(
    int indice
);

// Altura aproximada de la cabeza sobre el suelo (para anclar etiquetas 2D).
float ObtenerAlturaCabezaSeleccion(
    const PersonajeEscena3D& personaje
);

// Dibuja cielo + escena 3D. intensidadConfeti: 0 normal, 1 fiesta total.
void DibujarEscenaSeleccion(
    float tiempo,
    const PersonajeEscena3D personajes[],
    int cantidad,
    float intensidadConfeti
);
