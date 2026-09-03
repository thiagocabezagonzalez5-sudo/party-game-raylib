#include "Minigames/MinijuegoColorSeguro.h"

#include "Minigames/UtilidadesMinijuegos.h"


//==================================================
// COLORES
//==================================================

static Color ObtenerColorArcoiris(
    int indice
)
{
    switch (indice)
    {
        case 0:
            return RED;

        case 1:
            return ORANGE;

        case 2:
            return YELLOW;

        case 3:
            return GREEN;

        case 4:
            return BLUE;

        case 5:
            return Color{
                75,
                0,
                130,
                255
            };

        case 6:
            return VIOLET;
    }

    return WHITE;
}


static const char* NombreColorPlataforma(
    int indice
)
{
    switch (indice)
    {
        case 0: return "ROJO";
        case 1: return "NARANJA";
        case 2: return "AMARILLO";
        case 3: return "VERDE";
        case 4: return "AZUL";
        case 5: return "INDIGO";
        case 6: return "VIOLETA";
    }

    return "?";
}


//==================================================
// TV PROVISIONAL
//==================================================

static void DibujarTelevisor(
    Color colorPantalla
)
{
    Vector3 cuerpo =
    {
        0.0f,
        4.2f,
        -8.2f
    };

    DrawCube(
        cuerpo,
        4.8f,
        3.1f,
        0.55f,
        Color{
            35,
            35,
            42,
            255
        }
    );

    DrawCubeWires(
        cuerpo,
        4.8f,
        3.1f,
        0.55f,
        BLACK
    );

    DrawCube(
        Vector3{
            0.0f,
            4.2f,
            -7.90f
        },
        4.05f,
        2.35f,
        0.08f,
        colorPantalla
    );

    DrawCubeWires(
        Vector3{
            0.0f,
            4.2f,
            -7.85f
        },
        4.10f,
        2.40f,
        0.05f,
        RAYWHITE
    );

    DrawCube(
        Vector3{
            -1.45f,
            2.0f,
            -8.2f
        },
        0.30f,
        1.45f,
        0.30f,
        DARKGRAY
    );

    DrawCube(
        Vector3{
            1.45f,
            2.0f,
            -8.2f
        },
        0.30f,
        1.45f,
        0.30f,
        DARKGRAY
    );

    DrawCube(
        Vector3{
            0.0f,
            1.25f,
            -8.2f
        },
        4.0f,
        0.25f,
        1.0f,
        DARKGRAY
    );
}


//==================================================
// RONDA
//==================================================

static void ElegirNuevaPlataformaSegura(
    MinijuegoColorSeguro& minijuego
)
{
    int anterior =
        minijuego.indicePlataformaSegura;

    if (minijuego.cantidadPlataformas <= 1)
    {
        minijuego.indicePlataformaSegura =
            0;
    }
    else
    {
        do
        {
            minijuego.indicePlataformaSegura =
                GetRandomValue(
                    0,
                    minijuego.cantidadPlataformas - 1
                );
        }
        while (
            minijuego.indicePlataformaSegura ==
            anterior
        );
    }

    minijuego.fase =
        FASE_ELEGIR_PLATAFORMA;

    minijuego.tiempoFase =
        minijuego.duracionElegirPlataforma;
}


static void TirarPlataformasIncorrectas(
    MinijuegoColorSeguro& minijuego
)
{
    for (
        int i = 0;
        i < minijuego.cantidadPlataformas;
        i++
    )
    {
        BloquePrueba& plataforma =
            minijuego.plataformas[i];

        if (
            i ==
            minijuego.indicePlataformaSegura
        )
        {
            plataforma.activaColision =
                true;

            plataforma.cayendo =
                false;

            continue;
        }

        plataforma.activaColision =
            false;

        plataforma.cayendo =
            true;

        plataforma.velocidadCaida =
            0.0f;
    }

    minijuego.fase =
        FASE_CAIDA_PLATAFORMAS;

    minijuego.tiempoFase =
        minijuego.duracionCaidaPlataformas;
}


static void ActualizarCaidaPlataformas(
    MinijuegoColorSeguro& minijuego,
    float deltaTime
)
{
    for (
        int i = 0;
        i < minijuego.cantidadPlataformas;
        i++
    )
    {
        BloquePrueba& plataforma =
            minijuego.plataformas[i];

        if (!plataforma.cayendo)
        {
            continue;
        }

        plataforma.velocidadCaida +=
            12.0f * deltaTime;

        plataforma.posicion.y -=
            plataforma.velocidadCaida *
            deltaTime;
    }
}


static void ActualizarTemblorCamara(
    MinijuegoColorSeguro& minijuego,
    float deltaTime
)
{
    if (minijuego.tiempoTemblorCamara > 0.0f)
    {
        minijuego.tiempoTemblorCamara -=
            deltaTime;

        float desplazamientoX =
            (
                (float)GetRandomValue(-1000, 1000) /
                1000.0f
            ) *
            minijuego.intensidadTemblorCamara;

        float desplazamientoY =
            (
                (float)GetRandomValue(-1000, 1000) /
                1000.0f
            ) *
            minijuego.intensidadTemblorCamara;

        minijuego.camara.position =
            minijuego.posicionCamaraBase;

        minijuego.camara.position.x +=
            desplazamientoX;

        minijuego.camara.position.y +=
            desplazamientoY;

        minijuego.camara.target =
            minijuego.objetivoCamaraBase;

        minijuego.camara.target.x +=
            desplazamientoX * 0.45f;

        minijuego.camara.target.y +=
            desplazamientoY * 0.45f;
    }
    else
    {
        minijuego.tiempoTemblorCamara =
            0.0f;

        minijuego.camara.position =
            minijuego.posicionCamaraBase;

        minijuego.camara.target =
            minijuego.objetivoCamaraBase;
    }
}


//==================================================
// INICIALIZAR
//==================================================

void MinijuegoColorSeguro::Inicializar()
{
    cantidadPlataformas =
        0;

    const float RADIO =
        4.2f;

    const float X_DIAGONAL =
        3.64f;

    const float Z_DIAGONAL =
        2.10f;

    Vector3 posiciones[CANTIDAD_PLATAFORMAS_COLOR] =
    {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -RADIO },
        { X_DIAGONAL, 0.0f, -Z_DIAGONAL },
        { X_DIAGONAL, 0.0f, Z_DIAGONAL },
        { 0.0f, 0.0f, RADIO },
        { -X_DIAGONAL, 0.0f, Z_DIAGONAL },
        { -X_DIAGONAL, 0.0f, -Z_DIAGONAL }
    };

    for (
        int i = 0;
        i < CANTIDAD_PLATAFORMAS_COLOR;
        i++
    )
    {
        AgregarBloquePrueba(
            plataformas,
            cantidadPlataformas,
            CANTIDAD_PLATAFORMAS_COLOR,
            posiciones[i],
            Vector3{
                2.8f,
                0.60f,
                2.8f
            },
            ObtenerColorArcoiris(i)
        );
    }

    fase =
        FASE_ELEGIR_PLATAFORMA;

    indicePlataformaSegura =
        -1;

    numeroRonda =
        1;

    tiempoFase =
        duracionElegirPlataforma;

    posicionCamaraBase =
    {
        0.0f,
        10.0f,
        15.5f
    };

    objetivoCamaraBase =
    {
        0.0f,
        0.8f,
        -1.5f
    };

    camara.position =
        posicionCamaraBase;

    camara.target =
        objetivoCamaraBase;

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy =
        55.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;

    tiempoTemblorCamara =
        0.0f;

    intensidadTemblorCamara =
        0.0f;

    ElegirNuevaPlataformaSegura(
        *this
    );

    tiempoTemblorCamara =
        0.0f;

    camara.position =
        posicionCamaraBase;

    camara.target =
        objetivoCamaraBase;
}


void MinijuegoColorSeguro::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -0.65f, 1.0f, 0.55f },
        { 0.65f, 1.0f, 0.55f },
        { -0.65f, 1.0f, -0.55f },
        { 0.65f, 1.0f, -0.55f }
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
        jugadores[i].posicionSpawn =
            spawns[i];

        jugadores[i].tamano =
        {
            0.8f,
            1.4f,
            0.8f
        };

        jugadores[i].velocidadMovimiento =
            5.0f;

        jugadores[i].fuerzaSalto =
            7.2f;

        jugadores[i].gravedad =
            18.0f;

        jugadores[i].duracionRespawn =
            1.2f;

        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


void MinijuegoColorSeguro::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    ReiniciarBloquesPrueba(
        plataformas,
        cantidadPlataformas
    );

    numeroRonda =
        1;

    indicePlataformaSegura =
        -1;

    ElegirNuevaPlataformaSegura(
        *this
    );

    tiempoTemblorCamara =
        0.0f;

    camara.position =
        posicionCamaraBase;

    camara.target =
        objetivoCamaraBase;

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

void MinijuegoColorSeguro::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    tiempoFase -=
        deltaTime;

    if (
        fase ==
        FASE_ELEGIR_PLATAFORMA
    )
    {
        if (tiempoFase <= 0.0f)
        {
            TirarPlataformasIncorrectas(
                *this
            );
        }
    }
    else
    {
        ActualizarCaidaPlataformas(
            *this,
            deltaTime
        );

        if (tiempoFase <= 0.0f)
        {
            ReiniciarBloquesPrueba(
                plataformas,
                cantidadPlataformas
            );

            numeroRonda++;

            ElegirNuevaPlataformaSegura(
                *this
            );

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
    }

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

        if (
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(
                participantes[i]
            );

        ActualizarJugadorPrueba(
            jugador,
            entrada,
            plataformas,
            cantidadPlataformas,
            particulas,
            cantidadParticulas,
            true,
            false,
            false,
            deltaTime
        );
    }

    bool huboImpactoGolpeSuelo =
        ResolverGolpesSuelo(
            jugadores,
            participantes,
            cantidadMaxima
        );

    if (huboImpactoGolpeSuelo)
    {
        tiempoTemblorCamara =
            0.24f;

        intensidadTemblorCamara =
            0.20f;
    }

    ResolverGolpesJugadores(
        jugadores,
        participantes,
        cantidadMaxima
    );

    ResolverColisionesJugadoresNormales(
        jugadores,
        participantes,
        cantidadMaxima
    );

    ActualizarTemblorCamara(
        *this,
        deltaTime
    );
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoColorSeguro::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
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

    for (
        int i = 0;
        i < cantidadPlataformas;
        i++
    )
    {
        const BloquePrueba& plataforma =
            plataformas[i];

        DrawCube(
            plataforma.posicion,
            plataforma.tamano.x,
            plataforma.tamano.y,
            plataforma.tamano.z,
            plataforma.color
        );

        DrawCubeWires(
            plataforma.posicion,
            plataforma.tamano.x,
            plataforma.tamano.y,
            plataforma.tamano.z,
            BLACK
        );

        if (mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxBloquePrueba(
                    plataforma
                ),
                YELLOW
            );
        }
    }

    Color colorPantalla =
        ObtenerColorArcoiris(
            indicePlataformaSegura
        );

    DibujarTelevisor(
        colorPantalla
    );

    DibujarParticulasTierra(
        particulas,
        cantidadParticulas
    );

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        DibujarJugadorCuboPrueba(
            jugadores[i],
            participantes[i]
        );

        if (
            mostrarDebug &&
            participantes[i].activo &&
            participantes[i].conectado &&
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
        "MINIJUEGO 1 - COLOR SEGURO",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        TextFormat(
            "COLOR: %s",
            NombreColorPlataforma(
                indicePlataformaSegura
            )
        ),
        25,
        70,
        26,
        colorPantalla
    );

    DrawText(
        TextFormat(
            "RONDA: %d   TIEMPO: %.1f",
            numeroRonda,
            tiempoFase > 0.0f
            ? tiempoFase
            : 0.0f
        ),
        25,
        105,
        22,
        BLACK
    );

    DrawText(
        fase == FASE_ELEGIR_PLATAFORMA
        ? "CORRE AL COLOR DE LA TV"
        : "SOLO QUEDA LA PLATAFORMA CORRECTA",
        25,
        138,
        20,
        BLACK
    );

    DrawText(
        "SALTO EN EL AIRE: GOLPE AL SUELO   E/SHIFT/B: GOLPEAR",
        25,
        168,
        18,
        DARKGRAY
    );
}
