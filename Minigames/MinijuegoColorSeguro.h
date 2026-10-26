#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


const int CANTIDAD_PLATAFORMAS_COLOR = 7;
const int MAX_PARTICULAS_COLOR_SEGURO = 220;


enum FaseMinijuegoColor
{
    FASE_ELEGIR_PLATAFORMA = 0,
    FASE_CAIDA_PLATAFORMAS
};


enum EstadoPartidaColorSeguro
{
    COLOR_SEGURO_PREPARACION = 0,
    COLOR_SEGURO_JUGANDO,
    COLOR_SEGURO_TERMINADO
};


// Efectos propios del volcan (solo visuales, no afectan el juego).
enum TipoParticulaColorSeguro
{
    PARTICULA_COLOR_GOTA = 0,     // gota de lava con gravedad
    PARTICULA_COLOR_ROCA,         // escombro de basalto
    PARTICULA_COLOR_BRASA,        // brasa que sube (aditiva)
    PARTICULA_COLOR_HUMO          // bocanada translucida
};


struct ParticulaColorSeguro
{
    bool activa = false;
    TipoParticulaColorSeguro tipo = PARTICULA_COLOR_GOTA;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    Color color{ 255, 255, 255, 255 };
};


struct EstadoJugadorColorSeguro
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
    int rondasSobrevividas = 0;

    // Estado de la IA cuando el participante es bot o esta desconectado.
    int rondaBot = -1;
    float retrasoBot = 0.0f;
    Vector3 objetivoBot{};
};


struct MinijuegoColorSeguro
{
    ResultadoMinijuego resultado;

    EstadoJugadorColorSeguro estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba plataformas[CANTIDAD_PLATAFORMAS_COLOR];
    int cantidadPlataformas = 0;

    Camera3D camara{};

    Vector3 posicionCamaraBase{};
    Vector3 objetivoCamaraBase{};

    EstadoPartidaColorSeguro estado =
        COLOR_SEGURO_PREPARACION;

    FaseMinijuegoColor fase =
        FASE_ELEGIR_PLATAFORMA;

    int indicePlataformaSegura = 0;
    int numeroRonda = 1;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 60.0f;
    float tiempoJugado = 0.0f;

    float tiempoFase = 5.0f;
    float duracionElegirPlataforma = 5.0f;
    float duracionCaidaPlataformas = 2.0f;

    // Audio comun (lo conecta GestorMinijuegos; puede ser nullptr).
    AudioJuego* audio = nullptr;

    // Estado visual: avanza en Actualizar y Dibujar solo lo lee.
    float tiempoAnimacion = 0.0f;
    float tiempoAnuncio = 0.0f;         // desde que la TV mostro el color
    float acumuladorBrasas = 0.0f;
    float acumuladorHumo = 0.0f;
    unsigned int semillaEfectos = 12345u;
    float salpicaduraPlataforma[CANTIDAD_PLATAFORMAS_COLOR]{};
    float emergerPlataforma[CANTIDAD_PLATAFORMAS_COLOR]{};
    float efectoEliminacion[MAX_PARTICIPANTES]{};
    Vector3 posicionEliminacion[MAX_PARTICIPANTES]{};
    ParticulaColorSeguro efectos[MAX_PARTICULAS_COLOR_SEGURO];

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
