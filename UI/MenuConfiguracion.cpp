#include "UI/MenuConfiguracion.h"


//==================================================
// COLORES
//==================================================

static const Color COLOR_NARANJA =
{
    255, 120, 20, 255
};

static const Color COLOR_CRISTAL =
{
    25, 28, 35, 255
};


//==================================================
// UTILIDADES
//==================================================

static float LimitarFloat(
    float valor,
    float minimo,
    float maximo
)
{
    if (valor < minimo)
    {
        return minimo;
    }

    if (valor > maximo)
    {
        return maximo;
    }

    return valor;
}


static int ValorCircular(
    int valor,
    int minimo,
    int maximo
)
{
    if (valor < minimo)
    {
        return maximo;
    }

    if (valor > maximo)
    {
        return minimo;
    }

    return valor;
}


static const char* NombreModoTeclado(
    ModoTeclado modo
)
{
    if (modo == TECLADO_DIVIDIDO)
    {
        return "2 JUGADORES";
    }

    return "1 JUGADOR";
}


//==================================================
// SIDEBAR
//==================================================

static int ObtenerAnchoSidebar()
{
    int ancho =
        (int)(
            GetScreenWidth() * 0.22f
        );

    if (ancho < 190)
    {
        ancho = 190;
    }

    if (ancho > 330)
    {
        ancho = 330;
    }

    return ancho;
}


//==================================================
// RECTANGULO CATEGORIA
//==================================================

static Rectangle ObtenerRectCategoria(
    int indice
)
{
    int anchoSidebar =
        ObtenerAnchoSidebar();

    const float alto =
        54.0f;

    const float separacion =
        14.0f;

    const float inicioY =
        42.0f;

    return Rectangle
    {
        30.0f,
        inicioY + indice * (alto + separacion),
        (float)anchoSidebar - 60.0f,
        alto
    };
}


//==================================================
// RECTANGULO OPCION
//==================================================

static Rectangle ObtenerRectOpcion(
    int indice
)
{
    int sidebar =
        ObtenerAnchoSidebar();

    return Rectangle
    {
        (float)sidebar + 65.0f,
        155.0f + indice * 74.0f,
        (float)GetScreenWidth() - sidebar - 130.0f,
        54.0f
    };
}


//==================================================
// BARRA AUDIO
//==================================================

static Rectangle ObtenerRectBarraAudio(
    int indice
)
{
    int sidebar =
        ObtenerAnchoSidebar();

    float ancho =
        GetScreenWidth() * 0.28f;

    if (ancho < 220.0f)
    {
        ancho = 220.0f;
    }

    if (ancho > 500.0f)
    {
        ancho = 500.0f;
    }

    return Rectangle
    {
        (float)sidebar + 390.0f,
        170.0f + indice * 74.0f,
        ancho,
        24.0f
    };
}


//==================================================
// VOLUMEN DESDE MOUSE
//==================================================

static float ObtenerVolumenMouse(
    Rectangle barra,
    float mouseX
)
{
    float porcentaje =
        (
            mouseX -
            barra.x
        ) /
        barra.width;

    return LimitarFloat(
        porcentaje,
        0.0f,
        1.0f
    );
}


//==================================================
// DIBUJAR BARRA
//==================================================

static void DibujarBarra(
    Rectangle barra,
    float valor,
    bool seleccionada,
    float alpha
)
{
    Color color =
        seleccionada
        ? COLOR_NARANJA
        : RAYWHITE;

    color =
        Fade(
            color,
            alpha
        );

    DrawRectangle(
        (int)barra.x,
        (int)barra.y,
        (int)barra.width,
        (int)barra.height,
        Fade(
            BLACK,
            0.45f * alpha
        )
    );

    int anchoRelleno =
        (int)(
            (barra.width - 4.0f) * valor
        );

    DrawRectangle(
        (int)barra.x + 2,
        (int)barra.y + 2,
        anchoRelleno,
        (int)barra.height - 4,
        color
    );

    DrawRectangleLines(
        (int)barra.x,
        (int)barra.y,
        (int)barra.width,
        (int)barra.height,
        color
    );
}


//==================================================
// INICIAR TRANSICION CONTENIDO
//==================================================

static void IniciarTransicionContenido(
    MenuConfiguracion& menu,
    PantallaMenuConfiguracion nuevaPantalla,
    int nuevaCategoria
)
{
    menu.pantallaDestino =
        nuevaPantalla;

    menu.categoriaDestino =
        nuevaCategoria;

    menu.transicionContenidoActiva =
        true;

    menu.transicionContenidoSaliendo =
        true;
}


//==================================================
// INICIAR SALIDA MENU
//==================================================

static void IniciarSalidaMenu(
    MenuConfiguracion& menu
)
{
    menu.saliendo =
        true;

    menu.arrastrandoVolumenMusica =
        false;

    menu.arrastrandoVolumenSonidos =
        false;
}


//==================================================
// CAMBIAR CATEGORIA
//==================================================

static void CambiarCategoria(
    MenuConfiguracion& menu,
    int nuevaCategoria
)
{
    nuevaCategoria =
        ValorCircular(
            nuevaCategoria,
            0,
            CANTIDAD_CATEGORIAS_CONFIGURACION - 1
        );

    if (
        nuevaCategoria ==
        menu.categoriaActual
    )
    {
        return;
    }

    IniciarTransicionContenido(
        menu,
        CONFIG_DETALLE_CATEGORIA,
        nuevaCategoria
    );
}


//==================================================
// INICIALIZAR
//==================================================

void MenuConfiguracion::Inicializar()
{
    pantallaActual =
        CONFIG_SELECCION_CATEGORIA;

    categoriaActual =
        CATEGORIA_AUDIO;

    categoriaCursor =
        CATEGORIA_AUDIO;

    categoriaHover =
        -1;

    opcionAudioSeleccionada =
        0;

    opcionVideoSeleccionada =
        0;

    opcionControlSeleccionada =
        0;

    volver =
        false;

    configuracionCambiada =
        false;

    arrastrandoVolumenSonidos =
        false;

    arrastrandoVolumenMusica =
        false;

    alphaGeneral =
        0.0f;

    alphaContenido =
        1.0f;

    saliendo =
        false;

    transicionContenidoActiva =
        false;

    transicionContenidoSaliendo =
        false;

    pantallaDestino =
        CONFIG_SELECCION_CATEGORIA;

    categoriaDestino =
        CATEGORIA_AUDIO;
}


//==================================================
// ACTUALIZAR
//==================================================

void MenuConfiguracion::Actualizar(
    ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    AudioJuego& audio
)
{
    float deltaTime =
        GetFrameTime();

    //==================================================
    // TRANSICION GENERAL
    //==================================================

    if (saliendo)
    {
        alphaGeneral -=
            deltaTime /
            DURACION_TRANSICION;

        if (alphaGeneral <= 0.0f)
        {
            alphaGeneral =
                0.0f;

            volver =
                true;
        }

        return;
    }

    if (alphaGeneral < 1.0f)
    {
        alphaGeneral +=
            deltaTime /
            DURACION_TRANSICION;

        if (alphaGeneral > 1.0f)
        {
            alphaGeneral =
                1.0f;
        }
    }

    //==================================================
    // TRANSICION ENTRE APARTADOS
    //==================================================

    if (transicionContenidoActiva)
    {
        if (transicionContenidoSaliendo)
        {
            alphaContenido -=
                deltaTime /
                DURACION_TRANSICION_CONTENIDO;

            if (alphaContenido <= 0.0f)
            {
                alphaContenido =
                    0.0f;

                pantallaActual =
                    pantallaDestino;

                categoriaActual =
                    categoriaDestino;

                categoriaCursor =
                    categoriaDestino;

                opcionAudioSeleccionada =
                    0;

                opcionVideoSeleccionada =
                    0;

                opcionControlSeleccionada =
                    0;

                transicionContenidoSaliendo =
                    false;
            }

            return;
        }
        else
        {
            alphaContenido +=
                deltaTime /
                DURACION_TRANSICION_CONTENIDO;

            if (alphaContenido >= 1.0f)
            {
                alphaContenido =
                    1.0f;

                transicionContenidoActiva =
                    false;
            }

            return;
        }
    }

    Vector2 mouse =
        GetMousePosition();

    bool click =
        IsMouseButtonPressed(
            MOUSE_BUTTON_LEFT
        );

    categoriaHover =
        -1;

    for (
        int i = 0;
        i < CANTIDAD_CATEGORIAS_CONFIGURACION;
        i++
    )
    {
        Rectangle rect =
            ObtenerRectCategoria(i);

        if (
            CheckCollisionPointRec(
                mouse,
                rect
            )
        )
        {
            categoriaHover =
                i;
        }
    }

    //==================================================
    // SELECTOR DE CATEGORIA
    //==================================================

    if (
        pantallaActual ==
        CONFIG_SELECCION_CATEGORIA
    )
    {
        if (IsKeyPressed(KEY_UP))
        {
            categoriaCursor--;

            categoriaCursor =
                ValorCircular(
                    categoriaCursor,
                    0,
                    CANTIDAD_CATEGORIAS_CONFIGURACION - 1
                );
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            categoriaCursor++;

            categoriaCursor =
                ValorCircular(
                    categoriaCursor,
                    0,
                    CANTIDAD_CATEGORIAS_CONFIGURACION - 1
                );
        }

        if (IsKeyPressed(KEY_Q))
        {
            categoriaCursor--;

            categoriaCursor =
                ValorCircular(
                    categoriaCursor,
                    0,
                    CANTIDAD_CATEGORIAS_CONFIGURACION - 1
                );
        }

        if (IsKeyPressed(KEY_E))
        {
            categoriaCursor++;

            categoriaCursor =
                ValorCircular(
                    categoriaCursor,
                    0,
                    CANTIDAD_CATEGORIAS_CONFIGURACION - 1
                );
        }

        if (categoriaHover >= 0)
        {
            categoriaCursor =
                categoriaHover;
        }

        if (
            click &&
            categoriaHover >= 0
        )
        {
            IniciarTransicionContenido(
                *this,
                CONFIG_DETALLE_CATEGORIA,
                categoriaHover
            );
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            IniciarTransicionContenido(
                *this,
                CONFIG_DETALLE_CATEGORIA,
                categoriaCursor
            );
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            IniciarSalidaMenu(
                *this
            );
        }

        return;
    }

    //==================================================
    // DETALLE CATEGORIA
    //==================================================

    if (IsKeyPressed(KEY_Q))
    {
        CambiarCategoria(
            *this,
            categoriaActual - 1
        );

        return;
    }

    if (IsKeyPressed(KEY_E))
    {
        CambiarCategoria(
            *this,
            categoriaActual + 1
        );

        return;
    }

    if (
        click &&
        categoriaHover >= 0
    )
    {
        CambiarCategoria(
            *this,
            categoriaHover
        );

        return;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        IniciarTransicionContenido(
            *this,
            CONFIG_SELECCION_CATEGORIA,
            categoriaActual
        );

        arrastrandoVolumenMusica =
            false;

        arrastrandoVolumenSonidos =
            false;

        return;
    }

    //==================================================
    // AUDIO
    //==================================================

    if (
        categoriaActual ==
        CATEGORIA_AUDIO
    )
    {
        if (IsKeyPressed(KEY_UP))
        {
            opcionAudioSeleccionada--;

            opcionAudioSeleccionada =
                ValorCircular(
                    opcionAudioSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_AUDIO - 1
                );
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            opcionAudioSeleccionada++;

            opcionAudioSeleccionada =
                ValorCircular(
                    opcionAudioSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_AUDIO - 1
                );
        }

        for (
            int i = 0;
            i < CANTIDAD_OPCIONES_AUDIO;
            i++
        )
        {
            Rectangle rect =
                ObtenerRectOpcion(i);

            if (
                CheckCollisionPointRec(
                    mouse,
                    rect
                )
            )
            {
                opcionAudioSeleccionada =
                    i;
            }
        }

        int direccion =
            0;

        if (IsKeyPressed(KEY_LEFT))
        {
            direccion =
                -1;
        }

        if (IsKeyPressed(KEY_RIGHT))
        {
            direccion =
                1;
        }

        if (
            opcionAudioSeleccionada ==
                AUDIO_VOLUMEN_SONIDOS &&
            direccion != 0
        )
        {
            config.volumenSonidos =
                LimitarFloat(
                    config.volumenSonidos +
                        0.05f * direccion,
                    0.0f,
                    1.0f
                );

            audio.AplicarVolumenSonidos(
                config.volumenSonidos
            );

            configuracionCambiada =
                true;
        }

        if (
            opcionAudioSeleccionada ==
                AUDIO_VOLUMEN_MUSICA &&
            direccion != 0
        )
        {
            config.volumenMusica =
                LimitarFloat(
                    config.volumenMusica +
                        0.05f * direccion,
                    0.0f,
                    1.0f
                );

            audio.AplicarVolumenMusica(
                config.volumenMusica
            );

            configuracionCambiada =
                true;
        }

        Rectangle barraSonidos =
            ObtenerRectBarraAudio(
                AUDIO_VOLUMEN_SONIDOS
            );

        if (
            click &&
            CheckCollisionPointRec(
                mouse,
                barraSonidos
            )
        )
        {
            arrastrandoVolumenSonidos =
                true;

            opcionAudioSeleccionada =
                AUDIO_VOLUMEN_SONIDOS;
        }

        if (
            arrastrandoVolumenSonidos &&
            IsMouseButtonDown(
                MOUSE_BUTTON_LEFT
            )
        )
        {
            config.volumenSonidos =
                ObtenerVolumenMouse(
                    barraSonidos,
                    mouse.x
                );

            audio.AplicarVolumenSonidos(
                config.volumenSonidos
            );

            configuracionCambiada =
                true;
        }

        Rectangle barraMusica =
            ObtenerRectBarraAudio(
                AUDIO_VOLUMEN_MUSICA
            );

        if (
            click &&
            CheckCollisionPointRec(
                mouse,
                barraMusica
            )
        )
        {
            arrastrandoVolumenMusica =
                true;

            opcionAudioSeleccionada =
                AUDIO_VOLUMEN_MUSICA;
        }

        if (
            arrastrandoVolumenMusica &&
            IsMouseButtonDown(
                MOUSE_BUTTON_LEFT
            )
        )
        {
            config.volumenMusica =
                ObtenerVolumenMouse(
                    barraMusica,
                    mouse.x
                );

            audio.AplicarVolumenMusica(
                config.volumenMusica
            );

            configuracionCambiada =
                true;
        }

        if (
            !IsMouseButtonDown(
                MOUSE_BUTTON_LEFT
            )
        )
        {
            arrastrandoVolumenSonidos =
                false;

            arrastrandoVolumenMusica =
                false;
        }

        Rectangle volverRect =
            ObtenerRectOpcion(
                AUDIO_VOLVER
            );

        if (
            opcionAudioSeleccionada ==
                AUDIO_VOLVER
        )
        {
            if (
                IsKeyPressed(KEY_ENTER) ||
                (
                    click &&
                    CheckCollisionPointRec(
                        mouse,
                        volverRect
                    )
                )
            )
            {
                IniciarTransicionContenido(
                    *this,
                    CONFIG_SELECCION_CATEGORIA,
                    categoriaActual
                );
            }
        }

        return;
    }

    //==================================================
    // VIDEO
    //==================================================

    if (
        categoriaActual ==
        CATEGORIA_VIDEO
    )
    {
        if (IsKeyPressed(KEY_UP))
        {
            opcionVideoSeleccionada--;

            opcionVideoSeleccionada =
                ValorCircular(
                    opcionVideoSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_VIDEO - 1
                );
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            opcionVideoSeleccionada++;

            opcionVideoSeleccionada =
                ValorCircular(
                    opcionVideoSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_VIDEO - 1
                );
        }

        for (
            int i = 0;
            i < CANTIDAD_OPCIONES_VIDEO;
            i++
        )
        {
            Rectangle rect =
                ObtenerRectOpcion(i);

            if (
                CheckCollisionPointRec(
                    mouse,
                    rect
                )
            )
            {
                opcionVideoSeleccionada =
                    i;
            }
        }

        int direccion =
            0;

        if (IsKeyPressed(KEY_LEFT))
        {
            direccion =
                -1;
        }

        if (IsKeyPressed(KEY_RIGHT))
        {
            direccion =
                1;
        }

        bool enter =
            IsKeyPressed(KEY_ENTER);

        Rectangle rectActual =
            ObtenerRectOpcion(
                opcionVideoSeleccionada
            );

        bool clickOpcion =
            click &&
            CheckCollisionPointRec(
                mouse,
                rectActual
            );

        if (
            (
                enter ||
                clickOpcion
            ) &&
            direccion == 0
        )
        {
            direccion =
                1;
        }

        if (
            opcionVideoSeleccionada ==
                VIDEO_MODO_VENTANA &&
            direccion != 0
        )
        {
            int nuevoModo =
                (int)config.modoVentana +
                direccion;

            nuevoModo =
                ValorCircular(
                    nuevoModo,
                    (int)MODO_VENTANA,
                    (int)MODO_SIN_BORDES
                );

            ModoVentana modoActual =
                config.modoVentana;

            AplicarModoVentana(
                modoActual,
                (ModoVentana)nuevoModo,
                resoluciones[
                    config.indiceResolucion
                ]
            );

            config.modoVentana =
                modoActual;

            configuracionCambiada =
                true;
        }
        else if (
            opcionVideoSeleccionada ==
                VIDEO_RESOLUCION &&
            direccion != 0
        )
        {
            if (
                config.modoVentana !=
                MODO_SIN_BORDES
            )
            {
                config.indiceResolucion +=
                    direccion;

                if (
                    config.indiceResolucion < 0
                )
                {
                    config.indiceResolucion =
                        cantidadResoluciones - 1;
                }

                if (
                    config.indiceResolucion >=
                    cantidadResoluciones
                )
                {
                    config.indiceResolucion =
                        0;
                }

                ModoVentana modoActual =
                    config.modoVentana;

                AplicarModoVentana(
                    modoActual,
                    modoActual,
                    resoluciones[
                        config.indiceResolucion
                    ]
                );

                config.modoVentana =
                    modoActual;

                configuracionCambiada =
                    true;
            }
        }
        else if (
            opcionVideoSeleccionada ==
                VIDEO_FPS &&
            direccion != 0
        )
        {
            config.indiceFPS +=
                direccion;

            if (
                config.indiceFPS < 0
            )
            {
                config.indiceFPS =
                    cantidadOpcionesFPS - 1;
            }

            if (
                config.indiceFPS >=
                cantidadOpcionesFPS
            )
            {
                config.indiceFPS =
                    0;
            }

            SetTargetFPS(
                opcionesFPS[
                    config.indiceFPS
                ]
            );

            configuracionCambiada =
                true;
        }
        else if (
            opcionVideoSeleccionada ==
                VIDEO_MOSTRAR_FPS &&
            (
                enter ||
                clickOpcion
            )
        )
        {
            config.mostrarFPS =
                !config.mostrarFPS;

            configuracionCambiada =
                true;
        }
        else if (
            opcionVideoSeleccionada ==
                VIDEO_VOLVER &&
            (
                enter ||
                clickOpcion
            )
        )
        {
            IniciarTransicionContenido(
                *this,
                CONFIG_SELECCION_CATEGORIA,
                categoriaActual
            );
        }

        return;
    }

    //==================================================
    // CONTROL
    //==================================================

    if (
        categoriaActual ==
        CATEGORIA_CONTROL
    )
    {
        if (IsKeyPressed(KEY_UP))
        {
            opcionControlSeleccionada--;

            opcionControlSeleccionada =
                ValorCircular(
                    opcionControlSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_CONTROL - 1
                );
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            opcionControlSeleccionada++;

            opcionControlSeleccionada =
                ValorCircular(
                    opcionControlSeleccionada,
                    0,
                    CANTIDAD_OPCIONES_CONTROL - 1
                );
        }

        for (
            int i = 0;
            i < CANTIDAD_OPCIONES_CONTROL;
            i++
        )
        {
            Rectangle rect =
                ObtenerRectOpcion(i);

            if (
                CheckCollisionPointRec(
                    mouse,
                    rect
                )
            )
            {
                opcionControlSeleccionada =
                    i;
            }
        }

        Rectangle rectActual =
            ObtenerRectOpcion(
                opcionControlSeleccionada
            );

        bool clickOpcion =
            click &&
            CheckCollisionPointRec(
                mouse,
                rectActual
            );

        if (
            opcionControlSeleccionada ==
            CONTROL_MODO_TECLADO
        )
        {
            if (
                IsKeyPressed(KEY_LEFT) ||
                IsKeyPressed(KEY_RIGHT) ||
                IsKeyPressed(KEY_ENTER) ||
                clickOpcion
            )
            {
                if (
                    config.modoTeclado ==
                    TECLADO_COMPLETO
                )
                {
                    config.modoTeclado =
                        TECLADO_DIVIDIDO;
                }
                else
                {
                    config.modoTeclado =
                        TECLADO_COMPLETO;
                }

                configuracionCambiada =
                    true;
            }
        }
        else if (
            opcionControlSeleccionada ==
                CONTROL_VOLVER &&
            (
                IsKeyPressed(KEY_ENTER) ||
                clickOpcion
            )
        )
        {
            IniciarTransicionContenido(
                *this,
                CONFIG_SELECCION_CATEGORIA,
                categoriaActual
            );
        }

        return;
    }

    //==================================================
    // ACERCA DE
    //==================================================

    if (
        categoriaActual ==
        CATEGORIA_ACERCA_DE
    )
    {
        Rectangle volverRect =
            ObtenerRectOpcion(0);

        if (
            IsKeyPressed(KEY_ENTER) ||
            (
                click &&
                CheckCollisionPointRec(
                    mouse,
                    volverRect
                )
            )
        )
        {
            IniciarTransicionContenido(
                *this,
                CONFIG_SELECCION_CATEGORIA,
                categoriaActual
            );
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void MenuConfiguracion::Dibujar(
    const ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int opcionesFPS[]
)
{
    int anchoPantalla =
        GetScreenWidth();

    int altoPantalla =
        GetScreenHeight();

    int anchoSidebar =
        ObtenerAnchoSidebar();

    DrawRectangle(
        0,
        0,
        anchoPantalla,
        altoPantalla,
        Fade(
            COLOR_CRISTAL,
            0.78f * alphaGeneral
        )
    );

    DrawRectangle(
        0,
        0,
        anchoSidebar,
        altoPantalla,
        Fade(
            BLACK,
            0.20f * alphaGeneral
        )
    );

    DrawLineEx(
        Vector2{
            (float)anchoSidebar,
            0.0f
        },
        Vector2{
            (float)anchoSidebar,
            (float)altoPantalla
        },
        3.0f,
        Fade(
            RAYWHITE,
            0.65f * alphaGeneral
        )
    );

    const char* categorias[] =
    {
        "AUDIO",
        "VIDEO",
        "CONTROL",
        "ACERCA DE"
    };

    for (
        int i = 0;
        i < CANTIDAD_CATEGORIAS_CONFIGURACION;
        i++
    )
    {
        Rectangle rect =
            ObtenerRectCategoria(i);

        bool activa =
            (
                pantallaActual ==
                CONFIG_DETALLE_CATEGORIA
            ) &&
            (
                categoriaActual == i
            );

        bool cursorSelector =
            (
                pantallaActual ==
                CONFIG_SELECCION_CATEGORIA
            ) &&
            (
                categoriaCursor == i
            );

        bool hover =
            (categoriaHover == i);

        if (
            activa ||
            cursorSelector
        )
        {
            DrawRectangle(
                (int)rect.x,
                (int)rect.y,
                (int)rect.width,
                (int)rect.height,
                Fade(
                    COLOR_NARANJA,
                    0.95f * alphaGeneral
                )
            );
        }

        if (
            hover &&
            !activa &&
            !cursorSelector
        )
        {
            DrawRectangleLines(
                (int)rect.x,
                (int)rect.y,
                (int)rect.width,
                (int)rect.height,
                Fade(
                    COLOR_NARANJA,
                    alphaGeneral
                )
            );
        }

        if (
            activa ||
            cursorSelector
        )
        {
            DrawText(
                ">",
                (int)rect.x - 26,
                (int)rect.y + 10,
                28,
                Fade(
                    COLOR_NARANJA,
                    alphaGeneral
                )
            );
        }

        int anchoTexto =
            MeasureText(
                categorias[i],
                28
            );

        DrawText(
            categorias[i],
            (int)(
                rect.x +
                rect.width / 2.0f -
                anchoTexto / 2.0f
            ),
            (int)rect.y + 10,
            28,
            Fade(
                RAYWHITE,
                alphaGeneral
            )
        );
    }

    int xContenido =
        anchoSidebar + 55;

    if (
        pantallaActual ==
        CONFIG_SELECCION_CATEGORIA
    )
    {
        DrawText(
            "CONFIGURACION",
            xContenido,
            40,
            48,
            Fade(
                RAYWHITE,
                alphaGeneral
            )
        );

        DrawText(
            "Selecciona una categoria",
            xContenido,
            125,
            28,
            Fade(
                LIGHTGRAY,
                alphaGeneral
            )
        );

        DrawText(
            "ENTER / CLICK - ENTRAR",
            xContenido,
            altoPantalla - 100,
            22,
            Fade(
                COLOR_NARANJA,
                alphaGeneral
            )
        );

        DrawText(
            "Q / E - CAMBIAR CATEGORIA",
            xContenido,
            altoPantalla - 70,
            18,
            Fade(
                LIGHTGRAY,
                alphaGeneral
            )
        );

        DrawText(
            "ESC - VOLVER",
            xContenido,
            altoPantalla - 42,
            18,
            Fade(
                GRAY,
                alphaGeneral
            )
        );

        return;
    }

    float alphaPanel =
        alphaGeneral *
        alphaContenido;

    const char* tituloCategoria =
        "";

    if (
        categoriaActual ==
        CATEGORIA_AUDIO
    )
    {
        tituloCategoria =
            "CONFIG AUDIO";
    }
    else if (
        categoriaActual ==
        CATEGORIA_VIDEO
    )
    {
        tituloCategoria =
            "CONFIG VIDEO";
    }
    else if (
        categoriaActual ==
        CATEGORIA_CONTROL
    )
    {
        tituloCategoria =
            "CONTROLES";
    }
    else
    {
        tituloCategoria =
            "ACERCA DE";
    }

    DrawText(
        tituloCategoria,
        xContenido,
        40,
        48,
        Fade(
            RAYWHITE,
            alphaPanel
        )
    );

    //==================================================
    // AUDIO
    //==================================================

    if (
        categoriaActual ==
        CATEGORIA_AUDIO
    )
    {
        const char* nombres[] =
        {
            "volumen sonidos",
            "volumen musica",
            "volver"
        };

        for (
            int i = 0;
            i < CANTIDAD_OPCIONES_AUDIO;
            i++
        )
        {
            Rectangle rect =
                ObtenerRectOpcion(i);

            bool seleccionada =
                (opcionAudioSeleccionada == i);

            Color color =
                seleccionada
                ? COLOR_NARANJA
                : RAYWHITE;

            if (seleccionada)
            {
                DrawText(
                    ">",
                    (int)rect.x - 28,
                    (int)rect.y + 10,
                    26,
                    Fade(
                        COLOR_NARANJA,
                        alphaPanel
                    )
                );
            }

            DrawText(
                nombres[i],
                (int)rect.x,
                (int)rect.y + 10,
                28,
                Fade(
                    color,
                    alphaPanel
                )
            );
        }

        Rectangle barraSonidos =
            ObtenerRectBarraAudio(
                AUDIO_VOLUMEN_SONIDOS
            );

        DibujarBarra(
            barraSonidos,
            config.volumenSonidos,
            opcionAudioSeleccionada ==
                AUDIO_VOLUMEN_SONIDOS,
            alphaPanel
        );

        DrawText(
            TextFormat(
                "%d%%",
                (int)(
                    config.volumenSonidos * 100.0f
                )
            ),
            (int)(
                barraSonidos.x +
                barraSonidos.width + 20
            ),
            (int)barraSonidos.y,
            22,
            Fade(
                RAYWHITE,
                alphaPanel
            )
        );

        Rectangle barraMusica =
            ObtenerRectBarraAudio(
                AUDIO_VOLUMEN_MUSICA
            );

        DibujarBarra(
            barraMusica,
            config.volumenMusica,
            opcionAudioSeleccionada ==
                AUDIO_VOLUMEN_MUSICA,
            alphaPanel
        );

        DrawText(
            TextFormat(
                "%d%%",
                (int)(
                    config.volumenMusica * 100.0f
                )
            ),
            (int)(
                barraMusica.x +
                barraMusica.width + 20
            ),
            (int)barraMusica.y,
            22,
            Fade(
                RAYWHITE,
                alphaPanel
            )
        );
    }
    //==================================================
    // VIDEO
    //==================================================
    else if (
        categoriaActual ==
        CATEGORIA_VIDEO
    )
    {
        const char* nombres[] =
        {
            "modo de ventana",
            "resolucion",
            "limite fps",
            "mostrar fps",
            "volver"
        };

        for (
            int i = 0;
            i < CANTIDAD_OPCIONES_VIDEO;
            i++
        )
        {
            Rectangle rect =
                ObtenerRectOpcion(i);

            bool seleccionada =
                (opcionVideoSeleccionada == i);

            bool bloqueada =
                (
                    i == VIDEO_RESOLUCION &&
                    config.modoVentana ==
                        MODO_SIN_BORDES
                );

            Color color =
                bloqueada
                ? GRAY
                : (
                    seleccionada
                    ? COLOR_NARANJA
                    : RAYWHITE
                );

            if (seleccionada)
            {
                DrawText(
                    ">",
                    (int)rect.x - 28,
                    (int)rect.y + 10,
                    26,
                    Fade(
                        color,
                        alphaPanel
                    )
                );
            }

            DrawText(
                nombres[i],
                (int)rect.x,
                (int)rect.y + 10,
                28,
                Fade(
                    color,
                    alphaPanel
                )
            );

            int xValor =
                (int)rect.x + 360;

            if (i == VIDEO_MODO_VENTANA)
            {
                DrawText(
                    NombreModoVentana(
                        config.modoVentana
                    ),
                    xValor,
                    (int)rect.y + 10,
                    26,
                    Fade(
                        color,
                        alphaPanel
                    )
                );
            }
            else if (i == VIDEO_RESOLUCION)
            {
                if (
                    config.modoVentana ==
                    MODO_SIN_BORDES
                )
                {
                    int monitor =
                        GetCurrentMonitor();

                    DrawText(
                        TextFormat(
                            "%d x %d - BLOQUEADA",
                            GetMonitorWidth(monitor),
                            GetMonitorHeight(monitor)
                        ),
                        xValor,
                        (int)rect.y + 10,
                        24,
                        Fade(
                            GRAY,
                            alphaPanel
                        )
                    );
                }
                else
                {
                    DrawText(
                        TextFormat(
                            "%d x %d",
                            resoluciones[
                                config.indiceResolucion
                            ].ancho,
                            resoluciones[
                                config.indiceResolucion
                            ].alto
                        ),
                        xValor,
                        (int)rect.y + 10,
                        26,
                        Fade(
                            color,
                            alphaPanel
                        )
                    );
                }
            }
            else if (i == VIDEO_FPS)
            {
                DrawText(
                    TextFormat(
                        "%d",
                        opcionesFPS[
                            config.indiceFPS
                        ]
                    ),
                    xValor,
                    (int)rect.y + 10,
                    26,
                    Fade(
                        color,
                        alphaPanel
                    )
                );
            }
            else if (i == VIDEO_MOSTRAR_FPS)
            {
                DrawText(
                    config.mostrarFPS
                    ? "SI"
                    : "NO",
                    xValor,
                    (int)rect.y + 10,
                    26,
                    Fade(
                        color,
                        alphaPanel
                    )
                );
            }
        }
    }
    //==================================================
    // CONTROL
    //==================================================
    else if (
        categoriaActual ==
        CATEGORIA_CONTROL
    )
    {
        const char* nombres[] =
        {
            "modo teclado",
            "volver"
        };

        for (
            int i = 0;
            i < CANTIDAD_OPCIONES_CONTROL;
            i++
        )
        {
            Rectangle rect =
                ObtenerRectOpcion(i);

            bool seleccionada =
                opcionControlSeleccionada ==
                i;

            Color color =
                seleccionada
                ? COLOR_NARANJA
                : RAYWHITE;

            if (seleccionada)
            {
                DrawText(
                    ">",
                    (int)rect.x - 28,
                    (int)rect.y + 10,
                    26,
                    Fade(
                        COLOR_NARANJA,
                        alphaPanel
                    )
                );
            }

            DrawText(
                nombres[i],
                (int)rect.x,
                (int)rect.y + 10,
                28,
                Fade(
                    color,
                    alphaPanel
                )
            );

            if (i == CONTROL_MODO_TECLADO)
            {
                DrawText(
                    NombreModoTeclado(
                        config.modoTeclado
                    ),
                    (int)rect.x + 330,
                    (int)rect.y + 10,
                    26,
                    Fade(
                        color,
                        alphaPanel
                    )
                );
            }
        }

        int yInfo =
            350;

        if (
            config.modoTeclado ==
            TECLADO_COMPLETO
        )
        {
            DrawText(
                "1 JUGADOR EN TECLADO",
                xContenido,
                yInfo,
                24,
                Fade(
                    COLOR_NARANJA,
                    alphaPanel
                )
            );

            DrawText(
                "P1: WASD O FLECHAS",
                xContenido,
                yInfo + 38,
                20,
                Fade(
                    LIGHTGRAY,
                    alphaPanel
                )
            );

            DrawText(
                "SALTO: ESPACIO",
                xContenido,
                yInfo + 68,
                20,
                Fade(
                    LIGHTGRAY,
                    alphaPanel
                )
            );
        }
        else
        {
            DrawText(
                "TECLADO DIVIDIDO",
                xContenido,
                yInfo,
                24,
                Fade(
                    COLOR_NARANJA,
                    alphaPanel
                )
            );

            DrawText(
                "P1: WASD + ESPACIO",
                xContenido,
                yInfo + 38,
                20,
                Fade(
                    RED,
                    alphaPanel
                )
            );

            DrawText(
                "P2: FLECHAS + CTRL DERECHO",
                xContenido,
                yInfo + 68,
                20,
                Fade(
                    BLUE,
                    alphaPanel
                )
            );
        }
    }
    //==================================================
    // ACERCA DE
    //==================================================
    else if (
        categoriaActual ==
        CATEGORIA_ACERCA_DE
    )
    {
        DrawText(
            "PARTY GAME",
            xContenido,
            150,
            42,
            Fade(
                RAYWHITE,
                alphaPanel
            )
        );

        DrawText(
            "Juego local para 2 a 4 jugadores",
            xContenido,
            225,
            24,
            Fade(
                LIGHTGRAY,
                alphaPanel
            )
        );

        DrawText(
            "Desarrollado en C++ con raylib",
            xContenido,
            265,
            24,
            Fade(
                LIGHTGRAY,
                alphaPanel
            )
        );

        DrawText(
            "Proyecto en desarrollo",
            xContenido,
            305,
            24,
            Fade(
                LIGHTGRAY,
                alphaPanel
            )
        );

        DrawText(
            "> volver",
            xContenido,
            390,
            28,
            Fade(
                COLOR_NARANJA,
                alphaPanel
            )
        );
    }

    DrawText(
        "Q / E - CAMBIAR CATEGORIA",
        xContenido,
        altoPantalla - 72,
        18,
        Fade(
            LIGHTGRAY,
            alphaPanel
        )
    );

    DrawText(
        "ESC - VOLVER A CATEGORIAS",
        xContenido,
        altoPantalla - 44,
        18,
        Fade(
            GRAY,
            alphaPanel
        )
    );
}