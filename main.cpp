#include "Core/Juego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "raylib.h"
#include "rlgl.h"

#include <cstdlib>
#include <ctime>


//==================================================
// HERRAMIENTAS DE DEPURACION
//==================================================

// Tecla de captura. F12 no sirve: la Zona de Pruebas ya la usa y raylib
// guarda con ella su propia captura junto al ejecutable.
static const int TECLA_CAPTURA = KEY_INSERT;


// Guarda la pantalla actual en build/capturas/ (carpeta del ejecutable)
// con marca de tiempo y resolucion en el nombre. Se llama antes de
// EndDrawing, con el frame ya dibujado.
static void GuardarCapturaDepuracion()
{
    const char* carpeta =
        TextFormat("%scapturas", GetApplicationDirectory());

    if (!DirectoryExists(carpeta))
    {
        MakeDirectory(carpeta);
    }

    char marca[32] = {};
    std::time_t ahora = std::time(nullptr);
    std::strftime(marca, sizeof(marca), "%Y%m%d_%H%M%S", std::localtime(&ahora));

    // TextFormat usa buffers rotativos: las rutas se copian a arrays
    // propios para que no se pisen.
    char base[512] = {};
    TextCopy(base, TextFormat("%s/captura_%s_%dx%d", carpeta, marca, GetRenderWidth(), GetRenderHeight()));

    char ruta[512] = {};
    TextCopy(ruta, TextFormat("%s.png", base));

    for (int n = 2; FileExists(ruta) && n < 100; n++)
    {
        TextCopy(ruta, TextFormat("%s_%d.png", base, n));
    }

    // Lo dibujado en el frame sigue en el lote de rlgl: se vacia antes
    // de leer el framebuffer.
    rlDrawRenderBatchActive();

    Image imagen = LoadImageFromScreen();
    bool guardada = ExportImage(imagen, ruta);
    UnloadImage(imagen);

    TraceLog(
        guardada ? LOG_INFO : LOG_WARNING,
        guardada ? "Captura guardada: %s" : "No se pudo guardar la captura: %s",
        ruta
    );
}


int main(int argc, char* argv[])
{
    Juego juego;


    // --ventana W H: modo ventana con ese tamano, sin guardarlo en config.
    for (int i = 1; i + 2 < argc; i++)
    {
        if (TextIsEqual(argv[i], "--ventana"))
        {
            int ancho = std::atoi(argv[i + 1]);
            int alto = std::atoi(argv[i + 2]);

            if (ancho > 0 && alto > 0)
            {
                juego.anchoVentanaForzada = ancho;
                juego.altoVentanaForzada = alto;
            }
        }
    }


    juego.Inicializar();


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

        if (IsKeyPressed(TECLA_CAPTURA))
        {
            GuardarCapturaDepuracion();
        }


        EndDrawing();
    }


    //------------------------------
    // CERRAR
    //------------------------------

    juego.Descargar();
    DescargarModeloJugadorCompartido();


    CloseWindow();


    return 0;
}
