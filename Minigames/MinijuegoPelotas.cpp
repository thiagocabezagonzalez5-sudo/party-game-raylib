#include "Minigames/MinijuegoPelotas.h"

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


//==================================================
// INICIALIZAR
//==================================================

void MinijuegoPelotas::Inicializar()
{
    cantidadBloques =
        0;

    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        1,
        Vector3{
            0.0f,
            -0.5f,
            0.0f
        },
        Vector3{
            11.0f,
            1.0f,
            11.0f
        },
        Color{
            74,
            78,
            92,
            255
        }
    );

    camara.position =
    {
        0.0f,
        8.0f,
        13.0f
    };

    camara.target =
    {
        0.0f,
        0.5f,
        0.0f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy =
        52.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;
}


void MinijuegoPelotas::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Color colores[MAX_JUGADORES_PRUEBA] =
    {
        RED,
        BLUE,
        GREEN,
        GOLD
    };

    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.2f, 0.65f, 2.2f },
        { 2.2f, 0.65f, 2.2f },
        { -2.2f, 0.65f, -2.2f },
        { 2.2f, 0.65f, -2.2f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (
        int i = 0;
        i < limite;
        i++
    )
    {
        jugadores[i].numero =
            i + 1;

        jugadores[i].color =
            colores[i];

        jugadores[i].posicionSpawn =
            spawns[i];

        jugadores[i].tamano =
        {
            1.30f,
            1.30f,
            1.30f
        };

        jugadores[i].velocidadMovimiento =
            16.0f;

        jugadores[i].fuerzaSalto =
            0.0f;

        jugadores[i].gravedad =
            18.0f;

        jugadores[i].duracionRespawn =
            1.2f;

        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


void MinijuegoPelotas::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoPelotas::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    ModoTeclado modoTeclado,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

        if (!jugador.activo)
        {
            continue;
        }

        EntradaJugadorPrueba entrada =
            LeerEntradaJugadorPrueba(
                i,
                jugador,
                modoTeclado
            );

        ActualizarJugadorPrueba(
            jugador,
            entrada,
            bloques,
            cantidadBloques,
            particulas,
            cantidadParticulas,
            false,
            true,
            true,
            deltaTime
        );
    }

    ResolverColisionesPelotas(
        jugadores,
        cantidadMaxima
    );
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoPelotas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    bool mostrarDebug
) const
{
    ClearBackground(
        Color{
            125,
            190,
            220,
            255
        }
    );

    BeginMode3D(
        camara
    );

    DrawCube(
        bloques[0].posicion,
        bloques[0].tamano.x,
        bloques[0].tamano.y,
        bloques[0].tamano.z,
        bloques[0].color
    );

    DrawCubeWires(
        bloques[0].posicion,
        bloques[0].tamano.x,
        bloques[0].tamano.y,
        bloques[0].tamano.z,
        BLACK
    );

    DrawCubeWires(
        Vector3{
            0.0f,
            0.03f,
            0.0f
        },
        10.0f,
        0.04f,
        10.0f,
        RAYWHITE
    );

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        DibujarJugadorPelotaPrueba(
            jugadores[i]
        );

        if (
            mostrarDebug &&
            jugadores[i].activo &&
            !jugadores[i].cayendo
        )
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(
                    jugadores[i]
                ),
                LIME
            );
        }
    }

    EndMode3D();

    DrawText(
        "MINIJUEGO 2 - PELOTAS / EMPUJONES",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        "EMPUJA A LOS DEMAS FUERA DE LA ARENA",
        25,
        70,
        22,
        BLACK
    );

    DrawText(
        "PISTA DE HIELO - ACELERA Y CONSERVA LA INERCIA",
        25,
        102,
        20,
        BLACK
    );

    DrawText(
        "MAS VELOCIDAD = MAS FUERZA DE EMPUJE",
        25,
        132,
        20,
        MAROON
    );

    int posicionY =
        168;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (!jugadores[i].activo)
        {
            continue;
        }

        float velocidad =
            sqrtf(
                jugadores[i].velocidad.x *
                jugadores[i].velocidad.x +
                jugadores[i].velocidad.z *
                jugadores[i].velocidad.z
            );

        DrawText(
            TextFormat(
                "J%d VELOCIDAD: %.1f",
                jugadores[i].numero,
                velocidad
            ),
            25,
            posicionY,
            18,
            jugadores[i].color
        );

        posicionY +=
            24;
    }
}
