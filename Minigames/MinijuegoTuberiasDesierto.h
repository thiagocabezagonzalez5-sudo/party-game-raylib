#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// TUBERIAS DEL DESIERTO
//==================================================
//
// Todos contra todos (2-4) en 5 rondas. Un oasis-cisterna alimenta una red
// de acueductos de piedra que baja por una pared de arenisca hacia 4-6
// canteros. Las tuberias se cruzan (una pasa por delante de la otra y a
// veces por detras de columnas). Hay un canto dorado (+3), un canto
// espejismo (-1, se revela al final) y el resto vacios (0). Se observa el
// entramado 5 s, cada jugador elige una entrada (varios pueden coincidir),
// se abre la compuerta y el agua recorre las tuberias. Gana quien suma mas
// puntos; desempate por acierto en la ultima ronda.
//==================================================

inline constexpr int MAX_ENTRADAS_TUBERIAS = 6;
inline constexpr int MAX_ETAPAS_TUBERIAS = 7;
inline constexpr int RONDAS_TUBERIAS = 5;


enum FaseTuberiasDesierto
{
    FASE_TUBERIAS_PREPARACION = 0,
    FASE_TUBERIAS_OBSERVAR,
    FASE_TUBERIAS_ELEGIR,
    FASE_TUBERIAS_COMPUERTA,
    FASE_TUBERIAS_FLUJO,
    FASE_TUBERIAS_REVELAR,
    FASE_TUBERIAS_TERMINADO
};


struct EstadoJugadorTuberias
{
    bool participa = false;
    int seleccion = 0;
    bool bloqueado = false;
    int puntos = 0;
    int deltaRonda = 0;
    bool aciertoUltima = false;

    bool izquierdaPrevia = false;
    bool derechaPrevia = false;

    // IA (bots y humanos desconectados).
    int objetivo = 0;
    float tiempoDecision = 0.0f;
    float tiempoPaso = 0.0f;
};


struct MinijuegoTuberiasDesierto
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorTuberias estadosJugadores[MAX_PARTICIPANTES];

    int ronda = 0;
    int cantidadEntradas = 4;
    int etapas = 3;
    int carril[MAX_ENTRADAS_TUBERIAS][MAX_ETAPAS_TUBERIAS + 1]{};
    int oroCantaro = 0;
    int mirajeCantaro = 1;
    int entradaGanadora = 0;

    float progresoAgua = 0.0f;
    float compuerta = 0.0f;
    bool mirajeRevelado = false;

    bool empate = false;
    bool partidaValida = false;

    Camera3D camara{};
    FaseTuberiasDesierto fase = FASE_TUBERIAS_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoFase = 0.0f;
    float tiempoAnimacion = 0.0f;

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
