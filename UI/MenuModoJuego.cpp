#include "UI/MenuModoJuego.h"


static Rectangle ObtenerBotonModo(
    int indice
)
{
    const float ancho = 360.0f;
    const float alto = 150.0f;
    const float separacion = 42.0f;

    float total =
        ancho * 2.0f +
        separacion;

    float xInicial =
        GetScreenWidth() / 2.0f -
        total / 2.0f;

    return Rectangle
    {
        xInicial + indice * (ancho + separacion),
        GetScreenHeight() / 2.0f - 55.0f,
        ancho,
        alto
    };
}


static bool ConfirmarConCualquierMando()
{
    for (int i = 0; i < 4; i++)
    {
        if (
            IsGamepadAvailable(i) &&
            IsGamepadButtonPressed(
                i,
                GAMEPAD_BUTTON_RIGHT_FACE_DOWN
            )
        )
        {
            return true;
        }
    }

    return false;
}


static bool CancelarConCualquierMando()
{
    for (int i = 0; i < 4; i++)
    {
        if (
            IsGamepadAvailable(i) &&
            IsGamepadButtonPressed(
                i,
                GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
            )
        )
        {
            return true;
        }
    }

    return false;
}


void MenuModoJuego::Inicializar()
{
    opcionSeleccionada =
        MODO_JUEGO_MINIJUEGOS;

    confirmar = false;
    volver = false;
    alphaEntrada = 0.0f;
}


void MenuModoJuego::Actualizar(
    float deltaTime
)
{
    confirmar = false;
    volver = false;

    alphaEntrada +=
        deltaTime * 4.5f;

    if (alphaEntrada > 1.0f)
    {
        alphaEntrada = 1.0f;
    }

    if (
        IsKeyPressed(KEY_LEFT) ||
        IsKeyPressed(KEY_A)
    )
    {
        opcionSeleccionada =
            MODO_JUEGO_MINIJUEGOS;
    }

    if (
        IsKeyPressed(KEY_RIGHT) ||
        IsKeyPressed(KEY_D)
    )
    {
        opcionSeleccionada =
            MODO_JUEGO_TABLERO;
    }

    Vector2 mouse =
        GetMousePosition();

    for (int i = 0; i < 2; i++)
    {
        Rectangle boton =
            ObtenerBotonModo(i);

        if (CheckCollisionPointRec(mouse, boton))
        {
            opcionSeleccionada = i;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                confirmar = true;
            }
        }
    }

    if (
        IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_SPACE) ||
        ConfirmarConCualquierMando()
    )
    {
        confirmar = true;
    }

    if (
        IsKeyPressed(KEY_ESCAPE) ||
        CancelarConCualquierMando()
    )
    {
        volver = true;
    }
}


void MenuModoJuego::Dibujar() const
{
    Color cristal =
        Fade(
            Color{ 16, 18, 24, 255 },
            0.64f * alphaEntrada
        );

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        cristal
    );

    const char* titulo =
        "ELEGIR MODO";

    DrawText(
        titulo,
        GetScreenWidth() / 2 -
            MeasureText(titulo, 44) / 2,
        90,
        44,
        Fade(RAYWHITE, alphaEntrada)
    );

    const char* nombres[2] =
    {
        "MINIJUEGOS",
        "TABLERO"
    };

    const char* descripciones[2] =
    {
        "Elegi personajes y despues un minijuego",
        "Modo tablero - en construccion"
    };

    for (int i = 0; i < 2; i++)
    {
        Rectangle boton =
            ObtenerBotonModo(i);

        bool seleccionado =
            opcionSeleccionada == i;

        DrawRectangle(
            (int)boton.x,
            (int)boton.y,
            (int)boton.width,
            (int)boton.height,
            Fade(
                seleccionado
                ? Color{ 55, 58, 68, 255 }
                : Color{ 25, 27, 34, 255 },
                0.90f * alphaEntrada
            )
        );

        DrawRectangleLinesEx(
            boton,
            seleccionado ? 5.0f : 2.0f,
            Fade(
                seleccionado ? ORANGE : GRAY,
                alphaEntrada
            )
        );

        int anchoNombre =
            MeasureText(nombres[i], 31);

        DrawText(
            nombres[i],
            (int)(boton.x + boton.width / 2.0f - anchoNombre / 2.0f),
            (int)boton.y + 32,
            31,
            Fade(
                seleccionado ? ORANGE : RAYWHITE,
                alphaEntrada
            )
        );

        int anchoDescripcion =
            MeasureText(descripciones[i], 17);

        DrawText(
            descripciones[i],
            (int)(boton.x + boton.width / 2.0f - anchoDescripcion / 2.0f),
            (int)boton.y + 91,
            17,
            Fade(LIGHTGRAY, alphaEntrada)
        );
    }

    const char* ayuda =
        "A/D O FLECHAS | ENTER/ESPACIO/A CONFIRMAR | ESC/B VOLVER";

    DrawText(
        ayuda,
        GetScreenWidth() / 2 -
            MeasureText(ayuda, 18) / 2,
        GetScreenHeight() - 72,
        18,
        Fade(LIGHTGRAY, alphaEntrada)
    );
}
