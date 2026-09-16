#include "Core/Juego.h"
#include "Editor/EditorMapas.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "raylib.h"


static void AplicarZoomEditorConRueda(
    EditorMapas& editor
)
{
    float rueda = GetMouseWheelMove();

    if (rueda == 0.0f)
    {
        return;
    }

    Ray rayo =
        GetScreenToWorldRay(
            GetMousePosition(),
            editor.camara
        );

    float velocidadZoom =
        IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT)
        ? 3.0f
        : 1.45f;

    Vector3 desplazamiento =
    {
        rayo.direction.x * rueda * velocidadZoom,
        rayo.direction.y * rueda * velocidadZoom,
        rayo.direction.z * rueda * velocidadZoom
    };

    editor.camara.position.x += desplazamiento.x;
    editor.camara.position.y += desplazamiento.y;
    editor.camara.position.z += desplazamiento.z;

    editor.camara.target.x += desplazamiento.x;
    editor.camara.target.y += desplazamiento.y;
    editor.camara.target.z += desplazamiento.z;
}


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
                AplicarZoomEditorConRueda(editorMapas);

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
