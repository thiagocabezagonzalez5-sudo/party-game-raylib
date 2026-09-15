#include "Minigames/MinijuegoConteoExplosivo.h"

#include "Minigames/TiposMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_CONTEO = 2.5f;
static const float DURACION_OBSERVACION_CONTEO = 8.0f;
static const float DURACION_RESPUESTA_CONTEO = 7.0f;


static Color ColorDronConteo(int indice)
{
    const Color colores[] =
    {
        SKYBLUE,
        ORANGE,
        LIME,
        VIOLET,
        GOLD
    };

    return colores[indice % 5];
}


static void PrepararDronesConteo(
    MinijuegoConteoExplosivo& minijuego
)
{
    minijuego.cantidadDrones = GetRandomValue(
        12,
        MAX_DRONES_CONTEO_EXPLOSIVO
    );

    for (int i = 0; i < minijuego.cantidadDrones; i++)
    {
        DronConteoExplosivo& dron = minijuego.drones[i];

        dron.posicion =
        {
            GetRandomValue(-540, 540) / 100.0f,
            GetRandomValue(90, 250) / 100.0f,
            GetRandomValue(-330, 330) / 100.0f
        };

        float velocidadX = GetRandomValue(-120, 120) / 100.0f;
        float velocidadZ = GetRandomValue(-105, 105) / 100.0f;

        if (std::fabs(velocidadX) < 0.38f)
        {
            velocidadX = velocidadX < 0.0f ? -0.62f : 0.62f;
        }

        if (std::fabs(velocidadZ) < 0.32f)
        {
            velocidadZ = velocidadZ < 0.0f ? -0.54f : 0.54f;
        }

        dron.velocidad = { velocidadX, 0.0f, velocidadZ };
        dron.color = ColorDronConteo(i);
        dron.faseFlotacion = GetRandomValue(0, 628) / 100.0f;
    }
}


static void ActualizarDronesConteo(
    MinijuegoConteoExplosivo& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < minijuego.cantidadDrones; i++)
    {
        DronConteoExplosivo& dron = minijuego.drones[i];

        dron.posicion.x += dron.velocidad.x * deltaTime;
        dron.posicion.z += dron.velocidad.z * deltaTime;

        if (dron.posicion.x < -5.7f || dron.posicion.x > 5.7f)
        {
            dron.posicion.x = Clamp(dron.posicion.x, -5.7f, 5.7f);
            dron.velocidad.x *= -1.0f;
        }

        if (dron.posicion.z < -3.5f || dron.posicion.z > 3.5f)
        {
            dron.posicion.z = Clamp(dron.posicion.z, -3.5f, 3.5f);
            dron.velocidad.z *= -1.0f;
        }
    }
}


static void FinalizarConteo(
    MinijuegoConteoExplosivo& minijuego
)
{
    int mejorDiferencia = 1000;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorConteoExplosivo& jugador = minijuego.jugadores[i];
        jugador.diferencia = std::abs(
            jugador.respuesta - minijuego.cantidadDrones
        );

        if (jugador.diferencia < mejorDiferencia)
        {
            mejorDiferencia = jugador.diferencia;
            cantidadGanadores = 1;
        }
        else if (jugador.diferencia == mejorDiferencia)
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

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                minijuego.jugadores[j].diferencia <
                    minijuego.jugadores[i].diferencia
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            100 - minijuego.jugadores[i].diferencia * 15;

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

    minijuego.fase = FASE_CONTEO_TERMINADO;
    minijuego.tiempoFase = 0.0f;
}


static void DibujarDronConteo(
    const DronConteoExplosivo& dron,
    float tiempoAnimacion
)
{
    Vector3 posicion = dron.posicion;
    posicion.y += std::sin(
        tiempoAnimacion * 2.8f + dron.faseFlotacion
    ) * 0.14f;

    DrawSphere(posicion, 0.24f, dron.color);
    DrawSphereWires(posicion, 0.25f, 6, 8, Fade(RAYWHITE, 0.8f));

    DrawCube(
        { posicion.x - 0.35f, posicion.y, posicion.z },
        0.34f,
        0.06f,
        0.12f,
        LIGHTGRAY
    );

    DrawCube(
        { posicion.x + 0.35f, posicion.y, posicion.z },
        0.34f,
        0.06f,
        0.12f,
        LIGHTGRAY
    );
}


static const char* TextoFaseConteo(
    FaseConteoExplosivo fase
)
{
    switch (fase)
    {
        case FASE_CONTEO_PREPARACION: return "PREPARATE";
        case FASE_CONTEO_OBSERVACION: return "CUENTA LOS DRONES";
        case FASE_CONTEO_RESPUESTA: return "CUANTOS VISTE?";
        case FASE_CONTEO_TERMINADO: return "RESULTADOS";
    }

    return "";
}


void MinijuegoConteoExplosivo::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    camara.position = { 0.0f, 9.0f, 11.0f };
    camara.target = { 0.0f, 1.1f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CONTEO_PREPARACION;
    cantidadDrones = 0;
    tiempoFase = DURACION_PREPARACION_CONTEO;
    tiempoAnimacion = 0.0f;
}


void MinijuegoConteoExplosivo::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    PrepararDronesConteo(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].respuesta = 10;
        jugadores[i].retrasoBot = GetRandomValue(35, 160) / 100.0f;
    }
}


void MinijuegoConteoExplosivo::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_CONTEO_PREPARACION ||
        fase == FASE_CONTEO_OBSERVACION
    )
    {
        ActualizarDronesConteo(*this, deltaTime);
    }

    if (fase == FASE_CONTEO_TERMINADO)
    {
        return;
    }

    tiempoFase -= deltaTime;

    if (fase == FASE_CONTEO_PREPARACION)
    {
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CONTEO_OBSERVACION;
            tiempoFase = DURACION_OBSERVACION_CONTEO;
        }

        return;
    }

    if (fase == FASE_CONTEO_OBSERVACION)
    {
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CONTEO_RESPUESTA;
            tiempoFase = DURACION_RESPUESTA_CONTEO;
        }

        return;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorConteoExplosivo& jugador = jugadores[i];

        if (participantes[i].esBot)
        {
            jugador.retrasoBot -= deltaTime;

            if (!jugador.botRespondio && jugador.retrasoBot <= 0.0f)
            {
                int error = GetRandomValue(-2, 2);

                if (GetRandomValue(0, 99) < 38)
                {
                    error = 0;
                }

                jugador.respuesta = cantidadDrones + error;
                jugador.respuesta = Clamp(jugador.respuesta, 0, 30);
                jugador.botRespondio = true;
            }

            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participantes[i]);

        if (entrada.golpear)
        {
            jugador.respuesta++;
        }

        if (entrada.saltar)
        {
            jugador.respuesta--;
        }

        jugador.respuesta = Clamp(jugador.respuesta, 0, 30);
    }

    if (tiempoFase <= 0.0f)
    {
        FinalizarConteo(*this);
    }
}


void MinijuegoConteoExplosivo::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 17, 24, 43, 255 });

    BeginMode3D(camara);

    DrawPlane(
        { 0.0f, 0.0f, 0.0f },
        { 13.0f, 8.5f },
        Color{ 31, 47, 72, 255 }
    );

    DrawGrid(14, 1.0f);

    DrawCubeWires(
        { 0.0f, 2.0f, 0.0f },
        12.5f,
        4.0f,
        8.0f,
        Fade(SKYBLUE, 0.38f)
    );

    if (
        fase == FASE_CONTEO_PREPARACION ||
        fase == FASE_CONTEO_OBSERVACION ||
        fase == FASE_CONTEO_TERMINADO
    )
    {
        for (int i = 0; i < cantidadDrones; i++)
        {
            DibujarDronConteo(drones[i], tiempoAnimacion);
        }
    }
    else
    {
        DrawCube(
            { 0.0f, 2.2f, 0.0f },
            12.7f,
            4.5f,
            8.2f,
            Color{ 11, 16, 29, 255 }
        );

        DrawCubeWires(
            { 0.0f, 2.2f, 0.0f },
            12.7f,
            4.5f,
            8.2f,
            MAGENTA
        );
    }

    EndMode3D();

    DrawRectangle(18, 16, 520, 112, Fade(BLACK, 0.76f));
    DrawText("CONTEO EXPLOSIVO", 32, 28, 30, GOLD);
    DrawText(TextoFaseConteo(fase), 32, 66, 22, RAYWHITE);

    if (fase != FASE_CONTEO_TERMINADO)
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoFase > 0.0f ? tiempoFase : 0.0f),
            32,
            96,
            18,
            LIGHTGRAY
        );
    }

    if (
        fase == FASE_CONTEO_RESPUESTA ||
        fase == FASE_CONTEO_TERMINADO
    )
    {
        int anchoPanel = 210;
        int separacion = 14;
        int cantidadActivos = resultado.cantidadParticipantes;
        int anchoTotal = cantidadActivos * anchoPanel +
            (cantidadActivos - 1) * separacion;
        int x = (GetScreenWidth() - anchoTotal) / 2;
        int orden = 0;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            int panelX = x + orden * (anchoPanel + separacion);
            Color color = participantes[i].color;

            DrawRectangle(
                panelX,
                GetScreenHeight() - 190,
                anchoPanel,
                116,
                Fade(BLACK, 0.78f)
            );

            DrawRectangleLinesEx(
                {
                    (float)panelX,
                    (float)GetScreenHeight() - 190.0f,
                    (float)anchoPanel,
                    116.0f
                },
                3.0f,
                color
            );

            DrawText(
                TextFormat("J%d", participantes[i].numeroJugador),
                panelX + 14,
                GetScreenHeight() - 176,
                20,
                color
            );

            DrawText(
                TextFormat("%02d", jugadores[i].respuesta),
                panelX + 76,
                GetScreenHeight() - 166,
                42,
                RAYWHITE
            );

            if (fase == FASE_CONTEO_TERMINADO)
            {
                const char* texto =
                    resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal);

                DrawText(
                    texto,
                    panelX + 14,
                    GetScreenHeight() - 112,
                    18,
                    resultado.participantes[i].posicionFinal == 1
                        ? GOLD
                        : LIGHTGRAY
                );
            }

            orden++;
        }
    }

    if (fase == FASE_CONTEO_RESPUESTA)
    {
        DrawText(
            "GOLPEAR: +1     SALTAR: -1",
            GetScreenWidth() / 2 - 174,
            148,
            21,
            RAYWHITE
        );
    }
    else if (fase == FASE_CONTEO_TERMINADO)
    {
        DrawText(
            TextFormat("HABIA %d DRONES", cantidadDrones),
            GetScreenWidth() / 2 - 128,
            148,
            26,
            GOLD
        );
    }
}


const ResultadoMinijuego&
MinijuegoConteoExplosivo::ObtenerResultado() const
{
    return resultado;
}
