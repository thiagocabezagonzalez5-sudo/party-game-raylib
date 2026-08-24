#include "Entities/Jugador.h"
#include "Systems/Input.h"

void Jugador::Inicializar(
    int nuevoId
)
{
    id = nuevoId;
    color = (id == 0) ? RED : BLUE;

    posicion = { 0.0f, 1.0f, 0.0f };
    velocidad = { 0.0f, 0.0f, 0.0f };

    ancho = 1.0f;
    alto = 2.0f;
    profundidad = 1.0f;

    enSuelo = false;
    caido = false;

    puntaje = 0;

    modelo = LoadModel("Assets/Modelos/Jugador_Raylib_Normalizado.glb");

    rotacion = 0.0f;
    escala = 1.0f;
}

void Jugador::Actualizar(
    float deltaTime,
    InputJugador input
)
{
    // Actualizar la posición y velocidad del jugador
    velocidad.x = input.moverX * 5.0f; // Velocidad horizontal
    velocidad.z = input.moverZ * 5.0f; // Velocidad vertical

    // Aplicar gravedad
    if (!enSuelo) {
        velocidad.y -= 9.81f * deltaTime; // Gravedad
    }

    // Actualizar la posición del jugador
    posicion.x += velocidad.x * deltaTime;
    posicion.y += velocidad.y * deltaTime;
    posicion.z += velocidad.z * deltaTime;

    // Verificar si el jugador está en el suelo
    if (posicion.y <= 1.0f) {
        posicion.y = 1.0f;
        enSuelo = true;
        velocidad.y = 0.0f;
    }
}

void Jugador::Saltar() {
    if (enSuelo) {
        velocidad.y = 8.0f;
        enSuelo = false;
    }
}

void Jugador::Dibujar() const {
    DrawModel(modelo, posicion, escala, color);
}

BoundingBox Jugador::ObtenerHitbox() const {
    BoundingBox hitbox;
    hitbox.min = {
        posicion.x - ancho / 2,
        posicion.y,
        posicion.z - profundidad / 2
    };
    hitbox.max = {
        posicion.x + ancho / 2,
        posicion.y + alto,
        posicion.z + profundidad / 2
    };
    return hitbox;
}

void Jugador::Reiniciar() {
    posicion = { 0.0f, 1.0f, 0.0f };
    velocidad = { 0.0f, 0.0f, 0.0f };
    enSuelo = false;
    caido = false;
    puntaje = 0;
}