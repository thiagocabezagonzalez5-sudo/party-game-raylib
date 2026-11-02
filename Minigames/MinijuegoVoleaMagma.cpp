#include "Minigames/MinijuegoVoleaMagma.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
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
// CONSTANTES (REGLAS Y CANCHA)
//==================================================

static const float DURACION_PREPARACION_VOLEA = 3.0f;
static const float DURACION_PARTIDO_VOLEA = 75.0f;
static const float DURACION_PUNTO_ORO_VOLEA = 20.0f;
static const int PUNTOS_PARA_GANAR_VOLEA = 7;
static const float PAUSA_PUNTO_VOLEA = 1.7f;
static const float ESPERA_SAQUE_VOLEA = 4.0f;

// Cancha: la red esta en x = 0; el equipo 0 ocupa x < 0 y el 1 x > 0.
static const float MITAD_CANCHA_X = 9.0f;
static const float MITAD_CANCHA_Z = 5.0f;
static const float ALTURA_RED = 2.4f;
static const float LIMITE_JUGADOR_X = 8.6f;
static const float LIMITE_JUGADOR_RED = 0.7f;
static const float LIMITE_JUGADOR_Z = 4.6f;
static const float ALTURA_CENTRO_JUGADOR = 0.7f;

static const float RADIO_PELOTA = 0.45f;
static const float GRAVEDAD_PELOTA = 14.0f;
static const float ALCANCE_HORIZONTAL = 1.6f;
static const float ALCANCE_VERTICAL_MAXIMO = 2.9f;
static const float MULTIPLICADOR_MINORIA = 1.25f;
static const float RECARGA_GOLPE_VOLEA = 0.32f;

static const float TEMPERATURA_SOBRECALENTADA = 0.85f;
static const float DURACION_CHARCO = 4.0f;
static const float RADIO_CHARCO = 1.8f;
static const float MULTIPLICADOR_CHARCO = 0.45f;

// Los mismos tonos de equipo que el resto de los 2 vs 2 (rojo y azul).
static const Color COLOR_EQUIPO_0 = { 238, 55, 66, 255 };
static const Color COLOR_EQUIPO_1 = { 40, 159, 224, 255 };


//==================================================
// UTILIDADES
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


static bool JugadorEsBot(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


// El estadio se dibuja con primitivas: del paquete GLB solo se cargan las
// piezas jugables (red, postes, roca, charco, sombra, indicador y estela).
static const ModeloVoleaMagma3D PIEZAS_JUGABLES_VOLEA[] =
{
    MODELO_VOLEA_POSTE,
    MODELO_VOLEA_RED,
    MODELO_VOLEA_ROCA,
    MODELO_VOLEA_ROCA_CALIENTE,
    MODELO_VOLEA_CHARCO,
    MODELO_VOLEA_SOMBRA,
    MODELO_VOLEA_INDICADOR,
    MODELO_VOLEA_ESTELA
};


static void CargarPiezasJugablesVolea()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteVoleaMagmaRetro3D();
    const int mallasEsperadas[CANTIDAD_MODELOS_VOLEA_MAGMA_3D] = {3,8,3,5,4,6,6,3,5,4,4,4,2,3,3,1,2};

    for (ModeloVoleaMagma3D pieza : PIEZAS_JUGABLES_VOLEA)
    {
        PaqueteModelosEscenarioRetro3D una = { paquete.recursos + pieza, paquete.definiciones + pieza, 1 };
        CargarPaqueteModelosEscenarioRetro3D(una);

        RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[pieza];
        if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[pieza]) continue;
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.cargado = false;
        TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
    }
}


static Color ColorEquipoVolea(int equipo)
{
    return equipo == 0 ? COLOR_EQUIPO_0 : COLOR_EQUIPO_1;
}


// Signo de x en la mitad propia: equipo 0 = -1, equipo 1 = +1.
static float SignoMitad(int equipo)
{
    return equipo == 0 ? -1.0f : 1.0f;
}


static bool EsMinoria(const MinijuegoVoleaMagma& m, int equipo)
{
    int otro = equipo == 0 ? 1 : 0;

    return m.cantidadJugadoresEquipo[equipo] < m.cantidadJugadoresEquipo[otro];
}


static float AlcanceHorizontal(const MinijuegoVoleaMagma& m, int equipo)
{
    return ALCANCE_HORIZONTAL * (EsMinoria(m, equipo) ? MULTIPLICADOR_MINORIA : 1.0f);
}


static float PiesJugador(const JugadorPrueba& jugador)
{
    return jugador.posicion.y - jugador.tamano.y * 0.5f;
}


static bool PelotaAlAlcance(
    const MinijuegoVoleaMagma& m,
    int equipo,
    const JugadorPrueba& jugador
)
{
    float dx = m.pelota.posicion.x - jugador.posicion.x;
    float dz = m.pelota.posicion.z - jugador.posicion.z;
    float relativa = m.pelota.posicion.y - PiesJugador(jugador);
    float alcance = AlcanceHorizontal(m, equipo);

    return
        dx * dx + dz * dz <= alcance * alcance &&
        relativa >= 0.1f &&
        relativa <= ALCANCE_VERTICAL_MAXIMO;
}


// Tiempo y posicion en que la pelota baja hasta la altura dada (si ya esta
// por debajo, devuelve su posicion actual con t = 0).
static void PredecirPelota(
    const PelotaVolea& pelota,
    float altura,
    float& tiempo,
    float& x,
    float& z
)
{
    tiempo = 0.0f;

    if (pelota.estado == PELOTA_VOLEA_EN_JUEGO)
    {
        float discriminante =
            pelota.velocidad.y * pelota.velocidad.y +
            2.0f * GRAVEDAD_PELOTA * (pelota.posicion.y - altura);

        if (discriminante > 0.0f)
        {
            tiempo = (pelota.velocidad.y + std::sqrt(discriminante)) / GRAVEDAD_PELOTA;
        }

        if (tiempo < 0.0f) tiempo = 0.0f;
    }

    x = pelota.posicion.x + pelota.velocidad.x * tiempo;
    z = pelota.posicion.z + pelota.velocidad.z * tiempo;
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarVolea(MinijuegoVoleaMagma& m)
{
    if (m.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    m.empate = m.puntos[0] == m.puntos[1];
    m.equipoGanador = -1;

    if (!m.empate)
    {
        m.equipoGanador = m.puntos[0] > m.puntos[1] ? 0 : 1;
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
    m.resultado.desenlace = m.empate ? DESENLACE_EMPATE : DESENLACE_CON_GANADOR;
    m.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = m.estadosJugadores[i].equipo;
        resultadoJugador.numeroEquipo = equipo;
        resultadoJugador.puntuacionMinijuego =
            equipo >= 0 && equipo < 2 ? m.puntos[equipo] : 0;
        resultadoJugador.puntosObtenidos = 0;
        resultadoJugador.posicionFinal =
            m.empate || equipo == m.equipoGanador ? 1 : 2;
    }

    m.fase = FASE_VOLEA_TERMINADO;
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

static void IniciarSaque(MinijuegoVoleaMagma& m, int equipo)
{
    m.equipoSaque = equipo;
    m.pelota.estado = PELOTA_VOLEA_SAQUE;
    m.pelota.posicion = { equipo == 0 ? -3.5f : 3.5f, 2.4f, 0.0f };
    m.pelota.velocidad = {};
    m.pelota.tiempoEstado = 0.0f;
    m.toques[0] = 0;
    m.toques[1] = 0;
    m.equipoUltimoToque = -1;
    m.ultimoJugador = -1;

    for (int k = 0; k < LARGO_ESTELA_VOLEA; k++)
    {
        m.pelota.estela[k] = m.pelota.posicion;
    }
}


void MinijuegoVoleaMagma::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;
    pelota = {};
    pelota.temperatura = 0.6f;

    for (int i = 0; i < MAX_CHARCOS_VOLEA; i++)
    {
        charcos[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_VOLEA; i++)
    {
        particulas[i] = {};
    }

    // Suelo de obsidiana: unico bloque con colision del mapa.
    bloques[0] = {};
    bloques[0].posicion = { 0.0f, -0.5f, 0.0f };
    bloques[0].posicionInicial = bloques[0].posicion;
    bloques[0].tamano = { 2.0f * MITAD_CANCHA_X + 2.0f, 1.0f, 2.0f * MITAD_CANCHA_Z + 2.0f };
    bloques[0].color = Color{ 36, 32, 44, 255 };

    puntos[0] = 0;
    puntos[1] = 0;
    puntosTotales = 0;
    equipoSaque = 0;
    toques[0] = 0;
    toques[1] = 0;
    equipoUltimoToque = -1;
    ultimoJugador = -1;
    avisoToques = 0.0f;
    equipoGanador = -1;
    empate = false;
    modoOro = false;
    partidaValida = false;
    mensaje = MENSAJE_VOLEA_NINGUNO;
    equipoMensaje = -1;
    tiempoMensaje = 0.0f;

    // Un poco mas atras y baja que antes: la cancha queda entera y detras
    // se ven las gradas, el lago de lava y los volcanes.
    camara.position = { 0.0f, 8.6f, 16.6f };
    camara.target = { 0.0f, 2.1f, -1.6f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_VOLEA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_VOLEA;
    tiempoRestante = DURACION_PARTIDO_VOLEA;
    tiempoAnimacion = 0.0f;

    for (int i = 0; i < MAX_CHISPAS_VOLEA; i++)
    {
        chispas[i] = {};
    }

    semillaVisual = 1u;
    tiempoVisual = 0.0f;
    tiempoAnuncioInicio = -1.0f;
    tiempoFin = -1.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        destelloJugador[i] = 0.0f;
    }

    festejo[0] = festejo[1] = 0.0f;
    pulsoMarcador[0] = pulsoMarcador[1] = 0.0f;
    sacudidaRed = 0.0f;
    destelloGolpe = 0.0f;
    posicionGolpe = {};
    tiempoImpacto = -1.0f;
    posicionImpacto = {};

    IniciarSaque(*this, 0);
}


void MinijuegoVoleaMagma::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );
    resultado.cantidadEquipos = 2;

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            indices[cantidad++] = i;
        }
    }

    if (cantidad < 2)
    {
        for (int k = 0; k < cantidad; k++)
        {
            resultado.participantes[indices[k]].numeroEquipo = 0;
        }

        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_VOLEA_TERMINADO;
        return;
    }

    partidaValida = true;
    CargarPiezasJugablesVolea();

    for (int i = cantidad - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = indices[i];
        indices[i] = indices[otro];
        indices[otro] = temporal;
    }

    // 4: 2 vs 2. 3: 2 vs 1 (el solitario tiene ventaja). 2: 1 vs 1.
    int enEquipo0 = cantidad >= 3 ? 2 : 1;
    int ordenEquipo[2] = { 0, 0 };

    for (int k = 0; k < cantidad; k++)
    {
        int equipo = k < enEquipo0 ? 0 : 1;
        int indice = indices[k];

        estadosJugadores[indice].equipo = equipo;
        estadosJugadores[indice].orden = ordenEquipo[equipo]++;
        cantidadJugadoresEquipo[equipo]++;
        resultado.participantes[indice].numeroEquipo = equipo;
    }

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        EstadoJugadorVolea& estado = estadosJugadores[indice];
        float signo = SignoMitad(estado.equipo);
        bool solo = cantidadJugadoresEquipo[estado.equipo] <= 1;

        float x = solo ? 4.5f : (estado.orden == 0 ? 3.0f : 6.2f);
        float z = solo ? 0.0f : (estado.orden == 0 ? -1.3f : 1.3f);
        Vector3 spawn = { signo * x, ALTURA_CENTRO_JUGADOR, z };

        ConfigurarJugadorMinijuegoEstandar(jugadores[indice], spawn);
        jugadores[indice].posicion = spawn;
        jugadores[indice].direccionMirada = { -signo, 0.0f, 0.0f };
        jugadores[indice].enSuelo = true;
        jugadores[indice].cayendo = false;

        estado.habilidad = 0.45f + 0.35f * Aleatorio01();
        estado.apuntarX = -signo * 5.5f;
        estado.tiempoDecision = Aleatorio01() * 0.4f;
    }

    IniciarSaque(*this, GetRandomValue(0, 1));
}


//==================================================
// GOLPES
//==================================================

struct SonidosFrameVolea
{
    bool impacto = false;
    bool remate = false;
    bool anota = false;
    bool recibe = false;
};


// Calcula la velocidad para que la pelota caiga en (objetivoX, objetivoZ)
// con un vuelo de "vuelo" segundos, alargandolo hasta pasar sobre la red.
static void LanzarHaciaObjetivo(
    PelotaVolea& pelota,
    float objetivoX,
    float objetivoZ,
    float vuelo
)
{
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;

    for (int intento = 0; intento < 14; intento++)
    {
        vx = (objetivoX - pelota.posicion.x) / vuelo;
        vz = (objetivoZ - pelota.posicion.z) / vuelo;
        vy = (RADIO_PELOTA - pelota.posicion.y + 0.5f * GRAVEDAD_PELOTA * vuelo * vuelo) / vuelo;

        bool cruzaRed = pelota.posicion.x * objetivoX < 0.0f && std::fabs(pelota.posicion.x) > 0.6f;

        if (!cruzaRed || vuelo >= 2.4f)
        {
            break;
        }

        float tiempoRed = -pelota.posicion.x / vx;
        float alturaRed =
            pelota.posicion.y + vy * tiempoRed - 0.5f * GRAVEDAD_PELOTA * tiempoRed * tiempoRed;

        if (alturaRed >= ALTURA_RED + 0.8f)
        {
            break;
        }

        vuelo += 0.1f;
    }

    pelota.velocidad = { vx, vy, vz };
}


static void ResolverGolpeJugador(
    MinijuegoVoleaMagma& m,
    int indice,
    JugadorPrueba& jugador,
    const InputMinijuegoParticipante& entrada,
    bool esBot,
    SonidosFrameVolea& sonidos
)
{
    EstadoJugadorVolea& estado = m.estadosJugadores[indice];
    int equipo = estado.equipo;

    estado.recargaGolpe = RECARGA_GOLPE_VOLEA;
    estado.tiempoSwing = 0.22f;
    jugador.golpeando = true;
    jugador.tiempoGolpe = 0.2f;

    PelotaVolea& pelota = m.pelota;

    if (pelota.estado == PELOTA_VOLEA_PUNTO || pelota.enfriamientoToque > 0.0f)
    {
        return;
    }

    if (!PelotaAlAlcance(m, equipo, jugador))
    {
        return;
    }

    // Solo se golpea en la mitad propia (o pegada a la red).
    float signo = SignoMitad(equipo);

    if (pelota.posicion.x * signo < -0.8f)
    {
        return;
    }

    // En el saque solo puede golpear el equipo que saca.
    if (pelota.estado == PELOTA_VOLEA_SAQUE && equipo != m.equipoSaque)
    {
        return;
    }

    // Maximo 2 toques por equipo y nunca dos seguidos del mismo jugador
    // (salvo que el equipo tenga un solo integrante).
    bool toqueSeguido =
        m.equipoUltimoToque == equipo &&
        m.ultimoJugador == indice &&
        m.cantidadJugadoresEquipo[equipo] > 1;

    if (m.toques[equipo] >= 2 || toqueSeguido)
    {
        m.avisoToques = 1.2f;
        return;
    }

    float ataque = -signo;
    bool remate = !jugador.enSuelo;
    float objetivoX = ataque * 5.8f;
    float objetivoZ = 0.0f;

    if (esBot)
    {
        objetivoX = estado.apuntarX;
        objetivoZ = estado.apuntarZ;
    }
    else
    {
        float lateral = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
        float profundo = (entrada.atras ? 1.0f : 0.0f) - (entrada.adelante ? 1.0f : 0.0f);
        objetivoX = ataque * 5.8f + lateral * 2.4f;
        objetivoZ = profundo * 3.2f;
    }

    if (remate)
    {
        // El remate cae mas cerca de la red, rapido y bajo.
        objetivoX = ataque * Acotar(std::fabs(objetivoX) * 0.7f, 2.2f, 6.0f);
    }

    float distancia = Acotar(std::fabs(objetivoX), 2.0f, 8.3f);
    objetivoX = ataque * distancia;
    objetivoZ = Acotar(objetivoZ, -4.2f, 4.2f);

    LanzarHaciaObjetivo(pelota, objetivoX, objetivoZ, remate ? 0.55f : 1.15f);

    pelota.estado = PELOTA_VOLEA_EN_JUEGO;
    pelota.enfriamientoToque = 0.2f;
    m.toques[equipo]++;
    m.equipoUltimoToque = equipo;
    m.ultimoJugador = indice;

    if (!esBot)
    {
        if (remate)
        {
            sonidos.remate = true;
        }
        else
        {
            sonidos.impacto = true;
        }
    }
}


//==================================================
// IA DE BOTS
//==================================================
//
// El bot que esta mas cerca del punto donde la pelota bajara a altura de
// golpe es el responsable: va hasta alli, salta si la pelota queda alta y
// golpea con un pequeno retardo. Los demas se colocan en formacion
// (delantero / zaguero) para no estorbarse.
//==================================================

static int ElegirResponsable(
    const MinijuegoVoleaMagma& m,
    int equipo,
    const JugadorPrueba jugadores[],
    int limite,
    float puntoX,
    float puntoZ
)
{
    int mejor = -1;
    float mejorDistancia = 100000.0f;

    for (int j = 0; j < limite; j++)
    {
        if (m.estadosJugadores[j].equipo != equipo)
        {
            continue;
        }

        float dx = jugadores[j].posicion.x - puntoX;
        float dz = jugadores[j].posicion.z - puntoZ;
        float distancia = dx * dx + dz * dz;

        // Quien acaba de tocar la pelota no puede repetir (si hay companero).
        if (
            m.equipoUltimoToque == equipo &&
            m.ultimoJugador == j &&
            m.cantidadJugadoresEquipo[equipo] > 1
        )
        {
            distancia += 100.0f;
        }

        if (distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
            mejor = j;
        }
    }

    return mejor;
}


static InputMinijuegoParticipante CrearEntradaBotVolea(
    MinijuegoVoleaMagma& m,
    int indice,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorVolea& estado = m.estadosJugadores[indice];
    const JugadorPrueba& jugador = jugadores[indice];
    const PelotaVolea& pelota = m.pelota;
    float signo = SignoMitad(estado.equipo);
    float ataque = -signo;

    estado.tiempoDecision -= deltaTime;

    if (estado.tiempoDecision <= 0.0f)
    {
        float error = 1.1f * (1.0f - estado.habilidad);
        estado.tiempoDecision = 0.4f + 0.4f * Aleatorio01();
        estado.ruidoX = (Aleatorio01() - 0.5f) * 2.0f * error;
        estado.ruidoZ = (Aleatorio01() - 0.5f) * 2.0f * error;
        estado.apuntarX = ataque * (3.5f + 4.2f * Aleatorio01());
        estado.apuntarZ = (Aleatorio01() - 0.5f) * 7.0f;
    }

    float tiempo = 0.0f;
    float puntoX = 0.0f;
    float puntoZ = 0.0f;
    PredecirPelota(pelota, 1.9f, tiempo, puntoX, puntoZ);

    bool enMiLado = false;

    if (pelota.estado == PELOTA_VOLEA_SAQUE)
    {
        enMiLado = m.equipoSaque == estado.equipo;
    }
    else if (pelota.estado == PELOTA_VOLEA_EN_JUEGO)
    {
        enMiLado = puntoX * signo > -0.3f;
    }

    bool responsable =
        enMiLado &&
        ElegirResponsable(m, estado.equipo, jugadores, limite, puntoX, puntoZ) == indice;

    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;

    if (responsable)
    {
        objetivoX = puntoX + estado.ruidoX;
        objetivoZ = puntoZ + estado.ruidoZ;
    }
    else
    {
        bool solo = m.cantidadJugadoresEquipo[estado.equipo] <= 1;
        float profundidad = solo ? 4.5f : (estado.orden == 0 ? 2.9f : 6.2f);
        float desfaseZ = solo ? 0.0f : (estado.orden == 0 ? -1.3f : 1.3f);
        objetivoX = signo * profundidad;
        objetivoZ = pelota.posicion.z * 0.5f + desfaseZ + 0.9f * std::sin((float)GetTime() * 1.1f + (float)indice * 1.7f);
    }

    // Se queda en su mitad.
    float minimoX = signo < 0.0f ? -LIMITE_JUGADOR_X : LIMITE_JUGADOR_RED;
    float maximoX = signo < 0.0f ? -LIMITE_JUGADOR_RED : LIMITE_JUGADOR_X;
    objetivoX = Acotar(objetivoX, minimoX, maximoX);
    objetivoZ = Acotar(objetivoZ, -LIMITE_JUGADOR_Z, LIMITE_JUGADOR_Z);

    float dx = objetivoX - jugador.posicion.x;
    float dz = objetivoZ - jugador.posicion.z;

    entrada.derecha = dx > 0.18f;
    entrada.izquierda = dx < -0.18f;
    entrada.atras = dz > 0.18f;
    entrada.adelante = dz < -0.18f;

    bool listo = false;

    if (responsable)
    {
        float hx = pelota.posicion.x - jugador.posicion.x;
        float hz = pelota.posicion.z - jugador.posicion.z;
        float distanciaH = std::sqrt(hx * hx + hz * hz);
        float relativa = pelota.posicion.y - PiesJugador(jugador);
        float alcance = AlcanceHorizontal(m, estado.equipo);

        bool bajando = pelota.estado == PELOTA_VOLEA_SAQUE || pelota.velocidad.y <= 1.5f;
        listo = PelotaAlAlcance(m, estado.equipo, jugador) && distanciaH < alcance * 0.85f && bajando;

        // Pelota demasiado alta para alcanzarla de pie: salta.
        if (
            !listo &&
            jugador.enSuelo &&
            distanciaH < 1.5f &&
            relativa > ALCANCE_VERTICAL_MAXIMO &&
            relativa < 4.3f
        )
        {
            entrada.saltar = true;
        }
    }

    if (listo && estado.recargaGolpe <= 0.0f)
    {
        if (estado.retardoGolpe < 0.0f)
        {
            estado.retardoGolpe = 0.15f + 0.2f * Aleatorio01();
            if (Aleatorio01() < 0.12f) estado.retardoGolpe += 1.0f; // golpe tardio (fallo)
        }
        else
        {
            estado.retardoGolpe -= deltaTime;
        }

        if (estado.retardoGolpe <= 0.0f)
        {
            entrada.golpear = true;
            estado.retardoGolpe = -1.0f;
        }
    }
    else
    {
        estado.retardoGolpe = -1.0f;
    }

    return entrada;
}


//==================================================
// PELOTA, PUNTOS Y CHARCOS
//==================================================

static void AgregarCharco(MinijuegoVoleaMagma& m, float x, float z)
{
    int elegido = 0;
    float menorTiempo = 100000.0f;

    // Reutiliza el hueco libre o el charco mas viejo.
    for (int i = 0; i < MAX_CHARCOS_VOLEA; i++)
    {
        if (!m.charcos[i].activo)
        {
            elegido = i;
            menorTiempo = -1.0f;
            break;
        }

        float restante = DURACION_CHARCO - m.charcos[i].tiempo;

        if (restante < menorTiempo)
        {
            menorTiempo = restante;
            elegido = i;
        }
    }

    m.charcos[elegido].activo = true;
    m.charcos[elegido].x = x;
    m.charcos[elegido].z = z;
    m.charcos[elegido].tiempo = 0.0f;
}


static void AnotarPunto(
    MinijuegoVoleaMagma& m,
    int ganador,
    float aterrizajeX,
    float aterrizajeZ,
    bool dentro,
    const Participante participantes[],
    int limite,
    SonidosFrameVolea& sonidos
)
{
    m.puntos[ganador]++;
    m.puntosTotales++;
    m.pelota.estado = PELOTA_VOLEA_PUNTO;
    m.pelota.tiempoEstado = 0.0f;
    m.pelota.velocidad = {};
    m.pelota.posicion.y = RADIO_PELOTA;
    m.mensaje = MENSAJE_VOLEA_PUNTO;
    m.equipoMensaje = ganador;
    m.tiempoMensaje = PAUSA_PUNTO_VOLEA;

    for (int i = 0; i < limite; i++)
    {
        int equipo = m.estadosJugadores[i].equipo;

        if (equipo < 0 || JugadorEsBot(participantes[i]))
        {
            continue;
        }

        if (equipo == ganador) sonidos.anota = true;
        else sonidos.recibe = true;
    }

    // Roca sobrecalentada: deja un charco de lava donde cayo.
    if (dentro && m.pelota.temperatura >= TEMPERATURA_SOBRECALENTADA)
    {
        AgregarCharco(
            m,
            Acotar(aterrizajeX, -MITAD_CANCHA_X + 1.0f, MITAD_CANCHA_X - 1.0f),
            Acotar(aterrizajeZ, -MITAD_CANCHA_Z + 1.0f, MITAD_CANCHA_Z - 1.0f)
        );
        m.mensaje = MENSAJE_VOLEA_CHARCO;
    }

    // Cada 3 puntos la roca se enfria un tono y luego vuelve a calentarse.
    if (m.puntosTotales % 3 == 0)
    {
        m.pelota.temperatura = Acotar(m.pelota.temperatura - 0.45f, 0.05f, 1.0f);
    }

    if (m.modoOro || m.puntos[ganador] >= PUNTOS_PARA_GANAR_VOLEA)
    {
        FinalizarVolea(m);
    }
}


static void ActualizarPelotaVolea(
    MinijuegoVoleaMagma& m,
    float deltaTime,
    const Participante participantes[],
    int limite,
    SonidosFrameVolea& sonidos
)
{
    PelotaVolea& pelota = m.pelota;

    pelota.enfriamientoToque -= deltaTime;
    pelota.tiempoEstado += deltaTime;

    // Estela para dar lectura a la trayectoria.
    pelota.tiempoEstela += deltaTime;

    if (pelota.tiempoEstela >= 0.03f)
    {
        pelota.tiempoEstela = 0.0f;

        for (int k = LARGO_ESTELA_VOLEA - 1; k > 0; k--)
        {
            pelota.estela[k] = pelota.estela[k - 1];
        }

        pelota.estela[0] = pelota.posicion;
    }

    if (pelota.estado == PELOTA_VOLEA_SAQUE)
    {
        pelota.posicion.y = 2.4f + 0.1f * std::sin(m.tiempoAnimacion * 3.0f);
        pelota.velocidad = {};

        // Si nadie saca a tiempo, se suelta y cae en la mitad del sacador.
        if (pelota.tiempoEstado >= ESPERA_SAQUE_VOLEA)
        {
            pelota.estado = PELOTA_VOLEA_EN_JUEGO;
        }

        return;
    }

    if (pelota.estado == PELOTA_VOLEA_PUNTO)
    {
        return;
    }

    // Se recalienta mientras esta en juego.
    pelota.temperatura = Acotar(pelota.temperatura + deltaTime / 16.0f, 0.0f, 1.0f);

    const int subpasos = 3;
    float h = deltaTime / (float)subpasos;

    for (int paso = 0; paso < subpasos; paso++)
    {
        float xAnterior = pelota.posicion.x;

        pelota.velocidad.y -= GRAVEDAD_PELOTA * h;
        pelota.posicion.x += pelota.velocidad.x * h;
        pelota.posicion.y += pelota.velocidad.y * h;
        pelota.posicion.z += pelota.velocidad.z * h;

        // Cruce de la red.
        if ((xAnterior < 0.0f) != (pelota.posicion.x < 0.0f))
        {
            if (pelota.posicion.y < ALTURA_RED + RADIO_PELOTA)
            {
                pelota.posicion.x = xAnterior < 0.0f ? -0.05f : 0.05f;
                pelota.velocidad.x = -pelota.velocidad.x * 0.35f;
                pelota.velocidad.y *= 0.8f;

                if (pelota.enfriamientoToque <= 0.0f)
                {
                    sonidos.impacto = true;
                    pelota.enfriamientoToque = 0.1f;
                }
            }
            else
            {
                // La pelota entra en la mitad contraria: sus toques empiezan de cero.
                m.toques[pelota.posicion.x < 0.0f ? 0 : 1] = 0;
            }
        }

        // Caida al suelo: fin del punto.
        if (pelota.posicion.y <= RADIO_PELOTA && pelota.velocidad.y < 0.0f)
        {
            float x = pelota.posicion.x;
            float z = pelota.posicion.z;
            bool dentro =
                std::fabs(x) <= MITAD_CANCHA_X &&
                std::fabs(z) <= MITAD_CANCHA_Z;
            int lado = x < 0.0f ? 0 : 1;
            int ganador = 1 - lado;

            // Fuera de la cancha pierde quien la toco por ultima vez.
            if (!dentro && m.equipoUltimoToque >= 0)
            {
                ganador = 1 - m.equipoUltimoToque;
            }

            pelota.posicion.y = RADIO_PELOTA;
            AnotarPunto(m, ganador, x, z, dentro, participantes, limite, sonidos);
            return;
        }
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoVoleaMagma::ActualizarPartida(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
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

    if (fase == FASE_VOLEA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_VOLEA_JUGANDO;
        }

        // Durante la cuenta regresiva todos esperan quietos.
        return;
    }

    if (fase != FASE_VOLEA_JUGANDO || !partidaValida)
    {
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;

        if (!modoOro && puntos[0] == puntos[1])
        {
            // Empate al final del tiempo: punto de oro con tope de 20 s.
            modoOro = true;
            tiempoRestante = DURACION_PUNTO_ORO_VOLEA;
            mensaje = MENSAJE_VOLEA_ORO;
            tiempoMensaje = 2.5f;
        }
        else
        {
            FinalizarVolea(*this);
            return;
        }
    }

    if (tiempoMensaje > 0.0f) tiempoMensaje -= deltaTime;
    if (avisoToques > 0.0f) avisoToques -= deltaTime;

    SonidosFrameVolea sonidos;

    // Charcos de lava: envejecen y ralentizan a quien los pisa.
    for (int c = 0; c < MAX_CHARCOS_VOLEA; c++)
    {
        if (!charcos[c].activo)
        {
            continue;
        }

        charcos[c].tiempo += deltaTime;

        if (charcos[c].tiempo >= DURACION_CHARCO)
        {
            charcos[c].activo = false;
        }
    }

    // Jugadores: entrada, fisica estandar y limites de su mitad.
    InputMinijuegoParticipante entradas[MAX_PARTICIPANTES]{};

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorVolea& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        bool esBot = JugadorEsBot(participantes[i]);

        if (estado.recargaGolpe > 0.0f) estado.recargaGolpe -= deltaTime;
        if (estado.tiempoSwing > 0.0f) estado.tiempoSwing -= deltaTime;

        InputMinijuegoParticipante entrada{};

        if (esBot)
        {
            entrada = CrearEntradaBotVolea(*this, i, jugadores, limite, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        entradas[i] = entrada;

        // El golpe lo gestiona esta logica; la fisica solo mueve y salta.
        // Sin salto en el aire para que no se active el ground pound.
        InputMinijuegoParticipante entradaFisica = entrada;
        entradaFisica.golpear = false;

        if (!jugador.enSuelo)
        {
            entradaFisica.saltar = false;
        }

        // Charco de lava en su mitad: ralentiza.
        for (int c = 0; c < MAX_CHARCOS_VOLEA; c++)
        {
            if (!charcos[c].activo)
            {
                continue;
            }

            float dx = jugador.posicion.x - charcos[c].x;
            float dz = jugador.posicion.z - charcos[c].z;

            if (dx * dx + dz * dz < RADIO_CHARCO * RADIO_CHARCO && jugador.enSuelo)
            {
                jugador.tiempoRalentizado = 0.12f;
                jugador.multiplicadorRalentizacion = MULTIPLICADOR_CHARCO;
            }
        }

        // La minoria (2 vs 1, 1 vs 1 no) corre algo mas.
        jugador.velocidadMovimiento =
            VELOCIDAD_JUGADOR_ESTANDAR * (EsMinoria(*this, estado.equipo) ? MULTIPLICADOR_MINORIA : 1.0f);

        ActualizarJugadorPruebaNormal(
            jugador,
            entradaFisica,
            bloques,
            1,
            particulas,
            MAX_PARTICULAS_VOLEA,
            true,
            false,
            deltaTime
        );

        // Cada jugador queda en su mitad de la cancha.
        float signo = SignoMitad(estado.equipo);
        float minimoX = signo < 0.0f ? -LIMITE_JUGADOR_X : LIMITE_JUGADOR_RED;
        float maximoX = signo < 0.0f ? -LIMITE_JUGADOR_RED : LIMITE_JUGADOR_X;
        jugador.posicion.x = Acotar(jugador.posicion.x, minimoX, maximoX);
        jugador.posicion.z = Acotar(jugador.posicion.z, -LIMITE_JUGADOR_Z, LIMITE_JUGADOR_Z);
        jugador.cayendo = false;

        if (jugador.posicion.y < ALTURA_CENTRO_JUGADOR - 0.5f)
        {
            jugador.posicion.y = ALTURA_CENTRO_JUGADOR;
            jugador.velocidad.y = 0.0f;
        }
    }

    // Separacion entre companeros.
    for (int i = 0; i < limite; i++)
    {
        for (int j = i + 1; j < limite; j++)
        {
            if (
                estadosJugadores[i].equipo < 0 ||
                estadosJugadores[i].equipo != estadosJugadores[j].equipo
            )
            {
                continue;
            }

            float dx = jugadores[j].posicion.x - jugadores[i].posicion.x;
            float dz = jugadores[j].posicion.z - jugadores[i].posicion.z;
            float distancia = std::sqrt(dx * dx + dz * dz);

            if (distancia < 0.9f)
            {
                float nx = distancia > 0.001f ? dx / distancia : 1.0f;
                float nz = distancia > 0.001f ? dz / distancia : 0.0f;
                float empuje = (0.9f - distancia) * 0.5f;
                jugadores[i].posicion.x -= nx * empuje;
                jugadores[i].posicion.z -= nz * empuje;
                jugadores[j].posicion.x += nx * empuje;
                jugadores[j].posicion.z += nz * empuje;
            }
        }
    }

    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_VOLEA, deltaTime);

    // Golpes (flanco del boton para humanos; decision de la IA para bots).
    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorVolea& estado = estadosJugadores[i];

        if (estado.equipo < 0 || !entradas[i].golpear || estado.recargaGolpe > 0.0f)
        {
            continue;
        }

        ResolverGolpeJugador(
            *this,
            i,
            jugadores[i],
            entradas[i],
            JugadorEsBot(participantes[i]),
            sonidos
        );
    }

    ActualizarPelotaVolea(*this, deltaTime, participantes, limite, sonidos);

    // Pausa entre puntos y siguiente saque.
    if (
        resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO &&
        pelota.estado == PELOTA_VOLEA_PUNTO &&
        pelota.tiempoEstado >= PAUSA_PUNTO_VOLEA
    )
    {
        int ganadorPunto = equipoMensaje >= 0 ? equipoMensaje : 0;
        bool enfrio = puntosTotales > 0 && puntosTotales % 3 == 0;

        IniciarSaque(*this, ganadorPunto);

        if (enfrio)
        {
            mensaje = MENSAJE_VOLEA_ENFRIA;
            tiempoMensaje = 1.6f;
        }
    }

    // Un sonido de cada tipo por frame.
    if (sonidos.remate)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_DISPARO);
    }
    else if (sonidos.impacto)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_IMPACTO);
    }

    if (sonidos.anota)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
    }

    if (sonidos.recibe)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ERROR);
    }
}


//==================================================
// VISUAL: ESTADIO VOLCANICO DE NOCHE
//==================================================
// Cancha de obsidiana sobre una isla de basalto en medio de un lago de
// lava; detras, gradas talladas en la roca con publico de piedra y el
// marcador de lava; al fondo, volcanes en dos planos. La logica (cancha,
// red, reglas de toques, puntos, IA) no depende de esta decoracion. Todo el
// estadio se arma con primitivas agrupadas por rlgl; solo las piezas
// jugables (red, postes, roca, charcos, sombra, indicador y estela) usan
// sus GLB, con su fallback por pieza.
//==================================================

static Color ColorRoca(float temperatura, float t)
{
    if (temperatura >= TEMPERATURA_SOBRECALENTADA)
    {
        float pulso = 0.5f + 0.5f * std::sin(t * 10.0f);
        return Color{ 255, (unsigned char)(200 + 40 * pulso), (unsigned char)(70 + 60 * pulso), 255 };
    }

    float k = Acotar(temperatura / TEMPERATURA_SOBRECALENTADA, 0.0f, 1.0f);

    return Color
    {
        (unsigned char)(70 + 185 * k),
        (unsigned char)(58 + 62 * k),
        (unsigned char)(58 - 38 * k),
        255
    };
}


// Paleta: obsidiana y basalto frios, roca de grada tibia, lava como fuente
// de luz y el blanco hueso de las lineas. El acento (lo que hay que mirar)
// es la roca de magma y su marca de caida.
static const Color COLOR_CIELO_ALTO_VOLEA = { 18, 10, 28, 255 };
static const Color COLOR_CIELO_MEDIO_VOLEA = { 62, 20, 38, 255 };
static const Color COLOR_CIELO_BAJO_VOLEA = { 170, 62, 36, 255 };
static const Color COLOR_NIEBLA_VOLEA = { 122, 50, 46, 255 };
static const Color COLOR_OBSIDIANA_VOLEA = { 46, 42, 60, 255 };
static const Color COLOR_BASALTO_VOLEA = { 82, 72, 78, 255 };
static const Color COLOR_BASALTO_OSCURO_VOLEA = { 50, 43, 50, 255 };
static const Color COLOR_ROCA_GRADA_VOLEA = { 112, 86, 76, 255 };
static const Color COLOR_COSTRA_VOLEA = { 58, 20, 18, 255 };
static const Color COLOR_LAVA_VOLEA = { 255, 100, 22, 255 };
static const Color COLOR_LAVA_CLARA_VOLEA = { 255, 210, 100, 255 };
static const Color COLOR_LINEA_VOLEA = { 240, 230, 212, 255 };
static const Color COLOR_ORO_VOLEA = { 255, 204, 84, 255 };
static const Color COLOR_HIERRO_VOLEA = { 58, 50, 52, 255 };

// Isla: pasillo alrededor de la cancha (un escalon abajo) y la base de las
// gradas. La plataforma de la cancha coincide con el bloque de colision.
static const float Y_PASILLO_VOLEA = -0.18f;
static const float Y_LAVA_VOLEA = -0.95f;
static const float PASILLO_X_VOLEA = 12.6f;
static const float PASILLO_Z_FRENTE_VOLEA = 10.0f;
static const float PASILLO_Z_FONDO_VOLEA = -10.6f;
static const float GRADA_X_VOLEA = 16.0f;
static const float GRADA_Z_FONDO_VOLEA = -20.6f;
static const float PLATAFORMA_X_VOLEA = MITAD_CANCHA_X + 1.0f;
static const float PLATAFORMA_Z_VOLEA = MITAD_CANCHA_Z + 1.0f;

static const int FILAS_GRADA_VOLEA = 5;
static const float FONDO_FILA_VOLEA = 1.75f;
static const float ALTO_FILA_VOLEA = 1.1f;
static const float MEDIO_PALCO_VOLEA = 4.6f;
static const float Z_MARCADOR_VOLEA = -13.3f;

static const float DURACION_ANUNCIO_VOLEA = 1.8f;

// Ojo de la niebla (la camara de juego) y ejes de los carteles que miran a
// camara; Dibujar los ajusta a la vista del frame.
static Vector3 g_ojoVolea = { 0.0f, 8.6f, 16.6f };
static Vector3 g_frenteUVolea = { 1.0f, 0.0f, 0.0f };
static Vector3 g_frenteVVolea = { 0.0f, 0.95f, -0.31f };


//==================================================
// VISUAL: COLOR, NIEBLA Y PRIMITIVAS
//==================================================

static float Limitar01Volea(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static unsigned char CanalVolea(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoVolea(Color color, float factor)
{
    return Color{
        CanalVolea((float)color.r * factor),
        CanalVolea((float)color.g * factor),
        CanalVolea((float)color.b * factor),
        color.a
    };
}


static Color MezclaVolea(Color a, Color b, float t)
{
    t = Limitar01Volea(t);

    return Color{
        CanalVolea((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalVolea((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalVolea((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalVolea((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaVolea(Color color, float alfa)
{
    color.a = CanalVolea(255.0f * alfa);
    return color;
}


static float RuidoVolea(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static Color ColorEquipoVisualVolea(int equipo)
{
    return ColorEquipoVolea(equipo < 0 ? 0 : equipo);
}


// Humo del volcan: lo lejano se funde con un rojo apagado.
static float NieblaVolea(Vector3 p)
{
    float dx = p.x - g_ojoVolea.x;
    float dy = p.y - g_ojoVolea.y;
    float dz = p.z - g_ojoVolea.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 24.0f) / 150.0f;
    return n < 0.0f ? 0.0f : (n > 0.88f ? 0.88f : n);
}


static Color TinteVolea(Color color, Vector3 p, float emisivo = 0.0f)
{
    Color niebla = COLOR_NIEBLA_VOLEA;
    niebla.a = color.a;
    return MezclaVolea(color, niebla, NieblaVolea(p) * (1.0f - 0.6f * emisivo));
}


static void VerticeVolea(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadVolea(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeVolea(a, color);
    VerticeVolea(b, color);
    VerticeVolea(c, color);
    VerticeVolea(d, color);
}


static void QuadNieblaVolea(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color, float emisivo = 0.0f)
{
    VerticeVolea(a, TinteVolea(color, a, emisivo));
    VerticeVolea(b, TinteVolea(color, b, emisivo));
    VerticeVolea(c, TinteVolea(color, c, emisivo));
    VerticeVolea(d, TinteVolea(color, d, emisivo));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.55). Respeta la matriz actual de rlgl.
static void CuboVolea(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadVolea({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, color);
    QuadVolea({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoVolea(color, 0.78f));
    QuadVolea({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoVolea(color, 0.55f));
    QuadVolea({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoVolea(color, 0.62f));
    QuadVolea({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoVolea(color, 0.70f));
    QuadVolea({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoVolea(color, 0.45f));
    rlEnd();
}


static void CuboNieblaVolea(Vector3 c, Vector3 t, Color color)
{
    CuboVolea(c, t, TinteVolea(color, c));
}


// Roca iluminada desde abajo por la lava: las caras se calientan hacia el
// pie y se enfrian hacia arriba.
static void CuboRocaVolea(Vector3 c, Vector3 t, Color color, float calorPie = 0.35f)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;
    Color arriba = TinteVolea(color, { c.x, y1, c.z });
    Color abajo = TinteVolea(MezclaVolea(color, Color{ 210, 80, 34, 255 }, calorPie), { c.x, y0, c.z }, 0.5f);

    rlBegin(RL_QUADS);
    QuadVolea({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, arriba);

    const float caras[4] = { 0.78f, 0.55f, 0.62f, 0.70f };
    Vector3 q[4][4] =
    {
        { { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 } },
        { { x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 } },
        { { x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 } },
        { { x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 } }
    };

    for (int k = 0; k < 4; k++)
    {
        Color b = TonoVolea(abajo, caras[k]);
        Color a = TonoVolea(arriba, caras[k]);
        VerticeVolea(q[k][0], b);
        VerticeVolea(q[k][1], b);
        VerticeVolea(q[k][2], a);
        VerticeVolea(q[k][3], a);
    }
    rlEnd();
}


// Caja girada alrededor de Y (grados).
static void CuboGiradoVolea(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboVolea({ 0.0f, 0.0f, 0.0f }, t, color);
    rlPopMatrix();
}


static Vector3 SumaVolea(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaVolea(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaVolea(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }


static Vector3 NormalizarVolea(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 0.0001f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static Vector3 CruzVolea(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}


// Luz de la escena: la luna de arriba y algo de frente.
static float LuzNormalVolea(Vector3 n)
{
    const float LX = -0.25f, LY = 0.84f, LZ = 0.48f;
    float d = n.x * LX + n.y * LY + n.z * LZ;
    return 0.56f + 0.46f * Limitar01Volea(d);
}


// Prisma (o tronco de cono) entre dos puntos. emisivo acerca todas las
// caras a la luz plena (llamas, lava).
static void PrismaVolea(
    Vector3 a,
    Vector3 b,
    float r0,
    float r1,
    int lados,
    Color color,
    float giro = 0.0f,
    float emisivo = 0.0f,
    bool niebla = false
)
{
    Vector3 eje = NormalizarVolea(RestaVolea(b, a));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarVolea(CruzVolea(eje, apoyo));
    Vector3 v = CruzVolea(eje, u);
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaVolea(EscalaVolea(u, std::cos(a0)), EscalaVolea(v, std::sin(a0)));
        Vector3 d1 = SumaVolea(EscalaVolea(u, std::cos(a1)), EscalaVolea(v, std::sin(a1)));
        Vector3 dm = SumaVolea(EscalaVolea(u, std::cos(am)), EscalaVolea(v, std::sin(am)));

        float f = LuzNormalVolea(dm);
        f = f + (1.0f - f) * emisivo;
        if (k & 1) f *= 0.94f;
        Color cara = TonoVolea(color, f);

        Vector3 p0 = SumaVolea(a, EscalaVolea(d0, r0));
        Vector3 p1 = SumaVolea(a, EscalaVolea(d1, r0));
        Vector3 p2 = SumaVolea(b, EscalaVolea(d1, r1));
        Vector3 p3 = SumaVolea(b, EscalaVolea(d0, r1));

        if (niebla) QuadNieblaVolea(p0, p1, p2, p3, cara, emisivo);
        else QuadVolea(p0, p1, p2, p3, cara);

        if (r1 > 0.001f)
        {
            Color tapa = TonoVolea(color, LuzNormalVolea(eje) + (1.0f - LuzNormalVolea(eje)) * emisivo);
            if (niebla) tapa = TinteVolea(tapa, b, emisivo);
            QuadVolea(b, p2, p3, b, tapa);
        }
    }

    rlEnd();
}


// Roca facetada: esfera de pocos lados con radios irregulares.
static void RocaVolea(Vector3 c, Vector3 r, int semilla, Color color, float irregular = 0.14f, bool niebla = true, float emisivo = 0.0f)
{
    const int ANILLOS = 4;
    const int LADOS = 7;

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
                float f = (aa[q] == 0 || aa[q] == ANILLOS)
                    ? 1.0f - irregular * 0.3f
                    : 1.0f - irregular + 2.0f * irregular * RuidoVolea(aa[q] * 31 + (kk[q] % LADOS), semilla);
                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = { c.x + n.x * r.x * f, c.y + n.y * r.y * f, c.z + n.z * r.z * f };
                float luz = LuzNormalVolea(n);
                luz = luz + (1.0f - luz) * emisivo;
                Color cv = TonoVolea(color, luz * (0.94f + 0.12f * RuidoVolea(aa[q] * 7 + kk[q], semilla + 5)));
                VerticeVolea(p, niebla ? TinteVolea(cv, p, emisivo) : cv);
            }
        }
    }

    rlEnd();
}


// Disco o corona horizontal.
static void AnilloVolea(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadVolea(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorVolea(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloVolea(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorVolea(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


static void ResplandorFrenteVolea(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorVolea(c, g_frenteUVolea, g_frenteVVolea, radio, color, estirado);
}


// Cono de luz (aditivo): del foco hacia el charco, se apaga en la base.
static void ConoLuzVolea(Vector3 foco, Vector3 baseCentro, float radio, Color color)
{
    const int LADOS = 16;
    Vector3 eje = NormalizarVolea(RestaVolea(baseCentro, foco));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarVolea(CruzVolea(eje, apoyo));
    Vector3 v = CruzVolea(eje, u);

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaVolea(baseCentro, SumaVolea(EscalaVolea(u, std::cos(a0) * radio), EscalaVolea(v, std::sin(a0) * radio)));
        Vector3 p1 = SumaVolea(baseCentro, SumaVolea(EscalaVolea(u, std::cos(a1) * radio), EscalaVolea(v, std::sin(a1) * radio)));

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(foco.x, foco.y, foco.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p1.x, p1.y, p1.z);
    }
    rlEnd();
}


// Linea gruesa horizontal sobre el suelo (pintura, juntas, grietas).
static void TrazoSueloVolea(float xa, float za, float xb, float zb, float y, float ancho, Color color)
{
    float dx = xb - xa, dz = zb - za;
    float l = std::sqrt(dx * dx + dz * dz);
    if (l < 0.0001f) return;
    float nx = -dz / l * ancho * 0.5f, nz = dx / l * ancho * 0.5f;

    rlBegin(RL_QUADS);
    QuadVolea({ xa + nx, y, za + nz }, { xb + nx, y, zb + nz }, { xb - nx, y, zb - nz }, { xa - nx, y, za - nz }, color);
    rlEnd();
}


//==================================================
// VISUAL: CHISPAS Y DETECCION DE EVENTOS
//==================================================

static float AzarVisualVolea(MinijuegoVoleaMagma& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadChispasVolea(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return n < 1 ? 1 : n;
}


static void EmitirChispaVolea(
    MinijuegoVoleaMagma& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool brilla,
    bool humo = false
)
{
    int indice = 0;
    float menor = 1000.0f;

    for (int i = 0; i < MAX_CHISPAS_VOLEA; i++)
    {
        if (!m.chispas[i].activa)
        {
            indice = i;
            break;
        }

        if (m.chispas[i].vida < menor)
        {
            menor = m.chispas[i].vida;
            indice = i;
        }
    }

    ChispaVolea& c = m.chispas[indice];
    c.activa = true;
    c.posicion = posicion;
    c.velocidad = velocidad;
    c.color = color;
    c.vida = vida;
    c.vidaMaxima = vida;
    c.tamano = tamano;
    c.gravedad = gravedad;
    c.giro = AzarVisualVolea(m, 0.0f, 360.0f);
    c.brilla = brilla;
    c.humo = humo;
}


// Golpe: chispas blancas y del color del equipo donde estaba la roca.
static void EfectoGolpeVolea(MinijuegoVoleaMagma& m, int jugador, int equipo, bool remate)
{
    Vector3 p = m.pelota.posicion;
    Color colorEquipo = MezclaVolea(ColorEquipoVisualVolea(equipo), WHITE, 0.25f);
    int cantidad = CantidadChispasVolea(remate ? 22 : 12);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualVolea(m, 0.0f, 6.2832f);
        float v = AzarVisualVolea(m, 1.5f, remate ? 6.0f : 4.0f);
        Vector3 vel = { std::cos(a) * v, AzarVisualVolea(m, -0.5f, 3.5f), std::sin(a) * v };
        Color c = k % 3 == 0 ? colorEquipo : (k % 3 == 1 ? COLOR_LAVA_CLARA_VOLEA : COLOR_LAVA_VOLEA);
        EmitirChispaVolea(m, p, vel, c, AzarVisualVolea(m, 0.3f, 0.6f), AzarVisualVolea(m, 0.06f, 0.12f), 9.0f, true);
    }

    m.destelloGolpe = remate ? 1.0f : 0.7f;
    m.posicionGolpe = p;

    if (jugador >= 0 && jugador < MAX_PARTICIPANTES)
    {
        m.destelloJugador[jugador] = 1.0f;
    }

    if (remate)
    {
        ActivarTemblorCamaraGeneral(0.1f, 0.18f);
    }
}


// La roca pega en la red de cadenas: chispas y la red se sacude.
static void EfectoRedVolea(MinijuegoVoleaMagma& m)
{
    Vector3 p = { 0.0f, m.pelota.posicion.y, m.pelota.posicion.z };
    int cantidad = CantidadChispasVolea(10);

    for (int k = 0; k < cantidad; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        Vector3 vel = { lado * AzarVisualVolea(m, 1.0f, 3.0f), AzarVisualVolea(m, 0.5f, 3.0f), AzarVisualVolea(m, -2.0f, 2.0f) };
        EmitirChispaVolea(m, p, vel, Color{ 255, 220, 150, 255 }, AzarVisualVolea(m, 0.25f, 0.5f), 0.06f, 10.0f, true);
    }

    m.sacudidaRed = 1.0f;
}


// La roca cae: estallido de lava, esquirlas de roca, marca quemada y el
// publico del equipo que anota festeja.
static void EfectoPuntoVolea(MinijuegoVoleaMagma& m, int ganador, bool charco)
{
    Vector3 p = { m.pelota.posicion.x, 0.15f, m.pelota.posicion.z };
    int cantidad = CantidadChispasVolea(charco ? 34 : 24);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualVolea(m, 0.0f, 6.2832f);
        float v = AzarVisualVolea(m, 1.2f, 4.5f);
        Vector3 vel = { std::cos(a) * v, AzarVisualVolea(m, 2.5f, 6.5f), std::sin(a) * v };
        bool roca = k % 4 == 3;
        Color c = roca ? Color{ 74, 62, 66, 255 } : (k % 2 == 0 ? COLOR_LAVA_VOLEA : COLOR_LAVA_CLARA_VOLEA);
        EmitirChispaVolea(m, p, vel, c, AzarVisualVolea(m, 0.5f, 0.95f), AzarVisualVolea(m, 0.08f, 0.17f), 13.0f, !roca);
    }

    // Humo del impacto.
    int humo = CantidadChispasVolea(6);
    for (int k = 0; k < humo; k++)
    {
        float a = AzarVisualVolea(m, 0.0f, 6.2832f);
        Vector3 vel = { std::cos(a) * 0.6f, AzarVisualVolea(m, 0.8f, 1.6f), std::sin(a) * 0.6f };
        EmitirChispaVolea(m, { p.x, 0.4f, p.z }, vel, Color{ 70, 60, 62, 255 }, AzarVisualVolea(m, 1.0f, 1.6f), AzarVisualVolea(m, 0.5f, 0.8f), -0.4f, false, true);
    }

    m.tiempoImpacto = 0.0f;
    m.posicionImpacto = { m.pelota.posicion.x, 0.0f, m.pelota.posicion.z };

    if (ganador >= 0 && ganador < 2)
    {
        m.festejo[ganador] = 1.0f;
        m.pulsoMarcador[ganador] = 1.0f;
    }

    ActivarTemblorCamaraGeneral(0.12f, 0.22f);
}


// La roca se enfria: nube de vapor que sube.
static void EfectoEnfriaVolea(MinijuegoVoleaMagma& m)
{
    Vector3 p = m.pelota.posicion;
    int cantidad = CantidadChispasVolea(10);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualVolea(m, 0.0f, 6.2832f);
        Vector3 vel = { std::cos(a) * 0.8f, AzarVisualVolea(m, 1.0f, 2.2f), std::sin(a) * 0.8f };
        EmitirChispaVolea(m, p, vel, Color{ 220, 226, 236, 255 }, AzarVisualVolea(m, 0.9f, 1.5f), AzarVisualVolea(m, 0.35f, 0.6f), -0.5f, false, true);
    }
}


static void EmitirConfetiVolea(MinijuegoVoleaMagma& m, const JugadorPrueba jugadores[], int limite)
{
    int cantidad = CantidadChispasVolea(36);

    for (int i = 0; i < limite; i++)
    {
        int equipo = m.estadosJugadores[i].equipo;

        if (equipo < 0 || (!m.empate && equipo != m.equipoGanador))
        {
            continue;
        }

        Color colorEquipo = ColorEquipoVisualVolea(equipo);

        for (int k = 0; k < cantidad; k++)
        {
            float a = AzarVisualVolea(m, 0.0f, 6.2832f);
            float v = AzarVisualVolea(m, 0.5f, 2.6f);
            Vector3 p = { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.6f, jugadores[i].posicion.z };
            Vector3 vel = { std::cos(a) * v, AzarVisualVolea(m, 3.0f, 6.5f), std::sin(a) * v };
            Color c = k % 3 == 0 ? colorEquipo : (k % 3 == 1 ? COLOR_ORO_VOLEA : Color{ 244, 240, 236, 255 });
            EmitirChispaVolea(m, p, vel, c, AzarVisualVolea(m, 1.4f, 2.4f), AzarVisualVolea(m, 0.08f, 0.14f), 3.2f, false);
        }
    }
}


// Foto de la partida antes de un paso: comparandola con la de despues se
// sabe que paso (golpe, red, punto, enfriamiento) sin tocar las reglas.
struct InstantaneaVolea
{
    FaseVoleaMagma fase = FASE_VOLEA_PREPARACION;
    int puntos[2]{};
    int toques[2]{};
    Vector3 posicionPelota{};
    Vector3 velocidadPelota{};
    EstadoPelotaVolea estadoPelota = PELOTA_VOLEA_SAQUE;
    float temperatura = 0.0f;
    bool charco[MAX_CHARCOS_VOLEA]{};
};


static void TomarInstantaneaVolea(const MinijuegoVoleaMagma& m, InstantaneaVolea& s)
{
    s.fase = m.fase;
    s.puntos[0] = m.puntos[0];
    s.puntos[1] = m.puntos[1];
    s.toques[0] = m.toques[0];
    s.toques[1] = m.toques[1];
    s.posicionPelota = m.pelota.posicion;
    s.velocidadPelota = m.pelota.velocidad;
    s.estadoPelota = m.pelota.estado;
    s.temperatura = m.pelota.temperatura;

    for (int c = 0; c < MAX_CHARCOS_VOLEA; c++)
    {
        s.charco[c] = m.charcos[c].activo && m.charcos[c].tiempo < 0.05f;
    }
}


static void AmortiguarVolea(float& valor, float velocidad, float deltaTime)
{
    if (valor > 0.0f)
    {
        valor -= deltaTime * velocidad;
        if (valor < 0.0f) valor = 0.0f;
    }
}


static void ActualizarVisualVolea(
    MinijuegoVoleaMagma& m,
    const InstantaneaVolea& s,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int limite
)
{
    m.tiempoVisual += deltaTime;

    // Golpes: cuenta de toques que sube.
    for (int e = 0; e < 2; e++)
    {
        if (m.toques[e] > s.toques[e])
        {
            int j = m.ultimoJugador;
            bool remate = j >= 0 && j < limite && !jugadores[j].enSuelo;
            EfectoGolpeVolea(m, j, e, remate);
        }
    }

    // La red devuelve la roca.
    if (
        s.estadoPelota == PELOTA_VOLEA_EN_JUEGO &&
        m.pelota.estado == PELOTA_VOLEA_EN_JUEGO &&
        s.velocidadPelota.x * m.pelota.velocidad.x < 0.0f &&
        std::fabs(m.pelota.posicion.x) <= 0.06f
    )
    {
        EfectoRedVolea(m);
    }

    // Punto.
    for (int e = 0; e < 2; e++)
    {
        if (m.puntos[e] > s.puntos[e])
        {
            bool charcoNuevo = false;

            for (int c = 0; c < MAX_CHARCOS_VOLEA; c++)
            {
                if (m.charcos[c].activo && m.charcos[c].tiempo < 0.05f && !s.charco[c])
                {
                    charcoNuevo = true;
                }
            }

            EfectoPuntoVolea(m, e, charcoNuevo);
        }
    }

    // Enfriamiento (cada tres puntos).
    if (m.pelota.temperatura < s.temperatura - 0.2f)
    {
        EfectoEnfriaVolea(m);
    }

    // Brasas que suelta la roca en vuelo, mas cuanto mas caliente.
    if (m.pelota.estado == PELOTA_VOLEA_EN_JUEGO && m.fase == FASE_VOLEA_JUGANDO)
    {
        float tasa = (0.25f + 0.75f * m.pelota.temperatura) * FactorCalidadGrafica(CalidadParticulas());

        if (AzarVisualVolea(m, 0.0f, 1.0f) < tasa)
        {
            Vector3 p = m.pelota.posicion;
            Vector3 vel = { AzarVisualVolea(m, -0.6f, 0.6f), AzarVisualVolea(m, 0.2f, 1.2f), AzarVisualVolea(m, -0.6f, 0.6f) };
            Color c = m.pelota.temperatura >= TEMPERATURA_SOBRECALENTADA ? COLOR_LAVA_CLARA_VOLEA : COLOR_LAVA_VOLEA;
            EmitirChispaVolea(m, p, vel, c, AzarVisualVolea(m, 0.3f, 0.6f), AzarVisualVolea(m, 0.05f, 0.09f), 1.5f, true);
        }
    }

    // Anuncio de inicio y papelitos del final.
    if (s.fase == FASE_VOLEA_PREPARACION && m.fase == FASE_VOLEA_JUGANDO)
    {
        m.tiempoAnuncioInicio = 0.0f;
    }
    else if (m.tiempoAnuncioInicio >= 0.0f)
    {
        m.tiempoAnuncioInicio += deltaTime;
        if (m.tiempoAnuncioInicio > DURACION_ANUNCIO_VOLEA) m.tiempoAnuncioInicio = -1.0f;
    }

    if (m.fase == FASE_VOLEA_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        float antes = m.tiempoFin;
        m.tiempoFin = m.tiempoFin < 0.0f ? 0.0f : m.tiempoFin + deltaTime;

        if (antes < 0.0f || (antes < 0.8f && m.tiempoFin >= 0.8f))
        {
            EmitirConfetiVolea(m, jugadores, limite);
        }

        if (!m.empate && m.equipoGanador >= 0)
        {
            m.festejo[m.equipoGanador] = 1.0f;
        }
    }

    if (m.tiempoImpacto >= 0.0f)
    {
        m.tiempoImpacto += deltaTime;
        if (m.tiempoImpacto > 3.0f) m.tiempoImpacto = -1.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        AmortiguarVolea(m.destelloJugador[i], 3.0f, deltaTime);
    }

    AmortiguarVolea(m.festejo[0], 0.4f, deltaTime);
    AmortiguarVolea(m.festejo[1], 0.4f, deltaTime);
    AmortiguarVolea(m.pulsoMarcador[0], 1.2f, deltaTime);
    AmortiguarVolea(m.pulsoMarcador[1], 1.2f, deltaTime);
    AmortiguarVolea(m.sacudidaRed, 2.5f, deltaTime);
    AmortiguarVolea(m.destelloGolpe, 4.0f, deltaTime);

    for (int i = 0; i < MAX_CHISPAS_VOLEA; i++)
    {
        ChispaVolea& c = m.chispas[i];
        if (!c.activa) continue;

        c.vida -= deltaTime;
        if (c.vida <= 0.0f)
        {
            c.activa = false;
            continue;
        }

        c.velocidad.y -= c.gravedad * deltaTime;
        c.posicion.x += c.velocidad.x * deltaTime;
        c.posicion.y += c.velocidad.y * deltaTime;
        c.posicion.z += c.velocidad.z * deltaTime;
        c.giro += 240.0f * deltaTime;

        if (c.humo)
        {
            c.velocidad.x *= 1.0f - 0.8f * deltaTime;
            c.velocidad.z *= 1.0f - 0.8f * deltaTime;
        }
        else if (c.posicion.y < 0.04f)
        {
            c.posicion.y = 0.04f;
            c.velocidad.y = -c.velocidad.y * 0.3f;
            c.velocidad.x *= 0.6f;
            c.velocidad.z *= 0.6f;
        }
    }
}


void MinijuegoVoleaMagma::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f)
    {
        deltaTime = 0.05f;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    InstantaneaVolea antes;
    TomarInstantaneaVolea(*this, antes);

    ActualizarPartida(deltaTime, jugadores, cantidadMaxima, participantes);

    ActualizarVisualVolea(*this, antes, deltaTime, jugadores, limite);
}


//==================================================
// VISUAL: CIELO Y HORIZONTE
//==================================================

static float HorizontePantallaVolea(const Camera3D& vista)
{
    Vector2 h = GetWorldToScreen({ vista.position.x, vista.position.y, vista.position.z - 4000.0f }, vista);
    return h.y;
}


// Fondo 2D: noche de humo, roja cerca del horizonte por el brillo de la
// lava; estrellas entre la ceniza y nubes bajas iluminadas desde abajo.
static void DibujarCieloVolea(const Camera3D& vista, float t, bool decoracion)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float horizonte = HorizontePantallaVolea(vista);
    if (horizonte < (float)alto * 0.08f) horizonte = (float)alto * 0.08f;
    if (horizonte > (float)alto * 0.9f) horizonte = (float)alto * 0.9f;

    int corte = (int)(horizonte * 0.45f);
    DrawRectangleGradientV(0, 0, ancho, corte + 1, COLOR_CIELO_ALTO_VOLEA, COLOR_CIELO_MEDIO_VOLEA);
    DrawRectangleGradientV(0, corte, ancho, (int)horizonte - corte + 1, COLOR_CIELO_MEDIO_VOLEA, COLOR_CIELO_BAJO_VOLEA);
    DrawRectangle(0, (int)horizonte, ancho, alto - (int)horizonte, COLOR_NIEBLA_VOLEA);

    if (!decoracion)
    {
        return;
    }

    // Estrellas que titilan entre la ceniza.
    for (int k = 0; k < 70; k++)
    {
        float x = RuidoVolea(k, 401) * (float)ancho;
        float y = RuidoVolea(k, 402) * horizonte * 0.7f;
        float titila = 0.55f + 0.45f * std::sin(t * (1.3f + RuidoVolea(k, 403) * 2.0f) + (float)k);
        unsigned char a = CanalVolea(200.0f * titila * (1.0f - y / (horizonte * 0.8f)));
        DrawRectangle((int)x, (int)y, k % 7 == 0 ? 2 : 1, k % 7 == 0 ? 2 : 1, Color{ 255, 236, 220, a });
    }

    // Bancos de ceniza que derivan, con la panza roja por la lava.
    for (int k = 0; k < 9; k++)
    {
        float ancho0 = (float)ancho * (0.22f + 0.18f * RuidoVolea(k, 411));
        float deriva = std::fmod(RuidoVolea(k, 412) * (float)ancho * 1.4f + t * (4.0f + 5.0f * RuidoVolea(k, 413)), (float)ancho * 1.4f) - (float)ancho * 0.2f;
        float y = horizonte * (0.28f + 0.55f * RuidoVolea(k, 414));
        float altoNube = horizonte * (0.08f + 0.06f * RuidoVolea(k, 415));
        Color cuerpo = MezclaVolea(COLOR_CIELO_MEDIO_VOLEA, Color{ 40, 22, 30, 255 }, 0.4f);
        Color panza = MezclaVolea(COLOR_CIELO_BAJO_VOLEA, cuerpo, 0.35f);
        DrawEllipse((int)deriva, (int)y, ancho0 * 0.5f, altoNube, AlfaVolea(cuerpo, 0.55f));
        DrawEllipse((int)(deriva + ancho0 * 0.12f), (int)(y + altoNube * 0.45f), ancho0 * 0.42f, altoNube * 0.5f, AlfaVolea(panza, 0.45f));
    }
}


// Volcan: cono irregular con anillos; la actividad pinta coladas de lava y
// el crater encendido.
static void VolcanVolea(Vector3 base, float radio, float alto, int semilla, float t, float actividad, bool detalle)
{
    const int LADOS = detalle ? 22 : 12;
    const int ANILLOS = 5;
    Color roca = Color{ 64, 44, 46, 255 };
    Color cumbre = Color{ 84, 58, 56, 255 };

    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        for (int k = 0; k < LADOS; k++)
        {
            int aa[4] = { a, a, a + 1, a + 1 };
            int kk[4] = { k, k + 1, k + 1, k };
            bool colada = actividad > 0.0f && a >= 1 && RuidoVolea(k, semilla + 7) < 0.22f;

            for (int q = 0; q < 4; q++)
            {
                float h = (float)aa[q] / (float)ANILLOS;
                float ang = 6.2832f * (float)(kk[q] % LADOS) / (float)LADOS;
                float irregular = 1.0f + 0.16f * (RuidoVolea(aa[q] * 41 + kk[q] % LADOS, semilla) - 0.5f);
                float r = radio * (1.0f - 0.84f * std::pow(h, 0.75f)) * (a == ANILLOS - 1 && aa[q] == ANILLOS ? 1.0f : irregular);
                Vector3 p = { base.x + std::cos(ang) * r, base.y + alto * h, base.z + std::sin(ang) * r };
                float luz = 0.55f + 0.45f * Limitar01Volea(-std::sin(ang) * 0.6f + 0.5f);
                Color c = TonoVolea(MezclaVolea(roca, cumbre, h), luz);

                if (colada)
                {
                    float pulso = 0.75f + 0.25f * std::sin(t * 1.4f + h * 6.0f - (float)k);
                    c = MezclaVolea(c, TonoVolea(COLOR_LAVA_VOLEA, pulso), actividad * (0.55f + 0.45f * h));
                    VerticeVolea(p, TinteVolea(c, p, 0.8f));
                }
                else
                {
                    VerticeVolea(p, TinteVolea(c, p));
                }
            }
        }
    }

    rlEnd();

    // Crater: boca encendida.
    if (actividad > 0.0f)
    {
        float rCrater = radio * 0.16f;
        float pulso = 0.8f + 0.2f * std::sin(t * 2.2f + (float)semilla);
        Color boca = TinteVolea(MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, 0.3f * pulso), { base.x, base.y + alto, base.z }, 0.9f);
        AnilloVolea({ base.x, base.y + alto - 0.3f, base.z }, 0.0f, rCrater, boca, LADOS);
    }
}


// Cordillera lejana: dientes de sierra con dos tonos, se pierde en el humo.
static void CordilleraVolea(float z, float alturaBase, float alturaVariable, int semilla, Color color, int picos)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < picos; k++)
    {
        float x0 = -280.0f + 560.0f * (float)k / (float)picos;
        float x1 = -280.0f + 560.0f * (float)(k + 1) / (float)picos;
        float xm = (x0 + x1) * 0.5f + (RuidoVolea(k, semilla) - 0.5f) * (x1 - x0) * 0.6f;
        float h = alturaBase + alturaVariable * RuidoVolea(k, semilla + 1);
        float zk = z + (RuidoVolea(k, semilla + 2) - 0.5f) * 14.0f;
        Vector3 pie0 = { x0, Y_LAVA_VOLEA, zk };
        Vector3 pie1 = { x1, Y_LAVA_VOLEA, zk };
        Vector3 cima = { xm, Y_LAVA_VOLEA + h, zk };
        Color luz = TinteVolea(color, cima);
        Color sombra = TinteVolea(TonoVolea(color, 0.7f), cima);
        QuadVolea(pie0, { xm, Y_LAVA_VOLEA, zk }, cima, cima, luz);
        QuadVolea({ xm, Y_LAVA_VOLEA, zk }, pie1, cima, cima, sombra);
    }

    rlEnd();
}


static void DibujarHorizonteVolea(float t, bool decoracion)
{
    // Plano de atras (mas claro, perdido en el humo) y volcanes cercanos.
    CordilleraVolea(-215.0f, 26.0f, 30.0f, 501, Color{ 120, 70, 70, 255 }, decoracion ? 30 : 14);

    if (decoracion)
    {
        CordilleraVolea(-160.0f, 14.0f, 18.0f, 511, Color{ 80, 50, 54, 255 }, 26);
    }

    VolcanVolea({ -38.0f, Y_LAVA_VOLEA, -112.0f }, 58.0f, 46.0f, 521, t, 1.0f, decoracion);
    VolcanVolea({ 62.0f, Y_LAVA_VOLEA, -130.0f }, 50.0f, 34.0f, 531, t, decoracion ? 0.45f : 0.0f, decoracion);

    if (decoracion)
    {
        VolcanVolea({ 120.0f, Y_LAVA_VOLEA, -96.0f }, 34.0f, 20.0f, 541, t, 0.0f, true);
        VolcanVolea({ -118.0f, Y_LAVA_VOLEA, -86.0f }, 30.0f, 16.0f, 551, t, 0.0f, true);
    }
}


//==================================================
// VISUAL: LAGO DE LAVA E ISLA
//==================================================

// Costra oscura en placas que derivan y lava viva en las grietas.
static Color ColorLavaVolea(float x, float z, float t)
{
    float a = std::sin(x * 0.42f + t * 0.55f + std::sin(z * 0.31f - t * 0.3f) * 1.6f);
    float b = std::sin(z * 0.57f - t * 0.42f + std::sin(x * 0.23f + t * 0.2f) * 1.4f);
    float v = 0.5f + 0.25f * a + 0.25f * b;
    float brillo = Limitar01Volea((v - 0.36f) / 0.42f);
    Color c = MezclaVolea(COLOR_COSTRA_VOLEA, COLOR_LAVA_VOLEA, brillo);

    if (v > 0.8f)
    {
        c = MezclaVolea(c, COLOR_LAVA_CLARA_VOLEA, (v - 0.8f) / 0.2f);
    }

    return c;
}


static void DibujarLagoLavaVolea(float t, bool animada, int calidad)
{
    float tl = animada ? t : 0.0f;

    // Lejos: grandes paños que se apagan hacia el humo del horizonte.
    {
        const int N = 10;
        rlBegin(RL_QUADS);
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                float x0 = -300.0f + 600.0f * (float)i / (float)N, x1 = -300.0f + 600.0f * (float)(i + 1) / (float)N;
                float z0 = -300.0f + 330.0f * (float)j / (float)N, z1 = -300.0f + 330.0f * (float)(j + 1) / (float)N;
                Vector3 p[4] = { { x0, Y_LAVA_VOLEA - 0.05f, z1 }, { x1, Y_LAVA_VOLEA - 0.05f, z1 }, { x1, Y_LAVA_VOLEA - 0.05f, z0 }, { x0, Y_LAVA_VOLEA - 0.05f, z0 } };
                for (int q = 0; q < 4; q++)
                {
                    VerticeVolea(p[q], TinteVolea(ColorLavaVolea(p[q].x * 0.3f, p[q].z * 0.3f, tl * 0.5f), p[q], 0.35f));
                }
            }
        }
        rlEnd();
    }

    // Cerca: rejilla animada alrededor de la isla.
    float celda = calidad >= 2 ? 1.6f : (calidad == 1 ? 2.4f : 3.2f);
    int nx = (int)(96.0f / celda);
    int nz = (int)(70.0f / celda);

    rlBegin(RL_QUADS);
    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < nz; j++)
        {
            float x0 = -48.0f + celda * (float)i, x1 = x0 + celda;
            float z0 = -52.0f + celda * (float)j, z1 = z0 + celda;

            // Debajo de la isla no hace falta.
            if (x0 > -PASILLO_X_VOLEA && x1 < PASILLO_X_VOLEA && z0 > PASILLO_Z_FONDO_VOLEA && z1 < PASILLO_Z_FRENTE_VOLEA) continue;
            if (x0 > -GRADA_X_VOLEA && x1 < GRADA_X_VOLEA && z0 > GRADA_Z_FONDO_VOLEA && z1 < PASILLO_Z_FONDO_VOLEA) continue;

            Vector3 p[4] = { { x0, Y_LAVA_VOLEA, z1 }, { x1, Y_LAVA_VOLEA, z1 }, { x1, Y_LAVA_VOLEA, z0 }, { x0, Y_LAVA_VOLEA, z0 } };
            for (int q = 0; q < 4; q++)
            {
                VerticeVolea(p[q], TinteVolea(ColorLavaVolea(p[q].x, p[q].z, tl), p[q], 0.7f));
            }
        }
    }
    rlEnd();
}


// Borburjas de la lava: domos que crecen y revientan con un aro.
static void DibujarBurbujasLavaVolea(float t)
{
    for (int k = 0; k < 22; k++)
    {
        float x = -40.0f + 80.0f * RuidoVolea(k, 601);
        float z = -46.0f + 58.0f * RuidoVolea(k, 602);

        if (std::fabs(x) < PASILLO_X_VOLEA + 0.8f && z > PASILLO_Z_FONDO_VOLEA - 0.8f) continue;
        if (std::fabs(x) < GRADA_X_VOLEA + 0.8f && z > GRADA_Z_FONDO_VOLEA - 0.8f) continue;

        float ciclo = 2.2f + 2.0f * RuidoVolea(k, 603);
        float u = std::fmod(t / ciclo + RuidoVolea(k, 604), 1.0f);
        Vector3 c = { x, Y_LAVA_VOLEA, z };

        if (u < 0.75f)
        {
            float r = 0.15f + 0.45f * (u / 0.75f);
            RocaVolea({ x, Y_LAVA_VOLEA + r * 0.1f, z }, { r, r * 0.6f, r }, k, MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, u), 0.08f, true, 0.9f);
        }
        else
        {
            float v = (u - 0.75f) / 0.25f;
            AnilloVolea({ c.x, c.y + 0.02f, c.z }, 0.4f + 0.7f * v, 0.55f + 0.8f * v, TinteVolea(AlfaVolea(COLOR_LAVA_CLARA_VOLEA, 1.0f - v), c, 0.9f), 14);
        }
    }
}


// Isla de basalto: pasillo de losas, la base de las gradas y los
// acantilados que bajan a la lava.
static void DibujarIslaVolea(bool decoracion)
{
    // Lecho oscuro (las juntas entre losas).
    rlBegin(RL_QUADS);
    QuadNieblaVolea(
        { -PASILLO_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, PASILLO_Z_FRENTE_VOLEA },
        { PASILLO_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, PASILLO_Z_FRENTE_VOLEA },
        { PASILLO_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, PASILLO_Z_FONDO_VOLEA },
        { -PASILLO_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, PASILLO_Z_FONDO_VOLEA },
        Color{ 22, 18, 22, 255 }
    );
    QuadNieblaVolea(
        { -GRADA_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, PASILLO_Z_FONDO_VOLEA },
        { GRADA_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, PASILLO_Z_FONDO_VOLEA },
        { GRADA_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, GRADA_Z_FONDO_VOLEA },
        { -GRADA_X_VOLEA, Y_PASILLO_VOLEA - 0.02f, GRADA_Z_FONDO_VOLEA },
        Color{ 22, 18, 22, 255 }
    );

    // Losas del pasillo con brillo variable (+-10 %).
    const float LOSA = 1.3f;
    int nx = (int)(PASILLO_X_VOLEA * 2.0f / LOSA) + 1;
    int nz = (int)((PASILLO_Z_FRENTE_VOLEA - PASILLO_Z_FONDO_VOLEA) / LOSA) + 1;

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < nz; j++)
        {
            float x0 = -PASILLO_X_VOLEA + LOSA * (float)i + 0.04f;
            float z0 = PASILLO_Z_FONDO_VOLEA + LOSA * (float)j + 0.04f;
            float x1 = x0 + LOSA - 0.08f;
            float z1 = z0 + LOSA - 0.08f;
            if (x1 > PASILLO_X_VOLEA) x1 = PASILLO_X_VOLEA;
            if (z1 > PASILLO_Z_FRENTE_VOLEA) z1 = PASILLO_Z_FRENTE_VOLEA;
            if (x0 >= x1 || z0 >= z1) continue;

            // Bajo la plataforma no se ve.
            if (x0 > -PLATAFORMA_X_VOLEA && x1 < PLATAFORMA_X_VOLEA && z0 > -PLATAFORMA_Z_VOLEA && z1 < PLATAFORMA_Z_VOLEA) continue;

            float f = 0.9f + 0.2f * RuidoVolea(i * 53 + j, 611);
            Color c = TonoVolea(COLOR_BASALTO_VOLEA, f);
            Vector3 centro = { (x0 + x1) * 0.5f, Y_PASILLO_VOLEA, (z0 + z1) * 0.5f };
            Color cn = TinteVolea(c, centro);
            Color cb = TonoVolea(cn, 0.9f);
            VerticeVolea({ x0, Y_PASILLO_VOLEA, z1 }, cn);
            VerticeVolea({ x1, Y_PASILLO_VOLEA, z1 }, cb);
            VerticeVolea({ x1, Y_PASILLO_VOLEA, z0 }, cn);
            VerticeVolea({ x0, Y_PASILLO_VOLEA, z0 }, TonoVolea(cn, 1.06f));
        }
    }
    rlEnd();

    // Acantilados: bloques de roca que bajan a la lava, calientes al pie.
    const float ALTO_RISCO = 1.2f;
    float yRisco = Y_PASILLO_VOLEA - ALTO_RISCO * 0.5f;
    int bloques = decoracion ? 1 : 2;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;

        for (float z = PASILLO_Z_FONDO_VOLEA; z < PASILLO_Z_FRENTE_VOLEA; z += 1.6f * (float)bloques)
        {
            int k = (int)((z + 40.0f) * 3.0f) + lado * 7;
            float sale = 0.2f + 0.35f * RuidoVolea(k, 621);
            CuboRocaVolea({ s * (PASILLO_X_VOLEA + sale * 0.5f - 0.1f), yRisco, z + 0.8f * (float)bloques }, { sale + 0.2f, ALTO_RISCO + 0.1f * RuidoVolea(k, 622), 1.6f * (float)bloques + 0.05f }, TonoVolea(COLOR_BASALTO_OSCURO_VOLEA, 0.9f + 0.2f * RuidoVolea(k, 623)), 0.55f);
        }

        for (float z = GRADA_Z_FONDO_VOLEA; z < PASILLO_Z_FONDO_VOLEA; z += 1.6f * (float)bloques)
        {
            int k = (int)((z + 40.0f) * 3.0f) + lado * 11;
            float sale = 0.2f + 0.35f * RuidoVolea(k, 624);
            CuboRocaVolea({ s * (GRADA_X_VOLEA + sale * 0.5f - 0.1f), yRisco, z + 0.8f * (float)bloques }, { sale + 0.2f, ALTO_RISCO, 1.6f * (float)bloques + 0.05f }, TonoVolea(COLOR_BASALTO_OSCURO_VOLEA, 0.9f + 0.2f * RuidoVolea(k, 625)), 0.55f);
        }

        // Escalon entre el pasillo y la base de las gradas.
        CuboRocaVolea({ s * (PASILLO_X_VOLEA + GRADA_X_VOLEA) * 0.5f, yRisco, PASILLO_Z_FONDO_VOLEA + 0.15f }, { GRADA_X_VOLEA - PASILLO_X_VOLEA, ALTO_RISCO, 0.4f }, COLOR_BASALTO_OSCURO_VOLEA, 0.55f);
        rlBegin(RL_QUADS);
        QuadNieblaVolea(
            { s * PASILLO_X_VOLEA, Y_PASILLO_VOLEA, PASILLO_Z_FONDO_VOLEA },
            { s * GRADA_X_VOLEA, Y_PASILLO_VOLEA, PASILLO_Z_FONDO_VOLEA },
            { s * GRADA_X_VOLEA, Y_PASILLO_VOLEA, PASILLO_Z_FONDO_VOLEA - 0.01f },
            { s * PASILLO_X_VOLEA, Y_PASILLO_VOLEA, PASILLO_Z_FONDO_VOLEA - 0.01f },
            COLOR_BASALTO_VOLEA
        );
        rlEnd();
    }

    // Frente de la isla (casi siempre fuera de cuadro).
    CuboRocaVolea({ 0.0f, yRisco, PASILLO_Z_FRENTE_VOLEA + 0.2f }, { PASILLO_X_VOLEA * 2.0f + 0.4f, ALTO_RISCO, 0.4f }, COLOR_BASALTO_OSCURO_VOLEA, 0.55f);
}


// Plataforma de la cancha (coincide con el bloque de colision): losas de
// obsidiana pulida teñidas por equipo, margen de basalto, lineas de hueso,
// linea central de lava bajo la red y el canto con la veta de magma.
static void DibujarCanchaVolea(float t, bool efectos)
{
    // Canto de la plataforma (un escalon sobre el pasillo).
    float alto = -Y_PASILLO_VOLEA;
    CuboVolea({ 0.0f, -alto * 0.5f, 0.0f }, { PLATAFORMA_X_VOLEA * 2.0f, alto, PLATAFORMA_Z_VOLEA * 2.0f }, TinteVolea(COLOR_OBSIDIANA_VOLEA, { 0.0f, 0.0f, 0.0f }));

    // Lecho de juntas.
    rlBegin(RL_QUADS);
    QuadVolea({ -PLATAFORMA_X_VOLEA, 0.001f, PLATAFORMA_Z_VOLEA }, { PLATAFORMA_X_VOLEA, 0.001f, PLATAFORMA_Z_VOLEA }, { PLATAFORMA_X_VOLEA, 0.001f, -PLATAFORMA_Z_VOLEA }, { -PLATAFORMA_X_VOLEA, 0.001f, -PLATAFORMA_Z_VOLEA }, Color{ 18, 16, 24, 255 });
    rlEnd();

    // Margen de basalto entre la linea y el canto (losas de 1 x 1).
    rlBegin(RL_QUADS);
    for (int i = 0; i < (int)(PLATAFORMA_X_VOLEA * 2.0f); i++)
    {
        for (int j = 0; j < (int)(PLATAFORMA_Z_VOLEA * 2.0f); j++)
        {
            float x0 = -PLATAFORMA_X_VOLEA + (float)i;
            float z0 = -PLATAFORMA_Z_VOLEA + (float)j;
            bool dentro = x0 >= -MITAD_CANCHA_X && x0 + 1.0f <= MITAD_CANCHA_X && z0 >= -MITAD_CANCHA_Z && z0 + 1.0f <= MITAD_CANCHA_Z;
            if (dentro) continue;

            float f = 0.92f + 0.16f * RuidoVolea(i * 31 + j, 631);
            Color c = TinteVolea(TonoVolea(Color{ 62, 56, 64, 255 }, f), { x0, 0.0f, z0 });
            QuadVolea({ x0 + 0.03f, 0.004f, z0 + 0.97f }, { x0 + 0.97f, 0.004f, z0 + 0.97f }, { x0 + 0.97f, 0.004f, z0 + 0.03f }, { x0 + 0.03f, 0.004f, z0 + 0.03f }, c);
        }
    }
    rlEnd();

    // Losas de obsidiana de la cancha (2 x 2), cada mitad teñida por su
    // equipo y con un brillo de pulido en diagonal.
    rlBegin(RL_QUADS);
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            float x0 = -MITAD_CANCHA_X + 2.0f * (float)i;
            float z0 = -MITAD_CANCHA_Z + 2.0f * (float)j;
            float xc = x0 + 1.0f;
            int equipo = xc < 0.0f ? 0 : 1;
            float f = 0.92f + 0.16f * RuidoVolea(i * 17 + j, 641);
            Color base = MezclaVolea(COLOR_OBSIDIANA_VOLEA, ColorEquipoVolea(equipo), 0.26f);
            base = TinteVolea(TonoVolea(base, f), { xc, 0.0f, z0 + 1.0f });
            Color brillo = MezclaVolea(base, Color{ 200, 190, 230, 255 }, 0.18f);
            Color oscuro = TonoVolea(base, 0.82f);
            VerticeVolea({ x0 + 0.04f, 0.006f, z0 + 1.96f }, oscuro);
            VerticeVolea({ x0 + 1.96f, 0.006f, z0 + 1.96f }, base);
            VerticeVolea({ x0 + 1.96f, 0.006f, z0 + 0.04f }, brillo);
            VerticeVolea({ x0 + 0.04f, 0.006f, z0 + 0.04f }, base);
        }
    }
    rlEnd();

    // Grietas de magma finas en algunas losas (laten despacio).
    for (int k = 0; k < 14; k++)
    {
        float x = -MITAD_CANCHA_X + 0.6f + (2.0f * MITAD_CANCHA_X - 1.2f) * RuidoVolea(k, 651);
        float z = -MITAD_CANCHA_Z + 0.6f + (2.0f * MITAD_CANCHA_Z - 1.2f) * RuidoVolea(k, 652);
        float a = 6.2832f * RuidoVolea(k, 653);
        float l = 0.4f + 0.6f * RuidoVolea(k, 654);
        float pulso = efectos ? 0.6f + 0.4f * std::sin(t * 1.5f + (float)k) : 0.8f;
        Color c = AlfaVolea(TonoVolea(COLOR_LAVA_VOLEA, 0.55f + 0.3f * pulso), 1.0f);
        float xb = x + std::cos(a) * l, zb = z + std::sin(a) * l;
        TrazoSueloVolea(x, z, xb, zb, 0.008f, 0.035f, c);
        TrazoSueloVolea(xb, zb, xb + std::cos(a + 0.9f) * l * 0.6f, zb + std::sin(a + 0.9f) * l * 0.6f, 0.008f, 0.025f, c);
    }

    // Emblema de cada equipo en el centro de su mitad: aro y llama.
    for (int e = 0; e < 2; e++)
    {
        float cx = (e == 0 ? -1.0f : 1.0f) * MITAD_CANCHA_X * 0.5f;
        Color c = MezclaVolea(ColorEquipoVolea(e), COLOR_OBSIDIANA_VOLEA, 0.35f);
        AnilloVolea({ cx, 0.009f, 0.0f }, 1.45f, 1.62f, c, 32);
        rlBegin(RL_QUADS);
        QuadVolea({ cx, 0.009f, -1.05f }, { cx + 0.5f, 0.009f, 0.15f }, { cx, 0.009f, 0.75f }, { cx - 0.5f, 0.009f, 0.15f }, c);
        QuadVolea({ cx, 0.01f, -0.35f }, { cx + 0.24f, 0.01f, 0.25f }, { cx, 0.01f, 0.5f }, { cx - 0.24f, 0.01f, 0.25f }, MezclaVolea(c, COLOR_OBSIDIANA_VOLEA, 0.5f));
        rlEnd();
    }

    // Lineas de la cancha (el borde dibujado es el borde real).
    const float ANCHO = 0.14f;
    const float Y = 0.012f;
    TrazoSueloVolea(-MITAD_CANCHA_X - ANCHO * 0.5f, -MITAD_CANCHA_Z, MITAD_CANCHA_X + ANCHO * 0.5f, -MITAD_CANCHA_Z, Y, ANCHO, COLOR_LINEA_VOLEA);
    TrazoSueloVolea(-MITAD_CANCHA_X - ANCHO * 0.5f, MITAD_CANCHA_Z, MITAD_CANCHA_X + ANCHO * 0.5f, MITAD_CANCHA_Z, Y, ANCHO, COLOR_LINEA_VOLEA);
    TrazoSueloVolea(-MITAD_CANCHA_X, -MITAD_CANCHA_Z, -MITAD_CANCHA_X, MITAD_CANCHA_Z, Y, ANCHO, COLOR_LINEA_VOLEA);
    TrazoSueloVolea(MITAD_CANCHA_X, -MITAD_CANCHA_Z, MITAD_CANCHA_X, MITAD_CANCHA_Z, Y, ANCHO, COLOR_LINEA_VOLEA);

    // Linea de ataque punteada a cada lado (solo referencia).
    for (int e = -1; e <= 1; e += 2)
    {
        for (int k = 0; k < 10; k++)
        {
            float z0 = -MITAD_CANCHA_Z + (float)k * 1.0f + 0.25f;
            TrazoSueloVolea((float)e * 3.0f, z0, (float)e * 3.0f, z0 + 0.5f, Y, 0.08f, AlfaVolea(TonoVolea(COLOR_LINEA_VOLEA, 0.6f), 1.0f));
        }
    }

    // Linea central de lava bajo la red.
    float pulso = efectos ? 0.8f + 0.2f * std::sin(t * 3.0f) : 0.9f;
    TrazoSueloVolea(0.0f, -MITAD_CANCHA_Z - 0.6f, 0.0f, MITAD_CANCHA_Z + 0.6f, Y + 0.002f, 0.2f, MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, 0.4f * pulso));

    // Canto alzado con veta de magma.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        CuboNieblaVolea({ 0.0f, 0.04f, s * (PLATAFORMA_Z_VOLEA - 0.09f) }, { PLATAFORMA_X_VOLEA * 2.0f, 0.08f, 0.18f }, Color{ 96, 86, 96, 255 });
        CuboNieblaVolea({ s * (PLATAFORMA_X_VOLEA - 0.09f), 0.04f, 0.0f }, { 0.18f, 0.08f, PLATAFORMA_Z_VOLEA * 2.0f }, Color{ 96, 86, 96, 255 });
    }

    Color veta = MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, 0.25f * pulso);
    rlBegin(RL_QUADS);
    float yv0 = Y_PASILLO_VOLEA + 0.03f, yv1 = Y_PASILLO_VOLEA + 0.07f;
    float px = PLATAFORMA_X_VOLEA + 0.005f, pz = PLATAFORMA_Z_VOLEA + 0.005f;
    QuadVolea({ -px, yv0, pz }, { px, yv0, pz }, { px, yv1, pz }, { -px, yv1, pz }, veta);
    QuadVolea({ px, yv0, -pz }, { -px, yv0, -pz }, { -px, yv1, -pz }, { px, yv1, -pz }, veta);
    QuadVolea({ px, yv0, pz }, { px, yv0, -pz }, { px, yv1, -pz }, { px, yv1, pz }, veta);
    QuadVolea({ -px, yv0, -pz }, { -px, yv0, pz }, { -px, yv1, pz }, { -px, yv1, -pz }, veta);
    rlEnd();
}


//==================================================
// VISUAL: UTILERIA
//==================================================

// Llama: tres lenguas que titilan y un nucleo claro.
static void LlamaVolea(Vector3 base, float escala, float t, int semilla)
{
    for (int k = 0; k < 3; k++)
    {
        float fase = t * (7.0f + (float)k * 1.7f) + (float)semilla * 1.3f + (float)k * 2.1f;
        float alto = escala * (0.75f + 0.18f * std::sin(fase) + 0.08f * std::sin(fase * 2.3f)) * (k == 0 ? 1.0f : 0.66f);
        float dx = (float)(k == 0 ? 0 : (k == 1 ? -1 : 1)) * escala * 0.14f;
        float vaiven = std::sin(fase * 0.7f) * escala * 0.07f;
        Color c = k == 0 ? COLOR_LAVA_VOLEA : Color{ 255, 150, 40, 255 };
        PrismaVolea({ base.x + dx, base.y, base.z }, { base.x + dx + vaiven, base.y + alto, base.z }, escala * (k == 0 ? 0.26f : 0.17f), 0.0f, 5, c, fase, 1.0f);
    }

    PrismaVolea(base, { base.x, base.y + escala * 0.42f, base.z }, escala * 0.13f, 0.0f, 5, COLOR_LAVA_CLARA_VOLEA, 0.0f, 1.0f);
}


static Vector3 FuegoBraseroVolea(Vector3 base, float escala)
{
    return { base.x, base.y + 1.24f * escala, base.z };
}


// Brasero: pedestal de basalto, cuenco de hierro con brasas y llama.
static void BraseroVolea(Vector3 base, float escala, float t, int semilla)
{
    PrismaVolea(base, { base.x, base.y + 0.86f * escala, base.z }, 0.34f * escala, 0.26f * escala, 6, COLOR_BASALTO_VOLEA, 0.3f, 0.0f, true);
    CuboNieblaVolea({ base.x, base.y + 0.06f * escala, base.z }, { 0.8f * escala, 0.12f * escala, 0.8f * escala }, COLOR_BASALTO_OSCURO_VOLEA);
    PrismaVolea({ base.x, base.y + 0.86f * escala, base.z }, { base.x, base.y + 1.22f * escala, base.z }, 0.2f * escala, 0.44f * escala, 8, COLOR_HIERRO_VOLEA, 0.0f, 0.0f, true);
    AnilloVolea({ base.x, base.y + 1.2f * escala, base.z }, 0.0f, 0.4f * escala, Color{ 255, 120, 40, 255 }, 10);

    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.57f + 0.4f;
        CuboGiradoVolea({ base.x + std::cos(a) * 0.46f * escala, base.y + 1.2f * escala, base.z + std::sin(a) * 0.46f * escala }, { 0.08f * escala, 0.12f * escala, 0.08f * escala }, a * 57.3f, COLOR_HIERRO_VOLEA);
    }

    LlamaVolea(FuegoBraseroVolea(base, escala), 0.85f * escala, t, semilla);
}


// Columna de basalto hexagonal con tapa clara.
static void ColumnaBasaltoVolea(Vector3 base, float radio, float alto, int semilla, float y0 = Y_LAVA_VOLEA)
{
    float giro = RuidoVolea(semilla, 701) * 1.0f;
    Color c = TonoVolea(COLOR_BASALTO_VOLEA, 0.86f + 0.22f * RuidoVolea(semilla, 702));
    PrismaVolea({ base.x, y0, base.z }, { base.x, base.y + alto, base.z }, radio, radio * 0.94f, 6, c, giro, 0.0f, true);
    PrismaVolea({ base.x, base.y + alto, base.z }, { base.x, base.y + alto + 0.08f, base.z }, radio * 0.94f, radio * 0.82f, 6, TonoVolea(c, 1.25f), giro, 0.0f, true);
}


// Grupo de columnas de basalto que salen de la lava.
static void GrupoColumnasVolea(Vector3 centro, int cantidad, float alturaMax, int semilla)
{
    for (int k = 0; k < cantidad; k++)
    {
        float a = 6.2832f * RuidoVolea(k, semilla) ;
        float d = k == 0 ? 0.0f : 0.8f + 1.4f * RuidoVolea(k, semilla + 1);
        float alto = alturaMax * (k == 0 ? 1.0f : 0.35f + 0.55f * RuidoVolea(k, semilla + 2));
        float radio = 0.55f + 0.3f * RuidoVolea(k, semilla + 3);
        ColumnaBasaltoVolea({ centro.x + std::cos(a) * d, Y_LAVA_VOLEA, centro.z + std::sin(a) * d }, radio, alto, semilla + k);
    }
}


// Bandera en mastil: tela que flamea, con franjas y un rombo claro.
static void BanderaVolea(Vector3 pie, float altoMastil, float ancho, float altoTela, Color color, float t, int semilla, float direccion)
{
    PrismaVolea(pie, { pie.x, pie.y + altoMastil, pie.z }, 0.06f, 0.05f, 6, COLOR_HIERRO_VOLEA, 0.0f, 0.0f, true);
    CuboNieblaVolea({ pie.x, pie.y + altoMastil + 0.08f, pie.z }, { 0.16f, 0.16f, 0.16f }, COLOR_ORO_VOLEA);

    const int NX = 8, NY = 4;
    Vector3 p[NX + 1][NY + 1];

    for (int i = 0; i <= NX; i++)
    {
        for (int j = 0; j <= NY; j++)
        {
            float u = (float)i / (float)NX;
            float v = (float)j / (float)NY;
            float ola = std::sin(t * 3.4f - u * 5.0f + (float)semilla) * 0.32f * u;
            p[i][j] = {
                pie.x + direccion * u * ancho,
                pie.y + altoMastil - v * altoTela - u * 0.15f + ola * 0.25f,
                pie.z + ola
            };
        }
    }

    rlBegin(RL_QUADS);
    for (int i = 0; i < NX; i++)
    {
        for (int j = 0; j < NY; j++)
        {
            bool franja = j == 0 || j == NY - 1;
            bool rombo = (i == 3 || i == 4) && (j == 1 || j == 2);
            Color c = franja ? TonoVolea(color, 0.62f) : (rombo ? MezclaVolea(color, WHITE, 0.65f) : color);
            float sombra = 0.82f + 0.18f * std::sin(t * 3.4f - (float)i / (float)NX * 5.0f + (float)semilla + 1.2f);
            QuadNieblaVolea(p[i][j + 1], p[i + 1][j + 1], p[i + 1][j], p[i][j], TonoVolea(c, sombra));
        }
    }
    rlEnd();
}


// Estandarte colgante (de la muralla de columnas): cuelga y se mece.
static void EstandarteVolea(Vector3 arriba, float ancho, float alto, Color color, float t, int semilla)
{
    const int NY = 6;
    CuboNieblaVolea({ arriba.x, arriba.y + 0.05f, arriba.z + 0.05f }, { ancho + 0.3f, 0.1f, 0.1f }, COLOR_HIERRO_VOLEA);

    rlBegin(RL_QUADS);
    for (int j = 0; j < NY; j++)
    {
        float v0 = (float)j / (float)NY, v1 = (float)(j + 1) / (float)NY;
        float o0 = std::sin(t * 1.8f + v0 * 2.5f + (float)semilla) * 0.18f * v0;
        float o1 = std::sin(t * 1.8f + v1 * 2.5f + (float)semilla) * 0.18f * v1;
        float a0 = ancho * 0.5f, a1 = ancho * 0.5f;
        Color c = (j == 0 || j == NY - 2) ? TonoVolea(color, 0.6f) : color;

        if (j == NY - 1)
        {
            // Punta en cola de golondrina.
            QuadNieblaVolea({ arriba.x - a0, arriba.y - alto * v0, arriba.z + o0 }, { arriba.x, arriba.y - alto * (v0 + 0.06f), arriba.z + o0 }, { arriba.x, arriba.y - alto * (v0 + 0.06f), arriba.z + o0 }, { arriba.x - a1, arriba.y - alto * v1, arriba.z + o1 }, c);
            QuadNieblaVolea({ arriba.x, arriba.y - alto * (v0 + 0.06f), arriba.z + o0 }, { arriba.x + a0, arriba.y - alto * v0, arriba.z + o0 }, { arriba.x + a1, arriba.y - alto * v1, arriba.z + o1 }, { arriba.x, arriba.y - alto * (v0 + 0.06f), arriba.z + o0 }, c);
        }
        else
        {
            QuadNieblaVolea({ arriba.x - a0, arriba.y - alto * v0, arriba.z + o0 }, { arriba.x + a0, arriba.y - alto * v0, arriba.z + o0 }, { arriba.x + a1, arriba.y - alto * v1, arriba.z + o1 }, { arriba.x - a1, arriba.y - alto * v1, arriba.z + o1 }, c);
        }
    }

    // Emblema: llama clara en el centro.
    float om = std::sin(t * 1.8f + 0.45f * 2.5f + (float)semilla) * 0.18f * 0.45f + 0.01f;
    Color claro = MezclaVolea(color, COLOR_LAVA_CLARA_VOLEA, 0.7f);
    QuadNieblaVolea({ arriba.x, arriba.y - alto * 0.22f, arriba.z + om }, { arriba.x + ancho * 0.22f, arriba.y - alto * 0.45f, arriba.z + om }, { arriba.x, arriba.y - alto * 0.62f, arriba.z + om }, { arriba.x - ancho * 0.22f, arriba.y - alto * 0.45f, arriba.z + om }, claro);
    rlEnd();
}


// Estante con rocas de repuesto (se enfrian, apenas brillan).
static void EstanteRocasVolea(Vector3 base, float t)
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        CuboNieblaVolea({ base.x, base.y + 0.45f, base.z + (float)lado * 0.75f }, { 0.14f, 0.9f, 0.14f }, COLOR_HIERRO_VOLEA);
    }

    CuboNieblaVolea({ base.x, base.y + 0.5f, base.z }, { 0.5f, 0.08f, 1.6f }, COLOR_HIERRO_VOLEA);

    for (int k = 0; k < 3; k++)
    {
        float brillo = 0.45f + 0.15f * std::sin(t * 1.3f + (float)k * 2.0f);
        Color c = MezclaVolea(Color{ 86, 60, 56, 255 }, COLOR_LAVA_VOLEA, brillo * 0.5f);
        RocaVolea({ base.x, base.y + 0.82f, base.z - 0.5f + (float)k * 0.5f }, { 0.26f, 0.24f, 0.26f }, 710 + k, c, 0.18f, true, 0.2f);
    }
}


// Caldero de agua para enfriar las manos: vapor que sube.
static void CalderoVolea(Vector3 base)
{
    PrismaVolea(base, { base.x, base.y + 0.55f, base.z }, 0.38f, 0.48f, 10, COLOR_HIERRO_VOLEA, 0.0f, 0.0f, true);
    AnilloVolea({ base.x, base.y + 0.5f, base.z }, 0.0f, 0.42f, TinteVolea(Color{ 70, 110, 140, 255 }, base), 12);

    for (int k = 0; k < 3; k++)
    {
        CuboNieblaVolea({ base.x + std::cos((float)k * 2.09f) * 0.36f, base.y + 0.05f, base.z + std::sin((float)k * 2.09f) * 0.36f }, { 0.12f, 0.1f, 0.12f }, COLOR_BASALTO_OSCURO_VOLEA);
    }
}


// Cristales de obsidiana que asoman entre las losas.
static void CristalesObsidianaVolea(Vector3 base, float escala, int semilla)
{
    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.6f + RuidoVolea(k, semilla) * 1.2f;
        float abre = k == 0 ? 0.0f : 0.35f + 0.25f * RuidoVolea(k, semilla + 1);
        Vector3 dir = NormalizarVolea({ std::cos(a) * abre, 1.0f, std::sin(a) * abre });
        float largo = escala * (k == 0 ? 1.0f : 0.5f + 0.3f * RuidoVolea(k, semilla + 2));
        Vector3 pie = { base.x + std::cos(a) * abre * 0.2f * escala, base.y, base.z + std::sin(a) * abre * 0.2f * escala };
        Vector3 medio = SumaVolea(pie, EscalaVolea(dir, largo * 0.72f));
        Vector3 punta = SumaVolea(pie, EscalaVolea(dir, largo));
        float r = escala * (k == 0 ? 0.18f : 0.12f);
        Color c = Color{ 40, 34, 58, 255 };
        PrismaVolea(pie, medio, r * 0.85f, r, 6, c, a, 0.15f, true);
        PrismaVolea(medio, punta, r, 0.0f, 6, Color{ 90, 74, 130, 255 }, a, 0.25f, true);
    }
}


// Cascada de lava que cae del costado de las gradas al lago.
static void CascadaLavaVolea(Vector3 arriba, float ancho, float t, bool animada)
{
    const int NY = 10;
    float alto = arriba.y - Y_LAVA_VOLEA;
    float tl = animada ? t : 0.0f;

    rlBegin(RL_QUADS);
    for (int j = 0; j < NY; j++)
    {
        float v0 = (float)j / (float)NY, v1 = (float)(j + 1) / (float)NY;
        float y0 = arriba.y - alto * v0, y1 = arriba.y - alto * v1;
        // Sale hacia afuera y cae (parabola corta).
        float x0 = arriba.x + (arriba.x > 0.0f ? 1.0f : -1.0f) * 1.2f * std::sqrt(v0);
        float x1 = arriba.x + (arriba.x > 0.0f ? 1.0f : -1.0f) * 1.2f * std::sqrt(v1);
        float w0 = ancho * (1.0f + 0.4f * v0), w1 = ancho * (1.0f + 0.4f * v1);
        float b0 = 0.5f + 0.5f * std::sin(v0 * 18.0f - tl * 6.0f);
        float b1 = 0.5f + 0.5f * std::sin(v1 * 18.0f - tl * 6.0f);
        Color c0 = MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, b0 * 0.6f);
        Color c1 = MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, b1 * 0.6f);
        Vector3 p0 = { x0, y0, arriba.z - w0 * 0.5f }, p1 = { x0, y0, arriba.z + w0 * 0.5f };
        Vector3 p2 = { x1, y1, arriba.z + w1 * 0.5f }, p3 = { x1, y1, arriba.z - w1 * 0.5f };
        VerticeVolea(p0, TinteVolea(c0, p0, 0.8f));
        VerticeVolea(p1, TinteVolea(c0, p1, 0.8f));
        VerticeVolea(p2, TinteVolea(c1, p2, 0.8f));
        VerticeVolea(p3, TinteVolea(c1, p3, 0.8f));
    }
    rlEnd();
}


//==================================================
// VISUAL: GRADAS, PUBLICO Y MARCADOR
//==================================================

static float ZFrenteFilaVolea(int fila)
{
    return PASILLO_Z_FONDO_VOLEA - 0.3f - (float)fila * FONDO_FILA_VOLEA;
}


static float TopeFilaVolea(int fila)
{
    return 0.55f + (float)fila * ALTO_FILA_VOLEA;
}


// Hincha de piedra: golem de roca con ojos de brasa y bufanda de su equipo.
static void HinchaVolea(Vector3 pies, int equipo, float salto, bool brazos, int semilla, float t)
{
    float y = pies.y + salto;
    float f = 0.82f + 0.3f * RuidoVolea(semilla, 801);
    Color roca = TinteVolea(TonoVolea(Color{ 104, 92, 90, 255 }, f), pies);
    Color bufanda = TinteVolea(ColorEquipoVolea(equipo), pies);
    float giro = (RuidoVolea(semilla, 802) - 0.5f) * 30.0f;

    CuboGiradoVolea({ pies.x, y + 0.27f, pies.z }, { 0.46f, 0.5f, 0.32f }, giro, roca);
    CuboGiradoVolea({ pies.x, y + 0.55f, pies.z }, { 0.5f, 0.1f, 0.36f }, giro, bufanda);
    CuboGiradoVolea({ pies.x, y + 0.76f, pies.z }, { 0.32f, 0.3f, 0.28f }, giro + 8.0f, TonoVolea(roca, 1.08f));

    // Ojos de brasa.
    Color ojo = TinteVolea(Color{ 255, 190, 70, 255 }, pies, 0.9f);
    CuboVolea({ pies.x - 0.07f, y + 0.79f, pies.z + 0.145f }, { 0.06f, 0.05f, 0.02f }, ojo);
    CuboVolea({ pies.x + 0.07f, y + 0.79f, pies.z + 0.145f }, { 0.06f, 0.05f, 0.02f }, ojo);

    if (brazos)
    {
        float vaiven = std::sin(t * 9.0f + (float)semilla) * 0.06f;
        CuboVolea({ pies.x - 0.3f, y + 0.72f + vaiven, pies.z }, { 0.12f, 0.42f, 0.12f }, roca);
        CuboVolea({ pies.x + 0.3f, y + 0.72f - vaiven, pies.z }, { 0.12f, 0.42f, 0.12f }, roca);
    }
    else
    {
        CuboVolea({ pies.x - 0.29f, y + 0.24f, pies.z }, { 0.12f, 0.36f, 0.14f }, TonoVolea(roca, 0.9f));
        CuboVolea({ pies.x + 0.29f, y + 0.24f, pies.z }, { 0.12f, 0.36f, 0.14f }, TonoVolea(roca, 0.9f));
    }
}


static void DibujarGradasVolea(const MinijuegoVoleaMagma& m, float t, bool decoracion, int paso)
{
    Color roca = COLOR_ROCA_GRADA_VOLEA;

    // Escalones tallados: cada fila es un bloque hasta la muralla, partido
    // en tramos con tono propio (sillares de la roca).
    for (int fila = 0; fila < FILAS_GRADA_VOLEA; fila++)
    {
        float zf = ZFrenteFilaVolea(fila);
        float tope = TopeFilaVolea(fila);
        float fondo = GRADA_Z_FONDO_VOLEA + 0.4f;
        float zc = (zf + fondo) * 0.5f;
        float largo = zf - fondo;
        float alto = tope - Y_PASILLO_VOLEA;

        for (int tramo = 0; tramo < 8; tramo++)
        {
            float xa = -GRADA_X_VOLEA + (float)tramo * (GRADA_X_VOLEA * 2.0f / 8.0f);
            float xb = xa + GRADA_X_VOLEA * 2.0f / 8.0f;

            // El palco del marcador corta las filas bajas en el centro.
            if (fila <= 1 && xb > -MEDIO_PALCO_VOLEA && xa < MEDIO_PALCO_VOLEA)
            {
                if (xa < -MEDIO_PALCO_VOLEA) xb = -MEDIO_PALCO_VOLEA;
                else if (xb > MEDIO_PALCO_VOLEA) xa = MEDIO_PALCO_VOLEA;
                else continue;
            }

            float f = 0.9f + 0.16f * RuidoVolea(fila * 13 + tramo, 811);
            CuboRocaVolea({ (xa + xb) * 0.5f, Y_PASILLO_VOLEA + alto * 0.5f, zc }, { xb - xa + 0.01f, alto, largo }, TonoVolea(roca, f), 0.18f);
        }

        // Labio claro del escalon.
        CuboNieblaVolea({ 0.0f, tope + 0.02f, zf - 0.09f }, { GRADA_X_VOLEA * 2.0f, 0.06f, 0.2f }, TonoVolea(roca, 1.22f));
    }

    // Palco del marcador: bloque tallado con el marcador encima.
    float palcoFrente = ZFrenteFilaVolea(0);
    float palcoFondo = Z_MARCADOR_VOLEA - 0.7f;
    CuboRocaVolea({ 0.0f, (Y_PASILLO_VOLEA + 1.4f) * 0.5f, (palcoFrente + palcoFondo) * 0.5f }, { MEDIO_PALCO_VOLEA * 2.0f, 1.4f - Y_PASILLO_VOLEA, palcoFrente - palcoFondo }, TonoVolea(roca, 0.92f), 0.18f);

    // Escaleras de los pasillos (dos por lado).
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * 10.2f;

        for (int fila = 0; fila < FILAS_GRADA_VOLEA; fila++)
        {
            float zf = ZFrenteFilaVolea(fila);
            float tope = TopeFilaVolea(fila);
            CuboNieblaVolea({ x, tope - ALTO_FILA_VOLEA * 0.25f, zf + 0.35f }, { 1.0f, ALTO_FILA_VOLEA * 0.5f, 0.7f }, TonoVolea(roca, 1.1f));
        }
    }

    // Mejillas laterales de roca, en rampa hacia la muralla.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * (GRADA_X_VOLEA - 0.4f);

        for (int fila = 0; fila < FILAS_GRADA_VOLEA; fila++)
        {
            float zf = ZFrenteFilaVolea(fila);
            float tope = TopeFilaVolea(fila) + 0.9f;
            CuboRocaVolea({ x, (Y_PASILLO_VOLEA + tope) * 0.5f, zf - FONDO_FILA_VOLEA * 0.5f + 0.4f }, { 0.8f, tope - Y_PASILLO_VOLEA, FONDO_FILA_VOLEA + 0.02f }, TonoVolea(COLOR_BASALTO_VOLEA, 0.95f), 0.3f);
        }
    }

    // Muralla de columnas de basalto al fondo, alturas variadas.
    float zMuralla = GRADA_Z_FONDO_VOLEA + 0.2f;
    float topeUltima = TopeFilaVolea(FILAS_GRADA_VOLEA - 1);

    for (int k = 0; k < 16; k++)
    {
        float x = -GRADA_X_VOLEA + 1.0f + (float)k * ((GRADA_X_VOLEA * 2.0f - 2.0f) / 15.0f);
        float alto = topeUltima + 1.2f + 2.6f * RuidoVolea(k, 821) + (std::fabs(x) < 5.0f ? 0.0f : 0.8f);
        ColumnaBasaltoVolea({ x, Y_PASILLO_VOLEA, zMuralla }, 1.08f, alto - Y_PASILLO_VOLEA, 830 + k, Y_PASILLO_VOLEA);
    }

    if (!decoracion)
    {
        return;
    }

    // Estandartes colgados de la muralla: rojos a la izquierda, azules a la
    // derecha y el del volcan en el medio.
    const float xs[6] = { -13.0f, -9.0f, -5.4f, 5.4f, 9.0f, 13.0f };
    for (int k = 0; k < 6; k++)
    {
        int equipo = xs[k] < 0.0f ? 0 : 1;
        EstandarteVolea({ xs[k], topeUltima + 2.4f, zMuralla + 1.25f }, 1.3f, 2.6f, ColorEquipoVolea(equipo), t, 840 + k);
    }

    // Publico: la mitad izquierda alienta al equipo 1 y la derecha al 2.
    for (int fila = 0; fila < FILAS_GRADA_VOLEA; fila++)
    {
        float zf = ZFrenteFilaVolea(fila);
        float tope = TopeFilaVolea(fila);
        int asiento = 0;

        for (float x = -GRADA_X_VOLEA + 1.2f; x < GRADA_X_VOLEA - 1.1f; x += 0.92f, asiento++)
        {
            if (asiento % paso != 0) continue;
            if (std::fabs(std::fabs(x) - 10.2f) < 0.65f) continue;
            if (fila <= 1 && std::fabs(x) < MEDIO_PALCO_VOLEA + 0.3f) continue;

            int clave = fila * 97 + asiento;
            if (RuidoVolea(clave, 851) < 0.12f) continue;

            int equipo = x < 0.0f ? 0 : 1;
            float euforia = 0.15f + 0.85f * m.festejo[equipo];
            float fase = RuidoVolea(clave, 852) * 6.2832f;
            float rapidez = 7.0f + 4.0f * RuidoVolea(clave, 853);
            float salto = (0.02f + euforia * 0.3f) * std::fabs(std::sin(t * rapidez + fase));
            bool brazos = euforia > 0.3f + 0.6f * RuidoVolea(clave, 854);
            float xj = x + (RuidoVolea(clave, 855) - 0.5f) * 0.2f;

            HinchaVolea({ xj, tope, zf - 0.7f }, equipo, salto, brazos, clave, t);
        }
    }
}


// Siete segmentos: a arriba, b arriba-der, c abajo-der, d abajo,
// e abajo-izq, f arriba-izq, g medio.
static const unsigned char SEGMENTOS_VOLEA[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };


static void DigitoVolea(float cx, float cy, float z, float alto, int digito, Color encendido, Color apagado, bool soloEncendidos, float ensanche)
{
    float w = alto * 0.56f;
    float g = alto * 0.13f + ensanche;
    float hw = w * 0.5f, hh = alto * 0.5f;
    float rects[7][4] =
    {
        { cx, cy + hh - g * 0.5f, w, g },
        { cx + hw - g * 0.5f, cy + hh * 0.5f, g, hh },
        { cx + hw - g * 0.5f, cy - hh * 0.5f, g, hh },
        { cx, cy - hh + g * 0.5f, w, g },
        { cx - hw + g * 0.5f, cy - hh * 0.5f, g, hh },
        { cx - hw + g * 0.5f, cy + hh * 0.5f, g, hh },
        { cx, cy, w, g }
    };
    unsigned char mascara = SEGMENTOS_VOLEA[digito < 0 ? 0 : (digito > 9 ? 9 : digito)];

    rlBegin(RL_QUADS);
    for (int s = 0; s < 7; s++)
    {
        bool on = (mascara >> s) & 1;
        if (soloEncendidos && !on) continue;
        Color c = on ? encendido : apagado;
        float x0 = rects[s][0] - rects[s][2] * 0.5f + 0.02f, x1 = rects[s][0] + rects[s][2] * 0.5f - 0.02f;
        float y0 = rects[s][1] - rects[s][3] * 0.5f + 0.02f, y1 = rects[s][1] + rects[s][3] * 0.5f - 0.02f;
        QuadVolea({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, c);
    }
    rlEnd();
}


static void NumeroVolea(float cx, float cy, float z, float alto, int valor, Color encendido, Color apagado, bool soloEncendidos, float ensanche)
{
    if (valor < 0) valor = 0;

    if (valor < 10)
    {
        DigitoVolea(cx, cy, z, alto, valor, encendido, apagado, soloEncendidos, ensanche);
        return;
    }

    float paso = alto * 0.36f;
    DigitoVolea(cx - paso, cy, z, alto * 0.82f, (valor / 10) % 10, encendido, apagado, soloEncendidos, ensanche);
    DigitoVolea(cx + paso, cy, z, alto * 0.82f, valor % 10, encendido, apagado, soloEncendidos, ensanche);
}


static float XDigitoVolea(int equipo)
{
    return equipo == 0 ? -2.15f : 2.15f;
}


// Marcador de piedra: losa tallada con los puntos en lava, la brasa del
// saque y el canal-termometro de la roca.
static void DibujarMarcadorVolea(const MinijuegoVoleaMagma& m, float t)
{
    float z = Z_MARCADOR_VOLEA;
    float yBase = 1.4f;
    float alto = 3.5f;
    float yc = yBase + alto * 0.5f;

    CuboRocaVolea({ 0.0f, yc, z - 0.35f }, { 8.6f, alto, 0.7f }, Color{ 74, 64, 68, 255 }, 0.1f);
    CuboNieblaVolea({ 0.0f, yBase + alto + 0.12f, z - 0.3f }, { 9.2f, 0.24f, 0.9f }, COLOR_BASALTO_VOLEA);
    CuboNieblaVolea({ 0.0f, yBase + 0.06f, z - 0.25f }, { 9.0f, 0.16f, 0.9f }, COLOR_BASALTO_VOLEA);

    for (int s = -1; s <= 1; s += 2)
    {
        ColumnaBasaltoVolea({ (float)s * 4.75f, yBase, z - 0.3f }, 0.5f, alto + 0.5f, 860 + s, yBase);
    }

    // Placas del color de cada equipo detras de su numero.
    for (int e = 0; e < 2; e++)
    {
        float x = XDigitoVolea(e);
        float salto = m.pulsoMarcador[e];
        Color placa = MezclaVolea(ColorEquipoVolea(e), Color{ 40, 30, 34, 255 }, 0.45f - 0.25f * salto);
        rlBegin(RL_QUADS);
        QuadNieblaVolea({ x - 1.25f, yBase + 1.0f, z + 0.01f }, { x + 1.25f, yBase + 1.0f, z + 0.01f }, { x + 1.25f, yBase + 3.3f, z + 0.01f }, { x - 1.25f, yBase + 3.3f, z + 0.01f }, placa);
        rlEnd();
    }

    Color apagado = Color{ 34, 26, 30, 255 };

    for (int e = 0; e < 2; e++)
    {
        float brillo = 0.85f + 0.15f * std::sin(t * 2.0f + (float)e) + 0.3f * m.pulsoMarcador[e];
        Color lava = TonoVolea(MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, 0.35f + 0.4f * m.pulsoMarcador[e]), brillo);
        NumeroVolea(XDigitoVolea(e), yBase + 2.15f, z + 0.02f, 1.85f, m.puntos[e], lava, apagado, false, 0.0f);
    }

    // Guion central.
    rlBegin(RL_QUADS);
    QuadVolea({ -0.3f, yBase + 2.05f, z + 0.02f }, { 0.3f, yBase + 2.05f, z + 0.02f }, { 0.3f, yBase + 2.25f, z + 0.02f }, { -0.3f, yBase + 2.25f, z + 0.02f }, COLOR_LAVA_VOLEA);
    rlEnd();

    // Brasa del saque sobre el numero del equipo que saca.
    if (m.fase != FASE_VOLEA_TERMINADO)
    {
        float x = XDigitoVolea(m.equipoSaque);
        float pulso = 0.75f + 0.25f * std::sin(t * 6.0f);
        CuboVolea({ x, yBase + 3.12f, z + 0.05f }, { 0.22f, 0.22f, 0.06f }, TonoVolea(COLOR_LAVA_CLARA_VOLEA, pulso));
    }

    // Canal-termometro de la roca.
    float x0 = -3.4f, x1 = 3.4f;
    float y0 = yBase + 0.42f, y1 = yBase + 0.72f;
    float lleno = x0 + (x1 - x0) * Limitar01Volea(m.pelota.temperatura);
    Color roca = ColorRoca(m.pelota.temperatura, m.tiempoAnimacion);
    rlBegin(RL_QUADS);
    QuadVolea({ x0 - 0.06f, y0 - 0.06f, z + 0.01f }, { x1 + 0.06f, y0 - 0.06f, z + 0.01f }, { x1 + 0.06f, y1 + 0.06f, z + 0.01f }, { x0 - 0.06f, y1 + 0.06f, z + 0.01f }, Color{ 24, 18, 22, 255 });
    QuadVolea({ x0, y0, z + 0.02f }, { lleno, y0, z + 0.02f }, { lleno, y1, z + 0.02f }, { x0, y1, z + 0.02f }, roca);
    // Marca del punto de sobrecalentamiento.
    float xm = x0 + (x1 - x0) * TEMPERATURA_SOBRECALENTADA;
    QuadVolea({ xm - 0.03f, y0 - 0.1f, z + 0.03f }, { xm + 0.03f, y0 - 0.1f, z + 0.03f }, { xm + 0.03f, y1 + 0.1f, z + 0.03f }, { xm - 0.03f, y1 + 0.1f, z + 0.03f }, COLOR_LINEA_VOLEA);
    rlEnd();
}


//==================================================
// VISUAL: ANILLO CERCANO Y MEDIA DISTANCIA
//==================================================

static const Vector3 BRASEROS_VOLEA[] =
{
    { -11.4f, Y_PASILLO_VOLEA, -9.4f }, { 11.4f, Y_PASILLO_VOLEA, -9.4f },
    { -11.5f, Y_PASILLO_VOLEA, -3.4f }, { 11.5f, Y_PASILLO_VOLEA, -3.4f },
    { -11.5f, Y_PASILLO_VOLEA, 3.6f }, { 11.5f, Y_PASILLO_VOLEA, 3.6f },
    { -5.6f, Y_PASILLO_VOLEA, -9.9f }, { 5.6f, Y_PASILLO_VOLEA, -9.9f }
};
static const int CANTIDAD_BRASEROS_VOLEA = 8;


static void DibujarUtileriaVolea(float t, int paso)
{
    // Braseros.
    for (int k = 0; k < CANTIDAD_BRASEROS_VOLEA; k++)
    {
        BraseroVolea(BRASEROS_VOLEA[k], k >= 6 ? 0.9f : 1.0f, t, k);
    }

    // Banderas de cada equipo en su lado del pasillo.
    BanderaVolea({ -11.9f, Y_PASILLO_VOLEA, -6.6f }, 4.2f, 1.9f, 1.1f, ColorEquipoVolea(0), t, 1, 1.0f);
    BanderaVolea({ 11.9f, Y_PASILLO_VOLEA, -6.6f }, 4.2f, 1.9f, 1.1f, ColorEquipoVolea(1), t, 2, -1.0f);

    // Estante con rocas de repuesto y caldero de agua.
    EstanteRocasVolea({ 11.6f, Y_PASILLO_VOLEA, 0.1f }, t);
    CalderoVolea({ -11.6f, Y_PASILLO_VOLEA, 0.1f });

    // Bancos de piedra de cada equipo.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * 11.7f;
        CuboNieblaVolea({ x, Y_PASILLO_VOLEA + 0.42f, 6.4f }, { 1.0f, 0.16f, 2.6f }, COLOR_BASALTO_VOLEA);
        CuboNieblaVolea({ x, Y_PASILLO_VOLEA + 0.17f, 5.4f }, { 0.7f, 0.34f, 0.3f }, COLOR_BASALTO_OSCURO_VOLEA);
        CuboNieblaVolea({ x, Y_PASILLO_VOLEA + 0.17f, 7.4f }, { 0.7f, 0.34f, 0.3f }, COLOR_BASALTO_OSCURO_VOLEA);
        CuboNieblaVolea({ x, Y_PASILLO_VOLEA + 0.53f, 6.0f }, { 0.6f, 0.06f, 0.5f }, MezclaVolea(ColorEquipoVolea(s < 0 ? 0 : 1), WHITE, 0.15f));
    }

    // Columnas de basalto en los rincones del pasillo.
    for (int s = -1; s <= 1; s += 2)
    {
        ColumnaBasaltoVolea({ (float)s * 12.1f, Y_PASILLO_VOLEA, -10.1f }, 0.5f, 3.4f, 870 + s, Y_PASILLO_VOLEA);
        ColumnaBasaltoVolea({ (float)s * 12.5f, Y_PASILLO_VOLEA, -9.3f }, 0.4f, 2.2f, 872 + s, Y_PASILLO_VOLEA);
        ColumnaBasaltoVolea({ (float)s * 12.6f, Y_PASILLO_VOLEA, 9.2f }, 0.5f, 1.8f, 874 + s, Y_PASILLO_VOLEA);
    }

    // Cristales de obsidiana y piedras sueltas.
    if (paso == 1)
    {
        CristalesObsidianaVolea({ -12.2f, Y_PASILLO_VOLEA, -1.5f }, 0.8f, 881);
        CristalesObsidianaVolea({ 12.2f, Y_PASILLO_VOLEA, 2.0f }, 0.7f, 882);
        CristalesObsidianaVolea({ -12.3f, Y_PASILLO_VOLEA, 5.0f }, 0.6f, 883);
        CristalesObsidianaVolea({ 12.3f, Y_PASILLO_VOLEA, -5.0f }, 0.7f, 884);
    }

    for (int k = 0; k < 8; k += paso)
    {
        float s = k % 2 == 0 ? -1.0f : 1.0f;
        float z = -8.5f + 2.3f * (float)k;
        RocaVolea({ s * (12.2f + 0.2f * RuidoVolea(k, 891)), Y_PASILLO_VOLEA + 0.12f, z }, { 0.32f, 0.2f, 0.28f }, 890 + k, COLOR_BASALTO_OSCURO_VOLEA);
    }
}


static void DibujarAlrededoresVolea(float t, bool animada, int paso)
{
    // Columnas de basalto que salen del lago a los costados.
    GrupoColumnasVolea({ -18.5f, 0.0f, -4.0f }, 5, 4.5f, 901);
    GrupoColumnasVolea({ 18.8f, 0.0f, -3.0f }, 5, 5.0f, 911);
    GrupoColumnasVolea({ -16.4f, 0.0f, 5.5f }, 3, 2.6f, 921);
    GrupoColumnasVolea({ 16.8f, 0.0f, 6.0f }, 3, 2.2f, 931);

    if (paso == 1)
    {
        GrupoColumnasVolea({ -26.0f, 0.0f, -16.0f }, 6, 7.0f, 941);
        GrupoColumnasVolea({ 27.0f, 0.0f, -18.0f }, 6, 8.0f, 951);
        GrupoColumnasVolea({ -36.0f, 0.0f, -34.0f }, 5, 9.0f, 961);
        GrupoColumnasVolea({ 38.0f, 0.0f, -36.0f }, 5, 10.0f, 971);
    }

    // Banderas grandes sobre las columnas mas altas.
    BanderaVolea({ -18.5f, 4.5f + Y_LAVA_VOLEA, -4.0f }, 2.6f, 2.4f, 1.3f, ColorEquipoVolea(0), t, 3, 1.0f);
    BanderaVolea({ 18.8f, 5.0f + Y_LAVA_VOLEA, -3.0f }, 2.6f, 2.4f, 1.3f, ColorEquipoVolea(1), t, 4, -1.0f);

    // Cascadas de lava desde los costados de las gradas.
    CascadaLavaVolea({ -GRADA_X_VOLEA - 0.3f, 4.6f, -15.6f }, 1.1f, t, animada);
    CascadaLavaVolea({ GRADA_X_VOLEA + 0.3f, 4.6f, -15.6f }, 1.1f, t, animada);

    for (int s = -1; s <= 1; s += 2)
    {
        CuboRocaVolea({ (float)s * (GRADA_X_VOLEA + 0.2f), 4.4f, -15.6f }, { 1.0f, 0.5f, 1.8f }, COLOR_BASALTO_OSCURO_VOLEA, 0.6f);
    }
}


//==================================================
// VISUAL: PIEZAS JUGABLES (GLB CON FALLBACK POR PIEZA)
//==================================================

static void DibujarRedVolea(float t)
{
    // Red de cadenas incandescentes.
    float brillo = 0.5f + 0.5f * std::sin(t * 4.0f);
    Color cadena = Color{ 255, (unsigned char)(110 + 60 * brillo), 40, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float z = (float)lado * (MITAD_CANCHA_Z + 0.4f);

        // Zocalo de basalto bajo cada poste.
        PrismaVolea({ 0.0f, -0.02f, z }, { 0.0f, 0.16f, z }, 0.42f, 0.36f, 6, COLOR_BASALTO_OSCURO_VOLEA, 0.0f, 0.0f, true);

        if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_POSTE, {0,0,z}, {1,1,1}, cadena))
        {
            (DrawCylinder)({ 0.0f, 0.0f, z }, 0.14f, 0.18f, ALTURA_RED + 0.5f, 8, Color{ 20, 18, 26, 255 });
            (DrawSphere)({ 0.0f, ALTURA_RED + 0.6f, z }, 0.22f, cadena);
        }
    }

    float z0 = -(MITAD_CANCHA_Z + 0.4f);
    float z1 = MITAD_CANCHA_Z + 0.4f;

    if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_RED, {0,0,0}, {1,1,1}, cadena))
    {
        for (int fila = 0; fila < 5; fila++)
        {
            float y = 0.9f + 0.375f * (float)fila;
            DrawCylinderEx({ 0.0f, y, z0 }, { 0.0f, y, z1 }, 0.045f, 0.045f, 6, cadena);
        }

        for (int k = 0; k <= 12; k++)
        {
            float z = z0 + (z1 - z0) * (float)k / 12.0f;
            DrawCylinderEx({ 0.0f, 0.9f, z }, { 0.0f, ALTURA_RED, z }, 0.035f, 0.035f, 5, cadena);
        }

        (DrawCube)({ 0.0f, ALTURA_RED, 0.0f }, 0.1f, 0.1f, z1 - z0, Color{ 255, 210, 110, 255 });
    }
}


static void DibujarCharcos(const MinijuegoVoleaMagma& m)
{
    for (int i = 0; i < MAX_CHARCOS_VOLEA; i++)
    {
        const CharcoLavaVolea& charco = m.charcos[i];

        if (!charco.activo)
        {
            continue;
        }

        float restante = 1.0f - charco.tiempo / DURACION_CHARCO;
        float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 7.0f + (float)i);
        float alfa = restante < 0.25f ? restante / 0.25f : 1.0f;

        Vector3 p = {charco.x,0,charco.z};
        Color exterior = Fade(Color{255,(unsigned char)(70 + 50 * pulso),20,255},alfa);
        if (DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_CHARCO, p, {1,1,1}, exterior, 0, true))
        {
            DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_CHARCO, p, {1,1,1}, Fade(Color{255,190,60,255},alfa), 1, true);
            for (int j = 2; j < 4; j++)
                DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_CHARCO, p, {1,1,1},
                    Fade(ColorMaterialVoleaMagmaRetro3D(MODELO_VOLEA_CHARCO,j),alfa), j, true);
        }
        else
        {
            (DrawCylinder)({ charco.x, 0.04f, charco.z }, RADIO_CHARCO, RADIO_CHARCO, 0.04f, 24, exterior);
            (DrawCylinder)({ charco.x, 0.07f, charco.z }, RADIO_CHARCO * 0.6f, RADIO_CHARCO * 0.6f, 0.03f, 20, Fade(Color{255,190,60,255},alfa));
        }
    }
}


static void DibujarPelota(const MinijuegoVoleaMagma& m)
{
    const PelotaVolea& pelota = m.pelota;
    float t = m.tiempoAnimacion;
    Color color = ColorRoca(pelota.temperatura, t);
    bool caliente = pelota.temperatura >= TEMPERATURA_SOBRECALENTADA;

    // Sombra siempre visible y marcador del punto de caida.
    float altura = Acotar(pelota.posicion.y, 0.0f, 9.0f);
    float radioSombra = RADIO_PELOTA * (1.25f - 0.08f * altura);

    if (radioSombra < 0.3f) radioSombra = 0.3f;

    float escalaSombra = radioSombra / 0.48f;
    // Disco local Y=.015..027: sumar .03 mantiene la altura del cilindro
    // original (.045..060) y evita ocultarlo bajo las losas GLB (Y=.031).
    if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_SOMBRA,
        {pelota.posicion.x,0.03f,pelota.posicion.z}, {escalaSombra,1,escalaSombra}))
        (DrawCylinder)({ pelota.posicion.x, 0.045f, pelota.posicion.z }, radioSombra, radioSombra, 0.015f, 18, Color{8,6,10,255});

    if (pelota.estado == PELOTA_VOLEA_EN_JUEGO)
    {
        float tiempo = 0.0f;
        float x = 0.0f;
        float z = 0.0f;
        PredecirPelota(pelota, RADIO_PELOTA, tiempo, x, z);

        if (std::fabs(x) < MITAD_CANCHA_X + 3.0f && std::fabs(z) < MITAD_CANCHA_Z + 3.0f)
        {
            float pulso = 0.5f + 0.5f * std::sin(t * 9.0f);
            float escala = (0.55f + 0.1f * pulso) / 0.60f;
            // Solo el aro exterior y sus cuatro luces pulsan; aro interior fijo.
            bool modelo = DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_INDICADOR, {x,0,z}, {escala,1,escala}, WHITE, 0);
            if (modelo)
            {
                DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_INDICADOR, {x,0,z}, {1,1,1}, WHITE, 1);
                DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_INDICADOR, {x,0,z}, {escala,1,escala}, WHITE, 2);
            }
            else
            {
                DrawCircle3D({ x, 0.07f, z }, 0.55f + 0.1f * pulso, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 255, 240, 160, 255 });
                DrawCircle3D({ x, 0.07f, z }, 0.3f, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 255, 120, 60, 255 });
            }
        }
    }

    for (int k = LARGO_ESTELA_VOLEA - 1; k >= 1; k--)
    {
        float f = 1.0f - (float)k / (float)LARGO_ESTELA_VOLEA;
        float radio = RADIO_PELOTA * (0.25f + 0.55f * f);
        float escala = radio / 0.18f;
        Color ascua = Fade(color,0.5f * f);
        if (DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_ESTELA, pelota.estela[k], {escala,escala,escala}, ascua, 0, true))
            DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_ESTELA, pelota.estela[k], {escala,escala,escala},
                Fade(ColorMaterialVoleaMagmaRetro3D(MODELO_VOLEA_ESTELA,1),0.5f * f), 1, true);
        else
            (DrawSphere)(pelota.estela[k], radio, ascua);
    }

    // El halo de la roca al rojo va con las luces aditivas.
    if (!DibujarModeloVoleaMagmaRetro3D(caliente ? MODELO_VOLEA_ROCA_CALIENTE : MODELO_VOLEA_ROCA,
        pelota.posicion, {1,1,1}, color))
    {
        (DrawSphere)(pelota.posicion, RADIO_PELOTA, color);
        (DrawSphere)(
            { pelota.posicion.x + 0.12f, pelota.posicion.y + 0.14f, pelota.posicion.z + 0.1f },
            RADIO_PELOTA * 0.38f,
            Color{ (unsigned char)(color.r * 0.55f), (unsigned char)(color.g * 0.45f), (unsigned char)(color.b * 0.4f), 255 }
        );
    }
}


// Linea de caida punteada de la roca a su sombra: une las dos alturas.
static void DibujarLineaCaidaVolea(const MinijuegoVoleaMagma& m)
{
    const PelotaVolea& pelota = m.pelota;

    if (pelota.estado == PELOTA_VOLEA_PUNTO || pelota.posicion.y < 1.0f)
    {
        return;
    }

    float tramo = 0.28f;
    int n = (int)((pelota.posicion.y - RADIO_PELOTA - 0.1f) / tramo);
    Color c = AlfaVolea(COLOR_LAVA_CLARA_VOLEA, 0.55f);

    for (int k = 0; k < n; k += 2)
    {
        float y0 = 0.05f + (float)k * tramo;
        CuboVolea({ pelota.posicion.x, y0 + tramo * 0.5f, pelota.posicion.z }, { 0.035f, tramo, 0.035f }, c);
    }
}


//==================================================
// VISUAL: JUGADORES, CHISPAS Y LUCES
//==================================================

static void DibujarJugadoresVolea(
    const MinijuegoVoleaMagma& m,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    bool mostrarDebug
)
{
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorVolea& estado = m.estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];

        // Aro de su equipo en el suelo.
        Color colorEquipo = MezclaVolea(ColorEquipoVolea(estado.equipo), WHITE, 0.15f);
        AnilloVolea({ jugador.posicion.x, 0.02f, jugador.posicion.z }, 0.5f, 0.64f, colorEquipo, 24);

        // Alcance del golpe (solo depuracion).
        if (mostrarDebug)
        {
            DrawCircle3D(
                { jugador.posicion.x, 0.08f, jugador.posicion.z },
                AlcanceHorizontal(m, estado.equipo),
                { 1.0f, 0.0f, 0.0f }, 90.0f, LIME
            );
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorModeloCompartido(jugador, visual);
    }
}


static void DibujarChispasSolidasVolea(const MinijuegoVoleaMagma& m)
{
    for (int i = 0; i < MAX_CHISPAS_VOLEA; i++)
    {
        const ChispaVolea& c = m.chispas[i];
        if (!c.activa || c.brilla || c.humo) continue;
        float vida = Limitar01Volea(c.vida / c.vidaMaxima);
        float s = c.tamano * (0.5f + 0.5f * vida);
        CuboGiradoVolea(c.posicion, { s, s * 0.5f, s }, c.giro, c.color);
    }
}


// Humo y vapor (mezcla normal, sin escribir profundidad).
static void DibujarHumoVolea(const MinijuegoVoleaMagma& m, float t, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < MAX_CHISPAS_VOLEA; i++)
    {
        const ChispaVolea& c = m.chispas[i];
        if (!c.activa || !c.humo) continue;
        float vida = Limitar01Volea(c.vida / c.vidaMaxima);
        float r = c.tamano * (1.8f - vida);
        ResplandorFrenteVolea(c.posicion, r, AlfaVolea(c.color, 0.55f * vida));
    }

    if (decoracion)
    {
        // Penacho del volcan mayor: bocanadas oscuras que suben y se abren.
        Vector3 crater = { -38.0f, Y_LAVA_VOLEA + 46.0f, -112.0f };
        for (int k = 0; k < 9; k++)
        {
            float u = std::fmod(t * 0.05f + (float)k / 9.0f, 1.0f);
            Vector3 p = { crater.x + u * 22.0f + std::sin(t * 0.3f + (float)k) * 2.0f, crater.y + u * 40.0f, crater.z + u * 6.0f };
            float r = 6.0f + 14.0f * u;
            Color c = MezclaVolea(Color{ 140, 64, 48, 255 }, Color{ 54, 34, 40, 255 }, u);
            ResplandorFrenteVolea(p, r, AlfaVolea(TinteVolea(c, p), 0.75f * (1.0f - u * 0.6f)));
        }

        // Vapor del caldero.
        for (int k = 0; k < 4; k++)
        {
            float u = std::fmod(t * 0.35f + (float)k * 0.25f, 1.0f);
            Vector3 p = { -11.6f + std::sin(t + (float)k) * 0.15f, Y_PASILLO_VOLEA + 0.7f + u * 1.6f, 0.1f };
            ResplandorFrenteVolea(p, 0.25f + 0.45f * u, AlfaVolea(Color{ 220, 220, 228, 255 }, 0.28f * (1.0f - u)));
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarLucesVolea(
    const MinijuegoVoleaMagma& m,
    const JugadorPrueba jugadores[],
    int limite,
    float t,
    bool decoracion
)
{
    float tj = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Resplandor de la lava contra los cantos de la isla.
    for (int s = -1; s <= 1; s += 2)
    {
        for (int k = 0; k < 8; k++)
        {
            float z = -9.5f + 2.6f * (float)k;
            float pulso = 0.8f + 0.2f * std::sin(t * 1.3f + (float)k);
            ResplandorSueloVolea({ (float)s * (PASILLO_X_VOLEA + 0.4f), Y_PASILLO_VOLEA + 0.01f, z }, 2.2f * pulso, Color{ 255, 110, 40, 30 });
        }
    }

    // Braseros: halo, cono y charco de luz.
    for (int k = 0; k < CANTIDAD_BRASEROS_VOLEA; k++)
    {
        float escala = k >= 6 ? 0.9f : 1.0f;
        Vector3 f = FuegoBraseroVolea(BRASEROS_VOLEA[k], escala);
        float parpadeo = 0.86f + 0.09f * std::sin(t * 13.0f + (float)k * 2.3f) + 0.05f * std::sin(t * 29.0f + (float)k);
        Vector3 llama = { f.x, f.y + 0.45f, f.z };
        ResplandorFrenteVolea(llama, 1.6f * parpadeo, Color{ 255, 140, 50, 70 });
        ResplandorFrenteVolea(llama, 0.6f * parpadeo, Color{ 255, 220, 150, 150 });
        ConoLuzVolea(llama, { f.x, Y_PASILLO_VOLEA + 0.02f, f.z }, 1.6f, Color{ 255, 170, 90, 14 });

        for (int r = 0; r < 3; r++)
        {
            ResplandorSueloVolea({ f.x, Y_PASILLO_VOLEA + 0.015f + 0.002f * (float)r, f.z }, (1.3f + 1.0f * (float)r) * parpadeo, Color{ 255, 150, 70, 34 });
        }
    }

    // Linea central y cable superior de la red.
    float pulsoRed = 0.5f + 0.5f * std::sin(tj * 4.0f);
    float sacudida = m.sacudidaRed;
    for (int k = 0; k <= 10; k++)
    {
        float z = -MITAD_CANCHA_Z - 0.4f + (2.0f * MITAD_CANCHA_Z + 0.8f) * (float)k / 10.0f;
        ResplandorFrenteVolea({ 0.0f, ALTURA_RED, z }, 0.35f + 0.25f * sacudida, Color{ 255, 150, 60, (unsigned char)(50 + 40 * pulsoRed + 120 * sacudida) });
        ResplandorSueloVolea({ 0.0f, 0.02f, z }, 0.5f, Color{ 255, 120, 40, 40 });
    }

    // Marcador: los numeros encendidos irradian.
    if (decoracion)
    {
        for (int e = 0; e < 2; e++)
        {
            Color brillo = AlfaVolea(MezclaVolea(COLOR_LAVA_VOLEA, COLOR_LAVA_CLARA_VOLEA, 0.3f), 0.35f + 0.4f * m.pulsoMarcador[e]);
            NumeroVolea(XDigitoVolea(e), 1.4f + 2.15f, Z_MARCADOR_VOLEA + 0.05f, 1.85f, m.puntos[e], brillo, BLANK, true, 0.14f);
            ResplandorFrenteVolea({ XDigitoVolea(e), 3.5f, Z_MARCADOR_VOLEA + 0.3f }, 2.2f + 1.2f * m.pulsoMarcador[e], AlfaVolea(MezclaVolea(ColorEquipoVolea(e), COLOR_LAVA_VOLEA, 0.5f), 0.12f + 0.3f * m.pulsoMarcador[e]));
        }

        // Cascadas: bruma caliente al pie.
        for (int s = -1; s <= 1; s += 2)
        {
            ResplandorFrenteVolea({ (float)s * (GRADA_X_VOLEA + 1.4f), 1.0f, -15.6f }, 3.0f, Color{ 255, 120, 40, 50 });
            ResplandorFrenteVolea({ (float)s * (GRADA_X_VOLEA + 0.8f), 3.2f, -15.6f }, 2.2f, Color{ 255, 150, 60, 40 });
        }

        // Crater del volcan mayor y su reflejo en el humo.
        float erupcion = 0.75f + 0.25f * std::sin(t * 0.9f) * std::sin(t * 2.3f);
        ResplandorFrenteVolea({ -38.0f, Y_LAVA_VOLEA + 47.0f, -112.0f }, 26.0f * erupcion, Color{ 255, 110, 40, 70 });
        ResplandorFrenteVolea({ -38.0f, Y_LAVA_VOLEA + 47.0f, -112.0f }, 10.0f * erupcion, Color{ 255, 200, 110, 90 });
        ResplandorFrenteVolea({ 62.0f, Y_LAVA_VOLEA + 34.5f, -130.0f }, 12.0f, Color{ 255, 100, 40, 40 });
    }

    // Charcos de lava activos.
    for (int i = 0; i < MAX_CHARCOS_VOLEA; i++)
    {
        const CharcoLavaVolea& charco = m.charcos[i];
        if (!charco.activo) continue;
        float restante = 1.0f - charco.tiempo / DURACION_CHARCO;
        float alfa = restante < 0.25f ? restante / 0.25f : 1.0f;
        ResplandorSueloVolea({ charco.x, 0.09f, charco.z }, RADIO_CHARCO * 1.6f, AlfaVolea(COLOR_LAVA_VOLEA, 0.3f * alfa));
        ResplandorFrenteVolea({ charco.x, 0.6f, charco.z }, 1.6f, AlfaVolea(COLOR_LAVA_VOLEA, 0.2f * alfa));
    }

    // Marca quemada del ultimo impacto: borde encendido que se apaga.
    if (m.tiempoImpacto >= 0.0f)
    {
        float u = Limitar01Volea(m.tiempoImpacto / 3.0f);
        Vector3 p = { m.posicionImpacto.x, 0.03f, m.posicionImpacto.z };
        ResplandorSueloVolea(p, 0.8f + 1.8f * Limitar01Volea(m.tiempoImpacto * 3.0f), AlfaVolea(COLOR_LAVA_VOLEA, 0.6f * (1.0f - u)));
        if (m.tiempoImpacto < 0.4f)
        {
            float v = m.tiempoImpacto / 0.4f;
            ResplandorFrenteVolea({ p.x, 0.8f, p.z }, 1.2f + 2.4f * v, AlfaVolea(COLOR_LAVA_CLARA_VOLEA, 0.8f * (1.0f - v)));
        }
    }

    // Roca: halo segun la temperatura, charco de luz en el suelo y la marca
    // de caida con el color del equipo que tiene que defender.
    {
        const PelotaVolea& pelota = m.pelota;
        Color color = ColorRoca(pelota.temperatura, tj);
        bool caliente = pelota.temperatura >= TEMPERATURA_SOBRECALENTADA;
        float calor = 0.35f + 0.65f * pelota.temperatura;
        ResplandorFrenteVolea(pelota.posicion, RADIO_PELOTA * (2.6f + (caliente ? 1.2f : 0.0f) * (0.8f + 0.2f * std::sin(tj * 10.0f))), AlfaVolea(color, 0.45f * calor));
        float altura = Acotar(pelota.posicion.y, 0.0f, 9.0f);
        ResplandorSueloVolea({ pelota.posicion.x, 0.035f, pelota.posicion.z }, 1.4f + 0.18f * altura, AlfaVolea(COLOR_LAVA_VOLEA, 0.32f * calor * (1.0f - altura / 12.0f)));

        if (pelota.estado == PELOTA_VOLEA_EN_JUEGO)
        {
            float tiempo = 0.0f, x = 0.0f, z = 0.0f;
            PredecirPelota(pelota, RADIO_PELOTA, tiempo, x, z);

            if (std::fabs(x) < MITAD_CANCHA_X + 3.0f && std::fabs(z) < MITAD_CANCHA_Z + 3.0f)
            {
                bool fuera = std::fabs(x) > MITAD_CANCHA_X || std::fabs(z) > MITAD_CANCHA_Z;
                Color c = fuera ? Color{ 230, 230, 230, 255 } : MezclaVolea(ColorEquipoVolea(x < 0.0f ? 0 : 1), WHITE, 0.2f);
                float pulso = 0.5f + 0.5f * std::sin(tj * 9.0f);
                ResplandorSueloVolea({ x, 0.05f, z }, 1.2f + 0.3f * pulso, AlfaVolea(c, 0.55f));
            }
        }

        if (m.destelloGolpe > 0.0f)
        {
            ResplandorFrenteVolea(m.posicionGolpe, 0.8f + 1.4f * (1.0f - m.destelloGolpe), AlfaVolea(COLOR_LAVA_CLARA_VOLEA, 0.8f * m.destelloGolpe));
        }
    }

    // Destello del jugador que golpea.
    for (int i = 0; i < limite; i++)
    {
        float d = m.destelloJugador[i];
        if (m.estadosJugadores[i].equipo < 0 || d <= 0.0f) continue;
        Vector3 p = jugadores[i].posicion;
        Color c = MezclaVolea(ColorEquipoVolea(m.estadosJugadores[i].equipo), WHITE, 0.4f);
        ResplandorFrenteVolea({ p.x, p.y + 0.3f, p.z }, 1.0f + 0.6f * (1.0f - d), AlfaVolea(c, 0.45f * d));
    }

    // Chispas con brillo.
    for (int i = 0; i < MAX_CHISPAS_VOLEA; i++)
    {
        const ChispaVolea& c = m.chispas[i];
        if (!c.activa || !c.brilla) continue;
        float vida = Limitar01Volea(c.vida / c.vidaMaxima);
        ResplandorFrenteVolea(c.posicion, c.tamano * (1.4f + vida), AlfaVolea(c.color, 0.95f * vida));
    }

    // Brasas que suben del lago y cruzan el estadio.
    int brasas = (int)(70.0f * FactorCalidadGrafica(CalidadParticulas()));
    for (int k = 0; k < brasas; k++)
    {
        float ciclo = 5.0f + 4.0f * RuidoVolea(k, 991);
        float u = std::fmod(t / ciclo + RuidoVolea(k, 992), 1.0f);
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        float x = k % 3 == 0
            ? (RuidoVolea(k, 993) - 0.5f) * 30.0f
            : lado * (PASILLO_X_VOLEA + 1.0f + 14.0f * RuidoVolea(k, 994));
        Vector3 p = { x + std::sin(t * 0.8f + (float)k) * 0.6f + u * 1.5f, Y_LAVA_VOLEA + 0.3f + 9.0f * u, -24.0f + 32.0f * RuidoVolea(k, 995) };
        float alfa = std::sin(u * 3.1416f);
        Color c = k % 4 == 0 ? COLOR_LAVA_CLARA_VOLEA : COLOR_LAVA_VOLEA;
        ResplandorFrenteVolea(p, 0.06f + 0.05f * RuidoVolea(k, 996), AlfaVolea(c, 0.9f * alfa));
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoVoleaMagma::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    const float t = tiempoVisual;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    int calidadLava = CalidadDecoracion() == CALIDAD_ALTA ? 2 : (decoracion ? 1 : 0);

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    Camera3D vista = camara;

    if (fase == FASE_VOLEA_PREPARACION && resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Limitar01Volea((tiempoPreparacion - 0.5f) / (DURACION_PREPARACION_VOLEA - 0.5f));
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 5.0f * suave;
        vista.position.z += 6.0f * suave;
        vista.target.y += 1.0f * suave;
    }

    // Ejes de la niebla y de los carteles que miran a camara.
    g_ojoVolea = vista.position;
    {
        Vector3 frente = NormalizarVolea(RestaVolea(vista.target, vista.position));
        g_frenteUVolea = NormalizarVolea(CruzVolea(frente, { 0.0f, 1.0f, 0.0f }));
        g_frenteVVolea = CruzVolea(g_frenteUVolea, frente);
    }

    // El bucle del juego no limpia la pantalla.
    ClearBackground(COLOR_CIELO_ALTO_VOLEA);
    DibujarCieloVolea(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte y lago.
    DibujarHorizonteVolea(t, decoracion);
    DibujarLagoLavaVolea(t, efectos, calidadLava);

    if (decoracion && efectos)
    {
        DibujarBurbujasLavaVolea(t);
    }

    // Media distancia: gradas, marcador y alrededores.
    DibujarIslaVolea(decoracion);
    DibujarGradasVolea(*this, t, decoracion, paso);
    DibujarMarcadorVolea(*this, t);

    if (decoracion)
    {
        DibujarAlrededoresVolea(t, efectos, paso);
    }

    // Arena y anillo cercano.
    DibujarCanchaVolea(t, efectos);

    if (decoracion)
    {
        DibujarUtileriaVolea(t, paso);
    }

    // Piezas jugables.
    DibujarRedVolea(tiempoAnimacion);
    DibujarCharcos(*this);
    DibujarJugadoresVolea(*this, jugadores, limite, participantes, mostrarDebug);
    DibujarLineaCaidaVolea(*this);
    DibujarPelota(*this);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_VOLEA);
    DibujarChispasSolidasVolea(*this);

    DibujarHumoVolea(*this, t, decoracion);

    if (efectos)
    {
        DibujarLucesVolea(*this, jugadores, limite, t, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo < 0 || !participantes[i].activo)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.75f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Avisos sobre la roca: saque, limite de toques y roca al rojo.
    if (fase == FASE_VOLEA_JUGANDO && pelota.estado != PELOTA_VOLEA_PUNTO)
    {
        Vector2 s = GetWorldToScreen({ pelota.posicion.x, pelota.posicion.y + 0.9f, pelota.posicion.z }, vista);
        if (s.y < Px(150.0f)) s.y = Px(150.0f);

        if (avisoToques > 0.0f && equipoUltimoToque >= 0)
        {
            DibujarTextoMundoContornoUI("¡MÁXIMO 2 TOQUES!", s.x, s.y, UI_TEXTO, Color{ 255, 120, 110, 255 }, Limitar01Volea(avisoToques / 0.3f));
        }
        else if (pelota.estado == PELOTA_VOLEA_SAQUE)
        {
            char texto[32];
            std::snprintf(texto, sizeof(texto), "SACA EL EQUIPO %d", equipoSaque + 1);
            float pulso = 0.8f + 0.2f * std::sin(t * 5.0f);
            DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO, MezclaVolea(ColorEquipoVolea(equipoSaque), WHITE, 0.3f), pulso);
        }
        else if (pelota.temperatura >= TEMPERATURA_SOBRECALENTADA)
        {
            float pulso = 0.7f + 0.3f * std::sin(t * 10.0f);
            DibujarTextoMundoContornoUI("¡AL ROJO!", s.x, s.y, UI_TEXTO_PEQUENO, Color{ 255, 190, 90, 255 }, pulso);
        }
    }

    // Anuncios grandes.
    if (tiempoAnuncioInicio >= 0.0f && fase == FASE_VOLEA_JUGANDO)
    {
        float u = tiempoAnuncioInicio / DURACION_ANUNCIO_VOLEA;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
        float y = (float)alto * 0.3f - u * Px(16.0f);
        DibujarTextoMundoContornoUI("¡A JUGAR!", (float)ancho * 0.5f, y, UI_TEXTO_TITULO, COLOR_LAVA_CLARA_VOLEA, alfa);
        DibujarTextoMundoContornoUI("PRIMERO A 7 PUNTOS", (float)ancho * 0.5f, y + Px(52.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }
    else if (tiempoMensaje > 0.0f && fase == FASE_VOLEA_JUGANDO)
    {
        float total = mensaje == MENSAJE_VOLEA_ORO ? 2.5f : (mensaje == MENSAJE_VOLEA_ENFRIA ? 1.6f : PAUSA_PUNTO_VOLEA);
        float u = Limitar01Volea(1.0f - tiempoMensaje / total);
        float alfa = u < 0.08f ? u / 0.08f : (tiempoMensaje < 0.3f ? tiempoMensaje / 0.3f : 1.0f);
        float salto = u < 0.15f && ObtenerFactorMovimientoCamaraMinijuegos() > 0.0f ? (1.0f - u / 0.15f) * Px(14.0f) : 0.0f;
        float y = (float)alto * 0.3f - salto;
        float cx = (float)ancho * 0.5f;

        if (mensaje == MENSAJE_VOLEA_PUNTO || mensaje == MENSAJE_VOLEA_CHARCO)
        {
            char texto[40];
            int equipo = equipoMensaje < 0 ? 0 : equipoMensaje;
            std::snprintf(texto, sizeof(texto), "¡PUNTO DEL EQUIPO %d!", equipo + 1);
            DibujarTextoMundoContornoUI(texto, cx, y, UI_TEXTO_TITULO, MezclaVolea(ColorEquipoVolea(equipo), WHITE, 0.25f), alfa);

            if (mensaje == MENSAJE_VOLEA_CHARCO)
            {
                DibujarTextoMundoContornoUI("¡CHARCO DE LAVA!", cx, y + Px(54.0f), UI_TEXTO_GRANDE, COLOR_LAVA_CLARA_VOLEA, alfa);
            }
        }
        else if (mensaje == MENSAJE_VOLEA_ENFRIA)
        {
            DibujarTextoMundoContornoUI("LA ROCA SE ENFRÍA", cx, y, UI_TEXTO_GRANDE, Color{ 170, 214, 255, 255 }, alfa);
        }
        else if (mensaje == MENSAJE_VOLEA_ORO)
        {
            DibujarTextoMundoContornoUI("¡PUNTO DE ORO!", cx, y, UI_TEXTO_TITULO, COLOR_ORO_VOLEA, alfa);
            DibujarTextoMundoContornoUI("EL PRÓXIMO PUNTO GANA", cx, y + Px(52.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
        }
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)ancho * 0.5f, (float)alto * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "VOLEA DE MAGMA",
        "PASÁ LA ROCA SOBRE LA RED: QUE CAIGA EN LA MITAD RIVAL",
        resultado
    );

    hud.fase = fase == FASE_VOLEA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_VOLEA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = modoOro ? DURACION_PUNTO_ORO_VOLEA : DURACION_PARTIDO_VOLEA;

    for (int i = 0; i < limite; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        const EstadoJugadorVolea& estado = estadosJugadores[i];

        if (!h.visible || estado.equipo < 0)
        {
            continue;
        }

        int total = puntos[estado.equipo];
        h.equipo = estado.equipo;
        h.rol = estado.equipo == 0 ? "EQUIPO 1" : "EQUIPO 2";
        std::snprintf(h.valor, sizeof(h.valor), "%d %s", total, total == 1 ? "PUNTO" : "PUNTOS");
        h.valorNumerico = total;

        if (fase == FASE_VOLEA_JUGANDO)
        {
            bool enSuMitad = (pelota.posicion.x < 0.0f) == (estado.equipo == 0);

            if (avisoToques > 0.0f && equipoUltimoToque == estado.equipo)
            {
                std::snprintf(h.estado, sizeof(h.estado), "¡YA VAN 2 TOQUES!");
            }
            else if (pelota.estado == PELOTA_VOLEA_SAQUE && equipoSaque == estado.equipo)
            {
                std::snprintf(h.estado, sizeof(h.estado), "¡SACAN!");
            }
            else if (pelota.estado == PELOTA_VOLEA_EN_JUEGO && enSuMitad && toques[estado.equipo] > 0)
            {
                std::snprintf(h.estado, sizeof(h.estado), "TOQUE %d DE 2", toques[estado.equipo]);
            }
        }

        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "EQUIPO %d", estado.equipo + 1);

        if (!JugadorEsBot(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "MOVÉ: %s  GOLPE: %s\nSALTÁ Y GOLPEÁ: REMATE",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (fase == FASE_VOLEA_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        static char nota[56];
        hud.finPorTiempo = puntos[0] < PUNTOS_PARA_GANAR_VOLEA && puntos[1] < PUNTOS_PARA_GANAR_VOLEA && !modoOro;
        std::snprintf(nota, sizeof(nota), "EQUIPO 1  %d - %d  EQUIPO 2", puntos[0], puntos[1]);
        hud.notaFinal = nota;

        if (!empate)
        {
            hud.tituloFinal = equipoGanador == 0 ? "¡GANA EL EQUIPO 1!" : "¡GANA EL EQUIPO 2!";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoVoleaMagma::ObtenerResultado() const
{
    return resultado;
}
