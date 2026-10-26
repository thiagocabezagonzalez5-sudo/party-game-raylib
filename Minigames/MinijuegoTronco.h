#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


enum AccionTronco
{
    ACCION_TRONCO_TIRAR = 0,
    ACCION_TRONCO_EMPUJAR
};


// Pool fijo de particulas (solo visual): aserrin, chispas, astillas,
// confeti y ambiente (hojas y motas de aserrin al sol).
static const int MAX_PARTICULAS_TRONCO = 220;


enum TipoParticulaTronco
{
    PARTICULA_TRONCO_ASERRIN = 0,
    PARTICULA_TRONCO_CHISPA,
    PARTICULA_TRONCO_ASTILLA,
    PARTICULA_TRONCO_CONFETI,
    PARTICULA_TRONCO_HOJA,
    PARTICULA_TRONCO_MOTA,
    PARTICULA_TRONCO_POLVO
};


struct ParticulaTronco
{
    bool activa = false;
    int tipo = PARTICULA_TRONCO_ASERRIN;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.05f;
    float giro = 0.0f;
    Color color{ 255, 255, 255, 255 };
};


enum EstadoPartidaTronco
{
    TRONCO_ESPERANDO_JUGADORES = 0,
    TRONCO_PREPARANDO,
    TRONCO_JUGANDO,
    TRONCO_FINALIZADO
};


struct EstadoJugadorTronco
{
    bool respondio = false;
    bool acerto = false;

    float animacionGolpe = 0.0f;

    // Solo para bots: segundos hasta responder el golpe actual.
    // Negativo significa que todavia no eligio un tiempo de reaccion.
    float tiempoReaccionBot = -1.0f;
};


struct EstadoEquipoTronco
{
    float progresoCorte = 0.0f;

    float posicionSierra = -0.72f;
    float objetivoSierra = -0.72f;
    int direccionSierra = 1;

    float tiempoCoordinacion = 0.0f;
    float tiempoBloqueo = 0.0f;
    float tiempoFeedback = 0.0f;

    bool golpeEnCurso = false;
    bool ultimoGolpeCorrecto = false;

    // Solo visual: no intervienen en las reglas.
    float sacudida = 0.0f;          // temblor de sierra y tronco tras un error
    float destello = 0.0f;          // luz en el corte tras una pasada buena
    float avisoError = 0.0f;        // cartel «¡DESCOORDINADOS!»
    float avisoHito = 0.0f;         // cartel «¡25 %!», «¡50 %!»...
    int hito = 0;
    float caidaTronco = -1.0f;      // segundos desde que se partio (< 0: entero)
};


struct MinijuegoTronco
{
    ResultadoMinijuego resultado;

    Camera3D camara{};

    EstadoJugadorTronco estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoEquipoTronco equipos[2];

    int equipoPorJugador[
        MAX_JUGADORES_PRUEBA
    ] = { -1, -1, -1, -1 };

    int ordenEnEquipoPorJugador[
        MAX_JUGADORES_PRUEBA
    ] = { -1, -1, -1, -1 };

    int cantidadJugadoresEquipo[2] =
    {
        0,
        0
    };

    EstadoPartidaTronco estado =
        TRONCO_ESPERANDO_JUGADORES;

    int jugadoresEnPartida = 0;
    int equipoGanador = -1;

    float tiempoPreparacion = 0.0f;
    float tiempoPartida = 0.0f;

    bool partidaValida = false;
    bool empate = false;

    // Segundos restantes del aviso "SE RECONFIGURARON LOS EQUIPOS".
    float avisoReconfiguracion = 0.0f;

    // Lo conecta GestorMinijuegos; puede ser nullptr (sin sonido).
    AudioJuego* audio = nullptr;

    ParticulaTronco particulas[MAX_PARTICULAS_TRONCO];
    float tiempoAnimacion = 0.0f;
    float tiempoFinalizado = 0.0f;
    float acumuladorAmbiente = 0.0f;

    void Inicializar();

    void ConfigurarJugadores(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    ) const;

    void Reiniciar(
        const Participante participantes[],
        int cantidadMaxima
    );

    void PrepararEquipos(
        const Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        int cantidadMaxima,
        const Participante participantes[]
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
