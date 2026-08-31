#include "UI/MenuPrincipal.h"


//==================================================
// RECTANGULO BOTON
//==================================================

static Rectangle ObtenerRectBotonMenu(
    int indice
)
{
    float anchoPantalla =
        (float)GetScreenWidth();

    float altoPantalla =
        (float)GetScreenHeight();

    float anchoBoton =
        440.0f;

    if (
        anchoBoton >
        anchoPantalla * 0.65f
    )
    {
        anchoBoton =
            anchoPantalla * 0.65f;
    }

    float altoBoton =
        52.0f;

    float separacion =
        70.0f;

    float centroX =
        anchoPantalla / 2.0f;

    float inicioY =
        altoPantalla / 2.0f - 35.0f;

    return Rectangle
    {
        centroX - anchoBoton / 2.0f,
        inicioY + indice * separacion,
        anchoBoton,
        altoBoton
    };
}


//==================================================
// INICIAR PULSACION
//==================================================

static void IniciarPulsacion(
    MenuPrincipal& menu,
    int indice
)
{
    if (menu.accionPendiente != -1)
    {
        return;
    }

    menu.botonPresionado =
        indice;

    menu.accionPendiente =
        indice;

    menu.tiempoBotonPresionado =
        menu.DURACION_BOTON_PRESION;
}


//==================================================
// APLICAR ACCION
//==================================================

static void AplicarAccionPendiente(
    MenuPrincipal& menu
)
{
    switch (menu.accionPendiente)
    {
        case 0:
        {
            menu.empezarJuego =
                true;
            break;
        }

        case 1:
        {
            menu.abrirConfiguracion =
                true;
            break;
        }

        case 2:
        {
            menu.salir =
                true;
            break;
        }
    }

    menu.accionPendiente =
        -1;

    menu.botonPresionado =
        -1;

    menu.tiempoBotonPresionado =
        0.0f;
}


//==================================================
// INICIALIZAR
//==================================================

void MenuPrincipal::Inicializar()
{
    opcionSeleccionada =
        0;

    empezarJuego =
        false;

    abrirConfiguracion =
        false;

    salir =
        false;

    botonPresionado =
        -1;

    accionPendiente =
        -1;

    tiempoBotonPresionado =
        0.0f;

    if (!recursosCargados)
    {
        fondo.Cargar(
            "Assets/UI/MenuFondo.gif",
            15.0f
        );

        recursosCargados =
            true;
    }
}


//==================================================
// PREPARAR ENTRADA
//==================================================

void MenuPrincipal::PrepararEntrada(
    bool desdeLogo
)
{
    opcionSeleccionada =
        0;

    empezarJuego =
        false;

    abrirConfiguracion =
        false;

    salir =
        false;

    botonPresionado =
        -1;

    accionPendiente =
        -1;

    tiempoBotonPresionado =
        0.0f;

    tiempoEntrada =
        0.0f;

    entradaActiva =
        true;

    fadeBlancoActivo =
        desdeLogo;
}


//==================================================
// ACTUALIZAR
//==================================================

void MenuPrincipal::Actualizar(
    float deltaTime
)
{
    //------------------------------
    // GIF
    //------------------------------

    fondo.Actualizar(
        deltaTime
    );

    //------------------------------
    // TRANSICION
    //------------------------------

    if (entradaActiva)
    {
        tiempoEntrada +=
            deltaTime;

        if (
            tiempoEntrada >=
            RETRASO_MENU +
            DURACION_FADE_MENU
        )
        {
            entradaActiva =
                false;
        }
    }

    //------------------------------
    // BLOQUEAR INPUT DURANTE FADE
    //------------------------------

    if (entradaActiva)
    {
        return;
    }

    //------------------------------
    // BOTON PRESIONADO
    //------------------------------

    if (accionPendiente != -1)
    {
        tiempoBotonPresionado -=
            deltaTime;

        if (
            tiempoBotonPresionado <=
            0.0f
        )
        {
            AplicarAccionPendiente(
                *this
            );
        }

        return;
    }

    //==================================================
    // TECLADO
    //==================================================

    if (IsKeyPressed(KEY_DOWN))
    {
        opcionSeleccionada++;

        if (
            opcionSeleccionada > 2
        )
        {
            opcionSeleccionada =
                0;
        }
    }

    if (IsKeyPressed(KEY_UP))
    {
        opcionSeleccionada--;

        if (
            opcionSeleccionada < 0
        )
        {
            opcionSeleccionada =
                2;
        }
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        IniciarPulsacion(
            *this,
            opcionSeleccionada
        );
    }

    //==================================================
    // MOUSE
    //==================================================

    Vector2 mouse =
        GetMousePosition();

    for (
        int i = 0;
        i < 3;
        i++
    )
    {
        Rectangle boton =
            ObtenerRectBotonMenu(i);

        if (
            CheckCollisionPointRec(
                mouse,
                boton
            )
        )
        {
            opcionSeleccionada =
                i;

            if (
                IsMouseButtonPressed(
                    MOUSE_BUTTON_LEFT
                )
            )
            {
                IniciarPulsacion(
                    *this,
                    i
                );
            }
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void MenuPrincipal::Dibujar()
{
    //==================================================
    // FONDO
    //==================================================

    fondo
        .DibujarPantallaCompleta();

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(
            BLACK,
            0.30f
        )
    );

    //==================================================
    // FADE BLANCO SOLO DESDE LOGO
    //==================================================

    if (fadeBlancoActivo)
    {
        float progresoVideo =
            tiempoEntrada /
            DURACION_FADE_VIDEO;

        if (progresoVideo < 0.0f)
        {
            progresoVideo =
                0.0f;
        }

        if (progresoVideo > 1.0f)
        {
            progresoVideo =
                1.0f;
        }

        float alphaBlanco =
            1.0f -
            progresoVideo;

        if (alphaBlanco > 0.0f)
        {
            DrawRectangle(
                0,
                0,
                GetScreenWidth(),
                GetScreenHeight(),
                Fade(
                    WHITE,
                    alphaBlanco
                )
            );
        }
    }

    //==================================================
    // FADE UI
    //==================================================

    float progresoMenu =
        (
            tiempoEntrada -
            RETRASO_MENU
        ) /
        DURACION_FADE_MENU;

    if (progresoMenu < 0.0f)
    {
        progresoMenu =
            0.0f;
    }

    if (progresoMenu > 1.0f)
    {
        progresoMenu =
            1.0f;
    }

    if (!entradaActiva)
    {
        progresoMenu =
            1.0f;
    }

    Color blancoMenu =
        Fade(
            RAYWHITE,
            progresoMenu
        );

    Color naranja =
        Fade(
            ORANGE,
            progresoMenu
        );

    Color sombra =
        Fade(
            BLACK,
            progresoMenu * 0.8f
        );

    //==================================================
    // TITULO
    //==================================================

    const char* titulo =
        "PARTY GAME";

    int tamanoTitulo =
        64;

    int anchoTitulo =
        MeasureText(
            titulo,
            tamanoTitulo
        );

    int tituloX =
        GetScreenWidth() / 2 -
        anchoTitulo / 2;

    int tituloY =
        GetScreenHeight() / 2 -
        230;

    DrawText(
        titulo,
        tituloX + 5,
        tituloY + 5,
        tamanoTitulo,
        sombra
    );

    DrawText(
        titulo,
        tituloX,
        tituloY,
        tamanoTitulo,
        naranja
    );

    //==================================================
    // OPCIONES
    //==================================================

    const char* opciones[] =
    {
        "EMPEZAR JUEGO",
        "CONFIGURACION",
        "SALIR"
    };

    const int TAMANO_TEXTO =
        30;

    for (
        int i = 0;
        i < 3;
        i++
    )
    {
        Rectangle boton =
            ObtenerRectBotonMenu(i);

        int anchoTexto =
            MeasureText(
                opciones[i],
                TAMANO_TEXTO
            );

        int textoX =
            (int)(
                boton.x +
                boton.width / 2.0f -
                anchoTexto / 2.0f
            );

        int textoY =
            (int)(
                boton.y +
                boton.height / 2.0f -
                TAMANO_TEXTO / 2.0f
            );

        bool seleccionada =
            (i == opcionSeleccionada);

        bool presionada =
            (
                i == botonPresionado &&
                tiempoBotonPresionado > 0.0f
            );

        if (seleccionada)
        {
            if (presionada)
            {
                DrawRectangle(
                    (int)boton.x,
                    (int)boton.y,
                    (int)boton.width,
                    (int)boton.height,
                    naranja
                );
            }

            DrawRectangleLines(
                (int)boton.x,
                (int)boton.y,
                (int)boton.width,
                (int)boton.height,
                naranja
            );

            DrawText(
                ">",
                (int)boton.x - 45,
                textoY,
                TAMANO_TEXTO,
                naranja
            );

            int anchoCursor =
                MeasureText(
                    "<",
                    TAMANO_TEXTO
                );

            DrawText(
                "<",
                (int)(
                    boton.x +
                    boton.width +
                    45 -
                    anchoCursor
                ),
                textoY,
                TAMANO_TEXTO,
                naranja
            );

            DrawText(
                opciones[i],
                textoX,
                textoY,
                TAMANO_TEXTO,
                presionada
                ? BLACK
                : naranja
            );
        }
        else
        {
            DrawText(
                opciones[i],
                textoX,
                textoY,
                TAMANO_TEXTO,
                blancoMenu
            );
        }
    }

    //==================================================
    // SCANLINES
    //==================================================

    for (
        int y = 0;
        y < GetScreenHeight();
        y += 6
    )
    {
        DrawLine(
            0,
            y,
            GetScreenWidth(),
            y,
            Fade(
                BLACK,
                0.06f *
                progresoMenu
            )
        );
    }

    //==================================================
    // AYUDA
    //==================================================

    const char* ayuda =
        "FLECHAS / MOUSE: MOVER   ENTER / CLICK: SELECCIONAR";

    int anchoAyuda =
        MeasureText(
            ayuda,
            18
        );

    DrawText(
        ayuda,
        GetScreenWidth() / 2 -
            anchoAyuda / 2,
        GetScreenHeight() - 55,
        18,
        Fade(
            LIGHTGRAY,
            progresoMenu
        )
    );
}


//==================================================
// DESCARGAR
//==================================================

void MenuPrincipal::Descargar()
{
    fondo.Descargar();
}