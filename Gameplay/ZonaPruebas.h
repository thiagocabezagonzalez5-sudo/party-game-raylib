#pragma once

#include "raylib.h"
#include "Core/ConfiguracionJuego.h"


const int MAX_BLOQUES_PRUEBA = 8;
const int MAX_JUGADORES_PRUEBA = 2;


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

    Color color = GRAY;
};


struct JugadorPrueba
{
    int numero = 1;
    bool activo = true;
    Color color = RED;

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

    Vector3 velocidad =
    {
        0.0f,
        0.0f,
        0.0f
    };

    Vector3 tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };

    float velocidadMovimiento = 5.0f;
    float fuerzaSalto = 7.2f;
    float gravedad = 18.0f;

    bool enSuelo = false;
    bool cayendo = false;

    float tiempoRespawn = 0.0f;
    float duracionRespawn = 1.2f;
};


struct ZonaPruebas
{
    JugadorPrueba jugadores[MAX_JUGADORES_PRUEBA];
    int cantidadJugadoresActivos = 2;

    ModoTeclado modoTecladoActual = TECLADO_DIVIDIDO;

    BloquePrueba bloques[MAX_BLOQUES_PRUEBA];
    int cantidadBloques = 0;

    Camera3D camara = { 0 };

    bool mostrarDebug = false;
    bool volverAlMenu = false;

    void Inicializar(
        ModoTeclado modoTeclado
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void ReiniciarJugador(
        int indice
    );

    void ReiniciarJugadores();
};
