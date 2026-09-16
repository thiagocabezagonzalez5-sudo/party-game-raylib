#include "Minigames/MinijuegoSecuenciaNeon.h"

#include "raylib.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_SECUENCIA = 2.5f;
static const float DURACION_PULSO_VISIBLE = 0.50f;
static const float DURACION_PAUSA_PULSO = 0.20f;
static const float DURACION_RESOLUCION_SECUENCIA = 1.25f;


static int ContarVivosSecuencia(
    const MinijuegoSecuenciaNeon& minijuego
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


static const char* SimboloPulsoNeon(PulsoSecuenciaNeon pulso)
{
    switch (pulso)
    {
        case PULSO_NEON_ARRIBA: return "^";
        case PULSO_NEON_DERECHA: return ">";
        case PULSO_NEON_ABAJO: return "V";
        case PULSO_NEON_IZQUIERDA: return "<";
        case CANTIDAD_PULSOS_NEON: break;
    }

    return "?";
}


static Color ColorPulsoNeon(PulsoSecuenciaNeon pulso)
{
    switch (pulso)
    {
        case PULSO_NEON_ARRIBA: return SKYBLUE;
        case PULSO_NEON_DERECHA: return ORANGE;
        case PULSO_NEON_ABAJO: return LIME;
        case PULSO_NEON_IZQUIERDA: return VIOLET;
        case CANTIDAD_PULSOS_NEON: break;
    }

    return RAYWHITE;
}


static bool LeerPulsoHumano(
    const Participante& participante,
    PulsoSecuenciaNeon& pulso
)
{
    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_ARRIBA
    ))
    {
        pulso = PULSO_NEON_ARRIBA;
        return true;
    }

    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_DERECHA
    ))
    {
        pulso = PULSO_NEON_DERECHA;
        return true;
    }

    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_ABAJO
    ))
    {
        pulso = PULSO_NEON_ABAJO;
        return true;
    }

    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_IZQUIERDA
    ))
    {
        pulso = PULSO_NEON_IZQUIERDA;
        return true;
    }

    return false;
}


static void RegistrarPulsoJugador(
    MinijuegoSecuenciaNeon& minijuego,
    int indiceParticipante,
    PulsoSecuenciaNeon pulso
)
{
    EstadoJugadorSecuenciaNeon& jugador =
        minijuego.jugadores[indiceParticipante];

    if (
        jugador.eliminado ||
        jugador.completoRonda ||
        jugador.indiceRespuesta >= minijuego.cantidadPulsos
    )
    {
        return;
    }

    if (pulso != minijuego.secuencia[jugador.indiceRespuesta])
    {
        jugador.eliminado = true;
        jugador.falloEstaRonda = true;
        return;
    }

    jugador.indiceRespuesta++;
    jugador.aciertosTotales++;

    if (jugador.indiceRespuesta >= minijuego.cantidadPulsos)
    {
        jugador.completoRonda = true;
        jugador.rondasSuperadas++;
    }
}


static bool TodosResolvidosSecuencia(
    const MinijuegoSecuenciaNeon& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado &&
            !minijuego.jugadores[i].completoRonda
        )
        {
            return false;
        }
    }

    return true;
}


static void ComenzarRespuestaSecuencia(
    MinijuegoSecuenciaNeon& minijuego
)
{
    minijuego.fase = FASE_SECUENCIA_RESPONDIENDO;
    minijuego.pulsoVisible = false;
    minijuego.tiempoRespuesta =
        1.5f + minijuego.cantidadPulsos * 0.78f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorSecuenciaNeon& jugador = minijuego.jugadores[i];

        if (
            !minijuego.resultado.participantes[i].participo ||
            jugador.eliminado
        )
        {
            continue;
        }

        jugador.indiceRespuesta = 0;
        jugador.completoRonda = false;
        jugador.falloEstaRonda = false;
        jugador.tiempoRespuestaBot = GetRandomValue(24, 50) / 100.0f;

        int probabilidadFallo = 5 + minijuego.cantidadPulsos * 3 + i * 2;
        jugador.indiceFalloBot =
            GetRandomValue(0, 99) < probabilidadFallo
            ? GetRandomValue(0, minijuego.cantidadPulsos - 1)
            : -1;
    }
}


static void ComenzarMuestraSecuencia(
    MinijuegoSecuenciaNeon& minijuego,
    bool agregarPulso
)
{
    if (agregarPulso)
    {
        minijuego.secuencia[minijuego.cantidadPulsos] =
            (PulsoSecuenciaNeon)GetRandomValue(
                0,
                CANTIDAD_PULSOS_NEON - 1
            );

        minijuego.cantidadPulsos++;
    }

    minijuego.numeroRonda++;
    minijuego.indiceMuestra = 0;
    minijuego.pulsoVisible = true;
    minijuego.tiempoPasoMuestra = DURACION_PULSO_VISIBLE;
    minijuego.fase = FASE_SECUENCIA_MOSTRANDO;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        minijuego.jugadores[i].falloEstaRonda = false;
    }
}


static int PuntuacionJugadorSecuencia(
    const EstadoJugadorSecuenciaNeon& jugador
)
{
    return
        jugador.rondasSuperadas * 100 +
        jugador.aciertosTotales +
        (!jugador.eliminado ? 1000 : 0);
}


static void FinalizarSecuenciaNeon(
    MinijuegoSecuenciaNeon& minijuego
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

        int puntuacion = PuntuacionJugadorSecuencia(minijuego.jugadores[i]);
        minijuego.jugadores[i].puntuacionFinal = puntuacion;

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

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                minijuego.jugadores[j].puntuacionFinal >
                    minijuego.jugadores[i].puntuacionFinal
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            minijuego.jugadores[i].rondasSuperadas * 100 +
            minijuego.jugadores[i].aciertosTotales;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_SECUENCIA_TERMINADO;
}


static void ComenzarResolucionSecuencia(
    MinijuegoSecuenciaNeon& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorSecuenciaNeon& jugador = minijuego.jugadores[i];

        if (
            minijuego.resultado.participantes[i].participo &&
            !jugador.eliminado &&
            !jugador.completoRonda
        )
        {
            jugador.eliminado = true;
            jugador.falloEstaRonda = true;
        }
    }

    minijuego.fase = FASE_SECUENCIA_RESOLUCION;
    minijuego.tiempoResolucion = DURACION_RESOLUCION_SECUENCIA;
}


static void DibujarPadNeon(
    Vector2 posicion,
    PulsoSecuenciaNeon pulso,
    bool encendido,
    float tiempoAnimacion
)
{
    Color color = ColorPulsoNeon(pulso);
    float radio = encendido
        ? 52.0f + std::sin(tiempoAnimacion * 10.0f) * 3.0f
        : 44.0f;

    if (encendido)
    {
        DrawCircleV(posicion, radio + 18.0f, Fade(color, 0.20f));
    }

    DrawCircleV(
        posicion,
        radio,
        encendido ? color : Fade(color, 0.20f)
    );

    DrawCircleLines(
        (int)posicion.x,
        (int)posicion.y,
        radio,
        encendido ? RAYWHITE : Fade(RAYWHITE, 0.32f)
    );

    const char* simbolo = SimboloPulsoNeon(pulso);
    int tamano = 42;

    DrawText(
        simbolo,
        (int)posicion.x - MeasureText(simbolo, tamano) / 2,
        (int)posicion.y - tamano / 2,
        tamano,
        encendido ? BLACK : Fade(RAYWHITE, 0.55f)
    );
}


void MinijuegoSecuenciaNeon::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_PULSOS_SECUENCIA_NEON; i++)
    {
        secuencia[i] = PULSO_NEON_ARRIBA;
    }

    cantidadPulsos = 0;
    indiceMuestra = 0;
    numeroRonda = 0;
    fase = FASE_SECUENCIA_PREPARACION;
    pulsoVisible = false;
    tiempoPreparacion = DURACION_PREPARACION_SECUENCIA;
    tiempoPasoMuestra = 0.0f;
    tiempoRespuesta = 0.0f;
    tiempoResolucion = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoSecuenciaNeon::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    for (int i = 0; i < 3; i++)
    {
        secuencia[i] = (PulsoSecuenciaNeon)GetRandomValue(
            0,
            CANTIDAD_PULSOS_NEON - 1
        );
    }

    cantidadPulsos = 3;
}


void MinijuegoSecuenciaNeon::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_SECUENCIA_TERMINADO)
    {
        return;
    }

    if (fase == FASE_SECUENCIA_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            ComenzarMuestraSecuencia(*this, false);
        }

        return;
    }

    if (fase == FASE_SECUENCIA_MOSTRANDO)
    {
        tiempoPasoMuestra -= deltaTime;

        if (tiempoPasoMuestra <= 0.0f)
        {
            if (pulsoVisible)
            {
                pulsoVisible = false;
                tiempoPasoMuestra = DURACION_PAUSA_PULSO;
            }
            else
            {
                indiceMuestra++;

                if (indiceMuestra >= cantidadPulsos)
                {
                    ComenzarRespuestaSecuencia(*this);
                }
                else
                {
                    pulsoVisible = true;
                    tiempoPasoMuestra = DURACION_PULSO_VISIBLE;
                }
            }
        }

        return;
    }

    if (fase == FASE_SECUENCIA_RESPONDIENDO)
    {
        tiempoRespuesta -= deltaTime;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            EstadoJugadorSecuenciaNeon& jugador = jugadores[i];

            if (
                !resultado.participantes[i].participo ||
                jugador.eliminado ||
                jugador.completoRonda
            )
            {
                continue;
            }

            if (participantes[i].esBot)
            {
                jugador.tiempoRespuestaBot -= deltaTime;

                if (jugador.tiempoRespuestaBot <= 0.0f)
                {
                    PulsoSecuenciaNeon pulso =
                        secuencia[jugador.indiceRespuesta];

                    if (jugador.indiceRespuesta == jugador.indiceFalloBot)
                    {
                        pulso = (PulsoSecuenciaNeon)(
                            (pulso + 1 + GetRandomValue(0, 2)) %
                            CANTIDAD_PULSOS_NEON
                        );
                    }

                    RegistrarPulsoJugador(*this, i, pulso);
                    jugador.tiempoRespuestaBot =
                        GetRandomValue(24, 50) / 100.0f;
                }
            }
            else
            {
                PulsoSecuenciaNeon pulso = PULSO_NEON_ARRIBA;

                if (LeerPulsoHumano(participantes[i], pulso))
                {
                    RegistrarPulsoJugador(*this, i, pulso);
                }
            }
        }

        if (TodosResolvidosSecuencia(*this) || tiempoRespuesta <= 0.0f)
        {
            ComenzarResolucionSecuencia(*this);
        }

        return;
    }

    tiempoResolucion -= deltaTime;

    if (tiempoResolucion <= 0.0f)
    {
        if (
            ContarVivosSecuencia(*this) <= 1 ||
            cantidadPulsos >= MAX_PULSOS_SECUENCIA_NEON
        )
        {
            FinalizarSecuenciaNeon(*this);
        }
        else
        {
            ComenzarMuestraSecuencia(*this, true);
        }
    }
}


void MinijuegoSecuenciaNeon::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 10, 13, 29, 255 });

    DrawRectangleGradientV(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(Color{ 38, 18, 66, 255 }, 0.62f),
        Fade(Color{ 7, 16, 31, 255 }, 0.92f)
    );

    Vector2 centro =
    {
        GetScreenWidth() * 0.5f,
        GetScreenHeight() * 0.45f
    };

    PulsoSecuenciaNeon pulsoEncendido = PULSO_NEON_ARRIBA;
    bool mostrarPulso =
        fase == FASE_SECUENCIA_MOSTRANDO &&
        pulsoVisible &&
        indiceMuestra < cantidadPulsos;

    if (mostrarPulso)
    {
        pulsoEncendido = secuencia[indiceMuestra];
    }

    DibujarPadNeon(
        { centro.x, centro.y - 100.0f },
        PULSO_NEON_ARRIBA,
        mostrarPulso && pulsoEncendido == PULSO_NEON_ARRIBA,
        tiempoAnimacion
    );

    DibujarPadNeon(
        { centro.x + 100.0f, centro.y },
        PULSO_NEON_DERECHA,
        mostrarPulso && pulsoEncendido == PULSO_NEON_DERECHA,
        tiempoAnimacion
    );

    DibujarPadNeon(
        { centro.x, centro.y + 100.0f },
        PULSO_NEON_ABAJO,
        mostrarPulso && pulsoEncendido == PULSO_NEON_ABAJO,
        tiempoAnimacion
    );

    DibujarPadNeon(
        { centro.x - 100.0f, centro.y },
        PULSO_NEON_IZQUIERDA,
        mostrarPulso && pulsoEncendido == PULSO_NEON_IZQUIERDA,
        tiempoAnimacion
    );

    DrawCircleLines((int)centro.x, (int)centro.y, 34.0f, Fade(RAYWHITE, 0.30f));
    DrawText(
        TextFormat("%d", cantidadPulsos),
        (int)centro.x - 12,
        (int)centro.y - 17,
        30,
        RAYWHITE
    );

    DrawRectangle(18, 16, 540, 116, Fade(BLACK, 0.76f));
    DrawText("SECUENCIA NEON", 32, 28, 30, GOLD);

    if (fase == FASE_SECUENCIA_PREPARACION)
    {
        DrawText("PREPARATE", 32, 68, 21, RAYWHITE);
        DrawText(
            TextFormat("EMPIEZA EN %.1f", tiempoPreparacion),
            32,
            99,
            18,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_SECUENCIA_MOSTRANDO)
    {
        DrawText("MEMORIZA LOS PULSOS", 32, 68, 21, SKYBLUE);
        DrawText(
            TextFormat("RONDA %d  -  LONGITUD %d", numeroRonda, cantidadPulsos),
            32,
            99,
            18,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_SECUENCIA_RESPONDIENDO)
    {
        DrawText("REPITE LA SECUENCIA", 32, 68, 21, LIME);
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoRespuesta),
            32,
            99,
            18,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_SECUENCIA_RESOLUCION)
    {
        DrawText("RESULTADO DE LA RONDA", 32, 74, 21, RAYWHITE);
    }
    else
    {
        DrawText("SECUENCIA TERMINADA", 32, 68, 21, RAYWHITE);
        DrawText("R PARA JUGAR DE NUEVO", 32, 99, 18, LIGHTGRAY);
    }

    int anchoPanel = 240;
    int separacion = 14;
    int cantidad = resultado.cantidadParticipantes;
    int anchoTotal = cantidad * anchoPanel + (cantidad - 1) * separacion;
    int xInicial = (GetScreenWidth() - anchoTotal) / 2;
    int y = GetScreenHeight() - 151;
    int orden = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        int x = xInicial + orden * (anchoPanel + separacion);
        const EstadoJugadorSecuenciaNeon& jugador = jugadores[i];
        Color color = jugador.eliminado
            ? Fade(participantes[i].color, 0.30f)
            : participantes[i].color;

        DrawRectangle(x, y, anchoPanel, 92, Fade(BLACK, 0.78f));
        DrawRectangleLinesEx(
            { (float)x, (float)y, (float)anchoPanel, 92.0f },
            3.0f,
            color
        );

        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            x + 12,
            y + 10,
            20,
            color
        );

        if (fase == FASE_SECUENCIA_TERMINADO)
        {
            DrawText(
                resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal),
                x + 12,
                y + 43,
                18,
                resultado.participantes[i].posicionFinal == 1
                    ? GOLD
                    : LIGHTGRAY
            );
        }
        else if (jugador.eliminado)
        {
            DrawText(
                jugador.falloEstaRonda ? "ERROR" : "ELIMINADO",
                x + 12,
                y + 43,
                18,
                RED
            );
        }
        else if (fase == FASE_SECUENCIA_RESPONDIENDO)
        {
            DrawText(
                TextFormat("%d / %d", jugador.indiceRespuesta, cantidadPulsos),
                x + 12,
                y + 43,
                18,
                RAYWHITE
            );
        }
        else
        {
            DrawText(
                TextFormat("RONDAS: %d", jugador.rondasSuperadas),
                x + 12,
                y + 43,
                18,
                LIGHTGRAY
            );
        }

        orden++;
    }
}


const ResultadoMinijuego&
MinijuegoSecuenciaNeon::ObtenerResultado() const
{
    return resultado;
}
