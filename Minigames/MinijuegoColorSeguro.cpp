#include "Minigames/MinijuegoColorSeguro.h"

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


//==================================================
// COLORES
//==================================================

static Color ObtenerColorPlataforma(
    int indice
)
{
    // Centro blanco y seis colores alrededor,
    // siguiendo la distribucion visual de la referencia.
    switch (indice)
    {
        case 0:
            return RAYWHITE;

        case 1:
            return Color{ 242, 214, 74, 255 };

        case 2:
            return Color{ 226, 58, 55, 255 };

        case 3:
            return Color{ 74, 184, 92, 255 };

        case 4:
            return Color{ 228, 126, 177, 255 };

        case 5:
            return Color{ 78, 92, 201, 255 };

        case 6:
            return Color{ 71, 189, 205, 255 };
    }

    return WHITE;
}


static const char* NombreColorPlataforma(
    int indice
)
{
    switch (indice)
    {
        case 0: return "BLANCO";
        case 1: return "AMARILLO";
        case 2: return "ROJO";
        case 3: return "VERDE";
        case 4: return "ROSA";
        case 5: return "VIOLETA";
        case 6: return "CELESTE";
    }

    return "?";
}


//==================================================
// PLATAFORMA HEXAGONAL
//==================================================

static Color OscurecerColorPlataforma(
    Color color,
    float factor
)
{
    if (factor < 0.0f)
    {
        factor = 0.0f;
    }

    if (factor > 1.0f)
    {
        factor = 1.0f;
    }

    return Color{
        (unsigned char)((float)color.r * factor),
        (unsigned char)((float)color.g * factor),
        (unsigned char)((float)color.b * factor),
        color.a
    };
}


static void DibujarPlataformaHexagonal(
    const BloquePrueba& plataforma
)
{
    const float RADIO_VISUAL =
        2.35f;

    const float MITAD_ALTURA =
        plataforma.tamano.y / 2.0f;

    Vector3 verticesSuperiores[6]{};
    Vector3 verticesInferiores[6]{};

    for (int i = 0; i < 6; i++)
    {
        float angulo =
            (30.0f + 60.0f * (float)i) *
            DEG2RAD;

        float x =
            std::cos(angulo) *
            RADIO_VISUAL;

        float z =
            std::sin(angulo) *
            RADIO_VISUAL;

        verticesSuperiores[i] =
        {
            plataforma.posicion.x + x,
            plataforma.posicion.y + MITAD_ALTURA,
            plataforma.posicion.z + z
        };

        verticesInferiores[i] =
        {
            plataforma.posicion.x + x,
            plataforma.posicion.y - MITAD_ALTURA,
            plataforma.posicion.z + z
        };
    }

    Vector3 centroSuperior =
    {
        plataforma.posicion.x,
        plataforma.posicion.y + MITAD_ALTURA,
        plataforma.posicion.z
    };

    Vector3 centroInferior =
    {
        plataforma.posicion.x,
        plataforma.posicion.y - MITAD_ALTURA,
        plataforma.posicion.z
    };

    Color colorLateral =
        OscurecerColorPlataforma(
            plataforma.color,
            0.34f
        );

    Color colorLateralAlterno =
    {
        38,
        40,
        45,
        255
    };

    Color colorInferior =
        OscurecerColorPlataforma(
            plataforma.color,
            0.20f
        );

    for (int i = 0; i < 6; i++)
    {
        int siguiente =
            (i + 1) % 6;

        // IMPORTANTE:
        // El orden de los vertices deja la normal mirando
        // hacia arriba. Antes estaban invertidos y raylib
        // ocultaba la superficie por backface culling.
        DrawTriangle3D(
            centroSuperior,
            verticesSuperiores[siguiente],
            verticesSuperiores[i],
            plataforma.color
        );

        // Cara inferior, visible cuando una plataforma cae.
        DrawTriangle3D(
            centroInferior,
            verticesInferiores[i],
            verticesInferiores[siguiente],
            colorInferior
        );

        Color colorCara =
            i % 2 == 0
            ? colorLateral
            : colorLateralAlterno;

        // Dos triangulos con la normal mirando hacia afuera.
        DrawTriangle3D(
            verticesSuperiores[i],
            verticesSuperiores[siguiente],
            verticesInferiores[siguiente],
            colorCara
        );

        DrawTriangle3D(
            verticesSuperiores[i],
            verticesInferiores[siguiente],
            verticesInferiores[i],
            colorCara
        );

        DrawLine3D(
            verticesSuperiores[i],
            verticesSuperiores[siguiente],
            BLACK
        );

        DrawLine3D(
            verticesSuperiores[i],
            verticesInferiores[i],
            Fade(
                BLACK,
                0.72f
            )
        );
    }
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

    // Siete hexagonos regulares: uno central y seis vecinos.
    // Radio visual 2.35. Estas distancias hacen que los
    // lados queden encastrados sin separaciones grandes.
    const float RADIO_HEXAGONO =
        2.35f;

    const float DISTANCIA_HORIZONTAL =
        4.07032f;

    const float MEDIO_HORIZONTAL =
        DISTANCIA_HORIZONTAL / 2.0f;

    const float DISTANCIA_DIAGONAL_Z =
        RADIO_HEXAGONO * 1.5f;

    Vector3 posiciones[CANTIDAD_PLATAFORMAS_COLOR] =
    {
        // Centro blanco.
        { 0.0f, 0.0f, 0.0f },

        // Fila superior.
        {
            -MEDIO_HORIZONTAL,
            0.0f,
            -DISTANCIA_DIAGONAL_Z
        },
        {
            MEDIO_HORIZONTAL,
            0.0f,
            -DISTANCIA_DIAGONAL_Z
        },

        // Derecha.
        {
            DISTANCIA_HORIZONTAL,
            0.0f,
            0.0f
        },

        // Fila inferior.
        {
            MEDIO_HORIZONTAL,
            0.0f,
            DISTANCIA_DIAGONAL_Z
        },
        {
            -MEDIO_HORIZONTAL,
            0.0f,
            DISTANCIA_DIAGONAL_Z
        },

        // Izquierda.
        {
            -DISTANCIA_HORIZONTAL,
            0.0f,
            0.0f
        }
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

            // El alto vuelve a 0.60f. Con 0.72f los pies
            // del jugador aparecian dentro del AABB y la
            // deteccion de aterrizaje nunca se activaba.
            Vector3{
                4.10f,
                0.60f,
                3.65f
            },
            ObtenerColorPlataforma(i)
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
        11.4f,
        15.8f
    };

    objetivoCamaraBase =
    {
        0.0f,
        0.45f,
        -0.75f
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

        DibujarPlataformaHexagonal(
            plataforma
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
        ObtenerColorPlataforma(
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
