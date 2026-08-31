#include "Gameplay/ZonaPruebas.h"

#include <cmath>


//==================================================
// CREAR BOUNDING BOX DE UN BLOQUE
//==================================================

static BoundingBox CrearHitboxBloque(
    const BloquePrueba& bloque
)
{
    Vector3 mitad =
    {
        bloque.tamano.x / 2.0f,
        bloque.tamano.y / 2.0f,
        bloque.tamano.z / 2.0f
    };


    BoundingBox caja =
    {
        Vector3
        {
            bloque.posicion.x -
                mitad.x,

            bloque.posicion.y -
                mitad.y,

            bloque.posicion.z -
                mitad.z
        },

        Vector3
        {
            bloque.posicion.x +
                mitad.x,

            bloque.posicion.y +
                mitad.y,

            bloque.posicion.z +
                mitad.z
        }
    };


    return caja;
}


//==================================================
// CREAR BOUNDING BOX DEL JUGADOR
//==================================================

static BoundingBox CrearHitboxJugador(
    const JugadorPrueba& jugador
)
{
    Vector3 mitad =
    {
        jugador.tamano.x / 2.0f,
        jugador.tamano.y / 2.0f,
        jugador.tamano.z / 2.0f
    };


    BoundingBox caja =
    {
        Vector3
        {
            jugador.posicion.x -
                mitad.x,

            jugador.posicion.y -
                mitad.y,

            jugador.posicion.z -
                mitad.z
        },

        Vector3
        {
            jugador.posicion.x +
                mitad.x,

            jugador.posicion.y +
                mitad.y,

            jugador.posicion.z +
                mitad.z
        }
    };


    return caja;
}


//==================================================
// SOLAPAMIENTO DE DOS CAJAS
//==================================================

static bool CajasSeSolapan(
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


//==================================================
// SOLAPAMIENTO SOLO XZ
//==================================================

static bool SolapanXZ(
    const BoundingBox& a,
    const BoundingBox& b
)
{
    return
        a.min.x < b.max.x &&
        a.max.x > b.min.x &&

        a.min.z < b.max.z &&
        a.max.z > b.min.z;
}


//==================================================
// AGREGAR BLOQUE
//==================================================

static void AgregarBloque(
    ZonaPruebas& zona,
    Vector3 posicion,
    Vector3 tamano,
    Color color
)
{
    if (
        zona.cantidadBloques >=
        MAX_BLOQUES_PRUEBA
    )
    {
        return;
    }


    BloquePrueba& bloque =
        zona.bloques[
            zona.cantidadBloques
        ];


    bloque.posicion =
        posicion;


    bloque.tamano =
        tamano;


    bloque.color =
        color;


    zona.cantidadBloques++;
}


//==================================================
// COLISION HORIZONTAL EN X
//==================================================

static void ResolverColisionX(
    JugadorPrueba& jugador,
    BloquePrueba bloques[],
    int cantidadBloques,
    float movimientoX
)
{
    if (movimientoX == 0.0f)
    {
        return;
    }


    jugador.posicion.x +=
        movimientoX;


    BoundingBox jugadorBox =
        CrearHitboxJugador(
            jugador
        );


    for (
        int i = 0;
        i < cantidadBloques;
        i++
    )
    {
        BoundingBox bloqueBox =
            CrearHitboxBloque(
                bloques[i]
            );


        if (
            !CajasSeSolapan(
                jugadorBox,
                bloqueBox
            )
        )
        {
            continue;
        }


        //------------------------------
        // MOVIMIENTO HACIA +X
        //------------------------------

        if (movimientoX > 0.0f)
        {
            jugador.posicion.x =
                bloqueBox.min.x -
                jugador.tamano.x /
                2.0f;
        }


        //------------------------------
        // MOVIMIENTO HACIA -X
        //------------------------------

        else
        {
            jugador.posicion.x =
                bloqueBox.max.x +
                jugador.tamano.x /
                2.0f;
        }


        jugadorBox =
            CrearHitboxJugador(
                jugador
            );
    }
}


//==================================================
// COLISION HORIZONTAL EN Z
//==================================================

static void ResolverColisionZ(
    JugadorPrueba& jugador,
    BloquePrueba bloques[],
    int cantidadBloques,
    float movimientoZ
)
{
    if (movimientoZ == 0.0f)
    {
        return;
    }


    jugador.posicion.z +=
        movimientoZ;


    BoundingBox jugadorBox =
        CrearHitboxJugador(
            jugador
        );


    for (
        int i = 0;
        i < cantidadBloques;
        i++
    )
    {
        BoundingBox bloqueBox =
            CrearHitboxBloque(
                bloques[i]
            );


        if (
            !CajasSeSolapan(
                jugadorBox,
                bloqueBox
            )
        )
        {
            continue;
        }


        //------------------------------
        // MOVIMIENTO HACIA +Z
        //------------------------------

        if (movimientoZ > 0.0f)
        {
            jugador.posicion.z =
                bloqueBox.min.z -
                jugador.tamano.z /
                2.0f;
        }


        //------------------------------
        // MOVIMIENTO HACIA -Z
        //------------------------------

        else
        {
            jugador.posicion.z =
                bloqueBox.max.z +
                jugador.tamano.z /
                2.0f;
        }


        jugadorBox =
            CrearHitboxJugador(
                jugador
            );
    }
}


//==================================================
// MOVIMIENTO HORIZONTAL
//==================================================

static void ActualizarMovimientoHorizontal(
    JugadorPrueba& jugador,
    BloquePrueba bloques[],
    int cantidadBloques,
    float deltaTime
)
{
    float direccionX =
        0.0f;


    float direccionZ =
        0.0f;


    //------------------------------
    // INPUT
    //------------------------------

    if (IsKeyDown(KEY_A))
    {
        direccionX -=
            1.0f;
    }


    if (IsKeyDown(KEY_D))
    {
        direccionX +=
            1.0f;
    }


    if (IsKeyDown(KEY_W))
    {
        direccionZ -=
            1.0f;
    }


    if (IsKeyDown(KEY_S))
    {
        direccionZ +=
            1.0f;
    }


    //==================================================
    // NORMALIZAR DIAGONAL
    //==================================================

    float longitud =
        sqrtf(
            direccionX *
                direccionX +

            direccionZ *
                direccionZ
        );


    if (longitud > 0.0f)
    {
        direccionX /=
            longitud;


        direccionZ /=
            longitud;
    }


    //------------------------------
    // VELOCIDAD
    //------------------------------

    jugador.velocidad.x =
        direccionX *
        jugador.velocidadMovimiento;


    jugador.velocidad.z =
        direccionZ *
        jugador.velocidadMovimiento;


    //------------------------------
    // MOVIMIENTO
    //------------------------------

    float movimientoX =
        jugador.velocidad.x *
        deltaTime;


    float movimientoZ =
        jugador.velocidad.z *
        deltaTime;


    /*
        Resolvemos cada eje por separado.

        Esto evita que al chocar contra
        una pared el jugador quede
        completamente clavado.

        Puede seguir "deslizandose"
        por el otro eje.
    */


    ResolverColisionX(
        jugador,
        bloques,
        cantidadBloques,
        movimientoX
    );


    ResolverColisionZ(
        jugador,
        bloques,
        cantidadBloques,
        movimientoZ
    );
}


//==================================================
// GRAVEDAD Y COLISION VERTICAL
//==================================================

static void ActualizarVertical(
    JugadorPrueba& jugador,
    BloquePrueba bloques[],
    int cantidadBloques,
    float deltaTime
)
{
    //------------------------------
    // SALTO
    //------------------------------

    if (
        jugador.enSuelo &&
        IsKeyPressed(KEY_SPACE)
    )
    {
        jugador.velocidad.y =
            jugador.fuerzaSalto;


        jugador.enSuelo =
            false;
    }


    //------------------------------
    // GRAVEDAD
    //------------------------------

    jugador.velocidad.y -=
        jugador.gravedad *
        deltaTime;


    float posicionAnteriorY =
        jugador.posicion.y;


    jugador.posicion.y +=
        jugador.velocidad.y *
        deltaTime;


    jugador.enSuelo =
        false;


    //==================================================
    // POSICIONES ANTERIORES
    //==================================================

    float mitadAlto =
        jugador.tamano.y /
        2.0f;


    float pieAnterior =
        posicionAnteriorY -
        mitadAlto;


    float cabezaAnterior =
        posicionAnteriorY +
        mitadAlto;


    //==================================================
    // HITBOX NUEVA
    //==================================================

    BoundingBox jugadorBox =
        CrearHitboxJugador(
            jugador
        );


    //==================================================
    // COMPROBAR BLOQUES
    //==================================================

    for (
        int i = 0;
        i < cantidadBloques;
        i++
    )
    {
        BoundingBox bloqueBox =
            CrearHitboxBloque(
                bloques[i]
            );


        //------------------------------
        // DEBE ESTAR SOBRE EL BLOQUE
        // EN X/Z
        //------------------------------

        if (
            !SolapanXZ(
                jugadorBox,
                bloqueBox
            )
        )
        {
            continue;
        }


        float pieNuevo =
            jugador.posicion.y -
            mitadAlto;


        float cabezaNueva =
            jugador.posicion.y +
            mitadAlto;


        //==================================================
        // CAYENDO SOBRE EL BLOQUE
        //==================================================

        if (
            jugador.velocidad.y <=
                0.0f &&

            pieAnterior >=
                bloqueBox.max.y -
                0.01f &&

            pieNuevo <=
                bloqueBox.max.y
        )
        {
            jugador.posicion.y =
                bloqueBox.max.y +
                mitadAlto;


            jugador.velocidad.y =
                0.0f;


            jugador.enSuelo =
                true;


            jugadorBox =
                CrearHitboxJugador(
                    jugador
                );


            continue;
        }


        //==================================================
        // GOLPEAR EL TECHO DEL BLOQUE
        //==================================================

        if (
            jugador.velocidad.y >
                0.0f &&

            cabezaAnterior <=
                bloqueBox.min.y +
                0.01f &&

            cabezaNueva >=
                bloqueBox.min.y
        )
        {
            jugador.posicion.y =
                bloqueBox.min.y -
                mitadAlto;


            jugador.velocidad.y =
                0.0f;


            jugadorBox =
                CrearHitboxJugador(
                    jugador
                );
        }
    }
}


//==================================================
// DIBUJAR JUGADOR PLACEHOLDER
//==================================================

static void DibujarJugador(
    const JugadorPrueba& jugador
)
{
    /*
        MAS ADELANTE ESTA ES UNA DE
        LAS FUNCIONES QUE CAMBIAREMOS
        POR DrawModel().

        La fisica NO tendra que cambiar.
    */


    DrawCube(
        jugador.posicion,

        jugador.tamano.x,
        jugador.tamano.y,
        jugador.tamano.z,

        RED
    );


    DrawCubeWires(
        jugador.posicion,

        jugador.tamano.x,
        jugador.tamano.y,
        jugador.tamano.z,

        MAROON
    );
}


//==================================================
// INICIALIZAR
//==================================================

void ZonaPruebas::Inicializar()
{
    //==================================================
    // RESET
    //==================================================

    cantidadBloques =
        0;


    volverAlMenu =
        false;


    mostrarDebug =
        false;


    //==================================================
    // JUGADOR
    //==================================================

    jugador.posicionSpawn =
    {
        0.0f,
        1.0f,
        3.5f
    };


    jugador.tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };


    jugador.velocidadMovimiento =
        5.0f;


    jugador.fuerzaSalto =
        7.2f;


    jugador.gravedad =
        18.0f;


    jugador.duracionRespawn =
        1.2f;


    ReiniciarJugador();


    //==================================================
    // SUELO PRINCIPAL
    //==================================================

    /*
        Centro Y = -0.5
        Alto = 1

        Entonces la superficie superior
        queda exactamente en Y = 0.
    */


    AgregarBloque(
        *this,

        Vector3
        {
            0.0f,
            -0.5f,
            0.0f
        },

        Vector3
        {
            14.0f,
            1.0f,
            14.0f
        },

        Color
        {
            105,
            105,
            115,
            255
        }
    );


    //==================================================
    // BLOQUE BAJO
    //==================================================

    /*
        Superficie superior = Y 1.

        Se puede saltar directamente
        desde el suelo.
    */


    AgregarBloque(
        *this,

        Vector3
        {
            -3.0f,
            0.5f,
            -2.0f
        },

        Vector3
        {
            2.5f,
            1.0f,
            2.5f
        },

        ORANGE
    );


    //==================================================
    // BLOQUE ALTO
    //==================================================

    /*
        Superficie superior = Y 2.

        La idea es poder llegar desde
        el bloque anterior.
    */


    AgregarBloque(
        *this,

        Vector3
        {
            2.0f,
            1.0f,
            1.0f
        },

        Vector3
        {
            3.0f,
            2.0f,
            3.0f
        },

        BLUE
    );


    //==================================================
    // CAMARA FIJA
    //==================================================

    camara.position =
    {
        11.0f,
        11.0f,
        11.0f
    };


    camara.target =
    {
        0.0f,
        0.5f,
        0.0f
    };


    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };


    camara.fovy =
        45.0f;


    camara.projection =
        CAMERA_PERSPECTIVE;
}


//==================================================
// REINICIAR JUGADOR
//==================================================

void ZonaPruebas::ReiniciarJugador()
{
    jugador.posicion =
        jugador.posicionSpawn;


    jugador.velocidad =
    {
        0.0f,
        0.0f,
        0.0f
    };


    jugador.enSuelo =
        false;


    jugador.cayendo =
        false;


    jugador.tiempoRespawn =
        0.0f;
}


//==================================================
// ACTUALIZAR
//==================================================

void ZonaPruebas::Actualizar(
    float deltaTime
)
{
    //==================================================
    // VOLVER AL MENU
    //==================================================

    if (IsKeyPressed(KEY_ESCAPE))
    {
        volverAlMenu =
            true;


        return;
    }


    //==================================================
    // DEBUG
    //==================================================

    if (IsKeyPressed(KEY_F3))
    {
        mostrarDebug =
            !mostrarDebug;
    }


    //==================================================
    // RESPAWN MANUAL
    //==================================================

    if (IsKeyPressed(KEY_R))
    {
        ReiniciarJugador();


        return;
    }


    //==================================================
    // JUGADOR CAYENDO / MUERTO
    //==================================================

    if (jugador.cayendo)
    {
        jugador.tiempoRespawn +=
            deltaTime;


        if (
            jugador.tiempoRespawn >=
            jugador.duracionRespawn
        )
        {
            ReiniciarJugador();
        }


        return;
    }


    //==================================================
    // MOVIMIENTO
    //==================================================

    ActualizarMovimientoHorizontal(
        jugador,
        bloques,
        cantidadBloques,
        deltaTime
    );


    //==================================================
    // GRAVEDAD
    //==================================================

    ActualizarVertical(
        jugador,
        bloques,
        cantidadBloques,
        deltaTime
    );


    //==================================================
    // CAIDA AL VACIO
    //==================================================

    if (
        jugador.posicion.y <
        -8.0f
    )
    {
        jugador.cayendo =
            true;


        jugador.tiempoRespawn =
            0.0f;
    }
}


//==================================================
// DIBUJAR
//==================================================

void ZonaPruebas::Dibujar() const
{
    //==================================================
    // FONDO
    //==================================================

    ClearBackground(
        Color
        {
            125,
            190,
            220,
            255
        }
    );


    //==================================================
    // MUNDO 3D
    //==================================================

    BeginMode3D(
        camara
    );


    //------------------------------
    // BLOQUES
    //------------------------------

    for (
        int i = 0;
        i < cantidadBloques;
        i++
    )
    {
        DrawCube(
            bloques[i].posicion,

            bloques[i].tamano.x,
            bloques[i].tamano.y,
            bloques[i].tamano.z,

            bloques[i].color
        );


        DrawCubeWires(
            bloques[i].posicion,

            bloques[i].tamano.x,
            bloques[i].tamano.y,
            bloques[i].tamano.z,

            BLACK
        );


        //------------------------------
        // HITBOX DEBUG
        //------------------------------

        if (mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxBloque(
                    bloques[i]
                ),

                YELLOW
            );
        }
    }


    //------------------------------
    // JUGADOR
    //------------------------------

    if (!jugador.cayendo)
    {
        DibujarJugador(
            jugador
        );
    }


    //------------------------------
    // HITBOX JUGADOR
    //------------------------------

    if (
        mostrarDebug &&
        !jugador.cayendo
    )
    {
        DrawBoundingBox(
            CrearHitboxJugador(
                jugador
            ),

            LIME
        );
    }


    //------------------------------
    // GRID DE REFERENCIA
    //------------------------------

    DrawGrid(
        30,
        1.0f
    );


    EndMode3D();


    //==================================================
    // INTERFAZ
    //==================================================

    DrawText(
        "ZONA DE PRUEBAS",
        25,
        25,
        30,
        BLACK
    );


    DrawText(
        "WASD - Mover",
        25,
        70,
        20,
        BLACK
    );


    DrawText(
        "ESPACIO - Saltar",
        25,
        98,
        20,
        BLACK
    );


    DrawText(
        "R - Respawn",
        25,
        126,
        20,
        BLACK
    );


    DrawText(
        "F3 - Debug / Hitboxes",
        25,
        154,
        20,
        BLACK
    );


    DrawText(
        "ESC - Volver al menu",
        25,
        182,
        20,
        BLACK
    );


    //==================================================
    // MENSAJE DE CAIDA
    //==================================================

    if (jugador.cayendo)
    {
        const char* texto =
            "TE CAISTE";


        int tamano =
            48;


        int ancho =
            MeasureText(
                texto,
                tamano
            );


        DrawText(
            texto,

            GetScreenWidth() /
                2 -
                ancho /
                2,

            GetScreenHeight() /
                2 -
                30,

            tamano,

            RED
        );


        const char* textoRespawn =
            "Reapareciendo...";


        int anchoRespawn =
            MeasureText(
                textoRespawn,
                24
            );


        DrawText(
            textoRespawn,

            GetScreenWidth() /
                2 -
                anchoRespawn /
                2,

            GetScreenHeight() /
                2 +
                35,

            24,

            BLACK
        );
    }


    //==================================================
    // DEBUG
    //==================================================

    if (mostrarDebug)
    {
        int x =
            GetScreenWidth() -
            300;


        int y =
            30;


        DrawRectangle(
            x - 15,
            y - 15,
            280,
            190,

            Fade(
                BLACK,
                0.65f
            )
        );


        DrawText(
            "DEBUG P1",
            x,
            y,
            22,
            LIME
        );


        DrawText(
            TextFormat(
                "X: %.2f",
                jugador.posicion.x
            ),
            x,
            y + 35,
            20,
            WHITE
        );


        DrawText(
            TextFormat(
                "Y: %.2f",
                jugador.posicion.y
            ),
            x,
            y + 60,
            20,
            WHITE
        );


        DrawText(
            TextFormat(
                "Z: %.2f",
                jugador.posicion.z
            ),
            x,
            y + 85,
            20,
            WHITE
        );


        DrawText(
            TextFormat(
                "Vel Y: %.2f",
                jugador.velocidad.y
            ),
            x,
            y + 110,
            20,
            WHITE
        );


        DrawText(
            jugador.enSuelo
            ? "En suelo: SI"
            : "En suelo: NO",

            x,
            y + 135,
            20,

            jugador.enSuelo
            ? LIME
            : ORANGE
        );
    }
}