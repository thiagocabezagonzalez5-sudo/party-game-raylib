#include "Core/CatalogoMinijuegos.h"


static const DatosMinijuegoCatalogo DATOS_MINIJUEGOS[] =
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
        true,
        true
    },
    {
        MINIJUEGO_FABRICA_67,
        "FABRICA 67",
        "En equipo agarra los 6 y 7 que pasan por las cintas y arma 67 antes que el rival.",
        Color{ 229, 173, 62, 255 },
        "6 - FABRICA 67",
        true,
        true
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
    },
    {
        MINIJUEGO_TERRITORIO_CONQUISTA,
        "TERRITORIO",
        "Pisa baldosas para pintarlas de tu color. El ground pound reclama un area; gana quien domine mas.",
        Color{ 120, 200, 90, 255 },
        "CATALOGO - TERRITORIO EN CONQUISTA",
        true
    },
    {
        MINIJUEGO_DEFENSA_NUCLEO,
        "DEFENSA NUCLEO",
        "2 vs 2: devuelve el nucleo que rebota y acelera hasta meterlo en el arco rival. Primero a 5 gana.",
        Color{ 90, 140, 240, 255 },
        "CATALOGO - DEFENSA DEL NUCLEO",
        true
    },
    {
        MINIJUEGO_LLUVIA_APILADA,
        "LLUVIA APILADA",
        "Atrapa las piezas que caen, apilalas y aseguralas en tu base. Un golpe hace caer la pila rival.",
        Color{ 235, 150, 200, 255 },
        "CATALOGO - LLUVIA APILADA",
        true
    },
    {
        MINIJUEGO_CUERDA_ACANTILADO,
        "CUERDA ACANTILADO",
        "1 vs 3: tira de la cuerda al ritmo del pulso. El bando arrastrado cae al mar.",
        Color{ 200, 170, 110, 255 },
        "CATALOGO - CUERDA DEL ACANTILADO",
        true
    },
    {
        MINIJUEGO_ULTIMO_ASIENTO,
        "ULTIMO ASIENTO",
        "Cuando calla la musica, ocupa una taza. Empuja rivales: el que queda sin taza sale.",
        Color{ 150, 90, 220, 255 },
        "CATALOGO - ULTIMO ASIENTO",
        true
    },
    {
        MINIJUEGO_CAJAS_PUERTO,
        "CAJAS DEL PUERTO",
        "1 vs 3: escondete en un contenedor y evita las cargas de la grua del solitario.",
        Color{ 52, 120, 160, 255 },
        "CATALOGO - CAJAS DEL PUERTO",
        true
    },
    {
        MINIJUEGO_LABERINTO_INCLINADO,
        "LABERINTO JADE",
        "Inclina tu losa del templo y guia la esfera de jade al altar; evita agujeros y dardos.",
        Color{ 52, 176, 128, 255 },
        "CATALOGO - LABERINTO JADE",
        true
    },
    {
        MINIJUEGO_VETA_CRISTAL,
        "VETA DE CRISTAL",
        "2 vs 2: golpea geodas con la cabeza, junta gemas y esquiva la vagoneta.",
        Color{ 90, 200, 235, 255 },
        "CATALOGO - VETA DE CRISTAL",
        true
    },
    {
        MINIJUEGO_CAPSULAS_BARAJADAS,
        "CAPSULAS BARAJADAS",
        "Sigue el nucleo entre capsulas barajadas y elige. Confirmar rapido vale doble. 2-4 jugadores",
        Color{ 80, 230, 170, 255 },
        "CATALOGO - CAPSULAS BARAJADAS",
        true
    },
    {
        MINIJUEGO_BATEO_METEORICO,
        "BATEO METEORICO",
        "Batea meteoritos en el momento justo para lanzarlos a los anillos; el dorado vale doble, el rojo no.",
        Color{ 80, 140, 240, 255 },
        "CATALOGO - BATEO METEORICO",
        true
    },
    {
        MINIJUEGO_RACIMO_TOXICO,
        "RACIMO TOXICO",
        "Por turnos toma 1 o 2 frutos del racimo; las bayas toxicas quitan vidas y el dorado salta un turno.",
        Color{ 70, 150, 70, 255 },
        "CATALOGO - RACIMO TOXICO",
        true
    },
    {
        MINIJUEGO_TESORERO_ACORRALADO,
        "TESORERO CERCADO",
        "El trio persigue al tesorero y lo golpea para que suelte monedas. Las rejas cambian el patio. 1 vs 3",
        Color{ 232, 186, 64, 255 },
        "CATALOGO - TESORERO CERCADO",
        true
    },
    {
        MINIJUEGO_DESCENSO_NUBES,
        "DESCENSO EN NUBES",
        "Planea entre nubes, recoge anillos, esquiva tormentas y aterriza en el centro. 2-4 jugadores",
        Color{ 110, 190, 245, 255 },
        "CATALOGO - DESCENSO EN NUBES",
        true
    },
    {
        MINIJUEGO_VOLEA_MAGMA,
        "VOLEA DE MAGMA",
        "Voley 2v2 sobre obsidiana: golpea la roca de magma sobre la red; si cae en tu mitad, punto rival.",
        Color{ 255, 120, 40, 255 },
        "CATALOGO - VOLEA DE MAGMA",
        true
    },
    {
        MINIJUEGO_PAREJAS_GLACIAR,
        "PAREJAS GLACIARES",
        "Memoria por turnos: derrite bloques de hielo, haz parejas y busca la aurora que vale 3 puntos.",
        Color{ 90, 170, 220, 255 },
        "CATALOGO - PAREJAS GLACIARES",
        true
    },
    {
        MINIJUEGO_ESFERAS_CANON,
        "ESFERAS DEL CANON",
        "Rueda tu esfera de piedra por el canon: esquiva grietas, arena y cactus. Rampa de atajo. 2-4 jugadores",
        Color{ 214, 110, 64, 255 },
        "CATALOGO - ESFERAS DEL CANON",
        true
    },
    {
        MINIJUEGO_PESCA_ISLA,
        "PESCA ISLENA",
        "Lanza el anzuelo, engancha al pez justo al picar y gana el tira y afloja. Cuidado con las botas",
        Color{ 60, 210, 215, 255 },
        "CATALOGO - PESCA ISLENA",
        true
    },
    {
        MINIJUEGO_RODILLOS_NEON,
        "RODILLOS NEON",
        "Detiene tus 3 rodillos neon en el momento justo: iguales dan puntos y parar en la linea da comodin.",
        Color{ 255, 60, 200, 255 },
        "CATALOGO - RODILLOS NEON",
        true
    },
    {
        MINIJUEGO_BOLAS_AZUCAR,
        "BOLAS DE AZUCAR",
        "Crea una bola, hazla crecer rodando por azucar glas y lanzala a tus rivales. El chocolate la derrite.",
        Color{ 240, 110, 160, 255 },
        "CATALOGO - BOLAS DE AZUCAR",
        true
    },
    {
        MINIJUEGO_GRUA_CHATARRA,
        "GRUA DE CHATARRA",
        "Maneja tu garra magnetica, centrala sobre la chatarra valiosa y llevala a tu tolva. 2-4 jugadores",
        Color{ 232, 150, 48, 255 },
        "CATALOGO - GRUA DE CHATARRA",
        true
    },
    {
        MINIJUEGO_PISOTON_PLAGAS,
        "PISOTON DE PLAGAS",
        "Aplasta plagas con ground pound en un jardin gigante. Esquiva avispas y disputa la flor. Todos contra todos",
        Color{ 96, 172, 64, 255 },
        "CATALOGO - PISOTON DE PLAGAS",
        true
    },
    {
        MINIJUEGO_BALSAS_RAPIDO,
        "BALSAS DEL RAPIDO",
        "Rema en equipo por el rio de la selva, esquiva rocas y elige cascada o remanso. Primero a la meta.",
        Color{ 40, 190, 170, 255 },
        "CATALOGO - BALSAS DEL RAPIDO",
        true
    },
    {
        MINIJUEGO_AUTOS_GLOBO,
        "AUTOS DE GLOBO",
        "Autos flotantes con globos: embiste de lado para reventar los de tus rivales. Usa el turbo y las placas",
        Color{ 96, 200, 240, 255 },
        "CATALOGO - AUTOS DE GLOBO",
        true
    },
    {
        MINIJUEGO_SENDERO_INVISIBLE,
        "SENDERO INVISIBLE",
        "Memoriza la ruta segura de las losas del cementerio. Cada caida revela una grieta a todos.",
        Color{ 120, 230, 150, 255 },
        "CATALOGO - SENDERO INVISIBLE",
        true
    },
    {
        MINIJUEGO_BANQUETE_TURBO,
        "BANQUETE TURBO",
        "Machaca el boton para devorar raciones espaciales. Rechaza las picantes con salto. Las doradas valen x2",
        Color{ 90, 170, 240, 255 },
        "CATALOGO - BANQUETE TURBO",
        true
    },
    {
        MINIJUEGO_TUBERIAS_DESIERTO,
        "TUBERIAS DESIERTO",
        "Sigue las tuberias de piedra del oasis y elige la entrada del cantaro dorado. Cuidado con el espejismo.",
        Color{ 235, 170, 70, 255 },
        "CATALOGO - TUBERIAS DESIERTO",
        true
    },
    {
        MINIJUEGO_TREPA_MASTIL,
        "TREPA EL MASTIL",
        "Alterna salto y accion para trepar tu mastil. Aprovecha las olas y esquiva cuervos. Gana el primero en la cofa",
        Color{ 70, 140, 210, 255 },
        "CATALOGO - TREPA EL MASTIL",
        true
    },
    {
        MINIJUEGO_GUARDIAN_RUINAS,
        "GUARDIAN DE RUINAS",
        "Un guardian defiende el portal con escudo y embestida; el trio lanza orbes que rebotan en las columnas.",
        Color{ 100, 220, 200, 255 },
        "CATALOGO - GUARDIAN DE RUINAS",
        true
    }
};

static_assert(
    sizeof(DATOS_MINIJUEGOS) / sizeof(DATOS_MINIJUEGOS[0]) ==
        CANTIDAD_MINIJUEGOS,
    "Cada IdMinijuego necesita su entrada en DATOS_MINIJUEGOS"
);


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


bool MinijuegoAdmiteCantidadJugadores(
    IdMinijuego id,
    int cantidadJugadores
)
{
    if (cantidadJugadores < 2)
    {
        return false;
    }

    if (ObtenerDatosMinijuego(id).requiereParDeJugadores)
    {
        return cantidadJugadores % 2 == 0;
    }

    return true;
}
