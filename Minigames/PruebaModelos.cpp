#include "Minigames/PruebaModelos.h"

#include "raymath.h"

#include <cstring>


//==================================================
// BUSCAR ANIMACION IDLE
//==================================================

static bool NombreEsIdle(
    const char* nombre
)
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

    if (modeloCargado)
    {
        return;
    }

    if (!FileExists(rutaModelo))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro el modelo de prueba: %s",
            rutaModelo
        );

        return;
    }

    modelo =
        LoadModel(
            rutaModelo
        );

    modeloCargado =
        modelo.meshCount > 0;

    if (modeloCargado)
    {
        // El GLB de Don Sahur fue exportado con el eje
        // vertical orientado sobre Z. El giro negativo lo
        // deja de pie y evita que quede bajo la plataforma.
        modelo.transform =
            MatrixRotateX(
                90.0f * DEG2RAD
            );

        animaciones =
            LoadModelAnimations(
                rutaModelo,
                &cantidadAnimaciones
            );

        indiceAnimacionIdle =
            -1;

        for (
            int i = 0;
            animaciones != nullptr &&
            i < cantidadAnimaciones;
            i++
        )
        {
            if (NombreEsIdle(animaciones[i].name))
            {
                indiceAnimacionIdle =
                    (int)i;

                break;
            }
        }

        // Si el archivo contiene una sola animacion,
        // se usa como Idle aunque el clip no tenga nombre.
        if (
            indiceAnimacionIdle < 0 &&
            animaciones != nullptr &&
            cantidadAnimaciones == 1
        )
        {
            indiceAnimacionIdle =
                0;
        }

        animacionIdleActiva =
            animaciones != nullptr &&
            indiceAnimacionIdle >= 0 &&
            IsModelAnimationValid(
                modelo,
                animaciones[indiceAnimacionIdle]
            ) &&
            animaciones[indiceAnimacionIdle]
                .keyframeCount > 0;
    }

    if (!modeloCargado)
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar el modelo de prueba"
        );
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

    if (animacionIdleActiva)
    {
        const float FOTOGRAMAS_POR_SEGUNDO =
            30.0f;

        fotogramaAnimacionIdle +=
            FOTOGRAMAS_POR_SEGUNDO *
            deltaTime;

        int cantidadFotogramas =
            animaciones[indiceAnimacionIdle]
                .keyframeCount;

        int fotogramaActual =
            (int)fotogramaAnimacionIdle %
            cantidadFotogramas;

        UpdateModelAnimation(
            modelo,
            animaciones[indiceAnimacionIdle],
            fotogramaActual
        );
    }
}


//==================================================
// DIBUJAR
//==================================================

void PruebaModelos::Dibujar() const
{
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
                modelo,
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
            rutaModelo
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
                modelo.meshCount,
                modelo.materialCount
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
                animaciones[indiceAnimacionIdle].name,
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
    if (animaciones != nullptr)
    {
        UnloadModelAnimations(
            animaciones,
            cantidadAnimaciones
        );

        animaciones =
            nullptr;

        cantidadAnimaciones =
            0;

        indiceAnimacionIdle =
            -1;

        animacionIdleActiva =
            false;
    }

    if (modeloCargado)
    {
        UnloadModel(
            modelo
        );

        modeloCargado =
            false;

        modelo = {};
    }
}
