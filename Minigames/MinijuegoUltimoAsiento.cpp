#include "Minigames/MinijuegoUltimoAsiento.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
#include <cstdio>


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


static const char* NombreJugadorAsiento(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static const char* TextoGolpeAsiento(const Participante& participante)
{
    if (participante.esBot)
    {
        return "BOT";
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        return "B";
    }

    if (participante.control == CONTROL_TECLADO_FLECHAS)
    {
        return "SHIFT DER";
    }

    return "E";
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

    camara.position = { 0.0f, 21.0f, 12.5f };
    camara.target = { 0.0f, 0.0f, 0.9f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
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
}


void MinijuegoUltimoAsiento::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
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
// VISUAL: ESCENARIO (solo decoracion, la logica no depende de esto)
//==================================================

// MODELO FUTURO: carrusel central (base, techo, caballitos y postes), tazas
// giratorias, vallas, globos, puestos de feria, montana rusa y noria de
// fondo, gradas de espectadores y arcos de luces: reemplazar cada uno por GLB.

static void DibujarSueloAsiento(const MinijuegoUltimoAsiento& m)
{
    bool luces = m.subfase == SUBFASE_ULTIMO_ASIENTO_MUSICA || m.fase != FASE_ULTIMO_ASIENTO_JUGANDO;

    DrawPlane({ 0.0f, -0.12f, 0.0f }, { 90.0f, 90.0f }, Color{ 34, 28, 52, 255 });

    // Disco de la arena con anillos concentricos de feria.
    DrawCylinder({ 0.0f, SUELO_ASIENTO - 0.45f, 0.0f }, RADIO_ARENA_ASIENTO + 0.5f, RADIO_ARENA_ASIENTO + 0.5f, 0.45f, 48, Color{ 70, 52, 96, 255 });
    DrawCylinder({ 0.0f, SUELO_ASIENTO - 0.02f, 0.0f }, RADIO_ARENA_ASIENTO, RADIO_ARENA_ASIENTO, 0.02f, 48, luces ? Color{ 120, 84, 160, 255 } : Color{ 56, 44, 84, 255 });
    DrawCylinder({ 0.0f, SUELO_ASIENTO - 0.01f, 0.0f }, RADIO_ARENA_ASIENTO * 0.78f, RADIO_ARENA_ASIENTO * 0.78f, 0.02f, 48, luces ? Color{ 98, 66, 140, 255 } : Color{ 46, 36, 70, 255 });
    DrawCylinder({ 0.0f, SUELO_ASIENTO, 0.0f }, RADIO_ARENA_ASIENTO * 0.52f, RADIO_ARENA_ASIENTO * 0.52f, 0.02f, 48, luces ? Color{ 130, 92, 170, 255 } : Color{ 60, 48, 90, 255 });

    // Valla con postes y bombillas en el borde.
    for (int k = 0; k < 32; k++)
    {
        float angulo = (float)k * 2.0f * PI / 32.0f;
        float x = std::cos(angulo) * (RADIO_ARENA_ASIENTO + 0.15f);
        float z = std::sin(angulo) * (RADIO_ARENA_ASIENTO + 0.15f);

        DrawCylinder({ x, SUELO_ASIENTO, z }, 0.07f, 0.07f, 0.8f, 6, Color{ 235, 235, 240, 255 });

        Color bombilla = Color{ 50, 50, 64, 255 };

        if (luces)
        {
            float tono = std::fmod((float)k * 11.25f + m.anguloCarrusel * 3.0f, 360.0f);
            bombilla = ColorFromHSV(tono, 0.8f, 1.0f);
        }

        DrawSphereEx({ x, SUELO_ASIENTO + 0.9f, z }, 0.13f, 6, 6, bombilla);
    }
}


static void DibujarCarruselAsiento(const MinijuegoUltimoAsiento& m)
{
    bool luces = m.subfase == SUBFASE_ULTIMO_ASIENTO_MUSICA || m.fase != FASE_ULTIMO_ASIENTO_JUGANDO;
    float giro = m.anguloCarrusel * DEG2RAD;

    DrawCylinder({ 0.0f, SUELO_ASIENTO, 0.0f }, RADIO_CARRUSEL_VISUAL_ASIENTO, RADIO_CARRUSEL_VISUAL_ASIENTO, 0.45f, 24, Color{ 200, 60, 90, 255 });
    DrawCylinder({ 0.0f, SUELO_ASIENTO + 0.45f, 0.0f }, RADIO_CARRUSEL_VISUAL_ASIENTO + 0.1f, RADIO_CARRUSEL_VISUAL_ASIENTO + 0.1f, 0.08f, 24, Color{ 240, 210, 90, 255 });
    DrawCylinder({ 0.0f, SUELO_ASIENTO + 0.5f, 0.0f }, 0.35f, 0.35f, 2.9f, 10, Color{ 250, 240, 220, 255 });
    DrawCylinder({ 0.0f, SUELO_ASIENTO + 3.2f, 0.0f }, 0.15f, RADIO_CARRUSEL_VISUAL_ASIENTO + 0.4f, 1.0f, 24, Color{ 70, 130, 230, 255 });
    DrawSphereEx({ 0.0f, SUELO_ASIENTO + 4.35f, 0.0f }, 0.22f, 8, 8, GOLD);

    for (int k = 0; k < 6; k++)
    {
        float angulo = giro + (float)k * 2.0f * PI / 6.0f;
        float x = std::cos(angulo) * 1.55f;
        float z = std::sin(angulo) * 1.55f;
        float vaiven = std::sin(m.tiempoAnimacion * 4.0f + (float)k) * 0.12f * m.velocidadCarrusel;
        Color colorCaballo = (k % 2 == 0) ? Color{ 250, 250, 250, 255 } : Color{ 255, 180, 60, 255 };

        DrawCylinder({ x, SUELO_ASIENTO + 0.5f, z }, 0.05f, 0.05f, 2.7f, 6, GOLD);
        DrawCube({ x, SUELO_ASIENTO + 1.2f + vaiven, z }, 0.5f, 0.45f, 0.5f, colorCaballo);
        DrawCube({ x + std::cos(angulo + 1.57f) * 0.05f, SUELO_ASIENTO + 1.6f + vaiven, z }, 0.25f, 0.3f, 0.25f, colorCaballo);
    }

    for (int k = 0; k < 12; k++)
    {
        float angulo = giro + (float)k * 2.0f * PI / 12.0f;
        Color bombilla = Color{ 50, 50, 64, 255 };

        if (luces)
        {
            bombilla = ColorFromHSV(std::fmod((float)k * 30.0f + m.anguloCarrusel * 4.0f, 360.0f), 0.8f, 1.0f);
        }

        DrawSphereEx(
            { std::cos(angulo) * (RADIO_CARRUSEL_VISUAL_ASIENTO + 0.3f), SUELO_ASIENTO + 3.25f, std::sin(angulo) * (RADIO_CARRUSEL_VISUAL_ASIENTO + 0.3f) },
            0.12f, 6, 6, bombilla
        );
    }
}


static void DibujarFondoAsiento(const MinijuegoUltimoAsiento& m)
{
    // Montana rusa al fondo.
    const int puntos = 21;
    Vector3 anterior{};

    for (int p = 0; p < puntos; p++)
    {
        float t = (float)p / (float)(puntos - 1);
        float x = -21.0f + 42.0f * t;
        float y = 5.0f + 3.2f * std::sin(t * 9.0f) + 1.6f * std::sin(t * 23.0f);
        Vector3 actual = { x, y, -17.0f };

        DrawCube({ x, y * 0.5f - 0.1f, -17.0f }, 0.22f, y, 0.22f, Color{ 190, 190, 205, 255 });

        if (p > 0)
        {
            DrawCylinderEx({ anterior.x, anterior.y, anterior.z - 0.35f }, { actual.x, actual.y, actual.z - 0.35f }, 0.1f, 0.1f, 5, Color{ 235, 70, 70, 255 });
            DrawCylinderEx({ anterior.x, anterior.y, anterior.z + 0.35f }, { actual.x, actual.y, actual.z + 0.35f }, 0.1f, 0.1f, 5, Color{ 235, 70, 70, 255 });
        }

        anterior = actual;
    }

    float tCarro = std::fmod(m.tiempoAnimacion * 0.08f, 1.0f);
    DrawCube(
        { -21.0f + 42.0f * tCarro, 5.0f + 3.2f * std::sin(tCarro * 9.0f) + 1.6f * std::sin(tCarro * 23.0f) + 0.35f, -17.0f },
        1.1f, 0.5f, 0.8f, Color{ 255, 220, 60, 255 }
    );

    // Noria.
    const Vector3 centroNoria = { 15.0f, 6.4f, -12.5f };
    float giroNoria = m.tiempoAnimacion * 0.25f;

    DrawCylinderEx({ centroNoria.x - 1.6f, 0.0f, centroNoria.z }, centroNoria, 0.16f, 0.16f, 6, Color{ 210, 210, 225, 255 });
    DrawCylinderEx({ centroNoria.x + 1.6f, 0.0f, centroNoria.z }, centroNoria, 0.16f, 0.16f, 6, Color{ 210, 210, 225, 255 });

    for (int k = 0; k < 8; k++)
    {
        float angulo = giroNoria + (float)k * 2.0f * PI / 8.0f;
        float siguiente = giroNoria + (float)(k + 1) * 2.0f * PI / 8.0f;
        Vector3 borde = { centroNoria.x + std::cos(angulo) * 5.0f, centroNoria.y + std::sin(angulo) * 5.0f, centroNoria.z };
        Vector3 bordeSiguiente = { centroNoria.x + std::cos(siguiente) * 5.0f, centroNoria.y + std::sin(siguiente) * 5.0f, centroNoria.z };

        DrawCylinderEx(centroNoria, borde, 0.07f, 0.07f, 5, Color{ 240, 110, 190, 255 });
        DrawCylinderEx(borde, bordeSiguiente, 0.09f, 0.09f, 5, Color{ 240, 110, 190, 255 });
        DrawCube({ borde.x, borde.y - 0.45f, borde.z }, 0.8f, 0.7f, 0.7f, ColorFromHSV((float)k * 45.0f, 0.7f, 1.0f));
    }

    // Puestos de feria con techo a rayas.
    for (int k = 0; k < 3; k++)
    {
        float x = -15.0f + (float)k * 5.2f;
        DrawCube({ x, 0.9f, -11.0f }, 3.4f, 1.8f, 1.8f, Color{ 120, 80, 60, 255 });

        for (int franja = 0; franja < 6; franja++)
        {
            Color colorFranja = (franja % 2 == 0) ? Color{ 235, 60, 70, 255 } : Color{ 250, 250, 250, 255 };
            DrawCube({ x - 1.4f + (float)franja * 0.56f, 2.05f, -11.0f }, 0.56f, 0.5f, 2.2f, colorFranja);
        }
    }

    // Globos flotando.
    for (int k = 0; k < 14; k++)
    {
        float angulo = PI + 0.15f + (float)k * (PI - 0.3f) / 13.0f;
        float radio = RADIO_ARENA_ASIENTO + 1.5f + (float)(k % 3) * 0.6f;
        float vaiven = std::sin(m.tiempoAnimacion * 1.5f + (float)k) * 0.25f;
        Vector3 base = { std::cos(angulo) * radio, 0.1f, std::sin(angulo) * radio };
        Vector3 globo = { base.x, 2.6f + (float)(k % 4) * 0.4f + vaiven, base.z };

        DrawLine3D(base, globo, Color{ 230, 230, 235, 255 });
        DrawSphereEx(globo, 0.4f, 8, 8, ColorFromHSV((float)k * 26.0f, 0.75f, 1.0f));
    }

    // Gradas de espectadores.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube({ (float)lado * DISTANCIA_GRADA_ASIENTO, ALTURA_GRADA_ASIENTO * 0.5f - 0.05f, 0.3f }, 2.6f, ALTURA_GRADA_ASIENTO + 0.1f, 5.6f, Color{ 150, 100, 70, 255 });
        DrawCubeWires({ (float)lado * DISTANCIA_GRADA_ASIENTO, ALTURA_GRADA_ASIENTO * 0.5f - 0.05f, 0.3f }, 2.6f, ALTURA_GRADA_ASIENTO + 0.1f, 5.6f, Color{ 90, 60, 40, 255 });
    }
}


static Color ColorTazaAsiento(const MinijuegoUltimoAsiento& m, int c)
{
    const TazaUltimoAsiento& taza = m.tazas[c];

    if (m.subfase == SUBFASE_ULTIMO_ASIENTO_RESOLUCION)
    {
        return taza.trampa ? Color{ 200, 50, 50, 255 } : Color{ 90, 90, 100, 255 };
    }

    if (
        taza.trampa &&
        m.tiempoSubfase <= DURACION_AVISO_TRAMPA_ASIENTO
    )
    {
        bool encendida = std::fmod(m.tiempoAnimacion * 8.0f, 1.0f) < 0.5f;
        return encendida ? Color{ 255, 40, 40, 255 } : Color{ 255, 255, 255, 255 };
    }

    if (taza.ocupante >= 0)
    {
        return m.coloresJugadores[taza.ocupante];
    }

    float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 9.0f);
    return Color{ 255, (unsigned char)(215 + 40.0f * pulso), (unsigned char)(90 + 120.0f * pulso), 255 };
}


static void DibujarTazasAsiento(const MinijuegoUltimoAsiento& m)
{
    if (m.fase != FASE_ULTIMO_ASIENTO_JUGANDO || m.subfase == SUBFASE_ULTIMO_ASIENTO_MUSICA)
    {
        return;
    }

    for (int c = 0; c < m.cantidadTazas; c++)
    {
        const TazaUltimoAsiento& taza = m.tazas[c];

        if (!taza.activa)
        {
            continue;
        }

        Color color = ColorTazaAsiento(m, c);
        Vector3 centro = { taza.posicion.x, SUELO_ASIENTO + 0.03f, taza.posicion.z };
        float extra = taza.destello * 0.25f;

        DrawCylinder(centro, RADIO_TAZA_ASIENTO, RADIO_TAZA_ASIENTO, 0.03f, 28, Fade(color, 0.55f));
        DrawCylinder({ centro.x, centro.y + 0.03f, centro.z }, RADIO_TAZA_ASIENTO * 0.6f, RADIO_TAZA_ASIENTO * 0.6f, 0.03f, 24, Fade(WHITE, 0.35f));
        DrawCircle3D({ centro.x, centro.y + 0.07f, centro.z }, RADIO_TAZA_ASIENTO + extra, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
        DrawCircle3D({ centro.x, centro.y + 0.07f, centro.z }, RADIO_TAZA_ASIENTO - 0.12f, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
        DrawCylinderWires({ centro.x, centro.y, centro.z }, RADIO_TAZA_ASIENTO, RADIO_TAZA_ASIENTO * 0.8f, 2.4f, 16, Fade(color, 0.7f));
    }
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
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    bool oscuro =
        fase == FASE_ULTIMO_ASIENTO_JUGANDO &&
        subfase != SUBFASE_ULTIMO_ASIENTO_MUSICA;

    ClearBackground(oscuro ? Color{ 20, 14, 40, 255 } : Color{ 58, 34, 102, 255 });
    BeginMode3D(camara);

    DibujarSueloAsiento(*this);
    DibujarFondoAsiento(*this);
    DibujarCarruselAsiento(*this);
    DibujarTazasAsiento(*this);

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];

        if (vivo[i])
        {
            DrawCircle3D({ jugador.posicion.x, 0.03f, jugador.posicion.z }, 0.62f, { 1.0f, 0.0f, 0.0f }, 90.0f, coloresJugadores[i]);
            DrawCircle3D({ jugador.posicion.x, 0.03f, jugador.posicion.z }, 0.52f, { 1.0f, 0.0f, 0.0f }, 90.0f, WHITE);
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = coloresJugadores[i];
        DibujarJugadorCuboPrueba(jugador, visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_ULTIMO_ASIENTO);

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

    EndMode3D();

    // Etiquetas flotantes.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.35f, jugadores[i].posicion.z },
            camara
        );
        const char* nombre = NombreJugadorAsiento(participantes[i], i);
        int ancho = MeasureText(nombre, 18);

        DrawRectangle((int)pantalla.x - ancho / 2 - 4, (int)pantalla.y - 2, ancho + 8, 22, Fade(BLACK, 0.55f));
        DrawText(nombre, (int)pantalla.x - ancho / 2, (int)pantalla.y, 18, vivo[i] ? coloresJugadores[i] : GRAY);
    }

    int vivos = ContarVivosAsiento(*this, limite);

    // Cabecera.
    DrawRectangle(14, 12, 520, 92, Fade(BLACK, 0.74f));
    DrawText("ULTIMO ASIENTO", 28, 20, 28, GOLD);
    DrawText(TextFormat("VIVOS: %d    RONDA: %d", vivos, ronda > 0 ? ronda : 1), 28, 52, 18, RAYWHITE);
    DrawText("MOVER: WASD/FLECHAS/STICK   SALTO: ESPACIO/ENTER/A", 28, 74, 14, LIGHTGRAY);

    if (fase == FASE_ULTIMO_ASIENTO_JUGANDO)
    {
        DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
        DrawText(
            TextFormat("TIEMPO %.1f", tiempoRestante),
            anchoPantalla - 200,
            22,
            26,
            tiempoRestante <= 6.0f ? RED : GOLD
        );

        const char* aviso = "SUENA EL ORGANILLO: CORRE Y PREPARATE";
        Color colorAviso = RAYWHITE;

        if (subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS)
        {
            aviso = TextFormat("A UNA TAZA!  CIERRAN EN %.1f", tiempoSubfase > 0.0f ? tiempoSubfase : 0.0f);
            colorAviso = YELLOW;

            if (indiceTrampa >= 0 && tiempoSubfase <= DURACION_AVISO_TRAMPA_ASIENTO)
            {
                aviso = "CUIDADO: LA TAZA ROJA ES TRAMPA!";
                colorAviso = RED;
            }
        }
        else if (subfase == SUBFASE_ULTIMO_ASIENTO_RESOLUCION)
        {
            aviso = "RONDA TERMINADA";
            colorAviso = ORANGE;
        }

        int a = MeasureText(aviso, 26);
        DrawRectangle(anchoPantalla / 2 - a / 2 - 14, 112, a + 28, 40, Fade(BLACK, 0.8f));
        DrawText(aviso, anchoPantalla / 2 - a / 2, 120, 26, colorAviso);
    }

    // Tarjetas inferiores.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo) activos++;
    }

    if (activos > 0)
    {
        int separacion = 10;
        int anchoTarjeta = (anchoPantalla - 28 - separacion * (activos - 1)) / activos;

        if (anchoTarjeta > 300) anchoTarjeta = 300;

        int xInicial = (anchoPantalla - (anchoTarjeta * activos + separacion * (activos - 1))) / 2;
        int k = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            int x = xInicial + k * (anchoTarjeta + separacion);
            int y = altoPantalla - 92;

            DrawRectangle(x, y, anchoTarjeta, 80, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 80, coloresJugadores[i]);
            DrawText(NombreJugadorAsiento(participantes[i], i), x + 18, y + 8, 20, coloresJugadores[i]);

            const char* estado = "EN JUEGO";
            Color colorEstado = RAYWHITE;

            if (!vivo[i])
            {
                estado = TextFormat("ELIMINADO R%d", rondaEliminacion[i]);
                colorEstado = GRAY;
            }
            else if (subfase == SUBFASE_ULTIMO_ASIENTO_TAZAS && tazaDe[i] >= 0)
            {
                estado = "EN TAZA";
                colorEstado = LIME;
            }

            DrawText(estado, x + 18, y + 34, 18, colorEstado);

            if (participantes[i].esBot)
            {
                DrawText("CONTROL: BOT", x + 18, y + 58, 14, LIGHTGRAY);
            }
            else
            {
                DrawText(
                    TextFormat("EMPUJAR: %s   SALTO: %s", TextoGolpeAsiento(participantes[i]), ObtenerTextoBotonPrincipal(participantes[i])),
                    x + 18,
                    y + 58,
                    14,
                    LIGHTGRAY
                );
            }

            k++;
        }
    }

    if (fase == FASE_ULTIMO_ASIENTO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 90, 96, GOLD);

        const char* ayuda = "CUANDO PARE LA MUSICA OCUPA UNA TAZA. EMPUJA A LOS RIVALES!";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 + 20, 20, RAYWHITE);
    }
    else if (
        fase == FASE_ULTIMO_ASIENTO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 20;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE DE SUPERVIVIENTES";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorAsiento(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (
                    !resultado.participantes[i].participo ||
                    resultado.participantes[i].posicionFinal != posicion
                )
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %s",
                        posicion,
                        NombreJugadorAsiento(participantes[i], i),
                        vivo[i] ? "SOBREVIVE" : TextFormat("ELIMINADO EN LA RONDA %d", rondaEliminacion[i])
                    ),
                    px + 40,
                    py + 62 + fila * 30,
                    22,
                    coloresJugadores[i]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            anchoPantalla / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoUltimoAsiento::ObtenerResultado() const
{
    return resultado;
}
