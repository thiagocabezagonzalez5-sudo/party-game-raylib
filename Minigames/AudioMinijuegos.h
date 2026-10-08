#pragma once

#include "Systems/Audio.h"

#include <cmath>


//==================================================
// AUDIO COMUN DE MINIJUEGOS
//==================================================
//
// Ayudas pequenas para que todos los minijuegos suenen igual en los
// momentos compartidos (cuenta regresiva, inicio, alerta de tiempo,
// resultado). El puntero de audio puede ser nullptr: en ese caso no
// suena nada y el minijuego funciona igual.
//==================================================


inline void ReproducirSonidoMinijuego(
    AudioJuego* audio,
    TipoSonidoJuego tipo
)
{
    if (audio != nullptr)
    {
        audio->ReproducirSonido(tipo);
    }
}


// Llamar una vez por frame durante la preparacion con el tiempo restante
// antes y despues de descontar deltaTime. Suena un tick en 3, 2, 1 y el
// sonido de inicio al llegar a cero.
inline void ActualizarAudioCuentaRegresiva(
    AudioJuego* audio,
    float tiempoAntes,
    float tiempoDespues
)
{
    if (tiempoDespues <= 0.0f)
    {
        if (tiempoAntes > 0.0f)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_INICIO_MINIJUEGO);
        }

        return;
    }

    // El primer frame (antes == duracion completa) tambien marca su numero.
    if (std::ceil(tiempoAntes) != std::ceil(tiempoDespues) ||
        tiempoAntes == std::ceil(tiempoAntes))
    {
        if (std::ceil(tiempoDespues) <= 3.0f)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_CUENTA_REGRESIVA);
        }
    }
}


// Llamar una vez por frame mientras se juega. Suena una vez por segundo
// cuando quedan "umbral" segundos o menos.
inline void ActualizarAudioAlertaTiempo(
    AudioJuego* audio,
    float tiempoAntes,
    float tiempoDespues,
    float umbral = 5.0f
)
{
    if (tiempoDespues <= 0.0f || tiempoDespues > umbral)
    {
        return;
    }

    if (std::ceil(tiempoAntes) != std::ceil(tiempoDespues))
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ALERTA_TIEMPO);
    }
}
