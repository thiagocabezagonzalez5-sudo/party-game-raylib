#include "Minigames/MinijuegoTronco.h"

#include "Minigames/UtilidadesMinijuegos.h"


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_TRONCO =
    2.5f;

static const float DURACION_PARTIDA_TRONCO =
    35.0f;

static const float DURACION_RONDA_TRONCO =
    2.4f;


//==================================================
// UTILIDADES DE ENTRADA
//==================================================

static int ContarJugadoresActivosTronco(
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    int cantidad =
        0;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (jugadores[i].activo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static bool AccionTecladoPresionada(
    int indiceJugador,
    AccionTronco accion,
    ModoTeclado modoTeclado
)
{
    if (modoTeclado == TECLADO_COMPLETO)
    {
        if (indiceJugador != 0)
        {
            return false;
        }

        switch (accion)
        {
            case ACCION_TRONCO_ARRIBA:
                return IsKeyPressed(KEY_W);

            case ACCION_TRONCO_ABAJO:
                return IsKeyPressed(KEY_S);

            case ACCION_TRONCO_IZQUIERDA:
                return IsKeyPressed(KEY_A);

            case ACCION_TRONCO_DERECHA:
                return IsKeyPressed(KEY_D);
        }
    }

    if (indiceJugador == 0)
    {
        switch (accion)
        {
            case ACCION_TRONCO_ARRIBA:
                return IsKeyPressed(KEY_W);

            case ACCION_TRONCO_ABAJO:
                return IsKeyPressed(KEY_S);

            case ACCION_TRONCO_IZQUIERDA:
                return IsKeyPressed(KEY_A);

            case ACCION_TRONCO_DERECHA:
                return IsKeyPressed(KEY_D);
        }
    }
    else if (indiceJugador == 1)
    {
        switch (accion)
        {
            case ACCION_TRONCO_ARRIBA:
                return IsKeyPressed(KEY_UP);

            case ACCION_TRONCO_ABAJO:
                return IsKeyPressed(KEY_DOWN);

            case ACCION_TRONCO_IZQUIERDA:
                return IsKeyPressed(KEY_LEFT);

            case ACCION_TRONCO_DERECHA:
                return IsKeyPressed(KEY_RIGHT);
        }
    }

    return false;
}


static bool AccionGamepadPresionada(
    int indiceGamepad,
    AccionTronco accion
)
{
    if (!IsGamepadAvailable(indiceGamepad))
    {
        return false;
    }

    switch (accion)
    {
        case ACCION_TRONCO_ARRIBA:
            return IsGamepadButtonPressed(
                indiceGamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_UP
            );

        case ACCION_TRONCO_ABAJO:
            return IsGamepadButtonPressed(
                indiceGamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_DOWN
            );

        case ACCION_TRONCO_IZQUIERDA:
            return IsGamepadButtonPressed(
                indiceGamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_LEFT
            );

        case ACCION_TRONCO_DERECHA:
            return IsGamepadButtonPressed(
                indiceGamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
            );
    }

    return false;
}


static bool AccionJugadorPresionada(
    int indiceJugador,
    const JugadorPrueba& jugador,
    AccionTronco accion,
    ModoTeclado modoTeclado
)
{
    if (jugador.usaGamepad)
    {
        return AccionGamepadPresionada(
            jugador.indiceGamepad,
            accion
        );
    }

    return AccionTecladoPresionada(
        indiceJugador,
        accion,
        modoTeclado
    );
}


static bool AlgunaAccionJugadorPresionada(
    int indiceJugador,
    const JugadorPrueba& jugador,
    ModoTeclado modoTeclado
)
{
    for (
        int accion = ACCION_TRONCO_ARRIBA;
        accion <= ACCION_TRONCO_DERECHA;
        accion++
    )
    {
        if (
            AccionJugadorPresionada(
                indiceJugador,
                jugador,
                (AccionTronco)accion,
                modoTeclado
            )
        )
        {
            return true;
        }
    }

    return false;
}


static const char* TextoAccionTronco(
    int indiceJugador,
    const JugadorPrueba& jugador,
    AccionTronco accion,
    ModoTeclado modoTeclado
)
{
    if (jugador.usaGamepad)
    {
        switch (accion)
        {
            case ACCION_TRONCO_ARRIBA:
                return "Y";

            case ACCION_TRONCO_ABAJO:
                return "A";

            case ACCION_TRONCO_IZQUIERDA:
                return "X";

            case ACCION_TRONCO_DERECHA:
                return "B";
        }
    }

    if (
        modoTeclado == TECLADO_DIVIDIDO &&
        indiceJugador == 1
    )
    {
        switch (accion)
        {
            case ACCION_TRONCO_ARRIBA:
                return "FLECHA ARRIBA";

            case ACCION_TRONCO_ABAJO:
                return "FLECHA ABAJO";

            case ACCION_TRONCO_IZQUIERDA:
                return "FLECHA IZQ";

            case ACCION_TRONCO_DERECHA:
                return "FLECHA DER";
        }
    }

    switch (accion)
    {
        case ACCION_TRONCO_ARRIBA:
            return "W";

        case ACCION_TRONCO_ABAJO:
            return "S";

        case ACCION_TRONCO_IZQUIERDA:
            return "A";

        case ACCION_TRONCO_DERECHA:
            return "D";
    }

    return "?";
}


static const char* TextoDispositivoTronco(
    int indiceJugador,
    const JugadorPrueba& jugador,
    ModoTeclado modoTeclado
)
{
    if (jugador.usaGamepad)
    {
        return TextFormat(
            "CONTROL %d",
            jugador.indiceGamepad + 1
        );
    }

    if (
        modoTeclado == TECLADO_DIVIDIDO &&
        indiceJugador == 1
    )
    {
        return "TECLADO FLECHAS";
    }

    return "TECLADO WASD";
}


//==================================================
// INICIALIZAR
//==================================================

void MinijuegoTronco::Inicializar()
{
    camara.position =
    {
        0.0f,
        6.5f,
        12.0f
    };

    camara.target =
    {
        0.0f,
        1.0f,
        0.0f
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
}


void MinijuegoTronco::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Color colores[MAX_JUGADORES_PRUEBA] =
    {
        RED,
        BLUE,
        GREEN,
        GOLD
    };

    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -3.8f, 0.7f, 2.2f },
        { 3.8f, 0.7f, 2.2f },
        { -3.8f, 0.7f, -1.3f },
        { 3.8f, 0.7f, -1.3f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (
        int i = 0;
        i < limite;
        i++
    )
    {
        jugadores[i].numero =
            i + 1;

        jugadores[i].color =
            colores[i];

        jugadores[i].posicionSpawn =
            spawns[i];

        jugadores[i].tamano =
        {
            0.8f,
            1.4f,
            0.8f
        };

        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


void MinijuegoTronco::Reiniciar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    estado =
        TRONCO_PREPARANDO;

    rondasCompletadas =
        0;

    rondasObjetivo =
        10;

    jugadoresEnRonda =
        ContarJugadoresActivosTronco(
            jugadores,
            cantidadMaxima
        );

    tiempoPreparacion =
        DURACION_PREPARACION_TRONCO;

    tiempoPartida =
        DURACION_PARTIDA_TRONCO;

    tiempoRonda =
        DURACION_RONDA_TRONCO;

    tiempoPausaRonda =
        0.0f;

    rondaEnPausa =
        false;

    ultimaRondaCorrecta =
        false;

    PrepararNuevaRonda(
        jugadores,
        cantidadMaxima
    );
}


void MinijuegoTronco::PrepararNuevaRonda(
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    tiempoRonda =
        DURACION_RONDA_TRONCO;

    rondaEnPausa =
        false;

    tiempoPausaRonda =
        0.0f;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        estadosJugadores[i].respondio =
            false;

        estadosJugadores[i].acerto =
            false;

        estadosJugadores[i].animacionGolpe =
            0.0f;

        if (jugadores[i].activo)
        {
            estadosJugadores[i].accion =
                (AccionTronco)GetRandomValue(
                    ACCION_TRONCO_ARRIBA,
                    ACCION_TRONCO_DERECHA
                );
        }
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoTronco::Actualizar(
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    ModoTeclado modoTeclado
)
{
    int jugadoresActivos =
        ContarJugadoresActivosTronco(
            jugadores,
            cantidadMaxima
        );

    if (jugadoresActivos != jugadoresEnRonda)
    {
        Reiniciar(
            jugadores,
            cantidadMaxima
        );

        return;
    }

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (estadosJugadores[i].animacionGolpe > 0.0f)
        {
            estadosJugadores[i].animacionGolpe -=
                deltaTime;

            if (estadosJugadores[i].animacionGolpe < 0.0f)
            {
                estadosJugadores[i].animacionGolpe =
                    0.0f;
            }
        }
    }

    if (
        estado == TRONCO_GANADO ||
        estado == TRONCO_PERDIDO
    )
    {
        return;
    }

    if (estado == TRONCO_PREPARANDO)
    {
        tiempoPreparacion -=
            deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion =
                0.0f;

            estado =
                TRONCO_JUGANDO;
        }

        return;
    }

    tiempoPartida -=
        deltaTime;

    if (tiempoPartida <= 0.0f)
    {
        tiempoPartida =
            0.0f;

        estado =
            TRONCO_PERDIDO;

        return;
    }

    if (rondaEnPausa)
    {
        tiempoPausaRonda -=
            deltaTime;

        if (tiempoPausaRonda <= 0.0f)
        {
            PrepararNuevaRonda(
                jugadores,
                cantidadMaxima
            );
        }

        return;
    }

    tiempoRonda -=
        deltaTime;

    bool alguienFallo =
        false;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (
            !jugadores[i].activo ||
            estadosJugadores[i].respondio
        )
        {
            continue;
        }

        AccionTronco accionCorrecta =
            estadosJugadores[i].accion;

        if (
            AccionJugadorPresionada(
                i,
                jugadores[i],
                accionCorrecta,
                modoTeclado
            )
        )
        {
            estadosJugadores[i].respondio =
                true;

            estadosJugadores[i].acerto =
                true;

            estadosJugadores[i].animacionGolpe =
                0.28f;
        }
        else if (
            AlgunaAccionJugadorPresionada(
                i,
                jugadores[i],
                modoTeclado
            )
        )
        {
            estadosJugadores[i].respondio =
                true;

            estadosJugadores[i].acerto =
                false;

            alguienFallo =
                true;
        }
    }

    bool todosRespondieron =
        jugadoresActivos > 0;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (
            jugadores[i].activo &&
            !estadosJugadores[i].respondio
        )
        {
            todosRespondieron =
                false;
        }

        if (
            jugadores[i].activo &&
            estadosJugadores[i].respondio &&
            !estadosJugadores[i].acerto
        )
        {
            alguienFallo =
                true;
        }
    }

    if (
        alguienFallo ||
        tiempoRonda <= 0.0f
    )
    {
        ultimaRondaCorrecta =
            false;

        rondaEnPausa =
            true;

        tiempoPausaRonda =
            0.65f;

        return;
    }

    if (todosRespondieron)
    {
        rondasCompletadas++;

        ultimaRondaCorrecta =
            true;

        if (
            rondasCompletadas >=
            rondasObjetivo
        )
        {
            rondasCompletadas =
                rondasObjetivo;

            estado =
                TRONCO_GANADO;

            return;
        }

        rondaEnPausa =
            true;

        tiempoPausaRonda =
            0.38f;
    }
}


//==================================================
// DIBUJO 3D
//==================================================

static void DibujarEscenarioTronco(
    const MinijuegoTronco& minijuego,
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    BeginMode3D(
        minijuego.camara
    );

    DrawPlane(
        Vector3{ 0.0f, 0.0f, 0.0f },
        Vector2{ 16.0f, 11.0f },
        Color{ 94, 154, 76, 255 }
    );

    DrawCylinderEx(
        Vector3{ -2.8f, 1.0f, 0.0f },
        Vector3{ 2.8f, 1.0f, 0.0f },
        0.82f,
        0.82f,
        24,
        Color{ 125, 75, 38, 255 }
    );

    DrawCylinderEx(
        Vector3{ -2.83f, 1.0f, 0.0f },
        Vector3{ -2.79f, 1.0f, 0.0f },
        0.72f,
        0.72f,
        24,
        Color{ 205, 154, 88, 255 }
    );

    float porcentaje =
        minijuego.rondasObjetivo > 0
        ? (float)minijuego.rondasCompletadas /
            (float)minijuego.rondasObjetivo
        : 0.0f;

    float anchoCorte =
        0.08f +
        0.72f * porcentaje;

    DrawCube(
        Vector3{ 0.0f, 1.0f, 0.0f },
        anchoCorte,
        1.72f,
        1.72f,
        Color{ 78, 46, 27, 255 }
    );

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (!jugadores[i].activo)
        {
            continue;
        }

        Vector3 posicion =
            jugadores[i].posicion;

        if (
            minijuego.estadosJugadores[i]
                .animacionGolpe > 0.0f
        )
        {
            posicion.x +=
                posicion.x < 0.0f
                ? 0.25f
                : -0.25f;
        }

        DrawCube(
            posicion,
            jugadores[i].tamano.x,
            jugadores[i].tamano.y,
            jugadores[i].tamano.z,
            jugadores[i].color
        );

        DrawCubeWires(
            posicion,
            jugadores[i].tamano.x,
            jugadores[i].tamano.y,
            jugadores[i].tamano.z,
            BLACK
        );

        float direccion =
            posicion.x < 0.0f
            ? 1.0f
            : -1.0f;

        Vector3 mango =
        {
            posicion.x +
                direccion * 0.55f,
            posicion.y + 0.15f,
            posicion.z
        };

        DrawCube(
            mango,
            0.12f,
            0.95f,
            0.12f,
            BROWN
        );

        DrawCube(
            Vector3{
                mango.x,
                mango.y + 0.45f,
                mango.z
            },
            0.48f,
            0.28f,
            0.12f,
            LIGHTGRAY
        );
    }

    EndMode3D();
}


//==================================================
// DIBUJO 2D
//==================================================

void MinijuegoTronco::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    ModoTeclado modoTeclado
) const
{
    ClearBackground(
        Color{ 135, 196, 225, 255 }
    );

    DibujarEscenarioTronco(
        *this,
        jugadores,
        cantidadMaxima
    );

    DrawText(
        "MINIJUEGO COOPERATIVO - TRONCO COORDINADO",
        25,
        22,
        28,
        BLACK
    );

    DrawText(
        TextFormat(
            "TIEMPO: %.1f",
            tiempoPartida
        ),
        GetScreenWidth() - 190,
        25,
        24,
        tiempoPartida <= 8.0f
        ? RED
        : DARKBLUE
    );

    int anchoBarra =
        430;

    int xBarra =
        GetScreenWidth() / 2 -
        anchoBarra / 2;

    DrawRectangle(
        xBarra,
        67,
        anchoBarra,
        26,
        Fade(BLACK, 0.45f)
    );

    float progreso =
        rondasObjetivo > 0
        ? (float)rondasCompletadas /
            (float)rondasObjetivo
        : 0.0f;

    DrawRectangle(
        xBarra + 3,
        70,
        (int)((anchoBarra - 6) * progreso),
        20,
        ORANGE
    );

    DrawText(
        TextFormat(
            "CORTE %d / %d",
            rondasCompletadas,
            rondasObjetivo
        ),
        xBarra + 135,
        71,
        18,
        RAYWHITE
    );

    if (estado == TRONCO_PREPARANDO)
    {
        const char* texto =
            TextFormat(
                "PREPARADOS... %.0f",
                tiempoPreparacion + 1.0f
            );

        int tamano =
            42;

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, tamano) / 2,
            112,
            tamano,
            MAROON
        );
    }
    else if (estado == TRONCO_GANADO)
    {
        const char* texto =
            "TRONCO CORTADO - EQUIPO GANADOR";

        int tamano =
            34;

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, tamano) / 2,
            112,
            tamano,
            DARKGREEN
        );
    }
    else if (estado == TRONCO_PERDIDO)
    {
        const char* texto =
            "SE TERMINO EL TIEMPO";

        int tamano =
            38;

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, tamano) / 2,
            112,
            tamano,
            RED
        );
    }
    else if (rondaEnPausa)
    {
        const char* texto =
            ultimaRondaCorrecta
            ? "GOLPE COORDINADO"
            : "FALLO DE COORDINACION";

        int tamano =
            30;

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, tamano) / 2,
            112,
            tamano,
            ultimaRondaCorrecta
            ? DARKGREEN
            : RED
        );
    }
    else
    {
        DrawText(
            TextFormat(
                "VENTANA DE COORDINACION: %.1f",
                tiempoRonda
            ),
            GetScreenWidth() / 2 - 190,
            116,
            24,
            BLACK
        );
    }

    int cantidadActivos =
        ContarJugadoresActivosTronco(
            jugadores,
            cantidadMaxima
        );

    const int anchoTarjeta =
        224;

    const int altoTarjeta =
        126;

    const int separacion =
        14;

    int anchoTotal =
        cantidadActivos * anchoTarjeta +
        (cantidadActivos - 1) * separacion;

    int x =
        GetScreenWidth() / 2 -
        anchoTotal / 2;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (!jugadores[i].activo)
        {
            continue;
        }

        Color colorTarjeta =
            Fade(RAYWHITE, 0.92f);

        if (estadosJugadores[i].respondio)
        {
            colorTarjeta =
                estadosJugadores[i].acerto
                ? Fade(LIME, 0.88f)
                : Fade(RED, 0.88f);
        }

        DrawRectangle(
            x,
            158,
            anchoTarjeta,
            altoTarjeta,
            colorTarjeta
        );

        DrawRectangleLinesEx(
            Rectangle{
                (float)x,
                158.0f,
                (float)anchoTarjeta,
                (float)altoTarjeta
            },
            4.0f,
            jugadores[i].color
        );

        DrawText(
            TextFormat(
                "JUGADOR %d",
                jugadores[i].numero
            ),
            x + 12,
            168,
            20,
            BLACK
        );

        DrawText(
            TextoDispositivoTronco(
                i,
                jugadores[i],
                modoTeclado
            ),
            x + 12,
            194,
            15,
            DARKGRAY
        );

        const char* textoAccion =
            estadosJugadores[i].respondio
            ? (
                estadosJugadores[i].acerto
                ? "LISTO"
                : "ERROR"
            )
            : TextoAccionTronco(
                i,
                jugadores[i],
                estadosJugadores[i].accion,
                modoTeclado
            );

        int tamanoAccion =
            MeasureText(textoAccion, 38) <=
                anchoTarjeta - 20
            ? 38
            : 22;

        DrawText(
            textoAccion,
            x + anchoTarjeta / 2 -
                MeasureText(
                    textoAccion,
                    tamanoAccion
                ) / 2,
            225,
            tamanoAccion,
            estadosJugadores[i].respondio
            ? (
                estadosJugadores[i].acerto
                ? DARKGREEN
                : MAROON
            )
            : DARKBLUE
        );

        x +=
            anchoTarjeta +
            separacion;
    }
}
