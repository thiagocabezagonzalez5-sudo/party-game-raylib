#pragma once

#include "raylib.h"
#include "Systems/Input.h"

struct Jugador{
    // IDENTIFICACION
    int id;
    Color color;

    // FISICA
    Vector3 posicion;
    Vector3 velocidad;

    float ancho;
    float alto;
    float profundidad;

    bool enSuelo;
    bool caido;

    // PARTIDA
    int puntaje;

    // VISUAL
    Model modelo;

    float rotacion;
    float escala;
    
    // FUNCIONES
    void Inicializar(
        int nuevoId
    );

    void Actualizar(
        float deltaTime,
        InputJugador input
    );

    void Saltar();

    void Dibujar() const;

    BoundingBox ObtenerHitbox() const;

    void Reiniciar();
};