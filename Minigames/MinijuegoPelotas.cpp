#include "Minigames/MinijuegoPelotas.h"

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_PELOTAS = 3.0f;
static const float DURACION_PARTIDA_PELOTAS = 60.0f;
static const float DURACION_TEXTO_YA_PELOTAS = 0.75f;


static void InicializarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarJugadoresVivosPelotas(
    const MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void FinalizarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const Participante participantes[]
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
        ContarJugadoresVivosPelotas(
            minijuego,
            participantes
        );

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        cantidadVivos == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(minijuego.tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorPelotas& estadoJugador =
            minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs =
                tiempoFinalMs;
        }

        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        resultadoJugador.posicionFinal =
            estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_PELOTAS_TERMINADO;
}


//==================================================
// INICIALIZAR
//==================================================

void MinijuegoPelotas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    fase = FASE_PELOTAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PELOTAS;
    tiempoRestante = DURACION_PARTIDA_PELOTAS;
    tiempoJugado = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

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

    fase = FASE_PELOTAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PELOTAS;
    tiempoRestante = DURACION_PARTIDA_PELOTAS;
    tiempoJugado = 0.0f;

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
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoPelotas(
            *this,
            participantes
        );
    }

    if (fase == FASE_PELOTAS_TERMINADO)
    {
        return;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PELOTAS_JUGANDO;
        }

        return;
    }

    tiempoRestante -= deltaTime;

    if (tiempoRestante < 0.0f)
    {
        tiempoRestante = 0.0f;
    }

    tiempoJugado =
        DURACION_PARTIDA_PELOTAS - tiempoRestante;

    int vivosAntes =
        ContarJugadoresVivosPelotas(
            *this,
            participantes
        );

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

        if (estadosJugadores[i].eliminado)
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            continue;
        }

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
            bloques,
            cantidadBloques,
            particulas,
            cantidadParticulas,
            false,
            true,
            false,
            deltaTime
        );
    }

    int eliminadosEsteFrame = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminadosEsteFrame++;
        }
    }

    int posicionEliminados =
        vivosAntes - eliminadosEsteFrame + 1;

    int tiempoSobrevividoMs =
        (int)std::lround(tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal =
                posicionEliminados;
            estadosJugadores[i].tiempoSobrevividoMs =
                tiempoSobrevividoMs;

            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }
    }

    ResolverColisionesPelotas(
        jugadores,
        participantes,
        cantidadMaxima
    );

    int vivosDespues =
        vivosAntes - eliminadosEsteFrame;

    if (vivosDespues <= 1 || tiempoRestante <= 0.0f)
    {
        FinalizarResultadoPelotas(
            *this,
            participantes
        );
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoPelotas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
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
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        DibujarJugadorPelotaPrueba(
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

    if (fase == FASE_PELOTAS_JUGANDO)
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoRestante),
            GetScreenWidth() - 210,
            26,
            26,
            tiempoRestante <= 10.0f ? RED : DARKBLUE
        );
    }

    int posicionY =
        168;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            estadosJugadores[i].eliminado
        )
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
                participantes[i].numeroJugador,
                velocidad
            ),
            25,
            posicionY,
            18,
            participantes[i].color
        );

        posicionY +=
            24;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        const char* texto = TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            ORANGE
        );
    }
    else if (
        fase == FASE_PELOTAS_JUGANDO &&
        tiempoJugado < DURACION_TEXTO_YA_PELOTAS
    )
    {
        const char* texto = "YA";

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            LIME
        );
    }
    else if (fase == FASE_PELOTAS_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 330,
            GetScreenHeight() / 2 - 155,
            660,
            310,
            Fade(BLACK, 0.90f)
        );

        int indicesGanadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(
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
                ? participantes[indicesGanadores[0]].numeroJugador
                : 0
            );

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 130,
            34,
            GOLD
        );

        int y = GetScreenHeight() / 2 - 75;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    resultado.participantes[i]
                        .puntuacionMinijuego / 1000.0f
                ),
                GetScreenWidth() / 2 - 210,
                y,
                22,
                participantes[i].color
            );

            y += 30;
        }

        const char* reiniciar = "R PARA REINICIAR";

        DrawText(
            reiniciar,
            GetScreenWidth() / 2 -
                MeasureText(reiniciar, 22) / 2,
            GetScreenHeight() / 2 + 112,
            22,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoPelotas::ObtenerResultado() const
{
    return resultado;
}
