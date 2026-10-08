#include "Minigames/MinijuegoAutosGlobo.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


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


static const char* TextoTurboGlobo(const Participante& participante)
{
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
// VISUAL: ESCENARIO (solo decoracion, la logica no depende de esto)
//==================================================

// MODELO FUTURO: plaza elevada con barreras de energia, rascacielos de
// cristal con jardines verticales, autopistas flotantes con trafico,
// hologramas, drones, placas de carga y autos flotantes con globos de
// energia: reemplazar cada uno por un GLB.

static float HashGlobo(int semilla)
{
    unsigned int x = (unsigned int)semilla * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return (float)(x & 0xFFFFu) / 65535.0f;
}


static void DibujarPlazaGlobo(const MinijuegoAutosGlobo& m)
{
    const float anchoPlaza = (LIMITE_X_GLOBO + RADIO_AUTO_GLOBO) * 2.0f;
    const float largoPlaza = (LIMITE_Z_GLOBO + RADIO_AUTO_GLOBO) * 2.0f;
    Color neon = Color{ 80, 220, 255, 255 };

    // Base y losa.
    DrawCube({ 0.0f, -1.0f, 0.0f }, anchoPlaza + 1.6f, 2.0f, largoPlaza + 1.6f, Color{ 24, 28, 46, 255 });
    DrawCube({ 0.0f, -0.05f, 0.0f }, anchoPlaza, 0.1f, largoPlaza, Color{ 34, 42, 70, 255 });

    // Cuadricula luminosa.
    for (int k = -8; k <= 8; k += 2)
    {
        DrawCube({ (float)k, 0.005f, 0.0f }, 0.04f, 0.01f, largoPlaza, Fade(neon, 0.35f));
    }

    for (int k = -6; k <= 6; k += 2)
    {
        DrawCube({ 0.0f, 0.005f, (float)k }, anchoPlaza, 0.01f, 0.04f, Fade(neon, 0.35f));
    }

    // Anillo central.
    float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 2.0f);
    DrawCircle3D({ 0.0f, 0.02f, 0.0f }, 1.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(neon, 0.5f + 0.4f * pulso));
    DrawCircle3D({ 0.0f, 0.02f, 0.0f }, 2.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 255, 90, 200, 255 }, 0.4f));

    // Barreras de energia en el borde.
    Color barrera = Color{ 255, 90, 200, 255 };
    float ex = anchoPlaza * 0.5f;
    float ez = largoPlaza * 0.5f;

    DrawCube({ 0.0f, 0.25f, -ez }, anchoPlaza + 0.4f, 0.5f, 0.3f, Color{ 50, 56, 90, 255 });
    DrawCube({ 0.0f, 0.25f, ez }, anchoPlaza + 0.4f, 0.5f, 0.3f, Color{ 50, 56, 90, 255 });
    DrawCube({ -ex, 0.25f, 0.0f }, 0.3f, 0.5f, largoPlaza, Color{ 50, 56, 90, 255 });
    DrawCube({ ex, 0.25f, 0.0f }, 0.3f, 0.5f, largoPlaza, Color{ 50, 56, 90, 255 });

    DrawCube({ 0.0f, 0.52f, -ez }, anchoPlaza + 0.4f, 0.05f, 0.1f, barrera);
    DrawCube({ 0.0f, 0.52f, ez }, anchoPlaza + 0.4f, 0.05f, 0.1f, barrera);
    DrawCube({ -ex, 0.52f, 0.0f }, 0.1f, 0.05f, largoPlaza, barrera);
    DrawCube({ ex, 0.52f, 0.0f }, 0.1f, 0.05f, largoPlaza, barrera);
}


static void DibujarCiudadGlobo(const MinijuegoAutosGlobo& m)
{
    float t = m.tiempoAnimacion;

    // Rascacielos de cristal tras la plaza y a los lados.
    for (int k = 0; k < 30; k++)
    {
        float x = 0.0f;
        float z = 0.0f;

        if (k < 14)
        {
            x = -26.0f + (float)k * 4.0f + HashGlobo(k) * 1.5f;
            z = -16.0f - HashGlobo(k + 50) * 6.0f;
        }
        else
        {
            x = (k % 2 == 0 ? -1.0f : 1.0f) * (15.5f + HashGlobo(k + 100) * 6.0f);
            z = -12.0f + (float)((k - 14) / 2) * 3.4f;
        }

        float ancho = 2.4f + HashGlobo(k + 200) * 1.8f;
        float alto = 8.0f + HashGlobo(k + 300) * 14.0f;
        Color cristal = (k % 3 == 0) ? Color{ 40, 80, 130, 255 } : ((k % 3 == 1) ? Color{ 50, 100, 150, 255 } : Color{ 34, 64, 110, 255 });

        DrawCube({ x, alto * 0.5f - 6.0f, z }, ancho, alto, ancho, cristal);
        DrawCubeWires({ x, alto * 0.5f - 6.0f, z }, ancho, alto, ancho, Color{ 110, 190, 240, 255 });

        // Hileras de ventanas encendidas.
        for (int p = 0; p < 4; p++)
        {
            float y = -4.0f + HashGlobo(k * 7 + p) * (alto - 4.0f);
            DrawCube({ x, y, z + ancho * 0.5f + 0.01f }, ancho * 0.8f, 0.12f, 0.02f, Fade(Color{ 255, 240, 160, 255 }, 0.8f));
        }

        // Jardin vertical en algunos edificios.
        if (k % 3 == 0)
        {
            DrawCube({ x, alto * 0.5f - 3.0f, z + ancho * 0.5f + 0.06f }, ancho * 0.5f, alto * 0.4f, 0.1f, Color{ 60, 170, 80, 255 });
        }
    }

    // Autopista flotante trasera con trafico luminoso.
    DrawCube({ 0.0f, -0.4f, -9.6f }, 50.0f, 0.3f, 2.2f, Color{ 52, 58, 86, 255 });
    DrawCube({ 0.0f, -0.22f, -9.6f }, 50.0f, 0.02f, 0.08f, Fade(Color{ 255, 230, 120, 255 }, 0.8f));

    for (int k = 0; k < 8; k++)
    {
        float x = std::fmod(t * (4.0f + (float)(k % 3)) + (float)k * 7.0f, 50.0f) - 25.0f;
        float z = -9.6f + (k % 2 == 0 ? -0.5f : 0.5f);
        DrawCube({ x, -0.1f, z }, 0.8f, 0.2f, 0.4f, (k % 2 == 0) ? Color{ 255, 120, 120, 255 } : Color{ 120, 220, 255, 255 });
    }

    // Autopista flotante lateral.
    DrawCube({ 12.4f, -0.5f, 0.0f }, 2.0f, 0.3f, 40.0f, Color{ 52, 58, 86, 255 });

    for (int k = 0; k < 5; k++)
    {
        float z = std::fmod(t * 5.0f + (float)k * 9.0f, 40.0f) - 20.0f;
        DrawCube({ 12.4f + (k % 2 == 0 ? -0.4f : 0.4f), -0.25f, z }, 0.4f, 0.2f, 0.8f, Color{ 120, 220, 255, 255 });
    }

    // Hologramas en las esquinas de la plaza.
    const float holoX[4] = { -10.6f, 10.6f, -10.6f, 10.6f };
    const float holoZ[4] = { -7.6f, -7.6f, 7.6f, 7.6f };

    for (int h = 0; h < 4; h++)
    {
        float y = 2.4f + 0.2f * std::sin(t * 2.0f + (float)h);
        Color holo = (h % 2 == 0) ? Color{ 90, 255, 220, 255 } : Color{ 255, 120, 230, 255 };

        DrawCylinder({ holoX[h], 0.0f, holoZ[h] }, 0.3f, 0.45f, 0.5f, 8, Color{ 50, 56, 90, 255 });
        DrawCylinder({ holoX[h], 0.5f, holoZ[h] }, 0.04f, 0.04f, y - 0.5f, 6, Fade(holo, 0.5f));

        rlPushMatrix();
        rlTranslatef(holoX[h], y, holoZ[h]);
        rlRotatef(t * 60.0f + (float)h * 45.0f, 0.0f, 1.0f, 0.0f);
        DrawCircle3D({ 0.0f, 0.0f, 0.0f }, 0.8f, { 1.0f, 0.0f, 0.0f }, 90.0f, holo);
        DrawCircle3D({ 0.0f, 0.0f, 0.0f }, 0.8f, { 0.0f, 0.0f, 1.0f }, 90.0f, Fade(holo, 0.7f));
        DrawCircle3D({ 0.0f, 0.0f, 0.0f }, 0.8f, { 0.0f, 1.0f, 0.0f }, 90.0f, Fade(holo, 0.5f));
        rlPopMatrix();
    }

    // Drones en orbita.
    for (int d = 0; d < 4; d++)
    {
        float angulo = t * (0.5f + 0.1f * (float)d) + (float)d * 1.57f;
        float x = std::cos(angulo) * 13.0f;
        float z = std::sin(angulo) * 9.5f;
        float y = 3.5f + (float)d * 0.6f;
        float aspa = t * 30.0f;

        DrawSphere({ x, y, z }, 0.22f, Color{ 220, 224, 240, 255 });
        DrawLine3D({ x - std::cos(aspa) * 0.4f, y + 0.15f, z - std::sin(aspa) * 0.4f }, { x + std::cos(aspa) * 0.4f, y + 0.15f, z + std::sin(aspa) * 0.4f }, Color{ 150, 160, 190, 255 });
        DrawLine3D({ x - std::sin(aspa) * 0.4f, y + 0.15f, z + std::cos(aspa) * 0.4f }, { x + std::sin(aspa) * 0.4f, y + 0.15f, z - std::cos(aspa) * 0.4f }, Color{ 150, 160, 190, 255 });

        if (std::fmod(t * 2.0f + (float)d, 1.0f) < 0.5f)
        {
            DrawSphere({ x, y - 0.15f, z }, 0.07f, RED);
        }
    }
}


static void DibujarPlacasGlobo(const MinijuegoAutosGlobo& m)
{
    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        const PlacaAutosGlobo& placa = m.placas[p];

        if (!placa.activa)
        {
            continue;
        }

        if (placa.enAviso)
        {
            bool parpadeo = std::fmod(m.tiempoAnimacion * 6.0f, 1.0f) < 0.5f;
            DrawCircle3D({ placa.x, 0.03f, placa.z }, RADIO_PLACA_GLOBO, { 1.0f, 0.0f, 0.0f }, 90.0f, parpadeo ? ORANGE : Fade(ORANGE, 0.3f));
            DrawCircle3D({ placa.x, 0.03f, placa.z }, RADIO_PLACA_GLOBO * 0.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(ORANGE, 0.5f));
            continue;
        }

        float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 5.0f);
        Color verde = Color{ 90, 255, 160, 255 };
        bool poco = placa.tiempo < 3.0f && std::fmod(m.tiempoAnimacion * 8.0f, 1.0f) < 0.5f;

        DrawCylinder({ placa.x, 0.0f, placa.z }, RADIO_PLACA_GLOBO, RADIO_PLACA_GLOBO, 0.06f, 24, Fade(verde, poco ? 0.25f : 0.6f));
        DrawCircle3D({ placa.x, 0.08f, placa.z }, RADIO_PLACA_GLOBO, { 1.0f, 0.0f, 0.0f }, 90.0f, verde);

        // Haz de luz y globo flotante.
        DrawCylinder({ placa.x, 0.06f, placa.z }, 0.5f, 0.5f, 1.6f, 12, Fade(verde, 0.12f + 0.1f * pulso));
        DrawSphere({ placa.x, 0.9f + 0.15f * pulso, placa.z }, 0.25f, verde);
    }
}


//==================================================
// VISUAL: AUTOS
//==================================================

static void DibujarAutoGlobo(
    const AutoGlobo& a,
    Color color,
    float tiempo,
    int indice
)
{
    bool parpadeo = a.inmunidad > 0.0f && std::fmod(tiempo * 12.0f, 1.0f) < 0.5f;
    Color carroceria = parpadeo ? WHITE : color;
    float fx = std::cos(a.angulo);
    float fz = std::sin(a.angulo);
    float sx = -fz;
    float sz = fx;

    // Sombra y halo de levitacion.
    DrawCircle3D({ a.x, 0.02f, a.z }, 0.95f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.35f));
    DrawCircle3D({ a.x, 0.05f, a.z }, 0.8f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 120, 230, 255, 255 }, 0.7f));

    rlPushMatrix();
    rlTranslatef(a.x, 0.0f, a.z);
    rlRotatef(-a.angulo * RAD2DEG, 0.0f, 1.0f, 0.0f);

    // Chasis: el frente (+X local) es la zona que embiste.
    DrawCube({ 0.0f, 0.3f, 0.0f }, 2.0f, 0.3f, 1.2f, carroceria);
    DrawCube({ 0.8f, 0.27f, 0.0f }, 0.6f, 0.2f, 1.0f, Color{ 230, 235, 245, 255 });
    DrawCube({ 1.1f, 0.3f, 0.0f }, 0.08f, 0.22f, 1.1f, Color{ 120, 230, 255, 255 });
    DrawCube({ -0.1f, 0.55f, 0.0f }, 0.8f, 0.22f, 0.9f, Fade(Color{ 150, 220, 255, 255 }, 0.7f));
    DrawCube({ -1.02f, 0.32f, 0.0f }, 0.06f, 0.16f, 1.0f, Color{ 255, 70, 70, 255 });

    // Aletas traseras.
    DrawCube({ -0.9f, 0.6f, 0.5f }, 0.3f, 0.22f, 0.07f, carroceria);
    DrawCube({ -0.9f, 0.6f, -0.5f }, 0.3f, 0.22f, 0.07f, carroceria);

    // Discos de levitacion.
    for (int ex = -1; ex <= 1; ex += 2)
    {
        for (int ez = -1; ez <= 1; ez += 2)
        {
            DrawCylinder({ 0.65f * (float)ex, 0.08f, 0.5f * (float)ez }, 0.17f, 0.17f, 0.1f, 8, Color{ 120, 230, 255, 255 });
        }
    }

    // Llama del turbo.
    if (a.turboActivo > 0.0f)
    {
        float largo = 0.9f + 0.4f * std::sin(tiempo * 50.0f);
        DrawCylinderEx({ -1.05f, 0.35f, 0.0f }, { -1.05f - largo, 0.35f, 0.0f }, 0.28f, 0.0f, 6, Color{ 255, 170, 60, 255 });
        DrawCylinderEx({ -1.05f, 0.35f, 0.0f }, { -1.05f - largo * 0.6f, 0.35f, 0.0f }, 0.16f, 0.0f, 6, Color{ 255, 250, 200, 255 });
    }

    rlPopMatrix();

    // Globos de energia amarrados detras del auto.
    float rastroX = -a.vx * 0.08f;
    float rastroZ = -a.vz * 0.08f;

    for (int k = 0; k < a.globos; k++)
    {
        float lateral = ((float)k - 1.0f) * 0.5f;
        float balanceo = std::sin(tiempo * 3.0f + (float)(k + indice * 3)) * 0.12f;
        float atras = 0.7f + 0.15f * std::fabs((float)k - 1.0f);
        float altura = 1.9f + 0.2f * (float)(k % 2) + 0.08f * std::sin(tiempo * 4.0f + (float)k);

        Vector3 amarre = { a.x - fx * 0.9f, 0.6f, a.z - fz * 0.9f };
        Vector3 globo =
        {
            a.x - fx * atras + sx * (lateral + balanceo) + rastroX,
            altura,
            a.z - fz * atras + sz * (lateral + balanceo) + rastroZ
        };

        DrawLine3D(amarre, { globo.x, globo.y - 0.3f, globo.z }, Color{ 230, 235, 245, 255 });
        DrawSphere(globo, 0.4f, Fade(parpadeo ? WHITE : color, 0.3f));
        DrawSphere(globo, 0.3f, parpadeo ? WHITE : color);
        DrawSphere({ globo.x - 0.1f, globo.y + 0.1f, globo.z }, 0.08f, WHITE);
    }
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoAutosGlobo::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteGlobo(cantidadMaxima);
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    ClearBackground(Color{ 14, 18, 40, 255 });
    DrawRectangleGradientV(0, 0, anchoPantalla, altoPantalla, Color{ 24, 16, 64, 255 }, Color{ 12, 40, 80, 255 });

    BeginMode3D(camara);

    DibujarCiudadGlobo(*this);
    DibujarPlazaGlobo(*this);
    DibujarPlacasGlobo(*this);

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !autos[i].vivo)
        {
            continue;
        }

        DibujarAutoGlobo(autos[i], participantes[i].color, tiempoAnimacion, i);

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawCircle3D({ autos[i].x, 0.1f, autos[i].z }, RADIO_AUTO_GLOBO, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_AUTOS_GLOBO);

    EndMode3D();

    // Etiquetas flotantes.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !autos[i].vivo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen({ autos[i].x, 2.6f, autos[i].z }, camara);
        const char* nombre = NombreJugadorGlobo(participantes[i], i);
        int ancho = MeasureText(nombre, 18);

        DrawRectangle((int)pantalla.x - ancho / 2 - 4, (int)pantalla.y - 2, ancho + 8, 22, Fade(BLACK, 0.55f));
        DrawText(nombre, (int)pantalla.x - ancho / 2, (int)pantalla.y, 18, participantes[i].color);
    }

    // Aviso sobre placas.
    for (int p = 0; p < MAX_PLACAS_AUTOS_GLOBO; p++)
    {
        if (!placas[p].activa)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen({ placas[p].x, 1.8f, placas[p].z }, camara);
        const char* texto = placas[p].enAviso
            ? TextFormat("CARGA EN %.1f", placas[p].tiempo > 0.0f ? placas[p].tiempo : 0.0f)
            : "+1 GLOBO";
        int ancho = MeasureText(texto, 18);

        DrawRectangle((int)pantalla.x - ancho / 2 - 5, (int)pantalla.y - 2, ancho + 10, 22, Fade(BLACK, 0.6f));
        DrawText(texto, (int)pantalla.x - ancho / 2, (int)pantalla.y, 18, placas[p].enAviso ? ORANGE : Color{ 90, 255, 160, 255 });
    }

    // Cabecera.
    DrawRectangle(14, 12, 560, 92, Fade(BLACK, 0.74f));
    DrawText("AUTOS DE GLOBO", 28, 20, 28, GOLD);
    DrawText("EMBISTE CON EL FRENTE EL LADO O LA COLA DE UN RIVAL", 28, 54, 15, RAYWHITE);
    DrawText("ACELERAR / GIRAR: WASD, FLECHAS O STICK   PLACA VERDE: +1 GLOBO", 28, 76, 14, LIGHTGRAY);

    if (fase == FASE_AUTOS_GLOBO_JUGANDO)
    {
        DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
        DrawText(
            TextFormat("TIEMPO %.1f", tiempoRestante),
            anchoPantalla - 200,
            22,
            26,
            tiempoRestante <= 6.0f ? RED : GOLD
        );

        if (tiempoEvento > 0.0f)
        {
            int a = MeasureText(textoEvento, 26);
            DrawRectangle(anchoPantalla / 2 - a / 2 - 14, 112, a + 28, 40, Fade(BLACK, 0.8f));
            DrawText(textoEvento, anchoPantalla / 2 - a / 2, 119, 26, ORANGE);
        }
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
            const AutoGlobo& a = autos[i];

            DrawRectangle(x, y, anchoTarjeta, 80, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 80, participantes[i].color);
            DrawText(NombreJugadorGlobo(participantes[i], i), x + 18, y + 8, 20, participantes[i].color);

            if (a.vivo)
            {
                for (int g = 0; g < GLOBOS_INICIALES_AUTOS_GLOBO; g++)
                {
                    int cx = x + anchoTarjeta - 24 - g * 24;
                    DrawCircle(cx, y + 20, 9.0f, g < a.globos ? participantes[i].color : Fade(GRAY, 0.4f));
                }

                // Barra de recarga del turbo.
                float lista = a.turboRecarga <= 0.0f ? 1.0f : 1.0f - a.turboRecarga / RECARGA_TURBO_GLOBO;
                DrawRectangle(x + 18, y + 36, 110, 10, Fade(GRAY, 0.5f));
                DrawRectangle(x + 18, y + 36, (int)(110.0f * LimitarGlobo(lista, 0.0f, 1.0f)), 10, lista >= 1.0f ? Color{ 120, 230, 255, 255 } : ORANGE);
                DrawText("TURBO", x + 136, y + 33, 14, lista >= 1.0f ? Color{ 120, 230, 255, 255 } : LIGHTGRAY);
            }
            else
            {
                DrawText("ELIMINADO", x + 18, y + 34, 18, GRAY);
            }

            if (participantes[i].esBot)
            {
                DrawText("CONTROL: BOT", x + 18, y + 58, 14, LIGHTGRAY);
            }
            else
            {
                DrawText(
                    TextFormat(
                        "ACEL: %s  TURBO: %s",
                        ObtenerTextoAccionDireccionalControl(participantes[i], CONTROL_DIRECCION_ARRIBA),
                        TextoTurboGlobo(participantes[i])
                    ),
                    x + 18,
                    y + 58,
                    14,
                    LIGHTGRAY
                );
            }

            k++;
        }
    }

    if (fase == FASE_AUTOS_GLOBO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 90, 96, GOLD);

        const char* ayuda = "REVIENTA LOS GLOBOS RIVALES CHOCANDO CON EL FRENTE DE TU AUTO";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 + 20, 20, RAYWHITE);

        const char* ayuda2 = "TURBO CORTO: RECARGA 2.5 S";
        DrawText(ayuda2, anchoPantalla / 2 - MeasureText(ayuda2, 18) / 2, altoPantalla / 2 + 48, 18, LIGHTGRAY);
    }
    else if (
        fase == FASE_AUTOS_GLOBO_TERMINADO &&
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
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorGlobo(participantes[ganadores[0]], ganadores[0]));
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
                        NombreJugadorGlobo(participantes[i], i),
                        autos[i].vivo ? TextFormat("%d GLOBOS", autos[i].globos) : "ELIMINADO"
                    ),
                    px + 40,
                    py + 62 + fila * 30,
                    22,
                    participantes[i].color
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


const ResultadoMinijuego& MinijuegoAutosGlobo::ObtenerResultado() const
{
    return resultado;
}
