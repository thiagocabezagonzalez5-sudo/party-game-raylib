#include "Core/CatalogoMinijuegos.h"


static const DatosMinijuegoCatalogo DATOS_MINIJUEGOS[
    CANTIDAD_MINIJUEGOS
] =
{
    {
        MINIJUEGO_COLOR_SEGURO,
        "COLOR SEGURO",
        "Corre a la isla del color indicado sobre un mar de lava rodeado de volcanes.",
        Color{ 235, 92, 85, 255 },
        "2 - COLOR SEGURO",
        true
    },
    {
        MINIJUEGO_PELOTAS,
        "PELOTAS",
        "Deslizate sobre una cumbre nevada altisima, choca rivales y tiralos de la montana.",
        Color{ 102, 184, 235, 255 },
        "3 - PELOTAS / EMPUJONES",
        true
    },
    {
        MINIJUEGO_TRONCO,
        "TRONCO 2V2",
        "Coordina con tu companero para avanzar mas rapido que el otro equipo.",
        Color{ 150, 102, 62, 255 },
        "5 - TRONCO COORDINADO",
        false
    },
    {
        MINIJUEGO_FABRICA_67,
        "FABRICA 67",
        "En equipo agarra los 6 y 7 que pasan por las cintas y arma 67 antes que el rival.",
        Color{ 229, 173, 62, 255 },
        "6 - FABRICA 67",
        false
    },
    {
        MINIJUEGO_ISLA_FUEGO,
        "ISLA BAJO FUEGO",
        "Esquiva los proyectiles. Un impacto directo te manda volando fuera de la arena.",
        Color{ 220, 98, 52, 255 },
        "8 - ISLA BAJO FUEGO",
        true
    },
    {
        MINIJUEGO_CAPITAN_MANDA,
        "CAPITAN MANDA",
        "Reacciona apenas aparece la bandera. Fallar o tardar te deja fuera.",
        Color{ 111, 95, 205, 255 },
        "9 - CAPITAN MANDA",
        true
    },
    {
        MINIJUEGO_BARRA_GIRATORIA,
        "BARRA GIRATORIA",
        "Salta la barra, golpea rivales y evita caer de la plataforma.",
        Color{ 67, 196, 143, 255 },
        "0 - BARRA GIRATORIA",
        true
    },
    {
        MINIJUEGO_NUCLEOS_ENERGIA,
        "NUCLEOS ENERGIA",
        "Recolecta energia. Un golpe suelta 3 recogidas y un ground pound puede soltar 5.",
        Color{ 70, 205, 225, 255 },
        "F10 - NUCLEOS DE ENERGIA",
        true
    },
    {
        MINIJUEGO_REFUGIO_PINCHOS,
        "REFUGIO TALADROS",
        "1 vs 3 en una cueva: uno elige desde que lado avanzan los taladros y el equipo se cubre.",
        Color{ 178, 99, 74, 255 },
        "F11 - REFUGIO DE TALADROS",
        true
    },
    {
        MINIJUEGO_MIRADAS_CRUZADAS,
        "MIRADAS CRUZADAS",
        "1 vs 3: evita mirar hacia el mismo lado que el jugador solitario durante cinco rondas.",
        Color{ 171, 124, 221, 255 },
        "F12 - MIRADAS CRUZADAS",
        true
    },
    {
        MINIJUEGO_MUROS_LOCOS,
        "MUROS LOCOS",
        "Busca el hueco de cada muro. Cada oleada acelera y los golpes pueden sacarte de la arena.",
        Color{ 231, 106, 72, 255 },
        "F9 - MUROS LOCOS",
        true
    },
    {
        MINIJUEGO_TORMENTA_MAGNETICA,
        "TORMENTA MAGNETICA",
        "Sobrevive a un nucleo que alterna atraccion y repulsion mientras cambia de posicion.",
        Color{ 74, 191, 220, 255 },
        "F8 - TORMENTA MAGNETICA",
        true
    },
    {
        MINIJUEGO_CONTEO_EXPLOSIVO,
        "CONTEO EXPLOSIVO",
        "Observa los drones en movimiento, recuerdalos y elige la cantidad exacta antes del final.",
        Color{ 244, 184, 61, 255 },
        "F6 - CONTEO EXPLOSIVO",
        true
    },
    {
        MINIJUEGO_PASO_SILENCIOSO,
        "PASO SILENCIOSO",
        "Avanza mientras el centinela duerme. Si te mueves cuando mira, perderas parte del camino.",
        Color{ 99, 205, 145, 255 },
        "F7 - PASO SILENCIOSO",
        true
    },
    {
        MINIJUEGO_CIRCUITO_VOLTAJE,
        "CIRCUITO VOLTAJE",
        "Completa cuatro vueltas. Acelera en las rectas y suelta en las curvas para evitar trompos.",
        Color{ 244, 108, 68, 255 },
        "F4 - CIRCUITO VOLTAJE",
        true
    },
    {
        MINIJUEGO_TRAZO_PERFECTO,
        "TRAZO PERFECTO",
        "Sigue el punto dorado alrededor de la figura. El recorrido mas preciso obtiene la victoria.",
        Color{ 102, 157, 235, 255 },
        "F5 - TRAZO PERFECTO",
        true
    },
    {
        MINIJUEGO_CARGA_INESTABLE,
        "CARGA INESTABLE",
        "Pasa la carga en cualquier sentido antes de que explote. El ultimo jugador en pie gana.",
        Color{ 239, 85, 112, 255 },
        "F1 - CARGA INESTABLE",
        true
    },
    {
        MINIJUEGO_SECUENCIA_NEON,
        "SECUENCIA NEON",
        "Memoriza los pulsos direccionales y repitelos sin equivocarte mientras la secuencia crece.",
        Color{ 119, 91, 222, 255 },
        "F2 - SECUENCIA NEON",
        true
    },
    {
        MINIJUEGO_INTERRUPTORES_CAOS,
        "INTERRUPTORES",
        "Elige un interruptor por turno. Si activas la sobrecarga quedas fuera; el ultimo en pie gana.",
        Color{ 226, 75, 107, 255 },
        "PAG ARRIBA - INTERRUPTORES DEL CAOS",
        true
    },
    {
        MINIJUEGO_TANQUES_PLASMA,
        "TANQUES PLASMA",
        "Combate cenital de tanques. Dos impactos te eliminan; esquiva, apunta y dispara hasta quedar solo.",
        Color{ 65, 190, 226, 255 },
        "PAG ABAJO - TANQUES DE PLASMA",
        true
    },
    {
        MINIJUEGO_PASARELAS_VACIO,
        "PASARELAS VACIO",
        "Cruza plataformas suspendidas que se derrumban bajo tus pies y llega primero a la meta.",
        Color{ 102, 160, 232, 255 },
        "INICIO - PASARELAS DEL VACIO",
        true
    },
    {
        MINIJUEGO_CANTERA_FUGA,
        "CANTERA EN FUGA",
        "1 vs 3: lanza rocas desde la cima o esquivalas mientras escalas una pendiente en 3D.",
        Color{ 196, 123, 68, 255 },
        "FIN - CANTERA EN FUGA",
        true
    }
};


bool EsIdMinijuegoValido(
    IdMinijuego id
)
{
    return
        id >= MINIJUEGO_COLOR_SEGURO &&
        id < CANTIDAD_MINIJUEGOS;
}


const DatosMinijuegoCatalogo& ObtenerDatosMinijuego(
    IdMinijuego id
)
{
    if (!EsIdMinijuegoValido(id))
    {
        return DATOS_MINIJUEGOS[0];
    }

    return DATOS_MINIJUEGOS[
        static_cast<int>(id)
    ];
}


const DatosMinijuegoCatalogo& ObtenerDatosMinijuegoPorIndice(
    int indice
)
{
    return ObtenerDatosMinijuego(
        ObtenerIdMinijuegoPorIndice(indice)
    );
}


IdMinijuego ObtenerIdMinijuegoPorIndice(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= CANTIDAD_MINIJUEGOS
    )
    {
        return MINIJUEGO_COLOR_SEGURO;
    }

    return DATOS_MINIJUEGOS[indice].id;
}


int ObtenerCantidadMinijuegosDisponiblesTablero()
{
    int cantidad = 0;

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        if (DATOS_MINIJUEGOS[i].disponibleEnTablero)
        {
            cantidad++;
        }
    }

    return cantidad;
}


IdMinijuego ObtenerMinijuegoDisponibleTablero(
    int indiceDisponible
)
{
    if (indiceDisponible < 0)
    {
        return MINIJUEGO_COLOR_SEGURO;
    }

    int indiceActual = 0;

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        if (!DATOS_MINIJUEGOS[i].disponibleEnTablero)
        {
            continue;
        }

        if (indiceActual == indiceDisponible)
        {
            return DATOS_MINIJUEGOS[i].id;
        }

        indiceActual++;
    }

    return MINIJUEGO_COLOR_SEGURO;
}
