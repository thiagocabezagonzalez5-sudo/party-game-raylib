#include "Minigames/MinijuegoTrazoPerfecto.h"

#include "raymath.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_TRAZO = 2.5f;
static const float DURACION_TRAZO_PERFECTO = 18.0f;
static const float VELOCIDAD_CURSOR_TRAZO = 0.72f;


static const Vector2 PUNTOS_CRISTAL[] =
{
    { 0.00f, -0.92f },
    { 0.66f, -0.30f },
    { 0.50f,  0.62f },
    { 0.00f,  0.92f },
    {-0.50f,  0.62f },
    {-0.66f, -0.30f }
};


static const Vector2 PUNTOS_RAYO[] =
{
    {-0.18f, -0.92f },
    { 0.56f, -0.92f },
    { 0.16f, -0.16f },
    { 0.65f, -0.16f },
    {-0.44f,  0.92f },
    {-0.15f,  0.18f },
    {-0.64f,  0.18f }
};


static const Vector2 PUNTOS_COHETE[] =
{
    { 0.00f, -0.94f },
    { 0.38f, -0.38f },
    { 0.43f,  0.34f },
    { 0.80f,  0.76f },
    { 0.25f,  0.62f },
    { 0.00f,  0.92f },
    {-0.25f,  0.62f },
    {-0.80f,  0.76f },
    {-0.43f,  0.34f },
    {-0.38f, -0.38f }
};


static const Vector2* ObtenerPuntosTrazo(
    FormaTrazoPerfecto forma,
    int& cantidad
)
{
    switch (forma)
    {
        case FORMA_TRAZO_CRISTAL:
            cantidad = sizeof(PUNTOS_CRISTAL) / sizeof(PUNTOS_CRISTAL[0]);
            return PUNTOS_CRISTAL;

        case FORMA_TRAZO_RAYO:
            cantidad = sizeof(PUNTOS_RAYO) / sizeof(PUNTOS_RAYO[0]);
            return PUNTOS_RAYO;

        case FORMA_TRAZO_COCHETE:
            cantidad = sizeof(PUNTOS_COHETE) / sizeof(PUNTOS_COHETE[0]);
            return PUNTOS_COHETE;

        case CANTIDAD_FORMAS_TRAZO:
            break;
    }

    cantidad = 0;
    return nullptr;
}


static const char* NombreFormaTrazo(FormaTrazoPerfecto forma)
{
    switch (forma)
    {
        case FORMA_TRAZO_CRISTAL: return "CRISTAL";
        case FORMA_TRAZO_RAYO: return "RAYO";
        case FORMA_TRAZO_COCHETE: return "COHETE";
        case CANTIDAD_FORMAS_TRAZO: break;
    }

    return "FIGURA";
}


static Vector2 PuntoRecorridoTrazo(
    FormaTrazoPerfecto forma,
    float progreso
)
{
    int cantidad = 0;
    const Vector2* puntos = ObtenerPuntosTrazo(forma, cantidad);

    if (puntos == nullptr || cantidad <= 0)
    {
        return {};
    }

    float longitudTotal = 0.0f;

    for (int i = 0; i < cantidad; i++)
    {
        longitudTotal += Vector2Distance(
            puntos[i],
            puntos[(i + 1) % cantidad]
        );
    }

    float distanciaObjetivo =
        Clamp(progreso, 0.0f, 1.0f) * longitudTotal;

    for (int i = 0; i < cantidad; i++)
    {
        Vector2 inicio = puntos[i];
        Vector2 fin = puntos[(i + 1) % cantidad];
        float longitudSegmento = Vector2Distance(inicio, fin);

        if (
            distanciaObjetivo <= longitudSegmento ||
            i == cantidad - 1
        )
        {
            float avanceSegmento = longitudSegmento > 0.0f
                ? distanciaObjetivo / longitudSegmento
                : 0.0f;

            return Vector2Lerp(
                inicio,
                fin,
                Clamp(avanceSegmento, 0.0f, 1.0f)
            );
        }

        distanciaObjetivo -= longitudSegmento;
    }

    return puntos[0];
}


static int CalcularPuntuacionTrazo(
    const EstadoJugadorTrazoPerfecto& jugador
)
{
    if (jugador.tiempoEvaluado <= 0.0f)
    {
        return 0;
    }

    return (int)std::round(
        Clamp(
            jugador.precisionAcumulada / jugador.tiempoEvaluado,
            0.0f,
            1.0f
        ) * 100.0f
    );
}


static void FinalizarTrazoPerfecto(
    MinijuegoTrazoPerfecto& minijuego
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

        EstadoJugadorTrazoPerfecto& jugador = minijuego.jugadores[i];
        jugador.puntuacionFinal = CalcularPuntuacionTrazo(jugador);

        if (jugador.puntuacionFinal > mejorPuntuacion)
        {
            mejorPuntuacion = jugador.puntuacionFinal;
            cantidadGanadores = 1;
        }
        else if (jugador.puntuacionFinal == mejorPuntuacion)
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
            minijuego.jugadores[i].puntuacionFinal;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_TRAZO_TERMINADO;
}


static Vector2 EntradaDireccionTrazo(
    const Participante& participante
)
{
    InputMinijuegoParticipante entrada =
        LeerInputMinijuegoParticipante(participante);

    Vector2 direccion =
    {
        (entrada.derecha ? 1.0f : 0.0f) -
            (entrada.izquierda ? 1.0f : 0.0f),
        (entrada.atras ? 1.0f : 0.0f) -
            (entrada.adelante ? 1.0f : 0.0f)
    };

    if (Vector2LengthSqr(direccion) > 1.0f)
    {
        direccion = Vector2Normalize(direccion);
    }

    return direccion;
}


static void ActualizarCursorBotTrazo(
    EstadoJugadorTrazoPerfecto& jugador,
    Vector2 objetivo,
    float tiempoAnimacion,
    float deltaTime
)
{
    Vector2 objetivoImperfecto =
    {
        objetivo.x + std::sin(
            tiempoAnimacion * 1.7f + jugador.desfaseBot
        ) * jugador.errorBot,
        objetivo.y + std::cos(
            tiempoAnimacion * 1.35f + jugador.desfaseBot * 0.7f
        ) * jugador.errorBot
    };

    Vector2 diferencia = Vector2Subtract(objetivoImperfecto, jugador.cursor);
    float distancia = Vector2Length(diferencia);

    if (distancia > 0.001f)
    {
        float paso = VELOCIDAD_CURSOR_TRAZO * deltaTime;

        if (paso >= distancia)
        {
            jugador.cursor = objetivoImperfecto;
        }
        else
        {
            jugador.cursor = Vector2Add(
                jugador.cursor,
                Vector2Scale(Vector2Normalize(diferencia), paso)
            );
        }
    }
}


static Vector2 TransformarPuntoPanelTrazo(
    Vector2 punto,
    Rectangle panel
)
{
    float altoDibujo = panel.height - 54.0f;
    float escala = fminf(panel.width * 0.35f, altoDibujo * 0.42f);

    return
    {
        panel.x + panel.width * 0.5f + punto.x * escala,
        panel.y + 46.0f + altoDibujo * 0.5f + punto.y * escala
    };
}


static Rectangle ObtenerPanelTrazo(
    int orden,
    int cantidadParticipantes
)
{
    int columnas = cantidadParticipantes <= 2
        ? cantidadParticipantes
        : 2;

    int filas = (cantidadParticipantes + columnas - 1) / columnas;

    float margenX = 22.0f;
    float inicioY = 112.0f;
    float margenInferior = 22.0f;
    float separacion = 12.0f;

    float ancho =
        (GetScreenWidth() - margenX * 2.0f -
            separacion * (columnas - 1)) /
        columnas;

    float alto =
        (GetScreenHeight() - inicioY - margenInferior -
            separacion * (filas - 1)) /
        filas;

    int columna = orden % columnas;
    int fila = orden / columnas;

    return
    {
        margenX + columna * (ancho + separacion),
        inicioY + fila * (alto + separacion),
        ancho,
        alto
    };
}


void MinijuegoTrazoPerfecto::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    fase = FASE_TRAZO_PREPARACION;
    forma = FORMA_TRAZO_CRISTAL;
    tiempoPreparacion = DURACION_PREPARACION_TRAZO;
    tiempoTrazo = DURACION_TRAZO_PERFECTO;
    tiempoAnimacion = 0.0f;
    progresoObjetivo = 0.0f;
}


void MinijuegoTrazoPerfecto::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    forma = (FormaTrazoPerfecto)GetRandomValue(
        0,
        CANTIDAD_FORMAS_TRAZO - 1
    );

    Vector2 inicio = PuntoRecorridoTrazo(forma, 0.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].cursor = inicio;
        jugadores[i].huellas[0] = inicio;
        jugadores[i].cantidadHuellas = 1;
        jugadores[i].desfaseBot = GetRandomValue(0, 628) / 100.0f;
        jugadores[i].errorBot = 0.025f + i * 0.012f;
    }
}


void MinijuegoTrazoPerfecto::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_TRAZO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_TRAZO_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_TRAZO_DIBUJANDO;
        }

        return;
    }

    tiempoTrazo -= deltaTime;
    progresoObjetivo = 1.0f -
        Clamp(tiempoTrazo / DURACION_TRAZO_PERFECTO, 0.0f, 1.0f);

    Vector2 objetivo = PuntoRecorridoTrazo(forma, progresoObjetivo);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorTrazoPerfecto& jugador = jugadores[i];

        if (participantes[i].esBot)
        {
            ActualizarCursorBotTrazo(
                jugador,
                objetivo,
                tiempoAnimacion,
                deltaTime
            );
        }
        else
        {
            Vector2 direccion = EntradaDireccionTrazo(participantes[i]);
            jugador.cursor = Vector2Add(
                jugador.cursor,
                Vector2Scale(
                    direccion,
                    VELOCIDAD_CURSOR_TRAZO * deltaTime
                )
            );
        }

        jugador.cursor.x = Clamp(jugador.cursor.x, -1.05f, 1.05f);
        jugador.cursor.y = Clamp(jugador.cursor.y, -1.05f, 1.05f);

        float distancia = Vector2Distance(jugador.cursor, objetivo);
        float precision = 1.0f - Clamp(distancia / 0.42f, 0.0f, 1.0f);
        precision *= precision;

        jugador.precisionAcumulada += precision * deltaTime;
        jugador.tiempoEvaluado += deltaTime;
        jugador.tiempoNuevaHuella -= deltaTime;

        if (
            jugador.tiempoNuevaHuella <= 0.0f &&
            jugador.cantidadHuellas < MAX_HUELLAS_TRAZO_PERFECTO
        )
        {
            jugador.huellas[jugador.cantidadHuellas] = jugador.cursor;
            jugador.cantidadHuellas++;
            jugador.tiempoNuevaHuella = 0.075f;
        }
    }

    if (tiempoTrazo <= 0.0f)
    {
        tiempoTrazo = 0.0f;
        progresoObjetivo = 1.0f;
        FinalizarTrazoPerfecto(*this);
    }
}


void MinijuegoTrazoPerfecto::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 24, 20, 35, 255 });

    DrawText("TRAZO PERFECTO", 24, 20, 30, GOLD);
    DrawText(
        TextFormat("FIGURA: %s", NombreFormaTrazo(forma)),
        24,
        58,
        19,
        LIGHTGRAY
    );

    if (fase == FASE_TRAZO_PREPARACION)
    {
        DrawText(
            TextFormat("PREPARATE: %.1f", tiempoPreparacion),
            GetScreenWidth() - 260,
            26,
            22,
            RAYWHITE
        );
    }
    else if (fase == FASE_TRAZO_DIBUJANDO)
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoTrazo),
            GetScreenWidth() - 218,
            26,
            22,
            RAYWHITE
        );

        DrawText(
            "MUEVE TU MARCADOR Y SIGUE EL PUNTO DORADO",
            GetScreenWidth() - 470,
            60,
            17,
            LIGHTGRAY
        );
    }
    else
    {
        DrawText("RESULTADOS", GetScreenWidth() - 190, 26, 22, RAYWHITE);
        DrawText("R PARA REPETIR", GetScreenWidth() - 190, 60, 17, LIGHTGRAY);
    }

    int cantidadPuntos = 0;
    const Vector2* puntos = ObtenerPuntosTrazo(forma, cantidadPuntos);
    Vector2 objetivo = PuntoRecorridoTrazo(forma, progresoObjetivo);

    int orden = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Rectangle panel = ObtenerPanelTrazo(
            orden,
            resultado.cantidadParticipantes
        );

        DrawRectangleRec(panel, Fade(BLACK, 0.58f));
        DrawRectangleLinesEx(panel, 3.0f, participantes[i].color);

        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            (int)panel.x + 12,
            (int)panel.y + 10,
            20,
            participantes[i].color
        );

        int puntuacion = fase == FASE_TRAZO_TERMINADO
            ? jugadores[i].puntuacionFinal
            : CalcularPuntuacionTrazo(jugadores[i]);

        DrawText(
            TextFormat("PRECISION %d%%", puntuacion),
            (int)(panel.x + panel.width - 164.0f),
            (int)panel.y + 12,
            18,
            RAYWHITE
        );

        for (int punto = 0; punto < cantidadPuntos; punto++)
        {
            Vector2 inicio = TransformarPuntoPanelTrazo(
                puntos[punto],
                panel
            );

            Vector2 fin = TransformarPuntoPanelTrazo(
                puntos[(punto + 1) % cantidadPuntos],
                panel
            );

            DrawLineEx(inicio, fin, 7.0f, Fade(RAYWHITE, 0.24f));
            DrawLineEx(inicio, fin, 2.0f, Fade(SKYBLUE, 0.70f));
        }

        for (int huella = 1; huella < jugadores[i].cantidadHuellas; huella++)
        {
            DrawLineEx(
                TransformarPuntoPanelTrazo(
                    jugadores[i].huellas[huella - 1],
                    panel
                ),
                TransformarPuntoPanelTrazo(
                    jugadores[i].huellas[huella],
                    panel
                ),
                3.0f,
                Fade(participantes[i].color, 0.64f)
            );
        }

        Vector2 posicionObjetivo = TransformarPuntoPanelTrazo(
            objetivo,
            panel
        );

        Vector2 posicionCursor = TransformarPuntoPanelTrazo(
            jugadores[i].cursor,
            panel
        );

        float pulso = 8.0f + std::sin(tiempoAnimacion * 6.0f) * 2.0f;
        DrawCircleV(posicionObjetivo, pulso, GOLD);
        DrawCircleLines(
            (int)posicionObjetivo.x,
            (int)posicionObjetivo.y,
            pulso + 4.0f,
            RAYWHITE
        );

        DrawCircleV(posicionCursor, 9.0f, participantes[i].color);
        DrawCircleLines(
            (int)posicionCursor.x,
            (int)posicionCursor.y,
            11.0f,
            RAYWHITE
        );

        if (fase == FASE_TRAZO_TERMINADO)
        {
            const char* texto =
                resultado.participantes[i].posicionFinal == 1
                ? "GANADOR"
                : TextFormat(
                    "PUESTO %d",
                    resultado.participantes[i].posicionFinal
                );

            DrawText(
                texto,
                (int)panel.x + 12,
                (int)(panel.y + panel.height - 30.0f),
                18,
                resultado.participantes[i].posicionFinal == 1
                    ? GOLD
                    : LIGHTGRAY
            );
        }

        orden++;
    }
}


const ResultadoMinijuego&
MinijuegoTrazoPerfecto::ObtenerResultado() const
{
    return resultado;
}
