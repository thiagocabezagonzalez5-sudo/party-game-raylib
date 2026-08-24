#include "UI/MenuConfiguracion.h"

static float LimitarFloat(
    float valor,
    float minimo,
    float maximo
)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

static int LimitarEnteroCircular(
    int valor,
    int minimo,
    int maximo
)
{
    if (valor < minimo) return maximo;
    if (valor > maximo) return minimo;
    return valor;
}

static void DibujarBarra(
    int x,
    int y,
    int ancho,
    int alto,
    float valor,
    Color colorRelleno
)
{
    DrawRectangleLines(
        x,
        y,
        ancho,
        alto,
        BLACK
    );

    int anchoRelleno =
        (int)(ancho * valor);

    DrawRectangle(
        x + 2,
        y + 2,
        anchoRelleno - 4 > 0 ? anchoRelleno - 4 : 0,
        alto - 4,
        colorRelleno
    );
}

void MenuConfiguracion::Inicializar()
{
    categoriaActual = CATEGORIA_AUDIO;

    opcionAudioSeleccionada = 0;
    opcionVideoSeleccionada = 0;

    volver = false;
    configuracionCambiada = false;
}

void MenuConfiguracion::Actualizar(
    ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    AudioJuego& audio
)
{
    if (IsKeyPressed(KEY_TAB))
    {
        if (categoriaActual == CATEGORIA_AUDIO)
            categoriaActual = CATEGORIA_VIDEO;
        else
            categoriaActual = CATEGORIA_AUDIO;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        volver = true;
        return;
    }

    int direccion = 0;

    if (IsKeyPressed(KEY_LEFT))
        direccion = -1;

    if (IsKeyPressed(KEY_RIGHT))
        direccion = 1;

    bool enter = IsKeyPressed(KEY_ENTER);

    if (categoriaActual == CATEGORIA_AUDIO)
    {
        if (IsKeyPressed(KEY_UP))
        {
            opcionAudioSeleccionada--;
            opcionAudioSeleccionada =
                LimitarEnteroCircular(
                    opcionAudioSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_AUDIO - 1
                );
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            opcionAudioSeleccionada++;
            opcionAudioSeleccionada =
                LimitarEnteroCircular(
                    opcionAudioSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_AUDIO - 1
                );
        }

        switch (opcionAudioSeleccionada)
        {
            case AUDIO_VOLUMEN_SONIDOS:
            {
                if (direccion != 0)
                {
                    config.volumenSonidos =
                        LimitarFloat(
                            config.volumenSonidos + 0.05f * direccion,
                            0.0f,
                            1.0f
                        );

                    audio.AplicarVolumenSonidos(
                        config.volumenSonidos
                    );

                    configuracionCambiada = true;
                }

                break;
            }

            case AUDIO_VOLUMEN_MUSICA:
            {
                if (direccion != 0)
                {
                    config.volumenMusica =
                        LimitarFloat(
                            config.volumenMusica + 0.05f * direccion,
                            0.0f,
                            1.0f
                        );

                    audio.AplicarVolumenMusica(
                        config.volumenMusica
                    );

                    configuracionCambiada = true;
                }

                break;
            }

            case AUDIO_VOLVER:
            {
                if (enter)
                {
                    volver = true;
                }

                break;
            }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_UP))
        {
            opcionVideoSeleccionada--;
            opcionVideoSeleccionada =
                LimitarEnteroCircular(
                    opcionVideoSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_VIDEO - 1
                );
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            opcionVideoSeleccionada++;
            opcionVideoSeleccionada =
                LimitarEnteroCircular(
                    opcionVideoSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_VIDEO - 1
                );
        }

        switch (opcionVideoSeleccionada)
        {
            case VIDEO_MODO_VENTANA:
            {
                if (direccion != 0 || enter)
                {
                    int nuevoModo =
                        (int)config.modoVentana + (direccion == 0 ? 1 : direccion);

                    if (nuevoModo < (int)MODO_VENTANA)
                        nuevoModo = (int)MODO_SIN_BORDES;

                    if (nuevoModo > (int)MODO_SIN_BORDES)
                        nuevoModo = (int)MODO_VENTANA;

                    AplicarModoVentana(
                        config.modoVentana,
                        (ModoVentana)nuevoModo,
                        resoluciones[config.indiceResolucion]
                    );

                    configuracionCambiada = true;
                }

                break;
            }

            case VIDEO_RESOLUCION:
            {
                if (config.modoVentana != MODO_SIN_BORDES)
                {
                    if (direccion != 0)
                    {
                        config.indiceResolucion += direccion;

                        if (config.indiceResolucion < 0)
                            config.indiceResolucion = cantidadResoluciones - 1;

                        if (config.indiceResolucion >= cantidadResoluciones)
                            config.indiceResolucion = 0;

                        AplicarModoVentana(
                            config.modoVentana,
                            config.modoVentana,
                            resoluciones[config.indiceResolucion]
                        );

                        configuracionCambiada = true;
                    }
                }

                break;
            }

            case VIDEO_FPS:
            {
                if (direccion != 0)
                {
                    config.indiceFPS += direccion;

                    if (config.indiceFPS < 0)
                        config.indiceFPS = cantidadOpcionesFPS - 1;

                    if (config.indiceFPS >= cantidadOpcionesFPS)
                        config.indiceFPS = 0;

                    SetTargetFPS(
                        opcionesFPS[config.indiceFPS]
                    );

                    configuracionCambiada = true;
                }

                break;
            }

            case VIDEO_MOSTRAR_FPS:
            {
                if (direccion != 0 || enter)
                {
                    config.mostrarFPS = !config.mostrarFPS;
                    configuracionCambiada = true;
                }

                break;
            }

            case VIDEO_VOLVER:
            {
                if (enter)
                {
                    volver = true;
                }

                break;
            }
        }
    }
}

void MenuConfiguracion::Dibujar(
    const ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int opcionesFPS[]
)
{
    ClearBackground(
        Color{ 230, 230, 230, 255 }
    );

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    int anchoSidebar = 220;

    DrawLineEx(
        Vector2{ (float)anchoSidebar, 40.0f },
        Vector2{ (float)anchoSidebar, (float)altoPantalla - 40.0f },
        4.0f,
        BLACK
    );

    // Sidebar
    int xSidebar = 30;
    int yAudio = 120;
    int yVideo = 180;

    if (categoriaActual == CATEGORIA_AUDIO)
    {
        DrawText(">", xSidebar - 20, yAudio, 28, BLACK);
        DrawText("audio", xSidebar, yAudio, 28, BLACK);
    }
    else
    {
        DrawText("audio", xSidebar, yAudio, 28, BLACK);
    }

    if (categoriaActual == CATEGORIA_VIDEO)
    {
        DrawText(">", xSidebar - 20, yVideo, 28, BLACK);
        DrawText("video", xSidebar, yVideo, 28, BLACK);
    }
    else
    {
        DrawText("video", xSidebar, yVideo, 28, BLACK);
    }

    // Titulo principal
    int xContenido = anchoSidebar + 40;

    const char* titulo =
        categoriaActual == CATEGORIA_AUDIO
        ? "opciones de audio"
        : "opciones de video";

    DrawText(
        titulo,
        xContenido,
        50,
        54,
        BLACK
    );

    int yBase = 150;
    int separacion = 65;

    if (categoriaActual == CATEGORIA_AUDIO)
    {
        const char* nombres[] =
        {
            "volumen sonidos",
            "volumen musica",
            "volver"
        };

        for (int i = 0; i < CANTIDAD_OPCIONES_AUDIO; i++)
        {
            int y = yBase + i * separacion;
            Color color = (i == opcionAudioSeleccionada) ? BLACK : DARKGRAY;

            if (i == opcionAudioSeleccionada)
            {
                DrawText(">", xContenido - 25, y, 24, BLACK);
            }

            DrawText(
                nombres[i],
                xContenido,
                y,
                28,
                color
            );

            if (i == AUDIO_VOLUMEN_SONIDOS)
            {
                DibujarBarra(
                    xContenido + 320,
                    y + 6,
                    240,
                    24,
                    config.volumenSonidos,
                    BLACK
                );

                DrawText(
                    TextFormat("%d%%", (int)(config.volumenSonidos * 100.0f)),
                    xContenido + 580,
                    y,
                    24,
                    BLACK
                );
            }
            else if (i == AUDIO_VOLUMEN_MUSICA)
            {
                DibujarBarra(
                    xContenido + 320,
                    y + 6,
                    240,
                    24,
                    config.volumenMusica,
                    BLACK
                );

                DrawText(
                    TextFormat("%d%%", (int)(config.volumenMusica * 100.0f)),
                    xContenido + 580,
                    y,
                    24,
                    BLACK
                );
            }
        }
    }
    else
    {
        const char* nombres[] =
        {
            "modo de ventana",
            "resolucion",
            "limite fps",
            "mostrar fps",
            "volver"
        };

        for (int i = 0; i < CANTIDAD_OPCIONES_VIDEO; i++)
        {
            int y = yBase + i * separacion;
            Color color = (i == opcionVideoSeleccionada) ? BLACK : DARKGRAY;

            if (i == opcionVideoSeleccionada)
            {
                DrawText(">", xContenido - 25, y, 24, BLACK);
            }

            DrawText(
                nombres[i],
                xContenido,
                y,
                28,
                color
            );

            const char* valor = "";

            if (i == VIDEO_MODO_VENTANA)
            {
                valor = NombreModoVentana(config.modoVentana);

                DrawText(
                    valor,
                    xContenido + 350,
                    y,
                    26,
                    BLACK
                );
            }
            else if (i == VIDEO_RESOLUCION)
            {
                if (config.modoVentana == MODO_SIN_BORDES)
                {
                    DrawText(
                        "bloqueada en sin bordes",
                        xContenido + 350,
                        y,
                        24,
                        GRAY
                    );
                }
                else
                {
                    DrawText(
                        TextFormat(
                            "%d x %d",
                            resoluciones[config.indiceResolucion].ancho,
                            resoluciones[config.indiceResolucion].alto
                        ),
                        xContenido + 350,
                        y,
                        26,
                        BLACK
                    );
                }
            }
            else if (i == VIDEO_FPS)
            {
                DrawText(
                    TextFormat("%d", opcionesFPS[config.indiceFPS]),
                    xContenido + 350,
                    y,
                    26,
                    BLACK
                );
            }
            else if (i == VIDEO_MOSTRAR_FPS)
            {
                DrawText(
                    config.mostrarFPS ? "si" : "no",
                    xContenido + 350,
                    y,
                    26,
                    BLACK
                );
            }
        }
    }

    DrawText(
        "TAB: cambiar categoria  |  flechas: navegar/cambiar  |  ESC: volver",
        xContenido,
        altoPantalla - 50,
        20,
        DARKGRAY
    );
}