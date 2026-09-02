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


static float LimitarFloat(
    float valor,
    float minimo,
    float maximo
)
{
    if (valor < minimo)
    {
        return minimo;
    }

    if (valor > maximo)
    {
        return maximo;
    }

    return valor;
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


static const char* NombreColorPlataforma(
    int indice
)
{
    switch (indice)
    {
        case 0: return "ROJO";
        case 1: return "NARANJA";
        case 2: return "AMARILLO";
        case 3: return "VERDE";
        case 4: return "AZUL";
        case 5: return "INDIGO";
        case 6: return "VIOLETA";
    }

    return "?";
}


static Color ObtenerColorArcoiris(
    int indice
)
{
    switch (indice)
    {
        case 0:
            return RED;

        case 1:
            return ORANGE;

        case 2:
            return YELLOW;

        case 3:
            return GREEN;

        case 4:
            return BLUE;

        case 5:
            return Color{
                75,
                0,
                130,
                255
            };

        case 6:
            return VIOLET;
    }

    return WHITE;
}


static const char* NombreModoPrueba(
    ModoZonaPruebas modo
)
{
    switch (modo)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            return "1 - ZONA PRINCIPAL";

        case PRUEBA_COLOR_SEGURO:
            return "2 - COLOR SEGURO";

        case PRUEBA_PELOTAS_EMPUJON:
            return "3 - PELOTAS / EMPUJONES";
    }

    return "PRUEBA";
}


//==================================================
// HITBOXES
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
// BLOQUES / ESCENARIOS
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

    bloque.posicionInicial =
        posicion;

    bloque.tamano =
        tamano;

    bloque.color =
        color;

    bloque.activaColision =
        true;

    bloque.cayendo =
        false;

    bloque.velocidadCaida =
        0.0f;

    zona.cantidadBloques++;
}


static void AgregarPlataformaColor(
    ZonaPruebas& zona,
    Vector3 posicion,
    Color color
)
{
    AgregarBloque(
        zona,
        posicion,
        Vector3{
            2.8f,
            0.60f,
            2.8f
        },
        color
    );
}


static void ReiniciarPlataformas(
    ZonaPruebas& zona
)
{
    for (
        int i = 0;
        i < zona.cantidadBloques;
        i++
    )
    {
        BloquePrueba& bloque =
            zona.bloques[i];

        bloque.posicion =
            bloque.posicionInicial;

        bloque.activaColision =
            true;

        bloque.cayendo =
            false;

        bloque.velocidadCaida =
            0.0f;
    }
}


static void ConfigurarZonaPrincipal(
    ZonaPruebas& zona
)
{
    zona.cantidadBloques =
        0;

    AgregarBloque(
        zona,
        Vector3{
            0.0f,
            -0.5f,
            0.0f
        },
        Vector3{
            14.0f,
            1.0f,
            14.0f
        },
        Color{
            105,
            105,
            115,
            255
        }
    );

    AgregarBloque(
        zona,
        Vector3{
            -3.0f,
            0.5f,
            -2.0f
        },
        Vector3{
            2.5f,
            1.0f,
            2.5f
        },
        ORANGE
    );

    AgregarBloque(
        zona,
        Vector3{
            2.0f,
            1.0f,
            -1.0f
        },
        Vector3{
            3.0f,
            2.0f,
            3.0f
        },
        BLUE
    );

    zona.camara.position =
    {
        0.0f,
        6.5f,
        13.0f
    };

    zona.camara.target =
    {
        0.0f,
        1.0f,
        -1.0f
    };

    zona.camara.fovy =
        50.0f;
}


static void ConfigurarMinijuegoColor(
    ZonaPruebas& zona
)
{
    zona.cantidadBloques =
        0;

    const float RADIO =
        4.2f;

    const float X_DIAGONAL =
        3.64f;

    const float Z_DIAGONAL =
        2.10f;

    Vector3 posiciones[7] =
    {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -RADIO },
        { X_DIAGONAL, 0.0f, -Z_DIAGONAL },
        { X_DIAGONAL, 0.0f, Z_DIAGONAL },
        { 0.0f, 0.0f, RADIO },
        { -X_DIAGONAL, 0.0f, Z_DIAGONAL },
        { -X_DIAGONAL, 0.0f, -Z_DIAGONAL }
    };

    for (
        int i = 0;
        i < 7;
        i++
    )
    {
        AgregarPlataformaColor(
            zona,
            posiciones[i],
            ObtenerColorArcoiris(i)
        );
    }

    zona.faseMinijuego =
        FASE_ELEGIR_PLATAFORMA;

    zona.indicePlataformaSegura =
        -1;

    zona.numeroRonda =
        1;

    zona.tiempoFase =
        zona.duracionElegirPlataforma;

    zona.camara.position =
    {
        0.0f,
        10.0f,
        15.5f
    };

    zona.camara.target =
    {
        0.0f,
        0.8f,
        -1.5f
    };

    zona.camara.fovy =
        55.0f;
}


static void ConfigurarMinijuegoPelotas(
    ZonaPruebas& zona
)
{
    zona.cantidadBloques =
        0;

    AgregarBloque(
        zona,
        Vector3{
            0.0f,
            -0.5f,
            0.0f
        },
        Vector3{
            11.0f,
            1.0f,
            11.0f
        },
        Color{
            74,
            78,
            92,
            255
        }
    );

    zona.camara.position =
    {
        0.0f,
        8.0f,
        13.0f
    };

    zona.camara.target =
    {
        0.0f,
        0.5f,
        0.0f
    };

    zona.camara.fovy =
        52.0f;
}


//==================================================
// MINIJUEGO COLOR SEGURO
//==================================================

static void ElegirNuevaPlataformaSegura(
    ZonaPruebas& zona
)
{
    int anterior =
        zona.indicePlataformaSegura;

    if (zona.cantidadBloques <= 1)
    {
        zona.indicePlataformaSegura =
            0;
    }
    else
    {
        do
        {
            zona.indicePlataformaSegura =
                GetRandomValue(
                    0,
                    zona.cantidadBloques - 1
                );
        }
        while (
            zona.indicePlataformaSegura ==
            anterior
        );
    }

    zona.faseMinijuego =
        FASE_ELEGIR_PLATAFORMA;

    zona.tiempoFase =
        zona.duracionElegirPlataforma;
}


static void TirarPlataformasIncorrectas(
    ZonaPruebas& zona
)
{
    for (
        int i = 0;
        i < zona.cantidadBloques;
        i++
    )
    {
        BloquePrueba& bloque =
            zona.bloques[i];

        if (
            i ==
            zona.indicePlataformaSegura
        )
        {
            bloque.activaColision =
                true;

            bloque.cayendo =
                false;

            continue;
        }

        bloque.activaColision =
            false;

        bloque.cayendo =
            true;

        bloque.velocidadCaida =
            0.0f;
    }

    zona.faseMinijuego =
        FASE_CAIDA_PLATAFORMAS;

    zona.tiempoFase =
        zona.duracionCaidaPlataformas;
}


static void ActualizarCaidaPlataformas(
    ZonaPruebas& zona,
    float deltaTime
)
{
    for (
        int i = 0;
        i < zona.cantidadBloques;
        i++
    )
    {
        BloquePrueba& bloque =
            zona.bloques[i];

        if (!bloque.cayendo)
        {
            continue;
        }

        bloque.velocidadCaida +=
            12.0f * deltaTime;

        bloque.posicion.y -=
            bloque.velocidadCaida *
            deltaTime;
    }
}


static void ActualizarMinijuegoColor(
    ZonaPruebas& zona,
    float deltaTime
)
{
    zona.tiempoFase -=
        deltaTime;

    if (
        zona.faseMinijuego ==
        FASE_ELEGIR_PLATAFORMA
    )
    {
        if (zona.tiempoFase <= 0.0f)
        {
            TirarPlataformasIncorrectas(
                zona
            );
        }

        return;
    }

    ActualizarCaidaPlataformas(
        zona,
        deltaTime
    );

    if (zona.tiempoFase <= 0.0f)
    {
        ReiniciarPlataformas(
            zona
        );

        zona.numeroRonda++;

        ElegirNuevaPlataformaSegura(
            zona
        );

        zona.ReiniciarJugadores();
    }
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
    const int CANTIDAD_CREAR =
        14;

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

        particula.activa =
            true;

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
            cosf(angulo) *
                velocidadHorizontal,

            AleatorioFloat(
                0.8f,
                2.3f
            ),

            sinf(angulo) *
                velocidadHorizontal
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
            particula.activa =
                false;

            continue;
        }

        particula.velocidad.y -=
            8.0f * deltaTime;

        particula.posicion.x +=
            particula.velocidad.x *
            deltaTime;

        particula.posicion.y +=
            particula.velocidad.y *
            deltaTime;

        particula.posicion.z +=
            particula.velocidad.z *
            deltaTime;
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

        porcentajeVida =
            LimitarFloat(
                porcentajeVida,
                0.0f,
                1.0f
            );

        float tamanoActual =
            particula.tamano *
            (
                0.35f +
                0.65f * porcentajeVida
            );

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
// INPUT
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
// CONEXION DE JUGADORES
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
            jugador.activo =
                true;

            jugador.usaGamepad =
                false;

            jugador.indiceGamepad =
                -1;
        }
        else
        {
            int indiceGamepad =
                ObtenerIndiceGamepadParaJugador(
                    i,
                    zona.modoTecladoActual
                );

            jugador.usaGamepad =
                true;

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
            jugador.velocidad = {};
            jugador.empuje = {};
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
// COLISION CON BLOQUES
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
        if (!bloques[i].activaColision)
        {
            continue;
        }

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
        if (!bloques[i].activaColision)
        {
            continue;
        }

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
// MOVIMIENTO
//==================================================

static void ActualizarMovimientoHorizontal(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    bool usarEmpuje,
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

    float movimientoBaseX =
        direccionX *
        jugador.velocidadMovimiento;

    float movimientoBaseZ =
        direccionZ *
        jugador.velocidadMovimiento;

    if (usarEmpuje)
    {
        jugador.velocidad.x =
            movimientoBaseX +
            jugador.empuje.x;

        jugador.velocidad.z =
            movimientoBaseZ +
            jugador.empuje.z;

        float factorFreno =
            1.0f -
            3.8f * deltaTime;

        if (factorFreno < 0.0f)
        {
            factorFreno =
                0.0f;
        }

        jugador.empuje.x *=
            factorFreno;

        jugador.empuje.z *=
            factorFreno;
    }
    else
    {
        jugador.velocidad.x =
            movimientoBaseX;

        jugador.velocidad.z =
            movimientoBaseZ;

        jugador.empuje.x =
            0.0f;

        jugador.empuje.z =
            0.0f;
    }

    ResolverColisionX(
        jugador,
        bloques,
        cantidadBloques,
        jugador.velocidad.x *
        deltaTime
    );

    ResolverColisionZ(
        jugador,
        bloques,
        cantidadBloques,
        jugador.velocidad.z *
        deltaTime
    );
}


static void ActualizarVertical(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    bool permitirSalto,
    float deltaTime
)
{
    if (
        permitirSalto &&
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
        jugador.gravedad *
        deltaTime;

    float posicionAnteriorY =
        jugador.posicion.y;

    jugador.posicion.y +=
        jugador.velocidad.y *
        deltaTime;

    jugador.enSuelo =
        false;

    BoundingBox cajaJugador =
        CrearHitboxJugador(
            jugador
        );

    for (
        int i = 0;
        i < cantidadBloques;
        i++
    )
    {
        if (!bloques[i].activaColision)
        {
            continue;
        }

        BoundingBox cajaBloque =
            CrearHitboxBloque(
                bloques[i]
            );

        if (
            !SolapanXZ(
                cajaJugador,
                cajaBloque
            )
        )
        {
            continue;
        }

        float mitadAltoJugador =
            jugador.tamano.y /
            2.0f;

        float piesAnteriores =
            posicionAnteriorY -
            mitadAltoJugador;

        float piesActuales =
            jugador.posicion.y -
            mitadAltoJugador;

        if (
            jugador.velocidad.y <= 0.0f &&
            piesAnteriores >= cajaBloque.max.y &&
            piesActuales <= cajaBloque.max.y
        )
        {
            jugador.posicion.y =
                cajaBloque.max.y +
                mitadAltoJugador;

            jugador.velocidad.y =
                0.0f;

            jugador.enSuelo =
                true;

            cajaJugador =
                CrearHitboxJugador(
                    jugador
                );
        }
        else if (
            jugador.velocidad.y > 0.0f
        )
        {
            float cabezaAnterior =
                posicionAnteriorY +
                mitadAltoJugador;

            float cabezaActual =
                jugador.posicion.y +
                mitadAltoJugador;

            if (
                cabezaAnterior <= cajaBloque.min.y &&
                cabezaActual >= cajaBloque.min.y
            )
            {
                jugador.posicion.y =
                    cajaBloque.min.y -
                    mitadAltoJugador;

                jugador.velocidad.y =
                    0.0f;

                cajaJugador =
                    CrearHitboxJugador(
                        jugador
                    );
            }
        }
    }
}


static void ActualizarJugador(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    ModoZonaPruebas modoActual,
    float deltaTime
)
{
    if (jugador.cooldownChoque > 0.0f)
    {
        jugador.cooldownChoque -=
            deltaTime;

        if (jugador.cooldownChoque < 0.0f)
        {
            jugador.cooldownChoque =
                0.0f;
        }
    }

    if (jugador.cayendo)
    {
        jugador.tiempoRespawn +=
            deltaTime;

        if (
            modoActual != PRUEBA_COLOR_SEGURO &&
            jugador.tiempoRespawn >=
            jugador.duracionRespawn
        )
        {
            jugador.posicion =
                jugador.posicionSpawn;

            jugador.velocidad = {};
            jugador.empuje = {};
            jugador.enSuelo = false;
            jugador.cayendo = false;
            jugador.tiempoRespawn = 0.0f;
        }

        return;
    }

    bool usarEmpuje =
        modoActual ==
        PRUEBA_PELOTAS_EMPUJON;

    bool permitirSalto =
        modoActual !=
        PRUEBA_PELOTAS_EMPUJON;

    ActualizarMovimientoHorizontal(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        usarEmpuje,
        deltaTime
    );

    ActualizarVertical(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        particulas,
        permitirSalto,
        deltaTime
    );

    if (jugador.posicion.y < -8.0f)
    {
        jugador.cayendo =
            true;

        jugador.tiempoRespawn =
            0.0f;

        jugador.empuje = {};
    }
}


//==================================================
// COLISION ENTRE JUGADORES NORMAL
//==================================================

static void ResolverColisionesJugadoresNormales(
    ZonaPruebas& zona
)
{
    const float MARGEN =
        0.001f;

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        JugadorPrueba& a =
            zona.jugadores[i];

        if (
            !a.activo ||
            a.cayendo
        )
        {
            continue;
        }

        for (
            int j = i + 1;
            j < MAX_JUGADORES_PRUEBA;
            j++
        )
        {
            JugadorPrueba& b =
                zona.jugadores[j];

            if (
                !b.activo ||
                b.cayendo
            )
            {
                continue;
            }

            BoundingBox cajaA =
                CrearHitboxJugador(a);

            BoundingBox cajaB =
                CrearHitboxJugador(b);

            if (
                !CajasSeSolapan(
                    cajaA,
                    cajaB
                )
            )
            {
                continue;
            }

            float solapeX =
                (
                    cajaA.max.x < cajaB.max.x
                    ? cajaA.max.x
                    : cajaB.max.x
                ) -
                (
                    cajaA.min.x > cajaB.min.x
                    ? cajaA.min.x
                    : cajaB.min.x
                );

            float solapeZ =
                (
                    cajaA.max.z < cajaB.max.z
                    ? cajaA.max.z
                    : cajaB.max.z
                ) -
                (
                    cajaA.min.z > cajaB.min.z
                    ? cajaA.min.z
                    : cajaB.min.z
                );

            if (
                solapeX <= 0.0f ||
                solapeZ <= 0.0f
            )
            {
                continue;
            }

            if (solapeX < solapeZ)
            {
                float direccion =
                    a.posicion.x < b.posicion.x
                    ? -1.0f
                    : 1.0f;

                float correccion =
                    solapeX /
                    2.0f +
                    MARGEN;

                a.posicion.x +=
                    direccion *
                    correccion;

                b.posicion.x -=
                    direccion *
                    correccion;
            }
            else
            {
                float direccion =
                    a.posicion.z < b.posicion.z
                    ? -1.0f
                    : 1.0f;

                float correccion =
                    solapeZ /
                    2.0f +
                    MARGEN;

                a.posicion.z +=
                    direccion *
                    correccion;

                b.posicion.z -=
                    direccion *
                    correccion;
            }
        }
    }
}


//==================================================
// COLISION / EMPUJON ENTRE PELOTAS
//==================================================

static void ResolverEmpujonesPelotas(
    ZonaPruebas& zona
)
{
    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        JugadorPrueba& a =
            zona.jugadores[i];

        if (
            !a.activo ||
            a.cayendo
        )
        {
            continue;
        }

        for (
            int j = i + 1;
            j < MAX_JUGADORES_PRUEBA;
            j++
        )
        {
            JugadorPrueba& b =
                zona.jugadores[j];

            if (
                !b.activo ||
                b.cayendo
            )
            {
                continue;
            }

            float diferenciaY =
                fabsf(
                    a.posicion.y -
                    b.posicion.y
                );

            float alturaPermitida =
                (
                    a.tamano.y +
                    b.tamano.y
                ) /
                2.0f;

            if (diferenciaY >= alturaPermitida)
            {
                continue;
            }

            float dx =
                b.posicion.x -
                a.posicion.x;

            float dz =
                b.posicion.z -
                a.posicion.z;

            float distancia =
                sqrtf(
                    dx * dx +
                    dz * dz
                );

            float radioA =
                a.tamano.x /
                2.0f;

            float radioB =
                b.tamano.x /
                2.0f;

            float distanciaMinima =
                radioA +
                radioB;

            if (distancia >= distanciaMinima)
            {
                continue;
            }

            float nx =
                1.0f;

            float nz =
                0.0f;

            if (distancia > 0.001f)
            {
                nx =
                    dx /
                    distancia;

                nz =
                    dz /
                    distancia;
            }
            else if (i > j)
            {
                nx =
                    -1.0f;
            }

            float solape =
                distanciaMinima -
                distancia;

            a.posicion.x -=
                nx *
                solape /
                2.0f;

            a.posicion.z -=
                nz *
                solape /
                2.0f;

            b.posicion.x +=
                nx *
                solape /
                2.0f;

            b.posicion.z +=
                nz *
                solape /
                2.0f;

            float relativo =
                (
                    a.velocidad.x -
                    b.velocidad.x
                ) * nx +
                (
                    a.velocidad.z -
                    b.velocidad.z
                ) * nz;

            if (
                relativo > 0.20f &&
                a.cooldownChoque <= 0.0f &&
                b.cooldownChoque <= 0.0f
            )
            {
                float fuerza =
                    2.8f +
                    relativo *
                    0.55f;

                if (fuerza > 6.5f)
                {
                    fuerza =
                        6.5f;
                }

                a.empuje.x -=
                    nx * fuerza;

                a.empuje.z -=
                    nz * fuerza;

                b.empuje.x +=
                    nx * fuerza;

                b.empuje.z +=
                    nz * fuerza;

                a.cooldownChoque =
                    0.14f;

                b.cooldownChoque =
                    0.14f;
            }
        }
    }
}


//==================================================
// DIBUJO 3D
//==================================================

static void DibujarJugadorCubo(
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


static void DibujarJugadorPelota(
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

    float radio =
        jugador.tamano.x /
        2.0f;

    DrawSphere(
        jugador.posicion,
        radio,
        jugador.color
    );

    DrawSphereWires(
        jugador.posicion,
        radio,
        12,
        16,
        BLACK
    );
}


static void DibujarTelevisor(
    Color colorPantalla
)
{
    Vector3 cuerpo =
    {
        0.0f,
        4.2f,
        -8.2f
    };

    DrawCube(
        cuerpo,
        4.8f,
        3.1f,
        0.55f,
        Color{
            35,
            35,
            42,
            255
        }
    );

    DrawCubeWires(
        cuerpo,
        4.8f,
        3.1f,
        0.55f,
        BLACK
    );

    DrawCube(
        Vector3{
            0.0f,
            4.2f,
            -7.90f
        },
        4.05f,
        2.35f,
        0.08f,
        colorPantalla
    );

    DrawCubeWires(
        Vector3{
            0.0f,
            4.2f,
            -7.85f
        },
        4.10f,
        2.40f,
        0.05f,
        RAYWHITE
    );

    DrawCube(
        Vector3{
            -1.45f,
            2.0f,
            -8.2f
        },
        0.30f,
        1.45f,
        0.30f,
        DARKGRAY
    );

    DrawCube(
        Vector3{
            1.45f,
            2.0f,
            -8.2f
        },
        0.30f,
        1.45f,
        0.30f,
        DARKGRAY
    );

    DrawCube(
        Vector3{
            0.0f,
            1.25f,
            -8.2f
        },
        4.0f,
        0.25f,
        1.0f,
        DARKGRAY
    );
}


static void DibujarArenaPelotas()
{
    DrawCube(
        Vector3{
            0.0f,
            -0.5f,
            0.0f
        },
        11.0f,
        1.0f,
        11.0f,
        Color{
            74,
            78,
            92,
            255
        }
    );

    DrawCubeWires(
        Vector3{
            0.0f,
            -0.5f,
            0.0f
        },
        11.0f,
        1.0f,
        11.0f,
        BLACK
    );

    DrawCubeWires(
        Vector3{
            0.0f,
            0.03f,
            0.0f
        },
        10.0f,
        0.04f,
        10.0f,
        RAYWHITE
    );
}


//==================================================
// CONFIGURAR JUGADORES SEGUN PROTOTIPO
//==================================================

static void ConfigurarJugadoresParaModo(
    ZonaPruebas& zona
)
{
    Color colores[MAX_JUGADORES_PRUEBA] =
    {
        RED,
        BLUE,
        GREEN,
        GOLD
    };

    Vector3 spawnsPrincipal[MAX_JUGADORES_PRUEBA] =
    {
        { -1.2f, 1.0f, 4.0f },
        { 1.2f, 1.0f, 4.0f },
        { -2.4f, 1.0f, 3.0f },
        { 2.4f, 1.0f, 3.0f }
    };

    Vector3 spawnsColor[MAX_JUGADORES_PRUEBA] =
    {
        { -0.65f, 1.0f, 0.55f },
        { 0.65f, 1.0f, 0.55f },
        { -0.65f, 1.0f, -0.55f },
        { 0.65f, 1.0f, -0.55f }
    };

    Vector3 spawnsPelotas[MAX_JUGADORES_PRUEBA] =
    {
        { -2.2f, 0.65f, 2.2f },
        { 2.2f, 0.65f, 2.2f },
        { -2.2f, 0.65f, -2.2f },
        { 2.2f, 0.65f, -2.2f }
    };

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        JugadorPrueba& jugador =
            zona.jugadores[i];

        jugador.numero =
            i + 1;

        jugador.color =
            colores[i];

        if (
            zona.modoActual ==
            PRUEBA_ZONA_PRINCIPAL
        )
        {
            jugador.posicionSpawn =
                spawnsPrincipal[i];

            jugador.tamano =
            {
                0.8f,
                1.4f,
                0.8f
            };

            jugador.velocidadMovimiento =
                5.0f;
        }
        else if (
            zona.modoActual ==
            PRUEBA_COLOR_SEGURO
        )
        {
            jugador.posicionSpawn =
                spawnsColor[i];

            jugador.tamano =
            {
                0.8f,
                1.4f,
                0.8f
            };

            jugador.velocidadMovimiento =
                5.0f;
        }
        else
        {
            jugador.posicionSpawn =
                spawnsPelotas[i];

            jugador.tamano =
            {
                1.30f,
                1.30f,
                1.30f
            };

            jugador.velocidadMovimiento =
                4.4f;
        }

        jugador.fuerzaSalto =
            7.2f;

        jugador.gravedad =
            18.0f;

        jugador.duracionRespawn =
            1.2f;

        jugador.empuje = {};
        jugador.cooldownChoque = 0.0f;

        zona.ReiniciarJugador(i);
    }
}


//==================================================
// INICIALIZAR / CAMBIAR PROTOTIPO
//==================================================

void ZonaPruebas::Inicializar(
    ModoTeclado modoTeclado
)
{
    volverAlMenu =
        false;

    mostrarDebug =
        false;

    modoTecladoActual =
        modoTeclado;

    cantidadJugadoresActivos =
        0;

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.projection =
        CAMERA_PERSPECTIVE;

    for (
        int i = 0;
        i < MAX_PARTICULAS_TIERRA;
        i++
    )
    {
        particulas[i].activa =
            false;
    }

    for (
        int i = 0;
        i < MAX_JUGADORES_PRUEBA;
        i++
    )
    {
        jugadores[i].activo =
            false;

        jugadores[i].usaGamepad =
            false;

        jugadores[i].indiceGamepad =
            -1;
    }

    CambiarModo(
        PRUEBA_ZONA_PRINCIPAL
    );

    ActualizarJugadoresConectados(
        *this
    );
}


void ZonaPruebas::CambiarModo(
    ModoZonaPruebas nuevoModo
)
{
    modoActual =
        nuevoModo;

    for (
        int i = 0;
        i < MAX_PARTICULAS_TIERRA;
        i++
    )
    {
        particulas[i].activa =
            false;
    }

    if (
        modoActual ==
        PRUEBA_ZONA_PRINCIPAL
    )
    {
        ConfigurarZonaPrincipal(
            *this
        );
    }
    else if (
        modoActual ==
        PRUEBA_COLOR_SEGURO
    )
    {
        ConfigurarMinijuegoColor(
            *this
        );

        ElegirNuevaPlataformaSegura(
            *this
        );
    }
    else
    {
        ConfigurarMinijuegoPelotas(
            *this
        );
    }

    ConfigurarJugadoresParaModo(
        *this
    );

    ActualizarJugadoresConectados(
        *this
    );

    ReiniciarJugadores();
}


//==================================================
// REINICIAR JUGADORES
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

    jugador.velocidad = {};
    jugador.empuje = {};

    jugador.enSuelo =
        false;

    jugador.cayendo =
        false;

    jugador.tiempoRespawn =
        0.0f;

    jugador.cooldownChoque =
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
    // CAMBIAR ENTRE PROTOTIPOS
    //==================================================

    if (IsKeyPressed(KEY_ONE))
    {
        CambiarModo(
            PRUEBA_ZONA_PRINCIPAL
        );

        return;
    }

    if (IsKeyPressed(KEY_TWO))
    {
        CambiarModo(
            PRUEBA_COLOR_SEGURO
        );

        return;
    }

    if (IsKeyPressed(KEY_THREE))
    {
        CambiarModo(
            PRUEBA_PELOTAS_EMPUJON
        );

        return;
    }


    ActualizarJugadoresConectados(
        *this
    );

    ActualizarParticulas(
        particulas,
        MAX_PARTICULAS_TIERRA,
        deltaTime
    );

    if (
        modoActual ==
        PRUEBA_COLOR_SEGURO
    )
    {
        ActualizarMinijuegoColor(
            *this,
            deltaTime
        );
    }


    if (IsKeyPressed(KEY_R))
    {
        if (
            modoActual ==
            PRUEBA_COLOR_SEGURO
        )
        {
            ReiniciarPlataformas(
                *this
            );

            numeroRonda =
                1;

            indicePlataformaSegura =
                -1;

            ElegirNuevaPlataformaSegura(
                *this
            );
        }

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
                jugador,
                modoTecladoActual
            );

        ActualizarJugador(
            jugador,
            entrada,
            bloques,
            cantidadBloques,
            particulas,
            modoActual,
            deltaTime
        );
    }


    if (
        modoActual ==
        PRUEBA_PELOTAS_EMPUJON
    )
    {
        ResolverEmpujonesPelotas(
            *this
        );
    }
    else
    {
        ResolverColisionesJugadoresNormales(
            *this
        );
    }
}


//==================================================
// DIBUJAR
//==================================================

void ZonaPruebas::Dibujar() const
{
    ClearBackground(
        Color{
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
    // FONDO / VACIO
    //==================================================

    DrawCube(
        Vector3{
            0.0f,
            -10.0f,
            0.0f
        },
        32.0f,
        0.20f,
        32.0f,
        Color{
            35,
            38,
            48,
            255
        }
    );


    //==================================================
    // ESCENARIO
    //==================================================

    if (
        modoActual ==
        PRUEBA_PELOTAS_EMPUJON
    )
    {
        DibujarArenaPelotas();
    }
    else
    {
        for (
            int i = 0;
            i < cantidadBloques;
            i++
        )
        {
            const BloquePrueba& bloque =
                bloques[i];

            DrawCube(
                bloque.posicion,
                bloque.tamano.x,
                bloque.tamano.y,
                bloque.tamano.z,
                bloque.color
            );

            DrawCubeWires(
                bloque.posicion,
                bloque.tamano.x,
                bloque.tamano.y,
                bloque.tamano.z,
                BLACK
            );

            if (
                mostrarDebug &&
                bloque.activaColision
            )
            {
                DrawBoundingBox(
                    CrearHitboxBloque(
                        bloque
                    ),
                    YELLOW
                );
            }
        }
    }


    if (
        modoActual ==
        PRUEBA_COLOR_SEGURO
    )
    {
        DibujarTelevisor(
            ObtenerColorArcoiris(
                indicePlataformaSegura
            )
        );
    }


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

        if (
            modoActual ==
            PRUEBA_PELOTAS_EMPUJON
        )
        {
            DibujarJugadorPelota(
                jugador
            );
        }
        else
        {
            DibujarJugadorCubo(
                jugador
            );
        }

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


    EndMode3D();


    //==================================================
    // SELECTOR DE PROTOTIPOS
    //==================================================

    DrawRectangle(
        15,
        15,
        390,
        118,
        Fade(
            BLACK,
            0.62f
        )
    );

    DrawText(
        "ZONA DE PRUEBAS",
        25,
        25,
        24,
        RAYWHITE
    );

    DrawText(
        "1 Principal   2 Color seguro   3 Pelotas",
        25,
        58,
        18,
        LIGHTGRAY
    );

    DrawText(
        NombreModoPrueba(
            modoActual
        ),
        25,
        88,
        20,
        GOLD
    );


    //==================================================
    // HUD PARTICULAR DE CADA PROTOTIPO
    //==================================================

    if (
        modoActual ==
        PRUEBA_ZONA_PRINCIPAL
    )
    {
        DrawText(
            "Zona libre para probar movimiento, saltos y colisiones",
            25,
            150,
            20,
            BLACK
        );
    }
    else if (
        modoActual ==
        PRUEBA_COLOR_SEGURO
    )
    {
        Color colorSeguro =
            ObtenerColorArcoiris(
                indicePlataformaSegura
            );

        DrawRectangle(
            25,
            150,
            310,
            55,
            Fade(
                BLACK,
                0.65f
            )
        );

        DrawRectangle(
            35,
            160,
            35,
            35,
            colorSeguro
        );

        DrawText(
            NombreColorPlataforma(
                indicePlataformaSegura
            ),
            85,
            165,
            25,
            RAYWHITE
        );

        if (
            faseMinijuego ==
            FASE_ELEGIR_PLATAFORMA
        )
        {
            int segundos =
                (int)ceilf(
                    tiempoFase
                );

            DrawText(
                TextFormat(
                    "CAEN EN: %d",
                    segundos
                ),
                25,
                218,
                22,
                BLACK
            );
        }
        else
        {
            DrawText(
                "SOLO QUEDA EL COLOR DE LA TV!",
                25,
                218,
                22,
                RED
            );
        }

        DrawText(
            TextFormat(
                "RONDA: %d",
                numeroRonda
            ),
            25,
            248,
            20,
            BLACK
        );
    }
    else
    {
        DrawText(
            "CHOCA CONTRA OTRA PELOTA PARA EMPUJARLA",
            25,
            150,
            20,
            BLACK
        );

        DrawText(
            "Objetivo del prototipo: tirar rivales de la plataforma",
            25,
            180,
            18,
            DARKGRAY
        );
    }


    DrawText(
        TextFormat(
            "JUGADORES ACTIVOS: %d / 4",
            cantidadJugadoresActivos
        ),
        25,
        GetScreenHeight() - 68,
        18,
        BLACK
    );

    DrawText(
        "R - Reiniciar   F3 - Hitboxes   ESC - Menu",
        25,
        GetScreenHeight() - 38,
        18,
        BLACK
    );
}
