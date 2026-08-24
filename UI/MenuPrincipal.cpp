#include "UI/MenuPrincipal.h"


//==================================================
// INICIALIZAR
//==================================================

void MenuPrincipal::Inicializar()
{
    //------------------------------
    // ESTADO DEL MENU
    //------------------------------

    opcionSeleccionada = 0;

    empezarJuego = false;

    abrirConfiguracion = false;

    salir = false;


    //------------------------------
    // CARGAR RECURSOS UNA SOLA VEZ
    //------------------------------

    if (!recursosCargados)
    {
        fondo.Cargar(
            "Assets/UI/MenuFondo.gif",
            15.0f
        );

        recursosCargados = true;
    }
}


//==================================================
// PREPARAR TRANSICION
//==================================================

void MenuPrincipal::PrepararEntrada(
    bool usarFade
)
{
    //------------------------------
    // RESET OPCIONES
    //------------------------------

    opcionSeleccionada = 0;

    empezarJuego = false;

    abrirConfiguracion = false;

    salir = false;


    //------------------------------
    // CON TRANSICION
    //------------------------------

    if (usarFade)
    {
        tiempoEntrada = 0.0f;

        entradaActiva = true;
    }


    //------------------------------
    // SIN TRANSICION
    //------------------------------

    else
    {
        tiempoEntrada =
            RETRASO_MENU +
            DURACION_FADE_MENU;

        entradaActiva = false;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MenuPrincipal::Actualizar(
    float deltaTime
)
{
    //------------------------------
    // VIDEO
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
            entradaActiva = false;
        }
    }


    /*
        Mientras el menu todavia
        esta apareciendo no aceptamos
        input.
    */

    if (entradaActiva)
    {
        return;
    }


    //==================================================
    // NAVEGACION
    //==================================================

    if (IsKeyPressed(KEY_DOWN))
    {
        opcionSeleccionada++;


        if (
            opcionSeleccionada >
            2
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
            opcionSeleccionada <
            0
        )
        {
            opcionSeleccionada =
                2;
        }
    }


    //==================================================
    // ENTER
    //==================================================

    if (IsKeyPressed(KEY_ENTER))
    {
        switch (
            opcionSeleccionada
        )
        {
            case 0:
            {
                empezarJuego =
                    true;

                break;
            }


            case 1:
            {
                abrirConfiguracion =
                    true;

                break;
            }


            case 2:
            {
                salir =
                    true;

                break;
            }
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void MenuPrincipal::Dibujar()
{
    //------------------------------
    // FONDO ANIMADO
    //------------------------------

    fondo
        .DibujarPantallaCompleta();


    //------------------------------
    // OSCURECER FONDO
    //------------------------------

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
    // FADE VIDEO DESDE BLANCO
    //==================================================

    float progresoVideo =
        tiempoEntrada /
        DURACION_FADE_VIDEO;


    if (progresoVideo < 0.0f)
        progresoVideo = 0.0f;


    if (progresoVideo > 1.0f)
        progresoVideo = 1.0f;


    float alphaBlanco =
        1.0f -
        progresoVideo;


    //------------------------------
    // FONDO BLANCO QUE DESAPARECE
    //------------------------------

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


    //==================================================
    // FADE DEL MENU
    //==================================================

    float progresoMenu =
        (
            tiempoEntrada -
            RETRASO_MENU
        ) /
        DURACION_FADE_MENU;


    if (progresoMenu < 0.0f)
        progresoMenu = 0.0f;


    if (progresoMenu > 1.0f)
        progresoMenu = 1.0f;


    /*
        Cuando ya termino la animacion,
        el menu siempre queda en 1.
    */

    if (!entradaActiva)
    {
        progresoMenu = 1.0f;
    }


    Color blancoMenu =
        Fade(
            RAYWHITE,
            progresoMenu
        );


    Color seleccionado =
        Fade(
            GOLD,
            progresoMenu
        );


    Color sombra =
        Fade(
            BLACK,
            progresoMenu *
            0.8f
        );


    //==================================================
    // POSICIONES CENTRADAS
    //==================================================

    int centroX =
        GetScreenWidth() /
        2;


    int centroY =
        GetScreenHeight() /
        2;


    //==================================================
    // TITULO RETRO
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
        centroX -
        anchoTitulo /
        2;


    int tituloY =
        centroY -
        220;


    //------------------------------
    // SOMBRA TITULO
    //------------------------------

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
        seleccionado
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


    const int CANTIDAD =
        3;


    const int TAMANO_TEXTO =
        30;


    const int SEPARACION =
        65;


    int inicioY =
        centroY -
        40;


    for (
        int i = 0;
        i < CANTIDAD;
        i++
    )
    {
        int anchoTexto =
            MeasureText(
                opciones[i],
                TAMANO_TEXTO
            );


        int x =
            centroX -
            anchoTexto /
            2;


        int y =
            inicioY +
            i *
            SEPARACION;


        //------------------------------
        // SELECCION
        //------------------------------

        if (i == opcionSeleccionada)
        {
            int margenX = 30;
            int margenY = 10;

            int distanciaCursor = 80;

            DrawRectangleLines(
                x - margenX,
                y - margenY,
                anchoTexto + margenX * 2,
                TAMANO_TEXTO + margenY * 2,
                seleccionado
            );

            DrawText(
                ">",
                x - distanciaCursor,
                y,
                TAMANO_TEXTO,
                seleccionado
            );

            DrawText(
                "<",
                x + anchoTexto + distanciaCursor - MeasureText("<", TAMANO_TEXTO),
                y,
                TAMANO_TEXTO,
                seleccionado
            );

            DrawText(
                opciones[i],
                x,
                y,
                TAMANO_TEXTO,
                seleccionado
            );
        }
        else
        {
            DrawText(
                opciones[i],
                x,
                y,
                TAMANO_TEXTO,
                blancoMenu
            );
        }
    }


    //==================================================
    // LINEAS RETRO / SCANLINES
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
        "FLECHAS: MOVER   ENTER: SELECCIONAR";


    int anchoAyuda =
        MeasureText(
            ayuda,
            18
        );


    DrawText(
        ayuda,

        centroX -
        anchoAyuda /
        2,

        GetScreenHeight() -
        55,

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