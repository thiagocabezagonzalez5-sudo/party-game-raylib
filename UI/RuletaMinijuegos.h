#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Systems/Audio.h"
#include "raylib.h"


// Carrusel que presenta el minijuego de la ronda del tablero.
// El resultado se recibe ya decidido en Iniciar(); la animacion
// solo lo muestra y siempre se detiene exactamente en ese minijuego.
enum EstadoRuletaMinijuegos
{
    RULETA_INACTIVA = 0,
    RULETA_INICIANDO,
    RULETA_GIRANDO,
    RULETA_FRENANDO,
    RULETA_SELECCIONADO,
    RULETA_TERMINADA
};


struct RuletaMinijuegos
{
    // Minijuegos disponibles para tablero, en orden de catalogo.
    IdMinijuego opciones[CANTIDAD_MINIJUEGOS]{};
    int cantidadOpciones = 0;

    IdMinijuego minijuegoElegido = MINIJUEGO_COLOR_SEGURO;
    int indiceElegido = 0;

    EstadoRuletaMinijuegos estado = RULETA_INACTIVA;

    float tiempoEstado = 0.0f;
    float tiempoTotal = 0.0f;

    // Posicion continua del carrusel medida en miniaturas.
    // El valor entero centrado indica la opcion bajo el marcador.
    float posicion = 0.0f;
    float posicionInicial = 0.0f;

    // El frenado empieza en posicionInicioFrenado y termina
    // exactamente en posicionFinal, que apunta a indiceElegido.
    float posicionInicioFrenado = 0.0f;
    float posicionFinal = 0.0f;

    // Confirmar solo acelera el tiempo de la animacion;
    // nunca cambia posicionFinal.
    bool frenadoAcelerado = false;

    int ultimaCasillaCentrada = 0;
    float tiempoDesdeUltimoTick = 0.0f;

    AudioJuego* audio = nullptr;

    void Iniciar(
        IdMinijuego elegido,
        AudioJuego* audioJuego
    );

    void Actualizar(
        float deltaTime,
        bool confirmar
    );

    void Dibujar() const;

    bool Termino() const;

    IdMinijuego ObtenerMinijuegoElegido() const;
};
