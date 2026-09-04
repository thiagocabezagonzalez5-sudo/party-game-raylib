#include "Minigames/MinijuegoCapitanManda.h"

#include <cmath>


static const float DURACION_PREPARACION_CAPITAN = 3.0f;
static const float DURACION_MOSTRAR_ORDEN = 0.72f;
static const float DURACION_RESOLUCION = 0.85f;
static const int MAX_RONDAS_CAPITAN = 20;


static int ContarVivosCapitan(
    const MinijuegoCapitanManda& minijuego
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static bool TodosLosVivosRespondieron(
    const MinijuegoCapitanManda& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado &&
            !minijuego.jugadores[i].respondio
        )
        {
            return false;
        }
    }

    return true;
}


static float ObtenerTiempoRespuestaRonda(
    int numeroRonda
)
{
    float tiempo =
        1.55f -
        (numeroRonda - 1) * 0.075f;

    if (tiempo < 0.60f)
    {
        tiempo = 0.60f;
    }

    return tiempo;
}


static void PrepararNuevaRonda(
    MinijuegoCapitanManda& minijuego
)
{
    minijuego.numeroRonda++;

    minijuego.ordenActual =
        GetRandomValue(0, 1) == 0
        ? CONTROL_DIRECCION_IZQUIERDA
        : CONTROL_DIRECCION_DERECHA;

    minijuego.tiempoRespuesta =
        ObtenerTiempoRespuestaRonda(
            minijuego.numeroRonda
        );

    minijuego.tiempoFase =
        DURACION_MOSTRAR_ORDEN;

    minijuego.fase =
        FASE_CAPITAN_MOSTRANDO;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.jugadores[i].eliminado)
        {
            continue;
        }

        minijuego.jugadores[i].respondio = false;
        minijuego.jugadores[i].acerto = false;
        minijuego.jugadores[i].tiempoFeedback = 0.0f;
    }
}


static void FinalizarCapitan(
    MinijuegoCapitanManda& minijuego
)
{
    int vivos =
        ContarVivosCapitan(minijuego);

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        vivos == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorCapitanManda& estadoJugador =
            minijuego.jugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
        }

        resultadoJugador.posicionFinal =
            estadoJugador.posicionFinal;

        resultadoJugador.numeroEquipo = -1;

        resultadoJugador.puntuacionMinijuego =
            estadoJugador.rondasSuperadas * 1000 +
            (!estadoJugador.eliminado ? 500 : 0);

        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase =
        FASE_CAPITAN_TERMINADO;
}


static void ResolverRonda(
    MinijuegoCapitanManda& minijuego
)
{
    int vivosAntes =
        ContarVivosCapitan(minijuego);

    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !minijuego.resultado.participantes[i].participo ||
            minijuego.jugadores[i].eliminado
        )
        {
            continue;
        }

        if (
            !minijuego.jugadores[i].respondio ||
            !minijuego.jugadores[i].acerto
        )
        {
            eliminados++;
        }
    }

    int posicionEliminados =
        vivosAntes - eliminados + 1;

    if (posicionEliminados < 1)
    {
        posicionEliminados = 1;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCapitanManda& jugador =
            minijuego.jugadores[i];

        if (
            !minijuego.resultado.participantes[i].participo ||
            jugador.eliminado
        )
        {
            continue;
        }

        if (!jugador.respondio || !jugador.acerto)
        {
            jugador.eliminado = true;
            jugador.posicionFinal =
                posicionEliminados;
            jugador.tiempoFeedback =
                DURACION_RESOLUCION;
        }
        else
        {
            jugador.rondasSuperadas++;
            jugador.tiempoFeedback =
                DURACION_RESOLUCION;
        }
    }

    int vivosDespues =
        ContarVivosCapitan(minijuego);

    if (
        vivosDespues <= 1 ||
        minijuego.numeroRonda >= MAX_RONDAS_CAPITAN
    )
    {
        FinalizarCapitan(minijuego);
        return;
    }

    minijuego.fase =
        FASE_CAPITAN_RESOLVIENDO;

    minijuego.tiempoFase =
        DURACION_RESOLUCION;
}


void MinijuegoCapitanManda::Inicializar()
{
    resultado = {};
    resultado.formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    fase =
        FASE_CAPITAN_PREPARACION;

    ordenActual =
        CONTROL_DIRECCION_IZQUIERDA;

    numeroRonda = 0;

    tiempoPreparacion =
        DURACION_PREPARACION_CAPITAN;

    tiempoFase = 0.0f;
    tiempoRespuesta = 1.55f;

    resultadoInicializado = false;
}


void MinijuegoCapitanManda::Reiniciar(
    const Participante participantes[]
)
{
    Inicializar();

    if (participantes != nullptr)
    {
        InicializarResultadoMinijuego(
            resultado,
            participantes,
            FORMATO_MINIJUEGO_INDIVIDUAL
        );

        resultadoInicializado = true;
    }
}


void MinijuegoCapitanManda::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    if (participantes == nullptr)
    {
        return;
    }

    if (!resultadoInicializado)
    {
        InicializarResultadoMinijuego(
            resultado,
            participantes,
            FORMATO_MINIJUEGO_INDIVIDUAL
        );

        resultadoInicializado = true;
    }

    if (fase == FASE_CAPITAN_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CAPITAN_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            PrepararNuevaRonda(*this);
        }

        return;
    }

    if (fase == FASE_CAPITAN_MOSTRANDO)
    {
        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            tiempoFase = tiempoRespuesta;
            fase = FASE_CAPITAN_RESPONDIENDO;
        }

        return;
    }

    if (fase == FASE_CAPITAN_RESPONDIENDO)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                !resultado.participantes[i].participo ||
                jugadores[i].eliminado ||
                jugadores[i].respondio ||
                !participantes[i].conectado
            )
            {
                continue;
            }

            bool izquierda =
                AccionDireccionalControlPresionada(
                    participantes[i],
                    CONTROL_DIRECCION_IZQUIERDA
                );

            bool derecha =
                AccionDireccionalControlPresionada(
                    participantes[i],
                    CONTROL_DIRECCION_DERECHA
                );

            if (!izquierda && !derecha)
            {
                continue;
            }

            jugadores[i].respondio = true;

            if (
                ordenActual == CONTROL_DIRECCION_IZQUIERDA
            )
            {
                jugadores[i].acerto =
                    izquierda && !derecha;
            }
            else
            {
                jugadores[i].acerto =
                    derecha && !izquierda;
            }
        }

        tiempoFase -= deltaTime;

        if (
            tiempoFase <= 0.0f ||
            TodosLosVivosRespondieron(*this)
        )
        {
            ResolverRonda(*this);
        }

        return;
    }

    if (fase == FASE_CAPITAN_RESOLVIENDO)
    {
        tiempoFase -= deltaTime;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (jugadores[i].tiempoFeedback > 0.0f)
            {
                jugadores[i].tiempoFeedback -= deltaTime;
            }
        }

        if (tiempoFase <= 0.0f)
        {
            PrepararNuevaRonda(*this);
        }
    }
}


static void DibujarCapitan3D(
    AccionDireccionalControl orden,
    FaseCapitanManda fase
)
{
    DrawCube(
        Vector3{ 0.0f, 1.15f, -1.8f },
        1.15f,
        1.45f,
        0.75f,
        Color{ 64, 78, 104, 255 }
    );

    DrawSphere(
        Vector3{ 0.0f, 2.15f, -1.8f },
        0.48f,
        Color{ 232, 194, 142, 255 }
    );

    bool mostrarOrden =
        fase == FASE_CAPITAN_MOSTRANDO ||
        fase == FASE_CAPITAN_RESPONDIENDO;

    float alturaIzquierda =
        mostrarOrden &&
        orden == CONTROL_DIRECCION_IZQUIERDA
        ? 2.95f
        : 1.55f;

    float alturaDerecha =
        mostrarOrden &&
        orden == CONTROL_DIRECCION_DERECHA
        ? 2.95f
        : 1.55f;

    DrawCylinder(
        Vector3{ -0.92f, alturaIzquierda - 0.45f, -1.8f },
        0.04f,
        0.04f,
        1.45f,
        8,
        DARKGRAY
    );

    DrawCube(
        Vector3{ -1.18f, alturaIzquierda, -1.8f },
        0.55f,
        0.42f,
        0.08f,
        Color{ 55, 195, 225, 255 }
    );

    DrawCylinder(
        Vector3{ 0.92f, alturaDerecha - 0.45f, -1.8f },
        0.04f,
        0.04f,
        1.45f,
        8,
        DARKGRAY
    );

    DrawCube(
        Vector3{ 1.18f, alturaDerecha, -1.8f },
        0.55f,
        0.42f,
        0.08f,
        Color{ 245, 145, 55, 255 }
    );
}


static void DibujarJugadoresCapitan3D(
    const MinijuegoCapitanManda& minijuego,
    const Participante participantes[]
)
{
    Vector3 posiciones[MAX_PARTICIPANTES] =
    {
        { -4.0f, 0.35f, 1.6f },
        { -1.35f, 0.35f, 2.2f },
        { 1.35f, 0.35f, 2.2f },
        { 4.0f, 0.35f, 1.6f }
    };

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        Color color =
            participantes[i].color;

        if (minijuego.jugadores[i].eliminado)
        {
            color = Fade(color, 0.28f);
        }

        Vector3 base =
            posiciones[i];

        DrawCylinder(
            Vector3{ base.x, 0.03f, base.z },
            0.88f,
            0.88f,
            0.18f,
            24,
            minijuego.jugadores[i].eliminado
            ? DARKGRAY
            : Color{ 82, 88, 102, 255 }
        );

        if (!minijuego.jugadores[i].eliminado)
        {
            DrawCube(
                Vector3{ base.x, 0.78f, base.z },
                0.72f,
                1.22f,
                0.72f,
                color
            );

            DrawCubeWires(
                Vector3{ base.x, 0.78f, base.z },
                0.72f,
                1.22f,
                0.72f,
                BLACK
            );
        }
    }
}


void MinijuegoCapitanManda::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(
        Color{ 118, 188, 220, 255 }
    );

    Camera3D camara{};
    camara.position =
        { 0.0f, 7.8f, 12.8f };
    camara.target =
        { 0.0f, 1.1f, 0.3f };
    camara.up =
        { 0.0f, 1.0f, 0.0f };
    camara.fovy = 47.0f;
    camara.projection =
        CAMERA_PERSPECTIVE;

    BeginMode3D(camara);

    DrawCube(
        Vector3{ 0.0f, -0.30f, 0.4f },
        11.0f,
        0.60f,
        7.2f,
        Color{ 211, 190, 142, 255 }
    );

    DrawCubeWires(
        Vector3{ 0.0f, -0.30f, 0.4f },
        11.0f,
        0.60f,
        7.2f,
        DARKBROWN
    );

    DibujarCapitan3D(
        ordenActual,
        fase
    );

    DibujarJugadoresCapitan3D(
        *this,
        participantes
    );

    EndMode3D();

    DrawText(
        "MINIJUEGO 9 - CAPITAN MANDA",
        25,
        20,
        30,
        BLACK
    );

    DrawText(
        "COPIA LA DIRECCION. SI FALLAS O TARDAS, QUEDAS FUERA.",
        25,
        57,
        19,
        DARKGRAY
    );

    // Ayuda fija y separada del titulo. En gamepad las acciones
    // direccionales del minijuego usan los botones de cara X/B.
    DrawRectangle(
        20,
        84,
        520,
        54,
        Fade(RAYWHITE, 0.82f)
    );

    DrawRectangleLines(
        20,
        84,
        520,
        54,
        Fade(DARKGRAY, 0.65f)
    );

    DrawText(
        "TECLADO: A/D O FLECHA IZQ/DER",
        30,
        90,
        17,
        DARKGRAY
    );

    DrawText(
        "MANDO: X = IZQUIERDA    B = DERECHA",
        30,
        113,
        18,
        DARKBLUE
    );

    if (fase == FASE_CAPITAN_PREPARACION)
    {
        int numero =
            (int)std::ceil(
                tiempoPreparacion
            );

        if (numero < 1)
        {
            numero = 1;
        }

        const char* texto =
            TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 90) / 2,
            150,
            90,
            GOLD
        );
    }
    else if (
        fase == FASE_CAPITAN_MOSTRANDO ||
        fase == FASE_CAPITAN_RESPONDIENDO
    )
    {
        const char* direccion =
            ordenActual == CONTROL_DIRECCION_IZQUIERDA
            ? "<  IZQUIERDA"
            : "DERECHA  >";

        int tamano = 48;

        DrawText(
            direccion,
            GetScreenWidth() / 2 -
                MeasureText(direccion, tamano) / 2,
            150,
            tamano,
            ordenActual == CONTROL_DIRECCION_IZQUIERDA
            ? Color{ 25, 155, 205, 255 }
            : ORANGE
        );

        if (fase == FASE_CAPITAN_RESPONDIENDO)
        {
            const char* ahora = "AHORA";

            DrawText(
                ahora,
                GetScreenWidth() / 2 -
                    MeasureText(ahora, 28) / 2,
                208,
                28,
                LIME
            );

            DrawRectangle(
                GetScreenWidth() / 2 - 160,
                243,
                320,
                16,
                Fade(BLACK, 0.35f)
            );

            float porcentaje =
                tiempoRespuesta > 0.0f
                ? tiempoFase / tiempoRespuesta
                : 0.0f;

            if (porcentaje < 0.0f)
            {
                porcentaje = 0.0f;
            }

            DrawRectangle(
                GetScreenWidth() / 2 - 160,
                243,
                (int)(320.0f * porcentaje),
                16,
                porcentaje < 0.30f
                ? RED
                : GOLD
            );
        }
    }

    DrawText(
        TextFormat("RONDA %d", numeroRonda),
        GetScreenWidth() - 180,
        25,
        24,
        DARKBLUE
    );

    // El panel global de ZonaPruebas ocupa la franja inferior.
    // Subimos este bloque para que ningun control quede tapado.
    int y = GetScreenHeight() - 305;

    DrawRectangle(
        18,
        y - 10,
        640,
        120,
        Fade(BLACK, 0.46f)
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCapitanManda& jugador =
            jugadores[i];

        const char* estado =
            jugador.eliminado
            ? "FUERA"
            : (
                jugador.respondio
                ? (jugador.acerto ? "OK" : "ERROR")
                : "ESPERANDO"
            );

        const char* izquierda =
            ObtenerTextoAccionDireccionalControl(
                participantes[i],
                CONTROL_DIRECCION_IZQUIERDA
            );

        const char* derecha =
            ObtenerTextoAccionDireccionalControl(
                participantes[i],
                CONTROL_DIRECCION_DERECHA
            );

        DrawText(
            TextFormat(
                "J%d  %s   IZQ:%s  DER:%s",
                participantes[i].numeroJugador,
                estado,
                izquierda,
                derecha
            ),
            28,
            y,
            18,
            jugador.eliminado
            ? GRAY
            : participantes[i].color
        );

        y += 27;
    }

    if (fase == FASE_CAPITAN_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 325,
            GetScreenHeight() / 2 - 145,
            650,
            290,
            Fade(BLACK, 0.90f)
        );

        int ganadores[MAX_PARTICIPANTES]{};

        int cantidadGanadores =
            ObtenerIndicesGanadores(
                resultado,
                ganadores,
                MAX_PARTICIPANTES
            );

        const char* titulo =
            resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                ? participantes[ganadores[0]].numeroJugador
                : 0
            );

        DrawText(
            titulo,
            GetScreenWidth() / 2 -
                MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 118,
            34,
            GOLD
        );

        int fila =
            GetScreenHeight() / 2 - 62;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POS %d   RONDAS %d",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    jugadores[i].rondasSuperadas
                ),
                GetScreenWidth() / 2 - 190,
                fila,
                21,
                participantes[i].color
            );

            fila += 29;
        }

        const char* reiniciar =
            "R PARA REINICIAR";

        DrawText(
            reiniciar,
            GetScreenWidth() / 2 -
                MeasureText(reiniciar, 22) / 2,
            GetScreenHeight() / 2 + 105,
            22,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoCapitanManda::ObtenerResultado() const
{
    return resultado;
}
