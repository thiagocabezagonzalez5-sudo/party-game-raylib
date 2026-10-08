#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


inline constexpr int TOTAL_FRUTAS_RACIMO = 22;
inline constexpr int VIDAS_RACIMO = 2;
inline constexpr int MAX_VUELOS_RACIMO = 2;


enum FaseRacimoToxico
{
    FASE_RACIMO_PREPARACION = 0,
    FASE_RACIMO_TURNO,
    FASE_RACIMO_ANIMACION,
    FASE_RACIMO_TERMINADO
};


enum TipoFrutaRacimo
{
    FRUTA_RACIMO_NORMAL = 0,
    FRUTA_RACIMO_TOXICA,
    FRUTA_RACIMO_DORADA
};


enum EventoRacimoToxico
{
    EVENTO_RACIMO_NINGUNO = 0,
    EVENTO_RACIMO_SEGURO,
    EVENTO_RACIMO_TOXICA,
    EVENTO_RACIMO_ELIMINADO,
    EVENTO_RACIMO_DORADA
};


struct EstadoJugadorRacimoToxico
{
    bool vivo = false;
    int vidas = VIDAS_RACIMO;
    int posicionEliminacion = 0;
    int frutasSeguras = 0;
    float sacudida = 0.0f;
};


struct VueloFrutaRacimo
{
    bool activo = false;
    int tipo = FRUTA_RACIMO_NORMAL;
    int jugador = -1;
    float progreso = 0.0f;
    Vector3 origen{};
};


struct MinijuegoRacimoToxico
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorRacimoToxico estadosJugadores[MAX_PARTICIPANTES];
    VueloFrutaRacimo vuelos[MAX_VUELOS_RACIMO];
    int tipoFruta[TOTAL_FRUTAS_RACIMO]{};
    float posicionBalsaX[MAX_PARTICIPANTES]{};

    Camera3D camara{};
    FaseRacimoToxico fase = FASE_RACIMO_PREPARACION;

    int frente = 0;
    int turno = -1;
    int seleccion = 1;
    int ultimoJugador = -1;
    int jugadorSaltado = -1;
    EventoRacimoToxico ultimoEvento = EVENTO_RACIMO_NINGUNO;
    bool saltarSiguiente = false;
    bool accionPrevia = false;
    bool racimoRepuesto = false;

    float tiempoTurno = 0.0f;
    float tiempoPausa = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoAnimacion = 0.0f;
    float tiempoBot = 0.0f;

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
