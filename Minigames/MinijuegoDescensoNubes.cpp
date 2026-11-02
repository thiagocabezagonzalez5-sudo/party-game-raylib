#include "Minigames/MinijuegoDescensoNubes.h"

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
#include <cstdlib>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_NUBES = 3.0f;
static const float TIEMPO_LIMITE_NUBES = 52.0f;

// Caida: ~40 s de la altura inicial al suelo a velocidad base.
static const float ALTURA_INICIAL_NUBES = 100.0f;
static const float VELOCIDAD_CAIDA_NUBES = 2.5f;
static const float MULTIPLICADOR_FRENO_NUBES = 0.5f;
static const float MAXIMO_ADELANTO_NUBES = 6.0f;

static const float DURACION_FRENO_NUBES = 1.0f;
static const float RECARGA_FRENO_NUBES = 2.0f;

// Movimiento horizontal dentro del cilindro de caida.
static const float RADIO_CILINDRO_NUBES = 6.0f;
static const float ACELERACION_NUBES = 16.0f;
static const float ROZAMIENTO_NUBES = 3.2f;
static const float VELOCIDAD_MAXIMA_NUBES = 6.5f;
static const float ACELERACION_VIENTO_NUBES = 11.0f;
static const float MITAD_BANDA_VIENTO_NUBES = 1.6f;

// Objetos.
static const float RADIO_RECOGER_NUBES = 1.0f;
static const float DURACION_ATURDIMIENTO_NUBES = 0.8f;
static const int PENALIZACION_TORMENTA_NUBES = 2;
static const float MITAD_ALTO_TORMENTA_NUBES = 0.9f;

// Aterrizaje.
static const float RADIO_ISLA_NUBES = 12.0f;  // solo visual (el cilindro limita)
static const float RADIO_CENTRO_NUBES = 1.8f;
static const int BONUS_CENTRO_NUBES = 5;

static const float ALTURA_CENTRO_JUGADOR_NUBES = 0.72f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarNubes(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioNubes(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


// Pseudoaleatorio estable para decoracion (no consume el generador global).
static float HashNubes(int k, int semilla)
{
    float v = std::sin((float)k * 12.9898f + (float)semilla * 78.233f) * 43758.5453f;
    return v - std::floor(v);
}


static int LimiteNubes(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


// Solo las piezas jugables (planeadores, anillos, estrella, tormenta,
// viento y diana) usan GLB; el decorado de ambiente se arma con
// primitivas y sus GLB no se cargan.
static void CargarPiezasJugablesNubes()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteDescensoNubesRetro3D();
    const int tramos[3][2] =
    {
        { MODELO_NUBES_PLANEADOR, MODELO_NUBES_ESTRELLA },
        { MODELO_NUBES_TORMENTA, MODELO_NUBES_BANDA },
        { MODELO_NUBES_DIANA, MODELO_NUBES_DIANA }
    };
    // Mallas por pieza segun el manifest (igual que el paquete completo).
    const int mallasEsperadas[CANTIDAD_MODELOS_DESCENSO_NUBES_3D] =
        { 6, 6, 3, 3, 2, 3, 5, 3, 2, 2, 4, 3, 4, 5, 4, 4, 4, 4, 7 };

    for (const auto& tramo : tramos)
    {
        PaqueteModelosEscenarioRetro3D parte = paquete;
        parte.recursos += tramo[0];
        parte.definiciones += tramo[0];
        parte.cantidad = tramo[1] - tramo[0] + 1;
        CargarPaqueteModelosEscenarioRetro3D(parte);

        for (int i = tramo[0]; i <= tramo[1]; i++)
        {
            RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];

            if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i])
            {
                continue;
            }

            UnloadModel(recurso.modelo);
            recurso.modelo = {};
            recurso.cargado = false;
            TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
        }
    }
}


static void AsignarColoresNubes(
    MinijuegoDescensoNubes& m,
    const Participante participantes[]
)
{
    const Color respaldo[] =
    {
        Color{ 232, 62, 62, 255 },
        Color{ 62, 124, 238, 255 },
        Color{ 66, 202, 96, 255 },
        Color{ 246, 206, 52, 255 },
        Color{ 232, 92, 204, 255 },
        Color{ 250, 144, 44, 255 }
    };
    const int cantidadRespaldo = (int)(sizeof(respaldo) / sizeof(respaldo[0]));

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.coloresJugadores[i] = participantes[i].color;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!participantes[i].activo || !participantes[i].esBot)
        {
            continue;
        }

        for (int c = 0; c < cantidadRespaldo; c++)
        {
            bool repetido = false;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (j == i || !participantes[j].activo)
                {
                    continue;
                }

                const Color& otro = (!participantes[j].esBot || j < i)
                    ? m.coloresJugadores[j]
                    : participantes[j].color;

                // Parecido, no solo identico: el rojo de un humano y el
                // rojo de respaldo no deben convivir en el cielo.
                int diferencia =
                    std::abs((int)otro.r - (int)respaldo[c].r) +
                    std::abs((int)otro.g - (int)respaldo[c].g) +
                    std::abs((int)otro.b - (int)respaldo[c].b);

                if (diferencia < 90)
                {
                    repetido = true;
                }
            }

            if (!repetido)
            {
                m.coloresJugadores[i] = respaldo[c];
                break;
            }
        }
    }
}


//==================================================
// LOGICA: NIVEL (anillos, tormentas, vientos)
//==================================================

static void GenerarNivelNubes(MinijuegoDescensoNubes& m)
{
    // Dos senderos de anillos que serpentean; el jugador elige cual seguir.
    float sx[2] = { AleatorioNubes(-3.0f, 0.0f), AleatorioNubes(0.0f, 3.0f) };
    float sz[2] = { AleatorioNubes(-3.0f, 3.0f), AleatorioNubes(-3.0f, 3.0f) };

    m.cantidadAnillos = 60;

    for (int k = 0; k < m.cantidadAnillos; k++)
    {
        int sendero = k % 2;

        sx[sendero] += AleatorioNubes(-2.4f, 2.4f);
        sz[sendero] += AleatorioNubes(-2.4f, 2.4f);

        float radio = std::sqrt(sx[sendero] * sx[sendero] + sz[sendero] * sz[sendero]);

        if (radio > 5.2f)
        {
            sx[sendero] *= 5.2f / radio;
            sz[sendero] *= 5.2f / radio;
        }

        AnilloNubes& anillo = m.anillos[k];
        anillo = {};
        anillo.posicion =
        {
            sx[sendero],
            94.0f - (float)k * 1.45f - AleatorioNubes(0.0f, 0.5f),
            sz[sendero]
        };
        anillo.valor = GetRandomValue(0, 99) < 12 ? 3 : 1;
    }

    m.cantidadTormentas = 14;

    for (int j = 0; j < m.cantidadTormentas; j++)
    {
        TormentaNubes& tormenta = m.tormentas[j];
        tormenta = {};
        float altura = 90.0f - (float)j * 5.4f - AleatorioNubes(0.0f, 2.0f);

        for (int intento = 0; intento < 12; intento++)
        {
            float angulo = AleatorioNubes(0.0f, 2.0f * PI);
            float radio = AleatorioNubes(0.0f, 5.0f);
            tormenta.posicion = { std::cos(angulo) * radio, altura, std::sin(angulo) * radio };

            bool libre = true;

            for (int k = 0; k < m.cantidadAnillos; k++)
            {
                float dx = m.anillos[k].posicion.x - tormenta.posicion.x;
                float dz = m.anillos[k].posicion.z - tormenta.posicion.z;

                if (
                    std::fabs(m.anillos[k].posicion.y - altura) < 1.5f &&
                    std::sqrt(dx * dx + dz * dz) < 2.2f
                )
                {
                    libre = false;
                }
            }

            if (libre)
            {
                break;
            }
        }
    }

    const float alturasViento[5] = { 85.0f, 68.0f, 52.0f, 36.0f, 22.0f };
    m.cantidadVientos = 5;

    for (int v = 0; v < m.cantidadVientos; v++)
    {
        int direccion = GetRandomValue(0, 3);
        m.vientos[v].altura = alturasViento[v] + AleatorioNubes(-3.0f, 3.0f);
        m.vientos[v].dirX = direccion == 0 ? 1.0f : (direccion == 1 ? -1.0f : 0.0f);
        m.vientos[v].dirZ = direccion == 2 ? 1.0f : (direccion == 3 ? -1.0f : 0.0f);
    }
}


static float AlturaMediaNubes(const MinijuegoDescensoNubes& m, int limite)
{
    float suma = 0.0f;
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.participa[i] && !m.estados[i].aterrizo)
        {
            suma += m.estados[i].altura;
            cantidad++;
        }
    }

    return cantidad > 0 ? suma / (float)cantidad : 0.0f;
}


static void AterrizarNubes(
    MinijuegoDescensoNubes& m,
    int indice,
    JugadorPrueba& jugador
)
{
    EstadoJugadorNubes& estado = m.estados[indice];

    estado.altura = 0.0f;
    estado.aterrizo = true;
    estado.velX = 0.0f;
    estado.velZ = 0.0f;
    estado.frenando = 0.0f;
    estado.aturdido = 0.0f;

    float distancia = std::sqrt(
        jugador.posicion.x * jugador.posicion.x +
        jugador.posicion.z * jugador.posicion.z
    );

    if (distancia <= RADIO_CENTRO_NUBES)
    {
        estado.bonusCentro = true;
        estado.puntos += BONUS_CENTRO_NUBES;
    }

    jugador.posicion.y = ALTURA_CENTRO_JUGADOR_NUBES;
    jugador.velocidad = {};
    ReproducirSonidoMinijuego(m.audio, SONIDO_ATERRIZAJE);
}


static void FinalizarNubes(MinijuegoDescensoNubes& m, int limite)
{
    int mejores = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < limite; j++)
        {
            if (j == i || !m.participa[j])
            {
                continue;
            }

            const EstadoJugadorNubes& a = m.estados[j];
            const EstadoJugadorNubes& b = m.estados[i];

            if (a.puntos > b.puntos || (a.puntos == b.puntos && a.golpes < b.golpes))
            {
                posicion++;
            }
        }

        m.resultado.participantes[i].posicionFinal = posicion;
        m.resultado.participantes[i].puntuacionMinijuego = m.estados[i].puntos;

        if (posicion == 1)
        {
            mejores++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = mejores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;
    m.fase = FASE_NUBES_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// IA DE BOTS
//==================================================

static void ReevaluarBotNubes(
    MinijuegoDescensoNubes& m,
    int indice,
    const JugadorPrueba& jugador
)
{
    EstadoJugadorNubes& estado = m.estados[indice];
    float altura = estado.altura;
    bool puedeFrenar = estado.recargaFreno <= 0.0f && estado.frenando <= 0.0f;

    int mejor = -1;
    float mejorPuntaje = -1.0f;
    bool mejorNecesitaFreno = false;

    for (int k = 0; k < m.cantidadAnillos; k++)
    {
        const AnilloNubes& anillo = m.anillos[k];

        if ((anillo.recogidoPor & (1u << indice)) != 0u)
        {
            continue;
        }

        float dy = altura - anillo.posicion.y;

        if (dy < 0.3f || dy > 14.0f)
        {
            continue;
        }

        float dx = anillo.posicion.x - jugador.posicion.x;
        float dz = anillo.posicion.z - jugador.posicion.z;
        float distancia = std::sqrt(dx * dx + dz * dz);
        float tiempo = dy / VELOCIDAD_CAIDA_NUBES;
        float alcance = tiempo * 4.2f + 0.8f;
        float extra = puedeFrenar ? 2.2f : 0.0f;

        if (distancia > alcance + extra)
        {
            continue;
        }

        // Descarta anillos protegidos por una tormenta.
        bool peligroso = false;

        for (int t = 0; t < m.cantidadTormentas; t++)
        {
            const TormentaNubes& tormenta = m.tormentas[t];
            float tx = tormenta.posicion.x - anillo.posicion.x;
            float tz = tormenta.posicion.z - anillo.posicion.z;

            if (
                std::fabs(tormenta.posicion.y - anillo.posicion.y) < 1.6f &&
                std::sqrt(tx * tx + tz * tz) < tormenta.radio + 0.7f
            )
            {
                peligroso = true;
            }
        }

        if (peligroso)
        {
            continue;
        }

        float puntaje = (float)anillo.valor / (0.5f + tiempo * 0.3f + distancia * 0.1f);

        if (puntaje > mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejor = k;
            mejorNecesitaFreno = distancia > alcance;
        }
    }

    estado.botFrenar = false;

    if (altura < 8.0f)
    {
        estado.botObjetivoX = AleatorioNubes(-0.8f, 0.8f);
        estado.botObjetivoZ = AleatorioNubes(-0.8f, 0.8f);
    }
    else if (mejor >= 0)
    {
        estado.botObjetivoX = m.anillos[mejor].posicion.x + AleatorioNubes(-0.4f, 0.4f);
        estado.botObjetivoZ = m.anillos[mejor].posicion.z + AleatorioNubes(-0.4f, 0.4f);
        estado.botFrenar = mejorNecesitaFreno;
    }
    else
    {
        estado.botObjetivoX = jugador.posicion.x * 0.7f + AleatorioNubes(-1.0f, 1.0f);
        estado.botObjetivoZ = jugador.posicion.z * 0.7f + AleatorioNubes(-1.0f, 1.0f);
    }
}


static InputMinijuegoParticipante CrearEntradaBotNubes(
    MinijuegoDescensoNubes& m,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorNubes& estado = m.estados[indice];

    estado.botReevaluar -= deltaTime;

    if (estado.botReevaluar <= 0.0f)
    {
        ReevaluarBotNubes(m, indice, jugador);
        estado.botReevaluar = 0.3f;
    }

    float dx = estado.botObjetivoX - jugador.posicion.x;
    float dz = estado.botObjetivoZ - jugador.posicion.z;

    // Repulsion de tormentas cercanas por debajo.
    for (int t = 0; t < m.cantidadTormentas; t++)
    {
        const TormentaNubes& tormenta = m.tormentas[t];
        float dy = estado.altura - tormenta.posicion.y;

        if (dy < -0.5f || dy > 7.0f || (tormenta.golpeadoA & (1u << indice)) != 0u)
        {
            continue;
        }

        float rx = jugador.posicion.x - tormenta.posicion.x;
        float rz = jugador.posicion.z - tormenta.posicion.z;
        float distancia = std::sqrt(rx * rx + rz * rz);
        float margen = tormenta.radio + 1.8f;

        if (distancia < margen && distancia > 0.01f)
        {
            dx += rx / distancia * (margen - distancia) * 2.5f;
            dz += rz / distancia * (margen - distancia) * 2.5f;
        }
    }

    float longitud = std::sqrt(dx * dx + dz * dz);

    if (longitud > 0.25f)
    {
        const float umbral = 0.38f;
        entrada.derecha = dx / longitud > umbral;
        entrada.izquierda = dx / longitud < -umbral;
        entrada.atras = dz / longitud > umbral;
        entrada.adelante = dz / longitud < -umbral;
    }

    if (estado.botFrenar && estado.recargaFreno <= 0.0f && estado.frenando <= 0.0f)
    {
        entrada.golpear = true;
        estado.botFrenar = false;
    }

    return entrada;
}


//==================================================
// CICLO DE VIDA
//==================================================

void MinijuegoDescensoNubes::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estados[i] = {};
        participa[i] = false;
        coloresJugadores[i] = LIGHTGRAY;
    }

    cantidadAnillos = 0;
    cantidadTormentas = 0;
    cantidadVientos = 0;

    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_NUBES_PREPARACION;
    alturaCamara = ALTURA_INICIAL_NUBES;
    tiempoPreparacion = DURACION_PREPARACION_NUBES;
    tiempoJuego = 0.0f;
    tiempoAnimacion = 0.0f;

    for (int i = 0; i < MAX_PARTICULAS_NUBES; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_AVISOS_NUBES; i++)
    {
        avisos[i] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        destelloJugador[i] = 0.0f;
    }

    semillaVisual = 0x9E3779B9u;
    relampagoTormenta = 0.0f;
    tiempoFin = -1.0f;
    tiempoConfeti = 0.0f;
}


void MinijuegoDescensoNubes::Reiniciar(
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

    AsignarColoresNubes(*this, participantes);
    GenerarNivelNubes(*this);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_NUBES_TERMINADO;
        return;
    }

    CargarPiezasJugablesNubes();
    int limite = LimiteNubes(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        float angulo = (float)k * 2.0f * PI / (float)cantidad + 0.6f;
        Vector3 spawn =
        {
            std::cos(angulo) * 4.2f,
            ALTURA_INICIAL_NUBES + ALTURA_CENTRO_JUGADOR_NUBES,
            std::sin(angulo) * 4.2f
        };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        participa[i] = true;
        estados[i].altura = ALTURA_INICIAL_NUBES;
        estados[i].botObjetivoX = spawn.x;
        estados[i].botObjetivoZ = spawn.z;
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoDescensoNubes::ActualizarPartida(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_NUBES_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_NUBES_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_NUBES_JUGANDO;
        }

        return;
    }

    int limite = LimiteNubes(cantidadMaxima);

    float restanteAntes = TIEMPO_LIMITE_NUBES - tiempoJuego;
    tiempoJuego += deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, TIEMPO_LIMITE_NUBES - tiempoJuego);

    // Altura maxima permitida: nadie puede adelantarse mucho al ritmo base.
    float alturaBase = ALTURA_INICIAL_NUBES - VELOCIDAD_CAIDA_NUBES * tiempoJuego;

    for (int i = 0; i < limite; i++)
    {
        if (!participa[i] || estados[i].aterrizo)
        {
            continue;
        }

        EstadoJugadorNubes& estado = estados[i];
        JugadorPrueba& jugador = jugadores[i];

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotNubes(*this, i, jugador, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        // Temporizadores.
        if (estado.aturdido > 0.0f)
        {
            estado.aturdido -= deltaTime;
            entrada.izquierda = entrada.derecha = entrada.adelante = entrada.atras = false;
            entrada.golpear = entrada.saltar = false;
        }

        if (estado.frenando > 0.0f)
        {
            estado.frenando -= deltaTime;

            if (estado.frenando <= 0.0f)
            {
                estado.frenando = 0.0f;
                estado.recargaFreno = RECARGA_FRENO_NUBES;
            }
        }
        else if (estado.recargaFreno > 0.0f)
        {
            estado.recargaFreno -= deltaTime;

            if (estado.recargaFreno < 0.0f)
            {
                estado.recargaFreno = 0.0f;
            }
        }

        if ((entrada.golpear || entrada.saltar) && estado.frenando <= 0.0f && estado.recargaFreno <= 0.0f)
        {
            estado.frenando = DURACION_FRENO_NUBES;
        }

        // Movimiento horizontal.
        float dirX = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
        float dirZ = (entrada.atras ? 1.0f : 0.0f) - (entrada.adelante ? 1.0f : 0.0f);
        float longitud = std::sqrt(dirX * dirX + dirZ * dirZ);

        if (longitud > 0.0f)
        {
            dirX /= longitud;
            dirZ /= longitud;
            jugador.direccionMirada = { dirX, 0.0f, dirZ };
        }

        estado.velX += dirX * ACELERACION_NUBES * deltaTime;
        estado.velZ += dirZ * ACELERACION_NUBES * deltaTime;

        for (int v = 0; v < cantidadVientos; v++)
        {
            if (std::fabs(estado.altura - vientos[v].altura) < MITAD_BANDA_VIENTO_NUBES)
            {
                estado.velX += vientos[v].dirX * ACELERACION_VIENTO_NUBES * deltaTime;
                estado.velZ += vientos[v].dirZ * ACELERACION_VIENTO_NUBES * deltaTime;
            }
        }

        float factor = LimitarNubes(1.0f - ROZAMIENTO_NUBES * deltaTime, 0.0f, 1.0f);
        estado.velX *= factor;
        estado.velZ *= factor;

        float velocidad = std::sqrt(estado.velX * estado.velX + estado.velZ * estado.velZ);

        if (velocidad > VELOCIDAD_MAXIMA_NUBES)
        {
            estado.velX *= VELOCIDAD_MAXIMA_NUBES / velocidad;
            estado.velZ *= VELOCIDAD_MAXIMA_NUBES / velocidad;
        }

        jugador.posicion.x += estado.velX * deltaTime;
        jugador.posicion.z += estado.velZ * deltaTime;

        float radio = std::sqrt(jugador.posicion.x * jugador.posicion.x + jugador.posicion.z * jugador.posicion.z);

        if (radio > RADIO_CILINDRO_NUBES)
        {
            float escala = RADIO_CILINDRO_NUBES / radio;
            jugador.posicion.x *= escala;
            jugador.posicion.z *= escala;
            estado.velX *= 0.5f;
            estado.velZ *= 0.5f;
        }

        // Caida vertical.
        float alturaAntes = estado.altura;
        float ritmo = estado.frenando > 0.0f ? MULTIPLICADOR_FRENO_NUBES : 1.0f;
        estado.altura -= VELOCIDAD_CAIDA_NUBES * ritmo * deltaTime;

        if (estado.altura > alturaBase + MAXIMO_ADELANTO_NUBES)
        {
            estado.altura = alturaBase + MAXIMO_ADELANTO_NUBES;
        }

        if (estado.altura > alturaAntes)
        {
            estado.altura = alturaAntes;
        }

        // Anillos: se recoge al cruzar su altura estando alineado.
        for (int k = 0; k < cantidadAnillos; k++)
        {
            AnilloNubes& anillo = anillos[k];

            if (
                (anillo.recogidoPor & (1u << i)) != 0u ||
                anillo.posicion.y > alturaAntes + 0.15f ||
                anillo.posicion.y < estado.altura - 0.15f
            )
            {
                continue;
            }

            float dx = anillo.posicion.x - jugador.posicion.x;
            float dz = anillo.posicion.z - jugador.posicion.z;

            if (std::sqrt(dx * dx + dz * dz) <= RADIO_RECOGER_NUBES)
            {
                anillo.recogidoPor |= (1u << i);
                estado.puntos += anillo.valor;
                ReproducirSonidoMinijuego(
                    audio,
                    anillo.valor >= 3 ? SONIDO_RECOGER_NUCLEO_ESPECIAL : SONIDO_RECOGER_OBJETO
                );
            }
        }

        // Tormentas.
        for (int t = 0; t < cantidadTormentas; t++)
        {
            TormentaNubes& tormenta = tormentas[t];

            if (
                (tormenta.golpeadoA & (1u << i)) != 0u ||
                tormenta.posicion.y - MITAD_ALTO_TORMENTA_NUBES > alturaAntes ||
                tormenta.posicion.y + MITAD_ALTO_TORMENTA_NUBES < estado.altura
            )
            {
                continue;
            }

            float dx = tormenta.posicion.x - jugador.posicion.x;
            float dz = tormenta.posicion.z - jugador.posicion.z;

            if (std::sqrt(dx * dx + dz * dz) <= tormenta.radio + 0.35f)
            {
                tormenta.golpeadoA |= (1u << i);
                estado.aturdido = DURACION_ATURDIMIENTO_NUBES;
                estado.golpes++;
                estado.puntos -= PENALIZACION_TORMENTA_NUBES;

                if (estado.puntos < 0)
                {
                    estado.puntos = 0;
                }

                ReproducirSonidoMinijuego(audio, SONIDO_IMPACTO);
            }
        }

        jugador.posicion.y = estado.altura + ALTURA_CENTRO_JUGADOR_NUBES;

        if (estado.altura <= 0.0f)
        {
            AterrizarNubes(*this, i, jugador);
        }
    }

    // Tiempo limite duro: quien siga en el aire aterriza donde esta.
    if (tiempoJuego >= TIEMPO_LIMITE_NUBES)
    {
        for (int i = 0; i < limite; i++)
        {
            if (participa[i] && !estados[i].aterrizo)
            {
                AterrizarNubes(*this, i, jugadores[i]);
            }
        }
    }

    bool todosAterrizaron = true;

    for (int i = 0; i < limite; i++)
    {
        if (participa[i] && !estados[i].aterrizo)
        {
            todosAterrizaron = false;
        }
    }

    // Camara compartida: sigue suavemente la altura media de quienes caen.
    float objetivoCamara = todosAterrizaron ? 0.0f : AlturaMediaNubes(*this, limite);
    alturaCamara += (objetivoCamara - alturaCamara) * LimitarNubes(deltaTime * 5.0f, 0.0f, 1.0f);

    if (todosAterrizaron)
    {
        FinalizarNubes(*this, limite);
    }
}



//==================================================
// VISUAL: EFECTOS (particulas, avisos y destellos)
//==================================================
//
// Solo decoracion: se calcula comparando el estado antes y despues de
// cada paso de la partida. El azar sale de semillaVisual, nunca del
// generador de la logica.

static const Color COLOR_ACENTO_NUBES = { 255, 204, 64, 255 };


struct InstantaneaNubes
{
    FaseNubes fase = FASE_NUBES_PREPARACION;
    float alturaCamara = 0.0f;
    bool aterrizo[MAX_PARTICIPANTES]{};
    bool frenando[MAX_PARTICIPANTES]{};
    unsigned int recogido[MAX_ANILLOS_NUBES]{};
    unsigned int golpeado[MAX_TORMENTAS_NUBES]{};
};


static void TomarInstantaneaNubes(const MinijuegoDescensoNubes& m, InstantaneaNubes& s)
{
    s.fase = m.fase;
    s.alturaCamara = m.alturaCamara;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        s.aterrizo[i] = m.estados[i].aterrizo;
        s.frenando[i] = m.estados[i].frenando > 0.0f;
    }

    for (int k = 0; k < MAX_ANILLOS_NUBES; k++)
    {
        s.recogido[k] = m.anillos[k].recogidoPor;
    }

    for (int k = 0; k < MAX_TORMENTAS_NUBES; k++)
    {
        s.golpeado[k] = m.tormentas[k].golpeadoA;
    }
}


static float Limitar01Nubes(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static float AzarVisualNubes(MinijuegoDescensoNubes& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasNubes(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 2 ? 2 : n;
}


static void EmitirParticulaNubes(
    MinijuegoDescensoNubes& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool brilla
)
{
    for (int i = 0; i < MAX_PARTICULAS_NUBES; i++)
    {
        ParticulaNubes& p = m.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.color = color;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.brilla = brilla;
        return;
    }
}


static void AgregarAvisoNubes(
    MinijuegoDescensoNubes& m,
    Vector3 posicion,
    const char* texto,
    Color color,
    bool grande
)
{
    int elegido = 0;

    for (int i = 0; i < MAX_AVISOS_NUBES; i++)
    {
        if (!m.avisos[i].activo)
        {
            elegido = i;
            break;
        }

        if (m.avisos[i].vida < m.avisos[elegido].vida)
        {
            elegido = i;
        }
    }

    AvisoNubes& a = m.avisos[elegido];
    a.activo = true;
    a.posicion = posicion;
    a.vidaMaxima = grande ? 1.9f : 1.1f;
    a.vida = a.vidaMaxima;
    a.color = color;
    a.grande = grande;
    std::snprintf(a.texto, sizeof(a.texto), "%s", texto);
}


// Estallido esferico de chispas en dos colores.
static void EstallidoNubes(
    MinijuegoDescensoNubes& m,
    Vector3 centro,
    Color a,
    Color b,
    int base,
    float fuerza,
    float gravedad,
    float tamano,
    bool brilla
)
{
    int n = CantidadParticulasNubes(base);

    for (int k = 0; k < n; k++)
    {
        float angulo = AzarVisualNubes(m, 0.0f, 2.0f * PI);
        float alto = AzarVisualNubes(m, -0.6f, 1.0f);
        float plano = std::sqrt(1.0f - alto * alto * 0.7f);
        float v = fuerza * AzarVisualNubes(m, 0.45f, 1.0f);
        Vector3 velocidad = { std::cos(angulo) * plano * v, alto * v, std::sin(angulo) * plano * v };

        EmitirParticulaNubes(
            m,
            centro,
            velocidad,
            (k % 2 == 0) ? a : b,
            AzarVisualNubes(m, 0.45f, 0.95f),
            tamano * AzarVisualNubes(m, 0.7f, 1.3f),
            gravedad,
            brilla
        );
    }
}


static void ActualizarVisualNubes(
    MinijuegoDescensoNubes& m,
    const InstantaneaNubes& antes,
    float deltaTime,
    int limite,
    const JugadorPrueba jugadores[]
)
{
    float dt = deltaTime > 0.05f ? 0.05f : deltaTime;

    // Anillos recogidos: chispas del color del jugador y «+1» / «+3».
    for (int k = 0; k < m.cantidadAnillos; k++)
    {
        unsigned int nuevos = m.anillos[k].recogidoPor & ~antes.recogido[k];

        if (nuevos == 0u)
        {
            continue;
        }

        bool dorado = m.anillos[k].valor >= 3;

        for (int i = 0; i < limite; i++)
        {
            if ((nuevos & (1u << i)) == 0u)
            {
                continue;
            }

            Color color = m.coloresJugadores[i];
            EstallidoNubes(
                m,
                m.anillos[k].posicion,
                dorado ? COLOR_ACENTO_NUBES : WHITE,
                color,
                dorado ? 24 : 12,
                dorado ? 4.4f : 3.0f,
                1.4f,
                dorado ? 0.13f : 0.10f,
                true
            );

            Vector3 arriba = jugadores[i].posicion;
            arriba.y += 2.2f;
            AgregarAvisoNubes(
                m,
                arriba,
                dorado ? "+3" : "+1",
                dorado ? COLOR_ACENTO_NUBES : Color{ 255, 255, 255, 255 },
                dorado
            );
            m.destelloJugador[i] = 1.0f;
        }
    }

    // Tormentas: humo gris, chispas del rayo y destello.
    for (int t = 0; t < m.cantidadTormentas; t++)
    {
        unsigned int nuevos = m.tormentas[t].golpeadoA & ~antes.golpeado[t];

        if (nuevos == 0u)
        {
            continue;
        }

        for (int i = 0; i < limite; i++)
        {
            if ((nuevos & (1u << i)) == 0u)
            {
                continue;
            }

            Vector3 p = jugadores[i].posicion;
            p.y += 0.6f;
            EstallidoNubes(m, p, Color{ 104, 108, 132, 255 }, Color{ 150, 154, 178, 255 }, 14, 2.0f, -0.8f, 0.22f, false);
            EstallidoNubes(m, p, Color{ 255, 240, 140, 255 }, WHITE, 12, 5.0f, 2.0f, 0.08f, true);

            Vector3 arriba = jugadores[i].posicion;
            arriba.y += 2.2f;
            AgregarAvisoNubes(m, arriba, "¡RAYO! -2", Color{ 255, 150, 120, 255 }, false);
            m.relampagoTormenta = 1.0f;
            ActivarTemblorCamaraGeneral(0.12f, 0.22f);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        const EstadoJugadorNubes& e = m.estados[i];

        // Aterrizaje: polvo y pasto; en la diana, estallido dorado.
        if (!antes.aterrizo[i] && e.aterrizo)
        {
            Vector3 pies = { jugadores[i].posicion.x, 0.1f, jugadores[i].posicion.z };
            int n = CantidadParticulasNubes(16);

            for (int k = 0; k < n; k++)
            {
                float angulo = 2.0f * PI * (float)k / (float)n + AzarVisualNubes(m, -0.2f, 0.2f);
                float v = AzarVisualNubes(m, 1.6f, 3.0f);
                EmitirParticulaNubes(
                    m,
                    pies,
                    { std::cos(angulo) * v, AzarVisualNubes(m, 0.6f, 1.6f), std::sin(angulo) * v },
                    (k % 2 == 0) ? Color{ 196, 168, 120, 255 } : Color{ 150, 200, 96, 255 },
                    AzarVisualNubes(m, 0.5f, 0.8f),
                    0.16f,
                    3.0f,
                    false
                );
            }

            if (e.bonusCentro)
            {
                Vector3 p = pies;
                p.y += 0.8f;
                EstallidoNubes(m, p, COLOR_ACENTO_NUBES, m.coloresJugadores[i], 28, 5.2f, 3.0f, 0.12f, true);

                Vector3 arriba = jugadores[i].posicion;
                arriba.y += 2.2f;
                AgregarAvisoNubes(m, arriba, "¡CENTRO! +5", COLOR_ACENTO_NUBES, true);
            }

            m.destelloJugador[i] = 1.0f;
        }

        // Freno: bocanada de aire bajo el ala.
        if (!antes.frenando[i] && e.frenando > 0.0f && !e.aterrizo)
        {
            Vector3 ala = jugadores[i].posicion;
            ala.y += 1.1f;
            int n = CantidadParticulasNubes(8);

            for (int k = 0; k < n; k++)
            {
                EmitirParticulaNubes(
                    m,
                    ala,
                    { AzarVisualNubes(m, -1.8f, 1.8f), AzarVisualNubes(m, 0.6f, 1.6f), AzarVisualNubes(m, -1.0f, 1.0f) },
                    Color{ 248, 250, 255, 255 },
                    AzarVisualNubes(m, 0.35f, 0.6f),
                    0.2f,
                    0.0f,
                    false
                );
            }
        }
    }

    // Fin: confeti sobre quien gano.
    if (
        antes.fase != FASE_NUBES_TERMINADO &&
        m.fase == FASE_NUBES_TERMINADO &&
        m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        m.tiempoFin = 0.0f;
        m.tiempoConfeti = 0.0f;
    }

    if (m.tiempoFin >= 0.0f)
    {
        m.tiempoFin += dt;
        m.tiempoConfeti -= dt;

        if (m.tiempoFin < 3.2f && m.tiempoConfeti <= 0.0f)
        {
            m.tiempoConfeti = 0.12f;
            const Color confeti[4] =
            {
                COLOR_ACENTO_NUBES,
                Color{ 255, 255, 255, 255 },
                Color{ 120, 206, 246, 255 },
                Color{ 246, 150, 196, 255 }
            };

            for (int i = 0; i < limite; i++)
            {
                if (!m.participa[i] || m.resultado.participantes[i].posicionFinal != 1)
                {
                    continue;
                }

                int n = CantidadParticulasNubes(6);

                for (int k = 0; k < n; k++)
                {
                    Vector3 p = jugadores[i].posicion;
                    p.x += AzarVisualNubes(m, -1.0f, 1.0f);
                    p.y += 2.8f;
                    p.z += AzarVisualNubes(m, -1.0f, 1.0f);
                    EmitirParticulaNubes(
                        m,
                        p,
                        { AzarVisualNubes(m, -1.6f, 1.6f), AzarVisualNubes(m, 1.8f, 3.6f), AzarVisualNubes(m, -1.6f, 1.6f) },
                        k % 5 == 4 ? m.coloresJugadores[i] : confeti[k % 4],
                        AzarVisualNubes(m, 1.2f, 1.8f),
                        0.11f,
                        3.4f,
                        false
                    );
                }
            }
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_NUBES; i++)
    {
        ParticulaNubes& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= dt;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float freno = 1.0f - 1.4f * dt;
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.velocidad.y -= p.gravedad * dt;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;
    }

    // Los rotulos acompanan a la camara (los jugadores caen con ella).
    float bajada = m.alturaCamara - antes.alturaCamara;

    for (int i = 0; i < MAX_AVISOS_NUBES; i++)
    {
        AvisoNubes& a = m.avisos[i];

        if (!a.activo)
        {
            continue;
        }

        a.vida -= dt;
        a.posicion.y += bajada + 0.9f * dt;

        if (a.vida <= 0.0f)
        {
            a.activo = false;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.destelloJugador[i] -= dt * 2.4f;

        if (m.destelloJugador[i] < 0.0f)
        {
            m.destelloJugador[i] = 0.0f;
        }
    }

    m.relampagoTormenta -= dt * 3.0f;

    if (m.relampagoTormenta < 0.0f)
    {
        m.relampagoTormenta = 0.0f;
    }
}


void MinijuegoDescensoNubes::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    InstantaneaNubes antes;
    TomarInstantaneaNubes(*this, antes);

    ActualizarPartida(deltaTime, jugadores, cantidadMaxima, participantes);

    ActualizarVisualNubes(*this, antes, deltaTime, LimiteNubes(cantidadMaxima), jugadores);
}


//==================================================
// VISUAL: PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Tarde dorada: cielo celeste con bruma calida hacia el sol (al fondo a
// la izquierda), nubes con copete encendido y sombra lavanda, islas de
// campo con pasto, trigo, cal y tejas. El acento (oro) queda para los
// anillos dorados y el borde de la diana.

static const Color COLOR_NIEBLA_NUBES = { 164, 202, 238, 255 };
static const Color COLOR_BRUMA_DORADA_NUBES = { 240, 206, 168, 255 };
static const Color COLOR_NUBE_LUZ_NUBES = { 255, 240, 212, 255 };
static const Color COLOR_NUBE_SOMBRA_NUBES = { 150, 168, 216, 255 };
static const Color COLOR_PASTO_NUBES = { 116, 174, 80, 255 };
static const Color COLOR_TIERRA_NUBES = { 150, 104, 70, 255 };
static const Color COLOR_ROCA_NUBES = { 142, 118, 108, 255 };
static const Color COLOR_ROCA_CALIDA_NUBES = { 182, 134, 96, 255 };
static const Color COLOR_MADERA_NUBES = { 140, 92, 58, 255 };
static const Color COLOR_MADERA_CLARA_NUBES = { 196, 152, 100, 255 };
static const Color COLOR_TECHO_NUBES = { 176, 76, 58, 255 };
static const Color COLOR_CAL_NUBES = { 244, 234, 214, 255 };
static const Color COLOR_TRIGO_NUBES = { 232, 196, 104, 255 };
static const Color COLOR_AGUA_NUBES = { 104, 176, 216, 255 };

static const float NIVEL_MAR_NUBES = -5.0f;

// Camara del frame (la fija Dibujar) para la niebla y los halos.
static Vector3 ojoNubes = { 0.0f, 112.0f, 9.5f };
static Vector3 derechaVistaNubes = { 1.0f, 0.0f, 0.0f };
static Vector3 arribaVistaNubes = { 0.0f, 0.515f, -0.857f };

// Niebla del objeto que se esta dibujando (FijarNieblaNubes).
static float nieblaActualNubes = 0.0f;
static Color colorNieblaActualNubes = COLOR_NIEBLA_NUBES;


static unsigned char CanalNubes(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoNubes(Color c, float f)
{
    return Color{ CanalNubes((float)c.r * f), CanalNubes((float)c.g * f), CanalNubes((float)c.b * f), c.a };
}


static Color MezclaNubes(Color a, Color b, float t)
{
    t = Limitar01Nubes(t);

    return Color{
        CanalNubes((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalNubes((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalNubes((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalNubes((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaNubes(Color c, float alfa)
{
    c.a = CanalNubes(255.0f * Limitar01Nubes(alfa));
    return c;
}


// Hacia el sol (al fondo, a la izquierda) la bruma se dora.
static Color ColorNieblaNubes(Vector3 p)
{
    float dx = p.x - ojoNubes.x;
    float dz = p.z - ojoNubes.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.01f)
    {
        return COLOR_NIEBLA_NUBES;
    }

    float haciaSol = (-dx * 0.7f - dz * 0.7f) / largo;
    float dorado = Limitar01Nubes((haciaSol + 0.1f) * 0.6f) * Limitar01Nubes(largo / 70.0f);
    return MezclaNubes(COLOR_NIEBLA_NUBES, COLOR_BRUMA_DORADA_NUBES, dorado);
}


static float CantidadNieblaNubes(Vector3 p)
{
    float dx = p.x - ojoNubes.x;
    float dy = p.y - ojoNubes.y;
    float dz = p.z - ojoNubes.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 20.0f) / 105.0f;
    return n < 0.0f ? 0.0f : (n > 0.8f ? 0.8f : n);
}


static void FijarNieblaNubes(Vector3 p)
{
    nieblaActualNubes = CantidadNieblaNubes(p);
    colorNieblaActualNubes = ColorNieblaNubes(p);
}


static void SinNieblaNubes()
{
    nieblaActualNubes = 0.0f;
}


static Color ConNieblaNubes(Color c)
{
    Color niebla = colorNieblaActualNubes;
    niebla.a = c.a;
    return MezclaNubes(c, niebla, nieblaActualNubes);
}


// Caras con luz de tarde: arriba calidas, los costados algo frios.
static Color CaraNubes(Color c, float f, bool arriba)
{
    Color r = arriba
        ? Color{ CanalNubes((float)c.r * f * 1.06f), CanalNubes((float)c.g * f * 1.0f), CanalNubes((float)c.b * f * 0.9f), c.a }
        : Color{ CanalNubes((float)c.r * f * 0.95f), CanalNubes((float)c.g * f * 0.97f), CanalNubes((float)c.b * f * 1.06f), c.a };
    return ConNieblaNubes(r);
}


static void VerticeNubes(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadNubes(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeNubes(a, color);
    VerticeNubes(b, color);
    VerticeNubes(c, color);
    VerticeNubes(d, color);
}


// Caja con caras sombreadas (sol desde la izquierda); respeta la matriz
// actual para piezas que giran.
static void CajaNubes(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlCheckRenderBatchLimit(24);
    rlBegin(RL_QUADS);
    QuadNubes({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, CaraNubes(color, 1.0f, true));
    QuadNubes({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, CaraNubes(color, 0.76f, false));
    QuadNubes({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, CaraNubes(color, 0.62f, false));
    QuadNubes({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, CaraNubes(color, 0.6f, false));
    QuadNubes({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, CaraNubes(color, 0.86f, false));
    QuadNubes({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, CaraNubes(color, 0.45f, false));
    rlEnd();
}


// Luz de un costado segun su orientacion: la izquierda (-X) al sol.
static float LuzLadoNubes(float angulo)
{
    return 0.7f - 0.17f * std::cos(angulo) + 0.06f * std::sin(angulo);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (radio r1).
static void PrismaNubes(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    float giro = 0.0f
)
{
    float yArriba = base.y + alto;
    float paso = 2.0f * PI / (float)lados;
    Color tapaArriba = CaraNubes(color, 1.0f, true);
    Color tapaAbajo = CaraNubes(color, 0.45f, false);

    rlCheckRenderBatchLimit(lados * 12);
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadNubes(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            CaraNubes(color, LuzLadoNubes(a0 + paso * 0.5f), false)
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadNubes(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }

        if (r0 > 0.001f)
        {
            Vector3 centro = { base.x, base.y, base.z };
            QuadNubes(centro, { base.x + c0 * r0, base.y, base.z + s0 * r0 }, { base.x + c1 * r0, base.y, base.z + s1 * r0 }, centro, tapaAbajo);
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y; el color va tal cual (sin luz).
static void CoronaNubes(Vector3 c, float r0, float r1, Color color, int lados)
{
    rlCheckRenderBatchLimit(lados * 4);
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;

        QuadNubes(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Tramo de corona entre los angulos a0 y a1 (para trazos discontinuos).
static void ArcoPlanoNubes(Vector3 c, float r0, float r1, float a0, float a1, Color color)
{
    const int PASOS = 3;
    rlCheckRenderBatchLimit(PASOS * 4);
    rlBegin(RL_QUADS);

    for (int k = 0; k < PASOS; k++)
    {
        float b0 = a0 + (a1 - a0) * (float)k / (float)PASOS;
        float b1 = a0 + (a1 - a0) * (float)(k + 1) / (float)PASOS;

        QuadNubes(
            { c.x + std::cos(b0) * r1, c.y, c.z + std::sin(b0) * r1 },
            { c.x + std::cos(b0) * r0, c.y, c.z + std::sin(b0) * r0 },
            { c.x + std::cos(b1) * r0, c.y, c.z + std::sin(b1) * r0 },
            { c.x + std::cos(b1) * r1, c.y, c.z + std::sin(b1) * r1 },
            color
        );
    }

    rlEnd();
}


// Disco de luz con borde que se apaga (u y v definen el plano).
static void ResplandorNubes(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 18;
    rlCheckRenderBatchLimit(LADOS * 3);
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


// Halo que mira a la camara.
static void ResplandorVistaNubes(Vector3 c, float radio, Color color)
{
    ResplandorNubes(c, derechaVistaNubes, arribaVistaNubes, radio, color);
}


// Charco de luz en el suelo.
static void ResplandorSueloNubes(Vector3 c, float radio, Color color)
{
    ResplandorNubes(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Bocanada redonda con luz por vertice: copete encendido arriba a la
// izquierda, sombra fria abajo. Los colores llegan ya con niebla.
static void BocanadaNubes(Vector3 c, float radio, float aplastado, Color luz, Color sombra)
{
    const int ANILLOS = 5;
    const int SECTORES = 10;
    const float lx = -0.5f, ly = 0.78f, lz = 0.36f;

    rlCheckRenderBatchLimit(ANILLOS * SECTORES * 4);
    rlBegin(RL_QUADS);

    for (int i = 0; i < ANILLOS; i++)
    {
        float t0 = -PI * 0.5f + PI * (float)i / (float)ANILLOS;
        float t1 = -PI * 0.5f + PI * (float)(i + 1) / (float)ANILLOS;

        for (int j = 0; j < SECTORES; j++)
        {
            float f0 = 2.0f * PI * (float)j / (float)SECTORES;
            float f1 = 2.0f * PI * (float)(j + 1) / (float)SECTORES;
            const float th[4] = { t0, t0, t1, t1 };
            const float fi[4] = { f0, f1, f1, f0 };

            for (int v = 0; v < 4; v++)
            {
                float nx = std::cos(th[v]) * std::cos(fi[v]);
                float ny = std::sin(th[v]);
                float nz = std::cos(th[v]) * std::sin(fi[v]);
                float l = Limitar01Nubes((nx * lx + ny * ly + nz * lz) * 0.62f + 0.48f);
                VerticeNubes({ c.x + nx * radio, c.y + ny * radio * aplastado, c.z + nz * radio }, MezclaNubes(sombra, luz, l));
            }
        }
    }

    rlEnd();
}


// Nube de varias bocanadas (cumulo chico).
static void NubeNubes(Vector3 c, float escala, int semilla, float aplastado)
{
    FijarNieblaNubes(c);
    Color luz = ConNieblaNubes(COLOR_NUBE_LUZ_NUBES);
    Color sombra = ConNieblaNubes(COLOR_NUBE_SOMBRA_NUBES);

    BocanadaNubes(c, 1.25f * escala, aplastado, luz, sombra);

    int partes = 2 + semilla % 3;

    for (int k = 0; k < partes; k++)
    {
        float a = (float)k * 2.4f + HashNubes(semilla, 500 + k) * 1.2f;
        float d = (0.9f + 0.4f * HashNubes(semilla, 520 + k)) * escala;
        float r = (0.65f + 0.35f * HashNubes(semilla, 540 + k)) * escala;
        Vector3 p = { c.x + std::cos(a) * d, c.y - 0.15f * escala + 0.3f * escala * HashNubes(semilla, 560 + k), c.z + std::sin(a) * d * 0.7f };
        BocanadaNubes(p, r, aplastado, luz, sombra);
    }
}


//==================================================
// VISUAL: CAMARA Y CIELO
//==================================================

static Camera3D VistaNubes(const MinijuegoDescensoNubes& m)
{
    Camera3D v = m.camara;
    float ca = m.alturaCamara;
    float movimiento = ObtenerFactorMovimientoCamaraMinijuegos();

    v.position = { 0.0f, ca + 12.5f, 9.5f };
    v.target = { 0.0f, ca - 3.2f, 0.2f };

    // Entrada: durante la cuenta la camara baja en picada desde mas alto.
    if (m.fase == FASE_NUBES_PREPARACION)
    {
        float u = Limitar01Nubes(m.tiempoPreparacion / DURACION_PREPARACION_NUBES);
        float s = u * u * (3.0f - 2.0f * u) * movimiento;
        v.position.x -= 4.0f * s;
        v.position.y += 10.0f * s;
        v.position.z += 8.0f * s;
        v.target.y -= 2.0f * s;
    }

    // Al terminar el plano se abre para mostrar la isla entera.
    if (m.tiempoFin >= 0.0f)
    {
        float u = movimiento > 0.0f ? Limitar01Nubes(m.tiempoFin / 1.4f) : 1.0f;
        float s = u * u * (3.0f - 2.0f * u);
        v.position.y += 6.0f * s;
        v.position.z += 6.5f * s;
        v.target.z -= 1.2f * s;
    }

    return AplicarTemblorGeneralACamara(v);
}


static void FijarVistaNubes(const Camera3D& vista)
{
    ojoNubes = vista.position;

    Vector3 f = { vista.target.x - vista.position.x, vista.target.y - vista.position.y, vista.target.z - vista.position.z };
    float lf = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
    f = { f.x / lf, f.y / lf, f.z / lf };

    // derecha = f x up(0,1,0); arriba = derecha x f.
    Vector3 d = { -f.z, 0.0f, f.x };
    float ld = std::sqrt(d.x * d.x + d.z * d.z);
    d = ld > 0.001f ? Vector3{ d.x / ld, 0.0f, d.z / ld } : Vector3{ 1.0f, 0.0f, 0.0f };

    derechaVistaNubes = d;
    arribaVistaNubes = { d.y * f.z - d.z * f.y, d.z * f.x - d.x * f.z, d.x * f.y - d.y * f.x };
}


// Fondo 2D (antes del 3D): la mirada cae hacia el vacio; arriba la bruma
// se dora porque mira hacia el sol de la tarde.
static void DibujarCieloNubes()
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int medio = (int)((float)alto * 0.42f);

    DrawRectangleGradientV(0, 0, ancho, medio, Color{ 246, 206, 160, 255 }, Color{ 170, 206, 240, 255 });
    DrawRectangleGradientV(0, medio, ancho, alto - medio, Color{ 170, 206, 240, 255 }, Color{ 116, 168, 228, 255 });

    Vector2 sol = { (float)ancho * 0.05f, -(float)alto * 0.1f };
    DrawCircleGradient(sol, (float)alto * 1.0f, Fade(Color{ 255, 224, 170, 255 }, 0.5f), Fade(Color{ 255, 224, 170, 255 }, 0.0f));
    DrawCircleGradient(sol, (float)alto * 0.4f, Fade(Color{ 255, 244, 214, 255 }, 0.7f), Fade(Color{ 255, 236, 200, 255 }, 0.0f));
}


// Haces del sol sobre toda la escena (2D aditivo, al final del mundo).
static void DibujarRayosSolNubes(float t)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    Vector2 sol = { (float)ancho * 0.05f, -(float)alto * 0.1f };

    BeginBlendMode(BLEND_ADDITIVE);

    for (int k = 0; k < 6; k++)
    {
        float a = 0.3f + (float)k * 0.19f + 0.03f * std::sin(t * 0.15f + (float)k * 1.7f);
        float abre = 0.02f + 0.016f * HashNubes(k, 900);
        float largo = (float)ancho * (0.55f + 0.3f * HashNubes(k, 901));
        Vector2 p1 = { sol.x + std::cos(a - abre) * largo, sol.y + std::sin(a - abre) * largo };
        Vector2 p2 = { sol.x + std::cos(a + abre) * largo, sol.y + std::sin(a + abre) * largo };
        float alfa = 0.022f + 0.016f * (0.5f + 0.5f * std::sin(t * 0.4f + (float)k * 2.1f));
        DrawTriangle(sol, p2, p1, Fade(Color{ 255, 220, 160, 255 }, alfa));
    }

    EndBlendMode();
}


//==================================================
// VISUAL: MAR DE NUBES E ISLAS LEJANAS (horizonte)
//==================================================

// Mar de nubes al fondo del vacio: base ondulada con niebla por distancia
// y bocanadas que derivan.
static void DibujarMarNubes(float t)
{
    const int ANILLOS = 7;
    const int LADOS = 32;
    const float radios[ANILLOS + 1] = { 0.0f, 14.0f, 26.0f, 42.0f, 64.0f, 94.0f, 136.0f, 210.0f };
    float y = NIVEL_MAR_NUBES;

    rlCheckRenderBatchLimit(ANILLOS * LADOS * 4);
    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        for (int k = 0; k < LADOS; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)LADOS;
            float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
            Vector3 p[4] =
            {
                { std::cos(a0) * radios[a + 1], y, std::sin(a0) * radios[a + 1] },
                { std::cos(a0) * radios[a], y, std::sin(a0) * radios[a] },
                { std::cos(a1) * radios[a], y, std::sin(a1) * radios[a] },
                { std::cos(a1) * radios[a + 1], y, std::sin(a1) * radios[a + 1] }
            };

            for (int v = 0; v < 4; v++)
            {
                float ondulacion = std::sin(p[v].x * 0.07f + t * 0.08f) * std::cos(p[v].z * 0.06f - t * 0.05f);
                Color base = MezclaNubes(COLOR_NUBE_SOMBRA_NUBES, COLOR_NUBE_LUZ_NUBES, 0.5f + 0.32f * ondulacion);
                VerticeNubes(p[v], MezclaNubes(base, ColorNieblaNubes(p[v]), CantidadNieblaNubes(p[v])));
            }
        }
    }

    rlEnd();

    NivelCalidadGrafica calidad = CalidadDecoracion();
    int cantidad = calidad == CALIDAD_ALTA ? 64 : (calidad == CALIDAD_MEDIA ? 34 : 14);

    for (int k = 0; k < cantidad; k++)
    {
        float r = 14.0f + 120.0f * std::pow(HashNubes(k, 80), 1.3f);
        float a = HashNubes(k, 81) * 2.0f * PI + t * 0.004f;
        float radio = 3.0f + 5.0f * HashNubes(k, 83) + r * 0.03f;
        Vector3 c = { std::cos(a) * r, y + 0.4f * HashNubes(k, 84), std::sin(a) * r };
        FijarNieblaNubes(c);
        BocanadaNubes(c, radio, 0.4f, ConNieblaNubes(COLOR_NUBE_LUZ_NUBES), ConNieblaNubes(COLOR_NUBE_SOMBRA_NUBES));

        if (calidad == CALIDAD_ALTA && k % 2 == 0)
        {
            Vector3 c2 = { c.x + radio * 0.7f, c.y + radio * 0.12f, c.z - radio * 0.3f };
            BocanadaNubes(c2, radio * 0.7f, 0.5f, ConNieblaNubes(COLOR_NUBE_LUZ_NUBES), ConNieblaNubes(COLOR_NUBE_SOMBRA_NUBES));
        }
    }
}


// Isla flotante de campo: tapa de pasto, reborde, estratos de tierra y
// roca que se afinan hacia abajo y raices colgantes.
static void IslaCampoNubes(Vector3 c, float radio, float profundidad, int semilla, bool detalle)
{
    const int LADOS = 18;
    const int NIVELES = 4;
    Vector3 anillos[NIVELES + 1][LADOS];

    for (int l = 0; l <= NIVELES; l++)
    {
        float f = (float)l / (float)NIVELES;
        float y = c.y - 0.35f - profundidad * f;
        float r = radio * std::pow(1.0f - f * 0.96f, 0.85f);

        for (int k = 0; k < LADOS; k++)
        {
            float a = 2.0f * PI * (float)k / (float)LADOS;
            float irregular = l == 0
                ? 0.93f + 0.1f * HashNubes(semilla, k)
                : 0.7f + 0.42f * HashNubes(semilla + l * 17, k);
            anillos[l][k] = { c.x + std::cos(a) * r * irregular, y, c.z + std::sin(a) * r * irregular };
        }
    }

    FijarNieblaNubes(c);
    Color pasto = COLOR_PASTO_NUBES;
    Color reborde = TonoNubes(COLOR_PASTO_NUBES, 0.78f);

    rlCheckRenderBatchLimit(LADOS * (NIVELES + 2) * 4);
    rlBegin(RL_QUADS);

    for (int k = 0; k < LADOS; k++)
    {
        int k1 = (k + 1) % LADOS;
        float am = 2.0f * PI * ((float)k + 0.5f) / (float)LADOS;
        float luz = LuzLadoNubes(am);
        Vector3 a0 = anillos[0][k];
        Vector3 a1 = anillos[0][k1];

        QuadNubes(c, { a1.x, c.y, a1.z }, { a0.x, c.y, a0.z }, c, CaraNubes(pasto, 0.93f + 0.12f * HashNubes(semilla, k + 40), true));
        QuadNubes({ a0.x, c.y, a0.z }, { a1.x, c.y, a1.z }, a1, a0, CaraNubes(reborde, luz, false));

        for (int l = 0; l < NIVELES; l++)
        {
            Color estrato = l == 0 ? COLOR_TIERRA_NUBES : (((l + semilla) % 2 == 0) ? COLOR_ROCA_NUBES : COLOR_ROCA_CALIDA_NUBES);
            float oscuro = 1.0f - 0.34f * (float)l / (float)NIVELES;
            QuadNubes(anillos[l][k], anillos[l][k1], anillos[l + 1][k1], anillos[l + 1][k], CaraNubes(estrato, luz * oscuro, false));
        }
    }

    rlEnd();

    if (!detalle)
    {
        return;
    }

    // Raices y piedras que cuelgan del primer estrato.
    for (int k = 0; k < LADOS; k += 3)
    {
        Vector3 p = anillos[1][(k + semilla) % LADOS];
        float largo = 0.5f + 0.9f * HashNubes(semilla, 300 + k);
        CajaNubes({ p.x * 0.97f + c.x * 0.03f, p.y - largo * 0.5f, p.z * 0.97f + c.z * 0.03f }, { 0.09f, largo, 0.09f }, Color{ 104, 78, 58, 255 });
    }
}


static void ArbolNubes(Vector3 base, float alto, int semilla)
{
    FijarNieblaNubes(base);
    CajaNubes({ base.x, base.y + alto * 0.3f, base.z }, { 0.16f * alto, alto * 0.6f, 0.16f * alto }, COLOR_MADERA_NUBES);

    Color luz = ConNieblaNubes(Color{ 170, 212, 104, 255 });
    Color sombra = ConNieblaNubes(Color{ 52, 112, 70, 255 });
    float giro = HashNubes(semilla, 610) * 6.0f;

    BocanadaNubes({ base.x, base.y + alto * 0.82f, base.z }, alto * 0.42f, 0.85f, luz, sombra);
    BocanadaNubes({ base.x + std::cos(giro) * alto * 0.22f, base.y + alto * 0.66f, base.z + std::sin(giro) * alto * 0.22f }, alto * 0.3f, 0.85f, luz, sombra);
}


// Pino lejano: cono simple.
static void PinoNubes(Vector3 base, float alto)
{
    PrismaNubes(base, alto * 0.32f, 0.0f, alto, 6, Color{ 70, 128, 78, 255 });
}


static void MolinoNubes(Vector3 base, float escala, float giro)
{
    FijarNieblaNubes(base);
    float e = escala;

    PrismaNubes(base, 1.0f * e, 1.0f * e, 0.35f * e, 8, COLOR_ROCA_NUBES);
    PrismaNubes({ base.x, base.y + 0.35f * e, base.z }, 0.9f * e, 0.6f * e, 2.35f * e, 8, COLOR_CAL_NUBES);
    PrismaNubes({ base.x, base.y + 2.7f * e, base.z }, 0.74f * e, 0.0f, 1.0f * e, 8, COLOR_TECHO_NUBES);
    CajaNubes({ base.x, base.y + 0.55f * e, base.z + 0.86f * e }, { 0.44f * e, 0.78f * e, 0.12f * e }, COLOR_MADERA_NUBES);
    CajaNubes({ base.x, base.y + 1.75f * e, base.z + 0.72f * e }, { 0.3f * e, 0.32f * e, 0.1f * e }, Color{ 88, 118, 148, 255 });

    // Aspas de lona sobre un eje que mira al frente.
    Vector3 eje = { base.x, base.y + 2.8f * e, base.z + 0.78f * e };
    CajaNubes(eje, { 0.22f * e, 0.22f * e, 0.34f * e }, COLOR_MADERA_NUBES);

    rlPushMatrix();
    rlTranslatef(eje.x, eje.y, eje.z + 0.16f * e);
    // El eje se inclina hacia arriba para que las aspas se lean desde
    // la camara alta.
    rlRotatef(-35.0f, 1.0f, 0.0f, 0.0f);
    rlRotatef(giro * RAD2DEG, 0.0f, 0.0f, 1.0f);

    for (int a = 0; a < 4; a++)
    {
        rlPushMatrix();
        rlRotatef(90.0f * (float)a, 0.0f, 0.0f, 1.0f);
        CajaNubes({ 0.0f, 1.25f * e, 0.0f }, { 0.1f * e, 2.5f * e, 0.08f * e }, COLOR_MADERA_NUBES);
        CajaNubes({ 0.26f * e, 1.5f * e, -0.02f * e }, { 0.42f * e, 1.9f * e, 0.03f * e }, Color{ 255, 250, 236, 255 });

        // Enrejado de la lona.
        CajaNubes({ 0.47f * e, 1.5f * e, 0.0f }, { 0.05f * e, 1.9f * e, 0.05f * e }, COLOR_MADERA_NUBES);

        for (int b = 0; b < 4; b++)
        {
            CajaNubes({ 0.26f * e, (0.62f + 0.58f * (float)b) * e, 0.0f }, { 0.44f * e, 0.04f * e, 0.05f * e }, COLOR_MADERA_NUBES);
        }

        rlPopMatrix();
    }

    rlPopMatrix();
}


// Granero rojo con molduras blancas, porton en cruz y techo a dos aguas.
static void GraneroNubes(Vector3 base, float escala, float anguloY)
{
    FijarNieblaNubes(base);
    const Color pared = { 176, 66, 52, 255 };
    const Color techo = { 98, 78, 74, 255 };

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);
    rlScalef(escala, escala, escala);

    CajaNubes({ 0.0f, 0.8f, 0.0f }, { 2.2f, 1.6f, 2.4f }, pared);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            CajaNubes({ (float)sx * 1.1f, 0.8f, (float)sz * 1.2f }, { 0.1f, 1.62f, 0.1f }, COLOR_CAL_NUBES);
        }
    }

    // Porton al frente con su cruz.
    CajaNubes({ 0.0f, 0.62f, 1.22f }, { 1.0f, 1.24f, 0.05f }, TonoNubes(pared, 0.82f));
    CajaNubes({ 0.0f, 1.27f, 1.24f }, { 1.1f, 0.08f, 0.06f }, COLOR_CAL_NUBES);
    CajaNubes({ -0.52f, 0.62f, 1.24f }, { 0.07f, 1.3f, 0.06f }, COLOR_CAL_NUBES);
    CajaNubes({ 0.52f, 0.62f, 1.24f }, { 0.07f, 1.3f, 0.06f }, COLOR_CAL_NUBES);

    for (int s = -1; s <= 1; s += 2)
    {
        rlPushMatrix();
        rlTranslatef(0.0f, 0.62f, 1.25f);
        rlRotatef(42.0f * (float)s, 0.0f, 0.0f, 1.0f);
        CajaNubes({ 0.0f, 0.0f, 0.0f }, { 0.07f, 1.5f, 0.05f }, COLOR_CAL_NUBES);
        rlPopMatrix();
    }

    // Techo a dos aguas con la cumbrera de adelante hacia atras.
    rlCheckRenderBatchLimit(16);
    rlBegin(RL_QUADS);
    QuadNubes({ -1.32f, 1.55f, 1.35f }, { 0.0f, 2.5f, 1.35f }, { 0.0f, 2.5f, -1.35f }, { -1.32f, 1.55f, -1.35f }, CaraNubes(techo, 1.0f, true));
    QuadNubes({ 1.32f, 1.55f, 1.35f }, { 0.0f, 2.5f, 1.35f }, { 0.0f, 2.5f, -1.35f }, { 1.32f, 1.55f, -1.35f }, CaraNubes(techo, 0.72f, true));
    rlEnd();

    rlBegin(RL_TRIANGLES);
    for (int s = -1; s <= 1; s += 2)
    {
        Color c = CaraNubes(pared, s > 0 ? 0.8f : 0.6f, false);
        VerticeNubes({ -1.1f, 1.6f, 1.2f * (float)s }, c);
        VerticeNubes({ 1.1f, 1.6f, 1.2f * (float)s }, c);
        VerticeNubes({ 0.0f, 2.42f, 1.2f * (float)s }, c);
    }
    rlEnd();

    CajaNubes({ 0.0f, 1.95f, 1.23f }, { 0.42f, 0.4f, 0.05f }, COLOR_CAL_NUBES);
    CajaNubes({ 0.0f, 1.95f, 1.25f }, { 0.3f, 0.28f, 0.05f }, Color{ 70, 50, 44, 255 });

    rlPopMatrix();
}


static void FardoNubes(Vector3 base, float radio, float anguloY)
{
    FijarNieblaNubes(base);
    rlPushMatrix();
    rlTranslatef(base.x, base.y + radio, base.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    PrismaNubes({ 0.0f, -radio * 0.7f, 0.0f }, radio, radio, radio * 1.4f, 10, COLOR_TRIGO_NUBES);
    rlPopMatrix();
}


// Casita encalada con tejas y humo que sube de la chimenea.
static void CasitaNubes(Vector3 base, float anguloY, float t)
{
    FijarNieblaNubes(base);

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);

    CajaNubes({ 0.0f, 0.55f, 0.0f }, { 1.5f, 1.1f, 1.1f }, COLOR_CAL_NUBES);
    CajaNubes({ 0.0f, 0.4f, 0.56f }, { 0.34f, 0.8f, 0.05f }, COLOR_MADERA_NUBES);
    CajaNubes({ 0.48f, 0.66f, 0.56f }, { 0.3f, 0.28f, 0.05f }, Color{ 255, 214, 140, 255 });
    CajaNubes({ -0.48f, 0.66f, 0.56f }, { 0.3f, 0.28f, 0.05f }, Color{ 255, 214, 140, 255 });

    rlCheckRenderBatchLimit(16);
    rlBegin(RL_QUADS);
    QuadNubes({ -0.85f, 1.1f, 0.68f }, { 0.85f, 1.1f, 0.68f }, { 0.85f, 1.75f, 0.0f }, { -0.85f, 1.75f, 0.0f }, CaraNubes(COLOR_TECHO_NUBES, 1.0f, true));
    QuadNubes({ -0.85f, 1.1f, -0.68f }, { 0.85f, 1.1f, -0.68f }, { 0.85f, 1.75f, 0.0f }, { -0.85f, 1.75f, 0.0f }, CaraNubes(COLOR_TECHO_NUBES, 0.7f, false));
    rlEnd();

    rlBegin(RL_TRIANGLES);
    for (int s = -1; s <= 1; s += 2)
    {
        Color c = CaraNubes(COLOR_CAL_NUBES, s < 0 ? 0.86f : 0.62f, false);
        VerticeNubes({ 0.75f * (float)s, 1.1f, 0.55f }, c);
        VerticeNubes({ 0.75f * (float)s, 1.1f, -0.55f }, c);
        VerticeNubes({ 0.75f * (float)s, 1.7f, 0.0f }, c);
    }
    rlEnd();

    CajaNubes({ 0.45f, 1.7f, -0.2f }, { 0.22f, 0.6f, 0.22f }, COLOR_ROCA_NUBES);

    // Humo: bocanadas que suben, crecen y se aclaran.
    for (int k = 0; k < 4; k++)
    {
        float u = std::fmod(t * 0.35f + (float)k * 0.25f, 1.0f);
        Vector3 p = { 0.45f + u * 0.5f, 2.05f + u * 1.6f, -0.2f - u * 0.3f };
        Color luz = ConNieblaNubes(MezclaNubes(Color{ 214, 214, 222, 255 }, COLOR_NUBE_LUZ_NUBES, u));
        Color sombra = ConNieblaNubes(MezclaNubes(Color{ 150, 150, 166, 255 }, COLOR_NUBE_SOMBRA_NUBES, u));
        BocanadaNubes(p, 0.12f + 0.22f * u, 0.9f, luz, sombra);
    }

    rlPopMatrix();
}


static void CercaNubes(Vector3 a, Vector3 b, int postes)
{
    FijarNieblaNubes(a);
    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    for (int p = 0; p <= postes; p++)
    {
        float f = (float)p / (float)postes;
        CajaNubes({ a.x + dx * f, a.y + 0.3f, a.z + dz * f }, { 0.1f, 0.6f, 0.1f }, COLOR_MADERA_NUBES);
    }

    rlPushMatrix();
    rlTranslatef((a.x + b.x) * 0.5f, a.y, (a.z + b.z) * 0.5f);
    rlRotatef(std::atan2(dx, dz) * RAD2DEG, 0.0f, 1.0f, 0.0f);
    CajaNubes({ 0.0f, 0.44f, 0.0f }, { 0.05f, 0.07f, largo }, COLOR_MADERA_CLARA_NUBES);
    CajaNubes({ 0.0f, 0.22f, 0.0f }, { 0.05f, 0.07f, largo }, COLOR_MADERA_CLARA_NUBES);
    rlPopMatrix();
}


// Surcos de trigo maduro alternados con tierra.
static void CampoTrigoNubes(Vector3 c, float ancho, float largo, float anguloY)
{
    FijarNieblaNubes(c);
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);

    int surcos = (int)(ancho / 0.36f);

    rlCheckRenderBatchLimit(4);
    rlBegin(RL_QUADS);
    QuadNubes({ -ancho * 0.5f, 0.012f, largo * 0.5f }, { ancho * 0.5f, 0.012f, largo * 0.5f }, { ancho * 0.5f, 0.012f, -largo * 0.5f }, { -ancho * 0.5f, 0.012f, -largo * 0.5f }, CaraNubes(COLOR_TIERRA_NUBES, 1.0f, true));
    rlEnd();

    for (int s = 0; s < surcos; s++)
    {
        float x = -ancho * 0.5f + 0.18f + (float)s * 0.36f;
        CajaNubes({ x, 0.13f, 0.0f }, { 0.22f, 0.24f, largo * 0.94f }, TonoNubes(COLOR_TRIGO_NUBES, 0.92f + 0.12f * HashNubes(s, 640)));
    }

    rlPopMatrix();
}


static void EstanqueNubes(Vector3 c, float radio, float t)
{
    FijarNieblaNubes(c);
    CoronaNubes({ c.x, c.y + 0.01f, c.z }, radio, radio + 0.25f, CaraNubes(Color{ 170, 150, 120, 255 }, 1.0f, true), 18);
    CoronaNubes({ c.x, c.y + 0.012f, c.z }, 0.0f, radio, CaraNubes(COLOR_AGUA_NUBES, 1.0f, true), 18);

    float brillo = 0.5f + 0.5f * std::sin(t * 1.6f);
    CoronaNubes({ c.x, c.y + 0.016f, c.z }, radio * 0.45f, radio * 0.55f, CaraNubes(MezclaNubes(COLOR_AGUA_NUBES, WHITE, 0.3f + 0.2f * brillo), 1.0f, true), 18);

    for (int k = 0; k < 5; k++)
    {
        float a = 2.4f + (float)k * 0.32f;
        CajaNubes({ c.x + std::cos(a) * radio, c.y + 0.3f, c.z + std::sin(a) * radio }, { 0.06f, 0.6f, 0.06f }, Color{ 92, 140, 70, 255 });
    }

    // Pato que se mece.
    float a = t * 0.25f;
    Vector3 p = { c.x + std::cos(a) * radio * 0.5f, c.y + 0.1f + 0.02f * std::sin(t * 3.0f), c.z + std::sin(a) * radio * 0.5f };
    CajaNubes(p, { 0.22f, 0.16f, 0.3f }, Color{ 250, 250, 244, 255 });
    CajaNubes({ p.x, p.y + 0.14f, p.z + 0.12f }, { 0.13f, 0.14f, 0.13f }, Color{ 250, 250, 244, 255 });
    CajaNubes({ p.x, p.y + 0.13f, p.z + 0.22f }, { 0.07f, 0.05f, 0.08f }, Color{ 240, 150, 60, 255 });
}


// Cascada que cae del borde de una isla hacia el vacio.
static void CascadaNubes(Vector3 borde, Vector3 afuera, float ancho, float largo, float t)
{
    FijarNieblaNubes(borde);
    Vector3 lado = { -afuera.z, 0.0f, afuera.x };
    const int TRAMOS = 10;

    rlCheckRenderBatchLimit(TRAMOS * 4);
    rlBegin(RL_QUADS);

    for (int s = 0; s < TRAMOS; s++)
    {
        float f0 = (float)s / (float)TRAMOS;
        float f1 = (float)(s + 1) / (float)TRAMOS;
        float y0 = borde.y - largo * f0;
        float y1 = borde.y - largo * f1;
        float o0 = 0.15f + 0.6f * f0 * f0;
        float o1 = 0.15f + 0.6f * f1 * f1;
        float w0 = ancho * (1.0f + 0.4f * f0);
        float w1 = ancho * (1.0f + 0.4f * f1);
        bool franja = std::fmod((float)s * 0.37f - t * 1.6f + 10.0f, 1.0f) < 0.35f;
        Color agua = franja ? Color{ 214, 238, 250, 255 } : Color{ 140, 200, 236, 255 };
        Color c = CaraNubes(agua, 0.95f, false);

        QuadNubes(
            { borde.x + afuera.x * o0 - lado.x * w0, y0, borde.z + afuera.z * o0 - lado.z * w0 },
            { borde.x + afuera.x * o0 + lado.x * w0, y0, borde.z + afuera.z * o0 + lado.z * w0 },
            { borde.x + afuera.x * o1 + lado.x * w1, y1, borde.z + afuera.z * o1 + lado.z * w1 },
            { borde.x + afuera.x * o1 - lado.x * w1, y1, borde.z + afuera.z * o1 - lado.z * w1 },
            c
        );
    }

    rlEnd();

    Vector3 pie = { borde.x + afuera.x * 0.75f, borde.y - largo, borde.z + afuera.z * 0.75f };
    FijarNieblaNubes(pie);
    BocanadaNubes(pie, ancho * 2.4f, 0.6f, ConNieblaNubes(COLOR_NUBE_LUZ_NUBES), ConNieblaNubes(COLOR_NUBE_SOMBRA_NUBES));
}


// Manga de viento a franjas que flamea.
static void MangaVientoNubes(Vector3 base, float t)
{
    FijarNieblaNubes(base);
    CajaNubes({ base.x, base.y + 1.1f, base.z }, { 0.08f, 2.2f, 0.08f }, Color{ 210, 210, 214, 255 });

    rlPushMatrix();
    rlTranslatef(base.x, base.y + 2.1f, base.z);
    rlRotatef(-20.0f + 14.0f * std::sin(t * 1.3f), 0.0f, 1.0f, 0.0f);
    rlRotatef(-90.0f + 8.0f + 6.0f * std::sin(t * 4.0f), 0.0f, 0.0f, 1.0f);

    for (int s = 0; s < 4; s++)
    {
        float r0 = 0.2f - 0.032f * (float)s;
        PrismaNubes({ 0.0f, 0.3f * (float)s, 0.0f }, r0, r0 - 0.032f, 0.3f, 8, s % 2 == 0 ? Color{ 238, 120, 56, 255 } : Color{ 246, 244, 238, 255 });
    }

    rlPopMatrix();
}


// Baliza de pista: poste con lampara ambar (la luz va en DibujarLuces).
static void BalizaNubes(Vector3 base, bool encendida)
{
    FijarNieblaNubes(base);
    CajaNubes({ base.x, base.y + 0.35f, base.z }, { 0.12f, 0.7f, 0.12f }, Color{ 238, 236, 230, 255 });
    CajaNubes({ base.x, base.y + 0.26f, base.z }, { 0.13f, 0.12f, 0.13f }, Color{ 222, 96, 60, 255 });
    CajaNubes({ base.x, base.y + 0.8f, base.z }, { 0.24f, 0.22f, 0.24f }, encendida ? Color{ 255, 224, 130, 255 } : Color{ 222, 186, 120, 255 });
    PrismaNubes({ base.x, base.y + 0.91f, base.z }, 0.18f, 0.0f, 0.14f, 6, Color{ 214, 84, 60, 255 });
}


// Islas lejanas (dos profundidades) que asoman del vacio con la bruma.
static void DibujarIslasLejanasNubes(float t, float ca)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();

    if (calidad == CALIDAD_BAJA)
    {
        return;
    }

    const int TOTAL = 18;
    int paso = calidad == CALIDAD_ALTA ? 1 : 2;

    for (int k = 0; k < TOTAL; k += paso)
    {
        float y = -2.0f + 104.0f * ((float)k + 0.5f) / (float)TOTAL + (HashNubes(k, 303) - 0.5f) * 5.0f;

        if (y < ca - 85.0f || y > ca + 8.0f)
        {
            continue;
        }

        bool lejos = k % 2 == 1;
        float a = PI * 0.88f + PI * 1.24f * HashNubes(k, 301);
        float r = (lejos ? 62.0f : 38.0f) + 16.0f * HashNubes(k, 302);
        float radio = (lejos ? 7.0f : 4.5f) + 4.0f * HashNubes(k, 304);
        Vector3 c = { std::cos(a) * r, y + std::sin(t * 0.3f + (float)k) * 0.3f, std::sin(a) * r };

        IslaCampoNubes(c, radio, radio * 1.4f, k * 13 + 7, false);

        // Lomas, pinos y algun molino recortados en la bruma.
        FijarNieblaNubes(c);
        PrismaNubes({ c.x - radio * 0.2f, c.y, c.z - radio * 0.15f }, radio * 0.55f, radio * 0.2f, radio * 0.28f, 8, COLOR_PASTO_NUBES);

        int pinos = 2 + k % 3;

        for (int p = 0; p < pinos; p++)
        {
            float b = HashNubes(k, 320 + p) * 2.0f * PI;
            float d = radio * (0.35f + 0.4f * HashNubes(k, 330 + p));
            PinoNubes({ c.x + std::cos(b) * d, c.y, c.z + std::sin(b) * d }, 1.2f + radio * 0.18f);
        }

        if (k % 3 == 0)
        {
            MolinoNubes({ c.x + radio * 0.3f, c.y, c.z + radio * 0.2f }, 0.9f, t * 0.9f + (float)k);
        }

        NubeNubes({ c.x + radio * 0.6f, c.y - radio * 0.5f, c.z + radio * 0.3f }, radio * 0.42f, k, 0.5f);
    }
}


//==================================================
// VISUAL: ISLAS DE CAMPO, GLOBOS Y AVES (media distancia)
//==================================================

struct IslaCieloNubes
{
    float x, y, z;
    float radio;
    float profundidad;
    int tipo;
};

// A los costados y al fondo del cilindro de caida (adelante queda
// fuera de cuadro).
static const IslaCieloNubes ISLAS_CIELO_NUBES[] =
{
    { -15.5f, 93.0f, -6.0f, 4.3f, 5.2f, 0 },
    {  16.0f, 85.0f, -9.0f, 3.9f, 4.6f, 1 },
    { -14.5f, 75.0f, -12.0f, 3.6f, 4.4f, 2 },
    {  15.0f, 66.0f, -2.0f, 3.4f, 4.2f, 3 },
    { -16.5f, 57.0f, 0.0f, 4.0f, 5.0f, 4 },
    {  14.5f, 48.0f, -11.0f, 4.4f, 5.0f, 0 },
    { -13.5f, 39.0f, -13.0f, 3.7f, 4.5f, 1 },
    {  16.5f, 30.0f, 1.0f, 3.5f, 4.2f, 2 },
    { -15.0f, 21.0f, -4.0f, 3.9f, 4.6f, 3 },
    {  14.0f, 13.0f, -12.0f, 3.4f, 4.2f, 4 },
    {   2.0f, 70.0f, -22.0f, 4.8f, 5.6f, 1 },
    {  -3.0f, 34.0f, -23.0f, 4.6f, 5.4f, 0 }
};
static const int CANTIDAD_ISLAS_CIELO_NUBES = (int)(sizeof(ISLAS_CIELO_NUBES) / sizeof(ISLAS_CIELO_NUBES[0]));


struct GloboCieloNubes
{
    float y;
    float angulo;
    float radio;
    float escala;
    int paleta;
};

static const GloboCieloNubes GLOBOS_CIELO_NUBES[] =
{
    { 97.0f, 3.6f, 10.0f, 1.1f, 0 },
    { 88.0f, 5.9f, 9.5f, 0.95f, 1 },
    { 78.0f, 4.5f, 11.0f, 1.2f, 2 },
    { 63.0f, 3.3f, 9.8f, 1.0f, 3 },
    { 52.0f, 5.6f, 10.5f, 1.15f, 0 },
    { 42.0f, 4.2f, 9.6f, 1.0f, 1 },
    { 27.0f, 3.5f, 10.4f, 1.1f, 2 },
    { 17.0f, 5.8f, 9.8f, 0.95f, 3 }
};
static const int CANTIDAD_GLOBOS_NUBES = (int)(sizeof(GLOBOS_CIELO_NUBES) / sizeof(GLOBOS_CIELO_NUBES[0]));


static bool PosicionGloboNubes(int k, float t, float ca, Vector3& centro)
{
    const GloboCieloNubes& g = GLOBOS_CIELO_NUBES[k];

    if (g.y < ca - 40.0f || g.y > ca + 13.0f)
    {
        return false;
    }

    float a = g.angulo + t * 0.012f * (k % 2 == 0 ? 1.0f : -1.0f);
    centro = { std::cos(a) * g.radio, g.y + std::sin(t * 0.7f + (float)k) * 0.35f, std::sin(a) * g.radio };
    return true;
}


// Globo aerostatico a gajos con barquilla de mimbre.
static void GloboNubes(Vector3 c, float escala, int paleta)
{
    const Color colores[4][2] =
    {
        { COLOR_CAL_NUBES, Color{ 200, 96, 72, 255 } },
        { COLOR_CAL_NUBES, Color{ 60, 160, 170, 255 } },
        { Color{ 166, 126, 198, 255 }, Color{ 250, 228, 160, 255 } },
        { Color{ 234, 144, 66, 255 }, Color{ 132, 62, 80, 255 } }
    };
    const int GAJOS = 12;
    const int FILAS = 8;
    float R = 1.5f * escala;
    float rr[FILAS + 1];
    float yy[FILAS + 1];

    for (int i = 0; i <= FILAS; i++)
    {
        float th = 2.55f * (float)i / (float)FILAS;
        rr[i] = R * std::sin(th) + (i == FILAS ? R * 0.22f : 0.0f);
        yy[i] = c.y + R * 1.1f * std::cos(th);
    }

    FijarNieblaNubes(c);
    rlCheckRenderBatchLimit(FILAS * GAJOS * 4);
    rlBegin(RL_QUADS);

    for (int i = 0; i < FILAS; i++)
    {
        float alturaLuz = 1.06f - 0.3f * (float)i / (float)FILAS;

        for (int g = 0; g < GAJOS; g++)
        {
            float a0 = 2.0f * PI * (float)g / (float)GAJOS;
            float a1 = 2.0f * PI * (float)(g + 1) / (float)GAJOS;
            Color color = CaraNubes(colores[paleta % 4][g % 2], LuzLadoNubes(a0 + PI / (float)GAJOS) * alturaLuz + 0.12f, i < 3);

            QuadNubes(
                { c.x + std::cos(a0) * rr[i], yy[i], c.z + std::sin(a0) * rr[i] },
                { c.x + std::cos(a1) * rr[i], yy[i], c.z + std::sin(a1) * rr[i] },
                { c.x + std::cos(a1) * rr[i + 1], yy[i + 1], c.z + std::sin(a1) * rr[i + 1] },
                { c.x + std::cos(a0) * rr[i + 1], yy[i + 1], c.z + std::sin(a0) * rr[i + 1] },
                color
            );
        }
    }

    rlEnd();

    // Barquilla, cuerdas y bolsas de arena.
    float boca = yy[FILAS];
    float cesta = boca - 0.95f * escala;
    float lado = 0.3f * escala;
    CajaNubes({ c.x, cesta, c.z }, { lado * 2.0f, 0.42f * escala, lado * 2.0f }, Color{ 176, 124, 72, 255 });
    CajaNubes({ c.x, cesta + 0.22f * escala, c.z }, { lado * 2.1f, 0.06f * escala, lado * 2.1f }, Color{ 120, 82, 50, 255 });
    CajaNubes({ c.x + lado * 1.05f, cesta - 0.05f * escala, c.z }, { 0.08f * escala, 0.16f * escala, 0.12f * escala }, Color{ 198, 178, 140, 255 });
    CajaNubes({ c.x - lado * 1.05f, cesta - 0.05f * escala, c.z }, { 0.08f * escala, 0.16f * escala, 0.12f * escala }, Color{ 198, 178, 140, 255 });

    Color cuerda = ConNieblaNubes(Color{ 92, 70, 52, 255 });

    for (int k = 0; k < 4; k++)
    {
        float a = PI * 0.25f + PI * 0.5f * (float)k;
        DrawLine3D(
            { c.x + std::cos(a) * rr[FILAS], boca, c.z + std::sin(a) * rr[FILAS] },
            { c.x + std::cos(a) * lado * 1.3f, cesta + 0.2f * escala, c.z + std::sin(a) * lado * 1.3f },
            cuerda
        );
    }
}


static void AveNubes(Vector3 p, float rumbo, float aleteo, float escala)
{
    FijarNieblaNubes(p);
    Color ala = ConNieblaNubes(Color{ 84, 90, 116, 255 });
    Color alaClara = ConNieblaNubes(Color{ 236, 232, 222, 255 });

    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(rumbo * RAD2DEG, 0.0f, 1.0f, 0.0f);
    rlScalef(escala, escala, escala);

    CajaNubes({ 0.0f, 0.0f, 0.0f }, { 0.14f, 0.12f, 0.46f }, Color{ 236, 232, 222, 255 });
    CajaNubes({ 0.0f, 0.05f, 0.27f }, { 0.11f, 0.11f, 0.13f }, Color{ 236, 232, 222, 255 });
    CajaNubes({ 0.0f, 0.04f, 0.36f }, { 0.05f, 0.04f, 0.08f }, Color{ 240, 160, 60, 255 });

    rlCheckRenderBatchLimit(12);
    rlBegin(RL_TRIANGLES);

    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s;
        VerticeNubes({ x * 0.06f, 0.02f, 0.12f }, alaClara);
        VerticeNubes({ x * 0.06f, 0.02f, -0.12f }, alaClara);
        VerticeNubes({ x * 0.36f, 0.02f + aleteo * 0.5f, -0.02f }, alaClara);

        VerticeNubes({ x * 0.36f, 0.02f + aleteo * 0.5f, -0.02f }, ala);
        VerticeNubes({ x * 0.33f, 0.02f + aleteo * 0.5f, 0.1f }, ala);
        VerticeNubes({ x * 0.7f, 0.02f + aleteo, -0.1f }, ala);
    }

    rlEnd();
    rlPopMatrix();
}


static void UtileriaIslaNubes(Vector3 c, float radio, int tipo, int semilla, float t)
{
    float r = radio;

    switch (tipo)
    {
        case 0:
            CampoTrigoNubes({ c.x + 0.3f * r, c.y, c.z + 0.15f * r }, 0.9f * r, 0.75f * r, 20.0f);
            MolinoNubes({ c.x - 0.38f * r, c.y, c.z - 0.25f * r }, 0.75f, t * 1.1f + (float)semilla);
            CercaNubes({ c.x - 0.5f * r, c.y, c.z + 0.55f * r }, { c.x + 0.25f * r, c.y, c.z + 0.72f * r }, 4);
            ArbolNubes({ c.x + 0.45f * r, c.y, c.z - 0.5f * r }, 1.4f, semilla);
            break;

        case 1:
            GraneroNubes({ c.x - 0.05f * r, c.y, c.z - 0.25f * r }, 0.72f, -12.0f + 25.0f * HashNubes(semilla, 700));
            FardoNubes({ c.x + 0.5f * r, c.y, c.z + 0.25f * r }, 0.3f, 30.0f);
            FardoNubes({ c.x + 0.25f * r, c.y, c.z + 0.5f * r }, 0.3f, 75.0f);
            FardoNubes({ c.x - 0.45f * r, c.y, c.z + 0.4f * r }, 0.28f, -20.0f);
            ArbolNubes({ c.x - 0.55f * r, c.y, c.z - 0.35f * r }, 1.5f, semilla + 1);
            break;

        case 2:
        {
            for (int k = 0; k < 4; k++)
            {
                float a = 0.6f + (float)k * 1.5f + HashNubes(semilla, 710 + k);
                ArbolNubes({ c.x + std::cos(a) * r * 0.55f, c.y, c.z + std::sin(a) * r * 0.5f }, 1.2f + 0.4f * HashNubes(semilla, 720 + k), semilla + k);
            }

            EstanqueNubes({ c.x + 0.1f * r, c.y, c.z + 0.05f * r }, 0.32f * r, t);

            // La cascada mira hacia el centro de la escena.
            float a = std::atan2(-c.z, -c.x);
            Vector3 afuera = { std::cos(a), 0.0f, std::sin(a) };
            CascadaNubes({ c.x + afuera.x * r * 0.92f, c.y - 0.1f, c.z + afuera.z * r * 0.92f }, afuera, 0.35f, 6.0f, t);
            break;
        }

        case 3:
            CasitaNubes({ c.x - 0.1f * r, c.y, c.z - 0.2f * r }, 15.0f * (HashNubes(semilla, 730) - 0.5f), t);
            ArbolNubes({ c.x + 0.5f * r, c.y, c.z - 0.3f * r }, 1.3f, semilla);
            ArbolNubes({ c.x - 0.55f * r, c.y, c.z + 0.2f * r }, 1.1f, semilla + 2);
            CercaNubes({ c.x - 0.3f * r, c.y, c.z + 0.6f * r }, { c.x + 0.45f * r, c.y, c.z + 0.45f * r }, 3);

            for (int k = 0; k < 8; k++)
            {
                float a = HashNubes(semilla, 740 + k) * 2.0f * PI;
                float d = r * (0.3f + 0.4f * HashNubes(semilla, 750 + k));
                const Color flores[3] = { Color{ 250, 240, 250, 255 }, Color{ 236, 150, 196, 255 }, Color{ 190, 160, 236, 255 } };
                CajaNubes({ c.x + std::cos(a) * d, c.y + 0.06f, c.z + std::sin(a) * d }, { 0.14f, 0.1f, 0.14f }, flores[k % 3]);
            }
            break;

        default:
        {
            // Amarradero: poste con un globo chico atado, cajones y barriles.
            Vector3 poste = { c.x + 0.2f * r, c.y, c.z + 0.1f * r };
            FijarNieblaNubes(poste);
            CajaNubes({ poste.x, poste.y + 0.6f, poste.z }, { 0.18f, 1.2f, 0.18f }, COLOR_MADERA_NUBES);
            CajaNubes({ c.x - 0.4f * r, c.y + 0.25f, c.z - 0.1f * r }, { 0.5f, 0.5f, 0.5f }, COLOR_MADERA_CLARA_NUBES);
            CajaNubes({ c.x - 0.35f * r, c.y + 0.7f, c.z - 0.1f * r }, { 0.38f, 0.38f, 0.38f }, COLOR_MADERA_CLARA_NUBES);
            PrismaNubes({ c.x - 0.1f * r, c.y, c.z - 0.5f * r }, 0.24f, 0.24f, 0.55f, 8, COLOR_MADERA_NUBES);
            PrismaNubes({ c.x + 0.15f * r, c.y, c.z - 0.55f * r }, 0.24f, 0.24f, 0.55f, 8, COLOR_MADERA_NUBES);
            ArbolNubes({ c.x + 0.55f * r, c.y, c.z - 0.4f * r }, 1.3f, semilla);

            Vector3 globo = { poste.x + 0.6f + 0.2f * std::sin(t * 0.6f), poste.y + 4.2f + 0.2f * std::sin(t * 0.9f), poste.z + 0.3f };
            DrawLine3D({ poste.x, poste.y + 1.2f, poste.z }, { globo.x, globo.y - 1.9f, globo.z }, ConNieblaNubes(Color{ 92, 70, 52, 255 }));
            GloboNubes(globo, 0.7f, semilla % 4);
            break;
        }
    }
}


static void DibujarIslasCieloNubes(float t, float ca)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();

    if (calidad == CALIDAD_BAJA)
    {
        return;
    }

    int paso = calidad == CALIDAD_ALTA ? 1 : 2;

    for (int k = 0; k < CANTIDAD_ISLAS_CIELO_NUBES; k += paso)
    {
        const IslaCieloNubes& d = ISLAS_CIELO_NUBES[k];

        if (d.y < ca - 50.0f || d.y > ca + 12.0f)
        {
            continue;
        }

        Vector3 c = { d.x, d.y + std::sin(t * 0.45f + (float)k) * 0.18f, d.z };
        IslaCampoNubes(c, d.radio, d.profundidad, k * 17 + 3, true);
        UtileriaIslaNubes(c, d.radio, d.tipo, k, t);
        NubeNubes({ c.x + d.radio * 0.4f, c.y - d.profundidad * 0.75f, c.z + d.radio * 0.2f }, d.radio * 0.45f, k, 0.5f);
    }
}


static void DibujarGlobosNubes(float t, float ca)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();

    if (calidad == CALIDAD_BAJA)
    {
        return;
    }

    int paso = calidad == CALIDAD_ALTA ? 1 : 2;

    for (int k = 0; k < CANTIDAD_GLOBOS_NUBES; k += paso)
    {
        Vector3 c{};

        if (PosicionGloboNubes(k, t, ca, c))
        {
            GloboNubes(c, GLOBOS_CIELO_NUBES[k].escala, GLOBOS_CIELO_NUBES[k].paleta);
        }
    }
}


// Bandadas en V que rodean el cilindro.
static void DibujarAvesNubes(float t, float ca)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();

    if (calidad == CALIDAD_BAJA)
    {
        return;
    }

    const float alturas[5] = { 92.0f, 70.0f, 50.0f, 31.0f, 12.0f };
    int paso = calidad == CALIDAD_ALTA ? 1 : 2;

    for (int b = 0; b < 5; b += paso)
    {
        float y0 = alturas[b] + std::sin(t * 0.4f + (float)b) * 0.8f;

        if (y0 < ca - 32.0f || y0 > ca + 12.0f)
        {
            continue;
        }

        float sentido = b % 2 == 0 ? 1.0f : -1.0f;
        float phi = t * 0.2f * sentido + (float)b * 1.9f;
        float r = 11.5f + (float)(b % 2) * 2.5f;
        Vector3 base = { std::cos(phi) * r, y0, std::sin(phi) * r };
        Vector3 v = { -std::sin(phi) * sentido, 0.0f, std::cos(phi) * sentido };
        Vector3 radial = { std::cos(phi), 0.0f, std::sin(phi) };
        float rumbo = std::atan2(v.x, v.z);

        for (int j = 0; j < 5; j++)
        {
            float fila = (float)((j + 1) / 2);
            float lado = j == 0 ? 0.0f : (j % 2 == 1 ? 1.0f : -1.0f);
            Vector3 p =
            {
                base.x - v.x * fila * 0.8f + radial.x * lado * fila * 0.65f,
                base.y + std::sin(t * 1.3f + (float)j) * 0.1f,
                base.z - v.z * fila * 0.8f + radial.z * lado * fila * 0.65f
            };

            AveNubes(p, rumbo, std::sin(t * 9.0f + (float)j * 0.7f + (float)b) * 0.35f, 1.0f);
        }
    }
}


// Capas de nubes alrededor del cilindro: al caer pasan hacia arriba (en
// calidad baja queda una por capa, parte del cielo simple).
static void DibujarNubesCercanasNubes(float t, float ca)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();
    int porCapa = calidad == CALIDAD_ALTA ? 3 : (calidad == CALIDAD_MEDIA ? 2 : 1);

    for (int s = 0; s < 18; s++)
    {
        float y = 2.0f + (float)s * 6.0f + HashNubes(s, 401) * 2.5f;

        if (y < ca - 24.0f || y > ca + 11.0f)
        {
            continue;
        }

        for (int j = 0; j < porCapa; j++)
        {
            int id = s * 7 + j;
            float a = HashNubes(id, 402) * 2.0f * PI + t * 0.012f * (s % 2 == 0 ? 1.0f : -1.0f);
            float r = 9.6f + HashNubes(id, 403) * 5.5f;
            Vector3 c = { std::cos(a) * r, y + HashNubes(id, 404) * 1.2f, std::sin(a) * r };

            if (c.z > 6.0f)
            {
                continue;
            }

            NubeNubes(c, 0.9f + HashNubes(id, 405) * 0.9f, id, 0.55f);
        }
    }
}


//==================================================
// VISUAL: ISLA DIANA (aterrizaje)
//==================================================

static const float RADIO_ZONA_ATERRIZAJE_NUBES = RADIO_CILINDRO_NUBES;
static const float ANGULOS_BALIZAS_NUBES[4] = { 0.785f, 2.356f, 3.927f, 5.498f };


static Vector3 PosicionBalizaNubes(int k)
{
    float r = RADIO_ZONA_ATERRIZAJE_NUBES + 0.95f;
    return { std::cos(ANGULOS_BALIZAS_NUBES[k]) * r, 0.0f, std::sin(ANGULOS_BALIZAS_NUBES[k]) * r };
}


static bool BalizaEncendidaNubes(int k, float t)
{
    return ((int)std::floor(t * 4.0f) & 3) == k;
}


// Diana pintada con primitivas (de lejos, o si falta el GLB).
static void DianaPrimitivaNubes(bool fallback)
{
    if (fallback)
    {
        DrawCylinder({ 0.0f, 0.0f, 0.0f }, RADIO_CENTRO_NUBES, RADIO_CENTRO_NUBES, 0.02f, 24, Color{ 235, 60, 60, 255 });
        DrawCylinder({ 0.0f, 0.01f, 0.0f }, RADIO_CENTRO_NUBES * 0.66f, RADIO_CENTRO_NUBES * 0.66f, 0.02f, 24, WHITE);
        DrawCylinder({ 0.0f, 0.02f, 0.0f }, RADIO_CENTRO_NUBES * 0.33f, RADIO_CENTRO_NUBES * 0.33f, 0.02f, 24, Color{ 235, 60, 60, 255 });
        return;
    }

    FijarNieblaNubes({ 0.0f, 0.0f, 0.0f });
    CoronaNubes({ 0.0f, 0.02f, 0.0f }, 0.0f, RADIO_CENTRO_NUBES, CaraNubes(Color{ 228, 70, 62, 255 }, 1.0f, true), 28);
    CoronaNubes({ 0.0f, 0.025f, 0.0f }, 0.0f, RADIO_CENTRO_NUBES * 0.66f, CaraNubes(Color{ 248, 246, 238, 255 }, 1.0f, true), 28);
    CoronaNubes({ 0.0f, 0.03f, 0.0f }, 0.0f, RADIO_CENTRO_NUBES * 0.33f, CaraNubes(Color{ 228, 70, 62, 255 }, 1.0f, true), 28);
}


static void DibujarIslaDianaNubes(float t, float ca)
{
    bool baja = CalidadDecoracion() == CALIDAD_BAJA;
    Vector3 c = { 0.0f, 0.0f, 0.0f };

    IslaCampoNubes(c, RADIO_ISLA_NUBES, 13.0f, 91, !baja && ca < 45.0f);

    if (ca > 70.0f)
    {
        DianaPrimitivaNubes(false);
        return;
    }

    // Cesped cortado en franjas dentro de la zona de aterrizaje.
    FijarNieblaNubes(c);
    rlCheckRenderBatchLimit(12 * 4);
    rlBegin(RL_QUADS);

    for (int s = -6; s < 6; s++)
    {
        float xm = (float)s + 0.5f;
        float h = std::sqrt(RADIO_ZONA_ATERRIZAJE_NUBES * RADIO_ZONA_ATERRIZAJE_NUBES - xm * xm);
        Color franja = CaraNubes(TonoNubes(COLOR_PASTO_NUBES, (s & 1) ? 1.07f : 0.95f), 1.0f, true);
        QuadNubes({ (float)s, 0.006f, h }, { (float)s + 1.0f, 0.006f, h }, { (float)s + 1.0f, 0.006f, -h }, { (float)s, 0.006f, -h }, franja);
    }

    rlEnd();

    // Borde de la zona de aterrizaje (el mismo radio del cilindro de caida):
    // linea de cal y mojones.
    CoronaNubes({ 0.0f, 0.012f, 0.0f }, RADIO_ZONA_ATERRIZAJE_NUBES - 0.06f, RADIO_ZONA_ATERRIZAJE_NUBES + 0.08f, CaraNubes(COLOR_CAL_NUBES, 1.0f, true), 64);

    int mojones = baja ? 16 : 32;

    for (int k = 0; k < mojones; k++)
    {
        float a = 2.0f * PI * (float)k / (float)mojones;
        float r = RADIO_ZONA_ATERRIZAJE_NUBES + 0.32f;
        CajaNubes({ std::cos(a) * r, 0.07f, std::sin(a) * r }, { 0.24f, 0.14f, 0.24f }, COLOR_CAL_NUBES);
    }

    // Diana: GLB de cerca; de lejos, la version pintada.
    if (ca <= 30.0f)
    {
        if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_DIANA, { 0.0f, 0.0f, 0.0f }))
        {
            DianaPrimitivaNubes(true);
        }
    }
    else
    {
        DianaPrimitivaNubes(false);
    }

    // Aro dorado: borde exacto del bonus de +5.
    FijarNieblaNubes(c);
    CoronaNubes({ 0.0f, 0.045f, 0.0f }, RADIO_CENTRO_NUBES, RADIO_CENTRO_NUBES + 0.16f, CaraNubes(COLOR_ACENTO_NUBES, 1.0f, true), 40);

    for (int k = 0; k < 4; k++)
    {
        BalizaNubes(PosicionBalizaNubes(k), BalizaEncendidaNubes(k, t));
    }

    if (baja)
    {
        return;
    }

    // Granja alrededor de la pista.
    MolinoNubes({ -9.0f, 0.0f, -4.6f }, 1.3f, t * 0.9f);
    GraneroNubes({ 8.4f, 0.0f, -5.6f }, 1.0f, -18.0f);
    CampoTrigoNubes({ 9.4f, 0.0f, 0.6f }, 2.6f, 3.4f, 12.0f);
    FardoNubes({ 7.6f, 0.0f, 3.8f }, 0.36f, 30.0f);
    FardoNubes({ 8.6f, 0.0f, 4.6f }, 0.36f, 75.0f);
    FardoNubes({ 7.2f, 0.0f, 5.1f }, 0.34f, -10.0f);
    EstanqueNubes({ -8.6f, 0.0f, 3.2f }, 1.3f, t);
    MangaVientoNubes({ -7.4f, 0.0f, -0.6f }, t);
    CasitaNubes({ -3.8f, 0.0f, -9.6f }, 8.0f, t);

    const Vector3 arboles[7] =
    {
        { 1.2f, 0.0f, -9.8f }, { 4.4f, 0.0f, -9.0f }, { -6.8f, 0.0f, -8.0f }, { 10.4f, 0.0f, -1.8f },
        { -10.6f, 0.0f, 0.2f }, { -5.2f, 0.0f, 8.6f }, { 4.2f, 0.0f, 9.4f }
    };

    for (int k = 0; k < 7; k++)
    {
        ArbolNubes(arboles[k], 1.5f + 0.5f * HashNubes(k, 760), 30 + k);
    }

    CercaNubes({ -2.8f, 0.0f, 10.3f }, { 2.6f, 0.0f, 10.4f }, 5);
    CercaNubes({ 6.6f, 0.0f, 7.6f }, { 9.6f, 0.0f, 5.8f }, 3);

    for (int k = 0; k < 26; k++)
    {
        float a = HashNubes(k, 770) * 2.0f * PI;
        float d = 6.9f + 4.3f * HashNubes(k, 771);
        const Color flores[4] = { Color{ 250, 244, 250, 255 }, Color{ 238, 150, 196, 255 }, Color{ 190, 160, 238, 255 }, Color{ 250, 226, 120, 255 } };
        CajaNubes({ std::cos(a) * d, 0.06f, std::sin(a) * d }, { 0.14f, 0.1f, 0.14f }, flores[k % 4]);
    }

    // Cascada al fondo que cae al mar de nubes.
    float a = -2.2f;
    Vector3 afuera = { std::cos(a), 0.0f, std::sin(a) };
    CascadaNubes({ afuera.x * RADIO_ISLA_NUBES * 0.95f, -0.15f, afuera.z * RADIO_ISLA_NUBES * 0.95f }, afuera, 0.6f, 9.0f, t);
}


//==================================================
// VISUAL: PIEZAS JUGABLES
//==================================================

// MODELO FUTURO: islas de campo, granero, molino, casita, globos y aves
// (hoy con primitivas). Planeadores, anillos, estrella, tormenta, viento
// y diana ya son GLB.

static void DibujarObjetosNubes(const MinijuegoDescensoNubes& m, int limite)
{
    float ca = m.alturaCamara;
    float t = m.tiempoAnimacion;

    unsigned int todos = 0u;

    for (int i = 0; i < limite; i++)
    {
        if (m.participa[i])
        {
            todos |= (1u << i);
        }
    }

    for (int k = 0; k < m.cantidadAnillos; k++)
    {
        const AnilloNubes& anillo = m.anillos[k];

        if (
            anillo.posicion.y < ca - 17.0f ||
            anillo.posicion.y > ca + 9.0f ||
            (anillo.recogidoPor & todos) == todos
        )
        {
            continue;
        }

        bool dorado = anillo.valor >= 3;
        Color color = dorado ? Color{ 255, 205, 40, 255 } : Color{ 255, 255, 255, 255 };
        float radio = dorado ? 1.0f : 0.85f;
        Vector3 p = anillo.posicion;

        bool modelo = DibujarModeloDescensoNubesRetro3D(
            dorado ? MODELO_NUBES_ANILLO_DORADO : MODELO_NUBES_ANILLO_BLANCO, p,
            0, {0,1,0}, {1,1,1}, WHITE, dorado ? 0 : -1);
        if (modelo && dorado)
        {
            DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ANILLO_DORADO, p, 0, {0,1,0}, {1,1,1}, WHITE, 1);
            // BOMBILLAS conserva su material y pulsa alrededor del centro local.
            float pulso = (0.28f + 0.05f * std::sin(t * 6.0f)) / 0.252f;
            DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ANILLO_DORADO,
                {p.x,p.y + 0.12f * (1.0f - pulso),p.z}, 0, {0,1,0}, {pulso,pulso,pulso}, WHITE, 2);
        }
        if (!modelo)
        {
            for (int g = 0; g < 6; g++)
            {
                DrawCircle3D({ p.x, p.y, p.z }, radio - (float)g * 0.05f, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
            }

            DrawCircle3D({ p.x, p.y + 0.06f, p.z }, radio, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
            // Contorno oscuro para que el anillo se lea sobre el cielo claro.
            DrawCircle3D({ p.x, p.y, p.z }, radio + 0.06f, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 20, 50, 140, 255 });

            if (dorado)
            {
                DrawSphereEx({ p.x, p.y, p.z }, 0.28f + 0.05f * std::sin(t * 6.0f), 8, 8, Color{ 255, 230, 90, 255 });
            }
        }
    }

    for (int j = 0; j < m.cantidadTormentas; j++)
    {
        const TormentaNubes& tormenta = m.tormentas[j];

        if (tormenta.posicion.y < ca - 17.0f || tormenta.posicion.y > ca + 9.0f)
        {
            continue;
        }

        Vector3 p = tormenta.posicion;
        bool rayo = std::fmod(t * 3.0f + (float)j, 1.0f) < 0.3f;
        float escalaTormenta = tormenta.radio / 1.6f;
        Vector3 escala = {escalaTormenta,escalaTormenta,escalaTormenta};
        bool modelo = DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_TORMENTA, p, 0, {0,1,0}, escala, WHITE, 0);
        if (modelo)
        {
            for (int malla = 1; malla < (rayo ? 4 : 3); malla++)
                DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_TORMENTA, p, 0, {0,1,0}, escala, WHITE, malla);
        }
        else
        {
            DrawSphereEx(p, tormenta.radio * 0.8f, 10, 10, Color{ 66, 68, 92, 255 });
            DrawSphereEx({ p.x + tormenta.radio * 0.7f, p.y - 0.1f, p.z }, tormenta.radio * 0.6f, 8, 8, Color{ 80, 82, 108, 255 });
            DrawSphereEx({ p.x - tormenta.radio * 0.7f, p.y - 0.15f, p.z + 0.2f }, tormenta.radio * 0.55f, 8, 8, Color{ 76, 78, 100, 255 });

            if (rayo)
            {
                DrawLine3D({ p.x, p.y, p.z }, { p.x + 0.3f, p.y - 0.8f, p.z }, YELLOW);
                DrawLine3D({ p.x + 0.3f, p.y - 0.8f, p.z }, { p.x - 0.1f, p.y - 1.3f, p.z }, YELLOW);
            }
        }
        // El aro rojo del GLB se omite: la zona exacta de riesgo va en
        // DibujarGuiasNubes.
    }

    for (int v = 0; v < m.cantidadVientos; v++)
    {
        const VientoNubes& viento = m.vientos[v];

        if (viento.altura < ca - 17.0f || viento.altura > ca + 9.0f)
        {
            continue;
        }

        if (DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_BANDA, {0,viento.altura,0}, 0, {0,1,0}, {1,1,1}, WHITE, 0, true))
            DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_BANDA, {0,viento.altura,0}, 0, {0,1,0}, {1,1,1}, WHITE, 1, true);
        else
            DrawCylinderWires({ 0.0f, viento.altura - MITAD_BANDA_VIENTO_NUBES, 0.0f }, RADIO_CILINDRO_NUBES, RADIO_CILINDRO_NUBES, MITAD_BANDA_VIENTO_NUBES * 2.0f, 12, Fade(SKYBLUE, 0.22f));

        for (int a = 0; a < 5; a++)
        {
            float lateral = ((float)a - 2.0f) * 2.2f;
            float avance = std::fmod(t * 2.0f + (float)a * 0.4f, 3.0f) - 1.5f;
            Vector3 centro =
            {
                viento.dirZ != 0.0f ? lateral : avance * 2.0f * viento.dirX,
                viento.altura,
                viento.dirZ != 0.0f ? avance * 2.0f * viento.dirZ : lateral
            };

            Vector3 inicio = { centro.x - viento.dirX * 0.8f, centro.y, centro.z - viento.dirZ * 0.8f };
            Vector3 punta = { centro.x + viento.dirX * 0.4f, centro.y, centro.z + viento.dirZ * 0.4f };
            Vector3 fin = { centro.x + viento.dirX * 1.2f, centro.y, centro.z + viento.dirZ * 1.2f };

            // +X local gira hacia el viento. En raylib Y positivo lleva +X a -Z.
            if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_FLECHA, centro,
                -std::atan2(viento.dirZ,viento.dirX) * RAD2DEG))
            {
                DrawCylinderEx(inicio, punta, 0.08f, 0.08f, 5, Color{ 255, 255, 255, 255 });
                DrawCylinderEx(punta, fin, 0.3f, 0.0f, 6, Color{ 90, 210, 255, 255 });
            }
        }
    }
}


static void DibujarPlaneadorNubes(
    const MinijuegoDescensoNubes& m,
    int indice,
    const JugadorPrueba& jugador
)
{
    const EstadoJugadorNubes& estado = m.estados[indice];
    Color color = m.coloresJugadores[indice];
    float ancho = estado.frenando > 0.0f ? 3.4f : 2.2f;
    float arriba = jugador.posicion.y + 1.3f;

    if (!DibujarModeloDescensoNubesRetro3D(estado.frenando > 0 ? MODELO_NUBES_PLANEADOR_FRENADO : MODELO_NUBES_PLANEADOR,
        jugador.posicion, 0, {0,1,0}, {1,1,1}, color))
    {
        DrawCube({ jugador.posicion.x, arriba, jugador.posicion.z }, ancho, 0.06f, 1.1f, color);
        DrawCube({ jugador.posicion.x, arriba + 0.04f, jugador.posicion.z }, ancho * 0.5f, 0.04f, 1.12f, WHITE);
        DrawLine3D({ jugador.posicion.x, jugador.posicion.y + 0.5f, jugador.posicion.z }, { jugador.posicion.x - ancho * 0.45f, arriba, jugador.posicion.z }, LIGHTGRAY);
        DrawLine3D({ jugador.posicion.x, jugador.posicion.y + 0.5f, jugador.posicion.z }, { jugador.posicion.x + ancho * 0.45f, arriba, jugador.posicion.z }, LIGHTGRAY);
    }

    if (estado.aturdido > 0.0f)
    {
        for (int s = 0; s < 3; s++)
        {
            float angulo = m.tiempoAnimacion * 10.0f + (float)s * 2.1f;
            Vector3 p = { jugador.posicion.x + std::cos(angulo) * 0.7f, jugador.posicion.y + 1.0f, jugador.posicion.z + std::sin(angulo) * 0.7f };
            float escalaEstrella = 0.1f / 0.29f;
            if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ESTRELLA, p, -65, {1,0,0},
                {escalaEstrella,escalaEstrella,escalaEstrella}))
                DrawSphereEx(p, 0.1f, 4, 4, YELLOW);
        }
    }
}


static void DibujarParticulasNubes(const MinijuegoDescensoNubes& m)
{
    SinNieblaNubes();

    for (int i = 0; i < MAX_PARTICULAS_NUBES; i++)
    {
        const ParticulaNubes& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        float f = p.vida / p.vidaMaxima;
        float s = p.tamano * (0.45f + 0.55f * f);
        CajaNubes(p.posicion, { s, s, s }, p.brilla ? MezclaNubes(WHITE, p.color, 0.4f + 0.6f * (1.0f - f)) : p.color);
    }
}


// Vilanos que flotan (fijos en el mundo: al caer se ven subir).
static void DibujarVilanosNubes(float t, float ca)
{
    int cantidad = (int)(56.0f * FactorCalidadGrafica(CalidadParticulas()));
    SinNieblaNubes();

    for (int k = 0; k < cantidad; k++)
    {
        float x = (HashNubes(k, 800) - 0.5f) * 30.0f + std::sin(t * 0.6f + (float)k) * 0.5f;
        float z = -15.0f + HashNubes(k, 801) * 21.0f;

        if (x * x + z * z < 6.8f * 6.8f)
        {
            continue;
        }

        float y = ca - 22.0f + std::fmod(HashNubes(k, 802) * 34.0f - ca + 3400.0f + t * 0.35f, 34.0f);
        FijarNieblaNubes({ x, y, z });
        CajaNubes({ x, y, z }, { 0.07f, 0.07f, 0.07f }, Color{ 255, 252, 240, 255 });
    }
}


//==================================================
// VISUAL: TRANSLUCIDOS (arcoiris, guias y peligro)
//==================================================

static void DibujarArcoirisNubes(float ca)
{
    if (CalidadDecoracion() == CALIDAD_BAJA || ca < 22.0f)
    {
        return;
    }

    float alfa = Limitar01Nubes((ca - 22.0f) / 12.0f);
    const Color bandas[7] =
    {
        Color{ 238, 96, 86, 255 }, Color{ 246, 154, 74, 255 }, Color{ 250, 218, 96, 255 },
        Color{ 124, 200, 112, 255 }, Color{ 92, 172, 230, 255 }, Color{ 112, 122, 212, 255 },
        Color{ 164, 114, 202, 255 }
    };
    // Arco recostado hacia atras (mira a la camara que mira hacia abajo),
    // corrido a la izquierda para no cruzar el cilindro de caida.
    Vector3 centro = { -30.0f, 8.0f, -30.0f };
    Vector3 u = { 1.0f, 0.0f, 0.0f };
    Vector3 v = { 0.0f, 0.62f, -0.78f };
    const int PASOS = 40;

    rlCheckRenderBatchLimit(7 * PASOS * 4);
    rlBegin(RL_QUADS);

    for (int b = 0; b < 7; b++)
    {
        float r0 = 31.0f - (float)b * 0.8f;
        float r1 = r0 - 0.8f;
        float borde = (b == 0 || b == 6) ? 0.7f : 1.0f;

        for (int s = 0; s < PASOS; s++)
        {
            float a0 = PI * (float)s / (float)PASOS;
            float a1 = PI * (float)(s + 1) / (float)PASOS;
            // Las patas se pierden en la bruma del mar de nubes.
            float f0 = std::sin(a0);
            float f1 = std::sin(a1);
            Color c0 = AlfaNubes(MezclaNubes(bandas[b], COLOR_NIEBLA_NUBES, 0.2f), 0.42f * alfa * borde * (0.25f + 0.75f * f0));
            Color c1 = AlfaNubes(MezclaNubes(bandas[b], COLOR_NIEBLA_NUBES, 0.2f), 0.42f * alfa * borde * (0.25f + 0.75f * f1));
            Vector3 p00 = { centro.x + u.x * std::cos(a0) * r0 + v.x * f0 * r0, centro.y + v.y * f0 * r0, centro.z + v.z * f0 * r0 };
            Vector3 p01 = { centro.x + u.x * std::cos(a0) * r1 + v.x * f0 * r1, centro.y + v.y * f0 * r1, centro.z + v.z * f0 * r1 };
            Vector3 p11 = { centro.x + u.x * std::cos(a1) * r1 + v.x * f1 * r1, centro.y + v.y * f1 * r1, centro.z + v.z * f1 * r1 };
            Vector3 p10 = { centro.x + u.x * std::cos(a1) * r0 + v.x * f1 * r0, centro.y + v.y * f1 * r0, centro.z + v.z * f1 * r0 };
            VerticeNubes(p00, c0);
            VerticeNubes(p01, c0);
            VerticeNubes(p11, c1);
            VerticeNubes(p10, c1);
        }
    }

    rlEnd();
}


static void DibujarGuiasNubes(
    const MinijuegoDescensoNubes& m,
    const JugadorPrueba jugadores[],
    int limite,
    float t
)
{
    float ca = m.alturaCamara;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    DibujarArcoirisNubes(ca);

    bool enElAire = false;

    for (int i = 0; i < limite; i++)
    {
        if (m.participa[i] && !m.estados[i].aterrizo)
        {
            enElAire = true;
        }
    }

    // Borde del cilindro de caida, a la altura de los planeadores.
    if (enElAire && ca > 1.0f)
    {
        const int TRAZOS = 40;
        float y = ca;

        for (int k = 0; k < TRAZOS; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)TRAZOS + t * 0.05f;
            float a1 = a0 + 2.0f * PI / (float)TRAZOS * 0.55f;
            ArcoPlanoNubes({ 0.0f, y, 0.0f }, RADIO_CILINDRO_NUBES - 0.06f, RADIO_CILINDRO_NUBES + 0.06f, a0, a1, Color{ 255, 255, 255, 120 });
        }
    }

    // Tormentas: zona exacta del golpe (su radio mas el del planeador),
    // a rayas rojas y amarillas; titila rapido si alguien va a caer encima.
    for (int j = 0; j < m.cantidadTormentas; j++)
    {
        const TormentaNubes& tormenta = m.tormentas[j];

        if (tormenta.posicion.y < ca - 17.0f || tormenta.posicion.y > ca + 9.0f)
        {
            continue;
        }

        Vector3 p = tormenta.posicion;
        float alcance = tormenta.radio + 0.35f;
        bool aviso = false;

        for (int i = 0; i < limite; i++)
        {
            if (!m.participa[i] || m.estados[i].aterrizo || (tormenta.golpeadoA & (1u << i)) != 0u)
            {
                continue;
            }

            float dy = m.estados[i].altura - p.y;
            float dx = jugadores[i].posicion.x - p.x;
            float dz = jugadores[i].posicion.z - p.z;

            if (dy > -0.9f && dy < 6.0f && std::sqrt(dx * dx + dz * dz) < alcance + 1.0f)
            {
                aviso = true;
            }
        }

        float pulso = 0.5f + 0.5f * std::sin(t * (aviso ? 16.0f : 4.0f) + (float)j);
        float alfa = aviso ? 0.65f + 0.35f * pulso : 0.42f + 0.18f * pulso;
        const int TRAZOS = 28;

        for (int k = 0; k < TRAZOS; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)TRAZOS - t * 0.4f;
            float a1 = a0 + 2.0f * PI / (float)TRAZOS;
            Color c = (k % 2 == 0) ? Color{ 236, 62, 52, 255 } : Color{ 255, 210, 60, 255 };
            ArcoPlanoNubes(p, alcance - 0.1f, alcance + 0.1f, a0, a1, AlfaNubes(c, alfa));
        }

        if (aviso)
        {
            float onda = std::fmod(t * 1.8f + (float)j * 0.3f, 1.0f);
            CoronaNubes(p, alcance + 0.2f + onda * 0.9f, alcance + 0.32f + onda * 0.9f, AlfaNubes(Color{ 255, 80, 60, 255 }, 0.7f * (1.0f - onda)), 40);
        }

        // Lluvia bajo la nube.
        if (CalidadEfectos() != CALIDAD_BAJA)
        {
            Color gota = Color{ 186, 204, 240, 170 };

            for (int k = 0; k < 14; k++)
            {
                float a = HashNubes(j * 31 + k, 850) * 2.0f * PI;
                float d = tormenta.radio * 0.75f * std::sqrt(HashNubes(j * 31 + k, 851));
                float caida = std::fmod(t * 5.0f + HashNubes(j * 31 + k, 852) * 3.0f, 3.0f);
                Vector3 g0 = { p.x + std::cos(a) * d, p.y - 0.6f - caida, p.z + std::sin(a) * d };
                DrawLine3D(g0, { g0.x, g0.y - 0.45f, g0.z }, gota);
            }
        }
    }

    // Guia de cada planeador: aro de recogida (radio exacto) y plomada.
    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i] || m.estados[i].aterrizo)
        {
            continue;
        }

        Color c = m.coloresJugadores[i];
        Vector3 p = { jugadores[i].posicion.x, m.estados[i].altura, jugadores[i].posicion.z };
        CoronaNubes(p, RADIO_RECOGER_NUBES - 0.1f, RADIO_RECOGER_NUBES, AlfaNubes(c, 0.85f), 32);
        CoronaNubes(p, 0.0f, RADIO_RECOGER_NUBES - 0.1f, AlfaNubes(c, 0.16f), 32);

        for (int d = 1; d <= 9; d++)
        {
            Vector3 q = { p.x, p.y - (float)d * 1.0f, p.z };
            CoronaNubes(q, 0.0f, 0.09f, AlfaNubes(MezclaNubes(c, WHITE, 0.3f), 0.7f * (1.0f - (float)d / 10.0f)), 8);
        }
    }

    // Hebras de viento: al caer pasan hacia arriba (sensacion de velocidad).
    if (CalidadEfectos() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 26; k++)
        {
            float x = (HashNubes(k, 860) - 0.5f) * 26.0f;
            float z = -12.0f + HashNubes(k, 861) * 18.0f;

            if (x * x + z * z < 7.0f * 7.0f)
            {
                continue;
            }

            float y = ca - 20.0f + std::fmod(HashNubes(k, 862) * 30.0f - ca + 3000.0f, 30.0f);
            DrawLine3D({ x, y, z }, { x, y + 1.6f, z }, Color{ 255, 255, 255, 90 });
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: LUCES FALSAS (aditivas, al final del 3D)
//==================================================

// Punto del zigzag de un rayo (estable mientras dura el destello).
static Vector3 PuntoRayoNubes(Vector3 origen, int semilla, int k, float largo)
{
    float f = (float)k / 5.0f;
    float lado = k == 0 || k == 5 ? 0.0f : (HashNubes(semilla, 870 + k) - 0.5f) * 0.9f;
    float fondo = k == 0 || k == 5 ? 0.0f : (HashNubes(semilla, 880 + k) - 0.5f) * 0.6f;
    return { origen.x + lado, origen.y - largo * f, origen.z + fondo };
}


static void DibujarLucesNubes(
    const MinijuegoDescensoNubes& m,
    const JugadorPrueba jugadores[],
    int limite,
    float t
)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float ca = m.alturaCamara;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Haces de sol que bajan en diagonal por el costado izquierdo (fuera
    // del cilindro, para no lavar el centro).
    for (int k = 0; k < 3; k++)
    {
        float x0 = -42.0f + (float)k * 7.0f + 2.0f * std::sin(t * 0.1f + (float)k);
        Vector3 a = { x0, ca + 26.0f, -30.0f };
        Vector3 b = { x0 + 18.0f, ca - 40.0f, -10.0f };
        float ancho = 1.8f + 1.2f * HashNubes(k, 890);
        float alfa = 0.03f + 0.015f * std::sin(t * 0.5f + (float)k * 1.3f);
        Color c = AlfaNubes(Color{ 255, 222, 160, 255 }, alfa);
        rlCheckRenderBatchLimit(8);
        rlBegin(RL_QUADS);
        QuadNubes({ a.x - ancho, a.y, a.z }, { a.x + ancho, a.y, a.z }, { b.x + ancho, b.y, b.z }, { b.x - ancho, b.y, b.z }, c);
        QuadNubes({ a.x, a.y, a.z - ancho }, { a.x, a.y, a.z + ancho }, { b.x, b.y, b.z + ancho }, { b.x, b.y, b.z - ancho }, c);
        rlEnd();
    }

    unsigned int todos = 0u;

    for (int i = 0; i < limite; i++)
    {
        if (m.participa[i])
        {
            todos |= (1u << i);
        }
    }

    // Halo de los anillos dorados (el acento de la escena).
    for (int k = 0; k < m.cantidadAnillos; k++)
    {
        const AnilloNubes& anillo = m.anillos[k];

        if (
            anillo.valor < 3 ||
            anillo.posicion.y < ca - 17.0f ||
            anillo.posicion.y > ca + 9.0f ||
            (anillo.recogidoPor & todos) == todos
        )
        {
            continue;
        }

        float pulso = 0.85f + 0.15f * std::sin(t * 6.0f + (float)k);
        ResplandorVistaNubes(anillo.posicion, 1.9f * pulso, AlfaNubes(COLOR_ACENTO_NUBES, 0.3f));
        ResplandorVistaNubes(anillo.posicion, 0.75f, AlfaNubes(Color{ 255, 244, 200, 255 }, 0.35f));
    }

    // Rayos de las tormentas (mismo parpadeo que la malla del rayo).
    for (int j = 0; j < m.cantidadTormentas; j++)
    {
        const TormentaNubes& tormenta = m.tormentas[j];

        if (tormenta.posicion.y < ca - 17.0f || tormenta.posicion.y > ca + 9.0f)
        {
            continue;
        }

        float fase = std::fmod(t * 3.0f + (float)j, 1.0f);

        if (fase >= 0.3f)
        {
            continue;
        }

        float brillo = 1.0f - fase / 0.3f;
        Vector3 p = tormenta.posicion;
        ResplandorVistaNubes(p, tormenta.radio * 1.3f, AlfaNubes(Color{ 170, 180, 255, 255 }, 0.16f * brillo));

        int semilla = j * 97 + (int)std::floor(t * 3.0f + (float)j);
        Vector3 origen = { p.x + (HashNubes(semilla, 869) - 0.5f) * tormenta.radio, p.y - 0.3f, p.z };
        Color nucleo = AlfaNubes(Color{ 255, 252, 210, 255 }, 0.9f * brillo);
        Color halo = AlfaNubes(Color{ 200, 210, 255, 255 }, 0.3f * brillo);

        for (int k = 0; k < 5; k++)
        {
            Vector3 a = PuntoRayoNubes(origen, semilla, k, 2.6f);
            Vector3 b = PuntoRayoNubes(origen, semilla, k + 1, 2.6f);
            rlCheckRenderBatchLimit(8);
            rlBegin(RL_QUADS);
            QuadNubes({ a.x - 0.05f, a.y, a.z }, { a.x + 0.05f, a.y, a.z }, { b.x + 0.05f, b.y, b.z }, { b.x - 0.05f, b.y, b.z }, nucleo);
            QuadNubes({ a.x - 0.2f, a.y, a.z }, { a.x + 0.2f, a.y, a.z }, { b.x + 0.2f, b.y, b.z }, { b.x - 0.2f, b.y, b.z }, halo);
            rlEnd();
        }
    }

    // Quemadores de los globos.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

        for (int k = 0; k < CANTIDAD_GLOBOS_NUBES; k += paso)
        {
            Vector3 c{};

            if (!PosicionGloboNubes(k, t, ca, c))
            {
                continue;
            }

            float e = GLOBOS_CIELO_NUBES[k].escala;
            float llama = 0.6f + 0.4f * std::sin(t * 13.0f + (float)k * 2.0f) * std::sin(t * 7.0f + (float)k);
            Vector3 boca = { c.x, c.y - 1.5f * e * 1.1f * 0.83f, c.z };
            FijarNieblaNubes(c);
            float apagado = 1.0f - nieblaActualNubes;
            ResplandorVistaNubes(boca, 0.55f * e, AlfaNubes(Color{ 255, 170, 80, 255 }, 0.6f * llama * apagado));
            ResplandorVistaNubes({ c.x, c.y - 0.5f * e, c.z }, 1.6f * e, AlfaNubes(Color{ 255, 190, 120, 255 }, 0.12f * llama * apagado));
        }
    }

    // Balizas de la pista, sus charcos de luz y el resplandor de la diana.
    if (ca < 50.0f)
    {
        for (int k = 0; k < 4; k++)
        {
            Vector3 b = PosicionBalizaNubes(k);
            float on = BalizaEncendidaNubes(k, t) ? 1.0f : 0.25f;
            ResplandorVistaNubes({ b.x, 0.8f, b.z }, 0.6f, AlfaNubes(Color{ 255, 200, 110, 255 }, 0.55f * on));
            ResplandorSueloNubes({ b.x, 0.03f, b.z }, 2.2f, AlfaNubes(Color{ 255, 190, 100, 255 }, 0.14f * on));
        }

        float pulso = 0.75f + 0.25f * std::sin(t * 2.4f);
        ResplandorSueloNubes({ 0.0f, 0.06f, 0.0f }, RADIO_CENTRO_NUBES + 1.6f, AlfaNubes(COLOR_ACENTO_NUBES, 0.16f * pulso));
    }

    // Destello de cada planeador al sumar o aterrizar.
    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i] || m.destelloJugador[i] <= 0.0f)
        {
            continue;
        }

        Vector3 p = jugadores[i].posicion;
        p.y += 0.7f;
        ResplandorVistaNubes(p, 1.6f, AlfaNubes(m.coloresJugadores[i], 0.5f * m.destelloJugador[i]));
        ResplandorVistaNubes(p, 0.8f, AlfaNubes(WHITE, 0.4f * m.destelloJugador[i]));
    }

    // Chispas que brillan.
    for (int i = 0; i < MAX_PARTICULAS_NUBES; i++)
    {
        const ParticulaNubes& p = m.particulas[i];

        if (!p.activa || !p.brilla)
        {
            continue;
        }

        float f = p.vida / p.vidaMaxima;
        ResplandorVistaNubes(p.posicion, p.tamano * 3.0f, AlfaNubes(p.color, 0.35f * f));
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoDescensoNubes::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteNubes(cantidadMaxima);
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    const float t = tiempoAnimacion;
    const float ca = alturaCamara;

    Camera3D vista = VistaNubes(*this);
    FijarVistaNubes(vista);

    // El bucle principal no limpia la pantalla: limpiar antes del cielo.
    ClearBackground(COLOR_NIEBLA_NUBES);
    DibujarCieloNubes();

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte y lejania.
    DibujarMarNubes(t);
    DibujarIslasLejanasNubes(t, ca);

    // Isla diana, media distancia y anillo cercano.
    DibujarIslaDianaNubes(t, ca);
    DibujarIslasCieloNubes(t, ca);
    DibujarGlobosNubes(t, ca);
    DibujarAvesNubes(t, ca);
    DibujarNubesCercanasNubes(t, ca);

    // Piezas jugables y planeadores.
    DibujarObjetosNubes(*this, limite);

    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = coloresJugadores[i];
        DibujarJugadorCuboPrueba(jugador, visual);

        if (!estados[i].aterrizo)
        {
            DibujarPlaneadorNubes(*this, i, jugador);
        }

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }

    DibujarParticulasNubes(*this);
    DibujarVilanosNubes(t, ca);

    DibujarGuiasNubes(*this, jugadores, limite, t);
    DibujarLucesNubes(*this, jugadores, limite, t);

    rlEnableBackfaceCulling();
    EndMode3D();

    DibujarRayosSolNubes(t);

    // Destello del rayo que pega.
    if (relampagoTormenta > 0.0f && CalidadEfectos() != CALIDAD_BAJA)
    {
        DrawRectangle(0, 0, ancho, alto, Fade(Color{ 236, 240, 255, 255 }, 0.2f * relampagoTormenta));
    }

    // Cartelitos J# sobre cada planeador.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += estados[i].aterrizo ? 1.5f : 2.0f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        if (p.y < Px(132.0f))
        {
            p.y = Px(132.0f);
        }

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = coloresJugadores[i];
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // «+1», «+3», «¡RAYO! -2», «¡CENTRO! +5» sobre el mundo.
    for (int i = 0; i < MAX_AVISOS_NUBES; i++)
    {
        const AvisoNubes& aviso = avisos[i];

        if (!aviso.activo)
        {
            continue;
        }

        float u = 1.0f - aviso.vida / aviso.vidaMaxima;
        Vector2 s = GetWorldToScreen(aviso.posicion, vista);

        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.x < Px(80.0f)) s.x = Px(80.0f);
        if (s.x > (float)ancho - Px(80.0f)) s.x = (float)ancho - Px(80.0f);

        float alfa = aviso.vida < 0.35f ? aviso.vida / 0.35f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        DibujarTextoMundoContornoUI(aviso.texto, s.x, s.y, aviso.grande ? UI_TEXTO_GRANDE : UI_TEXTO, aviso.color, alfa);
    }

    // Anuncios: rafaga que se acerca o la diana a la vista.
    if (fase == FASE_NUBES_JUGANDO)
    {
        float yAnuncio = (float)alto * 0.22f;
        bool anunciado = false;

        for (int v = 0; v < cantidadVientos; v++)
        {
            float distancia = alturaCamara - vientos[v].altura;

            if (distancia > MITAD_BANDA_VIENTO_NUBES && distancia < 8.0f)
            {
                const char* direccion = vientos[v].dirX > 0.0f
                    ? "¡RÁFAGA HACIA LA DERECHA!"
                    : (vientos[v].dirX < 0.0f
                        ? "¡RÁFAGA HACIA LA IZQUIERDA!"
                        : (vientos[v].dirZ > 0.0f ? "¡RÁFAGA HACIA ABAJO!" : "¡RÁFAGA HACIA ARRIBA!"));
                float alfa = 0.75f + 0.25f * std::sin(t * 10.0f);
                DibujarTextoMundoContornoUI(direccion, (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 150, 228, 255, 255 }, alfa);
                anunciado = true;
                break;
            }
        }

        bool alguienCae = false;

        for (int i = 0; i < limite; i++)
        {
            if (participa[i] && !estados[i].aterrizo)
            {
                alguienCae = true;
            }
        }

        if (!anunciado && alguienCae && alturaCamara < 14.0f)
        {
            float alfa = Limitar01Nubes((14.0f - alturaCamara) / 2.0f) * (0.8f + 0.2f * std::sin(t * 8.0f));
            DibujarTextoMundoContornoUI("¡APUNTÁ A LA DIANA!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, COLOR_ACENTO_NUBES, alfa);
        }
    }

    // HUD comun (con los colores que ve el mundo, por si un bot cambio el suyo).
    Participante vistaHud[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        vistaHud[i] = participantes[i];

        if (participa[i])
        {
            vistaHud[i].color = coloresJugadores[i];
        }
    }

    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "DESCENSO EN NUBES",
        "PLANEÁ Y PASÁ POR LOS ANILLOS (DORADOS x3), ESQUIVÁ LAS TORMENTAS Y ATERRIZÁ EN LA DIANA (+5).",
        resultado
    );

    hud.fase = fase == FASE_NUBES_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_NUBES_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;

    float restante = TIEMPO_LIMITE_NUBES - tiempoJuego;
    hud.tiempoRestante = restante > 0.0f ? restante : 0.0f;
    hud.tiempoTotal = TIEMPO_LIMITE_NUBES;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = tiempoJuego >= TIEMPO_LIMITE_NUBES;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];

        if (!j.visible || i >= limite || !participa[i])
        {
            continue;
        }

        const EstadoJugadorNubes& e = estados[i];

        j.valorNumerico = e.puntos;
        std::snprintf(j.valor, sizeof(j.valor), "%d PTS", e.puntos);
        j.progreso = Limitar01Nubes(1.0f - e.altura / ALTURA_INICIAL_NUBES);

        if (e.aterrizo)
        {
            std::snprintf(j.estado, sizeof(j.estado), "%s", e.bonusCentro ? "¡EN LA DIANA!" : "EN TIERRA");
        }
        else if (e.aturdido > 0.0f)
        {
            std::snprintf(j.estado, sizeof(j.estado), "¡RAYO!");
        }
        else if (e.frenando > 0.0f)
        {
            std::snprintf(j.estado, sizeof(j.estado), "FRENANDO");
        }
        else
        {
            std::snprintf(j.estado, sizeof(j.estado), "ALTURA %d M", (int)std::ceil(e.altura));
        }

        std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "%d PTS", e.puntos);

        if (e.golpes == 1)
        {
            std::snprintf(j.detalleDesempate, sizeof(j.detalleDesempate), "1 TORMENTA");
        }
        else
        {
            std::snprintf(j.detalleDesempate, sizeof(j.detalleDesempate), "%d TORMENTAS", e.golpes);
        }

        if (!participantes[i].esBot && participantes[i].conectado)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "MOVÉ: %s\nFRENÁ: %s / %s",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (
        fase == FASE_NUBES_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        resultado.desenlace == DESENLACE_EMPATE
    )
    {
        hud.notaFinal = "EMPATE EN PUNTOS Y EN TORMENTAS";
    }

    DibujarHudMinijuego(hud, vistaHud);
}


const ResultadoMinijuego& MinijuegoDescensoNubes::ObtenerResultado() const
{
    return resultado;
}
