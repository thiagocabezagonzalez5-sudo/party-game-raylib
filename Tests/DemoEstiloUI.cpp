// Demo del estilo UI "Carteles de playa" (UI/EstiloUI + UI/DemoEstiloUI).
//
//   DemoEstiloUI.exe                      -> interactivo (teclado y mando)
//   DemoEstiloUI.exe --captura ANCHO ALTO CARPETA
//                                         -> reproduce una secuencia de
//                                            entradas y exporta capturas
//
// Solo usa EstiloUI y DemoEstiloUI: no depende del resto del juego.

#include "UI/DemoEstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


static void Capturar(const char* carpeta, int ancho, int alto, const char* nombre)
{
    rlDrawRenderBatchActive();

    Image imagen = LoadImageFromScreen();
    char ruta[600];

    std::snprintf(ruta, sizeof ruta, "%s/ui_demo_%dx%d_%s.png", carpeta, ancho, alto, nombre);
    ExportImage(imagen, ruta);
    UnloadImage(imagen);

    std::printf("captura: %s\n", ruta);
}


// Entrada sintetica para el modo captura (equivale a pulsar teclas/botones).
struct PasoGuion
{
    int frame;
    const char* accion;
};

static const PasoGuion GUION[] =
{
    { 181, "abajo" },
    { 183, "derecha" },
    { 184, "derecha" },
    { 201, "pestana+" },
    { 261, "abajo" },
    { 262, "abajo" },
    { 263, "abajo" },
    { 264, "derecha" },
    { 270, "aceptar" }
};

struct PasoCaptura
{
    int frame;
    const char* nombre;
};

static const PasoCaptura CAPTURAS[] =
{
    { 14, "01_entrada_mitad_caida" },
    { 60, "02_entrada_rebote" },
    { 180, "03_reposo" },
    { 200, "04_palanca_ajustada" },
    { 207, "05_cambio_pestana_mitad" },
    { 260, "06_pestana_reposo" },
    { 268, "07_boton_foco_mitad" },
    { 272, "08_boton_pulsado" },
    { 330, "09_final" }
};


static EntradaUI EntradaDeGuion(int frame)
{
    EntradaUI entrada;

    for (const PasoGuion& paso : GUION)
    {
        if (paso.frame != frame)
        {
            continue;
        }

        if (!std::strcmp(paso.accion, "abajo")) entrada.abajo = true;
        if (!std::strcmp(paso.accion, "derecha")) entrada.derecha = true;
        if (!std::strcmp(paso.accion, "aceptar")) entrada.aceptar = true;
        if (!std::strcmp(paso.accion, "pestana+")) entrada.pestanaSiguiente = true;
    }

    return entrada;
}


int main(int argc, char** argv)
{
    bool modoCaptura = argc >= 5 && !std::strcmp(argv[1], "--captura");
    int ancho = modoCaptura ? std::atoi(argv[2]) : 1280;
    int alto = modoCaptura ? std::atoi(argv[3]) : 720;

    SetConfigFlags(modoCaptura ? FLAG_WINDOW_UNDECORATED : FLAG_WINDOW_RESIZABLE);
    InitWindow(ancho, alto, "Demo estilo UI - Carteles de playa");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    DemoEstiloUI demo;
    demo.Inicializar();

    LectorEntradaUI lector;
    int frame = 0;
    int ultimaCaptura = 335;

    while (!WindowShouldClose() && !demo.salir)
    {
        EntradaUI entrada;
        float dt = GetFrameTime();

        if (modoCaptura)
        {
            // Tiempo fijo: la secuencia es reproducible.
            entrada = EntradaDeGuion(frame);
            dt = 1.0f / 60.0f;
        }
        else
        {
            entrada = LeerEntradaUI(lector);
        }

        demo.Actualizar(dt, entrada);

        BeginDrawing();
        demo.Dibujar();

        if (modoCaptura)
        {
            for (const PasoCaptura& captura : CAPTURAS)
            {
                if (captura.frame == frame)
                {
                    Capturar(argv[4], ancho, alto, captura.nombre);
                }
            }
        }

        EndDrawing();

        frame++;

        if (modoCaptura && frame > ultimaCaptura)
        {
            break;
        }
    }

    demo.Descargar();
    CloseWindow();

    return 0;
}
