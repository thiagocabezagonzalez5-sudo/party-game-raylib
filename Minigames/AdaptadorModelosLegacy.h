#pragma once

#include "Minigames/ModeloJugadorCompartido.h"

#include <cmath>


//==================================================
// ADAPTADOR TEMPORAL PARA DIBUJOS LEGACY
//==================================================
//
// Algunas pantallas antiguas dibujan al jugador directamente con primitivas.
// El adaptador se inyecta SOLO en esas unidades desde CMake y reemplaza las
// dimensiones exactas del placeholder de jugador por el GLB compartido.
// El resto de la geometria sigue llegando a raylib sin cambios.
//==================================================

inline bool CasiIgualModeloLegacy(
    float valor,
    float esperado,
    float tolerancia = 0.035f
)
{
    return std::fabs(valor - esperado) <= tolerancia;
}


inline bool EsCuboJugadorLegacy(
    float ancho,
    float alto,
    float profundidad
)
{
#if defined(MODELO_LEGACY_TRONCO)
    return
        CasiIgualModeloLegacy(ancho, 0.78f, 0.08f) &&
        CasiIgualModeloLegacy(alto, 1.45f, 0.10f) &&
        CasiIgualModeloLegacy(profundidad, 0.78f, 0.08f);
#elif defined(MODELO_LEGACY_CAPITAN)
    return
        CasiIgualModeloLegacy(ancho, 0.72f) &&
        CasiIgualModeloLegacy(alto, 1.22f) &&
        CasiIgualModeloLegacy(profundidad, 0.72f);
#elif defined(MODELO_LEGACY_TABLERO_FINAL)
    return
        CasiIgualModeloLegacy(ancho, 0.50f) &&
        CasiIgualModeloLegacy(alto, 0.62f) &&
        CasiIgualModeloLegacy(profundidad, 0.50f);
#elif defined(MODELO_LEGACY_TABLERO_PRUEBA)
    return
        CasiIgualModeloLegacy(ancho, 0.48f) &&
        CasiIgualModeloLegacy(alto, 0.62f) &&
        CasiIgualModeloLegacy(profundidad, 0.48f);
#else
    (void)ancho;
    (void)alto;
    (void)profundidad;
    return false;
#endif
}


inline bool EsCabezaJugadorLegacy(float radio)
{
#if defined(MODELO_LEGACY_TABLERO_FINAL)
    return CasiIgualModeloLegacy(radio, 0.27f, 0.01f);
#elif defined(MODELO_LEGACY_TABLERO_PRUEBA)
    return CasiIgualModeloLegacy(radio, 0.26f, 0.01f);
#else
    (void)radio;
    return false;
#endif
}


inline float AnguloJugadorLegacy(Vector3 posicion)
{
#if defined(MODELO_LEGACY_TRONCO)
    float centroEquipo = posicion.x < 0.0f ? -3.9f : 3.9f;

    // Los jugadores miran hacia el tronco ubicado en el centro de su equipo.
    return posicion.x < centroEquipo ? 90.0f : -90.0f;
#elif defined(MODELO_LEGACY_CAPITAN)
    // Los participantes miran hacia el capitan, ubicado al fondo de la arena.
    return 180.0f;
#else
    (void)posicion;
    return 0.0f;
#endif
}


inline float EscalaJugadorLegacy()
{
#if defined(MODELO_LEGACY_TABLERO_FINAL) || defined(MODELO_LEGACY_TABLERO_PRUEBA)
    return 0.18f;
#else
    return ESCALA_MODELO_JUGADOR_3D;
#endif
}


inline void DrawCubeConModeloLegacy(
    Vector3 posicion,
    float ancho,
    float alto,
    float profundidad,
    Color color
)
{
    if (EsCuboJugadorLegacy(ancho, alto, profundidad))
    {
        Vector3 pies =
        {
            posicion.x,
            posicion.y - alto * 0.5f,
            posicion.z
        };

        DibujarModeloJugadorEnPosicion(
            pies,
            AnguloJugadorLegacy(posicion),
            color,
            EscalaJugadorLegacy()
        );

        return;
    }

    DrawCubeV(
        posicion,
        { ancho, alto, profundidad },
        color
    );
}


inline void DrawCubeWiresConModeloLegacy(
    Vector3 posicion,
    float ancho,
    float alto,
    float profundidad,
    Color color
)
{
    if (EsCuboJugadorLegacy(ancho, alto, profundidad))
    {
        return;
    }

    DrawCubeWiresV(
        posicion,
        { ancho, alto, profundidad },
        color
    );
}


inline void DrawSphereConModeloLegacy(
    Vector3 centro,
    float radio,
    Color color
)
{
    if (EsCabezaJugadorLegacy(radio))
    {
        return;
    }

    DrawSphereEx(
        centro,
        radio,
        16,
        16,
        color
    );
}


#define DrawCube DrawCubeConModeloLegacy
#define DrawCubeWires DrawCubeWiresConModeloLegacy
#define DrawSphere DrawSphereConModeloLegacy
