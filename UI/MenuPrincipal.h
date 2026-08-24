#pragma once

#include "raylib.h"

#include "Systems/FondoAnimado.h"


struct MenuPrincipal
{
    //------------------------------
    // OPCIONES
    //------------------------------

    int opcionSeleccionada = 0;

    bool empezarJuego = false;

    bool abrirConfiguracion = false;

    bool salir = false;


    //------------------------------
    // RECURSOS
    //------------------------------

    FondoAnimado fondo;

    bool recursosCargados = false;


    //------------------------------
    // TRANSICION
    //------------------------------

    float tiempoEntrada = 0.0f;

    bool entradaActiva = false;


    /*
        Bajamos mucho los tiempos.

        El menu empieza a aparecer
        casi junto con el video.
    */

    const float DURACION_FADE_VIDEO =
        0.65f;

    const float RETRASO_MENU =
        0.15f;

    const float DURACION_FADE_MENU =
        0.65f;


    //------------------------------
    // FUNCIONES
    //------------------------------

    void Inicializar();

    void PrepararEntrada(
        bool usarFade
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar();

    void Descargar();
};