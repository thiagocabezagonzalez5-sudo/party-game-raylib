#include "raylib.h"
#include "Systems/Input.h"


InputJugador LeerInputJugador(
    int jugador
)
{
    InputJugador input{};


    //==================================================
    // JUGADOR 0 - TECLADO
    //==================================================

    if (jugador == 0)
    {
        if (IsKeyDown(KEY_UP))
        {
            input.moverZ -= 1.0f;
        }

        if (IsKeyDown(KEY_DOWN))
        {
            input.moverZ += 1.0f;
        }

        if (IsKeyDown(KEY_LEFT))
        {
            input.moverX -= 1.0f;
        }

        if (IsKeyDown(KEY_RIGHT))
        {
            input.moverX += 1.0f;
        }


        input.saltar =
            IsKeyPressed(KEY_SPACE);


        input.accion =
            IsKeyPressed(KEY_E);


        return input;
    }


    //==================================================
    // GAMEPAD
    //==================================================

    int gamepad =
        jugador - 1;


    if (!IsGamepadAvailable(gamepad))
    {
        return input;
    }


    const float DEADZONE =
        0.2f;


    input.saltar =
        IsGamepadButtonPressed(
            gamepad,
            GAMEPAD_BUTTON_RIGHT_FACE_DOWN
        );


    input.accion =
        IsGamepadButtonPressed(
            gamepad,
            GAMEPAD_BUTTON_RIGHT_FACE_LEFT
        );


    //==================================================
    // EJE X
    //==================================================

    float ejeX =
        GetGamepadAxisMovement(
            gamepad,
            GAMEPAD_AXIS_LEFT_X
        );


    if (
        ejeX > DEADZONE ||
        ejeX < -DEADZONE
    )
    {
        input.moverX =
            ejeX;
    }


    //==================================================
    // EJE Y
    //==================================================

    float ejeY =
        GetGamepadAxisMovement(
            gamepad,
            GAMEPAD_AXIS_LEFT_Y
        );


    if (
        ejeY > DEADZONE ||
        ejeY < -DEADZONE
    )
    {
        input.moverZ =
            ejeY;
    }


    return input;
}