#include "UI/SeleccionPersonajes.h"

#include "Systems/Input.h"


//==================================================
// COLORES DE JUGADORES
//==================================================

static Color ObtenerColorJugador(
    int indice
)
{
    switch (indice)
    {
        case 0:
            return RED;

        case 1:
            return BLUE;

        case 2:
            return GREEN;

        case 3:
            return GOLD;
    }

    return WHITE;
}


//==================================================
// UTILIDAD CIRCULAR
//==================================================

static int LimitarCircular(
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


//==================================================
// CARGAR TEXTURA SEGURA
//==================================================

static bool CargarTexturaSegura(
    const char* ruta,
    Texture2D& textura
)
{
    if (!FileExists(ruta))
    {
        TraceLog(
            LOG_WARNING,
            "No existe la textura: %s",
            ruta
        );

        return false;
    }

    textura =
        LoadTexture(
            ruta
        );


    /*
        Usamos textura.id porque tu version
        de raylib no tiene IsTextureReady().
    */

    if (textura.id <= 0)
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar: %s",
            ruta
        );

        return false;
    }


    return true;
}


//==================================================
// DIBUJAR TEXTURA AJUSTADA
//==================================================

static void DibujarTexturaAjustada(
    Texture2D textura,
    Rectangle destino,
    Color tint
)
{
    if (textura.id <= 0)
    {
        return;
    }


    float escalaX =
        destino.width /
        textura.width;

    float escalaY =
        destino.height /
        textura.height;


    /*
        Usamos la escala MENOR.

        Esto significa que la imagen entra
        completa sin deformarse.
    */

    float escala =
        escalaX < escalaY
        ? escalaX
        : escalaY;


    float anchoFinal =
        textura.width *
        escala;

    float altoFinal =
        textura.height *
        escala;


    Rectangle origen =
    {
        0.0f,
        0.0f,
        (float)textura.width,
        (float)textura.height
    };


    Rectangle destinoFinal =
    {
        destino.x +
            (
                destino.width -
                anchoFinal
            ) /
            2.0f,

        destino.y +
            (
                destino.height -
                altoFinal
            ) /
            2.0f,

        anchoFinal,
        altoFinal
    };


    DrawTexturePro(
        textura,
        origen,
        destinoFinal,
        Vector2{
            0.0f,
            0.0f
        },
        0.0f,
        tint
    );
}


//==================================================
// CUADRICULA
//==================================================

static Rectangle ObtenerRectCuadricula()
{
    float ancho =
        440.0f;

    float alto =
        440.0f;


    return Rectangle
    {
        GetScreenWidth() /
            2.0f -
            ancho /
            2.0f,

        GetScreenHeight() /
            2.0f -
            alto /
            2.0f +
            35.0f,

        ancho,

        alto
    };
}


//==================================================
// CELDA
//==================================================

static Rectangle ObtenerRectCelda(
    int indice,
    int columnas,
    int filas
)
{
    Rectangle grid =
        ObtenerRectCuadricula();


    float anchoCelda =
        grid.width /
        columnas;

    float altoCelda =
        grid.height /
        filas;


    int columna =
        indice %
        columnas;

    int fila =
        indice /
        columnas;


    return Rectangle
    {
        grid.x +
            columna *
            anchoCelda,

        grid.y +
            fila *
            altoCelda,

        anchoCelda,

        altoCelda
    };
}


//==================================================
// PANEL DEL JUGADOR
//==================================================

static Rectangle ObtenerRectPanelJugador(
    int indice
)
{
    const float ancho =
        285.0f;

    const float alto =
        245.0f;

    const float margen =
        24.0f;


    switch (indice)
    {
        case 0:
        {
            return Rectangle
            {
                margen,
                margen,
                ancho,
                alto
            };
        }


        case 1:
        {
            return Rectangle
            {
                GetScreenWidth() -
                    ancho -
                    margen,

                margen,

                ancho,

                alto
            };
        }


        case 2:
        {
            return Rectangle
            {
                margen,

                GetScreenHeight() -
                    alto -
                    margen,

                ancho,

                alto
            };
        }


        case 3:
        {
            return Rectangle
            {
                GetScreenWidth() -
                    ancho -
                    margen,

                GetScreenHeight() -
                    alto -
                    margen,

                ancho,

                alto
            };
        }
    }


    return Rectangle{
        0,
        0,
        0,
        0
    };
}


//==================================================
// MOVER CURSOR
//==================================================

static void MoverCursor(
    JugadorSeleccion& jugador,
    int deltaX,
    int deltaY,
    int columnas,
    int filas
)
{
    int columna =
        jugador.cursorPersonaje %
        columnas;


    int fila =
        jugador.cursorPersonaje /
        columnas;


    columna =
        LimitarCircular(
            columna +
            deltaX,
            0,
            columnas - 1
        );


    fila =
        LimitarCircular(
            fila +
            deltaY,
            0,
            filas - 1
        );


    jugador.cursorPersonaje =
        fila *
        columnas +
        columna;
}


//==================================================
// CONEXIONES
//==================================================

static int ContarParticipantesActivos(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (participantes[i].activo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


//==================================================
// TODOS LISTOS
//==================================================

static void ActualizarTodosListos(
    SeleccionPersonajes& seleccion,
    const Participante participantes[],
    int cantidadMaxima
)
{
    int activos = ContarParticipantesActivos(
        participantes,
        cantidadMaxima
    );

    bool cantidadValida =
        activos == 2 || activos == 4;

    bool activosPreparados =
        cantidadValida;


    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (
            participantes[i].activo &&
            (
                !participantes[i].conectado ||
                !seleccion.jugadores[i].listo
            )
        )
        {
            activosPreparados = false;
        }
    }

    seleccion.todosListos =
        activosPreparados;
}


//==================================================
// PLACEHOLDER
//==================================================

static void DibujarPlaceholder(
    Rectangle rect,
    Color color
)
{
    DrawRectangle(
        (int)rect.x,
        (int)rect.y,
        (int)rect.width,
        (int)rect.height,
        Fade(
            color,
            0.18f
        )
    );


    float centroX =
        rect.x +
        rect.width /
        2.0f;


    float centroY =
        rect.y +
        rect.height /
        2.0f;


    DrawCircle(
        (int)centroX,
        (int)(
            centroY -
            25.0f
        ),
        22.0f,
        Fade(
            RAYWHITE,
            0.75f
        )
    );


    DrawRectangle(
        (int)(
            centroX -
            24.0f
        ),
        (int)(
            centroY +
            2.0f
        ),
        48,
        60,
        Fade(
            RAYWHITE,
            0.75f
        )
    );
}


//==================================================
// COLORES SOBRE LA CELDA
//==================================================

static void DibujarJugadoresEnCelda(
    Rectangle rect,
    const int jugadores[],
    int cantidad
)
{
    if (cantidad <= 0)
    {
        return;
    }


    //------------------------------
    // UNO
    //------------------------------

    if (cantidad == 1)
    {
        DrawRectangle(
            (int)rect.x,
            (int)rect.y,
            (int)rect.width,
            (int)rect.height,
            Fade(
                ObtenerColorJugador(
                    jugadores[0]
                ),
                0.34f
            )
        );

        return;
    }


    //------------------------------
    // DOS
    //------------------------------

    if (cantidad == 2)
    {
        DrawRectangle(
            (int)rect.x,
            (int)rect.y,
            (int)(
                rect.width /
                2.0f
            ),
            (int)rect.height,
            Fade(
                ObtenerColorJugador(
                    jugadores[0]
                ),
                0.34f
            )
        );


        DrawRectangle(
            (int)(
                rect.x +
                rect.width /
                2.0f
            ),
            (int)rect.y,
            (int)(
                rect.width /
                2.0f
            ),
            (int)rect.height,
            Fade(
                ObtenerColorJugador(
                    jugadores[1]
                ),
                0.34f
            )
        );

        return;
    }


    //------------------------------
    // TRES
    //------------------------------

    if (cantidad == 3)
    {
        DrawRectangle(
            (int)rect.x,
            (int)rect.y,
            (int)(
                rect.width /
                2.0f
            ),
            (int)(
                rect.height /
                2.0f
            ),
            Fade(
                ObtenerColorJugador(
                    jugadores[0]
                ),
                0.34f
            )
        );


        DrawRectangle(
            (int)(
                rect.x +
                rect.width /
                2.0f
            ),
            (int)rect.y,
            (int)(
                rect.width /
                2.0f
            ),
            (int)(
                rect.height /
                2.0f
            ),
            Fade(
                ObtenerColorJugador(
                    jugadores[1]
                ),
                0.34f
            )
        );


        DrawRectangle(
            (int)rect.x,
            (int)(
                rect.y +
                rect.height /
                2.0f
            ),
            (int)rect.width,
            (int)(
                rect.height /
                2.0f
            ),
            Fade(
                ObtenerColorJugador(
                    jugadores[2]
                ),
                0.34f
            )
        );

        return;
    }


    //------------------------------
    // CUATRO
    //------------------------------

    for (
        int i = 0;
        i < 4;
        i++
    )
    {
        int columna =
            i %
            2;

        int fila =
            i /
            2;


        DrawRectangle(
            (int)(
                rect.x +
                columna *
                rect.width /
                2.0f
            ),

            (int)(
                rect.y +
                fila *
                rect.height /
                2.0f
            ),

            (int)(
                rect.width /
                2.0f
            ),

            (int)(
                rect.height /
                2.0f
            ),

            Fade(
                ObtenerColorJugador(
                    jugadores[i]
                ),
                0.34f
            )
        );
    }
}


//==================================================
// TRIANGULOS CONFIRMADOS
//==================================================

static void DibujarMarcadoresConfirmados(
    Rectangle rect,
    const SeleccionPersonajes& seleccion,
    const Participante participantes[],
    int cantidadMaxima,
    int indicePersonaje
)
{
    for (
        int i = 0;
        i <
        cantidadMaxima;
        i++
    )
    {
        const JugadorSeleccion& jugador =
            seleccion.jugadores[i];


        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            !jugador.listo ||
            participantes[i].personajeSeleccionado !=
                indicePersonaje
        )
        {
            continue;
        }


        Color color =
            ObtenerColorJugador(i);


        float tamano =
            34.0f;


        //------------------------------
        // JUGADOR 1
        //------------------------------

        if (i == 0)
        {
            DrawTriangle(
                Vector2{
                    rect.x,
                    rect.y
                },

                Vector2{
                    rect.x +
                    tamano,
                    rect.y
                },

                Vector2{
                    rect.x,
                    rect.y +
                    tamano
                },

                color
            );


            DrawText(
                "1",
                (int)rect.x + 5,
                (int)rect.y + 3,
                18,
                WHITE
            );
        }


        //------------------------------
        // JUGADOR 2
        //------------------------------

        else if (i == 1)
        {
            DrawTriangle(
                Vector2{
                    rect.x +
                    rect.width,
                    rect.y
                },

                Vector2{
                    rect.x +
                    rect.width -
                    tamano,
                    rect.y
                },

                Vector2{
                    rect.x +
                    rect.width,
                    rect.y +
                    tamano
                },

                color
            );


            DrawText(
                "2",

                (int)(
                    rect.x +
                    rect.width -
                    20
                ),

                (int)rect.y + 3,

                18,

                WHITE
            );
        }


        //------------------------------
        // JUGADOR 3
        //------------------------------

        else if (i == 2)
        {
            DrawTriangle(
                Vector2{
                    rect.x,
                    rect.y +
                    rect.height
                },

                Vector2{
                    rect.x +
                    tamano,
                    rect.y +
                    rect.height
                },

                Vector2{
                    rect.x,
                    rect.y +
                    rect.height -
                    tamano
                },

                color
            );


            DrawText(
                "3",

                (int)rect.x + 5,

                (int)(
                    rect.y +
                    rect.height -
                    21
                ),

                18,

                WHITE
            );
        }


        //------------------------------
        // JUGADOR 4
        //------------------------------

        else
        {
            DrawTriangle(
                Vector2{
                    rect.x +
                    rect.width,
                    rect.y +
                    rect.height
                },

                Vector2{
                    rect.x +
                    rect.width -
                    tamano,
                    rect.y +
                    rect.height
                },

                Vector2{
                    rect.x +
                    rect.width,
                    rect.y +
                    rect.height -
                    tamano
                },

                color
            );


            DrawText(
                "4",

                (int)(
                    rect.x +
                    rect.width -
                    20
                ),

                (int)(
                    rect.y +
                    rect.height -
                    21
                ),

                18,

                WHITE
            );
        }
    }
}


//==================================================
// INICIALIZAR
//==================================================

void SeleccionPersonajes::Inicializar(
    Participante participantes[],
    int cantidadMaxima
)
{
    //==================================================
    // DEFINIR PERSONAJES
    //==================================================

    personajes[0].nombre =
        "TUNG TUNG";

    personajes[0].color =
        ORANGE;

    personajes[0].rutaIcono =
        "Assets/Personajes/TungTung/Icono.png";

    personajes[0].rutaRetrato =
        "Assets/Personajes/TungTung/Retrato.png";


    personajes[1].nombre =
        "PERSONAJE 2";

    personajes[1].color =
        SKYBLUE;

    personajes[1].rutaIcono =
        "Assets/Personajes/Personaje2/Icono.png";

    personajes[1].rutaRetrato =
        "Assets/Personajes/Personaje2/Retrato.png";


    personajes[2].nombre =
        "PERSONAJE 3";

    personajes[2].color =
        LIME;

    personajes[2].rutaIcono =
        "Assets/Personajes/Personaje3/Icono.png";

    personajes[2].rutaRetrato =
        "Assets/Personajes/Personaje3/Retrato.png";


    personajes[3].nombre =
        "PERSONAJE 4";

    personajes[3].color =
        VIOLET;

    personajes[3].rutaIcono =
        "Assets/Personajes/Personaje4/Icono.png";

    personajes[3].rutaRetrato =
        "Assets/Personajes/Personaje4/Retrato.png";


    //==================================================
    // CARGAR RECURSOS UNA SOLA VEZ
    //==================================================

    if (!recursosCargados)
    {
        for (
            int i = 0;
            i <
            MAX_PERSONAJES_SELECCION;
            i++
        )
        {
            personajes[i].iconoCargado =
                CargarTexturaSegura(
                    personajes[i].rutaIcono,
                    personajes[i].icono
                );


            personajes[i].retratoCargado =
                CargarTexturaSegura(
                    personajes[i].rutaRetrato,
                    personajes[i].retrato
                );
        }


        recursosCargados =
            true;
    }


    //==================================================
    // REINICIAR JUGADORES
    //==================================================

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        participantes[i].activo =
            false;


        participantes[i].personajeSeleccionado =
            -1;


        participantes[i].color =
            ObtenerColorJugador(i);


        jugadores[i].cursorPersonaje =
            i %
            MAX_PERSONAJES_SELECCION;


        jugadores[i].listo =
            false;


        jugadores[i].bloqueoHorizontal =
            false;


        jugadores[i].bloqueoVertical =
            false;
    }


    volverAlMenu =
        false;


    todosListos =
        false;


    iniciarPartida =
        false;


    alphaEntrada =
        0.0f;
}


//==================================================
// ACTUALIZAR
//==================================================

void SeleccionPersonajes::Actualizar(
    float deltaTime,
    Participante participantes[],
    int cantidadMaxima
)
{
    //------------------------------
    // FADE
    //------------------------------

    if (
        alphaEntrada <
        1.0f
    )
    {
        alphaEntrada +=
            deltaTime /
            DURACION_ENTRADA;


        if (
            alphaEntrada >
            1.0f
        )
        {
            alphaEntrada =
                1.0f;
        }
    }


    //------------------------------
    // CONEXIONES
    //------------------------------

    ActualizarConexionesParticipantes(
        participantes,
        cantidadMaxima
    );


    //------------------------------
    // VOLVER
    //------------------------------

    if (IsKeyPressed(KEY_ESCAPE))
    {
        volverAlMenu =
            true;

        return;
    }


    //==================================================
    // JUGADORES
    //==================================================

    for (
        int i = 0;
        i <
        cantidadMaxima;
        i++
    )
    {
        JugadorSeleccion& jugador =
            jugadores[i];


        Participante& participante =
            participantes[i];


        if (!participante.conectado)
        {
            continue;
        }


        InputSeleccionParticipante entrada =
            LeerInputSeleccionParticipante(
                participante
            );


        if (!participante.activo)
        {
            if (entrada.confirmar)
            {
                participante.activo =
                    true;

                jugador.listo =
                    false;
            }

            continue;
        }


        //==================================================
        // TECLADO
        //==================================================

        if (participante.control != CONTROL_GAMEPAD)
        {
            if (!jugador.listo)
            {
                if (
                    entrada.izquierda
                )
                {
                    MoverCursor(
                        jugador,
                        -1,
                        0,
                        columnas,
                        filas
                    );
                }


                if (
                    entrada.derecha
                )
                {
                    MoverCursor(
                        jugador,
                        1,
                        0,
                        columnas,
                        filas
                    );
                }


                if (
                    entrada.arriba
                )
                {
                    MoverCursor(
                        jugador,
                        0,
                        -1,
                        columnas,
                        filas
                    );
                }


                if (
                    entrada.abajo
                )
                {
                    MoverCursor(
                        jugador,
                        0,
                        1,
                        columnas,
                        filas
                    );
                }
            }


            //------------------------------
            // CONFIRMAR
            //------------------------------

            if (
                entrada.confirmar
            )
            {
                jugador.listo =
                    true;


                participante.personajeSeleccionado =
                    jugador.cursorPersonaje;
            }


            //------------------------------
            // CANCELAR
            //------------------------------

            if (
                entrada.cancelar
            )
            {
                if (jugador.listo)
                {
                    jugador.listo =
                        false;
                }
                else
                {
                    participante.activo =
                        false;

                    participante.personajeSeleccionado =
                        -1;
                }
            }
        }


        //==================================================
        // GAMEPAD
        //==================================================

        else
        {
            bool izquierda =
                entrada.izquierda;


            bool derecha =
                entrada.derecha;


            bool arriba =
                entrada.arriba;


            bool abajo =
                entrada.abajo;


            if (!jugador.listo)
            {
                if (
                    izquierda &&
                    !jugador.bloqueoHorizontal
                )
                {
                    MoverCursor(
                        jugador,
                        -1,
                        0,
                        columnas,
                        filas
                    );


                    jugador.bloqueoHorizontal =
                        true;
                }


                if (
                    derecha &&
                    !jugador.bloqueoHorizontal
                )
                {
                    MoverCursor(
                        jugador,
                        1,
                        0,
                        columnas,
                        filas
                    );


                    jugador.bloqueoHorizontal =
                        true;
                }


                if (
                    arriba &&
                    !jugador.bloqueoVertical
                )
                {
                    MoverCursor(
                        jugador,
                        0,
                        -1,
                        columnas,
                        filas
                    );


                    jugador.bloqueoVertical =
                        true;
                }


                if (
                    abajo &&
                    !jugador.bloqueoVertical
                )
                {
                    MoverCursor(
                        jugador,
                        0,
                        1,
                        columnas,
                        filas
                    );


                    jugador.bloqueoVertical =
                        true;
                }
            }


            if (
                !izquierda &&
                !derecha
            )
            {
                jugador.bloqueoHorizontal =
                    false;
            }


            if (
                !arriba &&
                !abajo
            )
            {
                jugador.bloqueoVertical =
                    false;
            }


            //------------------------------
            // CONFIRMAR
            //------------------------------

            if (
                entrada.confirmar
            )
            {
                jugador.listo =
                    true;


                participante.personajeSeleccionado =
                    jugador.cursorPersonaje;
            }


            //------------------------------
            // CANCELAR
            //------------------------------

            if (
                entrada.cancelar
            )
            {
                if (jugador.listo)
                {
                    jugador.listo =
                        false;
                }
                else
                {
                    participante.activo =
                        false;

                    participante.personajeSeleccionado =
                        -1;
                }
            }
        }
    }


    ActualizarTodosListos(
        *this,
        participantes,
        cantidadMaxima
    );


    iniciarPartida =
        todosListos;
}


//==================================================
// DIBUJAR
//==================================================

void SeleccionPersonajes::Dibujar(
    const Participante participantes[],
    int cantidadMaxima
) const
{
    float alpha =
        alphaEntrada;


    //------------------------------
    // CRISTAL
    //------------------------------

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),

        Fade(
            Color{
                15,
                18,
                25,
                255
            },

            0.68f *
            alpha
        )
    );


    //==================================================
    // TITULO
    //==================================================

    const char* titulo =
        "ELEGI TU PERSONAJE";


    int anchoTitulo =
        MeasureText(
            titulo,
            44
        );


    DrawText(
        titulo,

        GetScreenWidth() /
            2 -
            anchoTitulo /
            2,

        38,

        44,

        Fade(
            RAYWHITE,
            alpha
        )
    );


    //==================================================
    // GRID
    //==================================================

    Rectangle grid =
        ObtenerRectCuadricula();


    DrawRectangle(
        (int)grid.x - 10,
        (int)grid.y - 10,
        (int)grid.width + 20,
        (int)grid.height + 20,

        Fade(
            BLACK,
            0.50f *
            alpha
        )
    );


    for (
        int i = 0;
        i <
        MAX_PERSONAJES_SELECCION;
        i++
    )
    {
        Rectangle celda =
            ObtenerRectCelda(
                i,
                columnas,
                filas
            );


        //------------------------------
        // FONDO
        //------------------------------

        DrawRectangle(
            (int)celda.x,
            (int)celda.y,
            (int)celda.width,
            (int)celda.height,

            Fade(
                DARKGRAY,
                0.85f *
                alpha
            )
        );


        //------------------------------
        // ICONO
        //------------------------------

        Rectangle areaImagen =
        {
            celda.x + 10.0f,
            celda.y + 10.0f,
            celda.width - 20.0f,
            celda.height - 45.0f
        };


        if (
            personajes[i]
                .iconoCargado
        )
        {
            DibujarTexturaAjustada(
                personajes[i].icono,
                areaImagen,
                Fade(
                    WHITE,
                    alpha
                )
            );
        }
        else
        {
            DibujarPlaceholder(
                areaImagen,
                personajes[i].color
            );
        }


        //------------------------------
        // JUGADORES APUNTANDO
        //------------------------------

        int jugadoresHover[4];

        int cantidadHover =
            0;


        for (
            int j = 0;
            j < cantidadMaxima;
            j++
        )
        {
            if (
                participantes[j].activo &&
                participantes[j].conectado &&
                jugadores[j].cursorPersonaje ==
                    i
            )
            {
                jugadoresHover[
                    cantidadHover
                ] =
                    j;


                cantidadHover++;
            }
        }


        DibujarJugadoresEnCelda(
            celda,
            jugadoresHover,
            cantidadHover
        );


        //------------------------------
        // BORDE
        //------------------------------

        DrawRectangleLines(
            (int)celda.x,
            (int)celda.y,
            (int)celda.width,
            (int)celda.height,

            Fade(
                RAYWHITE,
                0.9f *
                alpha
            )
        );


        //------------------------------
        // NOMBRE
        //------------------------------

        DrawRectangle(
            (int)celda.x,
            (int)(
                celda.y +
                celda.height -
                36
            ),

            (int)celda.width,

            36,

            Fade(
                BLACK,
                0.70f *
                alpha
            )
        );


        int anchoNombre =
            MeasureText(
                personajes[i].nombre,
                20
            );


        DrawText(
            personajes[i].nombre,

            (int)(
                celda.x +
                celda.width /
                2.0f -
                anchoNombre /
                2
            ),

            (int)(
                celda.y +
                celda.height -
                28
            ),

            20,

            Fade(
                RAYWHITE,
                alpha
            )
        );


        //------------------------------
        // TRIANGULOS
        //------------------------------

        DibujarMarcadoresConfirmados(
            celda,
            *this,
            participantes,
            cantidadMaxima,
            i
        );
    }


    //==================================================
    // PANELES JUGADORES
    //==================================================

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        const JugadorSeleccion& jugador =
            jugadores[i];


        const Participante& participante =
            participantes[i];


        Rectangle panel =
            ObtenerRectPanelJugador(i);


        Color colorJugador =
            participante.color;


        //------------------------------
        // NO CONECTADO
        //------------------------------

        if (!participante.conectado)
        {
            DrawRectangle(
                (int)panel.x,
                (int)panel.y,
                (int)panel.width,
                (int)panel.height,

                Fade(
                    BLACK,
                    0.42f *
                    alpha
                )
            );


            DrawRectangleLines(
                (int)panel.x,
                (int)panel.y,
                (int)panel.width,
                (int)panel.height,

                Fade(
                    GRAY,
                    0.65f *
                    alpha
                )
            );


            DrawText(
                TextFormat(
                    "JUGADOR %d",
                    i + 1
                ),

                (int)panel.x + 15,

                (int)panel.y + 15,

                28,

                Fade(
                    GRAY,
                    alpha
                )
            );


            DrawText(
                participante.activo
                ? "CONTROL DESCONECTADO"
                : "NO CONECTADO",

                (int)panel.x + 15,

                (int)panel.y + 65,

                20,

                Fade(
                    GRAY,
                    alpha
                )
            );


            continue;
        }


        //------------------------------
        // DISPONIBLE PARA INCORPORARSE
        //------------------------------

        if (!participante.activo)
        {
            DrawRectangle(
                (int)panel.x,
                (int)panel.y,
                (int)panel.width,
                (int)panel.height,
                Fade(BLACK, 0.32f * alpha)
            );

            DrawRectangleLines(
                (int)panel.x,
                (int)panel.y,
                (int)panel.width,
                (int)panel.height,
                Fade(colorJugador, 0.70f * alpha)
            );

            DrawText(
                TextFormat("JUGADOR %d", i + 1),
                (int)panel.x + 15,
                (int)panel.y + 15,
                28,
                Fade(colorJugador, alpha)
            );

            DrawText(
                ObtenerNombreControlParticipante(
                    participante
                ),
                (int)panel.x + 15,
                (int)panel.y + 58,
                18,
                Fade(RAYWHITE, alpha)
            );

            DrawText(
                "CONFIRMAR PARA UNIRSE",
                (int)panel.x + 15,
                (int)panel.y + 95,
                18,
                Fade(LIGHTGRAY, alpha)
            );

            continue;
        }


        //==================================================
        // PERSONAJE MOSTRADO
        //==================================================

        int indicePersonaje =
            jugador.listo
            ? participante.personajeSeleccionado
            : jugador.cursorPersonaje;


        if (indicePersonaje < 0)
        {
            indicePersonaje =
                0;
        }


        //==================================================
        // FONDO DEL PANEL
        //==================================================

        DrawRectangle(
            (int)panel.x,
            (int)panel.y,
            (int)panel.width,
            (int)panel.height,

            Fade(
                colorJugador,
                0.20f *
                alpha
            )
        );


        //------------------------------
        // RETRATO
        //------------------------------

        if (
            personajes[
                indicePersonaje
            ].retratoCargado
        )
        {
            DibujarTexturaAjustada(
                personajes[
                    indicePersonaje
                ].retrato,

                panel,

                Fade(
                    WHITE,
                    0.72f *
                    alpha
                )
            );
        }
        else
        {
            DibujarPlaceholder(
                panel,
                personajes[
                    indicePersonaje
                ].color
            );
        }


        //------------------------------
        // OSCURECER ABAJO
        //------------------------------

        DrawRectangle(
            (int)panel.x,
            (int)(
                panel.y +
                panel.height -
                72
            ),

            (int)panel.width,

            72,

            Fade(
                BLACK,
                0.68f *
                alpha
            )
        );


        //------------------------------
        // BORDE
        //------------------------------

        DrawRectangleLines(
            (int)panel.x,
            (int)panel.y,
            (int)panel.width,
            (int)panel.height,

            Fade(
                colorJugador,
                alpha
            )
        );


        //------------------------------
        // JUGADOR
        //------------------------------

        DrawText(
            TextFormat(
                "JUGADOR %d",
                i + 1
            ),

            (int)panel.x + 12,

            (int)panel.y + 10,

            26,

            Fade(
                colorJugador,
                alpha
            )
        );


        //------------------------------
        // DISPOSITIVO
        //------------------------------

        DrawText(
            ObtenerNombreControlParticipante(
                participante
            ),

            (int)panel.x + 12,

            (int)panel.y + 42,

            18,

            Fade(
                RAYWHITE,
                alpha
            )
        );


        //------------------------------
        // PERSONAJE
        //------------------------------

        DrawText(
            personajes[
                indicePersonaje
            ].nombre,

            (int)panel.x + 12,

            (int)(
                panel.y +
                panel.height -
                62
            ),

            22,

            Fade(
                RAYWHITE,
                alpha
            )
        );


        //------------------------------
        // ESTADO
        //------------------------------

        DrawText(
            jugador.listo
            ? "LISTO"
            : "ELIGIENDO",

            (int)panel.x + 12,

            (int)(
                panel.y +
                panel.height -
                34
            ),

            20,

            Fade(
                jugador.listo
                ? colorJugador
                : LIGHTGRAY,

                alpha
            )
        );
    }


    //==================================================
    // ABAJO
    //==================================================

    if (todosListos)
    {
        const char* texto =
            "TODOS LISTOS";


        int ancho =
            MeasureText(
                texto,
                28
            );


        DrawText(
            texto,

            GetScreenWidth() /
                2 -
                ancho /
                2,

            GetScreenHeight() -
                45,

            28,

            Fade(
                GOLD,
                alpha
            )
        );
    }
    else
    {
        const char* texto =
            "WASD + ESPACIO | MANDO: STICK/DPAD + A | ESC: VOLVER";


        int ancho =
            MeasureText(
                texto,
                18
            );


        DrawText(
            texto,

            GetScreenWidth() /
                2 -
                ancho /
                2,

            GetScreenHeight() -
                38,

            18,

            Fade(
                LIGHTGRAY,
                alpha
            )
        );
    }
}


//==================================================
// DESCARGAR
//==================================================

void SeleccionPersonajes::Descargar()
{
    if (!recursosCargados)
    {
        return;
    }


    for (
        int i = 0;
        i <
        MAX_PERSONAJES_SELECCION;
        i++
    )
    {
        if (
            personajes[i]
                .iconoCargado &&
            personajes[i]
                .icono.id >
                0
        )
        {
            UnloadTexture(
                personajes[i]
                    .icono
            );


            personajes[i]
                .icono.id =
                0;


            personajes[i]
                .iconoCargado =
                false;
        }


        if (
            personajes[i]
                .retratoCargado &&
            personajes[i]
                .retrato.id >
                0
        )
        {
            UnloadTexture(
                personajes[i]
                    .retrato
            );


            personajes[i]
                .retrato.id =
                0;


            personajes[i]
                .retratoCargado =
                false;
        }
    }


    recursosCargados =
        false;
}
