#pragma once

#include "raylib.h"

#include "Core/WindowUtils.h"
#include "Core/ConfiguracionJuego.h"
#include "Systems/Audio.h"


enum PantallaMenuConfiguracion
{
    CONFIG_SELECCION_CATEGORIA = 0,
    CONFIG_DETALLE_CATEGORIA
};


enum CategoriaConfiguracion
{
    CATEGORIA_AUDIO = 0,
    CATEGORIA_VIDEO,
    CATEGORIA_CONTROL,
    CATEGORIA_ACERCA_DE,

    CANTIDAD_CATEGORIAS_CONFIGURACION
};


enum OpcionAudio
{
    AUDIO_VOLUMEN_SONIDOS = 0,
    AUDIO_VOLUMEN_MUSICA,
    AUDIO_VOLVER,

    CANTIDAD_OPCIONES_AUDIO
};


enum OpcionVideo
{
    VIDEO_MODO_VENTANA = 0,
    VIDEO_RESOLUCION,
    VIDEO_FPS,
    VIDEO_MOSTRAR_FPS,
    VIDEO_VOLVER,

    CANTIDAD_OPCIONES_VIDEO
};


enum OpcionControl
{
    CONTROL_MODO_TECLADO = 0,
    CONTROL_VOLVER,

    CANTIDAD_OPCIONES_CONTROL
};


struct MenuConfiguracion
{
    //------------------------------
    // PANTALLA
    //------------------------------

    PantallaMenuConfiguracion pantallaActual =
        CONFIG_SELECCION_CATEGORIA;


    //------------------------------
    // CATEGORIAS
    //------------------------------

    int categoriaActual =
        CATEGORIA_AUDIO;

    int categoriaCursor =
        CATEGORIA_AUDIO;

    int categoriaHover =
        -1;


    //------------------------------
    // OPCIONES
    //------------------------------

    int opcionAudioSeleccionada =
        0;

    int opcionVideoSeleccionada =
        0;

    int opcionControlSeleccionada =
        0;


    //------------------------------
    // ESTADOS
    //------------------------------

    bool volver =
        false;

    bool configuracionCambiada =
        false;


    //------------------------------
    // MOUSE
    //------------------------------

    bool arrastrandoVolumenSonidos =
        false;

    bool arrastrandoVolumenMusica =
        false;


    //------------------------------
    // TRANSICIONES
    //------------------------------

    float alphaGeneral =
        0.0f;

    float alphaContenido =
        1.0f;

    bool saliendo =
        false;

    bool transicionContenidoActiva =
        false;

    bool transicionContenidoSaliendo =
        false;

    PantallaMenuConfiguracion pantallaDestino =
        CONFIG_SELECCION_CATEGORIA;

    int categoriaDestino =
        CATEGORIA_AUDIO;

    const float DURACION_TRANSICION =
        0.25f;

    const float DURACION_TRANSICION_CONTENIDO =
        0.16f;


    //------------------------------
    // FUNCIONES
    //------------------------------

    void Inicializar();

    void Actualizar(
        ConfiguracionJuego& config,
        Resolucion resoluciones[],
        int cantidadResoluciones,
        int opcionesFPS[],
        int cantidadOpcionesFPS,
        AudioJuego& audio
    );

    void Dibujar(
        const ConfiguracionJuego& config,
        Resolucion resoluciones[],
        int opcionesFPS[]
    );
};