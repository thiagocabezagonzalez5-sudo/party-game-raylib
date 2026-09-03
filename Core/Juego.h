#pragma once

#include "Tipos.h"

#include "raylib.h"


//==================================================
// CORE
//==================================================

#include "Core/ConfiguracionJuego.h"
#include "Core/Participante.h"
#include "Core/WindowUtils.h"


//==================================================
// SYSTEMS
//==================================================

#include "Systems/Audio.h"


//==================================================
// UI
//==================================================

#include "UI/MenuPrincipal.h"
#include "UI/MenuConfiguracion.h"
#include "UI/PantallaLogo.h"
#include "UI/SeleccionPersonajes.h"


//==================================================
// GAMEPLAY
//==================================================

#include "Gameplay/ZonaPruebas.h"


//==================================================
// JUEGO
//==================================================

struct Juego
{
    //------------------------------
    // CONSTANTES
    //------------------------------

    static const int MAX_RESOLUCIONES =
        10;


    static const int CANTIDAD_OPCIONES_FPS =
        5;


    //------------------------------
    // ESTADO
    //------------------------------

    EstadoJuego estado =
        ESTADO_LOGO;


    bool cerrarJuego =
        false;


    //------------------------------
    // MENUS / PANTALLAS
    //------------------------------

    MenuPrincipal menuPrincipal;


    MenuConfiguracion menuConfiguracion;


    PantallaLogo pantallaLogo;


    SeleccionPersonajes seleccionPersonajes;


    //------------------------------
    // GAMEPLAY
    //------------------------------

    ZonaPruebas zonaPruebas;


    //------------------------------
    // AUDIO
    //------------------------------

    AudioJuego audio;


    //------------------------------
    // CONFIGURACION
    //------------------------------

    ConfiguracionJuego config;


    //------------------------------
    // PARTICIPANTES
    //------------------------------

    Participante participantes[
        MAX_PARTICIPANTES
    ];


    // Solo puede ser 0 antes de confirmar, o 2/3/4
    // cuando exista una seleccion confirmada.
    int cantidadParticipantes =
        0;


    const char* rutaConfiguracion =
        "config.ini";


    //------------------------------
    // RESOLUCIONES
    //------------------------------

    Resolucion resoluciones[
        MAX_RESOLUCIONES
    ];


    int cantidadResoluciones =
        0;


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


    //------------------------------
    // LOGO / CARGA
    //------------------------------

    bool menuPreparado =
        false;


    bool cargaMenuSolicitada =
        false;


    //------------------------------
    // FUNCIONES
    //------------------------------

    void Inicializar();


    void InicializarResoluciones();


    void Actualizar(
        float deltaTime
    );


    void Dibujar();


    bool DebeCerrar();


    void Descargar();
};
