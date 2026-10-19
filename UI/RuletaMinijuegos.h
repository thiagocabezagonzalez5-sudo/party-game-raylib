#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Systems/Audio.h"
#include "UI/SeleccionMinijuegos.h"
#include "raylib.h"


// Carrusel que presenta el minijuego de la ronda del tablero.
// El resultado se recibe ya decidido en Iniciar(); la animacion
// solo lo muestra y siempre se detiene exactamente en ese minijuego.
// Al detenerse cae la previa (PreviaMinijuego, la misma del selector
// libre) y Termino() se activa recien cuando todos estan listos.
enum EstadoRuletaMinijuegos
{
    RULETA_INACTIVA = 0,
    RULETA_INICIANDO,
    RULETA_GIRANDO,
    RULETA_FRENANDO,
    RULETA_SELECCIONADO,
    RULETA_PREVIA,
    RULETA_TERMINADA
};


struct RuletaMinijuegos
{
    // Minijuegos disponibles para tablero, en orden de catalogo.
    IdMinijuego opciones[CANTIDAD_MINIJUEGOS]{};
    int cantidadOpciones = 0;

    IdMinijuego minijuegoElegido = MINIJUEGO_COLOR_SEGURO;
    int indiceElegido = 0;

    EstadoRuletaMinijuegos estado = RULETA_INACTIVA;

    float tiempoEstado = 0.0f;
    float tiempoTotal = 0.0f;

    // Posicion continua del carrusel medida en miniaturas.
    // El valor entero centrado indica la opcion bajo el marcador.
    float posicion = 0.0f;
    float posicionInicial = 0.0f;

    // El frenado empieza en posicionInicioFrenado y termina
    // exactamente en posicionFinal, que apunta a indiceElegido.
    float posicionInicioFrenado = 0.0f;
    float posicionFinal = 0.0f;

    // Confirmar solo acelera el tiempo de la animacion;
    // nunca cambia posicionFinal.
    bool frenadoAcelerado = false;

    int ultimaCasillaCentrada = 0;
    float tiempoDesdeUltimoTick = 0.0f;

    // Cartel del minijuego elegido con la bandera de cada jugador.
    PreviaMinijuego previa;

    // Con la previa quieta, lo fijo de la pantalla se dibuja una vez en
    // esta textura (ver DibujarEnCacheMinijuegos). Se libera al terminar
    // la previa y en Descargar().
    static const int CANTIDAD_CLAVE_CACHE_RULETA = 6;
    mutable RenderTexture2D cachePrevia = {};
    mutable bool cacheValida = false;
    mutable int claveCache[CANTIDAD_CLAVE_CACHE_RULETA] = {};

    AudioJuego* audio = nullptr;

    // opcionesFijas (opcional): lista propia de minijuegos en lugar de
    // todos los de tablero (p. ej. los 3 de duelo). tituloRuleta va arriba.
    void Iniciar(
        IdMinijuego elegido,
        AudioJuego* audioJuego,
        const IdMinijuego* opcionesFijas = nullptr,
        int cantidadFijas = 0,
        const char* tituloRuleta = "MINIJUEGO DE LA RONDA"
    );

    const char* titulo = "MINIJUEGO DE LA RONDA";

    // confirmar acelera el frenado; participantes alimentan la previa.
    void Actualizar(
        float deltaTime,
        bool confirmar,
        const Participante participantes[],
        int cantidad
    );

    void Dibujar(
        const Participante participantes[],
        int cantidad
    ) const;

    // Velo, tendedero, carteles y ayuda de la ruleta (sin la previa).
    void DibujarTendedero() const;

    // Libera la textura de la previa. Seguro llamar varias veces.
    void Descargar();

    bool Termino() const;

    IdMinijuego ObtenerMinijuegoElegido() const;
};
