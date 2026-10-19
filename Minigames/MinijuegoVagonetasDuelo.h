#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// VAGONETAS DE MINA (DUELO 1v1)
//==================================================
//
// Carrera de vagonetas entre dos jugadores por un circuito sinuoso de
// mina volcanica. Pantalla dividida: cada uno ve su propia pista, y las
// dos pistas son IDENTICAS (misma semilla: mismas rocas, huecos, monedas
// y plataformas de impulso), asi que gana la destreza.
//
//  - IZQUIERDA / DERECHA: cambiar de riel (3 rieles).
//  - SALTAR: brincar. Las vias rotas (huecos) y las vigas bajas se saltan;
//    las rocas altas solo se esquivan cambiando de riel.
//  - GOLPEAR: turbo corto. Gasta un tercio de la energia (se recarga sola
//    y con las monedas); chocar con el turbo activo duele el doble.
//  - Las placas de impulso dan velocidad gratis; las monedas, un empujon
//    pequeno. Un choque frena y suelta chispas.
//  - Una ayuda suave de "goma" empuja un poco al que va atras.
//
// Gana quien cruza la meta primero (tiempo exacto dentro del cuadro). Con
// tope de 45 s gana quien lleva mas distancia. Desempate determinista:
// menos choques, mas monedas y por ultimo el primer participante activo.
//==================================================

inline constexpr int CARRILES_VAGONETAS = 3;
inline constexpr int MAX_ELEMENTOS_VAGONETAS = 320;
inline constexpr int MAX_PARTICULAS_VAGONETAS = 96;
inline constexpr int MAX_AVISOS_VAGONETAS = 3;


enum FaseVagonetasDuelo
{
    FASE_VAGONETAS_PREPARACION = 0,
    FASE_VAGONETAS_CARRERA,
    FASE_VAGONETAS_TERMINADO
};


enum TipoElementoVagonetas
{
    ELEMENTO_VAGONETAS_ROCA = 0,    // alta: solo se esquiva
    ELEMENTO_VAGONETAS_HUECO,       // via rota: se salta o se esquiva
    ELEMENTO_VAGONETAS_VIGA,        // baja, a todo lo ancho: se salta
    ELEMENTO_VAGONETAS_MONEDA,
    ELEMENTO_VAGONETAS_MONEDA_ALTA, // en el aire: hay que saltar
    ELEMENTO_VAGONETAS_PLACA        // impulso gratis
};


enum AvisoVagonetas
{
    AVISO_VAGONETAS_NINGUNO = 0,
    AVISO_VAGONETAS_TURBO,
    AVISO_VAGONETAS_IMPULSO,
    AVISO_VAGONETAS_CHOQUE,
    AVISO_VAGONETAS_ADELANTE
};


struct ElementoPistaVagonetas
{
    TipoElementoVagonetas tipo = ELEMENTO_VAGONETAS_MONEDA;
    int carriles = 0;           // mascara de bits (hasta 3 rieles)
    float distancia = 0.0f;     // centro, en metros desde la salida
    float largo = 1.0f;         // para huecos: longitud de la via rota
};


struct AvisoFlotanteVagonetas
{
    AvisoVagonetas tipo = AVISO_VAGONETAS_NINGUNO;
    float tiempo = 0.0f;
};


struct ParticulaVagonetas
{
    bool activa = false;
    bool polvo = false;         // humo/polvo translucido en vez de chispa
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.08f;
    Color color = WHITE;
};


struct CarroVagonetas
{
    int participante = -1;      // indice en Participante[]
    bool controlIA = false;

    // Estado de carrera.
    float distancia = 0.0f;
    float velocidad = 0.0f;
    float lateral = 0.0f;       // metros respecto al riel central
    int carrilObjetivo = 1;
    float altura = 0.0f;
    float velocidadVertical = 0.0f;
    float energia = 1.0f;       // 0..1; el turbo cuesta un tercio
    float turbo = 0.0f;         // segundos de turbo restantes
    float recargaTurbo = 0.0f;
    float impulsoPlaca = 0.0f;
    float bonoMonedas = 0.0f;   // m/s extra, decae solo
    float aturdido = 0.0f;
    float inmune = 0.0f;
    float hundido = 0.0f;       // sacudida al caer en un hueco
    int monedas = 0;
    int choques = 0;
    bool llego = false;
    float tiempoLlegada = 0.0f;
    int cursor = 0;             // primer elemento aun por delante

    bool tomado[MAX_ELEMENTOS_VAGONETAS]{};

    // Entrada del cuadro anterior (flancos).
    bool previoIzquierda = false;
    bool previoDerecha = false;
    bool previoSaltar = false;
    bool previoGolpear = false;

    // Animacion.
    float giroRuedas = 0.0f;
    float inclinacion = 0.0f;
    float guinada = 0.0f;
    float sacudida = 0.0f;
    float destelloChoque = 0.0f;
    float tiempoChispas = 0.0f;
    float tiempoPolvo = 0.0f;
    float tiempoSalto = 0.0f;
    AvisoFlotanteVagonetas avisos[MAX_AVISOS_VAGONETAS]{};

    // IA (bots y humanos desconectados).
    float habilidad = 1.0f;
    float probabilidadError = 0.05f;
    float reservaTurbo = 0.0f;
    float anticipacion = 0.0f;
    float tiempoDecision = 0.0f;
    float tiempoTurbo = 0.0f;
    int elementoPlaneado = -1;
    int accionPlaneada = 0;     // 0 nada, 1 cambiar de riel, 2 saltar
    int carrilDestino = 1;
    float distanciaDisparo = 0.0f;
    int pulsacionesPendientes = 0;
    float esperaPulsacion = 0.0f;
    int direccionPulsacion = 0;
};


struct MinijuegoVagonetasDuelo
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    CarroVagonetas carros[2];
    int cantidadCarros = 0;

    ElementoPistaVagonetas elementos[MAX_ELEMENTOS_VAGONETAS];
    int cantidadElementos = 0;
    unsigned int semillaPista = 1;
    unsigned int semillaEfectos = 1;
    unsigned int semillaIA = 1;
    int lider = -1;                 // carro que va adelante (-1 si es parejo)

    ParticulaVagonetas particulas[2][MAX_PARTICULAS_VAGONETAS];

    int ganador = -1;               // 0 o 1 (posicion en carros[])
    bool partidaValida = false;

    FaseVagonetasDuelo fase = FASE_VAGONETAS_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoCarrera = 0.0f;
    float tiempoFinal = 0.0f;       // desde que alguien cruza la meta
    float tiempoAnimacion = 0.0f;

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
