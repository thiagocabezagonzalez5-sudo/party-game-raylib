#include "Minigames/MinijuegoCapitanManda.h"
#include "Minigames/AudioMinijuegos.h"
#include "Minigames/ModeloJugadorCompartido.h"

#define SOMBRAS_RETRO_AUTOMATICAS
#include "Minigames/SombrasRetro.h"

#include <cmath>


static const float DURACION_PREPARACION_CAPITAN = 3.0f;
static const float DURACION_RESOLUCION = 0.85f;
static const int MAX_RONDAS_CAPITAN = 40;
static const float ESPERA_MINIMA_ENTRE_RONDAS_CAPITAN = 0.45f;
static const float ESPERA_MAXIMA_ENTRE_RONDAS_CAPITAN = 1.45f;


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


static float ObtenerEsperaAleatoriaEntreRondasCapitan()
{
    int minimo =
        (int)(ESPERA_MINIMA_ENTRE_RONDAS_CAPITAN * 100.0f);

    int maximo =
        (int)(ESPERA_MAXIMA_ENTRE_RONDAS_CAPITAN * 100.0f);

    return
        (float)GetRandomValue(minimo, maximo) /
        100.0f;
}


static void ProgramarEsperaEntreRondasCapitan(
    MinijuegoCapitanManda& minijuego
)
{
    minijuego.fase = FASE_CAPITAN_MOSTRANDO;
    minijuego.tiempoFase =
        ObtenerEsperaAleatoriaEntreRondasCapitan();

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

    // La orden aparece y puede responderse en el mismo frame. La espera
    // aleatoria ocurre antes de esta funcion, entre una ronda y la siguiente.
    minijuego.tiempoFase =
        minijuego.tiempoRespuesta;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);

    minijuego.fase =
        FASE_CAPITAN_RESPONDIENDO;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.jugadores[i].eliminado)
        {
            continue;
        }

        minijuego.jugadores[i].respondio = false;
        minijuego.jugadores[i].acerto = false;
        minijuego.jugadores[i].tiempoFeedback = 0.0f;
        minijuego.jugadores[i].tiempoBot =
            GetRandomValue(30, (int)(minijuego.tiempoRespuesta * 85.0f)) / 100.0f;
        minijuego.jugadores[i].botAcertara =
            GetRandomValue(0, 99) >= 6 + minijuego.numeroRonda * 2;
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

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
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
            jugador.posicionFinal = posicionEliminados;
            jugador.tiempoFeedback = DURACION_RESOLUCION;
        }
        else
        {
            jugador.rondasSuperadas++;
            jugador.tiempoFeedback = DURACION_RESOLUCION;
        }
    }

    int vivosDespues =
        ContarVivosCapitan(minijuego);

    // Ya no existe un maximo de rondas. Solo termina cuando queda uno o
    // ninguno; si todos fallan la misma ronda se conserva el empate.
    if (vivosDespues <= 1 || minijuego.numeroRonda >= MAX_RONDAS_CAPITAN)
    {
        FinalizarCapitan(minijuego);
        return;
    }

    ReproducirSonidoMinijuego(
        minijuego.audio,
        eliminados > 0 ? SONIDO_ELIMINADO : SONIDO_ACIERTO
    );

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

    fase = FASE_CAPITAN_PREPARACION;
    ordenActual = CONTROL_DIRECCION_IZQUIERDA;
    numeroRonda = 0;
    tiempoPreparacion = DURACION_PREPARACION_CAPITAN;
    tiempoFase = 0.0f;
    tiempoRespuesta = 1.55f;
    resultadoInicializado = false;
}


void MinijuegoCapitanManda::Reiniciar(
    const Participante participantes[]
)
{
    Inicializar();

    if (participantes == nullptr)
    {
        return;
    }

    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    resultadoInicializado = true;
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
        fase = FASE_CAPITAN_PREPARACION;
        tiempoPreparacion = DURACION_PREPARACION_CAPITAN;
    }

    if (fase == FASE_CAPITAN_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CAPITAN_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

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
            tiempoFase = 0.0f;
            PrepararNuevaRonda(*this);
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
                jugadores[i].respondio
            )
            {
                continue;
            }

            // Bots y humanos desconectados reaccionan tras un retraso y a
            // veces se equivocan (mas a medida que avanzan las rondas).
            if (participantes[i].esBot || !participantes[i].conectado)
            {
                jugadores[i].tiempoBot -= deltaTime;

                if (jugadores[i].tiempoBot <= 0.0f)
                {
                    jugadores[i].respondio = true;
                    jugadores[i].acerto = jugadores[i].botAcertara;
                }

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

            if (ordenActual == CONTROL_DIRECCION_IZQUIERDA)
            {
                jugadores[i].acerto = izquierda && !derecha;
            }
            else
            {
                jugadores[i].acerto = derecha && !izquierda;
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
            ProgramarEsperaEntreRondasCapitan(*this);
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
        fase == FASE_CAPITAN_RESPONDIENDO;

    float alturaIzquierda =
        mostrarOrden && orden == CONTROL_DIRECCION_IZQUIERDA
        ? 2.95f
        : 1.55f;

    float alturaDerecha =
        mostrarOrden && orden == CONTROL_DIRECCION_DERECHA
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

        Color color = participantes[i].color;

        if (minijuego.jugadores[i].eliminado)
        {
            color = Fade(color, 0.28f);
        }

        Vector3 base = posiciones[i];

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

        // Anillo del color del jugador en el borde de su pedestal: el
        // modelo es oscuro y no basta para saber quien es quien.
        for (int k = 0; k < 3; k++)
        {
            DrawCircle3D(
                Vector3{ base.x, 0.23f + k * 0.01f, base.z },
                0.62f + k * 0.07f,
                Vector3{ 1.0f, 0.0f, 0.0f },
                90.0f,
                minijuego.jugadores[i].eliminado ? Fade(color, 0.6f) : color
            );
        }

        if (!minijuego.jugadores[i].eliminado)
        {
            DibujarModeloJugadorEnPosicion(
                Vector3{ base.x, 0.17f, base.z },
                180.0f,
                color
            );

            // Flecha sobre la cabeza.
            // MODELO FUTURO: icono de jugador del GLB.
            float rebote =
                std::sin((float)GetTime() * 5.0f + base.x) * 0.06f;
            DrawCylinderEx(
                Vector3{ base.x, 2.55f + rebote, base.z },
                Vector3{ base.x, 2.87f + rebote, base.z },
                0.0f,
                0.20f,
                10,
                color
            );
        }
    }
}


//==================================================
// ESCENARIO: cubierta de un barco pirata (solo visual)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB la cubierta, el mastil con velas,
// las barandas, los barriles, el timon y el mar con olas.
//==================================================
static void DibujarCubiertaCapitan(float tiempo)
{
    // Mar con olas detras y a los lados.
    DrawCube({ 0.0f, -1.2f, 0.0f }, 60.0f, 0.2f, 40.0f, Color{ 40, 110, 160, 255 });

    for (int ola = 0; ola < 10; ola++)
    {
        float x = -22.0f + ola * 4.8f;
        float z = -9.0f + std::fmod(ola * 3.7f, 14.0f);
        DrawCube(
            { x, -1.05f + std::sin(tiempo * 1.5f + ola) * 0.08f, z },
            2.4f,
            0.06f,
            0.4f,
            Fade(RAYWHITE, 0.6f)
        );
    }

    // Tablones de la cubierta.
    for (int t = 0; t < 8; t++)
    {
        DrawCube(
            { -4.8f + t * 1.37f, 0.02f, 0.4f },
            0.05f,
            0.04f,
            7.2f,
            Fade(DARKBROWN, 0.55f)
        );
    }

    // Barandas a ambos lados y al fondo.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube({ lado * 5.4f, 0.8f, 0.4f }, 0.2f, 0.15f, 7.2f, Color{ 120, 80, 45, 255 });

        for (int p = 0; p < 5; p++)
        {
            DrawCube({ lado * 5.4f, 0.4f, -2.8f + p * 1.6f }, 0.15f, 0.8f, 0.15f, Color{ 100, 66, 38, 255 });
        }
    }

    DrawCube({ 0.0f, 0.8f, -3.2f }, 11.0f, 0.15f, 0.2f, Color{ 120, 80, 45, 255 });

    // Mastil con vela y bandera.
    DrawCylinder({ 0.0f, 0.0f, -3.0f }, 0.18f, 0.22f, 7.0f, 8, Color{ 105, 70, 40, 255 });
    DrawCube({ 0.0f, 5.6f, -3.0f }, 5.0f, 0.15f, 0.15f, Color{ 105, 70, 40, 255 });
    DrawCube({ 0.0f, 4.0f + std::sin(tiempo) * 0.05f, -2.9f }, 4.6f, 3.0f, 0.06f, Fade(Color{ 240, 232, 205, 255 }, 0.95f));
    DrawCube({ 0.6f + std::sin(tiempo * 3.0f) * 0.1f, 7.1f, -3.0f }, 1.1f, 0.5f, 0.05f, RED);

    // Barriles y timon.
    for (int b = 0; b < 3; b++)
    {
        DrawCylinder({ -4.5f + b * 0.8f, 0.0f, -2.4f }, 0.35f, 0.35f, 0.8f, 10, Color{ 130, 90, 50, 255 });
    }

    DrawCylinder({ 4.2f, 0.0f, -2.4f }, 0.12f, 0.12f, 1.0f, 8, DARKBROWN);
    DrawCircle3D({ 4.2f, 1.1f, -2.4f }, 0.5f, { 0.0f, 0.0f, 1.0f }, 0.0f, Color{ 150, 105, 60, 255 });
}


void MinijuegoCapitanManda::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 118, 188, 220, 255 });

    Camera3D camara{};
    camara.position = { 0.0f, 7.8f, 12.8f };
    camara.target = { 0.0f, 1.1f, 0.3f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 47.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(camara);

    DibujarCubiertaCapitan((float)GetTime());

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

    DibujarCapitan3D(ordenActual, fase);
    DibujarJugadoresCapitan3D(*this, participantes);

    EndMode3D();

    const int anchoPantalla = GetScreenWidth();

    DrawRectangle(15, 12, 745, 66, Fade(BLACK, 0.55f));
    DrawText("MINIJUEGO 9 - CAPITAN MANDA", 25, 20, 30, RAYWHITE);
    DrawText(
        "LA PAUSA ENTRE RONDAS ES ALEATORIA. CUANDO SUBE LA BANDERA, REACCIONA.",
        25,
        54,
        18,
        LIGHTGRAY
    );

    DrawRectangle(20, 86, 390, 54, Fade(RAYWHITE, 0.82f));
    DrawRectangleLines(20, 86, 390, 54, Fade(DARKGRAY, 0.65f));
    DrawText("TECLADO: A/D O FLECHA IZQ/DER", 30, 92, 17, DARKGRAY);
    DrawText("MANDO: X = IZQUIERDA    B = DERECHA", 30, 115, 18, DARKBLUE);

    if (fase == FASE_CAPITAN_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 92) / 2,
            GetScreenHeight() / 2 - 72,
            92,
            GOLD
        );
    }
    else if (fase == FASE_CAPITAN_MOSTRANDO)
    {
        const char* espera = "PREPARATE...";
        DrawRectangle(anchoPantalla / 2 - 150, 154, 300, 52, Fade(BLACK, 0.55f));
        DrawText(
            espera,
            anchoPantalla / 2 - MeasureText(espera, 34) / 2,
            163,
            34,
            RAYWHITE
        );
    }
    else if (fase == FASE_CAPITAN_RESPONDIENDO)
    {
        const char* direccion =
            ordenActual == CONTROL_DIRECCION_IZQUIERDA
            ? "<  IZQUIERDA"
            : "DERECHA  >";

        int tamano = 48;

        // Panel central: orden, aviso y barra juntos, sobre el cielo y
        // por encima de la cabeza del capitan.
        DrawRectangle(
            anchoPantalla / 2 - 210,
            86,
            420,
            128,
            Fade(BLACK, 0.62f)
        );

        DrawText(
            direccion,
            anchoPantalla / 2 - MeasureText(direccion, tamano) / 2,
            94,
            tamano,
            ordenActual == CONTROL_DIRECCION_IZQUIERDA
            ? Color{ 90, 205, 245, 255 }
            : ORANGE
        );

        const char* reaccion = "REACCIONA!";
        DrawText(
            reaccion,
            anchoPantalla / 2 - MeasureText(reaccion, 26) / 2,
            150,
            26,
            LIME
        );

        DrawRectangle(
            anchoPantalla / 2 - 160,
            186,
            320,
            16,
            Fade(WHITE, 0.30f)
        );

        float porcentaje =
            tiempoRespuesta > 0.0f
            ? tiempoFase / tiempoRespuesta
            : 0.0f;

        if (porcentaje < 0.0f) porcentaje = 0.0f;

        DrawRectangle(
            anchoPantalla / 2 - 160,
            186,
            (int)(320.0f * porcentaje),
            16,
            porcentaje < 0.30f ? RED : GOLD
        );
    }

    DrawRectangle(anchoPantalla - 270, 12, 255, 66, Fade(BLACK, 0.55f));

    DrawText(
        fase == FASE_CAPITAN_PREPARACION
            ? "RONDA 0"
            : TextFormat("RONDA %d", numeroRonda),
        anchoPantalla - 180,
        18,
        24,
        RAYWHITE
    );

    DrawText(
        "HASTA QUEDAR UNO",
        anchoPantalla - 245,
        50,
        17,
        LIGHTGRAY
    );

    // Estado por jugador en el cielo de la derecha, sin tapar la cubierta.
    int y = 92;
    DrawRectangle(anchoPantalla - 420, y - 6, 405, 4 * 24 + 8, Fade(BLACK, 0.55f));

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCapitanManda& jugador = jugadores[i];

        const char* estadoJugador =
            jugador.eliminado
            ? "FUERA"
            : (
                fase == FASE_CAPITAN_PREPARACION ||
                fase == FASE_CAPITAN_MOSTRANDO
                ? "LISTO"
                : (
                    jugador.respondio
                    ? (jugador.acerto ? "OK" : "ERROR")
                    : "ESPERANDO"
                )
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
                estadoJugador,
                izquierda,
                derecha
            ),
            anchoPantalla - 410,
            y,
            17,
            jugador.eliminado ? GRAY : participantes[i].color
        );

        y += 24;
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
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 118,
            34,
            GOLD
        );

        int fila = GetScreenHeight() / 2 - 62;

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

        const char* reiniciar = TextoReinicioMinijuego();
        DrawText(
            reiniciar,
            GetScreenWidth() / 2 - MeasureText(reiniciar, 22) / 2,
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
