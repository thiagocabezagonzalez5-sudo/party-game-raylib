#include "UI/MenuPausa.h"

void ActualizarMenuPausa(
    bool& menuAbierto,
    int& opcionSeleccionada,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int& indiceResolucion,
    ModoVentana& modoVentana,
    bool& mostrarFPS,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    int& indiceFPS,
    Jugador& jugador
)
{
    if (IsKeyPressed(KEY_UP))
    {
        opcionSeleccionada--;

        if (opcionSeleccionada < 0)
        {
            opcionSeleccionada = CANTIDAD_OPCIONES_PAUSA - 1;
        }
    }

    if (IsKeyPressed(KEY_DOWN))
    {
        opcionSeleccionada++;

        if (opcionSeleccionada >= CANTIDAD_OPCIONES_PAUSA)
        {
            opcionSeleccionada = 0;
        }
    }

    int direccion = 0;

    if (IsKeyPressed(KEY_LEFT))
    {
        direccion = -1;
    }

    if (IsKeyPressed(KEY_RIGHT))
    {
        direccion = 1;
    }

    bool enter = IsKeyPressed(KEY_ENTER);

    if (opcionSeleccionada == OPCION_RESOLUCION)
    {
        if (enter && direccion == 0)
        {
            direccion = 1;
        }

        if (direccion != 0)
        {
            indiceResolucion += direccion;

            if (indiceResolucion < 0)
            {
                indiceResolucion = cantidadResoluciones - 1;
            }

            if (indiceResolucion >= cantidadResoluciones)
            {
                indiceResolucion = 0;
            }

            AplicarModoVentana(
                modoVentana,
                modoVentana,
                resoluciones[indiceResolucion]
            );
        }
    }
    else if (opcionSeleccionada == OPCION_MODO_VENTANA)
    {
        if (enter && direccion == 0)
        {
            direccion = 1;
        }

        if (direccion != 0)
        {
            int nuevoModo = (int)modoVentana + direccion;

            if (nuevoModo < MODO_VENTANA)
            {
                nuevoModo = MODO_SIN_BORDES;
            }

            if (nuevoModo > MODO_SIN_BORDES)
            {
                nuevoModo = MODO_VENTANA;
            }

            AplicarModoVentana(
                modoVentana,
                (ModoVentana)nuevoModo,
                resoluciones[indiceResolucion]
            );
        }
    }
    else if (opcionSeleccionada == OPCION_MOSTRAR_FPS)
    {
        if (direccion != 0 || enter)
        {
            mostrarFPS = !mostrarFPS;
        }
    }
    else if (opcionSeleccionada == OPCION_LIMITE_FPS)
    {
        if (enter && direccion == 0)
        {
            direccion = 1;
        }

        if (direccion != 0)
        {
            indiceFPS += direccion;

            if (indiceFPS < 0)
            {
                indiceFPS = cantidadOpcionesFPS - 1;
            }

            if (indiceFPS >= cantidadOpcionesFPS)
            {
                indiceFPS = 0;
            }

            SetTargetFPS(opcionesFPS[indiceFPS]);
        }
    }
    else if (opcionSeleccionada == OPCION_REINICIAR && enter)
    {
        jugador.Reiniciar();
    }
    else if (opcionSeleccionada == OPCION_VOLVER && enter)
    {
        menuAbierto = false;
    }
}

void DibujarMenuPausa(
    int opcionSeleccionada,
    Resolucion resoluciones[],
    int indiceResolucion,
    ModoVentana modoVentana,
    bool mostrarFPS,
    int opcionesFPS[],
    int indiceFPS
)
{
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    DrawRectangle(
        0,
        0,
        anchoPantalla,
        altoPantalla,
        Fade(BLACK, 0.65f)
    );

    int anchoModal = 700;
    int altoModal = 520;

    if (anchoModal > anchoPantalla - 40)
    {
        anchoModal = anchoPantalla - 40;
    }

    if (altoModal > altoPantalla - 40)
    {
        altoModal = altoPantalla - 40;
    }

    int modalX = (anchoPantalla - anchoModal) / 2;
    int modalY = (altoPantalla - altoModal) / 2;

    DrawRectangle(modalX, modalY, anchoModal, altoModal, RAYWHITE);
    DrawRectangleLines(modalX, modalY, anchoModal, altoModal, DARKGRAY);

    DrawText(
        "PAUSA / CONFIGURACION",
        modalX + 30,
        modalY + 25,
        30,
        BLACK
    );

    const int INICIO_Y = 90;
    const int ALTO_FILA = 55;

    for (int i = 0; i < CANTIDAD_OPCIONES_PAUSA; i++)
    {
        int filaY = modalY + INICIO_Y + i * ALTO_FILA;

        if (i == opcionSeleccionada)
        {
            DrawRectangle(
                modalX + 20,
                filaY - 8,
                anchoModal - 40,
                42,
                Fade(SKYBLUE, 0.35f)
            );
        }

        const char* nombre = "";
        const char* valor = "";

        switch (i)
        {
            case OPCION_RESOLUCION:
                nombre = "Resolucion";
                valor = TextFormat(
                    "%d x %d",
                    resoluciones[indiceResolucion].ancho,
                    resoluciones[indiceResolucion].alto
                );
                break;

            case OPCION_MODO_VENTANA:
                nombre = "Modo";
                valor = NombreModoVentana(modoVentana);
                break;

            case OPCION_MOSTRAR_FPS:
                nombre = "Mostrar FPS";
                valor = mostrarFPS ? "SI" : "NO";
                break;

            case OPCION_LIMITE_FPS:
                nombre = "Limite de FPS";
                valor = TextFormat("%d", opcionesFPS[indiceFPS]);
                break;

            case OPCION_REINICIAR:
                nombre = "Reiniciar jugador";
                valor = "ENTER";
                break;

            case OPCION_VOLVER:
                nombre = "Volver al juego";
                valor = "ENTER";
                break;
        }

        DrawText(nombre, modalX + 40, filaY, 22, BLACK);

        int anchoValor = MeasureText(valor, 22);

        DrawText(
            valor,
            modalX + anchoModal - 40 - anchoValor,
            filaY,
            22,
            DARKGRAY
        );
    }

    DrawText(
        "ARRIBA/ABAJO: navegar   IZQ/DER: cambiar",
        modalX + 30,
        modalY + altoModal - 65,
        17,
        GRAY
    );

    DrawText(
        "ENTER: activar   TAB: volver",
        modalX + 30,
        modalY + altoModal - 38,
        17,
        GRAY
    );
}