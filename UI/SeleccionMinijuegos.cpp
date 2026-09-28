#include "UI/SeleccionMinijuegos.h"

#include "Systems/Input.h"

#include <cmath>


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

    const int columnas = 8;
    const int filas =
        (CANTIDAD_MINIJUEGOS + columnas - 1) /
        columnas;

    const float separacionX = 11.0f;
    const float separacionY = 10.0f;

    float anchoCelda =
        (area.width - separacionX * (columnas - 1)) /
        (float)columnas;

    float altoCelda =
        (area.height - separacionY * (filas - 1)) /
        (float)filas;

    int columna = indice % columnas;
    int fila = indice / columnas;

    return Rectangle
    {
        area.x + columna * (anchoCelda + separacionX),
        area.y + fila * (altoCelda + separacionY),
        anchoCelda,
        altoCelda
    };
}


static void DibujarMiniatura(
    int indice,
    Rectangle rect
)
{
    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuegoPorIndice(indice);

    DrawRectangle(
        (int)rect.x,
        (int)rect.y,
        (int)rect.width,
        (int)rect.height,
        Fade(datos.color, 0.25f)
    );

    float cx = rect.x + rect.width / 2.0f;
    float cy = rect.y + (rect.height - 36.0f) / 2.0f;

    // Las miniaturas conservan sus proporciones al agregar columnas.
    float escala = std::fmin(
        1.0f,
        std::fmin((rect.width - 8.0f) / 140.0f, (rect.height - 44.0f) / 118.0f)
    );
    Camera2D camaraMiniatura{};
    camaraMiniatura.target = { cx, cy };
    camaraMiniatura.offset = { cx, cy };
    camaraMiniatura.zoom = std::fmax(escala, 0.1f);
    BeginMode2D(camaraMiniatura);

    switch (indice)
    {
        case MINIJUEGO_COLOR_SEGURO:
        {
            DrawCircle((int)cx, (int)cy + 25, 44.0f, ORANGE);
            DrawPoly({ cx, cy - 5.0f }, 6, 31.0f, 30.0f, RAYWHITE);
            DrawPolyLines({ cx, cy - 5.0f }, 6, 31.0f, 30.0f, RED);
            break;
        }

        case MINIJUEGO_PELOTAS:
        {
            DrawTriangle(
                { cx - 48.0f, cy + 34.0f },
                { cx, cy - 43.0f },
                { cx + 48.0f, cy + 34.0f },
                RAYWHITE
            );
            DrawCircle((int)cx - 20, (int)cy + 7, 18.0f, SKYBLUE);
            DrawCircle((int)cx + 20, (int)cy + 7, 18.0f, ORANGE);
            break;
        }

        case MINIJUEGO_TRONCO:
        {
            DrawRectangle((int)cx - 48, (int)cy - 11, 96, 22, BROWN);
            DrawCircle((int)cx - 48, (int)cy, 11.0f, DARKBROWN);
            DrawCircle((int)cx + 48, (int)cy, 11.0f, DARKBROWN);
            break;
        }

        case MINIJUEGO_FABRICA_67:
        {
            DrawRectangle((int)cx - 48, (int)cy - 34, 96, 14, DARKGRAY);
            DrawRectangle((int)cx - 48, (int)cy + 23, 96, 14, DARKGRAY);
            DrawText("6", (int)cx - 31, (int)cy - 16, 27, ORANGE);
            DrawText("7", (int)cx + 12, (int)cy - 16, 27, SKYBLUE);
            break;
        }

        case MINIJUEGO_ISLA_FUEGO:
        {
            DrawCircle((int)cx, (int)cy + 13, 43.0f, GRAY);
            DrawCircle((int)cx + 17, (int)cy - 25, 11.0f, DARKGRAY);
            DrawCircleLines((int)cx + 4, (int)cy + 6, 18.0f, RED);
            break;
        }

        case MINIJUEGO_CAPITAN_MANDA:
        {
            DrawText("<", (int)cx - 45, (int)cy - 31, 57, SKYBLUE);
            DrawText(">", (int)cx + 8, (int)cy - 31, 57, ORANGE);
            break;
        }

        case MINIJUEGO_BARRA_GIRATORIA:
        {
            DrawCircle((int)cx, (int)cy, 42.0f, GRAY);
            DrawLineEx(
                { cx - 44.0f, cy - 24.0f },
                { cx + 44.0f, cy + 24.0f },
                9.0f,
                ORANGE
            );
            DrawCircle((int)cx, (int)cy, 9.0f, DARKGRAY);
            break;
        }

        case MINIJUEGO_NUCLEOS_ENERGIA:
        {
            DrawCircle((int)cx - 23, (int)cy + 5, 19.0f, SKYBLUE);
            DrawCircle((int)cx + 23, (int)cy - 8, 24.0f, GOLD);
            DrawCircleLines((int)cx - 23, (int)cy + 5, 26.0f, RAYWHITE);
            DrawCircleLines((int)cx + 23, (int)cy - 8, 31.0f, ORANGE);
            break;
        }

        case MINIJUEGO_REFUGIO_PINCHOS:
        {
            DrawRectangle((int)cx - 14, (int)cy - 14, 28, 28, GRAY);
            DrawRectangle((int)cx - 53, (int)cy - 6, 25, 12, DARKGRAY);
            DrawTriangle(
                { cx - 28.0f, cy },
                { cx - 12.0f, cy - 14.0f },
                { cx - 12.0f, cy + 14.0f },
                LIGHTGRAY
            );
            DrawRectangle((int)cx + 28, (int)cy - 6, 25, 12, DARKGRAY);
            DrawTriangle(
                { cx + 28.0f, cy },
                { cx + 12.0f, cy + 14.0f },
                { cx + 12.0f, cy - 14.0f },
                LIGHTGRAY
            );
            DrawCircleLines((int)cx, (int)cy, 48.0f, BROWN);
            break;
        }

        case MINIJUEGO_MIRADAS_CRUZADAS:
        {
            DrawCircle((int)cx, (int)cy - 24, 23.0f, GOLD);
            DrawCircle((int)cx - 31, (int)cy + 23, 18.0f, SKYBLUE);
            DrawCircle((int)cx, (int)cy + 29, 18.0f, GREEN);
            DrawCircle((int)cx + 31, (int)cy + 23, 18.0f, ORANGE);
            DrawText("<", (int)cx - 8, (int)cy - 39, 28, BLACK);
            break;
        }

        case MINIJUEGO_MUROS_LOCOS:
        {
            DrawRectangle((int)cx - 52, (int)cy - 30, 37, 64, RED);
            DrawRectangle((int)cx + 16, (int)cy - 30, 36, 64, ORANGE);
            DrawRectangleLines((int)cx - 52, (int)cy - 30, 37, 64, BLACK);
            DrawRectangleLines((int)cx + 16, (int)cy - 30, 36, 64, BLACK);
            DrawCircle((int)cx, (int)cy + 18, 10.0f, SKYBLUE);
            break;
        }

        case MINIJUEGO_TORMENTA_MAGNETICA:
        {
            DrawCircle((int)cx, (int)cy, 23.0f, SKYBLUE);
            DrawCircleLines((int)cx, (int)cy, 36.0f, RAYWHITE);
            DrawCircleLines((int)cx, (int)cy, 48.0f, PINK);
            DrawText("<", (int)cx - 62, (int)cy - 15, 30, SKYBLUE);
            DrawText(">", (int)cx + 42, (int)cy - 15, 30, PINK);
            break;
        }

        case MINIJUEGO_CONTEO_EXPLOSIVO:
        {
            DrawCircle((int)cx, (int)cy, 45.0f, Fade(SKYBLUE, 0.25f));

            for (int i = 0; i < 7; i++)
            {
                float angulo = i * 51.43f * DEG2RAD;
                float radio = i % 2 == 0 ? 31.0f : 20.0f;
                DrawCircle(
                    (int)(cx + std::cos(angulo) * radio),
                    (int)(cy + std::sin(angulo) * radio),
                    6.0f,
                    i % 2 == 0 ? GOLD : SKYBLUE
                );
            }

            DrawText("?", (int)cx - 8, (int)cy - 17, 32, RAYWHITE);
            break;
        }

        case MINIJUEGO_PASO_SILENCIOSO:
        {
            DrawRectangle((int)cx - 47, (int)cy + 27, 94, 8, DARKGRAY);
            DrawCircle((int)cx + 30, (int)cy - 25, 17.0f, DARKGRAY);
            DrawCircle((int)cx + 35, (int)cy - 27, 5.0f, LIME);

            for (int i = 0; i < 3; i++)
            {
                DrawCircle(
                    (int)cx - 37 + i * 24,
                    (int)cy + 15 - i * 8,
                    8.0f,
                    i == 0 ? SKYBLUE : (i == 1 ? ORANGE : VIOLET)
                );
            }
            break;
        }

        case MINIJUEGO_CIRCUITO_VOLTAJE:
        {
            DrawEllipseLines((int)cx, (int)cy, 49.0f, 31.0f, RAYWHITE);
            DrawEllipseLines((int)cx, (int)cy, 31.0f, 15.0f, DARKGRAY);
            DrawRectanglePro(
                { cx, cy - 23.0f, 25.0f, 13.0f },
                { 12.5f, 6.5f },
                0.0f,
                ORANGE
            );
            DrawCircle((int)cx + 10, (int)cy - 23, 3.0f, RAYWHITE);
            break;
        }

        case MINIJUEGO_TRAZO_PERFECTO:
        {
            Vector2 puntos[] =
            {
                { cx, cy - 42.0f },
                { cx + 38.0f, cy - 10.0f },
                { cx + 27.0f, cy + 35.0f },
                { cx, cy + 47.0f },
                { cx - 27.0f, cy + 35.0f },
                { cx - 38.0f, cy - 10.0f }
            };

            for (int i = 0; i < 6; i++)
            {
                DrawLineEx(
                    puntos[i],
                    puntos[(i + 1) % 6],
                    4.0f,
                    SKYBLUE
                );
            }

            DrawCircleV(puntos[1], 7.0f, GOLD);
            break;
        }

        case MINIJUEGO_CARGA_INESTABLE:
        {
            DrawCircle((int)cx, (int)cy, 34.0f, RED);
            DrawCircleLines((int)cx, (int)cy, 42.0f, ORANGE);
            DrawCircle((int)cx - 9, (int)cy - 9, 8.0f, RAYWHITE);

            for (int i = 0; i < 8; i++)
            {
                float angulo = i * 45.0f * DEG2RAD;
                DrawLineEx(
                    {
                        cx + std::cos(angulo) * 45.0f,
                        cy + std::sin(angulo) * 45.0f
                    },
                    {
                        cx + std::cos(angulo) * 57.0f,
                        cy + std::sin(angulo) * 57.0f
                    },
                    3.0f,
                    GOLD
                );
            }
            break;
        }

        case MINIJUEGO_SECUENCIA_NEON:
        {
            DrawCircle((int)cx, (int)cy - 30, 16.0f, SKYBLUE);
            DrawCircle((int)cx + 34, (int)cy, 16.0f, ORANGE);
            DrawCircle((int)cx, (int)cy + 30, 16.0f, LIME);
            DrawCircle((int)cx - 34, (int)cy, 16.0f, VIOLET);

            DrawText("^", (int)cx - 7, (int)cy - 42, 22, BLACK);
            DrawText(">", (int)cx + 28, (int)cy - 11, 22, BLACK);
            DrawText("V", (int)cx - 7, (int)cy + 19, 22, BLACK);
            DrawText("<", (int)cx - 40, (int)cy - 11, 22, BLACK);
            break;
        }

        case MINIJUEGO_INTERRUPTORES_CAOS:
        {
            for (int i = 0; i < 5; i++)
            {
                const Color colores[5] =
                {
                    RED, SKYBLUE, GOLD, LIME, VIOLET
                };
                float x = cx - 42.0f + i * 21.0f;
                DrawRectangle((int)x - 7, (int)cy + 2, 14, 30, DARKGRAY);
                DrawCircle((int)x, (int)cy - 2, 10.0f, colores[i]);
                DrawCircleLines((int)x, (int)cy - 2, 10.0f, RAYWHITE);
            }
            break;
        }

        case MINIJUEGO_TANQUES_PLASMA:
        {
            DrawRectanglePro(
                { cx - 27.0f, cy + 13.0f, 42.0f, 26.0f },
                { 21.0f, 13.0f },
                -20.0f,
                SKYBLUE
            );
            DrawRectanglePro(
                { cx + 27.0f, cy - 13.0f, 42.0f, 26.0f },
                { 21.0f, 13.0f },
                160.0f,
                ORANGE
            );
            DrawCircle((int)cx, (int)cy, 6.0f, GOLD);
            DrawCircleLines((int)cx, (int)cy, 13.0f, RAYWHITE);
            break;
        }

        case MINIJUEGO_PASARELAS_VACIO:
        {
            for (int i = 0; i < 4; i++)
            {
                float x = cx - 43.0f + i * 29.0f;
                float y = cy + 25.0f - i * 17.0f;
                DrawRectangle((int)x - 13, (int)y - 5, 26, 11, SKYBLUE);
                DrawRectangleLines((int)x - 13, (int)y - 5, 26, 11, RAYWHITE);
            }

            DrawCircle((int)cx - 43, (int)cy + 9, 7.0f, GOLD);
            break;
        }

        case MINIJUEGO_CANTERA_FUGA:
        {
            DrawTriangle(
                { cx - 50.0f, cy + 34.0f },
                { cx + 50.0f, cy + 34.0f },
                { cx + 35.0f, cy - 35.0f },
                BROWN
            );
            DrawCircle((int)cx + 18, (int)cy - 13, 13.0f, DARKGRAY);
            DrawCircle((int)cx - 15, (int)cy + 12, 9.0f, GRAY);
            break;
        }

        case CANTIDAD_MINIJUEGOS:
            break;
    }

    EndMode2D();
}


static int MoverIndice(
    int actual,
    int deltaX,
    int deltaY
)
{
    const int columnas = 8;
    const int filas =
        (CANTIDAD_MINIJUEGOS + columnas - 1) /
        columnas;

    int columna = actual % columnas;
    int fila = actual / columnas;

    columna += deltaX;
    fila += deltaY;

    if (columna < 0) columna = columnas - 1;
    if (columna >= columnas) columna = 0;
    if (fila < 0) fila = filas - 1;
    if (fila >= filas) fila = 0;

    int candidato = fila * columnas + columna;

    if (candidato >= CANTIDAD_MINIJUEGOS)
    {
        candidato = CANTIDAD_MINIJUEGOS - 1;
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

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
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

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
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

        int tamanoNombre = 11;
        while (
            tamanoNombre > 8 &&
            MeasureText(ObtenerDatosMinijuegoPorIndice(i).nombre, tamanoNombre) >
                celda.width - 6.0f
        )
        {
            tamanoNombre--;
        }

        int anchoNombre =
            MeasureText(ObtenerDatosMinijuegoPorIndice(i).nombre, tamanoNombre);

        DrawText(
            ObtenerDatosMinijuegoPorIndice(i).nombre,
            (int)(celda.x + celda.width / 2.0f - anchoNombre / 2.0f),
            (int)(celda.y + celda.height - 25.0f),
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
        ObtenerDatosMinijuegoPorIndice(indiceSeleccionado);

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
