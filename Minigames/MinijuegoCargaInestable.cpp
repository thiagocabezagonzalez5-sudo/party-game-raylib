#include "Minigames/MinijuegoCargaInestable.h"

#include "raylib.h"
#include "raymath.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_CARGA = 2.5f;
static const float DURACION_EXPLOSION_CARGA = 1.35f;


static int ContarVivosCarga(
    const MinijuegoCargaInestable& minijuego
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


static int ElegirVivoAleatorioCarga(
    const MinijuegoCargaInestable& minijuego
)
{
    int vivos[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            vivos[cantidad] = i;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return -1;
    }

    return vivos[GetRandomValue(0, cantidad - 1)];
}


static int BuscarSiguienteVivoCarga(
    const MinijuegoCargaInestable& minijuego,
    int origen,
    int direccion
)
{
    int indice = origen;

    for (int intento = 0; intento < MAX_PARTICIPANTES; intento++)
    {
        indice += direccion;

        if (indice < 0) indice = MAX_PARTICIPANTES - 1;
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


static void PrepararCargaNueva(
    MinijuegoCargaInestable& minijuego,
    int portadorInicial
)
{
    minijuego.numeroRonda++;
    minijuego.portador = portadorInicial;
    minijuego.portadorExplosion = -1;
    minijuego.duracionCarga = GetRandomValue(590, 980) / 100.0f;
    minijuego.tiempoCarga = minijuego.duracionCarga;
    minijuego.bloqueoPase = 0.48f;
    minijuego.tiempoExplosion = 0.0f;
    minijuego.fase = FASE_CARGA_ACTIVA;

    if (portadorInicial >= 0)
    {
        minijuego.jugadores[portadorInicial].tiempoDecisionBot =
            GetRandomValue(32, 105) / 100.0f;
    }
}


static void PasarCarga(
    MinijuegoCargaInestable& minijuego,
    int direccion
)
{
    if (minijuego.portador < 0 || minijuego.bloqueoPase > 0.0f)
    {
        return;
    }

    int anterior = minijuego.portador;
    int siguiente = BuscarSiguienteVivoCarga(
        minijuego,
        anterior,
        direccion
    );

    if (siguiente == anterior)
    {
        return;
    }

    minijuego.jugadores[anterior].cantidadPases++;
    minijuego.portador = siguiente;
    minijuego.bloqueoPase = 0.24f;
    minijuego.jugadores[siguiente].tiempoDecisionBot =
        GetRandomValue(30, 110) / 100.0f;
}


static void FinalizarCargaInestable(
    MinijuegoCargaInestable& minijuego
)
{
    int ganador = ElegirVivoAleatorioCarga(minijuego);

    if (ganador >= 0)
    {
        minijuego.jugadores[ganador].posicionFinal = 1;
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
            minijuego.jugadores[i].cantidadPases * 5 +
            (MAX_PARTICIPANTES + 1 - resultadoJugador.posicionFinal) * 20;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.fase = FASE_CARGA_TERMINADO;
    minijuego.portador = ganador;
}


static Vector2 PosicionJugadorCarga(
    int orden,
    int cantidadParticipantes
)
{
    float angulo =
        -PI / 2.0f +
        orden * PI * 2.0f / cantidadParticipantes;

    float radioX = fminf(GetScreenWidth() * 0.31f, 390.0f);
    float radioY = fminf(GetScreenHeight() * 0.30f, 220.0f);

    return
    {
        GetScreenWidth() * 0.5f + std::cos(angulo) * radioX,
        GetScreenHeight() * 0.54f + std::sin(angulo) * radioY
    };
}


void MinijuegoCargaInestable::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    fase = FASE_CARGA_PREPARACION;
    portador = -1;
    portadorExplosion = -1;
    numeroRonda = 0;
    tiempoPreparacion = DURACION_PREPARACION_CARGA;
    tiempoCarga = 0.0f;
    duracionCarga = 0.0f;
    bloqueoPase = 0.0f;
    tiempoExplosion = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoCargaInestable::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    portador = ElegirVivoAleatorioCarga(*this);
}


void MinijuegoCargaInestable::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_CARGA_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CARGA_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            PrepararCargaNueva(*this, portador);
        }

        return;
    }

    if (fase == FASE_CARGA_EXPLOSION)
    {
        tiempoExplosion -= deltaTime;

        if (tiempoExplosion <= 0.0f)
        {
            if (ContarVivosCarga(*this) <= 1)
            {
                FinalizarCargaInestable(*this);
            }
            else
            {
                PrepararCargaNueva(
                    *this,
                    ElegirVivoAleatorioCarga(*this)
                );
            }
        }

        return;
    }

    bloqueoPase -= deltaTime;
    if (bloqueoPase < 0.0f) bloqueoPase = 0.0f;

    tiempoCarga -= deltaTime;

    if (tiempoCarga <= 0.0f)
    {
        int vivosAntes = ContarVivosCarga(*this);

        if (portador >= 0)
        {
            jugadores[portador].eliminado = true;
            jugadores[portador].posicionFinal = vivosAntes;
        }

        portadorExplosion = portador;
        portador = -1;
        fase = FASE_CARGA_EXPLOSION;
        tiempoExplosion = DURACION_EXPLOSION_CARGA;
        return;
    }

    if (portador < 0 || bloqueoPase > 0.0f)
    {
        return;
    }

    if (participantes[portador].esBot)
    {
        jugadores[portador].tiempoDecisionBot -= deltaTime;

        if (jugadores[portador].tiempoDecisionBot <= 0.0f)
        {
            PasarCarga(
                *this,
                GetRandomValue(0, 1) == 0 ? -1 : 1
            );
        }

        return;
    }

    InputMinijuegoParticipante entrada =
        LeerInputMinijuegoParticipante(participantes[portador]);

    if (entrada.golpear)
    {
        PasarCarga(*this, 1);
    }
    else if (entrada.saltar)
    {
        PasarCarga(*this, -1);
    }
}


void MinijuegoCargaInestable::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 26, 18, 28, 255 });

    Vector2 centro =
    {
        GetScreenWidth() * 0.5f,
        GetScreenHeight() * 0.54f
    };

    DrawCircleV(centro, 160.0f, Color{ 44, 31, 49, 255 });
    DrawCircleLines((int)centro.x, (int)centro.y, 160.0f, VIOLET);
    DrawCircleLines((int)centro.x, (int)centro.y, 150.0f, Fade(PINK, 0.35f));

    int ordenes[MAX_PARTICIPANTES]{};
    int cantidadActivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            ordenes[cantidadActivos] = i;
            cantidadActivos++;
        }
    }

    for (int orden = 0; orden < cantidadActivos; orden++)
    {
        int i = ordenes[orden];
        Vector2 posicion = PosicionJugadorCarga(orden, cantidadActivos);
        bool eliminado = jugadores[i].eliminado;
        Color color = eliminado
            ? Fade(participantes[i].color, 0.28f)
            : participantes[i].color;

        DrawLineEx(centro, posicion, 4.0f, Fade(color, 0.22f));
        DrawCircleV(posicion, 48.0f, Fade(BLACK, 0.72f));
        DrawCircleV(posicion, 38.0f, color);
        DrawCircleLines((int)posicion.x, (int)posicion.y, 48.0f, color);

        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            (int)posicion.x - 17,
            (int)posicion.y - 13,
            24,
            eliminado ? DARKGRAY : BLACK
        );

        if (eliminado)
        {
            DrawLineEx(
                { posicion.x - 32.0f, posicion.y - 32.0f },
                { posicion.x + 32.0f, posicion.y + 32.0f },
                7.0f,
                RED
            );

            DrawLineEx(
                { posicion.x + 32.0f, posicion.y - 32.0f },
                { posicion.x - 32.0f, posicion.y + 32.0f },
                7.0f,
                RED
            );
        }

        bool tieneCarga = i == portador;
        bool recibioExplosion =
            fase == FASE_CARGA_EXPLOSION && i == portadorExplosion;

        if (tieneCarga)
        {
            float avance = duracionCarga > 0.0f
                ? 1.0f - Clamp(tiempoCarga / duracionCarga, 0.0f, 1.0f)
                : 0.0f;

            float pulso = std::sin(tiempoAnimacion * (5.0f + avance * 9.0f));
            float radio = 18.0f + avance * 14.0f + pulso * 2.5f;

            Vector2 direccionCentro = Vector2Normalize(
                Vector2Subtract(centro, posicion)
            );

            Vector2 posicionCarga = Vector2Add(
                posicion,
                Vector2Scale(direccionCentro, 68.0f)
            );

            Color colorCarga = ColorLerp(
                Color{ 92, 211, 255, 255 },
                RED,
                avance
            );

            DrawCircleV(posicionCarga, radio + 8.0f, Fade(colorCarga, 0.20f));
            DrawCircleV(posicionCarga, radio, colorCarga);
            DrawCircleLines(
                (int)posicionCarga.x,
                (int)posicionCarga.y,
                radio + 4.0f,
                RAYWHITE
            );
        }

        if (recibioExplosion)
        {
            float progreso = 1.0f - Clamp(
                tiempoExplosion / DURACION_EXPLOSION_CARGA,
                0.0f,
                1.0f
            );

            DrawCircleV(posicion, 35.0f + progreso * 105.0f, Fade(ORANGE, 0.48f));
            DrawCircleLines(
                (int)posicion.x,
                (int)posicion.y,
                55.0f + progreso * 130.0f,
                GOLD
            );
        }
    }

    DrawRectangle(18, 16, 540, 118, Fade(BLACK, 0.78f));
    DrawText("CARGA INESTABLE", 32, 28, 30, GOLD);

    if (fase == FASE_CARGA_PREPARACION)
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
    else if (fase == FASE_CARGA_ACTIVA)
    {
        DrawText("GOLPEAR: HORARIO   SALTAR: ANTIHORARIO", 32, 68, 18, RAYWHITE);
        DrawText(
            TextFormat("RONDA %d  -  PASALA ANTES DE QUE EXPLOTE", numeroRonda),
            32,
            100,
            17,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_CARGA_EXPLOSION)
    {
        DrawText("SOBRECARGA!", 32, 74, 26, RED);
    }
    else
    {
        DrawText("ULTIMO EN PIE", 32, 68, 22, RAYWHITE);
        DrawText("R PARA JUGAR DE NUEVO", 32, 100, 18, LIGHTGRAY);
    }

    if (fase == FASE_CARGA_TERMINADO && portador >= 0)
    {
        DrawText(
            TextFormat("GANADOR: J%d", participantes[portador].numeroJugador),
            GetScreenWidth() / 2 - 105,
            GetScreenHeight() / 2 - 18,
            30,
            GOLD
        );
    }
}


const ResultadoMinijuego&
MinijuegoCargaInestable::ObtenerResultado() const
{
    return resultado;
}
