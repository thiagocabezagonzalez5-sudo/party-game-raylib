#include "Minigames/PruebaModelos.h"
#include "Minigames/ModeloJugadorCompartido.h"

#include "raymath.h"

#include <cmath>


// Vitrina de los cuatro personajes de bloques y de sus animaciones
// procedurales (Zona de Pruebas). Cada animacion se repite en bucle.

static const char* NOMBRES_ANIMACION_PRUEBA[CANTIDAD_ANIMACIONES_PRUEBA_MODELOS] =
{
    "REPOSO",
    "CAMINAR",
    "GOLPE",
    "CAIDA Y LEVANTARSE",
    "GOLPE LEVE (TAMBALEO)",
    "SALTO"
};


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
}


void PruebaModelos::Reiniciar()
{
    rotacion =
        0.0f;

    escala =
        ESCALA_MODELO_JUGADOR_3D * 1.6f;

    rotacionAutomatica =
        true;

    tiempo =
        0.0f;

    animacion =
        ANIMACION_PRUEBA_REPOSO;
}


//==================================================
// ACTUALIZAR
//==================================================

void PruebaModelos::Actualizar(
    float deltaTime
)
{
    tiempo +=
        deltaTime;

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

    escala =
        Clamp(escala, 0.10f, 1.20f);

    if (IsKeyPressed(KEY_SPACE))
    {
        rotacionAutomatica =
            !rotacionAutomatica;
    }

    // 1..6 eligen la animacion; TAB pasa a la siguiente.
    for (int i = 0; i < CANTIDAD_ANIMACIONES_PRUEBA_MODELOS; i++)
    {
        if (IsKeyPressed(KEY_ONE + i))
        {
            animacion = (AnimacionPruebaModelos)i;
            tiempo = 0.0f;
        }
    }

    if (IsKeyPressed(KEY_TAB))
    {
        animacion = (AnimacionPruebaModelos)((animacion + 1) % CANTIDAD_ANIMACIONES_PRUEBA_MODELOS);
        tiempo = 0.0f;
    }

    if (rotacionAutomatica)
    {
        rotacion +=
            28.0f * deltaTime;
    }
}


//==================================================
// DIBUJAR
//==================================================

// Pose de la animacion elegida en el instante "t" (bucles de 1.6 s).
static PosePersonajeBloque PoseVitrina(
    AnimacionPruebaModelos animacion,
    float t,
    float desfase
)
{
    PosePersonajeBloque pose;
    pose.tiempo = t + desfase;

    const float CICLO = 1.6f;
    float u = std::fmod(t, CICLO) / CICLO;

    switch (animacion)
    {
        case ANIMACION_PRUEBA_CAMINAR:
            pose.caminar = 1.0f;
            pose.faseCaminar = pose.tiempo * 11.0f;
            break;

        case ANIMACION_PRUEBA_GOLPE:
            pose.golpe = u < 0.5f ? u / 0.5f : -1.0f;
            break;

        case ANIMACION_PRUEBA_CAIDA:
            pose.aturdido = u > 0.1f && u < 0.85f;
            pose.caida =
                u < 0.10f ? u / 0.10f :
                u < 0.65f ? 1.0f :
                u < 0.85f ? 1.0f - (u - 0.65f) / 0.20f :
                0.0f;
            break;

        case ANIMACION_PRUEBA_TAMBALEO:
            pose.aturdido = u < 0.6f;
            pose.tambaleo =
                u < 0.08f ? u / 0.08f :
                u < 0.60f ? 1.0f - (u - 0.08f) / 0.52f :
                0.0f;
            break;

        case ANIMACION_PRUEBA_SALTO:
            pose.enAire = u < 0.55f;
            break;

        default:
            break;
    }

    return pose;
}


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

    Color colores[4] =
    {
        RED,
        BLUE,
        GREEN,
        GOLD
    };

    for (
        int i = 0;
        i < CANTIDAD_PERSONAJES_BLOQUE;
        i++
    )
    {
        Vector3 pedestal = { -3.0f + 2.0f * i, 0.0f, 0.0f };

        DrawCylinder(
            Vector3{
                pedestal.x,
                0.12f,
                pedestal.z
            },
            0.75f,
            0.75f,
            0.24f,
            24,
            DARKGRAY
        );

        PosePersonajeBloque pose =
            PoseVitrina(animacion, tiempo, i * 0.37f);

        float salto = 0.0f;

        if (pose.enAire)
        {
            float u = std::fmod(tiempo, 1.6f) / (1.6f * 0.55f);
            salto = 4.0f * u * (1.0f - u) * 0.8f;
        }

        DibujarPersonajeBloque(
            i,
            { pedestal.x, 0.24f + salto, pedestal.z },
            rotacion,
            escala,
            colores[i],
            pose
        );
    }

    EndMode3D();

    DrawText(
        "PRUEBA DE PERSONAJES",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        TextFormat(
            "Animacion: %s",
            NOMBRES_ANIMACION_PRUEBA[animacion]
        ),
        25,
        70,
        22,
        DARKGREEN
    );

    for (int i = 0; i < CANTIDAD_PERSONAJES_BLOQUE; i++)
    {
        DrawText(
            TextFormat(
                "%s - %s",
                ObtenerNombrePersonajeBloque(i),
                ObtenerLemaPersonajeBloque(i)
            ),
            25,
            105 + 26 * i,
            18,
            BLACK
        );
    }

    DrawText(
        TextFormat(
            "Rotación: %.1f   Escala: %.2f",
            rotacion,
            escala
        ),
        25,
        215,
        18,
        BLACK
    );

    DrawText(
        "1-6 / TAB: animación   FLECHAS IZQ/DER: rotar   ARRIBA/ABAJO: escala",
        25,
        GetScreenHeight() - 80,
        18,
        BLACK
    );

    DrawText(
        "ESPACIO: rotación automática ON/OFF",
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
    // Los personajes son procedurales: esta prueba no carga recursos y
    // solo guarda estado local de inspeccion. Se conserva por simetria con
    // el resto de pruebas de ZonaPruebas.
}
