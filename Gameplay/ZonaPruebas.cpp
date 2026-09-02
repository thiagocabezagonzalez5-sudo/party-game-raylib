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
// PLATAFORMAS DEL MINIJUEGO
//==================================================

static void AgregarPlataforma(
    ZonaPruebas& zona,
    Vector3 posicion,
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
    bloque.posicionInicial = posicion;

    bloque.tamano =
    {
        2.8f,
        0.60f,
        2.8f
    };

    bloque.color = color;
    bloque.activaColision = true;
    bloque.cayendo = false;
    bloque.velocidadCaida = 0.0f;

    zona.cantidadBloques++;
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

        for (
            int i = 0;
            i < MAX_JUGADORES_PRUEBA;
            i++
        )
        {
            if (
                zona.jugadores[i].activo &&
                zona.jugadores[i].cayendo
            )
            {
                zona.ReiniciarJugador(i);
            }
        }

        zona.numeroRonda++;

        ElegirNuevaPlataformaSegura(
            zona
        );
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

        if (porcentajeVida < 0.0f)
        {
            porcentajeVida =
                0.0f;
        }

        float tamanoActual =
            particula.tamano *
            (
                0.35f +
                0.65f *
                porcentajeVida
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
// COLISION CON PLATAFORMAS
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
        jugador.gravedad *
        deltaTime;

    float posicionAnteriorY =
        jugador.posicion.y;

    jugador.posicion.y +=
        jugador.velocidad.y *
        deltaTime;

    jugador.enSuelo =
        false;

    float mitadAlto =
        jugador.tamano.y /
        2.0f;

    float pieAnterior =
        posicionAnteriorY -
        mitadAlto;

    float cabezaAnterior =
        posicionAnteriorY +
        mitadAlto;

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
// COLISION ENTRE JUGADORES
//==================================================

static void ResolverColisionesEntreJugadores(
    ZonaPruebas& zona
)
{
    const float MARGEN =
        0.002f;

    // Dos pasadas ayudan a separar grupos de 3 o 4 jugadores.
    for (
        int pasada = 0;
        pasada < 2;
        pasada++
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

                    if (
                        fabsf(
                            a.posicion.x -
                            b.posicion.x
                        ) < 0.001f
                    )
                    {
                        direccion =
                            i < j
                            ? -1.0f
                            : 1.0f;
                    }

                    float empuje =
                        solapeX /
                        2.0f +
                        MARGEN;

                    a.posicion.x +=
                        direccion *
                        empuje;

                    b.posicion.x -=
                        direccion *
                        empuje;
                }
                else
                {
                    float direccion =
                        a.posicion.z < b.posicion.z
                        ? -1.0f
                        : 1.0f;

                    if (
                        fabsf(
                            a.posicion.z -
                            b.posicion.z
                        ) < 0.001f
                    )
                    {
                        direccion =
                            i < j
                            ? -1.0f
                            : 1.0f;
                    }

                    float empuje =
                        solapeZ /
                        2.0f +
                        MARGEN;

                    a.posicion.z +=
                        direccion *
                        empuje;

                    b.posicion.z -=
                        direccion *
                        empuje;
                }
            }
        }
    }
}


//==================================================
// DIBUJO
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

    faseMinijuego =
        FASE_ELEGIR_PLATAFORMA;

    indicePlataformaSegura =
        -1;

    numeroRonda =
        1;

    tiempoFase =
        duracionElegirPlataforma;


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
    // 7 PLATAFORMAS: CENTRO + 6 VERTICES DEL HEXAGONO
    //==================================================

    const float RADIO =
        4.2f;

    const float X_DIAGONAL =
        3.64f;

    const float Z_DIAGONAL =
        2.10f;

    Vector3 posiciones[MAX_BLOQUES_PRUEBA] =
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
        i < MAX_BLOQUES_PRUEBA;
        i++
    )
    {
        AgregarPlataforma(
            *this,
            posiciones[i],
            ObtenerColorArcoiris(i)
        );
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

    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -0.65f, 1.0f, 0.55f },
        { 0.65f, 1.0f, 0.55f },
        { -0.65f, 1.0f, -0.55f },
        { 0.65f, 1.0f, -0.55f }
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
            spawns[i];

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
    // CAMARA FIJA DETRAS DE LOS JUGADORES
    //==================================================

    camara.position =
    {
        0.0f,
        10.0f,
        15.5f
    };

    camara.target =
    {
        0.0f,
        0.8f,
        -1.5f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy =
        55.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;


    ActualizarJugadoresConectados(
        *this
    );

    ElegirNuevaPlataformaSegura(
        *this
    );
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


    ActualizarJugadoresConectados(
        *this
    );

    ActualizarParticulas(
        particulas,
        MAX_PARTICULAS_TIERRA,
        deltaTime
    );

    ActualizarMinijuegoColor(
        *this,
        deltaTime
    );


    if (IsKeyPressed(KEY_R))
    {
        ReiniciarPlataformas(
            *this
        );

        ReiniciarJugadores();

        numeroRonda =
            1;

        ElegirNuevaPlataformaSegura(
            *this
        );

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
            deltaTime
        );
    }


    ResolverColisionesEntreJugadores(
        *this
    );
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
    // VACIO / REFERENCIA VISUAL
    //==================================================

    DrawCube(
        Vector3{
            0.0f,
            -10.0f,
            0.0f
        },
        30.0f,
        0.20f,
        30.0f,
        Color{
            35,
            38,
            48,
            255
        }
    );


    //==================================================
    // PLATAFORMAS
    //==================================================

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


    //==================================================
    // TELEVISOR
    //==================================================

    DibujarTelevisor(
        ObtenerColorArcoiris(
            indicePlataformaSegura
        )
    );


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


    EndMode3D();


    //==================================================
    // HUD DEL MINIJUEGO
    //==================================================

    DrawText(
        "PRUEBA MINIJUEGO: COLOR SEGURO",
        25,
        25,
        28,
        BLACK
    );

    Color colorSeguro =
        ObtenerColorArcoiris(
            indicePlataformaSegura
        );

    DrawRectangle(
        25,
        68,
        300,
        55,
        Fade(
            BLACK,
            0.65f
        )
    );

    DrawRectangle(
        35,
        78,
        35,
        35,
        colorSeguro
    );

    DrawRectangleLines(
        35,
        78,
        35,
        35,
        RAYWHITE
    );

    DrawText(
        NombreColorPlataforma(
            indicePlataformaSegura
        ),
        85,
        83,
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
            138,
            24,
            BLACK
        );
    }
    else
    {
        DrawText(
            "SOLO QUEDA EL COLOR DE LA TV!",
            25,
            138,
            24,
            RED
        );
    }

    DrawText(
        TextFormat(
            "RONDA: %d",
            numeroRonda
        ),
        25,
        170,
        20,
        BLACK
    );

    DrawText(
        TextFormat(
            "JUGADORES ACTIVOS: %d / 4",
            cantidadJugadoresActivos
        ),
        25,
        198,
        20,
        BLACK
    );

    DrawText(
        "R - Reiniciar prueba   F3 - Hitboxes   ESC - Menu",
        25,
        GetScreenHeight() - 35,
        18,
        BLACK
    );


    //==================================================
    // ESTADO DE JUGADORES CAIDOS
    //==================================================

    int yCaidos =
        230;

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
                    "P%d CAYO - vuelve en la proxima ronda",
                    jugador.numero
                ),
                25,
                yCaidos,
                18,
                jugador.color
            );

            yCaidos +=
                24;
        }
    }
}
