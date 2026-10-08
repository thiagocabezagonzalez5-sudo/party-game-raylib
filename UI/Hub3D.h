#pragma once

#include "raylib.h"

//==================================================
// HUB 3D
//==================================================
// Escenario 3D del menu principal: una isla-plaza al atardecer con
// cuatro objetos que funcionan como opciones:
//
//   0  DIRIGIBLE  amarrado a una torre  -> PARTIDA
//   1  BARCO      en el embarcadero     -> MINIJUEGOS
//   2  TORRE DE ENGRANAJES              -> CONFIGURACION
//   3  ARCO DE PIEDRA con portal        -> SALIR
//
// Este struct solo se ocupa de la escena y de la camara. La logica de
// seleccion, input y carteles vive en MenuPrincipal.

constexpr int HUB_CANTIDAD_OPCIONES =
    4;

struct Hub3D
{
    //------------------------------
    // TIEMPO
    //------------------------------

    float tiempo =
        0.0f;

    // Avanza solo mientras la camara esta en modo ambiental.
    float tiempoAmbiental =
        0.0f;

    //------------------------------
    // CAMARA
    //------------------------------

    // 0 = recorrido ambiental, 1 = enfocando la opcion elegida.
    float enfoque =
        0.0f;

    // 0..1, acercamiento breve al confirmar.
    float zoomConfirmacion =
        0.0f;

    Vector3 anclaFoco =
        { 0.0f, 3.0f, 0.0f };

    Vector3 camaraPosicion =
        { 0.0f, 7.0f, 26.0f };

    Vector3 camaraObjetivo =
        { 0.0f, 4.0f, 0.0f };

    bool camaraInicializada =
        false;

    //------------------------------
    // RESULTADO DEL ULTIMO DIBUJO
    //------------------------------

    // Posicion en pantalla del cartel flotante de cada opcion.
    Vector2 posicionPantallaOpcion[HUB_CANTIDAD_OPCIONES] = {};

    bool opcionVisible[HUB_CANTIDAD_OPCIONES] = {};


    //------------------------------
    // FUNCIONES
    //------------------------------

    void Inicializar();

    // opcionEnfocada: opcion que la camara debe mirar si enfocar es true.
    // zoomObjetivo: 0 normal, 1 acercamiento de confirmacion.
    void Actualizar(
        float deltaTime,
        int opcionEnfocada,
        bool enfocar,
        float zoomObjetivo
    );

    // opcionResaltada: -1 dibuja la escena sin resaltar ningun objeto.
    void Dibujar(
        int opcionResaltada
    );

    void Descargar();
};
