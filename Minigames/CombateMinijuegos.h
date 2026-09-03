#pragma once

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


//==================================================
// MOVIMIENTO NORMAL CON RALENTIZACION POR GOLPE
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
        jugador.tiempoRalentizado -= deltaTime;

        if (jugador.tiempoRalentizado < 0.0f)
        {
            jugador.tiempoRalentizado = 0.0f;
        }
    }

    float velocidadOriginal =
        jugador.velocidadMovimiento;

    if (jugador.tiempoRalentizado > 0.0f)
    {
        jugador.velocidadMovimiento *= 0.45f;
    }

    bool golpeandoAntes =
        jugador.golpeando;

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

    // La utilidad base ya tiene cooldown. Para los minijuegos
    // de pelea normal lo hacemos mas claro y deliberado.
    if (
        !golpeandoAntes &&
        jugador.golpeando &&
        jugador.cooldownGolpe < 0.80f
    )
    {
        jugador.cooldownGolpe = 0.80f;
    }
}


//==================================================
// COLISION SOLIDA ENTRE JUGADORES, SIN EMPUJAR
//==================================================

inline void ResolverColisionesJugadoresSinEmpuje(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    const float MARGEN = 0.001f;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        JugadorPrueba& a = jugadores[i];

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
            JugadorPrueba& b = jugadores[j];

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
                    a.empuje.x = 0.0f;
                }
                else if (bVaHaciaA && !aVaHaciaB)
                {
                    b.posicion.x +=
                        aEstaALaIzquierda
                        ? correccion
                        : -correccion;

                    b.velocidad.x = 0.0f;
                    b.empuje.x = 0.0f;
                }
                else if (aVaHaciaB && bVaHaciaA)
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

                    a.velocidad.x = 0.0f;
                    b.velocidad.x = 0.0f;
                    a.empuje.x = 0.0f;
                    b.empuje.x = 0.0f;
                }
                // Si ambos ya se estan separando, no los corregimos:
                // el siguiente movimiento elimina el pequeno solape sin
                // empujar al jugador que esta delante.
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
                    a.empuje.z = 0.0f;
                }
                else if (bVaHaciaA && !aVaHaciaB)
                {
                    b.posicion.z +=
                        aEstaArriba
                        ? correccion
                        : -correccion;

                    b.velocidad.z = 0.0f;
                    b.empuje.z = 0.0f;
                }
                else if (aVaHaciaB && bVaHaciaA)
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

                    a.velocidad.z = 0.0f;
                    b.velocidad.z = 0.0f;
                    a.empuje.z = 0.0f;
                    b.empuje.z = 0.0f;
                }
            }
        }
    }
}


//==================================================
// GOLPE HORIZONTAL CON COOLDOWN, SLOW Y PARTICULAS
//==================================================

inline void CrearParticulasImpactoGolpe(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    Vector3 posicionImpacto
)
{
    if (particulas == nullptr || cantidadMaxima <= 0)
    {
        return;
    }

    int creadas = 0;
    const int CANTIDAD_CREAR = 18;

    for (
        int i = 0;
        i < cantidadMaxima && creadas < CANTIDAD_CREAR;
        i++
    )
    {
        ParticulaTierra& particula =
            particulas[i];

        if (particula.activa)
        {
            continue;
        }

        float aleatorioX =
            (float)GetRandomValue(-100, 100) / 100.0f;

        float aleatorioZ =
            (float)GetRandomValue(-100, 100) / 100.0f;

        float longitud =
            std::sqrt(
                aleatorioX * aleatorioX +
                aleatorioZ * aleatorioZ
            );

        if (longitud < 0.01f)
        {
            aleatorioX = 1.0f;
            aleatorioZ = 0.0f;
            longitud = 1.0f;
        }

        aleatorioX /= longitud;
        aleatorioZ /= longitud;

        float velocidadHorizontal =
            (float)GetRandomValue(18, 42) / 10.0f;

        particula.activa = true;

        particula.posicion =
        {
            posicionImpacto.x +
                (float)GetRandomValue(-12, 12) / 100.0f,
            posicionImpacto.y +
                (float)GetRandomValue(-8, 18) / 100.0f,
            posicionImpacto.z +
                (float)GetRandomValue(-12, 12) / 100.0f
        };

        particula.velocidad =
        {
            aleatorioX * velocidadHorizontal,
            (float)GetRandomValue(18, 45) / 10.0f,
            aleatorioZ * velocidadHorizontal
        };

        particula.vidaMaxima =
            (float)GetRandomValue(18, 34) / 100.0f;

        particula.vida =
            particula.vidaMaxima;

        particula.tamano =
            (float)GetRandomValue(7, 14) / 100.0f;

        particula.color =
            GetRandomValue(0, 1) == 0
            ? YELLOW
            : GOLD;

        creadas++;
    }
}


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

            float nx =
                dx / distancia;

            float nz =
                dz / distancia;

            float frente =
                nx * atacante.direccionMirada.x +
                nz * atacante.direccionMirada.z;

            if (frente < 0.25f)
            {
                continue;
            }

            const float FUERZA_GOLPE = 4.2f;
            const float DURACION_RALENTIZACION = 0.70f;

            objetivo.empuje.x +=
                nx * FUERZA_GOLPE;

            objetivo.empuje.z +=
                nz * FUERZA_GOLPE;

            objetivo.tiempoRalentizado =
                DURACION_RALENTIZACION;

            Vector3 posicionImpacto =
            {
                atacante.posicion.x +
                    nx * 0.82f,
                (atacante.posicion.y +
                    objetivo.posicion.y) /
                    2.0f + 0.12f,
                atacante.posicion.z +
                    nz * 0.82f
            };

            CrearParticulasImpactoGolpe(
                particulas,
                cantidadParticulas,
                posicionImpacto
            );

            atacante.golpeYaConecto = true;

            break;
        }
    }
}
