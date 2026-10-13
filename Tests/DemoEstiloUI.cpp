// Demo del estilo UI "Carteles de playa" (UI/EstiloUI + UI/DemoEstiloUI).
//
//   DemoEstiloUI.exe                      -> interactivo (teclado y mando)
//   DemoEstiloUI.exe --captura ANCHO ALTO CARPETA [--reducir-movimiento]
//                                         -> reproduce una secuencia de
//                                            entradas y exporta capturas
//                                            (opcionalmente con
//                                            reducirMovimiento activado)
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
    // VIDEO: palanca con texto propio (60 -> 120 FPS).
    { 181, "abajo" },
    { 183, "derecha" },
    { 184, "derecha" },
    // AUDIO: palancas 0..100 % por defecto.
    { 201, "pestana+" },
    // JUEGO -> CONTROLES: palanca con rango, selector deshabilitado y tarjeta.
    { 261, "pestana+" },
    { 262, "pestana+" },
    { 341, "derecha" },
    { 342, "derecha" },
    { 343, "abajo" },
    { 344, "derecha" },     // ignorado: el selector esta deshabilitado
    // VOLVER -> aviso "salir sin aplicar".
    { 361, "abajo" },
    { 362, "aceptar" },
    { 456, "izquierda" },
    { 500, "volver" },      // cancela (SEGUIR EDITANDO)
    // APLICAR -> aviso con cuenta atras.
    { 501, "derecha" },
    { 502, "aceptar" }
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
    { 180, "03_reposo_pestanas_ajustadas" },
    { 200, "04_palanca_texto_propio" },
    { 207, "05_cambio_pestana_mitad" },
    { 260, "06_palancas_porcentaje" },
    { 340, "07_controles_tarjeta" },
    { 360, "08_rango_y_deshabilitado" },
    { 368, "09_modal_entrada" },
    { 455, "10_modal_reposo" },
    { 465, "11_modal_foco_mitad" },
    { 506, "12_modal_cerrado" },
    { 585, "13_modal_cuenta_atras" }
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
        if (!std::strcmp(paso.accion, "izquierda")) entrada.izquierda = true;
        if (!std::strcmp(paso.accion, "aceptar")) entrada.aceptar = true;
        if (!std::strcmp(paso.accion, "volver")) entrada.volver = true;
        if (!std::strcmp(paso.accion, "pestana+")) entrada.pestanaSiguiente = true;
    }

    return entrada;
}


int main(int argc, char** argv)
{
    bool modoCaptura = argc >= 5 && !std::strcmp(argv[1], "--captura");
    int ancho = modoCaptura ? std::atoi(argv[2]) : 1280;
    int alto = modoCaptura ? std::atoi(argv[3]) : 720;
    bool reducir = argc >= 6 && !std::strcmp(argv[5], "--reducir-movimiento");

    SetConfigFlags(modoCaptura ? FLAG_WINDOW_UNDECORATED : FLAG_WINDOW_RESIZABLE);
    InitWindow(ancho, alto, "Demo estilo UI - Carteles de playa");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    DemoEstiloUI demo;
    demo.Inicializar(reducir);

    LectorEntradaUI lector;
    int frame = 0;
    int ultimaCaptura = 590;

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
