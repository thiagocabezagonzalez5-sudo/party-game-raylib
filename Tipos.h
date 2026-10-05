#pragma once


//==================================================
// ESTADOS GENERALES DEL JUEGO
//==================================================

enum EstadoJuego
{
    ESTADO_LOGO,

    ESTADO_MENU,
    ESTADO_CONFIGURACION,

    // Flujo final de JUGAR:
    // modo -> personajes -> catalogo -> minijuego.
    ESTADO_SELECCION_MODO,
    ESTADO_SELECCION_JUGADORES,
    ESTADO_SELECCION_MINIJUEGO,
    ESTADO_TABLERO_VACIO,

    // Desarrollo / ejecucion de prototipos y minijuegos.
    ESTADO_ZONA_PRUEBAS,

    // Reservados para etapas posteriores del proyecto final.
    ESTADO_PARTIDA,

    // Tablero: fin de ronda -> ruleta -> minijuego -> resultado -> tablero.
    ESTADO_RULETA_MINIJUEGO,
    ESTADO_MINIJUEGO,
    ESTADO_RESULTADO
};
