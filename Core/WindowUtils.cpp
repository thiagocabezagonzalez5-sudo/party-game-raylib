#include "Core/WindowUtils.h"

#include <cmath>


//==================================================
// NOMBRE DEL MODO
//==================================================

const char* NombreModoVentana(
    ModoVentana modo
)
{
    switch (modo)
    {
        case MODO_VENTANA:
            return "Ventana";

        case MODO_PANTALLA_COMPLETA:
            return "Pantalla completa";

        case MODO_SIN_BORDES:
            return "Ventana sin bordes";
    }

    return "Desconocido";
}


//==================================================
// AGREGAR RESOLUCION
//==================================================

void AgregarResolucion(
    Resolucion resoluciones[],
    int& cantidad,
    int maximo,
    int ancho,
    int alto
)
{
    //------------------------------
    // ARRAY LLENO
    //------------------------------

    if (cantidad >= maximo)
    {
        return;
    }


    //------------------------------
    // EVITAR DUPLICADOS
    //------------------------------

    for (int i = 0; i < cantidad; i++)
    {
        if (
            resoluciones[i].ancho == ancho &&
            resoluciones[i].alto == alto
        )
        {
            return;
        }
    }


    resoluciones[cantidad] =
        Resolucion{
            ancho,
            alto
        };

    cantidad++;
}


//==================================================
// CENTRAR VENTANA
//==================================================

void CentrarVentana(
    int ancho,
    int alto
)
{
    int monitor =
        GetCurrentMonitor();


    Vector2 posicionMonitor =
        GetMonitorPosition(
            monitor
        );


    int anchoMonitor =
        GetMonitorWidth(
            monitor
        );


    int altoMonitor =
        GetMonitorHeight(
            monitor
        );


    int x =
        (int)posicionMonitor.x +
        (anchoMonitor - ancho) / 2;


    int y =
        (int)posicionMonitor.y +
        (altoMonitor - alto) / 2;


    SetWindowPosition(
        x,
        y
    );
}


//==================================================
// CAMBIAR MODO DE VENTANA
//==================================================

void AplicarModoVentana(
    ModoVentana& modoActual,
    ModoVentana nuevoModo,
    Resolucion resolucion
)
{
    //------------------------------
    // SALIR DEL MODO ACTUAL
    //------------------------------

    if (
        modoActual ==
        MODO_PANTALLA_COMPLETA
    )
    {
        if (IsWindowFullscreen())
        {
            ToggleFullscreen();
        }
    }

    else if (
        modoActual ==
        MODO_SIN_BORDES
    )
    {
        ToggleBorderlessWindowed();
    }


    //------------------------------
    // VENTANA NORMAL
    //------------------------------

    if (
        nuevoModo ==
        MODO_VENTANA
    )
    {
        // Una ventana del tamano del monitor queda tapada por la barra de
        // titulo y la de tareas (se pierde la parte inferior del HUD): se
        // limita al area util manteniendo la proporcion.
        int monitor = GetCurrentMonitor();
        int anchoMaximo = GetMonitorWidth(monitor) - 16;
        int altoMaximo = GetMonitorHeight(monitor) - 96;

        int ancho = resolucion.ancho;
        int alto = resolucion.alto;

        if (
            anchoMaximo > 0 &&
            altoMaximo > 0 &&
            (ancho > anchoMaximo || alto > altoMaximo)
        )
        {
            float escala = std::fmin(
                (float)anchoMaximo / (float)ancho,
                (float)altoMaximo / (float)alto
            );

            ancho = (int)((float)ancho * escala);
            alto = (int)((float)alto * escala);
        }

        SetWindowSize(
            ancho,
            alto
        );


        CentrarVentana(
            ancho,
            alto
        );
    }


    //------------------------------
    // PANTALLA COMPLETA
    //------------------------------

    else if (
        nuevoModo ==
        MODO_PANTALLA_COMPLETA
    )
    {
        SetWindowSize(
            resolucion.ancho,
            resolucion.alto
        );


        CentrarVentana(
            resolucion.ancho,
            resolucion.alto
        );


        ToggleFullscreen();
    }


    //------------------------------
    // SIN BORDES
    //------------------------------

    else if (
        nuevoModo ==
        MODO_SIN_BORDES
    )
    {
        /*
            En borderless Raylib utiliza
            normalmente el tamaño completo
            del monitor actual.
        */

        ToggleBorderlessWindowed();
    }


    modoActual =
        nuevoModo;
}


//==================================================
// VSYNC
//==================================================

void AplicarVSync(
    bool activo
)
{
    if (activo)
    {
        SetWindowState(FLAG_VSYNC_HINT);
    }
    else
    {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
}
