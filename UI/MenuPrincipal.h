#pragma once

#include "raylib.h"

#include "Systems/FondoAnimado.h"

struct MenuPrincipal
{
    //------------------------------
    // MENU
    //------------------------------

    int opcionSeleccionada =
        0;

    bool empezarJuego =
        false;

    bool abrirConfiguracion =
        false;

    bool salir =
        false;


    //------------------------------
    // FONDO
    //------------------------------

    FondoAnimado fondo;

    bool recursosCargados =
        false;


    //------------------------------
    // TRANSICION
    //------------------------------

    float tiempoEntrada =
        0.0f;

    bool entradaActiva =
        false;

    bool fadeBlancoActivo =
        true;

    const float DURACION_FADE_VIDEO =
        0.65f;

    const float RETRASO_MENU =
        0.10f;

    const float DURACION_FADE_MENU =
        0.35f;


    //------------------------------
    // CLICK / ENTER VISUAL
    //------------------------------

    int botonPresionado =
        -1;

    int accionPendiente =
        -1;

    float tiempoBotonPresionado =
        0.0f;

    const float DURACION_BOTON_PRESION =
        0.12f;


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

    void Descargar();
};