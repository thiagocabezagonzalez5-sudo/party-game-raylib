#include "Minigames/PruebaModelos.h"
#include "Minigames/ModeloJugadorCompartido.h"

#include "raymath.h"



//==================================================
// INICIALIZAR
//==================================================

void PruebaModelos::Inicializar()
{
    camara.position =
    {
        0.0f,
        4.2f,
        10.5f
    };

    camara.target =
    {
        0.0f,
        1.4f,
        0.0f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy =
        48.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;

    Reiniciar();

    InicializarModeloJugadorCompartido();

    if (AnimacionIdleModeloJugadorCompartidoActiva())
    {
        AplicarFotogramaIdleModeloJugadorCompartido(0);
    }
}


void PruebaModelos::Reiniciar()
{
    rotacion =
        0.0f;

    escala =
        0.25f;

    rotacionAutomatica =
        true;

    fotogramaAnimacionIdle =
        0.0f;
}


//==================================================
// ACTUALIZAR
//==================================================

void PruebaModelos::Actualizar(
    float deltaTime
)
{
    if (IsKeyDown(KEY_LEFT))
    {
        rotacion -=
            90.0f * deltaTime;
    }

    if (IsKeyDown(KEY_RIGHT))
    {
        rotacion +=
            90.0f * deltaTime;
    }

    if (IsKeyDown(KEY_UP))
    {
        escala +=
            0.22f * deltaTime;
    }

    if (IsKeyDown(KEY_DOWN))
    {
        escala -=
            0.22f * deltaTime;
    }

    if (escala < 0.05f)
    {
        escala =
            0.05f;
    }

    if (escala > 1.50f)
    {
        escala =
            1.50f;
    }

    if (IsKeyPressed(KEY_SPACE))
    {
        rotacionAutomatica =
            !rotacionAutomatica;
    }

    if (rotacionAutomatica)
    {
        rotacion +=
            28.0f * deltaTime;
    }

    if (AnimacionIdleModeloJugadorCompartidoActiva())
    {
        const float FOTOGRAMAS_POR_SEGUNDO =
            30.0f;

        fotogramaAnimacionIdle +=
            FOTOGRAMAS_POR_SEGUNDO *
            deltaTime;

        int cantidadFotogramas =
            ObtenerCantidadFotogramasIdleModeloJugadorCompartido();

        if (cantidadFotogramas > 0)
        {
            int fotogramaActual =
                (int)fotogramaAnimacionIdle %
                cantidadFotogramas;

            AplicarFotogramaIdleModeloJugadorCompartido(
                fotogramaActual
            );
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void PruebaModelos::Dibujar() const
{
    const Model* modelo =
        ObtenerModeloJugadorCompartidoRender();

    bool modeloCargado =
        modelo != nullptr;

    bool animacionIdleActiva =
        AnimacionIdleModeloJugadorCompartidoActiva();

    int cantidadAnimaciones =
        ObtenerCantidadAnimacionesModeloJugadorCompartido();

    ClearBackground(
        Color{
            115,
            170,
            205,
            255
        }
    );

    BeginMode3D(
        camara
    );

    DrawPlane(
        Vector3{
            0.0f,
            0.0f,
            0.0f
        },
        Vector2{
            12.0f,
            8.0f
        },
        Color{
            78,
            82,
            92,
            255
        }
    );

    Vector3 posiciones[4] =
    {
        { -3.0f, 0.35f, 0.0f },
        { -1.0f, 0.35f, 0.0f },
        { 1.0f, 0.35f, 0.0f },
        { 3.0f, 0.35f, 0.0f }
    };

    Color colores[4] =
    {
        RED,
        BLUE,
        GREEN,
        GOLD
    };

    for (
        int i = 0;
        i < 4;
        i++
    )
    {
        DrawCylinder(
            Vector3{
                posiciones[i].x,
                0.12f,
                posiciones[i].z
            },
            0.75f,
            0.75f,
            0.24f,
            24,
            DARKGRAY
        );

        if (modeloCargado)
        {
            DrawModelEx(
                *modelo,
                posiciones[i],
                Vector3{
                    0.0f,
                    1.0f,
                    0.0f
                },
                rotacion,
                Vector3{
                    escala,
                    escala,
                    escala
                },
                colores[i]
            );
        }
        else
        {
            DrawCube(
                Vector3{
                    posiciones[i].x,
                    1.0f,
                    posiciones[i].z
                },
                0.8f,
                1.8f,
                0.8f,
                colores[i]
            );
        }
    }

    EndMode3D();

    DrawText(
        "PRUEBA DE MODELOS",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        modeloCargado
        ? "GLB CARGADO CORRECTAMENTE"
        : "MODELO NO CARGADO - SE MUESTRAN CUBOS",
        25,
        70,
        22,
        modeloCargado
        ? DARKGREEN
        : MAROON
    );

    DrawText(
        TextFormat(
            "Ruta: %s",
            RUTA_MODELO_JUGADOR_3D
        ),
        25,
        105,
        18,
        BLACK
    );

    if (modeloCargado)
    {
        DrawText(
            TextFormat(
                "Meshes: %d   Materiales: %d",
                modelo->meshCount,
                modelo->materialCount
            ),
            25,
            135,
            18,
            BLACK
        );
    }

    DrawText(
        TextFormat(
            "Rotacion: %.1f   Escala: %.2f",
            rotacion,
            escala
        ),
        25,
        165,
        18,
        BLACK
    );

    if (modeloCargado)
    {
        DrawText(
            animacionIdleActiva
            ? TextFormat(
                "Idle activo: %s   Clips: %d",
                ObtenerNombreIdleModeloJugadorCompartido(),
                cantidadAnimaciones
            )
            : TextFormat(
                "Idle no encontrado   Clips: %d",
                cantidadAnimaciones
            ),
            25,
            195,
            18,
            animacionIdleActiva
            ? DARKGREEN
            : MAROON
        );
    }

    DrawText(
        "FLECHAS IZQ/DER: rotar   ARRIBA/ABAJO: escala",
        25,
        GetScreenHeight() - 80,
        18,
        BLACK
    );

    DrawText(
        "ESPACIO: rotacion automatica ON/OFF",
        25,
        GetScreenHeight() - 50,
        18,
        BLACK
    );
}


//==================================================
// DESCARGAR
//==================================================

void PruebaModelos::Descargar()
{
    // El modelo y sus animaciones pertenecen a ModeloJugadorCompartido.
    // PruebaModelos conserva solo estado local de inspeccion.
}
