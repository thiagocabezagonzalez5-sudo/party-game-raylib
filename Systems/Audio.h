#pragma once

#include "raylib.h"

struct AudioJuego
{
    Music musicaMenu = { 0 };

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

    void Actualizar();

    void AplicarVolumenMusica(
        float volumen
    );

    void AplicarVolumenSonidos(
        float volumen
    );

    void Descargar();
};