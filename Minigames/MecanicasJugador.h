#pragma once

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


//==================================================
// PERFIL ESTANDAR DEL JUGADOR
//==================================================
//
// Este archivo define las mecanicas compartidas por los
// minijuegos donde el jugador se mueve de forma "normal":
//
// - movimiento directo en X/Z
// - salto
// - golpe al suelo
// - golpe horizontal
// - cooldown del golpe
// - ralentizacion al recibir un golpe
// - particulas amarillas al impactar
// - colision solida entre jugadores SIN empuje de choque
//
// Los minijuegos con fisica especial (por ejemplo Pelotas)
// no deben usar este perfil y pueden seguir usando las
// utilidades de bajo nivel de UtilidadesMinijuegos.
//==================================================


const float VELOCIDAD_JUGADOR_ESTANDAR = 5.0f;
const float FUERZA_SALTO_JUGADOR_ESTANDAR = 7.2f;
const float GRAVEDAD_JUGADOR_ESTANDAR = 18.0f;
const float DURACION_RESPAWN_JUGADOR_ESTANDAR = 1.2f;

const float COOLDOWN_GOLPE_JUGADOR_ESTANDAR = 0.80f;
const float FUERZA_GOLPE_JUGADOR_ESTANDAR = 4.2f;
const float DURACION_RALENTIZACION_GOLPE = 0.70f;
const float MULTIPLICADOR_VELOCIDAD_RALENTIZADO = 0.45f;


//==================================================
// CONFIGURACION
//==================================================

inline void ConfigurarJugadorMinijuegoEstandar(
    JugadorPrueba& jugador,
    Vector3 posicionSpawn
)
{
    jugador.posicionSpawn =
        posicionSpawn;

    jugador.tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };

    jugador.velocidadMovimiento =
        VELOCIDAD_JUGADOR_ESTANDAR;

    jugador.fuerzaSalto =
        FUERZA_SALTO_JUGADOR_ESTANDAR;

    jugador.gravedad =
        GRAVEDAD_JUGADOR_ESTANDAR;

    jugador.duracionRespawn =
        DURACION_RESPAWN_JUGADOR_ESTANDAR;

    jugador.tiempoRalentizado =
        0.0f;

    ReiniciarJugadorPrueba(
        jugador
    );
}


//==================================================
// MOVIMIENTO NORMAL + RALENTIZACION
//==================================================

inline void ActualizarJugadorPruebaNormal(
    JugadorPrueba& jugador,
    const InputMinijuegoParticipante& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    int cantidadParticulas,
    bool permitirSalto,
    bool respawnAutomatico,
    float deltaTime
)
{
    if (jugador.tiempoRalentizado > 0.0f)
    {
        jugador.tiempoRalentizado -=
            deltaTime;

        if (jugador.tiempoRalentizado < 0.0f)
        {
            jugador.tiempoRalentizado =
                0.0f;
        }
    }

    float velocidadOriginal =
        jugador.velocidadMovimiento;

    if (jugador.tiempoRalentizado > 0.0f)
    {
        jugador.velocidadMovimiento *=
            MULTIPLICADOR_VELOCIDAD_RALENTIZADO;
    }

    bool golpeandoAntes =
        jugador.golpeando;

    bool estabaCayendo =
        jugador.cayendo;

    ActualizarJugadorPrueba(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        particulas,
        cantidadParticulas,
        permitirSalto,
        false,
        respawnAutomatico,
        deltaTime
    );

    jugador.velocidadMovimiento =
        velocidadOriginal;

    // ActualizarJugadorPrueba inicia el golpe. Aqui fijamos
    // el cooldown comun para todos los minijuegos estandar.
    if (
        !golpeandoAntes &&
        jugador.golpeando
    )
    {
        jugador.cooldownGolpe =
            COOLDOWN_GOLPE_JUGADOR_ESTANDAR;
    }

    // Un respawn empieza limpio, sin conservar el efecto
    // de ralentizacion de una vida anterior.
    if (
        estabaCayendo &&
        !jugador.cayendo
    )
    {
        jugador.tiempoRalentizado =
            0.0f;
    }
}


inline void ActualizarJugadorMinijuegoEstandar(
    JugadorPrueba& jugador,
    const Participante& participante,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    int cantidadParticulas,
    bool respawnAutomatico,
    float deltaTime
)
{
    if (!participante.activo)
    {
        return;
    }

    InputMinijuegoParticipante entrada{};

    if (participante.conectado)
    {
        entrada =
            LeerInputMinijuegoParticipante(
                participante
            );
    }

    ActualizarJugadorPruebaNormal(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        particulas,
        cantidadParticulas,
        true,
        respawnAutomatico,
        deltaTime
    );
}


//==================================================
// COLISION SOLIDA ENTRE JUGADORES
//==================================================
//
// Esta colision solo impide que dos jugadores se atraviesen.
// No genera impulso, no suma velocidad y no lanza al rival.
// Si uno avanza contra otro quieto, retrocede el que avanzo.
// Si ambos avanzan uno contra otro, se reparte solamente la
// correccion necesaria para que dejen de superponerse.
//==================================================

inline void ResolverColisionesJugadoresSinEmpuje(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    const float MARGEN =
        0.001f;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        JugadorPrueba& a =
            jugadores[i];

        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            a.cayendo
        )
        {
            continue;
        }

        for (int j = i + 1; j < cantidadMaxima; j++)
        {
            JugadorPrueba& b =
                jugadores[j];

            if (
                !participantes[j].activo ||
                !participantes[j].conectado ||
                b.cayendo
            )
            {
                continue;
            }

            BoundingBox cajaA =
                CrearHitboxJugadorPrueba(a);

            BoundingBox cajaB =
                CrearHitboxJugadorPrueba(b);

            bool seSolapan =
                cajaA.min.x < cajaB.max.x &&
                cajaA.max.x > cajaB.min.x &&
                cajaA.min.y < cajaB.max.y &&
                cajaA.max.y > cajaB.min.y &&
                cajaA.min.z < cajaB.max.z &&
                cajaA.max.z > cajaB.min.z;

            if (!seSolapan)
            {
                continue;
            }

            float solapeX =
                a.posicion.x < b.posicion.x
                ? cajaA.max.x - cajaB.min.x
                : cajaB.max.x - cajaA.min.x;

            float solapeZ =
                a.posicion.z < b.posicion.z
                ? cajaA.max.z - cajaB.min.z
                : cajaB.max.z - cajaA.min.z;

            if (solapeX < solapeZ)
            {
                bool aEstaALaIzquierda =
                    a.posicion.x < b.posicion.x;

                bool aVaHaciaB =
                    aEstaALaIzquierda
                    ? a.velocidad.x > 0.0f
                    : a.velocidad.x < 0.0f;

                bool bVaHaciaA =
                    aEstaALaIzquierda
                    ? b.velocidad.x < 0.0f
                    : b.velocidad.x > 0.0f;

                float correccion =
                    solapeX + MARGEN;

                if (aVaHaciaB && !bVaHaciaA)
                {
                    a.posicion.x +=
                        aEstaALaIzquierda
                        ? -correccion
                        : correccion;

                    a.velocidad.x = 0.0f;
                }
                else if (bVaHaciaA && !aVaHaciaB)
                {
                    b.posicion.x +=
                        aEstaALaIzquierda
                        ? correccion
                        : -correccion;

                    b.velocidad.x = 0.0f;
                }
                else
                {
                    float mitad =
                        correccion / 2.0f;

                    a.posicion.x +=
                        aEstaALaIzquierda
                        ? -mitad
                        : mitad;

                    b.posicion.x +=
                        aEstaALaIzquierda
                        ? mitad
                        : -mitad;

                    if (aVaHaciaB)
                    {
                        a.velocidad.x = 0.0f;
                    }

                    if (bVaHaciaA)
                    {
                        b.velocidad.x = 0.0f;
                    }
                }
            }
            else
            {
                bool aEstaArriba =
                    a.posicion.z < b.posicion.z;

                bool aVaHaciaB =
                    aEstaArriba
                    ? a.velocidad.z > 0.0f
                    : a.velocidad.z < 0.0f;

                bool bVaHaciaA =
                    aEstaArriba
                    ? b.velocidad.z < 0.0f
                    : b.velocidad.z > 0.0f;

                float correccion =
                    solapeZ + MARGEN;

                if (aVaHaciaB && !bVaHaciaA)
                {
                    a.posicion.z +=
                        aEstaArriba
                        ? -correccion
                        : correccion;

                    a.velocidad.z = 0.0f;
                }
                else if (bVaHaciaA && !aVaHaciaB)
                {
                    b.posicion.z +=
                        aEstaArriba
                        ? correccion
                        : -correccion;

                    b.velocidad.z = 0.0f;
                }
                else
                {
                    float mitad =
                        correccion / 2.0f;

                    a.posicion.z +=
                        aEstaArriba
                        ? -mitad
                        : mitad;

                    b.posicion.z +=
                        aEstaArriba
                        ? mitad
                        : -mitad;

                    if (aVaHaciaB)
                    {
                        a.velocidad.z = 0.0f;
                    }

                    if (bVaHaciaA)
                    {
                        b.velocidad.z = 0.0f;
                    }
                }
            }
        }
    }
}


//==================================================
// PARTICULAS DEL GOLPE HORIZONTAL
//==================================================

inline void CrearParticulasImpactoGolpe(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    Vector3 posicionImpacto
)
{
    if (
        particulas == nullptr ||
        cantidadMaxima <= 0
    )
    {
        return;
    }

    const int CANTIDAD_CREAR =
        18;

    int creadas =
        0;

    for (
        int i = 0;
        i < cantidadMaxima &&
        creadas < CANTIDAD_CREAR;
        i++
    )
    {
        ParticulaTierra& particula =
            particulas[i];

        if (particula.activa)
        {
            continue;
        }

        float direccionX =
            (float)GetRandomValue(-100, 100) /
            100.0f;

        float direccionZ =
            (float)GetRandomValue(-100, 100) /
            100.0f;

        float longitud =
            std::sqrt(
                direccionX * direccionX +
                direccionZ * direccionZ
            );

        if (longitud < 0.01f)
        {
            direccionX = 1.0f;
            direccionZ = 0.0f;
            longitud = 1.0f;
        }

        direccionX /= longitud;
        direccionZ /= longitud;

        float velocidadHorizontal =
            (float)GetRandomValue(18, 42) /
            10.0f;

        particula.activa =
            true;

        particula.posicion =
        {
            posicionImpacto.x +
                (float)GetRandomValue(-12, 12) /
                100.0f,

            posicionImpacto.y +
                (float)GetRandomValue(-8, 18) /
                100.0f,

            posicionImpacto.z +
                (float)GetRandomValue(-12, 12) /
                100.0f
        };

        particula.velocidad =
        {
            direccionX * velocidadHorizontal,
            (float)GetRandomValue(18, 45) / 10.0f,
            direccionZ * velocidadHorizontal
        };

        particula.vidaMaxima =
            (float)GetRandomValue(18, 34) /
            100.0f;

        particula.vida =
            particula.vidaMaxima;

        particula.tamano =
            (float)GetRandomValue(7, 14) /
            100.0f;

        particula.color =
            GetRandomValue(0, 1) == 0
            ? YELLOW
            : GOLD;

        creadas++;
    }
}


//==================================================
// GOLPE HORIZONTAL
//==================================================

inline void ResolverGolpesJugadoresConEfectos(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        JugadorPrueba& atacante =
            jugadores[i];

        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            atacante.cayendo ||
            atacante.aplastado ||
            !atacante.golpeando ||
            atacante.golpeYaConecto
        )
        {
            continue;
        }

        for (int j = 0; j < cantidadMaxima; j++)
        {
            if (i == j)
            {
                continue;
            }

            JugadorPrueba& objetivo =
                jugadores[j];

            if (
                !participantes[j].activo ||
                !participantes[j].conectado ||
                objetivo.cayendo ||
                objetivo.aplastado ||
                objetivo.tiempoInmunidad > 0.0f
            )
            {
                continue;
            }

            float dx =
                objetivo.posicion.x -
                atacante.posicion.x;

            float dz =
                objetivo.posicion.z -
                atacante.posicion.z;

            float distancia =
                std::sqrt(
                    dx * dx +
                    dz * dz
                );

            if (
                distancia < 0.001f ||
                distancia > 1.55f
            )
            {
                continue;
            }

            if (
                std::fabs(
                    objetivo.posicion.y -
                    atacante.posicion.y
                ) > 1.0f
            )
            {
                continue;
            }

            float normalX =
                dx / distancia;

            float normalZ =
                dz / distancia;

            float frente =
                normalX * atacante.direccionMirada.x +
                normalZ * atacante.direccionMirada.z;

            if (frente < 0.25f)
            {
                continue;
            }

            // El golpe SI puede desplazar: el empuje viene de una
            // accion deliberada del jugador, no del simple contacto.
            objetivo.empuje.x +=
                normalX * FUERZA_GOLPE_JUGADOR_ESTANDAR;

            objetivo.empuje.z +=
                normalZ * FUERZA_GOLPE_JUGADOR_ESTANDAR;

            objetivo.tiempoRalentizado =
                DURACION_RALENTIZACION_GOLPE;

            Vector3 posicionImpacto =
            {
                atacante.posicion.x +
                    normalX * 0.82f,

                (
                    atacante.posicion.y +
                    objetivo.posicion.y
                ) /
                2.0f +
                0.12f,

                atacante.posicion.z +
                    normalZ * 0.82f
            };

            CrearParticulasImpactoGolpe(
                particulas,
                cantidadParticulas,
                posicionImpacto
            );

            atacante.golpeYaConecto =
                true;

            break;
        }
    }
}


//==================================================
// INTERACCIONES ESTANDAR
//==================================================

inline bool ResolverInteraccionesJugadoresMinijuegoEstandar(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    bool huboGolpeSuelo =
        ResolverGolpesSuelo(
            jugadores,
            participantes,
            cantidadMaxima
        );

    ResolverGolpesJugadoresConEfectos(
        jugadores,
        participantes,
        cantidadMaxima,
        particulas,
        cantidadParticulas
    );

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantes,
        cantidadMaxima
    );

    return huboGolpeSuelo;
}
