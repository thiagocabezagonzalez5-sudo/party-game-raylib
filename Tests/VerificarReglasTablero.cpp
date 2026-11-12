// Prueba de las reglas del tablero que mueven monedas y trofeos (sin
// pantalla): duelo, CAOS, INTERCAMBIO, escudo (incluido el alquiler del
// Mercado Flotante) y premios de ronda. Reglas oficiales en
// Docs/ReglasTableros.md. Usa azar con semilla fija y recorre semillas.
#include "Gameplay/PartidaTablero.h"

#include <cstdarg>
#include <cstdio>

static int errores = 0;

static void Comprobar(bool condicion, const char* formato, ...)
{
    if (!condicion)
    {
        char texto[256];
        va_list argumentos;
        va_start(argumentos, formato);
        std::vsnprintf(texto, sizeof(texto), formato, argumentos);
        va_end(argumentos);
        std::fprintf(stderr, "FALLO: %s\n", texto);
        errores++;
    }
}

static void RegistroSilencioso(int nivel, const char* formato, va_list argumentos)
{
    if (nivel < LOG_WARNING) return;
    char texto[512];
    std::vsnprintf(texto, sizeof(texto), formato, argumentos);
    std::fprintf(stderr, "%s\n", texto);
}

// Objetos grandes fuera de la pila.
static Participante participantes[MAX_PARTICIPANTES];
static PartidaTablero partida;

static void PrepararPartida(int cantidad, IdTablero tablero)
{
    const Color colores[] = { RED, BLUE, GREEN, YELLOW };

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i] = {};
        participantes[i].activo = i < cantidad;
        participantes[i].esBot = true;
        participantes[i].conectado = true;
        participantes[i].numeroJugador = i + 1;
        participantes[i].color = colores[i];
    }

    partida = PartidaTablero{};
    partida.Inicializar(participantes, cantidad, nullptr, tablero);
}

static void VaciarInventario(int jugador)
{
    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        partida.jugadores[jugador].inventario[i] = OBJETO_VACIO;
    }
}

static void DarEscudo(int jugador)
{
    VaciarInventario(jugador);
    AgregarObjetoInventario(partida.jugadores[jugador].inventario, OBJETO_ESCUDO);
}

static bool TieneEscudo(int jugador)
{
    return TieneObjetoInventario(partida.jugadores[jugador].inventario, OBJETO_ESCUDO);
}

static void FijarJugador(int jugador, int monedas, int trofeos)
{
    partida.jugadores[jugador].monedas = monedas;
    partida.jugadores[jugador].trofeos = trofeos;
    VaciarInventario(jugador);
}

static bool NadieNegativo()
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (partida.jugadores[i].monedas < 0 || partida.jugadores[i].trofeos < 0)
        {
            return false;
        }
    }

    return true;
}

// Resultado individual con un ganador entre los activos (ganador = -1:
// empate, todos segundos; los indices en `fuera` no participaron).
static ResultadoMinijuego ResultadoCon(int ganador, int fuera = -1)
{
    ResultadoMinijuego resultado;
    InicializarResultadoMinijuego(resultado, participantes, FORMATO_MINIJUEGO_INDIVIDUAL);
    resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    resultado.desenlace = ganador >= 0 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;
        if (i == fuera)
        {
            resultado.participantes[i].participo = false;
            resultado.cantidadParticipantes--;
            continue;
        }
        resultado.participantes[i].posicionFinal = i == ganador ? 1 : 2;
    }

    return resultado;
}


//==================================================
// 1. DUELO
//==================================================

static void ArmarDuelo(int retador, int rival, TipoApuestaDuelo tipo, int cantidad)
{
    OpcionApuestaDuelo apuesta;
    apuesta.tipo = tipo;
    apuesta.cantidad = cantidad;
    IniciarDueloTablero(partida, retador, rival, apuesta);
}

static void ProbarDuelo()
{
    PrepararPartida(2, TABLERO_ISLA_ARBOLEDA);

    // Opciones: el tope del mas pobre limita la apuesta.
    {
        FijarJugador(0, 30, 0);
        FijarJugador(1, 7, 0);
        OpcionApuestaDuelo opciones[MAX_OPCIONES_APUESTA_DUELO];
        int n = ListarOpcionesApuestaDuelo(partida, 0, 1, opciones);
        bool todasDentro = true;
        for (int i = 0; i < n; i++)
        {
            if (opciones[i].tipo == APUESTA_DUELO_TROFEO) todasDentro = false;
            if (opciones[i].tipo == APUESTA_DUELO_MONEDAS && opciones[i].cantidad > 7) todasDentro = false;
        }
        Comprobar(n == 2 && todasDentro, "Duelo: con 30 vs 7 monedas y sin trofeos las opciones son 5 y 7 (hay %d)", n);

        FijarJugador(1, 0, 0);
        Comprobar(ListarOpcionesApuestaDuelo(partida, 0, 1, opciones) == 0, "Duelo: sin monedas ni trofeos no hay apuesta");
    }

    // Monedas: el perdedor tiene MENOS que la apuesta -> paga solo lo que tiene.
    // (Puede pasar si pierde monedas entre la apuesta y el pago.)
    {
        FijarJugador(0, 25, 0);
        FijarJugador(1, 4, 0);
        ArmarDuelo(0, 1, APUESTA_DUELO_MONEDAS, 20);
        ResultadoMinijuego r = ResultadoCon(0);
        partida.AplicarResultadoMinijuego(r);
        Comprobar(partida.jugadores[1].monedas == 0, "Duelo monedas: perdedor con 4 apuesta 20 queda en 0 (tiene %d)", partida.jugadores[1].monedas);
        Comprobar(partida.jugadores[0].monedas == 29, "Duelo monedas: ganador cobra solo 4 (25 -> %d, esperado 29)", partida.jugadores[0].monedas);
        Comprobar(partida.duelo.pagado == 4, "Duelo monedas: pagado = %d (esperado 4)", partida.duelo.pagado);
        Comprobar(partida.fase == FASE_PARTIDA_TABLERO_DUELO_RESULTADO, "Duelo: tras el pago la fase es DUELO_RESULTADO");
        Comprobar(partida.jugadores[0].premioUltimoMinijuego == 0 && partida.jugadores[1].premioUltimoMinijuego == 0,
            "Duelo: no entrega premios de ronda");
    }

    // Monedas: pago completo, gana el rival.
    {
        FijarJugador(0, 12, 0);
        FijarJugador(1, 12, 0);
        ArmarDuelo(0, 1, APUESTA_DUELO_MONEDAS, 10);
        ResultadoMinijuego r = ResultadoCon(1);
        partida.AplicarResultadoMinijuego(r);
        Comprobar(partida.jugadores[0].monedas == 2 && partida.jugadores[1].monedas == 22,
            "Duelo monedas: gana el rival 10 (J1=%d J2=%d, esperado 2/22)", partida.jugadores[0].monedas, partida.jugadores[1].monedas);
    }

    // Monedas: el escudo NO protege el duelo y no se consume.
    {
        FijarJugador(0, 10, 0);
        FijarJugador(1, 10, 0);
        DarEscudo(1);
        ArmarDuelo(0, 1, APUESTA_DUELO_MONEDAS, 5);
        ResultadoMinijuego r = ResultadoCon(0);
        partida.AplicarResultadoMinijuego(r);
        Comprobar(partida.jugadores[1].monedas == 5 && TieneEscudo(1), "Duelo: el escudo no protege la apuesta ni se consume");
    }

    // Trofeo: el perdedor tiene trofeo.
    {
        FijarJugador(0, 5, 1);
        FijarJugador(1, 5, 2);
        ArmarDuelo(0, 1, APUESTA_DUELO_TROFEO, 1);
        ResultadoMinijuego r = ResultadoCon(0);
        partida.AplicarResultadoMinijuego(r);
        Comprobar(partida.jugadores[0].trofeos == 2 && partida.jugadores[1].trofeos == 1 && partida.duelo.pagado == 1,
            "Duelo trofeo: pasa 1 trofeo (J1=%d J2=%d)", partida.jugadores[0].trofeos, partida.jugadores[1].trofeos);
        Comprobar(partida.jugadores[0].monedas == 5 && partida.jugadores[1].monedas == 5, "Duelo trofeo: las monedas no cambian");
    }

    // Trofeo: el perdedor ya no tiene trofeo -> nadie paga, nadie negativo.
    {
        FijarJugador(0, 5, 1);
        FijarJugador(1, 5, 0);
        ArmarDuelo(0, 1, APUESTA_DUELO_TROFEO, 1);
        ResultadoMinijuego r = ResultadoCon(0);
        partida.AplicarResultadoMinijuego(r);
        Comprobar(partida.jugadores[0].trofeos == 1 && partida.jugadores[1].trofeos == 0 && partida.duelo.pagado == 0,
            "Duelo trofeo sin trofeo: no se mueve nada (J1=%d J2=%d pagado=%d)",
            partida.jugadores[0].trofeos, partida.jugadores[1].trofeos, partida.duelo.pagado);
    }

    // Empate: nadie paga.
    {
        FijarJugador(0, 15, 1);
        FijarJugador(1, 15, 1);
        ArmarDuelo(0, 1, APUESTA_DUELO_MONEDAS, 10);
        ResultadoMinijuego r = ResultadoCon(-1);
        partida.AplicarResultadoMinijuego(r);
        Comprobar(partida.duelo.empate && partida.jugadores[0].monedas == 15 && partida.jugadores[1].monedas == 15,
            "Duelo empate (ninguno primero): nadie paga");

        // Empate con los dos primeros.
        ArmarDuelo(0, 1, APUESTA_DUELO_TROFEO, 1);
        ResultadoMinijuego r2 = ResultadoCon(0);
        r2.participantes[1].posicionFinal = 1;
        partida.AplicarResultadoMinijuego(r2);
        Comprobar(partida.duelo.empate && partida.jugadores[0].trofeos == 1 && partida.jugadores[1].trofeos == 1,
            "Duelo empate (ambos primeros): nadie paga el trofeo");
    }

    // Ronda cancelada: ContinuarTrasMinijuego sin resultado (nullptr).
    {
        FijarJugador(0, 15, 1);
        FijarJugador(1, 15, 1);
        ArmarDuelo(0, 1, APUESTA_DUELO_MONEDAS, 10);
        partida.ContinuarTrasMinijuego();
        Comprobar(partida.duelo.cancelado && partida.duelo.pagado == 0 &&
            partida.jugadores[0].monedas == 15 && partida.jugadores[1].monedas == 15,
            "Duelo cancelado (sin resultado): nadie paga");
        Comprobar(partida.fase == FASE_PARTIDA_TABLERO_DUELO_RESULTADO, "Duelo cancelado: fase DUELO_RESULTADO");

        // Directo con nullptr y apuesta de trofeo.
        ArmarDuelo(0, 1, APUESTA_DUELO_TROFEO, 1);
        ResolverDueloTablero(partida, nullptr);
        Comprobar(partida.duelo.cancelado && partida.jugadores[0].trofeos == 1 && partida.jugadores[1].trofeos == 1,
            "Duelo cancelado (nullptr, trofeo): nadie paga");

        // Resultado con estado CANCELADO.
        ArmarDuelo(0, 1, APUESTA_DUELO_MONEDAS, 10);
        ResultadoMinijuego r = ResultadoCon(0);
        r.estado = RESULTADO_MINIJUEGO_CANCELADO;
        partida.AplicarResultadoMinijuego(r);
        Comprobar(partida.duelo.cancelado && partida.jugadores[0].monedas == 15 && partida.jugadores[1].monedas == 15,
            "Duelo con resultado CANCELADO: nadie paga");
    }

    // Barrido: cualquier apuesta listada y cualquier desenlace conserva el
    // total y no deja negativos.
    for (int semilla = 1; semilla <= 400; semilla++)
    {
        SetRandomSeed(semilla);
        PrepararPartida(GetRandomValue(2, 4), TABLERO_ISLA_ARBOLEDA);
        int a = partida.ordenParticipantes[0];
        int b = partida.ordenParticipantes[1];
        for (int i = 0; i < partida.cantidadJugadores; i++)
        {
            FijarJugador(partida.ordenParticipantes[i], GetRandomValue(0, 30), GetRandomValue(0, 2));
        }
        OpcionApuestaDuelo opciones[MAX_OPCIONES_APUESTA_DUELO];
        int n = ListarOpcionesApuestaDuelo(partida, a, b, opciones);
        if (n == 0) continue;
        OpcionApuestaDuelo elegida = opciones[GetRandomValue(0, n - 1)];
        // A veces el perdedor pierde monedas antes del pago.
        if (GetRandomValue(0, 3) == 0) partida.jugadores[b].monedas = GetRandomValue(0, 3);
        int sumaM = partida.jugadores[a].monedas + partida.jugadores[b].monedas;
        int sumaT = partida.jugadores[a].trofeos + partida.jugadores[b].trofeos;
        IniciarDueloTablero(partida, a, b, elegida);
        int caso = GetRandomValue(0, 3);
        if (caso == 3)
        {
            ResolverDueloTablero(partida, nullptr);
        }
        else
        {
            ResultadoMinijuego r = ResultadoCon(caso == 0 ? a : (caso == 1 ? b : -1));
            ResolverDueloTablero(partida, &r);
        }
        Comprobar(NadieNegativo(), "Duelo barrido semilla %d: queda un negativo", semilla);
        Comprobar(partida.jugadores[a].monedas + partida.jugadores[b].monedas == sumaM &&
            partida.jugadores[a].trofeos + partida.jugadores[b].trofeos == sumaT,
            "Duelo barrido semilla %d: el total no se conserva", semilla);
    }
}


//==================================================
// 2. CAOS e INTERCAMBIO
//==================================================

static void ProbarCaos()
{
    PrepararPartida(4, TABLERO_ISLA_ARBOLEDA);
    char resumen[112];

    // Casos fijos.
    FijarJugador(0, 7, 0);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_MONEDAS, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].monedas == 0, "CAOS -20 con 7 monedas: queda en 0 (tiene %d)", partida.jugadores[0].monedas);

    FijarJugador(0, 4, 0);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_POCAS, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].monedas == 0, "CAOS -10 con 4 monedas: queda en 0");

    FijarJugador(0, 9, 0);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_TODAS, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].monedas == 0, "CAOS pierde todas: queda en 0");

    FijarJugador(0, 3, 0);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_TROFEO, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].trofeos == 0 && partida.jugadores[0].monedas == 3, "CAOS pierde trofeo sin trofeos: nada cambia");

    FijarJugador(0, 3, 2);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_TROFEO, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].trofeos == 1, "CAOS pierde trofeo: 2 -> %d", partida.jugadores[0].trofeos);

    // Caos global con pobres: nadie baja de 0.
    int monedasIniciales[4] = { 0, 5, 20, 33 };
    for (int i = 0; i < 4; i++) FijarJugador(i, monedasIniciales[i], 0);
    AplicarOpcionCaos(partida, 1, CAOS_PARA_TODOS, resumen, sizeof(resumen));
    int esperadas[4] = { 0, 0, 0, 13 };
    for (int i = 0; i < 4; i++)
    {
        Comprobar(partida.jugadores[i].monedas == esperadas[i], "CAOS global: J%d %d -> %d (esperado %d)",
            i + 1, monedasIniciales[i], partida.jugadores[i].monedas, esperadas[i]);
    }

    // Sin trofeos nunca sale la de trofeo; sin nada solo el caos global.
    bool vistas[CANTIDAD_OPCIONES_CAOS] = {};
    for (int semilla = 1; semilla <= 1500; semilla++)
    {
        SetRandomSeed(semilla);
        int cantidad = GetRandomValue(2, 4);
        PrepararPartida(cantidad, TABLERO_ISLA_ARBOLEDA);
        int antes[MAX_PARTICIPANTES] = {};
        int trofeosAntes[MAX_PARTICIPANTES] = {};
        bool escudo[MAX_PARTICIPANTES] = {};
        for (int i = 0; i < cantidad; i++)
        {
            int j = partida.ordenParticipantes[i];
            FijarJugador(j, GetRandomValue(0, 3) == 0 ? 0 : GetRandomValue(0, 40), GetRandomValue(0, 2));
            if (GetRandomValue(0, 4) == 0) { DarEscudo(j); escudo[j] = true; }
            antes[j] = partida.jugadores[j].monedas;
            trofeosAntes[j] = partida.jugadores[j].trofeos;
        }
        int quien = partida.ordenParticipantes[GetRandomValue(0, cantidad - 1)];
        OpcionCaos opcion = ElegirOpcionCaos(partida, quien);
        vistas[opcion] = true;

        if (trofeosAntes[quien] == 0)
        {
            Comprobar(opcion != CAOS_PIERDE_TROFEO, "CAOS semilla %d: sale 'pierde trofeo' sin trofeos", semilla);
        }
        else
        {
            Comprobar(opcion != CAOS_PIERDE_POCAS, "CAOS semilla %d: sale la sustituta teniendo trofeos", semilla);
        }
        if (trofeosAntes[quien] == 0 && antes[quien] == 0)
        {
            Comprobar(opcion == CAOS_PARA_TODOS, "CAOS semilla %d: sin nada debe salir el caos global", semilla);
        }

        AplicarOpcionCaos(partida, quien, opcion, resumen, sizeof(resumen));
        Comprobar(NadieNegativo(), "CAOS semilla %d opcion %d: queda un negativo", semilla, (int)opcion);

        for (int i = 0; i < cantidad; i++)
        {
            int j = partida.ordenParticipantes[i];
            const EstadoJugadorPartidaTablero& e = partida.jugadores[j];
            Comprobar(e.monedas <= antes[j] && e.trofeos <= trofeosAntes[j], "CAOS semilla %d: J%d gana algo", semilla, j + 1);
            if (opcion == CAOS_PARA_TODOS)
            {
                int perdida = antes[j] < CAOS_MONEDAS_GLOBAL ? antes[j] : CAOS_MONEDAS_GLOBAL;
                int esperado = (escudo[j] && perdida > 0) ? antes[j] : antes[j] - perdida;
                Comprobar(e.monedas == esperado, "CAOS global semilla %d: J%d %d -> %d (esperado %d)",
                    semilla, j + 1, antes[j], e.monedas, esperado);
                Comprobar(TieneEscudo(j) == (escudo[j] && perdida == 0), "CAOS global semilla %d: escudo de J%d mal consumido", semilla, j + 1);
            }
            else if (j != quien)
            {
                Comprobar(e.monedas == antes[j] && e.trofeos == trofeosAntes[j] && TieneEscudo(j) == escudo[j],
                    "CAOS semilla %d: afecta a J%d que no cayo", semilla, j + 1);
            }
        }
    }
    for (int o = 0; o < CANTIDAD_OPCIONES_CAOS; o++)
    {
        Comprobar(vistas[o], "CAOS: la opcion %d nunca salio en el barrido", o);
    }
}

static void ProbarIntercambio()
{
    // Casos fijos con pobres.
    PrepararPartida(2, TABLERO_ISLA_ARBOLEDA);
    int a = partida.ordenParticipantes[0];
    int b = partida.ordenParticipantes[1];

    EventoInteractivo ev;
    ev.personaA = a; ev.personaB = b; ev.recurso = RECURSO_INTERCAMBIO_MONEDAS;
    ev.accion = INTERCAMBIO_DA; ev.cantidad = 15;
    FijarJugador(a, 3, 0); FijarJugador(b, 0, 0);
    AplicarIntercambio(partida, ev);
    Comprobar(partida.jugadores[a].monedas == 0 && partida.jugadores[b].monedas == 3, "Intercambio DA 15 con 3: da solo 3");

    ev = EventoInteractivo{};
    ev.personaA = a; ev.personaB = b; ev.recurso = RECURSO_INTERCAMBIO_MONEDAS;
    ev.accion = INTERCAMBIO_RECIBE; ev.cantidad = 12;
    FijarJugador(a, 0, 0); FijarJugador(b, 0, 0);
    AplicarIntercambio(partida, ev);
    Comprobar(partida.jugadores[a].monedas == 0 && partida.jugadores[b].monedas == 0 && ev.movido == 0, "Intercambio RECIBE de quien no tiene: nada");

    ev = EventoInteractivo{};
    ev.personaA = a; ev.personaB = b; ev.recurso = RECURSO_INTERCAMBIO_TROFEOS;
    ev.accion = INTERCAMBIO_DA; ev.cantidad = 1;
    FijarJugador(a, 5, 0); FijarJugador(b, 5, 1);
    AplicarIntercambio(partida, ev);
    Comprobar(partida.jugadores[a].trofeos == 0 && partida.jugadores[b].trofeos == 1, "Intercambio DA trofeo sin trofeo: nada");

    ev = EventoInteractivo{};
    ev.personaA = a; ev.personaB = b; ev.recurso = RECURSO_INTERCAMBIO_MONEDAS;
    ev.accion = INTERCAMBIO_CAMBIA;
    FijarJugador(a, 2, 0); FijarJugador(b, 31, 0);
    AplicarIntercambio(partida, ev);
    Comprobar(partida.jugadores[a].monedas == 31 && partida.jugadores[b].monedas == 2, "Intercambio CAMBIA: permuta 2 y 31");

    // Escudo bloquea la entrega y se consume.
    ev = EventoInteractivo{};
    ev.personaA = a; ev.personaB = b; ev.recurso = RECURSO_INTERCAMBIO_MONEDAS;
    ev.accion = INTERCAMBIO_DA; ev.cantidad = 8;
    FijarJugador(a, 10, 0); FijarJugador(b, 0, 0); DarEscudo(a);
    AplicarIntercambio(partida, ev);
    Comprobar(partida.jugadores[a].monedas == 10 && partida.jugadores[b].monedas == 0 && !TieneEscudo(a),
        "Intercambio: el escudo de quien da bloquea y se consume");

    // Barrido al azar.
    bool vistas[3][2] = {};
    for (int semilla = 1; semilla <= 2000; semilla++)
    {
        SetRandomSeed(semilla);
        int cantidad = GetRandomValue(2, 4);
        PrepararPartida(cantidad, TABLERO_ISLA_ARBOLEDA);
        int antesM[MAX_PARTICIPANTES] = {};
        int antesT[MAX_PARTICIPANTES] = {};
        int sumaM = 0, sumaT = 0;
        for (int i = 0; i < cantidad; i++)
        {
            int j = partida.ordenParticipantes[i];
            FijarJugador(j, GetRandomValue(0, 2) == 0 ? GetRandomValue(0, 4) : GetRandomValue(0, 40), GetRandomValue(0, 2));
            if (GetRandomValue(0, 4) == 0) DarEscudo(j);
            antesM[j] = partida.jugadores[j].monedas;
            antesT[j] = partida.jugadores[j].trofeos;
            sumaM += antesM[j];
            sumaT += antesT[j];
        }

        EventoInteractivo e;
        SortearIntercambio(partida, e);
        Comprobar(e.personaA >= 0 && e.personaB >= 0 && e.personaA != e.personaB &&
            partida.jugadores[e.personaA].participa && partida.jugadores[e.personaB].participa,
            "Intercambio semilla %d: personas invalidas (%d, %d)", semilla, e.personaA, e.personaB);
        if (e.personaA < 0 || e.personaB < 0) continue;
        if (e.recurso == RECURSO_INTERCAMBIO_MONEDAS)
        {
            Comprobar(e.cantidad >= INTERCAMBIO_MONEDAS_MIN && e.cantidad <= INTERCAMBIO_MONEDAS_MAX,
                "Intercambio semilla %d: cantidad de monedas %d fuera de 5..15", semilla, e.cantidad);
        }
        else
        {
            Comprobar(e.cantidad == 1, "Intercambio semilla %d: cantidad de trofeos %d", semilla, e.cantidad);
            int tA = antesT[e.personaA], tB = antesT[e.personaB];
            bool mueve = e.accion == INTERCAMBIO_DA ? tA > 0 : (e.accion == INTERCAMBIO_RECIBE ? tB > 0 : tA != tB);
            Comprobar(mueve, "Intercambio semilla %d: sale trofeos sin efecto (deberia caer en monedas)", semilla);
        }
        vistas[e.accion][e.recurso] = true;

        bool escudoA = TieneEscudo(e.personaA);
        bool escudoB = TieneEscudo(e.personaB);
        AplicarIntercambio(partida, e);

        Comprobar(NadieNegativo(), "Intercambio semilla %d (accion %d recurso %d cantidad %d): queda un negativo",
            semilla, (int)e.accion, (int)e.recurso, e.cantidad);

        int nuevaM = 0, nuevaT = 0;
        for (int i = 0; i < cantidad; i++)
        {
            int j = partida.ordenParticipantes[i];
            nuevaM += partida.jugadores[j].monedas;
            nuevaT += partida.jugadores[j].trofeos;
            if (j != e.personaA && j != e.personaB)
            {
                Comprobar(partida.jugadores[j].monedas == antesM[j] && partida.jugadores[j].trofeos == antesT[j],
                    "Intercambio semilla %d: afecta a un tercero J%d", semilla, j + 1);
            }
        }
        Comprobar(nuevaM == sumaM && nuevaT == sumaT, "Intercambio semilla %d: el total no se conserva", semilla);

        int* antes = e.recurso == RECURSO_INTERCAMBIO_MONEDAS ? antesM : antesT;
        int ahoraA = e.recurso == RECURSO_INTERCAMBIO_MONEDAS ? partida.jugadores[e.personaA].monedas : partida.jugadores[e.personaA].trofeos;
        int ahoraB = e.recurso == RECURSO_INTERCAMBIO_MONEDAS ? partida.jugadores[e.personaB].monedas : partida.jugadores[e.personaB].trofeos;
        if (e.accion == INTERCAMBIO_CAMBIA)
        {
            Comprobar(ahoraA == antes[e.personaB] && ahoraB == antes[e.personaA], "Intercambio CAMBIA semilla %d: no permuta", semilla);
            Comprobar(TieneEscudo(e.personaA) == escudoA && TieneEscudo(e.personaB) == escudoB,
                "Intercambio CAMBIA semilla %d: consume un escudo", semilla);
        }
        else
        {
            int origen = e.accion == INTERCAMBIO_DA ? e.personaA : e.personaB;
            bool escudoOrigen = origen == e.personaA ? escudoA : escudoB;
            int disponible = antes[origen];
            int pedido = e.cantidad < disponible ? e.cantidad : disponible;
            int esperado = (escudoOrigen && pedido > 0) ? 0 : pedido;
            int ahoraOrigen = origen == e.personaA ? ahoraA : ahoraB;
            Comprobar(antes[origen] - ahoraOrigen == esperado, "Intercambio semilla %d: se movio %d (esperado %d)",
                semilla, antes[origen] - ahoraOrigen, esperado);
        }
    }
    for (int acc = 0; acc < 3; acc++)
    {
        for (int rec = 0; rec < 2; rec++)
        {
            Comprobar(vistas[acc][rec], "Intercambio: la combinacion accion %d recurso %d nunca salio", acc, rec);
        }
    }
}


//==================================================
// 3. ESCUDO
//==================================================

static void ProbarEscudo()
{
    PrepararPartida(3, TABLERO_ISLA_ARBOLEDA);
    char resumen[112];

    // Directo: bloquea una vez y se consume.
    FijarJugador(0, 10, 0);
    DarEscudo(0);
    AgregarObjetoInventario(partida.jugadores[0].inventario, OBJETO_BOTAS);
    Comprobar(EscudoAbsorbePerdida(partida, 0), "Escudo: absorbe la primera perdida");
    Comprobar(!TieneEscudo(0), "Escudo: se consume");
    Comprobar(TieneObjetoInventario(partida.jugadores[0].inventario, OBJETO_BOTAS), "Escudo: no se lleva otros objetos");
    Comprobar(!EscudoAbsorbePerdida(partida, 0), "Escudo: sin escudo no absorbe");
    Comprobar(!EscudoAbsorbePerdida(partida, -1) && !EscudoAbsorbePerdida(partida, MAX_PARTICIPANTES), "Escudo: indice invalido");

    // Dos escudos: uno por perdida.
    VaciarInventario(1);
    AgregarObjetoInventario(partida.jugadores[1].inventario, OBJETO_ESCUDO);
    AgregarObjetoInventario(partida.jugadores[1].inventario, OBJETO_ESCUDO);
    EscudoAbsorbePerdida(partida, 1);
    Comprobar(ContarObjetosInventario(partida.jugadores[1].inventario) == 1 && TieneEscudo(1), "Escudo: con dos se consume uno");

    // A traves de CAOS: bloquea -20 y el trofeo.
    FijarJugador(0, 25, 1);
    DarEscudo(0);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_MONEDAS, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].monedas == 25 && !TieneEscudo(0), "Escudo + CAOS -20: no pierde y se consume");
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_MONEDAS, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].monedas == 5, "Escudo consumido: la segunda perdida se cobra (tiene %d)", partida.jugadores[0].monedas);
    DarEscudo(0);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_TROFEO, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].trofeos == 1 && !TieneEscudo(0), "Escudo + CAOS trofeo: conserva el trofeo");

    // Sin nada que perder el escudo no se gasta.
    FijarJugador(0, 0, 0);
    DarEscudo(0);
    AplicarOpcionCaos(partida, 0, CAOS_PIERDE_TODAS, resumen, sizeof(resumen));
    Comprobar(TieneEscudo(0), "Escudo: CAOS sin monedas no lo gasta");

    // Iman contra quien tiene escudo.
    FijarJugador(0, 10, 0);
    FijarJugador(1, 10, 0);
    DarEscudo(1);
    AgregarObjetoInventario(partida.jugadores[0].inventario, OBJETO_IMAN);
    UsarObjeto(partida, 0, 0, 1, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[0].monedas == 10 && partida.jugadores[1].monedas == 10 && !TieneEscudo(1),
        "Escudo + Iman: bloquea el robo y se consume");
    FijarJugador(1, 3, 0);
    AgregarObjetoInventario(partida.jugadores[0].inventario, OBJETO_IMAN);
    UsarObjeto(partida, 0, 0, 1, resumen, sizeof(resumen));
    Comprobar(partida.jugadores[1].monedas == 0 && partida.jugadores[0].monedas == 13, "Iman con 3 monedas: roba 3, nadie negativo");

    // Mercado Flotante: alquiler del puesto de un rival.
    PrepararPartida(2, TABLERO_MERCADO_FLOTANTE);
    Comprobar(partida.definicion != nullptr && partida.definicion->ResolverCasilla != nullptr, "Mercado: hook ResolverCasilla");
    if (partida.definicion == nullptr || partida.definicion->ResolverCasilla == nullptr) return;

    int puesto = -1;
    for (int i = 0; i < partida.tablero.cantidadCasillas; i++)
    {
        if (partida.tablero.casillas[i].tipo == CASILLA_NEUTRA) { puesto = i; break; }
    }
    Comprobar(puesto >= 0, "Mercado: hay una casilla azul (puesto)");
    if (puesto < 0) return;

    int dueno = 0, inquilino = 1;
    FijarJugador(dueno, 10, 0);
    FijarJugador(inquilino, 10, 0);
    partida.jugadores[dueno].casillaActual = puesto;
    partida.jugadores[inquilino].casillaActual = puesto;

    partida.definicion->ResolverCasilla(partida, dueno);
    Comprobar(partida.jugadores[dueno].monedas == 7, "Mercado: abrir el puesto cuesta 3 (10 -> %d)", partida.jugadores[dueno].monedas);

    DarEscudo(inquilino);
    partida.definicion->ResolverCasilla(partida, inquilino);
    Comprobar(partida.jugadores[inquilino].monedas == 10 && partida.jugadores[dueno].monedas == 7 && !TieneEscudo(inquilino),
        "Mercado + Escudo: evita el alquiler y se consume (inquilino %d, dueno %d)",
        partida.jugadores[inquilino].monedas, partida.jugadores[dueno].monedas);

    partida.definicion->ResolverCasilla(partida, inquilino);
    Comprobar(partida.jugadores[inquilino].monedas == 7 && partida.jugadores[dueno].monedas == 10,
        "Mercado sin escudo: paga 3 de alquiler (inquilino %d, dueno %d)",
        partida.jugadores[inquilino].monedas, partida.jugadores[dueno].monedas);

    partida.jugadores[inquilino].monedas = 1;
    partida.definicion->ResolverCasilla(partida, inquilino);
    Comprobar(partida.jugadores[inquilino].monedas == 0 && partida.jugadores[dueno].monedas == 11,
        "Mercado: con 1 moneda paga 1 y queda en 0");

    DarEscudo(inquilino);
    partida.definicion->ResolverCasilla(partida, inquilino);
    Comprobar(partida.jugadores[inquilino].monedas == 0 && TieneEscudo(inquilino),
        "Mercado: sin monedas no paga ni gasta el escudo");

    partida.definicion->ResolverCasilla(partida, dueno);
    Comprobar(partida.jugadores[dueno].monedas == 13, "Mercado: el puesto propio cobra 2");
}


//==================================================
// 4. PREMIOS DE RONDA
//==================================================

static void ProbarPremiosRonda()
{
    // 3 en la partida (J1..J3); J4 no participa.
    PrepararPartida(3, TABLERO_ISLA_ARBOLEDA);
    for (int i = 0; i < MAX_PARTICIPANTES; i++) partida.jugadores[i].monedas = 10;
    Comprobar(!partida.EsDueloPendiente(), "Premios: no hay duelo pendiente");

    ResultadoMinijuego r = ResultadoCon(1);
    partida.AplicarResultadoMinijuego(r);
    Comprobar(partida.jugadores[1].monedas == 20 && partida.jugadores[1].premioUltimoMinijuego == 10, "Premios: ganador +10 (tiene %d)", partida.jugadores[1].monedas);
    Comprobar(partida.jugadores[0].monedas == 13 && partida.jugadores[0].premioUltimoMinijuego == 3, "Premios: participante J1 +3 (tiene %d)", partida.jugadores[0].monedas);
    Comprobar(partida.jugadores[2].monedas == 13 && partida.jugadores[2].premioUltimoMinijuego == 3, "Premios: participante J3 +3 (tiene %d)", partida.jugadores[2].monedas);
    Comprobar(partida.jugadores[3].monedas == 10 && partida.jugadores[3].premioUltimoMinijuego == 0, "Premios: no participante J4 0 (tiene %d)", partida.jugadores[3].monedas);

    // Empate: todos participacion.
    for (int i = 0; i < MAX_PARTICIPANTES; i++) partida.jugadores[i].monedas = 0;
    ResultadoMinijuego empate = ResultadoCon(-1);
    partida.AplicarResultadoMinijuego(empate);
    Comprobar(partida.jugadores[0].monedas == 3 && partida.jugadores[1].monedas == 3 && partida.jugadores[2].monedas == 3 && partida.jugadores[3].monedas == 0,
        "Premios con empate sin primero: +3 a cada participante");

    // Dos ganadores (empate en primer puesto): +10 a ambos.
    for (int i = 0; i < MAX_PARTICIPANTES; i++) partida.jugadores[i].monedas = 0;
    ResultadoMinijuego dos = ResultadoCon(0);
    dos.participantes[2].posicionFinal = 1;
    partida.AplicarResultadoMinijuego(dos);
    Comprobar(partida.jugadores[0].monedas == 10 && partida.jugadores[2].monedas == 10 && partida.jugadores[1].monedas == 3,
        "Premios con dos primeros: +10 a ambos, +3 al resto");

    // Con 4 jugadores, cada uno ganador por turno.
    PrepararPartida(4, TABLERO_FORJA_VOLCAN);
    for (int g = 0; g < 4; g++)
    {
        int antes[MAX_PARTICIPANTES];
        for (int i = 0; i < MAX_PARTICIPANTES; i++) antes[i] = partida.jugadores[i].monedas;
        ResultadoMinijuego rg = ResultadoCon(g);
        partida.AplicarResultadoMinijuego(rg);
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            int esperado = i == g ? 10 : 3;
            Comprobar(partida.jugadores[i].monedas - antes[i] == esperado, "Premios 4J: J%d recibe %d (esperado %d)",
                i + 1, partida.jugadores[i].monedas - antes[i], esperado);
        }
    }
}


int main()
{
    SetTraceLogCallback(RegistroSilencioso);

    ProbarDuelo();
    ProbarCaos();
    ProbarIntercambio();
    ProbarEscudo();
    ProbarPremiosRonda();

    std::printf("Verificacion Reglas del Tablero: %d errores\n", errores);
    return errores == 0 ? 0 : 1;
}
