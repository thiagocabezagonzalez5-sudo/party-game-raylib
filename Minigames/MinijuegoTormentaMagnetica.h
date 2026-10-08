#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


const int MAX_PINCHOS_TORMENTA_MAGNETICA = 12;


enum FaseTormentaMagnetica
{
    FASE_MAGNETICA_PREPARACION = 0,
    FASE_MAGNETICA_JUGANDO,
    FASE_MAGNETICA_TERMINADO
};


struct EstadoJugadorTormentaMagnetica
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
};


struct PinchoTormentaMagnetica
{
    bool activo = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float tiempoVida = 0.0f;

    // Telegrafia: marca en el suelo antes de que el pincho caiga.
    float aviso = 0.0f;
    float avisoTotal = 0.85f;
};


// IA de bots: perciben el cambio de campo con retraso (~0.3 s) y con error
// en la posicion del nucleo; a veces se distraen de los pinchos.
struct EstadoBotTormentaMagnetica
{
    int cambiosVistos = 0;
    float retardo = 0.0f;
    bool campoAtrae = true;
    Vector3 nucleoPercibido{};
    Vector3 ancla{};
    float tiempoAncla = 0.0f;
    float tiempoAtencion = 0.0f;
    bool atento = true;
};


struct MinijuegoTormentaMagnetica
{
    ResultadoMinijuego resultado;

    EstadoJugadorTormentaMagnetica estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    PinchoTormentaMagnetica pinchos[
        MAX_PINCHOS_TORMENTA_MAGNETICA
    ];

    EstadoBotTormentaMagnetica bots[MAX_JUGADORES_PRUEBA];

    BloquePrueba suelo;
    Camera3D camara{};

    Vector3 posicionNucleo{};
    bool campoAtrae = true;

    FaseTormentaMagnetica fase = FASE_MAGNETICA_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 0.0f;
    float tiempoJugado = 0.0f;
    float tiempoHastaCambioCampo = 3.0f;
    float tiempoHastaPincho = 1.2f;
    float tiempoAnimacion = 0.0f;

    int cambiosCampo = 0;
    float tiempoDesdeCambio = 99.0f;

    AudioJuego* audio = nullptr;

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
