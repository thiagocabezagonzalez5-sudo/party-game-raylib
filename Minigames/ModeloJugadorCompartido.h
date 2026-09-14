#pragma once

#include "Core/Participante.h"
#include "Core/RecursosJuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Minigames/TransformacionModeloJugador.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <cstring>


//==================================================
// MODELO COMPARTIDO DE JUGADOR
//==================================================
//
// Todos los minijuegos reutilizan una sola instancia del GLB. Asi evitamos
// cargar el mismo modelo y sus animaciones una vez por minijuego/jugador.
//==================================================

struct RecursoModeloJugadorCompartido
{
    Model modelo{};
    bool modeloCargado = false;

    ModelAnimation* animaciones = nullptr;
    int cantidadAnimaciones = 0;
    int indiceIdle = -1;
};


inline RecursoModeloJugadorCompartido& ObtenerModeloJugadorCompartido()
{
    static RecursoModeloJugadorCompartido recurso;
    return recurso;
}


inline bool NombreAnimacionEsIdleCompartido(const char* nombre)
{
    if (nombre == nullptr)
    {
        return false;
    }

    return
        std::strstr(nombre, "Idle") != nullptr ||
        std::strstr(nombre, "idle") != nullptr ||
        std::strstr(nombre, "IDLE") != nullptr;
}


inline void InicializarModeloJugadorCompartido()
{
    RecursoModeloJugadorCompartido& recurso =
        ObtenerModeloJugadorCompartido();

    if (recurso.modeloCargado)
    {
        return;
    }

    if (!FileExists(RUTA_MODELO_JUGADOR_3D))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro el modelo compartido del jugador: %s",
            RUTA_MODELO_JUGADOR_3D
        );
        return;
    }

    recurso.modelo = LoadModel(RUTA_MODELO_JUGADOR_3D);
    recurso.modeloCargado = recurso.modelo.meshCount > 0;

    if (!recurso.modeloCargado)
    {
        recurso.modelo = {};
        TraceLog(LOG_WARNING, "No se pudo cargar el modelo compartido del jugador");
        return;
    }

    PrepararTransformacionModeloJugador(recurso.modelo);

    recurso.animaciones =
        LoadModelAnimations(
            RUTA_MODELO_JUGADOR_3D,
            &recurso.cantidadAnimaciones
        );

    for (
        int i = 0;
        recurso.animaciones != nullptr &&
        i < recurso.cantidadAnimaciones;
        i++
    )
    {
        if (NombreAnimacionEsIdleCompartido(recurso.animaciones[i].name))
        {
            recurso.indiceIdle = i;
            break;
        }
    }

    if (
        recurso.indiceIdle < 0 &&
        recurso.animaciones != nullptr &&
        recurso.cantidadAnimaciones == 1
    )
    {
        recurso.indiceIdle = 0;
    }
}


inline void ActualizarAnimacionModeloJugadorCompartido()
{
    RecursoModeloJugadorCompartido& recurso =
        ObtenerModeloJugadorCompartido();

    if (
        !recurso.modeloCargado ||
        recurso.animaciones == nullptr ||
        recurso.indiceIdle < 0 ||
        recurso.indiceIdle >= recurso.cantidadAnimaciones
    )
    {
        return;
    }

    ModelAnimation& animacion =
        recurso.animaciones[recurso.indiceIdle];

    if (
        animacion.keyframeCount <= 0 ||
        !IsModelAnimationValid(recurso.modelo, animacion)
    )
    {
        return;
    }

    const int FPS_ANIMACION = 30;
    int fotograma =
        (int)(GetTime() * FPS_ANIMACION) % animacion.keyframeCount;

    UpdateModelAnimation(recurso.modelo, animacion, fotograma);
}


inline float ObtenerAnguloModeloJugadorCompartido(
    const JugadorPrueba& jugador
)
{
    float x = jugador.direccionMirada.x;
    float z = jugador.direccionMirada.z;

    if (std::fabs(x) < 0.001f && std::fabs(z) < 0.001f)
    {
        return 0.0f;
    }

    return std::atan2(x, z) * RAD2DEG;
}


inline void DibujarSombraModeloJugadorCompartido(
    const JugadorPrueba& jugador
)
{
    if (jugador.cayendo || !jugador.enSuelo)
    {
        return;
    }

    Vector3 posicionSombra =
    {
        jugador.posicion.x,
        jugador.posicion.y - jugador.tamano.y * 0.5f + 0.012f,
        jugador.posicion.z
    };

    float radio = jugador.tamano.x * 0.48f;

    DrawCylinder(
        posicionSombra,
        radio,
        radio,
        0.018f,
        10,
        Fade(BLACK, 0.28f)
    );
}


inline void DibujarJugadorModeloCompartido(
    const JugadorPrueba& jugador,
    const Participante& participante
)
{
    if (
        !participante.activo ||
        !participante.conectado ||
        jugador.cayendo
    )
    {
        return;
    }

    DibujarSombraModeloJugadorCompartido(jugador);

    InicializarModeloJugadorCompartido();

    RecursoModeloJugadorCompartido& recurso =
        ObtenerModeloJugadorCompartido();

    if (!recurso.modeloCargado)
    {
        DrawCube(
            jugador.posicion,
            jugador.tamano.x,
            jugador.tamano.y,
            jugador.tamano.z,
            participante.color
        );
        return;
    }

    ActualizarAnimacionModeloJugadorCompartido();

    float alpha = 1.0f;

    if (jugador.tiempoInmunidad > 0.0f)
    {
        int fase = (int)(jugador.tiempoInmunidad * 12.0f);
        alpha = fase % 2 == 0 ? 0.22f : 1.0f;
    }

    Color color = Fade(participante.color, alpha);

    Vector3 posicion = jugador.posicion;
    posicion.y -= jugador.tamano.y * 0.5f;
    posicion.y += 0.04f;

    float escalaY = jugador.aplastado ? 0.28f : 1.0f;

    Vector3 escala =
    {
        ESCALA_MODELO_JUGADOR_3D,
        ESCALA_MODELO_JUGADOR_3D * escalaY,
        ESCALA_MODELO_JUGADOR_3D
    };

    DrawModelEx(
        recurso.modelo,
        posicion,
        { 0.0f, 1.0f, 0.0f },
        ObtenerAnguloModeloJugadorCompartido(jugador),
        escala,
        color
    );

    if (jugador.golpeando && !jugador.aplastado)
    {
        Vector3 golpe =
        {
            jugador.posicion.x + jugador.direccionMirada.x * 0.72f,
            jugador.posicion.y + 0.12f,
            jugador.posicion.z + jugador.direccionMirada.z * 0.72f
        };

        DrawSphere(golpe, 0.18f, Fade(color, 0.88f));
    }
}


inline void DibujarModeloJugadorEnPosicion(
    Vector3 posicionPies,
    float anguloY,
    Color color,
    float escala = ESCALA_MODELO_JUGADOR_3D
)
{
    InicializarModeloJugadorCompartido();

    RecursoModeloJugadorCompartido& recurso =
        ObtenerModeloJugadorCompartido();

    if (!recurso.modeloCargado)
    {
        float factorEscala =
            escala / ESCALA_MODELO_JUGADOR_3D;

        float anchoFallback = 0.78f * factorEscala;
        float altoFallback = 1.44f * factorEscala;

        DrawCube(
            {
                posicionPies.x,
                posicionPies.y + altoFallback * 0.5f,
                posicionPies.z
            },
            anchoFallback,
            altoFallback,
            anchoFallback,
            color
        );
        return;
    }

    ActualizarAnimacionModeloJugadorCompartido();

    DrawModelEx(
        recurso.modelo,
        posicionPies,
        { 0.0f, 1.0f, 0.0f },
        anguloY,
        { escala, escala, escala },
        color
    );
}


inline void DescargarModeloJugadorCompartido()
{
    RecursoModeloJugadorCompartido& recurso =
        ObtenerModeloJugadorCompartido();

    if (recurso.animaciones != nullptr)
    {
        UnloadModelAnimations(
            recurso.animaciones,
            recurso.cantidadAnimaciones
        );

        recurso.animaciones = nullptr;
        recurso.cantidadAnimaciones = 0;
        recurso.indiceIdle = -1;
    }

    if (recurso.modeloCargado)
    {
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.modeloCargado = false;
    }
}
