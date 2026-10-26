#include "Minigames/MinijuegoMiradasCruzadas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "Systems/CalidadGrafica.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_MIRADAS = 2.5f;
static const float DURACION_ELECCION_MIRADAS = 1.65f;
static const float DURACION_RESOLUCION_MIRADAS = 1.15f;
static const int MAX_RONDAS_MIRADAS = 5;


// Solo visual (definida con el dibujo, mas abajo).
static void ActualizarVisualMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    float deltaTime,
    const Participante participantes[]
);


static const char* NombreDireccionMirada(
    DireccionMiradaCruzada direccion
)
{
    switch (direccion)
    {
        case MIRADA_FRENTE: return "AL FRENTE";
        case MIRADA_IZQUIERDA: return "A LA IZQUIERDA";
        case MIRADA_DERECHA: return "A LA DERECHA";
        case MIRADA_ARRIBA: return "ARRIBA";
        case MIRADA_ABAJO: return "ABAJO";
        case CANTIDAD_DIRECCIONES_MIRADA: break;
    }

    return "?";
}


static int ContarEquipoVivoMiradas(
    const MinijuegoMiradasCruzadas& minijuego
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static bool LeerDireccionMiradaHumana(
    const Participante& participante,
    DireccionMiradaCruzada& direccion
)
{
    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(participante);

    if (entrada.izquierda)
    {
        direccion = MIRADA_IZQUIERDA;
        return true;
    }

    if (entrada.derecha)
    {
        direccion = MIRADA_DERECHA;
        return true;
    }

    if (entrada.arriba)
    {
        direccion = MIRADA_ARRIBA;
        return true;
    }

    if (entrada.abajo)
    {
        direccion = MIRADA_ABAJO;
        return true;
    }

    return false;
}


static void PrepararRondaMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    const Participante participantes[]
)
{
    minijuego.numeroRonda++;
    minijuego.fase = FASE_MIRADAS_ELECCION;
    minijuego.tiempoFase = DURACION_ELECCION_MIRADAS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        minijuego.jugadores[i].direccion = MIRADA_FRENTE;
        minijuego.jugadores[i].eligioDireccion = false;

        if (
            (participantes[i].esBot || !participantes[i].conectado) &&
            !minijuego.jugadores[i].eliminado
        )
        {
            ReiniciarBotMiradas(
                minijuego.bots[i],
                0.32f,
                1.05f
            );
        }
    }
}


static void FinalizarMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    bool ganaSolo
)
{
    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;

        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego =
            esSolo
            ? 3 - ContarEquipoVivoMiradas(minijuego)
            : (!minijuego.jugadores[i].eliminado ? 1 : 0);
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_MIRADAS_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void ResolverRondaMiradas(
    MinijuegoMiradasCruzadas& minijuego
)
{
    DireccionMiradaCruzada direccionSolo =
        minijuego.jugadores[minijuego.indiceSolo].direccion;

    int vivosAntes = ContarEquipoVivoMiradas(minijuego);
    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            minijuego.jugadores[i].eliminado
        )
        {
            continue;
        }

        if (minijuego.jugadores[i].direccion == direccionSolo)
        {
            minijuego.jugadores[i].eliminado = true;
            minijuego.jugadores[i].rondaEliminacion = minijuego.numeroRonda;
            eliminados++;
        }
    }

    ReproducirSonidoMinijuego(
        minijuego.audio,
        eliminados > 0 ? SONIDO_ELIMINADO : SONIDO_ACIERTO
    );

    int posicion = vivosAntes - eliminados + 1;
    if (posicion < 2) posicion = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.jugadores[i].eliminado &&
            minijuego.jugadores[i].posicionFinal == 0
        )
        {
            minijuego.jugadores[i].posicionFinal = posicion;
        }
    }

    if (ContarEquipoVivoMiradas(minijuego) <= 0)
    {
        FinalizarMiradas(minijuego, true);
        return;
    }

    if (minijuego.numeroRonda >= MAX_RONDAS_MIRADAS)
    {
        FinalizarMiradas(minijuego, false);
        return;
    }

    minijuego.fase = FASE_MIRADAS_RESOLUCION;
    minijuego.tiempoFase = DURACION_RESOLUCION_MIRADAS;
}


void MinijuegoMiradasCruzadas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
        bots[i] = {};
    }

    fase = FASE_MIRADAS_PREPARACION;
    indiceSolo = -1;
    numeroRonda = 0;
    tiempoPreparacion = DURACION_PREPARACION_MIRADAS;
    tiempoFase = 0.0f;

    // Solo visual.
    tiempoAnimacion = 0.0f;
    semillaVisual = 0x2545F491u;
    rondaRevelada = 0;
    tiempoRevelado = 0.0f;
    confetiLanzado = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        visual[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_MIRADAS; i++)
    {
        particulas[i] = {};
    }
}


void MinijuegoMiradasCruzadas::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();

    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    int indices[MAX_PARTICIPANTES]{};
    int cantidad =
        ObtenerIndicesParticipantesActivos(
            participantes,
            indices,
            MAX_PARTICIPANTES
        );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_MIRADAS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        resultado.participantes[i].numeroEquipo =
            i == indiceSolo ? 0 : 1;
    }
}


void MinijuegoMiradasCruzadas::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    ActualizarVisualMiradas(*this, deltaTime, participantes);

    if (
        fase == FASE_MIRADAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_MIRADAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            PrepararRondaMiradas(*this, participantes);
        }

        return;
    }

    if (fase == FASE_MIRADAS_ELECCION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                !resultado.participantes[i].participo ||
                (i != indiceSolo && jugadores[i].eliminado)
            )
            {
                continue;
            }

            if (participantes[i].esBot || !participantes[i].conectado)
            {
                if (
                    ActualizarDecisionBotMiradas(
                        bots[i],
                        deltaTime,
                        CANTIDAD_DIRECCIONES_MIRADA
                    )
                )
                {
                    jugadores[i].direccion =
                        (DireccionMiradaCruzada)bots[i].direccionElegida;
                    jugadores[i].eligioDireccion = true;
                }

                continue;
            }

            if (!participantes[i].conectado)
            {
                continue;
            }

            DireccionMiradaCruzada nueva = jugadores[i].direccion;

            if (LeerDireccionMiradaHumana(participantes[i], nueva))
            {
                jugadores[i].direccion = nueva;
                jugadores[i].eligioDireccion = true;
                ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
            }
        }

        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            ResolverRondaMiradas(*this);
        }

        return;
    }

    if (fase == FASE_MIRADAS_RESOLUCION)
    {
        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            PrepararRondaMiradas(*this, participantes);
        }
    }
}


//==================================================
// VISUAL: TEATRO DE TITERES EN UNA FERIA DE CAMPO
//==================================================
//
// Tablado de madera al atardecer, con telon bordo recogido a los lados,
// guirnaldas de banderines y bombillas y un arcoiris pintado de utileria.
// El solitario maneja su cabeza-titere (con corona) desde el palco del
// fondo; el equipo, cada uno sobre su trampilla, al frente. Las cabezas
// giran hacia la direccion elegida (izquierda / derecha son las de la
// pantalla) recien al resolver la ronda; a quien coincide se le abre la
// trampilla. Alrededor: carpas a rayas, fardos, calabazas, faroles, el
// carro de los titeres, torres de focos, colinas en tres planos y un
// molino. Nada de esto toca la logica.
//
// MODELO FUTURO: cabezas-titere con animacion de giro, palco, arcoiris
// de utileria, telon con lambrequin, carpas, molino, granero, carro de
// titeres, torres de focos, fardos, calabazas y faroles.
//==================================================


static const float Y_TABLADO_MIRADAS = 1.0f;
static const float X_TABLADO_MIRADAS = 8.2f;
static const float Z_FONDO_TABLADO_MIRADAS = -7.6f;
static const float Z_FRENTE_TABLADO_MIRADAS = 4.4f;
static const float Z_SOLO_MIRADAS = -4.5f;
static const float Z_EQUIPO_MIRADAS = 1.8f;            // el del medio, un poco mas adelante
static const float Y_PALCO_MIRADAS = 3.9f;
static const float ALTURA_CABEZA_SOLO_MIRADAS = 2.6f;  // sobre los pies
static const float ALTURA_CABEZA_EQUIPO_MIRADAS = 2.35f;
static const float RADIO_CABEZA_SOLO_MIRADAS = 1.1f;
static const float RADIO_CABEZA_EQUIPO_MIRADAS = 0.92f;
static const float FONDO_VARA_MIRADAS = 0.42f;         // vara detras del jugador
static const float Z_ARCOIRIS_MIRADAS = -7.0f;
static const float X_POSTE_TELON_MIRADAS = 8.6f;
static const float Z_TELON_MIRADAS = -7.35f;
static const float ESPERA_TRAMPILLA_MIRADAS = 0.45f;   // se ve el giro antes de caer
static const float DURACION_CAIDA_MIRADAS = 0.5f;

static const Vector3 LUZ_MIRADAS = { -0.58f, 0.70f, 0.42f };   // sol bajo a la izquierda
static const Color COLOR_BRUMA_MIRADAS = { 244, 198, 160, 255 };
static const Color COLOR_MADERA_MIRADAS = { 184, 132, 84, 255 };
static const Color COLOR_MADERA_OSCURA_MIRADAS = { 110, 72, 46, 255 };
static const Color COLOR_TELON_MIRADAS = { 136, 30, 50, 255 };
static const Color COLOR_DORADO_MIRADAS = { 232, 182, 76, 255 };
static const Color COLOR_LUZ_CALIDA_MIRADAS = { 255, 214, 150, 255 };

static const Vector3 POSICION_FOCOS_MIRADAS[2] =
{
    { -10.0f, 7.4f, -1.0f },
    { 10.0f, 7.4f, -1.0f }
};

static const Vector3 FAROLES_MIRADAS[4] =
{
    { -2.05f, Y_PALCO_MIRADAS + 1.25f, -3.05f },
    { 2.05f, Y_PALCO_MIRADAS + 1.25f, -3.05f },
    { -7.9f, Y_TABLADO_MIRADAS + 2.1f, 3.9f },
    { 7.9f, Y_TABLADO_MIRADAS + 2.1f, 3.9f }
};

struct TramoGuirnaldaMiradas
{
    Vector3 a;
    Vector3 b;
    float caida;
    int cantidad;
};

// Bombillas: de las torres de focos al telon y a las carpas.
static const TramoGuirnaldaMiradas BOMBILLAS_MIRADAS[6] =
{
    { { -10.0f, 7.5f, -1.0f }, { -8.6f, 10.0f, -7.3f }, 0.7f, 7 },
    { { 10.0f, 7.5f, -1.0f }, { 8.6f, 10.0f, -7.3f }, 0.7f, 7 },
    { { -10.0f, 7.5f, -1.0f }, { -16.5f, 8.2f, -12.0f }, 1.2f, 9 },
    { { 10.0f, 7.5f, -1.0f }, { 16.5f, 8.4f, -13.0f }, 1.2f, 9 },
    { { -16.5f, 8.2f, -12.0f }, { -25.0f, 9.0f, -22.0f }, 1.4f, 9 },
    { { 16.5f, 8.4f, -13.0f }, { 25.0f, 9.6f, -24.0f }, 1.4f, 9 }
};


//==================================================
// UTILIDADES VISUALES
//==================================================

static float Limitar01Miradas(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float HashMiradas(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 1013904223u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue que sortea
// al solitario ni el de los bots.
static float AzarMiradas(MinijuegoMiradasCruzadas& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static float EaseOutBackMiradas(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    float u = t - 1.0f;
    return 1.0f + 2.70158f * u * u * u + 1.70158f * u * u;
}


static Color TonoMiradas(Color color, float factor)
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


static Color MezclaMiradas(Color a, Color b, float t)
{
    Color c = ColorLerp(a, b, Limitar01Miradas(t));
    c.a = a.a;
    return c;
}


// Niebla de color: lo lejano se funde con la bruma calida del atardecer.
static Color NieblaMiradas(Color color, float z)
{
    return MezclaMiradas(color, COLOR_BRUMA_MIRADAS, Limitar01Miradas((-z - 12.0f) / 100.0f) * 0.86f);
}


// Luz falsa por normal: arriba ~1.0, frente ~0.78, izquierda ~0.84,
// derecha y fondo ~0.56, abajo ~0.5.
static Color LuzMiradas(Color color, Vector3 n)
{
    float d = n.x * LUZ_MIRADAS.x + n.y * LUZ_MIRADAS.y + n.z * LUZ_MIRADAS.z;
    return TonoMiradas(color, 0.56f + 0.50f * (d > 0.0f ? d : 0.0f) + 0.07f * n.y);
}


static void VerticeMiradas(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


// Marco local: rollo (Z), luego cabeceo (X), luego giro (Y), escala y
// traslacion. Sirve para las cabezas-titere y las piezas que giran.
struct MarcoMiradas
{
    Vector3 origen = { 0.0f, 0.0f, 0.0f };
    float escala = 1.0f;
    float cosGiro = 1.0f, senGiro = 0.0f;
    float cosCabeceo = 1.0f, senCabeceo = 0.0f;
    float cosRollo = 1.0f, senRollo = 0.0f;
};

static const MarcoMiradas MARCO_MUNDO_MIRADAS{};


static MarcoMiradas CrearMarcoMiradas(Vector3 origen, float giro, float cabeceo, float rollo, float escala)
{
    MarcoMiradas m;
    m.origen = origen;
    m.escala = escala;
    m.cosGiro = std::cos(giro * DEG2RAD);
    m.senGiro = std::sin(giro * DEG2RAD);
    m.cosCabeceo = std::cos(cabeceo * DEG2RAD);
    m.senCabeceo = std::sin(cabeceo * DEG2RAD);
    m.cosRollo = std::cos(rollo * DEG2RAD);
    m.senRollo = std::sin(rollo * DEG2RAD);
    return m;
}


static Vector3 RotarMarcoMiradas(const MarcoMiradas& m, Vector3 v)
{
    float x = v.x * m.cosRollo - v.y * m.senRollo;
    float y = v.x * m.senRollo + v.y * m.cosRollo;
    float z = v.z;

    float y1 = y * m.cosCabeceo - z * m.senCabeceo;
    float z1 = y * m.senCabeceo + z * m.cosCabeceo;

    return { x * m.cosGiro + z1 * m.senGiro, y1, -x * m.senGiro + z1 * m.cosGiro };
}


static Vector3 PuntoMarcoMiradas(const MarcoMiradas& m, Vector3 v)
{
    Vector3 r = RotarMarcoMiradas(m, { v.x * m.escala, v.y * m.escala, v.z * m.escala });
    return { m.origen.x + r.x, m.origen.y + r.y, m.origen.z + r.z };
}


// Caja con caras sombreadas por la luz falsa (respeta el marco).
static void CajaMarcoMiradas(const MarcoMiradas& m, Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    auto cara = [&](Vector3 n, Vector3 a, Vector3 b, Vector3 d, Vector3 e)
    {
        Color tono = LuzMiradas(color, RotarMarcoMiradas(m, n));
        rlColor4ub(tono.r, tono.g, tono.b, tono.a);
        Vector3 p;
        p = PuntoMarcoMiradas(m, a); rlVertex3f(p.x, p.y, p.z);
        p = PuntoMarcoMiradas(m, b); rlVertex3f(p.x, p.y, p.z);
        p = PuntoMarcoMiradas(m, d); rlVertex3f(p.x, p.y, p.z);
        p = PuntoMarcoMiradas(m, e); rlVertex3f(p.x, p.y, p.z);
    };

    rlBegin(RL_QUADS);
    cara({ 0.0f, 1.0f, 0.0f }, { x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 });
    cara({ 0.0f, -1.0f, 0.0f }, { x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 });
    cara({ 0.0f, 0.0f, 1.0f }, { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 });
    cara({ 0.0f, 0.0f, -1.0f }, { x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 });
    cara({ 1.0f, 0.0f, 0.0f }, { x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 });
    cara({ -1.0f, 0.0f, 0.0f }, { x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 });
    rlEnd();
}


static void CajaMiradas(Vector3 c, float ancho, float alto, float fondo, Color color)
{
    CajaMarcoMiradas(MARCO_MUNDO_MIRADAS, c, { ancho, alto, fondo }, color);
}


// Esfera facetada (low poly) con sombreado por cara.
static void EsferaMarcoMiradas(
    const MarcoMiradas& m,
    Vector3 c,
    Vector3 radios,
    Color color,
    int anillos,
    int gajos
)
{
    rlBegin(RL_QUADS);

    for (int a = 0; a < anillos; a++)
    {
        float la0 = -PI * 0.5f + PI * (float)a / (float)anillos;
        float la1 = -PI * 0.5f + PI * (float)(a + 1) / (float)anillos;
        float lam = (la0 + la1) * 0.5f;

        for (int g = 0; g < gajos; g++)
        {
            float lo0 = 2.0f * PI * (float)g / (float)gajos;
            float lo1 = 2.0f * PI * (float)(g + 1) / (float)gajos;
            float lom = (lo0 + lo1) * 0.5f;

            Vector3 n = { std::cos(lam) * std::sin(lom), std::sin(lam), std::cos(lam) * std::cos(lom) };
            Color tono = LuzMiradas(color, RotarMarcoMiradas(m, n));
            rlColor4ub(tono.r, tono.g, tono.b, tono.a);

            auto vertice = [&](float la, float lo)
            {
                Vector3 p = PuntoMarcoMiradas(m, {
                    c.x + std::cos(la) * std::sin(lo) * radios.x,
                    c.y + std::sin(la) * radios.y,
                    c.z + std::cos(la) * std::cos(lo) * radios.z
                });
                rlVertex3f(p.x, p.y, p.z);
            };

            vertice(la0, lo0);
            vertice(la0, lo1);
            vertice(la1, lo1);
            vertice(la1, lo0);
        }
    }

    rlEnd();
}


static void EsferaMiradas(Vector3 c, float radio, Color color, int anillos = 6, int gajos = 8)
{
    EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, c, { radio, radio, radio }, color, anillos, gajos);
}


// Cilindro / cono vertical (eje Y local) con franjas opcionales.
static void CilindroMarcoMiradas(
    const MarcoMiradas& m,
    Vector3 base,
    float radioAbajo,
    float radioArriba,
    float alto,
    int lados,
    Color color,
    Color colorFranja,
    int franjas
)
{
    float pendiente = alto > 0.001f ? (radioAbajo - radioArriba) / alto : 0.0f;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = (a0 + a1) * 0.5f;

        Color base0 = franjas > 0 && ((k * franjas * 2) / lados) % 2 == 1 ? colorFranja : color;
        Vector3 n = Vector3Normalize({ std::sin(am), pendiente, std::cos(am) });
        Color tono = LuzMiradas(base0, RotarMarcoMiradas(m, n));
        rlColor4ub(tono.r, tono.g, tono.b, tono.a);

        Vector3 p;
        p = PuntoMarcoMiradas(m, { base.x + std::sin(a0) * radioAbajo, base.y, base.z + std::cos(a0) * radioAbajo });
        rlVertex3f(p.x, p.y, p.z);
        p = PuntoMarcoMiradas(m, { base.x + std::sin(a1) * radioAbajo, base.y, base.z + std::cos(a1) * radioAbajo });
        rlVertex3f(p.x, p.y, p.z);
        p = PuntoMarcoMiradas(m, { base.x + std::sin(a1) * radioArriba, base.y + alto, base.z + std::cos(a1) * radioArriba });
        rlVertex3f(p.x, p.y, p.z);
        p = PuntoMarcoMiradas(m, { base.x + std::sin(a0) * radioArriba, base.y + alto, base.z + std::cos(a0) * radioArriba });
        rlVertex3f(p.x, p.y, p.z);
    }

    if (radioArriba > 0.001f)
    {
        Color tapa = LuzMiradas(color, RotarMarcoMiradas(m, { 0.0f, 1.0f, 0.0f }));
        rlColor4ub(tapa.r, tapa.g, tapa.b, tapa.a);
        Vector3 centro = PuntoMarcoMiradas(m, { base.x, base.y + alto, base.z });

        for (int k = 0; k < lados; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)lados;
            float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            Vector3 p0 = PuntoMarcoMiradas(m, { base.x + std::sin(a0) * radioArriba, base.y + alto, base.z + std::cos(a0) * radioArriba });
            Vector3 p1 = PuntoMarcoMiradas(m, { base.x + std::sin(a1) * radioArriba, base.y + alto, base.z + std::cos(a1) * radioArriba });
            rlVertex3f(centro.x, centro.y, centro.z);
            rlVertex3f(p0.x, p0.y, p0.z);
            rlVertex3f(p1.x, p1.y, p1.z);
            rlVertex3f(p1.x, p1.y, p1.z);
        }
    }

    rlEnd();
}


static void CilindroMiradas(Vector3 base, float radioAbajo, float radioArriba, float alto, int lados, Color color)
{
    CilindroMarcoMiradas(MARCO_MUNDO_MIRADAS, base, radioAbajo, radioArriba, alto, lados, color, color, 0);
}


// Cilindro entre dos puntos (varas, postes inclinados, riendas).
static void CilindroEntreMiradas(Vector3 a, Vector3 b, float radioA, float radioB, int lados, Color color)
{
    Vector3 d = Vector3Subtract(b, a);
    if (Vector3Length(d) < 0.0001f) return;
    d = Vector3Normalize(d);

    Vector3 referencia = std::fabs(d.y) < 0.95f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(d, referencia));
    Vector3 v = Vector3CrossProduct(d, u);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = (a0 + a1) * 0.5f;

        Vector3 n = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));
        Color tono = LuzMiradas(color, n);
        rlColor4ub(tono.r, tono.g, tono.b, tono.a);

        Vector3 o0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 o1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));

        Vector3 p;
        p = Vector3Add(a, Vector3Scale(o0, radioA)); rlVertex3f(p.x, p.y, p.z);
        p = Vector3Add(a, Vector3Scale(o1, radioA)); rlVertex3f(p.x, p.y, p.z);
        p = Vector3Add(b, Vector3Scale(o1, radioB)); rlVertex3f(p.x, p.y, p.z);
        p = Vector3Add(b, Vector3Scale(o0, radioB)); rlVertex3f(p.x, p.y, p.z);
    }

    rlEnd();
}


// Disco de luz con borde que se apaga (u y v definen el plano).
static void ResplandorMiradas(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloMiradas(float x, float y, float z, float radio, Color color)
{
    ResplandorMiradas({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void ResplandorFrenteMiradas(Vector3 centro, float radio, Color color)
{
    ResplandorMiradas(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
}


// Anillo plano en el suelo (r0..r1).
static void AnilloMiradas(float x, float y, float z, float r0, float r1, Color color)
{
    const int LADOS = 28;
    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;

        rlVertex3f(x + std::cos(a0) * r1, y, z + std::sin(a0) * r1);
        rlVertex3f(x + std::cos(a0) * r0, y, z + std::sin(a0) * r0);
        rlVertex3f(x + std::cos(a1) * r0, y, z + std::sin(a1) * r0);
        rlVertex3f(x + std::cos(a1) * r1, y, z + std::sin(a1) * r1);
    }

    rlEnd();
}


static Vector3 PuntoCatenariaMiradas(Vector3 a, Vector3 b, float caida, float u)
{
    return {
        a.x + (b.x - a.x) * u,
        a.y + (b.y - a.y) * u - caida * 4.0f * u * (1.0f - u),
        a.z + (b.z - a.z) * u
    };
}


//==================================================
// POSICIONES DE LOS TITERES
//==================================================

static int CantidadEquipoMiradas(const MinijuegoMiradasCruzadas& minijuego)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (i != minijuego.indiceSolo && minijuego.resultado.participantes[i].participo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int RanuraEquipoMiradas(const MinijuegoMiradasCruzadas& minijuego, int indice)
{
    int ranura = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (i == minijuego.indiceSolo || !minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        if (i == indice)
        {
            return ranura;
        }

        ranura++;
    }

    return -1;
}


// Pies del jugador: el solitario en el palco, el equipo en sus trampillas.
static Vector3 PiesJugadorMiradas(const MinijuegoMiradasCruzadas& minijuego, int indice)
{
    if (indice == minijuego.indiceSolo)
    {
        return { 0.0f, Y_PALCO_MIRADAS, Z_SOLO_MIRADAS };
    }

    // En arco: el del medio adelanta para no tapar al solitario.
    int ranura = RanuraEquipoMiradas(minijuego, indice);
    int cantidad = CantidadEquipoMiradas(minijuego);
    float x = 0.0f;
    float z = Z_EQUIPO_MIRADAS + 0.6f;

    if (cantidad == 2)
    {
        x = ranura == 0 ? -2.6f : 2.6f;
        z = Z_EQUIPO_MIRADAS + 0.3f;
    }
    else if (cantidad >= 3)
    {
        int r = ranura < 0 ? 0 : (ranura > 2 ? 2 : ranura);
        x = -4.6f + 4.6f * (float)r;
        z = r == 1 ? Z_EQUIPO_MIRADAS + 0.6f : Z_EQUIPO_MIRADAS;
    }

    return { x, Y_TABLADO_MIRADAS, z };
}


static float RadioCabezaMiradas(const MinijuegoMiradasCruzadas& minijuego, int indice)
{
    return indice == minijuego.indiceSolo ? RADIO_CABEZA_SOLO_MIRADAS : RADIO_CABEZA_EQUIPO_MIRADAS;
}


static Vector3 CentroCabezaMiradas(const MinijuegoMiradasCruzadas& minijuego, int indice)
{
    Vector3 pies = PiesJugadorMiradas(minijuego, indice);
    float altura = indice == minijuego.indiceSolo ? ALTURA_CABEZA_SOLO_MIRADAS : ALTURA_CABEZA_EQUIPO_MIRADAS;
    return { pies.x, pies.y + altura, pies.z - FONDO_VARA_MIRADAS };
}


// Giro de la cabeza (grados) para cada direccion de la pantalla.
static void AngulosMiradaCruzada(
    DireccionMiradaCruzada direccion,
    float& giro,
    float& cabeceo
)
{
    giro = 0.0f;
    cabeceo = 0.0f;

    switch (direccion)
    {
        case MIRADA_IZQUIERDA: giro = -55.0f; break;
        case MIRADA_DERECHA: giro = 55.0f; break;
        case MIRADA_ARRIBA: cabeceo = -35.0f; break;
        case MIRADA_ABAJO: cabeceo = 35.0f; break;
        case MIRADA_FRENTE:
        case CANTIDAD_DIRECCIONES_MIRADA:
            break;
    }
}


static bool RevelandoMiradas(const MinijuegoMiradasCruzadas& minijuego)
{
    return minijuego.fase == FASE_MIRADAS_RESOLUCION || minijuego.fase == FASE_MIRADAS_TERMINADO;
}


//==================================================
// PARTICULAS Y ESTADO VISUAL
//==================================================

static int CantidadParticulasMiradas(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static void EmitirParticulaMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_MIRADAS; i++)
    {
        ParticulaMiradas& p = minijuego.particulas[i];

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


static void EmitirChispasMiradas(MinijuegoMiradasCruzadas& minijuego, Vector3 centro, Color color, int base)
{
    int cantidad = CantidadParticulasMiradas(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarMiradas(minijuego, 0.0f, 2.0f * PI);
        float r = AzarMiradas(minijuego, 1.0f, 2.6f);
        Color c = k % 3 == 0 ? Color{ 255, 246, 206, 255 } : color;

        EmitirParticulaMiradas(
            minijuego,
            centro,
            { std::cos(a) * r, AzarMiradas(minijuego, 1.6f, 3.8f), std::sin(a) * r * 0.6f },
            AzarMiradas(minijuego, 0.45f, 0.8f),
            AzarMiradas(minijuego, 0.07f, 0.13f),
            6.5f,
            c
        );
    }
}


// Polvo y astillas que salen de la trampilla.
static void EmitirPolvoMiradas(MinijuegoMiradasCruzadas& minijuego, Vector3 centro, int base)
{
    int cantidad = CantidadParticulasMiradas(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarMiradas(minijuego, 0.0f, 2.0f * PI);
        float r = AzarMiradas(minijuego, 0.6f, 1.8f);
        Color c = k % 3 == 0 ? Color{ 150, 112, 76, 255 } : Color{ 196, 178, 150, 255 };

        EmitirParticulaMiradas(
            minijuego,
            { centro.x + std::cos(a) * 0.5f, centro.y + 0.1f, centro.z + std::sin(a) * 0.5f },
            { std::cos(a) * r, AzarMiradas(minijuego, 1.0f, 2.8f), std::sin(a) * r },
            AzarMiradas(minijuego, 0.5f, 0.9f),
            AzarMiradas(minijuego, 0.10f, 0.2f),
            3.5f,
            c
        );
    }
}


static void EmitirConfetiMiradas(MinijuegoMiradasCruzadas& minijuego, Vector3 centro, float ancho, int base)
{
    const Color colores[6] =
    {
        { 255, 214, 84, 255 }, { 236, 116, 72, 255 }, { 104, 176, 214, 255 },
        { 236, 96, 140, 255 }, { 120, 206, 128, 255 }, { 250, 246, 236, 255 }
    };

    int cantidad = CantidadParticulasMiradas(base);

    for (int k = 0; k < cantidad; k++)
    {
        EmitirParticulaMiradas(
            minijuego,
            {
                centro.x + AzarMiradas(minijuego, -ancho, ancho),
                centro.y + AzarMiradas(minijuego, 1.8f, 3.6f),
                centro.z + AzarMiradas(minijuego, -1.0f, 1.0f)
            },
            {
                AzarMiradas(minijuego, -0.9f, 0.9f),
                AzarMiradas(minijuego, 0.0f, 1.6f),
                AzarMiradas(minijuego, -0.6f, 0.6f)
            },
            AzarMiradas(minijuego, 2.4f, 3.8f),
            AzarMiradas(minijuego, 0.07f, 0.12f),
            1.5f,
            colores[k % 6]
        );
    }
}


static void ActualizarVisualMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    float deltaTime,
    const Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICULAS_MIRADAS; i++)
    {
        ParticulaMiradas& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f || p.posicion.y < 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        float freno = std::exp(-1.4f * deltaTime);
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, deltaTime));
    }

    if (minijuego.indiceSolo < 0)
    {
        return;
    }

    bool revelando = RevelandoMiradas(minijuego);
    minijuego.tiempoRevelado += deltaTime;

    // Se acaba de resolver una ronda: quien coincidio queda atrapado, el
    // resto se salva con un destello de su color.
    if (
        revelando &&
        minijuego.numeroRonda > 0 &&
        minijuego.rondaRevelada != minijuego.numeroRonda
    )
    {
        minijuego.rondaRevelada = minijuego.numeroRonda;
        minijuego.tiempoRevelado = 0.0f;
        int atrapados = 0;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (i == minijuego.indiceSolo || !minijuego.resultado.participantes[i].participo)
            {
                continue;
            }

            const EstadoJugadorMiradasCruzadas& jugador = minijuego.jugadores[i];

            if (jugador.eliminado && jugador.rondaEliminacion == minijuego.numeroRonda)
            {
                minijuego.visual[i].tiempoAtrapado = 0.0f;
                minijuego.visual[i].cayo = false;
                atrapados++;
            }
            else if (!jugador.eliminado)
            {
                minijuego.visual[i].destello = 1.0f;
                Vector3 cabeza = CentroCabezaMiradas(minijuego, i);
                EmitirChispasMiradas(minijuego, cabeza, participantes[i].color, 10);
            }
        }

        if (atrapados > 0)
        {
            minijuego.visual[minijuego.indiceSolo].destello = 1.0f;
            EmitirChispasMiradas(minijuego, CentroCabezaMiradas(minijuego, minijuego.indiceSolo), COLOR_DORADO_MIRADAS, 16);
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        VisualJugadorMiradas& v = minijuego.visual[i];
        const EstadoJugadorMiradasCruzadas& jugador = minijuego.jugadores[i];

        float objetivoGiro = 0.0f;
        float objetivoCabeceo = 0.0f;

        if (revelando)
        {
            AngulosMiradaCruzada(jugador.direccion, objetivoGiro, objetivoCabeceo);
        }

        // Resorte con un poco de rebote: se lee como un titere que gira.
        v.velocidadGiro += (objetivoGiro - v.giro) * 260.0f * deltaTime;
        v.velocidadGiro *= std::exp(-17.0f * deltaTime);
        v.giro += v.velocidadGiro * deltaTime;
        v.velocidadCabeceo += (objetivoCabeceo - v.cabeceo) * 260.0f * deltaTime;
        v.velocidadCabeceo *= std::exp(-17.0f * deltaTime);
        v.cabeceo += v.velocidadCabeceo * deltaTime;

        // Eligio: el farolito se enciende (igual para cualquier direccion).
        bool eligio = minijuego.fase == FASE_MIRADAS_ELECCION && jugador.eligioDireccion;
        if (eligio && !v.eligioAntes)
        {
            v.pulsoListo = 1.0f;
        }
        v.eligioAntes = eligio;

        v.pulsoListo = std::fmax(0.0f, v.pulsoListo - deltaTime * 3.2f);
        v.destello = std::fmax(0.0f, v.destello - deltaTime * 1.3f);

        if (v.tiempoAtrapado >= 0.0f)
        {
            float antes = v.tiempoAtrapado;
            v.tiempoAtrapado += deltaTime;
            Vector3 pies = PiesJugadorMiradas(minijuego, i);

            if (!v.cayo && v.tiempoAtrapado >= ESPERA_TRAMPILLA_MIRADAS)
            {
                v.cayo = true;
                EmitirPolvoMiradas(minijuego, pies, 18);
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
            }

            float cierre = ESPERA_TRAMPILLA_MIRADAS + DURACION_CAIDA_MIRADAS;
            if (antes < cierre && v.tiempoAtrapado >= cierre)
            {
                EmitirPolvoMiradas(minijuego, pies, 8);
            }
        }
    }

    // Confeti sobre quien gano, despues de ver la ultima trampilla.
    if (
        minijuego.fase == FASE_MIRADAS_TERMINADO &&
        minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        !minijuego.confetiLanzado &&
        minijuego.tiempoRevelado > 0.9f
    )
    {
        minijuego.confetiLanzado = true;

        if (minijuego.resultado.participantes[minijuego.indiceSolo].posicionFinal == 1)
        {
            EmitirConfetiMiradas(minijuego, CentroCabezaMiradas(minijuego, minijuego.indiceSolo), 2.6f, 110);
        }
        else
        {
            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (
                    i != minijuego.indiceSolo &&
                    minijuego.resultado.participantes[i].participo &&
                    !minijuego.jugadores[i].eliminado
                )
                {
                    EmitirConfetiMiradas(minijuego, CentroCabezaMiradas(minijuego, i), 1.6f, 55);
                }
            }
        }
    }
}


//==================================================
// CIELO, HORIZONTE Y SUELO
//==================================================

// Bollo de nube: arriba casi blanco, abajo rosado por el sol bajo.
static void EsferaNubeMiradas(Vector3 c, Vector3 radios, Color luz, Color sombra)
{
    const int ANILLOS = 4;
    const int GAJOS = 9;

    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        float la0 = -PI * 0.5f + PI * (float)a / (float)ANILLOS;
        float la1 = -PI * 0.5f + PI * (float)(a + 1) / (float)ANILLOS;
        float lam = (la0 + la1) * 0.5f;

        for (int g = 0; g < GAJOS; g++)
        {
            float lo0 = 2.0f * PI * (float)g / (float)GAJOS;
            float lo1 = 2.0f * PI * (float)(g + 1) / (float)GAJOS;
            float lom = (lo0 + lo1) * 0.5f;
            float nx = std::cos(lam) * std::sin(lom);
            float ny = std::sin(lam);
            Color tono = MezclaMiradas(sombra, luz, 0.55f + 0.45f * ny - 0.25f * nx);
            rlColor4ub(tono.r, tono.g, tono.b, 255);

            float las[4] = { la0, la0, la1, la1 };
            float los[4] = { lo0, lo1, lo1, lo0 };

            for (int k = 0; k < 4; k++)
            {
                rlVertex3f(
                    c.x + std::cos(las[k]) * std::sin(los[k]) * radios.x,
                    c.y + std::sin(las[k]) * radios.y,
                    c.z + std::cos(las[k]) * std::cos(los[k]) * radios.z
                );
            }
        }
    }

    rlEnd();
}


static void DibujarCieloMiradas(float t)
{
    // Atardecer: oro en el horizonte, durazno, lavanda y azul arriba.
    const float z = -175.0f;
    const float ys[5] = { -40.0f, 9.0f, 17.0f, 30.0f, 120.0f };
    const Color cs[5] =
    {
        { 252, 210, 152, 255 },
        { 252, 204, 150, 255 },
        { 242, 166, 148, 255 },
        { 168, 140, 190, 255 },
        { 84, 96, 168, 255 }
    };

    rlBegin(RL_QUADS);
    for (int k = 0; k < 4; k++)
    {
        VerticeMiradas({ -420.0f, ys[k], z }, cs[k]);
        VerticeMiradas({ 420.0f, ys[k], z }, cs[k]);
        VerticeMiradas({ 420.0f, ys[k + 1], z }, cs[k + 1]);
        VerticeMiradas({ -420.0f, ys[k + 1], z }, cs[k + 1]);
    }
    rlEnd();

    // Sol bajo, a la izquierda, detras de las carpas.
    Vector3 sol = { -112.0f, 12.0f, z + 3.0f };
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < 24; k++)
    {
        float a0 = 2.0f * PI * (float)k / 24.0f;
        float a1 = 2.0f * PI * (float)(k + 1) / 24.0f;
        VerticeMiradas(sol, Color{ 255, 250, 228, 255 });
        VerticeMiradas({ sol.x + std::cos(a0) * 6.5f, sol.y + std::sin(a0) * 6.5f, sol.z }, Color{ 255, 228, 168, 255 });
        VerticeMiradas({ sol.x + std::cos(a1) * 6.5f, sol.y + std::sin(a1) * 6.5f, sol.z }, Color{ 255, 228, 168, 255 });
    }
    rlEnd();

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    // Nubes de tarde: racimos achatados que derivan despacio.
    int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 9 : 5;

    for (int n = 0; n < cantidad; n++)
    {
        float base = -190.0f + (float)n * 44.0f;
        float x = std::fmod(base + t * (0.7f + 0.2f * (float)(n % 3)) + 380.0f, 380.0f) - 190.0f;
        float y = 11.0f + 6.0f * HashMiradas(n, 3);
        float zn = -150.0f + 14.0f * HashMiradas(n, 7);
        Color luz = MezclaMiradas(Color{ 255, 246, 236, 255 }, Color{ 255, 230, 214, 255 }, HashMiradas(n, 11));
        Color sombra = MezclaMiradas(Color{ 236, 168, 168, 255 }, Color{ 206, 160, 190, 255 }, HashMiradas(n, 12));

        for (int b = 0; b < 5; b++)
        {
            float dx = ((float)b - 2.0f) * 4.0f;
            float r = 3.4f + 2.6f * HashMiradas(n, b + 20) - std::fabs((float)b - 2.0f) * 0.7f;
            EsferaNubeMiradas({ x + dx, y + r * 0.2f, zn }, { r, r * 0.42f, r * 0.55f }, luz, sombra);
        }
    }

    // Bandada lejana que cruza el cielo.
    if (CalidadDecoracion() == CALIDAD_ALTA)
    {
        rlBegin(RL_TRIANGLES);
        for (int k = 0; k < 6; k++)
        {
            float x = std::fmod(-70.0f + t * 2.2f + (float)k * 2.3f + 140.0f, 140.0f) - 70.0f;
            float y = 9.6f + 0.8f * HashMiradas(k, 4) + 0.15f * std::sin(t * 1.3f + (float)k);
            float zb = -55.0f - 3.0f * HashMiradas(k, 9);
            float ala = 0.35f * std::sin(t * 7.0f + (float)k * 1.7f);
            Color ave = NieblaMiradas(Color{ 70, 52, 70, 255 }, zb);

            VerticeMiradas({ x - 0.55f, y + ala, zb }, ave);
            VerticeMiradas({ x, y - 0.05f, zb }, ave);
            VerticeMiradas({ x, y + 0.08f, zb }, ave);
            VerticeMiradas({ x + 0.55f, y + ala, zb }, ave);
            VerticeMiradas({ x, y + 0.08f, zb }, ave);
            VerticeMiradas({ x, y - 0.05f, zb }, ave);
        }
        rlEnd();
    }
}


static float AlturaColinaMiradas(int capa, float x)
{
    switch (capa)
    {
        case 0: return 6.2f + 2.8f * std::sin(x * 0.021f + 0.7f) + 1.6f * std::sin(x * 0.057f + 2.1f);
        case 1: return 4.0f + 2.2f * std::sin(x * 0.034f + 1.9f) + 1.1f * std::sin(x * 0.09f + 0.4f);
        default: break;
    }

    return 2.2f + 1.6f * std::sin(x * 0.05f + 0.3f) + 0.7f * std::sin(x * 0.13f + 1.2f);
}


static void DibujarHorizonteMiradas(float t)
{
    const float zs[3] = { -118.0f, -78.0f, -46.0f };
    const Color colores[3] =
    {
        { 176, 140, 170, 255 },     // lejanas, lavanda por la bruma
        { 132, 146, 104, 255 },     // medias, oliva
        { 108, 146, 74, 255 }       // cercanas, verde de campo
    };

    bool baja = CalidadDecoracion() == CALIDAD_BAJA;

    for (int capa = baja ? 1 : 0; capa < 3; capa++)
    {
        float z = zs[capa];
        float paso = capa == 0 ? 8.0f : 5.0f;
        Color arriba = NieblaMiradas(TonoMiradas(colores[capa], 1.08f), z);
        Color abajo = NieblaMiradas(TonoMiradas(colores[capa], 0.82f), z);

        rlBegin(RL_QUADS);
        for (float x = -280.0f; x < 280.0f; x += paso)
        {
            float h0 = AlturaColinaMiradas(capa, x);
            float h1 = AlturaColinaMiradas(capa, x + paso);
            VerticeMiradas({ x, -1.0f, z }, abajo);
            VerticeMiradas({ x + paso, -1.0f, z }, abajo);
            VerticeMiradas({ x + paso, h1, z }, arriba);
            VerticeMiradas({ x, h0, z }, arriba);
        }
        rlEnd();

        // Setos que dibujan parcelas sobre la colina cercana.
        if (capa == 2 && !baja)
        {
            Color seto = NieblaMiradas(Color{ 78, 112, 58, 255 }, z + 1.0f);
            rlBegin(RL_QUADS);
            for (float x = -120.0f; x < 120.0f; x += 4.0f)
            {
                float h0 = AlturaColinaMiradas(capa, x) * 0.55f;
                float h1 = AlturaColinaMiradas(capa, x + 4.0f) * 0.55f;
                VerticeMiradas({ x, h0 - 0.12f, z + 0.3f }, seto);
                VerticeMiradas({ x + 4.0f, h1 - 0.12f, z + 0.3f }, seto);
                VerticeMiradas({ x + 4.0f, h1 + 0.12f, z + 0.3f }, seto);
                VerticeMiradas({ x, h0 + 0.12f, z + 0.3f }, seto);
            }
            rlEnd();
        }
    }

    if (baja)
    {
        return;
    }

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Arboles sobre la cresta de la colina cercana.
    for (int k = 0; k < 30; k += paso)
    {
        float x = -130.0f + (float)k * 9.0f + 5.0f * HashMiradas(k, 2);
        if (std::fabs(x) < 14.0f) continue;     // detras del telon no se ven

        float z = zs[2] + 1.0f + 2.0f * HashMiradas(k, 3);
        float y = AlturaColinaMiradas(2, x) - 0.4f;
        float r = 1.3f + 0.9f * HashMiradas(k, 4);
        Color copa = NieblaMiradas(HashMiradas(k, 5) < 0.25f ? Color{ 176, 150, 68, 255 } : Color{ 74, 110, 56, 255 }, z);

        CilindroMiradas({ x, y - 0.6f, z }, 0.22f, 0.18f, 1.0f, 5, NieblaMiradas(Color{ 92, 66, 48, 255 }, z));
        EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, { x, y + r * 0.9f, z }, { r, r * 0.95f, r }, copa, 4, 7);
    }

    // Granero a la izquierda (MODELO FUTURO: granero).
    {
        float z = -36.0f;
        Color pared = NieblaMiradas(Color{ 156, 82, 62, 255 }, z);
        Color techo = NieblaMiradas(Color{ 96, 70, 66, 255 }, z);
        Color blanco = NieblaMiradas(Color{ 236, 226, 210, 255 }, z);
        CajaMiradas({ -31.0f, 2.5f, z }, 8.0f, 5.0f, 6.0f, pared);

        rlBegin(RL_QUADS);
        Color techoLuz = LuzMiradas(techo, Vector3Normalize({ -0.6f, 0.8f, 0.0f }));
        Color techoSombra = LuzMiradas(techo, Vector3Normalize({ 0.6f, 0.8f, 0.0f }));
        VerticeMiradas({ -35.4f, 4.9f, z + 3.3f }, techoLuz);
        VerticeMiradas({ -31.0f, 8.0f, z + 3.3f }, techoLuz);
        VerticeMiradas({ -31.0f, 8.0f, z - 3.3f }, techoLuz);
        VerticeMiradas({ -35.4f, 4.9f, z - 3.3f }, techoLuz);
        VerticeMiradas({ -26.6f, 4.9f, z + 3.3f }, techoSombra);
        VerticeMiradas({ -31.0f, 8.0f, z + 3.3f }, techoSombra);
        VerticeMiradas({ -31.0f, 8.0f, z - 3.3f }, techoSombra);
        VerticeMiradas({ -26.6f, 4.9f, z - 3.3f }, techoSombra);
        rlEnd();

        rlBegin(RL_TRIANGLES);
        Color hastial = TonoMiradas(pared, 0.8f);
        VerticeMiradas({ -35.0f, 5.0f, z + 3.01f }, hastial);
        VerticeMiradas({ -27.0f, 5.0f, z + 3.01f }, hastial);
        VerticeMiradas({ -31.0f, 7.8f, z + 3.01f }, hastial);
        rlEnd();

        CajaMiradas({ -31.0f, 1.6f, z + 3.05f }, 2.8f, 3.2f, 0.08f, TonoMiradas(pared, 0.7f));
        CajaMiradas({ -31.0f, 3.25f, z + 3.1f }, 3.1f, 0.18f, 0.08f, blanco);
        CajaMiradas({ -32.5f, 1.6f, z + 3.1f }, 0.18f, 3.3f, 0.08f, blanco);
        CajaMiradas({ -29.5f, 1.6f, z + 3.1f }, 0.18f, 3.3f, 0.08f, blanco);
    }

    // Molino a la derecha: las aspas giran sin parar.
    // (MODELO FUTURO: molino de viento.)
    {
        Vector3 base = { 26.0f, 0.0f, -40.0f };
        Color muro = NieblaMiradas(Color{ 232, 218, 196, 255 }, base.z);
        Color teja = NieblaMiradas(Color{ 120, 82, 64, 255 }, base.z);
        Color aspa = NieblaMiradas(Color{ 214, 196, 170, 255 }, base.z);

        CilindroMiradas(base, 1.7f, 1.05f, 6.0f, 8, muro);
        CilindroMiradas({ base.x, 6.0f, base.z }, 1.3f, 0.0f, 1.5f, 8, teja);
        CajaMiradas({ base.x, 1.0f, base.z + 1.62f }, 0.8f, 1.8f, 0.1f, teja);

        MarcoMiradas aspas = CrearMarcoMiradas({ base.x, 6.2f, base.z + 1.4f }, 0.0f, 0.0f, t * 40.0f, 1.0f);
        CajaMarcoMiradas(aspas, { 0.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 0.4f }, teja);

        for (int k = 0; k < 4; k++)
        {
            MarcoMiradas brazo = CrearMarcoMiradas(aspas.origen, 0.0f, 0.0f, t * 40.0f + 90.0f * (float)k, 1.0f);
            CajaMarcoMiradas(brazo, { 0.0f, 2.0f, 0.1f }, { 0.12f, 4.0f, 0.08f }, teja);
            CajaMarcoMiradas(brazo, { 0.32f, 2.4f, 0.12f }, { 0.55f, 2.8f, 0.04f }, aspa);
        }
    }
}


static void DibujarSueloMiradas()
{
    // Pasto en parcelas con variacion de tono y niebla hacia el fondo.
    rlBegin(RL_QUADS);
    for (int fila = 0; fila < 14; fila++)
    {
        float z0 = -48.0f + (float)fila * 5.0f;
        float z1 = z0 + 5.0f;

        for (int col = 0; col < 30; col++)
        {
            float x0 = -90.0f + (float)col * 6.0f;
            float x1 = x0 + 6.0f;
            float tono = 0.92f + 0.14f * HashMiradas(col, fila + 40);
            Color pasto = NieblaMiradas(TonoMiradas(Color{ 120, 158, 78, 255 }, tono), (z0 + z1) * 0.5f);

            VerticeMiradas({ x0, 0.0f, z1 }, pasto);
            VerticeMiradas({ x1, 0.0f, z1 }, pasto);
            VerticeMiradas({ x1, 0.0f, z0 }, pasto);
            VerticeMiradas({ x0, 0.0f, z0 }, pasto);
        }
    }
    rlEnd();

    // Tierra pisada frente al tablado y alrededor (bordes irregulares).
    rlBegin(RL_QUADS);
    for (int fila = 0; fila < 16; fila++)
    {
        float z0 = -12.0f + (float)fila * 2.0f;

        for (int col = 0; col < 14; col++)
        {
            float x0 = -14.0f + (float)col * 2.0f;
            float borde = std::fabs(x0 + 1.0f) / 14.0f;
            if (HashMiradas(col, fila + 90) < borde * borde * 1.4f - 0.2f) continue;

            float tono = 0.9f + 0.16f * HashMiradas(col, fila + 70);
            Color tierra = TonoMiradas(Color{ 180, 146, 104, 255 }, tono);
            VerticeMiradas({ x0, 0.01f, z0 + 2.0f }, tierra);
            VerticeMiradas({ x0 + 2.0f, 0.01f, z0 + 2.0f }, tierra);
            VerticeMiradas({ x0 + 2.0f, 0.01f, z0 }, tierra);
            VerticeMiradas({ x0, 0.01f, z0 }, tierra);
        }
    }
    rlEnd();
}


//==================================================
// FERIA: CARPAS, CERCAS Y UTILERIA DEL SUELO
//==================================================

// Carpa a rayas con faldon festoneado y banderin que flamea.
// (MODELO FUTURO: carpa de feria.)
static void DibujarCarpaMiradas(Vector3 base, float radio, float alto, Color a, Color b, float t, int semilla)
{
    Color ca = NieblaMiradas(a, base.z);
    Color cb = NieblaMiradas(b, base.z);
    float pared = alto * 0.5f;

    CilindroMarcoMiradas(MARCO_MUNDO_MIRADAS, base, radio, radio, pared, 16, ca, cb, 4);
    CilindroMarcoMiradas(MARCO_MUNDO_MIRADAS, { base.x, base.y + pared, base.z }, radio * 1.18f, 0.0f, alto - pared, 16, ca, cb, 4);

    // Faldon festoneado al pie del techo.
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < 16; k++)
    {
        float a0 = 2.0f * PI * (float)k / 16.0f;
        float a1 = 2.0f * PI * (float)(k + 1) / 16.0f;
        float am = (a0 + a1) * 0.5f;
        float r = radio * 1.19f;
        Color c = LuzMiradas(k % 2 == 0 ? cb : ca, { std::sin(am), 0.0f, std::cos(am) });
        VerticeMiradas({ base.x + std::sin(a0) * r, base.y + pared + 0.02f, base.z + std::cos(a0) * r }, c);
        VerticeMiradas({ base.x + std::sin(a1) * r, base.y + pared + 0.02f, base.z + std::cos(a1) * r }, c);
        VerticeMiradas({ base.x + std::sin(am) * r, base.y + pared - 0.5f, base.z + std::cos(am) * r }, c);
    }
    rlEnd();

    // Entrada en sombra.
    CajaMiradas({ base.x, base.y + pared * 0.42f, base.z + radio - 0.05f }, radio * 0.55f, pared * 0.84f, 0.12f, NieblaMiradas(Color{ 70, 46, 44, 255 }, base.z));

    // Mastil y banderin.
    Vector3 tope = { base.x, base.y + alto, base.z };
    CilindroEntreMiradas(tope, { tope.x, tope.y + 1.3f, tope.z }, 0.05f, 0.04f, 5, NieblaMiradas(Color{ 120, 90, 62, 255 }, base.z));

    Color tela = NieblaMiradas(semilla % 2 == 0 ? COLOR_DORADO_MIRADAS : Color{ 240, 236, 224, 255 }, base.z);
    rlBegin(RL_QUADS);
    for (int s = 0; s < 4; s++)
    {
        float u0 = (float)s / 4.0f;
        float u1 = (float)(s + 1) / 4.0f;
        float o0 = std::sin(t * 5.0f + u0 * 4.0f + (float)semilla) * 0.18f * u0;
        float o1 = std::sin(t * 5.0f + u1 * 4.0f + (float)semilla) * 0.18f * u1;
        float h0 = 0.34f * (1.0f - u0 * 0.6f);
        float h1 = 0.34f * (1.0f - u1 * 0.6f);
        Color c = TonoMiradas(tela, 0.82f + 0.18f * std::cos(t * 5.0f + u0 * 4.0f));
        VerticeMiradas({ tope.x + u0 * 1.1f, tope.y + 1.25f - h0, tope.z + o0 }, c);
        VerticeMiradas({ tope.x + u1 * 1.1f, tope.y + 1.25f - h1, tope.z + o1 }, c);
        VerticeMiradas({ tope.x + u1 * 1.1f, tope.y + 1.25f, tope.z + o1 }, c);
        VerticeMiradas({ tope.x + u0 * 1.1f, tope.y + 1.25f, tope.z + o0 }, c);
    }
    rlEnd();
}


static void DibujarFardoMiradas(Vector3 c, float giro, int semilla)
{
    MarcoMiradas m = CrearMarcoMiradas(c, giro, 0.0f, 0.0f, 1.0f);
    Color paja = TonoMiradas(Color{ 220, 186, 104, 255 }, 0.92f + 0.12f * HashMiradas(semilla, 1));
    CajaMarcoMiradas(m, { 0.0f, 0.36f, 0.0f }, { 1.4f, 0.72f, 0.8f }, paja);
    CajaMarcoMiradas(m, { -0.35f, 0.36f, 0.0f }, { 0.06f, 0.74f, 0.82f }, Color{ 150, 112, 64, 255 });
    CajaMarcoMiradas(m, { 0.35f, 0.36f, 0.0f }, { 0.06f, 0.74f, 0.82f }, Color{ 150, 112, 64, 255 });
    CajaMarcoMiradas(m, { 0.1f, 0.73f, 0.12f }, { 0.5f, 0.04f, 0.08f }, TonoMiradas(paja, 1.12f));
}


static void DibujarCalabazaMiradas(Vector3 c, float r, int semilla)
{
    Color naranja = TonoMiradas(Color{ 228, 128, 48, 255 }, 0.92f + 0.14f * HashMiradas(semilla, 2));
    EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, { c.x, c.y + r * 0.7f, c.z }, { r, r * 0.72f, r * 0.9f }, naranja, 5, 10);
    EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, { c.x - r * 0.42f, c.y + r * 0.68f, c.z }, { r * 0.62f, r * 0.68f, r * 0.82f }, TonoMiradas(naranja, 0.94f), 5, 8);
    EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, { c.x + r * 0.42f, c.y + r * 0.68f, c.z }, { r * 0.62f, r * 0.68f, r * 0.82f }, TonoMiradas(naranja, 0.94f), 5, 8);
    CilindroEntreMiradas({ c.x, c.y + r * 1.3f, c.z }, { c.x + r * 0.12f, c.y + r * 1.62f, c.z }, r * 0.1f, r * 0.07f, 5, Color{ 96, 110, 52, 255 });
}


static void DibujarBarrilMiradas(Vector3 base, float alto, float radio)
{
    Color madera = Color{ 150, 100, 62, 255 };
    Color aro = Color{ 72, 66, 64, 255 };
    CilindroMiradas(base, radio * 0.86f, radio, alto * 0.5f, 12, madera);
    CilindroMiradas({ base.x, base.y + alto * 0.5f, base.z }, radio, radio * 0.86f, alto * 0.5f, 12, madera);
    CilindroMiradas({ base.x, base.y + alto * 0.14f, base.z }, radio * 0.93f, radio * 0.95f, 0.07f, 12, aro);
    CilindroMiradas({ base.x, base.y + alto * 0.80f, base.z }, radio * 0.95f, radio * 0.93f, 0.07f, 12, aro);
}


static void DibujarCajonMiradas(Vector3 c, float lado, int semilla)
{
    Color madera = TonoMiradas(Color{ 176, 130, 82, 255 }, 0.9f + 0.14f * HashMiradas(semilla, 3));
    CajaMiradas({ c.x, c.y + lado * 0.5f, c.z }, lado, lado, lado, madera);
    Color liston = TonoMiradas(madera, 0.75f);
    CajaMiradas({ c.x, c.y + lado * 0.5f, c.z + lado * 0.51f }, lado * 0.96f, 0.08f, 0.02f, liston);
    CajaMiradas({ c.x, c.y + lado * 0.2f, c.z + lado * 0.51f }, lado * 0.96f, 0.08f, 0.02f, liston);
    CajaMiradas({ c.x, c.y + lado * 0.8f, c.z + lado * 0.51f }, lado * 0.96f, 0.08f, 0.02f, liston);
}


// Farol con poste y vidrio calido (el halo va en las luces).
static void DibujarFarolMiradas(Vector3 luz, float altoPoste)
{
    Color hierro = Color{ 58, 52, 56, 255 };
    CilindroEntreMiradas({ luz.x, luz.y - altoPoste, luz.z }, { luz.x, luz.y - 0.25f, luz.z }, 0.06f, 0.05f, 6, Color{ 96, 66, 46, 255 });
    CajaMiradas({ luz.x, luz.y - 0.22f, luz.z }, 0.34f, 0.06f, 0.34f, hierro);
    CajaMiradas(luz, 0.26f, 0.32f, 0.26f, COLOR_LUZ_CALIDA_MIRADAS);
    CilindroMiradas({ luz.x, luz.y + 0.16f, luz.z }, 0.24f, 0.0f, 0.2f, 4, hierro);
}


// Carro de los titiriteros, con toldo y ruedas (MODELO FUTURO: carro).
static void DibujarCarroMiradas(Vector3 base)
{
    Color pintura = Color{ 62, 124, 126, 255 };
    Color madera = COLOR_MADERA_OSCURA_MIRADAS;

    CajaMiradas({ base.x, base.y + 1.05f, base.z }, 1.9f, 1.1f, 3.4f, pintura);
    CajaMiradas({ base.x, base.y + 1.05f, base.z + 1.71f }, 1.4f, 0.7f, 0.02f, TonoMiradas(COLOR_DORADO_MIRADAS, 0.9f));
    CajaMiradas({ base.x, base.y + 0.45f, base.z }, 1.5f, 0.14f, 3.0f, madera);

    // Toldo a rayas (dos aguas).
    for (int k = 0; k < 6; k++)
    {
        float z0 = base.z - 1.7f + (float)k * (3.4f / 6.0f);
        Color tela = k % 2 == 0 ? Color{ 240, 232, 214, 255 } : Color{ 196, 92, 74, 255 };
        Color luz = LuzMiradas(tela, Vector3Normalize({ -0.6f, 0.8f, 0.0f }));
        Color sombra = LuzMiradas(tela, Vector3Normalize({ 0.6f, 0.8f, 0.0f }));

        rlBegin(RL_QUADS);
        VerticeMiradas({ base.x - 1.05f, base.y + 1.6f, z0 }, luz);
        VerticeMiradas({ base.x - 1.05f, base.y + 1.6f, z0 + 3.4f / 6.0f }, luz);
        VerticeMiradas({ base.x, base.y + 2.4f, z0 + 3.4f / 6.0f }, luz);
        VerticeMiradas({ base.x, base.y + 2.4f, z0 }, luz);
        VerticeMiradas({ base.x + 1.05f, base.y + 1.6f, z0 }, sombra);
        VerticeMiradas({ base.x + 1.05f, base.y + 1.6f, z0 + 3.4f / 6.0f }, sombra);
        VerticeMiradas({ base.x, base.y + 2.4f, z0 + 3.4f / 6.0f }, sombra);
        VerticeMiradas({ base.x, base.y + 2.4f, z0 }, sombra);
        rlEnd();
    }

    // Ruedas con rayos.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int eje = -1; eje <= 1; eje += 2)
        {
            Vector3 centro = { base.x + (float)lado * 1.0f, base.y + 0.55f, base.z + (float)eje * 1.1f };
            MarcoMiradas rueda = CrearMarcoMiradas(centro, 0.0f, 0.0f, 90.0f, 1.0f);
            CilindroMarcoMiradas(rueda, { 0.0f, -0.06f, 0.0f }, 0.55f, 0.55f, 0.12f, 14, madera, madera, 0);

            for (int r = 0; r < 4; r++)
            {
                MarcoMiradas rayo = CrearMarcoMiradas(centro, 90.0f, 45.0f * (float)r, 0.0f, 1.0f);
                CajaMarcoMiradas(rayo, { 0.0f, 0.0f, 0.0f }, { 0.07f, 0.95f, 0.07f }, TonoMiradas(COLOR_MADERA_MIRADAS, 0.9f));
            }

            CajaMiradas({ centro.x + (float)lado * 0.08f, centro.y, centro.z }, 0.1f, 0.16f, 0.16f, Color{ 70, 64, 62, 255 });
        }
    }
}


// Torre de focos de madera con el reflector apuntando al palco.
static void DibujarTorreFocoMiradas(int lado)
{
    Vector3 foco = POSICION_FOCOS_MIRADAS[lado];
    Color madera = COLOR_MADERA_OSCURA_MIRADAS;

    for (int k = 0; k < 4; k++)
    {
        float sx = (k & 1) ? 0.7f : -0.7f;
        float sz = (k & 2) ? 0.7f : -0.7f;
        CilindroEntreMiradas(
            { foco.x + sx, 0.0f, foco.z + sz },
            { foco.x + sx * 0.4f, foco.y - 0.6f, foco.z + sz * 0.4f },
            0.08f, 0.06f, 5, madera
        );
    }

    for (int nivel = 1; nivel <= 3; nivel++)
    {
        float y = (foco.y - 0.6f) * (float)nivel / 3.6f;
        float s = 0.7f - 0.3f * y / (foco.y - 0.6f);
        CajaMiradas({ foco.x, y, foco.z + s }, s * 2.0f, 0.08f, 0.08f, madera);
        CajaMiradas({ foco.x, y, foco.z - s }, s * 2.0f, 0.08f, 0.08f, madera);
    }

    CajaMiradas({ foco.x, foco.y - 0.55f, foco.z }, 0.9f, 0.12f, 0.9f, madera);

    Vector3 objetivo = { 0.0f, Y_PALCO_MIRADAS + 1.8f, Z_SOLO_MIRADAS };
    Vector3 dir = Vector3Normalize(Vector3Subtract(objetivo, foco));
    Color metal = Color{ 64, 60, 66, 255 };
    CilindroEntreMiradas(Vector3Subtract(foco, Vector3Scale(dir, 0.45f)), Vector3Add(foco, Vector3Scale(dir, 0.3f)), 0.26f, 0.36f, 10, metal);
    EsferaMiradas(Vector3Add(foco, Vector3Scale(dir, 0.3f)), 0.27f, Color{ 255, 240, 200, 255 }, 4, 8);
}


static void DibujarGuirnaldaBanderinesMiradas(Vector3 a, Vector3 b, float caida, int cantidad, float t, int semilla)
{
    static const Color PALETA[5] =
    {
        { 236, 200, 110, 255 }, { 244, 238, 224, 255 }, { 96, 170, 164, 255 },
        { 212, 112, 92, 255 }, { 160, 118, 176, 255 }
    };

    Color cuerda = Color{ 86, 62, 44, 255 };

    for (int s = 0; s < 16; s++)
    {
        DrawLine3D(
            PuntoCatenariaMiradas(a, b, caida, (float)s / 16.0f),
            PuntoCatenariaMiradas(a, b, caida, (float)(s + 1) / 16.0f),
            cuerda
        );
    }

    Vector3 dir = Vector3Normalize(Vector3Subtract(b, a));

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < cantidad; k++)
    {
        float u = ((float)k + 0.5f) / (float)cantidad;
        Vector3 p = PuntoCatenariaMiradas(a, b, caida, u);
        float vaiven = std::sin(t * 2.2f + (float)k * 0.9f + (float)semilla) * 0.12f;
        Color c = TonoMiradas(PALETA[(k + semilla) % 5], 0.86f + 0.14f * std::cos(t * 2.2f + (float)k * 0.9f));

        VerticeMiradas(Vector3Subtract(p, Vector3Scale(dir, 0.22f)), c);
        VerticeMiradas(Vector3Add(p, Vector3Scale(dir, 0.22f)), c);
        VerticeMiradas({ p.x, p.y - 0.46f, p.z + vaiven }, TonoMiradas(c, 0.86f));
    }
    rlEnd();
}


static void DibujarGuirnaldasBombillasMiradas()
{
    Color cuerda = Color{ 52, 44, 40, 255 };

    for (const TramoGuirnaldaMiradas& tramo : BOMBILLAS_MIRADAS)
    {
        for (int s = 0; s < 16; s++)
        {
            DrawLine3D(
                PuntoCatenariaMiradas(tramo.a, tramo.b, tramo.caida, (float)s / 16.0f),
                PuntoCatenariaMiradas(tramo.a, tramo.b, tramo.caida, (float)(s + 1) / 16.0f),
                cuerda
            );
        }

        for (int k = 0; k < tramo.cantidad; k++)
        {
            Vector3 p = PuntoCatenariaMiradas(tramo.a, tramo.b, tramo.caida, ((float)k + 0.5f) / (float)tramo.cantidad);
            Color c = k % 3 == 0 ? Color{ 255, 206, 150, 255 } : Color{ 255, 240, 196, 255 };
            DrawSphereEx({ p.x, p.y - 0.1f, p.z }, 0.09f, 4, 6, c);
        }
    }
}


static void DibujarCercaMiradas(float x, float z0, float z1)
{
    Color madera = Color{ 150, 112, 76, 255 };

    for (float z = z0; z <= z1 + 0.01f; z += 2.0f)
    {
        CajaMiradas({ x, 0.55f, z }, 0.16f, 1.1f, 0.16f, NieblaMiradas(madera, z));
    }

    CajaMiradas({ x, 0.75f, (z0 + z1) * 0.5f }, 0.08f, 0.12f, z1 - z0, NieblaMiradas(madera, (z0 + z1) * 0.5f));
    CajaMiradas({ x, 0.4f, (z0 + z1) * 0.5f }, 0.08f, 0.12f, z1 - z0, NieblaMiradas(madera, (z0 + z1) * 0.5f));
}


static void DibujarFeriaMiradas(float t)
{
    NivelCalidadGrafica decoracion = CalidadDecoracion();

    // Torres de focos: son parte de la luz del escenario, siempre.
    DibujarTorreFocoMiradas(0);
    DibujarTorreFocoMiradas(1);

    if (decoracion == CALIDAD_BAJA)
    {
        return;
    }

    bool alta = decoracion == CALIDAD_ALTA;

    // Carpas (media distancia).
    DibujarCarpaMiradas({ -16.5f, 0.0f, -12.0f }, 3.3f, 7.0f, Color{ 204, 98, 76, 255 }, Color{ 244, 234, 214, 255 }, t, 0);
    DibujarCarpaMiradas({ 16.5f, 0.0f, -13.0f }, 3.5f, 7.2f, Color{ 150, 84, 136, 255 }, Color{ 244, 234, 214, 255 }, t, 1);

    if (alta)
    {
        DibujarCarpaMiradas({ -25.0f, 0.0f, -22.0f }, 3.9f, 7.8f, Color{ 70, 140, 140, 255 }, Color{ 244, 234, 214, 255 }, t, 2);
        DibujarCarpaMiradas({ 25.0f, 0.0f, -24.0f }, 4.1f, 8.4f, Color{ 206, 144, 64, 255 }, Color{ 244, 234, 214, 255 }, t, 3);
    }

    DibujarGuirnaldasBombillasMiradas();

    // Cercas de la feria a los costados.
    DibujarCercaMiradas(-13.5f, -16.0f, 2.0f);
    DibujarCercaMiradas(13.5f, -16.0f, 2.0f);

    // Utileria del suelo (anillo cercano).
    DibujarFardoMiradas({ -10.0f, 0.0f, 2.4f }, 12.0f, 1);
    DibujarFardoMiradas({ -10.2f, 0.0f, 0.9f }, -8.0f, 2);
    DibujarFardoMiradas({ -10.1f, 0.72f, 1.65f }, 4.0f, 3);
    DibujarCalabazaMiradas({ -9.1f, 0.0f, 3.8f }, 0.42f, 1);
    DibujarCalabazaMiradas({ -9.9f, 0.0f, 4.3f }, 0.3f, 2);

    DibujarCajonMiradas({ 10.3f, 0.0f, 2.2f }, 0.9f, 1);
    DibujarCajonMiradas({ 10.2f, 0.9f, 2.25f }, 0.7f, 2);
    DibujarCajonMiradas({ 11.1f, 0.0f, 1.3f }, 0.8f, 3);
    DibujarCalabazaMiradas({ 9.3f, 0.0f, 3.9f }, 0.46f, 3);
    DibujarCalabazaMiradas({ 10.1f, 0.0f, 4.4f }, 0.28f, 4);

    if (alta)
    {
        DibujarCarroMiradas({ -11.6f, 0.0f, -5.0f });
        DibujarBarrilMiradas({ -10.0f, 0.0f, -9.0f }, 1.1f, 0.5f);
        DibujarBarrilMiradas({ -11.0f, 0.0f, -9.6f }, 1.1f, 0.5f);
        DibujarBarrilMiradas({ 11.0f, 0.0f, -3.2f }, 1.1f, 0.5f);
        DibujarFardoMiradas({ 11.6f, 0.0f, -7.6f }, 30.0f, 4);
        DibujarFardoMiradas({ 11.4f, 0.72f, -7.5f }, 22.0f, 5);
        DibujarCalabazaMiradas({ 10.4f, 0.0f, -6.2f }, 0.5f, 5);
    }
}


//==================================================
// EL TEATRO: TABLADO, PALCO, ARCOIRIS Y TELON
//==================================================

static void DibujarTabladoMiradas(float t)
{
    const float x0 = -X_TABLADO_MIRADAS;
    const float x1 = X_TABLADO_MIRADAS;
    const float z0 = Z_FONDO_TABLADO_MIRADAS;
    const float z1 = Z_FRENTE_TABLADO_MIRADAS;
    const float y = Y_TABLADO_MIRADAS;

    // Estructura bajo los tablones.
    CajaMiradas({ 0.0f, y * 0.47f, (z0 + z1) * 0.5f }, x1 - x0 - 0.1f, y * 0.94f, z1 - z0 - 0.1f, Color{ 84, 58, 40, 255 });

    // Tablones a lo largo, con juntas desfasadas y tono propio.
    int cantidad = 30;
    float ancho = (x1 - x0) / (float)cantidad;

    for (int k = 0; k < cantidad; k++)
    {
        float cx = x0 + ancho * ((float)k + 0.5f);
        float corte = z0 + (z1 - z0) * (0.25f + 0.5f * HashMiradas(k, 5));

        for (int p = 0; p < 2; p++)
        {
            float a = p == 0 ? z0 : corte + 0.03f;
            float b = p == 0 ? corte - 0.03f : z1;
            float tono = 0.88f + 0.2f * HashMiradas(k, 11 + p);
            CajaMiradas({ cx, y - 0.03f, (a + b) * 0.5f }, ancho - 0.05f, 0.06f, b - a, TonoMiradas(COLOR_MADERA_MIRADAS, tono));
        }
    }

    // Bordes con grosor.
    Color borde = Color{ 128, 84, 52, 255 };
    CajaMiradas({ 0.0f, y - 0.02f, z1 - 0.06f }, x1 - x0 + 0.3f, 0.2f, 0.28f, borde);
    CajaMiradas({ x0 - 0.02f, y - 0.02f, (z0 + z1) * 0.5f }, 0.28f, 0.2f, z1 - z0, borde);
    CajaMiradas({ x1 + 0.02f, y - 0.02f, (z0 + z1) * 0.5f }, 0.28f, 0.2f, z1 - z0, borde);

    // Frente: tablas verticales.
    for (int k = 0; k < 28; k++)
    {
        float x = x0 + (x1 - x0) * ((float)k + 0.5f) / 28.0f;
        CajaMiradas({ x, y * 0.45f, z1 + 0.03f }, (x1 - x0) / 28.0f - 0.04f, y * 0.9f, 0.06f, TonoMiradas(Color{ 120, 80, 50, 255 }, 0.9f + 0.16f * HashMiradas(k, 33)));
    }

    // Faldon de festones bordo con ribete dorado y borlas.
    const int FESTONES = 9;
    float anchoFeston = (x1 - x0) / (float)FESTONES;

    for (int f = 0; f < FESTONES; f++)
    {
        float xa = x0 + anchoFeston * (float)f;
        float xb = xa + anchoFeston;
        float zf = z1 + 0.1f;

        for (int capa = 0; capa < 2; capa++)
        {
            float extra = capa == 0 ? 0.08f : 0.0f;
            Color c = capa == 0 ? COLOR_DORADO_MIRADAS : TonoMiradas(COLOR_TELON_MIRADAS, 0.95f);
            float z = zf + (float)capa * 0.02f;

            rlBegin(RL_TRIANGLES);
            for (int s = 0; s < 8; s++)
            {
                float u0 = (float)s / 8.0f;
                float u1 = (float)(s + 1) / 8.0f;
                float h0 = 0.4f * std::sin(u0 * PI) + extra;
                float h1 = 0.4f * std::sin(u1 * PI) + extra;
                Color tono = TonoMiradas(c, 0.78f + 0.2f * std::sin((u0 + u1) * 0.5f * PI));
                VerticeMiradas({ xa + (xb - xa) * u0, y - 0.1f, z }, tono);
                VerticeMiradas({ xa + (xb - xa) * u0, y - 0.1f - h0, z }, tono);
                VerticeMiradas({ xa + (xb - xa) * u1, y - 0.1f - h1, z }, tono);
                VerticeMiradas({ xa + (xb - xa) * u0, y - 0.1f, z }, tono);
                VerticeMiradas({ xa + (xb - xa) * u1, y - 0.1f - h1, z }, tono);
                VerticeMiradas({ xa + (xb - xa) * u1, y - 0.1f, z }, tono);
            }
            rlEnd();
        }

        EsferaMiradas({ xb, y - 0.2f, zf + 0.06f }, 0.07f, COLOR_DORADO_MIRADAS, 3, 6);
        CilindroMiradas({ xb, y - 0.52f, zf + 0.06f }, 0.08f, 0.02f, 0.28f, 6, COLOR_DORADO_MIRADAS);
    }

    // Candilejas: lamparitas con pantalla de lata en el borde.
    for (int k = 0; k < 8; k++)
    {
        float x = -7.0f + 2.0f * (float)k;
        CajaMiradas({ x, y + 0.13f, z1 - 0.12f }, 0.5f, 0.26f, 0.08f, Color{ 150, 150, 160, 255 });
        CajaMiradas({ x, y + 0.02f, z1 - 0.22f }, 0.5f, 0.04f, 0.2f, Color{ 120, 120, 130, 255 });
        EsferaMiradas({ x, y + 0.12f, z1 - 0.26f }, 0.1f, Color{ 255, 240, 200, 255 }, 3, 6);
    }

    (void)t;
}


// Trampilla bajo un jugador del equipo: se abre en dos hojas cuando lo
// atrapan y se vuelve a cerrar.
static void DibujarTrampillaMiradas(Vector3 pies, float apertura, Color colorJugador, bool eliminado)
{
    const float lado = 1.5f;
    float y = Y_TABLADO_MIRADAS;

    // Anillo pintado del color del jugador.
    AnilloMiradas(pies.x, y + 0.012f, pies.z, 1.0f, 1.16f, eliminado ? MezclaMiradas(colorJugador, Color{ 120, 100, 90, 255 }, 0.6f) : colorJugador);

    // Marco de la trampilla.
    Color marco = Color{ 92, 62, 40, 255 };
    CajaMiradas({ pies.x, y + 0.006f, pies.z - lado * 0.5f }, lado + 0.1f, 0.02f, 0.06f, marco);
    CajaMiradas({ pies.x, y + 0.006f, pies.z + lado * 0.5f }, lado + 0.1f, 0.02f, 0.06f, marco);
    CajaMiradas({ pies.x - lado * 0.5f, y + 0.006f, pies.z }, 0.06f, 0.02f, lado, marco);
    CajaMiradas({ pies.x + lado * 0.5f, y + 0.006f, pies.z }, 0.06f, 0.02f, lado, marco);

    if (apertura <= 0.01f)
    {
        // Argolla y bisagras.
        CajaMiradas({ pies.x, y + 0.012f, pies.z + lado * 0.36f }, 0.22f, 0.02f, 0.06f, Color{ 70, 66, 64, 255 });
        CajaMiradas({ pies.x - lado * 0.42f, y + 0.012f, pies.z }, 0.16f, 0.02f, 0.3f, Color{ 70, 66, 64, 255 });
        CajaMiradas({ pies.x + lado * 0.42f, y + 0.012f, pies.z }, 0.16f, 0.02f, 0.3f, Color{ 70, 66, 64, 255 });
        return;
    }

    // Hueco negro y dos hojas que bajan girando en sus bisagras.
    CajaMiradas({ pies.x, y + 0.004f, pies.z }, lado - 0.04f, 0.012f, lado - 0.04f, Color{ 18, 12, 10, 255 });

    float angulo = 88.0f * apertura;

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 bisagra = { pies.x + (float)s * lado * 0.5f, y, pies.z };
        MarcoMiradas hoja = CrearMarcoMiradas(bisagra, 0.0f, 0.0f, (float)s * angulo, 1.0f);
        CajaMarcoMiradas(hoja, { -(float)s * lado * 0.25f, -0.03f, 0.0f }, { lado * 0.5f - 0.03f, 0.06f, lado - 0.04f }, TonoMiradas(COLOR_MADERA_MIRADAS, 0.92f));
    }
}


// Palco del solitario: torre pintada con columnas, escalera y faroles.
// (MODELO FUTURO: palco del titiritero.)
static void DibujarPalcoMiradas(float t)
{
    const float yb = Y_TABLADO_MIRADAS;
    const float ya = Y_PALCO_MIRADAS;
    const float z = Z_SOLO_MIRADAS;
    Color pintura = Color{ 52, 98, 104, 255 };
    Color panel = Color{ 40, 78, 86, 255 };
    Color crema = Color{ 236, 222, 192, 255 };

    CajaMiradas({ 0.0f, (yb + ya) * 0.5f, z }, 4.0f, ya - yb, 3.0f, pintura);

    // Panel frontal con marco dorado y un sol pintado.
    CajaMiradas({ 0.0f, (yb + ya) * 0.5f, z + 1.51f }, 3.1f, (ya - yb) - 0.5f, 0.02f, panel);
    CajaMiradas({ 0.0f, ya - 0.24f, z + 1.53f }, 3.2f, 0.08f, 0.03f, COLOR_DORADO_MIRADAS);
    CajaMiradas({ 0.0f, yb + 0.24f, z + 1.53f }, 3.2f, 0.08f, 0.03f, COLOR_DORADO_MIRADAS);
    CajaMiradas({ -1.56f, (yb + ya) * 0.5f, z + 1.53f }, 0.08f, (ya - yb) - 0.4f, 0.03f, COLOR_DORADO_MIRADAS);
    CajaMiradas({ 1.56f, (yb + ya) * 0.5f, z + 1.53f }, 0.08f, (ya - yb) - 0.4f, 0.03f, COLOR_DORADO_MIRADAS);

    Vector3 sol = { 0.0f, (yb + ya) * 0.5f, z + 1.56f };
    for (int k = 0; k < 8; k++)
    {
        MarcoMiradas rayo = CrearMarcoMiradas(sol, 0.0f, 0.0f, 22.5f + 45.0f * (float)k + std::sin(t * 0.8f) * 6.0f, 1.0f);
        CajaMarcoMiradas(rayo, { 0.0f, 0.62f, 0.0f }, { 0.14f, 0.42f, 0.03f }, COLOR_DORADO_MIRADAS);
    }
    EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, sol, { 0.38f, 0.38f, 0.08f }, Color{ 246, 204, 96, 255 }, 4, 12);

    // Columnas torneadas en las esquinas.
    for (int k = 0; k < 4; k++)
    {
        float cx = (k & 1) ? 1.95f : -1.95f;
        float cz = z + ((k & 2) ? 1.45f : -1.45f);
        CilindroMiradas({ cx, yb, cz }, 0.2f, 0.16f, ya - yb, 8, crema);
        CilindroMiradas({ cx, yb + 0.6f, cz }, 0.21f, 0.21f, 0.1f, 8, COLOR_DORADO_MIRADAS);
        CilindroMiradas({ cx, ya - 0.3f, cz }, 0.2f, 0.2f, 0.1f, 8, COLOR_DORADO_MIRADAS);
    }

    // Piso del palco y cornisa.
    CajaMiradas({ 0.0f, ya + 0.04f, z }, 4.4f, 0.12f, 3.4f, COLOR_MADERA_MIRADAS);
    CajaMiradas({ 0.0f, ya - 0.06f, z + 1.72f }, 4.5f, 0.1f, 0.06f, COLOR_DORADO_MIRADAS);

    // Baranda baja (el frente queda libre para el titiritero).
    Color baranda = TonoMiradas(crema, 0.92f);
    for (int s = -1; s <= 1; s += 2)
    {
        for (int k = 0; k < 4; k++)
        {
            CilindroMiradas({ (float)s * (1.15f + 0.3f * (float)k), ya + 0.1f, z + 1.55f }, 0.05f, 0.05f, 0.42f, 6, baranda);
        }
        CajaMiradas({ (float)s * 1.6f, ya + 0.55f, z + 1.55f }, 1.0f, 0.08f, 0.12f, COLOR_MADERA_OSCURA_MIRADAS);
        CajaMiradas({ (float)s * 2.12f, ya + 0.55f, z }, 0.12f, 0.08f, 3.1f, COLOR_MADERA_OSCURA_MIRADAS);
    }

    // Escalera lateral desde el tablado.
    for (int k = 0; k < 7; k++)
    {
        float alto = (ya - yb) * (float)(k + 1) / 7.0f;
        float x = 4.75f - 0.38f * ((float)k + 0.5f);
        CajaMiradas({ x, yb + alto * 0.5f, z + 0.6f }, 0.38f, alto, 1.1f, TonoMiradas(COLOR_MADERA_MIRADAS, 0.86f + 0.08f * (float)(k % 2)));
    }
    CilindroEntreMiradas({ 4.8f, yb + 0.95f, z + 1.15f }, { 2.15f, ya + 0.9f, z + 1.15f }, 0.05f, 0.05f, 6, COLOR_MADERA_OSCURA_MIRADAS);
    CilindroEntreMiradas({ 4.75f, yb, z + 1.15f }, { 4.75f, yb + 0.95f, z + 1.15f }, 0.05f, 0.05f, 6, COLOR_MADERA_OSCURA_MIRADAS);

    // Faroles en las esquinas del frente.
    DibujarFarolMiradas(FAROLES_MIRADAS[0], 1.2f);
    DibujarFarolMiradas(FAROLES_MIRADAS[1], 1.2f);
}


// Arcoiris de utileria: madera pintada, con borde crema, nubes recortadas
// a los pies y puntales detras. (MODELO FUTURO: arcoiris de utileria.)
static void DibujarArcoirisMiradas()
{
    static const Color BANDAS[7] =
    {
        { 224, 96, 88, 255 }, { 238, 150, 74, 255 }, { 240, 206, 96, 255 },
        { 124, 186, 106, 255 }, { 92, 168, 206, 255 }, { 102, 116, 190, 255 },
        { 152, 106, 182, 255 }
    };

    const float base = Y_TABLADO_MIRADAS;
    const float z = Z_ARCOIRIS_MIRADAS;
    const float rx = 6.9f;
    const float ry = 7.2f;
    const float ancho = 0.44f;
    const float grosor = 0.14f;
    int segmentos = CalidadDecoracion() == CALIDAD_ALTA ? 30 : 20;

    auto punto = [&](float a, float r, float zz) -> Vector3
    {
        return { std::cos(a) * (rx - r), base + std::sin(a) * (ry - r), zz };
    };

    rlBegin(RL_QUADS);

    // Ribete crema exterior e interior y las siete bandas.
    for (int b = -1; b <= 7; b++)
    {
        float r0 = b < 0 ? -0.14f : (b == 7 ? 7.0f * ancho : (float)b * ancho);
        float r1 = b < 0 ? 0.0f : (b == 7 ? 7.0f * ancho + 0.12f : (float)(b + 1) * ancho);
        Color color = (b < 0 || b == 7) ? Color{ 246, 236, 214, 255 } : BANDAS[b];

        for (int s = 0; s < segmentos; s++)
        {
            float a0 = PI * (float)s / (float)segmentos;
            float a1 = PI * (float)(s + 1) / (float)segmentos;
            Color c = TonoMiradas(color, 0.86f + 0.1f * HashMiradas(s, b + 3));

            VerticeMiradas(punto(a0, r1, z + grosor), c);
            VerticeMiradas(punto(a1, r1, z + grosor), c);
            VerticeMiradas(punto(a1, r0, z + grosor), c);
            VerticeMiradas(punto(a0, r0, z + grosor), c);
        }
    }

    // Canto exterior e interior (grosor de la madera).
    for (int s = 0; s < segmentos; s++)
    {
        float a0 = PI * (float)s / (float)segmentos;
        float a1 = PI * (float)(s + 1) / (float)segmentos;
        Color canto = Color{ 150, 118, 88, 255 };

        VerticeMiradas(punto(a0, -0.14f, z - grosor), canto);
        VerticeMiradas(punto(a1, -0.14f, z - grosor), canto);
        VerticeMiradas(punto(a1, -0.14f, z + grosor), TonoMiradas(canto, 1.1f));
        VerticeMiradas(punto(a0, -0.14f, z + grosor), TonoMiradas(canto, 1.1f));

        VerticeMiradas(punto(a0, 7.0f * ancho + 0.12f, z - grosor), TonoMiradas(canto, 0.8f));
        VerticeMiradas(punto(a1, 7.0f * ancho + 0.12f, z - grosor), TonoMiradas(canto, 0.8f));
        VerticeMiradas(punto(a1, 7.0f * ancho + 0.12f, z + grosor), canto);
        VerticeMiradas(punto(a0, 7.0f * ancho + 0.12f, z + grosor), canto);
    }

    rlEnd();

    // Puntales detras de cada pie.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * (rx - 1.6f);
        CilindroEntreMiradas({ x, base + 3.4f, z - 0.15f }, { x, base, z - 1.5f }, 0.07f, 0.07f, 5, COLOR_MADERA_OSCURA_MIRADAS);
        CajaMiradas({ x, base + 0.05f, z - 0.7f }, 0.2f, 0.1f, 1.6f, COLOR_MADERA_OSCURA_MIRADAS);
    }

    // Nubes de madera pintada al pie del arcoiris.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * (rx - 1.6f);
        Color nube = Color{ 248, 244, 236, 255 };
        EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, { x, base + 0.7f, z + 0.35f }, { 1.0f, 0.72f, 0.22f }, nube, 4, 10);
        EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, { x - 0.85f, base + 0.45f, z + 0.4f }, { 0.7f, 0.5f, 0.2f }, nube, 4, 10);
        EsferaMarcoMiradas(MARCO_MUNDO_MIRADAS, { x + 0.85f, base + 0.5f, z + 0.4f }, { 0.75f, 0.55f, 0.2f }, nube, 4, 10);
    }
}


// Telon bordo recogido a los lados, lambrequin festoneado, columnas y
// banderines. (MODELO FUTURO: boca de escena con telon.)
static void DibujarTelonMiradas(float t)
{
    const float xp = X_POSTE_TELON_MIRADAS;
    const float z = Z_TELON_MIRADAS;
    const float yb = Y_TABLADO_MIRADAS;
    const float ya = 10.4f;
    Color crema = Color{ 236, 222, 192, 255 };
    bool animado = CalidadEfectos() != CALIDAD_BAJA;

    // Columnas con capitel dorado y remate.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * xp;
        CajaMiradas({ x, yb + 0.25f, z }, 0.8f, 0.5f, 0.8f, COLOR_DORADO_MIRADAS);
        CilindroMiradas({ x, yb + 0.5f, z }, 0.3f, 0.26f, ya - yb - 0.9f, 10, crema);
        CajaMiradas({ x, ya - 0.3f, z }, 0.8f, 0.4f, 0.8f, COLOR_DORADO_MIRADAS);
        EsferaMiradas({ x, ya + 0.5f, z }, 0.3f, COLOR_DORADO_MIRADAS, 4, 8);
    }

    // Telones: franjas verticales con pliegues; se cinen con un cordon
    // dorado y se abren de nuevo hacia el piso.
    const int COLUMNAS = 14;
    const int FILAS = 12;
    const float yLazo = 4.3f;

    for (int s = -1; s <= 1; s += 2)
    {
        rlBegin(RL_QUADS);

        for (int f = 0; f < FILAS; f++)
        {
            float v0 = (float)f / (float)FILAS;
            float v1 = (float)(f + 1) / (float)FILAS;

            for (int c = 0; c < COLUMNAS; c++)
            {
                float u0 = (float)c / (float)COLUMNAS;
                float u1 = (float)(c + 1) / (float)COLUMNAS;

                auto vertice = [&](float u, float v)
                {
                    float y = ya - 0.2f - (ya - 0.2f - yb) * v;
                    float interior;

                    if (y > yLazo)
                    {
                        float k = (ya - 0.2f - y) / (ya - 0.2f - yLazo);
                        interior = 6.3f + 1.5f * (1.0f - (1.0f - k) * (1.0f - k));
                    }
                    else
                    {
                        float k = (yLazo - y) / (yLazo - yb);
                        interior = 7.8f - 0.75f * k;
                    }

                    float x = xp - 0.1f - (xp - 0.1f - interior) * u;
                    float fase = u * 2.0f * PI * 3.5f;
                    float amplitud = 0.12f + 0.1f * v;
                    float ola = animado ? 0.05f * std::sin(t * 1.3f + y * 0.5f + (float)s) : 0.0f;
                    float zz = z + 0.25f + std::sin(fase) * amplitud + ola * u;
                    Color tono = TonoMiradas(COLOR_TELON_MIRADAS, 0.7f + 0.32f * (0.5f + 0.5f * std::cos(fase)) + (s < 0 ? 0.05f : -0.03f));
                    VerticeMiradas({ (float)s * x, y, zz }, tono);
                };

                vertice(u0, v1);
                vertice(u1, v1);
                vertice(u1, v0);
                vertice(u0, v0);
            }
        }

        rlEnd();

        // Cordon dorado y borla en el lazo.
        Vector3 lazo = { (float)s * 7.9f, yLazo, z + 0.5f };
        CilindroEntreMiradas({ (float)s * (xp - 0.1f), yLazo + 0.1f, z + 0.42f }, lazo, 0.06f, 0.06f, 6, COLOR_DORADO_MIRADAS);
        EsferaMiradas(lazo, 0.14f, COLOR_DORADO_MIRADAS, 4, 8);
        CilindroMiradas({ lazo.x, lazo.y - 0.62f, lazo.z }, 0.14f, 0.04f, 0.5f, 8, COLOR_DORADO_MIRADAS);
    }

    // Dintel de madera y lambrequin festoneado con ribete dorado.
    CajaMiradas({ 0.0f, ya - 0.05f, z }, 2.0f * xp + 0.9f, 0.6f, 0.5f, COLOR_MADERA_OSCURA_MIRADAS);
    CajaMiradas({ 0.0f, ya - 0.35f, z + 0.28f }, 2.0f * xp - 0.4f, 0.55f, 0.04f, COLOR_TELON_MIRADAS);
    CajaMiradas({ 0.0f, ya + 0.0f, z + 0.31f }, 2.0f * xp - 0.4f, 0.08f, 0.04f, COLOR_DORADO_MIRADAS);

    const int FESTONES = 8;
    float x0 = -xp + 0.2f;
    float paso = (2.0f * xp - 0.4f) / (float)FESTONES;

    for (int f = 0; f < FESTONES; f++)
    {
        float xa = x0 + paso * (float)f;

        for (int capa = 0; capa < 2; capa++)
        {
            float extra = capa == 0 ? 0.08f : 0.0f;
            Color c = capa == 0 ? COLOR_DORADO_MIRADAS : COLOR_TELON_MIRADAS;
            float zz = z + 0.3f + (float)capa * 0.02f;

            rlBegin(RL_TRIANGLES);
            for (int k = 0; k < 8; k++)
            {
                float u0 = (float)k / 8.0f;
                float u1 = (float)(k + 1) / 8.0f;
                float h0 = 0.45f * std::sin(u0 * PI) + extra;
                float h1 = 0.45f * std::sin(u1 * PI) + extra;
                Color tono = TonoMiradas(c, 0.8f + 0.2f * std::sin((u0 + u1) * 0.5f * PI));
                float yTop = ya - 0.6f;
                VerticeMiradas({ xa + paso * u0, yTop, zz }, tono);
                VerticeMiradas({ xa + paso * u0, yTop - h0, zz }, tono);
                VerticeMiradas({ xa + paso * u1, yTop - h1, zz }, tono);
                VerticeMiradas({ xa + paso * u0, yTop, zz }, tono);
                VerticeMiradas({ xa + paso * u1, yTop - h1, zz }, tono);
                VerticeMiradas({ xa + paso * u1, yTop, zz }, tono);
            }
            rlEnd();
        }

        if (f > 0)
        {
            CilindroMiradas({ xa, ya - 0.95f, z + 0.36f }, 0.07f, 0.02f, 0.3f, 6, COLOR_DORADO_MIRADAS);
        }
    }

    // Banderines en dos guirnaldas bajo el lambrequin.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        DibujarGuirnaldaBanderinesMiradas({ -xp + 0.3f, ya - 1.0f, z + 0.45f }, { 0.0f, ya - 1.0f, z + 0.45f }, 0.5f, 9, animado ? t : 0.0f, 0);
        DibujarGuirnaldaBanderinesMiradas({ 0.0f, ya - 1.0f, z + 0.45f }, { xp - 0.3f, ya - 1.0f, z + 0.45f }, 0.5f, 9, animado ? t : 0.0f, 2);
    }
}


// Utileria sobre el tablado: perchero de marionetas, baul, macetas y
// cajones. (MODELO FUTURO: marionetas, baul y macetas.)
static void DibujarUtileriaTabladoMiradas(float t)
{
    const float y = Y_TABLADO_MIRADAS;

    // Faroles de las esquinas del frente: luz del tablado, siempre.
    DibujarFarolMiradas(FAROLES_MIRADAS[2], 2.1f);
    DibujarFarolMiradas(FAROLES_MIRADAS[3], 2.1f);

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    // Macetas con flores en las esquinas del frente.
    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 maceta = { (float)s * 7.0f, y, 3.7f };
        CilindroMiradas(maceta, 0.3f, 0.42f, 0.55f, 8, Color{ 196, 108, 72, 255 });
        EsferaMiradas({ maceta.x, y + 0.75f, maceta.z }, 0.45f, Color{ 86, 136, 72, 255 }, 4, 7);

        for (int k = 0; k < 5; k++)
        {
            float a = (float)k * 1.26f + (float)s;
            Color flor = k % 2 == 0 ? Color{ 248, 236, 214, 255 } : Color{ 236, 128, 156, 255 };
            EsferaMiradas({ maceta.x + std::cos(a) * 0.33f, y + 0.95f + 0.08f * (float)(k % 2), maceta.z + std::sin(a) * 0.3f }, 0.09f, flor, 3, 5);
        }
    }

    // Perchero con marionetas que se mecen (fondo izquierdo).
    {
        Vector3 base = { -6.4f, y, -6.2f };
        Color madera = COLOR_MADERA_OSCURA_MIRADAS;
        CilindroMiradas({ base.x - 1.1f, y, base.z }, 0.07f, 0.07f, 2.6f, 6, madera);
        CilindroMiradas({ base.x + 1.1f, y, base.z }, 0.07f, 0.07f, 2.6f, 6, madera);
        CajaMiradas({ base.x, y + 2.6f, base.z }, 2.5f, 0.1f, 0.12f, madera);

        static const Color ROPAS[3] = { { 236, 224, 196, 255 }, { 72, 140, 140, 255 }, { 206, 148, 66, 255 } };

        for (int k = 0; k < 3; k++)
        {
            float x = base.x - 0.7f + 0.7f * (float)k;
            float vaiven = std::sin(t * 1.4f + (float)k * 1.3f) * 9.0f;
            MarcoMiradas m = CrearMarcoMiradas({ x, y + 2.55f, base.z }, 0.0f, 0.0f, vaiven, 1.0f);
            DrawLine3D(PuntoMarcoMiradas(m, { -0.12f, 0.0f, 0.0f }), PuntoMarcoMiradas(m, { -0.12f, -0.9f, 0.0f }), Color{ 230, 226, 214, 255 });
            DrawLine3D(PuntoMarcoMiradas(m, { 0.12f, 0.0f, 0.0f }), PuntoMarcoMiradas(m, { 0.12f, -0.9f, 0.0f }), Color{ 230, 226, 214, 255 });
            EsferaMarcoMiradas(m, { 0.0f, -0.95f, 0.0f }, { 0.17f, 0.18f, 0.17f }, Color{ 240, 200, 168, 255 }, 4, 8);
            CajaMarcoMiradas(m, { 0.0f, -1.38f, 0.0f }, { 0.34f, 0.5f, 0.2f }, ROPAS[k]);
            CajaMarcoMiradas(m, { -0.08f, -1.8f, 0.0f }, { 0.1f, 0.36f, 0.1f }, TonoMiradas(ROPAS[k], 0.7f));
            CajaMarcoMiradas(m, { 0.08f, -1.8f, 0.0f }, { 0.1f, 0.36f, 0.1f }, TonoMiradas(ROPAS[k], 0.7f));
        }
    }

    // Baul abierto con telas (fondo derecho, detras de la escalera).
    {
        Vector3 c = { 6.6f, y, -6.5f };
        Color cuero = Color{ 120, 62, 46, 255 };
        CajaMiradas({ c.x, y + 0.35f, c.z }, 1.4f, 0.7f, 0.8f, cuero);
        CajaMiradas({ c.x, y + 0.36f, c.z + 0.41f }, 1.42f, 0.08f, 0.02f, COLOR_DORADO_MIRADAS);
        MarcoMiradas tapa = CrearMarcoMiradas({ c.x, y + 0.7f, c.z - 0.4f }, 0.0f, 70.0f, 0.0f, 1.0f);
        CajaMarcoMiradas(tapa, { 0.0f, 0.04f, 0.4f }, { 1.4f, 0.08f, 0.8f }, TonoMiradas(cuero, 0.9f));
        CajaMiradas({ c.x - 0.2f, y + 0.74f, c.z }, 0.7f, 0.1f, 0.5f, Color{ 152, 106, 178, 255 });
        CajaMiradas({ c.x + 0.4f, y + 0.8f, c.z + 0.1f }, 0.3f, 0.22f, 0.3f, COLOR_DORADO_MIRADAS);
    }

    // Cajones apilados (fondo izquierdo, junto al arcoiris).
    DibujarCajonMiradas({ -7.3f, y, -3.6f }, 0.8f, 7);
    DibujarCajonMiradas({ -7.2f, y + 0.8f, -3.55f }, 0.6f, 8);
    DibujarBarrilMiradas({ 7.3f, y, -2.4f }, 1.0f, 0.42f);
}


//==================================================
// TITERES: JUGADOR, VARA Y CABEZA GIGANTE
//==================================================

// Cabeza-titere de papel mache en un marco ya girado (escala = radio).
static void DibujarCabezaTitereMiradas(const MarcoMiradas& m, Color color, bool corona, bool caida)
{
    Color piel = caida ? MezclaMiradas(color, Color{ 150, 140, 130, 255 }, 0.55f) : MezclaMiradas(color, WHITE, 0.08f);
    Color oscuro = TonoMiradas(piel, 0.62f);

    EsferaMarcoMiradas(m, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.96f, 0.94f }, piel, 9, 14);
    EsferaMarcoMiradas(m, { -0.98f, 0.0f, -0.05f }, { 0.16f, 0.24f, 0.13f }, TonoMiradas(piel, 0.92f), 4, 6);
    EsferaMarcoMiradas(m, { 0.98f, 0.0f, -0.05f }, { 0.16f, 0.24f, 0.13f }, TonoMiradas(piel, 0.92f), 4, 6);

    // Ojos grandes (cerrados si ya cayo).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 0.34f;

        if (caida)
        {
            CajaMarcoMiradas(m, { x, 0.14f, 0.86f }, { 0.3f, 0.06f, 0.06f }, Color{ 40, 30, 36, 255 });
            continue;
        }

        EsferaMarcoMiradas(m, { x, 0.16f, 0.78f }, { 0.25f, 0.29f, 0.16f }, Color{ 250, 246, 238, 255 }, 5, 8);
        EsferaMarcoMiradas(m, { x, 0.13f, 0.92f }, { 0.12f, 0.14f, 0.06f }, Color{ 34, 28, 40, 255 }, 4, 6);
        EsferaMarcoMiradas(m, { x - 0.04f, 0.2f, 0.97f }, { 0.04f, 0.04f, 0.02f }, WHITE, 2, 4);
        CajaMarcoMiradas(m, { x, 0.5f, 0.8f }, { 0.3f, 0.07f, 0.08f }, Color{ 70, 46, 40, 255 });
    }

    // Nariz, cachetes y sonrisa pintada.
    EsferaMarcoMiradas(m, { 0.0f, -0.08f, 0.97f }, { 0.16f, 0.14f, 0.14f }, MezclaMiradas(piel, Color{ 214, 104, 92, 255 }, 0.45f), 4, 8);
    EsferaMarcoMiradas(m, { -0.56f, -0.2f, 0.74f }, { 0.17f, 0.11f, 0.06f }, MezclaMiradas(piel, Color{ 246, 140, 156, 255 }, 0.6f), 3, 6);
    EsferaMarcoMiradas(m, { 0.56f, -0.2f, 0.74f }, { 0.17f, 0.11f, 0.06f }, MezclaMiradas(piel, Color{ 246, 140, 156, 255 }, 0.6f), 3, 6);

    if (caida)
    {
        EsferaMarcoMiradas(m, { 0.0f, -0.44f, 0.84f }, { 0.1f, 0.12f, 0.05f }, Color{ 96, 30, 40, 255 }, 3, 6);
    }
    else
    {
        for (int k = 0; k < 7; k++)
        {
            float x = -0.36f + 0.12f * (float)k;
            float y = -0.46f + 0.5f * x * x;
            float z = std::sqrt(std::fmax(0.0f, 1.0f - x * x - y * y)) * 0.94f + 0.01f;
            CajaMarcoMiradas(m, { x, y, z }, { 0.13f, 0.06f, 0.05f }, Color{ 112, 32, 44, 255 });
        }
    }

    if (corona)
    {
        // Corona dorada con picos y gemas: el que manda.
        CilindroMarcoMiradas(m, { 0.0f, 0.7f, 0.0f }, 0.56f, 0.6f, 0.3f, 14, COLOR_DORADO_MIRADAS, COLOR_DORADO_MIRADAS, 0);

        for (int k = 0; k < 5; k++)
        {
            float a = 2.0f * PI * (float)k / 5.0f;
            CilindroMarcoMiradas(m, { std::sin(a) * 0.5f, 0.98f, std::cos(a) * 0.5f }, 0.13f, 0.0f, 0.32f, 5, COLOR_DORADO_MIRADAS, COLOR_DORADO_MIRADAS, 0);
            EsferaMarcoMiradas(m, { std::sin(a) * 0.5f, 1.32f, std::cos(a) * 0.5f }, { 0.06f, 0.06f, 0.06f }, Color{ 255, 240, 200, 255 }, 3, 4);
        }

        EsferaMarcoMiradas(m, { 0.0f, 0.85f, 0.6f }, { 0.09f, 0.09f, 0.05f }, Color{ 200, 50, 80, 255 }, 3, 6);
        EsferaMarcoMiradas(m, { -0.42f, 0.85f, 0.42f }, { 0.07f, 0.07f, 0.04f }, Color{ 60, 150, 170, 255 }, 3, 6);
        EsferaMarcoMiradas(m, { 0.42f, 0.85f, 0.42f }, { 0.07f, 0.07f, 0.04f }, Color{ 60, 150, 170, 255 }, 3, 6);
    }
    else if (!caida)
    {
        // Gorrito de cono a rayas con pompon.
        MarcoMiradas gorro = m;
        CilindroMarcoMiradas(gorro, { 0.0f, 0.74f, -0.05f }, 0.44f, 0.0f, 0.8f, 12, Color{ 246, 236, 214, 255 }, oscuro, 3);
        EsferaMarcoMiradas(gorro, { 0.0f, 1.56f, -0.05f }, { 0.14f, 0.14f, 0.14f }, MezclaMiradas(color, WHITE, 0.4f), 3, 6);
    }
}


// Golilla, tela del cuerpo y vara (no giran con la cabeza).
static void DibujarCuerpoTitereMiradas(Vector3 cabeza, float radio, Color color, float t, float escala)
{
    float yCuello = cabeza.y - radio * 0.9f;
    float largo = 1.15f * escala;
    Color tela = TonoMiradas(color, 0.82f);

    // Tela con pliegues que se mece un poco.
    rlBegin(RL_QUADS);
    const int LADOS = 14;
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
        float am = (a0 + a1) * 0.5f;
        float r0 = 0.3f * escala;
        float r1a = (0.72f + 0.08f * std::sin(a0 * 6.0f + t * 2.0f)) * escala;
        float r1b = (0.72f + 0.08f * std::sin(a1 * 6.0f + t * 2.0f)) * escala;
        Color tono = LuzMiradas(tela, { std::sin(am), 0.3f, std::cos(am) });
        tono = TonoMiradas(tono, 0.86f + 0.14f * std::cos(am * 6.0f));

        VerticeMiradas({ cabeza.x + std::sin(a0) * r1a, yCuello - largo, cabeza.z + std::cos(a0) * r1a }, tono);
        VerticeMiradas({ cabeza.x + std::sin(a1) * r1b, yCuello - largo, cabeza.z + std::cos(a1) * r1b }, tono);
        VerticeMiradas({ cabeza.x + std::sin(a1) * r0, yCuello, cabeza.z + std::cos(a1) * r0 }, tono);
        VerticeMiradas({ cabeza.x + std::sin(a0) * r0, yCuello, cabeza.z + std::cos(a0) * r0 }, tono);
    }
    rlEnd();

    // Dobladillo festoneado crema.
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
        float am = (a0 + a1) * 0.5f;
        float r = (0.73f + 0.08f * std::sin(am * 6.0f + t * 2.0f)) * escala;
        Color c = LuzMiradas(Color{ 246, 232, 204, 255 }, { std::sin(am), 0.0f, std::cos(am) });
        VerticeMiradas({ cabeza.x + std::sin(a0) * r, yCuello - largo + 0.01f, cabeza.z + std::cos(a0) * r }, c);
        VerticeMiradas({ cabeza.x + std::sin(a1) * r, yCuello - largo + 0.01f, cabeza.z + std::cos(a1) * r }, c);
        VerticeMiradas({ cabeza.x + std::sin(am) * (r + 0.02f), yCuello - largo - 0.16f * escala, cabeza.z + std::cos(am) * (r + 0.02f) }, c);
    }
    rlEnd();

    // Golilla de volados.
    for (int k = 0; k < 10; k++)
    {
        float a = 2.0f * PI * (float)k / 10.0f;
        Color c = k % 2 == 0 ? Color{ 250, 246, 236, 255 } : Color{ 232, 222, 206, 255 };
        EsferaMiradas({ cabeza.x + std::sin(a) * radio * 0.48f, yCuello + 0.02f, cabeza.z + std::cos(a) * radio * 0.48f }, radio * 0.2f, c, 3, 6);
    }
}


static void DibujarTitereMiradas(
    const MinijuegoMiradasCruzadas& minijuego,
    const Participante participantes[],
    int indice,
    float aparicion
)
{
    float t = minijuego.tiempoAnimacion;
    const VisualJugadorMiradas& v = minijuego.visual[indice];
    bool esSolo = indice == minijuego.indiceSolo;
    Color color = participantes[indice].color;
    Vector3 pies = PiesJugadorMiradas(minijuego, indice);
    float radio = RadioCabezaMiradas(minijuego, indice);

    // Atrapado: espera con la cabeza girada, se abre la trampilla y cae.
    float s = v.tiempoAtrapado;
    if (!esSolo && minijuego.jugadores[indice].eliminado && s < 0.0f)
    {
        s = 99.0f;
    }

    bool atrapado = s >= 0.0f;
    float caida = atrapado ? Limitar01Miradas((s - ESPERA_TRAMPILLA_MIRADAS) / DURACION_CAIDA_MIRADAS) : 0.0f;
    bool yaCayo = atrapado && s >= ESPERA_TRAMPILLA_MIRADAS + DURACION_CAIDA_MIRADAS;

    if (!esSolo)
    {
        float apertura = 0.0f;
        if (atrapado && s >= ESPERA_TRAMPILLA_MIRADAS)
        {
            float abierta = Limitar01Miradas((s - ESPERA_TRAMPILLA_MIRADAS) / 0.1f);
            float cerrando = Limitar01Miradas((s - ESPERA_TRAMPILLA_MIRADAS - DURACION_CAIDA_MIRADAS) / 0.2f);
            apertura = abierta * (1.0f - cerrando);
        }

        DibujarTrampillaMiradas(pies, apertura, color, minijuego.jugadores[indice].eliminado);
    }
    else
    {
        AnilloMiradas(pies.x, pies.y + 0.12f, pies.z, 0.95f, 1.1f, color);
    }

    // Sacudida mientras espera la trampilla.
    float tiembla = atrapado && s < ESPERA_TRAMPILLA_MIRADAS
        ? std::sin(t * 60.0f) * 0.06f * (1.0f - s / ESPERA_TRAMPILLA_MIRADAS)
        : 0.0f;
    float hundido = caida * caida * 3.2f;

    if (!yaCayo)
    {
        float escalaModelo = ESCALA_MODELO_JUGADOR_3D * (0.4f + 0.6f * Limitar01Miradas(aparicion));
        Vector3 cuerpo = { pies.x + tiembla, pies.y - hundido, pies.z + 0.1f };

        if (hundido < 0.05f)
        {
            DibujarSombraRetroCircular({ pies.x, pies.y + 0.02f, pies.z + 0.1f }, 0.46f, 0.38f, pies.y + 0.025f);
        }

        DibujarModeloJugadorEnPosicion(cuerpo, v.giro * 0.5f, color, escalaModelo);
    }

    if (aparicion <= 0.01f)
    {
        return;
    }

    // Cabeza sobre la vara: bamboleo de titere, salto del farolito al
    // elegir (no delata la direccion) y brinco al salvarse.
    float bamboleo = std::sin(t * 2.1f + pies.x) * 0.05f;
    float brinco = std::sin(v.pulsoListo * PI) * 0.22f + std::sin(v.destello * PI) * 0.18f;
    Vector3 cabeza = CentroCabezaMiradas(minijuego, indice);
    cabeza.y += bamboleo + brinco;
    cabeza.x += tiembla;

    float escala = aparicion;
    float giroReposo = RevelandoMiradas(minijuego) ? 0.0f : std::sin(t * 1.7f + pies.x) * 4.0f;

    if (!yaCayo)
    {
        Vector3 cabezaVara = { cabeza.x, cabeza.y - hundido, cabeza.z };
        Vector3 piso = { pies.x + tiembla, pies.y - hundido, pies.z - FONDO_VARA_MIRADAS };

        CilindroEntreMiradas(piso, { cabezaVara.x, cabezaVara.y - radio * escala * 0.5f, cabezaVara.z }, 0.07f, 0.06f, 6, COLOR_MADERA_OSCURA_MIRADAS);
        DibujarCuerpoTitereMiradas(cabezaVara, radio * escala, color, t, (esSolo ? 1.15f : 1.0f) * escala);

        // Farolito de LISTO bajo la golilla.
        bool listo = minijuego.fase == FASE_MIRADAS_ELECCION && minijuego.jugadores[indice].eligioDireccion;
        Vector3 farol = { cabezaVara.x, cabezaVara.y - radio * escala * 1.15f, cabezaVara.z + 0.36f * escala + 0.08f };
        CajaMiradas({ farol.x, farol.y + 0.14f, farol.z }, 0.2f, 0.05f, 0.2f, Color{ 58, 52, 56, 255 });
        CajaMiradas(farol, 0.17f, 0.22f, 0.17f, listo ? Color{ 255, 236, 150, 255 } : Color{ 120, 116, 120, 255 });

        // Cabeza: si lo atraparon, salta de la vara y rueda junto a la trampilla.
        if (atrapado && caida > 0.0f)
        {
            Vector3 reposo = { pies.x + 1.05f, pies.y + radio * 0.8f, pies.z + 0.75f };
            Vector3 p =
            {
                cabeza.x + (reposo.x - cabeza.x) * caida,
                cabeza.y + (reposo.y - cabeza.y) * caida + 1.6f * 4.0f * caida * (1.0f - caida),
                cabeza.z + (reposo.z - cabeza.z) * caida
            };
            MarcoMiradas m = CrearMarcoMiradas(
                p,
                v.giro * (1.0f - caida),
                v.cabeceo + (-15.0f - v.cabeceo) * caida,
                -30.0f * caida - 360.0f * caida * (1.0f - caida),
                radio * (1.0f - 0.2f * caida)
            );
            DibujarCabezaTitereMiradas(m, color, false, false);
        }
        else
        {
            MarcoMiradas m = CrearMarcoMiradas(cabeza, v.giro + giroReposo, v.cabeceo, 0.0f, radio * escala);
            DibujarCabezaTitereMiradas(m, color, esSolo, false);
        }

        return;
    }

    // Ya cayo: queda la cabeza tirada sobre el tablado, gris y dormida.
    Vector3 reposo = { pies.x + 1.05f, pies.y + radio * 0.8f, pies.z + 0.75f };
    DibujarSombraRetroCircular({ reposo.x, pies.y + 0.02f, reposo.z }, radio * 0.8f, radio * 0.6f, pies.y + 0.025f);
    MarcoMiradas m = CrearMarcoMiradas(reposo, 0.0f, -15.0f, -30.0f, radio * 0.8f);
    DibujarCabezaTitereMiradas(m, color, false, true);
}


static void DibujarParticulasMiradas(const MinijuegoMiradasCruzadas& minijuego)
{
    for (int i = 0; i < MAX_PARTICULAS_MIRADAS; i++)
    {
        const ParticulaMiradas& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        float tam = p.tamano * (0.5f + 0.5f * vida);
        Color c = p.color;
        c.a = (unsigned char)(255.0f * Limitar01Miradas(vida * 1.6f));
        CajaMiradas(p.posicion, tam, tam, tam, c);
    }
}


// Onda roja donde atraparon a alguien.
static void DibujarOndasAtrapadoMiradas(const MinijuegoMiradasCruzadas& minijuego)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float s = minijuego.visual[i].tiempoAtrapado;

        if (!minijuego.resultado.participantes[i].participo || s < 0.0f || s > 0.9f)
        {
            continue;
        }

        Vector3 pies = PiesJugadorMiradas(minijuego, i);
        float u = s / 0.9f;
        float r = 0.8f + 1.8f * u;
        AnilloMiradas(pies.x, pies.y + 0.03f, pies.z, r - 0.16f, r, Fade(Color{ 236, 64, 52, 255 }, 0.85f * (1.0f - u)));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// LUCES FALSAS (aditivas, al final del 3D)
//==================================================

static void DibujarLucesMiradas(
    const MinijuegoMiradasCruzadas& minijuego,
    const Participante participantes[]
)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = minijuego.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Sol y su bruma.
    ResplandorFrenteMiradas({ -112.0f, 12.0f, -170.0f }, 42.0f, Color{ 255, 170, 110, 90 });
    ResplandorFrenteMiradas({ -112.0f, 12.0f, -169.0f }, 14.0f, Color{ 255, 230, 180, 120 });

    // Candilejas: halo y charco calido sobre el tablado.
    for (int k = 0; k < 8; k++)
    {
        float x = -7.0f + 2.0f * (float)k;
        float pulso = 0.9f + 0.1f * std::sin(t * 7.0f + (float)k * 1.3f);
        ResplandorFrenteMiradas({ x, Y_TABLADO_MIRADAS + 0.14f, Z_FRENTE_TABLADO_MIRADAS - 0.2f }, 0.5f * pulso, Color{ 255, 220, 150, 150 });
        ResplandorSueloMiradas(x, Y_TABLADO_MIRADAS + 0.015f, Z_FRENTE_TABLADO_MIRADAS - 1.3f, 1.9f * pulso, Color{ 255, 186, 110, 46 });
    }

    // Faroles: halo, charco y titileo de llama.
    for (int k = 0; k < 4; k++)
    {
        Vector3 f = FAROLES_MIRADAS[k];
        float pulso = 0.85f + 0.15f * std::sin(t * 9.0f + (float)k * 2.1f);
        float piso = k < 2 ? Y_PALCO_MIRADAS + 0.12f : Y_TABLADO_MIRADAS + 0.015f;
        ResplandorFrenteMiradas({ f.x, f.y, f.z + 0.2f }, 0.75f * pulso, Color{ 255, 200, 120, 130 });
        ResplandorFrenteMiradas({ f.x, f.y, f.z + 0.21f }, 0.25f, Color{ 255, 244, 210, 200 });
        ResplandorSueloMiradas(f.x, piso, f.z, 1.6f * pulso, Color{ 255, 176, 96, 48 });
    }

    // Focos de las torres: cono suave y charco sobre el palco.
    Vector3 objetivo = { 0.0f, Y_PALCO_MIRADAS + 1.8f, Z_SOLO_MIRADAS };
    for (int s = 0; s < 2; s++)
    {
        Vector3 foco = POSICION_FOCOS_MIRADAS[s];
        Vector3 dir = Vector3Normalize(Vector3Subtract(objetivo, foco));
        Vector3 boca = Vector3Add(foco, Vector3Scale(dir, 0.35f));
        Vector3 fin = Vector3Add(objetivo, Vector3Scale(dir, 1.5f));
        DrawCylinderEx(boca, fin, 0.3f, 2.0f, 16, Color{ 255, 228, 170, 9 });
        DrawCylinderEx(boca, fin, 0.2f, 1.1f, 12, Color{ 255, 236, 190, 7 });
        ResplandorFrenteMiradas(Vector3Add(boca, Vector3Scale(dir, 0.05f)), 0.9f, Color{ 255, 236, 190, 150 });
    }
    ResplandorSueloMiradas(0.0f, Y_PALCO_MIRADAS + 0.12f, Z_SOLO_MIRADAS + 0.2f, 2.4f, Color{ 255, 226, 170, 70 });

    // Bombillas de las guirnaldas, titilando.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        int n = 0;
        for (const TramoGuirnaldaMiradas& tramo : BOMBILLAS_MIRADAS)
        {
            for (int k = 0; k < tramo.cantidad; k++, n++)
            {
                Vector3 p = PuntoCatenariaMiradas(tramo.a, tramo.b, tramo.caida, ((float)k + 0.5f) / (float)tramo.cantidad);
                float brillo = 0.7f + 0.3f * std::sin(t * 3.0f + (float)n * 1.9f);
                ResplandorFrenteMiradas({ p.x, p.y - 0.1f, p.z + 0.05f }, 0.42f, Color{ 255, 214, 140, (unsigned char)(120.0f * brillo) });
            }
        }
    }

    // Luciernagas del atardecer a los costados.
    int luciernagas = (int)(28.0f * FactorCalidadGrafica(CalidadParticulas()));
    for (int k = 0; k < luciernagas; k++)
    {
        float lado = HashMiradas(k, 1) < 0.5f ? -1.0f : 1.0f;
        float x = lado * (9.0f + 6.0f * HashMiradas(k, 2)) + 0.8f * std::sin(t * 0.5f + (float)k);
        float y = 0.6f + 3.0f * HashMiradas(k, 4) + 0.5f * std::sin(t * 0.7f + (float)k * 1.7f);
        float z = -12.0f + 15.0f * HashMiradas(k, 3) + 0.8f * std::cos(t * 0.4f + (float)k * 0.9f);
        float brillo = 0.5f + 0.5f * std::sin(t * 2.3f + (float)k * 2.1f);

        if (brillo < 0.15f)
        {
            continue;
        }

        ResplandorFrenteMiradas({ x, y, z }, 0.32f, Color{ 255, 230, 120, (unsigned char)(110.0f * brillo) });
        ResplandorFrenteMiradas({ x, y, z + 0.01f }, 0.08f, Color{ 255, 250, 200, (unsigned char)(230.0f * brillo) });
    }

    // Feedback de los titeres.
    if (minijuego.indiceSolo >= 0)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!minijuego.resultado.participantes[i].participo)
            {
                continue;
            }

            const VisualJugadorMiradas& v = minijuego.visual[i];
            Vector3 pies = PiesJugadorMiradas(minijuego, i);
            Vector3 cabeza = CentroCabezaMiradas(minijuego, i);
            float radio = RadioCabezaMiradas(minijuego, i);

            // Farolito LISTO encendido.
            if (minijuego.fase == FASE_MIRADAS_ELECCION && minijuego.jugadores[i].eligioDireccion)
            {
                Vector3 farol = { cabeza.x, cabeza.y - radio * 1.15f, cabeza.z + 0.44f };
                ResplandorFrenteMiradas({ farol.x, farol.y, farol.z + 0.1f }, 0.5f + 0.25f * v.pulsoListo, Color{ 255, 226, 130, 170 });
            }

            // Destello al salvarse (o al atrapar, el solitario).
            if (v.destello > 0.0f)
            {
                Color c = MezclaMiradas(participantes[i].color, Color{ 255, 236, 160, 255 }, 0.4f);
                c.a = (unsigned char)(150.0f * v.destello);
                ResplandorSueloMiradas(pies.x, pies.y + 0.03f, pies.z, 1.8f, c);
                ResplandorFrenteMiradas({ cabeza.x, cabeza.y, cabeza.z - radio * 0.6f }, radio * (1.5f + 0.9f * (1.0f - v.destello)), Color{ 255, 200, 110, (unsigned char)(55.0f * v.destello) });
            }
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJAR
//==================================================

static Camera3D ObtenerCamaraMiradas(const MinijuegoMiradasCruzadas& minijuego)
{
    // En la cuenta la camara baja y se acerca al tablado (x reducir movimiento).
    float intro = minijuego.fase == FASE_MIRADAS_PREPARACION
        ? (minijuego.tiempoPreparacion / DURACION_PREPARACION_MIRADAS) * ObtenerFactorMovimientoCamaraMinijuegos()
        : 0.0f;
    intro *= intro;

    Camera3D camara{};
    camara.position = { 0.0f, 7.6f + 2.4f * intro, 17.0f + 6.0f * intro };
    camara.target = { 0.0f, 4.0f + 0.5f * intro, -1.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 45.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Los titeres asoman en ola con rebote durante la cuenta.
static float AparicionTitereMiradas(const MinijuegoMiradasCruzadas& minijuego, int orden)
{
    if (minijuego.fase != FASE_MIRADAS_PREPARACION)
    {
        return 1.0f;
    }

    float transcurrido = DURACION_PREPARACION_MIRADAS - minijuego.tiempoPreparacion;
    return EaseOutBackMiradas((transcurrido - 0.25f - 0.16f * (float)orden) / 0.45f);
}


void MinijuegoMiradasCruzadas::Dibujar(
    const Participante participantes[]
) const
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    ClearBackground(COLOR_BRUMA_MIRADAS);

    const float t = tiempoAnimacion;
    Camera3D camara = ObtenerCamaraMiradas(*this);

    BeginMode3D(camara);
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    DibujarCieloMiradas(t);
    DibujarHorizonteMiradas(t);
    DibujarSueloMiradas();
    DibujarFeriaMiradas(t);
    DibujarTabladoMiradas(t);
    DibujarArcoirisMiradas();
    DibujarTelonMiradas(t);
    DibujarPalcoMiradas(t);
    DibujarUtileriaTabladoMiradas(t);

    if (indiceSolo >= 0)
    {
        DibujarTitereMiradas(*this, participantes, indiceSolo, AparicionTitereMiradas(*this, 0));

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (i == indiceSolo || !resultado.participantes[i].participo)
            {
                continue;
            }

            DibujarTitereMiradas(*this, participantes, i, AparicionTitereMiradas(*this, 1 + RanuraEquipoMiradas(*this, i)));
        }

        DibujarParticulasMiradas(*this);
        DibujarOndasAtrapadoMiradas(*this);
    }

    DibujarLucesMiradas(*this, participantes);

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    EndMode3D();

    if (indiceSolo < 0)
    {
        return;
    }

    // Cartelitos J# sobre cada cabeza-titere.
    float tiempoResolucion = DURACION_RESOLUCION_MIRADAS - tiempoFase;
    Vector2 anclaSolo{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool esSolo = i == indiceSolo;
        float radio = RadioCabezaMiradas(*this, i);
        Vector3 cabeza = CentroCabezaMiradas(*this, i);
        Vector3 sobre = { cabeza.x, cabeza.y + radio * (esSolo ? 1.45f : 1.72f) + (esSolo ? 0.25f : 0.12f), cabeza.z };
        Vector2 p = GetWorldToScreen(sobre, camara);

        if (esSolo)
        {
            anclaSolo = p;

            // Durante el revelado el anuncio ocupa su lugar.
            if (fase == FASE_MIRADAS_RESOLUCION)
            {
                continue;
            }
        }
        else if (jugadores[i].eliminado)
        {
            float s = visual[i].tiempoAtrapado;

            if (s >= 0.0f && s < ESPERA_TRAMPILLA_MIRADAS + DURACION_CAIDA_MIRADAS)
            {
                float alfa = Limitar01Miradas(s / 0.12f);
                float salto = std::sin(Limitar01Miradas(s / 0.25f) * PI) * Px(10.0f);
                DibujarTextoMundoContornoUI("¡ATRAPADO!", p.x, p.y - Px(12.0f) - salto, UI_TEXTO, UI_PELIGRO, alfa);
            }

            continue;
        }

        if (AparicionTitereMiradas(*this, esSolo ? 0 : 1 + RanuraEquipoMiradas(*this, i)) < 0.9f)
        {
            continue;
        }

        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);
        DibujarEtiquetaUI(rotulo, p, 0.0f, opciones);
    }

    // Revelado: la direccion del solitario recien se nombra al resolver.
    if (fase == FASE_MIRADAS_RESOLUCION)
    {
        int atrapados = 0;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                i != indiceSolo &&
                jugadores[i].eliminado &&
                jugadores[i].rondaEliminacion == numeroRonda
            )
            {
                atrapados++;
            }
        }

        float alfa = Limitar01Miradas(tiempoResolucion / 0.2f);
        float pop = 1.0f + 0.25f * (1.0f - Limitar01Miradas(tiempoResolucion / 0.18f));
        float y = anclaSolo.y - Px(30.0f);
        float minimo = Px(150.0f);
        if (y < minimo) y = minimo;

        char texto[48];
        std::snprintf(
            texto,
            sizeof(texto),
            "J%d MIRÓ %s",
            participantes[indiceSolo].numeroJugador,
            NombreDireccionMirada(jugadores[indiceSolo].direccion)
        );
        DibujarTextoMundoContornoUI(texto, anclaSolo.x, y - Px(6.0f) * (pop - 1.0f) * 4.0f, UI_TEXTO_GRANDE, UI_FOCO, alfa);

        if (atrapados > 0)
        {
            std::snprintf(
                texto,
                sizeof(texto),
                atrapados == 1 ? "¡ATRAPÓ A %d!" : "¡ATRAPÓ A %d JUGADORES!",
                atrapados
            );
        }
        else
        {
            std::snprintf(texto, sizeof(texto), "¡NADIE COINCIDIÓ!");
        }

        DibujarTextoMundoContornoUI(
            texto,
            anclaSolo.x,
            y + Px(34.0f),
            UI_TEXTO,
            atrapados > 0 ? UI_PELIGRO : UI_POSITIVO,
            alfa
        );
    }

    // HUD comun (Minigames/HudMinijuego): solo lee el estado. La direccion
    // elegida NUNCA aparece antes de resolver: solo un «LISTO» neutro.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "MIRADAS CRUZADAS",
        "1 VS 3: EL SOLITARIO ATRAPA A QUIEN MIRE HACIA SU MISMO LADO. EQUIPO: AGUANTÁ 5 RONDAS.",
        resultado
    );

    hud.fase = fase == FASE_MIRADAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_MIRADAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin reloj de partida: el cartel central lleva la ronda y la barra
    // muestra el tiempo que queda para elegir.
    int rondaVisible = numeroRonda < 1 ? 1 : numeroRonda;
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = fase == FASE_MIRADAS_ELECCION ? "RONDA: ¡ELEGÍ!" : "RONDA";
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", rondaVisible, MAX_RONDAS_MIRADAS);
    hud.progresoMedidor = fase == FASE_MIRADAS_ELECCION
        ? tiempoFase / DURACION_ELECCION_MIRADAS
        : (fase == FASE_MIRADAS_PREPARACION ? 1.0f : 0.0f);
    hud.finPorTiempo = false;

    int tamanoEquipo = 0;
    int atrapadosTotal = 0;
    int atrapadosRonda = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (i == indiceSolo || !resultado.participantes[i].participo)
        {
            continue;
        }

        tamanoEquipo++;

        if (jugadores[i].eliminado)
        {
            atrapadosTotal++;

            if (jugadores[i].rondaEliminacion == numeroRonda)
            {
                atrapadosRonda++;
            }
        }
    }

    if (fase == FASE_MIRADAS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        hud.tituloFinal = resultado.participantes[indiceSolo].posicionFinal == 1
            ? "¡GANA EL SOLITARIO!"
            : "¡GANA EL EQUIPO!";
        hud.notaFinal = "EL EQUIPO GANA SI ALGUIEN AGUANTA LAS 5 RONDAS.";
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const Participante& participante = participantes[i];
        bool esSolo = i == indiceSolo;

        jugador.equipo = esSolo ? 0 : 1;
        jugador.rol = esSolo ? "SOLITARIO" : "EQUIPO";

        if (esSolo)
        {
            jugador.valorNumerico = atrapadosTotal;
            jugador.progreso = tamanoEquipo > 0 ? (float)atrapadosTotal / tamanoEquipo : 0.0f;
            std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", atrapadosTotal, tamanoEquipo);
            std::snprintf(
                jugador.detalleFinal,
                sizeof(jugador.detalleFinal),
                atrapadosTotal == 1 ? "%d ATRAPADO" : "%d ATRAPADOS",
                atrapadosTotal
            );

            if (fase == FASE_MIRADAS_RESOLUCION && atrapadosRonda > 0)
            {
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡ATRAPÓ!");
            }
        }
        else
        {
            jugador.eliminado = jugadores[i].eliminado;

            if (jugadores[i].eliminado)
            {
                std::snprintf(
                    jugador.detalleFinal,
                    sizeof(jugador.detalleFinal),
                    "CAYÓ EN RONDA %d",
                    jugadores[i].rondaEliminacion
                );
            }
            else
            {
                std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "A SALVO");

                if (fase == FASE_MIRADAS_RESOLUCION)
                {
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡A SALVO!");
                }
            }
        }

        // Igual para cualquier direccion: solo confirma que eligio, como el
        // farolito de la vara.
        if (
            fase == FASE_MIRADAS_ELECCION &&
            jugadores[i].eligioDireccion &&
            !jugador.eliminado
        )
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "LISTO");
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MIRÁ: %s\nSIN TOCAR: AL FRENTE",
                TextoMoverHud(participante)
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoMiradasCruzadas::ObtenerResultado() const
{
    return resultado;
}
