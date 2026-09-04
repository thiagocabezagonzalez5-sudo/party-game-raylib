#include "UI/SeleccionMinijuegos.h"

#include "Systems/Input.h"

#include <cmath>


struct DatosMinijuegoCatalogo
{
    const char* nombre;
    const char* descripcion;
    Color color;
};


static const DatosMinijuegoCatalogo DATOS_CATALOGO[
    CANTIDAD_MINIJUEGOS_CATALOGO
] =
{
    {
        "COLOR SEGURO",
        "Corre al color indicado antes de que las otras plataformas caigan.",
        Color{ 235, 92, 85, 255 }
    },
    {
        "PELOTAS",
        "Gana velocidad, choca contra los rivales y trata de tirarlos de la montana.",
        Color{ 102, 184, 235, 255 }
    },
    {
        "TRONCO 2V2",
        "Coordina con tu companero para avanzar mas rapido que el otro equipo.",
        Color{ 150, 102, 62, 255 }
    },
    {
        "FABRICA 67",
        "En equipo coloca las piezas 6 y 7 en el orden correcto antes que el rival.",
        Color{ 229, 173, 62, 255 }
    },
    {
        "ISLA BAJO FUEGO",
        "Esquiva los proyectiles. Un impacto directo te manda volando fuera de la isla.",
        Color{ 220, 98, 52, 255 }
    },
    {
        "CAPITAN MANDA",
        "Reacciona a izquierda o derecha. Fallar o tardar te deja fuera.",
        Color{ 111, 95, 205, 255 }
    },
    {
        "BARRA GIRATORIA",
        "Salta la barra que gira y acelera. El ultimo jugador sobre la arena gana.",
        Color{ 67, 196, 143, 255 }
    }
};


static Rectangle ObtenerAreaCatalogo()
{
    return Rectangle
    {
        95.0f,
        150.0f,
        GetScreenWidth() - 520.0f,
        430.0f
    };
}


static Rectangle ObtenerCeldaCatalogo(
    int indice
)
{
    Rectangle area =
        ObtenerAreaCatalogo();

    const int columnas = 4;
    const float separacion = 18.0f;

    float anchoCelda =
        (area.width - separacion * 3.0f) /
        4.0f;

    float altoCelda = 178.0f;

    int columna = indice % columnas;
    int fila = indice / columnas;

    return Rectangle
    {
        area.x + columna * (anchoCelda + separacion),
        area.y + fila * (altoCelda + 24.0f),
        anchoCelda,
        altoCelda
    };
}


static void DibujarMiniatura(
    int indice,
    Rectangle rect
)
{
    const DatosMinijuegoCatalogo& datos =
        DATOS_CATALOGO[indice];

    DrawRectangle(
        (int)rect.x,
        (int)rect.y,
        (int)rect.width,
        (int)rect.height,
        Fade(datos.color, 0.25f)
    );

    float cx = rect.x + rect.width / 2.0f;
    float cy = rect.y + rect.height / 2.0f - 10.0f;

    switch (indice)
    {
        case CATALOGO_COLOR_SEGURO:
        {
            DrawPoly(
                Vector2{ cx, cy },
                6,
                38.0f,
                30.0f,
                RAYWHITE
            );
            DrawPolyLines(
                Vector2{ cx, cy },
                6,
                38.0f,
                30.0f,
                RED
            );
            break;
        }

        case CATALOGO_PELOTAS:
        {
            DrawCircle((int)cx - 28, (int)cy, 25.0f, SKYBLUE);
            DrawCircle((int)cx + 28, (int)cy, 25.0f, ORANGE);
            DrawCircleLines((int)cx - 28, (int)cy, 25.0f, DARKBLUE);
            DrawCircleLines((int)cx + 28, (int)cy, 25.0f, MAROON);
            break;
        }

        case CATALOGO_TRONCO:
        {
            DrawRectangle(
                (int)cx - 55,
                (int)cy - 13,
                110,
                26,
                BROWN
            );
            DrawCircle((int)cx - 55, (int)cy, 13.0f, DARKBROWN);
            DrawCircle((int)cx + 55, (int)cy, 13.0f, DARKBROWN);
            break;
        }

        case CATALOGO_FABRICA_67:
        {
            DrawRectangle((int)cx - 62, (int)cy - 32, 124, 18, DARKGRAY);
            DrawRectangle((int)cx - 62, (int)cy + 18, 124, 18, DARKGRAY);
            DrawText("6", (int)cx - 36, (int)cy - 17, 28, RED);
            DrawText("7", (int)cx + 16, (int)cy - 17, 28, BLUE);
            break;
        }

        case CATALOGO_ISLA_FUEGO:
        {
            DrawCircle((int)cx, (int)cy + 13, 48.0f, GREEN);
            DrawCircle((int)cx + 17, (int)cy - 26, 12.0f, DARKGRAY);
            DrawCircleLines((int)cx, (int)cy + 13, 48.0f, DARKGREEN);
            break;
        }

        case CATALOGO_CAPITAN_MANDA:
        {
            DrawText("<", (int)cx - 58, (int)cy - 33, 62, SKYBLUE);
            DrawText(">", (int)cx + 18, (int)cy - 33, 62, ORANGE);
            break;
        }

        case CATALOGO_BARRA_GIRATORIA:
        {
            DrawCircle((int)cx, (int)cy, 46.0f, DARKGREEN);
            DrawLineEx(
                Vector2{ cx - 52.0f, cy - 28.0f },
                Vector2{ cx + 52.0f, cy + 28.0f },
                10.0f,
                ORANGE
            );
            DrawCircle((int)cx, (int)cy, 10.0f, DARKGRAY);
            break;
        }
    }
}


static int MoverIndice(
    int actual,
    int deltaX,
    int deltaY
)
{
    const int columnas = 4;

    int columna = actual % columnas;
    int fila = actual / columnas;

    columna += deltaX;
    fila += deltaY;

    if (columna < 0) columna = columnas - 1;
    if (columna >= columnas) columna = 0;
    if (fila < 0) fila = 1;
    if (fila > 1) fila = 0;

    int candidato =
        fila * columnas + columna;

    if (candidato >= CANTIDAD_MINIJUEGOS_CATALOGO)
    {
        candidato = CANTIDAD_MINIJUEGOS_CATALOGO - 1;
    }

    return candidato;
}


void SeleccionMinijuegos::Inicializar()
{
    indiceSeleccionado = 0;
    indiceAnterior = -1;
    confirmado = false;
    volver = false;
    progresoPanel = 0.0f;
}


void SeleccionMinijuegos::Actualizar(
    float deltaTime,
    const Participante& jugadorUno
)
{
    confirmado = false;
    volver = false;

    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(
            jugadorUno
        );

    int nuevoIndice =
        indiceSeleccionado;

    if (entrada.izquierda)
    {
        nuevoIndice =
            MoverIndice(nuevoIndice, -1, 0);
    }

    if (entrada.derecha)
    {
        nuevoIndice =
            MoverIndice(nuevoIndice, 1, 0);
    }

    if (entrada.arriba)
    {
        nuevoIndice =
            MoverIndice(nuevoIndice, 0, -1);
    }

    if (entrada.abajo)
    {
        nuevoIndice =
            MoverIndice(nuevoIndice, 0, 1);
    }

    Vector2 mouse =
        GetMousePosition();

    bool mouseSobreCelda = false;

    for (int i = 0; i < CANTIDAD_MINIJUEGOS_CATALOGO; i++)
    {
        Rectangle celda =
            ObtenerCeldaCatalogo(i);

        if (CheckCollisionPointRec(mouse, celda))
        {
            nuevoIndice = i;
            mouseSobreCelda = true;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                confirmado = true;
            }
        }
    }

    if (nuevoIndice != indiceSeleccionado)
    {
        indiceAnterior = indiceSeleccionado;
        indiceSeleccionado = nuevoIndice;
        progresoPanel = 0.0f;
    }

    progresoPanel +=
        deltaTime * 6.0f;

    if (progresoPanel > 1.0f)
    {
        progresoPanel = 1.0f;
    }

    if (entrada.confirmar)
    {
        confirmado = true;
    }

    if (
        entrada.cancelar ||
        IsKeyPressed(KEY_ESCAPE)
    )
    {
        volver = true;
    }

    (void)mouseSobreCelda;
}


void SeleccionMinijuegos::Dibujar(
    const Participante& jugadorUno
) const
{
    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(Color{ 15, 17, 23, 255 }, 0.60f)
    );

    const char* titulo =
        "MINIJUEGOS";

    DrawText(
        titulo,
        GetScreenWidth() / 2 -
            MeasureText(titulo, 40) / 2,
        42,
        40,
        RAYWHITE
    );

    Rectangle area =
        ObtenerAreaCatalogo();

    DrawRectangle(
        (int)area.x - 18,
        (int)area.y - 18,
        (int)area.width + 36,
        (int)area.height + 36,
        Fade(BLACK, 0.58f)
    );

    DrawRectangleLinesEx(
        Rectangle{
            area.x - 18,
            area.y - 18,
            area.width + 36,
            area.height + 36
        },
        3.0f,
        Fade(RAYWHITE, 0.70f)
    );

    for (int i = 0; i < CANTIDAD_MINIJUEGOS_CATALOGO; i++)
    {
        Rectangle celda =
            ObtenerCeldaCatalogo(i);

        DibujarMiniatura(i, celda);

        DrawRectangle(
            (int)celda.x,
            (int)(celda.y + celda.height - 36.0f),
            (int)celda.width,
            36,
            Fade(BLACK, 0.72f)
        );

        int anchoNombre =
            MeasureText(
                DATOS_CATALOGO[i].nombre,
                16
            );

        DrawText(
            DATOS_CATALOGO[i].nombre,
            (int)(celda.x + celda.width / 2.0f - anchoNombre / 2.0f),
            (int)(celda.y + celda.height - 27.0f),
            16,
            RAYWHITE
        );

        DrawRectangleLinesEx(
            celda,
            i == indiceSeleccionado
                ? 5.0f
                : 1.5f,
            i == indiceSeleccionado
                ? RED
                : Fade(RAYWHITE, 0.55f)
        );
    }

    float suavizado =
        1.0f -
        (1.0f - progresoPanel) *
        (1.0f - progresoPanel);

    float anchoPanel = 360.0f;
    float xDestino = GetScreenWidth() - anchoPanel - 28.0f;
    float xPanel =
        GetScreenWidth() + 20.0f -
        (GetScreenWidth() + 20.0f - xDestino) * suavizado;

    Rectangle panel =
    {
        xPanel,
        130.0f,
        anchoPanel,
        330.0f
    };

    DrawRectangle(
        (int)panel.x,
        (int)panel.y,
        (int)panel.width,
        (int)panel.height,
        Fade(Color{ 20, 22, 29, 255 }, 0.94f)
    );

    DrawRectangleLinesEx(
        panel,
        4.0f,
        RED
    );

    const DatosMinijuegoCatalogo& seleccionado =
        DATOS_CATALOGO[indiceSeleccionado];

    DrawText(
        seleccionado.nombre,
        (int)panel.x + 28,
        (int)panel.y + 34,
        28,
        RAYWHITE
    );

    DrawText(
        "DESCRIPCION",
        (int)panel.x + 28,
        (int)panel.y + 93,
        18,
        ORANGE
    );

    const char* texto =
        seleccionado.descripcion;

    int inicio = 0;
    int largo = (int)TextLength(texto);
    int y = (int)panel.y + 126;

    while (inicio < largo)
    {
        int fin = inicio;
        int ultimoEspacio = -1;

        while (fin < largo)
        {
            if (texto[fin] == ' ')
            {
                ultimoEspacio = fin;
            }

            char linea[128]{};
            int cantidad = fin - inicio + 1;

            if (cantidad > 126)
            {
                cantidad = 126;
            }

            for (int k = 0; k < cantidad; k++)
            {
                linea[k] = texto[inicio + k];
            }

            linea[cantidad] = '\0';

            if (MeasureText(linea, 19) > 300)
            {
                break;
            }

            fin++;
        }

        if (fin < largo && ultimoEspacio >= inicio)
        {
            fin = ultimoEspacio;
        }

        if (fin <= inicio)
        {
            fin = inicio + 1;
        }

        char lineaFinal[128]{};
        int cantidadFinal = fin - inicio;

        if (fin == largo)
        {
            cantidadFinal = largo - inicio;
        }

        if (cantidadFinal > 126)
        {
            cantidadFinal = 126;
        }

        for (int k = 0; k < cantidadFinal; k++)
        {
            lineaFinal[k] = texto[inicio + k];
        }

        lineaFinal[cantidadFinal] = '\0';

        DrawText(
            lineaFinal,
            (int)panel.x + 28,
            y,
            19,
            LIGHTGRAY
        );

        y += 27;
        inicio = fin;

        while (inicio < largo && texto[inicio] == ' ')
        {
            inicio++;
        }
    }

    DrawText(
        TextFormat(
            "J1: %s",
            ObtenerNombreControlParticipante(jugadorUno)
        ),
        95,
        GetScreenHeight() - 82,
        19,
        jugadorUno.color
    );

    DrawText(
        "J1 ELIGE | MOVER CURSOR O MOUSE | CONFIRMAR PARA JUGAR | ESC VOLVER",
        95,
        GetScreenHeight() - 52,
        18,
        LIGHTGRAY
    );
}
