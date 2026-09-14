#pragma once

#include "Minigames/ModeloJugadorCompartido.h"

#include <cmath>


//==================================================
// ADAPTADOR TEMPORAL PARA MINIJUEGOS LEGACY
//==================================================
//
// Tronco y Capitan dibujaban a sus jugadores directamente con DrawCube en
// medio del codigo de escenario, en vez de pasar por UtilidadesMinijuegos.
// Este adaptador se inyecta SOLO en esas dos unidades desde CMake y convierte
// exclusivamente las dimensiones conocidas de jugador en el modelo GLB.
// El resto de cubos del escenario sigue llegando a raylib mediante DrawCubeV.
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
#else
    (void)ancho;
    (void)alto;
    (void)profundidad;
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
            color
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


#define DrawCube DrawCubeConModeloLegacy
#define DrawCubeWires DrawCubeWiresConModeloLegacy
