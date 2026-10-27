#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


enum FasePasoSilencioso
{
    FASE_PASO_PREPARACION = 0,
    FASE_PASO_CARRERA,
    FASE_PASO_TERMINADO
};


struct EstadoJugadorPasoSilencioso
{
    float progreso = 0.0f;
    int penalizaciones = 0;
    bool llegoMeta = false;
    bool castigadoEnAlerta = false;

    bool botAvanzando = false;
    bool botAnticipo = false;
    float reaccionBot = 0.0f;

    // Feedback visual del castigo.
    float progresoVisual = 0.0f;
    float flashCastigo = 0.0f;
    float perdidaFlotante = 0.0f;

    // Solo visual: andar en puntas de pie, sacudida y llegada.
    float progresoAnterior = 0.0f;
    float marcha = 0.0f;            // 0 quieto .. 1 caminando
    float fasePaso = 0.0f;
    float sacudida = 0.0f;
    float destelloMeta = 0.0f;
    int penalizacionesVistas = 0;
    bool metaVista = false;
};


// Efectos del templo (solo visuales).
struct ParticulaPasoSilencioso
{
    bool activa = false;
    bool brillo = false;            // se dibuja en la pasada aditiva
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
};


static const int MAX_PARTICULAS_PASO = 260;


struct MinijuegoPasoSilencioso
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorPasoSilencioso jugadores[MAX_PARTICIPANTES];

    Camera3D camara{};
    FasePasoSilencioso fase = FASE_PASO_PREPARACION;

    bool centinelaAlerta = false;
    bool avisoPrevio = false;
    float tiempoEnAlerta = 0.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoEstadoCentinela = 0.0f;
    float tiempoCarrera = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: el gigante, anuncios y particulas. Nada de esto toca
    // la logica ni consume el GetRandomValue de los bots.
    unsigned int semillaVisual = 2654435761u;
    float despertar = 0.0f;         // 0 dormido .. 1 mirando (suavizado)
    float giroCabeza = 0.0f;        // grados, barrido de la mirada
    float tiempoAnuncio = -1.0f;
    int tipoAnuncio = 0;
    bool alertaVista = false;
    bool avisoVisto = false;
    bool finVisto = false;
    ParticulaPasoSilencioso particulas[MAX_PARTICULAS_PASO];

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
