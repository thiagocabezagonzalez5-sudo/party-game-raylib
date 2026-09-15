#include "Minigames/MinijuegoCircuitoVoltaje.h"

#include "raylib.h"
#include "raymath.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_CIRCUITO = 2.5f;
static const float DURACION_CARRERA_CIRCUITO = 45.0f;
static const float VUELTAS_CIRCUITO = 4.0f;


static float FraccionVueltaCircuito(float avance)
{
    float fraccion = avance - std::floor(avance);
    return fraccion < 0.0f ? fraccion + 1.0f : fraccion;
}


static float IntensidadCurvaCircuito(float avance)
{
    float angulo =
        -PI / 2.0f +
        FraccionVueltaCircuito(avance) * PI * 2.0f;

    float curva = std::fabs(std::cos(angulo));
    return curva * curva;
}


static float VelocidadSeguraCircuito(float avance)
{
    return 1.0f - IntensidadCurvaCircuito(avance) * 0.42f;
}


static int PuntuacionAvanceCircuito(
    const EstadoJugadorCircuitoVoltaje& jugador
)
{
    return (int)(Clamp(
        jugador.avance,
        0.0f,
        VUELTAS_CIRCUITO
    ) * 10000.0f + 0.5f);
}


static void FinalizarCircuitoVoltaje(
    MinijuegoCircuitoVoltaje& minijuego
)
{
    int mejorAvance = -1;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        int avance = PuntuacionAvanceCircuito(minijuego.jugadores[i]);

        if (avance > mejorAvance)
        {
            mejorAvance = avance;
            cantidadGanadores = 1;
        }
        else if (avance == mejorAvance)
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

        int avance = PuntuacionAvanceCircuito(minijuego.jugadores[i]);
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                PuntuacionAvanceCircuito(minijuego.jugadores[j]) > avance
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            avance / 40 - minijuego.jugadores[i].trompos * 10;

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

    minijuego.fase = FASE_CIRCUITO_TERMINADO;
}


static void ActualizarBotCircuito(
    EstadoJugadorCircuitoVoltaje& jugador,
    int indiceParticipante,
    float tiempoAnimacion,
    bool& acelerando
)
{
    float velocidadSegura = VelocidadSeguraCircuito(jugador.avance);
    float margen = 0.035f + indiceParticipante * 0.012f;

    acelerando = jugador.velocidad < velocidadSegura - margen;

    float pulsoError = std::sin(
        tiempoAnimacion * 0.72f + indiceParticipante * 1.9f
    );

    if (pulsoError > 0.965f)
    {
        acelerando = true;
    }
}


static void DibujarAutoCircuito(
    Vector2 posicion,
    float rotacion,
    Color color,
    bool enTrompo,
    float tiempoAnimacion
)
{
    if (enTrompo)
    {
        rotacion += tiempoAnimacion * 720.0f;
    }

    DrawCircle(
        (int)posicion.x,
        (int)posicion.y + 5,
        13.0f,
        Fade(BLACK, 0.35f)
    );

    DrawRectanglePro(
        { posicion.x, posicion.y, 30.0f, 17.0f },
        { 15.0f, 8.5f },
        rotacion,
        color
    );

    Vector2 frente =
    {
        posicion.x + std::cos(rotacion * DEG2RAD) * 12.0f,
        posicion.y + std::sin(rotacion * DEG2RAD) * 12.0f
    };

    DrawCircleV(frente, 3.0f, RAYWHITE);
}


void MinijuegoCircuitoVoltaje::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    fase = FASE_CIRCUITO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_CIRCUITO;
    tiempoCarrera = DURACION_CARRERA_CIRCUITO;
    tiempoAnimacion = 0.0f;
}


void MinijuegoCircuitoVoltaje::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


void MinijuegoCircuitoVoltaje::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_CIRCUITO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CIRCUITO_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CIRCUITO_CARRERA;
        }

        return;
    }

    tiempoCarrera -= deltaTime;
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

        EstadoJugadorCircuitoVoltaje& jugador = jugadores[i];

        if (jugador.tiempoTrompo > 0.0f)
        {
            jugador.tiempoTrompo -= deltaTime;
            jugador.velocidad -= 0.55f * deltaTime;

            if (jugador.velocidad < 0.08f)
            {
                jugador.velocidad = 0.08f;
            }

            continue;
        }

        bool acelerando = false;

        if (participantes[i].esBot)
        {
            ActualizarBotCircuito(
                jugador,
                i,
                tiempoAnimacion,
                acelerando
            );
        }
        else
        {
            InputMinijuegoParticipante entrada =
                LeerInputMinijuegoParticipante(participantes[i]);

            acelerando = entrada.adelante;
        }

        if (acelerando)
        {
            jugador.velocidad += 0.78f * deltaTime;
        }
        else
        {
            jugador.velocidad -= 0.52f * deltaTime;
        }

        jugador.velocidad = Clamp(jugador.velocidad, 0.0f, 1.08f);

        float velocidadSegura = VelocidadSeguraCircuito(jugador.avance);

        if (jugador.velocidad > velocidadSegura + 0.075f)
        {
            jugador.tiempoExcesoCurva += deltaTime;
        }
        else
        {
            jugador.tiempoExcesoCurva -= deltaTime * 2.2f;

            if (jugador.tiempoExcesoCurva < 0.0f)
            {
                jugador.tiempoExcesoCurva = 0.0f;
            }
        }

        if (jugador.tiempoExcesoCurva >= 0.33f)
        {
            jugador.tiempoTrompo = 1.05f;
            jugador.tiempoExcesoCurva = 0.0f;
            jugador.velocidad = 0.24f;
            jugador.trompos++;
            continue;
        }

        jugador.avance += jugador.velocidad * 0.175f * deltaTime;

        if (jugador.avance >= VUELTAS_CIRCUITO)
        {
            jugador.avance = VUELTAS_CIRCUITO;
            jugador.llegoMeta = true;
            alguienLlego = true;
        }
    }

    if (alguienLlego || tiempoCarrera <= 0.0f)
    {
        tiempoCarrera = tiempoCarrera < 0.0f ? 0.0f : tiempoCarrera;
        FinalizarCircuitoVoltaje(*this);
    }
}


void MinijuegoCircuitoVoltaje::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 22, 35, 31, 255 });

    int centroX = GetScreenWidth() / 2;
    int centroY = GetScreenHeight() / 2 + 35;

    DrawEllipse(
        centroX,
        centroY,
        390.0f,
        245.0f,
        Color{ 56, 59, 66, 255 }
    );

    DrawEllipse(
        centroX,
        centroY,
        205.0f,
        78.0f,
        Color{ 30, 78, 53, 255 }
    );

    for (int i = 0; i <= MAX_PARTICIPANTES; i++)
    {
        DrawEllipseLines(
            centroX,
            centroY,
            365.0f - i * 34.0f,
            220.0f - i * 27.0f,
            i == 0 || i == MAX_PARTICIPANTES
                ? RAYWHITE
                : Fade(RAYWHITE, 0.30f)
        );
    }

    DrawRectangle(
        centroX - 16,
        centroY - 224,
        32,
        112,
        RAYWHITE
    );

    for (int fila = 0; fila < 7; fila++)
    {
        for (int columna = 0; columna < 2; columna++)
        {
            if ((fila + columna) % 2 == 0)
            {
                DrawRectangle(
                    centroX - 16 + columna * 16,
                    centroY - 224 + fila * 16,
                    16,
                    16,
                    BLACK
                );
            }
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        float radioX = 348.0f - i * 34.0f;
        float radioY = 206.5f - i * 27.0f;
        float angulo =
            -PI / 2.0f +
            FraccionVueltaCircuito(jugadores[i].avance) * PI * 2.0f;

        Vector2 posicion =
        {
            centroX + std::cos(angulo) * radioX,
            centroY + std::sin(angulo) * radioY
        };

        Vector2 tangente =
        {
            -radioX * std::sin(angulo),
            radioY * std::cos(angulo)
        };

        float rotacion = std::atan2(tangente.y, tangente.x) * RAD2DEG;

        DibujarAutoCircuito(
            posicion,
            rotacion,
            participantes[i].color,
            jugadores[i].tiempoTrompo > 0.0f,
            tiempoAnimacion
        );
    }

    DrawRectangle(18, 16, 480, 118, Fade(BLACK, 0.78f));
    DrawText("CIRCUITO VOLTAJE", 32, 28, 30, GOLD);

    if (fase == FASE_CIRCUITO_PREPARACION)
    {
        DrawText("PREPARATE", 32, 68, 22, RAYWHITE);
        DrawText(
            TextFormat("EMPIEZA EN %.1f", tiempoPreparacion),
            32,
            100,
            18,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_CIRCUITO_CARRERA)
    {
        DrawText("MANTEN ADELANTE; SUELTA EN CURVAS", 32, 68, 19, RAYWHITE);
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoCarrera),
            32,
            100,
            18,
            LIGHTGRAY
        );
    }
    else
    {
        DrawText("CARRERA TERMINADA", 32, 68, 22, RAYWHITE);
        DrawText("R PARA CORRER DE NUEVO", 32, 100, 18, LIGHTGRAY);
    }

    int panelX = GetScreenWidth() - 250;
    int panelY = 20;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        DrawRectangle(panelX, panelY, 226, 64, Fade(BLACK, 0.76f));
        DrawRectangle(panelX, panelY, 8, 64, participantes[i].color);

        int vuelta = (int)std::floor(jugadores[i].avance) + 1;
        if (vuelta > (int)VUELTAS_CIRCUITO) vuelta = (int)VUELTAS_CIRCUITO;

        DrawText(
            TextFormat("J%d  VUELTA %d/4", participantes[i].numeroJugador, vuelta),
            panelX + 18,
            panelY + 9,
            18,
            participantes[i].color
        );

        if (fase == FASE_CIRCUITO_TERMINADO)
        {
            DrawText(
                resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal),
                panelX + 18,
                panelY + 36,
                17,
                resultado.participantes[i].posicionFinal == 1
                    ? GOLD
                    : LIGHTGRAY
            );
        }
        else if (jugadores[i].tiempoTrompo > 0.0f)
        {
            DrawText("TROMPO!", panelX + 18, panelY + 36, 17, RED);
        }
        else
        {
            DrawText(
                TextFormat("VELOCIDAD %d%%", (int)(jugadores[i].velocidad * 100.0f)),
                panelX + 18,
                panelY + 36,
                16,
                LIGHTGRAY
            );
        }

        panelY += 72;
    }
}


const ResultadoMinijuego&
MinijuegoCircuitoVoltaje::ObtenerResultado() const
{
    return resultado;
}
