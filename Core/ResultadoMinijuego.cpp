#include "Core/ResultadoMinijuego.h"


void InicializarResultadoMinijuego(
    ResultadoMinijuego& resultado,
    const Participante participantes[],
    FormatoMinijuego formato
)
{
    resultado = {};
    resultado.formato = formato;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo =
            participantes[i].activo;

        if (participantes[i].activo)
        {
            resultado.cantidadParticipantes++;
        }
    }
}


bool ResultadoMinijuegoFinalizado(
    const ResultadoMinijuego& resultado
)
{
    return resultado.estado ==
        RESULTADO_MINIJUEGO_FINALIZADO;
}


bool ParticipanteEsGanador(
    const ResultadoMinijuego& resultado,
    int indiceParticipante
)
{
    if (
        indiceParticipante < 0 ||
        indiceParticipante >= MAX_PARTICIPANTES ||
        resultado.estado != RESULTADO_MINIJUEGO_FINALIZADO ||
        !resultado.participantes[indiceParticipante].participo
    )
    {
        return false;
    }

    if (resultado.formato == FORMATO_MINIJUEGO_COOPERATIVO)
    {
        return resultado.desenlace ==
            DESENLACE_VICTORIA_COOPERATIVA;
    }

    const ResultadoParticipante& participante =
        resultado.participantes[indiceParticipante];

    if (resultado.formato == FORMATO_MINIJUEGO_INDIVIDUAL)
    {
        return participante.posicionFinal == 1;
    }

    if (participante.numeroEquipo < 0)
    {
        return false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const ResultadoParticipante& integrante =
            resultado.participantes[i];

        if (
            integrante.participo &&
            integrante.numeroEquipo == participante.numeroEquipo &&
            integrante.posicionFinal == 1
        )
        {
            return true;
        }
    }

    return false;
}


int ObtenerIndicesGanadores(
    const ResultadoMinijuego& resultado,
    int indicesGanadores[],
    int capacidad
)
{
    if (indicesGanadores == nullptr || capacidad <= 0)
    {
        return 0;
    }

    int cantidad = 0;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES && cantidad < capacidad;
        i++
    )
    {
        if (ParticipanteEsGanador(resultado, i))
        {
            indicesGanadores[cantidad] = i;
            cantidad++;
        }
    }

    return cantidad;
}


bool ValidarResultadoMinijuego(
    const ResultadoMinijuego& resultado,
    const Participante participantes[]
)
{
    if (participantes == nullptr)
    {
        return false;
    }

    if (
        resultado.formato < FORMATO_MINIJUEGO_INDIVIDUAL ||
        resultado.formato > FORMATO_MINIJUEGO_COOPERATIVO ||
        resultado.estado < RESULTADO_MINIJUEGO_EN_CURSO ||
        resultado.estado > RESULTADO_MINIJUEGO_CANCELADO ||
        resultado.desenlace < DESENLACE_SIN_DEFINIR ||
        resultado.desenlace > DESENLACE_COMPLETADO
    )
    {
        return false;
    }

    if (
        resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO &&
        resultado.desenlace != DESENLACE_SIN_DEFINIR
    )
    {
        return false;
    }

    if (
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        resultado.desenlace == DESENLACE_SIN_DEFINIR
    )
    {
        return false;
    }

    if (
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO &&
        resultado.desenlace != DESENLACE_SIN_DEFINIR
    )
    {
        return false;
    }

    bool desenlaceCooperativo =
        resultado.desenlace == DESENLACE_VICTORIA_COOPERATIVA ||
        resultado.desenlace == DESENLACE_DERROTA_COOPERATIVA;

    if (
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        desenlaceCooperativo &&
        resultado.formato != FORMATO_MINIJUEGO_COOPERATIVO
    )
    {
        return false;
    }

    if (
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        resultado.formato == FORMATO_MINIJUEGO_COOPERATIVO &&
        !desenlaceCooperativo &&
        resultado.desenlace != DESENLACE_COMPLETADO
    )
    {
        return false;
    }

    if (
        resultado.formato == FORMATO_MINIJUEGO_EQUIPOS
    )
    {
        if (
            resultado.cantidadEquipos < 0 ||
            resultado.cantidadEquipos > MAX_PARTICIPANTES ||
            (
                resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
                resultado.cantidadEquipos == 0
            )
        )
        {
            return false;
        }
    }
    else if (resultado.cantidadEquipos != 0)
    {
        return false;
    }

    int cantidadReal = 0;
    int cantidadGanadores = 0;
    bool equiposGanadores[MAX_PARTICIPANTES]{};
    int cantidadEquiposGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const ResultadoParticipante& resultadoParticipante =
            resultado.participantes[i];

        if (resultadoParticipante.posicionFinal < 0)
        {
            return false;
        }

        if (
            resultadoParticipante.participo &&
            !participantes[i].activo
        )
        {
            return false;
        }

        if (
            resultado.estado == RESULTADO_MINIJUEGO_CANCELADO &&
            resultadoParticipante.puntosObtenidos != 0
        )
        {
            return false;
        }

        if (
            resultado.formato != FORMATO_MINIJUEGO_EQUIPOS &&
            resultadoParticipante.numeroEquipo != -1
        )
        {
            return false;
        }

        if (!resultadoParticipante.participo)
        {
            if (resultadoParticipante.numeroEquipo != -1)
            {
                return false;
            }

            continue;
        }

        cantidadReal++;

        if (
            resultado.formato == FORMATO_MINIJUEGO_EQUIPOS
        )
        {
            bool equipoPendiente =
                resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO &&
                resultado.cantidadEquipos == 0 &&
                resultadoParticipante.numeroEquipo == -1;

            if (
                !equipoPendiente &&
                (
                    resultadoParticipante.numeroEquipo < 0 ||
                    resultadoParticipante.numeroEquipo >=
                        resultado.cantidadEquipos
                )
            )
            {
                return false;
            }
        }
        if (ParticipanteEsGanador(resultado, i))
        {
            cantidadGanadores++;

            if (
                resultado.formato == FORMATO_MINIJUEGO_EQUIPOS &&
                !equiposGanadores[
                    resultadoParticipante.numeroEquipo
                ]
            )
            {
                equiposGanadores[
                    resultadoParticipante.numeroEquipo
                ] = true;

                cantidadEquiposGanadores++;
            }
        }
    }

    if (cantidadReal != resultado.cantidadParticipantes)
    {
        return false;
    }

    if (
        resultado.estado != RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        return cantidadGanadores == 0;
    }

    if (resultado.desenlace == DESENLACE_CON_GANADOR)
    {
        if (
            resultado.formato == FORMATO_MINIJUEGO_INDIVIDUAL &&
            cantidadGanadores != 1
        )
        {
            return false;
        }

        if (
            resultado.formato == FORMATO_MINIJUEGO_EQUIPOS &&
            cantidadEquiposGanadores != 1
        )
        {
            return false;
        }
    }

    if (resultado.desenlace == DESENLACE_EMPATE)
    {
        if (
            resultado.formato == FORMATO_MINIJUEGO_INDIVIDUAL &&
            cantidadGanadores < 2
        )
        {
            return false;
        }

        if (
            resultado.formato == FORMATO_MINIJUEGO_EQUIPOS &&
            cantidadEquiposGanadores < 2
        )
        {
            return false;
        }
    }

    if (
        (
            resultado.desenlace == DESENLACE_DERROTA_COOPERATIVA ||
            resultado.desenlace == DESENLACE_COMPLETADO
        ) &&
        cantidadGanadores != 0
    )
    {
        return false;
    }

    if (
        resultado.desenlace == DESENLACE_VICTORIA_COOPERATIVA &&
        cantidadGanadores != cantidadReal
    )
    {
        return false;
    }

    return true;
}
