#include "Systems/Audio.h"

static float LimitarFloat(
    float valor,
    float minimo,
    float maximo
)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

void AudioJuego::Inicializar()
{
    InitAudioDevice();

    dispositivoInicializado = true;
}

void AudioJuego::CargarMusicaMenu(
    const char* ruta
)
{
    if (!dispositivoInicializado)
    {
        return;
    }

    if (musicaMenuCargada)
    {
        return;
    }

    if (!FileExists(ruta))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro musica del menu: %s",
            ruta
        );

        return;
    }

    musicaMenu = LoadMusicStream(ruta);

    musicaMenu.looping = true;

    musicaMenuCargada = true;

    SetMusicVolume(
        musicaMenu,
        volumenMusica
    );
}

void AudioJuego::ReproducirMusicaMenu()
{
    if (!musicaMenuCargada)
    {
        return;
    }

    if (musicaMenuSonando)
    {
        return;
    }

    PlayMusicStream(musicaMenu);

    musicaMenuSonando = true;
}

void AudioJuego::Actualizar()
{
    if (
        musicaMenuCargada &&
        musicaMenuSonando
    )
    {
        UpdateMusicStream(musicaMenu);
    }
}

void AudioJuego::AplicarVolumenMusica(
    float volumen
)
{
    volumenMusica = LimitarFloat(
        volumen,
        0.0f,
        1.0f
    );

    if (musicaMenuCargada)
    {
        SetMusicVolume(
            musicaMenu,
            volumenMusica
        );
    }
}

void AudioJuego::AplicarVolumenSonidos(
    float volumen
)
{
    volumenSonidos = LimitarFloat(
        volumen,
        0.0f,
        1.0f
    );

    /*
        Todavia no tenes efectos
        cargados, pero ya queda
        preparado para el futuro.
    */
}

void AudioJuego::Descargar()
{
    if (musicaMenuCargada)
    {
        StopMusicStream(musicaMenu);
        UnloadMusicStream(musicaMenu);

        musicaMenuCargada = false;
        musicaMenuSonando = false;
    }

    if (dispositivoInicializado)
    {
        CloseAudioDevice();

        dispositivoInicializado = false;
    }
}