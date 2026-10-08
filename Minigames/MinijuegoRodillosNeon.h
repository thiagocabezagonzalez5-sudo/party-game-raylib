#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// RODILLOS NEON
//==================================================
//
// Todos contra todos (2-4). Cada jugador tiene su maquina arcade con 3
// rodillos verticales que giran a la vez. La accion detiene los rodillos de
// izquierda a derecha: el simbolo que queda en la linea central es el
// resultado. 3 iguales = 10 puntos (estrellas = 20), 2 iguales = 3. Detener
// justo en la linea convierte el simbolo en comodin. En cada ronda hay un
// fallo de sistema (glitch) que acelera o invierte un rodillo durante 1 s.
// 5 rondas, cada una mas rapida.
//==================================================

inline constexpr int CANTIDAD_RODILLOS_NEON = 3;
inline constexpr int SIMBOLOS_POR_RODILLO = 10;
inline constexpr int TIPOS_SIMBOLO_NEON = 5;
inline constexpr int RONDAS_RODILLOS_NEON = 5;


enum FaseRodillosNeon
{
    FASE_RODILLOS_PREPARACION = 0,
    FASE_RODILLOS_JUGANDO,
    FASE_RODILLOS_TERMINADO
};


enum EtapaRondaRodillos
{
    ETAPA_RODILLOS_AVISO = 0,
    ETAPA_RODILLOS_GIRO,
    ETAPA_RODILLOS_RESULTADO
};


// Tipos: 0 triangulo, 1 circulo, 2 cuadrado, 3 rombo, 4 estrella.
inline constexpr int SIMBOLO_ESTRELLA_NEON = 4;


enum EstadoRodilloNeon
{
    RODILLO_GIRANDO = 0,
    RODILLO_FRENANDO,
    RODILLO_DETENIDO
};


enum ResultadoRondaRodillos
{
    RONDA_RODILLOS_NADA = 0,
    RONDA_RODILLOS_PAR,
    RONDA_RODILLOS_TRIPLE,
    RONDA_RODILLOS_TRIPLE_ESTRELLA
};


struct RodilloNeon
{
    int tira[SIMBOLOS_POR_RODILLO]{};

    float angulo = 0.0f;           // radianes; el simbolo k esta en angulo + k * paso
    EstadoRodilloNeon estado = RODILLO_GIRANDO;
    int ultimoCruce = 0;

    float anguloInicialFreno = 0.0f;
    float anguloFinalFreno = 0.0f;
    float tiempoFreno = 0.0f;

    int simbolo = -1;              // tipo de simbolo al detener (-1 = ninguno)
    bool comodin = false;

    float tiempoEnRodillo = 0.0f;  // IA: tiempo esperando a ser el rodillo actual
};


struct EstadoJugadorRodillos
{
    int puntos = 0;
    int triples = 0;
    int puntosRonda = 0;
    ResultadoRondaRodillos resultadoRonda = RONDA_RODILLOS_NADA;

    RodilloNeon rodillos[CANTIDAD_RODILLOS_NEON];
    int rodilloActual = 0;          // 0..2 gira; 3 = todos detenidos
    float tiempoGesto = 0.0f;       // gesto del personaje al detener un rodillo
    int maquina = -1;
    float posicionX = 0.0f;

    // IA (bots y humanos desconectados).
    float sigmaTiming = 0.08f;
    float errorTiming = 0.0f;
    int objetivoSimbolo = -1;
    float retardoBot = 0.5f;
    float tiempoRodilloActual = 0.0f;
    float anguloPrevioObjetivo = 0.0f;
    bool anguloPrevioValido = false;
};


struct MinijuegoRodillosNeon
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorRodillos estadosJugadores[MAX_PARTICIPANTES];

    Camera3D camara{};
    FaseRodillosNeon fase = FASE_RODILLOS_PREPARACION;
    EtapaRondaRodillos etapa = ETAPA_RODILLOS_AVISO;

    int ronda = 0;
    int cantidadMaquinas = 0;

    // Fallo de sistema de la ronda: 0 pendiente, 1 aviso, 2 efecto, 3 hecho.
    int estadoGlitch = 0;
    int glitchJugador = -1;
    int glitchRodillo = -1;
    bool glitchInvierte = false;
    float tiempoGlitch = 0.0f;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoEtapa = 0.0f;
    float tiempoAnimacion = 0.0f;
    float enfriamientoTick = 0.0f;

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
