#include "Gameplay/GestorMinijuegos.h"


void GestorMinijuegos::InicializarMinijuego(
    IdMinijuego id
)
{
    switch (id)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Inicializar();
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Inicializar();
            break;

        default:
            break;
    }
}


void GestorMinijuegos::ActivarMinijuego(
    IdMinijuego id,
    ContextoMinijuego& contexto
)
{
    minijuegoActivo = id;

    switch (id)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Inicializar();
            minijuegoColor.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Reiniciar(
                contexto.participantes
            );
            break;

        default:
            break;
    }
}


void GestorMinijuegos::ReiniciarActivo(
    ContextoMinijuego& contexto
)
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Reiniciar(
                contexto.participantes
            );
            break;

        default:
            break;
    }
}


void GestorMinijuegos::ActualizarActivo(
    float deltaTime,
    ContextoMinijuego& contexto
)
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        default:
            break;
    }
}


void GestorMinijuegos::DibujarActivo(
    const ContextoMinijuego& contexto,
    bool mostrarDebug
) const
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                mostrarDebug
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Dibujar(
                contexto.participantes
            );
            break;

        default:
            break;
    }
}


const ResultadoMinijuego*
GestorMinijuegos::ObtenerResultadoActivo() const
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            return &minijuegoColor.ObtenerResultado();

        case MINIJUEGO_CAPITAN_MANDA:
            return &minijuegoCapitanManda.ObtenerResultado();

        default:
            return nullptr;
    }
}
