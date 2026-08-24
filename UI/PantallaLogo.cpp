#include "UI/PantallaLogo.h"


//==================================================
// INICIALIZAR
//==================================================

void PantallaLogo::Inicializar(
    const char* rutaLogo
)
{
    tiempo = 0.0f;


    if (FileExists(rutaLogo))
    {
        logo =
            LoadTexture(
                rutaLogo
            );


    logoCargado =
        logo.id > 0;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void PantallaLogo::Actualizar(
    float deltaTime
)
{
    tiempo +=
        deltaTime;
}


//==================================================
// DIBUJAR
//==================================================

void PantallaLogo::Dibujar() const
{
    //------------------------------
    // FONDO
    //------------------------------

    ClearBackground(
        RAYWHITE
    );


    if (!logoCargado)
    {
        return;
    }


    //------------------------------
    // ALPHA
    //------------------------------

    float alpha = 1.0f;


    //------------------------------
    // FADE IN
    //------------------------------

    if (
        tiempo <
        DURACION_FADE_ENTRADA
    )
    {
        alpha =
            tiempo /
            DURACION_FADE_ENTRADA;
    }


    //------------------------------
    // FADE OUT
    //------------------------------

    else if (
        tiempo >
        INICIO_FADE_SALIDA
    )
    {
        alpha =
            1.0f -
            (
                tiempo -
                INICIO_FADE_SALIDA
            ) /
            (
                DURACION_TOTAL -
                INICIO_FADE_SALIDA
            );
    }


    //------------------------------
    // LIMITES
    //------------------------------

    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }


    if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }


    //------------------------------
    // TAMAÑO LOGO
    //------------------------------

    float maxAncho =
        GetScreenWidth() *
        0.40f;


    float maxAlto =
        GetScreenHeight() *
        0.40f;


    float escalaX =
        maxAncho /
        logo.width;


    float escalaY =
        maxAlto /
        logo.height;


    float escala =
        escalaX < escalaY
        ? escalaX
        : escalaY;


    float ancho =
        logo.width *
        escala;


    float alto =
        logo.height *
        escala;


    //------------------------------
    // CENTRAR
    //------------------------------

    Rectangle origen =
    {
        0,
        0,
        (float)logo.width,
        (float)logo.height
    };


    Rectangle destino =
    {
        (
            GetScreenWidth() -
            ancho
        ) / 2.0f,

        (
            GetScreenHeight() -
            alto
        ) / 2.0f,

        ancho,

        alto
    };


    DrawTexturePro(
        logo,
        origen,
        destino,
        Vector2{
            0,
            0
        },
        0,
        Fade(
            WHITE,
            alpha
        )
    );
}


//==================================================
// TERMINO
//==================================================

bool PantallaLogo::Termino() const
{
    return
        tiempo >=
        DURACION_TOTAL;
}


//==================================================
// DESCARGAR
//==================================================

void PantallaLogo::Descargar()
{
    if (logoCargado)
    {
        UnloadTexture(
            logo
        );
    }


    logoCargado = false;
}