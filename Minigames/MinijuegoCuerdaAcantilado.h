#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Fases de la ronda: cuenta regresiva, tira y afloja y pantalla final.
enum FaseCuerdaAcantilado
{
    FASE_CUERDA_PREPARACION = 0,
    FASE_CUERDA_JUGANDO,
    FASE_CUERDA_TERMINADO
};


// Resultado visible del ultimo intento de tiron de un jugador.
enum FeedbackCuerdaAcantilado
{
    FEEDBACK_CUERDA_NINGUNO = 0,
    FEEDBACK_CUERDA_ACIERTO,
    FEEDBACK_CUERDA_RACHA,
    FEEDBACK_CUERDA_TROPIEZO
};


// Estado individual de ritmo: cada jugador tiene su propio pulso, asi el
// teclado compartido no obliga a pulsar todos en el mismo instante.
struct EstadoJugadorCuerdaAcantilado
{
    int equipo = 0;           // 0 = solitario, 1 = trio
    int ordenEnEquipo = 0;    // posicion en la fila sobre el acantilado

    float fasePulso = 0.0f;       // fase del indicador (crece sin limite)
    float tiempoTropiezo = 0.0f;  // bloqueo tras un fallo
    float tiempoRecarga = 0.0f;   // minimo entre tirones (anti machacar)
    float retroceso = 0.0f;       // 0..1, inclinacion del cuerpo al tirar
    float tiempoFeedback = 0.0f;
    FeedbackCuerdaAcantilado feedback = FEEDBACK_CUERDA_NINGUNO;

    int racha = 0;
    int aciertos = 0;
    int fallos = 0;

    // IA de bots: posicion del pulso en la que pulsaran.
    float objetivoBot = 0.5f;
    float pulsoAnterior = 0.0f;
};


static const int MAX_PARTICULAS_VISUALES_CUERDA = 260;


// Acantilados (solo visual): espuma, polvo, chispas y confeti.
struct ParticulaVisualCuerda
{
    bool activa = false;
    bool brillo = false;            // se dibuja en la pasada aditiva
    bool papel = false;             // confeti (cuadradito que gira)
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    float giro = 0.0f;
};


struct MinijuegoCuerdaAcantilado
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr
    EstadoJugadorCuerdaAcantilado estadosJugadores[MAX_PARTICIPANTES];

    Camera3D camara{};
    FaseCuerdaAcantilado fase = FASE_CUERDA_PREPARACION;

    int indiceSolo = -1;
    int cantidadRivales = 0;

    // -1 = linea de victoria del solitario, +1 = linea del trio.
    float marcador = 0.0f;
    float marcadorObjetivo = 0.0f;

    // Handicap calculado segun la cantidad de rivales del solitario.
    float fuerzaSolo = 1.0f;
    float factorVentanaSolo = 1.0f;

    bool empate = false;
    bool ganaSolo = false;

    float tiempoPreparacion = 0.0f;
    float tiempoJuego = 0.0f;
    float tiempoFin = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: nada de esto toca la logica ni consume el
    // GetRandomValue de los bots.
    unsigned int semillaVisual = 2246822519u;
    int aciertosVistos[MAX_PARTICIPANTES];
    int fallosVistos[MAX_PARTICIPANTES];
    float destelloTiron[MAX_PARTICIPANTES];
    float anuncioRacha[MAX_PARTICIPANTES];
    bool caidaVista[MAX_PARTICIPANTES];
    bool chapuzonVisto[MAX_PARTICIPANTES];
    float esperaOla = 0.0f;
    float esperaConfeti = 0.0f;
    bool finVisto = false;
    ParticulaVisualCuerda particulasVisuales[MAX_PARTICULAS_VISUALES_CUERDA];

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
