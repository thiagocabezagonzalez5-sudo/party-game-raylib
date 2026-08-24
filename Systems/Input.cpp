#include "raylib.h"
#include "Systems/Input.h"
InputJugador LeerInputJugador(
    int jugador
)
{
    InputJugador input { 0 };

    if(jugador == 0){
        if (IsKeyDown(KEY_UP)) input.moverZ -= 1.0f;
        if (IsKeyDown(KEY_DOWN)) input.moverZ += 1.0f;
        if (IsKeyDown(KEY_LEFT)) input.moverX -= 1.0f;
        if (IsKeyDown(KEY_RIGHT)) input.moverX += 1.0f;

        input.saltar = IsKeyPressed(KEY_SPACE);
        input.accion = IsKeyPressed(KEY_E);

    }
    else{  
        const int DEADZONE = 0.2f;  

        if(!IsGamepadAvailable){
            return input;
        }
        
        int gamepad = jugador - 1;

        input.saltar = IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        input.accion = IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);

        if (GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X) > DEADZONE ||
            GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X) < -DEADZONE
        ) 
        {
            input.moverX = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
        }
        if (GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y) > DEADZONE || 
            GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y) < -DEADZONE
        )
        {
            input.moverZ = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
        }
    }
    return input;
}