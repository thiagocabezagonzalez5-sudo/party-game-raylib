#include "UI/MiniaturasMinijuegos.h"

#include <cmath>


void DibujarMiniaturaMinijuego(
    IdMinijuego id,
    Rectangle rect,
    float escalaMaxima
)
{
    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(id);

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
        escalaMaxima,
        std::fmin((rect.width - 8.0f) / 140.0f, (rect.height - 44.0f) / 118.0f)
    );
    Camera2D camaraMiniatura{};
    camaraMiniatura.target = { cx, cy };
    camaraMiniatura.offset = { cx, cy };
    camaraMiniatura.zoom = std::fmax(escala, 0.1f);
    BeginMode2D(camaraMiniatura);

    switch (id)
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

        case MINIJUEGO_TERRITORIO_CONQUISTA:
        {
            // Cuadricula de baldosas pintadas por cuatro colores.
            const Color colores[4] = { SKYBLUE, ORANGE, LIME, VIOLET };

            for (int fila = 0; fila < 4; fila++)
            {
                for (int columna = 0; columna < 4; columna++)
                {
                    int indice = (fila / 2) * 2 + (columna / 2);
                    if ((fila + columna) % 3 == 0) indice = (indice + 1) % 4;

                    DrawRectangle(
                        (int)cx - 44 + columna * 22,
                        (int)cy - 44 + fila * 22,
                        20,
                        20,
                        colores[indice]
                    );
                }
            }

            DrawCircleLines((int)cx, (int)cy, 20.0f, RAYWHITE);
            break;
        }

        case MINIJUEGO_DEFENSA_NUCLEO:
        {
            // Cancha con dos arcos y el nucleo en el centro.
            DrawRectangleLines((int)cx - 52, (int)cy - 34, 104, 68, RAYWHITE);
            DrawLine((int)cx, (int)cy - 34, (int)cx, (int)cy + 34, Fade(RAYWHITE, 0.6f));
            DrawRectangle((int)cx - 56, (int)cy - 14, 6, 28, SKYBLUE);
            DrawRectangle((int)cx + 50, (int)cy - 14, 6, 28, ORANGE);
            DrawCircle((int)cx + 8, (int)cy - 6, 9.0f, GOLD);
            DrawCircleLines((int)cx + 8, (int)cy - 6, 14.0f, YELLOW);
            DrawCircle((int)cx - 32, (int)cy + 12, 8.0f, SKYBLUE);
            DrawCircle((int)cx + 32, (int)cy + 14, 8.0f, ORANGE);
            break;
        }

        case MINIJUEGO_LLUVIA_APILADA:
        {
            // Pila de piezas y una pieza cayendo con su sombra.
            const Color colores[4] = { PINK, GOLD, SKYBLUE, PINK };

            for (int i = 0; i < 4; i++)
            {
                DrawRectangle(
                    (int)cx - 38,
                    (int)cy + 26 - i * 13,
                    30,
                    11,
                    colores[i]
                );
            }

            DrawRectangle((int)cx + 14, (int)cy - 40, 24, 11, GOLD);
            DrawLine((int)cx + 26, (int)cy - 26, (int)cx + 26, (int)cy + 18, Fade(RAYWHITE, 0.5f));
            DrawEllipse((int)cx + 26, (int)cy + 34, 16.0f, 5.0f, Fade(BLACK, 0.45f));
            break;
        }

        case MINIJUEGO_CUERDA_ACANTILADO:
        {
            // Dos acantilados unidos por la cuerda sobre el mar.
            DrawRectangle((int)cx - 56, (int)cy + 4, 34, 36, BROWN);
            DrawRectangle((int)cx + 22, (int)cy + 4, 34, 36, BROWN);
            DrawRectangle((int)cx - 22, (int)cy + 26, 44, 14, BLUE);
            DrawLineEx({ cx - 40.0f, cy - 4.0f }, { cx + 40.0f, cy - 4.0f }, 3.0f, BEIGE);
            DrawCircle((int)cx - 40, (int)cy - 10, 8.0f, GOLD);
            DrawCircle((int)cx + 30, (int)cy - 10, 7.0f, SKYBLUE);
            DrawCircle((int)cx + 40, (int)cy - 10, 7.0f, LIME);
            DrawCircle((int)cx + 50, (int)cy - 10, 7.0f, ORANGE);
            break;
        }

        case MINIJUEGO_ULTIMO_ASIENTO:
        {
            // Carrusel con valla de bombillas y tres tazas: libre, trampa y ocupada.
            DrawCircle((int)cx, (int)(cy + 2), 38, Color{ 70, 52, 96, 255 });
            DrawCircle((int)cx, (int)(cy + 2), 34, Color{ 120, 84, 160, 255 });
            for (int k = 0; k < 14; k++)
            {
                float a = (float)k * 6.2832f / 14.0f;
                DrawCircle((int)(cx + std::cos(a) * 36), (int)(cy + 2 + std::sin(a) * 36), 2,
                    k % 2 == 0 ? Color{ 255, 220, 90, 255 } : Color{ 255, 120, 200, 255 });
            }
            DrawCircle((int)(cx - 20), (int)(cy + 16), 9, Color{ 255, 235, 120, 255 });
            DrawCircle((int)(cx - 20), (int)(cy + 16), 5, Color{ 255, 255, 255, 255 });
            DrawCircle((int)(cx + 22), (int)(cy + 14), 9, Color{ 235, 50, 50, 255 });
            DrawCircle((int)(cx + 22), (int)(cy + 14), 5, Color{ 255, 255, 255, 255 });
            DrawCircle((int)(cx + 18), (int)(cy - 20), 9, Color{ 62, 124, 238, 255 });
            DrawRectangle((int)(cx + 15), (int)(cy - 25), 6, 8, Color{ 240, 240, 245, 255 });
            DrawCircle((int)cx, (int)(cy - 4), 12, Color{ 200, 60, 90, 255 });
            DrawRectangle((int)(cx - 2), (int)(cy - 22), 4, 18, Color{ 250, 240, 220, 255 });
            DrawTriangle(
                Vector2{ cx, cy - 32 },
                Vector2{ cx - 14, cy - 18 },
                Vector2{ cx + 14, cy - 18 },
                Color{ 70, 130, 230, 255 });
            DrawCircle((int)cx, (int)(cy - 33), 2, GOLD);
            break;
        }

        case MINIJUEGO_CAJAS_PUERTO:
        {
            int x0 = (int)cx - 52;
            int y0 = (int)cy - 42;
            DrawRectangle(x0, y0, 104, 84, Color{ 10, 18, 36, 255 });
            DrawRectangle(x0, y0 + 56, 104, 28, Color{ 86, 62, 42, 255 });
            DrawRectangle(x0, y0 + 76, 104, 8, Color{ 9, 22, 42, 255 });
            DrawRectangle(x0 + 6, y0 + 8, 92, 5, Color{ 224, 164, 40, 255 });
            DrawRectangle(x0 + 10, y0 + 8, 4, 48, Color{ 224, 164, 40, 255 });
            DrawRectangle(x0 + 90, y0 + 8, 4, 48, Color{ 224, 164, 40, 255 });
            DrawLineEx({ cx, cy - 28.0f }, { cx, cy - 6.0f }, 2.0f, LIGHTGRAY);
            DrawTriangle({ cx - 5.0f, cy - 6.0f }, { cx, cy + 1.0f }, { cx + 5.0f, cy - 6.0f }, Color{ 90, 94, 100, 255 });
            const Color colores[4] = { Color{ 168, 62, 52, 255 }, Color{ 52, 110, 166, 255 }, Color{ 70, 140, 84, 255 }, Color{ 196, 150, 56, 255 } };
            for (int k = 0; k < 4; k++)
            {
                int bx = x0 + 8 + k * 24;
                DrawRectangle(bx, y0 + 34, 20, 22, colores[k]);
                DrawRectangle(bx + 9, y0 + 36, 2, 18, Color{ 0, 0, 0, 120 });
            }
            DrawCircle((int)cx + 28, y0 + 30, 3.0f, GOLD);
            DrawCircle((int)cx - 22, y0 + 24, 6.0f, Color{ 255, 150, 50, 200 });
            DrawCircle((int)cx - 22, y0 + 24, 3.0f, Color{ 255, 230, 140, 255 });
            break;
        }

        case MINIJUEGO_LABERINTO_INCLINADO:
        {
            // Losa de piedra con laberinto, agujero, esfera de jade y altar dorado.
            DrawRectangle((int)(cx - 50), (int)(cy - 40), 100, 80, Color{ 82, 74, 62, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy - 36), 92, 72, Color{ 108, 99, 83, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy - 12), 62, 8, Color{ 150, 136, 112, 255 });
            DrawRectangle((int)(cx - 16), (int)(cy + 10), 62, 8, Color{ 150, 136, 112, 255 });
            DrawRectangle((int)(cx + 8), (int)(cy - 36), 8, 24, Color{ 150, 136, 112, 255 });
            DrawCircle((int)(cx - 30), (int)(cy + 24), 7, Color{ 170, 70, 40, 255 });
            DrawCircle((int)(cx - 30), (int)(cy + 24), 5, Color{ 8, 6, 12, 255 });
            DrawCircle((int)(cx - 34), (int)(cy - 24), 6, Color{ 52, 176, 128, 255 });
            DrawCircle((int)(cx - 36), (int)(cy - 26), 2, Color{ 200, 255, 225, 255 });
            DrawRectangle((int)(cx - 8), (int)(cy - 28), 10, 3, Color{ 235, 70, 40, 255 });
            DrawRectangle((int)(cx + 28), (int)(cy + 20), 14, 14, Color{ 168, 154, 128, 255 });
            DrawCircle((int)(cx + 35), (int)(cy + 24), 5, Color{ 240, 196, 70, 255 });
            DrawRectangleLines((int)(cx - 50), (int)(cy - 40), 100, 80, Fade(Color{ 52, 176, 128, 255 }, 0.9f));
            break;
        }

        case MINIJUEGO_VETA_CRISTAL:
        {
            // Fondo de mina
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 36, 28, 26, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 14, Color{ 58, 48, 44, 255 });
            // Vigas de madera
            DrawRectangle((int)(cx - 52), (int)(cy - 30), 104, 4, Color{ 120, 82, 46, 255 });
            DrawRectangle((int)(cx - 40), (int)(cy - 42), 5, 14, Color{ 120, 82, 46, 255 });
            DrawRectangle((int)(cx + 35), (int)(cy - 42), 5, 14, Color{ 120, 82, 46, 255 });
            // Mitades de equipo
            DrawRectangle((int)(cx - 52), (int)(cy + 12), 48, 30, Color{ 110, 52, 56, 255 });
            DrawRectangle((int)(cx + 4), (int)(cy + 12), 48, 30, Color{ 48, 84, 118, 255 });
            // Riel central y travesanos
            DrawRectangle((int)(cx - 4), (int)(cy + 12), 8, 30, Color{ 42, 36, 32, 255 });
            for (int k = 0; k < 5; k++)
            {
                DrawRectangle((int)(cx - 4), (int)(cy + 14 + k * 6), 8, 2, Color{ 120, 82, 46, 255 });
            }
            DrawLineEx({ cx - 2.0f, cy + 12.0f }, { cx - 2.0f, cy + 42.0f }, 1.5f, Color{ 160, 164, 172, 255 });
            DrawLineEx({ cx + 2.0f, cy + 12.0f }, { cx + 2.0f, cy + 42.0f }, 1.5f, Color{ 160, 164, 172, 255 });
            // Vagoneta
            DrawRectangle((int)(cx - 6), (int)(cy + 24), 12, 8, Color{ 128, 92, 56, 255 });
            DrawCircle((int)(cx - 3), (int)(cy + 33), 2.0f, Color{ 30, 30, 34, 255 });
            DrawCircle((int)(cx + 3), (int)(cy + 33), 2.0f, Color{ 30, 30, 34, 255 });
            // Geodas flotantes
            DrawRectangle((int)(cx - 40), (int)(cy - 14), 14, 14, Color{ 112, 88, 142, 255 });
            DrawRectangleLines((int)(cx - 40), (int)(cy - 14), 14, 14, Color{ 90, 220, 255, 255 });
            DrawRectangle((int)(cx + 26), (int)(cy - 14), 14, 14, Color{ 112, 88, 142, 255 });
            DrawRectangleLines((int)(cx + 26), (int)(cy - 14), 14, 14, Color{ 90, 220, 255, 255 });
            DrawRectangle((int)(cx - 8), (int)(cy - 20), 16, 16, Color{ 112, 88, 142, 255 });
            DrawRectangleLines((int)(cx - 8), (int)(cy - 20), 16, 16, Color{ 255, 200, 70, 255 });
            // Gemas cayendo
            DrawPoly({ cx - 33.0f, cy + 4.0f }, 4, 3.5f, 45.0f, Color{ 80, 170, 255, 255 });
            DrawPoly({ cx + 33.0f, cy + 6.0f }, 4, 3.5f, 45.0f, Color{ 255, 205, 60, 255 });
            DrawPoly({ cx, cy + 2.0f }, 4, 3.0f, 45.0f, Color{ 80, 170, 255, 255 });
            break;
        }

        case MINIJUEGO_CAPSULAS_BARAJADAS:
        {
            // Mesa de laboratorio con tres capsulas, nucleo brillante y brazo robotico.
            DrawRectangle((int)(cx - 50), (int)(cy - 40), 100, 80, Color{ 30, 44, 56, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy - 36), 28, 16, Color{ 10, 40, 36, 255 });
            DrawRectangle((int)(cx - 40), (int)(cy - 30), 4, 6, Color{ 80, 255, 170, 255 });
            DrawRectangle((int)(cx - 32), (int)(cy - 33), 4, 9, Color{ 80, 255, 170, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy + 14), 92, 6, Color{ 172, 182, 194, 255 });
            DrawRectangle((int)(cx - 42), (int)(cy + 20), 4, 18, Color{ 120, 128, 140, 255 });
            DrawRectangle((int)(cx + 38), (int)(cy + 20), 4, 18, Color{ 120, 128, 140, 255 });
            for (int k = 0; k < 3; k++)
            {
                float x = cx - 28.0f + (float)k * 28.0f;
                DrawRectangle((int)(x - 11), (int)(cy - 12), 22, 26, Color{ 190, 198, 208, 255 });
                DrawRectangle((int)(x - 12), (int)(cy - 2), 24, 6,
                    k == 0 ? Color{ 235, 90, 90, 255 } : (k == 1 ? Color{ 90, 200, 120, 255 } : Color{ 90, 140, 235, 255 }));
                DrawRectangle((int)(x - 9), (int)(cy - 17), 18, 5, Color{ 150, 160, 174, 255 });
            }
            // brazo robotico sosteniendo el nucleo sobre la capsula central
            DrawLineEx(Vector2{ cx + 40, cy - 38 }, Vector2{ cx + 22, cy - 34 }, 4.0f, Color{ 240, 140, 40, 255 });
            DrawLineEx(Vector2{ cx + 22, cy - 34 }, Vector2{ cx, cy - 26 }, 3.0f, Color{ 240, 140, 40, 255 });
            DrawCircle((int)cx, (int)(cy - 24), 5, Color{ 120, 255, 170, 255 });
            DrawCircleLines((int)cx, (int)(cy - 24), 8, Color{ 160, 255, 210, 255 });
            // marcador de jugador
            DrawTriangle(Vector2{ cx - 36, cy - 6 }, Vector2{ cx - 20, cy - 6 }, Vector2{ cx - 28, cy + 4 }, Color{ 246, 206, 52, 255 });
            break;
        }

        case MINIJUEGO_BATEO_METEORICO:
        {
            int x0 = (int)cx - 52;
            int y0 = (int)cy - 42;
            DrawRectangle(x0, y0, 104, 84, Color{ 10, 14, 40, 255 });
            // Estrellas
            DrawCircle(x0 + 10, y0 + 8, 1.5f, RAYWHITE);
            DrawCircle(x0 + 30, y0 + 16, 1.2f, RAYWHITE);
            DrawCircle(x0 + 62, y0 + 6, 1.5f, RAYWHITE);
            DrawCircle(x0 + 94, y0 + 20, 1.2f, RAYWHITE);
            // Planeta con anillo
            DrawCircle(x0 + 80, y0 + 14, 8.0f, Color{ 214, 150, 96, 255 });
            DrawLineEx({ (float)x0 + 66.0f, (float)y0 + 18.0f }, { (float)x0 + 96.0f, (float)y0 + 10.0f }, 2.0f, Color{ 220, 200, 160, 255 });
            // Suelo y anillos de puntuacion
            DrawRectangle(x0, y0 + 60, 104, 24, Color{ 44, 50, 74, 255 });
            DrawRectangle(x0 + 30, y0 + 62, 44, 8, Color{ 52, 96, 170, 255 });
            DrawRectangle(x0 + 40, y0 + 63, 24, 6, Color{ 150, 90, 210, 255 });
            DrawRectangle(x0 + 47, y0 + 64, 10, 4, Color{ 255, 210, 80, 255 });
            // Trayectoria del meteorito golpeado
            for (int k = 0; k < 6; k++)
            {
                float s = (float)k / 5.0f;
                DrawCircle((int)(x0 + 20 + 32 * s), (int)(y0 + 74 - 60 * 4.0f * s * (1.0f - s)), 2.0f + 2.0f * (1.0f - s), Fade(Color{ 255, 170, 70, 255 }, 0.35f + 0.13f * (float)k));
            }
            DrawCircle(x0 + 52, y0 + 66, 5.0f, GOLD);
            // Bate
            DrawLineEx({ (float)x0 + 14.0f, (float)y0 + 80.0f }, { (float)x0 + 30.0f, (float)y0 + 62.0f }, 5.0f, Color{ 120, 80, 50, 255 });
            DrawCircle(x0 + 12, y0 + 78, 3.0f, Color{ 80, 140, 240, 255 });
            break;
        }

        case MINIJUEGO_RACIMO_TOXICO:
        {
            DrawRectangle((int)cx - 52, (int)cy - 42, 104, 84, Color{ 14, 34, 26, 255 });
            DrawRectangle((int)cx - 52, (int)cy + 22, 104, 20, Color{ 36, 78, 52, 255 });
            DrawLineEx({ cx - 30.0f, cy - 40.0f }, { cx + 6.0f, cy - 34.0f }, 5.0f, Color{ 78, 60, 44, 255 });
            DrawLineEx({ cx, cy - 36.0f }, { cx, cy + 8.0f }, 3.0f, Color{ 60, 130, 64, 255 });
            for (int k = 0; k < 6; k++)
            {
                float fy = cy - 28.0f + 7.0f * (float)k;
                float fx = cx + (k % 2 == 0 ? -6.0f : 6.0f);
                Color c = Color{ 176, 206, 74, 255 };
                if (k == 2) c = Color{ 150, 50, 190, 255 };
                if (k == 4) c = Color{ 255, 212, 50, 255 };
                DrawCircle((int)fx, (int)fy, 4.0f, c);
            }
            for (int k = 0; k < 3; k++)
            {
                DrawRectangle((int)cx - 44 + k * 32, (int)cy + 26, 24, 7, Color{ 120, 88, 56, 255 });
                DrawCircle((int)cx - 32 + k * 32, (int)cy + 19, 5.0f, Color{ 230, 200, 120, 255 });
            }
            DrawCircle((int)cx + 34, (int)cy - 18, 2.0f, Color{ 220, 255, 120, 255 });
            DrawCircle((int)cx - 38, (int)cy - 6, 2.0f, Color{ 220, 255, 120, 255 });
            break;
        }

        case MINIJUEGO_TESORERO_ACORRALADO:
        {
            // Foso y patio
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 38, 92, 140, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy - 28), 92, 66, Color{ 124, 120, 114, 255 });
            // Muro del fondo con almenas
            DrawRectangle((int)(cx - 46), (int)(cy - 34), 92, 8, Color{ 150, 146, 138, 255 });
            for (int k = 0; k < 7; k++)
            {
                DrawRectangle((int)(cx - 46 + k * 14), (int)(cy - 39), 8, 5, Color{ 110, 106, 100, 255 });
            }
            // Torres de esquina
            DrawRectangle((int)(cx - 52), (int)(cy - 40), 10, 16, Color{ 150, 146, 138, 255 });
            DrawRectangle((int)(cx + 42), (int)(cy - 40), 10, 16, Color{ 150, 146, 138, 255 });
            DrawTriangle({ cx - 52.0f, cy - 40.0f }, { cx - 42.0f, cy - 40.0f }, { cx - 47.0f, cy - 45.0f }, Color{ 190, 60, 52, 255 });
            DrawTriangle({ cx + 42.0f, cy - 40.0f }, { cx + 52.0f, cy - 40.0f }, { cx + 47.0f, cy - 45.0f }, Color{ 190, 60, 52, 255 });
            // Torre del homenaje central
            DrawRectangle((int)(cx - 9), (int)(cy - 10), 18, 18, Color{ 160, 154, 146, 255 });
            DrawRectangleLines((int)(cx - 9), (int)(cy - 10), 18, 18, Color{ 70, 66, 62, 255 });
            DrawRectangle((int)(cx - 1), (int)(cy - 20), 2, 10, Color{ 110, 74, 42, 255 });
            DrawRectangle((int)(cx + 1), (int)(cy - 20), 7, 4, Color{ 255, 205, 60, 255 });
            // Rejas levadizas
            for (int k = 0; k < 4; k++)
            {
                DrawLineEx({ cx - 26.0f, cy - 6.0f + k * 4.0f }, { cx - 26.0f, cy - 4.0f + k * 4.0f }, 2.0f, Color{ 60, 60, 66, 255 });
                DrawLineEx({ cx + 26.0f, cy - 6.0f + k * 4.0f }, { cx + 26.0f, cy - 4.0f + k * 4.0f }, 2.0f, Color{ 60, 60, 66, 255 });
            }
            DrawRectangle((int)(cx - 14), (int)(cy + 20), 28, 3, Color{ 230, 60, 50, 255 });
            // Tesorero con bolsa de oro huyendo
            DrawRectangle((int)(cx + 22), (int)(cy + 8), 8, 12, Color{ 255, 205, 60, 255 });
            DrawCircle((int)(cx + 26), (int)(cy + 4), 4.0f, Color{ 255, 220, 90, 255 });
            // Trio persiguiendo
            DrawRectangle((int)(cx - 34), (int)(cy + 4), 7, 11, Color{ 238, 55, 66, 255 });
            DrawRectangle((int)(cx - 34), (int)(cy + 18), 7, 11, Color{ 238, 55, 66, 255 });
            DrawRectangle((int)(cx - 22), (int)(cy + 10), 7, 11, Color{ 238, 55, 66, 255 });
            // Monedas sueltas
            DrawCircle((int)(cx + 12), (int)(cy + 24), 3.0f, Color{ 255, 210, 50, 255 });
            DrawCircle((int)(cx + 36), (int)(cy + 22), 3.0f, Color{ 255, 210, 50, 255 });
            DrawCircle((int)(cx + 18), (int)(cy + 30), 3.0f, Color{ 255, 210, 50, 255 });
            break;
        }

        case MINIJUEGO_DESCENSO_NUBES:
        {
            // Cielo con nubes, anillos, tormenta, planeador y arcoiris.
            DrawRectangle((int)(cx - 50), (int)(cy - 40), 100, 80, Color{ 120, 190, 245, 255 });
            DrawRectangle((int)(cx - 50), (int)(cy + 22), 100, 18, Color{ 240, 246, 255, 255 });
            DrawCircle((int)(cx - 30), (int)(cy + 26), 12, Color{ 250, 250, 255, 255 });
            DrawCircle((int)(cx + 8), (int)(cy + 28), 14, Color{ 250, 250, 255, 255 });
            DrawCircle((int)(cx + 38), (int)(cy + 25), 11, Color{ 250, 250, 255, 255 });
            // arcoiris
            for (int b = 0; b < 4; b++)
            {
                DrawRing(Vector2{ cx + 28, cy - 16 }, 20.0f - (float)b * 3.0f, 23.0f - (float)b * 3.0f, 180.0f, 360.0f, 12,
                    b == 0 ? Color{ 235, 70, 70, 255 } : (b == 1 ? Color{ 250, 200, 60, 255 } : (b == 2 ? Color{ 90, 200, 110, 255 } : Color{ 90, 140, 235, 255 })));
            }
            // anillos en la caida
            DrawRing({ cx - 14.0f, cy - 8.0f }, 7.0f, 10.0f, 0.0f, 360.0f, 16, Color{ 255, 205, 40, 255 });
            DrawRing({ cx - 4.0f, cy + 12.0f }, 5.5f, 8.0f, 0.0f, 360.0f, 16, Color{ 255, 255, 255, 255 });
            // nube de tormenta
            DrawCircle((int)(cx + 30), (int)(cy + 4), 8, Color{ 66, 68, 92, 255 });
            DrawCircle((int)(cx + 38), (int)(cy + 6), 6, Color{ 80, 82, 108, 255 });
            DrawLineEx(Vector2{ cx + 31, cy + 9 }, Vector2{ cx + 34, cy + 15 }, 2.0f, YELLOW);
            // planeador con jugador
            DrawRectangle((int)(cx - 40), (int)(cy - 32), 26, 4, Color{ 232, 62, 62, 255 });
            DrawLineEx(Vector2{ cx - 34, cy - 28 }, Vector2{ cx - 27, cy - 18 }, 1.0f, LIGHTGRAY);
            DrawLineEx(Vector2{ cx - 20, cy - 28 }, Vector2{ cx - 27, cy - 18 }, 1.0f, LIGHTGRAY);
            DrawRectangle((int)(cx - 31), (int)(cy - 18), 8, 11, Color{ 250, 240, 220, 255 });
            break;
        }

        case MINIJUEGO_VOLEA_MAGMA:
        {
            int x0 = (int)cx - 52;
            int y0 = (int)cy - 42;
            DrawRectangle(x0, y0, 104, 84, Color{ 40, 16, 16, 255 });
            // Volcan humeante al fondo
            DrawTriangle({ (float)x0 + 8.0f, (float)y0 + 48.0f }, { (float)x0 + 40.0f, (float)y0 + 48.0f }, { (float)x0 + 24.0f, (float)y0 + 20.0f }, Color{ 74, 52, 48, 255 });
            DrawCircle(x0 + 24, y0 + 20, 3.0f, Color{ 255, 130, 40, 255 });
            DrawCircle(x0 + 28, y0 + 12, 4.0f, Fade(LIGHTGRAY, 0.45f));
            DrawCircle(x0 + 33, y0 + 6, 5.0f, Fade(LIGHTGRAY, 0.3f));
            // Lago de lava
            DrawRectangle(x0, y0 + 70, 104, 14, Color{ 255, 90, 24, 255 });
            // Cancha de obsidiana con lineas
            DrawRectangle(x0 + 6, y0 + 52, 92, 18, Color{ 46, 40, 58, 255 });
            DrawRectangle(x0 + 6, y0 + 52, 92, 2, Color{ 255, 130, 50, 255 });
            // Red de cadenas
            DrawRectangle(x0 + 50, y0 + 34, 4, 28, Color{ 22, 20, 28, 255 });
            DrawLineEx({ (float)x0 + 52.0f, (float)y0 + 36.0f }, { (float)x0 + 52.0f, (float)y0 + 58.0f }, 2.0f, Color{ 255, 150, 50, 255 });
            DrawRectangle(x0 + 46, y0 + 34, 12, 2, Color{ 255, 210, 110, 255 });
            // Jugadores de ambos equipos
            DrawRectangle(x0 + 18, y0 + 40, 9, 16, Color{ 255, 150, 40, 255 });
            DrawCircle(x0 + 22, y0 + 37, 5.0f, Color{ 255, 190, 120, 255 });
            DrawRectangle(x0 + 78, y0 + 40, 9, 16, Color{ 70, 200, 255, 255 });
            DrawCircle(x0 + 82, y0 + 37, 5.0f, Color{ 160, 225, 255, 255 });
            // Roca de magma con estela
            DrawCircle(x0 + 40, y0 + 20, 2.0f, Fade(Color{ 255, 150, 50, 255 }, 0.35f));
            DrawCircle(x0 + 46, y0 + 15, 3.0f, Fade(Color{ 255, 150, 50, 255 }, 0.55f));
            DrawCircle(x0 + 54, y0 + 12, 6.0f, Color{ 255, 200, 70, 255 });
            DrawCircle(x0 + 54, y0 + 12, 9.0f, Fade(Color{ 255, 140, 40, 255 }, 0.3f));
            DrawCircle(x0 + 54, y0 + 60, 3.0f, Fade(BLACK, 0.5f));
            break;
        }

        case MINIJUEGO_PAREJAS_GLACIAR:
        {
            DrawRectangle((int)cx - 52, (int)cy - 42, 104, 84, Color{ 10, 20, 48, 255 });
            DrawTriangle({ cx - 52.0f, cy - 10.0f }, { cx - 30.0f, cy - 38.0f }, { cx - 8.0f, cy - 10.0f }, Color{ 90, 120, 170, 255 });
            DrawTriangle({ cx - 20.0f, cy - 10.0f }, { cx + 6.0f, cy - 34.0f }, { cx + 30.0f, cy - 10.0f }, Color{ 110, 140, 190, 255 });
            DrawRectangle((int)cx - 40, (int)cy - 40, 30, 4, Color{ 80, 255, 160, 120 });
            DrawRectangle((int)cx + 6, (int)cy - 42, 36, 4, Color{ 200, 90, 240, 120 });
            Color simbolos[4] = { Color{ 230, 70, 70, 255 }, Color{ 70, 110, 240, 255 }, Color{ 245, 210, 60, 255 }, Color{ 120, 255, 170, 255 } };
            for (int f = 0; f < 3; f++)
            {
                for (int c = 0; c < 4; c++)
                {
                    int bx = (int)cx - 42 + c * 22;
                    int by = (int)cy - 8 + f * 17;
                    DrawRectangle(bx, by, 18, 14, Color{ 170, 218, 246, 255 });
                    DrawRectangleLines(bx, by, 18, 14, Color{ 90, 140, 200, 255 });
                    if ((f + c) % 3 == 0)
                    {
                        DrawCircle(bx + 9, by + 7, 4.0f, simbolos[(f * 4 + c) % 4]);
                    }
                }
            }
            break;
        }

        case MINIJUEGO_ESFERAS_CANON:
        {
            // Cielo y paredes del canon
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 236, 176, 124, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 22, 84, Color{ 150, 70, 48, 255 });
            DrawRectangle((int)(cx + 30), (int)(cy - 42), 22, 84, Color{ 150, 70, 48, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy - 20), 22, 8, Color{ 190, 100, 58, 255 });
            DrawRectangle((int)(cx + 30), (int)(cy + 10), 22, 8, Color{ 190, 100, 58, 255 });
            // Pista serpenteante (lecho seco)
            DrawLineEx({ cx + 6.0f, cy - 40.0f }, { cx - 12.0f, cy - 20.0f }, 22.0f, Color{ 196, 122, 80, 255 });
            DrawLineEx({ cx - 12.0f, cy - 20.0f }, { cx + 12.0f, cy }, 22.0f, Color{ 196, 122, 80, 255 });
            DrawLineEx({ cx + 12.0f, cy }, { cx - 6.0f, cy + 22.0f }, 22.0f, Color{ 196, 122, 80, 255 });
            DrawLineEx({ cx - 6.0f, cy + 22.0f }, { cx, cy + 40.0f }, 22.0f, Color{ 196, 122, 80, 255 });
            // Arena suelta y grieta
            DrawCircle((int)(cx + 12), (int)(cy - 4), 7.0f, Color{ 238, 214, 150, 255 });
            DrawRectangle((int)(cx - 14), (int)(cy - 28), 16, 4, Color{ 18, 10, 10, 255 });
            // Cactus y roca
            DrawRectangle((int)(cx - 20), (int)(cy + 8), 3, 9, Color{ 62, 150, 78, 255 });
            DrawRectangle((int)(cx - 23), (int)(cy + 11), 3, 2, Color{ 62, 150, 78, 255 });
            DrawCircle((int)(cx + 8), (int)(cy + 26), 4.0f, Color{ 126, 70, 54, 255 });
            // Esferas de piedra rodando con sus jugadores
            DrawCircle((int)(cx - 8), (int)(cy - 8), 8.0f, Color{ 150, 134, 120, 255 });
            DrawCircle((int)(cx - 10), (int)(cy - 10), 2.0f, Color{ 84, 72, 64, 255 });
            DrawRectangle((int)(cx - 11), (int)(cy - 20), 6, 8, Color{ 238, 55, 66, 255 });
            DrawCircle((int)(cx + 8), (int)(cy + 8), 8.0f, Color{ 150, 134, 120, 255 });
            DrawCircle((int)(cx + 10), (int)(cy + 10), 2.0f, Color{ 84, 72, 64, 255 });
            DrawRectangle((int)(cx + 5), (int)(cy - 4), 6, 8, Color{ 40, 159, 224, 255 });
            // Meta
            for (int k = 0; k < 6; k++)
            {
                DrawRectangle((int)(cx - 12 + k * 4), (int)(cy + 38), 4, 4, k % 2 == 0 ? RAYWHITE : Color{ 30, 30, 34, 255 });
            }
            break;
        }

        case MINIJUEGO_PESCA_ISLA:
        {
            // Laguna turquesa con muelle, corcho, peces bajo el agua y palmera.
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 238, 222, 170, 255 });
            DrawCircle((int)cx, (int)(cy + 2), 38, Color{ 60, 210, 215, 255 });
            DrawCircle((int)cx, (int)(cy + 2), 30, Color{ 40, 170, 190, 255 });
            // peces: pequeno, dorado y bota
            DrawEllipse((int)(cx - 12), (int)(cy + 10), 7, 4, Color{ 10, 40, 60, 255 });
            DrawTriangle(Vector2{ cx - 5, cy + 10 }, Vector2{ cx + 1, cy + 6 }, Vector2{ cx + 1, cy + 14 }, Color{ 10, 40, 60, 255 });
            DrawEllipse((int)(cx + 16), (int)(cy - 2), 9, 5, Color{ 255, 205, 40, 255 });
            DrawTriangle(Vector2{ cx + 7, cy - 2 }, Vector2{ cx + 1, cy - 7 }, Vector2{ cx + 1, cy + 3 }, Color{ 255, 205, 40, 255 });
            DrawRectangle((int)(cx - 4), (int)(cy + 22), 5, 8, Color{ 120, 80, 50, 255 });
            DrawRectangle((int)(cx - 4), (int)(cy + 28), 10, 4, Color{ 100, 66, 40, 255 });
            // muelle de bambu abajo
            DrawRectangle((int)(cx - 14), (int)(cy + 30), 28, 11, Color{ 214, 180, 90, 255 });
            DrawRectangle((int)(cx - 14), (int)(cy + 34), 28, 2, Color{ 150, 160, 70, 255 });
            // cana, linea y corcho con aviso
            DrawLineEx(Vector2{ cx - 6, cy + 30 }, Vector2{ cx - 14, cy + 6 }, 2.0f, Color{ 120, 84, 50, 255 });
            DrawLineEx(Vector2{ cx - 14, cy + 6 }, Vector2{ cx - 12, cy - 12 }, 1.0f, WHITE);
            DrawCircle((int)(cx - 12), (int)(cy - 12), 4, Color{ 232, 62, 62, 255 });
            DrawRectangle((int)(cx - 13), (int)(cy - 28), 3, 9, Color{ 232, 62, 62, 255 });
            DrawCircle((int)(cx - 12), (int)(cy - 16), 2, Color{ 232, 62, 62, 255 });
            // palmera
            DrawLineEx(Vector2{ cx + 40, cy - 6 }, Vector2{ cx + 36, cy - 30 }, 3.0f, Color{ 130, 92, 56, 255 });
            DrawTriangle(Vector2{ cx + 36, cy - 32 }, Vector2{ cx + 22, cy - 26 }, Vector2{ cx + 34, cy - 24 }, Color{ 50, 160, 70, 255 });
            DrawTriangle(Vector2{ cx + 36, cy - 32 }, Vector2{ cx + 50, cy - 26 }, Vector2{ cx + 38, cy - 24 }, Color{ 50, 160, 70, 255 });
            DrawTriangle(Vector2{ cx + 36, cy - 32 }, Vector2{ cx + 30, cy - 40 }, Vector2{ cx + 42, cy - 40 }, Color{ 60, 180, 80, 255 });
            break;
        }

        case MINIJUEGO_RODILLOS_NEON:
        {
            int x0 = (int)cx - 52;
            int y0 = (int)cy - 42;
            DrawRectangle(x0, y0, 104, 84, Color{ 10, 8, 26, 255 });
            // Rejilla luminosa de fondo
            for (int k = 0; k < 6; k++)
            {
                DrawLine(x0, y0 + 60 + k * 5, x0 + 104, y0 + 60 + k * 5, Color{ 40, 80, 190, 255 });
                DrawLine(x0 + k * 21, y0 + 60, x0 + k * 21, y0 + 84, Color{ 40, 80, 190, 255 });
            }
            // Gabinete arcade con borde neon
            DrawRectangle(x0 + 10, y0 + 6, 84, 66, Color{ 34, 28, 56, 255 });
            DrawRectangleLines(x0 + 10, y0 + 6, 84, 66, Color{ 0, 230, 255, 255 });
            DrawRectangle(x0 + 18, y0 + 18, 68, 34, Color{ 14, 12, 30, 255 });
            // Tres rodillos con simbolos: circulo, circulo y estrella
            DrawCircle(x0 + 30, y0 + 35, 8.0f, Color{ 255, 60, 200, 255 });
            DrawTriangle({ (float)x0 + 52.0f, (float)y0 + 25.0f }, { (float)x0 + 43.0f, (float)y0 + 43.0f }, { (float)x0 + 61.0f, (float)y0 + 43.0f }, Color{ 0, 230, 255, 255 });
            DrawPoly({ (float)x0 + 74.0f, (float)y0 + 35.0f }, 5, 9.0f, -90.0f, Color{ 255, 235, 90, 255 });
            // Linea central de comodin
            DrawRectangle(x0 + 16, y0 + 34, 72, 2, Fade(RAYWHITE, 0.9f));
            DrawTriangle({ (float)x0 + 12.0f, (float)y0 + 29.0f }, { (float)x0 + 12.0f, (float)y0 + 41.0f }, { (float)x0 + 18.0f, (float)y0 + 35.0f }, GOLD);
            // Boton de detener
            DrawCircle(x0 + 52, y0 + 62, 5.0f, Color{ 255, 90, 90, 255 });
            DrawCircle(x0 + 52, y0 + 62, 8.0f, Fade(Color{ 255, 90, 90, 255 }, 0.3f));
            break;
        }

        case MINIJUEGO_BOLAS_AZUCAR:
        {
            DrawRectangle((int)cx - 52, (int)cy - 42, 104, 84, Color{ 255, 214, 230, 255 });
            DrawRectangle((int)cx - 48, (int)cy - 24, 96, 64, Color{ 208, 152, 92, 255 });
            DrawRectangle((int)cx - 46, (int)cy - 8, 36, 40, Color{ 252, 250, 255, 255 });
            DrawCircle((int)cx + 22, (int)cy + 8, 11.0f, Color{ 70, 38, 22, 255 });
            DrawCircle((int)cx + 22, (int)cy + 8, 6.0f, Color{ 108, 62, 40, 255 });
            DrawCircle((int)cx - 28, (int)cy + 12, 9.0f, WHITE);
            DrawCircleLines((int)cx - 28, (int)cy + 12, 9.0f, Color{ 240, 80, 140, 255 });
            DrawCircle((int)cx - 4, (int)cy - 2, 5.0f, WHITE);
            DrawLineEx({ cx - 18.0f, cy + 4.0f }, { cx - 8.0f, cy }, 2.0f, Color{ 240, 80, 140, 255 });
            DrawCircle((int)cx + 30, (int)cy - 14, 7.0f, Color{ 255, 70, 100, 255 });
            DrawCircle((int)cx + 26, (int)cy - 17, 2.0f, Fade(WHITE, 0.7f));
            DrawRectangle((int)cx - 3, (int)cy - 40, 3, 16, WHITE);
            DrawCircle((int)cx - 2, (int)cy - 33, 8.0f, Color{ 240, 80, 140, 255 });
            DrawCircle((int)cx - 2, (int)cy - 33, 4.0f, Fade(WHITE, 0.6f));
            break;
        }

        case MINIJUEGO_GRUA_CHATARRA:
        {
            // Fondo industrial y pozo de chatarra
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 40, 40, 48, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy - 10), 92, 50, Color{ 84, 70, 60, 255 });
            DrawRectangleLines((int)(cx - 46), (int)(cy - 10), 92, 50, Color{ 140, 78, 50, 255 });
            // Engranajes gigantes al fondo
            DrawCircle((int)(cx - 30), (int)(cy - 26), 13.0f, Color{ 110, 82, 54, 255 });
            DrawCircle((int)(cx - 30), (int)(cy - 26), 4.0f, Color{ 52, 54, 62, 255 });
            DrawCircle((int)(cx + 30), (int)(cy - 28), 10.0f, Color{ 110, 82, 54, 255 });
            DrawCircle((int)(cx + 30), (int)(cy - 28), 3.0f, Color{ 52, 54, 62, 255 });
            for (int k = 0; k < 8; k++)
            {
                float a = (float)k * 0.7853982f;
                DrawRectangle((int)(cx - 30 + std::cos(a) * 14.0f) - 2, (int)(cy - 26 + std::sin(a) * 14.0f) - 2, 4, 4, Color{ 110, 82, 54, 255 });
            }
            // Tolvas de colores en el borde
            DrawRectangle((int)(cx - 40), (int)(cy - 14), 14, 8, Color{ 238, 55, 66, 255 });
            DrawRectangle((int)(cx - 20), (int)(cy - 14), 14, 8, Color{ 40, 159, 224, 255 });
            DrawRectangle((int)(cx + 6), (int)(cy - 14), 14, 8, Color{ 80, 200, 90, 255 });
            DrawRectangle((int)(cx + 26), (int)(cy - 14), 14, 8, Color{ 240, 200, 60, 255 });
            // Objetos: tuercas, engranaje, motor y bateria dorada
            DrawPoly({ cx - 28.0f, cy + 10.0f }, 6, 5.0f, 0.0f, Color{ 170, 176, 186, 255 });
            DrawPoly({ cx + 22.0f, cy + 24.0f }, 6, 5.0f, 0.0f, Color{ 170, 176, 186, 255 });
            DrawCircle((int)(cx + 4), (int)(cy + 30), 6.0f, Color{ 224, 140, 52, 255 });
            DrawRectangle((int)(cx + 24), (int)(cy + 6), 12, 9, Color{ 90, 110, 140, 255 });
            DrawRectangle((int)(cx - 14), (int)(cy + 16), 8, 11, Color{ 255, 205, 60, 255 });
            // Garra magnetica con cable bajando sobre la bateria
            DrawLineEx({ cx - 10.0f, cy - 40.0f }, { cx - 10.0f, cy + 4.0f }, 1.5f, Color{ 150, 150, 160, 255 });
            DrawRectangle((int)(cx - 17), (int)(cy + 2), 14, 6, Color{ 238, 55, 66, 255 });
            DrawLineEx({ cx - 16.0f, cy + 8.0f }, { cx - 13.0f, cy + 15.0f }, 2.0f, Color{ 190, 190, 200, 255 });
            DrawLineEx({ cx - 4.0f, cy + 8.0f }, { cx - 7.0f, cy + 15.0f }, 2.0f, Color{ 190, 190, 200, 255 });
            // Segunda garra
            DrawLineEx({ cx + 12.0f, cy - 40.0f }, { cx + 12.0f, cy - 4.0f }, 1.5f, Color{ 150, 150, 160, 255 });
            DrawRectangle((int)(cx + 6), (int)(cy - 6), 12, 5, Color{ 40, 159, 224, 255 });
            break;
        }

        case MINIJUEGO_PISOTON_PLAGAS:
        {
            // Cielo y cesped
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 90, 160, 70, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 16, Color{ 150, 205, 235, 255 });
            // Cerca de madera
            DrawRectangle((int)(cx - 52), (int)(cy - 30), 104, 3, Color{ 190, 138, 84, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy - 24), 104, 3, Color{ 190, 138, 84, 255 });
            for (int k = 0; k < 9; k++)
            {
                DrawRectangle((int)(cx - 50 + k * 12), (int)(cy - 34), 4, 14, Color{ 160, 112, 66, 255 });
            }
            // Brizna de pasto gigante y flor al fondo
            DrawTriangle({ cx - 46.0f, cy - 26.0f }, { cx - 40.0f, cy - 26.0f }, { cx - 43.0f, cy - 42.0f }, Color{ 50, 130, 50, 255 });
            DrawRectangle((int)(cx + 36), (int)(cy - 38), 2, 12, Color{ 60, 140, 52, 255 });
            DrawCircle((int)(cx + 37), (int)(cy - 37), 5.0f, Color{ 240, 90, 130, 255 });
            DrawCircle((int)(cx + 37), (int)(cy - 37), 2.0f, Color{ 250, 200, 60, 255 });
            // Madriguera
            DrawCircle((int)(cx - 30), (int)(cy - 12), 8.0f, Color{ 118, 82, 50, 255 });
            DrawCircle((int)(cx - 30), (int)(cy - 12), 4.0f, Color{ 40, 26, 18, 255 });
            // Escarabajo aplastado por un pisoton (onda de choque)
            DrawCircle((int)(cx - 4), (int)(cy + 6), 10.0f, Color{ 150, 38, 30, 255 });
            DrawCircle((int)(cx - 4), (int)(cy + 6), 4.0f, Color{ 30, 20, 20, 255 });
            DrawCircleLines((int)(cx - 4), (int)(cy + 6), 17.0f, Color{ 255, 255, 255, 255 });
            DrawCircleLines((int)(cx - 4), (int)(cy + 6), 22.0f, Color{ 255, 255, 255, 140 });
            // Babosa dorada
            DrawCircle((int)(cx + 26), (int)(cy + 24), 6.0f, Color{ 255, 205, 40, 255 });
            DrawCircle((int)(cx + 17), (int)(cy + 26), 5.0f, Color{ 240, 180, 30, 255 });
            DrawLineEx({ cx + 29.0f, cy + 20.0f }, { cx + 32.0f, cy + 13.0f }, 1.5f, Color{ 200, 140, 20, 255 });
            DrawCircle((int)(cx + 32), (int)(cy + 12), 2.0f, BLACK);
            // Oruga
            DrawCircle((int)(cx - 36), (int)(cy + 28), 5.0f, Color{ 120, 200, 70, 255 });
            DrawCircle((int)(cx - 28), (int)(cy + 29), 5.0f, Color{ 90, 170, 56, 255 });
            DrawCircle((int)(cx - 20), (int)(cy + 28), 5.0f, Color{ 120, 200, 70, 255 });
            // Avispa
            DrawCircle((int)(cx + 24), (int)(cy - 6), 5.0f, Color{ 255, 214, 40, 255 });
            DrawRectangle((int)(cx + 22), (int)(cy - 11), 2, 10, Color{ 30, 24, 20, 255 });
            DrawCircle((int)(cx + 18), (int)(cy - 6), 3.0f, Color{ 30, 24, 20, 255 });
            DrawCircle((int)(cx + 22), (int)(cy - 12), 4.0f, Color{ 220, 240, 255, 200 });
            break;
        }

        case MINIJUEGO_BALSAS_RAPIDO:
        {
            // Rio turquesa con isla central y una balsa con remos.
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 30, 90, 50, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 20, 84, Color{ 24, 70, 40, 255 });
            DrawRectangle((int)(cx + 32), (int)(cy - 42), 20, 84, Color{ 24, 70, 40, 255 });
            DrawRectangle((int)(cx - 32), (int)(cy - 42), 64, 84, Color{ 40, 190, 190, 255 });
            DrawRectangle((int)(cx - 6), (int)(cy - 42), 12, 84, Color{ 80, 140, 60, 255 });
            for (int k = 0; k < 4; k++)
            {
                DrawRectangle((int)(cx - 26 + k * 6), (int)(cy - 34 + k * 17), 12, 2, Color{ 220, 250, 250, 255 });
                DrawRectangle((int)(cx + 12 - k * 5), (int)(cy - 26 + k * 16), 12, 2, Color{ 220, 250, 250, 255 });
            }
            // Roca y banana.
            DrawCircle((int)(cx + 20), (int)(cy - 22), 6.0f, Color{ 120, 122, 128, 255 });
            DrawRectangle((int)(cx - 28), (int)(cy - 14), 8, 3, Color{ 255, 220, 40, 255 });
            // Balsa de troncos con remos.
            for (int k = 0; k < 4; k++)
            {
                DrawRectangle((int)(cx - 20 + k * 5), (int)(cy + 4), 4, 24, Color{ 160, 108, 58, 255 });
            }
            DrawLineEx({ cx - 22.0f, cy + 14.0f }, { cx - 34.0f, cy + 22.0f }, 2.0f, Color{ 120, 84, 48, 255 });
            DrawLineEx({ cx + 2.0f, cy + 14.0f }, { cx + 14.0f, cy + 22.0f }, 2.0f, Color{ 120, 84, 48, 255 });
            DrawRectangle((int)(cx - 38), (int)(cy + 21), 6, 3, Color{ 235, 215, 150, 255 });
            DrawRectangle((int)(cx + 12), (int)(cy + 21), 6, 3, Color{ 235, 215, 150, 255 });
            DrawRectangle((int)(cx - 9), (int)(cy - 2), 2, 12, Color{ 90, 60, 36, 255 });
            DrawTriangle({ cx - 7.0f, cy - 2.0f }, { cx - 7.0f, cy + 4.0f }, { cx + 3.0f, cy + 1.0f }, Color{ 255, 150, 40, 255 });
            break;
        }

        case MINIJUEGO_AUTOS_GLOBO:
        {
            // Cielo nocturno y rascacielos
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 20, 24, 56, 255 });
            DrawRectangle((int)(cx - 50), (int)(cy - 40), 14, 26, Color{ 50, 100, 150, 255 });
            DrawRectangle((int)(cx - 32), (int)(cy - 42), 12, 34, Color{ 40, 80, 130, 255 });
            DrawRectangle((int)(cx + 24), (int)(cy - 40), 14, 30, Color{ 50, 100, 150, 255 });
            DrawRectangle((int)(cx + 40), (int)(cy - 42), 12, 22, Color{ 40, 80, 130, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy - 34), 6, 2, Color{ 255, 240, 160, 255 });
            DrawRectangle((int)(cx + 28), (int)(cy - 30), 6, 2, Color{ 255, 240, 160, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy - 20), 104, 2, Color{ 80, 220, 255, 255 });
            // Plaza con barrera de energia
            DrawRectangle((int)(cx - 48), (int)(cy - 16), 96, 54, Color{ 40, 48, 80, 255 });
            DrawRectangleLines((int)(cx - 48), (int)(cy - 16), 96, 54, Color{ 255, 90, 200, 255 });
            DrawCircleLines((int)cx, (int)(cy + 11), 14.0f, Color{ 80, 220, 255, 255 });
            // Placa de carga
            DrawCircle((int)(cx + 30), (int)(cy - 4), 6.0f, Color{ 90, 255, 160, 255 });
            // Auto rojo con globos
            DrawRectangle((int)(cx - 38), (int)(cy + 14), 24, 12, Color{ 238, 55, 66, 255 });
            DrawRectangle((int)(cx - 16), (int)(cy + 17), 5, 6, Color{ 230, 235, 245, 255 });
            DrawLineEx({ cx - 36.0f, cy + 14.0f }, { cx - 40.0f, cy + 4.0f }, 1.0f, WHITE);
            DrawCircle((int)(cx - 41), (int)(cy + 1), 5.0f, Color{ 238, 55, 66, 255 });
            DrawCircle((int)(cx - 31), (int)(cy - 1), 5.0f, Color{ 238, 55, 66, 255 });
            // Auto azul embistiendo de lado
            DrawRectangle((int)(cx + 10), (int)(cy + 2), 12, 22, Color{ 60, 130, 240, 255 });
            DrawRectangle((int)(cx + 12), (int)(cy + 24), 8, 4, Color{ 230, 235, 245, 255 });
            DrawCircle((int)(cx + 28), (int)(cy + 10), 5.0f, Color{ 60, 130, 240, 255 });
            // Chispas del choque
            DrawLineEx({ cx - 8.0f, cy + 20.0f }, { cx + 2.0f, cy + 14.0f }, 2.0f, Color{ 255, 240, 120, 255 });
            DrawLineEx({ cx - 8.0f, cy + 14.0f }, { cx + 2.0f, cy + 20.0f }, 2.0f, Color{ 255, 240, 120, 255 });
            // Globo reventado
            DrawCircleLines((int)(cx - 20), (int)(cy - 6), 7.0f, Color{ 255, 255, 255, 255 });
            break;
        }

        case MINIJUEGO_SENDERO_INVISIBLE:
        {
            // Cielo nocturno con luna y una cuadricula de losas con una grieta.
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 14, 16, 36, 255 });
            DrawCircle((int)(cx + 30), (int)(cy - 26), 11.0f, Color{ 150, 170, 230, 90 });
            DrawCircle((int)(cx + 30), (int)(cy - 26), 8.0f, Color{ 238, 238, 214, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy - 8), 14, 18, Color{ 80, 84, 98, 255 });
            DrawCircle((int)(cx - 39), (int)(cy - 8), 7.0f, Color{ 80, 84, 98, 255 });
            DrawRectangle((int)(cx - 20), (int)(cy - 14), 16, 5, Color{ 22, 24, 30, 255 });
            for (int fila = 0; fila < 3; fila++)
            {
                for (int col = 0; col < 4; col++)
                {
                    Color losa = (fila + col) % 2 == 0 ? Color{ 86, 90, 104, 255 } : Color{ 74, 78, 92, 255 };
                    if (fila == 1 && col == 2) losa = Color{ 150, 235, 170, 255 };
                    DrawRectangle((int)(cx - 36 + col * 22), (int)(cy + 4 + fila * 12), 20, 10, losa);
                }
            }
            DrawLineEx({ cx + 12.0f, cy + 18.0f }, { cx + 18.0f, cy + 24.0f }, 2.0f, Color{ 200, 110, 255, 255 });
            DrawCircle((int)(cx - 8), (int)(cy + 14), 3.0f, Color{ 110, 255, 150, 255 });
            DrawCircle((int)(cx - 8), (int)(cy + 14), 6.0f, Color{ 110, 255, 150, 60 });
            DrawRectangle((int)(cx - 52), (int)(cy + 34), 104, 8, Color{ 40, 50, 70, 255 });
            break;
        }

        case MINIJUEGO_BANQUETE_TURBO:
        {
            // Espacio y ventanal con la Tierra
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 8, 10, 28, 255 });
            DrawCircle((int)(cx - 14), (int)(cy - 22), 22.0f, Color{ 28, 86, 190, 255 });
            DrawCircle((int)(cx - 20), (int)(cy - 26), 7.0f, Color{ 70, 150, 70, 255 });
            DrawCircle((int)(cx - 4), (int)(cy - 16), 5.0f, Color{ 70, 150, 70, 255 });
            DrawCircleLines((int)(cx - 14), (int)(cy - 22), 24.0f, Color{ 120, 190, 255, 140 });
            DrawRectangle((int)(cx + 26), (int)(cy - 38), 2, 2, WHITE);
            DrawRectangle((int)(cx + 40), (int)(cy - 28), 2, 2, WHITE);
            DrawRectangle((int)(cx + 18), (int)(cy - 14), 2, 2, WHITE);
            // Marco del ventanal y pared
            DrawRectangleLines((int)(cx - 52), (int)(cy - 42), 104, 46, Color{ 80, 220, 255, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy + 4), 104, 38, Color{ 40, 46, 70, 255 });
            // Robot camarero
            DrawRectangle((int)(cx - 7), (int)(cy - 2), 14, 12, Color{ 225, 230, 240, 255 });
            DrawCircle((int)cx, (int)(cy - 6), 6.0f, Color{ 235, 238, 246, 255 });
            DrawRectangle((int)(cx - 4), (int)(cy - 8), 8, 3, Color{ 24, 30, 48, 255 });
            DrawCircle((int)(cx - 2), (int)(cy - 7), 1.0f, Color{ 120, 255, 220, 255 });
            DrawCircle((int)(cx + 2), (int)(cy - 7), 1.0f, Color{ 120, 255, 220, 255 });
            DrawLineEx({ cx - 7.0f, cy + 2.0f }, { cx - 28.0f, cy + 18.0f }, 2.0f, Color{ 150, 160, 190, 255 });
            DrawLineEx({ cx + 7.0f, cy + 2.0f }, { cx + 28.0f, cy + 18.0f }, 2.0f, Color{ 150, 160, 190, 255 });
            // Mesas magneticas con tubos de comida
            DrawRectangle((int)(cx - 46), (int)(cy + 20), 40, 6, Color{ 70, 80, 118, 255 });
            DrawRectangle((int)(cx + 6), (int)(cy + 20), 40, 6, Color{ 70, 80, 118, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy + 26), 40, 1, Color{ 80, 220, 255, 255 });
            DrawRectangle((int)(cx + 6), (int)(cy + 26), 40, 1, Color{ 80, 220, 255, 255 });
            DrawRectangle((int)(cx - 40), (int)(cy + 14), 28, 6, Color{ 80, 200, 120, 255 });
            DrawCircle((int)(cx - 40), (int)(cy + 17), 3.0f, Color{ 80, 200, 120, 255 });
            DrawRectangle((int)(cx + 12), (int)(cy + 14), 28, 6, Color{ 255, 205, 50, 255 });
            DrawCircle((int)(cx + 12), (int)(cy + 17), 3.0f, Color{ 255, 205, 50, 255 });
            // Racion picante en la cola
            DrawRectangle((int)(cx - 8), (int)(cy + 12), 16, 4, Color{ 230, 50, 40, 255 });
            DrawCircle((int)cx, (int)(cy + 9), 2.0f, Color{ 255, 150, 40, 255 });
            // Jugadores comiendo
            DrawRectangle((int)(cx - 34), (int)(cy + 30), 10, 10, Color{ 238, 55, 66, 255 });
            DrawRectangle((int)(cx + 24), (int)(cy + 30), 10, 10, Color{ 60, 130, 240, 255 });
            break;
        }

        case MINIJUEGO_TUBERIAS_DESIERTO:
        {
            // Pared de arenisca con tuberias cruzadas y cantaros abajo.
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 250, 206, 140, 255 });
            DrawCircle((int)(cx + 36), (int)(cy - 30), 9.0f, Color{ 255, 224, 130, 255 });
            DrawRectangle((int)(cx - 44), (int)(cy - 32), 88, 56, Color{ 214, 178, 118, 255 });
            DrawRectangle((int)(cx - 20), (int)(cy - 40), 40, 8, Color{ 50, 190, 215, 255 });
            DrawLineEx({ cx - 28.0f, cy - 28.0f }, { cx - 28.0f, cy - 14.0f }, 5.0f, Color{ 196, 128, 82, 255 });
            DrawLineEx({ cx - 28.0f, cy - 14.0f }, { cx - 6.0f, cy + 6.0f }, 5.0f, Color{ 196, 128, 82, 255 });
            DrawLineEx({ cx - 6.0f, cy - 28.0f }, { cx - 6.0f, cy - 14.0f }, 5.0f, Color{ 196, 128, 82, 255 });
            DrawLineEx({ cx - 6.0f, cy - 14.0f }, { cx - 28.0f, cy + 6.0f }, 5.0f, Color{ 196, 128, 82, 255 });
            DrawLineEx({ cx + 16.0f, cy - 28.0f }, { cx + 16.0f, cy + 6.0f }, 5.0f, Color{ 196, 128, 82, 255 });
            DrawLineEx({ cx - 28.0f, cy + 6.0f }, { cx - 28.0f, cy + 12.0f }, 5.0f, Color{ 60, 170, 235, 255 });
            DrawRectangle((int)(cx - 44), (int)(cy + 24), 88, 6, Color{ 190, 154, 100, 255 });
            DrawCircle((int)(cx - 28), (int)(cy + 20), 6.0f, Color{ 178, 98, 58, 255 });
            DrawCircle((int)(cx - 6), (int)(cy + 20), 6.0f, Color{ 255, 205, 60, 255 });
            DrawCircle((int)(cx + 16), (int)(cy + 20), 6.0f, Color{ 178, 98, 58, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy + 30), 104, 12, Color{ 226, 190, 128, 255 });
            DrawRectangle((int)(cx + 36), (int)(cy + 8), 5, 22, Color{ 62, 140, 74, 255 });
            break;
        }

        case MINIJUEGO_TREPA_MASTIL:
        {
            // Cielo y mar
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 56, Color{ 110, 170, 225, 255 });
            DrawRectangle((int)(cx - 52), (int)(cy + 4), 104, 38, Color{ 22, 84, 150, 255 });
            DrawCircle((int)(cx + 38), (int)(cy - 30), 7.0f, Color{ 255, 244, 190, 255 });
            DrawRectangle((int)(cx - 46), (int)(cy + 12), 12, 1, Color{ 255, 255, 255, 180 });
            DrawRectangle((int)(cx + 30), (int)(cy + 8), 14, 1, Color{ 255, 255, 255, 180 });
            // Isla lejana
            DrawTriangle({ cx - 52.0f, cy + 4.0f }, { cx - 30.0f, cy + 4.0f }, { cx - 41.0f, cy - 6.0f }, Color{ 214, 190, 120, 255 });
            // Casco y cubierta
            DrawRectangle((int)(cx - 44), (int)(cy + 24), 88, 14, Color{ 92, 58, 34, 255 });
            DrawRectangle((int)(cx - 44), (int)(cy + 22), 88, 3, Color{ 160, 112, 70, 255 });
            // Mastiles con velas
            DrawRectangle((int)(cx - 30), (int)(cy - 26), 18, 30, Color{ 236, 228, 200, 255 });
            DrawRectangle((int)(cx - 30), (int)(cy - 14), 18, 4, Color{ 238, 55, 66, 255 });
            DrawRectangle((int)(cx + 8), (int)(cy - 26), 18, 30, Color{ 236, 228, 200, 255 });
            DrawRectangle((int)(cx + 8), (int)(cy - 14), 18, 4, Color{ 60, 130, 240, 255 });
            DrawRectangle((int)(cx - 22), (int)(cy - 38), 3, 62, Color{ 120, 80, 48, 255 });
            DrawRectangle((int)(cx + 16), (int)(cy - 38), 3, 62, Color{ 120, 80, 48, 255 });
            // Cofas con banderas
            DrawRectangle((int)(cx - 27), (int)(cy - 36), 13, 3, Color{ 140, 96, 58, 255 });
            DrawRectangle((int)(cx + 11), (int)(cy - 36), 13, 3, Color{ 140, 96, 58, 255 });
            DrawTriangle({ cx - 20.0f, cy - 42.0f }, { cx - 20.0f, cy - 36.0f }, { cx - 11.0f, cy - 39.0f }, Color{ 238, 55, 66, 255 });
            DrawTriangle({ cx + 18.0f, cy - 42.0f }, { cx + 18.0f, cy - 36.0f }, { cx + 27.0f, cy - 39.0f }, Color{ 60, 130, 240, 255 });
            // Trepadores
            DrawRectangle((int)(cx - 25), (int)(cy - 6), 8, 10, Color{ 238, 55, 66, 255 });
            DrawRectangle((int)(cx + 13), (int)(cy - 20), 8, 10, Color{ 60, 130, 240, 255 });
            // Jarcias
            DrawLineEx({ cx - 20.0f, cy - 34.0f }, { cx - 40.0f, cy + 22.0f }, 1.0f, Color{ 190, 170, 120, 255 });
            DrawLineEx({ cx + 18.0f, cy - 34.0f }, { cx + 40.0f, cy + 22.0f }, 1.0f, Color{ 190, 170, 120, 255 });
            // Cuervo
            DrawCircle((int)(cx + 36), (int)(cy - 12), 4.0f, Color{ 30, 30, 40, 255 });
            DrawLineEx({ cx + 36.0f, cy - 12.0f }, { cx + 30.0f, cy - 19.0f }, 1.5f, Color{ 30, 30, 40, 255 });
            DrawLineEx({ cx + 36.0f, cy - 12.0f }, { cx + 42.0f, cy - 19.0f }, 1.5f, Color{ 30, 30, 40, 255 });
            DrawTriangle({ cx + 31.0f, cy - 13.0f }, { cx + 31.0f, cy - 10.0f }, { cx + 26.0f, cy - 11.0f }, Color{ 250, 150, 40, 255 });
            break;
        }

        case MINIJUEGO_GUARDIAN_RUINAS:
        {
            // Plaza de ruinas con portal arriba, guardian con escudo y orbes.
            DrawRectangle((int)(cx - 52), (int)(cy - 42), 104, 84, Color{ 30, 70, 46, 255 });
            DrawRectangle((int)(cx - 36), (int)(cy - 30), 72, 68, Color{ 104, 112, 96, 255 });
            DrawRectangle((int)(cx - 22), (int)(cy - 40), 6, 20, Color{ 128, 134, 116, 255 });
            DrawRectangle((int)(cx + 16), (int)(cy - 40), 6, 20, Color{ 128, 134, 116, 255 });
            DrawRectangle((int)(cx - 22), (int)(cy - 42), 44, 6, Color{ 128, 134, 116, 255 });
            DrawRectangle((int)(cx - 16), (int)(cy - 36), 32, 16, Color{ 100, 240, 220, 150 });
            DrawRectangle((int)(cx - 10), (int)(cy - 18), 20, 4, Color{ 120, 220, 255, 255 });
            DrawRectangle((int)(cx - 4), (int)(cy - 22), 8, 4, Color{ 90, 60, 40, 255 });
            DrawRectangle((int)(cx - 24), (int)(cy - 4), 8, 16, Color{ 150, 156, 138, 255 });
            DrawRectangle((int)(cx + 16), (int)(cy - 4), 8, 16, Color{ 150, 156, 138, 255 });
            DrawCircle((int)(cx - 6), (int)(cy - 2), 4.0f, Color{ 190, 250, 255, 255 });
            DrawCircle((int)(cx + 6), (int)(cy + 8), 4.0f, Color{ 255, 170, 70, 255 });
            DrawLineEx({ cx - 20.0f, cy + 30.0f }, { cx - 7.0f, cy + 4.0f }, 1.5f, Color{ 255, 255, 255, 120 });
            DrawRectangle((int)(cx - 20), (int)(cy + 28), 6, 10, Color{ 255, 170, 60, 255 });
            DrawRectangle((int)(cx + 2), (int)(cy + 24), 6, 10, Color{ 255, 170, 60, 255 });
            DrawRectangle((int)(cx + 22), (int)(cy + 28), 6, 10, Color{ 255, 170, 60, 255 });
            break;
        }

        case CANTIDAD_MINIJUEGOS:
            break;
    }

    EndMode2D();
}
