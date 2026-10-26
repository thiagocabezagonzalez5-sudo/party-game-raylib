#include "Minigames/MinijuegoCapitanManda.h"
#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Systems/CalidadGrafica.h"
#include "UI/EstiloUI.h"

#define SOMBRAS_RETRO_AUTOMATICAS
#include "Minigames/SombrasRetro.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_CAPITAN = 3.0f;
static const float DURACION_RESOLUCION = 0.85f;
static const int MAX_RONDAS_CAPITAN = 40;
static const float ESPERA_MINIMA_ENTRE_RONDAS_CAPITAN = 0.45f;
static const float ESPERA_MAXIMA_ENTRE_RONDAS_CAPITAN = 1.45f;

// Solo visual.
static const float DURACION_CAIDA_CAPITAN = 0.7f;
static const float NIVEL_MAR_CAPITAN = -1.3f;
static const float ALTURA_ALCAZAR_CAPITAN = 1.5f;
static const float Z_FRENTE_ALCAZAR_CAPITAN = -1.2f;
static const float Z_POPA_CAPITAN = -4.9f;
static const float Z_PROA_CAPITAN = 9.0f;
static const float X_AMURADA_CAPITAN = 5.25f;
static const float Z_CAPITAN = -2.7f;
static const float ESCALA_CAPITAN = 1.15f;
static const float ESCALA_JUGADOR_CAPITAN = 0.9f;

static const Vector3 POSICIONES_JUGADOR_CAPITAN[MAX_PARTICIPANTES] =
{
    { -4.0f, 0.0f, 2.0f },
    { -1.8f, 0.0f, 2.6f },
    { 1.8f, 0.0f, 2.6f },
    { 4.0f, 0.0f, 2.0f }
};

// Donde flota cada uno al caer por la borda: mar abierto detras de la
// amurada, a distinta distancia para que no se tapen entre si.
static const Vector3 POSICIONES_AGUA_CAPITAN[MAX_PARTICIPANTES] =
{
    { -9.0f, 0.0f, -8.0f },
    { -14.0f, 0.0f, -12.0f },
    { 14.0f, 0.0f, -12.0f },
    { 9.0f, 0.0f, -8.0f }
};

// Banderas: un color distinto para cada lado (azul a la izquierda,
// naranja a la derecha), las dos con la misma franja blanca junto al asta.
static const Color COLOR_BANDERA_IZQUIERDA_CAPITAN = { 46, 112, 214, 255 };
static const Color COLOR_BANDERA_DERECHA_CAPITAN = { 236, 116, 40, 255 };
static const Color COLOR_NIEBLA_CAPITAN = { 236, 204, 178, 255 };


//==================================================
// EFECTOS VISUALES (sin efecto en la logica)
//==================================================

static float Limitar01Capitan(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float HashCapitan(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue que sortea
// las ordenes y los bots.
static float AzarCapitan(MinijuegoCapitanManda& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasCapitan(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static void EmitirParticulaCapitan(
    MinijuegoCapitanManda& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_CAPITAN; i++)
    {
        ParticulaCapitanManda& p = minijuego.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.color = color;
        return;
    }
}


static void EmitirChispasCapitan(MinijuegoCapitanManda& minijuego, Vector3 centro)
{
    int cantidad = CantidadParticulasCapitan(12);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarCapitan(minijuego, 0.0f, 6.2832f);
        float r = AzarCapitan(minijuego, 1.2f, 2.6f);
        Color color = k % 3 == 0 ? Color{ 255, 246, 196, 255 } : Color{ 255, 206, 84, 255 };

        EmitirParticulaCapitan(
            minijuego,
            centro,
            { std::cos(a) * r, AzarCapitan(minijuego, 2.0f, 4.2f), std::sin(a) * r * 0.6f },
            AzarCapitan(minijuego, 0.35f, 0.6f),
            AzarCapitan(minijuego, 0.06f, 0.11f),
            7.5f,
            color
        );
    }
}


static void EmitirSalpicaduraCapitan(MinijuegoCapitanManda& minijuego, Vector3 centro)
{
    int cantidad = CantidadParticulasCapitan(26);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarCapitan(minijuego, 0.0f, 6.2832f);
        float r = AzarCapitan(minijuego, 0.6f, 2.4f);
        Color color = k % 2 == 0 ? Color{ 236, 246, 250, 255 } : Color{ 150, 206, 232, 255 };

        EmitirParticulaCapitan(
            minijuego,
            { centro.x + std::cos(a) * 0.3f, NIVEL_MAR_CAPITAN + 0.1f, centro.z + std::sin(a) * 0.3f },
            { std::cos(a) * r, AzarCapitan(minijuego, 3.0f, 6.5f), std::sin(a) * r },
            AzarCapitan(minijuego, 0.5f, 0.9f),
            AzarCapitan(minijuego, 0.08f, 0.16f),
            13.0f,
            color
        );
    }
}


static void EmitirConfetiCapitan(MinijuegoCapitanManda& minijuego)
{
    const Color colores[6] =
    {
        { 255, 214, 84, 255 }, { 236, 116, 40, 255 }, { 46, 112, 214, 255 },
        { 236, 72, 112, 255 }, { 96, 206, 128, 255 }, { 250, 246, 236, 255 }
    };

    int cantidad = CantidadParticulasCapitan(110);

    for (int k = 0; k < cantidad; k++)
    {
        EmitirParticulaCapitan(
            minijuego,
            {
                AzarCapitan(minijuego, -5.0f, 5.0f),
                AzarCapitan(minijuego, 5.5f, 8.0f),
                AzarCapitan(minijuego, -2.0f, 4.0f)
            },
            {
                AzarCapitan(minijuego, -0.8f, 0.8f),
                AzarCapitan(minijuego, -0.4f, 1.2f),
                AzarCapitan(minijuego, -0.6f, 0.6f)
            },
            AzarCapitan(minijuego, 2.6f, 4.2f),
            AzarCapitan(minijuego, 0.07f, 0.12f),
            1.6f,
            colores[k % 6]
        );
    }
}


static void ActualizarVisualCapitan(MinijuegoCapitanManda& minijuego, float deltaTime)
{
    minijuego.tiempoAnimacion += deltaTime;

    for (int i = 0; i < MAX_PARTICULAS_CAPITAN; i++)
    {
        ParticulaCapitanManda& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        float freno = std::exp(-1.2f * deltaTime);
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        // Las gotas desaparecen al volver al agua.
        if (p.posicion.y < NIVEL_MAR_CAPITAN - 0.2f)
        {
            p.activa = false;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCapitanManda& jugador = minijuego.jugadores[i];

        float objetivo = 180.0f - 90.0f * (float)jugador.direccionRespuesta;

        if (minijuego.fase == FASE_CAPITAN_TERMINADO && !jugador.eliminado)
        {
            objetivo = 360.0f;      // el que queda en pie mira a camara
        }

        jugador.velocidadGiro += (objetivo - jugador.giroVisual) * 300.0f * deltaTime;
        jugador.velocidadGiro *= std::exp(-22.0f * deltaTime);
        jugador.giroVisual += jugador.velocidadGiro * deltaTime;

        jugador.destello -= deltaTime * 1.8f;
        if (jugador.destello < 0.0f) jugador.destello = 0.0f;

        jugador.sacudida -= deltaTime * 2.6f;
        if (jugador.sacudida < 0.0f) jugador.sacudida = 0.0f;

        if (jugador.tiempoCaida >= 0.0f)
        {
            jugador.tiempoCaida += deltaTime;

            if (!jugador.salpico && jugador.tiempoCaida >= DURACION_CAIDA_CAPITAN)
            {
                jugador.salpico = true;
                EmitirSalpicaduraCapitan(minijuego, POSICIONES_AGUA_CAPITAN[i]);
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
                ActivarTemblorCamaraGeneral(0.07f, 0.18f);
            }
        }
    }
}


//==================================================
// LOGICA
//==================================================

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
        minijuego.jugadores[i].direccionRespuesta = 0;
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
        minijuego.jugadores[i].direccionRespuesta = 0;
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

    if (!minijuego.confetiLanzado)
    {
        minijuego.confetiLanzado = true;
        EmitirConfetiCapitan(minijuego);
    }
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

            // Visual: sale despedido por la borda y cae al agua.
            jugador.tiempoCaida = 0.0f;
            jugador.salpico = false;
            jugador.sacudida = 1.0f;
        }
        else
        {
            jugador.rondasSuperadas++;
            jugador.tiempoFeedback = DURACION_RESOLUCION;
        }
    }

    if (eliminados > 0)
    {
        ActivarTemblorCamaraGeneral(0.10f, 0.22f);
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

    for (int i = 0; i < MAX_PARTICULAS_CAPITAN; i++)
    {
        particulas[i] = {};
    }

    fase = FASE_CAPITAN_PREPARACION;
    ordenActual = CONTROL_DIRECCION_IZQUIERDA;
    numeroRonda = 0;
    tiempoPreparacion = DURACION_PREPARACION_CAPITAN;
    tiempoFase = 0.0f;
    tiempoRespuesta = 1.55f;
    resultadoInicializado = false;

    tiempoAnimacion = 0.0f;
    semillaVisual = 2463534242u;
    confetiLanzado = false;
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

    // Animaciones y particulas siguen tambien con el minijuego terminado.
    ActualizarVisualCapitan(*this, deltaTime);

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
        int signoOrden =
            ordenActual == CONTROL_DIRECCION_IZQUIERDA ? -1 : 1;

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

                    // Visual: gira hacia el lado que eligio.
                    jugadores[i].direccionRespuesta =
                        jugadores[i].acerto ? signoOrden : -signoOrden;

                    if (jugadores[i].acerto)
                    {
                        jugadores[i].destello = 1.0f;
                        EmitirChispasCapitan(
                            *this,
                            { POSICIONES_JUGADOR_CAPITAN[i].x, 1.7f, POSICIONES_JUGADOR_CAPITAN[i].z }
                        );
                    }
                    else
                    {
                        jugadores[i].sacudida = 1.0f;
                    }
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

            // Visual: gira hacia el lado pulsado (las dos a la vez: no gira).
            jugadores[i].direccionRespuesta =
                izquierda && !derecha ? -1 : (derecha && !izquierda ? 1 : 0);

            if (jugadores[i].acerto)
            {
                jugadores[i].destello = 1.0f;
                EmitirChispasCapitan(
                    *this,
                    { POSICIONES_JUGADOR_CAPITAN[i].x, 1.7f, POSICIONES_JUGADOR_CAPITAN[i].z }
                );
            }
            else
            {
                jugadores[i].sacudida = 1.0f;
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


//==================================================
// PRIMITIVAS DE DIBUJO
//==================================================

static Color TonoCapitan(Color color, float factor)
{
    float r = (float)color.r * factor;
    float g = (float)color.g * factor;
    float b = (float)color.b * factor;

    return Color{
        (unsigned char)(r > 255.0f ? 255.0f : r),
        (unsigned char)(g > 255.0f ? 255.0f : g),
        (unsigned char)(b > 255.0f ? 255.0f : b),
        color.a
    };
}


static void VerticeCapitan(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadCapitan(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeCapitan(a, color);
    VerticeCapitan(b, color);
    VerticeCapitan(c, color);
    VerticeCapitan(d, color);
}


// Caja con caras sombreadas (luz falsa desde arriba a la izquierda).
static void CajaCapitan(Vector3 c, float ancho, float alto, float fondo, Color color)
{
    float x0 = c.x - ancho * 0.5f, x1 = c.x + ancho * 0.5f;
    float y0 = c.y - alto * 0.5f, y1 = c.y + alto * 0.5f;
    float z0 = c.z - fondo * 0.5f, z1 = c.z + fondo * 0.5f;

    rlBegin(RL_QUADS);
    QuadCapitan({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, TonoCapitan(color, 1.08f));
    QuadCapitan({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoCapitan(color, 0.84f));
    QuadCapitan({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoCapitan(color, 0.58f));
    QuadCapitan({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoCapitan(color, 0.66f));
    QuadCapitan({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoCapitan(color, 0.76f));
    QuadCapitan({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoCapitan(color, 0.45f));
    rlEnd();
}


// Toro horizontal (eje Y): rollos de cuerda y salvavidas a franjas.
static void ToroCapitan(
    Vector3 c,
    float radio,
    float grosor,
    Color colorA,
    Color colorB,
    int franjas
)
{
    const int SEG_U = 18;
    const int SEG_V = 8;

    rlBegin(RL_QUADS);

    for (int u = 0; u < SEG_U; u++)
    {
        float a0 = 6.2832f * (float)u / (float)SEG_U;
        float a1 = 6.2832f * (float)(u + 1) / (float)SEG_U;
        Color base = franjas > 0 && ((u * franjas * 2 / SEG_U) % 2 == 1) ? colorB : colorA;

        for (int v = 0; v < SEG_V; v++)
        {
            float b0 = 6.2832f * (float)v / (float)SEG_V;
            float b1 = 6.2832f * (float)(v + 1) / (float)SEG_V;
            Color color = TonoCapitan(base, 0.72f + 0.34f * std::sin((b0 + b1) * 0.5f));

            auto punto = [&](float a, float b) -> Vector3
            {
                float r = radio + grosor * std::cos(b);
                return { c.x + r * std::cos(a), c.y + grosor * std::sin(b), c.z + r * std::sin(a) };
            };

            QuadCapitan(punto(a0, b0), punto(a0, b1), punto(a1, b1), punto(a1, b0), color);
        }
    }

    rlEnd();
}


// Disco de luz con borde que se apaga (u y v definen el plano).
static void ResplandorCapitan(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 16;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloCapitan(float x, float y, float z, float radio, Color color)
{
    ResplandorCapitan({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void ResplandorFrenteCapitan(Vector3 centro, float radio, Color color)
{
    ResplandorCapitan(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
}


// Anillo plano en el suelo (r0..r1).
static void AnilloCapitan(float x, float y, float z, float r0, float r1, Color color)
{
    const int LADOS = 28;
    rlBegin(RL_QUADS);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;

        QuadCapitan(
            { x + std::cos(a0) * r1, y, z + std::sin(a0) * r1 },
            { x + std::cos(a0) * r0, y, z + std::sin(a0) * r0 },
            { x + std::cos(a1) * r0, y, z + std::sin(a1) * r0 },
            { x + std::cos(a1) * r1, y, z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


static float EaseOutBackCapitan(float t)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}


// Balanceo del barco (solo dibujo). Respeta "reducir movimiento".
static void AplicarBalanceoCapitan(float t)
{
    float factor = ObtenerFactorMovimientoCamaraMinijuegos();
    rlTranslatef(0.0f, std::sin(t * 0.9f) * 0.05f * factor, 0.0f);
    rlRotatef(std::sin(t * 0.62f) * 1.1f * factor, 0.0f, 0.0f, 1.0f);
    rlRotatef(std::sin(t * 0.47f + 1.0f) * 0.45f * factor, 1.0f, 0.0f, 0.0f);
}


//==================================================
// CIELO, MAR Y FONDO
//==================================================

static Color NieblaCapitan(Color color, float cantidad)
{
    return ColorLerp(color, COLOR_NIEBLA_CAPITAN, Limitar01Capitan(cantidad));
}


static void DibujarCieloCapitan(float t)
{
    // Degradado: bruma calida en el horizonte y azul arriba.
    const float z = -170.0f;
    const float ys[4] = { -30.0f, 4.0f, 22.0f, 120.0f };
    const Color cs[4] =
    {
        COLOR_NIEBLA_CAPITAN,
        COLOR_NIEBLA_CAPITAN,
        { 150, 182, 222, 255 },
        { 78, 124, 196, 255 }
    };

    rlBegin(RL_QUADS);
    for (int k = 0; k < 3; k++)
    {
        VerticeCapitan({ -320.0f, ys[k], z }, cs[k]);
        VerticeCapitan({ 320.0f, ys[k], z }, cs[k]);
        VerticeCapitan({ 320.0f, ys[k + 1], z }, cs[k + 1]);
        VerticeCapitan({ -320.0f, ys[k + 1], z }, cs[k + 1]);
    }
    rlEnd();

    // Sol bajo de la tarde.
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < 20; k++)
    {
        float a0 = 6.2832f * (float)k / 20.0f;
        float a1 = 6.2832f * (float)(k + 1) / 20.0f;
        VerticeCapitan({ 40.0f, 13.0f, z + 2.0f }, Color{ 255, 246, 214, 255 });
        VerticeCapitan({ 40.0f + std::cos(a0) * 3.6f, 13.0f + std::sin(a0) * 3.6f, z + 2.0f }, Color{ 255, 232, 180, 255 });
        VerticeCapitan({ 40.0f + std::cos(a1) * 3.6f, 13.0f + std::sin(a1) * 3.6f, z + 2.0f }, Color{ 255, 232, 180, 255 });
    }
    rlEnd();

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    // Nubes: grupos de esferas achatadas que derivan lento.
    int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 9 : 5;

    for (int n = 0; n < cantidad; n++)
    {
        float base = -150.0f + (float)n * 37.0f;
        float x = std::fmod(base + t * (0.5f + 0.1f * (float)(n % 3)) + 300.0f, 300.0f) - 150.0f;
        float y = 9.0f + 5.0f * HashCapitan(n, 3);
        float zn = -150.0f + 10.0f * HashCapitan(n, 7);
        Color nube = ColorLerp(Color{ 255, 240, 230, 255 }, Color{ 236, 214, 214, 255 }, HashCapitan(n, 11));

        for (int b = 0; b < 5; b++)
        {
            float dx = ((float)b - 2.0f) * 3.4f;
            float r = 3.0f + 2.6f * HashCapitan(n, b + 20) - std::fabs((float)b - 2.0f) * 0.6f;

            rlPushMatrix();
            rlTranslatef(x + dx, y + r * 0.25f, zn);
            rlScalef(1.0f, 0.48f, 0.6f);
            (DrawSphereEx)({ 0.0f, 0.0f, 0.0f }, r, 6, 10, TonoCapitan(nube, b % 2 == 0 ? 1.0f : 0.95f));
            rlPopMatrix();
        }
    }
}


static void DibujarFondoCapitan(float t)
{
    const float mar = NIVEL_MAR_CAPITAN;

    // Cordillera lejana, casi fundida en la bruma.
    for (int m = 0; m < 9; m++)
    {
        float x = -140.0f + (float)m * 35.0f + 10.0f * HashCapitan(m, 1);
        float alto = 12.0f + 10.0f * HashCapitan(m, 2);
        Color color = NieblaCapitan(Color{ 92, 116, 140, 255 }, 0.62f + 0.12f * HashCapitan(m, 3));
        (DrawCylinder)({ x, mar - 0.5f, -150.0f }, 0.0f, alto * 1.3f, alto, 7, color);
    }

    // Isla con faro a la izquierda.
    const float ix = -42.0f;
    const float iz = -88.0f;
    (DrawCylinder)({ ix, mar - 0.5f, iz }, 9.5f, 11.0f, 0.8f, 18, NieblaCapitan(Color{ 226, 200, 146, 255 }, 0.35f));
    (DrawCylinder)({ ix + 1.0f, mar + 0.2f, iz }, 2.6f, 7.8f, 5.0f, 12, NieblaCapitan(Color{ 74, 122, 78, 255 }, 0.38f));
    (DrawCylinder)({ ix - 3.5f, mar + 0.2f, iz + 1.0f }, 1.2f, 5.2f, 7.4f, 10, NieblaCapitan(Color{ 58, 102, 66, 255 }, 0.4f));

    // Faro: torre a franjas sobre la loma.
    for (int f = 0; f < 4; f++)
    {
        Color franja = f % 2 == 0 ? Color{ 238, 234, 226, 255 } : Color{ 196, 66, 52, 255 };
        (DrawCylinder)({ ix + 2.4f, mar + 5.0f + (float)f * 0.9f, iz }, 0.62f - 0.05f * (float)f, 0.66f - 0.05f * (float)f, 0.9f, 10, NieblaCapitan(franja, 0.3f));
    }
    (DrawCylinder)({ ix + 2.4f, mar + 8.6f, iz }, 0.0f, 0.62f, 0.7f, 10, NieblaCapitan(Color{ 60, 60, 66, 255 }, 0.3f));
    (DrawSphere)({ ix + 2.4f, mar + 8.4f, iz }, 0.36f, Color{ 255, 236, 170, 255 });

    // Palmeras en la playa.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int p = 0; p < 4; p++)
        {
            float px = ix - 7.0f + (float)p * 4.2f;
            float pz = iz + 6.0f - (float)(p % 2) * 2.0f;
            float inclina = (p % 2 == 0 ? 1.0f : -1.0f) * 0.8f;
            Vector3 pie = { px, mar + 0.2f, pz };
            Vector3 copa = { px + inclina, mar + 3.6f, pz };
            (DrawCylinderEx)(pie, copa, 0.16f, 0.11f, 6, NieblaCapitan(Color{ 120, 88, 60, 255 }, 0.35f));

            for (int h = 0; h < 5; h++)
            {
                rlPushMatrix();
                rlTranslatef(copa.x, copa.y, copa.z);
                rlRotatef((float)h * 72.0f + std::sin(t * 1.3f + (float)p) * 6.0f, 0.0f, 1.0f, 0.0f);
                rlRotatef(-22.0f, 0.0f, 0.0f, 1.0f);
                (DrawCube)({ 0.9f, 0.0f, 0.0f }, 1.8f, 0.08f, 0.45f, NieblaCapitan(Color{ 64, 132, 72, 255 }, 0.35f));
                rlPopMatrix();
            }
        }
    }

    // Puerto a la derecha: muelle de piedra, casas y torre.
    const float px0 = 46.0f;
    const float pz0 = -94.0f;
    (DrawCylinder)({ px0 + 6.0f, mar - 0.5f, pz0 - 6.0f }, 6.0f, 18.0f, 9.0f, 9, NieblaCapitan(Color{ 82, 120, 84, 255 }, 0.48f));
    (DrawCube)({ px0, mar + 0.4f, pz0 + 2.0f }, 26.0f, 1.6f, 4.0f, NieblaCapitan(Color{ 150, 140, 126, 255 }, 0.35f));

    const Color casas[5] =
    {
        { 214, 122, 96, 255 }, { 232, 204, 134, 255 }, { 150, 182, 204, 255 },
        { 240, 232, 214, 255 }, { 196, 150, 120, 255 }
    };

    for (int c = 0; c < 8; c++)
    {
        float cx = px0 - 11.0f + (float)c * 3.1f;
        float ancho = 2.2f + 0.6f * HashCapitan(c, 40);
        float alto = 1.8f + 1.6f * HashCapitan(c, 41);
        float cz = pz0 + 0.2f - 1.4f * HashCapitan(c, 42);
        Color pared = NieblaCapitan(casas[c % 5], 0.34f);
        CajaCapitan({ cx, mar + 1.2f + alto * 0.5f, cz }, ancho, alto, 2.0f, pared);

        rlPushMatrix();
        rlTranslatef(cx, mar + 1.2f + alto, cz);
        rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        (DrawCylinder)({ 0.0f, 0.0f, 0.0f }, 0.0f, ancho * 0.78f, 1.0f, 4, NieblaCapitan(Color{ 150, 70, 54, 255 }, 0.34f));
        rlPopMatrix();

        // Ventanas encendidas.
        (DrawCube)({ cx, mar + 1.2f + alto * 0.55f, cz + 1.01f }, 0.4f, 0.4f, 0.02f, Color{ 255, 214, 140, 255 });
    }

    // Torre de la iglesia.
    CajaCapitan({ px0 + 8.0f, mar + 4.2f, pz0 - 1.5f }, 1.6f, 6.0f, 1.6f, NieblaCapitan(Color{ 226, 218, 200, 255 }, 0.34f));
    (DrawCylinder)({ px0 + 8.0f, mar + 7.2f, pz0 - 1.5f }, 0.0f, 1.2f, 2.2f, 4, NieblaCapitan(Color{ 92, 110, 132, 255 }, 0.34f));

    // Mastiles de botes amarrados.
    for (int b = 0; b < 4; b++)
    {
        float bx = px0 - 10.0f + (float)b * 5.5f;
        (DrawCube)({ bx, mar + 0.1f, pz0 + 5.2f }, 2.6f, 0.6f, 1.0f, NieblaCapitan(Color{ 120, 70, 50, 255 }, 0.35f));
        (DrawCylinder)({ bx, mar + 0.3f, pz0 + 5.2f }, 0.05f, 0.07f, 4.0f + (float)(b % 2), 5, NieblaCapitan(Color{ 90, 70, 54, 255 }, 0.35f));
    }

    // Velero lejano que cruza el horizonte.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        float x = std::fmod(t * 0.9f, 160.0f) - 80.0f;
        float cabeceo = std::sin(t * 1.1f) * 0.1f;
        Vector3 casco = { x, mar + 0.3f + cabeceo, -64.0f };
        (DrawCube)(casco, 4.4f, 0.9f, 1.4f, NieblaCapitan(Color{ 92, 58, 42, 255 }, 0.45f));
        (DrawCylinder)({ x, casco.y + 0.4f, casco.z }, 0.06f, 0.08f, 4.8f, 5, NieblaCapitan(Color{ 80, 60, 44, 255 }, 0.45f));
        (DrawCube)({ x + 0.1f, casco.y + 2.9f, casco.z }, 2.6f, 3.0f, 0.06f, NieblaCapitan(Color{ 244, 236, 214, 255 }, 0.4f));
    }
}


// Mar abierto: malla con olas cruzadas; las crestas se aclaran y lejos se
// funde con la bruma del horizonte.
static void DibujarMarCapitan(float t)
{
    const float paso = CalidadDecoracion() == CALIDAD_ALTA ? 3.0f : 5.0f;
    const float pasoLejos = 10.0f;

    rlBegin(RL_QUADS);

    for (int tramo = 0; tramo < 2; tramo++)
    {
        float zIni = tramo == 0 ? -44.0f : -170.0f;
        float zFin = tramo == 0 ? 20.0f : -42.0f;
        float xMax = tramo == 0 ? 70.0f : 320.0f;
        float p = tramo == 0 ? paso : pasoLejos;

        for (float z = zIni; z < zFin; z += p)
        {
            for (float x = -xMax; x < xMax; x += p)
            {
                const float xs[4] = { x, x, x + p, x + p };
                const float zs[4] = { z, z + p, z + p, z };

                for (int k = 0; k < 4; k++)
                {
                    float ola =
                        std::sin(xs[k] * 0.38f + t * 1.2f) +
                        std::sin(zs[k] * 0.52f - t * 1.5f + xs[k] * 0.11f);
                    float brillo = Limitar01Capitan(0.5f + 0.28f * ola);
                    float lejos = Limitar01Capitan((-zs[k] - 12.0f) / 120.0f);
                    Color agua = ColorLerp(Color{ 22, 82, 124, 255 }, Color{ 66, 146, 178, 255 }, brillo);
                    agua = ColorLerp(agua, COLOR_NIEBLA_CAPITAN, lejos * 0.8f);

                    rlColor4ub(agua.r, agua.g, agua.b, 255);
                    rlVertex3f(xs[k], NIVEL_MAR_CAPITAN + 0.09f * ola * (1.0f - lejos), zs[k]);
                }
            }
        }
    }

    rlEnd();
}


// Espuma en las crestas y bruma baja sobre el horizonte (translucidas).
static void DibujarEspumaYNieblaCapitan(float t)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 70 : 36;
        rlBegin(RL_QUADS);

        for (int k = 0; k < cantidad; k++)
        {
            float x = -40.0f + 80.0f * HashCapitan(k, 90);
            if (std::fabs(x) < 6.5f) x += x < 0.0f ? -7.0f : 7.0f;
            float z = std::fmod(-40.0f + 52.0f * HashCapitan(k, 91) + t * 0.6f + 40.0f, 52.0f) - 40.0f;
            float fase = t * 0.9f + 6.2832f * HashCapitan(k, 92);
            float alfa = Limitar01Capitan(std::sin(fase)) * 0.55f;
            float largo = 0.6f + 1.2f * HashCapitan(k, 93);
            float y = NIVEL_MAR_CAPITAN + 0.13f;

            QuadCapitan(
                { x - largo, y, z - 0.12f },
                { x - largo, y, z + 0.12f },
                { x + largo, y, z + 0.12f },
                { x + largo, y, z - 0.12f },
                Color{ 240, 248, 250, (unsigned char)(alfa * 255.0f) }
            );
        }

        rlEnd();
    }

    // Bruma: velos verticales que tapan la base de la costa lejana.
    const float zs[2] = { -60.0f, -110.0f };

    rlBegin(RL_QUADS);
    for (int v = 0; v < 2; v++)
    {
        Color abajo = COLOR_NIEBLA_CAPITAN;
        abajo.a = 150;
        Color arriba = COLOR_NIEBLA_CAPITAN;
        arriba.a = 0;

        VerticeCapitan({ -320.0f, NIVEL_MAR_CAPITAN - 0.5f, zs[v] }, abajo);
        VerticeCapitan({ 320.0f, NIVEL_MAR_CAPITAN - 0.5f, zs[v] }, abajo);
        VerticeCapitan({ 320.0f, NIVEL_MAR_CAPITAN + 6.0f + 4.0f * (float)v, zs[v] }, arriba);
        VerticeCapitan({ -320.0f, NIVEL_MAR_CAPITAN + 6.0f + 4.0f * (float)v, zs[v] }, arriba);
    }
    rlEnd();

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Gaviotas en circulo a ambos lados del barco (solo ambiente; su vuelo
// depende del reloj, nunca de la orden).
static void DibujarGaviotasCapitan(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    const float centrosX[6] = { -13.0f, -18.0f, 14.0f, 19.0f, -24.0f, 25.0f };

    for (int g = 0; g < 6; g++)
    {
        float fase = (float)g * 1.37f;
        float radio = 2.6f + 0.8f * (float)(g % 3);
        float a = t * (0.45f + 0.06f * (float)g) + fase;
        Vector3 c =
        {
            centrosX[g] + std::cos(a) * radio,
            4.6f + 0.6f * (float)(g % 2) + 0.4f * std::sin(t * 0.7f + fase),
            -9.0f - 3.0f * (float)(g % 3) + std::sin(a) * radio * 0.7f
        };
        float aleteo = std::sin(t * 9.0f + fase * 3.0f) * 0.35f;
        float dx = -std::sin(a);
        float dz = std::cos(a) * 0.7f;
        float largo = std::sqrt(dx * dx + dz * dz);
        dx /= largo;
        dz /= largo;
        Vector3 lado = { -dz, 0.0f, dx };
        Color pluma = Color{ 238, 238, 242, 255 };
        Color punta = Color{ 70, 72, 80, 255 };

        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 extremo =
            {
                c.x + lado.x * 0.6f * (float)s,
                c.y + 0.18f + aleteo,
                c.z + lado.z * 0.6f * (float)s
            };
            Vector3 frente = { c.x + dx * 0.18f, c.y, c.z + dz * 0.18f };
            Vector3 atras = { c.x - dx * 0.18f, c.y, c.z - dz * 0.18f };
            DrawTriangle3D(frente, extremo, atras, pluma);
            DrawTriangle3D(frente, atras, extremo, pluma);
            (DrawSphere)(extremo, 0.05f, punta);
        }
    }
}


//==================================================
// BARCO
//==================================================

// Tablones de madera a lo largo del barco, con juntas, alturas y tonos
// variados sobre una base oscura que hace de rendija.
static void DibujarTablonesCapitan(float x0, float x1, float z0, float z1, float yTope, int semilla)
{
    CajaCapitan(
        { (x0 + x1) * 0.5f, yTope - 0.07f, (z0 + z1) * 0.5f },
        x1 - x0, 0.1f, z1 - z0,
        Color{ 54, 36, 26, 255 }
    );

    const float anchoTablon = 0.44f;
    int columnas = (int)((x1 - x0) / anchoTablon);
    bool clavos = CalidadDecoracion() == CALIDAD_ALTA;

    for (int c = 0; c < columnas; c++)
    {
        float cx = x0 + anchoTablon * ((float)c + 0.5f);
        float z = z0 - 1.6f * HashCapitan(c, semilla);
        int segmento = 0;

        while (z < z1)
        {
            float largo = 2.0f + 1.8f * HashCapitan(c * 7 + segmento, semilla + 1);
            float a = z < z0 ? z0 : z;
            float b = z + largo > z1 ? z1 : z + largo;

            if (b - a > 0.1f)
            {
                float h = HashCapitan(c * 13 + segmento, semilla + 2);
                Color madera = ColorLerp(Color{ 140, 96, 60, 255 }, Color{ 182, 132, 82, 255 }, h);

                if (HashCapitan(c * 5 + segmento, semilla + 3) > 0.86f)
                {
                    madera = Color{ 118, 82, 54, 255 };     // tablon gastado
                }

                CajaCapitan(
                    { cx, yTope - 0.04f + 0.012f * h, (a + b) * 0.5f },
                    anchoTablon - 0.045f, 0.08f, b - a - 0.035f,
                    madera
                );

                if (clavos)
                {
                    for (int s = -1; s <= 1; s += 2)
                    {
                        (DrawCube)({ cx - 0.1f, yTope + 0.002f + 0.012f * h, (s < 0 ? a : b) - 0.12f * (float)s }, 0.035f, 0.01f, 0.035f, Color{ 52, 44, 40, 255 });
                        (DrawCube)({ cx + 0.1f, yTope + 0.002f + 0.012f * h, (s < 0 ? a : b) - 0.12f * (float)s }, 0.035f, 0.01f, 0.035f, Color{ 52, 44, 40, 255 });
                    }
                }
            }

            z += largo;
            segmento++;
        }
    }
}


// Amurada lateral (lado -1 / 1): tracas horizontales, candeleros y regala.
static void DibujarAmuradaCapitan(float lado)
{
    const Color madera = { 122, 80, 50, 255 };
    const Color regala = { 156, 108, 68, 255 };
    const Color candelero = { 90, 60, 40, 255 };
    float x = lado * X_AMURADA_CAPITAN;

    // Tramo de la cubierta principal y tramo del alcazar.
    struct Tramo { float z0, z1, y0, y1; };
    const Tramo tramos[2] =
    {
        { Z_FRENTE_ALCAZAR_CAPITAN, Z_PROA_CAPITAN, 0.0f, 0.95f },
        { Z_POPA_CAPITAN, Z_FRENTE_ALCAZAR_CAPITAN, ALTURA_ALCAZAR_CAPITAN, ALTURA_ALCAZAR_CAPITAN + 0.9f }
    };

    for (int t = 0; t < 2; t++)
    {
        const Tramo& tr = tramos[t];
        float zc = (tr.z0 + tr.z1) * 0.5f;
        float largo = tr.z1 - tr.z0;
        int tracas = 4;
        float altoTraca = (tr.y1 - tr.y0) / (float)tracas;

        for (int k = 0; k < tracas; k++)
        {
            Color c = ColorLerp(madera, Color{ 140, 94, 60, 255 }, HashCapitan(k + t * 10, (int)(lado * 3.0f) + 50));
            CajaCapitan({ x + lado * 0.08f, tr.y0 + altoTraca * ((float)k + 0.5f), zc }, 0.16f, altoTraca - 0.02f, largo, c);
        }

        CajaCapitan({ x + lado * 0.06f, tr.y1 + 0.05f, zc }, 0.34f, 0.1f, largo + 0.1f, regala);

        for (float z = tr.z0 + 0.3f; z < tr.z1; z += 1.35f)
        {
            CajaCapitan({ x - lado * 0.04f, (tr.y0 + tr.y1) * 0.5f, z }, 0.12f, tr.y1 - tr.y0, 0.12f, candelero);
        }
    }

    // Pilar que une ambos tramos en el frente del alcazar.
    CajaCapitan({ x - lado * 0.02f, (ALTURA_ALCAZAR_CAPITAN + 0.9f) * 0.5f, Z_FRENTE_ALCAZAR_CAPITAN }, 0.24f, ALTURA_ALCAZAR_CAPITAN + 0.9f, 0.24f, candelero);

    // Portas de los canones (huecos oscuros en la cara interior).
    (DrawCube)({ x - lado * 0.005f, 0.5f, 3.9f }, 0.02f, 0.42f, 0.5f, Color{ 30, 20, 16, 255 });
}


// Barandilla con balaustres a lo largo de X.
static void DibujarBarandaXCapitan(float x0, float x1, float y, float z, float alto)
{
    const Color balaustre = { 168, 120, 76, 255 };
    CajaCapitan({ (x0 + x1) * 0.5f, y + alto + 0.04f, z }, x1 - x0, 0.09f, 0.2f, Color{ 150, 104, 64, 255 });
    CajaCapitan({ (x0 + x1) * 0.5f, y + 0.05f, z }, x1 - x0, 0.08f, 0.16f, Color{ 120, 82, 52, 255 });

    for (float x = x0 + 0.15f; x < x1 - 0.05f; x += 0.34f)
    {
        CajaCapitan({ x, y + alto * 0.5f + 0.05f, z }, 0.08f, alto - 0.1f, 0.08f, balaustre);
    }
}


static void DibujarCanonCapitan(float lado, float z)
{
    float x = lado * 4.55f;
    const Color curena = { 108, 72, 46, 255 };
    const Color hierro = { 46, 48, 54, 255 };

    DibujarSombraRetroRectangular({ x, 0.0f, z }, 0.9f, 0.8f);
    CajaCapitan({ x, 0.24f, z }, 0.8f, 0.24f, 0.6f, curena);
    CajaCapitan({ x - lado * 0.12f, 0.42f, z }, 0.5f, 0.16f, 0.52f, TonoCapitan(curena, 0.9f));

    for (int r = 0; r < 4; r++)
    {
        float rx = x + (r < 2 ? -0.26f : 0.26f);
        float rz = z + (r % 2 == 0 ? -0.33f : 0.27f);
        (DrawCylinderEx)({ rx, 0.15f, rz }, { rx, 0.15f, rz + 0.06f }, 0.15f, 0.15f, 10, Color{ 76, 52, 36, 255 });
    }

    Vector3 atras = { x - lado * 0.5f, 0.5f, z };
    Vector3 boca = { x + lado * 0.75f, 0.54f, z };
    (DrawCylinderEx)(atras, boca, 0.17f, 0.12f, 12, hierro);
    (DrawCylinderEx)({ boca.x - lado * 0.08f, boca.y, boca.z }, { boca.x + lado * 0.02f, boca.y, boca.z }, 0.15f, 0.15f, 12, TonoCapitan(hierro, 1.3f));
    (DrawSphere)({ atras.x - lado * 0.06f, atras.y, atras.z }, 0.1f, hierro);

    // Balas apiladas.
    for (int b = 0; b < 4; b++)
    {
        float bx = x - lado * 0.7f + (b == 3 ? 0.0f : ((float)b - 1.0f) * 0.2f);
        float by = b == 3 ? 0.26f : 0.1f;
        float bz = z + 0.55f + (b == 3 ? 0.06f : 0.0f);
        (DrawSphere)({ bx, by, bz }, 0.1f, TonoCapitan(hierro, 1.1f));
    }
}


static void DibujarBarrilCapitan(Vector3 base, float alto, float radio)
{
    const Color duela = { 140, 96, 56, 255 };
    const Color aro = { 64, 58, 54, 255 };

    DibujarSombraRetroCircular(base, radio * 1.05f, radio * 0.9f, base.y + 0.018f);
    (DrawCylinder)(base, radio * 0.88f, radio * 0.88f, alto * 0.12f, 12, duela);
    (DrawCylinder)({ base.x, base.y + alto * 0.12f, base.z }, radio, radio * 0.88f, alto * 0.38f, 12, TonoCapitan(duela, 0.95f));
    (DrawCylinder)({ base.x, base.y + alto * 0.5f, base.z }, radio * 0.88f, radio, alto * 0.38f, 12, TonoCapitan(duela, 0.95f));
    (DrawCylinder)({ base.x, base.y + alto * 0.88f, base.z }, radio * 0.88f, radio * 0.88f, alto * 0.12f, 12, duela);
    (DrawCylinder)({ base.x, base.y + alto - 0.005f, base.z }, radio * 0.8f, radio * 0.8f, 0.01f, 12, TonoCapitan(duela, 1.2f));

    const float aros[3] = { 0.14f, 0.5f, 0.86f };
    for (int k = 0; k < 3; k++)
    {
        float r = k == 1 ? radio + 0.012f : radio * 0.9f + 0.012f;
        (DrawCylinder)({ base.x, base.y + alto * aros[k] - 0.03f, base.z }, r, r, 0.06f, 12, aro);
    }
}


static void DibujarRolloCuerdaCapitan(Vector3 base)
{
    const Color cuerda = { 196, 162, 108, 255 };
    DibujarSombraRetroCircular(base, 0.55f, 0.5f, base.y + 0.018f);

    for (int k = 0; k < 4; k++)
    {
        ToroCapitan(
            { base.x, base.y + 0.06f + (float)k * 0.1f, base.z },
            0.42f - 0.035f * (float)k,
            0.06f,
            TonoCapitan(cuerda, 1.0f - 0.05f * (float)(k % 2)),
            cuerda,
            0
        );
    }
}


static void DibujarFarolCapitan(Vector3 base)
{
    const Color hierro = { 44, 42, 46, 255 };
    CajaCapitan({ base.x, base.y + 0.45f, base.z }, 0.1f, 0.9f, 0.1f, Color{ 96, 64, 42, 255 });
    CajaCapitan({ base.x, base.y + 0.94f, base.z }, 0.3f, 0.06f, 0.3f, hierro);
    (DrawCube)({ base.x, base.y + 1.12f, base.z }, 0.22f, 0.3f, 0.22f, Color{ 255, 218, 136, 255 });

    for (int e = 0; e < 4; e++)
    {
        float sx = e < 2 ? -0.12f : 0.12f;
        float sz = e % 2 == 0 ? -0.12f : 0.12f;
        (DrawCube)({ base.x + sx, base.y + 1.12f, base.z + sz }, 0.04f, 0.32f, 0.04f, hierro);
    }

    (DrawCylinder)({ base.x, base.y + 1.28f, base.z }, 0.0f, 0.22f, 0.18f, 4, hierro);
    (DrawSphere)({ base.x, base.y + 1.5f, base.z }, 0.04f, hierro);
}


static void DibujarTimonCapitan(Vector3 base)
{
    const Color madera = { 132, 88, 52, 255 };
    const Color laton = { 214, 170, 70, 255 };
    Vector3 eje = { base.x, base.y + 1.05f, base.z };

    DibujarSombraRetroRectangular(base, 0.5f, 0.5f, 0.0f, base.y + 0.018f);
    CajaCapitan({ base.x, base.y + 0.5f, base.z - 0.12f }, 0.3f, 1.0f, 0.3f, TonoCapitan(madera, 0.85f));

    // Rueda en el plano XY (mira a camara), quieta: nunca gira hacia un lado.
    const int LADOS = 16;
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        (DrawCylinderEx)(
            { eje.x + std::cos(a0) * 0.5f, eje.y + std::sin(a0) * 0.5f, eje.z + 0.06f },
            { eje.x + std::cos(a1) * 0.5f, eje.y + std::sin(a1) * 0.5f, eje.z + 0.06f },
            0.045f, 0.045f, 5, madera
        );
    }

    for (int r = 0; r < 8; r++)
    {
        float a = 6.2832f * (float)r / 8.0f;
        Vector3 fuera = { eje.x + std::cos(a) * 0.72f, eje.y + std::sin(a) * 0.72f, eje.z + 0.06f };
        (DrawCylinderEx)({ eje.x, eje.y, eje.z + 0.06f }, fuera, 0.03f, 0.04f, 5, madera);
        (DrawSphere)(fuera, 0.05f, TonoCapitan(madera, 1.15f));
    }

    (DrawCylinderEx)({ eje.x, eje.y, eje.z - 0.1f }, { eje.x, eje.y, eje.z + 0.14f }, 0.1f, 0.1f, 10, laton);
}


static void DibujarCofreCapitan(Vector3 base)
{
    const Color madera = { 112, 66, 44, 255 };
    const Color laton = { 214, 170, 70, 255 };
    DibujarSombraRetroRectangular(base, 0.95f, 0.65f, 0.0f, base.y + 0.018f);
    CajaCapitan({ base.x, base.y + 0.26f, base.z }, 0.9f, 0.52f, 0.58f, madera);
    CajaCapitan({ base.x, base.y + 0.6f, base.z }, 0.92f, 0.16f, 0.6f, TonoCapitan(madera, 1.12f));

    for (int s = -1; s <= 1; s += 2)
    {
        CajaCapitan({ base.x + (float)s * 0.3f, base.y + 0.36f, base.z }, 0.07f, 0.72f, 0.62f, laton);
    }

    CajaCapitan({ base.x, base.y + 0.44f, base.z + 0.3f }, 0.14f, 0.16f, 0.04f, laton);
}


// Vela mayor: malla hinchada por el viento, con costuras.
static void DibujarVelaCapitan(float t, float yArriba, float yAbajo, float anchoArriba, float anchoAbajo, float z)
{
    const int NU = 10;
    const int NV = 6;
    const Color lona = { 242, 230, 204, 255 };

    auto punto = [&](int u, int v) -> Vector3
    {
        float fu = (float)u / (float)NU;
        float fv = (float)v / (float)NV;
        float ancho = anchoArriba + (anchoAbajo - anchoArriba) * fv;
        float panza = std::sin(fu * PI) * std::sin(fv * PI * 0.85f + 0.25f);
        float flameo = std::sin(t * 2.2f + fu * 3.0f + fv * 2.0f) * 0.05f * fv;
        return {
            -ancho * 0.5f + ancho * fu,
            yArriba + (yAbajo - yArriba) * fv,
            z + 0.75f * panza + flameo
        };
    };

    rlBegin(RL_QUADS);

    for (int u = 0; u < NU; u++)
    {
        for (int v = 0; v < NV; v++)
        {
            float fu = ((float)u + 0.5f) / (float)NU;
            float fv = ((float)v + 0.5f) / (float)NV;
            float luz = 0.74f + 0.26f * std::sin(fu * PI) - 0.06f * fv + (u % 2 == 0 ? 0.0f : -0.03f);
            Color c = TonoCapitan(lona, luz);
            Vector3 a = punto(u, v), b = punto(u + 1, v), cc = punto(u + 1, v + 1), d = punto(u, v + 1);

            // Las dos caras.
            QuadCapitan(a, d, cc, b, c);
            QuadCapitan(a, b, cc, d, TonoCapitan(c, 0.8f));
        }
    }

    rlEnd();

    // Costuras verticales y relinga inferior.
    for (int u = 1; u < NU; u += 2)
    {
        for (int v = 0; v < NV; v++)
        {
            Vector3 a = punto(u, v), b = punto(u, v + 1);
            a.z += 0.02f;
            b.z += 0.02f;
            DrawLine3D(a, b, Color{ 196, 180, 150, 255 });
        }
    }
}


static void DibujarAparejoCapitan()
{
    const Color cabo = { 66, 50, 36, 255 };
    const float zMastil = Z_POPA_CAPITAN + 0.9f;

    // Obenques hacia las amuradas del alcazar, con flechastes.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        Vector3 arriba = { s * 0.2f, 8.4f, zMastil };
        const float zsAnclaje[3] = { Z_POPA_CAPITAN + 0.3f, Z_POPA_CAPITAN + 1.2f, Z_POPA_CAPITAN + 2.1f };
        Vector3 abajo[3];

        for (int k = 0; k < 3; k++)
        {
            abajo[k] = { s * (X_AMURADA_CAPITAN + 0.1f), ALTURA_ALCAZAR_CAPITAN + 0.95f, zsAnclaje[k] };
            DrawLine3D(arriba, abajo[k], cabo);
        }

        for (int r = 1; r < 9; r++)
        {
            float f = (float)r / 9.0f;
            Vector3 a = { arriba.x + (abajo[0].x - arriba.x) * f, arriba.y + (abajo[0].y - arriba.y) * f, arriba.z + (abajo[0].z - arriba.z) * f };
            Vector3 b = { arriba.x + (abajo[2].x - arriba.x) * f, arriba.y + (abajo[2].y - arriba.y) * f, arriba.z + (abajo[2].z - arriba.z) * f };
            DrawLine3D(a, b, Fade(cabo, 0.8f));
        }

        // Escotas de la vela y brazas de la verga.
        DrawLine3D({ s * 3.3f, 3.35f, zMastil + 0.3f }, { s * 4.6f, ALTURA_ALCAZAR_CAPITAN + 0.95f, Z_FRENTE_ALCAZAR_CAPITAN - 0.3f }, cabo);
        DrawLine3D({ s * 3.7f, 7.0f, zMastil }, { s * (X_AMURADA_CAPITAN + 0.1f), ALTURA_ALCAZAR_CAPITAN + 0.95f, Z_POPA_CAPITAN + 0.05f }, cabo);
    }
}


static void DibujarMastilCapitan(float t)
{
    const Color madera = { 128, 88, 54, 255 };
    const Color hierro = { 58, 56, 60, 255 };
    const float zMastil = Z_POPA_CAPITAN + 0.9f;

    (DrawCylinder)({ 0.0f, ALTURA_ALCAZAR_CAPITAN, zMastil }, 0.17f, 0.26f, 12.0f, 12, madera);

    for (int k = 0; k < 4; k++)
    {
        float y = ALTURA_ALCAZAR_CAPITAN + 0.6f + (float)k * 1.5f;
        float r = 0.255f - 0.0075f * (y - ALTURA_ALCAZAR_CAPITAN);
        (DrawCylinder)({ 0.0f, y, zMastil }, r + 0.02f, r + 0.02f, 0.1f, 12, hierro);
    }

    // Vergas.
    (DrawCylinderEx)({ -3.8f, 7.05f, zMastil + 0.12f }, { 3.8f, 7.05f, zMastil + 0.12f }, 0.09f, 0.09f, 8, madera);
    (DrawCylinderEx)({ -3.4f, 3.35f, zMastil + 0.12f }, { 3.4f, 3.35f, zMastil + 0.12f }, 0.07f, 0.07f, 8, TonoCapitan(madera, 0.9f));

    // Cofa.
    (DrawCylinder)({ 0.0f, 8.3f, zMastil }, 0.9f, 0.7f, 0.25f, 10, TonoCapitan(madera, 0.9f));

    DibujarVelaCapitan(t, 6.95f, 3.45f, 7.0f, 6.4f, zMastil + 0.18f);
}


static void DibujarCubiertaCapitan(float t)
{
    // Casco exterior (asoma en la proa y entre las olas).
    CajaCapitan({ 0.0f, -0.75f, 2.0f }, X_AMURADA_CAPITAN * 2.0f + 0.5f, 1.4f, Z_PROA_CAPITAN - Z_POPA_CAPITAN + 0.4f, Color{ 84, 52, 36, 255 });
    CajaCapitan({ 0.0f, -1.2f, 2.0f }, X_AMURADA_CAPITAN * 2.0f + 0.56f, 0.18f, Z_PROA_CAPITAN - Z_POPA_CAPITAN + 0.45f, Color{ 36, 30, 28, 255 });

    // Cubierta principal y alcazar.
    DibujarTablonesCapitan(-X_AMURADA_CAPITAN, X_AMURADA_CAPITAN, Z_FRENTE_ALCAZAR_CAPITAN, Z_PROA_CAPITAN, 0.0f, 11);
    DibujarTablonesCapitan(-X_AMURADA_CAPITAN, X_AMURADA_CAPITAN, Z_POPA_CAPITAN, Z_FRENTE_ALCAZAR_CAPITAN, ALTURA_ALCAZAR_CAPITAN, 29);

    // Frente del alcazar: mamparo de tablas verticales con puerta de camara.
    CajaCapitan({ 0.0f, ALTURA_ALCAZAR_CAPITAN * 0.5f, Z_FRENTE_ALCAZAR_CAPITAN - 0.1f }, X_AMURADA_CAPITAN * 2.0f, ALTURA_ALCAZAR_CAPITAN, 0.2f, Color{ 112, 74, 48, 255 });

    for (float x = -X_AMURADA_CAPITAN + 0.35f; x < X_AMURADA_CAPITAN; x += 0.35f)
    {
        DrawLine3D({ x, 0.02f, Z_FRENTE_ALCAZAR_CAPITAN + 0.005f }, { x, ALTURA_ALCAZAR_CAPITAN - 0.05f, Z_FRENTE_ALCAZAR_CAPITAN + 0.005f }, Color{ 70, 46, 32, 255 });
    }

    CajaCapitan({ 0.0f, ALTURA_ALCAZAR_CAPITAN - 0.05f, Z_FRENTE_ALCAZAR_CAPITAN + 0.02f }, X_AMURADA_CAPITAN * 2.0f, 0.1f, 0.08f, Color{ 150, 104, 64, 255 });
    CajaCapitan({ 0.0f, 0.42f, Z_FRENTE_ALCAZAR_CAPITAN + 0.03f }, 0.95f, 0.84f, 0.06f, Color{ 76, 48, 32, 255 });
    CajaCapitan({ 0.0f, 0.42f, Z_FRENTE_ALCAZAR_CAPITAN + 0.06f }, 0.75f, 0.74f, 0.04f, Color{ 104, 66, 42, 255 });
    (DrawSphere)({ 0.26f, 0.42f, Z_FRENTE_ALCAZAR_CAPITAN + 0.1f }, 0.04f, Color{ 230, 186, 80, 255 });

    // Escaleras a ambos lados.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 4.2f;

        for (int e = 0; e < 5; e++)
        {
            float y = ALTURA_ALCAZAR_CAPITAN - ALTURA_ALCAZAR_CAPITAN / 6.0f * (float)(e + 1);
            float z = Z_FRENTE_ALCAZAR_CAPITAN + 0.14f + 0.28f * (float)e;
            CajaCapitan({ x, y + 0.03f, z }, 0.95f, 0.07f, 0.3f, Color{ 160, 112, 70, 255 });
            CajaCapitan({ x, y * 0.5f, z }, 0.9f, y, 0.26f, Color{ 86, 58, 38, 255 });
        }

        DrawLine3D({ x - (float)lado * 0.48f, ALTURA_ALCAZAR_CAPITAN + 0.55f, Z_FRENTE_ALCAZAR_CAPITAN }, { x - (float)lado * 0.48f, 0.65f, Z_FRENTE_ALCAZAR_CAPITAN + 1.1f }, Color{ 66, 50, 36, 255 });
    }

    // Barandillas del alcazar (sin cortar las escaleras) y de popa.
    DibujarBarandaXCapitan(-3.6f, 3.6f, ALTURA_ALCAZAR_CAPITAN, Z_FRENTE_ALCAZAR_CAPITAN + 0.02f, 0.42f);
    DibujarBarandaXCapitan(-X_AMURADA_CAPITAN, -4.75f, ALTURA_ALCAZAR_CAPITAN, Z_FRENTE_ALCAZAR_CAPITAN + 0.02f, 0.42f);
    DibujarBarandaXCapitan(4.75f, X_AMURADA_CAPITAN, ALTURA_ALCAZAR_CAPITAN, Z_FRENTE_ALCAZAR_CAPITAN + 0.02f, 0.42f);
    DibujarBarandaXCapitan(-X_AMURADA_CAPITAN, X_AMURADA_CAPITAN, ALTURA_ALCAZAR_CAPITAN, Z_POPA_CAPITAN + 0.1f, 0.85f);

    DibujarAmuradaCapitan(-1.0f);
    DibujarAmuradaCapitan(1.0f);

    // Escotilla con enjaretado entre los jugadores y el alcazar.
    CajaCapitan({ 0.0f, 0.07f, 0.55f }, 2.0f, 0.14f, 1.3f, Color{ 120, 82, 52, 255 });
    CajaCapitan({ 0.0f, 0.1f, 0.55f }, 1.7f, 0.12f, 1.0f, Color{ 34, 24, 20, 255 });

    for (int k = 0; k < 6; k++)
    {
        CajaCapitan({ -0.7f + (float)k * 0.28f, 0.15f, 0.55f }, 0.06f, 0.05f, 1.0f, Color{ 150, 106, 66, 255 });
    }

    for (int k = 0; k < 4; k++)
    {
        CajaCapitan({ 0.0f, 0.15f, 0.17f + (float)k * 0.25f }, 1.7f, 0.05f, 0.06f, Color{ 140, 98, 62, 255 });
    }

    DibujarMastilCapitan(t);
    DibujarAparejoCapitan();

    // Utileria.
    DibujarCanonCapitan(-1.0f, 3.9f);
    DibujarCanonCapitan(1.0f, 3.9f);
    DibujarTimonCapitan({ 3.1f, ALTURA_ALCAZAR_CAPITAN, Z_POPA_CAPITAN + 1.0f });
    DibujarCofreCapitan({ -2.9f, ALTURA_ALCAZAR_CAPITAN, Z_POPA_CAPITAN + 0.75f });
    DibujarFarolCapitan({ -4.95f, ALTURA_ALCAZAR_CAPITAN + 0.9f, Z_FRENTE_ALCAZAR_CAPITAN });
    DibujarFarolCapitan({ 4.95f, ALTURA_ALCAZAR_CAPITAN + 0.9f, Z_FRENTE_ALCAZAR_CAPITAN });

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    DibujarBarrilCapitan({ -4.35f, ALTURA_ALCAZAR_CAPITAN, Z_POPA_CAPITAN + 0.65f }, 0.9f, 0.36f);
    DibujarBarrilCapitan({ -4.5f, ALTURA_ALCAZAR_CAPITAN, Z_POPA_CAPITAN + 1.5f }, 0.8f, 0.32f);
    CajaCapitan({ -3.7f, ALTURA_ALCAZAR_CAPITAN + 0.3f, Z_POPA_CAPITAN + 1.45f }, 0.6f, 0.6f, 0.6f, Color{ 150, 112, 70, 255 });
    DibujarBarrilCapitan({ -4.55f, 0.0f, 5.4f }, 0.95f, 0.38f);
    DibujarBarrilCapitan({ -3.75f, 0.0f, 5.8f }, 0.95f, 0.38f);
    DibujarBarrilCapitan({ -4.2f, 0.95f, 5.6f }, 0.8f, 0.32f);
    DibujarRolloCuerdaCapitan({ 4.4f, 0.0f, 5.5f });
    DibujarRolloCuerdaCapitan({ 4.0f, ALTURA_ALCAZAR_CAPITAN, Z_POPA_CAPITAN + 1.9f });
    CajaCapitan({ 3.6f, 0.3f, 6.2f }, 0.7f, 0.6f, 0.7f, Color{ 150, 112, 70, 255 });
}


//==================================================
// CAPITAN
//==================================================

// Altura de cada bandera (0 abajo, 1 arriba). Solo depende de la orden
// mientras se responde (y al resolverla, cuando ya es publica): en la
// espera y en la cuenta las dos estan abajo y quietas por igual.
static float AlturaBanderaCapitan(const MinijuegoCapitanManda& minijuego, AccionDireccionalControl lado)
{
    if (minijuego.fase == FASE_CAPITAN_TERMINADO)
    {
        return 1.0f;    // festejo: las dos arriba
    }

    if (minijuego.ordenActual != lado)
    {
        return 0.0f;
    }

    if (minijuego.fase == FASE_CAPITAN_RESPONDIENDO)
    {
        // Arranca ya casi arriba: la orden se lee desde el primer frame.
        float e = minijuego.tiempoRespuesta - minijuego.tiempoFase;
        return 0.6f + 0.4f * EaseOutBackCapitan(Limitar01Capitan(e / 0.14f));
    }

    if (minijuego.fase == FASE_CAPITAN_RESOLVIENDO)
    {
        float k = Limitar01Capitan(minijuego.tiempoFase / 0.45f);
        return k * k * (3.0f - 2.0f * k);
    }

    return 0.0f;
}


static void DibujarBanderaCapitan(Vector3 tope, float lado, float alto, Color color, float t)
{
    const int NU = 6;
    const int NV = 3;
    float subida = Limitar01Capitan(alto);

    auto punto = [&](int u, int v) -> Vector3
    {
        float fu = (float)u / (float)NU;
        float fv = (float)v / (float)NV;
        float ancho = 0.42f + 0.62f * subida;
        float onda = std::sin(t * 8.0f - fu * 5.0f) * 0.08f * fu * (0.3f + 0.7f * subida);
        return {
            tope.x + lado * fu * ancho,
            tope.y - fv * 0.6f - fu * (1.0f - subida) * 0.3f + onda * 0.4f,
            tope.z + onda
        };
    };

    rlBegin(RL_QUADS);

    for (int u = 0; u < NU; u++)
    {
        for (int v = 0; v < NV; v++)
        {
            Color c = u == 0 ? Color{ 246, 244, 236, 255 } : color;
            float luz = 0.86f + 0.14f * std::sin(t * 8.0f - ((float)u + 0.5f) / (float)NU * 5.0f);
            c = TonoCapitan(c, luz);
            Vector3 a = punto(u, v), b = punto(u + 1, v), cc = punto(u + 1, v + 1), d = punto(u, v + 1);
            QuadCapitan(a, b, cc, d, c);
            QuadCapitan(a, d, cc, b, TonoCapitan(c, 0.82f));
        }
    }

    rlEnd();

    (DrawCylinderEx)({ tope.x, tope.y - 1.25f, tope.z }, { tope.x, tope.y + 0.05f, tope.z }, 0.03f, 0.03f, 6, Color{ 110, 78, 50, 255 });
    (DrawSphere)({ tope.x, tope.y + 0.09f, tope.z }, 0.055f, Color{ 230, 186, 80, 255 });
}


static void DibujarBrazoCapitan(Vector3 hombro, Vector3 mano, float lado, Color abrigo)
{
    Vector3 codo =
    {
        (hombro.x + mano.x) * 0.5f + lado * 0.1f,
        (hombro.y + mano.y) * 0.5f - 0.04f,
        (hombro.z + mano.z) * 0.5f + 0.06f
    };

    (DrawCylinderEx)(hombro, codo, 0.1f, 0.09f, 8, abrigo);
    (DrawCylinderEx)(codo, mano, 0.09f, 0.085f, 8, abrigo);

    Vector3 puno = { codo.x + (mano.x - codo.x) * 0.78f, codo.y + (mano.y - codo.y) * 0.78f, codo.z + (mano.z - codo.z) * 0.78f };
    (DrawCylinderEx)(puno, mano, 0.11f, 0.11f, 8, Color{ 214, 170, 70, 255 });
    (DrawSphere)(mano, 0.085f, Color{ 230, 188, 148, 255 });
}


static void DibujarCapitan3D(const MinijuegoCapitanManda& minijuego)
{
    float t = minijuego.tiempoAnimacion;
    const float z = Z_CAPITAN;
    const float y0 = ALTURA_ALCAZAR_CAPITAN;

    // Respiracion e idle simetricos: nada se inclina hacia un lado.
    float respira = std::sin(t * 2.2f) * 0.02f;
    float cabeceo = std::sin(t * 1.1f) * 0.015f;

    const Color abrigo = { 40, 54, 92, 255 };
    const Color oro = { 224, 180, 76, 255 };
    const Color piel = { 232, 190, 150, 255 };
    const Color barba = { 72, 48, 34, 255 };

    DibujarSombraRetroCircular({ 0.0f, y0, z }, 0.7f, 0.52f, y0 + 0.018f);

    // Algo mas grande que la tripulacion, escalado desde sus pies.
    rlPushMatrix();
    rlTranslatef(0.0f, y0, z);
    rlScalef(ESCALA_CAPITAN, ESCALA_CAPITAN, ESCALA_CAPITAN);
    rlTranslatef(0.0f, -y0, -z);

    // Botas y calzas.
    for (int s = -1; s <= 1; s += 2)
    {
        CajaCapitan({ (float)s * 0.17f, y0 + 0.09f, z + 0.03f }, 0.22f, 0.18f, 0.34f, Color{ 34, 28, 26, 255 });
        CajaCapitan({ (float)s * 0.17f, y0 + 0.36f, z }, 0.2f, 0.4f, 0.22f, Color{ 206, 194, 166, 255 });
    }

    // Faldon del abrigo y torso.
    (DrawCylinder)({ 0.0f, y0 + 0.32f, z }, 0.37f, 0.45f, 0.56f, 12, TonoCapitan(abrigo, 0.92f));
    CajaCapitan({ 0.0f, y0 + 1.08f + respira * 0.5f, z }, 0.76f, 0.56f + respira, 0.46f, abrigo);

    // Solapas, botones, cinturon y pañuelo.
    for (int s = -1; s <= 1; s += 2)
    {
        CajaCapitan({ (float)s * 0.13f, y0 + 1.1f, z + 0.235f }, 0.1f, 0.48f, 0.03f, Color{ 150, 46, 46, 255 });

        for (int b = 0; b < 3; b++)
        {
            (DrawSphere)({ (float)s * 0.22f, y0 + 1.22f - (float)b * 0.15f, z + 0.24f }, 0.032f, oro);
        }

        // Charreteras con flecos.
        CajaCapitan({ (float)s * 0.43f, y0 + 1.37f + respira, z }, 0.24f, 0.06f, 0.3f, oro);
        for (int f = 0; f < 3; f++)
        {
            (DrawCylinderEx)(
                { (float)s * (0.36f + 0.06f * (float)f), y0 + 1.35f + respira, z + 0.13f },
                { (float)s * (0.36f + 0.06f * (float)f), y0 + 1.25f + respira, z + 0.13f },
                0.015f, 0.015f, 4, oro
            );
        }
    }

    CajaCapitan({ 0.0f, y0 + 0.86f, z }, 0.8f, 0.09f, 0.48f, Color{ 58, 40, 30, 255 });
    CajaCapitan({ 0.0f, y0 + 0.86f, z + 0.245f }, 0.13f, 0.11f, 0.02f, oro);
    CajaCapitan({ 0.0f, y0 + 1.33f, z + 0.2f }, 0.18f, 0.16f, 0.1f, Color{ 244, 240, 230, 255 });

    // Cabeza.
    float yCabeza = y0 + 1.62f + respira + cabeceo;
    (DrawSphere)({ 0.0f, yCabeza, z }, 0.27f, piel);
    (DrawSphere)({ 0.0f, yCabeza - 0.02f, z + 0.26f }, 0.05f, TonoCapitan(piel, 0.92f));
    CajaCapitan({ 0.0f, yCabeza - 0.17f, z + 0.1f }, 0.4f, 0.2f, 0.26f, barba);
    CajaCapitan({ 0.0f, yCabeza - 0.08f, z + 0.25f }, 0.26f, 0.05f, 0.05f, barba);

    // Ojos (parpadeo a la vez) y cejas.
    bool parpadea = std::fmod(t, 3.7f) < 0.12f;
    for (int s = -1; s <= 1; s += 2)
    {
        (DrawCube)({ (float)s * 0.09f, yCabeza + 0.05f, z + 0.245f }, 0.055f, parpadea ? 0.012f : 0.055f, 0.02f, Color{ 30, 26, 30, 255 });
        (DrawCube)({ (float)s * 0.1f, yCabeza + 0.12f, z + 0.24f }, 0.1f, 0.025f, 0.03f, barba);
    }

    // Tricornio con ribete dorado.
    float yAla = yCabeza + 0.17f;
    (DrawCylinder)({ 0.0f, yAla, z }, 0.5f, 0.42f, 0.14f, 3, Color{ 30, 28, 34, 255 });
    (DrawCylinderWires)({ 0.0f, yAla + 0.1f, z }, 0.5f, 0.5f, 0.05f, 3, oro);
    (DrawCylinder)({ 0.0f, yAla + 0.02f, z }, 0.22f, 0.26f, 0.3f, 10, Color{ 38, 36, 42, 255 });
    (DrawSphere)({ 0.0f, yAla + 0.22f, z + 0.25f }, 0.05f, oro);

    // Brazos y banderas: misma pose a los dos lados mientras no hay orden.
    const AccionDireccionalControl lados[2] = { CONTROL_DIRECCION_IZQUIERDA, CONTROL_DIRECCION_DERECHA };

    for (int k = 0; k < 2; k++)
    {
        float s = k == 0 ? -1.0f : 1.0f;
        float alto = AlturaBanderaCapitan(minijuego, lados[k]);
        float a = Limitar01Capitan(alto);

        Vector3 hombro = { s * 0.44f, y0 + 1.3f + respira, z + 0.02f };
        Vector3 manoBaja = { s * 0.6f, y0 + 0.72f, z + 0.2f };
        Vector3 manoAlta = { s * 0.98f, y0 + 2.0f, z + 0.1f };
        Vector3 mano =
        {
            manoBaja.x + (manoAlta.x - manoBaja.x) * a,
            manoBaja.y + (manoAlta.y - manoBaja.y) * alto,
            manoBaja.z + (manoAlta.z - manoBaja.z) * a
        };

        DibujarBrazoCapitan(hombro, mano, s, abrigo);
        DibujarBanderaCapitan(
            { mano.x, mano.y + 0.85f, mano.z + 0.02f },
            s,
            alto,
            k == 0 ? COLOR_BANDERA_IZQUIERDA_CAPITAN : COLOR_BANDERA_DERECHA_CAPITAN,
            t
        );
    }

    rlPopMatrix();
}


//==================================================
// JUGADORES
//==================================================

static void DibujarJugadoresCapitan3D(
    const MinijuegoCapitanManda& minijuego,
    const Participante participantes[]
)
{
    float t = minijuego.tiempoAnimacion;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCapitanManda& jugador = minijuego.jugadores[i];
        Color color = participantes[i].color;
        Vector3 base = POSICIONES_JUGADOR_CAPITAN[i];

        // Esterilla de cuerda con anillo del color del jugador.
        (DrawCylinder)({ base.x, 0.0f, base.z }, 0.8f, 0.84f, 0.07f, 22, Color{ 172, 136, 90, 255 });
        for (int k = 0; k < 4; k++)
        {
            DrawCircle3D({ base.x, 0.075f, base.z }, 0.2f + 0.14f * (float)k, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 120, 92, 60, 255 });
        }
        AnilloCapitan(base.x, 0.08f, base.z, 0.62f, 0.78f, jugador.eliminado ? Fade(color, 0.35f) : color);

        if (!jugador.eliminado)
        {
            float tiembla = std::sin(t * 60.0f) * 0.07f * jugador.sacudida;
            DibujarSombraRetroCircular({ base.x, 0.08f, base.z }, 0.45f, 0.38f, 0.1f);
            DibujarModeloJugadorEnPosicion({ base.x + tiembla, 0.08f, base.z }, jugador.giroVisual, color, ESCALA_JUGADOR_CAPITAN * ESCALA_MODELO_JUGADOR_3D);
            continue;
        }

        // Cae por la borda: arco hacia el agua dando vueltas.
        if (jugador.tiempoCaida >= 0.0f && jugador.tiempoCaida < DURACION_CAIDA_CAPITAN)
        {
            float u = jugador.tiempoCaida / DURACION_CAIDA_CAPITAN;
            Vector3 agua = POSICIONES_AGUA_CAPITAN[i];
            float lado = agua.x < 0.0f ? -1.0f : 1.0f;
            Vector3 p =
            {
                base.x + (agua.x - base.x) * u,
                0.08f + (NIVEL_MAR_CAPITAN - 0.6f - 0.08f) * u + 3.4f * 4.0f * u * (1.0f - u) * 0.75f,
                base.z + (agua.z - base.z) * u
            };

            rlPushMatrix();
            rlTranslatef(p.x, p.y + 0.7f, p.z);
            rlRotatef(-lado * 330.0f * u, 0.0f, 0.0f, 1.0f);
            DibujarModeloJugadorEnPosicion({ 0.0f, -0.7f, 0.0f }, jugador.giroVisual, color, ESCALA_JUGADOR_CAPITAN * ESCALA_MODELO_JUGADOR_3D);
            rlPopMatrix();
        }
    }
}


// Los que cayeron flotan con un salvavidas de su color (fuera del barco).
static void DibujarNaufragosCapitan(
    const MinijuegoCapitanManda& minijuego,
    const Participante participantes[]
)
{
    float t = minijuego.tiempoAnimacion;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorCapitanManda& jugador = minijuego.jugadores[i];

        if (
            !minijuego.resultado.participantes[i].participo ||
            !jugador.eliminado ||
            jugador.tiempoCaida < DURACION_CAIDA_CAPITAN
        )
        {
            continue;
        }

        Vector3 agua = POSICIONES_AGUA_CAPITAN[i];
        float bob = std::sin(t * 1.7f + (float)i) * 0.08f;
        float hundido = 1.0f - Limitar01Capitan((jugador.tiempoCaida - DURACION_CAIDA_CAPITAN) / 0.4f);
        float y = NIVEL_MAR_CAPITAN + bob - 0.5f * hundido;
        Color color = participantes[i].color;

        DibujarModeloJugadorEnPosicion({ agua.x, y - 1.0f, agua.z }, 0.0f, color, ESCALA_JUGADOR_CAPITAN * ESCALA_MODELO_JUGADOR_3D);
        ToroCapitan({ agua.x, y + 0.05f, agua.z }, 0.5f, 0.13f, Color{ 244, 242, 236, 255 }, color, 4);
    }
}


static void DibujarMechaCapitan(const MinijuegoCapitanManda& minijuego)
{
    if (minijuego.fase != FASE_CAPITAN_RESPONDIENDO)
    {
        return;
    }

    // Mecha simetrica sobre la baranda: se consume hacia el centro.
    float fraccion = minijuego.tiempoRespuesta > 0.0f
        ? Limitar01Capitan(minijuego.tiempoFase / minijuego.tiempoRespuesta)
        : 0.0f;
    float mitad = 3.2f * fraccion;
    float y = ALTURA_ALCAZAR_CAPITAN + 0.52f;
    float z = Z_FRENTE_ALCAZAR_CAPITAN + 0.02f;

    if (mitad > 0.02f)
    {
        (DrawCylinderEx)({ -mitad, y, z }, { mitad, y, z }, 0.035f, 0.035f, 6, Color{ 70, 54, 40, 255 });
    }

    Color chispa = fraccion < 0.3f ? Color{ 255, 90, 60, 255 } : Color{ 255, 200, 90, 255 };
    (DrawSphere)({ -mitad, y, z }, 0.07f, chispa);
    (DrawSphere)({ mitad, y, z }, 0.07f, chispa);
}


static void DibujarParticulasCapitan(const MinijuegoCapitanManda& minijuego)
{
    for (int i = 0; i < MAX_PARTICULAS_CAPITAN; i++)
    {
        const ParticulaCapitanManda& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        float tam = p.tamano * (0.5f + 0.5f * vida);
        (DrawCube)(p.posicion, tam, tam, tam, Fade(p.color, Limitar01Capitan(vida * 1.6f)));
    }
}


// Anillo rojo que se expande donde estaba el eliminado y espuma donde cae.
static void DibujarAnillosCapitan(const MinijuegoCapitanManda& minijuego)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorCapitanManda& jugador = minijuego.jugadores[i];

        if (!minijuego.resultado.participantes[i].participo || jugador.tiempoCaida < 0.0f)
        {
            continue;
        }

        float u = jugador.tiempoCaida / 0.6f;
        if (u < 1.0f)
        {
            Vector3 base = POSICIONES_JUGADOR_CAPITAN[i];
            float r = 0.5f + 1.0f * u;
            AnilloCapitan(base.x, 0.1f, base.z, r - 0.12f, r, Fade(Color{ 236, 64, 52, 255 }, 0.8f * (1.0f - u)));
        }

        float s = (jugador.tiempoCaida - DURACION_CAIDA_CAPITAN) / 0.9f;
        if (s >= 0.0f && s < 1.0f)
        {
            Vector3 agua = POSICIONES_AGUA_CAPITAN[i];
            float r = 0.4f + 1.6f * s;
            AnilloCapitan(agua.x, NIVEL_MAR_CAPITAN + 0.15f, agua.z, r - 0.25f, r, Fade(Color{ 240, 248, 250, 255 }, 0.8f * (1.0f - s)));
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarLucesCapitan(
    const MinijuegoCapitanManda& minijuego,
    const Participante participantes[]
)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f)
    {
        return;
    }

    float t = minijuego.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Sol y su reflejo sobre el mar.
    ResplandorFrenteCapitan({ 40.0f, 13.0f, -166.0f }, 26.0f, Color{ 255, 190, 120, 90 });
    ResplandorCapitan({ 34.0f, NIVEL_MAR_CAPITAN + 0.2f, -80.0f }, { 4.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 70.0f }, 1.0f, Color{ 255, 214, 150, 60 });

    // Faro de la isla: destello lento.
    float faro = 0.5f + 0.5f * std::sin(t * 2.0f);
    ResplandorFrenteCapitan({ -39.6f, NIVEL_MAR_CAPITAN + 8.4f, -87.0f }, 3.0f, Color{ 255, 230, 160, (unsigned char)(40.0f + 110.0f * faro) });

    // Faroles del alcazar: halo y charco calido que laten.
    for (int s = -1; s <= 1; s += 2)
    {
        float pulso = 0.85f + 0.15f * std::sin(t * 9.0f + (float)s * 1.7f);
        Vector3 luz = { (float)s * 4.95f, ALTURA_ALCAZAR_CAPITAN + 2.02f, Z_FRENTE_ALCAZAR_CAPITAN };
        ResplandorFrenteCapitan({ luz.x, luz.y, luz.z + 0.15f }, 0.9f * pulso, Color{ 255, 196, 110, 110 });
        ResplandorSueloCapitan(luz.x, ALTURA_ALCAZAR_CAPITAN + 0.02f, luz.z - 0.6f, 1.5f * pulso, Color{ 255, 176, 90, 45 });
        ResplandorSueloCapitan(luz.x * 0.92f, 0.02f, luz.z + 0.9f, 1.6f * pulso, Color{ 255, 176, 90, 38 });
    }

    // Bandera que sube: halo de su color detras de la tela.
    if (minijuego.fase == FASE_CAPITAN_RESPONDIENDO)
    {
        float s = minijuego.ordenActual == CONTROL_DIRECCION_IZQUIERDA ? -1.0f : 1.0f;
        Color c = minijuego.ordenActual == CONTROL_DIRECCION_IZQUIERDA ? COLOR_BANDERA_IZQUIERDA_CAPITAN : COLOR_BANDERA_DERECHA_CAPITAN;
        c.a = 120;
        ResplandorFrenteCapitan({ s * 1.7f, ALTURA_ALCAZAR_CAPITAN + 2.95f, Z_CAPITAN - 0.05f }, 1.3f, c);

        // Chispas de la mecha.
        float fraccion = minijuego.tiempoRespuesta > 0.0f ? Limitar01Capitan(minijuego.tiempoFase / minijuego.tiempoRespuesta) : 0.0f;
        float mitad = 3.2f * fraccion;
        for (int k = -1; k <= 1; k += 2)
        {
            ResplandorFrenteCapitan({ (float)k * mitad, ALTURA_ALCAZAR_CAPITAN + 0.52f, Z_FRENTE_ALCAZAR_CAPITAN + 0.08f }, 0.35f + 0.08f * std::sin(t * 40.0f), Color{ 255, 170, 70, 150 });
        }
    }

    // Brillo de acierto bajo cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorCapitanManda& jugador = minijuego.jugadores[i];

        if (!minijuego.resultado.participantes[i].participo || jugador.eliminado || jugador.destello <= 0.0f)
        {
            continue;
        }

        Vector3 base = POSICIONES_JUGADOR_CAPITAN[i];
        Color c = ColorLerp(participantes[i].color, Color{ 255, 226, 96, 255 }, 0.5f);
        c.a = (unsigned char)(150.0f * jugador.destello);
        ResplandorSueloCapitan(base.x, 0.1f, base.z, 1.3f, c);
        ResplandorFrenteCapitan({ base.x, 1.0f, base.z + 0.3f }, 1.1f * jugador.destello + 0.3f, Color{ 255, 236, 160, (unsigned char)(90.0f * jugador.destello) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoCapitanManda::Dibujar(
    const Participante participantes[]
) const
{
    const float t = tiempoAnimacion;

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    ClearBackground(Color{ 96, 140, 204, 255 });

    // Camara fija; en la cuenta se acerca un poco (x reducir movimiento).
    Camera3D camara{};
    float intro = fase == FASE_CAPITAN_PREPARACION
        ? (tiempoPreparacion / DURACION_PREPARACION_CAPITAN) * ObtenerFactorMovimientoCamaraMinijuegos()
        : 0.0f;
    intro *= intro;
    camara.position = { 0.0f, 5.0f + 1.2f * intro, 12.5f + 3.0f * intro };
    camara.target = { 0.0f, 2.6f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 47.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(camara);

    DibujarCieloCapitan(t);
    DibujarFondoCapitan(t);
    DibujarMarCapitan(t);
    DibujarNaufragosCapitan(*this, participantes);
    DibujarEspumaYNieblaCapitan(t);
    DibujarGaviotasCapitan(t);

    rlPushMatrix();
    AplicarBalanceoCapitan(t);
    DibujarCubiertaCapitan(t);
    DibujarCapitan3D(*this);
    DibujarJugadoresCapitan3D(*this, participantes);
    DibujarMechaCapitan(*this);
    DibujarAnillosCapitan(*this);
    rlPopMatrix();

    DibujarParticulasCapitan(*this);

    rlPushMatrix();
    AplicarBalanceoCapitan(t);
    DibujarLucesCapitan(*this, participantes);
    rlPopMatrix();

    EndMode3D();

    // Rotulos J1..J4 sobre los que siguen a bordo.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || jugadores[i].eliminado)
        {
            continue;
        }

        Vector3 cabeza = POSICIONES_JUGADOR_CAPITAN[i];
        cabeza.y = 1.9f;
        Vector2 p = GetWorldToScreen(cabeza, camara);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, -5.0f, op);
    }

    // Orden escrita junto a la bandera, solo cuando ya subio.
    if (fase == FASE_CAPITAN_RESPONDIENDO)
    {
        bool izquierda = ordenActual == CONTROL_DIRECCION_IZQUIERDA;
        float s = izquierda ? -1.0f : 1.0f;
        Vector2 p = GetWorldToScreen({ s * 1.7f, ALTURA_ALCAZAR_CAPITAN + 3.55f, Z_CAPITAN }, camara);
        float y = p.y - (float)TamanoTexto(UI_TEXTO_GRANDE);
        if (y < Px(140.0f)) y = Px(140.0f);

        Color color = izquierda
            ? Color{ 120, 182, 255, 255 }
            : Color{ 255, 166, 84, 255 };

        DibujarTextoMundoContornoUI(izquierda ? "¡IZQUIERDA!" : "¡DERECHA!", p.x, y, UI_TEXTO_GRANDE, color);
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CAPITÁN MANDA",
        "CUANDO EL CAPITÁN SUBA UNA BANDERA, GIRÁ YA HACIA ESE LADO. SI FALLÁS O TARDÁS, AL AGUA.",
        resultado
    );

    hud.fase = fase == FASE_CAPITAN_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CAPITAN_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin limite de tiempo: ronda actual y cuanto se acorta la reaccion.
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "RONDA";
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d", numeroRonda);
    hud.progresoMedidor = numeroRonda > 0
        ? (1.55f - ObtenerTiempoRespuestaRonda(numeroRonda)) / (1.55f - 0.60f)
        : 0.0f;

    int vivos = ContarVivosCapitan(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        const EstadoJugadorCapitanManda& estado = jugadores[i];
        j.eliminado = estado.eliminado;
        j.valorNumerico = estado.rondasSuperadas;

        if (!estado.eliminado)
        {
            if (fase == FASE_CAPITAN_MOSTRANDO)
            {
                std::snprintf(j.estado, sizeof(j.estado), "ATENTO");
            }
            else if (fase == FASE_CAPITAN_RESPONDIENDO && estado.respondio)
            {
                std::snprintf(j.estado, sizeof(j.estado), "%s", estado.acerto ? "¡BIEN!" : "¡MAL!");
            }
            else if (fase == FASE_CAPITAN_RESOLVIENDO)
            {
                std::snprintf(j.estado, sizeof(j.estado), "¡A SALVO!");
            }
        }

        std::snprintf(
            j.detalleFinal,
            sizeof(j.detalleFinal),
            "%d RONDA%s",
            estado.rondasSuperadas,
            estado.rondasSuperadas == 1 ? "" : "S"
        );

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "IZQUIERDA: %s\nDERECHA: %s",
                ObtenerTextoAccionDireccionalControl(participantes[i], CONTROL_DIRECCION_IZQUIERDA),
                ObtenerTextoAccionDireccionalControl(participantes[i], CONTROL_DIRECCION_DERECHA)
            );
        }
    }

    // Tablon final: quienes caen en la misma ronda comparten puesto.
    if (fase == FASE_CAPITAN_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (vivos == 0)
        {
            hud.tituloFinal = "¡TODOS AL AGUA!";
        }

        for (int i = 0; i < MAX_PARTICIPANTES && hud.notaFinal == nullptr; i++)
        {
            if (!hud.jugadores[i].visible) continue;

            for (int k = i + 1; k < MAX_PARTICIPANTES; k++)
            {
                if (!hud.jugadores[k].visible) continue;
                if (resultado.participantes[k].posicionFinal != resultado.participantes[i].posicionFinal) continue;

                hud.notaFinal = jugadores[i].eliminado
                    ? "CAYERON EN LA MISMA RONDA: COMPARTEN PUESTO"
                    : "SIGUEN A BORDO TRAS 40 RONDAS: COMPARTEN PUESTO";
                break;
            }
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoCapitanManda::ObtenerResultado() const
{
    return resultado;
}
