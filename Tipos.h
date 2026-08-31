#pragma once


//==================================================
// ESTADOS GENERALES DEL JUEGO
//==================================================

enum EstadoJuego
{
    //------------------------------
    // INTRO
    //------------------------------

    ESTADO_LOGO,


    //------------------------------
    // MENUS
    //------------------------------

    ESTADO_MENU,

    ESTADO_CONFIGURACION,

    ESTADO_SELECCION_JUGADORES,


    //------------------------------
    // DESARROLLO / TEST
    //------------------------------

    ESTADO_ZONA_PRUEBAS,


    //------------------------------
    // JUEGO
    //------------------------------

    ESTADO_PARTIDA,

    ESTADO_MINIJUEGO,

    ESTADO_RESULTADO
};