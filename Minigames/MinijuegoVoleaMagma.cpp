#include "Minigames/MinijuegoVoleaMagma.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

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

static const Color COLOR_EQUIPO_0 = { 255, 150, 40, 255 };
static const Color COLOR_EQUIPO_1 = { 70, 200, 255, 255 };


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

    camara.position = { 0.0f, 9.0f, 14.5f };
    camara.target = { 0.0f, 1.6f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_VOLEA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_VOLEA;
    tiempoRestante = DURACION_PARTIDO_VOLEA;
    tiempoAnimacion = 0.0f;

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
    CargarPaqueteVoleaMagmaRetro3D();

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

void MinijuegoVoleaMagma::Actualizar(
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
// VISUAL: GLB COMPARTIDOS CON FALLBACK POR PIEZA
//==================================================
// La logica (cancha, red, reglas de toques, puntos, IA) no depende de esta
// decoracion.
//==================================================

static float Hash01(int n)
{
    float v = std::sin((float)n * 12.9898f) * 43758.5453f;

    return v - std::floor(v);
}


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


static void DibujarLagoLava(float t)
{
    if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_LAGO, {0,0,0}))
        (DrawCube)({ 0.0f, -1.7f, -20.0f }, 120.0f, 0.4f, 90.0f, Color{ 255, 86, 20, 255 });

    // Burbujas y ondas de lava.
    for (int i = 0; i < 16; i++)
    {
        float x = -26.0f + 52.0f * Hash01(i * 5 + 1);
        float z = -26.0f + 36.0f * Hash01(i * 5 + 2);
        float fase = std::sin(t * (0.8f + Hash01(i) * 1.2f) + (float)i * 1.7f);

        // Evita dibujar burbujas sobre la cancha.
        if (std::fabs(x) < 11.0f && std::fabs(z) < 7.0f)
        {
            continue;
        }

        Vector3 p = {x,-1.45f + 0.18f * fase,z};
        float radio = 0.5f + 0.15f * fase;
        float escala = radio / 0.48f;
        Color color = {255,(unsigned char)(150 + 60 * fase),40,255};
        if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_BURBUJA, p, {escala,escala,escala}, color))
            (DrawSphere)(p, radio, color);
    }
}


static void DibujarVolcan(float x, float z, float alto, float radio, float t, int semilla)
{
    ModeloVoleaMagma3D pieza = alto > 20.0f ? MODELO_VOLEA_VOLCAN_MAYOR : MODELO_VOLEA_VOLCAN_MENOR;
    if (!DibujarModeloVoleaMagmaRetro3D(pieza, {x,-1.5f,z}))
    {
        (DrawCylinder)({ x, -1.5f, z }, radio * 0.12f, radio, alto, 14, Color{ 52, 38, 36, 255 });
        (DrawSphere)({ x, -1.5f + alto, z }, radio * 0.14f, Color{ 255, 120, 30, 255 });

        // Coladas de lava por la ladera.
        for (int k = 0; k < 3; k++)
        {
            float angulo = (float)k * 2.1f + (float)semilla;
            DrawCylinderEx(
                { x + std::cos(angulo) * radio * 0.1f, -1.5f + alto, z + std::sin(angulo) * radio * 0.1f },
                { x + std::cos(angulo) * radio * 0.75f, -1.5f + alto * 0.22f, z + std::sin(angulo) * radio * 0.75f },
                0.25f, 0.5f, 6, Color{ 255, 100, 30, 255 }
            );
        }
    }

    // Humo que sube y se desvanece.
    for (int k = 0; k < 6; k++)
    {
        float fase = std::fmod(t * 0.12f + (float)k / 6.0f, 1.0f);
        float altura = -1.5f + alto + fase * 18.0f;
        float radioHumo = 1.5f + 3.5f * fase;

        // El humo no proyecta una mancha humana sobre la cancha.
        (DrawSphere)(
            { x + 4.0f * fase * (float)(k % 2 == 0 ? 1 : -1), altura, z },
            radioHumo,
            Color{ 70, 64, 66, (unsigned char)(120.0f * (1.0f - fase)) }
        );
    }
}


static void DibujarEscenarioVolcan(float t)
{
    DibujarLagoLava(t);
    DibujarVolcan(-28.0f, -38.0f, 20.0f, 16.0f, t, 1);
    DibujarVolcan(26.0f, -44.0f, 26.0f, 20.0f, t, 4);

    // Columnas de basalto hexagonales alrededor de la cancha.
    for (int i = 0; i < 10; i++)
    {
        float x = -15.0f + 3.4f * (float)i;
        float alto = 2.5f + 4.5f * Hash01(i * 3 + 7);
        if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_COLUMNA,
            {x,-1.5f,-9.0f - Hash01(i + 40) * 2.0f}, {1,alto / 4.0f,1}))
        {
            (DrawCylinder)({ x, -1.5f, -9.0f - Hash01(i + 40) * 2.0f }, 1.0f, 1.2f, alto, 6, Color{ 44, 42, 52, 255 });
            (DrawCylinder)({ x, -1.5f + alto, -9.0f - Hash01(i + 40) * 2.0f }, 0.9f, 1.0f, 0.15f, 6, Color{ 80, 74, 88, 255 });
        }
    }

    for (int i = 0; i < 4; i++)
    {
        float lado = i % 2 == 0 ? -1.0f : 1.0f;
        float z = i < 2 ? -3.5f : 3.5f;
        float alto = 2.0f + 2.5f * Hash01(i + 90);
        if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_COLUMNA_LLAMA,
            {lado * 12.2f,-1.5f,z}, {1,alto / 4.0f,1}))
        {
            (DrawCylinder)({ lado * 12.2f, -1.5f, z }, 0.8f, 1.0f, alto, 6, Color{ 44, 42, 52, 255 });
            (DrawSphere)({ lado * 12.2f, -1.5f + alto + 0.3f, z }, 0.28f, Color{ 255, 140, 40, 255 });
        }
    }

    // Ceniza cayendo.
    for (int i = 0; i < 40; i++)
    {
        float velocidad = 0.5f + 0.7f * Hash01(i * 3 + 1);
        float fase = std::fmod(t * velocidad * 0.35f + Hash01(i * 3 + 2), 1.0f);
        float x = -16.0f + 32.0f * Hash01(i * 3 + 3) + 1.5f * std::sin(t + (float)i);
        float z = -12.0f + 22.0f * Hash01(i * 7 + 5);

        Vector3 p = {x,14.0f - 15.0f * fase,z};
        Color color = ColorMaterialVoleaMagmaRetro3D(MODELO_VOLEA_CENIZA, 0);
        color.a = 200;
        if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_CENIZA, p, {1,1,1}, color, 0, true))
            (DrawCube)(p, 0.09f, 0.09f, 0.09f, Color{190,180,176,200});
    }
}


static void DibujarCancha(float t)
{
    // Losa de obsidiana, zona de juego y lineas.
    if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_CANCHA, {0,0,0}))
    {
        (DrawCube)({ 0.0f, -0.5f, 0.0f }, 2.0f * MITAD_CANCHA_X + 2.0f, 1.0f, 2.0f * MITAD_CANCHA_Z + 2.0f, Color{ 30, 26, 38, 255 });
        (DrawCube)({ 0.0f, 0.01f, 0.0f }, 2.0f * MITAD_CANCHA_X, 0.02f, 2.0f * MITAD_CANCHA_Z, Color{ 50, 44, 62, 255 });

        Color linea = Color{ 255, 130, 50, 255 };
        (DrawCube)({ 0.0f, 0.03f, -MITAD_CANCHA_Z }, 2.0f * MITAD_CANCHA_X, 0.03f, 0.14f, linea);
        (DrawCube)({ 0.0f, 0.03f, MITAD_CANCHA_Z }, 2.0f * MITAD_CANCHA_X, 0.03f, 0.14f, linea);
        (DrawCube)({ -MITAD_CANCHA_X, 0.03f, 0.0f }, 0.14f, 0.03f, 2.0f * MITAD_CANCHA_Z, linea);
        (DrawCube)({ MITAD_CANCHA_X, 0.03f, 0.0f }, 0.14f, 0.03f, 2.0f * MITAD_CANCHA_Z, linea);
        (DrawCube)({ 0.0f, 0.03f, 0.0f }, 0.14f, 0.03f, 2.0f * MITAD_CANCHA_Z, Color{ 255, 220, 120, 255 });
    }

    // El borde modular completa el perimetro de la plataforma existente.
    // Respaldo propio: no volver a dibujar la losa si solo falla este GLB.
    if (!DibujarModeloVoleaMagmaRetro3D(MODELO_VOLEA_BORDE, {0,0,0}))
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            (DrawCube)({0,0.16f,lado * 5.9f},20,0.3f,0.22f,Color{53,51,64,255});
            (DrawCube)({0,0.31f,lado * 5.9f},20,0.035f,0.12f,Color{255,152,55,255});
            (DrawCube)({lado * 9.9f,0.16f,0},0.22f,0.3f,12,Color{53,51,64,255});
            (DrawCube)({lado * 9.9f,0.31f,0},0.12f,0.035f,12,Color{255,152,55,255});
        }
    }

    // Marcas de cada mitad con el color del equipo.
    (DrawCube)({ -MITAD_CANCHA_X * 0.5f, 0.025f, 0.0f }, MITAD_CANCHA_X - 0.6f, 0.02f, 2.0f * MITAD_CANCHA_Z - 0.6f, Fade(COLOR_EQUIPO_0, 0.12f));
    (DrawCube)({ MITAD_CANCHA_X * 0.5f, 0.025f, 0.0f }, MITAD_CANCHA_X - 0.6f, 0.02f, 2.0f * MITAD_CANCHA_Z - 0.6f, Fade(COLOR_EQUIPO_1, 0.12f));

    // Red de cadenas incandescentes.
    float brillo = 0.5f + 0.5f * std::sin(t * 4.0f);
    Color cadena = Color{ 255, (unsigned char)(110 + 60 * brillo), 40, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float z = (float)lado * (MITAD_CANCHA_Z + 0.4f);
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

    if (caliente)
    {
        // Halo conservado; no agrega otra sombra ademas de sombra_pelota.
        (DrawSphere)(pelota.posicion, RADIO_PELOTA * 1.5f, Color{ 255, 150, 40, (unsigned char)(50 + 40 * (0.5f + 0.5f * std::sin(t * 10.0f))) });
    }

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


//==================================================
// DIBUJO
//==================================================

static void ListaJugadoresEquipo(
    const MinijuegoVoleaMagma& m,
    int equipo,
    const Participante participantes[],
    int limite,
    char* destino,
    int capacidad
)
{
    destino[0] = '\0';
    int escrito = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.estadosJugadores[i].equipo != equipo)
        {
            continue;
        }

        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        int n = std::snprintf(
            destino + escrito,
            (size_t)(capacidad - escrito),
            "%sJ%d%s",
            escrito > 0 ? "  " : "",
            numero,
            participantes[i].esBot ? "B" : ""
        );

        if (n < 0 || n >= capacidad - escrito)
        {
            break;
        }

        escrito += n;
    }
}


static const char* TextoBotonGolpe(const Participante& participante)
{
    if (participante.esBot || !participante.conectado) return "AUTO";
    if (participante.control == CONTROL_GAMEPAD) return "B";
    if (participante.control == CONTROL_TECLADO_FLECHAS) return "SHIFT DER";

    return "E";
}


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

    ClearBackground(Color{ 34, 14, 14, 255 });
    BeginMode3D(camara);

    DibujarEscenarioVolcan(tiempoAnimacion);
    DibujarCancha(tiempoAnimacion);
    DibujarCharcos(*this);

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorVolea& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];
        Color colorEquipo = ColorEquipoVolea(estado.equipo);

        DrawCircle3D({ jugador.posicion.x, 0.06f, jugador.posicion.z }, 0.62f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);
        DrawCircle3D({ jugador.posicion.x, 0.06f, jugador.posicion.z }, 0.52f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);

        // Alcance del golpe (solo depuracion).
        if (mostrarDebug)
        {
            DrawCircle3D(
                { jugador.posicion.x, 0.08f, jugador.posicion.z },
                AlcanceHorizontal(*this, estado.equipo),
                { 1.0f, 0.0f, 0.0f }, 90.0f, LIME
            );
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = colorEquipo;
        DibujarJugadorCuboPrueba(jugador, visual);
    }

    DibujarPelota(*this);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_VOLEA);

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    // Marcador central.
    char equipo0[32];
    char equipo1[32];
    ListaJugadoresEquipo(*this, 0, participantes, limite, equipo0, 32);
    ListaJugadoresEquipo(*this, 1, participantes, limite, equipo1, 32);

    int centro = anchoPantalla / 2;
    DrawRectangle(centro - 230, 8, 460, 78, Fade(BLACK, 0.65f));
    DrawRectangle(centro - 230, 8, 8, 78, COLOR_EQUIPO_0);
    DrawRectangle(centro + 222, 8, 8, 78, COLOR_EQUIPO_1);

    const char* puntaje = TextFormat("%d  -  %d", puntos[0], puntos[1]);
    DrawText(puntaje, centro - MeasureText(puntaje, 44) / 2, 12, 44, RAYWHITE);
    DrawText("EQUIPO 1", centro - 216, 14, 16, COLOR_EQUIPO_0);
    DrawText(equipo0, centro - 216, 34, 16, LIGHTGRAY);
    DrawText("EQUIPO 2", centro + 214 - MeasureText("EQUIPO 2", 16), 14, 16, COLOR_EQUIPO_1);
    DrawText(equipo1, centro + 214 - MeasureText(equipo1, 16), 34, 16, LIGHTGRAY);

    const char* textoTiempo = modoOro
        ? TextFormat("PUNTO DE ORO  %02d", (int)std::ceil(tiempoRestante))
        : TextFormat("%02d", (int)std::ceil(tiempoRestante));
    DrawText(
        textoTiempo,
        centro - MeasureText(textoTiempo, 22) / 2,
        58,
        22,
        modoOro || tiempoRestante <= 6.0f ? RED : GOLD
    );

    DrawText("VOLEA DE MAGMA", 28, 20, 26, GOLD);
    DrawText(TextFormat("PRIMERO A %d PUNTOS", PUNTOS_PARA_GANAR_VOLEA), 28, 50, 16, Color{ 255, 190, 120, 255 });

    // Termometro de la roca.
    int barraX = anchoPantalla - 190;
    DrawText("ROCA", barraX, 20, 16, LIGHTGRAY);
    DrawRectangle(barraX, 40, 150, 12, Fade(BLACK, 0.7f));
    DrawRectangle(barraX, 40, (int)(150.0f * pelota.temperatura), 12, ColorRoca(pelota.temperatura, tiempoAnimacion));
    DrawRectangleLines(barraX, 40, 150, 12, LIGHTGRAY);

    if (pelota.temperatura >= TEMPERATURA_SOBRECALENTADA)
    {
        DrawText("SOBRECALENTADA", barraX, 56, 16, ORANGE);
    }

    // Nombres sobre los jugadores.

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorVolea& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }


        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.3f, jugadores[i].posicion.z },
            camara
        );
        const char* nombre = TextFormat(
            "J%d%s",
            participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1,
            participantes[i].esBot ? " BOT" : ""
        );
        DrawText(nombre, (int)pantalla.x - MeasureText(nombre, 18) / 2, (int)pantalla.y, 18, ColorEquipoVolea(estado.equipo));
    }

    // Toques restantes de cada equipo y ayuda de controles.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        int cx = equipo == 0 ? anchoPantalla / 4 : (anchoPantalla * 3) / 4;
        bool aviso = avisoToques > 0.0f && equipoUltimoToque == equipo;
        const char* texto = aviso
            ? "MAXIMO 2 TOQUES"
            : TextFormat("TOQUES %d/2", toques[equipo]);
        DrawText(
            texto,
            cx - MeasureText(texto, 20) / 2,
            altoPantalla - 70,
            20,
            aviso ? RED : ColorEquipoVolea(equipo)
        );
    }

    const char* ayuda = "MOVER: WASD / FLECHAS / STICK   SALTAR: ESPACIO / ENTER / A   GOLPE: E / SHIFT DER / B (apunta con la direccion)   SALTO + GOLPE = REMATE";
    int ayudaAncho = MeasureText(ayuda, 14);
    DrawRectangle(centro - ayudaAncho / 2 - 12, altoPantalla - 34, ayudaAncho + 24, 28, Fade(BLACK, 0.78f));
    DrawText(ayuda, centro - ayudaAncho / 2, altoPantalla - 28, 14, RAYWHITE);

    // Etiqueta del boton de golpe de cada humano durante la cuenta atras.
    if (fase == FASE_VOLEA_PREPARACION)
    {
        int fila = 0;

        for (int i = 0; i < limite; i++)
        {
            if (estadosJugadores[i].equipo < 0 || JugadorEsBot(participantes[i]))
            {
                continue;
            }

            DrawText(
                TextFormat("J%d GOLPE: %s", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1, TextoBotonGolpe(participantes[i])),
                28,
                altoPantalla / 2 + 40 + fila * 22,
                20,
                ColorEquipoVolea(estadosJugadores[i].equipo)
            );
            fila++;
        }
    }

    if (tiempoMensaje > 0.0f && fase == FASE_VOLEA_JUGANDO)
    {
        const char* texto = "";
        Color colorTexto = GOLD;

        if (mensaje == MENSAJE_VOLEA_PUNTO || mensaje == MENSAJE_VOLEA_CHARCO)
        {
            texto = TextFormat("PUNTO EQUIPO %d", equipoMensaje + 1);
            colorTexto = ColorEquipoVolea(equipoMensaje < 0 ? 0 : equipoMensaje);
        }
        else if (mensaje == MENSAJE_VOLEA_ENFRIA)
        {
            texto = "LA ROCA SE ENFRIA";
            colorTexto = Color{ 160, 200, 255, 255 };
        }
        else if (mensaje == MENSAJE_VOLEA_ORO)
        {
            texto = "PUNTO DE ORO";
        }

        DrawText(texto, centro - MeasureText(texto, 40) / 2, altoPantalla / 2 - 150, 40, colorTexto);

        if (mensaje == MENSAJE_VOLEA_CHARCO)
        {
            const char* extra = "CHARCO DE LAVA!";
            DrawText(extra, centro - MeasureText(extra, 22) / 2, altoPantalla / 2 - 106, 22, ORANGE);
        }
    }

    if (fase == FASE_VOLEA_JUGANDO && pelota.estado == PELOTA_VOLEA_SAQUE)
    {
        const char* texto = TextFormat("SAQUE DEL EQUIPO %d", equipoSaque + 1);
        DrawText(texto, centro - MeasureText(texto, 22) / 2, altoPantalla / 2 - 190, 22, ColorEquipoVolea(equipoSaque));
    }

    if (fase == FASE_VOLEA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, centro - MeasureText(texto, 96) / 2, altoPantalla / 2 - 160, 96, GOLD);
    }
    else if (
        fase == FASE_VOLEA_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 520;
        int panelAlto = 190;
        int px = centro - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 40;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        const char* titulo = empate
            ? "EMPATE EN EL VOLCAN"
            : TextFormat("GANA EL EQUIPO %d", equipoGanador + 1);
        DrawText(
            titulo,
            centro - MeasureText(titulo, 32) / 2,
            py + 16,
            32,
            empate ? YELLOW : ColorEquipoVolea(equipoGanador)
        );

        const char* marcador = TextFormat("%d  -  %d", puntos[0], puntos[1]);
        DrawText(marcador, centro - MeasureText(marcador, 48) / 2, py + 62, 48, RAYWHITE);
        DrawText(equipo0, px + 24, py + 124, 18, COLOR_EQUIPO_0);
        DrawText(equipo1, px + panelAncho - 24 - MeasureText(equipo1, 18), py + 124, 18, COLOR_EQUIPO_1);
        DrawText(
            TextoReinicioMinijuego(),
            centro - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }

}


const ResultadoMinijuego& MinijuegoVoleaMagma::ObtenerResultado() const
{
    return resultado;
}
