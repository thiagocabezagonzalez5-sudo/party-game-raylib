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
        "En equipo agarra los 6 y 7 que pasan por las cintas y arma 67 antes que el rival.",
        Color{ 229, 173, 62, 255 }
    },
    {
        "ISLA BAJO FUEGO",
        "Esquiva los proyectiles. Un impacto directo te manda volando fuera de la arena.",
        Color{ 220, 98, 52, 255 }
    },
    {
        "CAPITAN MANDA",
        "Reacciona apenas aparece la bandera. Fallar o tardar te deja fuera.",
        Color{ 111, 95, 205, 255 }
    },
    {
        "BARRA GIRATORIA",
        "Salta la barra, golpea rivales y evita caer de la plataforma.",
        Color{ 67, 196, 143, 255 }
    },
    {
        "NUCLEOS ENERGIA",
        "Recolecta energia. Un golpe hace que el rival suelte sus ultimas tres recogidas.",
        Color{ 70, 205, 225, 255 }
    },
    {
        "REFUGIO PINCHOS",
        "1 vs 3: uno controla desde que lado atacan los pinchos y el equipo se cubre tras bloques.",
        Color{ 178, 99, 74, 255 }
    },
    {
        "MIRADAS CRUZADAS",
        "1 vs 3: evita mirar hacia el mismo lado que el jugador solitario durante cinco rondas.",
        Color{ 171, 124, 221, 255 }
    }
};


static Rectangle ObtenerAreaCatalogo()
{
    return Rectangle
    {
        45.0f,
        150.0f,
        GetScreenWidth() - 460.0f,
        430.0f
    };
}


static Rectangle ObtenerCeldaCatalogo(
    int indice
)
{
    Rectangle area = ObtenerAreaCatalogo();

    const int columnas = 5;
    const float separacion = 14.0f;

    float anchoCelda =
        (area.width - separacion * (columnas - 1)) /
        (float)columnas;

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
    const DatosMinijuegoCatalogo& datos = DATOS_CATALOGO[indice];

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
            DrawPoly({ cx, cy }, 6, 34.0f, 30.0f, RAYWHITE);
            DrawPolyLines({ cx, cy }, 6, 34.0f, 30.0f, RED);
            break;
        }

        case CATALOGO_PELOTAS:
        {
            DrawCircle((int)cx - 23, (int)cy, 22.0f, SKYBLUE);
            DrawCircle((int)cx + 23, (int)cy, 22.0f, ORANGE);
            DrawCircleLines((int)cx - 23, (int)cy, 22.0f, DARKBLUE);
            DrawCircleLines((int)cx + 23, (int)cy, 22.0f, MAROON);
            break;
        }

        case CATALOGO_TRONCO:
        {
            DrawRectangle((int)cx - 48, (int)cy - 11, 96, 22, BROWN);
            DrawCircle((int)cx - 48, (int)cy, 11.0f, DARKBROWN);
            DrawCircle((int)cx + 48, (int)cy, 11.0f, DARKBROWN);
            break;
        }

        case CATALOGO_FABRICA_67:
        {
            DrawRectangle((int)cx - 52, (int)cy - 34, 104, 14, DARKGRAY);
            DrawRectangle((int)cx - 52, (int)cy + 23, 104, 14, DARKGRAY);
            DrawText("6", (int)cx - 31, (int)cy - 16, 27, ORANGE);
            DrawText("7", (int)cx + 12, (int)cy - 16, 27, SKYBLUE);
            break;
        }

        case CATALOGO_ISLA_FUEGO:
        {
            DrawCircle((int)cx, (int)cy + 13, 43.0f, GRAY);
            DrawCircle((int)cx + 17, (int)cy - 25, 11.0f, DARKGRAY);
            DrawCircleLines((int)cx + 4, (int)cy + 6, 18.0f, RED);
            break;
        }

        case CATALOGO_CAPITAN_MANDA:
        {
            DrawText("<", (int)cx - 50, (int)cy - 31, 57, SKYBLUE);
            DrawText(">", (int)cx + 13, (int)cy - 31, 57, ORANGE);
            break;
        }

        case CATALOGO_BARRA_GIRATORIA:
        {
            DrawCircle((int)cx, (int)cy, 42.0f, GRAY);
            DrawLineEx(
                { cx - 46.0f, cy - 25.0f },
                { cx + 46.0f, cy + 25.0f },
                9.0f,
                ORANGE
            );
            DrawCircle((int)cx, (int)cy, 9.0f, DARKGRAY);
            break;
        }

        case CATALOGO_NUCLEOS_ENERGIA:
        {
            DrawCircle((int)cx - 23, (int)cy + 5, 19.0f, SKYBLUE);
            DrawCircle((int)cx + 23, (int)cy - 8, 24.0f, GOLD);
            DrawCircleLines((int)cx - 23, (int)cy + 5, 26.0f, RAYWHITE);
            DrawCircleLines((int)cx + 23, (int)cy - 8, 31.0f, ORANGE);
            break;
        }

        case CATALOGO_REFUGIO_PINCHOS:
        {
            DrawRectangle((int)cx - 16, (int)cy - 16, 32, 32, GRAY);
            DrawRectangle((int)cx - 53, (int)cy - 7, 25, 14, DARKGRAY);
            DrawRectangle((int)cx + 28, (int)cy - 7, 25, 14, DARKGRAY);
            DrawTriangle(
                { cx - 28.0f, cy },
                { cx - 14.0f, cy - 11.0f },
                { cx - 14.0f, cy + 11.0f },
                RED
            );
            DrawTriangle(
                { cx + 28.0f, cy },
                { cx + 14.0f, cy + 11.0f },
                { cx + 14.0f, cy - 11.0f },
                RED
            );
            break;
        }

        case CATALOGO_MIRADAS_CRUZADAS:
        {
            DrawCircle((int)cx, (int)cy - 24, 23.0f, GOLD);
            DrawCircle((int)cx - 34, (int)cy + 23, 19.0f, SKYBLUE);
            DrawCircle((int)cx, (int)cy + 29, 19.0f, GREEN);
            DrawCircle((int)cx + 34, (int)cy + 23, 19.0f, ORANGE);
            DrawText("<", (int)cx - 8, (int)cy - 39, 28, BLACK);
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
    const int columnas = 5;
    const int filas = 2;

    int columna = actual % columnas;
    int fila = actual / columnas;

    columna += deltaX;
    fila += deltaY;

    if (columna < 0) columna = columnas - 1;
    if (columna >= columnas) columna = 0;
    if (fila < 0) fila = filas - 1;
    if (fila >= filas) fila = 0;

    int candidato = fila * columnas + columna;

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
        LeerInputSeleccionParticipante(jugadorUno);

    int nuevoIndice = indiceSeleccionado;

    if (entrada.izquierda)
        nuevoIndice = MoverIndice(nuevoIndice, -1, 0);

    if (entrada.derecha)
        nuevoIndice = MoverIndice(nuevoIndice, 1, 0);

    if (entrada.arriba)
        nuevoIndice = MoverIndice(nuevoIndice, 0, -1);

    if (entrada.abajo)
        nuevoIndice = MoverIndice(nuevoIndice, 0, 1);

    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < CANTIDAD_MINIJUEGOS_CATALOGO; i++)
    {
        Rectangle celda = ObtenerCeldaCatalogo(i);

        if (CheckCollisionPointRec(mouse, celda))
        {
            nuevoIndice = i;

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

    progresoPanel += deltaTime * 6.0f;
    if (progresoPanel > 1.0f) progresoPanel = 1.0f;

    if (entrada.confirmar)
    {
        confirmado = true;
    }

    if (entrada.cancelar || IsKeyPressed(KEY_ESCAPE))
    {
        volver = true;
    }
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

    const char* titulo = "MINIJUEGOS";

    DrawText(
        titulo,
        GetScreenWidth() / 2 - MeasureText(titulo, 40) / 2,
        42,
        40,
        RAYWHITE
    );

    Rectangle area = ObtenerAreaCatalogo();

    DrawRectangle(
        (int)area.x - 14,
        (int)area.y - 18,
        (int)area.width + 28,
        (int)area.height + 36,
        Fade(BLACK, 0.58f)
    );

    DrawRectangleLinesEx(
        {
            area.x - 14,
            area.y - 18,
            area.width + 28,
            area.height + 36
        },
        3.0f,
        Fade(RAYWHITE, 0.70f)
    );

    for (int i = 0; i < CANTIDAD_MINIJUEGOS_CATALOGO; i++)
    {
        Rectangle celda = ObtenerCeldaCatalogo(i);
        DibujarMiniatura(i, celda);

        DrawRectangle(
            (int)celda.x,
            (int)(celda.y + celda.height - 36.0f),
            (int)celda.width,
            36,
            Fade(BLACK, 0.72f)
        );

        int tamanoNombre = 13;
        int anchoNombre =
            MeasureText(DATOS_CATALOGO[i].nombre, tamanoNombre);

        DrawText(
            DATOS_CATALOGO[i].nombre,
            (int)(celda.x + celda.width / 2.0f - anchoNombre / 2.0f),
            (int)(celda.y + celda.height - 26.0f),
            tamanoNombre,
            RAYWHITE
        );

        DrawRectangleLinesEx(
            celda,
            i == indiceSeleccionado ? 5.0f : 1.5f,
            i == indiceSeleccionado
                ? RED
                : Fade(RAYWHITE, 0.55f)
        );
    }

    float suavizado =
        1.0f -
        (1.0f - progresoPanel) *
        (1.0f - progresoPanel);

    float anchoPanel = 350.0f;
    float xDestino = GetScreenWidth() - anchoPanel - 22.0f;
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

    DrawRectangleLinesEx(panel, 4.0f, RED);

    const DatosMinijuegoCatalogo& seleccionado =
        DATOS_CATALOGO[indiceSeleccionado];

    DrawText(
        seleccionado.nombre,
        (int)panel.x + 24,
        (int)panel.y + 34,
        25,
        RAYWHITE
    );

    DrawText(
        "DESCRIPCION",
        (int)panel.x + 24,
        (int)panel.y + 91,
        18,
        ORANGE
    );

    const char* texto = seleccionado.descripcion;
    int inicio = 0;
    int largo = (int)TextLength(texto);
    int y = (int)panel.y + 124;

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
            if (cantidad > 126) cantidad = 126;

            for (int k = 0; k < cantidad; k++)
            {
                linea[k] = texto[inicio + k];
            }

            linea[cantidad] = '\0';

            if (MeasureText(linea, 18) > 296)
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
        int cantidadFinal =
            fin == largo
                ? largo - inicio
                : fin - inicio;

        if (cantidadFinal > 126) cantidadFinal = 126;

        for (int k = 0; k < cantidadFinal; k++)
        {
            lineaFinal[k] = texto[inicio + k];
        }

        lineaFinal[cantidadFinal] = '\0';

        DrawText(
            lineaFinal,
            (int)panel.x + 24,
            y,
            18,
            LIGHTGRAY
        );

        y += 26;
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
        45,
        GetScreenHeight() - 82,
        19,
        jugadorUno.color
    );

    DrawText(
        "J1 ELIGE | MOVER CURSOR O MOUSE | CONFIRMAR PARA JUGAR | ESC VOLVER",
        45,
        GetScreenHeight() - 52,
        18,
        LIGHTGRAY
    );
}
