#include "Minigames/MinijuegoEsferasCanon.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HorneadoMallas.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <vector>


// El canon se dibuja con primitivas: del paquete GLB solo se cargan las
// piezas jugables (esfera, aro del jugador y los obstaculos).
static const ModeloEsferasCanon3D PIEZAS_JUGABLES_ESFERAS[] =
{
    MODELO_ESFERAS_CACTUS,
    MODELO_ESFERAS_ROCA,
    MODELO_ESFERAS_ESFERA,
    MODELO_ESFERAS_ARO
};


static void CargarPiezasJugablesEsferas()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteEsferasCanonRetro3D();

    for (ModeloEsferasCanon3D pieza : PIEZAS_JUGABLES_ESFERAS)
    {
        PaqueteModelosEscenarioRetro3D una = { paquete.recursos + pieza, paquete.definiciones + pieza, 1 };
        CargarPaqueteModelosEscenarioRetro3D(una);
    }
}


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

    for (int i = 0; i < MAX_PARTICULAS_ESFERAS; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        visualJugadores[i] = {};
    }

    semillaVisual = 0x5EEDu;
    tiempoVisual = 0.0f;
    anuncioInicio = -1.0f;
    anuncioMeta = -1.0f;
    jugadorMeta = -1;
    confetiFinal = false;
}


static void LiberarMallasHornoE();


void MinijuegoEsferasCanon::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    // Cada partida vuelve a hornear el canon: no quedan mallas de la anterior.
    LiberarMallasHornoE();
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
    CargarPiezasJugablesEsferas();

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


void MinijuegoEsferasCanon::ActualizarReglas(
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
// VISUAL: PALETA, LUZ Y NIEBLA
//==================================================
//
// Canon de roca roja a la hora dorada: el sol bajo entra por la izquierda,
// las paredes del lado derecho se encienden de naranja y las del izquierdo
// quedan en sombra violacea; las sombras se estiran hacia la camara.
// Todo se dibuja con rlgl (sin DrawCube/DrawSphere): las primitivas de
// raylib quedan solo para el respaldo de las piezas GLB jugables.

static const Color COLOR_CIELO_ALTO_E = { 62, 70, 134, 255 };
static const Color COLOR_CIELO_MEDIO_E = { 222, 124, 112, 255 };
static const Color COLOR_CIELO_BAJO_E = { 255, 196, 118, 255 };
static const Color COLOR_NIEBLA_E = { 238, 166, 122, 255 };
static const Color COLOR_SOL_E = { 255, 242, 196, 255 };

static const Color COLOR_ROCA_E = { 184, 80, 50, 255 };
static const Color COLOR_ROCA_OSCURA_E = { 134, 52, 40, 255 };
static const Color COLOR_ROCA_CLARA_E = { 220, 132, 82, 255 };
static const Color COLOR_CREMA_E = { 236, 190, 134, 255 };
static const Color COLOR_CORNISA_E = { 206, 128, 82, 255 };
static const Color COLOR_SUELO_E = { 190, 118, 78, 255 };
static const Color COLOR_ARENA_E = { 240, 204, 142, 255 };
static const Color COLOR_MADERA_E = { 156, 102, 60, 255 };
static const Color COLOR_MADERA_OSCURA_E = { 96, 60, 40, 255 };
static const Color COLOR_CACTUS_E = { 94, 140, 84, 255 };
static const Color COLOR_TURQUESA_E = { 52, 204, 190, 255 };
static const Color COLOR_SOMBRA_E = { 88, 50, 98, 255 };
static const Color COLOR_ABISMO_E = { 26, 12, 20, 255 };

// Hacia el sol (normalizado a mano: |v| = 1).
static const Vector3 SOL_E = { -0.618f, 0.358f, -0.700f };
// Direccion de las sombras largas sobre el suelo (opuesta al sol, en XZ).
static const float SOMBRA_X_E = 0.662f;
static const float SOMBRA_Z_E = 0.749f;

// Perfil del canon (en laterales y alturas sobre el lecho).
static const float PASO_CANON_E = 2.5f;
static const float S_INICIO_CANON_E = -36.0f;
static const int MUESTRAS_CANON_E = 112;
static const float S_ARCO_E = 134.0f;

static Vector3 g_ojoE = { 0.0f, 10.0f, 10.0f };
static Vector3 g_frenteUE = { 1.0f, 0.0f, 0.0f };
static Vector3 g_frenteVE = { 0.0f, 1.0f, 0.0f };


static float Limitar01E(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static unsigned char CanalE(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoE(Color color, float factor)
{
    return Color{
        CanalE((float)color.r * factor),
        CanalE((float)color.g * factor),
        CanalE((float)color.b * factor),
        color.a
    };
}


static Color MezclaE(Color a, Color b, float t)
{
    t = Limitar01E(t);

    return Color{
        CanalE((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalE((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalE((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalE((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaE(Color color, float alfa)
{
    color.a = CanalE(255.0f * alfa);
    return color;
}


static float SuaveE(float x, int semilla)
{
    float base = std::floor(x);
    float f = x - base;
    f = f * f * (3.0f - 2.0f * f);
    float a = Ruido01Esferas((int)base + 1000, semilla);
    float b = Ruido01Esferas((int)base + 1001, semilla);
    return a + (b - a) * f;
}


static float Suave2DE(float x, float z, int semilla)
{
    float bx = std::floor(x), bz = std::floor(z);
    float fx = x - bx, fz = z - bz;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fz = fz * fz * (3.0f - 2.0f * fz);
    int ix = (int)bx, iz = (int)bz;
    float a = Ruido01Esferas(ix * 73 + iz * 9157, semilla);
    float b = Ruido01Esferas((ix + 1) * 73 + iz * 9157, semilla);
    float c = Ruido01Esferas(ix * 73 + (iz + 1) * 9157, semilla);
    float d = Ruido01Esferas((ix + 1) * 73 + (iz + 1) * 9157, semilla);
    float ab = a + (b - a) * fx;
    float cd = c + (d - c) * fx;
    return ab + (cd - ab) * fz;
}


static Vector3 SumaE(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaE(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaE(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
static float PuntoE(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }


static Vector3 NormalizarE(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 0.0001f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static Vector3 CruzE(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}


// Calima dorada: lo lejano se funde con el durazno del horizonte.
static float NieblaE(Vector3 p)
{
    float dx = p.x - g_ojoE.x;
    float dy = p.y - g_ojoE.y;
    float dz = p.z - g_ojoE.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 30.0f) / 230.0f;
    return n < 0.0f ? 0.0f : (n > 0.9f ? 0.9f : n);
}


// Horneado: lo estatico del canon se graba una vez en mallas (la niebla la
// pone el shader por pixel). Mientras se graba, VerticeE junta triangulos
// en lugar de mandarlos a rlgl.
struct GrabacionE
{
    GrabadorMalla malla;
    int modo = RL_TRIANGLES;
    int enQuad = 0;
    Vector3 quad[4]{};
    Color quadColor[4]{};
};

static GrabacionE g_grabacionE;


static void BeginE(int modo)
{
    if (g_grabacionE.malla.activa)
    {
        g_grabacionE.modo = modo;
        g_grabacionE.enQuad = 0;
        return;
    }

    rlBegin(modo);
}


static void EndE()
{
    if (!g_grabacionE.malla.activa)
    {
        rlEnd();
    }
}


static void GrabarVerticeE(Vector3 p, Color c)
{
    GrabarVerticeMalla(g_grabacionE.malla, p, c);
}


static Color TinteE(Color color, Vector3 p, float emisivo = 0.0f)
{
    if (g_grabacionE.malla.activa)
    {
        return color;
    }

    Color niebla = COLOR_NIEBLA_E;
    niebla.a = color.a;
    return MezclaE(color, niebla, NieblaE(p) * (1.0f - 0.6f * emisivo));
}


// Luz de la tarde: caras al sol doradas, caras en sombra hacia el violeta.
static Color IluminarE(Color c, Vector3 n)
{
    float d = PuntoE(n, SOL_E);
    float luz = 0.60f + 0.46f * Limitar01E(d) + 0.10f * Limitar01E(n.y);
    Color r = TonoE(c, luz);

    if (d > 0.0f)
    {
        r = MezclaE(r, Color{ 255, 210, 150, 255 }, 0.18f * d);
    }
    else
    {
        r = MezclaE(r, MezclaE(COLOR_SOMBRA_E, c, 0.45f), 0.26f * Limitar01E(-d * 1.5f));
    }

    r.a = c.a;
    return r;
}


static void VerticeE(Vector3 p, Color color)
{
    if (g_grabacionE.malla.activa)
    {
        if (g_grabacionE.modo == RL_QUADS)
        {
            g_grabacionE.quad[g_grabacionE.enQuad] = p;
            g_grabacionE.quadColor[g_grabacionE.enQuad] = color;

            if (++g_grabacionE.enQuad == 4)
            {
                g_grabacionE.enQuad = 0;
                static const int ORDEN[6] = { 0, 1, 2, 0, 2, 3 };
                for (int k : ORDEN) GrabarVerticeE(g_grabacionE.quad[k], g_grabacionE.quadColor[k]);
            }
        }
        else
        {
            GrabarVerticeE(p, color);
        }

        return;
    }

    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadE(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeE(a, color);
    VerticeE(b, color);
    VerticeE(c, color);
    VerticeE(d, color);
}


static void QuadNieblaE(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color, float emisivo = 0.0f)
{
    VerticeE(a, TinteE(color, a, emisivo));
    VerticeE(b, TinteE(color, b, emisivo));
    VerticeE(c, TinteE(color, c, emisivo));
    VerticeE(d, TinteE(color, d, emisivo));
}


// Cara iluminada segun su normal; haciaFuera solo orienta la normal.
static void QuadLuzE(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color, Vector3 haciaFuera, bool niebla = true)
{
    Vector3 n = NormalizarE(CruzE(RestaE(b, a), RestaE(d, a)));
    if (PuntoE(n, haciaFuera) < 0.0f) n = EscalaE(n, -1.0f);
    Color cara = IluminarE(color, n);

    if (niebla) QuadNieblaE(a, b, c, d, cara);
    else QuadE(a, b, c, d, cara);
}


// Caja orientada por tres ejes (u, v, w unitarios) con medias medidas.
// Cada cara se ilumina por su normal: volumen sin texturas.
static void CajaEjesE(Vector3 c, Vector3 u, Vector3 v, Vector3 w, Vector3 media, Color color, bool niebla = true, float emisivo = 0.0f)
{
    Vector3 U = EscalaE(u, media.x);
    Vector3 V = EscalaE(v, media.y);
    Vector3 W = EscalaE(w, media.z);
    Vector3 p[8];

    for (int k = 0; k < 8; k++)
    {
        Vector3 q = c;
        q = SumaE(q, (k & 1) ? U : EscalaE(U, -1.0f));
        q = SumaE(q, (k & 2) ? V : EscalaE(V, -1.0f));
        q = SumaE(q, (k & 4) ? W : EscalaE(W, -1.0f));
        p[k] = q;
    }

    static const int CARAS[6][4] =
    {
        { 2, 3, 7, 6 }, { 0, 1, 5, 4 },   // +v, -v
        { 1, 3, 7, 5 }, { 0, 2, 6, 4 },   // +u, -u
        { 4, 5, 7, 6 }, { 0, 1, 3, 2 }    // +w, -w
    };
    Vector3 normales[6] = { v, EscalaE(v, -1.0f), u, EscalaE(u, -1.0f), w, EscalaE(w, -1.0f) };

    BeginE(RL_QUADS);

    for (int f = 0; f < 6; f++)
    {
        Color cara = IluminarE(color, normales[f]);
        cara = MezclaE(cara, color, emisivo);
        cara.a = color.a;
        if (niebla) cara = TinteE(cara, c, emisivo);
        QuadE(p[CARAS[f][0]], p[CARAS[f][1]], p[CARAS[f][2]], p[CARAS[f][3]], cara);
    }

    EndE();
}


// Caja girada alrededor de Y (radianes).
static void CajaE(Vector3 c, Vector3 t, float giro, Color color, bool niebla = true, float emisivo = 0.0f)
{
    float cs = std::cos(giro);
    float sn = std::sin(giro);
    CajaEjesE(c, { cs, 0.0f, -sn }, { 0.0f, 1.0f, 0.0f }, { sn, 0.0f, cs },
        { t.x * 0.5f, t.y * 0.5f, t.z * 0.5f }, color, niebla, emisivo);
}


// Viga entre dos puntos (seccion cuadrada de lado grosor).
static void VigaE(Vector3 a, Vector3 b, float grosor, Color color, bool niebla = true)
{
    Vector3 eje = RestaE(b, a);
    float largo = std::sqrt(PuntoE(eje, eje));
    if (largo < 0.0001f) return;
    Vector3 w = EscalaE(eje, 1.0f / largo);
    Vector3 apoyo = std::fabs(w.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarE(CruzE(apoyo, w));
    Vector3 v = CruzE(w, u);
    CajaEjesE(EscalaE(SumaE(a, b), 0.5f), u, v, w, { grosor * 0.5f, grosor * 0.5f, largo * 0.5f }, color, niebla);
}


// Tronco de cono o prisma entre dos puntos.
static void PrismaE(Vector3 a, Vector3 b, float r0, float r1, int lados, Color color, bool niebla = true, float giro = 0.0f, float emisivo = 0.0f)
{
    Vector3 eje = NormalizarE(RestaE(b, a));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarE(CruzE(eje, apoyo));
    Vector3 v = CruzE(eje, u);
    float paso = 6.2832f / (float)lados;
    Color tapa = IluminarE(color, eje);
    tapa = MezclaE(tapa, color, emisivo);
    if (niebla) tapa = TinteE(tapa, b, emisivo);

    BeginE(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaE(EscalaE(u, std::cos(a0)), EscalaE(v, std::sin(a0)));
        Vector3 d1 = SumaE(EscalaE(u, std::cos(a1)), EscalaE(v, std::sin(a1)));
        Vector3 dm = SumaE(EscalaE(u, std::cos(am)), EscalaE(v, std::sin(am)));
        Color cara = MezclaE(IluminarE(color, dm), color, emisivo);
        cara.a = color.a;

        Vector3 p0 = SumaE(a, EscalaE(d0, r0));
        Vector3 p1 = SumaE(a, EscalaE(d1, r0));
        Vector3 p2 = SumaE(b, EscalaE(d1, r1));
        Vector3 p3 = SumaE(b, EscalaE(d0, r1));

        if (niebla) QuadNieblaE(p0, p1, p2, p3, cara, emisivo);
        else QuadE(p0, p1, p2, p3, cara);

        if (r1 > 0.001f)
        {
            QuadE(b, p2, p3, b, tapa);
        }
    }

    EndE();
}


// Roca facetada: esfera de pocos lados con radios irregulares.
static void RocaE(Vector3 c, Vector3 r, int semilla, Color color, float irregular = 0.16f, bool niebla = true)
{
    const int ANILLOS = 4;
    const int LADOS = 7;

    BeginE(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        for (int k = 0; k < LADOS; k++)
        {
            int aa[4] = { a, a, a + 1, a + 1 };
            int kk[4] = { k, k + 1, k + 1, k };

            for (int q = 0; q < 4; q++)
            {
                float th = -1.5708f + 3.1416f * (float)aa[q] / (float)ANILLOS;
                float ph = 6.2832f * (float)kk[q] / (float)LADOS + (float)semilla;
                float f = (aa[q] == 0 || aa[q] == ANILLOS)
                    ? 1.0f - irregular * 0.3f
                    : 1.0f - irregular + 2.0f * irregular * Ruido01Esferas(aa[q] * 31 + (kk[q] % LADOS), semilla);
                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = { c.x + n.x * r.x * f, c.y + n.y * r.y * f, c.z + n.z * r.z * f };
                Color cv = IluminarE(TonoE(color, 0.94f + 0.12f * Ruido01Esferas(aa[q] * 7 + kk[q], semilla + 5)), n);
                VerticeE(p, niebla ? TinteE(cv, p) : cv);
            }
        }
    }

    EndE();
}


// Disco con borde que se apaga (u y v definen el plano). Sirve para
// sombras suaves (mezcla normal) y para charcos de luz (aditivo).
static void DiscoE(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f, float bordeAlfa = 0.0f)
{
    const int LADOS = 18;
    unsigned char borde = CanalE((float)color.a * bordeAlfa);
    BeginE(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        Color exterior = { color.r, color.g, color.b, borde };
        VerticeE(centro, color);
        VerticeE({ centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0 }, exterior);
        VerticeE({ centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1 }, exterior);
    }

    EndE();
}


static void DiscoSueloE(Vector3 c, float radio, Color color)
{
    DiscoE(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void DiscoFrenteE(Vector3 c, float radio, Color color)
{
    DiscoE(c, g_frenteUE, g_frenteVE, radio, color);
}


// Sombra larga de la tarde: elipse que nace al pie y se estira lejos del sol.
static void SombraLargaE(Vector3 pie, float ancho, float alto, float alfa)
{
    float largo = ancho * 0.5f + alto * 1.3f;
    Vector3 u = { SOMBRA_X_E, 0.0f, SOMBRA_Z_E };
    Vector3 v = { -SOMBRA_Z_E, 0.0f, SOMBRA_X_E };
    Vector3 centro = { pie.x + u.x * largo * 0.55f, pie.y + 0.02f, pie.z + u.z * largo * 0.55f };
    DiscoE(centro, u, v, ancho * 0.5f, Color{ 70, 34, 52, CanalE(255.0f * alfa) }, largo / (ancho * 0.5f) * 0.6f, 0.15f);
}


// Cono de luz (aditivo): del foco hacia el charco, se apaga en la base.
static void ConoLuzE(Vector3 foco, Vector3 baseCentro, float radio, Color color)
{
    const int LADOS = 14;
    Vector3 eje = NormalizarE(RestaE(baseCentro, foco));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarE(CruzE(eje, apoyo));
    Vector3 v = CruzE(eje, u);

    BeginE(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaE(baseCentro, SumaE(EscalaE(u, std::cos(a0) * radio), EscalaE(v, std::sin(a0) * radio)));
        Vector3 p1 = SumaE(baseCentro, SumaE(EscalaE(u, std::cos(a1) * radio), EscalaE(v, std::sin(a1) * radio)));

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(foco.x, foco.y, foco.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p1.x, p1.y, p1.z);
    }
    EndE();
}


static void TrianguloE(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    BeginE(RL_TRIANGLES);
    VerticeE(a, color);
    VerticeE(b, color);
    VerticeE(c, color);
    EndE();
}


// Cuadradito que mira a la camara (particulas).
static void QuadFrenteE(Vector3 c, float r, Color color)
{
    Vector3 u = EscalaE(g_frenteUE, r);
    Vector3 v = EscalaE(g_frenteVE, r);
    QuadE(
        RestaE(RestaE(c, u), v),
        RestaE(SumaE(c, u), v),
        SumaE(SumaE(c, u), v),
        SumaE(RestaE(c, u), v),
        color
    );
}


//==================================================
// VISUAL: MARCOS DE LA PISTA
//==================================================

struct MarcoPistaE
{
    Vector3 c{};       // centro sobre el lecho
    float tx = 0.0f;   // tangente
    float tz = -1.0f;
    float nx = 1.0f;   // lateral derecho (el mismo signo que la logica)
    float nz = 0.0f;
};


static void TangenteVerticeE(const MinijuegoEsferasCanon& m, int i, float& tx, float& tz)
{
    int a = i > 0 ? i - 1 : 0;
    int b = i < PUNTOS_PISTA_ESFERAS - 1 ? i + 1 : PUNTOS_PISTA_ESFERAS - 1;
    tx = m.puntosX[b] - m.puntosX[a];
    tz = m.puntosZ[b] - m.puntosZ[a];
    float largo = std::sqrt(tx * tx + tz * tz);

    if (largo < 0.0001f)
    {
        tx = 0.0f;
        tz = -1.0f;
        return;
    }

    tx /= largo;
    tz /= largo;
}


// Centro igual al de la logica (interpolacion lineal entre puntos) y
// lateral suavizado por vertice; fuera de la pista se prolonga en recta.
static MarcoPistaE MarcoPistaCanonE(const MinijuegoEsferasCanon& m, float s)
{
    MarcoPistaE f;
    const int ultimo = PUNTOS_PISTA_ESFERAS - 1;

    if (s <= 0.0f)
    {
        TangenteVerticeE(m, 0, f.tx, f.tz);
        f.c = { m.puntosX[0] + f.tx * s, 0.0f, m.puntosZ[0] + f.tz * s };
    }
    else if (s >= m.longitudPista)
    {
        TangenteVerticeE(m, ultimo, f.tx, f.tz);
        float extra = s - m.longitudPista;
        f.c = { m.puntosX[ultimo] + f.tx * extra, 0.0f, m.puntosZ[ultimo] + f.tz * extra };
    }
    else
    {
        int i = 0;

        while (i < PUNTOS_PISTA_ESFERAS - 2 && m.puntosS[i + 1] < s)
        {
            i++;
        }

        float largo = m.puntosS[i + 1] - m.puntosS[i];
        float t = largo > 0.0001f ? Limitar01E((s - m.puntosS[i]) / largo) : 0.0f;
        float ax = 0.0f, az = 0.0f, bx = 0.0f, bz = 0.0f;

        TangenteVerticeE(m, i, ax, az);
        TangenteVerticeE(m, i + 1, bx, bz);
        f.tx = ax + (bx - ax) * t;
        f.tz = az + (bz - az) * t;
        float l = std::sqrt(f.tx * f.tx + f.tz * f.tz);
        if (l < 0.0001f) l = 1.0f;
        f.tx /= l;
        f.tz /= l;
        f.c = {
            m.puntosX[i] + (m.puntosX[i + 1] - m.puntosX[i]) * t,
            0.0f,
            m.puntosZ[i] + (m.puntosZ[i + 1] - m.puntosZ[i]) * t
        };
    }

    f.c.y = ElevacionPistaEsferas(s);
    f.nx = -f.tz;
    f.nz = f.tx;
    return f;
}


static Vector3 EnPistaE(const MarcoPistaE& f, float lateral, float altura)
{
    return { f.c.x + f.nx * lateral, f.c.y + altura, f.c.z + f.nz * lateral };
}


static Vector3 PuntoCanonE(const MinijuegoEsferasCanon& m, float s, float lateral, float altura)
{
    return EnPistaE(MarcoPistaCanonE(m, s), lateral, altura);
}


static Vector3 TangenteE(const MarcoPistaE& f) { return { f.tx, 0.0f, f.tz }; }
static Vector3 LateralE(const MarcoPistaE& f) { return { f.nx, 0.0f, f.nz }; }


static float SMuestraE(int k)
{
    return S_INICIO_CANON_E + PASO_CANON_E * (float)k;
}


// Alturas de las terrazas a cada lado (lado 0 izquierda, 1 derecha).
static float AlturaTerrazaE(int k, int lado, int nivel)
{
    float x = (float)k * 0.31f;

    if (nivel == 1) return 2.3f + 0.8f * SuaveE(x, 11 + lado * 7);
    if (nivel == 2) return AlturaTerrazaE(k, lado, 1) + 2.3f + 1.0f * SuaveE(x * 0.8f, 23 + lado * 7);
    return 9.3f + 1.2f * SuaveE(x * 0.6f, 37 + lado * 7);
}


// Laterales del borde de cada terraza (sin signo).
static float LateralTerrazaE(int k, int lado, int nivel)
{
    float x = (float)k * 0.37f;

    if (nivel == 1) return 6.9f + 0.9f * SuaveE(x, 51 + lado * 5);
    if (nivel == 2) return 9.7f + 1.1f * SuaveE(x * 0.7f, 61 + lado * 5);
    return 12.2f + 1.0f * SuaveE(x * 0.6f, 71 + lado * 5);
}


struct CanonCacheE
{
    MarcoPistaE marcos[MUESTRAS_CANON_E];
    float compresion[2][MUESTRAS_CANON_E];
};


// En las curvas cerradas el lado interior se comprime para que las
// terrazas no se crucen sobre si mismas.
static void PrepararCanonE(const MinijuegoEsferasCanon& m, CanonCacheE& cache)
{
    for (int k = 0; k < MUESTRAS_CANON_E; k++)
    {
        cache.marcos[k] = MarcoPistaCanonE(m, SMuestraE(k));
    }

    float crudo[2][MUESTRAS_CANON_E];

    for (int k = 0; k < MUESTRAS_CANON_E; k++)
    {
        int a = k > 0 ? k - 1 : 0;
        int b = k < MUESTRAS_CANON_E - 1 ? k + 1 : MUESTRAS_CANON_E - 1;
        const MarcoPistaE& f = cache.marcos[k];
        float dtx = cache.marcos[b].tx - cache.marcos[a].tx;
        float dtz = cache.marcos[b].tz - cache.marcos[a].tz;
        float curvatura = (dtx * f.nx + dtz * f.nz) / (PASO_CANON_E * (float)(b - a > 0 ? b - a : 1));

        for (int lado = 0; lado < 2; lado++)
        {
            float signo = lado == 0 ? -1.0f : 1.0f;
            float interior = curvatura * signo;
            float limite = interior > 0.001f ? 0.78f / interior : 1000.0f;
            float escala = (limite - 4.9f) / (13.5f - 4.9f);
            crudo[lado][k] = escala > 1.0f ? 1.0f : (escala < 0.25f ? 0.25f : escala);
        }
    }

    for (int lado = 0; lado < 2; lado++)
    {
        for (int k = 0; k < MUESTRAS_CANON_E; k++)
        {
            float minimo = 1.0f;

            for (int d = -3; d <= 3; d++)
            {
                int j = k + d;
                if (j < 0 || j >= MUESTRAS_CANON_E) continue;
                if (crudo[lado][j] < minimo) minimo = crudo[lado][j];
            }

            cache.compresion[lado][k] = minimo;
        }
    }
}


// Punto del perfil de la pared: lateral sin signo desde el centro.
static Vector3 PuntoParedE(const CanonCacheE& cache, int k, int lado, float lateral, float altura)
{
    float signo = lado == 0 ? -1.0f : 1.0f;
    float l = lateral;

    if (l > 4.9f)
    {
        l = 4.9f + (l - 4.9f) * cache.compresion[lado][k];
    }

    return EnPistaE(cache.marcos[k], signo * l, altura);
}


//==================================================
// VISUAL: CIELO Y HORIZONTE
//==================================================

static float HorizontePantallaE(const Camera3D& vista)
{
    Vector2 h = GetWorldToScreen({ vista.position.x, vista.position.y, vista.position.z - 4000.0f }, vista);
    return h.y;
}


// Cielo de la hora dorada: violeta arriba, coral y oro sobre el horizonte,
// sol bajo con su halo y vetas de nubes encendidas que derivan.
static void DibujarCieloE(const Camera3D& vista, float t, bool decoracion)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float horizonte = HorizontePantallaE(vista);
    float arriba = horizonte - (float)alto * 0.95f;
    float medio = arriba + (horizonte - arriba) * 0.55f;

    DrawRectangleGradientV(0, (int)arriba, ancho, (int)(medio - arriba) + 1, COLOR_CIELO_ALTO_E, COLOR_CIELO_MEDIO_E);
    DrawRectangleGradientV(0, (int)medio, ancho, (int)(horizonte - medio) + 1, COLOR_CIELO_MEDIO_E, COLOR_CIELO_BAJO_E);
    DrawRectangleGradientV(0, (int)horizonte, ancho, alto, COLOR_CIELO_BAJO_E, COLOR_NIEBLA_E);

    if (arriba > 0.0f)
    {
        DrawRectangle(0, 0, ancho, (int)arriba + 1, COLOR_CIELO_ALTO_E);
    }

    // Sol bajo (puede quedar fuera del cuadro: su halo igual tiñe el cielo).
    Vector3 frente = NormalizarE(RestaE(vista.target, vista.position));

    if (PuntoE(frente, SOL_E) > 0.05f)
    {
        Vector3 lejos = SumaE(vista.position, EscalaE(SOL_E, 3000.0f));
        Vector2 p = GetWorldToScreen(lejos, vista);
        float r = Px(46.0f);

        DrawCircleGradient(p, r * 7.0f, Color{ 255, 200, 130, 70 }, Color{ 255, 170, 110, 0 });
        DrawCircleGradient(p, r * 3.0f, Color{ 255, 226, 160, 120 }, Color{ 255, 200, 130, 0 });
        DrawCircleV(p, r, COLOR_SOL_E);
    }

    if (!decoracion)
    {
        return;
    }

    // Vetas de nubes finas, panza encendida por el sol.
    for (int k = 0; k < 9; k++)
    {
        float anchoNube = (float)ancho * (0.20f + 0.22f * Ruido01Esferas(k, 411));
        float deriva = std::fmod(Ruido01Esferas(k, 412) * (float)ancho * 1.5f + t * (3.0f + 4.0f * Ruido01Esferas(k, 413)), (float)ancho * 1.5f) - (float)ancho * 0.25f;
        float y = horizonte - (float)alto * (0.06f + 0.42f * Ruido01Esferas(k, 414));
        float altoNube = Px(7.0f + 9.0f * Ruido01Esferas(k, 415));
        float u = Limitar01E((horizonte - y) / ((float)alto * 0.5f));
        Color cuerpo = MezclaE(Color{ 255, 176, 140, 255 }, Color{ 196, 110, 150, 255 }, u);
        DrawEllipse((int)deriva, (int)y, anchoNube * 0.5f, altoNube, AlfaE(cuerpo, 0.55f));
        DrawEllipse((int)(deriva - anchoNube * 0.1f), (int)(y + altoNube * 0.4f), anchoNube * 0.36f, altoNube * 0.45f, AlfaE(Color{ 255, 222, 160, 255 }, 0.55f));
    }
}


// Mesa o butte: tronco de caja con talud y cornisa clara.
static void MesaE(Vector3 base, float ancho, float fondo, float alto, float giro, Color color, int semilla, bool niebla, bool estratos)
{
    float cs = std::cos(giro);
    float sn = std::sin(giro);
    Vector3 u = { cs, 0.0f, -sn };
    Vector3 w = { sn, 0.0f, cs };
    Vector3 arriba = { 0.0f, 1.0f, 0.0f };

    // Talud de cascajo al pie.
    float talud = alto * 0.28f;
    int capas = estratos ? 3 : 1;
    float y0 = base.y;
    float a0 = ancho * 1.35f, f0 = fondo * 1.35f;
    float yTalud = base.y + talud;
    float a1 = ancho, f1 = fondo;

    BeginE(RL_QUADS);

    auto Anillo = [&](float y, float a, float f, int esquina) -> Vector3
    {
        float sx = (esquina == 0 || esquina == 3) ? -1.0f : 1.0f;
        float sz = (esquina < 2) ? -1.0f : 1.0f;
        return SumaE({ base.x, y, base.z }, SumaE(EscalaE(u, sx * a * 0.5f), EscalaE(w, sz * f * 0.5f)));
    };

    auto Tramo = [&](float ya, float aa, float fa, float yb, float ab, float fb, Color c)
    {
        for (int e = 0; e < 4; e++)
        {
            int e1 = (e + 1) % 4;
            Vector3 p0 = Anillo(ya, aa, fa, e);
            Vector3 p1 = Anillo(ya, aa, fa, e1);
            Vector3 p2 = Anillo(yb, ab, fb, e1);
            Vector3 p3 = Anillo(yb, ab, fb, e);
            Vector3 medio = EscalaE(SumaE(p0, p1), 0.5f);
            Vector3 fuera = { medio.x - base.x, 0.2f, medio.z - base.z };
            Vector3 n = NormalizarE(CruzE(RestaE(p1, p0), RestaE(p3, p0)));
            if (PuntoE(n, fuera) < 0.0f) n = EscalaE(n, -1.0f);
            Color cara = IluminarE(c, n);
            if (niebla) QuadNieblaE(p0, p1, p2, p3, cara);
            else QuadE(p0, p1, p2, p3, cara);
        }
    };

    Tramo(y0, a0, f0, yTalud, a1, f1, TonoE(color, 0.92f));

    float y = yTalud;
    float resto = alto - talud;

    for (int c = 0; c < capas; c++)
    {
        float yb = y + resto / (float)capas;
        float encoge = 0.97f - 0.03f * Ruido01Esferas(c, semilla);
        Color banda = c % 2 == 0 ? color : MezclaE(color, COLOR_CREMA_E, 0.28f);
        Tramo(y, a1, f1, yb, a1 * encoge, f1 * encoge, banda);
        a1 *= encoge;
        f1 *= encoge;
        y = yb;
    }

    // Techo plano con borde claro.
    Color techo = IluminarE(MezclaE(color, COLOR_CREMA_E, 0.2f), arriba);
    Vector3 t0 = Anillo(y, a1, f1, 0), t1 = Anillo(y, a1, f1, 1), t2 = Anillo(y, a1, f1, 2), t3 = Anillo(y, a1, f1, 3);
    if (niebla) QuadNieblaE(t0, t1, t2, t3, techo);
    else QuadE(t0, t1, t2, t3, techo);

    EndE();
}


// Horizonte en dos planos: mesetas rojizas en la calima y, detras,
// siluetas lilas casi fundidas con el cielo.
static void DibujarHorizonteE(float yLejos, bool decoracion, bool completo)
{
    // Llanura lejana hasta el horizonte.
    Color llano = MezclaE(COLOR_CORNISA_E, COLOR_NIEBLA_E, 0.55f);
    BeginE(RL_QUADS);
    QuadNieblaE({ -900.0f, yLejos, -200.0f }, { 900.0f, yLejos, -200.0f }, { 900.0f, yLejos, -950.0f }, { -900.0f, yLejos, -950.0f }, llano);
    EndE();

    if (!decoracion)
    {
        return;
    }

    // Plano lejano: siluetas lilas.
    if (completo)
    {
        for (int k = 0; k < 11; k++)
        {
            float x = -820.0f + 164.0f * (float)k + 60.0f * (Ruido01Esferas(k, 501) - 0.5f);
            float z = -760.0f - 90.0f * Ruido01Esferas(k, 502);
            float alto = 46.0f + 50.0f * Ruido01Esferas(k, 503);
            float ancho = 90.0f + 110.0f * Ruido01Esferas(k, 504);
            Color c = MezclaE(Color{ 176, 104, 128, 255 }, COLOR_CIELO_BAJO_E, 0.42f + 0.12f * Ruido01Esferas(k, 505));
            MesaE({ x, yLejos - 4.0f, z }, ancho, ancho * 0.5f, alto, 0.2f * Ruido01Esferas(k, 506), c, k, false, false);
        }
    }

    // Plano cercano: mesetas y buttes rojizos en la calima.
    for (int k = 0; k < 13; k++)
    {
        float x = -520.0f + 87.0f * (float)k + 40.0f * (Ruido01Esferas(k, 511) - 0.5f);
        float z = -400.0f - 110.0f * Ruido01Esferas(k, 512);
        float alto = 24.0f + 34.0f * Ruido01Esferas(k, 513);
        float ancho = 30.0f + 70.0f * Ruido01Esferas(k, 514);
        if (std::fabs(x) < 60.0f) { alto *= 0.6f; }
        Color c = MezclaE(COLOR_ROCA_E, COLOR_NIEBLA_E, 0.5f + 0.1f * Ruido01Esferas(k, 515));
        MesaE({ x, yLejos - 2.0f, z }, ancho, ancho * 0.6f, alto, 0.4f * Ruido01Esferas(k, 516), c, k + 40, false, true);
    }
}


//==================================================
// VISUAL: PAREDES ESTRATIFICADAS Y MESETA
//==================================================

static const int PUNTOS_PERFIL_E = 14;


// Perfil escalonado de un lado del canon en la muestra k: tres terrazas
// con estratos (cada banda algo entrada o salida) y la cornisa de la meseta.
static void PerfilParedE(const CanonCacheE& cache, int k, int lado, Vector3 p[PUNTOS_PERFIL_E])
{
    float h1 = AlturaTerrazaE(k, lado, 1);
    float h2 = AlturaTerrazaE(k, lado, 2);
    float h3 = AlturaTerrazaE(k, lado, 3);
    float l1 = LateralTerrazaE(k, lado, 1);
    float l2 = LateralTerrazaE(k, lado, 2);
    int semilla = 300 + lado * 17;

    auto Jitter = [&](int banda) -> float
    {
        return (Ruido01Esferas(k * 13 + banda, semilla) - 0.5f) * 0.34f;
    };

    float base = 4.45f + 0.10f * Ruido01Esferas(k, semilla + 1);
    p[0] = PuntoParedE(cache, k, lado, base, -0.5f);

    static const float FR1[3] = { 0.36f, 0.70f, 1.0f };
    for (int b = 0; b < 3; b++)
    {
        float lat = base + 0.5f * FR1[b] + Jitter(b) * 0.6f;
        if (lat < 4.45f) lat = 4.45f;
        p[1 + b] = PuntoParedE(cache, k, lado, lat, h1 * FR1[b]);
    }

    p[4] = PuntoParedE(cache, k, lado, l1, h1);

    static const float FR2[3] = { 0.33f, 0.66f, 1.0f };
    for (int b = 0; b < 3; b++)
    {
        p[5 + b] = PuntoParedE(cache, k, lado, l1 + 0.8f * FR2[b] + Jitter(3 + b), h1 + (h2 - h1) * FR2[b]);
    }

    p[8] = PuntoParedE(cache, k, lado, l2, h2);

    static const float FR3[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    for (int b = 0; b < 4; b++)
    {
        p[9 + b] = PuntoParedE(cache, k, lado, l2 + 1.3f * FR3[b] + Jitter(6 + b), h2 + (h3 - h2) * FR3[b]);
    }

    p[13] = PuntoParedE(cache, k, lado, l2 + 1.9f, h3);
}


static Color ColorSegmentoParedE(int segmento)
{
    static const Color COLORES[PUNTOS_PERFIL_E - 1] =
    {
        { 134, 52, 40, 255 },   // pie oscuro
        { 184, 80, 50, 255 },
        { 220, 132, 82, 255 },
        { 206, 128, 82, 255 },  // cornisa 1
        { 176, 72, 46, 255 },
        { 230, 168, 112, 255 }, // veta crema
        { 214, 118, 72, 255 },
        { 206, 128, 82, 255 },  // cornisa 2
        { 140, 56, 42, 255 },
        { 190, 86, 52, 255 },
        { 236, 182, 124, 255 }, // veta crema
        { 212, 112, 66, 255 },
        { 210, 136, 88, 255 }   // borde de la meseta
    };

    return COLORES[segmento];
}


static void DibujarParedesCanonE(const CanonCacheE& cache, int k0, int k1, bool decoracion)
{
    Vector3 perfiles[2][2][PUNTOS_PERFIL_E];

    for (int lado = 0; lado < 2; lado++)
    {
        float signo = lado == 0 ? -1.0f : 1.0f;
        PerfilParedE(cache, k0, lado, perfiles[lado][0]);

        for (int k = k0; k < k1; k++)
        {
            Vector3* a = perfiles[lado][(k - k0) & 1];
            Vector3* b = perfiles[lado][(k - k0 + 1) & 1];
            PerfilParedE(cache, k + 1, lado, b);

            const MarcoPistaE& f = cache.marcos[k];
            Vector3 haciaDentro = { -signo * f.nx, 0.55f, -signo * f.nz };
            float variacion = 0.95f + 0.10f * Ruido01Esferas(k, 700 + lado);

            BeginE(RL_QUADS);

            for (int j = 0; j < PUNTOS_PERFIL_E - 1; j++)
            {
                Color c = TonoE(ColorSegmentoParedE(j), variacion);
                QuadLuzE(a[j], b[j], b[j + 1], a[j + 1], c, haciaDentro);
            }

            // Meseta hacia el horizonte (en X puro: no se cruza en las curvas).
            // Franjas con manchas de tierra mas clara y mas rojiza.
            static const float FRANJAS[6] = { 0.0f, 6.0f, 14.0f, 26.0f, 60.0f, 420.0f };
            Vector3 ea = a[PUNTOS_PERFIL_E - 1];
            Vector3 eb = b[PUNTOS_PERFIL_E - 1];
            Color mesetaBase = IluminarE(COLOR_CORNISA_E, { 0.0f, 1.0f, 0.0f });

            for (int q = 0; q < 5; q++)
            {
                Vector3 p[4] =
                {
                    { ea.x + signo * FRANJAS[q], ea.y, ea.z },
                    { eb.x + signo * FRANJAS[q], eb.y, eb.z },
                    { eb.x + signo * FRANJAS[q + 1], eb.y, eb.z },
                    { ea.x + signo * FRANJAS[q + 1], ea.y, ea.z }
                };

                for (int v = 0; v < 4; v++)
                {
                    Color meseta = TonoE(mesetaBase, 0.84f + 0.22f * Suave2DE(p[v].x * 0.07f, p[v].z * 0.07f, 715));
                    meseta = MezclaE(meseta, Color{ 222, 160, 108, 255 }, 0.35f * Suave2DE(p[v].x * 0.035f + 9.0f, p[v].z * 0.035f, 741));
                    VerticeE(p[v], TinteE(meseta, p[v]));
                }
            }

            EndE();

            // Grietas verticales oscuras en la cara de la segunda terraza.
            if (decoracion && Ruido01Esferas(k, 720 + lado) < 0.3f)
            {
                Vector3 p0 = a[4];
                Vector3 p1 = a[7];
                Vector3 off = { -signo * f.nx * 0.03f, 0.0f, -signo * f.nz * 0.03f };
                Vector3 t = { f.tx * 0.07f, 0.0f, f.tz * 0.07f };
                BeginE(RL_QUADS);
                QuadNieblaE(SumaE(SumaE(p0, off), t), SumaE(RestaE(p0, t), off), SumaE(RestaE(p1, t), off), SumaE(SumaE(p1, t), off), Color{ 96, 38, 40, 255 });
                EndE();
            }
        }
    }
}


// Cierre del fondo: el canon dobla fuera de la vista.
static void DibujarCierreCanonE(const CanonCacheE& cache)
{
    int k = MUESTRAS_CANON_E - 1;
    const MarcoPistaE& f = cache.marcos[k];
    float h = 0.5f * (AlturaTerrazaE(k, 0, 3) + AlturaTerrazaE(k, 1, 3));
    Vector3 a = EnPistaE(f, -16.0f, -0.6f);
    Vector3 b = EnPistaE(f, 16.0f, -0.6f);
    Vector3 c = EnPistaE(f, 16.0f, h);
    Vector3 d = EnPistaE(f, -16.0f, h);
    Vector3 hacia = { -f.tx, 0.0f, -f.tz };

    BeginE(RL_QUADS);
    QuadLuzE(a, b, c, d, COLOR_ROCA_E, hacia);
    EndE();
}


//==================================================
// VISUAL: LECHO DEL RIO SECO (ARENA JUGABLE)
//==================================================

static bool EnGrietaVisualE(float s)
{
    for (int g = 0; g < CANTIDAD_GRIETAS_ESFERAS; g++)
    {
        if (s > GRIETAS_ESFERAS[g].s0 && s < GRIETAS_ESFERAS[g].s1) return true;
    }

    return false;
}


// Color del lecho en un vertice: la pared izquierda tapa el sol bajo y
// deja una franja de sombra violacea junto a ella.
static Color ColorLechoE(Color base, float lateral)
{
    float sombra = Limitar01E((-1.4f - lateral) / 2.6f);
    Color c = TonoE(base, 1.0f - 0.16f * sombra);
    return MezclaE(c, COLOR_SOMBRA_E, 0.20f * sombra);
}


static void TiraLechoE(const MarcoPistaE& fa, const MarcoPistaE& fb, int k, float altura, bool niebla)
{
    const int TIRAS = 8;
    const float MITAD = 4.6f;

    BeginE(RL_QUADS);

    for (int t = 0; t < TIRAS; t++)
    {
        float l0 = -MITAD + 2.0f * MITAD * (float)t / (float)TIRAS;
        float l1 = -MITAD + 2.0f * MITAD * (float)(t + 1) / (float)TIRAS;
        float brillo = 0.92f + 0.14f * Ruido01Esferas(k * 9 + t, 811);
        if (t == 0 || t == TIRAS - 1) brillo *= 0.86f;
        Color base = TonoE(COLOR_SUELO_E, brillo);
        Vector3 a = EnPistaE(fa, l0, altura);
        Vector3 b = EnPistaE(fa, l1, altura);
        Vector3 c = EnPistaE(fb, l1, altura);
        Vector3 d = EnPistaE(fb, l0, altura);
        Color ca = ColorLechoE(base, l0);
        Color cb = ColorLechoE(base, l1);

        if (niebla)
        {
            VerticeE(a, TinteE(ca, a));
            VerticeE(b, TinteE(cb, b));
            VerticeE(c, TinteE(cb, c));
            VerticeE(d, TinteE(ca, d));
        }
        else
        {
            VerticeE(a, ca);
            VerticeE(b, cb);
            VerticeE(c, cb);
            VerticeE(d, ca);
        }
    }

    EndE();
}


// Trazo fino sobre el lecho (craquelado, lineas pintadas).
static void TrazoLechoE(const MinijuegoEsferasCanon& m, float sa, float la, float sb, float lb, float ancho, float altura, Color color)
{
    Vector3 a = PuntoCanonE(m, sa, la, altura);
    Vector3 b = PuntoCanonE(m, sb, lb, altura);
    float dx = b.x - a.x, dz = b.z - a.z;
    float l = std::sqrt(dx * dx + dz * dz);
    if (l < 0.0001f) return;
    float nx = -dz / l * ancho * 0.5f, nz = dx / l * ancho * 0.5f;

    BeginE(RL_QUADS);
    QuadNieblaE({ a.x + nx, a.y, a.z + nz }, { b.x + nx, b.y, b.z + nz }, { b.x - nx, b.y, b.z - nz }, { a.x - nx, a.y, a.z - nz }, color);
    EndE();
}


static void DibujarLechoE(const MinijuegoEsferasCanon& m, const CanonCacheE& cache, int k0, int k1, bool decoracion)
{
    for (int k = k0; k < k1; k++)
    {
        float sa = SMuestraE(k);
        float sb = SMuestraE(k + 1);

        // Tramos del muestreo, cortados en los bordes de las grietas.
        float cortes[8];
        int n = 0;
        cortes[n++] = sa;

        for (int g = 0; g < CANTIDAD_GRIETAS_ESFERAS; g++)
        {
            if (GRIETAS_ESFERAS[g].s0 > sa && GRIETAS_ESFERAS[g].s0 < sb) cortes[n++] = GRIETAS_ESFERAS[g].s0;
            if (GRIETAS_ESFERAS[g].s1 > sa && GRIETAS_ESFERAS[g].s1 < sb) cortes[n++] = GRIETAS_ESFERAS[g].s1;
        }

        cortes[n++] = sb;

        for (int c = 0; c < n - 1; c++)
        {
            float medio = 0.5f * (cortes[c] + cortes[c + 1]);
            if (EnGrietaVisualE(medio)) continue;

            MarcoPistaE fa = c == 0 ? cache.marcos[k] : MarcoPistaCanonE(m, cortes[c]);
            MarcoPistaE fb = c == n - 2 ? cache.marcos[k + 1] : MarcoPistaCanonE(m, cortes[c + 1]);
            TiraLechoE(fa, fb, k, 0.0f, true);
        }

        if (!decoracion || sb < -4.0f || sa > m.longitudPista + 20.0f)
        {
            continue;
        }

        // Barro seco craquelado y piedritas (fuera de grietas y arena).
        for (int q = 0; q < 2; q++)
        {
            float s = sa + PASO_CANON_E * Ruido01Esferas(k * 5 + q, 821);
            float l = -3.6f + 7.2f * Ruido01Esferas(k * 5 + q, 822);
            Color linea = TonoE(COLOR_SUELO_E, 0.70f);

            for (int paso = 0; paso < 3; paso++)
            {
                float s2 = s + 0.5f + 0.4f * Ruido01Esferas(k * 17 + q * 3 + paso, 823);
                float l2 = l + (Ruido01Esferas(k * 17 + q * 3 + paso, 824) - 0.5f) * 1.3f;
                if (l2 > 4.2f) l2 = 4.2f;
                if (l2 < -4.2f) l2 = -4.2f;

                if (!EnGrietaVisualE(s) && !EnGrietaVisualE(s2) && !EnArenaEsferas(s) && !EnArenaEsferas(s2))
                {
                    TrazoLechoE(m, s, l, s2, l2, 0.07f, 0.012f, linea);
                }

                s = s2;
                l = l2;
            }
        }

        for (int q = 0; q < 3; q++)
        {
            float s = sa + PASO_CANON_E * Ruido01Esferas(k * 7 + q, 831);
            float l = -4.1f + 8.2f * Ruido01Esferas(k * 7 + q, 832);
            if (EnGrietaVisualE(s) || EnArenaEsferas(s)) continue;
            Vector3 p = PuntoCanonE(m, s, l, 0.04f);
            float tam = 0.10f + 0.12f * Ruido01Esferas(k * 7 + q, 833);
            Color c = q == 0 ? COLOR_CREMA_E : (q == 1 ? COLOR_ROCA_CLARA_E : COLOR_ROCA_OSCURA_E);
            CajaE(p, { tam * 1.4f, tam * 0.6f, tam }, 6.0f * Ruido01Esferas(k * 7 + q, 834), c);
        }
    }
}


// Arena suelta: ondas del viento, bordes difuminados y montoncitos junto a
// las paredes. Coincide con las zonas que frenan.
static void DibujarArenasE(const MinijuegoEsferasCanon& m, float sMin, float sMax, bool decoracion)
{
    for (int z = 0; z < CANTIDAD_ARENAS_ESFERAS; z++)
    {
        float s0 = ARENAS_ESFERAS[z].s0;
        float s1 = ARENAS_ESFERAS[z].s1;
        if (s1 < sMin || s0 > sMax) continue;

        const float BORDE = 0.6f;
        const float PASO = 0.4f;
        const int TIRAS = 8;
        float inicio = s0 - BORDE;
        float fin = s1 + BORDE;
        int pasos = (int)((fin - inicio) / PASO) + 1;

        MarcoPistaE fa = MarcoPistaCanonE(m, inicio);

        for (int p = 0; p < pasos; p++)
        {
            float sa = inicio + PASO * (float)p;
            float sb = sa + PASO;
            if (sb > fin) sb = fin;
            MarcoPistaE fb = MarcoPistaCanonE(m, sb);

            float alfaA = Limitar01E((sa - inicio) / BORDE) * Limitar01E((fin - sa) / BORDE);
            float alfaB = Limitar01E((sb - inicio) / BORDE) * Limitar01E((fin - sb) / BORDE);

            BeginE(RL_QUADS);

            for (int t = 0; t < TIRAS; t++)
            {
                float l0 = -4.5f + 9.0f * (float)t / (float)TIRAS;
                float l1 = -4.5f + 9.0f * (float)(t + 1) / (float)TIRAS;
                float ondaA0 = 0.93f + 0.09f * std::sin(sa * 7.0f + 1.4f * std::sin(l0 * 0.9f));
                float ondaA1 = 0.93f + 0.09f * std::sin(sa * 7.0f + 1.4f * std::sin(l1 * 0.9f));
                float ondaB0 = 0.93f + 0.09f * std::sin(sb * 7.0f + 1.4f * std::sin(l0 * 0.9f));
                float ondaB1 = 0.93f + 0.09f * std::sin(sb * 7.0f + 1.4f * std::sin(l1 * 0.9f));
                Vector3 a = EnPistaE(fa, l0, 0.03f);
                Vector3 b = EnPistaE(fa, l1, 0.03f);
                Vector3 c = EnPistaE(fb, l1, 0.03f);
                Vector3 d = EnPistaE(fb, l0, 0.03f);

                VerticeE(a, TinteE(AlfaE(ColorLechoE(TonoE(COLOR_ARENA_E, ondaA0), l0), alfaA), a));
                VerticeE(b, TinteE(AlfaE(ColorLechoE(TonoE(COLOR_ARENA_E, ondaA1), l1), alfaA), b));
                VerticeE(c, TinteE(AlfaE(ColorLechoE(TonoE(COLOR_ARENA_E, ondaB1), l1), alfaB), c));
                VerticeE(d, TinteE(AlfaE(ColorLechoE(TonoE(COLOR_ARENA_E, ondaB0), l0), alfaB), d));
            }

            EndE();
            fa = fb;
        }

        if (!decoracion)
        {
            continue;
        }

        // Huellas del viento: vetas mas claras que cruzan la arena.
        for (int q = 0; q < 4; q++)
        {
            float s = s0 + (s1 - s0) * (0.15f + 0.22f * (float)q);
            float l = -3.4f + 1.2f * Ruido01Esferas(z * 7 + q, 842);
            TrazoLechoE(m, s, l, s + 0.5f, l + 2.6f + 1.4f * Ruido01Esferas(z * 7 + q, 843), 0.12f, 0.045f, AlfaE(Color{ 252, 228, 176, 255 }, 0.7f));
        }
    }
}


//==================================================
// VISUAL: GRIETAS Y PUENTES ROTOS
//==================================================

// Cara vertical hacia el abismo con degrade de la roca a la oscuridad.
static void CaraAbismoE(Vector3 arribaA, Vector3 arribaB, float profundidad, Color roca)
{
    // El canto se ve apenas: enseguida la grieta se hunde en la oscuridad
    // (la calima casi no la aclara, para que el peligro se lea de lejos).
    const float CORTES[4] = { 0.0f, 0.05f, 0.2f, 1.0f };
    Color tonos[4] =
    {
        TonoE(roca, 0.8f),
        Color{ 72, 28, 34, 255 },
        Color{ 36, 14, 24, 255 },
        COLOR_ABISMO_E
    };

    BeginE(RL_QUADS);

    for (int b = 0; b < 3; b++)
    {
        Vector3 a0 = { arribaA.x, arribaA.y - profundidad * CORTES[b], arribaA.z };
        Vector3 b0 = { arribaB.x, arribaB.y - profundidad * CORTES[b], arribaB.z };
        Vector3 a1 = { arribaA.x, arribaA.y - profundidad * CORTES[b + 1], arribaA.z };
        Vector3 b1 = { arribaB.x, arribaB.y - profundidad * CORTES[b + 1], arribaB.z };
        VerticeE(a0, TinteE(tonos[b], a0, 0.8f));
        VerticeE(b0, TinteE(tonos[b], b0, 0.8f));
        VerticeE(b1, TinteE(tonos[b + 1], b1, 0.8f));
        VerticeE(a1, TinteE(tonos[b + 1], a1, 0.8f));
    }

    EndE();
}


// MODELO FUTURO: puente colgante roto (franja segura de tablones + restos).
// Tabla de puente colgando hacia el vacio, con balanceo.
static void TablaColganteE(Vector3 anclaje, Vector3 lateral, Vector3 haciaAbismo, float largo, float balanceo, Color color)
{
    Vector3 abajo = NormalizarE(SumaE({ 0.0f, -1.0f, 0.0f }, EscalaE(haciaAbismo, 0.35f + balanceo)));
    Vector3 centro = SumaE(anclaje, EscalaE(abajo, largo * 0.5f));
    Vector3 grosor = NormalizarE(CruzE(abajo, lateral));
    CajaEjesE(centro, lateral, grosor, abajo, { 0.17f, 0.035f, largo * 0.5f }, color);
}


static void DibujarGrietasE(const MinijuegoEsferasCanon& m, float t, float sMin, float sMax, bool decoracion)
{
    const float PROFUNDIDAD = 10.0f;
    const float MITAD = 4.6f;

    for (int g = 0; g < CANTIDAD_GRIETAS_ESFERAS; g++)
    {
        const GrietaEsferas& grieta = GRIETAS_ESFERAS[g];
        if (grieta.s1 < sMin || grieta.s0 > sMax) continue;

        MarcoPistaE f0 = MarcoPistaCanonE(m, grieta.s0);
        MarcoPistaE f1 = MarcoPistaCanonE(m, grieta.s1);

        // Labios (cara lejana, cercana y costados) y fondo.
        for (int tira = 0; tira < 4; tira++)
        {
            float l0 = -MITAD + 2.0f * MITAD * (float)tira / 4.0f;
            float l1 = -MITAD + 2.0f * MITAD * (float)(tira + 1) / 4.0f;
            Color roca = TonoE(COLOR_ROCA_E, 0.85f + 0.1f * Ruido01Esferas(g * 4 + tira, 851));
            CaraAbismoE(EnPistaE(f1, l0, 0.0f), EnPistaE(f1, l1, 0.0f), PROFUNDIDAD, roca);
            CaraAbismoE(EnPistaE(f0, l0, 0.0f), EnPistaE(f0, l1, 0.0f), PROFUNDIDAD, TonoE(roca, 0.7f));
        }

        for (int lado = -1; lado <= 1; lado += 2)
        {
            for (int q = 0; q < 3; q++)
            {
                float sa = grieta.s0 + (grieta.s1 - grieta.s0) * (float)q / 3.0f;
                float sb = grieta.s0 + (grieta.s1 - grieta.s0) * (float)(q + 1) / 3.0f;
                Color roca = lado < 0 ? TonoE(COLOR_ROCA_E, 0.62f) : COLOR_ROCA_CLARA_E;
                CaraAbismoE(PuntoCanonE(m, sa, (float)lado * MITAD, 0.0f), PuntoCanonE(m, sb, (float)lado * MITAD, 0.0f), PROFUNDIDAD, roca);
            }
        }

        BeginE(RL_QUADS);
        QuadE(EnPistaE(f0, -MITAD, -PROFUNDIDAD), EnPistaE(f0, MITAD, -PROFUNDIDAD), EnPistaE(f1, MITAD, -PROFUNDIDAD), EnPistaE(f1, -MITAD, -PROFUNDIDAD), COLOR_ABISMO_E);
        EndE();

        // Estratos que asoman en la cara lejana.
        for (int e = 0; e < 2; e++)
        {
            float y = -0.55f - 1.1f * (float)e;
            Vector3 a = EnPistaE(f1, -MITAD, y);
            Vector3 b = EnPistaE(f1, MITAD, y);
            Vector3 c = EnPistaE(f1, MITAD, y - 0.18f);
            Vector3 d = EnPistaE(f1, -MITAD, y - 0.18f);
            Vector3 off = { -f1.tx * 0.02f, 0.0f, -f1.tz * 0.02f };
            BeginE(RL_QUADS);
            QuadE(SumaE(a, off), SumaE(b, off), SumaE(c, off), SumaE(d, off), e == 0 ? Color{ 110, 46, 42, 255 } : Color{ 64, 26, 32, 255 });
            EndE();
        }

        // Canto claro desmoronado en los dos labios: marca el peligro.
        for (int lado = 0; lado < 2; lado++)
        {
            const MarcoPistaE& f = lado == 0 ? f0 : f1;
            float hacia = lado == 0 ? -1.0f : 1.0f;
            Vector3 a = EnPistaE(f, -4.45f, 0.014f);
            Vector3 b = EnPistaE(f, 4.45f, 0.014f);
            Vector3 c = SumaE(b, { f.tx * 0.22f * hacia, 0.0f, f.tz * 0.22f * hacia });
            Vector3 d = SumaE(a, { f.tx * 0.22f * hacia, 0.0f, f.tz * 0.22f * hacia });
            BeginE(RL_QUADS);
            QuadNieblaE(a, b, c, d, Color{ 232, 178, 120, 255 });
            EndE();
        }

        // Puente: solo queda la franja segura de tablones.
        float c = grieta.centroSeguro;
        float mitad = grieta.mitadSegura;
        float desde = grieta.s0 - 0.75f;
        float hasta = grieta.s1 + 0.75f;
        int tabla = 0;

        for (float s = desde; s < hasta; s += 0.39f, tabla++)
        {
            MarcoPistaE f = MarcoPistaCanonE(m, s + 0.17f);
            Color madera = TonoE(COLOR_MADERA_E, 0.88f + 0.18f * Ruido01Esferas(g * 40 + tabla, 861));
            CajaEjesE(EnPistaE(f, c, 0.03f), LateralE(f), { 0.0f, 1.0f, 0.0f }, TangenteE(f), { mitad + 0.05f, 0.045f, 0.165f }, madera);
        }

        // Largueros bajo el tablero.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 a = PuntoCanonE(m, desde, c + (float)lado * (mitad - 0.12f), -0.1f);
            Vector3 b = PuntoCanonE(m, hasta, c + (float)lado * (mitad - 0.12f), -0.1f);
            VigaE(a, b, 0.14f, COLOR_MADERA_OSCURA_E);
        }

        // Postes y barandas de soga vencidas.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float l = c + (float)lado * (mitad + 0.32f);
            Vector3 pieA = PuntoCanonE(m, grieta.s0 - 0.9f, l, 0.0f);
            Vector3 pieB = PuntoCanonE(m, grieta.s1 + 0.9f, l, 0.0f);
            Vector3 topeA = { pieA.x, pieA.y + 0.95f, pieA.z };
            Vector3 topeB = { pieB.x, pieB.y + 0.95f, pieB.z };
            PrismaE(pieA, topeA, 0.07f, 0.06f, 6, COLOR_MADERA_OSCURA_E);
            PrismaE(pieB, topeB, 0.07f, 0.06f, 6, COLOR_MADERA_OSCURA_E);

            Vector3 previo = topeA;
            for (int q = 1; q <= 8; q++)
            {
                float u = (float)q / 8.0f;
                Vector3 p = {
                    topeA.x + (topeB.x - topeA.x) * u,
                    topeA.y + (topeB.y - topeA.y) * u - 0.9f * 4.0f * u * (1.0f - u),
                    topeA.z + (topeB.z - topeA.z) * u
                };
                VigaE(previo, p, 0.04f, Color{ 120, 92, 62, 255 });
                previo = p;
            }
        }

        if (!decoracion)
        {
            continue;
        }

        // Restos del puente ancho: tablas que cuelgan de los dos labios.
        for (int labio = 0; labio < 2; labio++)
        {
            const MarcoPistaE& f = labio == 0 ? f0 : f1;
            Vector3 haciaAbismo = labio == 0 ? TangenteE(f) : EscalaE(TangenteE(f), -1.0f);

            for (int q = 0; q < 18; q++)
            {
                float l = -4.1f + 0.48f * (float)q;
                if (std::fabs(l - c) < mitad + 0.35f) continue;
                if (Ruido01Esferas(g * 50 + labio * 20 + q, 871) < 0.42f) continue;

                float largo = 0.7f + 1.3f * Ruido01Esferas(g * 50 + labio * 20 + q, 872);
                float balanceo = 0.12f * std::sin(t * 1.4f + (float)q * 0.9f + (float)g);
                Color madera = TonoE(COLOR_MADERA_E, 0.75f + 0.2f * Ruido01Esferas(q, 873 + g));
                TablaColganteE(EnPistaE(f, l, -0.02f), LateralE(f), haciaAbismo, largo, balanceo, madera);
            }
        }

        // Cascotes junto a los labios, pegados a las paredes.
        for (int q = 0; q < 6; q++)
        {
            float lado = q % 2 == 0 ? -1.0f : 1.0f;
            float s = (q < 3 ? grieta.s0 - 0.5f : grieta.s1 + 0.5f) + (Ruido01Esferas(g * 9 + q, 881) - 0.5f) * 0.5f;
            float l = lado * (3.7f + 0.6f * Ruido01Esferas(g * 9 + q, 882));
            Vector3 p = PuntoCanonE(m, s, l, 0.1f);
            RocaE(p, { 0.22f, 0.16f, 0.2f }, g * 9 + q, COLOR_ROCA_CLARA_E);
        }
    }
}


//==================================================
// VISUAL: RAMPA DEL ATAJO, PORTICOS Y ARCO NATURAL
//==================================================

static float AlturaRampaE(float s)
{
    return 0.05f + 0.95f * Limitar01E((s - RAMPA_S0_ESFERAS) / (RAMPA_S1_ESFERAS - RAMPA_S0_ESFERAS));
}


// Chevron pintado (V que apunta hacia adelante) sobre la pista o la rampa.
static void ChevronE(const MinijuegoEsferasCanon& m, float s, float lateral, float ancho, float altura, Color color)
{
    TrazoLechoE(m, s - 0.25f, lateral - ancho, s + 0.3f, lateral, 0.16f, altura, color);
    TrazoLechoE(m, s + 0.3f, lateral, s - 0.25f, lateral + ancho, 0.16f, altura, color);
}


// Trazo sobre la rampa (sigue su pendiente).
static void TrazoRampaE(const MinijuegoEsferasCanon& m, float sa, float la, float sb, float lb, float ancho, Color color)
{
    Vector3 a = PuntoCanonE(m, sa, la, AlturaRampaE(sa) + 0.075f);
    Vector3 b = PuntoCanonE(m, sb, lb, AlturaRampaE(sb) + 0.075f);
    float dx = b.x - a.x, dz = b.z - a.z;
    float l = std::sqrt(dx * dx + dz * dz);
    if (l < 0.0001f) return;
    float nx = -dz / l * ancho * 0.5f, nz = dx / l * ancho * 0.5f;

    BeginE(RL_QUADS);
    QuadNieblaE({ a.x + nx, a.y, a.z + nz }, { b.x + nx, b.y, b.z + nz }, { b.x - nx, b.y, b.z - nz }, { a.x - nx, a.y, a.z - nz }, color, 0.4f);
    EndE();
}


// MODELO FUTURO: rampa de tablones con caballetes y chevrones turquesa.
static void DibujarRampaE(const MinijuegoEsferasCanon& m, float t, bool decoracion)
{
    const float L0 = RAMPA_LAT0_ESFERAS;
    const float L1 = RAMPA_LAT1_ESFERAS;
    float lc = 0.5f * (L0 + L1);
    float mitad = 0.5f * (L1 - L0);

    // Tablones del tablero inclinados con la pendiente.
    int tabla = 0;
    for (float s = RAMPA_S0_ESFERAS; s < RAMPA_S1_ESFERAS - 0.05f; s += 0.44f, tabla++)
    {
        float sm = s + 0.2f;
        MarcoPistaE f = MarcoPistaCanonE(m, sm);
        Vector3 subida = NormalizarE({ f.tx * (RAMPA_S1_ESFERAS - RAMPA_S0_ESFERAS), 0.95f, f.tz * (RAMPA_S1_ESFERAS - RAMPA_S0_ESFERAS) });
        Vector3 u = LateralE(f);
        Vector3 v = NormalizarE(CruzE(u, subida));
        Color madera = TonoE(COLOR_MADERA_E, 0.95f + 0.16f * Ruido01Esferas(tabla, 901));
        CajaEjesE(EnPistaE(f, lc, AlturaRampaE(sm) - 0.02f), u, v, subida, { mitad + 0.06f, 0.05f, 0.2f }, madera);
    }

    // Costados, caballetes y frente alto.
    for (int lado = 0; lado < 2; lado++)
    {
        float l = lado == 0 ? L0 - 0.02f : L1 + 0.02f;
        Vector3 a = PuntoCanonE(m, RAMPA_S0_ESFERAS, l, 0.0f);
        Vector3 b = PuntoCanonE(m, RAMPA_S1_ESFERAS, l, AlturaRampaE(RAMPA_S1_ESFERAS) - 0.05f);
        VigaE(a, b, 0.14f, COLOR_MADERA_OSCURA_E);

        for (int q = 0; q < 2; q++)
        {
            float s = q == 0 ? RAMPA_S1_ESFERAS - 0.08f : RAMPA_S0_ESFERAS + 2.0f;
            Vector3 pie = PuntoCanonE(m, s, l, 0.0f);
            Vector3 tope = PuntoCanonE(m, s, l, AlturaRampaE(s) - 0.04f);
            VigaE(pie, tope, 0.13f, COLOR_MADERA_OSCURA_E);
        }

        Vector3 cruzA = PuntoCanonE(m, RAMPA_S0_ESFERAS + 2.0f, l, 0.05f);
        Vector3 cruzB = PuntoCanonE(m, RAMPA_S1_ESFERAS - 0.1f, l, AlturaRampaE(RAMPA_S1_ESFERAS) - 0.1f);
        VigaE(cruzA, cruzB, 0.08f, COLOR_MADERA_OSCURA_E);
    }

    {
        MarcoPistaE f = MarcoPistaCanonE(m, RAMPA_S1_ESFERAS);
        Vector3 a = EnPistaE(f, L0, 0.0f), b = EnPistaE(f, L1, 0.0f);
        Vector3 c = EnPistaE(f, L1, AlturaRampaE(RAMPA_S1_ESFERAS)), d = EnPistaE(f, L0, AlturaRampaE(RAMPA_S1_ESFERAS));
        BeginE(RL_QUADS);
        QuadLuzE(a, b, c, d, COLOR_MADERA_OSCURA_E, TangenteE(f));
        EndE();
    }

    // Chevrones turquesa sobre el tablero: el acento del atajo.
    float brillo = 0.85f + 0.15f * std::sin(t * 4.0f);
    Color acento = MezclaE(COLOR_TURQUESA_E, WHITE, 0.15f * brillo);

    for (int q = 0; q < 2; q++)
    {
        float s = RAMPA_S0_ESFERAS + 0.9f + 1.3f * (float)q;
        TrazoRampaE(m, s - 0.3f, L0 + 0.25f, s + 0.3f, lc, 0.2f, acento);
        TrazoRampaE(m, s + 0.3f, lc, s - 0.3f, L1 - 0.25f, 0.2f, acento);
    }

    // Flechas en el piso que llevan a la rampa.
    for (int q = 0; q < 4; q++)
    {
        float s = RAMPA_S0_ESFERAS - 8.5f + 2.1f * (float)q;
        float alfa = 0.55f + 0.45f * Limitar01E(0.5f + 0.5f * std::sin(t * 5.0f - (float)q * 0.9f));
        ChevronE(m, s, lc, 0.5f, 0.013f, AlfaE(COLOR_TURQUESA_E, alfa));
    }

    if (!decoracion)
    {
        return;
    }

    // Cartel en la cornisa derecha: tabla con flecha turquesa.
    Vector3 pie = PuntoCanonE(m, RAMPA_S0_ESFERAS - 6.0f, 4.75f, 0.0f);
    Vector3 tope = { pie.x, pie.y + 2.6f, pie.z };
    PrismaE(pie, tope, 0.09f, 0.08f, 6, COLOR_MADERA_OSCURA_E);
    MarcoPistaE f = MarcoPistaCanonE(m, RAMPA_S0_ESFERAS - 6.0f);
    Vector3 centroTabla = { tope.x - f.nx * 0.55f, tope.y - 0.35f, tope.z - f.nz * 0.55f };
    CajaEjesE(centroTabla, LateralE(f), { 0.0f, 1.0f, 0.0f }, TangenteE(f), { 0.62f, 0.3f, 0.05f }, COLOR_MADERA_E);
    Vector3 flecha0 = SumaE(centroTabla, { -f.tx * 0.06f - f.nx * 0.4f, 0.0f, -f.tz * 0.06f - f.nz * 0.4f });
    Vector3 flecha1 = SumaE(centroTabla, { -f.tx * 0.06f + f.nx * 0.25f, 0.0f, -f.tz * 0.06f + f.nz * 0.25f });
    VigaE(flecha0, flecha1, 0.12f, COLOR_TURQUESA_E);
    TrianguloE(
        SumaE(flecha1, { f.nx * 0.28f - f.tx * 0.07f, 0.0f, f.nz * 0.28f - f.tz * 0.07f }),
        SumaE(flecha1, { -f.tx * 0.07f, 0.22f, -f.tz * 0.07f }),
        SumaE(flecha1, { -f.tx * 0.07f, -0.22f, -f.tz * 0.07f }),
        COLOR_TURQUESA_E
    );
}


struct FaroE
{
    Vector3 llama{};
    Vector3 suelo{};
};


static const int MAX_FAROS_E = 20;


// Farol colgado o sobre poste: armazon oscuro y vidrio encendido.
static void DibujarFarolE(Vector3 centro, float t, int semilla)
{
    float parpadeo = 0.9f + 0.1f * std::sin(t * 11.0f + (float)semilla * 1.7f);
    CajaE({ centro.x, centro.y + 0.24f, centro.z }, { 0.3f, 0.06f, 0.3f }, 0.0f, Color{ 52, 40, 34, 255 });
    CajaE({ centro.x, centro.y - 0.22f, centro.z }, { 0.28f, 0.05f, 0.28f }, 0.0f, Color{ 52, 40, 34, 255 });
    CajaE(centro, { 0.2f, 0.4f, 0.2f }, 0.0f, TonoE(Color{ 255, 206, 120, 255 }, parpadeo), false, 1.0f);
}


static void PosteFarolE(Vector3 pie, float alto, float t, int semilla, FaroE faros[], int& cantidad)
{
    Vector3 tope = { pie.x, pie.y + alto, pie.z };
    PrismaE(pie, tope, 0.08f, 0.07f, 6, COLOR_MADERA_OSCURA_E);
    Vector3 farol = { tope.x, tope.y + 0.3f, tope.z };
    DibujarFarolE(farol, t, semilla);

    if (cantidad < MAX_FAROS_E)
    {
        faros[cantidad++] = { farol, pie };
    }
}


// Tela que flamea (bandera o banderin) entre dos puntos del mastil.
static void TelaE(Vector3 arriba, Vector3 abajo, Vector3 haciaFuera, float largo, float t, int semilla, Color color, bool cuadros)
{
    const int TRAMOS = 6;
    Vector3 previoA = arriba, previoB = abajo;

    BeginE(RL_QUADS);

    for (int q = 1; q <= TRAMOS; q++)
    {
        float u = (float)q / (float)TRAMOS;
        float onda = 0.16f * u * std::sin(t * 5.0f - u * 3.2f + (float)semilla);
        Vector3 desplaza = SumaE(EscalaE(haciaFuera, largo * u), { 0.0f, -0.08f * u, 0.0f });
        Vector3 lado = NormalizarE(CruzE(haciaFuera, { 0.0f, 1.0f, 0.0f }));
        Vector3 a = SumaE(SumaE(arriba, desplaza), EscalaE(lado, onda));
        Vector3 b = SumaE(SumaE(abajo, desplaza), EscalaE(lado, onda));
        Color c = color;

        if (cuadros)
        {
            c = (q & 1) ? Color{ 34, 30, 32, 255 } : Color{ 246, 238, 222, 255 };
        }
        else
        {
            c = TonoE(color, 0.88f + 0.14f * std::sin(t * 5.0f - u * 3.2f + (float)semilla + 1.2f));
        }

        QuadNieblaE(previoA, a, b, previoB, c);
        previoA = a;
        previoB = b;
    }

    EndE();
}


// MODELO FUTURO: portico de salida, totems de control y pilares de la meta.
static void DibujarPorticosE(const MinijuegoEsferasCanon& m, const Participante participantes[], int limite, float t, FaroE faros[], int& cantidadFaros, float sMin, float sMax)
{
    // Salida: arco de madera con lona turquesa y linea de cal.
    if (S_SALIDA_ESFERAS - 1.2f > sMin - 4.0f && S_SALIDA_ESFERAS - 1.2f < sMax)
    {
        float s = S_SALIDA_ESFERAS - 1.2f;
        MarcoPistaE f = MarcoPistaCanonE(m, s);
        {
            Vector3 a = EnPistaE(f, -4.4f, 0.013f), b = EnPistaE(f, 4.4f, 0.013f);
            Vector3 d = SumaE(a, EscalaE(TangenteE(f), 0.3f)), c = SumaE(b, EscalaE(TangenteE(f), 0.3f));
            BeginE(RL_QUADS);
            QuadNieblaE(a, b, c, d, COLOR_CREMA_E);
            EndE();
        }

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 pie = EnPistaE(f, (float)lado * 4.7f, 0.0f);
            PrismaE(pie, { pie.x, pie.y + 7.2f, pie.z }, 0.2f, 0.17f, 6, COLOR_MADERA_E);
            Vector3 farol = EnPistaE(f, (float)lado * 4.25f, 6.1f);
            VigaE({ farol.x, farol.y + 0.3f, farol.z }, { farol.x, farol.y + 0.75f, farol.z }, 0.03f, Color{ 52, 40, 34, 255 });
            DibujarFarolE(farol, t, lado + 3);
            if (cantidadFaros < MAX_FAROS_E) faros[cantidadFaros++] = { farol, EnPistaE(f, (float)lado * 4.25f, 0.0f) };
        }

        VigaE(EnPistaE(f, -5.0f, 7.0f), EnPistaE(f, 5.0f, 7.0f), 0.26f, COLOR_MADERA_OSCURA_E);

        BeginE(RL_QUADS);
        for (int q = 0; q < 8; q++)
        {
            float l0 = -3.0f + 0.75f * (float)q, l1 = l0 + 0.75f;
            float o0 = 0.08f * std::sin(t * 2.2f + l0), o1 = 0.08f * std::sin(t * 2.2f + l1);
            Vector3 a = EnPistaE(f, l0, 6.85f), b = EnPistaE(f, l1, 6.85f);
            Vector3 c = SumaE(EnPistaE(f, l1, 6.2f), EscalaE(TangenteE(f), o1));
            Vector3 d = SumaE(EnPistaE(f, l0, 6.2f), EscalaE(TangenteE(f), o0));
            Color tela = q == 0 || q == 7 ? COLOR_CREMA_E : COLOR_TURQUESA_E;
            QuadLuzE(a, b, c, d, tela, EscalaE(TangenteE(f), -1.0f));
        }
        EndE();
    }

    // Controles: totems pintados, soga con banderines y cintas de color
    // por cada jugador que ya paso.
    for (int k = 0; k < CANTIDAD_CHECKPOINTS_ESFERAS; k++)
    {
        float s = CHECKPOINTS_ESFERAS[k];
        if (s < sMin - 4.0f || s > sMax) continue;

        MarcoPistaE f = MarcoPistaCanonE(m, s);
        {
            Vector3 a = EnPistaE(f, -4.4f, 0.012f), b = EnPistaE(f, 4.4f, 0.012f);
            Vector3 d = SumaE(a, EscalaE(TangenteE(f), 0.22f)), c = SumaE(b, EscalaE(TangenteE(f), 0.22f));
            BeginE(RL_QUADS);
            QuadNieblaE(a, b, c, d, AlfaE(COLOR_CREMA_E, 0.85f));
            EndE();
        }

        static const Color BLOQUES[5] =
        {
            { 52, 204, 190, 255 }, { 236, 190, 134, 255 }, { 176, 72, 46, 255 },
            { 52, 204, 190, 255 }, { 236, 190, 134, 255 }
        };

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 pie = EnPistaE(f, (float)lado * 4.7f, 0.0f);

            for (int b = 0; b < 5; b++)
            {
                float ancho = 0.5f - 0.03f * (float)b;
                CajaE({ pie.x, pie.y + 0.45f + 0.92f * (float)b, pie.z }, { ancho, 0.86f, ancho }, std::atan2(f.tx, f.tz), BLOQUES[b]);
            }

            Vector3 farol = { pie.x, pie.y + 4.95f, pie.z };
            DibujarFarolE(farol, t, k * 2 + lado);
            if (cantidadFaros < MAX_FAROS_E) faros[cantidadFaros++] = { farol, pie };
        }

        // Soga con banderines.
        Vector3 a = EnPistaE(f, -4.7f, 4.6f);
        Vector3 b = EnPistaE(f, 4.7f, 4.6f);
        Vector3 previo = a;
        for (int q = 1; q <= 12; q++)
        {
            float u = (float)q / 12.0f;
            Vector3 p = { a.x + (b.x - a.x) * u, a.y - 0.55f * 4.0f * u * (1.0f - u), a.z + (b.z - a.z) * u };
            VigaE(previo, p, 0.035f, Color{ 120, 92, 62, 255 });

            if (q < 12)
            {
                static const Color BANDERINES[3] = { { 52, 204, 190, 255 }, { 246, 230, 196, 255 }, { 232, 150, 60, 255 } };
                float vaiven = 0.12f * std::sin(t * 4.0f + (float)q * 0.8f);
                Vector3 punta = { p.x + f.tx * vaiven, p.y - 0.5f, p.z + f.tz * vaiven };
                Vector3 izq = { p.x - f.nx * 0.18f, p.y, p.z - f.nz * 0.18f };
                Vector3 der = { p.x + f.nx * 0.18f, p.y, p.z + f.nz * 0.18f };
                TrianguloE(izq, der, punta, TinteE(BANDERINES[q % 3], p));
            }

            previo = p;
        }

        // Cintas de los que ya pasaron (en el totem derecho).
        Vector3 totem = EnPistaE(f, 4.7f, 0.0f);
        for (int i = 0; i < limite; i++)
        {
            if (!m.estadosJugadores[i].participa || m.estadosJugadores[i].checkpoints <= k) continue;
            float y = totem.y + 1.0f + 0.42f * (float)i;
            float vaiven = 0.1f * std::sin(t * 6.0f + (float)i);
            Vector3 nudo = { totem.x - f.nx * 0.27f, y, totem.z - f.nz * 0.27f };
            Vector3 cola = { nudo.x - f.tx * 0.5f + f.nx * vaiven, y - 0.18f, nudo.z - f.tz * 0.5f + f.nz * vaiven };
            VigaE(nudo, cola, 0.12f, participantes[i].color, false);
        }
    }

    // Meta: pilares de piedra apilada, bandera a cuadros y faroles.
    float sMeta = LONGITUD_META_ESFERAS;
    if (sMeta > sMin - 6.0f && sMeta < sMax)
    {
        MarcoPistaE f = MarcoPistaCanonE(m, sMeta);
        Vector3 tg = TangenteE(f);

        // Franja a cuadros en el piso.
        BeginE(RL_QUADS);
        for (int fila = 0; fila < 2; fila++)
        {
            for (int c = 0; c < 10; c++)
            {
                float l0 = -4.4f + 0.88f * (float)c, l1 = l0 + 0.88f;
                float s0 = sMeta + 0.45f * (float)fila, s1 = s0 + 0.45f;
                Color color = ((c + fila) & 1) ? Color{ 40, 34, 36, 255 } : Color{ 246, 238, 222, 255 };
                QuadNieblaE(PuntoCanonE(m, s0, l0, 0.014f), PuntoCanonE(m, s0, l1, 0.014f), PuntoCanonE(m, s1, l1, 0.014f), PuntoCanonE(m, s1, l0, 0.014f), color);
            }
        }
        EndE();

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 pie = EnPistaE(f, (float)lado * 4.85f, 0.0f);
            float y = pie.y;

            for (int b = 0; b < 6; b++)
            {
                float r = 0.62f - 0.05f * (float)b;
                float alto = 0.5f + 0.5f * Ruido01Esferas(b, 911 + lado);
                RocaE({ pie.x, y + alto * 0.5f, pie.z }, { r, alto * 0.62f, r }, b + 20 + lado, b % 2 == 0 ? COLOR_ROCA_CLARA_E : COLOR_CREMA_E, 0.12f);
                y += alto * 0.95f;
            }

            Vector3 mastilPie = { pie.x, y, pie.z };
            Vector3 mastilTope = { pie.x, y + 1.6f, pie.z };
            PrismaE(mastilPie, mastilTope, 0.05f, 0.04f, 5, COLOR_MADERA_OSCURA_E);
            Vector3 haciaFuera = EscalaE(LateralE(f), (float)-lado);
            TelaE(mastilTope, { mastilTope.x, mastilTope.y - 0.55f, mastilTope.z }, SumaE(EscalaE(haciaFuera, 0.4f), EscalaE(tg, 0.6f)), 1.0f, t, lado + 5, COLOR_TURQUESA_E, false);

            Vector3 farol = EnPistaE(f, (float)lado * 3.9f, 4.9f);
            VigaE({ farol.x, farol.y + 0.3f, farol.z }, { farol.x, farol.y + 1.0f, farol.z }, 0.03f, Color{ 52, 40, 34, 255 });
            DibujarFarolE(farol, t, lado + 9);
            if (cantidadFaros < MAX_FAROS_E) faros[cantidadFaros++] = { farol, EnPistaE(f, (float)lado * 3.9f, 0.0f) };
        }

        VigaE(EnPistaE(f, -5.3f, 5.95f), EnPistaE(f, 5.3f, 5.95f), 0.3f, COLOR_MADERA_OSCURA_E);

        // Pancarta a cuadros que ondea.
        BeginE(RL_QUADS);
        for (int fila = 0; fila < 2; fila++)
        {
            for (int c = 0; c < 8; c++)
            {
                float l0 = -3.2f + 0.8f * (float)c, l1 = l0 + 0.8f;
                float y0 = 5.8f - 0.55f * (float)fila, y1 = y0 - 0.55f;
                float o0 = 0.1f * std::sin(t * 2.4f + l0 * 0.9f) * (float)(fila + 1) * 0.5f;
                float o1 = 0.1f * std::sin(t * 2.4f + l1 * 0.9f) * (float)(fila + 1) * 0.5f;
                Color color = ((c + fila) & 1) ? Color{ 40, 34, 36, 255 } : Color{ 246, 238, 222, 255 };
                Vector3 a = SumaE(EnPistaE(f, l0, y0), EscalaE(tg, o0 * 0.6f));
                Vector3 b = SumaE(EnPistaE(f, l1, y0), EscalaE(tg, o1 * 0.6f));
                Vector3 cc = SumaE(EnPistaE(f, l1, y1), EscalaE(tg, o1));
                Vector3 d = SumaE(EnPistaE(f, l0, y1), EscalaE(tg, o0));
                QuadNieblaE(a, b, cc, d, color);
            }
        }
        EndE();
    }
}


// Arco natural al fondo: enmarca la llegada.
// MODELO FUTURO: arco natural de arenisca.
static void DibujarArcoNaturalE(const MinijuegoEsferasCanon& m)
{
    MarcoPistaE f = MarcoPistaCanonE(m, S_ARCO_E);
    const int TRAMOS = 16;
    const float RADIO = 10.6f;
    const float BASE = 4.0f;
    const float CIMA = 15.5f;
    Vector3 tg = TangenteE(f);

    for (int q = 0; q < TRAMOS; q++)
    {
        float a0 = 3.1416f * (float)q / (float)TRAMOS;
        float a1 = 3.1416f * (float)(q + 1) / (float)TRAMOS;
        Vector3 p0 = EnPistaE(f, -RADIO * std::cos(a0), BASE + (CIMA - BASE) * std::sin(a0));
        Vector3 p1 = EnPistaE(f, -RADIO * std::cos(a1), BASE + (CIMA - BASE) * std::sin(a1));
        Vector3 centro = EscalaE(SumaE(p0, p1), 0.5f);
        Vector3 w = NormalizarE(RestaE(p1, p0));
        Vector3 u = NormalizarE(CruzE(tg, w));
        float largo = std::sqrt(PuntoE(RestaE(p1, p0), RestaE(p1, p0)));
        float am = 0.5f * (a0 + a1);
        float grosor = 2.6f - 1.0f * std::sin(am);
        float profundo = 3.0f - 0.9f * std::sin(am);
        Color c = (q % 3 == 0) ? COLOR_CREMA_E : ((q % 3 == 1) ? COLOR_ROCA_E : COLOR_ROCA_CLARA_E);
        CajaEjesE(centro, u, tg, w, { grosor * 0.5f, profundo * 0.5f, largo * 0.5f + 0.35f }, c);
    }

    // Pies del arco sobre las terrazas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 pie = EnPistaE(f, (float)lado * RADIO, 0.0f);
        RocaE({ pie.x, pie.y + 2.4f, pie.z }, { 2.2f, 3.2f, 2.4f }, 960 + lado, COLOR_ROCA_E, 0.12f);
    }
}


//==================================================
// VISUAL: UTILERIA DEL CANON
//==================================================

// MODELO FUTURO: saguaro, nopal, calavera, rueda de carreta, barril.
static void SaguaroE(Vector3 pie, float alto, int semilla, bool sombras)
{
    float r = 0.18f + 0.06f * Ruido01Esferas(semilla, 1001);
    Vector3 tope = { pie.x, pie.y + alto, pie.z };

    if (sombras) SombraLargaE(pie, r * 3.0f, alto, 0.22f);

    PrismaE(pie, tope, r, r * 0.9f, 8, COLOR_CACTUS_E);
    RocaE(tope, { r * 0.9f, r * 0.7f, r * 0.9f }, semilla, COLOR_CACTUS_E, 0.05f);

    int brazos = 1 + (int)(Ruido01Esferas(semilla, 1002) * 2.0f);
    for (int b = 0; b < brazos; b++)
    {
        float ang = 6.2832f * Ruido01Esferas(semilla * 3 + b, 1003);
        float h = alto * (0.38f + 0.2f * Ruido01Esferas(semilla * 3 + b, 1004));
        float largo = 0.35f + 0.15f * Ruido01Esferas(semilla * 3 + b, 1005);
        Vector3 dir = { std::cos(ang), 0.0f, std::sin(ang) };
        Vector3 codo0 = { pie.x, pie.y + h, pie.z };
        Vector3 codo1 = { pie.x + dir.x * largo, pie.y + h + 0.1f, pie.z + dir.z * largo };
        Vector3 punta = { codo1.x, codo1.y + alto * (0.3f + 0.2f * Ruido01Esferas(semilla + b, 1006)), codo1.z };
        PrismaE(codo0, codo1, r * 0.62f, r * 0.62f, 6, COLOR_CACTUS_E);
        PrismaE(codo1, punta, r * 0.62f, r * 0.56f, 6, COLOR_CACTUS_E);
        RocaE(punta, { r * 0.56f, r * 0.45f, r * 0.56f }, semilla + b, COLOR_CACTUS_E, 0.05f);
    }

    if (Ruido01Esferas(semilla, 1007) < 0.4f)
    {
        CajaE({ tope.x, tope.y + r * 0.7f, tope.z }, { 0.12f, 0.08f, 0.12f }, 0.3f, Color{ 246, 230, 200, 255 });
    }
}


static void NopalE(Vector3 pie, int semilla)
{
    for (int q = 0; q < 4; q++)
    {
        float ang = 6.2832f * Ruido01Esferas(semilla * 5 + q, 1011);
        float alto = 0.25f + 0.35f * (float)(q / 2);
        Vector3 eje = { std::cos(ang), 0.0f, std::sin(ang) };
        Vector3 c = { pie.x + eje.z * 0.25f * (float)(q % 2 * 2 - 1), pie.y + alto, pie.z - eje.x * 0.25f * (float)(q % 2 * 2 - 1) };
        PrismaE(SumaE(c, EscalaE(eje, -0.05f)), SumaE(c, EscalaE(eje, 0.05f)), 0.27f, 0.27f, 7, TonoE(COLOR_CACTUS_E, 1.05f));

        if (q >= 2)
        {
            CajaE({ c.x, c.y + 0.29f, c.z }, { 0.08f, 0.1f, 0.08f }, 0.0f, Color{ 222, 80, 112, 255 });
        }
    }
}


static void ArbustoE(Vector3 pie, int semilla)
{
    for (int q = 0; q < 3; q++)
    {
        float dx = (Ruido01Esferas(semilla * 3 + q, 1021) - 0.5f) * 0.6f;
        float dz = (Ruido01Esferas(semilla * 3 + q, 1022) - 0.5f) * 0.6f;
        float r = 0.22f + 0.16f * Ruido01Esferas(semilla * 3 + q, 1023);
        Color c = q == 1 ? Color{ 156, 150, 98, 255 } : Color{ 128, 124, 78, 255 };
        RocaE({ pie.x + dx, pie.y + r * 0.6f, pie.z + dz }, { r, r * 0.7f, r }, semilla * 3 + q, c, 0.25f);
    }
}


static void PiedrasE(Vector3 pie, int semilla, float escala, bool sombras)
{
    int n = 1 + (int)(Ruido01Esferas(semilla, 1031) * 3.0f);

    for (int q = 0; q < n; q++)
    {
        float r = escala * (0.35f + 0.6f * Ruido01Esferas(semilla * 4 + q, 1032));
        float dx = (Ruido01Esferas(semilla * 4 + q, 1033) - 0.5f) * escala * 1.6f;
        float dz = (Ruido01Esferas(semilla * 4 + q, 1034) - 0.5f) * escala * 1.6f;
        Vector3 c = { pie.x + dx, pie.y + r * 0.5f, pie.z + dz };
        if (sombras) SombraLargaE({ c.x, pie.y, c.z }, r * 2.0f, r, 0.2f);
        Color color = q % 2 == 0 ? COLOR_ROCA_E : COLOR_ROCA_CLARA_E;
        RocaE(c, { r, r * 0.72f, r * 0.9f }, semilla * 4 + q, color);
    }
}


static void CalaveraE(Vector3 pie, float giro)
{
    Color hueso = { 236, 228, 210, 255 };
    CajaE({ pie.x, pie.y + 0.12f, pie.z }, { 0.32f, 0.22f, 0.42f }, giro, hueso);
    float cs = std::cos(giro), sn = std::sin(giro);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 base = { pie.x + cs * 0.16f * (float)lado, pie.y + 0.2f, pie.z - sn * 0.16f * (float)lado };
        Vector3 medio = { base.x + cs * 0.25f * (float)lado, base.y + 0.05f, base.z - sn * 0.25f * (float)lado };
        Vector3 punta = { medio.x + cs * 0.08f * (float)lado, medio.y + 0.22f, medio.z - sn * 0.08f * (float)lado };
        VigaE(base, medio, 0.07f, hueso);
        VigaE(medio, punta, 0.05f, hueso);
    }

    CajaE({ pie.x + sn * 0.2f, pie.y + 0.1f, pie.z + cs * 0.2f }, { 0.08f, 0.06f, 0.04f }, giro, Color{ 60, 40, 36, 255 });
}


static void CercaRotaE(Vector3 pie, Vector3 dir, int semilla)
{
    Vector3 previo{};

    for (int q = 0; q < 3; q++)
    {
        Vector3 p = SumaE(pie, EscalaE(dir, 0.9f * (float)q));
        float alto = 0.8f + 0.3f * Ruido01Esferas(semilla + q, 1041);
        Vector3 tope = { p.x + 0.05f * (float)(q - 1), p.y + alto, p.z };
        VigaE(p, tope, 0.11f, Color{ 128, 104, 84, 255 });

        if (q > 0 && Ruido01Esferas(semilla + q, 1042) < 0.7f)
        {
            VigaE({ previo.x, previo.y + 0.55f, previo.z }, { p.x, p.y + 0.5f, p.z }, 0.07f, Color{ 146, 118, 92, 255 });
        }

        previo = p;
    }
}


static void RuedaCarretaE(Vector3 pie, Vector3 plano, float apoyo)
{
    Vector3 arriba = NormalizarE({ -plano.z * apoyo, 1.0f, plano.x * apoyo });
    Vector3 centro = SumaE(pie, EscalaE(arriba, 0.55f));
    const int RAYOS = 10;

    for (int q = 0; q < RAYOS; q++)
    {
        float a0 = 6.2832f * (float)q / (float)RAYOS;
        float a1 = 6.2832f * (float)(q + 1) / (float)RAYOS;
        Vector3 p0 = SumaE(centro, SumaE(EscalaE(plano, std::cos(a0) * 0.55f), EscalaE(arriba, std::sin(a0) * 0.55f)));
        Vector3 p1 = SumaE(centro, SumaE(EscalaE(plano, std::cos(a1) * 0.55f), EscalaE(arriba, std::sin(a1) * 0.55f)));
        VigaE(p0, p1, 0.08f, COLOR_MADERA_OSCURA_E);
        if (q % 2 == 0) VigaE(centro, p0, 0.045f, COLOR_MADERA_E);
    }

    CajaE(centro, { 0.16f, 0.16f, 0.16f }, 0.0f, Color{ 70, 56, 46, 255 });
}


static void BarrilE(Vector3 pie, int semilla)
{
    Vector3 tope = { pie.x, pie.y + 0.85f, pie.z };
    PrismaE(pie, tope, 0.3f, 0.3f, 8, TonoE(COLOR_MADERA_E, 0.9f + 0.15f * Ruido01Esferas(semilla, 1051)));

    for (int a = 0; a < 2; a++)
    {
        float y = pie.y + 0.18f + 0.48f * (float)a;
        PrismaE({ pie.x, y, pie.z }, { pie.x, y + 0.07f, pie.z }, 0.315f, 0.315f, 8, Color{ 70, 60, 56, 255 });
    }
}


static void ArbolSecoE(Vector3 pie, float alto, int semilla, bool sombras)
{
    Color corteza = { 116, 96, 82, 255 };
    Vector3 tope = { pie.x, pie.y + alto, pie.z };
    if (sombras) SombraLargaE(pie, 0.6f, alto, 0.16f);
    PrismaE(pie, tope, 0.13f, 0.07f, 5, corteza);

    for (int q = 0; q < 4; q++)
    {
        float ang = 6.2832f * Ruido01Esferas(semilla * 4 + q, 1061);
        float h = alto * (0.45f + 0.45f * Ruido01Esferas(semilla * 4 + q, 1062));
        float largo = 0.5f + 0.6f * Ruido01Esferas(semilla * 4 + q, 1063);
        Vector3 a = { pie.x, pie.y + h, pie.z };
        Vector3 b = { a.x + std::cos(ang) * largo, a.y + largo * 0.6f, a.z + std::sin(ang) * largo };
        VigaE(a, b, 0.06f, corteza);
        Vector3 c = { b.x + std::cos(ang + 0.8f) * largo * 0.4f, b.y + largo * 0.35f, b.z + std::sin(ang + 0.8f) * largo * 0.4f };
        VigaE(b, c, 0.04f, corteza);
    }
}


static void ChimeneaHadaE(Vector3 pie, float alto, int semilla, bool sombras)
{
    if (sombras) SombraLargaE(pie, 1.6f, alto, 0.2f);
    float y = pie.y;
    int piezas = 3 + (int)(Ruido01Esferas(semilla, 1071) * 2.0f);

    for (int q = 0; q < piezas; q++)
    {
        float h = alto / (float)piezas;
        float r = 0.75f - 0.12f * (float)q + 0.1f * Ruido01Esferas(semilla + q, 1072);
        Color c = q % 2 == 0 ? COLOR_ROCA_E : COLOR_ROCA_CLARA_E;
        RocaE({ pie.x, y + h * 0.5f, pie.z }, { r, h * 0.62f, r }, semilla + q, c, 0.14f);
        y += h * 0.9f;
    }

    RocaE({ pie.x, y + 0.25f, pie.z }, { 0.95f, 0.38f, 0.9f }, semilla + 9, COLOR_CREMA_E, 0.1f);
}


// Utileria por ranuras deterministas a lo largo del canon: en las dos
// cornisas y en la meseta. paso 2 en calidad media.
static void DibujarUtileriaE(const CanonCacheE& cache, float sMin, float sMax, int paso, bool sombras)
{
    for (int lado = 0; lado < 2; lado++)
    {
        float signo = lado == 0 ? -1.0f : 1.0f;

        for (int nivel = 1; nivel <= 4; nivel++)
        {
            float separacion = nivel == 4 ? 3.6f : (nivel == 3 ? 4.4f : 3.1f);
            int ranuras = (int)((240.0f + 20.0f) / separacion);

            for (int j = 0; j < ranuras; j += paso)
            {
                int semilla = j * 7 + lado * 1301 + nivel * 311;
                float s = -20.0f + separacion * (float)j + separacion * 0.8f * Ruido01Esferas(semilla, 1101);
                if (s < sMin || s > sMax) continue;

                int k = (int)((s - S_INICIO_CANON_E) / PASO_CANON_E + 0.5f);
                if (k < 0 || k >= MUESTRAS_CANON_E) continue;

                float desde = 0.0f, hasta = 0.0f, altura = 0.0f;
                if (nivel == 1)
                {
                    desde = 5.4f;
                    hasta = LateralTerrazaE(k, lado, 1) - 0.4f;
                    altura = AlturaTerrazaE(k, lado, 1);
                }
                else if (nivel == 2)
                {
                    desde = LateralTerrazaE(k, lado, 1) + 1.2f;
                    hasta = LateralTerrazaE(k, lado, 2) - 0.4f;
                    altura = AlturaTerrazaE(k, lado, 2);
                }
                else
                {
                    // Meseta: desde el borde, en X puro (como el suelo de la meseta).
                    desde = nivel == 3 ? 0.8f : 10.0f;
                    hasta = nivel == 3 ? 10.0f : 34.0f;
                    altura = AlturaTerrazaE(k, lado, 3);
                }

                float lateral = desde + (hasta - desde) * Ruido01Esferas(semilla, 1102);
                Vector3 pie{};

                if (nivel <= 2)
                {
                    pie = PuntoParedE(cache, k, lado, lateral, altura);
                }
                else
                {
                    pie = PuntoParedE(cache, k, lado, LateralTerrazaE(k, lado, 2) + 1.9f, altura);
                    pie.x += signo * lateral;
                }

                float corrimiento = s - SMuestraE(k);
                pie.x += cache.marcos[k].tx * corrimiento;
                pie.z += cache.marcos[k].tz * corrimiento;

                float tipo = Ruido01Esferas(semilla, 1103);

                if (nivel == 4)
                {
                    if (tipo < 0.3f) SaguaroE(pie, 3.0f + 1.8f * Ruido01Esferas(semilla, 1104), semilla, sombras);
                    else if (tipo < 0.55f) PiedrasE(pie, semilla, 1.7f, sombras);
                    else if (tipo < 0.75f) ArbustoE(pie, semilla);
                    else if (tipo < 0.87f) ChimeneaHadaE(pie, 5.0f + 4.0f * Ruido01Esferas(semilla, 1105), semilla, sombras);
                    else ArbolSecoE(pie, 2.8f, semilla, sombras);
                    continue;
                }

                if (nivel == 3)
                {
                    if (tipo < 0.34f) SaguaroE(pie, 2.6f + 1.6f * Ruido01Esferas(semilla, 1104), semilla, sombras);
                    else if (tipo < 0.55f) PiedrasE(pie, semilla, 1.2f, sombras);
                    else if (tipo < 0.70f) ArbolSecoE(pie, 2.4f, semilla, sombras);
                    else if (tipo < 0.82f) ChimeneaHadaE(pie, 4.0f + 3.0f * Ruido01Esferas(semilla, 1105), semilla, sombras);
                    else ArbustoE(pie, semilla);
                    continue;
                }

                if (tipo < 0.16f) SaguaroE(pie, (nivel == 1 ? 1.3f : 1.7f) + 0.9f * Ruido01Esferas(semilla, 1104), semilla, sombras);
                else if (tipo < 0.40f) NopalE(pie, semilla);
                else if (tipo < 0.60f) PiedrasE(pie, semilla, 0.6f, sombras);
                else if (tipo < 0.80f) ArbustoE(pie, semilla);
                else if (tipo < 0.85f && nivel == 1) CalaveraE(pie, 6.2832f * Ruido01Esferas(semilla, 1106));
                else if (tipo < 0.90f && nivel == 1) CercaRotaE(pie, TangenteE(cache.marcos[k]), semilla);
                else if (tipo < 0.94f && nivel == 1) RuedaCarretaE(pie, TangenteE(cache.marcos[k]), signo * 0.35f);
                else if (nivel == 1) BarrilE(pie, semilla);
                else ArbolSecoE(pie, 1.8f, semilla, sombras);
            }
        }
    }
}


// Media distancia: buttes estratificados sobre la meseta.
static void DibujarButtesE(const CanonCacheE& cache, float sMin, float sMax, int paso)
{
    for (int j = 0; j < 16; j += paso)
    {
        for (int lado = 0; lado < 2; lado++)
        {
            int semilla = j * 13 + lado * 101;
            float s = -10.0f + 17.0f * (float)j + 8.0f * Ruido01Esferas(semilla, 1201);
            if (s < sMin - 30.0f || s > sMax + 60.0f) continue;
            int k = (int)((s - S_INICIO_CANON_E) / PASO_CANON_E + 0.5f);
            if (k < 0 || k >= MUESTRAS_CANON_E) continue;

            float lateral = 20.0f + 42.0f * Ruido01Esferas(semilla, 1202);
            Vector3 pie = PuntoParedE(cache, k, lado, LateralTerrazaE(k, lado, 2) + 1.9f, AlturaTerrazaE(k, lado, 3) - 0.3f);
            pie.x += (lado == 0 ? -1.0f : 1.0f) * lateral;
            float alto = 8.0f + 14.0f * Ruido01Esferas(semilla, 1203);
            float ancho = 6.0f + 10.0f * Ruido01Esferas(semilla, 1204);
            Color c = TonoE(COLOR_ROCA_E, 0.92f + 0.14f * Ruido01Esferas(semilla, 1205));
            MesaE(pie, ancho, ancho * (0.6f + 0.4f * Ruido01Esferas(semilla, 1206)), alto, 1.2f * Ruido01Esferas(semilla, 1207), c, semilla, true, true);
        }
    }
}


//==================================================
// VISUAL: VIDA AMBIENTAL (BUITRES Y PLANTAS RODADORAS)
//==================================================

static void DibujarBuitresE(const MinijuegoEsferasCanon& m, float sCamara, float t)
{
    MarcoPistaE f = MarcoPistaCanonE(m, sCamara + 38.0f);

    for (int b = 0; b < 3; b++)
    {
        float ang = t * (0.32f + 0.05f * (float)b) + 2.1f * (float)b;
        float radio = 6.0f + 2.5f * (float)b;
        Vector3 c = { f.c.x + std::cos(ang) * radio, f.c.y + 15.0f + 1.5f * (float)b, f.c.z + std::sin(ang) * radio };
        Vector3 rumbo = { -std::sin(ang), 0.0f, std::cos(ang) };
        Vector3 ala = { rumbo.z, 0.0f, -rumbo.x };
        float aleteo = 0.25f * std::sin(t * 2.2f + (float)b);
        Color pluma = TinteE(Color{ 56, 40, 40, 255 }, c);
        Vector3 cola = SumaE(c, EscalaE(rumbo, -0.5f));
        Vector3 pico = SumaE(c, EscalaE(rumbo, 0.45f));

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 punta = SumaE(c, SumaE(EscalaE(ala, 1.5f * (float)lado), { 0.0f, 0.35f + aleteo, 0.0f }));
            Vector3 medio = SumaE(c, SumaE(EscalaE(ala, 0.7f * (float)lado), { 0.0f, 0.1f + aleteo * 0.4f, 0.0f }));
            TrianguloE(SumaE(c, EscalaE(rumbo, 0.3f)), medio, SumaE(c, EscalaE(rumbo, -0.3f)), pluma);
            TrianguloE(SumaE(medio, EscalaE(rumbo, 0.2f)), punta, SumaE(medio, EscalaE(rumbo, -0.25f)), pluma);
        }

        TrianguloE(pico, SumaE(c, EscalaE(ala, 0.15f)), cola, pluma);
        TrianguloE(pico, SumaE(c, EscalaE(ala, -0.15f)), cola, pluma);
    }
}


// Plantas rodadoras que cruzan la cornisa empujadas por el viento.
static void DibujarRodadorasE(const CanonCacheE& cache, float sCamara, float t)
{
    for (int r = 0; r < 4; r++)
    {
        int lado = r % 2;
        float recorrido = std::fmod(t * (1.8f + 0.4f * (float)r) + 17.0f * (float)r, 70.0f);
        float s = sCamara - 12.0f + recorrido;
        int k = (int)((s - S_INICIO_CANON_E) / PASO_CANON_E + 0.5f);
        if (k < 0 || k >= MUESTRAS_CANON_E) continue;

        int nivel = r < 2 ? 1 : 2;
        float lateral = nivel == 1 ? 5.6f + 0.6f * (float)r : LateralTerrazaE(k, lado, 1) + 1.5f;
        Vector3 pie = PuntoParedE(cache, k, lado, lateral, AlturaTerrazaE(k, lado, nivel));
        float bote = std::fabs(std::sin(t * 3.2f + (float)r)) * 0.35f;
        Vector3 c = { pie.x, pie.y + 0.38f + bote, pie.z };
        float giro = t * 4.0f + (float)r;
        Color rama = TinteE(Color{ 170, 140, 96, 255 }, c);

        for (int q = 0; q < 5; q++)
        {
            float a = giro + 1.2566f * (float)q;
            Vector3 d = { std::cos(a) * 0.38f, std::sin(a) * 0.38f, std::sin(a * 0.7f + (float)q) * 0.2f };
            Vector3 eje = { cache.marcos[k].tx, 0.0f, cache.marcos[k].tz };
            Vector3 lat = { cache.marcos[k].nx, 0.0f, cache.marcos[k].nz };
            Vector3 p0 = SumaE(c, SumaE(EscalaE(eje, d.x), SumaE({ 0.0f, d.y, 0.0f }, EscalaE(lat, d.z))));
            Vector3 p1 = SumaE(c, SumaE(EscalaE(eje, -d.x), SumaE({ 0.0f, -d.y, 0.0f }, EscalaE(lat, -d.z))));
            VigaE(p0, p1, 0.05f, rama, false);
        }

        SombraLargaE(pie, 0.7f, 0.4f, 0.16f);
    }
}


//==================================================
// VISUAL: LUCES ADITIVAS
//==================================================

static void DibujarLucesE(const MinijuegoEsferasCanon& m, const FaroE faros[], int cantidadFaros, float t, bool decoracion, float sCamara)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    for (int k = 0; k < cantidadFaros; k++)
    {
        float parpadeo = 0.86f + 0.09f * std::sin(t * 13.0f + (float)k * 2.3f) + 0.05f * std::sin(t * 29.0f + (float)k);
        Vector3 llama = faros[k].llama;
        DiscoFrenteE(llama, 1.4f * parpadeo, Color{ 255, 150, 70, 60 });
        DiscoFrenteE(llama, 0.5f * parpadeo, Color{ 255, 226, 160, 150 });
        Vector3 suelo = { faros[k].suelo.x, faros[k].suelo.y + 0.03f, faros[k].suelo.z };
        ConoLuzE(llama, suelo, 1.4f, Color{ 255, 170, 90, 12 });

        for (int r = 0; r < 3; r++)
        {
            DiscoSueloE({ suelo.x, suelo.y + 0.004f * (float)r, suelo.z }, (1.0f + 0.9f * (float)r) * parpadeo, Color{ 255, 160, 80, 30 });
        }
    }

    // Brillo del acento sobre la rampa.
    Vector3 rampa = PuntoCanonE(m, 0.5f * (RAMPA_S0_ESFERAS + RAMPA_S1_ESFERAS), 0.5f * (RAMPA_LAT0_ESFERAS + RAMPA_LAT1_ESFERAS), 0.6f);
    DiscoSueloE(rampa, 2.0f + 0.2f * std::sin(t * 4.0f), Color{ 60, 220, 200, 26 });

    // Polvo dorado suspendido en la luz de la tarde.
    if (decoracion)
    {
        int cantidad = (int)(46.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int q = 0; q < cantidad; q++)
        {
            float s = sCamara - 6.0f + std::fmod(Ruido01Esferas(q, 1301) * 40.0f + t * 0.6f, 40.0f);
            float lateral = -6.0f + 12.0f * Ruido01Esferas(q, 1302) + 0.6f * std::sin(t * 0.7f + (float)q);
            float altura = 0.8f + 6.0f * Ruido01Esferas(q, 1303) + 0.4f * std::sin(t * 0.9f + (float)q * 1.3f);
            Vector3 p = PuntoCanonE(m, s, lateral, altura);
            float titila = 0.5f + 0.5f * std::sin(t * (1.5f + Ruido01Esferas(q, 1304)) + (float)q);
            DiscoFrenteE(p, 0.06f + 0.05f * Ruido01Esferas(q, 1305), Color{ 255, 220, 150, CanalE(60.0f + 90.0f * titila) });
        }
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: PARTICULAS Y EVENTOS
//==================================================

static float AzarVisualE(MinijuegoEsferasCanon& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasE(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return n < 1 ? 1 : n;
}


static void EmitirParticulaE(MinijuegoEsferasCanon& m, Vector3 posicion, Vector3 velocidad, Color color, float vida, float tamano, float gravedad, bool brilla, bool polvo)
{
    int indice = 0;
    float menor = 1000.0f;

    for (int i = 0; i < MAX_PARTICULAS_ESFERAS; i++)
    {
        if (!m.particulas[i].activa)
        {
            indice = i;
            break;
        }

        if (m.particulas[i].vida < menor)
        {
            menor = m.particulas[i].vida;
            indice = i;
        }
    }

    ParticulaEsferas& p = m.particulas[indice];
    p.activa = true;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.color = color;
    p.vida = vida;
    p.vidaMaxima = vida;
    p.tamano = tamano;
    p.gravedad = gravedad;
    p.brilla = brilla;
    p.polvo = polvo;
}


static void NubePolvoE(MinijuegoEsferasCanon& m, Vector3 p, int base, Color color, float fuerza)
{
    int cantidad = CantidadParticulasE(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualE(m, 0.0f, 6.2832f);
        float v = AzarVisualE(m, 0.4f, 1.6f) * fuerza;
        Vector3 vel = { std::cos(a) * v, AzarVisualE(m, 0.3f, 1.2f) * fuerza, std::sin(a) * v };
        EmitirParticulaE(m, p, vel, color, AzarVisualE(m, 0.6f, 1.1f), AzarVisualE(m, 0.25f, 0.5f), -0.3f, false, true);
    }
}


static void EsquirlasE(MinijuegoEsferasCanon& m, Vector3 p, int base, Color color, float fuerza)
{
    int cantidad = CantidadParticulasE(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualE(m, 0.0f, 6.2832f);
        float v = AzarVisualE(m, 1.0f, 3.2f) * fuerza;
        Vector3 vel = { std::cos(a) * v, AzarVisualE(m, 1.5f, 4.0f) * fuerza, std::sin(a) * v };
        Color c = k % 3 == 0 ? COLOR_CREMA_E : color;
        EmitirParticulaE(m, p, vel, c, AzarVisualE(m, 0.4f, 0.8f), AzarVisualE(m, 0.035f, 0.07f), 12.0f, false, false);
    }
}


static void ConfetiE(MinijuegoEsferasCanon& m, Vector3 p, Color color, int base)
{
    int cantidad = CantidadParticulasE(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualE(m, 0.0f, 6.2832f);
        float v = AzarVisualE(m, 0.6f, 2.8f);
        Vector3 vel = { std::cos(a) * v, AzarVisualE(m, 3.0f, 6.5f), std::sin(a) * v };
        Color c = k % 3 == 0 ? color : (k % 3 == 1 ? COLOR_TURQUESA_E : Color{ 250, 236, 200, 255 });
        EmitirParticulaE(m, p, vel, c, AzarVisualE(m, 1.2f, 2.2f), AzarVisualE(m, 0.035f, 0.06f), 3.2f, false, false);
    }
}


static Vector3 PieEsferaE(const MinijuegoEsferasCanon& m, int i)
{
    const EstadoJugadorEsferas& e = m.estadosJugadores[i];
    return { e.x, ElevacionPistaEsferas(e.s), e.z };
}


static void ActualizarVisualEsferas(
    MinijuegoEsferasCanon& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;
    m.tiempoVisual += deltaTime;

    if (m.anuncioInicio >= 0.0f)
    {
        m.anuncioInicio += deltaTime;
        if (m.anuncioInicio > 3.4f) m.anuncioInicio = -1.0f;
    }

    if (m.anuncioMeta >= 0.0f)
    {
        m.anuncioMeta += deltaTime;
        if (m.anuncioMeta > 3.0f) m.anuncioMeta = -1.0f;
    }

    bool jugando = m.fase == FASE_ESFERAS_JUGANDO;

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorEsferas& e = m.estadosJugadores[i];
        VisualJugadorEsferas& v = m.visualJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        bool cayendo = e.penalizacion > PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS;
        bool enAire = e.tiempoAire > 0.0f;
        Color color = participantes[i].color;
        Vector3 pie = PieEsferaE(m, i);

        if (jugando)
        {
            float dvx = e.velocidadX - v.velocidadX;
            float dvz = e.velocidadZ - v.velocidadZ;
            float dv = std::sqrt(dvx * dvx + dvz * dvz);

            if (cayendo && !v.cayendo)
            {
                Vector3 borde = { e.caidaX, pie.y + 0.1f, e.caidaZ };
                EsquirlasE(m, borde, 14, COLOR_ROCA_CLARA_E, 0.7f);
                NubePolvoE(m, borde, 6, COLOR_CREMA_E, 0.8f);
                v.avisoCaida = 1.5f;
            }
            else if (!cayendo && v.cayendo)
            {
                NubePolvoE(m, pie, 8, COLOR_ARENA_E, 1.0f);
                v.destello = 1.0f;
            }

            if (enAire && !v.enAire)
            {
                NubePolvoE(m, pie, 8, COLOR_ARENA_E, 1.2f);
                EsquirlasE(m, { pie.x, pie.y + 0.6f, pie.z }, 8, COLOR_MADERA_E, 0.8f);
                v.avisoAtajo = 1.4f;
            }
            else if (!enAire && v.enAire)
            {
                NubePolvoE(m, pie, 12, COLOR_ARENA_E, 1.5f);
            }

            if (e.checkpoints > v.checkpoints && e.checkpoints <= CANTIDAD_CHECKPOINTS_ESFERAS)
            {
                float s = CHECKPOINTS_ESFERAS[e.checkpoints - 1];
                ConfetiE(m, PuntoCanonE(m, s, -4.7f, 4.8f), color, 14);
                ConfetiE(m, PuntoCanonE(m, s, 4.7f, 4.8f), color, 14);
                v.avisoControl = 1.4f;
                v.destello = 1.0f;
            }

            if (e.terminado && !v.terminado)
            {
                ConfetiE(m, PuntoCanonE(m, LONGITUD_META_ESFERAS, -4.85f, 5.6f), color, 22);
                ConfetiE(m, PuntoCanonE(m, LONGITUD_META_ESFERAS, 4.85f, 5.6f), color, 22);
                v.destello = 1.0f;

                if (m.jugadorMeta < 0)
                {
                    m.jugadorMeta = i;
                    m.anuncioMeta = 0.0f;
                }
            }

            // Impulso propio: rafaga de polvo detras.
            if (e.recargaImpulso > v.recarga + 1.0f)
            {
                Vector3 atras = { pie.x - e.velocidadX * 0.08f, pie.y + 0.2f, pie.z - e.velocidadZ * 0.08f };
                NubePolvoE(m, atras, 8, COLOR_CREMA_E, 1.1f);
            }
            else if (!cayendo && !enAire && !v.enAire && !v.cayendo && dv > 2.6f && v.avisoChoque <= 0.0f && e.penalizacion <= 0.0f)
            {
                // Choque (pared, roca, cactus u otra esfera): esquirlas del lado del golpe.
                Vector3 contacto = { pie.x - dvx / dv * 0.9f, pie.y + 0.7f, pie.z - dvz / dv * 0.9f };
                EsquirlasE(m, contacto, dv > 5.0f ? 12 : 7, COLOR_ROCA_E, 0.6f + 0.1f * dv);
                v.avisoChoque = 0.2f;
                v.destello = dv > 5.0f ? 0.8f : 0.4f;
            }

            // Polvo al rodar: mucho en la arena, apenas en el lecho firme.
            float rapidez = std::sqrt(e.velocidadX * e.velocidadX + e.velocidadZ * e.velocidadZ);
            bool arena = EnArenaEsferas(e.s);

            if (!enAire && !cayendo && e.penalizacion <= 0.0f && rapidez > (arena ? 1.2f : 7.0f))
            {
                v.polvoArena -= deltaTime * (arena ? 12.0f : 3.0f) * FactorCalidadGrafica(CalidadParticulas());

                if (v.polvoArena <= 0.0f)
                {
                    v.polvoArena += 1.0f;
                    Vector3 atras = { pie.x - e.velocidadX / rapidez * 0.7f, pie.y + 0.15f, pie.z - e.velocidadZ / rapidez * 0.7f };
                    Vector3 vel = { AzarVisualE(m, -0.5f, 0.5f), AzarVisualE(m, 0.4f, 1.0f), AzarVisualE(m, -0.5f, 0.5f) };
                    EmitirParticulaE(m, atras, vel, arena ? COLOR_ARENA_E : COLOR_CREMA_E, AzarVisualE(m, 0.5f, 0.9f), AzarVisualE(m, 0.25f, 0.45f), -0.2f, false, true);
                }
            }
        }

        v.enAire = enAire;
        v.cayendo = cayendo;
        v.terminado = e.terminado;
        v.checkpoints = e.checkpoints;
        v.velocidadX = e.velocidadX;
        v.velocidadZ = e.velocidadZ;
        v.recarga = e.recargaImpulso;

        if (v.destello > 0.0f) v.destello -= deltaTime * 2.5f;
        if (v.avisoControl > 0.0f) v.avisoControl -= deltaTime;
        if (v.avisoAtajo > 0.0f) v.avisoAtajo -= deltaTime;
        if (v.avisoCaida > 0.0f) v.avisoCaida -= deltaTime;
        if (v.avisoChoque > 0.0f) v.avisoChoque -= deltaTime;
    }

    // Confeti sobre el ganador.
    if (m.fase == FASE_ESFERAS_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO && !m.confetiFinal)
    {
        m.confetiFinal = true;

        for (int i = 0; i < limite; i++)
        {
            if (m.estadosJugadores[i].participa && m.resultado.participantes[i].posicionFinal == 1)
            {
                Vector3 p = { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.4f, jugadores[i].posicion.z };
                ConfetiE(m, p, participantes[i].color, 50);
            }
        }
    }

    for (int k = 0; k < MAX_PARTICULAS_ESFERAS; k++)
    {
        ParticulaEsferas& p = m.particulas[k];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        if (p.polvo)
        {
            float freno = 1.0f - 1.8f * deltaTime;
            p.velocidad.x *= freno;
            p.velocidad.z *= freno;
            p.tamano += deltaTime * 0.5f;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;
    }
}


void MinijuegoEsferasCanon::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    FaseEsferasCanon faseAntes = fase;
    ActualizarReglas(deltaTime, jugadores, cantidadMaxima, participantes);

    if (faseAntes == FASE_ESFERAS_PREPARACION && fase == FASE_ESFERAS_JUGANDO)
    {
        anuncioInicio = 0.0f;
    }

    if (partidaValida)
    {
        ActualizarVisualEsferas(*this, deltaTime, jugadores, participantes, LimiteJugadoresEsferas(cantidadMaxima));
    }
}


static void DibujarParticulasE(const MinijuegoEsferasCanon& m)
{
    // Solidas primero (esquirlas, confeti); despues el polvo translucido.
    BeginE(RL_QUADS);
    for (int k = 0; k < MAX_PARTICULAS_ESFERAS; k++)
    {
        const ParticulaEsferas& p = m.particulas[k];
        if (!p.activa || p.polvo) continue;
        QuadFrenteE(p.posicion, p.tamano, p.color);
    }
    EndE();

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int k = 0; k < MAX_PARTICULAS_ESFERAS; k++)
    {
        const ParticulaEsferas& p = m.particulas[k];
        if (!p.activa || !p.polvo) continue;
        float u = p.vida / p.vidaMaxima;
        DiscoFrenteE(p.posicion, p.tamano, AlfaE(p.color, 0.5f * u));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: PIEZAS JUGABLES (GLB CON RESPALDO DE PRIMITIVAS)
//==================================================

static void DibujarObstaculoEsferas(const ObstaculoEsferas& obstaculo, float elevacion, bool sombras)
{
    Vector3 pie = { obstaculo.x, elevacion, obstaculo.z };

    // Sombra de contacto y sombra larga de la tarde.
    DiscoSueloE({ pie.x, pie.y + 0.02f, pie.z }, obstaculo.radio * 1.1f, Color{ 60, 28, 40, 110 });
    if (sombras) SombraLargaE(pie, obstaculo.radio * 1.8f, obstaculo.cactus ? 1.7f : 1.2f, 0.26f);

    float escala = obstaculo.cactus ? 1.0f : obstaculo.radio / 0.9f;
    if (DibujarModeloEsferasCanonRetro3D(obstaculo.cactus ? MODELO_ESFERAS_CACTUS : MODELO_ESFERAS_ROCA,
        pie, 0, {0,1,0}, {escala,escala,escala})) return;

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
    }
    if (!DibujarModeloEsferasCanonRetro3D(MODELO_ESFERAS_ARO,
        {centro.x,centro.y - RADIO_ESFERA,centro.z}, 0, {0,1,0}, {1,1,1}, Fade(colorJugador,0.7f)))
        DrawCircle3D({ centro.x, centro.y - RADIO_ESFERA + 0.03f, centro.z }, RADIO_ESFERA + 0.15f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(colorJugador, 0.7f));
}


// Sombra de cada esfera en el lecho (se achica y aclara en el salto).
static void DibujarSombrasEsferasE(const MinijuegoEsferasCanon& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorEsferas& e = m.estadosJugadores[i];
        if (!e.participa || e.penalizacion > PENALIZACION_ESFERAS - TIEMPO_CAIDA_VISUAL_ESFERAS) continue;

        bool sobreGrieta = false;
        for (int g = 0; g < CANTIDAD_GRIETAS_ESFERAS; g++)
        {
            const GrietaEsferas& grieta = GRIETAS_ESFERAS[g];
            if (e.s >= grieta.s0 && e.s <= grieta.s1 && std::fabs(e.lateral - grieta.centroSeguro) > grieta.mitadSegura) sobreGrieta = true;
        }

        if (sobreGrieta) continue;

        float alturaAire = e.tiempoAire > 0.0f
            ? ALTURA_SALTO_ESFERAS * std::sin(3.14159265f * (1.0f - e.tiempoAire / TIEMPO_AIRE_ESFERAS))
            : 0.0f;
        float u = 1.0f - Limitar01E(alturaAire / 3.0f) * 0.5f;
        Vector3 pie = { e.x, ElevacionPistaEsferas(e.s) + 0.035f, e.z };
        DiscoSueloE(pie, 1.05f * u, Color{ 50, 22, 36, CanalE(140.0f * u) });
        SombraLargaE({ e.x, pie.y, e.z }, 1.8f * u, 2.0f * u, 0.22f * u);
    }
}


//==================================================
// VISUAL: HORNEADO DEL CANON ESTATICO
//==================================================
//
// Paredes, meseta, lecho, arena, utileria, buttes, horizonte y arco no se
// mueven: se graban una vez (por calidad) en mallas por tramos de pista y
// se dibujan con un shader de niebla por pixel. Cada cuadro solo se
// arman a mano las piezas animadas (puentes, porticos, rampa, banderines).

static const int TRAMO_HORNO_E = 8;
static const int TRAMOS_HORNO_E = (MUESTRAS_CANON_E - 1 + TRAMO_HORNO_E - 1) / TRAMO_HORNO_E;

struct HornoCanonE
{
    bool listo = false;
    int clave = -1;
    Mesh tramos[TRAMOS_HORNO_E]{};      // paredes y lecho
    bool tramoValido[TRAMOS_HORNO_E]{};
    Mesh utileria[TRAMOS_HORNO_E]{};    // cactus, piedras y sombras (van despues)
    bool utileriaValida[TRAMOS_HORNO_E]{};
    float sTramo[TRAMOS_HORNO_E + 1]{};
    Mesh global{};                      // horizonte, buttes, cierre y arco
    bool globalValido = false;
    Mesh arenas{};                      // traslucidas: sobre el lecho
    bool arenasValidas = false;
    bool shaderListo = false;
    Shader shader{};
    Material material{};
    int locOjo = -1;
    int locNiebla = -1;
};

static HornoCanonE g_hornoE;


static const char* VS_NIEBLA_E =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec4 fragColor;\n"
    "out vec3 fragPos;\n"
    "void main()\n"
    "{\n"
    "    fragColor = vertexColor;\n"
    "    fragPos = vertexPosition;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char* FS_NIEBLA_E =
    "#version 330\n"
    "in vec4 fragColor;\n"
    "in vec3 fragPos;\n"
    "uniform vec3 ojo;\n"
    "uniform vec4 niebla;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    float n = clamp((distance(fragPos, ojo) - 30.0) / 230.0, 0.0, 0.9);\n"
    "    finalColor = vec4(mix(fragColor.rgb, niebla.rgb, n), fragColor.a);\n"
    "}\n";


static void EmpezarGrabacionE()
{
    g_grabacionE.enQuad = 0;
    EmpezarGrabacionMalla(g_grabacionE.malla);
}


static bool TerminarGrabacionE(Mesh& malla)
{
    return TerminarGrabacionMalla(g_grabacionE.malla, malla);
}


// Libera las mallas horneadas (se vuelven a grabar al dibujar).
static void LiberarMallasHornoE()
{
    for (int c = 0; c < TRAMOS_HORNO_E; c++)
    {
        LiberarMallaHorneada(g_hornoE.tramos[c], g_hornoE.tramoValido[c]);
        LiberarMallaHorneada(g_hornoE.utileria[c], g_hornoE.utileriaValida[c]);
    }

    LiberarMallaHorneada(g_hornoE.global, g_hornoE.globalValido);
    LiberarMallaHorneada(g_hornoE.arenas, g_hornoE.arenasValidas);
    g_hornoE.listo = false;
    g_hornoE.clave = -1;
}


static int ClaveHornoE()
{
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool completo = CalidadDecoracion() == CALIDAD_ALTA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    return (decoracion ? 1 : 0) | (completo ? 2 : 0) | (sombras ? 4 : 0);
}


// Graba el canon estatico para la calidad actual (solo si cambio).
static void HornearCanonE(const MinijuegoEsferasCanon& m)
{
    int clave = ClaveHornoE();

    if (g_hornoE.listo && g_hornoE.clave == clave)
    {
        return;
    }

    if (!IsWindowReady())
    {
        return;
    }

    LiberarMallasHornoE();

    if (!g_hornoE.shaderListo)
    {
        g_hornoE.shader = LoadShaderFromMemory(VS_NIEBLA_E, FS_NIEBLA_E);
        g_hornoE.locOjo = GetShaderLocation(g_hornoE.shader, "ojo");
        g_hornoE.locNiebla = GetShaderLocation(g_hornoE.shader, "niebla");
        g_hornoE.material = LoadMaterialDefault();
        g_hornoE.material.shader = g_hornoE.shader;
        g_hornoE.shaderListo = true;
    }

    bool decoracion = (clave & 1) != 0;
    bool completo = (clave & 2) != 0;
    bool sombras = (clave & 4) != 0;
    int paso = completo ? 1 : 2;

    static CanonCacheE cache;
    PrepararCanonE(m, cache);

    for (int c = 0; c < TRAMOS_HORNO_E; c++)
    {
        int k0 = c * TRAMO_HORNO_E;
        int k1 = k0 + TRAMO_HORNO_E;
        if (k1 > MUESTRAS_CANON_E - 1) k1 = MUESTRAS_CANON_E - 1;
        float sA = c == 0 ? -1.0e9f : SMuestraE(k0);
        float sB = c == TRAMOS_HORNO_E - 1 ? 1.0e9f : SMuestraE(k1) - 0.001f;
        g_hornoE.sTramo[c] = SMuestraE(k0);
        g_hornoE.sTramo[c + 1] = SMuestraE(k1);

        EmpezarGrabacionE();
        DibujarParedesCanonE(cache, k0, k1, decoracion);
        DibujarLechoE(m, cache, k0, k1, decoracion);
        g_hornoE.tramoValido[c] = TerminarGrabacionE(g_hornoE.tramos[c]);

        if (decoracion)
        {
            EmpezarGrabacionE();
            DibujarUtileriaE(cache, sA, sB, paso, sombras);
            g_hornoE.utileriaValida[c] = TerminarGrabacionE(g_hornoE.utileria[c]);
        }
    }

    float yLejos = cache.marcos[MUESTRAS_CANON_E - 1].c.y + AlturaTerrazaE(MUESTRAS_CANON_E - 1, 0, 3) - 0.2f;
    EmpezarGrabacionE();
    DibujarHorizonteE(yLejos, decoracion, completo);
    if (decoracion) DibujarButtesE(cache, -1.0e9f, 1.0e9f, paso);
    DibujarCierreCanonE(cache);
    DibujarArcoNaturalE(m);
    g_hornoE.globalValido = TerminarGrabacionE(g_hornoE.global);

    EmpezarGrabacionE();
    DibujarArenasE(m, -1.0e9f, 1.0e9f, decoracion);
    g_hornoE.arenasValidas = TerminarGrabacionE(g_hornoE.arenas);

    g_hornoE.clave = clave;
    g_hornoE.listo = true;
}


static void UsarShaderHornoE()
{
    rlDrawRenderBatchActive();
    float ojo[3] = { g_ojoE.x, g_ojoE.y, g_ojoE.z };
    float niebla[4] = { COLOR_NIEBLA_E.r / 255.0f, COLOR_NIEBLA_E.g / 255.0f, COLOR_NIEBLA_E.b / 255.0f, 1.0f };
    SetShaderValue(g_hornoE.shader, g_hornoE.locOjo, ojo, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_hornoE.shader, g_hornoE.locNiebla, niebla, SHADER_UNIFORM_VEC4);
}


// Primer tramo que puede verse (el que queda bajo la camara o detras).
static int PrimerTramoHornoE(float sDesde)
{
    for (int c = 0; c < TRAMOS_HORNO_E; c++)
    {
        if (g_hornoE.sTramo[c + 1] >= sDesde) return c;
    }

    return TRAMOS_HORNO_E - 1;
}


// Lo opaco: global, paredes y lecho de los tramos visibles (de cerca a
// lejos) y despues las arenas traslucidas encima del lecho.
static void DibujarHornoE(float sDesde)
{
    if (!g_hornoE.listo)
    {
        return;
    }

    UsarShaderHornoE();
    Matrix identidad = MatrixIdentity();

    for (int c = PrimerTramoHornoE(sDesde); c < TRAMOS_HORNO_E; c++)
    {
        if (g_hornoE.tramoValido[c]) DrawMesh(g_hornoE.tramos[c], g_hornoE.material, identidad);
    }

    if (g_hornoE.globalValido) DrawMesh(g_hornoE.global, g_hornoE.material, identidad);
    if (g_hornoE.arenasValidas) DrawMesh(g_hornoE.arenas, g_hornoE.material, identidad);
}


// Utileria de los tramos entre sDesde y sHasta (con sus sombras suaves).
static void DibujarUtileriaHornoE(float sDesde, float sHasta)
{
    if (!g_hornoE.listo)
    {
        return;
    }

    UsarShaderHornoE();
    Matrix identidad = MatrixIdentity();

    for (int c = PrimerTramoHornoE(sDesde); c < TRAMOS_HORNO_E; c++)
    {
        if (g_hornoE.sTramo[c] > sHasta) break;
        if (g_hornoE.utileriaValida[c]) DrawMesh(g_hornoE.utileria[c], g_hornoE.material, identidad);
    }
}


//==================================================
// DIBUJAR
//==================================================

static void OrdenCarreraE(const MinijuegoEsferasCanon& m, int limite, int posicion[MAX_PARTICIPANTES])
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        posicion[i] = 0;
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorEsferas& a = m.estadosJugadores[i];
        if (!a.participa) continue;
        int delante = 0;

        for (int j = 0; j < limite; j++)
        {
            const EstadoJugadorEsferas& b = m.estadosJugadores[j];
            if (j == i || !b.participa) continue;

            bool antes;
            if (a.terminado != b.terminado) antes = b.terminado;
            else if (a.terminado) antes = b.tiempoMeta < a.tiempoMeta || (b.tiempoMeta == a.tiempoMeta && j < i);
            else antes = b.progresoMaximo > a.progresoMaximo + 0.001f || (std::fabs(b.progresoMaximo - a.progresoMaximo) <= 0.001f && j < i);

            if (antes) delante++;
        }

        posicion[i] = delante + 1;
    }
}


static void FormatoSegundosE(char* destino, int tam, float segundos)
{
    std::snprintf(destino, (size_t)tam, "%.1f S", segundos);

    for (char* c = destino; *c; c++)
    {
        if (*c == '.') *c = ',';
    }
}


void MinijuegoEsferasCanon::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteJugadoresEsferas(cantidadMaxima);
    const float t = tiempoVisual;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool completo = CalidadDecoracion() == CALIDAD_ALTA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    int paso = completo ? 1 : 2;


    // Vista: la camara de la logica, mirando un poco mas adelante por la
    // bajada; en la cuenta llega desde mas alto y atras.
    Camera3D vista = camara;
    {
        ProyeccionEsferas p = ProyectarPistaEsferas(*this, camara.target.x, camara.target.z);
        MarcoPistaE f = MarcoPistaCanonE(*this, p.s);
        // Con el grupo disperso (camara alta) casi no se adelanta: el
        // ultimo tiene que seguir en cuadro.
        float altura = camara.position.y - camara.target.y;
        float adelanto = 7.0f * Limitar01E(8.0f / (altura > 1.0f ? altura : 1.0f));
        adelanto *= adelanto / 7.0f;
        vista.target.x += f.tx * adelanto;
        vista.target.z += f.tz * adelanto;
        vista.target.y -= adelanto * 0.14f;
    }

    if (fase == FASE_ESFERAS_PREPARACION && resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Limitar01E((tiempoPreparacion - 0.4f) / (DURACION_PREPARACION_ESFERAS - 0.4f));
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 7.0f * suave;
        vista.position.z += 7.0f * suave;
        vista.target.z -= 6.0f * suave;
    }

    g_ojoE = vista.position;
    {
        Vector3 frente = NormalizarE(RestaE(vista.target, vista.position));
        g_frenteUE = NormalizarE(CruzE(frente, { 0.0f, 1.0f, 0.0f }));
        g_frenteVE = CruzE(g_frenteUE, frente);
    }

    float sCamara = ProyectarPistaEsferas(*this, vista.target.x, vista.target.z).s;
    float sOjo = ProyectarPistaEsferas(*this, vista.position.x, vista.position.z).s;
    if (vista.position.z > 0.0f) sOjo = -vista.position.z;
    float sMin = sCamara - 16.0f;
    if (sOjo - 6.0f < sMin) sMin = sOjo - 6.0f;
    float sMax = sCamara + 95.0f;

    // El bucle del juego no limpia la pantalla.
    ClearBackground(COLOR_CIELO_ALTO_E);
    DibujarCieloE(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Canon estatico horneado (horizonte, buttes, paredes, lecho, arenas,
    // arco): desde el tramo bajo la camara hasta el fondo.
    HornearCanonE(*this);
    bool horneado = g_hornoE.listo;

    if (horneado)
    {
        DibujarHornoE(sMin - 10.0f);
    }
    else
    {
        // Sin ventana/GL (no deberia pasar): armado inmediato.
        static CanonCacheE cache;
        PrepararCanonE(*this, cache);
        float yLejos = cache.marcos[MUESTRAS_CANON_E - 1].c.y + AlturaTerrazaE(MUESTRAS_CANON_E - 1, 0, 3) - 0.2f;
        DibujarHorizonteE(yLejos, decoracion, completo);
        if (decoracion) DibujarButtesE(cache, sMin, sMax, paso);
        int k0 = (int)((sMin - 10.0f - S_INICIO_CANON_E) / PASO_CANON_E);
        if (k0 < 0) k0 = 0;
        DibujarParedesCanonE(cache, k0, MUESTRAS_CANON_E - 1, decoracion);
        DibujarCierreCanonE(cache);
        DibujarLechoE(*this, cache, k0, MUESTRAS_CANON_E - 1, decoracion);
        DibujarArenasE(*this, sMin, sMax + 40.0f, decoracion);
        DibujarArcoNaturalE(*this);
        if (decoracion) DibujarUtileriaE(cache, sMin, sMax, paso, sombras);
    }

    static CanonCacheE cache;
    PrepararCanonE(*this, cache);

    DibujarGrietasE(*this, t, sMin, sMax + 40.0f, decoracion);
    DibujarRampaE(*this, t, decoracion);

    FaroE faros[MAX_FAROS_E];
    int cantidadFaros = 0;
    DibujarPorticosE(*this, participantes, limite, t, faros, cantidadFaros, sMin, sMax + 40.0f);

    for (int g = 0; g < CANTIDAD_GRIETAS_ESFERAS; g++)
    {
        const GrietaEsferas& grieta = GRIETAS_ESFERAS[g];
        if (grieta.s1 < sMin || grieta.s0 > sMax + 40.0f) continue;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 pie = PuntoCanonE(*this, grieta.s0 - 1.3f, (float)lado * 4.3f, 0.0f);
            PosteFarolE(pie, 1.6f, t, g * 2 + lado, faros, cantidadFaros);
        }
    }

    if (decoracion)
    {
        if (horneado) DibujarUtileriaHornoE(sMin, sMax);
        DibujarRodadorasE(cache, sCamara, t);
        DibujarBuitresE(*this, sCamara, t);
    }

    // Obstaculos (todos: son pocos y su orden lo siguen los GLB).
    for (int k = 0; k < cantidadObstaculos; k++)
    {
        DibujarObstaculoEsferas(obstaculos[k], ElevacionPistaEsferas(obstaculos[k].s), sombras);
    }

    DibujarSombrasEsferasE(*this, limite);

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
        DibujarJugadorModeloCompartido(jugador, participanteVisual);

        if (mostrarDebug)
        {
            DrawSphereWires(centro, RADIO_ESFERA, 8, 8, LIME);
        }
    }

    DibujarParticulasE(*this);

    if (efectos)
    {
        DibujarLucesE(*this, faros, cantidadFaros, t, decoracion, sCamara);

        // Destello del color del jugador en cada acierto o golpe.
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        BeginBlendMode(BLEND_ADDITIVE);

        for (int i = 0; i < limite; i++)
        {
            float d = visualJugadores[i].destello;
            if (!estadosJugadores[i].participa || d <= 0.0f) continue;
            Vector3 c = { jugadores[i].posicion.x, jugadores[i].posicion.y - 1.7f, jugadores[i].posicion.z };
            Color col = participantes[i].color;
            DiscoFrenteE(c, 1.6f + 0.6f * (1.0f - d), Color{ col.r, col.g, col.b, CanalE(110.0f * d) });
        }

        EndBlendMode();
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Cartelitos J1..J4 y avisos sobre cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (!estadosJugadores[i].participa || !participantes[i].activo)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.8f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        Rectangle placa = DibujarEtiquetaUI(rotulo, p, 0.0f, op);

        const VisualJugadorEsferas& v = visualJugadores[i];
        float yAviso = placa.y - Px(18.0f);
        float subir = ObtenerFactorMovimientoCamaraMinijuegos();

        if (v.avisoCaida > 0.0f)
        {
            float u = 1.0f - v.avisoCaida / 1.5f;
            DibujarTextoMundoContornoUI("¡AL VACÍO!", p.x, yAviso - u * Px(22.0f) * subir, UI_TEXTO, Color{ 255, 136, 120, 255 }, Limitar01E(v.avisoCaida / 0.4f));
        }
        else if (v.avisoAtajo > 0.0f)
        {
            float u = 1.0f - v.avisoAtajo / 1.4f;
            DibujarTextoMundoContornoUI("¡ATAJO!", p.x, yAviso - u * Px(22.0f) * subir, UI_TEXTO, Color{ 120, 236, 222, 255 }, Limitar01E(v.avisoAtajo / 0.4f));
        }
        else if (v.avisoControl > 0.0f)
        {
            float u = 1.0f - v.avisoControl / 1.4f;
            DibujarTextoMundoContornoUI("¡CONTROL!", p.x, yAviso - u * Px(22.0f) * subir, UI_TEXTO_PEQUENO, Color{ 255, 232, 170, 255 }, Limitar01E(v.avisoControl / 0.4f));
        }
    }

    // Anuncios grandes.
    // Despues del «¡YA!» del HUD, la pista de como correr.
    if (anuncioInicio >= 0.9f && fase == FASE_ESFERAS_JUGANDO)
    {
        float u = (anuncioInicio - 0.9f) / 2.5f;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.75f ? (1.0f - u) / 0.25f : 1.0f);
        float y = (float)alto * 0.27f - u * Px(10.0f) * ObtenerFactorMovimientoCamaraMinijuegos();
        DibujarTextoMundoContornoUI("¡CRUZÁ LAS GRIETAS POR LOS PUENTES!", (float)ancho * 0.5f, y, UI_TEXTO_GRANDE, Color{ 255, 222, 150, 255 }, alfa);
        DibujarTextoMundoContornoUI("LA RAMPA TURQUESA ES UN ATAJO", (float)ancho * 0.5f, y + Px(40.0f), UI_TEXTO, Color{ 140, 240, 226, 255 }, alfa);
    }
    else if (anuncioMeta >= 0.0f && jugadorMeta >= 0 && fase == FASE_ESFERAS_JUGANDO)
    {
        float u = anuncioMeta / 3.0f;
        float alfa = u < 0.06f ? u / 0.06f : (u > 0.8f ? (1.0f - u) / 0.2f : 1.0f);
        float salto = u < 0.1f ? (1.0f - u / 0.1f) * Px(14.0f) * ObtenerFactorMovimientoCamaraMinijuegos() : 0.0f;
        char texto[40];
        std::snprintf(texto, sizeof(texto), "¡J%d LLEGÓ A LA META!", participantes[jugadorMeta].numeroJugador > 0 ? participantes[jugadorMeta].numeroJugador : jugadorMeta + 1);
        float y = (float)alto * 0.3f - salto;
        DibujarTextoMundoContornoUI(texto, (float)ancho * 0.5f, y, UI_TEXTO_TITULO, MezclaE(participantes[jugadorMeta].color, WHITE, 0.3f), alfa);
        DibujarTextoMundoContornoUI("¡LOS DEMÁS TIENEN 12 SEGUNDOS!", (float)ancho * 0.5f, y + Px(52.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)ancho * 0.5f, (float)alto * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "ESFERAS DEL CAÑÓN",
        "RODÁ HASTA LA META: CRUZÁ LAS GRIETAS POR LOS PUENTES",
        resultado
    );

    hud.fase = fase == FASE_ESFERAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_ESFERAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_ESFERAS;

    // Tras la primera llegada, el reloj pasa a ser el cierre de 12 s.
    if (hayGanadorPorMeta && tiempoCierre >= 0.0f && tiempoCierre < tiempoRestante)
    {
        hud.tiempoRestante = tiempoCierre;
        hud.tiempoTotal = TIEMPO_CIERRE_ESFERAS;
    }

    int posicion[MAX_PARTICIPANTES];
    OrdenCarreraE(*this, limite, posicion);

    for (int i = 0; i < limite; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        const EstadoJugadorEsferas& e = estadosJugadores[i];

        if (!h.visible || !e.participa)
        {
            continue;
        }

        std::snprintf(h.valor, sizeof(h.valor), "%dº", posicion[i]);
        h.valorNumerico = posicion[i];
        h.progreso = Limitar01E(e.progresoMaximo / LONGITUD_META_ESFERAS);

        if (e.terminado)
        {
            char tiempo[16];
            FormatoSegundosE(tiempo, sizeof(tiempo), e.tiempoMeta);
            std::snprintf(h.estado, sizeof(h.estado), "META %s", tiempo);
            std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%s", tiempo);
        }
        else
        {
            if (e.penalizacion > 0.0f) std::snprintf(h.estado, sizeof(h.estado), "¡AL VACÍO!");
            else if (e.tiempoAire > 0.0f) std::snprintf(h.estado, sizeof(h.estado), "¡ATAJO!");

            std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d %%", (int)(100.0f * Limitar01E(e.progresoMaximo / LONGITUD_META_ESFERAS)));
        }

        if (!EsControladoPorBotEsferas(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "MOVÉ: %s\nIMPULSO: %s",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (fase == FASE_ESFERAS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        static char nota[56];
        hud.finPorTiempo = !hayGanadorPorMeta;
        nota[0] = '\0';

        for (int i = 0; i < limite; i++)
        {
            if (estadosJugadores[i].participa && resultado.participantes[i].posicionFinal == 1)
            {
                if (estadosJugadores[i].terminado)
                {
                    char tiempo[16];
                    FormatoSegundosE(tiempo, sizeof(tiempo), estadosJugadores[i].tiempoMeta);
                    std::snprintf(nota, sizeof(nota), "LLEGÓ A LA META EN %s", tiempo);
                }
                else
                {
                    std::snprintf(nota, sizeof(nota), "NADIE LLEGÓ: GANA QUIEN RODÓ MÁS LEJOS");
                }
            }
        }

        hud.notaFinal = nota[0] ? nota : nullptr;
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoEsferasCanon::ObtenerResultado() const
{
    return resultado;
}


void MinijuegoEsferasCanon::Descargar()
{
    LiberarMallasHornoE();
    LiberarShaderHorneado(g_hornoE.shader);
    LiberarMaterialHorneado(g_hornoE.material, g_hornoE.shaderListo);
}
