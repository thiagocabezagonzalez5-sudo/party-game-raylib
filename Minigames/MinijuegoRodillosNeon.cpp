#include "Minigames/MinijuegoRodillosNeon.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HorneadoMallas.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>


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
// PIEZAS GLB JUGABLES
//==================================================
// Del paquete GLB solo se cargan las piezas que muestran el estado del
// juego: marco y boton del color del jugador, tambores, simbolos y linea de
// pago con la banda del comodin (tramo seguido del enum). El gabinete y la
// sala arcade se construyen con primitivas propias horneadas en mallas.

static_assert(MODELO_RODILLOS_MARCO + 8 == MODELO_RODILLOS_LINEA,
    "Marco, boton, tambor, simbolos y linea deben ir seguidos en el paquete Rodillos Neon");

static void CargarPiezasJugablesRodillos()
{
    // Mismas primitives por GLB que exige CargarPaqueteRodillosNeonRetro3D.
    static const int mallasEsperadas[CANTIDAD_MODELOS_RODILLOS_NEON_3D] =
        { 4,3,7,2,4,4,2,3,2,2,2,2,4,4,1,1,1,4,5,4,4 };

    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteRodillosNeonRetro3D();
    PaqueteModelosEscenarioRetro3D tramo = paquete;
    tramo.recursos += MODELO_RODILLOS_MARCO;
    tramo.definiciones += MODELO_RODILLOS_MARCO;
    tramo.cantidad = MODELO_RODILLOS_LINEA - MODELO_RODILLOS_MARCO + 1;
    CargarPaqueteModelosEscenarioRetro3D(tramo);

    for (int i = MODELO_RODILLOS_MARCO; i <= MODELO_RODILLOS_LINEA; i++)
    {
        RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];
        if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i]) continue;
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.cargado = false;
        TraceLog(LOG_WARNING, "Paquete Rodillos Neon incompatible; se usan primitivas: %s", recurso.ruta);
    }

    // Copia en CPU de la linea: la banda del comodin mueve sus bordes.
    AnimacionRodillosRetro3D& animacion = ObtenerModelosEscenariosRetro3D().animacionesRodillos[3];
    RecursoModeloEscenarioRetro3D& linea = paquete.recursos[MODELO_RODILLOS_LINEA];

    if (animacion.intentada || !linea.cargado)
    {
        return;
    }

    animacion.intentada = true;
    Mesh& malla = linea.modelo.meshes[1];
    bool valido = malla.vboId && malla.vboId[0];

    if (valido)
    {
        animacion.vertices = static_cast<float*>(MemAlloc(malla.vertexCount * 3 * sizeof(float)));
        valido = animacion.vertices != nullptr;
        if (valido) std::memcpy(animacion.vertices, malla.vertices, malla.vertexCount * 3 * sizeof(float));
    }

    if (!valido)
    {
        MemFree(animacion.vertices);
        animacion.vertices = nullptr;
        UnloadModel(linea.modelo);
        linea.modelo = {};
        linea.cargado = false;
        TraceLog(LOG_WARNING, "No se puede animar la pieza de Rodillos Neon; se usan primitivas: %s", linea.ruta);
    }
}


static void LiberarMallasSalaRodillos();


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
    tiempoFin = 0.0f;

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
    // Cada partida vuelve a hornear la sala (la fila de maquinas cambia).
    LiberarMallasSalaRodillos();
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
    CargarPiezasJugablesRodillos();
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
    rodillo.tiempoParada = 0.0f;
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

    // Reloj visual: sigue con el tablon final (no toca ninguna regla).
    float deltaVisual = deltaTime > 0.05f ? 0.05f : deltaTime;
    tiempoVisual += deltaVisual;

    if (resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        tiempoFin += deltaVisual;
    }

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

            if (rodillo.estado != RODILLO_GIRANDO)
            {
                rodillo.tiempoParada += deltaTime;
            }

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
// VISUAL: SALA ARCADE RETRO DE LOS 80
//==================================================
// Casino arcade de noche: alfombra estampada de luz negra, fila de
// tragamonedas con marquesina de bombillas y corona del color de cada
// jugador, maquinas de otros juegos con pantallas en modo demo, cabina de
// premios con peluches, grua de peluches, rocola, pinball, columnas de LEDs,
// letreros de tubos de neon que titilan y, por dos arcos, un segundo salon
// perdido en la bruma. Lo estatico se hornea en mallas una vez por partida y
// por calidad; lo animado va en pocos lotes de triangulos.
//
// Del paquete GLB solo quedan las piezas jugables (marco, boton, tambores,
// simbolos y linea de pago); el resto se construye con primitivas.
//==================================================

static const Color COLOR_FONDO_R = { 14, 9, 26, 255 };
static const Color COLOR_BRUMA_R = { 36, 20, 60, 255 };
static const Color COLOR_ALFOMBRA_R = { 44, 30, 80, 255 };
static const Color COLOR_PARED_R = { 70, 36, 88, 255 };
static const Color COLOR_PARED_OSCURA_R = { 42, 24, 60, 255 };
static const Color COLOR_ZOCALO_R = { 86, 28, 48, 255 };
static const Color COLOR_TECHO_R = { 26, 18, 40, 255 };
static const Color COLOR_CROMO_R = { 186, 192, 212, 255 };
static const Color COLOR_ORO_R = { 232, 184, 88, 255 };
static const Color COLOR_LACA_R = { 116, 28, 52, 255 };
static const Color COLOR_NEGRO_R = { 18, 14, 26, 255 };
static const Color COLOR_ROSA_R = { 255, 70, 190, 255 };
static const Color COLOR_CIAN_R = { 60, 230, 255, 255 };
static const Color COLOR_AMARILLO_R = { 255, 222, 96, 255 };
static const Color COLOR_VIOLETA_R = { 170, 100, 255, 255 };
static const Color COLOR_CALIDO_R = { 255, 214, 150, 255 };
static const Color COLOR_GLITCH_R = { 255, 40, 200, 255 };

static const float Z_PARED_R = -11.0f;
static const float ALTO_TECHO_R = 11.5f;
static const float ARCO_X0_R = 12.5f;
static const float ARCO_X1_R = 19.5f;
static const float ARCO_ALTO_R = 6.6f;
static const float PILASTRAS_R[6] = { -20.4f, -11.6f, -7.4f, 7.4f, 11.6f, 20.4f };
static const int BOMBILLAS_MARQUESINA_R = 30;


//--------------------------------------------------
// Utilidades de color y azar visual (semilla propia)
//--------------------------------------------------

static float HashR(int a, int b)
{
    unsigned int h = (unsigned int)a * 73856093u ^ (unsigned int)b * 19349663u ^ 0x2c1b3c6du;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float Lim01R(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalR(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoR(Color c, float f)
{
    return Color{ CanalR((float)c.r * f), CanalR((float)c.g * f), CanalR((float)c.b * f), c.a };
}


static Color MezclaR(Color a, Color b, float t)
{
    t = Lim01R(t);
    return Color{
        CanalR((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalR((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalR((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalR((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaR(Color c, float alfa)
{
    c.a = CanalR(alfa * 255.0f);
    return c;
}


static float XMaquinaR(int k, int n)
{
    return ((float)k - (float)(n - 1) * 0.5f) * SEPARACION_MAQUINAS;
}


// Medio ancho de la fila de maquinas (incluye la palanca).
static float MitadFilaR(int n)
{
    return n > 0 ? (float)(n - 1) * 0.5f * SEPARACION_MAQUINAS + 2.4f : 4.0f;
}


//--------------------------------------------------
// Emision de triangulos (inmediata o grabada en malla)
//--------------------------------------------------

static GrabadorMalla g_grabR;
static Vector3 g_ojoR = { 0.0f, 7.0f, 24.0f };
static float g_emisR = 0.0f;        // 1 = superficie luminosa: casi no la tapa la bruma
static Vector3 g_derR = { 1.0f, 0.0f, 0.0f };   // ejes de pantalla (billboards)
static Vector3 g_arribaR = { 0.0f, 1.0f, 0.0f };


static Color BrumaR(Color c, Vector3 p)
{
    float dx = p.x - g_ojoR.x;
    float dy = p.y - g_ojoR.y;
    float dz = p.z - g_ojoR.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01R((d - 20.0f) / 46.0f) * 0.80f * (1.0f - 0.6f * g_emisR);
    Color r = MezclaR(c, COLOR_BRUMA_R, n);
    r.a = c.a;
    return r;
}


static void VR(Vector3 p, Color c)
{
    if (g_grabR.activa)
    {
        GrabarVerticeMalla(g_grabR, p, c);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteR()
{
    if (!g_grabR.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteR()
{
    if (!g_grabR.activa) rlEnd();
}


static void TriR(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VR(a, BrumaR(color, a));
    VR(b, BrumaR(color, b));
    VR(c, BrumaR(color, c));
}


static void QuadR(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriR(a, b, c, color);
    TriR(a, c, d, color);
}


// Quad sin bruma con un color por vertice (luces aditivas y sombras suaves).
static void QuadColoresR(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VR(a, ca); VR(b, cb); VR(c, cc);
    VR(a, ca); VR(c, cc); VR(d, cd);
}


// Caja con caras sombreadas (arriba 1.0, frente 0.80, izquierda 0.70, derecha 0.62).
static void CajaR(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    QuadR({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, color);
    QuadR({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoR(color, 0.80f));
    QuadR({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoR(color, 0.52f));
    QuadR({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoR(color, 0.70f));
    QuadR({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoR(color, 0.62f));

    if (y0 > 0.02f)
    {
        QuadR({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoR(color, 0.40f));
    }
}


// Rectangulo luminoso mirando a +Z.
static void PanelR(float x0, float y0, float x1, float y1, float z, Color color)
{
    float emis = g_emisR;
    g_emisR = 1.0f;
    QuadR({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, color);
    g_emisR = emis;
}


static float LuzR(Vector3 n)
{
    float d = n.x * 0.32f + n.y * 0.84f + n.z * 0.44f;
    return 0.56f + 0.46f * Lim01R(d);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas por normal.
static void CilindroR(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    eje = Vector3Scale(eje, 1.0f / largo);
    Vector3 referencia = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, referencia));
    Vector3 v = Vector3CrossProduct(eje, u);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = 0.5f * (a0 + a1);
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));
        Color cara = TonoR(color, LuzR(nm));

        QuadR(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            cara
        );

        if (tapas)
        {
            TriR(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), TonoR(color, LuzR(eje)));
            TriR(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), TonoR(color, LuzR(Vector3Negate(eje))));
        }
    }
}


static void BolaR(Vector3 c, float r, Color color, int anillos = 4, int lados = 8)
{
    for (int i = 0; i < anillos; i++)
    {
        float la0 = -0.5f * PI + PI * (float)i / (float)anillos;
        float la1 = -0.5f * PI + PI * (float)(i + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float lo0 = 2.0f * PI * (float)k / (float)lados;
            float lo1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float lam = 0.5f * (la0 + la1), lom = 0.5f * (lo0 + lo1);
            Vector3 n = { std::cos(lam) * std::cos(lom), std::sin(lam), std::cos(lam) * std::sin(lom) };
            auto P = [&](float la, float lo)
            {
                return Vector3{ c.x + r * std::cos(la) * std::cos(lo), c.y + r * std::sin(la), c.z + r * std::cos(la) * std::sin(lo) };
            };
            QuadR(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), TonoR(color, LuzR(n)));
        }
    }
}


static void DiscoR(Vector3 c, Vector3 u, Vector3 v, float r, int lados, Color color)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        TriR(
            c,
            Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a0) * r), Vector3Scale(v, std::sin(a0) * r))),
            Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a1) * r), Vector3Scale(v, std::sin(a1) * r))),
            color
        );
    }
}


// Trazo plano de a a b sobre el plano de normal n (letras de neon, lineas
// de la alfombra). Los extremos se alargan medio ancho: uniones sin huecos.
static void TrazoR(Vector3 a, Vector3 b, Vector3 n, float ancho, Color color)
{
    Vector3 d = Vector3Subtract(b, a);
    float largo = Vector3Length(d);

    if (largo < 0.0001f)
    {
        return;
    }

    d = Vector3Scale(d, 1.0f / largo);
    Vector3 lado = Vector3Scale(Vector3Normalize(Vector3CrossProduct(n, d)), ancho * 0.5f);
    Vector3 extra = Vector3Scale(d, ancho * 0.5f);
    Vector3 a2 = Vector3Subtract(a, extra);
    Vector3 b2 = Vector3Add(b, extra);

    QuadR(Vector3Add(a2, lado), Vector3Subtract(a2, lado), Vector3Subtract(b2, lado), Vector3Add(b2, lado), color);
}


//--------------------------------------------------
// Luces aditivas (sin bruma, borde transparente)
//--------------------------------------------------

static void GlowR(Vector3 c, Vector3 u, Vector3 v, float r, Color color)
{
    const int LADOS = 16;
    Color borde = color;
    borde.a = 0;

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
        VR(c, color);
        VR(Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a0) * r), Vector3Scale(v, std::sin(a0) * r))), borde);
        VR(Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a1) * r), Vector3Scale(v, std::sin(a1) * r))), borde);
    }
}


static void GlowCaraR(Vector3 c, float r, Color color)
{
    GlowR(c, g_derR, g_arribaR, r, color);
}


static void GlowSueloR(Vector3 c, float r, Color color)
{
    GlowR({ c.x, c.y, c.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, r, color);
}


static void GlowTrazoR(Vector3 a, Vector3 b, Vector3 n, float ancho, Color color)
{
    Vector3 d = Vector3Subtract(b, a);
    float largo = Vector3Length(d);

    if (largo < 0.0001f)
    {
        return;
    }

    d = Vector3Scale(d, 1.0f / largo);
    Vector3 lado = Vector3Scale(Vector3Normalize(Vector3CrossProduct(n, d)), ancho * 0.5f);
    Vector3 extra = Vector3Scale(d, ancho * 0.25f);
    Vector3 a2 = Vector3Subtract(a, extra);
    Vector3 b2 = Vector3Add(b, extra);
    Color borde = color;
    borde.a = 0;

    QuadColoresR(a2, b2, Vector3Add(b2, lado), Vector3Add(a2, lado), color, color, borde, borde);
    QuadColoresR(a2, b2, Vector3Subtract(b2, lado), Vector3Subtract(a2, lado), color, color, borde, borde);
}


// Sombra suave en el suelo: rectangulo opaco que se desvanece hacia afuera.
static void SombraRectR(float x0, float z0, float x1, float z1, float margen, float alfa)
{
    Color dentro = AlfaR(Color{ 6, 2, 14, 255 }, alfa);
    Color fuera = AlfaR(Color{ 6, 2, 14, 255 }, 0.0f);
    float y = 0.0f;
    Vector3 a = { x0, y, z0 }, b = { x1, y, z0 }, c = { x1, y, z1 }, d = { x0, y, z1 };
    Vector3 A = { x0 - margen, y, z0 - margen }, B = { x1 + margen, y, z0 - margen };
    Vector3 C = { x1 + margen, y, z1 + margen }, D = { x0 - margen, y, z1 + margen };

    QuadColoresR(a, b, c, d, dentro, dentro, dentro, dentro);
    QuadColoresR(A, B, b, a, fuera, fuera, dentro, dentro);
    QuadColoresR(B, C, c, b, fuera, fuera, dentro, dentro);
    QuadColoresR(C, D, d, c, fuera, fuera, dentro, dentro);
    QuadColoresR(D, A, a, d, fuera, fuera, dentro, dentro);
}


static void SombraRedondaR(float x, float z, float radio, float alfa)
{
    const int LADOS = 14;
    Color dentro = AlfaR(Color{ 6, 2, 14, 255 }, alfa);
    Color fuera = AlfaR(Color{ 6, 2, 14, 255 }, 0.0f);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
        VR({ x, 0.0f, z }, dentro);
        VR({ x + std::cos(a0) * radio, 0.0f, z + std::sin(a0) * radio }, fuera);
        VR({ x + std::cos(a1) * radio, 0.0f, z + std::sin(a1) * radio }, fuera);
    }
}


//--------------------------------------------------
// Letras de tubo de neon (grilla 2 x 3, -1 levanta el lapiz, -2 termina)
//--------------------------------------------------

static const float* TrazosLetraR(char c)
{
    static const float J[] = { 2,3, 2,0.7f, 1.4f,0, 0.6f,0, 0,0.7f, -2 };
    static const float A[] = { 0,0, 0,2, 1,3, 2,2, 2,0, -1, 0,1.3f, 2,1.3f, -2 };
    static const float C[] = { 2,2.5f, 1.4f,3, 0.6f,3, 0,2.4f, 0,0.6f, 0.6f,0, 1.4f,0, 2,0.5f, -2 };
    static const float K[] = { 0,0, 0,3, -1, 2,3, 0,1.2f, -1, 0.75f,1.75f, 2,0, -2 };
    static const float P[] = { 0,0, 0,3, 1.5f,3, 2,2.5f, 2,2, 1.5f,1.5f, 0,1.5f, -2 };
    static const float O[] = { 0.6f,0, 1.4f,0, 2,0.6f, 2,2.4f, 1.4f,3, 0.6f,3, 0,2.4f, 0,0.6f, 0.6f,0, -2 };
    static const float T[] = { 0,3, 2,3, -1, 1,3, 1,0, -2 };
    static const float R[] = { 0,0, 0,3, 1.5f,3, 2,2.5f, 2,2, 1.5f,1.5f, 0,1.5f, -1, 1,1.5f, 2,0, -2 };
    static const float D[] = { 0,0, 0,3, 1.2f,3, 2,2.2f, 2,0.8f, 1.2f,0, 0,0, -2 };
    static const float E[] = { 2,3, 0,3, 0,0, 2,0, -1, 0,1.5f, 1.4f,1.5f, -2 };
    static const float SIETE[] = { 0,3, 2,3, 0.7f,0, -2 };
    static const float M[] = { 0,0, 0,3, 1,1.6f, 2,3, 2,0, -2 };
    static const float I[] = { 1,0, 1,3, -2 };
    static const float S[] = { 2,2.5f, 1.4f,3, 0.6f,3, 0,2.4f, 0,2, 0.5f,1.5f, 1.5f,1.5f, 2,1, 2,0.6f, 1.4f,0, 0.6f,0, 0,0.5f, -2 };

    switch (c)
    {
        case 'J': return J;
        case 'A': return A;
        case 'C': return C;
        case 'K': return K;
        case 'P': return P;
        case 'O': return O;
        case 'T': return T;
        case 'R': return R;
        case 'D': return D;
        case 'E': return E;
        case '7': return SIETE;
        case 'M': return M;
        case 'I': return I;
        case 'S': return S;
        default: return nullptr;
    }
}


static float AnchoTextoNeonR(const char* texto, float escala)
{
    int n = (int)std::strlen(texto);
    return n > 0 ? ((float)n * 2.0f + (float)(n - 1) * 0.8f) * escala : 0.0f;
}


// Llama a trazo(ax, ay, bx, by, letra) por cada tramo del texto.
template <typename F>
static void RecorrerTextoNeonR(const char* texto, float centroX, float y0, float escala, F trazo)
{
    float x = centroX - AnchoTextoNeonR(texto, escala) * 0.5f;

    for (int i = 0; texto[i] != '\0'; i++)
    {
        const float* p = TrazosLetraR(texto[i]);

        if (p != nullptr)
        {
            int k = 0;
            bool pluma = false;
            float px = 0.0f, py = 0.0f;

            while (p[k] != -2.0f)
            {
                if (p[k] == -1.0f)
                {
                    pluma = false;
                    k++;
                    continue;
                }

                float qx = x + p[k] * escala;
                float qy = y0 + p[k + 1] * escala;

                if (pluma)
                {
                    trazo(px, py, qx, qy, i);
                }

                px = qx;
                py = qy;
                pluma = true;
                k += 2;
            }
        }

        x += 2.8f * escala;
    }
}


// Cada letrero tiene una letra cansada que a ratos titila.
static float TitileoR(float t, int semilla, int letra)
{
    int cansada = (int)(HashR(semilla, 7) * 5.0f);

    if (letra != cansada)
    {
        return 1.0f;
    }

    float ciclo = std::fmod(t + (float)semilla * 1.7f, 5.5f);

    if (ciclo > 0.9f)
    {
        return 1.0f;
    }

    return HashR((int)(t * 18.0f), semilla) > 0.45f ? 1.0f : 0.16f;
}


static void TextoNeonR(const char* texto, float cx, float y0, float z, float escala, float grosor, Color color, float t, int semilla)
{
    float emis = g_emisR;
    g_emisR = 1.0f;
    Vector3 n = { 0.0f, 0.0f, 1.0f };

    RecorrerTextoNeonR(texto, cx, y0, escala, [&](float ax, float ay, float bx, float by, int letra)
    {
        float brillo = TitileoR(t, semilla, letra);
        Color tubo = TonoR(color, 0.35f + 0.65f * brillo);
        Color nucleo = MezclaR(tubo, WHITE, 0.55f * brillo);
        TrazoR({ ax, ay, z }, { bx, by, z }, n, grosor, tubo);
        TrazoR({ ax, ay, z + 0.02f }, { bx, by, z + 0.02f }, n, grosor * 0.42f, nucleo);
    });

    g_emisR = emis;
}


static void GlowTextoNeonR(const char* texto, float cx, float y0, float z, float escala, float ancho, Color color, float t, int semilla)
{
    Vector3 n = { 0.0f, 0.0f, 1.0f };

    RecorrerTextoNeonR(texto, cx, y0, escala, [&](float ax, float ay, float bx, float by, int letra)
    {
        float brillo = TitileoR(t, semilla, letra);
        GlowTrazoR({ ax, ay, z + 0.04f }, { bx, by, z + 0.04f }, n, ancho, AlfaR(color, 0.30f * brillo));
    });
}


//--------------------------------------------------
// Capa 1: alfombra estampada y alfombras de cada maquina
//--------------------------------------------------

static const Color MOTIVOS_ALFOMBRA_R[4] =
{
    { 200, 64, 160, 255 },
    { 46, 172, 184, 255 },
    { 218, 172, 74, 255 },
    { 132, 98, 206, 255 }
};


// Alfombra de luz negra: base oscura con triangulos, zigzags, aros y
// puntos de colores sembrados con la semilla visual.
static void AlfombraR(float x0, float x1, float z0, float z1, float paso, bool motivos, float brillo, int semilla)
{
    Vector3 arriba = { 0.0f, 1.0f, 0.0f };
    int nx = (int)std::ceil((x1 - x0) / paso);
    int nz = (int)std::ceil((z1 - z0) / paso);

    for (int ix = 0; ix < nx; ix++)
    {
        for (int iz = 0; iz < nz; iz++)
        {
            float xa = x0 + (float)ix * paso, xb = std::fmin(xa + paso, x1);
            float za = z0 + (float)iz * paso, zb = std::fmin(za + paso, z1);
            int hx = ix + semilla * 131, hz = iz + semilla * 71;
            Color base = TonoR(COLOR_ALFOMBRA_R, brillo * (0.95f + 0.07f * HashR(hx, hz)));
            QuadR({ xa, 0.0f, za }, { xa, 0.0f, zb }, { xb, 0.0f, zb }, { xb, 0.0f, za }, base);

            if (!motivos)
            {
                continue;
            }

            for (int m = 0; m < 2; m++)
            {
                float tipo = HashR(hx * 3 + m, hz * 5 + 11);

                if (tipo > 0.80f)
                {
                    continue;
                }

                float cx = xa + paso * (0.2f + 0.6f * HashR(hx + m * 17, hz + 31));
                float cz = za + paso * (0.2f + 0.6f * HashR(hx + 53, hz + m * 29));
                Color color = MezclaR(TonoR(MOTIVOS_ALFOMBRA_R[(int)(HashR(hx + m, hz * 7) * 3.99f)], brillo * 0.72f), base, 0.22f);
                float giro = HashR(hx * 11, hz * 13 + m) * 2.0f * PI;
                float s = paso * (0.08f + 0.06f * HashR(hx * 19 + m, hz));

                if (tipo < 0.24f)
                {
                    Vector3 p[3];
                    for (int k = 0; k < 3; k++)
                    {
                        float a = giro + 2.0944f * (float)k;
                        p[k] = { cx + std::cos(a) * s * 1.7f, 0.0f, cz + std::sin(a) * s * 1.7f };
                    }
                    TriR(p[0], p[1], p[2], color);
                }
                else if (tipo < 0.46f)
                {
                    float dx = std::cos(giro) * s * 0.9f, dz = std::sin(giro) * s * 0.9f;
                    float nx2 = -dz * 0.8f, nz2 = dx * 0.8f;
                    Vector3 a = { cx - 1.5f * dx, 0.0f, cz - 1.5f * dz };
                    Vector3 b = { cx - 0.5f * dx + nx2, 0.0f, cz - 0.5f * dz + nz2 };
                    Vector3 c = { cx + 0.5f * dx - nx2, 0.0f, cz + 0.5f * dz - nz2 };
                    Vector3 d = { cx + 1.5f * dx, 0.0f, cz + 1.5f * dz };
                    TrazoR(a, b, arriba, s * 0.35f, color);
                    TrazoR(b, c, arriba, s * 0.35f, color);
                    TrazoR(c, d, arriba, s * 0.35f, color);
                }
                else if (tipo < 0.60f)
                {
                    for (int k = 0; k < 8; k++)
                    {
                        float a0 = 0.7854f * (float)k, a1 = 0.7854f * (float)(k + 1);
                        TrazoR(
                            { cx + std::cos(a0) * s, 0.0f, cz + std::sin(a0) * s },
                            { cx + std::cos(a1) * s, 0.0f, cz + std::sin(a1) * s },
                            arriba, s * 0.3f, color
                        );
                    }
                }
                else
                {
                    float r = s * 0.55f;
                    QuadR({ cx - r, 0.0f, cz }, { cx, 0.0f, cz + r }, { cx + r, 0.0f, cz }, { cx, 0.0f, cz - r }, color);
                }
            }
        }
    }
}


// Alfombra roja de la maquina, con ribete dorado y rombo central.
static void AlfombraMaquinaR(float X)
{
    Vector3 arriba = { 0.0f, 1.0f, 0.0f };
    Color terciopelo = { 108, 26, 52, 255 };
    float x0 = X - 1.55f, x1 = X + 1.55f, z0 = 2.35f, z1 = 5.5f;

    QuadR({ x0, 0.0f, z0 }, { x0, 0.0f, z1 }, { x1, 0.0f, z1 }, { x1, 0.0f, z0 }, terciopelo);
    TrazoR({ x0 + 0.12f, 0.0f, z0 + 0.12f }, { x1 - 0.12f, 0.0f, z0 + 0.12f }, arriba, 0.07f, COLOR_ORO_R);
    TrazoR({ x1 - 0.12f, 0.0f, z0 + 0.12f }, { x1 - 0.12f, 0.0f, z1 - 0.12f }, arriba, 0.07f, COLOR_ORO_R);
    TrazoR({ x1 - 0.12f, 0.0f, z1 - 0.12f }, { x0 + 0.12f, 0.0f, z1 - 0.12f }, arriba, 0.07f, COLOR_ORO_R);
    TrazoR({ x0 + 0.12f, 0.0f, z1 - 0.12f }, { x0 + 0.12f, 0.0f, z0 + 0.12f }, arriba, 0.07f, COLOR_ORO_R);

    float cz = 0.5f * (z0 + z1);
    Color rombo = TonoR(COLOR_ORO_R, 0.75f);
    TrazoR({ X, 0.0f, cz - 1.1f }, { X + 0.9f, 0.0f, cz }, arriba, 0.05f, rombo);
    TrazoR({ X + 0.9f, 0.0f, cz }, { X, 0.0f, cz + 1.1f }, arriba, 0.05f, rombo);
    TrazoR({ X, 0.0f, cz + 1.1f }, { X - 0.9f, 0.0f, cz }, arriba, 0.05f, rombo);
    TrazoR({ X - 0.9f, 0.0f, cz }, { X, 0.0f, cz - 1.1f }, arriba, 0.05f, rombo);

    // Flecos en los extremos.
    for (int k = 0; k < 14; k++)
    {
        float x = x0 + 0.1f + (x1 - x0 - 0.2f) * (float)k / 13.0f;
        TrazoR({ x, 0.0f, z1 }, { x, 0.0f, z1 + 0.14f }, arriba, 0.035f, COLOR_ORO_R);
    }
}


//--------------------------------------------------
// Utileria (horneada)
//--------------------------------------------------

static void TabureteR(float x, float z, Color asiento)
{
    CilindroR({ x, 0.0f, z }, { x, 0.05f, z }, 0.32f, 0.3f, 10, COLOR_CROMO_R);
    CilindroR({ x, 0.05f, z }, { x, 0.74f, z }, 0.045f, 0.045f, 6, COLOR_CROMO_R, false);
    CilindroR({ x, 0.36f, z }, { x, 0.4f, z }, 0.22f, 0.22f, 10, TonoR(COLOR_CROMO_R, 0.8f));
    CilindroR({ x, 0.74f, z }, { x, 0.88f, z }, 0.3f, 0.32f, 12, asiento);
}


static void PosteCordonR(float x, float z)
{
    CilindroR({ x, 0.0f, z }, { x, 0.05f, z }, 0.22f, 0.2f, 10, COLOR_ORO_R);
    CilindroR({ x, 0.05f, z }, { x, 1.0f, z }, 0.045f, 0.045f, 6, COLOR_ORO_R, false);
    BolaR({ x, 1.04f, z }, 0.08f, COLOR_ORO_R, 3, 6);
}


static void CordonR(Vector3 a, Vector3 b)
{
    Color terciopelo = { 168, 24, 48, 255 };
    Vector3 previo = a;

    for (int k = 1; k <= 6; k++)
    {
        float u = (float)k / 6.0f;
        Vector3 p = Vector3Lerp(a, b, u);
        p.y -= 0.32f * 4.0f * u * (1.0f - u);
        CilindroR(previo, p, 0.04f, 0.04f, 5, terciopelo, false);
        previo = p;
    }
}


static void PalmeraR(float x, float z, int semilla)
{
    Color maceta = { 232, 226, 236, 255 };
    Color tronco = { 120, 82, 60, 255 };
    Color hoja = { 52, 150, 98, 255 };

    CilindroR({ x, 0.0f, z }, { x, 0.75f, z }, 0.36f, 0.46f, 10, maceta);
    CilindroR({ x, 0.75f, z }, { x, 0.82f, z }, 0.48f, 0.48f, 10, COLOR_ORO_R);

    for (int k = 0; k < 5; k++)
    {
        float y0 = 0.82f + 0.42f * (float)k;
        CilindroR({ x, y0, z }, { x + 0.02f * (float)k, y0 + 0.42f, z }, 0.12f - 0.012f * (float)k, 0.11f - 0.012f * (float)k, 6, TonoR(tronco, 0.9f + 0.1f * (float)(k % 2)));
    }

    Vector3 copa = { x + 0.1f, 2.95f, z };

    for (int h = 0; h < 7; h++)
    {
        float a = 2.0f * PI * (float)h / 7.0f + HashR(semilla, h) * 0.5f;
        Vector3 dir = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 lado = { -dir.z, 0.0f, dir.x };
        Vector3 previo = copa;
        float ancho = 0.32f;

        for (int s = 1; s <= 3; s++)
        {
            float u = (float)s / 3.0f;
            Vector3 p = { copa.x + dir.x * 1.3f * u, copa.y + 0.35f * u - 0.9f * u * u, copa.z + dir.z * 1.3f * u };
            float anchoSiguiente = ancho * (1.0f - 0.3f * u);
            QuadR(
                Vector3Add(previo, Vector3Scale(lado, ancho)),
                Vector3Subtract(previo, Vector3Scale(lado, ancho)),
                Vector3Subtract(p, Vector3Scale(lado, anchoSiguiente)),
                Vector3Add(p, Vector3Scale(lado, anchoSiguiente)),
                TonoR(hoja, 0.85f + 0.2f * HashR(h, s + semilla))
            );
            previo = p;
            ancho = anchoSiguiente;
        }
    }
}


static void PapeleraR(float x, float z)
{
    CilindroR({ x, 0.0f, z }, { x, 0.8f, z }, 0.28f, 0.3f, 10, COLOR_CROMO_R);
    CilindroR({ x, 0.8f, z }, { x, 0.86f, z }, 0.31f, 0.31f, 10, TonoR(COLOR_CROMO_R, 0.7f));
    DiscoR({ x, 0.87f, z }, { 1, 0, 0 }, { 0, 0, 1 }, 0.24f, 10, COLOR_NEGRO_R);
}


// Peluche: osito (cuerpo, cabeza, orejas, hocico) o pelota.
static void PelucheR(Vector3 pie, float tam, int semilla)
{
    static const Color COLORES[6] =
    {
        { 240, 150, 190, 255 }, { 150, 210, 240, 255 }, { 250, 220, 120, 255 },
        { 170, 230, 160, 255 }, { 210, 170, 240, 255 }, { 240, 180, 140, 255 }
    };
    Color color = COLORES[(int)(HashR(semilla, 3) * 5.99f)];

    if (HashR(semilla, 9) < 0.3f)
    {
        BolaR({ pie.x, pie.y + tam, pie.z }, tam, color, 4, 8);
        return;
    }

    BolaR({ pie.x, pie.y + tam * 0.9f, pie.z }, tam, color, 3, 7);
    BolaR({ pie.x, pie.y + tam * 2.15f, pie.z + 0.02f }, tam * 0.72f, color, 3, 7);
    BolaR({ pie.x - tam * 0.5f, pie.y + tam * 2.7f, pie.z }, tam * 0.26f, TonoR(color, 0.85f), 2, 5);
    BolaR({ pie.x + tam * 0.5f, pie.y + tam * 2.7f, pie.z }, tam * 0.26f, TonoR(color, 0.85f), 2, 5);
    BolaR({ pie.x, pie.y + tam * 2.0f, pie.z + tam * 0.62f }, tam * 0.24f, Color{ 250, 236, 220, 255 }, 2, 5);
}


static void RegaloR(Vector3 pie, float tam, int semilla)
{
    static const Color COLORES[4] = { { 80, 120, 230, 255 }, { 230, 70, 90, 255 }, { 70, 190, 140, 255 }, { 240, 160, 60, 255 } };
    Color caja = COLORES[(int)(HashR(semilla, 5) * 3.99f)];
    CajaR({ pie.x, pie.y + tam * 0.5f, pie.z }, { tam, tam, tam }, caja);
    CajaR({ pie.x, pie.y + tam * 0.5f, pie.z }, { tam * 0.18f, tam * 1.02f, tam * 1.02f }, COLOR_AMARILLO_R);
    CajaR({ pie.x, pie.y + tam * 0.5f, pie.z }, { tam * 1.02f, tam * 1.02f, tam * 0.18f }, COLOR_AMARILLO_R);
}


// Cabina de premios: mostrador vidriado, estanteria con peluches y regalos.
static void CabinaPremiosR(float cx, float cz, int paso)
{
    Color madera = TonoR(COLOR_LACA_R, 0.8f);

    // Estanteria.
    CajaR({ cx, 2.7f, cz - 0.95f }, { 4.7f, 5.4f, 0.4f }, COLOR_PARED_OSCURA_R);
    CajaR({ cx - 2.3f, 2.7f, cz - 0.55f }, { 0.14f, 5.4f, 0.6f }, madera);
    CajaR({ cx + 2.3f, 2.7f, cz - 0.55f }, { 0.14f, 5.4f, 0.6f }, madera);
    CajaR({ cx, 5.5f, cz - 0.7f }, { 4.9f, 0.2f, 0.7f }, COLOR_CROMO_R);

    for (int s = 0; s < 4; s++)
    {
        float y = 1.3f + 1.05f * (float)s;
        CajaR({ cx, y, cz - 0.5f }, { 4.5f, 0.08f, 0.6f }, COLOR_ORO_R);

        for (int j = 0; j < 6; j += (s == 3 ? paso : 1))
        {
            int semilla = s * 17 + j * 5 + 3;
            Vector3 pie = { cx - 1.85f + 0.74f * (float)j + 0.1f * (HashR(semilla, 1) - 0.5f), y + 0.04f, cz - 0.45f };

            if (HashR(semilla, 2) < 0.25f)
            {
                RegaloR(pie, 0.34f, semilla);
            }
            else
            {
                PelucheR(pie, 0.15f + 0.04f * HashR(semilla, 4), semilla);
            }
        }
    }

    // Mostrador.
    CajaR({ cx, 0.55f, cz + 0.95f }, { 4.5f, 1.1f, 0.9f }, COLOR_LACA_R);
    CajaR({ cx, 0.3f, cz + 1.41f }, { 4.5f, 0.06f, 0.04f }, COLOR_ORO_R);
    CajaR({ cx, 0.8f, cz + 1.41f }, { 4.5f, 0.06f, 0.04f }, COLOR_ORO_R);
    CajaR({ cx, 1.32f, cz + 0.95f }, { 4.5f, 0.42f, 0.9f }, Color{ 120, 170, 200, 255 });
    CajaR({ cx, 1.55f, cz + 0.95f }, { 4.56f, 0.05f, 0.96f }, COLOR_CROMO_R);

    for (int j = 0; j < 5; j++)
    {
        PelucheR({ cx - 1.7f + 0.85f * (float)j, 1.12f, cz + 0.95f }, 0.07f, 40 + j);
    }

    // Expendedor de tickets.
    CajaR({ cx + 1.9f, 1.85f, cz + 1.0f }, { 0.4f, 0.55f, 0.35f }, COLOR_CROMO_R);
    PanelR(cx + 1.78f, 1.75f, cx + 2.02f, 1.95f, cz + 1.18f, Color{ 255, 90, 90, 255 });

    // Osito gigante al costado.
    float ox = cx + (cx < 0.0f ? 2.9f : -2.9f);
    BolaR({ ox, 0.55f, cz + 1.0f }, 0.55f, Color{ 236, 150, 186, 255 }, 5, 10);
    BolaR({ ox, 1.38f, cz + 1.05f }, 0.4f, Color{ 236, 150, 186, 255 }, 5, 10);
    BolaR({ ox - 0.3f, 1.72f, cz + 1.0f }, 0.13f, Color{ 214, 120, 160, 255 }, 3, 6);
    BolaR({ ox + 0.3f, 1.72f, cz + 1.0f }, 0.13f, Color{ 214, 120, 160, 255 }, 3, 6);
    BolaR({ ox, 1.32f, cz + 1.42f }, 0.13f, Color{ 252, 236, 226, 255 }, 3, 6);
    CajaR({ ox, 1.04f, cz + 1.3f }, { 0.5f, 0.1f, 0.12f }, COLOR_CIAN_R);
}


// Grua de peluches: base, postes cromados, techo luminoso y monte de peluches.
static void GruaPeluchesR(float cx, float cz)
{
    Color rosa = { 206, 64, 150, 255 };
    CajaR({ cx, 0.75f, cz }, { 1.9f, 1.5f, 1.9f }, rosa);
    CajaR({ cx, 1.52f, cz }, { 1.96f, 0.06f, 1.96f }, COLOR_CROMO_R);
    CajaR({ cx + 0.5f, 1.05f, cz + 0.96f }, { 0.55f, 0.45f, 0.04f }, COLOR_NEGRO_R);
    CajaR({ cx - 0.45f, 1.2f, cz + 0.98f }, { 0.5f, 0.12f, 0.12f }, COLOR_CROMO_R);
    CilindroR({ cx - 0.45f, 1.26f, cz + 0.98f }, { cx - 0.45f, 1.48f, cz + 0.98f }, 0.025f, 0.025f, 5, COLOR_NEGRO_R, false);
    BolaR({ cx - 0.45f, 1.5f, cz + 0.98f }, 0.06f, Color{ 230, 40, 60, 255 }, 3, 6);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            CajaR({ cx + 0.9f * (float)sx, 2.7f, cz + 0.9f * (float)sz }, { 0.08f, 2.36f, 0.08f }, COLOR_CROMO_R);
        }
    }

    CajaR({ cx, 4.2f, cz }, { 1.96f, 0.7f, 1.96f }, TonoR(rosa, 0.7f));
    PanelR(cx - 0.85f, 3.95f, cx + 0.85f, 4.45f, cz + 0.985f, Color{ 255, 220, 110, 255 });
    CajaR({ cx, 1.58f, cz }, { 1.8f, 0.06f, 1.8f }, COLOR_NEGRO_R);

    for (int k = 0; k < 16; k++)
    {
        float px = cx - 0.65f + 1.3f * HashR(k, 21);
        float pz = cz - 0.65f + 1.3f * HashR(k, 22);
        float alto = 1.62f + 0.18f * (1.0f - std::fabs(px - cx) - std::fabs(pz - cz)) * HashR(k, 23);
        static const Color C[4] = { { 240, 150, 190, 255 }, { 150, 210, 240, 255 }, { 250, 220, 120, 255 }, { 170, 230, 160, 255 } };
        BolaR({ px, alto + 0.12f, pz }, 0.15f, C[k % 4], 3, 6);
    }
}


static void RocolaR(float cx, float cz)
{
    Color madera = { 128, 64, 34, 255 };
    CajaR({ cx, 1.3f, cz }, { 1.6f, 2.6f, 1.0f }, madera);
    CilindroR({ cx, 2.6f, cz - 0.5f }, { cx, 2.6f, cz + 0.5f }, 0.8f, 0.8f, 14, TonoR(madera, 0.9f));
    CajaR({ cx, 0.8f, cz + 0.51f }, { 1.1f, 0.8f, 0.03f }, COLOR_NEGRO_R);

    for (int k = 0; k < 6; k++)
    {
        CajaR({ cx, 0.47f + 0.13f * (float)k, cz + 0.53f }, { 1.1f, 0.03f, 0.03f }, COLOR_CROMO_R);
    }

    PanelR(cx - 0.55f, 1.45f, cx + 0.55f, 2.1f, cz + 0.52f, Color{ 250, 232, 190, 255 });
    CajaR({ cx, 1.32f, cz + 0.6f }, { 1.3f, 0.12f, 0.24f }, COLOR_CROMO_R);
}


static void PinballR(float cx, float cz)
{
    Color cuerpo = { 34, 44, 120, 255 };

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            CilindroR({ cx + 0.42f * (float)sx, 0.0f, cz + 0.2f + 1.05f * (float)sz }, { cx + 0.42f * (float)sx, 0.95f, cz + 0.2f + 1.05f * (float)sz }, 0.05f, 0.05f, 6, COLOR_CROMO_R, false);
        }
    }

    CajaR({ cx, 1.1f, cz + 0.2f }, { 1.0f, 0.34f, 2.4f }, cuerpo);
    float emis = g_emisR;
    g_emisR = 1.0f;
    QuadR({ cx - 0.44f, 1.28f, cz - 0.95f }, { cx - 0.44f, 1.28f, cz + 1.35f }, { cx + 0.44f, 1.28f, cz + 1.35f }, { cx + 0.44f, 1.28f, cz - 0.95f }, Color{ 40, 26, 96, 255 });
    DiscoR({ cx - 0.18f, 1.29f, cz - 0.3f }, { 1, 0, 0 }, { 0, 0, 1 }, 0.12f, 10, COLOR_ROSA_R);
    DiscoR({ cx + 0.2f, 1.29f, cz - 0.15f }, { 1, 0, 0 }, { 0, 0, 1 }, 0.12f, 10, COLOR_CIAN_R);
    DiscoR({ cx, 1.29f, cz + 0.2f }, { 1, 0, 0 }, { 0, 0, 1 }, 0.12f, 10, COLOR_AMARILLO_R);
    g_emisR = emis;
    CajaR({ cx - 0.15f, 1.3f, cz + 1.0f }, { 0.28f, 0.03f, 0.06f }, COLOR_CROMO_R);
    CajaR({ cx + 0.15f, 1.3f, cz + 1.0f }, { 0.28f, 0.03f, 0.06f }, COLOR_CROMO_R);
    CajaR({ cx, 2.05f, cz - 0.95f }, { 1.0f, 1.4f, 0.3f }, cuerpo);
    CajaR({ cx, 2.78f, cz - 0.95f }, { 1.04f, 0.06f, 0.34f }, COLOR_CROMO_R);
}


// Maquina vertical de otro juego (la pantalla se anima aparte).
static void MaquinaVideoR(float x, float z, int variante)
{
    static const Color CUERPOS[4] = { { 40, 52, 128, 255 }, { 30, 24, 40, 255 }, { 132, 32, 44, 255 }, { 24, 96, 106, 255 } };
    static const Color ACENTOS[4] = { COLOR_AMARILLO_R, COLOR_ROSA_R, COLOR_CIAN_R, COLOR_VIOLETA_R };
    Color cuerpo = CUERPOS[variante % 4];
    Color acento = ACENTOS[(variante + 1) % 4];

    CajaR({ x, 1.95f, z }, { 1.6f, 3.9f, 1.3f }, cuerpo);
    CajaR({ x - 0.81f, 2.1f, z }, { 0.02f, 2.6f, 1.0f }, TonoR(acento, 0.7f));
    CajaR({ x + 0.81f, 2.1f, z }, { 0.02f, 2.6f, 1.0f }, TonoR(acento, 0.7f));
    CajaR({ x, 2.75f, z + 0.62f }, { 1.36f, 1.16f, 0.1f }, COLOR_NEGRO_R);
    CajaR({ x, 1.75f, z + 0.85f }, { 1.5f, 0.22f, 0.6f }, TonoR(cuerpo, 0.8f));
    CilindroR({ x - 0.38f, 1.86f, z + 0.85f }, { x - 0.38f, 2.04f, z + 0.85f }, 0.03f, 0.03f, 5, COLOR_NEGRO_R, false);
    BolaR({ x - 0.38f, 2.08f, z + 0.85f }, 0.07f, Color{ 230, 50, 60, 255 }, 3, 6);

    for (int b = 0; b < 3; b++)
    {
        CajaR({ x + 0.08f + 0.22f * (float)b, 1.88f, z + 0.85f }, { 0.13f, 0.05f, 0.13f }, ACENTOS[(variante + b) % 4]);
    }

    CajaR({ x, 0.9f, z + 0.66f }, { 0.5f, 0.5f, 0.03f }, TonoR(COLOR_CROMO_R, 0.75f));
    PanelR(x - 0.15f, 0.95f, x - 0.07f, 1.05f, z + 0.68f, Color{ 255, 80, 60, 255 });
    PanelR(x + 0.07f, 0.95f, x + 0.15f, 1.05f, z + 0.68f, Color{ 255, 80, 60, 255 });
    CajaR({ x, 4.25f, z + 0.05f }, { 1.6f, 0.7f, 1.2f }, COLOR_NEGRO_R);
    PanelR(x - 0.72f, 4.0f, x + 0.72f, 4.5f, z + 0.66f, MezclaR(acento, WHITE, 0.25f));
    PanelR(x - 0.5f, 4.18f, x + 0.5f, 4.32f, z + 0.67f, TonoR(cuerpo, 0.9f));
}


// Posiciones de la fila de maquinas de otros juegos contra la pared.
static int PosicionesFilaFondoR(float xs[], int maximo)
{
    int n = 0;

    for (float x = -27.0f; x <= 27.01f && n < maximo; x += 2.0f)
    {
        float ax = std::fabs(x);
        bool libre = !(ax > ARCO_X0_R - 0.9f && ax < ARCO_X1_R + 0.9f) && !(ax > 8.4f && ax < 10.6f);

        for (float p : PILASTRAS_R)
        {
            if (std::fabs(x - p) < 1.45f) libre = false;
        }

        if (libre)
        {
            xs[n++] = x;
        }
    }

    return n;
}


static const float Z_FILA_FONDO_R = -9.6f;


//--------------------------------------------------
// Capas 3 a 5: pared con arcos, segundo salon, techo
//--------------------------------------------------

static void ParedesSalaR(bool arcos)
{
    const float z = Z_PARED_R;
    const float h = ALTO_TECHO_R;

    // Muro (con aberturas para los arcos si hay decoracion).
    if (arcos)
    {
        float tramos[3][2] = { { -40.0f, -ARCO_X1_R }, { -ARCO_X0_R, ARCO_X0_R }, { ARCO_X1_R, 40.0f } };
        for (auto& t : tramos)
        {
            CajaR({ 0.5f * (t[0] + t[1]), h * 0.5f, z - 0.3f }, { t[1] - t[0], h, 0.6f }, COLOR_PARED_R);
        }
        for (int s = -1; s <= 1; s += 2)
        {
            float cx = (float)s * 0.5f * (ARCO_X0_R + ARCO_X1_R);
            CajaR({ cx, 0.5f * (ARCO_ALTO_R + h), z - 0.3f }, { ARCO_X1_R - ARCO_X0_R, h - ARCO_ALTO_R, 0.6f }, COLOR_PARED_R);
            // Esquinas del arco escalonadas.
            for (int e = 0; e < 3; e++)
            {
                float lado = 0.9f - 0.3f * (float)e;
                float y = ARCO_ALTO_R - 0.3f * (float)(e + 1) + 0.15f;
                CajaR({ (float)s * (ARCO_X0_R + lado * 0.5f), y, z - 0.3f }, { lado, 0.3f, 0.6f }, COLOR_PARED_R);
                CajaR({ (float)s * (ARCO_X1_R - lado * 0.5f), y, z - 0.3f }, { lado, 0.3f, 0.6f }, COLOR_PARED_R);
            }
            // Jambas y dintel cromados.
            CajaR({ (float)s * ARCO_X0_R, ARCO_ALTO_R * 0.5f, z + 0.05f }, { 0.3f, ARCO_ALTO_R, 0.2f }, TonoR(COLOR_CROMO_R, 0.8f));
            CajaR({ (float)s * ARCO_X1_R, ARCO_ALTO_R * 0.5f, z + 0.05f }, { 0.3f, ARCO_ALTO_R, 0.2f }, TonoR(COLOR_CROMO_R, 0.8f));
            CajaR({ cx, ARCO_ALTO_R + 0.1f, z + 0.05f }, { ARCO_X1_R - ARCO_X0_R + 0.3f, 0.2f, 0.2f }, TonoR(COLOR_CROMO_R, 0.8f));
        }
    }
    else
    {
        CajaR({ 0.0f, h * 0.5f, z - 0.3f }, { 80.0f, h, 0.6f }, COLOR_PARED_R);
    }

    // Empapelado a rayas, zocalo, baranda cromada y friso dorado.
    for (int k = 0; k < 40; k++)
    {
        float x0 = -40.0f + 2.0f * (float)k, x1 = x0 + 1.0f;
        float centro = std::fabs(x0 + 0.5f);
        bool enArco = arcos && centro > ARCO_X0_R - 0.2f && centro < ARCO_X1_R + 0.2f;

        if (!enArco)
        {
            QuadR({ x0, 1.4f, z + 0.03f }, { x1, 1.4f, z + 0.03f }, { x1, 7.2f, z + 0.03f }, { x0, 7.2f, z + 0.03f }, TonoR(COLOR_PARED_R, 0.82f));
        }
    }

    float tramosZocalo[3][2] = { { -40.0f, -ARCO_X1_R }, { -ARCO_X0_R, ARCO_X0_R }, { ARCO_X1_R, 40.0f } };
    int cantidadTramos = arcos ? 3 : 1;

    for (int t = 0; t < cantidadTramos; t++)
    {
        float x0 = arcos ? tramosZocalo[t][0] : -40.0f;
        float x1 = arcos ? tramosZocalo[t][1] : 40.0f;
        CajaR({ 0.5f * (x0 + x1), 0.65f, z + 0.06f }, { x1 - x0, 1.3f, 0.12f }, COLOR_ZOCALO_R);
        CajaR({ 0.5f * (x0 + x1), 1.34f, z + 0.1f }, { x1 - x0, 0.07f, 0.2f }, COLOR_CROMO_R);
    }

    CajaR({ 0.0f, 7.32f, z + 0.06f }, { 80.0f, 0.12f, 0.12f }, COLOR_ORO_R);
    QuadR({ -40.0f, 7.4f, z + 0.02f }, { 40.0f, 7.4f, z + 0.02f }, { 40.0f, h, z + 0.02f }, { -40.0f, h, z + 0.02f }, COLOR_PARED_OSCURA_R);
    CajaR({ 0.0f, h - 0.2f, z + 0.12f }, { 80.0f, 0.4f, 0.24f }, TonoR(COLOR_PARED_OSCURA_R, 0.8f));

    // Pilastras con remate cromado (los LEDs se animan aparte).
    for (float px : PILASTRAS_R)
    {
        CajaR({ px, h * 0.5f, z + 0.3f }, { 1.1f, h, 0.6f }, COLOR_PARED_OSCURA_R);
        CajaR({ px, 0.2f, z + 0.32f }, { 1.24f, 0.4f, 0.66f }, COLOR_CROMO_R);
        CajaR({ px, h - 0.6f, z + 0.32f }, { 1.24f, 0.3f, 0.66f }, COLOR_CROMO_R);
        CajaR({ px, 0.7f, z + 0.62f }, { 0.9f, 0.04f, 0.02f }, COLOR_ORO_R);
    }

    // Tablero del letrero JACKPOT.
    CajaR({ 0.0f, 9.65f, z + 0.08f }, { 13.0f, 2.6f, 0.16f }, COLOR_NEGRO_R);
    CajaR({ 0.0f, 8.32f, z + 0.12f }, { 13.1f, 0.08f, 0.2f }, COLOR_CROMO_R);
    CajaR({ 0.0f, 10.98f, z + 0.12f }, { 13.1f, 0.08f, 0.2f }, COLOR_CROMO_R);
}


// Afiche de juego retro: sol de bandas sobre una grilla en perspectiva.
static void AficheR(float cx, float cy, float z)
{
    CajaR({ cx, cy, z + 0.04f }, { 2.4f, 3.2f, 0.08f }, COLOR_CROMO_R);
    float emis = g_emisR;
    g_emisR = 1.0f;
    QuadR({ cx - 1.08f, cy - 1.48f, z + 0.09f }, { cx + 1.08f, cy - 1.48f, z + 0.09f }, { cx + 1.08f, cy + 1.48f, z + 0.09f }, { cx - 1.08f, cy + 1.48f, z + 0.09f }, Color{ 40, 20, 80, 255 });
    DiscoR({ cx, cy + 0.35f, z + 0.1f }, { 1, 0, 0 }, { 0, 1, 0 }, 0.75f, 18, Color{ 255, 120, 90, 255 });
    for (int b = 0; b < 4; b++)
    {
        float y = cy + 0.05f + 0.16f * (float)b;
        QuadR({ cx - 0.8f, y, z + 0.11f }, { cx + 0.8f, y, z + 0.11f }, { cx + 0.8f, y + 0.05f + 0.01f * (float)b, z + 0.11f }, { cx - 0.8f, y + 0.05f + 0.01f * (float)b, z + 0.11f }, Color{ 40, 20, 80, 255 });
    }
    QuadR({ cx - 1.08f, cy - 1.48f, z + 0.11f }, { cx + 1.08f, cy - 1.48f, z + 0.11f }, { cx + 1.08f, cy - 0.2f, z + 0.11f }, { cx - 1.08f, cy - 0.2f, z + 0.11f }, Color{ 26, 12, 50, 255 });
    for (int g = 0; g < 7; g++)
    {
        float xb = cx - 1.08f + 2.16f * (float)g / 6.0f;
        TrazoR({ cx + (xb - cx) * 0.25f, cy - 0.2f, z + 0.12f }, { xb, cy - 1.48f, z + 0.12f }, { 0, 0, 1 }, 0.03f, COLOR_ROSA_R);
    }
    for (int g = 0; g < 4; g++)
    {
        float y = cy - 0.3f - 0.32f * (float)(g * g) / 3.0f;
        TrazoR({ cx - 1.08f, y, z + 0.12f }, { cx + 1.08f, y, z + 0.12f }, { 0, 0, 1 }, 0.03f, COLOR_ROSA_R);
    }
    g_emisR = emis;
}


static void SegundoSalonR(int paso)
{
    const float zFondo = -36.0f;

    AlfombraR(-24.0f, -8.0f, -36.0f, -11.0f, 2.0f, true, 0.7f, 5);
    AlfombraR(8.0f, 24.0f, -36.0f, -11.0f, 2.0f, true, 0.7f, 6);

    for (int s = -1; s <= 1; s += 2)
    {
        float cx = (float)s * 16.0f;
        CajaR({ cx, 4.6f, zFondo - 0.3f }, { 18.0f, 9.2f, 0.6f }, COLOR_PARED_OSCURA_R);
        PanelR(cx - 8.0f, 6.1f, cx + 8.0f, 6.3f, zFondo + 0.02f, Color{ 255, 60, 180, 255 });
        PanelR(cx - 8.0f, 6.6f, cx + 8.0f, 6.7f, zFondo + 0.02f, Color{ 60, 200, 255, 255 });
        CajaR({ (float)s * 8.0f, 4.6f, -23.5f }, { 0.4f, 9.2f, 25.0f }, COLOR_PARED_OSCURA_R);
        CajaR({ (float)s * 24.0f, 4.6f, -23.5f }, { 0.4f, 9.2f, 25.0f }, COLOR_PARED_OSCURA_R);
        QuadR({ cx - 8.2f, 9.2f, -11.6f }, { cx + 8.2f, 9.2f, -11.6f }, { cx + 8.2f, 9.2f, zFondo }, { cx - 8.2f, 9.2f, zFondo }, COLOR_TECHO_R);

        // Filas de maquinas en dos profundidades.
        int filas = paso == 1 ? 2 : 1;
        for (int f = 0; f < filas; f++)
        {
            float z = f == 0 ? -18.5f : -27.5f;
            for (int j = 0; j < 8; j++)
            {
                float x = cx - 6.6f + 1.9f * (float)j;
                int v = j + f * 3 + (s > 0 ? 5 : 0);
                static const Color CUERPOS[3] = { { 40, 52, 128, 255 }, { 132, 32, 44, 255 }, { 24, 96, 106, 255 } };
                static const Color PANTALLAS[3] = { { 90, 200, 255, 255 }, { 255, 110, 200, 255 }, { 160, 255, 140, 255 } };
                CajaR({ x, 1.95f, z }, { 1.5f, 3.9f, 1.2f }, CUERPOS[v % 3]);
                PanelR(x - 0.55f, 2.3f, x + 0.55f, 3.2f, z + 0.61f, PANTALLAS[(v + 1) % 3]);
                PanelR(x - 0.65f, 3.95f, x + 0.65f, 4.4f, z + 0.61f, PANTALLAS[v % 3]);
            }
        }
    }
}


static void TechoSalaR(int n, bool decoracion)
{
    const float h = ALTO_TECHO_R;
    float mitad = MitadFilaR(n);

    QuadR({ -40.0f, h, Z_PARED_R }, { 40.0f, h, Z_PARED_R }, { 40.0f, h, 26.0f }, { -40.0f, h, 26.0f }, COLOR_TECHO_R);

    if (decoracion)
    {
        for (int k = 0; k < 8; k++)
        {
            float z = Z_PARED_R + 2.0f + 4.0f * (float)k;
            CajaR({ 0.0f, h - 0.15f, z }, { 80.0f, 0.3f, 0.3f }, TonoR(COLOR_TECHO_R, 1.3f));
        }

        for (int i = -4; i <= 4; i++)
        {
            for (int k = 0; k < 7; k++)
            {
                float x = (float)i * 6.0f;
                float z = Z_PARED_R + 4.0f + 4.0f * (float)k;
                float emis = g_emisR;
                g_emisR = 1.0f;
                QuadR({ x - 0.6f, h - 0.02f, z - 0.6f }, { x + 0.6f, h - 0.02f, z - 0.6f }, { x + 0.6f, h - 0.02f, z + 0.6f }, { x - 0.6f, h - 0.02f, z + 0.6f }, Color{ 150, 120, 170, 255 });
                g_emisR = emis;
            }
        }
    }

    // Puente de focos sobre la fila de maquinas.
    float x0 = -mitad - 2.5f, x1 = mitad + 2.5f;
    CajaR({ 0.0f, 10.75f, 4.55f }, { x1 - x0, 0.1f, 0.1f }, TonoR(COLOR_CROMO_R, 0.6f));
    CajaR({ 0.0f, 10.75f, 5.05f }, { x1 - x0, 0.1f, 0.1f }, TonoR(COLOR_CROMO_R, 0.6f));
    CajaR({ 0.0f, 10.4f, 4.8f }, { x1 - x0, 0.1f, 0.1f }, TonoR(COLOR_CROMO_R, 0.6f));

    for (float x = x0; x <= x1 + 0.01f; x += 1.0f)
    {
        CilindroR({ x, 10.75f, 4.55f }, { x + 0.5f, 10.4f, 4.8f }, 0.025f, 0.025f, 4, TonoR(COLOR_CROMO_R, 0.6f), false);
        CilindroR({ x, 10.75f, 5.05f }, { x + 0.5f, 10.4f, 4.8f }, 0.025f, 0.025f, 4, TonoR(COLOR_CROMO_R, 0.6f), false);
    }

    CilindroR({ x0, 10.75f, 4.8f }, { x0, h, 4.8f }, 0.03f, 0.03f, 4, COLOR_NEGRO_R, false);
    CilindroR({ x1, 10.75f, 4.8f }, { x1, h, 4.8f }, 0.03f, 0.03f, 4, COLOR_NEGRO_R, false);

    for (int k = 0; k < n; k++)
    {
        float X = XMaquinaR(k, n);
        CilindroR({ X, 10.4f, 4.8f }, { X, 10.0f, 4.45f }, 0.2f, 0.27f, 10, COLOR_NEGRO_R);
        CilindroR({ X, 10.02f, 4.47f }, { X, 9.98f, 4.43f }, 0.29f, 0.29f, 10, COLOR_CROMO_R, false);
    }
}


//--------------------------------------------------
// Gabinete tragamonedas (horneado; las piezas jugables son GLB)
//--------------------------------------------------

// Panel de la marquesina: rayos de sol y «777» dorado con sombra.
static void MarquesinaArteR(float X)
{
    const float x0 = X - 2.0f, x1 = X + 2.0f, y0 = 5.82f, y1 = 6.9f, z = 1.81f;
    const float w = x1 - x0, h = y1 - y0, L = 2.0f * (w + h);
    const int RAYOS = 24;
    float emis = g_emisR;
    g_emisR = 1.0f;

    auto Perimetro = [&](float s)
    {
        if (s < w) return Vector3{ x0 + s, y0, z };
        if (s < w + h) return Vector3{ x1, y0 + (s - w), z };
        if (s < 2.0f * w + h) return Vector3{ x1 - (s - w - h), y1, z };
        return Vector3{ x0, y1 - (s - 2.0f * w - h), z };
    };

    float marcas[RAYOS + 4];
    int cantidad = 0;
    for (int j = 0; j < RAYOS; j++) marcas[cantidad++] = L * (float)j / (float)RAYOS;
    float esquinas[3] = { w, w + h, 2.0f * w + h };
    for (float e : esquinas) marcas[cantidad++] = e;

    for (int a = 1; a < cantidad; a++)
    {
        for (int b = a; b > 0 && marcas[b - 1] > marcas[b]; b--)
        {
            float tmp = marcas[b];
            marcas[b] = marcas[b - 1];
            marcas[b - 1] = tmp;
        }
    }

    Vector3 centro = { X, 6.36f, z };
    for (int j = 0; j < cantidad; j++)
    {
        float s0 = marcas[j];
        float s1 = j + 1 < cantidad ? marcas[j + 1] : L;
        int rayo = (int)(0.5f * (s0 + s1) / (L / (float)RAYOS));
        TriR(centro, Perimetro(s0), Perimetro(s1 >= L ? L - 0.0001f : s1), rayo % 2 == 0 ? Color{ 66, 36, 146, 255 } : Color{ 40, 24, 104, 255 });
    }

    Vector3 n = { 0.0f, 0.0f, 1.0f };
    RecorrerTextoNeonR("777", X, 5.95f, 0.28f, [&](float ax, float ay, float bx, float by, int)
    {
        TrazoR({ ax + 0.05f, ay - 0.05f, z + 0.01f }, { bx + 0.05f, by - 0.05f, z + 0.01f }, n, 0.18f, Color{ 150, 20, 44, 255 });
    });
    RecorrerTextoNeonR("777", X, 5.95f, 0.28f, [&](float ax, float ay, float bx, float by, int)
    {
        TrazoR({ ax, ay, z + 0.02f }, { bx, by, z + 0.02f }, n, 0.15f, Color{ 255, 212, 92, 255 });
    });

    // Estrellitas a los costados.
    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 c = { X + 1.62f * (float)s, 6.36f, z + 0.02f };
        for (int k = 0; k < 4; k++)
        {
            float a = 1.5708f * (float)k;
            TriR(c, { c.x + std::cos(a - 0.35f) * 0.1f, c.y + std::sin(a - 0.35f) * 0.1f, c.z }, { c.x + std::cos(a) * 0.26f, c.y + std::sin(a) * 0.26f, c.z }, COLOR_CIAN_R);
            TriR(c, { c.x + std::cos(a) * 0.26f, c.y + std::sin(a) * 0.26f, c.z }, { c.x + std::cos(a + 0.35f) * 0.1f, c.y + std::sin(a + 0.35f) * 0.1f, c.z }, COLOR_CIAN_R);
        }
    }

    g_emisR = emis;
}


static void GabineteR(float X)
{
    Color laca = COLOR_LACA_R;
    Color lacaOscura = TonoR(COLOR_LACA_R, 0.68f);
    Color cromo = COLOR_CROMO_R;

    // Cuerpo inferior, zocalo, puerta de monedas y bandeja con monedas.
    CajaR({ X, 0.6f, -0.05f }, { 4.3f, 1.2f, 3.5f }, laca);
    CajaR({ X, 0.07f, 1.71f }, { 4.34f, 0.14f, 0.06f }, cromo);
    CajaR({ X - 1.3f, 0.74f, 1.72f }, { 0.75f, 0.62f, 0.05f }, TonoR(cromo, 0.8f));
    CajaR({ X - 1.3f, 0.86f, 1.75f }, { 0.08f, 0.22f, 0.03f }, COLOR_NEGRO_R);
    PanelR(X - 1.5f, 0.52f, X - 1.1f, 0.6f, 1.76f, Color{ 255, 90, 70, 255 });
    CajaR({ X + 0.55f, 0.66f, 1.72f }, { 1.4f, 0.34f, 0.04f }, COLOR_NEGRO_R);
    CajaR({ X + 0.55f, 0.3f, 1.95f }, { 1.75f, 0.08f, 0.5f }, cromo);
    CajaR({ X + 0.55f, 0.4f, 2.18f }, { 1.75f, 0.2f, 0.05f }, cromo);
    CajaR({ X - 0.32f, 0.4f, 1.95f }, { 0.05f, 0.2f, 0.5f }, cromo);
    CajaR({ X + 1.42f, 0.4f, 1.95f }, { 0.05f, 0.2f, 0.5f }, cromo);

    for (int c = 0; c < 6; c++)
    {
        float x = X + 0.0f + 0.22f * (float)c + 0.05f * HashR(c, 61);
        float z = 1.88f + 0.12f * (float)(c % 2);
        CilindroR({ x, 0.34f, z }, { x, 0.37f + 0.03f * (float)(c % 3), z }, 0.08f, 0.08f, 8, COLOR_ORO_R);
    }

    // Repisa de control (el boton GLB va al centro).
    CajaR({ X, 1.08f, 1.95f }, { 4.3f, 0.18f, 0.9f }, lacaOscura);
    CajaR({ X, 1.08f, 2.41f }, { 4.34f, 0.2f, 0.04f }, cromo);

    for (int s = -1; s <= 1; s += 2)
    {
        for (int b = 0; b < 2; b++)
        {
            CajaR({ X + (float)s * (1.2f + 0.5f * (float)b), 1.2f, 1.95f }, { 0.34f, 0.07f, 0.3f }, b == 0 ? TonoR(COLOR_AMARILLO_R, 0.85f) : TonoR(COLOR_CIAN_R, 0.75f));
        }
    }

    // Cuerpo de los rodillos con la ventana (x -1.65..1.65, y 1.8..4.2).
    CajaR({ X, 1.485f, 0.04f }, { 4.3f, 0.63f, 3.68f }, laca);
    CajaR({ X - 1.9f, 3.0f, 0.04f }, { 0.5f, 2.4f, 3.68f }, laca);
    CajaR({ X + 1.9f, 3.0f, 0.04f }, { 0.5f, 2.4f, 3.68f }, laca);
    CajaR({ X, 4.9f, 0.04f }, { 4.3f, 1.4f, 3.68f }, laca);
    CajaR({ X, 3.0f, -1.6f }, { 3.3f, 2.4f, 0.2f }, COLOR_NEGRO_R);
    CajaR({ X, 1.82f, 1.45f }, { 3.3f, 0.04f, 0.9f }, COLOR_NEGRO_R);
    CajaR({ X, 4.18f, 1.45f }, { 3.3f, 0.04f, 0.9f }, COLOR_NEGRO_R);

    for (int s = -1; s <= 1; s += 2)
    {
        CajaR({ X + (float)s * 0.525f, 3.0f, 1.25f }, { 0.05f, 2.4f, 1.3f }, TonoR(cromo, 0.45f));
    }

    // Bisel cromado de la ventana.
    CajaR({ X, 4.25f, 1.92f }, { 3.5f, 0.1f, 0.1f }, cromo);
    CajaR({ X, 1.75f, 1.92f }, { 3.5f, 0.1f, 0.1f }, cromo);
    CajaR({ X - 1.70f, 3.0f, 1.92f }, { 0.1f, 2.6f, 0.1f }, cromo);
    CajaR({ X + 1.70f, 3.0f, 1.92f }, { 0.1f, 2.6f, 0.1f }, cromo);

    // Filetes dorados y franjas de arte en los costados.
    const float alturas[3] = { 1.2f, 4.22f, 5.56f };
    for (int s = -1; s <= 1; s += 2)
    {
        float xs = X + (float)s * 2.165f;
        for (float y : alturas)
        {
            CajaR({ xs, y, 0.04f }, { 0.03f, 0.07f, 3.7f }, COLOR_ORO_R);
        }

        static const Color FRANJAS[3] = { { 236, 70, 170, 255 }, { 60, 200, 230, 255 }, { 236, 190, 80, 255 } };
        for (int j = 0; j < 3; j++)
        {
            float z0 = -1.5f + 0.45f * (float)j;
            TrazoR({ xs + 0.01f * (float)s, 1.45f, z0 }, { xs + 0.01f * (float)s, 3.95f, z0 + 1.6f }, { (float)s, 0.0f, 0.0f }, 0.16f, FRANJAS[j]);
        }
    }

    // Display del puntaje con marco dorado.
    CajaR({ X, 4.98f, 1.89f }, { 1.62f, 0.66f, 0.04f }, COLOR_ORO_R);
    CajaR({ X, 4.98f, 1.9f }, { 1.5f, 0.56f, 0.05f }, COLOR_NEGRO_R);

    // Marquesina, corona redondeada y pedestal del faro.
    CajaR({ X, 6.35f, 0.05f }, { 4.5f, 1.5f, 3.5f }, lacaOscura);
    MarquesinaArteR(X);
    CilindroR({ X - 2.25f, 7.1f, 1.42f }, { X + 2.25f, 7.1f, 1.42f }, 0.38f, 0.38f, 10, lacaOscura);
    CajaR({ X, 7.3f, 0.0f }, { 4.5f, 0.4f, 2.8f }, lacaOscura);
    CajaR({ X, 7.51f, 0.0f }, { 4.56f, 0.04f, 2.86f }, COLOR_ORO_R);
    CilindroR({ X, 7.5f, 0.3f }, { X, 7.62f, 0.3f }, 0.36f, 0.34f, 10, cromo);

    // Cubo de la palanca.
    CajaR({ X + 2.28f, 3.2f, 0.5f }, { 0.24f, 0.62f, 0.62f }, cromo);
}


//--------------------------------------------------
// Horneado de la sala
//--------------------------------------------------

struct DisposicionR
{
    float mitad;
    float cabinaX, cabinaZ;
    float gruaX, gruaZ;
    float rocolaX, rocolaZ;
    float pinballX, pinballZ;
};


static DisposicionR DisposicionSalaR(int n)
{
    float m = MitadFilaR(n);
    return DisposicionR{ m, -(m + 4.7f), -3.0f, m + 2.6f, -1.6f, m + 5.6f, -4.4f, m + 3.6f, 3.0f };
}


static void SueloSalaR(int n, bool decoracion, bool sombras)
{
    AlfombraR(-30.0f, 30.0f, Z_PARED_R, 18.0f, 1.5f, true, 1.0f, 1);

    for (int k = 0; k < n; k++)
    {
        AlfombraMaquinaR(XMaquinaR(k, n));
    }

    if (!sombras)
    {
        return;
    }

    for (int k = 0; k < n; k++)
    {
        float X = XMaquinaR(k, n);
        SombraRectR(X - 2.15f, -1.8f, X + 2.15f, 1.8f, 0.6f, 0.6f);
    }

    if (!decoracion)
    {
        return;
    }

    DisposicionR d = DisposicionSalaR(n);
    SombraRectR(d.cabinaX - 2.4f, d.cabinaZ - 1.2f, d.cabinaX + 2.4f, d.cabinaZ + 1.4f, 0.6f, 0.5f);
    SombraRectR(d.gruaX - 0.95f, d.gruaZ - 0.95f, d.gruaX + 0.95f, d.gruaZ + 0.95f, 0.45f, 0.5f);
    SombraRectR(d.rocolaX - 0.8f, d.rocolaZ - 0.5f, d.rocolaX + 0.8f, d.rocolaZ + 0.5f, 0.4f, 0.5f);
    SombraRectR(d.pinballX - 0.5f, d.pinballZ - 1.1f, d.pinballX + 0.5f, d.pinballZ + 1.4f, 0.35f, 0.4f);
    SombraRedondaR(-(d.mitad + 1.1f), -1.2f, 0.8f, 0.45f);
    SombraRedondaR(d.mitad + 1.0f, -3.4f, 0.8f, 0.45f);
    SombraRedondaR(d.gruaX, d.gruaZ + 1.7f, 0.5f, 0.4f);
    SombraRedondaR(d.pinballX, d.pinballZ + 2.2f, 0.5f, 0.4f);
    SombraRedondaR(d.mitad + 1.0f, 5.6f, 0.5f, 0.4f);

    float xs[32];
    int cantidad = PosicionesFilaFondoR(xs, 32);
    for (int k = 0; k < cantidad; k++)
    {
        SombraRectR(xs[k] - 0.8f, Z_FILA_FONDO_R - 0.65f, xs[k] + 0.8f, Z_FILA_FONDO_R + 1.15f, 0.35f, 0.5f);
    }
}


static void UtileriaSalaR(int n, int paso)
{
    DisposicionR d = DisposicionSalaR(n);
    Color rojo = { 200, 36, 60, 255 };

    CabinaPremiosR(d.cabinaX, d.cabinaZ, paso);
    GruaPeluchesR(d.gruaX, d.gruaZ);
    RocolaR(d.rocolaX, d.rocolaZ);
    PinballR(d.pinballX, d.pinballZ);
    TabureteR(d.gruaX, d.gruaZ + 1.7f, rojo);
    TabureteR(d.pinballX, d.pinballZ + 2.2f, rojo);

    if (paso == 1)
    {
        TabureteR(d.cabinaX - 1.1f, d.cabinaZ + 2.5f, Color{ 40, 150, 190, 255 });
        TabureteR(d.cabinaX + 0.9f, d.cabinaZ + 2.5f, Color{ 40, 150, 190, 255 });
        PalmeraR(d.cabinaX - 3.4f, d.cabinaZ - 0.6f, 3);
    }

    PalmeraR(-(d.mitad + 1.1f), -1.2f, 1);
    PalmeraR(d.mitad + 1.0f, -3.4f, 2);
    PapeleraR(d.mitad + 1.0f, 5.6f);

    // Cordon de terciopelo delante de la cabina de premios.
    float postes[3] = { -(d.mitad + 1.3f), -(d.mitad + 4.0f), -(d.mitad + 6.7f) };
    for (int k = 0; k < 3; k++)
    {
        PosteCordonR(postes[k], 5.4f);
        if (k > 0) CordonR({ postes[k - 1], 0.92f, 5.4f }, { postes[k], 0.92f, 5.4f });
    }

    float xs[32];
    int cantidad = PosicionesFilaFondoR(xs, 32);
    for (int k = 0; k < cantidad; k += paso)
    {
        MaquinaVideoR(xs[k], Z_FILA_FONDO_R, k);
    }

    AficheR(-9.5f, 4.0f, Z_PARED_R);
    AficheR(9.5f, 4.0f, Z_PARED_R);
}


struct HornoSalaR
{
    bool listo = false;
    int clave = -1;
    Mesh suelo{};
    bool sueloValido = false;
    Mesh sala{};
    bool salaValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoSalaR g_hornoR;


static void LiberarMallasSalaRodillos()
{
    LiberarMallaHorneada(g_hornoR.suelo, g_hornoR.sueloValido);
    LiberarMallaHorneada(g_hornoR.sala, g_hornoR.salaValida);
    g_hornoR.listo = false;
    g_hornoR.clave = -1;
}


static int ClaveHornoR(int n)
{
    return n * 16 + (int)CalidadDecoracion() * 2 + (CalidadSombras() != CALIDAD_BAJA ? 1 : 0);
}


// Graba la sala estatica para la cantidad de maquinas y la calidad actual.
static void HornearSalaR(int n)
{
    int clave = ClaveHornoR(n);

    if ((g_hornoR.listo && g_hornoR.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasSalaRodillos();

    AsegurarMaterialHorneado(g_hornoR.material, g_hornoR.materialListo);

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    EmpezarGrabacionMalla(g_grabR);
    SueloSalaR(n, decoracion, sombras);
    g_hornoR.sueloValido = TerminarGrabacionMalla(g_grabR, g_hornoR.suelo);

    EmpezarGrabacionMalla(g_grabR);
    ParedesSalaR(decoracion);
    TechoSalaR(n, decoracion);

    if (decoracion)
    {
        SegundoSalonR(paso);
        UtileriaSalaR(n, paso);
    }

    for (int k = 0; k < n; k++)
    {
        GabineteR(XMaquinaR(k, n));
    }

    g_hornoR.salaValida = TerminarGrabacionMalla(g_grabR, g_hornoR.sala);
    g_hornoR.clave = clave;
    g_hornoR.listo = true;
}


// El suelo va sin prueba de profundidad (alfombra, ribetes y sombras se
// pintan en orden, sin parpadeos entre capas coplanares); despues la sala.
static void DibujarHornoR()
{
    if (!g_hornoR.listo)
    {
        return;
    }

    rlDrawRenderBatchActive();
    Matrix identidad = MatrixIdentity();

    if (g_hornoR.sueloValido)
    {
        rlDisableDepthTest();
        DrawMesh(g_hornoR.suelo, g_hornoR.material, identidad);
        rlDrawRenderBatchActive();
        rlEnableDepthTest();
    }

    if (g_hornoR.salaValida)
    {
        DrawMesh(g_hornoR.sala, g_hornoR.material, identidad);
    }
}


//--------------------------------------------------
// Animado: LEDs, letreros, pantallas y utileria con movimiento
//--------------------------------------------------

static const Color BOMBILLA_ENCENDIDA_R = { 255, 238, 176, 255 };
static const Color BOMBILLA_APAGADA_R = { 110, 74, 62, 255 };


static void LedsPilastrasR(float t, bool fiesta)
{
    for (int p = 0; p < 6; p++)
    {
        float px = PILASTRAS_R[p];
        Color base = (p % 2 == 0) ? COLOR_CIAN_R : COLOR_ROSA_R;

        for (int s = -1; s <= 1; s += 2)
        {
            float x = px + 0.3f * (float)s;

            for (int k = 0; k < 19; k++)
            {
                float y = 0.95f + 0.55f * (float)k;
                float pulso = 0.5f + 0.5f * std::sin(t * 4.0f - (float)k * 0.55f + (float)p * 1.3f + (s > 0 ? 0.8f : 0.0f));
                Color color = base;

                if (fiesta)
                {
                    bool par = ((k + (int)(t * 8.0f)) % 2) == 0;
                    color = par ? COLOR_AMARILLO_R : COLOR_ROSA_R;
                    pulso = 1.0f;
                }

                PanelR(x - 0.09f, y - 0.16f, x + 0.09f, y + 0.16f, Z_PARED_R + 0.62f, TonoR(color, 0.22f + 0.78f * pulso * pulso));
            }
        }
    }
}


static void BombillaR(Vector3 p, bool encendida)
{
    float emis = g_emisR;
    g_emisR = 1.0f;
    CajaR(p, { 0.12f, 0.12f, 0.06f }, encendida ? BOMBILLA_ENCENDIDA_R : BOMBILLA_APAGADA_R);
    g_emisR = emis;
}


static bool BombillaLetreroEncendidaR(int k, float t, bool fiesta)
{
    if (fiesta) return std::fmod(t, 0.24f) < 0.12f;
    return ((k + (int)(t * 6.0f)) % 2) == 0;
}


static void ContornoArcoR(int s, float z, float ancho, Color color)
{
    float x0 = (float)s * ARCO_X0_R, x1 = (float)s * ARCO_X1_R, y = ARCO_ALTO_R + 0.3f;
    Vector3 n = { 0.0f, 0.0f, 1.0f };
    TrazoR({ x0, 0.4f, z }, { x0, y, z }, n, ancho, color);
    TrazoR({ x0, y, z }, { x1, y, z }, n, ancho, color);
    TrazoR({ x1, y, z }, { x1, 0.4f, z }, n, ancho, color);
}


static void LetrerosR(float t, bool decoracion, int n, bool fiesta)
{
    float z = Z_PARED_R + 0.2f;
    TextoNeonR("JACKPOT", 0.0f, 8.78f, z, 0.62f, 0.17f, COLOR_ROSA_R, t, 1);

    for (int k = 0; k < 27; k++)
    {
        float x = -6.3f + 12.6f * (float)k / 26.0f;
        BombillaR({ x, 8.47f, z }, BombillaLetreroEncendidaR(k, t, fiesta));
        BombillaR({ x, 10.83f, z }, BombillaLetreroEncendidaR(k + 1, t, fiesta));
    }

    if (!decoracion)
    {
        return;
    }

    TextoNeonR("ARCADE", -16.0f, 7.6f, Z_PARED_R + 0.1f, 0.4f, 0.13f, COLOR_CIAN_R, t, 2);
    TextoNeonR("777", 16.0f, 7.55f, Z_PARED_R + 0.1f, 0.58f, 0.17f, Color{ 255, 76, 96, 255 }, t, 3);

    float emis = g_emisR;
    g_emisR = 1.0f;
    for (int s = -1; s <= 1; s += 2)
    {
        ContornoArcoR(s, Z_PARED_R + 0.2f, 0.1f, Color{ 190, 120, 255, 255 });
        ContornoArcoR(s, Z_PARED_R + 0.22f, 0.04f, Color{ 236, 214, 255, 255 });
    }
    g_emisR = emis;

    DisposicionR d = DisposicionSalaR(n);
    TextoNeonR("PREMIOS", d.cabinaX, 5.74f, d.cabinaZ - 0.62f, 0.24f, 0.08f, COLOR_AMARILLO_R, t, 4);
}


static void PantallaJuegoR(float x, float z, int tipo, float t)
{
    const float x0 = x - 0.58f, x1 = x + 0.58f, y0 = 2.25f, y1 = 3.25f;
    float zz = z + 0.68f;
    static const Color FONDOS[4] = { { 10, 14, 44, 255 }, { 20, 44, 28, 255 }, { 8, 8, 26, 255 }, { 34, 12, 44, 255 } };
    PanelR(x0, y0, x1, y1, zz, FONDOS[tipo % 4]);
    zz += 0.02f;

    switch (tipo % 4)
    {
        case 0:
        {
            float desvio = 0.1f * std::sin(t * 1.4f + x);
            for (int r = 0; r < 3; r++)
            {
                for (int c = 0; c < 5; c++)
                {
                    float cx = x - 0.36f + 0.18f * (float)c + desvio;
                    float cy = 3.08f - 0.15f * (float)r;
                    PanelR(cx - 0.05f, cy - 0.04f, cx + 0.05f, cy + 0.04f, zz, r == 0 ? Color{ 255, 90, 200, 255 } : Color{ 110, 255, 140, 255 });
                }
            }
            float cx = x + 0.4f * std::sin(t * 0.9f + x);
            PanelR(cx - 0.07f, 2.32f, cx + 0.07f, 2.4f, zz, Color{ 120, 220, 255, 255 });
            float by = 2.42f + std::fmod(t * 1.5f + x, 0.5f);
            PanelR(cx - 0.012f, by, cx + 0.012f, by + 0.06f, zz, WHITE);
            break;
        }
        case 1:
        {
            PanelR(x - 0.3f, y0, x + 0.3f, y1, zz, Color{ 74, 74, 86, 255 });
            for (int d = 0; d < 4; d++)
            {
                float y = y1 - 0.08f - std::fmod(t * 1.6f + (float)d * 0.25f, 1.0f) * (y1 - y0 - 0.12f);
                PanelR(x - 0.02f, y - 0.06f, x + 0.02f, y, zz + 0.01f, Color{ 250, 250, 220, 255 });
            }
            float cx = x + 0.12f * std::sin(t * 1.3f + x);
            PanelR(cx - 0.07f, 2.35f, cx + 0.07f, 2.55f, zz + 0.02f, Color{ 255, 70, 70, 255 });
            break;
        }
        case 2:
        {
            for (int s = 0; s < 10; s++)
            {
                float sx = x0 + 0.04f + (x1 - x0 - 0.08f) * HashR(s, 7 + (int)x);
                float sy = y1 - 0.03f - std::fmod(t * 0.5f * (0.5f + HashR(s, 8)) + HashR(s, 9), 1.0f) * (y1 - y0 - 0.06f);
                PanelR(sx - 0.012f, sy - 0.012f, sx + 0.012f, sy + 0.012f, zz, Color{ 230, 230, 255, 255 });
            }
            float cx = x + 0.35f * std::sin(t + x);
            float emis = g_emisR;
            g_emisR = 1.0f;
            TriR({ cx - 0.08f, 2.32f, zz + 0.01f }, { cx + 0.08f, 2.32f, zz + 0.01f }, { cx, 2.5f, zz + 0.01f }, Color{ 255, 220, 90, 255 });
            g_emisR = emis;
            break;
        }
        default:
        {
            static const Color BLOQUES[4] = { { 255, 90, 90, 255 }, { 90, 200, 255, 255 }, { 255, 220, 80, 255 }, { 170, 110, 255, 255 } };
            for (int c = 0; c < 6; c++)
            {
                int alto = 1 + (int)(HashR(c, (int)x) * 3.0f);
                for (int r = 0; r < alto; r++)
                {
                    float bx = x0 + 0.1f + 0.16f * (float)c, by = y0 + 0.04f + 0.13f * (float)r;
                    PanelR(bx, by, bx + 0.14f, by + 0.11f, zz, BLOQUES[(c + r) % 4]);
                }
            }
            float fy = y1 - 0.15f - std::fmod(t * 0.35f, 1.0f) * 0.45f;
            PanelR(x - 0.15f, fy, x + 0.15f, fy + 0.13f, zz, BLOQUES[(int)(t * 0.35f) % 4]);
            break;
        }
    }
}


static void UtileriaAnimadaR(int n, float t)
{
    DisposicionR d = DisposicionSalaR(n);

    // Grua: el carro recorre el techo y la garra sube y baja.
    float gx = d.gruaX + 0.55f * std::sin(t * 0.6f);
    float gz = d.gruaZ + 0.5f * std::sin(t * 0.43f + 1.0f);
    float gy = 3.25f + 0.3f * std::sin(t * 0.9f);
    CajaR({ gx, 3.82f, d.gruaZ }, { 0.1f, 0.06f, 1.8f }, COLOR_CROMO_R);
    CajaR({ gx, 3.76f, gz }, { 0.22f, 0.1f, 0.22f }, COLOR_CROMO_R);
    CilindroR({ gx, 3.72f, gz }, { gx, gy, gz }, 0.012f, 0.012f, 4, COLOR_NEGRO_R, false);
    CajaR({ gx, gy - 0.04f, gz }, { 0.16f, 0.1f, 0.16f }, COLOR_CROMO_R);

    for (int k = 0; k < 3; k++)
    {
        float a = 2.0944f * (float)k + 0.3f;
        Vector3 base = { gx, gy - 0.08f, gz };
        Vector3 punta = { gx + std::cos(a) * 0.16f, gy - 0.3f, gz + std::sin(a) * 0.16f };
        CilindroR(base, punta, 0.02f, 0.015f, 4, COLOR_CROMO_R, false);
    }

    // Rocola: tubos de burbujas que cambian de color.
    static const Color TONOS[4] = { COLOR_ROSA_R, COLOR_AMARILLO_R, COLOR_CIAN_R, COLOR_VIOLETA_R };
    float emis = g_emisR;
    g_emisR = 1.0f;

    for (int s = -1; s <= 1; s += 2)
    {
        for (int k = 0; k < 8; k++)
        {
            float y = 0.25f + 0.3f * (float)k;
            int tono = (k + (int)(t * 3.0f)) % 4;
            Color c = MezclaR(TONOS[tono], TONOS[(tono + 1) % 4], std::fmod(t * 3.0f, 1.0f));
            CajaR({ d.rocolaX + 0.74f * (float)s, y + 0.14f, d.rocolaZ + 0.48f }, { 0.12f, 0.28f, 0.08f }, c);
        }
    }

    for (int k = 0; k < 9; k++)
    {
        float a0 = PI * (float)k / 9.0f, a1 = PI * (float)(k + 1) / 9.0f;
        int tono = (k + (int)(t * 3.0f)) % 4;
        TrazoR(
            { d.rocolaX + std::cos(a0) * 0.74f, 2.6f + std::sin(a0) * 0.74f, d.rocolaZ + 0.52f },
            { d.rocolaX + std::cos(a1) * 0.74f, 2.6f + std::sin(a1) * 0.74f, d.rocolaZ + 0.52f },
            { 0, 0, 1 }, 0.1f, TONOS[tono]
        );
    }

    // Pinball: tablero trasero con luces que corren.
    float px = d.pinballX, pz = d.pinballZ - 0.79f;
    PanelR(px - 0.44f, 1.45f, px + 0.44f, 2.65f, pz, Color{ 44, 20, 96, 255 });
    for (int k = 0; k < 10; k++)
    {
        float a = 2.0f * PI * (float)k / 10.0f;
        bool encendida = ((k + (int)(t * 7.0f)) % 3) == 0;
        float lx = px + std::cos(a) * 0.3f, ly = 2.05f + std::sin(a) * 0.38f;
        PanelR(lx - 0.04f, ly - 0.04f, lx + 0.04f, ly + 0.04f, pz + 0.02f, encendida ? COLOR_AMARILLO_R : Color{ 90, 50, 70, 255 });
    }
    TriR({ px - 0.16f, 1.95f, pz + 0.02f }, { px + 0.16f, 1.95f, pz + 0.02f }, { px, 2.25f, pz + 0.02f }, std::fmod(t, 1.0f) < 0.5f ? COLOR_ROSA_R : COLOR_CIAN_R);
    g_emisR = emis;
}


//--------------------------------------------------
// Animado: cada maquina (bombillas, faro, palanca, contador, lamparas)
//--------------------------------------------------

static bool EnGlitchR(const MinijuegoRodillosNeon& m, int jugador)
{
    return (m.estadoGlitch == 1 || m.estadoGlitch == 2) && m.glitchJugador == jugador && m.fase == FASE_RODILLOS_JUGANDO;
}


// 0 normal (persecucion), 1 festejo (triple o ganador), 2 falla.
static int ModoMaquinaR(const MinijuegoRodillosNeon& m, int jugador)
{
    const EstadoJugadorRodillos& e = m.estadosJugadores[jugador];

    if (EnGlitchR(m, jugador))
    {
        return 2;
    }

    if (m.fase == FASE_RODILLOS_JUGANDO && m.etapa == ETAPA_RODILLOS_RESULTADO && e.resultadoRonda >= RONDA_RODILLOS_TRIPLE)
    {
        return 1;
    }

    if (m.fase == FASE_RODILLOS_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        m.resultado.participantes[jugador].posicionFinal == 1)
    {
        return 1;
    }

    return 0;
}


static Vector3 PosBombillaMarquesinaR(float X, int k)
{
    const float x0 = -2.12f, x1 = 2.12f, y0 = 5.7f, y1 = 7.0f, z = 1.84f;

    if (k < 12) return { X + x0 + (x1 - x0) * (float)k / 11.0f, y1, z };
    k -= 12;
    if (k < 3) return { X + x1, y1 - (y1 - y0) * (float)(k + 1) / 4.0f, z };
    k -= 3;
    if (k < 12) return { X + x1 - (x1 - x0) * (float)k / 11.0f, y0, z };
    k -= 12;
    return { X + x0, y0 + (y1 - y0) * (float)(k + 1) / 4.0f, z };
}


static bool BombillaMarquesinaEncendidaR(int k, float t, int modo, int semilla)
{
    if (modo == 1) return ((k % 2) == 0) == (std::fmod(t, 0.24f) < 0.12f);
    if (modo == 2) return HashR(k + semilla * 50, (int)(t * 20.0f)) > 0.5f;
    return ((k + (int)(t * 7.0f)) % 3) == 0;
}


static void DigitoR(float cx, float cy, float z, int valor, Color encendido, Color apagado)
{
    static const unsigned char MAPA[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
    unsigned char bits = valor >= 0 ? MAPA[valor % 10] : 0;
    const float w = 0.12f, h = 0.18f, g = 0.03f, grosor = 0.045f;
    const float seg[7][4] =
    {
        { -w + g, h, w - g, h },
        { w, h - g, w, g },
        { w, -g, w, -h + g },
        { -w + g, -h, w - g, -h },
        { -w, -g, -w, -h + g },
        { -w, h - g, -w, g },
        { -w + g, 0.0f, w - g, 0.0f }
    };

    for (int s = 0; s < 7; s++)
    {
        TrazoR({ cx + seg[s][0], cy + seg[s][1], z }, { cx + seg[s][2], cy + seg[s][3], z }, { 0, 0, 1 }, grosor, (bits & (1 << s)) ? encendido : apagado);
    }
}


static void MaquinaAnimadaR(const MinijuegoRodillosNeon& m, int jugador, float X, int semilla)
{
    const EstadoJugadorRodillos& e = m.estadosJugadores[jugador];
    Color colorJugador = COLORES_JUGADORES_RODILLOS[jugador % MAX_PARTICIPANTES];
    float t = m.tiempoVisual;
    int modo = ModoMaquinaR(m, jugador);
    float emis = g_emisR;
    g_emisR = 1.0f;

    // Bombillas de la marquesina.
    for (int k = 0; k < BOMBILLAS_MARQUESINA_R; k++)
    {
        BombillaR(PosBombillaMarquesinaR(X, k), BombillaMarquesinaEncendidaR(k, t, modo, semilla));
    }

    // Faro de la corona, del color del jugador.
    float brillo = 0.6f + 0.2f * std::sin(t * 2.0f + (float)semilla);
    if (modo == 1) brillo = std::fmod(t, 0.25f) < 0.125f ? 1.0f : 0.7f;
    Color faro = MezclaR(TonoR(colorJugador, brillo), WHITE, modo == 1 ? 0.35f : 0.08f);
    CilindroR({ X, 7.62f, 0.3f }, { X, 7.95f, 0.3f }, 0.27f, 0.22f, 10, faro, false);
    BolaR({ X, 7.95f, 0.3f }, 0.22f, faro, 3, 10);

    // Contador de puntos de tres digitos (los ceros a la izquierda apagados).
    Color ambar = { 255, 172, 60, 255 };
    Color apagado = { 46, 20, 22, 255 };
    bool parpadeo = modo == 1 && std::fmod(t, 0.3f) < 0.12f;
    int puntos = e.puntos > 999 ? 999 : e.puntos;
    int digitos[3] = { puntos / 100, (puntos / 10) % 10, puntos % 10 };
    if (digitos[0] == 0) digitos[0] = -1;
    if (digitos[0] < 0 && digitos[1] == 0) digitos[1] = -1;
    for (int d = 0; d < 3; d++)
    {
        DigitoR(X - 0.36f + 0.36f * (float)d, 4.98f, 1.935f, parpadeo ? -1 : digitos[d], ambar, apagado);
    }

    // Lamparas sobre cada rodillo: girando, el que toca, detenido, comodin o falla.
    for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
    {
        const RodilloNeon& rodillo = e.rodillos[r];
        float x = X + ((float)r - 1.0f) * SEPARACION_RODILLOS;
        bool aviso = m.estadoGlitch == 1 && m.glitchJugador == jugador && m.glitchRodillo == r;
        bool efecto = m.estadoGlitch == 2 && m.glitchJugador == jugador && m.glitchRodillo == r && rodillo.estado == RODILLO_GIRANDO;
        bool arriba = false, doble = false;
        Color c;

        if (aviso || efecto)
        {
            c = std::fmod(t, 0.2f) < 0.1f ? COLOR_GLITCH_R : TonoR(COLOR_GLITCH_R, 0.45f);
            arriba = efecto && m.glitchInvierte;
            doble = efecto && !m.glitchInvierte;
        }
        else if (rodillo.estado != RODILLO_GIRANDO)
        {
            c = rodillo.comodin ? COLOR_AMARILLO_R : Color{ 236, 242, 255, 255 };
        }
        else if (m.fase == FASE_RODILLOS_JUGANDO && m.etapa == ETAPA_RODILLOS_GIRO && e.rodilloActual == r)
        {
            c = TonoR(colorJugador, 0.55f + 0.45f * (0.5f + 0.5f * std::sin(t * 10.0f)));
        }
        else
        {
            c = Color{ 64, 42, 76, 255 };
        }

        const float z = 1.935f;
        if (arriba)
        {
            TriR({ x - 0.15f, 4.37f, z }, { x + 0.15f, 4.37f, z }, { x, 4.58f, z }, c);
        }
        else if (doble)
        {
            TriR({ x - 0.12f, 4.58f, z }, { x + 0.12f, 4.58f, z }, { x, 4.46f, z }, c);
            TriR({ x - 0.12f, 4.47f, z }, { x + 0.12f, 4.47f, z }, { x, 4.35f, z }, c);
        }
        else
        {
            TriR({ x - 0.15f, 4.58f, z }, { x + 0.15f, 4.58f, z }, { x, 4.37f, z }, c);
        }
    }

    g_emisR = emis;

    // Palanca: se tira al empezar cada ronda.
    float tiron = 0.0f;
    if (m.fase == FASE_RODILLOS_JUGANDO && m.etapa == ETAPA_RODILLOS_AVISO)
    {
        float u = m.tiempoEtapa / 0.6f;
        if (u < 1.0f) tiron = std::sin(u * PI);
    }
    float angulo = 0.25f + tiron * 1.0f;
    Vector3 pivote = { X + 2.44f, 3.2f, 0.5f };
    Vector3 direccion = { 0.0f, std::cos(angulo), std::sin(angulo) };
    CilindroR(pivote, Vector3Add(pivote, Vector3Scale(direccion, 1.3f)), 0.05f, 0.04f, 6, COLOR_CROMO_R, false);
    BolaR(Vector3Add(pivote, Vector3Scale(direccion, 1.42f)), 0.16f, Color{ 226, 40, 58, 255 }, 4, 8);

    // Aro del color del jugador alrededor de sus pies.
    for (int k = 0; k < 20; k++)
    {
        float a0 = 2.0f * PI * (float)k / 20.0f, a1 = 2.0f * PI * (float)(k + 1) / 20.0f;
        TrazoR({ X + std::cos(a0) * 0.86f, 0.02f, 3.7f + std::sin(a0) * 0.86f }, { X + std::cos(a1) * 0.86f, 0.02f, 3.7f + std::sin(a1) * 0.86f }, { 0, 1, 0 }, 0.09f, colorJugador);
    }
}


//--------------------------------------------------
// Particulas deterministas: monedas, chispas de comodin y confeti
//--------------------------------------------------

static void MonedasR(const MinijuegoRodillosNeon& m, int jugador, float X, float factor)
{
    const EstadoJugadorRodillos& e = m.estadosJugadores[jugador];

    if (m.fase != FASE_RODILLOS_JUGANDO || m.etapa != ETAPA_RODILLOS_RESULTADO)
    {
        return;
    }

    int base = e.resultadoRonda == RONDA_RODILLOS_TRIPLE_ESTRELLA ? 40
        : (e.resultadoRonda == RONDA_RODILLOS_TRIPLE ? 26 : (e.resultadoRonda == RONDA_RODILLOS_PAR ? 8 : 0));
    int cantidad = (int)((float)base * factor + 0.5f);
    const float g = 11.0f;

    for (int i = 0; i < cantidad; i++)
    {
        int s = jugador * 1000 + m.ronda * 100 + i;
        float t0 = m.tiempoEtapa - 0.035f * (float)i;

        if (t0 <= 0.0f)
        {
            continue;
        }

        float vx = (HashR(s, 1) - 0.5f) * 3.2f;
        float vy = 3.4f + 2.4f * HashR(s, 2);
        float vz = 1.2f + 2.2f * HashR(s, 3);
        Vector3 p0 = { X + 0.55f + (HashR(s, 4) - 0.5f) * 1.2f, 0.62f, 2.05f };
        float aterriza = (vy + std::sqrt(vy * vy + 2.0f * g * (p0.y - 0.03f))) / g;
        float tt = t0 < aterriza ? t0 : aterriza;
        Vector3 p = { p0.x + vx * tt, p0.y + vy * tt - 0.5f * g * tt * tt, p0.z + vz * tt };

        if (t0 < aterriza)
        {
            float giro = t0 * 14.0f + HashR(s, 5) * 6.0f;
            CajaR(p, { 0.18f, 0.04f + 0.14f * std::fabs(std::sin(giro)), 0.04f + 0.14f * std::fabs(std::cos(giro)) }, COLOR_ORO_R);
        }
        else
        {
            CajaR(p, { 0.18f, 0.035f, 0.18f }, TonoR(COLOR_ORO_R, 0.9f));
        }
    }
}


static void ChispasComodinR(const MinijuegoRodillosNeon& m, int jugador, float X, float factor)
{
    const EstadoJugadorRodillos& e = m.estadosJugadores[jugador];
    int cantidad = (int)(12.0f * factor + 0.5f);

    for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
    {
        const RodilloNeon& rodillo = e.rodillos[r];

        if (rodillo.estado == RODILLO_GIRANDO || !rodillo.comodin || rodillo.tiempoParada > 0.6f || m.fase != FASE_RODILLOS_JUGANDO)
        {
            continue;
        }

        float u = rodillo.tiempoParada / 0.6f;
        Vector3 c = { X + ((float)r - 1.0f) * SEPARACION_RODILLOS, ALTURA_RODILLOS, 2.0f };

        for (int k = 0; k < cantidad; k++)
        {
            int s = jugador * 300 + r * 40 + k + m.ronda * 7;
            float a = 2.0f * PI * HashR(s, 1);
            float v = 1.4f + 1.6f * HashR(s, 2);
            Vector3 p = { c.x + std::cos(a) * v * rodillo.tiempoParada, c.y + std::sin(a) * v * rodillo.tiempoParada, c.z + 0.6f * rodillo.tiempoParada };
            float tam = 0.09f * (1.0f - u);
            float emis = g_emisR;
            g_emisR = 1.0f;
            CajaR(p, { tam, tam, tam }, k % 3 == 0 ? WHITE : COLOR_AMARILLO_R);
            g_emisR = emis;
        }
    }
}


static void ConfetiR(const MinijuegoRodillosNeon& m, int jugador, float X, float factor)
{
    if (m.fase != FASE_RODILLOS_TERMINADO || m.resultado.estado != RESULTADO_MINIJUEGO_FINALIZADO ||
        m.resultado.participantes[jugador].posicionFinal != 1)
    {
        return;
    }

    static const Color COLORES[5] = { COLOR_ROSA_R, COLOR_CIAN_R, COLOR_AMARILLO_R, COLOR_VIOLETA_R, { 120, 255, 150, 255 } };
    int cantidad = (int)(60.0f * factor + 0.5f);

    for (int i = 0; i < cantidad; i++)
    {
        int s = jugador * 700 + i;
        float v = 1.1f + 1.0f * HashR(s, 1);
        float periodo = 10.0f / v;
        float tt = std::fmod(m.tiempoFin + HashR(s, 2) * periodo, periodo);
        float y = 10.0f - v * tt;

        if (m.tiempoFin < HashR(s, 2) * 1.5f || y < 0.03f)
        {
            continue;
        }

        float x = X + (HashR(s, 3) - 0.5f) * 5.5f + 0.3f * std::sin(tt * 3.0f + HashR(s, 4) * 6.0f);
        float z = 1.2f + 3.6f * HashR(s, 5);
        float giro = tt * 6.0f + HashR(s, 6) * 6.0f;
        CajaR({ x, y, z }, { 0.14f * std::fabs(std::cos(giro)) + 0.03f, 0.14f, 0.02f }, COLORES[i % 5]);
    }
}


static void DibujarAnimadoR(const MinijuegoRodillosNeon& m, const int jugadorMaquina[], bool decoracion)
{
    int n = m.cantidadMaquinas;
    float t = m.tiempoVisual;
    float factor = FactorCalidadGrafica(CalidadParticulas());
    bool fiesta = false;

    for (int k = 0; k < n; k++)
    {
        if (jugadorMaquina[k] >= 0 && ModoMaquinaR(m, jugadorMaquina[k]) == 1) fiesta = true;
    }

    LoteR();
    LedsPilastrasR(t, fiesta);
    LetrerosR(t, decoracion, n, fiesta);

    if (decoracion)
    {
        float xs[32];
        int cantidad = PosicionesFilaFondoR(xs, 32);
        int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
        for (int k = 0; k < cantidad; k += paso)
        {
            PantallaJuegoR(xs[k], Z_FILA_FONDO_R, k, t);
        }
        UtileriaAnimadaR(n, t);
    }

    for (int k = 0; k < n; k++)
    {
        int jugador = jugadorMaquina[k];
        if (jugador < 0) continue;
        float X = XMaquinaR(k, n);
        MaquinaAnimadaR(m, jugador, X, k);
        MonedasR(m, jugador, X, factor);
        ChispasComodinR(m, jugador, X, factor);
        ConfetiR(m, jugador, X, factor);
    }

    FinLoteR();
}


//--------------------------------------------------
// Luces aditivas (al final del 3D)
//--------------------------------------------------

static void LucesMaquinaR(const MinijuegoRodillosNeon& m, int jugador, float X, int semilla)
{
    const EstadoJugadorRodillos& e = m.estadosJugadores[jugador];
    Color colorJugador = COLORES_JUGADORES_RODILLOS[jugador % MAX_PARTICIPANTES];
    float t = m.tiempoVisual;
    float ta = m.tiempoAnimacion;
    int modo = ModoMaquinaR(m, jugador);
    Vector3 ux = { 1.0f, 0.0f, 0.0f }, uy = { 0.0f, 1.0f, 0.0f };

    // Charco calido del foco y reflejo del color del jugador en la alfombra.
    GlowSueloR({ X, 0.03f, 3.1f }, 3.5f, AlfaR(COLOR_CALIDO_R, 0.2f));
    GlowSueloR({ X, 0.04f, 2.4f }, 2.3f, AlfaR(colorJugador, modo == 1 ? 0.26f : 0.12f));

    // Marquesina, bombillas y faro.
    GlowR({ X, 6.36f, 1.88f }, ux, uy, 3.1f, AlfaR(Color{ 255, 186, 120, 255 }, 0.15f));
    for (int k = 0; k < BOMBILLAS_MARQUESINA_R; k++)
    {
        if (BombillaMarquesinaEncendidaR(k, t, modo, semilla))
        {
            Vector3 p = PosBombillaMarquesinaR(X, k);
            p.z += 0.05f;
            GlowCaraR(p, 0.26f, AlfaR(Color{ 255, 220, 150, 255 }, 0.5f));
        }
    }

    float brillo = modo == 1 ? 1.0f : 0.55f + 0.2f * std::sin(t * 2.0f + (float)semilla);
    GlowCaraR({ X, 7.85f, 0.6f }, modo == 1 ? 1.6f : 1.0f, AlfaR(colorJugador, 0.38f * brillo));

    if (modo == 1)
    {
        // Haces giratorios del faro.
        for (int b = 0; b < 2; b++)
        {
            float a = t * 5.0f + PI * (float)b;
            Vector3 c = { X, 7.85f, 0.3f };
            Vector3 d = { std::cos(a), 0.15f, std::sin(a) };
            Vector3 lado = { -d.z * 0.7f, 0.0f, d.x * 0.7f };
            Vector3 lejos = Vector3Add(c, Vector3Scale(d, 7.0f));
            Color fuerte = AlfaR(colorJugador, 0.22f), nada = AlfaR(colorJugador, 0.0f);
            QuadColoresR(c, c, Vector3Add(lejos, lado), Vector3Subtract(lejos, lado), fuerte, fuerte, nada, nada);
        }
    }

    // Ventana de rodillos: linea de pago, giro, freno, comodin y falla.
    const float zr = 1.87f;
    GlowTrazoR({ X - 1.62f, ALTURA_RODILLOS, zr }, { X + 1.62f, ALTURA_RODILLOS, zr }, { 0, 0, 1 }, 0.18f, AlfaR(COLOR_AMARILLO_R, 0.22f));

    for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
    {
        const RodilloNeon& rodillo = e.rodillos[r];
        float x = X + ((float)r - 1.0f) * SEPARACION_RODILLOS;
        float x0 = x - 0.4f, x1 = x + 0.4f, y0 = 1.8f, y1 = 4.2f, ym = ALTURA_RODILLOS;
        bool aviso = m.estadoGlitch == 1 && m.glitchJugador == jugador && m.glitchRodillo == r;
        bool efecto = m.estadoGlitch == 2 && m.glitchJugador == jugador && m.glitchRodillo == r && rodillo.estado == RODILLO_GIRANDO;

        if (rodillo.estado == RODILLO_GIRANDO)
        {
            // Estela vertical del giro y un brillo que corre con el tambor.
            Color medio = AlfaR(WHITE, 0.07f), borde = AlfaR(WHITE, 0.0f);
            QuadColoresR({ x0, y0, zr }, { x1, y0, zr }, { x1, ym, zr }, { x0, ym, zr }, borde, borde, medio, medio);
            QuadColoresR({ x0, ym, zr }, { x1, ym, zr }, { x1, y1, zr }, { x0, y1, zr }, medio, medio, borde, borde);
            float sentido = efecto && m.glitchInvierte ? -1.0f : 1.0f;
            float banda = std::fmod(ta * 3.5f * sentido + (float)r * 0.31f + 10.0f, 1.0f);
            float by = y1 - banda * (y1 - y0);
            GlowTrazoR({ x0, by, zr }, { x1, by, zr }, { 0, 0, 1 }, 0.5f, AlfaR(WHITE, 0.09f));
        }
        else
        {
            Color luz = rodillo.comodin ? COLOR_AMARILLO_R : WHITE;
            float alfa = rodillo.comodin ? 0.3f + 0.14f * std::sin(t * 8.0f) : 0.14f;
            GlowR({ x, ym, zr }, ux, uy, 0.62f, AlfaR(luz, alfa));

            if (rodillo.tiempoParada < 0.3f)
            {
                float f = (1.0f - rodillo.tiempoParada / 0.3f) * 0.5f;
                Color c = AlfaR(luz, f);
                QuadColoresR({ x0, y0, zr }, { x1, y0, zr }, { x1, y1, zr }, { x0, y1, zr }, c, c, c, c);
            }
        }

        if (aviso || efecto)
        {
            bool encendido = std::fmod(t, 0.2f) < 0.12f;
            if (aviso && encendido)
            {
                Color c = AlfaR(COLOR_GLITCH_R, 0.3f);
                QuadColoresR({ x0, y0, zr }, { x1, y0, zr }, { x1, y1, zr }, { x0, y1, zr }, c, c, c, c);
            }
            else if (efecto)
            {
                Color c = AlfaR(COLOR_GLITCH_R, 0.14f);
                QuadColoresR({ x0, y0, zr }, { x1, y0, zr }, { x1, y1, zr }, { x0, y1, zr }, c, c, c, c);
            }

            // Lluvia de estatica.
            for (int b = 0; b < (aviso ? 6 : 3); b++)
            {
                float y = y0 + 0.05f + (y1 - y0 - 0.1f) * HashR((int)(t * 24.0f), b * 7 + r);
                Color c = AlfaR(b % 2 == 0 ? COLOR_GLITCH_R : COLOR_CIAN_R, 0.55f);
                QuadColoresR({ x0, y, zr }, { x1, y, zr }, { x1, y + 0.05f, zr }, { x0, y + 0.05f, zr }, c, c, c, c);
            }
        }
    }

    // Reflejo del vidrio.
    Color reflejo = AlfaR(WHITE, 0.05f), nada = AlfaR(WHITE, 0.0f);
    QuadColoresR({ X - 1.2f, 1.8f, 1.9f }, { X - 0.7f, 1.8f, 1.9f }, { X + 0.1f, 4.2f, 1.9f }, { X - 0.4f, 4.2f, 1.9f }, nada, reflejo, reflejo, nada);

    // Contador y lamparas.
    GlowR({ X, 4.98f, 1.96f }, ux, uy, 0.75f, AlfaR(Color{ 255, 160, 60, 255 }, 0.18f));
}


static void PolvoFocoR(float X, float t, int semilla, float factor)
{
    int cantidad = (int)(14.0f * factor + 0.5f);

    for (int i = 0; i < cantidad; i++)
    {
        int s = semilla * 100 + i;
        float y = 0.5f + 9.0f * std::fmod(HashR(s, 1) + t * 0.02f * (0.5f + HashR(s, 2)), 1.0f);
        float u = 1.0f - y / 10.0f;
        float radio = (0.26f + 2.3f * u) * HashR(s, 3) * 0.85f;
        float a = HashR(s, 4) * 2.0f * PI + t * 0.1f;
        float z = 4.45f - 2.05f * u;
        GlowCaraR({ X + std::cos(a) * radio, y, z + std::sin(a) * radio }, 0.05f, AlfaR(COLOR_CALIDO_R, 0.5f));
    }
}


static void DibujarLucesR(const MinijuegoRodillosNeon& m, const int jugadorMaquina[], bool decoracion)
{
    int n = m.cantidadMaquinas;
    float t = m.tiempoVisual;
    float factor = FactorCalidadGrafica(CalidadParticulas());

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Conos de los focos del techo (uno por maquina).
    for (int k = 0; k < n; k++)
    {
        float X = XMaquinaR(k, n);
        (DrawCylinderEx)({ X, 9.98f, 4.43f }, { X, 0.05f, 2.4f }, 0.26f, 2.7f, 18, AlfaR(COLOR_CALIDO_R, 0.04f));
    }

    LoteR();

    for (int k = 0; k < n; k++)
    {
        if (jugadorMaquina[k] < 0) continue;
        float X = XMaquinaR(k, n);
        LucesMaquinaR(m, jugadorMaquina[k], X, k);
        PolvoFocoR(X, t, k + 1, factor);
    }

    // Letrero JACKPOT y su resplandor en la pared.
    float z = Z_PARED_R + 0.2f;
    GlowR({ 0.0f, 9.65f, z + 0.06f }, { 1, 0, 0 }, { 0, 1, 0 }, 7.5f, AlfaR(COLOR_ROSA_R, 0.1f));
    GlowTextoNeonR("JACKPOT", 0.0f, 8.78f, z, 0.62f, 0.9f, COLOR_ROSA_R, t, 1);

    // Columnas de LEDs.
    for (int p = 0; p < 6; p++)
    {
        Color c = (p % 2 == 0) ? COLOR_CIAN_R : COLOR_ROSA_R;
        for (int s = -1; s <= 1; s += 2)
        {
            float x = PILASTRAS_R[p] + 0.3f * (float)s;
            GlowTrazoR({ x, 0.8f, Z_PARED_R + 0.66f }, { x, 11.0f, Z_PARED_R + 0.66f }, { 0, 0, 1 }, 0.8f, AlfaR(c, 0.08f));
        }
    }

    if (decoracion)
    {
        GlowTextoNeonR("ARCADE", -16.0f, 7.6f, Z_PARED_R + 0.1f, 0.4f, 0.7f, COLOR_CIAN_R, t, 2);
        GlowTextoNeonR("777", 16.0f, 7.55f, Z_PARED_R + 0.1f, 0.58f, 0.9f, Color{ 255, 76, 96, 255 }, t, 3);
        DisposicionR d = DisposicionSalaR(n);
        GlowTextoNeonR("PREMIOS", d.cabinaX, 5.74f, d.cabinaZ - 0.62f, 0.24f, 0.45f, COLOR_AMARILLO_R, t, 4);
        GlowR({ d.cabinaX, 3.0f, d.cabinaZ - 0.3f }, { 1, 0, 0 }, { 0, 1, 0 }, 3.2f, AlfaR(Color{ 255, 200, 150, 255 }, 0.08f));
        GlowSueloR({ d.cabinaX, 0.04f, d.cabinaZ + 1.8f }, 3.0f, AlfaR(COLOR_CALIDO_R, 0.1f));

        for (int s = -1; s <= 1; s += 2)
        {
            Color violeta = AlfaR(COLOR_VIOLETA_R, 0.16f);
            float x0 = (float)s * ARCO_X0_R, x1 = (float)s * ARCO_X1_R, y = ARCO_ALTO_R + 0.3f, zz = Z_PARED_R + 0.25f;
            GlowTrazoR({ x0, 0.4f, zz }, { x0, y, zz }, { 0, 0, 1 }, 0.8f, violeta);
            GlowTrazoR({ x0, y, zz }, { x1, y, zz }, { 0, 0, 1 }, 0.8f, violeta);
            GlowTrazoR({ x1, y, zz }, { x1, 0.4f, zz }, { 0, 0, 1 }, 0.8f, violeta);
        }

        float xs[32];
        int cantidad = PosicionesFilaFondoR(xs, 32);
        int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
        static const Color TONOS[4] = { { 90, 150, 255, 255 }, { 90, 255, 140, 255 }, { 200, 200, 255, 255 }, { 220, 120, 255, 255 } };
        for (int k = 0; k < cantidad; k += paso)
        {
            GlowR({ xs[k], 2.75f, Z_FILA_FONDO_R + 0.75f }, { 1, 0, 0 }, { 0, 1, 0 }, 1.1f, AlfaR(TONOS[k % 4], 0.12f));
            GlowSueloR({ xs[k], 0.04f, Z_FILA_FONDO_R + 1.4f }, 1.3f, AlfaR(TONOS[k % 4], 0.07f));
        }

        GlowR({ d.gruaX, 4.2f, d.gruaZ + 1.05f }, { 1, 0, 0 }, { 0, 1, 0 }, 1.4f, AlfaR(COLOR_AMARILLO_R, 0.14f));
        GlowR({ d.rocolaX, 1.8f, d.rocolaZ + 0.6f }, { 1, 0, 0 }, { 0, 1, 0 }, 1.5f, AlfaR(COLOR_ROSA_R, 0.12f));
        GlowSueloR({ d.rocolaX, 0.04f, d.rocolaZ + 1.2f }, 1.8f, AlfaR(COLOR_ROSA_R, 0.08f));
        GlowR({ d.pinballX, 2.05f, d.pinballZ - 0.7f }, { 1, 0, 0 }, { 0, 1, 0 }, 1.0f, AlfaR(COLOR_VIOLETA_R, 0.14f));
    }

    FinLoteR();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Piezas jugables GLB (con su primitiva de respaldo por pieza)
//--------------------------------------------------

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


// Marco y boton del color del jugador, tambores, simbolos y linea de pago
// con la banda del comodin. Las transformaciones se heredan una vez.
static void DibujarPiezasJugablesR(
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

    // Tubos de neon del color del jugador en los bordes del gabinete.
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
        (DrawSphere)({ 0.0f, 1.42f, 1.95f }, 0.3f, colorBoton);
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


//--------------------------------------------------
// Camara de la vista (no toca la camara del estado)
//--------------------------------------------------

static Camera3D CamaraBaseR(int n)
{
    Camera3D c{};
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = 45.0f;
    c.projection = CAMERA_PERSPECTIVE;

    if (n <= 2)
    {
        c.position = { 0.0f, 6.8f, 19.5f };
        c.target = { 0.0f, 3.9f, 0.0f };
    }
    else if (n == 3)
    {
        c.position = { 0.0f, 6.85f, 21.0f };
        c.target = { 0.0f, 3.85f, 0.0f };
    }
    else
    {
        c.position = { 0.0f, 6.9f, 22.6f };
        c.target = { 0.0f, 3.8f, 0.0f };
    }

    return c;
}


// Durante la cuenta la camara baja desde atras y a un costado.
static Camera3D CamaraVistaR(const MinijuegoRodillosNeon& m)
{
    Camera3D c = CamaraBaseR(m.cantidadMaquinas);

    if (m.fase == FASE_RODILLOS_PREPARACION)
    {
        float u = Lim01R(m.tiempoPreparacion / DURACION_PREPARACION_RODILLOS);
        float s = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        c.position.x += 5.0f * s;
        c.position.y += 2.6f * s;
        c.position.z += 7.0f * s;
        c.target.y += 0.6f * s;
    }

    return c;
}


//==================================================
// DIBUJO
//==================================================

static float AlfaAnuncioR(float u)
{
    if (u < 0.12f) return u / 0.12f;
    if (u > 0.8f) return Lim01R((1.0f - u) / 0.2f);
    return 1.0f;
}


static void AnunciosR(
    const MinijuegoRodillosNeon& m,
    const int jugadorMaquina[],
    const Camera3D& vista
)
{
    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();
    int n = m.cantidadMaquinas;
    float t = m.tiempoVisual;

    // «¡RONDA N!» al tirar de las palancas.
    if (m.fase == FASE_RODILLOS_JUGANDO && m.etapa == ETAPA_RODILLOS_AVISO)
    {
        float u = Lim01R(m.tiempoEtapa / DURACION_AVISO_RODILLOS);
        float escala = u < 0.18f ? 0.7f + 0.3f * (u / 0.18f) : 1.0f;
        float alfa = AlfaAnuncioR(u);
        const char* texto = m.ronda >= RONDAS_RODILLOS_NEON - 1 ? "¡ÚLTIMA RONDA!" : TextFormat("¡RONDA %d!", m.ronda + 1);
        DibujarTextoMundoContornoUI(texto, ancho * 0.5f, alto * 0.30f, (int)((float)UI_TEXTO_TITULO * escala), COLOR_AMARILLO_R, alfa);

        if (m.ronda > 0)
        {
            DibujarTextoMundoContornoUI("¡MÁS RÁPIDO!", ancho * 0.5f, alto * 0.30f + Px(62.0f), UI_TEXTO_GRANDE, COLOR_CIAN_R, alfa);
        }
    }

    int token = n >= 4 ? UI_TEXTO : UI_TEXTO_GRANDE;

    for (int k = 0; k < n; k++)
    {
        int jugador = jugadorMaquina[k];
        if (jugador < 0) continue;
        const EstadoJugadorRodillos& e = m.estadosJugadores[jugador];
        float X = XMaquinaR(k, n);
        Vector2 cima = GetWorldToScreen({ X, 8.35f, 0.6f }, vista);
        if (cima.y < alto * 0.22f) cima.y = alto * 0.22f;   // bajo las etiquetas del HUD

        // Falla del sistema sobre la maquina afectada.
        if (EnGlitchR(m, jugador))
        {
            if (m.estadoGlitch == 1)
            {
                if (std::fmod(t, 0.3f) < 0.2f)
                {
                    DibujarTextoMundoContornoUI("¡FALLO DE SISTEMA!", cima.x, cima.y - Px(30.0f), UI_TEXTO, COLOR_GLITCH_R);
                }
            }
            else
            {
                DibujarTextoMundoContornoUI(m.glitchInvierte ? "¡RODILLO INVERTIDO!" : "¡RODILLO ACELERADO!", cima.x, cima.y - Px(30.0f), UI_TEXTO, COLOR_GLITCH_R);
            }
        }

        // Comodin recien detenido.
        if (m.fase == FASE_RODILLOS_JUGANDO && m.etapa == ETAPA_RODILLOS_GIRO)
        {
            int mejor = -1;
            for (int r = 0; r < CANTIDAD_RODILLOS_NEON; r++)
            {
                const RodilloNeon& rodillo = e.rodillos[r];
                if (rodillo.estado != RODILLO_GIRANDO && rodillo.comodin && rodillo.tiempoParada < 1.1f &&
                    (mejor < 0 || rodillo.tiempoParada < e.rodillos[mejor].tiempoParada))
                {
                    mejor = r;
                }
            }

            if (mejor >= 0)
            {
                float u = e.rodillos[mejor].tiempoParada / 1.1f;
                Vector2 p = GetWorldToScreen({ X + ((float)mejor - 1.0f) * SEPARACION_RODILLOS, 4.7f, 2.0f }, vista);
                DibujarTextoMundoContornoUI("¡COMODÍN!", p.x, p.y - Px(20.0f) - u * Px(24.0f), UI_TEXTO, COLOR_AMARILLO_R, AlfaAnuncioR(u));
            }
        }

        // Resultado de la ronda.
        if (m.fase == FASE_RODILLOS_JUGANDO && m.etapa == ETAPA_RODILLOS_RESULTADO)
        {
            float u = Lim01R(m.tiempoEtapa / DURACION_RESULTADO_RODILLOS);
            float alfa = AlfaAnuncioR(u);
            float y = cima.y - Px(34.0f) - u * Px(20.0f);
            const char* linea = "SIN PREMIO";
            Color color = Color{ 190, 180, 210, 255 };

            if (e.resultadoRonda == RONDA_RODILLOS_TRIPLE_ESTRELLA) { linea = "¡ESTRELLAS!"; color = COLOR_AMARILLO_R; }
            else if (e.resultadoRonda == RONDA_RODILLOS_TRIPLE) { linea = "¡TRIPLE!"; color = COLOR_AMARILLO_R; }
            else if (e.resultadoRonda == RONDA_RODILLOS_PAR) { linea = "¡PAR!"; color = Color{ 236, 246, 255, 255 }; }

            DibujarTextoMundoContornoUI(linea, cima.x, y, e.resultadoRonda == RONDA_RODILLOS_NADA ? UI_TEXTO : token, color, alfa);

            if (e.puntosRonda > 0)
            {
                DibujarTextoMundoContornoUI(TextFormat("+%d", e.puntosRonda), cima.x, y + Px(token == UI_TEXTO ? 24.0f : 32.0f), token, Color{ 140, 255, 190, 255 }, alfa);
            }
        }
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

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(COLOR_FONDO_R);
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 12, 8, 24, 255 }, Color{ 40, 22, 64, 255 });

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int n = cantidadMaquinas;

    // Maquina -> participante.
    int jugadorMaquina[MAX_PARTICIPANTES];
    for (int k = 0; k < MAX_PARTICIPANTES; k++) jugadorMaquina[k] = -1;
    for (int i = 0; i < limite; i++)
    {
        int k = estadosJugadores[i].maquina;
        if (resultado.participantes[i].participo && k >= 0 && k < MAX_PARTICIPANTES) jugadorMaquina[k] = i;
    }

    Camera3D vista = CamaraVistaR(*this);
    g_ojoR = CamaraBaseR(n).position;
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derR = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaR = Vector3CrossProduct(g_derR, frente);
    HornearSalaR(n);

    BeginMode3D(vista);

    rlDisableBackfaceCulling();
    DibujarHornoR();
    DibujarAnimadoR(*this, jugadorMaquina, decoracion);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarPiezasJugablesR(*this, i, COLORES_JUGADORES_RODILLOS[i % MAX_PARTICIPANTES]);
        }
    }

    // Personaje compartido de pie delante de su maquina, mirandola.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

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

    if (efectos)
    {
        rlDisableBackfaceCulling();
        DibujarLucesR(*this, jugadorMaquina, decoracion);
        rlEnableBackfaceCulling();
    }

    EndMode3D();

    // Cartelitos J1..J4 sobre cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 cabeza = { estadosJugadores[i].posicionX, 0.7f + jugadores[i].tamano.y * 0.5f + 0.42f, 3.7f };
        Vector2 p = GetWorldToScreen(cabeza, vista);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        DibujarEtiquetaUI(TextFormat("J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1), p, 0.0f, opciones);
    }

    AnunciosR(*this, jugadorMaquina, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "RODILLOS NEÓN",
        "DETENÉ LOS 3 RODILLOS: 3 IGUALES SUMAN 10 (ESTRELLAS 20), 2 IGUALES 3. ¡EN LA LÍNEA ES COMODÍN!",
        resultado
    );

    hud.fase = fase == FASE_RODILLOS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_RODILLOS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "RONDA";
    int rondaVisible = ronda < RONDAS_RODILLOS_NEON ? ronda + 1 : RONDAS_RODILLOS_NEON;
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", rondaVisible, RONDAS_RODILLOS_NEON);
    float avance = (float)ronda + (etapa == ETAPA_RODILLOS_RESULTADO ? 1.0f : 0.0f);
    hud.progresoMedidor = fase == FASE_RODILLOS_TERMINADO ? 1.0f : Lim01R(avance / (float)RONDAS_RODILLOS_NEON);
    hud.finPorTiempo = fase == FASE_RODILLOS_TERMINADO && tiempoRestante <= 0.0f;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorRodillos& e = estadosJugadores[i];
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        const Participante& participante = participantes[i];

        jugador.visible = true;
        jugador.valorNumerico = e.puntos;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d PTS", e.puntos);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d PTS", e.puntos);
        std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "%d TRIPLES", e.triples);

        if (fase == FASE_RODILLOS_JUGANDO)
        {
            if (EnGlitchR(*this, i))
            {
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡GLITCH!");
            }
            else if (etapa == ETAPA_RODILLOS_GIRO)
            {
                if (e.rodilloActual < CANTIDAD_RODILLOS_NEON)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "RODILLO %d/3", e.rodilloActual + 1);
                else
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡LISTO!");
            }
            else if (etapa == ETAPA_RODILLOS_RESULTADO)
            {
                if (e.resultadoRonda == RONDA_RODILLOS_TRIPLE_ESTRELLA)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡ESTRELLAS! +%d", e.puntosRonda);
                else if (e.resultadoRonda == RONDA_RODILLOS_TRIPLE)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡TRIPLE! +%d", e.puntosRonda);
                else if (e.resultadoRonda == RONDA_RODILLOS_PAR)
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "PAR +%d", e.puntosRonda);
                else
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "SIN PREMIO");
            }
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "DETENÉ: %s / %s",
                TextoBotonGolpeHud(participante),
                ObtenerTextoBotonPrincipal(participante)
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);

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
                160 + 18 * i,
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


void MinijuegoRodillosNeon::Descargar()
{
    LiberarMallasSalaRodillos();
    LiberarMaterialHorneado(g_hornoR.material, g_hornoR.materialListo);
}
