#include "Minigames/MinijuegoEsferasCanon.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_ESFERAS = 3.0f;
static const float DURACION_PARTIDA_ESFERAS = 70.0f;
static const float TIEMPO_CIERRE_ESFERAS = 12.0f;

// Pista.
static const float PENDIENTE_Y_ESFERAS = 0.12f;
static const float LONGITUD_META_ESFERAS = 120.0f;
static const float MITAD_ANCHO_PISTA_ESFERAS = 4.2f;
static const float LIMITE_LATERAL_ESFERAS = 3.4f;
static const float S_SALIDA_ESFERAS = 2.0f;

// Esfera.
static const float RADIO_ESFERA = 1.0f;
static const float ACELERACION_ENTRADA_ESFERAS = 3.8f;
static const float ACELERACION_PENDIENTE_ESFERAS = 2.0f;
static const float ROZAMIENTO_TANGENTE_ESFERAS = 0.9f;
static const float ROZAMIENTO_LATERAL_ESFERAS = 1.6f;
static const float ROZAMIENTO_ARENA_ESFERAS = 3.2f;
static const float ROZAMIENTO_AIRE_ESFERAS = 0.15f;
static const float VELOCIDAD_MAXIMA_ESFERAS = 15.0f;
static const float IMPULSO_ESFERAS = 6.5f;
static const float RECARGA_IMPULSO_ESFERAS = 3.0f;
static const float RESTITUCION_PARED_ESFERAS = 0.4f;
static const float RESTITUCION_ESFERAS = 0.95f;

// Grietas y caidas.
static const float PENALIZACION_ESFERAS = 2.0f;
static const float TIEMPO_CAIDA_VISUAL_ESFERAS = 0.6f;

// Rampa de atajo.
static const float RAMPA_S0_ESFERAS = 90.0f;
static const float RAMPA_S1_ESFERAS = 93.5f;
static const float RAMPA_LAT0_ESFERAS = 2.0f;
static const float RAMPA_LAT1_ESFERAS = 3.6f;
static const float TIEMPO_AIRE_ESFERAS = 1.1f;
static const float VELOCIDAD_RAMPA_ESFERAS = 13.0f;
static const float ALTURA_SALTO_ESFERAS = 2.4f;


struct GrietaEsferas
{
    float s0;
    float s1;
    float centroSeguro;
    float mitadSegura;
};


struct ZonaArenaEsferas
{
    float s0;
    float s1;
};


static const int CANTIDAD_GRIETAS_ESFERAS = 3;
static const GrietaEsferas GRIETAS_ESFERAS[CANTIDAD_GRIETAS_ESFERAS] =
{
    { 40.0f, 43.5f, 1.5f, 1.0f },
    { 74.0f, 77.5f, -1.5f, 1.0f },
    { 96.0f, 102.0f, 0.0f, 0.8f }
};

static const int CANTIDAD_ARENAS_ESFERAS = 3;
static const ZonaArenaEsferas ARENAS_ESFERAS[CANTIDAD_ARENAS_ESFERAS] =
{
    { 18.0f, 26.0f },
    { 64.0f, 71.0f },
    { 108.0f, 116.0f }
};

static const int CANTIDAD_CHECKPOINTS_ESFERAS = 3;
static const float CHECKPOINTS_ESFERAS[CANTIDAD_CHECKPOINTS_ESFERAS] = { 28.0f, 58.0f, 88.0f };

// s, lateral, radio, cactus.
struct DatosObstaculoEsferas
{
    float s;
    float lateral;
    float radio;
    bool cactus;
};

static const int CANTIDAD_DATOS_OBSTACULOS_ESFERAS = 20;
static const DatosObstaculoEsferas DATOS_OBSTACULOS_ESFERAS[CANTIDAD_DATOS_OBSTACULOS_ESFERAS] =
{
    { 10.0f, -1.5f, 0.9f, false }, { 14.0f, 1.8f, 0.9f, false },
    { 34.0f, 0.5f, 0.9f, false }, { 48.0f, -2.0f, 0.9f, false },
    { 52.0f, 1.5f, 0.9f, false }, { 80.0f, 1.0f, 0.9f, false },
    { 84.0f, -1.8f, 0.9f, false }, { 104.0f, -1.0f, 0.9f, false },
    { 106.0f, 1.5f, 0.9f, false }, { 112.0f, 0.0f, 0.9f, false },
    { 20.0f, 2.5f, 0.5f, true }, { 24.0f, -2.5f, 0.5f, true },
    { 36.0f, -2.8f, 0.5f, true }, { 44.0f, 2.8f, 0.5f, true },
    { 56.0f, -0.5f, 0.5f, true }, { 68.0f, 1.5f, 0.5f, true },
    { 70.0f, -2.0f, 0.5f, true }, { 86.0f, 2.4f, 0.5f, true },
    { 110.0f, -2.5f, 0.5f, true }, { 114.0f, 2.5f, 0.5f, true }
};


//==================================================
// UTILIDADES
//==================================================

static float LimitarEsferas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AbsEsferas(float valor)
{
    return valor < 0.0f ? -valor : valor;
}


static float AleatorioEsferas(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}


static float Ruido01Esferas(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static int LimiteJugadoresEsferas(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static bool EsControladoPorBotEsferas(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static const char* TextoBotonImpulsoEsferas(const Participante& participante)
{
    if (participante.control == CONTROL_GAMEPAD)
    {
        return "B";
    }

    return participante.control == CONTROL_TECLADO_FLECHAS ? "SHIFT DER" : "E";
}


static float ElevacionPistaEsferas(float s)
{
    return -PENDIENTE_Y_ESFERAS * s;
}


//==================================================
// GEOMETRIA DE LA PISTA (GAMEPLAY)
//==================================================

struct ProyeccionEsferas
{
    float s = 0.0f;
    float lateral = 0.0f;
    float px = 0.0f;
    float pz = 0.0f;
    float tx = 0.0f;
    float tz = -1.0f;
    float rx = 1.0f;
    float rz = 0.0f;
};


// Punto de la linea central, su tangente y su lateral derecho a distancia s.
static void PuntoPistaEsferas(
    const MinijuegoEsferasCanon& minijuego,
    float s,
    float lateral,
    float& x,
    float& z,
    float& tangenteX,
    float& tangenteZ
)
{
    s = LimitarEsferas(s, 0.0f, minijuego.longitudPista);

    int i = 0;

    while (i < PUNTOS_PISTA_ESFERAS - 2 && minijuego.puntosS[i + 1] < s)
    {
        i++;
    }

    float dx = minijuego.puntosX[i + 1] - minijuego.puntosX[i];
    float dz = minijuego.puntosZ[i + 1] - minijuego.puntosZ[i];
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.0001f)
    {
        largo = 1.0f;
        dz = -1.0f;
    }

    float t = LimitarEsferas((s - minijuego.puntosS[i]) / largo, 0.0f, 1.0f);

    tangenteX = dx / largo;
    tangenteZ = dz / largo;
    x = minijuego.puntosX[i] + dx * t + (-tangenteZ) * lateral;
    z = minijuego.puntosZ[i] + dz * t + tangenteX * lateral;
}


static ProyeccionEsferas ProyectarPistaEsferas(
    const MinijuegoEsferasCanon& minijuego,
    float x,
    float z
)
{
    ProyeccionEsferas mejor{};
    float mejorDistancia2 = 1.0e12f;

    for (int i = 0; i < PUNTOS_PISTA_ESFERAS - 1; i++)
    {
        float ax = minijuego.puntosX[i];
        float az = minijuego.puntosZ[i];
        float dx = minijuego.puntosX[i + 1] - ax;
        float dz = minijuego.puntosZ[i + 1] - az;
        float largo2 = dx * dx + dz * dz;

        if (largo2 < 0.000001f)
        {
            continue;
        }

        float t = LimitarEsferas(((x - ax) * dx + (z - az) * dz) / largo2, 0.0f, 1.0f);
        float qx = ax + dx * t;
        float qz = az + dz * t;
        float d2 = (x - qx) * (x - qx) + (z - qz) * (z - qz);

        if (d2 < mejorDistancia2)
        {
            float largo = std::sqrt(largo2);

            mejorDistancia2 = d2;
            mejor.px = qx;
            mejor.pz = qz;
            mejor.tx = dx / largo;
            mejor.tz = dz / largo;
            mejor.rx = -mejor.tz;
            mejor.rz = mejor.tx;
            mejor.s = minijuego.puntosS[i] + t * largo;
            mejor.lateral = (x - qx) * mejor.rx + (z - qz) * mejor.rz;
        }
    }

    return mejor;
}


static void ConstruirPistaEsferas(MinijuegoEsferasCanon& minijuego)
{
    // Puntos de control: avanza hacia -Z serpenteando en X.
    static const int CANTIDAD_CONTROL = 14;
    static const float CONTROL_X[CANTIDAD_CONTROL] =
    {
        0.0f, 0.0f, 3.0f, 8.0f, 9.0f, 4.0f, -3.0f,
        -8.0f, -9.0f, -4.0f, 3.0f, 7.0f, 3.0f, 0.0f
    };

    int cantidad = 0;

    for (int i = 0; i < CANTIDAD_CONTROL - 1; i++)
    {
        int a = i > 0 ? i - 1 : 0;
        int d = i + 2 < CANTIDAD_CONTROL ? i + 2 : CANTIDAD_CONTROL - 1;

        for (int k = 0; k < 4; k++)
        {
            float t = (float)k / 4.0f;
            float t2 = t * t;
            float t3 = t2 * t;

            float p0x = CONTROL_X[a];
            float p1x = CONTROL_X[i];
            float p2x = CONTROL_X[i + 1];
            float p3x = CONTROL_X[d];

            minijuego.puntosX[cantidad] = 0.5f * (
                2.0f * p1x +
                (-p0x + p2x) * t +
                (2.0f * p0x - 5.0f * p1x + 4.0f * p2x - p3x) * t2 +
                (-p0x + 3.0f * p1x - 3.0f * p2x + p3x) * t3
            );
            minijuego.puntosZ[cantidad] = -10.0f * ((float)i + t);
            cantidad++;
        }
    }

    minijuego.puntosX[cantidad] = CONTROL_X[CANTIDAD_CONTROL - 1];
    minijuego.puntosZ[cantidad] = -10.0f * (float)(CANTIDAD_CONTROL - 1);
    cantidad++;

    minijuego.puntosS[0] = 0.0f;

    for (int i = 1; i < PUNTOS_PISTA_ESFERAS; i++)
    {
        float dx = minijuego.puntosX[i] - minijuego.puntosX[i - 1];
        float dz = minijuego.puntosZ[i] - minijuego.puntosZ[i - 1];
        minijuego.puntosS[i] = minijuego.puntosS[i - 1] + std::sqrt(dx * dx + dz * dz);
    }

    minijuego.longitudPista = minijuego.puntosS[PUNTOS_PISTA_ESFERAS - 1];

    // Obstaculos a lo largo de la pista.
    minijuego.cantidadObstaculos = 0;

    for (int k = 0; k < CANTIDAD_DATOS_OBSTACULOS_ESFERAS && k < MAX_OBSTACULOS_ESFERAS; k++)
    {
        const DatosObstaculoEsferas& datos = DATOS_OBSTACULOS_ESFERAS[k];
        ObstaculoEsferas& obstaculo = minijuego.obstaculos[minijuego.cantidadObstaculos++];
        float tx = 0.0f;
        float tz = 0.0f;

        obstaculo.s = datos.s;
        obstaculo.lateral = datos.lateral;
        obstaculo.radio = datos.radio;
        obstaculo.cactus = datos.cactus;
        PuntoPistaEsferas(minijuego, datos.s, datos.lateral, obstaculo.x, obstaculo.z, tx, tz);
    }
}


//==================================================
// FISICA DE LAS ESFERAS
//==================================================

static bool EnArenaEsferas(float s)
{
    for (int k = 0; k < CANTIDAD_ARENAS_ESFERAS; k++)
    {
        if (s >= ARENAS_ESFERAS[k].s0 && s <= ARENAS_ESFERAS[k].s1)
        {
            return true;
        }
    }

    return false;
}


static void PosicionarEnPistaEsferas(
    const MinijuegoEsferasCanon& minijuego,
    EstadoJugadorEsferas& estado,
    float s,
    float lateral
)
{
    float tx = 0.0f;
    float tz = 0.0f;

    PuntoPistaEsferas(minijuego, s, lateral, estado.x, estado.z, tx, tz);
    estado.velocidadX = 0.0f;
    estado.velocidadZ = 0.0f;
    estado.s = s;
    estado.lateral = lateral;
    estado.tiempoAire = 0.0f;
}


static float LateralSalidaEsferas(int indice)
{
    return ((float)(indice % MAX_PARTICIPANTES) - 1.5f) * 2.2f;
}


static void ReaparecerEsferas(
    MinijuegoEsferasCanon& minijuego,
    int indice
)
{
    EstadoJugadorEsferas& estado = minijuego.estadosJugadores[indice];
    float s = estado.checkpoints == 0
        ? S_SALIDA_ESFERAS
        : CHECKPOINTS_ESFERAS[estado.checkpoints - 1];

    PosicionarEnPistaEsferas(minijuego, estado, s, LateralSalidaEsferas(indice) * 0.5f);
}


// Un paso de fisica de una esfera. entrada: direccion pedida (-1..1).
static void PasoEsferaEsferas(
    MinijuegoEsferasCanon& minijuego,
    int indice,
    float dirX,
    float dirZ,
    float deltaTime
)
{
    EstadoJugadorEsferas& estado = minijuego.estadosJugadores[indice];

    if (estado.penalizacion > 0.0f)
    {
        float antes = estado.penalizacion;
        estado.penalizacion -= deltaTime;

        if (antes > PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS &&
            estado.penalizacion <= PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS)
        {
            ReaparecerEsferas(minijuego, indice);
        }

        if (estado.penalizacion < 0.0f)
        {
            estado.penalizacion = 0.0f;
        }

        return;
    }

    bool enAire = estado.tiempoAire > 0.0f;

    if (estado.terminado)
    {
        float freno = 1.0f - 2.5f * deltaTime;
        if (freno < 0.0f) freno = 0.0f;
        estado.velocidadX *= freno;
        estado.velocidadZ *= freno;
    }

    ProyeccionEsferas proyeccion = ProyectarPistaEsferas(minijuego, estado.x, estado.z);
    bool enArena = !enAire && EnArenaEsferas(proyeccion.s);

    // Aceleraciones: entrada, pendiente y rozamiento.
    float aceleracion = ACELERACION_ENTRADA_ESFERAS;
    if (enArena) aceleracion *= 0.7f;
    if (enAire) aceleracion *= 0.3f;

    if (!estado.terminado)
    {
        estado.velocidadX += dirX * aceleracion * deltaTime;
        estado.velocidadZ += dirZ * aceleracion * deltaTime;
    }

    if (!enAire)
    {
        estado.velocidadX += proyeccion.tx * ACELERACION_PENDIENTE_ESFERAS * deltaTime;
        estado.velocidadZ += proyeccion.tz * ACELERACION_PENDIENTE_ESFERAS * deltaTime;
    }

    float vt = estado.velocidadX * proyeccion.tx + estado.velocidadZ * proyeccion.tz;
    float vn = estado.velocidadX * proyeccion.rx + estado.velocidadZ * proyeccion.rz;
    float rozamientoT = enAire
        ? ROZAMIENTO_AIRE_ESFERAS
        : (enArena ? ROZAMIENTO_ARENA_ESFERAS : ROZAMIENTO_TANGENTE_ESFERAS);
    float rozamientoN = enAire ? ROZAMIENTO_AIRE_ESFERAS : ROZAMIENTO_LATERAL_ESFERAS;
    float factorT = 1.0f - rozamientoT * deltaTime;
    float factorN = 1.0f - rozamientoN * deltaTime;

    if (factorT < 0.0f) factorT = 0.0f;
    if (factorN < 0.0f) factorN = 0.0f;

    vt *= factorT;
    vn *= factorN;
    estado.velocidadX = proyeccion.tx * vt + proyeccion.rx * vn;
    estado.velocidadZ = proyeccion.tz * vt + proyeccion.rz * vn;

    float velocidad = std::sqrt(estado.velocidadX * estado.velocidadX + estado.velocidadZ * estado.velocidadZ);

    if (velocidad > VELOCIDAD_MAXIMA_ESFERAS)
    {
        float factor = VELOCIDAD_MAXIMA_ESFERAS / velocidad;
        estado.velocidadX *= factor;
        estado.velocidadZ *= factor;
    }

    estado.x += estado.velocidadX * deltaTime;
    estado.z += estado.velocidadZ * deltaTime;

    // Paredes del canon.
    proyeccion = ProyectarPistaEsferas(minijuego, estado.x, estado.z);

    if (AbsEsferas(proyeccion.lateral) > LIMITE_LATERAL_ESFERAS)
    {
        float signo = proyeccion.lateral > 0.0f ? 1.0f : -1.0f;

        estado.x = proyeccion.px + proyeccion.rx * signo * LIMITE_LATERAL_ESFERAS;
        estado.z = proyeccion.pz + proyeccion.rz * signo * LIMITE_LATERAL_ESFERAS;

        float normal = estado.velocidadX * proyeccion.rx + estado.velocidadZ * proyeccion.rz;

        if (normal * signo > 0.0f)
        {
            estado.velocidadX -= proyeccion.rx * normal * (1.0f + RESTITUCION_PARED_ESFERAS);
            estado.velocidadZ -= proyeccion.rz * normal * (1.0f + RESTITUCION_PARED_ESFERAS);
        }

        proyeccion = ProyectarPistaEsferas(minijuego, estado.x, estado.z);
    }

    estado.s = proyeccion.s;
    estado.lateral = proyeccion.lateral;

    if (estado.s > estado.progresoMaximo)
    {
        estado.progresoMaximo = estado.s;
    }

    // Giro visual segun el desplazamiento.
    float rapidez = std::sqrt(estado.velocidadX * estado.velocidadX + estado.velocidadZ * estado.velocidadZ);

    if (rapidez > 0.2f)
    {
        estado.giro += rapidez * deltaTime / RADIO_ESFERA;
        estado.ejeX = -estado.velocidadZ / rapidez;
        estado.ejeZ = estado.velocidadX / rapidez;
    }

    if (enAire)
    {
        estado.tiempoAire -= deltaTime;
        if (estado.tiempoAire < 0.0f) estado.tiempoAire = 0.0f;
        return;
    }

    // Rampa de atajo: lanza la esfera por el aire.
    if (
        !estado.terminado &&
        estado.s >= RAMPA_S0_ESFERAS && estado.s <= RAMPA_S1_ESFERAS &&
        estado.lateral >= RAMPA_LAT0_ESFERAS && estado.lateral <= RAMPA_LAT1_ESFERAS &&
        estado.velocidadX * proyeccion.tx + estado.velocidadZ * proyeccion.tz > 1.5f
    )
    {
        estado.tiempoAire = TIEMPO_AIRE_ESFERAS;
        estado.velocidadX = proyeccion.tx * VELOCIDAD_RAMPA_ESFERAS;
        estado.velocidadZ = proyeccion.tz * VELOCIDAD_RAMPA_ESFERAS;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);
        return;
    }

    // Obstaculos: rocas (rebote) y cactus (rebote fuerte).
    for (int k = 0; k < minijuego.cantidadObstaculos; k++)
    {
        const ObstaculoEsferas& obstaculo = minijuego.obstaculos[k];

        if (AbsEsferas(obstaculo.s - estado.s) > 3.0f)
        {
            continue;
        }

        float dx = estado.x - obstaculo.x;
        float dz = estado.z - obstaculo.z;
        float distancia = std::sqrt(dx * dx + dz * dz);
        float minimo = RADIO_ESFERA + obstaculo.radio;

        if (distancia >= minimo)
        {
            continue;
        }

        float nx = distancia > 0.0001f ? dx / distancia : 1.0f;
        float nz = distancia > 0.0001f ? dz / distancia : 0.0f;

        estado.x = obstaculo.x + nx * minimo;
        estado.z = obstaculo.z + nz * minimo;

        float normal = estado.velocidadX * nx + estado.velocidadZ * nz;

        if (normal < 0.0f)
        {
            float restitucion = obstaculo.cactus ? 1.2f : 0.5f;
            estado.velocidadX -= nx * normal * (1.0f + restitucion);
            estado.velocidadZ -= nz * normal * (1.0f + restitucion);

            if (obstaculo.cactus)
            {
                float saliente = estado.velocidadX * nx + estado.velocidadZ * nz;

                if (saliente < 4.0f)
                {
                    estado.velocidadX += nx * (4.0f - saliente);
                    estado.velocidadZ += nz * (4.0f - saliente);
                }
            }

            if (minijuego.cooldownSonidoChoque <= 0.0f && -normal > 1.5f)
            {
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
                minijuego.cooldownSonidoChoque = 0.15f;
            }
        }
    }

    // Grietas: cae salvo por el puente seguro.
    if (!estado.terminado)
    {
        for (int k = 0; k < CANTIDAD_GRIETAS_ESFERAS; k++)
        {
            const GrietaEsferas& grieta = GRIETAS_ESFERAS[k];

            if (
                estado.s >= grieta.s0 && estado.s <= grieta.s1 &&
                AbsEsferas(estado.lateral - grieta.centroSeguro) > grieta.mitadSegura
            )
            {
                estado.penalizacion = PENALIZACION_ESFERAS;
                estado.caidaX = estado.x;
                estado.caidaZ = estado.z;
                estado.velocidadX = 0.0f;
                estado.velocidadZ = 0.0f;
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
                return;
            }
        }
    }
}


// Choques elasticos entre esferas.
static void ResolverChoquesEsferas(MinijuegoEsferasCanon& minijuego, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorEsferas& a = minijuego.estadosJugadores[i];

        if (!a.participa || a.penalizacion > 0.0f || a.tiempoAire > 0.0f)
        {
            continue;
        }

        for (int j = i + 1; j < limite; j++)
        {
            EstadoJugadorEsferas& b = minijuego.estadosJugadores[j];

            if (!b.participa || b.penalizacion > 0.0f || b.tiempoAire > 0.0f)
            {
                continue;
            }

            float dx = b.x - a.x;
            float dz = b.z - a.z;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float minimo = RADIO_ESFERA * 2.0f;

            if (distancia >= minimo)
            {
                continue;
            }

            float nx = 1.0f;
            float nz = 0.0f;

            if (distancia > 0.0001f)
            {
                nx = dx / distancia;
                nz = dz / distancia;
            }

            float solape = (minimo - distancia) * 0.5f;
            a.x -= nx * solape;
            a.z -= nz * solape;
            b.x += nx * solape;
            b.z += nz * solape;

            float relativa = (a.velocidadX - b.velocidadX) * nx + (a.velocidadZ - b.velocidadZ) * nz;

            if (relativa > 0.0f)
            {
                float impulso = (1.0f + RESTITUCION_ESFERAS) * relativa * 0.5f + 0.4f;

                a.velocidadX -= nx * impulso;
                a.velocidadZ -= nz * impulso;
                b.velocidadX += nx * impulso;
                b.velocidadZ += nz * impulso;

                if (minijuego.cooldownSonidoChoque <= 0.0f && relativa > 1.5f)
                {
                    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
                    minijuego.cooldownSonidoChoque = 0.15f;
                }
            }
        }
    }
}


//==================================================
// IA DE BOTS
//==================================================

static InputMinijuegoParticipante CrearEntradaBotEsferas(
    MinijuegoEsferasCanon& minijuego,
    int indice,
    float deltaTime
)
{
    EstadoJugadorEsferas& estado = minijuego.estadosJugadores[indice];
    InputMinijuegoParticipante entrada{};

    // Antiatasco: sin progreso en 2.5 s -> carril aleatorio, impulso y marcha atras.
    estado.atascoTiempo += deltaTime;
    estado.marchaAtras -= deltaTime;

    if (estado.progresoMaximo > estado.atascoReferencia + 0.3f)
    {
        estado.atascoReferencia = estado.progresoMaximo;
        estado.atascoTiempo = 0.0f;
    }
    else if (estado.atascoTiempo > 2.5f)
    {
        float x = 0.0f;
        float z = 0.0f;
        float tx = 0.0f;
        float tz = 0.0f;

        estado.atascoTiempo = 0.0f;
        estado.latPreferida = AleatorioEsferas(-2.5f, 2.5f);
        estado.latObjetivo = estado.latPreferida;
        PuntoPistaEsferas(minijuego, estado.s + 5.5f, estado.latObjetivo, x, z, tx, tz);
        estado.objetivoX = x;
        estado.objetivoZ = z;
        estado.tiempoDecision = 0.7f;
        estado.marchaAtras = 0.6f;
    }

    estado.tiempoDecision -= deltaTime;

    if (estado.tiempoDecision <= 0.0f)
    {
        estado.tiempoDecision = AleatorioEsferas(0.2f, 0.32f);

        float s = estado.s;
        float latObjetivo = estado.latPreferida + AleatorioEsferas(-0.5f, 0.5f);
        bool carrilObstaculo = false;

        // Esquiva obstaculos cercanos por delante.
        for (int k = 0; k < minijuego.cantidadObstaculos; k++)
        {
            const ObstaculoEsferas& obstaculo = minijuego.obstaculos[k];

            if (
                obstaculo.s > s - 0.5f && obstaculo.s < s + 9.0f &&
                AbsEsferas(obstaculo.lateral - latObjetivo) < obstaculo.radio + 1.9f
            )
            {
                latObjetivo = obstaculo.lateral > 0.0f
                    ? obstaculo.lateral - (obstaculo.radio + 2.0f)
                    : obstaculo.lateral + (obstaculo.radio + 2.0f);
                carrilObstaculo = true;
            }
        }

        latObjetivo = LimitarEsferas(latObjetivo, -3.0f, 3.0f);

        // Grietas: se alinea con el puente seguro.
        for (int k = 0; k < CANTIDAD_GRIETAS_ESFERAS; k++)
        {
            const GrietaEsferas& grieta = GRIETAS_ESFERAS[k];

            if (s > grieta.s0 - 9.0f && s < grieta.s1 + 0.5f)
            {
                latObjetivo = grieta.centroSeguro + AleatorioEsferas(-0.25f, 0.25f) * grieta.mitadSegura;
                carrilObstaculo = true;
            }
        }

        // Atajo arriesgado: sube por la rampa en vez de usar el puente.
        if (estado.arriesgado && s > 78.0f && s < RAMPA_S1_ESFERAS && estado.tiempoAire <= 0.0f)
        {
            latObjetivo = 0.5f * (RAMPA_LAT0_ESFERAS + RAMPA_LAT1_ESFERAS);
        }

        float x = 0.0f;
        float z = 0.0f;
        float tx = 0.0f;
        float tz = 0.0f;

        PuntoPistaEsferas(minijuego, s + 5.5f, latObjetivo, x, z, tx, tz);
        estado.objetivoX = x;
        estado.objetivoZ = z;
        estado.latObjetivo = latObjetivo;

        // Impulso en rectas despejadas.
        if (
            estado.recargaImpulso <= 0.0f &&
            !carrilObstaculo &&
            !EnArenaEsferas(s) &&
            estado.penalizacion <= 0.0f &&
            estado.tiempoAire <= 0.0f &&
            GetRandomValue(1, 100) <= 40
        )
        {
            estado.impulsoPendiente = true;
        }
    }

    float dx = estado.objetivoX - estado.x;
    float dz = estado.objetivoZ - estado.z;

    if (estado.marchaAtras > 0.0f)
    {
        dx = -dx;
        dz = -dz;

        if (estado.marchaAtras - deltaTime <= 0.0f)
        {
            estado.impulsoPendiente = true;
        }
    }

    if (dx > 0.5f) entrada.derecha = true;
    if (dx < -0.5f) entrada.izquierda = true;
    if (dz > 0.5f) entrada.atras = true;
    if (dz < -0.5f) entrada.adelante = true;

    if (estado.impulsoPendiente)
    {
        entrada.golpear = true;
        estado.impulsoPendiente = false;
    }

    return entrada;
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarEsferas(MinijuegoEsferasCanon& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    int orden[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.estadosJugadores[i].participa)
        {
            orden[cantidad++] = i;
        }
    }

    // Meta primero (por tiempo) y despues por progreso.
    for (int a = 0; a < cantidad; a++)
    {
        for (int b = a + 1; b < cantidad; b++)
        {
            const EstadoJugadorEsferas& ea = minijuego.estadosJugadores[orden[a]];
            const EstadoJugadorEsferas& eb = minijuego.estadosJugadores[orden[b]];
            bool intercambiar;

            if (ea.terminado != eb.terminado)
            {
                intercambiar = eb.terminado;
            }
            else if (ea.terminado)
            {
                intercambiar = eb.tiempoMeta < ea.tiempoMeta;
            }
            else
            {
                intercambiar = eb.progresoMaximo > ea.progresoMaximo + 0.001f;
            }

            if (intercambiar)
            {
                int temporal = orden[a];
                orden[a] = orden[b];
                orden[b] = temporal;
            }
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 0;

    for (int k = 0; k < cantidad; k++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[orden[k]];

        resultadoJugador.posicionFinal = k + 1;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            (int)minijuego.estadosJugadores[orden[k]].progresoMaximo;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_ESFERAS_TERMINADO;
}


//==================================================
// INICIALIZACION
//==================================================

void MinijuegoEsferasCanon::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    ConstruirPistaEsferas(*this);

    camara.position = { 0.0f, 10.0f, 12.0f };
    camara.target = { 0.0f, 0.0f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_ESFERAS_PREPARACION;
    partidaValida = false;
    hayGanadorPorMeta = false;

    tiempoPreparacion = DURACION_PREPARACION_ESFERAS;
    tiempoRestante = DURACION_PARTIDA_ESFERAS;
    tiempoCarrera = 0.0f;
    tiempoCierre = -1.0f;
    tiempoAnimacion = 0.0f;
    cooldownSonidoChoque = 0.0f;
}


void MinijuegoEsferasCanon::Reiniciar(
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

    int limite = LimiteJugadoresEsferas(cantidadMaxima);
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
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_ESFERAS_TERMINADO;
        return;
    }

    partidaValida = true;
    CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteEsferasCanonRetro3D());

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        EstadoJugadorEsferas& estado = estadosJugadores[indice];
        float lateral = ((float)k - (float)(cantidad - 1) * 0.5f) * 2.2f;

        estado.participa = true;
        PosicionarEnPistaEsferas(*this, estado, S_SALIDA_ESFERAS, lateral);
        estado.latPreferida = AleatorioEsferas(-1.2f, 1.2f);
        estado.arriesgado = GetRandomValue(1, 100) <= 40;
        estado.tiempoDecision = AleatorioEsferas(0.0f, 0.3f);

        ConfigurarJugadorMinijuegoEstandar(jugadores[indice], { estado.x, 2.7f, estado.z });
        jugadores[indice].direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugadores[indice].enSuelo = true;
    }

    // La camara arranca ya encuadrando la salida.
    camara.target = { 0.0f, ElevacionPistaEsferas(S_SALIDA_ESFERAS), -S_SALIDA_ESFERAS };
    camara.position = { 0.0f, camara.target.y + 8.0f, camara.target.z + 9.0f };
}


//==================================================
// ACTUALIZAR
//==================================================

static void ActualizarCamaraEsferas(MinijuegoEsferasCanon& minijuego, int limite, float deltaTime)
{
    float minimoS = 1.0e9f;
    float maximoS = -1.0e9f;
    float sumaX = 0.0f;
    float sumaZ = 0.0f;
    int contados = 0;

    for (int pasada = 0; pasada < 2 && contados == 0; pasada++)
    {
        for (int i = 0; i < limite; i++)
        {
            const EstadoJugadorEsferas& estado = minijuego.estadosJugadores[i];

            if (!estado.participa || (pasada == 0 && estado.terminado))
            {
                continue;
            }

            float x = estado.penalizacion > 0.0f ? estado.caidaX : estado.x;
            float z = estado.penalizacion > 0.0f ? estado.caidaZ : estado.z;

            if (estado.s < minimoS) minimoS = estado.s;
            if (estado.s > maximoS) maximoS = estado.s;
            sumaX += x;
            sumaZ += z;
            contados++;
        }
    }

    if (contados == 0)
    {
        return;
    }

    float dispersion = LimitarEsferas(maximoS - minimoS, 0.0f, 28.0f);
    float objetivoX = sumaX / (float)contados;
    float objetivoZ = sumaZ / (float)contados;
    float objetivoY = ElevacionPistaEsferas(0.5f * (minimoS + maximoS));
    float distancia = 8.0f + 0.55f * dispersion;

    float factor = 1.0f - std::exp(-4.0f * deltaTime);
    Vector3 objetivoCamara = { objetivoX, objetivoY + distancia, objetivoZ + distancia + 1.0f };
    Vector3 objetivoMirada = { objetivoX, objetivoY, objetivoZ };

    minijuego.camara.position.x += (objetivoCamara.x - minijuego.camara.position.x) * factor;
    minijuego.camara.position.y += (objetivoCamara.y - minijuego.camara.position.y) * factor;
    minijuego.camara.position.z += (objetivoCamara.z - minijuego.camara.position.z) * factor;
    minijuego.camara.target.x += (objetivoMirada.x - minijuego.camara.target.x) * factor;
    minijuego.camara.target.y += (objetivoMirada.y - minijuego.camara.target.y) * factor;
    minijuego.camara.target.z += (objetivoMirada.z - minijuego.camara.target.z) * factor;
}


void MinijuegoEsferasCanon::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_ESFERAS_TERMINADO ||
        resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int limite = LimiteJugadoresEsferas(cantidadMaxima);

    if (fase == FASE_ESFERAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_ESFERAS_JUGANDO;
        }
    }
    else
    {
        float restanteAntes = tiempoRestante;
        tiempoRestante -= deltaTime;
        ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);
        tiempoCarrera += deltaTime;

        if (cooldownSonidoChoque > 0.0f)
        {
            cooldownSonidoChoque -= deltaTime;
        }

        // Entrada una vez por cuadro; el movimiento se integra en 2 pasos.
        float direcciones[MAX_PARTICIPANTES][2]{};

        for (int i = 0; i < limite; i++)
        {
            EstadoJugadorEsferas& estado = estadosJugadores[i];

            if (!estado.participa)
            {
                continue;
            }

            bool bot = EsControladoPorBotEsferas(participantes[i]);
            InputMinijuegoParticipante entrada{};

            if (bot)
            {
                entrada = CrearEntradaBotEsferas(*this, i, deltaTime);
            }
            else
            {
                entrada = LeerInputMinijuegoParticipante(participantes[i]);
            }

            float dirX = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
            float dirZ = (entrada.atras ? 1.0f : 0.0f) - (entrada.adelante ? 1.0f : 0.0f);
            float longitud = std::sqrt(dirX * dirX + dirZ * dirZ);

            if (longitud > 0.01f)
            {
                dirX /= longitud;
                dirZ /= longitud;
            }

            if (estado.recargaImpulso > 0.0f)
            {
                estado.recargaImpulso -= deltaTime;
            }

            // Impulso corto con la accion (golpe) o el salto.
            if (
                (entrada.golpear || entrada.saltar) &&
                estado.recargaImpulso <= 0.0f &&
                estado.penalizacion <= 0.0f &&
                estado.tiempoAire <= 0.0f &&
                !estado.terminado
            )
            {
                float impulsoX = dirX;
                float impulsoZ = dirZ;

                if (longitud <= 0.01f)
                {
                    ProyeccionEsferas proyeccion = ProyectarPistaEsferas(*this, estado.x, estado.z);
                    impulsoX = proyeccion.tx;
                    impulsoZ = proyeccion.tz;
                }

                estado.velocidadX += impulsoX * IMPULSO_ESFERAS;
                estado.velocidadZ += impulsoZ * IMPULSO_ESFERAS;
                estado.recargaImpulso = RECARGA_IMPULSO_ESFERAS;

                if (!bot)
                {
                    ReproducirSonidoMinijuego(audio, SONIDO_SALTO);
                }
            }

            direcciones[i][0] = dirX;
            direcciones[i][1] = dirZ;
        }

        const int PASOS = 2;
        float paso = deltaTime / (float)PASOS;

        for (int p = 0; p < PASOS; p++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (estadosJugadores[i].participa)
                {
                    PasoEsferaEsferas(*this, i, direcciones[i][0], direcciones[i][1], paso);
                }
            }

            ResolverChoquesEsferas(*this, limite);
        }

        // Checkpoints y meta.
        bool todosTerminaron = true;

        for (int i = 0; i < limite; i++)
        {
            EstadoJugadorEsferas& estado = estadosJugadores[i];

            if (!estado.participa)
            {
                continue;
            }

            if (
                estado.checkpoints < CANTIDAD_CHECKPOINTS_ESFERAS &&
                estado.penalizacion <= 0.0f &&
                estado.s >= CHECKPOINTS_ESFERAS[estado.checkpoints]
            )
            {
                estado.checkpoints++;

                if (!EsControladoPorBotEsferas(participantes[i]))
                {
                    ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
                }
            }

            if (!estado.terminado && estado.s >= LONGITUD_META_ESFERAS)
            {
                estado.terminado = true;
                estado.tiempoMeta = tiempoCarrera;
                ReproducirSonidoMinijuego(audio, SONIDO_RECOGER_OBJETO);

                if (tiempoCierre < 0.0f)
                {
                    tiempoCierre = TIEMPO_CIERRE_ESFERAS;
                    hayGanadorPorMeta = true;
                }
            }

            if (!estado.terminado)
            {
                todosTerminaron = false;
            }
        }

        if (tiempoCierre >= 0.0f)
        {
            tiempoCierre -= deltaTime;
        }

        if (todosTerminaron || tiempoRestante <= 0.0f || (hayGanadorPorMeta && tiempoCierre <= 0.0f))
        {
            if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
            FinalizarEsferas(*this);
        }
    }

    // Sincroniza los jugadores (se dibujan sobre su esfera).
    ActualizarCamaraEsferas(*this, limite, deltaTime);

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorEsferas& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        float alturaAire = estado.tiempoAire > 0.0f
            ? ALTURA_SALTO_ESFERAS * std::sin(3.14159265f * (1.0f - estado.tiempoAire / TIEMPO_AIRE_ESFERAS))
            : 0.0f;
        float caida = 0.0f;

        if (estado.penalizacion > PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS)
        {
            float t = PENALIZACION_ESFERAS - estado.penalizacion;
            caida = t * t * 14.0f;
        }

        float x = estado.penalizacion > PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS ? estado.caidaX : estado.x;
        float z = estado.penalizacion > PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS ? estado.caidaZ : estado.z;
        float centroY = ElevacionPistaEsferas(estado.s) + RADIO_ESFERA + alturaAire - caida;

        JugadorPrueba& jugador = jugadores[i];
        jugador.posicion = { x, centroY + RADIO_ESFERA + 0.7f, z };
        jugador.velocidad = { estado.velocidadX, 0.0f, estado.velocidadZ };
        jugador.enSuelo = true;
        jugador.cayendo = false;

        float rapidez = std::sqrt(estado.velocidadX * estado.velocidadX + estado.velocidadZ * estado.velocidadZ);

        if (rapidez > 0.3f)
        {
            jugador.direccionMirada = { estado.velocidadX / rapidez, 0.0f, estado.velocidadZ / rapidez };
        }
    }
}


//==================================================
// VISUAL: ESCENARIO
//==================================================

static void DibujarCuadroEsferas(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, b, color);
    DrawTriangle3D(a, c, d, color);
    DrawTriangle3D(a, d, c, color);
}


// Punto del borde de la pista en el vertice i, usando la tangente promedio.
static Vector3 BordePistaEsferas(
    const MinijuegoEsferasCanon& minijuego,
    int i,
    float lateral,
    float altura
)
{
    int antes = i > 0 ? i - 1 : 0;
    int despues = i < PUNTOS_PISTA_ESFERAS - 1 ? i + 1 : PUNTOS_PISTA_ESFERAS - 1;
    float tx = minijuego.puntosX[despues] - minijuego.puntosX[antes];
    float tz = minijuego.puntosZ[despues] - minijuego.puntosZ[antes];
    float largo = std::sqrt(tx * tx + tz * tz);

    if (largo < 0.0001f)
    {
        tx = 0.0f;
        tz = -1.0f;
        largo = 1.0f;
    }

    tx /= largo;
    tz /= largo;

    return {
        minijuego.puntosX[i] - tz * lateral,
        ElevacionPistaEsferas(minijuego.puntosS[i]) + altura,
        minijuego.puntosZ[i] + tx * lateral
    };
}


static Vector3 PuntoSobrePistaEsferas(
    const MinijuegoEsferasCanon& minijuego,
    float s,
    float lateral,
    float altura
)
{
    float x = 0.0f;
    float z = 0.0f;
    float tx = 0.0f;
    float tz = 0.0f;

    PuntoPistaEsferas(minijuego, s, lateral, x, z, tx, tz);
    return { x, ElevacionPistaEsferas(s) + altura, z };
}


// En el fallback conservar la tangente promedio de los vertices originales;
// interpolar solo donde una frontera del paquete corta un segmento.
static Vector3 BordeFallbackEsferas(const MinijuegoEsferasCanon& minijuego,
    float s, int vertice, float lateral, float altura)
{
    if (std::fabs(s - minijuego.puntosS[vertice]) < 0.0001f)
        return BordePistaEsferas(minijuego, vertice, lateral, altura);
    return PuntoSobrePistaEsferas(minijuego, s, lateral, altura);
}


// Canon desertico: lecho seco, paredes estratificadas, grietas con puentes
// rotos, arena, rampa, cactus, rocas, banderas de checkpoint y meta.
// GLB globales ensamblados sin mover sus pivotes; fallback por pieza.
// (DrawCube), (DrawSphere) y (DrawCylinder) evitan los macros de sombras
// humanas de SombrasRetro: el canon desciende bajo Y=0 y su suelo no es plano.
static void DibujarPistaEsferas(const MinijuegoEsferasCanon& minijuego)
{
    int n = PUNTOS_PISTA_ESFERAS;
    float ancho = MITAD_ANCHO_PISTA_ESFERAS;
    const float limites[6] = {0,40,74,96,130,minijuego.longitudPista};
    bool pistas[5]{}, paredes[5][2]{};
    for (int tramo = 0; tramo < 4; tramo++)
    {
        pistas[tramo] = DibujarModeloEsferasCanonRetro3D((ModeloEsferasCanon3D)(tramo * 3), {0,0,0});
        for (int lado = 0; lado < 2; lado++)
            paredes[tramo][lado] = DibujarModeloEsferasCanonRetro3D(
                (ModeloEsferasCanon3D)(tramo * 3 + 1 + lado), {0,0,0});
    }
    // El arte acaba en S=130; conservar el resto de la pista logica (142.23).
    if (!DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_SUELO, {0,0,0}))
        (DrawCube)({0,-24,-65},110,0.5f,180,Color{152,83,62,255});

    // Lecho del rio seco.
    for (int i = 0; i < n - 1; i++)
    {
        for (int tramo = 0; tramo < 5; tramo++)
        {
            if (pistas[tramo]) continue;
            float s0 = std::fmax(minijuego.puntosS[i], limites[tramo]);
            float s1 = std::fmin(minijuego.puntosS[i + 1], limites[tramo + 1]);
            if (s1 <= s0) continue;
            Color color = i % 2 == 0 ? Color{ 196, 122, 80, 255 } : Color{ 186, 112, 72, 255 };
            DibujarCuadroEsferas(
                BordeFallbackEsferas(minijuego, s0, i, -ancho, 0.0f),
                BordeFallbackEsferas(minijuego, s0, i, ancho, 0.0f),
                BordeFallbackEsferas(minijuego, s1, i + 1, ancho, 0.0f),
                BordeFallbackEsferas(minijuego, s1, i + 1, -ancho, 0.0f),
                color
            );

            // Arena suelta.
            float medio = 0.5f * (s0 + s1);

            if (EnArenaEsferas(medio))
            {
                DibujarCuadroEsferas(
                    BordeFallbackEsferas(minijuego, s0, i, -ancho + 0.2f, 0.03f),
                    BordeFallbackEsferas(minijuego, s0, i, ancho - 0.2f, 0.03f),
                    BordeFallbackEsferas(minijuego, s1, i + 1, ancho - 0.2f, 0.03f),
                    BordeFallbackEsferas(minijuego, s1, i + 1, -ancho + 0.2f, 0.03f),
                    Color{ 238, 214, 150, 255 }
                );
            }

            // Grietas (oscuras) salvo el puente seguro.
            for (int g = 0; g < CANTIDAD_GRIETAS_ESFERAS; g++)
            {
                const GrietaEsferas& grieta = GRIETAS_ESFERAS[g];

                if (medio >= grieta.s0 - 0.6f && medio <= grieta.s1 + 0.6f)
                {
                    DibujarCuadroEsferas(
                        BordeFallbackEsferas(minijuego, s0, i, -ancho, 0.04f),
                        BordeFallbackEsferas(minijuego, s0, i, ancho, 0.04f),
                        BordeFallbackEsferas(minijuego, s1, i + 1, ancho, 0.04f),
                        BordeFallbackEsferas(minijuego, s1, i + 1, -ancho, 0.04f),
                        Color{ 18, 10, 10, 255 }
                    );
                }
            }
        }
    }

    // Puentes colgantes rotos sobre las grietas.
    for (int g = 0; g < CANTIDAD_GRIETAS_ESFERAS; g++)
    {
        const GrietaEsferas& grieta = GRIETAS_ESFERAS[g];
        if (DibujarModeloEsferasCanonRetro3D((ModeloEsferasCanon3D)(MODELO_ESFERAS_PUENTE_1 + g), {0,0,0})) continue;

        for (float s = grieta.s0; s <= grieta.s1; s += 0.7f)
        {
            Vector3 p = PuntoSobrePistaEsferas(minijuego, s, grieta.centroSeguro, 0.07f);
            (DrawCube)(p, grieta.mitadSegura * 2.0f, 0.1f, 0.5f, Color{ 128, 88, 50, 255 });
        }

        Vector3 izquierda = PuntoSobrePistaEsferas(minijuego, 0.5f * (grieta.s0 + grieta.s1), -ancho, 1.0f);
        Vector3 derecha = PuntoSobrePistaEsferas(minijuego, 0.5f * (grieta.s0 + grieta.s1), ancho, 1.0f);
        DrawLine3D(izquierda, derecha, Color{ 90, 64, 40, 255 });
    }

    // Paredes estratificadas.
    for (int i = 0; i < n - 1; i += 2)
    {
        int j = i + 2 < n ? i + 2 : n - 1;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            for (int tramo = 0; tramo < 5; tramo++)
            {
                if (paredes[tramo][lado < 0 ? 0 : 1]) continue;
                float s0 = std::fmax(minijuego.puntosS[i], limites[tramo]);
                float s1 = std::fmin(minijuego.puntosS[j], limites[tramo + 1]);
                if (s1 <= s0) continue;
                float l = (float)lado * (ancho + 0.2f);
                Vector3 a0 = BordeFallbackEsferas(minijuego, s0, i, l, -0.3f);
                Vector3 b0 = BordeFallbackEsferas(minijuego, s1, j, l, -0.3f);

                static const float ALTURAS[4] = { 0.0f, 2.4f, 4.4f, 7.0f };
                static const Color BANDAS[3] =
                {
                    Color{ 120, 52, 40, 255 }, Color{ 188, 96, 56, 255 }, Color{ 214, 160, 100, 255 }
                };

                for (int b = 0; b < 3; b++)
                {
                    DibujarCuadroEsferas(
                        { a0.x, a0.y + ALTURAS[b], a0.z },
                        { b0.x, b0.y + ALTURAS[b], b0.z },
                        { b0.x, b0.y + ALTURAS[b + 1], b0.z },
                        { a0.x, a0.y + ALTURAS[b + 1], a0.z },
                        BANDAS[b]
                    );
                }
            }
        }
    }

    // Rampa de atajo.
    if (!DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_RAMPA, {0,0,0}))
    {
        Vector3 a = PuntoSobrePistaEsferas(minijuego, RAMPA_S0_ESFERAS, RAMPA_LAT0_ESFERAS, 0.05f);
        Vector3 b = PuntoSobrePistaEsferas(minijuego, RAMPA_S0_ESFERAS, RAMPA_LAT1_ESFERAS, 0.05f);
        Vector3 c = PuntoSobrePistaEsferas(minijuego, RAMPA_S1_ESFERAS, RAMPA_LAT1_ESFERAS, 1.0f);
        Vector3 d = PuntoSobrePistaEsferas(minijuego, RAMPA_S1_ESFERAS, RAMPA_LAT0_ESFERAS, 1.0f);

        DibujarCuadroEsferas(a, b, c, d, Color{ 150, 100, 60, 255 });
        DrawLine3D(a, d, Color{ 255, 230, 120, 255 });
        DrawLine3D(b, c, Color{ 255, 230, 120, 255 });
    }

    // Banderas de checkpoint.
    for (int k = 0; k < CANTIDAD_CHECKPOINTS_ESFERAS; k++)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 base = PuntoSobrePistaEsferas(minijuego, CHECKPOINTS_ESFERAS[k], (float)lado * (ancho - 0.3f), 0.0f);
            float tx = 0, tz = 0, x = 0, z = 0;
            PuntoPistaEsferas(minijuego, CHECKPOINTS_ESFERAS[k], 0, x, z, tx, tz);
            // Raylib gira +X hacia -Z: el signo del yaw es opuesto al atan2
            // del visor. La bandera apunta hacia dentro en ambos lados.
            float yaw = -std::atan2(tx, -tz) * RAD2DEG + (lado > 0 ? 180.0f : 0.0f);
            if (DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_CHECKPOINT, base, yaw)) continue;
            (DrawCube)({ base.x, base.y + 1.6f, base.z }, 0.12f, 3.2f, 0.12f, Color{ 90, 60, 36, 255 });
            (DrawCube)({ base.x - (float)lado * 0.4f, base.y + 2.8f, base.z }, 0.8f, 0.6f, 0.05f, Color{ 90, 210, 120, 255 });
        }
    }

    // Salida y meta (cuadros blanco y negro) con arco.
    for (int m = 0; m < 2; m++)
    {
        float s = m == 0 ? S_SALIDA_ESFERAS - 1.2f : LONGITUD_META_ESFERAS;
        float tx = 0, tz = 0, x = 0, z = 0;
        PuntoPistaEsferas(minijuego, s, 0, x, z, tx, tz);
        if (DibujarModeloEsferasCanonRetro3D(m == 0 ? MODELO_ESFERAS_SALIDA : MODELO_ESFERAS_META,
            {x,ElevacionPistaEsferas(s),z}, -std::atan2(tx,-tz) * RAD2DEG)) continue;

        for (int c = 0; c < 8; c++)
        {
            Color color = c % 2 == 0 ? RAYWHITE : Color{ 30, 30, 34, 255 };
            float lateralA = -ancho + (float)c * ancho * 0.25f;
            DibujarCuadroEsferas(
                PuntoSobrePistaEsferas(minijuego, s, lateralA, 0.04f),
                PuntoSobrePistaEsferas(minijuego, s, lateralA + ancho * 0.25f, 0.04f),
                PuntoSobrePistaEsferas(minijuego, s + 0.8f, lateralA + ancho * 0.25f, 0.04f),
                PuntoSobrePistaEsferas(minijuego, s + 0.8f, lateralA, 0.04f),
                color
            );
        }

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 base = PuntoSobrePistaEsferas(minijuego, s, (float)lado * (ancho - 0.2f), 0.0f);
            (DrawCube)({ base.x, base.y + 2.4f, base.z }, 0.5f, 4.8f, 0.5f, Color{ 200, 150, 100, 255 });
        }

        Vector3 centro = PuntoSobrePistaEsferas(minijuego, s, 0.0f, 4.8f);
        (DrawCube)(centro, ancho * 2.2f, 0.6f, 0.5f, m == 0 ? Color{ 90, 210, 120, 255 } : Color{ 230, 60, 50, 255 });
    }

    // Cactus y rocas decorativas fuera de la pista, sobre las paredes.
    for (int i = 0; i < 18; i++)
    {
        float s = 4.0f + Ruido01Esferas(i, 5) * (LONGITUD_META_ESFERAS - 6.0f);
        float lado = i % 2 == 0 ? -1.0f : 1.0f;
        Vector3 p = PuntoSobrePistaEsferas(minijuego, s, lado * (ancho + 1.0f), 7.0f);
        if (DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_CACTUS, p, 0, {0,1,0},
            {0.65f,1.3f / 1.7f,0.65f})) continue;

        (DrawCylinder)({ p.x, p.y, p.z }, 0.2f, 0.2f, 1.3f, 6, Color{ 70, 130, 70, 255 });
        (DrawCylinder)({ p.x + 0.35f, p.y + 0.5f, p.z }, 0.12f, 0.12f, 0.6f, 6, Color{ 70, 130, 70, 255 });
    }

    // Mesas lejanas.
    for (int i = 0; i < 10; i++)
    {
        float lado = i % 2 == 0 ? -1.0f : 1.0f;
        float z = -(float)i * 14.0f;
        float altura = 8.0f + Ruido01Esferas(i, 9) * 8.0f;
        float x = lado * (20.0f + Ruido01Esferas(i, 8) * 10.0f);
        if (DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_MESA,
            {x,ElevacionPistaEsferas(-z) - 2.0f,z}, 0, {0,1,0}, {1,altura / 10.0f,1})) continue;

        (DrawCube)({ x, ElevacionPistaEsferas(-z) + altura * 0.5f - 2.0f, z }, 7.0f, altura, 6.0f, Color{ 172, 86, 58, 255 });
        (DrawCube)({ x, ElevacionPistaEsferas(-z) + altura - 1.7f, z }, 7.4f, 0.8f, 6.4f, Color{ 206, 130, 84, 255 });
    }

    // Arco natural cerca de la salida.
    if (!DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_ARCO_NATURAL, {0,0,0}))
    {
        Vector3 izquierda = PuntoSobrePistaEsferas(minijuego, 12.0f, -ancho - 0.6f, 0.0f);
        Vector3 derecha = PuntoSobrePistaEsferas(minijuego, 12.0f, ancho + 0.6f, 0.0f);
        (DrawCube)({ izquierda.x, izquierda.y + 2.8f, izquierda.z }, 1.6f, 5.6f, 1.6f, Color{ 150, 76, 52, 255 });
        (DrawCube)({ derecha.x, derecha.y + 2.8f, derecha.z }, 1.6f, 5.6f, 1.6f, Color{ 150, 76, 52, 255 });
        (DrawCube)({ 0.5f * (izquierda.x + derecha.x), izquierda.y + 5.9f, izquierda.z }, ancho * 2.0f + 3.4f, 1.0f, 1.8f, Color{ 170, 88, 58, 255 });
    }
}


static void DibujarObstaculoEsferas(const ObstaculoEsferas& obstaculo, float elevacion)
{
    float escala = obstaculo.cactus ? 1.0f : obstaculo.radio / 0.9f;
    if (DibujarModeloEsferasCanonRetro3D(obstaculo.cactus ? MODELO_ESFERAS_CACTUS : MODELO_ESFERAS_ROCA,
        {obstaculo.x,elevacion,obstaculo.z}, 0, {0,1,0}, {escala,escala,escala})) return;
    if (obstaculo.cactus)
    {
        Color verde = Color{ 62, 150, 78, 255 };

        (DrawCylinder)({ obstaculo.x, elevacion, obstaculo.z }, 0.35f, 0.35f, 1.6f, 8, verde);
        (DrawCylinder)({ obstaculo.x + 0.45f, elevacion + 0.7f, obstaculo.z }, 0.2f, 0.2f, 0.8f, 8, verde);
        (DrawCylinder)({ obstaculo.x - 0.45f, elevacion + 0.5f, obstaculo.z }, 0.2f, 0.2f, 0.7f, 8, verde);
        (DrawSphere)({ obstaculo.x, elevacion + 1.6f, obstaculo.z }, 0.35f, verde);
    }
    else
    {
        DrawSphereEx({ obstaculo.x, elevacion + obstaculo.radio * 0.8f, obstaculo.z }, obstaculo.radio, 6, 6, Color{ 126, 70, 54, 255 });
        DrawSphereWires({ obstaculo.x, elevacion + obstaculo.radio * 0.8f, obstaculo.z }, obstaculo.radio * 1.01f, 6, 6, Color{ 70, 40, 34, 255 });
    }
}


static void DibujarEsferaEsferas(
    const EstadoJugadorEsferas& estado,
    Vector3 centro,
    Color colorJugador
)
{
    if (!DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_ESFERA, centro,
        estado.giro * RAD2DEG, {estado.ejeX,0,estado.ejeZ}))
    {
        DrawSphereEx(centro, RADIO_ESFERA, 12, 12, Color{ 150, 134, 120, 255 });

        // Manchas que giran con la esfera (rotacion de Rodrigues).
        static const Vector3 BASE[6] =
        {
            { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
            { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }
        };

        float c = std::cos(estado.giro);
        float s = std::sin(estado.giro);

        for (int k = 0; k < 6; k++)
        {
            Vector3 v = BASE[k];
            float punto = estado.ejeX * v.x + estado.ejeZ * v.z;
            Vector3 cruz = { -estado.ejeZ * v.y, estado.ejeZ * v.x - estado.ejeX * v.z, estado.ejeX * v.y };
            Vector3 r =
            {
                v.x * c + cruz.x * s + estado.ejeX * punto * (1.0f - c),
                v.y * c + cruz.y * s,
                v.z * c + cruz.z * s + estado.ejeZ * punto * (1.0f - c)
            };

            (DrawSphere)(
                { centro.x + r.x * RADIO_ESFERA * 0.96f, centro.y + r.y * RADIO_ESFERA * 0.96f, centro.z + r.z * RADIO_ESFERA * 0.96f },
                0.2f,
                Color{ 84, 72, 64, 255 }
            );
        }
    }
    if (!DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_ARO,
        {centro.x,centro.y - RADIO_ESFERA,centro.z}, 0, {0,1,0}, {1,1,1}, Fade(colorJugador,0.7f)))
        DrawCircle3D({ centro.x, centro.y - RADIO_ESFERA + 0.03f, centro.z }, RADIO_ESFERA + 0.15f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(colorJugador, 0.7f));
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoEsferasCanon::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteJugadoresEsferas(cantidadMaxima);

    ClearBackground(Color{ 236, 176, 124, 255 });
    BeginMode3D(camara);

    DibujarPistaEsferas(*this);

    for (int k = 0; k < cantidadObstaculos; k++)
    {
        DibujarObstaculoEsferas(obstaculos[k], ElevacionPistaEsferas(obstaculos[k].s));
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorEsferas& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];
        bool cayendo = estado.penalizacion > PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS;

        // Tras reaparecer parpadea hasta que termina la penalizacion.
        if (!cayendo && estado.penalizacion > 0.0f && std::sin(tiempoAnimacion * 24.0f) < 0.0f)
        {
            continue;
        }

        Vector3 centro = { jugador.posicion.x, jugador.posicion.y - 0.7f - RADIO_ESFERA, jugador.posicion.z };
        Color colorJugador = participantes[i].color;

        DibujarEsferaEsferas(estado, centro, colorJugador);

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(jugador, participanteVisual);

        if (mostrarDebug)
        {
            DrawSphereWires(centro, RADIO_ESFERA, 8, 8, LIME);
        }
    }

    EndMode3D();

    // HUD: tiempo y barra de progreso.
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int barraX = ancho / 2 - 220;

    DrawRectangle(barraX - 20, 10, 480, 70, Fade(BLACK, 0.78f));

    const char* titulo = TextFormat("ESFERAS DEL CANON  -  %.0f s", tiempoRestante);
    DrawText(titulo, ancho / 2 - MeasureText(titulo, 20) / 2, 14, 20, RAYWHITE);

    DrawRectangle(barraX, 52, 440, 6, Color{ 80, 70, 64, 255 });

    for (int k = 0; k < CANTIDAD_CHECKPOINTS_ESFERAS; k++)
    {
        DrawRectangle(barraX + (int)(440.0f * CHECKPOINTS_ESFERAS[k] / LONGITUD_META_ESFERAS), 48, 2, 14, Color{ 90, 210, 120, 255 });
    }

    DrawRectangle(barraX + 438, 46, 4, 18, RAYWHITE);

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorEsferas& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        float proporcion = LimitarEsferas(estado.progresoMaximo / LONGITUD_META_ESFERAS, 0.0f, 1.0f);
        DrawCircle(barraX + (int)(440.0f * proporcion), 55 + (i % 2 == 0 ? -1 : 1), 6.0f, participantes[i].color);
        DrawCircleLines(barraX + (int)(440.0f * proporcion), 55 + (i % 2 == 0 ? -1 : 1), 6.0f, BLACK);
    }

    if (hayGanadorPorMeta && tiempoCierre >= 0.0f && fase == FASE_ESFERAS_JUGANDO)
    {
        const char* cierre = TextFormat("LLEGO EL PRIMERO  -  QUEDAN %.0f s", tiempoCierre);
        DrawText(cierre, ancho / 2 - MeasureText(cierre, 16) / 2, 62, 16, ORANGE);
    }

    // HUD: estado y controles de cada jugador.
    int lineasJugadores = 0;
    int anchoHud = 170;
    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].participa) lineasJugadores++;
        if (estadosJugadores[i].participa && !EsControladoPorBotEsferas(participantes[i])) anchoHud = 560;
    }
    if (lineasJugadores > 0)
    {
        DrawRectangle(8, alto - 30 - (lineasJugadores - 1) * 24 - 8, anchoHud, lineasJugadores * 24 + 10, Fade(BLACK, 0.72f));
    }
    int y = alto - 30;

    for (int i = limite - 1; i >= 0; i--)
    {
        const EstadoJugadorEsferas& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        const char* impulso = estado.recargaImpulso > 0.0f
            ? TextFormat("IMPULSO %.1f s", estado.recargaImpulso)
            : "IMPULSO LISTO";

        const char* linea = EsControladoPorBotEsferas(participantes[i])
            ? TextFormat("J%d BOT  %.0f%%", participantes[i].numeroJugador, 100.0f * LimitarEsferas(estado.progresoMaximo / LONGITUD_META_ESFERAS, 0.0f, 1.0f))
            : TextFormat(
                "J%d  MOVER LA ESFERA  IMPULSO [%s]  %s  %.0f%%",
                participantes[i].numeroJugador,
                TextoBotonImpulsoEsferas(participantes[i]),
                impulso,
                100.0f * LimitarEsferas(estado.progresoMaximo / LONGITUD_META_ESFERAS, 0.0f, 1.0f)
            );

        DrawText(linea, 18, y, 18, participantes[i].color);
        y -= 24;
    }

    if (fase == FASE_ESFERAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, ancho / 2 - MeasureText(texto, 90) / 2, alto / 2 - 50, 90, YELLOW);

        const char* pista = "RUEDA HASTA LA META  -  PUENTES: SOLO POR LA FRANJA DE TABLONES  -  RAMPA DERECHA: ATAJO";
        DrawText(pista, ancho / 2 - MeasureText(pista, 18) / 2, alto / 2 + 60, 18, RAYWHITE);
    }
    else if (
        fase == FASE_ESFERAS_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        DrawRectangle(ancho / 2 - 300, alto / 2 - 150, 600, 300, Fade(BLACK, 0.92f));
        DrawText("RESULTADO", ancho / 2 - MeasureText("RESULTADO", 32) / 2, alto / 2 - 130, 32, RAYWHITE);

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (!resultado.participantes[i].participo || resultado.participantes[i].posicionFinal != posicion)
                {
                    continue;
                }

                const char* fila = estadosJugadores[i].terminado
                    ? TextFormat("%d.  J%d  %.1f s", posicion, participantes[i].numeroJugador, estadosJugadores[i].tiempoMeta)
                    : TextFormat("%d.  J%d  %d%%", posicion, participantes[i].numeroJugador, (int)(100.0f * estadosJugadores[i].progresoMaximo / LONGITUD_META_ESFERAS));

                DrawText(fila, ancho / 2 - 120, alto / 2 - 80 + (posicion - 1) * 34, 26, participantes[i].color);
            }
        }

        DrawText(TextoReinicioMinijuego(), ancho / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2, alto / 2 + 120, 18, LIGHTGRAY);
    }
    else if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        const char* texto = "SE NECESITAN AL MENOS 2 JUGADORES";
        DrawText(texto, ancho / 2 - MeasureText(texto, 26) / 2, alto / 2, 26, RED);
    }
}


const ResultadoMinijuego& MinijuegoEsferasCanon::ObtenerResultado() const
{
    return resultado;
}
