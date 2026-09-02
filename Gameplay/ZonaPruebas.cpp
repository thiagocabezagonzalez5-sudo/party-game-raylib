#include "Gameplay/ZonaPruebas.h"

#include <cmath>


struct EntradaJugadorPrueba
{
    bool izquierda = false;
    bool derecha = false;
    bool adelante = false;
    bool atras = false;
    bool saltar = false;
};


//==================================================
// HITBOX DE BLOQUE
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

    return BoundingBox
    {
        Vector3
        {
            bloque.posicion.x - mitad.x,
            bloque.posicion.y - mitad.y,
            bloque.posicion.z - mitad.z
        },

        Vector3
        {
            bloque.posicion.x + mitad.x,
            bloque.posicion.y + mitad.y,
            bloque.posicion.z + mitad.z
        }
    };
}


//==================================================
// HITBOX DE JUGADOR
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

    return BoundingBox
    {
        Vector3
        {
            jugador.posicion.x - mitad.x,
            jugador.posicion.y - mitad.y,
            jugador.posicion.z - mitad.z
        },

        Vector3
        {
            jugador.posicion.x + mitad.x,
            jugador.posicion.y + mitad.y,
            jugador.posicion.z + mitad.z
        }
    };
}


//==================================================
// COLISIONES AUXILIARES
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

    bloque.posicion = posicion;
    bloque.tamano = tamano;
    bloque.color = color;

    zona.cantidadBloques++;
}


//==================================================
// LEER INPUT DE UN JUGADOR
//==================================================

static EntradaJugadorPrueba LeerEntradaJugador(
    int indiceJugador,
    ModoTeclado modoTeclado
)
{
    EntradaJugadorPrueba entrada;

    //==================================================
    // UN JUGADOR USA EL TECLADO COMPLETO
    //==================================================

    if (modoTeclado == TECLADO_COMPLETO)
    {
        if (indiceJugador != 0)
        {
            return entrada;
        }

        entrada.izquierda =
            IsKeyDown(KEY_A) ||
            IsKeyDown(KEY_LEFT);

        entrada.derecha =
            IsKeyDown(KEY_D) ||
            IsKeyDown(KEY_RIGHT);

        entrada.adelante =
            IsKeyDown(KEY_W) ||
            IsKeyDown(KEY_UP);

        entrada.atras =
            IsKeyDown(KEY_S) ||
            IsKeyDown(KEY_DOWN);

        entrada.saltar =
            IsKeyPressed(KEY_SPACE);

        return entrada;
    }

    //==================================================
    // TECLADO DIVIDIDO
    //==================================================

    if (indiceJugador == 0)
    {
        entrada.izquierda = IsKeyDown(KEY_A);
        entrada.derecha = IsKeyDown(KEY_D);
        entrada.adelante = IsKeyDown(KEY_W);
        entrada.atras = IsKeyDown(KEY_S);
        entrada.saltar = IsKeyPressed(KEY_SPACE);
    }
    else if (indiceJugador == 1)
    {
        entrada.izquierda = IsKeyDown(KEY_LEFT);
        entrada.derecha = IsKeyDown(KEY_RIGHT);
        entrada.adelante = IsKeyDown(KEY_UP);
        entrada.atras = IsKeyDown(KEY_DOWN);
        entrada.saltar = IsKeyPressed(KEY_ENTER);
    }

    return entrada;
}


//==================================================
// COLISION HORIZONTAL X
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

    jugador.posicion.x += movimientoX;

    BoundingBox jugadorBox =
        CrearHitboxJugador(jugador);

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

        if (movimientoX > 0.0f)
        {
            jugador.posicion.x =
                bloqueBox.min.x -
                jugador.tamano.x / 2.0f;
        }
        else
        {
            jugador.posicion.x =
                bloqueBox.max.x +
                jugador.tamano.x / 2.0f;
        }

        jugadorBox =
            CrearHitboxJugador(jugador);
    }
}


//==================================================
// COLISION HORIZONTAL Z
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

    jugador.posicion.z += movimientoZ;

    BoundingBox jugadorBox =
        CrearHitboxJugador(jugador);

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

        if (movimientoZ > 0.0f)
        {
            jugador.posicion.z =
                bloqueBox.min.z -
                jugador.tamano.z / 2.0f;
        }
        else
        {
            jugador.posicion.z =
                bloqueBox.max.z +
                jugador.tamano.z / 2.0f;
        }

        jugadorBox =
            CrearHitboxJugador(jugador);
    }
}


//==================================================
// MOVIMIENTO HORIZONTAL
//==================================================

static void ActualizarMovimientoHorizontal(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    float deltaTime
)
{
    float direccionX = 0.0f;
    float direccionZ = 0.0f;

    if (entrada.izquierda)
    {
        direccionX -= 1.0f;
    }

    if (entrada.derecha)
    {
        direccionX += 1.0f;
    }

    /*
        La camara esta detras del jugador,
        sobre el lado +Z del escenario.

        W / flecha arriba mueve hacia -Z,
        o sea alejandose de la camara.
    */

    if (entrada.adelante)
    {
        direccionZ -= 1.0f;
    }

    if (entrada.atras)
    {
        direccionZ += 1.0f;
    }

    float longitud =
        sqrtf(
            direccionX * direccionX +
            direccionZ * direccionZ
        );

    if (longitud > 0.0f)
    {
        direccionX /= longitud;
        direccionZ /= longitud;
    }

    jugador.velocidad.x =
        direccionX *
        jugador.velocidadMovimiento;

    jugador.velocidad.z =
        direccionZ *
        jugador.velocidadMovimiento;

    ResolverColisionX(
        jugador,
        bloques,
        cantidadBloques,
        jugador.velocidad.x * deltaTime
    );

    ResolverColisionZ(
        jugador,
        bloques,
        cantidadBloques,
        jugador.velocidad.z * deltaTime
    );
}


//==================================================
// MOVIMIENTO VERTICAL
//==================================================

static void ActualizarVertical(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    float deltaTime
)
{
    if (
        jugador.enSuelo &&
        entrada.saltar
    )
    {
        jugador.velocidad.y =
            jugador.fuerzaSalto;

        jugador.enSuelo = false;
    }

    jugador.velocidad.y -=
        jugador.gravedad * deltaTime;

    float posicionAnteriorY =
        jugador.posicion.y;

    jugador.posicion.y +=
        jugador.velocidad.y * deltaTime;

    jugador.enSuelo = false;

    float mitadAlto =
        jugador.tamano.y / 2.0f;

    float pieAnterior =
        posicionAnteriorY - mitadAlto;

    float cabezaAnterior =
        posicionAnteriorY + mitadAlto;

    BoundingBox jugadorBox =
        CrearHitboxJugador(jugador);

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
            !SolapanXZ(
                jugadorBox,
                bloqueBox
            )
        )
        {
            continue;
        }

        float pieNuevo =
            jugador.posicion.y - mitadAlto;

        float cabezaNueva =
            jugador.posicion.y + mitadAlto;

        if (
            jugador.velocidad.y <= 0.0f &&
            pieAnterior >= bloqueBox.max.y - 0.01f &&
            pieNuevo <= bloqueBox.max.y
        )
        {
            jugador.posicion.y =
                bloqueBox.max.y + mitadAlto;

            jugador.velocidad.y = 0.0f;
            jugador.enSuelo = true;

            jugadorBox =
                CrearHitboxJugador(jugador);

            continue;
        }

        if (
            jugador.velocidad.y > 0.0f &&
            cabezaAnterior <= bloqueBox.min.y + 0.01f &&
            cabezaNueva >= bloqueBox.min.y
        )
        {
            jugador.posicion.y =
                bloqueBox.min.y - mitadAlto;

            jugador.velocidad.y = 0.0f;

            jugadorBox =
                CrearHitboxJugador(jugador);
        }
    }
}


//==================================================
// ACTUALIZAR UN JUGADOR
//==================================================

static void ActualizarJugador(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    float deltaTime
)
{
    if (!jugador.activo)
    {
        return;
    }

    if (jugador.cayendo)
    {
        jugador.tiempoRespawn +=
            deltaTime;

        return;
    }

    ActualizarMovimientoHorizontal(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        deltaTime
    );

    ActualizarVertical(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        deltaTime
    );

    if (jugador.posicion.y < -8.0f)
    {
        jugador.cayendo = true;
        jugador.tiempoRespawn = 0.0f;
    }
}


//==================================================
// DIBUJAR JUGADOR PLACEHOLDER
//==================================================

static void DibujarJugador(
    const JugadorPrueba& jugador
)
{
    if (
        !jugador.activo ||
        jugador.cayendo
    )
    {
        return;
    }

    DrawCube(
        jugador.posicion,
        jugador.tamano.x,
        jugador.tamano.y,
        jugador.tamano.z,
        jugador.color
    );

    DrawCubeWires(
        jugador.posicion,
        jugador.tamano.x,
        jugador.tamano.y,
        jugador.tamano.z,
        BLACK
    );
}


//==================================================
// INICIALIZAR
//==================================================

void ZonaPruebas::Inicializar(
    ModoTeclado modoTeclado
)
{
    cantidadBloques = 0;
    volverAlMenu = false;
    mostrarDebug = false;

    modoTecladoActual =
        modoTeclado;

    cantidadJugadoresActivos =
        modoTecladoActual == TECLADO_DIVIDIDO
        ? 2
        : 1;

    //==================================================
    // P1
    //==================================================

    jugadores[0].numero = 1;
    jugadores[0].activo = true;
    jugadores[0].color = RED;
    jugadores[0].posicionSpawn =
    {
        -1.0f,
        1.0f,
        3.5f
    };

    jugadores[0].tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };

    jugadores[0].velocidadMovimiento = 5.0f;
    jugadores[0].fuerzaSalto = 7.2f;
    jugadores[0].gravedad = 18.0f;
    jugadores[0].duracionRespawn = 1.2f;

    //==================================================
    // P2
    //==================================================

    jugadores[1].numero = 2;
    jugadores[1].activo =
        modoTecladoActual ==
        TECLADO_DIVIDIDO;

    jugadores[1].color = BLUE;
    jugadores[1].posicionSpawn =
    {
        1.0f,
        1.0f,
        3.5f
    };

    jugadores[1].tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };

    jugadores[1].velocidadMovimiento = 5.0f;
    jugadores[1].fuerzaSalto = 7.2f;
    jugadores[1].gravedad = 18.0f;
    jugadores[1].duracionRespawn = 1.2f;

    ReiniciarJugadores();

    //==================================================
    // SUELO
    //==================================================

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
    // CAMARA FIJA DETRAS DE LOS JUGADORES
    //==================================================

    /*
        La camara NO sigue a nadie.

        Esta ubicada sobre +Z y mira
        hacia el centro del escenario.
        Por eso vemos a los jugadores
        desde atras cuando avanzan con
        W o flecha arriba hacia -Z.
    */

    camara.position =
    {
        0.0f,
        6.5f,
        13.0f
    };

    camara.target =
    {
        0.0f,
        1.0f,
        -1.0f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


//==================================================
// REINICIAR UN JUGADOR
//==================================================

void ZonaPruebas::ReiniciarJugador(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= MAX_JUGADORES_PRUEBA
    )
    {
        return;
    }

    JugadorPrueba& jugador =
        jugadores[indice];

    jugador.posicion =
        jugador.posicionSpawn;

    jugador.velocidad =
    {
        0.0f,
        0.0f,
        0.0f
    };

    jugador.enSuelo = false;
    jugador.cayendo = false;
    jugador.tiempoRespawn = 0.0f;
}


void ZonaPruebas::ReiniciarJugadores()
{
    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        ReiniciarJugador(i);
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void ZonaPruebas::Actualizar(
    float deltaTime
)
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        volverAlMenu = true;
        return;
    }

    if (IsKeyPressed(KEY_F3))
    {
        mostrarDebug =
            !mostrarDebug;
    }

    if (IsKeyPressed(KEY_R))
    {
        ReiniciarJugadores();
        return;
    }

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

        if (!jugador.activo)
        {
            continue;
        }

        EntradaJugadorPrueba entrada =
            LeerEntradaJugador(
                i,
                modoTecladoActual
            );

        ActualizarJugador(
            jugador,
            entrada,
            bloques,
            cantidadBloques,
            deltaTime
        );

        if (
            jugador.cayendo &&
            jugador.tiempoRespawn >=
                jugador.duracionRespawn
        )
        {
            ReiniciarJugador(i);
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void ZonaPruebas::Dibujar() const
{
    ClearBackground(
        Color
        {
            125,
            190,
            220,
            255
        }
    );

    BeginMode3D(camara);

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

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        const JugadorPrueba& jugador =
            jugadores[i];

        DibujarJugador(jugador);

        if (
            mostrarDebug &&
            jugador.activo &&
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
    }

    DrawGrid(
        30,
        1.0f
    );

    EndMode3D();

    DrawText(
        "ZONA DE PRUEBAS",
        25,
        25,
        30,
        BLACK
    );

    if (
        modoTecladoActual ==
        TECLADO_DIVIDIDO
    )
    {
        DrawText(
            "P1 ROJO: WASD + ESPACIO",
            25,
            70,
            20,
            RED
        );

        DrawText(
            "P2 AZUL: FLECHAS + ENTER",
            25,
            98,
            20,
            BLUE
        );
    }
    else
    {
        DrawText(
            "P1 ROJO: WASD O FLECHAS + ESPACIO",
            25,
            70,
            20,
            RED
        );
    }

    DrawText(
        "R - Respawn de jugadores",
        25,
        136,
        20,
        BLACK
    );

    DrawText(
        "F3 - Debug / Hitboxes",
        25,
        164,
        20,
        BLACK
    );

    DrawText(
        "ESC - Volver al menu",
        25,
        192,
        20,
        BLACK
    );

    int yCaida =
        GetScreenHeight() - 70;

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        const JugadorPrueba& jugador =
            jugadores[i];

        if (
            jugador.activo &&
            jugador.cayendo
        )
        {
            DrawText(
                TextFormat(
                    "P%d SE CAYO - REAPARECIENDO...",
                    jugador.numero
                ),
                25,
                yCaida,
                22,
                jugador.color
            );

            yCaida -= 30;
        }
    }

    if (mostrarDebug)
    {
        int x =
            GetScreenWidth() - 330;

        int y = 30;

        DrawRectangle(
            x - 15,
            y - 15,
            310,
            105 +
                cantidadJugadoresActivos * 115,
            Fade(
                BLACK,
                0.70f
            )
        );

        DrawText(
            "DEBUG",
            x,
            y,
            22,
            LIME
        );

        y += 38;

        for (
            int i = 0;
            i < MAX_JUGADORES_PRUEBA;
            i++
        )
        {
            const JugadorPrueba& jugador =
                jugadores[i];

            if (!jugador.activo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "P%d",
                    jugador.numero
                ),
                x,
                y,
                20,
                jugador.color
            );

            DrawText(
                TextFormat(
                    "X %.2f  Y %.2f  Z %.2f",
                    jugador.posicion.x,
                    jugador.posicion.y,
                    jugador.posicion.z
                ),
                x,
                y + 28,
                18,
                WHITE
            );

            DrawText(
                TextFormat(
                    "Vel Y: %.2f",
                    jugador.velocidad.y
                ),
                x,
                y + 53,
                18,
                WHITE
            );

            DrawText(
                jugador.enSuelo
                ? "En suelo: SI"
                : "En suelo: NO",
                x,
                y + 78,
                18,
                jugador.enSuelo
                ? LIME
                : ORANGE
            );

            y += 112;
        }
    }
}
