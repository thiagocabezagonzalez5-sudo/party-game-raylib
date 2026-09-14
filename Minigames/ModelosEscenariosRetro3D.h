#pragma once

#include "Core/RecursosJuego.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>


//==================================================
// MODELOS COMPARTIDOS DE ESCENARIOS RETRO 3D
//==================================================
//
// Los modelos de decoracion se cargan una sola vez para que todos los
// minijuegos que usan el mismo tema compartan la memoria de GPU. Cada slot
// es opcional: si el archivo no existe, el escenario conserva sus primitivas
// 3D como fallback y el juego sigue funcionando.
//==================================================

struct RecursoModeloEscenarioRetro3D
{
    Model modelo{};
    const char* ruta = nullptr;
    Vector3 dimensiones{};
    bool cargado = false;
};


struct ModelosEscenariosRetro3D
{
    RecursoModeloEscenarioRetro3D montanaLava;
    bool inicializados = false;
};


inline ModelosEscenariosRetro3D& ObtenerModelosEscenariosRetro3D()
{
    static ModelosEscenariosRetro3D recursos;
    return recursos;
}


inline BoundingBox RotarLimitesModeloEscenarioRetro3D(
    BoundingBox limites,
    Matrix rotacion
)
{
    Vector3 esquinas[8] =
    {
        { limites.min.x, limites.min.y, limites.min.z },
        { limites.max.x, limites.min.y, limites.min.z },
        { limites.min.x, limites.max.y, limites.min.z },
        { limites.max.x, limites.max.y, limites.min.z },
        { limites.min.x, limites.min.y, limites.max.z },
        { limites.max.x, limites.min.y, limites.max.z },
        { limites.min.x, limites.max.y, limites.max.z },
        { limites.max.x, limites.max.y, limites.max.z }
    };

    Vector3 primera = Vector3Transform(esquinas[0], rotacion);
    BoundingBox rotados = { primera, primera };

    for (int i = 1; i < 8; i++)
    {
        Vector3 esquina = Vector3Transform(esquinas[i], rotacion);

        if (esquina.x < rotados.min.x) rotados.min.x = esquina.x;
        if (esquina.y < rotados.min.y) rotados.min.y = esquina.y;
        if (esquina.z < rotados.min.z) rotados.min.z = esquina.z;

        if (esquina.x > rotados.max.x) rotados.max.x = esquina.x;
        if (esquina.y > rotados.max.y) rotados.max.y = esquina.y;
        if (esquina.z > rotados.max.z) rotados.max.z = esquina.z;
    }

    return rotados;
}


inline void PrepararSlotModeloEscenarioRetro3D(
    RecursoModeloEscenarioRetro3D& recurso,
    const char* ruta,
    float rotacionX
)
{
    recurso = {};
    recurso.ruta = ruta;

    if (ruta == nullptr || !FileExists(ruta))
    {
        return;
    }

    recurso.modelo = LoadModel(ruta);

    if (recurso.modelo.meshCount <= 0)
    {
        recurso.modelo = {};
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar el modelo de escenario: %s",
            ruta
        );
        return;
    }

    Model modeloSinTransformacion = recurso.modelo;
    modeloSinTransformacion.transform = MatrixIdentity();

    BoundingBox limites = GetModelBoundingBox(modeloSinTransformacion);
    Matrix rotacion = MatrixRotateX(rotacionX * DEG2RAD);
    BoundingBox limitesRotados =
        RotarLimitesModeloEscenarioRetro3D(limites, rotacion);

    recurso.dimensiones =
    {
        limitesRotados.max.x - limitesRotados.min.x,
        limitesRotados.max.y - limitesRotados.min.y,
        limitesRotados.max.z - limitesRotados.min.z
    };

    if (
        recurso.dimensiones.x <= 0.001f ||
        recurso.dimensiones.y <= 0.001f ||
        recurso.dimensiones.z <= 0.001f
    )
    {
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.dimensiones = {};

        TraceLog(
            LOG_WARNING,
            "El modelo de escenario no tiene dimensiones validas: %s",
            ruta
        );
        return;
    }

    Vector3 centroBase =
    {
        (limitesRotados.min.x + limitesRotados.max.x) * 0.5f,
        limitesRotados.min.y,
        (limitesRotados.min.z + limitesRotados.max.z) * 0.5f
    };

    Matrix centrarBase = MatrixTranslate(
        -centroBase.x,
        -centroBase.y,
        -centroBase.z
    );

    recurso.modelo.transform = MatrixMultiply(rotacion, centrarBase);
    recurso.cargado = true;
}


inline void InicializarModelosEscenariosRetro3D()
{
    ModelosEscenariosRetro3D& recursos =
        ObtenerModelosEscenariosRetro3D();

    if (recursos.inicializados)
    {
        return;
    }

    PrepararSlotModeloEscenarioRetro3D(
        recursos.montanaLava,
        RUTA_MODELO_MONTANA_LAVA_3D,
        ROTACION_X_MODELO_MONTANA_LAVA_3D
    );

    recursos.inicializados = true;
}


inline bool DibujarModeloMontanaLavaEscenarioRetro3D(
    Vector3 centroBase,
    float anchoObjetivo,
    float alturaObjetivo,
    float anguloY
)
{
    RecursoModeloEscenarioRetro3D& recurso =
        ObtenerModelosEscenariosRetro3D().montanaLava;

    if (!recurso.cargado)
    {
        return false;
    }

    float escalaAncho = anchoObjetivo / recurso.dimensiones.x;
    float escalaAlto = alturaObjetivo / recurso.dimensiones.y;
    float escala =
        escalaAncho < escalaAlto
        ? escalaAncho
        : escalaAlto;

    if (!std::isfinite(escala) || escala <= 0.001f)
    {
        return false;
    }

    DrawModelEx(
        recurso.modelo,
        centroBase,
        { 0.0f, 1.0f, 0.0f },
        anguloY,
        { escala, escala, escala },
        WHITE
    );

    return true;
}


inline void DescargarSlotModeloEscenarioRetro3D(
    RecursoModeloEscenarioRetro3D& recurso
)
{
    if (recurso.cargado)
    {
        UnloadModel(recurso.modelo);
    }

    recurso = {};
}


inline void DescargarModelosEscenariosRetro3D()
{
    ModelosEscenariosRetro3D& recursos =
        ObtenerModelosEscenariosRetro3D();

    if (!recursos.inicializados)
    {
        return;
    }

    DescargarSlotModeloEscenarioRetro3D(recursos.montanaLava);
    recursos.inicializados = false;
}
