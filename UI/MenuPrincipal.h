#pragma once

#include "raylib.h"

#include "UI/Hub3D.h"

// Opciones del HUB (valor de opcionSeleccionada).
enum OpcionMenuPrincipal
{
    OPCION_MENU_PARTIDA = 0,
    OPCION_MENU_MINIJUEGOS,
    OPCION_MENU_CONFIGURACION,
    OPCION_MENU_SALIR,
    OPCION_MENU_CANTIDAD
};

struct MenuPrincipal
{
    //------------------------------
    // MENU
    //------------------------------

    int opcionSeleccionada =
        0;

    bool empezarTablero =
        false;

    bool empezarMinijuegos =
        false;

    bool abrirConfiguracion =
        false;

    bool salir =
        false;


    //------------------------------
    // ESCENARIO
    //------------------------------

    Hub3D hub;

    bool recursosCargados =
        false;


    //------------------------------
    // TRANSICION DE ENTRADA
    //------------------------------

    float tiempoEntrada =
        0.0f;

    bool entradaActiva =
        false;

    // true: fade desde blanco (viene del logo). false: desde negro.
    bool fadeBlancoActivo =
        true;


    //------------------------------
    // CAMARA E INTERACCION
    //------------------------------

    // Segundos desde la ultima accion del jugador.
    float tiempoSinInput =
        999.0f;

    bool confirmando =
        false;

    float tiempoConfirmacion =
        0.0f;

    // Ultimo estado del stick de cada gamepad (-1, 0, 1) para detectar flancos.
    int ejeHorizontalPrevio[4] = {};

    int ejeVerticalPrevio[4] = {};


    //------------------------------
    // FUNCIONES
    //------------------------------

    void Inicializar();

    void PrepararEntrada(
        bool desdeLogo
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar();

    // Fondo ambiental para otras pantallas (sin carteles ni seleccion).
    void ActualizarFondo(
        float deltaTime
    );

    void DibujarFondo();

    void Descargar();
};
