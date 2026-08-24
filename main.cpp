#include "Core/Juego.h"
#include "raylib.h"


int main()
{
    Juego juego;


    juego.Inicializar();


    while (
        !WindowShouldClose() &&
        !juego.DebeCerrar()
    )
    {
        float deltaTime =
            GetFrameTime();

        if (deltaTime > 0.1f)
        {
            deltaTime = 0.1f;
        }


        //------------------------------
        // UPDATE
        //------------------------------

        juego.Actualizar(
            deltaTime
        );


        //------------------------------
        // DRAW
        //------------------------------

        BeginDrawing();


        juego.Dibujar();


        EndDrawing();
    }


    //------------------------------
    // CERRAR
    //------------------------------

    juego.Descargar();


    CloseWindow();


    return 0;
}