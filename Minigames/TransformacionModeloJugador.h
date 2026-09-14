#pragma once

#include "Core/RecursosJuego.h"

#include "raylib.h"
#include "raymath.h"


//==================================================
// TRANSFORMACION COMPARTIDA DEL MODELO DE JUGADOR
//==================================================
//
// Algunos GLB traen la geometria lejos del origen aunque visualmente hayan
// sido exportados correctamente. Esta utilidad centra el modelo sobre X/Z,
// coloca su punto mas bajo en Y = 0 y aplica la correccion de eje configurada.
//==================================================

inline void PrepararTransformacionModeloJugador(Model& modelo)
{
    if (modelo.meshCount <= 0)
    {
        return;
    }

    // GetModelBoundingBox tambien aplica model.transform. Se usa una copia
    // con identidad para medir solamente la geometria ya importada por raylib.
    Model modeloSinTransformacion = modelo;
    modeloSinTransformacion.transform = MatrixIdentity();

    BoundingBox limites =
        GetModelBoundingBox(modeloSinTransformacion);

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

    Matrix rotacionEje =
        MatrixRotateX(
            ROTACION_X_MODELO_JUGADOR_3D * DEG2RAD
        );

    Vector3 primeraEsquina =
        Vector3Transform(esquinas[0], rotacionEje);

    BoundingBox limitesRotados =
    {
        primeraEsquina,
        primeraEsquina
    };

    for (int i = 1; i < 8; i++)
    {
        Vector3 esquina =
            Vector3Transform(esquinas[i], rotacionEje);

        if (esquina.x < limitesRotados.min.x)
            limitesRotados.min.x = esquina.x;
        if (esquina.y < limitesRotados.min.y)
            limitesRotados.min.y = esquina.y;
        if (esquina.z < limitesRotados.min.z)
            limitesRotados.min.z = esquina.z;

        if (esquina.x > limitesRotados.max.x)
            limitesRotados.max.x = esquina.x;
        if (esquina.y > limitesRotados.max.y)
            limitesRotados.max.y = esquina.y;
        if (esquina.z > limitesRotados.max.z)
            limitesRotados.max.z = esquina.z;
    }

    Vector3 centroBase =
    {
        (limitesRotados.min.x + limitesRotados.max.x) * 0.5f,
        limitesRotados.min.y,
        (limitesRotados.min.z + limitesRotados.max.z) * 0.5f
    };

    Matrix centrarEnOrigen =
        MatrixTranslate(
            -centroBase.x,
            -centroBase.y,
            -centroBase.z
        );

    // MatrixMultiply aplica primero la matriz izquierda. Por eso el modelo
    // se endereza antes de trasladar su base al origen.
    modelo.transform =
        MatrixMultiply(rotacionEje, centrarEnOrigen);
}
