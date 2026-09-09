#include "Systems/Audio.h"

#include "Core/RecursosJuego.h"


struct RutasSonidoJuego
{
    const char* wav = nullptr;
    const char* mp3 = nullptr;
};


static const RutasSonidoJuego RUTAS_SONIDOS[CANTIDAD_SONIDOS_JUEGO] =
{
    { RUTA_SFX_UI_MOVER_WAV, RUTA_SFX_UI_MOVER_MP3 },
    { RUTA_SFX_UI_CONFIRMAR_WAV, RUTA_SFX_UI_CONFIRMAR_MP3 },
    { RUTA_SFX_CUENTA_REGRESIVA_WAV, RUTA_SFX_CUENTA_REGRESIVA_MP3 },
    { RUTA_SFX_INICIO_MINIJUEGO_WAV, RUTA_SFX_INICIO_MINIJUEGO_MP3 },
    { RUTA_SFX_RECOGER_NUCLEO_WAV, RUTA_SFX_RECOGER_NUCLEO_MP3 },
    { RUTA_SFX_RECOGER_NUCLEO_ESPECIAL_WAV, RUTA_SFX_RECOGER_NUCLEO_ESPECIAL_MP3 },
    { RUTA_SFX_ALERTA_TIEMPO_WAV, RUTA_SFX_ALERTA_TIEMPO_MP3 },
    { RUTA_SFX_RESULTADO_WAV, RUTA_SFX_RESULTADO_MP3 }
};


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


static void CargarSonidoDesdeArchivo(
    AudioJuego& audio,
    TipoSonidoJuego tipo,
    const char* rutaWav,
    const char* rutaMp3
)
{
    if (!audio.dispositivoInicializado)
    {
        return;
    }

    const char* rutaElegida = nullptr;

    // Para efectos cortos preferimos WAV. Si no existe,
    // permitimos MP3 con el mismo nombre como alternativa.
    if (FileExists(rutaWav))
    {
        rutaElegida = rutaWav;
    }
    else if (FileExists(rutaMp3))
    {
        rutaElegida = rutaMp3;
    }
    else
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro SFX. Agrega %s o %s",
            rutaWav,
            rutaMp3
        );

        return;
    }

    Sound sonido =
        LoadSound(rutaElegida);

    if (!IsSoundValid(sonido))
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar SFX: %s",
            rutaElegida
        );

        return;
    }

    audio.sonidos[tipo] = sonido;
    audio.sonidosCargados[tipo] = true;

    SetSoundVolume(
        audio.sonidos[tipo],
        audio.volumenSonidos
    );

    TraceLog(
        LOG_INFO,
        "SFX cargado: %s",
        rutaElegida
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

    for (
        int i = 0;
        i < CANTIDAD_SONIDOS_JUEGO;
        i++
    )
    {
        sonidosCargados[i] = false;

        CargarSonidoDesdeArchivo(
            *this,
            (TipoSonidoJuego)i,
            RUTAS_SONIDOS[i].wav,
            RUTAS_SONIDOS[i].mp3
        );
    }
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

    // Si un llamador antiguo pasa una ruta obsoleta, usamos la ruta
    // compartida actual. Esto evita que otra reorganizacion de Assets
    // vuelva a romper la musica del menu.
    const char* rutaElegida = ruta;

    if (rutaElegida == nullptr || !FileExists(rutaElegida))
    {
        rutaElegida = RUTA_MUSICA_MENU;
    }

    if (!FileExists(rutaElegida))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro musica del menu: %s",
            rutaElegida
        );

        return;
    }

    musicaMenu = LoadMusicStream(rutaElegida);

    if (!IsMusicValid(musicaMenu))
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar musica del menu: %s",
            rutaElegida
        );

        return;
    }

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
