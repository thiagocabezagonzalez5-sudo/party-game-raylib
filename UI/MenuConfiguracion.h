#pragma once

#include "raylib.h"

#include "Core/WindowUtils.h"
#include "Core/ConfiguracionJuego.h"
#include "Systems/Audio.h"


enum CategoriaConfiguracion
{
    CATEGORIA_VIDEO = 0,
    CATEGORIA_GRAFICOS,
    CATEGORIA_AUDIO,
    CATEGORIA_CONTROLES,
    CATEGORIA_JUEGO,
    CATEGORIA_ACCESIBILIDAD,

    CANTIDAD_CATEGORIAS_CONFIGURACION
};


// Cada opcion tiene un identificador propio; las categorias solo
// deciden cuales se listan (ver ObtenerOpcionCategoria en el .cpp).
enum OpcionConfiguracion
{
    OPCION_NINGUNA = -1,

    OPCION_VIDEO_MODO = 0,
    OPCION_VIDEO_RESOLUCION,
    OPCION_VIDEO_FPS,
    OPCION_VIDEO_MOSTRAR_FPS,
    OPCION_VIDEO_VSYNC,

    OPCION_GRAFICO_PRESET,
    OPCION_GRAFICO_PARTICULAS,
    OPCION_GRAFICO_DECORACION,
    OPCION_GRAFICO_EFECTOS,
    OPCION_GRAFICO_SOMBRAS,

    OPCION_AUDIO_SONIDOS,
    OPCION_AUDIO_MUSICA,

    OPCION_CONTROL_MODO_TECLADO,
    OPCION_CONTROL_ZONA_MUERTA,

    OPCION_JUEGO_RESTAURAR,

    OPCION_ACCESIBILIDAD_MOVIMIENTO
};


struct MenuConfiguracion
{
    //------------------------------
    // SELECCION
    //------------------------------

    int categoriaActual =
        CATEGORIA_VIDEO;

    int opcionSeleccionada[CANTIDAD_CATEGORIAS_CONFIGURACION] =
        {};

    int categoriaHover =
        -1;


    //------------------------------
    // ESTADOS PARA EL LLAMADOR
    //------------------------------

    // El menu termino su animacion de salida: Juego debe volver al HUB.
    bool volver =
        false;

    // Algo cambio y ya es definitivo: Juego debe guardar config.ini.
    bool configuracionCambiada =
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

    bool cambiandoCategoria =
        false;

    int categoriaDestino =
        CATEGORIA_VIDEO;

    const float DURACION_TRANSICION =
        0.25f;

    const float DURACION_TRANSICION_CONTENIDO =
        0.12f;


    //------------------------------
    // ENTRADA
    //------------------------------

    float bloqueoEntrada =
        0.0f;

    int direccionPrevia =
        0;

    float tiempoDireccion =
        0.0f;

    float siguienteRepeticion =
        0.0f;

    int ejeHorizontalPrevio =
        0;

    int ejeVerticalPrevio =
        0;

    Vector2 mouseAnterior =
        { 0.0f, 0.0f };


    //------------------------------
    // MOUSE / PREVIEW DE SONIDO
    //------------------------------

    int opcionArrastrada =
        OPCION_NINGUNA;

    float enfriamientoPreview =
        0.0f;


    //------------------------------
    // CONFIRMACION DE VIDEO
    //------------------------------

    // Tras cambiar modo o resolucion se pide confirmar; si no se confirma
    // a tiempo, se vuelve a lo anterior.
    bool confirmandoVideo =
        false;

    float tiempoConfirmacion =
        0.0f;

    ModoVentana modoPrevio =
        MODO_VENTANA;

    int resolucionPrevia =
        0;

    const float DURACION_CONFIRMACION =
        10.0f;


    //------------------------------
    // AVISOS
    //------------------------------

    bool restaurarArmado =
        false;

    float tiempoRestaurar =
        0.0f;

    const char* aviso =
        "";

    float tiempoAviso =
        0.0f;

    bool vsyncSincronizado =
        false;


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
