#include "Systems/Audio.h"

#include "Core/RecursosJuego.h"

#include <cstdio>


// Rutas base sin extension, en el mismo orden que TipoSonidoJuego.
static const char* const RUTAS_SONIDOS[] =
{
    RUTA_SFX_UI_MOVER,
    RUTA_SFX_UI_CONFIRMAR,
    RUTA_SFX_CUENTA_REGRESIVA,
    RUTA_SFX_INICIO_MINIJUEGO,
    RUTA_SFX_RECOGER_NUCLEO,
    RUTA_SFX_RECOGER_NUCLEO_ESPECIAL,
    RUTA_SFX_ALERTA_TIEMPO,
    RUTA_SFX_RESULTADO,

    RUTA_SFX_UI_CANCELAR,

    RUTA_SFX_SALTO,
    RUTA_SFX_ATERRIZAJE,
    RUTA_SFX_GOLPE,
    RUTA_SFX_CAIDA,
    RUTA_SFX_GROUND_POUND,

    RUTA_SFX_DADO,
    RUTA_SFX_PASO_TABLERO,
    RUTA_SFX_MONEDA,
    RUTA_SFX_COMPRA,
    RUTA_SFX_CASILLA_POSITIVA,
    RUTA_SFX_CASILLA_NEGATIVA,
    RUTA_SFX_EVENTO_TABLERO,
    RUTA_SFX_RULETA_TICK,

    RUTA_SFX_RECOGER_OBJETO,
    RUTA_SFX_EXPLOSION,
    RUTA_SFX_BOTON,
    RUTA_SFX_DISPARO,
    RUTA_SFX_PLATAFORMA,
    RUTA_SFX_IMPACTO,
    RUTA_SFX_ACIERTO,
    RUTA_SFX_ERROR,
    RUTA_SFX_ELIMINADO
};

static_assert(
    sizeof(RUTAS_SONIDOS) / sizeof(RUTAS_SONIDOS[0]) ==
        CANTIDAD_SONIDOS_JUEGO,
    "Cada TipoSonidoJuego necesita su ruta en RUTAS_SONIDOS"
);


// Para efectos cortos preferimos WAV; OGG y MP3 son alternativas.
static const char* const EXTENSIONES_SONIDO[] =
{
    ".wav",
    ".ogg",
    ".mp3"
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
    const char* rutaBase
)
{
    if (!audio.dispositivoInicializado)
    {
        return;
    }

    char rutaElegida[256]{};
    bool encontrada = false;

    for (const char* extension : EXTENSIONES_SONIDO)
    {
        std::snprintf(
            rutaElegida,
            sizeof(rutaElegida),
            "%s%s",
            rutaBase,
            extension
        );

        if (FileExists(rutaElegida))
        {
            encontrada = true;
            break;
        }
    }

    if (!encontrada)
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro SFX. Agrega %s.wav, .ogg o .mp3",
            rutaBase
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
            RUTAS_SONIDOS[i]
        );
    }
}


// Rutas base (sin extension) de las categorias que no son el menu.
static const char* const RUTAS_MUSICA_CATEGORIA[CANTIDAD_CATEGORIAS_MUSICA] =
{
    nullptr,
    RUTA_MUSICA_TABLERO,
    RUTA_MUSICA_MINIJUEGO,
    RUTA_MUSICA_RESULTADO
};


static bool CargarPistaMusica(
    AudioJuego& audio,
    CategoriaMusica categoria,
    const char* ruta
)
{
    if (ruta == nullptr || !FileExists(ruta))
    {
        return false;
    }

    Music musica = LoadMusicStream(ruta);

    if (!IsMusicValid(musica))
    {
        TraceLog(LOG_WARNING, "No se pudo cargar musica: %s", ruta);
        return false;
    }

    musica.looping = true;
    SetMusicVolume(musica, audio.volumenMusica);

    audio.musicas[categoria] = musica;
    audio.musicasCargadas[categoria] = true;

    TraceLog(LOG_INFO, "Musica cargada: %s", ruta);
    return true;
}


void AudioJuego::CargarMusicaMenu(
    const char* ruta
)
{
    if (!dispositivoInicializado)
    {
        return;
    }

    if (musicasCargadas[MUSICA_MENU])
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

    if (!CargarPistaMusica(*this, MUSICA_MENU, rutaElegida))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro musica del menu: %s",
            rutaElegida
        );
    }

    // Categorias opcionales: se prueban .ogg y .mp3.
    for (int i = 0; i < CANTIDAD_CATEGORIAS_MUSICA; i++)
    {
        if (RUTAS_MUSICA_CATEGORIA[i] == nullptr || musicasCargadas[i])
        {
            continue;
        }

        const char* extensiones[] = { ".ogg", ".mp3" };
        bool cargada = false;

        for (const char* extension : extensiones)
        {
            char rutaPista[256]{};
            std::snprintf(
                rutaPista,
                sizeof(rutaPista),
                "%s%s",
                RUTAS_MUSICA_CATEGORIA[i],
                extension
            );

            if (CargarPistaMusica(*this, (CategoriaMusica)i, rutaPista))
            {
                cargada = true;
                break;
            }
        }

        if (!cargada)
        {
            TraceLog(
                LOG_INFO,
                "Musica opcional ausente: %s (.ogg/.mp3); se mantiene la pista actual",
                RUTAS_MUSICA_CATEGORIA[i]
            );
        }
    }
}


void AudioJuego::ReproducirMusicaMenu()
{
    if (!musicasCargadas[MUSICA_MENU] || musicaActual == MUSICA_MENU)
    {
        return;
    }

    if (musicaActual >= 0)
    {
        StopMusicStream(musicas[musicaActual]);
    }

    PlayMusicStream(musicas[MUSICA_MENU]);
    musicaActual = MUSICA_MENU;
}


void AudioJuego::SeleccionarMusica(
    CategoriaMusica categoria
)
{
    if (
        musicaActual < 0 ||
        categoria < 0 ||
        categoria >= CANTIDAD_CATEGORIAS_MUSICA ||
        categoria == musicaActual ||
        !musicasCargadas[categoria]
    )
    {
        return;
    }

    StopMusicStream(musicas[musicaActual]);
    PlayMusicStream(musicas[categoria]);
    musicaActual = categoria;
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
    if (musicaActual >= 0 && musicasCargadas[musicaActual])
    {
        UpdateMusicStream(musicas[musicaActual]);
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

    for (int i = 0; i < CANTIDAD_CATEGORIAS_MUSICA; i++)
    {
        if (musicasCargadas[i])
        {
            SetMusicVolume(
                musicas[i],
                volumenMusica
            );
        }
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
    for (int i = 0; i < CANTIDAD_CATEGORIAS_MUSICA; i++)
    {
        if (!musicasCargadas[i])
        {
            continue;
        }

        StopMusicStream(musicas[i]);
        UnloadMusicStream(musicas[i]);
        musicasCargadas[i] = false;
    }

    musicaActual = -1;

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
