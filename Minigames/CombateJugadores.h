#pragma once

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


//==================================================
// CONTEXTO DE PARTICULAS DE COMBATE
//==================================================

inline ParticulaTierra*& ObtenerParticulasCombate()
{
    static ParticulaTierra* particulas = nullptr;
    return particulas;
}


inline int& ObtenerCantidadParticulasCombate()
{
    static int cantidad = 0;
    return cantidad;
}


inline void RegistrarParticulasCombate(
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    ObtenerParticulasCombate() = particulas;
    ObtenerCantidadParticulasCombate() = cantidadParticulas;
}


//==================================================
// ACTUALIZACION CON RALENTIZACION
//==================================================

inline void ActualizarJugadorPruebaConEfectos(
    JugadorPrueba& jugador,
    const InputMinijuegoParticipante& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    int cantidadParticulas,
    bool permitirSalto,
    bool usarEmpuje,
    bool respawnAutomatico,
    float deltaTime
)
{
    RegistrarParticulasCombate(
        particulas,
        cantidadParticulas
    );

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

    if (
        jugador.tiempoRalentizado > 0.0f &&
        !usarEmpuje
    )
    {
        jugador.velocidadMovimiento *= 0.45f;
    }

    bool iniciaGolpe =
        entrada.golpear &&
        jugador.cooldownGolpe <= 0.0f &&
        !jugador.golpeSueloActivo;

    ActualizarJugadorPrueba(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        particulas,
        cantidadParticulas,
        permitirSalto,
        usarEmpuje,
        respawnAutomatico,
        deltaTime
    );

    jugador.velocidadMovimiento =
        velocidadOriginal;

    // El sistema base ya tenia cooldown de golpe. Lo dejamos
    // un poco mas marcado para que no se pueda spamear.
    if (iniciaGolpe && jugador.golpeando)
    {
        jugador.cooldownGolpe = 0.65f;
    }
}


//==================================================
// COLISION SOLIDA SIN EMPUJAR AL RIVAL
//==================================================

inline bool CajasJugadoresSeSolapan(
    const BoundingBox& a,
    const BoundingBox& b
)
{
    return
        a.min.x < b.max.x &&
        a.max.x > b.min.x &&
        a.min.y < b.max.y &&
        a.max.y > b.min.y &&
        a.min.z < b.max.z &&
        a.max.z > b.min.z;
}


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

            if (!CajasJugadoresSeSolapan(cajaA, cajaB))
            {
                continue;
            }

            float velocidadA =
                a.velocidad.x * a.velocidad.x +
                a.velocidad.z * a.velocidad.z;

            float velocidadB =
                b.velocidad.x * b.velocidad.x +
                b.velocidad.z * b.velocidad.z;

            // Solo corregimos al jugador que esta intentando
            // entrar en el otro. El rival no se desplaza.
            JugadorPrueba* movil =
                velocidadA > velocidadB
                ? &a
                : &b;

            JugadorPrueba* fijo =
                movil == &a
                ? &b
                : &a;

            BoundingBox cajaMovil =
                CrearHitboxJugadorPrueba(*movil);

            BoundingBox cajaFijo =
                CrearHitboxJugadorPrueba(*fijo);

            float solapeX =
                movil->posicion.x < fijo->posicion.x
                ? cajaMovil.max.x - cajaFijo.min.x
                : cajaFijo.max.x - cajaMovil.min.x;

            float solapeZ =
                movil->posicion.z < fijo->posicion.z
                ? cajaMovil.max.z - cajaFijo.min.z
                : cajaFijo.max.z - cajaMovil.min.z;

            if (solapeX < solapeZ)
            {
                float correccion =
                    solapeX + MARGEN;

                if (movil->posicion.x < fijo->posicion.x)
                {
                    movil->posicion.x -= correccion;
                }
                else
                {
                    movil->posicion.x += correccion;
                }

                movil->velocidad.x = 0.0f;
                movil->empuje.x = 0.0f;
            }
            else
            {
                float correccion =
                    solapeZ + MARGEN;

                if (movil->posicion.z < fijo->posicion.z)
                {
                    movil->posicion.z -= correccion;
                }
                else
                {
                    movil->posicion.z += correccion;
                }

                movil->velocidad.z = 0.0f;
                movil->empuje.z = 0.0f;
            }
        }
    }
}


//==================================================
// PARTICULAS DE IMPACTO
//==================================================

inline float AleatorioCombate(
    float minimo,
    float maximo
)
{
    float t =
        (float)GetRandomValue(0, 1000) /
        1000.0f;

    return minimo +
        (maximo - minimo) * t;
}


inline void CrearParticulasGolpeJugador(
    Vector3 posicionImpacto
)
{
    ParticulaTierra* particulas =
        ObtenerParticulasCombate();

    int cantidadMaxima =
        ObtenerCantidadParticulasCombate();

    if (
        particulas == nullptr ||
        cantidadMaxima <= 0
    )
    {
        return;
    }

    int creadas = 0;
    const int CANTIDAD_CREAR = 18;

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

        float angulo =
            AleatorioCombate(
                0.0f,
                6.28318f
            );

        float velocidad =
            AleatorioCombate(
                1.8f,
                4.6f
            );

        particula.activa = true;
        particula.posicion = posicionImpacto;

        particula.posicion.x +=
            AleatorioCombate(-0.12f, 0.12f);

        particula.posicion.y +=
            AleatorioCombate(-0.08f, 0.22f);

        particula.posicion.z +=
            AleatorioCombate(-0.12f, 0.12f);

        particula.velocidad =
        {
            std::cos(angulo) * velocidad,
            AleatorioCombate(1.1f, 3.4f),
            std::sin(angulo) * velocidad
        };

        particula.vidaMaxima =
            AleatorioCombate(0.22f, 0.48f);

        particula.vida =
            particula.vidaMaxima;

        particula.tamano =
            AleatorioCombate(0.07f, 0.16f);

        particula.color =
            creadas % 3 == 0
            ? Color{ 255, 230, 70, 255 }
            : Color{ 255, 190, 35, 255 };

        creadas++;
    }
}


//==================================================
// GOLPE HORIZONTAL CON EFECTOS
//==================================================

inline void ResolverGolpesJugadoresConEfectos(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
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

            float nx = dx / distancia;
            float nz = dz / distancia;

            float frente =
                nx * atacante.direccionMirada.x +
                nz * atacante.direccionMirada.z;

            if (frente < 0.25f)
            {
                continue;
            }

            // El golpe ya no lanza al rival. En cambio lo
            // ralentiza brevemente, haciendo que pegar tenga
            // utilidad sin reemplazar el empuje de Pelotas.
            objetivo.tiempoRalentizado = 0.80f;

            objetivo.velocidad.x *= 0.35f;
            objetivo.velocidad.z *= 0.35f;

            Vector3 impacto =
            {
                atacante.posicion.x + dx * 0.55f,
                (atacante.posicion.y + objetivo.posicion.y) / 2.0f + 0.18f,
                atacante.posicion.z + dz * 0.55f
            };

            CrearParticulasGolpeJugador(
                impacto
            );

            atacante.golpeYaConecto = true;
            break;
        }
    }
}
