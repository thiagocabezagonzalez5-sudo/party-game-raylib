#include "Minigames/MinijuegoBalsasRapido.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <vector>


//==================================================
// CONSTANTES (REGLAS Y RIO)
//==================================================
//
// El rio avanza hacia -Z en el mundo. Internamente cada balsa usa "p"
// (avance, 0 a LONGITUD_META) y "x" (lateral); el mundo es (x, y, -p).

static const float DURACION_PREPARACION_BALSAS = 3.0f;
static const float DURACION_CARRERA_BALSAS = 70.0f;
static const float LONGITUD_META = 120.0f;

static const float CENTRO_CARRIL = 8.0f;
static const float MEDIO_ANCHO_CARRIL = 5.6f;
static const float MEDIO_ANCHO_ISLA = 2.4f;
static const float RADIO_BALSA = 0.85f;
static const float LIMITE_LATERAL = 4.75f;

static const float RAPIDO_INICIO = 52.0f;
static const float RAPIDO_FIN = 82.0f;
static const float DESVIO_CASCADA = -3.2f;
static const float DESVIO_REMANSO = 3.1f;

static const float IMPULSO_PALADA = 0.5f;
static const float BONUS_SINCRONIA = 0.55f;
static const float VENTANA_SINCRONIA = 0.25f;
static const float TORQUE_PALADA = 1.3f;
static const float RECARGA_PALADA = 0.17f;
static const float ARRASTRE_LINEAL = 0.35f;
static const float ARRASTRE_CUADRATICO = 0.09f;
static const float VELOCIDAD_MAXIMA = 10.0f;
static const float VELOCIDAD_MAXIMA_BOOST = 12.0f;
static const float MULTIPLICADOR_MINORIA = 1.12f;

static const float CORRIENTE_BASE = 1.0f;
static const float CORRIENTE_CASCADA = 3.0f;
static const float CORRIENTE_REMANSO = -1.0f;
static const float DURACION_BOOST = 2.2f;
static const float CORRIENTE_BOOST = 2.5f;

static const Color COLOR_EQUIPO_0 = { 255, 150, 40, 255 };
static const Color COLOR_EQUIPO_1 = { 240, 80, 170, 255 };


//==================================================
// UTILIDADES
//==================================================

static float Acotar(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float Aleatorio01()
{
    return (float)GetRandomValue(0, 10000) / 10000.0f;
}


static bool JugadorEsBot(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static Color ColorEquipoBalsas(int equipo)
{
    return equipo == 0 ? COLOR_EQUIPO_0 : COLOR_EQUIPO_1;
}


static float CentroCarril(int equipo)
{
    return equipo == 0 ? -CENTRO_CARRIL : CENTRO_CARRIL;
}


static unsigned int HashIndice(int k)
{
    return (((unsigned int)k * 2654435761u) >> 8) & 0xFFu;
}


static bool EsMinoria(const MinijuegoBalsasRapido& m, int equipo)
{
    int otro = equipo == 0 ? 1 : 0;

    return m.cantidadJugadoresEquipo[equipo] < m.cantidadJugadoresEquipo[otro];
}


//==================================================
// DATOS LOGICOS: RECORRIDO (mismo para los dos carriles)
//==================================================

static void AgregarObstaculo(
    MinijuegoBalsasRapido& m,
    TipoObstaculoBalsas tipo,
    float p,
    float desvio,
    float a,
    float b
)
{
    if (m.cantidadObstaculos >= MAX_OBSTACULOS_BALSAS)
    {
        return;
    }

    ObstaculoBalsas& o = m.obstaculos[m.cantidadObstaculos++];
    o.tipo = tipo;
    o.p = p;
    o.desvio = desvio;
    o.a = a;
    o.b = b;
}


static void CrearRecorrido(MinijuegoBalsasRapido& m)
{
    m.cantidadObstaculos = 0;

    const TipoObstaculoBalsas R = OBSTACULO_BALSAS_ROCA;
    const TipoObstaculoBalsas T = OBSTACULO_BALSAS_TRONCO;
    const TipoObstaculoBalsas V = OBSTACULO_BALSAS_REMOLINO;
    const TipoObstaculoBalsas B = OBSTACULO_BALSAS_BANANA;

    // Tramo inicial.
    AgregarObstaculo(m, R, 14.0f, -2.4f, 0.9f, 0.0f);
    AgregarObstaculo(m, R, 17.0f, 2.8f, 0.9f, 0.0f);
    AgregarObstaculo(m, B, 20.0f, 0.4f, 0.5f, 0.0f);
    AgregarObstaculo(m, T, 25.0f, 1.4f, 1.7f, 0.5f);
    AgregarObstaculo(m, V, 30.0f, -2.2f, 2.0f, 0.0f);
    AgregarObstaculo(m, R, 31.0f, 2.6f, 0.9f, 0.0f);
    AgregarObstaculo(m, B, 35.0f, -1.2f, 0.5f, 0.0f);
    AgregarObstaculo(m, R, 38.0f, 0.6f, 0.9f, 0.0f);
    AgregarObstaculo(m, T, 43.0f, -2.4f, 1.7f, 0.5f);
    AgregarObstaculo(m, V, 46.0f, 2.4f, 2.0f, 0.0f);
    AgregarObstaculo(m, B, 49.0f, 0.0f, 0.5f, 0.0f);

    // Rapido: isla divisora; a la izquierda cascada corta con rocas
    // en zigzag, a la derecha remanso largo y seguro.
    AgregarObstaculo(m, OBSTACULO_BALSAS_DIVISOR, 0.5f * (RAPIDO_INICIO + RAPIDO_FIN), 0.0f, 0.7f, 0.5f * (RAPIDO_FIN - RAPIDO_INICIO));
    AgregarObstaculo(m, R, 57.0f, -2.5f, 0.5f, 0.0f);
    AgregarObstaculo(m, R, 63.0f, -3.9f, 0.5f, 0.0f);
    AgregarObstaculo(m, R, 69.0f, -2.5f, 0.5f, 0.0f);
    AgregarObstaculo(m, R, 75.0f, -3.9f, 0.5f, 0.0f);
    AgregarObstaculo(m, B, 66.0f, -3.1f, 0.5f, 0.0f);
    AgregarObstaculo(m, B, 60.0f, 3.1f, 0.5f, 0.0f);
    AgregarObstaculo(m, B, 72.0f, 3.1f, 0.5f, 0.0f);

    // Tramo final.
    AgregarObstaculo(m, B, 86.0f, 0.0f, 0.5f, 0.0f);
    AgregarObstaculo(m, R, 90.0f, -2.5f, 0.9f, 0.0f);
    AgregarObstaculo(m, T, 94.0f, 1.8f, 1.7f, 0.5f);
    AgregarObstaculo(m, V, 98.0f, -1.8f, 2.0f, 0.0f);
    AgregarObstaculo(m, R, 100.0f, 2.4f, 0.9f, 0.0f);
    AgregarObstaculo(m, B, 104.0f, 2.4f, 0.5f, 0.0f);
    AgregarObstaculo(m, T, 108.0f, -1.5f, 1.7f, 0.5f);
    AgregarObstaculo(m, R, 112.0f, 0.9f, 0.9f, 0.0f);
    AgregarObstaculo(m, V, 114.0f, 2.0f, 2.0f, 0.0f);
}


// Corriente del agua en el avance p para una balsa a "desvio" del centro.
static float CorrienteRio(float p, float desvio)
{
    float peso =
        Acotar((p - RAPIDO_INICIO) / 3.0f, 0.0f, 1.0f) *
        Acotar((RAPIDO_FIN - p) / 3.0f, 0.0f, 1.0f);
    float t = Acotar(desvio / 1.2f, -1.0f, 1.0f);
    float extra = t < 0.0f
        ? -t * CORRIENTE_CASCADA
        : t * CORRIENTE_REMANSO;

    return CORRIENTE_BASE + peso * extra;
}


//==================================================
// PARTICULAS
//==================================================

static void EmitirSalpicadura(
    MinijuegoBalsasRapido& m,
    float x,
    float p,
    int cantidad,
    float fuerza,
    Color color
)
{
    for (int k = 0; k < cantidad; k++)
    {
        for (int i = 0; i < MAX_PARTICULAS_BALSAS; i++)
        {
            ParticulaTierra& particula = m.particulas[i];

            if (particula.activa)
            {
                continue;
            }

            particula.activa = true;
            particula.posicion = { x, 0.2f, -p };
            particula.velocidad =
            {
                (Aleatorio01() - 0.5f) * fuerza,
                1.0f + Aleatorio01() * fuerza * 0.5f,
                (Aleatorio01() - 0.5f) * fuerza
            };
            particula.vidaMaxima = 0.4f + Aleatorio01() * 0.3f;
            particula.vida = particula.vidaMaxima;
            particula.tamano = 0.07f + Aleatorio01() * 0.06f;
            particula.color = color;
            break;
        }
    }
}


//==================================================
// FISICA DE LA BALSA
//==================================================

// Una palada en un lado (0 izquierdo, 1 derecho). fuerza escala el empuje.
static void Palada(
    MinijuegoBalsasRapido& m,
    BalsaRio& b,
    int lado,
    float fuerza,
    bool humano
)
{
    float multiplicador = EsMinoria(m, b.equipo) ? MULTIPLICADOR_MINORIA : 1.0f;
    int otro = 1 - lado;

    b.velocidad += IMPULSO_PALADA * fuerza * multiplicador;

    // Pala izquierda empuja el agua y la proa gira hacia la derecha.
    b.omega += (lado == 0 ? TORQUE_PALADA : -TORQUE_PALADA) * (fuerza > 1.0f ? 0.8f : 1.0f);

    if (b.paladaSinPareja[otro] && b.tiempoPalada[otro] < VENTANA_SINCRONIA)
    {
        b.velocidad += BONUS_SINCRONIA * multiplicador;
        b.paladaSinPareja[otro] = false;
        b.paladaSinPareja[lado] = false;
        b.omega *= 0.5f;
        b.tiempoSync = 0.7f;
    }
    else
    {
        b.paladaSinPareja[lado] = true;
    }

    b.tiempoPalada[lado] = 0.0f;

    float lateral = lado == 0 ? -0.9f : 0.9f;
    EmitirSalpicadura(
        m,
        b.x + std::cos(b.rumbo) * lateral,
        b.p,
        3,
        2.2f,
        Color{ 210, 245, 250, 255 }
    );

    if (humano && m.tiempoSonidoPalada <= 0.0f)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
        m.tiempoSonidoPalada = 0.1f;
    }
}


static void AplicarImpacto(MinijuegoBalsasRapido& m, BalsaRio& b, float nx)
{
    if (b.tiempoImpacto > 0.0f)
    {
        return;
    }

    b.tiempoImpacto = 0.45f;
    b.velocidad *= 0.35f;
    b.omega += nx * 1.4f;
    ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
    EmitirSalpicadura(m, b.x, b.p, 8, 4.0f, Color{ 230, 250, 255, 255 });
}


static void ResolverObstaculos(MinijuegoBalsasRapido& m, BalsaRio& b, float deltaTime)
{
    for (int k = 0; k < m.cantidadObstaculos; k++)
    {
        const ObstaculoBalsas& o = m.obstaculos[k];
        float ox = b.centroCarril + o.desvio;
        float dx = b.x - ox;
        float dp = b.p - o.p;

        if (o.tipo == OBSTACULO_BALSAS_BANANA)
        {
            if (!b.recogidas[k] && dx * dx + dp * dp < 1.3f * 1.3f)
            {
                b.recogidas[k] = true;
                b.bananas++;
                b.tiempoBoost = DURACION_BOOST;
                b.velocidad += 1.2f;
                ReproducirSonidoMinijuego(m.audio, SONIDO_RECOGER_OBJETO);
                EmitirSalpicadura(m, b.x, b.p, 8, 3.0f, Color{ 255, 225, 60, 255 });
            }

            continue;
        }

        if (o.tipo == OBSTACULO_BALSAS_REMOLINO)
        {
            float alcance = o.a + 0.3f;
            float d = std::sqrt(dx * dx + dp * dp);

            if (d < alcance)
            {
                float signo = o.desvio > 0.0f ? -1.0f : 1.0f;
                b.omega += signo * 3.0f * (1.3f - d / alcance) * deltaTime;
                b.velocidad *= std::exp(-0.8f * deltaTime);
            }

            continue;
        }

        float nx = 0.0f;
        float np = -1.0f;
        float penetracion = 0.0f;

        if (o.tipo == OBSTACULO_BALSAS_ROCA)
        {
            float minimo = o.a + RADIO_BALSA;
            float d = std::sqrt(dx * dx + dp * dp);

            if (d >= minimo)
            {
                continue;
            }

            if (d > 0.0001f)
            {
                nx = dx / d;
                np = dp / d;
            }

            penetracion = minimo - d;
        }
        else
        {
            // Rectangulo (tronco o isla divisora) contra circulo.
            float cx = Acotar(b.x, ox - o.a, ox + o.a);
            float cp = Acotar(b.p, o.p - o.b, o.p + o.b);
            float rx = b.x - cx;
            float rp = b.p - cp;
            float d = std::sqrt(rx * rx + rp * rp);

            if (d >= RADIO_BALSA)
            {
                continue;
            }

            if (d > 0.0001f)
            {
                nx = rx / d;
                np = rp / d;
                penetracion = RADIO_BALSA - d;
            }
            else
            {
                float px = o.a + RADIO_BALSA - std::fabs(dx);
                float pp = o.b + RADIO_BALSA - std::fabs(dp);

                if (px < pp)
                {
                    nx = dx >= 0.0f ? 1.0f : -1.0f;
                    np = 0.0f;
                    penetracion = px;
                }
                else
                {
                    nx = 0.0f;
                    np = dp >= 0.0f ? 1.0f : -1.0f;
                    penetracion = pp;
                }
            }
        }

        b.x += nx * penetracion;
        b.p += np * penetracion;
        b.velocidad *= std::exp(-5.0f * deltaTime);
        AplicarImpacto(m, b, nx);
    }
}


static void ActualizarBalsa(MinijuegoBalsasRapido& m, BalsaRio& b, float deltaTime)
{
    for (int s = 0; s < 2; s++)
    {
        b.tiempoPalada[s] = Acotar(b.tiempoPalada[s] + deltaTime, 0.0f, 9.0f);
    }

    if (b.tiempoImpacto > 0.0f) b.tiempoImpacto -= deltaTime;
    if (b.tiempoSync > 0.0f) b.tiempoSync -= deltaTime;
    if (b.tiempoBoost > 0.0f) b.tiempoBoost -= deltaTime;

    // Rumbo: amortiguado y con tendencia a enderezarse; en solitario hay
    // ayuda de estabilidad extra.
    float asistencia = b.cantidad <= 1 ? 1.0f : 0.0f;
    float restauracion = 0.9f + 1.6f * asistencia;
    float amortiguacion = 2.8f + 2.4f * asistencia;

    b.omega -= b.rumbo * restauracion * deltaTime;
    b.omega *= std::exp(-amortiguacion * deltaTime);
    b.omega = Acotar(b.omega, -3.0f, 3.0f);
    b.rumbo += b.omega * deltaTime;

    if (b.rumbo > 0.95f || b.rumbo < -0.95f)
    {
        b.rumbo = Acotar(b.rumbo, -0.95f, 0.95f);
        b.omega = 0.0f;
    }

    // Arrastre del agua.
    b.velocidad -=
        (ARRASTRE_LINEAL * b.velocidad + ARRASTRE_CUADRATICO * b.velocidad * b.velocidad) *
        deltaTime;
    float tope = b.tiempoBoost > 0.0f ? VELOCIDAD_MAXIMA_BOOST : VELOCIDAD_MAXIMA;
    b.velocidad = Acotar(b.velocidad, 0.0f, tope);

    float corriente = CorrienteRio(b.p, b.x - b.centroCarril);

    if (b.tiempoBoost > 0.0f)
    {
        corriente += CORRIENTE_BOOST;
    }

    b.x += b.velocidad * std::sin(b.rumbo) * deltaTime;
    b.p += (b.velocidad * std::cos(b.rumbo) + corriente) * deltaTime;

    ResolverObstaculos(m, b, deltaTime);

    // Orillas del carril.
    float desvio = b.x - b.centroCarril;

    if (desvio > LIMITE_LATERAL || desvio < -LIMITE_LATERAL)
    {
        float signo = desvio > 0.0f ? 1.0f : -1.0f;
        b.x = b.centroCarril + signo * LIMITE_LATERAL;
        b.omega -= signo * 2.5f * deltaTime;
        b.velocidad *= std::exp(-1.5f * deltaTime);
    }

    if (b.p < 0.0f)
    {
        b.p = 0.0f;
    }

    if (b.p >= LONGITUD_META)
    {
        b.terminada = true;
    }
}


//==================================================
// IA DE BOTS
//==================================================

// Calcula hacia que x quiere ir el equipo: ruta elegida en el rapido,
// esquivando rocas, troncos y remolinos, y atraida por las bananas.
static void CalcularObjetivoBot(
    const MinijuegoBalsasRapido& m,
    BalsaRio& b,
    float deltaTime
)
{
    b.tiempoRuido -= deltaTime;

    if (b.tiempoRuido <= 0.0f)
    {
        b.ruido = (Aleatorio01() - 0.5f) * 1.0f;
        b.tiempoRuido = 1.2f + Aleatorio01();
    }

    float minimo = b.centroCarril - LIMITE_LATERAL + 0.2f;
    float maximo = b.centroCarril + LIMITE_LATERAL - 0.2f;
    float objetivo = b.centroCarril;

    if (b.p >= 38.0f && b.p <= RAPIDO_FIN + 2.0f)
    {
        objetivo += b.ruta == 1 ? DESVIO_CASCADA : DESVIO_REMANSO;
    }

    int amenaza = -1;
    float menorDistancia = 999.0f;

    for (int k = 0; k < m.cantidadObstaculos; k++)
    {
        const ObstaculoBalsas& o = m.obstaculos[k];

        if (o.tipo == OBSTACULO_BALSAS_BANANA)
        {
            continue;
        }

        float adelante = o.p - b.p;
        float mitad = o.a;

        if (o.tipo == OBSTACULO_BALSAS_DIVISOR)
        {
            if (b.p > o.p + o.b)
            {
                continue;
            }

            adelante = (o.p - o.b) - b.p;
        }
        else
        {
            if (adelante < -0.5f)
            {
                continue;
            }

            if (o.tipo == OBSTACULO_BALSAS_REMOLINO)
            {
                mitad = o.a * 0.9f;
            }
        }

        if (adelante > 9.0f)
        {
            continue;
        }

        float margen = mitad + RADIO_BALSA + 0.35f;
        float dx = (b.centroCarril + o.desvio) - b.x;

        if (std::fabs(dx) < margen && adelante < menorDistancia)
        {
            menorDistancia = adelante;
            amenaza = k;
        }
    }

    if (amenaza >= 0)
    {
        const ObstaculoBalsas& o = m.obstaculos[amenaza];
        float mitad = o.tipo == OBSTACULO_BALSAS_REMOLINO ? o.a * 0.9f : o.a;
        float margen = mitad + RADIO_BALSA + 0.35f;
        float ox = b.centroCarril + o.desvio;
        float izquierda = ox - margen - 0.1f;
        float derecha = ox + margen + 0.1f;
        bool preferirIzquierda = ox >= b.x;

        if (preferirIzquierda && izquierda < minimo) preferirIzquierda = false;
        else if (!preferirIzquierda && derecha > maximo) preferirIzquierda = true;

        objetivo = preferirIzquierda ? izquierda : derecha;
    }
    else
    {
        // Sin amenazas: se desvia levemente hacia la banana mas cercana.
        float mejor = 8.0f;

        for (int k = 0; k < m.cantidadObstaculos; k++)
        {
            const ObstaculoBalsas& o = m.obstaculos[k];

            if (o.tipo != OBSTACULO_BALSAS_BANANA || b.recogidas[k])
            {
                continue;
            }

            float adelante = o.p - b.p;
            float ox = b.centroCarril + o.desvio;

            if (adelante > 1.0f && adelante < mejor && std::fabs(ox - b.x) < 3.0f)
            {
                mejor = adelante;
                objetivo = ox;
            }
        }

        objetivo += b.ruido;
    }

    b.objetivoX = Acotar(objetivo, minimo, maximo);
}


// Error de rumbo: positivo = la balsa debe girar hacia la derecha.
static float ErrorRumboBot(const BalsaRio& b)
{
    float deseado = Acotar((b.objetivoX - b.x) * 0.32f, -0.6f, 0.6f);

    return deseado - b.rumbo - 0.25f * b.omega;
}


//==================================================
// FIN DE LA PARTIDA
//==================================================

static void FinalizarBalsas(MinijuegoBalsasRapido& m)
{
    float p0 = m.balsas[0].p;
    float p1 = m.balsas[1].p;

    if (m.balsas[0].terminada != m.balsas[1].terminada)
    {
        m.empate = false;
        m.equipoGanador = m.balsas[0].terminada ? 0 : 1;
    }
    else
    {
        m.empate = std::fabs(p0 - p1) < 0.01f;
        m.equipoGanador = m.empate ? -1 : (p0 > p1 ? 0 : 1);
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
    ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
    m.resultado.desenlace = m.empate ? DESENLACE_EMPATE : DESENLACE_CON_GANADOR;
    m.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = m.estadosJugadores[i].equipo;
        resultadoJugador.numeroEquipo = equipo;
        resultadoJugador.puntuacionMinijuego =
            equipo >= 0 && equipo < 2 ? (int)m.balsas[equipo].p : 0;
        resultadoJugador.puntosObtenidos = 0;
        resultadoJugador.posicionFinal =
            m.empate || equipo == m.equipoGanador ? 1 : 2;
    }

    m.fase = FASE_BALSAS_TERMINADO;
}


//==================================================
// CAMARA
//==================================================

static void ActualizarCamara(MinijuegoBalsasRapido& m, float deltaTime, bool instantanea)
{
    float p0 = m.balsas[0].p;
    float p1 = m.balsas[1].p;
    float medio = 0.5f * (p0 + p1);
    float separacion = Acotar(std::fabs(p0 - p1), 0.0f, 25.0f);

    if (instantanea)
    {
        m.focoCamara = medio;
        m.separacionCamara = separacion;
    }
    else
    {
        float t = Acotar(4.0f * deltaTime, 0.0f, 1.0f);
        m.focoCamara += (medio - m.focoCamara) * t;
        m.separacionCamara += (separacion - m.separacionCamara) * t;
    }

    float zoom = m.separacionCamara;

    m.camara.position = { 0.0f, 11.0f + zoom * 0.25f, -m.focoCamara + 15.5f + zoom * 0.55f };
    m.camara.target = { 0.0f, 0.4f, -m.focoCamara - 5.5f };
    m.camara.up = { 0.0f, 1.0f, 0.0f };
    m.camara.fovy = 56.0f;
    m.camara.projection = CAMERA_PERSPECTIVE;
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

static void LiberarMallasBalsas();

void MinijuegoBalsasRapido::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;

    for (int e = 0; e < 2; e++)
    {
        balsas[e] = {};
        balsas[e].equipo = e;
        balsas[e].centroCarril = CentroCarril(e);
        balsas[e].x = balsas[e].centroCarril;
        balsas[e].ruta = GetRandomValue(0, 99) < 60 ? 0 : 1;
        balsas[e].objetivoX = balsas[e].centroCarril;
    }

    for (int i = 0; i < MAX_PARTICULAS_BALSAS; i++)
    {
        particulas[i] = {};
    }

    CrearRecorrido(*this);

    equipoGanador = -1;
    empate = false;
    partidaValida = false;
    tiempoSonidoPalada = 0.0f;

    fase = FASE_BALSAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_BALSAS;
    tiempoRestante = DURACION_CARRERA_BALSAS;
    tiempoAnimacion = 0.0f;

    // Estado solo visual.
    for (int e = 0; e < 2; e++)
    {
        visualBalsas[e] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_VISUAL_BALSAS; i++)
    {
        particulasVisual[i] = {};
    }

    semillaVisual = 0x5EEDBA15u;
    tiempoVisual = 0.0f;
    anuncioBifurcacion = -1.0f;
    bifurcacionAnunciada = false;
    tiempoConfeti = 0.0f;
    acumuladorAmbiente = 0.0f;

    ActualizarCamara(*this, 0.0f, true);
}


void MinijuegoBalsasRapido::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    LiberarMallasBalsas();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );
    resultado.cantidadEquipos = 2;

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            indices[cantidad++] = i;
        }
    }

    if (cantidad < 2)
    {
        for (int k = 0; k < cantidad; k++)
        {
            resultado.participantes[indices[k]].numeroEquipo = 0;
        }

        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_BALSAS_TERMINADO;
        return;
    }

    partidaValida = true;

    for (int i = cantidad - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = indices[i];
        indices[i] = indices[otro];
        indices[otro] = temporal;
    }

    // 4: 2 vs 2. 3: 2 vs 1 (el solitario tiene ventaja). 2: 1 vs 1.
    int enEquipo0 = cantidad >= 3 ? 2 : 1;
    int ordenEquipo[2] = { 0, 0 };

    for (int k = 0; k < cantidad; k++)
    {
        int equipo = k < enEquipo0 ? 0 : 1;
        int indice = indices[k];

        estadosJugadores[indice].equipo = equipo;
        estadosJugadores[indice].lado = ordenEquipo[equipo] == 0 ? 0 : 1;
        ordenEquipo[equipo]++;
        cantidadJugadoresEquipo[equipo]++;
        resultado.participantes[indice].numeroEquipo = equipo;
    }

    for (int e = 0; e < 2; e++)
    {
        balsas[e].activa = true;
        balsas[e].cantidad = cantidadJugadoresEquipo[e];
    }

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        EstadoJugadorBalsas& estado = estadosJugadores[indice];

        if (cantidadJugadoresEquipo[estado.equipo] <= 1)
        {
            estado.lado = 2;
        }

        estado.periodo = 0.36f + 0.14f * Aleatorio01();
        estado.temporizador = Aleatorio01() * 0.35f;

        const BalsaRio& balsa = balsas[estado.equipo];
        Vector3 spawn = { balsa.x, 1.0f, 0.0f };

        ConfigurarJugadorMinijuegoEstandar(jugadores[indice], spawn);
        jugadores[indice].posicion = spawn;
        jugadores[indice].direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugadores[indice].enSuelo = true;
        jugadores[indice].cayendo = false;
    }

    ActualizarCamara(*this, 0.0f, true);
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoBalsasRapido::ActualizarLogica(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    if (deltaTime > 0.05f)
    {
        deltaTime = 0.05f;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    tiempoAnimacion += deltaTime;

    if (fase == FASE_BALSAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_BALSAS_JUGANDO;
        }

        // Durante la cuenta regresiva todos esperan quietos.
        return;
    }

    if (fase != FASE_BALSAS_JUGANDO || !partidaValida)
    {
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (tiempoSonidoPalada > 0.0f) tiempoSonidoPalada -= deltaTime;

    for (int e = 0; e < 2; e++)
    {
        CalcularObjetivoBot(*this, balsas[e], deltaTime);
    }

    // Entradas: cada participante rema.
    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorBalsas& estado = estadosJugadores[i];

        if (!participantes[i].activo || estado.equipo < 0)
        {
            continue;
        }

        BalsaRio& balsa = balsas[estado.equipo];

        if (balsa.terminada)
        {
            continue;
        }

        if (estado.recarga > 0.0f) estado.recarga -= deltaTime;

        if (JugadorEsBot(participantes[i]))
        {
            estado.temporizador -= deltaTime;

            if (estado.temporizador > 0.0f)
            {
                continue;
            }

            float error = ErrorRumboBot(balsa);
            float siguiente = estado.periodo * (0.85f + 0.3f * Aleatorio01());

            if (estado.lado == 2)
            {
                if (error > 0.12f) Palada(*this, balsa, 0, 1.4f, false);
                else if (error < -0.12f) Palada(*this, balsa, 1, 1.4f, false);
                else
                {
                    Palada(*this, balsa, 0, 1.0f, false);
                    Palada(*this, balsa, 1, 1.0f, false);
                }
            }
            else if (
                (estado.lado == 0 && error < -0.12f) ||
                (estado.lado == 1 && error > 0.12f)
            )
            {
                // Remar aqui empeoraria el rumbo: espera y reevalua.
                siguiente = 0.08f;
            }
            else
            {
                Palada(*this, balsa, estado.lado, 1.0f, false);

                bool ayuda =
                    (estado.lado == 0 && error > 0.12f) ||
                    (estado.lado == 1 && error < -0.12f);

                if (ayuda) siguiente *= 0.8f;
            }

            estado.temporizador = siguiente;
            continue;
        }

        InputMinijuegoParticipante entrada = LeerInputMinijuegoParticipante(participantes[i]);

        if (!entrada.golpear || estado.recarga > 0.0f)
        {
            continue;
        }

        estado.recarga = RECARGA_PALADA;

        if (estado.lado == 2)
        {
            if (entrada.izquierda && !entrada.derecha) Palada(*this, balsa, 1, 1.4f, true);
            else if (entrada.derecha && !entrada.izquierda) Palada(*this, balsa, 0, 1.4f, true);
            else
            {
                Palada(*this, balsa, 0, 1.0f, true);
                Palada(*this, balsa, 1, 1.0f, true);
            }
        }
        else
        {
            Palada(*this, balsa, estado.lado, 1.0f, true);
        }
    }

    for (int e = 0; e < 2; e++)
    {
        if (balsas[e].activa && !balsas[e].terminada)
        {
            ActualizarBalsa(*this, balsas[e], deltaTime);
        }
    }

    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_BALSAS, deltaTime);

    // Los jugadores viajan sobre su balsa.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorBalsas& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        const BalsaRio& b = balsas[estado.equipo];
        float lateral = estado.lado == 0 ? -0.45f : (estado.lado == 1 ? 0.45f : 0.0f);
        float longitudinal = 0.1f;
        float seno = std::sin(b.rumbo);
        float coseno = std::cos(b.rumbo);
        float flotar = 0.04f * std::sin(tiempoAnimacion * 3.0f + (float)estado.equipo);

        JugadorPrueba& jugador = jugadores[i];
        jugador.posicion =
        {
            b.x + coseno * lateral + seno * longitudinal,
            0.3f + flotar + jugador.tamano.y * 0.5f,
            -b.p + seno * lateral - coseno * longitudinal
        };
        jugador.velocidad = {};
        jugador.direccionMirada = { seno, 0.0f, -coseno };
        jugador.enSuelo = true;
        jugador.cayendo = false;
    }

    ActualizarCamara(*this, deltaTime, false);

    if (
        balsas[0].terminada ||
        balsas[1].terminada ||
        tiempoRestante <= 0.0f
    )
    {
        tiempoRestante = tiempoRestante < 0.0f ? 0.0f : tiempoRestante;
        FinalizarBalsas(*this);
    }
}


//==================================================
// VISUAL: RIO DE LA SELVA
//==================================================
//
// Selva humeda de manana: el rio corre entre una isla central con ruinas y
// dos orillas de selva cerrada (arboles gigantes con lianas, acantilados
// de roca con cascadas, templos tomados por la vegetacion). Rayos de sol
// atraviesan la bruma y dejan charcos de luz en el agua. Lo estatico se
// hornea por tramos del rio (agua con corriente y espuma por shader); lo
// que se mueve (balsas, troncos, bananas, remolinos, loros, luces) se arma
// cada cuadro. La logica no depende de nada de esta seccion.
//
// MODELO FUTURO: balsa de troncos con remos y banderin, troncos flotantes,
// racimo de bananas sobre hoja, loro guacamayo, cabeza de piedra tallada,
// arco de meta del templo, totem de la bifurcacion.

static const Color COLOR_CIELO_ALTO_B = { 118, 170, 182, 255 };
static const Color COLOR_CIELO_BAJO_B = { 214, 226, 196, 255 };
static const Color COLOR_NIEBLA_B = { 184, 204, 176, 255 };
static const Color COLOR_SOL_B = { 255, 240, 196, 255 };
static const Color COLOR_AGUA_B = { 44, 120, 104, 255 };
static const Color COLOR_AGUA_ORILLA_B = { 82, 138, 102, 255 };
static const Color COLOR_REMANSO_B = { 30, 90, 82, 255 };
static const Color COLOR_RAPIDO_B = { 92, 162, 150, 255 };
static const Color COLOR_BARRO_B = { 98, 76, 52, 255 };
static const Color COLOR_TIERRA_B = { 78, 66, 42, 255 };
static const Color COLOR_PIEDRA_B = { 122, 120, 104, 255 };
static const Color COLOR_PIEDRA_MOJADA_B = { 70, 74, 66, 255 };
static const Color COLOR_RUINA_B = { 160, 154, 126, 255 };
static const Color COLOR_MUSGO_B = { 76, 126, 50, 255 };
static const Color COLOR_HOJA_B = { 48, 114, 50, 255 };
static const Color COLOR_HOJA_CLARA_B = { 104, 160, 62, 255 };
static const Color COLOR_HOJA_OSCURA_B = { 28, 76, 42, 255 };
static const Color COLOR_TRONCO_B = { 94, 72, 52, 255 };
static const Color COLOR_MADERA_B = { 156, 108, 64, 255 };
static const Color COLOR_CUERDA_B = { 196, 170, 112, 255 };
static const Color COLOR_ORO_B = { 248, 198, 72, 255 };     // acento: meta y bananas
static const Color COLOR_ESPUMA_B = { 232, 248, 242, 255 };


static float Lim01B(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalB(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoB(Color c, float f)
{
    return { CanalB((float)c.r * f), CanalB((float)c.g * f), CanalB((float)c.b * f), c.a };
}


static Color MezclaB(Color a, Color b, float t)
{
    t = Lim01B(t);
    return {
        CanalB((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalB((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalB((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalB((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaB(Color c, float alfa)
{
    c.a = CanalB((float)c.a * Lim01B(alfa));
    return c;
}


// Hash determinista 0..1 (azar visual sin tocar GetRandomValue).
static float Ruido01B(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static Color VariarB(Color c, int indice, int semilla, float amplitud)
{
    return TonoB(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01B(indice, semilla));
}


static float AzarVisualB(MinijuegoBalsasRapido& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


//--------------------------------------------------
// Estado visual: avisos, estela y particulas propias
//--------------------------------------------------

static void EmitirParticulaB(
    MinijuegoBalsasRapido& m,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUAL_BALSAS; i++)
    {
        ParticulaVisualBalsas& p = m.particulasVisual[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.tipo = tipo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.giro = AzarVisualB(m, 0.0f, 6.28f);
        p.color = color;
        return;
    }
}


static int CantidadParticulasB(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


static void ActualizarVisualBalsas(MinijuegoBalsasRapido& m, float dt)
{
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.0f) dt = 0.0f;

    m.tiempoVisual += dt;
    bool jugando = m.fase == FASE_BALSAS_JUGANDO && m.partidaValida;

    for (int e = 0; e < 2; e++)
    {
        const BalsaRio& b = m.balsas[e];
        VisualBalsaRio& v = m.visualBalsas[e];

        if (!b.activa)
        {
            continue;
        }

        v.destello = v.destello > dt * 2.5f ? v.destello - dt * 2.5f : 0.0f;
        v.avisoSync = v.avisoSync > dt ? v.avisoSync - dt : 0.0f;
        v.avisoBanana = v.avisoBanana > dt ? v.avisoBanana - dt : 0.0f;
        v.avisoChoque = v.avisoChoque > dt ? v.avisoChoque - dt : 0.0f;
        v.sacudida = v.sacudida > dt * 2.2f ? v.sacudida - dt * 2.2f : 0.0f;
        v.anuncioRuta = v.anuncioRuta > dt ? v.anuncioRuta - dt : 0.0f;
        v.anuncioMeta = v.anuncioMeta > dt ? v.anuncioMeta - dt : 0.0f;

        Color colorEquipo = ColorEquipoBalsas(e);
        Vector3 centro = { b.x, 0.25f, -b.p };

        // Sincronia: anillo de espuma y chispas del color del equipo.
        if (b.tiempoSync > v.tiempoSyncAnterior + 0.2f)
        {
            v.avisoSync = 1.0f;
            v.destello = 1.0f;

            int n = CantidadParticulasB(14);
            for (int k = 0; k < n; k++)
            {
                float a = 6.2832f * (float)k / (float)n;
                EmitirParticulaB(m, 3, { centro.x + std::cos(a) * 1.0f, 0.3f, centro.z + std::sin(a) * 1.0f },
                    { std::cos(a) * 2.4f, AzarVisualB(m, 1.6f, 3.0f), std::sin(a) * 2.4f },
                    AzarVisualB(m, 0.45f, 0.7f), 0.09f, MezclaB(colorEquipo, WHITE, 0.25f));
            }
        }

        // Choque: astillas y salpicon gris, cabeceo y temblor leve.
        if (b.tiempoImpacto > v.tiempoImpactoAnterior + 0.2f)
        {
            v.avisoChoque = 1.0f;
            v.sacudida = 1.0f;
            ActivarTemblorCamaraGeneral(0.08f, 0.2f);

            int n = CantidadParticulasB(10);
            for (int k = 0; k < n; k++)
            {
                EmitirParticulaB(m, 0, { centro.x + AzarVisualB(m, -0.6f, 0.6f), 0.35f, centro.z - 0.9f },
                    { AzarVisualB(m, -2.5f, 2.5f), AzarVisualB(m, 2.0f, 4.0f), AzarVisualB(m, -2.5f, 0.5f) },
                    AzarVisualB(m, 0.5f, 0.9f), AzarVisualB(m, 0.07f, 0.13f), Color{ 150, 150, 140, 255 });
            }
        }

        // Banana: destellos dorados.
        if (b.bananas > v.bananasAnterior)
        {
            v.avisoBanana = 1.2f;
            v.destello = 1.0f;

            int n = CantidadParticulasB(12);
            for (int k = 0; k < n; k++)
            {
                EmitirParticulaB(m, 3, { centro.x + AzarVisualB(m, -0.5f, 0.5f), 0.6f, centro.z + AzarVisualB(m, -0.5f, 0.5f) },
                    { AzarVisualB(m, -1.8f, 1.8f), AzarVisualB(m, 2.0f, 3.6f), AzarVisualB(m, -1.8f, 1.8f) },
                    AzarVisualB(m, 0.5f, 0.8f), 0.08f, COLOR_ORO_B);
            }
        }

        if (b.terminada && !v.terminadaAnterior)
        {
            v.anuncioMeta = 3.0f;
        }

        // Ruta elegida al entrar al rapido.
        if (v.ruta < 0 && b.p > RAPIDO_INICIO + 1.0f && b.p < RAPIDO_FIN)
        {
            v.ruta = b.x - b.centroCarril < 0.0f ? 0 : 1;
            v.anuncioRuta = 1.6f;
        }

        float avance = b.velocidad + CorrienteRio(b.p, b.x - b.centroCarril);
        float objetivo = jugando && !b.terminada ? Lim01B(avance / 9.0f) : 0.0f;
        v.estela += (objetivo - v.estela) * Lim01B(dt * 3.0f);

        // Espuma en la proa cuando la balsa va rapido o baja por la cascada.
        if (jugando && !b.terminada && avance > 3.5f)
        {
            float probabilidad = (avance - 3.5f) * 0.9f * dt * 8.0f * FactorCalidadGrafica(CalidadParticulas());
            if (AzarVisualB(m, 0.0f, 1.0f) < probabilidad)
            {
                float s = std::sin(b.rumbo), c = std::cos(b.rumbo);
                float lado = AzarVisualB(m, -0.8f, 0.8f);
                EmitirParticulaB(m, 0, { b.x + s * 1.3f + c * lado, 0.15f, -b.p - c * 1.3f + s * lado },
                    { c * lado * 1.5f, AzarVisualB(m, 1.2f, 2.4f), AzarVisualB(m, 0.0f, 1.0f) },
                    AzarVisualB(m, 0.35f, 0.6f), AzarVisualB(m, 0.06f, 0.11f), COLOR_ESPUMA_B);
            }
        }

        v.tiempoSyncAnterior = b.tiempoSync;
        v.tiempoImpactoAnterior = b.tiempoImpacto;
        v.bananasAnterior = b.bananas;
        v.terminadaAnterior = b.terminada;
    }

    // Aviso de la bifurcacion cuando la primera balsa se acerca.
    float mayorP = m.balsas[0].p > m.balsas[1].p ? m.balsas[0].p : m.balsas[1].p;

    if (!m.bifurcacionAnunciada && jugando && mayorP > 27.0f)
    {
        m.bifurcacionAnunciada = true;
        m.anuncioBifurcacion = 0.0f;
    }

    if (m.anuncioBifurcacion >= 0.0f)
    {
        m.anuncioBifurcacion += dt;
    }

    // Ambiente: hojas que caen y rocio de los saltos de la cascada.
    float foco = m.focoCamara;
    m.acumuladorAmbiente += dt * 7.0f * FactorCalidadGrafica(CalidadParticulas());

    while (m.acumuladorAmbiente >= 1.0f)
    {
        m.acumuladorAmbiente -= 1.0f;

        if (AzarVisualB(m, 0.0f, 1.0f) < 0.45f)
        {
            Color hoja = MezclaB(COLOR_HOJA_CLARA_B, Color{ 196, 170, 70, 255 }, AzarVisualB(m, 0.0f, 0.7f));
            EmitirParticulaB(m, 2, { AzarVisualB(m, -17.0f, 17.0f), AzarVisualB(m, 6.0f, 9.0f), -(foco + AzarVisualB(m, -6.0f, 34.0f)) },
                { AzarVisualB(m, -0.4f, 0.4f), AzarVisualB(m, -0.9f, -0.6f), AzarVisualB(m, -0.3f, 0.3f) },
                AzarVisualB(m, 6.0f, 9.0f), AzarVisualB(m, 0.12f, 0.18f), hoja);
        }
        else
        {
            static const float SALTOS[3] = { 56.0f, 65.0f, 74.0f };
            int salto = (int)AzarVisualB(m, 0.0f, 2.99f);
            int carril = AzarVisualB(m, 0.0f, 1.0f) < 0.5f ? 0 : 1;
            float p = SALTOS[salto];

            if (std::fabs(p - foco) < 40.0f)
            {
                float x = CentroCarril(carril) + DESVIO_CASCADA + AzarVisualB(m, -2.0f, 2.0f);
                EmitirParticulaB(m, 0, { x, 0.1f, -p + AzarVisualB(m, -0.4f, 0.4f) },
                    { AzarVisualB(m, -0.5f, 0.5f), AzarVisualB(m, 1.5f, 2.8f), AzarVisualB(m, -1.2f, 0.0f) },
                    AzarVisualB(m, 0.5f, 0.8f), AzarVisualB(m, 0.08f, 0.14f), COLOR_ESPUMA_B);
            }
        }
    }

    // Confeti sobre la balsa ganadora.
    if (m.fase == FASE_BALSAS_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO && m.tiempoConfeti < 2.6f)
    {
        m.tiempoConfeti += dt;
        int desde = m.empate ? 0 : m.equipoGanador;
        int hasta = m.empate ? 1 : m.equipoGanador;
        int n = CantidadParticulasB(3);

        for (int e = desde; e <= hasta && e >= 0; e++)
        {
            const BalsaRio& b = m.balsas[e];
            for (int k = 0; k < n; k++)
            {
                static const Color CONFETI[5] = {
                    { 255, 214, 80, 255 }, { 255, 255, 255, 255 }, { 120, 220, 200, 255 },
                    { 255, 150, 40, 255 }, { 240, 80, 170, 255 }
                };
                Color c = k % 2 == 0 ? ColorEquipoBalsas(e) : CONFETI[(int)AzarVisualB(m, 0.0f, 4.99f)];
                EmitirParticulaB(m, 1, { b.x + AzarVisualB(m, -1.5f, 1.5f), AzarVisualB(m, 4.0f, 5.0f), -b.p + AzarVisualB(m, -1.5f, 1.5f) },
                    { AzarVisualB(m, -1.5f, 1.5f), AzarVisualB(m, 0.5f, 2.5f), AzarVisualB(m, -1.5f, 1.5f) },
                    AzarVisualB(m, 1.6f, 2.4f), AzarVisualB(m, 0.07f, 0.11f), c);
            }
        }
    }

    // Integracion.
    for (int i = 0; i < MAX_PARTICULAS_VISUAL_BALSAS; i++)
    {
        ParticulaVisualBalsas& p = m.particulasVisual[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= dt;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        if (p.tipo == 1)
        {
            p.velocidad.y -= 3.0f * dt;
            if (p.velocidad.y < -1.4f) p.velocidad.y = -1.4f;
            p.velocidad.x += std::sin(m.tiempoVisual * 5.0f + (float)i) * 2.0f * dt;
        }
        else if (p.tipo == 2)
        {
            p.velocidad.x = 0.5f * std::sin(m.tiempoVisual * 1.6f + (float)i * 1.3f);
            if (p.posicion.y < 0.04f)
            {
                p.posicion.y = 0.04f;
                p.velocidad = { 0.0f, 0.0f, -1.6f };
            }
        }
        else
        {
            p.velocidad.y -= 9.0f * dt;
        }

        p.giro += dt * 3.0f;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        if (p.tipo != 2 && p.posicion.y < -0.05f)
        {
            p.activa = false;
        }
    }
}


void MinijuegoBalsasRapido::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    ActualizarLogica(deltaTime, jugadores, cantidadMaxima, participantes);
    ActualizarVisualBalsas(*this, deltaTime);
}


//--------------------------------------------------
// Geometria: lotes de triangulos y grabacion en mallas
//--------------------------------------------------
//
// Cada vertice lleva un "modo" para el shader del horneado: 0 opaco con
// bruma, 1 agua (dato = velocidad de la corriente, alfa = espuma), 2 cielo
// sin bruma, 3 follaje que se mece (dato = peso), 4 agua que cae (dato =
// velocidad). Lo que se arma cada cuadro aplica la misma bruma en CPU.

struct GrabacionBalsas
{
    bool activa = false;
    std::vector<float> posiciones;
    std::vector<unsigned char> colores;
    std::vector<float> datos;
};

static GrabacionBalsas g_grabB;
static Vector3 g_ojoB = { 0.0f, 11.0f, 15.0f };
static Vector3 g_derB = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaB = { 0.0f, 1.0f, 0.0f };
static float g_modoB = 0.0f;
static float g_datoB = 0.0f;


// Misma formula que el shader (FS_HORNO_B).
static float CantidadNieblaB(Vector3 p)
{
    float dx = p.x - g_ojoB.x, dy = p.y - g_ojoB.y, dz = p.z - g_ojoB.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01B((d - 30.0f) / 150.0f) * 0.82f;
    float bajo = 1.0f - Lim01B(p.y / 6.0f);
    n += bajo * Lim01B((d - 40.0f) / 120.0f) * 0.18f;
    return n > 0.88f ? 0.88f : n;
}


static Color NieblaB(Color c, Vector3 p)
{
    if (g_grabB.activa)
    {
        return c;
    }

    Color r = MezclaB(c, COLOR_NIEBLA_B, CantidadNieblaB(p));
    r.a = c.a;
    return r;
}


static void VB(Vector3 p, Color c)
{
    if (g_grabB.activa)
    {
        g_grabB.posiciones.push_back(p.x);
        g_grabB.posiciones.push_back(p.y);
        g_grabB.posiciones.push_back(p.z);
        g_grabB.colores.push_back(c.r);
        g_grabB.colores.push_back(c.g);
        g_grabB.colores.push_back(c.b);
        g_grabB.colores.push_back(c.a);
        g_grabB.datos.push_back(g_datoB);
        g_grabB.datos.push_back(g_modoB);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteB()
{
    if (!g_grabB.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteB()
{
    if (!g_grabB.activa) rlEnd();
}


static void TriB(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VB(a, NieblaB(color, a));
    VB(b, NieblaB(color, b));
    VB(c, NieblaB(color, c));
}


static void QuadB(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriB(a, b, c, color);
    TriB(a, c, d, color);
}


// Quad con un color por vertice (con bruma).
static void Quad4B(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VB(a, NieblaB(ca, a)); VB(b, NieblaB(cb, b)); VB(c, NieblaB(cc, c));
    VB(a, NieblaB(ca, a)); VB(c, NieblaB(cc, c)); VB(d, NieblaB(cd, d));
}


// Quad sin bruma con un color por vertice (luces aditivas y velos).
static void QuadColoresB(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VB(a, ca); VB(b, cb); VB(c, cc);
    VB(a, ca); VB(c, cc); VB(d, cd);
}


// Sombreado por caras de la guia (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62) interpolado para normales cualesquiera; lo que mira arriba
// recibe la luz tibia de la manana.
static float LuzB(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f)
    {
        return 1.0f;
    }

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.45f) +
        (n.z > 0.0f ? az * 0.78f : az * 0.58f) +
        (n.x < 0.0f ? ax * 0.70f : ax * 0.62f);

    return f / suma;
}


static Color CaraB(Color c, Vector3 n)
{
    Color r = TonoB(c, LuzB(n));

    if (n.y > 0.3f)
    {
        r = MezclaB(r, Color{ 255, 236, 180, 255 }, 0.10f * n.y);
    }

    return r;
}


static void CajaEjesB(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
{
    auto P = [&](float sx, float sy, float sz)
    {
        return Vector3{
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    };

    Vector3 nx = Vector3Normalize(ex), ny = Vector3Normalize(ey), nz = Vector3Normalize(ez);

    QuadB(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraB(color, ny));
    QuadB(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraB(color, nz));
    QuadB(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraB(color, Vector3Negate(nz)));
    QuadB(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraB(color, Vector3Negate(nx)));
    QuadB(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraB(color, nx));

    if (fondo)
    {
        QuadB(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraB(color, Vector3Negate(ny)));
    }
}


static void CajaB(Vector3 c, Vector3 t, Color color)
{
    CajaEjesB(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, false);
}


// Caja girada en Y (grados) y, opcionalmente, ladeada sobre su eje X.
static void CajaGiradaB(Vector3 c, Vector3 t, float giroY, Color color, float ladeo = 0.0f)
{
    float a = giroY * DEG2RAD;
    float l = ladeo * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz0 = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = { -uz0.x * std::sin(l), std::cos(l), -uz0.z * std::sin(l) };
    Vector3 uz = { uz0.x * std::cos(l), std::sin(l), uz0.z * std::cos(l) };

    CajaEjesB(c, Vector3Scale(ux, t.x * 0.5f), Vector3Scale(uy, t.y * 0.5f), Vector3Scale(uz, t.z * 0.5f), color, false);
}


// Viga de seccion cuadrada entre dos puntos.
static void VigaB(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesB(centro, Vector3Scale(v, grosor * 0.5f), Vector3Scale(w, grosor * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas.
static void CilB(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    eje = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, ref));
    Vector3 v = Vector3CrossProduct(eje, u);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = 0.5f * (a0 + a1);
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));

        QuadB(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraB(color, nm)
        );

        if (tapas)
        {
            TriB(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraB(color, eje));
            TriB(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraB(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide con caras sombreadas; "bulto" deforma el radio por vertice
// (rocas y copas irregulares) de forma determinista.
static void BolaB(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float bulto = 0.0f, int semilla = 0)
{
    auto P = [&](int i, int k)
    {
        float la = -0.5f * PI + PI * (float)i / (float)anillos;
        float lo = 2.0f * PI * (float)(k % lados) / (float)lados;
        float f = 1.0f;

        if (bulto > 0.0f && i > 0 && i < anillos)
        {
            f = 1.0f - bulto + 2.0f * bulto * Ruido01B(i * 31 + (k % lados), semilla);
        }

        return Vector3{ c.x + r.x * f * std::cos(la) * std::cos(lo), c.y + r.y * f * std::sin(la), c.z + r.z * f * std::cos(la) * std::sin(lo) };
    };

    for (int i = 0; i < anillos; i++)
    {
        float lam = -0.5f * PI + PI * ((float)i + 0.5f) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float lom = 2.0f * PI * ((float)k + 0.5f) / (float)lados;
            Vector3 n = { std::cos(lam) * std::cos(lom), std::sin(lam), std::cos(lam) * std::sin(lom) };
            QuadB(P(i, k), P(i, k + 1), P(i + 1, k + 1), P(i + 1, k), CaraB(color, n));
        }
    }
}


// Disco horizontal con color de centro y de borde.
static void DiscoB(Vector3 c, float rx, float rz, int lados, Color centro, Color borde)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };
        VB(c, NieblaB(centro, c)); VB(p1, NieblaB(borde, p1)); VB(p0, NieblaB(borde, p0));
    }
}


static void AnilloB(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera, float desde = 0.0f, float hasta = 2.0f * PI)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = desde + (hasta - desde) * (float)k / (float)lados;
        float a1 = desde + (hasta - desde) * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        Quad4B(i0, o0, o1, i1, dentro, fuera, fuera, dentro);
    }
}


// Disco que mira a la camara (halos y nubes de rocio).
static void HaloB(Vector3 c, float radio, Color centro, Color borde, int lados = 12)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derB, std::cos(a0) * radio), Vector3Scale(g_arribaB, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derB, std::cos(a1) * radio), Vector3Scale(g_arribaB, std::sin(a1) * radio)));
        VB(c, centro); VB(p0, borde); VB(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas).
static void PuntoB(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derB, r), u = Vector3Scale(g_arribaB, r);
    QuadColoresB(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Tira delgada entre dos puntos que mira a la camara (lianas, sogas).
static void TiraB(Vector3 a, Vector3 b, float ancho, Color ca, Color cb)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 vista = Vector3Subtract(Vector3Scale(Vector3Add(a, b), 0.5f), g_ojoB);
    Vector3 lado = Vector3CrossProduct(eje, vista);
    float l = Vector3Length(lado);

    if (l < 0.0001f)
    {
        return;
    }

    lado = Vector3Scale(lado, ancho * 0.5f / l);
    Quad4B(Vector3Subtract(a, lado), Vector3Add(a, lado), Vector3Add(b, lado), Vector3Subtract(b, lado), ca, ca, cb, cb);
}


// Punto local (x derecha, y arriba, z atras) de algo girado en Y (radianes,
// positivo = hacia +x como el rumbo de la balsa).
static Vector3 LocalB(Vector3 base, float rumbo, Vector3 local)
{
    float s = std::sin(rumbo), c = std::cos(rumbo);
    return {
        base.x + c * local.x - s * local.z,
        base.y + local.y,
        base.z + s * local.x + c * local.z
    };
}


//--------------------------------------------------
// Horno: lo estatico se graba por tramos del rio
//--------------------------------------------------

static const float P_INICIO_HORNO_B = -40.0f;
static const float LARGO_TRAMO_B = 20.0f;
static const int TRAMOS_B = 14;          // hasta p = 240

struct HornoBalsas
{
    bool listo = false;
    int clave = -1;
    bool shaderListo = false;
    Shader shader{};
    Material material{};
    int locOjo = -1;
    int locNiebla = -1;
    int locTiempo = -1;
    Mesh tramos[TRAMOS_B]{};
    bool tramoValido[TRAMOS_B]{};
    Mesh lejos{};
    bool lejosValido = false;
    Mesh cielo{};
    bool cieloValido = false;
};

static HornoBalsas g_hornoB;


static const char* VS_HORNO_B =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform float tiempo;\n"
    "out vec4 fragColor;\n"
    "out vec3 fragPos;\n"
    "out vec2 fragDato;\n"
    "void main()\n"
    "{\n"
    "    vec3 p = vertexPosition;\n"
    "    if (vertexTexCoord.y > 2.5 && vertexTexCoord.y < 3.5)\n"
    "    {\n"
    "        float w = vertexTexCoord.x;\n"
    "        p.x += w * 0.22 * sin(tiempo * 1.3 + p.z * 0.37 + p.y * 0.5);\n"
    "        p.z += w * 0.12 * sin(tiempo * 1.1 + p.x * 0.41);\n"
    "    }\n"
    "    fragColor = vertexColor;\n"
    "    fragPos = p;\n"
    "    fragDato = vertexTexCoord;\n"
    "    gl_Position = mvp * vec4(p, 1.0);\n"
    "}\n";

// Agua: vetas que corren con la corriente (mas rapidas en la cascada,
// quietas en el remanso), espuma donde el alfa lo pide y destellos.
static const char* FS_HORNO_B =
    "#version 330\n"
    "in vec4 fragColor;\n"
    "in vec3 fragPos;\n"
    "in vec2 fragDato;\n"
    "uniform vec3 ojo;\n"
    "uniform vec4 niebla;\n"
    "uniform float tiempo;\n"
    "out vec4 finalColor;\n"
    "float h(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }\n"
    "float r(vec2 p)\n"
    "{\n"
    "    vec2 i = floor(p); vec2 f = fract(p); f = f * f * (3.0 - 2.0 * f);\n"
    "    return mix(mix(h(i), h(i + vec2(1.0, 0.0)), f.x), mix(h(i + vec2(0.0, 1.0)), h(i + vec2(1.0, 1.0)), f.x), f.y);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec3 c = fragColor.rgb;\n"
    "    float a = fragColor.a;\n"
    "    float modo = fragDato.y;\n"
    "    if (modo > 1.5 && modo < 2.5) { finalColor = fragColor; return; }\n"
    "    if (modo > 0.5 && modo < 1.5)\n"
    "    {\n"
    "        float vel = fragDato.x;\n"
    "        float espuma = a;\n"
    "        vec2 q = vec2(fragPos.x, fragPos.z + tiempo * vel);\n"
    "        float n1 = r(q * vec2(0.55, 0.2));\n"
    "        float n2 = r(q * vec2(1.6, 0.7) + vec2(13.0, 7.0));\n"
    "        float n3 = r(vec2(fragPos.x * 2.4, fragPos.z * 2.0 + tiempo * vel * 1.3) + vec2(5.0, 3.0));\n"
    "        c *= 0.9 + 0.2 * n1;\n"
    "        float vetas = smoothstep(0.6, 0.8, n1 * 0.55 + n2 * 0.45);\n"
    "        c = mix(c, vec3(0.78, 0.92, 0.86), vetas * (0.16 + 0.4 * min(vel * 0.12, 1.0)));\n"
    "        float e = smoothstep(0.62 - 0.4 * espuma, 0.8 - 0.3 * espuma, n2 * 0.5 + n3 * 0.5) * espuma;\n"
    "        c = mix(c, vec3(0.91, 0.97, 0.95), clamp(e * 1.5, 0.0, 1.0));\n"
    "        float brillo = pow(n3 * n2, 7.0) * 2.2;\n"
    "        c += vec3(brillo) * (1.0 - espuma);\n"
    "        a = 1.0;\n"
    "    }\n"
    "    else if (modo > 3.5)\n"
    "    {\n"
    "        vec2 q = vec2((fragPos.x + fragPos.z) * 1.4, fragPos.y * 0.45 + tiempo * fragDato.x);\n"
    "        float n = r(q * vec2(2.0, 0.8));\n"
    "        c = mix(c, vec3(0.95, 1.0, 1.0), smoothstep(0.42, 0.85, n));\n"
    "    }\n"
    "    float d = distance(fragPos, ojo);\n"
    "    float nb = clamp((d - 30.0) / 150.0, 0.0, 1.0) * 0.82;\n"
    "    nb += (1.0 - clamp(fragPos.y / 6.0, 0.0, 1.0)) * clamp((d - 40.0) / 120.0, 0.0, 1.0) * 0.18;\n"
    "    nb = min(nb, 0.88);\n"
    "    finalColor = vec4(mix(c, niebla.rgb, nb), a);\n"
    "}\n";


static void EmpezarGrabacionB()
{
    g_grabB.activa = true;
    g_grabB.posiciones.clear();
    g_grabB.colores.clear();
    g_grabB.datos.clear();
    g_modoB = 0.0f;
    g_datoB = 0.0f;
}


// Cierra la grabacion y sube la malla (sin indices: cabe cualquier tamano).
static bool TerminarGrabacionB(Mesh& malla)
{
    g_grabB.activa = false;
    g_modoB = 0.0f;
    g_datoB = 0.0f;
    int n = (int)(g_grabB.posiciones.size() / 3);
    n -= n % 3;

    if (n < 3)
    {
        g_grabB.posiciones.clear();
        g_grabB.colores.clear();
        g_grabB.datos.clear();
        return false;
    }

    malla = {};
    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * (int)sizeof(float)));
    malla.texcoords = (float*)MemAlloc((unsigned int)(n * 2 * (int)sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    for (int i = 0; i < n * 3; i++) malla.vertices[i] = g_grabB.posiciones[(size_t)i];
    for (int i = 0; i < n * 4; i++) malla.colors[i] = g_grabB.colores[(size_t)i];
    for (int i = 0; i < n * 2; i++) malla.texcoords[i] = g_grabB.datos[(size_t)i];

    UploadMesh(&malla, false);
    g_grabB.posiciones.clear();
    g_grabB.colores.clear();
    g_grabB.datos.clear();
    return true;
}


static void LiberarMallaB(Mesh& malla, bool& valida)
{
    if (valida) UnloadMesh(malla);
    valida = false;
    malla = {};
}


static void LiberarMallasBalsas()
{
    for (int c = 0; c < TRAMOS_B; c++)
    {
        LiberarMallaB(g_hornoB.tramos[c], g_hornoB.tramoValido[c]);
    }

    LiberarMallaB(g_hornoB.lejos, g_hornoB.lejosValido);
    LiberarMallaB(g_hornoB.cielo, g_hornoB.cieloValido);
    g_hornoB.listo = false;
    g_hornoB.clave = -1;
}


//--------------------------------------------------
// Escena estatica: agua de los carriles
//--------------------------------------------------

static const float SALTOS_CASCADA_B[3] = { 56.0f, 65.0f, 74.0f };


// Color, velocidad de la corriente (para las vetas) y espuma del agua en un
// punto del carril; sale de CorrienteRio y de los obstaculos reales.
static void DatosAguaB(
    const MinijuegoBalsasRapido& m,
    int equipo,
    float x,
    float p,
    Color& color,
    float& velocidad,
    float& espuma
)
{
    float c = CentroCarril(equipo);
    float desvio = x - c;
    float rapido = Lim01B((p - RAPIDO_INICIO) / 3.0f) * Lim01B((RAPIDO_FIN - p) / 3.0f);
    float orilla = Lim01B((std::fabs(desvio) - 4.2f) / 1.4f);

    velocidad = 0.3f + 1.5f * CorrienteRio(p, desvio);
    color = MezclaB(COLOR_AGUA_B, COLOR_AGUA_ORILLA_B, orilla * 0.85f);
    espuma = orilla * orilla * 0.42f;

    if (desvio < -0.7f)
    {
        float lado = Lim01B((-desvio - 0.7f) / 0.8f) * rapido;
        color = MezclaB(color, COLOR_RAPIDO_B, lado * 0.75f);
        espuma = std::fmax(espuma, lado * 0.42f);

        for (int s = 0; s < 3; s++)
        {
            float d = std::fabs(p - SALTOS_CASCADA_B[s]);
            espuma = std::fmax(espuma, lado * Lim01B(1.0f - d / 1.2f));
        }
    }
    else if (desvio > 0.7f)
    {
        float lado = Lim01B((desvio - 0.7f) / 0.8f) * rapido;
        color = MezclaB(color, COLOR_REMANSO_B, lado * 0.85f);
        espuma *= 1.0f - 0.6f * lado;
    }

    // La isla divisora parte el agua: espuma en la proa.
    {
        float dp = p - RAPIDO_INICIO;
        if (dp > -3.0f && dp < 0.5f)
        {
            espuma = std::fmax(espuma, Lim01B(1.0f - std::fabs(desvio) / 1.6f) * Lim01B((dp + 3.0f) / 3.0f) * 0.85f);
        }
    }

    for (int k = 0; k < m.cantidadObstaculos; k++)
    {
        const ObstaculoBalsas& o = m.obstaculos[k];
        float ox = c + o.desvio;
        float dx = x - ox;
        float dp = p - o.p;

        if (std::fabs(dp) > o.b + 4.5f || std::fabs(dx) > o.a + 2.0f)
        {
            continue;
        }

        if (o.tipo == OBSTACULO_BALSAS_ROCA)
        {
            float d = std::sqrt(dx * dx + dp * dp);
            espuma = std::fmax(espuma, Lim01B(1.0f - (d - o.a) / 0.75f) * 0.95f);

            // Estela de espuma corriente abajo (+p).
            if (dp > 0.0f && std::fabs(dx) < o.a * 1.1f)
            {
                espuma = std::fmax(espuma, Lim01B(1.0f - dp / 3.2f) * 0.55f * (1.0f - std::fabs(dx) / (o.a * 1.1f)));
            }
        }
        else if (o.tipo == OBSTACULO_BALSAS_TRONCO || o.tipo == OBSTACULO_BALSAS_DIVISOR)
        {
            float rx = std::fabs(dx) - o.a;
            float rp = std::fabs(dp) - o.b;
            float d = std::sqrt(std::fmax(rx, 0.0f) * std::fmax(rx, 0.0f) + std::fmax(rp, 0.0f) * std::fmax(rp, 0.0f));
            espuma = std::fmax(espuma, Lim01B(1.0f - d / 0.65f) * (o.tipo == OBSTACULO_BALSAS_TRONCO ? 0.7f : 0.6f));
        }
        else if (o.tipo == OBSTACULO_BALSAS_REMOLINO)
        {
            float d = std::sqrt(dx * dx + dp * dp);
            float dentro = Lim01B(1.0f - d / o.a);
            color = MezclaB(color, Color{ 18, 66, 62, 255 }, dentro * 0.75f);
            velocidad *= 1.0f - 0.8f * dentro;
            espuma = std::fmax(espuma, Lim01B(1.0f - std::fabs(d - o.a * 0.95f) / 0.45f) * 0.55f);
        }
    }
}


static void AguaTramoB(const MinijuegoBalsasRapido& m, float p0, float p1)
{
    const int COLUMNAS = 19;
    const int FILAS = 32;
    float pasoP = (p1 - p0) / (float)FILAS;

    static Color colores[2][COLUMNAS + 1];
    static float velocidades[2][COLUMNAS + 1];
    static float espumas[2][COLUMNAS + 1];

    g_modoB = 1.0f;

    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);
        float x0 = c - MEDIO_ANCHO_CARRIL - 0.35f;
        float pasoX = (2.0f * MEDIO_ANCHO_CARRIL + 0.7f) / (float)COLUMNAS;

        for (int f = 0; f <= FILAS; f++)
        {
            float p = p0 + pasoP * (float)f;
            int fila = f & 1;

            for (int k = 0; k <= COLUMNAS; k++)
            {
                float x = x0 + pasoX * (float)k;
                DatosAguaB(m, e, x, p, colores[fila][k], velocidades[fila][k], espumas[fila][k]);
                int indice = (int)std::floor((p + 100.0f) / pasoP + 0.5f) * 41 + k + e * 997;
                colores[fila][k] = VariarB(colores[fila][k], indice, 17, 0.035f);
                colores[fila][k].a = CanalB(espumas[fila][k] * 255.0f);
            }

            if (f == 0)
            {
                continue;
            }

            int ant = fila ^ 1;
            float pa = p - pasoP;

            for (int k = 0; k < COLUMNAS; k++)
            {
                float xa = x0 + pasoX * (float)k;
                float xb = xa + pasoX;
                Vector3 v00 = { xa, 0.0f, -pa }, v10 = { xb, 0.0f, -pa };
                Vector3 v01 = { xa, 0.0f, -p }, v11 = { xb, 0.0f, -p };

                g_datoB = velocidades[ant][k]; VB(v00, colores[ant][k]);
                g_datoB = velocidades[ant][k + 1]; VB(v10, colores[ant][k + 1]);
                g_datoB = velocidades[fila][k + 1]; VB(v11, colores[fila][k + 1]);
                g_datoB = velocidades[ant][k]; VB(v00, colores[ant][k]);
                g_datoB = velocidades[fila][k + 1]; VB(v11, colores[fila][k + 1]);
                g_datoB = velocidades[fila][k]; VB(v01, colores[fila][k]);
            }
        }
    }

    g_modoB = 0.0f;
    g_datoB = 0.0f;
}


//--------------------------------------------------
// Escena estatica: orillas e isla central
//--------------------------------------------------

struct PuntoPerfilB
{
    float d;        // distancia desde el borde (hacia afuera del agua)
    float y;
    Color color;
    float ruido;    // amplitud del ruido de altura
};


// Franja de terreno extruida a lo largo del rio con el perfil dado. El
// borde del agua (d = 0) queda exacto en xBorde.
static void FranjaPerfilB(float xBorde, float dir, const PuntoPerfilB* perfil, int n, float p0, float p1, float paso, int semilla)
{
    int filas = (int)std::ceil((p1 - p0) / paso - 0.001f);
    if (filas < 1) filas = 1;
    float pasoReal = (p1 - p0) / (float)filas;

    auto Punto = [&](int i, float p, int fila)
    {
        float y = perfil[i].y;
        float d = perfil[i].d;

        if (perfil[i].ruido > 0.0f)
        {
            y += (Ruido01B(fila * 131 + i, semilla) - 0.5f) * 2.0f * perfil[i].ruido;
            if (i > 2) d += (Ruido01B(fila * 71 + i, semilla + 5) - 0.5f) * perfil[i].ruido * 0.8f;
        }

        return Vector3{ xBorde + dir * d, y, -p };
    };

    for (int f = 0; f < filas; f++)
    {
        float pa = p0 + pasoReal * (float)f;
        float pb = pa + pasoReal;
        int fa = (int)std::floor((pa + 100.0f) / paso + 0.5f);
        int fb = fa + 1;

        for (int i = 0; i < n - 1; i++)
        {
            Vector3 a0 = Punto(i, pa, fa), a1 = Punto(i + 1, pa, fa);
            Vector3 b0 = Punto(i, pb, fb), b1 = Punto(i + 1, pb, fb);
            float dx = std::fabs(a1.x - a0.x);
            float dy = a1.y - a0.y;
            float l = std::sqrt(dx * dx + dy * dy);
            Vector3 normal = l > 0.0001f ? Vector3{ -dir * dy / l, dx / l, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };

            Color ca = CaraB(VariarB(perfil[i].color, fa * 17 + i, semilla + 1, 0.07f), normal);
            Color cb = CaraB(VariarB(perfil[i + 1].color, fa * 17 + i + 1, semilla + 1, 0.07f), normal);
            Color cc = CaraB(VariarB(perfil[i + 1].color, fb * 17 + i + 1, semilla + 1, 0.07f), normal);
            Color cd = CaraB(VariarB(perfil[i].color, fb * 17 + i, semilla + 1, 0.07f), normal);
            Quad4B(a0, a1, b1, b0, ca, cb, cc, cd);
        }
    }
}


static void OrillasTramoB(float p0, float p1, bool decoracion)
{
    const float BORDE = CENTRO_CARRIL + MEDIO_ANCHO_CARRIL;     // 13.6
    const float ISLA = MEDIO_ANCHO_ISLA;                        // 2.4
    Color musgoPiedra = MezclaB(COLOR_PIEDRA_B, COLOR_MUSGO_B, 0.45f);
    Color sueloSelva = MezclaB(COLOR_TIERRA_B, COLOR_HOJA_OSCURA_B, 0.5f);

    const PuntoPerfilB exterior[] = {
        { -0.4f, -0.55f, COLOR_PIEDRA_MOJADA_B, 0.0f },
        { -0.04f, 0.1f, COLOR_PIEDRA_MOJADA_B, 0.0f },
        { 0.06f, 0.48f, COLOR_PIEDRA_B, 0.0f },
        { 0.7f, 0.58f, musgoPiedra, 0.04f },
        { 1.9f, 0.8f, COLOR_BARRO_B, 0.1f },
        { 3.8f, 1.2f, MezclaB(COLOR_BARRO_B, COLOR_MUSGO_B, 0.4f), 0.2f },
        { 6.8f, 1.9f, sueloSelva, 0.3f },
        { 10.5f, 2.8f, COLOR_HOJA_OSCURA_B, 0.4f },
        { 14.0f, 4.2f, COLOR_TIERRA_B, 0.6f },
        { 16.2f, 9.5f, COLOR_PIEDRA_B, 1.4f },
        { 18.8f, 15.5f, MezclaB(COLOR_PIEDRA_B, COLOR_MUSGO_B, 0.3f), 1.6f },
        { 22.5f, 18.0f, COLOR_MUSGO_B, 1.0f },
        { 42.0f, 20.0f, COLOR_HOJA_OSCURA_B, 1.0f }
    };
    const int nExterior = (int)(sizeof(exterior) / sizeof(exterior[0]));
    int nUsado = decoracion ? nExterior : nExterior - 1;

    FranjaPerfilB(-BORDE, -1.0f, exterior, nUsado, p0, p1, 1.0f, 301);
    FranjaPerfilB(BORDE, 1.0f, exterior, nUsado, p0, p1, 1.0f, 302);

    const PuntoPerfilB isla[] = {
        { -0.4f, -0.55f, COLOR_PIEDRA_MOJADA_B, 0.0f },
        { -0.04f, 0.1f, COLOR_PIEDRA_MOJADA_B, 0.0f },
        { 0.06f, 0.48f, COLOR_PIEDRA_B, 0.0f },
        { 0.55f, 0.58f, musgoPiedra, 0.03f },
        { 1.4f, 0.72f, MezclaB(COLOR_TIERRA_B, COLOR_MUSGO_B, 0.55f), 0.06f },
        { 2.4f, 0.8f, MezclaB(COLOR_TIERRA_B, COLOR_MUSGO_B, 0.45f), 0.0f }
    };

    FranjaPerfilB(-ISLA, 1.0f, isla, 6, p0, p1, 1.0f, 303);
    FranjaPerfilB(ISLA, -1.0f, isla, 6, p0, p1, 1.0f, 304);
}


//--------------------------------------------------
// Escena estatica: rocas e isla divisora
//--------------------------------------------------

// Roca del rio: piedra irregular con musgo arriba y la linea mojada abajo.
static void RocaB(Vector3 c, Vector3 r, int semilla, bool musgo = true)
{
    const int anillos = 5;
    const int lados = 9;

    auto P = [&](int i, int k)
    {
        float la = -0.5f * PI + PI * (float)i / (float)anillos;
        float lo = 2.0f * PI * (float)(k % lados) / (float)lados + (float)semilla;
        float f = (i > 0 && i < anillos) ? 0.86f + 0.24f * Ruido01B(i * 31 + (k % lados), semilla) : 1.0f;
        return Vector3{ c.x + r.x * f * std::cos(la) * std::cos(lo), c.y + r.y * f * std::sin(la), c.z + r.z * f * std::cos(la) * std::sin(lo) };
    };

    for (int i = 1; i < anillos; i++)
    {
        for (int k = 0; k < lados; k++)
        {
            Vector3 a = P(i, k), b = P(i, k + 1), d = P(i + 1, k + 1), e = P(i + 1, k);
            Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(e, a), Vector3Subtract(b, a)));
            if (n.y < 0.0f && i >= anillos - 1) n = Vector3Negate(n);
            float alto = (a.y + d.y) * 0.5f - c.y;
            Color base = VariarB(COLOR_PIEDRA_B, i * 13 + k, semilla, 0.08f);

            if (alto < r.y * 0.15f)
            {
                base = MezclaB(base, COLOR_PIEDRA_MOJADA_B, 0.65f);
            }
            else if (musgo && std::fabs(n.y) > 0.6f && i >= anillos - 2)
            {
                base = MezclaB(base, COLOR_MUSGO_B, 0.75f);
            }

            QuadB(a, b, d, e, CaraB(base, Vector3{ n.x, std::fabs(n.y), n.z }));
        }
    }
}


// Helecho: abanico de hojas que se arquean (se mecen con el shader).
static void HelechoB(Vector3 base, float tam, int semilla, Color color)
{
    int hojas = 7;
    float giro0 = Ruido01B(semilla, 41) * 6.28f;

    for (int h = 0; h < hojas; h++)
    {
        float a = giro0 + 6.2832f * (float)h / (float)hojas;
        float largo = tam * (0.75f + 0.5f * Ruido01B(semilla * 7 + h, 42));
        Vector3 dir = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 lado = { -dir.z * 0.16f * tam, 0.0f, dir.x * 0.16f * tam };
        Vector3 p0 = base;
        Vector3 p1 = { base.x + dir.x * largo * 0.45f, base.y + largo * 0.55f, base.z + dir.z * largo * 0.45f };
        Vector3 p2 = { base.x + dir.x * largo, base.y + largo * 0.25f, base.z + dir.z * largo };
        Color c = VariarB(color, semilla * 11 + h, 43, 0.1f);

        g_modoB = 3.0f;
        g_datoB = 0.0f;
        VB(p0, NieblaB(TonoB(c, 0.7f), p0));
        g_datoB = 0.4f;
        VB(Vector3Add(p1, lado), NieblaB(c, p1));
        VB(Vector3Subtract(p1, lado), NieblaB(TonoB(c, 0.85f), p1));
        VB(Vector3Add(p1, lado), NieblaB(c, p1));
        g_datoB = 1.0f;
        VB(p2, NieblaB(TonoB(c, 1.1f), p2));
        g_datoB = 0.4f;
        VB(Vector3Subtract(p1, lado), NieblaB(TonoB(c, 0.85f), p1));
        g_modoB = 0.0f;
        g_datoB = 0.0f;
    }
}


static void RocasTramoB(const MinijuegoBalsasRapido& m, float p0, float p1, bool decoracion)
{
    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);

        for (int k = 0; k < m.cantidadObstaculos; k++)
        {
            const ObstaculoBalsas& o = m.obstaculos[k];

            if (o.p < p0 || o.p >= p1)
            {
                continue;
            }

            if (o.tipo == OBSTACULO_BALSAS_ROCA)
            {
                float a = o.a;
                int semilla = k * 7 + e * 3 + 11;
                RocaB({ c + o.desvio, -0.15f, -o.p }, { a * 0.98f, a * 0.95f, a * 0.95f }, semilla);

                if (a > 0.7f)
                {
                    RocaB({ c + o.desvio + a * 0.3f, a * 0.45f, -o.p + a * 0.15f }, { a * 0.5f, a * 0.42f, a * 0.48f }, semilla + 1);
                    if (decoracion) HelechoB({ c + o.desvio - a * 0.2f, a * 0.72f, -o.p - a * 0.1f }, 0.45f, semilla, COLOR_HOJA_CLARA_B);
                }
            }
        }
    }
}


// Isla divisora del rapido (del tamano exacto de su rectangulo) con el
// totem de la bifurcacion en la proa: flecha clara a la cascada y verde
// al remanso.
static void DivisorTramoB(const MinijuegoBalsasRapido& m, float p0, float p1, bool decoracion)
{
    for (int k = 0; k < m.cantidadObstaculos; k++)
    {
        const ObstaculoBalsas& o = m.obstaculos[k];

        if (o.tipo != OBSTACULO_BALSAS_DIVISOR)
        {
            continue;
        }

        for (int e = 0; e < 2; e++)
        {
            float x = CentroCarril(e) + o.desvio;
            float pIni = o.p - o.b;
            float pFin = o.p + o.b;

            // Lomo de rocas a lo largo de la isla.
            int bloques = (int)((pFin - pIni) / 1.5f);

            for (int b = 0; b <= bloques; b++)
            {
                float p = pIni + 0.6f + (pFin - pIni - 1.2f) * (float)b / (float)bloques;

                if (p < p0 || p >= p1)
                {
                    continue;
                }

                int s = 500 + b * 5 + e * 211;
                float alto = 0.55f + 0.35f * Ruido01B(s, 3);
                RocaB({ x + (Ruido01B(s, 4) - 0.5f) * 0.1f, -0.1f, -p }, { o.a * 0.98f, alto, 0.95f }, s);

                if (decoracion && b % 3 == 1)
                {
                    HelechoB({ x, alto * 0.8f, -p }, 0.55f, s, COLOR_HOJA_B);
                }
            }

            // Proa y totem de la bifurcacion.
            if (pIni >= p0 && pIni < p1)
            {
                RocaB({ x, -0.1f, -(pIni + 0.55f) }, { o.a * 0.9f, 0.7f, 0.65f }, 900 + e);

                Vector3 pie = { x, 0.45f, -(pIni + 0.7f) };
                CajaB({ pie.x, pie.y + 0.15f, pie.z }, { 0.9f, 0.3f, 0.9f }, COLOR_RUINA_B);
                CajaB({ pie.x, pie.y + 1.55f, pie.z }, { 0.52f, 2.5f, 0.52f }, VariarB(COLOR_RUINA_B, e, 77, 0.05f));
                // Cara tallada mirando corriente arriba (+z).
                CajaB({ pie.x - 0.12f, pie.y + 2.3f, pie.z + 0.27f }, { 0.1f, 0.08f, 0.04f }, COLOR_PIEDRA_MOJADA_B);
                CajaB({ pie.x + 0.12f, pie.y + 2.3f, pie.z + 0.27f }, { 0.1f, 0.08f, 0.04f }, COLOR_PIEDRA_MOJADA_B);
                CajaB({ pie.x, pie.y + 2.12f, pie.z + 0.29f }, { 0.08f, 0.16f, 0.08f }, TonoB(COLOR_RUINA_B, 0.85f));
                CajaB({ pie.x, pie.y + 1.92f, pie.z + 0.27f }, { 0.28f, 0.06f, 0.04f }, COLOR_PIEDRA_MOJADA_B);
                CajaB({ pie.x, pie.y + 2.9f, pie.z }, { 0.7f, 0.18f, 0.7f }, MezclaB(COLOR_RUINA_B, COLOR_MUSGO_B, 0.5f));

                // Flechas: cascada (izquierda, clara con olas) y remanso
                // (derecha, verde con hoja).
                for (int lado = -1; lado <= 1; lado += 2)
                {
                    Color tabla = lado < 0 ? Color{ 214, 238, 236, 255 } : Color{ 112, 180, 76, 255 };
                    float y = pie.y + (lado < 0 ? 1.62f : 1.12f);
                    float x0 = pie.x + (float)lado * 0.26f;
                    float x1 = pie.x + (float)lado * 1.0f;
                    float z = pie.z + 0.28f;
                    CajaB({ (x0 + x1) * 0.5f, y, z }, { std::fabs(x1 - x0), 0.34f, 0.07f }, tabla);
                    Vector3 a = { x1, y + 0.3f, z + 0.04f };
                    Vector3 b = { x1 + (float)lado * 0.4f, y, z + 0.04f };
                    Vector3 d = { x1, y - 0.3f, z + 0.04f };
                    TriB(a, b, d, tabla);
                    TriB(a, d, b, tabla);

                    if (lado < 0)
                    {
                        for (int w = 0; w < 3; w++)
                        {
                            float wx = x0 - 0.15f - (float)w * 0.22f;
                            CajaB({ wx, y + 0.04f * (float)(w % 2 == 0 ? 1 : -1), z + 0.045f }, { 0.16f, 0.05f, 0.02f }, Color{ 60, 150, 170, 255 });
                        }
                    }
                    else
                    {
                        CajaB({ x0 + 0.35f, y, z + 0.045f }, { 0.3f, 0.12f, 0.02f }, COLOR_HOJA_OSCURA_B);
                    }
                }
            }

            if (decoracion)
            {
                static const float PALMITAS[2] = { 61.0f, 73.5f };
                for (int q = 0; q < 2; q++)
                {
                    float p = PALMITAS[q];
                    if (p < p0 || p >= p1) continue;
                    Vector3 base = { x, 0.6f, -p };
                    Vector3 punta = { x + 0.15f * (float)(q == 0 ? 1 : -1), 2.6f, -p - 0.2f };
                    CilB(base, punta, 0.12f, 0.08f, 6, COLOR_TRONCO_B);
                    HelechoB(punta, 1.1f, 700 + q + e * 9, COLOR_HOJA_CLARA_B);
                }
            }
        }
    }
}


//--------------------------------------------------
// Escena estatica: plantas, arboles y ruinas
//--------------------------------------------------

// Altura del terreno de la orilla exterior a d unidades del agua (sin el
// ruido del perfil; las piezas se hunden un poco para no flotar).
static float AlturaExteriorB(float d)
{
    static const float D[] = { 0.06f, 0.7f, 1.9f, 3.8f, 6.8f, 10.5f, 14.0f, 16.2f, 18.8f, 22.5f, 42.0f };
    static const float Y[] = { 0.48f, 0.58f, 0.8f, 1.2f, 1.9f, 2.8f, 4.2f, 9.5f, 15.5f, 18.0f, 20.0f };
    const int n = 11;

    if (d <= D[0]) return Y[0];

    for (int i = 0; i < n - 1; i++)
    {
        if (d <= D[i + 1])
        {
            float t = (d - D[i]) / (D[i + 1] - D[i]);
            return Y[i] + (Y[i + 1] - Y[i]) * t;
        }
    }

    return Y[n - 1];
}


// Liana colgante: dos tiras cruzadas con hojitas; se mece mas abajo.
static void LianaB(Vector3 arriba, float largo, int semilla)
{
    const int tramos = 6;
    Color verde = VariarB(Color{ 58, 104, 44, 255 }, semilla, 61, 0.12f);
    float curva = (Ruido01B(semilla, 62) - 0.5f) * 0.6f;

    g_modoB = 3.0f;

    for (int t = 0; t < tramos; t++)
    {
        float u0 = (float)t / (float)tramos, u1 = (float)(t + 1) / (float)tramos;
        Vector3 a = { arriba.x + curva * std::sin(u0 * 3.0f), arriba.y - largo * u0, arriba.z };
        Vector3 b = { arriba.x + curva * std::sin(u1 * 3.0f), arriba.y - largo * u1, arriba.z };
        float w = 0.05f;
        Color c0 = TonoB(verde, 0.85f + 0.15f * u0), c1 = TonoB(verde, 0.85f + 0.15f * u1);

        g_datoB = u0 * u0;
        VB({ a.x - w, a.y, a.z }, c0); VB({ a.x + w, a.y, a.z }, c0);
        g_datoB = u1 * u1;
        VB({ b.x + w, b.y, b.z }, c1);
        g_datoB = u0 * u0;
        VB({ a.x - w, a.y, a.z }, c0);
        g_datoB = u1 * u1;
        VB({ b.x + w, b.y, b.z }, c1); VB({ b.x - w, b.y, b.z }, c1);

        g_datoB = u0 * u0;
        VB({ a.x, a.y, a.z - w }, c0); VB({ a.x, a.y, a.z + w }, c0);
        g_datoB = u1 * u1;
        VB({ b.x, b.y, b.z + w }, c1);
        g_datoB = u0 * u0;
        VB({ a.x, a.y, a.z - w }, c0);
        g_datoB = u1 * u1;
        VB({ b.x, b.y, b.z + w }, c1); VB({ b.x, b.y, b.z - w }, c1);

        // Hojita.
        if (t % 2 == 1)
        {
            float lado = (t % 4 == 1) ? 1.0f : -1.0f;
            Color hoja = VariarB(COLOR_HOJA_CLARA_B, semilla + t, 63, 0.12f);
            g_datoB = u1 * u1;
            VB(b, hoja);
            VB({ b.x + lado * 0.32f, b.y + 0.05f, b.z + 0.1f }, TonoB(hoja, 0.85f));
            VB({ b.x + lado * 0.2f, b.y - 0.18f, b.z - 0.08f }, hoja);
        }
    }

    g_modoB = 0.0f;
    g_datoB = 0.0f;
}


// Hoja grande (oreja de elefante): rombo arqueado que se mece.
static void HojaGrandeB(Vector3 base, float giro, float largo, float ancho, float alzada, Color color)
{
    Vector3 dir = { std::cos(giro), 0.0f, std::sin(giro) };
    Vector3 lado = { -dir.z, 0.0f, dir.x };
    Vector3 tallo = { base.x + dir.x * largo * 0.15f, base.y + alzada, base.z + dir.z * largo * 0.15f };
    Vector3 medio = { base.x + dir.x * largo * 0.55f, base.y + alzada + largo * 0.12f, base.z + dir.z * largo * 0.55f };
    Vector3 punta = { base.x + dir.x * largo, base.y + alzada - largo * 0.18f, base.z + dir.z * largo };
    Vector3 izq = Vector3Add(medio, Vector3Scale(lado, ancho));
    Vector3 der = Vector3Subtract(medio, Vector3Scale(lado, ancho));

    g_modoB = 3.0f;
    g_datoB = 0.0f;
    VB(base, NieblaB(TonoB(color, 0.6f), base));
    g_datoB = 0.2f;
    VB(tallo, NieblaB(TonoB(color, 0.75f), tallo));
    VB(tallo, NieblaB(TonoB(color, 0.75f), tallo));

    g_datoB = 0.2f; VB(tallo, NieblaB(TonoB(color, 0.8f), tallo));
    g_datoB = 0.7f; VB(izq, NieblaB(TonoB(color, 1.08f), izq));
    g_datoB = 1.0f; VB(punta, NieblaB(color, punta));
    g_datoB = 0.2f; VB(tallo, NieblaB(TonoB(color, 0.8f), tallo));
    g_datoB = 1.0f; VB(punta, NieblaB(color, punta));
    g_datoB = 0.7f; VB(der, NieblaB(TonoB(color, 0.88f), der));
    g_modoB = 0.0f;
    g_datoB = 0.0f;

    // Nervio central.
    CajaEjesB(Vector3Scale(Vector3Add(tallo, punta), 0.5f), { 0.02f, 0.0f, 0.0f }, { 0.0f, 0.02f, 0.0f }, Vector3Scale(Vector3Subtract(punta, tallo), 0.5f), TonoB(color, 1.3f));
}


static void MataHojasB(Vector3 base, float tam, int semilla)
{
    int hojas = 5;
    for (int h = 0; h < hojas; h++)
    {
        float giro = 6.2832f * ((float)h / (float)hojas) + Ruido01B(semilla + h, 71) * 0.8f;
        float largo = tam * (0.8f + 0.4f * Ruido01B(semilla + h, 72));
        Color c = MezclaB(COLOR_HOJA_B, COLOR_HOJA_CLARA_B, Ruido01B(semilla + h, 73) * 0.7f);
        HojaGrandeB(base, giro, largo, largo * 0.35f, tam * (0.35f + 0.4f * Ruido01B(semilla + h, 74)), c);
    }
}


static void ArbustoB(Vector3 base, float tam, int semilla)
{
    for (int k = 0; k < 3; k++)
    {
        float a = (float)k * 2.1f + Ruido01B(semilla, 81) * 3.0f;
        Vector3 c = { base.x + std::cos(a) * tam * 0.45f, base.y + tam * (0.45f + 0.15f * (float)k), base.z + std::sin(a) * tam * 0.45f };
        Color color = MezclaB(COLOR_HOJA_OSCURA_B, COLOR_HOJA_B, Ruido01B(semilla + k, 82));
        BolaB(c, { tam * 0.62f, tam * 0.5f, tam * 0.62f }, color, 3, 7, 0.14f, semilla + k);
    }
}


// Juncos de la orilla.
static void JuncosB(Vector3 base, int semilla)
{
    g_modoB = 3.0f;
    for (int k = 0; k < 6; k++)
    {
        float ox = (Ruido01B(semilla + k, 91) - 0.5f) * 0.7f;
        float oz = (Ruido01B(semilla + k, 92) - 0.5f) * 0.7f;
        float alto = 0.9f + 0.8f * Ruido01B(semilla + k, 93);
        Color c = MezclaB(Color{ 110, 140, 60, 255 }, Color{ 150, 160, 80, 255 }, Ruido01B(semilla + k, 94));
        Vector3 a = { base.x + ox - 0.03f, base.y, base.z + oz };
        Vector3 b = { base.x + ox + 0.03f, base.y, base.z + oz };
        Vector3 t = { base.x + ox * 1.4f, base.y + alto, base.z + oz * 1.4f };
        g_datoB = 0.0f; VB(a, NieblaB(TonoB(c, 0.7f), a)); VB(b, NieblaB(TonoB(c, 0.7f), b));
        g_datoB = 1.0f; VB(t, NieblaB(c, t));
    }
    g_modoB = 0.0f;
    g_datoB = 0.0f;
}


// Arbol gigante de la selva: tronco con raices tabulares, ramas, copa de
// varias masas y lianas que cuelgan.
static void ArbolGiganteB(Vector3 base, float alto, float radio, int semilla, bool detalle)
{
    float incl = (Ruido01B(semilla, 101) - 0.5f) * 0.12f * alto;
    Vector3 cima = { base.x + incl, base.y + alto, base.z + (Ruido01B(semilla, 102) - 0.5f) * 1.0f };
    Color corteza = VariarB(COLOR_TRONCO_B, semilla, 103, 0.1f);

    CilB({ base.x, base.y - 0.6f, base.z }, cima, radio, radio * 0.62f, 8, corteza, false);

    // Raices tabulares.
    for (int r = 0; r < 4; r++)
    {
        float a = (float)r * 1.5708f + Ruido01B(semilla + r, 104) * 0.6f;
        Vector3 dir = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 pie = { base.x + dir.x * radio * 0.8f, base.y - 0.2f, base.z + dir.z * radio * 0.8f };
        Vector3 alto0 = { base.x + dir.x * radio * 0.8f, base.y + 2.4f, base.z + dir.z * radio * 0.8f };
        Vector3 lejos = { base.x + dir.x * radio * 2.6f, base.y - 0.2f, base.z + dir.z * radio * 2.6f };
        Color c = CaraB(corteza, { dir.z, 0.3f, -dir.x });
        TriB(pie, alto0, lejos, c);
        TriB(pie, lejos, alto0, TonoB(c, 0.9f));
    }

    // Ramas y copa.
    int masas = detalle ? 5 : 3;
    for (int k = 0; k < masas; k++)
    {
        float a = 6.2832f * (float)k / (float)masas + Ruido01B(semilla + k, 105);
        float r = 2.0f + 1.6f * Ruido01B(semilla + k, 106);
        Vector3 c = { cima.x + std::cos(a) * r * 0.8f, cima.y + 0.6f + 1.4f * Ruido01B(semilla + k, 107), cima.z + std::sin(a) * r * 0.8f };
        Color hoja = MezclaB(COLOR_HOJA_OSCURA_B, COLOR_HOJA_B, 0.3f + 0.6f * Ruido01B(semilla + k, 108));

        if (detalle)
        {
            VigaB({ cima.x, cima.y - 1.2f, cima.z }, { c.x, c.y - 0.6f, c.z }, radio * 0.32f, corteza);
        }

        BolaB(c, { r * 1.2f, r * 0.62f, r * 1.2f }, hoja, 3, 8, 0.16f, semilla + k);

        if (detalle)
        {
            LianaB({ c.x + std::cos(a) * r * 0.6f, c.y - r * 0.4f, c.z + std::sin(a) * r * 0.6f }, 3.5f + 4.5f * Ruido01B(semilla + k, 109), semilla * 3 + k);
        }
    }

    BolaB({ cima.x, cima.y + 1.6f, cima.z }, { 2.8f, 1.5f, 2.8f }, MezclaB(COLOR_HOJA_B, COLOR_HOJA_CLARA_B, 0.35f), 3, 8, 0.12f, semilla + 50);

    // Helecho epifito en el tronco.
    if (detalle)
    {
        HelechoB({ base.x + incl * 0.4f, base.y + alto * 0.42f, base.z + radio * 0.8f }, 0.7f, semilla, COLOR_HOJA_CLARA_B);
    }
}


// Palmera curva con penacho de hojas que se mecen.
static void PalmeraB(Vector3 base, float alto, float inclX, int semilla)
{
    const int seg = 5;
    Vector3 ant = base;
    Color tronco = VariarB(Color{ 128, 102, 72, 255 }, semilla, 111, 0.08f);

    for (int s = 1; s <= seg; s++)
    {
        float u = (float)s / (float)seg;
        Vector3 p = { base.x + inclX * u * u * alto * 0.35f, base.y + alto * u, base.z + 0.2f * std::sin(u * 2.0f + (float)semilla) };
        CilB(ant, p, 0.17f - 0.04f * u, 0.15f - 0.04f * u, 6, s % 2 == 0 ? tronco : TonoB(tronco, 0.88f), false);
        ant = p;
    }

    int hojas = 8;
    g_modoB = 3.0f;

    for (int h = 0; h < hojas; h++)
    {
        float a = 6.2832f * (float)h / (float)hojas + Ruido01B(semilla, 112);
        Vector3 dir = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 lado = { -dir.z, 0.0f, dir.x };
        float largo = 2.0f + 0.6f * Ruido01B(semilla + h, 113);
        Color c = MezclaB(COLOR_HOJA_B, COLOR_HOJA_CLARA_B, Ruido01B(semilla + h, 114));
        Vector3 prev = ant;

        for (int t = 1; t <= 4; t++)
        {
            float u = (float)t / 4.0f;
            float u0 = (float)(t - 1) / 4.0f;
            Vector3 p = { ant.x + dir.x * largo * u, ant.y + 0.5f * u - 1.5f * u * u, ant.z + dir.z * largo * u };
            float w0 = 0.42f * (1.0f - u0 * 0.7f), w1 = 0.42f * (1.0f - u * 0.7f);
            Vector3 a0 = Vector3Add(prev, Vector3Scale(lado, w0)), b0 = Vector3Subtract(prev, Vector3Scale(lado, w0));
            Vector3 a1 = Vector3Add(p, Vector3Scale(lado, w1)), b1 = Vector3Subtract(p, Vector3Scale(lado, w1));
            a0.y -= 0.12f; b0.y -= 0.12f; a1.y -= 0.12f; b1.y -= 0.12f;
            Color c0 = TonoB(c, 0.85f + 0.2f * u0), c1 = TonoB(c, 0.85f + 0.2f * u);

            g_datoB = u0; VB(prev, NieblaB(c0, prev)); VB(a0, NieblaB(c0, a0));
            g_datoB = u; VB(a1, NieblaB(c1, a1));
            g_datoB = u0; VB(prev, NieblaB(c0, prev));
            g_datoB = u; VB(a1, NieblaB(c1, a1)); VB(p, NieblaB(c1, p));
            g_datoB = u0; VB(prev, NieblaB(TonoB(c0, 0.85f), prev));
            g_datoB = u; VB(p, NieblaB(TonoB(c1, 0.85f), p)); VB(b1, NieblaB(TonoB(c1, 0.85f), b1));
            g_datoB = u0; VB(prev, NieblaB(TonoB(c0, 0.85f), prev));
            g_datoB = u; VB(b1, NieblaB(TonoB(c1, 0.85f), b1));
            g_datoB = u0; VB(b0, NieblaB(TonoB(c0, 0.85f), b0));
            prev = p;
        }
    }

    g_modoB = 0.0f;
    g_datoB = 0.0f;

    for (int k = 0; k < 3; k++)
    {
        float a = (float)k * 2.1f;
        BolaB({ ant.x + std::cos(a) * 0.18f, ant.y - 0.15f, ant.z + std::sin(a) * 0.18f }, { 0.13f, 0.13f, 0.13f }, Color{ 110, 84, 46, 255 }, 2, 5);
    }
}


static void ColumnaRotaB(Vector3 base, float alto, int semilla, bool caida)
{
    int tambores = (int)(alto / 0.55f);
    if (tambores < 1) tambores = 1;
    float y = base.y;

    CajaB({ base.x, base.y + 0.12f, base.z }, { 1.0f, 0.24f, 1.0f }, VariarB(COLOR_RUINA_B, semilla, 121, 0.06f));
    y += 0.24f;

    for (int t = 0; t < tambores; t++)
    {
        float ox = (Ruido01B(semilla + t, 122) - 0.5f) * 0.06f;
        Color c = VariarB(COLOR_RUINA_B, semilla + t, 123, 0.08f);
        if (t == tambores - 1) c = MezclaB(c, COLOR_MUSGO_B, 0.35f);
        CilB({ base.x + ox, y, base.z }, { base.x + ox, y + 0.52f, base.z }, 0.36f, 0.36f, 10, c);
        y += 0.55f;
    }

    // Musgo que cuelga del corte.
    CilB({ base.x, y - 0.02f, base.z }, { base.x, y + 0.06f, base.z }, 0.38f, 0.3f, 10, COLOR_MUSGO_B);

    if (caida)
    {
        float a = Ruido01B(semilla, 124) * 6.28f;
        Vector3 d = { std::cos(a) * 0.6f, 0.0f, std::sin(a) * 0.6f };
        Vector3 c0 = { base.x + std::cos(a) * 1.1f, base.y + 0.3f, base.z + std::sin(a) * 1.1f };
        CilB(Vector3Subtract(c0, d), Vector3Add(c0, d), 0.33f, 0.33f, 10, MezclaB(COLOR_RUINA_B, COLOR_MUSGO_B, 0.25f));
    }
}


// Cabeza de piedra tallada (mira hacia "giro").
static void CabezaPiedraB(Vector3 base, float tam, float giro, int semilla)
{
    Color piedra = VariarB(Color{ 134, 132, 112, 255 }, semilla, 131, 0.06f);
    Vector3 c = { base.x, base.y + tam * 0.85f, base.z };
    Vector3 f = { std::cos(giro), 0.0f, std::sin(giro) };
    Vector3 l = { -f.z, 0.0f, f.x };

    BolaB(c, { tam, tam * 1.05f, tam }, piedra, 4, 10, 0.03f, semilla);
    // Casco y musgo.
    BolaB({ c.x - f.x * tam * 0.05f, c.y + tam * 0.45f, c.z - f.z * tam * 0.05f }, { tam * 1.04f, tam * 0.55f, tam * 1.04f }, MezclaB(piedra, COLOR_MUSGO_B, 0.55f), 3, 10, 0.06f, semilla + 1);

    auto Rasgo = [&](float lx, float ly, float fz, Vector3 t, Color col)
    {
        Vector3 p = { c.x + l.x * lx * tam + f.x * fz * tam, c.y + ly * tam, c.z + l.z * lx * tam + f.z * fz * tam };
        CajaEjesB(p, Vector3Scale(l, t.x * tam * 0.5f), { 0.0f, t.y * tam * 0.5f, 0.0f }, Vector3Scale(f, t.z * tam * 0.5f), col);
    };

    Color oscuro = TonoB(piedra, 0.45f);
    Rasgo(-0.36f, 0.12f, 0.9f, { 0.28f, 0.1f, 0.12f }, oscuro);   // ojos
    Rasgo(0.36f, 0.12f, 0.9f, { 0.28f, 0.1f, 0.12f }, oscuro);
    Rasgo(0.0f, -0.12f, 0.98f, { 0.24f, 0.32f, 0.2f }, TonoB(piedra, 1.08f));   // nariz
    Rasgo(0.0f, -0.48f, 0.88f, { 0.5f, 0.12f, 0.16f }, TonoB(piedra, 0.6f));    // boca
    Rasgo(-0.98f, -0.05f, 0.0f, { 0.12f, 0.4f, 0.3f }, TonoB(piedra, 0.9f));    // orejas
    Rasgo(0.98f, -0.05f, 0.0f, { 0.12f, 0.4f, 0.3f }, TonoB(piedra, 0.9f));
}


// Templete escalonado con escalinata hacia el rio y puerta oscura.
static void TemploB(Vector3 base, float ancho, int niveles, float giro, int semilla)
{
    Vector3 f = { std::cos(giro), 0.0f, std::sin(giro) };
    Vector3 l = { -f.z, 0.0f, f.x };
    float y = base.y;
    float a = ancho;

    for (int n = 0; n < niveles; n++)
    {
        float h = 0.9f;
        Color c = VariarB(COLOR_RUINA_B, semilla + n, 141, 0.06f);
        c = MezclaB(c, COLOR_MUSGO_B, 0.12f + 0.08f * (float)n);
        CajaEjesB({ base.x, y + h * 0.5f, base.z }, Vector3Scale(l, a * 0.5f), { 0.0f, h * 0.5f, 0.0f }, Vector3Scale(f, a * 0.5f), c);
        // Franja de grecas.
        CajaEjesB({ base.x + f.x * a * 0.5f, y + h * 0.62f, base.z + f.z * a * 0.5f }, Vector3Scale(l, a * 0.45f), { 0.0f, 0.1f, 0.0f }, Vector3Scale(f, 0.04f), TonoB(c, 0.7f));
        y += h;
        a -= 1.3f;
    }

    // Escalinata.
    int escalones = niveles * 3;
    for (int s = 0; s < escalones; s++)
    {
        float h = (float)niveles * 0.9f / (float)escalones;
        float sale = ancho * 0.5f + 0.9f - (float)s * (1.3f * 0.5f * (float)niveles / (float)escalones + 0.05f);
        Vector3 c = { base.x + f.x * sale, base.y + h * ((float)s + 0.5f), base.z + f.z * sale };
        CajaEjesB(c, Vector3Scale(l, 0.8f), { 0.0f, h * 0.5f, 0.0f }, Vector3Scale(f, 0.35f), VariarB(COLOR_RUINA_B, s, 142, 0.05f));
    }

    // Santuario con puerta.
    Vector3 cima = { base.x, y, base.z };
    CajaEjesB({ cima.x, cima.y + 0.8f, cima.z }, Vector3Scale(l, a * 0.4f), { 0.0f, 0.8f, 0.0f }, Vector3Scale(f, a * 0.35f), VariarB(COLOR_RUINA_B, semilla, 143, 0.05f));
    CajaEjesB({ cima.x + f.x * a * 0.36f, cima.y + 0.6f, cima.z + f.z * a * 0.36f }, Vector3Scale(l, 0.35f), { 0.0f, 0.6f, 0.0f }, Vector3Scale(f, 0.03f), Color{ 30, 28, 24, 255 });
    CajaEjesB({ cima.x, cima.y + 1.7f, cima.z }, Vector3Scale(l, a * 0.45f), { 0.0f, 0.12f, 0.0f }, Vector3Scale(f, a * 0.4f), MezclaB(COLOR_RUINA_B, COLOR_MUSGO_B, 0.5f));
}


// Peñon junto a la orilla con una cascada que cae al rio.
static void PenonCascadaB(float lado, float p, int semilla, bool detalle)
{
    const float BORDE = CENTRO_CARRIL + MEDIO_ANCHO_CARRIL;
    float xBase = lado * (BORDE + 2.7f);
    float alto = 6.2f;

    RocaB({ xBase, 0.0f, -p }, { 2.2f, alto * 0.62f, 2.0f }, semilla);
    RocaB({ xBase + lado * 0.6f, alto * 0.55f, -p + 0.3f }, { 1.8f, alto * 0.45f, 1.7f }, semilla + 1);
    RocaB({ xBase - lado * 0.2f, 0.2f, -p - 2.0f }, { 1.3f, 1.6f, 1.2f }, semilla + 2);
    RocaB({ xBase - lado * 0.1f, 0.2f, -p + 2.1f }, { 1.2f, 1.9f, 1.1f }, semilla + 3);

    if (detalle)
    {
        ArbustoB({ xBase + lado * 0.6f, alto + 0.4f, -p - 0.5f }, 1.1f, semilla);
        HelechoB({ xBase - lado * 0.8f, alto * 0.7f, -p + 1.2f }, 0.8f, semilla, COLOR_HOJA_CLARA_B);
        LianaB({ xBase - lado * 1.3f, alto + 0.2f, -p - 1.2f }, 3.2f, semilla);
    }

    // Lamina de agua: sale del labio y cae en curva hasta el borde del rio.
    g_modoB = 4.0f;
    g_datoB = 2.6f;
    const int seg = 6;
    float ancho = 0.85f;
    Color agua = { 196, 230, 228, 255 };

    for (int s = 0; s < seg; s++)
    {
        float u0 = (float)s / (float)seg, u1 = (float)(s + 1) / (float)seg;
        float x0 = xBase - lado * (1.5f + 1.2f * std::sqrt(u0)), x1 = xBase - lado * (1.5f + 1.2f * std::sqrt(u1));
        float y0 = alto + 0.2f - (alto + 0.3f) * u0 * u0 * 0.6f - (alto + 0.3f) * u0 * 0.4f;
        float y1 = alto + 0.2f - (alto + 0.3f) * u1 * u1 * 0.6f - (alto + 0.3f) * u1 * 0.4f;
        float w0 = ancho * (1.0f + 0.4f * u0), w1 = ancho * (1.0f + 0.4f * u1);
        Color c0 = TonoB(agua, 0.92f + 0.08f * u0), c1 = TonoB(agua, 0.92f + 0.08f * u1);
        QuadColoresB({ x0, y0, -p - w0 }, { x0, y0, -p + w0 }, { x1, y1, -p + w1 }, { x1, y1, -p - w1 }, c0, c0, c1, c1);
    }

    g_modoB = 0.0f;
    g_datoB = 0.0f;

    // Poza de espuma al pie.
    DiscoB({ lado * (BORDE + 0.1f), 0.02f, -p }, 1.5f, 1.7f, 12, COLOR_ESPUMA_B, AlfaB(COLOR_ESPUMA_B, 0));
}

static void EscenaTramoB(const MinijuegoBalsasRapido& m, float p0, float p1, bool decoracion, int paso);
static void EscenaLejanaB(bool decoracion);
static void CieloEstaticoB(bool decoracion);


static int ClaveHornoB()
{
    return (int)CalidadDecoracion() * 2 + (CalidadSombras() != CALIDAD_BAJA ? 1 : 0);
}


static void HornearBalsas(const MinijuegoBalsasRapido& m)
{
    int clave = ClaveHornoB();

    if ((g_hornoB.listo && g_hornoB.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasBalsas();

    if (!g_hornoB.shaderListo)
    {
        g_hornoB.shader = LoadShaderFromMemory(VS_HORNO_B, FS_HORNO_B);
        g_hornoB.locOjo = GetShaderLocation(g_hornoB.shader, "ojo");
        g_hornoB.locNiebla = GetShaderLocation(g_hornoB.shader, "niebla");
        g_hornoB.locTiempo = GetShaderLocation(g_hornoB.shader, "tiempo");
        g_hornoB.material = LoadMaterialDefault();
        g_hornoB.material.shader = g_hornoB.shader;
        g_hornoB.shaderListo = true;
    }

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    for (int c = 0; c < TRAMOS_B; c++)
    {
        float p0 = P_INICIO_HORNO_B + LARGO_TRAMO_B * (float)c;
        float p1 = p0 + LARGO_TRAMO_B;

        EmpezarGrabacionB();
        EscenaTramoB(m, p0, p1, decoracion, paso);
        g_hornoB.tramoValido[c] = TerminarGrabacionB(g_hornoB.tramos[c]);
    }

    EmpezarGrabacionB();
    EscenaLejanaB(decoracion);
    g_hornoB.lejosValido = TerminarGrabacionB(g_hornoB.lejos);

    EmpezarGrabacionB();
    CieloEstaticoB(decoracion);
    g_hornoB.cieloValido = TerminarGrabacionB(g_hornoB.cielo);

    g_hornoB.clave = clave;
    g_hornoB.listo = true;
}


static void UsarShaderB(float tiempo)
{
    rlDrawRenderBatchActive();
    float ojo[3] = { g_ojoB.x, g_ojoB.y, g_ojoB.z };
    float niebla[4] = { COLOR_NIEBLA_B.r / 255.0f, COLOR_NIEBLA_B.g / 255.0f, COLOR_NIEBLA_B.b / 255.0f, 1.0f };
    SetShaderValue(g_hornoB.shader, g_hornoB.locOjo, ojo, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_hornoB.shader, g_hornoB.locNiebla, niebla, SHADER_UNIFORM_VEC4);
    SetShaderValue(g_hornoB.shader, g_hornoB.locTiempo, &tiempo, SHADER_UNIFORM_FLOAT);
}


// Tramos visibles de cerca a lejos (asi la prueba de profundidad descarta
// lo tapado antes de pintarlo) y despues la lejania.
static void DibujarHornoB(float pDesde, float pHasta, float tiempo)
{
    if (!g_hornoB.listo)
    {
        return;
    }

    UsarShaderB(tiempo);
    Matrix identidad = MatrixIdentity();

    for (int c = 0; c < TRAMOS_B; c++)
    {
        float p0 = P_INICIO_HORNO_B + LARGO_TRAMO_B * (float)c;

        if (p0 + LARGO_TRAMO_B < pDesde || p0 > pHasta || !g_hornoB.tramoValido[c])
        {
            continue;
        }

        DrawMesh(g_hornoB.tramos[c], g_hornoB.material, identidad);
    }

    if (g_hornoB.lejosValido) DrawMesh(g_hornoB.lejos, g_hornoB.material, identidad);
}


// El cielo va despues de todo lo opaco: solo se pinta donde no hay nada.
static void DibujarCieloHornoB(float tiempo)
{
    if (!g_hornoB.listo || !g_hornoB.cieloValido)
    {
        return;
    }

    UsarShaderB(tiempo);
    DrawMesh(g_hornoB.cielo, g_hornoB.material, MatrixIdentity());
}


//--------------------------------------------------
// Escena por tramo: isla central, selva de las orillas, salida y meta
//--------------------------------------------------

// Fuegos (antorchas y braseros): la base se hornea, la llama se anima.
static const Vector3 FUEGOS_B[] = {
    { -14.4f, 3.55f, -118.95f }, { 14.4f, 3.55f, -118.95f }, { 0.0f, 3.55f, -118.95f },
    { 1.15f, 1.55f, -38.0f }, { -1.15f, 1.55f, -84.0f }, { 1.15f, 1.55f, -102.0f },
    { -15.6f, 1.45f, 1.6f }, { 15.6f, 1.45f, 1.6f }
};
static const int CANTIDAD_FUEGOS_B = (int)(sizeof(FUEGOS_B) / sizeof(FUEGOS_B[0]));

struct PiezaOrillaB
{
    float p;
    float lado;
    int tipo;       // 0 templo, 1 cabeza, 2 columnas, 3 peñon con cascada
};

static const PiezaOrillaB PIEZAS_ORILLA_B[] = {
    { 12.0f, -1.0f, 2 }, { 22.0f, 1.0f, 1 }, { 30.0f, -1.0f, 3 }, { 46.0f, -1.0f, 0 },
    { 58.0f, 1.0f, 2 }, { 72.0f, -1.0f, 1 }, { 86.0f, 1.0f, 3 }, { 96.0f, -1.0f, 2 },
    { 104.0f, 1.0f, 0 }, { 132.0f, 1.0f, 1 }, { 140.0f, -1.0f, 3 }, { 150.0f, 1.0f, 2 },
    { 162.0f, -1.0f, 0 }, { 188.0f, 1.0f, 3 }, { 205.0f, -1.0f, 1 }, { 214.0f, 1.0f, 0 }
};
static const int CANTIDAD_PIEZAS_ORILLA_B = (int)(sizeof(PIEZAS_ORILLA_B) / sizeof(PIEZAS_ORILLA_B[0]));


static bool CercaDePiezaB(float p, float lado, float margen)
{
    for (int k = 0; k < CANTIDAD_PIEZAS_ORILLA_B; k++)
    {
        if (PIEZAS_ORILLA_B[k].lado == lado && std::fabs(PIEZAS_ORILLA_B[k].p - p) < margen)
        {
            return true;
        }
    }

    return std::fabs(p - LONGITUD_META) < 4.0f || std::fabs(p) < 4.0f;
}


static void BaseFuegoB(Vector3 llama, bool poste)
{
    if (poste)
    {
        CilB({ llama.x, 0.4f, llama.z }, { llama.x, llama.y - 0.25f, llama.z }, 0.09f, 0.07f, 6, COLOR_TRONCO_B);
    }

    CilB({ llama.x, llama.y - 0.32f, llama.z }, { llama.x, llama.y - 0.05f, llama.z }, 0.12f, 0.24f, 8, Color{ 92, 84, 70, 255 });
    DiscoB({ llama.x, llama.y - 0.06f, llama.z }, 0.2f, 0.2f, 8, Color{ 60, 30, 20, 255 }, Color{ 40, 26, 20, 255 });
}


static void IslaTramoB(float p0, float p1, bool decoracion, int paso)
{
    int k0 = (int)std::ceil((p0 + 100.0f) / 3.0f);
    int k1 = (int)std::ceil((p1 + 100.0f) / 3.0f);

    for (int k = k0; k < k1; k++)
    {
        float p = (float)k * 3.0f - 100.0f;
        bool cercaMeta = std::fabs(p - LONGITUD_META) < 4.0f || std::fabs(p) < 3.5f;

        if ((k % paso) == 0 && !cercaMeta)
        {
            float lado = (k % 2 == 0) ? 1.0f : -1.0f;
            HelechoB({ lado * (1.45f + 0.3f * Ruido01B(k, 211)), 0.62f, -p + (Ruido01B(k, 212) - 0.5f) }, 0.55f + 0.25f * Ruido01B(k, 213), k, COLOR_HOJA_B);

            if (k % 3 == 0)
            {
                MataHojasB({ -lado * 1.3f, 0.62f, -p - 1.2f }, 0.9f, k * 5);
            }
        }

        if (k % 4 != 0 || cercaMeta)
        {
            continue;
        }

        int tipo = (k / 4) % 5;
        int s = k * 13 + 7;

        if (tipo == 0)
        {
            ColumnaRotaB({ -1.0f, 0.6f, -p }, 1.4f + 1.2f * Ruido01B(s, 221), s, false);
            ColumnaRotaB({ 1.0f, 0.6f, -p - 0.6f }, 0.8f + 0.8f * Ruido01B(s, 222), s + 1, true);
        }
        else if (tipo == 1)
        {
            CabezaPiedraB({ 0.0f, 0.4f, -p }, 0.85f, 1.5708f, s);
            if (decoracion) HelechoB({ 1.2f, 0.6f, -p + 0.4f }, 0.7f, s, COLOR_HOJA_CLARA_B);
        }
        else if (tipo == 2)
        {
            PalmeraB({ 0.4f, 0.6f, -p }, 4.2f + 0.8f * Ruido01B(s, 223), (Ruido01B(s, 224) - 0.5f) * 1.2f, s);
            if (decoracion) ArbustoB({ -1.1f, 0.6f, -p - 0.8f }, 0.7f, s);
        }
        else if (tipo == 3)
        {
            TemploB({ 0.0f, 0.55f, -p }, 3.3f, 2, 1.5708f, s);
        }
        else
        {
            // Escalones caidos y una vasija.
            CajaGiradaB({ -0.6f, 0.75f, -p }, { 1.6f, 0.3f, 0.9f }, 12.0f, VariarB(COLOR_RUINA_B, s, 225, 0.06f));
            CajaGiradaB({ 0.7f, 0.7f, -p - 0.9f }, { 1.1f, 0.25f, 0.8f }, -20.0f, MezclaB(COLOR_RUINA_B, COLOR_MUSGO_B, 0.3f), 8.0f);
            BolaB({ 0.9f, 1.05f, -p + 0.6f }, { 0.32f, 0.38f, 0.32f }, Color{ 160, 96, 62, 255 }, 4, 8);
            CilB({ 0.9f, 1.35f, -p + 0.6f }, { 0.9f, 1.5f, -p + 0.6f }, 0.15f, 0.18f, 8, Color{ 150, 90, 58, 255 });
        }
    }

    // Braseros de la isla.
    for (int f = 3; f <= 5; f++)
    {
        float p = -FUEGOS_B[f].z;
        if (p >= p0 && p < p1)
        {
            CajaB({ FUEGOS_B[f].x, 0.85f, FUEGOS_B[f].z }, { 0.6f, 0.5f, 0.6f }, VariarB(COLOR_RUINA_B, f, 226, 0.05f));
            BaseFuegoB(FUEGOS_B[f], false);
        }
    }
}


static void SelvaTramoB(float p0, float p1, bool decoracion, int paso)
{
    const float BORDE = CENTRO_CARRIL + MEDIO_ANCHO_CARRIL;

    for (int ladoI = 0; ladoI < 2; ladoI++)
    {
        float lado = ladoI == 0 ? -1.0f : 1.0f;
        int base = ladoI * 5000;

        // Muro de copas sobre el acantilado (tambien en BAJA: fondo simple).
        float pasoCopa = decoracion ? 4.5f : 7.0f;
        int c0 = (int)std::ceil((p0 + 100.0f) / pasoCopa);
        int c1 = (int)std::ceil((p1 + 100.0f) / pasoCopa);

        for (int k = c0; k < c1; k++)
        {
            float p = (float)k * pasoCopa - 100.0f + (Ruido01B(k + base, 301) - 0.5f) * 2.0f;
            float d = 20.0f + 7.0f * Ruido01B(k + base, 302);
            float r = 3.8f + 2.4f * Ruido01B(k + base, 303);
            Color hoja = MezclaB(COLOR_HOJA_OSCURA_B, COLOR_HOJA_B, 0.2f + 0.5f * Ruido01B(k + base, 304));
            BolaB({ lado * (BORDE + d), AlturaExteriorB(d) + r * 0.55f, -p }, { r, r * 0.75f, r * 1.1f }, hoja, 3, 8, 0.15f, k + base);
        }

        if (!decoracion)
        {
            continue;
        }

        // Orilla: juncos, helechos, hojas grandes y arbustos.
        float pasoOrilla = 1.7f * (float)paso;
        int o0 = (int)std::ceil((p0 + 100.0f) / pasoOrilla);
        int o1 = (int)std::ceil((p1 + 100.0f) / pasoOrilla);

        for (int k = o0; k < o1; k++)
        {
            float p = (float)k * pasoOrilla - 100.0f;
            int s = k + base;
            float j = Ruido01B(s, 311);

            if (CercaDePiezaB(p, lado, 2.5f))
            {
                continue;
            }

            float d = 0.7f + 0.5f * j;
            if (k % 2 == 0) JuncosB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.05f, -p }, s);

            d = 1.8f + 1.4f * Ruido01B(s, 312);
            if (k % 3 == 0) MataHojasB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.05f, -p - 0.5f }, 1.1f + 0.5f * j, s);
            else HelechoB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.05f, -p - 0.5f }, 0.8f + 0.4f * j, s, k % 2 == 0 ? COLOR_HOJA_B : COLOR_HOJA_CLARA_B);

            d = 3.4f + 1.6f * Ruido01B(s, 313);
            if (k % 2 == 1) ArbustoB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.1f, -p }, 1.0f + 0.6f * j, s);

            // Piedras de la orilla (del lado de afuera del agua).
            if (k % 4 == 2)
            {
                float r = 0.35f + 0.25f * j;
                RocaB({ lado * (BORDE + 0.45f + r), 0.35f, -p - 0.8f }, { r, r * 0.8f, r * 1.1f }, s);
            }
        }

        // Arboles gigantes y palmeras.
        float pasoArbol = 6.5f * (paso == 1 ? 1.0f : 1.4f);
        int a0 = (int)std::ceil((p0 + 100.0f + (float)ladoI * 3.0f) / pasoArbol);
        int a1 = (int)std::ceil((p1 + 100.0f + (float)ladoI * 3.0f) / pasoArbol);

        for (int k = a0; k < a1; k++)
        {
            float p = (float)k * pasoArbol - 100.0f - (float)ladoI * 3.0f;
            int s = k * 7 + base;

            if (CercaDePiezaB(p, lado, 4.5f))
            {
                continue;
            }

            float d = 6.0f + 5.0f * Ruido01B(s, 321);
            ArbolGiganteB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.1f, -p }, 12.5f + 5.0f * Ruido01B(s, 322), 0.85f + 0.4f * Ruido01B(s, 323), s, paso == 1 || k % 2 == 0);

            if (k % 2 == 0)
            {
                float dp = 3.6f + 1.2f * Ruido01B(s, 324);
                PalmeraB({ lado * (BORDE + dp), AlturaExteriorB(dp) - 0.1f, -p - 3.0f }, 5.0f + 1.5f * Ruido01B(s, 325), -lado * 0.9f, s);
            }

            // Arboles sobre el acantilado.
            float da = 21.0f + 4.0f * Ruido01B(s, 326);
            ArbolGiganteB({ lado * (BORDE + da), AlturaExteriorB(da) - 0.2f, -p - 2.0f }, 8.0f + 4.0f * Ruido01B(s, 327), 0.8f, s + 3, false);
        }

        // Piezas grandes de la orilla.
        for (int k = 0; k < CANTIDAD_PIEZAS_ORILLA_B; k++)
        {
            const PiezaOrillaB& pieza = PIEZAS_ORILLA_B[k];

            if (pieza.lado != lado || pieza.p < p0 || pieza.p >= p1)
            {
                continue;
            }

            int s = 800 + k * 17;
            float giro = lado < 0.0f ? 0.0f : 3.14159f;   // de frente al rio

            if (pieza.tipo == 0)
            {
                float d = 7.0f;
                TemploB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.3f, -pieza.p }, 5.2f, 3, giro, s);
                ArbolGiganteB({ lado * (BORDE + d + 4.5f), AlturaExteriorB(d + 4.5f), -pieza.p - 3.5f }, 14.0f, 1.0f, s, true);
            }
            else if (pieza.tipo == 1)
            {
                float d = 2.9f;
                CabezaPiedraB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.5f, -pieza.p }, 1.25f, giro + lado * 0.35f, s);
                HelechoB({ lado * (BORDE + d - 1.4f), AlturaExteriorB(d - 1.4f), -pieza.p + 1.1f }, 0.9f, s, COLOR_HOJA_B);
            }
            else if (pieza.tipo == 2)
            {
                for (int q = 0; q < 3; q++)
                {
                    float d = 2.2f + 1.6f * (float)q;
                    ColumnaRotaB({ lado * (BORDE + d), AlturaExteriorB(d) - 0.1f, -pieza.p + (float)q * 1.7f - 1.7f }, 1.2f + 1.6f * Ruido01B(s + q, 331), s + q, q == 1);
                }
                LianaB({ lado * (BORDE + 3.8f), 4.4f, -pieza.p }, 2.0f, s);
            }
            else
            {
                PenonCascadaB(lado, pieza.p, s, true);
            }
        }
    }

    // Peñones con cascada tambien en BAJA (son parte del fondo).
    if (!decoracion)
    {
        for (int k = 0; k < CANTIDAD_PIEZAS_ORILLA_B; k++)
        {
            const PiezaOrillaB& pieza = PIEZAS_ORILLA_B[k];
            if (pieza.tipo == 3 && pieza.p >= p0 && pieza.p < p1) PenonCascadaB(pieza.lado, pieza.p, 800 + k * 17, false);
        }
    }
}


// Muelles de salida con banderines que cruzan cada carril.
static void SalidaTramoB(float p0, float p1, bool decoracion)
{
    if (0.0f < p0 || 0.0f >= p1)
    {
        return;
    }

    const float BORDE = CENTRO_CARRIL + MEDIO_ANCHO_CARRIL;

    for (int ladoI = 0; ladoI < 2; ladoI++)
    {
        float lado = ladoI == 0 ? -1.0f : 1.0f;

        // Muelle de tablones sobre la orilla.
        for (int t = 0; t < 7; t++)
        {
            float z = -2.4f + (float)t * 0.8f;
            CajaB({ lado * (BORDE + 1.5f), 0.7f, z }, { 3.0f, 0.14f, 0.74f }, VariarB(COLOR_MADERA_B, t + ladoI * 9, 401, 0.1f));
        }

        for (int q = 0; q < 4; q++)
        {
            float x = lado * (BORDE + 0.1f + 2.8f * (float)(q % 2));
            float z = q < 2 ? -2.6f : 2.6f;
            CilB({ x, -0.4f, z }, { x, 1.0f, z }, 0.12f, 0.12f, 6, TonoB(COLOR_MADERA_B, 0.7f));
        }

        if (decoracion)
        {
            CajaGiradaB({ lado * (BORDE + 2.3f), 1.05f, -1.2f }, { 0.6f, 0.55f, 0.6f }, 15.0f, VariarB(COLOR_MADERA_B, ladoI, 402, 0.08f));
            CajaGiradaB({ lado * (BORDE + 2.5f), 1.05f, -0.4f }, { 0.5f, 0.5f, 0.5f }, -10.0f, TonoB(COLOR_MADERA_B, 0.85f));
            CilB({ lado * (BORDE + 1.4f), 0.78f, 1.4f }, { lado * (BORDE + 1.4f), 0.9f, 1.4f }, 0.35f, 0.35f, 10, COLOR_CUERDA_B);
        }

        // Postes y soga con banderines.
        float xa = lado * (BORDE + 0.6f);
        float xb = lado * (MEDIO_ANCHO_ISLA - 0.4f);
        float z = -0.5f;
        CilB({ xa, 0.4f, z }, { xa, 4.6f, z }, 0.13f, 0.11f, 6, COLOR_TRONCO_B);
        CilB({ xb, 0.6f, z }, { xb, 4.6f, z }, 0.13f, 0.11f, 6, COLOR_TRONCO_B);
        BaseFuegoB(FUEGOS_B[6 + ladoI], true);

        static const Color BANDERIN[4] = { { 222, 164, 62, 255 }, { 62, 150, 140, 255 }, { 240, 228, 192, 255 }, { 190, 96, 62, 255 } };
        const int n = 14;
        Vector3 ant = { xa, 4.4f, z };

        for (int k = 1; k <= n; k++)
        {
            float u = (float)k / (float)n;
            Vector3 q = { xa + (xb - xa) * u, 4.4f - 0.9f * 4.0f * u * (1.0f - u), z };
            VigaB(ant, q, 0.04f, COLOR_CUERDA_B);

            if (k < n)
            {
                Color c = BANDERIN[(k + ladoI) % 4];
                g_modoB = 3.0f;
                g_datoB = 0.0f;
                VB({ q.x - 0.22f, q.y, q.z }, c);
                VB({ q.x + 0.22f, q.y, q.z }, c);
                g_datoB = 0.8f;
                VB({ q.x, q.y - 0.55f, q.z }, TonoB(c, 0.85f));
                g_modoB = 0.0f;
                g_datoB = 0.0f;
            }

            ant = q;
        }
    }
}


// Portico del templo en la meta: tres pilares, dintel tallado con el sol
// dorado (acento), estandartes que se mecen y antorchas.
static void MetaTramoB(float p0, float p1)
{
    if (LONGITUD_META < p0 || LONGITUD_META >= p1)
    {
        return;
    }

    const float z = -LONGITUD_META;
    const float X_PILAR[3] = { -14.4f, 0.0f, 14.4f };
    Color piedra = COLOR_RUINA_B;

    for (int k = 0; k < 3; k++)
    {
        float x = X_PILAR[k];
        CajaB({ x, 0.45f, z }, { 2.0f, 0.9f, 2.0f }, TonoB(piedra, 0.9f));
        CajaB({ x, 3.4f, z }, { 1.6f, 5.2f, 1.6f }, VariarB(piedra, k, 501, 0.05f));
        for (int b = 0; b < 3; b++)
        {
            CajaB({ x, 1.6f + (float)b * 1.6f, z }, { 1.7f, 0.18f, 1.7f }, TonoB(piedra, 0.78f));
        }
        // Mascaron tallado.
        CajaB({ x, 5.3f, z + 0.82f }, { 1.0f, 0.7f, 0.12f }, TonoB(piedra, 0.85f));
        CajaB({ x - 0.25f, 5.4f, z + 0.9f }, { 0.2f, 0.14f, 0.06f }, COLOR_PIEDRA_MOJADA_B);
        CajaB({ x + 0.25f, 5.4f, z + 0.9f }, { 0.2f, 0.14f, 0.06f }, COLOR_PIEDRA_MOJADA_B);
        CajaB({ x, 5.1f, z + 0.9f }, { 0.5f, 0.1f, 0.06f }, COLOR_PIEDRA_MOJADA_B);
        BaseFuegoB(FUEGOS_B[k], false);
        CajaB({ FUEGOS_B[k].x, FUEGOS_B[k].y - 0.42f, FUEGOS_B[k].z - 0.05f }, { 0.36f, 0.12f, 0.3f }, TonoB(piedra, 0.7f));
    }

    // Dintel con grecas y cresteria escalonada.
    CajaB({ 0.0f, 6.65f, z }, { 31.0f, 1.3f, 1.5f }, VariarB(piedra, 9, 502, 0.04f));
    CajaB({ 0.0f, 6.65f, z + 0.76f }, { 30.6f, 0.4f, 0.06f }, TonoB(piedra, 0.72f));

    for (int g = 0; g < 30; g++)
    {
        float x = -14.5f + (float)g;
        CajaB({ x, 6.65f + ((g % 2 == 0) ? 0.08f : -0.08f), z + 0.8f }, { 0.5f, 0.2f, 0.05f }, TonoB(piedra, 1.08f));
    }

    for (int c = -6; c <= 6; c++)
    {
        float alto = 0.5f + 0.25f * (float)(6 - std::abs(c)) * 0.3f;
        CajaB({ (float)c * 2.3f, 7.3f + alto * 0.5f, z }, { 1.2f, alto, 1.1f }, MezclaB(piedra, COLOR_MUSGO_B, 0.15f + 0.05f * (float)(std::abs(c) % 3)));
    }

    // Sol dorado al centro (acento) y discos menores sobre cada carril.
    for (int k = -1; k <= 1; k++)
    {
        float x = (float)k * CENTRO_CARRIL;
        float r = k == 0 ? 1.15f : 0.75f;
        CilB({ x, 6.7f, z + 0.76f }, { x, 6.7f, z + 0.95f }, r, r, 14, COLOR_ORO_B);
        CilB({ x, 6.7f, z + 0.95f }, { x, 6.7f, z + 1.0f }, r * 0.55f, r * 0.55f, 14, TonoB(COLOR_ORO_B, 1.15f));
        for (int ry = 0; ry < 8; ry++)
        {
            float a = (float)ry * 0.785f;
            CajaEjesB({ x + std::cos(a) * r * 1.25f, 6.7f + std::sin(a) * r * 1.25f, z + 0.82f }, { std::cos(a) * 0.22f * r, std::sin(a) * 0.22f * r, 0.0f }, { -std::sin(a) * 0.08f, std::cos(a) * 0.08f, 0.0f }, { 0.0f, 0.0f, 0.05f }, TonoB(COLOR_ORO_B, 0.9f));
        }
    }

    // Estandartes que cuelgan del dintel sobre cada carril.
    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);
        for (int q = -1; q <= 1; q += 2)
        {
            float x = c + (float)q * 3.1f;
            float z0 = z + 0.8f;
            Color tela = { 236, 222, 182, 255 };
            g_modoB = 3.0f;
            for (int t = 0; t < 4; t++)
            {
                float u0 = (float)t / 4.0f, u1 = (float)(t + 1) / 4.0f;
                float y0 = 6.0f - 2.4f * u0, y1 = 6.0f - 2.4f * u1;
                Color c0 = t == 2 ? COLOR_ORO_B : tela;
                g_datoB = u0 * 0.7f;
                VB({ x - 0.55f, y0, z0 }, c0); VB({ x + 0.55f, y0, z0 }, c0);
                g_datoB = u1 * 0.7f;
                VB({ x + 0.55f, y1, z0 }, c0);
                g_datoB = u0 * 0.7f;
                VB({ x - 0.55f, y0, z0 }, c0);
                g_datoB = u1 * 0.7f;
                VB({ x + 0.55f, y1, z0 }, c0); VB({ x - 0.55f, y1, z0 }, c0);
            }
            g_datoB = 0.7f;
            VB({ x - 0.55f, 3.6f, z0 }, tela); VB({ x + 0.55f, 3.6f, z0 }, tela);
            g_datoB = 0.8f;
            VB({ x, 3.1f, z0 }, tela);
            g_modoB = 0.0f;
            g_datoB = 0.0f;
        }
    }

    // Plataforma de la isla al pie del pilar central.
    CajaB({ 0.0f, 0.75f, z + 1.8f }, { 3.6f, 0.3f, 1.6f }, TonoB(piedra, 0.95f));
}


static void EscenaTramoB(const MinijuegoBalsasRapido& m, float p0, float p1, bool decoracion, int paso)
{
    AguaTramoB(m, p0, p1);
    OrillasTramoB(p0, p1, decoracion);
    RocasTramoB(m, p0, p1, decoracion);
    DivisorTramoB(m, p0, p1, decoracion);
    IslaTramoB(p0, p1, decoracion, paso);
    SelvaTramoB(p0, p1, decoracion, paso);
    SalidaTramoB(p0, p1, decoracion);
    MetaTramoB(p0, p1);
}


// Horizonte (sin bruma del shader: el color ya viene neblinoso): cierre de
// selva al fondo del rio, piramide entre la bruma, cascadas lejanas y dos
// cordones de montañas.
static void SiluetaB(float z, float x0, float x1, float yBase, float altoMax, int puntos, int semilla, Color abajo, Color arriba)
{
    float prevX = x0;
    float prevY = yBase + altoMax * (0.4f + 0.6f * Ruido01B(0, semilla));

    for (int i = 1; i <= puntos; i++)
    {
        float x = x0 + (x1 - x0) * (float)i / (float)puntos;
        float y = yBase + altoMax * (0.35f + 0.65f * Ruido01B(i, semilla));
        QuadColoresB({ prevX, yBase - 30.0f, z }, { x, yBase - 30.0f, z }, { x, y, z }, { prevX, prevY, z }, abajo, abajo, arriba, arriba);
        prevX = x;
        prevY = y;
    }
}


static void EscenaLejanaB(bool decoracion)
{
    g_modoB = 2.0f;

    Color bruma = COLOR_NIEBLA_B;
    Color lejana = MezclaB(COLOR_CIELO_BAJO_B, Color{ 150, 176, 170, 255 }, 0.5f);
    Color media = MezclaB(COLOR_NIEBLA_B, Color{ 70, 110, 80, 255 }, 0.35f);

    // Montañas en dos profundidades.
    SiluetaB(-720.0f, -1200.0f, 1200.0f, -10.0f, 150.0f, decoracion ? 40 : 20, 601, MezclaB(lejana, bruma, 0.5f), lejana);
    SiluetaB(-560.0f, -900.0f, 900.0f, -10.0f, 90.0f, decoracion ? 36 : 18, 602, MezclaB(media, bruma, 0.6f), media);

    if (decoracion)
    {
        // Cascadas lejanas que bajan por la montaña media.
        for (int k = 0; k < 3; k++)
        {
            float x = -260.0f + (float)k * 230.0f + 40.0f * Ruido01B(k, 603);
            float alto = 40.0f + 25.0f * Ruido01B(k, 604);
            QuadColoresB({ x - 3.0f, 0.0f, -555.0f }, { x + 3.0f, 0.0f, -555.0f }, { x + 2.0f, alto, -555.0f }, { x - 2.0f, alto, -555.0f },
                MezclaB(bruma, WHITE, 0.6f), MezclaB(bruma, WHITE, 0.6f), MezclaB(media, WHITE, 0.5f), MezclaB(media, WHITE, 0.5f));
        }

        // Piramide escalonada entre la bruma.
        Color piramide = MezclaB(COLOR_NIEBLA_B, Color{ 120, 120, 100, 255 }, 0.3f);
        for (int n = 0; n < 6; n++)
        {
            float w = 46.0f - (float)n * 7.0f;
            float y0 = (float)n * 7.0f;
            Color c = TonoB(piramide, 0.97f + 0.01f * (float)n);
            QuadColoresB({ -60.0f - w, y0, -430.0f }, { -60.0f + w, y0, -430.0f }, { -60.0f + w, y0 + 7.0f, -430.0f }, { -60.0f - w, y0 + 7.0f, -430.0f }, c, c, c, c);
        }
        QuadColoresB({ -66.0f, 42.0f, -430.0f }, { -54.0f, 42.0f, -430.0f }, { -54.0f, 52.0f, -430.0f }, { -66.0f, 52.0f, -430.0f }, piramide, piramide, piramide, piramide);
    }

    g_modoB = 0.0f;

    // Cierre de selva donde el rio se pierde entre la bruma.
    for (int k = 0; k < 16; k++)
    {
        float x = -60.0f + (float)k * 8.0f;
        float r = 7.0f + 4.0f * Ruido01B(k, 605);
        BolaB({ x, r * 0.5f, -258.0f - 8.0f * Ruido01B(k, 606) }, { r, r * 0.9f, r }, MezclaB(COLOR_HOJA_OSCURA_B, COLOR_HOJA_B, Ruido01B(k, 607)), 3, 8, 0.15f, k + 600);
    }
}


// Fondo del cielo: panel lejano con degrade de la bruma al celeste.
static void CieloEstaticoB(bool decoracion)
{
    (void)decoracion;
    g_modoB = 2.0f;
    const float z = -760.0f;
    const float x = 1400.0f;
    float ys[4] = { -80.0f, 18.0f, 90.0f, 520.0f };
    Color cs[4] = { COLOR_NIEBLA_B, COLOR_CIELO_BAJO_B, MezclaB(COLOR_CIELO_BAJO_B, COLOR_CIELO_ALTO_B, 0.6f), COLOR_CIELO_ALTO_B };

    for (int k = 0; k < 3; k++)
    {
        QuadColoresB({ -x, ys[k], z }, { x, ys[k], z }, { x, ys[k + 1], z }, { -x, ys[k + 1], z }, cs[k], cs[k], cs[k + 1], cs[k + 1]);
    }

    // Laterales (por si la camara de la cuenta mira mas abierto).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float xl = (float)lado * 700.0f;
        for (int k = 0; k < 3; k++)
        {
            QuadColoresB({ xl, ys[k], 200.0f }, { xl, ys[k], z }, { xl, ys[k + 1], z }, { xl, ys[k + 1], 200.0f }, cs[k], cs[k], cs[k + 1], cs[k + 1]);
        }
    }

    g_modoB = 0.0f;
}


//==================================================
// DIBUJO
//==================================================

//--------------------------------------------------
// Lo que se mueve: balsas, troncos, bananas, remolinos, fuegos y particulas
//--------------------------------------------------

// Marco de una balsa: base sobre la cubierta (y = donde pisan los remeros),
// rumbo de la logica y cabeceo/rolido solo visuales y leves.
struct MarcoBalsaB
{
    Vector3 base;
    float rumbo;
    float cabeceo;
    float rolido;
};


static Vector3 EnBalsaB(const MarcoBalsaB& f, float x, float y, float z)
{
    return LocalB(f.base, f.rumbo, { x, y - f.cabeceo * z + f.rolido * x, z });
}


// Caja alineada con la balsa (centro y medios tamanos en coordenadas locales).
static void CajaBalsaB(const MarcoBalsaB& f, float x, float y, float z, float hx, float hy, float hz, Color color)
{
    Vector3 c = EnBalsaB(f, x, y, z);
    Vector3 ex = Vector3Subtract(EnBalsaB(f, x + hx, y, z), c);
    Vector3 ey = Vector3Subtract(EnBalsaB(f, x, y + hy, z), c);
    Vector3 ez = Vector3Subtract(EnBalsaB(f, x, y, z + hz), c);
    CajaEjesB(c, ex, ey, ez, color, true);
}


// Elipse plana sobre el agua girada con el rumbo (sombra, estela).
static void ElipseAguaB(Vector3 c, float rumbo, float rx, float rz, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = LocalB(c, rumbo, { std::cos(a0) * rx, 0.0f, std::sin(a0) * rz });
        Vector3 p1 = LocalB(c, rumbo, { std::cos(a1) * rx, 0.0f, std::sin(a1) * rz });
        VB(c, centro); VB(p1, borde); VB(p0, borde);
    }
}


// Cinta plana sobre el agua entre dos puntos (espuma de estelas y remolinos).
static void CintaAguaB(Vector3 a, Vector3 b, float anchoA, float anchoB, Color ca, Color cb)
{
    float dx = b.x - a.x, dz = b.z - a.z;
    float l = std::sqrt(dx * dx + dz * dz);

    if (l < 0.0001f)
    {
        return;
    }

    float nx = -dz / l, nz = dx / l;
    float ha = anchoA * 0.5f, hb = anchoB * 0.5f;
    QuadColoresB(
        { a.x - nx * ha, a.y, a.z - nz * ha }, { a.x + nx * ha, a.y, a.z + nz * ha },
        { b.x + nx * hb, b.y, b.z + nz * hb }, { b.x - nx * hb, b.y, b.z - nz * hb },
        ca, ca, cb, cb);
}


static float SuaveB(float u)
{
    u = Lim01B(u);
    return u * u * (3.0f - 2.0f * u);
}


// Pala del remo segun el tiempo desde la ultima palada: entra adelante,
// barre hacia atras bajo el agua y vuelve por el aire.
static void PoseRemoB(float tp, float& z, float& y)
{
    if (tp < 0.3f)
    {
        z = -0.55f + 1.35f * SuaveB(tp / 0.3f);
        y = -0.1f - 0.34f * Lim01B(tp / 0.06f);
    }
    else if (tp < 0.62f)
    {
        float u = (tp - 0.3f) / 0.32f;
        float s = SuaveB(u);
        z = 0.8f - 1.25f * s;
        y = -0.44f + 0.34f * s + 0.26f * std::sin(u * PI);
    }
    else
    {
        z = -0.45f;
        y = -0.1f;
    }
}


static MarcoBalsaB MarcoDeBalsaB(const MinijuegoBalsasRapido& m, int e)
{
    const BalsaRio& b = m.balsas[e];
    const VisualBalsaRio& v = m.visualBalsas[e];
    float mov = ObtenerFactorMovimientoCamaraMinijuegos();

    // Mismo flote que los remeros (la logica los sube y baja asi).
    MarcoBalsaB f;
    f.base = { b.x, 0.3f + 0.04f * std::sin(m.tiempoAnimacion * 3.0f + (float)e), -b.p };
    f.rumbo = b.rumbo;
    f.cabeceo = (0.025f * std::sin(m.tiempoVisual * 2.1f + (float)e) + v.sacudida * 0.14f * std::sin(v.sacudida * 14.0f)) * mov;
    f.rolido = (Acotar(-b.omega * 0.04f, -0.07f, 0.07f) + 0.015f * std::sin(m.tiempoVisual * 1.7f + 2.0f * (float)e)) * mov;
    return f;
}


// MODELO FUTURO: balsa de troncos con remos y banderin.
static void DibujarBalsaB(const MinijuegoBalsasRapido& m, int e, bool detalle)
{
    const BalsaRio& b = m.balsas[e];
    MarcoBalsaB f = MarcoDeBalsaB(m, e);
    Color equipo = ColorEquipoBalsas(e);
    int lados = detalle ? 7 : 5;

    // Cinco troncos atados; los del medio asoman mas en la proa.
    for (int k = 0; k < 5; k++)
    {
        float lx = -0.68f + 0.34f * (float)k;
        float extra = k == 2 ? 0.2f : (k == 1 || k == 3 ? 0.1f : 0.0f);
        float z0 = -1.3f - extra;
        float z1 = 1.22f + 0.08f * Ruido01B(k, 40 + e);
        Color madera = VariarB(COLOR_MADERA_B, k + e * 5, 91, 0.1f);
        CilB(EnBalsaB(f, lx, -0.17f, z0), EnBalsaB(f, lx, -0.17f, z1), 0.17f, 0.17f, lados, madera, true);

        if (detalle)
        {
            Color corte = MezclaB(madera, Color{ 236, 206, 150, 255 }, 0.55f);
            CilB(EnBalsaB(f, lx, -0.17f, z0 - 0.012f), EnBalsaB(f, lx, -0.17f, z0 + 0.004f), 0.13f, 0.13f, lados, corte, true);
        }
    }

    // Travesanos: el de proa pintado del color del equipo; sogas en cada cruce.
    const float TRAVESANOS[2] = { -0.92f, 0.95f };

    for (int q = 0; q < 2; q++)
    {
        float z = TRAVESANOS[q];
        Color color = q == 0 ? equipo : TonoB(COLOR_TRONCO_B, 0.9f);
        VigaB(EnBalsaB(f, -0.9f, 0.03f, z), EnBalsaB(f, 0.9f, 0.03f, z), 0.13f, color);

        if (detalle)
        {
            for (int k = 0; k < 5; k++)
            {
                float lx = -0.68f + 0.34f * (float)k;
                CilB(EnBalsaB(f, lx, -0.15f, z - 0.06f), EnBalsaB(f, lx, -0.15f, z + 0.06f), 0.195f, 0.195f, 5, COLOR_CUERDA_B, false);
            }
        }
    }

    // Bulto de carga atado en la popa con lona del equipo.
    if (detalle)
    {
        CajaBalsaB(f, -0.42f, 0.16f, 1.02f, 0.2f, 0.14f, 0.16f, TonoB(equipo, 0.8f));
        CajaBalsaB(f, -0.42f, 0.31f, 1.02f, 0.21f, 0.02f, 0.03f, COLOR_CUERDA_B);
    }

    // Mastil con banderin del equipo que flamea hacia atras.
    float tv = m.tiempoVisual;
    float mov = ObtenerFactorMovimientoCamaraMinijuegos();
    Vector3 pie = EnBalsaB(f, 0.42f, 0.0f, 1.05f);
    Vector3 punta = EnBalsaB(f, 0.42f, 2.75f, 1.05f);
    CilB(pie, punta, 0.05f, 0.04f, 5, TonoB(COLOR_TRONCO_B, 1.1f), true);
    BolaB(punta, { 0.09f, 0.09f, 0.09f }, COLOR_ORO_B, 2, 5);

    int tramos = detalle ? 6 : 3;
    Color tela = MezclaB(equipo, WHITE, 0.08f);
    Vector3 arribaAnt = EnBalsaB(f, 0.42f, 2.68f, 1.08f);
    Vector3 abajoAnt = EnBalsaB(f, 0.42f, 1.96f, 1.08f);

    for (int i = 1; i <= tramos; i++)
    {
        float u = (float)i / (float)tramos;
        float ola = std::sin(tv * 7.0f - u * 4.0f + (float)e) * 0.14f * u * mov;
        float medio = 2.32f - 0.1f * u;
        float mitad = 0.36f * (1.0f - u * 0.92f);
        Vector3 arriba = EnBalsaB(f, 0.42f + ola, medio + mitad, 1.08f + 1.15f * u);
        Vector3 abajo = EnBalsaB(f, 0.42f + ola, medio - mitad, 1.08f + 1.15f * u);
        Color c = (i % 2 == 0) ? TonoB(tela, 0.86f) : tela;
        QuadB(arribaAnt, arriba, abajo, abajoAnt, c);
        arribaAnt = arriba;
        abajoAnt = abajo;
    }

    // Remos: uno por lado; con un solo remero, doble pala desde el centro.
    float hombro = b.cantidad <= 1 ? 0.24f : 0.62f;

    for (int s = 0; s < 2; s++)
    {
        float signo = s == 0 ? -1.0f : 1.0f;
        float zb = 0.0f, yb = 0.0f;
        PoseRemoB(b.tiempoPalada[s], zb, yb);

        Vector3 mango = EnBalsaB(f, signo * hombro, 0.82f, -0.1f + 0.25f * (zb + 0.45f) / 1.25f);
        Vector3 pala = EnBalsaB(f, signo * 1.48f, yb, zb);
        Vector3 d = Vector3Normalize(Vector3Subtract(pala, mango));
        Vector3 cuello = Vector3Subtract(pala, Vector3Scale(d, 0.26f));
        VigaB(Vector3Subtract(mango, Vector3Scale(d, 0.12f)), cuello, 0.06f, Color{ 128, 90, 54, 255 });

        Vector3 ancho = Vector3CrossProduct(d, { 0.0f, 1.0f, 0.0f });
        ancho = Vector3Length(ancho) < 0.001f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3Normalize(ancho);
        Vector3 grosor = Vector3Normalize(Vector3CrossProduct(ancho, d));
        CajaEjesB(pala, Vector3Scale(ancho, 0.12f), Vector3Scale(grosor, 0.025f), Vector3Scale(d, 0.26f), Color{ 228, 204, 146, 255 }, true);
        CajaEjesB(Vector3Add(pala, Vector3Scale(d, 0.2f)), Vector3Scale(ancho, 0.125f), Vector3Scale(grosor, 0.03f), Vector3Scale(d, 0.07f), equipo, true);
    }
}


// MODELO FUTURO: tronco flotante.
static void DibujarTroncoB(const ObstaculoBalsas& o, float c, int k, float t, bool detalle)
{
    float x = c + o.desvio;
    float z = -o.p;
    float y = 0.08f + 0.05f * std::sin(t * 1.5f + (float)k);
    float giro = 0.06f * std::sin(t * 1.1f + (float)k * 1.7f);
    Vector3 a = { x - o.a, y, z - giro * o.a };
    Vector3 b = { x + o.a, y + 0.03f, z + giro * o.a };
    Color corteza = VariarB(COLOR_TRONCO_B, k, 313, 0.08f);
    int lados = detalle ? 8 : 6;

    CilB(a, b, 0.4f, 0.36f, lados, corteza, true);

    Vector3 eje = Vector3Normalize(Vector3Subtract(b, a));
    Color corte = Color{ 190, 152, 104, 255 };
    CilB(Vector3Subtract(a, Vector3Scale(eje, 0.015f)), a, 0.3f, 0.3f, lados, corte, true);
    CilB(b, Vector3Add(b, Vector3Scale(eje, 0.015f)), 0.27f, 0.27f, lados, corte, true);

    if (detalle)
    {
        Vector3 nace = Vector3Lerp(a, b, 0.32f);
        nace.y += 0.25f;
        CilB(nace, { nace.x - 0.25f, nace.y + 0.55f, nace.z - 0.2f }, 0.09f, 0.04f, 5, corteza, true);
        BolaB({ nace.x - 0.3f, nace.y + 0.62f, nace.z - 0.22f }, { 0.22f, 0.12f, 0.2f }, COLOR_HOJA_B, 2, 6);
        Vector3 musgo = Vector3Lerp(a, b, 0.62f);
        BolaB({ musgo.x, musgo.y + 0.33f, musgo.z }, { 0.55f, 0.08f, 0.26f }, COLOR_MUSGO_B, 2, 6);
    }
}


// MODELO FUTURO: racimo de bananas sobre hoja.
static void DibujarBananaB(Vector3 c, float t, int k, bool detalle)
{
    DiscoB({ c.x, 0.05f, c.z }, 0.62f, 0.55f, 9, COLOR_HOJA_CLARA_B, COLOR_HOJA_B);

    float y = 0.18f + 0.06f * std::sin(t * 3.0f + (float)k);
    float giro = t * 0.9f + (float)k;
    Color banana = Color{ 250, 214, 64, 255 };
    int lados = detalle ? 6 : 4;

    for (int q = 0; q < 3; q++)
    {
        float a = giro + (float)q * 2.094f;
        float dx = std::cos(a), dz = std::sin(a);
        Vector3 p0 = { c.x + dx * 0.04f, y, c.z + dz * 0.04f };
        Vector3 p1 = { c.x + dx * 0.2f, y + 0.04f, c.z + dz * 0.2f };
        Vector3 p2 = { c.x + dx * 0.34f, y + 0.15f, c.z + dz * 0.34f };
        Vector3 p3 = { c.x + dx * 0.4f, y + 0.3f, c.z + dz * 0.4f };
        CilB(p0, p1, 0.07f, 0.095f, lados, banana, false);
        CilB(p1, p2, 0.095f, 0.08f, lados, banana, false);
        CilB(p2, p3, 0.08f, 0.035f, lados, MezclaB(banana, Color{ 150, 170, 60, 255 }, 0.25f), true);
    }

    CilB({ c.x, y - 0.02f, c.z }, { c.x + 0.03f, y + 0.24f, c.z }, 0.05f, 0.035f, 5, Color{ 110, 84, 46, 255 }, true);
}


// Todo lo opaco que se mueve, con la misma bruma que el horneado.
static void DibujarDinamicoOpacoB(const MinijuegoBalsasRapido& m, bool detalle)
{
    float foco = m.focoCamara;
    float t = m.tiempoAnimacion;

    LoteB();

    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);

        for (int k = 0; k < m.cantidadObstaculos; k++)
        {
            const ObstaculoBalsas& o = m.obstaculos[k];

            if (o.p < foco - 22.0f || o.p > foco + 70.0f)
            {
                continue;
            }

            if (o.tipo == OBSTACULO_BALSAS_TRONCO)
            {
                DibujarTroncoB(o, c, k + e * 13, t, detalle);
            }
            else if (o.tipo == OBSTACULO_BALSAS_BANANA && !m.balsas[e].recogidas[k])
            {
                DibujarBananaB({ c + o.desvio, 0.0f, -o.p }, m.tiempoVisual, k + e * 5, detalle);
            }
        }
    }

    for (int e = 0; e < 2; e++)
    {
        if (m.balsas[e].activa)
        {
            DibujarBalsaB(m, e, detalle);
        }
    }

    FinLoteB();
}


// Velos sobre el agua: sombra, estela, anillos, remolinos y particulas.
static void DibujarDinamicoTransparenteB(const MinijuegoBalsasRapido& m, bool efectos)
{
    float foco = m.focoCamara;
    float tv = m.tiempoVisual;
    float mov = ObtenerFactorMovimientoCamaraMinijuegos();
    bool sombras = CalidadSombras() != CALIDAD_BAJA;

    rlBegin(RL_TRIANGLES);

    // Remolinos: brazos de espuma en espiral que giran.
    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);

        for (int k = 0; k < m.cantidadObstaculos; k++)
        {
            const ObstaculoBalsas& o = m.obstaculos[k];

            if (o.tipo != OBSTACULO_BALSAS_REMOLINO || o.p < foco - 22.0f || o.p > foco + 70.0f)
            {
                continue;
            }

            Vector3 centro = { c + o.desvio, 0.05f, -o.p };
            float sentido = o.desvio > 0.0f ? -1.0f : 1.0f;
            float giro = tv * 1.7f * sentido * (0.35f + 0.65f * mov);
            const int SEG = 10;

            for (int brazo = 0; brazo < 3; brazo++)
            {
                for (int j = 0; j < SEG; j++)
                {
                    float u0 = (float)j / (float)SEG, u1 = (float)(j + 1) / (float)SEG;
                    float r0 = o.a * (0.98f - 0.86f * u0), r1 = o.a * (0.98f - 0.86f * u1);
                    float a0 = giro + (float)brazo * 2.094f + sentido * u0 * 2.6f;
                    float a1 = giro + (float)brazo * 2.094f + sentido * u1 * 2.6f;
                    Vector3 p0 = { centro.x + std::cos(a0) * r0, 0.05f, centro.z + std::sin(a0) * r0 };
                    Vector3 p1 = { centro.x + std::cos(a1) * r1, 0.05f, centro.z + std::sin(a1) * r1 };
                    CintaAguaB(p0, p1, 0.3f * (1.0f - 0.7f * u0), 0.3f * (1.0f - 0.7f * u1),
                        AlfaB(COLOR_ESPUMA_B, 0.62f * (1.0f - u0)), AlfaB(COLOR_ESPUMA_B, 0.62f * (1.0f - u1)));
                }
            }

            ElipseAguaB({ centro.x, 0.045f, centro.z }, 0.0f, o.a * 0.35f, o.a * 0.35f, Color{ 8, 40, 38, 150 }, Color{ 8, 40, 38, 0 }, 12);
        }
    }

    // Halo dorado de las bananas pendientes.
    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);

        for (int k = 0; k < m.cantidadObstaculos; k++)
        {
            const ObstaculoBalsas& o = m.obstaculos[k];

            if (o.tipo != OBSTACULO_BALSAS_BANANA || m.balsas[e].recogidas[k] || o.p < foco - 22.0f || o.p > foco + 70.0f)
            {
                continue;
            }

            float pulso = 0.5f + 0.5f * std::sin(tv * 4.0f + (float)k);
            AnilloB({ c + o.desvio, 0.06f, -o.p }, 0.62f, 0.85f + 0.12f * pulso, 16, AlfaB(COLOR_ORO_B, 0.75f), AlfaB(COLOR_ORO_B, 0.0f));
        }
    }

    // Balsas: sombra, anillo del equipo, estela, salpicones de los remos.
    for (int e = 0; e < 2; e++)
    {
        const BalsaRio& b = m.balsas[e];
        const VisualBalsaRio& v = m.visualBalsas[e];

        if (!b.activa)
        {
            continue;
        }

        Color equipo = ColorEquipoBalsas(e);
        Vector3 agua = { b.x, 0.035f, -b.p };

        if (sombras)
        {
            ElipseAguaB({ agua.x, 0.03f, agua.z }, b.rumbo, 1.2f, 1.75f, Color{ 8, 34, 30, 120 }, Color{ 8, 34, 30, 0 });
        }

        // Anillo del color del equipo alrededor (se lee de lejos).
        for (int k = 0; k < 18; k++)
        {
            float a0 = 2.0f * PI * (float)k / 18.0f, a1 = 2.0f * PI * (float)(k + 1) / 18.0f;
            Vector3 i0 = LocalB(agua, b.rumbo, { std::cos(a0) * 1.25f, 0.0f, std::sin(a0) * 1.75f });
            Vector3 i1 = LocalB(agua, b.rumbo, { std::cos(a1) * 1.25f, 0.0f, std::sin(a1) * 1.75f });
            Vector3 o0 = LocalB(agua, b.rumbo, { std::cos(a0) * 1.45f, 0.0f, std::sin(a0) * 1.97f });
            Vector3 o1 = LocalB(agua, b.rumbo, { std::cos(a1) * 1.45f, 0.0f, std::sin(a1) * 1.97f });
            QuadColoresB(i0, o0, o1, i1, AlfaB(equipo, 0.55f), AlfaB(equipo, 0.0f), AlfaB(equipo, 0.0f), AlfaB(equipo, 0.55f));
        }

        // Estela en V detras de la popa.
        float L = v.estela;

        if (L > 0.04f)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                Vector3 cerca = LocalB(agua, b.rumbo, { (float)lado * 0.8f, 0.0f, 1.25f });
                Vector3 lejos = LocalB(agua, b.rumbo, { (float)lado * (0.85f + 1.7f * L), 0.0f, 1.3f + 4.8f * L });
                CintaAguaB(cerca, lejos, 0.32f, 0.9f, AlfaB(COLOR_ESPUMA_B, 0.7f * L), AlfaB(COLOR_ESPUMA_B, 0.0f));
            }

            ElipseAguaB(LocalB(agua, b.rumbo, { 0.0f, 0.005f, 1.75f }), b.rumbo, 0.65f, 0.95f, AlfaB(COLOR_ESPUMA_B, 0.5f * L), AlfaB(COLOR_ESPUMA_B, 0.0f), 10);
        }

        // Salpicon al entrar cada pala.
        for (int s = 0; s < 2; s++)
        {
            float tp = b.tiempoPalada[s];

            if (tp > 0.45f)
            {
                continue;
            }

            float signo = s == 0 ? -1.0f : 1.0f;
            float u = tp / 0.45f;
            Vector3 entra = LocalB(agua, b.rumbo, { signo * 1.48f, 0.01f, -0.4f + 0.9f * SuaveB(tp / 0.3f) });
            AnilloB(entra, 0.12f + 0.7f * u, 0.3f + 0.85f * u, 12, AlfaB(COLOR_ESPUMA_B, 0.85f * (1.0f - u)), AlfaB(COLOR_ESPUMA_B, 0.0f));
        }

        // Sincronia: onda que se abre; turbo: anillo dorado que late.
        if (b.tiempoSync > 0.0f)
        {
            float u = 1.0f - b.tiempoSync / 0.7f;
            float r = 1.4f + u * 2.6f;
            AnilloB({ agua.x, 0.05f, agua.z }, r, r + 0.35f, 22, AlfaB(MezclaB(equipo, WHITE, 0.5f), 0.9f * (1.0f - u)), AlfaB(WHITE, 0.0f));
        }

        if (b.tiempoBoost > 0.0f)
        {
            float pulso = 0.5f + 0.5f * std::sin(tv * 12.0f);
            float r = 1.7f + 0.15f * pulso;
            AnilloB({ agua.x, 0.055f, agua.z }, r, r + 0.3f, 22, AlfaB(COLOR_ORO_B, 0.85f), AlfaB(COLOR_ORO_B, 0.0f));
        }
    }

    // Particulas de la logica (salpicaduras de cada palada y choque).
    for (int i = 0; i < MAX_PARTICULAS_BALSAS; i++)
    {
        const ParticulaTierra& p = m.particulas[i];

        if (!p.activa || p.vidaMaxima <= 0.0f)
        {
            continue;
        }

        PuntoB(p.posicion, p.tamano, AlfaB(p.color, Lim01B(p.vida / p.vidaMaxima) * 0.95f));
    }

    // Particulas visuales: espuma, confeti, hojas y chispas.
    for (int i = 0; i < MAX_PARTICULAS_VISUAL_BALSAS; i++)
    {
        const ParticulaVisualBalsas& p = m.particulasVisual[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = Lim01B(p.vida / p.vidaMaxima);

        if (p.tipo == 1 || p.tipo == 2)
        {
            float largo = p.tipo == 2 ? 1.5f : 1.0f;
            float cg = std::cos(p.giro), sg = std::sin(p.giro);
            float aplaste = 0.3f + 0.7f * std::fabs(std::sin(p.giro * 1.7f));
            Vector3 ax = Vector3Add(Vector3Scale(g_derB, cg * p.tamano * largo), Vector3Scale(g_arribaB, sg * p.tamano * largo));
            Vector3 ay = Vector3Add(Vector3Scale(g_derB, -sg * p.tamano * aplaste), Vector3Scale(g_arribaB, cg * p.tamano * aplaste));
            Color c = AlfaB(p.color, Lim01B(p.vida / (p.vidaMaxima * 0.3f)));
            Vector3 q = p.posicion;
            QuadColoresB(
                Vector3Subtract(Vector3Subtract(q, ax), ay), Vector3Subtract(Vector3Add(q, ax), ay),
                Vector3Add(Vector3Add(q, ax), ay), Vector3Add(Vector3Subtract(q, ax), ay), c, c, c, c);
        }
        else if (p.tipo == 0)
        {
            PuntoB(p.posicion, p.tamano * (1.0f + 0.6f * (1.0f - vida)), AlfaB(p.color, 0.9f * vida));
        }
        else
        {
            PuntoB(p.posicion, p.tamano, AlfaB(p.color, vida));
        }
    }

    // Llamas de antorchas y braseros (la base esta horneada).
    for (int f = 0; f < CANTIDAD_FUEGOS_B; f++)
    {
        Vector3 l = FUEGOS_B[f];

        if (-l.z < foco - 25.0f || -l.z > foco + 80.0f)
        {
            continue;
        }

        float alto = (0.5f + 0.08f * std::sin(tv * 11.0f + (float)f * 1.3f) + 0.05f * std::sin(tv * 17.0f + (float)f)) * (f < 3 ? 1.3f : 1.0f);
        float vaiven = 0.06f * std::sin(tv * 6.0f + (float)f) * mov;
        Vector3 der = Vector3Normalize({ g_derB.x, 0.0f, g_derB.z });
        Vector3 izq0 = Vector3Subtract(l, Vector3Scale(der, 0.2f));
        Vector3 der0 = Vector3Add(l, Vector3Scale(der, 0.2f));
        Vector3 tope = { l.x + der.x * vaiven, l.y + alto, l.z + der.z * vaiven };
        VB(izq0, Color{ 255, 120, 30, 235 }); VB(der0, Color{ 255, 120, 30, 235 }); VB(tope, Color{ 255, 70, 20, 0 });

        Vector3 izq1 = Vector3Subtract(l, Vector3Scale(der, 0.1f));
        Vector3 der1 = Vector3Add(l, Vector3Scale(der, 0.1f));
        Vector3 tope1 = { l.x + der.x * vaiven * 0.6f, l.y + alto * 0.62f, l.z + der.z * vaiven * 0.6f };
        izq1.y += 0.02f; der1.y += 0.02f;
        VB(izq1, Color{ 255, 236, 140, 255 }); VB(der1, Color{ 255, 236, 140, 255 }); VB(tope1, Color{ 255, 200, 80, 0 });
    }

    rlEnd();
    rlDrawRenderBatchActive();

    // Brillos aditivos (solo con efectos).
    if (efectos)
    {
        BeginBlendMode(BLEND_ADDITIVE);
        rlBegin(RL_TRIANGLES);

        for (int f = 0; f < CANTIDAD_FUEGOS_B; f++)
        {
            Vector3 l = FUEGOS_B[f];

            if (-l.z < foco - 25.0f || -l.z > foco + 80.0f)
            {
                continue;
            }

            float parpadeo = 0.85f + 0.15f * std::sin(tv * 13.0f + (float)f * 2.1f);
            HaloB({ l.x, l.y + 0.25f, l.z }, 0.9f * parpadeo, Color{ 120, 64, 20, 255 }, Color{ 0, 0, 0, 255 }, 12);
        }

        for (int e = 0; e < 2; e++)
        {
            const VisualBalsaRio& v = m.visualBalsas[e];

            if (!m.balsas[e].activa || v.destello <= 0.0f)
            {
                continue;
            }

            Color c = TonoB(MezclaB(ColorEquipoBalsas(e), WHITE, 0.3f), 0.6f * v.destello);
            HaloB({ m.balsas[e].x, 1.1f, -m.balsas[e].p }, 1.3f + 0.6f * (1.0f - v.destello), c, Color{ 0, 0, 0, 255 }, 14);
        }

        for (int i = 0; i < MAX_PARTICULAS_VISUAL_BALSAS; i++)
        {
            const ParticulaVisualBalsas& p = m.particulasVisual[i];

            if (p.activa && p.tipo == 3)
            {
                float vida = Lim01B(p.vida / p.vidaMaxima);
                PuntoB(p.posicion, p.tamano * 2.2f, TonoB(p.color, 0.45f * vida));
            }
        }

        rlEnd();
        rlDrawRenderBatchActive();
        EndBlendMode();
    }
}


static Camera3D VistaBalsas(const MinijuegoBalsasRapido& m)
{
    Camera3D vista = m.camara;

    // En la cuenta la camara baja desde mas alto y atras.
    if (m.fase == FASE_BALSAS_PREPARACION && m.resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Lim01B((m.tiempoPreparacion - 0.3f) / (DURACION_PREPARACION_BALSAS - 0.3f));
        float s = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 8.0f * s;
        vista.position.z += 11.0f * s;
        vista.target.z -= 6.0f * s;
    }

    return vista;
}


static int NumeroJugadorB(const Participante& participante, int indice)
{
    return participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1;
}


static void DibujarAnunciosB(
    const MinijuegoBalsasRapido& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();
    float mov = ObtenerFactorMovimientoCamaraMinijuegos();
    bool jugando = m.fase == FASE_BALSAS_JUGANDO;

    // Anuncio de la bifurcacion (alfa) y carteles de cada carril, que ceden
    // el lugar al anuncio grande.
    float alfaBifurcacion = 0.0f;

    if (jugando && m.anuncioBifurcacion >= 0.0f && m.anuncioBifurcacion < 2.2f)
    {
        float u = m.anuncioBifurcacion / 2.2f;
        alfaBifurcacion = u < 0.08f ? u / 0.08f : (u > 0.75f ? (1.0f - u) / 0.25f : 1.0f);
    }

    // Los carteles aparecen recien cuando termino el anuncio grande y solo
    // si en pantalla hay lugar para los dos rotulos sin pisarse.
    float alfaCarteles[2]{};
    Vector2 cartelIzq[2]{}, cartelDer[2]{};
    float despuesAnuncio = m.anuncioBifurcacion >= 0.0f ? Lim01B((m.anuncioBifurcacion - 2.2f) / 0.35f) : 0.0f;

    for (int e = 0; e < 2; e++)
    {
        float falta = RAPIDO_INICIO - m.balsas[e].p;

        if (!jugando || !m.balsas[e].activa || falta <= -0.5f || falta >= 13.0f)
        {
            continue;
        }

        float c = CentroCarril(e);
        float z = -(RAPIDO_INICIO + 0.6f);
        cartelIzq[e] = GetWorldToScreen({ c + DESVIO_CASCADA, 2.3f, z }, vista);
        cartelDer[e] = GetWorldToScreen({ c + DESVIO_REMANSO, 2.3f, z }, vista);
        float lugar = Lim01B((cartelDer[e].x - cartelIzq[e].x - Px(95.0f)) / Px(25.0f));
        alfaCarteles[e] = Lim01B((13.0f - falta) / 3.0f) * Lim01B((falta + 0.5f) / 2.0f) * despuesAnuncio * lugar;
    }

    // Cartelitos J# sobre cada remero; los dos de una balsa se separan un
    // poco para no pisarse cuando la balsa queda lejos.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorBalsas& estado = m.estadosJugadores[i];

        if (estado.equipo < 0 || !participantes[i].activo)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.75f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        if (estado.lado != 2)
        {
            const BalsaRio& b = m.balsas[estado.equipo];
            Vector2 medio = GetWorldToScreen({ b.x, cabeza.y, -b.p }, vista);
            float separado = medio.x + (estado.lado == 0 ? -1.0f : 1.0f) * Px(21.0f);
            p.x = estado.lado == 0 ? std::fmin(p.x, separado) : std::fmax(p.x, separado);
        }

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", NumeroJugadorB(participantes[i], i));

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Avisos sobre cada balsa (los menores se callan mientras se leen los
    // carteles de la bifurcacion; el HUD igual los muestra).
    for (int e = 0; e < 2; e++)
    {
        const BalsaRio& b = m.balsas[e];
        const VisualBalsaRio& v = m.visualBalsas[e];

        if (!b.activa)
        {
            continue;
        }

        Vector2 p = GetWorldToScreen({ b.x, 3.6f, -b.p }, vista);
        float y = p.y - Px(30.0f);
        bool callar = alfaCarteles[e] > 0.0f || alfaBifurcacion > 0.0f;

        if (v.anuncioMeta > 0.0f)
        {
            float u = 1.0f - v.anuncioMeta / 3.0f;
            float escala = u < 0.08f ? u / 0.08f : 1.0f;
            DibujarTextoMundoContornoUI("¡EN LA META!", p.x, y - Px(8.0f) * (1.0f - escala) * mov, UI_TEXTO_GRANDE, COLOR_ORO_B, Lim01B(v.anuncioMeta / 0.5f) * escala);
        }
        else if (v.anuncioRuta > 0.0f)
        {
            float u = 1.0f - v.anuncioRuta / 1.6f;
            const char* texto = v.ruta == 0 ? "¡CASCADA!" : "¡REMANSO!";
            Color color = v.ruta == 0 ? Color{ 200, 244, 255, 255 } : Color{ 170, 236, 130, 255 };
            DibujarTextoMundoContornoUI(texto, p.x, y - u * Px(24.0f) * mov, UI_TEXTO_GRANDE, color, Lim01B(v.anuncioRuta / 0.4f));
        }
        else if (!callar && v.avisoChoque > 0.0f)
        {
            float u = 1.0f - v.avisoChoque;
            DibujarTextoMundoContornoUI("¡CHOQUE!", p.x, y - u * Px(22.0f) * mov, UI_TEXTO, Color{ 255, 150, 120, 255 }, Lim01B(v.avisoChoque / 0.3f));
        }
        else if (!callar && v.avisoBanana > 0.0f)
        {
            float u = 1.0f - v.avisoBanana / 1.2f;
            DibujarTextoMundoContornoUI("¡TURBO!", p.x, y - u * Px(22.0f) * mov, UI_TEXTO, Color{ 255, 226, 110, 255 }, Lim01B(v.avisoBanana / 0.3f));
        }
        else if (!callar && v.avisoSync > 0.0f)
        {
            float u = 1.0f - v.avisoSync;
            DibujarTextoMundoContornoUI("¡SINCRO!", p.x, y - u * Px(22.0f) * mov, UI_TEXTO, MezclaB(ColorEquipoBalsas(e), WHITE, 0.45f), Lim01B(v.avisoSync / 0.3f));
        }
    }

    // Un solo anuncio grande a la vez, en el tercio superior.
    float tInicio = DURACION_CARRERA_BALSAS - m.tiempoRestante;

    if (jugando && tInicio > 0.9f && tInicio < 3.6f)
    {
        float u = (tInicio - 0.9f) / 2.7f;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.75f ? (1.0f - u) / 0.25f : 1.0f);
        float y = alto * 0.27f - u * Px(10.0f) * mov;
        DibujarTextoMundoContornoUI("¡REMEN A LA VEZ!", ancho * 0.5f, y, UI_TEXTO_GRANDE, Color{ 255, 226, 150, 255 }, alfa);
    }
    else if (alfaBifurcacion > 0.0f)
    {
        float u = m.anuncioBifurcacion / 2.2f;
        float salto = u < 0.1f ? (1.0f - u / 0.1f) * Px(12.0f) * mov : 0.0f;
        DibujarTextoMundoContornoUI("¡SE VIENE LA BIFURCACIÓN!", ancho * 0.5f, alto * 0.27f - salto, UI_TEXTO_GRANDE, Color{ 255, 226, 150, 255 }, alfaBifurcacion);
    }

    // Carteles sobre la entrada de cada canal mientras la balsa de ese
    // carril se acerca: un rotulo corto por canal.
    for (int e = 0; e < 2; e++)
    {
        if (alfaCarteles[e] <= 0.01f)
        {
            continue;
        }

        OpcionesEtiquetaUI op;
        op.pico = true;
        op.alfa = alfaCarteles[e];
        op.borde = Color{ 120, 200, 230, 255 };
        DibujarEtiquetaUI("« CASCADA", cartelIzq[e], 0.0f, op);
        op.borde = Color{ 120, 200, 90, 255 };
        DibujarEtiquetaUI("REMANSO »", cartelDer[e], 0.0f, op);
    }
}


static void PrepararHudB(
    const MinijuegoBalsasRapido& m,
    const Participante participantes[],
    int limite,
    DatosHudMinijuego& hud
)
{
    PrepararDatosHudMinijuego(
        hud,
        "BALSAS DEL RÁPIDO",
        "REMÁ EN EQUIPO HASTA LA META: CASCADA RÁPIDA O REMANSO SEGURO",
        m.resultado
    );

    hud.fase = m.fase == FASE_BALSAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_BALSAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_CARRERA_BALSAS;

    for (int i = 0; i < limite; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        const EstadoJugadorBalsas& estado = m.estadosJugadores[i];

        if (!h.visible || estado.equipo < 0)
        {
            continue;
        }

        const BalsaRio& b = m.balsas[estado.equipo];
        const VisualBalsaRio& v = m.visualBalsas[estado.equipo];
        float avance = Lim01B(b.p / LONGITUD_META);
        int porcentaje = (int)(avance * 100.0f);

        h.equipo = estado.equipo;
        std::snprintf(h.valor, sizeof(h.valor), "%d %%", porcentaje);
        h.valorNumerico = porcentaje / 10;
        h.progreso = avance;
        h.rol = estado.lado == 0 ? "REMO IZQUIERDO" : (estado.lado == 1 ? "REMO DERECHO" : "REMA SOLO");

        if (b.terminada) std::snprintf(h.estado, sizeof(h.estado), "¡EN LA META!");
        else if (v.avisoChoque > 0.0f) std::snprintf(h.estado, sizeof(h.estado), "¡CHOQUE!");
        else if (b.tiempoBoost > 0.0f) std::snprintf(h.estado, sizeof(h.estado), "¡TURBO!");
        else if (b.tiempoSync > 0.0f) std::snprintf(h.estado, sizeof(h.estado), "¡SINCRO!");

        if (b.terminada) std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "META");
        else std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d %%", porcentaje);

        if (!JugadorEsBot(participantes[i]))
        {
            if (estado.lado == 2)
            {
                std::snprintf(h.controles, sizeof(h.controles), "REMÁ: %s\nGIRÁ: %s + REMÁ",
                    TextoBotonGolpeHud(participantes[i]), TextoMoverHud(participantes[i]));
            }
            else
            {
                std::snprintf(h.controles, sizeof(h.controles), "REMÁ: %s\n%s",
                    TextoBotonGolpeHud(participantes[i]), estado.lado == 0 ? "LADO IZQUIERDO" : "LADO DERECHO");
            }
        }
    }

    if (m.fase == FASE_BALSAS_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        bool alguienLlego = m.balsas[0].terminada || m.balsas[1].terminada;
        hud.finPorTiempo = !alguienLlego;

        if (m.empate) hud.notaFinal = "LAS DOS BALSAS QUEDARON IGUALADAS";
        else if (alguienLlego) hud.notaFinal = "¡LLEGÓ PRIMERA A LA META DEL TEMPLO!";
        else hud.notaFinal = "NADIE LLEGÓ: GANA LA BALSA MÁS AVANZADA";
    }
}


void MinijuegoBalsasRapido::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    Camera3D vista = VistaBalsas(*this);
    g_ojoB = vista.position;
    {
        Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
        g_derB = Vector3Normalize(Vector3CrossProduct(frente, { 0.0f, 1.0f, 0.0f }));
        g_arribaB = Vector3CrossProduct(g_derB, frente);
    }

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(COLOR_CIELO_BAJO_B);
    HornearBalsas(*this);

    float foco = focoCamara;
    float tiempoShader = efectos ? tiempoVisual : 0.0f;

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarHornoB(foco - 30.0f, foco + 150.0f, tiempoShader);
    DibujarCieloHornoB(tiempoShader);
    rlDrawRenderBatchActive();

    // Lo que se mueve: balsas con sus remos, troncos y bananas.
    DibujarDinamicoOpacoB(*this, decoracion);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorModeloCompartido(jugadores[i], visual);
    }

    // Velos sobre el agua y particulas, sin escribir profundidad.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    DibujarDinamicoTransparenteB(*this, efectos);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();

    if (mostrarDebug)
    {
        for (int e = 0; e < 2; e++)
        {
            DrawCircle3D({ balsas[e].x, 0.1f, -balsas[e].p }, RADIO_BALSA, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }
    }

    EndMode3D();

    DibujarAnunciosB(*this, jugadores, participantes, limite, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    DatosHudMinijuego hud;
    PrepararHudB(*this, participantes, limite, hud);
    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoBalsasRapido::ObtenerResultado() const
{
    return resultado;
}


