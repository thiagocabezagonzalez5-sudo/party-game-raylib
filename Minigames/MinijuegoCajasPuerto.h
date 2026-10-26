#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


inline constexpr int CANTIDAD_CONTENEDORES_CAJAS = 8;
inline constexpr int RONDAS_CAJAS = 3;
inline constexpr int MAX_ELECCIONES_CAJAS = 3;
inline constexpr int MAX_PARTICULAS_CAJAS = 140;
inline constexpr int SENUELOS_POR_JUGADOR_CAJAS = 2;


enum FaseCajasPuerto
{
    FASE_CAJAS_PREPARACION = 0,
    FASE_CAJAS_ESCONDER,
    FASE_CAJAS_CIERRE,
    FASE_CAJAS_ELEGIR,
    FASE_CAJAS_RESOLVER,
    FASE_CAJAS_TERMINADO
};


struct EstadoJugadorCajasPuerto
{
    bool vivo = false;
    bool accionPrevia = false;
    int contenedor = -1;
    int rondasSobrevividas = 0;
    int objetivoBot = -1;
    float retrasoBot = 0.0f;
};


struct ContenedorCajasPuerto
{
    Vector3 posicion{};
    float apertura = 1.0f;
    float humo = 0.0f;
    float caida = 0.0f;
    int ocupante = -1;
    int marcas = 0;
    bool reforzado = false;
    bool elegido = false;
    bool revelado = false;

    // Solo visual: reaccion al golpe de la carga.
    float sacudida = 0.0f;          // 1 al impacto, se apaga sola
    float aviso = 0.0f;             // cartel flotante (1 -> 0)
    int tipoGolpe = 0;              // 0 vacio, 1 aplastado, 2 a salvo
};


// Particulas del puerto: astillas, polvo, chispas y salpicaduras.
struct ParticulaCajasPuerto
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 9.0f;
    Color color = WHITE;
};


// Marcas falsas en la niebla: solo existen cuando el gruero es humano y
// hay un polizon humano que necesita ver su propia marca. Se mueven como
// un jugador mas, asi la marca real no delata su contenedor.
struct SenueloCajasPuerto
{
    Vector3 posicion{};
    float objetivoX = 0.0f;
    float espera = 0.0f;
};


struct MinijuegoCajasPuerto
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorCajasPuerto estadosJugadores[MAX_PARTICIPANTES];
    ContenedorCajasPuerto contenedores[CANTIDAD_CONTENEDORES_CAJAS];
    bool historialElegidos[CANTIDAD_CONTENEDORES_CAJAS]{};
    int ordenElegidos[MAX_ELECCIONES_CAJAS]{};

    Camera3D camara{};
    FaseCajasPuerto fase = FASE_CAJAS_PREPARACION;

    int indiceSolo = -1;
    int ronda = 1;
    int cursor = 3;
    int eleccionesRestantes = 0;
    int cantidadElegidos = 0;
    int pasoResolucion = 0;
    int eliminadosRonda = 0;
    int eliminadosTotal = 0;
    int direccionPrevia = 0;
    int botSoloObjetivo = -1;
    bool accionSoloPrevia = false;
    bool caidaIniciada = false;
    bool detonado = false;
    bool revelacionHecha = false;

    float ganchoX = 0.0f;
    float repeticionCursor = 0.0f;
    float botSoloTemporizador = 0.0f;
    float tiempoPaso = 0.0f;
    float tiempoFinResolucion = 0.0f;
    float tiempoFase = 0.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Estado solo visual (Dibujar es const).
    ParticulaCajasPuerto particulas[MAX_PARTICULAS_CAJAS];
    SenueloCajasPuerto senuelos[MAX_PARTICIPANTES][SENUELOS_POR_JUGADOR_CAJAS];
    float niebla = 0.0f;            // 0..1 cobertura del patio
    float tiempoAnuncio = 0.0f;     // segundos desde el ultimo cambio de fase
    float tiempoSalpicadura = 0.0f;
    unsigned int semillaVisual = 1u;
    FaseCajasPuerto faseAnterior = FASE_CAJAS_PREPARACION;

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
