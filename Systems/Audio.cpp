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
    }

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_UI_MOVER,
        "Assets/Audio/SFX/ui_mover.wav",
        "Assets/Audio/SFX/ui_mover.mp3"
    );

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_UI_CONFIRMAR,
        "Assets/Audio/SFX/ui_confirmar.wav",
        "Assets/Audio/SFX/ui_confirmar.mp3"
    );

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_CUENTA_REGRESIVA,
        "Assets/Audio/SFX/cuenta_regresiva.wav",
        "Assets/Audio/SFX/cuenta_regresiva.mp3"
    );

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_INICIO_MINIJUEGO,
        "Assets/Audio/SFX/inicio_minijuego.wav",
        "Assets/Audio/SFX/inicio_minijuego.mp3"
    );

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_RECOGER_NUCLEO,
        "Assets/Audio/SFX/recoger_nucleo.wav",
        "Assets/Audio/SFX/recoger_nucleo.mp3"
    );

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_RECOGER_NUCLEO_ESPECIAL,
        "Assets/Audio/SFX/recoger_nucleo_especial.wav",
        "Assets/Audio/SFX/recoger_nucleo_especial.mp3"
    );

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_ALERTA_TIEMPO,
        "Assets/Audio/SFX/alerta_tiempo.wav",
        "Assets/Audio/SFX/alerta_tiempo.mp3"
    );

    CargarSonidoDesdeArchivo(
        *this,
        SONIDO_RESULTADO,
        "Assets/Audio/SFX/resultado.wav",
        "Assets/Audio/SFX/resultado.mp3"
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

    if (!IsMusicValid(musicaMenu))
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar musica del menu: %s",
            ruta
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
