#pragma once

#include "raylib.h"


enum TipoSonidoJuego
{
    SONIDO_UI_MOVER = 0,
    SONIDO_UI_CONFIRMAR,
    SONIDO_CUENTA_REGRESIVA,
    SONIDO_INICIO_MINIJUEGO,
    SONIDO_RECOGER_NUCLEO,
    SONIDO_RECOGER_NUCLEO_ESPECIAL,
    SONIDO_ALERTA_TIEMPO,
    SONIDO_RESULTADO,
    CANTIDAD_SONIDOS_JUEGO
};


struct AudioJuego
{
    Music musicaMenu{};

    Sound sonidos[CANTIDAD_SONIDOS_JUEGO]{};
    bool sonidosCargados[CANTIDAD_SONIDOS_JUEGO]{};

    bool dispositivoInicializado = false;
    bool musicaMenuCargada = false;
    bool musicaMenuSonando = false;

    float volumenMusica = 0.35f;
    float volumenSonidos = 0.60f;

    void Inicializar();

    void CargarMusicaMenu(
        const char* ruta
    );

    void ReproducirMusicaMenu();

    void ReproducirSonido(
        TipoSonidoJuego tipo
    );

    void Actualizar();

    void AplicarVolumenMusica(
        float volumen
    );

    void AplicarVolumenSonidos(
        float volumen
    );

    void Descargar();
};
