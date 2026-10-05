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

        case CANTIDAD_MINIJUEGOS:
            break;
    }

    EndMode2D();
}
