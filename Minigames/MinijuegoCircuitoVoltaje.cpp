#include "Minigames/MinijuegoCircuitoVoltaje.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// CIRCUITO VOLTAJE - PISTA NOCTURNA ELECTRICA 3D
//==================================================
//
// Logica: cada jugador solo tiene "avance" (vueltas recorridas, 0..4),
// velocidad y estado de trompo. La pista 3D es un ovalo en el plano XZ
// y se calcula a partir del avance: la logica no depende de la decoracion.
// Recta = arriba/abajo del ovalo, curva = extremos izquierdo y derecho.
//==================================================


static const float DURACION_PREPARACION_CIRCUITO = 2.5f;
static const float DURACION_CARRERA_CIRCUITO = 45.0f;
static const float VUELTAS_CIRCUITO = 4.0f;

// Geometria del ovalo: el borde k (0 = exterior, 4 = interior) tiene
// semiejes (RADIO_X_EXTERIOR - k * ANCHO_CARRIL, ...). Cada carril es
// la franja entre dos bordes consecutivos.
static const float RADIO_X_EXTERIOR = 18.0f;
static const float RADIO_Z_EXTERIOR = 10.4f;
static const float ANCHO_CARRIL = 1.6f;
static const float PERALTE_MAXIMO = 1.1f;


// Parte visual (definida al final): no toca el azar ni las reglas.
static void ReiniciarVisualCircuito(MinijuegoCircuitoVoltaje& minijuego);

static void ActualizarVisualCircuito(
    MinijuegoCircuitoVoltaje& minijuego,
    float deltaTime,
    const Participante participantes[]
);


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

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


// IA: acelera mientras va por debajo de la velocidad segura de la zona
// (con un margen distinto por bot) y, de vez en cuando, se pasa de
// acelerador a proposito para que no sea perfecta.
static void ActualizarBotCircuito(
    EstadoJugadorCircuitoVoltaje& jugador,
    int indiceParticipante,
    float tiempoAnimacion,
    bool& acelerando
)
{
    (void)indiceParticipante;
    float velocidadSegura = VelocidadSeguraCircuito(jugador.avance);
    float margen = jugador.margenBot;

    acelerando = jugador.velocidad < velocidadSegura - margen;

    float pulsoError = std::sin(
        tiempoAnimacion * 0.72f + jugador.faseErrorBot
    );

    if (pulsoError > 0.965f)
    {
        acelerando = true;
    }
}


void MinijuegoCircuitoVoltaje::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
        jugadores[i].margenBot = 0.03f + 0.06f * (float)GetRandomValue(0, 1000) / 1000.0f;
        jugadores[i].faseErrorBot = 6.2832f * (float)GetRandomValue(0, 1000) / 1000.0f;
    }

    fase = FASE_CIRCUITO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_CIRCUITO;
    tiempoCarrera = DURACION_CARRERA_CIRCUITO;
    tiempoAnimacion = 0.0f;

    ReiniciarVisualCircuito(*this);
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
    ActualizarVisualCircuito(*this, deltaTime, participantes);

    if (fase == FASE_CIRCUITO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CIRCUITO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CIRCUITO_CARRERA;
        }

        return;
    }

    float carreraAntes = tiempoCarrera;
    tiempoCarrera -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, carreraAntes, tiempoCarrera);
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

        if (participantes[i].esBot || !participantes[i].conectado)
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
            ReproducirSonidoMinijuego(audio, SONIDO_GOLPE);
            continue;
        }

        float vueltaAntes = std::floor(jugador.avance);
        jugador.avance += jugador.velocidad * 0.175f * deltaTime;

        if (jugador.avance >= VUELTAS_CIRCUITO)
        {
            jugador.avance = VUELTAS_CIRCUITO;
            jugador.llegoMeta = true;
            alguienLlego = true;
        }
        else if (std::floor(jugador.avance) > vueltaAntes)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
        }
    }

    if (alguienLlego || tiempoCarrera <= 0.0f)
    {
        tiempoCarrera = tiempoCarrera < 0.0f ? 0.0f : tiempoCarrera;
        FinalizarCircuitoVoltaje(*this);
    }
}

//==================================================
// VISUAL: AUTODROMO NOCTURNO DE AUTOS ELECTRICOS
//==================================================
//
// La carrera solo conoce el avance de cada auto; todo lo de aqui se
// deduce de ese estado y no lo modifica. El azar visual usa su propia
// semilla (semillaVisual y hashes por indice), nunca GetRandomValue.
//
// Capas (de adelante hacia atras):
//  1. Arena: ovalo de asfalto con peralte, cuatro carriles, pianos rojos
//     y blancos en las curvas (la zona donde hay que soltar), lineas de
//     frenado y de meta.
//  2. Anillo cercano: muro con publicidad y chevrones, alambrado, boxes
//     con su calle, neumaticos, conos, cargadores, carpas, podio, auto
//     de seguridad, puestos de comisarios y camara de TV.
//  3. Media distancia: tribuna principal y laterales con publico, torres
//     de luz, portico de meta con semaforo y pantalla gigante.
//  4. Horizonte: ciudad iluminada en el valle y dos cordones de sierras
//     con molinos de viento.
//  5. Cielo: degrade nocturno, luna, estrellas y reflectores.
//
// MODELO FUTURO: auto electrico, portico de meta con semaforo, torre de
// luz, modulo de tribuna, box de equipo, cargador, pantalla gigante,
// molino de viento, auto de seguridad y carpa.
//==================================================


static const int SEGMENTOS_PISTA_CIRCUITO = 120;
static const float DURACION_AVISO_CIRCUITO = 1.5f;
static const float DURACION_ANUNCIO_VUELTA_CIRCUITO = 1.6f;

// Arco de la recta de boxes (lejana) y de las tribunas.
static const float ANGULO_META_CIRCUITO = -PI / 2.0f;
static const float MEDIO_ARCO_BOXES_CIRCUITO = 0.62f;
static const float MEDIO_ARCO_TRIBUNA_CIRCUITO = 0.74f;

// Camara definitiva: vista de transmision desde la recta opuesta.
static const Vector3 OJO_CIRCUITO = { 0.0f, 20.5f, 29.6f };
static const Vector3 MIRA_CIRCUITO = { 0.0f, 0.0f, -6.0f };
static const float FOV_CIRCUITO = 48.0f;

// Paleta: noche azul violacea, asfalto pizarra, pasto verde frio, luz de
// sodio calida y un acento cian electrico reservado a la energia, la
// meta y el semaforo.
static const Color COLOR_CIELO_ALTO_CIRCUITO = { 6, 8, 26, 255 };
static const Color COLOR_CIELO_MEDIO_CIRCUITO = { 22, 22, 58, 255 };
static const Color COLOR_CIELO_BAJO_CIRCUITO = { 70, 50, 104, 255 };
static const Color COLOR_NIEBLA_CIRCUITO = { 38, 36, 76, 255 };
static const Color COLOR_ASFALTO_CIRCUITO = { 62, 65, 76, 255 };
static const Color COLOR_GOMA_CIRCUITO = { 36, 36, 42, 255 };
static const Color COLOR_PASTO_CIRCUITO = { 40, 84, 58, 255 };
static const Color COLOR_PASTO_OSCURO_CIRCUITO = { 26, 54, 42, 255 };
static const Color COLOR_HORMIGON_CIRCUITO = { 140, 142, 150, 255 };
static const Color COLOR_ACERO_CIRCUITO = { 88, 96, 116, 255 };
static const Color COLOR_ACERO_OSCURO_CIRCUITO = { 46, 50, 64, 255 };
static const Color COLOR_BLANCO_CIRCUITO = { 228, 230, 236, 255 };
static const Color COLOR_PIANO_CIRCUITO = { 216, 54, 60, 255 };
static const Color COLOR_LUZ_CIRCUITO = { 255, 224, 168, 255 };
static const Color COLOR_VOLTAJE_CIRCUITO = { 86, 228, 255, 255 };
static const Color COLOR_AMBAR_CIRCUITO = { 255, 178, 52, 255 };
static const Color COLOR_CARBONO_CIRCUITO = { 30, 32, 40, 255 };
static const Color COLOR_SIERRA_CIRCUITO = { 30, 30, 62, 255 };
static const Color COLOR_EDIFICIO_CIRCUITO = { 34, 36, 62, 255 };
static const Color COLOR_VENTANA_CIRCUITO = { 255, 206, 132, 255 };


static float Limitar01Circuito(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float HashCircuito(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFFu) / 16777215.0f;
}


static float AzarCircuito(MinijuegoCircuitoVoltaje& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static float SuaveCircuito(float u)
{
    u = Limitar01Circuito(u);
    return u * u * (3.0f - 2.0f * u);
}


//--------------------------------------------------
// Color, luz falsa y niebla
//--------------------------------------------------

static unsigned char CanalCircuito(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoCircuito(Color color, float factor)
{
    return Color{
        CanalCircuito((float)color.r * factor),
        CanalCircuito((float)color.g * factor),
        CanalCircuito((float)color.b * factor),
        color.a
    };
}


static Color MezclaCircuito(Color a, Color b, float t)
{
    t = Limitar01Circuito(t);

    return Color{
        CanalCircuito((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalCircuito((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalCircuito((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalCircuito((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color ConAlfaCircuito(Color color, float alfa)
{
    color.a = CanalCircuito(255.0f * Limitar01Circuito(alfa));
    return color;
}


// Noche: las caras de arriba reciben la luz blanca de las torres; los
// costados quedan en sombra azulada.
static Color SombraCircuito(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalCircuito((float)color.r * factor * 0.99f),
            CanalCircuito((float)color.g * factor * 1.0f),
            CanalCircuito((float)color.b * factor * 1.04f),
            color.a
        };
    }

    return Color{
        CanalCircuito((float)color.r * factor * 0.88f),
        CanalCircuito((float)color.g * factor * 0.92f),
        CanalCircuito((float)color.b * factor * 1.06f),
        color.a
    };
}


// Arriba 1.0, frente 0.78, izquierda 0.68, derecha 0.60, fondo 0.50.
static float LuzNormalCircuito(Vector3 n)
{
    float f = 0.64f + 0.36f * n.y + 0.14f * n.z - 0.04f * n.x;
    if (f < 0.42f) f = 0.42f;
    if (f > 1.0f) f = 1.0f;
    return f;
}


// Lo lejano se funde con la bruma violacea de la ciudad.
static float CantidadNieblaCircuito(Vector3 p)
{
    float dx = p.x - OJO_CIRCUITO.x;
    float dy = p.y - OJO_CIRCUITO.y;
    float dz = p.z - OJO_CIRCUITO.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 40.0f) / 190.0f;
    return n < 0.0f ? 0.0f : (n > 0.86f ? 0.86f : n);
}


static Color TinteCircuito(Color color, Vector3 p)
{
    Color niebla = COLOR_NIEBLA_CIRCUITO;
    niebla.a = color.a;
    return MezclaCircuito(color, niebla, CantidadNieblaCircuito(p));
}


//--------------------------------------------------
// Geometria inmediata (rlgl) con caras sombreadas
//--------------------------------------------------

static Vector3 SumaCircuito(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaCircuito(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaCircuito(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }

static Vector3 CruzCircuito(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

static Vector3 NormalizarCircuito(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 1.0e-5f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static void VerticeCircuito(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadCircuito(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeCircuito(a, color);
    VerticeCircuito(b, color);
    VerticeCircuito(c, color);
    VerticeCircuito(d, color);
}


// Caja alineada con caras sombreadas (arriba 1.0, frente 0.78,
// izquierda 0.70, derecha 0.62, fondo 0.54).
static void CuboCircuito(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadCircuito({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraCircuito(color, 1.0f, true));
    QuadCircuito({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraCircuito(color, 0.78f, false));
    QuadCircuito({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraCircuito(color, 0.54f, false));
    QuadCircuito({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraCircuito(color, 0.62f, false));
    QuadCircuito({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraCircuito(color, 0.70f, false));
    rlEnd();
}


static void CuboNieblaCircuito(Vector3 c, Vector3 t, Color color)
{
    CuboCircuito(c, t, TinteCircuito(color, c));
}


// Caja orientada (ex, ey, ez unitarios y ortogonales), luz por normal.
static void CajaOrientadaCircuito(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Vector3 t, Color color)
{
    Vector3 hx = EscalaCircuito(ex, t.x * 0.5f);
    Vector3 hy = EscalaCircuito(ey, t.y * 0.5f);
    Vector3 hz = EscalaCircuito(ez, t.z * 0.5f);

    auto P = [&](float sx, float sy, float sz) -> Vector3
    {
        return {
            c.x + hx.x * sx + hy.x * sy + hz.x * sz,
            c.y + hx.y * sx + hy.y * sy + hz.y * sz,
            c.z + hx.z * sx + hy.z * sy + hz.z * sz
        };
    };

    auto Cara = [&](Vector3 n) -> Color
    {
        return SombraCircuito(color, LuzNormalCircuito(n), n.y > 0.7f);
    };

    rlBegin(RL_QUADS);
    QuadCircuito(P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), P(-1, 1, -1), Cara(ey));
    QuadCircuito(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), Cara(ez));
    QuadCircuito(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), Cara(EscalaCircuito(ez, -1.0f)));
    QuadCircuito(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), Cara(ex));
    QuadCircuito(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), Cara(EscalaCircuito(ex, -1.0f)));
    rlEnd();
}


static void CajaNieblaCircuito(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Vector3 t, Color color)
{
    CajaOrientadaCircuito(c, ex, ey, ez, t, TinteCircuito(color, c));
}


// Viga de seccion rectangular de "a" a "b".
static void VigaCircuito(Vector3 a, Vector3 b, float ancho, float alto, Color color)
{
    Vector3 ez = NormalizarCircuito(RestaCircuito(b, a));
    Vector3 arriba = std::fabs(ez.y) > 0.95f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    Vector3 ex = NormalizarCircuito(CruzCircuito(arriba, ez));
    Vector3 ey = CruzCircuito(ez, ex);
    Vector3 d = RestaCircuito(b, a);
    float largo = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    Vector3 c = EscalaCircuito(SumaCircuito(a, b), 0.5f);
    CajaOrientadaCircuito(c, ex, ey, ez, { ancho, alto, largo }, TinteCircuito(color, c));
}


// Cilindro o tronco de cono de "a" (radio r0) a "b" (r1), luz por normal.
static void CilindroCircuito(Vector3 a, Vector3 b, float r0, float r1, int lados, Color color, bool tapas = true)
{
    Vector3 eje = NormalizarCircuito(RestaCircuito(b, a));
    Vector3 ref = std::fabs(eje.y) > 0.95f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    Vector3 u = NormalizarCircuito(CruzCircuito(ref, eje));
    Vector3 v = CruzCircuito(eje, u);
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaCircuito(EscalaCircuito(u, std::cos(a0)), EscalaCircuito(v, std::sin(a0)));
        Vector3 d1 = SumaCircuito(EscalaCircuito(u, std::cos(a1)), EscalaCircuito(v, std::sin(a1)));
        Vector3 n = SumaCircuito(EscalaCircuito(u, std::cos(am)), EscalaCircuito(v, std::sin(am)));
        Color cara = SombraCircuito(color, LuzNormalCircuito(n), false);

        QuadCircuito(
            SumaCircuito(a, EscalaCircuito(d0, r0)),
            SumaCircuito(a, EscalaCircuito(d1, r0)),
            SumaCircuito(b, EscalaCircuito(d1, r1)),
            SumaCircuito(b, EscalaCircuito(d0, r1)),
            cara
        );

        if (tapas && r1 > 0.001f)
        {
            Color tapa = SombraCircuito(color, LuzNormalCircuito(eje), eje.y > 0.7f);
            QuadCircuito(b, SumaCircuito(b, EscalaCircuito(d1, r1)), SumaCircuito(b, EscalaCircuito(d0, r1)), b, tapa);
        }
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorCircuito(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloCircuito(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorCircuito(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara.
static void ResplandorFrenteCircuito(Vector3 c, float radio, Color color)
{
    Vector3 haciaOjo = NormalizarCircuito(RestaCircuito(OJO_CIRCUITO, c));
    Vector3 u = NormalizarCircuito(CruzCircuito({ 0.0f, 1.0f, 0.0f }, haciaOjo));
    Vector3 v = CruzCircuito(haciaOjo, u);
    ResplandorCircuito(c, u, v, radio, color);
}


// Cinta que mira a la camara entre dos puntos, con color en cada punta
// (conos de luz, chispas, reflectores).
static void HazCircuito(Vector3 a, Vector3 b, float anchoA, float anchoB, Color colorA, Color colorB)
{
    Vector3 dir = RestaCircuito(b, a);
    Vector3 medio = EscalaCircuito(SumaCircuito(a, b), 0.5f);
    Vector3 haciaOjo = RestaCircuito(OJO_CIRCUITO, medio);
    Vector3 lado = NormalizarCircuito(CruzCircuito(dir, haciaOjo));
    Vector3 la = EscalaCircuito(lado, anchoA * 0.5f);
    Vector3 lb = EscalaCircuito(lado, anchoB * 0.5f);

    rlBegin(RL_QUADS);
    VerticeCircuito(RestaCircuito(a, la), colorA);
    VerticeCircuito(SumaCircuito(a, la), colorA);
    VerticeCircuito(SumaCircuito(b, lb), colorB);
    VerticeCircuito(RestaCircuito(b, lb), colorB);
    rlEnd();
}


//--------------------------------------------------
// Ovalo: puntos, marcos y pose de cada auto
//--------------------------------------------------

// Altura del peralte: el exterior sube en las curvas; fuera de la pista
// (k < 0) se mantiene la altura del borde y por dentro (k > 4) es llano.
static float AlturaPistaCircuito(float k, float angulo)
{
    float coseno = std::cos(angulo);
    float factor = 1.0f - k / 4.0f;
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    return PERALTE_MAXIMO * coseno * coseno * factor;
}


// Punto del ovalo para el borde k (0 exterior ... 4 interior; negativos
// hacia afuera, mayores que 4 hacia el infield).
static Vector3 PuntoOvaloCircuito(float k, float angulo, float dy = 0.0f)
{
    return Vector3
    {
        std::cos(angulo) * (RADIO_X_EXTERIOR - ANCHO_CARRIL * k),
        AlturaPistaCircuito(k, angulo) + dy,
        std::sin(angulo) * (RADIO_Z_EXTERIOR - ANCHO_CARRIL * k)
    };
}


static Vector3 PuntoOvaloLlanoCircuito(float k, float angulo, float y)
{
    return Vector3
    {
        std::cos(angulo) * (RADIO_X_EXTERIOR - ANCHO_CARRIL * k),
        y,
        std::sin(angulo) * (RADIO_Z_EXTERIOR - ANCHO_CARRIL * k)
    };
}


struct MarcoCircuito
{
    Vector3 punto;
    Vector3 adelante;   // sentido de la carrera
    Vector3 afuera;     // normal horizontal hacia afuera del ovalo
};


static MarcoCircuito MarcoOvaloCircuito(float k, float angulo, float y)
{
    float rx = RADIO_X_EXTERIOR - ANCHO_CARRIL * k;
    float rz = RADIO_Z_EXTERIOR - ANCHO_CARRIL * k;
    MarcoCircuito marco;
    marco.punto = PuntoOvaloLlanoCircuito(k, angulo, y);
    marco.adelante = NormalizarCircuito({ -std::sin(angulo) * rx, 0.0f, std::cos(angulo) * rz });
    marco.afuera = NormalizarCircuito({ std::cos(angulo) * rz, 0.0f, std::sin(angulo) * rx });
    return marco;
}


// Caja apoyada en un marco del ovalo: "largo" va en el sentido de la
// pista, "fondo" hacia afuera; el centro se corre hacia afuera "salida".
static void CajaMarcoCircuito(const MarcoCircuito& m, float salida, float y, float largo, float alto, float fondo, Color color)
{
    Vector3 c = SumaCircuito(m.punto, EscalaCircuito(m.afuera, salida));
    c.y = y + alto * 0.5f;
    CajaNieblaCircuito(c, m.adelante, { 0.0f, 1.0f, 0.0f }, m.afuera, { largo, alto, fondo }, color);
}


struct PoseAutoCircuito
{
    Vector3 posicion;
    float rumbo;        // grados
    float peralte;      // grados
    Vector3 adelante;
    Vector3 afuera;
};


// Misma ubicacion que usaba la carrera: carril i (i + 0.5 bordes hacia
// adentro), angulo por fraccion de vuelta y peralte segun la curva.
static PoseAutoCircuito CalcularPoseAutoCircuito(int indice, float avance)
{
    float angulo = -PI / 2.0f + FraccionVueltaCircuito(avance) * PI * 2.0f;
    float k = (float)indice + 0.5f;
    float radioX = RADIO_X_EXTERIOR - ANCHO_CARRIL * k;
    float radioZ = RADIO_Z_EXTERIOR - ANCHO_CARRIL * k;
    float intensidad = std::cos(angulo) * std::cos(angulo);

    PoseAutoCircuito pose;
    pose.posicion = PuntoOvaloCircuito(k, angulo);
    pose.rumbo = std::atan2(radioZ * std::cos(angulo), -radioX * std::sin(angulo)) * RAD2DEG;
    pose.peralte = intensidad * 12.0f;
    pose.adelante = NormalizarCircuito({ -std::sin(angulo) * radioX, 0.0f, std::cos(angulo) * radioZ });
    pose.afuera = NormalizarCircuito({ std::cos(angulo) * radioZ, 0.0f, std::sin(angulo) * radioX });
    return pose;
}


static bool ExcedidoCircuito(const MinijuegoCircuitoVoltaje& m, int i)
{
    const EstadoJugadorCircuitoVoltaje& j = m.jugadores[i];

    return m.fase == FASE_CIRCUITO_CARRERA &&
        j.tiempoTrompo <= 0.0f &&
        !j.llegoMeta &&
        j.velocidad > VelocidadSeguraCircuito(j.avance) + 0.075f;
}


static int VueltaActualCircuito(const EstadoJugadorCircuitoVoltaje& j)
{
    int vuelta = (int)std::floor(j.avance) + 1;
    if (vuelta > (int)VUELTAS_CIRCUITO) vuelta = (int)VUELTAS_CIRCUITO;
    if (vuelta < 1) vuelta = 1;
    return vuelta;
}


//--------------------------------------------------
// Estado visual: particulas, avisos y anuncios
//--------------------------------------------------

static const Color PALETA_FIESTA_CIRCUITO[6] =
{
    { 255, 214, 80, 255 },
    { 86, 228, 255, 255 },
    { 255, 96, 186, 255 },
    { 240, 240, 250, 255 },
    { 150, 255, 140, 255 },
    { 255, 150, 60, 255 }
};


static void ReiniciarVisualCircuito(MinijuegoCircuitoVoltaje& m)
{
    for (int i = 0; i < MAX_PARTICULAS_CIRCUITO_VOLTAJE; i++) m.particulas[i] = {};
    for (int i = 0; i < MAX_AVISOS_CIRCUITO_VOLTAJE; i++) m.avisos[i] = {};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.vueltaVista[i] = 0;
        m.tromposVistos[i] = 0;
        m.metaVista[i] = false;
        m.excesoVisto[i] = false;
        m.destelloVuelta[i] = 0.0f;
        m.acumuladorChispas[i] = 0.0f;
    }

    m.siguienteParticula = 0;
    m.semillaVisual = 0x2545F491u;
    m.vueltaLiderVista = 0;
    m.tiempoAnuncioVuelta = 99.0f;
    m.tiempoDesdeFin = 0.0f;
    m.acumuladorFuegos = 0.0f;
}


static int CantidadParticulasCircuito(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


static void EmitirParticulaCircuito(
    MinijuegoCircuitoVoltaje& m,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float duracion,
    float tamano
)
{
    ParticulaCircuitoVoltaje& p = m.particulas[m.siguienteParticula];
    m.siguienteParticula = (m.siguienteParticula + 1) % MAX_PARTICULAS_CIRCUITO_VOLTAJE;

    p.activa = true;
    p.tipo = tipo;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.color = color;
    p.vida = duracion;
    p.duracion = duracion;
    p.tamano = tamano;
}


static void AgregarAvisoCircuito(MinijuegoCircuitoVoltaje& m, int jugador, int tipo, int valor)
{
    int elegido = 0;
    float menorVida = 1.0e9f;

    for (int i = 0; i < MAX_AVISOS_CIRCUITO_VOLTAJE; i++)
    {
        if (!m.avisos[i].activo)
        {
            elegido = i;
            break;
        }

        if (m.avisos[i].vida < menorVida)
        {
            menorVida = m.avisos[i].vida;
            elegido = i;
        }
    }

    // Un aviso nuevo del mismo auto reemplaza al anterior.
    for (int i = 0; i < MAX_AVISOS_CIRCUITO_VOLTAJE; i++)
    {
        if (m.avisos[i].activo && m.avisos[i].jugador == jugador)
        {
            elegido = i;
            break;
        }
    }

    AvisoCircuitoVoltaje& aviso = m.avisos[elegido];
    aviso.activo = true;
    aviso.jugador = jugador;
    aviso.tipo = tipo;
    aviso.valor = valor;
    aviso.vida = DURACION_AVISO_CIRCUITO;
}


// Rafaga radial: vuelta completada, meta, fuegos artificiales.
static void EmitirRafagaCircuito(
    MinijuegoCircuitoVoltaje& m,
    Vector3 centro,
    Color color,
    int cantidad,
    float fuerza,
    int tipo,
    float duracion
)
{
    int n = CantidadParticulasCircuito(cantidad);

    for (int k = 0; k < n; k++)
    {
        float a = AzarCircuito(m, 0.0f, 6.2832f);
        float e = AzarCircuito(m, -0.4f, 1.0f);
        float f = fuerza * AzarCircuito(m, 0.55f, 1.0f);
        float ce = std::sqrt(1.0f - e * e * 0.5f);
        Vector3 v = { std::cos(a) * ce * f, (0.5f + e * 0.8f) * f, std::sin(a) * ce * f };
        Color c = (k % 3 == 0) ? MezclaCircuito(color, WHITE, 0.6f) : color;
        EmitirParticulaCircuito(m, tipo, centro, v, c, duracion * AzarCircuito(m, 0.7f, 1.1f), AzarCircuito(m, 0.05f, 0.1f) * (tipo == 3 ? 4.0f : 1.0f));
    }
}


static void ActualizarVisualCircuito(
    MinijuegoCircuitoVoltaje& m,
    float dt,
    const Participante participantes[]
)
{
    // Particulas.
    for (int i = 0; i < MAX_PARTICULAS_CIRCUITO_VOLTAJE; i++)
    {
        ParticulaCircuitoVoltaje& p = m.particulas[i];
        if (!p.activa) continue;

        p.vida -= dt;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.posicion = SumaCircuito(p.posicion, EscalaCircuito(p.velocidad, dt));

        switch (p.tipo)
        {
            case 0: // chispa electrica
                p.velocidad.y -= 7.0f * dt;
                p.velocidad = EscalaCircuito(p.velocidad, 1.0f - 1.8f * dt);
                break;

            case 1: // humo de goma
                p.velocidad = EscalaCircuito(p.velocidad, 1.0f - 1.6f * dt);
                p.velocidad.y += 0.6f * dt;
                p.tamano += 0.9f * dt;
                break;

            case 2: // confeti
                p.velocidad.y -= 2.4f * dt;
                if (p.velocidad.y < -1.6f) p.velocidad.y = -1.6f;
                p.velocidad.x *= 1.0f - 0.8f * dt;
                p.velocidad.z *= 1.0f - 0.8f * dt;
                break;

            default: // fuego artificial
                p.velocidad.y -= 3.0f * dt;
                p.velocidad = EscalaCircuito(p.velocidad, 1.0f - 0.9f * dt);
                break;
        }
    }

    for (int i = 0; i < MAX_AVISOS_CIRCUITO_VOLTAJE; i++)
    {
        if (!m.avisos[i].activo) continue;
        m.avisos[i].vida -= dt;
        if (m.avisos[i].vida <= 0.0f) m.avisos[i].activo = false;
    }

    m.tiempoAnuncioVuelta += dt;

    int vueltaLider = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.destelloVuelta[i] -= dt * 1.6f;
        if (m.destelloVuelta[i] < 0.0f) m.destelloVuelta[i] = 0.0f;

        if (!m.resultado.participantes[i].participo) continue;

        const EstadoJugadorCircuitoVoltaje& j = m.jugadores[i];
        PoseAutoCircuito pose = CalcularPoseAutoCircuito(i, j.avance);
        Color color = participantes[i].color;
        bool humano = !participantes[i].esBot && participantes[i].conectado;
        int vuelta = (int)std::floor(j.avance);
        if (vuelta > vueltaLider) vueltaLider = vuelta;

        Vector3 centro = pose.posicion;
        centro.y += 0.5f;

        // Vuelta completada o meta.
        if (j.llegoMeta && !m.metaVista[i])
        {
            m.metaVista[i] = true;
            m.vueltaVista[i] = vuelta;
            m.destelloVuelta[i] = 1.4f;
            EmitirRafagaCircuito(m, centro, color, 34, 5.0f, 0, 0.9f);
            EmitirRafagaCircuito(m, centro, COLOR_VOLTAJE_CIRCUITO, 18, 4.0f, 0, 0.8f);
            AgregarAvisoCircuito(m, i, 2, 0);
        }
        else if (vuelta > m.vueltaVista[i] && !j.llegoMeta)
        {
            m.vueltaVista[i] = vuelta;
            m.destelloVuelta[i] = 1.0f;
            EmitirRafagaCircuito(m, centro, color, 20, 4.0f, 0, 0.7f);
            AgregarAvisoCircuito(m, i, 1, vuelta + 1);
        }

        // Trompo: humo, chispas y un temblor corto si lo sufre un humano.
        if (j.trompos > m.tromposVistos[i])
        {
            m.tromposVistos[i] = j.trompos;
            int humo = CantidadParticulasCircuito(10);

            for (int k = 0; k < humo; k++)
            {
                Vector3 v = { AzarCircuito(m, -1.2f, 1.2f), AzarCircuito(m, 0.3f, 1.0f), AzarCircuito(m, -1.2f, 1.2f) };
                Color gris = MezclaCircuito(Color{ 120, 122, 136, 255 }, Color{ 196, 198, 210, 255 }, AzarCircuito(m, 0.0f, 1.0f));
                EmitirParticulaCircuito(m, 1, SumaCircuito(pose.posicion, { 0.0f, 0.25f, 0.0f }), v, gris, AzarCircuito(m, 0.9f, 1.4f), AzarCircuito(m, 0.25f, 0.4f));
            }

            EmitirRafagaCircuito(m, SumaCircuito(pose.posicion, { 0.0f, 0.3f, 0.0f }), COLOR_AMBAR_CIRCUITO, 16, 3.6f, 0, 0.5f);
            AgregarAvisoCircuito(m, i, 0, 0);

            if (humano)
            {
                ActivarTemblorCamaraGeneral(0.12f, 0.24f);
            }
        }

        // Pasarse de velocidad: aviso sonoro al humano al empezar.
        bool exceso = ExcedidoCircuito(m, i);

        if (exceso && !m.excesoVisto[i] && humano)
        {
            ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
        }

        m.excesoVisto[i] = exceso;

        // Estela de chispas segun la velocidad; con exceso, chispas
        // naranjas de las ruedas y algo de humo.
        if (m.fase == FASE_CIRCUITO_CARRERA && !j.llegoMeta)
        {
            float ritmo = j.velocidad * j.velocidad * 26.0f * FactorCalidadGrafica(CalidadParticulas());
            if (exceso) ritmo += 30.0f * FactorCalidadGrafica(CalidadParticulas());
            m.acumuladorChispas[i] += ritmo * dt;

            while (m.acumuladorChispas[i] >= 1.0f)
            {
                m.acumuladorChispas[i] -= 1.0f;
                Vector3 atras = SumaCircuito(pose.posicion, EscalaCircuito(pose.adelante, -1.05f));
                atras.y += 0.22f;
                float lado = AzarCircuito(m, -0.4f, 0.4f);
                atras = SumaCircuito(atras, EscalaCircuito(pose.afuera, lado));
                Vector3 v = EscalaCircuito(pose.adelante, -AzarCircuito(m, 1.0f, 2.6f));
                v.y = AzarCircuito(m, 0.4f, 1.8f);
                v = SumaCircuito(v, EscalaCircuito(pose.afuera, AzarCircuito(m, -0.8f, 0.8f)));

                Color c = exceso && AzarCircuito(m, 0.0f, 1.0f) < 0.6f
                    ? COLOR_AMBAR_CIRCUITO
                    : MezclaCircuito(color, COLOR_VOLTAJE_CIRCUITO, 0.45f);

                EmitirParticulaCircuito(m, 0, atras, v, c, AzarCircuito(m, 0.25f, 0.45f), 0.05f);

                if (exceso && AzarCircuito(m, 0.0f, 1.0f) < 0.25f)
                {
                    Vector3 vh = { AzarCircuito(m, -0.3f, 0.3f), AzarCircuito(m, 0.2f, 0.5f), AzarCircuito(m, -0.3f, 0.3f) };
                    EmitirParticulaCircuito(m, 1, atras, vh, Color{ 170, 172, 186, 255 }, 0.7f, 0.16f);
                }
            }
        }
    }

    // Anuncio central cuando el lider empieza una vuelta nueva.
    if (m.fase == FASE_CIRCUITO_CARRERA && vueltaLider > m.vueltaLiderVista && vueltaLider < (int)VUELTAS_CIRCUITO)
    {
        m.vueltaLiderVista = vueltaLider;
        m.tiempoAnuncioVuelta = 0.0f;
    }

    // Final: confeti sobre el ganador y fuegos sobre la tribuna.
    if (m.fase == FASE_CIRCUITO_TERMINADO)
    {
        m.tiempoDesdeFin += dt;

        if (m.tiempoDesdeFin < 3.2f)
        {
            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1) continue;

                PoseAutoCircuito pose = CalcularPoseAutoCircuito(i, m.jugadores[i].avance);
                int n = CantidadParticulasCircuito(3);

                for (int k = 0; k < n; k++)
                {
                    Vector3 p = { pose.posicion.x + AzarCircuito(m, -1.6f, 1.6f), pose.posicion.y + AzarCircuito(m, 3.2f, 4.4f), pose.posicion.z + AzarCircuito(m, -1.6f, 1.6f) };
                    Vector3 v = { AzarCircuito(m, -0.8f, 0.8f), AzarCircuito(m, -0.4f, 0.6f), AzarCircuito(m, -0.8f, 0.8f) };
                    Color c = k % 2 == 0 ? participantes[i].color : PALETA_FIESTA_CIRCUITO[(int)AzarCircuito(m, 0.0f, 5.99f)];
                    EmitirParticulaCircuito(m, 2, p, v, c, AzarCircuito(m, 2.0f, 2.8f), AzarCircuito(m, 0.08f, 0.13f));
                }
            }
        }

        if (m.tiempoDesdeFin < 6.0f)
        {
            m.acumuladorFuegos += dt;

            while (m.acumuladorFuegos >= 0.55f)
            {
                m.acumuladorFuegos -= 0.55f;
                float lado = AzarCircuito(m, 0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
                Vector3 centro = { lado * AzarCircuito(m, 15.0f, 27.0f), AzarCircuito(m, 10.0f, 15.0f), AzarCircuito(m, -30.0f, -22.0f) };
                Color c = PALETA_FIESTA_CIRCUITO[(int)AzarCircuito(m, 0.0f, 5.99f)];
                EmitirRafagaCircuito(m, centro, c, 28, 6.5f, 3, 1.5f);
            }
        }
    }
}


//--------------------------------------------------
// Capa 5: cielo nocturno (2D, antes del 3D)
//--------------------------------------------------

static void DibujarCieloCircuito(const Camera3D& vista, float t)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int medio = alto / 2;

    DrawRectangleGradientV(0, 0, ancho, medio, COLOR_CIELO_ALTO_CIRCUITO, COLOR_CIELO_MEDIO_CIRCUITO);
    DrawRectangleGradientV(0, medio, ancho, alto - medio, COLOR_CIELO_MEDIO_CIRCUITO, COLOR_CIELO_BAJO_CIRCUITO);

    // Estrellas fijas en la cupula (direcciones por hash), con titileo.
    Vector3 frente = NormalizarCircuito(RestaCircuito(vista.target, vista.position));

    for (int i = 0; i < 170; i++)
    {
        float az = -1.25f + 2.5f * HashCircuito(i, 11);
        float el = -0.16f + 0.62f * HashCircuito(i, 12) * HashCircuito(i, 13);
        Vector3 dir = { std::sin(az) * std::cos(el), std::sin(el), -std::cos(az) * std::cos(el) };

        if (dir.x * frente.x + dir.y * frente.y + dir.z * frente.z < 0.2f) continue;

        Vector2 s = GetWorldToScreen(SumaCircuito(vista.position, EscalaCircuito(dir, 900.0f)), vista);
        if (s.x < 0.0f || s.y < 0.0f || s.x > (float)ancho || s.y > (float)alto) continue;

        float brillo = 0.45f + 0.55f * HashCircuito(i, 14);
        brillo *= 0.7f + 0.3f * std::sin(t * (1.5f + 2.0f * HashCircuito(i, 15)) + (float)i);
        float tam = HashCircuito(i, 16) > 0.86f ? Px(2.5f) : Px(1.5f);
        DrawRectangleV({ s.x, s.y }, { tam, tam }, ConAlfaCircuito(Color{ 226, 232, 255, 255 }, brillo));
    }

    // Luna baja sobre las sierras, a la izquierda.
    Vector3 dirLuna = NormalizarCircuito({ -0.62f, -0.134f, -1.0f });
    Vector2 luna = GetWorldToScreen(SumaCircuito(vista.position, EscalaCircuito(dirLuna, 900.0f)), vista);
    float r = Px(30.0f);

    DrawCircleGradient(luna, r * 4.0f, Color{ 150, 150, 220, 60 }, Color{ 150, 150, 220, 0 });
    DrawCircleV(luna, r * 1.25f, Color{ 200, 200, 240, 40 });
    DrawCircleV(luna, r, Color{ 238, 234, 218, 255 });
    DrawCircleV({ luna.x - r * 0.3f, luna.y - r * 0.2f }, r * 0.22f, Color{ 214, 208, 192, 255 });
    DrawCircleV({ luna.x + r * 0.35f, luna.y + r * 0.25f }, r * 0.16f, Color{ 218, 212, 196, 255 });
    DrawCircleV({ luna.x + r * 0.1f, luna.y - r * 0.45f }, r * 0.1f, Color{ 220, 214, 198, 255 });
}


//--------------------------------------------------
// Capa 4: horizonte (sierras, ciudad en el valle, molinos)
//--------------------------------------------------

// Cordon de sierras: perfil por hash a "distancia" z, entre alturas.
static void SierraCircuito(float z, float yBase, float yMin, float yMax, Color color, int semilla, int puntos)
{
    float x0 = -520.0f;
    float paso = 1040.0f / (float)puntos;

    rlBegin(RL_QUADS);

    for (int i = 0; i < puntos; i++)
    {
        float xa = x0 + paso * (float)i;
        float xb = xa + paso;
        float ha = yMin + (yMax - yMin) * (0.55f * HashCircuito(i, semilla) + 0.45f * HashCircuito(i / 3, semilla + 1));
        float hb = yMin + (yMax - yMin) * (0.55f * HashCircuito(i + 1, semilla) + 0.45f * HashCircuito((i + 1) / 3, semilla + 1));
        Color arriba = TinteCircuito(TonoCircuito(color, 1.12f), { xa, ha, z });
        Color abajo = TinteCircuito(TonoCircuito(color, 0.8f), { xa, yBase, z });

        VerticeCircuito({ xa, yBase, z }, abajo);
        VerticeCircuito({ xb, yBase, z }, abajo);
        VerticeCircuito({ xb, hb, z }, arriba);
        VerticeCircuito({ xa, ha, z }, arriba);
    }

    rlEnd();
}


static float CrestaSierraCircuito(float x, float yMin, float yMax, int semilla, int puntos)
{
    float u = (x + 520.0f) / (1040.0f / (float)puntos);
    int i = (int)std::floor(u);
    float f = u - (float)i;
    float ha = yMin + (yMax - yMin) * (0.55f * HashCircuito(i, semilla) + 0.45f * HashCircuito(i / 3, semilla + 1));
    float hb = yMin + (yMax - yMin) * (0.55f * HashCircuito(i + 1, semilla) + 0.45f * HashCircuito((i + 1) / 3, semilla + 1));
    return ha + (hb - ha) * f;
}


static void MolinoCircuito(Vector3 base, float alto, float t, int semilla)
{
    Color blanco = Color{ 150, 154, 180, 255 };
    Vector3 cubo = { base.x, base.y + alto, base.z };
    CilindroCircuito(base, cubo, alto * 0.035f, alto * 0.02f, 6, TinteCircuito(blanco, cubo));
    CuboNieblaCircuito({ cubo.x, cubo.y, cubo.z + alto * 0.02f }, { alto * 0.05f, alto * 0.05f, alto * 0.1f }, blanco);

    float giro = t * (0.7f + 0.3f * HashCircuito(semilla, 3)) + HashCircuito(semilla, 4) * 6.0f;
    Vector3 eje = { cubo.x, cubo.y, cubo.z + alto * 0.08f };

    for (int k = 0; k < 3; k++)
    {
        float a = giro + (float)k * 2.0944f;
        Vector3 punta = { eje.x + std::cos(a) * alto * 0.48f, eje.y + std::sin(a) * alto * 0.48f, eje.z };
        VigaCircuito(eje, punta, alto * 0.03f, alto * 0.012f, blanco);
    }
}


static void DibujarHorizonteCircuito(float t, int paso)
{
    // Sierras lejanas (claras por la niebla) y cercanas (mas oscuras).
    SierraCircuito(-330.0f, -140.0f, -50.0f, -12.0f, TonoCircuito(COLOR_SIERRA_CIRCUITO, 1.25f), 41, 46 / paso + 8);
    SierraCircuito(-235.0f, -140.0f, -54.0f, -18.0f, COLOR_SIERRA_CIRCUITO, 57, 60 / paso + 8);

    // Molinos sobre la cresta cercana: aspas que giran y baliza roja.
    for (int i = 0; i < 6; i++)
    {
        float x = -150.0f + 60.0f * (float)i + 20.0f * HashCircuito(i, 71);
        float y = CrestaSierraCircuito(x, -54.0f, -18.0f, 57, 60 / paso + 8) - 2.0f;
        MolinoCircuito({ x, y, -232.0f }, 26.0f, t, i);
    }

    // Ciudad en el valle: bloques con ventanas encendidas.
    for (int i = 0; i < 44; i += paso)
    {
        float x = -190.0f + 380.0f * ((float)i + HashCircuito(i, 81) * 0.6f) / 44.0f;
        float z = -135.0f - 50.0f * HashCircuito(i, 82);
        float anchoEdif = 6.0f + 9.0f * HashCircuito(i, 83);
        float alto = 14.0f + 30.0f * HashCircuito(i, 84) * HashCircuito(i, 85);

        // Mas altos en el centro del valle.
        alto *= 0.7f + 0.6f * (1.0f - std::fabs(x) / 200.0f);
        float yBase = -60.0f;
        float yTope = yBase + alto + 18.0f;

        Color fachada = MezclaCircuito(COLOR_EDIFICIO_CIRCUITO, Color{ 52, 46, 80, 255 }, HashCircuito(i, 86));
        CuboNieblaCircuito({ x, (yBase + yTope) * 0.5f, z }, { anchoEdif, yTope - yBase, anchoEdif * 0.8f }, fachada);

        // Ventanas: filas en la parte alta (la baja queda tapada).
        int columnas = (int)(anchoEdif / 1.6f);
        Color ventana = HashCircuito(i, 87) > 0.7f ? Color{ 170, 220, 255, 255 } : COLOR_VENTANA_CIRCUITO;
        ventana = TinteCircuito(ventana, { x, yTope, z });

        rlBegin(RL_QUADS);

        for (int fila = 0; fila < 9; fila++)
        {
            float y = yTope - 2.0f - 2.2f * (float)fila;

            for (int c = 0; c < columnas; c++)
            {
                if (HashCircuito(i * 31 + fila, c + 7) < 0.5f) continue;

                float xv = x - anchoEdif * 0.5f + 0.8f + 1.6f * (float)c;
                float zf = z + anchoEdif * 0.4f + 0.05f;
                QuadCircuito({ xv - 0.45f, y - 0.6f, zf }, { xv + 0.45f, y - 0.6f, zf }, { xv + 0.45f, y + 0.6f, zf }, { xv - 0.45f, y + 0.6f, zf }, ventana);
            }
        }

        rlEnd();
    }

    // Torre de television con baliza (hito del horizonte).
    Vector3 torre = { 46.0f, -60.0f, -170.0f };
    CilindroCircuito(torre, { torre.x, 30.0f, torre.z }, 2.4f, 1.0f, 8, TinteCircuito(Color{ 70, 70, 104, 255 }, { torre.x, 0.0f, torre.z }));
    CilindroCircuito({ torre.x, 14.0f, torre.z }, { torre.x, 19.0f, torre.z }, 5.0f, 4.2f, 10, TinteCircuito(Color{ 86, 84, 120, 255 }, { torre.x, 16.0f, torre.z }));
}


//--------------------------------------------------
// Terreno: meseta de pasto, caminos de servicio y arboles
//--------------------------------------------------

static void PinoCircuito(Vector3 base, float alto, int semilla)
{
    Color copa = MezclaCircuito(Color{ 26, 56, 46, 255 }, Color{ 36, 70, 54, 255 }, HashCircuito(semilla, 2));
    CilindroCircuito(base, { base.x, base.y + alto * 0.25f, base.z }, alto * 0.05f, alto * 0.04f, 5, TinteCircuito(Color{ 60, 44, 36, 255 }, base));

    for (int k = 0; k < 3; k++)
    {
        float y0 = base.y + alto * (0.2f + 0.24f * (float)k);
        float r = alto * (0.3f - 0.07f * (float)k);
        Vector3 a = { base.x, y0, base.z };
        CilindroCircuito(a, { base.x, y0 + alto * 0.42f, base.z }, r, 0.0f, 7, TinteCircuito(copa, a), false);
    }
}


static void DibujarTerrenoCircuito(int paso, bool decoracion)
{
    // Meseta: pasto en franjas anchas, con caida al valle detras.
    rlBegin(RL_QUADS);

    for (int i = 0; i < 18; i++)
    {
        float z0 = 40.0f - 5.0f * (float)i;
        float z1 = z0 - 5.0f;
        Color c = (i % 2 == 0) ? COLOR_PASTO_OSCURO_CIRCUITO : TonoCircuito(COLOR_PASTO_OSCURO_CIRCUITO, 1.06f);
        QuadCircuito({ -90.0f, -0.04f, z0 }, { 90.0f, -0.04f, z0 }, { 90.0f, -0.04f, z1 }, { -90.0f, -0.04f, z1 }, TinteCircuito(c, { 0.0f, 0.0f, (z0 + z1) * 0.5f }));
    }

    // Barranco de la meseta hacia el valle.
    Color roca = TinteCircuito(Color{ 52, 46, 62, 255 }, { 0.0f, -20.0f, -50.0f });
    QuadCircuito({ -90.0f, -0.04f, -50.0f }, { 90.0f, -0.04f, -50.0f }, { 90.0f, -60.0f, -62.0f }, { -90.0f, -60.0f, -62.0f }, roca);
    rlEnd();

    if (!decoracion) return;

    // Camino de servicio que rodea el autodromo.
    rlBegin(RL_QUADS);

    for (int s = 0; s < 72; s++)
    {
        float a0 = 6.2832f * (float)s / 72.0f;
        float a1 = 6.2832f * (float)(s + 1) / 72.0f;
        Color c = TonoCircuito(Color{ 58, 60, 70, 255 }, 0.94f + 0.12f * HashCircuito(s, 5));
        QuadCircuito(
            PuntoOvaloLlanoCircuito(-6.6f, a0, -0.02f),
            PuntoOvaloLlanoCircuito(-7.6f, a0, -0.02f),
            PuntoOvaloLlanoCircuito(-7.6f, a1, -0.02f),
            PuntoOvaloLlanoCircuito(-6.6f, a1, -0.02f),
            TinteCircuito(c, PuntoOvaloLlanoCircuito(-7.0f, a0, 0.0f))
        );
    }

    rlEnd();

    // Arboleda a los costados y detras de las tribunas.
    for (int i = 0; i < 40; i += paso)
    {
        float lado = (i % 2 == 0) ? -1.0f : 1.0f;
        float x = lado * (36.0f + 30.0f * HashCircuito(i, 91));
        float z = -46.0f + 70.0f * HashCircuito(i, 92);
        PinoCircuito({ x, 0.0f, z }, 5.0f + 4.0f * HashCircuito(i, 93), i);
    }

    for (int i = 0; i < 18; i += paso)
    {
        float x = -44.0f + 88.0f * ((float)i + HashCircuito(i, 94)) / 18.0f;
        if (std::fabs(x) < 26.0f) continue;
        PinoCircuito({ x, 0.0f, -38.0f - 8.0f * HashCircuito(i, 95) }, 6.0f + 3.0f * HashCircuito(i, 96), i + 50);
    }
}


//--------------------------------------------------
// Capa 1: la pista
//--------------------------------------------------

static bool EnCurvaCircuito(float angulo)
{
    float c = std::cos(angulo);
    return c * c > 0.5f;
}


static void DibujarPistaCircuito()
{
    const int S = SEGMENTOS_PISTA_CIRCUITO;

    rlBegin(RL_QUADS);

    for (int s = 0; s < S; s++)
    {
        float a0 = -PI / 2.0f + 6.2832f * (float)s / (float)S;
        float a1 = -PI / 2.0f + 6.2832f * (float)(s + 1) / (float)S;
        float am = (a0 + a1) * 0.5f;
        float curva = std::cos(am) * std::cos(am);

        // Asfalto: cada carril con variacion por tramo y la linea de goma
        // de la trazada, mas oscura donde se frena (curvas).
        for (int carril = 0; carril < 4; carril++)
        {
            float k = (float)carril;
            float variacion = 0.93f + 0.13f * HashCircuito(s, carril + 1);
            Color base = TonoCircuito(COLOR_ASFALTO_CIRCUITO, variacion);
            QuadCircuito(
                PuntoOvaloCircuito(k, a0), PuntoOvaloCircuito(k + 1.0f, a0),
                PuntoOvaloCircuito(k + 1.0f, a1), PuntoOvaloCircuito(k, a1),
                SombraCircuito(base, 1.0f, true)
            );

            Color goma = MezclaCircuito(base, COLOR_GOMA_CIRCUITO, 0.25f + 0.45f * curva);
            QuadCircuito(
                PuntoOvaloCircuito(k + 0.32f, a0, 0.006f), PuntoOvaloCircuito(k + 0.68f, a0, 0.006f),
                PuntoOvaloCircuito(k + 0.68f, a1, 0.006f), PuntoOvaloCircuito(k + 0.32f, a1, 0.006f),
                SombraCircuito(goma, 1.0f, true)
            );
        }

        // Divisorias de carril discontinuas y lineas de borde.
        Color linea = Color{ 214, 216, 222, 255 };

        if (s % 3 != 0)
        {
            for (int d = 1; d < 4; d++)
            {
                float k = (float)d;
                QuadCircuito(
                    PuntoOvaloCircuito(k - 0.03f, a0, 0.012f), PuntoOvaloCircuito(k + 0.03f, a0, 0.012f),
                    PuntoOvaloCircuito(k + 0.03f, a1, 0.012f), PuntoOvaloCircuito(k - 0.03f, a1, 0.012f),
                    linea
                );
            }
        }

        QuadCircuito(
            PuntoOvaloCircuito(0.06f, a0, 0.012f), PuntoOvaloCircuito(0.12f, a0, 0.012f),
            PuntoOvaloCircuito(0.12f, a1, 0.012f), PuntoOvaloCircuito(0.06f, a1, 0.012f),
            linea
        );
        QuadCircuito(
            PuntoOvaloCircuito(3.88f, a0, 0.012f), PuntoOvaloCircuito(3.94f, a0, 0.012f),
            PuntoOvaloCircuito(3.94f, a1, 0.012f), PuntoOvaloCircuito(3.88f, a1, 0.012f),
            linea
        );

        // Pianos rojos y blancos en las curvas (la zona de soltar); en
        // las rectas, banquina de hormigon por dentro y franja gris afuera.
        bool curvaFuerte = EnCurvaCircuito(am);
        Color piano = (s % 2 == 0) ? COLOR_PIANO_CIRCUITO : COLOR_BLANCO_CIRCUITO;
        Color exterior = curvaFuerte ? piano : Color{ 74, 78, 92, 255 };
        Color interior = curvaFuerte ? piano : TonoCircuito(COLOR_HORMIGON_CIRCUITO, 0.75f);

        QuadCircuito(
            PuntoOvaloCircuito(-0.5f, a0, 0.02f), PuntoOvaloCircuito(0.0f, a0, 0.02f),
            PuntoOvaloCircuito(0.0f, a1, 0.02f), PuntoOvaloCircuito(-0.5f, a1, 0.02f),
            SombraCircuito(exterior, 0.96f, true)
        );
        QuadCircuito(
            PuntoOvaloCircuito(4.0f, a0, 0.02f), PuntoOvaloCircuito(4.45f, a0, 0.02f),
            PuntoOvaloCircuito(4.45f, a1, 0.02f), PuntoOvaloCircuito(4.0f, a1, 0.02f),
            SombraCircuito(interior, 0.96f, true)
        );

        // Banquina de hormigon del infield.
        QuadCircuito(
            PuntoOvaloCircuito(4.45f, a0, 0.0f), PuntoOvaloCircuito(4.8f, a0, 0.0f),
            PuntoOvaloCircuito(4.8f, a1, 0.0f), PuntoOvaloCircuito(4.45f, a1, 0.0f),
            SombraCircuito(TonoCircuito(COLOR_HORMIGON_CIRCUITO, 0.62f + 0.06f * HashCircuito(s, 9)), 1.0f, true)
        );
    }

    rlEnd();

    // Lineas de frenado (entrada de curva, rojas) y de salida (verdes).
    const float entradas[2] = { -PI / 4.0f, 3.0f * PI / 4.0f };
    const float salidas[2] = { PI / 4.0f, 5.0f * PI / 4.0f };

    rlBegin(RL_QUADS);

    for (int i = 0; i < 2; i++)
    {
        for (int tramo = 0; tramo < 8; tramo++)
        {
            float k0 = 0.5f * (float)tramo;
            Color rojo = (tramo % 2 == 0) ? COLOR_PIANO_CIRCUITO : COLOR_BLANCO_CIRCUITO;
            float a = entradas[i];
            QuadCircuito(
                PuntoOvaloCircuito(k0, a, 0.018f), PuntoOvaloCircuito(k0 + 0.5f, a, 0.018f),
                PuntoOvaloCircuito(k0 + 0.5f, a + 0.03f, 0.018f), PuntoOvaloCircuito(k0, a + 0.03f, 0.018f),
                rojo
            );
        }

        float b = salidas[i];
        QuadCircuito(
            PuntoOvaloCircuito(0.0f, b, 0.018f), PuntoOvaloCircuito(4.0f, b, 0.018f),
            PuntoOvaloCircuito(4.0f, b + 0.016f, 0.018f), PuntoOvaloCircuito(0.0f, b + 0.016f, 0.018f),
            Color{ 90, 200, 120, 255 }
        );
    }

    // Linea de meta a cuadros sobre los cuatro carriles.
    for (int fila = 0; fila < 8; fila++)
    {
        for (int columna = 0; columna < 2; columna++)
        {
            float k0 = 0.5f * (float)fila;
            float x0 = -0.5f + 0.5f * (float)columna;
            Vector3 p0 = PuntoOvaloCircuito(k0, ANGULO_META_CIRCUITO, 0.02f);
            Vector3 p1 = PuntoOvaloCircuito(k0 + 0.5f, ANGULO_META_CIRCUITO, 0.02f);
            Color c = ((fila + columna) % 2 == 0) ? Color{ 24, 24, 30, 255 } : COLOR_BLANCO_CIRCUITO;
            QuadCircuito({ x0, p0.y, p0.z }, { x0 + 0.5f, p0.y, p0.z }, { x0 + 0.5f, p1.y, p1.z }, { x0, p1.y, p1.z }, c);
        }
    }

    rlEnd();
}


// Infield: pasto con franjas de corte y el logo del rayo pintado.
static void DibujarInfieldCircuito()
{
    const float K = 4.8f;
    float rx = RADIO_X_EXTERIOR - ANCHO_CARRIL * K;
    float rz = RADIO_Z_EXTERIOR - ANCHO_CARRIL * K;
    const int FRANJAS = 22;

    rlBegin(RL_QUADS);

    for (int i = 0; i < FRANJAS; i++)
    {
        float x0 = -rx + 2.0f * rx * (float)i / (float)FRANJAS;
        float x1 = -rx + 2.0f * rx * (float)(i + 1) / (float)FRANJAS;
        float h0 = rz * std::sqrt(Limitar01Circuito(1.0f - (x0 / rx) * (x0 / rx)));
        float h1 = rz * std::sqrt(Limitar01Circuito(1.0f - (x1 / rx) * (x1 / rx)));
        Color c = (i % 2 == 0) ? COLOR_PASTO_CIRCUITO : TonoCircuito(COLOR_PASTO_CIRCUITO, 0.86f);
        QuadCircuito({ x0, 0.0f, -h0 }, { x1, 0.0f, -h1 }, { x1, 0.0f, h1 }, { x0, 0.0f, h0 }, SombraCircuito(c, 1.0f, true));
    }

    rlEnd();

    // Rayo pintado (acento apagado) dentro de un circulo blanco.
    rlBegin(RL_TRIANGLES);
    Color pintura = MezclaCircuito(COLOR_VOLTAJE_CIRCUITO, COLOR_PASTO_CIRCUITO, 0.35f);
    Vector3 r[6] = {
        { 0.55f, 0.012f, -1.75f }, { -0.95f, 0.012f, 0.25f }, { 0.05f, 0.012f, 0.15f },
        { -0.55f, 0.012f, 1.75f }, { 0.95f, 0.012f, -0.25f }, { -0.05f, 0.012f, -0.15f }
    };
    VerticeCircuito(r[0], pintura); VerticeCircuito(r[1], pintura); VerticeCircuito(r[2], pintura);
    VerticeCircuito(r[3], pintura); VerticeCircuito(r[4], pintura); VerticeCircuito(r[5], pintura);
    VerticeCircuito(r[1], pintura); VerticeCircuito(r[4], pintura); VerticeCircuito(r[2], pintura);
    VerticeCircuito(r[4], pintura); VerticeCircuito(r[1], pintura); VerticeCircuito(r[5], pintura);
    rlEnd();

    rlBegin(RL_QUADS);

    for (int k = 0; k < 40; k++)
    {
        float a0 = 6.2832f * (float)k / 40.0f;
        float a1 = 6.2832f * (float)(k + 1) / 40.0f;
        Color c = Color{ 210, 214, 220, 255 };
        QuadCircuito(
            { std::cos(a0) * 2.25f, 0.01f, std::sin(a0) * 2.05f }, { std::cos(a0) * 2.05f, 0.01f, std::sin(a0) * 1.87f },
            { std::cos(a1) * 2.05f, 0.01f, std::sin(a1) * 1.87f }, { std::cos(a1) * 2.25f, 0.01f, std::sin(a1) * 2.05f },
            c
        );
    }

    rlEnd();
}


//--------------------------------------------------
// Capa 2: muro, publicidad, chevrones y alambrado
//--------------------------------------------------

static const Color PUBLICIDAD_CIRCUITO[4][2] =
{
    { { 28, 40, 84, 255 }, { 86, 228, 255, 255 } },
    { { 226, 228, 234, 255 }, { 210, 60, 70, 255 } },
    { { 34, 34, 40, 255 }, { 255, 190, 60, 255 } },
    { { 70, 40, 104, 255 }, { 236, 236, 246, 255 } }
};


static void DibujarMuroCircuito(bool decoracion)
{
    const int S = SEGMENTOS_PISTA_CIRCUITO;
    const float KI = -0.5f;
    const float KE = -0.72f;
    const float ALTO = 0.9f;

    rlBegin(RL_QUADS);

    for (int s = 0; s < S; s++)
    {
        float a0 = -PI / 2.0f + 6.2832f * (float)s / (float)S;
        float a1 = -PI / 2.0f + 6.2832f * (float)(s + 1) / (float)S;
        float am = (a0 + a1) * 0.5f;

        Vector3 i0 = PuntoOvaloCircuito(KI, a0, 0.0f);
        Vector3 i1 = PuntoOvaloCircuito(KI, a1, 0.0f);
        Vector3 e0 = PuntoOvaloCircuito(KE, a0, 0.0f);
        Vector3 e1 = PuntoOvaloCircuito(KE, a1, 0.0f);
        Vector3 i0a = { i0.x, i0.y + ALTO, i0.z };
        Vector3 i1a = { i1.x, i1.y + ALTO, i1.z };
        Vector3 e0a = { e0.x, e0.y + ALTO, e0.z };
        Vector3 e1a = { e1.x, e1.y + ALTO, e1.z };

        // Cara interior: publicidad en las rectas, chevrones en las curvas.
        Vector3 n = NormalizarCircuito({ -std::cos(am), 0.0f, -std::sin(am) });
        float luz = LuzNormalCircuito(n);

        if (EnCurvaCircuito(am))
        {
            Color fondo = (s % 2 == 0) ? COLOR_AMBAR_CIRCUITO : Color{ 30, 30, 36, 255 };
            QuadCircuito(i0, i1, i1a, i0a, SombraCircuito(fondo, luz, false));
        }
        else
        {
            int marca = (s / 5) % 4;
            Color fondo = PUBLICIDAD_CIRCUITO[marca][0];
            Color logo = PUBLICIDAD_CIRCUITO[marca][1];
            Vector3 m0 = { i0.x, i0.y + ALTO * 0.3f, i0.z };
            Vector3 m1 = { i1.x, i1.y + ALTO * 0.3f, i1.z };
            Vector3 m0a = { i0.x, i0.y + ALTO * 0.7f, i0.z };
            Vector3 m1a = { i1.x, i1.y + ALTO * 0.7f, i1.z };
            bool conLogo = (s % 5) >= 1 && (s % 5) <= 3;

            QuadCircuito(i0, i1, m1, m0, SombraCircuito(fondo, luz, false));
            QuadCircuito(m0, m1, m1a, m0a, SombraCircuito(conLogo ? logo : fondo, luz, false));
            QuadCircuito(m0a, m1a, i1a, i0a, SombraCircuito(fondo, luz, false));
        }

        // Tapa y cara exterior de hormigon.
        Color tapa = TonoCircuito(COLOR_HORMIGON_CIRCUITO, 0.95f + 0.08f * HashCircuito(s, 21));
        QuadCircuito(i0a, i1a, e1a, e0a, SombraCircuito(tapa, 1.0f, true));
        QuadCircuito(e0, e1, e1a, e0a, SombraCircuito(TonoCircuito(COLOR_HORMIGON_CIRCUITO, 0.8f), 0.6f, false));

        // Talud de pasto por fuera del peralte (solo se nota en curvas).
        Vector3 t0 = PuntoOvaloLlanoCircuito(-2.6f, a0, -0.03f);
        Vector3 t1 = PuntoOvaloLlanoCircuito(-2.6f, a1, -0.03f);
        QuadCircuito(e0, e1, t1, t0, SombraCircuito(COLOR_PASTO_OSCURO_CIRCUITO, 0.92f, true));
    }

    rlEnd();

    if (!decoracion) return;

    // Alambrado sobre el muro (no en la recta cercana: taparia los autos).
    rlBegin(RL_LINES);

    for (int s = 0; s < S; s++)
    {
        float a0 = -PI / 2.0f + 6.2832f * (float)s / (float)S;
        float a1 = -PI / 2.0f + 6.2832f * (float)(s + 1) / (float)S;
        Vector3 p0 = PuntoOvaloCircuito(-0.62f, a0, ALTO);
        Vector3 p1 = PuntoOvaloCircuito(-0.62f, a1, ALTO);

        if (p0.z > 1.5f || p1.z > 1.5f) continue;

        Color alambre = TinteCircuito(Color{ 150, 156, 176, 150 }, p0);

        for (int h = 1; h <= 3; h++)
        {
            float y = 0.75f * (float)h;
            VerticeCircuito({ p0.x, p0.y + y, p0.z }, alambre);
            VerticeCircuito({ p1.x, p1.y + y, p1.z }, alambre);
        }

        // Malla en diagonal.
        VerticeCircuito(p0, alambre);
        VerticeCircuito({ p1.x, p1.y + 2.25f, p1.z }, alambre);
        VerticeCircuito({ p0.x, p0.y + 2.25f, p0.z }, alambre);
        VerticeCircuito(p1, alambre);
    }

    rlEnd();

    for (int s = 0; s < S; s += 3)
    {
        float a = -PI / 2.0f + 6.2832f * (float)s / (float)S;
        Vector3 p = PuntoOvaloCircuito(-0.62f, a, ALTO);
        if (p.z > 1.5f) continue;
        VigaCircuito(p, { p.x, p.y + 2.35f, p.z }, 0.07f, 0.07f, COLOR_ACERO_CIRCUITO);
    }
}


//--------------------------------------------------
// Boxes: calle de boxes y garajes de los equipos
//--------------------------------------------------

static const Color EQUIPOS_CIRCUITO[8] =
{
    { 40, 190, 180, 255 },
    { 250, 140, 40, 255 },
    { 150, 90, 220, 255 },
    { 220, 224, 232, 255 },
    { 230, 70, 160, 255 },
    { 170, 220, 60, 255 },
    { 40, 70, 150, 255 },
    { 160, 166, 180, 255 }
};


static const int CANTIDAD_BOXES_CIRCUITO = 8;
static const float K_CALLE_BOXES_CIRCUITO = -0.72f;
static const float K_FRENTE_BOXES_CIRCUITO = -2.45f;
static const float K_FONDO_BOXES_CIRCUITO = -4.3f;
static const float ALTO_BOXES_CIRCUITO = 2.5f;


static float AnguloBoxCircuito(int i)
{
    float paso = 2.0f * MEDIO_ARCO_BOXES_CIRCUITO / (float)CANTIDAD_BOXES_CIRCUITO;
    return ANGULO_META_CIRCUITO - MEDIO_ARCO_BOXES_CIRCUITO + paso * ((float)i + 0.5f);
}


static void NeumaticosCircuito(Vector3 base, int pila, Color franja)
{
    for (int k = 0; k < pila; k++)
    {
        Vector3 a = { base.x, base.y + 0.17f * (float)k, base.z };
        CilindroCircuito(a, { a.x, a.y + 0.16f, a.z }, 0.26f, 0.26f, 10, TinteCircuito(Color{ 30, 30, 34, 255 }, a));
        CilindroCircuito({ a.x, a.y + 0.155f, a.z }, { a.x, a.y + 0.165f, a.z }, 0.14f, 0.14f, 8, TinteCircuito(franja, a));
    }
}


static void DibujarBoxesCircuito(float t, bool decoracion)
{
    // Calle de boxes: asfalto mas claro con linea discontinua.
    const int S = 40;
    float a0 = ANGULO_META_CIRCUITO - MEDIO_ARCO_BOXES_CIRCUITO - 0.12f;
    float a1 = ANGULO_META_CIRCUITO + MEDIO_ARCO_BOXES_CIRCUITO + 0.12f;

    rlBegin(RL_QUADS);

    for (int s = 0; s < S; s++)
    {
        float b0 = a0 + (a1 - a0) * (float)s / (float)S;
        float b1 = a0 + (a1 - a0) * (float)(s + 1) / (float)S;
        Color c = TonoCircuito(Color{ 74, 76, 88, 255 }, 0.94f + 0.1f * HashCircuito(s, 31));
        QuadCircuito(
            PuntoOvaloLlanoCircuito(K_CALLE_BOXES_CIRCUITO, b0, -0.01f), PuntoOvaloLlanoCircuito(K_FRENTE_BOXES_CIRCUITO, b0, -0.01f),
            PuntoOvaloLlanoCircuito(K_FRENTE_BOXES_CIRCUITO, b1, -0.01f), PuntoOvaloLlanoCircuito(K_CALLE_BOXES_CIRCUITO, b1, -0.01f),
            SombraCircuito(c, 1.0f, true)
        );

        if (s % 2 == 0)
        {
            QuadCircuito(
                PuntoOvaloLlanoCircuito(-1.55f, b0, 0.0f), PuntoOvaloLlanoCircuito(-1.6f, b0, 0.0f),
                PuntoOvaloLlanoCircuito(-1.6f, b1, 0.0f), PuntoOvaloLlanoCircuito(-1.55f, b1, 0.0f),
                Color{ 210, 212, 220, 255 }
            );
        }
    }

    rlEnd();

    for (int i = 0; i < CANTIDAD_BOXES_CIRCUITO; i++)
    {
        float a = AnguloBoxCircuito(i);
        float paso = 2.0f * MEDIO_ARCO_BOXES_CIRCUITO / (float)CANTIDAD_BOXES_CIRCUITO;
        MarcoCircuito m = MarcoOvaloCircuito(K_FRENTE_BOXES_CIRCUITO, a, 0.0f);
        Vector3 pa = PuntoOvaloLlanoCircuito(K_FRENTE_BOXES_CIRCUITO, a - paso * 0.5f, 0.0f);
        Vector3 pb = PuntoOvaloLlanoCircuito(K_FRENTE_BOXES_CIRCUITO, a + paso * 0.5f, 0.0f);
        float largo = std::sqrt((pb.x - pa.x) * (pb.x - pa.x) + (pb.z - pa.z) * (pb.z - pa.z));
        float fondo = ANCHO_CARRIL * (K_FRENTE_BOXES_CIRCUITO - K_FONDO_BOXES_CIRCUITO);
        Color equipo = EQUIPOS_CIRCUITO[i];
        Vector3 arriba = { 0.0f, 1.0f, 0.0f };

        // Piso, fondo iluminado, tabiques y techo: el frente queda abierto.
        CajaMarcoCircuito(m, fondo * 0.5f, -0.02f, largo, 0.04f, fondo, Color{ 150, 152, 160, 255 });
        CajaMarcoCircuito(m, fondo - 0.1f, 0.0f, largo, ALTO_BOXES_CIRCUITO, 0.2f, MezclaCircuito(equipo, WHITE, 0.45f));
        CajaMarcoCircuito(m, fondo * 0.5f, ALTO_BOXES_CIRCUITO, largo + 0.1f, 0.26f, fondo + 0.2f, COLOR_ACERO_OSCURO_CIRCUITO);

        Vector3 lado = SumaCircuito(m.punto, EscalaCircuito(m.afuera, fondo * 0.5f));
        Vector3 tabique = SumaCircuito(lado, EscalaCircuito(m.adelante, -largo * 0.5f));
        tabique.y = ALTO_BOXES_CIRCUITO * 0.5f;
        CajaNieblaCircuito(tabique, m.adelante, arriba, m.afuera, { 0.16f, ALTO_BOXES_CIRCUITO, fondo }, Color{ 200, 202, 210, 255 });

        // Portón enrollable a medio subir y cartel del equipo.
        CajaMarcoCircuito(m, 0.08f, ALTO_BOXES_CIRCUITO - 0.55f, largo - 0.1f, 0.55f, 0.1f, Color{ 120, 126, 140, 255 });
        CajaMarcoCircuito(m, -0.05f, ALTO_BOXES_CIRCUITO + 0.26f, largo * 0.8f, 0.5f, 0.12f, equipo);

        if (!decoracion) continue;

        // Dentro: neumaticos, carro de herramientas y un auto de repuesto.
        Vector3 fondoBox = SumaCircuito(m.punto, EscalaCircuito(m.afuera, fondo * 0.72f));
        NeumaticosCircuito(SumaCircuito(fondoBox, EscalaCircuito(m.adelante, -largo * 0.32f)), 3 + i % 2, equipo);
        Vector3 carro = SumaCircuito(fondoBox, EscalaCircuito(m.adelante, largo * 0.3f));
        carro.y = 0.45f;
        CajaNieblaCircuito(carro, m.adelante, arriba, m.afuera, { 0.6f, 0.9f, 0.4f }, Color{ 196, 52, 52, 255 });
        Vector3 auxiliar = SumaCircuito(m.punto, EscalaCircuito(m.afuera, fondo * 0.38f));
        auxiliar.y = 0.22f;
        CajaNieblaCircuito(auxiliar, m.adelante, arriba, m.afuera, { 1.6f, 0.26f, 0.7f }, TonoCircuito(equipo, 0.7f));
        auxiliar.y = 0.42f;
        CajaNieblaCircuito(auxiliar, m.adelante, arriba, m.afuera, { 0.5f, 0.2f, 0.36f }, COLOR_CARBONO_CIRCUITO);

        // Terraza VIP sobre el techo: baranda de vidrio y gente de pie.
        Vector3 baranda = SumaCircuito(m.punto, EscalaCircuito(m.afuera, -0.05f));
        baranda.y = ALTO_BOXES_CIRCUITO + 0.13f + 0.35f;
        CajaNieblaCircuito(baranda, m.adelante, arriba, m.afuera, { largo, 0.04f, 0.06f }, COLOR_ACERO_CIRCUITO);

        for (int p = 0; p < 3; p++)
        {
            float h = HashCircuito(i * 7 + p, 41);
            Vector3 pie = SumaCircuito(m.punto, EscalaCircuito(m.afuera, 0.5f + 1.2f * h));
            pie = SumaCircuito(pie, EscalaCircuito(m.adelante, largo * (-0.35f + 0.35f * (float)p)));
            float salto = 0.03f * std::sin(t * 4.0f + h * 9.0f);
            pie.y = ALTO_BOXES_CIRCUITO + 0.13f + salto;
            Color ropa = MezclaCircuito(PALETA_FIESTA_CIRCUITO[(i + p) % 6], Color{ 60, 60, 80, 255 }, 0.4f);
            CuboNieblaCircuito({ pie.x, pie.y + 0.32f, pie.z }, { 0.26f, 0.64f, 0.2f }, ropa);
            CuboNieblaCircuito({ pie.x, pie.y + 0.76f, pie.z }, { 0.2f, 0.22f, 0.2f }, Color{ 224, 178, 140, 255 });
        }
    }
}


//--------------------------------------------------
// Capa 3: tribunas con publico
//--------------------------------------------------

static const Color ROPA_PUBLICO_CIRCUITO[9] =
{
    { 206, 74, 74, 255 }, { 70, 116, 206, 255 }, { 232, 196, 70, 255 },
    { 86, 176, 104, 255 }, { 226, 226, 232, 255 }, { 142, 92, 192, 255 },
    { 232, 132, 56, 255 }, { 46, 46, 56, 255 }, { 80, 196, 206, 255 }
};

static const Color PIEL_PUBLICO_CIRCUITO[4] =
{
    { 236, 192, 152, 255 }, { 198, 142, 102, 255 }, { 142, 96, 64, 255 }, { 246, 216, 182, 255 }
};


// Tribuna escalonada sobre un arco del ovalo: filas de gradas con gente
// sentada que se balancea (mas fuerte con "animo", al final).
static void TribunaCircuito(
    float angulo0,
    float angulo1,
    float kFrente,
    int filas,
    float yBase,
    float t,
    float animo,
    int semilla,
    int paso,
    bool techo
)
{
    const float FONDO_FILA = 0.52f;   // en unidades de k
    const float ALTO_FILA = 0.46f;
    int tramos = (int)(std::fabs(angulo1 - angulo0) * 22.0f) + 4;

    // Gradas: huella y contrahuella por fila, con estructura debajo.
    rlBegin(RL_QUADS);

    for (int f = 0; f < filas; f++)
    {
        float k0 = kFrente - FONDO_FILA * (float)f;
        float k1 = k0 - FONDO_FILA;
        float y = yBase + ALTO_FILA * (float)(f + 1);
        float yAnterior = yBase + ALTO_FILA * (float)f;

        for (int s = 0; s < tramos; s++)
        {
            float a0 = angulo0 + (angulo1 - angulo0) * (float)s / (float)tramos;
            float a1 = angulo0 + (angulo1 - angulo0) * (float)(s + 1) / (float)tramos;
            Vector3 p = PuntoOvaloLlanoCircuito(k0, a0, y);
            Color huella = TinteCircuito(TonoCircuito(Color{ 112, 118, 136, 255 }, 0.94f + 0.08f * HashCircuito(s, f + semilla)), p);
            Color frente = TinteCircuito(Color{ 70, 76, 96, 255 }, p);

            QuadCircuito(PuntoOvaloLlanoCircuito(k0, a0, y), PuntoOvaloLlanoCircuito(k1, a0, y), PuntoOvaloLlanoCircuito(k1, a1, y), PuntoOvaloLlanoCircuito(k0, a1, y), SombraCircuito(huella, 1.0f, true));
            QuadCircuito(PuntoOvaloLlanoCircuito(k0, a0, yAnterior), PuntoOvaloLlanoCircuito(k0, a1, yAnterior), PuntoOvaloLlanoCircuito(k0, a1, y), PuntoOvaloLlanoCircuito(k0, a0, y), SombraCircuito(frente, 0.8f, false));
        }
    }

    // Frente bajo la primera fila y muro trasero.
    float kAtras = kFrente - FONDO_FILA * (float)filas;
    float yTope = yBase + ALTO_FILA * (float)filas;

    for (int s = 0; s < tramos; s++)
    {
        float a0 = angulo0 + (angulo1 - angulo0) * (float)s / (float)tramos;
        float a1 = angulo0 + (angulo1 - angulo0) * (float)(s + 1) / (float)tramos;
        Vector3 p = PuntoOvaloLlanoCircuito(kFrente, a0, 0.0f);
        Color cemento = TinteCircuito(Color{ 84, 88, 106, 255 }, p);
        QuadCircuito(PuntoOvaloLlanoCircuito(kFrente, a0, 0.0f), PuntoOvaloLlanoCircuito(kFrente, a1, 0.0f), PuntoOvaloLlanoCircuito(kFrente, a1, yBase), PuntoOvaloLlanoCircuito(kFrente, a0, yBase), SombraCircuito(cemento, 0.75f, false));
        QuadCircuito(PuntoOvaloLlanoCircuito(kAtras, a0, yTope), PuntoOvaloLlanoCircuito(kAtras, a1, yTope), PuntoOvaloLlanoCircuito(kAtras, a1, yTope + 1.3f), PuntoOvaloLlanoCircuito(kAtras, a0, yTope + 1.3f), SombraCircuito(TonoCircuito(cemento, 0.9f), 0.75f, false));
        QuadCircuito(PuntoOvaloLlanoCircuito(kAtras, a0, yTope + 1.3f), PuntoOvaloLlanoCircuito(kAtras - 0.25f, a0, yTope + 1.3f), PuntoOvaloLlanoCircuito(kAtras - 0.25f, a1, yTope + 1.3f), PuntoOvaloLlanoCircuito(kAtras, a1, yTope + 1.3f), SombraCircuito(cemento, 1.0f, true));
    }

    rlEnd();

    // Publico: cuerpo y cabeza; algunos asientos vacios.
    for (int f = 0; f < filas; f++)
    {
        float k = kFrente - FONDO_FILA * ((float)f + 0.55f);
        float y = yBase + ALTO_FILA * (float)(f + 1);
        float rx = RADIO_X_EXTERIOR - ANCHO_CARRIL * k;
        float rz = RADIO_Z_EXTERIOR - ANCHO_CARRIL * k;
        float largoArco = std::fabs(angulo1 - angulo0) * 0.5f * (rx + rz);
        int asientos = (int)(largoArco / 0.56f);

        for (int s = 0; s < asientos; s += paso)
        {
            int id = semilla * 1000 + f * 97 + s;
            float h = HashCircuito(id, 1);
            float a = angulo0 + (angulo1 - angulo0) * ((float)s + 0.5f) / (float)asientos;
            Vector3 p = PuntoOvaloLlanoCircuito(k, a, y);

            if (h < 0.1f)
            {
                CuboNieblaCircuito({ p.x, p.y + 0.12f, p.z }, { 0.36f, 0.24f, 0.3f }, Color{ 50, 80, 150, 255 });
                continue;
            }

            float fase = h * 30.0f;
            float salto = (0.025f + 0.12f * animo) * std::fabs(std::sin(t * (3.0f + 3.0f * animo) + fase));
            Color ropa = MezclaCircuito(ROPA_PUBLICO_CIRCUITO[(int)(HashCircuito(id, 2) * 8.99f)], Color{ 78, 82, 108, 255 }, 0.3f);
            Color piel = PIEL_PUBLICO_CIRCUITO[(int)(HashCircuito(id, 3) * 3.99f)];

            CuboNieblaCircuito({ p.x, p.y + 0.26f + salto, p.z }, { 0.34f, 0.46f, 0.28f }, ropa);
            CuboNieblaCircuito({ p.x, p.y + 0.6f + salto, p.z }, { 0.22f, 0.22f, 0.22f }, piel);

            // Brazos arriba en parte del publico cuando hay festejo.
            if (animo > 0.3f && HashCircuito(id, 4) < 0.45f)
            {
                CuboNieblaCircuito({ p.x - 0.2f, p.y + 0.72f + salto, p.z }, { 0.08f, 0.36f, 0.08f }, ropa);
                CuboNieblaCircuito({ p.x + 0.2f, p.y + 0.72f + salto, p.z }, { 0.08f, 0.36f, 0.08f }, ropa);
            }
        }
    }

    // Banderas sobre el muro trasero, que ondean.
    int banderas = tramos / 3;

    for (int b = 0; b <= banderas; b++)
    {
        float a = angulo0 + (angulo1 - angulo0) * (float)b / (float)(banderas > 0 ? banderas : 1);
        Vector3 base = PuntoOvaloLlanoCircuito(kAtras - 0.12f, a, yTope + 1.3f);
        Vector3 punta = { base.x, base.y + 2.4f, base.z };
        VigaCircuito(base, punta, 0.06f, 0.06f, COLOR_ACERO_CIRCUITO);

        Color tela = TinteCircuito(PALETA_FIESTA_CIRCUITO[(b + semilla) % 6], punta);
        rlBegin(RL_QUADS);

        for (int s = 0; s < 4; s++)
        {
            float u0 = 0.25f * (float)s;
            float u1 = 0.25f * (float)(s + 1);
            float o0 = 0.18f * u0 * std::sin(t * 5.0f + u0 * 5.0f + (float)b);
            float o1 = 0.18f * u1 * std::sin(t * 5.0f + u1 * 5.0f + (float)b);
            Color c = SombraCircuito(tela, 0.82f + 0.18f * std::sin(t * 5.0f + u0 * 5.0f + (float)b), false);
            QuadCircuito(
                { punta.x + u0 * 1.1f, punta.y - 0.7f + o0, punta.z + o0 },
                { punta.x + u1 * 1.1f, punta.y - 0.7f + o1, punta.z + o1 },
                { punta.x + u1 * 1.1f, punta.y + o1, punta.z + o1 },
                { punta.x + u0 * 1.1f, punta.y + o0, punta.z + o0 },
                c
            );
        }

        rlEnd();
    }

    if (!techo) return;

    // Visera sobre las filas de atras, con columnas.
    rlBegin(RL_QUADS);
    float kVisera = kAtras + FONDO_FILA * 2.6f;

    for (int s = 0; s < tramos; s++)
    {
        float a0 = angulo0 + (angulo1 - angulo0) * (float)s / (float)tramos;
        float a1 = angulo0 + (angulo1 - angulo0) * (float)(s + 1) / (float)tramos;
        float y = yTope + 2.9f;
        Vector3 p = PuntoOvaloLlanoCircuito(kAtras, a0, y);
        Color chapa = TinteCircuito(Color{ 150, 156, 172, 255 }, p);
        QuadCircuito(PuntoOvaloLlanoCircuito(kVisera, a0, y + 0.4f), PuntoOvaloLlanoCircuito(kAtras - 0.3f, a0, y), PuntoOvaloLlanoCircuito(kAtras - 0.3f, a1, y), PuntoOvaloLlanoCircuito(kVisera, a1, y + 0.4f), SombraCircuito(chapa, 1.0f, true));
        QuadCircuito(PuntoOvaloLlanoCircuito(kVisera, a0, y + 0.15f), PuntoOvaloLlanoCircuito(kVisera, a1, y + 0.15f), PuntoOvaloLlanoCircuito(kVisera, a1, y + 0.4f), PuntoOvaloLlanoCircuito(kVisera, a0, y + 0.4f), SombraCircuito(TonoCircuito(chapa, 0.7f), 0.78f, false));
    }

    rlEnd();

    for (int s = 0; s <= tramos; s += 4)
    {
        float a = angulo0 + (angulo1 - angulo0) * (float)s / (float)tramos;
        Vector3 base = PuntoOvaloLlanoCircuito(kAtras - 0.15f, a, yTope);
        VigaCircuito(base, { base.x, yTope + 3.0f, base.z }, 0.14f, 0.14f, COLOR_ACERO_OSCURO_CIRCUITO);
    }
}


//--------------------------------------------------
// Anillo cercano: utileria del infield y del borde de la pista
//--------------------------------------------------

static void BanderaCircuito(Vector3 punta, Color color, float t, float fase, float largo, float alto)
{
    Color tela = TinteCircuito(color, punta);
    rlBegin(RL_QUADS);

    for (int s = 0; s < 4; s++)
    {
        float u0 = 0.25f * (float)s;
        float u1 = 0.25f * (float)(s + 1);
        float o0 = 0.2f * u0 * std::sin(t * 5.5f + u0 * 5.0f + fase);
        float o1 = 0.2f * u1 * std::sin(t * 5.5f + u1 * 5.0f + fase);
        Color c = SombraCircuito(tela, 0.8f + 0.2f * std::sin(t * 5.5f + u0 * 5.0f + fase), false);
        QuadCircuito(
            { punta.x + u0 * largo, punta.y - alto + o0, punta.z + o0 },
            { punta.x + u1 * largo, punta.y - alto + o1, punta.z + o1 },
            { punta.x + u1 * largo, punta.y + o1, punta.z + o1 },
            { punta.x + u0 * largo, punta.y + o0, punta.z + o0 },
            c
        );
    }

    rlEnd();
}


static void ConoCircuito(Vector3 base)
{
    CuboNieblaCircuito({ base.x, base.y + 0.02f, base.z }, { 0.34f, 0.04f, 0.34f }, Color{ 230, 110, 40, 255 });
    CilindroCircuito(base, { base.x, base.y + 0.46f, base.z }, 0.14f, 0.02f, 8, TinteCircuito(Color{ 240, 120, 40, 255 }, base), false);
    CilindroCircuito({ base.x, base.y + 0.18f, base.z }, { base.x, base.y + 0.26f, base.z }, 0.1f, 0.085f, 8, TinteCircuito(COLOR_BLANCO_CIRCUITO, base), false);
}


// Comisario en su puesto con bandera (amarilla si hay un trompo en pista).
static void PuestoComisarioCircuito(Vector3 base, float t, bool alerta, float giro)
{
    Vector3 adelante = { std::cos(giro), 0.0f, std::sin(giro) };
    Vector3 lado = { -adelante.z, 0.0f, adelante.x };
    Vector3 arriba = { 0.0f, 1.0f, 0.0f };

    CajaNieblaCircuito({ base.x, base.y + 0.55f, base.z }, adelante, arriba, lado, { 1.0f, 1.1f, 1.0f }, Color{ 236, 120, 40, 255 });
    CajaNieblaCircuito({ base.x, base.y + 1.18f, base.z }, adelante, arriba, lado, { 1.2f, 0.12f, 1.2f }, COLOR_BLANCO_CIRCUITO);

    Vector3 pie = SumaCircuito(base, EscalaCircuito(adelante, 0.9f));
    CuboNieblaCircuito({ pie.x, pie.y + 0.4f, pie.z }, { 0.28f, 0.8f, 0.22f }, Color{ 236, 120, 40, 255 });
    CuboNieblaCircuito({ pie.x, pie.y + 0.92f, pie.z }, { 0.22f, 0.22f, 0.22f }, Color{ 210, 166, 128, 255 });

    float onda = std::sin(t * (alerta ? 9.0f : 3.0f));
    Vector3 mano = { pie.x, pie.y + 1.0f, pie.z };
    Vector3 punta = { mano.x + 0.5f * onda, mano.y + 0.9f, mano.z };
    VigaCircuito(mano, punta, 0.04f, 0.04f, Color{ 80, 80, 90, 255 });
    BanderaCircuito(punta, alerta ? Color{ 255, 220, 40, 255 } : Color{ 70, 190, 90, 255 }, t * (alerta ? 2.0f : 1.0f), 0.0f, 0.6f, 0.42f);
}


static void CarpaCircuito(Vector3 base, float lado, Color franja)
{
    float h = lado * 0.5f;
    float alto = 1.0f;

    for (int i = 0; i < 4; i++)
    {
        float sx = (i % 2 == 0) ? -1.0f : 1.0f;
        float sz = (i < 2) ? -1.0f : 1.0f;
        VigaCircuito({ base.x + sx * h, 0.0f, base.z + sz * h }, { base.x + sx * h, alto, base.z + sz * h }, 0.05f, 0.05f, COLOR_ACERO_CIRCUITO);
    }

    Vector3 cima = { base.x, alto + 0.7f, base.z };
    Vector3 e[4] = {
        { base.x - h, alto, base.z - h }, { base.x + h, alto, base.z - h },
        { base.x + h, alto, base.z + h }, { base.x - h, alto, base.z + h }
    };

    rlBegin(RL_TRIANGLES);

    for (int i = 0; i < 4; i++)
    {
        Vector3 a = e[i];
        Vector3 b = e[(i + 1) % 4];
        Vector3 n = NormalizarCircuito(CruzCircuito(RestaCircuito(b, a), RestaCircuito(cima, a)));
        if (n.y < 0.0f) n = EscalaCircuito(n, -1.0f);
        Color c = SombraCircuito(TinteCircuito(COLOR_BLANCO_CIRCUITO, cima), LuzNormalCircuito(n), true);
        VerticeCircuito(a, c);
        VerticeCircuito(b, c);
        VerticeCircuito(cima, c);
    }

    rlEnd();

    // Faldon con la franja del color de la organizacion.
    rlBegin(RL_QUADS);

    for (int i = 0; i < 4; i++)
    {
        Vector3 a = e[i];
        Vector3 b = e[(i + 1) % 4];
        Color c = SombraCircuito(TinteCircuito(franja, cima), 0.8f, false);
        QuadCircuito({ a.x, a.y - 0.22f, a.z }, { b.x, b.y - 0.22f, b.z }, b, a, c);
    }

    rlEnd();
}


// Cargador de autos electricos con pantalla cian y cable.
static void CargadorCircuito(Vector3 base, float t, int indice)
{
    CuboNieblaCircuito({ base.x, base.y + 0.03f, base.z }, { 0.7f, 0.06f, 0.5f }, Color{ 120, 124, 136, 255 });
    CuboNieblaCircuito({ base.x, base.y + 0.66f, base.z }, { 0.38f, 1.2f, 0.28f }, Color{ 226, 230, 236, 255 });
    CuboNieblaCircuito({ base.x, base.y + 1.3f, base.z }, { 0.46f, 0.1f, 0.34f }, COLOR_ACERO_OSCURO_CIRCUITO);

    float pulso = 0.6f + 0.4f * std::sin(t * 2.5f + (float)indice * 1.3f);
    Color pantalla = MezclaCircuito(Color{ 20, 60, 80, 255 }, COLOR_VOLTAJE_CIRCUITO, pulso);
    CuboCircuito({ base.x, base.y + 0.92f, base.z + 0.145f }, { 0.26f, 0.3f, 0.02f }, pantalla);

    // Cable en curva hacia el suelo.
    Vector3 anterior = { base.x + 0.19f, base.y + 0.8f, base.z };

    for (int k = 1; k <= 6; k++)
    {
        float u = (float)k / 6.0f;
        Vector3 p = { base.x + 0.19f + 0.5f * u, base.y + 0.8f - 0.78f * std::sin(u * 1.57f), base.z + 0.25f * u };
        VigaCircuito(anterior, p, 0.04f, 0.04f, Color{ 30, 30, 36, 255 });
        anterior = p;
    }
}


static void PodioCircuito(Vector3 base)
{
    Vector3 arriba = { 0.0f, 1.0f, 0.0f };
    CuboNieblaCircuito({ base.x, base.y + 0.38f, base.z }, { 0.9f, 0.76f, 0.8f }, Color{ 230, 232, 238, 255 });
    CuboNieblaCircuito({ base.x - 0.9f, base.y + 0.26f, base.z }, { 0.9f, 0.52f, 0.8f }, Color{ 214, 216, 224, 255 });
    CuboNieblaCircuito({ base.x + 0.9f, base.y + 0.18f, base.z }, { 0.9f, 0.36f, 0.8f }, Color{ 206, 208, 216, 255 });
    CuboNieblaCircuito({ base.x, base.y + 0.78f, base.z }, { 0.92f, 0.04f, 0.82f }, Color{ 236, 196, 70, 255 });
    CuboNieblaCircuito({ base.x - 0.9f, base.y + 0.54f, base.z }, { 0.92f, 0.04f, 0.82f }, Color{ 190, 196, 210, 255 });
    CuboNieblaCircuito({ base.x + 0.9f, base.y + 0.38f, base.z }, { 0.92f, 0.04f, 0.82f }, Color{ 196, 128, 76, 255 });
    CajaNieblaCircuito({ base.x, base.y + 0.9f, base.z - 0.6f }, { 1.0f, 0.0f, 0.0f }, arriba, { 0.0f, 0.0f, 1.0f }, { 2.9f, 1.8f, 0.08f }, Color{ 40, 48, 90, 255 });
    CuboNieblaCircuito({ base.x, base.y + 1.5f, base.z - 0.55f }, { 2.4f, 0.12f, 0.02f }, COLOR_VOLTAJE_CIRCUITO);
}


// Auto de seguridad estacionado con baliza ambar.
static void AutoSeguridadCircuito(Vector3 base, float t)
{
    Color carroceria = Color{ 214, 218, 226, 255 };
    CuboNieblaCircuito({ base.x, base.y + 0.36f, base.z }, { 1.9f, 0.36f, 0.86f }, carroceria);
    CuboNieblaCircuito({ base.x - 0.15f, base.y + 0.66f, base.z }, { 1.0f, 0.3f, 0.76f }, Color{ 40, 50, 70, 255 });
    CuboNieblaCircuito({ base.x, base.y + 0.36f, base.z + 0.44f }, { 1.6f, 0.1f, 0.02f }, COLOR_AMBAR_CIRCUITO);
    bool encendida = std::fmod(t * 3.0f, 1.0f) < 0.5f;
    CuboNieblaCircuito({ base.x - 0.15f, base.y + 0.86f, base.z }, { 0.6f, 0.1f, 0.2f }, encendida ? Color{ 255, 200, 60, 255 } : Color{ 120, 80, 30, 255 });

    for (int i = 0; i < 4; i++)
    {
        float x = base.x + ((i % 2 == 0) ? 0.6f : -0.6f);
        float z = base.z + ((i < 2) ? 0.44f : -0.44f);
        CilindroCircuito({ x, base.y + 0.2f, z - 0.08f }, { x, base.y + 0.2f, z + 0.08f }, 0.2f, 0.2f, 8, Color{ 24, 24, 28, 255 });
    }
}


static void CamaraTVCircuito(Vector3 base, float giro)
{
    CuboNieblaCircuito({ base.x, base.y + 0.3f, base.z }, { 1.2f, 0.6f, 1.0f }, Color{ 70, 74, 90, 255 });

    for (int i = 0; i < 3; i++)
    {
        float a = 2.0944f * (float)i;
        VigaCircuito({ base.x + std::cos(a) * 0.35f, base.y + 0.6f, base.z + std::sin(a) * 0.35f }, { base.x, base.y + 1.5f, base.z }, 0.04f, 0.04f, COLOR_ACERO_OSCURO_CIRCUITO);
    }

    Vector3 adelante = { std::cos(giro), 0.0f, std::sin(giro) };
    Vector3 lado = { -adelante.z, 0.0f, adelante.x };
    CajaNieblaCircuito({ base.x, base.y + 1.65f, base.z }, adelante, { 0.0f, 1.0f, 0.0f }, lado, { 0.7f, 0.34f, 0.3f }, Color{ 36, 36, 44, 255 });
    Vector3 lente = SumaCircuito(base, EscalaCircuito(adelante, 0.45f));
    CilindroCircuito({ lente.x, base.y + 1.65f, lente.z }, { lente.x + adelante.x * 0.3f, base.y + 1.65f, lente.z + adelante.z * 0.3f }, 0.1f, 0.12f, 8, Color{ 20, 20, 26, 255 });

    // Camarografo.
    Vector3 pie = SumaCircuito(base, EscalaCircuito(adelante, -0.45f));
    CuboNieblaCircuito({ pie.x, base.y + 0.6f + 0.42f, pie.z }, { 0.3f, 0.84f, 0.24f }, Color{ 40, 44, 60, 255 });
    CuboNieblaCircuito({ pie.x, base.y + 0.6f + 0.96f, pie.z }, { 0.22f, 0.22f, 0.22f }, Color{ 198, 142, 102, 255 });
}


static bool HayTromposCircuito(const MinijuegoCircuitoVoltaje& m)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (m.resultado.participantes[i].participo && m.jugadores[i].tiempoTrompo > 0.0f) return true;
    }

    return false;
}


static void DibujarUtileriaCircuito(const MinijuegoCircuitoVoltaje& m, float t, int paso)
{
    bool alerta = HayTromposCircuito(m);

    // Infield: podio, banderas, auto de seguridad, cargadores y carpas.
    PodioCircuito({ -6.4f, 0.0f, 0.4f });

    for (int i = 0; i < 3; i++)
    {
        Vector3 base = { -4.4f + 0.7f * (float)i, 0.0f, -1.2f };
        Vector3 punta = { base.x, 2.6f, base.z };
        CilindroCircuito(base, punta, 0.04f, 0.03f, 6, TinteCircuito(COLOR_ACERO_CIRCUITO, base));
        BanderaCircuito(punta, PALETA_FIESTA_CIRCUITO[i + 1], t, (float)i * 1.7f, 0.9f, 0.55f);
    }

    AutoSeguridadCircuito({ -2.6f, 0.0f, 1.0f }, t);

    for (int i = 0; i < 3; i++)
    {
        CargadorCircuito({ 3.9f + 1.25f * (float)i, 0.0f, -0.9f }, t, i);
    }

    CarpaCircuito({ 8.4f, 0.0f, 0.1f }, 1.3f, COLOR_VOLTAJE_CIRCUITO);
    CarpaCircuito({ -8.8f, 0.0f, 0.0f }, 1.2f, COLOR_PIANO_CIRCUITO);

    // Comisarios a la salida de cada curva.
    MarcoCircuito c1 = MarcoOvaloCircuito(-1.7f, 0.82f, 0.0f);
    MarcoCircuito c2 = MarcoOvaloCircuito(-1.7f, PI + 0.82f, 0.0f);
    PuestoComisarioCircuito(c1.punto, t, alerta, std::atan2(-c1.afuera.z, -c1.afuera.x));
    PuestoComisarioCircuito(c2.punto, t + 0.4f, alerta, std::atan2(-c2.afuera.z, -c2.afuera.x));

    // Recta cercana: pilas de neumaticos, conos, camara de TV y la valla
    // de publicidad del camino de servicio.
    for (int i = 0; i < 46; i += paso)
    {
        float a = PI / 2.0f - 1.15f + 2.3f * (float)i / 45.0f;
        Vector3 p = PuntoOvaloLlanoCircuito(-1.35f, a, 0.0f);
        NeumaticosCircuito(p, 2 + (i % 3 == 0 ? 1 : 0), (i % 2 == 0) ? COLOR_PIANO_CIRCUITO : COLOR_BLANCO_CIRCUITO);
    }

    for (int i = 0; i < 5; i++)
    {
        float a = ANGULO_META_CIRCUITO + MEDIO_ARCO_BOXES_CIRCUITO + 0.2f + 0.06f * (float)i;
        ConoCircuito(PuntoOvaloLlanoCircuito(-1.6f, a, 0.0f));
        ConoCircuito(PuntoOvaloLlanoCircuito(-1.6f, -PI - a, 0.0f));
    }

    MarcoCircuito tv = MarcoOvaloCircuito(-2.4f, PI / 2.0f + 0.32f, 0.0f);
    CamaraTVCircuito(tv.punto, std::atan2(-tv.afuera.z, -tv.afuera.x) + 0.5f * std::sin(t * 0.4f));

    rlBegin(RL_QUADS);

    for (int s = 0; s < 40; s++)
    {
        float a0 = PI / 2.0f - 1.2f + 2.4f * (float)s / 40.0f;
        float a1 = PI / 2.0f - 1.2f + 2.4f * (float)(s + 1) / 40.0f;
        Vector3 p0 = PuntoOvaloLlanoCircuito(-2.9f, a0, 0.0f);
        Vector3 p1 = PuntoOvaloLlanoCircuito(-2.9f, a1, 0.0f);
        Color valla = SombraCircuito((s / 4) % 2 == 0 ? Color{ 40, 48, 90, 255 } : Color{ 220, 222, 230, 255 }, 0.7f, false);
        QuadCircuito(p0, p1, { p1.x, 0.7f, p1.z }, { p0.x, 0.7f, p0.z }, valla);
    }

    rlEnd();
}


//--------------------------------------------------
// Portico de meta con semaforo y contador de vueltas
//--------------------------------------------------

static const float X_PORTICO_CIRCUITO = -2.6f;


static float AnguloEnXCircuito(float k, float x)
{
    float rx = RADIO_X_EXTERIOR - ANCHO_CARRIL * k;
    float c = x / rx;
    if (c < -1.0f) c = -1.0f;
    if (c > 1.0f) c = 1.0f;
    return -std::acos(c);
}


// Luces del semaforo: 0 apagado, 1 rojo, 2 verde.
static int EstadoSemaforoCircuito(const MinijuegoCircuitoVoltaje& m, int columna)
{
    if (m.fase == FASE_CIRCUITO_PREPARACION)
    {
        float transcurrido = DURACION_PREPARACION_CIRCUITO - m.tiempoPreparacion;
        return transcurrido > 0.15f + 0.45f * (float)columna ? 1 : 0;
    }

    if (m.fase == FASE_CIRCUITO_CARRERA && DURACION_CARRERA_CIRCUITO - m.tiempoCarrera < 1.6f)
    {
        return 2;
    }

    return 0;
}


static int VueltaLiderCircuito(const MinijuegoCircuitoVoltaje& m)
{
    int mejor = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;
        int v = m.jugadores[i].llegoMeta ? (int)VUELTAS_CIRCUITO : VueltaActualCircuito(m.jugadores[i]);
        if (m.fase == FASE_CIRCUITO_PREPARACION) v = 0;
        if (v > mejor) mejor = v;
    }

    return mejor;
}


static Vector3 PanelSemaforoCircuito()
{
    Vector3 pata = PuntoOvaloLlanoCircuito(-0.61f, AnguloEnXCircuito(-0.61f, X_PORTICO_CIRCUITO), 0.0f);
    return { pata.x, 7.0f, pata.z };
}


static Vector3 LuzSemaforoCircuito(int columna, int fila)
{
    Vector3 panel = PanelSemaforoCircuito();
    return { panel.x - 1.0f + 0.5f * (float)columna, panel.y + 0.25f - 0.42f * (float)fila, panel.z + 0.2f };
}


static void DibujarPorticoCircuito(const MinijuegoCircuitoVoltaje& m, float t)
{
    float x = X_PORTICO_CIRCUITO;
    Vector3 afuera = PuntoOvaloLlanoCircuito(-0.61f, AnguloEnXCircuito(-0.61f, x), 0.0f);
    Vector3 adentro = PuntoOvaloLlanoCircuito(4.62f, AnguloEnXCircuito(4.62f, x), 0.0f);
    const float ALTO = 5.2f;

    // Patas, dintel y diagonales.
    VigaCircuito({ x, 0.0f, afuera.z }, { x, ALTO + 0.4f, afuera.z }, 0.42f, 0.42f, COLOR_ACERO_CIRCUITO);
    VigaCircuito({ x, 0.0f, adentro.z }, { x, ALTO + 0.4f, adentro.z }, 0.34f, 0.34f, COLOR_ACERO_CIRCUITO);
    VigaCircuito({ x, ALTO + 0.2f, afuera.z - 0.2f }, { x, ALTO + 0.2f, adentro.z + 0.2f }, 0.5f, 0.6f, COLOR_ACERO_OSCURO_CIRCUITO);

    // Franja a cuadros sobre el dintel y luces led que corren (acento).
    int cuadros = 16;
    float largo = adentro.z - afuera.z;

    for (int i = 0; i < cuadros; i++)
    {
        float z0 = afuera.z + largo * ((float)i + 0.5f) / (float)cuadros;
        Color c = (i % 2 == 0) ? Color{ 24, 24, 30, 255 } : COLOR_BLANCO_CIRCUITO;
        CuboCircuito({ x, ALTO + 0.52f, z0 }, { 0.52f, 0.06f, largo / (float)cuadros }, c);

        float onda = std::sin(t * 6.0f - (float)i * 0.7f);
        Color led = MezclaCircuito(Color{ 20, 60, 80, 255 }, COLOR_VOLTAJE_CIRCUITO, 0.5f + 0.5f * onda);
        CuboCircuito({ x + 0.26f, ALTO + 0.05f, z0 }, { 0.02f, 0.12f, largo / (float)cuadros * 0.7f }, led);
    }

    // Cartel de meta a cuadros en la punta interior del dintel, de frente
    // a la camara (queda por encima de los autos que pasan detras).
    Vector3 cartel = { x, ALTO + 0.2f, adentro.z + 0.32f };
    CuboNieblaCircuito(cartel, { 3.6f, 1.5f, 0.2f }, Color{ 26, 28, 36, 255 });

    for (int c = 0; c < 10; c++)
    {
        for (int f = 0; f < 3; f++)
        {
            Color celda = ((c + f) % 2 == 0) ? Color{ 24, 24, 30, 255 } : COLOR_BLANCO_CIRCUITO;
            CuboCircuito({ cartel.x - 1.5f + 0.333f * (float)c, cartel.y - 0.36f + 0.333f * (float)f, cartel.z + 0.11f }, { 0.333f, 0.333f, 0.02f }, celda);
        }
    }

    for (int s = 0; s < 12; s++)
    {
        float onda = 0.5f + 0.5f * std::sin(t * 7.0f - (float)s * 0.8f);
        Color led = MezclaCircuito(Color{ 20, 60, 80, 255 }, COLOR_VOLTAJE_CIRCUITO, onda);
        float lx = cartel.x - 1.65f + 0.3f * (float)s;
        CuboCircuito({ lx, cartel.y + 0.66f, cartel.z + 0.11f }, { 0.24f, 0.1f, 0.03f }, led);
        CuboCircuito({ lx, cartel.y - 0.66f, cartel.z + 0.11f }, { 0.24f, 0.1f, 0.03f }, led);
    }

    // Panel del semaforo sobre la pata exterior, mirando a la camara.
    Vector3 panel = PanelSemaforoCircuito();
    VigaCircuito({ x, ALTO + 0.4f, afuera.z }, { panel.x, panel.y - 0.6f, panel.z }, 0.2f, 0.2f, COLOR_ACERO_OSCURO_CIRCUITO);
    CuboNieblaCircuito(panel, { 2.8f, 1.5f, 0.36f }, Color{ 26, 28, 36, 255 });
    CuboNieblaCircuito({ panel.x, panel.y + 0.8f, panel.z }, { 2.9f, 0.1f, 0.46f }, COLOR_ACERO_CIRCUITO);

    for (int c = 0; c < 5; c++)
    {
        int estado = EstadoSemaforoCircuito(m, c);
        Color color = estado == 1 ? Color{ 255, 60, 50, 255 } : (estado == 2 ? Color{ 90, 255, 120, 255 } : Color{ 70, 30, 30, 255 });

        for (int f = 0; f < 2; f++)
        {
            Vector3 luz = LuzSemaforoCircuito(c, f);
            CilindroCircuito(luz, { luz.x, luz.y, luz.z + 0.05f }, 0.15f, 0.15f, 10, color);
        }
    }

    // Contador de vueltas del lider (cuatro pilotos cian bajo el panel).
    int vueltaLider = VueltaLiderCircuito(m);

    for (int v = 0; v < (int)VUELTAS_CIRCUITO; v++)
    {
        bool encendida = v < vueltaLider;
        Color color = encendida ? COLOR_VOLTAJE_CIRCUITO : Color{ 30, 50, 60, 255 };
        CuboCircuito({ panel.x - 0.75f + 0.5f * (float)v, panel.y - 0.98f, panel.z + 0.1f }, { 0.36f, 0.16f, 0.1f }, color);
    }

    CuboNieblaCircuito({ panel.x, panel.y - 0.98f, panel.z - 0.02f }, { 2.2f, 0.3f, 0.2f }, Color{ 26, 28, 36, 255 });
}


//--------------------------------------------------
// Torres de luz y pantalla gigante
//--------------------------------------------------

struct TorreLuzCircuitoDef
{
    Vector3 base;
    float alto;
    Vector3 objetivo;
};


static const TorreLuzCircuitoDef TORRES_LUZ_CIRCUITO[6] =
{
    { { -30.0f, 0.0f, -15.0f }, 17.0f, { -13.0f, 0.0f, -4.0f } },
    { { 30.0f, 0.0f, -15.0f }, 17.0f, { 13.0f, 0.0f, -4.0f } },
    { { -12.0f, 0.0f, -29.0f }, 18.0f, { -5.0f, 0.0f, -8.0f } },
    { { 12.0f, 0.0f, -29.0f }, 18.0f, { 5.0f, 0.0f, -8.0f } },
    { { -31.0f, 0.0f, 13.0f }, 15.0f, { -14.0f, 0.0f, 6.0f } },
    { { 31.0f, 0.0f, 13.0f }, 15.0f, { 14.0f, 0.0f, 6.0f } }
};


static Vector3 CabezaTorreCircuito(const TorreLuzCircuitoDef& torre)
{
    return { torre.base.x, torre.base.y + torre.alto + 1.0f, torre.base.z };
}


static void TorreLuzCircuito(const TorreLuzCircuitoDef& torre, float t)
{
    Vector3 b = torre.base;
    float alto = torre.alto;
    Vector3 patasAbajo[4];
    Vector3 patasArriba[4];

    for (int i = 0; i < 4; i++)
    {
        float sx = (i == 0 || i == 3) ? -1.0f : 1.0f;
        float sz = (i < 2) ? -1.0f : 1.0f;
        patasAbajo[i] = { b.x + sx * 0.8f, b.y, b.z + sz * 0.8f };
        patasArriba[i] = { b.x + sx * 0.35f, b.y + alto, b.z + sz * 0.35f };
        VigaCircuito(patasAbajo[i], patasArriba[i], 0.12f, 0.12f, COLOR_ACERO_CIRCUITO);
    }

    // Reticulado en X por tramos.
    int tramos = (int)(alto / 2.4f);

    for (int k = 0; k < tramos; k++)
    {
        float u0 = (float)k / (float)tramos;
        float u1 = (float)(k + 1) / (float)tramos;

        for (int i = 0; i < 4; i++)
        {
            int j = (i + 1) % 4;
            Vector3 a0 = SumaCircuito(patasAbajo[i], EscalaCircuito(RestaCircuito(patasArriba[i], patasAbajo[i]), u0));
            Vector3 b1 = SumaCircuito(patasAbajo[j], EscalaCircuito(RestaCircuito(patasArriba[j], patasAbajo[j]), u1));
            Vector3 a1 = SumaCircuito(patasAbajo[i], EscalaCircuito(RestaCircuito(patasArriba[i], patasAbajo[i]), u1));
            VigaCircuito(a0, b1, 0.05f, 0.05f, COLOR_ACERO_OSCURO_CIRCUITO);
            VigaCircuito(a1, { b1.x, b1.y, b1.z }, 0.05f, 0.05f, COLOR_ACERO_OSCURO_CIRCUITO);
        }
    }

    // Cabezal con grilla de lamparas inclinado hacia la pista.
    Vector3 cabeza = CabezaTorreCircuito(torre);
    Vector3 haciaPista = NormalizarCircuito(RestaCircuito(torre.objetivo, cabeza));
    Vector3 plano = NormalizarCircuito({ haciaPista.x, 0.0f, haciaPista.z });
    Vector3 lado = { -plano.z, 0.0f, plano.x };
    Vector3 arriba = NormalizarCircuito(CruzCircuito(haciaPista, lado));
    if (arriba.y < 0.0f) arriba = EscalaCircuito(arriba, -1.0f);

    CajaNieblaCircuito(cabeza, lado, arriba, haciaPista, { 3.6f, 2.3f, 0.3f }, COLOR_ACERO_OSCURO_CIRCUITO);

    for (int f = 0; f < 3; f++)
    {
        for (int c = 0; c < 4; c++)
        {
            float titila = (f == 1 && c == 2) ? 0.75f + 0.25f * std::sin(t * 23.0f) : 1.0f;
            Vector3 p = SumaCircuito(cabeza, SumaCircuito(EscalaCircuito(lado, -1.2f + 0.8f * (float)c), EscalaCircuito(arriba, -0.7f + 0.7f * (float)f)));
            p = SumaCircuito(p, EscalaCircuito(haciaPista, 0.17f));
            CajaOrientadaCircuito(p, lado, arriba, haciaPista, { 0.66f, 0.56f, 0.06f }, TonoCircuito(Color{ 255, 246, 220, 255 }, titila));
        }
    }
}


static void DibujarPantallaCircuito(const MinijuegoCircuitoVoltaje& m, const Participante participantes[], float t)
{
    Vector3 base = { 24.5f, 0.0f, -20.5f };
    Vector3 normal = NormalizarCircuito({ -base.x * 0.85f, 0.0f, 12.0f - base.z });
    Vector3 derecha = { normal.z, 0.0f, -normal.x };
    Vector3 arriba = { 0.0f, 1.0f, 0.0f };
    const float ANCHO = 8.0f;
    const float ALTO = 4.6f;
    Vector3 centro = { base.x, 8.6f, base.z };

    for (int i = -1; i <= 1; i += 2)
    {
        Vector3 pie = SumaCircuito(base, EscalaCircuito(derecha, (float)i * ANCHO * 0.32f));
        VigaCircuito(pie, { pie.x, centro.y - ALTO * 0.5f, pie.z }, 0.4f, 0.4f, COLOR_ACERO_OSCURO_CIRCUITO);
    }

    CajaNieblaCircuito(centro, derecha, arriba, normal, { ANCHO + 0.5f, ALTO + 0.5f, 0.5f }, Color{ 30, 32, 44, 255 });

    // Contenido: barras de avance de cada auto, con su color.
    Vector3 cara = SumaCircuito(centro, EscalaCircuito(normal, 0.27f));
    auto P = [&](float u, float v) -> Vector3
    {
        return SumaCircuito(cara, SumaCircuito(EscalaCircuito(derecha, u), EscalaCircuito(arriba, v)));
    };

    int ganador = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (m.fase == FASE_CIRCUITO_TERMINADO && m.resultado.participantes[i].participo && m.resultado.participantes[i].posicionFinal == 1)
        {
            ganador = i;
        }
    }

    Color fondo = Color{ 10, 18, 40, 255 };

    if (ganador >= 0 && std::fmod(t * 2.0f, 1.0f) < 0.5f)
    {
        fondo = MezclaCircuito(fondo, participantes[ganador].color, 0.45f);
    }

    rlBegin(RL_QUADS);
    QuadCircuito(P(-ANCHO * 0.5f, -ALTO * 0.5f), P(ANCHO * 0.5f, -ALTO * 0.5f), P(ANCHO * 0.5f, ALTO * 0.5f), P(-ANCHO * 0.5f, ALTO * 0.5f), fondo);

    // Franja de titulo que corre.
    for (int s = 0; s < 16; s++)
    {
        float u0 = -ANCHO * 0.5f + ANCHO * (float)s / 16.0f;
        float u1 = u0 + ANCHO / 16.0f;
        float onda = 0.5f + 0.5f * std::sin(t * 5.0f - (float)s * 0.6f);
        Color c = MezclaCircuito(Color{ 20, 70, 100, 255 }, COLOR_VOLTAJE_CIRCUITO, onda);
        QuadCircuito(P(u0, ALTO * 0.3f), P(u1, ALTO * 0.3f), P(u1, ALTO * 0.45f), P(u0, ALTO * 0.45f), c);
    }

    int fila = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        float v0 = ALTO * 0.16f - (float)fila * ALTO * 0.19f;
        float v1 = v0 - ALTO * 0.12f;
        float avance = Limitar01Circuito(m.jugadores[i].avance / VUELTAS_CIRCUITO);
        float u0 = -ANCHO * 0.42f;
        float u1 = u0 + ANCHO * 0.84f * avance;
        Color c = participantes[i].color;

        QuadCircuito(P(u0, v1), P(ANCHO * 0.42f, v1), P(ANCHO * 0.42f, v0), P(u0, v0), Color{ 30, 40, 64, 255 });
        if (avance > 0.002f) QuadCircuito(P(u0, v1), P(u1, v1), P(u1, v0), P(u0, v0), c);
        QuadCircuito(P(u1 - 0.06f, v1 - 0.05f), P(u1 + 0.06f, v1 - 0.05f), P(u1 + 0.06f, v0 + 0.05f), P(u1 - 0.06f, v0 + 0.05f), WHITE);
        fila++;
    }

    rlEnd();
}


//--------------------------------------------------
// Autos electricos
//--------------------------------------------------

// Auto en el marco local: +X adelante, +Y arriba, +Z hacia el interior.
static void DibujarAutoCircuito(
    const PoseAutoCircuito& pose,
    float desvioLateral,
    Color color,
    bool enTrompo,
    bool advertencia,
    float destello,
    float t,
    float giroRuedas
)
{
    Color cuerpo = color;

    if (advertencia && ((int)(t * 12.0f) % 2 == 0))
    {
        cuerpo = MezclaCircuito(color, Color{ 255, 60, 50, 255 }, 0.65f);
    }

    if (destello > 0.0f)
    {
        cuerpo = MezclaCircuito(cuerpo, WHITE, 0.5f * Limitar01Circuito(destello));
    }

    Color claro = MezclaCircuito(cuerpo, WHITE, 0.55f);
    Color carbono = COLOR_CARBONO_CIRCUITO;

    rlPushMatrix();
    rlTranslatef(pose.posicion.x, pose.posicion.y, pose.posicion.z);
    rlRotatef(-pose.rumbo, 0.0f, 1.0f, 0.0f);
    rlRotatef(pose.peralte, 1.0f, 0.0f, 0.0f);
    rlTranslatef(0.0f, 0.0f, desvioLateral);

    if (enTrompo)
    {
        rlRotatef(std::fmod(t * 720.0f, 360.0f), 0.0f, 1.0f, 0.0f);
    }

    // Sombra blanda bajo el auto.
    if (CalidadSombras() != CALIDAD_BAJA)
    {
        rlBegin(RL_QUADS);
        QuadCircuito({ -1.3f, 0.025f, 0.62f }, { 1.4f, 0.025f, 0.62f }, { 1.4f, 0.025f, -0.62f }, { -1.3f, 0.025f, -0.62f }, Color{ 0, 0, 10, 60 });
        QuadCircuito({ -1.1f, 0.03f, 0.48f }, { 1.2f, 0.03f, 0.48f }, { 1.2f, 0.03f, -0.48f }, { -1.1f, 0.03f, -0.48f }, Color{ 0, 0, 10, 90 });
        rlEnd();
    }

    // Piso, pontones, monocasco y nariz.
    CuboCircuito({ 0.0f, 0.13f, 0.0f }, { 2.0f, 0.06f, 0.86f }, carbono);
    CuboCircuito({ -0.15f, 0.3f, 0.33f }, { 0.95f, 0.24f, 0.22f }, cuerpo);
    CuboCircuito({ -0.15f, 0.3f, -0.33f }, { 0.95f, 0.24f, 0.22f }, cuerpo);
    CuboCircuito({ 0.3f, 0.3f, 0.0f }, { 1.2f, 0.26f, 0.42f }, cuerpo);
    CuboCircuito({ 1.02f, 0.22f, 0.0f }, { 0.5f, 0.14f, 0.28f }, claro);

    // Aleron delantero con derivas del color del jugador.
    CuboCircuito({ 1.2f, 0.12f, 0.0f }, { 0.28f, 0.04f, 1.04f }, carbono);
    CuboCircuito({ 1.2f, 0.18f, 0.51f }, { 0.32f, 0.12f, 0.03f }, cuerpo);
    CuboCircuito({ 1.2f, 0.18f, -0.51f }, { 0.32f, 0.12f, 0.03f }, cuerpo);

    // Cubierta del motor con franja electrica, deriva y aleron trasero.
    CuboCircuito({ -0.55f, 0.43f, 0.0f }, { 0.8f, 0.26f, 0.34f }, cuerpo);
    CuboCircuito({ -0.55f, 0.565f, 0.0f }, { 0.7f, 0.02f, 0.1f }, COLOR_VOLTAJE_CIRCUITO);
    CuboCircuito({ -0.78f, 0.66f, 0.0f }, { 0.48f, 0.18f, 0.03f }, claro);
    CuboCircuito({ -0.98f, 0.52f, 0.2f }, { 0.08f, 0.36f, 0.04f }, carbono);
    CuboCircuito({ -0.98f, 0.52f, -0.2f }, { 0.08f, 0.36f, 0.04f }, carbono);
    CuboCircuito({ -1.02f, 0.73f, 0.0f }, { 0.28f, 0.06f, 1.0f }, claro);
    CuboCircuito({ -1.02f, 0.66f, 0.49f }, { 0.32f, 0.2f, 0.03f }, carbono);
    CuboCircuito({ -1.02f, 0.66f, -0.49f }, { 0.32f, 0.2f, 0.03f }, carbono);

    // Halo de proteccion sobre la cabina.
    VigaCircuito({ 0.42f, 0.44f, 0.0f }, { 0.3f, 0.66f, 0.0f }, 0.05f, 0.05f, carbono);
    VigaCircuito({ 0.3f, 0.66f, 0.0f }, { -0.15f, 0.62f, 0.17f }, 0.05f, 0.05f, carbono);
    VigaCircuito({ 0.3f, 0.66f, 0.0f }, { -0.15f, 0.62f, -0.17f }, 0.05f, 0.05f, carbono);

    // Ruedas con llanta que gira.
    for (int r = 0; r < 4; r++)
    {
        float x = (r < 2) ? 0.72f : -0.68f;
        float radio = (r < 2) ? 0.2f : 0.23f;
        float z = (r % 2 == 0) ? 0.5f : -0.5f;
        float signo = (r % 2 == 0) ? 1.0f : -1.0f;
        CilindroCircuito({ x, radio, z - 0.11f }, { x, radio, z + 0.11f }, radio, radio, 10, Color{ 24, 24, 28, 255 });

        Vector3 cubo = { x, radio, z + signo * 0.115f };
        CilindroCircuito({ cubo.x, cubo.y, cubo.z - signo * 0.01f }, cubo, radio * 0.6f, radio * 0.6f, 8, Color{ 150, 154, 166, 255 });
        float a = giroRuedas;
        VigaCircuito(
            { cubo.x + std::cos(a) * radio * 0.55f, cubo.y + std::sin(a) * radio * 0.55f, cubo.z + signo * 0.006f },
            { cubo.x - std::cos(a) * radio * 0.55f, cubo.y - std::sin(a) * radio * 0.55f, cubo.z + signo * 0.006f },
            0.05f, 0.01f, cuerpo
        );
    }

    // Faros led y luz trasera (roja viva si se pasa o hace trompo).
    CuboCircuito({ 1.28f, 0.22f, 0.0f }, { 0.03f, 0.04f, 0.24f }, Color{ 255, 250, 220, 255 });
    bool frena = advertencia || enTrompo;
    CuboCircuito({ -1.17f, 0.52f, 0.0f }, { 0.03f, 0.08f, 0.22f }, frena ? Color{ 255, 70, 60, 255 } : Color{ 150, 30, 30, 255 });

    // Piloto con el modelo compartido, sentado en la cabina.
    DibujarModeloJugadorEnPosicion({ -0.12f, 0.1f, 0.0f }, 90.0f, color, ESCALA_MODELO_JUGADOR_3D * 0.42f);

    rlPopMatrix();
}


//--------------------------------------------------
// Particulas y luces aditivas
//--------------------------------------------------

static void DibujarParticulasCircuito(const MinijuegoCircuitoVoltaje& m, bool aditivas)
{
    for (int i = 0; i < MAX_PARTICULAS_CIRCUITO_VOLTAJE; i++)
    {
        const ParticulaCircuitoVoltaje& p = m.particulas[i];
        if (!p.activa) continue;

        float vida = Limitar01Circuito(p.vida / p.duracion);
        bool luminosa = p.tipo == 0 || p.tipo == 3;

        if (luminosa != aditivas) continue;

        if (luminosa)
        {
            Vector3 cola = SumaCircuito(p.posicion, EscalaCircuito(p.velocidad, -0.06f));
            HazCircuito(cola, p.posicion, p.tamano * 0.3f, p.tamano, ConAlfaCircuito(p.color, 0.0f), ConAlfaCircuito(p.color, vida));
            ResplandorFrenteCircuito(p.posicion, p.tamano * 2.2f, ConAlfaCircuito(p.color, 0.5f * vida));
        }
        else if (p.tipo == 1)
        {
            Color humo = ConAlfaCircuito(p.color, 0.55f * vida);
            ResplandorFrenteCircuito(p.posicion, p.tamano, humo);
            ResplandorFrenteCircuito(p.posicion, p.tamano * 0.6f, humo);
        }
        else
        {
            // Confeti: papelito que gira.
            float a = (float)i * 1.7f + p.vida * 9.0f;
            Vector3 u = { std::cos(a) * p.tamano, std::sin(a * 1.3f) * p.tamano * 0.6f, std::sin(a) * p.tamano };
            Vector3 v = { -std::sin(a) * p.tamano * 0.6f, p.tamano * 0.5f, std::cos(a) * p.tamano * 0.6f };
            Color c = SombraCircuito(p.color, 0.75f + 0.25f * std::sin(a * 2.0f), false);
            rlBegin(RL_QUADS);
            QuadCircuito(RestaCircuito(RestaCircuito(p.posicion, u), v), RestaCircuito(SumaCircuito(p.posicion, u), v), SumaCircuito(SumaCircuito(p.posicion, u), v), SumaCircuito(RestaCircuito(p.posicion, u), v), c);
            rlEnd();
        }
    }
}


// Chispas y fuegos sin luces aditivas (calidad de efectos baja).
static void DibujarParticulasPlanasCircuito(const MinijuegoCircuitoVoltaje& m)
{
    for (int i = 0; i < MAX_PARTICULAS_CIRCUITO_VOLTAJE; i++)
    {
        const ParticulaCircuitoVoltaje& p = m.particulas[i];
        if (!p.activa || (p.tipo != 0 && p.tipo != 3)) continue;
        CuboCircuito(p.posicion, { p.tamano, p.tamano, p.tamano }, p.color);
    }
}


static void DibujarLucesCircuito(const MinijuegoCircuitoVoltaje& m, const Participante participantes[], float t, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    Color calida = COLOR_LUZ_CIRCUITO;

    // Torres: halo, cono hacia la pista y charco de luz sobre el asfalto.
    for (int i = 0; i < 6; i++)
    {
        const TorreLuzCircuitoDef& torre = TORRES_LUZ_CIRCUITO[i];
        Vector3 cabeza = CabezaTorreCircuito(torre);
        Vector3 objetivo = torre.objetivo;
        objetivo.y = 0.06f;

        if (decoracion)
        {
            ResplandorFrenteCircuito(cabeza, 4.2f, ConAlfaCircuito(calida, 0.28f));
            ResplandorFrenteCircuito(cabeza, 2.0f, ConAlfaCircuito(WHITE, 0.5f));
            HazCircuito(cabeza, objetivo, 3.0f, 14.0f, ConAlfaCircuito(calida, 0.09f), ConAlfaCircuito(calida, 0.0f));
        }

        ResplandorSueloCircuito(objetivo, 10.0f, ConAlfaCircuito(calida, 0.11f), 1.3f);
        ResplandorSueloCircuito(objetivo, 5.5f, ConAlfaCircuito(calida, 0.07f), 1.3f);

        // Bichitos que revolotean alrededor del cabezal.
        if (decoracion)
        {
            for (int b = 0; b < 5; b++)
            {
                float a = t * (1.2f + 0.3f * (float)b) + (float)(i * 5 + b);
                Vector3 p = { cabeza.x + std::cos(a) * (1.4f + 0.3f * (float)b), cabeza.y + 0.6f * std::sin(a * 1.7f), cabeza.z + std::sin(a) * 1.2f };
                ResplandorFrenteCircuito(p, 0.12f, ConAlfaCircuito(calida, 0.8f));
            }
        }
    }

    // Luz de los boxes sobre la calle de boxes.
    for (int i = 0; i < CANTIDAD_BOXES_CIRCUITO; i++)
    {
        float a = AnguloBoxCircuito(i);
        Vector3 p = PuntoOvaloLlanoCircuito(K_FRENTE_BOXES_CIRCUITO + 0.5f, a, 0.02f);
        ResplandorSueloCircuito(p, 1.8f, ConAlfaCircuito(MezclaCircuito(calida, EQUIPOS_CIRCUITO[i], 0.3f), 0.22f), 1.4f);
        Vector3 dentro = PuntoOvaloLlanoCircuito(K_FRENTE_BOXES_CIRCUITO - 0.6f, a, ALTO_BOXES_CIRCUITO - 0.3f);
        ResplandorFrenteCircuito(dentro, 1.2f, ConAlfaCircuito(calida, 0.22f));
    }

    if (decoracion)
    {
        // Cargadores (cian), baliza del auto de seguridad y reflectores.
        for (int i = 0; i < 3; i++)
        {
            float pulso = 0.6f + 0.4f * std::sin(t * 2.5f + (float)i * 1.3f);
            ResplandorSueloCircuito({ 3.9f + 1.25f * (float)i, 0.03f, -0.6f }, 1.1f, ConAlfaCircuito(COLOR_VOLTAJE_CIRCUITO, 0.3f * pulso));
            ResplandorFrenteCircuito({ 3.9f + 1.25f * (float)i, 0.92f, -0.74f }, 0.4f, ConAlfaCircuito(COLOR_VOLTAJE_CIRCUITO, 0.5f * pulso));
        }

        if (std::fmod(t * 3.0f, 1.0f) < 0.5f)
        {
            ResplandorFrenteCircuito({ -2.75f, 0.9f, 1.0f }, 0.9f, ConAlfaCircuito(COLOR_AMBAR_CIRCUITO, 0.6f));
            ResplandorSueloCircuito({ -2.6f, 0.03f, 1.0f }, 2.4f, ConAlfaCircuito(COLOR_AMBAR_CIRCUITO, 0.12f));
        }

        for (int i = 0; i < 2; i++)
        {
            Vector3 base = { i == 0 ? -20.0f : 22.0f, 2.0f, -36.0f };
            float a = 0.35f * std::sin(t * 0.45f + (float)i * 2.4f) + (i == 0 ? -0.25f : 0.25f);
            Vector3 dir = NormalizarCircuito({ std::sin(a), 0.85f, -0.35f });
            HazCircuito(base, SumaCircuito(base, EscalaCircuito(dir, 110.0f)), 1.2f, 9.0f, Color{ 190, 200, 255, 60 }, Color{ 190, 200, 255, 0 });
        }

        // Balizas rojas de los molinos y de la torre de TV.
        float baliza = std::fmod(t * 0.8f, 1.0f) < 0.3f ? 1.0f : 0.15f;
        ResplandorFrenteCircuito({ 46.0f, 30.5f, -170.0f }, 3.0f, ConAlfaCircuito(Color{ 255, 60, 60, 255 }, 0.7f * baliza));

        // Flashes de fotos en las tribunas.
        for (int f = 0; f < 6; f++)
        {
            float ciclo = t * 1.3f + (float)f * 0.37f;
            int id = (int)std::floor(ciclo);
            float u = ciclo - (float)id;

            if (u < 0.08f)
            {
                float a = ANGULO_META_CIRCUITO + (HashCircuito(id, f) - 0.5f) * 2.0f * MEDIO_ARCO_TRIBUNA_CIRCUITO;
                float k = -4.6f - 3.4f * HashCircuito(id, f + 9);
                Vector3 p = PuntoOvaloLlanoCircuito(k, a, 3.6f + 2.0f * HashCircuito(id, f + 3));
                ResplandorFrenteCircuito(p, 0.9f, ConAlfaCircuito(WHITE, 0.9f * (1.0f - u / 0.08f)));
            }
        }
    }

    // Semaforo y contador de vueltas.
    for (int c = 0; c < 5; c++)
    {
        int estado = EstadoSemaforoCircuito(m, c);
        if (estado == 0) continue;
        Color color = estado == 1 ? Color{ 255, 60, 50, 255 } : Color{ 90, 255, 120, 255 };

        for (int f = 0; f < 2; f++)
        {
            ResplandorFrenteCircuito(SumaCircuito(LuzSemaforoCircuito(c, f), { 0.0f, 0.0f, 0.15f }), 0.75f, ConAlfaCircuito(color, 0.7f));
        }
    }

    // Autos: brillo electrico bajo el chasis, faros, luz trasera y
    // destello al completar una vuelta.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const EstadoJugadorCircuitoVoltaje& j = m.jugadores[i];
        PoseAutoCircuito pose = CalcularPoseAutoCircuito(i, j.avance);
        Color color = participantes[i].color;
        float vel = Limitar01Circuito(j.velocidad);
        Vector3 suelo = { pose.posicion.x, pose.posicion.y + 0.05f, pose.posicion.z };

        ResplandorSueloCircuito(suelo, 1.5f, ConAlfaCircuito(MezclaCircuito(color, COLOR_VOLTAJE_CIRCUITO, 0.25f), 0.3f + 0.4f * vel), 1.5f);

        Vector3 frente = SumaCircuito(pose.posicion, EscalaCircuito(pose.adelante, 1.3f));
        frente.y += 0.22f;
        Vector3 lejos = SumaCircuito(pose.posicion, EscalaCircuito(pose.adelante, 5.5f));
        lejos.y += 0.05f;
        HazCircuito(frente, lejos, 0.3f, 2.4f, Color{ 255, 250, 220, 70 }, Color{ 255, 250, 220, 0 });
        ResplandorFrenteCircuito(frente, 0.45f, Color{ 255, 250, 220, 140 });
        ResplandorSueloCircuito(SumaCircuito(pose.posicion, EscalaCircuito(pose.adelante, 3.4f)), 1.6f, Color{ 255, 250, 220, 30 }, 1.4f);

        Vector3 cola = SumaCircuito(pose.posicion, EscalaCircuito(pose.adelante, -1.2f));
        cola.y += 0.52f;
        bool exceso = ExcedidoCircuito(m, i);
        bool trompo = j.tiempoTrompo > 0.0f;
        float rojo = (exceso || trompo) ? 0.85f : 0.3f;
        ResplandorFrenteCircuito(cola, (exceso || trompo) ? 0.9f : 0.45f, ConAlfaCircuito(Color{ 255, 50, 40, 255 }, rojo));

        if (m.destelloVuelta[i] > 0.0f)
        {
            float d = Limitar01Circuito(m.destelloVuelta[i]);
            ResplandorFrenteCircuito({ pose.posicion.x, pose.posicion.y + 0.6f, pose.posicion.z }, 2.6f * d + 0.5f, ConAlfaCircuito(color, 0.6f * d));
            ResplandorSueloCircuito(suelo, 3.2f, ConAlfaCircuito(color, 0.35f * d));
        }
    }

    DibujarParticulasCircuito(m, true);

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Dibujar
//--------------------------------------------------

static Camera3D CamaraCircuito(const MinijuegoCircuitoVoltaje& m)
{
    Camera3D camara{};
    camara.position = OJO_CIRCUITO;
    camara.target = MIRA_CIRCUITO;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = FOV_CIRCUITO;
    camara.projection = CAMERA_PERSPECTIVE;

    // Entrada: durante la cuenta la camara barre desde un costado alto.
    if (m.fase == FASE_CIRCUITO_PREPARACION)
    {
        float u = Limitar01Circuito(m.tiempoPreparacion / DURACION_PREPARACION_CIRCUITO);
        float s = SuaveCircuito(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        camara.position.x += 12.0f * s;
        camara.position.y += 6.0f * s;
        camara.position.z += 5.0f * s;
        camara.target.x += 3.0f * s;
    }

    return camara;
}


static const char* TextoAcelerarCircuito(const Participante& participante)
{
    switch (participante.control)
    {
        case CONTROL_GAMEPAD: return "STICK ARRIBA";
        case CONTROL_TECLADO_FLECHAS: return "FLECHA ARRIBA";
        case CONTROL_TECLADO_WASD: return "W";
        case CONTROL_TECLADO_COMPLETO: return "W / ARRIBA";
        case CONTROL_NINGUNO: break;
    }

    return "";
}


void MinijuegoCircuitoVoltaje::Dibujar(
    const Participante participantes[]
) const
{

    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    Camera3D vista = CamaraCircuito(*this);

    ClearBackground(COLOR_NIEBLA_CIRCUITO);
    DibujarCieloCircuito(vista, t);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte y terreno.
    if (decoracion) DibujarHorizonteCircuito(t, paso);
    DibujarTerrenoCircuito(paso, decoracion);

    // Arena: infield, pista, muro y boxes.
    DibujarInfieldCircuito();
    DibujarPistaCircuito();
    DibujarMuroCircuito(decoracion);
    DibujarBoxesCircuito(t, decoracion);

    // Media distancia y anillo cercano.
    if (decoracion)
    {
        float animo = fase == FASE_CIRCUITO_TERMINADO ? 1.0f : (tiempoAnuncioVuelta < 1.5f ? 0.45f : 0.0f);
        TribunaCircuito(ANGULO_META_CIRCUITO - MEDIO_ARCO_TRIBUNA_CIRCUITO, ANGULO_META_CIRCUITO + MEDIO_ARCO_TRIBUNA_CIRCUITO, -4.6f, 7, 2.85f, t, animo, 1, paso, true);
        TribunaCircuito(-0.5f, 0.5f, -2.9f, 5, 1.6f, t, animo, 2, paso, false);
        TribunaCircuito(PI - 0.5f, PI + 0.5f, -2.9f, 5, 1.6f, t, animo, 3, paso, false);

        for (int i = 0; i < 6; i++) TorreLuzCircuito(TORRES_LUZ_CIRCUITO[i], t);

        DibujarPantallaCircuito(*this, participantes, t);
        DibujarUtileriaCircuito(*this, t, paso);
    }

    DibujarPorticoCircuito(*this, t);

    // Autos con su piloto.
    Vector2 rotulos[MAX_PARTICIPANTES]{};
    Vector2 avisosPantalla[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        const EstadoJugadorCircuitoVoltaje& j = jugadores[i];
        PoseAutoCircuito pose = CalcularPoseAutoCircuito(i, j.avance);
        bool enTrompo = j.tiempoTrompo > 0.0f;
        float desvio = 0.0f;

        if (enTrompo)
        {
            float progreso = 1.0f - j.tiempoTrompo / 1.05f;
            desvio = -std::sin(Limitar01Circuito(progreso) * PI) * 0.5f;
        }

        DibujarAutoCircuito(
            pose,
            desvio,
            participantes[i].color,
            enTrompo,
            ExcedidoCircuito(*this, i),
            destelloVuelta[i],
            t,
            j.avance * 420.0f
        );

        rotulos[i] = GetWorldToScreen({ pose.posicion.x, pose.posicion.y + 1.55f, pose.posicion.z }, vista);
        avisosPantalla[i] = GetWorldToScreen({ pose.posicion.x, pose.posicion.y + 2.3f, pose.posicion.z }, vista);
    }

    DibujarParticulasCircuito(*this, false);

    if (efectos)
    {
        DibujarLucesCircuito(*this, participantes, t, decoracion);
    }
    else
    {
        DibujarParticulasPlanasCircuito(*this);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Cartelitos J1..J4 sobre cada auto. Cuando los autos van juntos (la
    // grilla de largada) se separan de costado para no taparse.
    for (int pasada = 0; pasada < 4; pasada++)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            for (int k = i + 1; k < MAX_PARTICIPANTES; k++)
            {
                if (!resultado.participantes[i].participo || !resultado.participantes[k].participo) continue;

                float dx = rotulos[k].x - rotulos[i].x;
                float dy = rotulos[k].y - rotulos[i].y;
                float minimoX = Px(40.0f);

                if (std::fabs(dy) < Px(26.0f) && std::fabs(dx) < minimoX)
                {
                    float empuje = (minimoX - std::fabs(dx)) * 0.5f;
                    float signo = dx > 0.0f || (dx == 0.0f && k > i) ? 1.0f : -1.0f;
                    rotulos[i].x -= empuje * signo;
                    rotulos[k].x += empuje * signo;
                }
            }
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo) continue;

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, rotulos[i], 0.0f, op);
    }

    // «¡SOLTÁ!» sobre los humanos que se pasan de velocidad.
    if (fase == FASE_CIRCUITO_CARRERA)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo || participantes[i].esBot || !ExcedidoCircuito(*this, i)) continue;

            float pulso = 0.75f + 0.25f * std::sin(t * 16.0f);
            DibujarTextoMundoContornoUI("¡SOLTÁ!", avisosPantalla[i].x, avisosPantalla[i].y - Px(6.0f), UI_TEXTO, Color{ 255, 150, 60, 255 }, pulso);
        }
    }

    // Avisos sobre los autos: trompo, vuelta y meta.
    for (int a = 0; a < MAX_AVISOS_CIRCUITO_VOLTAJE; a++)
    {
        const AvisoCircuitoVoltaje& aviso = avisos[a];
        if (!aviso.activo) continue;

        int i = aviso.jugador;
        float u = 1.0f - aviso.vida / DURACION_AVISO_CIRCUITO;
        float alfa = aviso.vida < 0.4f ? aviso.vida / 0.4f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        float x = avisosPantalla[i].x;
        float y = avisosPantalla[i].y - Px(14.0f) - u * Px(36.0f);
        float limiteAbajo = (float)(alto - MargenBarraRondaMinijuegos()) - Px(60.0f);
        if (y < Px(130.0f)) y = Px(130.0f);
        if (y > limiteAbajo) y = limiteAbajo;
        if (x < Px(120.0f)) x = Px(120.0f);
        if (x > (float)ancho - Px(120.0f)) x = (float)ancho - Px(120.0f);

        char texto[32];

        if (aviso.tipo == 0)
        {
            DibujarTextoMundoContornoUI("¡TROMPO!", x, y, UI_TEXTO, Color{ 236, 236, 242, 255 }, alfa);
        }
        else if (aviso.tipo == 1)
        {
            std::snprintf(texto, sizeof(texto), "VUELTA %d", aviso.valor);
            DibujarTextoMundoContornoUI(texto, x, y, UI_TEXTO, MezclaCircuito(participantes[i].color, WHITE, 0.25f), alfa);
        }
        else
        {
            DibujarTextoMundoContornoUI("¡META!", x, y, UI_TEXTO_GRANDE, Color{ 255, 214, 80, 255 }, alfa);
        }
    }

    // Anuncio central: el lider abre una vuelta nueva.
    if (fase == FASE_CIRCUITO_CARRERA && tiempoAnuncioVuelta < DURACION_ANUNCIO_VUELTA_CIRCUITO)
    {
        float u = tiempoAnuncioVuelta;
        float alfa = u < 0.15f ? u / 0.15f : (u > DURACION_ANUNCIO_VUELTA_CIRCUITO - 0.4f ? (DURACION_ANUNCIO_VUELTA_CIRCUITO - u) / 0.4f : 1.0f);
        int vuelta = vueltaLiderVista + 1;
        char texto[32];

        if (vuelta >= (int)VUELTAS_CIRCUITO)
        {
            std::snprintf(texto, sizeof(texto), "¡ÚLTIMA VUELTA!");
        }
        else
        {
            std::snprintf(texto, sizeof(texto), "¡VUELTA %d!", vuelta);
        }

        DibujarTextoMundoContornoUI(texto, (float)ancho * 0.5f, (float)alto * 0.24f, UI_TEXTO_GRANDE, COLOR_VOLTAJE_CIRCUITO, alfa);
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CIRCUITO VOLTAJE",
        "MANTENÉ ACELERADO EN LAS RECTAS Y SOLTÁ EN LAS CURVAS ROJAS: CUATRO VUELTAS.",
        resultado
    );

    hud.fase = fase == FASE_CIRCUITO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CIRCUITO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoCarrera;
    hud.tiempoTotal = DURACION_CARRERA_CIRCUITO;
    hud.segundosAlerta = 10.0f;

    bool alguienEnMeta = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        if (!h.visible) continue;

        const EstadoJugadorCircuitoVoltaje& j = jugadores[i];
        int vuelta = VueltaActualCircuito(j);
        if (j.llegoMeta) alguienEnMeta = true;

        std::snprintf(h.valor, sizeof(h.valor), "V%d/%d", vuelta, (int)VUELTAS_CIRCUITO);
        h.valorNumerico = vuelta;
        h.progreso = Limitar01Circuito(j.velocidad / 1.08f);

        if (j.llegoMeta)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡EN META!");
        }
        else if (j.tiempoTrompo > 0.0f)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡TROMPO!");
        }
        else if (ExcedidoCircuito(*this, i))
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡MUY RÁPIDO!");
        }

        if (j.llegoMeta)
        {
            std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "¡4 VUELTAS!");
        }
        else
        {
            int centesimas = (int)(Clamp(j.avance, 0.0f, VUELTAS_CIRCUITO) * 100.0f);
            std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d,%02d VUELTAS", centesimas / 100, centesimas % 100);
        }

        if (j.trompos > 0)
        {
            std::snprintf(h.detalleDesempate, sizeof(h.detalleDesempate), "%d TROMPO%s", j.trompos, j.trompos == 1 ? "" : "S");
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "MANTENÉ %s: ACELERÁ\nSOLTÁ EN LAS CURVAS",
                TextoAcelerarCircuito(participantes[i])
            );
        }
    }

    hud.finPorTiempo = !alguienEnMeta;

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoCircuitoVoltaje::ObtenerResultado() const
{
    return resultado;
}
