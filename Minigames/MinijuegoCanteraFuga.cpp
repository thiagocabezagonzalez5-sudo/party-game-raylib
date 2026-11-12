#include "Minigames/MinijuegoCanteraFuga.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>

// El escenario tiene su propia luz falsa (caras sombreadas, focos de obra):
// se usan las primitivas puras de raylib y las sombras que importan
// (rocas sobre la rampa, maquinas) se dibujan a mano.
#undef DrawCube
#undef DrawSphere
#undef DrawCylinder


static const float DURACION_PREPARACION_CANTERA = 2.5f;
static const float DURACION_PARTIDA_CANTERA = 30.0f;
static const float LIMITE_X_CANTERA = 4.25f;
static const float Z_CIMA_CANTERA = -9.25f;
static const float Z_BASE_CANTERA = 9.25f;
static const float VELOCIDAD_ESCALADOR_CANTERA = 4.25f;
static const float DURACION_ATURDIDO_CANTERA = 1.05f;
static const float RECARGA_ROCA_CANTERA = 0.82f;


static float LimitarCantera(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AlturaPendienteCantera(float z)
{
    float progreso = (Z_BASE_CANTERA - z) / (Z_BASE_CANTERA - Z_CIMA_CANTERA);
    progreso = LimitarCantera(progreso, 0.0f, 1.0f);
    return progreso * 5.0f;
}


static float CalcularProgresoCantera(float z)
{
    return LimitarCantera(
        (Z_BASE_CANTERA - z) / (Z_BASE_CANTERA - Z_CIMA_CANTERA),
        0.0f,
        1.0f
    );
}


//==================================================
// VISUAL: AZAR PROPIO, PARTICULAS Y DESTELLOS
//==================================================
//
// Todo lo de este bloque es decorativo. Usa un generador propio para no
// consumir el GetRandomValue del juego (rocas e IA quedan identicas).
//==================================================

static const Color COLOR_ROCA_CANTERA = { 80, 86, 102, 255 };
static const Color COLOR_POLVO_CANTERA = { 214, 184, 140, 190 };


static float AzarVisualCantera(MinijuegoCanteraFuga& m, float minimo, float maximo)
{
    unsigned int x = m.semillaVisual;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m.semillaVisual = x;
    return minimo + (maximo - minimo) * ((float)(x & 0xFFFFFF) / 16777215.0f);
}


static int CantidadParticulasCantera(int cantidad)
{
    int escalada = (int)std::lround((float)cantidad * FactorCalidadGrafica(CalidadParticulas()));
    return escalada < 1 ? 1 : escalada;
}


static ParticulaCanteraFuga* ReservarParticulaCantera(MinijuegoCanteraFuga& m)
{
    for (int i = 0; i < MAX_PARTICULAS_CANTERA_FUGA; i++)
    {
        if (!m.particulas[i].activa) return &m.particulas[i];
    }

    return nullptr;
}


// Suelo aproximado para que escombros y confeti se apoyen en algo.
static float SueloVisualCantera(float x, float z)
{
    if (std::fabs(x) <= 5.3f && z <= Z_BASE_CANTERA + 2.3f)
    {
        return AlturaPendienteCantera(z);
    }

    return -0.22f;
}


static void EmitirParticulasCantera(
    MinijuegoCanteraFuga& m,
    TipoParticulaCanteraFuga tipo,
    Vector3 posicion,
    int cantidad,
    float fuerza,
    float tamano,
    Color color
)
{
    static const Color CONFETI[5] =
    {
        { 255, 214, 92, 255 }, { 255, 246, 226, 255 }, { 255, 136, 64, 255 },
        { 120, 214, 236, 255 }, { 236, 92, 110, 255 }
    };

    int total = CantidadParticulasCantera(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaCanteraFuga* p = ReservarParticulaCantera(m);
        if (p == nullptr) return;

        float angulo = AzarVisualCantera(m, 0.0f, 6.2832f);
        float velocidad = AzarVisualCantera(m, 0.25f, 1.0f) * fuerza;

        *p = {};
        p->activa = true;
        p->tipo = tipo;
        p->posicion = posicion;
        p->velocidad = { std::cos(angulo) * velocidad, 0.0f, std::sin(angulo) * velocidad };
        p->tamano = tamano * AzarVisualCantera(m, 0.7f, 1.3f);
        p->color = color;

        switch (tipo)
        {
            case PARTICULA_CANTERA_POLVO:
            {
                p->velocidad.y = AzarVisualCantera(m, 0.15f, 0.7f) * fuerza * 0.5f;
                p->vidaMaxima = AzarVisualCantera(m, 0.55f, 1.1f);
                float tono = AzarVisualCantera(m, 0.88f, 1.08f);
                p->color.r = (unsigned char)LimitarCantera(color.r * tono, 0.0f, 255.0f);
                p->color.g = (unsigned char)LimitarCantera(color.g * tono, 0.0f, 255.0f);
                p->color.b = (unsigned char)LimitarCantera(color.b * tono, 0.0f, 255.0f);
                break;
            }

            case PARTICULA_CANTERA_ESCOMBRO:
                p->velocidad.y = AzarVisualCantera(m, 1.6f, 4.0f) * (0.6f + fuerza * 0.25f);
                p->vidaMaxima = AzarVisualCantera(m, 0.7f, 1.2f);
                break;

            case PARTICULA_CANTERA_CHISPA:
                p->velocidad.y = AzarVisualCantera(m, 2.0f, 4.8f);
                p->vidaMaxima = AzarVisualCantera(m, 0.25f, 0.55f);
                p->color = { 255, (unsigned char)AzarVisualCantera(m, 170.0f, 235.0f), (unsigned char)AzarVisualCantera(m, 60.0f, 120.0f), 255 };
                break;

            case PARTICULA_CANTERA_CONFETI:
                p->velocidad.y = AzarVisualCantera(m, 3.2f, 6.4f);
                p->vidaMaxima = AzarVisualCantera(m, 1.6f, 2.6f);
                if (color.a == 0)
                    p->color = CONFETI[(int)AzarVisualCantera(m, 0.0f, 4.99f)];
                break;
        }

        p->vida = p->vidaMaxima;
    }
}


static void ActivarDestelloCantera(MinijuegoCanteraFuga& m, Vector3 posicion)
{
    int elegido = 0;
    float masViejo = -1.0f;

    for (int i = 0; i < MAX_DESTELLOS_CANTERA_FUGA; i++)
    {
        if (!m.destellos[i].activo)
        {
            elegido = i;
            break;
        }

        if (m.destellos[i].tiempo > masViejo)
        {
            masViejo = m.destellos[i].tiempo;
            elegido = i;
        }
    }

    m.destellos[elegido].activo = true;
    m.destellos[elegido].posicion = posicion;
    m.destellos[elegido].tiempo = 0.0f;
}


static void ActualizarParticulasCantera(MinijuegoCanteraFuga& m, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICULAS_CANTERA_FUGA; i++)
    {
        ParticulaCanteraFuga& p = m.particulas[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        if (p.tipo == PARTICULA_CANTERA_POLVO)
        {
            float freno = 1.0f - 1.8f * deltaTime;
            if (freno < 0.0f) freno = 0.0f;
            p.velocidad.x *= freno;
            p.velocidad.z *= freno;
            p.velocidad.y = p.velocidad.y * freno + 0.35f * deltaTime;
        }
        else if (p.tipo == PARTICULA_CANTERA_CONFETI)
        {
            float freno = 1.0f - 1.2f * deltaTime;
            if (freno < 0.0f) freno = 0.0f;
            p.velocidad.x *= freno;
            p.velocidad.z *= freno;
            p.velocidad.y -= 5.0f * deltaTime;
            if (p.velocidad.y < -1.4f) p.velocidad.y = -1.4f;
        }
        else
        {
            p.velocidad.y -= 12.0f * deltaTime;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        float suelo = SueloVisualCantera(p.posicion.x, p.posicion.z) + p.tamano * 0.5f;
        if (p.tipo != PARTICULA_CANTERA_POLVO && p.posicion.y < suelo)
        {
            p.posicion.y = suelo;

            if (p.tipo == PARTICULA_CANTERA_ESCOMBRO)
            {
                p.velocidad.y = -p.velocidad.y * 0.35f;
                p.velocidad.x *= 0.6f;
                p.velocidad.z *= 0.6f;
            }
            else
            {
                p.velocidad = { 0.0f, 0.0f, 0.0f };
            }
        }
    }

    for (int i = 0; i < MAX_DESTELLOS_CANTERA_FUGA; i++)
    {
        if (!m.destellos[i].activo) continue;
        m.destellos[i].tiempo += deltaTime;
        if (m.destellos[i].tiempo > 0.6f) m.destellos[i].activo = false;
    }
}


// Humo del motor de la tolva y motas de polvo en la luz de los focos.
// Es continuo: no depende de la recarga ni anticipa ningun lanzamiento.
static void ActualizarAmbienteCantera(MinijuegoCanteraFuga& m, float deltaTime)
{
    if (FactorCalidadGrafica(CalidadParticulas()) < 0.5f) return;

    m.temporizadorAmbiente -= deltaTime;
    if (m.temporizadorAmbiente > 0.0f) return;

    m.temporizadorAmbiente = AzarVisualCantera(m, 0.10f, 0.22f);

    ParticulaCanteraFuga* humo = ReservarParticulaCantera(m);
    if (humo != nullptr)
    {
        *humo = {};
        humo->activa = true;
        humo->tipo = PARTICULA_CANTERA_POLVO;
        humo->posicion = { m.posicionTolvaX + 1.18f, 6.55f, -10.30f };
        humo->velocidad = { AzarVisualCantera(m, -0.10f, 0.25f), AzarVisualCantera(m, 0.55f, 0.95f), AzarVisualCantera(m, -0.15f, 0.05f) };
        humo->vidaMaxima = AzarVisualCantera(m, 0.9f, 1.4f);
        humo->vida = humo->vidaMaxima;
        humo->tamano = AzarVisualCantera(m, 0.14f, 0.22f);
        humo->color = { 92, 88, 86, 150 };
    }

    ParticulaCanteraFuga* mota = ReservarParticulaCantera(m);
    if (mota != nullptr)
    {
        float z = AzarVisualCantera(m, Z_CIMA_CANTERA + 1.0f, Z_BASE_CANTERA);
        *mota = {};
        mota->activa = true;
        mota->tipo = PARTICULA_CANTERA_POLVO;
        mota->posicion = { AzarVisualCantera(m, -5.5f, 5.5f), AlturaPendienteCantera(z) + AzarVisualCantera(m, 0.6f, 3.2f), z };
        mota->velocidad = { AzarVisualCantera(m, -0.18f, 0.18f), AzarVisualCantera(m, 0.02f, 0.18f), AzarVisualCantera(m, -0.12f, 0.12f) };
        mota->vidaMaxima = AzarVisualCantera(m, 2.0f, 3.4f);
        mota->vida = mota->vidaMaxima;
        mota->tamano = 0.05f;
        mota->color = { 255, 232, 190, 120 };
    }
}


static int BuscarEscaladorMasAdelantado(
    const MinijuegoCanteraFuga& minijuego,
    const JugadorPrueba jugadores[]
)
{
    int mejor = -1;
    float mejorZ = 100000.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo
        )
        {
            continue;
        }

        if (jugadores[i].posicion.z < mejorZ)
        {
            mejorZ = jugadores[i].posicion.z;
            mejor = i;
        }
    }

    return mejor;
}


static void FinalizarCantera(
    MinijuegoCanteraFuga& minijuego,
    bool ganaSolo
)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

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
        resultadoJugador.puntuacionMinijuego = esSolo
            ? minijuego.impactosSolo * 100
            : (int)std::lround(
                minijuego.estadosJugadores[i].progresoMaximo * 1000.0f
            );
    }

    minijuego.fase = FASE_CANTERA_TERMINADO;

    // Cierre: confeti en la meta si llegan los escaladores; chispas y
    // polvo en la tolva si aguanta el operador.
    if (ganaSolo)
    {
        Vector3 tolva = { minijuego.posicionTolvaX, 6.4f, -9.9f };
        EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_CHISPA, tolva, 22, 2.6f, 0.07f, WHITE);
        EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_POLVO, tolva, 14, 2.4f, 0.34f, COLOR_POLVO_CANTERA);
    }
    else
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);

        for (int k = 0; k < 3; k++)
        {
            Vector3 meta = { -3.0f + 3.0f * (float)k, AlturaPendienteCantera(Z_CIMA_CANTERA) + 0.4f, Z_CIMA_CANTERA + 0.6f };
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_CONFETI, meta, 22, 2.2f, 0.11f, Color{ 0, 0, 0, 0 });
        }
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static int ContarEscaladoresCantera(const MinijuegoCanteraFuga& minijuego)
{
    int escaladores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (i != minijuego.indiceSolo && minijuego.resultado.participantes[i].participo)
        {
            escaladores++;
        }
    }

    return escaladores < 1 ? 1 : (escaladores > 3 ? 3 : escaladores);
}


// Con mas escaladores solo importa el que lidera, asi que el operador
// necesita rocas mas seguidas y golpes que frenen mas para seguir en juego.
static float RecargaRocaCantera(const MinijuegoCanteraFuga& minijuego)
{
    const float recargas[3] = { 0.95f, 0.45f, 0.15f };
    return recargas[ContarEscaladoresCantera(minijuego) - 1];
}


static float AturdimientoCantera(const MinijuegoCanteraFuga& minijuego)
{
    return DURACION_ATURDIDO_CANTERA + 0.2f * (float)(ContarEscaladoresCantera(minijuego) - 1);
}


static void LanzarRocaCantera(MinijuegoCanteraFuga& minijuego)
{
    if (minijuego.cooldownRoca > 0.0f)
    {
        return;
    }

    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        RocaCanteraFuga& roca = minijuego.rocas[i];

        if (roca.activa)
        {
            continue;
        }

        roca.activa = true;
        roca.radio = GetRandomValue(64, 82) / 100.0f;
        roca.posicion =
        {
            minijuego.posicionTolvaX,
            AlturaPendienteCantera(Z_CIMA_CANTERA) + roca.radio,
            Z_CIMA_CANTERA + 0.35f
        };
        roca.velocidadX = GetRandomValue(-65, 65) / 100.0f;
        roca.velocidadZ = GetRandomValue(61, 73) / 10.0f;
        minijuego.cooldownRoca = RecargaRocaCantera(minijuego);

        // Visual / sonido: suceden en el mismo instante en que aparece la
        // roca, asi que no anticipan nada.
        roca.giro = 0.0f;
        roca.giroLateral = 0.0f;
        roca.temporizadorPolvo = 0.0f;
        roca.semilla = ++minijuego.contadorRocas;
        minijuego.retrocesoTolva = 1.0f;
        EmitirParticulasCantera(
            minijuego, PARTICULA_CANTERA_POLVO,
            { roca.posicion.x, roca.posicion.y - roca.radio * 0.4f, roca.posicion.z - 0.2f },
            5, 1.4f, 0.26f, COLOR_POLVO_CANTERA
        );

        if (minijuego.esperaSonidoLanzar <= 0.0f)
        {
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_DISPARO);
            minijuego.esperaSonidoLanzar = 0.22f;
        }
        return;
    }
}


static InputMinijuegoParticipante CrearEntradaBotEscalador(
    const MinijuegoCanteraFuga& minijuego,
    int indiceJugador,
    const JugadorPrueba& jugador
)
{
    InputMinijuegoParticipante entrada{};
    entrada.adelante = true;

    float objetivoX = (indiceJugador - 1.5f) * 1.25f;
    float peligroMasCercano = 100000.0f;
    const RocaCanteraFuga* rocaPeligrosa = nullptr;

    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        const RocaCanteraFuga& roca = minijuego.rocas[i];

        if (!roca.activa)
        {
            continue;
        }

        float dz = jugador.posicion.z - roca.posicion.z;
        float dx = std::fabs(jugador.posicion.x - roca.posicion.x);

        if (dz > -0.8f && dz < 4.5f && dx < 1.55f && dz < peligroMasCercano)
        {
            peligroMasCercano = dz;
            rocaPeligrosa = &roca;
        }
    }

    if (rocaPeligrosa != nullptr)
    {
        objetivoX = jugador.posicion.x <= rocaPeligrosa->posicion.x
            ? -LIMITE_X_CANTERA + 0.35f
            : LIMITE_X_CANTERA - 0.35f;
    }

    if (jugador.posicion.x < objetivoX - 0.18f)
        entrada.derecha = true;
    else if (jugador.posicion.x > objetivoX + 0.18f)
        entrada.izquierda = true;

    return entrada;
}


static void ActualizarControlSoloCantera(
    MinijuegoCanteraFuga& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    Participante participantes[]
)
{
    bool lanzar = false;
    Participante& participanteSolo = participantes[minijuego.indiceSolo];

    if (participanteSolo.esBot || !participanteSolo.conectado)
    {
        minijuego.tiempoDecisionBot -= deltaTime;

        int objetivo = BuscarEscaladorMasAdelantado(minijuego, jugadores);

        if (objetivo >= 0)
        {
            float xObjetivo = jugadores[objetivo].posicion.x;
            float paso = 5.2f * deltaTime;

            if (minijuego.posicionTolvaX < xObjetivo - paso)
                minijuego.posicionTolvaX += paso;
            else if (minijuego.posicionTolvaX > xObjetivo + paso)
                minijuego.posicionTolvaX -= paso;
            else
                minijuego.posicionTolvaX = xObjetivo;
        }

        bool alineado =
            objetivo >= 0 &&
            std::fabs(minijuego.posicionTolvaX - jugadores[objetivo].posicion.x) < 0.9f;

        if (minijuego.tiempoDecisionBot <= 0.0f && alineado)
        {
            lanzar = true;
            minijuego.tiempoDecisionBot = GetRandomValue(30, 70) / 100.0f * RecargaRocaCantera(minijuego) / RECARGA_ROCA_CANTERA;
        }
    }
    else
    {
        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participanteSolo);

        float direccion =
            (entrada.derecha ? 1.0f : 0.0f) -
            (entrada.izquierda ? 1.0f : 0.0f);

        minijuego.posicionTolvaX += direccion * 5.2f * deltaTime;
        lanzar = entrada.golpear;
    }

    minijuego.posicionTolvaX = LimitarCantera(
        minijuego.posicionTolvaX,
        -LIMITE_X_CANTERA + 0.42f,
        LIMITE_X_CANTERA - 0.42f
    );

    JugadorPrueba& jugadorSolo = jugadores[minijuego.indiceSolo];
    jugadorSolo.posicion =
    {
        minijuego.posicionTolvaX,
        AlturaPendienteCantera(Z_CIMA_CANTERA) + jugadorSolo.tamano.y * 0.5f,
        Z_CIMA_CANTERA - 1.45f
    };
    jugadorSolo.direccionMirada = { 0.0f, 0.0f, 1.0f };
    jugadorSolo.enSuelo = true;
    jugadorSolo.cayendo = false;

    if (lanzar)
    {
        LanzarRocaCantera(minijuego);
    }
}


static void ActualizarRocasCantera(
    MinijuegoCanteraFuga& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[]
)
{
    for (int rocaIndice = 0; rocaIndice < MAX_ROCAS_CANTERA_FUGA; rocaIndice++)
    {
        RocaCanteraFuga& roca = minijuego.rocas[rocaIndice];

        if (!roca.activa)
        {
            continue;
        }

        roca.velocidadZ += 1.85f * deltaTime;
        roca.posicion.x += roca.velocidadX * deltaTime;
        roca.posicion.z += roca.velocidadZ * deltaTime;

        float limiteRoca = LIMITE_X_CANTERA - roca.radio;
        bool reboto = false;

        if (roca.posicion.x < -limiteRoca)
        {
            roca.posicion.x = -limiteRoca;
            roca.velocidadX = std::fabs(roca.velocidadX) + 0.35f;
            reboto = true;
        }
        else if (roca.posicion.x > limiteRoca)
        {
            roca.posicion.x = limiteRoca;
            roca.velocidadX = -std::fabs(roca.velocidadX) - 0.35f;
            reboto = true;
        }

        roca.posicion.y = AlturaPendienteCantera(roca.posicion.z) + roca.radio;

        // Visual: rueda (giro segun lo recorrido) y levanta polvo.
        roca.giro += roca.velocidadZ * deltaTime / roca.radio;
        roca.giroLateral += roca.velocidadX * deltaTime / roca.radio;
        roca.temporizadorPolvo -= deltaTime;

        Vector3 contacto = { roca.posicion.x, roca.posicion.y - roca.radio + 0.06f, roca.posicion.z - roca.radio * 0.35f };

        if (roca.temporizadorPolvo <= 0.0f)
        {
            float factor = FactorCalidadGrafica(CalidadParticulas());
            roca.temporizadorPolvo = 0.075f / (factor > 0.25f ? factor : 0.25f);
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_POLVO, contacto, 1, 0.7f, 0.24f, COLOR_POLVO_CANTERA);
        }

        if (reboto)
        {
            Vector3 lado = { roca.posicion.x + (roca.posicion.x > 0.0f ? roca.radio : -roca.radio), contacto.y + 0.2f, roca.posicion.z };
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_POLVO, lado, 4, 1.3f, 0.24f, COLOR_POLVO_CANTERA);
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_ESCOMBRO, lado, 2, 1.2f, 0.08f, Color{ 150, 116, 80, 255 });
        }

        if (roca.posicion.z > Z_BASE_CANTERA + 1.8f)
        {
            // Visual: se parte contra la barrera del pie de la rampa.
            Vector3 pie = { roca.posicion.x, roca.posicion.y - roca.radio * 0.3f, roca.posicion.z + roca.radio * 0.5f };
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_ESCOMBRO, pie, 7, 2.0f, 0.13f, COLOR_ROCA_CANTERA);
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_POLVO, pie, 7, 1.8f, 0.34f, COLOR_POLVO_CANTERA);

            if (minijuego.esperaSonidoRoca <= 0.0f)
            {
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
                minijuego.esperaSonidoRoca = 0.45f;
            }

            roca.activa = false;
            continue;
        }

        for (int jugadorIndice = 0; jugadorIndice < MAX_PARTICIPANTES; jugadorIndice++)
        {
            if (
                jugadorIndice == minijuego.indiceSolo ||
                !minijuego.resultado.participantes[jugadorIndice].participo ||
                minijuego.estadosJugadores[jugadorIndice].tiempoAturdido > 0.0f
            )
            {
                continue;
            }

            JugadorPrueba& jugador = jugadores[jugadorIndice];
            float dx = jugador.posicion.x - roca.posicion.x;
            float dz = jugador.posicion.z - roca.posicion.z;
            float radioChoque = roca.radio + jugador.tamano.x * 0.46f;

            if (dx * dx + dz * dz > radioChoque * radioChoque)
            {
                continue;
            }

            EstadoJugadorCanteraFuga& estado =
                minijuego.estadosJugadores[jugadorIndice];
            estado.tiempoAturdido = AturdimientoCantera(minijuego);
            estado.impulsoLateral = dx >= 0.0f ? 2.1f : -2.1f;
            estado.impactosRecibidos++;
            minijuego.impactosSolo++;

            jugador.aplastado = true;
            jugador.tiempoAplastado = estado.tiempoAturdido;
            roca.velocidadX -= estado.impulsoLateral * 0.34f;

            // Feedback del golpe: escombros, polvo, chispas, anillo,
            // temblor leve y sonido. El panel de la tolva tambien destella.
            Vector3 punto =
            {
                jugador.posicion.x - dx * 0.45f,
                AlturaPendienteCantera(jugador.posicion.z) + 0.55f,
                jugador.posicion.z - dz * 0.45f
            };
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_ESCOMBRO, punto, 10, 2.6f, 0.12f, COLOR_ROCA_CANTERA);
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_POLVO, punto, 12, 2.2f, 0.32f, COLOR_POLVO_CANTERA);
            EmitirParticulasCantera(minijuego, PARTICULA_CANTERA_CHISPA, punto, 7, 2.0f, 0.06f, WHITE);
            ActivarDestelloCantera(minijuego, { jugador.posicion.x, 0.0f, jugador.posicion.z });
            ActivarTemblorCamaraGeneral(0.14f, 0.24f);
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
            minijuego.destelloOperador = 1.0f;
        }
    }
}


void MinijuegoCanteraFuga::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        rocas[i] = {};
    }

    camara.position = { 11.8f, 12.7f, 16.8f };
    camara.target = { 0.0f, 2.15f, -0.8f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 49.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CANTERA_PREPARACION;
    indiceSolo = -1;
    impactosSolo = 0;
    posicionTolvaX = 0.0f;
    cooldownRoca = 0.0f;
    tiempoDecisionBot = 0.9f;
    tiempoPreparacion = DURACION_PREPARACION_CANTERA;
    tiempoRestante = DURACION_PARTIDA_CANTERA;
    tiempoAnimacion = 0.0f;

    // Estado visual (el puntero de audio lo asigna el gestor y se conserva).
    for (int i = 0; i < MAX_PARTICULAS_CANTERA_FUGA; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_DESTELLOS_CANTERA_FUGA; i++)
    {
        destellos[i] = {};
    }

    contadorRocas = 0;
    retrocesoTolva = 0.0f;
    destelloOperador = 0.0f;
    esperaSonidoLanzar = 0.0f;
    esperaSonidoRoca = 0.0f;
    temporizadorAmbiente = 0.0f;
}


void MinijuegoCanteraFuga::Reiniciar(
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

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_CANTERA_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    const float spawnsX[MAX_PARTICIPANTES] =
    {
        -2.8f, -0.95f, 0.95f, 2.8f
    };

    int cursorEquipo = 0;
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool esSolo = i == indiceSolo;
        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;

        Vector3 spawn = esSolo
            ? Vector3{ 0.0f, AlturaPendienteCantera(Z_CIMA_CANTERA) + 0.70f, Z_CIMA_CANTERA - 1.45f }
            : Vector3{
                spawnsX[cursorEquipo % MAX_PARTICIPANTES],
                AlturaPendienteCantera(Z_BASE_CANTERA - 0.45f) + 0.70f,
                Z_BASE_CANTERA - 0.45f
            };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = esSolo
            ? Vector3{ 0.0f, 0.0f, 1.0f }
            : Vector3{ 0.0f, 0.0f, -1.0f };

        if (!esSolo)
        {
            cursorEquipo++;
        }
    }
}


void MinijuegoCanteraFuga::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    // Visual: sigue animando tambien con el tablon final.
    ActualizarParticulasCantera(*this, deltaTime);
    ActualizarAmbienteCantera(*this, deltaTime);
    retrocesoTolva -= deltaTime * 4.0f;
    destelloOperador -= deltaTime * 2.5f;
    esperaSonidoLanzar -= deltaTime;
    esperaSonidoRoca -= deltaTime;
    if (retrocesoTolva < 0.0f) retrocesoTolva = 0.0f;
    if (destelloOperador < 0.0f) destelloOperador = 0.0f;

    if (
        fase == FASE_CANTERA_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_CANTERA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CANTERA_JUGANDO;
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante, 5.0f);
    cooldownRoca -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
    if (cooldownRoca < 0.0f) cooldownRoca = 0.0f;

    ActualizarControlSoloCantera(*this, deltaTime, jugadores, participantes);

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (i == indiceSolo || !resultado.participantes[i].participo)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        EstadoJugadorCanteraFuga& estado = estadosJugadores[i];

        if (estado.tiempoAturdido > 0.0f)
        {
            estado.tiempoAturdido -= deltaTime;
            jugador.posicion.z += 3.7f * deltaTime;
            jugador.posicion.x += estado.impulsoLateral * deltaTime;

            if (estado.tiempoAturdido <= 0.0f)
            {
                estado.tiempoAturdido = 0.0f;
                jugador.aplastado = false;
                jugador.tiempoAplastado = 0.0f;
            }
        }
        else
        {
            InputMinijuegoParticipante entrada{};

            if (participantes[i].esBot || !participantes[i].conectado)
            {
                entrada = CrearEntradaBotEscalador(*this, i, jugador);
            }
            else
            {
                entrada = LeerInputMinijuegoParticipante(participantes[i]);
            }

            float moverX =
                (entrada.derecha ? 1.0f : 0.0f) -
                (entrada.izquierda ? 1.0f : 0.0f);
            float moverZ =
                (entrada.atras ? 1.0f : 0.0f) -
                (entrada.adelante ? 1.0f : 0.0f);
            float longitud = std::sqrt(moverX * moverX + moverZ * moverZ);

            if (longitud > 0.01f)
            {
                moverX /= longitud;
                moverZ /= longitud;
                jugador.posicion.x += moverX * VELOCIDAD_ESCALADOR_CANTERA * deltaTime;
                jugador.posicion.z += moverZ * VELOCIDAD_ESCALADOR_CANTERA * deltaTime;
                jugador.direccionMirada = { moverX, 0.0f, moverZ };
            }
        }

        jugador.posicion.x = LimitarCantera(
            jugador.posicion.x,
            -LIMITE_X_CANTERA,
            LIMITE_X_CANTERA
        );
        jugador.posicion.z = LimitarCantera(
            jugador.posicion.z,
            Z_CIMA_CANTERA,
            Z_BASE_CANTERA
        );
        jugador.posicion.y = AlturaPendienteCantera(jugador.posicion.z) + 0.70f;
        jugador.enSuelo = true;
        jugador.cayendo = false;

        float progreso = CalcularProgresoCantera(jugador.posicion.z);
        if (progreso > estado.progresoMaximo)
        {
            estado.progresoMaximo = progreso;
        }

        if (jugador.posicion.z <= Z_CIMA_CANTERA + 0.08f)
        {
            FinalizarCantera(*this, false);
            return;
        }
    }

    ActualizarRocasCantera(*this, deltaTime, jugadores);

    if (tiempoRestante <= 0.0f)
    {
        FinalizarCantera(*this, true);
    }
}


//==================================================
// VISUAL: AYUDAS DE DIBUJO
//==================================================
//
// Cantera al atardecer: rampa de losas, paredes escalonadas con estratos,
// focos de obra y una tolva sobre rieles. Las rocas peligrosas son las
// unicas piezas gris azuladas facetadas que hay sobre la rampa; la
// decoracion usa tonos calidos y queda fuera de la zona de juego.
//==================================================

static const Color HORIZONTE_CANTERA = { 246, 196, 140, 255 };
static const Color SUELO_CANTERA = { 132, 98, 70, 255 };
static const float ALTO_ESTRATO_CANTERA = 0.62f;
static const float Z_PARED_FONDO_CANTERA = -11.9f;
static const float Z_FRENTE_PARED_CANTERA = 9.8f;
static const int SEGMENTOS_PARED_CANTERA = 9;

static const Color ESTRATOS_CANTERA[6] =
{
    { 192, 138, 92, 255 },
    { 166, 112, 78, 255 },
    { 208, 166, 114, 255 },
    { 146, 100, 72, 255 },
    { 184, 128, 88, 255 },
    { 128, 90, 68, 255 }
};


static float Hash01Cantera(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.0f;
}


static Color TinteCantera(Color c, float f)
{
    return
    {
        (unsigned char)LimitarCantera((float)c.r * f, 0.0f, 255.0f),
        (unsigned char)LimitarCantera((float)c.g * f, 0.0f, 255.0f),
        (unsigned char)LimitarCantera((float)c.b * f, 0.0f, 255.0f),
        c.a
    };
}


static Color MezclaCantera(Color a, Color b, float t)
{
    t = LimitarCantera(t, 0.0f, 1.0f);
    return
    {
        (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t),
        (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t),
        (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t),
        (unsigned char)((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static void CulloCantera(bool activo)
{
    rlDrawRenderBatchActive();
    if (activo) rlEnableBackfaceCulling();
    else rlDisableBackfaceCulling();
}


// Caja con cada cara a un brillo distinto: luz calida desde arriba y desde
// el lado de la camara (+X, +Z). La niebla acerca el color al horizonte.
static void CajaFacetadaCantera(Vector3 centro, Vector3 tamano, Color color, float niebla = 0.0f)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = MezclaCantera(TinteCantera(color, 1.14f), HORIZONTE_CANTERA, niebla);
    Color frente = MezclaCantera(TinteCantera(color, 0.86f), HORIZONTE_CANTERA, niebla);
    Color atras = MezclaCantera(TinteCantera(color, 0.56f), HORIZONTE_CANTERA, niebla);
    Color derecha = MezclaCantera(TinteCantera(color, 0.97f), HORIZONTE_CANTERA, niebla);
    Color izquierda = MezclaCantera(TinteCantera(color, 0.62f), HORIZONTE_CANTERA, niebla);

    DrawTriangle3D({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, tapa);
    DrawTriangle3D({ x0, y1, z0 }, { x1, y1, z1 }, { x1, y1, z0 }, tapa);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, atras);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, atras);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, derecha);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, derecha);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, izquierda);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z1 }, { x0, y1, z0 }, izquierda);
}


static void CajaBordeCantera(Vector3 centro, Vector3 tamano, Color color)
{
    CajaFacetadaCantera(centro, tamano, color);
    DrawCubeWiresV(centro, tamano, Fade(BLACK, 0.25f));
}


static void CajaGiradaCantera(Vector3 centro, Vector3 tamano, float anguloGrados, Color color)
{
    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, centro.z);
    rlRotatef(anguloGrados, 0.0f, 1.0f, 0.0f);
    CajaFacetadaCantera({ 0.0f, 0.0f, 0.0f }, tamano, color);
    rlPopMatrix();
}


static Vector3 PuntoRampaCantera(float x, float z, float elevacion)
{
    return { x, AlturaPendienteCantera(z) + elevacion, z };
}


// Cuadrilatero sobre la rampa (mismo plano que la colision), visible desde arriba.
static void DibujarCuadroPendienteCantera(
    float x0,
    float x1,
    float zAbajo,
    float zArriba,
    float elevacion,
    Color color
)
{
    Vector3 a = PuntoRampaCantera(x0, zAbajo, elevacion);
    Vector3 b = PuntoRampaCantera(x1, zAbajo, elevacion);
    Vector3 c = PuntoRampaCantera(x1, zArriba, elevacion);
    Vector3 d = PuntoRampaCantera(x0, zArriba, elevacion);
    DrawTriangle3D(a, c, d, color);
    DrawTriangle3D(a, b, c, color);
}


// Disco apoyado sobre la rampa (sombras de contacto).
static void DiscoRampaCantera(float x, float z, float radio, float elevacion, Color color)
{
    const int LADOS = 12;
    Vector3 centro = PuntoRampaCantera(x, z, elevacion);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        DrawTriangle3D(
            centro,
            PuntoRampaCantera(x + std::cos(a1) * radio, z + std::sin(a1) * radio, elevacion),
            PuntoRampaCantera(x + std::cos(a0) * radio, z + std::sin(a0) * radio, elevacion),
            color
        );
    }
}


// Anillo apoyado sobre la rampa (marca de jugador, destello de golpe).
static void AnilloRampaCantera(float x, float z, float radioInterno, float radioExterno, float elevacion, Color color)
{
    const int LADOS = 20;

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);
        Vector3 i0 = PuntoRampaCantera(x + c0 * radioInterno, z + s0 * radioInterno, elevacion);
        Vector3 i1 = PuntoRampaCantera(x + c1 * radioInterno, z + s1 * radioInterno, elevacion);
        Vector3 e0 = PuntoRampaCantera(x + c0 * radioExterno, z + s0 * radioExterno, elevacion);
        Vector3 e1 = PuntoRampaCantera(x + c1 * radioExterno, z + s1 * radioExterno, elevacion);
        DrawTriangle3D(i0, e1, e0, color);
        DrawTriangle3D(i0, i1, e1, color);
    }
}


// Disco de luz suave (centro opaco, borde transparente). Solo con mezcla
// aditiva y sin escribir profundidad.
static void ResplandorCantera(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 14;
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


// Charco de luz que sigue la pendiente.
static void ResplandorRampaCantera(float x, float z, float radio, Color color)
{
    const int LADOS = 16;
    Vector3 centro = PuntoRampaCantera(x, z, 0.06f);
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = PuntoRampaCantera(x + std::cos(a0) * radio, z + std::sin(a0) * radio, 0.06f);
        Vector3 p1 = PuntoRampaCantera(x + std::cos(a1) * radio, z + std::sin(a1) * radio, 0.06f);

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p1.x, p1.y, p1.z);
    }

    rlEnd();
}


//==================================================
// VISUAL: ROCAS FACETADAS
//==================================================

// Icosaedro deformado por hash y girado segun lo que rodo. Cada cara se
// sombrea segun su normal: se lee como piedra tallada, no como esfera.
static void DibujarRocaFacetadaCantera(
    Vector3 centro,
    float radio,
    float giro,
    float giroLateral,
    int semilla,
    Color base
)
{
    static const float F = 1.618034f;
    static const Vector3 VERTICES[12] =
    {
        { -1.0f, F, 0.0f }, { 1.0f, F, 0.0f }, { -1.0f, -F, 0.0f }, { 1.0f, -F, 0.0f },
        { 0.0f, -1.0f, F }, { 0.0f, 1.0f, F }, { 0.0f, -1.0f, -F }, { 0.0f, 1.0f, -F },
        { F, 0.0f, -1.0f }, { F, 0.0f, 1.0f }, { -F, 0.0f, -1.0f }, { -F, 0.0f, 1.0f }
    };
    static const unsigned char CARAS[20][3] =
    {
        { 0, 11, 5 }, { 0, 5, 1 }, { 0, 1, 7 }, { 0, 7, 10 }, { 0, 10, 11 },
        { 1, 5, 9 }, { 5, 11, 4 }, { 11, 10, 2 }, { 10, 7, 6 }, { 7, 1, 8 },
        { 3, 9, 4 }, { 3, 4, 2 }, { 3, 2, 6 }, { 3, 6, 8 }, { 3, 8, 9 },
        { 4, 9, 5 }, { 2, 4, 11 }, { 6, 2, 10 }, { 8, 6, 7 }, { 9, 8, 1 }
    };

    float cx = std::cos(giro), sx = std::sin(giro);
    float cz = std::cos(-giroLateral), sz = std::sin(-giroLateral);
    Vector3 p[12];

    for (int i = 0; i < 12; i++)
    {
        Vector3 v = Vector3Scale(
            Vector3Normalize(VERTICES[i]),
            radio * (0.82f + 0.24f * Hash01Cantera(semilla, i))
        );
        v.y *= 0.9f;

        float y = v.y * cx - v.z * sx;
        float z = v.y * sx + v.z * cx;
        v.y = y;
        v.z = z;

        float x = v.x * cz - v.y * sz;
        y = v.x * sz + v.y * cz;
        v.x = x;
        v.y = y;

        p[i] = v;
    }

    const Vector3 luz = Vector3Normalize({ 0.45f, 0.85f, 0.35f });

    for (int f = 0; f < 20; f++)
    {
        Vector3 a = p[CARAS[f][0]];
        Vector3 b = p[CARAS[f][1]];
        Vector3 c = p[CARAS[f][2]];
        Vector3 n = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));

        if (Vector3DotProduct(n, Vector3Add(Vector3Add(a, b), c)) < 0.0f)
        {
            Vector3 t = b;
            b = c;
            c = t;
            n = Vector3Negate(n);
        }

        n = Vector3Normalize(n);
        float d = Vector3DotProduct(n, luz);
        float brillo = 0.50f + 0.68f * (d > 0.0f ? d : 0.0f) + 0.10f * (n.y > 0.0f ? n.y : 0.0f);

        DrawTriangle3D(Vector3Add(centro, a), Vector3Add(centro, b), Vector3Add(centro, c), TinteCantera(base, brillo));
    }
}


//==================================================
// VISUAL: CIELO, FONDO Y SUELO
//==================================================

// Cielo de atardecer en 2D, antes de la escena 3D.
static void DibujarCieloCantera(float tiempo)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangleGradientV(0, 0, ancho, alto, Color{ 94, 124, 178, 255 }, HORIZONTE_CANTERA);

    for (int k = 0; k < 5; k++)
    {
        float recorrido = (float)ancho * 1.4f;
        float x = std::fmod((float)ancho * (0.08f + 0.23f * (float)k) + tiempo * (5.0f + 1.5f * (float)k), recorrido) - (float)ancho * 0.2f;
        float y = (float)alto * (0.20f + 0.045f * (float)(k % 3));
        DrawEllipse((int)x, (int)y, (float)ancho * (0.07f + 0.02f * (float)(k % 2)), (float)alto * 0.016f, Fade(Color{ 255, 236, 214, 255 }, 0.38f));
        DrawEllipse((int)(x + (float)ancho * 0.04f), (int)(y - (float)alto * 0.01f), (float)ancho * 0.045f, (float)alto * 0.014f, Fade(Color{ 255, 240, 222, 255 }, 0.30f));
    }
}


// Suelo de la fosa: disco macizo y un anillo que se funde con el horizonte.
static void DibujarSueloFosaCantera()
{
    const int LADOS = 48;
    const float Y = -0.22f;
    const float INTERNO = 34.0f;
    const float EXTERNO = 160.0f;
    Color lejos = MezclaCantera(SUELO_CANTERA, HORIZONTE_CANTERA, 0.88f);

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        rlColor4ub(SUELO_CANTERA.r, SUELO_CANTERA.g, SUELO_CANTERA.b, 255);
        rlVertex3f(0.0f, Y, 0.0f);
        rlVertex3f(c1 * INTERNO, Y, s1 * INTERNO);
        rlVertex3f(c0 * INTERNO, Y, s0 * INTERNO);

        rlColor4ub(SUELO_CANTERA.r, SUELO_CANTERA.g, SUELO_CANTERA.b, 255);
        rlVertex3f(c0 * INTERNO, Y, s0 * INTERNO);
        rlVertex3f(c1 * INTERNO, Y, s1 * INTERNO);
        rlColor4ub(lejos.r, lejos.g, lejos.b, 255);
        rlVertex3f(c1 * EXTERNO, Y, s1 * EXTERNO);

        rlColor4ub(SUELO_CANTERA.r, SUELO_CANTERA.g, SUELO_CANTERA.b, 255);
        rlVertex3f(c0 * INTERNO, Y, s0 * INTERNO);
        rlColor4ub(lejos.r, lejos.g, lejos.b, 255);
        rlVertex3f(c1 * EXTERNO, Y, s1 * EXTERNO);
        rlVertex3f(c0 * EXTERNO, Y, s0 * EXTERNO);
    }

    rlEnd();
}


static void MontanaCantera(Vector3 base, float radio, float alto, int semilla, Color color, float niebla)
{
    const int LADOS = 6;
    Vector3 cima =
    {
        base.x + (Hash01Cantera(semilla, 1) - 0.5f) * radio * 0.5f,
        base.y + alto,
        base.z + (Hash01Cantera(semilla, 2) - 0.5f) * radio * 0.5f
    };
    Vector3 anillo[LADOS + 1];

    for (int k = 0; k < LADOS; k++)
    {
        float a = 6.2832f * (float)k / (float)LADOS + 0.4f * Hash01Cantera(semilla, k + 3);
        float r = radio * (0.75f + 0.4f * Hash01Cantera(semilla, k + 11));
        anillo[k] = { base.x + std::cos(a) * r, base.y, base.z + std::sin(a) * r };
    }
    anillo[LADOS] = anillo[0];

    for (int k = 0; k < LADOS; k++)
    {
        float mx = (anillo[k].x + anillo[k + 1].x) * 0.5f - base.x;
        float mz = (anillo[k].z + anillo[k + 1].z) * 0.5f - base.z;
        float largo = std::sqrt(mx * mx + mz * mz);
        if (largo > 0.001f) { mx /= largo; mz /= largo; }

        float brillo = 0.78f + 0.26f * (mx * 0.75f + mz * 0.55f);
        Color c = MezclaCantera(TinteCantera(color, brillo), HORIZONTE_CANTERA, niebla);
        DrawTriangle3D(anillo[k], cima, anillo[k + 1], c);
    }
}


// Montañas lejanas (con niebla) y mesetas rojizas a media distancia.
static void DibujarFondoCantera(bool detallado)
{
    const Vector3 centro = { 0.0f, -0.22f, -5.0f };

    for (int i = 0; i < 16; i++)
    {
        float a = 6.2832f * (float)i / 16.0f + 0.3f * Hash01Cantera(i, 41);
        float d = 92.0f + 16.0f * Hash01Cantera(i, 42);
        Vector3 base = { centro.x + std::cos(a) * d, centro.y, centro.z + std::sin(a) * d };
        MontanaCantera(base, 18.0f + 10.0f * Hash01Cantera(i, 43), 16.0f + 14.0f * Hash01Cantera(i, 44), i + 100, Color{ 150, 112, 128, 255 }, 0.60f);
    }

    if (!detallado) return;

    for (int i = 0; i < 11; i++)
    {
        float a = 6.2832f * (float)i / 11.0f + 0.5f * Hash01Cantera(i, 51);
        float d = 46.0f + 12.0f * Hash01Cantera(i, 52);
        float ancho = 8.0f + 7.0f * Hash01Cantera(i, 53);
        float alto = 4.0f + 6.0f * Hash01Cantera(i, 54);
        float x = centro.x + std::cos(a) * d;
        float z = centro.z + std::sin(a) * d;
        float y = -0.22f;

        for (int capa = 0; capa < 3; capa++)
        {
            float altoCapa = alto * (capa == 0 ? 0.5f : 0.25f);
            float anchoCapa = ancho * (1.0f - 0.13f * (float)capa);
            CajaFacetadaCantera(
                { x, y + altoCapa * 0.5f, z },
                { anchoCapa, altoCapa, anchoCapa * 0.8f },
                ESTRATOS_CANTERA[(i + capa) % 6],
                0.42f
            );
            y += altoCapa;
        }
    }
}


// Parches de grava en la fosa, con variacion por hash.
static void DibujarParchesFosaCantera()
{
    const float CELDA = 1.7f;

    for (int iz = 0; iz < 14; iz++)
    {
        for (int ix = 0; ix < 16; ix++)
        {
            float x = -12.0f + ((float)ix + 0.5f) * CELDA;
            float z = -10.0f + ((float)iz + 0.5f) * CELDA;

            if (x < 5.6f && z < 12.4f) continue;
            if (Hash01Cantera(ix, iz + 300) < 0.35f) continue;

            float variacion = 0.84f + 0.26f * Hash01Cantera(ix + 13, iz);
            Color color = TinteCantera(SUELO_CANTERA, variacion);
            float m = CELDA * 0.5f - 0.06f;
            float y = -0.205f;
            DrawTriangle3D({ x - m, y, z - m }, { x - m, y, z + m }, { x + m, y, z + m }, color);
            DrawTriangle3D({ x - m, y, z - m }, { x + m, y, z + m }, { x + m, y, z - m }, color);
        }
    }
}


//==================================================
// VISUAL: PAREDES DE LA CANTERA
//==================================================

// Columna de roca en franjas horizontales. La franja depende de la altura
// en el mundo, asi los estratos quedan alineados entre columnas vecinas.
static void ColumnaEstratosCantera(
    float x0,
    float x1,
    float z0,
    float z1,
    float yBase,
    float yTope,
    int semilla,
    float niebla,
    bool detallado
)
{
    if (!detallado)
    {
        int banda = (int)((yTope + 0.22f) / ALTO_ESTRATO_CANTERA);
        CajaFacetadaCantera(
            { (x0 + x1) * 0.5f, (yBase + yTope) * 0.5f, (z0 + z1) * 0.5f },
            { x1 - x0, yTope - yBase, z1 - z0 },
            ESTRATOS_CANTERA[banda % 6],
            niebla
        );
        return;
    }

    int banda = (int)std::floor((yBase + 0.22f) / ALTO_ESTRATO_CANTERA);
    if (banda < 0) banda = 0;
    float y = yBase;

    while (y < yTope - 0.01f)
    {
        float limite = -0.22f + (float)(banda + 1) * ALTO_ESTRATO_CANTERA;
        if (limite > yTope) limite = yTope;

        float entrante = 0.09f * Hash01Cantera(semilla, banda);
        Color color = TinteCantera(ESTRATOS_CANTERA[banda % 6], 0.9f + 0.18f * Hash01Cantera(semilla + 17, banda));

        CajaFacetadaCantera(
            { (x0 + x1 - entrante) * 0.5f, (y + limite) * 0.5f, (z0 + z1 - entrante) * 0.5f },
            { x1 - x0 - entrante, limite - y, z1 - z0 - entrante },
            color,
            niebla
        );

        y = limite;
        banda++;
    }
}


static float InicioSegmentoParedCantera(int segmento)
{
    float largo = (Z_FRENTE_PARED_CANTERA - Z_PARED_FONDO_CANTERA) / (float)SEGMENTOS_PARED_CANTERA;
    return Z_PARED_FONDO_CANTERA + largo * (float)segmento;
}


// Altura de cada banco de la pared izquierda: escalonada siguiendo la rampa.
static float AlturaBancoCantera(int segmento, int banco)
{
    return AlturaPendienteCantera(InicioSegmentoParedCantera(segmento)) +
        1.3f * (float)(banco + 1) +
        0.25f * Hash01Cantera(segmento * 7 + banco, 91);
}


// Pared izquierda en bancos escalonados (como una cantera real), con
// repisas. Queda detras de la rampa vista desde la camara.
static void DibujarParedIzquierdaCantera(bool detallado)
{
    float largo = (Z_FRENTE_PARED_CANTERA - Z_PARED_FONDO_CANTERA) / (float)SEGMENTOS_PARED_CANTERA;

    for (int s = 0; s < SEGMENTOS_PARED_CANTERA; s++)
    {
        float z0 = InicioSegmentoParedCantera(s);
        float z1 = z0 + largo;

        for (int b = 0; b < 3; b++)
        {
            float xCara = -5.9f - 2.3f * (float)b;
            float xFondo = b == 2 ? -18.0f : xCara - 2.3f;
            float tope = AlturaBancoCantera(s, b);

            ColumnaEstratosCantera(xFondo, xCara, z0, z1, -0.22f, tope, s * 7 + b, 0.0f, detallado);

            if (detallado && Hash01Cantera(s, b + 40) > 0.45f)
            {
                float y = tope - 0.8f - 0.7f * Hash01Cantera(s, b + 41);
                float z = z0 + largo * (0.3f + 0.4f * Hash01Cantera(s, b + 42));
                CajaFacetadaCantera({ xCara + 0.12f, y, z }, { 0.30f, 0.14f, 0.9f + 0.5f * Hash01Cantera(s, b + 43) }, Color{ 214, 176, 126, 255 });
            }
        }
    }
}


// Pared del fondo detras de la cima y meseta a la derecha de la tolva.
static void DibujarParedFondoCantera(bool detallado)
{
    const float BANCOS_Z[4] = { Z_PARED_FONDO_CANTERA, -14.2f, -16.8f, -22.0f };
    const float TOPES[3] = { 6.7f, 8.4f, 10.2f };

    for (int s = 0; s < 10; s++)
    {
        float x0 = -18.0f + 3.1f * (float)s;
        float x1 = x0 + 3.1f;

        for (int b = 0; b < 3; b++)
        {
            float tope = TOPES[b] + 0.35f * Hash01Cantera(s, b + 60);
            float base = x0 > 5.0f ? -0.22f : 4.4f;
            ColumnaEstratosCantera(x0, x1, BANCOS_Z[b + 1], BANCOS_Z[b], base, tope, s * 5 + b + 200, 0.06f * (float)b, detallado);
        }
    }

    const float MESETA_X[4] = { 5.35f, 7.6f, 9.8f, 12.0f };
    const float MESETA_TOPE[3] = { 5.0f, 4.3f, 3.5f };

    for (int s = 0; s < 3; s++)
    {
        ColumnaEstratosCantera(MESETA_X[s], MESETA_X[s + 1], Z_PARED_FONDO_CANTERA, -8.6f, -0.22f, MESETA_TOPE[s], s + 300, 0.0f, detallado);
    }
}


// Banquina de grava a los costados de la rampa y talud derecho hasta la
// fosa, ambos con franjas de estratos.
static void DibujarTaludesCantera()
{
    const int TRAMOS = 10;
    const float X_BERMA_DERECHA = 5.35f;

    for (int k = 0; k < TRAMOS + 2; k++)
    {
        float zAbajo = k < TRAMOS
            ? Z_BASE_CANTERA + (Z_CIMA_CANTERA - Z_BASE_CANTERA) * ((float)k / (float)TRAMOS)
            : (k == TRAMOS ? Z_CIMA_CANTERA : -10.6f);
        float zArriba = k < TRAMOS
            ? Z_BASE_CANTERA + (Z_CIMA_CANTERA - Z_BASE_CANTERA) * ((float)(k + 1) / (float)TRAMOS)
            : (k == TRAMOS ? -10.6f : Z_PARED_FONDO_CANTERA);
        Color grava = TinteCantera(Color{ 168, 130, 92, 255 }, 0.92f + 0.12f * Hash01Cantera(k, 500));

        DibujarCuadroPendienteCantera(-5.95f, -4.75f, zAbajo, zArriba, 0.0f, grava);
        DibujarCuadroPendienteCantera(4.75f, X_BERMA_DERECHA, zAbajo, zArriba, 0.0f, grava);

        if (k >= TRAMOS || zArriba < -8.6f) continue;

        // Talud derecho: franjas de estrato entre la berma y la fosa.
        float h0 = AlturaPendienteCantera(zAbajo);
        float h1 = AlturaPendienteCantera(zArriba);
        float hMax = h0 > h1 ? h0 : h1;
        int banda = 0;

        for (float ya = -0.22f; ya < hMax - 0.01f; ya += ALTO_ESTRATO_CANTERA, banda++)
        {
            float yb = ya + ALTO_ESTRATO_CANTERA;
            float ya0 = ya < h0 ? ya : h0, yb0 = yb < h0 ? yb : h0;
            float ya1 = ya < h1 ? ya : h1, yb1 = yb < h1 ? yb : h1;
            Vector3 a = { X_BERMA_DERECHA + 0.55f * (h0 - ya0), ya0, zAbajo };
            Vector3 b = { X_BERMA_DERECHA + 0.55f * (h0 - yb0), yb0, zAbajo };
            Vector3 c = { X_BERMA_DERECHA + 0.55f * (h1 - yb1), yb1, zArriba };
            Vector3 d = { X_BERMA_DERECHA + 0.55f * (h1 - ya1), ya1, zArriba };
            Color color = TinteCantera(ESTRATOS_CANTERA[banda % 6], 0.93f + 0.12f * Hash01Cantera(k, banda + 520));

            DrawTriangle3D(a, b, c, color);
            DrawTriangle3D(a, c, d, color);
        }
    }
}


//==================================================
// VISUAL: RAMPA, META Y PIE
//==================================================

static void DibujarRampaCantera(float tiempo)
{
    // Juntas: la rampa completa en tono oscuro, debajo de las losas.
    Color juntas = { 150, 112, 74, 255 };
    DibujarCuadroPendienteCantera(-4.75f, 4.75f, Z_BASE_CANTERA, Z_CIMA_CANTERA, 0.0f, juntas);

    // Losas: conservan las franjas alternas que marcan el avance.
    const int FILAS = 20;
    const int COLUMNAS = 6;
    const float JUNTA = 0.035f;

    for (int f = 0; f < FILAS; f++)
    {
        float zAbajo = Z_BASE_CANTERA + (Z_CIMA_CANTERA - Z_BASE_CANTERA) * ((float)f / (float)FILAS);
        float zArriba = Z_BASE_CANTERA + (Z_CIMA_CANTERA - Z_BASE_CANTERA) * ((float)(f + 1) / (float)FILAS);
        Color base = (f / 2) % 2 == 0 ? Color{ 196, 150, 90, 255 } : Color{ 214, 170, 104, 255 };

        for (int c = 0; c < COLUMNAS; c++)
        {
            float x0 = -4.75f + 9.5f * ((float)c / (float)COLUMNAS);
            float x1 = -4.75f + 9.5f * ((float)(c + 1) / (float)COLUMNAS);
            Color color = TinteCantera(base, 0.94f + 0.11f * Hash01Cantera(f, c + 600));
            DibujarCuadroPendienteCantera(x0 + JUNTA, x1 - JUNTA, zAbajo - JUNTA, zArriba + JUNTA, 0.025f, color);
        }
    }

    // Grietas en las losas.
    const float GRIETAS[6][4] =
    {
        { -3.6f, 6.4f, -2.9f, 5.7f }, { -2.9f, 5.7f, -2.6f, 4.9f },
        { 2.2f, 1.8f, 3.0f, 1.2f }, { 1.0f, -4.2f, 1.6f, -4.9f },
        { -1.6f, -1.0f, -0.9f, -1.5f }, { 3.4f, -6.6f, 3.9f, -7.2f }
    };
    for (int g = 0; g < 6; g++)
    {
        DrawLine3D(
            PuntoRampaCantera(GRIETAS[g][0], GRIETAS[g][1], 0.04f),
            PuntoRampaCantera(GRIETAS[g][2], GRIETAS[g][3], 0.04f),
            Color{ 116, 84, 56, 255 }
        );
    }

    // Linea de meta en cuadros en la cima.
    const int columnas = 10;
    for (int fila = 0; fila < 2; fila++)
    {
        for (int columna = 0; columna < columnas; columna++)
        {
            float x0 = -4.75f + 9.5f * (columna / (float)columnas);
            float x1 = -4.75f + 9.5f * ((columna + 1) / (float)columnas);
            bool clara = (fila + columna) % 2 == 0;
            DibujarCuadroPendienteCantera(
                x0,
                x1,
                Z_CIMA_CANTERA + 0.55f * (fila + 1),
                Z_CIMA_CANTERA + 0.55f * fila,
                0.05f,
                clara ? Color{ 245, 240, 228, 255 } : Color{ 54, 50, 50, 255 }
            );
        }
    }

    // Cordones con franjas de peligro amarillo/oscuro (igual que antes).
    for (int lado = 0; lado < 2; lado++)
    {
        float x = lado == 0 ? -5.05f : 5.05f;
        const int tramos = 10;
        for (int k = 0; k < tramos; k++)
        {
            float t0 = k / (float)tramos;
            float t1 = (k + 1) / (float)tramos;
            DrawCylinderEx(
                { x, 0.15f + 5.0f * t0, Z_BASE_CANTERA + (Z_CIMA_CANTERA - Z_BASE_CANTERA) * t0 },
                { x, 0.15f + 5.0f * t1, Z_BASE_CANTERA + (Z_CIMA_CANTERA - Z_BASE_CANTERA) * t1 },
                0.18f,
                0.18f,
                10,
                k % 2 == 0 ? Color{ 238, 192, 52, 255 } : Color{ 58, 54, 52, 255 }
            );
        }
    }

    // Mastiles de meta con banderas y guirnalda de banderines, por encima
    // de la linea de vision al operador.
    float yCima = AlturaPendienteCantera(Z_CIMA_CANTERA);
    const float ALTO_MASTIL = 3.9f;

    for (int lado = 0; lado < 2; lado++)
    {
        float x = lado == 0 ? -4.7f : 4.7f;
        float s = lado == 0 ? 1.0f : -1.0f;
        float flameo = 0.08f * std::sin(tiempo * 4.0f + (float)lado * 1.3f);
        DrawCylinderEx({ x, yCima, Z_CIMA_CANTERA }, { x, yCima + ALTO_MASTIL, Z_CIMA_CANTERA }, 0.06f, 0.05f, 6, Color{ 70, 66, 64, 255 });
        DrawTriangle3D(
            { x, yCima + ALTO_MASTIL, Z_CIMA_CANTERA },
            { x, yCima + ALTO_MASTIL - 0.5f, Z_CIMA_CANTERA },
            { x + s * 0.9f, yCima + ALTO_MASTIL - 0.25f + flameo, Z_CIMA_CANTERA + flameo },
            Color{ 226, 60, 52, 255 }
        );
        DrawTriangle3D(
            { x, yCima + ALTO_MASTIL, Z_CIMA_CANTERA },
            { x + s * 0.9f, yCima + ALTO_MASTIL - 0.25f + flameo, Z_CIMA_CANTERA + flameo },
            { x, yCima + ALTO_MASTIL - 0.5f, Z_CIMA_CANTERA },
            Color{ 196, 46, 42, 255 }
        );
    }

    const Color BANDERINES[3] = { { 255, 214, 92, 255 }, { 236, 92, 80, 255 }, { 255, 246, 226, 255 } };
    const int PUNTOS = 16;
    float yCuerda = yCima + ALTO_MASTIL - 0.05f;
    Vector3 anterior = { -4.7f, yCuerda, Z_CIMA_CANTERA };

    for (int k = 1; k <= PUNTOS; k++)
    {
        float u = (float)k / (float)PUNTOS;
        Vector3 punto = { -4.7f + 9.4f * u, yCuerda - 0.25f * std::sin(3.1416f * u), Z_CIMA_CANTERA };
        DrawLine3D(anterior, punto, Color{ 60, 54, 50, 255 });

        if (k < PUNTOS)
        {
            float vaiven = 0.06f * std::sin(tiempo * 3.0f + (float)k);
            DrawTriangle3D(
                { punto.x - 0.17f, punto.y, punto.z },
                { punto.x, punto.y - 0.3f, punto.z + vaiven },
                { punto.x + 0.17f, punto.y, punto.z },
                BANDERINES[k % 3]
            );
        }

        anterior = punto;
    }
}


// Explanada al pie de la rampa y barrera donde se parten las rocas.
static void DibujarPieRampaCantera()
{
    CajaFacetadaCantera({ -0.3f, -0.11f, 10.8f }, { 11.3f, 0.22f, 3.1f }, Color{ 176, 140, 98, 255 });

    for (int k = 0; k < 5; k++)
    {
        float x = -3.92f + 1.96f * (float)k;
        CajaBordeCantera({ x, 0.36f, 11.85f }, { 1.9f, 0.72f, 0.6f }, Color{ 184, 176, 162, 255 });
        CajaFacetadaCantera({ x, 0.50f, 11.54f }, { 1.6f, 0.16f, 0.02f }, Color{ 236, 120, 52, 255 });
    }
}


//==================================================
// VISUAL: UTILERIA DE OBRA
//==================================================

static void DibujarAndamioCantera(Vector3 base, float alto)
{
    const Color METAL = { 158, 158, 164, 255 };
    const Color MADERA = { 160, 112, 66, 255 };
    const float AX = 0.55f, AZ = 0.5f;

    for (int i = 0; i < 4; i++)
    {
        float x = base.x + (i % 2 == 0 ? -AX : AX);
        float z = base.z + (i < 2 ? -AZ : AZ);
        DrawCylinderEx({ x, base.y, z }, { x, base.y + alto, z }, 0.04f, 0.04f, 5, METAL);
    }

    for (int nivel = 1; nivel <= 2; nivel++)
    {
        float y = base.y + alto * 0.5f * (float)nivel;
        CajaFacetadaCantera({ base.x, y, base.z }, { AX * 2.0f + 0.2f, 0.07f, AZ * 2.0f + 0.1f }, MADERA);
        DrawLine3D({ base.x - AX, y - alto * 0.5f, base.z + AZ }, { base.x + AX, y, base.z + AZ }, METAL);
        DrawLine3D({ base.x + AX, y - alto * 0.5f, base.z - AZ }, { base.x + AX, y, base.z + AZ }, METAL);
    }
}


// Marco de vigas de entibado contra la pared del fondo.
static void DibujarEntibadoCantera(float x)
{
    const Color MADERA = { 124, 86, 54, 255 };
    float y = AlturaPendienteCantera(Z_CIMA_CANTERA);
    float z = Z_PARED_FONDO_CANTERA + 0.16f;

    CajaBordeCantera({ x - 0.75f, y + 0.8f, z }, { 0.22f, 1.6f, 0.22f }, MADERA);
    CajaBordeCantera({ x + 0.75f, y + 0.8f, z }, { 0.22f, 1.6f, 0.22f }, MADERA);
    CajaBordeCantera({ x, y + 1.7f, z }, { 1.9f, 0.22f, 0.26f }, TinteCantera(MADERA, 1.1f));
}


struct FocoObraCantera
{
    Vector3 base;
    float alto;
    Vector3 objetivo;
};


static const FocoObraCantera FOCOS_CANTERA[3] =
{
    { { -7.0f, 6.3f, -10.9f }, 4.2f, { -1.4f, 0.0f, -4.6f } },
    { { 6.5f, 5.0f, -10.4f }, 5.2f, { 1.8f, 0.0f, -0.8f } },
    { { -7.0f, 1.7f, 8.3f }, 5.2f, { -0.6f, 0.0f, 4.4f } }
};


static Vector3 LenteFocoCantera(const FocoObraCantera& foco, int lampara)
{
    Vector3 tope = { foco.base.x, foco.base.y + foco.alto, foco.base.z };
    Vector3 objetivo = PuntoRampaCantera(foco.objetivo.x, foco.objetivo.z, 0.0f);
    Vector3 lateral = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(objetivo, tope), { 0.0f, 1.0f, 0.0f }));
    return Vector3Add(tope, Vector3Scale(lateral, lampara == 0 ? -0.36f : 0.36f));
}


static void DibujarFocoObraCantera(const FocoObraCantera& foco)
{
    const Color METAL = { 120, 122, 130, 255 };
    Vector3 tope = { foco.base.x, foco.base.y + foco.alto, foco.base.z };

    // Mastil reticulado.
    for (int i = 0; i < 4; i++)
    {
        float sx = i % 2 == 0 ? -1.0f : 1.0f;
        float sz = i < 2 ? -1.0f : 1.0f;
        DrawCylinderEx(
            { foco.base.x + sx * 0.32f, foco.base.y, foco.base.z + sz * 0.32f },
            { tope.x + sx * 0.16f, tope.y - 0.2f, tope.z + sz * 0.16f },
            0.045f, 0.035f, 5, METAL
        );
    }

    for (float y = foco.base.y + 0.6f; y < tope.y - 0.4f; y += 0.9f)
    {
        float u = (y - foco.base.y) / foco.alto;
        float m = 0.32f - 0.16f * u;
        DrawLine3D({ foco.base.x - m, y, foco.base.z + m }, { foco.base.x + m, y + 0.45f, foco.base.z + m }, METAL);
        DrawLine3D({ foco.base.x + m, y, foco.base.z - m }, { foco.base.x + m, y + 0.45f, foco.base.z + m }, METAL);
    }

    CajaFacetadaCantera({ tope.x, tope.y - 0.2f, tope.z }, { 0.5f, 0.08f, 0.5f }, METAL);

    // Dos lamparas orientadas hacia la rampa.
    Vector3 objetivo = PuntoRampaCantera(foco.objetivo.x, foco.objetivo.z, 0.0f);

    for (int l = 0; l < 2; l++)
    {
        Vector3 lente = LenteFocoCantera(foco, l);
        Vector3 dir = Vector3Normalize(Vector3Subtract(objetivo, lente));
        Vector3 u = Vector3Normalize(Vector3CrossProduct(dir, { 0.0f, 1.0f, 0.0f }));
        Vector3 v = Vector3CrossProduct(u, dir);
        Vector3 caja = Vector3Subtract(lente, Vector3Scale(dir, 0.16f));

        CajaFacetadaCantera(caja, { 0.40f, 0.34f, 0.40f }, Color{ 58, 58, 64, 255 });

        Vector3 c = Vector3Add(lente, Vector3Scale(dir, 0.05f));
        Vector3 a0 = Vector3Add(c, Vector3Add(Vector3Scale(u, -0.17f), Vector3Scale(v, -0.14f)));
        Vector3 a1 = Vector3Add(c, Vector3Add(Vector3Scale(u, 0.17f), Vector3Scale(v, -0.14f)));
        Vector3 a2 = Vector3Add(c, Vector3Add(Vector3Scale(u, 0.17f), Vector3Scale(v, 0.14f)));
        Vector3 a3 = Vector3Add(c, Vector3Add(Vector3Scale(u, -0.17f), Vector3Scale(v, 0.14f)));
        Color luz = { 255, 244, 214, 255 };
        DrawTriangle3D(a0, a1, a2, luz);
        DrawTriangle3D(a0, a2, a3, luz);
    }
}


// Camion volquete estacionado en la fosa.
static void DibujarCamionCantera(Vector3 posicion)
{
    DibujarSombraRetroRectangular({ posicion.x, posicion.y + 0.2f, posicion.z }, 2.0f, 3.6f, 0.0f, posicion.y + 0.015f, 0.32f);

    for (int i = 0; i < 4; i++)
    {
        float x = posicion.x + (i % 2 == 0 ? -0.88f : 0.88f);
        float z = posicion.z + (i < 2 ? -1.05f : 1.05f);
        float xi = x + (i % 2 == 0 ? -0.15f : 0.15f);
        DrawCylinderEx({ x, posicion.y + 0.42f, z }, { xi, posicion.y + 0.42f, z }, 0.42f, 0.42f, 10, Color{ 40, 38, 38, 255 });
        DrawCylinderEx({ xi, posicion.y + 0.42f, z }, { xi + (i % 2 == 0 ? -0.02f : 0.02f), posicion.y + 0.42f, z }, 0.2f, 0.2f, 8, Color{ 168, 166, 160, 255 });
    }

    CajaBordeCantera({ posicion.x, posicion.y + 0.7f, posicion.z }, { 1.6f, 0.3f, 3.2f }, Color{ 64, 60, 60, 255 });
    CajaBordeCantera({ posicion.x, posicion.y + 1.38f, posicion.z + 1.15f }, { 1.5f, 1.05f, 0.9f }, Color{ 226, 172, 60, 255 });
    CajaFacetadaCantera({ posicion.x, posicion.y + 1.55f, posicion.z + 1.61f }, { 1.2f, 0.42f, 0.04f }, Color{ 70, 96, 112, 255 });
    CajaBordeCantera({ posicion.x, posicion.y + 1.3f, posicion.z - 0.55f }, { 1.75f, 0.9f, 2.1f }, Color{ 206, 150, 52, 255 });
    CajaFacetadaCantera({ posicion.x, posicion.y + 1.78f, posicion.z - 0.55f }, { 1.5f, 0.12f, 1.85f }, Color{ 176, 140, 100, 255 });
    CajaGiradaCantera({ posicion.x - 0.2f, posicion.y + 1.92f, posicion.z - 0.7f }, { 0.5f, 0.24f, 0.45f }, 20.0f, Color{ 190, 150, 104, 255 });
    CajaGiradaCantera({ posicion.x + 0.3f, posicion.y + 1.9f, posicion.z - 0.2f }, { 0.4f, 0.2f, 0.4f }, -15.0f, Color{ 168, 128, 90, 255 });
}


static void DibujarMontonGravaCantera(Vector3 base, float radio, float alto, int semilla)
{
    MontanaCantera(base, radio, alto, semilla, Color{ 186, 146, 100, 255 }, 0.0f);
}


static void DibujarUtileriaCantera(bool detallado)
{
    // Andamios sobre el primer banco de la pared izquierda.
    for (int s = 4; s <= 5; s++)
    {
        float largo = (Z_FRENTE_PARED_CANTERA - Z_PARED_FONDO_CANTERA) / (float)SEGMENTOS_PARED_CANTERA;
        float z = InicioSegmentoParedCantera(s) + largo * 0.5f;
        DibujarAndamioCantera({ -7.05f, AlturaBancoCantera(s, 0), z }, 2.3f);
    }

    DibujarEntibadoCantera(-3.6f);
    DibujarEntibadoCantera(3.6f);

    for (int i = 0; i < 3; i++)
    {
        DibujarFocoObraCantera(FOCOS_CANTERA[i]);
    }

    if (!detallado) return;

    DibujarCamionCantera({ 10.6f, -0.22f, -2.0f });
    DibujarMontonGravaCantera({ 13.6f, -0.22f, 3.6f }, 2.1f, 1.7f, 701);
    DibujarMontonGravaCantera({ 13.0f, -0.22f, -7.2f }, 2.5f, 2.1f, 702);

    // Bloques sueltos (tonos calidos y rectos: no se confunden con las
    // rocas grises facetadas que bajan por la rampa).
    const float BLOQUES[10][2] =
    {
        { 7.8f, 6.8f }, { 9.2f, 9.4f }, { 12.2f, 0.4f }, { 9.0f, -6.4f }, { 15.0f, -2.6f },
        { 8.2f, 2.6f }, { -8.6f, 11.4f }, { -11.4f, 12.8f }, { -6.8f, 13.4f }, { 11.4f, 12.0f }
    };

    for (int i = 0; i < 10; i++)
    {
        float tam = 0.4f + 0.45f * Hash01Cantera(i, 800);
        Color color = TinteCantera(ESTRATOS_CANTERA[i % 6], 0.95f + 0.1f * Hash01Cantera(i, 801));
        Vector3 centro = { BLOQUES[i][0], -0.22f + tam * 0.4f, BLOQUES[i][1] };
        DibujarSombraRetroCircular(centro, tam * 0.75f, tam * 0.65f, -0.2f, 0.25f, 8);
        CajaGiradaCantera(centro, { tam * 1.3f, tam * 0.8f, tam }, 70.0f * Hash01Cantera(i, 802), color);
    }
}


//==================================================
// VISUAL: TOLVA DEL OPERADOR
//==================================================

static void QuadCantera(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
}


static Vector3 MezclaPuntoCantera(Vector3 a, Vector3 b, float t)
{
    return Vector3Lerp(a, b, t);
}


static void DibujarTolvaCantera(const MinijuegoCanteraFuga& m, Color colorSolo)
{
    float x = m.posicionTolvaX;
    float y0 = AlturaPendienteCantera(Z_CIMA_CANTERA);
    float retroceso = m.retrocesoTolva * m.retrocesoTolva;
    float salto = 0.07f * retroceso;

    // Plataforma de hormigon de la cima, rieles y durmientes.
    CajaBordeCantera({ 0.0f, y0 - 0.225f, Z_CIMA_CANTERA - 1.2f }, { 10.4f, 0.45f, 2.4f }, Color{ 112, 104, 96, 255 });

    for (int k = 0; k < 17; k++)
    {
        CajaFacetadaCantera({ -4.8f + 0.6f * (float)k, y0 + 0.02f, -9.9f }, { 0.24f, 0.05f, 1.05f }, Color{ 112, 80, 56, 255 });
    }

    CajaFacetadaCantera({ 0.0f, y0 + 0.08f, -9.55f }, { 10.0f, 0.08f, 0.09f }, Color{ 168, 168, 176, 255 });
    CajaFacetadaCantera({ 0.0f, y0 + 0.08f, -10.25f }, { 10.0f, 0.08f, 0.09f }, Color{ 168, 168, 176, 255 });

    // Ruedas sobre los rieles.
    for (int i = 0; i < 4; i++)
    {
        float wx = x + (i % 2 == 0 ? -0.62f : 0.62f);
        float wz = i < 2 ? -9.55f : -10.25f;
        DrawCylinderEx({ wx, y0 + 0.24f, wz - 0.07f }, { wx, y0 + 0.24f, wz + 0.07f }, 0.16f, 0.16f, 10, Color{ 40, 40, 46, 255 });
    }

    // Chasis.
    CajaBordeCantera({ x, y0 + 0.44f + salto, -9.9f }, { 1.75f, 0.28f, 1.15f }, Color{ 70, 74, 84, 255 });

    // Tolva: tronco de piramide invertido.
    const float ZC = -9.95f;
    float yb = y0 + 0.58f + salto;
    float yt = y0 + 1.45f + salto;
    Vector3 b0 = { x - 0.55f, yb, ZC - 0.42f }, b1 = { x + 0.55f, yb, ZC - 0.42f };
    Vector3 b2 = { x + 0.55f, yb, ZC + 0.42f }, b3 = { x - 0.55f, yb, ZC + 0.42f };
    Vector3 t0 = { x - 0.95f, yt, ZC - 0.68f }, t1 = { x + 0.95f, yt, ZC - 0.68f };
    Vector3 t2 = { x + 0.95f, yt, ZC + 0.68f }, t3 = { x - 0.95f, yt, ZC + 0.68f };
    Color acero = { 96, 102, 114, 255 };

    QuadCantera(b3, b2, t2, t3, TinteCantera(acero, 0.95f));
    QuadCantera(b1, b2, t2, t1, TinteCantera(acero, 0.82f));
    QuadCantera(b0, b3, t3, t0, TinteCantera(acero, 0.6f));
    QuadCantera(b0, b1, t1, t0, TinteCantera(acero, 0.52f));
    QuadCantera(
        { t0.x + 0.06f, yt - 0.12f, t0.z + 0.06f }, { t1.x - 0.06f, yt - 0.12f, t1.z + 0.06f },
        { t2.x - 0.06f, yt - 0.12f, t2.z - 0.06f }, { t3.x + 0.06f, yt - 0.12f, t3.z - 0.06f },
        Color{ 38, 40, 46, 255 }
    );
    DrawLine3D(t0, t1, Fade(BLACK, 0.4f));
    DrawLine3D(t1, t2, Fade(BLACK, 0.4f));
    DrawLine3D(t2, t3, Fade(BLACK, 0.4f));
    DrawLine3D(t3, t0, Fade(BLACK, 0.4f));

    // Carga de piedras asomando por la boca.
    for (int k = 0; k < 5; k++)
    {
        Vector3 p = { x - 0.6f + 0.3f * (float)k, yt - 0.02f + 0.05f * (float)(k % 2), ZC - 0.25f + 0.22f * (float)(k % 3) };
        DibujarRocaFacetadaCantera(p, 0.2f, (float)k, 0.3f * (float)k, 900 + k, COLOR_ROCA_CANTERA);
    }

    // Borde con franjas de peligro.
    for (int k = 0; k < 8; k++)
    {
        float u0 = (float)k / 8.0f, u1 = (float)(k + 1) / 8.0f;
        Vector3 p0 = MezclaPuntoCantera(t3, t2, u0), p1 = MezclaPuntoCantera(t3, t2, u1);
        Color franja = k % 2 == 0 ? Color{ 238, 192, 52, 255 } : Color{ 48, 46, 46, 255 };
        QuadCantera(
            { p0.x, p0.y + 0.002f, p0.z + 0.01f }, { p1.x, p1.y + 0.002f, p1.z + 0.01f },
            { p1.x, p1.y - 0.14f, p1.z - 0.02f }, { p0.x, p0.y - 0.14f, p0.z - 0.02f },
            franja
        );
    }

    // Panel del color del operador en la cara frontal (destella al acertar).
    Color panel = MezclaCantera(colorSolo, WHITE, 0.55f * m.destelloOperador);
    Vector3 pa = MezclaPuntoCantera(MezclaPuntoCantera(b3, t3, 0.18f), MezclaPuntoCantera(b2, t2, 0.18f), 0.16f);
    Vector3 pb = MezclaPuntoCantera(MezclaPuntoCantera(b3, t3, 0.18f), MezclaPuntoCantera(b2, t2, 0.18f), 0.84f);
    Vector3 pc = MezclaPuntoCantera(MezclaPuntoCantera(b3, t3, 0.72f), MezclaPuntoCantera(b2, t2, 0.72f), 0.84f);
    Vector3 pd = MezclaPuntoCantera(MezclaPuntoCantera(b3, t3, 0.72f), MezclaPuntoCantera(b2, t2, 0.72f), 0.16f);
    const Vector3 afuera = { 0.0f, -0.008f, 0.03f };
    QuadCantera(Vector3Add(pa, afuera), Vector3Add(pb, afuera), Vector3Add(pc, afuera), Vector3Add(pd, afuera), panel);

    // Canaleta de salida hacia la rampa.
    Color canaleta = { 128, 132, 142, 255 };
    QuadCantera(
        { x - 0.48f, yb + 0.02f, ZC + 0.4f }, { x + 0.48f, yb + 0.02f, ZC + 0.4f },
        { x + 0.52f, y0 + 0.3f, -9.12f }, { x - 0.52f, y0 + 0.3f, -9.12f },
        canaleta
    );
    QuadCantera(
        { x - 0.48f, yb + 0.02f, ZC + 0.4f }, { x - 0.52f, y0 + 0.3f, -9.12f },
        { x - 0.52f, y0 + 0.5f, -9.12f }, { x - 0.48f, yb + 0.25f, ZC + 0.4f },
        TinteCantera(canaleta, 0.7f)
    );
    QuadCantera(
        { x + 0.48f, yb + 0.02f, ZC + 0.4f }, { x + 0.52f, y0 + 0.3f, -9.12f },
        { x + 0.52f, y0 + 0.5f, -9.12f }, { x + 0.48f, yb + 0.25f, ZC + 0.4f },
        TinteCantera(canaleta, 0.85f)
    );

    // Motor con caño de escape y baliza giratoria.
    CajaBordeCantera({ x + 1.18f, y0 + 0.72f + salto * 0.5f, -10.15f }, { 0.5f, 0.55f, 0.75f }, Color{ 214, 162, 58, 255 });
    DrawCylinderEx({ x + 1.18f, y0 + 1.0f, -10.30f }, { x + 1.18f, y0 + 1.5f, -10.30f }, 0.06f, 0.06f, 6, Color{ 52, 50, 50, 255 });
    DrawCylinderEx({ x - 0.98f, y0 + 0.58f, -10.35f }, { x - 0.98f, y0 + 1.6f, -10.35f }, 0.04f, 0.04f, 5, Color{ 52, 50, 50, 255 });
    DrawCylinderEx({ x - 0.98f, y0 + 1.6f, -10.35f }, { x - 0.98f, y0 + 1.78f, -10.35f }, 0.1f, 0.08f, 8, Color{ 255, 150, 44, 255 });
}


//==================================================
// VISUAL: LUCES (aditivas, al final de la escena)
//==================================================

static void DibujarLucesCantera(const MinijuegoCanteraFuga& m)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Focos de obra: haz, halo en la lente y charco de luz en la rampa.
    for (int i = 0; i < 3; i++)
    {
        const FocoObraCantera& foco = FOCOS_CANTERA[i];
        Vector3 objetivo = PuntoRampaCantera(foco.objetivo.x, foco.objetivo.z, 0.0f);
        ResplandorRampaCantera(foco.objetivo.x, foco.objetivo.z, 3.6f, Color{ 255, 214, 150, 40 });

        for (int l = 0; l < 2; l++)
        {
            Vector3 lente = LenteFocoCantera(foco, l);
            Vector3 dir = Vector3Normalize(Vector3Subtract(objetivo, lente));
            Vector3 u = Vector3Normalize(Vector3CrossProduct(dir, { 0.0f, 1.0f, 0.0f }));
            Vector3 v = Vector3CrossProduct(u, dir);
            ResplandorCantera(Vector3Add(lente, Vector3Scale(dir, 0.08f)), u, v, 0.75f, Color{ 255, 230, 170, 150 });

            const int LADOS = 12;
            rlBegin(RL_TRIANGLES);
            for (int k = 0; k < LADOS; k++)
            {
                float a0 = 6.2832f * (float)k / (float)LADOS;
                float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
                Vector3 p0 = PuntoRampaCantera(foco.objetivo.x + std::cos(a0) * 2.4f, foco.objetivo.z + std::sin(a0) * 2.4f, 0.05f);
                Vector3 p1 = PuntoRampaCantera(foco.objetivo.x + std::cos(a1) * 2.4f, foco.objetivo.z + std::sin(a1) * 2.4f, 0.05f);
                rlColor4ub(255, 222, 160, 16);
                rlVertex3f(lente.x, lente.y, lente.z);
                rlColor4ub(255, 222, 160, 0);
                rlVertex3f(p0.x, p0.y, p0.z);
                rlColor4ub(255, 222, 160, 0);
                rlVertex3f(p1.x, p1.y, p1.z);
            }
            rlEnd();
        }
    }

    // Meta: brillo calido que late suave.
    float pulso = 0.8f + 0.2f * std::sin(t * 3.0f);
    for (int k = 0; k < 3; k++)
    {
        ResplandorRampaCantera(-3.2f + 3.2f * (float)k, Z_CIMA_CANTERA + 0.55f, 2.0f * pulso, Color{ 255, 210, 110, 42 });
    }

    // Baliza giratoria de la tolva (continua: no anticipa lanzamientos).
    float y0 = AlturaPendienteCantera(Z_CIMA_CANTERA);
    Vector3 baliza = { m.posicionTolvaX - 0.98f, y0 + 1.7f, -10.35f };
    float giro = t * 6.0f;
    ResplandorCantera(baliza, { std::cos(giro), 0.0f, std::sin(giro) }, { 0.0f, 1.0f, 0.0f }, 0.9f, Color{ 255, 150, 50, 120 });
    ResplandorCantera(baliza, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.35f, Color{ 255, 170, 70, 110 });

    // Destello del panel del operador cuando una roca acierta.
    if (m.destelloOperador > 0.0f)
    {
        ResplandorCantera(
            { m.posicionTolvaX, y0 + 1.05f, -9.3f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
            1.3f, Color{ 255, 220, 160, (unsigned char)(150.0f * m.destelloOperador) }
        );
    }

    // Golpes a escaladores: fogonazo calido en la rampa.
    for (int i = 0; i < MAX_DESTELLOS_CANTERA_FUGA; i++)
    {
        const DestelloCanteraFuga& d = m.destellos[i];
        if (!d.activo) continue;
        float k = d.tiempo / 0.6f;
        ResplandorRampaCantera(d.posicion.x, d.posicion.z, 1.2f + 1.4f * k, Color{ 255, 140, 70, (unsigned char)(120.0f * (1.0f - k)) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


static void DibujarParticulasCantera(const MinijuegoCanteraFuga& m)
{
    for (int i = 0; i < MAX_PARTICULAS_CANTERA_FUGA; i++)
    {
        const ParticulaCanteraFuga& p = m.particulas[i];
        if (!p.activa) continue;

        float vida = p.vida / p.vidaMaxima;
        float tamano = p.tamano;
        Color color = p.color;

        if (p.tipo == PARTICULA_CANTERA_POLVO)
        {
            tamano *= 1.7f - 0.7f * vida;
            color.a = (unsigned char)((float)p.color.a * vida * 0.85f);
        }
        else
        {
            color.a = (unsigned char)((float)p.color.a * (vida > 0.3f ? 1.0f : vida / 0.3f));
        }

        DrawCubeV(p.posicion, { tamano, tamano, tamano }, color);
    }
}


void MinijuegoCanteraFuga::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    bool detallado = CalidadDecoracion() != CALIDAD_BAJA;

    ClearBackground(HORIZONTE_CANTERA);
    DibujarCieloCantera(tiempoAnimacion);

    // Entrada de camara suave durante la cuenta (respeta reducir movimiento).
    Camera3D vista = camara;
    if (fase == FASE_CANTERA_PREPARACION)
    {
        float k = LimitarCantera(tiempoPreparacion / DURACION_PREPARACION_CANTERA, 0.0f, 1.0f);
        k = k * k * (3.0f - 2.0f * k) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.x -= 1.4f * k;
        vista.position.y += 2.0f * k;
        vista.position.z += 2.4f * k;
    }

    BeginMode3D(vista);
    CulloCantera(false);

    DibujarSueloFosaCantera();
    DibujarFondoCantera(detallado);
    if (detallado) DibujarParchesFosaCantera();

    DibujarParedFondoCantera(detallado);
    DibujarParedIzquierdaCantera(detallado);
    DibujarTaludesCantera();
    DibujarPieRampaCantera();
    DibujarRampaCantera(tiempoAnimacion);
    DibujarUtileriaCantera(detallado);

    Color colorSolo = indiceSolo >= 0
        ? participantes[indiceSolo].color
        : Color{ 220, 80, 60, 255 };
    DibujarTolvaCantera(*this, colorSolo);

    // Rocas peligrosas: gris azulado frio y facetadas, fuera de la gama
    // calida del escenario. Sombra de contacto justo debajo (no anticipa).
    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        const RocaCanteraFuga& roca = rocas[i];
        if (!roca.activa) continue;

        DiscoRampaCantera(roca.posicion.x - 0.12f, roca.posicion.z - 0.1f, roca.radio * 1.02f, 0.045f, Fade(BLACK, 0.32f));
        DibujarRocaFacetadaCantera(roca.posicion, roca.radio, roca.giro, roca.giroLateral, roca.semilla, COLOR_ROCA_CANTERA);

        if (mostrarDebug)
        {
            DrawSphereWires(roca.posicion, roca.radio + 0.42f, 8, 8, RED);
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;

        float pies = jugadores[i].posicion.y - jugadores[i].tamano.y * 0.5f;

        // Anillo del color del escalador sobre la rampa.
        if (i != indiceSolo)
        {
            AnilloRampaCantera(jugadores[i].posicion.x, jugadores[i].posicion.z, 0.48f, 0.6f, 0.05f, participantes[i].color);
        }

        DibujarJugadorCuboPrueba(jugadores[i], participanteVisual);

        // Flecha del color del jugador sobre la cabeza (el modelo es oscuro).
        // MODELO FUTURO: puede pasar a ser un icono de jugador del GLB.
        float rebote = std::sin((float)GetTime() * 5.0f + (float)i) * 0.06f;
        float cabeza = pies + 2.15f + rebote;
        DrawCylinderEx(
            { jugadores[i].posicion.x, cabeza, jugadores[i].posicion.z },
            { jugadores[i].posicion.x, cabeza + 0.34f, jugadores[i].posicion.z },
            0.0f,
            0.22f,
            10,
            participantes[i].color
        );

        // Aturdido: estrellitas girando sobre el escalador aplastado.
        if (i != indiceSolo && estadosJugadores[i].tiempoAturdido > 0.0f)
        {
            for (int k = 0; k < 3; k++)
            {
                float a = tiempoAnimacion * 6.0f + 2.094f * (float)k;
                Vector3 estrella =
                {
                    jugadores[i].posicion.x + std::cos(a) * 0.45f,
                    pies + 0.75f + 0.08f * std::sin(tiempoAnimacion * 9.0f + (float)k),
                    jugadores[i].posicion.z + std::sin(a) * 0.45f
                };
                DrawCubeV(estrella, { 0.14f, 0.14f, 0.14f }, Color{ 255, 222, 96, 255 });
                DrawCubeWiresV(estrella, { 0.15f, 0.15f, 0.15f }, Color{ 140, 92, 30, 255 });
            }
        }

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    // Anillo que se expande donde golpeo una roca.
    for (int i = 0; i < MAX_DESTELLOS_CANTERA_FUGA; i++)
    {
        const DestelloCanteraFuga& d = destellos[i];
        if (!d.activo) continue;
        float k = d.tiempo / 0.6f;
        float radio = 0.45f + 1.5f * k;
        AnilloRampaCantera(d.posicion.x, d.posicion.z, radio, radio + 0.2f * (1.0f - k) + 0.03f, 0.07f, Color{ 255, 140, 70, (unsigned char)(220.0f * (1.0f - k)) });
    }

    DibujarParticulasCantera(*this);
    DibujarLucesCantera(*this);

    CulloCantera(true);
    EndMode3D();

    // HUD comun (Minigames/HudMinijuego): solo lee el estado. El operador
    // va a la izquierda del reloj y los escaladores a la derecha.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CANTERA EN FUGA",
        "1 VS 3: EL OPERADOR SUELTA ROCAS DESDE LA CIMA. LOS ESCALADORES SUBEN Y LAS ESQUIVAN.",
        resultado
    );

    hud.fase = fase == FASE_CANTERA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CANTERA_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_CANTERA;
    hud.segundosAlerta = 5.0f;
    hud.finPorTiempo = tiempoRestante <= 0.0f;

    if (fase == FASE_CANTERA_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO && indiceSolo >= 0)
    {
        hud.tituloFinal = resultado.participantes[indiceSolo].posicionFinal == 1
            ? "¡GANA EL OPERADOR!"
            : "¡GANAN LOS ESCALADORES!";
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const Participante& participante = participantes[i];

        if (i == indiceSolo)
        {
            jugador.equipo = 0;
            jugador.rol = "OPERADOR";
            jugador.valorNumerico = impactosSolo;
            std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d IMPACTOS", impactosSolo);

            if (!participante.esBot)
            {
                // Tolva: solo izquierda/derecha. Roca: golpear (E / SHIFT DER / X).
                const char* mover =
                    participante.control == CONTROL_GAMEPAD ? "STICK"
                    : (participante.control == CONTROL_TECLADO_FLECHAS ? "FLECHAS"
                    : (participante.control == CONTROL_TECLADO_WASD ? "A D" : "A D / FLECHAS"));

                std::snprintf(
                    jugador.controles,
                    sizeof(jugador.controles),
                    "TOLVA: %s\nROCA: %s",
                    mover,
                    TextoBotonGolpeHud(participante)
                );
            }
            continue;
        }

        // Mientras se juega, la altura actual. Al terminar, la etiqueta
        // pasa a la altura maxima alcanzada: el mismo dato del tablon y de
        // la puntuacion del escalador.
        int maximo = (int)std::lround(estadosJugadores[i].progresoMaximo * 100.0f);
        float proporcion = hud.fase == HUD_MINIJUEGO_FIN
            ? estadosJugadores[i].progresoMaximo
            : CalcularProgresoCantera(jugadores[i].posicion.z);

        jugador.equipo = 1;
        jugador.rol = "ESCALADOR";
        jugador.valorNumerico = estadosJugadores[i].impactosRecibidos;
        jugador.progreso = proporcion;
        std::snprintf(
            jugador.valor,
            sizeof(jugador.valor),
            "%d%%",
            hud.fase == HUD_MINIJUEGO_FIN ? maximo : (int)(proporcion * 100.0f)
        );
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "MÁX %d%%", maximo);

        if (estadosJugadores[i].tiempoAturdido > 0.0f)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "ATURDIDO");
        }

        if (!participante.esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "ESCALÁ Y ESQUIVÁ\n%s",
                TextoMoverHud(participante)
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoCanteraFuga::ObtenerResultado() const
{
    return resultado;
}
