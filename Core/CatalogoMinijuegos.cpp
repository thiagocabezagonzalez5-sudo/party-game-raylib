#include "Core/CatalogoMinijuegos.h"


static const DatosMinijuegoCatalogo DATOS_MINIJUEGOS[] =
{
    {
        MINIJUEGO_COLOR_SEGURO,
        "COLOR SEGURO",
        "Corré a la isla del color indicado sobre un mar de lava rodeado de volcanes.",
        Color{ 235, 92, 85, 255 },
        "2 - COLOR SEGURO",
        true
    },
    {
        MINIJUEGO_PELOTAS,
        "PELOTAS",
        "Deslizate sobre una cumbre nevada altísima, chocá rivales y tiralos de la montaña.",
        Color{ 102, 184, 235, 255 },
        "3 - PELOTAS / EMPUJONES",
        true
    },
    {
        MINIJUEGO_TRONCO,
        "TRONCO 2V2",
        "Coordiná con tu compañero para avanzar más rápido que el otro equipo.",
        Color{ 150, 102, 62, 255 },
        "5 - TRONCO COORDINADO",
        true,
        true
    },
    {
        MINIJUEGO_FABRICA_67,
        "FÁBRICA 67",
        "En equipo, agarrá los 6 y 7 que pasan por las cintas y armá 67 antes que el rival.",
        Color{ 229, 173, 62, 255 },
        "6 - FÁBRICA 67",
        true,
        true
    },
    {
        MINIJUEGO_ISLA_FUEGO,
        "ISLA BAJO FUEGO",
        "Esquivá los proyectiles. Un impacto directo te manda volando fuera de la arena.",
        Color{ 220, 98, 52, 255 },
        "8 - ISLA BAJO FUEGO",
        true
    },
    {
        MINIJUEGO_CAPITAN_MANDA,
        "CAPITÁN MANDA",
        "Reaccioná apenas aparece la bandera. Fallar o tardar te deja afuera.",
        Color{ 111, 95, 205, 255 },
        "9 - CAPITÁN MANDA",
        true
    },
    {
        MINIJUEGO_BARRA_GIRATORIA,
        "BARRA GIRATORIA",
        "Saltá la barra, golpeá rivales y evitá caer de la plataforma.",
        Color{ 67, 196, 143, 255 },
        "0 - BARRA GIRATORIA",
        true
    },
    {
        MINIJUEGO_NUCLEOS_ENERGIA,
        "NÚCLEOS ENERGIA",
        "Recolectá energía. Un golpe suelta 3 recogidas y un ground pound puede soltar 5.",
        Color{ 70, 205, 225, 255 },
        "F10 - NÚCLEOS DE ENERGIA",
        true
    },
    {
        MINIJUEGO_REFUGIO_PINCHOS,
        "REFUGIO TALADROS",
        "1 vs 3 en una cueva: uno elige desde qué lado avanzan los taladros y el equipo se cubre.",
        Color{ 178, 99, 74, 255 },
        "F11 - REFUGIO DE TALADROS",
        true
    },
    {
        MINIJUEGO_MIRADAS_CRUZADAS,
        "MIRADAS CRUZADAS",
        "1 vs 3: evitá mirar hacia el mismo lado que el jugador solitario durante cinco rondas.",
        Color{ 171, 124, 221, 255 },
        "F12 - MIRADAS CRUZADAS",
        true
    },
    {
        MINIJUEGO_MUROS_LOCOS,
        "MUROS LOCOS",
        "Buscá el hueco de cada muro. Cada oleada acelera y los golpes pueden sacarte de la arena.",
        Color{ 231, 106, 72, 255 },
        "F9 - MUROS LOCOS",
        true
    },
    {
        MINIJUEGO_TORMENTA_MAGNETICA,
        "TORMENTA MAGNÉTICA",
        "Sobreviví a un núcleo que alterna atracción y repulsión mientras cambia de posición.",
        Color{ 74, 191, 220, 255 },
        "F8 - TORMENTA MAGNÉTICA",
        true
    },
    {
        MINIJUEGO_CONTEO_EXPLOSIVO,
        "CONTEO EXPLOSIVO",
        "Observá los drones en movimiento, recordalos y elegí la cantidad exacta antes del final.",
        Color{ 244, 184, 61, 255 },
        "F6 - CONTEO EXPLOSIVO",
        true
    },
    {
        MINIJUEGO_PASO_SILENCIOSO,
        "PASO SILENCIOSO",
        "Avanzá mientras el centinela duerme. Si te movés cuando mira, perdés parte del camino.",
        Color{ 99, 205, 145, 255 },
        "F7 - PASO SILENCIOSO",
        true
    },
    {
        MINIJUEGO_CIRCUITO_VOLTAJE,
        "CIRCUITO VOLTAJE",
        "Completá cuatro vueltas. Acelerá en las rectas y soltá en las curvas para evitar trompos.",
        Color{ 244, 108, 68, 255 },
        "F4 - CIRCUITO VOLTAJE",
        true
    },
    {
        MINIJUEGO_TRAZO_PERFECTO,
        "TRAZO PERFECTO",
        "Seguí el punto dorado alrededor de la figura. El recorrido más preciso obtiene la victoria.",
        Color{ 102, 157, 235, 255 },
        "F5 - TRAZO PERFECTO",
        true
    },
    {
        MINIJUEGO_CARGA_INESTABLE,
        "CARGA INESTABLE",
        "Pasá la carga en cualquier sentido antes de que explote. El último jugador en pie gana.",
        Color{ 239, 85, 112, 255 },
        "F1 - CARGA INESTABLE",
        true
    },
    {
        MINIJUEGO_SECUENCIA_NEON,
        "SECUENCIA NEÓN",
        "Memorizá los pulsos direccionales y repetilos sin equivocarte mientras la secuencia crece.",
        Color{ 119, 91, 222, 255 },
        "F2 - SECUENCIA NEÓN",
        true
    },
    {
        MINIJUEGO_INTERRUPTORES_CAOS,
        "INTERRUPTORES",
        "Elegí un interruptor por turno. Si activás la sobrecarga, quedás afuera; el último en pie gana.",
        Color{ 226, 75, 107, 255 },
        "PÁG ARRIBA - INTERRUPTORES DEL CAOS",
        true
    },
    {
        MINIJUEGO_TANQUES_PLASMA,
        "TANQUES PLASMA",
        "Combate cenital de tanques. Dos impactos te eliminan; esquivá, apuntá y dispará hasta quedar solo.",
        Color{ 65, 190, 226, 255 },
        "PÁG ABAJO - TANQUES DE PLASMA",
        true
    },
    {
        MINIJUEGO_PASARELAS_VACIO,
        "PASARELAS VACIO",
        "Cruzá plataformas suspendidas que se derrumban bajo tus pies y llegá primero a la meta.",
        Color{ 102, 160, 232, 255 },
        "INICIO - PASARELAS DEL VACIO",
        true
    },
    {
        MINIJUEGO_CANTERA_FUGA,
        "CANTERA EN FUGA",
        "1 vs 3: lanzá rocas desde la cima o esquivalas mientras escalás una pendiente en 3D.",
        Color{ 196, 123, 68, 255 },
        "FIN - CANTERA EN FUGA",
        true
    },
    {
        MINIJUEGO_TERRITORIO_CONQUISTA,
        "TERRITORIO",
        "Pisá baldosas para pintarlas de tu color. El ground pound reclama un área; gana quien domine más.",
        Color{ 120, 200, 90, 255 },
        "CATÁLOGO - TERRITORIO EN CONQUISTA",
        true
    },
    {
        MINIJUEGO_DEFENSA_NUCLEO,
        "DEFENSA NÚCLEO",
        "2 vs 2: devolvé el núcleo que rebota y acelera hasta meterlo en el arco rival. Primero a 5 gana.",
        Color{ 90, 140, 240, 255 },
        "CATÁLOGO - DEFENSA DEL NÚCLEO",
        true
    },
    {
        MINIJUEGO_LLUVIA_APILADA,
        "LLUVIA APILADA",
        "Atrapá las piezas que caen, apilalas y aseguralas en tu base. Un golpe hace caer la pila rival.",
        Color{ 235, 150, 200, 255 },
        "CATÁLOGO - LLUVIA APILADA",
        true
    },
    {
        MINIJUEGO_CUERDA_ACANTILADO,
        "CUERDA ACANTILADO",
        "1 vs 3: tirá de la cuerda al ritmo del pulso. El bando arrastrado cae al mar.",
        Color{ 200, 170, 110, 255 },
        "CATÁLOGO - CUERDA DEL ACANTILADO",
        true
    },
    {
        MINIJUEGO_ULTIMO_ASIENTO,
        "ÚLTIMO ASIENTO",
        "Cuando calla la música, ocupá una taza. Empujá rivales: el que queda sin taza sale.",
        Color{ 150, 90, 220, 255 },
        "CATÁLOGO - ÚLTIMO ASIENTO",
        true
    },
    {
        MINIJUEGO_CAJAS_PUERTO,
        "CAJAS DEL PUERTO",
        "1 vs 3: escondete en un contenedor y evitá las cargas de la grúa del solitario.",
        Color{ 52, 120, 160, 255 },
        "CATÁLOGO - CAJAS DEL PUERTO",
        true
    },
    {
        MINIJUEGO_LABERINTO_INCLINADO,
        "LABERINTO JADE",
        "Incliná tu losa del templo y guiá la esfera de jade al altar; evitá agujeros y dardos.",
        Color{ 52, 176, 128, 255 },
        "CATÁLOGO - LABERINTO JADE",
        true
    },
    {
        MINIJUEGO_VETA_CRISTAL,
        "VETA DE CRISTAL",
        "2 vs 2: golpeá geodas con la cabeza, juntá gemas y esquivá la vagoneta.",
        Color{ 90, 200, 235, 255 },
        "CATÁLOGO - VETA DE CRISTAL",
        true
    },
    {
        MINIJUEGO_CAPSULAS_BARAJADAS,
        "CÁPSULAS BARAJADAS",
        "Seguí el núcleo entre cápsulas barajadas y elegí. Confirmar rápido vale doble. 2-4 jugadores",
        Color{ 80, 230, 170, 255 },
        "CATÁLOGO - CÁPSULAS BARAJADAS",
        true
    },
    {
        MINIJUEGO_BATEO_METEORICO,
        "BATEO METEÓRICO",
        "Bateá meteoritos en el momento justo para lanzarlos a los anillos; el dorado vale doble, el rojo no.",
        Color{ 80, 140, 240, 255 },
        "CATÁLOGO - BATEO METEÓRICO",
        true
    },
    {
        MINIJUEGO_RACIMO_TOXICO,
        "RACIMO TÓXICO",
        "Por turnos, tomá 1 o 2 frutos del racimo; las bayas tóxicas quitan vidas y el dorado salta un turno.",
        Color{ 70, 150, 70, 255 },
        "CATÁLOGO - RACIMO TÓXICO",
        true
    },
    {
        MINIJUEGO_TESORERO_ACORRALADO,
        "TESORERO CERCADO",
        "El trío persigue al tesorero y lo golpea para que suelte monedas. Las rejas cambian el patio. 1 vs 3",
        Color{ 232, 186, 64, 255 },
        "CATÁLOGO - TESORERO CERCADO",
        true
    },
    {
        MINIJUEGO_DESCENSO_NUBES,
        "DESCENSO EN NUBES",
        "Planeá entre nubes, recogé anillos, esquivá tormentas y aterrizá en el centro. 2-4 jugadores",
        Color{ 110, 190, 245, 255 },
        "CATÁLOGO - DESCENSO EN NUBES",
        true
    },
    {
        MINIJUEGO_VOLEA_MAGMA,
        "VOLEA DE MAGMA",
        "Vóley 2v2 sobre obsidiana: golpeá la roca de magma sobre la red; si cae en tu mitad, punto rival.",
        Color{ 255, 120, 40, 255 },
        "CATÁLOGO - VOLEA DE MAGMA",
        true
    },
    {
        MINIJUEGO_PAREJAS_GLACIAR,
        "PAREJAS GLACIARES",
        "Memoria por turnos: derretí bloques de hielo, hacé parejas y buscá la aurora que vale 3 puntos.",
        Color{ 90, 170, 220, 255 },
        "CATÁLOGO - PAREJAS GLACIARES",
        true
    },
    {
        MINIJUEGO_ESFERAS_CANON,
        "ESFERAS DEL CAÑÓN",
        "Rodá tu esfera de piedra por el cañón: esquivá grietas, arena y cactus. Rampa de atajo. 2-4 jugadores",
        Color{ 214, 110, 64, 255 },
        "CATÁLOGO - ESFERAS DEL CAÑÓN",
        true
    },
    {
        MINIJUEGO_PESCA_ISLA,
        "PESCA ISLEÑA",
        "Lanzá el anzuelo, enganchá al pez justo al picar y ganá el tira y afloja. ¡Cuidado con las botas!",
        Color{ 60, 210, 215, 255 },
        "CATÁLOGO - PESCA ISLEÑA",
        true
    },
    {
        MINIJUEGO_RODILLOS_NEON,
        "RODILLOS NEÓN",
        "Detené tus 3 rodillos neón en el momento justo: iguales dan puntos y parar en la línea da comodín.",
        Color{ 255, 60, 200, 255 },
        "CATÁLOGO - RODILLOS NEÓN",
        true
    },
    {
        MINIJUEGO_BOLAS_AZUCAR,
        "BOLAS DE AZÚCAR",
        "Creá una bola, hacela crecer rodando por azúcar glas y lanzala a tus rivales. El chocolate la derrite.",
        Color{ 240, 110, 160, 255 },
        "CATÁLOGO - BOLAS DE AZÚCAR",
        true
    },
    {
        MINIJUEGO_GRUA_CHATARRA,
        "GRÚA DE CHATARRA",
        "Manejá tu garra magnética, centrala sobre la chatarra valiosa y llevala a tu tolva. 2-4 jugadores",
        Color{ 232, 150, 48, 255 },
        "CATÁLOGO - GRÚA DE CHATARRA",
        true
    },
    {
        MINIJUEGO_PISOTON_PLAGAS,
        "PISOTÓN DE PLAGAS",
        "Aplastá plagas con ground pound en un jardín gigante. Esquivá avispas y disputá la flor. Todos contra todos",
        Color{ 96, 172, 64, 255 },
        "CATÁLOGO - PISOTÓN DE PLAGAS",
        true
    },
    {
        MINIJUEGO_BALSAS_RAPIDO,
        "BALSAS DEL RÁPIDO",
        "Remá en equipo por el río de la selva, esquivá rocas y elegí cascada o remanso. Primero a la meta.",
        Color{ 40, 190, 170, 255 },
        "CATÁLOGO - BALSAS DEL RÁPIDO",
        true
    },
    {
        MINIJUEGO_AUTOS_GLOBO,
        "AUTOS DE GLOBO",
        "Autos flotantes con globos: embestí de lado para reventar los de tus rivales. Usá el turbo y las placas.",
        Color{ 96, 200, 240, 255 },
        "CATÁLOGO - AUTOS DE GLOBO",
        true
    },
    {
        MINIJUEGO_SENDERO_INVISIBLE,
        "SENDERO INVISIBLE",
        "Memorizá la ruta segura de las losas del cementerio. Cada caída revela una grieta a todos.",
        Color{ 120, 230, 150, 255 },
        "CATÁLOGO - SENDERO INVISIBLE",
        true
    },
    {
        MINIJUEGO_BANQUETE_TURBO,
        "BANQUETE TURBO",
        "Machacá el botón para devorar raciones espaciales. Rechazá las picantes con salto. Las doradas valen x2",
        Color{ 90, 170, 240, 255 },
        "CATÁLOGO - BANQUETE TURBO",
        true
    },
    {
        MINIJUEGO_TUBERIAS_DESIERTO,
        "TUBERIAS DESIERTO",
        "Seguí las tuberías de piedra del oasis y elegí la entrada del cántaro dorado. ¡Cuidado con el espejismo!",
        Color{ 235, 170, 70, 255 },
        "CATÁLOGO - TUBERIAS DESIERTO",
        true
    },
    {
        MINIJUEGO_TREPA_MASTIL,
        "TREPÁ EL MÁSTIL",
        "Alterná salto y acción para trepar tu mástil. Aprovechá las olas y esquivá cuervos. Gana el primero en la cofa.",
        Color{ 70, 140, 210, 255 },
        "CATÁLOGO - TREPÁ EL MÁSTIL",
        true
    },
    {
        MINIJUEGO_GUARDIAN_RUINAS,
        "GUARDIÁN DE RUINAS",
        "Un guardián defiende el portal con escudo y embestida; el trío lanza orbes que rebotan en las columnas.",
        Color{ 100, 220, 200, 255 },
        "CATÁLOGO - GUARDIÁN DE RUINAS",
        true
    },
    {
        MINIJUEGO_CABLES_CRUZADOS,
        "CABLES CRUZADOS",
        "Duelo 1 contra 1: girá las piezas de cable para conectar la pila con la bombilla antes que tu rival. Gana quien llegue a 2 rondas.",
        Color{ 255, 205, 80, 255 },
        "CATÁLOGO - CABLES CRUZADOS",
        true,
        false,
        true
    },
    {
        MINIJUEGO_VAGONETAS_DUELO,
        "VAGONETAS DE MINA",
        "Duelo 1 contra 1: carrera en vagonetas por una mina de lava. Cambiá de riel, saltá las vías rotas y gastá el turbo con cabeza. Gana quien cruce la meta primero.",
        Color{ 255, 120, 50, 255 },
        "CATÁLOGO - VAGONETAS DE MINA",
        true,
        false,
        true
    },
    {
        MINIJUEGO_GEMAS_DESLIZANTES,
        "GEMAS DESLIZANTES",
        "Duelo 1 contra 1: deslizá las gemas y armá el dibujo del centro antes que tu rival. Mejor de 3 rondas.",
        Color{ 150, 110, 235, 255 },
        "CATÁLOGO - GEMAS DESLIZANTES",
        true,
        false,
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
        // Los de duelo (esDuelo) son exclusivos: no salen en las rondas.
        if (
            DATOS_MINIJUEGOS[i].disponibleEnTablero &&
            !DATOS_MINIJUEGOS[i].esDuelo
        )
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
        if (
            !DATOS_MINIJUEGOS[i].disponibleEnTablero ||
            DATOS_MINIJUEGOS[i].esDuelo
        )
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

    // Un minijuego de duelo es 1 contra 1: solo admite dos participantes.
    if (ObtenerDatosMinijuego(id).esDuelo)
    {
        return cantidadJugadores == 2;
    }

    if (ObtenerDatosMinijuego(id).requiereParDeJugadores)
    {
        return cantidadJugadores % 2 == 0;
    }

    return true;
}


// Solo para pruebas (harness): reemplaza el pool de duelo sin tocar el
// catalogo. Cantidad 0 o negativa vuelve a leer los marcados esDuelo.
static IdMinijuego poolDueloPruebas[CANTIDAD_MINIJUEGOS];
static int cantidadPoolDueloPruebas = 0;

void ForzarMinijuegosDueloParaPruebas(
    const IdMinijuego ids[],
    int cantidad
)
{
    cantidadPoolDueloPruebas = 0;

    for (int i = 0; i < cantidad && i < CANTIDAD_MINIJUEGOS; i++)
    {
        poolDueloPruebas[cantidadPoolDueloPruebas++] = ids[i];
    }
}


int ObtenerMinijuegosDuelo(
    IdMinijuego salida[],
    int capacidad
)
{
    int cantidad = 0;

    if (cantidadPoolDueloPruebas > 0)
    {
        for (int i = 0; i < cantidadPoolDueloPruebas && i < capacidad; i++)
        {
            salida[cantidad++] = poolDueloPruebas[i];
        }

        return cantidad;
    }

    for (int i = 0; i < CANTIDAD_MINIJUEGOS && cantidad < capacidad; i++)
    {
        const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuegoPorIndice(i);

        if (datos.esDuelo && MinijuegoAdmiteCantidadJugadores(datos.id, 2))
        {
            salida[cantidad] = datos.id;
            cantidad++;
        }
    }

    return cantidad;
}


bool EsMinijuegoDuelo(
    IdMinijuego id
)
{
    return EsIdMinijuegoValido(id) && ObtenerDatosMinijuego(id).esDuelo;
}
