#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/TiposMinijuegos.h"


enum DireccionPinchos
{
    PINCHOS_DESDE_ARRIBA = 0,
    PINCHOS_DESDE_ABAJO,
    PINCHOS_DESDE_IZQUIERDA,
    PINCHOS_DESDE_DERECHA
};


enum FaseRefugioPinchos
{
    FASE_PINCHOS_PREPARACION = 0,
    FASE_PINCHOS_ESPERANDO,
    FASE_PINCHOS_AVISO,
    FASE_PINCHOS_ATAQUE,
    FASE_PINCHOS_TERMINADO
};


struct EstadoJugadorRefugioPinchos
{
    bool eliminado = false;
    int posicionFinal = 0;
};


struct MinijuegoRefugioPinchos
{
    ResultadoMinijuego resultado{};

    EstadoJugadorRefugioPinchos estadosJugadores[MAX_PARTICIPANTES];
    EstadoBotRefugioPinchos estadosBots[MAX_PARTICIPANTES];

    BloquePrueba bloques[5];
    int cantidadBloques = 0;

    Camera3D camara{};

    FaseRefugioPinchos fase = FASE_PINCHOS_PREPARACION;
    DireccionPinchos direccionAviso = PINCHOS_DESDE_ARRIBA;

    int indiceSolo = -1;

    float tiempoPreparacion = 2.5f;
    float tiempoRestante = 20.0f;
    float tiempoFase = 0.0f;
    float cooldownAtaque = 0.0f;

    bool ataqueResuelto = false;

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
        Participante participantes[],
        ParticulaTierra particulas[],
        int cantidadParticulas
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
