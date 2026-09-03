#include "Gameplay/ZonaPruebas.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"


//==================================================
// NOMBRE DEL PROTOTIPO
//==================================================

static const char* NombreModoPrueba(
    ModoZonaPruebas modo
)
{
    switch (modo)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            return "1 - ZONA PRINCIPAL";

        case PRUEBA_COLOR_SEGURO:
            return "2 - COLOR SEGURO";

        case PRUEBA_PELOTAS_EMPUJON:
            return "3 - PELOTAS / EMPUJONES";

        case PRUEBA_MODELOS:
            return "4 - PRUEBA DE MODELOS";

        case PRUEBA_TRONCO_COORDINADO:
            return "5 - TRONCO COORDINADO";

        case PRUEBA_FABRICA_67:
            return "6 - FABRICA 67";

        case PRUEBA_TABLERO:
            return "7 - TABLERO";
    }

    return "PRUEBA";
}


//==================================================
// ZONA PRINCIPAL
//==================================================

static void ConfigurarZonaPrincipal(
    ZonaPruebas& zona
)
{
    zona.cantidadBloquesPrincipal =
        0;

    AgregarBloquePrueba(
        zona.bloquesPrincipal,
        zona.cantidadBloquesPrincipal,
        MAX_BLOQUES_PRUEBA,
        Vector3{
            0.0f,
            -0.5f,
            0.0f
        },
        Vector3{
            14.0f,
            1.0f,
            14.0f
        },
        Color{
            105,
            105,
            115,
            255
        }
    );

    AgregarBloquePrueba(
        zona.bloquesPrincipal,
        zona.cantidadBloquesPrincipal,
        MAX_BLOQUES_PRUEBA,
        Vector3{
            -3.0f,
            0.5f,
            -2.0f
        },
        Vector3{
            2.5f,
            1.0f,
            2.5f
        },
        ORANGE
    );

    AgregarBloquePrueba(
        zona.bloquesPrincipal,
        zona.cantidadBloquesPrincipal,
        MAX_BLOQUES_PRUEBA,
        Vector3{
            2.0f,
            1.0f,
            -1.0f
        },
        Vector3{
            3.0f,
            2.0f,
            3.0f
        },
        BLUE
    );

    zona.camaraPrincipal.position =
    {
        0.0f,
        6.5f,
        13.0f
    };

    zona.camaraPrincipal.target =
    {
        0.0f,
        1.0f,
        -1.0f
    };

    zona.camaraPrincipal.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    zona.camaraPrincipal.fovy =
        50.0f;

    zona.camaraPrincipal.projection =
        CAMERA_PERSPECTIVE;
}


static void ConfigurarJugadoresPrincipal(
    ZonaPruebas& zona
)
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -1.2f, 1.0f, 4.0f },
        { 1.2f, 1.0f, 4.0f },
        { -2.4f, 1.0f, 3.0f },
        { 2.4f, 1.0f, 3.0f }
    };

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        ConfigurarJugadorMinijuegoEstandar(
            zona.jugadores[i],
            spawns[i]
        );
    }
}


static void ActualizarZonaPrincipal(
    ZonaPruebas& zona,
    float deltaTime
)
{
    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        JugadorPrueba& jugador =
            zona.jugadores[i];

        const Participante& participante =
            zona.participantes[i];

        if (!participante.activo)
        {
            continue;
        }

        ActualizarJugadorMinijuegoEstandar(
            jugador,
            participante,
            zona.bloquesPrincipal,
            zona.cantidadBloquesPrincipal,
            zona.particulas,
            MAX_PARTICULAS_TIERRA,
            true,
            deltaTime
        );
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        zona.jugadores,
        zona.participantes,
        MAX_JUGADORES_PRUEBA,
        zona.particulas,
        MAX_PARTICULAS_TIERRA
    );
}


static void DibujarZonaPrincipal(
    const ZonaPruebas& zona
)
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
        zona.camaraPrincipal
    );

    for (
        int i = 0;
        i < zona.cantidadBloquesPrincipal;
        i++
    )
    {
        const BloquePrueba& bloque =
            zona.bloquesPrincipal[i];

        DrawCube(
            bloque.posicion,
            bloque.tamano.x,
            bloque.tamano.y,
            bloque.tamano.z,
            bloque.color
        );

        DrawCubeWires(
            bloque.posicion,
            bloque.tamano.x,
            bloque.tamano.y,
            bloque.tamano.z,
            BLACK
        );

        if (zona.mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxBloquePrueba(
                    bloque
                ),
                YELLOW
            );
        }
    }

    DibujarParticulasTierra(
        zona.particulas,
        MAX_PARTICULAS_TIERRA
    );

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        DibujarJugadorCuboPrueba(
            zona.jugadores[i],
            zona.participantes[i]
        );

        if (
            zona.mostrarDebug &&
            zona.participantes[i].activo &&
            zona.participantes[i].conectado &&
            !zona.jugadores[i].cayendo
        )
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(
                    zona.jugadores[i]
                ),
                LIME
            );
        }
    }

    DrawGrid(
        30,
        1.0f
    );

    EndMode3D();

    DrawText(
        "ZONA PRINCIPAL DE PRUEBAS",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        "MOVIMIENTO + SALTO + GOLPES + COLISION SOLIDA",
        25,
        70,
        20,
        BLACK
    );

    DrawText(
        "SALTO EN EL AIRE: GOLPE AL SUELO   E/SHIFT/B: GOLPEAR",
        25,
        100,
        18,
        DARKGRAY
    );
}


//==================================================
// INICIALIZAR
//==================================================

void ZonaPruebas::Inicializar(
    Participante participantesJuego[],
    int cantidadParticipantesJuego
)
{
    volverAlMenu =
        false;

    mostrarDebug =
        false;

    participantes =
        participantesJuego;

    cantidadParticipantes =
        cantidadParticipantesJuego;

    for (
        int i = 0;
        i < MAX_PARTICULAS_TIERRA;
        i++
    )
    {
        particulas[i].activa =
            false;
    }

    ConfigurarZonaPrincipal(
        *this
    );

    minijuegoColor.Inicializar();
    minijuegoPelotas.Inicializar();
    minijuegoTronco.Inicializar();
    minijuego67.Inicializar();

    prototipoTablero.Inicializar(
        participantes,
        cantidadParticipantes
    );

    CambiarModo(
        PRUEBA_ZONA_PRINCIPAL
    );
}


//==================================================
// CAMBIAR PROTOTIPO
//==================================================

void ZonaPruebas::CambiarModo(
    ModoZonaPruebas nuevoModo
)
{
    modoActual =
        nuevoModo;

    for (
        int i = 0;
        i < MAX_PARTICULAS_TIERRA;
        i++
    )
    {
        particulas[i].activa =
            false;
    }

    if (
        modoActual ==
        PRUEBA_ZONA_PRINCIPAL
    )
    {
        ConfigurarZonaPrincipal(
            *this
        );

        ConfigurarJugadoresPrincipal(
            *this
        );
    }
    else if (
        modoActual ==
        PRUEBA_COLOR_SEGURO
    )
    {
        minijuegoColor.Inicializar();

        minijuegoColor.ConfigurarJugadores(
            jugadores,
            MAX_JUGADORES_PRUEBA
        );
    }
    else if (
        modoActual ==
        PRUEBA_PELOTAS_EMPUJON
    )
    {
        minijuegoPelotas.Inicializar();

        minijuegoPelotas.ConfigurarJugadores(
            jugadores,
            MAX_JUGADORES_PRUEBA
        );
    }
    else if (
        modoActual ==
        PRUEBA_MODELOS
    )
    {
        pruebaModelos.Inicializar();
    }
    else if (
        modoActual ==
        PRUEBA_TRONCO_COORDINADO
    )
    {
        minijuegoTronco.Inicializar();

        minijuegoTronco.ConfigurarJugadores(
            jugadores,
            MAX_JUGADORES_PRUEBA
        );
    }
    else if (
        modoActual ==
        PRUEBA_FABRICA_67
    )
    {
        minijuego67.Inicializar();
    }
    else if (
        modoActual ==
        PRUEBA_TABLERO
    )
    {
        prototipoTablero.Reiniciar();
    }

    if (
        modoActual !=
        PRUEBA_MODELOS
    )
    {
        ReiniciarJugadores();
    }

    if (
        modoActual ==
        PRUEBA_TRONCO_COORDINADO
    )
    {
        minijuegoTronco.Reiniciar(
            participantes,
            MAX_JUGADORES_PRUEBA
        );
    }

    if (
        modoActual ==
        PRUEBA_FABRICA_67
    )
    {
        minijuego67.Reiniciar(
            participantes,
            MAX_JUGADORES_PRUEBA
        );
    }
}


//==================================================
// REINICIAR
//==================================================

void ZonaPruebas::ReiniciarJugador(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= MAX_JUGADORES_PRUEBA
    )
    {
        return;
    }

    ReiniciarJugadorPrueba(
        jugadores[indice]
    );
}


void ZonaPruebas::ReiniciarJugadores()
{
    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        ReiniciarJugador(i);
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void ZonaPruebas::Actualizar(
    float deltaTime
)
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        volverAlMenu =
            true;

        return;
    }

    if (IsKeyPressed(KEY_F3))
    {
        mostrarDebug =
            !mostrarDebug;
    }

    if (IsKeyPressed(KEY_ONE))
    {
        CambiarModo(
            PRUEBA_ZONA_PRINCIPAL
        );

        return;
    }

    if (IsKeyPressed(KEY_TWO))
    {
        CambiarModo(
            PRUEBA_COLOR_SEGURO
        );

        return;
    }

    if (IsKeyPressed(KEY_THREE))
    {
        CambiarModo(
            PRUEBA_PELOTAS_EMPUJON
        );

        return;
    }

    if (IsKeyPressed(KEY_FOUR))
    {
        CambiarModo(
            PRUEBA_MODELOS
        );

        return;
    }

    if (IsKeyPressed(KEY_FIVE))
    {
        CambiarModo(
            PRUEBA_TRONCO_COORDINADO
        );

        return;
    }

    if (IsKeyPressed(KEY_SIX))
    {
        CambiarModo(
            PRUEBA_FABRICA_67
        );

        return;
    }

    if (IsKeyPressed(KEY_SEVEN))
    {
        CambiarModo(
            PRUEBA_TABLERO
        );

        return;
    }

    if (IsKeyPressed(KEY_R))
    {
        if (
            modoActual ==
            PRUEBA_ZONA_PRINCIPAL
        )
        {
            ConfigurarJugadoresPrincipal(
                *this
            );
        }
        else if (
            modoActual ==
            PRUEBA_COLOR_SEGURO
        )
        {
            minijuegoColor.Reiniciar(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
        }
        else if (
            modoActual ==
            PRUEBA_PELOTAS_EMPUJON
        )
        {
            minijuegoPelotas.Reiniciar(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
        }
        else if (
            modoActual ==
            PRUEBA_MODELOS
        )
        {
            pruebaModelos.Reiniciar();
        }
        else if (
            modoActual ==
            PRUEBA_TRONCO_COORDINADO
        )
        {
            minijuegoTronco.Reiniciar(
                participantes,
                MAX_JUGADORES_PRUEBA
            );
        }
        else if (
            modoActual ==
            PRUEBA_FABRICA_67
        )
        {
            minijuego67.Reiniciar(
                participantes,
                MAX_JUGADORES_PRUEBA
            );
        }
        else if (
            modoActual ==
            PRUEBA_TABLERO
        )
        {
            prototipoTablero.Reiniciar();
        }

        return;
    }

    ActualizarParticulasTierra(
        particulas,
        MAX_PARTICULAS_TIERRA,
        deltaTime
    );

    if (
        modoActual !=
        PRUEBA_MODELOS
    )
    {
        for (
            int i = 0;
            i < MAX_PARTICIPANTES;
            i++
        )
        {
            bool estabaConectado =
                participantes[i].conectado;

            ActualizarConexionParticipante(
                participantes[i]
            );

            if (
                !participantes[i].activo ||
                participantes[i].conectado == estabaConectado
            )
            {
                continue;
            }

            if (participantes[i].conectado)
            {
                ReiniciarJugadorPrueba(
                    jugadores[i]
                );
            }
            else
            {
                jugadores[i].velocidad = {};
                jugadores[i].empuje = {};
                jugadores[i].cayendo = false;
                jugadores[i].enSuelo = false;
            }
        }
    }

    if (
        modoActual ==
        PRUEBA_ZONA_PRINCIPAL
    )
    {
        ActualizarZonaPrincipal(
            *this,
            deltaTime
        );
    }
    else if (
        modoActual ==
        PRUEBA_COLOR_SEGURO
    )
    {
        minijuegoColor.Actualizar(
            deltaTime,
            jugadores,
            MAX_JUGADORES_PRUEBA,
            participantes,
            particulas,
            MAX_PARTICULAS_TIERRA
        );
    }
    else if (
        modoActual ==
        PRUEBA_PELOTAS_EMPUJON
    )
    {
        minijuegoPelotas.Actualizar(
            deltaTime,
            jugadores,
            MAX_JUGADORES_PRUEBA,
            participantes,
            particulas,
            MAX_PARTICULAS_TIERRA
        );
    }
    else if (
        modoActual ==
        PRUEBA_MODELOS
    )
    {
        pruebaModelos.Actualizar(
            deltaTime
        );
    }
    else if (
        modoActual ==
        PRUEBA_TRONCO_COORDINADO
    )
    {
        minijuegoTronco.Actualizar(
            deltaTime,
            MAX_JUGADORES_PRUEBA,
            participantes
        );
    }
    else if (
        modoActual ==
        PRUEBA_FABRICA_67
    )
    {
        minijuego67.Actualizar(
            deltaTime,
            MAX_JUGADORES_PRUEBA,
            participantes
        );
    }
    else if (
        modoActual ==
        PRUEBA_TABLERO
    )
    {
        prototipoTablero.Actualizar(
            deltaTime
        );
    }
}


//==================================================
// DIBUJAR
//==================================================

void ZonaPruebas::Dibujar() const
{
    if (
        modoActual ==
        PRUEBA_ZONA_PRINCIPAL
    )
    {
        DibujarZonaPrincipal(
            *this
        );
    }
    else if (
        modoActual ==
        PRUEBA_COLOR_SEGURO
    )
    {
        minijuegoColor.Dibujar(
            jugadores,
            MAX_JUGADORES_PRUEBA,
            participantes,
            particulas,
            MAX_PARTICULAS_TIERRA,
            mostrarDebug
        );
    }
    else if (
        modoActual ==
        PRUEBA_PELOTAS_EMPUJON
    )
    {
        minijuegoPelotas.Dibujar(
            jugadores,
            MAX_JUGADORES_PRUEBA,
            participantes,
            mostrarDebug
        );
    }
    else if (
        modoActual ==
        PRUEBA_MODELOS
    )
    {
        pruebaModelos.Dibujar();
    }
    else if (
        modoActual ==
        PRUEBA_TRONCO_COORDINADO
    )
    {
        minijuegoTronco.Dibujar(
            jugadores,
            MAX_JUGADORES_PRUEBA,
            participantes
        );
    }
    else if (
        modoActual ==
        PRUEBA_FABRICA_67
    )
    {
        minijuego67.Dibujar(
            MAX_JUGADORES_PRUEBA,
            participantes
        );
    }
    else if (
        modoActual ==
        PRUEBA_TABLERO
    )
    {
        prototipoTablero.Dibujar(
            mostrarDebug
        );
    }

    DrawRectangle(
        18,
        GetScreenHeight() - 148,
        1180,
        126,
        Fade(
            RAYWHITE,
            0.86f
        )
    );

    DrawText(
        "1 PRINCIPAL  2 COLOR  3 PELOTAS  4 MODELOS  5 TRONCO  6 FABRICA  7 TABLERO",
        30,
        GetScreenHeight() - 133,
        20,
        BLACK
    );

    DrawText(
        TextFormat(
            "ACTUAL: %s",
            NombreModoPrueba(
                modoActual
            )
        ),
        30,
        GetScreenHeight() - 101,
        20,
        DARKBLUE
    );

    DrawText(
        "R REINICIAR   F3 DEBUG   ESC MENU",
        30,
        GetScreenHeight() - 69,
        18,
        DARKGRAY
    );

    if (
        modoActual !=
        PRUEBA_MODELOS
    )
    {
        DrawText(
            TextFormat(
                "JUGADORES ACTIVOS: %d / 4",
                cantidadParticipantes
            ),
            30,
            GetScreenHeight() - 42,
            18,
            DARKGREEN
        );
    }
}


//==================================================
// DESCARGAR
//==================================================

void ZonaPruebas::Descargar()
{
    minijuego67.Descargar();
    pruebaModelos.Descargar();
}
