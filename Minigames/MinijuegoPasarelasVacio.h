#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


inline constexpr int MAX_PLATAFORMAS_PASARELAS = 9;
inline constexpr int MAX_PARTICULAS_PASARELAS = 260;
inline constexpr int MAX_AVISOS_PASARELAS = 6;


enum FasePasarelasVacio
{
    FASE_PASARELAS_PREPARACION = 0,
    FASE_PASARELAS_CARRERA,
    FASE_PASARELAS_TERMINADO
};


struct EstadoJugadorPasarelasVacio
{
    bool eliminado = false;
    bool llegoMeta = false;
    int posicionFinal = 0;
    int indiceObjetivoBot = 1;
    int ultimoSoporteBot = -1;
    float distanciaSaltoBot = 0.78f;
    float desfaseXBot = 0.0f;
    float pausaBot = 0.0f;
    float progresoMaximo = 0.0f;
};


struct EstadoPlataformaPasarelas
{
    bool activada = false;
    bool ocupada = false;
    bool cayendo = false;
    float tiempoDerrumbe = 1.5f;
    float velocidadCaida = 0.0f;
};


// Polvo de piedra, chispas de runa y confeti (solo visuales).
struct ParticulaPasarelasVacio
{
    bool activa = false;
    bool brillo = false;            // se dibuja en la pasada aditiva
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
};


// Anuncio flotante sobre el mundo («¡J2 AL VACÍO!», «¡J1 LLEGÓ!»).
struct AvisoPasarelasVacio
{
    bool activo = false;
    bool meta = false;
    int jugador = 0;
    Vector3 posicion{};
    float vida = 0.0f;
};


// Caida de un eliminado hacia la nebulosa (la logica ya lo solto).
struct CaidaJugadorPasarelas
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float tiempo = 0.0f;
    float angulo = 0.0f;
};


struct MinijuegoPasarelasVacio
{
    ResultadoMinijuego resultado{};
    EstadoJugadorPasarelasVacio estadosJugadores[MAX_PARTICIPANTES];
    EstadoPlataformaPasarelas estadosPlataformas[MAX_PLATAFORMAS_PASARELAS];

    BloquePrueba plataformas[MAX_PLATAFORMAS_PASARELAS];
    int cantidadPlataformas = 0;

    Camera3D camara{};
    FasePasarelasVacio fase = FASE_PASARELAS_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    AudioJuego* audio = nullptr;

    // Solo visual: efectos, anuncios y memoria de eventos. Nada de esto
    // toca la logica ni consume el GetRandomValue de los bots.
    unsigned int semillaVisual = 2654435761u;
    float tiempoAnuncio = -1.0f;
    bool carreraVista = false;
    bool finVisto = false;
    bool activadaVista[MAX_PLATAFORMAS_PASARELAS] = {};
    bool caidaVista[MAX_PLATAFORMAS_PASARELAS] = {};
    bool grietaVista[MAX_PLATAFORMAS_PASARELAS] = {};
    float tiempoDesplome[MAX_PLATAFORMAS_PASARELAS] = {};
    float destelloPlataforma[MAX_PLATAFORMAS_PASARELAS] = {};
    bool eliminadoVisto[MAX_PARTICIPANTES] = {};
    bool metaVista[MAX_PARTICIPANTES] = {};
    bool enSueloVisto[MAX_PARTICIPANTES] = {};
    float destelloMeta[MAX_PARTICIPANTES] = {};
    CaidaJugadorPasarelas caidas[MAX_PARTICIPANTES];
    ParticulaPasarelasVacio particulasEfecto[MAX_PARTICULAS_PASARELAS];
    AvisoPasarelasVacio avisos[MAX_AVISOS_PASARELAS];

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
