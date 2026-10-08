#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// BANQUETE TURBO
//==================================================
//
// Todos contra todos en el comedor orbital de una estacion espacial. Un
// robot camarero rellena la bandeja de cada puesto con raciones de comida
// espacial; machacar la accion da un bocado y la barra de la racion baja.
// Algunas raciones son picantes (avisan en rojo): comerlas cuesta toser, pero
// se pueden rechazar con el salto. Las doradas valen doble. Gana quien
// termine mas raciones en 30 s; en empate, mas bocados totales.
//==================================================

inline constexpr int MAX_PARTICULAS_BANQUETE = MAX_PARTICULAS_TIERRA;
inline constexpr int LARGO_SECUENCIA_BANQUETE = 96;


enum FaseBanquete
{
    FASE_BANQUETE_PREPARACION = 0,
    FASE_BANQUETE_JUGANDO,
    FASE_BANQUETE_TERMINADO
};


enum TipoRacionBanquete
{
    RACION_BANQUETE_NORMAL = 0,
    RACION_BANQUETE_PICANTE,
    RACION_BANQUETE_DORADA
};


struct RacionBanquete
{
    TipoRacionBanquete tipo = RACION_BANQUETE_NORMAL;
    int bocados = 12;
    int colorTubo = 0;                // indice en la paleta de tubos
};


struct PuestoBanquete
{
    int indiceRacion = 0;
    int bocadosRestantes = 0;
    int bocadosRacion = 1;
    float llegada = 0.0f;             // 0..1: la racion desliza hasta la bandeja
    float tiempoRacion = 0.0f;

    float tos = 0.0f;
    float rechazo = 0.0f;
    float enfriamientoBocado = 0.0f;
    float enfriamientoSonido = 0.0f;
    float pulsoBocado = 0.0f;         // gesto de masticar
    float tiempoMensaje = 0.0f;

    // Racion rechazada saliendo de la bandeja.
    float salida = 0.0f;
    RacionBanquete racionSalida{};

    int puntos = 0;
    int raciones = 0;
    int doradas = 0;
    int picantesComidas = 0;
    int picantesRechazadas = 0;
    int bocadosTotales = 0;
};


struct EstadoBotBanquete
{
    float frecuencia = 9.0f;
    float cambioFrecuencia = 0.0f;
    float proximoToque = 0.0f;
    float probabilidadDetectar = 0.7f;
    bool reaccionActiva = false;
    float reaccion = 0.0f;
};


struct MinijuegoBanqueteTurbo
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    ParticulaTierra particulas[MAX_PARTICULAS_BANQUETE];
    RacionBanquete secuencia[LARGO_SECUENCIA_BANQUETE];
    PuestoBanquete puestos[MAX_PARTICIPANTES];
    EstadoBotBanquete bots[MAX_PARTICIPANTES];
    float posicionX[MAX_PARTICIPANTES]{};

    Camera3D camara{};
    FaseBanquete fase = FASE_BANQUETE_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Robot camarero.
    float robotX = 0.0f;
    float robotObjetivoX = 0.0f;
    int robotPuesto = -1;
    float robotBrazo = 0.0f;

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
