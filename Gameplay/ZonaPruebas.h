#pragma once

#include "raylib.h"
#include "Core/ConfiguracionJuego.h"


const int MAX_BLOQUES_PRUEBA = 8;
const int MAX_JUGADORES_PRUEBA = 4;
const int MAX_PARTICULAS_TIERRA = 160;


enum ModoZonaPruebas
{
    PRUEBA_ZONA_PRINCIPAL = 1,
    PRUEBA_COLOR_SEGURO = 2,
    PRUEBA_PELOTAS_EMPUJON = 3
};


enum FaseMinijuegoColor
{
    FASE_ELEGIR_PLATAFORMA = 0,
    FASE_CAIDA_PLATAFORMAS
};


struct BloquePrueba
{
    Vector3 posicion{};
    Vector3 posicionInicial{};

    Vector3 tamano =
    {
        1.0f,
        1.0f,
        1.0f
    };

    Color color = GRAY;

    bool activaColision = true;
    bool cayendo = false;

    float velocidadCaida = 0.0f;
};


struct ParticulaTierra
{
    bool activa = false;

    Vector3 posicion{};
    Vector3 velocidad{};

    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.08f;

    Color color =
    {
        125,
        84,
        48,
        255
    };
};


struct JugadorPrueba
{
    int numero = 1;
    bool activo = false;

    bool usaGamepad = false;
    int indiceGamepad = -1;

    Color color = RED;

    Vector3 posicion{};
    Vector3 posicionSpawn{};
    Vector3 velocidad{};
    Vector3 empuje{};

    Vector3 tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };

    float velocidadMovimiento = 5.0f;
    float fuerzaSalto = 7.2f;
    float gravedad = 18.0f;

    bool enSuelo = false;
    bool cayendo = false;

    float tiempoRespawn = 0.0f;
    float duracionRespawn = 1.2f;

    float cooldownChoque = 0.0f;
};


struct ZonaPruebas
{
    ModoZonaPruebas modoActual =
        PRUEBA_ZONA_PRINCIPAL;

    JugadorPrueba jugadores[MAX_JUGADORES_PRUEBA];
    int cantidadJugadoresActivos = 0;

    ModoTeclado modoTecladoActual = TECLADO_DIVIDIDO;

    BloquePrueba bloques[MAX_BLOQUES_PRUEBA];
    int cantidadBloques = 0;

    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];

    Camera3D camara{};

    bool mostrarDebug = false;
    bool volverAlMenu = false;

    FaseMinijuegoColor faseMinijuego =
        FASE_ELEGIR_PLATAFORMA;

    int indicePlataformaSegura = 0;
    int numeroRonda = 1;

    float tiempoFase = 5.0f;
    float duracionElegirPlataforma = 5.0f;
    float duracionCaidaPlataformas = 2.0f;

    void Inicializar(
        ModoTeclado modoTeclado
    );

    void CambiarModo(
        ModoZonaPruebas nuevoModo
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void ReiniciarJugador(
        int indice
    );

    void ReiniciarJugadores();
};
