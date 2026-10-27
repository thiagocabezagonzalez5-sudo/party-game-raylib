#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_PROYECTILES_TANQUES_PLASMA = 40;
inline constexpr int MAX_EFECTOS_TANQUES_PLASMA = 16;
inline constexpr int MAX_PARTICULAS_TANQUES_PLASMA = 240;


enum FaseTanquesPlasma
{
    FASE_TANQUES_PREPARACION = 0,
    FASE_TANQUES_COMBATE,
    FASE_TANQUES_TERMINADO
};


// El espacio de juego es el plano del suelo de la arena 3D:
// Vector2.x = eje X del mundo, Vector2.y = eje Z del mundo.
struct EstadoTanquePlasma
{
    Vector2 posicion{};
    Vector2 direccion = { 0.0f, -1.0f };

    int vidas = 2;
    int impactosAcertados = 0;
    bool eliminado = false;

    float recarga = 0.0f;
    float invulnerabilidad = 0.0f;
    float faseBot = 0.0f;

    // Solo visual: orientacion suavizada del casco (radianes sobre XZ).
    float anguloCasco = 0.0f;
    float tiempoMuerto = 0.0f;

    // Solo visual: orugas, culatazo, golpe recibido y lo ya mostrado.
    Vector2 posicionVista{};
    bool posicionIniciada = false;
    float giroOrugas = 0.0f;
    float marcha = 0.0f;            // 0 quieto .. 1 avanzando
    float retroceso = 0.0f;         // culatazo del canon
    float fogonazo = 0.0f;          // destello en la boca del canon
    float sacudida = 0.0f;
    float destelloGolpe = 0.0f;
    float recargaVista = 0.0f;
    int vidasVistas = 2;
    bool eliminacionVista = false;
};


struct ProyectilTanquePlasma
{
    bool activo = false;
    int propietario = -1;
    Vector2 posicion{};
    Vector2 velocidad{};
    float tiempoVida = 0.0f;
};


// Destello breve en el punto de un impacto (solo visual).
struct EfectoImpactoTanque
{
    bool activo = false;
    Vector2 posicion{};
    float tiempo = 0.0f;
    float tamano = 1.0f;
    int propietario = -1;
};


// Chispas, polvo, humo y confeti del canon (solo visuales).
struct ParticulaTanquesPlasma
{
    bool activa = false;
    bool brillo = false;            // se dibuja en la pasada aditiva
    bool humo = false;              // bocanada translucida que crece
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
};


struct MinijuegoTanquesPlasma
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoTanquePlasma tanques[MAX_PARTICIPANTES];
    ProyectilTanquePlasma proyectiles[MAX_PROYECTILES_TANQUES_PLASMA];
    EfectoImpactoTanque efectos[MAX_EFECTOS_TANQUES_PLASMA];

    FaseTanquesPlasma fase = FASE_TANQUES_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoCombate = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: anuncios y particulas. Nada de esto toca la logica
    // ni consume el GetRandomValue de los bots.
    unsigned int semillaVisual = 2246822519u;
    float tiempoAnuncio = -1.0f;
    bool combateVisto = false;
    bool finVisto = false;
    float destelloCristal[3] = {};  // racimos de cobertura golpeados
    ParticulaTanquesPlasma particulas[MAX_PARTICULAS_TANQUES_PLASMA];

    void Inicializar();

    void Reiniciar(
        Participante participantes[]
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[]
    );

    void Dibujar(
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
