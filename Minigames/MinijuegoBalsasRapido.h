#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// BALSAS DEL RAPIDO
//==================================================
//
// Carrera de balsas por equipos (2 vs 2; con 3 jugadores 2 vs 1 y con 2,
// 1 vs 1) por un rio de selva. Cada equipo comparte una balsa en su carril
// (dos carriles separados por una isla). Cada jugador rema en SU lado:
// una palada en el lado izquierdo empuja y gira la balsa hacia la derecha
// y viceversa. Paladas de ambos lados casi a la vez (0.25 s) dan un
// bonus y avanzan recto. Rocas y troncos frenan, los remolinos giran la
// balsa y las bananas dan un boost corto. A mitad del recorrido hay una
// bifurcacion: cascada corta con rocas o remanso largo y seguro.
// Con un solo jugador por balsa, ese jugador rema ambos lados: sin
// direccion rema los dos; con IZQ/DER gira hacia ese lado (con ayuda de
// estabilidad). Gana la primera balsa en la meta (120 u) o la mas
// avanzada a los 70 s; empate solo si van exactamente igual.
//==================================================

inline constexpr int MAX_OBSTACULOS_BALSAS = 40;
inline constexpr int MAX_PARTICULAS_BALSAS = MAX_PARTICULAS_TIERRA;


enum FaseBalsasRapido
{
    FASE_BALSAS_PREPARACION = 0,
    FASE_BALSAS_JUGANDO,
    FASE_BALSAS_TERMINADO
};


enum TipoObstaculoBalsas
{
    OBSTACULO_BALSAS_ROCA = 0,
    OBSTACULO_BALSAS_TRONCO,
    OBSTACULO_BALSAS_DIVISOR,     // isla que separa la cascada del remanso
    OBSTACULO_BALSAS_REMOLINO,
    OBSTACULO_BALSAS_BANANA
};


struct ObstaculoBalsas
{
    TipoObstaculoBalsas tipo = OBSTACULO_BALSAS_ROCA;
    float p = 0.0f;         // avance a lo largo del rio
    float desvio = 0.0f;    // posicion lateral respecto al centro del carril
    float a = 0.0f;         // radio (roca/remolino/banana) o medio ancho
    float b = 0.0f;         // medio largo (tronco/divisor)
};


struct EstadoJugadorBalsas
{
    int equipo = -1;
    int lado = 0;                  // 0 izquierdo, 1 derecho, 2 ambos (solo)
    float recarga = 0.0f;

    // IA (bots y humanos desconectados).
    float temporizador = 0.0f;
    float periodo = 0.42f;
};


struct BalsaRio
{
    bool activa = false;
    int equipo = 0;
    int cantidad = 0;              // jugadores en la balsa

    float centroCarril = 0.0f;
    float x = 0.0f;
    float p = 0.0f;
    float rumbo = 0.0f;            // radianes; positivo = hacia +x
    float omega = 0.0f;
    float velocidad = 0.0f;

    float tiempoBoost = 0.0f;
    float tiempoImpacto = 0.0f;
    float tiempoSync = 0.0f;
    float tiempoPalada[2]{ 9.0f, 9.0f };
    bool paladaSinPareja[2]{};

    bool terminada = false;
    int bananas = 0;
    bool recogidas[MAX_OBSTACULOS_BALSAS]{};

    // IA de rumbo del equipo.
    int ruta = 0;                  // 0 remanso seguro, 1 cascada
    float objetivoX = 0.0f;
    float ruido = 0.0f;
    float tiempoRuido = 0.0f;
};


// Solo visual: avisos y efectos de cada balsa (no afecta la logica).
struct VisualBalsaRio
{
    float destello = 0.0f;         // 1 al sincronizar o tomar banana
    float avisoSync = 0.0f;        // texto «¡SINCRO!» sobre la balsa
    float avisoBanana = 0.0f;      // texto «¡TURBO!»
    float avisoChoque = 0.0f;      // texto «¡CHOQUE!»
    float sacudida = 0.0f;         // cabeceo corto tras un choque
    float estela = 0.0f;           // intensidad suavizada de la estela
    float anuncioRuta = 0.0f;      // «¡CASCADA!» / «¡REMANSO!» al entrar
    int ruta = -1;                 // -1 sin elegir, 0 cascada, 1 remanso
    float anuncioMeta = 0.0f;
    float tiempoSyncAnterior = 0.0f;
    float tiempoImpactoAnterior = 0.0f;
    int bananasAnterior = 0;
    bool terminadaAnterior = false;
};


inline constexpr int MAX_PARTICULAS_VISUAL_BALSAS = 220;

struct ParticulaVisualBalsas
{
    bool activa = false;
    int tipo = 0;                  // 0 espuma, 1 confeti, 2 hoja, 3 chispa
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float giro = 0.0f;
    Color color{ 255, 255, 255, 255 };
};


struct MinijuegoBalsasRapido
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorBalsas estadosJugadores[MAX_PARTICIPANTES];
    int cantidadJugadoresEquipo[2]{};
    BalsaRio balsas[2];
    ObstaculoBalsas obstaculos[MAX_OBSTACULOS_BALSAS];
    int cantidadObstaculos = 0;
    ParticulaTierra particulas[MAX_PARTICULAS_BALSAS];

    int equipoGanador = -1;
    bool empate = false;
    bool partidaValida = false;

    float tiempoSonidoPalada = 0.0f;

    Camera3D camara{};
    float focoCamara = 0.0f;
    float separacionCamara = 0.0f;

    FaseBalsasRapido fase = FASE_BALSAS_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual (azar con semilla propia; nunca GetRandomValue).
    VisualBalsaRio visualBalsas[2];
    ParticulaVisualBalsas particulasVisual[MAX_PARTICULAS_VISUAL_BALSAS];
    unsigned int semillaVisual = 1u;
    float tiempoVisual = 0.0f;
    float anuncioBifurcacion = -1.0f;   // >= 0: segundos desde el aviso
    bool bifurcacionAnunciada = false;
    float tiempoConfeti = 0.0f;
    float acumuladorAmbiente = 0.0f;

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

    // Libera lo horneado en la GPU (mallas, material y shader); se rehace
    // solo la proxima vez que se dibuje.
    void Descargar();

    // Reglas sin cambios; Actualizar la llama y despues anima lo visual.
    void ActualizarLogica(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[]
    );
};
