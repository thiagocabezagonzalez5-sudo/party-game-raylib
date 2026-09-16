#include "Core/Juego.h"
#include "Editor/EditorMapas.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "raylib.h"


int main()
{
    Juego juego;
    EditorMapas editorMapas;

    bool editorActivo = false;


    juego.Inicializar();
    editorMapas.Inicializar();


    while (
        !WindowShouldClose() &&
        !juego.DebeCerrar()
    )
    {
        float deltaTime =
            GetFrameTime();

        if (deltaTime > DELTA_TIME_MAXIMO)
        {
            deltaTime = DELTA_TIME_MAXIMO;
        }


        //------------------------------
        // CAMBIO JUEGO / EDITOR
        //------------------------------

        bool controlPresionado =
            IsKeyDown(KEY_LEFT_CONTROL) ||
            IsKeyDown(KEY_RIGHT_CONTROL);

        bool cambiarEditor =
            controlPresionado &&
            IsKeyPressed(KEY_E);

        if (cambiarEditor)
        {
            editorActivo = !editorActivo;
        }


        //------------------------------
        // UPDATE
        //------------------------------

        if (!cambiarEditor)
        {
            if (editorActivo)
            {
                editorMapas.Actualizar(
                    deltaTime
                );
            }
            else
            {
                juego.Actualizar(
                    deltaTime
                );
            }
        }


        //------------------------------
        // DRAW
        //------------------------------

        BeginDrawing();


        if (editorActivo)
        {
            editorMapas.Dibujar();
        }
        else
        {
            juego.Dibujar();
        }


        EndDrawing();
    }


    //------------------------------
    // CERRAR
    //------------------------------

    editorMapas.Descargar();
    juego.Descargar();
    DescargarModeloJugadorCompartido();


    CloseWindow();


    return 0;
}
