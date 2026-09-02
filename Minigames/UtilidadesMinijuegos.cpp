#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


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


int CantidadJugadoresTeclado(
    ModoTeclado modoTeclado
)
{
    return
        modoTeclado == TECLADO_DIVIDIDO
        ? 2
        : 1;
}


int ObtenerIndiceGamepadParaJugador(
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
// INPUT
//==================================================

static EntradaJugadorPrueba LeerEntradaTeclado(
    int indiceJugador,
    ModoTeclado modoTeclado
)
{
    EntradaJugadorPrueba entrada{};

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

        entrada.golpear =
            IsKeyPressed(KEY_E);

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

        entrada.golpear =
            IsKeyPressed(KEY_E);
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

        entrada.golpear =
            IsKeyPressed(KEY_RIGHT_SHIFT);
    }

    return entrada;
}


static EntradaJugadorPrueba LeerEntradaGamepad(
    int indiceGamepad
)
{
    EntradaJugadorPrueba entrada{};

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

    entrada.golpear =
        IsGamepadButtonPressed(
            indiceGamepad,
            GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
        );

    return entrada;
}


EntradaJugadorPrueba LeerEntradaJugadorPrueba(
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

void ActualizarJugadoresConectadosPrueba(
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    int& cantidadActivos,
    ModoTeclado modoTeclado
)
{
    int cantidadTeclado =
        CantidadJugadoresTeclado(
            modoTeclado
        );

    cantidadActivos =
        0;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

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
                    modoTeclado
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
            ReiniciarJugadorPrueba(
                jugador
            );
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
            cantidadActivos++;
        }
    }
}


//==================================================
// BLOQUES / HITBOXES
//==================================================

void AgregarBloquePrueba(
    BloquePrueba bloques[],
    int& cantidadBloques,
    int cantidadMaxima,
    Vector3 posicion,
    Vector3 tamano,
    Color color
)
{
    if (cantidadBloques >= cantidadMaxima)
    {
        return;
    }

    BloquePrueba& bloque =
        bloques[cantidadBloques];

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

    cantidadBloques++;
}


void ReiniciarBloquesPrueba(
    BloquePrueba bloques[],
    int cantidadBloques
)
{
    for (
        int i = 0;
        i < cantidadBloques;
        i++
    )
    {
        bloques[i].posicion =
            bloques[i].posicionInicial;

        bloques[i].activaColision =
            true;

        bloques[i].cayendo =
            false;

        bloques[i].velocidadCaida =
            0.0f;
    }
}


BoundingBox CrearHitboxBloquePrueba(
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


BoundingBox CrearHitboxJugadorPrueba(
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
// PARTICULAS
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


void ActualizarParticulasTierra(
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


void DibujarParticulasTierra(
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
// MOVIMIENTO Y COLISION CON BLOQUES
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
        CrearHitboxJugadorPrueba(
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
            CrearHitboxBloquePrueba(
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
            CrearHitboxJugadorPrueba(
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
        CrearHitboxJugadorPrueba(
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
            CrearHitboxBloquePrueba(
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
            CrearHitboxJugadorPrueba(
                jugador
            );
    }
}


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

        jugador.direccionMirada.x =
            direccionX;

        jugador.direccionMirada.y =
            0.0f;

        jugador.direccionMirada.z =
            direccionZ;
    }

    if (usarEmpuje)
    {
        // Movimiento de pelota sobre hielo:
        // la direccion acelera, pero no reemplaza
        // inmediatamente la velocidad que ya tenia.
        const float ACELERACION =
            11.0f;

        const float ROZAMIENTO_HIELO =
            0.16f;

        const float VELOCIDAD_MAXIMA =
            16.0f;


        jugador.velocidad.x +=
            direccionX *
            ACELERACION *
            deltaTime;

        jugador.velocidad.z +=
            direccionZ *
            ACELERACION *
            deltaTime;


        // Los choques guardan un impulso que se aplica
        // una sola vez durante el siguiente movimiento.
        jugador.velocidad.x +=
            jugador.empuje.x;

        jugador.velocidad.z +=
            jugador.empuje.z;

        jugador.empuje.x =
            0.0f;

        jugador.empuje.z =
            0.0f;


        float factorRozamiento =
            1.0f -
            ROZAMIENTO_HIELO *
            deltaTime;

        if (factorRozamiento < 0.0f)
        {
            factorRozamiento =
                0.0f;
        }


        jugador.velocidad.x *=
            factorRozamiento;

        jugador.velocidad.z *=
            factorRozamiento;


        float velocidadHorizontal =
            sqrtf(
                jugador.velocidad.x *
                jugador.velocidad.x +
                jugador.velocidad.z *
                jugador.velocidad.z
            );

        if (
            velocidadHorizontal >
            VELOCIDAD_MAXIMA
        )
        {
            float factorLimite =
                VELOCIDAD_MAXIMA /
                velocidadHorizontal;

            jugador.velocidad.x *=
                factorLimite;

            jugador.velocidad.z *=
                factorLimite;
        }
    }
    else
    {
        jugador.velocidad.x =
            direccionX *
            jugador.velocidadMovimiento +
            jugador.empuje.x;

        jugador.velocidad.z =
            direccionZ *
            jugador.velocidadMovimiento +
            jugador.empuje.z;

        float factorFrenoEmpuje =
            1.0f -
            5.5f * deltaTime;

        if (factorFrenoEmpuje < 0.0f)
        {
            factorFrenoEmpuje =
                0.0f;
        }

        jugador.empuje.x *=
            factorFrenoEmpuje;

        jugador.empuje.z *=
            factorFrenoEmpuje;
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
    int cantidadParticulas,
    bool permitirSalto,
    float deltaTime
)
{
    const float VELOCIDAD_GOLPE_SUELO =
        24.0f;

    if (
        jugador.golpeSueloActivo &&
        jugador.velocidad.y >
            -VELOCIDAD_GOLPE_SUELO
    )
    {
        jugador.velocidad.y =
            -VELOCIDAD_GOLPE_SUELO;
    }

    if (
        permitirSalto &&
        jugador.enSuelo &&
        entrada.saltar &&
        !jugador.golpeSueloActivo
    )
    {
        CrearParticulasSalto(
            particulas,
            cantidadParticulas,
            jugador
        );

        jugador.velocidad.y =
            jugador.fuerzaSalto;

        jugador.enSuelo =
            false;
    }
    else if (
        permitirSalto &&
        !jugador.enSuelo &&
        entrada.saltar &&
        !jugador.golpeSueloActivo
    )
    {
        jugador.golpeSueloActivo =
            true;

        jugador.golpeando =
            false;

        jugador.velocidad.y =
            -VELOCIDAD_GOLPE_SUELO;
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
        CrearHitboxJugadorPrueba(
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
            CrearHitboxBloquePrueba(
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
            bool impactoGolpeSuelo =
                jugador.golpeSueloActivo;

            jugador.posicion.y =
                cajaBloque.max.y +
                mitadAltoJugador;

            jugador.velocidad.y =
                0.0f;

            jugador.enSuelo =
                true;

            jugador.golpeSueloActivo =
                false;

            if (impactoGolpeSuelo)
            {
                jugador.impactoGolpeSuelo =
                    true;
            }

            cajaJugador =
                CrearHitboxJugadorPrueba(
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
                    CrearHitboxJugadorPrueba(
                        jugador
                    );
            }
        }
    }
}


void ReiniciarJugadorPrueba(
    JugadorPrueba& jugador
)
{
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

    jugador.direccionMirada =
    {
        0.0f,
        0.0f,
        -1.0f
    };

    jugador.golpeSueloActivo =
        false;

    jugador.impactoGolpeSuelo =
        false;

    jugador.aplastado =
        false;

    jugador.tiempoAplastado =
        0.0f;

    jugador.tiempoInmunidad =
        0.0f;

    jugador.golpeando =
        false;

    jugador.golpeYaConecto =
        false;

    jugador.tiempoGolpe =
        0.0f;

    jugador.cooldownGolpe =
        0.0f;
}


void ActualizarJugadorPrueba(
    JugadorPrueba& jugador,
    const EntradaJugadorPrueba& entrada,
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
    if (jugador.tiempoInmunidad > 0.0f)
    {
        jugador.tiempoInmunidad -=
            deltaTime;

        if (jugador.tiempoInmunidad < 0.0f)
        {
            jugador.tiempoInmunidad =
                0.0f;
        }
    }

    if (jugador.cooldownGolpe > 0.0f)
    {
        jugador.cooldownGolpe -=
            deltaTime;

        if (jugador.cooldownGolpe < 0.0f)
        {
            jugador.cooldownGolpe =
                0.0f;
        }
    }

    if (jugador.tiempoGolpe > 0.0f)
    {
        jugador.tiempoGolpe -=
            deltaTime;

        if (jugador.tiempoGolpe <= 0.0f)
        {
            jugador.tiempoGolpe =
                0.0f;

            jugador.golpeando =
                false;
        }
    }

    if (jugador.aplastado)
    {
        jugador.tiempoAplastado -=
            deltaTime;

        jugador.velocidad.x =
            0.0f;

        jugador.velocidad.z =
            0.0f;

        jugador.empuje = {};

        if (jugador.tiempoAplastado <= 0.0f)
        {
            jugador.tiempoAplastado =
                0.0f;

            jugador.aplastado =
                false;

            jugador.tiempoInmunidad =
                1.6f;
        }

        EntradaJugadorPrueba entradaVacia{};

        ActualizarVertical(
            jugador,
            entradaVacia,
            bloques,
            cantidadBloques,
            particulas,
            cantidadParticulas,
            false,
            deltaTime
        );

        if (jugador.posicion.y < -8.0f)
        {
            jugador.cayendo =
                true;

            jugador.tiempoRespawn =
                0.0f;

            jugador.aplastado =
                false;

            jugador.empuje = {};
        }

        return;
    }

    if (
        entrada.golpear &&
        jugador.cooldownGolpe <= 0.0f &&
        !jugador.golpeSueloActivo
    )
    {
        jugador.golpeando =
            true;

        jugador.golpeYaConecto =
            false;

        jugador.tiempoGolpe =
            0.20f;

        jugador.cooldownGolpe =
            0.48f;
    }

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
            respawnAutomatico &&
            jugador.tiempoRespawn >=
            jugador.duracionRespawn
        )
        {
            ReiniciarJugadorPrueba(
                jugador
            );
        }

        return;
    }

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
        cantidadParticulas,
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

        jugador.golpeSueloActivo =
            false;

        jugador.impactoGolpeSuelo =
            false;
    }
}


//==================================================
// COLISION ENTRE JUGADORES
//==================================================

void ResolverColisionesJugadoresNormales(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    const float MARGEN =
        0.001f;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorPrueba& a =
            jugadores[i];

        if (
            !a.activo ||
            a.cayendo
        )
        {
            continue;
        }

        for (
            int j = i + 1;
            j < cantidadMaxima;
            j++
        )
        {
            JugadorPrueba& b =
                jugadores[j];

            if (
                !b.activo ||
                b.cayendo
            )
            {
                continue;
            }

            BoundingBox cajaA =
                CrearHitboxJugadorPrueba(a);

            BoundingBox cajaB =
                CrearHitboxJugadorPrueba(b);

            if (!CajasSeSolapan(cajaA, cajaB))
            {
                continue;
            }

            float solapeX =
                (
                    a.posicion.x < b.posicion.x
                    ? cajaA.max.x - cajaB.min.x
                    : cajaB.max.x - cajaA.min.x
                );

            float solapeZ =
                (
                    a.posicion.z < b.posicion.z
                    ? cajaA.max.z - cajaB.min.z
                    : cajaB.max.z - cajaA.min.z
                );

            if (solapeX < solapeZ)
            {
                float correccion =
                    solapeX / 2.0f +
                    MARGEN;

                if (a.posicion.x < b.posicion.x)
                {
                    a.posicion.x -= correccion;
                    b.posicion.x += correccion;
                }
                else
                {
                    a.posicion.x += correccion;
                    b.posicion.x -= correccion;
                }
            }
            else
            {
                float correccion =
                    solapeZ / 2.0f +
                    MARGEN;

                if (a.posicion.z < b.posicion.z)
                {
                    a.posicion.z -= correccion;
                    b.posicion.z += correccion;
                }
                else
                {
                    a.posicion.z += correccion;
                    b.posicion.z -= correccion;
                }
            }
        }
    }
}


void ResolverColisionesPelotas(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorPrueba& a =
            jugadores[i];

        if (
            !a.activo ||
            a.cayendo
        )
        {
            continue;
        }

        for (
            int j = i + 1;
            j < cantidadMaxima;
            j++
        )
        {
            JugadorPrueba& b =
                jugadores[j];

            if (
                !b.activo ||
                b.cayendo
            )
            {
                continue;
            }

            float diferenciaY =
                fabsf(
                    b.posicion.y -
                    a.posicion.y
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
                // La fuerza nace de la velocidad relativa:
                // un roce lento mueve poco y una embestida
                // a velocidad alta lanza mucho mas lejos.
                float fuerza =
                    0.65f +
                    relativo *
                    1.05f;

                if (fuerza > 16.0f)
                {
                    fuerza =
                        16.0f;
                }

                a.empuje.x -=
                    nx *
                    fuerza *
                    0.35f;

                a.empuje.z -=
                    nz *
                    fuerza *
                    0.35f;

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
// GOLPE AL SUELO
//==================================================

bool ResolverGolpesSuelo(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool huboImpacto =
        false;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorPrueba& atacante =
            jugadores[i];

        if (
            !atacante.activo ||
            !atacante.impactoGolpeSuelo
        )
        {
            continue;
        }

        huboImpacto =
            true;

        atacante.impactoGolpeSuelo =
            false;

        for (
            int j = 0;
            j < cantidadMaxima;
            j++
        )
        {
            if (i == j)
            {
                continue;
            }

            JugadorPrueba& objetivo =
                jugadores[j];

            if (
                !objetivo.activo ||
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
                sqrtf(
                    dx * dx +
                    dz * dz
                );

            const float RADIO_APLASTAMIENTO =
                1.05f;

            if (
                distancia >
                RADIO_APLASTAMIENTO
            )
            {
                continue;
            }

            float diferenciaAltura =
                fabsf(
                    objetivo.posicion.y -
                    atacante.posicion.y
                );

            if (diferenciaAltura > 1.15f)
            {
                continue;
            }

            objetivo.aplastado =
                true;

            objetivo.tiempoAplastado =
                1.15f;

            objetivo.golpeando =
                false;

            objetivo.golpeSueloActivo =
                false;

            objetivo.velocidad.x =
                0.0f;

            objetivo.velocidad.z =
                0.0f;

            objetivo.empuje = {};
        }
    }

    return huboImpacto;
}


//==================================================
// GOLPE HORIZONTAL
//==================================================

void ResolverGolpesJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorPrueba& atacante =
            jugadores[i];

        if (
            !atacante.activo ||
            atacante.cayendo ||
            atacante.aplastado ||
            !atacante.golpeando ||
            atacante.golpeYaConecto
        )
        {
            continue;
        }

        for (
            int j = 0;
            j < cantidadMaxima;
            j++
        )
        {
            if (i == j)
            {
                continue;
            }

            JugadorPrueba& objetivo =
                jugadores[j];

            if (
                !objetivo.activo ||
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
                sqrtf(
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
                fabsf(
                    objetivo.posicion.y -
                    atacante.posicion.y
                ) > 1.0f
            )
            {
                continue;
            }

            float nx =
                dx /
                distancia;

            float nz =
                dz /
                distancia;

            float frente =
                nx * atacante.direccionMirada.x +
                nz * atacante.direccionMirada.z;

            if (frente < 0.25f)
            {
                continue;
            }

            const float FUERZA_GOLPE =
                4.2f;

            objetivo.empuje.x +=
                nx * FUERZA_GOLPE;

            objetivo.empuje.z +=
                nz * FUERZA_GOLPE;

            atacante.golpeYaConecto =
                true;

            break;
        }
    }
}


//==================================================
// DIBUJO
//==================================================

void DibujarJugadorCuboPrueba(
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

    float alturaVisual =
        jugador.aplastado
        ? jugador.tamano.y * 0.24f
        : jugador.tamano.y;

    Vector3 posicionVisual =
        jugador.posicion;

    posicionVisual.y -=
        (
            jugador.tamano.y -
            alturaVisual
        ) /
        2.0f;

    float alpha =
        1.0f;

    if (jugador.tiempoInmunidad > 0.0f)
    {
        int faseParpadeo =
            (int)(
                jugador.tiempoInmunidad *
                12.0f
            );

        alpha =
            faseParpadeo % 2 == 0
            ? 0.20f
            : 1.0f;
    }

    Color colorVisual =
        Fade(
            jugador.color,
            alpha
        );

    DrawCube(
        posicionVisual,
        jugador.tamano.x,
        alturaVisual,
        jugador.tamano.z,
        colorVisual
    );

    DrawCubeWires(
        posicionVisual,
        jugador.tamano.x,
        alturaVisual,
        jugador.tamano.z,
        Fade(
            BLACK,
            alpha
        )
    );

    if (
        jugador.golpeando &&
        !jugador.aplastado
    )
    {
        Vector3 posicionGolpe =
        {
            jugador.posicion.x +
                jugador.direccionMirada.x *
                0.72f,

            jugador.posicion.y +
                0.12f,

            jugador.posicion.z +
                jugador.direccionMirada.z *
                0.72f
        };

        DrawCube(
            posicionGolpe,
            0.34f,
            0.34f,
            0.34f,
            colorVisual
        );

        DrawCubeWires(
            posicionGolpe,
            0.34f,
            0.34f,
            0.34f,
            Fade(BLACK, alpha)
        );
    }
}


void DibujarJugadorPelotaPrueba(
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
