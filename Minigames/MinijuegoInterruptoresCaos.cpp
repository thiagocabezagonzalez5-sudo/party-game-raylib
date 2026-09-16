#include "Minigames/MinijuegoInterruptoresCaos.h"

#include "raylib.h"
#include "raymath.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_INTERRUPTORES = 2.5f;
static const float DURACION_TURNO_INTERRUPTORES = 10.0f;
static const float DURACION_SEGURO_INTERRUPTORES = 0.8f;
static const float DURACION_EXPLOSION_INTERRUPTORES = 1.45f;


static Color ColorInterruptorCaos(int indice)
{
    const Color colores[MAX_INTERRUPTORES_CAOS] =
    {
        Color{ 238, 72, 82, 255 },
        Color{ 76, 161, 238, 255 },
        Color{ 246, 197, 68, 255 },
        Color{ 85, 203, 116, 255 },
        Color{ 177, 95, 226, 255 }
    };

    if (indice < 0 || indice >= MAX_INTERRUPTORES_CAOS)
    {
        return GRAY;
    }

    return colores[indice];
}


static int ContarVivosInterruptores(
    const MinijuegoInterruptoresCaos& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int BuscarSiguienteVivoInterruptores(
    const MinijuegoInterruptoresCaos& minijuego,
    int origen
)
{
    int indice = origen;

    for (int intento = 0; intento < MAX_PARTICIPANTES; intento++)
    {
        indice++;
        if (indice >= MAX_PARTICIPANTES) indice = 0;

        if (
            minijuego.resultado.participantes[indice].participo &&
            !minijuego.jugadores[indice].eliminado
        )
        {
            return indice;
        }
    }

    return origen;
}


static int ElegirInterruptorDisponible(
    const MinijuegoInterruptoresCaos& minijuego
)
{
    int disponibles[MAX_INTERRUPTORES_CAOS]{};
    int cantidad = 0;

    for (int i = 0; i < minijuego.cantidadInterruptores; i++)
    {
        if (!minijuego.interruptoresUsados[i])
        {
            disponibles[cantidad] = i;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return 0;
    }

    return disponibles[GetRandomValue(0, cantidad - 1)];
}


static int MoverSeleccionInterruptor(
    const MinijuegoInterruptoresCaos& minijuego,
    int origen,
    int direccion
)
{
    int indice = origen;

    for (int intento = 0; intento < minijuego.cantidadInterruptores; intento++)
    {
        indice += direccion;

        if (indice < 0) indice = minijuego.cantidadInterruptores - 1;
        if (indice >= minijuego.cantidadInterruptores) indice = 0;

        if (!minijuego.interruptoresUsados[indice])
        {
            return indice;
        }
    }

    return origen;
}


static void PrepararRondaInterruptores(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int eliminados =
        minijuego.resultado.cantidadParticipantes -
        ContarVivosInterruptores(minijuego);

    minijuego.cantidadInterruptores = MAX_INTERRUPTORES_CAOS - eliminados;
    if (minijuego.cantidadInterruptores < 3)
    {
        minijuego.cantidadInterruptores = 3;
    }

    for (int i = 0; i < MAX_INTERRUPTORES_CAOS; i++)
    {
        minijuego.interruptoresUsados[i] = false;
    }

    minijuego.interruptorPeligroso = GetRandomValue(
        0,
        minijuego.cantidadInterruptores - 1
    );
}


static void PrepararTurnoInterruptores(
    MinijuegoInterruptoresCaos& minijuego,
    int jugador
)
{
    minijuego.jugadorTurno = jugador;
    minijuego.jugadorExplosion = -1;
    minijuego.numeroTurno++;
    minijuego.tiempoTurno = DURACION_TURNO_INTERRUPTORES;
    minijuego.interruptorSeleccionado =
        ElegirInterruptorDisponible(minijuego);
    minijuego.fase = FASE_INTERRUPTORES_ELECCION;

    if (jugador >= 0)
    {
        minijuego.jugadores[jugador].tiempoDecisionBot =
            GetRandomValue(75, 190) / 100.0f;
    }
}


static void FinalizarInterruptores(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int ganador = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            ganador = i;
            minijuego.jugadores[i].posicionFinal = 1;
            break;
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

        resultadoJugador.posicionFinal =
            minijuego.jugadores[i].posicionFinal;

        resultadoJugador.puntuacionMinijuego =
            (minijuego.resultado.cantidadParticipantes + 1 -
                resultadoJugador.posicionFinal) * 50 +
            minijuego.jugadores[i].turnosSuperados * 10;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        ganador >= 0 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_INTERRUPTORES_TERMINADO;
    minijuego.jugadorTurno = ganador;
}


static void ActivarInterruptor(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int indice = minijuego.interruptorSeleccionado;

    if (
        indice < 0 ||
        indice >= minijuego.cantidadInterruptores ||
        minijuego.interruptoresUsados[indice]
    )
    {
        return;
    }

    if (indice == minijuego.interruptorPeligroso)
    {
        int vivosAntes = ContarVivosInterruptores(minijuego);
        int eliminado = minijuego.jugadorTurno;

        minijuego.jugadores[eliminado].eliminado = true;
        minijuego.jugadores[eliminado].posicionFinal = vivosAntes;
        minijuego.jugadorExplosion = eliminado;
        minijuego.jugadorTurno = -1;
        minijuego.fase = FASE_INTERRUPTORES_EXPLOSION;
        minijuego.tiempoResolucion = DURACION_EXPLOSION_INTERRUPTORES;
        return;
    }

    minijuego.interruptoresUsados[indice] = true;
    minijuego.jugadores[minijuego.jugadorTurno].turnosSuperados++;
    minijuego.fase = FASE_INTERRUPTORES_SEGURO;
    minijuego.tiempoResolucion = DURACION_SEGURO_INTERRUPTORES;
}


void MinijuegoInterruptoresCaos::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_INTERRUPTORES_CAOS; i++)
    {
        interruptoresUsados[i] = false;
    }

    cantidadInterruptores = MAX_INTERRUPTORES_CAOS;
    interruptorPeligroso = 0;
    interruptorSeleccionado = 0;
    jugadorTurno = -1;
    jugadorExplosion = -1;
    numeroTurno = 0;
    fase = FASE_INTERRUPTORES_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_INTERRUPTORES;
    tiempoTurno = DURACION_TURNO_INTERRUPTORES;
    tiempoResolucion = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoInterruptoresCaos::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    PrepararRondaInterruptores(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            jugadorTurno = i;
            break;
        }
    }
}


void MinijuegoInterruptoresCaos::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_INTERRUPTORES_TERMINADO)
    {
        return;
    }

    if (fase == FASE_INTERRUPTORES_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            PrepararTurnoInterruptores(*this, jugadorTurno);
        }

        return;
    }

    if (fase == FASE_INTERRUPTORES_SEGURO)
    {
        tiempoResolucion -= deltaTime;

        if (tiempoResolucion <= 0.0f)
        {
            PrepararTurnoInterruptores(
                *this,
                BuscarSiguienteVivoInterruptores(*this, jugadorTurno)
            );
        }

        return;
    }

    if (fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        tiempoResolucion -= deltaTime;

        if (tiempoResolucion <= 0.0f)
        {
            if (ContarVivosInterruptores(*this) <= 1)
            {
                FinalizarInterruptores(*this);
            }
            else
            {
                int siguiente = BuscarSiguienteVivoInterruptores(
                    *this,
                    jugadorExplosion
                );
                PrepararRondaInterruptores(*this);
                PrepararTurnoInterruptores(*this, siguiente);
            }
        }

        return;
    }

    tiempoTurno -= deltaTime;

    if (jugadorTurno < 0)
    {
        return;
    }

    bool confirmar = tiempoTurno <= 0.0f;

    if (participantes[jugadorTurno].esBot)
    {
        jugadores[jugadorTurno].tiempoDecisionBot -= deltaTime;

        if (jugadores[jugadorTurno].tiempoDecisionBot <= 0.0f)
        {
            interruptorSeleccionado = ElegirInterruptorDisponible(*this);
            confirmar = true;
        }
    }
    else
    {
        if (AccionDireccionalControlPresionada(
            participantes[jugadorTurno],
            CONTROL_DIRECCION_IZQUIERDA
        ))
        {
            interruptorSeleccionado = MoverSeleccionInterruptor(
                *this,
                interruptorSeleccionado,
                -1
            );
        }

        if (AccionDireccionalControlPresionada(
            participantes[jugadorTurno],
            CONTROL_DIRECCION_DERECHA
        ))
        {
            interruptorSeleccionado = MoverSeleccionInterruptor(
                *this,
                interruptorSeleccionado,
                1
            );
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participantes[jugadorTurno]);

        confirmar = confirmar || entrada.golpear;
    }

    if (confirmar)
    {
        ActivarInterruptor(*this);
    }
}


void MinijuegoInterruptoresCaos::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 17, 12, 24, 255 });

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    Vector2 centro = { ancho * 0.5f, alto * 0.46f };

    for (int i = 0; i < 10; i++)
    {
        float x = fmodf(i * 173.0f + tiempoAnimacion * (15.0f + i), ancho + 80.0f) - 40.0f;
        float y = 115.0f + (i * 61 % (alto - 170));
        DrawCircle((int)x, (int)y, 2.0f + i % 3, Fade(VIOLET, 0.35f));
    }

    DrawCircleV(centro, 172.0f, Color{ 43, 35, 54, 255 });
    DrawCircleV(centro, 135.0f, Color{ 27, 24, 34, 255 });
    DrawCircleLines((int)centro.x, (int)centro.y, 172.0f, VIOLET);

    DrawRectangle(
        (int)centro.x - 86,
        (int)centro.y - 115,
        172,
        88,
        Color{ 58, 48, 66, 255 }
    );
    DrawRectangleLines(
        (int)centro.x - 86,
        (int)centro.y - 115,
        172,
        88,
        Fade(RAYWHITE, 0.6f)
    );

    Color reactor =
        fase == FASE_INTERRUPTORES_EXPLOSION
            ? RED
            : Color{ 106, 232, 245, 255 };
    float pulso = 6.0f + std::sin(tiempoAnimacion * 5.0f) * 3.0f;
    DrawCircle((int)centro.x, (int)centro.y - 71, 29.0f + pulso, Fade(reactor, 0.18f));
    DrawCircle((int)centro.x, (int)centro.y - 71, 26.0f, reactor);
    DrawText("!", (int)centro.x - 7, (int)centro.y - 88, 34, BLACK);

    float separacion = 105.0f;
    float inicioX = centro.x - separacion * (cantidadInterruptores - 1) * 0.5f;
    float yInterruptores = centro.y + 75.0f;

    for (int i = 0; i < cantidadInterruptores; i++)
    {
        Vector2 posicion = { inicioX + i * separacion, yInterruptores };
        bool usado = interruptoresUsados[i];
        bool seleccionado =
            fase == FASE_INTERRUPTORES_ELECCION &&
            i == interruptorSeleccionado;
        Color color = usado ? Fade(ColorInterruptorCaos(i), 0.22f) : ColorInterruptorCaos(i);

        if (seleccionado)
        {
            float radio = 39.0f + std::sin(tiempoAnimacion * 8.0f) * 3.0f;
            DrawCircleLines((int)posicion.x, (int)posicion.y, radio, RAYWHITE);
            DrawTriangle(
                { posicion.x, posicion.y - 55.0f },
                { posicion.x - 9.0f, posicion.y - 69.0f },
                { posicion.x + 9.0f, posicion.y - 69.0f },
                GOLD
            );
        }

        DrawRectangleRounded(
            { posicion.x - 32.0f, posicion.y - 19.0f, 64.0f, 50.0f },
            0.22f,
            6,
            usado ? Color{ 45, 43, 49, 255 } : Color{ 91, 88, 99, 255 }
        );
        DrawCircleV({ posicion.x, posicion.y - 15.0f }, 25.0f, color);
        DrawCircleLines((int)posicion.x, (int)posicion.y - 15, 25.0f, usado ? DARKGRAY : RAYWHITE);

        if (usado)
        {
            DrawText("OK", (int)posicion.x - 14, (int)posicion.y - 25, 20, LIGHTGRAY);
        }
    }

    if (fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        float progreso = 1.0f - Clamp(
            tiempoResolucion / DURACION_EXPLOSION_INTERRUPTORES,
            0.0f,
            1.0f
        );
        DrawCircleV(centro, 80.0f + progreso * 260.0f, Fade(ORANGE, 0.34f));
        DrawCircleLines((int)centro.x, (int)centro.y, 90.0f + progreso * 330.0f, GOLD);
    }

    DrawRectangle(18, 16, 560, 116, Fade(BLACK, 0.80f));
    DrawText("INTERRUPTORES DEL CAOS", 32, 28, 30, GOLD);

    if (fase == FASE_INTERRUPTORES_PREPARACION)
    {
        DrawText(TextFormat("PREPARATE  %.1f", tiempoPreparacion), 32, 77, 23, RAYWHITE);
    }
    else if (fase == FASE_INTERRUPTORES_ELECCION)
    {
        DrawText(
            TextFormat(
                "TURNO J%d  |  TIEMPO %.1f",
                participantes[jugadorTurno].numeroJugador,
                tiempoTurno
            ),
            32,
            70,
            22,
            participantes[jugadorTurno].color
        );
        DrawText("IZQ/DER ELEGIR  |  GOLPEAR ACTIVAR", 32, 101, 17, LIGHTGRAY);
    }
    else if (fase == FASE_INTERRUPTORES_SEGURO)
    {
        DrawText("SEGURO! PASA EL TURNO", 32, 76, 22, LIME);
    }
    else if (fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        DrawText(
            TextFormat("J%d ACTIVO LA SOBRECARGA", participantes[jugadorExplosion].numeroJugador),
            32,
            76,
            22,
            RED
        );
    }
    else
    {
        DrawText("ULTIMO EN PIE  |  R PARA REINICIAR", 32, 76, 20, RAYWHITE);
    }

    int tarjetaX = ancho - 236;
    int tarjetaY = 22;
    DrawRectangle(tarjetaX - 12, tarjetaY - 8, 222, 40 + resultado.cantidadParticipantes * 42, Fade(BLACK, 0.76f));
    DrawText("ORDEN", tarjetaX, tarjetaY, 18, LIGHTGRAY);

    int fila = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        int y = tarjetaY + 31 + fila * 42;
        Color color = jugadores[i].eliminado ? Fade(participantes[i].color, 0.28f) : participantes[i].color;
        DrawCircle(tarjetaX + 14, y + 11, 12.0f, color);
        DrawText(TextFormat("J%d", participantes[i].numeroJugador), tarjetaX + 35, y, 21, color);

        if (jugadores[i].eliminado)
        {
            DrawText(TextFormat("#%d", jugadores[i].posicionFinal), tarjetaX + 122, y + 2, 18, RED);
        }
        else if (i == jugadorTurno)
        {
            DrawText("TURNO", tarjetaX + 112, y + 2, 17, GOLD);
        }
        else
        {
            DrawText("ESPERA", tarjetaX + 106, y + 2, 16, LIGHTGRAY);
        }

        fila++;
    }
}


const ResultadoMinijuego&
MinijuegoInterruptoresCaos::ObtenerResultado() const
{
    return resultado;
}
