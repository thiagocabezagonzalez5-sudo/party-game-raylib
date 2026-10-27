#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_INTERRUPTORES_CAOS = 5;


enum FaseInterruptoresCaos
{
    FASE_INTERRUPTORES_PREPARACION = 0,
    FASE_INTERRUPTORES_ELECCION,
    FASE_INTERRUPTORES_SEGURO,
    FASE_INTERRUPTORES_EXPLOSION,
    FASE_INTERRUPTORES_TERMINADO
};


struct EstadoJugadorInterruptoresCaos
{
    bool eliminado = false;
    int posicionFinal = 0;
    int turnosSuperados = 0;
    float tiempoDecisionBot = 0.0f;

    // Solo visual: salto al salvarse y vuelo al recibir la descarga.
    float salto = 0.0f;
    float descarga = 0.0f;          // segundos desde el rayo (0 = sin rayo)
    bool cayo = false;
};


// Efectos de la sala de control (solo visuales).
struct ParticulaInterruptoresCaos
{
    bool activa = false;
    bool brillo = false;            // chispa: se dibuja en la pasada aditiva
    bool humo = false;              // humo: crece, sube y se apaga
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
};


enum AnuncioInterruptoresCaos
{
    ANUNCIO_INTERRUPTORES_NINGUNO = 0,
    ANUNCIO_INTERRUPTORES_TURNO,
    ANUNCIO_INTERRUPTORES_SEGURO,
    ANUNCIO_INTERRUPTORES_SOBRECARGA,
    ANUNCIO_INTERRUPTORES_RONDA
};


inline constexpr int MAX_PARTICULAS_INTERRUPTORES = 280;


struct MinijuegoInterruptoresCaos
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorInterruptoresCaos jugadores[MAX_PARTICIPANTES];

    bool interruptoresUsados[MAX_INTERRUPTORES_CAOS]{};
    int cantidadInterruptores = MAX_INTERRUPTORES_CAOS;
    int interruptorPeligroso = 0;
    int interruptorSeleccionado = 0;

    int jugadorTurno = -1;
    int jugadorExplosion = -1;
    int numeroTurno = 0;

    FaseInterruptoresCaos fase = FASE_INTERRUPTORES_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoTurno = 0.0f;
    float tiempoResolucion = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: nada de esto toca la logica ni consume el
    // GetRandomValue de los bots.
    unsigned int semillaVisual = 2654435761u;
    FaseInterruptoresCaos faseVista = FASE_INTERRUPTORES_PREPARACION;
    int seleccionVista = -1;
    float pulsoSeleccion = 0.0f;    // la seleccion acaba de moverse
    float tensionVista = 0.0f;      // aguja del medidor, suavizada
    float sobrecarga = 0.0f;        // 1 al saltar la sobrecarga, se apaga
    float aparicion = 0.0f;         // segundos desde que se armo la ronda
    float tiempoFin = 0.0f;
    float destelloPalanca[MAX_INTERRUPTORES_CAOS]{};
    AnuncioInterruptoresCaos anuncio = ANUNCIO_INTERRUPTORES_NINGUNO;
    float tiempoAnuncio = 0.0f;
    int jugadorAnuncio = -1;
    int palancaAnuncio = -1;
    float proximaChispa = 0.0f;     // chispazos del empalme flojo
    ParticulaInterruptoresCaos particulas[MAX_PARTICULAS_INTERRUPTORES];

    void Inicializar();

    void Reiniciar(
        Participante participantes[]
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[]
    );

    void Dibujar(
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
