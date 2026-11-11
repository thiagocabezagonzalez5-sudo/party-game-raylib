#include "Minigames/MinijuegoAutosGlobo.h"

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
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_GLOBO = 3.0f;
static const float DURACION_PARTIDA_GLOBO = 60.0f;

// Plaza: limites del CENTRO del auto (la barrera visual queda un radio mas alla).
static const float LIMITE_X_GLOBO = 8.3f;
static const float LIMITE_Z_GLOBO = 5.6f;
static const float RADIO_AUTO_GLOBO = 0.85f;
static const float SUELO_GLOBO = 0.0f;

// Conduccion.
static const float ACELERACION_GLOBO = 15.0f;
static const float VELOCIDAD_MAXIMA_GLOBO = 8.0f;
static const float VELOCIDAD_MARCHA_ATRAS_GLOBO = 3.5f;
static const float ACELERACION_TURBO_GLOBO = 38.0f;
static const float VELOCIDAD_TURBO_GLOBO = 13.5f;
static const float DURACION_TURBO_GLOBO = 0.6f;
static const float RECARGA_TURBO_GLOBO = 2.5f;
static const float GIRO_GLOBO = 2.8f;
static const float AGARRE_LATERAL_GLOBO = 3.2f;

// Combate.
static const float DURACION_INMUNIDAD_GLOBO = 1.0f;
static const float VELOCIDAD_MINIMA_EMBESTIDA_GLOBO = 1.0f;
static const float UMBRAL_FRENTE_GLOBO = 0.55f;
static const float UMBRAL_FLANCO_GLOBO = 0.4f;
static const float REBOTE_AUTOS_GLOBO = 0.85f;
static const float REBOTE_BARRERA_GLOBO = 0.75f;

// Placas de carga.
static const float PRIMERA_PLACA_GLOBO = 6.0f;
static const float INTERVALO_PLACA_GLOBO = 12.0f;
static const float AVISO_PLACA_GLOBO = 2.0f;
static const float VIDA_PLACA_GLOBO = 10.0f;
static const float RADIO_PLACA_GLOBO = 1.1f;

static const int CANTIDAD_SITIOS_PLACA_GLOBO = 8;
static const float SITIOS_PLACA_X_GLOBO[CANTIDAD_SITIOS_PLACA_GLOBO] = { -5.5f, 5.5f, -5.5f, 5.5f, 0.0f, 0.0f, -2.5f, 2.5f };
static const float SITIOS_PLACA_Z_GLOBO[CANTIDAD_SITIOS_PLACA_GLOBO] = { -3.2f, -3.2f, 3.2f, 3.2f, -3.8f, 3.8f, 0.0f, 0.0f };

// Puntos de salida (esquinas) por orden de participante.
static const float SPAWN_X_GLOBO[MAX_PARTICIPANTES] = { -6.0f, 6.0f, 6.0f, -6.0f };
static const float SPAWN_Z_GLOBO[MAX_PARTICIPANTES] = { 3.5f, -3.5f, 3.5f, -3.5f };


//==================================================
// UTILIDADES
//==================================================

static float LimitarGlobo(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioGlobo(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static int LimiteGlobo(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static float DistanciaGlobo(float ax, float az, float bx, float bz)
{
    float dx = ax - bx;
    float dz = az - bz;
    return std::sqrt(dx * dx + dz * dz);
}


static float NormalizarAnguloGlobo(float angulo)
{
    while (angulo > PI) angulo -= 2.0f * PI;
    while (angulo < -PI) angulo += 2.0f * PI;
    return angulo;
}


static const char* NombreJugadorGlobo(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static int ContarVivosGlobo(const MinijuegoAutosGlobo& m, int limite)
{
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo && m.autos[i].vivo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void AgregarParticulaGlobo(
    MinijuegoAutosGlobo& m,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_AUTOS_GLOBO; i++)
    {
        if (m.particulas[i].activa)
        {
            continue;
        }

        m.particulas[i].activa = true;
        m.particulas[i].posicion = posicion;
        m.particulas[i].velocidad = velocidad;
        m.particulas[i].vida = vida;
        m.particulas[i].vidaMaxima = vida;
        m.particulas[i].tamano = tamano;
        m.particulas[i].color = color;
        return;
    }
}


static void ExplosionGlobo(
    MinijuegoAutosGlobo& m,
    float x,
    float y,
    float z,
    Color color,
    int cantidad
)
{
    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AleatorioGlobo(0.0f, 2.0f * PI);
        float fuerza = AleatorioGlobo(1.5f, 4.5f);

        AgregarParticulaGlobo(
            m,
            { x, y, z },
            { std::cos(angulo) * fuerza, AleatorioGlobo(0.5f, 3.5f), std::sin(angulo) * fuerza },
            AleatorioGlobo(0.35f, 0.8f),
            AleatorioGlobo(0.07f, 0.15f),
            color
        );
    }
}


//==================================================
// LOGICA: AUTOS Y COMBATE
//==================================================

static void EliminarAutoGlobo(
    MinijuegoAutosGlobo& m,
    int indice,
    const Participante participantes[],
    float transcurrido
)
{
    AutoGlobo& auto_ = m.autos[indice];
    auto_.vivo = false;
    auto_.globos = 0;
    auto_.tiempoEliminacion = transcurrido;

    ExplosionGlobo(m, auto_.x, 0.6f, auto_.z, Color{ 255, 170, 60, 255 }, 26);
    ReproducirSonidoMinijuego(m.audio, SONIDO_ELIMINADO);

    std::snprintf(
        m.textoEvento,
        sizeof(m.textoEvento),
        "%s ELIMINADO",
        NombreJugadorGlobo(participantes[indice], indice)
    );
    m.tiempoEvento = 2.0f;
}


static void ReventarGloboGlobo(
    MinijuegoAutosGlobo& m,
    int victima,
    const Participante participantes[],
    float transcurrido
)
{
    AutoGlobo& auto_ = m.autos[victima];
    auto_.globos--;
    auto_.inmunidad = DURACION_INMUNIDAD_GLOBO;

    Color color = participantes[victima].color;
    ExplosionGlobo(m, auto_.x, 1.4f, auto_.z, color, 14);
    ReproducirSonidoMinijuego(m.audio, SONIDO_EXPLOSION);

    if (auto_.globos <= 0)
    {
        EliminarAutoGlobo(m, victima, participantes, transcurrido);
    }
}


static void ActualizarAutoGlobo(
    MinijuegoAutosGlobo& m,
    int indice,
    const InputMinijuegoParticipante& entrada,
    float deltaTime
)
{
    AutoGlobo& a = m.autos[indice];

    if (a.inmunidad > 0.0f) a.inmunidad -= deltaTime;
    if (a.turboRecarga > 0.0f) a.turboRecarga -= deltaTime;
    if (a.enfriamientoChoque > 0.0f) a.enfriamientoChoque -= deltaTime;
    if (a.enfriamientoBarrera > 0.0f) a.enfriamientoBarrera -= deltaTime;

    if (entrada.golpear && a.turboRecarga <= 0.0f && a.turboActivo <= 0.0f)
    {
        a.turboActivo = DURACION_TURBO_GLOBO;
        a.turboRecarga = RECARGA_TURBO_GLOBO;
        ReproducirSonidoMinijuego(m.audio, SONIDO_DISPARO);
    }

    bool turbo = a.turboActivo > 0.0f;

    if (turbo)
    {
        a.turboActivo -= deltaTime;
    }

    // Rumbo actual segun la velocidad longitudinal.
    float fx = std::cos(a.angulo);
    float fz = std::sin(a.angulo);
    float longitudinal = a.vx * fx + a.vz * fz;

    float direccion = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
    float factorGiro = LimitarGlobo(std::fabs(longitudinal) / 2.5f, 0.35f, 1.0f);
    float sentido = longitudinal < -0.5f ? -1.0f : 1.0f;
    a.angulo += direccion * GIRO_GLOBO * factorGiro * sentido * deltaTime;

    fx = std::cos(a.angulo);
    fz = std::sin(a.angulo);
    float sx = -fz;
    float sz = fx;

    longitudinal = a.vx * fx + a.vz * fz;
    float lateral = a.vx * sx + a.vz * sz;

    float acelerador = (entrada.adelante ? 1.0f : 0.0f) - (entrada.atras ? 0.6f : 0.0f);
    float aceleracion = ACELERACION_GLOBO;
    float maxima = VELOCIDAD_MAXIMA_GLOBO;

    if (turbo)
    {
        acelerador = 1.0f;
        aceleracion = ACELERACION_TURBO_GLOBO;
        maxima = VELOCIDAD_TURBO_GLOBO;
    }

    longitudinal += acelerador * aceleracion * deltaTime;

    if (std::fabs(acelerador) < 0.01f)
    {
        longitudinal *= std::exp(-1.1f * deltaTime);
    }
    else
    {
        longitudinal *= std::exp(-0.25f * deltaTime);
    }

    if (longitudinal > maxima)
    {
        longitudinal -= (longitudinal - maxima) * LimitarGlobo(deltaTime * 3.0f, 0.0f, 1.0f);
    }

    if (longitudinal < -VELOCIDAD_MARCHA_ATRAS_GLOBO)
    {
        longitudinal = -VELOCIDAD_MARCHA_ATRAS_GLOBO;
    }

    // El agarre lateral es bajo: el auto derrapa un poco al girar.
    lateral *= std::exp(-AGARRE_LATERAL_GLOBO * deltaTime);

    a.vx = fx * longitudinal + sx * lateral;
    a.vz = fz * longitudinal + sz * lateral;
    a.x += a.vx * deltaTime;
    a.z += a.vz * deltaTime;

    // Estela del turbo.
    if (turbo)
    {
        AgregarParticulaGlobo(
            m,
            { a.x - fx * 1.0f, 0.4f, a.z - fz * 1.0f },
            { -fx * 2.0f, 0.3f, -fz * 2.0f },
            0.3f,
            0.12f,
            Color{ 120, 230, 255, 255 }
        );
    }

    // Barreras rebotantes.
    float normalX = 0.0f;
    float normalZ = 0.0f;

    if (a.x > LIMITE_X_GLOBO) { a.x = LIMITE_X_GLOBO; normalX = -1.0f; }
    if (a.x < -LIMITE_X_GLOBO) { a.x = -LIMITE_X_GLOBO; normalX = 1.0f; }
    if (a.z > LIMITE_Z_GLOBO) { a.z = LIMITE_Z_GLOBO; normalZ = -1.0f; }
    if (a.z < -LIMITE_Z_GLOBO) { a.z = -LIMITE_Z_GLOBO; normalZ = 1.0f; }

    if (normalX != 0.0f || normalZ != 0.0f)
    {
        float entrante = -(a.vx * normalX + a.vz * normalZ);

        if (entrante > 0.0f)
        {
            float impulso = entrante * (1.0f + REBOTE_BARRERA_GLOBO);

            if (entrante < 2.0f)
            {
                impulso = entrante + 2.0f;
            }

            a.vx += normalX * impulso;
            a.vz += normalZ * impulso;

            if (entrante > 4.0f && a.enfriamientoBarrera <= 0.0f)
            {
                a.enfriamientoBarrera = 0.35f;
                ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
                ExplosionGlobo(m, a.x, 0.5f, a.z, Color{ 120, 230, 255, 255 }, 5);
            }
        }
    }
}


static void ResolverChoquesGlobo(
    MinijuegoAutosGlobo& m,
    const Participante participantes[],
    int limite,
    float transcurrido
)
{
    for (int i = 0; i < limite; i++)
    {
        for (int j = i + 1; j < limite; j++)
        {
            AutoGlobo& a = m.autos[i];
            AutoGlobo& b = m.autos[j];

            if (!a.vivo || !b.vivo)
            {
                continue;
            }

            float dx = b.x - a.x;
            float dz = b.z - a.z;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float minima = RADIO_AUTO_GLOBO * 2.0f;

            if (distancia >= minima)
            {
                continue;
            }

            float nx = 1.0f;
            float nz = 0.0f;

            if (distancia > 0.001f)
            {
                nx = dx / distancia;
                nz = dz / distancia;
            }

            // Orientacion de cada auto respecto al choque.
            float frenteA = std::cos(a.angulo) * nx + std::sin(a.angulo) * nz;
            float frenteB = -(std::cos(b.angulo) * nx + std::sin(b.angulo) * nz);
            float cierre = (a.vx - b.vx) * nx + (a.vz - b.vz) * nz;

            // Separacion de posiciones.
            float penetracion = minima - distancia + 0.01f;
            a.x -= nx * penetracion * 0.5f;
            a.z -= nz * penetracion * 0.5f;
            b.x += nx * penetracion * 0.5f;
            b.z += nz * penetracion * 0.5f;

            if (cierre <= 0.0f)
            {
                continue;
            }

            float impulso = (1.0f + REBOTE_AUTOS_GLOBO) * cierre * 0.5f;
            a.vx -= nx * impulso;
            a.vz -= nz * impulso;
            b.vx += nx * impulso;
            b.vz += nz * impulso;

            bool aEmbiste = frenteA > UMBRAL_FRENTE_GLOBO && frenteB < UMBRAL_FLANCO_GLOBO && cierre > VELOCIDAD_MINIMA_EMBESTIDA_GLOBO;
            bool bEmbiste = frenteB > UMBRAL_FRENTE_GLOBO && frenteA < UMBRAL_FLANCO_GLOBO && cierre > VELOCIDAD_MINIMA_EMBESTIDA_GLOBO;

            if (aEmbiste || bEmbiste)
            {
                int atacante = aEmbiste ? i : j;
                int victima = aEmbiste ? j : i;
                float signo = aEmbiste ? 1.0f : -1.0f;

                // Empuje extra a la victima.
                m.autos[victima].vx += nx * signo * 3.0f;
                m.autos[victima].vz += nz * signo * 3.0f;
                m.autos[atacante].vx -= nx * signo * 1.0f;
                m.autos[atacante].vz -= nz * signo * 1.0f;

                if (m.autos[victima].inmunidad <= 0.0f)
                {
                    ReventarGloboGlobo(m, victima, participantes, transcurrido);
                    ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
                }
            }
            else
            {
                if (frenteA > UMBRAL_FRENTE_GLOBO && frenteB > UMBRAL_FRENTE_GLOBO)
                {
                    // Choque frontal: se repelen con fuerza.
                    a.vx -= nx * 2.0f;
                    a.vz -= nz * 2.0f;
                    b.vx += nx * 2.0f;
                    b.vz += nz * 2.0f;
                }

                if (cierre > 2.0f && a.enfriamientoChoque <= 0.0f && b.enfriamientoChoque <= 0.0f)
                {
                    a.enfriamientoChoque = 0.3f;
                    b.enfriamientoChoque = 0.3f;
                    ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
                    ExplosionGlobo(m, (a.x + b.x) * 0.5f, 0.5f, (a.z + b.z) * 0.5f, Color{ 255, 240, 160, 255 }, 6);
                }
            }
        }
    }
}


//==================================================
// LOGICA: PLACAS DE CARGA
//==================================================

static void CrearPlacaGlobo(MinijuegoAutosGlobo& m)
{
    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        if (m.placas[p].activa)
        {
            continue;
        }

        // Sitio distinto de las placas actuales.
        for (int intento = 0; intento < 8; intento++)
        {
            int sitio = GetRandomValue(0, CANTIDAD_SITIOS_PLACA_GLOBO - 1);
            bool repetido = false;

            for (int q = 0; q < MAX_PLACAS_AUTOS_GLOBO; q++)
            {
                if (
                    m.placas[q].activa &&
                    DistanciaGlobo(m.placas[q].x, m.placas[q].z, SITIOS_PLACA_X_GLOBO[sitio], SITIOS_PLACA_Z_GLOBO[sitio]) < 1.0f
                )
                {
                    repetido = true;
                }
            }

            if (!repetido || intento == 7)
            {
                m.placas[p].activa = true;
                m.placas[p].enAviso = true;
                m.placas[p].x = SITIOS_PLACA_X_GLOBO[sitio];
                m.placas[p].z = SITIOS_PLACA_Z_GLOBO[sitio];
                m.placas[p].tiempo = AVISO_PLACA_GLOBO;
                return;
            }
        }

        return;
    }
}


static void ActualizarPlacasGlobo(MinijuegoAutosGlobo& m, int limite, float deltaTime)
{
    m.proximaPlaca -= deltaTime;

    if (m.proximaPlaca <= 0.0f)
    {
        CrearPlacaGlobo(m);
        m.proximaPlaca = INTERVALO_PLACA_GLOBO;
    }

    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        PlacaAutosGlobo& placa = m.placas[p];

        if (!placa.activa)
        {
            continue;
        }

        placa.tiempo -= deltaTime;

        if (placa.enAviso)
        {
            if (placa.tiempo <= 0.0f)
            {
                placa.enAviso = false;
                placa.tiempo = VIDA_PLACA_GLOBO;
            }

            continue;
        }

        if (placa.tiempo <= 0.0f)
        {
            placa.activa = false;
            continue;
        }

        for (int i = 0; i < limite; i++)
        {
            AutoGlobo& a = m.autos[i];

            if (
                !a.vivo ||
                a.globos >= GLOBOS_INICIALES_AUTOS_GLOBO ||
                DistanciaGlobo(a.x, a.z, placa.x, placa.z) > RADIO_PLACA_GLOBO
            )
            {
                continue;
            }

            a.globos++;
            placa.activa = false;
            ExplosionGlobo(m, placa.x, 0.8f, placa.z, Color{ 120, 255, 170, 255 }, 14);
            ReproducirSonidoMinijuego(m.audio, SONIDO_RECOGER_OBJETO);
            break;
        }
    }
}


//==================================================
// LOGICA: FIN DE PARTIDA
//==================================================

static int PuntajeFinalGlobo(const MinijuegoAutosGlobo& m, int indice)
{
    const AutoGlobo& a = m.autos[indice];

    if (a.vivo)
    {
        return 10000 + a.globos;
    }

    return (int)(a.tiempoEliminacion * 100.0f);
}


static void FinalizarPartidaGlobo(MinijuegoAutosGlobo& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        ResultadoParticipante& r = m.resultado.participantes[i];

        if (!r.participo)
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < limite; j++)
        {
            if (j != i && m.resultado.participantes[j].participo && PuntajeFinalGlobo(m, j) > PuntajeFinalGlobo(m, i))
            {
                posicion++;
            }
        }

        r.posicionFinal = posicion;
        r.puntuacionMinijuego = m.autos[i].vivo ? m.autos[i].globos : 0;
    }

    int ganadores = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo && m.resultado.participantes[i].posicionFinal == 1)
        {
            ganadores++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = ganadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    m.fase = FASE_AUTOS_GLOBO_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// BOTS
//==================================================

static InputMinijuegoParticipante CrearEntradaBotGlobo(
    MinijuegoAutosGlobo& m,
    int indice,
    int limite,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    const AutoGlobo& a = m.autos[indice];
    EstadoBotAutosGlobo& bot = m.bots[indice];

    bot.reevaluar -= deltaTime;

    if (bot.reevaluar <= 0.0f)
    {
        // Rival mas cercano (con algo de ruido para que no sea perfecto).
        int mejor = -1;
        float mejorDistancia = 1000000.0f;

        for (int j = 0; j < limite; j++)
        {
            if (j == indice || !m.autos[j].vivo)
            {
                continue;
            }

            float distancia = DistanciaGlobo(a.x, a.z, m.autos[j].x, m.autos[j].z) + AleatorioGlobo(0.0f, 1.5f);

            if (distancia < mejorDistancia)
            {
                mejorDistancia = distancia;
                mejor = j;
            }
        }

        bot.objetivo = mejor;
        bot.errorAngulo = AleatorioGlobo(-0.18f, 0.18f);
        bot.querTurbo = GetRandomValue(0, 99) < 55;
        bot.ladoRodeo = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
        bot.reevaluar = AleatorioGlobo(0.3f, 0.5f);
    }

    float fx = std::cos(a.angulo);
    float fz = std::sin(a.angulo);

    float apuntaX = 0.0f;
    float apuntaZ = 0.0f;
    bool hayRival = bot.objetivo >= 0 && m.autos[bot.objetivo].vivo;
    bool atacando = false;
    bool huyendo = false;

    // Placa de carga activa mas cercana.
    int placaCercana = -1;
    float distanciaPlaca = 1000000.0f;

    if (a.globos < GLOBOS_INICIALES_AUTOS_GLOBO)
    {
        for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
        {
            if (!m.placas[p].activa || m.placas[p].enAviso)
            {
                continue;
            }

            float distancia = DistanciaGlobo(a.x, a.z, m.placas[p].x, m.placas[p].z);

            if (distancia < distanciaPlaca)
            {
                distanciaPlaca = distancia;
                placaCercana = p;
            }
        }
    }

    float distanciaRival = hayRival
        ? DistanciaGlobo(a.x, a.z, m.autos[bot.objetivo].x, m.autos[bot.objetivo].z)
        : 1000000.0f;

    // Con un globo huye como maximo 4 s y solo si el rival esta mas sano.
    bool rivalFragil = hayRival && m.autos[bot.objetivo].globos <= 1;
    bool puedeHuir = a.globos == 1 && hayRival && distanciaRival < 5.5f && !rivalFragil;

    if (puedeHuir)
    {
        bot.tiempoHuida += deltaTime;
    }
    else if (a.globos != 1 || distanciaRival > 8.0f)
    {
        bot.tiempoHuida = 0.0f;
    }

    if (puedeHuir && bot.tiempoHuida < 4.0f)
    {
        huyendo = true;

        if (placaCercana >= 0 && distanciaPlaca < 10.0f)
        {
            apuntaX = m.placas[placaCercana].x;
            apuntaZ = m.placas[placaCercana].z;
        }
        else
        {
            const AutoGlobo& rival = m.autos[bot.objetivo];
            apuntaX = a.x + (a.x - rival.x) - a.x * 0.2f;
            apuntaZ = a.z + (a.z - rival.z) - a.z * 0.2f;
        }
    }
    else if (placaCercana >= 0 && distanciaPlaca < 6.5f && distanciaRival > 2.5f)
    {
        apuntaX = m.placas[placaCercana].x;
        apuntaZ = m.placas[placaCercana].z;
    }
    else if (hayRival)
    {
        const AutoGlobo& rival = m.autos[bot.objetivo];
        float rx = std::cos(rival.angulo);
        float rz = std::sin(rival.angulo);
        float lx = -rz;
        float lz = rx;
        float lado = ((a.x - rival.x) * lx + (a.z - rival.z) * lz) >= 0.0f ? 1.0f : -1.0f;

        // Apunta al flanco/trasera del rival; si el rival lo mira de frente, lo rodea
        // por el lado elegido para no quedar en un choque frontal eterno.
        float haciaMiX = a.x - rival.x;
        float haciaMiZ = a.z - rival.z;
        float mirandome = distanciaRival > 0.01f ? (rx * haciaMiX + rz * haciaMiZ) / distanciaRival : 0.0f;
        float separacion = distanciaRival < 3.0f ? 0.3f : 1.2f;
        float detras = distanciaRival < 3.0f ? 0.9f : 1.4f;

        if (mirandome > 0.35f && distanciaRival < 7.0f)
        {
            lado = bot.ladoRodeo;
            separacion = 2.4f;
            detras = 1.0f;
        }

        apuntaX = rival.x + lx * lado * separacion - rx * detras + rival.vx * 0.3f;
        apuntaZ = rival.z + lz * lado * separacion - rz * detras + rival.vz * 0.3f;
        atacando = true;
    }
    else
    {
        apuntaX = 0.0f;
        apuntaZ = 0.0f;
    }

    // Alejarse de las barreras si se esta yendo hacia ellas.
    float margen = 1.3f;
    bool haciaBarrera =
        (a.x > LIMITE_X_GLOBO - margen && fx > 0.0f) ||
        (a.x < -LIMITE_X_GLOBO + margen && fx < 0.0f) ||
        (a.z > LIMITE_Z_GLOBO - margen && fz > 0.0f) ||
        (a.z < -LIMITE_Z_GLOBO + margen && fz < 0.0f);

    if (haciaBarrera && !atacando)
    {
        apuntaX = 0.0f;
        apuntaZ = 0.0f;
    }

    float deseado = std::atan2(apuntaZ - a.z, apuntaX - a.x);
    float diferencia = NormalizarAnguloGlobo(deseado - a.angulo + bot.errorAngulo);

    entrada.derecha = diferencia > 0.07f;
    entrada.izquierda = diferencia < -0.07f;

    float velocidad = std::sqrt(a.vx * a.vx + a.vz * a.vz);

    if (std::fabs(diferencia) > 2.3f && velocidad > 3.0f)
    {
        entrada.atras = true;
    }
    else
    {
        entrada.adelante = true;
    }

    // Turbo en linea recta contra el rival, o para escapar.
    if (a.turboRecarga <= 0.0f && std::fabs(diferencia) < 0.14f)
    {
        if (atacando && bot.querTurbo && distanciaRival > 3.0f && distanciaRival < 7.5f)
        {
            entrada.golpear = true;
            bot.querTurbo = false;
        }
        else if (huyendo && distanciaRival < 4.0f)
        {
            entrada.golpear = true;
        }
    }

    return entrada;
}


//==================================================
// VISUAL: ESTADO (destellos, caidas y fin)
//==================================================
//
// Compara el estado de la logica con el del frame anterior y deja timers
// para el dibujo. Solo escribe en m.visual (y el temblor de camara general).

static Vector3 PosicionGloboVisual(const AutoGlobo& a, int k, float tiempo, int indice);
static void LiberarMallasGlobo();


static void ActualizarVisualGlobo(MinijuegoAutosGlobo& m, float dt)
{
    VisualAutosGlobo& v = m.visual;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const AutoGlobo& a = m.autos[i];

        if (v.reventon[i] > 0.0f) v.reventon[i] -= dt * 2.2f;
        if (v.recarga[i] > 0.0f) v.recarga[i] -= dt * 1.4f;
        if (v.turboDestello[i] > 0.0f) v.turboDestello[i] -= dt * 3.0f;
        if (v.caida[i] >= 0.0f) v.caida[i] += dt;

        if (!m.resultado.participantes[i].participo)
        {
            v.globosPrevios[i] = a.globos;
            v.vivoPrevio[i] = a.vivo;
            continue;
        }

        if (a.globos < v.globosPrevios[i])
        {
            // El globo que se va es el ultimo de la fila.
            Vector3 p = PosicionGloboVisual(a, a.globos, m.tiempoAnimacion, i);

            if (!a.vivo)
            {
                p = PosicionGloboVisual(a, 0, m.tiempoAnimacion, i);
            }

            v.reventon[i] = 1.0f;
            v.reventonX[i] = p.x;
            v.reventonY[i] = p.y;
            v.reventonZ[i] = p.z;
        }
        else if (a.globos > v.globosPrevios[i] && m.fase == FASE_AUTOS_GLOBO_JUGANDO)
        {
            v.recarga[i] = 1.0f;
        }

        if (v.vivoPrevio[i] && !a.vivo)
        {
            float largo = std::sqrt(a.x * a.x / (9.15f * 9.15f) + a.z * a.z / (6.45f * 6.45f));
            float dx = 0.0f;
            float dz = -1.0f;

            if (largo > 0.05f)
            {
                dx = a.x / 9.15f / largo;
                dz = a.z / 6.45f / largo;
            }

            v.caida[i] = 0.0f;
            v.caidaX[i] = a.x;
            v.caidaZ[i] = a.z;
            v.caidaDirX[i] = dx;
            v.caidaDirZ[i] = dz;
            v.caidaAngulo[i] = a.angulo;
            ActivarTemblorCamaraGeneral(0.22f, 0.32f);
        }

        if (a.turboActivo > v.turboPrevio[i] + 0.2f)
        {
            v.turboDestello[i] = 1.0f;
        }

        v.globosPrevios[i] = a.globos;
        v.vivoPrevio[i] = a.vivo;
        v.turboPrevio[i] = a.turboActivo;
    }

    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        const PlacaAutosGlobo& placa = m.placas[p];

        if (v.placaToma[p] > 0.0f) v.placaToma[p] -= dt * 1.6f;

        if (placa.activa && (!v.placaPrevia[p] || placa.enAviso != v.avisoPrevio[p]))
        {
            v.placaEdad[p] = 0.0f;
        }
        else
        {
            v.placaEdad[p] += dt;
        }

        // Se apago antes de vencer: alguien la tomo.
        if (v.placaPrevia[p] && !placa.activa && !v.avisoPrevio[p] && placa.tiempo > 0.0f)
        {
            v.placaToma[p] = 1.0f;
            v.placaTomaX[p] = placa.x;
            v.placaTomaZ[p] = placa.z;
        }

        v.placaPrevia[p] = placa.activa;
        v.avisoPrevio[p] = placa.enAviso;
    }

    if (m.fase == FASE_AUTOS_GLOBO_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        v.tiempoFin = v.tiempoFin < 0.0f ? 0.0f : v.tiempoFin + dt;
    }
}


//==================================================
// CICLO DE VIDA
//==================================================

void MinijuegoAutosGlobo::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        autos[i] = {};
        bots[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_AUTOS_GLOBO; i++)
    {
        particulas[i] = {};
    }

    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        placas[p] = {};
    }

    camara.position = { 0.0f, 17.5f, 9.5f };
    camara.target = { 0.0f, 0.0f, 0.4f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_AUTOS_GLOBO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_GLOBO;
    tiempoRestante = DURACION_PARTIDA_GLOBO;
    tiempoAnimacion = 0.0f;
    proximaPlaca = PRIMERA_PLACA_GLOBO;
    textoEvento[0] = '\0';
    tiempoEvento = 0.0f;

    visual = {};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        visual.caida[i] = -1.0f;
    }
}


void MinijuegoAutosGlobo::Reiniciar(
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

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_AUTOS_GLOBO_TERMINADO;
        return;
    }

    int limite = LimiteGlobo(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        AutoGlobo& a = autos[i];
        a.vivo = true;
        a.x = SPAWN_X_GLOBO[k % MAX_PARTICIPANTES];
        a.z = SPAWN_Z_GLOBO[k % MAX_PARTICIPANTES];
        a.angulo = std::atan2(-a.z, -a.x);
        a.globos = GLOBOS_INICIALES_AUTOS_GLOBO;

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], { a.x, 1.35f, a.z });
    }

    // Visual: el escenario horneado se rehace (puede cambiar la calidad).
    LiberarMallasGlobo();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        visual.globosPrevios[i] = autos[i].globos;
        visual.vivoPrevio[i] = autos[i].vivo;
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoAutosGlobo::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    ActualizarVisualGlobo(*this, deltaTime);

    tiempoAnimacion += deltaTime;
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_AUTOS_GLOBO, deltaTime);

    if (tiempoEvento > 0.0f)
    {
        tiempoEvento -= deltaTime;
    }

    if (
        fase == FASE_AUTOS_GLOBO_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    int limite = LimiteGlobo(cantidadMaxima);

    // Los jugadores se dibujan sobre el auto con el modelo compartido.
    auto sincronizarJugadores = [&]()
    {
        for (int i = 0; i < limite; i++)
        {
            if (!participantes[i].activo)
            {
                continue;
            }

            JugadorPrueba& jugador = jugadores[i];
            const AutoGlobo& a = autos[i];

            jugador.posicion = a.vivo ? Vector3{ a.x, 1.35f, a.z } : Vector3{ 0.0f, -20.0f, 0.0f };
            jugador.velocidad = {};
            jugador.empuje = {};
            jugador.enSuelo = true;
            jugador.cayendo = false;
            jugador.direccionMirada = { std::cos(a.angulo), 0.0f, std::sin(a.angulo) };
        }
    };

    if (fase == FASE_AUTOS_GLOBO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_AUTOS_GLOBO_JUGANDO;
        }

        sincronizarJugadores();
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    float transcurrido = DURACION_PARTIDA_GLOBO - tiempoRestante;

    // 1. Conduccion (humanos y bots; un humano desconectado conduce como bot).
    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo || !autos[i].vivo)
        {
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotGlobo(*this, i, limite, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        ActualizarAutoGlobo(*this, i, entrada, deltaTime);
    }

    // 2. Choques entre autos (globos).
    ResolverChoquesGlobo(*this, participantes, limite, transcurrido);

    // 3. Placas de carga.
    ActualizarPlacasGlobo(*this, limite, deltaTime);

    sincronizarJugadores();

    // 4. Condiciones de fin.
    if (ContarVivosGlobo(*this, limite) <= 1)
    {
        FinalizarPartidaGlobo(*this, limite);
        return;
    }

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarPartidaGlobo(*this, limite);
    }
}


//==================================================
// VISUAL: PLAZA FLOTANTE ENTRE RASCACIELOS (DE DIA)
//==================================================
//
// Mediodia claro en una ciudad jardin. La arena es una plaza-plataforma
// suspendida entre torres de cristal: losas de composite con juntas, los
// ocho zocalos donde aparecen las placas de carga y una barrera de energia
// sobre un bordillo que coincide con el limite real (centro del auto + su
// radio). Alrededor, un paseo con jardineras, bancos, faroles, kioscos de
// carga, pilones holograficos y banderas, y una baranda de vidrio al borde
// de la caida. Afuera: torres con jardines verticales y terrazas verdes,
// puentes, autopistas aereas con trafico y drones; al fondo la ciudad en
// dos planos de bruma y un mar de nubes abajo.
//
// Lo estatico se hornea en tres mallas (fondo, suelo sin profundidad y
// escena); por frame solo va lo que se mueve.
//
// MODELO FUTURO: auto flotante, globo de energia, placa de carga, torre de
// cristal con jardin vertical, auto volador de trafico, dron: GLB.

static const Color CIELO_ALTO_AG = { 58, 132, 222, 255 };
static const Color CIELO_MEDIO_AG = { 112, 178, 240, 255 };
static const Color CIELO_BAJO_AG = { 176, 214, 246, 255 };
static const Color HORIZONTE_AG = { 222, 236, 248, 255 };
static const Color NIEBLA_AG = { 196, 222, 244, 255 };
static const Color SOL_AG = { 255, 248, 226, 255 };
static const Color TINTE_SOL_AG = { 255, 246, 220, 255 };

static const Color LOSA_AG = { 150, 160, 176, 255 };
static const Color LOSA_CLARA_AG = { 184, 192, 204, 255 };
static const Color JUNTA_AG = { 100, 110, 128, 255 };
static const Color PASEO_AG = { 214, 208, 194, 255 };
static const Color BLANCO_AG = { 238, 242, 246, 255 };
static const Color METAL_AG = { 128, 140, 158, 255 };
static const Color METAL_OSCURO_AG = { 62, 72, 90, 255 };
static const Color CRISTAL_AG = { 104, 160, 206, 255 };
static const Color VERDE_AG = { 84, 158, 76, 255 };
static const Color VERDE_OSCURO_AG = { 44, 108, 60, 255 };
static const Color MADERA_AG = { 176, 128, 86, 255 };
static const Color ENERGIA_AG = { 110, 228, 255, 255 };    // barrera y levitacion
static const Color CARGA_AG = { 80, 246, 200, 255 };       // acento: placas de carga
static const Color AVISO_AG = { 255, 178, 64, 255 };
static const Color TURBO_AG = { 255, 158, 56, 255 };

// Camara de la vista.
static const Vector3 OJO_AG = { 0.0f, 12.6f, 14.8f };
static const Vector3 MIRA_AG = { 0.0f, 0.0f, -1.1f };
static const float FOV_AG = 48.0f;

// Borde interior de la barrera = limite real del centro + radio del auto.
static const float BX_AG = LIMITE_X_GLOBO + RADIO_AUTO_GLOBO;
static const float BZ_AG = LIMITE_Z_GLOBO + RADIO_AUTO_GLOBO;
static const float BORDILLO_AG = 0.4f;
// Paseo perimetral: hasta aca llega la plataforma; despues, la caida.
static const float PX_AG = 13.4f;
static const float PZ_AG = 10.0f;


static float Ruido01AG(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}


struct GrabacionAG
{
    bool activa = false;
    std::vector<float> posiciones;
    std::vector<unsigned char> colores;
};

static GrabacionAG g_grabAG;
static Vector3 g_ojoAG = OJO_AG;           // bruma: siempre desde la camara final
static Vector3 g_derAG = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaAG = { 0.0f, 1.0f, 0.0f };
static float g_emisAG = 0.0f;              // 1 = superficie luminosa: casi sin bruma


static float Lim01AG(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalAG(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoAG(Color c, float f)
{
    return { CanalAG((float)c.r * f), CanalAG((float)c.g * f), CanalAG((float)c.b * f), c.a };
}


static Color MezclaAG(Color a, Color b, float t)
{
    t = Lim01AG(t);
    return {
        CanalAG((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalAG((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalAG((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalAG((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaAG(Color c, float alfa)
{
    c.a = CanalAG((float)c.a * Lim01AG(alfa));
    return c;
}


static Color VariarAG(Color c, int indice, int semilla, float amplitud)
{
    return TonoAG(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01AG(indice, semilla));
}


// Niebla de color: lo lejano y lo que queda muy abajo se funde con la
// bruma celeste del mediodia.
static Color NieblaAG(Color c, Vector3 p)
{
    float dx = p.x - g_ojoAG.x, dy = p.y - g_ojoAG.y, dz = p.z - g_ojoAG.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float lejos = Lim01AG((d - 24.0f) / 72.0f) * 0.78f;
    float abajo = Lim01AG((-p.y - 2.0f) / 36.0f) * 0.88f;
    float n = 1.0f - (1.0f - lejos) * (1.0f - abajo);
    n *= 1.0f - 0.65f * g_emisAG;
    Color r = MezclaAG(c, NIEBLA_AG, n);
    r.a = c.a;
    return r;
}


static void VAG(Vector3 p, Color c)
{
    if (g_grabAG.activa)
    {
        g_grabAG.posiciones.push_back(p.x);
        g_grabAG.posiciones.push_back(p.y);
        g_grabAG.posiciones.push_back(p.z);
        g_grabAG.colores.push_back(c.r);
        g_grabAG.colores.push_back(c.g);
        g_grabAG.colores.push_back(c.b);
        g_grabAG.colores.push_back(c.a);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteAG()
{
    if (!g_grabAG.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteAG()
{
    if (!g_grabAG.activa) rlEnd();
}


static void TriAG(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VAG(a, NieblaAG(color, a));
    VAG(b, NieblaAG(color, b));
    VAG(c, NieblaAG(color, c));
}


static void QuadAG(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriAG(a, b, c, color);
    TriAG(a, c, d, color);
}


// Quad con un color por vertice (con bruma).
static void Quad4AG(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VAG(a, NieblaAG(ca, a)); VAG(b, NieblaAG(cb, b)); VAG(c, NieblaAG(cc, c));
    VAG(a, NieblaAG(ca, a)); VAG(c, NieblaAG(cc, c)); VAG(d, NieblaAG(cd, d));
}


// Quad sin bruma con un color por vertice (luces aditivas y sombras).
static void QuadColAG(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VAG(a, ca); VAG(b, cb); VAG(c, cc);
    VAG(a, ca); VAG(c, cc); VAG(d, cd);
}


// Sombreado por caras de la guia (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62) interpolado para normales cualesquiera.
static float LuzAG(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f) return 1.0f;

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.5f) +
        (n.z > 0.0f ? az * 0.78f : az * 0.6f) +
        (n.x < 0.0f ? ax * 0.70f : ax * 0.62f);

    return f / suma;
}


static Color CaraAG(Color c, Vector3 n)
{
    Color r = TonoAG(c, LuzAG(n));

    if (n.y > 0.3f) r = MezclaAG(r, TINTE_SOL_AG, 0.1f * n.y);

    return r;
}


// Caja con medio-ejes arbitrarios: caras sombreadas por normal.
static void CajaEjesAG(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
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

    QuadAG(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraAG(color, ny));
    QuadAG(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraAG(color, nz));
    QuadAG(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraAG(color, Vector3Negate(nz)));
    QuadAG(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraAG(color, Vector3Negate(nx)));
    QuadAG(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraAG(color, nx));

    if (fondo)
    {
        QuadAG(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraAG(color, Vector3Negate(ny)));
    }
}


static void CajaAG(Vector3 c, Vector3 t, Color color)
{
    CajaEjesAG(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, c.y - t.y * 0.5f > 0.05f);
}


// Caja girada en Y (grados).
static void CajaGiradaAG(Vector3 c, Vector3 t, float giroY, Color color)
{
    float a = giroY * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };
    CajaEjesAG(c, Vector3Scale(ux, t.x * 0.5f), { 0.0f, t.y * 0.5f, 0.0f }, Vector3Scale(uz, t.z * 0.5f), color, true);
}


// Viga de seccion rectangular entre dos puntos (alto x ancho).
static void VigaAG(Vector3 a, Vector3 b, float ancho, float alto, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesAG(centro, Vector3Scale(v, ancho * 0.5f), Vector3Scale(w, alto * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas.
static void CilAG(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

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

        QuadAG(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraAG(color, nm)
        );

        if (tapas)
        {
            TriAG(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraAG(color, eje));
            TriAG(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraAG(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide (o media cupula con desde = 0) con caras sombreadas.
static void BolaAG(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float desde = -0.5f * PI)
{
    for (int i = 0; i < anillos; i++)
    {
        float la0 = desde + (0.5f * PI - desde) * (float)i / (float)anillos;
        float la1 = desde + (0.5f * PI - desde) * (float)(i + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float lo0 = 2.0f * PI * (float)k / (float)lados;
            float lo1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float lam = 0.5f * (la0 + la1), lom = 0.5f * (lo0 + lo1);
            Vector3 n = { std::cos(lam) * std::cos(lom), std::sin(lam), std::cos(lam) * std::sin(lom) };
            auto P = [&](float la, float lo)
            {
                return Vector3{ c.x + r.x * std::cos(la) * std::cos(lo), c.y + r.y * std::sin(la), c.z + r.z * std::cos(la) * std::sin(lo) };
            };
            QuadAG(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraAG(color, n));
        }
    }
}


// Disco horizontal con color de centro y de borde (sin bruma si se pide).
static void DiscoAG(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, bool bruma = true)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };

        if (bruma)
        {
            VAG(c, NieblaAG(centro, c)); VAG(p1, NieblaAG(borde, p1)); VAG(p0, NieblaAG(borde, p0));
        }
        else
        {
            VAG(c, centro); VAG(p1, borde); VAG(p0, borde);
        }
    }
}


// Anillo horizontal entre r0 y r1 (opcionalmente solo un arco).
static void AnilloAG(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera, float desde = 0.0f, float hasta = 2.0f * PI)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = desde + (hasta - desde) * (float)k / (float)lados;
        float a1 = desde + (hasta - desde) * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColAG(i0, i1, o1, o0, dentro, dentro, fuera, fuera);
    }
}


// Disco que mira a la camara con centro y borde de distinto color (halos).
static void HaloAG(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derAG, std::cos(a0) * radio), Vector3Scale(g_arribaAG, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derAG, std::cos(a1) * radio), Vector3Scale(g_arribaAG, std::sin(a1) * radio)));
        VAG(c, centro); VAG(p0, borde); VAG(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas).
static void PuntoAG(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derAG, r), u = Vector3Scale(g_arribaAG, r);
    QuadColAG(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Tira que mira a la camara entre dos puntos (cuerdas, haces, estelas).
static void TiraAG(Vector3 a, Vector3 b, float anchoA, float anchoB, Color ca, Color cb)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 haciaOjo = Vector3Subtract(g_ojoAG, a);
    Vector3 lado = Vector3CrossProduct(eje, haciaOjo);
    float largo = Vector3Length(lado);

    if (largo < 0.00001f) return;

    lado = Vector3Scale(lado, 1.0f / largo);
    Vector3 la = Vector3Scale(lado, anchoA * 0.5f), lb = Vector3Scale(lado, anchoB * 0.5f);
    QuadColAG(Vector3Subtract(a, la), Vector3Add(a, la), Vector3Add(b, lb), Vector3Subtract(b, lb), ca, ca, cb, cb);
}


//--------------------------------------------------
// Horno: lo estatico se graba una vez en tres mallas
//--------------------------------------------------

struct HornoAG
{
    bool listo = false;
    int clave = -1;
    Mesh fondo{};
    bool fondoValido = false;
    Mesh suelo{};
    bool sueloValido = false;
    Mesh escena{};
    bool escenaValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoAG g_hornoAG;


static void EmpezarGrabacionAG()
{
    g_grabAG.activa = true;
    g_grabAG.posiciones.clear();
    g_grabAG.colores.clear();
}


// Cierra la grabacion y sube la malla (sin indices: cabe cualquier tamano).
static bool TerminarGrabacionAG(Mesh& malla)
{
    g_grabAG.activa = false;
    int n = (int)(g_grabAG.posiciones.size() / 3);
    n -= n % 3;

    if (n < 3)
    {
        g_grabAG.posiciones.clear();
        g_grabAG.colores.clear();
        return false;
    }

    malla = {};
    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * (int)sizeof(float)));
    malla.texcoords = (float*)MemAlloc((unsigned int)(n * 2 * (int)sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    for (int i = 0; i < n * 3; i++) malla.vertices[i] = g_grabAG.posiciones[(size_t)i];
    for (int i = 0; i < n * 4; i++) malla.colors[i] = g_grabAG.colores[(size_t)i];
    for (int i = 0; i < n * 2; i++) malla.texcoords[i] = 0.0f;

    UploadMesh(&malla, false);
    g_grabAG.posiciones.clear();
    g_grabAG.colores.clear();
    return true;
}


static void LiberarMallaAG(Mesh& malla, bool& valida)
{
    if (valida) UnloadMesh(malla);
    valida = false;
    malla = {};
}


static void LiberarMallasGlobo()
{
    LiberarMallaAG(g_hornoAG.fondo, g_hornoAG.fondoValido);
    LiberarMallaAG(g_hornoAG.suelo, g_hornoAG.sueloValido);
    LiberarMallaAG(g_hornoAG.escena, g_hornoAG.escenaValida);
    g_hornoAG.listo = false;
    g_hornoAG.clave = -1;
}


static void FondoEstaticoAG(bool decoracion, int paso);
static void SueloEstaticoAG(bool decoracion, bool sombras);
static void EscenaEstaticaAG(bool decoracion, int paso);


static int ClaveHornoAG()
{
    return (int)CalidadDecoracion() * 2 + (CalidadSombras() != CALIDAD_BAJA ? 1 : 0);
}


static void HornearAG()
{
    int clave = ClaveHornoAG();

    if ((g_hornoAG.listo && g_hornoAG.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasGlobo();

    if (!g_hornoAG.materialListo)
    {
        g_hornoAG.material = LoadMaterialDefault();
        g_hornoAG.materialListo = true;
    }

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    Vector3 ojo = g_ojoAG;
    g_ojoAG = OJO_AG;

    EmpezarGrabacionAG();
    FondoEstaticoAG(decoracion, paso);
    g_hornoAG.fondoValido = TerminarGrabacionAG(g_hornoAG.fondo);

    EmpezarGrabacionAG();
    SueloEstaticoAG(decoracion, sombras);
    g_hornoAG.sueloValido = TerminarGrabacionAG(g_hornoAG.suelo);

    EmpezarGrabacionAG();
    EscenaEstaticaAG(decoracion, paso);
    g_hornoAG.escenaValida = TerminarGrabacionAG(g_hornoAG.escena);

    g_ojoAG = ojo;
    g_hornoAG.clave = clave;
    g_hornoAG.listo = true;
}


static void DibujarMallaAG(const Mesh& malla, bool valida, bool profundidad)
{
    if (!g_hornoAG.listo || !valida) return;

    rlDrawRenderBatchActive();

    if (!profundidad) rlDisableDepthTest();

    DrawMesh(malla, g_hornoAG.material, MatrixIdentity());
    rlDrawRenderBatchActive();

    if (!profundidad) rlEnableDepthTest();
}


//--------------------------------------------------
// Fondo: torres, puentes y autopistas aereas
//--------------------------------------------------

// Autopistas aereas: arcos de elipse (x = cx + rx cos a, z = cz + rz sin a).
struct AutopistaAG { float cx, cz, rx, rz, y, a0, a1; };
static const AutopistaAG AUTOPISTAS_AG[] =
{
    { 0.0f, -14.0f, 34.0f, 10.0f, -4.5f, 1.05f * PI, 1.95f * PI },   // detras, bajo la plaza
    { 0.0f, -18.0f, 52.0f, 20.0f, 7.0f, 1.08f * PI, 1.92f * PI },    // detras, alta
    { -15.5f, 2.0f, 8.0f, 30.0f, -9.0f, 0.5f * PI, 1.5f * PI },      // izquierda, baja
    { 16.0f, -2.0f, 7.0f, 28.0f, 2.0f, -0.5f * PI, 0.5f * PI }       // derecha, a nivel
};
static const int CANTIDAD_AUTOPISTAS_AG = (int)(sizeof(AUTOPISTAS_AG) / sizeof(AUTOPISTAS_AG[0]));
static const float ANCHO_AUTOPISTA_AG = 2.4f;


static Vector3 PuntoAutopistaAG(const AutopistaAG& h, float a)
{
    return { h.cx + h.rx * std::cos(a), h.y, h.cz + h.rz * std::sin(a) };
}


static Vector3 TangenteAutopistaAG(const AutopistaAG& h, float a)
{
    return Vector3Normalize({ -h.rx * std::sin(a), 0.0f, h.rz * std::cos(a) });
}


static void AutopistaEstaticaAG(const AutopistaAG& h, int paso)
{
    int n = 56 / paso;
    Color asfalto = { 84, 94, 116, 255 };
    Color borde = { 226, 232, 240, 255 };

    for (int k = 0; k < n; k++)
    {
        float a0 = h.a0 + (h.a1 - h.a0) * (float)k / (float)n;
        float a1 = h.a0 + (h.a1 - h.a0) * (float)(k + 1) / (float)n;
        Vector3 p0 = PuntoAutopistaAG(h, a0), p1 = PuntoAutopistaAG(h, a1);
        Vector3 t0 = TangenteAutopistaAG(h, a0), t1 = TangenteAutopistaAG(h, a1);
        Vector3 l0 = { -t0.z, 0.0f, t0.x }, l1 = { -t1.z, 0.0f, t1.x };
        float m = ANCHO_AUTOPISTA_AG * 0.5f;

        // Tablero, canto inferior y barandas.
        QuadAG(Vector3Add(p0, Vector3Scale(l0, -m)), Vector3Add(p1, Vector3Scale(l1, -m)), Vector3Add(p1, Vector3Scale(l1, m)), Vector3Add(p0, Vector3Scale(l0, m)), CaraAG(asfalto, { 0.0f, 1.0f, 0.0f }));
        Vector3 abajo = { 0.0f, -0.45f, 0.0f };
        QuadAG(Vector3Add(p0, Vector3Scale(l0, m)), Vector3Add(p1, Vector3Scale(l1, m)), Vector3Add(Vector3Add(p1, Vector3Scale(l1, m)), abajo), Vector3Add(Vector3Add(p0, Vector3Scale(l0, m)), abajo), CaraAG(borde, l0));
        QuadAG(Vector3Add(p0, Vector3Scale(l0, -m)), Vector3Add(p1, Vector3Scale(l1, -m)), Vector3Add(Vector3Add(p1, Vector3Scale(l1, -m)), abajo), Vector3Add(Vector3Add(p0, Vector3Scale(l0, -m)), abajo), CaraAG(borde, Vector3Negate(l0)));
        Vector3 arriba = { 0.0f, 0.22f, 0.0f };
        QuadAG(Vector3Add(p0, Vector3Scale(l0, m)), Vector3Add(p1, Vector3Scale(l1, m)), Vector3Add(Vector3Add(p1, Vector3Scale(l1, m)), arriba), Vector3Add(Vector3Add(p0, Vector3Scale(l0, m)), arriba), TonoAG(borde, 0.9f));
        QuadAG(Vector3Add(p0, Vector3Scale(l0, -m)), Vector3Add(p1, Vector3Scale(l1, -m)), Vector3Add(Vector3Add(p1, Vector3Scale(l1, -m)), arriba), Vector3Add(Vector3Add(p0, Vector3Scale(l0, -m)), arriba), TonoAG(borde, 0.82f));

        // Linea central discontinua.
        if (k % 2 == 0)
        {
            Vector3 up = { 0.0f, 0.02f, 0.0f };
            QuadAG(Vector3Add(Vector3Add(p0, Vector3Scale(l0, -0.05f)), up), Vector3Add(Vector3Add(p1, Vector3Scale(l1, -0.05f)), up), Vector3Add(Vector3Add(p1, Vector3Scale(l1, 0.05f)), up), Vector3Add(Vector3Add(p0, Vector3Scale(l0, 0.05f)), up), Color{ 255, 226, 140, 255 });
        }

        // Anillo de levitacion bajo el tablero.
        if (k % 7 == 3)
        {
            g_emisAG = 1.0f;
            CilAG(Vector3Add(p0, Vector3{ 0.0f, -0.45f, 0.0f }), Vector3Add(p0, Vector3{ 0.0f, -0.75f, 0.0f }), 0.7f, 0.45f, 8, ENERGIA_AG);
            g_emisAG = 0.0f;
        }
    }
}


// Torre de cristal escalonada con jardines verticales y terrazas verdes.
static void TorreAG(float x, float z, float ancho, float hondo, float tope, int semilla, bool detalle, int paso)
{
    const float pie = -70.0f;
    Color vidrio = VariarAG(CRISTAL_AG, semilla, 41, 0.1f);

    if (Ruido01AG(semilla, 42) > 0.7f) vidrio = MezclaAG(vidrio, Color{ 200, 214, 226, 255 }, 0.5f);   // torre clara
    else if (Ruido01AG(semilla, 42) > 0.5f) vidrio = MezclaAG(vidrio, Color{ 96, 186, 196, 255 }, 0.4f); // verdosa

    int tramos = tope > 0.0f ? 3 : 2;
    float base = pie;
    float w = ancho, d = hondo;

    for (int t = 0; t < tramos; t++)
    {
        float alto = t == tramos - 1 ? tope : base + (tope - base) * (t == 0 ? 0.72f : 0.6f);

        if (t > 0 && alto - base < 2.0f) break;

        float hx = w * 0.5f, hz = d * 0.5f;
        Color oscuro = MezclaAG(vidrio, Color{ 30, 50, 80, 255 }, 0.45f);
        Color claro = MezclaAG(vidrio, Color{ 220, 238, 252, 255 }, 0.3f);

        // Caras: solo las que miran a la camara.
        const Vector3 normales[4] = { { 0, 0, 1 }, { 0, 0, -1 }, { -1, 0, 0 }, { 1, 0, 0 } };

        for (int f = 0; f < 4; f++)
        {
            Vector3 n = normales[f];
            Vector3 centro = { x + n.x * hx, 0.5f * (base + alto), z + n.z * hz };

            if (Vector3DotProduct(n, Vector3Subtract(OJO_AG, centro)) <= 0.0f) continue;

            Vector3 lado = { -n.z, 0.0f, n.x };
            float medio = (f < 2) ? hx : hz;
            Vector3 c0 = { centro.x - lado.x * medio, 0.0f, centro.z - lado.z * medio };
            Vector3 c1 = { centro.x + lado.x * medio, 0.0f, centro.z + lado.z * medio };
            float luz = LuzAG(n);
            Color abajo = TonoAG(oscuro, luz), arriba = TonoAG(claro, luz);
            Quad4AG({ c0.x, base, c0.z }, { c1.x, base, c1.z }, { c1.x, alto, c1.z }, { c0.x, alto, c0.z }, abajo, abajo, arriba, arriba);

            // Reflejo diagonal del cielo.
            float r0 = 0.15f + 0.4f * Ruido01AG(semilla + f, 43);
            Vector3 q0 = Vector3Lerp(c0, c1, r0), q1 = Vector3Lerp(c0, c1, r0 + 0.18f);
            Vector3 fuera = Vector3Scale(n, 0.02f);
            Color brillo = AlfaAG(Color{ 240, 250, 255, 255 }, 0.22f);
            VAG(Vector3Add({ q0.x, alto - 1.0f, q0.z }, fuera), NieblaAG(brillo, { q0.x, alto, q0.z }));
            VAG(Vector3Add({ q1.x, alto - 1.0f, q1.z }, fuera), NieblaAG(brillo, { q1.x, alto, q1.z }));
            VAG(Vector3Add({ q0.x, std::fmax(base, alto - 18.0f), q0.z }, fuera), NieblaAG(AlfaAG(brillo, 0.0f), { q0.x, alto - 18.0f, q0.z }));

            if (!detalle) continue;

            // Forjados claros cada piso y montantes verticales.
            float piso = 1.8f * (float)paso;
            float desde = std::fmax(base, -34.0f);
            Color forjado = TonoAG(MezclaAG(claro, Color{ 236, 242, 248, 255 }, 0.5f), luz);

            for (float y = alto - 0.6f; y > desde; y -= piso)
            {
                QuadAG(Vector3Add({ c0.x, y, c0.z }, fuera), Vector3Add({ c1.x, y, c1.z }, fuera), Vector3Add({ c1.x, y + 0.16f, c1.z }, fuera), Vector3Add({ c0.x, y + 0.16f, c0.z }, fuera), forjado);
            }

            int montantes = (int)(medio * 2.0f / (1.5f * (float)paso));
            Color montante = TonoAG(oscuro, luz * 0.85f);

            for (int k = 1; k < montantes; k++)
            {
                Vector3 p = Vector3Lerp(c0, c1, (float)k / (float)montantes);
                Vector3 e = Vector3Scale(lado, 0.04f);
                QuadAG(Vector3Add(Vector3Subtract({ p.x, desde, p.z }, e), fuera), Vector3Add(Vector3Add({ p.x, desde, p.z }, e), fuera), Vector3Add(Vector3Add({ p.x, alto, p.z }, e), fuera), Vector3Add(Vector3Subtract({ p.x, alto, p.z }, e), fuera), montante);
            }

            // Jardin vertical: franja de follaje en parches.
            if ((semilla + f) % 3 == 0)
            {
                float a0 = 0.18f + 0.3f * Ruido01AG(semilla + f, 44);
                float a1 = a0 + 0.3f + 0.15f * Ruido01AG(semilla + f, 45);
                float yTop = alto - 1.0f;
                float yBot = std::fmax(desde, alto - 22.0f);
                int columnas = 4;
                int filas = (int)((yTop - yBot) / (1.1f * (float)paso));
                Vector3 fuera2 = Vector3Scale(n, 0.08f);

                for (int i = 0; i < columnas; i++)
                {
                    for (int j = 0; j < filas; j++)
                    {
                        float u0 = a0 + (a1 - a0) * (float)i / (float)columnas;
                        float u1 = a0 + (a1 - a0) * (float)(i + 1) / (float)columnas;
                        float y0 = yTop - (float)j * 1.1f * (float)paso;
                        float y1 = y0 - 1.1f * (float)paso - 0.15f;
                        float hueco = Ruido01AG(i * 97 + j, semilla + 46);

                        if (hueco < 0.12f) continue;

                        Vector3 p0 = Vector3Lerp(c0, c1, u0), p1 = Vector3Lerp(c0, c1, u1);
                        Color hoja = VariarAG(hueco > 0.55f ? VERDE_AG : VERDE_OSCURO_AG, i * 31 + j, semilla + 47, 0.12f);
                        hoja = TonoAG(hoja, luz);
                        QuadAG(Vector3Add({ p0.x, y1, p0.z }, fuera2), Vector3Add({ p1.x, y1 + 0.2f, p1.z }, fuera2), Vector3Add({ p1.x, y0, p1.z }, fuera2), Vector3Add({ p0.x, y0 + 0.15f, p0.z }, fuera2), hoja);

                        // Flores sueltas.
                        if (hueco > 0.93f)
                        {
                            Vector3 pf = Vector3Lerp(p0, p1, 0.5f);
                            Vector3 f2 = Vector3Scale(n, 0.1f);
                            QuadAG(Vector3Add({ pf.x - lado.x * 0.2f, y0 - 0.5f, pf.z - lado.z * 0.2f }, f2), Vector3Add({ pf.x + lado.x * 0.2f, y0 - 0.5f, pf.z + lado.z * 0.2f }, f2), Vector3Add({ pf.x + lado.x * 0.2f, y0 - 0.2f, pf.z + lado.z * 0.2f }, f2), Vector3Add({ pf.x - lado.x * 0.2f, y0 - 0.2f, pf.z - lado.z * 0.2f }, f2), Color{ 255, 150, 190, 255 });
                        }
                    }
                }
            }
        }

        // Techo del tramo: terraza verde con arbolitos (o azotea tecnica).
        Vector3 techo = { x, alto, z };
        Color piso = Color{ 200, 206, 214, 255 };
        QuadAG({ x - hx, alto, z - hz }, { x - hx, alto, z + hz }, { x + hx, alto, z + hz }, { x + hx, alto, z - hz }, CaraAG(piso, { 0, 1, 0 }));

        if (detalle)
        {
            bool jardin = t < tramos - 1 || Ruido01AG(semilla, 48) > 0.4f;

            if (jardin)
            {
                QuadAG({ x - hx + 0.3f, alto + 0.05f, z - hz + 0.3f }, { x - hx + 0.3f, alto + 0.05f, z + hz - 0.3f }, { x + hx - 0.3f, alto + 0.05f, z + hz - 0.3f }, { x + hx - 0.3f, alto + 0.05f, z - hz + 0.3f }, CaraAG(VariarAG(VERDE_AG, semilla + t, 49, 0.1f), { 0, 1, 0 }));
                int arboles = (int)(w * d / 6.0f);
                if (arboles > 6) arboles = 6;

                for (int k = 0; k < arboles; k++)
                {
                    float ax = x + (Ruido01AG(k, semilla + t * 13 + 50) - 0.5f) * (w - 1.2f);
                    float az = z + (Ruido01AG(k, semilla + t * 13 + 51) - 0.5f) * (d - 1.2f);
                    float r = 0.5f + 0.4f * Ruido01AG(k, semilla + 52);
                    BolaAG({ ax, alto + 0.3f + r * 0.8f, az }, { r, r * 0.85f, r }, VariarAG(VERDE_OSCURO_AG, k, semilla + 53, 0.12f), 3, 6);
                }
            }
            else
            {
                CajaAG({ techo.x + w * 0.15f, alto + 0.6f, techo.z }, { w * 0.4f, 1.2f, d * 0.45f }, Color{ 180, 188, 200, 255 });
                CilAG({ x - w * 0.25f, alto, z }, { x - w * 0.25f, alto + 5.0f, z }, 0.12f, 0.06f, 5, Color{ 210, 216, 226, 255 });
            }
        }

        base = alto;
        w *= 0.78f;
        d *= 0.78f;
        x += (Ruido01AG(semilla + t, 54) - 0.5f) * w * 0.15f;
    }
}


struct TorreDatosAG { float x, z, ancho, hondo, tope; };


// Lista determinista de torres: fila trasera, costados y un segundo plano.
static int ListaTorresAG(TorreDatosAG* torres, int maximo)
{
    int n = 0;

    auto agregar = [&](float x, float z, float ancho, float hondo, float tope)
    {
        if (n < maximo) torres[n++] = { x, z, ancho, hondo, tope };
    };

    // Fila trasera cercana: alternan torres altas y bajas con terraza.
    for (int k = 0; k < 11; k++)
    {
        float x = -38.0f + (float)k * 7.6f + (Ruido01AG(k, 61) - 0.5f) * 2.4f;
        float z = -22.0f - 6.0f * Ruido01AG(k, 62);
        float alto = (k % 3 == 1) ? -9.0f + 8.0f * Ruido01AG(k, 63) : 14.0f + 26.0f * Ruido01AG(k, 64);
        agregar(x, z, 5.0f + 2.5f * Ruido01AG(k, 65), 5.0f + 2.0f * Ruido01AG(k, 66), alto);
    }

    // Costados.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 5; k++)
        {
            int s = k + (lado > 0 ? 20 : 30);
            float x = (float)lado * (24.0f + 4.0f * Ruido01AG(s, 67) + (k % 2) * 6.0f);
            float z = 12.0f - (float)k * 7.0f;
            float alto = (k % 2 == 0) ? 16.0f + 20.0f * Ruido01AG(s, 68) : -12.0f + 10.0f * Ruido01AG(s, 69);
            agregar(x, z, 5.0f + 2.0f * Ruido01AG(s, 70), 5.5f + 2.0f * Ruido01AG(s, 71), alto);
        }
    }

    // Segundo plano, mas lejos y palido por la niebla.
    for (int k = 0; k < 14; k++)
    {
        float x = -66.0f + (float)k * 10.2f + (Ruido01AG(k, 72) - 0.5f) * 4.0f;
        float z = -44.0f - 10.0f * Ruido01AG(k, 73);
        agregar(x, z, 6.0f + 3.0f * Ruido01AG(k, 74), 6.0f, 6.0f + 34.0f * Ruido01AG(k, 75));
    }

    // Torres bajas justo detras de la plaza: se ven sus terrazas desde arriba.
    agregar(-9.0f, -17.0f, 6.0f, 5.0f, -13.0f);
    agregar(8.0f, -18.0f, 7.0f, 5.5f, -8.0f);

    return n;
}


static void FondoEstaticoAG(bool decoracion, int paso)
{
    // Losa de la plataforma: canto con franja clara y quilla escalonada
    // (su cara de arriba queda bajo el suelo, que se pinta despues).
    Color losa = { 196, 204, 216, 255 };
    CajaAG({ 0.0f, -0.6f, 0.0f }, { PX_AG * 2.0f, 1.2f, PZ_AG * 2.0f }, losa);
    CajaAG({ 0.0f, -1.7f, 0.0f }, { PX_AG * 2.0f - 2.0f, 1.0f, PZ_AG * 2.0f - 2.0f }, TonoAG(losa, 0.86f));
    CajaAG({ 0.0f, -3.0f, 0.0f }, { PX_AG * 2.0f - 6.0f, 1.6f, PZ_AG * 2.0f - 5.0f }, TonoAG(losa, 0.74f));

    TorreDatosAG torres[48];
    int cantidad = ListaTorresAG(torres, 48);

    for (int k = 0; k < cantidad; k++)
    {
        bool lejana = torres[k].z < -40.0f;

        if (!decoracion && lejana) continue;

        TorreAG(torres[k].x, torres[k].z, torres[k].ancho, torres[k].hondo, torres[k].tope, 900 + k * 17, decoracion && !lejana, paso);
    }

    if (!decoracion) return;

    // Puentes de cristal entre torres vecinas de la fila trasera.
    for (int k = 0; k + 1 < 11; k++)
    {
        const TorreDatosAG& a = torres[k];
        const TorreDatosAG& b = torres[k + 1];
        float y = std::fmin(a.tope, b.tope) - 4.0f - 3.0f * Ruido01AG(k, 76);

        if (y < -20.0f || Ruido01AG(k, 77) < 0.35f) continue;

        Vector3 pa = { a.x + a.ancho * 0.5f, y, a.z + 0.3f };
        Vector3 pb = { b.x - b.ancho * 0.5f, y, b.z + 0.3f };
        VigaAG(pa, pb, 1.4f, 1.0f, Color{ 214, 222, 232, 255 });
        VigaAG(Vector3Add(pa, { 0.0f, 0.65f, 0.0f }), Vector3Add(pb, { 0.0f, 0.65f, 0.0f }), 1.2f, 0.3f, Color{ 140, 196, 226, 255 });
    }

    for (int h = 0; h < CANTIDAD_AUTOPISTAS_AG; h++)
    {
        AutopistaEstaticaAG(AUTOPISTAS_AG[h], paso);
    }
}


//--------------------------------------------------
// Suelo: losas de la plaza y paseo (sin profundidad)
//--------------------------------------------------

// Rectangulo horizontal (x0..x1, z0..z1) con color por esquina.
static void RectSueloAG(float x0, float z0, float x1, float z1, float y, Color c00, Color c10, Color c11, Color c01)
{
    Quad4AG({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, c00, c01, c11, c10);
}


static void RectPlanoAG(float x0, float z0, float x1, float z1, float y, Color c)
{
    RectSueloAG(x0, z0, x1, z1, y, c, c, c, c);
}


// Sombra suave sobre el suelo (elipse con borde transparente).
static void SombraSueloAG(float x, float z, float rx, float rz, float alfa)
{
    Color centro = { 20, 30, 52, CanalAG(255.0f * alfa) };
    Color borde = { 20, 30, 52, 0 };
    DiscoAG({ x, 0.0f, z }, rx, rz, 18, centro, borde, false);
}


// Utileria del paseo: posiciones compartidas por suelo (sombras) y escena.
struct ArbolAG { float x, z, r; };
static const ArbolAG ARBOLES_AG[] =
{
    { -11.7f, -8.5f, 1.05f }, { 11.7f, -8.5f, 1.05f },
    { -11.6f, -3.6f, 0.8f }, { 11.6f, -3.6f, 0.8f },
    { -11.7f, 4.6f, 0.75f }, { 11.7f, 4.6f, 0.75f }
};

struct FarolAG { float x, z; };
static const FarolAG FAROLES_AG[] =
{
    { -4.4f, -7.5f }, { 4.4f, -7.5f },
    { -10.2f, -0.9f }, { 10.2f, -0.9f },
    { -10.3f, 7.6f }, { 10.3f, 7.6f }
};


static void SueloEstaticoAG(bool decoracion, bool sombras)
{
    float exX = BX_AG + BORDILLO_AG;
    float exZ = BZ_AG + BORDILLO_AG;

    // Paseo: losetas claras con juntas.
    RectPlanoAG(-PX_AG, -PZ_AG, PX_AG, PZ_AG, 0.0f, TonoAG(PASEO_AG, 0.72f));

    const float lx = 1.34f, lz = 1.0f;
    int nx = (int)std::ceil(PX_AG * 2.0f / lx);
    int nz = (int)std::ceil(PZ_AG * 2.0f / lz);

    for (int i = 0; i < nx; i++)
    {
        for (int k = 0; k < nz; k++)
        {
            float x0 = -PX_AG + (float)i * lx + 0.03f;
            float x1 = std::fmin(-PX_AG + (float)(i + 1) * lx - 0.03f, PX_AG - 0.03f);
            float z0 = -PZ_AG + (float)k * lz + 0.03f;
            float z1 = std::fmin(-PZ_AG + (float)(k + 1) * lz - 0.03f, PZ_AG - 0.03f);

            // Las losetas que quedan bajo la arena no se ven.
            if (x0 > -exX && x1 < exX && z0 > -exZ && z1 < exZ) continue;

            Color c = VariarAG(PASEO_AG, i * 131 + k, 11, 0.05f);
            RectSueloAG(x0, z0, x1, z1, 0.0f, c, TonoAG(c, 1.03f), TonoAG(c, 0.98f), TonoAG(c, 0.95f));
        }
    }

    // Franja de borde al pie de la baranda (aviso de la caida).
    Color franja = { 70, 82, 104, 255 };
    RectPlanoAG(-PX_AG, -PZ_AG, PX_AG, -PZ_AG + 0.35f, 0.0f, franja);
    RectPlanoAG(-PX_AG, -PZ_AG, -PX_AG + 0.35f, PZ_AG, 0.0f, franja);
    RectPlanoAG(PX_AG - 0.35f, -PZ_AG, PX_AG, PZ_AG, 0.0f, franja);

    // Arena: paneles de composite azul pizarra con juntas finas.
    RectPlanoAG(-BX_AG, -BZ_AG, BX_AG, BZ_AG, 0.0f, JUNTA_AG);

    const int px = 12, pz = 10;
    float ax = BX_AG * 2.0f / (float)px;
    float az = BZ_AG * 2.0f / (float)pz;

    for (int i = 0; i < px; i++)
    {
        for (int k = 0; k < pz; k++)
        {
            float x0 = -BX_AG + (float)i * ax + 0.035f;
            float x1 = -BX_AG + (float)(i + 1) * ax - 0.035f;
            float z0 = -BZ_AG + (float)k * az + 0.035f;
            float z1 = -BZ_AG + (float)(k + 1) * az - 0.035f;
            Color base = ((i + k) % 2 == 0) ? LOSA_AG : MezclaAG(LOSA_AG, LOSA_CLARA_AG, 0.22f);
            Color c = VariarAG(base, i * 37 + k, 21, 0.045f);
            RectSueloAG(x0, z0, x1, z1, 0.0f, TonoAG(c, 0.97f), TonoAG(c, 1.04f), TonoAG(c, 1.01f), TonoAG(c, 0.95f));

            // Ranura de ventilacion en algunos paneles.
            if (decoracion && Ruido01AG(i * 37 + k, 22) > 0.84f)
            {
                float cx = 0.5f * (x0 + x1), cz = 0.5f * (z0 + z1);
                for (int r = -1; r <= 1; r++)
                {
                    RectPlanoAG(cx - 0.35f, cz + (float)r * 0.14f - 0.03f, cx + 0.35f, cz + (float)r * 0.14f + 0.03f, 0.0f, TonoAG(c, 0.78f));
                }
            }
        }
    }

    // Linea de carril: rectangulo discontinuo a 0.7 de la barrera.
    Color linea = { 236, 242, 248, 150 };
    float lxr = BX_AG - 0.7f, lzr = BZ_AG - 0.7f;

    for (float x = -lxr; x < lxr - 0.2f; x += 1.0f)
    {
        RectPlanoAG(x, -lzr - 0.05f, x + 0.55f, -lzr + 0.05f, 0.0f, linea);
        RectPlanoAG(x, lzr - 0.05f, x + 0.55f, lzr + 0.05f, 0.0f, linea);
    }

    for (float z = -lzr; z < lzr - 0.2f; z += 1.0f)
    {
        RectPlanoAG(-lxr - 0.05f, z, -lxr + 0.05f, z + 0.55f, 0.0f, linea);
        RectPlanoAG(lxr - 0.05f, z, lxr + 0.05f, z + 0.55f, 0.0f, linea);
    }

    // Emblema central: disco con un globo atado.
    Color emblema = { 108, 124, 152, 255 };
    DiscoAG({ 0.0f, 0.0f, 0.0f }, 2.35f, 2.35f, 40, TonoAG(emblema, 1.05f), emblema);
    AnilloAG({ 0.0f, 0.0f, 0.0f }, 2.35f, 2.5f, 40, Color{ 236, 242, 248, 200 }, Color{ 236, 242, 248, 200 });
    AnilloAG({ 0.0f, 0.0f, 0.0f }, 1.72f, 1.8f, 40, Color{ 160, 220, 240, 150 }, Color{ 160, 220, 240, 150 });
    Color dibujo = { 226, 236, 246, 190 };
    DiscoAG({ 0.0f, 0.0f, -0.35f }, 0.62f, 0.74f, 20, dibujo, dibujo, false);
    TriAG({ -0.12f, 0.0f, 0.46f }, { 0.12f, 0.0f, 0.46f }, { 0.0f, 0.0f, 0.34f }, dibujo);

    for (int k = 0; k < 6; k++)
    {
        float z0 = 0.46f + (float)k * 0.18f;
        float x0 = 0.08f * std::sin((float)k * 1.4f);
        float x1 = 0.08f * std::sin((float)(k + 1) * 1.4f);
        QuadColAG({ x0 - 0.025f, 0.0f, z0 }, { x0 + 0.025f, 0.0f, z0 }, { x1 + 0.025f, 0.0f, z0 + 0.18f }, { x1 - 0.025f, 0.0f, z0 + 0.18f }, dibujo, dibujo, dibujo, dibujo);
    }

    // Marcas de salida: chevrones que miran al centro.
    for (int s = 0; s < MAX_PARTICIPANTES; s++)
    {
        float sx = SPAWN_X_GLOBO[s], sz = SPAWN_Z_GLOBO[s];
        float largo = std::sqrt(sx * sx + sz * sz);
        Vector3 d = { -sx / largo, 0.0f, -sz / largo };
        Vector3 l = { -d.z, 0.0f, d.x };
        Color c = { 236, 242, 248, 110 };

        for (int k = 0; k < 2; k++)
        {
            Vector3 p = { sx + d.x * (0.9f + 0.45f * (float)k), 0.0f, sz + d.z * (0.9f + 0.45f * (float)k) };
            Vector3 punta = Vector3Add(p, Vector3Scale(d, 0.3f));
            QuadColAG(Vector3Add(p, Vector3Scale(l, 0.42f)), Vector3Add(punta, Vector3Scale(l, 0.06f)), Vector3Add(punta, Vector3Scale(d, -0.14f)), Vector3Add(p, Vector3Add(Vector3Scale(l, 0.42f), Vector3Scale(d, -0.14f))), c, c, c, c);
            QuadColAG(Vector3Add(p, Vector3Scale(l, -0.42f)), Vector3Add(punta, Vector3Scale(l, -0.06f)), Vector3Add(punta, Vector3Scale(d, -0.14f)), Vector3Add(p, Vector3Add(Vector3Scale(l, -0.42f), Vector3Scale(d, -0.14f))), c, c, c, c);
        }
    }

    // Zocalos de las placas de carga: aqui (y solo aqui) aparecen.
    for (int s = 0; s < CANTIDAD_SITIOS_PLACA_GLOBO; s++)
    {
        Vector3 c = { SITIOS_PLACA_X_GLOBO[s], 0.0f, SITIOS_PLACA_Z_GLOBO[s] };
        DiscoAG(c, RADIO_PLACA_GLOBO + 0.12f, RADIO_PLACA_GLOBO + 0.12f, 28, Color{ 112, 122, 140, 255 }, Color{ 106, 116, 134, 255 });
        DiscoAG(c, RADIO_PLACA_GLOBO, RADIO_PLACA_GLOBO, 28, Color{ 146, 156, 172, 255 }, Color{ 132, 142, 160, 255 });
        AnilloAG(c, 0.5f, 0.55f, 24, Color{ 118, 128, 146, 255 }, Color{ 118, 128, 146, 255 });

        for (int k = 0; k < 8; k++)
        {
            float a = (float)k / 8.0f * 2.0f * PI;
            Vector3 p = { c.x + std::cos(a) * 0.86f, 0.0f, c.z + std::sin(a) * 0.86f };
            DiscoAG(p, 0.06f, 0.06f, 8, Color{ 150, 196, 190, 255 }, Color{ 136, 178, 174, 255 });
        }
    }

    // Plataformas de drones en las esquinas delanteras del paseo.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 c = { (float)lado * 11.7f, 0.0f, 8.6f };
        DiscoAG(c, 0.95f, 0.95f, 24, Color{ 92, 104, 124, 255 }, Color{ 80, 92, 112, 255 });
        AnilloAG(c, 0.82f, 0.9f, 24, Color{ 255, 206, 96, 255 }, Color{ 255, 206, 96, 255 });
        RectPlanoAG(c.x - 0.32f, c.z - 0.4f, c.x - 0.2f, c.z + 0.4f, 0.0f, BLANCO_AG);
        RectPlanoAG(c.x + 0.2f, c.z - 0.4f, c.x + 0.32f, c.z + 0.4f, 0.0f, BLANCO_AG);
        RectPlanoAG(c.x - 0.2f, c.z - 0.06f, c.x + 0.2f, c.z + 0.06f, 0.0f, BLANCO_AG);
    }

    if (!sombras) return;

    // Sombra de contacto al pie del bordillo, hacia adentro.
    Color sOsc = { 20, 30, 52, 70 }, sNada = { 20, 30, 52, 0 };
    RectSueloAG(-BX_AG, -BZ_AG, BX_AG, -BZ_AG + 0.55f, 0.0f, sOsc, sOsc, sNada, sNada);
    RectSueloAG(-BX_AG, BZ_AG - 0.4f, BX_AG, BZ_AG, 0.0f, sNada, sNada, sOsc, sOsc);
    RectSueloAG(-BX_AG, -BZ_AG, -BX_AG + 0.5f, BZ_AG, 0.0f, sOsc, sNada, sNada, sOsc);
    RectSueloAG(BX_AG - 0.5f, -BZ_AG, BX_AG, BZ_AG, 0.0f, sNada, sOsc, sOsc, sNada);

    if (!decoracion) return;

    // Sombras de copas y jardineras (el sol entra desde atras a la derecha).
    for (const ArbolAG& a : ARBOLES_AG)
    {
        SombraSueloAG(a.x - 0.5f, a.z + 0.55f, a.r * 1.5f, a.r * 1.2f, 0.32f);
    }

    for (const FarolAG& f : FAROLES_AG)
    {
        SombraSueloAG(f.x - 0.2f, f.z + 0.25f, 0.35f, 0.25f, 0.3f);
    }
}


//--------------------------------------------------
// Escena: bordillo, barrera y utileria del paseo
//--------------------------------------------------

// Altura de la barrera de energia: baja al frente para ver los autos.
static float AltoBarreraAG(float z)
{
    return z > BZ_AG - 0.1f ? 0.5f : 0.95f;
}


static void PosteBarreraAG(float x, float z, float alto, bool esquina)
{
    float r = esquina ? 0.24f : 0.11f;
    CajaAG({ x, 0.28f + alto * 0.5f, z }, { r * 2.0f, alto, r * 2.0f }, esquina ? BLANCO_AG : METAL_AG);
    CajaAG({ x, 0.3f + alto + 0.05f, z }, { r * 2.4f, 0.1f, r * 2.4f }, BLANCO_AG);

    if (esquina)
    {
        CilAG({ x, 0.3f + alto + 0.1f, z }, { x, 0.3f + alto + 0.32f, z }, 0.16f, 0.1f, 8, METAL_OSCURO_AG);
    }
}


static void JardineraAG(float x, float z, float r, int semilla, bool detalle)
{
    CilAG({ x, 0.0f, z }, { x, 0.55f, z }, r, r * 1.06f, 16, BLANCO_AG);
    CilAG({ x, 0.55f, z }, { x, 0.62f, z }, r * 1.1f, r * 1.1f, 16, Color{ 210, 216, 224, 255 });
    DiscoAG({ x, 0.63f, z }, r * 0.98f, r * 0.98f, 16, Color{ 92, 66, 46, 255 }, Color{ 76, 54, 40, 255 });

    // Arbol: tronco y copa en tres bolas.
    float alto = 1.5f + r * 0.8f;
    CilAG({ x, 0.6f, z }, { x + 0.05f, alto, z }, 0.1f, 0.07f, 6, Color{ 120, 86, 60, 255 });
    int anillos = detalle ? 4 : 3;
    int lados = detalle ? 9 : 7;
    float rc = r * 1.0f;
    BolaAG({ x, alto + rc * 0.5f, z }, { rc, rc * 0.82f, rc }, VariarAG(VERDE_AG, semilla, 31, 0.08f), anillos, lados);
    BolaAG({ x - rc * 0.55f, alto + rc * 0.2f, z + rc * 0.2f }, { rc * 0.7f, rc * 0.6f, rc * 0.7f }, VariarAG(VERDE_OSCURO_AG, semilla, 32, 0.1f), anillos, lados);
    BolaAG({ x + rc * 0.4f, alto + rc * 1.0f, z - rc * 0.15f }, { rc * 0.62f, rc * 0.5f, rc * 0.62f }, MezclaAG(VERDE_AG, Color{ 170, 214, 104, 255 }, 0.45f), anillos, lados);

    // Flores al pie.
    if (detalle)
    {
        const Color flores[3] = { { 255, 140, 190, 255 }, { 255, 236, 140, 255 }, { 250, 250, 255, 255 } };
        for (int k = 0; k < 10; k++)
        {
            float a = (float)k / 10.0f * 2.0f * PI + Ruido01AG(k, semilla) * 0.4f;
            float rr = r * (0.62f + 0.28f * Ruido01AG(k, semilla + 1));
            BolaAG({ x + std::cos(a) * rr, 0.72f, z + std::sin(a) * rr }, { 0.1f, 0.08f, 0.1f }, flores[k % 3], 2, 5);
        }
    }
}


static void BancoAG(float x, float z, float giro)
{
    CajaGiradaAG({ x, 0.42f, z }, { 1.6f, 0.08f, 0.5f }, giro, MADERA_AG);
    float a = giro * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };

    for (int k = -1; k <= 1; k += 2)
    {
        Vector3 p = { x + ux.x * 0.62f * (float)k, 0.2f, z + ux.z * 0.62f * (float)k };
        CajaGiradaAG(p, { 0.1f, 0.4f, 0.46f }, giro, BLANCO_AG);
    }

    // Respaldo del lado contrario a la arena.
    Vector3 r = { x - uz.x * 0.24f, 0.68f, z - uz.z * 0.24f };
    CajaGiradaAG(r, { 1.6f, 0.34f, 0.07f }, giro, TonoAG(MADERA_AG, 0.92f));
}


static void FarolEscenaAG(float x, float z)
{
    CilAG({ x, 0.0f, z }, { x, 0.25f, z }, 0.2f, 0.16f, 8, METAL_OSCURO_AG);
    CilAG({ x, 0.25f, z }, { x, 3.1f, z }, 0.06f, 0.05f, 6, METAL_AG);
    // Cabeza en anillo (la luz va en el pase aditivo).
    CilAG({ x, 3.1f, z }, { x, 3.26f, z }, 0.34f, 0.3f, 12, BLANCO_AG);
    DiscoAG({ x, 3.09f, z }, 0.28f, 0.28f, 12, Color{ 255, 250, 228, 255 }, Color{ 255, 244, 200, 255 });
}


static void KioscoAG(float x, float z, float giro)
{
    float a = giro * DEG2RAD;
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };
    CajaGiradaAG({ x, 0.85f, z }, { 0.8f, 1.7f, 0.5f }, giro, BLANCO_AG);
    CajaGiradaAG({ x, 1.78f, z }, { 0.96f, 0.12f, 0.64f }, giro, METAL_OSCURO_AG);
    // Pantalla con un globo dibujado (frente hacia la arena).
    g_emisAG = 1.0f;
    CajaGiradaAG({ x + uz.x * 0.26f, 1.15f, z + uz.z * 0.26f }, { 0.6f, 0.7f, 0.04f }, giro, Color{ 90, 200, 230, 255 });
    CajaGiradaAG({ x + uz.x * 0.285f, 1.22f, z + uz.z * 0.285f }, { 0.22f, 0.26f, 0.02f }, giro, Color{ 220, 250, 255, 255 });
    CajaGiradaAG({ x + uz.x * 0.26f, 0.45f, z + uz.z * 0.26f }, { 0.4f, 0.1f, 0.03f }, giro, CARGA_AG);
    g_emisAG = 0.0f;
}


static void CanteroAG(float x0, float x1, float z, float hondo, int semilla)
{
    CajaAG({ 0.5f * (x0 + x1), 0.16f, z }, { x1 - x0, 0.32f, hondo }, BLANCO_AG);
    CajaAG({ 0.5f * (x0 + x1), 0.33f, z }, { x1 - x0 - 0.1f, 0.04f, hondo - 0.1f }, Color{ 88, 64, 46, 255 });
    int n = (int)((x1 - x0) / 0.32f);
    const Color flores[4] = { { 255, 128, 176, 255 }, { 255, 230, 120, 255 }, { 196, 150, 255, 255 }, { 250, 250, 255, 255 } };

    for (int k = 0; k < n; k++)
    {
        float x = x0 + 0.18f + (float)k * 0.32f;
        float dz = (Ruido01AG(k, semilla) - 0.5f) * (hondo - 0.3f);
        BolaAG({ x, 0.46f, z + dz }, { 0.2f, 0.16f, 0.2f }, VariarAG(VERDE_AG, k, semilla + 1, 0.12f), 2, 6);

        if (Ruido01AG(k, semilla + 2) > 0.35f)
        {
            BolaAG({ x + 0.05f, 0.6f, z + dz }, { 0.08f, 0.07f, 0.08f }, flores[k % 4], 2, 5);
        }
    }
}


static void PilonHoloAG(float x, float z)
{
    CilAG({ x, 0.0f, z }, { x, 0.35f, z }, 0.62f, 0.55f, 16, BLANCO_AG);
    CilAG({ x, 0.35f, z }, { x, 0.5f, z }, 0.42f, 0.36f, 16, METAL_OSCURO_AG);
    g_emisAG = 1.0f;
    CilAG({ x, 0.5f, z }, { x, 0.56f, z }, 0.3f, 0.3f, 16, ENERGIA_AG);
    g_emisAG = 0.0f;
}


static void MastilAG(float x, float z)
{
    CilAG({ x, 0.0f, z }, { x, 0.2f, z }, 0.16f, 0.12f, 8, METAL_OSCURO_AG);
    CilAG({ x, 0.2f, z }, { x, 4.2f, z }, 0.045f, 0.035f, 6, BLANCO_AG);
    BolaAG({ x, 4.24f, z }, { 0.08f, 0.08f, 0.08f }, Color{ 255, 206, 96, 255 }, 2, 6);
}


static void EscenaEstaticaAG(bool decoracion, int paso)
{
    float exX = BX_AG + BORDILLO_AG;

    // Bordillo de la arena: zocalo blanco con canto oscuro y la base de la
    // barrera (el borde interior es el limite real).
    Color bordillo = { 222, 228, 236, 255 };
    CajaAG({ 0.0f, 0.14f, -BZ_AG - BORDILLO_AG * 0.5f }, { exX * 2.0f, 0.28f, BORDILLO_AG }, bordillo);
    CajaAG({ 0.0f, 0.14f, BZ_AG + BORDILLO_AG * 0.5f }, { exX * 2.0f, 0.28f, BORDILLO_AG }, bordillo);
    CajaAG({ -BX_AG - BORDILLO_AG * 0.5f, 0.14f, 0.0f }, { BORDILLO_AG, 0.28f, BZ_AG * 2.0f }, bordillo);
    CajaAG({ BX_AG + BORDILLO_AG * 0.5f, 0.14f, 0.0f }, { BORDILLO_AG, 0.28f, BZ_AG * 2.0f }, bordillo);

    // Riel emisor sobre el bordillo (la cortina se dibuja por frame).
    Color riel = METAL_OSCURO_AG;
    CajaAG({ 0.0f, 0.3f, -BZ_AG - 0.2f }, { exX * 2.0f, 0.05f, 0.16f }, riel);
    CajaAG({ 0.0f, 0.3f, BZ_AG + 0.2f }, { exX * 2.0f, 0.05f, 0.16f }, riel);
    CajaAG({ -BX_AG - 0.2f, 0.3f, 0.0f }, { 0.16f, 0.05f, BZ_AG * 2.0f }, riel);
    CajaAG({ BX_AG + 0.2f, 0.3f, 0.0f }, { 0.16f, 0.05f, BZ_AG * 2.0f }, riel);

    // Postes emisores de la barrera.
    for (int k = 0; k <= 12; k++)
    {
        float x = -BX_AG - 0.2f + (float)k * (BX_AG + 0.2f) * 2.0f / 12.0f;
        bool esquina = k == 0 || k == 12;
        PosteBarreraAG(x, -BZ_AG - 0.2f, AltoBarreraAG(-BZ_AG), esquina);
        PosteBarreraAG(x, BZ_AG + 0.2f, AltoBarreraAG(BZ_AG), esquina);
    }

    for (int k = 1; k < 8; k++)
    {
        float z = -BZ_AG - 0.2f + (float)k * (BZ_AG + 0.2f) * 2.0f / 8.0f;
        PosteBarreraAG(-BX_AG - 0.2f, z, AltoBarreraAG(z), false);
        PosteBarreraAG(BX_AG + 0.2f, z, AltoBarreraAG(z), false);
    }

    // Baranda de vidrio al borde de la plataforma: pasamanos y postes (el
    // vidrio translucido va por frame) y un canto luminoso que marca la caida.
    Color pasamanos = { 206, 214, 226, 255 };
    VigaAG({ -PX_AG + 0.15f, 1.1f, -PZ_AG + 0.15f }, { PX_AG - 0.15f, 1.1f, -PZ_AG + 0.15f }, 0.1f, 0.08f, pasamanos);
    VigaAG({ -PX_AG + 0.15f, 1.1f, -PZ_AG + 0.15f }, { -PX_AG + 0.15f, 1.1f, PZ_AG }, 0.1f, 0.08f, pasamanos);
    VigaAG({ PX_AG - 0.15f, 1.1f, -PZ_AG + 0.15f }, { PX_AG - 0.15f, 1.1f, PZ_AG }, 0.1f, 0.08f, pasamanos);

    for (int k = 0; k <= 14; k++)
    {
        float x = -PX_AG + 0.15f + (float)k * (PX_AG - 0.15f) * 2.0f / 14.0f;
        CajaAG({ x, 0.55f, -PZ_AG + 0.15f }, { 0.08f, 1.1f, 0.08f }, METAL_AG);
    }

    for (int k = 1; k <= 10; k++)
    {
        float z = -PZ_AG + 0.15f + (float)k * (PZ_AG * 2.0f - 0.15f) / 10.0f;
        CajaAG({ -PX_AG + 0.15f, 0.55f, z }, { 0.08f, 1.1f, 0.08f }, METAL_AG);
        CajaAG({ PX_AG - 0.15f, 0.55f, z }, { 0.08f, 1.1f, 0.08f }, METAL_AG);
    }

    if (!decoracion) return;

    // Utileria del paseo.
    for (int k = 0; k < (int)(sizeof(ARBOLES_AG) / sizeof(ARBOLES_AG[0])); k++)
    {
        JardineraAG(ARBOLES_AG[k].x, ARBOLES_AG[k].z, ARBOLES_AG[k].r * 0.75f, 400 + k * 7, paso == 1);
    }

    for (const FarolAG& f : FAROLES_AG)
    {
        FarolEscenaAG(f.x, f.z);
    }

    BancoAG(-7.6f, -8.3f, 0.0f);
    BancoAG(7.6f, -8.3f, 0.0f);
    BancoAG(-11.9f, 2.0f, 90.0f);
    BancoAG(11.9f, 2.0f, -90.0f);

    KioscoAG(-12.5f, -1.0f, 90.0f);
    KioscoAG(12.5f, -1.0f, -90.0f);

    PilonHoloAG(0.0f, -8.4f);

    CanteroAG(-9.4f, -5.6f, -9.45f, 0.7f, 501);
    CanteroAG(-3.0f, -1.4f, -9.45f, 0.7f, 502);
    CanteroAG(1.4f, 3.0f, -9.45f, 0.7f, 503);
    CanteroAG(5.6f, 9.4f, -9.45f, 0.7f, 504);
    CanteroAG(-8.4f, -2.4f, 7.5f, 0.6f, 505);
    CanteroAG(2.4f, 8.4f, 7.5f, 0.6f, 506);

    MastilAG(-12.8f, 6.3f);
    MastilAG(12.8f, 6.3f);

    if (paso == 1)
    {
        // Papeleras y totems informativos.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = (float)lado * 10.2f;
            CilAG({ x, 0.0f, 5.9f }, { x, 0.75f, 5.9f }, 0.22f, 0.24f, 10, Color{ 120, 180, 120, 255 });
            CilAG({ x, 0.75f, 5.9f }, { x, 0.82f, 5.9f }, 0.26f, 0.26f, 10, METAL_OSCURO_AG);
            CajaAG({ (float)lado * 2.0f, 0.9f, -7.4f }, { 0.36f, 1.8f, 0.16f }, METAL_OSCURO_AG);
            g_emisAG = 1.0f;
            CajaAG({ (float)lado * 2.0f, 1.2f, -7.31f }, { 0.28f, 0.9f, 0.02f }, Color{ 150, 220, 250, 255 });
            g_emisAG = 0.0f;
        }
    }
}


//--------------------------------------------------
// Camara y cielo
//--------------------------------------------------

static Camera3D CamaraVistaAG(const MinijuegoAutosGlobo& m)
{
    Camera3D c = m.camara;
    c.position = OJO_AG;
    c.target = MIRA_AG;
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = FOV_AG;
    c.projection = CAMERA_PERSPECTIVE;

    // Entrada: la camara llega volando desde la ciudad, de costado y desde
    // mas alto, durante la cuenta regresiva.
    if (m.fase == FASE_AUTOS_GLOBO_PREPARACION && m.resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        float u = Lim01AG(1.0f - m.tiempoPreparacion / DURACION_PREPARACION_GLOBO);
        float e = 1.0f - u;
        e = e * e * (3.0f - 2.0f * e) * ObtenerFactorMovimientoCamaraMinijuegos();
        Vector3 atras = Vector3Normalize(Vector3Subtract(OJO_AG, MIRA_AG));
        c.position = Vector3Add(c.position, Vector3Scale(atras, 9.0f * e));
        c.position.x -= 7.0f * e;
        c.position.y += 2.5f * e;
        c.target.x -= 1.5f * e;
    }

    return AplicarTemblorGeneralACamara(c);
}


// Punto del cielo segun su elevacion (grados; negativa = bajo el horizonte
// del ojo): el degrade sigue a la camara durante la entrada.
static Vector2 PuntoCieloAG(const Camera3D& vista, float x, float elevacion)
{
    const float z = -70.0f;
    float d = vista.position.z - z;
    return GetWorldToScreen({ x, vista.position.y + std::tan(elevacion * DEG2RAD) * d, z }, vista);
}


static void NubeAG(float x, float y, float largo, float alto, Color cuerpo, Color luz)
{
    DrawEllipse((int)x, (int)y, largo * 0.5f, alto, cuerpo);
    DrawEllipse((int)(x - largo * 0.18f), (int)(y - alto * 0.55f), largo * 0.24f, alto * 0.95f, cuerpo);
    DrawEllipse((int)(x + largo * 0.12f), (int)(y - alto * 0.8f), largo * 0.2f, alto * 1.1f, cuerpo);
    DrawEllipse((int)(x + largo * 0.08f), (int)(y - alto * 0.95f), largo * 0.14f, alto * 0.7f, luz);
}


static void DibujarCieloAG(const Camera3D& vista, float t, bool decoracion)
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float s = (float)h / 720.0f;

    // Degrade por elevacion: celeste arriba y bruma clara abajo, donde
    // queda la ciudad a cientos de metros.
    const float elev[5] = { -4.0f, -12.0f, -19.0f, -27.0f, -36.0f };
    const Color col[5] = { CIELO_ALTO_AG, CIELO_MEDIO_AG, CIELO_BAJO_AG, HORIZONTE_AG, NIEBLA_AG };
    float ys[5];

    for (int k = 0; k < 5; k++) ys[k] = PuntoCieloAG(vista, 0.0f, elev[k]).y;

    if (ys[0] > 0.0f) DrawRectangle(0, 0, w, (int)ys[0] + 1, col[0]);

    for (int k = 0; k < 4; k++)
    {
        int y0 = (int)ys[k], y1 = (int)ys[k + 1];
        if (y1 > y0 && y1 > 0 && y0 < h) DrawRectangleGradientV(0, y0, w, y1 - y0 + 1, col[k], col[k + 1]);
    }

    if (ys[4] < (float)h) DrawRectangle(0, (int)ys[4], w, h - (int)ys[4], col[4]);

    // Sol del mediodia arriba a la derecha, casi fuera de cuadro.
    Vector2 sol = PuntoCieloAG(vista, 46.0f, -5.0f);
    float radioSol = 30.0f * s;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(sol, radioSol * 9.0f, Fade(Color{ 255, 244, 210, 255 }, 0.26f), Fade(Color{ 255, 244, 210, 255 }, 0.0f));
    DrawCircleGradient(sol, radioSol * 3.0f, Fade(SOL_AG, 0.5f), Fade(SOL_AG, 0.0f));
    EndBlendMode();
    DrawCircleV(sol, radioSol, Color{ 255, 252, 236, 255 });

    // Ciudad lejana en dos planos: columnas de torres que se pierden en la
    // bruma de abajo (la de atras mas palida y fina).
    for (int plano = 0; plano < 2; plano++)
    {
        int cantidad = decoracion ? (plano == 0 ? 34 : 22) : (plano == 0 ? 16 : 10);
        float yPie = PuntoCieloAG(vista, 0.0f, plano == 0 ? -24.0f : -29.0f).y;
        Color cuerpo = plano == 0
            ? MezclaAG(Color{ 120, 158, 196, 255 }, CIELO_BAJO_AG, 0.55f)
            : MezclaAG(Color{ 96, 136, 178, 255 }, CIELO_BAJO_AG, 0.32f);
        Color luz = MezclaAG(cuerpo, Color{ 236, 246, 255, 255 }, 0.35f);

        for (int k = 0; k < cantidad; k++)
        {
            int semilla = 700 + plano * 100;
            float x = (float)w * ((float)k + 0.5f * Ruido01AG(k, semilla)) / (float)cantidad;
            float ancho = (plano == 0 ? 18.0f : 30.0f) * s * (0.6f + 0.8f * Ruido01AG(k, semilla + 1));
            float tope = PuntoCieloAG(vista, 0.0f, (plano == 0 ? -8.0f : -12.0f) - 9.0f * Ruido01AG(k, semilla + 2)).y;

            if (tope >= yPie) continue;

            DrawRectangleGradientV((int)(x - ancho * 0.5f), (int)tope, (int)ancho, (int)(yPie - tope), cuerpo, MezclaAG(cuerpo, NIEBLA_AG, 0.8f));
            DrawRectangle((int)(x + ancho * 0.18f), (int)tope, (int)(ancho * 0.14f), (int)(yPie - tope), Fade(luz, 0.55f));

            // Antena o cupula en algunas.
            if (Ruido01AG(k, semilla + 3) > 0.6f)
            {
                DrawRectangle((int)(x - 1.0f * s), (int)(tope - 14.0f * s), (int)(2.0f * s) + 1, (int)(14.0f * s), cuerpo);
            }
        }
    }

    // Mar de nubes muy abajo, entre las torres.
    int nubes = decoracion ? 9 : 4;

    for (int i = 0; i < nubes; i++)
    {
        float e = -22.0f - 12.0f * Ruido01AG(i, 811);
        float y = PuntoCieloAG(vista, 0.0f, e).y;
        float largo = (180.0f + 220.0f * Ruido01AG(i, 812)) * s;
        float alto = (14.0f + 10.0f * Ruido01AG(i, 813)) * s;
        float recorrido = (float)w + largo * 2.0f;
        float x = std::fmod(Ruido01AG(i, 814) * recorrido + t * (4.0f + 5.0f * Ruido01AG(i, 815)) * s, recorrido) - largo;
        Color cuerpo = MezclaAG(Color{ 250, 252, 255, 255 }, NIEBLA_AG, 0.25f + 0.3f * Ruido01AG(i, 816));
        DrawEllipse((int)x, (int)(y + alto * 0.3f), largo * 0.46f, alto * 0.6f, MezclaAG(cuerpo, CIELO_BAJO_AG, 0.35f));
        NubeAG(x, y, largo, alto, cuerpo, Color{ 255, 255, 255, 255 });
    }
}


//--------------------------------------------------
// Ciudad en movimiento: trafico aereo (antes del suelo)
//--------------------------------------------------

static void DibujarTraficoAG(float t, int paso)
{
    const Color carrocerias[5] = { { 240, 242, 246, 255 }, { 176, 186, 200, 255 }, { 90, 200, 210, 255 }, { 255, 170, 90, 255 }, { 190, 150, 230, 255 } };

    LoteAG();

    for (int h = 0; h < CANTIDAD_AUTOPISTAS_AG; h++)
    {
        const AutopistaAG& autopista = AUTOPISTAS_AG[h];
        int cantidad = 10 / paso;

        for (int k = 0; k < cantidad; k++)
        {
            int s = h * 31 + k;
            bool ida = (k % 2) == 0;
            float velocidad = 0.018f + 0.01f * Ruido01AG(s, 81);
            float u = std::fmod(Ruido01AG(s, 82) + t * velocidad, 1.0f);

            if (!ida) u = 1.0f - u;

            float a = autopista.a0 + (autopista.a1 - autopista.a0) * u;
            Vector3 p = PuntoAutopistaAG(autopista, a);
            Vector3 f = TangenteAutopistaAG(autopista, a);

            if (!ida) f = Vector3Negate(f);

            Vector3 l = { -f.z, 0.0f, f.x };
            p = Vector3Add(p, Vector3Scale(l, -0.55f));
            p.y += 0.32f + 0.05f * std::sin(t * 3.0f + (float)s);
            Color c = carrocerias[s % 5];
            CajaEjesAG(p, Vector3Scale(f, 0.5f), { 0.0f, 0.13f, 0.0f }, Vector3Scale(l, 0.25f), c, true);
            CajaEjesAG(Vector3Add(Vector3Add(p, Vector3Scale(f, -0.08f)), { 0.0f, 0.2f, 0.0f }), Vector3Scale(f, 0.24f), { 0.0f, 0.08f, 0.0f }, Vector3Scale(l, 0.2f), Color{ 60, 90, 120, 255 });
            g_emisAG = 1.0f;
            CajaEjesAG(Vector3Add(p, Vector3Scale(f, -0.5f)), Vector3Scale(f, 0.02f), { 0.0f, 0.05f, 0.0f }, Vector3Scale(l, 0.2f), Color{ 255, 80, 80, 255 });
            g_emisAG = 0.0f;
        }
    }

    FinLoteAG();
}


//--------------------------------------------------
// Marcas del suelo (sin profundidad, despues del suelo horneado)
//--------------------------------------------------

// Elipse orientada sobre el suelo (sombra del auto).
static void ElipseSueloAG(Vector3 c, Vector3 eje, float largo, float ancho, Color centro, Color borde)
{
    Vector3 l = { -eje.z, 0.0f, eje.x };

    for (int k = 0; k < 18; k++)
    {
        float a0 = 2.0f * PI * (float)k / 18.0f;
        float a1 = 2.0f * PI * (float)(k + 1) / 18.0f;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(eje, std::cos(a0) * largo), Vector3Scale(l, std::sin(a0) * ancho)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(eje, std::cos(a1) * largo), Vector3Scale(l, std::sin(a1) * ancho)));
        VAG(c, centro); VAG(p0, borde); VAG(p1, borde);
    }
}


static void DibujarMarcasSueloAG(const MinijuegoAutosGlobo& m, int limite, bool sombras)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    LoteAG();

    // Placas de carga.
    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        const PlacaAutosGlobo& placa = m.placas[p];

        if (!placa.activa) continue;

        Vector3 c = { placa.x, 0.0f, placa.z };
        float edad = m.visual.placaEdad[p];

        if (placa.enAviso)
        {
            // Aviso: anillo ambar que se carga y parpadea.
            float progreso = Lim01AG(1.0f - placa.tiempo / AVISO_PLACA_GLOBO);
            bool encendido = std::fmod(t * 5.0f, 1.0f) < 0.6f;
            Color ambar = AlfaAG(AVISO_AG, encendido ? 0.95f : 0.45f);
            AnilloAG(c, RADIO_PLACA_GLOBO - 0.14f, RADIO_PLACA_GLOBO, 36, ambar, ambar);
            AnilloAG(c, RADIO_PLACA_GLOBO - 0.4f, RADIO_PLACA_GLOBO - 0.2f, 36, AVISO_AG, AVISO_AG, -0.5f * PI, -0.5f * PI + 2.0f * PI * progreso);

            for (int k = 0; k < 10; k++)
            {
                float a0 = t * 1.5f + (float)k / 10.0f * 2.0f * PI;
                AnilloAG(c, RADIO_PLACA_GLOBO + 0.08f, RADIO_PLACA_GLOBO + 0.2f, 3, AlfaAG(AVISO_AG, 0.7f), AlfaAG(AVISO_AG, 0.7f), a0, a0 + 0.35f);
            }

            continue;
        }

        // Encendida: disco menta con anillo blanco y chevrones que giran.
        float entra = Lim01AG(edad / 0.3f);
        float r = RADIO_PLACA_GLOBO * (0.6f + 0.4f * entra);
        bool poco = placa.tiempo < 3.0f && std::fmod(t * 6.0f, 1.0f) < 0.5f;
        float pulso = 0.5f + 0.5f * std::sin(t * 5.0f);
        Color centro = MezclaAG(CARGA_AG, Color{ 230, 255, 246, 255 }, 0.35f + 0.25f * pulso);
        Color borde = poco ? TonoAG(CARGA_AG, 0.6f) : CARGA_AG;
        DiscoAG(c, r, r, 32, centro, borde, false);
        AnilloAG(c, r, r + 0.1f, 32, Color{ 250, 255, 252, 255 }, Color{ 250, 255, 252, 255 });

        for (int k = 0; k < 3; k++)
        {
            float a0 = -t * 2.0f + (float)k / 3.0f * 2.0f * PI;
            AnilloAG(c, r * 0.55f, r * 0.72f, 6, Color{ 255, 255, 255, 220 }, Color{ 255, 255, 255, 220 }, a0, a0 + 1.2f);
        }

        // Globo dibujado en el centro.
        Color dibujo = { 40, 120, 104, 200 };
        DiscoAG({ c.x, 0.0f, c.z - 0.06f }, 0.2f, 0.24f, 14, dibujo, dibujo, false);
        TriAG({ c.x - 0.05f, 0.0f, c.z + 0.2f }, { c.x + 0.05f, 0.0f, c.z + 0.2f }, { c.x, 0.0f, c.z + 0.16f }, dibujo);
    }

    // Placas tomadas: onda que se abre.
    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        float v = m.visual.placaToma[p];

        if (v <= 0.0f) continue;

        float r = RADIO_PLACA_GLOBO + (1.0f - v) * 2.2f;
        AnilloAG({ m.visual.placaTomaX[p], 0.0f, m.visual.placaTomaZ[p] }, r - 0.18f, r, 36, AlfaAG(CARGA_AG, v), AlfaAG(CARGA_AG, 0.0f));
    }

    // Sombras de los autos (debajo, pegadas al suelo).
    if (sombras)
    {
        for (int i = 0; i < limite; i++)
        {
            const AutoGlobo& a = m.autos[i];

            if (!m.resultado.participantes[i].participo || !a.vivo) continue;

            Vector3 f = { std::cos(a.angulo), 0.0f, std::sin(a.angulo) };
            ElipseSueloAG({ a.x - 0.12f, 0.0f, a.z + 0.12f }, f, 1.15f, 0.72f, Color{ 16, 24, 44, 110 }, Color{ 16, 24, 44, 0 });
        }
    }

    FinLoteAG();
    rlDrawRenderBatchActive();
    rlEnableDepthTest();
}


//--------------------------------------------------
// Autos flotantes, globos y pilotos
//--------------------------------------------------

struct MarcoAG
{
    Vector3 o;      // origen (centro del auto a ras del suelo)
    Vector3 f;      // frente
    Vector3 u;      // arriba
    Vector3 s;      // costado
};


static Vector3 LocalAG(const MarcoAG& m, float lx, float ly, float lz)
{
    return Vector3Add(m.o, Vector3Add(Vector3Scale(m.f, lx), Vector3Add(Vector3Scale(m.u, ly), Vector3Scale(m.s, lz))));
}


static void CajaLocalAG(const MarcoAG& m, float lx, float ly, float lz, float hx, float hy, float hz, Color c)
{
    CajaEjesAG(LocalAG(m, lx, ly, lz), Vector3Scale(m.f, hx), Vector3Scale(m.u, hy), Vector3Scale(m.s, hz), c, true);
}


static float AlturaFlotacionAG(int indice, float t)
{
    return 0.26f + 0.05f * std::sin(t * 3.2f + (float)indice * 1.7f);
}


static MarcoAG MarcoAutoAG(const AutoGlobo& a, int indice, float t)
{
    MarcoAG m;
    m.o = { a.x, AlturaFlotacionAG(indice, t), a.z };
    m.f = { std::cos(a.angulo), 0.0f, std::sin(a.angulo) };
    m.u = { 0.0f, 1.0f, 0.0f };
    m.s = { -m.f.z, 0.0f, m.f.x };

    // Se inclina un poco al girar y al acelerar.
    float lateral = a.vx * m.s.x + a.vz * m.s.z;
    float roll = Lim01AG(std::fabs(lateral) / 6.0f) * 0.18f * (lateral > 0.0f ? 1.0f : -1.0f);
    m.u = Vector3Normalize(Vector3Add(m.u, Vector3Scale(m.s, -roll)));
    m.s = Vector3Normalize(Vector3CrossProduct(m.f, m.u));
    m.s = Vector3Negate(m.s);
    return m;
}


// Carroceria (sin el parabrisas, que es translucido).
static void CarroceriaAG(const MarcoAG& m, Color color, bool turbo, float t)
{
    Color casco = { 70, 80, 100, 255 };
    Color claro = MezclaAG(color, WHITE, 0.25f);

    CajaLocalAG(m, -0.02f, 0.1f, 0.0f, 0.96f, 0.1f, 0.54f, casco);
    CajaLocalAG(m, -0.06f, 0.3f, 0.0f, 0.9f, 0.11f, 0.6f, color);
    CajaLocalAG(m, 0.6f, 0.44f, 0.0f, 0.3f, 0.05f, 0.5f, claro);
    CajaLocalAG(m, -0.62f, 0.45f, 0.0f, 0.26f, 0.06f, 0.56f, claro);
    CajaLocalAG(m, -0.1f, 0.42f, 0.0f, 0.36f, 0.03f, 0.42f, Color{ 46, 54, 70, 255 });

    // Paragolpes de embestida (el frente es lo que revienta globos).
    CajaLocalAG(m, 0.96f, 0.24f, 0.0f, 0.1f, 0.14f, 0.62f, BLANCO_AG);
    g_emisAG = 1.0f;
    CajaLocalAG(m, 1.065f, 0.27f, 0.0f, 0.012f, 0.035f, 0.5f, ENERGIA_AG);
    CajaLocalAG(m, 1.0f, 0.34f, 0.44f, 0.07f, 0.04f, 0.1f, Color{ 255, 252, 230, 255 });
    CajaLocalAG(m, 1.0f, 0.34f, -0.44f, 0.07f, 0.04f, 0.1f, Color{ 255, 252, 230, 255 });
    g_emisAG = 0.0f;

    // Motores laterales traseros.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float z = 0.56f * (float)lado;
        CilAG(LocalAG(m, -1.02f, 0.3f, z), LocalAG(m, -0.3f, 0.3f, z), 0.15f, 0.17f, 8, BLANCO_AG);
        g_emisAG = 1.0f;
        CilAG(LocalAG(m, -1.06f, 0.3f, z), LocalAG(m, -1.01f, 0.3f, z), 0.11f, 0.11f, 8, turbo ? Color{ 255, 220, 150, 255 } : ENERGIA_AG);
        g_emisAG = 0.0f;
    }

    // Aleta y mastil de los globos.
    CajaLocalAG(m, -0.82f, 0.62f, 0.0f, 0.16f, 0.14f, 0.03f, color);
    CilAG(LocalAG(m, -0.86f, 0.7f, 0.0f), LocalAG(m, -0.9f, 1.3f, 0.0f), 0.025f, 0.02f, 5, METAL_AG, false);
    BolaAG(LocalAG(m, -0.9f, 1.32f, 0.0f), { 0.05f, 0.05f, 0.05f }, BLANCO_AG, 2, 5);
    (void)t;
}


static void GloboAG(Vector3 p, Color color, float escala)
{
    float r = 0.4f * escala;
    BolaAG(p, { r, r * 1.16f, r }, color, 5, 10);
    CilAG({ p.x, p.y - r * 1.12f, p.z }, { p.x, p.y - r * 1.32f, p.z }, 0.03f, 0.07f, 5, TonoAG(color, 0.8f));
    // Brillo hacia el sol y la camara.
    Vector3 b = Vector3Add(p, Vector3Add(Vector3Scale(g_derAG, r * 0.38f), Vector3Scale(g_arribaAG, r * 0.48f)));
    b = Vector3Add(b, Vector3Scale(Vector3Normalize(Vector3Subtract(g_ojoAG, p)), r * 0.8f));
    VAG(b, WHITE);
    VAG(Vector3Add(b, Vector3Scale(g_arribaAG, r * 0.22f)), Color{ 255, 255, 255, 200 });
    VAG(Vector3Add(b, Vector3Scale(g_derAG, r * 0.16f)), Color{ 255, 255, 255, 200 });
}


static void DibujarAutosAG(const MinijuegoAutosGlobo& m, const Participante participantes[], int limite)
{
    float t = m.tiempoAnimacion;

    // Carrocerias y globos en un lote.
    LoteAG();

    for (int i = 0; i < limite; i++)
    {
        const AutoGlobo& a = m.autos[i];

        if (!m.resultado.participantes[i].participo || !a.vivo) continue;

        bool parpadeo = a.inmunidad > 0.0f && std::fmod(t * 12.0f, 1.0f) < 0.5f;
        Color color = participantes[i].color;
        Color cuerpo = parpadeo ? MezclaAG(color, WHITE, 0.7f) : color;
        MarcoAG marco = MarcoAutoAG(a, i, t);
        CarroceriaAG(marco, cuerpo, a.turboActivo > 0.0f, t);

        Vector3 mastil = LocalAG(marco, -0.9f, 1.3f, 0.0f);
        float escalaGlobo = 1.0f + 0.25f * Lim01AG(m.visual.recarga[i]);

        for (int k = 0; k < a.globos; k++)
        {
            Vector3 g = PosicionGloboVisual(a, k, t, i);
            Color cg = parpadeo ? MezclaAG(color, WHITE, 0.6f) : MezclaAG(color, WHITE, 0.08f);
            TiraAG(mastil, { g.x, g.y - 0.42f, g.z }, 0.025f, 0.02f, Color{ 236, 240, 246, 255 }, Color{ 236, 240, 246, 255 });
            GloboAG(g, cg, (k == a.globos - 1) ? escalaGlobo : 1.0f);
        }
    }

    // Autos que se quedaron sin globos: salen despedidos y caen al vacio.
    for (int i = 0; i < limite; i++)
    {
        float c = m.visual.caida[i];

        if (!m.resultado.participantes[i].participo || m.autos[i].vivo || c < 0.0f || c > 2.6f) continue;

        MarcoAG marco;
        float dx = m.visual.caidaDirX[i], dz = m.visual.caidaDirZ[i];
        marco.o = { m.visual.caidaX[i] + dx * c * 6.0f, 0.3f + 4.2f * c - 6.5f * c * c, m.visual.caidaZ[i] + dz * c * 6.0f };
        float giro = m.visual.caidaAngulo[i] + c * 5.0f;
        float vuelco = c * 6.5f;
        Vector3 f0 = { std::cos(giro), 0.0f, std::sin(giro) };
        Vector3 s0 = { -f0.z, 0.0f, f0.x };
        marco.f = Vector3Add(Vector3Scale(f0, std::cos(vuelco)), Vector3Scale({ 0.0f, 1.0f, 0.0f }, std::sin(vuelco)));
        marco.u = Vector3Add(Vector3Scale(f0, -std::sin(vuelco)), Vector3Scale({ 0.0f, 1.0f, 0.0f }, std::cos(vuelco)));
        marco.s = s0;
        Color gris = MezclaAG(participantes[i].color, Color{ 90, 96, 108, 255 }, 0.45f);
        CarroceriaAG(marco, gris, false, t);
    }

    FinLoteAG();

    // Pilotos con el modelo compartido, sentados en la cabina.
    for (int i = 0; i < limite; i++)
    {
        const AutoGlobo& a = m.autos[i];

        if (!m.resultado.participantes[i].participo) continue;

        if (a.vivo)
        {
            MarcoAG marco = MarcoAutoAG(a, i, t);
            // Pies hundidos en la cabina: se lee sentado.
            Vector3 asiento = LocalAG(marco, -0.14f, 0.1f, 0.0f);
            float angulo = std::atan2(marco.f.x, marco.f.z) * RAD2DEG;
            DibujarModeloJugadorEnPosicion(asiento, angulo, participantes[i].color, ESCALA_MODELO_JUGADOR_3D * 0.52f);
        }
        else if (m.visual.caida[i] >= 0.0f && m.visual.caida[i] < 2.6f)
        {
            float c = m.visual.caida[i];
            float dx = m.visual.caidaDirX[i], dz = m.visual.caidaDirZ[i];
            Vector3 p = { m.visual.caidaX[i] + dx * c * 6.0f, 0.7f + 4.6f * c - 6.5f * c * c, m.visual.caidaZ[i] + dz * c * 6.0f };
            rlPushMatrix();
            rlTranslatef(p.x, p.y, p.z);
            rlRotatef(c * 300.0f, -dz, 0.0f, dx);
            DibujarModeloJugadorEnPosicion({ 0.0f, -0.4f, 0.0f }, c * 200.0f, participantes[i].color, ESCALA_MODELO_JUGADOR_3D * 0.52f);
            rlPopMatrix();
        }
    }
}


// Parabrisas, escudo de inmunidad y llamas: translucidos, despues de lo opaco.
static void DibujarAutosTranslucidoAG(const MinijuegoAutosGlobo& m, int limite, bool efectos)
{
    float t = m.tiempoAnimacion;

    LoteAG();

    for (int i = 0; i < limite; i++)
    {
        const AutoGlobo& a = m.autos[i];

        if (!m.resultado.participantes[i].participo || !a.vivo) continue;

        MarcoAG marco = MarcoAutoAG(a, i, t);
        Color vidrio = { 170, 226, 250, 120 };
        QuadColAG(LocalAG(marco, 0.3f, 0.47f, -0.46f), LocalAG(marco, 0.3f, 0.47f, 0.46f), LocalAG(marco, 0.16f, 0.82f, 0.4f), LocalAG(marco, 0.16f, 0.82f, -0.4f), vidrio, vidrio, AlfaAG(vidrio, 0.6f), AlfaAG(vidrio, 0.6f));

        if (a.inmunidad > 0.0f)
        {
            // Escudo que se desvanece mientras dura la inmunidad.
            float alfa = 0.1f + 0.12f * Lim01AG(a.inmunidad);
            HaloAG(LocalAG(marco, -0.1f, 0.9f, 0.0f), 1.55f, AlfaAG(Color{ 255, 255, 255, 255 }, alfa * 0.4f), AlfaAG(Color{ 220, 246, 255, 255 }, alfa * 1.6f), 20);
        }
    }

    FinLoteAG();

    if (!efectos) return;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteAG();

    for (int i = 0; i < limite; i++)
    {
        const AutoGlobo& a = m.autos[i];

        if (!m.resultado.participantes[i].participo || !a.vivo) continue;

        MarcoAG marco = MarcoAutoAG(a, i, t);

        // Discos de levitacion y charco de luz bajo el auto.
        DiscoAG({ a.x, 0.02f, a.z }, 1.5f, 1.5f, 18, AlfaAG(ENERGIA_AG, 0.16f), AlfaAG(ENERGIA_AG, 0.0f), false);

        for (int ex = -1; ex <= 1; ex += 2)
        {
            for (int ez = -1; ez <= 1; ez += 2)
            {
                Vector3 p = LocalAG(marco, 0.62f * (float)ex, -0.01f, 0.42f * (float)ez);
                p.y = 0.04f;
                DiscoAG(p, 0.3f, 0.3f, 10, AlfaAG(ENERGIA_AG, 0.45f), AlfaAG(ENERGIA_AG, 0.0f), false);
            }
        }

        // Llamas del turbo.
        if (a.turboActivo > 0.0f)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                float largo = 0.9f + 0.35f * std::sin(t * 47.0f + (float)lado);
                Vector3 b = LocalAG(marco, -1.06f, 0.3f, 0.56f * (float)lado);
                Vector3 e = LocalAG(marco, -1.06f - largo, 0.3f, 0.56f * (float)lado);
                TiraAG(b, e, 0.34f, 0.02f, AlfaAG(TURBO_AG, 0.9f), AlfaAG(Color{ 255, 90, 40, 255 }, 0.0f));
                TiraAG(b, LocalAG(marco, -1.06f - largo * 0.55f, 0.3f, 0.56f * (float)lado), 0.18f, 0.02f, Color{ 255, 250, 210, 230 }, Color{ 255, 230, 160, 0 });
            }
        }

        // Destello del arranque del turbo.
        float d = m.visual.turboDestello[i];

        if (d > 0.0f)
        {
            Vector3 c = { a.x, 0.05f, a.z };
            float r = 1.0f + (1.0f - d) * 1.6f;
            AnilloAG(c, r - 0.25f, r, 24, AlfaAG(TURBO_AG, 0.0f), AlfaAG(TURBO_AG, 0.7f * d));
        }

        // Halo suave de cada globo.
        for (int k = 0; k < a.globos; k++)
        {
            HaloAG(PosicionGloboVisual(a, k, t, i), 0.62f, AlfaAG(WHITE, 0.08f), AlfaAG(WHITE, 0.0f), 10);
        }
    }

    FinLoteAG();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Placas encendidas: globo holografico y haz (3D)
//--------------------------------------------------

static void DibujarPlacasAG(const MinijuegoAutosGlobo& m)
{
    float t = m.tiempoAnimacion;
    LoteAG();

    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        const PlacaAutosGlobo& placa = m.placas[p];

        if (!placa.activa || placa.enAviso) continue;

        float entra = Lim01AG(m.visual.placaEdad[p] / 0.35f);
        float sube = 0.3f + 0.75f * entra;
        Vector3 g = { placa.x, sube + 0.12f * std::sin(t * 3.0f + (float)p), placa.z };
        GloboAG(g, CARGA_AG, 0.85f * entra);
        CilAG({ placa.x, 0.0f, placa.z }, { placa.x, 0.08f, placa.z }, 0.16f, 0.12f, 8, BLANCO_AG);
    }

    FinLoteAG();
}


//--------------------------------------------------
// Vida cercana: drones, banderas y petalos
//--------------------------------------------------

static void DibujarAnimadoCercaAG(const MinijuegoAutosGlobo& m, bool decoracion)
{
    float t = m.tiempoAnimacion;
    LoteAG();

    // Banderas de la ciudad que ondean.
    if (decoracion)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = (float)lado * 12.8f;
            const int tiras = 8;

            for (int k = 0; k < tiras; k++)
            {
                float u0 = (float)k / (float)tiras, u1 = (float)(k + 1) / (float)tiras;
                float o0 = std::sin(t * 4.0f - u0 * 5.0f + (float)lado) * 0.16f * u0;
                float o1 = std::sin(t * 4.0f - u1 * 5.0f + (float)lado) * 0.16f * u1;
                float dx = -(float)lado;
                Vector3 a0 = { x + dx * u0 * 1.3f, 4.1f + o0 * 0.5f, 6.3f + o0 };
                Vector3 a1 = { x + dx * u1 * 1.3f, 4.1f + o1 * 0.5f, 6.3f + o1 };
                Color c0 = (k % 4 < 2) ? Color{ 80, 200, 220, 255 } : BLANCO_AG;
                float sombra = 0.85f + 0.15f * std::cos(t * 4.0f - u0 * 5.0f);
                QuadAG({ a0.x, a0.y - 0.75f, a0.z }, { a1.x, a1.y - 0.75f, a1.z }, a1, a0, TonoAG(c0, sombra));
            }
        }
    }

    // Drones que patrullan sobre el paseo (no tapan la arena).
    int drones = decoracion ? 3 : 0;

    for (int d = 0; d < drones; d++)
    {
        // Van y vienen por el arco del fondo y los costados: al frente
        // taparian a los autos.
        float a = PI + PI * (0.5f + 0.5f * std::sin(t * (0.16f + 0.04f * (float)d) + (float)d * 2.1f));
        Vector3 c = { std::cos(a) * 12.4f, 5.0f + 0.6f * (float)d + 0.15f * std::sin(t * 2.0f + (float)d), std::sin(a) * 9.0f };

        CajaAG(c, { 0.5f, 0.14f, 0.5f }, BLANCO_AG);
        CajaAG({ c.x, c.y - 0.12f, c.z }, { 0.2f, 0.1f, 0.2f }, METAL_OSCURO_AG);

        for (int ex = -1; ex <= 1; ex += 2)
        {
            for (int ez = -1; ez <= 1; ez += 2)
            {
                Vector3 r = { c.x + 0.38f * (float)ex, c.y + 0.08f, c.z + 0.38f * (float)ez };
                VigaAG(c, r, 0.05f, 0.04f, METAL_AG);
                // Helice que gira (aspa) sobre su disco borroso.
                float giro = t * 40.0f + (float)(ex * 2 + ez);
                Vector3 aspa = { std::cos(giro) * 0.22f, 0.0f, std::sin(giro) * 0.22f };
                Vector3 ancho = { -aspa.z * 0.12f, 0.0f, aspa.x * 0.12f };
                Vector3 rc = { r.x, r.y + 0.04f, r.z };
                DiscoAG(rc, 0.22f, 0.22f, 8, Color{ 200, 210, 222, 90 }, Color{ 200, 210, 222, 30 }, false);
                QuadColAG(Vector3Subtract(Vector3Subtract(rc, aspa), ancho), Vector3Add(Vector3Subtract(rc, aspa), ancho), Vector3Add(Vector3Add(rc, aspa), ancho), Vector3Subtract(Vector3Add(rc, aspa), ancho), METAL_OSCURO_AG, METAL_OSCURO_AG, METAL_OSCURO_AG, METAL_OSCURO_AG);
            }
        }

        // Luz de posicion que titila.
        if (std::fmod(t * 1.6f + (float)d * 0.3f, 1.0f) < 0.4f)
        {
            g_emisAG = 1.0f;
            CajaAG({ c.x, c.y - 0.2f, c.z }, { 0.08f, 0.06f, 0.08f }, Color{ 255, 90, 90, 255 });
            g_emisAG = 0.0f;
        }
    }

    // Petalos que cruza el viento.
    int petalos = (int)(36.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int k = 0; k < petalos; k++)
    {
        float recorrido = 34.0f;
        float x = std::fmod(Ruido01AG(k, 91) * recorrido + t * (0.9f + 0.6f * Ruido01AG(k, 92)), recorrido) - recorrido * 0.5f;
        float z = -9.0f + 18.0f * Ruido01AG(k, 93) + 0.6f * std::sin(t * 0.7f + (float)k);
        float y = 0.5f + 4.0f * Ruido01AG(k, 94) + 0.3f * std::sin(t * 1.3f + (float)k * 2.0f);
        Color c = (k % 3 == 0) ? Color{ 255, 255, 255, 230 } : Color{ 255, 170, 205, 230 };
        PuntoAG({ x, y, z }, 0.05f, c);
    }

    FinLoteAG();
}


//--------------------------------------------------
// Barrera de energia, baranda de vidrio y luces (translucido)
//--------------------------------------------------

static void CortinaAG(Vector3 a, Vector3 b, float alto, float t, float fase, Color base)
{
    // Banda vertical con degrade y un pulso que corre a lo largo.
    const int tramos = 14;

    for (int k = 0; k < tramos; k++)
    {
        float u0 = (float)k / (float)tramos, u1 = (float)(k + 1) / (float)tramos;
        Vector3 p0 = Vector3Lerp(a, b, u0), p1 = Vector3Lerp(a, b, u1);
        float pulso0 = std::fmax(0.0f, 1.0f - std::fabs(std::fmod(u0 + fase - t * 0.25f + 10.0f, 1.0f) - 0.5f) * 6.0f);
        float pulso1 = std::fmax(0.0f, 1.0f - std::fabs(std::fmod(u1 + fase - t * 0.25f + 10.0f, 1.0f) - 0.5f) * 6.0f);
        Color abajo0 = AlfaAG(base, 0.26f + 0.3f * pulso0), abajo1 = AlfaAG(base, 0.26f + 0.3f * pulso1);
        Color arriba = AlfaAG(base, 0.02f);
        QuadColAG(p0, p1, { p1.x, p1.y + alto, p1.z }, { p0.x, p0.y + alto, p0.z }, abajo0, abajo1, arriba, arriba);
        // Linea superior brillante.
        Color linea0 = AlfaAG(WHITE, 0.5f + 0.4f * pulso0), linea1 = AlfaAG(WHITE, 0.5f + 0.4f * pulso1);
        QuadColAG({ p0.x, p0.y + alto - 0.03f, p0.z }, { p1.x, p1.y + alto - 0.03f, p1.z }, { p1.x, p1.y + alto + 0.03f, p1.z }, { p0.x, p0.y + alto + 0.03f, p0.z }, linea0, linea1, linea1, linea0);
    }
}


static void DibujarBarrerasAG(const MinijuegoAutosGlobo& m, int limite)
{
    float t = m.tiempoAnimacion;
    float x = BX_AG + 0.2f, z = BZ_AG + 0.2f;
    float y = 0.33f;

    LoteAG();
    CortinaAG({ -x, y, -z }, { x, y, -z }, AltoBarreraAG(-BZ_AG) - 0.05f, t, 0.0f, ENERGIA_AG);
    CortinaAG({ x, y, -z }, { x, y, z }, AltoBarreraAG(0.0f) - 0.05f, t, 0.5f, ENERGIA_AG);
    CortinaAG({ -x, y, z }, { -x, y, -z }, AltoBarreraAG(0.0f) - 0.05f, t, 0.25f, ENERGIA_AG);
    CortinaAG({ x, y, z }, { -x, y, z }, AltoBarreraAG(BZ_AG) - 0.05f, t, 0.75f, ENERGIA_AG);

    // Onda donde un auto acaba de rebotar fuerte.
    for (int i = 0; i < limite; i++)
    {
        const AutoGlobo& a = m.autos[i];

        if (!m.resultado.participantes[i].participo || !a.vivo || a.enfriamientoBarrera <= 0.0f) continue;

        float v = a.enfriamientoBarrera / 0.35f;
        float dxBorde = LIMITE_X_GLOBO - std::fabs(a.x);
        float dzBorde = LIMITE_Z_GLOBO - std::fabs(a.z);
        Vector3 c;
        Vector3 lado;

        if (dxBorde < dzBorde) { c = { a.x > 0.0f ? x : -x, 0.75f, a.z }; lado = { 0.0f, 0.0f, 1.0f }; }
        else { c = { a.x, 0.75f, a.z > 0.0f ? z : -z }; lado = { 1.0f, 0.0f, 0.0f }; }

        float r = 0.4f + (1.0f - v) * 1.2f;
        Color onda = AlfaAG(WHITE, 0.7f * v);

        for (int k = 0; k < 12; k++)
        {
            float a0 = (float)k / 12.0f * 2.0f * PI, a1 = (float)(k + 1) / 12.0f * 2.0f * PI;
            Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(lado, std::cos(a0) * r), { 0.0f, std::sin(a0) * r * 0.45f, 0.0f }));
            Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(lado, std::cos(a1) * r), { 0.0f, std::sin(a1) * r * 0.45f, 0.0f }));
            VAG(c, AlfaAG(ENERGIA_AG, 0.35f * v)); VAG(p0, onda); VAG(p1, onda);
        }
    }

    // Vidrio de la baranda al borde de la caida.
    Color vidrio = { 190, 226, 248, 60 };
    Color vidrioArriba = { 230, 246, 255, 90 };
    float px = PX_AG - 0.15f, pz = PZ_AG - 0.15f;
    QuadColAG({ -px, 0.05f, -pz }, { px, 0.05f, -pz }, { px, 1.06f, -pz }, { -px, 1.06f, -pz }, vidrio, vidrio, vidrioArriba, vidrioArriba);
    QuadColAG({ -px, 0.05f, -pz }, { -px, 0.05f, PZ_AG }, { -px, 1.06f, PZ_AG }, { -px, 1.06f, -pz }, vidrio, vidrio, vidrioArriba, vidrioArriba);
    QuadColAG({ px, 0.05f, -pz }, { px, 0.05f, PZ_AG }, { px, 1.06f, PZ_AG }, { px, 1.06f, -pz }, vidrio, vidrio, vidrioArriba, vidrioArriba);
    FinLoteAG();
}


// Luces falsas aditivas: faroles, holograma, placas y carteles de las torres.
static void DibujarLucesAG(const MinijuegoAutosGlobo& m, bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteAG();

    // Brillo de la barrera sobre el suelo, hacia adentro.
    Color e0 = AlfaAG(ENERGIA_AG, 0.12f), e1 = AlfaAG(ENERGIA_AG, 0.0f);
    float y = 0.015f;
    QuadColAG({ -BX_AG, y, -BZ_AG }, { BX_AG, y, -BZ_AG }, { BX_AG, y, -BZ_AG + 0.7f }, { -BX_AG, y, -BZ_AG + 0.7f }, e0, e0, e1, e1);
    QuadColAG({ -BX_AG, y, BZ_AG }, { BX_AG, y, BZ_AG }, { BX_AG, y, BZ_AG - 0.5f }, { -BX_AG, y, BZ_AG - 0.5f }, e0, e0, e1, e1);
    QuadColAG({ -BX_AG, y, -BZ_AG }, { -BX_AG, y, BZ_AG }, { -BX_AG + 0.6f, y, BZ_AG }, { -BX_AG + 0.6f, y, -BZ_AG }, e0, e0, e1, e1);
    QuadColAG({ BX_AG, y, -BZ_AG }, { BX_AG, y, BZ_AG }, { BX_AG - 0.6f, y, BZ_AG }, { BX_AG - 0.6f, y, -BZ_AG }, e0, e0, e1, e1);

    // Placas: charco de luz y haz que sube.
    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        const PlacaAutosGlobo& placa = m.placas[p];

        if (!placa.activa) continue;

        Vector3 c = { placa.x, 0.02f, placa.z };

        if (placa.enAviso)
        {
            DiscoAG(c, 1.8f, 1.8f, 20, AlfaAG(AVISO_AG, 0.12f), AlfaAG(AVISO_AG, 0.0f), false);
            continue;
        }

        float pulso = 0.5f + 0.5f * std::sin(t * 5.0f);
        DiscoAG(c, 2.4f, 2.4f, 24, AlfaAG(CARGA_AG, 0.2f + 0.08f * pulso), AlfaAG(CARGA_AG, 0.0f), false);
        TiraAG({ placa.x, 0.0f, placa.z }, { placa.x, 3.2f, placa.z }, 1.6f, 0.9f, AlfaAG(CARGA_AG, 0.22f), AlfaAG(CARGA_AG, 0.0f));
        TiraAG({ placa.x, 0.0f, placa.z }, { placa.x, 2.2f, placa.z }, 0.6f, 0.3f, AlfaAG(Color{ 220, 255, 240, 255 }, 0.25f), AlfaAG(CARGA_AG, 0.0f));
    }

    if (decoracion)
    {
        // Faroles encendidos (de dia, tenues): halo y charco.
        for (const FarolAG& f : FAROLES_AG)
        {
            HaloAG({ f.x, 3.05f, f.z }, 0.9f, AlfaAG(Color{ 255, 244, 210, 255 }, 0.3f), AlfaAG(Color{ 255, 244, 210, 255 }, 0.0f));
            DiscoAG({ f.x, 0.02f, f.z }, 1.6f, 1.6f, 18, AlfaAG(Color{ 255, 240, 200, 255 }, 0.14f), AlfaAG(Color{ 255, 240, 200, 255 }, 0.0f), false);
        }

        // Holograma del pilon: globo de la plaza que gira sobre anillos.
        Vector3 h = { 0.0f, 2.6f + 0.12f * std::sin(t * 1.5f), -8.4f };
        Color holo = { 110, 230, 255, 255 };
        TiraAG({ 0.0f, 0.55f, -8.4f }, h, 0.7f, 1.6f, AlfaAG(holo, 0.22f), AlfaAG(holo, 0.02f));

        for (int k = 0; k < 3; k++)
        {
            float r = 0.7f + 0.25f * (float)k;
            float yy = 1.0f + 0.5f * (float)k + 0.1f * std::sin(t * 2.0f + (float)k);
            AnilloAG({ 0.0f, yy, -8.4f }, r - 0.05f, r, 24, AlfaAG(holo, 0.45f), AlfaAG(holo, 0.45f), t * (1.0f + 0.4f * (float)k), t * (1.0f + 0.4f * (float)k) + 1.6f * PI);
        }

        float ancho = std::fabs(std::cos(t * 1.2f));
        for (int k = 0; k < 16; k++)
        {
            float a0 = (float)k / 16.0f * 2.0f * PI, a1 = (float)(k + 1) / 16.0f * 2.0f * PI;
            Vector3 p0 = { h.x + std::cos(a0) * 0.75f * ancho, h.y + std::sin(a0) * 0.88f, h.z };
            Vector3 p1 = { h.x + std::cos(a1) * 0.75f * ancho, h.y + std::sin(a1) * 0.88f, h.z };
            VAG(h, AlfaAG(holo, 0.18f)); VAG(p0, AlfaAG(holo, 0.4f)); VAG(p1, AlfaAG(holo, 0.4f));
        }

        // Lineas de barrido del holograma.
        for (int k = 0; k < 5; k++)
        {
            float yy = h.y - 0.9f + std::fmod(t * 0.8f + (float)k * 0.36f, 1.8f);
            QuadColAG({ -0.9f, yy, -8.4f }, { 0.9f, yy, -8.4f }, { 0.9f, yy + 0.03f, -8.4f }, { -0.9f, yy + 0.03f, -8.4f }, AlfaAG(holo, 0.2f), AlfaAG(holo, 0.2f), AlfaAG(holo, 0.2f), AlfaAG(holo, 0.2f));
        }

        // Carteles holograficos en las torres de atras.
        const Vector3 carteles[3] = { { -21.5f, 6.0f, -18.5f }, { 23.0f, 9.0f, -21.0f }, { 3.0f, 15.0f, -27.0f } };
        const Color colores[3] = { { 255, 120, 210, 255 }, { 110, 230, 255, 255 }, { 255, 200, 110, 255 } };

        for (int k = 0; k < 3; k++)
        {
            Vector3 c = carteles[k];
            float an = 4.0f, al = 2.2f;
            Color col = colores[k];
            float parpadeo = 0.8f + 0.2f * std::sin(t * 9.0f + (float)k * 3.0f);
            QuadColAG({ c.x - an, c.y - al, c.z }, { c.x + an, c.y - al, c.z }, { c.x + an, c.y + al, c.z }, { c.x - an, c.y + al, c.z }, AlfaAG(col, 0.16f * parpadeo), AlfaAG(col, 0.16f * parpadeo), AlfaAG(col, 0.28f * parpadeo), AlfaAG(col, 0.28f * parpadeo));

            for (int b = 0; b < 4; b++)
            {
                float yy = c.y - al + std::fmod(t * 0.9f + (float)b * 0.55f + (float)k, al * 2.0f);
                QuadColAG({ c.x - an, yy, c.z + 0.02f }, { c.x + an, yy, c.z + 0.02f }, { c.x + an, yy + 0.18f, c.z + 0.02f }, { c.x - an, yy + 0.18f, c.z + 0.02f }, AlfaAG(col, 0.3f), AlfaAG(col, 0.3f), AlfaAG(col, 0.3f), AlfaAG(col, 0.3f));
            }

            // Globo dibujado en el cartel.
            HaloAG({ c.x - an * 0.5f, c.y + 0.2f, c.z + 0.03f }, 1.0f, AlfaAG(WHITE, 0.35f), AlfaAG(col, 0.2f), 14);
        }
    }

    FinLoteAG();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Efectos: particulas de la logica, reventones y confeti
//--------------------------------------------------

static void DibujarEfectosAG(const MinijuegoAutosGlobo& m, const Participante participantes[], int limite)
{
    float t = m.tiempoAnimacion;
    LoteAG();

    for (int k = 0; k < MAX_PARTICULAS_AUTOS_GLOBO; k++)
    {
        const ParticulaTierra& p = m.particulas[k];

        if (!p.activa || p.vidaMaxima <= 0.0f) continue;

        float vida = Lim01AG(p.vida / p.vidaMaxima);
        PuntoAG(p.posicion, p.tamano * (0.6f + 0.6f * vida), AlfaAG(p.color, 0.4f + 0.6f * vida));
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        // Reventon: jirones de goma del color del globo.
        float v = m.visual.reventon[i];

        if (v > 0.0f)
        {
            Vector3 c = { m.visual.reventonX[i], m.visual.reventonY[i], m.visual.reventonZ[i] };
            float abre = (1.0f - v) * 1.4f;
            Color color = participantes[i].color;

            for (int k = 0; k < 10; k++)
            {
                float a = (float)k / 10.0f * 2.0f * PI + (float)i;
                float e = -0.4f + 0.8f * Ruido01AG(k, 95 + i);
                Vector3 d = Vector3Add(Vector3Scale(g_derAG, std::cos(a)), Vector3Scale(g_arribaAG, std::sin(a) + e * 0.3f));
                Vector3 p = Vector3Add(c, Vector3Scale(d, abre));
                p.y -= (1.0f - v) * (1.0f - v) * 0.8f;
                PuntoAG(p, 0.09f * (0.5f + v), AlfaAG(color, v + 0.2f));
            }
        }

        // Humo del auto que cae.
        float c = m.visual.caida[i];

        if (!m.autos[i].vivo && c >= 0.0f && c < 2.6f)
        {
            float dx = m.visual.caidaDirX[i], dz = m.visual.caidaDirZ[i];

            for (int k = 0; k < 8; k++)
            {
                float ck = c - (float)k * 0.07f;
                if (ck < 0.0f) break;
                Vector3 p = { m.visual.caidaX[i] + dx * ck * 6.0f, 0.5f + 4.2f * ck - 6.5f * ck * ck, m.visual.caidaZ[i] + dz * ck * 6.0f };
                PuntoAG(p, 0.2f + 0.06f * (float)k, Color{ 120, 126, 138, (unsigned char)(170 - k * 18) });
            }
        }
    }

    // Confeti sobre el ganador.
    if (m.visual.tiempoFin >= 0.0f)
    {
        int ganadores[MAX_PARTICIPANTES]{};
        int cantidad = ObtenerIndicesGanadores(m.resultado, ganadores, MAX_PARTICIPANTES);
        const Color confeti[5] = { { 255, 90, 120, 255 }, { 255, 214, 70, 255 }, { 90, 220, 255, 255 }, { 140, 240, 140, 255 }, { 255, 255, 255, 255 } };
        int piezas = (int)(70.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int g = 0; g < cantidad; g++)
        {
            const AutoGlobo& a = m.autos[ganadores[g]];

            for (int k = 0; k < piezas; k++)
            {
                float ciclo = 2.6f;
                float u = std::fmod(m.visual.tiempoFin + Ruido01AG(k, 96) * ciclo, ciclo);
                float ang = Ruido01AG(k, 97) * 2.0f * PI;
                float r = 0.4f + 2.2f * Ruido01AG(k, 98);
                Vector3 p = { a.x + std::cos(ang) * r + 0.3f * std::sin(t * 3.0f + (float)k), 5.0f - u * 2.0f, a.z + std::sin(ang) * r };
                PuntoAG(p, 0.07f, confeti[k % 5]);
            }
        }
    }

    FinLoteAG();
}


//==================================================
// VISUAL: ANUNCIOS Y HUD
//==================================================

static void DibujarAnunciosAG(
    const MinijuegoAutosGlobo& m,
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    float t = m.tiempoAnimacion;

    // Placas: aviso con cuenta y «+1 GLOBO» sobre las encendidas.
    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        const PlacaAutosGlobo& placa = m.placas[p];

        if (!placa.activa) continue;

        if (placa.enAviso)
        {
            Vector2 q = GetWorldToScreen({ placa.x, 1.2f, placa.z }, vista);
            int segundos = (int)std::ceil(placa.tiempo > 0.0f ? placa.tiempo : 0.0f);
            DibujarTextoMundoContornoUI(TextFormat("¡CARGA EN %d!", segundos < 1 ? 1 : segundos), q.x, q.y, UI_TEXTO, AVISO_AG);
        }
        else
        {
            Vector2 q = GetWorldToScreen({ placa.x, 2.05f, placa.z }, vista);
            float alfa = placa.tiempo < 3.0f && std::fmod(t * 6.0f, 1.0f) < 0.5f ? 0.5f : 1.0f;
            DibujarTextoMundoContornoUI("+1 GLOBO", q.x, q.y, UI_TEXTO, CARGA_AG, alfa);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const AutoGlobo& a = m.autos[i];
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;

        // Cartelito J# sobre los globos.
        if (a.vivo)
        {
            Vector2 q = GetWorldToScreen({ a.x, 3.75f, a.z }, vista);
            OpcionesEtiquetaUI opciones;
            opciones.borde = participantes[i].color;
            opciones.pico = true;
            DibujarEtiquetaUI(TextFormat("J%d", numero), q, 0.0f, opciones);
        }

        // ¡PUM! donde revento el globo.
        float v = m.visual.reventon[i];

        if (v > 0.0f && a.vivo)
        {
            Vector2 q = GetWorldToScreen({ m.visual.reventonX[i], m.visual.reventonY[i] + (1.0f - v) * 0.8f, m.visual.reventonZ[i] }, vista);
            DibujarTextoMundoContornoUI("¡PUM!", q.x, q.y, UI_TEXTO_GRANDE, MezclaAG(participantes[i].color, WHITE, 0.25f), Lim01AG(v * 1.6f));
        }

        // +1 globo recuperado.
        float r = m.visual.recarga[i];

        if (r > 0.0f && a.vivo)
        {
            Vector2 q = GetWorldToScreen({ a.x, 4.3f + (1.0f - r) * 0.8f, a.z }, vista);
            DibujarTextoMundoContornoUI("+1", q.x, q.y, UI_TEXTO_GRANDE, CARGA_AG, Lim01AG(r * 1.8f));
        }

        // Eliminacion: anuncio grande en el tercio superior.
        float c = m.visual.caida[i];

        if (!a.vivo && c >= 0.0f && c < 2.0f && m.fase != FASE_AUTOS_GLOBO_PREPARACION)
        {
            float alfa = Lim01AG((2.0f - c) * 2.0f);
            float salto = c < 0.15f ? 1.0f - c / 0.15f : 0.0f;
            DibujarTextoMundoContornoUI(
                TextFormat("¡J%d SIN GLOBOS!", numero),
                (float)GetScreenWidth() * 0.5f,
                (float)GetScreenHeight() * 0.21f - salto * 10.0f * EscalaUI(),
                UI_TEXTO_GRANDE,
                MezclaAG(participantes[i].color, WHITE, 0.3f),
                alfa
            );
        }
    }
}


static void PrepararHudAG(
    const MinijuegoAutosGlobo& m,
    const Participante participantes[],
    int limite,
    DatosHudMinijuego& hud
)
{
    PrepararDatosHudMinijuego(
        hud,
        "AUTOS DE GLOBO",
        "EMBESTÍ CON EL FRENTE EL COSTADO O LA COLA DE UN RIVAL PARA REVENTARLE UN GLOBO. ¡LAS PLACAS DE CARGA TE DEVUELVEN UNO!",
        m.resultado
    );

    hud.fase = m.fase == FASE_AUTOS_GLOBO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_AUTOS_GLOBO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_GLOBO;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = m.tiempoRestante <= 0.0f;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        JugadorHudMinijuego& jugador = hud.jugadores[i];
        const Participante& participante = participantes[i];
        const AutoGlobo& a = m.autos[i];

        jugador.visible = true;
        jugador.valorNumerico = a.vivo ? a.globos : 0;
        jugador.eliminado = !a.vivo && m.fase != FASE_AUTOS_GLOBO_PREPARACION;
        std::snprintf(jugador.valor, sizeof(jugador.valor), a.globos == 1 ? "1 GLOBO" : "%d GLOBOS", a.vivo ? a.globos : 0);

        if (a.vivo)
        {
            float lista = a.turboRecarga <= 0.0f ? 1.0f : 1.0f - a.turboRecarga / RECARGA_TURBO_GLOBO;
            jugador.progreso = Lim01AG(lista);
            std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), a.globos == 1 ? "1 GLOBO" : "%d GLOBOS", a.globos);

            if (m.fase == FASE_AUTOS_GLOBO_JUGANDO)
            {
                if (a.turboActivo > 0.0f)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡TURBO!");
                else if (m.visual.recarga[i] > 0.0f)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡+1 GLOBO!");
                else if (a.inmunidad > 0.0f)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡PUM! INMUNE");
                else if (a.turboRecarga <= 0.0f)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "TURBO LISTO");
            }
        }
        else
        {
            std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "FUERA");
            std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "AGUANTÓ %d S", (int)a.tiempoEliminacion);
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MANEJÁ: %s\nTURBO: %s",
                TextoMoverHud(participante),
                TextoBotonGolpeHud(participante)
            );
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoAutosGlobo::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteGlobo(cantidadMaxima);
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(HORIZONTE_AG);

    Camera3D vista = CamaraVistaAG(*this);
    g_ojoAG = OJO_AG;
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derAG = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaAG = Vector3CrossProduct(g_derAG, frente);

    DibujarCieloAG(vista, tiempoAnimacion, decoracion);
    HornearAG();

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Fondo (con profundidad) y trafico: todo queda detras o debajo del suelo.
    DibujarMallaAG(g_hornoAG.fondo, g_hornoAG.fondoValido, true);

    if (decoracion)
    {
        DibujarTraficoAG(tiempoAnimacion, paso);
        rlDrawRenderBatchActive();
    }

    // Suelo sin profundidad y marcas por frame encima.
    DibujarMallaAG(g_hornoAG.suelo, g_hornoAG.sueloValido, false);
    DibujarMarcasSueloAG(*this, limite, sombras);

    // Escena cercana, vida, placas y autos.
    DibujarMallaAG(g_hornoAG.escena, g_hornoAG.escenaValida, true);
    DibujarAnimadoCercaAG(*this, decoracion);
    DibujarPlacasAG(*this);
    DibujarAutosAG(*this, participantes, limite);
    rlDrawRenderBatchActive();

    // Translucido: parabrisas, barrera, efectos y luces.
    DibujarAutosTranslucidoAG(*this, limite, efectos);
    DibujarBarrerasAG(*this, limite);
    DibujarEfectosAG(*this, participantes, limite);
    rlDrawRenderBatchActive();

    if (efectos)
    {
        DibujarLucesAG(*this, decoracion);
    }

    rlEnableBackfaceCulling();

    if (mostrarDebug)
    {
        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo || !autos[i].vivo) continue;

            DrawCircle3D({ autos[i].x, 0.1f, autos[i].z }, RADIO_AUTO_GLOBO, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }

        for (int s = 0; s < CANTIDAD_SITIOS_PLACA_GLOBO; s++)
        {
            DrawCircle3D({ SITIOS_PLACA_X_GLOBO[s], 0.1f, SITIOS_PLACA_Z_GLOBO[s] }, RADIO_PLACA_GLOBO, { 1.0f, 0.0f, 0.0f }, 90.0f, YELLOW);
        }
    }

    EndMode3D();

    DibujarAnunciosAG(*this, participantes, limite, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    (void)jugadores;
    DatosHudMinijuego hud;
    PrepararHudAG(*this, participantes, limite, hud);
    DibujarHudMinijuego(hud, participantes);
}


static Vector3 PosicionGloboVisual(const AutoGlobo& a, int k, float tiempo, int indice)
{
    float fx = std::cos(a.angulo);
    float fz = std::sin(a.angulo);
    float sx = -fz;
    float sz = fx;
    float lateral = ((float)k - 1.0f) * 0.66f;
    float balanceo = std::sin(tiempo * 3.0f + (float)(k + indice * 3)) * 0.1f;
    float atras = 1.2f + 0.1f * std::fabs((float)k - 1.0f);
    float altura = 2.55f + 0.3f * (float)(k == 1 ? 1 : 0) + 0.06f * std::sin(tiempo * 4.0f + (float)k);
    float rastroX = -a.vx * 0.06f;
    float rastroZ = -a.vz * 0.06f;

    return {
        a.x - fx * atras + sx * (lateral + balanceo) + rastroX,
        altura,
        a.z - fz * atras + sz * (lateral + balanceo) + rastroZ
    };
}


const ResultadoMinijuego& MinijuegoAutosGlobo::ObtenerResultado() const
{
    return resultado;
}
