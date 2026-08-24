#include "Systems/FondoAnimado.h"


//==================================================
// CARGAR
//==================================================

void FondoAnimado::Cargar(
    const char* ruta,
    float fps
)
{
    //------------------------------
    // YA ESTA CARGADO
    //------------------------------

    if (cargado)
    {
        return;
    }

    fpsAnimacion = fps;


    //------------------------------
    // COMPROBAR ARCHIVO
    //------------------------------

    if (!FileExists(ruta))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro el fondo animado: %s",
            ruta
        );

        cargado = false;

        return;
    }


    //------------------------------
    // CARGAR GIF
    //------------------------------

    imagen =
        LoadImageAnim(
            ruta,
            &cantidadFrames
        );


    if (imagen.data == nullptr)
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar el GIF"
        );

        cargado = false;

        return;
    }


    //------------------------------
    // CREAR TEXTURA
    //------------------------------

    textura =
        LoadTextureFromImage(
            imagen
        );

    SetTextureFilter(
        textura,
        TEXTURE_FILTER_POINT
    );

    cargado =
        textura.id > 0;


    frameActual = 0;

    tiempoFrame = 0.0f;
}


//==================================================
// ACTUALIZAR
//==================================================

void FondoAnimado::Actualizar(
    float deltaTime
)
{
    //------------------------------
    // SIN ANIMACION
    //------------------------------

    if (
        !cargado ||
        cantidadFrames <= 1
    )
    {
        return;
    }


    //------------------------------
    // TIEMPO
    //------------------------------

    tiempoFrame +=
        deltaTime;


    float duracionFrame =
        1.0f /
        fpsAnimacion;


    //------------------------------
    // TODAVIA NO TOCA CAMBIAR
    //------------------------------

    if (
        tiempoFrame <
        duracionFrame
    )
    {
        return;
    }


    //------------------------------
    // CUANTOS FRAMES PASARON
    //------------------------------

    int framesAvanzar =
        (int)(
            tiempoFrame /
            duracionFrame
        );


    //------------------------------
    // CONSERVAR RESTO DEL TIEMPO
    //------------------------------

    tiempoFrame -=
        framesAvanzar *
        duracionFrame;


    //------------------------------
    // AVANZAR
    //------------------------------

    frameActual =
        (
            frameActual +
            framesAvanzar
        ) %
        cantidadFrames;


    //------------------------------
    // UBICACION DEL FRAME
    //------------------------------

    int bytesPorFrame =
        imagen.width *
        imagen.height *
        4;


    unsigned char* datosFrame =
        (unsigned char*)imagen.data +
        frameActual *
        bytesPorFrame;


    //------------------------------
    // SOLO UNA SUBIDA A GPU
    //------------------------------

    UpdateTexture(
        textura,
        datosFrame
    );
}


//==================================================
// DIBUJAR FULLSCREEN
//==================================================

void FondoAnimado::DibujarPantallaCompleta() const
{
    //------------------------------
    // FALLBACK
    //------------------------------

    if (!cargado)
    {
        ClearBackground(
            Color{
                20,
                20,
                30,
                255
            }
        );

        return;
    }


    float anchoPantalla =
        (float)GetScreenWidth();


    float altoPantalla =
        (float)GetScreenHeight();


    //------------------------------
    // ESCALAR COMO "COVER"
    //------------------------------

    float escalaX =
        anchoPantalla /
        textura.width;


    float escalaY =
        altoPantalla /
        textura.height;


    float escala =
        escalaX > escalaY
        ? escalaX
        : escalaY;


    float anchoFinal =
        textura.width *
        escala;


    float altoFinal =
        textura.height *
        escala;


    //------------------------------
    // CENTRAR
    //------------------------------

    Rectangle origen =
    {
        0.0f,
        0.0f,
        (float)textura.width,
        (float)textura.height
    };


    Rectangle destino =
    {
        (
            anchoPantalla -
            anchoFinal
        ) / 2.0f,

        (
            altoPantalla -
            altoFinal
        ) / 2.0f,

        anchoFinal,

        altoFinal
    };


    DrawTexturePro(
        textura,
        origen,
        destino,
        Vector2{
            0.0f,
            0.0f
        },
        0.0f,
        WHITE
    );
}


//==================================================
// DESCARGAR
//==================================================

void FondoAnimado::Descargar()
{
    if (textura.id > 0)
    {
        UnloadTexture(textura);
    }


    if (imagen.data != nullptr)
    {
        UnloadImage(imagen);
    }


    cargado = false;
}