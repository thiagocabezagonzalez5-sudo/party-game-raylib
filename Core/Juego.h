#pragma once

#include "Tipos.h"

#include "Core/WindowUtils.h"
#include "Core/ConfiguracionJuego.h"

#include "Systems/Audio.h"

#include "UI/MenuPrincipal.h"
#include "UI/MenuConfiguracion.h"
#include "UI/PantallaLogo.h"


struct Juego
{
    //==================================================
    // ESTADO
    //==================================================

    EstadoJuego estado =
        ESTADO_MENU;


    bool cerrarJuego =
        false;


    //==================================================
    // CONFIGURACION JUEGO
    //==================================================

    ConfiguracionJuego config;

    const char* rutaConfiguracion = "config.ini";

    
    //==================================================
    // AUDIO
    //==================================================
    
    AudioJuego audio;


    //==================================================
    // MENUS
    //==================================================

    bool menuPreparado = false;

    bool cargaMenuSolicitada = false;

    MenuPrincipal menuPrincipal;

    MenuConfiguracion menuConfiguracion;

    PantallaLogo pantallaLogo;


    //==================================================
    // RESOLUCIONES
    //==================================================

    static const int MAX_RESOLUCIONES = 10;


    Resolucion resoluciones[
        MAX_RESOLUCIONES
    ];


    int cantidadResoluciones = 0;


    //==================================================
    // FPS
    //==================================================

    static const int CANTIDAD_OPCIONES_FPS = 5;


    int opcionesFPS[
        CANTIDAD_OPCIONES_FPS
    ] =
    {
        30,
        60,
        120,
        144,
        240
    };


    //==================================================
    // FUNCIONES
    //==================================================

    void Inicializar();

    void InicializarResoluciones();

    void Actualizar(
        float deltaTime
    );

    void Dibujar();

    bool DebeCerrar();

    void Descargar();
};