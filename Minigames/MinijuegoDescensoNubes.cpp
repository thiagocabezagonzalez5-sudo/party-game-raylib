#include "Minigames/MinijuegoDescensoNubes.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


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
static const float RADIO_ISLA_NUBES = 7.0f;
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


static const char* NombreJugadorNubes(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static const char* TextoGolpeNubes(const Participante& participante)
{
    if (participante.esBot) return "BOT";
    if (participante.control == CONTROL_GAMEPAD) return "B";
    if (participante.control == CONTROL_TECLADO_FLECHAS) return "SHIFT DER";
    return "E";
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

                if (
                    otro.r == respaldo[c].r &&
                    otro.g == respaldo[c].g &&
                    otro.b == respaldo[c].b
                )
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

    CargarPaqueteDescensoNubesRetro3D();
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

void MinijuegoDescensoNubes::Actualizar(
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
// VISUAL: CIELO (solo decoracion, la logica no depende de esto)
//==================================================

// MODELO FUTURO: planeador del jugador, anillos y estrellas, nubes de
// tormenta, flechas de viento, isla de aterrizaje con diana, islas
// flotantes, globos aerostaticos, molinos en nubes, aves y arcoiris: GLB.

static void DibujarNubeNubes(Vector3 p, float escala)
{
    if (DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_NUBE_BLANCA,
        p, 0, {0,1,0}, {escala,escala,escala})) return;
    DrawSphereEx(p, 1.1f * escala, 8, 8, Color{ 250, 250, 255, 255 });
    DrawSphereEx({ p.x + 1.0f * escala, p.y - 0.15f * escala, p.z }, 0.85f * escala, 8, 8, Color{ 240, 244, 252, 255 });
    DrawSphereEx({ p.x - 0.95f * escala, p.y - 0.2f * escala, p.z + 0.2f * escala }, 0.8f * escala, 8, 8, Color{ 238, 242, 250, 255 });
}


static void DibujarDecoracionNubes(const MinijuegoDescensoNubes& m)
{
    float ca = m.alturaCamara;
    float t = m.tiempoAnimacion;

    // Nubes sueltas alrededor del cilindro.
    for (int k = 0; k < 48; k++)
    {
        float altura = (float)k * 2.1f + HashNubes(k, 1) * 1.5f;

        if (altura < ca - 20.0f || altura > ca + 12.0f)
        {
            continue;
        }

        float angulo = HashNubes(k, 2) * 2.0f * PI;
        float radio = 8.5f + HashNubes(k, 3) * 7.0f;
        DibujarNubeNubes({ std::cos(angulo) * radio, altura, std::sin(angulo) * radio }, 1.0f + HashNubes(k, 4));
    }

    // Islas flotantes.
    for (int k = 0; k < 7; k++)
    {
        float altura = 12.0f + (float)k * 13.0f;

        if (altura < ca - 20.0f || altura > ca + 12.0f)
        {
            continue;
        }

        float lado = (k % 2 == 0) ? -1.0f : 1.0f;
        Vector3 c = { lado * (10.5f + HashNubes(k, 5) * 3.0f), altura, -4.0f + HashNubes(k, 6) * 8.0f };

        if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ISLA_FLOTANTE, c))
        {
            DrawCylinder({ c.x, c.y - 0.4f, c.z }, 2.4f, 2.4f, 0.4f, 12, Color{ 90, 190, 100, 255 });
            DrawCylinder({ c.x, c.y - 3.0f, c.z }, 0.5f, 2.4f, 2.6f, 12, Color{ 130, 100, 80, 255 });
            DrawCylinder({ c.x, c.y, c.z }, 0.1f, 0.1f, 1.0f, 6, Color{ 110, 76, 50, 255 });
            DrawSphereEx({ c.x, c.y + 1.3f, c.z }, 0.7f, 8, 8, Color{ 50, 150, 70, 255 });
        }
    }

    // Globos aerostaticos.
    for (int k = 0; k < 5; k++)
    {
        float altura = 20.0f + (float)k * 17.0f + std::sin(t * 0.8f + (float)k) * 0.6f;

        if (altura < ca - 20.0f || altura > ca + 12.0f)
        {
            continue;
        }

        float lado = (k % 2 == 0) ? 1.0f : -1.0f;
        Vector3 c = { lado * 9.5f, altura, 3.0f - (float)k };

        // El pivote del GLB esta en el centro de la envolvente, no la cesta.
        if (!DibujarModeloDescensoNubesRetro3D(k % 2 == 0 ? MODELO_NUBES_GLOBO_AZUL : MODELO_NUBES_GLOBO_ROJO,
            {c.x,c.y + 1.8f,c.z}))
        {
            DrawSphereEx({ c.x, c.y + 1.8f, c.z }, 1.6f, 10, 10, ColorFromHSV((float)k * 70.0f, 0.75f, 1.0f));
            DrawCube({ c.x, c.y - 0.3f, c.z }, 0.7f, 0.5f, 0.7f, Color{ 150, 100, 60, 255 });
            DrawLine3D({ c.x - 0.3f, c.y - 0.1f, c.z }, { c.x - 0.9f, c.y + 1.2f, c.z }, Color{ 80, 60, 40, 255 });
            DrawLine3D({ c.x + 0.3f, c.y - 0.1f, c.z }, { c.x + 0.9f, c.y + 1.2f, c.z }, Color{ 80, 60, 40, 255 });
        }
    }

    // Molinos de viento sobre nubes.
    for (int k = 0; k < 3; k++)
    {
        float altura = 30.0f + (float)k * 26.0f;

        if (altura < ca - 20.0f || altura > ca + 12.0f)
        {
            continue;
        }

        float lado = (k % 2 == 0) ? -1.0f : 1.0f;
        Vector3 base = { lado * 11.5f, altura, -6.0f };
        Vector3 eje = { base.x, base.y + 2.6f, base.z + 0.5f };

        DibujarNubeNubes({ base.x, base.y - 0.3f, base.z }, 1.6f);
        const float escalaMolino = 0.65f;
        Vector3 escala = {escalaMolino,escalaMolino,escalaMolino};
        if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_MOLINO, base, 0, {0,1,0}, escala))
        {
            DrawCylinder(base, 0.5f, 0.8f, 2.6f, 8, Color{ 220, 190, 150, 255 });
            DrawCylinder({ base.x, base.y + 2.6f, base.z }, 0.0f, 0.7f, 0.8f, 8, Color{ 200, 70, 60, 255 });
        }

        // El eje documentado pertenece a la torre: escalar tambien su offset.
        if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ASPAS,
            {base.x,base.y + 4.15f * escalaMolino,base.z + 0.88f * escalaMolino},
            t * 1.2f * RAD2DEG, {0,0,1}, escala))
        {
            for (int a = 0; a < 4; a++)
            {
                float angulo = t * 1.2f + (float)a * PI * 0.5f;
                DrawCylinderEx(eje, { eje.x + std::cos(angulo) * 2.2f, eje.y + std::sin(angulo) * 2.2f, eje.z }, 0.1f, 0.18f, 5, Color{ 250, 250, 245, 255 });
            }
        }
    }

    // Aves.
    for (int k = 0; k < 8; k++)
    {
        float altura = 6.0f + (float)k * 12.0f + std::sin(t + (float)k) * 1.0f;

        if (altura < ca - 20.0f || altura > ca + 12.0f)
        {
            continue;
        }

        float angulo = t * (0.35f + 0.05f * (float)k) + (float)k * 1.7f;
        Vector3 p = { std::cos(angulo) * 8.0f, altura, std::sin(angulo) * 8.0f };
        float aleteo = std::sin(t * 9.0f + (float)k) * 0.35f;

        if (!DibujarAveDescensoNubesRetro3D(p, aleteo))
        {
            DrawLine3D({ p.x - 0.5f, p.y + aleteo, p.z }, p, Color{ 60, 60, 70, 255 });
            DrawLine3D(p, { p.x + 0.5f, p.y + aleteo, p.z }, Color{ 60, 60, 70, 255 });
        }
    }

    // Arcoiris al fondo.
    const float alturaArcoiris = 62.0f;

    if (alturaArcoiris > ca - 20.0f && alturaArcoiris < ca + 24.0f)
    {
        if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ARCOIRIS, {0,alturaArcoiris,-16}))
        {
            for (int banda = 0; banda < 7; banda++)
            {
                Color color = ColorFromHSV((float)banda * 40.0f, 0.8f, 1.0f);
                float radio = 13.0f + (float)banda * 0.5f;

                for (int s = 0; s < 10; s++)
                {
                    float a1 = PI * (float)s / 10.0f;
                    float a2 = PI * (float)(s + 1) / 10.0f;
                    DrawCylinderEx(
                        { std::cos(a1) * radio, alturaArcoiris - 10.0f + std::sin(a1) * radio * 0.8f, -16.0f },
                        { std::cos(a2) * radio, alturaArcoiris - 10.0f + std::sin(a2) * radio * 0.8f, -16.0f },
                        0.25f, 0.25f, 4, color
                    );
                }
            }
        }
    }
}


static void DibujarIslaNubes(const MinijuegoDescensoNubes& m)
{
    if (m.alturaCamara > 30.0f)
    {
        return;
    }

    // Mar de nubes y base de la isla.
    if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_MAR, {0,0,0}))
        DrawPlane({ 0.0f, -2.5f, 0.0f }, { 80.0f, 80.0f }, Color{ 236, 242, 252, 255 });
    if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ISLA, {0,0,0}))
    {
        DrawCylinder({ 0.0f, -5.5f, 0.0f }, 1.0f, RADIO_ISLA_NUBES, 5.5f, 20, Color{ 130, 100, 80, 255 });
        DrawCylinder({ 0.0f, -0.3f, 0.0f }, RADIO_ISLA_NUBES, RADIO_ISLA_NUBES, 0.3f, 28, Color{ 90, 190, 100, 255 });
    }

    // Diana de aterrizaje.
    if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_DIANA, {0,0,0}))
    {
        DrawCylinder({ 0.0f, 0.0f, 0.0f }, RADIO_CENTRO_NUBES, RADIO_CENTRO_NUBES, 0.02f, 24, Color{ 235, 60, 60, 255 });
        DrawCylinder({ 0.0f, 0.01f, 0.0f }, RADIO_CENTRO_NUBES * 0.66f, RADIO_CENTRO_NUBES * 0.66f, 0.02f, 24, WHITE);
        DrawCylinder({ 0.0f, 0.02f, 0.0f }, RADIO_CENTRO_NUBES * 0.33f, RADIO_CENTRO_NUBES * 0.33f, 0.02f, 24, Color{ 235, 60, 60, 255 });
    }

    // Molino de la isla.
    Vector3 eje = { -5.2f, 4.2f, -3.0f };
    if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_MOLINO, {-5.2f,0,-3}))
    {
        DrawCylinder({ -5.2f, 0.0f, -3.0f }, 0.5f, 0.8f, 4.0f, 8, Color{ 220, 190, 150, 255 });
        DrawCylinder({ -5.2f, 4.0f, -3.0f }, 0.0f, 0.7f, 0.8f, 8, Color{ 200, 70, 60, 255 });
    }

    if (!DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_ASPAS, {-5.2f,4.15f,-2.12f},
        m.tiempoAnimacion * 1.2f * RAD2DEG, {0,0,1}))
    {
        for (int a = 0; a < 4; a++)
        {
            float angulo = m.tiempoAnimacion * 1.2f + (float)a * PI * 0.5f;
            DrawCylinderEx(eje, { eje.x + std::cos(angulo) * 2.2f, eje.y + std::sin(angulo) * 2.2f, eje.z + 0.5f }, 0.1f, 0.18f, 5, Color{ 250, 250, 245, 255 });
        }
    }
}


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

        // Mantener el indicador exacto de riesgo; omitir el aro rojo del GLB.
        DrawCircle3D({ p.x, p.y, p.z }, tormenta.radio + 0.35f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(RED, 0.6f));
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

    (void)t;
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
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    // Cielo: azul profundo arriba, celeste cerca de las nubes.
    float mezcla = LimitarNubes(alturaCamara / ALTURA_INICIAL_NUBES, 0.0f, 1.0f);
    Color cielo =
    {
        (unsigned char)(140.0f + (40.0f - 140.0f) * mezcla),
        (unsigned char)(205.0f + (70.0f - 205.0f) * mezcla),
        (unsigned char)(245.0f + (150.0f - 245.0f) * mezcla),
        255
    };
    ClearBackground(cielo);

    Camera3D vista = camara;
    vista.position = { 0.0f, alturaCamara + 13.0f, 7.5f };
    vista.target = { 0.0f, alturaCamara - 3.0f, 0.8f };

    BeginMode3D(vista);

    DibujarDecoracionNubes(*this);
    DibujarIslaNubes(*this);
    DibujarObjetosNubes(*this, limite);

    DrawCircle3D({ 0.0f, alturaCamara - 3.0f, 0.0f }, RADIO_CILINDRO_NUBES, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(WHITE, 0.2f));

    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];
        const EstadoJugadorNubes& estado = estados[i];
        Color color = coloresJugadores[i];

        if (!estado.aterrizo)
        {
            DrawCircle3D({ jugador.posicion.x, estado.altura, jugador.posicion.z }, 0.9f, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
            DrawLine3D(
                { jugador.posicion.x, estado.altura, jugador.posicion.z },
                { jugador.posicion.x, estado.altura - 9.0f, jugador.posicion.z },
                Fade(color, 0.5f)
            );
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = color;
        DibujarJugadorCuboPrueba(jugador, visual);

        if (!estado.aterrizo)
        {
            DibujarPlaneadorNubes(*this, i, jugador);
        }

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }

    EndMode3D();

    // Etiquetas flotantes.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.9f, jugadores[i].posicion.z },
            vista
        );
        const char* nombre = NombreJugadorNubes(participantes[i], i);
        int ancho = MeasureText(nombre, 18);

        int etiquetaY = (int)pantalla.y < 88 ? 88 : (int)pantalla.y;

        DrawRectangle((int)pantalla.x - ancho / 2 - 4, etiquetaY - 2, ancho + 8, 22, Fade(BLACK, 0.75f));
        DrawText(nombre, (int)pantalla.x - ancho / 2, etiquetaY, 18, coloresJugadores[i]);
    }

    // Cabecera.
    DrawRectangle(14, 12, 520, 66, Fade(BLACK, 0.74f));
    DrawText("DESCENSO EN NUBES", 28, 18, 28, GOLD);
    DrawText("MOVER: WASD/FLECHAS/STICK   FRENAR: BOTON PRINCIPAL O GOLPE", 28, 52, 14, RAYWHITE);

    if (fase == FASE_NUBES_JUGANDO)
    {
        float restante = TIEMPO_LIMITE_NUBES - tiempoJuego;
        DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
        DrawText(TextFormat("TIEMPO %.0f", restante > 0.0f ? restante : 0.0f), anchoPantalla - 200, 22, 26, restante <= 6.0f ? RED : GOLD);

        // Aviso de viento cercano.
        for (int v = 0; v < cantidadVientos; v++)
        {
            float distancia = alturaCamara - vientos[v].altura;

            if (distancia > MITAD_BANDA_VIENTO_NUBES && distancia < 8.0f)
            {
                const char* direccion = vientos[v].dirX > 0.0f ? "DERECHA" : (vientos[v].dirX < 0.0f ? "IZQUIERDA" : (vientos[v].dirZ > 0.0f ? "HACIA ABAJO" : "HACIA ARRIBA"));
                const char* aviso = TextFormat("RAFAGA DE VIENTO: %s", direccion);
                int a = MeasureText(aviso, 24);
                DrawRectangle(anchoPantalla / 2 - a / 2 - 14, 88, a + 28, 38, Fade(BLACK, 0.8f));
                DrawText(aviso, anchoPantalla / 2 - a / 2, 94, 24, SKYBLUE);
                break;
            }
        }
    }

    // Medidor de altura a la derecha.
    int barraX = anchoPantalla - 40;
    int barraY = 150;
    int barraAlto = 300;
    DrawRectangle(barraX, barraY, 10, barraAlto, Fade(BLACK, 0.6f));

    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        float proporcion = LimitarNubes(estados[i].altura / ALTURA_INICIAL_NUBES, 0.0f, 1.0f);
        DrawCircle(barraX + 5, barraY + (int)((1.0f - proporcion) * (float)barraAlto), 6.0f, coloresJugadores[i]);
    }

    DrawText("ISLA", barraX - 10, barraY + barraAlto + 6, 14, RAYWHITE);

    // Tarjetas inferiores.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participa[i]) activos++;
    }

    if (activos > 0)
    {
        int separacion = 10;
        int anchoTarjeta = (anchoPantalla - 28 - separacion * (activos - 1)) / activos;

        if (anchoTarjeta > 300) anchoTarjeta = 300;

        int xInicial = (anchoPantalla - (anchoTarjeta * activos + separacion * (activos - 1))) / 2;
        int k = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!participa[i])
            {
                continue;
            }

            int x = xInicial + k * (anchoTarjeta + separacion);
            int y = altoPantalla - 80;
            const EstadoJugadorNubes& estado = estados[i];

            DrawRectangle(x, y, anchoTarjeta, 70, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 70, coloresJugadores[i]);
            DrawText(NombreJugadorNubes(participantes[i], i), x + 18, y + 6, 18, coloresJugadores[i]);
            DrawText(TextFormat("%d PTS", estado.puntos), x + anchoTarjeta - 90, y + 6, 20, RAYWHITE);
            DrawText(TextFormat("TORMENTAS: %d", estado.golpes), x + 18, y + 28, 14, LIGHTGRAY);

            const char* textoFreno = "FRENO LISTO";
            Color colorFreno = LIME;

            if (estado.aterrizo)
            {
                textoFreno = estado.bonusCentro ? "ATERRIZO EN EL CENTRO +5" : "ATERRIZO";
                colorFreno = estado.bonusCentro ? GOLD : LIGHTGRAY;
            }
            else if (estado.aturdido > 0.0f)
            {
                textoFreno = "ATURDIDO";
                colorFreno = RED;
            }
            else if (estado.frenando > 0.0f)
            {
                textoFreno = "FRENANDO";
                colorFreno = SKYBLUE;
            }
            else if (estado.recargaFreno > 0.0f)
            {
                textoFreno = TextFormat("FRENO %.1f", estado.recargaFreno);
                colorFreno = ORANGE;
            }

            DrawText(textoFreno, x + 18, y + 46, 16, colorFreno);

            if (!participantes[i].esBot)
            {
                DrawText(
                    TextFormat("%s / %s", ObtenerTextoBotonPrincipal(participantes[i]), TextoGolpeNubes(participantes[i])),
                    x + anchoTarjeta - 100,
                    y + 48,
                    14,
                    LIGHTGRAY
                );
            }

            k++;
        }
    }

    if (fase == FASE_NUBES_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 130, 96, GOLD);

        const char* ayuda = "RECOGE ANILLOS (DORADOS x3), EVITA TORMENTAS Y ATERRIZA EN EL CENTRO";
        int anchoAyuda = MeasureText(ayuda, 20);
        DrawRectangle(anchoPantalla / 2 - anchoAyuda / 2 - 12, altoPantalla / 2 - 28, anchoAyuda + 24, 36, Fade(BLACK, 0.75f));
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 - 20, 20, RAYWHITE);
    }
    else if (
        fase == FASE_NUBES_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 40;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorNubes(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (!participa[i] || resultado.participantes[i].posicionFinal != posicion)
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d pts  (%d tormentas)",
                        posicion,
                        NombreJugadorNubes(participantes[i], i),
                        estados[i].puntos,
                        estados[i].golpes
                    ),
                    px + 30,
                    py + 62 + fila * 30,
                    20,
                    coloresJugadores[i]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            anchoPantalla / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoDescensoNubes::ObtenerResultado() const
{
    return resultado;
}
