#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// AUTOS DE GLOBO
//==================================================
//
// Todos contra todos sobre una plaza elevada de una ciudad futurista. Cada
// jugador conduce un auto flotante con 3 globos de energia. Embestir con el
// frente el lateral o la trasera de un rival le revienta un globo (queda
// inmune 1 s); los choques frontales solo se repelen. La accion es un turbo
// corto. Placas de carga que aparecen con aviso regeneran un globo. Sin
// globos se queda eliminado; gana el ultimo en pie o, a los 60 s, quien
// conserve mas globos.
//==================================================

inline constexpr int MAX_PARTICULAS_AUTOS_GLOBO = MAX_PARTICULAS_TIERRA;
inline constexpr int MAX_PLACAS_AUTOS_GLOBO = 2;
inline constexpr int GLOBOS_INICIALES_AUTOS_GLOBO = 3;


enum FaseAutosGlobo
{
    FASE_AUTOS_GLOBO_PREPARACION = 0,
    FASE_AUTOS_GLOBO_JUGANDO,
    FASE_AUTOS_GLOBO_TERMINADO
};


struct AutoGlobo
{
    bool vivo = false;
    float x = 0.0f;
    float z = 0.0f;
    float angulo = 0.0f;              // rumbo en radianes: frente = (cos, sin) sobre (x, z)
    float vx = 0.0f;
    float vz = 0.0f;
    int globos = 0;
    float inmunidad = 0.0f;
    float turboActivo = 0.0f;
    float turboRecarga = 0.0f;
    float enfriamientoChoque = 0.0f;
    float enfriamientoBarrera = 0.0f;
    float tiempoEliminacion = 0.0f;
};


struct PlacaAutosGlobo
{
    bool activa = false;
    bool enAviso = false;
    float x = 0.0f;
    float z = 0.0f;
    float tiempo = 0.0f;              // aviso restante o vida restante
};


struct EstadoBotAutosGlobo
{
    int objetivo = -1;
    float reevaluar = 0.0f;
    float errorAngulo = 0.0f;
    bool querTurbo = false;
    float tiempoHuida = 0.0f;
    float ladoRodeo = 1.0f;
};


struct MinijuegoAutosGlobo
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    ParticulaTierra particulas[MAX_PARTICULAS_AUTOS_GLOBO];
    AutoGlobo autos[MAX_PARTICIPANTES];
    PlacaAutosGlobo placas[MAX_PLACAS_AUTOS_GLOBO];
    EstadoBotAutosGlobo bots[MAX_PARTICIPANTES];

    Camera3D camara{};
    FaseAutosGlobo fase = FASE_AUTOS_GLOBO_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;
    float proximaPlaca = 0.0f;

    // Ultimo aviso de eliminacion para mostrar en pantalla.
    char textoEvento[48]{};
    float tiempoEvento = 0.0f;

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
