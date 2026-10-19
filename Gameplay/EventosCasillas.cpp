#include "Gameplay/EventosCasillas.h"

#include "Gameplay/PartidaTablero.h"
#include "UI/EstiloUI.h"
#include "UI/EstiloUIGlobal.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// CONSTANTES
//==================================================

static const float PI_EVENTO = 3.14159265f;
static const float LIMITE_TOTAL_EVENTO = 60.0f;       // red de seguridad

// Pasos de cada evento.
enum PasoCaos { PASO_CAOS_ESPERA = 0, PASO_CAOS_GIRA, PASO_CAOS_RESULTADO };

enum PasoIntercambio
{
    PASO_INTERCAMBIO_ESPERA = 0,
    PASO_INTERCAMBIO_DADO_1,
    PASO_INTERCAMBIO_DADO_2,
    PASO_INTERCAMBIO_DADO_3,
    PASO_INTERCAMBIO_RESUMEN
};

enum PasoObjetos { PASO_OBJETOS_CAIDA = 0, PASO_OBJETOS_ELIGE, PASO_OBJETOS_REVELA };

enum PasoAbejas { PASO_ABEJAS_LLEGAN = 0, PASO_ABEJAS_LLEVAN, PASO_ABEJAS_SUELTAN };

static const float DURACION_TIRADA_DADO = 0.90f;
static const float DURACION_DADO_VISTO = 1.50f;
static const float DURACION_CAIDA_OBJETOS = 0.95f;
static const float DURACION_ABEJAS_LLEGAN = 1.30f;
static const float DURACION_ABEJAS_LLEVAN = 2.30f;
static const float DURACION_ABEJAS_SUELTAN = 1.10f;
static const float ALTURA_VUELO_ABEJAS = 3.2f;


//==================================================
// UTILIDADES
//==================================================

static float Limitar01Evento(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static int MinimoEvento(int a, int b)
{
    return a < b ? a : b;
}


static void IrAPaso(EventoInteractivo& evento, int paso)
{
    evento.paso = paso;
    evento.tiempoPaso = 0.0f;
}


// Jugadores que participan, en el orden de turno.
static int ListarJugadoresEvento(
    const PartidaTablero& partida,
    int lista[]
)
{
    int cantidad = 0;

    for (int i = 0; i < partida.cantidadJugadores && cantidad < MAX_PARTICIPANTES; i++)
    {
        int indice = partida.ordenParticipantes[i];

        if (indice >= 0 && indice < MAX_PARTICIPANTES)
        {
            lista[cantidad++] = indice;
        }
    }

    return cantidad;
}


// El paso termina cuando el bot reacciona (retardoBot), el humano confirma
// (tras minimoHumano, para no heredar una pulsacion anterior) o se agota
// el limite (la partida nunca queda esperando).
static bool PasoListo(
    const EventoInteractivo& evento,
    bool automatico,
    bool confirmar,
    float minimoHumano,
    float retardoBot,
    float limite
)
{
    if (evento.tiempoPaso >= limite)
    {
        return true;
    }

    if (automatico)
    {
        return evento.tiempoPaso >= retardoBot;
    }

    return confirmar && evento.tiempoPaso >= minimoHumano;
}


//==================================================
// REGLAS: CAOS
//==================================================

// La ruleta tiene cuatro ranuras que se repiten. La A es "pierde 1 trofeo"
// o, sin trofeos, "pierde 10 monedas".
static OpcionCaos OpcionDeRanura(int ranura, bool conTrofeos)
{
    switch (ranura)
    {
        case 0: return conTrofeos ? CAOS_PIERDE_TROFEO : CAOS_PIERDE_POCAS;
        case 1: return CAOS_PIERDE_MONEDAS;
        case 2: return CAOS_PIERDE_TODAS;
        default: break;
    }

    return CAOS_PARA_TODOS;
}


OpcionCaos ElegirOpcionCaos(
    const PartidaTablero& partida,
    int jugador
)
{
    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    bool conTrofeos = estado.trofeos > 0;
    bool conMonedas = estado.monedas > 0;

    OpcionCaos validas[4];
    int cantidad = 0;

    for (int ranura = 0; ranura < 4; ranura++)
    {
        OpcionCaos opcion = OpcionDeRanura(ranura, conTrofeos);
        bool conEfecto = true;

        if (opcion == CAOS_PIERDE_TROFEO)
        {
            conEfecto = conTrofeos;
        }
        else if (opcion != CAOS_PARA_TODOS)
        {
            conEfecto = conMonedas;
        }

        if (conEfecto)
        {
            validas[cantidad++] = opcion;
        }
    }

    // El caos para todos siempre es posible.
    if (cantidad == 0)
    {
        return CAOS_PARA_TODOS;
    }

    return validas[GetRandomValue(0, cantidad - 1)];
}


void AplicarOpcionCaos(
    PartidaTablero& partida,
    int jugador,
    OpcionCaos opcion,
    char* resumen,
    int capacidad
)
{
    EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    switch (opcion)
    {
        case CAOS_PIERDE_TROFEO:
            if (estado.trofeos > 0 && EscudoAbsorbePerdida(partida, jugador))
            {
                std::snprintf(resumen, capacidad, "¡EL ESCUDO DE J%d BLOQUEA LA PÉRDIDA DEL TROFEO!", jugador + 1);
            }
            else if (estado.trofeos > 0)
            {
                estado.trofeos--;
                std::snprintf(resumen, capacidad, "¡J%d PIERDE 1 TROFEO!", jugador + 1);
            }
            else
            {
                std::snprintf(resumen, capacidad, "J%d NO TENÍA TROFEOS: SE SALVA", jugador + 1);
            }
            return;

        case CAOS_PIERDE_MONEDAS:
        case CAOS_PIERDE_POCAS:
        {
            int pedido =
                opcion == CAOS_PIERDE_MONEDAS
                ? CAOS_MONEDAS_PERDIDAS
                : CAOS_MONEDAS_SUSTITUTA;

            int perdida = MinimoEvento(pedido, estado.monedas);

            if (perdida > 0 && EscudoAbsorbePerdida(partida, jugador))
            {
                std::snprintf(resumen, capacidad, "¡EL ESCUDO DE J%d BLOQUEA LA PÉRDIDA DE MONEDAS!", jugador + 1);
                return;
            }

            estado.monedas -= perdida;
            partida.variacionMonedasEvento = -perdida;

            if (perdida == 0)
            {
                std::snprintf(resumen, capacidad, "J%d NO TENÍA MONEDAS: SE SALVA", jugador + 1);
            }
            else
            {
                std::snprintf(resumen, capacidad, "¡J%d PIERDE %d MONEDAS!", jugador + 1, perdida);
            }
            return;
        }

        case CAOS_PIERDE_TODAS:
        {
            int perdida = estado.monedas;

            if (perdida > 0 && EscudoAbsorbePerdida(partida, jugador))
            {
                std::snprintf(resumen, capacidad, "¡EL ESCUDO DE J%d BLOQUEA LA PÉRDIDA DE MONEDAS!", jugador + 1);
                return;
            }

            estado.monedas = 0;
            partida.variacionMonedasEvento = -perdida;

            if (perdida == 0)
            {
                std::snprintf(resumen, capacidad, "J%d NO TENÍA MONEDAS: SE SALVA", jugador + 1);
            }
            else
            {
                std::snprintf(resumen, capacidad, "¡J%d PIERDE TODAS SUS MONEDAS (%d)!", jugador + 1, perdida);
            }
            return;
        }

        case CAOS_PARA_TODOS:
        {
            int lista[MAX_PARTICIPANTES];
            int cantidad = ListarJugadoresEvento(partida, lista);
            int bloqueados = 0;

            for (int i = 0; i < cantidad; i++)
            {
                EstadoJugadorPartidaTablero& otro = partida.jugadores[lista[i]];
                int perdida = MinimoEvento(CAOS_MONEDAS_GLOBAL, otro.monedas);

                if (perdida > 0 && EscudoAbsorbePerdida(partida, lista[i]))
                {
                    perdida = 0;
                    bloqueados++;
                }

                otro.monedas -= perdida;

                if (lista[i] == jugador)
                {
                    partida.variacionMonedasEvento = -perdida;
                }
            }

            if (bloqueados > 0)
            {
                std::snprintf(
                    resumen, capacidad,
                    "¡CAOS GLOBAL! -%d A TODOS (%d ESCUDO%s LO BLOQUEA%s)",
                    CAOS_MONEDAS_GLOBAL, bloqueados,
                    bloqueados == 1 ? "" : "S", bloqueados == 1 ? "" : "N"
                );
            }
            else
            {
                std::snprintf(
                    resumen, capacidad,
                    "¡CAOS GLOBAL! TODOS PIERDEN HASTA %d MONEDAS",
                    CAOS_MONEDAS_GLOBAL
                );
            }
            return;
        }

        case CANTIDAD_OPCIONES_CAOS:
            break;
    }

    resumen[0] = '\0';
}


//==================================================
// REGLAS: INTERCAMBIO
//==================================================

static int TotalRecurso(
    const PartidaTablero& partida,
    int jugador,
    RecursoIntercambio recurso
)
{
    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    return recurso == RECURSO_INTERCAMBIO_MONEDAS
        ? estado.monedas
        : estado.trofeos;
}


static void FijarRecurso(
    PartidaTablero& partida,
    int jugador,
    RecursoIntercambio recurso,
    int valor
)
{
    EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    if (recurso == RECURSO_INTERCAMBIO_MONEDAS)
    {
        estado.monedas = valor;
    }
    else
    {
        estado.trofeos = valor;
    }
}


// Cuanto se movera de verdad con el sorteo actual (0 = no cambia nada).
static int EfectoIntercambio(
    const PartidaTablero& partida,
    const EventoInteractivo& evento
)
{
    int totalA = TotalRecurso(partida, evento.personaA, evento.recurso);
    int totalB = TotalRecurso(partida, evento.personaB, evento.recurso);

    switch (evento.accion)
    {
        case INTERCAMBIO_DA:
            return MinimoEvento(evento.cantidad, totalA);

        case INTERCAMBIO_RECIBE:
            return MinimoEvento(evento.cantidad, totalB);

        case INTERCAMBIO_CAMBIA:
            return totalA != totalB ? 1 : 0;
    }

    return 0;
}


void SortearIntercambio(
    const PartidaTablero& partida,
    EventoInteractivo& evento
)
{
    int lista[MAX_PARTICIPANTES];
    int cantidad = ListarJugadoresEvento(partida, lista);

    evento.sinEfecto = false;
    evento.movido = 0;

    if (cantidad < 2)
    {
        evento.sinEfecto = true;
        evento.personaA = cantidad > 0 ? lista[0] : -1;
        evento.personaB = -1;
        return;
    }

    // Dado 1: primera persona (cualquiera, incluido quien cayo).
    int indiceA = GetRandomValue(0, cantidad - 1);
    evento.personaA = lista[indiceA];

    // Dado 3: segunda persona, distinta de la primera.
    int indiceB = GetRandomValue(0, cantidad - 2);

    if (indiceB >= indiceA)
    {
        indiceB++;
    }

    evento.personaB = lista[indiceB];

    // Dado 2: da, recibe o intercambia, con monedas (5 a 15) o 1 trofeo.
    evento.accion = (AccionIntercambio)GetRandomValue(0, 2);
    evento.recurso =
        GetRandomValue(0, 3) == 0
        ? RECURSO_INTERCAMBIO_TROFEOS
        : RECURSO_INTERCAMBIO_MONEDAS;

    evento.cantidad =
        evento.recurso == RECURSO_INTERCAMBIO_TROFEOS
        ? 1
        : GetRandomValue(INTERCAMBIO_MONEDAS_MIN, INTERCAMBIO_MONEDAS_MAX);

    // Si los trofeos no mueven nada, el dado cae en monedas.
    if (
        evento.recurso == RECURSO_INTERCAMBIO_TROFEOS &&
        EfectoIntercambio(partida, evento) == 0
    )
    {
        evento.recurso = RECURSO_INTERCAMBIO_MONEDAS;
        evento.cantidad =
            GetRandomValue(INTERCAMBIO_MONEDAS_MIN, INTERCAMBIO_MONEDAS_MAX);
    }
}


static const char* NombreRecurso(RecursoIntercambio recurso, int cantidad)
{
    if (recurso == RECURSO_INTERCAMBIO_MONEDAS)
    {
        return cantidad == 1 ? "MONEDA" : "MONEDAS";
    }

    return cantidad == 1 ? "TROFEO" : "TROFEOS";
}


void AplicarIntercambio(
    PartidaTablero& partida,
    EventoInteractivo& evento
)
{
    char* texto = evento.resumen;
    int capacidad = (int)sizeof(evento.resumen);

    if (evento.sinEfecto || evento.personaA < 0 || evento.personaB < 0)
    {
        std::snprintf(texto, capacidad, "NO HAY OTRO JUGADOR PARA INTERCAMBIAR");
        return;
    }

    int a = evento.personaA;
    int b = evento.personaB;
    RecursoIntercambio recurso = evento.recurso;
    int totalA = TotalRecurso(partida, a, recurso);
    int totalB = TotalRecurso(partida, b, recurso);

    if (evento.accion == INTERCAMBIO_DA || evento.accion == INTERCAMBIO_RECIBE)
    {
        bool da = evento.accion == INTERCAMBIO_DA;
        int origen = da ? a : b;
        int destino = da ? b : a;
        int disponible = da ? totalA : totalB;
        int movido = MinimoEvento(evento.cantidad, disponible);

        if (movido > 0 && EscudoAbsorbePerdida(partida, origen))
        {
            evento.movido = 0;
            std::snprintf(texto, capacidad, "¡EL ESCUDO DE J%d BLOQUEA LA ENTREGA!", origen + 1);
            return;
        }

        evento.movido = movido;

        FijarRecurso(partida, origen, recurso, disponible - movido);
        FijarRecurso(partida, destino, recurso, TotalRecurso(partida, destino, recurso) + movido);

        if (movido == 0)
        {
            std::snprintf(
                texto, capacidad, "J%d NO TIENE %s PARA DAR: NO PASA NADA",
                origen + 1, NombreRecurso(recurso, 2)
            );
        }
        else if (movido < evento.cantidad)
        {
            std::snprintf(
                texto, capacidad, "J%d LE DA A J%d TODO LO QUE TIENE: %d %s",
                origen + 1, destino + 1, movido, NombreRecurso(recurso, movido)
            );
        }
        else if (da)
        {
            std::snprintf(
                texto, capacidad, "J%d LE DA %d %s A J%d",
                a + 1, movido, NombreRecurso(recurso, movido), b + 1
            );
        }
        else
        {
            std::snprintf(
                texto, capacidad, "J%d RECIBE %d %s DE J%d",
                a + 1, movido, NombreRecurso(recurso, movido), b + 1
            );
        }

        return;
    }

    // Intercambia: se permutan los totales del recurso sorteado.
    FijarRecurso(partida, a, recurso, totalB);
    FijarRecurso(partida, b, recurso, totalA);

    evento.movido = totalA != totalB ? 1 : 0;

    if (totalA == totalB)
    {
        std::snprintf(
            texto, capacidad, "J%d Y J%d TIENEN LO MISMO: NO CAMBIA NADA",
            a + 1, b + 1
        );
    }
    else
    {
        std::snprintf(
            texto, capacidad, "J%d Y J%d CAMBIAN SUS %s (%d POR %d)",
            a + 1, b + 1, NombreRecurso(recurso, 2), totalA, totalB
        );
    }
}


//==================================================
// INICIO
//==================================================

static void IniciarCaos(PartidaTablero& partida, EventoInteractivo& evento)
{
    EstadoJugadorPartidaTablero& estado = partida.jugadores[evento.jugador];
    bool conTrofeos = estado.trofeos > 0;

    // Las cuatro ranuras se repiten dos veces alrededor de la ruleta.
    const Color colores[4] =
    {
        Color{ 112, 64, 150, 255 },
        Color{ 190, 54, 54, 255 },
        Color{ 82, 48, 44, 255 },
        Color{ 168, 40, 112, 255 }
    };

    SectorRuletaOpciones sectores[8];

    for (int i = 0; i < 8; i++)
    {
        int ranura = i % 4;
        OpcionCaos opcion = OpcionDeRanura(ranura, conTrofeos);
        const char* texto = "";

        switch (opcion)
        {
            case CAOS_PIERDE_TROFEO: texto = "-1 TROFEO"; break;
            case CAOS_PIERDE_MONEDAS: texto = "-20 MONEDAS"; break;
            case CAOS_PIERDE_TODAS: texto = "SIN MONEDAS"; break;
            case CAOS_PARA_TODOS: texto = "CAOS GLOBAL"; break;
            case CAOS_PIERDE_POCAS: texto = "-10 MONEDAS"; break;
            case CANTIDAD_OPCIONES_CAOS: break;
        }

        std::snprintf(sectores[i].texto, sizeof(sectores[i].texto), "%s", texto);
        sectores[i].color = colores[ranura];
        evento.opcionSector[i] = opcion;
    }

    // El resultado se decide ahora; la ruleta solo lo muestra.
    evento.opcionCaos = ElegirOpcionCaos(partida, evento.jugador);

    int ranura = 0;

    for (int r = 0; r < 4; r++)
    {
        if (OpcionDeRanura(r, conTrofeos) == evento.opcionCaos)
        {
            ranura = r;
        }
    }

    int sectorGanador = ranura + 4 * GetRandomValue(0, 1);

    evento.ruleta.Iniciar(sectores, 8, sectorGanador, partida.audio);
    IrAPaso(evento, PASO_CAOS_ESPERA);
}


static void IniciarIntercambio(PartidaTablero& partida, EventoInteractivo& evento)
{
    SortearIntercambio(partida, evento);

    if (evento.sinEfecto)
    {
        AplicarIntercambio(partida, evento);
        IrAPaso(evento, PASO_INTERCAMBIO_RESUMEN);
        return;
    }

    IrAPaso(evento, PASO_INTERCAMBIO_ESPERA);
}


static void IniciarObjetos(PartidaTablero& partida, EventoInteractivo& evento)
{
    (void)partida;

    // Cual de los dos es el bueno se decide ahora.
    evento.ladoBueno = GetRandomValue(0, 1);
    evento.ladoResaltado = GetRandomValue(0, 1);
    evento.ladoElegido = -1;
    evento.reaccionBot = 1.0f + 0.8f * (float)GetRandomValue(0, 100) / 100.0f;

    IrAPaso(evento, PASO_OBJETOS_CAIDA);
    ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);
}


static void IniciarAbejas(PartidaTablero& partida, EventoInteractivo& evento)
{
    EstadoJugadorPartidaTablero& estado = partida.jugadores[evento.jugador];
    int casillaOriginal = estado.casillaActual;
    Vector3 posicionOriginal = estado.posicionVisual;

    // El punto de llegada exacto sale de la misma funcion que usa el
    // teletransporte; despues se deja a la ficha donde estaba.
    TeletransportarJugadorTablero(partida, evento.jugador, 0);
    evento.destinoVuelo = estado.posicionVisual;
    estado.casillaActual = casillaOriginal;
    estado.posicionVisual = posicionOriginal;
    evento.origenVuelo = posicionOriginal;

    std::snprintf(
        evento.resumen, sizeof(evento.resumen),
        "¡LAS ABEJAS LLEVAN A J%d AL INICIO!", evento.jugador + 1
    );

    IrAPaso(evento, PASO_ABEJAS_LLEGAN);
    ReproducirSonidoTablero(partida, SONIDO_ALERTA_TIEMPO);
}


void IniciarEventoInteractivo(
    PartidaTablero& partida,
    int jugador,
    TipoEventoInteractivo tipo,
    const char* fuente
)
{
    if (
        jugador < 0 ||
        jugador >= MAX_PARTICIPANTES ||
        tipo == EVENTO_INTERACTIVO_NINGUNO
    )
    {
        return;
    }

    EventoInteractivo& evento = partida.evento;

    evento = EventoInteractivo{};
    evento.tipo = tipo;
    evento.activo = true;
    evento.jugador = jugador;
    evento.fuente = fuente != nullptr ? fuente : "";
    evento.reaccionBot = 0.8f + 0.5f * (float)GetRandomValue(0, 100) / 100.0f;

    switch (tipo)
    {
        case EVENTO_INTERACTIVO_CAOS:
            IniciarCaos(partida, evento);
            break;

        case EVENTO_INTERACTIVO_INTERCAMBIO:
            IniciarIntercambio(partida, evento);
            break;

        case EVENTO_INTERACTIVO_OBJETOS_ARBOL:
            IniciarObjetos(partida, evento);
            break;

        case EVENTO_INTERACTIVO_ABEJAS:
            IniciarAbejas(partida, evento);
            break;

        case EVENTO_INTERACTIVO_TIENDA:
            IniciarTienda(partida, jugador, evento.tienda);
            break;

        case EVENTO_INTERACTIVO_NINGUNO:
            break;
    }
}


bool EventoInteractivoActivo(const PartidaTablero& partida)
{
    return partida.evento.activo;
}


//==================================================
// ACTUALIZACION
//==================================================

static void ActualizarCaos(
    PartidaTablero& partida,
    EventoInteractivo& evento,
    float deltaTime,
    bool automatico,
    bool confirmar
)
{
    switch (evento.paso)
    {
        case PASO_CAOS_ESPERA:
            if (PasoListo(evento, automatico, confirmar, 0.30f, evento.reaccionBot, 12.0f))
            {
                evento.ruleta.Girar();
                ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);
                IrAPaso(evento, PASO_CAOS_GIRA);
            }
            break;

        case PASO_CAOS_GIRA:
            evento.ruleta.Actualizar(deltaTime, confirmar && evento.tiempoPaso > 0.8f);

            if (evento.ruleta.Termino() || evento.tiempoPaso > 12.0f)
            {
                partida.variacionMonedasEvento = 0;

                AplicarOpcionCaos(
                    partida, evento.jugador, evento.opcionCaos,
                    evento.resumen, (int)sizeof(evento.resumen)
                );

                ReproducirSonidoTablero(partida, SONIDO_CASILLA_NEGATIVA);

                if (evento.opcionCaos == CAOS_PARA_TODOS)
                {
                    ReproducirSonidoTablero(partida, SONIDO_EXPLOSION);
                }

                IrAPaso(evento, PASO_CAOS_RESULTADO);
            }
            break;

        case PASO_CAOS_RESULTADO:
            evento.ruleta.Actualizar(deltaTime, false);

            if (PasoListo(evento, automatico, confirmar, 0.60f, 1.9f, 8.0f))
            {
                evento.terminado = true;
            }
            break;

        default:
            evento.terminado = true;
            break;
    }
}


static void ActualizarIntercambio(
    PartidaTablero& partida,
    EventoInteractivo& evento,
    bool automatico,
    bool confirmar
)
{
    switch (evento.paso)
    {
        case PASO_INTERCAMBIO_ESPERA:
            if (PasoListo(evento, automatico, confirmar, 0.30f, evento.reaccionBot, 12.0f))
            {
                IrAPaso(evento, PASO_INTERCAMBIO_DADO_1);
                ReproducirSonidoTablero(partida, SONIDO_DADO);
            }
            break;

        case PASO_INTERCAMBIO_DADO_1:
        case PASO_INTERCAMBIO_DADO_2:
        case PASO_INTERCAMBIO_DADO_3:
        {
            // El humano puede adelantar la lectura una vez que el dado paro.
            bool avanzar =
                evento.tiempoPaso >= DURACION_DADO_VISTO ||
                (
                    !automatico &&
                    confirmar &&
                    evento.tiempoPaso >= DURACION_TIRADA_DADO + 0.15f
                );

            if (!avanzar)
            {
                break;
            }

            if (evento.paso == PASO_INTERCAMBIO_DADO_3)
            {
                AplicarIntercambio(partida, evento);
                ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);
                IrAPaso(evento, PASO_INTERCAMBIO_RESUMEN);
            }
            else
            {
                IrAPaso(evento, evento.paso + 1);
                ReproducirSonidoTablero(partida, SONIDO_DADO);
            }
            break;
        }

        case PASO_INTERCAMBIO_RESUMEN:
            if (PasoListo(evento, automatico, confirmar, 0.60f, 2.2f, 8.0f))
            {
                evento.terminado = true;
            }
            break;

        default:
            evento.terminado = true;
            break;
    }
}


static void ElegirLadoObjetos(
    PartidaTablero& partida,
    EventoInteractivo& evento,
    int lado
)
{
    EstadoJugadorPartidaTablero& estado = partida.jugadores[evento.jugador];
    bool bueno = lado == evento.ladoBueno;

    evento.ladoElegido = lado;
    evento.ladoResaltado = lado;

    if (bueno)
    {
        evento.pasosExtra = OBJETO_BUENO_CASILLAS;

        std::snprintf(
            evento.resumen, sizeof(evento.resumen),
            "¡J%d ATRAPA EL OBJETO BUENO: AVANZA %d CASILLAS!",
            evento.jugador + 1, OBJETO_BUENO_CASILLAS
        );

        ReproducirSonidoTablero(partida, SONIDO_ACIERTO);
    }
    else
    {
        int perdida = MinimoEvento(OBJETO_MALO_MONEDAS, estado.monedas);
        bool escudo = perdida > 0 && EscudoAbsorbePerdida(partida, evento.jugador);

        if (escudo)
        {
            perdida = 0;
        }

        estado.monedas -= perdida;
        partida.variacionMonedasEvento = -perdida;

        if (escudo)
        {
            std::snprintf(
                evento.resumen, sizeof(evento.resumen),
                "¡J%d ATRAPA EL OBJETO MALO, PERO SU ESCUDO LO BLOQUEA!",
                evento.jugador + 1
            );
        }
        else if (perdida > 0)
        {
            std::snprintf(
                evento.resumen, sizeof(evento.resumen),
                "¡J%d ATRAPA EL OBJETO MALO: PIERDE %d MONEDAS!",
                evento.jugador + 1, perdida
            );
        }
        else
        {
            std::snprintf(
                evento.resumen, sizeof(evento.resumen),
                "¡J%d ATRAPA EL OBJETO MALO, PERO NO TENÍA MONEDAS!",
                evento.jugador + 1
            );
        }

        ReproducirSonidoTablero(partida, SONIDO_ERROR);
    }

    IrAPaso(evento, PASO_OBJETOS_REVELA);
}


static void ActualizarObjetos(
    PartidaTablero& partida,
    EventoInteractivo& evento,
    bool automatico,
    bool confirmar,
    const InputSeleccionParticipante& entrada
)
{
    switch (evento.paso)
    {
        case PASO_OBJETOS_CAIDA:
            if (evento.tiempoPaso >= DURACION_CAIDA_OBJETOS)
            {
                IrAPaso(evento, PASO_OBJETOS_ELIGE);
            }
            break;

        case PASO_OBJETOS_ELIGE:
        {
            if (automatico)
            {
                // El bot no sabe cual es el bueno: elige al azar.
                if (evento.tiempoPaso >= evento.reaccionBot)
                {
                    ElegirLadoObjetos(partida, evento, GetRandomValue(0, 1));
                }

                break;
            }

            int anterior = evento.ladoResaltado;

            if (entrada.izquierda)
            {
                evento.ladoResaltado = 0;
            }
            else if (entrada.derecha)
            {
                evento.ladoResaltado = 1;
            }

            if (evento.ladoResaltado != anterior)
            {
                ReproducirSonidoTablero(partida, SONIDO_UI_MOVER);
            }

            if (
                (confirmar && evento.tiempoPaso >= 0.30f) ||
                evento.tiempoPaso >= 15.0f
            )
            {
                ReproducirSonidoTablero(partida, SONIDO_UI_CONFIRMAR);
                ElegirLadoObjetos(partida, evento, evento.ladoResaltado);
            }
            break;
        }

        case PASO_OBJETOS_REVELA:
            if (PasoListo(evento, automatico, confirmar, 0.70f, 2.2f, 8.0f))
            {
                evento.terminado = true;
            }
            break;

        default:
            evento.terminado = true;
            break;
    }
}


static void ActualizarAbejas(
    PartidaTablero& partida,
    EventoInteractivo& evento
)
{
    EstadoJugadorPartidaTablero& estado = partida.jugadores[evento.jugador];

    switch (evento.paso)
    {
        case PASO_ABEJAS_LLEGAN:
            if (evento.tiempoPaso >= DURACION_ABEJAS_LLEGAN)
            {
                IrAPaso(evento, PASO_ABEJAS_LLEVAN);
            }
            break;

        case PASO_ABEJAS_LLEVAN:
        {
            float u = Limitar01Evento(evento.tiempoPaso / DURACION_ABEJAS_LLEVAN);
            float suave = u * u * (3.0f - 2.0f * u);

            estado.posicionVisual =
            {
                evento.origenVuelo.x + (evento.destinoVuelo.x - evento.origenVuelo.x) * suave,
                evento.origenVuelo.y + (evento.destinoVuelo.y - evento.origenVuelo.y) * suave
                    + std::sin(u * PI_EVENTO) * ALTURA_VUELO_ABEJAS,
                evento.origenVuelo.z + (evento.destinoVuelo.z - evento.origenVuelo.z) * suave
            };

            if (u >= 1.0f)
            {
                TeletransportarJugadorTablero(partida, evento.jugador, 0);
                ReproducirSonidoTablero(partida, SONIDO_ATERRIZAJE);
                IrAPaso(evento, PASO_ABEJAS_SUELTAN);
            }
            break;
        }

        case PASO_ABEJAS_SUELTAN:
            if (evento.tiempoPaso >= DURACION_ABEJAS_SUELTAN)
            {
                evento.terminado = true;
            }
            break;

        default:
            evento.terminado = true;
            break;
    }
}


void ActualizarEventoInteractivo(
    PartidaTablero& partida,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
)
{
    EventoInteractivo& evento = partida.evento;

    if (!evento.activo || evento.terminado)
    {
        return;
    }

    evento.tiempoPaso += deltaTime;
    evento.tiempoTotal += deltaTime;

    bool confirmar = puedeDecidir && !esAutomatico && entrada.confirmar;

    switch (evento.tipo)
    {
        case EVENTO_INTERACTIVO_CAOS:
            ActualizarCaos(partida, evento, deltaTime, esAutomatico, confirmar);
            break;

        case EVENTO_INTERACTIVO_INTERCAMBIO:
            ActualizarIntercambio(partida, evento, esAutomatico, confirmar);
            break;

        case EVENTO_INTERACTIVO_OBJETOS_ARBOL:
            ActualizarObjetos(
                partida, evento, esAutomatico, confirmar,
                puedeDecidir ? entrada : InputSeleccionParticipante{}
            );
            break;

        case EVENTO_INTERACTIVO_ABEJAS:
            ActualizarAbejas(partida, evento);
            break;

        case EVENTO_INTERACTIVO_TIENDA:
            ActualizarTienda(
                partida, evento.jugador, evento.tienda, deltaTime,
                esAutomatico, puedeDecidir, entrada
            );

            if (evento.tienda.terminado)
            {
                evento.terminado = true;
            }
            break;

        case EVENTO_INTERACTIVO_NINGUNO:
            evento.terminado = true;
            break;
    }

    // Red de seguridad: un evento no puede durar para siempre.
    float limiteTotal = evento.tipo == EVENTO_INTERACTIVO_TIENDA ? 4.0f * LIMITE_TOTAL_EVENTO : LIMITE_TOTAL_EVENTO;

    if (!evento.terminado && evento.tiempoTotal > limiteTotal)
    {
        evento.terminado = true;
    }
}


//==================================================
// TEXTOS
//==================================================

PedidoEventoInteractivo PedidoDelEventoInteractivo(const PartidaTablero& partida)
{
    const EventoInteractivo& evento = partida.evento;

    if (!evento.activo || evento.terminado)
    {
        return PEDIDO_EVENTO_NADA;
    }

    switch (evento.tipo)
    {
        case EVENTO_INTERACTIVO_CAOS:
            if (evento.paso == PASO_CAOS_ESPERA) return PEDIDO_EVENTO_GIRAR;
            if (evento.paso == PASO_CAOS_RESULTADO) return PEDIDO_EVENTO_CONTINUAR;
            break;

        case EVENTO_INTERACTIVO_INTERCAMBIO:
            if (evento.paso == PASO_INTERCAMBIO_ESPERA) return PEDIDO_EVENTO_DADOS;
            if (evento.paso == PASO_INTERCAMBIO_RESUMEN) return PEDIDO_EVENTO_CONTINUAR;
            break;

        case EVENTO_INTERACTIVO_OBJETOS_ARBOL:
            if (evento.paso == PASO_OBJETOS_ELIGE) return PEDIDO_EVENTO_ELEGIR_OBJETO;
            if (evento.paso == PASO_OBJETOS_REVELA) return PEDIDO_EVENTO_CONTINUAR;
            break;

        case EVENTO_INTERACTIVO_TIENDA:
            return PEDIDO_EVENTO_TIENDA;

        case EVENTO_INTERACTIVO_ABEJAS:
        case EVENTO_INTERACTIVO_NINGUNO:
            break;
    }

    return PEDIDO_EVENTO_NADA;
}


void TextoEventoInteractivo(
    const PartidaTablero& partida,
    char* destino,
    int capacidad
)
{
    const EventoInteractivo& evento = partida.evento;
    destino[0] = '\0';

    if (!evento.activo || evento.jugador < 0)
    {
        return;
    }

    bool bot = partida.participantes != nullptr && partida.participantes[evento.jugador].esBot;

    switch (evento.tipo)
    {
        case EVENTO_INTERACTIVO_CAOS:
            if (evento.paso == PASO_CAOS_ESPERA)
            {
                std::snprintf(destino, capacidad, "%s", bot ? "EL BOT GIRA LA RULETA DEL CAOS..." : "¡CAOS! GIRÁ LA RULETA");
            }
            else if (evento.paso == PASO_CAOS_GIRA)
            {
                std::snprintf(destino, capacidad, "LA RULETA ELIGE EL CASTIGO...");
            }
            else
            {
                std::snprintf(destino, capacidad, "%s", evento.resumen);
            }
            return;

        case EVENTO_INTERACTIVO_INTERCAMBIO:
            if (evento.paso == PASO_INTERCAMBIO_ESPERA)
            {
                std::snprintf(destino, capacidad, "%s", bot ? "EL BOT TIRA LOS TRES DADOS..." : "¡INTERCAMBIO! TIRÁ LOS TRES DADOS");
            }
            else if (evento.paso == PASO_INTERCAMBIO_DADO_1)
            {
                std::snprintf(destino, capacidad, "DADO 1: ¿QUIÉN ES LA PRIMERA PERSONA?");
            }
            else if (evento.paso == PASO_INTERCAMBIO_DADO_2)
            {
                std::snprintf(destino, capacidad, "DADO 2: ¿QUÉ PASA Y CUÁNTO?");
            }
            else if (evento.paso == PASO_INTERCAMBIO_DADO_3)
            {
                std::snprintf(destino, capacidad, "DADO 3: ¿CON QUIÉN?");
            }
            else
            {
                std::snprintf(destino, capacidad, "%s", evento.resumen);
            }
            return;

        case EVENTO_INTERACTIVO_OBJETOS_ARBOL:
            if (evento.paso == PASO_OBJETOS_CAIDA)
            {
                std::snprintf(destino, capacidad, "¡%s TIRA DOS OBJETOS!", evento.fuente[0] != '\0' ? evento.fuente : "EL ÁRBOL");
            }
            else if (evento.paso == PASO_OBJETOS_ELIGE)
            {
                std::snprintf(destino, capacidad, "%s", bot ? "EL BOT ELIGE UN OBJETO..." : "ELEGÍ CUÁL ATRAPAR: UNO ES BUENO Y OTRO MALO");
            }
            else
            {
                std::snprintf(destino, capacidad, "%s", evento.resumen);
            }
            return;

        case EVENTO_INTERACTIVO_ABEJAS:
            std::snprintf(destino, capacidad, "%s", evento.resumen);
            return;

        case EVENTO_INTERACTIVO_TIENDA:
            TextoTienda(partida, evento.tienda, evento.jugador, destino, capacidad);
            return;

        case EVENTO_INTERACTIVO_NINGUNO:
            return;
    }
}


//==================================================
// DIBUJO 2D
//==================================================

static const Color COLOR_CARA_EVENTO = { 255, 248, 232, 255 };
static const Color COLOR_TITULO_MALO = { 255, 120, 100, 255 };

// Una ficha de color de jugador para la cara del dado.
static Color ColorJugadorEvento(const PartidaTablero& partida, int jugador)
{
    if (
        partida.participantes != nullptr &&
        jugador >= 0 &&
        jugador < MAX_PARTICIPANTES
    )
    {
        return partida.participantes[jugador].color;
    }

    return LIGHTGRAY;
}


static Vector2 PuntoRefEvento(float x, float y)
{
    Rectangle r = RectRef(x, y, 0.0f, 0.0f);

    return Vector2{ r.x, r.y };
}


static void DibujarTituloEvento(
    const char* titulo,
    const char* subtitulo,
    Color color,
    float alfa
)
{
    float cx = (float)GetScreenWidth() * 0.5f;

    DibujarTextoMundoContornoUI(titulo, cx, PuntoRefEvento(0.0f, 166.0f).y, UI_TEXTO_GRANDE, color, alfa);

    if (subtitulo != nullptr && subtitulo[0] != '\0')
    {
        DibujarTextoMundoContornoUI(subtitulo, cx, PuntoRefEvento(0.0f, 202.0f).y, UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }
}


// Placa con el resultado en palabras.
static void DibujarBandaEvento(
    const EstiloUI& estilo,
    float yRef,
    const char* texto,
    float alfa
)
{
    if (alfa <= 0.0f)
    {
        return;
    }

    Rectangle rect = RectRef(290.0f, yRef, 700.0f, 52.0f);
    Rectangle placa = DibujarPlacaConMarcoUI(estilo, rect, 6.0f, alfa);

    int tamano = TamanoTextoAjustadoUI(texto, UI_TEXTO, placa.width - Px(24.0f));

    DibujarTextoAjustadoUI(
        texto,
        placa.x + placa.width * 0.5f,
        placa.y + (placa.height - (float)tamano) * 0.5f,
        UI_TEXTO,
        placa.width - Px(24.0f),
        Fade(UI_TINTA, alfa),
        true
    );
}


static void DibujarCaos2D(const PartidaTablero& partida, const EstiloUI& estilo, float alfa)
{
    const EventoInteractivo& evento = partida.evento;
    float t = estilo.tiempo;

    DibujarTituloEvento(
        "¡CAOS!",
        TextFormat("LA RULETA ELIGE EL CASTIGO DE J%d", evento.jugador + 1),
        COLOR_TITULO_MALO,
        alfa
    );

    evento.ruleta.Dibujar(PuntoRefEvento(640.0f, 392.0f), Px(138.0f), t);

    if (evento.paso == PASO_CAOS_RESULTADO)
    {
        DibujarBandaEvento(estilo, 550.0f, evento.resumen, Limitar01Evento(evento.tiempoPaso / 0.2f));
    }
}


// Cara de un dado: placa crema con borde de tinta. `banda` colorea la parte
// de arriba (color del jugador); vacio si es negro/sin alfa.
static void DibujarCartaDadoEvento(
    Vector2 centro,
    float lado,
    float giro,
    Color banda,
    bool conBanda,
    const char* grande,
    int tokenGrande,
    const char* chico,
    int icono,                 // 0 ninguno, 1 moneda, 2 trofeo
    bool resaltar,
    float alfa
)
{
    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, 0.0f);
    rlRotatef(giro, 0.0f, 0.0f, 1.0f);

    float borde = Px(3.0f);
    Rectangle cara = { -lado * 0.5f, -lado * 0.5f, lado, lado };

    DrawRectangleRounded(
        Rectangle{ cara.x - borde + Px(3.0f), cara.y - borde + Px(5.0f), cara.width + borde * 2.0f, cara.height + borde * 2.0f },
        0.16f, 6, Fade(BLACK, 0.30f * alfa)
    );
    DrawRectangleRounded(
        Rectangle{ cara.x - borde, cara.y - borde, cara.width + borde * 2.0f, cara.height + borde * 2.0f },
        0.18f, 6, Fade(UI_TINTA, alfa)
    );
    DrawRectangleRounded(cara, 0.16f, 6, Fade(COLOR_CARA_EVENTO, alfa));

    if (conBanda)
    {
        DrawRectangleRounded(
            Rectangle{ cara.x + lado * 0.07f, cara.y + lado * 0.07f, lado * 0.86f, lado * 0.17f },
            0.5f, 6, Fade(banda, alfa)
        );
    }

    if (resaltar)
    {
        float halo = Px(5.0f);

        DrawRectangleRoundedLinesEx(
            Rectangle{ cara.x - halo, cara.y - halo, cara.width + halo * 2.0f, cara.height + halo * 2.0f },
            0.2f, 6, Px(UI_BORDE_FOCO), Fade(UI_FOCO, alfa)
        );
    }

    float anchoMax = lado * 0.86f;
    int tamano = TamanoTextoAjustadoUI(grande, tokenGrande, anchoMax);
    float yGrande = conBanda ? cara.y + lado * 0.30f : cara.y + lado * 0.20f;

    if (chico == nullptr || chico[0] == '\0')
    {
        yGrande = cara.y + (lado - (float)tamano) * 0.5f + (conBanda ? lado * 0.08f : 0.0f);
    }

    DibujarTextoAjustadoUI(grande, 0.0f, yGrande, tokenGrande, anchoMax, Fade(UI_TINTA, alfa), true);

    if (chico != nullptr && chico[0] != '\0')
    {
        float yChico = cara.y + lado * 0.64f;

        if (icono != 0)
        {
            // Icono + texto centrados como un solo bloque (la matriz ya
            // mueve el origen al centro del dado).
            float anchoIcono = Px(icono == 1 ? 24.0f : 26.0f);
            float separacion = Px(5.0f);
            int tamanoChico = TamanoTextoAjustadoUI(chico, UI_TEXTO_GRANDE, anchoMax - anchoIcono - separacion);
            float anchoTexto = (float)MeasureText(chico, tamanoChico);
            float izquierda = -(anchoIcono + separacion + anchoTexto) * 0.5f;

            Vector2 posIcono = { izquierda + anchoIcono * 0.5f, yChico + (float)tamanoChico * 0.5f };

            if (icono == 1)
            {
                DibujarMonedaUI(posIcono, 12.0f, alfa);
            }
            else
            {
                DibujarTrofeoUI(posIcono, 26.0f, alfa);
            }

            DibujarTextoAjustadoUI(
                chico, izquierda + anchoIcono + separacion, yChico, UI_TEXTO_GRANDE,
                anchoMax - anchoIcono - separacion, Fade(UI_TINTA, alfa), false
            );
        }
        else
        {
            DibujarTextoAjustadoUI(chico, 0.0f, yChico, UI_TEXTO_PEQUENO, anchoMax, Fade(UI_TINTA_SUAVE, alfa), true);
        }
    }

    rlPopMatrix();
}


static void DibujarIntercambio2D(const PartidaTablero& partida, const EstiloUI& estilo, float alfa)
{
    const EventoInteractivo& evento = partida.evento;
    float t = estilo.tiempo;
    bool reducir = estilo.reducirMovimiento;

    DibujarTituloEvento(
        "¡INTERCAMBIO!",
        "TRES DADOS DECIDEN QUIÉN, QUÉ Y CON QUIÉN",
        UI_FOCO,
        alfa
    );

    int lista[MAX_PARTICIPANTES];
    int cantidadJugadores = ListarJugadoresEvento(partida, lista);

    const char* cabeceras[3] = { "PRIMERA PERSONA", "QUÉ PASA", "SEGUNDA PERSONA" };
    float xCentros[3] = { 450.0f, 640.0f, 830.0f };
    float lado = Px(150.0f);

    for (int d = 0; d < 3; d++)
    {
        int pasoDado = PASO_INTERCAMBIO_DADO_1 + d;
        bool resuelto = evento.paso > pasoDado;
        bool tirando = evento.paso == pasoDado && evento.tiempoPaso < DURACION_TIRADA_DADO;
        bool visto = (evento.paso == pasoDado && !tirando) || resuelto;
        bool pendiente = !tirando && !visto;
        bool actual = evento.paso == pasoDado;

        Vector2 centro = PuntoRefEvento(xCentros[d], 350.0f);

        DibujarTextoMundoContornoUI(
            cabeceras[d], centro.x, PuntoRefEvento(0.0f, 243.0f).y, UI_TEXTO_PEQUENO,
            pendiente ? Fade(UI_TEXTO_MUNDO, 0.6f) : UI_TEXTO_MUNDO, alfa
        );

        float giro = 0.0f;
        float escala = 1.0f;

        if (tirando && !reducir)
        {
            giro = 9.0f * std::sin(t * 26.0f);
            centro.y -= Px(10.0f) * std::sin(t * 13.0f);
        }
        else if (actual && visto && !reducir)
        {
            float rebote = 1.0f - Limitar01Evento((evento.tiempoPaso - DURACION_TIRADA_DADO) / 0.25f);
            escala = 1.0f + 0.18f * rebote * rebote;
        }

        float ladoCarta = lado * escala;

        // Cara que se muestra: al azar mientras gira, la real despues.
        int cara = (int)(t * 14.0f) + d * 3;

        if (d == 1)
        {
            if (pendiente)
            {
                DibujarCartaDadoEvento(centro, ladoCarta, giro, UI_MADERA_MEDIA, false, "?", UI_TEXTO_TITULO, nullptr, 0, false, alfa * 0.75f);
            }
            else if (tirando)
            {
                const char* palabras[3] = { "DA", "RECIBE", "CAMBIA" };
                DibujarCartaDadoEvento(centro, ladoCarta, giro, UI_MADERA_MEDIA, false, palabras[cara % 3], UI_TEXTO_GRANDE, nullptr, 0, false, alfa);
            }
            else
            {
                const char* palabras[3] = { "DA", "RECIBE", "CAMBIA" };
                char chico[32] = "";
                int icono = evento.recurso == RECURSO_INTERCAMBIO_MONEDAS ? 1 : 2;

                if (evento.accion == INTERCAMBIO_CAMBIA)
                {
                    std::snprintf(chico, sizeof(chico), "%s", evento.recurso == RECURSO_INTERCAMBIO_MONEDAS ? "MONEDAS" : "TROFEOS");
                }
                else
                {
                    std::snprintf(chico, sizeof(chico), "%d", evento.cantidad);
                }

                DibujarCartaDadoEvento(
                    centro, ladoCarta, giro, UI_MADERA_MEDIA, false,
                    palabras[evento.accion], UI_TEXTO_GRANDE, chico, icono, actual, alfa
                );
            }
        }
        else
        {
            int persona = d == 0 ? evento.personaA : evento.personaB;

            if (pendiente || persona < 0)
            {
                DibujarCartaDadoEvento(centro, ladoCarta, giro, UI_MADERA_MEDIA, false, "?", UI_TEXTO_TITULO, nullptr, 0, false, alfa * 0.75f);
            }
            else
            {
                int mostrada = tirando && cantidadJugadores > 0 ? lista[cara % cantidadJugadores] : persona;

                DibujarCartaDadoEvento(
                    centro, ladoCarta, giro, ColorJugadorEvento(partida, mostrada), true,
                    TextFormat("J%d", mostrada + 1), UI_TEXTO_TITULO, nullptr, 0, actual && !tirando, alfa
                );
            }
        }
    }

    if (evento.paso == PASO_INTERCAMBIO_RESUMEN)
    {
        DibujarBandaEvento(estilo, 462.0f, evento.resumen, Limitar01Evento(evento.tiempoPaso / 0.2f));
    }
}


// Objeto misterioso del arbol: estado 0 = oculto ("?"), 1 = bueno, 2 = malo.
static void DibujarObjetoArbol(
    Vector2 centro,
    float radio,
    int estado,
    float tiempo,
    float alfa
)
{
    // Sombra en el suelo.
    DrawEllipse((int)centro.x, (int)(centro.y + radio * 1.05f), radio * 0.9f, radio * 0.2f, Fade(BLACK, 0.30f * alfa));

    Color cuerpo = Color{ 176, 128, 80, 255 };
    Color luz = Color{ 214, 170, 112, 255 };

    if (estado == 1)
    {
        cuerpo = Color{ 255, 200, 60, 255 };
        luz = Color{ 255, 236, 150, 255 };

        float halo = 0.5f + 0.5f * std::sin(tiempo * 6.0f);
        DrawCircleV(centro, radio * (1.25f + 0.08f * halo), Fade(Color{ 130, 235, 150, 255 }, 0.30f * alfa));
    }
    else if (estado == 2)
    {
        cuerpo = Color{ 92, 48, 112, 255 };
        luz = Color{ 140, 84, 160, 255 };

        // Pinchos.
        for (int i = 0; i < 10; i++)
        {
            float a = (float)i * 36.0f * PI_EVENTO / 180.0f;
            Vector2 base1 = { centro.x + std::cos(a - 0.16f) * radio * 0.92f, centro.y + std::sin(a - 0.16f) * radio * 0.92f };
            Vector2 base2 = { centro.x + std::cos(a + 0.16f) * radio * 0.92f, centro.y + std::sin(a + 0.16f) * radio * 0.92f };
            Vector2 punta = { centro.x + std::cos(a) * radio * 1.28f, centro.y + std::sin(a) * radio * 1.28f };

            DrawTriangle(base1, base2, punta, Fade(Color{ 60, 28, 76, 255 }, alfa));
            DrawTriangle(base2, base1, punta, Fade(Color{ 60, 28, 76, 255 }, alfa));
        }
    }

    DrawCircleV(centro, radio + Px(3.0f), Fade(UI_TINTA, alfa));
    DrawCircleV(centro, radio, Fade(cuerpo, alfa));
    DrawCircleV(Vector2{ centro.x - radio * 0.28f, centro.y - radio * 0.30f }, radio * 0.38f, Fade(luz, 0.75f * alfa));

    const char* simbolo = estado == 0 ? "?" : (estado == 1 ? TextFormat("+%d", OBJETO_BUENO_CASILLAS) : TextFormat("-%d", OBJETO_MALO_MONEDAS));
    int tamano = (int)(radio * (estado == 0 ? 1.1f : 0.8f));
    int ancho = MeasureText(simbolo, tamano);

    Color tinta = estado == 2 ? Color{ 250, 240, 255, 255 } : UI_TINTA;

    DrawText(simbolo, (int)(centro.x - (float)ancho * 0.5f), (int)(centro.y - (float)tamano * 0.5f), tamano, Fade(tinta, alfa));

    // Hojita del arbol sobre el objeto oculto.
    if (estado == 0)
    {
        DrawEllipse((int)(centro.x + radio * 0.1f), (int)(centro.y - radio * 1.02f), radio * 0.34f, radio * 0.16f, Fade(Color{ 84, 138, 70, 255 }, alfa));
    }
}


static void DibujarObjetos2D(const PartidaTablero& partida, const EstiloUI& estilo, float alfa)
{
    const EventoInteractivo& evento = partida.evento;
    float t = estilo.tiempo;
    bool reducir = estilo.reducirMovimiento;

    DibujarTituloEvento(
        TextFormat("¡%s TIRA DOS OBJETOS!", evento.fuente[0] != '\0' ? evento.fuente : "EL ÁRBOL"),
        evento.paso == PASO_OBJETOS_REVELA ? "UNO ERA BUENO Y EL OTRO MALO" : "UNO ES BUENO Y EL OTRO ES MALO: ¿CUÁL ATRAPÁS?",
        UI_FOCO,
        alfa
    );

    float radio = Px(54.0f);
    float xSlots[2] = { 470.0f, 810.0f };
    float yFinal = 352.0f;

    for (int lado = 0; lado < 2; lado++)
    {
        // Caida desde la copa (arriba, al centro) hasta su lugar.
        float u = evento.paso == PASO_OBJETOS_CAIDA
            ? Limitar01Evento(evento.tiempoPaso / DURACION_CAIDA_OBJETOS)
            : 1.0f;

        float caida = u * u;
        float xRef = 640.0f + (xSlots[lado] - 640.0f) * (1.0f - (1.0f - u) * (1.0f - u));
        float yRef = -60.0f + (yFinal + 60.0f) * caida;

        if (!reducir && u >= 1.0f && evento.paso == PASO_OBJETOS_ELIGE)
        {
            yRef += 5.0f * std::sin(t * 3.2f + (float)lado * 1.7f);
        }

        Vector2 centro = PuntoRefEvento(xRef, yRef);
        bool elegido = evento.ladoElegido == lado;
        bool revelado = evento.paso == PASO_OBJETOS_REVELA;
        bool resaltado = evento.paso == PASO_OBJETOS_ELIGE && evento.ladoResaltado == lado;

        int estado = 0;
        float alfaObjeto = alfa;
        float escala = 1.0f;

        if (revelado)
        {
            bool bueno = lado == evento.ladoBueno;

            // El atrapado se destapa enseguida; el otro un instante despues.
            float retraso = elegido ? 0.0f : 0.7f;

            if (evento.tiempoPaso >= retraso)
            {
                estado = bueno ? 1 : 2;
            }

            if (!elegido)
            {
                alfaObjeto = alfa * 0.65f;
            }
            else if (!reducir)
            {
                float rebote = 1.0f - Limitar01Evento(evento.tiempoPaso / 0.3f);
                escala = 1.0f + 0.35f * rebote * rebote;
            }
        }
        else if (resaltado && !reducir)
        {
            escala = 1.0f + 0.05f * std::sin(t * 8.0f);
        }

        DibujarObjetoArbol(centro, radio * escala, estado, t, alfaObjeto);

        if (resaltado)
        {
            float mitadAncho = radio * 1.3f;
            DibujarMarcoFoco(Rectangle{ centro.x - mitadAncho, centro.y - radio * 1.45f, mitadAncho * 2.0f, radio * 2.75f });
        }

        // Etiqueta bajo el objeto.
        const char* etiqueta = lado == 0 ? "IZQUIERDA" : "DERECHA";
        Color colorEtiqueta = UI_TEXTO_MUNDO;

        if (revelado && estado != 0)
        {
            etiqueta = estado == 1 ? "BUENO" : "MALO";
            colorEtiqueta = estado == 1 ? Color{ 130, 235, 150, 255 } : Color{ 255, 120, 100, 255 };
        }

        DibujarTextoMundoContornoUI(
            etiqueta, centro.x, centro.y + radio * 1.55f, UI_TEXTO_PEQUENO, colorEtiqueta, alfaObjeto
        );
    }

    if (evento.paso == PASO_OBJETOS_REVELA)
    {
        DibujarBandaEvento(estilo, 492.0f, evento.resumen, Limitar01Evento(evento.tiempoPaso / 0.2f));
    }
}


void DibujarEventoInteractivo2D(const PartidaTablero& partida)
{
    const EventoInteractivo& evento = partida.evento;

    if (
        !evento.activo ||
        partida.fase != FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO
    )
    {
        return;
    }

    const EstiloUI& estilo = ObtenerEstiloUI();

    if (!estilo.cargado)
    {
        return;
    }

    float entrada = estilo.reducirMovimiento ? 1.0f : Limitar01Evento(evento.tiempoTotal / 0.25f);

    if (evento.tipo == EVENTO_INTERACTIVO_ABEJAS)
    {
        DibujarTituloEvento("¡ABEJAS!", evento.resumen, UI_FOCO, entrada);
        return;
    }

    DibujarVeloUI(0.58f * entrada);

    switch (evento.tipo)
    {
        case EVENTO_INTERACTIVO_CAOS:
            DibujarCaos2D(partida, estilo, entrada);
            break;

        case EVENTO_INTERACTIVO_INTERCAMBIO:
            DibujarIntercambio2D(partida, estilo, entrada);
            break;

        case EVENTO_INTERACTIVO_OBJETOS_ARBOL:
            DibujarObjetos2D(partida, estilo, entrada);
            break;

        case EVENTO_INTERACTIVO_TIENDA:
            DibujarTienda2D(partida, evento.tienda, evento.jugador, evento.tiempoTotal);
            break;

        case EVENTO_INTERACTIVO_ABEJAS:
        case EVENTO_INTERACTIVO_NINGUNO:
            break;
    }
}


//==================================================
// DIBUJO 3D: ABEJAS Y COFRE DEL REGALO
//==================================================

static void DibujarAbeja3D(Vector3 posicion, float tiempo, int indice)
{
    // Mas grandes que una abeja real: desde la camara del tablero deben
    // leerse como un enjambre.
    const float e = 1.9f;

    // Aleteo: dos alas finas que cabecean muy rapido.
    float aleteo = std::sin(tiempo * 70.0f + (float)indice * 1.3f);
    float ala = (0.05f + 0.045f * aleteo) * e;

    DrawCube(Vector3{ posicion.x - 0.05f * e, posicion.y + 0.07f * e, posicion.z }, 0.02f * e, ala, 0.11f * e, Color{ 245, 250, 255, 220 });
    DrawCube(Vector3{ posicion.x + 0.05f * e, posicion.y + 0.07f * e, posicion.z }, 0.02f * e, ala, 0.11f * e, Color{ 245, 250, 255, 220 });

    // Cuerpo con franjas.
    DrawSphereEx(posicion, 0.09f * e, 8, 8, Color{ 255, 214, 30, 255 });
    DrawSphereEx(Vector3{ posicion.x, posicion.y, posicion.z + 0.07f * e }, 0.075f * e, 8, 8, Color{ 30, 20, 16, 255 });
    DrawSphereEx(Vector3{ posicion.x, posicion.y, posicion.z - 0.06f * e }, 0.07f * e, 8, 8, Color{ 30, 20, 16, 255 });
}


static void DibujarAbejas3D(const PartidaTablero& partida)
{
    const EventoInteractivo& evento = partida.evento;

    if (evento.jugador < 0)
    {
        return;
    }

    Vector3 ficha = partida.jugadores[evento.jugador].posicionVisual;
    float t = partida.tiempoTablero;

    // Nube: llega girando desde arriba, rodea a la ficha mientras la
    // lleva y se dispersa al soltarla.
    float llegada = 1.0f;
    float dispersion = 0.0f;

    if (evento.paso == PASO_ABEJAS_LLEGAN)
    {
        llegada = Limitar01Evento(evento.tiempoPaso / DURACION_ABEJAS_LLEGAN);
        llegada = llegada * llegada * (3.0f - 2.0f * llegada);
    }
    else if (evento.paso == PASO_ABEJAS_SUELTAN)
    {
        dispersion = Limitar01Evento(evento.tiempoPaso / DURACION_ABEJAS_SUELTAN);
    }

    const int cantidad = 16;

    for (int i = 0; i < cantidad; i++)
    {
        float fase = (float)i * 2.399f;
        float velocidad = 2.4f + 0.18f * (float)(i % 5);
        float angulo = t * velocidad + fase;
        float radio = (0.55f + 0.09f * (float)(i % 4)) * (1.0f + dispersion * 3.0f);
        float altura = 0.75f + 0.35f * std::sin(t * 3.0f + fase) + dispersion * 2.2f;

        Vector3 posicion =
        {
            ficha.x + std::cos(angulo) * radio,
            ficha.y + altura,
            ficha.z + std::sin(angulo) * radio
        };

        // Al llegar vienen de lejos y de mas arriba.
        float lejania = 1.0f - llegada;

        posicion.x += 5.0f * lejania * std::cos(fase);
        posicion.y += 5.0f * lejania;
        posicion.z += 5.0f * lejania * std::sin(fase);

        DibujarAbeja3D(posicion, t, i);
    }
}


// Cofre del REGALO: aparece frente a la ficha, abre la tapa y suelta
// monedas. Dura lo mismo que la fase del evento de casilla.
static void DibujarCofreRegalo3D(const PartidaTablero& partida)
{
    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    if (
        turno < 0 ||
        partida.fase != FASE_PARTIDA_TABLERO_EVENTO_CASILLA ||
        partida.tiempoFase <= 0.0f
    )
    {
        return;
    }

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(partida.jugadores[turno].casillaActual);

    if (casilla == nullptr || casilla->tipo != CASILLA_REGALO)
    {
        return;
    }

    float t = partida.tiempoFase;
    float aparicion = Limitar01Evento(t / 0.25f);
    float escala = aparicion * (2.0f - aparicion);
    float apertura = Limitar01Evento((t - 0.25f) / 0.30f);

    Vector3 base = { casilla->posicion.x, casilla->posicion.y + 0.62f, casilla->posicion.z + 1.15f };
    Color madera = { 150, 96, 52, 255 };
    Color metal = { 236, 190, 70, 255 };

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlScalef(escala, escala, escala);

    // Cuerpo y tapa (la tapa gira sobre su borde de atras).
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 0.80f, 0.42f, 0.54f, madera);
    DrawCubeWires(Vector3{ 0.0f, 0.0f, 0.0f }, 0.80f, 0.42f, 0.54f, Color{ 58, 36, 24, 255 });
    DrawCube(Vector3{ 0.0f, 0.0f, 0.28f }, 0.10f, 0.44f, 0.03f, metal);

    rlPushMatrix();
    rlTranslatef(0.0f, 0.21f, -0.27f);
    rlRotatef(-105.0f * apertura, 1.0f, 0.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.07f, 0.27f }, 0.84f, 0.14f, 0.58f, Color{ 168, 108, 60, 255 });
    DrawCubeWires(Vector3{ 0.0f, 0.07f, 0.27f }, 0.84f, 0.14f, 0.58f, Color{ 58, 36, 24, 255 });
    rlPopMatrix();

    rlPopMatrix();

    // Monedas que saltan del cofre.
    if (apertura >= 1.0f)
    {
        for (int i = 0; i < 6; i++)
        {
            float tm = t - 0.55f - 0.06f * (float)i;

            if (tm < 0.0f || tm > 0.9f)
            {
                continue;
            }

            float u = tm / 0.9f;
            float lado = ((float)i - 2.5f) * 0.16f;
            Vector3 pos =
            {
                base.x + lado * (0.4f + u),
                base.y + 0.3f + 1.5f * u * (1.0f - u) * 2.0f,
                base.z + 0.05f * (float)(i % 3)
            };

            DrawCylinder(pos, 0.13f, 0.13f, 0.035f, 10, Color{ 255, 214, 64, 255 });
            DrawCylinderWires(pos, 0.13f, 0.13f, 0.035f, 10, Color{ 160, 110, 20, 255 });
        }
    }
}


void DibujarEventosCasillas3D(const PartidaTablero& partida)
{
    if (
        partida.evento.activo &&
        partida.fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO &&
        partida.evento.tipo == EVENTO_INTERACTIVO_ABEJAS
    )
    {
        DibujarAbejas3D(partida);
    }

    DibujarCofreRegalo3D(partida);
}
