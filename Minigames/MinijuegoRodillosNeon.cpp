#include "Minigames/MinijuegoRodillosNeon.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>


//==================================================
// CONSTANTES (REGLAS Y MAQUINA)
//==================================================

static const float DURACION_PREPARACION_RODILLOS = 3.0f;
static const float DURACION_AVISO_RODILLOS = 1.0f;
static const float DURACION_RESULTADO_RODILLOS = 2.4f;
// Tope por ronda y de toda la partida: siempre termina.
static const float TOPE_GIRO_RODILLOS = 14.0f;
static const float TIEMPO_LIMITE_RODILLOS = 100.0f;
static const float DURACION_FRENADO = 0.18f;

static const int PUNTOS_TRIPLE = 10;
static const int PUNTOS_TRIPLE_ESTRELLA = 20;
static const int PUNTOS_PAR = 3;

static const float PASO_SIMBOLO = 2.0f * PI / (float)SIMBOLOS_POR_RODILLO;
// Ventana del comodin: fraccion del paso a cada lado de la linea central.
static const float VENTANA_COMODIN_MINIMA = 0.14f * PASO_SIMBOLO;
// Semiancho minimo en tiempo (30 ms por lado = ventana total de 60 ms).
static const float SEMIVENTANA_TIEMPO_COMODIN = 0.03f;

// Velocidad de giro por ronda (grados por segundo).
static const float VELOCIDAD_RONDA_GRADOS[RONDAS_RODILLOS_NEON] =
{
    100.0f, 130.0f, 160.0f, 190.0f, 230.0f
};

static const float RADIO_RODILLO = 1.7f;
static const float ALTURA_RODILLOS = 3.0f;
static const float SEPARACION_MAQUINAS = 5.8f;
static const float SEPARACION_RODILLOS = 1.05f;

static const Color COLORES_SIMBOLOS[TIPOS_SIMBOLO_NEON] =
{
    { 0, 230, 255, 255 },
    { 255, 60, 200, 255 },
    { 130, 255, 80, 255 },
    { 255, 160, 40, 255 },
    { 255, 235, 90, 255 }
};

static const Color COLORES_JUGADORES_RODILLOS[MAX_PARTICIPANTES] =
{
    { 235, 80, 80, 255 },
    { 80, 140, 240, 255 },
    { 90, 205, 115, 255 },
    { 245, 205, 70, 255 }
};


//==================================================
// UTILIDADES LOGICAS
//==================================================

static float Acotar(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float Aleatorio01()
{
    return (float)GetRandomValue(0, 10000) / 10000.0f;
}


// Aproximacion de una normal estandar (suma de 4 uniformes, Irwin-Hall).
static float AleatorioGaussiano()
{
    float suma = 0.0f;

    for (int i = 0; i < 4; i++)
    {
        suma += Aleatorio01();
    }

    return (suma - 2.0f) / 0.577f;
}


static bool JugadorEsBot(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static const char* NombreJugadorRodillos(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


// Ventana del comodin (radianes a cada lado de la linea) para una ronda: nunca
// baja de 60 ms en total aunque el rodillo gire mas rapido.
static float VentanaComodin(int ronda)
{
    int indice = ronda < RONDAS_RODILLOS_NEON ? ronda : RONDAS_RODILLOS_NEON - 1;
    float porTiempo = VELOCIDAD_RONDA_GRADOS[indice] * DEG2RAD * SEMIVENTANA_TIEMPO_COMODIN;

    return porTiempo > VENTANA_COMODIN_MINIMA ? porTiempo : VENTANA_COMODIN_MINIMA;
}


// Normaliza un angulo a (-PI, PI].
static float NormalizarAngulo(float angulo)
{
    while (angulo > PI) angulo -= 2.0f * PI;
    while (angulo <= -PI) angulo += 2.0f * PI;

    return angulo;
}


// Tira del rodillo: dos simbolos de cada tipo, sin repetir vecinos si se puede.
static void BarajarTira(RodilloNeon& rodillo)
{
    for (int intento = 0; intento < 60; intento++)
    {
        for (int k = 0; k < SIMBOLOS_POR_RODILLO; k++)
        {
            rodillo.tira[k] = k % TIPOS_SIMBOLO_NEON;
        }

        for (int k = SIMBOLOS_POR_RODILLO - 1; k > 0; k--)
        {
            int j = GetRandomValue(0, k);
            int temporal = rodillo.tira[k];
            rodillo.tira[k] = rodillo.tira[j];
            rodillo.tira[j] = temporal;
        }

        bool valida = true;

        for (int k = 0; k < SIMBOLOS_POR_RODILLO; k++)
        {
            if (rodillo.tira[k] == rodillo.tira[(k + 1) % SIMBOLOS_POR_RODILLO])
            {
                valida = false;
                break;
            }
        }

        if (valida)
        {
            return;
        }
    }
}


// Indice entero del simbolo mas cercano a la linea central y su desvio.
static int SimboloEnLinea(const RodilloNeon& rodillo, float& desvio)
{
    int vuelta = (int)std::lround(-rodillo.angulo / PASO_SIMBOLO);
    desvio = rodillo.angulo + (float)vuelta * PASO_SIMBOLO;

    return ((vuelta % SIMBOLOS_POR_RODILLO) + SIMBOLOS_POR_RODILLO) % SIMBOLOS_POR_RODILLO;
}


static bool EstaDetenido(const EstadoJugadorRodillos& e)
{
    for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
    {
        if (e.rodillos[r].estado != RODILLO_DETENIDO)
        {
            return false;
        }
    }

    return true;
}


// Evalua los 3 rodillos detenidos. Los comodines coinciden con cualquiera.
static ResultadoRondaRodillos EvaluarRodillos(const EstadoJugadorRodillos& e, int& puntos)
{
    int comodines = 0;
    int tipos[CANTIDAD_RODILLOS_NEON]{};
    int cantidadTipos = 0;

    for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
    {
        if (e.rodillos[r].comodin)
        {
            comodines++;
        }
        else
        {
            tipos[cantidadTipos++] = e.rodillos[r].simbolo;
        }
    }

    // Tres comodines: mejor triple posible.
    if (cantidadTipos == 0)
    {
        puntos = PUNTOS_TRIPLE_ESTRELLA;
        return RONDA_RODILLOS_TRIPLE_ESTRELLA;
    }

    bool todosIguales = true;

    for (int k = 1; k < cantidadTipos; k++)
    {
        if (tipos[k] != tipos[0])
        {
            todosIguales = false;
        }
    }

    if (todosIguales)
    {
        if (tipos[0] == SIMBOLO_ESTRELLA_NEON)
        {
            puntos = PUNTOS_TRIPLE_ESTRELLA;
            return RONDA_RODILLOS_TRIPLE_ESTRELLA;
        }

        puntos = PUNTOS_TRIPLE;
        return RONDA_RODILLOS_TRIPLE;
    }

    // Hay al menos un par si hay un comodin o dos simbolos iguales.
    bool par = comodines > 0;

    for (int a = 0; a < cantidadTipos && !par; a++)
    {
        for (int b = a + 1; b < cantidadTipos; b++)
        {
            if (tipos[a] == tipos[b])
            {
                par = true;
            }
        }
    }

    if (par)
    {
        puntos = PUNTOS_PAR;
        return RONDA_RODILLOS_PAR;
    }

    puntos = 0;
    return RONDA_RODILLOS_NADA;
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarRodillos(MinijuegoRodillosNeon& m)
{
    if (m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int cantidadPrimeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorRodillos& a = m.estadosJugadores[i];
        int posicion = 1;

        // Mas puntos; si empatan, mas triples; si no, empate.
        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (j == i || !m.resultado.participantes[j].participo)
            {
                continue;
            }

            const EstadoJugadorRodillos& b = m.estadosJugadores[j];

            if (b.puntos > a.puntos || (b.puntos == a.puntos && b.triples > a.triples))
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = a.puntos;
        resultadoJugador.puntosObtenidos = 0;

        if (posicion == 1)
        {
            cantidadPrimeros++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace =
        cantidadPrimeros == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    m.fase = FASE_RODILLOS_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

static void ConfigurarCamaraRodillos(MinijuegoRodillosNeon& m)
{
    m.camara.up = { 0.0f, 1.0f, 0.0f };
    m.camara.projection = CAMERA_PERSPECTIVE;
    m.camara.fovy = 45.0f;

    if (m.cantidadMaquinas <= 2)
    {
        m.camara.position = { 0.0f, 5.4f, 14.5f };
        m.camara.target = { 0.0f, 2.4f, 1.0f };
    }
    else if (m.cantidadMaquinas == 3)
    {
        m.camara.position = { 0.0f, 5.8f, 18.0f };
        m.camara.target = { 0.0f, 2.4f, 1.0f };
    }
    else
    {
        m.camara.position = { 0.0f, 6.5f, 21.5f };
        m.camara.target = { 0.0f, 2.4f, 1.0f };
    }
}


void MinijuegoRodillosNeon::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadMaquinas = 0;
    ronda = 0;
    etapa = ETAPA_RODILLOS_AVISO;
    fase = FASE_RODILLOS_PREPARACION;
    estadoGlitch = 3;
    glitchJugador = -1;
    glitchRodillo = -1;
    glitchInvierte = false;
    tiempoGlitch = 0.0f;
    tiempoPreparacion = DURACION_PREPARACION_RODILLOS;
    tiempoRestante = TIEMPO_LIMITE_RODILLOS;
    tiempoEtapa = 0.0f;
    tiempoAnimacion = 0.0f;
    enfriamientoTick = 0.0f;

    ConfigurarCamaraRodillos(*this);
}


// Deja todos los rodillos girando para una ronda nueva.
static void PrepararRondaRodillos(MinijuegoRodillosNeon& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorRodillos& e = m.estadosJugadores[i];
        e.rodilloActual = 0;
        e.puntosRonda = 0;
        e.resultadoRonda = RONDA_RODILLOS_NADA;
        e.tiempoRodilloActual = 0.0f;
        e.objetivoSimbolo = -1;
        e.retardoBot = 0.4f + 1.6f * Aleatorio01();
        e.errorTiming = AleatorioGaussiano() * e.sigmaTiming;

        for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
        {
            RodilloNeon& rodillo = e.rodillos[r];
            BarajarTira(rodillo);
            rodillo.angulo = Aleatorio01() * 2.0f * PI;
            rodillo.estado = RODILLO_GIRANDO;
            rodillo.simbolo = -1;
            rodillo.comodin = false;
            rodillo.tiempoFreno = 0.0f;
            float desvio = 0.0f;
            SimboloEnLinea(rodillo, desvio);
            rodillo.ultimoCruce = (int)std::lround(-rodillo.angulo / PASO_SIMBOLO);
        }
    }

    // Fallo de sistema de esta ronda en un instante aleatorio.
    m.estadoGlitch = 0;
    m.glitchJugador = -1;
    m.glitchRodillo = -1;
    m.tiempoGlitch = 1.2f + 2.8f * Aleatorio01();
}


void MinijuegoRodillosNeon::Reiniciar(
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

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    int total = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            total++;
        }
    }

    if (total < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_RODILLOS_TERMINADO;
        return;
    }

    cantidadMaquinas = total;
    CargarPaqueteRodillosNeonRetro3D();
    ConfigurarCamaraRodillos(*this);

    int maquina = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        EstadoJugadorRodillos& e = estadosJugadores[i];
        e.maquina = maquina;
        e.posicionX = ((float)maquina - (float)(total - 1) * 0.5f) * SEPARACION_MAQUINAS;
        e.sigmaTiming = 0.05f + 0.07f * Aleatorio01();

        jugadores[i].posicion = { e.posicionX, 0.0f, 3.0f };
        jugadores[i].posicionSpawn = jugadores[i].posicion;
        maquina++;
    }

    PrepararRondaRodillos(*this, limite);
}


//==================================================
// LOGICA DE RODILLOS
//==================================================

struct SonidosFrameRodillos
{
    bool detener = false;
    bool tick = false;
};


static void DetenerRodillo(
    EstadoJugadorRodillos& e,
    int indiceRodillo,
    bool esHumano,
    float ventanaComodin,
    SonidosFrameRodillos& sonidos
)
{
    RodilloNeon& rodillo = e.rodillos[indiceRodillo];

    if (rodillo.estado != RODILLO_GIRANDO)
    {
        return;
    }

    float desvio = 0.0f;
    int indice = SimboloEnLinea(rodillo, desvio);
    int vuelta = (int)std::lround(-rodillo.angulo / PASO_SIMBOLO);

    rodillo.simbolo = rodillo.tira[indice];
    rodillo.comodin = std::fabs(desvio) <= ventanaComodin;
    rodillo.estado = RODILLO_FRENANDO;
    rodillo.tiempoFreno = 0.0f;
    rodillo.anguloInicialFreno = rodillo.angulo;
    rodillo.anguloFinalFreno = -(float)vuelta * PASO_SIMBOLO;

    e.rodilloActual = indiceRodillo + 1;
    e.tiempoGesto = 0.25f;
    e.tiempoRodilloActual = 0.0f;
    e.anguloPrevioValido = false;

    if (esHumano)
    {
        sonidos.detener = true;
    }
}


// Decide a que simbolo apuntara el bot en el rodillo que acaba de quedar activo.
static void ElegirObjetivoBot(EstadoJugadorRodillos& e)
{
    e.objetivoSimbolo = -1;
    e.tiempoRodilloActual = 0.0f;
    e.anguloPrevioValido = false;
    e.retardoBot = 0.2f + 1.2f * Aleatorio01();
    e.errorTiming = AleatorioGaussiano() * e.sigmaTiming;

    int actual = e.rodilloActual;

    if (actual <= 0 || actual >= CANTIDAD_RODILLOS_NEON)
    {
        return;
    }

    // ~55% de las veces busca repetir un simbolo ya detenido.
    if (Aleatorio01() >= 0.55f)
    {
        return;
    }

    const RodilloNeon& primero = e.rodillos[0];
    const RodilloNeon& segundo = e.rodillos[1];

    if (actual == 1)
    {
        if (!primero.comodin)
        {
            e.objetivoSimbolo = primero.simbolo;
        }
    }
    else
    {
        // Con dos iguales va por el triple; si no, repite uno de ellos.
        if (!primero.comodin && !segundo.comodin && primero.simbolo == segundo.simbolo)
        {
            e.objetivoSimbolo = primero.simbolo;
        }
        else if (!segundo.comodin)
        {
            e.objetivoSimbolo = segundo.simbolo;
        }
        else if (!primero.comodin)
        {
            e.objetivoSimbolo = primero.simbolo;
        }
    }
}


// true si el bot decide detener el rodillo actual en este frame.
static bool BotQuiereDetener(
    EstadoJugadorRodillos& e,
    const RodilloNeon& rodillo,
    float variacionAngulo,
    float deltaTime
)
{
    e.tiempoRodilloActual += deltaTime;

    // Sin objetivo: espera un tiempo y detiene al azar.
    if (e.objetivoSimbolo < 0)
    {
        return e.tiempoRodilloActual >= e.retardoBot;
    }

    // Con objetivo: espera a que ese simbolo cruce la linea (mas el error de
    // timing); si tarda demasiado, detiene igualmente.
    if (e.tiempoRodilloActual > 5.0f)
    {
        return true;
    }

    float velocidadAngular = deltaTime > 0.0001f ? variacionAngulo / deltaTime : 0.0f;
    float objetivoDesvio = velocidadAngular * e.errorTiming;
    float tolerancia = std::fabs(variacionAngulo) * 0.6f + 0.0005f;

    for (int k = 0; k < SIMBOLOS_POR_RODILLO; k++)
    {
        if (rodillo.tira[k] != e.objetivoSimbolo)
        {
            continue;
        }

        float posicion = NormalizarAngulo(rodillo.angulo + (float)k * PASO_SIMBOLO);

        if (std::fabs(posicion - objetivoDesvio) <= tolerancia)
        {
            return true;
        }
    }

    return false;
}


static void ResolverRonda(
    MinijuegoRodillosNeon& m,
    int limite,
    const Participante participantes[]
)
{
    bool humanoTriple = false;
    bool humanoPar = false;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorRodillos& e = m.estadosJugadores[i];
        int puntos = 0;
        e.resultadoRonda = EvaluarRodillos(e, puntos);
        e.puntosRonda = puntos;
        e.puntos += puntos;

        if (
            e.resultadoRonda == RONDA_RODILLOS_TRIPLE ||
            e.resultadoRonda == RONDA_RODILLOS_TRIPLE_ESTRELLA
        )
        {
            e.triples++;
        }

        if (!JugadorEsBot(participantes[i]))
        {
            if (e.resultadoRonda >= RONDA_RODILLOS_TRIPLE) humanoTriple = true;
            else if (e.resultadoRonda == RONDA_RODILLOS_PAR) humanoPar = true;
        }
    }

    if (humanoTriple)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
    }
    else if (humanoPar)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_MONEDA);
    }
}


// Programa y aplica el fallo de sistema (aviso parpadeante y luego efecto).
static void ActualizarGlitch(
    MinijuegoRodillosNeon& m,
    float deltaTime,
    int limite
)
{
    if (m.estadoGlitch == 0)
    {
        m.tiempoGlitch -= deltaTime;

        if (m.tiempoGlitch > 0.0f)
        {
            return;
        }

        // Elige un jugador con algun rodillo aun girando.
        int candidatos[MAX_PARTICIPANTES]{};
        int cantidad = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo)
            {
                continue;
            }

            if (m.estadosJugadores[i].rodilloActual < CANTIDAD_RODILLOS_NEON)
            {
                candidatos[cantidad++] = i;
            }
        }

        if (cantidad == 0)
        {
            m.estadoGlitch = 3;
            return;
        }

        int jugador = candidatos[GetRandomValue(0, cantidad - 1)];
        const EstadoJugadorRodillos& e = m.estadosJugadores[jugador];
        int primero = e.rodilloActual;

        m.glitchJugador = jugador;
        m.glitchRodillo = GetRandomValue(primero, CANTIDAD_RODILLOS_NEON - 1);
        m.glitchInvierte = GetRandomValue(0, 1) == 0;
        m.estadoGlitch = 1;
        m.tiempoGlitch = 0.8f;
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
        return;
    }

    if (m.estadoGlitch == 1 || m.estadoGlitch == 2)
    {
        m.tiempoGlitch -= deltaTime;

        if (m.tiempoGlitch > 0.0f)
        {
            return;
        }

        if (m.estadoGlitch == 1)
        {
            m.estadoGlitch = 2;
            m.tiempoGlitch = 1.0f;
        }
        else
        {
            m.estadoGlitch = 3;
        }
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoRodillosNeon::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;

    if (resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    if (deltaTime > 0.05f)
    {
        deltaTime = 0.05f;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    tiempoAnimacion += deltaTime;

    if (fase == FASE_RODILLOS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        // Los rodillos giran suavemente de adorno durante la cuenta atras.
        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
            {
                estadosJugadores[i].rodillos[r].angulo -=
                    VELOCIDAD_RONDA_GRADOS[0] * DEG2RAD * 0.5f * deltaTime;
            }
        }

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_RODILLOS_JUGANDO;
            etapa = ETAPA_RODILLOS_AVISO;
            tiempoEtapa = 0.0f;
        }

        return;
    }

    if (fase != FASE_RODILLOS_JUGANDO)
    {
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarRodillos(*this);
        return;
    }

    tiempoEtapa += deltaTime;
    enfriamientoTick -= deltaTime;
    SonidosFrameRodillos sonidos;

    // Los rodillos ya detenidos siguen su frenado en cualquier etapa.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        if (estadosJugadores[i].tiempoGesto > 0.0f)
        {
            estadosJugadores[i].tiempoGesto -= deltaTime;
        }

        for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
        {
            RodilloNeon& rodillo = estadosJugadores[i].rodillos[r];

            if (rodillo.estado == RODILLO_FRENANDO)
            {
                rodillo.tiempoFreno += deltaTime;
                float k = Acotar(rodillo.tiempoFreno / DURACION_FRENADO, 0.0f, 1.0f);
                float suave = k * k * (3.0f - 2.0f * k);
                rodillo.angulo =
                    rodillo.anguloInicialFreno +
                    (rodillo.anguloFinalFreno - rodillo.anguloInicialFreno) * suave;

                if (k >= 1.0f)
                {
                    rodillo.estado = RODILLO_DETENIDO;
                    rodillo.angulo = rodillo.anguloFinalFreno;
                }
            }
        }
    }

    if (etapa == ETAPA_RODILLOS_AVISO)
    {
        if (tiempoEtapa >= DURACION_AVISO_RODILLOS)
        {
            etapa = ETAPA_RODILLOS_GIRO;
            tiempoEtapa = 0.0f;
        }
    }
    else if (etapa == ETAPA_RODILLOS_GIRO)
    {
        ActualizarGlitch(*this, deltaTime, limite);

        float velocidadBase = VELOCIDAD_RONDA_GRADOS[ronda < RONDAS_RODILLOS_NEON ? ronda : RONDAS_RODILLOS_NEON - 1] * DEG2RAD;
        bool todosListos = true;

        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            EstadoJugadorRodillos& e = estadosJugadores[i];
            bool esBot = JugadorEsBot(participantes[i]);
            float variacionActual = 0.0f;

            // Giro de cada rodillo (el glitch cambia solo el suyo).
            for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
            {
                RodilloNeon& rodillo = e.rodillos[r];

                if (rodillo.estado != RODILLO_GIRANDO)
                {
                    continue;
                }

                float factor = 1.0f;

                if (estadoGlitch == 2 && glitchJugador == i && glitchRodillo == r)
                {
                    factor = glitchInvierte ? -1.0f : 2.0f;
                }

                float cambio = -velocidadBase * factor * deltaTime;
                rodillo.angulo += cambio;

                if (r == e.rodilloActual)
                {
                    variacionActual = cambio;
                }

                // Marca cada simbolo que pasa por la linea (tick del rodillo humano).
                int cruce = (int)std::lround(-rodillo.angulo / PASO_SIMBOLO);

                if (cruce != rodillo.ultimoCruce)
                {
                    rodillo.ultimoCruce = cruce;

                    if (!esBot && r == e.rodilloActual && enfriamientoTick <= 0.0f)
                    {
                        sonidos.tick = true;
                        enfriamientoTick = 0.125f;
                    }
                }
            }

            if (e.rodilloActual >= CANTIDAD_RODILLOS_NEON)
            {
                continue;
            }

            todosListos = false;

            // Detener el rodillo actual.
            bool detener = false;

            if (esBot)
            {
                RodilloNeon& actual = e.rodillos[e.rodilloActual];

                detener = BotQuiereDetener(e, actual, variacionActual, deltaTime);
            }
            else
            {
                InputMinijuegoParticipante entrada = LeerInputMinijuegoParticipante(participantes[i]);
                detener = entrada.golpear || entrada.saltar;
            }

            if (detener)
            {
                int indice = e.rodilloActual;
                DetenerRodillo(e, indice, !esBot, VentanaComodin(ronda), sonidos);

                if (esBot && e.rodilloActual < CANTIDAD_RODILLOS_NEON)
                {
                    ElegirObjetivoBot(e);
                    e.anguloPrevioValido = true;
                }
            }
        }

        // Tope por ronda: detiene en seco lo que siga girando.
        if (tiempoEtapa >= TOPE_GIRO_RODILLOS)
        {
            for (int i = 0; i < limite; i++)
            {
                if (!resultado.participantes[i].participo)
                {
                    continue;
                }

                EstadoJugadorRodillos& e = estadosJugadores[i];

                for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
                {
                    DetenerRodillo(e, r, false, 0.0f, sonidos);
                }

                e.rodilloActual = CANTIDAD_RODILLOS_NEON;
            }

            todosListos = true;
        }

        // La ronda termina cuando todos los rodillos de todos estan quietos.
        if (todosListos)
        {
            bool todoQuieto = true;

            for (int i = 0; i < limite; i++)
            {
                if (resultado.participantes[i].participo && !EstaDetenido(estadosJugadores[i]))
                {
                    todoQuieto = false;
                }
            }

            if (todoQuieto)
            {
                ResolverRonda(*this, limite, participantes);
                etapa = ETAPA_RODILLOS_RESULTADO;
                tiempoEtapa = 0.0f;
                estadoGlitch = 3;
            }
        }
    }
    else if (etapa == ETAPA_RODILLOS_RESULTADO)
    {
        if (tiempoEtapa >= DURACION_RESULTADO_RODILLOS)
        {
            ronda++;

            if (ronda >= RONDAS_RODILLOS_NEON)
            {
                FinalizarRodillos(*this);
            }
            else
            {
                PrepararRondaRodillos(*this, limite);
                etapa = ETAPA_RODILLOS_AVISO;
                tiempoEtapa = 0.0f;
            }
        }
    }

    if (sonidos.detener)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
    }
    else if (sonidos.tick)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_RULETA_TICK);
    }
}


//==================================================
// VISUAL: GLB COMPARTIDOS Y FALLBACK POR PIEZA
//==================================================
// La logica no depende de las mallas. Las transformaciones locales de la
// maquina y los hologramas se heredan una vez; nunca se repite su origen.
//==================================================

// Poligono plano relleno en el plano XY local (visible por ambas caras).
static void DibujarPoligonoPlano(
    int lados,
    float radioPar,
    float radioImpar,
    float anguloInicio,
    Color color
)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = anguloInicio + 2.0f * PI * (float)k / (float)lados;
        float a1 = anguloInicio + 2.0f * PI * (float)(k + 1) / (float)lados;
        float r0 = (k % 2 == 0) ? radioPar : radioImpar;
        float r1 = ((k + 1) % 2 == 0) ? radioPar : radioImpar;
        Vector3 centro = { 0.0f, 0.0f, 0.0f };
        Vector3 p0 = { std::cos(a0) * r0, std::sin(a0) * r0, 0.0f };
        Vector3 p1 = { std::cos(a1) * r1, std::sin(a1) * r1, 0.0f };

        DrawTriangle3D(centro, p0, p1, color);
        DrawTriangle3D(centro, p1, p0, color);
    }
}


// Simbolo geometrico en el plano local XY (la cara mira a +Z).
static void DibujarSimboloNeon(int tipo, float tamano, Color color)
{
    switch (tipo)
    {
        case 0:
            DibujarPoligonoPlano(3, tamano * 1.15f, tamano * 1.15f, PI * 0.5f, color);
            break;
        case 1:
            DibujarPoligonoPlano(16, tamano, tamano, 0.0f, color);
            break;
        case 2:
            DibujarPoligonoPlano(4, tamano * 1.2f, tamano * 1.2f, PI * 0.25f, color);
            break;
        case 3:
            DibujarPoligonoPlano(4, tamano * 1.25f, tamano * 1.25f, 0.0f, color);
            break;
        default:
            DibujarPoligonoPlano(10, tamano * 1.25f, tamano * 0.55f, PI * 0.5f, color);
            break;
    }
}


static void DibujarRodillo(
    const RodilloNeon& rodillo,
    float x,
    bool glitch,
    float t
)
{
    // Pivote del eje X: el giro real incluye frenado y fallo de sistema.
    if (!DibujarModeloRodillosNeonRetro3D(MODELO_RODILLOS_TAMBOR,
        {x,0,0}, -rodillo.angulo * RAD2DEG, {1,0,0}))
    {
        (DrawCylinderEx)(
            { x - 0.4f, 0.0f, 0.0f },
            { x + 0.4f, 0.0f, 0.0f },
            RADIO_RODILLO, RADIO_RODILLO, 20, Color{ 16, 14, 30, 255 }
        );
    }

    for (int k = 0; k < SIMBOLOS_POR_RODILLO; k++)
    {
        float angulo = NormalizarAngulo(rodillo.angulo + (float)k * PASO_SIMBOLO);

        // Solo se dibujan los simbolos de la cara frontal.
        if (std::fabs(angulo) > 1.45f)
        {
            continue;
        }

        Color color = COLORES_SIMBOLOS[rodillo.tira[k]];

        if (glitch)
        {
            float parpadeo = 0.5f + 0.5f * std::sin(t * 40.0f + (float)k);
            color = Color{ 255, (unsigned char)(60 + 120 * parpadeo), 220, 255 };
        }

        rlPushMatrix();
        // Composicion equivalente a tambor + k*36 grados, sin doble giro.
        Vector3 posicion = {x, (RADIO_RODILLO + .03f) * std::sin(angulo),
            (RADIO_RODILLO + .03f) * std::cos(angulo)};
        if (!DibujarSimboloRodillosNeonRetro3D(rodillo.tira[k], glitch, posicion,
            -angulo * RAD2DEG, color))
        {
            // Conserva exactamente la posicion de las primitivas originales.
            rlTranslatef(x, RADIO_RODILLO * std::sin(angulo), (RADIO_RODILLO + .03f) * std::cos(angulo));
            rlRotatef(-angulo * RAD2DEG, 1,0,0);
            DibujarSimboloNeon(rodillo.tira[k], 0.36f, color);
        }
        rlPopMatrix();
    }
}


static void DibujarMaquina(
    const MinijuegoRodillosNeon& m,
    int indice,
    Color colorJugador
)
{
    const EstadoJugadorRodillos& e = m.estadosJugadores[indice];
    float t = m.tiempoAnimacion;
    float pulso = 0.5f + 0.5f * std::sin(t * 5.0f);

    rlPushMatrix();
    rlTranslatef(e.posicionX, 0.0f, 0.0f);

    // Gabinete: base, laterales, cabecera y panel de control.
    if (!DibujarModeloRodillosNeonRetro3D(MODELO_RODILLOS_GABINETE))
    {
        (DrawCube)({ 0.0f, 0.75f, 0.3f }, 4.4f, 1.5f, 2.8f, Color{ 28, 22, 46, 255 });
        (DrawCube)({ -1.95f, 3.0f, 0.2f }, 0.5f, 3.2f, 3.4f, Color{ 34, 28, 56, 255 });
        (DrawCube)({ 1.95f, 3.0f, 0.2f }, 0.5f, 3.2f, 3.4f, Color{ 34, 28, 56, 255 });
        (DrawCube)({ 0.0f, 4.95f, 0.2f }, 4.4f, 1.2f, 3.4f, Color{ 30, 24, 52, 255 });
        (DrawCube)({ 0.0f, 1.55f, 1.95f }, 3.4f, 0.4f, 0.3f, Color{ 24, 20, 40, 255 });
        (DrawCube)({ 0.0f, 4.5f, 1.95f }, 3.4f, 0.55f, 0.3f, Color{ 24, 20, 40, 255 });

        // Estos aros ya pertenecen al gabinete GLB.
        DrawCircle3D({ -1.2f, 5.0f, 1.99f }, .32f, {0,0,1}, 0, COLORES_SIMBOLOS[1]);
        DrawCircle3D({ 0, 5.0f, 1.99f }, .32f, {0,0,1}, 0, COLORES_SIMBOLOS[0]);
        DrawCircle3D({ 1.2f, 5.0f, 1.99f }, .32f, {0,0,1}, 0, COLORES_SIMBOLOS[4]);
    }

    // Bordes neon del color del jugador.
    if (!DibujarModeloRodillosNeonRetro3D(MODELO_RODILLOS_MARCO,
        {}, 0, {0,1,0}, {1,1,1}, colorJugador))
    {
        (DrawCube)({ -2.2f, 2.9f, 1.95f }, 0.07f, 4.4f, 0.07f, colorJugador);
        (DrawCube)({ 2.2f, 2.9f, 1.95f }, 0.07f, 4.4f, 0.07f, colorJugador);
        (DrawCube)({ 0.0f, 5.55f, 1.95f }, 4.4f, 0.07f, 0.07f, colorJugador);
        (DrawCube)({ 0.0f, 0.04f, 1.75f }, 4.4f, 0.07f, 0.07f, colorJugador);
    }

    // Boton de detener, brillando cuando todavia queda un rodillo por parar.
    bool pendiente = e.rodilloActual < CANTIDAD_RODILLOS_NEON && m.etapa == ETAPA_RODILLOS_GIRO;
    Color colorBoton = pendiente ? Fade(colorJugador, .6f + .4f * pulso) : Fade(colorJugador, .35f);
    if (!DibujarModeloRodillosNeonRetro3D(MODELO_RODILLOS_BOTON,
        {0,1.2f,1.9f}, 0, {0,1,0}, {1,1,1}, colorBoton))
    {
        (DrawCube)({ 0.0f, 1.2f, 1.9f }, 1.5f, 0.35f, 0.8f, Color{ 40, 34, 66, 255 });
        (DrawSphere)({ 0.0f, 1.42f, 1.95f }, 0.3f, pendiente ? Fade(colorJugador, 0.6f + 0.4f * pulso) : Fade(colorJugador, 0.35f));
    }

    // Rodillos centrados en la altura de la ventana.
    rlPushMatrix();
    rlTranslatef(0.0f, ALTURA_RODILLOS, 0.0f);

    for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
    {
        float x = ((float)r - 1.0f) * SEPARACION_RODILLOS;
        bool glitch =
            m.estadoGlitch == 2 && m.glitchJugador == indice && m.glitchRodillo == r &&
            e.rodillos[r].estado == RODILLO_GIRANDO;
        DibujarRodillo(e.rodillos[r], x, glitch, t);

        // Resalte del simbolo detenido y marca de comodin.
        const RodilloNeon& rodillo = e.rodillos[r];

        if (rodillo.estado == RODILLO_DETENIDO || rodillo.estado == RODILLO_FRENANDO)
        {
            Color halo = rodillo.comodin ? Color{ 255, 235, 90, 255 } : Color{ 255, 255, 255, 255 };
            DrawCircle3D({ x, 0.0f, RADIO_RODILLO + 0.08f }, 0.52f, { 0.0f, 0.0f, 1.0f }, 0.0f, Fade(halo, rodillo.comodin ? 1.0f : 0.5f));

            if (rodillo.comodin)
            {
                DrawCircle3D({ x, 0.0f, RADIO_RODILLO + 0.08f }, 0.58f + 0.05f * pulso, { 0.0f, 0.0f, 1.0f }, 0.0f, halo);
            }
        }

        // Aviso del fallo de sistema: el marco parpadea en magenta.
        if (
            m.estadoGlitch == 1 && m.glitchJugador == indice && m.glitchRodillo == r &&
            std::fmod(t, 0.2f) < 0.1f
        )
        {
            (DrawCube)({ x, 0.0f, RADIO_RODILLO + 0.2f }, 0.95f, 2.6f, 0.05f, Fade(Color{ 255, 40, 200, 255 }, 0.55f));
        }
    }

    // Linea central y banda de comodin.
    float alturaComodin = RADIO_RODILLO * std::sin(VentanaComodin(m.ronda));
    bool lineaGLB = DibujarModeloAnimadoRodillosNeonRetro3D(MODELO_RODILLOS_LINEA,
        {0,0,RADIO_RODILLO + .14f}, t, alturaComodin);
    if (!lineaGLB)
        (DrawCube)({ 0.0f, 0.0f, RADIO_RODILLO + 0.14f }, 3.5f, 0.03f, 0.02f, Color{ 255, 255, 255, 255 });
    // Transparencia procedural necesaria para mostrar la ventana temporal.
    (DrawCube)({ 0.0f, 0.0f, RADIO_RODILLO + 0.12f }, 3.3f, alturaComodin * 2.0f, 0.02f, Fade(Color{ 255, 235, 90, 255 }, 0.25f));
    if (!lineaGLB)
    {
        DrawTriangle3D({ -1.9f, 0.14f, RADIO_RODILLO + 0.2f }, { -1.9f, -0.14f, RADIO_RODILLO + 0.2f }, { -1.65f, 0.0f, RADIO_RODILLO + 0.2f }, Color{ 255, 235, 90, 255 });
        DrawTriangle3D({ 1.9f, -0.14f, RADIO_RODILLO + 0.2f }, { 1.9f, 0.14f, RADIO_RODILLO + 0.2f }, { 1.65f, 0.0f, RADIO_RODILLO + 0.2f }, Color{ 255, 235, 90, 255 });
    }
    rlPopMatrix();

    rlPopMatrix();
}


static void DibujarSalaArcade(float t)
{
    // Suelo oscuro con rejilla luminosa.
    if (!DibujarModeloAnimadoRodillosNeonRetro3D(MODELO_RODILLOS_SUELO, {}, t))
    {
        (DrawCube)({ 0.0f, -0.2f, 0.0f }, 60.0f, 0.4f, 40.0f, Color{ 10, 8, 20, 255 });

        for (int i = -15; i <= 15; i++)
        {
            float brillo = 0.5f + 0.5f * std::sin(t * 2.0f + (float)i * 0.5f);
            Color color = (i % 2 == 0)
                ? Color{ 0, (unsigned char)(120 + 100 * brillo), 255, 255 }
                : Color{ (unsigned char)(160 + 90 * brillo), 40, 220, 255 };
            DrawLine3D({ (float)i * 2.0f, 0.01f, -16.0f }, { (float)i * 2.0f, 0.01f, 12.0f }, color);
        }

        for (int i = -8; i <= 6; i++)
        {
            DrawLine3D({ -30.0f, 0.01f, (float)i * 2.0f }, { 30.0f, 0.01f, (float)i * 2.0f }, Color{ 40, 80, 190, 255 });
        }

    }

    // Pared del fondo con rejilla.
    if (!DibujarModeloRodillosNeonRetro3D(MODELO_RODILLOS_PARED))
    {
        (DrawCube)({ 0.0f, 7.0f, -9.0f }, 60.0f, 16.0f, 0.4f, Color{ 14, 10, 30, 255 });

        for (int i = -14; i <= 14; i++)
        {
            DrawLine3D({ (float)i * 2.0f, 0.0f, -8.75f }, { (float)i * 2.0f, 15.0f, -8.75f }, Color{ 40, 30, 90, 255 });
        }

    }

    // Columnas de LEDs animadas.
    for (int c = 0; c < 9; c++)
    {
        float x = -24.0f + 6.0f * (float)c;

        auto pieza = c % 2 ? MODELO_RODILLOS_LED_ROSA : MODELO_RODILLOS_LED_CIAN;
        if (DibujarModeloAnimadoRodillosNeonRetro3D(pieza, {x,0,-8.5f}, t, (float)c)) continue;

        (DrawCube)({ x, 6.5f, -8.5f }, 0.5f, 13.0f, 0.4f, Color{ 22, 18, 40, 255 });

        for (int k = 0; k < 10; k++)
        {
            float fase = 0.5f + 0.5f * std::sin(t * 3.0f - (float)k * 0.6f + (float)c);
            Color color = (c % 2 == 0)
                ? Color{ 0, (unsigned char)(80 + 170 * fase), 255, 255 }
                : Color{ (unsigned char)(120 + 135 * fase), 40, 230, 255 };
            (DrawCube)({ x, 1.2f + 1.25f * (float)k, -8.25f }, 0.3f, 0.5f, 0.15f, color);
        }
    }

    // Holograma de formas girando sobre las maquinas.
    for (int k = 0; k < 3; k++)
    {
        float x = -12.0f + 12.0f * (float)k;
        float y = 9.5f + 0.5f * std::sin(t * 1.5f + (float)k);

        rlPushMatrix();
        rlTranslatef(x, y, -6.0f);
        rlRotatef(t * 40.0f + (float)k * 60.0f, 0.0f, 1.0f, 0.0f);
        rlRotatef(20.0f, 1.0f, 0.0f, 0.0f);

        auto pieza = static_cast<ModeloRodillosNeon3D>(MODELO_RODILLOS_HOLOGRAMA_CUBO + k);
        if (!DibujarModeloRodillosNeonRetro3D(pieza))
        {
            if (k == 0) DrawCubeWires({ 0.0f, 0.0f, 0.0f }, 1.8f, 1.8f, 1.8f, COLORES_SIMBOLOS[0]);
            else if (k == 1) DrawSphereWires({ 0.0f, 0.0f, 0.0f }, 1.1f, 8, 10, COLORES_SIMBOLOS[1]);
            else DrawCylinderWires({ 0.0f, -0.9f, 0.0f }, 0.0f, 1.2f, 1.8f, 4, COLORES_SIMBOLOS[4]);
        }

        rlPopMatrix();
    }

    // Letreros neon sin texto: marcos con simbolos.
    for (int k = 0; k < 4; k++)
    {
        float x = -18.0f + 12.0f * (float)k;
        auto pieza = static_cast<ModeloRodillosNeon3D>(MODELO_RODILLOS_LETRERO_CIAN + k);
        if (DibujarModeloRodillosNeonRetro3D(pieza, {x,12.3f,-8.4f})) continue;
        (DrawCube)({ x, 11.5f, -8.4f }, 4.6f, 0.08f, 0.1f, COLORES_SIMBOLOS[k % TIPOS_SIMBOLO_NEON]);
        (DrawCube)({ x, 13.1f, -8.4f }, 4.6f, 0.08f, 0.1f, COLORES_SIMBOLOS[k % TIPOS_SIMBOLO_NEON]);
        (DrawCube)({ x - 2.3f, 12.3f, -8.4f }, 0.08f, 1.7f, 0.1f, COLORES_SIMBOLOS[k % TIPOS_SIMBOLO_NEON]);
        (DrawCube)({ x + 2.3f, 12.3f, -8.4f }, 0.08f, 1.7f, 0.1f, COLORES_SIMBOLOS[k % TIPOS_SIMBOLO_NEON]);

        rlPushMatrix();
        rlTranslatef(x, 12.3f, -8.3f);
        DibujarSimboloNeon(k % TIPOS_SIMBOLO_NEON, 0.55f, COLORES_SIMBOLOS[(k + 2) % TIPOS_SIMBOLO_NEON]);
        rlPopMatrix();
    }
}


//==================================================
// DIBUJO
//==================================================

static const char* TextoResultadoRonda(ResultadoRondaRodillos resultado)
{
    switch (resultado)
    {
        case RONDA_RODILLOS_PAR: return "PAR";
        case RONDA_RODILLOS_TRIPLE: return "TRIPLE";
        case RONDA_RODILLOS_TRIPLE_ESTRELLA: return "TRIPLE ESTRELLA";
        default: return "SIN PREMIO";
    }
}


void MinijuegoRodillosNeon::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    ClearBackground(Color{ 6, 4, 16, 255 });
    BeginMode3D(camara);

    DibujarSalaArcade(tiempoAnimacion);

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarMaquina(*this, i, COLORES_JUGADORES_RODILLOS[i % MAX_PARTICIPANTES]);
            // Personaje compartido de pie delante de su maquina, mirandola.
            const EstadoJugadorRodillos& estado = estadosJugadores[i];
            JugadorPrueba visualJugador = jugadores[i];
            visualJugador.posicion = { estado.posicionX, 0.7f, 3.7f };
            visualJugador.velocidad = {};
            visualJugador.direccionMirada = { 0.0f, 0.0f, -1.0f };
            visualJugador.enSuelo = true;
            visualJugador.cayendo = false;
            visualJugador.golpeando = estado.tiempoGesto > 0.0f;
            visualJugador.tiempoGolpe = estado.tiempoGesto > 0.0f ? estado.tiempoGesto : 0.0f;

            Participante visual = participantes[i];
            visual.conectado = true;
            visual.color = COLORES_JUGADORES_RODILLOS[i % MAX_PARTICIPANTES];
            DibujarJugadorCuboPrueba(visualJugador, visual);
        }
    }

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    int centro = anchoPantalla / 2;

    // Paneles oscuros para que el HUD no se mezcle con los hologramas.
    DrawRectangle(16, 12, 600, 68, Fade(BLACK, 0.72f));
    DrawRectangle(anchoPantalla - 196, 12, 180, 66, Fade(BLACK, 0.72f));
    DrawText("RODILLOS NEON", 28, 20, 28, Color{ 0, 230, 255, 255 });
    DrawText("DETIENE LOS RODILLOS: 3 IGUALES = 10 (ESTRELLA = 20), 2 IGUALES = 3", 28, 52, 16, Color{ 255, 120, 220, 255 });

    const char* textoRonda = TextFormat(
        "RONDA %d / %d",
        ronda < RONDAS_RODILLOS_NEON ? ronda + 1 : RONDAS_RODILLOS_NEON,
        RONDAS_RODILLOS_NEON
    );
    DrawText(textoRonda, anchoPantalla - MeasureText(textoRonda, 26) - 28, 20, 26, GOLD);

    if (fase == FASE_RODILLOS_JUGANDO)
    {
        int nivel = ronda < RONDAS_RODILLOS_NEON ? ronda + 1 : RONDAS_RODILLOS_NEON;
        const char* velocidad = TextFormat("VELOCIDAD x%d", nivel);
        DrawText(velocidad, anchoPantalla - MeasureText(velocidad, 18) - 28, 50, 18, LIGHTGRAY);
    }

    // Aviso de fallo de sistema.
    if (estadoGlitch == 1 || estadoGlitch == 2)
    {
        bool parpadeo = std::fmod(tiempoAnimacion, 0.3f) < 0.15f;

        if (parpadeo || estadoGlitch == 2)
        {
            const char* texto = estadoGlitch == 1
                ? "FALLO DE SISTEMA..."
                : (glitchInvierte ? "GLITCH: RODILLO INVERTIDO" : "GLITCH: RODILLO ACELERADO");
            int textoAncho = MeasureText(texto, 26);
            DrawRectangle(centro - textoAncho / 2 - 14, 82, textoAncho + 28, 38, Fade(BLACK, 0.8f));
            DrawText(texto, centro - MeasureText(texto, 26) / 2, 86, 26, Color{ 255, 60, 210, 255 });
        }
    }

    // Tarjetas por maquina (nombre, puntos, estado de la ronda).
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorRodillos& e = estadosJugadores[i];
        Color colorJugador = COLORES_JUGADORES_RODILLOS[i % MAX_PARTICIPANTES];
        Vector2 baseMaquina = GetWorldToScreen({ e.posicionX, 5.9f, 0.0f }, camara);
        Vector2 pie = GetWorldToScreen({ e.posicionX, 0.0f, 1.0f }, camara);

        const char* nombre = NombreJugadorRodillos(participantes[i], i);
        DrawText(nombre, (int)baseMaquina.x - MeasureText(nombre, 22) / 2, (int)baseMaquina.y - 26, 22, colorJugador);

        int anchoTarjeta = 230;
        int altoTarjeta = 92;
        int px = (int)pie.x - anchoTarjeta / 2;
        int py = altoPantalla - altoTarjeta - 36;

        if (px < 6) px = 6;
        if (px + anchoTarjeta > anchoPantalla - 6) px = anchoPantalla - 6 - anchoTarjeta;

        DrawRectangle(px, py, anchoTarjeta, altoTarjeta, Fade(BLACK, 0.72f));
        DrawRectangleLines(px, py, anchoTarjeta, altoTarjeta, colorJugador);
        DrawText(TextFormat("%d PTS", e.puntos), px + 10, py + 6, 26, RAYWHITE);
        DrawText(TextFormat("TRIPLES %d", e.triples), px + 10, py + 38, 16, LIGHTGRAY);

        if (etapa == ETAPA_RODILLOS_RESULTADO)
        {
            const char* texto = TextoResultadoRonda(e.resultadoRonda);
            Color colorTexto = e.resultadoRonda >= RONDA_RODILLOS_TRIPLE ? GOLD
                : (e.resultadoRonda == RONDA_RODILLOS_PAR ? RAYWHITE : GRAY);
            DrawText(TextFormat("%s  %+d", texto, e.puntosRonda), px + 10, py + 62, 18, colorTexto);
        }
        else if (fase == FASE_RODILLOS_JUGANDO && etapa == ETAPA_RODILLOS_GIRO)
        {
            if (e.rodilloActual < CANTIDAD_RODILLOS_NEON)
            {
                DrawText(TextFormat("RODILLO %d", e.rodilloActual + 1), px + 10, py + 62, 18, colorJugador);
            }
            else
            {
                DrawText("LISTO", px + 10, py + 62, 18, GREEN);
            }
        }
        else if (JugadorEsBot(participantes[i]))
        {
            DrawText("AUTOMATICO", px + 10, py + 62, 18, LIGHTGRAY);
        }
        else
        {
            DrawText("DETENER: ESPACIO / E", px + 10, py + 62, 16, LIGHTGRAY);
        }
    }

    const char* ayuda = "ACCION: detener el rodillo actual (izquierda a derecha). Parar justo en la linea = COMODIN";
    DrawText(ayuda, centro - MeasureText(ayuda, 16) / 2, altoPantalla - 26, 16, LIGHTGRAY);

    if (fase == FASE_RODILLOS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, centro - MeasureText(texto, 96) / 2, altoPantalla / 2 - 150, 96, GOLD);
    }
    else if (fase == FASE_RODILLOS_JUGANDO && etapa == ETAPA_RODILLOS_AVISO)
    {
        const char* aviso = TextFormat("RONDA %d", ronda + 1);
        DrawText(aviso, centro - MeasureText(aviso, 48) / 2, altoPantalla / 2 - 150, 48, GOLD);
    }
    else if (
        fase == FASE_RODILLOS_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int activos = 0;

        for (int i = 0; i < limite; i++)
        {
            if (resultado.participantes[i].participo) activos++;
        }

        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = centro - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 60;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE EN LA SALA ARCADE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorRodillos(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, centro - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (
                    !resultado.participantes[i].participo ||
                    resultado.participantes[i].posicionFinal != posicion
                )
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d puntos   (%d triples)",
                        posicion,
                        NombreJugadorRodillos(participantes[i], i),
                        estadosJugadores[i].puntos,
                        estadosJugadores[i].triples
                    ),
                    px + 30,
                    py + 62 + fila * 30,
                    22,
                    COLORES_JUGADORES_RODILLOS[i % MAX_PARTICIPANTES]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            centro - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }

    if (mostrarDebug)
    {
        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat("J%d actual=%d obj=%d", i + 1, estadosJugadores[i].rodilloActual, estadosJugadores[i].objetivoSimbolo),
                28,
                120 + 18 * i,
                14,
                LIME
            );
        }
    }
}


const ResultadoMinijuego& MinijuegoRodillosNeon::ObtenerResultado() const
{
    return resultado;
}
