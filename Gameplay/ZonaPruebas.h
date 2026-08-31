#pragma once

#include "raylib.h"


//==================================================
// CONSTANTES
//==================================================

const int MAX_BLOQUES_PRUEBA =
    8;


//==================================================
// BLOQUE / PLATAFORMA
//==================================================

struct BloquePrueba
{
    Vector3 posicion =
    {
        0.0f,
        0.0f,
        0.0f
    };


    Vector3 tamano =
    {
        1.0f,
        1.0f,
        1.0f
    };


    Color color =
        GRAY;
};


//==================================================
// JUGADOR PLACEHOLDER
//==================================================

struct JugadorPrueba
{
    //------------------------------
    // POSICION
    //------------------------------

    Vector3 posicion =
    {
        0.0f,
        1.0f,
        0.0f
    };


    Vector3 posicionSpawn =
    {
        0.0f,
        1.0f,
        0.0f
    };


    //------------------------------
    // VELOCIDAD
    //------------------------------

    Vector3 velocidad =
    {
        0.0f,
        0.0f,
        0.0f
    };


    //------------------------------
    // TAMAÑO FISICO
    //------------------------------

    Vector3 tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };


    //------------------------------
    // MOVIMIENTO
    //------------------------------

    float velocidadMovimiento =
        5.0f;


    float fuerzaSalto =
        7.2f;


    float gravedad =
        18.0f;


    //------------------------------
    // ESTADOS
    //------------------------------

    bool enSuelo =
        false;


    bool cayendo =
        false;


    //------------------------------
    // RESPAWN
    //------------------------------

    float tiempoRespawn =
        0.0f;


    float duracionRespawn =
        1.2f;
};


//==================================================
// ZONA DE PRUEBAS
//==================================================

struct ZonaPruebas
{
    //------------------------------
    // JUGADOR
    //------------------------------

    JugadorPrueba jugador;


    //------------------------------
    // MUNDO
    //------------------------------

    BloquePrueba bloques[
        MAX_BLOQUES_PRUEBA
    ];


    int cantidadBloques =
        0;


    //------------------------------
    // CAMARA
    //------------------------------

    Camera3D camara =
    {
        0
    };


    //------------------------------
    // DEBUG
    //------------------------------

    bool mostrarDebug =
        false;


    //------------------------------
    // ESTADO
    //------------------------------

    bool volverAlMenu =
        false;


    //------------------------------
    // FUNCIONES
    //------------------------------

    void Inicializar();


    void Actualizar(
        float deltaTime
    );


    void Dibujar() const;


    void ReiniciarJugador();
};