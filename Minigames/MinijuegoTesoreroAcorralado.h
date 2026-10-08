#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Referencia de reglas: Mario Party 1 - Bash 'n' Cash (1 vs 3).
// Aqui se transforma en un patio de armas: el tesorero lleva el tesoro y el
// trio lo persigue para hacerle soltar monedas. Las rejas levadizas suben y
// bajan y pueden cortar el paso.


inline constexpr int MAX_BLOQUES_TESORERO = 8;
inline constexpr int MAX_REJAS_TESORERO = 4;
inline constexpr int MAX_MONEDAS_TESORERO = 48;


enum FaseTesoreroAcorralado
{
    FASE_TESORERO_PREPARACION = 0,
    FASE_TESORERO_JUGANDO,
    FASE_TESORERO_TERMINADO
};


enum EstadoRejaTesorero
{
    REJA_TESORERO_ABIERTA = 0,
    REJA_TESORERO_AVISO,
    REJA_TESORERO_CERRADA
};


struct EstadoJugadorTesorero
{
    // 0 = tesorero, 1 = trio, -1 = no participa.
    int equipo = -1;

    // Monedas robadas por un miembro del trio.
    int monedasRobadas = 0;

    // Inteligencia de bot.
    float tiempoDecision = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;
    float cooldownAccionBot = 0.0f;
    float tiempoSinProgreso = 0.0f;
    float ultimaX = 0.0f;
    float ultimaZ = 0.0f;
    float tiempoRodeo = 0.0f;
    float rodeoX = 0.0f;
    float rodeoZ = 0.0f;
};


struct RejaTesorero
{
    float x = 0.0f;
    float z = 0.0f;
    float mitadX = 0.2f;
    float mitadZ = 2.0f;

    EstadoRejaTesorero estado = REJA_TESORERO_ABIERTA;
    float tiempoEstado = 0.0f;

    // 0 = levantada del todo, 1 = bajada del todo (bloquea).
    float altura = 0.0f;
};


struct MonedaTesorero
{
    bool activa = false;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float velocidadX = 0.0f;
    float velocidadY = 0.0f;
    float velocidadZ = 0.0f;
    float tiempoSinRecoger = 0.0f;
    float edad = 0.0f;
};


struct MinijuegoTesoreroAcorralado
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorTesorero estadosJugadores[MAX_PARTICIPANTES];
    RejaTesorero rejas[MAX_REJAS_TESORERO];
    MonedaTesorero monedas[MAX_MONEDAS_TESORERO];
    BloquePrueba bloques[MAX_BLOQUES_TESORERO];
    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];
    int cantidadBloques = 0;

    Camera3D camara{};
    FaseTesoreroAcorralado fase = FASE_TESORERO_PREPARACION;

    int indiceTesorero = -1;
    int monedasTesorero = 0;
    int monedasPerdidas = 0;
    bool ganaTesorero = false;
    bool partidaValida = false;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;
    float cooldownSonidoMoneda = 0.0f;

    void Inicializar();

    void Reiniciar(
        JugadorPrueba jugadores[],
        Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[]
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
