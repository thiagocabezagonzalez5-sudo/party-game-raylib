#include "Minigames/MinijuegoParejasGlaciar.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// PAREJAS GLACIARES
//==================================================
//
// Memoria de parejas por turnos sobre un tablero de 4x4 bloques de hielo.
// Cada jugador revela 2 bloques; si coinciden se los queda y repite. La
// pareja aurora vale 3 puntos. Cada 4 fallos seguidos el glaciar cruje y
// baraja 2 bloques ocultos.
//==================================================


static const float DURACION_PREPARACION_GLACIAR = 3.0f;
static const float DURACION_TURNO_GLACIAR = 6.0f;
static const float DURACION_REVELADO_GLACIAR = 1.0f;
static const float DURACION_CRUJIDO_GLACIAR = 1.3f;
static const float DURACION_PARTIDA_GLACIAR = 90.0f;
static const float SEPARACION_BLOQUES_GLACIAR = 1.9f;
static const float ALTURA_BLOQUE_GLACIAR = 0.9f;
static const int FALLOS_PARA_CRUJIR_GLACIAR = 4;

// Camara de juego (el ojo tambien sirve para la niebla del visual).
static const Vector3 OJO_GLACIAR = { 0.0f, 6.8f, 11.4f };
static const Vector3 MIRA_GLACIAR = { 0.0f, 2.4f, -2.2f };


// Del paquete GLB del glaciar solo se cargan las piezas jugables: bloques,
// cursor y simbolos (tramo seguido del enum hasta el tempano de cada
// jugador) y los fragmentos del crujido. El paisaje (mar, montanas,
// icebergs, cueva, fauna y aurora) se construye con primitivas propias.
static_assert(MODELO_GLACIAR_BLOQUE_OCULTO + 11 == MODELO_GLACIAR_TEMPANO,
    "Bloques, cursor, simbolos y tempano deben ir seguidos en el paquete Glaciar");

static void CargarPiezasJugablesGlaciar()
{
    // Mismas primitives por GLB que exige CargarPaqueteParejasGlaciarRetro3D.
    static const int mallasEsperadas[CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D] =
        { 2,3,3,4,4,2,1,1,1,1,1,1,1,2,3,3,3,4,3,2,2,2,2,2 };
    const int tramos[2][2] =
    {
        { MODELO_GLACIAR_BLOQUE_OCULTO, MODELO_GLACIAR_TEMPANO },
        { MODELO_GLACIAR_FRAGMENTO, MODELO_GLACIAR_FRAGMENTO }
    };

    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteParejasGlaciarRetro3D();

    for (int t = 0; t < 2; t++)
    {
        PaqueteModelosEscenarioRetro3D tramo = paquete;
        tramo.recursos += tramos[t][0];
        tramo.definiciones += tramos[t][0];
        tramo.cantidad = tramos[t][1] - tramos[t][0] + 1;
        CargarPaqueteModelosEscenarioRetro3D(tramo);

        for (int i = tramos[t][0]; i <= tramos[t][1]; i++)
        {
            RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];
            if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i]) continue;
            UnloadModel(recurso.modelo);
            recurso.modelo = {};
            recurso.cargado = false;
            TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
        }
    }
}


//==================================================
// LOGICA
//==================================================


static bool EsControlBotGlaciar(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static float PosicionXBloqueGlaciar(int indice)
{
    return ((float)(indice % COLUMNAS_GLACIAR) - 1.5f) * SEPARACION_BLOQUES_GLACIAR;
}


static float PosicionZBloqueGlaciar(int indice)
{
    return ((float)(indice / COLUMNAS_GLACIAR) - 1.5f) * SEPARACION_BLOQUES_GLACIAR;
}


static bool BloqueDisponibleGlaciar(const MinijuegoParejasGlaciar& minijuego, int indice)
{
    return
        indice >= 0 &&
        indice < CANTIDAD_BLOQUES_GLACIAR &&
        !minijuego.bloques[indice].emparejado &&
        !minijuego.bloques[indice].revelado;
}


static int SiguienteJugadorGlaciar(const MinijuegoParejasGlaciar& minijuego, int desde)
{
    for (int paso = 1; paso <= MAX_PARTICIPANTES; paso++)
    {
        int indice = (desde + paso) % MAX_PARTICIPANTES;

        if (minijuego.resultado.participantes[indice].participo)
        {
            return indice;
        }
    }

    return desde;
}


static void BarajarBloquesGlaciar(MinijuegoParejasGlaciar& minijuego)
{
    int simbolos[CANTIDAD_BLOQUES_GLACIAR]{};

    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        simbolos[i] = i / 2;
    }

    for (int i = CANTIDAD_BLOQUES_GLACIAR - 1; i > 0; i--)
    {
        int j = GetRandomValue(0, i);
        int temporal = simbolos[i];
        simbolos[i] = simbolos[j];
        simbolos[j] = temporal;
    }

    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        minijuego.bloques[i] = {};
        minijuego.bloques[i].simbolo = simbolos[i];
    }
}


static void OlvidarBloqueGlaciar(MinijuegoParejasGlaciar& minijuego, int indice)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        minijuego.memoriaBots[i][indice] = -1;
    }
}


static void IniciarTurnoGlaciar(MinijuegoParejasGlaciar& minijuego)
{
    minijuego.fase = FASE_GLACIAR_ELEGIR;
    minijuego.tiempoTurno = DURACION_TURNO_GLACIAR;
    minijuego.primera = -1;
    minijuego.segunda = -1;
    minijuego.accionPrevia = true;
    minijuego.botObjetivo = -1;
    minijuego.botTemporizador = 0.7f;
}


static void FinalizarGlaciar(MinijuegoParejasGlaciar& minijuego)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int ganadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorParejasGlaciar& estado = minijuego.estadosJugadores[i];
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (!minijuego.resultado.participantes[j].participo)
            {
                continue;
            }

            const EstadoJugadorParejasGlaciar& otro = minijuego.estadosJugadores[j];

            if (
                otro.puntos > estado.puntos ||
                (otro.puntos == estado.puntos && otro.parejas > estado.parejas)
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego = estado.puntos * 100 + estado.parejas;

        if (posicion == 1)
        {
            ganadores++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        ganadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_GLACIAR_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void RevelarBloqueGlaciar(MinijuegoParejasGlaciar& minijuego, int indice)
{
    BloqueGlaciar& bloque = minijuego.bloques[indice];
    bloque.revelado = true;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);

    // Memoria imperfecta de cada jugador controlado por IA.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            GetRandomValue(1, 100) <= minijuego.estadosJugadores[i].precisionMemoria
        )
        {
            minijuego.memoriaBots[i][indice] = bloque.simbolo;
        }
    }

    if (minijuego.primera < 0)
    {
        minijuego.primera = indice;
        minijuego.tiempoTurno = DURACION_TURNO_GLACIAR;
        minijuego.botObjetivo = -1;
        minijuego.botTemporizador = 0.6f;
        return;
    }

    minijuego.segunda = indice;
    minijuego.fase = FASE_GLACIAR_REVELANDO;
    minijuego.tiempoPausa = DURACION_REVELADO_GLACIAR;
    minijuego.ultimoJugador = minijuego.turno;

    BloqueGlaciar& primero = minijuego.bloques[minijuego.primera];
    minijuego.aciertoActual = primero.simbolo == bloque.simbolo;

    if (!minijuego.aciertoActual)
    {
        minijuego.ultimoEvento = EVENTO_GLACIAR_FALLO;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        return;
    }

    EstadoJugadorParejasGlaciar& estado = minijuego.estadosJugadores[minijuego.turno];
    bool aurora = bloque.simbolo == SIMBOLO_AURORA_GLACIAR;

    primero.emparejado = true;
    bloque.emparejado = true;
    primero.duenio = minijuego.turno;
    bloque.duenio = minijuego.turno;
    estado.puntos += aurora ? 3 : 1;
    estado.parejas++;
    minijuego.fallosSeguidos = 0;
    minijuego.ultimoEvento = aurora ? EVENTO_GLACIAR_AURORA : EVENTO_GLACIAR_PAREJA;

    OlvidarBloqueGlaciar(minijuego, minijuego.primera);
    OlvidarBloqueGlaciar(minijuego, indice);

    ReproducirSonidoMinijuego(
        minijuego.audio,
        aurora ? SONIDO_RECOGER_NUCLEO_ESPECIAL : SONIDO_ACIERTO
    );
}


static int ElegirBloqueBotGlaciar(const MinijuegoParejasGlaciar& minijuego, int bot)
{
    const int* memoria = minijuego.memoriaBots[bot];

    if (minijuego.primera >= 0)
    {
        int simbolo = minijuego.bloques[minijuego.primera].simbolo;

        for (int j = 0; j < CANTIDAD_BLOQUES_GLACIAR; j++)
        {
            if (
                j != minijuego.primera &&
                memoria[j] == simbolo &&
                BloqueDisponibleGlaciar(minijuego, j)
            )
            {
                return j;
            }
        }
    }
    else
    {
        for (int j = 0; j < CANTIDAD_BLOQUES_GLACIAR; j++)
        {
            for (int k = j + 1; k < CANTIDAD_BLOQUES_GLACIAR; k++)
            {
                if (
                    memoria[j] >= 0 &&
                    memoria[j] == memoria[k] &&
                    BloqueDisponibleGlaciar(minijuego, j) &&
                    BloqueDisponibleGlaciar(minijuego, k)
                )
                {
                    return j;
                }
            }
        }
    }

    int desconocidos[CANTIDAD_BLOQUES_GLACIAR]{};
    int cantidadDesconocidos = 0;
    int cualquiera[CANTIDAD_BLOQUES_GLACIAR]{};
    int cantidadCualquiera = 0;

    for (int j = 0; j < CANTIDAD_BLOQUES_GLACIAR; j++)
    {
        if (!BloqueDisponibleGlaciar(minijuego, j))
        {
            continue;
        }

        cualquiera[cantidadCualquiera] = j;
        cantidadCualquiera++;

        if (memoria[j] < 0)
        {
            desconocidos[cantidadDesconocidos] = j;
            cantidadDesconocidos++;
        }
    }

    if (cantidadDesconocidos > 0)
    {
        return desconocidos[GetRandomValue(0, cantidadDesconocidos - 1)];
    }

    if (cantidadCualquiera > 0)
    {
        return cualquiera[GetRandomValue(0, cantidadCualquiera - 1)];
    }

    return -1;
}


static void ActualizarBotGlaciar(
    MinijuegoParejasGlaciar& minijuego,
    float deltaTime
)
{
    if (minijuego.botObjetivo < 0 || !BloqueDisponibleGlaciar(minijuego, minijuego.botObjetivo))
    {
        minijuego.botObjetivo = ElegirBloqueBotGlaciar(minijuego, minijuego.turno);

        if (minijuego.botObjetivo < 0)
        {
            return;
        }
    }

    minijuego.botTemporizador -= deltaTime;

    if (minijuego.botTemporizador > 0.0f)
    {
        return;
    }

    int columna = minijuego.cursor % COLUMNAS_GLACIAR;
    int fila = minijuego.cursor / COLUMNAS_GLACIAR;
    int columnaObjetivo = minijuego.botObjetivo % COLUMNAS_GLACIAR;
    int filaObjetivo = minijuego.botObjetivo / COLUMNAS_GLACIAR;

    if (columna != columnaObjetivo)
    {
        columna += columnaObjetivo > columna ? 1 : -1;
        minijuego.botTemporizador = 0.1f;
    }
    else if (fila != filaObjetivo)
    {
        fila += filaObjetivo > fila ? 1 : -1;
        minijuego.botTemporizador = 0.1f;
    }
    else
    {
        int objetivo = minijuego.botObjetivo;
        minijuego.botObjetivo = -1;
        RevelarBloqueGlaciar(minijuego, objetivo);
        return;
    }

    minijuego.cursor = fila * COLUMNAS_GLACIAR + columna;

    if (
        minijuego.cursor == minijuego.botObjetivo
    )
    {
        minijuego.botTemporizador = 0.35f;
    }
}


static void ElegirAleatorioGlaciar(MinijuegoParejasGlaciar& minijuego)
{
    int candidatos[CANTIDAD_BLOQUES_GLACIAR]{};
    int cantidad = 0;

    for (int j = 0; j < CANTIDAD_BLOQUES_GLACIAR; j++)
    {
        if (BloqueDisponibleGlaciar(minijuego, j))
        {
            candidatos[cantidad] = j;
            cantidad++;
        }
    }

    if (cantidad == 0)
    {
        return;
    }

    int elegido = candidatos[GetRandomValue(0, cantidad - 1)];
    minijuego.cursor = elegido;
    RevelarBloqueGlaciar(minijuego, elegido);
}


static void ActualizarElegirGlaciar(
    MinijuegoParejasGlaciar& minijuego,
    float deltaTime,
    Participante participantes[]
)
{
    minijuego.tiempoTurno -= deltaTime;

    const Participante& participante = participantes[minijuego.turno];

    if (EsControlBotGlaciar(participante))
    {
        ActualizarBotGlaciar(minijuego, deltaTime);
    }
    else
    {
        InputMinijuegoParticipante entrada = LeerInputMinijuegoParticipante(participante);

        int direccion = 0;
        if (entrada.izquierda) direccion = 1;
        else if (entrada.derecha) direccion = 2;
        else if (entrada.adelante) direccion = 3;
        else if (entrada.atras) direccion = 4;

        if (direccion != 0)
        {
            minijuego.repeticionCursor -= deltaTime;

            if (direccion != minijuego.direccionPrevia || minijuego.repeticionCursor <= 0.0f)
            {
                int columna = minijuego.cursor % COLUMNAS_GLACIAR;
                int fila = minijuego.cursor / COLUMNAS_GLACIAR;

                if (direccion == 1 && columna > 0) columna--;
                else if (direccion == 2 && columna < COLUMNAS_GLACIAR - 1) columna++;
                else if (direccion == 3 && fila > 0) fila--;
                else if (direccion == 4 && fila < COLUMNAS_GLACIAR - 1) fila++;

                int nuevo = fila * COLUMNAS_GLACIAR + columna;

                if (nuevo != minijuego.cursor)
                {
                    minijuego.cursor = nuevo;
                    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);
                }

                minijuego.repeticionCursor =
                    direccion != minijuego.direccionPrevia ? 0.26f : 0.13f;
            }
        }
        else
        {
            minijuego.repeticionCursor = 0.0f;
        }

        minijuego.direccionPrevia = direccion;

        bool confirmar = entrada.golpear && !minijuego.accionPrevia;
        minijuego.accionPrevia = entrada.golpear;

        if (confirmar)
        {
            if (BloqueDisponibleGlaciar(minijuego, minijuego.cursor))
            {
                RevelarBloqueGlaciar(minijuego, minijuego.cursor);
            }
            else
            {
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
            }

            return;
        }
    }

    if (minijuego.fase == FASE_GLACIAR_ELEGIR && minijuego.tiempoTurno <= 0.0f)
    {
        ElegirAleatorioGlaciar(minijuego);
    }
}


static bool TodasEmparejadasGlaciar(const MinijuegoParejasGlaciar& minijuego)
{
    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        if (!minijuego.bloques[i].emparejado)
        {
            return false;
        }
    }

    return true;
}


static void IniciarCrujidoGlaciar(MinijuegoParejasGlaciar& minijuego)
{
    int ocultos[CANTIDAD_BLOQUES_GLACIAR]{};
    int cantidad = 0;

    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        if (BloqueDisponibleGlaciar(minijuego, i))
        {
            ocultos[cantidad] = i;
            cantidad++;
        }
    }

    if (cantidad < 2)
    {
        IniciarTurnoGlaciar(minijuego);
        return;
    }

    int a = GetRandomValue(0, cantidad - 1);
    int b = GetRandomValue(0, cantidad - 2);
    if (b >= a) b++;

    minijuego.crujidoA = ocultos[a];
    minijuego.crujidoB = ocultos[b];
    minijuego.crujidoIntercambiado = false;
    minijuego.fase = FASE_GLACIAR_CRUJIDO;
    minijuego.tiempoPausa = DURACION_CRUJIDO_GLACIAR;
    minijuego.ultimoEvento = EVENTO_GLACIAR_CRUJE;
    minijuego.fallosSeguidos = 0;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
}


static void ActualizarRevelandoGlaciar(MinijuegoParejasGlaciar& minijuego, float deltaTime)
{
    minijuego.tiempoPausa -= deltaTime;

    if (minijuego.tiempoPausa > 0.0f)
    {
        return;
    }

    if (minijuego.aciertoActual)
    {
        minijuego.primera = -1;
        minijuego.segunda = -1;

        if (TodasEmparejadasGlaciar(minijuego))
        {
            FinalizarGlaciar(minijuego);
            return;
        }

        IniciarTurnoGlaciar(minijuego);
        return;
    }

    minijuego.bloques[minijuego.primera].revelado = false;
    minijuego.bloques[minijuego.segunda].revelado = false;
    minijuego.primera = -1;
    minijuego.segunda = -1;
    minijuego.fallosSeguidos++;
    minijuego.turno = SiguienteJugadorGlaciar(minijuego, minijuego.turno);

    // Los bots olvidan algo con el paso de los turnos.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        for (int j = 0; j < CANTIDAD_BLOQUES_GLACIAR; j++)
        {
            if (minijuego.memoriaBots[i][j] >= 0 && GetRandomValue(1, 100) <= 8)
            {
                minijuego.memoriaBots[i][j] = -1;
            }
        }
    }

    if (minijuego.fallosSeguidos >= FALLOS_PARA_CRUJIR_GLACIAR)
    {
        IniciarCrujidoGlaciar(minijuego);
    }
    else
    {
        IniciarTurnoGlaciar(minijuego);
    }
}


static void ActualizarCrujidoGlaciar(MinijuegoParejasGlaciar& minijuego, float deltaTime)
{
    minijuego.tiempoPausa -= deltaTime;
    minijuego.bloques[minijuego.crujidoA].temblor = 1.0f;
    minijuego.bloques[minijuego.crujidoB].temblor = 1.0f;

    if (!minijuego.crujidoIntercambiado && minijuego.tiempoPausa <= DURACION_CRUJIDO_GLACIAR * 0.45f)
    {
        minijuego.crujidoIntercambiado = true;

        int temporal = minijuego.bloques[minijuego.crujidoA].simbolo;
        minijuego.bloques[minijuego.crujidoA].simbolo =
            minijuego.bloques[minijuego.crujidoB].simbolo;
        minijuego.bloques[minijuego.crujidoB].simbolo = temporal;

        OlvidarBloqueGlaciar(minijuego, minijuego.crujidoA);
        OlvidarBloqueGlaciar(minijuego, minijuego.crujidoB);
    }

    if (minijuego.tiempoPausa <= 0.0f)
    {
        minijuego.bloques[minijuego.crujidoA].temblor = 0.0f;
        minijuego.bloques[minijuego.crujidoB].temblor = 0.0f;
        minijuego.crujidoA = -1;
        minijuego.crujidoB = -1;
        IniciarTurnoGlaciar(minijuego);
    }
}


static void ActualizarEfectosGlaciar(MinijuegoParejasGlaciar& minijuego, float deltaTime)
{
    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        BloqueGlaciar& bloque = minijuego.bloques[i];
        float objetivo = (bloque.revelado || bloque.emparejado) ? 1.0f : 0.0f;

        if (bloque.derretido < objetivo)
        {
            bloque.derretido += 3.0f * deltaTime;
            if (bloque.derretido > objetivo) bloque.derretido = objetivo;
        }
        else if (bloque.derretido > objetivo)
        {
            bloque.derretido -= 3.0f * deltaTime;
            if (bloque.derretido < objetivo) bloque.derretido = objetivo;
        }
    }
}


//==================================================
// INTERFAZ
//==================================================


void MinijuegoParejasGlaciar::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        posicionTempanoX[i] = 0.0f;
        posicionTempanoZ[i] = 0.0f;

        for (int j = 0; j < CANTIDAD_BLOQUES_GLACIAR; j++)
        {
            memoriaBots[i][j] = -1;
        }
    }

    BarajarBloquesGlaciar(*this);

    // Camara baja: el altar y los tempanos abajo; el mar, el frente del
    // glaciar y la aurora arriba.
    camara.position = OJO_GLACIAR;
    camara.target = MIRA_GLACIAR;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 60.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    for (int i = 0; i < MAX_PARTICULAS_GLACIAR; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        destelloBloque[i] = 0.0f;
        sacudidaBloque[i] = 0.0f;
    }

    tiempoCrujidoVisual = 99.0f;
    turnoRepetido = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        destelloJugador[i] = 0.0f;
    }

    semillaVisual = 0x51EDu;
    brilloAurora = 0.0f;
    tiempoAnuncioTurno = 99.0f;
    tiempoSuma = 0.0f;
    sumaVisual = 0;
    jugadorSuma = -1;
    tiempoFinVisual = 0.0f;
    confetiPendiente = 0.0f;

    fase = FASE_GLACIAR_PREPARACION;
    ultimoEvento = EVENTO_GLACIAR_NINGUNO;
    turno = -1;
    ultimoJugador = -1;
    cursor = 5;
    primera = -1;
    segunda = -1;
    fallosSeguidos = 0;
    direccionPrevia = 0;
    botObjetivo = -1;
    crujidoA = -1;
    crujidoB = -1;
    aciertoActual = false;
    accionPrevia = false;
    crujidoIntercambiado = false;
    repeticionCursor = 0.0f;
    botTemporizador = 0.0f;
    tiempoTurno = DURACION_TURNO_GLACIAR;
    tiempoPausa = 0.0f;
    tiempoRestante = DURACION_PARTIDA_GLACIAR;
    tiempoPreparacion = DURACION_PREPARACION_GLACIAR;
    tiempoAnimacion = 0.0f;
}


void MinijuegoParejasGlaciar::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_GLACIAR_TERMINADO;
        return;
    }

    CargarPiezasJugablesGlaciar();
    const float esquinasX[MAX_PARTICIPANTES] = { -7.2f, 7.2f, -7.2f, 7.2f };
    const float esquinasZ[MAX_PARTICIPANTES] = { -2.6f, -2.6f, 2.6f, 2.6f };

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
    int ranura = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        float x = cantidad == 2 ? (ranura == 0 ? -7.2f : 7.2f) : esquinasX[ranura];
        float z = cantidad == 2 ? 0.0f : esquinasZ[ranura];
        posicionTempanoX[i] = x;
        posicionTempanoZ[i] = z;
        colorJugador[i] = participantes[i].color;
        estadosJugadores[i].precisionMemoria = GetRandomValue(60, 80);
        ranura++;

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], Vector3{ x, 0.75f, z });
        jugadores[i].direccionMirada = { x > 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f };
    }

    turno = indices[GetRandomValue(0, cantidad - 1)];
}


static void ActualizarPartidaGlaciar(
    MinijuegoParejasGlaciar& m,
    float deltaTime,
    Participante participantes[]
)
{
    m.tiempoAnimacion += deltaTime;

    if (
        m.fase == FASE_GLACIAR_TERMINADO ||
        m.resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    ActualizarEfectosGlaciar(m, deltaTime);

    if (m.fase == FASE_GLACIAR_PREPARACION)
    {
        float preparacionAntes = m.tiempoPreparacion;
        m.tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(m.audio, preparacionAntes, m.tiempoPreparacion);

        if (m.tiempoPreparacion <= 0.0f)
        {
            m.tiempoPreparacion = 0.0f;
            IniciarTurnoGlaciar(m);
        }

        return;
    }

    float restanteAntes = m.tiempoRestante;
    m.tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(m.audio, restanteAntes, m.tiempoRestante);

    if (m.tiempoRestante <= 0.0f)
    {
        m.tiempoRestante = 0.0f;
        FinalizarGlaciar(m);
        return;
    }

    switch (m.fase)
    {
    case FASE_GLACIAR_ELEGIR:
        ActualizarElegirGlaciar(m, deltaTime, participantes);
        break;

    case FASE_GLACIAR_REVELANDO:
        ActualizarRevelandoGlaciar(m, deltaTime);
        break;

    case FASE_GLACIAR_CRUJIDO:
        ActualizarCrujidoGlaciar(m, deltaTime);
        break;

    case FASE_GLACIAR_PREPARACION:
    case FASE_GLACIAR_TERMINADO:
        break;
    }
}


// Lo que el visual compara antes y despues de cada paso de la logica.
struct InstantaneaGlaciar
{
    FaseParejasGlaciar fase = FASE_GLACIAR_PREPARACION;
    int turno = -1;
    bool revelado[CANTIDAD_BLOQUES_GLACIAR] = {};
    bool emparejado[CANTIDAD_BLOQUES_GLACIAR] = {};
    int puntos[MAX_PARTICIPANTES] = {};
};

// Definidas con el resto del visual, al final del archivo.
static void TomarInstantaneaGlaciar(const MinijuegoParejasGlaciar& m, InstantaneaGlaciar& s);
static void ActualizarVisualGlaciar(MinijuegoParejasGlaciar& m, const InstantaneaGlaciar& s, float deltaTime);


void MinijuegoParejasGlaciar::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    (void)cantidadMaxima;

    InstantaneaGlaciar antes;
    TomarInstantaneaGlaciar(*this, antes);

    ActualizarPartidaGlaciar(*this, deltaTime, participantes);

    if (resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        ActualizarVisualGlaciar(*this, antes, deltaTime);
    }
}


//==================================================
// VISUAL: GLACIAR COSTERO DE NOCHE CON AURORA
//==================================================
//
// Capas, de adelante hacia atras: la banquisa nevada con el altar de
// hielo tallado (la grilla de 4x4) y un pozo de agua por lado donde flota
// el tempano de cada jugador; un anillo de utileria (faroles en postes,
// pinguinos, trineo con cajas, bandera de expedicion, cristales de hielo,
// inukshuk, pozo de pesca, rocas y ventisqueros); la cueva de hielo sobre
// la banquisa, el mar helado con icebergs, tempanos a la deriva y una
// foca; el frente del glaciar; dos cordilleras en la bruma; y el cielo
// con estrellas y la aurora boreal en cortinas. Todo con primitivas
// agrupadas por rlgl: los unicos GLB son las piezas jugables (bloques,
// cursor, simbolos, tempanos y fragmentos). El azar visual sale de
// semillaVisual o de un hash por indice; nunca de GetRandomValue.
//==================================================


static const Color COLOR_CIELO_GLACIAR = { 4, 6, 20, 255 };
static const Color COLOR_HORIZONTE_GLACIAR = { 34, 54, 90, 255 };
static const Color COLOR_NIEBLA_GLACIAR = { 42, 62, 98, 255 };
static const Color COLOR_NIEVE_GLACIAR = { 146, 164, 202, 255 };
static const Color COLOR_NIEVE_CLARA_GLACIAR = { 196, 210, 238, 255 };
static const Color COLOR_HIELO_GLACIAR = { 112, 166, 214, 255 };
static const Color COLOR_HIELO_CLARO_GLACIAR = { 178, 216, 244, 255 };
static const Color COLOR_HIELO_PROFUNDO_GLACIAR = { 34, 78, 140, 255 };
static const Color COLOR_TABLERO_GLACIAR = { 80, 114, 162, 255 };
static const Color COLOR_HUECO_GLACIAR = { 40, 64, 108, 255 };
static const Color COLOR_AGUA_GLACIAR = { 8, 22, 40, 255 };
static const Color COLOR_AGUA_CLARA_GLACIAR = { 22, 50, 78, 255 };
static const Color COLOR_MADERA_GLACIAR = { 124, 90, 62, 255 };
static const Color COLOR_MADERA_OSCURA_GLACIAR = { 76, 56, 44, 255 };
static const Color COLOR_PIEDRA_GLACIAR = { 84, 88, 104, 255 };
static const Color COLOR_HIERRO_GLACIAR = { 50, 54, 64, 255 };
static const Color COLOR_LUZ_GLACIAR = { 255, 192, 112, 255 };
static const Color COLOR_VIDRIO_GLACIAR = { 255, 222, 160, 255 };
static const Color COLOR_CRISTAL_GLACIAR = { 120, 214, 255, 255 };
static const Color COLOR_AURORA_VERDE_GLACIAR = { 80, 255, 166, 255 };
static const Color COLOR_AURORA_CIAN_GLACIAR = { 70, 212, 255, 255 };
static const Color COLOR_AURORA_VIOLETA_GLACIAR = { 184, 100, 255, 255 };
static const Color COLOR_PLUMA_GLACIAR = { 26, 28, 40, 255 };
static const Color COLOR_PLUMA_CLARA_GLACIAR = { 232, 236, 246, 255 };
static const Color COLOR_PICO_GLACIAR = { 246, 156, 46, 255 };
static const Color COLOR_FOCA_GLACIAR = { 104, 110, 124, 255 };
static const Color COLOR_BANDERA_GLACIAR = { 34, 74, 124, 255 };

// Ejes de la pantalla en el mundo (para lo que mira a la camara).
static const Vector3 FRENTE_U_GLACIAR = { 1.0f, 0.0f, 0.0f };
static const Vector3 FRENTE_V_GLACIAR = { 0.0f, 0.9514f, -0.3078f };

static const float Y_NIEVE_GLACIAR = -0.35f;
static const float Y_MAR_GLACIAR = -1.25f;
static const float Y_POZO_GLACIAR = -0.315f;
static const float RADIO_POZO_GLACIAR = 2.55f;
static const float MEDIO_TABLERO_GLACIAR = 4.25f;
static const float BASE_TABLERO_GLACIAR = 4.6f;
static const float DURACION_FIN_VISUAL_GLACIAR = 3.2f;
static const float DURACION_ANUNCIO_TURNO_GLACIAR = 1.4f;

// Niebla fija para piezas dibujadas con matriz propia (<0: por vertice).
static float nieblaFijaGlaciar = -1.0f;


//--------------------------------------------------
// Utilidades de color y geometria
//--------------------------------------------------

static float Limitar01Glaciar(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static unsigned char CanalGlaciar(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoGlaciar(Color color, float factor)
{
    return Color{
        CanalGlaciar((float)color.r * factor),
        CanalGlaciar((float)color.g * factor),
        CanalGlaciar((float)color.b * factor),
        color.a
    };
}


static Color MezclaGlaciar(Color a, Color b, float t)
{
    t = Limitar01Glaciar(t);

    return Color{
        CanalGlaciar((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalGlaciar((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalGlaciar((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalGlaciar((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaGlaciar(Color color, float alfa)
{
    color.a = CanalGlaciar(255.0f * alfa);
    return color;
}


static float RuidoGlaciar(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float SuaveGlaciar(float u)
{
    u = Limitar01Glaciar(u);
    return u * u * (3.0f - 2.0f * u);
}


// Salida con rebote (aparicion de los bloques).
static float ReboteGlaciar(float u)
{
    u = Limitar01Glaciar(u);
    float v = u - 1.0f;
    return 1.0f + 2.7f * v * v * v + 1.7f * v * v;
}


// Bruma azul: lo lejano se funde con el aire frio del horizonte.
static float CantidadNieblaGlaciar(Vector3 p)
{
    if (nieblaFijaGlaciar >= 0.0f)
    {
        return nieblaFijaGlaciar;
    }

    float dx = p.x - OJO_GLACIAR.x;
    float dy = p.y - OJO_GLACIAR.y;
    float dz = p.z - OJO_GLACIAR.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 30.0f) / 130.0f;

    return n < 0.0f ? 0.0f : (n > 0.8f ? 0.8f : n);
}


static Color TinteGlaciar(Color color, Vector3 p, float emisivo = 0.0f)
{
    Color niebla = COLOR_NIEBLA_GLACIAR;
    niebla.a = color.a;
    return MezclaGlaciar(color, niebla, CantidadNieblaGlaciar(p) * (1.0f - 0.65f * emisivo));
}


static Vector3 SumaGlaciar(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaGlaciar(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaGlaciar(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }


static Vector3 NormalizarGlaciar(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 0.0001f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static Vector3 CruzGlaciar(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}


static void VerticeGlaciar(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadGlaciar(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeGlaciar(a, color);
    VerticeGlaciar(b, color);
    VerticeGlaciar(c, color);
    VerticeGlaciar(d, color);
}


static void QuadNieblaGlaciar(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeGlaciar(a, TinteGlaciar(color, a));
    VerticeGlaciar(b, TinteGlaciar(color, b));
    VerticeGlaciar(c, TinteGlaciar(color, c));
    VerticeGlaciar(d, TinteGlaciar(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.55), sin niebla. Respeta la matriz de rlgl.
static void CuboCrudoGlaciar(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadGlaciar({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, color);
    QuadGlaciar({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoGlaciar(color, 0.78f));
    QuadGlaciar({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoGlaciar(color, 0.55f));
    QuadGlaciar({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoGlaciar(color, 0.62f));
    QuadGlaciar({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoGlaciar(color, 0.70f));
    QuadGlaciar({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoGlaciar(color, 0.45f));
    rlEnd();
}


static void CuboGlaciar(Vector3 c, Vector3 t, Color color)
{
    CuboCrudoGlaciar(c, t, TinteGlaciar(color, c));
}


static void CuboGiradoGlaciar(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    Color tenido = TinteGlaciar(color, c);
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboCrudoGlaciar({ 0.0f, 0.0f, 0.0f }, t, tenido);
    rlPopMatrix();
}


// Luz de la escena: el cielo de la aurora, de arriba, algo de frente.
static float LuzNormalGlaciar(Vector3 n)
{
    const float LX = -0.30f, LY = 0.82f, LZ = 0.48f;
    float d = n.x * LX + n.y * LY + n.z * LZ;
    return 0.52f + 0.50f * Limitar01Glaciar(d);
}


// Prisma (o tronco de cono) entre dos puntos, con niebla por vertice.
static void PrismaGlaciar(
    Vector3 a,
    Vector3 b,
    float r0,
    float r1,
    int lados,
    Color color,
    float emisivo = 0.0f,
    float giro = 0.0f
)
{
    Vector3 eje = NormalizarGlaciar(RestaGlaciar(b, a));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarGlaciar(CruzGlaciar(eje, apoyo));
    Vector3 v = CruzGlaciar(eje, u);
    float paso = 6.2832f / (float)lados;
    float luzTapa = LuzNormalGlaciar(eje);
    Color tapa = TonoGlaciar(color, luzTapa + (1.0f - luzTapa) * emisivo);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaGlaciar(EscalaGlaciar(u, std::cos(a0)), EscalaGlaciar(v, std::sin(a0)));
        Vector3 d1 = SumaGlaciar(EscalaGlaciar(u, std::cos(a1)), EscalaGlaciar(v, std::sin(a1)));
        Vector3 dm = SumaGlaciar(EscalaGlaciar(u, std::cos(am)), EscalaGlaciar(v, std::sin(am)));

        float f = LuzNormalGlaciar(dm);
        f = f + (1.0f - f) * emisivo;
        if (k & 1) f *= 0.93f;
        Color cara = TonoGlaciar(color, f);

        Vector3 p0 = SumaGlaciar(a, EscalaGlaciar(d0, r0));
        Vector3 p1 = SumaGlaciar(a, EscalaGlaciar(d1, r0));
        Vector3 p2 = SumaGlaciar(b, EscalaGlaciar(d1, r1));
        Vector3 p3 = SumaGlaciar(b, EscalaGlaciar(d0, r1));

        QuadNieblaGlaciar(p0, p1, p2, p3, cara);

        if (r1 > 0.001f)
        {
            Color tapaNiebla = TinteGlaciar(tapa, b, emisivo);
            QuadGlaciar(b, p2, p3, b, tapaNiebla);
        }
    }

    rlEnd();
}


// Roca, tempano o ventisquero facetado: esfera de pocos lados con radios
// irregulares.
static void RocaGlaciar(Vector3 c, Vector3 r, int semilla, Color color, float irregular = 0.14f, float emisivo = 0.0f)
{
    const int ANILLOS = 5;
    const int LADOS = 8;

    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        for (int k = 0; k < LADOS; k++)
        {
            int aa[4] = { a, a, a + 1, a + 1 };
            int kk[4] = { k, k + 1, k + 1, k };

            for (int q = 0; q < 4; q++)
            {
                float th = -1.5708f + 3.1416f * (float)aa[q] / (float)ANILLOS;
                float ph = 6.2832f * (float)kk[q] / (float)LADOS + (float)semilla;
                float f = 1.0f;

                if (aa[q] != 0 && aa[q] != ANILLOS)
                {
                    f = 1.0f - irregular + 2.0f * irregular * RuidoGlaciar(aa[q] * 31 + (kk[q] % LADOS), semilla);
                }

                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = { c.x + n.x * r.x * f, c.y + n.y * r.y * f, c.z + n.z * r.z * f };
                float luz = LuzNormalGlaciar(n);
                luz = luz + (1.0f - luz) * emisivo;
                Color cv = TonoGlaciar(color, luz * (0.93f + 0.12f * RuidoGlaciar(aa[q] * 7 + kk[q], semilla + 5)));
                VerticeGlaciar(p, TinteGlaciar(cv, p, emisivo));
            }
        }
    }

    rlEnd();
}


// Disco horizontal relleno.
static void SectorGlaciar(Vector3 c, float radio, Color color, int lados = 18)
{
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < lados; k++)
    {
        float b0 = 6.2832f * (float)k / (float)lados;
        float b1 = 6.2832f * (float)(k + 1) / (float)lados;
        VerticeGlaciar(c, color);
        VerticeGlaciar({ c.x + std::cos(b1) * radio, c.y, c.z + std::sin(b1) * radio }, color);
        VerticeGlaciar({ c.x + std::cos(b0) * radio, c.y, c.z + std::sin(b0) * radio }, color);
    }

    rlEnd();
}


// Corona horizontal (ondas, bordes, anillos de luz).
static void AnilloGlaciar(Vector3 c, float r0, float r1, Color color, int lados = 24)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadGlaciar(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Marco cuadrado horizontal (huecos de la grilla, cursor).
static void MarcoGlaciar(float x, float y, float z, float r0, float r1, Color color)
{
    rlBegin(RL_QUADS);
    QuadGlaciar({ x - r1, y, z - r1 }, { x + r1, y, z - r1 }, { x + r0, y, z - r0 }, { x - r0, y, z - r0 }, color);
    QuadGlaciar({ x + r1, y, z - r1 }, { x + r1, y, z + r1 }, { x + r0, y, z + r0 }, { x + r0, y, z - r0 }, color);
    QuadGlaciar({ x + r1, y, z + r1 }, { x - r1, y, z + r1 }, { x - r0, y, z + r0 }, { x + r0, y, z + r0 }, color);
    QuadGlaciar({ x - r1, y, z + r1 }, { x - r1, y, z - r1 }, { x - r0, y, z - r0 }, { x - r0, y, z + r0 }, color);
    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorGlaciar(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloGlaciar(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorGlaciar(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


static void ResplandorFrenteGlaciar(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorGlaciar(c, FRENTE_U_GLACIAR, FRENTE_V_GLACIAR, radio, color, estirado);
}


// Reflejo sobre el agua: estirado hacia la camara (eje z).
static void ReflejoAguaGlaciar(Vector3 c, float radio, Color color, float estirado)
{
    ResplandorGlaciar(c, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, radio, color, estirado);
}


static void HaloGlaciar(Vector3 p, float radio, Color exterior, Color interior)
{
    ResplandorFrenteGlaciar(p, radio, exterior);
    ResplandorFrenteGlaciar(p, radio * 0.42f, interior);
}


// Cono de luz (aditivo): del foco hacia el charco, se apaga en la base.
static void ConoLuzGlaciar(Vector3 foco, Vector3 baseCentro, float radio, Color color)
{
    const int LADOS = 16;
    Vector3 eje = NormalizarGlaciar(RestaGlaciar(baseCentro, foco));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarGlaciar(CruzGlaciar(eje, apoyo));
    Vector3 v = CruzGlaciar(eje, u);

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaGlaciar(baseCentro, SumaGlaciar(EscalaGlaciar(u, std::cos(a0) * radio), EscalaGlaciar(v, std::sin(a0) * radio)));
        Vector3 p1 = SumaGlaciar(baseCentro, SumaGlaciar(EscalaGlaciar(u, std::cos(a1) * radio), EscalaGlaciar(v, std::sin(a1) * radio)));

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(foco.x, foco.y, foco.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p1.x, p1.y, p1.z);
    }
    rlEnd();
}


// Charco de luz: discos concentricos muy tenues.
static void CharcoLuzGlaciar(Vector3 c, float radio, Color color, float intensidad)
{
    for (int r = 0; r < 3; r++)
    {
        float f = 0.55f + 0.45f * (float)r;
        ResplandorSueloGlaciar({ c.x, c.y + 0.002f * (float)r, c.z }, radio * f, AlfaGlaciar(color, (0.05f + 0.03f * (float)(2 - r)) * intensidad));
    }
}


//--------------------------------------------------
// Datos de la escena
//--------------------------------------------------

// Contorno de la banquisa (visto desde arriba, en orden angular desde
// el centro). El frente queda fuera de cuadro; a la izquierda se ensancha
// hacia atras para sostener la cueva de hielo.
static const Vector2 CONTORNO_BANQUISA_GLACIAR[] =
{
    { -15.5f, 13.0f }, { -14.4f, 6.5f }, { -15.1f, 1.5f }, { -14.0f, -3.4f },
    { -16.6f, -6.6f }, { -20.6f, -9.4f }, { -19.6f, -13.6f }, { -15.2f, -15.2f },
    { -10.0f, -13.0f }, { -6.4f, -8.4f }, { -2.2f, -7.7f }, { 2.4f, -8.1f },
    { 6.4f, -7.5f }, { 9.8f, -8.0f }, { 13.0f, -6.6f }, { 14.6f, -3.0f },
    { 14.0f, 1.6f }, { 15.2f, 6.2f }, { 14.6f, 13.0f }
};
static const int CANTIDAD_CONTORNO_GLACIAR =
    (int)(sizeof(CONTORNO_BANQUISA_GLACIAR) / sizeof(CONTORNO_BANQUISA_GLACIAR[0]));
static const Vector2 CENTRO_BANQUISA_GLACIAR = { 0.0f, 2.0f };

static const Vector3 CUEVA_GLACIAR = { -15.0f, -0.35f, -10.4f };
static const float RADIO_CUEVA_GLACIAR = 2.1f;

static const Vector3 POSTES_FAROL_GLACIAR[2] = { { -5.0f, -0.35f, -5.1f }, { 5.0f, -0.35f, -5.1f } };

struct IcebergGlaciar
{
    float x;
    float z;
    float radio;
    float alto;
    int tipo;       // 0 tabular, 1 aguja
};

static const IcebergGlaciar ICEBERGS_GLACIAR[] =
{
    { 23.0f, -16.5f, 4.6f, 3.0f, 0 },
    { -31.0f, -21.0f, 3.0f, 5.6f, 1 },
    { -5.0f, -27.0f, 3.4f, 2.2f, 0 },
    { 40.0f, -27.0f, 3.0f, 2.0f, 0 },
    { -48.0f, -30.0f, 4.2f, 3.0f, 0 },
    { 12.0f, -31.0f, 2.2f, 4.4f, 1 },
    { -21.0f, -32.0f, 2.6f, 1.8f, 0 }
};
static const int CANTIDAD_ICEBERGS_GLACIAR =
    (int)(sizeof(ICEBERGS_GLACIAR) / sizeof(ICEBERGS_GLACIAR[0]));


//--------------------------------------------------
// Estado visual: instantanea, particulas y efectos
//--------------------------------------------------

static void TomarInstantaneaGlaciar(const MinijuegoParejasGlaciar& m, InstantaneaGlaciar& s)
{
    s.fase = m.fase;
    s.turno = m.turno;

    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        s.revelado[i] = m.bloques[i].revelado;
        s.emparejado[i] = m.bloques[i].emparejado;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        s.puntos[i] = m.estadosJugadores[i].puntos;
    }
}


static float AzarGlaciar(MinijuegoParejasGlaciar& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasGlaciar(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


static void EmitirParticulaGlaciar(
    MinijuegoParejasGlaciar& m,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    int elegida = 0;
    float menorVida = 1e9f;

    for (int i = 0; i < MAX_PARTICULAS_GLACIAR; i++)
    {
        if (!m.particulas[i].activa)
        {
            elegida = i;
            break;
        }

        if (m.particulas[i].vida < menorVida)
        {
            menorVida = m.particulas[i].vida;
            elegida = i;
        }
    }

    ParticulaGlaciar& p = m.particulas[elegida];
    p.activa = true;
    p.tipo = tipo;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.vida = vida;
    p.vidaMaxima = vida;
    p.tamano = tamano;
    p.giro = AzarGlaciar(m, 0.0f, 360.0f);
    p.color = color;
}


static Vector3 CentroBloqueGlaciar(int indice, float y)
{
    return { PosicionXBloqueGlaciar(indice), y, PosicionZBloqueGlaciar(indice) };
}


// Esquirlas y vapor que saltan de un bloque.
static void EstallidoBloqueGlaciar(MinijuegoParejasGlaciar& m, int indice, int esquirlas, int vapor, Color color)
{
    if (indice < 0 || indice >= CANTIDAD_BLOQUES_GLACIAR)
    {
        return;
    }

    Vector3 c = CentroBloqueGlaciar(indice, 0.6f);
    int n = CantidadParticulasGlaciar(esquirlas);

    for (int k = 0; k < n; k++)
    {
        float a = AzarGlaciar(m, 0.0f, 6.2832f);
        float r = AzarGlaciar(m, 1.0f, 2.6f);
        EmitirParticulaGlaciar(m, PARTICULA_GLACIAR_ESQUIRLA,
            { c.x + std::cos(a) * 0.5f, c.y, c.z + std::sin(a) * 0.5f },
            { std::cos(a) * r, AzarGlaciar(m, 2.0f, 4.2f), std::sin(a) * r },
            AzarGlaciar(m, 0.6f, 1.0f), AzarGlaciar(m, 0.07f, 0.14f), color);
    }

    n = CantidadParticulasGlaciar(vapor);

    for (int k = 0; k < n; k++)
    {
        EmitirParticulaGlaciar(m, PARTICULA_GLACIAR_VAPOR,
            { c.x + AzarGlaciar(m, -0.5f, 0.5f), 0.4f, c.z + AzarGlaciar(m, -0.5f, 0.5f) },
            { AzarGlaciar(m, -0.25f, 0.25f), AzarGlaciar(m, 0.6f, 1.1f), AzarGlaciar(m, -0.2f, 0.2f) },
            AzarGlaciar(m, 0.9f, 1.5f), AzarGlaciar(m, 0.3f, 0.5f), Color{ 220, 236, 255, 255 });
    }
}


// Chispas del color de quien hizo la pareja: saltan del bloque y vuelan a
// su tempano.
static void ChispasParejaGlaciar(MinijuegoParejasGlaciar& m, int indice, int jugador, Color color, bool aurora)
{
    if (indice < 0 || indice >= CANTIDAD_BLOQUES_GLACIAR || jugador < 0 || jugador >= MAX_PARTICIPANTES)
    {
        return;
    }

    Vector3 c = CentroBloqueGlaciar(indice, 0.8f);
    int n = CantidadParticulasGlaciar(aurora ? 22 : 12);

    for (int k = 0; k < n; k++)
    {
        float a = AzarGlaciar(m, 0.0f, 6.2832f);
        float r = AzarGlaciar(m, 0.8f, 2.4f);
        Color tono = aurora
            ? (k % 3 == 0 ? COLOR_AURORA_VERDE_GLACIAR : (k % 3 == 1 ? COLOR_AURORA_CIAN_GLACIAR : COLOR_AURORA_VIOLETA_GLACIAR))
            : (k % 3 == 0 ? Color{ 255, 255, 255, 255 } : color);
        EmitirParticulaGlaciar(m, PARTICULA_GLACIAR_CHISPA, c,
            { std::cos(a) * r, AzarGlaciar(m, 1.8f, 4.4f), std::sin(a) * r },
            AzarGlaciar(m, 0.5f, 0.9f), AzarGlaciar(m, 0.07f, 0.14f), tono);
    }

    // Estela hacia el tempano del jugador (llega en ~0.8 s).
    Vector3 destino = { m.posicionTempanoX[jugador], 1.6f, m.posicionTempanoZ[jugador] };
    int estela = CantidadParticulasGlaciar(8);

    for (int k = 0; k < estela; k++)
    {
        float vida = AzarGlaciar(m, 0.7f, 0.9f);
        Vector3 v = EscalaGlaciar(RestaGlaciar(destino, c), 1.0f / vida);
        v.y += 3.0f * vida * 0.5f;
        EmitirParticulaGlaciar(m, PARTICULA_GLACIAR_CHISPA,
            { c.x + AzarGlaciar(m, -0.3f, 0.3f), c.y, c.z + AzarGlaciar(m, -0.3f, 0.3f) },
            v, vida, AzarGlaciar(m, 0.09f, 0.15f), aurora ? COLOR_AURORA_VERDE_GLACIAR : color);
    }
}


static void ActualizarParticulasGlaciar(MinijuegoParejasGlaciar& m, float dt)
{
    for (int i = 0; i < MAX_PARTICULAS_GLACIAR; i++)
    {
        ParticulaGlaciar& p = m.particulas[i];
        if (!p.activa) continue;

        p.vida -= dt;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        switch (p.tipo)
        {
        case PARTICULA_GLACIAR_ESQUIRLA:
            p.velocidad.y -= 11.0f * dt;
            p.giro += 420.0f * dt;
            break;
        case PARTICULA_GLACIAR_VAPOR:
            p.velocidad = EscalaGlaciar(p.velocidad, 1.0f - 1.4f * dt);
            p.velocidad.y += 0.3f * dt;
            p.tamano += 0.5f * dt;
            break;
        case PARTICULA_GLACIAR_CHISPA:
            p.velocidad.y -= 3.0f * dt;
            break;
        case PARTICULA_GLACIAR_CONFETI:
            p.velocidad.y -= 5.0f * dt;
            if (p.velocidad.y < -1.7f) p.velocidad.y = -1.7f;
            p.velocidad.x += std::sin(p.vida * 5.0f + p.giro) * 2.0f * dt;
            p.giro += 360.0f * dt;
            break;
        default:
            break;
        }

        p.posicion = SumaGlaciar(p.posicion, EscalaGlaciar(p.velocidad, dt));

        // Las esquirlas rebotan una vez sobre el altar y se quedan.
        if (p.tipo == PARTICULA_GLACIAR_ESQUIRLA && p.posicion.y < 0.04f)
        {
            p.posicion.y = 0.04f;
            p.velocidad = { p.velocidad.x * 0.3f, -p.velocidad.y * 0.25f, p.velocidad.z * 0.3f };
        }
        else if (p.tipo == PARTICULA_GLACIAR_CONFETI && p.posicion.y < Y_NIEVE_GLACIAR)
        {
            p.activa = false;
        }
    }
}


static void ActualizarVisualGlaciar(MinijuegoParejasGlaciar& m, const InstantaneaGlaciar& s, float deltaTime)
{
    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        m.destelloBloque[i] -= deltaTime * 1.8f;
        if (m.destelloBloque[i] < 0.0f) m.destelloBloque[i] = 0.0f;
        m.sacudidaBloque[i] -= deltaTime * 2.4f;
        if (m.sacudidaBloque[i] < 0.0f) m.sacudidaBloque[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.destelloJugador[i] -= deltaTime * 2.2f;
        if (m.destelloJugador[i] < 0.0f) m.destelloJugador[i] = 0.0f;
    }

    m.brilloAurora -= deltaTime * 0.22f;
    if (m.brilloAurora < 0.0f) m.brilloAurora = 0.0f;

    if (m.tiempoAnuncioTurno < 99.0f) m.tiempoAnuncioTurno += deltaTime;
    if (m.tiempoSuma < 99.0f) m.tiempoSuma += deltaTime;
    if (m.tiempoCrujidoVisual < 99.0f) m.tiempoCrujidoVisual += deltaTime;

    // Bloques que se derriten: esquirlas y vapor.
    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        if (!s.revelado[i] && m.bloques[i].revelado)
        {
            m.destelloBloque[i] = 1.0f;
            EstallidoBloqueGlaciar(m, i, 9, 5, COLOR_HIELO_CLARO_GLACIAR);
        }
    }

    // Parejas: chispas del jugador y el «+N» sobre su cabeza.
    for (int j = 0; j < MAX_PARTICIPANTES; j++)
    {
        int suma = m.estadosJugadores[j].puntos - s.puntos[j];

        if (suma <= 0 || m.fase != FASE_GLACIAR_REVELANDO)
        {
            continue;
        }

        bool aurora = m.ultimoEvento == EVENTO_GLACIAR_AURORA;
        Color color = m.colorJugador[j];
        m.sumaVisual = suma;
        m.jugadorSuma = j;
        m.tiempoSuma = 0.0f;
        m.destelloJugador[j] = 1.0f;

        if (m.primera >= 0) m.destelloBloque[m.primera] = 1.0f;
        if (m.segunda >= 0) m.destelloBloque[m.segunda] = 1.0f;

        ChispasParejaGlaciar(m, m.primera, j, color, aurora);
        ChispasParejaGlaciar(m, m.segunda, j, color, aurora);

        if (aurora)
        {
            m.brilloAurora = 1.0f;
        }
    }

    // Fallo: los dos bloques se sacuden y sueltan escarcha gris.
    if (s.fase != FASE_GLACIAR_REVELANDO && m.fase == FASE_GLACIAR_REVELANDO && !m.aciertoActual)
    {
        if (m.primera >= 0) m.sacudidaBloque[m.primera] = 1.0f;
        if (m.segunda >= 0) m.sacudidaBloque[m.segunda] = 1.0f;
        EstallidoBloqueGlaciar(m, m.primera, 4, 2, Color{ 150, 162, 180, 255 });
        EstallidoBloqueGlaciar(m, m.segunda, 4, 2, Color{ 150, 162, 180, 255 });
    }

    // El glaciar cruje: temblor fuerte y esquirlas de los dos bloques.
    if (s.fase != FASE_GLACIAR_CRUJIDO && m.fase == FASE_GLACIAR_CRUJIDO)
    {
        m.tiempoCrujidoVisual = 0.0f;
        ActivarTemblorCamaraGeneral(0.22f, 0.45f);
        EstallidoBloqueGlaciar(m, m.crujidoA, 10, 3, COLOR_HIELO_CLARO_GLACIAR);
        EstallidoBloqueGlaciar(m, m.crujidoB, 10, 3, COLOR_HIELO_CLARO_GLACIAR);
    }

    // Turno nuevo (o el mismo jugador que repite tras acertar).
    if (m.fase == FASE_GLACIAR_ELEGIR && (s.fase != FASE_GLACIAR_ELEGIR || s.turno != m.turno))
    {
        m.tiempoAnuncioTurno = 0.0f;
        m.turnoRepetido = s.fase == FASE_GLACIAR_REVELANDO && s.turno == m.turno;
    }

    // Fin: confeti sobre los ganadores.
    if (m.fase == FASE_GLACIAR_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (s.fase != FASE_GLACIAR_TERMINADO)
        {
            m.tiempoFinVisual = 0.0f;
        }

        if (m.tiempoFinVisual < DURACION_FIN_VISUAL_GLACIAR)
        {
            m.tiempoFinVisual += deltaTime;
            const Color colores[5] =
            {
                { 255, 96, 96, 255 }, { 96, 164, 255, 255 }, { 116, 222, 124, 255 },
                { 255, 216, 84, 255 }, { 232, 124, 255, 255 }
            };
            float ritmo = 40.0f * FactorCalidadGrafica(CalidadParticulas());

            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1)
                {
                    continue;
                }

                m.confetiPendiente += ritmo * deltaTime;

                while (m.confetiPendiente >= 1.0f)
                {
                    m.confetiPendiente -= 1.0f;
                    Vector3 p = {
                        m.posicionTempanoX[i] + AzarGlaciar(m, -1.6f, 1.6f),
                        AzarGlaciar(m, 4.5f, 6.0f),
                        m.posicionTempanoZ[i] + AzarGlaciar(m, -1.0f, 1.0f)
                    };
                    int c = (int)AzarGlaciar(m, 0.0f, 4.99f);
                    EmitirParticulaGlaciar(m, PARTICULA_GLACIAR_CONFETI, p,
                        { AzarGlaciar(m, -1.0f, 1.0f), AzarGlaciar(m, 0.5f, 2.0f), AzarGlaciar(m, -0.4f, 0.4f) },
                        AzarGlaciar(m, 2.0f, 3.0f), AzarGlaciar(m, 0.1f, 0.16f), colores[c]);
                }
            }
        }
    }

    ActualizarParticulasGlaciar(m, deltaTime);
}


//--------------------------------------------------
// Cielo (2D): degrade nocturno y estrellas
//--------------------------------------------------

static void DibujarCieloGlaciar(float t, bool efectos)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int a1 = (int)((float)alto * 0.16f);
    int a2 = (int)((float)alto * 0.32f);

    DrawRectangleGradientV(0, 0, ancho, a1, COLOR_CIELO_GLACIAR, Color{ 10, 18, 44, 255 });
    DrawRectangleGradientV(0, a1, ancho, a2 - a1, Color{ 10, 18, 44, 255 }, COLOR_HORIZONTE_GLACIAR);
    DrawRectangle(0, a2, ancho, alto - a2, COLOR_HORIZONTE_GLACIAR);

    for (int k = 0; k < 150; k++)
    {
        float x = RuidoGlaciar(k, 801) * (float)ancho;
        float y = RuidoGlaciar(k, 802) * (float)alto * 0.3f;
        float titila = efectos ? 0.5f + 0.5f * std::sin(t * (0.8f + 2.4f * RuidoGlaciar(k, 803)) + (float)k) : 0.8f;
        float alfa = (0.25f + 0.65f * titila) * (1.0f - y / ((float)alto * 0.34f));
        int lado = RuidoGlaciar(k, 804) > 0.88f ? 2 : 1;
        DrawRectangle((int)x, (int)y, lado, lado, AlfaGlaciar(Color{ 224, 236, 255, 255 }, alfa));
    }
}


//--------------------------------------------------
// Aurora boreal (3D, al fondo): cortinas que ondulan
//--------------------------------------------------

struct PuntoAuroraGlaciar
{
    Vector3 base;
    float alto;
    float intensidad;
};


static PuntoAuroraGlaciar PuntoAuroraEn(int cinta, float u, float t)
{
    PuntoAuroraGlaciar p;
    float c = (float)cinta;
    p.base = {
        -330.0f + 660.0f * u,
        12.5f + 3.0f * c + 2.6f * std::sin(u * 7.1f + c * 2.0f + t * 0.21f) + 0.8f * std::sin(u * 23.0f - t * 0.6f + c),
        -150.0f - 18.0f * c + 16.0f * std::sin(u * 4.3f + c * 1.3f + t * 0.05f)
    };
    p.alto = 22.0f + 8.0f * std::sin(u * 5.3f + t * 0.33f + c * 1.7f);

    float rayo = 0.5f + 0.5f * std::sin(u * 95.0f + t * (1.1f + 0.3f * c) + c * 3.0f);
    float envolvente = 0.5f + 0.5f * std::sin(u * 8.5f + t * 0.12f + c * 2.4f);
    p.intensidad = (0.3f + 0.7f * rayo * rayo) * (0.25f + 0.75f * envolvente);
    return p;
}


static void DibujarAuroraGlaciar(float t, bool efectos, float brillo)
{
    const int columnas = efectos ? 120 : 48;
    float tt = efectos ? t : 0.0f;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    if (efectos)
    {
        BeginBlendMode(BLEND_ADDITIVE);
    }

    for (int cinta = 0; cinta < 3; cinta++)
    {
        float fuerza = (cinta == 0 ? 1.45f : (cinta == 1 ? 1.1f : 0.85f)) * (0.9f + 0.9f * brillo) * (efectos ? 1.0f : 0.55f);
        Color bajo = cinta == 1 ? COLOR_AURORA_CIAN_GLACIAR : COLOR_AURORA_VERDE_GLACIAR;
        Color medio = MezclaGlaciar(bajo, COLOR_AURORA_CIAN_GLACIAR, 0.5f);
        Color arriba = COLOR_AURORA_VIOLETA_GLACIAR;

        rlBegin(RL_QUADS);

        for (int k = 0; k < columnas; k++)
        {
            PuntoAuroraGlaciar a = PuntoAuroraEn(cinta, (float)k / (float)columnas, tt);
            PuntoAuroraGlaciar b = PuntoAuroraEn(cinta, (float)(k + 1) / (float)columnas, tt);
            float ia = a.intensidad * fuerza;
            float ib = b.intensidad * fuerza;

            Vector3 a0 = { a.base.x, a.base.y - 1.2f, a.base.z };
            Vector3 b0 = { b.base.x, b.base.y - 1.2f, b.base.z };
            Vector3 a1 = a.base;
            Vector3 b1 = b.base;
            Vector3 a2 = { a.base.x, a.base.y + a.alto * 0.35f, a.base.z - 2.0f };
            Vector3 b2 = { b.base.x, b.base.y + b.alto * 0.35f, b.base.z - 2.0f };
            Vector3 a3 = { a.base.x, a.base.y + a.alto, a.base.z - 6.0f };
            Vector3 b3 = { b.base.x, b.base.y + b.alto, b.base.z - 6.0f };

            // Borde inferior suave.
            VerticeGlaciar(a0, AlfaGlaciar(bajo, 0.0f));
            VerticeGlaciar(b0, AlfaGlaciar(bajo, 0.0f));
            VerticeGlaciar(b1, AlfaGlaciar(MezclaGlaciar(bajo, WHITE, 0.25f), 0.6f * ib));
            VerticeGlaciar(a1, AlfaGlaciar(MezclaGlaciar(bajo, WHITE, 0.25f), 0.6f * ia));

            // Cuerpo de la cortina.
            VerticeGlaciar(a1, AlfaGlaciar(bajo, 0.55f * ia));
            VerticeGlaciar(b1, AlfaGlaciar(bajo, 0.55f * ib));
            VerticeGlaciar(b2, AlfaGlaciar(medio, 0.3f * ib));
            VerticeGlaciar(a2, AlfaGlaciar(medio, 0.3f * ia));

            // Copete violeta que se apaga.
            VerticeGlaciar(a2, AlfaGlaciar(medio, 0.3f * ia));
            VerticeGlaciar(b2, AlfaGlaciar(medio, 0.3f * ib));
            VerticeGlaciar(b3, AlfaGlaciar(arriba, 0.0f));
            VerticeGlaciar(a3, AlfaGlaciar(arriba, 0.0f));
        }

        rlEnd();
    }

    if (efectos)
    {
        EndBlendMode();
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Horizonte: dos cordilleras y el frente del glaciar
//--------------------------------------------------

static void CordilleraGlaciar(float z, float x0, float x1, float paso, float altoMin, float altoMax, int semilla, Color roca, Color nieve)
{
    int k = 0;
    float x = x0;

    rlBegin(RL_QUADS);

    while (x < x1)
    {
        float h = altoMin + (altoMax - altoMin) * RuidoGlaciar(k, semilla);
        float w = h * (1.2f + 0.6f * RuidoGlaciar(k, semilla + 1));
        float pz = z + 6.0f * (RuidoGlaciar(k, semilla + 2) - 0.5f);
        Vector3 P = { x, h, pz };
        Vector3 L = { x - w, Y_MAR_GLACIAR, pz };
        Vector3 R = { x + w * (0.9f + 0.4f * RuidoGlaciar(k, semilla + 3)), Y_MAR_GLACIAR, pz };
        Vector3 F = { x + w * 0.2f * (RuidoGlaciar(k, semilla + 4) - 0.5f), Y_MAR_GLACIAR, pz + w * 0.55f };
        float linea = 0.55f + 0.2f * RuidoGlaciar(k, semilla + 5);

        Vector3 Ls = SumaGlaciar(L, EscalaGlaciar(RestaGlaciar(P, L), linea));
        Vector3 Fs = SumaGlaciar(F, EscalaGlaciar(RestaGlaciar(P, F), linea));
        Vector3 Rs = SumaGlaciar(R, EscalaGlaciar(RestaGlaciar(P, R), linea));

        // Ladera iluminada (izquierda) y en sombra (derecha).
        QuadNieblaGlaciar(L, F, Fs, Ls, TonoGlaciar(roca, 0.95f));
        QuadNieblaGlaciar(F, R, Rs, Fs, TonoGlaciar(roca, 0.66f));
        QuadNieblaGlaciar(Ls, Fs, P, P, nieve);
        QuadNieblaGlaciar(Fs, Rs, P, P, TonoGlaciar(nieve, 0.78f));

        x += paso * (0.7f + 0.6f * RuidoGlaciar(k, semilla + 6));
        k++;
    }

    rlEnd();
}


// Frente del glaciar sobre el mar: columnas de hielo (seracs) de alturas
// distintas, con estratos y nieve arriba; detras, la meseta helada.
static void DibujarFrenteGlaciar(int paso)
{
    float x = -140.0f;
    int k = 0;

    rlBegin(RL_QUADS);

    while (x < 140.0f)
    {
        float w = (2.0f + 2.4f * RuidoGlaciar(k, 911)) * (float)paso;
        float h = 4.4f + 2.6f * RuidoGlaciar(k, 912);
        if (RuidoGlaciar(k, 913) > 0.82f) h += 1.8f;
        float z = -38.0f + 1.8f * (RuidoGlaciar(k, 914) - 0.5f);
        float x1 = x + w;
        float fondo = z - 5.0f;
        Color claro = MezclaGlaciar(COLOR_HIELO_CLARO_GLACIAR, COLOR_NIEVE_CLARA_GLACIAR, RuidoGlaciar(k, 915) * 0.6f);
        Color bajo = MezclaGlaciar(COLOR_HIELO_PROFUNDO_GLACIAR, COLOR_HIELO_GLACIAR, RuidoGlaciar(k, 916) * 0.5f);
        Color medio = MezclaGlaciar(bajo, claro, 0.55f);
        float estrato = h * (0.35f + 0.25f * RuidoGlaciar(k, 917));

        // Frente con degrade vertical y un estrato mas oscuro.
        VerticeGlaciar({ x, Y_MAR_GLACIAR, z }, TinteGlaciar(bajo, { x, Y_MAR_GLACIAR, z }));
        VerticeGlaciar({ x1, Y_MAR_GLACIAR, z }, TinteGlaciar(bajo, { x1, Y_MAR_GLACIAR, z }));
        VerticeGlaciar({ x1, estrato, z }, TinteGlaciar(TonoGlaciar(medio, 0.8f), { x1, estrato, z }));
        VerticeGlaciar({ x, estrato, z }, TinteGlaciar(TonoGlaciar(medio, 0.8f), { x, estrato, z }));

        VerticeGlaciar({ x, estrato, z }, TinteGlaciar(medio, { x, estrato, z }));
        VerticeGlaciar({ x1, estrato, z }, TinteGlaciar(medio, { x1, estrato, z }));
        VerticeGlaciar({ x1, h, z }, TinteGlaciar(claro, { x1, h, z }));
        VerticeGlaciar({ x, h, z }, TinteGlaciar(claro, { x, h, z }));

        // Nieve arriba y costados (grietas en sombra).
        QuadNieblaGlaciar({ x, h, z }, { x1, h, z }, { x1, h, fondo }, { x, h, fondo }, COLOR_NIEVE_CLARA_GLACIAR);
        QuadNieblaGlaciar({ x, Y_MAR_GLACIAR, z }, { x, Y_MAR_GLACIAR, fondo }, { x, h, fondo }, { x, h, z }, TonoGlaciar(bajo, 0.7f));
        QuadNieblaGlaciar({ x1, Y_MAR_GLACIAR, z }, { x1, Y_MAR_GLACIAR, fondo }, { x1, h, fondo }, { x1, h, z }, TonoGlaciar(bajo, 0.6f));

        // Grieta vertical oscura en algunas columnas.
        if (RuidoGlaciar(k, 918) > 0.55f)
        {
            float gx = x + w * (0.3f + 0.4f * RuidoGlaciar(k, 919));
            float gw = 0.18f + 0.2f * RuidoGlaciar(k, 920);
            QuadNieblaGlaciar({ gx, Y_MAR_GLACIAR + 0.6f, z + 0.03f }, { gx + gw, Y_MAR_GLACIAR + 0.6f, z + 0.03f },
                { gx + gw * 0.4f, h - 0.3f, z + 0.03f }, { gx, h - 0.3f, z + 0.03f }, COLOR_HIELO_PROFUNDO_GLACIAR);
        }

        x = x1;
        k++;
    }

    // Meseta del glaciar que sube hacia las montanas.
    for (int s = 0; s < 8; s++)
    {
        float xa = -200.0f + 50.0f * (float)s;
        float xb = xa + 50.0f;
        QuadNieblaGlaciar({ xa, 5.2f, -41.0f }, { xb, 5.2f, -41.0f }, { xb, 8.0f, -70.0f }, { xa, 8.0f, -70.0f },
            TonoGlaciar(COLOR_NIEVE_GLACIAR, 0.95f));
    }

    rlEnd();
}


//--------------------------------------------------
// Mar helado, icebergs y tempanos a la deriva
//--------------------------------------------------

static Color ColorMarGlaciar(float x, float z, float t, bool efectos)
{
    float tt = efectos ? t : 0.0f;
    float ola = 0.5f + 0.25f * std::sin(x * 0.35f + z * 0.22f + tt * 0.6f) + 0.25f * std::sin(x * 0.13f - z * 0.41f - tt * 0.45f);
    return MezclaGlaciar(COLOR_AGUA_GLACIAR, COLOR_AGUA_CLARA_GLACIAR, 0.15f + 0.55f * ola);
}


static void DibujarMarGlaciar(float t, bool efectos)
{
    rlBegin(RL_QUADS);

    // Cerca: celdas chicas con oleaje; lejos, celdas grandes.
    for (int fila = 0; fila < 2; fila++)
    {
        float z0 = fila == 0 ? 16.0f : -40.0f;
        float z1 = fila == 0 ? -40.0f : -150.0f;
        float pz = fila == 0 ? 2.5f : 11.0f;
        float px = fila == 0 ? 4.0f : 22.0f;
        float ancho = fila == 0 ? 64.0f : 240.0f;

        for (float z = z0; z > z1 + 0.01f; z -= pz)
        {
            float za = z, zb = z - pz;

            for (float x = -ancho; x < ancho - 0.01f; x += px)
            {
                float xa = x, xb = x + px;
                Vector3 p0 = { xa, Y_MAR_GLACIAR, za };
                Vector3 p1 = { xb, Y_MAR_GLACIAR, za };
                Vector3 p2 = { xb, Y_MAR_GLACIAR, zb };
                Vector3 p3 = { xa, Y_MAR_GLACIAR, zb };
                VerticeGlaciar(p0, TinteGlaciar(ColorMarGlaciar(xa, za, t, efectos), p0));
                VerticeGlaciar(p1, TinteGlaciar(ColorMarGlaciar(xb, za, t, efectos), p1));
                VerticeGlaciar(p2, TinteGlaciar(ColorMarGlaciar(xb, zb, t, efectos), p2));
                VerticeGlaciar(p3, TinteGlaciar(ColorMarGlaciar(xa, zb, t, efectos), p3));
            }
        }
    }

    rlEnd();
}


static float VaivenGlaciar(float t, int k, bool efectos)
{
    return efectos ? std::sin(t * 0.45f + (float)k * 1.7f) * 0.06f : 0.0f;
}


static void DibujarIcebergGlaciar(const IcebergGlaciar& b, int k, float t, bool efectos)
{
    float v = VaivenGlaciar(t, k, efectos);

    if (b.tipo == 0)
    {
        // Tabular: paredes azules, tapa de nieve y un escalon.
        float arriba = Y_MAR_GLACIAR + b.alto + v;
        PrismaGlaciar({ b.x, Y_MAR_GLACIAR - 0.8f, b.z }, { b.x, arriba, b.z }, b.radio, b.radio * 0.9f, 7, COLOR_HIELO_GLACIAR, 0.0f, (float)k * 0.7f);
        PrismaGlaciar({ b.x, arriba, b.z }, { b.x, arriba + 0.3f, b.z }, b.radio * 0.9f, b.radio * 0.84f, 7, COLOR_NIEVE_CLARA_GLACIAR, 0.0f, (float)k * 0.7f);
        PrismaGlaciar({ b.x + b.radio * 0.35f, arriba, b.z - b.radio * 0.2f }, { b.x + b.radio * 0.35f, arriba + b.alto * 0.45f, b.z - b.radio * 0.2f },
            b.radio * 0.46f, b.radio * 0.38f, 6, COLOR_HIELO_CLARO_GLACIAR, 0.0f, (float)k);
        PrismaGlaciar({ b.x + b.radio * 0.35f, arriba + b.alto * 0.45f, b.z - b.radio * 0.2f }, { b.x + b.radio * 0.35f, arriba + b.alto * 0.45f + 0.2f, b.z - b.radio * 0.2f },
            b.radio * 0.38f, b.radio * 0.33f, 6, COLOR_NIEVE_CLARA_GLACIAR, 0.0f, (float)k);
    }
    else
    {
        // Aguja: bloque facetado alto con la punta nevada.
        RocaGlaciar({ b.x, Y_MAR_GLACIAR + b.alto * 0.32f + v, b.z }, { b.radio, b.alto * 0.7f, b.radio * 0.8f }, 300 + k, COLOR_HIELO_GLACIAR, 0.24f);
        RocaGlaciar({ b.x + b.radio * 0.2f, Y_MAR_GLACIAR + b.alto * 0.86f + v, b.z }, { b.radio * 0.42f, b.alto * 0.36f, b.radio * 0.4f }, 320 + k, COLOR_HIELO_CLARO_GLACIAR, 0.2f);
    }
}


// Tempanos chicos que derivan despacio entre la banquisa y el glaciar.
static Vector3 PosicionTempanoDerivaGlaciar(int k, float t, bool efectos)
{
    float velocidad = efectos ? 0.25f + 0.35f * RuidoGlaciar(k, 931) : 0.0f;
    float x = std::fmod(RuidoGlaciar(k, 932) * 100.0f + t * velocidad, 100.0f) - 50.0f;
    float z = -17.5f - 13.0f * RuidoGlaciar(k, 933);
    return { x, Y_MAR_GLACIAR + VaivenGlaciar(t, k + 20, efectos), z };
}


static void DibujarTempanosDerivaGlaciar(float t, bool efectos, int cantidad)
{
    for (int k = 0; k < cantidad; k++)
    {
        Vector3 p = PosicionTempanoDerivaGlaciar(k, t, efectos);
        float r = 0.7f + 0.9f * RuidoGlaciar(k, 934);
        PrismaGlaciar({ p.x, p.y - 0.15f, p.z }, { p.x, p.y + 0.18f, p.z }, r, r * 0.9f, 6, COLOR_NIEVE_CLARA_GLACIAR, 0.0f, (float)k);
    }
}


static void FocaGlaciar(Vector3 base, float giro, float t, bool efectos)
{
    float cabeza = efectos ? std::sin(t * 0.7f) : 0.0f;
    float alza = cabeza > 0.4f ? (cabeza - 0.4f) * 0.6f : 0.0f;

    nieblaFijaGlaciar = CantidadNieblaGlaciar(base);
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    RocaGlaciar({ 0.0f, 0.3f, 0.0f }, { 0.95f, 0.34f, 0.42f }, 41, COLOR_FOCA_GLACIAR, 0.05f);
    RocaGlaciar({ -0.95f, 0.2f, 0.0f }, { 0.4f, 0.18f, 0.26f }, 42, TonoGlaciar(COLOR_FOCA_GLACIAR, 0.9f), 0.05f);
    RocaGlaciar({ 0.95f, 0.5f + alza, 0.0f }, { 0.3f, 0.27f, 0.27f }, 43, COLOR_FOCA_GLACIAR, 0.04f);
    CuboCrudoGlaciar({ 1.22f, 0.48f + alza, 0.0f }, { 0.12f, 0.1f, 0.12f }, TinteGlaciar(Color{ 70, 72, 84, 255 }, base));
    CuboCrudoGlaciar({ 1.1f, 0.6f + alza, 0.12f }, { 0.05f, 0.05f, 0.05f }, BLACK);
    CuboCrudoGlaciar({ 1.1f, 0.6f + alza, -0.12f }, { 0.05f, 0.05f, 0.05f }, BLACK);
    CuboCrudoGlaciar({ 0.3f, 0.12f, 0.42f }, { 0.36f, 0.06f, 0.18f }, TinteGlaciar(TonoGlaciar(COLOR_FOCA_GLACIAR, 0.8f), base));
    CuboCrudoGlaciar({ 0.3f, 0.12f, -0.42f }, { 0.36f, 0.06f, 0.18f }, TinteGlaciar(TonoGlaciar(COLOR_FOCA_GLACIAR, 0.8f), base));
    rlPopMatrix();
    nieblaFijaGlaciar = -1.0f;
}


//--------------------------------------------------
// Banquisa: superficie nevada, borde con grosor y grietas
//--------------------------------------------------

static Vector3 PuntoBanquisaGlaciar(int indice, float f, float y)
{
    Vector2 p = CONTORNO_BANQUISA_GLACIAR[indice];
    return {
        CENTRO_BANQUISA_GLACIAR.x + (p.x - CENTRO_BANQUISA_GLACIAR.x) * f,
        y,
        CENTRO_BANQUISA_GLACIAR.y + (p.y - CENTRO_BANQUISA_GLACIAR.y) * f
    };
}


static void DibujarBanquisaGlaciar()
{
    const float anillos[] = { 0.0f, 0.22f, 0.4f, 0.56f, 0.7f, 0.82f, 0.92f, 1.0f };
    const int cantidadAnillos = (int)(sizeof(anillos) / sizeof(anillos[0]));

    rlBegin(RL_QUADS);

    for (int i = 0; i < CANTIDAD_CONTORNO_GLACIAR - 1; i++)
    {
        for (int a = 0; a < cantidadAnillos - 1; a++)
        {
            float f0 = anillos[a], f1 = anillos[a + 1];
            float r = RuidoGlaciar(i * 17 + a, 951);
            Color c = MezclaGlaciar(COLOR_NIEVE_GLACIAR, COLOR_NIEVE_CLARA_GLACIAR, 0.12f + 0.28f * r);
            c = TonoGlaciar(c, 0.93f + 0.1f * RuidoGlaciar(i * 17 + a, 952));

            QuadNieblaGlaciar(
                PuntoBanquisaGlaciar(i, f0, Y_NIEVE_GLACIAR),
                PuntoBanquisaGlaciar(i + 1, f0, Y_NIEVE_GLACIAR),
                PuntoBanquisaGlaciar(i + 1, f1, Y_NIEVE_GLACIAR),
                PuntoBanquisaGlaciar(i, f1, Y_NIEVE_GLACIAR),
                c
            );
        }

        // Borde: labio de nieve y pared de hielo hasta el agua.
        Vector3 a = PuntoBanquisaGlaciar(i, 1.0f, Y_NIEVE_GLACIAR);
        Vector3 b = PuntoBanquisaGlaciar(i + 1, 1.0f, Y_NIEVE_GLACIAR);
        Vector3 n = NormalizarGlaciar({ b.z - a.z, 0.0f, -(b.x - a.x) });
        float luz = LuzNormalGlaciar(n);
        Vector3 a1 = { a.x + n.x * 0.12f, Y_NIEVE_GLACIAR - 0.3f, a.z + n.z * 0.12f };
        Vector3 b1 = { b.x + n.x * 0.12f, Y_NIEVE_GLACIAR - 0.3f, b.z + n.z * 0.12f };
        Vector3 a2 = { a.x + n.x * 0.35f, Y_MAR_GLACIAR - 0.4f, a.z + n.z * 0.35f };
        Vector3 b2 = { b.x + n.x * 0.35f, Y_MAR_GLACIAR - 0.4f, b.z + n.z * 0.35f };

        QuadNieblaGlaciar(a, b, b1, a1, TonoGlaciar(COLOR_NIEVE_CLARA_GLACIAR, luz));
        VerticeGlaciar(a1, TinteGlaciar(TonoGlaciar(COLOR_HIELO_GLACIAR, luz), a1));
        VerticeGlaciar(b1, TinteGlaciar(TonoGlaciar(COLOR_HIELO_GLACIAR, luz), b1));
        VerticeGlaciar(b2, TinteGlaciar(TonoGlaciar(COLOR_HIELO_PROFUNDO_GLACIAR, luz), b2));
        VerticeGlaciar(a2, TinteGlaciar(TonoGlaciar(COLOR_HIELO_PROFUNDO_GLACIAR, luz), a2));
    }

    rlEnd();

    // Grietas finas en la nieve (desde las esquinas del altar hacia afuera).
    rlBegin(RL_QUADS);
    for (int g = 0; g < 10; g++)
    {
        float ang = 6.2832f * ((float)g + 0.3f * RuidoGlaciar(g, 961)) / 10.0f;
        Vector3 p = { std::cos(ang) * 4.9f, Y_NIEVE_GLACIAR + 0.012f, std::sin(ang) * 4.9f };

        for (int s = 0; s < 4; s++)
        {
            float largo = 0.7f + 0.6f * RuidoGlaciar(g * 5 + s, 962);
            ang += (RuidoGlaciar(g * 5 + s, 963) - 0.5f) * 0.9f;
            Vector3 q = { p.x + std::cos(ang) * largo, p.y, p.z + std::sin(ang) * largo };
            float ancho = 0.05f * (1.0f - 0.2f * (float)s);
            Vector3 lado = { -std::sin(ang) * ancho, 0.0f, std::cos(ang) * ancho };
            QuadNieblaGlaciar(SumaGlaciar(p, lado), SumaGlaciar(q, lado), RestaGlaciar(q, lado), RestaGlaciar(p, lado), Color{ 92, 112, 156, 255 });
            p = q;
        }
    }
    rlEnd();
}


//--------------------------------------------------
// Cueva de hielo sobre la banquisa
//--------------------------------------------------

static void DibujarCuevaGlaciar(float t, bool decoracion)
{
    (void)t;
    Vector3 c = CUEVA_GLACIAR;
    float r = RADIO_CUEVA_GLACIAR;

    // Loma de bloques de hielo que forma la cueva.
    RocaGlaciar({ c.x - 2.6f, c.y + 1.2f, c.z - 2.0f }, { 3.2f, 2.9f, 2.6f }, 501, COLOR_HIELO_CLARO_GLACIAR, 0.2f);
    RocaGlaciar({ c.x + 1.6f, c.y + 1.5f, c.z - 2.4f }, { 3.0f, 3.4f, 2.4f }, 502, COLOR_HIELO_GLACIAR, 0.2f);
    RocaGlaciar({ c.x + 4.4f, c.y + 0.7f, c.z - 1.2f }, { 2.0f, 1.6f, 1.7f }, 503, COLOR_HIELO_CLARO_GLACIAR, 0.18f);
    RocaGlaciar({ c.x - 5.2f, c.y + 0.6f, c.z + 0.2f }, { 1.9f, 1.4f, 1.7f }, 504, COLOR_HIELO_GLACIAR, 0.2f);

    if (decoracion)
    {
        RocaGlaciar({ c.x - 0.6f, c.y + 3.7f, c.z - 3.0f }, { 2.6f, 1.4f, 2.0f }, 505, COLOR_NIEVE_CLARA_GLACIAR, 0.16f);
        RocaGlaciar({ c.x + 3.0f, c.y + 2.6f, c.z - 3.4f }, { 1.6f, 1.6f, 1.5f }, 506, COLOR_HIELO_CLARO_GLACIAR, 0.22f);
    }

    // Boca oscura (media elipse vertical) con el fondo azul profundo.
    rlBegin(RL_TRIANGLES);
    const int LADOS = 14;
    Vector3 centro = { c.x, c.y, c.z + 0.05f };

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 3.1416f * (float)k / (float)LADOS;
        float a1 = 3.1416f * (float)(k + 1) / (float)LADOS;
        VerticeGlaciar(centro, Color{ 10, 40, 80, 255 });
        VerticeGlaciar({ c.x + std::cos(a0) * r, c.y + std::sin(a0) * r * 1.3f, c.z + 0.05f }, Color{ 6, 18, 40, 255 });
        VerticeGlaciar({ c.x + std::cos(a1) * r, c.y + std::sin(a1) * r * 1.3f, c.z + 0.05f }, Color{ 6, 18, 40, 255 });
    }
    rlEnd();

    // Dovelas de hielo alrededor de la boca.
    for (int k = 0; k <= 8; k++)
    {
        float a = 3.1416f * (float)k / 8.0f;
        Vector3 p = { c.x + std::cos(a) * (r + 0.25f), c.y + std::sin(a) * (r * 1.3f + 0.25f), c.z + 0.2f };
        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(a * RAD2DEG - 90.0f, 0.0f, 0.0f, 1.0f);
        CuboCrudoGlaciar({ 0.0f, 0.0f, 0.0f }, { 0.6f, 0.5f, 0.55f }, TinteGlaciar(k % 2 ? COLOR_HIELO_CLARO_GLACIAR : COLOR_NIEVE_CLARA_GLACIAR, p));
        rlPopMatrix();
    }

    // Carambanos colgando del arco.
    for (int k = 0; k < 7; k++)
    {
        float a = 0.45f + 2.25f * (float)k / 6.0f;
        Vector3 p = { c.x + std::cos(a) * r * 0.92f, c.y + std::sin(a) * r * 1.2f, c.z + 0.12f };
        float largo = 0.35f + 0.45f * RuidoGlaciar(k, 507);
        PrismaGlaciar(p, { p.x, p.y - largo, p.z }, 0.08f, 0.0f, 5, COLOR_HIELO_CLARO_GLACIAR, 0.3f);
    }
}


//--------------------------------------------------
// Utileria
//--------------------------------------------------

static float BalanceoFarolGlaciar(float t, int k)
{
    return std::sin(t * 1.3f + (float)k * 2.1f) * 4.0f;
}


static Vector3 FarolGlaciar(int k, float t)
{
    Vector3 base = POSTES_FAROL_GLACIAR[k];
    float lado = base.x < 0.0f ? 1.0f : -1.0f;
    float ang = BalanceoFarolGlaciar(t, k) * DEG2RAD;
    Vector3 gancho = { base.x + lado * 0.62f, base.y + 2.72f, base.z };
    return { gancho.x + std::sin(ang) * 0.42f, gancho.y - std::cos(ang) * 0.42f, gancho.z };
}


static void PosteFarolGlaciar(int k, float t)
{
    Vector3 base = POSTES_FAROL_GLACIAR[k];
    float lado = base.x < 0.0f ? 1.0f : -1.0f;
    Vector3 l = FarolGlaciar(k, t);
    Vector3 gancho = { base.x + lado * 0.62f, base.y + 2.72f, base.z };

    RocaGlaciar({ base.x, base.y + 0.05f, base.z }, { 0.45f, 0.22f, 0.4f }, 601 + k, COLOR_NIEVE_CLARA_GLACIAR, 0.12f);
    PrismaGlaciar(base, { base.x, base.y + 2.9f, base.z }, 0.1f, 0.08f, 6, COLOR_MADERA_OSCURA_GLACIAR);
    CuboGlaciar({ base.x + lado * 0.32f, base.y + 2.76f, base.z }, { 0.72f, 0.09f, 0.09f }, COLOR_MADERA_GLACIAR);
    CuboGlaciar({ base.x + lado * 0.32f, base.y + 2.83f, base.z }, { 0.7f, 0.05f, 0.11f }, COLOR_NIEVE_CLARA_GLACIAR);
    PrismaGlaciar(gancho, { l.x, l.y + 0.2f, l.z }, 0.015f, 0.015f, 4, COLOR_HIERRO_GLACIAR);

    // Farol: tapa, vidrio encendido, base y barrotes.
    CuboGlaciar({ l.x, l.y + 0.2f, l.z }, { 0.32f, 0.06f, 0.32f }, COLOR_HIERRO_GLACIAR);
    CuboCrudoGlaciar({ l.x, l.y, l.z }, { 0.24f, 0.32f, 0.24f }, COLOR_VIDRIO_GLACIAR);
    CuboGlaciar({ l.x, l.y - 0.19f, l.z }, { 0.3f, 0.06f, 0.3f }, COLOR_HIERRO_GLACIAR);

    for (int e = 0; e < 4; e++)
    {
        float ex = (e & 1) ? 0.13f : -0.13f;
        float ez = (e & 2) ? 0.13f : -0.13f;
        CuboCrudoGlaciar({ l.x + ex, l.y, l.z + ez }, { 0.035f, 0.34f, 0.035f }, COLOR_HIERRO_GLACIAR);
    }
}


// Pinguino: cuerpo oscuro, panza blanca, pico y patas naranjas. Se
// balancea, gira la cabeza, aletea y a veces da un saltito.
static void PinguinoGlaciar(Vector3 base, float giro, float escala, float t, int semilla, bool efectos)
{
    float s = (float)semilla;
    float tt = efectos ? t : 0.0f;
    float balanceo = std::sin(tt * 2.6f + s) * 7.0f;
    float cabeza = std::sin(tt * 0.9f + s * 2.3f) * 35.0f;
    float pulso = std::sin(tt * 1.1f + s * 1.7f);
    float salto = pulso > 0.0f ? std::pow(pulso, 10.0f) * 0.22f : 0.0f;
    float aleteo = salto > 0.02f ? 40.0f : 12.0f + 6.0f * std::sin(tt * 5.0f + s);

    nieblaFijaGlaciar = CantidadNieblaGlaciar(base);
    rlPushMatrix();
    rlTranslatef(base.x, base.y + salto * escala, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(balanceo, 0.0f, 0.0f, 1.0f);
    rlScalef(escala, escala, escala);

    CuboGlaciar({ -0.12f, 0.03f, 0.1f }, { 0.14f, 0.06f, 0.22f }, COLOR_PICO_GLACIAR);
    CuboGlaciar({ 0.12f, 0.03f, 0.1f }, { 0.14f, 0.06f, 0.22f }, COLOR_PICO_GLACIAR);
    RocaGlaciar({ 0.0f, 0.52f, 0.0f }, { 0.31f, 0.47f, 0.28f }, 70 + semilla, COLOR_PLUMA_GLACIAR, 0.04f);
    RocaGlaciar({ 0.0f, 0.48f, 0.1f }, { 0.24f, 0.38f, 0.2f }, 80 + semilla, COLOR_PLUMA_CLARA_GLACIAR, 0.04f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        rlPushMatrix();
        rlTranslatef(0.3f * (float)lado, 0.7f, 0.0f);
        rlRotatef(aleteo * (float)lado, 0.0f, 0.0f, 1.0f);
        CuboGlaciar({ 0.0f, -0.2f, 0.0f }, { 0.06f, 0.4f, 0.16f }, COLOR_PLUMA_GLACIAR);
        rlPopMatrix();
    }

    rlPushMatrix();
    rlTranslatef(0.0f, 1.04f, 0.0f);
    rlRotatef(cabeza, 0.0f, 1.0f, 0.0f);
    RocaGlaciar({ 0.0f, 0.0f, 0.0f }, { 0.21f, 0.2f, 0.21f }, 90 + semilla, COLOR_PLUMA_GLACIAR, 0.04f);
    CuboGlaciar({ -0.1f, 0.02f, 0.15f }, { 0.07f, 0.07f, 0.04f }, COLOR_PLUMA_CLARA_GLACIAR);
    CuboGlaciar({ 0.1f, 0.02f, 0.15f }, { 0.07f, 0.07f, 0.04f }, COLOR_PLUMA_CLARA_GLACIAR);
    CuboCrudoGlaciar({ -0.1f, 0.02f, 0.175f }, { 0.03f, 0.03f, 0.02f }, BLACK);
    CuboCrudoGlaciar({ 0.1f, 0.02f, 0.175f }, { 0.03f, 0.03f, 0.02f }, BLACK);
    PrismaGlaciar({ 0.0f, -0.04f, 0.16f }, { 0.0f, -0.07f, 0.36f }, 0.06f, 0.0f, 4, COLOR_PICO_GLACIAR, 0.2f);
    rlPopMatrix();

    rlPopMatrix();
    nieblaFijaGlaciar = -1.0f;
}


static void TrineoGlaciar(Vector3 base, float giro)
{
    nieblaFijaGlaciar = CantidadNieblaGlaciar(base);
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    // Patines con la punta curvada, montantes y tablas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = 0.5f * (float)lado;
        CuboGlaciar({ x, 0.04f, 0.0f }, { 0.07f, 0.06f, 2.2f }, COLOR_HIERRO_GLACIAR);
        rlPushMatrix();
        rlTranslatef(x, 0.18f, 1.18f);
        rlRotatef(-50.0f, 1.0f, 0.0f, 0.0f);
        CuboGlaciar({ 0.0f, 0.0f, 0.0f }, { 0.07f, 0.06f, 0.42f }, COLOR_HIERRO_GLACIAR);
        rlPopMatrix();

        for (int m = -1; m <= 1; m++)
        {
            CuboGlaciar({ x, 0.2f, 0.75f * (float)m }, { 0.07f, 0.28f, 0.07f }, COLOR_MADERA_OSCURA_GLACIAR);
        }
    }

    for (int tabla = 0; tabla < 5; tabla++)
    {
        CuboGlaciar({ 0.0f, 0.36f, -0.9f + 0.45f * (float)tabla }, { 1.15f, 0.05f, 0.38f }, TonoGlaciar(COLOR_MADERA_GLACIAR, 0.92f + 0.08f * (float)(tabla & 1)));
    }

    // Cajas atadas con nieve encima.
    CuboGlaciar({ 0.0f, 0.68f, -0.35f }, { 0.9f, 0.6f, 0.75f }, COLOR_MADERA_GLACIAR);
    CuboGlaciar({ 0.0f, 0.68f, -0.35f }, { 0.94f, 0.08f, 0.79f }, COLOR_MADERA_OSCURA_GLACIAR);
    CuboGlaciar({ 0.0f, 1.0f, -0.35f }, { 0.86f, 0.06f, 0.7f }, COLOR_NIEVE_CLARA_GLACIAR);
    CuboGlaciar({ 0.05f, 0.62f, 0.5f }, { 0.7f, 0.48f, 0.55f }, TonoGlaciar(COLOR_MADERA_GLACIAR, 0.85f));
    CuboGlaciar({ 0.05f, 0.62f, 0.5f }, { 0.08f, 0.5f, 0.59f }, COLOR_MADERA_OSCURA_GLACIAR);
    CuboGlaciar({ 0.05f, 0.88f, 0.5f }, { 0.64f, 0.05f, 0.5f }, COLOR_NIEVE_CLARA_GLACIAR);
    CuboGlaciar({ 0.0f, 1.0f, -0.35f }, { 0.04f, 0.04f, 0.8f }, Color{ 150, 120, 80, 255 });

    rlPopMatrix();
    nieblaFijaGlaciar = -1.0f;
}


static void BarrilGlaciar(Vector3 base)
{
    PrismaGlaciar(base, { base.x, base.y + 0.85f, base.z }, 0.34f, 0.32f, 10, COLOR_MADERA_GLACIAR);
    PrismaGlaciar({ base.x, base.y + 0.18f, base.z }, { base.x, base.y + 0.24f, base.z }, 0.355f, 0.355f, 10, COLOR_HIERRO_GLACIAR);
    PrismaGlaciar({ base.x, base.y + 0.62f, base.z }, { base.x, base.y + 0.68f, base.z }, 0.345f, 0.345f, 10, COLOR_HIERRO_GLACIAR);
    PrismaGlaciar({ base.x, base.y + 0.85f, base.z }, { base.x, base.y + 0.92f, base.z }, 0.3f, 0.24f, 10, COLOR_NIEVE_CLARA_GLACIAR);
}


// Bandera de expedicion que ondea (tela por franjas, en el mundo).
static void BanderaGlaciar(Vector3 base, float t, bool efectos)
{
    float alto = 3.4f;
    PrismaGlaciar(base, { base.x, base.y + alto, base.z }, 0.06f, 0.05f, 6, COLOR_HIERRO_GLACIAR);
    PrismaGlaciar({ base.x, base.y + alto, base.z }, { base.x, base.y + alto + 0.1f, base.z }, 0.09f, 0.0f, 6, Color{ 220, 200, 120, 255 });

    const int TRAMOS = 8;
    float largo = 1.5f, ancho = 0.9f;
    float tt = efectos ? t : 0.0f;

    rlBegin(RL_QUADS);
    for (int k = 0; k < TRAMOS; k++)
    {
        float u0 = (float)k / (float)TRAMOS, u1 = (float)(k + 1) / (float)TRAMOS;
        float o0 = std::sin(tt * 4.0f - u0 * 5.0f) * 0.16f * u0;
        float o1 = std::sin(tt * 4.0f - u1 * 5.0f) * 0.16f * u1;
        float y0 = base.y + alto - 0.05f;
        Vector3 a = { base.x + largo * u0, y0 - u0 * 0.08f, base.z + o0 };
        Vector3 b = { base.x + largo * u1, y0 - u1 * 0.08f, base.z + o1 };
        Vector3 c = { b.x, b.y - ancho, b.z };
        Vector3 d = { a.x, a.y - ancho, a.z };
        float luz = 0.82f + 0.18f * std::cos(tt * 4.0f - u0 * 5.0f);
        bool franja = k == 2 || k == 3;
        QuadNieblaGlaciar(a, b, c, d, TonoGlaciar(franja ? COLOR_PLUMA_CLARA_GLACIAR : COLOR_BANDERA_GLACIAR, luz));

        // Franja horizontal blanca (cruz).
        Vector3 e = { a.x, a.y - ancho * 0.4f, a.z + 0.005f };
        Vector3 f = { b.x, b.y - ancho * 0.4f, b.z + 0.005f };
        Vector3 g = { b.x, b.y - ancho * 0.6f, b.z + 0.005f };
        Vector3 h = { a.x, a.y - ancho * 0.6f, a.z + 0.005f };
        QuadNieblaGlaciar(e, f, g, h, TonoGlaciar(COLOR_PLUMA_CLARA_GLACIAR, luz));
    }
    rlEnd();
}


// Racimo de cristales de hielo (prismas hexagonales con punta).
static void CristalesGlaciar(Vector3 base, float escala, int semilla)
{
    RocaGlaciar({ base.x, base.y + 0.05f, base.z }, { 0.8f * escala, 0.3f * escala, 0.7f * escala }, semilla, COLOR_NIEVE_CLARA_GLACIAR, 0.15f);

    for (int k = 0; k < 6; k++)
    {
        float ang = 6.2832f * RuidoGlaciar(k, semilla + 1);
        float inclinacion = k == 0 ? 0.05f : 0.25f + 0.35f * RuidoGlaciar(k, semilla + 2);
        Vector3 dir = NormalizarGlaciar({ std::cos(ang) * inclinacion, 1.0f, std::sin(ang) * inclinacion });
        float largo = (k == 0 ? 2.0f : 0.7f + 1.1f * RuidoGlaciar(k, semilla + 3)) * escala;
        float radio = (k == 0 ? 0.26f : 0.12f + 0.1f * RuidoGlaciar(k, semilla + 4)) * escala;
        Vector3 a = { base.x + std::cos(ang) * 0.25f * escala * (k ? 1.0f : 0.0f), base.y, base.z + std::sin(ang) * 0.25f * escala * (k ? 1.0f : 0.0f) };
        Vector3 b = SumaGlaciar(a, EscalaGlaciar(dir, largo));
        Color c = MezclaGlaciar(COLOR_CRISTAL_GLACIAR, COLOR_HIELO_CLARO_GLACIAR, RuidoGlaciar(k, semilla + 5) * 0.5f);
        PrismaGlaciar(a, b, radio, radio * 0.9f, 6, c, 0.45f, ang);
        PrismaGlaciar(b, SumaGlaciar(b, EscalaGlaciar(dir, radio * 2.4f)), radio * 0.9f, 0.0f, 6, MezclaGlaciar(c, WHITE, 0.4f), 0.6f, ang);
    }
}


static Vector3 PuntaCristalGlaciar(Vector3 base, float escala)
{
    return { base.x, base.y + 2.4f * escala, base.z };
}


// Inukshuk: figura de piedras apiladas con nieve en los hombros.
static void InukshukGlaciar(Vector3 base, float giro)
{
    nieblaFijaGlaciar = CantidadNieblaGlaciar(base);
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    CuboGlaciar({ -0.32f, 0.3f, 0.0f }, { 0.3f, 0.6f, 0.34f }, COLOR_PIEDRA_GLACIAR);
    CuboGlaciar({ 0.32f, 0.3f, 0.0f }, { 0.3f, 0.6f, 0.34f }, TonoGlaciar(COLOR_PIEDRA_GLACIAR, 0.9f));
    CuboGlaciar({ 0.0f, 0.7f, 0.0f }, { 1.1f, 0.2f, 0.4f }, TonoGlaciar(COLOR_PIEDRA_GLACIAR, 1.08f));
    CuboGlaciar({ 0.0f, 1.0f, 0.0f }, { 0.5f, 0.4f, 0.34f }, COLOR_PIEDRA_GLACIAR);
    CuboGlaciar({ 0.0f, 1.3f, 0.0f }, { 1.5f, 0.16f, 0.32f }, TonoGlaciar(COLOR_PIEDRA_GLACIAR, 0.95f));
    CuboGlaciar({ 0.0f, 1.56f, 0.0f }, { 0.38f, 0.34f, 0.3f }, TonoGlaciar(COLOR_PIEDRA_GLACIAR, 1.05f));
    CuboGlaciar({ 0.0f, 1.39f, 0.0f }, { 1.46f, 0.04f, 0.3f }, COLOR_NIEVE_CLARA_GLACIAR);
    CuboGlaciar({ 0.0f, 1.75f, 0.0f }, { 0.36f, 0.05f, 0.28f }, COLOR_NIEVE_CLARA_GLACIAR);
    rlPopMatrix();
    nieblaFijaGlaciar = -1.0f;
}


static Vector3 BoyaPescaGlaciar(Vector3 hoyo, float t, bool efectos)
{
    float y = efectos ? std::sin(t * 2.2f) * 0.03f + (std::sin(t * 0.6f) > 0.92f ? -0.06f : 0.0f) : 0.0f;
    return { hoyo.x + 0.1f, Y_NIEVE_GLACIAR + 0.03f + y, hoyo.z + 0.1f };
}


// Pozo de pesca: agujero en el hielo, cana en su horquilla y balde.
static void PozoPescaGlaciar(Vector3 hoyo, float t, bool efectos)
{
    AnilloGlaciar({ hoyo.x, Y_NIEVE_GLACIAR + 0.01f, hoyo.z }, 0.5f, 0.72f, TinteGlaciar(COLOR_NIEVE_CLARA_GLACIAR, hoyo), 16);
    SectorGlaciar({ hoyo.x, Y_NIEVE_GLACIAR + 0.012f, hoyo.z }, 0.5f, COLOR_AGUA_CLARA_GLACIAR, 16);

    Vector3 horquilla = { hoyo.x - 1.0f, Y_NIEVE_GLACIAR, hoyo.z + 0.5f };
    PrismaGlaciar(horquilla, { horquilla.x, horquilla.y + 0.55f, horquilla.z }, 0.04f, 0.035f, 5, COLOR_MADERA_OSCURA_GLACIAR);
    Vector3 mango = { hoyo.x - 1.5f, Y_NIEVE_GLACIAR + 0.1f, hoyo.z + 0.75f };
    Vector3 punta = { hoyo.x + 0.3f, Y_NIEVE_GLACIAR + 1.2f, hoyo.z };
    PrismaGlaciar(mango, punta, 0.035f, 0.015f, 5, COLOR_MADERA_GLACIAR);
    Vector3 boya = BoyaPescaGlaciar(hoyo, t, efectos);
    PrismaGlaciar(punta, boya, 0.006f, 0.006f, 3, Color{ 210, 220, 230, 255 });
    CuboGlaciar(boya, { 0.08f, 0.08f, 0.08f }, Color{ 230, 90, 60, 255 });

    // Balde con un pescado.
    Vector3 balde = { hoyo.x + 0.9f, Y_NIEVE_GLACIAR, hoyo.z + 0.6f };
    PrismaGlaciar(balde, { balde.x, balde.y + 0.4f, balde.z }, 0.22f, 0.27f, 10, COLOR_HIERRO_GLACIAR);
    CuboGiradoGlaciar({ balde.x, balde.y + 0.48f, balde.z }, { 0.08f, 0.22f, 0.04f }, 30.0f, Color{ 150, 176, 196, 255 });
}


static void VentisqueroGlaciar(Vector3 p, float escala, int semilla)
{
    RocaGlaciar({ p.x, p.y + 0.05f, p.z }, { 1.2f * escala, 0.38f * escala, 0.8f * escala }, semilla, COLOR_NIEVE_CLARA_GLACIAR, 0.14f);
}


static void RocaNevadaGlaciar(Vector3 p, float escala, int semilla)
{
    RocaGlaciar({ p.x, p.y + 0.3f * escala, p.z }, { 0.8f * escala, 0.6f * escala, 0.7f * escala }, semilla, COLOR_PIEDRA_GLACIAR, 0.22f);
    RocaGlaciar({ p.x + 0.1f * escala, p.y + 0.72f * escala, p.z }, { 0.6f * escala, 0.2f * escala, 0.5f * escala }, semilla + 1, COLOR_NIEVE_CLARA_GLACIAR, 0.14f);
}


static const Vector3 CRISTALES_GLACIAR[2] = { { -12.0f, -0.35f, -1.2f }, { 12.2f, -0.35f, 1.4f } };
static const float ESCALA_CRISTALES_GLACIAR[2] = { 1.0f, 0.85f };
static const Vector3 HOYO_PESCA_GLACIAR = { -11.4f, -0.35f, 3.0f };


static void DibujarUtileriaGlaciar(float t, int paso, bool efectos)
{
    // Pinguinos: un grupo entre el altar y el borde, otro junto al trineo.
    const Vector4 pinguinos[] =
    {
        { -6.6f, -6.6f, 20.0f, 1.0f }, { -7.6f, -7.1f, 50.0f, 0.85f }, { -5.7f, -7.3f, -15.0f, 0.75f },
        { 6.7f, -6.9f, -30.0f, 0.95f }, { 7.7f, -7.4f, 10.0f, 0.7f },
        { -11.8f, -6.0f, 30.0f, 0.9f }
    };
    int cantidadPinguinos = (int)(sizeof(pinguinos) / sizeof(pinguinos[0]));

    for (int k = 0; k < cantidadPinguinos; k += (k < 3 ? 1 : paso))
    {
        PinguinoGlaciar({ pinguinos[k].x, Y_NIEVE_GLACIAR, pinguinos[k].y }, pinguinos[k].z, pinguinos[k].w, t, k + 1, efectos);
    }

    // Pinguinos sobre el iceberg grande.
    const IcebergGlaciar& grande = ICEBERGS_GLACIAR[0];
    float cima = Y_MAR_GLACIAR + grande.alto + 0.3f + VaivenGlaciar(t, 0, efectos);
    PinguinoGlaciar({ grande.x - 1.2f, cima, grande.z + 1.6f }, -20.0f, 1.0f, t, 11, efectos);
    if (paso == 1)
    {
        PinguinoGlaciar({ grande.x - 2.4f, cima, grande.z + 1.0f }, 15.0f, 0.85f, t, 12, efectos);
        PinguinoGlaciar({ grande.x - 0.2f, cima, grande.z + 2.2f }, -40.0f, 0.8f, t, 13, efectos);
    }

    // Campamento de la derecha: trineo, barril y bandera.
    TrineoGlaciar({ 10.8f, Y_NIEVE_GLACIAR, -5.0f }, 28.0f);
    BarrilGlaciar({ 9.3f, Y_NIEVE_GLACIAR, -6.4f });
    BanderaGlaciar({ 12.5f, Y_NIEVE_GLACIAR, -4.6f }, t, efectos);
    RocaNevadaGlaciar({ 12.0f, Y_NIEVE_GLACIAR, -1.6f }, 0.9f, 611);

    // Lado izquierdo: cristales, inukshuk, pozo de pesca y rocas.
    for (int k = 0; k < 2; k++)
    {
        CristalesGlaciar(CRISTALES_GLACIAR[k], ESCALA_CRISTALES_GLACIAR[k], 620 + k * 10);
    }

    InukshukGlaciar({ -9.6f, Y_NIEVE_GLACIAR, -6.4f }, 18.0f);
    PozoPescaGlaciar(HOYO_PESCA_GLACIAR, t, efectos);
    RocaNevadaGlaciar({ -12.6f, Y_NIEVE_GLACIAR, 1.3f }, 0.8f, 641);

    if (paso == 1)
    {
        RocaNevadaGlaciar({ -13.2f, Y_NIEVE_GLACIAR, -3.6f }, 1.1f, 651);
        BarrilGlaciar({ 11.5f, Y_NIEVE_GLACIAR, -6.6f });
    }

    // Ventisqueros a lo largo de los bordes.
    const Vector4 ventisqueros[] =
    {
        { -13.0f, 5.8f, 1.1f, 0.0f }, { 13.4f, 5.4f, 1.0f, 0.0f }, { -12.8f, -2.4f, 0.9f, 0.0f },
        { 13.0f, -3.6f, 1.0f, 0.0f }, { -3.6f, -6.9f, 0.9f, 0.0f }, { 3.4f, -7.2f, 0.8f, 0.0f },
        { 0.2f, -7.1f, 0.7f, 0.0f }, { -17.0f, -7.8f, 1.2f, 0.0f }
    };
    int cantidadVentisqueros = (int)(sizeof(ventisqueros) / sizeof(ventisqueros[0]));

    for (int k = 0; k < cantidadVentisqueros; k += paso)
    {
        VentisqueroGlaciar({ ventisqueros[k].x, Y_NIEVE_GLACIAR, ventisqueros[k].y }, ventisqueros[k].z, 660 + k);
    }
}


//--------------------------------------------------
// Altar de hielo: la grilla jugable
//--------------------------------------------------

static void DibujarTableroGlaciar(const MinijuegoParejasGlaciar& m, const Participante participantes[])
{
    const float a = MEDIO_TABLERO_GLACIAR;
    const float b = BASE_TABLERO_GLACIAR;
    const float y0 = Y_NIEVE_GLACIAR;

    rlBegin(RL_QUADS);

    // Tapa en losas de hielo con variacion por losa.
    const int LOSAS = 6;
    float lado = 2.0f * a / (float)LOSAS;

    for (int i = 0; i < LOSAS; i++)
    {
        for (int j = 0; j < LOSAS; j++)
        {
            float xa = -a + lado * (float)i, xb = xa + lado;
            float za = -a + lado * (float)j, zb = za + lado;
            Color c = TonoGlaciar(COLOR_TABLERO_GLACIAR, 0.94f + 0.1f * RuidoGlaciar(i * 7 + j, 971));
            QuadGlaciar({ xa, 0.0f, zb }, { xb, 0.0f, zb }, { xb, 0.0f, za }, { xa, 0.0f, za }, c);
        }
    }

    // Costados en talud (frente, izquierda, derecha y fondo).
    Color claro = MezclaGlaciar(COLOR_TABLERO_GLACIAR, COLOR_HIELO_GLACIAR, 0.4f);
    QuadGlaciar({ -a, 0.0f, a }, { a, 0.0f, a }, { b, y0, b }, { -b, y0, b }, TonoGlaciar(claro, 0.78f));
    QuadGlaciar({ -a, 0.0f, -a }, { -a, 0.0f, a }, { -b, y0, b }, { -b, y0, -b }, TonoGlaciar(claro, 0.70f));
    QuadGlaciar({ a, 0.0f, a }, { a, 0.0f, -a }, { b, y0, -b }, { b, y0, b }, TonoGlaciar(claro, 0.62f));
    QuadGlaciar({ a, 0.0f, -a }, { -a, 0.0f, -a }, { -b, y0, -b }, { b, y0, -b }, TonoGlaciar(claro, 0.55f));

    // Filo escarchado del borde.
    Color escarcha = MezclaGlaciar(COLOR_TABLERO_GLACIAR, COLOR_HIELO_CLARO_GLACIAR, 0.55f);
    rlEnd();
    MarcoGlaciar(0.0f, 0.008f, 0.0f, a - 0.3f, a, escarcha);
    MarcoGlaciar(0.0f, 0.009f, 0.0f, a - 0.42f, a - 0.36f, MezclaGlaciar(COLOR_TABLERO_GLACIAR, COLOR_HIELO_CLARO_GLACIAR, 0.3f));

    // Huecos tallados de cada bloque; los de una pareja toman el color de
    // quien la hizo.
    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        float x = PosicionXBloqueGlaciar(i);
        float z = PosicionZBloqueGlaciar(i);
        const BloqueGlaciar& bloque = m.bloques[i];
        Color hueco = COLOR_HUECO_GLACIAR;

        if (bloque.emparejado && bloque.duenio >= 0)
        {
            hueco = MezclaGlaciar(COLOR_HUECO_GLACIAR, participantes[bloque.duenio].color, 0.5f);
        }

        rlBegin(RL_QUADS);
        QuadGlaciar({ x - 0.88f, 0.012f, z + 0.88f }, { x + 0.88f, 0.012f, z + 0.88f }, { x + 0.88f, 0.012f, z - 0.88f }, { x - 0.88f, 0.012f, z - 0.88f }, hueco);
        rlEnd();
        MarcoGlaciar(x, 0.014f, z, 0.88f, 0.93f, escarcha);
    }

    // Tachas de hielo claro entre los huecos.
    for (int fx = 0; fx < 5; fx++)
    {
        for (int fz = 0; fz < 5; fz++)
        {
            float x = ((float)fx - 2.0f) * SEPARACION_BLOQUES_GLACIAR;
            float z = ((float)fz - 2.0f) * SEPARACION_BLOQUES_GLACIAR;
            CuboCrudoGlaciar({ x, 0.03f, z }, { 0.14f, 0.06f, 0.14f }, COLOR_HIELO_CLARO_GLACIAR);
        }
    }

    // Pilares de las esquinas con un cristal en la punta.
    for (int e = 0; e < 4; e++)
    {
        float ex = (e & 1) ? a - 0.22f : -(a - 0.22f);
        float ez = (e & 2) ? a - 0.22f : -(a - 0.22f);
        PrismaGlaciar({ ex, 0.0f, ez }, { ex, 0.55f, ez }, 0.2f, 0.17f, 6, COLOR_HIELO_GLACIAR);
        PrismaGlaciar({ ex, 0.55f, ez }, { ex, 0.62f, ez }, 0.22f, 0.22f, 6, COLOR_HIELO_CLARO_GLACIAR);
        PrismaGlaciar({ ex, 0.62f, ez }, { ex, 1.05f, ez }, 0.11f, 0.0f, 6, COLOR_AURORA_VERDE_GLACIAR, 0.7f, 0.3f);
    }
}


// Grietas que corren por el altar mientras el glaciar cruje.
static void DibujarGrietasCrujidoGlaciar(const MinijuegoParejasGlaciar& m, Color color, float alfa)
{
    if (m.tiempoCrujidoVisual > 2.0f || alfa <= 0.0f)
    {
        return;
    }

    float avance = Limitar01Glaciar(m.tiempoCrujidoVisual / 0.35f);
    int origenes[2] = { m.crujidoA, m.crujidoB };

    rlBegin(RL_QUADS);

    for (int o = 0; o < 2; o++)
    {
        if (origenes[o] < 0) continue;

        for (int g = 0; g < 3; g++)
        {
            float ang = 6.2832f * ((float)g / 3.0f + RuidoGlaciar(origenes[o] * 3 + g, 981));
            Vector3 p = CentroBloqueGlaciar(origenes[o], 0.02f);

            for (int s = 0; s < 4; s++)
            {
                float largo = (0.45f + 0.35f * RuidoGlaciar(g * 9 + s + o * 40, 982)) * avance;
                ang += (RuidoGlaciar(g * 9 + s + o * 40, 983) - 0.5f) * 1.1f;
                Vector3 q = { p.x + std::cos(ang) * largo, 0.02f, p.z + std::sin(ang) * largo };
                if (std::fabs(q.x) > MEDIO_TABLERO_GLACIAR - 0.1f || std::fabs(q.z) > MEDIO_TABLERO_GLACIAR - 0.1f) break;
                float ancho = 0.045f;
                Vector3 l = { -std::sin(ang) * ancho, 0.0f, std::cos(ang) * ancho };
                QuadGlaciar(SumaGlaciar(p, l), SumaGlaciar(q, l), RestaGlaciar(q, l), RestaGlaciar(p, l), AlfaGlaciar(color, alfa));
                p = q;
            }
        }
    }

    rlEnd();
}


//--------------------------------------------------
// Pozos de los tempanos
//--------------------------------------------------

// Cada lado tiene un pozo (circulo con 2 jugadores, capsula con 4) donde
// flotan los tempanos de ese lado.
static bool PozoLadoGlaciar(const MinijuegoParejasGlaciar& m, int lado, float& x, float& z0, float& z1)
{
    bool hay = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;
        if ((m.posicionTempanoX[i] < 0.0f ? -1 : 1) != lado) continue;

        if (!hay)
        {
            x = m.posicionTempanoX[i];
            z0 = z1 = m.posicionTempanoZ[i];
            hay = true;
        }
        else
        {
            if (m.posicionTempanoZ[i] < z0) z0 = m.posicionTempanoZ[i];
            if (m.posicionTempanoZ[i] > z1) z1 = m.posicionTempanoZ[i];
        }
    }

    return hay;
}


static Vector3 BordePozoGlaciar(float x, float z0, float z1, int k, int lados, float radio, float y)
{
    float a = 6.2832f * (float)k / (float)lados;
    float s = std::sin(a);
    float cz = s < 0.0f ? z0 : z1;
    return { x + std::cos(a) * radio, y, cz + s * radio };
}


static void DibujarPozoGlaciar(float x, float z0, float z1, float t, bool efectos)
{
    const int LADOS = 40;
    float zc = (z0 + z1) * 0.5f;

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        Vector3 p0 = BordePozoGlaciar(x, z0, z1, k, LADOS, RADIO_POZO_GLACIAR, Y_POZO_GLACIAR);
        Vector3 p1 = BordePozoGlaciar(x, z0, z1, k + 1, LADOS, RADIO_POZO_GLACIAR, Y_POZO_GLACIAR);
        VerticeGlaciar({ x, Y_POZO_GLACIAR, zc }, ColorMarGlaciar(x, zc, t, efectos));
        VerticeGlaciar(p1, ColorMarGlaciar(p1.x, p1.z, t, efectos));
        VerticeGlaciar(p0, ColorMarGlaciar(p0.x, p0.z, t, efectos));
    }
    rlEnd();

    // Pared interior del lado de atras (la que mira a la camara) y labio
    // de nieve alrededor.
    rlBegin(RL_QUADS);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float h0 = Limitar01Glaciar(-std::sin(a0)) * 0.32f;
        float h1 = Limitar01Glaciar(-std::sin(a1)) * 0.32f;
        Vector3 e0 = BordePozoGlaciar(x, z0, z1, k, LADOS, RADIO_POZO_GLACIAR, Y_POZO_GLACIAR + 0.003f);
        Vector3 e1 = BordePozoGlaciar(x, z0, z1, k + 1, LADOS, RADIO_POZO_GLACIAR, Y_POZO_GLACIAR + 0.003f);
        Vector3 i0 = BordePozoGlaciar(x, z0, z1, k, LADOS, RADIO_POZO_GLACIAR - h0, Y_POZO_GLACIAR + 0.003f);
        Vector3 i1 = BordePozoGlaciar(x, z0, z1, k + 1, LADOS, RADIO_POZO_GLACIAR - h1, Y_POZO_GLACIAR + 0.003f);
        QuadGlaciar(e0, e1, i1, i0, COLOR_HIELO_GLACIAR);

        Vector3 l0 = BordePozoGlaciar(x, z0, z1, k, LADOS, RADIO_POZO_GLACIAR + 0.34f, Y_POZO_GLACIAR);
        Vector3 l1 = BordePozoGlaciar(x, z0, z1, k + 1, LADOS, RADIO_POZO_GLACIAR + 0.34f, Y_POZO_GLACIAR);
        Vector3 b0 = BordePozoGlaciar(x, z0, z1, k, LADOS, RADIO_POZO_GLACIAR, Y_POZO_GLACIAR + 0.004f);
        Vector3 b1 = BordePozoGlaciar(x, z0, z1, k + 1, LADOS, RADIO_POZO_GLACIAR, Y_POZO_GLACIAR + 0.004f);
        QuadGlaciar(l0, l1, b1, b0, TonoGlaciar(COLOR_NIEVE_CLARA_GLACIAR, 0.95f + 0.06f * RuidoGlaciar(k, 991)));
    }
    rlEnd();
}


//--------------------------------------------------
// Piezas jugables: bloques, simbolos, cursor y tempanos (GLB)
//--------------------------------------------------

static Color ColorSimboloGlaciar(int simbolo)
{
    const Color colores[8] =
    {
        Color{ 236, 84, 84, 255 },
        Color{ 80, 124, 246, 255 },
        Color{ 248, 212, 66, 255 },
        Color{ 80, 210, 110, 255 },
        Color{ 180, 100, 230, 255 },
        Color{ 244, 156, 60, 255 },
        Color{ 70, 216, 210, 255 },
        Color{ 120, 255, 170, 255 }
    };

    return colores[simbolo & 7];
}


static Color ColorAuroraGlaciar(float tiempo)
{
    float mezcla = 0.5f + 0.5f * std::sin(tiempo * 3.0f);
    return {
        (unsigned char)(120.0f + 120.0f * mezcla),
        (unsigned char)(255.0f - 120.0f * mezcla),
        (unsigned char)(170.0f + 40.0f * mezcla), 255
    };
}


static void DibujarSimboloGlaciar(int simbolo, Vector3 p, float tiempo)
{
    Color color = ColorSimboloGlaciar(simbolo);
    Color brillo = ColorAuroraGlaciar(tiempo);

    if (DibujarModeloParejasGlaciarRetro3D(
        (ModeloParejasGlaciar3D)(MODELO_GLACIAR_SIMBOLO_0 + (simbolo & 7)),
        p, {1,1,1}, brillo))
    {
        return;
    }

    // Respaldo con primitivas si falta el GLB del simbolo.
    switch (simbolo)
    {
    case 0:
        (DrawSphere)({ p.x, p.y + 0.35f, p.z }, 0.35f, color);
        break;
    case 1:
        (DrawCube)({ p.x, p.y + 0.3f, p.z }, 0.6f, 0.6f, 0.6f, color);
        break;
    case 2:
        (DrawCylinder)(p, 0.0f, 0.38f, 0.8f, 10, color);
        break;
    case 3:
        (DrawCylinder)(p, 0.28f, 0.28f, 0.7f, 10, color);
        break;
    case 4:
        (DrawCylinder)(p, 0.0f, 0.34f, 0.4f, 4, color);
        (DrawCylinder)({ p.x, p.y + 0.4f, p.z }, 0.34f, 0.0f, 0.4f, 4, color);
        break;
    case 5:
        (DrawSphere)({ p.x, p.y + 0.3f, p.z }, 0.3f, color);
        (DrawSphere)({ p.x, p.y + 0.78f, p.z }, 0.2f, color);
        break;
    case 6:
        for (int k = 0; k < 6; k++)
        {
            float angulo = (float)k * 1.0472f;
            (DrawSphere)(
                { p.x + std::cos(angulo) * 0.33f, p.y + 0.15f, p.z + std::sin(angulo) * 0.33f },
                0.11f,
                color
            );
        }
        break;
    default:
        (DrawCube)({ p.x, p.y + 0.4f, p.z }, 0.8f, 0.14f, 0.14f, brillo);
        (DrawCube)({ p.x, p.y + 0.4f, p.z }, 0.14f, 0.8f, 0.14f, brillo);
        (DrawCube)({ p.x, p.y + 0.4f, p.z }, 0.14f, 0.14f, 0.8f, brillo);
        break;
    }
}


// Aparicion en ola: durante la cuenta los bloques suben desde el hielo.
static float AparicionBloqueGlaciar(const MinijuegoParejasGlaciar& m, int indice)
{
    if (m.fase != FASE_GLACIAR_PREPARACION)
    {
        return 1.0f;
    }

    float transcurrido = DURACION_PREPARACION_GLACIAR - m.tiempoPreparacion;
    float retraso = 0.25f + 0.09f * (float)(indice % COLUMNAS_GLACIAR + (COLUMNAS_GLACIAR - 1 - indice / COLUMNAS_GLACIAR));
    return ReboteGlaciar((transcurrido - retraso) / 0.5f);
}


static Vector3 PosicionVisualBloqueGlaciar(const MinijuegoParejasGlaciar& m, int indice, float tiempo)
{
    const BloqueGlaciar& bloque = m.bloques[indice];
    float x = PosicionXBloqueGlaciar(indice);
    float z = PosicionZBloqueGlaciar(indice);

    if (bloque.temblor > 0.0f)
    {
        x += std::sin(tiempo * 60.0f) * 0.08f;
        z += std::cos(tiempo * 55.0f) * 0.08f;
    }

    float sacudida = m.sacudidaBloque[indice];

    if (sacudida > 0.0f)
    {
        x += std::sin(tiempo * 70.0f) * 0.07f * sacudida;
    }

    float subida = AparicionBloqueGlaciar(m, indice);
    float y = -(ALTURA_BLOQUE_GLACIAR + 0.15f) * (1.0f - subida);

    return { x, y, z };
}


static void DibujarBloqueGlaciar(const MinijuegoParejasGlaciar& m, int indice, float tiempo, Color colorDuenio)
{
    const BloqueGlaciar& bloque = m.bloques[indice];
    Vector3 p = PosicionVisualBloqueGlaciar(m, indice, tiempo);
    float x = p.x, z = p.z;
    float altura = ALTURA_BLOQUE_GLACIAR * (1.0f - 0.8f * bloque.derretido);

    bool modelo = DibujarModeloParejasGlaciarRetro3D(
        bloque.emparejado ? MODELO_GLACIAR_BLOQUE_EMPAREJADO : MODELO_GLACIAR_BLOQUE_OCULTO,
        p, {1,altura / (bloque.emparejado ? 0.26f : ALTURA_BLOQUE_GLACIAR),1},
        colorDuenio, !bloque.emparejado && bloque.derretido >= 0.5f ? 3 : -1);

    if (!modelo)
    {
        // Respaldo con primitivas si falta el GLB del bloque.
        Color hielo = bloque.emparejado ? Color{ 150, 190, 215, 255 } : Color{ 170, 218, 246, 255 };
        (DrawCube)({ x, p.y + altura * 0.5f, z }, 1.65f, altura, 1.65f, hielo);
        if (bloque.emparejado && bloque.duenio >= 0)
            (DrawCylinder)({ x, p.y, z }, 0.75f, 0.75f, 0.05f, 14, colorDuenio);
    }

    if (bloque.derretido >= 0.5f)
    {
        DibujarSimboloGlaciar(bloque.simbolo, { x, p.y + altura + 0.05f, z }, tiempo);
    }

    if (bloque.temblor > 0.0f)
    {
        // Fragmentos visuales ligados al mismo temblor, sin estado fisico.
        for (int k = 0; k < 3; k++)
        {
            float a = tiempo * 8.0f + k * 2.0944f;
            Vector3 f = { x + std::cos(a) * 0.9f,
                altura + 0.1f + 0.18f * (1.0f + std::sin(a * 2.0f)),
                z + std::sin(a) * 0.9f };
            if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_FRAGMENTO,
                f, {0.22f,0.22f,0.22f}, WHITE, -1, 0.75f, a * RAD2DEG))
                (DrawCube)(f, 0.13f, 0.15f, 0.13f, Fade(SKYBLUE, 0.75f));
        }
    }
}


static void DibujarTempanoGlaciar(float x, float z)
{
    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_TEMPANO, {x,0,z}))
    {
        (DrawCylinder)({ x, -0.45f, z }, 2.1f, 2.4f, 0.5f, 16, Color{ 205, 232, 248, 255 });
    }
}


static void DibujarCursorGlaciar(const MinijuegoParejasGlaciar& m)
{
    if (m.fase != FASE_GLACIAR_ELEGIR || m.turno < 0)
    {
        return;
    }

    float x = PosicionXBloqueGlaciar(m.cursor);
    float z = PosicionZBloqueGlaciar(m.cursor);

    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_CURSOR, {x,0,z}))
    {
        DrawCubeWires({ x, 0.55f, z }, 1.85f, 1.1f, 1.85f, YELLOW);
    }
}


// Flecha del color de quien juega, flotando sobre el bloque del cursor.
static void DibujarFlechaCursorGlaciar(const MinijuegoParejasGlaciar& m, const Participante participantes[], float t)
{
    if (m.fase != FASE_GLACIAR_ELEGIR || m.turno < 0)
    {
        return;
    }

    float x = PosicionXBloqueGlaciar(m.cursor);
    float z = PosicionZBloqueGlaciar(m.cursor);
    float altura = ALTURA_BLOQUE_GLACIAR * (1.0f - 0.8f * m.bloques[m.cursor].derretido);
    float y = altura + 0.75f + 0.1f * std::sin(t * 5.0f);
    Color color = participantes[m.turno].color;

    PrismaGlaciar({ x, y + 0.5f, z }, { x, y + 0.56f, z }, 0.3f, 0.3f, 4, MezclaGlaciar(color, WHITE, 0.2f), 0.4f, 0.785f);
    PrismaGlaciar({ x, y + 0.5f, z }, { x, y, z }, 0.3f, 0.0f, 4, color, 0.4f, 0.785f);
}


//--------------------------------------------------
// Jugadores
//--------------------------------------------------

static float SaltoJugadorGlaciar(const MinijuegoParejasGlaciar& m, int i)
{
    float d = m.destelloJugador[i];
    return d > 0.0f ? std::sin((1.0f - d) * 3.1416f) * 0.55f : 0.0f;
}


static Vector3 PosicionJugadorVisualGlaciar(const MinijuegoParejasGlaciar& m, const JugadorPrueba& jugador, int i)
{
    return {
        m.posicionTempanoX[i],
        0.05f + jugador.tamano.y * 0.5f + SaltoJugadorGlaciar(m, i),
        m.posicionTempanoZ[i]
    };
}


static void DibujarJugadoresGlaciar(
    const MinijuegoParejasGlaciar& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    bool mostrarDebug
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        JugadorPrueba visual = jugadores[i];
        visual.posicion = PosicionJugadorVisualGlaciar(m, jugadores[i], i);

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }
    }
}


//--------------------------------------------------
// Nieve que cae y particulas
//--------------------------------------------------

static int CantidadCoposGlaciar()
{
    return 50 + (int)(230.0f * FactorCalidadGrafica(CalidadParticulas()));
}


static Vector3 PosicionCopoGlaciar(int k, float t)
{
    const float ALTO = 13.0f;
    float velocidad = 0.7f + 0.6f * RuidoGlaciar(k, 1001);
    float x = (RuidoGlaciar(k, 1002) - 0.5f) * 40.0f;
    float z = -20.0f + 23.0f * RuidoGlaciar(k, 1003);
    float y = ALTO - std::fmod(RuidoGlaciar(k, 1004) * ALTO + t * velocidad, ALTO + 0.6f);
    float fase = RuidoGlaciar(k, 1005) * 6.2832f;
    return { x + std::sin(t * 0.8f + fase) * 0.5f, y - 0.4f, z + std::cos(t * 0.6f + fase) * 0.3f };
}


static void DibujarNieveGlaciar(float t)
{
    int cantidad = CantidadCoposGlaciar();

    rlBegin(RL_QUADS);
    for (int k = 0; k < cantidad; k++)
    {
        Vector3 p = PosicionCopoGlaciar(k, t);
        float s = 0.022f + 0.026f * RuidoGlaciar(k, 1006);
        Vector3 u = EscalaGlaciar(FRENTE_U_GLACIAR, s);
        Vector3 v = EscalaGlaciar(FRENTE_V_GLACIAR, s);
        Color c = AlfaGlaciar(Color{ 236, 244, 255, 255 }, 0.55f + 0.4f * RuidoGlaciar(k, 1007));
        QuadGlaciar(SumaGlaciar(RestaGlaciar(p, u), v), SumaGlaciar(SumaGlaciar(p, u), v),
            RestaGlaciar(SumaGlaciar(p, u), v), RestaGlaciar(RestaGlaciar(p, u), v), c);
    }
    rlEnd();
}


static void DibujarParticulasSolidasGlaciar(const MinijuegoParejasGlaciar& m)
{
    for (int i = 0; i < MAX_PARTICULAS_GLACIAR; i++)
    {
        const ParticulaGlaciar& p = m.particulas[i];

        if (!p.activa || p.tipo == PARTICULA_GLACIAR_VAPOR)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        float s = p.tamano;

        switch (p.tipo)
        {
        case PARTICULA_GLACIAR_ESQUIRLA:
            s *= 0.5f + 0.5f * vida;
            rlPushMatrix();
            rlTranslatef(p.posicion.x, p.posicion.y, p.posicion.z);
            rlRotatef(p.giro, 0.4f, 1.0f, 0.3f);
            CuboCrudoGlaciar({ 0.0f, 0.0f, 0.0f }, { s, s * 1.3f, s * 0.7f }, p.color);
            rlPopMatrix();
            break;
        case PARTICULA_GLACIAR_CHISPA:
            s *= 0.4f + 0.6f * vida;
            CuboCrudoGlaciar(p.posicion, { s, s, s }, p.color);
            break;
        case PARTICULA_GLACIAR_CONFETI:
            rlPushMatrix();
            rlTranslatef(p.posicion.x, p.posicion.y, p.posicion.z);
            rlRotatef(p.giro, 0.3f, 1.0f, 0.4f);
            CuboCrudoGlaciar({ 0.0f, 0.0f, 0.0f }, { s, s * 0.6f, 0.02f }, p.color);
            rlPopMatrix();
            break;
        default:
            break;
        }
    }
}


//--------------------------------------------------
// Translucidos: ondas, bruma, vapor, reflejos sumergidos
//--------------------------------------------------

// Marcas de lectura (en todas las calidades): duenio de cada pareja,
// cursor y primer bloque del turno, y anillo en el tempano de quien juega.
static void DibujarMarcasTurnoGlaciar(const MinijuegoParejasGlaciar& m, const Participante participantes[], float t)
{
    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        const BloqueGlaciar& bloque = m.bloques[i];

        if (bloque.emparejado && bloque.duenio >= 0)
        {
            MarcoGlaciar(PosicionXBloqueGlaciar(i), 0.02f, PosicionZBloqueGlaciar(i), 0.88f, 0.98f, AlfaGlaciar(participantes[bloque.duenio].color, 0.85f));
        }
    }

    bool hayTurno = m.turno >= 0 && (m.fase == FASE_GLACIAR_ELEGIR || m.fase == FASE_GLACIAR_REVELANDO);

    if (!hayTurno)
    {
        return;
    }

    Color color = participantes[m.turno].color;
    float pulso = 0.8f + 0.2f * std::sin(t * 7.0f);

    if (m.fase == FASE_GLACIAR_ELEGIR)
    {
        float altura = ALTURA_BLOQUE_GLACIAR * (1.0f - 0.8f * m.bloques[m.cursor].derretido);
        MarcoGlaciar(PosicionXBloqueGlaciar(m.cursor), 0.03f, PosicionZBloqueGlaciar(m.cursor), 0.93f, 1.1f, AlfaGlaciar(color, 0.95f * pulso));
        MarcoGlaciar(PosicionXBloqueGlaciar(m.cursor), altura + 0.03f, PosicionZBloqueGlaciar(m.cursor), 0.86f, 0.98f, AlfaGlaciar(color, 0.9f * pulso));

        if (m.primera >= 0)
        {
            MarcoGlaciar(PosicionXBloqueGlaciar(m.primera), 0.03f, PosicionZBloqueGlaciar(m.primera), 0.93f, 1.04f, AlfaGlaciar(color, 0.7f));
        }
    }

    AnilloGlaciar({ m.posicionTempanoX[m.turno], 0.14f, m.posicionTempanoZ[m.turno] }, 1.6f, 1.84f, AlfaGlaciar(color, 0.8f * pulso), 40);
}


static void DibujarTranslucidosGlaciar(const MinijuegoParejasGlaciar& m, const Participante participantes[], float t, bool efectos, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    DibujarMarcasTurnoGlaciar(m, participantes, t);

    // Ondas alrededor de cada tempano.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        Vector3 c = { m.posicionTempanoX[i], Y_POZO_GLACIAR + 0.01f, m.posicionTempanoZ[i] };
        SectorGlaciar({ c.x, 0.135f, c.z }, 0.5f, Color{ 12, 20, 40, 90 }, 14);

        if (efectos)
        {
            for (int r = 0; r < 2; r++)
            {
                float u = std::fmod(t * 0.35f + (float)r * 0.5f + (float)i * 0.23f, 1.0f);
                float radio = 2.15f + 0.4f * u;
                AnilloGlaciar(c, radio, radio + 0.05f, AlfaGlaciar(Color{ 170, 210, 240, 255 }, 0.3f * (1.0f - u)), 40);
            }
        }
    }

    // Hielo sumergido alrededor de la banquisa (falda celeste bajo el agua).
    rlBegin(RL_QUADS);
    for (int i = 0; i < CANTIDAD_CONTORNO_GLACIAR - 1; i++)
    {
        Vector3 a = PuntoBanquisaGlaciar(i, 1.0f, Y_MAR_GLACIAR + 0.02f);
        Vector3 b = PuntoBanquisaGlaciar(i + 1, 1.0f, Y_MAR_GLACIAR + 0.02f);
        Vector3 a1 = PuntoBanquisaGlaciar(i, 1.1f, Y_MAR_GLACIAR + 0.02f);
        Vector3 b1 = PuntoBanquisaGlaciar(i + 1, 1.1f, Y_MAR_GLACIAR + 0.02f);
        VerticeGlaciar(a, Color{ 90, 170, 220, 110 });
        VerticeGlaciar(b, Color{ 90, 170, 220, 110 });
        VerticeGlaciar(b1, Color{ 90, 170, 220, 0 });
        VerticeGlaciar(a1, Color{ 90, 170, 220, 0 });
    }
    rlEnd();

    // Espuma al pie del glaciar y de los icebergs.
    rlBegin(RL_QUADS);
    QuadGlaciar({ -140.0f, Y_MAR_GLACIAR + 0.03f, -36.6f }, { 140.0f, Y_MAR_GLACIAR + 0.03f, -36.6f },
        { 140.0f, Y_MAR_GLACIAR + 0.03f, -38.4f }, { -140.0f, Y_MAR_GLACIAR + 0.03f, -38.4f },
        TinteGlaciar(AlfaGlaciar(Color{ 200, 226, 250, 255 }, 0.35f), { 0.0f, 0.0f, -37.0f }));
    rlEnd();

    for (int k = 0; k < CANTIDAD_ICEBERGS_GLACIAR; k++)
    {
        const IcebergGlaciar& b = ICEBERGS_GLACIAR[k];
        AnilloGlaciar({ b.x, Y_MAR_GLACIAR + 0.03f, b.z }, b.radio * 0.95f, b.radio * 1.35f, AlfaGlaciar(Color{ 120, 190, 230, 255 }, 0.22f), 20);
    }

    // Bruma baja que deriva sobre el mar.
    int bancos = decoracion ? 12 : 5;
    for (int k = 0; k < bancos; k++)
    {
        float deriva = efectos ? t * (0.3f + 0.3f * RuidoGlaciar(k, 1011)) : 0.0f;
        float x = std::fmod(RuidoGlaciar(k, 1012) * 110.0f + deriva, 110.0f) - 55.0f;
        float z = -36.0f + 20.0f * RuidoGlaciar(k, 1013);
        ResplandorSueloGlaciar({ x, Y_MAR_GLACIAR + 0.35f, z }, 5.0f + 5.0f * RuidoGlaciar(k, 1014), AlfaGlaciar(Color{ 140, 170, 210, 255 }, 0.12f), 2.2f);
    }

    // Vapor del deshielo.
    for (int i = 0; i < MAX_PARTICULAS_GLACIAR; i++)
    {
        const ParticulaGlaciar& p = m.particulas[i];
        if (!p.activa || p.tipo != PARTICULA_GLACIAR_VAPOR) continue;

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteGlaciar(p.posicion, p.tamano, AlfaGlaciar(p.color, 0.45f * vida));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Luces aditivas
//--------------------------------------------------

static void DibujarLucesGlaciar(
    const MinijuegoParejasGlaciar& m,
    const Participante participantes[],
    float t,
    bool decoracion
)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    float aurora = m.brilloAurora;

    // Reflejo de la aurora sobre el mar: franjas verdes y violetas.
    for (int k = 0; k < 22; k++)
    {
        float x = -60.0f + 120.0f * RuidoGlaciar(k, 1021) + std::sin(t * 0.2f + (float)k) * 1.5f;
        float z = -34.0f + 18.0f * RuidoGlaciar(k, 1022);
        float brillo = 0.55f + 0.45f * std::sin(t * 1.4f + (float)k * 2.3f);
        Color c = k % 3 == 0 ? COLOR_AURORA_VIOLETA_GLACIAR : (k % 3 == 1 ? COLOR_AURORA_CIAN_GLACIAR : COLOR_AURORA_VERDE_GLACIAR);
        ReflejoAguaGlaciar({ x, Y_MAR_GLACIAR + 0.05f, z }, 0.7f + 0.8f * RuidoGlaciar(k, 1023), AlfaGlaciar(c, (0.14f + 0.18f * aurora) * brillo), 6.0f);
    }

    // La aurora tine la nieve: manchas verdes enormes y muy tenues.
    for (int k = 0; k < 5; k++)
    {
        float x = -12.0f + 6.0f * (float)k + std::sin(t * 0.15f + (float)k) * 2.0f;
        float z = -5.0f + 3.0f * RuidoGlaciar(k, 1024);
        Color c = k % 2 ? COLOR_AURORA_CIAN_GLACIAR : COLOR_AURORA_VERDE_GLACIAR;
        ResplandorSueloGlaciar({ x, Y_NIEVE_GLACIAR + 0.02f, z }, 7.0f, AlfaGlaciar(c, 0.035f + 0.11f * aurora), 1.4f);
    }

    // Faroles de los postes: halo, cono y charco tibio.
    for (int k = 0; k < 2; k++)
    {
        Vector3 l = FarolGlaciar(k, t);
        float parpadeo = 0.9f + 0.06f * std::sin(t * 11.0f + (float)k * 2.0f) + 0.04f * std::sin(t * 29.0f + (float)k);
        HaloGlaciar(l, 1.5f * parpadeo, Color{ 255, 176, 90, 80 }, Color{ 255, 232, 170, 170 });
        ConoLuzGlaciar(l, { l.x, Y_NIEVE_GLACIAR + 0.05f, l.z }, 2.0f, Color{ 255, 190, 110, 22 });
        CharcoLuzGlaciar({ l.x, Y_NIEVE_GLACIAR + 0.03f, l.z }, 3.6f * parpadeo, COLOR_LUZ_GLACIAR, 1.1f);
        CharcoLuzGlaciar({ l.x * 0.82f, 0.02f, l.z + 1.2f }, 2.2f * parpadeo, COLOR_LUZ_GLACIAR, 0.7f);
    }

    // Cueva de hielo: resplandor turquesa que sale de adentro.
    {
        Vector3 c = CUEVA_GLACIAR;
        float pulso = 0.85f + 0.15f * std::sin(t * 0.9f);
        ResplandorGlaciar({ c.x, c.y + 0.9f, c.z + 0.1f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, RADIO_CUEVA_GLACIAR * 1.2f * pulso, Color{ 60, 200, 255, 110 });
        ResplandorGlaciar({ c.x, c.y + 0.6f, c.z + 0.12f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, RADIO_CUEVA_GLACIAR * 0.6f, Color{ 180, 250, 255, 90 });
        CharcoLuzGlaciar({ c.x, Y_NIEVE_GLACIAR + 0.03f, c.z + 2.2f }, 3.6f * pulso, Color{ 80, 210, 255, 255 }, 1.0f);
    }

    // Cristales de hielo y pilares del altar: brillo frio.
    if (decoracion)
    {
        for (int k = 0; k < 2; k++)
        {
            Vector3 p = PuntaCristalGlaciar(CRISTALES_GLACIAR[k], ESCALA_CRISTALES_GLACIAR[k]);
            float pulso = 0.75f + 0.25f * std::sin(t * 1.3f + (float)k * 2.0f);
            HaloGlaciar({ p.x, p.y - 0.8f, p.z }, 1.9f * pulso, AlfaGlaciar(COLOR_CRISTAL_GLACIAR, 0.16f), AlfaGlaciar(WHITE, 0.14f));
            CharcoLuzGlaciar({ p.x, Y_NIEVE_GLACIAR + 0.03f, p.z + 0.6f }, 2.4f * pulso, COLOR_CRISTAL_GLACIAR, 0.8f);
        }
    }

    for (int e = 0; e < 4; e++)
    {
        float a = MEDIO_TABLERO_GLACIAR - 0.22f;
        float ex = (e & 1) ? a : -a;
        float ez = (e & 2) ? a : -a;
        float pulso = 0.8f + 0.2f * std::sin(t * 2.0f + (float)e * 1.6f) + 0.6f * aurora;
        ResplandorFrenteGlaciar({ ex, 0.9f, ez }, 0.55f * pulso, AlfaGlaciar(COLOR_AURORA_VERDE_GLACIAR, 0.3f));
    }

    // Bloques: simbolos revelados, parejas, destellos y el cursor.
    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        const BloqueGlaciar& bloque = m.bloques[i];
        Vector3 p = PosicionVisualBloqueGlaciar(m, i, t);
        float altura = ALTURA_BLOQUE_GLACIAR * (1.0f - 0.8f * bloque.derretido);

        if (bloque.derretido >= 0.5f)
        {
            bool esAurora = bloque.simbolo == SIMBOLO_AURORA_GLACIAR;
            Color c = esAurora ? ColorAuroraGlaciar(t) : ColorSimboloGlaciar(bloque.simbolo);
            float vivo = bloque.emparejado ? 0.6f : 1.0f;
            ResplandorSueloGlaciar({ p.x, p.y + altura + 0.02f, p.z }, 0.95f, AlfaGlaciar(c, 0.45f * vivo));
            ResplandorFrenteGlaciar({ p.x, p.y + altura + 0.4f, p.z }, esAurora ? 1.25f : 0.85f, AlfaGlaciar(c, (esAurora ? 0.4f : 0.22f) * vivo));

            if (esAurora)
            {
                // Rayos que giran alrededor de la aurora.
                float giro = t * 50.0f;
                Vector3 centro = { p.x, p.y + altura + 0.45f, p.z };

                for (int r = 0; r < 4; r++)
                {
                    float a = (giro + (float)r * 45.0f) * DEG2RAD;
                    Vector3 dir = { std::cos(a) * FRENTE_U_GLACIAR.x + std::sin(a) * FRENTE_V_GLACIAR.x, std::sin(a) * FRENTE_V_GLACIAR.y, std::sin(a) * FRENTE_V_GLACIAR.z };
                    Vector3 lado = { -std::sin(a) * 0.05f, std::cos(a) * 0.05f * FRENTE_V_GLACIAR.y, std::cos(a) * 0.05f * FRENTE_V_GLACIAR.z };
                    float largo = 1.1f;
                    rlBegin(RL_QUADS);
                    VerticeGlaciar(SumaGlaciar(centro, EscalaGlaciar(dir, -largo)), AlfaGlaciar(c, 0.0f));
                    VerticeGlaciar(SumaGlaciar(centro, lado), AlfaGlaciar(c, 0.5f));
                    VerticeGlaciar(SumaGlaciar(centro, EscalaGlaciar(dir, largo)), AlfaGlaciar(c, 0.0f));
                    VerticeGlaciar(RestaGlaciar(centro, lado), AlfaGlaciar(c, 0.5f));
                    rlEnd();
                }
            }
        }

        if (m.destelloBloque[i] > 0.0f)
        {
            float d = m.destelloBloque[i];
            HaloGlaciar({ p.x, p.y + altura + 0.3f, p.z }, 1.3f * d + 0.3f, AlfaGlaciar(COLOR_HIELO_CLARO_GLACIAR, 0.28f * d), AlfaGlaciar(WHITE, 0.16f * d));
        }
    }

    // Turno: primer bloque elegido, cursor, foco y anillo del tempano.
    bool hayTurno = m.turno >= 0 && (m.fase == FASE_GLACIAR_ELEGIR || m.fase == FASE_GLACIAR_REVELANDO);

    if (hayTurno)
    {
        Color color = participantes[m.turno].color;
        float pulso = 0.8f + 0.2f * std::sin(t * 7.0f);

        if (m.fase == FASE_GLACIAR_ELEGIR)
        {
            float x = PosicionXBloqueGlaciar(m.cursor);
            float z = PosicionZBloqueGlaciar(m.cursor);
            CharcoLuzGlaciar({ x, 0.04f, z }, 1.6f, color, 1.3f * pulso);
        }

        float x = m.posicionTempanoX[m.turno];
        float z = m.posicionTempanoZ[m.turno];
        ConoLuzGlaciar({ x, 4.6f, z - 0.3f }, { x, 0.15f, z }, 1.9f, AlfaGlaciar(color, 0.05f));
        CharcoLuzGlaciar({ x, 0.15f, z }, 2.4f, color, 0.6f * pulso);
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo || m.destelloJugador[i] <= 0.0f) continue;

        float d = m.destelloJugador[i];
        Vector3 c = { m.posicionTempanoX[i], 1.2f, m.posicionTempanoZ[i] };
        HaloGlaciar(c, 1.9f * d + 0.4f, AlfaGlaciar(participantes[i].color, 0.4f * d), AlfaGlaciar(WHITE, 0.35f * d));
    }

    // Crujido: brillo de las grietas.
    DibujarGrietasCrujidoGlaciar(m, COLOR_AURORA_CIAN_GLACIAR, 0.6f * Limitar01Glaciar(1.0f - (m.tiempoCrujidoVisual - 1.2f) / 0.8f));

    // Chispas con brillo.
    for (int i = 0; i < MAX_PARTICULAS_GLACIAR; i++)
    {
        const ParticulaGlaciar& p = m.particulas[i];
        if (!p.activa || p.tipo != PARTICULA_GLACIAR_CHISPA) continue;

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteGlaciar(p.posicion, p.tamano * 3.2f, AlfaGlaciar(p.color, 0.5f * vida));
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Anuncios sobre el mundo
//--------------------------------------------------

struct AnuncioGlaciar
{
    char texto[48] = "";
    int token = UI_TEXTO;
    Color color = WHITE;
};


static float AlfaAnuncioGlaciar(float u)
{
    if (u < 0.0f || u > 1.0f) return 0.0f;
    if (u < 0.12f) return u / 0.12f;
    if (u > 0.75f) return (1.0f - u) / 0.25f;
    return 1.0f;
}


static void DibujarAnunciosGlaciar(const MinijuegoParejasGlaciar& m, const Participante participantes[])
{
    AnuncioGlaciar lineas[2];
    int cantidad = 0;
    float alfa = 0.0f;
    float u = 0.0f;

    if (m.fase == FASE_GLACIAR_REVELANDO && m.ultimoJugador >= 0)
    {
        int numero = participantes[m.ultimoJugador].numeroJugador;
        u = 1.0f - m.tiempoPausa / DURACION_REVELADO_GLACIAR;
        alfa = AlfaAnuncioGlaciar(u);

        if (m.ultimoEvento == EVENTO_GLACIAR_AURORA)
        {
            std::snprintf(lineas[0].texto, sizeof(lineas[0].texto), "¡AURORA!");
            lineas[0].token = UI_TEXTO_TITULO;
            lineas[0].color = Color{ 130, 255, 190, 255 };
            std::snprintf(lineas[1].texto, sizeof(lineas[1].texto), "J%d SUMA 3 PUNTOS", numero);
            lineas[1].color = UI_TEXTO_MUNDO;
            cantidad = 2;
        }
        else if (m.ultimoEvento == EVENTO_GLACIAR_PAREJA)
        {
            std::snprintf(lineas[0].texto, sizeof(lineas[0].texto), "¡PAREJA!");
            lineas[0].token = UI_TEXTO_TITULO;
            lineas[0].color = MezclaGlaciar(participantes[m.ultimoJugador].color, WHITE, 0.3f);
            std::snprintf(lineas[1].texto, sizeof(lineas[1].texto), "J%d SUMA 1 PUNTO Y SIGUE", numero);
            lineas[1].color = UI_TEXTO_MUNDO;
            cantidad = 2;
        }
        else if (m.ultimoEvento == EVENTO_GLACIAR_FALLO)
        {
            std::snprintf(lineas[0].texto, sizeof(lineas[0].texto), "SIN PAREJA");
            lineas[0].token = UI_TEXTO_GRANDE;
            lineas[0].color = Color{ 214, 232, 255, 255 };
            std::snprintf(lineas[1].texto, sizeof(lineas[1].texto), "SE VUELVEN A CONGELAR");
            lineas[1].color = UI_TEXTO_MUNDO;
            cantidad = 2;
        }
    }
    else if (m.fase == FASE_GLACIAR_CRUJIDO)
    {
        u = 1.0f - m.tiempoPausa / DURACION_CRUJIDO_GLACIAR;
        alfa = AlfaAnuncioGlaciar(u);
        std::snprintf(lineas[0].texto, sizeof(lineas[0].texto), "¡EL GLACIAR CRUJE!");
        lineas[0].token = UI_TEXTO_TITULO;
        lineas[0].color = Color{ 140, 220, 255, 255 };
        std::snprintf(lineas[1].texto, sizeof(lineas[1].texto), "SE MEZCLAN DOS BLOQUES");
        lineas[1].color = UI_TEXTO_MUNDO;
        cantidad = 2;
    }
    else if (m.fase == FASE_GLACIAR_ELEGIR && m.turno >= 0 && m.tiempoAnuncioTurno < DURACION_ANUNCIO_TURNO_GLACIAR)
    {
        u = m.tiempoAnuncioTurno / DURACION_ANUNCIO_TURNO_GLACIAR;
        alfa = AlfaAnuncioGlaciar(u);
        std::snprintf(lineas[0].texto, sizeof(lineas[0].texto),
            m.turnoRepetido ? "¡OTRA VEZ J%d!" : "¡TURNO DE J%d!", participantes[m.turno].numeroJugador);
        lineas[0].token = UI_TEXTO_GRANDE;
        lineas[0].color = MezclaGlaciar(participantes[m.turno].color, WHITE, 0.3f);
        cantidad = 1;
    }

    if (cantidad == 0 || alfa <= 0.0f)
    {
        return;
    }

    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();
    float y = alto * 0.3f - u * Px(12.0f);

    for (int k = 0; k < cantidad; k++)
    {
        DibujarTextoMundoContornoUI(lineas[k].texto, ancho * 0.5f, y, lineas[k].token, lineas[k].color, alfa);
        y += lineas[k].token == UI_TEXTO_TITULO ? Px(62.0f) : (lineas[k].token == UI_TEXTO_GRANDE ? Px(40.0f) : Px(30.0f));
    }
}


// «+1» o «+3» que sube sobre quien hizo la pareja.
static void DibujarSumaGlaciar(const MinijuegoParejasGlaciar& m, const JugadorPrueba jugadores[], const Camera3D& vista)
{
    if (m.jugadorSuma < 0 || m.tiempoSuma > 1.3f || m.sumaVisual <= 0)
    {
        return;
    }

    float u = m.tiempoSuma / 1.3f;
    float alfa = AlfaAnuncioGlaciar(u);
    if (alfa <= 0.0f) return;

    int j = m.jugadorSuma;
    Vector3 cabeza = PosicionJugadorVisualGlaciar(m, jugadores[j], j);
    cabeza.y += jugadores[j].tamano.y * 0.5f + 1.4f + u * 0.7f;
    Vector2 s = GetWorldToScreen(cabeza, vista);
    char texto[8];
    std::snprintf(texto, sizeof(texto), "+%d", m.sumaVisual);
    Color color = m.sumaVisual >= 3 ? Color{ 130, 255, 190, 255 } : Color{ 236, 246, 255, 255 };
    DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, color, alfa);
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoParejasGlaciar::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;


    // Entrada: durante la cuenta la camara baja desde la aurora.
    Camera3D vista = camara;

    if (fase == FASE_GLACIAR_PREPARACION && resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Limitar01Glaciar((tiempoPreparacion - 0.4f) / (DURACION_PREPARACION_GLACIAR - 0.4f));
        float suave = SuaveGlaciar(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 3.0f * suave;
        vista.position.z += 6.0f * suave;
        vista.target.y += 6.5f * suave;
    }

    // El bucle del juego no limpia la pantalla: limpiar color y
    // profundidad antes del cielo.
    ClearBackground(COLOR_CIELO_GLACIAR);
    DibujarCieloGlaciar(t, efectos);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Cielo y horizonte.
    DibujarAuroraGlaciar(t, efectos, brilloAurora);

    if (decoracion)
    {
        CordilleraGlaciar(-104.0f, -260.0f, 260.0f, 22.0f, 9.0f, 15.0f, 1101,
            Color{ 70, 86, 120, 255 }, Color{ 170, 186, 220, 255 });
    }

    CordilleraGlaciar(-70.0f, -190.0f, 190.0f, paso == 1 ? 13.0f : 20.0f, 6.0f, 11.0f, 1201,
        Color{ 50, 62, 92, 255 }, Color{ 190, 204, 234, 255 });
    DibujarFrenteGlaciar(paso);

    // Mar y media distancia.
    DibujarMarGlaciar(t, efectos);

    for (int k = 0; k < CANTIDAD_ICEBERGS_GLACIAR; k += (decoracion ? 1 : 2))
    {
        DibujarIcebergGlaciar(ICEBERGS_GLACIAR[k], k, t, efectos);
    }

    if (decoracion)
    {
        DibujarTempanosDerivaGlaciar(t, efectos, paso == 1 ? 12 : 6);
        Vector3 foca = { 17.0f, Y_MAR_GLACIAR + VaivenGlaciar(t, 9, efectos), -10.8f };
        PrismaGlaciar({ foca.x, foca.y - 0.15f, foca.z }, { foca.x, foca.y + 0.2f, foca.z }, 1.7f, 1.6f, 7, COLOR_NIEVE_CLARA_GLACIAR, 0.0f, 0.4f);
        FocaGlaciar({ foca.x, foca.y + 0.2f, foca.z }, 160.0f, t, efectos);
    }

    // Banquisa, cueva y utileria.
    DibujarBanquisaGlaciar();
    DibujarCuevaGlaciar(t, decoracion);
    PosteFarolGlaciar(0, t);
    PosteFarolGlaciar(1, t);

    if (decoracion)
    {
        DibujarUtileriaGlaciar(t, paso, efectos);
    }

    // Arena: pozos con los tempanos y el altar con la grilla.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = 0.0f, z0 = 0.0f, z1 = 0.0f;

        if (PozoLadoGlaciar(*this, lado, x, z0, z1))
        {
            DibujarPozoGlaciar(x, z0, z1, t, efectos);
        }
    }

    DibujarTableroGlaciar(*this, participantes);
    DibujarGrietasCrujidoGlaciar(*this, Color{ 20, 36, 70, 255 }, 1.0f);

    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        Color duenio = bloques[i].duenio >= 0 ? participantes[bloques[i].duenio].color : WHITE;
        DibujarBloqueGlaciar(*this, i, t, duenio);
    }

    DibujarCursorGlaciar(*this);
    DibujarFlechaCursorGlaciar(*this, participantes, t);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarTempanoGlaciar(posicionTempanoX[i], posicionTempanoZ[i]);
        }
    }

    DibujarJugadoresGlaciar(*this, jugadores, participantes, mostrarDebug);
    DibujarParticulasSolidasGlaciar(*this);

    DibujarTranslucidosGlaciar(*this, participantes, t, efectos, decoracion);
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    DibujarNieveGlaciar(t);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    if (efectos)
    {
        DibujarLucesGlaciar(*this, participantes, t, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Cartelitos J1..J4 sobre cada jugador; el de turno lo dice.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 cabeza = PosicionJugadorVisualGlaciar(*this, jugadores[i], i);
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.5f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        bool deTurno = turno == i && (fase == FASE_GLACIAR_ELEGIR || fase == FASE_GLACIAR_REVELANDO);
        char rotulo[24];

        if (deTurno)
            std::snprintf(rotulo, sizeof(rotulo), "J%d: TURNO", participantes[i].numeroJugador);
        else
            std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    DibujarSumaGlaciar(*this, jugadores, vista);
    DibujarAnunciosGlaciar(*this, participantes);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "PAREJAS GLACIARES",
        "DERRETÍ BLOQUES DE A DOS Y HACÉ PAREJAS: LA AURORA VALE 3",
        resultado
    );

    hud.fase = fase == FASE_GLACIAR_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_GLACIAR_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_GLACIAR;
    hud.finPorTiempo = tiempoRestante <= 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];

        if (!h.visible)
        {
            continue;
        }

        const EstadoJugadorParejasGlaciar& estado = estadosJugadores[i];

        std::snprintf(h.valor, sizeof(h.valor), estado.puntos == 1 ? "%d PT" : "%d PTS", estado.puntos);
        h.valorNumerico = estado.puntos;
        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), estado.puntos == 1 ? "%d PUNTO" : "%d PUNTOS", estado.puntos);
        std::snprintf(h.detalleDesempate, sizeof(h.detalleDesempate), estado.parejas == 1 ? "%d PAREJA" : "%d PAREJAS", estado.parejas);

        bool deTurno = turno == i && (fase == FASE_GLACIAR_ELEGIR || fase == FASE_GLACIAR_REVELANDO);

        if (deTurno)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡LE TOCA!");

            if (fase == FASE_GLACIAR_ELEGIR)
            {
                h.progreso = Limitar01Glaciar(tiempoTurno / DURACION_TURNO_GLACIAR);
            }
        }
        else if (fase != FASE_GLACIAR_PREPARACION)
        {
            std::snprintf(h.estado, sizeof(h.estado), estado.parejas == 1 ? "%d PAREJA" : "%d PAREJAS", estado.parejas);
        }

        if (participantes[i].activo && !EsControlBotGlaciar(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "%s: CURSOR\n%s: DERRETIR",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoParejasGlaciar::ObtenerResultado() const
{
    return resultado;
}
