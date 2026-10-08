#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Lado del que salen los taladros. El orden importa: el opuesto de un lado
// es el mismo valor con el bit 0 invertido.
enum DireccionPinchos
{
    PINCHOS_DESDE_ARRIBA = 0,
    PINCHOS_DESDE_ABAJO,
    PINCHOS_DESDE_IZQUIERDA,
    PINCHOS_DESDE_DERECHA
};


enum FaseRefugioPinchos
{
    FASE_PINCHOS_PREPARACION = 0,
    FASE_PINCHOS_ESPERANDO,
    FASE_PINCHOS_AVISO,
    FASE_PINCHOS_ATAQUE,
    FASE_PINCHOS_TERMINADO
};


// Patrones de ataque. Todos cubren por completo el ancho de la arena (cada
// carril es un taladro): sobrevivir exige ponerse detras de una cobertura.
enum PatronPinchos
{
    PATRON_SALVA = 0,   // todos los carriles de un lado a la vez
    PATRON_OLA,         // los carriles disparan en cadena de un extremo al otro
    PATRON_ALTERNO,     // carriles pares y, un instante despues, los impares
    PATRON_RAFAGA,      // tres salvas seguidas por el mismo lado
    PATRON_TENAZA,      // dos lados opuestos a la vez
    PATRON_CRUCE,       // dos lados perpendiculares a la vez
    PATRON_VAIVEN,      // un lado y despues el opuesto: hay que cambiar de refugio
    PATRON_TOTAL
};


// Arena: rejilla conceptual de 8 x 7 carriles de 1.25 unidades (10 x 8.75).
// Cada carril es un taladro; las coberturas se alinean con la rejilla.
inline constexpr int COLUMNAS_PINCHOS = 8;
inline constexpr int FILAS_PINCHOS = 7;
inline constexpr int MAX_CARRILES_PINCHOS = 8;
inline constexpr int MAX_BLOQUES_PINCHOS = 10;
inline constexpr int MAX_SALVAS_PINCHOS = 4;

// Rejilla de navegacion de los bots (la logica de peligro no depende de
// ella: solo sirve para que los bots encuentren zonas seguras).
inline constexpr int CELDAS_X_PINCHOS = 32;
inline constexpr int CELDAS_Z_PINCHOS = 28;
inline constexpr int CELDAS_TOTALES_PINCHOS = CELDAS_X_PINCHOS * CELDAS_Z_PINCHOS;
inline constexpr int MAX_PARTICULAS_PINCHOS = 260;


struct EstadoJugadorRefugioPinchos
{
    bool eliminado = false;
    int posicionFinal = 0;
    float tiempoDesdeEliminacion = 0.0f;
};


// Una salva = un conjunto de carriles de un lado que disparan juntos (o en
// cadena si `paso` > 0). Los tiempos se miden desde el inicio del patron.
struct SalvaPinchos
{
    DireccionPinchos lado = PINCHOS_DESDE_ARRIBA;
    float inicioAviso = 0.0f;
    float disparo = 0.0f;
    float paso = 0.0f;
    bool invertido = false;
    unsigned int mascara = 0;
};


struct PatronActivoPinchos
{
    PatronPinchos tipo = PATRON_SALVA;
    int cantidad = 0;
    SalvaPinchos salvas[MAX_SALVAS_PINCHOS];
    float duracion = 0.0f;
};


// Cerebro de un bot del equipo: reaccion con retardo humano, errores y
// reubicacion entre ataques. Solo usa informacion visible para un humano.
struct CerebroBotRefugioPinchos
{
    float tiempoReaccion = 0.0f;
    int etapa = -1;
    bool confundido = false;
    PatronActivoPinchos percibido{};
    float proximaReubicacion = 0.0f;
    bool tieneDestino = false;
    bool destinoPatron = false;
    int celdaDestino = -1;
    float ultimoX = 0.0f;
    float ultimoZ = 0.0f;
    float tiempoAtasco = 0.0f;
    float campoDestino[CELDAS_TOTALES_PINCHOS]{};
};


struct ParticulaRefugioPinchos
{
    bool activa = false;
    bool chispa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.1f;
    Color color = { 120, 105, 90, 255 };
};


struct MinijuegoRefugioPinchos
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;

    EstadoJugadorRefugioPinchos estadosJugadores[MAX_PARTICIPANTES];
    CerebroBotRefugioPinchos cerebros[MAX_PARTICIPANTES];

    // Suelo + coberturas (rocas alineadas con la rejilla de carriles). Los
    // muros perimetrales son visuales y el limite de movimiento se resuelve
    // de forma independiente.
    BloquePrueba bloques[MAX_BLOQUES_PINCHOS];
    int cantidadBloques = 0;

    // Hasta donde llega cada taladro (desde la cara del muro) antes de
    // chocar con la primera cobertura de su carril. Es EXACTAMENTE la
    // franja que se dibuja y la que mata.
    float alcance[4][MAX_CARRILES_PINCHOS]{};
    bool chocaCobertura[4][MAX_CARRILES_PINCHOS]{};

    Camera3D camara{};

    FaseRefugioPinchos fase = FASE_PINCHOS_PREPARACION;
    DireccionPinchos ultimaDireccion = PINCHOS_DESDE_ARRIBA;
    PatronPinchos ultimoPatron = PATRON_TOTAL;

    int indiceSolo = -1;

    PatronActivoPinchos patron{};
    bool patronActivo = false;
    float tiempoPatron = 0.0f;
    float avisoActual = 1.15f;
    float tiempoExtension = 0.34f;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 44.0f;
    float cooldownAtaque = 0.0f;
    float tiempoEleccion = 0.0f;
    float decisionBotSolo = 0.0f;
    float tiempoPulso = 0.0f;
    int ataquesRealizados = 0;
    bool tiempoAgotado = false;

    bool salvaLanzada[MAX_SALVAS_PINCHOS][MAX_CARRILES_PINCHOS]{};
    bool salvaImpactada[MAX_SALVAS_PINCHOS][MAX_CARRILES_PINCHOS]{};
    bool salvaSonada[MAX_SALVAS_PINCHOS]{};
    bool avisoSonado[MAX_SALVAS_PINCHOS]{};
    bool ticSonado[MAX_SALVAS_PINCHOS]{};
    float temporizadorEmision = 0.0f;
    float temporizadorAmbiente = 0.0f;

    // Rejilla de navegacion: celdas bloqueadas por coberturas o fuera de la
    // zona caminable, y celdas a salvo de una salva completa de cada lado.
    bool bloqueada[CELDAS_TOTALES_PINCHOS]{};
    bool coberturaLado[4][CELDAS_TOTALES_PINCHOS]{};
    float distRefugio[4][CELDAS_TOTALES_PINCHOS]{};

    // Celdas a poca distancia de un refugio sea cual sea el lado del ataque.
    bool celdaComoda[CELDAS_TOTALES_PINCHOS]{};

    ParticulaRefugioPinchos particulasLocales[MAX_PARTICULAS_PINCHOS];

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
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;

    // Publica el estado visual de los taladros (direccion, progreso y aviso).
    void ConfigurarTaladrosVisuales() const;
};
