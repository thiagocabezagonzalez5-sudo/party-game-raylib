#pragma once

#include "raylib.h"

#include "Core/WindowUtils.h"
#include "Core/ConfiguracionJuego.h"
#include "Systems/Audio.h"

enum CategoriaConfiguracion
{
    CATEGORIA_AUDIO = 0,
    CATEGORIA_VIDEO
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

struct MenuConfiguracion
{
    int categoriaActual = CATEGORIA_AUDIO;

    int opcionAudioSeleccionada = 0;
    int opcionVideoSeleccionada = 0;

    bool volver = false;
    bool configuracionCambiada = false;

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