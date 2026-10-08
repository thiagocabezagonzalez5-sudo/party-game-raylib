#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// TREPA EL MASTIL
//==================================================
//
// Todos contra todos a bordo de un barco explorador en alta mar. Cada
// jugador trepa su propio mastil alternando SALTO y ACCION (una mano y la
// otra); repetir la misma tecla resbala. A mitad de la ronda suben olas que
// balancean el barco: trepar cuando el mastil se inclina hacia tu lado da
// bonus y hacerlo en contra te frena. Cuervos con aviso cruzan los carriles
// y tiran 1 m a quien tocan. Gana el primero en llegar a la cofa; a los
// 45 s gana quien este mas alto.
//==================================================

inline constexpr int MAX_PARTICULAS_MASTIL = MAX_PARTICULAS_TIERRA;


enum FaseMastil
{
    FASE_MASTIL_PREPARACION = 0,
    FASE_MASTIL_JUGANDO,
    FASE_MASTIL_TERMINADO
};


struct TrepadorMastil
{
    float altura = 0.0f;              // metros desde la cubierta hasta los pies
    int ultimaTecla = 0;              // 0 ninguna, 1 salto, 2 accion
    int lado = 1;                     // -1 babor, +1 estribor
    float enfriamiento = 0.0f;
    float enfriamientoSonido = 0.0f;
    float enfriamientoCaida = 0.0f;
    float inmunidad = 0.0f;
    float mano = 0.0f;                // -1 izquierda, +1 derecha (visual)
    float pulso = 0.0f;
    float tiempoMensaje = 0.0f;
    bool llego = false;
};


struct CuervoMastil
{
    bool activo = false;
    bool enAviso = false;
    float tiempoAviso = 0.0f;
    float altura = 0.0f;
    float x = 0.0f;                   // desplazamiento respecto al mastil
    float direccion = 1.0f;
    float fase = 0.0f;
    bool golpeo = false;
    float proximo = 0.0f;
};


struct EstadoBotMastil
{
    float frecuencia = 5.5f;
    float cambioFrecuencia = 0.0f;
    float proximoToque = 0.0f;
    bool aprovechaInclinacion = false;
};


struct MinijuegoTrepaMastil
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    ParticulaTierra particulas[MAX_PARTICULAS_MASTIL];
    TrepadorMastil trepadores[MAX_PARTICIPANTES];
    CuervoMastil cuervos[MAX_PARTICIPANTES];
    EstadoBotMastil bots[MAX_PARTICIPANTES];
    float posicionX[MAX_PARTICIPANTES]{};

    Camera3D camara{};
    FaseMastil fase = FASE_MASTIL_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Balanceo del barco: -1 (izquierda) a +1 (derecha).
    float inclinacion = 0.0f;
    float intensidadOlas = 0.0f;
    float faseOlas = 0.0f;

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
