#include "Systems/Audio.h"

#include <cmath>
#include <vector>


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


static Sound CrearTonoProcedural(
    float frecuenciaInicial,
    float frecuenciaFinal,
    float duracion,
    float amplitud
)
{
    const int FRECUENCIA_MUESTREO = 44100;

    int cantidadFrames =
        (int)(duracion * FRECUENCIA_MUESTREO);

    if (cantidadFrames < 1)
    {
        cantidadFrames = 1;
    }

    std::vector<short> muestras(
        cantidadFrames
    );

    double fase = 0.0;

    for (int i = 0; i < cantidadFrames; i++)
    {
        float progreso =
            cantidadFrames > 1
            ? (float)i / (float)(cantidadFrames - 1)
            : 1.0f;

        float frecuencia =
            frecuenciaInicial +
            (frecuenciaFinal - frecuenciaInicial) * progreso;

        fase +=
            6.28318530718 *
            (double)frecuencia /
            (double)FRECUENCIA_MUESTREO;

        float ataque =
            progreso < 0.08f
            ? progreso / 0.08f
            : 1.0f;

        float salida =
            1.0f - progreso;

        float envolvente =
            ataque * salida * salida;

        double onda =
            std::sin(fase) * 0.82 +
            std::sin(fase * 2.0) * 0.18;

        float muestra =
            (float)onda *
            amplitud *
            envolvente;

        muestra = LimitarFloat(
            muestra,
            -1.0f,
            1.0f
        );

        muestras[i] =
            (short)(muestra * 32767.0f);
    }

    Wave onda{};
    onda.frameCount = (unsigned int)cantidadFrames;
    onda.sampleRate = FRECUENCIA_MUESTREO;
    onda.sampleSize = 16;
    onda.channels = 1;
    onda.data = muestras.data();

    return LoadSoundFromWave(onda);
}


static void CargarSonidoProcedural(
    AudioJuego& audio,
    TipoSonidoJuego tipo,
    float frecuenciaInicial,
    float frecuenciaFinal,
    float duracion,
    float amplitud
)
{
    Sound sonido =
        CrearTonoProcedural(
            frecuenciaInicial,
            frecuenciaFinal,
            duracion,
            amplitud
        );

    if (!IsSoundValid(sonido))
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo crear sonido procedural %d",
            (int)tipo
        );

        return;
    }

    audio.sonidos[tipo] = sonido;
    audio.sonidosCargados[tipo] = true;

    SetSoundVolume(
        audio.sonidos[tipo],
        audio.volumenSonidos
    );
}


void AudioJuego::Inicializar()
{
    InitAudioDevice();

    dispositivoInicializado =
        IsAudioDeviceReady();

    if (!dispositivoInicializado)
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo inicializar el dispositivo de audio"
        );

        return;
    }

    CargarSonidoProcedural(
        *this,
        SONIDO_UI_MOVER,
        520.0f,
        640.0f,
        0.07f,
        0.62f
    );

    CargarSonidoProcedural(
        *this,
        SONIDO_UI_CONFIRMAR,
        650.0f,
        980.0f,
        0.12f,
        0.72f
    );

    CargarSonidoProcedural(
        *this,
        SONIDO_CUENTA_REGRESIVA,
        430.0f,
        350.0f,
        0.11f,
        0.72f
    );

    CargarSonidoProcedural(
        *this,
        SONIDO_INICIO_MINIJUEGO,
        520.0f,
        1180.0f,
        0.25f,
        0.78f
    );

    CargarSonidoProcedural(
        *this,
        SONIDO_RECOGER_NUCLEO,
        820.0f,
        1280.0f,
        0.11f,
        0.66f
    );

    CargarSonidoProcedural(
        *this,
        SONIDO_RECOGER_NUCLEO_ESPECIAL,
        920.0f,
        1760.0f,
        0.19f,
        0.78f
    );

    CargarSonidoProcedural(
        *this,
        SONIDO_ALERTA_TIEMPO,
        320.0f,
        260.0f,
        0.12f,
        0.72f
    );

    CargarSonidoProcedural(
        *this,
        SONIDO_RESULTADO,
        580.0f,
        1320.0f,
        0.38f,
        0.80f
    );
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


void AudioJuego::ReproducirSonido(
    TipoSonidoJuego tipo
)
{
    if (
        !dispositivoInicializado ||
        tipo < 0 ||
        tipo >= CANTIDAD_SONIDOS_JUEGO ||
        !sonidosCargados[tipo]
    )
    {
        return;
    }

    PlaySound(
        sonidos[tipo]
    );
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

    for (
        int i = 0;
        i < CANTIDAD_SONIDOS_JUEGO;
        i++
    )
    {
        if (!sonidosCargados[i])
        {
            continue;
        }

        SetSoundVolume(
            sonidos[i],
            volumenSonidos
        );
    }
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

    for (
        int i = 0;
        i < CANTIDAD_SONIDOS_JUEGO;
        i++
    )
    {
        if (!sonidosCargados[i])
        {
            continue;
        }

        UnloadSound(
            sonidos[i]
        );

        sonidosCargados[i] = false;
    }

    if (dispositivoInicializado)
    {
        CloseAudioDevice();

        dispositivoInicializado = false;
    }
}
