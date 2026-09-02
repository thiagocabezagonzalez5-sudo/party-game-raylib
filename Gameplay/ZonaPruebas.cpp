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
// UTILIDADES
//==================================================

static float AleatorioFloat(
    float minimo,
    float maximo
)
{
    float porcentaje =
        (float)GetRandomValue(0, 1000) /
        1000.0f;

    return
        minimo +
        (maximo - minimo) * porcentaje;
}


static int CantidadJugadoresTeclado(
    ModoTeclado modoTeclado
)
{
    return
        modoTeclado == TECLADO_DIVIDIDO
        ? 2
        : 1;
}


static int ObtenerIndiceGamepadParaJugador(
    int indiceJugador,
    ModoTeclado modoTeclado
)
{
    int cantidadTeclado =
        CantidadJugadoresTeclado(
            modoTeclado
        );

    if (indiceJugador < cantidadTeclado)
    {
        return -1;
    }

    return
        indiceJugador -
        cantidadTeclado;
}


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
// PARTICULAS DE TIERRA
//==================================================

static void CrearParticulasSalto(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    const JugadorPrueba& jugador
)
{
    const int CANTIDAD_CREAR = 14;

    int creadas = 0;

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
            AleatorioFloat(
                0.0f,
                6.28318f
            );

        float velocidadHorizontal =
            AleatorioFloat(
                0.7f,
                2.1f
            );

        particula.activa = true;

        particula.posicion =
        {
            jugador.posicion.x +
                AleatorioFloat(-0.22f, 0.22f),

            jugador.posicion.y -
                jugador.tamano.y / 2.0f +
                0.06f,

            jugador.posicion.z +
                AleatorioFloat(-0.22f, 0.22f)
        };

        particula.velocidad =
        {
            cosf(angulo) * velocidadHorizontal,
            AleatorioFloat(0.8f, 2.3f),
            sinf(angulo) * velocidadHorizontal
        };

        particula.vidaMaxima =
            AleatorioFloat(
                0.28f,
                0.52f
            );

        particula.vida =
            particula.vidaMaxima;

        particula.tamano =
            AleatorioFloat(
                0.05f,
                0.13f
            );

        int varianteColor =
            GetRandomValue(0, 2);

        if (varianteColor == 0)
        {
            particula.color =
            {
                116,
                77,
                44,
                255
            };
        }
        else if (varianteColor == 1)
        {
            particula.color =
            {
                145,
                102,
                62,
                255
            };
        }
        else
        {
            particula.color =
            {
                175,
                137,
                88,
                255
            };
        }

        creadas++;
    }
}


static void ActualizarParticulas(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    float deltaTime
)
{
    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        ParticulaTierra& particula =
            particulas[i];

        if (!particula.activa)
        {
            continue;
        }

        particula.vida -=
            deltaTime;

        if (particula.vida <= 0.0f)
        {
            particula.activa = false;
            continue;
        }

        particula.velocidad.y -=
            8.0f * deltaTime;

        particula.posicion.x +=
            particula.velocidad.x * deltaTime;

        particula.posicion.y +=
            particula.velocidad.y * deltaTime;

        particula.posicion.z +=
            particula.velocidad.z * deltaTime;
    }
}


static void DibujarParticulas(
    const ParticulaTierra particulas[],
    int cantidadMaxima
)
{
    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        const ParticulaTierra& particula =
            particulas[i];

        if (!particula.activa)
        {
            continue;
        }

        float porcentajeVida =
            particula.vidaMaxima > 0.0f
            ? particula.vida /
                particula.vidaMaxima
            : 0.0f;

        if (porcentajeVida < 0.0f)
        {
            porcentajeVida = 0.0f;
        }

        float tamanoActual =
            particula.tamano *
            (0.35f + 0.65f * porcentajeVida);

        DrawCube(
            particula.posicion,
            tamanoActual,
            tamanoActual,
            tamanoActual,
            Fade(
                particula.color,
                porcentajeVida
            )
        );
    }
}


//==================================================
// INPUT DE TECLADO
//==================================================

static EntradaJugadorPrueba LeerEntradaTeclado(
    int indiceJugador,
    ModoTeclado modoTeclado
)
{
    EntradaJugadorPrueba entrada;

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

    if (indiceJugador == 0)
    {
        entrada.izquierda =
            IsKeyDown(KEY_A);

        entrada.derecha =
            IsKeyDown(KEY_D);

        entrada.adelante =
            IsKeyDown(KEY_W);

        entrada.atras =
            IsKeyDown(KEY_S);

        entrada.saltar =
            IsKeyPressed(KEY_SPACE);
    }
    else if (indiceJugador == 1)
    {
        entrada.izquierda =
            IsKeyDown(KEY_LEFT);

        entrada.derecha =
            IsKeyDown(KEY_RIGHT);

        entrada.adelante =
            IsKeyDown(KEY_UP);

        entrada.atras =
            IsKeyDown(KEY_DOWN);

        entrada.saltar =
            IsKeyPressed(KEY_ENTER);
    }

    return entrada;
}


//==================================================
// INPUT DE GAMEPAD
//==================================================

static EntradaJugadorPrueba LeerEntradaGamepad(
    int indiceGamepad
)
{
    EntradaJugadorPrueba entrada;

    if (!IsGamepadAvailable(indiceGamepad))
    {
        return entrada;
    }

    const float DEADZONE =
        0.25f;

    float ejeX =
        GetGamepadAxisMovement(
            indiceGamepad,
            GAMEPAD_AXIS_LEFT_X
        );

    float ejeY =
        GetGamepadAxisMovement(
            indiceGamepad,
            GAMEPAD_AXIS_LEFT_Y
        );

    entrada.izquierda =
        IsGamepadButtonDown(
            indiceGamepad,
            GAMEPAD_BUTTON_LEFT_FACE_LEFT
        ) ||
        ejeX < -DEADZONE;

    entrada.derecha =
        IsGamepadButtonDown(
            indiceGamepad,
            GAMEPAD_BUTTON_LEFT_FACE_RIGHT
        ) ||
        ejeX > DEADZONE;

    entrada.adelante =
        IsGamepadButtonDown(
            indiceGamepad,
            GAMEPAD_BUTTON_LEFT_FACE_UP
        ) ||
        ejeY < -DEADZONE;

    entrada.atras =
        IsGamepadButtonDown(
            indiceGamepad,
            GAMEPAD_BUTTON_LEFT_FACE_DOWN
        ) ||
        ejeY > DEADZONE;

    entrada.saltar =
        IsGamepadButtonPressed(
            indiceGamepad,
            GAMEPAD_BUTTON_RIGHT_FACE_DOWN
        );

    return entrada;
}


static EntradaJugadorPrueba LeerEntradaJugador(
    int indiceJugador,
    const JugadorPrueba& jugador,
    ModoTeclado modoTeclado
)
{
    if (jugador.usaGamepad)
    {
        return LeerEntradaGamepad(
            jugador.indiceGamepad
        );
    }

    return LeerEntradaTeclado(
        indiceJugador,
        modoTeclado
    );
}


//==================================================
// CONECTAR / DESCONECTAR GAMEPADS
//==================================================

static void ActualizarJugadoresConectados(
    ZonaPruebas& zona
)
{
    int cantidadTeclado =
        CantidadJugadoresTeclado(
            zona.modoTecladoActual
        );

    zona.cantidadJugadoresActivos =
        0;

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        JugadorPrueba& jugador =
            zona.jugadores[i];

        bool estabaActivo =
            jugador.activo;

        if (i < cantidadTeclado)
        {
            jugador.activo = true;
            jugador.usaGamepad = false;
            jugador.indiceGamepad = -1;
        }
        else
        {
            int indiceGamepad =
                ObtenerIndiceGamepadParaJugador(
                    i,
                    zona.modoTecladoActual
                );

            jugador.usaGamepad = true;
            jugador.indiceGamepad =
                indiceGamepad;

            jugador.activo =
                IsGamepadAvailable(
                    indiceGamepad
                );
        }

        if (
            jugador.activo &&
            !estabaActivo
        )
        {
            zona.ReiniciarJugador(i);
        }

        if (
            !jugador.activo &&
            estabaActivo
        )
        {
            jugador.velocidad =
            {
                0.0f,
                0.0f,
                0.0f
            };

            jugador.cayendo = false;
            jugador.enSuelo = false;
        }

        if (jugador.activo)
        {
            zona.cantidadJugadoresActivos++;
        }
    }
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
            CrearHitboxJugador(
                jugador
            );
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
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    float deltaTime
)
{
    float direccionX =
        0.0f;

    float direccionZ =
        0.0f;

    if (entrada.izquierda)
    {
        direccionX -=
            1.0f;
    }

    if (entrada.derecha)
    {
        direccionX +=
            1.0f;
    }

    if (entrada.adelante)
    {
        direccionZ -=
            1.0f;
    }

    if (entrada.atras)
    {
        direccionZ +=
            1.0f;
    }

    float longitud =
        sqrtf(
            direccionX * direccionX +
            direccionZ * direccionZ
        );

    if (longitud > 0.0f)
    {
        direccionX /=
            longitud;

        direccionZ /=
            longitud;
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
    ParticulaTierra particulas[],
    float deltaTime
)
{
    if (
        jugador.enSuelo &&
        entrada.saltar
    )
    {
        CrearParticulasSalto(
            particulas,
            MAX_PARTICULAS_TIERRA,
            jugador
        );

        jugador.velocidad.y =
            jugador.fuerzaSalto;

        jugador.enSuelo =
            false;
    }

    jugador.velocidad.y -=
        jugador.gravedad * deltaTime;

    float posicionAnteriorY =
        jugador.posicion.y;

    jugador.posicion.y +=
        jugador.velocidad.y * deltaTime;

    jugador.enSuelo =
        false;

    float mitadAlto =
        jugador.tamano.y / 2.0f;

    float pieAnterior =
        posicionAnteriorY - mitadAlto;

    float cabezaAnterior =
        posicionAnteriorY + mitadAlto;

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

        if (
            jugador.velocidad.y <= 0.0f &&
            pieAnterior >= bloqueBox.max.y - 0.01f &&
            pieNuevo <= bloqueBox.max.y
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

        if (
            jugador.velocidad.y > 0.0f &&
            cabezaAnterior <= bloqueBox.min.y + 0.01f &&
            cabezaNueva >= bloqueBox.min.y
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
// ACTUALIZAR UN JUGADOR
//==================================================

static void ActualizarJugador(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
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
        particulas,
        deltaTime
    );

    if (jugador.posicion.y < -8.0f)
    {
        jugador.cayendo =
            true;

        jugador.tiempoRespawn =
            0.0f;
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
    cantidadBloques =
        0;

    volverAlMenu =
        false;

    mostrarDebug =
        false;

    modoTecladoActual =
        modoTeclado;

    cantidadJugadoresActivos =
        0;


    //==================================================
    // LIMPIAR PARTICULAS
    //==================================================

    for (
        int i = 0;
        i < MAX_PARTICULAS_TIERRA;
        i++
    )
    {
        particulas[i].activa =
            false;
    }


    //==================================================
    // CONFIGURAR LOS 4 SLOTS DE JUGADOR
    //==================================================

    Color colores[MAX_JUGADORES_PRUEBA] =
    {
        RED,
        BLUE,
        GREEN,
        GOLD
    };

    float posicionesX[MAX_JUGADORES_PRUEBA] =
    {
        -1.8f,
        -0.6f,
        0.6f,
        1.8f
    };

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

        jugador.numero =
            i + 1;

        jugador.activo =
            false;

        jugador.usaGamepad =
            false;

        jugador.indiceGamepad =
            -1;

        jugador.color =
            colores[i];

        jugador.posicionSpawn =
        {
            posicionesX[i],
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

        ReiniciarJugador(i);
    }


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

    camara.fovy =
        50.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;


    //==================================================
    // DETECTAR TECLADOS Y MANDOS INICIALES
    //==================================================

    ActualizarJugadoresConectados(
        *this
    );
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

    jugador.enSuelo =
        false;

    jugador.cayendo =
        false;

    jugador.tiempoRespawn =
        0.0f;
}


void ZonaPruebas::ReiniciarJugadores()
{
    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        if (jugadores[i].activo)
        {
            ReiniciarJugador(i);
        }
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
        volverAlMenu =
            true;

        return;
    }

    if (IsKeyPressed(KEY_F3))
    {
        mostrarDebug =
            !mostrarDebug;
    }


    //==================================================
    // MANDOS CONECTADOS EN CALIENTE
    //==================================================

    ActualizarJugadoresConectados(
        *this
    );


    //==================================================
    // PARTICULAS
    //==================================================

    ActualizarParticulas(
        particulas,
        MAX_PARTICULAS_TIERRA,
        deltaTime
    );


    if (IsKeyPressed(KEY_R))
    {
        ReiniciarJugadores();
        return;
    }


    //==================================================
    // JUGADORES
    //==================================================

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
                jugador,
                modoTecladoActual
            );

        ActualizarJugador(
            jugador,
            entrada,
            bloques,
            cantidadBloques,
            particulas,
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

    BeginMode3D(
        camara
    );


    //==================================================
    // BLOQUES
    //==================================================

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


    //==================================================
    // PARTICULAS
    //==================================================

    DibujarParticulas(
        particulas,
        MAX_PARTICULAS_TIERRA
    );


    //==================================================
    // JUGADORES
    //==================================================

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        const JugadorPrueba& jugador =
            jugadores[i];

        DibujarJugador(
            jugador
        );

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

    int yControl =
        70;

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

        if (jugador.usaGamepad)
        {
            DrawText(
                TextFormat(
                    "P%d: MANDO %d - STICK/CRUCETA + A",
                    jugador.numero,
                    jugador.indiceGamepad + 1
                ),
                25,
                yControl,
                20,
                jugador.color
            );
        }
        else if (
            modoTecladoActual ==
            TECLADO_COMPLETO
        )
        {
            DrawText(
                "P1: WASD O FLECHAS + ESPACIO",
                25,
                yControl,
                20,
                jugador.color
            );
        }
        else if (i == 0)
        {
            DrawText(
                "P1: WASD + ESPACIO",
                25,
                yControl,
                20,
                jugador.color
            );
        }
        else
        {
            DrawText(
                "P2: FLECHAS + ENTER",
                25,
                yControl,
                20,
                jugador.color
            );
        }

        yControl +=
            28;
    }

    DrawText(
        TextFormat(
            "JUGADORES ACTIVOS: %d / 4",
            cantidadJugadoresActivos
        ),
        25,
        yControl + 8,
        20,
        BLACK
    );

    DrawText(
        "R - Respawn de jugadores",
        25,
        yControl + 38,
        20,
        BLACK
    );

    DrawText(
        "F3 - Debug / Hitboxes",
        25,
        yControl + 66,
        20,
        BLACK
    );

    DrawText(
        "ESC - Volver al menu",
        25,
        yControl + 94,
        20,
        BLACK
    );


    //==================================================
    // MENSAJES DE CAIDA
    //==================================================

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

            yCaida -=
                30;
        }
    }


    //==================================================
    // DEBUG
    //==================================================

    if (mostrarDebug)
    {
        int x =
            GetScreenWidth() - 350;

        int y =
            30;

        DrawRectangle(
            x - 15,
            y - 15,
            330,
            105 +
                cantidadJugadoresActivos * 137,
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

        y +=
            38;

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

            if (jugador.usaGamepad)
            {
                DrawText(
                    TextFormat(
                        "Control: mando %d",
                        jugador.indiceGamepad + 1
                    ),
                    x,
                    y + 103,
                    18,
                    LIGHTGRAY
                );
            }
            else
            {
                DrawText(
                    "Control: teclado",
                    x,
                    y + 103,
                    18,
                    LIGHTGRAY
                );
            }

            y +=
                134;
        }
    }
}
