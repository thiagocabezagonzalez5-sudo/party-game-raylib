// Simula partidas COMPLETAS de tablero sin ventana, con todos los jugadores
// bots, para cada tablero del catalogo (2, 3 y 4 jugadores, varias semillas).
// Los minijuegos y duelos se reemplazan por un resultado falso valido, igual
// que el flujo de Core/Juego.cpp (ganador al azar; a veces empate o ronda
// cancelada). Comprueba que la partida termina, la regla de casillas
// (Docs/ReglasTableros.md), las conexiones y que no haya monedas ni trofeos
// negativos. Uso: SimularTableros.exe [--tablero N|nombre] [--semillas N]
#include "Board/CatalogoTableros.h"
#include "Core/ResultadoMinijuego.h"
#include "Gameplay/PartidaTablero.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>


//==================================================
// PARAMETROS
//==================================================

static const float DT_SIMULACION = 1.0f / 30.0f;
static const int LIMITE_PASOS = 200000;

// Una fase que no cambia en este tiempo simulado se considera trabada.
static const float LIMITE_SEGUNDOS_MISMA_FASE = 600.0f;

static const int PORCENTAJE_RONDA_CANCELADA = 5;
static const int PORCENTAJE_EMPATE_DUELO = 15;

static const int MAX_FALLAS_IMPRESAS = 6;


static const char* NombreFase(FasePartidaTablero fase)
{
    switch (fase)
    {
        case FASE_PARTIDA_TABLERO_ESPERANDO_DADO: return "ESPERANDO_DADO";
        case FASE_PARTIDA_TABLERO_MOSTRANDO_DADO: return "MOSTRANDO_DADO";
        case FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA: return "ELIGIENDO_RUTA";
        case FASE_PARTIDA_TABLERO_MOVIENDO: return "MOVIENDO";
        case FASE_PARTIDA_TABLERO_DECISION_TROFEO: return "DECISION_TROFEO";
        case FASE_PARTIDA_TABLERO_EVENTO_CASILLA: return "EVENTO_CASILLA";
        case FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO: return "EVENTO_INTERACTIVO";
        case FASE_PARTIDA_TABLERO_MENU_OBJETOS: return "MENU_OBJETOS";
        case FASE_PARTIDA_TABLERO_DUELO_PENDIENTE: return "DUELO_PENDIENTE";
        case FASE_PARTIDA_TABLERO_DUELO_RESULTADO: return "DUELO_RESULTADO";
        case FASE_PARTIDA_TABLERO_FIN_TURNO: return "FIN_TURNO";
        case FASE_PARTIDA_TABLERO_FIN_RONDA: return "FIN_RONDA";
        case FASE_PARTIDA_TABLERO_TERMINADA: return "TERMINADA";
    }

    return "DESCONOCIDA";
}


// Solo se usa si algun participante no fuera bot: confirma siempre.
static InputSeleccionParticipante EntradaConfirmar(const Participante&)
{
    InputSeleccionParticipante entrada{};
    entrada.confirmar = true;
    return entrada;
}


//==================================================
// ESTADISTICAS Y FALLAS
//==================================================

struct EstadisticasTablero
{
    int partidas = 0;
    int partidasTerminadas = 0;
    int rondas = 0;
    long long pasos = 0;
    int trofeosComprados = 0;
    int minijuegos = 0;
    int duelos = 0;
    int cancelados = 0;
    int visitas[CANTIDAD_TIPOS_CASILLA] = {};
    int casillasPorTipo[CANTIDAD_TIPOS_CASILLA] = {};

    int fallas = 0;
};


static void Fallar(EstadisticasTablero& e, const char* formato, ...)
    __attribute__((format(printf, 2, 3)));

static void Fallar(EstadisticasTablero& e, const char* formato, ...)
{
    if (e.fallas < MAX_FALLAS_IMPRESAS)
    {
        char texto[320];
        va_list args;
        va_start(args, formato);
        std::vsnprintf(texto, sizeof(texto), formato, args);
        va_end(args);
        std::printf("        - %s\n", texto);
    }
    else if (e.fallas == MAX_FALLAS_IMPRESAS)
    {
        std::printf("        - (mas fallas omitidas)\n");
    }

    e.fallas++;
}


// Cada casilla con 1 o 2 conexiones a casillas existentes. Devuelve la
// primera casilla que incumple (o -1) y describe el problema.
static int BuscarConexionInvalida(const Tablero& tablero, char* detalle, int capacidad)
{
    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        const Casilla& casilla = tablero.casillas[i];

        if (casilla.cantidadConexiones < 1 || casilla.cantidadConexiones > MAX_CONEXIONES_CASILLA)
        {
            std::snprintf(detalle, capacidad, "casilla %d tiene %d conexiones", i, casilla.cantidadConexiones);
            return i;
        }

        for (int c = 0; c < casilla.cantidadConexiones; c++)
        {
            int destino = casilla.conexiones[c].destino;

            if (destino < 0 || destino >= tablero.cantidadCasillas || destino == i)
            {
                std::snprintf(detalle, capacidad, "casilla %d: conexion %d con destino invalido %d", i, c, destino);
                return i;
            }
        }
    }

    return -1;
}


//==================================================
// VALIDACION ESTATICA DEL TABLERO
//==================================================

static bool ValidarDefinicion(const DefinicionTablero& def, EstadisticasTablero& e)
{
    if (def.Construir == nullptr)
    {
        Fallar(e, "la definicion no tiene Construir");
        return false;
    }

    static Tablero tablero;
    tablero = Tablero{};
    def.Construir(tablero);

    bool jugable = true;

    if (!tablero.recorridoValido)
    {
        Fallar(e, "tablero.recorridoValido es false tras Construir (%d casillas)", tablero.cantidadCasillas);
        jugable = false;
    }

    char detalle[160];

    if (BuscarConexionInvalida(tablero, detalle, (int)sizeof(detalle)) >= 0)
    {
        Fallar(e, "conexiones: %s", detalle);
        jugable = false;
    }

    TipoCasilla faltantes[CANTIDAD_TIPOS_CASILLA] = {};
    int cantidadFaltantes =
        ContarCasillasObligatoriasFaltantes(def, tablero, faltantes, CANTIDAD_TIPOS_CASILLA);

    if (cantidadFaltantes > 0)
    {
        char lista[200] = "";

        for (int i = 0; i < cantidadFaltantes && i < CANTIDAD_TIPOS_CASILLA; i++)
        {
            std::strncat(lista, i > 0 ? ", " : "", sizeof(lista) - std::strlen(lista) - 1);
            std::strncat(lista, ObtenerNombreTipoCasilla(faltantes[i]), sizeof(lista) - std::strlen(lista) - 1);
        }

        Fallar(e, "regla de casillas: faltan %d tipo(s): %s", cantidadFaltantes, lista);
    }

    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        TipoCasilla tipo = tablero.casillas[i].tipo;

        if (tipo >= 0 && tipo < CANTIDAD_TIPOS_CASILLA)
        {
            e.casillasPorTipo[tipo]++;
        }
    }

    bool tieneEvento = e.casillasPorTipo[CASILLA_EVENTO] > 0;

    if (tieneEvento && def.ResolverCasilla == nullptr)
    {
        Fallar(e, "tiene casillas EVENTO pero no define ResolverCasilla");
    }

    if (def.casillasTrofeo == nullptr || def.cantidadCasillasTrofeo <= 0)
    {
        Fallar(e, "no define casillas candidatas de trofeo");
    }

    for (int i = 0; i < def.cantidadCasillasTrofeo && def.casillasTrofeo != nullptr; i++)
    {
        int casilla = def.casillasTrofeo[i];

        if (casilla < 0 || casilla >= tablero.cantidadCasillas)
        {
            Fallar(e, "casilla de trofeo invalida: %d (hay %d casillas)", casilla, tablero.cantidadCasillas);
        }
    }

    if (def.cantidadRondas <= 0)
    {
        Fallar(e, "cantidadRondas = %d", def.cantidadRondas);
        jugable = false;
    }

    return jugable;
}


//==================================================
// MINIJUEGO FALSO
//==================================================

// Replica Core/Juego.cpp: en un duelo solo juegan retador y rival; el
// resultado se valida, se aplica y despues se continua.
static void ResolverMinijuegoFalso(
    PartidaTablero& partida,
    const Participante participantes[],
    EstadisticasTablero& e,
    const char* contexto
)
{
    bool duelo = partida.EsDueloPendiente();
    Participante jugando[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugando[i] = participantes[i];

        if (duelo)
        {
            jugando[i].activo =
                participantes[i].activo &&
                (i == partida.duelo.retador || i == partida.duelo.rival);
        }
    }

    if (duelo)
    {
        e.duelos++;

        if (
            partida.duelo.retador < 0 || partida.duelo.rival < 0 ||
            partida.duelo.retador == partida.duelo.rival ||
            !jugando[partida.duelo.retador].activo || !jugando[partida.duelo.rival].activo
        )
        {
            Fallar(e, "%s: duelo pendiente con retador %d y rival %d invalidos",
                contexto, partida.duelo.retador, partida.duelo.rival);
        }
    }
    else
    {
        e.minijuegos++;
    }

    // Ronda cancelada o sin resultado: Juego solo continua.
    if (GetRandomValue(1, 100) <= PORCENTAJE_RONDA_CANCELADA)
    {
        e.cancelados++;
        partida.ContinuarTrasMinijuego();
        return;
    }

    ResultadoMinijuego resultado;
    InicializarResultadoMinijuego(resultado, jugando, FORMATO_MINIJUEGO_INDIVIDUAL);
    resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;

    int activos[MAX_PARTICIPANTES];
    int cantidad = ObtenerIndicesParticipantesActivos(jugando, activos, MAX_PARTICIPANTES);

    if (duelo && cantidad >= 2 && GetRandomValue(1, 100) <= PORCENTAJE_EMPATE_DUELO)
    {
        resultado.desenlace = DESENLACE_EMPATE;

        for (int i = 0; i < cantidad; i++)
        {
            resultado.participantes[activos[i]].posicionFinal = 1;
        }
    }
    else if (cantidad > 0)
    {
        resultado.desenlace = DESENLACE_CON_GANADOR;
        int ganador = activos[GetRandomValue(0, cantidad - 1)];

        for (int i = 0; i < cantidad; i++)
        {
            resultado.participantes[activos[i]].posicionFinal = activos[i] == ganador ? 1 : 2;
        }
    }

    if (ValidarResultadoMinijuego(resultado, jugando))
    {
        partida.AplicarResultadoMinijuego(resultado);
    }
    else
    {
        Fallar(e, "%s: el resultado falso no valida (%d participantes, duelo %d)", contexto, cantidad, (int)duelo);
    }

    partida.ContinuarTrasMinijuego();
}


//==================================================
// UNA PARTIDA
//==================================================

static void SimularPartida(
    IdTablero id,
    int cantidadJugadores,
    unsigned int semilla,
    EstadisticasTablero& e
)
{
    static PartidaTablero partida;
    Participante participantes[MAX_PARTICIPANTES];

    for (int i = 0; i < cantidadJugadores; i++)
    {
        participantes[i].activo = true;
        participantes[i].esBot = true;
        participantes[i].conectado = true;
        participantes[i].numeroJugador = i + 1;
        participantes[i].personajeSeleccionado = i;
    }

    char contexto[96];
    std::snprintf(contexto, sizeof(contexto), "%d jugadores, semilla %u", cantidadJugadores, semilla);

    SetRandomSeed(semilla);

    partida = PartidaTablero{};
    partida.lectorEntrada = EntradaConfirmar;
    partida.Inicializar(participantes, cantidadJugadores, nullptr, id);

    e.partidas++;

    if (partida.cantidadJugadores != cantidadJugadores)
    {
        Fallar(e, "%s: la partida tomo %d jugadores", contexto, partida.cantidadJugadores);
        return;
    }

    FasePartidaTablero fasePrevia = partida.fase;
    float tiempoMismaFase = 0.0f;
    int paso = 0;

    for (; paso < LIMITE_PASOS && partida.fase != FASE_PARTIDA_TABLERO_TERMINADA; paso++)
    {
        FasePartidaTablero antes = partida.fase;
        int turno = ObtenerParticipanteTurnoTableroFinal(partida);
        int trofeosAntes = turno >= 0 ? partida.jugadores[turno].trofeos : 0;

        partida.Actualizar(DT_SIMULACION);

        if (partida.SolicitaSalida())
        {
            Fallar(e, "%s: la partida pidio salir sin entrada (fase %s)", contexto, NombreFase(partida.fase));
            return;
        }

        // Llegada a una casilla que se va a resolver.
        if (partida.fase == FASE_PARTIDA_TABLERO_EVENTO_CASILLA && antes != FASE_PARTIDA_TABLERO_EVENTO_CASILLA)
        {
            int actual = ObtenerParticipanteTurnoTableroFinal(partida);
            const Casilla* casilla =
                actual >= 0 ? partida.tablero.ObtenerCasilla(partida.jugadores[actual].casillaActual) : nullptr;

            if (casilla != nullptr && casilla->tipo >= 0 && casilla->tipo < CANTIDAD_TIPOS_CASILLA)
            {
                e.visitas[casilla->tipo]++;
            }
        }

        if (
            antes == FASE_PARTIDA_TABLERO_DECISION_TROFEO &&
            turno >= 0 &&
            partida.jugadores[turno].trofeos > trofeosAntes
        )
        {
            e.trofeosComprados += partida.jugadores[turno].trofeos - trofeosAntes;
        }

        if (partida.SolicitaMinijuego())
        {
            ResolverMinijuegoFalso(partida, participantes, e, contexto);
        }

        // Invariantes en cada paso.
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            const EstadoJugadorPartidaTablero& jugador = partida.jugadores[i];

            if (!jugador.participa)
            {
                continue;
            }

            if (jugador.monedas < 0 || jugador.trofeos < 0)
            {
                Fallar(e, "%s: J%d con %d monedas y %d trofeos (ronda %d, fase %s)",
                    contexto, i + 1, jugador.monedas, jugador.trofeos,
                    partida.rondaActual, NombreFase(partida.fase));
                return;
            }

            if (jugador.casillaActual < 0 || jugador.casillaActual >= partida.tablero.cantidadCasillas)
            {
                Fallar(e, "%s: J%d en casilla invalida %d (ronda %d)",
                    contexto, i + 1, jugador.casillaActual, partida.rondaActual);
                return;
            }
        }

        char detalle[160];

        if (BuscarConexionInvalida(partida.tablero, detalle, (int)sizeof(detalle)) >= 0)
        {
            Fallar(e, "%s: conexiones en ronda %d: %s", contexto, partida.rondaActual, detalle);
            return;
        }

        if (partida.fase == fasePrevia)
        {
            tiempoMismaFase += DT_SIMULACION;

            if (tiempoMismaFase >= LIMITE_SEGUNDOS_MISMA_FASE)
            {
                break;
            }
        }
        else
        {
            fasePrevia = partida.fase;
            tiempoMismaFase = 0.0f;
        }
    }

    e.pasos += paso;

    if (partida.fase != FASE_PARTIDA_TABLERO_TERMINADA)
    {
        Fallar(e, "%s: TRABADA en fase %s (ronda %d/%d, turno J%d, %.0f s en la fase, paso %d, evento %s)",
            contexto, NombreFase(partida.fase), partida.rondaActual, partida.cantidadRondas,
            ObtenerParticipanteTurnoTableroFinal(partida) + 1, tiempoMismaFase, paso,
            partida.evento.activo ? "activo" : "inactivo");
        return;
    }

    if (partida.rondaActual != partida.cantidadRondas)
    {
        Fallar(e, "%s: termino en la ronda %d de %d", contexto, partida.rondaActual, partida.cantidadRondas);
        return;
    }

    e.partidasTerminadas++;
    e.rondas += partida.rondaActual;
}


//==================================================
// PRINCIPAL
//==================================================

static bool CoincideTablero(const char* filtro, int indice, const char* nombre)
{
    if (filtro == nullptr || filtro[0] == '\0')
    {
        return true;
    }

    char* fin = nullptr;
    long numero = std::strtol(filtro, &fin, 10);

    if (fin != filtro && *fin == '\0')
    {
        return numero == indice;
    }

    // Subcadena sin distinguir mayusculas.
    for (const char* p = nombre; *p != '\0'; p++)
    {
        int k = 0;

        while (
            filtro[k] != '\0' && p[k] != '\0' &&
            std::tolower((unsigned char)p[k]) == std::tolower((unsigned char)filtro[k])
        )
        {
            k++;
        }

        if (filtro[k] == '\0')
        {
            return true;
        }
    }

    return false;
}


int main(int argc, char** argv)
{
    const char* filtro = nullptr;
    int semillas = 5;

    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--tablero") == 0 && i + 1 < argc)
        {
            filtro = argv[++i];
        }
        else if (std::strcmp(argv[i], "--semillas") == 0 && i + 1 < argc)
        {
            semillas = std::atoi(argv[++i]);
        }
        else
        {
            std::fprintf(stderr, "Uso: %s [--tablero N|nombre] [--semillas N]\n", argv[0]);
            return 2;
        }
    }

    if (semillas < 1)
    {
        semillas = 1;
    }

    SetTraceLogLevel(LOG_ERROR);

    int tablerosProbados = 0;
    int tablerosFallidos = 0;

    for (int t = 0; t < ObtenerCantidadTableros(); t++)
    {
        IdTablero id = (IdTablero)t;
        const DefinicionTablero& def = ObtenerDefinicionTablero(id);
        const char* nombre = def.nombre != nullptr && def.nombre[0] != '\0' ? def.nombre : "(sin nombre)";

        if (!CoincideTablero(filtro, t, nombre))
        {
            continue;
        }

        tablerosProbados++;
        std::printf("Tablero %d: %s\n", t, nombre);

        EstadisticasTablero e;

        if (def.id != id)
        {
            Fallar(e, "ObtenerDefinicionTablero(%d) devuelve la definicion con id %d", t, (int)def.id);
        }

        if (ValidarDefinicion(def, e))
        {
            for (int jugadores = 2; jugadores <= MAX_PARTICIPANTES; jugadores++)
            {
                for (int s = 0; s < semillas; s++)
                {
                    unsigned int semilla = 1000u * (unsigned int)(t + 1) + 100u * (unsigned int)jugadores + (unsigned int)s;
                    SimularPartida(id, jugadores, semilla, e);
                }
            }
        }

        char visitas[400] = "";

        for (int tipo = 0; tipo < CANTIDAD_TIPOS_CASILLA; tipo++)
        {
            char parte[48];
            std::snprintf(parte, sizeof(parte), "%s%s %d", tipo > 0 ? ", " : "",
                ObtenerNombreTipoCasilla((TipoCasilla)tipo), e.visitas[tipo]);
            std::strncat(visitas, parte, sizeof(visitas) - std::strlen(visitas) - 1);
        }

        double pasosMedios = e.partidas > 0 ? (double)e.pasos / e.partidas : 0.0;

        if (e.partidas > 0)
        {
            std::printf("        partidas %d/%d terminadas, rondas %d, pasos medios %.0f (%.0f s simulados)\n",
                e.partidasTerminadas, e.partidas, e.rondas, pasosMedios, pasosMedios * DT_SIMULACION);
            std::printf("        trofeos comprados %d, minijuegos %d, duelos %d, cancelados %d\n",
                e.trofeosComprados, e.minijuegos, e.duelos, e.cancelados);
            std::printf("        visitas: %s\n", visitas);

            // Aviso (no falla): un tipo presente que ningun bot resolvio.
            for (int tipo = 0; tipo < CANTIDAD_TIPOS_CASILLA; tipo++)
            {
                if (e.partidasTerminadas > 0 && e.casillasPorTipo[tipo] > 0 && e.visitas[tipo] == 0)
                {
                    std::printf("        AVISO: %d casilla(s) %s y ningun bot cayo en ellas\n",
                        e.casillasPorTipo[tipo], ObtenerNombreTipoCasilla((TipoCasilla)tipo));
                }
            }

            if (e.partidasTerminadas > 0 && e.trofeosComprados == 0)
            {
                Fallar(e, "ningun bot compro un trofeo en %d partidas", e.partidasTerminadas);
            }
        }

        bool ok = e.fallas == 0;

        if (!ok)
        {
            tablerosFallidos++;
        }

        std::printf("%s %s (%d falla(s))\n\n", ok ? "[OK]   " : "[FALLA]", nombre, e.fallas);
    }

    if (tablerosProbados == 0)
    {
        std::printf("Ningun tablero coincide con '%s'.\n", filtro != nullptr ? filtro : "");
        return 2;
    }

    std::printf("Resultado: %d tablero(s) OK, %d con FALLA.\n",
        tablerosProbados - tablerosFallidos, tablerosFallidos);

    return tablerosFallidos > 0 ? 1 : 0;
}
