#include "Gameplay/DueloTablero.h"

#include "Gameplay/PartidaTablero.h"
#include "UI/EstiloUI.h"
#include "UI/EstiloUIGlobal.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>


//==================================================
// UTILIDADES
//==================================================

static const float TIEMPO_INACTIVIDAD_DUELO = 20.0f;
static const float DURACION_ANUNCIO_DUELO = 1.9f;
static const float DURACION_CANCELADO_DUELO = 2.2f;
static const float DURACION_RESULTADO_DUELO = 4.5f;

static const int APUESTAS_MONEDAS_DUELO[3] = { 5, 10, 20 };


static float Limitar01Duelo(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static int MinimoDuelo(int a, int b)
{
    return a < b ? a : b;
}


static Color ColorJugadorDuelo(const PartidaTablero& partida, int jugador)
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


//==================================================
// REGLAS
//==================================================

int ListarOpcionesApuestaDuelo(
    const PartidaTablero& partida,
    int a,
    int b,
    OpcionApuestaDuelo opciones[]
)
{
    int cantidad = 0;

    if (
        a < 0 || a >= MAX_PARTICIPANTES ||
        b < 0 || b >= MAX_PARTICIPANTES ||
        a == b
    )
    {
        return 0;
    }

    const EstadoJugadorPartidaTablero& ja = partida.jugadores[a];
    const EstadoJugadorPartidaTablero& jb = partida.jugadores[b];
    int mas_pobre = MinimoDuelo(ja.monedas, jb.monedas);

    for (int i = 0; i < 3; i++)
    {
        int nominal = APUESTAS_MONEDAS_DUELO[i];
        int cantidadReal = MinimoDuelo(nominal, mas_pobre);

        if (cantidadReal <= 0)
        {
            continue;
        }

        // Tras el tope puede repetirse el valor: se muestra una sola vez.
        bool repetida = false;

        for (int j = 0; j < cantidad; j++)
        {
            if (opciones[j].tipo == APUESTA_DUELO_MONEDAS && opciones[j].cantidad == cantidadReal)
            {
                repetida = true;
            }
        }

        if (repetida)
        {
            continue;
        }

        OpcionApuestaDuelo opcion;
        opcion.tipo = APUESTA_DUELO_MONEDAS;
        opcion.cantidad = cantidadReal;
        opcion.limitada = cantidadReal < nominal;
        opciones[cantidad++] = opcion;
    }

    if (ja.trofeos >= 1 && jb.trofeos >= 1)
    {
        OpcionApuestaDuelo opcion;
        opcion.tipo = APUESTA_DUELO_TROFEO;
        opcion.cantidad = 1;
        opciones[cantidad++] = opcion;
    }

    return cantidad;
}


int ListarRivalesDuelo(
    const PartidaTablero& partida,
    int retador,
    int rivales[]
)
{
    int cantidad = 0;

    for (int i = 0; i < partida.cantidadJugadores && cantidad < 3; i++)
    {
        int indice = partida.ordenParticipantes[i];

        if (indice < 0 || indice == retador)
        {
            continue;
        }

        OpcionApuestaDuelo opciones[MAX_OPCIONES_APUESTA_DUELO];

        if (ListarOpcionesApuestaDuelo(partida, retador, indice, opciones) > 0)
        {
            rivales[cantidad++] = indice;
        }
    }

    // Por numero de jugador: la lista del selector no depende del turno.
    for (int a = 0; a < cantidad; a++)
    {
        for (int b = a + 1; b < cantidad; b++)
        {
            if (rivales[b] < rivales[a])
            {
                int temporal = rivales[a];
                rivales[a] = rivales[b];
                rivales[b] = temporal;
            }
        }
    }

    return cantidad;
}


int DecidirRivalDueloBot(
    const PartidaTablero& partida,
    int retador,
    const int rivales[],
    int cantidad
)
{
    if (cantidad <= 0)
    {
        return -1;
    }

    const EstadoJugadorPartidaTablero& yo = partida.jugadores[retador];

    // Lider: mas trofeos y, a igualdad, mas monedas.
    int lider = rivales[0];

    for (int i = 1; i < cantidad; i++)
    {
        const EstadoJugadorPartidaTablero& candidato = partida.jugadores[rivales[i]];
        const EstadoJugadorPartidaTablero& actual = partida.jugadores[lider];

        if (
            candidato.trofeos > actual.trofeos ||
            (candidato.trofeos == actual.trofeos && candidato.monedas > actual.monedas)
        )
        {
            lider = rivales[i];
        }
    }

    const EstadoJugadorPartidaTablero& ellider = partida.jugadores[lider];

    if (
        ellider.trofeos > yo.trofeos ||
        (ellider.trofeos == yo.trofeos && ellider.monedas >= yo.monedas)
    )
    {
        return lider;
    }

    // Yo voy primero: el mas parejo en monedas.
    int masParejo = rivales[0];

    for (int i = 1; i < cantidad; i++)
    {
        if (
            std::abs(partida.jugadores[rivales[i]].monedas - yo.monedas) <
            std::abs(partida.jugadores[masParejo].monedas - yo.monedas)
        )
        {
            masParejo = rivales[i];
        }
    }

    return masParejo;
}


int DecidirApuestaDueloBot(
    const PartidaTablero& partida,
    int retador,
    int rival,
    const OpcionApuestaDuelo opciones[],
    int cantidad
)
{
    if (cantidad <= 0)
    {
        return -1;
    }

    const EstadoJugadorPartidaTablero& yo = partida.jugadores[retador];
    const EstadoJugadorPartidaTablero& otro = partida.jugadores[rival];

    // Va por detras en trofeos y todavia no le alcanza para uno: apuesta
    // el trofeo para recortar.
    for (int i = 0; i < cantidad; i++)
    {
        if (
            opciones[i].tipo == APUESTA_DUELO_TROFEO &&
            otro.trofeos > yo.trofeos &&
            yo.monedas < partida.costoTrofeo
        )
        {
            return i;
        }
    }

    // Si no, la apuesta de monedas mas cercana al 40 % de las suyas.
    int objetivo = (yo.monedas * 4) / 10;
    int mejor = -1;
    int mejorDiferencia = 1000000;

    for (int i = 0; i < cantidad; i++)
    {
        if (opciones[i].tipo != APUESTA_DUELO_MONEDAS)
        {
            continue;
        }

        int diferencia = std::abs(opciones[i].cantidad - objetivo);

        if (diferencia < mejorDiferencia)
        {
            mejorDiferencia = diferencia;
            mejor = i;
        }
    }

    return mejor >= 0 ? mejor : 0;
}


void DescribirApuestaDuelo(
    const DueloTablero& duelo,
    char* destino,
    int capacidad
)
{
    if (duelo.apuesta == APUESTA_DUELO_TROFEO)
    {
        std::snprintf(destino, capacidad, "J%d VS J%d POR 1 TROFEO", duelo.retador + 1, duelo.rival + 1);
    }
    else
    {
        std::snprintf(destino, capacidad, "J%d VS J%d POR %d MONEDAS", duelo.retador + 1, duelo.rival + 1, duelo.cantidad);
    }
}


void IniciarDueloTablero(
    PartidaTablero& partida,
    int retador,
    int rival,
    OpcionApuestaDuelo apuesta
)
{
    DueloTablero& duelo = partida.duelo;

    duelo = DueloTablero{};
    duelo.activo = true;
    duelo.retador = retador;
    duelo.rival = rival;
    duelo.apuesta = apuesta.tipo;
    duelo.cantidad = apuesta.cantidad;

    partida.fase = FASE_PARTIDA_TABLERO_DUELO_PENDIENTE;
    partida.tiempoFase = 0.0f;
}


void ResolverDueloTablero(
    PartidaTablero& partida,
    const ResultadoMinijuego* resultado
)
{
    DueloTablero& duelo = partida.duelo;

    duelo.ganador = -1;
    duelo.perdedor = -1;
    duelo.pagado = 0;
    duelo.empate = false;
    duelo.cancelado = false;
    duelo.tiempo = 0.0f;

    if (!duelo.activo || duelo.retador < 0 || duelo.rival < 0)
    {
        duelo.cancelado = true;
    }
    else if (resultado == nullptr || !ResultadoMinijuegoFinalizado(*resultado))
    {
        duelo.cancelado = true;
    }
    else
    {
        bool gana1 = ParticipanteEsGanador(*resultado, duelo.retador);
        bool gana2 = ParticipanteEsGanador(*resultado, duelo.rival);

        if (gana1 == gana2)
        {
            duelo.empate = true;
        }
        else
        {
            duelo.ganador = gana1 ? duelo.retador : duelo.rival;
            duelo.perdedor = gana1 ? duelo.rival : duelo.retador;
        }
    }

    char texto[112] = "";

    if (duelo.cancelado)
    {
        std::snprintf(texto, sizeof(texto), "DUELO CANCELADO: NADIE PAGA");
    }
    else if (duelo.empate)
    {
        std::snprintf(texto, sizeof(texto), "¡EMPATE! SE DEVUELVE LA APUESTA: NADIE PAGA");
    }
    else
    {
        EstadoJugadorPartidaTablero& ganador = partida.jugadores[duelo.ganador];
        EstadoJugadorPartidaTablero& perdedor = partida.jugadores[duelo.perdedor];

        if (duelo.apuesta == APUESTA_DUELO_TROFEO)
        {
            if (perdedor.trofeos >= 1)
            {
                perdedor.trofeos--;
                ganador.trofeos++;
                duelo.pagado = 1;
            }

            std::snprintf(
                texto, sizeof(texto), "¡J%d GANA EL DUELO Y SE LLEVA 1 TROFEO DE J%d!",
                duelo.ganador + 1, duelo.perdedor + 1
            );
        }
        else
        {
            int pago = MinimoDuelo(duelo.cantidad, perdedor.monedas);

            perdedor.monedas -= pago;
            ganador.monedas += pago;
            duelo.pagado = pago;

            std::snprintf(
                texto, sizeof(texto), "¡J%d GANA EL DUELO Y SE LLEVA %d MONEDAS DE J%d!",
                duelo.ganador + 1, pago, duelo.perdedor + 1
            );
        }

        ReproducirSonidoTablero(partida, duelo.apuesta == APUESTA_DUELO_TROFEO ? SONIDO_COMPRA : SONIDO_MONEDA);
    }

    std::snprintf(duelo.resumen, sizeof(duelo.resumen), "%s", texto);

    duelo.activo = false;
    partida.fase = FASE_PARTIDA_TABLERO_DUELO_RESULTADO;
    partida.tiempoFase = 0.0f;
    ReproducirSonidoTablero(partida, duelo.cancelado || duelo.empate ? SONIDO_UI_CANCELAR : SONIDO_RESULTADO);
}


//==================================================
// SELECCION
//==================================================

void IniciarSeleccionDuelo(
    PartidaTablero& partida,
    int retador,
    EstadoDueloSeleccion& duelo
)
{
    duelo = EstadoDueloSeleccion{};
    duelo.retador = retador;
    duelo.cantidadRivales = ListarRivalesDuelo(partida, retador, duelo.rivales);
    duelo.proximaAccionBot = 0.9f + 0.5f * (float)GetRandomValue(0, 100) / 100.0f;

    IdMinijuego pool[MAX_PARTICIPANTES * 4];

    if (ObtenerMinijuegosDuelo(pool, MAX_PARTICIPANTES * 4) <= 0)
    {
        // Sin minijuegos de duelo en el catalogo no se puede apostar.
        std::snprintf(
            duelo.resumen, sizeof(duelo.resumen),
            "TODAVÍA NO HAY MINIJUEGOS DE DUELO: NADIE PIERDE NADA"
        );
        duelo.paso = PASO_DUELO_CANCELADO;
    }
    else if (duelo.cantidadRivales <= 0)
    {
        std::snprintf(
            duelo.resumen, sizeof(duelo.resumen),
            "J%d NO TIENE NADA PARA APOSTAR: DUELO CANCELADO", retador + 1
        );
        duelo.paso = PASO_DUELO_CANCELADO;
    }
    else if (duelo.cantidadRivales == 1)
    {
        // Un solo rival posible: se salta la eleccion de rival.
        duelo.rivalSeleccion = 0;
        duelo.cantidadOpciones = ListarOpcionesApuestaDuelo(partida, retador, duelo.rivales[0], duelo.opciones);
        duelo.paso = PASO_DUELO_APUESTA;
    }
}


static void AceptarSeleccionDuelo(
    PartidaTablero& partida,
    EstadoDueloSeleccion& duelo
)
{
    int rival = duelo.rivales[duelo.rivalSeleccion];
    const OpcionApuestaDuelo& apuesta = duelo.opciones[duelo.opcionSeleccion];

    if (apuesta.tipo == APUESTA_DUELO_TROFEO)
    {
        std::snprintf(
            duelo.resumen, sizeof(duelo.resumen),
            "¡J%d DESAFÍA A J%d POR 1 TROFEO!", duelo.retador + 1, rival + 1
        );
    }
    else
    {
        std::snprintf(
            duelo.resumen, sizeof(duelo.resumen),
            "¡J%d DESAFÍA A J%d POR %d MONEDAS!", duelo.retador + 1, rival + 1, apuesta.cantidad
        );
    }

    ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);
}




static void IrAPasoDuelo(EstadoDueloSeleccion& duelo, int paso)
{
    duelo.paso = paso;
    duelo.inicioPaso = duelo.reloj;
    duelo.tiempoSinAccion = 0.0f;
}


static void PasarAApuestaDuelo(
    PartidaTablero& partida,
    EstadoDueloSeleccion& duelo
)
{
    int rival = duelo.rivales[duelo.rivalSeleccion];

    duelo.cantidadOpciones =
        ListarOpcionesApuestaDuelo(partida, duelo.retador, rival, duelo.opciones);
    duelo.opcionSeleccion = 0;
    duelo.proximaAccionBot = 0.9f + 0.5f * (float)GetRandomValue(0, 100) / 100.0f;

    IrAPasoDuelo(duelo, PASO_DUELO_APUESTA);
}


static void CancelarSeleccionDuelo(
    PartidaTablero& partida,
    EstadoDueloSeleccion& duelo,
    const char* motivo
)
{
    std::snprintf(duelo.resumen, sizeof(duelo.resumen), "%s", motivo);
    ReproducirSonidoTablero(partida, SONIDO_UI_CANCELAR);
    IrAPasoDuelo(duelo, PASO_DUELO_CANCELADO);
}


bool ActualizarSeleccionDuelo(
    PartidaTablero& partida,
    EstadoDueloSeleccion& duelo,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
)
{
    duelo.reloj += deltaTime;
    duelo.tiempoSinAccion += deltaTime;

    float tiempoPaso = duelo.reloj - duelo.inicioPaso;
    bool confirmar = !esAutomatico && puedeDecidir && entrada.confirmar;

    char texto[112];

    switch (duelo.paso)
    {
        case PASO_DUELO_CANCELADO:
            return tiempoPaso >= DURACION_CANCELADO_DUELO || (confirmar && tiempoPaso >= 0.6f);

        case PASO_DUELO_ANUNCIO:
            if (tiempoPaso >= DURACION_ANUNCIO_DUELO || (confirmar && tiempoPaso >= 0.6f))
            {
                duelo.aceptado = true;
                return true;
            }
            return false;

        case PASO_DUELO_RIVAL:
            if (esAutomatico)
            {
                if (tiempoPaso >= duelo.proximaAccionBot)
                {
                    int rival = DecidirRivalDueloBot(partida, duelo.retador, duelo.rivales, duelo.cantidadRivales);

                    for (int i = 0; i < duelo.cantidadRivales; i++)
                    {
                        if (duelo.rivales[i] == rival)
                        {
                            duelo.rivalSeleccion = i;
                        }
                    }

                    PasarAApuestaDuelo(partida, duelo);
                }

                return false;
            }
            break;

        case PASO_DUELO_APUESTA:
            if (esAutomatico)
            {
                if (tiempoPaso >= duelo.proximaAccionBot)
                {
                    int indice = DecidirApuestaDueloBot(
                        partida, duelo.retador, duelo.rivales[duelo.rivalSeleccion],
                        duelo.opciones, duelo.cantidadOpciones
                    );

                    duelo.opcionSeleccion = indice < 0 ? 0 : indice;
                    AceptarSeleccionDuelo(partida, duelo);
                    IrAPasoDuelo(duelo, PASO_DUELO_ANUNCIO);
                }

                return false;
            }
            break;

        default:
            return true;
    }

    // Humano (RIVAL o APUESTA).
    if (duelo.tiempoSinAccion >= TIEMPO_INACTIVIDAD_DUELO)
    {
        std::snprintf(texto, sizeof(texto), "J%d NO RESPONDE: DUELO CANCELADO", duelo.retador + 1);
        CancelarSeleccionDuelo(partida, duelo, texto);
        return false;
    }

    if (!puedeDecidir)
    {
        return false;
    }

    int movimiento = 0;

    if (entrada.izquierda || entrada.arriba)
    {
        movimiento = -1;
    }
    else if (entrada.derecha || entrada.abajo)
    {
        movimiento = 1;
    }

    if (movimiento != 0)
    {
        duelo.tiempoSinAccion = 0.0f;
        ReproducirSonidoTablero(partida, SONIDO_UI_MOVER);

        if (duelo.paso == PASO_DUELO_RIVAL)
        {
            duelo.rivalSeleccion = (duelo.rivalSeleccion + duelo.cantidadRivales + movimiento) % duelo.cantidadRivales;
        }
        else if (duelo.cantidadOpciones > 0)
        {
            duelo.opcionSeleccion = (duelo.opcionSeleccion + duelo.cantidadOpciones + movimiento) % duelo.cantidadOpciones;
        }
    }

    if (entrada.cancelar)
    {
        if (duelo.paso == PASO_DUELO_APUESTA && duelo.cantidadRivales > 1)
        {
            ReproducirSonidoTablero(partida, SONIDO_UI_CANCELAR);
            IrAPasoDuelo(duelo, PASO_DUELO_RIVAL);
        }
        else
        {
            std::snprintf(texto, sizeof(texto), "J%d NO DESAFÍA A NADIE: DUELO CANCELADO", duelo.retador + 1);
            CancelarSeleccionDuelo(partida, duelo, texto);
        }

        return false;
    }

    if (confirmar && tiempoPaso >= 0.25f)
    {
        ReproducirSonidoTablero(partida, SONIDO_UI_CONFIRMAR);

        if (duelo.paso == PASO_DUELO_RIVAL)
        {
            PasarAApuestaDuelo(partida, duelo);
        }
        else
        {
            AceptarSeleccionDuelo(partida, duelo);
            IrAPasoDuelo(duelo, PASO_DUELO_ANUNCIO);
        }
    }

    return false;
}


void TextoSeleccionDuelo(
    const PartidaTablero& partida,
    const EstadoDueloSeleccion& duelo,
    char* destino,
    int capacidad
)
{
    bool bot = partida.participantes != nullptr && duelo.retador >= 0 && partida.participantes[duelo.retador].esBot;

    switch (duelo.paso)
    {
        case PASO_DUELO_RIVAL:
            std::snprintf(destino, capacidad, "%s", bot ? "EL BOT ELIGE A QUIÉN DESAFIAR..." : "¡DUELO! ELEGÍ A QUIÉN DESAFIAR");
            return;

        case PASO_DUELO_APUESTA:
            std::snprintf(destino, capacidad, "%s", bot ? "EL BOT ELIGE LA APUESTA..." : "ELEGÍ QUÉ APOSTAR (EL ESCUDO NO PROTEGE)");
            return;

        default:
            std::snprintf(destino, capacidad, "%s", duelo.resumen);
            return;
    }
}


//==================================================
// DIBUJO
//==================================================

static void DibujarCartaBase(Rectangle carta, bool resaltar, float alfa)
{
    DrawRectangleRounded(Rectangle{ carta.x + Px(3.0f), carta.y + Px(5.0f), carta.width, carta.height }, 0.12f, 6, Fade(BLACK, 0.3f * alfa));
    DrawRectangleRounded(carta, 0.12f, 6, Fade(UI_TINTA, alfa));
    DrawRectangleRounded(
        Rectangle{ carta.x + Px(3.0f), carta.y + Px(3.0f), carta.width - Px(6.0f), carta.height - Px(6.0f) },
        0.12f, 6, Fade(Color{ 255, 248, 232, 255 }, alfa)
    );

    if (resaltar)
    {
        float halo = Px(4.0f);

        DrawRectangleRoundedLinesEx(
            Rectangle{ carta.x - halo, carta.y - halo, carta.width + halo * 2.0f, carta.height + halo * 2.0f },
            0.14f, 6, Px(UI_BORDE_FOCO), Fade(UI_FOCO, alfa)
        );
    }
}


// Numero con su icono (moneda o trofeo) centrado en `centroX`.
static void DibujarCantidadDuelo(float centroX, float y, int cantidad, bool trofeo, int token, float alfa)
{
    const char* texto = TextFormat("%d", cantidad);
    int tamano = TamanoTexto(token);
    float anchoIcono = Px(trofeo ? 22.0f : 20.0f);
    float separacion = Px(5.0f);
    float anchoTexto = (float)MeasureText(texto, tamano);
    float izquierda = centroX - (anchoIcono + separacion + anchoTexto) * 0.5f;
    Vector2 icono = { izquierda + anchoIcono * 0.5f, y + (float)tamano * 0.5f };

    if (trofeo)
    {
        DibujarTrofeoUI(icono, 18.0f, alfa);
    }
    else
    {
        DibujarMonedaUI(icono, 8.0f, alfa);
    }

    DrawText(texto, (int)(izquierda + anchoIcono + separacion), (int)y, tamano, Fade(UI_TINTA, alfa));
}


static void DibujarCartaJugadorDuelo(
    const PartidaTablero& partida,
    Rectangle carta,
    int jugador,
    bool resaltar,
    float alfa
)
{
    DibujarCartaBase(carta, resaltar, alfa);

    DrawRectangleRounded(
        Rectangle{ carta.x + carta.width * 0.08f, carta.y + carta.height * 0.07f, carta.width * 0.84f, carta.height * 0.14f },
        0.5f, 6, Fade(ColorJugadorDuelo(partida, jugador), alfa)
    );

    DibujarTextoAjustadoUI(
        TextFormat("J%d", jugador + 1), carta.x + carta.width * 0.5f, carta.y + carta.height * 0.27f,
        UI_TEXTO_GRANDE, carta.width * 0.8f, Fade(UI_TINTA, alfa), true
    );

    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    DibujarCantidadDuelo(carta.x + carta.width * 0.5f, carta.y + carta.height * 0.58f, estado.monedas, false, UI_TEXTO, alfa);
    DibujarCantidadDuelo(carta.x + carta.width * 0.5f, carta.y + carta.height * 0.78f, estado.trofeos, true, UI_TEXTO, alfa);
}


static void DibujarBandaDuelo(const EstiloUI& estilo, float yRef, const char* texto, float alfa)
{
    if (alfa <= 0.0f)
    {
        return;
    }

    Rectangle rect = RectRef(290.0f, yRef, 700.0f, 52.0f);
    Rectangle placa = DibujarPlacaConMarcoUI(estilo, rect, 6.0f, alfa);
    int tamano = TamanoTextoAjustadoUI(texto, UI_TEXTO, placa.width - Px(24.0f));

    DibujarTextoAjustadoUI(
        texto, placa.x + placa.width * 0.5f, placa.y + (placa.height - (float)tamano) * 0.5f,
        UI_TEXTO, placa.width - Px(24.0f), Fade(UI_TINTA, alfa), true
    );
}


void DibujarSeleccionDuelo2D(
    const PartidaTablero& partida,
    const EstadoDueloSeleccion& duelo,
    float tiempoTotal
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    if (!estilo.cargado || duelo.retador < 0)
    {
        return;
    }

    bool humano = partida.participantes != nullptr && !partida.participantes[duelo.retador].esBot;
    float entrada = estilo.reducirMovimiento ? 1.0f : Limitar01Duelo(tiempoTotal / 0.25f);
    float cx = (float)GetScreenWidth() * 0.5f;
    float tiempoPaso = duelo.reloj - duelo.inicioPaso;

    DibujarVeloUI(0.58f * entrada);

    DibujarTextoMundoContornoUI("¡DUELO!", cx, RectRef(0.0f, 166.0f, 0.0f, 0.0f).y, UI_TEXTO_GRANDE, Color{ 255, 150, 90, 255 }, entrada);

    const char* subtitulo =
        duelo.paso == PASO_DUELO_RIVAL ? "ELEGÍ A QUIÉN DESAFIAR" :
        (duelo.paso == PASO_DUELO_APUESTA ? "ELEGÍ QUÉ APOSTAR" : "UN MINIJUEGO 1 VS 1 DECIDE EL DUELO");

    if (duelo.paso == PASO_DUELO_CANCELADO)
    {
        subtitulo = "NADIE PIERDE NADA";
    }

    DibujarTextoMundoContornoUI(subtitulo, cx, RectRef(0.0f, 202.0f, 0.0f, 0.0f).y, UI_TEXTO, UI_TEXTO_MUNDO, entrada);

    if (duelo.paso == PASO_DUELO_RIVAL)
    {
        int n = duelo.cantidadRivales;
        float separacion = 180.0f;
        float inicio = 640.0f - separacion * (float)(n - 1) * 0.5f;

        for (int r = 0; r < n; r++)
        {
            Rectangle carta = RectRef(inicio + separacion * (float)r - 72.0f, 250.0f, 144.0f, 150.0f);

            DibujarCartaJugadorDuelo(partida, carta, duelo.rivales[r], humano && r == duelo.rivalSeleccion, entrada);
        }

        DibujarTextoMundoContornoUI(
            TextFormat("TUS COSAS: %d MONEDAS Y %d %s", partida.jugadores[duelo.retador].monedas, partida.jugadores[duelo.retador].trofeos, partida.jugadores[duelo.retador].trofeos == 1 ? "TROFEO" : "TROFEOS"),
            cx, RectRef(0.0f, 424.0f, 0.0f, 0.0f).y, UI_TEXTO_PEQUENO, UI_TEXTO_MUNDO, entrada
        );
    }
    else if (duelo.paso == PASO_DUELO_APUESTA || duelo.paso == PASO_DUELO_ANUNCIO)
    {
        int rival = duelo.rivales[duelo.rivalSeleccion];

        // Los dos duelistas, con lo que tiene cada uno.
        Rectangle izquierda = RectRef(300.0f, 236.0f, 150.0f, 96.0f);
        Rectangle derecha = RectRef(830.0f, 236.0f, 150.0f, 96.0f);

        for (int lado = 0; lado < 2; lado++)
        {
            Rectangle carta = lado == 0 ? izquierda : derecha;
            int jugador = lado == 0 ? duelo.retador : rival;

            DibujarCartaBase(carta, false, entrada);
            DrawRectangleRounded(
                Rectangle{ carta.x + carta.width * 0.08f, carta.y + carta.height * 0.08f, carta.width * 0.84f, carta.height * 0.16f },
                0.5f, 6, Fade(ColorJugadorDuelo(partida, jugador), entrada)
            );
            DibujarTextoAjustadoUI(TextFormat("J%d", jugador + 1), carta.x + carta.width * 0.5f, carta.y + carta.height * 0.27f, UI_TEXTO, carta.width * 0.8f, Fade(UI_TINTA, entrada), true);
            DibujarCantidadDuelo(carta.x + carta.width * 0.5f, carta.y + carta.height * 0.52f, partida.jugadores[jugador].monedas, false, UI_TEXTO_PEQUENO, entrada);
            DibujarCantidadDuelo(carta.x + carta.width * 0.5f, carta.y + carta.height * 0.74f, partida.jugadores[jugador].trofeos, true, UI_TEXTO_PEQUENO, entrada);
        }

        DibujarTextoMundoContornoUI("VS", cx, RectRef(0.0f, 262.0f, 0.0f, 0.0f).y, UI_TEXTO_GRANDE, UI_FOCO, entrada);

        // Opciones de apuesta.
        int n = duelo.cantidadOpciones;
        float separacion = 160.0f;
        float inicio = 640.0f - separacion * (float)(n - 1) * 0.5f;

        for (int o = 0; o < n; o++)
        {
            const OpcionApuestaDuelo& opcion = duelo.opciones[o];
            Rectangle carta = RectRef(inicio + separacion * (float)o - 66.0f, 352.0f, 132.0f, 104.0f);
            bool elegida = o == duelo.opcionSeleccion;
            bool trofeo = opcion.tipo == APUESTA_DUELO_TROFEO;

            if (duelo.paso == PASO_DUELO_ANUNCIO && !elegida)
            {
                continue;
            }

            DibujarCartaBase(carta, humano && elegida && duelo.paso == PASO_DUELO_APUESTA, entrada);
            DibujarCantidadDuelo(carta.x + carta.width * 0.5f, carta.y + carta.height * 0.22f, opcion.cantidad, trofeo, UI_TEXTO_GRANDE, entrada);
            DibujarTextoAjustadoUI(
                trofeo ? "TROFEO" : (opcion.limitada ? "TOPE DEL MÁS POBRE" : "MONEDAS"),
                carta.x + carta.width * 0.5f, carta.y + carta.height * 0.68f,
                UI_TEXTO_PEQUENO - 2, carta.width * 0.88f, Fade(UI_TINTA_SUAVE, entrada), true
            );
        }

        if (duelo.paso == PASO_DUELO_APUESTA)
        {
            DibujarTextoMundoContornoUI(
                "EL ESCUDO NO PROTEGE EN UN DUELO: LA APUESTA ES VOLUNTARIA",
                cx, RectRef(0.0f, 478.0f, 0.0f, 0.0f).y, UI_TEXTO_PEQUENO, Color{ 255, 210, 140, 255 }, entrada
            );
        }
        else
        {
            DibujarBandaDuelo(estilo, 478.0f, duelo.resumen, Limitar01Duelo(tiempoPaso / 0.2f));
        }
    }
    else
    {
        DibujarBandaDuelo(estilo, 300.0f, duelo.resumen, Limitar01Duelo(tiempoPaso / 0.2f));
    }
}


//==================================================
// RESULTADO
//==================================================

void TextoDuelo(
    const PartidaTablero& partida,
    char* destino,
    int capacidad
)
{
    const DueloTablero& duelo = partida.duelo;

    destino[0] = '\0';

    if (partida.fase == FASE_PARTIDA_TABLERO_DUELO_PENDIENTE)
    {
        char apuesta[64];

        DescribirApuestaDuelo(duelo, apuesta, (int)sizeof(apuesta));
        std::snprintf(destino, capacidad, "¡DUELO! %s: SE SORTEA EL MINIJUEGO", apuesta);
    }
    else if (partida.fase == FASE_PARTIDA_TABLERO_DUELO_RESULTADO)
    {
        std::snprintf(destino, capacidad, "%s", duelo.resumen);
    }
}


void ActualizarDueloResultado(
    PartidaTablero& partida,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
)
{
    DueloTablero& duelo = partida.duelo;

    duelo.tiempo += deltaTime;

    bool confirmar = !esAutomatico && puedeDecidir && entrada.confirmar;

    if (duelo.tiempo >= DURACION_RESULTADO_DUELO || (confirmar && duelo.tiempo >= 0.8f))
    {
        partida.fase = FASE_PARTIDA_TABLERO_FIN_TURNO;
        partida.tiempoFase = 0.0f;
    }
}


void DibujarDueloResultado2D(const PartidaTablero& partida)
{
    const DueloTablero& duelo = partida.duelo;

    if (partida.fase != FASE_PARTIDA_TABLERO_DUELO_RESULTADO)
    {
        return;
    }

    const EstiloUI& estilo = ObtenerEstiloUI();

    if (!estilo.cargado)
    {
        return;
    }

    float entrada = estilo.reducirMovimiento ? 1.0f : Limitar01Duelo(duelo.tiempo / 0.25f);
    float cx = (float)GetScreenWidth() * 0.5f;
    bool sinGanador = duelo.cancelado || duelo.empate;

    DibujarVeloUI(0.55f * entrada);

    DibujarTextoMundoContornoUI(
        duelo.cancelado ? "DUELO CANCELADO" : (duelo.empate ? "¡EMPATE!" : TextFormat("¡GANA J%d!", duelo.ganador + 1)),
        cx, RectRef(0.0f, 166.0f, 0.0f, 0.0f).y, UI_TEXTO_GRANDE,
        sinGanador ? UI_TEXTO_MUNDO : Color{ 130, 235, 150, 255 }, entrada
    );

    if (!sinGanador)
    {
        // Los dos duelistas con lo que gana y pierde cada uno.
        for (int lado = 0; lado < 2; lado++)
        {
            int jugador = lado == 0 ? duelo.ganador : duelo.perdedor;
            bool gano = lado == 0;
            Rectangle carta = RectRef(lado == 0 ? 400.0f : 740.0f, 235.0f, 140.0f, 150.0f);

            DibujarCartaBase(carta, gano, entrada);
            DrawRectangleRounded(
                Rectangle{ carta.x + carta.width * 0.08f, carta.y + carta.height * 0.07f, carta.width * 0.84f, carta.height * 0.14f },
                0.5f, 6, Fade(ColorJugadorDuelo(partida, jugador), entrada)
            );
            DibujarTextoAjustadoUI(TextFormat("J%d", jugador + 1), carta.x + carta.width * 0.5f, carta.y + carta.height * 0.27f, UI_TEXTO_GRANDE, carta.width * 0.8f, Fade(UI_TINTA, entrada), true);

            const char* signo = gano ? "+" : "-";
            const char* texto = TextFormat("%s%d", signo, duelo.pagado);
            int tamano = TamanoTexto(UI_TEXTO_GRANDE);
            float anchoTexto = (float)MeasureText(texto, tamano);
            float anchoIcono = Px(26.0f);
            float izquierda = carta.x + (carta.width - anchoIcono - anchoTexto) * 0.5f;
            float y = carta.y + carta.height * 0.62f;
            Vector2 icono = { izquierda + anchoIcono * 0.4f, y + (float)tamano * 0.5f };

            if (duelo.apuesta == APUESTA_DUELO_TROFEO)
            {
                DibujarTrofeoUI(icono, 24.0f, entrada);
            }
            else
            {
                DibujarMonedaUI(icono, 11.0f, entrada);
            }

            DrawText(texto, (int)(izquierda + anchoIcono), (int)y, tamano, Fade(gano ? Color{ 20, 92, 48, 255 } : Color{ 160, 30, 24, 255 }, entrada));
        }
    }

    DibujarBandaDuelo(estilo, 430.0f, duelo.resumen, entrada);
}
