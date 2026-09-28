#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/ResultadoMinijuego.h"
#include "Gameplay/ContextoMinijuego.h"
#include "Minigames/MinijuegoCapitanManda.h"
#include "Minigames/MinijuegoColorSeguro.h"


// Checkpoint inicial de ETAPA 2.
// Este gestor empieza con un minijuego sencillo y uno 3D para validar
// que ContextoMinijuego soporta ambas familias de firmas sin ocultarlas.
struct GestorMinijuegos
{
    IdMinijuego minijuegoActivo = MINIJUEGO_COLOR_SEGURO;

    MinijuegoColorSeguro minijuegoColor;
    MinijuegoCapitanManda minijuegoCapitanManda;

    void InicializarMinijuego(
        IdMinijuego id
    );

    void ActivarMinijuego(
        IdMinijuego id,
        ContextoMinijuego& contexto
    );

    void ReiniciarActivo(
        ContextoMinijuego& contexto
    );

    void ActualizarActivo(
        float deltaTime,
        ContextoMinijuego& contexto
    );

    void DibujarActivo(
        const ContextoMinijuego& contexto,
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego* ObtenerResultadoActivo() const;
};
