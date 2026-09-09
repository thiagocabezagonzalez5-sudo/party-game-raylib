#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


enum FaseMinijuegoBarraGiratoria
{
    FASE_BARRA_PREPARACION = 0,
    FASE_BARRA_JUGANDO,
    FASE_BARRA_TERMINADO
};


struct EstadoJugadorBarraGiratoria
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;

    // Evita que una misma barra golpee varias veces en frames consecutivos.
    float cooldownImpacto = 0.0f;

    // Tras recibir una barra el jugador conserva el impulso, pero durante
    // unas decimas no puede corregirlo con movimiento propio.
    float tiempoStunBarra = 0.0f;
};


struct MinijuegoBarraGiratoria
{
    ResultadoMinijuego resultado;

    EstadoJugadorBarraGiratoria estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba suelo;
    Camera3D camara{};

    FaseMinijuegoBarraGiratoria fase =
        FASE_BARRA_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoJugado = 0.0f;

    float anguloBarra = 0.0f;
    float velocidadAngular = 0.9f;

    // A los diez segundos aparece una segunda barra desde arriba. Mientras
    // cae solo es visual; su colision se activa al asentarse sobre la primera.
    bool segundaBarraAparecio = false;
    bool segundaBarraLista = false;
    float alturaSegundaBarra = 8.0f;
    float anguloSegundaBarra = 1.5707963f;
    float velocidadAngularSegunda = 1.35f;

    void Inicializar();

    void ConfigurarJugadores(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    ) const;

    void Reiniciar(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[],
        ParticulaTierra particulas[],
        int cantidadParticulas
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        const ParticulaTierra particulas[],
        int cantidadParticulas,
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
