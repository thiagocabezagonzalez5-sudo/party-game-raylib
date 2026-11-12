#include "Minigames/MinijuegoUltimoAsiento.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstring>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_ASIENTO = 3.0f;
static const float DURACION_PARTIDA_ASIENTO = 60.0f;

// Fase de musica (aleatoria), ventana de tazas y pausa tras cerrar.
static const float MUSICA_MINIMA_ASIENTO = 2.5f;
static const float MUSICA_MAXIMA_ASIENTO = 6.0f;
static const float DURACION_TAZAS_ASIENTO = 3.0f;
static const float DURACION_RESOLUCION_ASIENTO = 1.6f;

// La taza trampa parpadea en rojo durante los ultimos segundos de la ventana.
static const float DURACION_AVISO_TRAMPA_ASIENTO = 1.1f;
static const int PROBABILIDAD_TRAMPA_ASIENTO = 45;

// Geometria. La arena es un disco; el suelo fisico queda en SUELO_ASIENTO.
static const float RADIO_ARENA_ASIENTO = 8.5f;
static const float RADIO_LIMITE_ASIENTO = 8.0f;
static const float RADIO_CARRUSEL_FISICO_ASIENTO = 2.5f;
static const float RADIO_CARRUSEL_VISUAL_ASIENTO = 2.1f;
static const float SUELO_ASIENTO = -0.05f;

// Tazas: radio visual, radio para entrar y radio para conservar el asiento.
static const float RADIO_TAZA_ASIENTO = 1.25f;
static const float RADIO_ENTRADA_TAZA_ASIENTO = 0.95f;
static const float RADIO_MANTENER_TAZA_ASIENTO = 1.35f;
static const float ANILLO_TAZA_MINIMO_ASIENTO = 4.4f;
static const float ANILLO_TAZA_MAXIMO_ASIENTO = 6.6f;
static const float SEPARACION_TAZAS_ASIENTO = 3.2f;

// Grada de espectadores para los eliminados.
static const float ALTURA_GRADA_ASIENTO = 0.7f;
static const float DISTANCIA_GRADA_ASIENTO = RADIO_ARENA_ASIENTO + 2.6f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarAsiento(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioAsiento(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static int LimiteAsiento(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static int ContarVivosAsiento(const MinijuegoUltimoAsiento& m, int limite)
{
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.vivo[i])
        {
            cantidad++;
        }
    }

    return cantidad;
}


static Vector3 PuestoEspectadorAsiento(int indice)
{
    float lado = (indice % 2 == 0) ? -1.0f : 1.0f;

    return
    {
        lado * DISTANCIA_GRADA_ASIENTO,
        ALTURA_GRADA_ASIENTO + 0.72f,
        -1.5f + (float)(indice / 2) * 2.4f
    };
}


static void AsignarColoresAsiento(
    MinijuegoUltimoAsiento& m,
    const Participante participantes[]
)
{
    const Color respaldo[] =
    {
        Color{ 232, 62, 62, 255 },
        Color{ 62, 124, 238, 255 },
        Color{ 66, 202, 96, 255 },
        Color{ 246, 206, 52, 255 },
        Color{ 232, 92, 204, 255 },
        Color{ 250, 144, 44, 255 }
    };
    const int cantidadRespaldo = (int)(sizeof(respaldo) / sizeof(respaldo[0]));

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.coloresJugadores[i] = participantes[i].color;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!participantes[i].activo || !participantes[i].esBot)
        {
            continue;
        }

        for (int c = 0; c < cantidadRespaldo; c++)
        {
            bool repetido = false;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    j != i &&
                    participantes[j].activo &&
                    !participantes[j].esBot &&
                    participantes[j].color.r == respaldo[c].r &&
                    participantes[j].color.g == respaldo[c].g &&
                    participantes[j].color.b == respaldo[c].b
                )
                {
                    repetido = true;
                }
            }

            for (int j = 0; j < i; j++)
            {
                if (
                    participantes[j].activo &&
                    participantes[j].esBot &&
                    m.coloresJugadores[j].r == respaldo[c].r &&
                    m.coloresJugadores[j].g == respaldo[c].g &&
                    m.coloresJugadores[j].b == respaldo[c].b
                )
                {
                    repetido = true;
                }
            }

            if (!repetido)
            {
                m.coloresJugadores[i] = respaldo[c];
                break;
            }
        }
    }
}


//==================================================
// LOGICA DE RONDAS (datos logicos, sin dependencia visual)
//==================================================

static bool TrampaVisibleAsiento(
    const MinijuegoUltimoAsiento& m,
    int taza,
    float retardo
)
{
    return
        taza >= 0 &&
        m.tazas[taza].trampa &&
        m.tiempoSubfase <= DURACION_AVISO_TRAMPA_ASIENTO - retardo;
}


static void IniciarMusicaAsiento(MinijuegoUltimoAsiento& m)
{
    m.subfase = SUBFASE_ULTIMO_ASIENTO_MUSICA;
    m.tiempoSubfase = AleatorioAsiento(MUSICA_MINIMA_ASIENTO, MUSICA_MAXIMA_ASIENTO);
    m.cantidadTazas = 0;
    m.indiceTrampa = -1;

    for (int c = 0; c < MAX_TAZAS_ULTIMO_ASIENTO; c++)
    {
        m.tazas[c] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.tazaDe[i] = -1;
        m.bots[i].objetivoTaza = -1;
        m.bots[i].tiempoVagar = 0.0f;
    }
}


static void IniciarTazasAsiento(MinijuegoUltimoAsiento& m, int limite)
{
    int vivos = ContarVivosAsiento(m, limite);
    int cantidad = vivos - 1;

    if (cantidad < 1) cantidad = 1;
    if (cantidad > MAX_TAZAS_ULTIMO_ASIENTO) cantidad = MAX_TAZAS_ULTIMO_ASIENTO;

    m.ronda++;
    m.subfase = SUBFASE_ULTIMO_ASIENTO_TAZAS;
    m.tiempoSubfase = DURACION_TAZAS_ASIENTO;
    m.cantidadTazas = cantidad;
    m.avisoTrampaDado = false;
    m.indiceTrampa = -1;

    for (int c = 0; c < MAX_TAZAS_ULTIMO_ASIENTO; c++)
    {
        m.tazas[c] = {};
    }

    // Posiciones aleatorias en el anillo, separadas entre si. Si no se logra
    // tras varios intentos se reparten uniformemente (siempre valido).
    for (int c = 0; c < cantidad; c++)
    {
        bool colocada = false;

        for (int intento = 0; intento < 40 && !colocada; intento++)
        {
            float angulo = AleatorioAsiento(0.0f, 2.0f * PI);
            float radio = AleatorioAsiento(ANILLO_TAZA_MINIMO_ASIENTO, ANILLO_TAZA_MAXIMO_ASIENTO);
            Vector3 candidata = { std::cos(angulo) * radio, SUELO_ASIENTO, std::sin(angulo) * radio };
            bool libre = true;

            for (int k = 0; k < c; k++)
            {
                float dx = candidata.x - m.tazas[k].posicion.x;
                float dz = candidata.z - m.tazas[k].posicion.z;

                if (std::sqrt(dx * dx + dz * dz) < SEPARACION_TAZAS_ASIENTO)
                {
                    libre = false;
                }
            }

            if (libre)
            {
                m.tazas[c].posicion = candidata;
                colocada = true;
            }
        }

        if (!colocada)
        {
            float angulo = (float)c * 2.0f * PI / (float)cantidad + 0.4f;
            m.tazas[c].posicion =
            {
                std::cos(angulo) * 5.5f,
                SUELO_ASIENTO,
                std::sin(angulo) * 5.5f
            };
        }

        m.tazas[c].activa = true;
        m.tazas[c].ocupante = -1;
    }

    // La trampa solo aparece con 3 o mas vivos para que siempre quede alguien.
    if (vivos >= 3 && GetRandomValue(0, 99) < PROBABILIDAD_TRAMPA_ASIENTO)
    {
        m.indiceTrampa = GetRandomValue(0, cantidad - 1);
        m.tazas[m.indiceTrampa].trampa = true;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.tazaDe[i] = -1;
        m.bots[i].retardoBase = AleatorioAsiento(0.15f, 0.45f);
        m.bots[i].retardo = m.bots[i].retardoBase;
        m.bots[i].objetivoTaza = -1;
        m.bots[i].reevaluar = 0.0f;
    }

    ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
}


static void EliminarJugadorAsiento(
    MinijuegoUltimoAsiento& m,
    JugadorPrueba jugadores[],
    int indice
)
{
    m.vivo[indice] = false;
    m.rondaEliminacion[indice] = m.ronda;
    m.tazaDe[indice] = -1;

    JugadorPrueba& jugador = jugadores[indice];
    jugador.posicion = PuestoEspectadorAsiento(indice);
    jugador.velocidad = {};
    jugador.empuje = {};
    jugador.enSuelo = true;
    jugador.cayendo = false;
    jugador.golpeando = false;
    jugador.preparandoGolpeSuelo = false;
    jugador.golpeSueloActivo = false;
    jugador.impactoGolpeSuelo = false;
    jugador.aplastado = false;
    jugador.direccionMirada = { -(jugador.posicion.x > 0.0f ? 1.0f : -1.0f), 0.0f, 0.0f };
}


static void FinalizarPartidaAsiento(
    MinijuegoUltimoAsiento& m,
    int limite
)
{
    int sobrevivientes = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo && m.vivo[i])
        {
            sobrevivientes++;
        }
    }

    for (int i = 0; i < limite; i++)
    {
        ResultadoParticipante& r = m.resultado.participantes[i];

        if (!r.participo)
        {
            continue;
        }

        int posicion = 1;

        if (!m.vivo[i])
        {
            // Mejor posicion cuanto mas tarde se fue eliminado.
            for (int j = 0; j < limite; j++)
            {
                if (
                    j != i &&
                    m.resultado.participantes[j].participo &&
                    (m.vivo[j] || m.rondaEliminacion[j] > m.rondaEliminacion[i])
                )
                {
                    posicion++;
                }
            }
        }

        r.posicionFinal = posicion;
        r.puntuacionMinijuego = m.vivo[i] ? m.ronda : m.rondaEliminacion[i] - 1;

        if (r.puntuacionMinijuego < 0)
        {
            r.puntuacionMinijuego = 0;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = sobrevivientes == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    m.fase = FASE_ULTIMO_ASIENTO_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


static void ResolverRondaAsiento(
    MinijuegoUltimoAsiento& m,
    JugadorPrueba jugadores[],
    int limite
)
{
    bool sobrevive[MAX_PARTICIPANTES]{};
    int cantidadSobrevivientes = 0;
    bool trampaOcupada = false;

    for (int i = 0; i < limite; i++)
    {
        int taza = m.tazaDe[i];

        if (!m.vivo[i] || taza < 0)
        {
            continue;
        }

        if (m.tazas[taza].trampa)
        {
            trampaOcupada = true;
            continue;
        }

        sobrevive[i] = true;
        cantidadSobrevivientes++;
    }

    m.subfase = SUBFASE_ULTIMO_ASIENTO_RESOLUCION;
    m.tiempoSubfase = DURACION_RESOLUCION_ASIENTO;

    // Si nadie quedara en pie la ronda no elimina a nadie.
    if (cantidadSobrevivientes == 0)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
        return;
    }

    if (trampaOcupada)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
    }

    bool huboEliminado = false;

    for (int i = 0; i < limite; i++)
    {
        if (m.vivo[i] && !sobrevive[i])
        {
            EliminarJugadorAsiento(m, jugadores, i);
            huboEliminado = true;
        }
    }

    if (huboEliminado)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ELIMINADO);
    }
}


static void ActualizarOcupacionAsiento(
    MinijuegoUltimoAsiento& m,
    const JugadorPrueba jugadores[],
    int limite
)
{
    for (int c = 0; c < m.cantidadTazas; c++)
    {
        TazaUltimoAsiento& taza = m.tazas[c];

        if (!taza.activa || taza.ocupante < 0)
        {
            continue;
        }

        int i = taza.ocupante;
        float dx = jugadores[i].posicion.x - taza.posicion.x;
        float dz = jugadores[i].posicion.z - taza.posicion.z;

        if (!m.vivo[i] || std::sqrt(dx * dx + dz * dz) > RADIO_MANTENER_TAZA_ASIENTO)
        {
            taza.ocupante = -1;
            m.tazaDe[i] = -1;
        }
    }

    for (int c = 0; c < m.cantidadTazas; c++)
    {
        TazaUltimoAsiento& taza = m.tazas[c];

        if (!taza.activa || taza.ocupante >= 0)
        {
            continue;
        }

        for (int i = 0; i < limite; i++)
        {
            if (!m.vivo[i] || m.tazaDe[i] >= 0 || jugadores[i].cayendo)
            {
                continue;
            }

            float dx = jugadores[i].posicion.x - taza.posicion.x;
            float dz = jugadores[i].posicion.z - taza.posicion.z;

            if (std::sqrt(dx * dx + dz * dz) <= RADIO_ENTRADA_TAZA_ASIENTO)
            {
                taza.ocupante = i;
                taza.destello = 1.0f;
                m.tazaDe[i] = c;
                ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
                break;
            }
        }
    }
}


//==================================================
// IA DE BOTS
//==================================================

static void DirigirEntradaAsiento(
    InputMinijuegoParticipante& entrada,
    float dx,
    float dz
)
{
    float longitud = std::sqrt(dx * dx + dz * dz);

    if (longitud < 0.001f)
    {
        return;
    }

    dx /= longitud;
    dz /= longitud;

    const float umbral = 0.38f;
    entrada.derecha = dx > umbral;
    entrada.izquierda = dx < -umbral;
    entrada.atras = dz > umbral;
    entrada.adelante = dz < -umbral;
}


static int ElegirTazaLibreAsiento(
    const MinijuegoUltimoAsiento& m,
    const JugadorPrueba& jugador,
    float retardo
)
{
    int mejor = -1;
    float mejorPuntaje = 1000000.0f;

    for (int c = 0; c < m.cantidadTazas; c++)
    {
        const TazaUltimoAsiento& taza = m.tazas[c];

        if (
            !taza.activa ||
            taza.ocupante >= 0 ||
            TrampaVisibleAsiento(m, c, retardo)
        )
        {
            continue;
        }

        float dx = taza.posicion.x - jugador.posicion.x;
        float dz = taza.posicion.z - jugador.posicion.z;
        float puntaje = std::sqrt(dx * dx + dz * dz) + AleatorioAsiento(0.0f, 1.2f);

        if (puntaje < mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejor = c;
        }
    }

    return mejor;
}


static InputMinijuegoParticipante CrearEntradaBotAsiento(
    MinijuegoUltimoAsiento& m,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoBotUltimoAsiento& bot = m.bots[indice];

    bot.cooldownGolpe -= deltaTime;

    if (bot.cooldownGolpe < 0.0f)
    {
        bot.cooldownGolpe = 0.0f;
    }

    bool hayDestino = false;
    float destinoX = 0.0f;
    float destinoZ = 0.0f;
    float parada = 0.2f;

    if (m.subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
    {
        bot.retardo -= deltaTime;

        if (bot.retardo <= 0.0f)
        {
            int propia = m.tazaDe[indice];

            if (propia >= 0 && !TrampaVisibleAsiento(m, propia, bot.retardoBase))
            {
                destinoX = m.tazas[propia].posicion.x;
                destinoZ = m.tazas[propia].posicion.z;
                parada = 0.15f;
                hayDestino = true;
            }
            else
            {
                int objetivo = bot.objetivoTaza;
                bot.reevaluar -= deltaTime;

                bool valido =
                    objetivo >= 0 &&
                    m.tazas[objetivo].activa &&
                    (m.tazas[objetivo].ocupante < 0 || m.tazas[objetivo].ocupante == indice) &&
                    !TrampaVisibleAsiento(m, objetivo, bot.retardoBase) &&
                    bot.reevaluar > 0.0f;

                if (!valido)
                {
                    objetivo = ElegirTazaLibreAsiento(m, jugador, bot.retardoBase);
                    bot.objetivoTaza = objetivo;
                    bot.reevaluar = AleatorioAsiento(0.8f, 1.6f);
                }

                if (objetivo >= 0)
                {
                    destinoX = m.tazas[objetivo].posicion.x;
                    destinoZ = m.tazas[objetivo].posicion.z;
                    parada = 0.15f;
                    hayDestino = true;
                }
                else
                {
                    // Sin tazas libres: empujar al ocupante mas cercano.
                    int victima = -1;
                    float mejor = 1000000.0f;

                    for (int c = 0; c < m.cantidadTazas; c++)
                    {
                        int ocupante = m.tazas[c].ocupante;

                        if (!m.tazas[c].activa || ocupante < 0 || ocupante == indice)
                        {
                            continue;
                        }

                        if (TrampaVisibleAsiento(m, c, bot.retardoBase))
                        {
                            continue;
                        }

                        float dx = m.tazas[c].posicion.x - jugador.posicion.x;
                        float dz = m.tazas[c].posicion.z - jugador.posicion.z;
                        float distancia = std::sqrt(dx * dx + dz * dz);

                        if (distancia < mejor)
                        {
                            mejor = distancia;
                            victima = ocupante;
                        }
                    }

                    if (victima >= 0)
                    {
                        // Se mueve hacia el ocupante (no hacia la taza) para
                        // quedar de frente al empujar.
                        destinoX = m.tazas[m.tazaDe[victima]].posicion.x;
                        destinoZ = m.tazas[m.tazaDe[victima]].posicion.z;
                        parada = 1.0f;
                        hayDestino = true;

                        if (mejor < 2.2f && bot.cooldownGolpe <= 0.0f)
                        {
                            entrada.golpear = true;
                            bot.cooldownGolpe = AleatorioAsiento(0.6f, 1.1f);
                        }
                    }
                }
            }
        }
    }

    if (!hayDestino)
    {
        // Deambular alrededor del carrusel.
        bot.tiempoVagar -= deltaTime;

        float dx = bot.vagarX - jugador.posicion.x;
        float dz = bot.vagarZ - jugador.posicion.z;

        if (bot.tiempoVagar <= 0.0f || std::sqrt(dx * dx + dz * dz) < 0.6f)
        {
            float angulo = AleatorioAsiento(0.0f, 2.0f * PI);
            float radio = AleatorioAsiento(3.4f, 7.2f);
            bot.vagarX = std::cos(angulo) * radio;
            bot.vagarZ = std::sin(angulo) * radio;
            bot.tiempoVagar = AleatorioAsiento(1.0f, 2.2f);
        }

        destinoX = bot.vagarX;
        destinoZ = bot.vagarZ;
        parada = 0.4f;
        hayDestino = true;
    }

    float dx = destinoX - jugador.posicion.x;
    float dz = destinoZ - jugador.posicion.z;

    if (hayDestino && std::sqrt(dx * dx + dz * dz) > parada)
    {
        DirigirEntradaAsiento(entrada, dx, dz);
    }

    return entrada;
}


//==================================================
// CICLO DE VIDA
//==================================================

static void ReiniciarVisualAsiento(MinijuegoUltimoAsiento& m);
static void ActualizarVisualAsiento(
    MinijuegoUltimoAsiento& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima
);


void MinijuegoUltimoAsiento::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        vivo[i] = false;
        rondaEliminacion[i] = 0;
        tazaDe[i] = -1;
        bots[i] = {};
        coloresJugadores[i] = LIGHTGRAY;
    }

    for (int i = 0; i < MAX_PARTICULAS_ULTIMO_ASIENTO; i++)
    {
        particulas[i] = {};
    }

    for (int c = 0; c < MAX_TAZAS_ULTIMO_ASIENTO; c++)
    {
        tazas[c] = {};
    }

    // Suelo fisico cuadrado bajo el disco: el limite circular se aplica en
    // Actualizar, asi que nadie llega a sus bordes.
    cantidadBloques = 0;
    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        MAX_BLOQUES_ULTIMO_ASIENTO,
        { 0.0f, SUELO_ASIENTO - 0.5f, 0.0f },
        { RADIO_ARENA_ASIENTO * 2.0f + 2.0f, 1.0f, RADIO_ARENA_ASIENTO * 2.0f + 2.0f },
        Color{ 80, 80, 80, 255 }
    );

    // Vista algo mas baja que la cenital: deja ver la feria, el valle y el
    // cielo detras de la plaza.
    camara.position = { 0.0f, 15.5f, 15.0f };
    camara.target = { 0.0f, 0.0f, 0.45f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 45.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_ULTIMO_ASIENTO_PREPARACION;
    subfase = SUBFASE_ULTIMO_ASIENTO_MUSICA;
    cantidadTazas = 0;
    ronda = 0;
    indiceTrampa = -1;
    avisoTrampaDado = false;
    tiempoPreparacion = DURACION_PREPARACION_ASIENTO;
    tiempoRestante = DURACION_PARTIDA_ASIENTO;
    tiempoSubfase = AleatorioAsiento(MUSICA_MINIMA_ASIENTO, MUSICA_MAXIMA_ASIENTO);
    tiempoAnimacion = 0.0f;
    anguloCarrusel = 0.0f;
    velocidadCarrusel = 1.0f;

    ReiniciarVisualAsiento(*this);
}


void MinijuegoUltimoAsiento::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteUltimoAsientoRetro3D());
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    AsignarColoresAsiento(*this, participantes);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_ULTIMO_ASIENTO_TERMINADO;
        return;
    }

    int limite = LimiteAsiento(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        float angulo = (float)k * 2.0f * PI / (float)cantidad + 0.5f;
        Vector3 spawn =
        {
            std::cos(angulo) * 6.0f,
            SUELO_ASIENTO + 0.72f,
            std::sin(angulo) * 6.0f
        };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = { -std::cos(angulo), 0.0f, -std::sin(angulo) };
        vivo[i] = true;
    }

    IniciarMusicaAsiento(*this);
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoUltimoAsiento::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    // El carrusel gira rapido con musica y se frena cuando se apagan las luces.
    float objetivoCarrusel =
        (fase == FASE_ULTIMO_ASIENTO_JUGANDO && subfase != SUBFASE_ULTIMO_ASIENTO_MUSICA)
        ? 0.0f
        : 1.0f;
    velocidadCarrusel += (objetivoCarrusel - velocidadCarrusel) * LimitarAsiento(deltaTime * 4.0f, 0.0f, 1.0f);
    anguloCarrusel += velocidadCarrusel * 55.0f * deltaTime;

    if (anguloCarrusel > 360.0f)
    {
        anguloCarrusel -= 360.0f;
    }

    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_ULTIMO_ASIENTO, deltaTime);
    ActualizarVisualAsiento(*this, deltaTime, jugadores, cantidadMaxima);

    for (int c = 0; c < MAX_TAZAS_ULTIMO_ASIENTO; c++)
    {
        if (tazas[c].destello > 0.0f)
        {
            tazas[c].destello -= deltaTime * 3.0f;

            if (tazas[c].destello < 0.0f)
            {
                tazas[c].destello = 0.0f;
            }
        }
    }

    if (
        fase == FASE_ULTIMO_ASIENTO_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_ULTIMO_ASIENTO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_ULTIMO_ASIENTO_JUGANDO;
        }

        return;
    }

    int limite = LimiteAsiento(cantidadMaxima);

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    // Los eliminados e inactivos no participan en las funciones compartidas.
    Participante efectivos[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectivos[i] = participantes[i];
        efectivos[i].activo = participantes[i].activo && vivo[i];
        efectivos[i].conectado = true;
    }

    // 1. Movimiento y fisica estandar.
    for (int i = 0; i < limite; i++)
    {
        if (!vivo[i])
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotAsiento(*this, i, jugador, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            bloques,
            cantidadBloques,
            particulas,
            MAX_PARTICULAS_ULTIMO_ASIENTO,
            true,
            true,
            deltaTime
        );

        // Limite circular de la arena y exclusion del carrusel central.
        float radio = std::sqrt(
            jugador.posicion.x * jugador.posicion.x +
            jugador.posicion.z * jugador.posicion.z
        );

        if (radio > RADIO_LIMITE_ASIENTO)
        {
            float escala = RADIO_LIMITE_ASIENTO / radio;
            jugador.posicion.x *= escala;
            jugador.posicion.z *= escala;
        }
        else if (radio < RADIO_CARRUSEL_FISICO_ASIENTO)
        {
            if (radio < 0.01f)
            {
                jugador.posicion.x = RADIO_CARRUSEL_FISICO_ASIENTO;
                jugador.posicion.z = 0.0f;
            }
            else
            {
                float escala = RADIO_CARRUSEL_FISICO_ASIENTO / radio;
                jugador.posicion.x *= escala;
                jugador.posicion.z *= escala;
            }
        }

        if (jugador.cayendo || jugador.posicion.y < -3.0f)
        {
            ReiniciarJugadorPrueba(jugador);
        }
    }

    // 2. Golpes, ground pound y colisiones entre jugadores vivos.
    float ralentizacionAntes[MAX_PARTICIPANTES]{};
    bool huboPound = false;

    for (int i = 0; i < limite; i++)
    {
        ralentizacionAntes[i] = jugadores[i].tiempoRalentizado;

        if (vivo[i] && jugadores[i].impactoGolpeSuelo)
        {
            huboPound = true;
        }
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        efectivos,
        limite,
        particulas,
        MAX_PARTICULAS_ULTIMO_ASIENTO
    );

    if (huboPound)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_GROUND_POUND);
    }

    for (int i = 0; i < limite; i++)
    {
        if (vivo[i] && jugadores[i].tiempoRalentizado > ralentizacionAntes[i] + 0.2f)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_GOLPE);
            break;
        }
    }

    // 3. Ocupacion de tazas.
    if (subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
    {
        ActualizarOcupacionAsiento(*this, jugadores, limite);

        if (
            indiceTrampa >= 0 &&
            !avisoTrampaDado &&
            tiempoSubfase <= DURACION_AVISO_TRAMPA_ASIENTO
        )
        {
            avisoTrampaDado = true;
            ReproducirSonidoMinijuego(audio, SONIDO_ERROR);
        }
    }

    // 4. Transiciones de subfase.
    tiempoSubfase -= deltaTime;

    if (tiempoSubfase <= 0.0f)
    {
        if (subfase == SUBFASE_ULTIMO_ASIENTO_MUSICA)
        {
            IniciarTazasAsiento(*this, limite);
        }
        else if (subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
        {
            ResolverRondaAsiento(*this, jugadores, limite);

            if (ContarVivosAsiento(*this, limite) <= 1)
            {
                FinalizarPartidaAsiento(*this, limite);
                return;
            }
        }
        else
        {
            IniciarMusicaAsiento(*this);
        }
    }

    // 5. Tiempo limite duro: empatan los supervivientes.
    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarPartidaAsiento(*this, limite);
    }
}



//==================================================
// VISUAL: FERIA DE DIA (solo decoracion, la logica no depende de esto)
//==================================================
//
// La plaza del carrusel es el disco jugable (radio 8.5): baldosas en
// abanico, bordillo a rayas y valla baja que coinciden con el limite real.
// Alrededor, la explanada de cesped de la feria con gradas, faroles con
// guirnaldas, puestos, carritos y bancos; mas atras la carpa de circo, la
// noria y la fila de arboles donde termina la explanada. Detras, pintado,
// el valle con colinas, un pueblo y el cielo de mediodia con nubes.

static const float Z_BORDE_ASI = -14.5f;          // fin de la explanada
static const Vector3 OJO_ASI = { 0.0f, 15.5f, 15.0f };
static const float ALTO_PLATAFORMA_ASI = 0.45f;
static const float ALTO_ALERO_ASI = 2.75f;
static const float ALTO_CUSPIDE_ASI = 3.6f;
static const float RADIO_TOLDO_ASI = 2.64f;
static const float RADIO_CABALLITOS_ASI = 1.78f;
static const Vector3 NORIA_ASI = { 14.6f, 6.4f, -13.6f };
static const Vector3 CARPA_ASI = { -14.8f, 0.0f, -12.4f };
static const Vector3 MARTILLO_ASI = { -12.6f, 0.0f, -7.0f };
static const Vector3 TAQUILLA_ASI = { 12.6f, 0.0f, -6.9f };
static const Vector3 CARRITO_GLOBOS_ASI = { 12.4f, 0.0f, 6.0f };
static const Vector3 CARRITO_HELADOS_ASI = { -12.4f, 0.0f, 6.0f };
static const float RADIO_FAROLES_ASI = 9.75f;
static const int CANTIDAD_FAROLES_ASI = 6;
static const float ANGULOS_FAROLES_ASI[CANTIDAD_FAROLES_ASI] = { 200.0f, 228.0f, 256.0f, 284.0f, 312.0f, 340.0f };
static const float X_PUESTOS_ASI[4] = { -7.1f, -3.5f, 3.5f, 7.1f };
static const float Z_PUESTOS_ASI = -11.3f;

static const Color CIELO_ALTO_ASI = { 74, 150, 224, 255 };
static const Color CIELO_MEDIO_ASI = { 128, 190, 238, 255 };
static const Color CIELO_BAJO_ASI = { 208, 232, 244, 255 };
static const Color NIEBLA_ASI = { 196, 224, 238, 255 };
static const Color PASTO_ASI = { 108, 168, 82, 255 };
static const Color PASTO_CLARO_ASI = { 140, 192, 94, 255 };
static const Color GRAVA_ASI = { 220, 202, 164, 255 };
static const Color LOSA_A_ASI = { 218, 194, 152, 255 };
static const Color LOSA_B_ASI = { 200, 148, 124, 255 };
static const Color LOSA_BORDE_ASI = { 236, 222, 190, 255 };
static const Color JUNTA_ASI = { 124, 98, 80, 255 };
static const Color CORAL_ASI = { 234, 100, 112, 255 };
static const Color CORAL_OSCURO_ASI = { 176, 62, 82, 255 };
static const Color CREMA_ASI = { 250, 238, 214, 255 };
static const Color MENTA_ASI = { 112, 200, 180, 255 };
static const Color TURQUESA_ASI = { 50, 148, 156, 255 };
static const Color ORO_ASI = { 230, 184, 86, 255 };
static const Color ORO_OSCURO_ASI = { 160, 118, 52, 255 };
static const Color MADERA_ASI = { 172, 120, 80, 255 };
static const Color MADERA_OSCURA_ASI = { 116, 80, 56, 255 };
static const Color HIERRO_ASI = { 50, 90, 82, 255 };
static const Color LILA_ASI = { 178, 148, 216, 255 };
static const Color ROSA_ASI = { 248, 172, 198, 255 };
static const Color LIMON_ASI = { 250, 222, 110, 255 };
static const Color HOJA_ASI = { 74, 142, 74, 255 };
static const Color HOJA_OSCURA_ASI = { 50, 106, 62, 255 };
static const Color TRONCO_ASI = { 110, 80, 58, 255 };
static const Color ACENTO_ASI = { 255, 204, 70, 255 };       // tazas libres
static const Color TRAMPA_ASI = { 236, 46, 46, 255 };


//==================================================
// VISUAL: UTILIDADES
//==================================================

static float Limitar01Asi(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalAsi(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoAsi(Color c, float f)
{
    return Color{ CanalAsi((float)c.r * f), CanalAsi((float)c.g * f), CanalAsi((float)c.b * f), c.a };
}


static Color MezclaAsi(Color a, Color b, float t)
{
    t = Limitar01Asi(t);
    return Color{
        CanalAsi((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalAsi((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalAsi((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalAsi((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaAsi(Color c, float alfa)
{
    c.a = CanalAsi(alfa);
    return c;
}


static float SuaveAsi(float u)
{
    u = Limitar01Asi(u);
    return u * u * (3.0f - 2.0f * u);
}


// Hash determinista en [0, 1) para decorar sin parpadeos.
static float Hash01Asi(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 2654435761u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFFu) / 16777216.0f;
}


// Azar propio del dibujo: nunca consume el GetRandomValue de la logica.
static float AzarAsi(MinijuegoUltimoAsiento& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFFFu) / 16777216.0f;
    return minimo + (maximo - minimo) * u;
}


// Mediodia: lo lejano se aclara hacia la calima celeste del valle.
static Color TinteAsi(Color c, Vector3 p)
{
    float dx = p.x - OJO_ASI.x;
    float dy = p.y - OJO_ASI.y;
    float dz = p.z - OJO_ASI.z;
    float n = (std::sqrt(dx * dx + dy * dy + dz * dz) - 24.0f) / 34.0f;
    Color niebla = NIEBLA_ASI;
    niebla.a = c.a;
    return MezclaAsi(c, niebla, n < 0.0f ? 0.0f : (n > 0.42f ? 0.42f : n));
}


// Sol alto, algo a la derecha y de frente.
static float LuzNormalAsi(Vector3 n)
{
    float d = n.x * 0.30f + n.y * 0.82f + n.z * 0.48f;
    float f = 0.66f + 0.34f * d;
    return f < 0.50f ? 0.50f : f;
}


static float LuzLateralAsi(float angulo)
{
    return LuzNormalAsi({ std::cos(angulo), 0.0f, std::sin(angulo) });
}


static void VerticeAsi(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadAsi(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeAsi(a, color);
    VerticeAsi(b, color);
    VerticeAsi(c, color);
    VerticeAsi(d, color);
}


// Caja sobre tres ejes (medios lados ya escalados), caras sombreadas.
static void CajaEjesAsi(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color)
{
    Vector3 v[8];

    for (int k = 0; k < 8; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sy = (k & 2) ? 1.0f : -1.0f;
        float sz = (k & 4) ? 1.0f : -1.0f;
        v[k] =
        {
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    }

    rlBegin(RL_QUADS);
    QuadAsi(v[6], v[7], v[3], v[2], TonoAsi(color, 1.0f));     // arriba
    QuadAsi(v[4], v[5], v[7], v[6], TonoAsi(color, 0.78f));    // frente
    QuadAsi(v[1], v[0], v[2], v[3], TonoAsi(color, 0.56f));    // fondo
    QuadAsi(v[5], v[1], v[3], v[7], TonoAsi(color, 0.62f));    // derecha
    QuadAsi(v[0], v[4], v[6], v[2], TonoAsi(color, 0.70f));    // izquierda
    QuadAsi(v[0], v[1], v[5], v[4], TonoAsi(color, 0.46f));    // abajo
    rlEnd();
}


static void CuboAsi(Vector3 c, Vector3 t, Color color)
{
    CajaEjesAsi(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color);
}


// Caja girada sobre Y (giro en radianes, mismo sentido que cos/sin del juego).
static void CuboGiradoAsi(Vector3 c, Vector3 t, float giro, Color color)
{
    float co = std::cos(giro);
    float si = std::sin(giro);

    CajaEjesAsi(
        c,
        { co * t.x * 0.5f, 0.0f, si * t.x * 0.5f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { -si * t.z * 0.5f, 0.0f, co * t.z * 0.5f },
        color
    );
}


static void BaseEjeAsi(Vector3 dir, Vector3& ex, Vector3& ey)
{
    Vector3 ayuda = std::fabs(dir.y) > 0.9f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    ex = Vector3Normalize(Vector3CrossProduct(ayuda, dir));
    ey = Vector3CrossProduct(dir, ex);
}


// Tubo (tronco de cono) entre dos puntos con caras sombreadas por normal.
static void TuboAsi(Vector3 a, Vector3 b, float r0, float r1, Color color, int lados = 8, bool tapa = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 ex, ey;
    BaseEjeAsi(dir, ex, ey);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        float am = (a0 + a1) * 0.5f;
        Vector3 n0 = Vector3Add(Vector3Scale(ex, std::cos(a0)), Vector3Scale(ey, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(ex, std::cos(a1)), Vector3Scale(ey, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(ex, std::cos(am)), Vector3Scale(ey, std::sin(am)));

        QuadAsi(
            Vector3Add(a, Vector3Scale(n0, r0)),
            Vector3Add(b, Vector3Scale(n0, r1)),
            Vector3Add(b, Vector3Scale(n1, r1)),
            Vector3Add(a, Vector3Scale(n1, r0)),
            TonoAsi(color, LuzNormalAsi(nm))
        );

        if (tapa)
        {
            Color c = TonoAsi(color, LuzNormalAsi(dir));
            QuadAsi(b, Vector3Add(b, Vector3Scale(n1, r1)), Vector3Add(b, Vector3Scale(n0, r1)), b, c);
        }
    }

    rlEnd();
}


static void VigaAsi(Vector3 a, Vector3 b, float grosor, Color color)
{
    TuboAsi(a, b, grosor * 0.5f, grosor * 0.5f, color, 4, false);
}


// Tronco de cono vertical (r0 abajo, r1 arriba) con tapa arriba.
static void PrismaAsi(Vector3 base, float r0, float r1, float alto, int lados, Color color, bool tapa = true, float giro = 0.0f)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        Color cara = TonoAsi(color, LuzLateralAsi(a0 + paso * 0.5f));
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadAsi(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            cara
        );

        if (tapa && r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadAsi(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, color);
        }
    }

    rlEnd();
}


// Prisma con caras de dos colores alternados (lonas a rayas, espejos).
static void PrismaRayasAsi(Vector3 base, float r0, float r1, float alto, int lados, Color a, Color b, float giro = 0.0f)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        Color cara = TonoAsi((k % 2 == 0) ? a : b, LuzLateralAsi(a0 + paso * 0.5f));
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadAsi(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            cara
        );
    }

    rlEnd();
}


// Cono de lona a gajos (toldo del carrusel, carpa, sombrillas).
static void ConoRayasAsi(Vector3 base, float radio, float alto, int gajos, Color a, Color b, float giro = 0.0f, float radioArriba = 0.0f)
{
    float paso = 6.2832f / (float)gajos;
    float inclinacion = (radio - radioArriba) / (alto > 0.01f ? alto : 0.01f);
    Vector3 cima = { base.x, base.y + alto, base.z };

    rlBegin(RL_QUADS);

    for (int k = 0; k < gajos; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 n = Vector3Normalize({ std::cos(am), inclinacion, std::sin(am) });
        Color cara = TonoAsi((k % 2 == 0) ? a : b, LuzNormalAsi(n));

        QuadAsi(
            { base.x + std::cos(a0) * radio, base.y, base.z + std::sin(a0) * radio },
            { cima.x + std::cos(a0) * radioArriba, cima.y, cima.z + std::sin(a0) * radioArriba },
            { cima.x + std::cos(a1) * radioArriba, cima.y, cima.z + std::sin(a1) * radioArriba },
            { base.x + std::cos(a1) * radio, base.y, base.z + std::sin(a1) * radio },
            cara
        );
    }

    rlEnd();
}


// Faldon de picos bajo un alero circular (banderolas del toldo).
static void FaldonAsi(Vector3 c, float radio, float y, float alto, int picos, Color a, Color b, float giro = 0.0f)
{
    float paso = 6.2832f / (float)picos;

    rlBegin(RL_QUADS);

    for (int k = 0; k < picos; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Color cara = TonoAsi((k % 2 == 0) ? a : b, LuzLateralAsi(am));
        Vector3 p0 = { c.x + std::cos(a0) * radio, y, c.z + std::sin(a0) * radio };
        Vector3 p1 = { c.x + std::cos(a1) * radio, y, c.z + std::sin(a1) * radio };
        Vector3 punta = { c.x + std::cos(am) * radio, y - alto, c.z + std::sin(am) * radio };
        Vector3 m0 = { p0.x, y - alto * 0.55f, p0.z };
        Vector3 m1 = { p1.x, y - alto * 0.55f, p1.z };

        QuadAsi(p0, m0, m1, p1, cara);
        QuadAsi(m0, punta, punta, m1, cara);
    }

    rlEnd();
}


// Corona plana (r0..r1) entre dos angulos, centrada en (cx, cz).
static void ArcoSueloAsi(float cx, float cz, float r0, float r1, float a0, float a1, float y, Color color, int pasos)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < pasos; k++)
    {
        float b0 = a0 + (a1 - a0) * (float)k / (float)pasos;
        float b1 = a0 + (a1 - a0) * (float)(k + 1) / (float)pasos;

        QuadAsi(
            { cx + std::cos(b0) * r1, y, cz + std::sin(b0) * r1 },
            { cx + std::cos(b0) * r0, y, cz + std::sin(b0) * r0 },
            { cx + std::cos(b1) * r0, y, cz + std::sin(b1) * r0 },
            { cx + std::cos(b1) * r1, y, cz + std::sin(b1) * r1 },
            color
        );
    }

    rlEnd();
}


static void AnilloAsi(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    ArcoSueloAsi(c.x, c.z, r0, r1, 0.0f, 6.2832f, c.y, color, lados);
}


// Casquete esferico sobre c (copas, globos, campana).
static void DomoAsi(Vector3 c, float rx, float ry, float rz, Color color, int lados = 12, int anillos = 4)
{
    rlBegin(RL_QUADS);

    for (int a = 0; a < anillos; a++)
    {
        float f0 = 1.5708f * (float)a / (float)anillos;
        float f1 = 1.5708f * (float)(a + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float t0 = 6.2832f * (float)k / (float)lados;
            float t1 = 6.2832f * (float)(k + 1) / (float)lados;
            float fm = (f0 + f1) * 0.5f;
            float tm = (t0 + t1) * 0.5f;
            float sy = ry < 0.0f ? -1.0f : 1.0f;
            Color cara = TonoAsi(color, LuzNormalAsi({ std::cos(fm) * std::cos(tm), std::sin(fm) * sy, std::cos(fm) * std::sin(tm) }));

            QuadAsi(
                { c.x + rx * std::cos(f0) * std::cos(t0), c.y + ry * std::sin(f0), c.z + rz * std::cos(f0) * std::sin(t0) },
                { c.x + rx * std::cos(f1) * std::cos(t0), c.y + ry * std::sin(f1), c.z + rz * std::cos(f1) * std::sin(t0) },
                { c.x + rx * std::cos(f1) * std::cos(t1), c.y + ry * std::sin(f1), c.z + rz * std::cos(f1) * std::sin(t1) },
                { c.x + rx * std::cos(f0) * std::cos(t1), c.y + ry * std::sin(f0), c.z + rz * std::cos(f0) * std::sin(t1) },
                cara
            );
        }
    }

    rlEnd();
}


static void EsferaAsi(Vector3 c, float r, Color color, int lados = 10, int anillos = 3)
{
    DomoAsi(c, r, r, r, color, lados, anillos);
    DomoAsi(c, r, -r, r, TonoAsi(color, 0.86f), lados, anillos);
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorAsi(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 18;
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


static void ResplandorSueloAsi(Vector3 c, float radio, Color color)
{
    ResplandorAsi(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Halo que mira a la camara (la vista baja a unos 45 grados).
static void ResplandorFrenteAsi(Vector3 c, float radio, Color color)
{
    ResplandorAsi(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.70f, 0.71f }, radio, color);
}


// Columna de luz que se apaga hacia arriba (tazas libres y trampa).
static void ColumnaLuzAsi(Vector3 base, float radio, float alto, Color color)
{
    const int LADOS = 10;
    rlBegin(RL_QUADS);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = { base.x + std::cos(a0) * radio, base.y, base.z + std::sin(a0) * radio };
        Vector3 p1 = { base.x + std::cos(a1) * radio, base.y, base.z + std::sin(a1) * radio };
        Vector3 q0 = { base.x + std::cos(a0) * radio * 0.8f, base.y + alto, base.z + std::sin(a0) * radio * 0.8f };
        Vector3 q1 = { base.x + std::cos(a1) * radio * 0.8f, base.y + alto, base.z + std::sin(a1) * radio * 0.8f };

        VerticeAsi(p0, color);
        VerticeAsi(q0, AlfaAsi(color, 0.0f));
        VerticeAsi(q1, AlfaAsi(color, 0.0f));
        VerticeAsi(p1, color);
    }

    rlEnd();
}


static int CantidadParticulasAsi(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


// Rebote de aparicion (sube, se pasa un poco y asienta).
static float EscalaAparicionAsi(float u)
{
    if (u <= 0.0f) return 0.0f;
    if (u >= 1.0f) return 1.0f;
    float v = u - 1.0f;
    return 1.0f + 2.70158f * v * v * v + 1.70158f * v * v;
}


//==================================================
// VISUAL: ESTADO Y EFECTOS (una vez por frame desde Actualizar)
//==================================================

static void EmitirParticulaAsi(
    MinijuegoUltimoAsiento& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool humo
)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_ASIENTO; i++)
    {
        ParticulaVisualAsiento& p = m.particulasVisuales[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.humo = humo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.color = color;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.giro = AzarAsi(m, 0.0f, 6.2832f);
        return;
    }
}


// Rocio de confeti en todas direcciones (hacia arriba).
static void EstallidoAsi(
    MinijuegoUltimoAsiento& m,
    Vector3 centro,
    int cantidad,
    Color a,
    Color b,
    float fuerza
)
{
    int n = CantidadParticulasAsi(cantidad);

    for (int k = 0; k < n; k++)
    {
        float ang = AzarAsi(m, 0.0f, 6.2832f);
        float f = AzarAsi(m, 0.4f, 1.0f) * fuerza;
        Vector3 v = { std::cos(ang) * f, AzarAsi(m, 0.8f, 1.5f) * fuerza, std::sin(ang) * f };
        EmitirParticulaAsi(m, centro, v, (k % 2 == 0) ? a : b, AzarAsi(m, 0.8f, 1.3f), AzarAsi(m, 0.07f, 0.12f), 9.0f, false);
    }
}


static void HumoAsi(MinijuegoUltimoAsiento& m, Vector3 centro, int cantidad, Color color)
{
    int n = CantidadParticulasAsi(cantidad);

    for (int k = 0; k < n; k++)
    {
        float ang = AzarAsi(m, 0.0f, 6.2832f);
        float f = AzarAsi(m, 0.6f, 1.6f);
        Vector3 p = { centro.x + std::cos(ang) * 0.25f, centro.y + AzarAsi(m, 0.0f, 0.6f), centro.z + std::sin(ang) * 0.25f };
        EmitirParticulaAsi(m, p, { std::cos(ang) * f, AzarAsi(m, 0.6f, 1.4f), std::sin(ang) * f }, color, AzarAsi(m, 0.6f, 0.9f), AzarAsi(m, 0.22f, 0.34f), -0.6f, true);
    }
}


static void ReiniciarVisualAsiento(MinijuegoUltimoAsiento& m)
{
    m.semillaVisual = 2654435761u;
    m.subfaseVista = SUBFASE_ULTIMO_ASIENTO_MUSICA;
    m.rondaVista = 0;
    m.tiempoEnSubfase = 0.0f;
    m.tiempoFin = 0.0f;
    m.finVisto = false;

    for (int c = 0; c < MAX_TAZAS_ULTIMO_ASIENTO; c++)
    {
        m.ocupanteVisto[c] = -1;
        m.rebote[c] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.vivoVisto[i] = false;
        m.posicionVista[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_VISUALES_ASIENTO; i++)
    {
        m.particulasVisuales[i] = {};
    }
}


static void ActualizarVisualAsiento(
    MinijuegoUltimoAsiento& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    int limite = LimiteAsiento(cantidadMaxima);

    // Particulas: el confeti cae y se posa; el humo sube y se abre.
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_ASIENTO; i++)
    {
        ParticulaVisualAsiento& p = m.particulasVisuales[i];

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

        if (p.humo)
        {
            float freno = 1.0f - LimitarAsiento(deltaTime * 2.5f, 0.0f, 1.0f);
            p.velocidad.x *= freno;
            p.velocidad.z *= freno;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;
        p.giro += deltaTime * 6.0f;

        if (!p.humo && p.posicion.y < 0.0f)
        {
            p.posicion.y = 0.0f;
            p.velocidad = {};
            p.gravedad = 0.0f;
        }
    }

    if (m.subfase != m.subfaseVista)
    {
        m.subfaseVista = m.subfase;
        m.tiempoEnSubfase = 0.0f;
    }
    else
    {
        m.tiempoEnSubfase += deltaTime;
    }

    // Taza ocupada: confeti del color del jugador y un salto de la taza.
    for (int c = 0; c < MAX_TAZAS_ULTIMO_ASIENTO; c++)
    {
        m.rebote[c] -= deltaTime * 2.5f;

        if (m.rebote[c] < 0.0f)
        {
            m.rebote[c] = 0.0f;
        }

        const TazaUltimoAsiento& taza = m.tazas[c];
        int ocupante = (c < m.cantidadTazas && taza.activa && m.subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
            ? taza.ocupante
            : -1;

        if (ocupante >= 0 && ocupante != m.ocupanteVisto[c] && ocupante < MAX_PARTICIPANTES)
        {
            m.rebote[c] = 1.0f;
            EstallidoAsi(m, { taza.posicion.x, 0.9f, taza.posicion.z }, 16, m.coloresJugadores[ocupante], ACENTO_ASI, 3.2f);
        }

        if (m.subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
        {
            m.ocupanteVisto[c] = ocupante;
        }
        else if (m.subfase == SUBFASE_ULTIMO_ASIENTO_MUSICA)
        {
            m.ocupanteVisto[c] = -1;
        }
    }

    // Eliminados: bocanada gris donde estaban (y chispas rojas si la
    // taza era trampa) y confeti chico al llegar a la grada.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        if (m.vivoVisto[i] && !m.vivo[i])
        {
            Vector3 antes = m.posicionVista[i];
            HumoAsi(m, { antes.x, 0.2f, antes.z }, 10, Color{ 236, 232, 226, 255 });

            for (int c = 0; c < m.cantidadTazas; c++)
            {
                if (m.tazas[c].trampa && m.tazas[c].ocupante == i)
                {
                    EstallidoAsi(m, { m.tazas[c].posicion.x, 0.8f, m.tazas[c].posicion.z }, 18, TRAMPA_ASI, Color{ 255, 170, 60, 255 }, 4.2f);
                }
            }

            Vector3 grada = jugadores[i].posicion;
            EstallidoAsi(m, { grada.x, grada.y + 0.6f, grada.z }, 8, Color{ 200, 200, 210, 255 }, m.coloresJugadores[i], 1.8f);
        }

        if (m.vivo[i])
        {
            m.posicionVista[i] = jugadores[i].posicion;
        }

        m.vivoVisto[i] = m.vivo[i];
    }

    // Fin: confeti sobre quien gano.
    if (m.fase == FASE_ULTIMO_ASIENTO_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (!m.finVisto)
        {
            m.finVisto = true;
            m.tiempoFin = 0.0f;
        }

        m.tiempoFin += deltaTime;

        if (m.tiempoFin < 3.0f)
        {
            const Color confeti[5] = { ACENTO_ASI, CORAL_ASI, MENTA_ASI, LILA_ASI, CREMA_ASI };

            for (int i = 0; i < limite; i++)
            {
                if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1)
                {
                    continue;
                }

                int n = CalidadParticulas() == CALIDAD_BAJA ? 1 : 2;

                for (int k = 0; k < n; k++)
                {
                    Vector3 p =
                    {
                        jugadores[i].posicion.x + AzarAsi(m, -0.9f, 0.9f),
                        jugadores[i].posicion.y + 2.4f + AzarAsi(m, 0.0f, 0.8f),
                        jugadores[i].posicion.z + AzarAsi(m, -0.9f, 0.9f)
                    };
                    int c = (int)AzarAsi(m, 0.0f, 4.99f);
                    Color color = (k == 1) ? m.coloresJugadores[i] : confeti[c];
                    EmitirParticulaAsi(m, p, { AzarAsi(m, -1.2f, 1.2f), AzarAsi(m, 0.6f, 2.4f), AzarAsi(m, -1.2f, 1.2f) }, color, AzarAsi(m, 1.4f, 2.2f), 0.09f, 3.0f, false);
                }
            }
        }
    }
}


//==================================================
// VISUAL: CIELO, VALLE Y COLINAS (2D, antes del 3D)
//==================================================

// Silueta suave de una fila de colinas (en pixeles sobre su base).
static float AltoColinaAsi(float x, float w, int capa, float alto)
{
    float u = x / w;
    float y =
        0.55f + 0.25f * std::sin(u * 7.1f + (float)capa * 1.7f) +
        0.14f * std::sin(u * 17.3f + (float)capa * 0.6f) +
        0.06f * std::sin(u * 41.0f + (float)capa * 2.3f);
    return y * alto;
}


static void NubeAsi(float x, float y, float escala, float alfa)
{
    Color sombra = Fade(Color{ 206, 222, 240, 255 }, alfa);
    Color luz = Fade(Color{ 255, 255, 255, 255 }, alfa);
    const float bultos[5][3] =
    {
        { -38.0f, 2.0f, 20.0f }, { -14.0f, -8.0f, 27.0f }, { 14.0f, -12.0f, 31.0f },
        { 40.0f, -2.0f, 22.0f }, { 58.0f, 4.0f, 15.0f }
    };

    for (int k = 0; k < 5; k++)
    {
        DrawCircleV({ x + bultos[k][0] * escala, y + bultos[k][1] * escala + 4.0f * escala }, bultos[k][2] * escala, sombra);
    }

    for (int k = 0; k < 5; k++)
    {
        DrawCircleV({ x + bultos[k][0] * escala - 2.0f * escala, y + bultos[k][1] * escala }, bultos[k][2] * escala * 0.92f, luz);
    }

    DrawRectangle((int)(x - 52.0f * escala), (int)(y + 2.0f * escala), (int)(116.0f * escala), (int)(16.0f * escala), sombra);
}


static void DibujarCieloAsi(const Camera3D& vista, float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float w = (float)ancho;
    float h = (float)alto;
    float e = h / 720.0f;

    // Donde termina la explanada en pantalla: ahi empieza el valle pintado.
    float yH = GetWorldToScreen({ 0.0f, 0.0f, Z_BORDE_ASI }, vista).y;
    yH = yH < 40.0f * e ? 40.0f * e : (yH > h * 0.6f ? h * 0.6f : yH);

    int y1 = (int)(yH * 0.5f);
    DrawRectangleGradientV(0, 0, ancho, y1 + 1, CIELO_ALTO_ASI, CIELO_MEDIO_ASI);
    DrawRectangleGradientV(0, y1, ancho, (int)yH - y1 + 2, CIELO_MEDIO_ASI, CIELO_BAJO_ASI);
    DrawRectangle(0, (int)yH, ancho, alto - (int)yH, TinteAsi(PASTO_ASI, { 0.0f, 0.0f, -40.0f }));

    // Sol de mediodia arriba a la derecha.
    Vector2 sol = { w * 0.74f, yH * 0.18f };
    DrawCircleGradient(sol, 120.0f * e, Fade(Color{ 255, 250, 220, 255 }, 0.45f), Fade(Color{ 255, 250, 220, 255 }, 0.0f));
    DrawCircleV(sol, 30.0f * e, Color{ 255, 248, 214, 255 });
    DrawCircleV(sol, 24.0f * e, Color{ 255, 253, 240, 255 });

    // Nubes que cruzan despacio.
    int nubes = detalle ? 7 : 4;

    for (int k = 0; k < nubes; k++)
    {
        float vel = (5.0f + 4.0f * Hash01Asi(k, 31)) * e;
        float tramo = w + 300.0f * e;
        float x = std::fmod(Hash01Asi(k, 32) * tramo + t * vel, tramo) - 150.0f * e;
        float y = yH * (0.14f + 0.5f * Hash01Asi(k, 33));
        NubeAsi(x, y, (0.55f + 0.5f * Hash01Asi(k, 34)) * e, 0.92f);
    }

    // Globo aerostatico a lo lejos.
    if (detalle)
    {
        float tramo = w + 160.0f * e;
        float x = std::fmod(0.3f * tramo + t * 7.0f * e, tramo) - 80.0f * e;
        float y = yH * 0.42f + std::sin(t * 0.6f) * 5.0f * e;
        float r = 15.0f * e;

        for (int k = 0; k < 6; k++)
        {
            Color gajo = (k % 2 == 0) ? Color{ 236, 128, 120, 255 } : Color{ 250, 226, 150, 255 };
            float x0 = x - r + 2.0f * r * (float)k / 6.0f;
            DrawRectangle((int)x0, (int)(y - r * 0.9f), (int)(2.0f * r / 6.0f) + 1, (int)(r * 1.6f), MezclaAsi(gajo, CIELO_MEDIO_ASI, 0.35f));
        }

        DrawCircleSector({ x, y - r * 0.9f }, r, 180.0f, 360.0f, 16, MezclaAsi(Color{ 236, 128, 120, 255 }, CIELO_MEDIO_ASI, 0.35f));
        DrawTriangle({ x - r, y + r * 0.7f }, { x, y + r * 1.25f }, { x + r, y + r * 0.7f }, MezclaAsi(Color{ 250, 226, 150, 255 }, CIELO_MEDIO_ASI, 0.35f));
        DrawLineV({ x - r * 0.3f, y + r * 1.15f }, { x - r * 0.25f, y + r * 1.55f }, Fade(Color{ 90, 70, 60, 255 }, 0.6f));
        DrawLineV({ x + r * 0.3f, y + r * 1.15f }, { x + r * 0.25f, y + r * 1.55f }, Fade(Color{ 90, 70, 60, 255 }, 0.6f));
        DrawRectangle((int)(x - r * 0.3f), (int)(y + r * 1.5f), (int)(r * 0.6f), (int)(r * 0.4f), MezclaAsi(Color{ 140, 98, 70, 255 }, CIELO_MEDIO_ASI, 0.3f));
    }

    // Colinas lejanas (celeste) con un pueblo, y colinas cercanas (verde).
    const float PASO = 4.0f;
    float altoLejos = 46.0f * e;
    float altoCerca = 26.0f * e;
    Color lejos = Color{ 156, 194, 210, 255 };
    Color lejosBase = Color{ 178, 208, 220, 255 };

    for (float x = 0.0f; x < w; x += PASO)
    {
        float a = AltoColinaAsi(x, w, 0, altoLejos);
        DrawRectangleGradientV((int)x, (int)(yH - 8.0f * e - a), (int)PASO + 1, (int)(a + 10.0f * e), lejos, lejosBase);
    }

    if (detalle)
    {
        // Pueblo sobre una loma: casas, tejados y el campanario.
        float x0 = w * 0.30f;

        for (int k = 0; k < 14; k++)
        {
            float x = x0 + (float)k * 11.0f * e + 3.0f * Hash01Asi(k, 41) * e;
            float base = yH - 8.0f * e - AltoColinaAsi(x, w, 0, altoLejos) + 6.0f * e;
            float ac = (7.0f + 6.0f * Hash01Asi(k, 42)) * e;
            Color pared = MezclaAsi(Color{ 236, 226, 206, 255 }, lejos, 0.45f);
            Color teja = MezclaAsi(Color{ 196, 104, 82, 255 }, lejos, 0.45f);
            DrawRectangle((int)x, (int)(base - ac), (int)(9.0f * e), (int)ac + 2, pared);
            DrawTriangle({ x - 1.0f * e, base - ac }, { x + 4.5f * e, base - ac - 5.0f * e }, { x + 10.0f * e, base - ac }, teja);

            if (k == 6)
            {
                DrawRectangle((int)(x + 2.0f * e), (int)(base - ac - 18.0f * e), (int)(5.0f * e), (int)(18.0f * e), pared);
                DrawTriangle({ x + 1.0f * e, base - ac - 18.0f * e }, { x + 4.5f * e, base - ac - 28.0f * e }, { x + 8.0f * e, base - ac - 18.0f * e }, teja);
            }
        }
    }

    Color cerca = Color{ 120, 170, 128, 255 };
    Color cercaBase = Color{ 112, 160, 106, 255 };

    for (float x = 0.0f; x < w; x += PASO)
    {
        float a = AltoColinaAsi(x, w, 3, altoCerca);
        DrawRectangleGradientV((int)x, (int)(yH - a), (int)PASO + 1, (int)(a + 4.0f * e), cerca, cercaBase);
    }

    // Bandada que cruza.
    if (detalle)
    {
        for (int k = 0; k < 5; k++)
        {
            float tramo = w + 200.0f * e;
            float x = std::fmod(Hash01Asi(k, 51) * 60.0f * e + t * 22.0f * e, tramo) - 100.0f * e + (float)k * 14.0f * e;
            float y = yH * 0.30f + (float)(k % 3) * 7.0f * e + std::sin(t * 2.0f + (float)k) * 2.0f * e;
            float aleteo = std::sin(t * 9.0f + (float)k * 1.3f) * 3.0f * e;
            Color ave = Color{ 70, 86, 110, 200 };
            DrawLineEx({ x - 6.0f * e, y - aleteo }, { x, y }, 2.0f * e, ave);
            DrawLineEx({ x, y }, { x + 6.0f * e, y - aleteo }, 2.0f * e, ave);
        }
    }
}


//==================================================
// VISUAL: EXPLANADA, PLAZA Y BORDE
//==================================================

static void DibujarTerrenoAsi(int paso)
{
    const float CELDA = 2.0f * (float)paso;
    const float X0 = -44.0f;
    const float X1 = 44.0f;
    const float Z1 = 22.0f;
    const float Y = -0.10f;

    rlBegin(RL_QUADS);

    for (float z = Z_BORDE_ASI; z < Z1; z += CELDA)
    {
        for (float x = X0; x < X1; x += CELDA)
        {
            // Lo que queda bajo la plaza no se ve.
            float cx = x + CELDA * 0.5f;
            float cz = z + CELDA * 0.5f;

            if (cx * cx + cz * cz < 7.0f * 7.0f)
            {
                continue;
            }

            int ix = (int)std::floor(x / CELDA);
            int iz = (int)std::floor(z / CELDA);
            float f = 0.93f + 0.12f * Hash01Asi(ix, iz);
            Color base = MezclaAsi(PASTO_ASI, PASTO_CLARO_ASI, 0.5f + 0.5f * std::sin(cx * 0.21f + cz * 0.13f) * std::cos(cz * 0.17f));
            Color c = TinteAsi(TonoAsi(base, f), { cx, 0.0f, cz });
            QuadAsi({ x, Y, z }, { x, Y, z + CELDA }, { x + CELDA, Y, z + CELDA }, { x + CELDA, Y, z }, c);
        }
    }

    rlEnd();

    // Paseo de grava alrededor de la plaza y hacia la feria.
    for (int k = 0; k < 48; k++)
    {
        float a0 = 6.2832f * (float)k / 48.0f;
        float a1 = 6.2832f * (float)(k + 1) / 48.0f;
        Color c = TonoAsi(GRAVA_ASI, 0.94f + 0.08f * Hash01Asi(k, 61));
        ArcoSueloAsi(0.0f, 0.0f, 8.9f, 10.9f, a0, a1, -0.085f, c, 1);
    }

    rlBegin(RL_QUADS);
    QuadAsi({ -1.3f, -0.085f, Z_BORDE_ASI }, { -1.3f, -0.085f, -10.6f }, { 1.3f, -0.085f, -10.6f }, { 1.3f, -0.085f, Z_BORDE_ASI }, TinteAsi(GRAVA_ASI, { 0.0f, 0.0f, -12.0f }));
    QuadAsi({ 10.6f, -0.085f, -1.6f }, { 10.6f, -0.085f, 1.6f }, { 44.0f, -0.085f, 1.6f }, { 44.0f, -0.085f, -1.6f }, TinteAsi(GRAVA_ASI, { 18.0f, 0.0f, 0.0f }));
    QuadAsi({ -44.0f, -0.085f, -1.6f }, { -44.0f, -0.085f, 1.6f }, { -10.6f, -0.085f, 1.6f }, { -10.6f, -0.085f, -1.6f }, TinteAsi(GRAVA_ASI, { -18.0f, 0.0f, 0.0f }));
    rlEnd();

    // Borde del paseo.
    AnilloAsi({ 0.0f, -0.08f, 0.0f }, 10.9f, 11.08f, Color{ 196, 178, 146, 255 }, 64);
}


// Baldosas en abanico desde el carrusel: el mismo dibujo de los toldos.
static void DibujarPlazaAsi()
{
    const float Y = SUELO_ASIENTO;
    const float radios[8] = { RADIO_CARRUSEL_FISICO_ASIENTO, 3.35f, 4.2f, 5.05f, 5.9f, 6.75f, 7.6f, RADIO_ARENA_ASIENTO };

    AnilloAsi({ 0.0f, Y - 0.006f, 0.0f }, RADIO_CARRUSEL_FISICO_ASIENTO - 0.1f, RADIO_ARENA_ASIENTO, JUNTA_ASI, 64);

    for (int a = 0; a < 7; a++)
    {
        float r0 = radios[a] + 0.035f;
        float r1 = radios[a + 1] - 0.035f;
        float rm = (r0 + r1) * 0.5f;
        int n = 16 * (int)(rm * 6.2832f / 1.15f / 16.0f + 0.5f);
        if (n < 16) n = 16;
        float hueco = 0.035f / rm;

        for (int s = 0; s < n; s++)
        {
            float a0 = 6.2832f * (float)s / (float)n + hueco;
            float a1 = 6.2832f * (float)(s + 1) / (float)n - hueco;
            int gajo = s * 16 / n;
            Color base = (gajo % 2 == 0) ? LOSA_A_ASI : LOSA_B_ASI;

            if (a == 0) base = MezclaAsi(LOSA_BORDE_ASI, ORO_ASI, 0.25f);
            if (a == 6) base = LOSA_BORDE_ASI;

            float f = 0.92f + 0.12f * Hash01Asi(a * 97 + s, 11);
            ArcoSueloAsi(0.0f, 0.0f, r0, r1, a0, a1, Y, TonoAsi(base, f), 2);
        }
    }

    // Filete de laton en el anillo exterior.
    AnilloAsi({ 0.0f, Y + 0.004f, 0.0f }, 7.56f, 7.64f, ORO_ASI, 72);
}


// Bordillo a rayas y valla baja: coinciden con el radio de la arena (el
// cuerpo del jugador se detiene justo antes).
static void DibujarBordeAsi(bool detalle)
{
    const int TRAMOS = 48;
    const float R0 = RADIO_ARENA_ASIENTO;
    const float R1 = RADIO_ARENA_ASIENTO + 0.45f;
    const float Y_ARRIBA = 0.16f;

    rlBegin(RL_QUADS);

    for (int k = 0; k < TRAMOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)TRAMOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)TRAMOS;
        float am = (a0 + a1) * 0.5f;
        Color piedra = (k % 2 == 0) ? CREMA_ASI : CORAL_ASI;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadAsi({ c0 * R0, Y_ARRIBA, s0 * R0 }, { c0 * R1, Y_ARRIBA, s0 * R1 }, { c1 * R1, Y_ARRIBA, s1 * R1 }, { c1 * R0, Y_ARRIBA, s1 * R0 }, piedra);
        QuadAsi({ c0 * R0, SUELO_ASIENTO, s0 * R0 }, { c0 * R0, Y_ARRIBA, s0 * R0 }, { c1 * R0, Y_ARRIBA, s1 * R0 }, { c1 * R0, SUELO_ASIENTO, s1 * R0 }, TonoAsi(piedra, LuzLateralAsi(am + 3.1416f) * 0.86f));
        QuadAsi({ c0 * R1, -0.12f, s0 * R1 }, { c0 * R1, Y_ARRIBA, s0 * R1 }, { c1 * R1, Y_ARRIBA, s1 * R1 }, { c1 * R1, -0.12f, s1 * R1 }, TonoAsi(piedra, LuzLateralAsi(am) * 0.9f));
    }

    rlEnd();

    if (!detalle)
    {
        return;
    }

    // Valla de postes blancos con remate dorado. Al frente es mas baja para
    // no tapar a quien corre por el borde.
    const int POSTES = 48;
    const float RV = RADIO_ARENA_ASIENTO + 0.24f;
    Vector3 anterior{};
    float altoAnterior = 0.0f;

    for (int k = 0; k <= POSTES; k++)
    {
        float a = 6.2832f * (float)k / (float)POSTES;
        float z = std::sin(a);
        float altura = 0.36f + 0.34f * Limitar01Asi(-z * 1.6f + 0.4f);
        Vector3 pie = { std::cos(a) * RV, Y_ARRIBA, z * RV };

        if (k < POSTES)
        {
            CuboGiradoAsi({ pie.x, Y_ARRIBA + altura * 0.5f, pie.z }, { 0.11f, altura, 0.11f }, a, CREMA_ASI);
            CuboGiradoAsi({ pie.x, Y_ARRIBA + altura + 0.05f, pie.z }, { 0.15f, 0.08f, 0.15f }, a, ORO_ASI);
        }

        if (k > 0)
        {
            float alturaRiel = (altura < altoAnterior ? altura : altoAnterior) - 0.06f;
            VigaAsi({ anterior.x, Y_ARRIBA + alturaRiel, anterior.z }, { pie.x, Y_ARRIBA + alturaRiel, pie.z }, 0.06f, MENTA_ASI);
            VigaAsi({ anterior.x, Y_ARRIBA + alturaRiel * 0.45f, anterior.z }, { pie.x, Y_ARRIBA + alturaRiel * 0.45f, pie.z }, 0.05f, CREMA_ASI);
        }

        anterior = pie;
        altoAnterior = altura;
    }
}


//==================================================
// VISUAL: CARRUSEL
//==================================================

static void CaballitoPrimitivoAsi(Vector3 c, float angulo, Color pelaje)
{
    // Frente tangente al giro.
    Vector3 frente = { -std::sin(angulo), 0.0f, std::cos(angulo) };
    float giro = std::atan2(frente.z, frente.x);
    CuboGiradoAsi(c, { 0.78f, 0.32f, 0.26f }, giro, pelaje);
    CuboGiradoAsi({ c.x + frente.x * 0.42f, c.y + 0.3f, c.z + frente.z * 0.42f }, { 0.22f, 0.42f, 0.2f }, giro, pelaje);
    CuboGiradoAsi({ c.x + frente.x * 0.52f, c.y + 0.5f, c.z + frente.z * 0.52f }, { 0.3f, 0.16f, 0.18f }, giro, pelaje);
    CuboGiradoAsi({ c.x - frente.x * 0.05f, c.y + 0.2f, c.z - frente.z * 0.05f }, { 0.3f, 0.06f, 0.3f }, giro, CORAL_ASI);

    for (int p = 0; p < 4; p++)
    {
        float lado = (p < 2) ? 0.28f : -0.28f;
        float desvio = (p % 2 == 0) ? 0.08f : -0.08f;
        Vector3 pata = { c.x + frente.x * lado + frente.z * desvio, c.y - 0.32f, c.z + frente.z * lado - frente.x * desvio };
        CuboAsi(pata, { 0.07f, 0.36f, 0.07f }, pelaje);
    }
}


static void DibujarCarruselAsi(const MinijuegoUltimoAsiento& m, bool luces, float t)
{
    const float y0 = SUELO_ASIENTO;
    const float yPiso = y0 + ALTO_PLATAFORMA_ASI;
    const float yAlero = y0 + ALTO_ALERO_ASI;
    const float R = RADIO_CARRUSEL_FISICO_ASIENTO;
    float giro = m.anguloCarrusel * DEG2RAD;

    // Plataforma: faldon cereza con estrellas que giran, bandas doradas y
    // piso de tablas en abanico.
    PrismaAsi({ 0.0f, y0, 0.0f }, R, R, ALTO_PLATAFORMA_ASI, 40, CORAL_OSCURO_ASI, false, giro);
    PrismaAsi({ 0.0f, y0, 0.0f }, R + 0.01f, R + 0.01f, 0.07f, 40, ORO_OSCURO_ASI, false, giro);
    PrismaAsi({ 0.0f, yPiso - 0.07f, 0.0f }, R + 0.01f, R + 0.01f, 0.07f, 40, ORO_ASI, false, giro);

    rlBegin(RL_QUADS);

    for (int k = 0; k < 12; k++)
    {
        float a = giro + 6.2832f * (float)k / 12.0f;
        Vector3 n = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 tg = { -n.z, 0.0f, n.x };
        Vector3 c = { n.x * (R + 0.015f), y0 + ALTO_PLATAFORMA_ASI * 0.5f, n.z * (R + 0.015f) };
        Color estrella = TonoAsi(CREMA_ASI, LuzLateralAsi(a));
        float s = 0.11f;
        QuadAsi(
            { c.x + tg.x * s, c.y, c.z + tg.z * s },
            { c.x, c.y + s, c.z },
            { c.x - tg.x * s, c.y, c.z - tg.z * s },
            { c.x, c.y - s, c.z },
            estrella
        );
    }

    rlEnd();

    for (int k = 0; k < 24; k++)
    {
        float a0 = giro + 6.2832f * (float)k / 24.0f;
        float a1 = giro + 6.2832f * (float)(k + 1) / 24.0f - 0.012f;
        Color tabla = TonoAsi((k % 2 == 0) ? MADERA_ASI : Color{ 196, 146, 100, 255 }, 0.95f + 0.08f * Hash01Asi(k, 71));
        ArcoSueloAsi(0.0f, 0.0f, 0.5f, R - 0.12f, a0, a1, yPiso, tabla, 1);
    }

    AnilloAsi({ 0.0f, yPiso + 0.003f, 0.0f }, R - 0.12f, R, ORO_ASI, 40);

    // Columna central de espejos.
    PrismaRayasAsi({ 0.0f, yPiso, 0.0f }, 0.44f, 0.44f, ALTO_ALERO_ASI - ALTO_PLATAFORMA_ASI, 8, TURQUESA_ASI, Color{ 214, 236, 238, 255 }, giro);
    PrismaAsi({ 0.0f, yPiso, 0.0f }, 0.5f, 0.5f, 0.18f, 8, ORO_ASI, true, giro);
    PrismaAsi({ 0.0f, yPiso + 1.1f, 0.0f }, 0.48f, 0.48f, 0.08f, 8, ORO_ASI, false, giro);

    // Caballitos en sus barras: suben y bajan al ritmo del organillo.
    for (int k = 0; k < 6; k++)
    {
        float a = giro + (float)k * 6.2832f / 6.0f;
        float x = std::cos(a) * RADIO_CABALLITOS_ASI;
        float z = std::sin(a) * RADIO_CABALLITOS_ASI;
        float vaiven = std::sin(t * 4.0f + (float)k * 1.4f) * 0.16f * m.velocidadCarrusel;
        Vector3 cuerpo = { x, yPiso + 0.95f + vaiven, z };

        TuboAsi({ x, yPiso, z }, { x, yAlero, z }, 0.045f, 0.045f, ORO_ASI, 6, false);

        if (!DibujarModeloUltimoAsientoRetro3D(MODELO_ASIENTO_CABALLITO, cuerpo, -a * RAD2DEG - 90.0f))
        {
            CaballitoPrimitivoAsi(cuerpo, a, (k % 2 == 0) ? CREMA_ASI : Color{ 236, 200, 150, 255 });
        }
    }

    // Toldo: corona turquesa con espejos, cono a gajos y faldon de picos.
    PrismaRayasAsi({ 0.0f, yAlero, 0.0f }, RADIO_TOLDO_ASI, RADIO_TOLDO_ASI, 0.26f, 24, TURQUESA_ASI, Color{ 70, 170, 172, 255 }, giro);
    PrismaAsi({ 0.0f, yAlero + 0.26f, 0.0f }, RADIO_TOLDO_ASI + 0.02f, RADIO_TOLDO_ASI + 0.02f, 0.05f, 24, ORO_ASI, false, giro);
    ConoRayasAsi({ 0.0f, yAlero + 0.3f, 0.0f }, RADIO_TOLDO_ASI + 0.06f, ALTO_CUSPIDE_ASI - ALTO_ALERO_ASI - 0.3f, 16, CORAL_ASI, CREMA_ASI, giro);
    FaldonAsi({ 0.0f, 0.0f, 0.0f }, RADIO_TOLDO_ASI + 0.01f, yAlero + 0.01f, 0.24f, 24, CREMA_ASI, CORAL_ASI, giro);

    // Bombillas del alero: corren mientras suena la musica.
    for (int k = 0; k < 24; k++)
    {
        float a = giro + 6.2832f * ((float)k + 0.5f) / 24.0f;
        Vector3 p = { std::cos(a) * (RADIO_TOLDO_ASI + 0.04f), yAlero + 0.13f, std::sin(a) * (RADIO_TOLDO_ASI + 0.04f) };
        bool encendida = luces && ((k + (int)(t * 8.0f)) % 3 != 0);
        CuboAsi(p, { 0.12f, 0.12f, 0.12f }, encendida ? Color{ 255, 236, 170, 255 } : Color{ 196, 180, 150, 255 });
    }

    // Remate: bola dorada y banderin que ondea.
    Vector3 cima = { 0.0f, y0 + ALTO_CUSPIDE_ASI, 0.0f };
    TuboAsi(cima, { 0.0f, cima.y + 0.75f, 0.0f }, 0.035f, 0.03f, ORO_OSCURO_ASI, 6, false);
    EsferaAsi({ 0.0f, cima.y + 0.06f, 0.0f }, 0.16f, ORO_ASI, 10, 3);
    EsferaAsi({ 0.0f, cima.y + 0.78f, 0.0f }, 0.07f, ORO_ASI, 8, 2);

    rlBegin(RL_QUADS);

    for (int s = 0; s < 4; s++)
    {
        float u0 = (float)s / 4.0f;
        float u1 = (float)(s + 1) / 4.0f;
        float o0 = std::sin(t * 6.0f - u0 * 4.0f) * 0.08f * u0;
        float o1 = std::sin(t * 6.0f - u1 * 4.0f) * 0.08f * u1;
        float h0 = 0.2f * (1.0f - u0);
        float h1 = 0.2f * (1.0f - u1);
        float yb = cima.y + 0.48f;
        QuadAsi({ u0 * 0.6f, yb - h0, o0 }, { u0 * 0.6f, yb + h0, o0 }, { u1 * 0.6f, yb + h1, o1 }, { u1 * 0.6f, yb - h1, o1 }, (s % 2 == 0) ? CORAL_ASI : ORO_ASI);
    }

    rlEnd();
}


//==================================================
// VISUAL: TAZAS (los asientos de la ronda)
//==================================================

static bool TazasVisiblesAsi(const MinijuegoUltimoAsiento& m)
{
    return m.fase == FASE_ULTIMO_ASIENTO_JUGANDO && m.subfase != SUBFASE_ULTIMO_ASIENTO_MUSICA;
}


static bool AvisoTrampaAsi(const MinijuegoUltimoAsiento& m, int c)
{
    return
        m.subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS &&
        m.tazas[c].trampa &&
        m.tiempoSubfase <= DURACION_AVISO_TRAMPA_ASIENTO;
}


// Las tazas brotan del piso en ola al cortarse la musica.
static float AparicionTazaAsi(const MinijuegoUltimoAsiento& m, int c)
{
    if (m.subfase != SUBFASE_ULTIMO_ASIENTO_TAZAS)
    {
        return 1.0f;
    }

    float transcurrido = DURACION_TAZAS_ASIENTO - m.tiempoSubfase;
    return EscalaAparicionAsi((transcurrido - 0.06f * (float)c) / 0.34f);
}


// Color de estado de la franja de la taza (y de su anillo en el piso).
static Color ColorTazaAsi(const MinijuegoUltimoAsiento& m, int c, float t)
{
    const TazaUltimoAsiento& taza = m.tazas[c];

    if (m.subfase == SUBFASE_ULTIMO_ASIENTO_RESOLUCION)
    {
        if (taza.trampa) return Color{ 150, 34, 34, 255 };
        if (taza.ocupante >= 0) return m.coloresJugadores[taza.ocupante];
        return Color{ 150, 146, 140, 255 };
    }

    if (AvisoTrampaAsi(m, c))
    {
        return std::fmod(t * 8.0f, 1.0f) < 0.5f ? TRAMPA_ASI : Color{ 255, 250, 245, 255 };
    }

    if (taza.ocupante >= 0)
    {
        return m.coloresJugadores[taza.ocupante];
    }

    float pulso = 0.5f + 0.5f * std::sin(t * 8.0f);
    return MezclaAsi(ACENTO_ASI, Color{ 255, 244, 196, 255 }, pulso * 0.6f);
}


static void TazaPrimitivaAsi(Vector3 pie, float escala, Color franja)
{
    float r = 1.2f * escala;
    PrismaAsi(pie, r, r * 0.98f, 0.06f * escala, 24, CREMA_ASI);
    PrismaAsi({ pie.x, pie.y + 0.06f * escala, pie.z }, r * 0.62f, r * 0.82f, 0.64f * escala, 20, Color{ 246, 244, 240, 255 }, false);
    PrismaAsi({ pie.x, pie.y + 0.36f * escala, pie.z }, r * 0.72f, r * 0.76f, 0.16f * escala, 20, franja, false);
    AnilloAsi({ pie.x, pie.y + 0.7f * escala, pie.z }, r * 0.74f, r * 0.84f, ORO_ASI, 20);
}


static void DibujarTazasAsi(const MinijuegoUltimoAsiento& m, float t)
{
    if (!TazasVisiblesAsi(m))
    {
        return;
    }

    float cierre = Limitar01Asi(m.tiempoSubfase / DURACION_TAZAS_ASIENTO);

    for (int c = 0; c < m.cantidadTazas; c++)
    {
        const TazaUltimoAsiento& taza = m.tazas[c];

        if (!taza.activa)
        {
            continue;
        }

        float s = AparicionTazaAsi(m, c);

        if (s <= 0.01f)
        {
            continue;
        }

        Color estado = ColorTazaAsi(m, c, t);
        float x = taza.posicion.x;
        float z = taza.posicion.z;

        // Anillo de estado en el piso (radio visual de la taza).
        AnilloAsi({ x, SUELO_ASIENTO + 0.012f, z }, (RADIO_TAZA_ASIENTO - 0.16f) * s, RADIO_TAZA_ASIENTO * s, estado, 32);

        // Reloj de cierre: el arco se consume mientras la taza sigue abierta.
        if (m.subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
        {
            float r0 = RADIO_TAZA_ASIENTO + 0.1f;
            float r1 = RADIO_TAZA_ASIENTO + 0.26f;
            ArcoSueloAsi(x, z, r0 * s, r1 * s, 0.0f, 6.2832f, SUELO_ASIENTO + 0.010f, AlfaAsi(JUNTA_ASI, 255.0f), 36);
            int pasos = 2 + (int)(34.0f * cierre);
            ArcoSueloAsi(x, z, r0 * s, r1 * s, -1.5708f, -1.5708f + 6.2832f * cierre, SUELO_ASIENTO + 0.016f, estado, pasos);
        }

        // La taza: salta al ocuparse y la trampa se sacude al expulsar.
        float salto = 1.0f + 0.16f * taza.destello + 0.1f * m.rebote[c];
        float alzada = 0.0f;
        float giroTaza = (float)c * 67.0f + 20.0f;

        if (m.subfase == SUBFASE_ULTIMO_ASIENTO_RESOLUCION && taza.trampa)
        {
            float u = DURACION_RESOLUCION_ASIENTO - m.tiempoSubfase;
            alzada = std::fabs(std::sin(u * 18.0f)) * 0.22f * Limitar01Asi(1.0f - u * 1.4f);
            giroTaza += std::sin(u * 30.0f) * 12.0f * Limitar01Asi(1.0f - u);
        }
        else if (AvisoTrampaAsi(m, c))
        {
            giroTaza += std::sin(t * 40.0f) * 4.0f;
        }

        Vector3 pie = { x, SUELO_ASIENTO + alzada, z };
        float e = s * salto;

        if (!DibujarModeloUltimoAsientoRetro3D(MODELO_ASIENTO_TAZA, pie, giroTaza, { 0.0f, 1.0f, 0.0f }, { e, e, e }, estado))
        {
            TazaPrimitivaAsi(pie, e, estado);
        }
    }
}


//==================================================
// VISUAL: UTILERIA DE LA FERIA
//==================================================

static Vector3 PosicionFarolAsi(int k)
{
    float a = ANGULOS_FAROLES_ASI[k] * DEG2RAD;
    return { std::cos(a) * RADIO_FAROLES_ASI, 0.0f, std::sin(a) * RADIO_FAROLES_ASI };
}


// Farola de hierro verde con globo: devuelve el gancho de la guirnalda.
static Vector3 FarolAsi(Vector3 pie)
{
    PrismaAsi({ pie.x, -0.08f, pie.z }, 0.2f, 0.16f, 0.3f, 8, HIERRO_ASI);
    TuboAsi({ pie.x, 0.2f, pie.z }, { pie.x, 3.1f, pie.z }, 0.07f, 0.05f, HIERRO_ASI, 6, false);
    PrismaAsi({ pie.x, 1.1f, pie.z }, 0.09f, 0.09f, 0.12f, 6, ORO_ASI, false);
    EsferaAsi({ pie.x, 3.3f, pie.z }, 0.2f, Color{ 255, 246, 220, 255 }, 10, 3);
    PrismaAsi({ pie.x, 3.46f, pie.z }, 0.16f, 0.04f, 0.12f, 8, HIERRO_ASI);
    return { pie.x, 3.02f, pie.z };
}


static Vector3 PuntoGuirnaldaAsi(Vector3 a, Vector3 b, float comba, float u)
{
    return
    {
        a.x + (b.x - a.x) * u,
        a.y + (b.y - a.y) * u - comba * 4.0f * u * (1.0f - u),
        a.z + (b.z - a.z) * u
    };
}


static Color ColorBombillaAsi(int k)
{
    const Color colores[5] =
    {
        Color{ 255, 236, 170, 255 }, Color{ 255, 168, 150, 255 }, Color{ 170, 236, 210, 255 },
        Color{ 255, 214, 120, 255 }, Color{ 214, 196, 255, 255 }
    };
    return colores[k % 5];
}


// Cable con bombillas de colores colgando entre dos ganchos.
static void GuirnaldaAsi(Vector3 a, Vector3 b, float comba, int bombillas, int semilla)
{
    const int TRAMOS = 10;
    Vector3 anterior = a;

    for (int s = 1; s <= TRAMOS; s++)
    {
        Vector3 p = PuntoGuirnaldaAsi(a, b, comba, (float)s / (float)TRAMOS);
        DrawLine3D(anterior, p, Color{ 60, 60, 64, 255 });
        anterior = p;
    }

    for (int k = 0; k < bombillas; k++)
    {
        Vector3 p = PuntoGuirnaldaAsi(a, b, comba, ((float)k + 0.5f) / (float)bombillas);
        CuboAsi({ p.x, p.y - 0.08f, p.z }, { 0.11f, 0.15f, 0.11f }, ColorBombillaAsi(k + semilla));
    }
}


// Cordel de banderines triangulares que ondean.
static void BanderinesAsi(Vector3 a, Vector3 b, float comba, int cantidad, float t, int semilla)
{
    const Color colores[5] = { CORAL_ASI, CREMA_ASI, MENTA_ASI, LIMON_ASI, LILA_ASI };
    Vector3 anterior = a;

    for (int s = 1; s <= 8; s++)
    {
        Vector3 p = PuntoGuirnaldaAsi(a, b, comba, (float)s / 8.0f);
        DrawLine3D(anterior, p, Color{ 90, 80, 70, 255 });
        anterior = p;
    }

    rlBegin(RL_QUADS);

    for (int k = 0; k < cantidad; k++)
    {
        float u0 = ((float)k + 0.1f) / (float)cantidad;
        float u1 = ((float)k + 0.9f) / (float)cantidad;
        Vector3 p0 = PuntoGuirnaldaAsi(a, b, comba, u0);
        Vector3 p1 = PuntoGuirnaldaAsi(a, b, comba, u1);
        Vector3 punta = PuntoGuirnaldaAsi(a, b, comba, (u0 + u1) * 0.5f);
        float ondeo = std::sin(t * 3.4f + (float)k * 0.9f + (float)semilla) * 0.08f;
        punta.y -= 0.34f;
        punta.z += ondeo;
        Color c = colores[(k + semilla) % 5];
        QuadAsi(p0, punta, punta, p1, c);
    }

    rlEnd();
}


// Puesto con mostrador, toldo a rayas y mercaderia segun el tipo:
// 0 palomitas, 1 algodon de azucar, 2 premios, 3 limonada.
static void PuestoAsi(Vector3 c, int tipo, float t)
{
    const Color rayas[4] = { CORAL_ASI, ROSA_ASI, MENTA_ASI, LIMON_ASI };
    const Color paredes[4] = { Color{ 250, 230, 210, 255 }, Color{ 236, 214, 240, 255 }, Color{ 214, 236, 226, 255 }, Color{ 252, 240, 200, 255 } };
    Color raya = TinteAsi(rayas[tipo], c);
    Color crema = TinteAsi(CREMA_ASI, c);
    Color pared = TinteAsi(paredes[tipo], c);
    Color madera = TinteAsi(MADERA_ASI, c);

    CuboAsi({ c.x, 1.25f, c.z - 0.72f }, { 3.2f, 2.5f, 0.14f }, pared);
    CuboAsi({ c.x - 1.55f, 1.25f, c.z }, { 0.12f, 2.5f, 1.56f }, pared);
    CuboAsi({ c.x + 1.55f, 1.25f, c.z }, { 0.12f, 2.5f, 1.56f }, pared);
    CuboAsi({ c.x, 0.5f, c.z + 0.62f }, { 3.1f, 1.0f, 0.36f }, madera);
    CuboAsi({ c.x, 1.04f, c.z + 0.62f }, { 3.24f, 0.08f, 0.5f }, TinteAsi(Color{ 236, 214, 176, 255 }, c));

    // Frente del mostrador a rayas.
    for (int k = 0; k < 8; k++)
    {
        float x = c.x - 1.55f + 0.39f * ((float)k + 0.5f);
        CuboAsi({ x, 0.5f, c.z + 0.81f }, { 0.39f, 0.9f, 0.03f }, (k % 2 == 0) ? raya : crema);
    }

    TuboAsi({ c.x - 1.5f, 1.08f, c.z + 0.78f }, { c.x - 1.5f, 2.3f, c.z + 1.08f }, 0.04f, 0.04f, TinteAsi(ORO_ASI, c), 6, false);
    TuboAsi({ c.x + 1.5f, 1.08f, c.z + 0.78f }, { c.x + 1.5f, 2.3f, c.z + 1.08f }, 0.04f, 0.04f, TinteAsi(ORO_ASI, c), 6, false);

    // Toldo inclinado a rayas y faldon de picos.
    rlBegin(RL_QUADS);

    for (int k = 0; k < 8; k++)
    {
        float x0 = c.x - 1.7f + 3.4f * (float)k / 8.0f;
        float x1 = c.x - 1.7f + 3.4f * (float)(k + 1) / 8.0f;
        Color tela = TonoAsi((k % 2 == 0) ? raya : crema, 0.96f);
        QuadAsi({ x0, 2.75f, c.z - 0.8f }, { x0, 2.3f, c.z + 1.2f }, { x1, 2.3f, c.z + 1.2f }, { x1, 2.75f, c.z - 0.8f }, tela);
        Color pico = TonoAsi((k % 2 == 0) ? crema : raya, 0.8f);
        QuadAsi({ x0, 2.3f, c.z + 1.2f }, { (x0 + x1) * 0.5f, 2.0f, c.z + 1.2f }, { (x0 + x1) * 0.5f, 2.0f, c.z + 1.2f }, { x1, 2.3f, c.z + 1.2f }, pico);
    }

    rlEnd();

    // Cartel con el icono del puesto.
    CuboAsi({ c.x, 3.08f, c.z - 0.62f }, { 1.9f, 0.56f, 0.1f }, crema);
    CuboAsi({ c.x, 3.08f, c.z - 0.56f }, { 1.7f, 0.4f, 0.04f }, raya);

    switch (tipo)
    {
        case 0:
        {
            // Maquina de palomitas: vitrina con pochoclo y cajitas.
            CuboAsi({ c.x - 0.6f, 1.45f, c.z + 0.2f }, { 0.9f, 0.8f, 0.7f }, TinteAsi(Color{ 210, 236, 246, 255 }, c));
            CuboAsi({ c.x - 0.6f, 1.25f, c.z + 0.2f }, { 0.84f, 0.34f, 0.64f }, TinteAsi(Color{ 252, 228, 150, 255 }, c));
            CuboAsi({ c.x - 0.6f, 1.9f, c.z + 0.2f }, { 1.0f, 0.12f, 0.8f }, raya);

            for (int k = 0; k < 4; k++)
            {
                Vector3 caja = { c.x + 0.25f + 0.32f * (float)k, 1.28f, c.z + 0.62f };
                CuboAsi(caja, { 0.2f, 0.36f, 0.16f }, (k % 2 == 0) ? raya : crema);
                CuboAsi({ caja.x, 1.5f, caja.z }, { 0.22f, 0.1f, 0.18f }, TinteAsi(Color{ 252, 236, 170, 255 }, c));
            }
            break;
        }

        case 1:
        {
            // Algodones de azucar en su varilla, en un exhibidor.
            for (int k = 0; k < 6; k++)
            {
                float x = c.x - 1.1f + 0.44f * (float)k;
                float y = 1.55f + 0.18f * (float)(k % 2);
                TuboAsi({ x, 1.08f, c.z + 0.55f }, { x, y, c.z + 0.55f }, 0.02f, 0.02f, crema, 4, false);
                EsferaAsi({ x, y + 0.16f, c.z + 0.55f }, 0.2f, TinteAsi((k % 2 == 0) ? ROSA_ASI : LILA_ASI, c), 8, 2);
            }
            break;
        }

        case 2:
        {
            // Tiro a las latas: piramide de latas y peluches colgados.
            for (int fila = 0; fila < 3; fila++)
            {
                for (int k = 0; k < 3 - fila; k++)
                {
                    float x = c.x - 0.3f + 0.22f * (float)k + 0.11f * (float)fila;
                    PrismaAsi({ x, 1.08f + 0.24f * (float)fila, c.z + 0.6f }, 0.09f, 0.09f, 0.22f, 8, (k + fila) % 2 == 0 ? TinteAsi(Color{ 196, 200, 210, 255 }, c) : raya);
                }
            }

            for (int k = 0; k < 4; k++)
            {
                float x = c.x - 1.15f + 0.75f * (float)k;
                Color peluche = TinteAsi((k % 2 == 0) ? Color{ 196, 150, 104, 255 } : LILA_ASI, c);
                float vaiven = std::sin(t * 1.6f + (float)k) * 0.03f;
                EsferaAsi({ x + vaiven, 1.95f, c.z - 0.5f }, 0.2f, peluche, 8, 2);
                EsferaAsi({ x + vaiven, 2.26f, c.z - 0.5f }, 0.14f, peluche, 8, 2);
                EsferaAsi({ x + vaiven - 0.1f, 2.37f, c.z - 0.5f }, 0.05f, peluche, 6, 2);
                EsferaAsi({ x + vaiven + 0.1f, 2.37f, c.z - 0.5f }, 0.05f, peluche, 6, 2);
            }
            break;
        }

        default:
        {
            // Limonada: jarras y limones.
            for (int k = 0; k < 3; k++)
            {
                float x = c.x - 0.9f + 0.9f * (float)k;
                PrismaAsi({ x, 1.08f, c.z + 0.45f }, 0.18f, 0.2f, 0.46f, 10, TinteAsi(Color{ 252, 236, 150, 255 }, c));
                PrismaAsi({ x, 1.54f, c.z + 0.45f }, 0.2f, 0.12f, 0.08f, 10, crema);
                EsferaAsi({ x + 0.35f, 1.15f, c.z + 0.65f }, 0.08f, TinteAsi(LIMON_ASI, c), 6, 2);
            }
            break;
        }
    }
}


// Carpa de circo a rayas con banderin en el mastil.
static void CarpaAsi(Vector3 c, float t)
{
    Color coral = TinteAsi(CORAL_ASI, c);
    Color crema = TinteAsi(CREMA_ASI, c);

    PrismaRayasAsi(c, 3.6f, 3.6f, 1.8f, 24, coral, crema);
    FaldonAsi(c, 3.66f, 1.84f, 0.32f, 24, TinteAsi(ORO_ASI, c), crema);
    ConoRayasAsi({ c.x, 1.8f, c.z }, 3.7f, 1.7f, 24, crema, coral, 0.0f, 1.3f);
    ConoRayasAsi({ c.x, 3.5f, c.z }, 1.3f, 1.4f, 24, crema, coral);

    // Entrada con cortinas recogidas hacia la feria.
    CuboGiradoAsi({ c.x + 1.2f, 0.85f, c.z + 3.25f }, { 1.3f, 1.7f, 0.1f }, 0.38f, TinteAsi(Color{ 120, 36, 52, 255 }, c));
    CuboGiradoAsi({ c.x + 1.0f, 2.05f, c.z + 3.3f }, { 1.6f, 0.3f, 0.12f }, 0.38f, TinteAsi(ORO_ASI, c));

    Vector3 cima = { c.x, 4.9f, c.z };
    TuboAsi(cima, { c.x, 6.0f, c.z }, 0.05f, 0.04f, TinteAsi(ORO_OSCURO_ASI, c), 6, false);

    rlBegin(RL_QUADS);

    for (int s = 0; s < 4; s++)
    {
        float u0 = (float)s / 4.0f;
        float u1 = (float)(s + 1) / 4.0f;
        float o0 = std::sin(t * 5.0f - u0 * 4.0f) * 0.12f * u0;
        float o1 = std::sin(t * 5.0f - u1 * 4.0f) * 0.12f * u1;
        float h0 = 0.28f * (1.0f - u0 * 0.8f);
        float h1 = 0.28f * (1.0f - u1 * 0.8f);
        QuadAsi({ c.x + u0, 5.7f - h0, c.z + o0 }, { c.x + u0, 5.7f + h0, c.z + o0 }, { c.x + u1, 5.7f + h1, c.z + o1 }, { c.x + u1, 5.7f - h1, c.z + o1 }, TinteAsi(MENTA_ASI, c));
    }

    rlEnd();

    // Vientos de la carpa con estacas.
    for (int k = 0; k < 6; k++)
    {
        float a = 6.2832f * ((float)k + 0.5f) / 6.0f;
        Vector3 borde = { c.x + std::cos(a) * 3.7f, 1.8f, c.z + std::sin(a) * 3.7f };
        Vector3 estaca = { c.x + std::cos(a) * 5.0f, 0.0f, c.z + std::sin(a) * 5.0f };
        DrawLine3D(borde, estaca, TinteAsi(Color{ 120, 100, 80, 255 }, c));
        CuboAsi({ estaca.x, 0.05f, estaca.z }, { 0.1f, 0.2f, 0.1f }, TinteAsi(MADERA_OSCURA_ASI, c));
    }
}


static void NoriaAsi(float t)
{
    const Vector3 centro = NORIA_ASI;
    float giroNoria = t * 0.22f;

    if (!DibujarModeloUltimoAsientoRetro3D(MODELO_ASIENTO_NORIA_SOPORTE, { centro.x, 0.0f, centro.z }))
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            TuboAsi({ centro.x - 2.0f, 0.0f, centro.z + 0.8f * (float)lado }, { centro.x, centro.y, centro.z + 0.5f * (float)lado }, 0.16f, 0.12f, TinteAsi(Color{ 200, 206, 220, 255 }, centro), 6, false);
            TuboAsi({ centro.x + 2.0f, 0.0f, centro.z + 0.8f * (float)lado }, { centro.x, centro.y, centro.z + 0.5f * (float)lado }, 0.16f, 0.12f, TinteAsi(Color{ 200, 206, 220, 255 }, centro), 6, false);
        }
    }

    bool ruedaGLB = DibujarModeloUltimoAsientoRetro3D(MODELO_ASIENTO_NORIA_RUEDA, centro, giroNoria * RAD2DEG, { 0.0f, 0.0f, 1.0f });

    for (int k = 0; k < 8; k++)
    {
        float angulo = giroNoria + (float)k * 6.2832f / 8.0f;
        float siguiente = giroNoria + (float)(k + 1) * 6.2832f / 8.0f;
        Vector3 borde = { centro.x + std::cos(angulo) * 5.0f, centro.y + std::sin(angulo) * 5.0f, centro.z };
        Vector3 bordeSiguiente = { centro.x + std::cos(siguiente) * 5.0f, centro.y + std::sin(siguiente) * 5.0f, centro.z };

        if (!ruedaGLB)
        {
            VigaAsi(centro, borde, 0.08f, TinteAsi(CREMA_ASI, centro));
            VigaAsi(borde, bordeSiguiente, 0.14f, TinteAsi(CORAL_ASI, centro));
        }

        // Las cabinas cuelgan siempre verticales.
        Color cabina = ColorFromHSV((float)k * 45.0f, 0.42f, 0.98f);

        if (!DibujarModeloUltimoAsientoRetro3D(MODELO_ASIENTO_NORIA_CABINA, borde, 0.0f, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, cabina))
        {
            CuboAsi({ borde.x, borde.y - 0.5f, borde.z }, { 0.8f, 0.7f, 0.7f }, TinteAsi(cabina, centro));
        }
    }
}


// Martillo de fuerza: el disco sube de golpe y a veces toca la campana.
static bool MartilloFuerzaAsi(Vector3 pie, float t)
{
    CuboAsi({ pie.x, 0.08f, pie.z }, { 1.2f, 0.26f, 1.0f }, MADERA_OSCURA_ASI);
    CuboAsi({ pie.x, 0.26f, pie.z + 0.1f }, { 0.5f, 0.12f, 0.5f }, CORAL_ASI);

    const float ALTO = 4.4f;
    CuboAsi({ pie.x, ALTO * 0.5f, pie.z - 0.3f }, { 0.34f, ALTO, 0.16f }, CREMA_ASI);

    for (int k = 0; k < 8; k++)
    {
        float y = 0.6f + 0.46f * (float)k;
        Color marca = MezclaAsi(MENTA_ASI, CORAL_ASI, (float)k / 7.0f);
        CuboAsi({ pie.x, y, pie.z - 0.215f }, { 0.36f, 0.1f, 0.02f }, marca);
    }

    DomoAsi({ pie.x, ALTO + 0.02f, pie.z - 0.3f }, 0.28f, 0.3f, 0.28f, ORO_ASI, 10, 3);

    // Ciclo de 3.6 s: golpe, subida, caida lenta. Uno de cada tres llega.
    float ciclo = std::fmod(t, 3.6f);
    int golpe = (int)(t / 3.6f);
    float tope = (golpe % 3 == 0) ? 1.0f : 0.45f + 0.35f * Hash01Asi(golpe, 81);
    float u = ciclo < 0.5f ? SuaveAsi(ciclo / 0.5f) : 1.0f - SuaveAsi((ciclo - 0.5f) / 1.6f);
    float y = 0.45f + (ALTO - 0.75f) * tope * u;
    CuboAsi({ pie.x, y, pie.z - 0.16f }, { 0.26f, 0.14f, 0.1f }, CORAL_OSCURO_ASI);

    // Mazo apoyado.
    TuboAsi({ pie.x + 0.5f, 0.0f, pie.z + 0.45f }, { pie.x + 0.62f, 1.2f, pie.z + 0.3f }, 0.04f, 0.04f, MADERA_ASI, 5, false);
    CuboGiradoAsi({ pie.x + 0.63f, 1.25f, pie.z + 0.29f }, { 0.5f, 0.22f, 0.22f }, 0.3f, CORAL_ASI);

    return tope >= 1.0f && ciclo > 0.42f && ciclo < 1.0f;
}


static void TaquillaAsi(Vector3 pie)
{
    CuboAsi({ pie.x, 1.0f, pie.z }, { 1.5f, 2.0f, 1.5f }, Color{ 248, 228, 196, 255 });
    CuboAsi({ pie.x, 1.3f, pie.z + 0.76f }, { 0.9f, 0.6f, 0.04f }, Color{ 120, 170, 196, 255 });
    CuboAsi({ pie.x, 0.98f, pie.z + 0.86f }, { 1.1f, 0.06f, 0.24f }, MADERA_ASI);
    CuboAsi({ pie.x, 0.1f, pie.z }, { 1.6f, 0.2f, 1.6f }, CORAL_OSCURO_ASI);
    ConoRayasAsi({ pie.x, 2.0f, pie.z }, 1.15f, 0.9f, 12, CORAL_ASI, CREMA_ASI, 0.26f);
    EsferaAsi({ pie.x, 2.95f, pie.z }, 0.1f, ORO_ASI, 8, 2);
    CuboAsi({ pie.x, 2.15f, pie.z + 0.8f }, { 1.2f, 0.3f, 0.06f }, MENTA_ASI);
}


static void SombrillaAsi(Vector3 c, float radio, Color a, Color b)
{
    TuboAsi({ c.x, c.y - 1.2f, c.z }, c, 0.025f, 0.025f, CREMA_ASI, 4, false);
    ConoRayasAsi(c, radio, 0.36f, 12, a, b);
    FaldonAsi(c, radio, c.y + 0.01f, 0.1f, 12, b, a);
}


static void CarritoHeladosAsi(Vector3 pie)
{
    CuboAsi({ pie.x, 0.75f, pie.z }, { 1.5f, 0.8f, 0.9f }, Color{ 236, 240, 246, 255 });
    CuboAsi({ pie.x, 0.75f, pie.z + 0.46f }, { 1.5f, 0.22f, 0.02f }, MENTA_ASI);
    CuboAsi({ pie.x, 1.19f, pie.z }, { 1.56f, 0.08f, 0.96f }, ROSA_ASI);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        TuboAsi({ pie.x + 0.55f * (float)lado, 0.25f, pie.z - 0.48f }, { pie.x + 0.55f * (float)lado, 0.25f, pie.z + 0.48f }, 0.24f, 0.24f, Color{ 70, 70, 80, 255 }, 10, true);
    }

    for (int k = 0; k < 3; k++)
    {
        float x = pie.x - 0.4f + 0.4f * (float)k;
        DomoAsi({ x, 1.24f, pie.z }, 0.15f, 0.14f, 0.15f, (k == 0) ? ROSA_ASI : (k == 1 ? Color{ 250, 244, 226, 255 } : Color{ 150, 100, 76, 255 }), 8, 2);
    }

    SombrillaAsi({ pie.x, 2.6f, pie.z }, 1.2f, MENTA_ASI, CREMA_ASI);
}


static void CarritoGlobosAsi(Vector3 pie, float t)
{
    CuboAsi({ pie.x, 0.55f, pie.z }, { 1.0f, 0.6f, 0.7f }, Color{ 120, 170, 210, 255 });
    CuboAsi({ pie.x, 0.88f, pie.z }, { 1.06f, 0.06f, 0.76f }, CREMA_ASI);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        TuboAsi({ pie.x + 0.36f * (float)lado, 0.2f, pie.z - 0.38f }, { pie.x + 0.36f * (float)lado, 0.2f, pie.z + 0.38f }, 0.18f, 0.18f, Color{ 70, 70, 80, 255 }, 10, true);
    }

    Vector3 ancla = { pie.x, 0.95f, pie.z };
    const Color globos[7] = { CORAL_ASI, LIMON_ASI, MENTA_ASI, LILA_ASI, ROSA_ASI, Color{ 120, 190, 240, 255 }, Color{ 255, 170, 90, 255 } };

    for (int k = 0; k < 7; k++)
    {
        float a = (float)k * 0.9f;
        float vaiven = std::sin(t * 1.4f + (float)k * 1.3f) * 0.12f;
        Vector3 globo = { pie.x + std::cos(a) * 0.45f + vaiven, 2.9f + 0.35f * (float)(k % 3), pie.z + std::sin(a) * 0.35f };

        if (DibujarModeloUltimoAsientoRetro3D(MODELO_ASIENTO_GLOBO, globo, 0.0f, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, globos[k]))
        {
            DrawLine3D(ancla, { globo.x, globo.y - 1.7006f, globo.z }, Color{ 240, 240, 244, 255 });
        }
        else
        {
            DrawLine3D(ancla, globo, Color{ 240, 240, 244, 255 });
            DomoAsi(globo, 0.3f, 0.38f, 0.3f, globos[k], 10, 3);
            DomoAsi(globo, 0.3f, -0.34f, 0.3f, TonoAsi(globos[k], 0.85f), 10, 3);
        }
    }
}


static void BancoAsi(Vector3 c, float giro)
{
    float co = std::cos(giro);
    float si = std::sin(giro);

    for (int k = 0; k < 3; k++)
    {
        float d = -0.15f + 0.15f * (float)k;
        CuboGiradoAsi({ c.x - si * d, 0.45f, c.z + co * d }, { 1.6f, 0.06f, 0.12f }, giro, MADERA_ASI);
    }

    CuboGiradoAsi({ c.x + si * 0.28f, 0.8f, c.z - co * 0.28f }, { 1.6f, 0.28f, 0.06f }, giro, MADERA_ASI);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        CuboGiradoAsi({ c.x + co * 0.7f * (float)lado, 0.3f, c.z + si * 0.7f * (float)lado }, { 0.08f, 0.6f, 0.5f }, giro, HIERRO_ASI);
    }
}


static void PapeleraAsi(Vector3 c)
{
    PrismaRayasAsi({ c.x, -0.08f, c.z }, 0.26f, 0.3f, 0.82f, 10, HIERRO_ASI, TonoAsi(HIERRO_ASI, 1.2f));
    PrismaAsi({ c.x, 0.74f, c.z }, 0.33f, 0.2f, 0.1f, 10, HIERRO_ASI);
}


static void MaceteroAsi(Vector3 c, int semilla)
{
    const Color flores[4] = { CORAL_ASI, LIMON_ASI, LILA_ASI, Color{ 250, 250, 246, 255 } };
    PrismaAsi({ c.x, -0.08f, c.z }, 0.62f, 0.7f, 0.5f, 12, Color{ 206, 196, 182, 255 });
    DomoAsi({ c.x, 0.42f, c.z }, 0.6f, 0.34f, 0.6f, HOJA_ASI, 10, 3);

    for (int k = 0; k < 9; k++)
    {
        float a = 6.2832f * Hash01Asi(k, semilla);
        float r = 0.5f * Hash01Asi(k, semilla + 1);
        CuboAsi({ c.x + std::cos(a) * r, 0.62f + 0.12f * (1.0f - r), c.z + std::sin(a) * r }, { 0.12f, 0.1f, 0.12f }, flores[(k + semilla) % 4]);
    }
}


static void ArbolAsi(Vector3 pie, float alto, int semilla)
{
    Color hoja = TinteAsi(MezclaAsi(HOJA_ASI, HOJA_OSCURA_ASI, Hash01Asi(semilla, 91)), pie);
    TuboAsi(pie, { pie.x, pie.y + alto * 0.45f, pie.z }, 0.2f * alto / 3.0f, 0.14f * alto / 3.0f, TinteAsi(TRONCO_ASI, pie), 6, false);

    for (int k = 0; k < 3; k++)
    {
        float a = 6.2832f * Hash01Asi(semilla, 92 + k);
        float r = alto * (0.22f + 0.08f * Hash01Asi(semilla, 95 + k));
        Vector3 c = { pie.x + std::cos(a) * alto * 0.12f, pie.y + alto * (0.55f + 0.12f * (float)k), pie.z + std::sin(a) * alto * 0.1f };
        EsferaAsi(c, r, TonoAsi(hoja, 0.94f + 0.1f * (float)k), 10, 3);
    }
}


// Gradas de los eliminados: estan siempre (tambien en calidad baja).
static void DibujarGradasAsi(float t, bool detalle)
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * DISTANCIA_GRADA_ASIENTO;

        if (!DibujarModeloUltimoAsientoRetro3D(MODELO_ASIENTO_GRADA, { x, 0.0f, 0.3f }, lado < 0 ? 180.0f : 0.0f))
        {
            CuboAsi({ x, ALTURA_GRADA_ASIENTO * 0.5f - 0.05f, 0.3f }, { 2.6f, ALTURA_GRADA_ASIENTO + 0.1f, 5.6f }, MADERA_ASI);
            CuboAsi({ x - (float)lado * 1.45f, 0.18f, 0.3f }, { 0.4f, 0.36f, 5.4f }, MADERA_OSCURA_ASI);
        }

        if (detalle)
        {
            // Banderines a lo largo del fondo de la grada.
            float xFondo = x + (float)lado * 1.35f;
            TuboAsi({ xFondo, 0.0f, -2.6f }, { xFondo, 2.4f, -2.6f }, 0.05f, 0.05f, CREMA_ASI, 6, false);
            TuboAsi({ xFondo, 0.0f, 3.2f }, { xFondo, 2.4f, 3.2f }, 0.05f, 0.05f, CREMA_ASI, 6, false);
            BanderinesAsi({ xFondo, 2.35f, -2.6f }, { xFondo, 2.35f, 3.2f }, 0.25f, 9, t, lado < 0 ? 0 : 2);
        }
    }
}


static void DibujarUtileriaAsi(const MinijuegoUltimoAsiento& m, float t, int paso, bool& campana)
{
    // Faroles con guirnaldas de bombillas sobre la mitad trasera del paseo.
    Vector3 ganchos[CANTIDAD_FAROLES_ASI];

    for (int k = 0; k < CANTIDAD_FAROLES_ASI; k++)
    {
        ganchos[k] = FarolAsi(PosicionFarolAsi(k));
    }

    for (int k = 0; k + 1 < CANTIDAD_FAROLES_ASI; k++)
    {
        GuirnaldaAsi(ganchos[k], ganchos[k + 1], 0.55f, 10, k * 3);
    }

    // Puestos de comida y juegos detras de la plaza, con banderines
    // cruzando el paso central.
    for (int k = 0; k < 4; k++)
    {
        PuestoAsi({ X_PUESTOS_ASI[k], 0.0f, Z_PUESTOS_ASI }, k, t);
    }

    BanderinesAsi({ -1.75f, 2.7f, Z_PUESTOS_ASI + 1.0f }, { 1.75f, 2.7f, Z_PUESTOS_ASI + 1.0f }, 0.3f, 7, t, 1);
    GuirnaldaAsi(ganchos[2], { X_PUESTOS_ASI[1] + 1.5f, 2.3f, Z_PUESTOS_ASI + 1.08f }, 0.3f, 5, 2);
    GuirnaldaAsi(ganchos[3], { X_PUESTOS_ASI[2] - 1.5f, 2.3f, Z_PUESTOS_ASI + 1.08f }, 0.3f, 5, 4);

    // Atracciones grandes y juegos sueltos.
    CarpaAsi(CARPA_ASI, t);
    NoriaAsi(t);
    campana = MartilloFuerzaAsi(MARTILLO_ASI, t);
    TaquillaAsi(TAQUILLA_ASI);
    CarritoHeladosAsi(CARRITO_HELADOS_ASI);
    CarritoGlobosAsi(CARRITO_GLOBOS_ASI, t);

    // Bancos, papeleras y maceteros en el paseo.
    BancoAsi({ -7.6f, 0.0f, 7.9f }, -0.78f);
    BancoAsi({ 7.6f, 0.0f, 7.9f }, 0.78f);
    BancoAsi({ -10.4f, 0.0f, -4.6f }, -1.15f);
    BancoAsi({ 10.4f, 0.0f, -4.6f }, 1.15f);
    PapeleraAsi({ -9.3f, 0.0f, 6.6f });
    PapeleraAsi({ 9.3f, 0.0f, 6.6f });
    MaceteroAsi({ -10.6f, 0.0f, -10.4f }, 1);
    MaceteroAsi({ 10.6f, 0.0f, -10.4f }, 2);
    MaceteroAsi({ -14.2f, 0.0f, 3.2f }, 3);
    MaceteroAsi({ 14.2f, 0.0f, 3.2f }, 4);

    // Arboles donde termina la explanada y a los costados.
    int semilla = 0;

    for (float x = -30.0f; x <= 30.0f; x += 2.6f * (float)paso)
    {
        semilla++;
        float z = Z_BORDE_ASI + 0.5f + 0.5f * Hash01Asi(semilla, 101);

        if (std::fabs(x - NORIA_ASI.x) < 2.5f || std::fabs(x) < 1.6f)
        {
            continue;
        }

        ArbolAsi({ x + 0.6f * Hash01Asi(semilla, 102), -0.1f, z }, 3.0f + 1.4f * Hash01Asi(semilla, 103), semilla);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (float z = -11.0f; z <= 14.0f; z += 3.2f * (float)paso)
        {
            semilla++;
            float x = (float)lado * (17.2f + 2.0f * Hash01Asi(semilla, 104));
            ArbolAsi({ x, -0.1f, z }, 2.8f + 1.2f * Hash01Asi(semilla, 105), semilla);
        }
    }

    (void)m;
}


//==================================================
// VISUAL: VIDA AMBIENTAL (notas, burbujas y globos sueltos)
//==================================================

// Notas musicales que suben del carrusel mientras suena el organillo.
static void DibujarNotasAsi(const MinijuegoUltimoAsiento& m, float t)
{
    float musica = m.velocidadCarrusel;

    if (musica < 0.05f)
    {
        return;
    }

    for (int k = 0; k < 6; k++)
    {
        float u = std::fmod(t * 0.42f + (float)k / 6.0f, 1.0f);
        float a = (float)k * 1.05f + u * 1.6f;
        float r = RADIO_TOLDO_ASI * 0.6f + u * 1.2f;
        Vector3 p = { std::cos(a) * r, ALTO_CUSPIDE_ASI + 0.2f + u * 3.0f, std::sin(a) * r };
        float vida = std::sin(u * 3.1416f) * musica;

        if (vida < 0.1f)
        {
            continue;
        }

        float s = 0.18f * vida;
        Color tinta = (k % 2 == 0) ? CORAL_OSCURO_ASI : TURQUESA_ASI;
        CuboAsi({ p.x, p.y, p.z }, { s * 1.3f, s, s }, tinta);
        CuboAsi({ p.x + s * 0.55f, p.y + s * 1.6f, p.z }, { s * 0.22f, s * 3.2f, s * 0.22f }, tinta);
        CuboAsi({ p.x + s * 0.95f, p.y + s * 2.9f, p.z }, { s * 0.8f, s * 0.4f, s * 0.2f }, tinta);
    }
}


// Globos que se escapan del carrito y suben hasta perderse.
static void DibujarGlobosSueltosAsi(float t)
{
    const Color colores[3] = { CORAL_ASI, Color{ 120, 190, 240, 255 }, LIMON_ASI };

    for (int k = 0; k < 3; k++)
    {
        float u = std::fmod(t * 0.07f + (float)k / 3.0f, 1.0f);
        Vector3 p =
        {
            CARRITO_GLOBOS_ASI.x - 1.0f - u * 7.0f + std::sin(t * 0.9f + (float)k) * 0.6f,
            3.2f + u * 15.0f,
            CARRITO_GLOBOS_ASI.z - u * 16.0f
        };
        Color c = TinteAsi(colores[k], p);
        DomoAsi(p, 0.3f, 0.38f, 0.3f, c, 8, 2);
        DomoAsi(p, 0.3f, -0.34f, 0.3f, TonoAsi(c, 0.85f), 8, 2);
        DrawLine3D({ p.x, p.y - 0.34f, p.z }, { p.x + 0.1f, p.y - 1.3f, p.z }, Color{ 240, 240, 244, 200 });
    }
}


// Burbujas de jabon de la maquina del puesto de premios (translucidas).
static void DibujarBurbujasAsi(float t, int cantidad)
{
    Vector3 origen = { X_PUESTOS_ASI[2] + 1.2f, 2.0f, Z_PUESTOS_ASI + 0.9f };

    for (int k = 0; k < cantidad; k++)
    {
        float periodo = 5.0f + 2.0f * Hash01Asi(k, 111);
        float u = std::fmod(t / periodo + Hash01Asi(k, 112), 1.0f);
        float deriva = 2.5f + 3.0f * Hash01Asi(k, 113);
        Vector3 p =
        {
            origen.x - u * deriva * (0.5f + Hash01Asi(k, 114)) + std::sin(t * 1.7f + (float)k) * 0.3f,
            origen.y + u * 3.5f + std::sin(t * 2.3f + (float)k * 2.0f) * 0.2f,
            origen.z + u * deriva
        };
        float r = 0.12f + 0.1f * Hash01Asi(k, 115);
        float alfa = 90.0f * std::sin(u * 3.1416f);
        Color pompa = AlfaAsi(MezclaAsi(Color{ 200, 230, 255, 255 }, ROSA_ASI, Hash01Asi(k, 116)), alfa);
        DomoAsi(p, r, r, r, pompa, 8, 2);
        DomoAsi(p, r, -r, r, pompa, 8, 2);
        CuboAsi({ p.x - r * 0.35f, p.y + r * 0.45f, p.z + r * 0.3f }, { r * 0.25f, r * 0.25f, r * 0.25f }, AlfaAsi(WHITE, alfa * 2.0f));
    }
}


//==================================================
// VISUAL: JUGADORES Y PARTICULAS
//==================================================

static void DibujarJugadoresAsi(
    const MinijuegoUltimoAsiento& m,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    bool mostrarDebug
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];

        if (m.vivo[i] && !jugador.cayendo)
        {
            AnilloAsi({ jugador.posicion.x, SUELO_ASIENTO + 0.03f, jugador.posicion.z }, 0.48f, 0.6f, AlfaAsi(m.coloresJugadores[i], 230.0f), 22);
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = m.coloresJugadores[i];
        DibujarJugadorModeloCompartido(jugador, visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }
}


static void DibujarParticulasVisualesAsi(const MinijuegoUltimoAsiento& m)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_ASIENTO; i++)
    {
        const ParticulaVisualAsiento& p = m.particulasVisuales[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = Limitar01Asi(p.vida / p.vidaMaxima);

        if (p.humo)
        {
            float r = p.tamano * (1.6f - vida);
            Color c = AlfaAsi(p.color, 255.0f);
            EsferaAsi(p.posicion, r, TonoAsi(c, 0.9f + 0.1f * vida), 8, 2);
        }
        else
        {
            float s = p.tamano * Limitar01Asi(p.vida / 0.3f + 0.3f);
            CuboGiradoAsi(p.posicion, { s * 2.0f, s * 0.5f, s * 1.2f }, p.giro, p.color);
        }
    }
}


//==================================================
// VISUAL: LUCES FALSAS (pasada aditiva al final del 3D)
//==================================================

static void DibujarLucesAsi(const MinijuegoUltimoAsiento& m, float t, bool luces, bool decoracion, bool campana)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Carrusel: bombillas del alero y charco tibio sobre las baldosas.
    float encendido = luces ? 1.0f : 0.25f;
    const float yAlero = SUELO_ASIENTO + ALTO_ALERO_ASI;
    float giro = m.anguloCarrusel * DEG2RAD;

    for (int r = 0; r < 3; r++)
    {
        ResplandorSueloAsi({ 0.0f, SUELO_ASIENTO + 0.02f + 0.002f * (float)r, 0.0f }, (3.2f + 1.0f * (float)r), AlfaAsi(Color{ 255, 206, 130, 255 }, 26.0f * encendido));
    }

    if (luces)
    {
        for (int k = 0; k < 24; k += 2)
        {
            float a = giro + 6.2832f * ((float)k + 0.5f) / 24.0f;
            Vector3 p = { std::cos(a) * (RADIO_TOLDO_ASI + 0.08f), yAlero + 0.13f, std::sin(a) * (RADIO_TOLDO_ASI + 0.08f) };
            ResplandorFrenteAsi(p, 0.34f, Color{ 255, 220, 150, 70 });
        }
    }

    // Tazas: las libres llaman con una columna dorada; la trampa, roja.
    if (TazasVisiblesAsi(m))
    {
        for (int c = 0; c < m.cantidadTazas; c++)
        {
            const TazaUltimoAsiento& taza = m.tazas[c];
            float s = AparicionTazaAsi(m, c);

            if (!taza.activa || s <= 0.01f)
            {
                continue;
            }

            Vector3 base = { taza.posicion.x, SUELO_ASIENTO + 0.03f, taza.posicion.z };
            float pulso = 0.75f + 0.25f * std::sin(t * 8.0f + (float)c);

            if (AvisoTrampaAsi(m, c) || (m.subfase == SUBFASE_ULTIMO_ASIENTO_RESOLUCION && taza.trampa))
            {
                ResplandorSueloAsi(base, 2.4f * s, AlfaAsi(TRAMPA_ASI, 90.0f * pulso));
                ColumnaLuzAsi(base, 0.95f * s, 4.5f, AlfaAsi(TRAMPA_ASI, 60.0f * pulso));
            }
            else if (m.subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS && taza.ocupante < 0)
            {
                ResplandorSueloAsi(base, 2.3f * s, AlfaAsi(ACENTO_ASI, 80.0f * pulso));
                ColumnaLuzAsi(base, 0.95f * s, 6.0f, AlfaAsi(ACENTO_ASI, 58.0f * pulso));
            }
            else if (taza.ocupante >= 0)
            {
                ResplandorSueloAsi(base, 2.0f * s, AlfaAsi(m.coloresJugadores[taza.ocupante], 40.0f + 60.0f * taza.destello));
            }
        }
    }

    if (decoracion)
    {
        // Globos de las farolas y bombillas de las guirnaldas (de dia, tenues).
        for (int k = 0; k < CANTIDAD_FAROLES_ASI; k++)
        {
            Vector3 p = PosicionFarolAsi(k);
            ResplandorFrenteAsi({ p.x, 3.3f, p.z }, 0.55f, Color{ 255, 236, 190, 60 });
        }

        if (campana)
        {
            ResplandorFrenteAsi({ MARTILLO_ASI.x, 4.5f, MARTILLO_ASI.z - 0.3f }, 1.2f, Color{ 255, 220, 120, 120 });
        }
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoUltimoAsiento::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{

    int limite = LimiteAsiento(cantidadMaxima);
    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    bool luces = !(fase == FASE_ULTIMO_ASIENTO_JUGANDO && subfase != SUBFASE_ULTIMO_ASIENTO_MUSICA);
    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();

    // Entrada: durante la cuenta la camara baja desde mas alto y gira un poco.
    Camera3D vista = camara;

    if (fase == FASE_ULTIMO_ASIENTO_PREPARACION)
    {
        float u = Limitar01Asi(tiempoPreparacion / DURACION_PREPARACION_ASIENTO);
        float entrada = SuaveAsi(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 6.0f * entrada;
        vista.position.z += 4.0f * entrada;
        vista.position.x += 3.0f * entrada;
    }

    DibujarCieloAsi(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarTerrenoAsi(paso);
    DibujarPlazaAsi();
    DibujarBordeAsi(decoracion);
    DibujarGradasAsi(t, decoracion);

    bool campana = false;

    if (decoracion)
    {
        DibujarUtileriaAsi(*this, t, paso, campana);
    }

    DibujarCarruselAsi(*this, luces, t);
    DibujarTazasAsi(*this, t);
    DibujarJugadoresAsi(*this, jugadores, limite, participantes, mostrarDebug);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_ULTIMO_ASIENTO);
    DibujarParticulasVisualesAsi(*this);

    if (decoracion)
    {
        DibujarNotasAsi(*this, t);
        DibujarGlobosSueltosAsi(t);
    }

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
        }

        for (int c = 0; c < cantidadTazas; c++)
        {
            DrawCircle3D({ tazas[c].posicion.x, 0.1f, tazas[c].posicion.z }, RADIO_ENTRADA_TAZA_ASIENTO, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }
    }

    // Translucidos: burbujas de jabon.
    if (decoracion)
    {
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        DibujarBurbujasAsi(t, CantidadParticulasAsi(14));
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
    }

    if (efectos)
    {
        DibujarLucesAsi(*this, t, luces, decoracion, campana);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre cada jugador (tambien en la grada).
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector2 p = GetWorldToScreen({ jugadores[i].posicion.x, jugadores[i].posicion.y + 1.3f, jugadores[i].posicion.z }, vista);
        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = coloresJugadores[i];
        op.pico = true;
        op.alfa = vivo[i] ? 1.0f : 0.7f;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Anuncios de la ronda en el tercio superior.
    if (fase == FASE_ULTIMO_ASIENTO_JUGANDO)
    {
        float y = alto * 0.25f;
        float cx = ancho * 0.5f;
        float enSubfase = subfaseVista == subfase ? tiempoEnSubfase : 0.0f;

        if (subfase == SUBFASE_ULTIMO_ASIENTO_MUSICA)
        {
            float alfa = Limitar01Asi(enSubfase / 0.25f) * Limitar01Asi((1.8f - enSubfase) / 0.4f);

            if (alfa > 0.01f)
            {
                DibujarTextoMundoContornoUI("¡SUENA LA MÚSICA: A CORRER!", cx, y, UI_TEXTO_GRANDE, CREMA_ASI, alfa);
            }
        }
        else if (subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
        {
            float transcurrido = DURACION_TAZAS_ASIENTO - tiempoSubfase;
            bool aviso = indiceTrampa >= 0 && tiempoSubfase <= DURACION_AVISO_TRAMPA_ASIENTO;

            if (aviso)
            {
                float latido = 0.6f + 0.4f * std::sin(t * 14.0f);
                DibujarTextoMundoContornoUI("¡LA TAZA ROJA ES TRAMPA!", cx, y, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 }, latido);
            }
            else if (transcurrido < 1.4f)
            {
                char texto[48];
                std::snprintf(texto, sizeof(texto), "¡RONDA %d: A UNA TAZA!", ronda);
                float alfa = Limitar01Asi(transcurrido / 0.15f) * Limitar01Asi((1.4f - transcurrido) / 0.3f);
                DibujarTextoMundoContornoUI(texto, cx, y, UI_TEXTO_GRANDE, ACENTO_ASI, alfa);
            }
        }
        else
        {
            // Quienes se quedaron sin taza en esta ronda.
            char texto[64] = "";
            int fuera = 0;

            for (int i = 0; i < limite; i++)
            {
                if (resultado.participantes[i].participo && !vivo[i] && rondaEliminacion[i] == ronda)
                {
                    char nombre[8];
                    std::snprintf(nombre, sizeof(nombre), fuera == 0 ? "J%d" : ", J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);
                    std::strncat(texto, nombre, sizeof(texto) - std::strlen(texto) - 1);
                    fuera++;
                }
            }

            char anuncio[96];

            if (fuera == 0)
            {
                std::snprintf(anuncio, sizeof(anuncio), "¡NADIE QUEDA FUERA!");
            }
            else
            {
                std::snprintf(anuncio, sizeof(anuncio), fuera == 1 ? "¡%s QUEDA FUERA!" : "¡%s QUEDAN FUERA!", texto);
            }

            float u = DURACION_RESOLUCION_ASIENTO - tiempoSubfase;
            float alfa = Limitar01Asi(u / 0.15f) * Limitar01Asi(tiempoSubfase / 0.3f);
            DibujarTextoMundoContornoUI(anuncio, cx, y, UI_TEXTO_GRANDE, fuera == 0 ? MENTA_ASI : Color{ 255, 190, 170, 255 }, alfa);
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "ÚLTIMO ASIENTO",
        "CUANDO PARE LA MÚSICA, OCUPÁ UNA TAZA. ¡EMPUJÁ A LOS RIVALES!",
        resultado
    );

    hud.fase = fase == FASE_ULTIMO_ASIENTO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_ULTIMO_ASIENTO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_ASIENTO;
    hud.finPorTiempo = tiempoRestante <= 0.0f;

    if (fase == FASE_ULTIMO_ASIENTO_TERMINADO && resultado.desenlace == DESENLACE_EMPATE)
    {
        hud.tituloFinal = "¡EMPATE DE SUPERVIVIENTES!";
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        // Rondas superadas: sube al cerrarse cada ronda que sobrevive.
        int superadas = vivo[i]
            ? (subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS ? ronda - 1 : ronda)
            : rondaEliminacion[i] - 1;
        if (superadas < 0) superadas = 0;

        std::snprintf(j.valor, sizeof(j.valor), "%d", superadas);
        j.valorNumerico = superadas;
        j.eliminado = !vivo[i] && fase != FASE_ULTIMO_ASIENTO_PREPARACION;

        if (vivo[i])
        {
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "SOBREVIVE");
        }
        else
        {
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "FUERA EN R%d", rondaEliminacion[i]);
        }

        if (fase == FASE_ULTIMO_ASIENTO_JUGANDO && vivo[i])
        {
            if (subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
            {
                int c = tazaDe[i];
                const char* estado = c < 0 ? "SIN TAZA" : (AvisoTrampaAsi(*this, c) ? "¡TRAMPA!" : "EN TAZA");
                std::snprintf(j.estado, sizeof(j.estado), "%s", estado);
            }
            else if (subfase == SUBFASE_ULTIMO_ASIENTO_RESOLUCION)
            {
                std::snprintf(j.estado, sizeof(j.estado), "¡SALVADO!");
            }
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "MOVÉ: %s  EMPUJÁ: %s\nSALTÁ: %s",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoUltimoAsiento::ObtenerResultado() const
{
    return resultado;
}
