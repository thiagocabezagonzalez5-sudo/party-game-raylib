#include "Minigames/MinijuegoColorSeguro.h"

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_COLOR = 3.0f;
static const float DURACION_PARTIDA_COLOR = 60.0f;
static const float DURACION_TEXTO_YA_COLOR = 0.75f;


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
// RESULTADO
//==================================================

static void InicializarResultadoColorSeguro(
    MinijuegoColorSeguro& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarJugadoresVivosColorSeguro(
    const MinijuegoColorSeguro& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int RegistrarEliminacionesColorSeguro(
    MinijuegoColorSeguro& minijuego,
    JugadorPrueba jugadores[],
    int vivosAntes
)
{
    int eliminadosEsteFrame = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminadosEsteFrame++;
        }
    }

    if (eliminadosEsteFrame == 0)
    {
        return 0;
    }

    int posicionEliminados =
        vivosAntes - eliminadosEsteFrame + 1;

    int tiempoSobrevividoMs =
        (int)std::lround(
            minijuego.tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            EstadoJugadorColorSeguro& estadoJugador =
                minijuego.estadosJugadores[i];

            estadoJugador.eliminado =
                true;

            estadoJugador.posicionFinal =
                posicionEliminados;

            estadoJugador.tiempoSobrevividoMs =
                tiempoSobrevividoMs;

            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }
    }

    return eliminadosEsteFrame;
}


static void RegistrarRondaSobrevividaColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            minijuego.estadosJugadores[i]
                .rondasSobrevividas++;
        }
    }
}


static void FinalizarResultadoColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int cantidadVivos =
        ContarJugadoresVivosColorSeguro(
            minijuego
        );

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        cantidadVivos == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(
            minijuego.tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorColorSeguro& estadoJugador =
            minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs =
                tiempoFinalMs;
        }

        resultadoJugador.posicionFinal =
            estadoJugador.posicionFinal;

        resultadoJugador.numeroEquipo = -1;

        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;

        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.estado =
        COLOR_SEGURO_TERMINADO;
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
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    estado =
        COLOR_SEGURO_PREPARACION;

    fase =
        FASE_ELEGIR_PLATAFORMA;

    tiempoPreparacion =
        DURACION_PREPARACION_COLOR;

    tiempoRestante =
        DURACION_PARTIDA_COLOR;

    tiempoJugado =
        0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

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
    bool participantesAnteriores[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantesAnteriores[i] =
            resultado.participantes[i].participo;
    }

    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo =
            participantesAnteriores[i];

        if (participantesAnteriores[i])
        {
            resultado.cantidadParticipantes++;
        }

        estadosJugadores[i] = {};
    }

    ReiniciarBloquesPrueba(
        plataformas,
        cantidadPlataformas
    );

    numeroRonda =
        1;

    estado =
        COLOR_SEGURO_PREPARACION;

    fase =
        FASE_ELEGIR_PLATAFORMA;

    indicePlataformaSegura =
        -1;

    tiempoPreparacion =
        DURACION_PREPARACION_COLOR;

    tiempoRestante =
        DURACION_PARTIDA_COLOR;

    tiempoJugado =
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
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoColorSeguro(
            *this,
            participantes
        );
    }

    if (
        estado ==
        COLOR_SEGURO_TERMINADO
    )
    {
        return;
    }

    if (
        estado ==
        COLOR_SEGURO_PREPARACION
    )
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -=
            deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion =
                0.0f;

            estado =
                COLOR_SEGURO_JUGANDO;
        }

        return;
    }

    tiempoRestante -=
        deltaTime;

    if (tiempoRestante < 0.0f)
    {
        tiempoRestante =
            0.0f;
    }

    tiempoJugado =
        DURACION_PARTIDA_COLOR -
        tiempoRestante;

    tiempoFase -=
        deltaTime;

    if (
        fase ==
        FASE_ELEGIR_PLATAFORMA &&
        tiempoFase <= 0.0f
    )
    {
        TirarPlataformasIncorrectas(
            *this
        );
    }

    if (
        fase ==
        FASE_CAIDA_PLATAFORMAS
    )
    {
        ActualizarCaidaPlataformas(
            *this,
            deltaTime
        );
    }

    int vivosAntes =
        ContarJugadoresVivosColorSeguro(
            *this
        );

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            jugador.cayendo =
                true;

            jugador.velocidad = {};
            jugador.empuje = {};

            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (
            participantes[i].activo &&
            participantes[i].conectado
        )
        {
            entrada =
                LeerInputMinijuegoParticipante(
                    participantes[i]
                );
        }

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

    int eliminadosEsteFrame =
        RegistrarEliminacionesColorSeguro(
            *this,
            jugadores,
            vivosAntes
        );

    int vivosDespues =
        vivosAntes - eliminadosEsteFrame;

    if (
        vivosDespues <= 1 ||
        tiempoRestante <= 0.0f
    )
    {
        FinalizarResultadoColorSeguro(
            *this
        );

        ActualizarTemblorCamara(
            *this,
            deltaTime
        );

        return;
    }

    if (
        fase ==
        FASE_CAIDA_PLATAFORMAS &&
        tiempoFase <= 0.0f
    )
    {
        RegistrarRondaSobrevividaColorSeguro(
            *this
        );

        ReiniciarBloquesPrueba(
            plataformas,
            cantidadPlataformas
        );

        numeroRonda++;

        ElegirNuevaPlataformaSegura(
            *this
        );
    }

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
    (void)cantidadMaxima;

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
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

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
            "RONDA: %d   TIEMPO TOTAL: %.1f",
            numeroRonda,
            tiempoRestante
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

    int posicionY =
        200;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const char* estadoJugador =
            estadosJugadores[i].eliminado
            ? "ELIMINADO"
            : (
                participantes[i].conectado
                ? "EN JUEGO"
                : "SIN CONTROL"
            );

        DrawText(
            TextFormat(
                "J%d %s  RONDAS %d",
                participantes[i].numeroJugador,
                estadoJugador,
                estadosJugadores[i].rondasSobrevividas
            ),
            25,
            posicionY,
            18,
            estadosJugadores[i].eliminado
            ? DARKGRAY
            : participantes[i].color
        );

        posicionY +=
            24;
    }

    if (estado == COLOR_SEGURO_PREPARACION)
    {
        int numero =
            (int)std::ceil(tiempoPreparacion);

        const char* texto =
            TextFormat(
                "%d",
                numero
            );

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            ORANGE
        );
    }
    else if (
        estado == COLOR_SEGURO_JUGANDO &&
        tiempoJugado < DURACION_TEXTO_YA_COLOR
    )
    {
        const char* texto =
            "YA";

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            LIME
        );
    }
    else if (
        estado ==
        COLOR_SEGURO_TERMINADO
    )
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 340,
            GetScreenHeight() / 2 - 170,
            680,
            340,
            Fade(BLACK, 0.90f)
        );

        int indicesGanadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores =
            ObtenerIndicesGanadores(
                resultado,
                indicesGanadores,
                MAX_PARTICIPANTES
            );

        const char* titulo =
            resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                ? participantes[indicesGanadores[0]]
                    .numeroJugador
                : 0
            );

        DrawText(
            titulo,
            GetScreenWidth() / 2 -
                MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 142,
            34,
            GOLD
        );

        int y =
            GetScreenHeight() / 2 - 88;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s  RONDAS %d",
                    participantes[i].numeroJugador,
                    resultado.participantes[i]
                        .posicionFinal,
                    estadosJugadores[i]
                        .tiempoSobrevividoMs /
                        1000.0f,
                    estadosJugadores[i]
                        .rondasSobrevividas
                ),
                GetScreenWidth() / 2 - 255,
                y,
                21,
                participantes[i].color
            );

            y +=
                31;
        }

        const char* reiniciar =
            "R PARA REINICIAR";

        DrawText(
            reiniciar,
            GetScreenWidth() / 2 -
                MeasureText(reiniciar, 22) / 2,
            GetScreenHeight() / 2 + 126,
            22,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoColorSeguro::ObtenerResultado() const
{
    return resultado;
}
