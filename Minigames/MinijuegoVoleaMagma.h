#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// VOLEA DE MAGMA
//==================================================
//
// Voleibol por equipos (2 vs 2; con 3 jugadores 2 vs 1 y con 2, 1 vs 1) en
// un estadio volcanico de noche: cancha de obsidiana sobre una isla de
// basalto en un lago de lava, gradas talladas en la roca con publico de
// piedra y volcanes al fondo. Una roca de magma cruza la red;
// cada equipo puede tocarla hasta 2 veces antes de devolverla. Golpear
// (accion) la manda a la mitad rival hacia donde apunta el stick; saltar y
// golpear es un remate. Si la roca cae en tu mitad, punto para el rival.
// Cada 3 puntos la roca se enfria y despues se recalienta: sobrecalentada,
// deja un charco de lava 4 s que ralentiza ese lado. Primero a 7 puntos o
// 75 s (si empatan: punto de oro de hasta 20 s).
//==================================================

inline constexpr int MAX_CHARCOS_VOLEA = 4;
inline constexpr int MAX_PARTICULAS_VOLEA = MAX_PARTICULAS_TIERRA;
inline constexpr int LARGO_ESTELA_VOLEA = 8;
inline constexpr int MAX_CHISPAS_VOLEA = 220;


enum FaseVoleaMagma
{
    FASE_VOLEA_PREPARACION = 0,
    FASE_VOLEA_JUGANDO,
    FASE_VOLEA_TERMINADO
};


enum EstadoPelotaVolea
{
    PELOTA_VOLEA_SAQUE = 0,    // flota esperando el saque
    PELOTA_VOLEA_EN_JUEGO,
    PELOTA_VOLEA_PUNTO         // ya cayo; pausa antes del siguiente saque
};


enum MensajeVolea
{
    MENSAJE_VOLEA_NINGUNO = 0,
    MENSAJE_VOLEA_PUNTO,
    MENSAJE_VOLEA_ENFRIA,
    MENSAJE_VOLEA_ORO,
    MENSAJE_VOLEA_CHARCO
};


struct EstadoJugadorVolea
{
    int equipo = -1;
    int orden = 0;                 // puesto dentro del equipo (delantero/zaguero)

    float recargaGolpe = 0.0f;
    float tiempoSwing = 0.0f;

    // IA (bots y humanos desconectados).
    float habilidad = 1.0f;
    float retardoGolpe = -1.0f;
    float apuntarX = 5.0f;
    float apuntarZ = 0.0f;
    float tiempoDecision = 0.0f;
    float ruidoX = 0.0f;
    float ruidoZ = 0.0f;
};


struct PelotaVolea
{
    Vector3 posicion{};
    Vector3 velocidad{};
    EstadoPelotaVolea estado = PELOTA_VOLEA_SAQUE;
    float temperatura = 0.6f;     // 0 fria, 1 al rojo; >= 0.85 sobrecalentada
    float tiempoEstado = 0.0f;
    float enfriamientoToque = 0.0f;
    Vector3 estela[LARGO_ESTELA_VOLEA]{};
    float tiempoEstela = 0.0f;
};


struct CharcoLavaVolea
{
    bool activo = false;
    float x = 0.0f;
    float z = 0.0f;
    float tiempo = 0.0f;
};


// Chispa, esquirla, vapor o papelito (solo visual).
struct ChispaVolea
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    float giro = 0.0f;
    bool brilla = false;
    bool humo = false;
};


struct MinijuegoVoleaMagma
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorVolea estadosJugadores[MAX_PARTICIPANTES];
    int cantidadJugadoresEquipo[2]{};

    PelotaVolea pelota;
    CharcoLavaVolea charcos[MAX_CHARCOS_VOLEA];
    BloquePrueba bloques[1];
    ParticulaTierra particulas[MAX_PARTICULAS_VOLEA];

    int puntos[2]{};
    int puntosTotales = 0;
    int equipoSaque = 0;

    // Reglas de toques: maximo 2 por equipo antes de cruzar la red.
    int toques[2]{};
    int equipoUltimoToque = -1;
    int ultimoJugador = -1;
    float avisoToques = 0.0f;

    int equipoGanador = -1;
    bool empate = false;
    bool modoOro = false;
    bool partidaValida = false;

    MensajeVolea mensaje = MENSAJE_VOLEA_NINGUNO;
    int equipoMensaje = -1;
    float tiempoMensaje = 0.0f;

    Camera3D camara{};
    FaseVoleaMagma fase = FASE_VOLEA_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: Actualizar compara el estado antes y despues de cada
    // paso de la partida; el azar sale de semillaVisual.
    ChispaVolea chispas[MAX_CHISPAS_VOLEA];
    unsigned int semillaVisual = 1u;
    float tiempoVisual = 0.0f;
    float tiempoAnuncioInicio = -1.0f;
    float tiempoFin = -1.0f;
    float destelloJugador[MAX_PARTICIPANTES] = {};
    float festejo[2] = {};
    float pulsoMarcador[2] = {};
    float sacudidaRed = 0.0f;
    float destelloGolpe = 0.0f;
    Vector3 posicionGolpe{};
    float tiempoImpacto = -1.0f;
    Vector3 posicionImpacto{};

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

    // Reglas de la partida (Actualizar le suma los efectos visuales).
    void ActualizarPartida(
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
