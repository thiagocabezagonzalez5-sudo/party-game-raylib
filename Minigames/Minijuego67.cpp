#include "Minigames/Minijuego67.h"

#include "Minigames/UtilidadesMinijuegos.h"


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PARTIDA_67 =
    30.0f;

static const float INICIO_ZONA_RECOGIDA_67 =
    0.68f;

static const float FIN_ZONA_RECOGIDA_67 =
    0.88f;


//==================================================
// UTILIDADES
//==================================================

static int ContarJugadoresActivos67(
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


static const char* TextoControlAccion67(
    int indiceJugador,
    const JugadorPrueba& jugador,
    ModoTeclado modoTeclado
)
{
    if (jugador.usaGamepad)
    {
        return "A";
    }

    if (
        modoTeclado == TECLADO_DIVIDIDO &&
        indiceJugador == 1
    )
    {
        return "ENTER";
    }

    return "ESPACIO";
}


static void ReiniciarObjetoCinta67(
    EstadoJugador67& estado,
    float demora
)
{
    estado.objetoActivo =
        false;

    estado.tiempoReaparicion =
        demora;

    estado.progresoObjeto =
        -0.06f;
}


static void ColocarPieza67(
    Minijuego67& minijuego,
    int indiceJugador
)
{
    EstadoJugador67& estadoJugador =
        minijuego.estadosJugadores[indiceJugador];

    int indiceMesa =
        indiceJugador /
        2;

    if (indiceMesa < 0 || indiceMesa >= 2)
    {
        return;
    }

    if (
        estadoJugador.tipoPieza ==
        PIEZA_NUMERO_6
    )
    {
        if (
            minijuego.mesas[indiceMesa] ==
            MESA_67_VACIA
        )
        {
            minijuego.mesas[indiceMesa] =
                MESA_67_CON_6;
        }
        else
        {
            // Dos numeros 6 juntos arruinan
            // la combinacion, como dos bases.
            minijuego.mesas[indiceMesa] =
                MESA_67_VACIA;

            estadoJugador.tiempoStun =
                0.55f;
        }
    }
    else
    {
        if (
            minijuego.mesas[indiceMesa] ==
            MESA_67_CON_6
        )
        {
            minijuego.mesas[indiceMesa] =
                MESA_67_COMPLETA;

            minijuego.tiempoMesaCompleta[
                indiceMesa
            ] = 0.55f;

            minijuego.puntos++;
        }
        else
        {
            // El 7 no puede colocarse antes del 6.
            estadoJugador.tiempoStun =
                0.55f;
        }
    }

    estadoJugador.llevaPieza =
        false;

    estadoJugador.mirandoMesa =
        false;

    estadoJugador.tiempoGiro =
        0.0f;
}


//==================================================
// INICIALIZAR / REINICIAR
//==================================================

void Minijuego67::Inicializar()
{
    tiempoPartida =
        DURACION_PARTIDA_67;

    velocidadCintas =
        0.24f;

    puntos =
        0;

    jugadoresEnPartida =
        0;

    terminado =
        false;
}


void Minijuego67::ConfigurarJugadores(
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
    }
}


void Minijuego67::Reiniciar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    tiempoPartida =
        DURACION_PARTIDA_67;

    velocidadCintas =
        0.24f;

    puntos =
        0;

    terminado =
        false;

    jugadoresEnPartida =
        ContarJugadoresActivos67(
            jugadores,
            cantidadMaxima
        );

    for (
        int i = 0;
        i < 2;
        i++
    )
    {
        mesas[i] =
            MESA_67_VACIA;

        tiempoMesaCompleta[i] =
            0.0f;
    }

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        EstadoJugador67& estado =
            estadosJugadores[i];

        estado.tipoPieza =
            i % 2 == 0
            ? PIEZA_NUMERO_6
            : PIEZA_NUMERO_7;

        estado.progresoObjeto =
            -0.06f +
            0.08f * (float)i;

        estado.objetoActivo =
            true;

        estado.tiempoReaparicion =
            0.0f;

        estado.llevaPieza =
            false;

        estado.mirandoMesa =
            false;

        estado.tiempoGiro =
            0.0f;

        estado.tiempoStun =
            0.0f;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void Minijuego67::Actualizar(
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    ModoTeclado modoTeclado
)
{
    int cantidadActivos =
        ContarJugadoresActivos67(
            jugadores,
            cantidadMaxima
        );

    if (cantidadActivos != jugadoresEnPartida)
    {
        Reiniciar(
            jugadores,
            cantidadMaxima
        );

        return;
    }

    if (
        terminado ||
        cantidadActivos < 2
    )
    {
        return;
    }

    tiempoPartida -=
        deltaTime;

    if (tiempoPartida <= 0.0f)
    {
        tiempoPartida =
            0.0f;

        terminado =
            true;

        return;
    }

    float porcentajeTiempo =
        1.0f -
        tiempoPartida /
        DURACION_PARTIDA_67;

    velocidadCintas =
        0.24f +
        porcentajeTiempo *
        0.18f;

    for (
        int i = 0;
        i < 2;
        i++
    )
    {
        if (tiempoMesaCompleta[i] > 0.0f)
        {
            tiempoMesaCompleta[i] -=
                deltaTime;

            if (tiempoMesaCompleta[i] <= 0.0f)
            {
                tiempoMesaCompleta[i] =
                    0.0f;

                mesas[i] =
                    MESA_67_VACIA;
            }
        }
    }

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

        EstadoJugador67& estado =
            estadosJugadores[i];

        if (estado.tiempoStun > 0.0f)
        {
            estado.tiempoStun -=
                deltaTime;

            if (estado.tiempoStun < 0.0f)
            {
                estado.tiempoStun =
                    0.0f;
            }

            continue;
        }

        if (
            estado.llevaPieza &&
            estado.tiempoGiro > 0.0f
        )
        {
            estado.tiempoGiro -=
                deltaTime;

            if (estado.tiempoGiro <= 0.0f)
            {
                estado.tiempoGiro =
                    0.0f;

                estado.mirandoMesa =
                    true;
            }
        }

        if (estado.objetoActivo)
        {
            estado.progresoObjeto +=
                velocidadCintas *
                deltaTime;

            if (estado.progresoObjeto > 1.06f)
            {
                ReiniciarObjetoCinta67(
                    estado,
                    0.28f
                );
            }
        }
        else
        {
            estado.tiempoReaparicion -=
                deltaTime;

            if (estado.tiempoReaparicion <= 0.0f)
            {
                estado.objetoActivo =
                    true;

                estado.progresoObjeto =
                    -0.06f;

                estado.tiempoReaparicion =
                    0.0f;
            }
        }

        EntradaJugadorPrueba entrada =
            LeerEntradaJugadorPrueba(
                i,
                jugadores[i],
                modoTeclado
            );

        if (!entrada.saltar)
        {
            continue;
        }

        if (estado.llevaPieza)
        {
            if (estado.mirandoMesa)
            {
                ColocarPieza67(
                    *this,
                    i
                );
            }

            continue;
        }

        bool objetoEnFrente =
            estado.objetoActivo &&
            estado.progresoObjeto >=
                INICIO_ZONA_RECOGIDA_67 &&
            estado.progresoObjeto <=
                FIN_ZONA_RECOGIDA_67;

        if (objetoEnFrente)
        {
            estado.llevaPieza =
                true;

            estado.mirandoMesa =
                false;

            estado.tiempoGiro =
                0.32f;

            ReiniciarObjetoCinta67(
                estado,
                0.58f
            );
        }
        else
        {
            estado.tiempoStun =
                0.48f;
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void Minijuego67::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    ModoTeclado modoTeclado
) const
{
    ClearBackground(
        Color{ 238, 220, 184, 255 }
    );

    DrawText(
        "FABRICA 67 - COOPERATIVO",
        28,
        20,
        30,
        Color{ 66, 42, 28, 255 }
    );

    DrawText(
        TextFormat(
            "PUNTOS: %d",
            puntos
        ),
        GetScreenWidth() / 2 - 75,
        23,
        28,
        DARKGREEN
    );

    DrawText(
        TextFormat(
            "TIEMPO: %.1f",
            tiempoPartida
        ),
        GetScreenWidth() - 190,
        24,
        24,
        tiempoPartida <= 7.0f
        ? RED
        : DARKBLUE
    );

    DrawText(
        "AGARRA LA PIEZA EN LA ZONA AMARILLA, ESPERA EL GIRO Y VUELVE A PRESIONAR",
        28,
        63,
        18,
        DARKGRAY
    );

    int cantidadActivos =
        ContarJugadoresActivos67(
            jugadores,
            cantidadMaxima
        );

    if (cantidadActivos < 2)
    {
        const char* texto =
            "SE NECESITAN AL MENOS 2 JUGADORES";

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 30) / 2,
            95,
            30,
            MAROON
        );
    }

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

        const EstadoJugador67& estado =
            estadosJugadores[i];

        int y =
            112 +
            i * 96;

        DrawRectangle(
            36,
            y,
            530,
            76,
            Color{ 80, 84, 92, 255 }
        );

        DrawRectangle(
            50,
            y + 25,
            494,
            28,
            Color{ 150, 155, 165, 255 }
        );

        int xInicioZona =
            50 +
            (int)(
                494.0f *
                INICIO_ZONA_RECOGIDA_67
            );

        int anchoZona =
            (int)(
                494.0f *
                (
                    FIN_ZONA_RECOGIDA_67 -
                    INICIO_ZONA_RECOGIDA_67
                )
            );

        DrawRectangle(
            xInicioZona,
            y + 21,
            anchoZona,
            36,
            Fade(YELLOW, 0.65f)
        );

        if (estado.objetoActivo)
        {
            int xObjeto =
                50 +
                (int)(
                    estado.progresoObjeto *
                    494.0f
                );

            Color colorPieza =
                estado.tipoPieza ==
                    PIEZA_NUMERO_6
                ? ORANGE
                : SKYBLUE;

            DrawCircle(
                xObjeto,
                y + 39,
                21.0f,
                colorPieza
            );

            const char* numero =
                estado.tipoPieza ==
                    PIEZA_NUMERO_6
                ? "6"
                : "7";

            DrawText(
                numero,
                xObjeto - 8,
                y + 22,
                32,
                BLACK
            );
        }

        DrawRectangle(
            590,
            y,
            258,
            76,
            Fade(
                jugadores[i].color,
                0.28f
            )
        );

        DrawRectangleLinesEx(
            Rectangle{
                590.0f,
                (float)y,
                258.0f,
                76.0f
            },
            3.0f,
            jugadores[i].color
        );

        DrawText(
            TextFormat(
                "J%d - PIEZA %d",
                jugadores[i].numero,
                estado.tipoPieza ==
                    PIEZA_NUMERO_6
                ? 6
                : 7
            ),
            602,
            y + 8,
            20,
            BLACK
        );

        const char* estadoTexto =
            estado.tiempoStun > 0.0f
            ? "STUN"
            : (
                estado.llevaPieza
                ? (
                    estado.mirandoMesa
                    ? "LISTO PARA DEJAR"
                    : "GIRANDO..."
                )
                : "ESPERANDO PIEZA"
            );

        DrawText(
            estadoTexto,
            602,
            y + 34,
            18,
            estado.tiempoStun > 0.0f
            ? RED
            : DARKBLUE
        );

        DrawText(
            TextFormat(
                "ACCION: %s",
                TextoControlAccion67(
                    i,
                    jugadores[i],
                    modoTeclado
                )
            ),
            602,
            y + 56,
            14,
            DARKGRAY
        );
    }

    for (
        int pareja = 0;
        pareja < 2;
        pareja++
    )
    {
        int primerJugador =
            pareja * 2;

        if (
            primerJugador >= cantidadMaxima ||
            !jugadores[primerJugador].activo
        )
        {
            continue;
        }

        int yMesa =
            145 +
            pareja * 192;

        DrawRectangle(
            900,
            yMesa,
            300,
            112,
            Color{ 115, 72, 42, 255 }
        );

        DrawRectangleLines(
            900,
            yMesa,
            300,
            112,
            BLACK
        );

        DrawText(
            TextFormat(
                "MESA PAREJA %d",
                pareja + 1
            ),
            922,
            yMesa + 10,
            20,
            RAYWHITE
        );

        const char* contenido =
            mesas[pareja] == MESA_67_VACIA
            ? "VACIA"
            : (
                mesas[pareja] == MESA_67_CON_6
                ? "6 _"
                : "67 +1"
            );

        int tamanoContenido =
            38;

        DrawText(
            contenido,
            1050 -
                MeasureText(
                    contenido,
                    tamanoContenido
                ) /
                2,
            yMesa + 48,
            tamanoContenido,
            mesas[pareja] == MESA_67_COMPLETA
            ? LIME
            : YELLOW
        );
    }

    if (terminado)
    {
        DrawRectangle(
            330,
            245,
            620,
            120,
            Fade(BLACK, 0.88f)
        );

        const char* texto =
            TextFormat(
                "TIEMPO - PUNTAJE FINAL: %d",
                puntos
            );

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 32) / 2,
            286,
            32,
            RAYWHITE
        );
    }
}
