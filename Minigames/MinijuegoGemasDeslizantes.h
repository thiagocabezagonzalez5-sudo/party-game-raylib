#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"

#include <cstdint>


//==================================================
// GEMAS DESLIZANTES (DUELO 1 VS 1)
//==================================================
//
// Puzle de deslizar en el taller de un joyero. Cada jugador tiene su
// bandeja de terciopelo con 8 gemas y un hueco (3x3); en el pedestal del
// centro brilla el dibujo que hay que armar (corona, flor, tesoro...).
// Mover (o el stick) DESLIZA una gema hacia ese lado: entra en el hueco
// desde el lado opuesto. El dibujo se compara por COLOR (y cada color tiene
// su figura grabada, para quien no distingue colores), asi que hay gemas
// repetidas y varias formas de resolverlo.
//
// - Mejor de 3 rondas (gana quien llegue a 2). Las dos bandejas reciben
//   EXACTAMENTE el mismo tablero mezclado: se elige al azar entre los
//   estados a 9..13 movimientos de la solucion (siempre resoluble, nunca
//   trivial). Cada ronda usa un dibujo y un tablero nuevos.
// - Gana la ronda quien arme primero el dibujo. Tope de 28 s por ronda: si
//   nadie termina, gana quien tenga mas gemas en su sitio; luego quien este
//   a menos movimientos de resolver; luego quien llego antes a su mejor
//   marca; luego menos movimientos; al final alterna por ronda. Nunca hay
//   empate.
// - Bots: usan el resolvedor real (distancias BFS desde el dibujo objetivo)
//   pero mueven con tiempos humanos, dudas y movimientos de mas.
//
// Un jugador desconectado pasa a ser bot. Solo participan los dos primeros
// participantes activos.
//==================================================

inline constexpr int CELDAS_GEMAS = 9;
inline constexpr int GEMAS_POR_TABLERO = 8;
inline constexpr int GEMA_HUECO = 8;           // valor de celda sin gema
inline constexpr int MAX_RONDAS_GEMAS = 3;
inline constexpr int MAX_PARTICULAS_GEMAS = 220;
inline constexpr int COLA_ENTRADA_GEMAS = 2;


enum FaseGemasDeslizantes
{
    FASE_GEMAS_PREPARACION = 0,    // cuenta regresiva inicial
    FASE_GEMAS_REPARTO,            // ronda nueva: las gemas caen en la bandeja
    FASE_GEMAS_JUGANDO,
    FASE_GEMAS_FIN_RONDA,          // celebracion
    FASE_GEMAS_TERMINADO
};


struct ParticulaGemas
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color = WHITE;
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    int tipo = 0;                  // 0 confeti, 1 destello, 2 polvo
};


struct TableroGemas
{
    int participante = -1;         // indice del Participante dueno

    // Gema (0..7) que ocupa cada celda, o GEMA_HUECO.
    uint8_t celda[CELDAS_GEMAS]{};
    uint8_t colorGema[GEMAS_POR_TABLERO]{};   // 1..5

    // Posicion visual de cada gema (relativa al centro de la bandeja).
    float px[GEMAS_POR_TABLERO]{};
    float pz[GEMAS_POR_TABLERO]{};
    float vx[GEMAS_POR_TABLERO]{};
    float vz[GEMAS_POR_TABLERO]{};
    float alza[GEMAS_POR_TABLERO]{};
    float destello[GEMAS_POR_TABLERO]{};

    int correctas = 0;             // gemas en su celda correcta (0..8)
    int distancia = 0;             // movimientos optimos que faltan
    int movimientos = 0;
    int mejorCorrectas = 0;
    float tiempoMejor = 0.0f;      // cuando llego a mejorCorrectas
    float tiempoRonda = 0.0f;

    bool completo = false;
    float tiempoCompleto = 0.0f;
    float tiempoBrillo = 0.0f;     // brillo continuo de la corona al terminar

    // Entrada humana.
    bool dirPrevia[4]{};
    int dirMantenida = -1;
    float tiempoMantenida = 0.0f;
    float proximaRepeticion = 0.0f;
    int cola[COLA_ENTRADA_GEMAS]{};
    int cantidadCola = 0;
    float recarga = 0.0f;
    float avisoBloqueo = 0.0f;

    // Bot.
    float habilidad = 1.0f;
    float tiempoDecision = 0.0f;
    int ultimaDireccion = -1;

    int victorias = 0;
};


struct MinijuegoGemasDeslizantes
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    TableroGemas tableros[2];
    ParticulaGemas particulas[MAX_PARTICULAS_GEMAS];
    int siguienteParticula = 0;

    uint32_t semilla = 1;
    int patronBase = 0;            // dibujo de la ronda 0 (cambia cada partida)
    int patronActual = 0;
    int ronda = 0;                 // 0..2
    int rondasJugadas = 0;
    int ganadorRonda = -1;         // 0 / 1, -1 si aun no hay
    bool rondaPorTiempo = false;
    bool partidaValida = false;

    Camera3D camara{};
    FaseGemasDeslizantes fase = FASE_GEMAS_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoFase = 0.0f;       // tiempo transcurrido de la fase actual
    float tiempoReparto = 0.0f;
    float tiempoAnimacion = 0.0f;
    float brilloReferencia = 0.0f;

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
