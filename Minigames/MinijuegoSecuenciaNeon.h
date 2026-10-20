#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_PULSOS_SECUENCIA_NEON = 8;
inline constexpr int MAX_PARTICULAS_SECUENCIA_NEON = 140;


// Solo visual: chispas de las consolas y motas de ambiente (pool fijo).
struct ParticulaSecuenciaNeon
{
    bool activa = false;
    bool gravedad = true;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.0f;
    Color color{};
};


enum PulsoSecuenciaNeon
{
    PULSO_NEON_ARRIBA = 0,
    PULSO_NEON_DERECHA,
    PULSO_NEON_ABAJO,
    PULSO_NEON_IZQUIERDA,
    CANTIDAD_PULSOS_NEON
};


enum FaseSecuenciaNeon
{
    FASE_SECUENCIA_PREPARACION = 0,
    FASE_SECUENCIA_MOSTRANDO,
    FASE_SECUENCIA_RESPONDIENDO,
    FASE_SECUENCIA_RESOLUCION,
    FASE_SECUENCIA_TERMINADO
};


struct EstadoJugadorSecuenciaNeon
{
    bool eliminado = false;
    bool completoRonda = false;
    bool falloEstaRonda = false;

    int indiceRespuesta = 0;
    int aciertosTotales = 0;
    int rondasSuperadas = 0;
    int puntuacionFinal = 0;

    float tiempoRespuestaBot = 0.0f;
    int indiceFalloBot = -1;

    // Solo visual y NEUTRO respecto de la direccion: destellos (1..0) de
    // la consola al pulsar, al fallar y al completar la ronda. Nunca se
    // guarda que boton se toco (todos lo verian en la pantalla compartida).
    float destelloPulso = 0.0f;
    float destelloError = 0.0f;
    float destelloRonda = 0.0f;
};


struct MinijuegoSecuenciaNeon
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorSecuenciaNeon jugadores[MAX_PARTICIPANTES];

    PulsoSecuenciaNeon secuencia[MAX_PULSOS_SECUENCIA_NEON];
    int cantidadPulsos = 0;
    int indiceMuestra = 0;
    int numeroRonda = 0;

    FaseSecuenciaNeon fase = FASE_SECUENCIA_PREPARACION;
    bool pulsoVisible = false;

    float tiempoPreparacion = 0.0f;
    float tiempoPasoMuestra = 0.0f;
    float tiempoRespuesta = 0.0f;
    float tiempoResolucion = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: brillo (0..1) de cada panel gigante. Solo lo enciende la
    // fase de MOSTRAR; se apaga en ~0,1 s (antes de la pausa entre pulsos).
    float brilloPanel[CANTIDAD_PULSOS_NEON]{};
    float pulsoCamara = 0.0f;
    float tiempoFase = 0.0f;
    float temporizadorAmbiente = 0.0f;
    ParticulaSecuenciaNeon particulas[MAX_PARTICULAS_SECUENCIA_NEON];

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
