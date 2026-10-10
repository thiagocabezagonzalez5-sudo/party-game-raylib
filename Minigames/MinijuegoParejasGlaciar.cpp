#include "Minigames/MinijuegoParejasGlaciar.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


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

    camara.position = { 0.0f, 14.0f, 10.0f };
    camara.target = { 0.0f, 0.0f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

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

    CargarPaqueteParejasGlaciarRetro3D();
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
        estadosJugadores[i].precisionMemoria = GetRandomValue(60, 80);
        ranura++;

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], Vector3{ x, 0.75f, z });
        jugadores[i].direccionMirada = { x > 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f };
    }

    turno = indices[GetRandomValue(0, cantidad - 1)];
}


void MinijuegoParejasGlaciar::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    (void)cantidadMaxima;

    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_GLACIAR_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    ActualizarEfectosGlaciar(*this, deltaTime);

    if (fase == FASE_GLACIAR_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            IniciarTurnoGlaciar(*this);
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarGlaciar(*this);
        return;
    }

    switch (fase)
    {
    case FASE_GLACIAR_ELEGIR:
        ActualizarElegirGlaciar(*this, deltaTime, participantes);
        break;

    case FASE_GLACIAR_REVELANDO:
        ActualizarRevelandoGlaciar(*this, deltaTime);
        break;

    case FASE_GLACIAR_CRUJIDO:
        ActualizarCrujidoGlaciar(*this, deltaTime);
        break;

    case FASE_GLACIAR_PREPARACION:
    case FASE_GLACIAR_TERMINADO:
        break;
    }
}


//==================================================
// VISUAL
//==================================================
//
// GLB compartidos con pivotes originales. El dibujo conserva las animaciones
// del estado; las primitivas se usan solo si falla la pieza correspondiente.
//==================================================


static Color ColorSimboloGlaciar(int simbolo)
{
    const Color colores[8] =
    {
        Color{ 230, 70, 70, 255 },
        Color{ 70, 110, 240, 255 },
        Color{ 245, 210, 60, 255 },
        Color{ 70, 200, 100, 255 },
        Color{ 170, 90, 220, 255 },
        Color{ 240, 150, 50, 255 },
        Color{ 60, 210, 200, 255 },
        Color{ 120, 255, 170, 255 }
    };

    return colores[simbolo & 7];
}


static void DibujarSimboloGlaciar(int simbolo, Vector3 p, float tiempo)
{
    Color color = ColorSimboloGlaciar(simbolo);
    float mezcla = 0.5f + 0.5f * std::sin(tiempo * 3.0f);
    Color brillo = {
        (unsigned char)(120.0f + 120.0f * mezcla),
        (unsigned char)(255.0f - 120.0f * mezcla),
        (unsigned char)(170.0f + 40.0f * mezcla), 255
    };
    if (DibujarModeloParejasGlaciarRetro3D(
        (ModeloParejasGlaciar3D)(MODELO_GLACIAR_SIMBOLO_0 + (simbolo & 7)),
        p, {1,1,1}, brillo))
    {
        // El halo sigue siendo un efecto procedural, no forma parte del GLB.
        if (simbolo == SIMBOLO_AURORA_GLACIAR)
            (DrawSphere)({p.x,p.y + 0.4f,p.z}, 0.55f, Fade(brillo,0.25f));
        return;
    }

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
    {
        // Aurora: estrella que cambia de color.
        (DrawCube)({ p.x, p.y + 0.4f, p.z }, 0.8f, 0.14f, 0.14f, brillo);
        (DrawCube)({ p.x, p.y + 0.4f, p.z }, 0.14f, 0.8f, 0.14f, brillo);
        (DrawCube)({ p.x, p.y + 0.4f, p.z }, 0.14f, 0.14f, 0.8f, brillo);
        (DrawSphere)({ p.x, p.y + 0.4f, p.z }, 0.55f, Fade(brillo, 0.25f));
        break;
    }
    }
}


static void DibujarBloqueGlaciar(const BloqueGlaciar& bloque, int indice, float tiempo, Color colorDuenio)
{
    float x = PosicionXBloqueGlaciar(indice);
    float z = PosicionZBloqueGlaciar(indice);

    if (bloque.temblor > 0.0f)
    {
        x += std::sin(tiempo * 60.0f) * 0.08f;
        z += std::cos(tiempo * 55.0f) * 0.08f;
    }

    float altura = ALTURA_BLOQUE_GLACIAR * (1.0f - 0.8f * bloque.derretido);
    Color hielo = bloque.emparejado
        ? Color{ 150, 190, 215, 255 }
        : Color{ 170, 218, 246, 255 };

    bool modelo = DibujarModeloParejasGlaciarRetro3D(
        bloque.emparejado ? MODELO_GLACIAR_BLOQUE_EMPAREJADO : MODELO_GLACIAR_BLOQUE_OCULTO,
        {x,0,z}, {1,altura / (bloque.emparejado ? 0.26f : ALTURA_BLOQUE_GLACIAR),1},
        colorDuenio, !bloque.emparejado && bloque.derretido >= 0.5f ? 3 : -1);
    if (!modelo)
    {
        (DrawCube)({ x, altura * 0.5f, z }, 1.65f, altura, 1.65f, hielo);
        DrawCubeWires({ x, altura * 0.5f, z }, 1.65f, altura, 1.65f, Color{ 90, 140, 200, 255 });
        if (bloque.emparejado && bloque.duenio >= 0)
            (DrawCylinder)({x,0,z}, 0.75f,0.75f,0.05f,14,colorDuenio);
    }

    if (bloque.derretido < 0.5f && !modelo)
    {
        // Copo tallado en la cara superior.
        Color talla = Color{ 110, 160, 215, 255 };
        float y = altura + 0.01f;
        DrawLine3D({ x - 0.5f, y, z }, { x + 0.5f, y, z }, talla);
        DrawLine3D({ x, y, z - 0.5f }, { x, y, z + 0.5f }, talla);
        DrawLine3D({ x - 0.35f, y, z - 0.35f }, { x + 0.35f, y, z + 0.35f }, talla);
        DrawLine3D({ x - 0.35f, y, z + 0.35f }, { x + 0.35f, y, z - 0.35f }, talla);
    }
    else if (bloque.derretido >= 0.5f)
    {
        DibujarSimboloGlaciar(bloque.simbolo, { x, altura + 0.05f, z }, tiempo);
    }

    if (bloque.temblor > 0.0f)
    {
        // Fragmentos visuales ligados al mismo temblor, sin estado fisico nuevo.
        for (int k = 0; k < 3; k++)
        {
            float a = tiempo * 8.0f + k * 2.0944f;
            Vector3 p = {x + std::cos(a) * 0.9f,
                altura + 0.1f + 0.18f * (1.0f + std::sin(a * 2.0f)),
                z + std::sin(a) * 0.9f};
            if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_FRAGMENTO,
                p, {0.22f,0.22f,0.22f}, WHITE, -1, 0.75f, a * RAD2DEG))
                (DrawCube)(p,0.13f,0.15f,0.13f,Fade(SKYBLUE,0.75f));
        }
    }
}


static void DibujarPinguinoGlaciar(float x, float y, float z)
{
    if (DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_PINGUINO, {x,y,z})) return;
    (DrawCylinder)({ x, y, z }, 0.28f, 0.34f, 0.8f, 8, Color{ 24, 28, 36, 255 });
    (DrawSphere)({ x, y + 0.45f, z + 0.18f }, 0.26f, Color{ 235, 238, 245, 255 });
    (DrawSphere)({ x, y + 0.95f, z }, 0.24f, Color{ 24, 28, 36, 255 });
    (DrawCube)({ x, y + 0.92f, z + 0.25f }, 0.12f, 0.06f, 0.16f, Color{ 245, 160, 40, 255 });
}


static void DibujarFocaGlaciar(float x, float y, float z)
{
    if (DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_FOCA, {x,y,z})) return;
    DrawCylinderEx({ x - 0.7f, y + 0.35f, z }, { x + 0.5f, y + 0.4f, z }, 0.42f, 0.34f, 10, Color{ 120, 128, 140, 255 });
    (DrawSphere)({ x + 0.7f, y + 0.55f, z }, 0.32f, Color{ 130, 138, 150, 255 });
    (DrawSphere)({ x - 0.75f, y + 0.35f, z }, 0.38f, Color{ 120, 128, 140, 255 });
    (DrawSphere)({ x + 0.92f, y + 0.62f, z + 0.14f }, 0.05f, BLACK);
}


static void DibujarEscenarioGlaciar(float tiempo)
{
    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_MAR, {0,0,0}))
        DrawPlane({ 0.0f, -0.6f, -6.0f }, { 120.0f, 90.0f }, Color{ 10, 30, 62, 255 });

    // Lago helado y losa del tablero.
    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_LAGO, {0,0,0}))
        (DrawCube)({ 0.0f, -0.35f, 0.0f }, 20.0f, 0.5f, 11.0f, Color{ 190, 224, 242, 255 });
    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_TABLERO, {0,0,0}))
        (DrawCube)({ 0.0f, -0.12f, 0.0f }, 8.2f, 0.28f, 8.2f, Color{ 120, 170, 215, 255 });

    // Montanas nevadas.
    for (int i = 0; i < 6; i++)
    {
        float x = -22.0f + 8.5f * (float)i;
        float alto = 9.0f + (float)((i * 7) % 5);
        // El GLB tiene la base en -.5 y altura 11: anclar la misma base
        // tras escalar conserva las alturas originales de cada montana.
        if (DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_MONTANA,
            {x,-0.5f + 0.5f * alto / 11.0f,-20}, {1,alto / 11.0f,1})) continue;
        (DrawCylinder)({ x, -0.5f, -20.0f }, 0.0f, 6.0f, alto, 6, Color{ 62, 84, 120, 255 });
        (DrawCylinder)({ x, alto * 0.6f - 0.5f, -20.0f }, 0.0f, 2.4f, alto * 0.4f, 6, Color{ 235, 242, 250, 255 });
    }

    // Aurora detras de las montanas.
    for (int i = 0; i < 9; i++)
    {
        float x = -24.0f + 6.0f * (float)i;
        float alto = 7.0f + 3.0f * std::sin(tiempo * 0.6f + (float)i);
        float escalaY = alto / 11.9067f;
        if (DibujarModeloParejasGlaciarRetro3D(
            (ModeloParejasGlaciar3D)(MODELO_GLACIAR_AURORA_VERDE + i % 3),
            {x,14.0f + alto * 0.3f - 12.14395f * escalaY,-26},
            {1,escalaY,1}, WHITE, -1, 0.22f)) continue;
        Color banda = i % 2 == 0 ? Color{ 80, 255, 160, 255 } : Color{ 200, 90, 240, 255 };
        (DrawCube)({ x, 14.0f + alto * 0.3f, -26.0f }, 5.0f, alto, 0.2f, Fade(banda, 0.22f));
    }

    // Icebergs, cueva de hielo y fauna.
    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_ICEBERG, {-12,0,-9}))
        (DrawCylinder)({ -12.0f, -0.5f, -9.0f }, 0.0f, 3.2f, 4.5f, 5, Color{ 170, 214, 240, 255 });
    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_ICEBERG,
        {11,-0.5f + 0.5f * 5.5f / 4.5f,-10}, {3.8f / 3.2f,5.5f / 4.5f,3.8f / 3.2f}))
        (DrawCylinder)({ 11.0f, -0.5f, -10.0f }, 0.0f, 3.8f, 5.5f, 5, Color{ 150, 200, 235, 255 });
    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_ICEBERG,
        {15,-0.5f + 0.5f * 3.0f / 4.5f,1}, {2.2f / 3.2f,3.0f / 4.5f,2.2f / 3.2f}))
        (DrawCylinder)({ 15.0f, -0.5f, 1.0f }, 0.0f, 2.2f, 3.0f, 5, Color{ 190, 226, 245, 255 });

    if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_CUEVA, {0,0,-11}))
    {
        (DrawCube)({ -4.0f, 1.8f, -11.0f }, 1.2f, 3.6f, 1.5f, Color{ 90, 160, 225, 255 });
        (DrawCube)({ 4.0f, 1.8f, -11.0f }, 1.2f, 3.6f, 1.5f, Color{ 90, 160, 225, 255 });
        (DrawCube)({ 0.0f, 3.9f, -11.0f }, 9.2f, 1.2f, 1.5f, Color{ 110, 180, 235, 255 });
        (DrawCube)({ 0.0f, 1.8f, -11.4f }, 6.8f, 3.6f, 0.4f, Color{ 20, 50, 100, 255 });
    }

    DibujarPinguinoGlaciar(-12.6f, 2.4f, -8.4f);
    DibujarPinguinoGlaciar(-11.6f, 1.2f, -7.4f);
    DibujarPinguinoGlaciar(11.8f, 3.0f, -9.2f);
    DibujarFocaGlaciar(12.5f, -0.4f, -3.0f);
}


static void DibujarTempanoGlaciar(float x, float z, bool activo, float tiempo)
{
    bool modelo = DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_TEMPANO, {x,0,z});
    if (!modelo)
        (DrawCylinder)({ x, -0.45f, z }, 2.1f, 2.4f, 0.5f, 16, Color{ 205, 232, 248, 255 });

    if (activo)
    {
        float pulso = 0.45f + 0.3f * std::sin(tiempo * 6.0f);
        // La tapa del GLB llega a .115: el indicador queda sobre ella.
        (DrawCylinder)({ x, modelo ? 0.12f : 0.06f, z }, 1.9f, 1.9f, 0.04f, 20, Fade(YELLOW, pulso));
    }
}


void MinijuegoParejasGlaciar::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 8, 14, 34, 255 });
    BeginMode3D(camara);

    DibujarEscenarioGlaciar(tiempoAnimacion);

    for (int i = 0; i < CANTIDAD_BLOQUES_GLACIAR; i++)
    {
        Color duenio = bloques[i].duenio >= 0 ? participantes[bloques[i].duenio].color : WHITE;
        DibujarBloqueGlaciar(bloques[i], i, tiempoAnimacion, duenio);
    }

    if (fase == FASE_GLACIAR_ELEGIR && turno >= 0)
    {
        if (!DibujarModeloParejasGlaciarRetro3D(MODELO_GLACIAR_CURSOR,
            {PosicionXBloqueGlaciar(cursor),0,PosicionZBloqueGlaciar(cursor)}))
            DrawCubeWires(
                { PosicionXBloqueGlaciar(cursor), 0.55f, PosicionZBloqueGlaciar(cursor) },
                1.85f, 1.1f, 1.85f, YELLOW
            );
        (DrawCylinder)(
            { PosicionXBloqueGlaciar(cursor), 0.048f, PosicionZBloqueGlaciar(cursor) },
            1.0f,
            1.0f,
            0.04f,
            4,
            Fade(participantes[turno].color, 0.5f)
        );
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool activo =
            i == turno &&
            (fase == FASE_GLACIAR_ELEGIR || fase == FASE_GLACIAR_REVELANDO);

        DibujarTempanoGlaciar(posicionTempanoX[i], posicionTempanoZ[i], activo, tiempoAnimacion);

        JugadorPrueba visual = jugadores[i];
        visual.posicion = {
            posicionTempanoX[i],
            0.05f + visual.tamano.y * 0.5f,
            posicionTempanoZ[i]
        };

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }
    }

    EndMode3D();

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Marcador sobre cada tempano.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { posicionTempanoX[i], 2.7f, posicionTempanoZ[i] },
            camara
        );
        const char* etiqueta = TextFormat(
            "J%d: %d PTS",
            participantes[i].numeroJugador,
            estadosJugadores[i].puntos
        );

        int etiquetaAncho = MeasureText(etiqueta, 20);
        DrawRectangle(
            (int)pantalla.x - etiquetaAncho / 2 - 8,
            (int)pantalla.y - 4,
            etiquetaAncho + 16,
            46,
            Fade(BLACK, 0.72f)
        );
        DrawText(
            etiqueta,
            (int)pantalla.x - etiquetaAncho / 2,
            (int)pantalla.y,
            20,
            participantes[i].color
        );
        DrawText(
            TextFormat("%d PAREJAS", estadosJugadores[i].parejas),
            (int)pantalla.x - 40,
            (int)pantalla.y + 22,
            15,
            RAYWHITE
        );
    }

    DrawRectangle(18, 16, 700, 112, Fade(BLACK, 0.79f));
    DrawText("PAREJAS GLACIARES", 32, 24, 28, GOLD);

    if (turno >= 0 && fase != FASE_GLACIAR_PREPARACION && fase != FASE_GLACIAR_TERMINADO)
    {
        DrawText(
            TextFormat(
                "TURNO DE J%d%s",
                participantes[turno].numeroJugador,
                participantes[turno].esBot ? " (BOT)" : ""
            ),
            32,
            58,
            20,
            participantes[turno].color
        );
    }

    DrawText("MOVER = CURSOR  |  ACCION (E / SHIFT DER / B) = REVELAR BLOQUE", 32, 84, 15, RAYWHITE);
    DrawText("AURORA = 3 PUNTOS  |  CADA 4 FALLOS SEGUIDOS EL GLACIAR CRUJE", 32, 104, 15, RAYWHITE);

    if (fase != FASE_GLACIAR_PREPARACION)
    {
        DrawText(
            TextFormat("TIEMPO %.0f", tiempoRestante),
            ancho - 190,
            24,
            23,
            tiempoRestante <= 10.0f ? RED : GOLD
        );
    }

    if (fase == FASE_GLACIAR_ELEGIR)
    {
        DrawText(
            TextFormat("ELIGE %.1f", tiempoTurno > 0.0f ? tiempoTurno : 0.0f),
            ancho - 190,
            54,
            20,
            tiempoTurno <= 2.0f ? RED : RAYWHITE
        );
    }

    const char* mensaje = nullptr;

    if (fase == FASE_GLACIAR_REVELANDO && ultimoJugador >= 0)
    {
        int numero = participantes[ultimoJugador].numeroJugador;

        if (ultimoEvento == EVENTO_GLACIAR_AURORA)
            mensaje = TextFormat("J%d ENCONTRO LA AURORA: +3", numero);
        else if (ultimoEvento == EVENTO_GLACIAR_PAREJA)
            mensaje = TextFormat("J%d HIZO PAREJA: +1", numero);
        else if (ultimoEvento == EVENTO_GLACIAR_FALLO)
            mensaje = "SIN PAREJA";
    }
    else if (fase == FASE_GLACIAR_CRUJIDO)
    {
        mensaje = "EL GLACIAR CRUJE: SE BARAJAN 2 BLOQUES";
    }

    if (mensaje != nullptr)
    {
        DrawText(mensaje, ancho / 2 - MeasureText(mensaje, 24) / 2, alto - 100, 24, YELLOW);
    }

    if (fase == FASE_GLACIAR_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            ancho / 2 - MeasureText(texto, 88) / 2,
            alto / 2 - 54,
            88,
            GOLD
        );
    }
    else if (
        fase == FASE_GLACIAR_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR)
        {
            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (
                    resultado.participantes[i].participo &&
                    resultado.participantes[i].posicionFinal == 1
                )
                {
                    titulo = TextFormat("GANA J%d", participantes[i].numeroJugador);
                }
            }
        }

        DrawRectangle(ancho / 2 - 290, alto / 2 - 92, 580, 184, Fade(BLACK, 0.91f));
        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, 34) / 2,
            alto / 2 - 51,
            34,
            GOLD
        );
        DrawText(
            TextoReinicioMinijuego(),
            ancho / 2 - MeasureText(TextoReinicioMinijuego(), 21) / 2,
            alto / 2 + 25,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoParejasGlaciar::ObtenerResultado() const
{
    return resultado;
}
