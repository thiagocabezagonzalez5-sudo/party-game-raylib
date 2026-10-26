#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


enum FaseMinijuegoBarraGiratoria
{
    FASE_BARRA_PREPARACION = 0,
    FASE_BARRA_JUGANDO,
    FASE_BARRA_TERMINADO
};


struct EstadoJugadorBarraGiratoria
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;

    // Evita que una misma barra golpee varias veces en frames consecutivos.
    float cooldownImpacto = 0.0f;

    // Tras recibir una barra el jugador conserva el impulso, pero durante
    // unas decimas no puede corregirlo con movimiento propio.
    float tiempoStunBarra = 0.0f;

    // Estado de la IA cuando el participante es bot o esta desconectado.
    float radioOrbitaBot = 0.0f;
    float errorSaltoBot = 0.0f;
    float enfriamientoSaltoBot = 0.0f;

    // Solo visual / audio: flanco de suelo, destello del golpe y caida
    // hacia el mar de nubes tras la eliminacion.
    bool enSueloAnterior = true;
    float destelloGolpe = 0.0f;
    float tiempoCaida = -1.0f;      // < 0: no esta cayendo
    Vector3 posicionCaida{};
    Vector3 velocidadCaida{};
    bool atravesoNubes = false;
};


// Efectos del templo celeste (solo visuales).
struct ParticulaBarraGiratoria
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    float freno = 1.2f;
    Color color{};
};


// Onda plana que se expande (aterrizaje de la barra, nube atravesada).
struct OndaBarraGiratoria
{
    bool activa = false;
    Vector3 posicion{};
    float radioMaximo = 1.0f;
    float vida = 0.0f;
    float vidaMaxima = 0.5f;
    Color color{};
};


// «¡J2 AFUERA!» sobre el lugar por donde cayo.
struct AvisoBarraGiratoria
{
    bool activo = false;
    int jugador = 0;
    Vector3 posicion{};
    float vida = 0.0f;
};


static const int MAX_PARTICULAS_BARRA = 320;
static const int MAX_ONDAS_BARRA = 12;
static const int MAX_AVISOS_BARRA = 4;


struct MinijuegoBarraGiratoria
{
    ResultadoMinijuego resultado;

    EstadoJugadorBarraGiratoria estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba suelo;
    Camera3D camara{};

    FaseMinijuegoBarraGiratoria fase =
        FASE_BARRA_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoJugado = 0.0f;

    float anguloBarra = 0.0f;
    float velocidadAngular = 0.9f;

    // A los diez segundos aparece una segunda barra desde arriba. Mientras
    // cae solo es visual; su colision se activa al asentarse sobre la primera.
    bool segundaBarraAparecio = false;
    bool segundaBarraLista = false;
    float alturaSegundaBarra = 8.0f;
    float anguloSegundaBarra = 1.5707963f;
    float velocidadAngularSegunda = 1.35f;

    // Solo visual: reloj de animacion, giro acumulado (sin envolver) de
    // los engranajes y efectos. Nada de esto toca la logica.
    float tiempoAnimacion = 0.0f;
    float giroMecanismo = 0.0f;
    float tiempoAterrizajeSegunda = -1.0f;
    unsigned int semillaVisual = 2463534242u;
    bool confetiLanzado = false;

    ParticulaBarraGiratoria particulasEfecto[MAX_PARTICULAS_BARRA];
    OndaBarraGiratoria ondas[MAX_ONDAS_BARRA];
    AvisoBarraGiratoria avisos[MAX_AVISOS_BARRA];

    // Lo conecta GestorMinijuegos; puede ser nullptr.
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
