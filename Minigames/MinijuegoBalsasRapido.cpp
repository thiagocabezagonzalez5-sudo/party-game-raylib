#include "Minigames/MinijuegoBalsasRapido.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


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

    ActualizarCamara(*this, 0.0f, true);
}


void MinijuegoBalsasRapido::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
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

void MinijuegoBalsasRapido::Actualizar(
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
// VISUAL: ESCENARIO DE SELVA
//==================================================
//
// MODELO FUTURO: rio con cascadas (GLB), balsa de troncos con remos
// animados, arboles gigantes con lianas, ruinas cubiertas de musgo, rocas
// y troncos flotantes, remolino con shader, bananas, loros animados y
// arco de meta. La logica (CrearRecorrido / ResolverObstaculos) no
// depende de estas funciones.

static const Color COLOR_AGUA = { 40, 190, 190, 255 };
static const Color COLOR_MUSGO = { 70, 120, 55, 255 };
static const Color COLOR_ROCA = { 110, 112, 118, 255 };


static void DibujarAguaYTierra(const MinijuegoBalsasRapido& m)
{
    float t = m.tiempoAnimacion;
    float centroZ = -0.5f * (LONGITUD_META + 20.0f);
    float largo = LONGITUD_META + 40.0f;

    // Carriles de agua turquesa.
    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);

        DrawCube({ c, -0.15f, centroZ }, 2.0f * MEDIO_ANCHO_CARRIL, 0.3f, largo, COLOR_AGUA);
        DrawCube({ c, -0.12f, centroZ }, 2.0f * MEDIO_ANCHO_CARRIL - 0.4f, 0.3f, largo, Color{ 60, 210, 205, 255 });
    }

    // Espuma que corre con la corriente (solo la ventana visible).
    float desplazamiento = std::fmod(t * 3.0f, 4.0f);
    int inicio = (int)((m.focoCamara - 20.0f) / 4.0f);

    for (int k = inicio; k < inicio + 16; k++)
    {
        float p = (float)k * 4.0f + desplazamiento;

        for (int e = 0; e < 2; e++)
        {
            float c = CentroCarril(e);
            float xo = (float)((int)HashIndice(k * 2 + e) % 9) - 4.0f;

            DrawCube({ c + xo, 0.02f, -p }, 1.4f, 0.02f, 0.14f, Fade(WHITE, 0.4f));
        }
    }

    // Isla central y orillas.
    DrawCube({ 0.0f, 0.05f, centroZ }, 2.0f * MEDIO_ANCHO_ISLA, 0.5f, largo, COLOR_MUSGO);
    DrawCube({ 0.0f, 0.32f, centroZ }, 2.0f * MEDIO_ANCHO_ISLA - 0.8f, 0.06f, largo, Color{ 90, 145, 65, 255 });

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (CENTRO_CARRIL + MEDIO_ANCHO_CARRIL + 4.0f);

        DrawCube({ x, 0.6f, centroZ }, 8.0f, 1.6f, largo, Color{ 38, 84, 40, 255 });
        DrawCube({ x + (float)lado * 9.0f, 3.0f, centroZ }, 10.0f, 6.0f, largo, Color{ 30, 70, 38, 255 });
        DrawCube({ (float)lado * (CENTRO_CARRIL + MEDIO_ANCHO_CARRIL - 0.1f), 0.0f, centroZ }, 0.3f, 0.8f, largo, Color{ 80, 80, 70, 255 });
    }
}


static void DibujarVegetacion(const MinijuegoBalsasRapido& m)
{
    float t = m.tiempoAnimacion;
    int inicio = (int)((m.focoCamara - 20.0f) / 7.0f);

    // Arboles gigantes con lianas en las dos orillas.
    for (int k = inicio; k < inicio + 11; k++)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            unsigned int h = HashIndice(k * 2 + (lado + 1));
            float x = (float)lado * (17.5f + (float)(h % 5));
            float z = -((float)k * 7.0f + (float)(h % 3));
            float alto = 8.0f + (float)(h % 4);
            Vector3 base = { x, 1.4f, z };

            DrawCylinder(base, 0.8f, 1.1f, alto, 10, Color{ 92, 62, 38, 255 });
            DrawSphere({ x, 1.4f + alto + 1.0f, z }, 3.6f, Color{ 36, 120, 48, 255 });
            DrawSphere({ x - (float)lado * 2.4f, 1.4f + alto, z + 1.5f }, 2.8f, Color{ 48, 142, 56, 255 });

            for (int v = 0; v < 3; v++)
            {
                float vx = x - (float)lado * (2.2f + (float)v * 0.8f);
                float vz = z + (float)v * 0.9f - 0.9f;
                float balanceo = 0.15f * std::sin(t * 1.2f + (float)(k + v));

                DrawLine3D(
                    { vx, 1.4f + alto - 0.3f, vz },
                    { vx + balanceo, 3.0f + (float)((h + (unsigned int)v) % 4), vz },
                    Color{ 40, 150, 60, 255 }
                );
            }
        }
    }

    // Palmeras y helechos sobre la isla.
    int inicioIsla = (int)((m.focoCamara - 20.0f) / 11.0f);

    for (int k = inicioIsla; k < inicioIsla + 7; k++)
    {
        float x = (k % 2 == 0) ? 0.9f : -0.9f;
        float z = -(float)k * 11.0f;

        DrawCylinder({ x, 0.3f, z }, 0.12f, 0.2f, 2.4f, 6, Color{ 110, 78, 44, 255 });
        DrawSphere({ x, 2.8f, z }, 0.9f, Color{ 52, 150, 62, 255 });
    }
}


static void DibujarRuinas(const MinijuegoBalsasRapido& m)
{
    static const float PUNTOS_RUINAS[5] = { 18.0f, 41.0f, 63.0f, 88.0f, 106.0f };

    for (int k = 0; k < 5; k++)
    {
        float p = PUNTOS_RUINAS[k];

        if (std::fabs(p - m.focoCamara) > 38.0f)
        {
            continue;
        }

        float z = -p;
        float alto = 2.6f - 0.5f * (float)(k % 3);

        DrawCube({ -1.2f, 0.6f + alto * 0.5f, z }, 1.0f, alto, 1.0f, Color{ 150, 150, 140, 255 });
        DrawCube({ -1.2f, 0.65f + alto, z }, 1.1f, 0.15f, 1.1f, COLOR_MUSGO);
        DrawCube({ 1.1f, 0.6f + 0.8f, z + 0.8f }, 1.0f, 1.6f, 1.0f, Color{ 140, 140, 130, 255 });
        DrawCube({ 1.1f, 1.45f, z + 0.8f }, 1.1f, 0.14f, 1.1f, COLOR_MUSGO);
        DrawCube({ 0.0f, 0.5f, z - 1.2f }, 2.4f, 0.4f, 0.9f, Color{ 128, 128, 120, 255 });

        // Bloques musgosos en las orillas.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            DrawCube({ (float)lado * 14.9f, 1.8f, z }, 1.4f, 2.0f, 1.4f, Color{ 120, 124, 114, 255 });
            DrawCube({ (float)lado * 14.9f, 2.85f, z }, 1.5f, 0.12f, 1.5f, COLOR_MUSGO);
        }
    }
}


static void DibujarCascadas(const MinijuegoBalsasRapido& m)
{
    float t = m.tiempoAnimacion;

    // Cascadas de las paredes de la orilla.
    static const float PUNTOS_CASCADAS[3] = { 30.0f, 95.0f, 113.0f };

    for (int k = 0; k < 3; k++)
    {
        float p = PUNTOS_CASCADAS[k];

        if (std::fabs(p - m.focoCamara) > 40.0f)
        {
            continue;
        }

        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = (float)lado * (CENTRO_CARRIL + MEDIO_ANCHO_CARRIL + 0.2f);
            float pulso = 0.65f + 0.2f * std::sin(t * 6.0f + (float)k);

            DrawCube({ x, 3.2f, -p }, 0.4f, 5.6f, 1.8f, Fade(Color{ 220, 245, 255, 255 }, pulso));
            DrawSphere({ x - (float)lado * 0.6f, 0.2f, -p }, 0.9f, Fade(WHITE, 0.55f));
        }
    }

    // Rapido: peldanos de espuma en el canal de la cascada de cada carril.
    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e) + DESVIO_CASCADA;

        for (int k = 0; k < 6; k++)
        {
            float p = RAPIDO_INICIO + 4.0f + (float)k * 4.8f;
            float fase = std::fmod(t * 2.0f + (float)k * 0.4f, 1.0f);

            DrawCube({ c, 0.04f, -p }, 2.6f, 0.04f, 0.5f + 0.3f * fase, Fade(WHITE, 0.6f - 0.4f * fase));
        }
    }
}


static void DibujarObstaculos(const MinijuegoBalsasRapido& m)
{
    float t = m.tiempoAnimacion;

    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);

        for (int k = 0; k < m.cantidadObstaculos; k++)
        {
            const ObstaculoBalsas& o = m.obstaculos[k];

            if (std::fabs(o.p - m.focoCamara) > 45.0f)
            {
                continue;
            }

            float x = c + o.desvio;
            float z = -o.p;

            if (o.tipo == OBSTACULO_BALSAS_ROCA)
            {
                DrawSphere({ x, 0.25f, z }, o.a, COLOR_ROCA);
                DrawSphere({ x + 0.25f, 0.55f, z - 0.1f }, o.a * 0.6f, Color{ 128, 130, 134, 255 });
                DrawCube({ x - 0.1f, 0.62f, z }, o.a * 1.0f, 0.12f, o.a * 0.9f, COLOR_MUSGO);
            }
            else if (o.tipo == OBSTACULO_BALSAS_TRONCO)
            {
                float giro = 0.06f * std::sin(t * 1.5f + (float)k);

                DrawCylinderEx(
                    { x - o.a, 0.15f, z - giro * 4.0f },
                    { x + o.a, 0.15f, z + giro * 4.0f },
                    0.42f, 0.42f, 8, Color{ 105, 70, 40, 255 }
                );
                DrawCube({ x + o.a, 0.15f, z + giro * 4.0f }, 0.1f, 0.7f, 0.7f, Color{ 150, 110, 70, 255 });
            }
            else if (o.tipo == OBSTACULO_BALSAS_DIVISOR)
            {
                float largo = 2.0f * o.b;

                DrawCube({ x, 0.3f, z }, 2.0f * o.a, 1.0f, largo, COLOR_ROCA);
                DrawCube({ x, 0.85f, z }, 2.0f * o.a + 0.1f, 0.14f, largo, COLOR_MUSGO);

                for (int r = 0; r < 4; r++)
                {
                    DrawSphere(
                        { x + ((r % 2 == 0) ? 0.2f : -0.2f), 0.9f, z - o.b + (float)r * largo / 3.0f },
                        0.55f,
                        Color{ 120, 124, 128, 255 }
                    );
                }
            }
            else if (o.tipo == OBSTACULO_BALSAS_REMOLINO)
            {
                for (int r = 0; r < 4; r++)
                {
                    float radio = o.a * (1.0f - 0.2f * (float)r);
                    float angulo = t * (160.0f + 40.0f * (float)r) * (o.desvio > 0.0f ? -1.0f : 1.0f);

                    DrawCircle3D({ x, 0.04f + 0.01f * (float)r, z }, radio, { 0.0f, 1.0f, 0.0f }, angulo, Fade(Color{ 15, 90, 110, 255 }, 0.8f));
                    DrawCircle3D({ x, 0.05f + 0.01f * (float)r, z }, radio * 0.92f, { 0.0f, 1.0f, 0.0f }, angulo + 40.0f, Fade(WHITE, 0.5f));
                    DrawCube(
                        { x + std::cos(angulo * DEG2RAD) * radio * 0.8f, 0.06f, z + std::sin(angulo * DEG2RAD) * radio * 0.8f },
                        0.3f, 0.05f, 0.3f, Fade(WHITE, 0.7f)
                    );
                }
            }
            else if (o.tipo == OBSTACULO_BALSAS_BANANA && !m.balsas[e].recogidas[k])
            {
                float flota = 0.3f + 0.12f * std::sin(t * 4.0f + (float)k);

                DrawCylinderEx({ x - 0.25f, flota, z }, { x, flota + 0.1f, z }, 0.12f, 0.12f, 6, Color{ 255, 220, 40, 255 });
                DrawCylinderEx({ x, flota + 0.1f, z }, { x + 0.25f, flota + 0.4f, z }, 0.12f, 0.08f, 6, Color{ 255, 230, 70, 255 });
                DrawCircle3D({ x, 0.05f, z }, 0.7f + 0.1f * std::sin(t * 4.0f), { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(YELLOW, 0.6f));
            }
        }
    }
}


static void DibujarLoros(const MinijuegoBalsasRapido& m)
{
    static const Color COLORES[3] =
    {
        { 230, 50, 50, 255 },
        { 60, 120, 240, 255 },
        { 250, 210, 40, 255 }
    };

    float t = m.tiempoAnimacion;

    for (int k = 0; k < 6; k++)
    {
        float lado = (k % 2 == 0) ? 1.0f : -1.0f;
        float p = m.focoCamara + (float)k * 7.0f - 8.0f + 4.0f * std::sin(t * 0.6f + (float)k);
        float x = lado * (15.5f + 1.5f * std::sin(t * 0.8f + (float)k * 2.0f));
        float y = 5.0f + 0.8f * std::sin(t * 1.7f + (float)k);
        float aleteo = 0.35f * std::sin(t * 16.0f + (float)k);
        Color color = COLORES[k % 3];

        DrawSphere({ x, y, -p }, 0.28f, color);
        DrawSphere({ x, y + 0.2f, -p - 0.22f }, 0.17f, color);
        DrawCube({ x, y + 0.2f, -p - 0.4f }, 0.1f, 0.06f, 0.14f, ORANGE);
        DrawCube({ x - 0.35f, y + aleteo, -p }, 0.5f, 0.05f, 0.3f, color);
        DrawCube({ x + 0.35f, y + aleteo, -p }, 0.5f, 0.05f, 0.3f, color);
        DrawCube({ x, y - 0.05f, -p + 0.35f }, 0.08f, 0.05f, 0.5f, Color{ 40, 80, 200, 255 });
    }
}


static void DibujarMeta(const MinijuegoBalsasRapido& m)
{
    if (m.focoCamara < LONGITUD_META - 55.0f)
    {
        return;
    }

    float z = -LONGITUD_META;

    // Linea de meta a cuadros sobre el agua.
    for (int e = 0; e < 2; e++)
    {
        float c = CentroCarril(e);

        for (int k = 0; k < 12; k++)
        {
            for (int fila = 0; fila < 2; fila++)
            {
                bool claro = (k + fila) % 2 == 0;

                DrawCube(
                    { c - 5.5f + (float)k * 0.92f, 0.04f, z - (float)fila * 0.9f },
                    0.92f, 0.03f, 0.9f,
                    claro ? RAYWHITE : Color{ 30, 30, 30, 255 }
                );
            }
        }
    }

    // Arco de piedra con viga.
    float extremo = CENTRO_CARRIL + MEDIO_ANCHO_CARRIL;

    DrawCube({ -extremo, 2.6f, z }, 1.0f, 5.2f, 1.0f, Color{ 150, 150, 140, 255 });
    DrawCube({ extremo, 2.6f, z }, 1.0f, 5.2f, 1.0f, Color{ 150, 150, 140, 255 });
    DrawCube({ 0.0f, 2.6f, z }, 1.2f, 5.2f, 1.2f, Color{ 150, 150, 140, 255 });
    DrawCube({ 0.0f, 5.4f, z }, 2.0f * extremo + 1.2f, 0.7f, 1.0f, Color{ 120, 124, 114, 255 });

    for (int k = 0; k < 20; k++)
    {
        float x = -extremo + 0.6f + (float)k * (2.0f * extremo - 1.2f) / 19.0f;

        DrawCube({ x, 5.1f, z + 0.55f }, 1.0f, 0.4f, 0.06f, k % 2 == 0 ? RAYWHITE : Color{ 30, 30, 30, 255 });
    }
}


static void DibujarBalsa(const MinijuegoBalsasRapido& m, const BalsaRio& b)
{
    float t = m.tiempoAnimacion;
    float y = 0.12f + 0.04f * std::sin(t * 3.0f + (float)b.equipo);
    Color colorEquipo = ColorEquipoBalsas(b.equipo);

    rlPushMatrix();
    rlTranslatef(b.x, y, -b.p);
    rlRotatef(-b.rumbo * RAD2DEG, 0.0f, 1.0f, 0.0f);

    for (int k = 0; k < 5; k++)
    {
        unsigned int h = HashIndice(k + b.equipo * 7);
        Color madera = { (unsigned char)(150 + h % 30), (unsigned char)(100 + h % 20), 56, 255 };

        DrawCube({ -0.68f + (float)k * 0.34f, 0.0f, 0.0f }, 0.32f, 0.22f, 2.6f, madera);
    }

    DrawCube({ 0.0f, 0.14f, -0.8f }, 1.8f, 0.08f, 0.14f, Color{ 90, 60, 36, 255 });
    DrawCube({ 0.0f, 0.14f, 0.8f }, 1.8f, 0.08f, 0.14f, Color{ 90, 60, 36, 255 });
    DrawCylinder({ 0.0f, 0.1f, -1.15f }, 0.03f, 0.03f, 1.4f, 6, Color{ 90, 60, 36, 255 });
    DrawCube({ 0.25f, 1.25f, -1.15f }, 0.5f, 0.3f, 0.04f, colorEquipo);

    // Remos: bajan al agua y barren hacia atras al remar.
    for (int s = 0; s < 2; s++)
    {
        float signo = s == 0 ? -1.0f : 1.0f;
        float tp = b.tiempoPalada[s];
        float zb = -0.8f;
        float yb = 0.35f;

        if (tp < 0.28f)
        {
            zb = -0.8f + 1.7f * (tp / 0.28f);
            yb = -0.05f;
        }
        else if (tp < 0.5f)
        {
            zb = 0.9f - 1.7f * ((tp - 0.28f) / 0.22f);
            yb = 0.5f;
        }

        Vector3 eje = { signo * 0.9f, 0.55f, 0.15f };
        Vector3 pala = { signo * 1.75f, yb, zb };

        DrawCylinderEx(eje, pala, 0.05f, 0.05f, 6, Color{ 120, 84, 48, 255 });
        DrawCube(pala, 0.14f, 0.05f, 0.55f, Color{ 235, 215, 150, 255 });
    }

    rlPopMatrix();

    if (b.tiempoBoost > 0.0f)
    {
        DrawCircle3D({ b.x, 0.08f, -b.p }, 1.7f + 0.15f * std::sin(t * 12.0f), { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(YELLOW, 0.8f));
    }

    if (b.tiempoSync > 0.0f)
    {
        DrawCircle3D({ b.x, 0.1f, -b.p }, 1.4f + (0.7f - b.tiempoSync) * 2.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(WHITE, b.tiempoSync));
    }
}


//==================================================
// DIBUJO
//==================================================

static const char* TextoBotonPalada(const Participante& participante)
{
    if (participante.esBot || !participante.conectado) return "AUTO";
    if (participante.control == CONTROL_GAMEPAD) return "B";
    if (participante.control == CONTROL_TECLADO_FLECHAS) return "SHIFT DER";

    return "E";
}


static void ListaJugadoresEquipo(
    const MinijuegoBalsasRapido& m,
    int equipo,
    const Participante participantes[],
    int limite,
    char* destino,
    int capacidad
)
{
    destino[0] = '\0';
    int escrito = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.estadosJugadores[i].equipo != equipo)
        {
            continue;
        }

        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        int n = std::snprintf(
            destino + escrito,
            (size_t)(capacidad - escrito),
            "%sJ%d%s",
            escrito > 0 ? " " : "",
            numero,
            participantes[i].esBot ? "B" : ""
        );

        if (n < 0 || n >= capacidad - escrito)
        {
            break;
        }

        escrito += n;
    }
}


static void TextoCentrado(const char* texto, int cx, int y, int tamano, Color color)
{
    DrawText(texto, cx - MeasureText(texto, tamano) / 2, y, tamano, color);
}


void MinijuegoBalsasRapido::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    ClearBackground(Color{ 20, 48, 36, 255 });
    BeginMode3D(camara);

    DibujarAguaYTierra(*this);
    DibujarVegetacion(*this);
    DibujarRuinas(*this);
    DibujarCascadas(*this);
    DibujarObstaculos(*this);
    DibujarMeta(*this);

    for (int e = 0; e < 2; e++)
    {
        if (balsas[e].activa)
        {
            DibujarBalsa(*this, balsas[e]);

            if (mostrarDebug)
            {
                DrawCircle3D({ balsas[e].x, 0.1f, -balsas[e].p }, RADIO_BALSA, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
            }
        }
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorBalsas& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = ColorEquipoBalsas(estado.equipo);
        DibujarJugadorCuboPrueba(jugadores[i], visual);
    }

    DibujarLoros(*this);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_BALSAS);

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    int centro = anchoPantalla / 2;

    // Etiquetas sobre las balsas.
    for (int e = 0; e < 2; e++)
    {
        if (!balsas[e].activa)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen({ balsas[e].x, 3.2f, -balsas[e].p }, camara);
        int x = (int)pantalla.x;
        int y = (int)pantalla.y;
        const char* nombre = TextFormat("EQUIPO %d", e + 1);

        TextoCentrado(nombre, x, y - 22, 20, ColorEquipoBalsas(e));

        if (balsas[e].tiempoSync > 0.0f)
        {
            TextoCentrado("SINCRO!", x, y - 46, 20, WHITE);
        }
        else if (balsas[e].tiempoBoost > 0.0f)
        {
            TextoCentrado("BOOST!", x, y - 46, 20, YELLOW);
        }
        else if (balsas[e].tiempoImpacto > 0.0f)
        {
            TextoCentrado("CHOQUE!", x, y - 46, 20, RED);
        }
    }

    // Panel de progreso.
    char equipo0[32];
    char equipo1[32];
    ListaJugadoresEquipo(*this, 0, participantes, limite, equipo0, 32);
    ListaJugadoresEquipo(*this, 1, participantes, limite, equipo1, 32);

    int panelAncho = 560;
    int px = centro - panelAncho / 2;
    DrawRectangle(px, 8, panelAncho, 72, Fade(BLACK, 0.65f));

    for (int e = 0; e < 2; e++)
    {
        int fila = 14 + e * 30;
        float avance = Acotar(balsas[e].p / LONGITUD_META, 0.0f, 1.0f);
        const char* nombres = e == 0 ? equipo0 : equipo1;

        DrawText(nombres, px + 12, fila + 2, 18, ColorEquipoBalsas(e));
        DrawRectangle(px + 170, fila, 370, 20, Fade(DARKGRAY, 0.8f));
        DrawRectangle(px + 170, fila, (int)(370.0f * avance), 20, ColorEquipoBalsas(e));
        DrawRectangleLines(px + 170, fila, 370, 20, RAYWHITE);
        DrawText(TextFormat("%d%%", (int)(avance * 100.0f)), px + 176, fila + 3, 14, BLACK);
        DrawText(TextFormat("B%d", balsas[e].bananas), px + 504, fila + 3, 14, YELLOW);
    }

    const char* textoTiempo = TextFormat("TIEMPO %d", (int)std::ceil(tiempoRestante));
    DrawText(textoTiempo, 24, 20, 30, tiempoRestante <= 10.0f ? RED : RAYWHITE);

    // Aviso de la bifurcacion.
    float menorP = balsas[0].p < balsas[1].p ? balsas[0].p : balsas[1].p;
    float mayorP = balsas[0].p < balsas[1].p ? balsas[1].p : balsas[0].p;

    if (fase == FASE_BALSAS_JUGANDO && mayorP > 34.0f && menorP < RAPIDO_FIN)
    {
        TextoCentrado("RAPIDO: IZQ = CASCADA CORTA CON ROCAS   DER = REMANSO LARGO Y SEGURO", centro, 94, 18, Color{ 180, 255, 240, 255 });
    }

    const char* ayuda = "REMAR: E / SHIFT DER / B (cada uno en su lado)   SOLO: IZQ/DER + REMAR PARA GIRAR   PALADAS JUNTAS = BONUS";
    DrawRectangle(0, altoPantalla - 38, GetScreenWidth(), 38, Fade(BLACK, 0.78f));
    TextoCentrado(ayuda, centro, altoPantalla - 29, 16, RAYWHITE);

    if (fase == FASE_BALSAS_PREPARACION)
    {
        int fila = 0;

        for (int i = 0; i < limite; i++)
        {
            if (estadosJugadores[i].equipo < 0 || JugadorEsBot(participantes[i]))
            {
                continue;
            }

            const char* lado = estadosJugadores[i].lado == 0
                ? "LADO IZQ"
                : (estadosJugadores[i].lado == 1 ? "LADO DER" : "AMBOS LADOS (IZQ/DER GIRA)");

            DrawText(
                TextFormat(
                    "J%d %s: %s",
                    participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1,
                    lado,
                    TextoBotonPalada(participantes[i])
                ),
                28,
                altoPantalla / 2 + 40 + fila * 22,
                20,
                ColorEquipoBalsas(estadosJugadores[i].equipo)
            );
            fila++;
        }

        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        TextoCentrado(TextFormat("%d", numero), centro, altoPantalla / 2 - 160, 96, GOLD);
    }
    else if (
        fase == FASE_BALSAS_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int ancho = 520;
        int alto = 170;
        int py = altoPantalla / 2 - alto / 2 - 40;

        DrawRectangle(centro - ancho / 2, py, ancho, alto, Fade(BLACK, 0.9f));

        const char* titulo = empate
            ? "EMPATE EN EL RIO"
            : TextFormat("GANA EL EQUIPO %d", equipoGanador + 1);
        TextoCentrado(titulo, centro, py + 16, 32, empate ? YELLOW : ColorEquipoBalsas(equipoGanador));

        const char* marcador = TextFormat("%d u  -  %d u", (int)balsas[0].p, (int)balsas[1].p);
        TextoCentrado(marcador, centro, py + 62, 40, RAYWHITE);
        TextoCentrado(TextoReinicioMinijuego(), centro, py + alto - 34, 18, RAYWHITE);
    }
}


const ResultadoMinijuego& MinijuegoBalsasRapido::ObtenerResultado() const
{
    return resultado;
}
