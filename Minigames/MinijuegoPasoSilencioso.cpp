#include "Minigames/MinijuegoPasoSilencioso.h"

#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_PASO = 2.5f;
static const float DURACION_CARRERA_PASO = 35.0f;
static const float VELOCIDAD_AVANCE_PASO = 0.105f;
static const float RETROCESO_ALERTA_PASO = 0.17f;


static float PosicionXCarrilPaso(int indiceParticipante)
{
    static const float posiciones[MAX_PARTICIPANTES] =
    {
        -3.6f,
        -1.2f,
         1.2f,
         3.6f
    };

    return posiciones[indiceParticipante];
}


static float PosicionZProgresoPaso(float progreso)
{
    return 7.0f - Clamp(progreso, 0.0f, 1.0f) * 15.0f;
}


static float NuevaDuracionEstadoCentinela(bool alerta)
{
    if (alerta)
    {
        return GetRandomValue(90, 175) / 100.0f;
    }

    return GetRandomValue(220, 390) / 100.0f;
}


static void CambiarEstadoCentinela(
    MinijuegoPasoSilencioso& minijuego,
    const Participante participantes[]
)
{
    minijuego.centinelaAlerta = !minijuego.centinelaAlerta;
    minijuego.tiempoEstadoCentinela =
        NuevaDuracionEstadoCentinela(minijuego.centinelaAlerta);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorPasoSilencioso& jugador = minijuego.jugadores[i];
        jugador.castigadoEnAlerta = false;

        if (
            !minijuego.resultado.participantes[i].participo ||
            !participantes[i].esBot
        )
        {
            continue;
        }

        if (minijuego.centinelaAlerta)
        {
            jugador.botAvanzando = true;
            jugador.reaccionBot = GetRandomValue(7, 34) / 100.0f;
        }
        else
        {
            jugador.botAvanzando = false;
            jugador.reaccionBot = GetRandomValue(8, 42) / 100.0f;
        }
    }
}


static int PuntuacionCarreraPaso(
    const EstadoJugadorPasoSilencioso& jugador
)
{
    return (int)(Clamp(jugador.progreso, 0.0f, 1.0f) * 10000.0f + 0.5f);
}


static void FinalizarPasoSilencioso(
    MinijuegoPasoSilencioso& minijuego
)
{
    int mejorPuntuacion = -1;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        int puntuacion = PuntuacionCarreraPaso(minijuego.jugadores[i]);

        if (puntuacion > mejorPuntuacion)
        {
            mejorPuntuacion = puntuacion;
            cantidadGanadores = 1;
        }
        else if (puntuacion == mejorPuntuacion)
        {
            cantidadGanadores++;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int puntuacionCarrera = PuntuacionCarreraPaso(minijuego.jugadores[i]);
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                PuntuacionCarreraPaso(minijuego.jugadores[j]) > puntuacionCarrera
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            puntuacionCarrera / 10 -
            minijuego.jugadores[i].penalizaciones * 20;

        if (resultadoJugador.puntuacionMinijuego < 0)
        {
            resultadoJugador.puntuacionMinijuego = 0;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_PASO_TERMINADO;
}


static void DibujarCentinelaPaso(
    const MinijuegoPasoSilencioso& minijuego
)
{
    Color colorLuz = minijuego.centinelaAlerta ? RED : LIME;
    float giro = minijuego.centinelaAlerta
        ? std::sin(minijuego.tiempoAnimacion * 8.0f) * 18.0f
        : 180.0f;

    DrawCylinder(
        { 0.0f, 1.15f, -9.2f },
        1.15f,
        1.45f,
        2.3f,
        12,
        Color{ 64, 70, 82, 255 }
    );

    DrawCylinderWires(
        { 0.0f, 1.15f, -9.2f },
        1.15f,
        1.45f,
        2.3f,
        12,
        LIGHTGRAY
    );

    Vector3 cabeza = { 0.0f, 3.25f, -9.2f };
    DrawSphere(cabeza, 0.82f, Color{ 45, 48, 58, 255 });

    float desplazamientoX = std::sin(giro * DEG2RAD) * 0.38f;
    DrawSphere(
        { cabeza.x + desplazamientoX, cabeza.y + 0.08f, cabeza.z + 0.66f },
        0.19f,
        colorLuz
    );

    if (minijuego.centinelaAlerta)
    {
        Vector3 origenVision =
        {
            cabeza.x + desplazamientoX,
            cabeza.y,
            cabeza.z + 0.65f
        };

        Vector3 bordeIzquierdo = { -5.0f, 0.08f, 7.8f };
        Vector3 bordeDerecho = { 5.0f, 0.08f, 7.8f };

        DrawTriangle3D(
            origenVision,
            bordeIzquierdo,
            bordeDerecho,
            Fade(RED, 0.10f)
        );

        DrawTriangle3D(
            origenVision,
            bordeDerecho,
            bordeIzquierdo,
            Fade(RED, 0.10f)
        );
    }
}


void MinijuegoPasoSilencioso::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    camara.position = { 0.0f, 10.8f, 14.5f };
    camara.target = { 0.0f, 0.7f, -1.2f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_PASO_PREPARACION;
    centinelaAlerta = false;
    tiempoPreparacion = DURACION_PREPARACION_PASO;
    tiempoEstadoCentinela = NuevaDuracionEstadoCentinela(false);
    tiempoCarrera = DURACION_CARRERA_PASO;
    tiempoAnimacion = 0.0f;
}


void MinijuegoPasoSilencioso::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].reaccionBot = GetRandomValue(8, 42) / 100.0f;
    }
}


void MinijuegoPasoSilencioso::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_PASO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_PASO_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            fase = FASE_PASO_CARRERA;
            tiempoPreparacion = 0.0f;
        }

        return;
    }

    tiempoCarrera -= deltaTime;
    tiempoEstadoCentinela -= deltaTime;

    if (tiempoEstadoCentinela <= 0.0f)
    {
        CambiarEstadoCentinela(*this, participantes);
    }

    bool alguienLlego = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            jugadores[i].llegoMeta
        )
        {
            continue;
        }

        EstadoJugadorPasoSilencioso& jugador = jugadores[i];
        bool avanzando = false;

        if (participantes[i].esBot)
        {
            jugador.reaccionBot -= deltaTime;

            if (jugador.reaccionBot <= 0.0f)
            {
                jugador.botAvanzando = !centinelaAlerta;
            }

            avanzando = jugador.botAvanzando;
        }
        else
        {
            InputMinijuegoParticipante entrada =
                LeerInputMinijuegoParticipante(participantes[i]);

            avanzando = entrada.adelante;
        }

        if (centinelaAlerta)
        {
            if (avanzando && !jugador.castigadoEnAlerta)
            {
                jugador.progreso -= RETROCESO_ALERTA_PASO;
                jugador.progreso = Clamp(jugador.progreso, 0.0f, 1.0f);
                jugador.penalizaciones++;
                jugador.castigadoEnAlerta = true;
            }

            continue;
        }

        if (avanzando)
        {
            jugador.progreso += VELOCIDAD_AVANCE_PASO * deltaTime;
            jugador.progreso = Clamp(jugador.progreso, 0.0f, 1.0f);

            if (jugador.progreso >= 1.0f)
            {
                jugador.llegoMeta = true;
                alguienLlego = true;
            }
        }
    }

    if (alguienLlego || tiempoCarrera <= 0.0f)
    {
        tiempoCarrera = tiempoCarrera < 0.0f ? 0.0f : tiempoCarrera;
        FinalizarPasoSilencioso(*this);
    }
}


void MinijuegoPasoSilencioso::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 14, 16, 27, 255 });

    BeginMode3D(camara);

    DrawPlane(
        { 0.0f, -0.06f, -0.6f },
        { 10.5f, 18.5f },
        Color{ 36, 42, 58, 255 }
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float x = PosicionXCarrilPaso(i);
        DrawCube(
            { x, 0.0f, -0.5f },
            1.85f,
            0.10f,
            17.0f,
            i % 2 == 0
                ? Color{ 53, 60, 78, 255 }
                : Color{ 43, 50, 68, 255 }
        );

        DrawLine3D(
            { x - 0.95f, 0.07f, 7.9f },
            { x - 0.95f, 0.07f, -8.9f },
            Fade(RAYWHITE, 0.25f)
        );
    }

    DrawCube(
        { 0.0f, 0.08f, -8.05f },
        10.2f,
        0.16f,
        0.28f,
        GOLD
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        float x = PosicionXCarrilPaso(i);
        float z = PosicionZProgresoPaso(jugadores[i].progreso);

        DrawCircle3D(
            { x, 0.03f, z },
            0.52f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(BLACK, 0.36f)
        );

        DibujarModeloJugadorEnPosicion(
            { x, 0.04f, z },
            180.0f,
            participantes[i].color
        );
    }

    DibujarCentinelaPaso(*this);

    EndMode3D();

    DrawRectangle(18, 16, 515, 126, Fade(BLACK, 0.78f));
    DrawText("PASO SILENCIOSO", 32, 28, 30, GOLD);

    if (fase == FASE_PASO_PREPARACION)
    {
        DrawText("PREPARATE", 32, 68, 23, RAYWHITE);
        DrawText(
            TextFormat("EMPIEZA EN %.1f", tiempoPreparacion),
            32,
            101,
            18,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_PASO_CARRERA)
    {
        DrawText(
            centinelaAlerta ? "ALTO! TE ESTA MIRANDO" : "AVANZA! ESTA DORMIDO",
            32,
            68,
            23,
            centinelaAlerta ? RED : LIME
        );

        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoCarrera),
            32,
            101,
            18,
            LIGHTGRAY
        );
    }
    else
    {
        DrawText("RESULTADOS", 32, 68, 23, RAYWHITE);
        DrawText("R PARA JUGAR DE NUEVO", 32, 101, 18, LIGHTGRAY);
    }

    int yInicial = GetScreenHeight() - 174;
    int anchoBarra = 240;
    int separacion = 16;
    int cantidadActivos = resultado.cantidadParticipantes;
    int anchoTotal = cantidadActivos * anchoBarra +
        (cantidadActivos - 1) * separacion;
    int xInicial = (GetScreenWidth() - anchoTotal) / 2;
    int orden = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        int x = xInicial + orden * (anchoBarra + separacion);
        float progreso = Clamp(jugadores[i].progreso, 0.0f, 1.0f);

        DrawRectangle(x, yInicial, anchoBarra, 88, Fade(BLACK, 0.78f));
        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            x + 12,
            yInicial + 10,
            19,
            participantes[i].color
        );

        DrawRectangle(x + 12, yInicial + 39, anchoBarra - 24, 18, DARKGRAY);
        DrawRectangle(
            x + 12,
            yInicial + 39,
            (int)((anchoBarra - 24) * progreso),
            18,
            participantes[i].color
        );

        if (fase == FASE_PASO_TERMINADO)
        {
            DrawText(
                resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal),
                x + 12,
                yInicial + 63,
                16,
                resultado.participantes[i].posicionFinal == 1
                    ? GOLD
                    : LIGHTGRAY
            );
        }
        else
        {
            DrawText(
                TextFormat("CASTIGOS: %d", jugadores[i].penalizaciones),
                x + 12,
                yInicial + 63,
                15,
                LIGHTGRAY
            );
        }

        orden++;
    }

    if (fase != FASE_PASO_TERMINADO)
    {
        DrawText(
            "MANTEN ADELANTE PARA AVANZAR. SUELTA CUANDO SE PONGA ROJO.",
            GetScreenWidth() / 2 - 338,
            158,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoPasoSilencioso::ObtenerResultado() const
{
    return resultado;
}
