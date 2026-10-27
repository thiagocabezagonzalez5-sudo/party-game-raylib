#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


const int MAX_PINCHOS_TORMENTA_MAGNETICA = 12;
const int MAX_PARTICULAS_TORMENTA_MAGNETICA = 260;
const int MAX_ONDAS_TORMENTA_MAGNETICA = 12;
const int MAX_AVISOS_TORMENTA_MAGNETICA = 4;


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

    // Solo visual / audio: flanco de suelo, destello del rayo recibido y
    // caida hacia la fosa tras la eliminacion.
    bool enSueloAnterior = true;
    float destelloGolpe = 0.0f;
    float tiempoCaida = -1.0f;      // < 0: no esta cayendo
    Vector3 posicionCaida{};
    Vector3 velocidadCaida{};
    bool tocoFondo = false;
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


// Efectos de la tormenta (solo visuales).
struct ParticulaTormentaMagnetica
{
    bool activa = false;
    bool brilla = false;            // halo aditivo (chispas)
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    float freno = 1.2f;
    Color color{};
};


// Onda plana: hacia afuera o hacia adentro (cambio de campo, impactos).
struct OndaTormentaMagnetica
{
    bool activa = false;
    bool haciaAdentro = false;
    Vector3 posicion{};
    float radioMaximo = 1.0f;
    float vida = 0.0f;
    float vidaMaxima = 0.5f;
    Color color{};
};


// «¡J2 AFUERA!» sobre el borde por donde cayo.
struct AvisoTormentaMagnetica
{
    bool activo = false;
    int jugador = 0;
    Vector3 posicion{};
    float vida = 0.0f;
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

    // Solo visual: reloj que sigue corriendo al terminar, duracion del
    // campo actual (para el medidor del HUD), lugar que dejo el nucleo y
    // efectos. Nada de esto toca la logica ni su azar.
    float tiempoVisual = 0.0f;
    float duracionCampo = 3.5f;
    Vector3 nucleoAnterior{};
    unsigned int semillaVisual = 2891336453u;
    bool confetiLanzado = false;

    ParticulaTormentaMagnetica particulasEfecto[MAX_PARTICULAS_TORMENTA_MAGNETICA];
    OndaTormentaMagnetica ondas[MAX_ONDAS_TORMENTA_MAGNETICA];
    AvisoTormentaMagnetica avisos[MAX_AVISOS_TORMENTA_MAGNETICA];

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
