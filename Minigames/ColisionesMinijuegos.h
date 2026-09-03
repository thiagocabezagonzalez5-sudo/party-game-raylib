#pragma once

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


//==================================================
// COLISION ROBUSTA CONTRA BLOQUES
//==================================================
//
// Utilidad compartida para los minijuegos con movimiento normal.
// Complementa la resolucion por ejes de UtilidadesMinijuegos:
//
// - evita atravesar un bloque aunque un frame avance demasiado;
// - detecta cruces entre la posicion anterior y la nueva;
// - si el jugador ya quedo superpuesto, lo saca por la cara mas
//   cercana en vez de mandarlo al lado opuesto del bloque;
// - conserva el comportamiento de caminar sobre la parte superior.
//==================================================


inline bool RangosSeSolapanColision(
    float minimoA,
    float maximoA,
    float minimoB,
    float maximoB
)
{
    return
        minimoA < maximoB &&
        maximoA > minimoB;
}


inline BoundingBox CrearHitboxJugadorEnPosicion(
    const JugadorPrueba& jugador,
    Vector3 posicion
)
{
    Vector3 mitad =
    {
        jugador.tamano.x / 2.0f,
        jugador.tamano.y / 2.0f,
        jugador.tamano.z / 2.0f
    };

    return BoundingBox
    {
        Vector3
        {
            posicion.x - mitad.x,
            posicion.y - mitad.y,
            posicion.z - mitad.z
        },
        Vector3
        {
            posicion.x + mitad.x,
            posicion.y + mitad.y,
            posicion.z + mitad.z
        }
    };
}


inline bool CajasSeSolapanColision(
    const BoundingBox& a,
    const BoundingBox& b
)
{
    return
        RangosSeSolapanColision(
            a.min.x,
            a.max.x,
            b.min.x,
            b.max.x
        ) &&
        RangosSeSolapanColision(
            a.min.y,
            a.max.y,
            b.min.y,
            b.max.y
        ) &&
        RangosSeSolapanColision(
            a.min.z,
            a.max.z,
            b.min.z,
            b.max.z
        );
}


inline void CorregirMovimientoJugadorContraBloques(
    JugadorPrueba& jugador,
    Vector3 posicionAnterior,
    BloquePrueba bloques[],
    int cantidadBloques
)
{
    if (
        bloques == nullptr ||
        cantidadBloques <= 0 ||
        jugador.cayendo
    )
    {
        return;
    }

    const float MARGEN = 0.001f;

    const float mitadX =
        jugador.tamano.x / 2.0f;

    const float mitadY =
        jugador.tamano.y / 2.0f;

    const float mitadZ =
        jugador.tamano.z / 2.0f;

    Vector3 destino =
        jugador.posicion;

    Vector3 inicio =
        posicionAnterior;


    //==================================================
    // DESATASCAR LA POSICION ANTERIOR
    //==================================================
    // Si por un error de un frame anterior el jugador ya estaba
    // dentro de un bloque, elegimos la penetracion mas pequena.
    // Esto evita el salto brusco hasta la cara opuesta.

    for (int i = 0; i < cantidadBloques; i++)
    {
        if (!bloques[i].activaColision)
        {
            continue;
        }

        BoundingBox bloqueBox =
            CrearHitboxBloquePrueba(
                bloques[i]
            );

        BoundingBox jugadorBox =
            CrearHitboxJugadorEnPosicion(
                jugador,
                inicio
            );

        if (!CajasSeSolapanColision(jugadorBox, bloqueBox))
        {
            continue;
        }

        float penetracionX =
            fminf(
                jugadorBox.max.x - bloqueBox.min.x,
                bloqueBox.max.x - jugadorBox.min.x
            );

        float penetracionY =
            fminf(
                jugadorBox.max.y - bloqueBox.min.y,
                bloqueBox.max.y - jugadorBox.min.y
            );

        float penetracionZ =
            fminf(
                jugadorBox.max.z - bloqueBox.min.z,
                bloqueBox.max.z - jugadorBox.min.z
            );

        if (
            penetracionX <= penetracionY &&
            penetracionX <= penetracionZ
        )
        {
            float izquierda =
                bloqueBox.min.x -
                mitadX -
                MARGEN;

            float derecha =
                bloqueBox.max.x +
                mitadX +
                MARGEN;

            inicio.x =
                fabsf(inicio.x - izquierda) <=
                fabsf(inicio.x - derecha)
                ? izquierda
                : derecha;

            destino.x = inicio.x;
            jugador.velocidad.x = 0.0f;
            jugador.empuje.x = 0.0f;
        }
        else if (penetracionZ <= penetracionY)
        {
            float frente =
                bloqueBox.min.z -
                mitadZ -
                MARGEN;

            float atras =
                bloqueBox.max.z +
                mitadZ +
                MARGEN;

            inicio.z =
                fabsf(inicio.z - frente) <=
                fabsf(inicio.z - atras)
                ? frente
                : atras;

            destino.z = inicio.z;
            jugador.velocidad.z = 0.0f;
            jugador.empuje.z = 0.0f;
        }
        // Si Y es la penetracion menor, no lo movemos lateralmente.
        // La correccion vertical de mas abajo decide techo o suelo.
    }


    //==================================================
    // BARRIDO EN X
    //==================================================

    float movimientoX =
        destino.x - inicio.x;

    float xSeguro =
        destino.x;

    if (fabsf(movimientoX) > 0.000001f)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            if (!bloques[i].activaColision)
            {
                continue;
            }

            BoundingBox bloqueBox =
                CrearHitboxBloquePrueba(
                    bloques[i]
                );

            bool solapaY =
                RangosSeSolapanColision(
                    inicio.y - mitadY,
                    inicio.y + mitadY,
                    bloqueBox.min.y,
                    bloqueBox.max.y
                );

            bool solapaZ =
                RangosSeSolapanColision(
                    inicio.z - mitadZ,
                    inicio.z + mitadZ,
                    bloqueBox.min.z,
                    bloqueBox.max.z
                );

            if (!solapaY || !solapaZ)
            {
                continue;
            }

            if (movimientoX > 0.0f)
            {
                float limite =
                    bloqueBox.min.x -
                    mitadX -
                    MARGEN;

                if (
                    inicio.x <= limite &&
                    xSeguro > limite
                )
                {
                    xSeguro =
                        fminf(
                            xSeguro,
                            limite
                        );

                    jugador.velocidad.x = 0.0f;
                    jugador.empuje.x = 0.0f;
                }
            }
            else
            {
                float limite =
                    bloqueBox.max.x +
                    mitadX +
                    MARGEN;

                if (
                    inicio.x >= limite &&
                    xSeguro < limite
                )
                {
                    xSeguro =
                        fmaxf(
                            xSeguro,
                            limite
                        );

                    jugador.velocidad.x = 0.0f;
                    jugador.empuje.x = 0.0f;
                }
            }
        }
    }


    //==================================================
    // BARRIDO EN Z
    //==================================================

    float movimientoZ =
        destino.z - inicio.z;

    float zSeguro =
        destino.z;

    if (fabsf(movimientoZ) > 0.000001f)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            if (!bloques[i].activaColision)
            {
                continue;
            }

            BoundingBox bloqueBox =
                CrearHitboxBloquePrueba(
                    bloques[i]
                );

            bool solapaY =
                RangosSeSolapanColision(
                    inicio.y - mitadY,
                    inicio.y + mitadY,
                    bloqueBox.min.y,
                    bloqueBox.max.y
                );

            bool solapaX =
                RangosSeSolapanColision(
                    xSeguro - mitadX,
                    xSeguro + mitadX,
                    bloqueBox.min.x,
                    bloqueBox.max.x
                );

            if (!solapaY || !solapaX)
            {
                continue;
            }

            if (movimientoZ > 0.0f)
            {
                float limite =
                    bloqueBox.min.z -
                    mitadZ -
                    MARGEN;

                if (
                    inicio.z <= limite &&
                    zSeguro > limite
                )
                {
                    zSeguro =
                        fminf(
                            zSeguro,
                            limite
                        );

                    jugador.velocidad.z = 0.0f;
                    jugador.empuje.z = 0.0f;
                }
            }
            else
            {
                float limite =
                    bloqueBox.max.z +
                    mitadZ +
                    MARGEN;

                if (
                    inicio.z >= limite &&
                    zSeguro < limite
                )
                {
                    zSeguro =
                        fmaxf(
                            zSeguro,
                            limite
                        );

                    jugador.velocidad.z = 0.0f;
                    jugador.empuje.z = 0.0f;
                }
            }
        }
    }

    jugador.posicion.x = xSeguro;
    jugador.posicion.z = zSeguro;


    //==================================================
    // CORRECCION FINAL DE SUPERPOSICION
    //==================================================
    // La vertical normal ya resuelve cruces de suelo/techo. Este
    // bloque cubre el caso restante: terminar un frame dentro de un
    // bloque por redondeo, movimiento de la plataforma o un frame largo.

    for (int i = 0; i < cantidadBloques; i++)
    {
        if (!bloques[i].activaColision)
        {
            continue;
        }

        BoundingBox bloqueBox =
            CrearHitboxBloquePrueba(
                bloques[i]
            );

        BoundingBox jugadorBox =
            CrearHitboxJugadorPrueba(
                jugador
            );

        if (!CajasSeSolapanColision(jugadorBox, bloqueBox))
        {
            continue;
        }

        float penetracionX =
            fminf(
                jugadorBox.max.x - bloqueBox.min.x,
                bloqueBox.max.x - jugadorBox.min.x
            );

        float penetracionY =
            fminf(
                jugadorBox.max.y - bloqueBox.min.y,
                bloqueBox.max.y - jugadorBox.min.y
            );

        float penetracionZ =
            fminf(
                jugadorBox.max.z - bloqueBox.min.z,
                bloqueBox.max.z - jugadorBox.min.z
            );

        if (
            penetracionY <= penetracionX &&
            penetracionY <= penetracionZ
        )
        {
            if (posicionAnterior.y >= bloques[i].posicion.y)
            {
                jugador.posicion.y =
                    bloqueBox.max.y +
                    mitadY +
                    MARGEN;

                jugador.velocidad.y = 0.0f;
                jugador.enSuelo = true;
                jugador.golpeSueloActivo = false;
            }
            else
            {
                jugador.posicion.y =
                    bloqueBox.min.y -
                    mitadY -
                    MARGEN;

                if (jugador.velocidad.y > 0.0f)
                {
                    jugador.velocidad.y = 0.0f;
                }
            }
        }
        else if (penetracionX <= penetracionZ)
        {
            if (jugador.posicion.x <= bloques[i].posicion.x)
            {
                jugador.posicion.x =
                    bloqueBox.min.x -
                    mitadX -
                    MARGEN;
            }
            else
            {
                jugador.posicion.x =
                    bloqueBox.max.x +
                    mitadX +
                    MARGEN;
            }

            jugador.velocidad.x = 0.0f;
            jugador.empuje.x = 0.0f;
        }
        else
        {
            if (jugador.posicion.z <= bloques[i].posicion.z)
            {
                jugador.posicion.z =
                    bloqueBox.min.z -
                    mitadZ -
                    MARGEN;
            }
            else
            {
                jugador.posicion.z =
                    bloqueBox.max.z +
                    mitadZ +
                    MARGEN;
            }

            jugador.velocidad.z = 0.0f;
            jugador.empuje.z = 0.0f;
        }
    }
}
