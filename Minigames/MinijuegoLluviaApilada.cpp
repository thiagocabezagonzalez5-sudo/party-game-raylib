#include "Minigames/MinijuegoLluviaApilada.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <ctime>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_LLUVIA = 3.0f;
static const float DURACION_PARTIDA_LLUVIA = 40.0f;

// Zona jugable (cara interior de los muros) y limites del centro del jugador.
static const float MEDIO_X_ARENA_LLUVIA = 8.5f;
static const float MEDIO_Z_ARENA_LLUVIA = 5.5f;
static const float LIMITE_JUGADOR_X_LLUVIA = 8.1f;
static const float LIMITE_JUGADOR_Z_LLUVIA = 5.1f;

// Zona donde pueden nacer piezas (deja margen contra los muros).
static const float LIMITE_PIEZA_X_LLUVIA = 7.6f;
static const float LIMITE_PIEZA_Z_LLUVIA = 4.7f;

static const float ALTURA_CAIDA_LLUVIA = 11.0f;
static const float RADIO_BASE_LLUVIA = 1.45f;
static const float RADIO_SEGURO_BASE_LLUVIA = 2.5f;
static const float ALTURA_PIEZA_EN_PILA_LLUVIA = 0.26f;

static const float GRAVEDAD_PIEZA_SUELTA_LLUVIA = 12.5f;
static const float VIDA_PIEZA_SUELTA_LLUVIA = 8.0f;
static const float BLOQUEO_DUENIO_PIEZA_LLUVIA = 1.5f;
static const float RADIO_RECOGIDA_SUELTA_LLUVIA = 0.85f;

static const float DURACION_RALENTIZACION_PELIGRO_LLUVIA = 0.8f;

// Distribuciones de obstaculos de la terraza (el campo "tema" elige una).
static const int CANTIDAD_TEMAS_LLUVIA = 5;

// Una base por puesto: cada jugador asegura su pila en la suya.
static const Vector3 POSICIONES_BASE_LLUVIA[4] =
{
    { -7.0f, 0.0f, 3.6f },
    { 7.0f, 0.0f, 3.6f },
    { -7.0f, 0.0f, -3.6f },
    { 7.0f, 0.0f, -3.6f }
};


//==================================================
// UTILIDADES
//==================================================

static float LimitarLluvia(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static int LimiteJugadoresLluvia(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static bool ControladoPorBotLluvia(const Participante& participante)
{
    // Un humano desconectado se trata como bot para no dejar la ronda rota.
    return participante.esBot || !participante.conectado;
}


static Vector3 ObtenerBaseLluvia(int indiceJugador)
{
    return POSICIONES_BASE_LLUVIA[indiceJugador % 4];
}


static bool JugadorEnBaseLluvia(int indice, const JugadorPrueba& jugador)
{
    Vector3 base = ObtenerBaseLluvia(indice);
    float dx = jugador.posicion.x - base.x;
    float dz = jugador.posicion.z - base.z;
    return dx * dx + dz * dz <= RADIO_BASE_LLUVIA * RADIO_BASE_LLUVIA;
}


// Velocidad: 1.0 sin pila y baja hasta ~0.42 con la pila llena.
static float FactorVelocidadPila(int pila)
{
    return LimitarLluvia(1.0f - 0.055f * (float)pila, 0.42f, 1.0f);
}


// Respuesta del control: cuanto mas pila, mas tarda la direccion en cambiar.
static float TasaRespuestaPila(int pila)
{
    return 16.0f / (1.0f + 0.45f * (float)pila);
}


static float AlturaTopePila(const JugadorPrueba& jugador, int pila)
{
    return jugador.posicion.y + jugador.tamano.y * 0.5f +
        (float)pila * ALTURA_PIEZA_EN_PILA_LLUVIA + 0.2f;
}


static bool PuntoLibreParaPieza(
    const MinijuegoLluviaApilada& minijuego,
    float x,
    float z
)
{
    for (int i = 0; i < minijuego.cantidadObstaculos; i++)
    {
        const ObstaculoLluvia& obstaculo = minijuego.obstaculos[i];
        float mitadX = obstaculo.tamano.x * 0.5f + 0.9f;
        float mitadZ = obstaculo.tamano.z * 0.5f + 0.9f;

        if (
            std::fabs(x - obstaculo.posicion.x) < mitadX &&
            std::fabs(z - obstaculo.posicion.z) < mitadZ
        )
        {
            return false;
        }
    }

    // Las bases son zona segura: nunca cae nada sobre ellas.
    for (int i = 0; i < 4; i++)
    {
        float dx = x - POSICIONES_BASE_LLUVIA[i].x;
        float dz = z - POSICIONES_BASE_LLUVIA[i].z;

        if (dx * dx + dz * dz < RADIO_SEGURO_BASE_LLUVIA * RADIO_SEGURO_BASE_LLUVIA)
        {
            return false;
        }
    }

    return true;
}


//==================================================
// GAMEPLAY DEL MAPA: obstaculos, muros y suelo (colisiones)
//==================================================

static void AgregarObstaculoLluvia(
    MinijuegoLluviaApilada& minijuego,
    float x,
    float z,
    float ancho,
    float alto,
    float largo,
    bool redondo
)
{
    if (minijuego.cantidadObstaculos >= MAX_OBSTACULOS_LLUVIA) return;

    ObstaculoLluvia& obstaculo =
        minijuego.obstaculos[minijuego.cantidadObstaculos++];
    obstaculo.posicion = { x, alto * 0.5f, z };
    obstaculo.tamano = { ancho, alto, largo };
    obstaculo.redondo = redondo;
}


static void ConfigurarMapaLluvia(MinijuegoLluviaApilada& minijuego)
{
    minijuego.cantidadObstaculos = 0;
    minijuego.cantidadBloques = 0;

    // Todos los obstaculos miden >= 2.0 de alto: no se pueden escalar con
    // el salto (1.44) y obligan a rodearlos.
    switch (minijuego.tema)
    {
    case 0: // Cuatro columnas de piedra y el pedestal del globo celeste.
        AgregarObstaculoLluvia(minijuego, -2.8f, -2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 2.8f, -2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, -2.8f, 2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 2.8f, 2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 0.0f, 0.0f, 1.8f, 2.4f, 1.8f, false);
        break;
    case 1: // Consola, cajas de instrumental y tres columnas.
        AgregarObstaculoLluvia(minijuego, -3.2f, -1.8f, 2.4f, 2.2f, 2.0f, false);
        AgregarObstaculoLluvia(minijuego, 3.4f, 1.6f, 1.1f, 3.0f, 1.1f, true);
        AgregarObstaculoLluvia(minijuego, 0.4f, 2.6f, 2.0f, 2.0f, 1.6f, false);
        AgregarObstaculoLluvia(minijuego, 2.8f, -2.6f, 1.0f, 2.6f, 1.0f, true);
        AgregarObstaculoLluvia(minijuego, -3.0f, 2.0f, 1.0f, 2.6f, 1.0f, true);
        break;
    case 2: // Pedestal del telescopio, consolas y esferas armilares.
        AgregarObstaculoLluvia(minijuego, 0.0f, 0.0f, 2.4f, 2.6f, 2.4f, true);
        AgregarObstaculoLluvia(minijuego, -4.2f, 0.0f, 1.5f, 2.0f, 1.5f, false);
        AgregarObstaculoLluvia(minijuego, 4.2f, 0.0f, 1.5f, 2.0f, 1.5f, false);
        AgregarObstaculoLluvia(minijuego, 0.0f, -3.4f, 1.2f, 2.0f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 0.0f, 3.4f, 1.2f, 2.0f, 1.2f, true);
        break;
    case 3: // Cajas de instrumental apiladas y dos esferas armilares.
        AgregarObstaculoLluvia(minijuego, -3.0f, -2.0f, 2.0f, 2.0f, 2.0f, false);
        AgregarObstaculoLluvia(minijuego, -1.0f, -2.4f, 1.8f, 2.0f, 1.8f, false);
        AgregarObstaculoLluvia(minijuego, 3.2f, 1.6f, 1.4f, 2.0f, 1.4f, true);
        AgregarObstaculoLluvia(minijuego, 0.5f, 2.8f, 1.2f, 2.0f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 3.2f, -2.2f, 2.0f, 2.0f, 2.0f, false);
        break;
    default: // Mesa de cartas celestes y cuatro obeliscos.
        AgregarObstaculoLluvia(minijuego, -3.2f, -2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, 3.2f, -2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, -3.2f, 2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, 3.2f, 2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, 0.0f, 0.0f, 3.2f, 2.0f, 1.6f, false);
        break;
    }

    // Suelo (cara superior en y = 0).
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 0.0f, -0.45f, 0.0f }, { 18.0f, 0.90f, 12.0f }, Color{ 104, 110, 136, 255 });

    // Muros bajos de los cuatro lados (ademas hay un clamp de seguridad).
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { -8.75f, 0.5f, 0.0f }, { 0.5f, 1.0f, 12.0f }, Color{ 120, 126, 150, 255 });
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 8.75f, 0.5f, 0.0f }, { 0.5f, 1.0f, 12.0f }, Color{ 120, 126, 150, 255 });
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 0.0f, 0.5f, -5.75f }, { 18.0f, 1.0f, 0.5f }, Color{ 120, 126, 150, 255 });
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 0.0f, 0.5f, 5.75f }, { 18.0f, 1.0f, 0.5f }, Color{ 120, 126, 150, 255 });

    for (int i = 0; i < minijuego.cantidadObstaculos; i++)
    {
        const ObstaculoLluvia& obstaculo = minijuego.obstaculos[i];
        AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
            obstaculo.posicion, obstaculo.tamano, Color{ 140, 146, 170, 255 });
    }
}


//==================================================
// PILA, DEPOSITO Y PIEZAS SUELTAS
//==================================================

static int BuscarSlotPiezaSuelta(MinijuegoLluviaApilada& minijuego)
{
    int mejor = 0;
    float menorVida = 100000.0f;

    for (int i = 0; i < MAX_PIEZAS_SUELTAS_LLUVIA; i++)
    {
        if (!minijuego.piezasSueltas[i].activa) return i;

        if (minijuego.piezasSueltas[i].tiempoVida < menorVida)
        {
            menorVida = minijuego.piezasSueltas[i].tiempoVida;
            mejor = i;
        }
    }

    return mejor;
}


// Mismo enfoque que Nucleos: las piezas perdidas saltan al suelo y los
// demas pueden recogerlas; el duenio tiene un breve bloqueo.
static void SoltarPiezasLluvia(
    MinijuegoLluviaApilada& minijuego,
    int jugador,
    int cantidad,
    Vector3 origen
)
{
    EstadoJugadorLluvia& estado = minijuego.estadosJugadores[jugador];

    for (int k = 0; k < cantidad && estado.pila > 0; k++)
    {
        int tope = estado.pila - 1;
        bool rara = estado.rara[tope];
        estado.pila--;

        PiezaSueltaLluvia& pieza =
            minijuego.piezasSueltas[BuscarSlotPiezaSuelta(minijuego)];

        float angulo = (float)GetRandomValue(0, 6283) / 1000.0f + (float)k * 1.27f;
        float fuerza = (float)GetRandomValue(30, 48) / 10.0f;

        pieza = {};
        pieza.activa = true;
        pieza.rara = rara;
        pieza.jugadorBloqueado = jugador;
        pieza.tiempoBloqueo = BLOQUEO_DUENIO_PIEZA_LLUVIA;
        pieza.tiempoVida = VIDA_PIEZA_SUELTA_LLUVIA;
        pieza.posicion =
        {
            origen.x + std::cos(angulo) * 0.55f,
            origen.y + 0.9f,
            origen.z + std::sin(angulo) * 0.55f
        };
        pieza.velocidad =
        {
            std::cos(angulo) * fuerza,
            (float)GetRandomValue(50, 68) / 10.0f,
            std::sin(angulo) * fuerza
        };
    }
}


static void DepositarPilaLluvia(
    MinijuegoLluviaApilada& minijuego,
    int jugador,
    Vector3 posicion
)
{
    EstadoJugadorLluvia& estado = minijuego.estadosJugadores[jugador];

    for (int k = 0; k < estado.pila; k++)
    {
        estado.aseguradas += estado.rara[k] ? 3 : 1;
    }

    if (estado.pila > 0)
    {
        estado.pila = 0;
        estado.tiempoAvisoDeposito = 0.9f;
        CrearParticulasImpactoGolpe(
            minijuego.particulas,
            MAX_PARTICULAS_TIERRA,
            { posicion.x, posicion.y + 0.6f, posicion.z }
        );
    }
}


static void AnadirPiezaAPila(
    EstadoJugadorLluvia& estado,
    bool rara
)
{
    if (estado.pila >= MAX_PILA_LLUVIA) return;

    estado.rara[estado.pila] = rara;
    estado.pila++;
}


// Empuja un punto fuera de un obstaculo (para que ninguna pieza suelta
// quede inalcanzable dentro de una columna).
static void SacarPuntoDeObstaculos(
    const MinijuegoLluviaApilada& minijuego,
    Vector3& punto
)
{
    for (int i = 0; i < minijuego.cantidadObstaculos; i++)
    {
        const ObstaculoLluvia& obstaculo = minijuego.obstaculos[i];
        float mitadX = obstaculo.tamano.x * 0.5f + 0.3f;
        float mitadZ = obstaculo.tamano.z * 0.5f + 0.3f;
        float dx = punto.x - obstaculo.posicion.x;
        float dz = punto.z - obstaculo.posicion.z;

        if (std::fabs(dx) >= mitadX || std::fabs(dz) >= mitadZ) continue;

        if (mitadX - std::fabs(dx) < mitadZ - std::fabs(dz))
            punto.x = obstaculo.posicion.x + (dx >= 0.0f ? mitadX : -mitadX);
        else
            punto.z = obstaculo.posicion.z + (dz >= 0.0f ? mitadZ : -mitadZ);
    }
}


static void ActualizarPiezasSueltasLluvia(
    MinijuegoLluviaApilada& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[]
)
{
    for (int n = 0; n < MAX_PIEZAS_SUELTAS_LLUVIA; n++)
    {
        PiezaSueltaLluvia& pieza = minijuego.piezasSueltas[n];
        if (!pieza.activa) continue;

        pieza.tiempoVida -= deltaTime;
        pieza.tiempoBloqueo -= deltaTime;
        if (pieza.tiempoBloqueo < 0.0f) pieza.tiempoBloqueo = 0.0f;

        if (pieza.tiempoVida <= 0.0f)
        {
            pieza.activa = false;
            continue;
        }

        pieza.velocidad.y -= GRAVEDAD_PIEZA_SUELTA_LLUVIA * deltaTime;
        pieza.posicion.x += pieza.velocidad.x * deltaTime;
        pieza.posicion.y += pieza.velocidad.y * deltaTime;
        pieza.posicion.z += pieza.velocidad.z * deltaTime;

        if (pieza.posicion.y <= 0.3f)
        {
            pieza.posicion.y = 0.3f;

            if (pieza.velocidad.y < -1.4f)
                pieza.velocidad.y *= -0.28f;
            else
                pieza.velocidad.y = 0.0f;

            pieza.velocidad.x *= 0.93f;
            pieza.velocidad.z *= 0.93f;
        }

        if (std::fabs(pieza.posicion.x) > LIMITE_PIEZA_X_LLUVIA + 0.4f)
        {
            pieza.posicion.x = LimitarLluvia(
                pieza.posicion.x, -LIMITE_PIEZA_X_LLUVIA - 0.4f, LIMITE_PIEZA_X_LLUVIA + 0.4f
            );
            pieza.velocidad.x = -pieza.velocidad.x * 0.55f;
        }

        if (std::fabs(pieza.posicion.z) > LIMITE_PIEZA_Z_LLUVIA + 0.4f)
        {
            pieza.posicion.z = LimitarLluvia(
                pieza.posicion.z, -LIMITE_PIEZA_Z_LLUVIA - 0.4f, LIMITE_PIEZA_Z_LLUVIA + 0.4f
            );
            pieza.velocidad.z = -pieza.velocidad.z * 0.55f;
        }

        SacarPuntoDeObstaculos(minijuego, pieza.posicion);

        for (int i = 0; i < limite; i++)
        {
            if (
                !participantes[i].activo ||
                jugadores[i].cayendo ||
                minijuego.estadosJugadores[i].pila >= MAX_PILA_LLUVIA
            )
            {
                continue;
            }

            if (i == pieza.jugadorBloqueado)
            {
                bool sigueEnAire = pieza.posicion.y > 0.4f;
                if (pieza.tiempoBloqueo > 0.0f || sigueEnAire) continue;
            }

            float dx = jugadores[i].posicion.x - pieza.posicion.x;
            float dz = jugadores[i].posicion.z - pieza.posicion.z;
            float dy = std::fabs(jugadores[i].posicion.y - pieza.posicion.y);

            if (
                dx * dx + dz * dz > RADIO_RECOGIDA_SUELTA_LLUVIA * RADIO_RECOGIDA_SUELTA_LLUVIA ||
                dy > 1.6f
            )
            {
                continue;
            }

            AnadirPiezaAPila(minijuego.estadosJugadores[i], pieza.rara);
            pieza.activa = false;
            break;
        }
    }
}


//==================================================
// PIEZAS QUE CAEN
//==================================================

static void GenerarPiezaCayendoLluvia(MinijuegoLluviaApilada& minijuego)
{
    int slot = -1;

    for (int i = 0; i < MAX_PIEZAS_CAYENDO_LLUVIA; i++)
    {
        if (!minijuego.piezasCayendo[i].activa)
        {
            slot = i;
            break;
        }
    }

    if (slot < 0) return;

    float x = 0.0f;
    float z = 0.0f;
    bool encontrado = false;

    for (int intento = 0; intento < 24 && !encontrado; intento++)
    {
        x = (float)GetRandomValue(-(int)(LIMITE_PIEZA_X_LLUVIA * 100.0f), (int)(LIMITE_PIEZA_X_LLUVIA * 100.0f)) / 100.0f;
        z = (float)GetRandomValue(-(int)(LIMITE_PIEZA_Z_LLUVIA * 100.0f), (int)(LIMITE_PIEZA_Z_LLUVIA * 100.0f)) / 100.0f;
        encontrado = PuntoLibreParaPieza(minijuego, x, z);
    }

    if (!encontrado) return;

    int sorteo = GetRandomValue(1, 100);
    TipoPiezaLluvia tipo = PIEZA_LLUVIA_NORMAL;

    if (minijuego.tiempoJugado > 4.0f && sorteo <= 16)
        tipo = PIEZA_LLUVIA_PELIGROSA;
    else if (sorteo <= 28)
        tipo = PIEZA_LLUVIA_RARA;

    PiezaCayendoLluvia& pieza = minijuego.piezasCayendo[slot];
    pieza = {};
    pieza.activa = true;
    pieza.tipo = tipo;
    pieza.x = x;
    pieza.z = z;
    pieza.duracion = (float)GetRandomValue(150, 195) / 100.0f;
    pieza.altura = ALTURA_CAIDA_LLUVIA;
}


static void GolpeDePeligroLluvia(
    MinijuegoLluviaApilada& minijuego,
    int jugadorIndice,
    JugadorPrueba& jugador
)
{
    EstadoJugadorLluvia& estado = minijuego.estadosJugadores[jugadorIndice];

    // Pierdes la mitad de la pila (redondeando hacia arriba).
    SoltarPiezasLluvia(minijuego, jugadorIndice, (estado.pila + 1) / 2, jugador.posicion);

    jugador.tiempoRalentizado = DURACION_RALENTIZACION_PELIGRO_LLUVIA;
    jugador.multiplicadorRalentizacion = MULTIPLICADOR_VELOCIDAD_RALENTIZADO;
}


static void ActualizarPiezasCayendoLluvia(
    MinijuegoLluviaApilada& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[]
)
{
    for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
    {
        PiezaCayendoLluvia& pieza = minijuego.piezasCayendo[n];
        if (!pieza.activa) continue;

        pieza.tiempo += deltaTime;

        if (pieza.tiempo >= pieza.duracion)
        {
            // Aterriza sin ser atrapada: se rompe.
            CrearParticulasImpactoGolpe(
                minijuego.particulas,
                MAX_PARTICULAS_TIERRA,
                { pieza.x, 0.2f, pieza.z }
            );
            pieza.activa = false;
            continue;
        }

        float avance = pieza.tiempo / pieza.duracion;
        pieza.altura = ALTURA_CAIDA_LLUVIA * (1.0f - avance * avance);

        for (int i = 0; i < limite; i++)
        {
            if (!participantes[i].activo || jugadores[i].cayendo) continue;

            EstadoJugadorLluvia& estado = minijuego.estadosJugadores[i];

            if (
                pieza.tipo != PIEZA_LLUVIA_PELIGROSA &&
                estado.pila >= MAX_PILA_LLUVIA
            )
            {
                continue;
            }

            // Con mas pila el jugador "abarca" mas: atrapa mas, pero tambien
            // es un blanco mas facil para lo peligroso.
            float radio = 0.80f + 0.03f * (float)estado.pila;
            float dx = jugadores[i].posicion.x - pieza.x;
            float dz = jugadores[i].posicion.z - pieza.z;

            if (dx * dx + dz * dz > radio * radio) continue;
            if (pieza.altura > AlturaTopePila(jugadores[i], estado.pila)) continue;

            if (pieza.tipo == PIEZA_LLUVIA_PELIGROSA)
            {
                GolpeDePeligroLluvia(minijuego, i, jugadores[i]);
            }
            else
            {
                AnadirPiezaAPila(estado, pieza.tipo == PIEZA_LLUVIA_RARA);
            }

            CrearParticulasImpactoGolpe(
                minijuego.particulas,
                MAX_PARTICULAS_TIERRA,
                { pieza.x, jugadores[i].posicion.y + 0.9f, pieza.z }
            );
            pieza.activa = false;
            break;
        }
    }
}


//==================================================
// IA DE BOTS
//==================================================

static InputMinijuegoParticipante CrearEntradaBotLluvia(
    MinijuegoLluviaApilada& minijuego,
    int indice,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorLluvia& estado = minijuego.estadosJugadores[indice];
    const JugadorPrueba& jugador = jugadores[indice];
    Vector3 base = ObtenerBaseLluvia(indice);
    float velocidadEfectiva =
        VELOCIDAD_JUGADOR_ESTANDAR * FactorVelocidadPila(estado.pila);

    estado.tiempoDecision -= deltaTime;

    if (estado.tiempoDecision <= 0.0f)
    {
        estado.tiempoDecision = (float)GetRandomValue(22, 40) / 100.0f;
        estado.acosaRival = false;
        estado.ignoraPeligro = GetRandomValue(1, 100) <= 22;

        estado.quiereDepositar =
            estado.pila >= estado.umbralDeposito ||
            (minijuego.tiempoRestante < 9.0f && estado.pila > 0);

        float objetivoX = base.x * 0.55f;
        float objetivoZ = base.z * 0.5f;

        if (estado.quiereDepositar)
        {
            objetivoX = base.x;
            objetivoZ = base.z;
        }
        else
        {
            // 1) Acosar a un rival con pila grande si yo llevo poco.
            int rival = -1;
            float mejorRival = 49.0f;

            if (estado.pila <= 2)
            {
                for (int j = 0; j < limite; j++)
                {
                    if (
                        j == indice ||
                        !participantes[j].activo ||
                        minijuego.estadosJugadores[j].pila < 5
                    )
                    {
                        continue;
                    }

                    float dx = jugadores[j].posicion.x - jugador.posicion.x;
                    float dz = jugadores[j].posicion.z - jugador.posicion.z;
                    float d2 = dx * dx + dz * dz;

                    if (d2 < mejorRival)
                    {
                        mejorRival = d2;
                        rival = j;
                    }
                }
            }

            if (rival >= 0 && GetRandomValue(1, 100) <= 45)
            {
                estado.acosaRival = true;
                objetivoX = jugadores[rival].posicion.x;
                objetivoZ = jugadores[rival].posicion.z;
            }
            else
            {
                // 2) Sombra alcanzable mas cercana (o pieza suelta).
                float mejorPuntuacion = 100000.0f;

                for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
                {
                    const PiezaCayendoLluvia& pieza = minijuego.piezasCayendo[n];

                    if (!pieza.activa || pieza.tipo == PIEZA_LLUVIA_PELIGROSA) continue;

                    float restante = pieza.duracion - pieza.tiempo;
                    float dx = pieza.x - jugador.posicion.x;
                    float dz = pieza.z - jugador.posicion.z;
                    float distancia = std::sqrt(dx * dx + dz * dz);
                    float llegada = distancia / (velocidadEfectiva * 0.8f);

                    if (restante < 0.15f || llegada > restante + 0.3f) continue;

                    float puntuacion = distancia;
                    if (pieza.tipo == PIEZA_LLUVIA_RARA) puntuacion -= 2.0f;

                    if (puntuacion < mejorPuntuacion)
                    {
                        mejorPuntuacion = puntuacion;
                        objetivoX = pieza.x;
                        objetivoZ = pieza.z;
                    }
                }

                for (int n = 0; n < MAX_PIEZAS_SUELTAS_LLUVIA; n++)
                {
                    const PiezaSueltaLluvia& pieza = minijuego.piezasSueltas[n];

                    if (
                        !pieza.activa ||
                        (pieza.jugadorBloqueado == indice && pieza.tiempoBloqueo > 0.0f)
                    )
                    {
                        continue;
                    }

                    float dx = pieza.posicion.x - jugador.posicion.x;
                    float dz = pieza.posicion.z - jugador.posicion.z;
                    float puntuacion = std::sqrt(dx * dx + dz * dz) + 0.5f;

                    if (puntuacion < mejorPuntuacion)
                    {
                        mejorPuntuacion = puntuacion;
                        objetivoX = pieza.posicion.x;
                        objetivoZ = pieza.posicion.z;
                    }
                }
            }
        }

        // Error de punteria: los bots no clavan el punto exacto.
        estado.objetivoX = objetivoX + (float)GetRandomValue(-30, 30) / 100.0f;
        estado.objetivoZ = objetivoZ + (float)GetRandomValue(-30, 30) / 100.0f;
    }

    float dirX = estado.objetivoX - jugador.posicion.x;
    float dirZ = estado.objetivoZ - jugador.posicion.z;
    float longitud = std::sqrt(dirX * dirX + dirZ * dirZ);

    if (longitud > 0.25f)
    {
        dirX /= longitud;
        dirZ /= longitud;
    }
    else
    {
        dirX = 0.0f;
        dirZ = 0.0f;
    }

    // Esquivar un peligro que va a caer encima (salvo descuido).
    if (!estado.ignoraPeligro)
    {
        for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
        {
            const PiezaCayendoLluvia& pieza = minijuego.piezasCayendo[n];

            if (!pieza.activa || pieza.tipo != PIEZA_LLUVIA_PELIGROSA) continue;

            float restante = pieza.duracion - pieza.tiempo;
            float ax = jugador.posicion.x - pieza.x;
            float az = jugador.posicion.z - pieza.z;
            float distancia = std::sqrt(ax * ax + az * az);

            if (restante < 1.3f && distancia < 1.7f)
            {
                if (distancia < 0.05f)
                {
                    ax = 1.0f;
                    az = 0.0f;
                    distancia = 1.0f;
                }

                dirX = ax / distancia;
                dirZ = az / distancia;
                break;
            }
        }
    }

    // Rodear obstaculos que estorban el camino.
    for (int i = 0; i < minijuego.cantidadObstaculos; i++)
    {
        const ObstaculoLluvia& obstaculo = minijuego.obstaculos[i];
        float ox = obstaculo.posicion.x - jugador.posicion.x;
        float oz = obstaculo.posicion.z - jugador.posicion.z;
        float distancia = std::sqrt(ox * ox + oz * oz);
        float radio = (obstaculo.tamano.x > obstaculo.tamano.z
            ? obstaculo.tamano.x
            : obstaculo.tamano.z) * 0.5f + 0.9f;

        if (distancia < 0.01f || distancia > radio + 1.2f) continue;
        if ((ox * dirX + oz * dirZ) / distancia < 0.2f) continue;

        float tangenteX = -dirZ;
        float tangenteZ = dirX;

        if (tangenteX * ox + tangenteZ * oz > 0.0f)
        {
            tangenteX = -tangenteX;
            tangenteZ = -tangenteZ;
        }

        float peso = LimitarLluvia((radio + 1.2f - distancia) / 1.2f, 0.0f, 1.5f);
        dirX += tangenteX * peso;
        dirZ += tangenteZ * peso;

        float norma = std::sqrt(dirX * dirX + dirZ * dirZ);
        if (norma > 0.001f)
        {
            dirX /= norma;
            dirZ /= norma;
        }
    }

    entrada.izquierda = dirX < -0.35f;
    entrada.derecha = dirX > 0.35f;
    entrada.adelante = dirZ < -0.35f;
    entrada.atras = dirZ > 0.35f;

    if (estado.quiereDepositar && JugadorEnBaseLluvia(indice, jugador) && estado.pila > 0)
    {
        entrada.saltar = true;
    }

    if (estado.acosaRival)
    {
        float dx = estado.objetivoX - jugador.posicion.x;
        float dz = estado.objetivoZ - jugador.posicion.z;
        entrada.golpear = dx * dx + dz * dz < 1.4f * 1.4f;
    }

    return entrada;
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarLluvia(MinijuegoLluviaApilada& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO) return;

    // Solo cuentan las piezas ASEGURADAS: lo que sigue en la pila al
    // terminar el tiempo se pierde.
    int mejorPuntaje = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].aseguradas > mejorPuntaje
        )
        {
            mejorPuntaje = minijuego.estadosJugadores[i].aseguradas;
        }
    }

    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].aseguradas == mejorPuntaje
        )
        {
            cantidadGanadores++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.desenlace =
        cantidadGanadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.resultado.cantidadEquipos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo) continue;

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                minijuego.estadosJugadores[j].aseguradas >
                    minijuego.estadosJugadores[i].aseguradas
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            minijuego.estadosJugadores[i].aseguradas;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_LLUVIA_TERMINADO;
}


//==================================================
// INICIALIZAR / REINICIAR / ACTUALIZAR
//==================================================

static void ReiniciarVisualLluvia(MinijuegoLluviaApilada& m);
static void ActualizarVisualLluvia(
    MinijuegoLluviaApilada& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
);


void MinijuegoLluviaApilada::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        estadosJugadores[i].umbralDeposito = GetRandomValue(3, 6);
    }

    for (int i = 0; i < MAX_PIEZAS_CAYENDO_LLUVIA; i++) piezasCayendo[i] = {};
    for (int i = 0; i < MAX_PIEZAS_SUELTAS_LLUVIA; i++) piezasSueltas[i] = {};
    for (int i = 0; i < MAX_PARTICULAS_TIERRA; i++) particulas[i] = {};

    // Distribucion pseudoaleatoria segun los segundos del reloj del sistema.
    tema = (int)(std::time(nullptr) % CANTIDAD_TEMAS_LLUVIA);
    ConfigurarMapaLluvia(*this);

    // Observatorio: la terraza vista desde el frente y en alto, con la
    // cupula y el cielo asomando detras. Solo visual (el movimiento va por
    // ejes del mundo).
    camara.position = { 0.0f, 15.5f, 15.0f };
    camara.target = { 0.0f, 0.0f, -0.6f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_LLUVIA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_LLUVIA;
    tiempoRestante = DURACION_PARTIDA_LLUVIA;
    tiempoAnimacion = 0.0f;
    tiempoSiguientePieza = 0.0f;
    tiempoJugado = 0.0f;

    ReiniciarVisualLluvia(*this);
}


void MinijuegoLluviaApilada::Reiniciar(
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

    if (resultado.cantidadParticipantes < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_LLUVIA_TERMINADO;
        return;
    }

    int limite = LimiteJugadoresLluvia(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo) continue;

        Vector3 base = ObtenerBaseLluvia(i);
        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            { base.x, 0.7f, base.z }
        );
        // Mirando hacia el centro de la arena.
        jugadores[i].direccionMirada = { base.x < 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f };
    }
}


void MinijuegoLluviaApilada::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    tiempoAnimacion += deltaTime;
    ActualizarVisualLluvia(*this, deltaTime, jugadores, cantidadMaxima, participantes);
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA, deltaTime);

    if (
        fase == FASE_LLUVIA_TERMINADO ||
        resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int limite = LimiteJugadoresLluvia(cantidadMaxima);

    if (fase == FASE_LLUVIA_PREPARACION)
    {
        for (int i = 0; i < limite; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_LLUVIA_JUGANDO;
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);
    tiempoJugado += deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;

    int jugadoresActivos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo) jugadoresActivos++;
    }

    // 1) Movimiento: velocidad e inercia dependen del tamano de la pila.
    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo) continue;

        JugadorPrueba& jugador = jugadores[i];
        EstadoJugadorLluvia& estado = estadosJugadores[i];

        InputMinijuegoParticipante entrada{};

        if (ControladoPorBotLluvia(participantes[i]))
            entrada = CrearEntradaBotLluvia(*this, i, jugadores, limite, participantes, deltaTime);
        else
            entrada = LeerInputMinijuegoParticipante(participantes[i]);

        // Boton principal dentro de la propia base: asegura la pila.
        if (entrada.saltar && estado.pila > 0 && JugadorEnBaseLluvia(i, jugador))
        {
            DepositarPilaLluvia(*this, i, jugador.posicion);
            entrada.saltar = false;
        }

        float rawX = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
        float rawZ = (entrada.atras ? 1.0f : 0.0f) - (entrada.adelante ? 1.0f : 0.0f);
        float suavizado = LimitarLluvia(TasaRespuestaPila(estado.pila) * deltaTime, 0.0f, 1.0f);

        estado.direccionX += (rawX - estado.direccionX) * suavizado;
        estado.direccionZ += (rawZ - estado.direccionZ) * suavizado;

        InputMinijuegoParticipante entradaProcesada = entrada;
        entradaProcesada.izquierda = estado.direccionX < -0.3f;
        entradaProcesada.derecha = estado.direccionX > 0.3f;
        entradaProcesada.adelante = estado.direccionZ < -0.3f;
        entradaProcesada.atras = estado.direccionZ > 0.3f;

        jugador.velocidadMovimiento =
            VELOCIDAD_JUGADOR_ESTANDAR * FactorVelocidadPila(estado.pila);
        jugador.fuerzaSalto =
            FUERZA_SALTO_JUGADOR_ESTANDAR *
            LimitarLluvia(1.0f - 0.03f * (float)estado.pila, 0.6f, 1.0f);

        ActualizarJugadorPruebaNormal(
            jugador,
            entradaProcesada,
            bloques,
            cantidadBloques,
            particulas,
            MAX_PARTICULAS_TIERRA,
            true,
            true,
            deltaTime
        );

        jugador.posicion.x = LimitarLluvia(
            jugador.posicion.x, -LIMITE_JUGADOR_X_LLUVIA, LIMITE_JUGADOR_X_LLUVIA
        );
        jugador.posicion.z = LimitarLluvia(
            jugador.posicion.z, -LIMITE_JUGADOR_Z_LLUVIA, LIMITE_JUGADOR_Z_LLUVIA
        );

        if (estado.tiempoAvisoDeposito > 0.0f) estado.tiempoAvisoDeposito -= deltaTime;
    }

    // 2) Golpes entre jugadores: hacen caer piezas de la pila del rival.
    float ralentizacionAntes[MAX_PARTICIPANTES]{};

    for (int i = 0; i < limite; i++)
    {
        ralentizacionAntes[i] = jugadores[i].tiempoRalentizado;
    }

    // Un humano desconectado lo controla la IA, pero debe seguir pudiendo
    // golpear y ser golpeado: para la utilidad compartida cuenta como conectado.
    Participante efectivos[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectivos[i] = participantes[i];
        if (efectivos[i].activo) efectivos[i].conectado = true;
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        efectivos,
        limite,
        particulas,
        MAX_PARTICULAS_TIERRA
    );

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo) continue;

        if (jugadores[i].golpeSueloRecibido)
        {
            SoltarPiezasLluvia(*this, i, 5, jugadores[i].posicion);
        }
        else if (jugadores[i].tiempoRalentizado > ralentizacionAntes[i] + 0.20f)
        {
            SoltarPiezasLluvia(*this, i, 3, jugadores[i].posicion);
        }
    }

    // 3) Lluvia de piezas: mas jugadores, mas ritmo.
    tiempoSiguientePieza -= deltaTime;
    int generadasEsteFrame = 0;

    while (tiempoSiguientePieza <= 0.0f && generadasEsteFrame < 2)
    {
        GenerarPiezaCayendoLluvia(*this);
        generadasEsteFrame++;

        float base = 1.6f / (float)(jugadoresActivos < 2 ? 2 : jugadoresActivos);
        tiempoSiguientePieza += base * (0.75f + (float)GetRandomValue(0, 50) / 100.0f);
    }

    ActualizarPiezasCayendoLluvia(*this, deltaTime, jugadores, limite, participantes);
    ActualizarPiezasSueltasLluvia(*this, deltaTime, jugadores, limite, participantes);

    if (tiempoRestante <= 0.0f)
    {
        FinalizarLluvia(*this);
    }
}



//==================================================
// VISUAL: CIMA DEL OBSERVATORIO DE NOCHE
//==================================================
//
// La terraza (arena) es la azotea de piedra del observatorio, sobre la cima
// nevada de una montania. Atras, la cupula grande con el telescopio asomando
// por la ranura y una antena; a los lados, telescopios en tripode, esfera
// armilar, farolas, pinos y rocas; abajo, un mar de nubes y dos cordilleras;
// arriba, la Via Lactea, la luna y estrellas fugaces. Lo que cae son
// fragmentos de estrella (cian), estrellas doradas (valen 3) y meteoros.

static const float Y_LLANO_LLU = -1.2f;          // cima alrededor de la terraza
static const float BORDE_X_LLU = 9.0f;           // cara exterior de los muros
static const float BORDE_Z_LLU = 6.0f;

static const Vector3 OJO_LLU = { 0.0f, 15.5f, 15.0f };
static const Vector3 CUPULA_LLU = { 10.6f, Y_LLANO_LLU, -11.8f };
static const float RADIO_CUPULA_LLU = 4.1f;
static const float ALTO_TAMBOR_LLU = 2.4f;
static const Vector3 ANTENA_LLU = { -12.2f, Y_LLANO_LLU, -10.6f };

static const Color COLOR_NIEBLA_LLU = { 40, 44, 88, 255 };
static const Color COLOR_NUBE_LLU = { 86, 86, 146, 255 };
static const Color LOSA_LLU = { 112, 118, 146, 255 };
static const Color JUNTA_LLU = { 62, 66, 92, 255 };
static const Color PIEDRA_LLU = { 128, 132, 158, 255 };
static const Color PIEDRA_CLARA_LLU = { 164, 168, 192, 255 };
static const Color PIEDRA_OSCURA_LLU = { 84, 88, 114, 255 };
static const Color ROCA_LLU = { 80, 84, 108, 255 };
static const Color NIEVE_LLU = { 196, 206, 232, 255 };
static const Color NIEVE_SUELO_LLU = { 160, 170, 204, 255 };
static const Color BRONCE_LLU = { 204, 160, 84, 255 };
static const Color BRONCE_OSCURO_LLU = { 132, 98, 56, 255 };
static const Color MADERA_LLU = { 122, 88, 64, 255 };
static const Color CUPULA_COLOR_LLU = { 196, 204, 226, 255 };
static const Color PINO_LLU = { 38, 66, 70, 255 };
static const Color LUZ_CALIDA_LLU = { 255, 196, 120, 255 };

static const Color FRAGMENTO_LLU = { 120, 232, 255, 255 };
static const Color DORADA_LLU = { 255, 214, 80, 255 };
static const Color METEORO_LLU = { 255, 108, 48, 255 };


//==================================================
// VISUAL: UTILIDADES
//==================================================

static float Limitar01Llu(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalLlu(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoLlu(Color c, float f)
{
    return Color{ CanalLlu((float)c.r * f), CanalLlu((float)c.g * f), CanalLlu((float)c.b * f), c.a };
}


static Color MezclaLlu(Color a, Color b, float t)
{
    t = Limitar01Llu(t);
    return Color{
        CanalLlu((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalLlu((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalLlu((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalLlu((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaLlu(Color c, float alfa)
{
    c.a = CanalLlu(alfa);
    return c;
}


static float SuaveLlu(float u)
{
    u = Limitar01Llu(u);
    return u * u * (3.0f - 2.0f * u);
}


// Hash determinista en [0, 1) para decorar sin parpadeos.
static float Hash01Llu(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 2654435761u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFFu) / 16777216.0f;
}


// Ruido de valor suave (interpolado) para el relieve de la cima.
static float RuidoLlu(float x, float z, int capa)
{
    float fx = std::floor(x);
    float fz = std::floor(z);
    int ix = (int)fx;
    int iz = (int)fz;
    float u = SuaveLlu(x - fx);
    float v = SuaveLlu(z - fz);
    float a = Hash01Llu(ix + capa * 131, iz);
    float b = Hash01Llu(ix + 1 + capa * 131, iz);
    float c = Hash01Llu(ix + capa * 131, iz + 1);
    float d = Hash01Llu(ix + 1 + capa * 131, iz + 1);
    return (a + (b - a) * u) + ((c + (d - c) * u) - (a + (b - a) * u)) * v;
}


// Azar propio del dibujo: nunca consume el GetRandomValue de la logica.
static float AzarLlu(MinijuegoLluviaApilada& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFFFu) / 16777216.0f;
    return minimo + (maximo - minimo) * u;
}


static float DistanciaOjoLlu(Vector3 p)
{
    float dx = p.x - OJO_LLU.x;
    float dy = p.y - OJO_LLU.y;
    float dz = p.z - OJO_LLU.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}


// Noche: lo lejano se hunde en azul y lo que baja del precipicio se funde
// con el mar de nubes.
static Color TinteLlu(Color c, Vector3 p)
{
    float n = (DistanciaOjoLlu(p) - 22.0f) / 36.0f;
    Color niebla = COLOR_NIEBLA_LLU;
    niebla.a = c.a;
    Color r = MezclaLlu(c, niebla, n < 0.0f ? 0.0f : (n > 0.45f ? 0.45f : n));

    if (p.y < Y_LLANO_LLU - 1.5f)
    {
        Color nube = COLOR_NUBE_LLU;
        nube.a = c.a;
        r = MezclaLlu(r, nube, (Y_LLANO_LLU - 1.5f - p.y) / 16.0f);
    }

    return r;
}


// Luz falsa por normal: la luna arriba a la izquierda y algo de frente.
static float LuzNormalLlu(Vector3 n)
{
    float d = n.x * -0.36f + n.y * 0.82f + n.z * 0.44f;
    float f = 0.64f + 0.36f * d;
    return f < 0.46f ? 0.46f : f;
}


static float LuzLateralLlu(float angulo)
{
    return LuzNormalLlu({ std::cos(angulo), 0.0f, std::sin(angulo) });
}


static void VerticeLlu(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadLlu(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeLlu(a, color);
    VerticeLlu(b, color);
    VerticeLlu(c, color);
    VerticeLlu(d, color);
}


// Rectangulo horizontal (centro, medios lados) a la altura c.y.
static void LosaLlu(Vector3 c, float mx, float mz, Color color)
{
    rlBegin(RL_QUADS);
    QuadLlu(
        { c.x - mx, c.y, c.z - mz },
        { c.x - mx, c.y, c.z + mz },
        { c.x + mx, c.y, c.z + mz },
        { c.x + mx, c.y, c.z - mz },
        color
    );
    rlEnd();
}


// Caja sobre tres ejes (medios lados ya escalados), caras sombreadas.
static void CajaEjesLlu(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color)
{
    Vector3 v[8];

    for (int k = 0; k < 8; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sy = (k & 2) ? 1.0f : -1.0f;
        float sz = (k & 4) ? 1.0f : -1.0f;
        v[k] =
        {
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    }

    rlBegin(RL_QUADS);
    QuadLlu(v[6], v[7], v[3], v[2], TonoLlu(color, 1.0f));     // arriba
    QuadLlu(v[4], v[5], v[7], v[6], TonoLlu(color, 0.78f));    // frente
    QuadLlu(v[1], v[0], v[2], v[3], TonoLlu(color, 0.56f));    // fondo
    QuadLlu(v[5], v[1], v[3], v[7], TonoLlu(color, 0.62f));    // derecha
    QuadLlu(v[0], v[4], v[6], v[2], TonoLlu(color, 0.70f));    // izquierda
    QuadLlu(v[0], v[1], v[5], v[4], TonoLlu(color, 0.46f));    // abajo
    rlEnd();
}


static void CuboLlu(Vector3 c, Vector3 t, Color color)
{
    CajaEjesLlu(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color);
}


static void CuboGiradoLlu(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    float a = giroGrados * DEG2RAD;
    float co = std::cos(a);
    float si = std::sin(a);

    CajaEjesLlu(
        c,
        { co * t.x * 0.5f, 0.0f, -si * t.x * 0.5f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { si * t.z * 0.5f, 0.0f, co * t.z * 0.5f },
        color
    );
}


// Base ortonormal alrededor de un eje (para tubos y aros orientados).
static void BaseEjeLlu(Vector3 dir, Vector3& ex, Vector3& ey)
{
    Vector3 ayuda = std::fabs(dir.y) > 0.9f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    ex = Vector3Normalize(Vector3CrossProduct(ayuda, dir));
    ey = Vector3CrossProduct(dir, ex);
}


// Tubo (tronco de cono) entre dos puntos con caras sombreadas por normal.
static void TuboLlu(Vector3 a, Vector3 b, float r0, float r1, Color color, int lados = 10, bool tapa = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 ex, ey;
    BaseEjeLlu(dir, ex, ey);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        float am = (a0 + a1) * 0.5f;
        Vector3 n0 = Vector3Add(Vector3Scale(ex, std::cos(a0)), Vector3Scale(ey, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(ex, std::cos(a1)), Vector3Scale(ey, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(ex, std::cos(am)), Vector3Scale(ey, std::sin(am)));
        Color cara = TonoLlu(color, LuzNormalLlu(nm));

        QuadLlu(
            Vector3Add(a, Vector3Scale(n0, r0)),
            Vector3Add(b, Vector3Scale(n0, r1)),
            Vector3Add(b, Vector3Scale(n1, r1)),
            Vector3Add(a, Vector3Scale(n1, r0)),
            cara
        );

        if (tapa)
        {
            Color c = TonoLlu(color, LuzNormalLlu(dir));
            QuadLlu(b, Vector3Add(b, Vector3Scale(n1, r1)), Vector3Add(b, Vector3Scale(n0, r1)), b, c);
        }
    }

    rlEnd();
}


static void VigaLlu(Vector3 a, Vector3 b, float grosor, Color color)
{
    TuboLlu(a, b, grosor * 0.5f, grosor * 0.5f, color, 4, false);
}


// Tronco de cono vertical (r0 abajo, r1 arriba) con tapa arriba.
static void PrismaLlu(Vector3 base, float r0, float r1, float alto, int lados, Color color, bool tapa = true)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = TonoLlu(color, 1.0f);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        Color cara = TonoLlu(color, LuzLateralLlu(a0 + paso * 0.5f));
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadLlu(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            cara
        );

        if (tapa && r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadLlu(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }
    }

    rlEnd();
}


// Columna acanalada: las caras alternan luz para marcar las estrias.
static void EstriadoLlu(Vector3 base, float r0, float r1, float alto, int lados, Color color)
{
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float estria = (k % 2 == 0) ? 1.0f : 0.86f;
        Color cara = TonoLlu(color, LuzLateralLlu(a0 + paso * 0.5f) * estria);
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadLlu(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c0 * r1, base.y + alto, base.z + s0 * r1 },
            { base.x + c1 * r1, base.y + alto, base.z + s1 * r1 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            cara
        );
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloLlu(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadLlu(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Aro con grosor en un plano cualquiera (u, v): esfera armilar, antena.
static void AroLlu(Vector3 c, Vector3 u, Vector3 v, float radio, float grosor, Color color, int lados = 24)
{
    Vector3 n = Vector3Normalize(Vector3CrossProduct(u, v));

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        Vector3 d0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 d1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Color cara = TonoLlu(color, LuzNormalLlu(Vector3Normalize(Vector3Add(d0, d1))));
        Vector3 h = Vector3Scale(n, grosor * 0.5f);

        Vector3 p0 = Vector3Add(c, Vector3Scale(d0, radio));
        Vector3 p1 = Vector3Add(c, Vector3Scale(d1, radio));
        QuadLlu(Vector3Subtract(p0, h), Vector3Add(p0, h), Vector3Add(p1, h), Vector3Subtract(p1, h), cara);

        Vector3 q0 = Vector3Add(c, Vector3Scale(d0, radio - grosor));
        Vector3 q1 = Vector3Add(c, Vector3Scale(d1, radio - grosor));
        Color plano = TonoLlu(color, LuzNormalLlu(n));
        QuadLlu(Vector3Add(q0, h), Vector3Add(p0, h), Vector3Add(p1, h), Vector3Add(q1, h), plano);
        QuadLlu(Vector3Subtract(q0, h), Vector3Subtract(p0, h), Vector3Subtract(p1, h), Vector3Subtract(q1, h), TonoLlu(plano, 0.8f));
    }

    rlEnd();
}


// Casquete esferico sobre c (cupulas, globos, faroles). angulo0/angulo1
// recortan una franja de azimut (la ranura de la cupula); si son iguales
// no hay ranura.
static void DomoLlu(Vector3 c, float rx, float ry, float rz, Color color, int lados = 16, int anillos = 5, float ranura0 = 0.0f, float ranura1 = 0.0f, bool gajos = false)
{
    rlBegin(RL_QUADS);

    for (int a = 0; a < anillos; a++)
    {
        float f0 = 1.5708f * (float)a / (float)anillos;
        float f1 = 1.5708f * (float)(a + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float t0 = 6.2832f * (float)k / (float)lados;
            float t1 = 6.2832f * (float)(k + 1) / (float)lados;
            float fm = (f0 + f1) * 0.5f;
            float tm = (t0 + t1) * 0.5f;

            if (ranura1 > ranura0)
            {
                float d = std::fmod(tm - ranura0 + 12.5664f, 6.2832f);
                if (d < ranura1 - ranura0 && a < anillos - 1) continue;
            }

            float gajo = (gajos && (k % 2 == 1)) ? 0.93f : 1.0f;
            Color cara = TonoLlu(color, LuzNormalLlu({ std::cos(fm) * std::cos(tm), std::sin(fm), std::cos(fm) * std::sin(tm) }) * gajo);

            QuadLlu(
                { c.x + rx * std::cos(f0) * std::cos(t0), c.y + ry * std::sin(f0), c.z + rz * std::cos(f0) * std::sin(t0) },
                { c.x + rx * std::cos(f1) * std::cos(t0), c.y + ry * std::sin(f1), c.z + rz * std::cos(f1) * std::sin(t0) },
                { c.x + rx * std::cos(f1) * std::cos(t1), c.y + ry * std::sin(f1), c.z + rz * std::cos(f1) * std::sin(t1) },
                { c.x + rx * std::cos(f0) * std::cos(t1), c.y + ry * std::sin(f0), c.z + rz * std::cos(f0) * std::sin(t1) },
                cara
            );
        }
    }

    rlEnd();
}


static void EsferaLlu(Vector3 c, float r, Color color, int lados = 12, int anillos = 4)
{
    DomoLlu(c, r, r, r, color, lados, anillos);
    DomoLlu(c, r, -r, r, TonoLlu(color, 0.82f), lados, anillos);
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorLlu(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloLlu(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorLlu(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (la vista baja a ~45 grados).
static void ResplandorFrenteLlu(Vector3 c, float radio, Color color)
{
    ResplandorLlu(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.70f, 0.71f }, radio, color);
}


static int CantidadParticulasLlu(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


//==================================================
// VISUAL: ESTADO Y EFECTOS (una vez por frame desde Actualizar)
//==================================================

static void EmitirParticulaLlu(
    MinijuegoLluviaApilada& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool brillo
)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_LLUVIA; i++)
    {
        ParticulaVisualLluvia& p = m.particulasVisuales[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.color = color;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.giro = AzarLlu(m, 0.0f, 180.0f);
        return;
    }
}


// Rocio de particulas en todas direcciones (hacia arriba).
static void EstallidoLlu(
    MinijuegoLluviaApilada& m,
    Vector3 centro,
    int cantidad,
    Color color,
    float fuerza,
    float vida,
    float tamano,
    float gravedad,
    bool brillo
)
{
    int n = CantidadParticulasLlu(cantidad);

    for (int k = 0; k < n; k++)
    {
        float a = AzarLlu(m, 0.0f, 6.2832f);
        float f = AzarLlu(m, 0.4f, 1.0f) * fuerza;
        Vector3 v = { std::cos(a) * f, AzarLlu(m, 0.6f, 1.4f) * fuerza, std::sin(a) * f };
        EmitirParticulaLlu(m, centro, v, color, vida * AzarLlu(m, 0.7f, 1.2f), tamano * AzarLlu(m, 0.7f, 1.3f), gravedad, brillo);
    }
}


static void ReiniciarVisualLluvia(MinijuegoLluviaApilada& m)
{
    m.semillaVisual = 2654435761u;

    for (int i = 0; i < MAX_PIEZAS_CAYENDO_LLUVIA; i++)
    {
        m.caidaVista[i] = false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.pilaVista[i] = 0;
        m.aseguradasVista[i] = 0;
        m.destelloBase[i] = 0.0f;
        m.sumaBase[i] = 0;
        m.golpePila[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS_VISUALES_LLUVIA; i++)
    {
        m.particulasVisuales[i] = {};
    }

    for (int i = 0; i < MAX_MARCAS_LLUVIA; i++)
    {
        m.marcas[i] = {};
    }

    m.esperaSonidoImpacto = 0.0f;
    m.tiempoFin = 0.0f;
    m.finVisto = false;
}


static int JugadorCercanoLlu(
    const MinijuegoLluviaApilada& m,
    const JugadorPrueba jugadores[],
    int limite,
    float x,
    float z
)
{
    int mejor = -1;
    float menor = 4.0f;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        float dx = jugadores[i].posicion.x - x;
        float dz = jugadores[i].posicion.z - z;
        float d = dx * dx + dz * dz;

        if (d < menor)
        {
            menor = d;
            mejor = i;
        }
    }

    return mejor;
}


static float AlturaPilaVisualLlu(const JugadorPrueba& jugador, int pila)
{
    return jugador.posicion.y + jugador.tamano.y * 0.5f + 0.25f + (float)pila * ALTURA_PIEZA_EN_PILA_LLUVIA;
}


static void ActualizarVisualLluvia(
    MinijuegoLluviaApilada& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
)
{
    int limite = LimiteJugadoresLluvia(cantidadMaxima);

    // Particulas: caen, se posan en las losas y se apagan.
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_LLUVIA; i++)
    {
        ParticulaVisualLluvia& p = m.particulasVisuales[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, deltaTime));
        p.giro += deltaTime * 240.0f;

        bool sobreTerraza =
            std::fabs(p.posicion.x) < MEDIO_X_ARENA_LLUVIA &&
            std::fabs(p.posicion.z) < MEDIO_Z_ARENA_LLUVIA;

        if (sobreTerraza && p.posicion.y < 0.04f && p.velocidad.y < 0.0f)
        {
            p.posicion.y = 0.04f;
            p.velocidad = { p.velocidad.x * 0.4f, 0.0f, p.velocidad.z * 0.4f };
            p.giro -= deltaTime * 240.0f;
        }
    }

    for (int i = 0; i < MAX_MARCAS_LLUVIA; i++)
    {
        if (m.marcas[i].vida > 0.0f)
        {
            m.marcas[i].vida -= deltaTime * 0.22f;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.destelloBase[i] -= deltaTime * 0.8f;
        if (m.destelloBase[i] < 0.0f) m.destelloBase[i] = 0.0f;
        m.golpePila[i] -= deltaTime * 1.6f;
        if (m.golpePila[i] < 0.0f) m.golpePila[i] = 0.0f;
    }

    if (m.esperaSonidoImpacto > 0.0f)
    {
        m.esperaSonidoImpacto -= deltaTime;
    }

    // Piezas que dejaron de caer este frame: rotas contra las losas o
    // atrapadas por alguien.
    bool golpeadoMeteoro[MAX_PARTICIPANTES]{};

    for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
    {
        const PiezaCayendoLluvia& pieza = m.piezasCayendo[n];

        if (m.caidaVista[n] && !pieza.activa)
        {
            Vector3 suelo = { pieza.x, 0.12f, pieza.z };

            if (pieza.tiempo >= pieza.duracion)
            {
                if (pieza.tipo == PIEZA_LLUVIA_PELIGROSA)
                {
                    for (int k = 0; k < MAX_MARCAS_LLUVIA; k++)
                    {
                        if (m.marcas[k].vida <= 0.0f)
                        {
                            m.marcas[k].x = pieza.x;
                            m.marcas[k].z = pieza.z;
                            m.marcas[k].vida = 1.0f;
                            m.marcas[k].giro = AzarLlu(m, 0.0f, 6.2832f);
                            break;
                        }
                    }

                    EstallidoLlu(m, suelo, 12, METEORO_LLU, 3.2f, 0.7f, 0.10f, 9.0f, true);
                    EstallidoLlu(m, suelo, 6, Color{ 70, 56, 52, 255 }, 2.6f, 0.9f, 0.10f, 12.0f, false);

                    if (m.esperaSonidoImpacto <= 0.0f)
                    {
                        ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
                        m.esperaSonidoImpacto = 0.3f;
                    }
                }
                else
                {
                    Color esquirla = pieza.tipo == PIEZA_LLUVIA_RARA ? DORADA_LLU : FRAGMENTO_LLU;
                    EstallidoLlu(m, suelo, 8, esquirla, 2.2f, 0.8f, 0.08f, 11.0f, false);
                    EstallidoLlu(m, suelo, 4, esquirla, 1.4f, 0.4f, 0.12f, 0.0f, true);
                }
            }
            else
            {
                int j = JugadorCercanoLlu(m, jugadores, limite, pieza.x, pieza.z);

                if (j >= 0)
                {
                    Vector3 tope = { jugadores[j].posicion.x, AlturaPilaVisualLlu(jugadores[j], m.estadosJugadores[j].pila), jugadores[j].posicion.z };

                    if (pieza.tipo == PIEZA_LLUVIA_PELIGROSA)
                    {
                        golpeadoMeteoro[j] = true;
                        m.golpePila[j] = 1.0f;
                        EstallidoLlu(m, tope, 16, METEORO_LLU, 3.4f, 0.6f, 0.12f, 6.0f, true);
                        EstallidoLlu(m, tope, 6, Color{ 90, 90, 100, 255 }, 2.0f, 0.9f, 0.12f, 9.0f, false);
                        ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
                        ActivarTemblorCamaraGeneral(0.12f, 0.22f);
                    }
                    else
                    {
                        Color brillo = pieza.tipo == PIEZA_LLUVIA_RARA ? DORADA_LLU : FRAGMENTO_LLU;
                        EstallidoLlu(m, tope, 7, brillo, 1.8f, 0.45f, 0.10f, 2.0f, true);
                    }
                }
            }
        }

        m.caidaVista[n] = pieza.activa;
    }

    // Cambios de pila: atrapar, asegurar y perder piezas.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorLluvia& estado = m.estadosJugadores[i];
        Color color = participantes[i].color;

        if (estado.aseguradas > m.aseguradasVista[i])
        {
            Vector3 base = ObtenerBaseLluvia(i);
            m.sumaBase[i] = estado.aseguradas - m.aseguradasVista[i];
            m.destelloBase[i] = 1.0f;
            ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);

            int n = CantidadParticulasLlu(22);

            for (int k = 0; k < n; k++)
            {
                float a = AzarLlu(m, 0.0f, 6.2832f);
                float r = AzarLlu(m, 0.2f, RADIO_BASE_LLUVIA);
                Vector3 p = { base.x + std::cos(a) * r, 0.1f, base.z + std::sin(a) * r };
                Color c = (k % 3 == 0) ? DORADA_LLU : MezclaLlu(color, WHITE, 0.25f);
                EmitirParticulaLlu(m, p, { 0.0f, AzarLlu(m, 2.0f, 4.4f), 0.0f }, c, AzarLlu(m, 0.7f, 1.2f), 0.10f, 0.0f, true);
            }
        }
        else if (estado.pila > m.pilaVista[i])
        {
            bool rara = estado.pila > 0 && estado.rara[estado.pila - 1];
            ReproducirSonidoMinijuego(m.audio, rara ? SONIDO_RECOGER_NUCLEO_ESPECIAL : SONIDO_RECOGER_NUCLEO);
        }
        else if (estado.pila < m.pilaVista[i])
        {
            m.golpePila[i] = 1.0f;

            if (!golpeadoMeteoro[i])
            {
                ReproducirSonidoMinijuego(m.audio, SONIDO_GOLPE);
            }
        }

        m.pilaVista[i] = estado.pila;
        m.aseguradasVista[i] = estado.aseguradas;
    }

    // Fin: confeti y polvo de estrellas sobre quien gano.
    if (m.fase == FASE_LLUVIA_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (!m.finVisto)
        {
            m.finVisto = true;
            m.tiempoFin = 0.0f;
        }

        m.tiempoFin += deltaTime;

        if (m.tiempoFin < 3.0f)
        {
            const Color confeti[5] =
            {
                DORADA_LLU, FRAGMENTO_LLU, Color{ 255, 250, 240, 255 },
                Color{ 255, 150, 200, 255 }, Color{ 190, 160, 255, 255 }
            };

            for (int i = 0; i < limite; i++)
            {
                if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1)
                {
                    continue;
                }

                int n = CalidadParticulas() == CALIDAD_BAJA ? 1 : 2;

                for (int k = 0; k < n; k++)
                {
                    Vector3 p =
                    {
                        jugadores[i].posicion.x + AzarLlu(m, -0.9f, 0.9f),
                        jugadores[i].posicion.y + 2.6f + AzarLlu(m, 0.0f, 0.8f),
                        jugadores[i].posicion.z + AzarLlu(m, -0.9f, 0.9f)
                    };
                    int c = (int)AzarLlu(m, 0.0f, 4.99f);
                    Color color = (k == 1) ? participantes[i].color : confeti[c];
                    EmitirParticulaLlu(m, p, { AzarLlu(m, -1.2f, 1.2f), AzarLlu(m, 1.0f, 3.0f), AzarLlu(m, -1.2f, 1.2f) }, color, AzarLlu(m, 1.4f, 2.2f), 0.09f, 3.2f, false);
                }
            }
        }
    }
}


//==================================================
// VISUAL: CIELO, CORDILLERAS Y MAR DE NUBES (2D, antes del 3D)
//==================================================

// Altura del horizonte en pantalla (sube cuando la camara de entrada esta alta).
static float HorizonteLlu(float h, float entrada)
{
    return h * (0.27f - 0.10f * entrada);
}


// Silueta de una cordillera: picos triangulares con hombros suaves.
static float CimaCordilleraLlu(float x, float w, float h, int capa, int picos, float altoMin, float altoMax)
{
    float y = 0.0f;

    for (int p = 0; p < picos; p++)
    {
        float centro = ((float)p + Hash01Llu(p, capa * 7 + 1)) * w / (float)(picos - 2) - w * 0.12f;
        float medio = (0.07f + 0.07f * Hash01Llu(p, capa * 7 + 2)) * w;
        float alto = (altoMin + (altoMax - altoMin) * Hash01Llu(p, capa * 7 + 3)) * h;
        float u = 1.0f - std::fabs(x - centro) / medio;

        if (u > 0.0f)
        {
            float forma = u * (0.65f + 0.35f * u);
            float v = alto * forma;
            if (v > y) y = v;
        }
    }

    // Lomos y aristas menores.
    y += h * 0.006f * std::sin(x * 0.045f + (float)capa) + h * 0.004f * std::sin(x * 0.13f + (float)capa * 2.0f);
    return y;
}


static void DibujarCordilleraLlu(float w, float h, float yH, int capa, int picos, float altoMin, float altoMax, Color roca, Color nieve, Color base, float desplazar)
{
    const float PASO = 4.0f;
    float lineaNieve = yH - (altoMin + (altoMax - altoMin) * 0.45f) * h;

    for (float x = 0.0f; x < w; x += PASO)
    {
        float xm = x + PASO * 0.5f + desplazar;
        float alto = CimaCordilleraLlu(xm, w, h, capa, picos, altoMin, altoMax);
        float yTope = yH - alto;
        float pendiente = (CimaCordilleraLlu(xm + PASO, w, h, capa, picos, altoMin, altoMax) - CimaCordilleraLlu(xm - PASO, w, h, capa, picos, altoMin, altoMax)) / (2.0f * PASO);

        // La luna esta a la izquierda: las laderas que la miran se aclaran.
        float luz = 1.0f + (pendiente < 0.0f ? -0.18f : 0.22f) * (std::fabs(pendiente) > 1.0f ? 1.0f : std::fabs(pendiente));
        Color arriba = TonoLlu(roca, luz);
        DrawRectangleGradientV((int)x, (int)yTope, (int)PASO + 1, (int)(yH + h * 0.04f - yTope) + 1, arriba, base);

        if (yTope < lineaNieve)
        {
            float hondo = (lineaNieve - yTope) * (0.55f + 0.25f * Hash01Llu((int)(xm / 18.0f), capa + 40));
            DrawRectangleGradientV((int)x, (int)yTope, (int)PASO + 1, (int)hondo + 1, TonoLlu(nieve, luz), AlfaLlu(TonoLlu(nieve, luz * 0.9f), 0.0f));
        }
    }
}


static void DibujarCieloLlu(float t, float entrada, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float w = (float)ancho;
    float h = (float)alto;
    float e = h / 720.0f;
    float yH = HorizonteLlu(h, entrada);

    // Degrade de la noche: cenit casi negro, horizonte violeta.
    int y1 = (int)(yH * 0.55f);
    DrawRectangleGradientV(0, 0, ancho, y1 + 1, Color{ 6, 8, 26, 255 }, Color{ 18, 22, 60, 255 });
    DrawRectangleGradientV(0, y1, ancho, (int)yH - y1 + 2, Color{ 18, 22, 60, 255 }, Color{ 60, 56, 118, 255 });
    DrawRectangleGradientV(0, (int)yH, ancho, alto - (int)yH, Color{ 96, 94, 154, 255 }, Color{ 48, 50, 104, 255 });

    // Via Lactea: banda en diagonal con polvo oscuro.
    if (detalle)
    {
        for (int k = 0; k < 16; k++)
        {
            float u = (float)k / 15.0f;
            Vector2 p = { -0.05f * w + u * 1.1f * w, 0.02f * h + u * yH * 0.75f + std::sin(u * 7.0f) * 10.0f * e };
            float r = (46.0f + 36.0f * Hash01Llu(k, 501)) * e;
            Color c = (k % 2 == 0) ? Color{ 150, 140, 222, 255 } : Color{ 210, 196, 238, 255 };
            DrawCircleGradient(p, r, Fade(c, 0.10f + 0.06f * Hash01Llu(k, 502)), Fade(c, 0.0f));
        }

        for (int k = 0; k < 7; k++)
        {
            float u = (0.5f + (float)k) / 7.0f;
            Vector2 p = { -0.05f * w + u * 1.1f * w + 18.0f * e, 0.02f * h + u * yH * 0.75f + 6.0f * e };
            DrawCircleGradient(p, (16.0f + 14.0f * Hash01Llu(k, 503)) * e, Fade(Color{ 6, 6, 22, 255 }, 0.30f), Fade(Color{ 6, 6, 22, 255 }, 0.0f));
        }
    }

    // Estrellas: titilan cada una a su ritmo.
    int estrellas = detalle ? 240 : 110;

    for (int k = 0; k < estrellas; k++)
    {
        float x, y;

        if (k % 3 == 0)
        {
            // Mas densas sobre la Via Lactea.
            float u = Hash01Llu(k, 511);
            x = -0.05f * w + u * 1.1f * w;
            y = 0.02f * h + u * yH * 0.75f + (Hash01Llu(k, 512) - 0.5f) * 70.0f * e;
        }
        else
        {
            x = Hash01Llu(k, 513) * w;
            y = Hash01Llu(k, 514) * (yH - 4.0f * e);
        }

        if (y > yH - 3.0f * e || y < 0.0f)
        {
            continue;
        }

        float brillo = Hash01Llu(k, 515);
        float titileo = 0.55f + 0.45f * std::sin(t * (1.2f + 2.6f * Hash01Llu(k, 516)) + brillo * 40.0f);
        float lado = (1.0f + 1.8f * brillo * brillo * brillo) * e;
        float bajo = Limitar01Llu((yH - y) / (60.0f * e));
        Color c = brillo > 0.8f ? Color{ 255, 236, 210, 255 } : (brillo < 0.25f ? Color{ 190, 210, 255, 255 } : Color{ 240, 244, 255, 255 });
        DrawRectangleV({ x - lado * 0.5f, y - lado * 0.5f }, { lado, lado }, Fade(c, (0.35f + 0.65f * titileo) * bajo));

        if (brillo > 0.94f)
        {
            float rayo = (5.0f + 4.0f * titileo) * e;
            DrawLineEx({ x - rayo, y }, { x + rayo, y }, 1.0f * e, Fade(c, 0.6f * titileo * bajo));
            DrawLineEx({ x, y - rayo }, { x, y + rayo }, 1.0f * e, Fade(c, 0.6f * titileo * bajo));
        }
    }

    // Luna con halo y crateres.
    Vector2 luna = { w * 0.43f, h * 0.19f - entrada * h * 0.07f };
    float radioLuna = 21.0f * e;
    DrawCircleGradient(luna, radioLuna * 5.5f, Fade(Color{ 150, 160, 230, 255 }, 0.22f), Fade(Color{ 150, 160, 230, 255 }, 0.0f));
    DrawCircleGradient(luna, radioLuna * 2.2f, Fade(Color{ 220, 226, 255, 255 }, 0.30f), Fade(Color{ 220, 226, 255, 255 }, 0.0f));
    DrawCircleV(luna, radioLuna, Color{ 236, 238, 250, 255 });
    DrawCircleV({ luna.x + radioLuna * 0.30f, luna.y - radioLuna * 0.20f }, radioLuna * 0.22f, Color{ 206, 210, 230, 255 });
    DrawCircleV({ luna.x - radioLuna * 0.35f, luna.y + radioLuna * 0.28f }, radioLuna * 0.16f, Color{ 212, 216, 234, 255 });
    DrawCircleV({ luna.x - radioLuna * 0.10f, luna.y - radioLuna * 0.45f }, radioLuna * 0.10f, Color{ 214, 218, 236, 255 });

    // Estrellas fugaces: una cada poco, con estela que se apaga.
    for (int s = 0; s < (detalle ? 2 : 1); s++)
    {
        const float PERIODO = 3.1f;
        float reloj = t + (float)s * 1.55f;
        int ciclo = (int)(reloj / PERIODO);
        float u = (reloj - (float)ciclo * PERIODO) / 0.9f;

        if (u >= 1.0f)
        {
            continue;
        }

        float lado = Hash01Llu(ciclo, 520 + s) < 0.5f ? -1.0f : 1.0f;
        Vector2 inicio = { (0.15f + 0.7f * Hash01Llu(ciclo, 521 + s)) * w, (0.03f + 0.12f * Hash01Llu(ciclo, 523 + s)) * h };
        Vector2 dir = { 0.86f * lado, 0.50f };
        float largo = (120.0f + 80.0f * Hash01Llu(ciclo, 525 + s)) * e;
        Vector2 cabeza = { inicio.x + dir.x * largo * 1.8f * u, inicio.y + dir.y * largo * 1.8f * u };
        float fundido = std::sin(u * 3.1416f);

        if (cabeza.y > yH)
        {
            continue;
        }

        for (int k = 0; k < 10; k++)
        {
            float a0 = (float)k / 10.0f;
            float a1 = (float)(k + 1) / 10.0f;
            Vector2 p0 = { cabeza.x - dir.x * largo * a0 * u, cabeza.y - dir.y * largo * a0 * u };
            Vector2 p1 = { cabeza.x - dir.x * largo * a1 * u, cabeza.y - dir.y * largo * a1 * u };
            DrawLineEx(p0, p1, (2.4f - 1.8f * a0) * e, Fade(Color{ 230, 240, 255, 255 }, (1.0f - a0) * 0.9f * fundido));
        }

        DrawCircleGradient(cabeza, 7.0f * e, Fade(WHITE, 0.8f * fundido), Fade(Color{ 180, 200, 255, 255 }, 0.0f));
    }

    // Dos cordilleras: la lejana clara y desaturada, la cercana oscura.
    DibujarCordilleraLlu(w, h, yH + 2.0f * e, 1, 11, 0.035f, 0.10f, Color{ 86, 88, 146, 255 }, Color{ 150, 156, 208, 255 }, Color{ 92, 92, 152, 255 }, 0.0f);

    // Observatorio vecino en la cordillera cercana, con baliza.
    float xVecino = w * 0.86f;
    float yVecino = yH + 8.0f * e - CimaCordilleraLlu(xVecino, w, h, 2, 9, 0.03f, 0.075f);
    DibujarCordilleraLlu(w, h, yH + 8.0f * e, 2, 9, 0.03f, 0.075f, Color{ 46, 48, 94, 255 }, Color{ 168, 174, 218, 255 }, Color{ 80, 80, 140, 255 }, 0.0f);
    DrawCircleSector({ xVecino, yVecino + 1.0f * e }, 7.0f * e, 180.0f, 360.0f, 12, Color{ 150, 156, 196, 255 });
    DrawRectangle((int)(xVecino - 7.0f * e), (int)(yVecino), (int)(14.0f * e), (int)(5.0f * e), Color{ 92, 96, 140, 255 });
    DrawLineEx({ xVecino + 12.0f * e, yVecino + 4.0f * e }, { xVecino + 12.0f * e, yVecino - 16.0f * e }, 1.2f * e, Color{ 70, 72, 120, 255 });

    if (std::fmod(t, 1.6f) < 0.5f)
    {
        DrawCircleGradient({ xVecino + 12.0f * e, yVecino - 16.0f * e }, 6.0f * e, Fade(Color{ 255, 70, 60, 255 }, 0.9f), Fade(Color{ 255, 70, 60, 255 }, 0.0f));
    }

    // Mar de nubes: bancos iluminados por la luna que derivan despacio.
    if (detalle)
    {
        for (int fila = 0; fila < 3; fila++)
        {
            float y = yH + (10.0f + 34.0f * (float)fila) * e;
            float radio = (24.0f + 16.0f * (float)fila) * e;
            float velocidad = (4.0f + 5.0f * (float)fila) * e;
            int cantidad = 14 - fila * 3;

            for (int k = 0; k < cantidad; k++)
            {
                float x = std::fmod((float)k * (w + 4.0f * radio) / (float)cantidad + t * velocidad + 37.0f * Hash01Llu(k, 530 + fila), w + 4.0f * radio) - 2.0f * radio;
                float r = radio * (0.7f + 0.6f * Hash01Llu(k, 533 + fila));
                Color c = MezclaLlu(Color{ 150, 150, 206, 255 }, Color{ 104, 104, 166, 255 }, (float)fila * 0.4f);

                for (int b = 0; b < 3; b++)
                {
                    float dx = ((float)b - 1.0f) * r * 0.9f;
                    float rb = r * (b == 1 ? 1.0f : 0.72f);
                    DrawCircleGradient({ x + dx, y + (b == 1 ? -r * 0.15f : 0.0f) }, rb, Fade(c, 0.32f), Fade(c, 0.0f));
                }
            }
        }
    }

}


//==================================================
// VISUAL: LA CIMA (relieve, nubes cercanas)
//==================================================

static const int CELDAS_X_LLU = 64;
static const int CELDAS_Z_LLU = 36;
static const float X0_TERRENO_LLU = -32.0f;
static const float Z0_TERRENO_LLU = -24.0f;


// < 1 dentro de la cima llana (meseta principal y dos espolones al fondo).
static float DistanciaCimaLlu(float x, float z)
{
    float ax = x / 15.8f;
    float az = (z + 0.6f) / 8.3f;
    float d0 = std::sqrt(std::sqrt(ax * ax * ax * ax + az * az * az * az));
    float bx = (x - CUPULA_LLU.x) / 6.4f;
    float bz = (z - CUPULA_LLU.z) / 5.6f;
    float d1 = std::sqrt(bx * bx + bz * bz);
    float cx = (x - ANTENA_LLU.x) / 5.0f;
    float cz = (z - ANTENA_LLU.z) / 4.4f;
    float d2 = std::sqrt(cx * cx + cz * cz);
    float d = d0 < d1 ? d0 : d1;
    return d < d2 ? d : d2;
}


static float AlturaTerrenoLlu(float x, float z)
{
    float d = DistanciaCimaLlu(x, z) + (RuidoLlu(x * 0.35f, z * 0.35f, 1) - 0.5f) * 0.14f;
    float bache = RuidoLlu(x * 0.8f, z * 0.8f, 2) - 0.5f;

    if (d < 1.0f)
    {
        return Y_LLANO_LLU + bache * 0.16f * SuaveLlu((d - 0.55f) / 0.45f);
    }

    float k = d - 1.0f;
    return Y_LLANO_LLU + bache * 0.16f - 26.0f * k - 5.0f * RuidoLlu(x * 0.5f, z * 0.5f, 3) * Limitar01Llu(k * 4.0f);
}


static const float* AlturasTerrenoLlu()
{
    static float alturas[(CELDAS_Z_LLU + 1) * (CELDAS_X_LLU + 1)];
    static bool listo = false;

    if (!listo)
    {
        for (int j = 0; j <= CELDAS_Z_LLU; j++)
        {
            for (int i = 0; i <= CELDAS_X_LLU; i++)
            {
                alturas[j * (CELDAS_X_LLU + 1) + i] = AlturaTerrenoLlu(X0_TERRENO_LLU + (float)i, Z0_TERRENO_LLU + (float)j);
            }
        }

        listo = true;
    }

    return alturas;
}


// Material por vertice (los parches de nieve se funden sin bordes).
static Color MaterialTerrenoLlu(Vector3 p, Vector3 n)
{
    if (p.y > Y_LLANO_LLU - 0.6f)
    {
        // Meseta: roca helada con parches de nieve.
        float parche = SuaveLlu((RuidoLlu(p.x * 0.42f, p.z * 0.42f, 4) - 0.6f) / 0.16f);
        Color suelo = TonoLlu(Color{ 82, 86, 110, 255 }, 0.9f + 0.18f * RuidoLlu(p.x * 1.4f, p.z * 1.4f, 6));
        return MezclaLlu(suelo, NIEVE_SUELO_LLU, parche * 0.85f);
    }

    // Precipicio: roca en estratos, nieve en las cornisas.
    float estrato = 0.84f + 0.16f * std::sin(p.y * 2.3f + RuidoLlu(p.x * 0.3f, p.z * 0.3f, 5) * 3.0f);
    float cornisa = SuaveLlu((n.y - 0.66f) / 0.16f) * SuaveLlu((RuidoLlu(p.x * 0.7f, p.z * 0.7f, 7) - 0.45f) / 0.2f);
    return MezclaLlu(TonoLlu(ROCA_LLU, estrato), NIEVE_SUELO_LLU, cornisa * 0.8f);
}


static void DibujarTerrenoLlu(int paso)
{
    const float* alturas = AlturasTerrenoLlu();
    const int fila = CELDAS_X_LLU + 1;

    for (int j = 0; j < CELDAS_Z_LLU; j += paso)
    {
        rlBegin(RL_QUADS);

        for (int i = 0; i < CELDAS_X_LLU; i += paso)
        {
            int i1 = i + paso > CELDAS_X_LLU ? CELDAS_X_LLU : i + paso;
            int j1 = j + paso > CELDAS_Z_LLU ? CELDAS_Z_LLU : j + paso;
            float x0 = X0_TERRENO_LLU + (float)i;
            float x1 = X0_TERRENO_LLU + (float)i1;
            float z0 = Z0_TERRENO_LLU + (float)j;
            float z1 = Z0_TERRENO_LLU + (float)j1;

            // La terraza tapa su propio hueco.
            if (x0 >= -BORDE_X_LLU && x1 <= BORDE_X_LLU && z0 >= -BORDE_Z_LLU && z1 <= BORDE_Z_LLU)
            {
                continue;
            }

            float h00 = alturas[j * fila + i];
            float h10 = alturas[j * fila + i1];
            float h01 = alturas[j1 * fila + i];
            float h11 = alturas[j1 * fila + i1];

            if (h00 < -36.0f && h10 < -36.0f && h01 < -36.0f && h11 < -36.0f)
            {
                continue;
            }

            float lado = x1 - x0;
            float dx = ((h10 + h11) - (h00 + h01)) * 0.5f / lado;
            float dz = ((h01 + h11) - (h00 + h10)) * 0.5f / lado;
            Vector3 n = Vector3Normalize({ -dx, 1.0f, -dz });
            float luz = LuzNormalLlu(n);
            Vector3 a = { x0, h00, z0 };
            Vector3 b = { x0, h01, z1 };
            Vector3 c = { x1, h11, z1 };
            Vector3 d = { x1, h10, z0 };
            VerticeLlu(a, TinteLlu(TonoLlu(MaterialTerrenoLlu(a, n), luz), a));
            VerticeLlu(b, TinteLlu(TonoLlu(MaterialTerrenoLlu(b, n), luz), b));
            VerticeLlu(c, TinteLlu(TonoLlu(MaterialTerrenoLlu(c, n), luz), c));
            VerticeLlu(d, TinteLlu(TonoLlu(MaterialTerrenoLlu(d, n), luz), d));
        }

        rlEnd();
    }
}


// Nubes bajas que rozan la cima (alfa normal, sin escribir profundidad).
static void DibujarNubesCercanasLlu(float t)
{
    for (int k = 0; k < 14; k++)
    {
        float angulo = 6.2832f * (float)k / 14.0f + t * 0.012f + Hash01Llu(k, 601) * 0.3f;
        float radio = 21.0f + 6.0f * Hash01Llu(k, 602);
        Vector3 c =
        {
            std::cos(angulo) * radio * 1.15f,
            -8.0f - 4.0f * Hash01Llu(k, 603),
            -2.0f + std::sin(angulo) * radio * 0.8f
        };

        if (c.z > 10.0f)
        {
            continue;
        }

        float r = 5.0f + 3.0f * Hash01Llu(k, 604);
        Color nube = Color{ 146, 146, 200, 70 };

        for (int b = 0; b < 3; b++)
        {
            Vector3 p = { c.x + ((float)b - 1.0f) * r * 0.8f, c.y + (b == 1 ? 0.8f : 0.0f), c.z };
            ResplandorLlu(p, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.70f, 0.71f }, r * (b == 1 ? 1.0f : 0.75f), nube, 1.4f);
        }
    }
}


//==================================================
// VISUAL: TERRAZA (arena), MUROS Y FACHADA
//==================================================

static const int CANTIDAD_VENTANAS_LLU = 7;


static void DibujarFachadaLlu()
{
    // Bloque de la azotea: su frente es la fachada del observatorio.
    CuboLlu({ 0.0f, Y_LLANO_LLU * 0.5f, 0.0f }, { BORDE_X_LLU * 2.0f, -Y_LLANO_LLU, BORDE_Z_LLU * 2.0f }, PIEDRA_OSCURA_LLU);
    CuboLlu({ 0.0f, -0.09f, 0.0f }, { BORDE_X_LLU * 2.0f + 0.3f, 0.18f, BORDE_Z_LLU * 2.0f + 0.3f }, PIEDRA_CLARA_LLU);
    CuboLlu({ 0.0f, Y_LLANO_LLU + 0.1f, 0.0f }, { BORDE_X_LLU * 2.0f + 0.2f, 0.2f, BORDE_Z_LLU * 2.0f + 0.2f }, PIEDRA_LLU);

    // Ventanas encendidas de la sala de abajo.
    for (int k = 0; k < CANTIDAD_VENTANAS_LLU; k++)
    {
        float x = -7.2f + 2.4f * (float)k;
        float z = BORDE_Z_LLU + 0.01f;
        CuboLlu({ x, -0.62f, z }, { 0.78f, 0.62f, 0.04f }, PIEDRA_CLARA_LLU);
        LosaLlu({ x, -0.94f, z + 0.12f }, 0.42f, 0.1f, PIEDRA_CLARA_LLU);

        rlBegin(RL_QUADS);
        Color vidrio = Color{ 255, 198, 120, 255 };
        Color vidrioBajo = Color{ 236, 150, 84, 255 };
        VerticeLlu({ x - 0.28f, -0.88f, z + 0.03f }, vidrioBajo);
        VerticeLlu({ x + 0.28f, -0.88f, z + 0.03f }, vidrioBajo);
        VerticeLlu({ x + 0.28f, -0.40f, z + 0.03f }, vidrio);
        VerticeLlu({ x - 0.28f, -0.40f, z + 0.03f }, vidrio);
        rlEnd();
        CuboLlu({ x, -0.64f, z + 0.04f }, { 0.04f, 0.48f, 0.02f }, BRONCE_OSCURO_LLU);
        CuboLlu({ x, -0.64f, z + 0.04f }, { 0.56f, 0.04f, 0.02f }, BRONCE_OSCURO_LLU);
    }
}


static void DibujarSueloTerrazaLlu()
{
    // Juntas: una sola losa oscura bajo las baldosas.
    LosaLlu({ 0.0f, 0.0f, 0.0f }, MEDIO_X_ARENA_LLUVIA, MEDIO_Z_ARENA_LLUVIA, JUNTA_LLU);

    // Losas a soga: filas de 1 de fondo, juntas trabadas.
    rlBegin(RL_QUADS);

    for (int fila = 0; fila < 11; fila++)
    {
        float z0 = -MEDIO_Z_ARENA_LLUVIA + (float)fila;
        float z1 = z0 + 1.0f;
        float x = -MEDIO_X_ARENA_LLUVIA - (fila % 2 == 0 ? 0.0f : 0.7f);
        int indice = 0;

        while (x < MEDIO_X_ARENA_LLUVIA)
        {
            float x0 = x < -MEDIO_X_ARENA_LLUVIA ? -MEDIO_X_ARENA_LLUVIA : x;
            float x1 = x + 1.4f > MEDIO_X_ARENA_LLUVIA ? MEDIO_X_ARENA_LLUVIA : x + 1.4f;
            float variacion = 0.90f + 0.18f * Hash01Llu(fila, indice);
            Color losa = TonoLlu(LOSA_LLU, variacion);

            if (Hash01Llu(fila, indice + 300) > 0.86f)
            {
                losa = MezclaLlu(losa, Color{ 136, 150, 168, 255 }, 0.35f);   // escarcha
            }

            Color borde = TonoLlu(losa, 0.9f);
            float y = 0.006f;
            QuadLlu({ x0 + 0.04f, y, z0 + 0.04f }, { x0 + 0.04f, y, z1 - 0.04f }, { x1 - 0.04f, y, z1 - 0.04f }, { x1 - 0.04f, y, z0 + 0.04f }, losa);
            // Canto de abajo un poco mas oscuro: da relieve a cada losa.
            QuadLlu({ x0 + 0.04f, y + 0.001f, z1 - 0.16f }, { x0 + 0.04f, y + 0.001f, z1 - 0.04f }, { x1 - 0.04f, y + 0.001f, z1 - 0.04f }, { x1 - 0.04f, y + 0.001f, z1 - 0.16f }, borde);

            x += 1.4f;
            indice++;
        }
    }

    rlEnd();

    // Rosa celeste de bronce embutida en el centro (apagada, no compite).
    Color bronce = MezclaLlu(BRONCE_LLU, LOSA_LLU, 0.45f);
    Color bronceOscuro = MezclaLlu(BRONCE_OSCURO_LLU, LOSA_LLU, 0.35f);
    AnilloLlu({ 0.0f, 0.012f, 0.0f }, 4.55f, 4.70f, bronce, 48);
    AnilloLlu({ 0.0f, 0.012f, 0.0f }, 3.30f, 3.40f, bronce, 48);

    rlBegin(RL_QUADS);

    for (int k = 0; k < 24; k++)
    {
        float a = 6.2832f * (float)k / 24.0f;
        float r0 = (k % 2 == 0) ? 3.95f : 4.20f;
        Vector3 d = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 p = { -d.z * 0.04f, 0.0f, d.x * 0.04f };
        QuadLlu(
            { d.x * r0 - p.x, 0.013f, d.z * r0 - p.z },
            { d.x * 4.5f - p.x, 0.013f, d.z * 4.5f - p.z },
            { d.x * 4.5f + p.x, 0.013f, d.z * 4.5f + p.z },
            { d.x * r0 + p.x, 0.013f, d.z * r0 + p.z },
            bronce
        );
    }

    rlEnd();

    // Estrella de ocho puntas: cada punta en dos tonos.
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 8; k++)
    {
        float a = 6.2832f * (float)k / 8.0f;
        float largo = (k % 2 == 0) ? 3.2f : 2.0f;
        float ancho = (k % 2 == 0) ? 0.34f : 0.24f;
        Vector3 punta = { std::cos(a) * largo, 0.014f, std::sin(a) * largo };
        Vector3 izq = { std::cos(a + 1.5708f) * ancho, 0.014f, std::sin(a + 1.5708f) * ancho };
        Vector3 der = { -izq.x, 0.014f, -izq.z };
        Vector3 centro = { 0.0f, 0.014f, 0.0f };
        VerticeLlu(centro, bronce);
        VerticeLlu(izq, bronce);
        VerticeLlu(punta, bronce);
        VerticeLlu(centro, bronceOscuro);
        VerticeLlu(punta, bronceOscuro);
        VerticeLlu(der, bronceOscuro);
    }

    rlEnd();

    // Escarcha acumulada al pie de los muros.
    Color escarcha = Color{ 196, 206, 232, 70 };
    Color nada = Color{ 196, 206, 232, 0 };
    float mx = MEDIO_X_ARENA_LLUVIA;
    float mz = MEDIO_Z_ARENA_LLUVIA;
    float y = 0.016f;
    rlBegin(RL_QUADS);
    QuadLlu({ -mx, y, -mz }, { -mx, y, mz }, { -mx + 0.3f, y, mz }, { -mx + 0.3f, y, -mz }, escarcha);
    VerticeLlu({ -mx + 0.3f, y, -mz }, escarcha); VerticeLlu({ -mx + 0.3f, y, mz }, escarcha); VerticeLlu({ -mx + 0.75f, y, mz }, nada); VerticeLlu({ -mx + 0.75f, y, -mz }, nada);
    QuadLlu({ mx - 0.3f, y, -mz }, { mx - 0.3f, y, mz }, { mx, y, mz }, { mx, y, -mz }, escarcha);
    VerticeLlu({ mx - 0.75f, y, -mz }, nada); VerticeLlu({ mx - 0.75f, y, mz }, nada); VerticeLlu({ mx - 0.3f, y, mz }, escarcha); VerticeLlu({ mx - 0.3f, y, -mz }, escarcha);
    QuadLlu({ -mx, y, -mz }, { -mx, y, -mz + 0.3f }, { mx, y, -mz + 0.3f }, { mx, y, -mz }, escarcha);
    VerticeLlu({ -mx, y, -mz + 0.3f }, escarcha); VerticeLlu({ -mx, y, -mz + 0.75f }, nada); VerticeLlu({ mx, y, -mz + 0.75f }, nada); VerticeLlu({ mx, y, -mz + 0.3f }, escarcha);
    rlEnd();
}


// Muro de sillares (sobre la caja de colision exacta) con albardilla.
static void MuroSillaresLlu(Vector3 centro, Vector3 tamano, bool largoEnX)
{
    float largo = largoEnX ? tamano.x : tamano.z;
    int tramos = (int)(largo / 1.5f + 0.5f);
    float paso = largo / (float)tramos;

    for (int k = 0; k < tramos; k++)
    {
        float desde = -largo * 0.5f + paso * (float)k;
        float medio = desde + paso * 0.5f;
        Color sillar = TonoLlu(PIEDRA_LLU, 0.92f + 0.14f * Hash01Llu(k, (int)(centro.x * 10.0f + centro.z * 7.0f)));
        Vector3 c = largoEnX ? Vector3{ centro.x + medio, centro.y, centro.z } : Vector3{ centro.x, centro.y, centro.z + medio };
        Vector3 t = largoEnX ? Vector3{ paso - 0.03f, tamano.y, tamano.z } : Vector3{ tamano.x, tamano.y, paso - 0.03f };
        CuboLlu(c, t, sillar);
    }

    // Junta horizontal y albardilla clara apenas mas ancha.
    Vector3 junta = largoEnX ? Vector3{ tamano.x, 0.03f, tamano.z + 0.01f } : Vector3{ tamano.x + 0.01f, 0.03f, tamano.z };
    CuboLlu({ centro.x, centro.y, centro.z }, junta, JUNTA_LLU);
    Vector3 albardilla = largoEnX ? Vector3{ tamano.x + 0.12f, 0.12f, tamano.z + 0.12f } : Vector3{ tamano.x + 0.12f, 0.12f, tamano.z + 0.12f };
    CuboLlu({ centro.x, centro.y + tamano.y * 0.5f + 0.06f, centro.z }, albardilla, PIEDRA_CLARA_LLU);
}


static void DibujarMurosLlu(bool detalle)
{
    MuroSillaresLlu({ -8.75f, 0.5f, 0.0f }, { 0.5f, 1.0f, 12.0f }, false);
    MuroSillaresLlu({ 8.75f, 0.5f, 0.0f }, { 0.5f, 1.0f, 12.0f }, false);
    MuroSillaresLlu({ 0.0f, 0.5f, -5.75f }, { 18.0f, 1.0f, 0.5f }, true);

    // Frente: balaustrada calada para ver la ultima fila de la terraza.
    float z = 5.75f;
    CuboLlu({ 0.0f, 0.1f, z }, { 18.0f, 0.2f, 0.5f }, PIEDRA_LLU);
    CuboLlu({ 0.0f, 0.91f, z }, { 18.0f, 0.18f, 0.5f }, PIEDRA_CLARA_LLU);

    for (int k = 0; k < 34; k++)
    {
        float x = -8.25f + 0.5f * (float)k;

        if (std::fabs(std::fabs(x) - 8.75f) < 0.3f)
        {
            continue;
        }

        int lados = detalle ? 8 : 4;
        PrismaLlu({ x, 0.2f, z }, 0.07f, 0.12f, 0.24f, lados, PIEDRA_CLARA_LLU, false);
        PrismaLlu({ x, 0.44f, z }, 0.12f, 0.06f, 0.38f, lados, PIEDRA_CLARA_LLU, false);
    }

    // Pilastras de esquina con farol de bronce encima.
    for (int k = 0; k < 4; k++)
    {
        float x = (k % 2 == 0) ? -8.75f : 8.75f;
        float zz = (k < 2) ? -5.75f : 5.75f;
        CuboLlu({ x, 0.7f, zz }, { 0.6f, 1.4f, 0.6f }, PIEDRA_LLU);
        CuboLlu({ x, 1.46f, zz }, { 0.74f, 0.12f, 0.74f }, PIEDRA_CLARA_LLU);
        CuboLlu({ x, 1.56f, zz }, { 0.3f, 0.08f, 0.3f }, BRONCE_OSCURO_LLU);
        CuboLlu({ x, 1.80f, zz }, { 0.24f, 0.40f, 0.24f }, Color{ 255, 214, 150, 255 });

        for (int p = 0; p < 4; p++)
        {
            float dx = (p % 2 == 0) ? -0.12f : 0.12f;
            float dz = (p < 2) ? -0.12f : 0.12f;
            VigaLlu({ x + dx, 1.6f, zz + dz }, { x + dx, 2.0f, zz + dz }, 0.035f, BRONCE_OSCURO_LLU);
        }

        PrismaLlu({ x, 2.0f, zz }, 0.22f, 0.0f, 0.18f, 4, BRONCE_OSCURO_LLU);
    }
}


//==================================================
// VISUAL: UTILERIA DEL OBSERVATORIO
//==================================================

// Farola de hierro con farol; devuelve el centro de la luz.
static Vector3 FarolaLlu(Vector3 pie, float alto)
{
    PrismaLlu(pie, 0.22f, 0.16f, 0.3f, 8, Color{ 54, 56, 70, 255 });
    TuboLlu({ pie.x, pie.y + 0.3f, pie.z }, { pie.x, pie.y + alto, pie.z }, 0.06f, 0.045f, Color{ 50, 52, 66, 255 }, 6, false);
    Vector3 luz = { pie.x, pie.y + alto + 0.22f, pie.z };
    CuboLlu({ luz.x, luz.y - 0.2f, luz.z }, { 0.22f, 0.06f, 0.22f }, Color{ 50, 52, 66, 255 });
    CuboLlu(luz, { 0.2f, 0.34f, 0.2f }, Color{ 255, 212, 150, 255 });
    PrismaLlu({ luz.x, luz.y + 0.17f, luz.z }, 0.2f, 0.0f, 0.16f, 4, Color{ 50, 52, 66, 255 });
    return luz;
}


static void PinoLlu(Vector3 pie, float alto, int semilla)
{
    float ancho = alto * (0.34f + 0.06f * Hash01Llu(semilla, 701));
    TuboLlu(pie, { pie.x, pie.y + alto * 0.25f, pie.z }, 0.12f, 0.1f, Color{ 76, 56, 48, 255 }, 5, false);

    for (int k = 0; k < 4; k++)
    {
        float y = pie.y + alto * (0.18f + 0.2f * (float)k);
        float r = ancho * (1.0f - 0.21f * (float)k);
        Color hoja = TinteLlu(TonoLlu(PINO_LLU, 0.92f + 0.12f * (float)k * 0.3f), pie);
        PrismaLlu({ pie.x, y, pie.z }, r, 0.0f, alto * 0.38f, 7, hoja);
        PrismaLlu({ pie.x, y + alto * 0.16f, pie.z }, r * 0.62f, 0.0f, alto * 0.23f, 7, TinteLlu(NIEVE_LLU, pie));
    }
}


static void RocaLlu(Vector3 pie, float tamano, int semilla)
{
    float giro = 360.0f * Hash01Llu(semilla, 711);
    float alto = tamano * (0.5f + 0.4f * Hash01Llu(semilla, 712));
    CuboGiradoLlu({ pie.x, pie.y + alto * 0.45f, pie.z }, { tamano, alto, tamano * 0.8f }, giro, TinteLlu(TonoLlu(ROCA_LLU, 0.9f), pie));
    CuboGiradoLlu({ pie.x + tamano * 0.35f, pie.y + alto * 0.3f, pie.z + tamano * 0.2f }, { tamano * 0.6f, alto * 0.65f, tamano * 0.6f }, giro + 35.0f, TinteLlu(TonoLlu(ROCA_LLU, 0.9f), pie));

    if (Hash01Llu(semilla, 713) > 0.4f)
    {
        CuboGiradoLlu({ pie.x - tamano * 0.08f, pie.y + alto * 0.92f, pie.z }, { tamano * 0.56f, 0.06f, tamano * 0.44f }, giro, TinteLlu(NIEVE_SUELO_LLU, pie));
    }
}


// Caja de instrumental de madera con flejes de bronce.
static void CajaInstrumentalLlu(Vector3 centro, Vector3 tamano, float giro)
{
    CuboGiradoLlu(centro, tamano, giro, MADERA_LLU);
    CuboGiradoLlu(centro, { tamano.x + 0.03f, tamano.y * 0.12f, tamano.z + 0.03f }, giro, BRONCE_OSCURO_LLU);
    CuboGiradoLlu({ centro.x, centro.y + tamano.y * 0.36f, centro.z }, { tamano.x + 0.03f, tamano.y * 0.08f, tamano.z + 0.03f }, giro, BRONCE_OSCURO_LLU);
    CuboGiradoLlu({ centro.x, centro.y - tamano.y * 0.36f, centro.z }, { tamano.x + 0.03f, tamano.y * 0.08f, tamano.z + 0.03f }, giro, BRONCE_OSCURO_LLU);
}


// MODELO FUTURO: esfera armilar.
// Esfera armilar: aros de bronce que giran alrededor de un globo.
static void EsferaArmilarLlu(Vector3 c, float radio, float t)
{
    float g = t * 0.5f;
    Vector3 eje = Vector3Normalize({ 0.4f, 0.92f, 0.0f });
    Vector3 ex, ey;
    BaseEjeLlu(eje, ex, ey);

    TuboLlu(Vector3Subtract(c, Vector3Scale(eje, radio * 1.25f)), Vector3Add(c, Vector3Scale(eje, radio * 1.25f)), 0.025f, 0.025f, BRONCE_OSCURO_LLU, 5);
    AroLlu(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, 0.06f, BRONCE_LLU);

    Vector3 m1 = Vector3Add(Vector3Scale(ex, std::cos(g)), Vector3Scale(ey, std::sin(g)));
    AroLlu(c, m1, eje, radio * 0.94f, 0.05f, BRONCE_LLU);
    Vector3 m2 = Vector3Add(Vector3Scale(ex, std::cos(g + 1.5708f)), Vector3Scale(ey, std::sin(g + 1.5708f)));
    AroLlu(c, m2, eje, radio * 0.88f, 0.05f, BRONCE_OSCURO_LLU);

    // Ecliptica inclinada.
    Vector3 e1 = Vector3Normalize({ std::cos(-g * 0.6f), 0.4f, std::sin(-g * 0.6f) });
    Vector3 e2 = Vector3Normalize(Vector3CrossProduct(e1, { 0.0f, 1.0f, 0.0f }));
    AroLlu(c, e1, e2, radio * 0.80f, 0.07f, Color{ 220, 180, 100, 255 });

    EsferaLlu(c, radio * 0.26f, Color{ 90, 130, 200, 255 }, 10, 3);
}


// MODELO FUTURO: telescopio refractor en tripode.
// Telescopio refractor en tripode de madera.
static void TelescopioTripodeLlu(Vector3 pie, float acimut, float elevacion, float escala)
{
    Vector3 cabeza = { pie.x, pie.y + 1.45f * escala, pie.z };

    for (int k = 0; k < 3; k++)
    {
        float a = acimut + 6.2832f * (float)k / 3.0f + 0.5f;
        VigaLlu(cabeza, { pie.x + std::cos(a) * 0.7f * escala, pie.y, pie.z + std::sin(a) * 0.7f * escala }, 0.07f * escala, MADERA_LLU);
    }

    CuboLlu(cabeza, { 0.22f * escala, 0.16f * escala, 0.22f * escala }, BRONCE_OSCURO_LLU);
    Vector3 dir = { std::cos(acimut) * std::cos(elevacion), std::sin(elevacion), std::sin(acimut) * std::cos(elevacion) };
    Vector3 centro = { cabeza.x, cabeza.y + 0.14f * escala, cabeza.z };
    Vector3 atras = Vector3Subtract(centro, Vector3Scale(dir, 0.7f * escala));
    Vector3 adelante = Vector3Add(centro, Vector3Scale(dir, 1.3f * escala));
    TuboLlu(atras, adelante, 0.09f * escala, 0.13f * escala, BRONCE_LLU, 10);
    TuboLlu(Vector3Subtract(adelante, Vector3Scale(dir, 0.25f * escala)), Vector3Add(adelante, Vector3Scale(dir, 0.05f * escala)), 0.15f * escala, 0.15f * escala, BRONCE_OSCURO_LLU, 10);
    TuboLlu(atras, Vector3Subtract(atras, Vector3Scale(dir, 0.2f * escala)), 0.04f * escala, 0.04f * escala, Color{ 60, 60, 70, 255 }, 6);
    Vector3 buscador = Vector3Add(centro, Vector3Scale({ -dir.z, 0.0f, dir.x }, 0.0f));
    buscador.y += 0.17f * escala;
    TuboLlu(buscador, Vector3Add(buscador, Vector3Scale(dir, 0.5f * escala)), 0.035f * escala, 0.04f * escala, BRONCE_OSCURO_LLU, 6);
}


// Mastil meteorologico con anemometro que gira; devuelve la baliza.
static Vector3 MastilMeteoLlu(Vector3 pie, float t)
{
    float alto = 3.4f;
    PrismaLlu(pie, 0.3f, 0.26f, 0.25f, 6, PIEDRA_OSCURA_LLU);

    for (int k = 0; k < 3; k++)
    {
        float a = 6.2832f * (float)k / 3.0f;
        Vector3 abajo = { pie.x + std::cos(a) * 0.16f, pie.y + 0.25f, pie.z + std::sin(a) * 0.16f };
        Vector3 arriba = { pie.x + std::cos(a) * 0.06f, pie.y + alto, pie.z + std::sin(a) * 0.06f };
        VigaLlu(abajo, arriba, 0.035f, Color{ 170, 176, 190, 255 });
    }

    for (int k = 1; k < 6; k++)
    {
        float y = pie.y + 0.25f + (alto - 0.25f) * (float)k / 6.0f;
        float r = 0.16f - 0.1f * (float)k / 6.0f;
        AnilloLlu({ pie.x, y, pie.z }, r - 0.02f, r + 0.01f, Color{ 150, 156, 172, 255 }, 6);
    }

    Vector3 tope = { pie.x, pie.y + alto, pie.z };
    float giro = t * 7.0f;

    for (int k = 0; k < 3; k++)
    {
        float a = giro + 6.2832f * (float)k / 3.0f;
        Vector3 brazo = { tope.x + std::cos(a) * 0.38f, tope.y + 0.1f, tope.z + std::sin(a) * 0.38f };
        VigaLlu({ tope.x, tope.y + 0.1f, tope.z }, brazo, 0.03f, Color{ 190, 196, 210, 255 });
        EsferaLlu(brazo, 0.08f, Color{ 220, 226, 236, 255 }, 6, 2);
    }

    // Veleta.
    float viento = 0.6f + 0.25f * std::sin(t * 0.7f);
    Vector3 cola = { tope.x - std::cos(viento) * 0.5f, tope.y - 0.25f, tope.z - std::sin(viento) * 0.5f };
    Vector3 punta = { tope.x + std::cos(viento) * 0.35f, tope.y - 0.25f, tope.z + std::sin(viento) * 0.35f };
    VigaLlu(cola, punta, 0.04f, BRONCE_LLU);
    CuboGiradoLlu(cola, { 0.04f, 0.2f, 0.22f }, -viento * RAD2DEG, BRONCE_LLU);

    return { tope.x, tope.y + 0.32f, tope.z };
}


//==================================================
// VISUAL: CUPULA PRINCIPAL Y RADIOTELESCOPIO
//==================================================

static float GiroCupulaLlu(float t)
{
    return 2.85f + 0.16f * std::sin(t * 0.11f);
}


static Vector3 DireccionTelescopioLlu(float t)
{
    float giro = GiroCupulaLlu(t);
    float elevacion = 0.78f + 0.06f * std::sin(t * 0.17f + 1.0f);
    return { std::cos(giro) * std::cos(elevacion), std::sin(elevacion), std::sin(giro) * std::cos(elevacion) };
}


// MODELO FUTURO: cupula del observatorio (tambor, domo de gajos con ranura, telescopio).
static void DibujarCupulaLlu(float t, bool detalle)
{
    Vector3 c = CUPULA_LLU;
    float r = RADIO_CUPULA_LLU;
    int lados = detalle ? 28 : 18;

    // Tambor de piedra con zocalo, ventanas y cornisa.
    PrismaLlu(c, r + 0.3f, r + 0.3f, 0.45f, lados, TinteLlu(PIEDRA_OSCURA_LLU, c));
    PrismaLlu({ c.x, c.y + 0.45f, c.z }, r, r, ALTO_TAMBOR_LLU - 0.45f, lados, TinteLlu(PIEDRA_LLU, c));
    PrismaLlu({ c.x, c.y + ALTO_TAMBOR_LLU - 0.22f, c.z }, r + 0.18f, r + 0.18f, 0.24f, lados, TinteLlu(PIEDRA_CLARA_LLU, c));

    // Puerta que mira a la terraza y ventanitas encendidas.
    float aPuerta = std::atan2(-c.z, -c.x);
    Vector3 dPuerta = { std::cos(aPuerta), 0.0f, std::sin(aPuerta) };
    Vector3 puerta = Vector3Add(c, Vector3Scale(dPuerta, r + 0.02f));
    CuboGiradoLlu({ puerta.x, c.y + 0.85f, puerta.z }, { 1.1f, 1.7f, 0.12f }, -aPuerta * RAD2DEG + 90.0f, TinteLlu(MADERA_LLU, c));
    CuboGiradoLlu({ puerta.x, c.y + 1.82f, puerta.z }, { 1.3f, 0.16f, 0.16f }, -aPuerta * RAD2DEG + 90.0f, TinteLlu(PIEDRA_CLARA_LLU, c));

    for (int k = -1; k <= 1; k += 2)
    {
        float a = aPuerta + 0.55f * (float)k;
        Vector3 v = Vector3Add(c, Vector3Scale({ std::cos(a), 0.0f, std::sin(a) }, r + 0.02f));
        CuboGiradoLlu({ v.x, c.y + 1.4f, v.z }, { 0.42f, 0.55f, 0.06f }, -a * RAD2DEG + 90.0f, Color{ 255, 196, 118, 255 });
    }

    // Domo de gajos con la ranura abierta; por dentro, oscuro y tibio.
    float giro = GiroCupulaLlu(t);
    float ancho = 0.42f;
    Vector3 base = { c.x, c.y + ALTO_TAMBOR_LLU, c.z };
    DomoLlu(base, r - 0.08f, r - 0.08f, r - 0.08f, Color{ 56, 44, 52, 255 }, lados, detalle ? 8 : 5);
    DomoLlu(base, r, r, r, TinteLlu(CUPULA_COLOR_LLU, base), lados, detalle ? 8 : 5, giro - ancho * 0.5f, giro + ancho * 0.5f, true);

    // Rieles de la compuerta a cada lado de la ranura.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float a = giro + (float)lado * ancho * 0.5f;
        Vector3 previo{};

        for (int k = 0; k <= 7; k++)
        {
            float f = 1.5708f * (float)k / 8.0f;
            Vector3 p = { base.x + (r + 0.04f) * std::cos(f) * std::cos(a), base.y + (r + 0.04f) * std::sin(f), base.z + (r + 0.04f) * std::cos(f) * std::sin(a) };
            if (k > 0) VigaLlu(previo, p, 0.12f, TinteLlu(Color{ 150, 156, 182, 255 }, base));
            previo = p;
        }
    }

    // Telescopio asomando por la ranura, siguiendo una estrella.
    Vector3 dir = DireccionTelescopioLlu(t);
    Vector3 pivote = { c.x, base.y + 0.5f, c.z };
    Vector3 atras = Vector3Subtract(pivote, Vector3Scale(dir, 1.4f));
    Vector3 boca = Vector3Add(pivote, Vector3Scale(dir, 4.4f));
    TuboLlu(atras, boca, 0.62f, 0.66f, Color{ 230, 234, 244, 255 }, 14);
    TuboLlu(Vector3Subtract(boca, Vector3Scale(dir, 0.5f)), Vector3Add(boca, Vector3Scale(dir, 0.08f)), 0.72f, 0.72f, Color{ 52, 54, 70, 255 }, 14);
    TuboLlu(Vector3Add(pivote, Vector3Scale(dir, 0.8f)), Vector3Add(pivote, Vector3Scale(dir, 1.1f)), 0.7f, 0.7f, BRONCE_LLU, 14);
    TuboLlu(Vector3Add(pivote, Vector3Scale(dir, 2.6f)), Vector3Add(pivote, Vector3Scale(dir, 2.85f)), 0.7f, 0.7f, BRONCE_LLU, 14);

    // Remate con pararrayos.
    Vector3 tope = { base.x, base.y + r, base.z };
    EsferaLlu(tope, 0.22f, BRONCE_LLU, 8, 3);
    VigaLlu(tope, { tope.x, tope.y + 1.0f, tope.z }, 0.05f, BRONCE_OSCURO_LLU);
}


static Vector3 PuntaAntenaLlu()
{
    return { ANTENA_LLU.x, ANTENA_LLU.y + 3.1f, ANTENA_LLU.z };
}


// MODELO FUTURO: radiotelescopio (pedestal, horquilla y plato).
static void DibujarAntenaLlu(float t)
{
    Vector3 pie = ANTENA_LLU;
    PrismaLlu(pie, 1.1f, 0.9f, 0.6f, 10, TinteLlu(PIEDRA_OSCURA_LLU, pie));
    TuboLlu({ pie.x, pie.y + 0.6f, pie.z }, { pie.x, pie.y + 2.3f, pie.z }, 0.32f, 0.26f, TinteLlu(Color{ 196, 200, 214, 255 }, pie), 10);

    float acimut = 0.6f + t * 0.08f;
    float elevacion = 0.75f + 0.1f * std::sin(t * 0.13f);
    Vector3 d = { std::cos(acimut) * std::cos(elevacion), std::sin(elevacion), std::sin(acimut) * std::cos(elevacion) };
    Vector3 centro = { pie.x, pie.y + 2.7f, pie.z };
    CuboGiradoLlu({ pie.x, pie.y + 2.45f, pie.z }, { 0.8f, 0.3f, 0.5f }, -acimut * RAD2DEG, TinteLlu(Color{ 170, 176, 192, 255 }, pie));

    // Plato parabolico: anillos de cuadrilateros alrededor del eje d.
    Vector3 ex, ey;
    BaseEjeLlu(d, ex, ey);
    const float R = 1.75f;
    const int ANILLOS = 4;
    const int LADOS = 18;
    Color plato = TinteLlu(Color{ 188, 194, 214, 255 }, pie);

    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        float r0 = R * (float)a / (float)ANILLOS;
        float r1 = R * (float)(a + 1) / (float)ANILLOS;
        float p0 = 0.55f * (r0 / R) * (r0 / R);
        float p1 = 0.55f * (r1 / R) * (r1 / R);

        for (int k = 0; k < LADOS; k++)
        {
            float a0 = 6.2832f * (float)k / (float)LADOS;
            float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
            Vector3 u0 = Vector3Add(Vector3Scale(ex, std::cos(a0)), Vector3Scale(ey, std::sin(a0)));
            Vector3 u1 = Vector3Add(Vector3Scale(ex, std::cos(a1)), Vector3Scale(ey, std::sin(a1)));
            Vector3 n = Vector3Normalize(Vector3Subtract(d, Vector3Scale(Vector3Add(u0, u1), 0.25f * (float)a / (float)ANILLOS)));
            Color cara = TonoLlu(plato, LuzNormalLlu(n) * ((k % 3 == 0) ? 0.94f : 1.0f));
            QuadLlu(
                Vector3Add(centro, Vector3Add(Vector3Scale(u0, r0), Vector3Scale(d, p0))),
                Vector3Add(centro, Vector3Add(Vector3Scale(u0, r1), Vector3Scale(d, p1))),
                Vector3Add(centro, Vector3Add(Vector3Scale(u1, r1), Vector3Scale(d, p1))),
                Vector3Add(centro, Vector3Add(Vector3Scale(u1, r0), Vector3Scale(d, p0))),
                cara
            );
        }
    }

    rlEnd();

    // Bocina en el foco, sostenida por tres patas.
    Vector3 foco = Vector3Add(centro, Vector3Scale(d, 1.35f));

    for (int k = 0; k < 3; k++)
    {
        float a = 6.2832f * (float)k / 3.0f;
        Vector3 borde = Vector3Add(centro, Vector3Add(Vector3Scale(Vector3Add(Vector3Scale(ex, std::cos(a)), Vector3Scale(ey, std::sin(a))), R * 0.92f), Vector3Scale(d, 0.5f)));
        VigaLlu(borde, foco, 0.05f, Color{ 160, 164, 180, 255 });
    }

    TuboLlu(Vector3Subtract(foco, Vector3Scale(d, 0.2f)), Vector3Add(foco, Vector3Scale(d, 0.15f)), 0.16f, 0.1f, Color{ 120, 124, 140, 255 }, 8);
}


// Toda la utileria fuera de la terraza; devuelve las luces en luces[].
static int DibujarUtileriaLlu(float t, int paso, Vector3 luces[], int maxLuces)
{
    int n = 0;
    float y = Y_LLANO_LLU;

    // Farolas (las cuatro mas cercanas a la terraza siempre).
    const Vector3 farolas[6] =
    {
        { -10.0f, y, 5.2f }, { -10.1f, y, -1.6f }, { 10.0f, y, 5.2f },
        { 10.1f, y, -1.6f }, { -4.6f, y, -7.4f }, { 3.6f, y, -7.4f }
    };

    for (int k = 0; k < 6; k++)
    {
        Vector3 luz = FarolaLlu(farolas[k], 2.2f);
        if (n < maxLuces) luces[n++] = luz;
    }

    // Izquierda: telescopio en tripode, esfera armilar, cajas y pinos.
    TelescopioTripodeLlu({ -11.4f, y, 2.0f }, 3.6f, 0.75f, 1.0f);
    PrismaLlu({ -12.0f, y, -3.4f }, 0.55f, 0.55f, 0.25f, 8, PIEDRA_OSCURA_LLU);
    PrismaLlu({ -12.0f, y + 0.25f, -3.4f }, 0.3f, 0.22f, 1.1f, 8, PIEDRA_LLU);
    PrismaLlu({ -12.0f, y + 1.35f, -3.4f }, 0.4f, 0.4f, 0.12f, 8, PIEDRA_CLARA_LLU);
    EsferaArmilarLlu({ -12.0f, y + 2.15f, -3.4f }, 0.7f, t);
    CajaInstrumentalLlu({ -12.8f, y + 0.35f, 5.6f }, { 1.0f, 0.7f, 0.7f }, 12.0f);
    CajaInstrumentalLlu({ -12.7f, y + 0.95f, 5.5f }, { 0.7f, 0.5f, 0.55f }, -8.0f);

    // Derecha: mastil meteorologico, telescopio blanco, cajas y banco.
    Vector3 baliza = MastilMeteoLlu({ 11.3f, y, 3.6f }, t);
    if (n < maxLuces) luces[n++] = baliza;
    PrismaLlu({ 11.8f, y, -2.6f }, 0.5f, 0.42f, 0.9f, 8, PIEDRA_LLU);
    TuboLlu({ 11.8f, y + 0.9f, -2.6f }, { 11.8f, y + 1.3f, -2.6f }, 0.16f, 0.16f, Color{ 60, 62, 76, 255 }, 8);
    {
        Vector3 dir = Vector3Normalize({ 0.5f, 0.8f, -0.3f });
        Vector3 c = { 11.8f, y + 1.55f, -2.6f };
        TuboLlu(Vector3Subtract(c, Vector3Scale(dir, 0.6f)), Vector3Add(c, Vector3Scale(dir, 1.3f)), 0.28f, 0.28f, Color{ 232, 236, 246, 255 }, 12);
        TuboLlu(Vector3Add(c, Vector3Scale(dir, 1.1f)), Vector3Add(c, Vector3Scale(dir, 1.35f)), 0.31f, 0.31f, Color{ 52, 54, 70, 255 }, 12);
    }
    CajaInstrumentalLlu({ 12.9f, y + 0.3f, 0.6f }, { 0.8f, 0.6f, 1.1f }, 20.0f);
    CuboLlu({ 10.6f, y + 0.45f, -5.0f }, { 1.6f, 0.08f, 0.5f }, MADERA_LLU);
    CuboLlu({ 10.0f, y + 0.22f, -5.0f }, { 0.1f, 0.44f, 0.4f }, Color{ 50, 52, 66, 255 });
    CuboLlu({ 11.2f, y + 0.22f, -5.0f }, { 0.1f, 0.44f, 0.4f }, Color{ 50, 52, 66, 255 });

    // Fondo: baranda de bronce al borde del precipicio.
    for (int k = 0; k < 13; k++)
    {
        float x = -7.2f + 1.2f * (float)k;
        VigaLlu({ x, y, -8.3f }, { x, y + 0.95f, -8.3f }, 0.07f, BRONCE_OSCURO_LLU);
    }

    VigaLlu({ -7.2f, y + 0.9f, -8.3f }, { 7.2f, y + 0.9f, -8.3f }, 0.06f, BRONCE_LLU);
    VigaLlu({ -7.2f, y + 0.5f, -8.3f }, { 7.2f, y + 0.5f, -8.3f }, 0.05f, BRONCE_OSCURO_LLU);

    // Pinos y rocas nevadas (a la mitad en calidad media).
    const Vector3 pinos[8] =
    {
        { -14.6f, -1.2f, 2.8f }, { -15.0f, 0.6f, 1.8f }, { -13.6f, -6.4f, 2.4f }, { -15.4f, -4.4f, 3.0f },
        { 14.8f, -3.4f, 2.6f }, { 15.2f, 1.4f, 2.0f }, { 13.8f, -7.2f, 2.2f }, { -9.6f, -7.8f, 1.8f }
    };

    for (int k = 0; k < 8; k += paso)
    {
        PinoLlu({ pinos[k].x, AlturaTerrenoLlu(pinos[k].x, pinos[k].y), pinos[k].y }, pinos[k].z, k);
    }

    const Vector3 rocas[10] =
    {
        { -10.4f, 7.0f, 0.9f }, { 10.6f, 7.0f, 0.8f }, { -13.6f, 0.0f, 1.1f }, { 13.4f, -1.0f, 1.2f },
        { -12.8f, -6.0f, 0.9f }, { 8.6f, -7.6f, 0.8f }, { 15.4f, 4.4f, 1.3f }, { -15.6f, 5.4f, 1.2f },
        { 5.0f, 7.2f, 0.7f }, { -5.6f, 7.2f, 0.6f }
    };

    for (int k = 0; k < 10; k += paso)
    {
        RocaLlu({ rocas[k].x, AlturaTerrenoLlu(rocas[k].x, rocas[k].y) - 0.05f, rocas[k].y }, rocas[k].z, k);
    }

    return n;
}


//==================================================
// VISUAL: OBSTACULOS (cada uno sobre su caja de colision exacta)
//==================================================

enum VestidoObstaculoLlu
{
    VESTIDO_LLU_COLUMNA = 0,
    VESTIDO_LLU_TELESCOPIO,
    VESTIDO_LLU_ARMILAR,
    VESTIDO_LLU_OBELISCO,
    VESTIDO_LLU_GLOBO,
    VESTIDO_LLU_CAJAS,
    VESTIDO_LLU_CONSOLA
};


static VestidoObstaculoLlu VestidoObstaculoLluvia(int tema, int indice, const ObstaculoLluvia& o)
{
    if (o.redondo)
    {
        if (o.tamano.x >= 2.0f) return VESTIDO_LLU_TELESCOPIO;
        if (o.tamano.y >= 2.5f) return VESTIDO_LLU_COLUMNA;
        return VESTIDO_LLU_ARMILAR;
    }

    if (o.tamano.x <= 1.0f) return VESTIDO_LLU_OBELISCO;
    if (tema == 0) return VESTIDO_LLU_GLOBO;
    if (tema == 3 || (tema == 1 && indice == 2)) return VESTIDO_LLU_CAJAS;
    return VESTIDO_LLU_CONSOLA;
}


// Zocalo que ocupa exactamente la huella de la colision.
static void ZocaloLlu(const ObstaculoLluvia& o, float alto)
{
    CuboLlu({ o.posicion.x, alto * 0.5f, o.posicion.z }, { o.tamano.x, alto, o.tamano.z }, PIEDRA_OSCURA_LLU);
    CuboLlu({ o.posicion.x, alto + 0.03f, o.posicion.z }, { o.tamano.x - 0.08f, 0.06f, o.tamano.z - 0.08f }, PIEDRA_LLU);
}


// MODELO FUTURO: columna, pedestal del telescopio, globo celeste, obelisco,
// consola de instrumentos y cajas (escalados a la caja de colision).
static void DibujarObstaculoLlu(const MinijuegoLluviaApilada& m, int indice, float t)
{
    const ObstaculoLluvia& o = m.obstaculos[indice];
    float x = o.posicion.x;
    float z = o.posicion.z;
    float w = o.tamano.x;
    float d = o.tamano.z;
    float alto = o.tamano.y;

    switch (VestidoObstaculoLluvia(m.tema, indice, o))
    {
    case VESTIDO_LLU_COLUMNA:
    {
        ZocaloLlu(o, 0.34f);
        PrismaLlu({ x, 0.40f, z }, w * 0.48f, w * 0.42f, 0.14f, 16, PIEDRA_CLARA_LLU);
        EstriadoLlu({ x, 0.54f, z }, w * 0.38f, w * 0.34f, alto - 0.54f - 0.40f, 16, PIEDRA_CLARA_LLU);
        PrismaLlu({ x, alto - 0.40f, z }, w * 0.34f, w * 0.48f, 0.16f, 16, PIEDRA_LLU);
        CuboLlu({ x, alto - 0.12f, z }, { w, 0.24f, d }, PIEDRA_CLARA_LLU);
        TuboLlu({ x, alto, z }, { x, alto + 0.3f, z }, 0.07f, 0.05f, BRONCE_OSCURO_LLU, 6);
        EsferaLlu({ x, alto + 0.42f, z }, 0.17f, BRONCE_LLU, 10, 3);
        AroLlu({ x, alto + 0.42f, z }, { std::cos(t * 0.8f + x), 0.0f, std::sin(t * 0.8f + x) }, { 0.0f, 1.0f, 0.0f }, 0.3f, 0.04f, BRONCE_LLU, 16);
        break;
    }
    case VESTIDO_LLU_TELESCOPIO:
    {
        ZocaloLlu(o, 0.4f);
        PrismaLlu({ x, 0.46f, z }, w * 0.47f, w * 0.44f, alto - 0.46f - 0.26f, 16, PIEDRA_LLU);
        PrismaLlu({ x, 1.4f, z }, w * 0.465f, w * 0.465f, 0.12f, 16, BRONCE_OSCURO_LLU);
        PrismaLlu({ x, alto - 0.26f, z }, w * 0.5f, w * 0.5f, 0.26f, 16, PIEDRA_CLARA_LLU);

        // Montura ecuatorial con el tubo que barre el cielo despacio.
        float acimut = 2.4f + 0.5f * std::sin(t * 0.15f);
        float elevacion = 0.62f;
        Vector3 dir = { std::cos(acimut) * std::cos(elevacion), std::sin(elevacion), std::sin(acimut) * std::cos(elevacion) };
        Vector3 pivote = { x, alto + 0.75f, z };
        PrismaLlu({ x, alto, z }, 0.42f, 0.3f, 0.45f, 10, Color{ 64, 66, 82, 255 });
        VigaLlu({ x - 0.3f, alto + 0.4f, z }, { x - 0.3f, alto + 0.9f, z }, 0.12f, Color{ 64, 66, 82, 255 });
        VigaLlu({ x + 0.3f, alto + 0.4f, z }, { x + 0.3f, alto + 0.9f, z }, 0.12f, Color{ 64, 66, 82, 255 });
        TuboLlu(Vector3Subtract(pivote, Vector3Scale(dir, 0.9f)), Vector3Add(pivote, Vector3Scale(dir, 1.7f)), 0.3f, 0.34f, Color{ 230, 234, 244, 255 }, 12);
        TuboLlu(Vector3Add(pivote, Vector3Scale(dir, 1.5f)), Vector3Add(pivote, Vector3Scale(dir, 1.75f)), 0.37f, 0.37f, BRONCE_LLU, 12);
        TuboLlu(Vector3Subtract(pivote, Vector3Scale(dir, 0.9f)), Vector3Subtract(pivote, Vector3Scale(dir, 1.1f)), 0.1f, 0.1f, Color{ 52, 54, 70, 255 }, 8);
        break;
    }
    case VESTIDO_LLU_ARMILAR:
    {
        ZocaloLlu(o, 0.3f);
        PrismaLlu({ x, 0.36f, z }, w * 0.34f, w * 0.24f, alto - 0.36f - 0.16f, 10, PIEDRA_LLU);
        PrismaLlu({ x, alto - 0.16f, z }, w * 0.42f, w * 0.42f, 0.16f, 12, PIEDRA_CLARA_LLU);
        TuboLlu({ x, alto, z }, { x, alto + 0.2f, z }, 0.06f, 0.05f, BRONCE_OSCURO_LLU, 6);
        EsferaArmilarLlu({ x, alto + 0.62f, z }, 0.5f, t + x);
        break;
    }
    case VESTIDO_LLU_OBELISCO:
    {
        ZocaloLlu(o, 0.3f);
        CuboLlu({ x, 0.3f + (alto - 0.3f - 0.36f) * 0.5f, z }, { w * 0.84f, alto - 0.3f - 0.36f, d * 0.84f }, PIEDRA_CLARA_LLU);
        CuboLlu({ x, alto - 0.36f, z }, { w * 0.9f, 0.08f, d * 0.9f }, BRONCE_OSCURO_LLU);
        rlPushMatrix();
        rlTranslatef(x, alto - 0.32f, z);
        rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        PrismaLlu({ 0.0f, 0.0f, 0.0f }, w * 0.6f, 0.0f, 0.62f, 4, BRONCE_LLU);
        rlPopMatrix();
        break;
    }
    case VESTIDO_LLU_GLOBO:
    {
        ZocaloLlu(o, 0.4f);
        CuboLlu({ x, 0.4f + (alto - 0.7f) * 0.5f, z }, { w * 0.82f, alto - 0.7f, d * 0.82f }, PIEDRA_LLU);
        CuboLlu({ x, alto - 0.15f, z }, { w, 0.3f, d }, PIEDRA_CLARA_LLU);
        CuboLlu({ x, 1.2f, z + d * 0.41f + 0.01f }, { w * 0.5f, 0.5f, 0.02f }, BRONCE_OSCURO_LLU);

        // Globo celeste azul noche con meridiano de bronce.
        Vector3 c = { x, alto + 0.72f, z };
        TuboLlu({ x, alto, z }, { x, alto + 0.2f, z }, 0.18f, 0.08f, BRONCE_OSCURO_LLU, 8);
        EsferaLlu(c, 0.52f, Color{ 40, 56, 120, 255 }, 14, 5);
        AroLlu(c, { std::cos(0.4f), std::sin(0.4f), 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.62f, 0.06f, BRONCE_LLU, 24);
        AroLlu(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.68f, 0.05f, BRONCE_OSCURO_LLU, 24);
        break;
    }
    case VESTIDO_LLU_CAJAS:
    {
        // Caja de abajo: la huella exacta; la de arriba apenas girada.
        CajaInstrumentalLlu({ x, 0.5f, z }, { w, 1.0f, d }, 0.0f);
        CajaInstrumentalLlu({ x, 1.0f + (alto - 1.0f) * 0.5f, z }, { w * 0.84f, alto - 1.0f, d * 0.84f }, 8.0f + 6.0f * (float)indice);
        CuboLlu({ x, 0.55f, z + d * 0.5f + 0.01f }, { w * 0.4f, 0.3f, 0.02f }, Color{ 226, 214, 182, 255 });
        break;
    }
    default:
    {
        // Consola de instrumentos: mueble con pupitre de cartas y diales.
        CuboLlu({ x, 0.5f, z }, { w, 1.0f, d }, Color{ 92, 70, 58, 255 });
        CuboLlu({ x, 1.02f, z }, { w + 0.06f, 0.06f, d + 0.06f }, BRONCE_OSCURO_LLU);
        float fondo = d * 0.42f;
        CuboLlu({ x, 1.0f + (alto - 1.0f) * 0.5f, z - d * 0.5f + fondo * 0.5f }, { w, alto - 1.0f, fondo }, Color{ 82, 64, 54, 255 });
        CuboLlu({ x, alto - 0.04f, z - d * 0.5f + fondo * 0.5f }, { w + 0.06f, 0.08f, fondo + 0.06f }, BRONCE_OSCURO_LLU);

        // Pupitre inclinado con una carta celeste.
        rlBegin(RL_QUADS);
        float y0 = 1.06f;
        float y1 = 1.36f;
        float zFrente = z + d * 0.5f - 0.06f;
        float zFondo = z - d * 0.5f + fondo;
        QuadLlu({ x - w * 0.46f, y0, zFrente }, { x + w * 0.46f, y0, zFrente }, { x + w * 0.46f, y1, zFondo }, { x - w * 0.46f, y1, zFondo }, Color{ 228, 216, 184, 255 });
        rlEnd();

        // Diales de bronce sobre el frente del armario.
        for (int k = 0; k < 3; k++)
        {
            float dx = (-1.0f + (float)k) * w * 0.28f;
            Vector3 c = { x + dx, 1.0f + (alto - 1.0f) * 0.6f, zFondo + 0.02f };
            ResplandorLlu(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.2f, Color{ 236, 230, 206, 255 }, 1.0f);
            AroLlu(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.2f, 0.04f, BRONCE_LLU, 14);
            float a = t * (0.6f + 0.3f * (float)k) + (float)k;
            VigaLlu(c, { c.x + std::cos(a) * 0.15f, c.y + std::sin(a) * 0.15f, c.z + 0.01f }, 0.025f, Color{ 60, 40, 40, 255 });
        }
        break;
    }
    }
}


//==================================================
// VISUAL: BASES, FRASCOS, MARCADORES Y PIEZAS
//==================================================

// Estrella plana de cinco puntas (relleno) en el plano (u, v).
static void EstrellaPlanaLlu(Vector3 c, Vector3 u, Vector3 v, float rExterior, float rInterior, Color color)
{
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 10; k++)
    {
        float a0 = 1.5708f + 6.2832f * (float)k / 10.0f;
        float a1 = 1.5708f + 6.2832f * (float)(k + 1) / 10.0f;
        float r0 = (k % 2 == 0) ? rExterior : rInterior;
        float r1 = (k % 2 == 0) ? rInterior : rExterior;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a0) * r0), Vector3Scale(v, std::sin(a0) * r0)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a1) * r1), Vector3Scale(v, std::sin(a1) * r1)));
        VerticeLlu(c, color);
        VerticeLlu(p0, color);
        VerticeLlu(p1, color);
    }

    rlEnd();
}


// Estrella con espesor (pieza rara).
static void EstrellaSolidaLlu(Vector3 c, Vector3 u, Vector3 v, float rExterior, float espesor, Color color)
{
    Vector3 n = Vector3Normalize(Vector3CrossProduct(u, v));
    Vector3 h = Vector3Scale(n, espesor * 0.5f);
    float rInterior = rExterior * 0.48f;
    Vector3 punta[10];

    for (int k = 0; k < 10; k++)
    {
        float a = 1.5708f + 6.2832f * (float)k / 10.0f;
        float r = (k % 2 == 0) ? rExterior : rInterior;
        punta[k] = Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a) * r), Vector3Scale(v, std::sin(a) * r)));
    }

    // Caras como piramides chatas: el centro sobresale y da volumen.
    Vector3 frente = Vector3Add(c, Vector3Scale(h, 1.6f));
    Vector3 dorso = Vector3Subtract(c, Vector3Scale(h, 1.6f));
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 10; k++)
    {
        Vector3 p0 = punta[k];
        Vector3 p1 = punta[(k + 1) % 10];
        Vector3 nf = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(p0, frente), Vector3Subtract(p1, frente)));
        if (Vector3DotProduct(nf, n) < 0.0f) nf = Vector3Negate(nf);
        Color cf = TonoLlu(color, LuzNormalLlu(nf) * ((k % 2 == 0) ? 1.05f : 0.9f));
        VerticeLlu(frente, cf);
        VerticeLlu(p0, cf);
        VerticeLlu(p1, cf);
        Color cd = TonoLlu(color, 0.62f);
        VerticeLlu(dorso, cd);
        VerticeLlu(p1, cd);
        VerticeLlu(p0, cd);
    }

    rlEnd();
}


// Fragmento de estrella: octaedro de cristal (chato si va en la pila).
static void CristalLlu(Vector3 c, float r, float alto, float giro, Color color)
{
    Vector3 arriba = { c.x, c.y + alto, c.z };
    Vector3 abajo = { c.x, c.y - alto, c.z };
    Vector3 ecuador[4];

    for (int k = 0; k < 4; k++)
    {
        float a = giro + 1.5708f * (float)k;
        ecuador[k] = { c.x + std::cos(a) * r, c.y, c.z + std::sin(a) * r };
    }

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 4; k++)
    {
        Vector3 p0 = ecuador[k];
        Vector3 p1 = ecuador[(k + 1) % 4];
        float am = giro + 1.5708f * ((float)k + 0.5f);
        Vector3 nArriba = Vector3Normalize({ std::cos(am) * alto, r, std::sin(am) * alto });
        Vector3 nAbajo = Vector3Normalize({ std::cos(am) * alto, -r, std::sin(am) * alto });
        Color ca = TonoLlu(MezclaLlu(color, WHITE, 0.25f), LuzNormalLlu(nArriba) * 1.08f);
        Color cb = TonoLlu(color, LuzNormalLlu(nAbajo) * 0.95f);
        VerticeLlu(arriba, ca);
        VerticeLlu(p1, ca);
        VerticeLlu(p0, ca);
        VerticeLlu(abajo, cb);
        VerticeLlu(p0, cb);
        VerticeLlu(p1, cb);
    }

    rlEnd();
}


// Meteoro: roca irregular oscura con grietas al rojo (el halo va aparte).
static void MeteoroLlu(Vector3 c, float r, float giro)
{
    const int ANILLOS = 5;
    const int LADOS = 8;
    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        float f0 = -1.5708f + 3.1416f * (float)a / (float)ANILLOS;
        float f1 = -1.5708f + 3.1416f * (float)(a + 1) / (float)ANILLOS;

        for (int k = 0; k < LADOS; k++)
        {
            float t0 = giro + 6.2832f * (float)k / (float)LADOS;
            float t1 = giro + 6.2832f * (float)(k + 1) / (float)LADOS;
            float r00 = r * (0.82f + 0.3f * Hash01Llu(a, k));
            float r01 = r * (0.82f + 0.3f * Hash01Llu(a, (k + 1) % LADOS));
            float r10 = r * (0.82f + 0.3f * Hash01Llu(a + 1, k));
            float r11 = r * (0.82f + 0.3f * Hash01Llu(a + 1, (k + 1) % LADOS));
            if (a == 0) { r00 = r; r01 = r; }
            if (a == ANILLOS - 1) { r10 = r; r11 = r; }
            float fm = (f0 + f1) * 0.5f;
            float tm = (t0 + t1) * 0.5f;
            Vector3 n = { std::cos(fm) * std::cos(tm), std::sin(fm), std::cos(fm) * std::sin(tm) };
            bool grieta = Hash01Llu(a * 13 + 5, k) > 0.8f;
            Color base = grieta ? Color{ 255, 132, 56, 255 } : TonoLlu(Color{ 74, 58, 54, 255 }, LuzNormalLlu(n));

            QuadLlu(
                { c.x + r00 * std::cos(f0) * std::cos(t0), c.y + r00 * std::sin(f0), c.z + r00 * std::cos(f0) * std::sin(t0) },
                { c.x + r10 * std::cos(f1) * std::cos(t0), c.y + r10 * std::sin(f1), c.z + r10 * std::cos(f1) * std::sin(t0) },
                { c.x + r11 * std::cos(f1) * std::cos(t1), c.y + r11 * std::sin(f1), c.z + r11 * std::cos(f1) * std::sin(t1) },
                { c.x + r01 * std::cos(f0) * std::cos(t1), c.y + r01 * std::sin(f0), c.z + r01 * std::cos(f0) * std::sin(t1) },
                base
            );
        }
    }

    rlEnd();
}


// MODELO FUTURO: fragmento de estrella, estrella dorada y meteoro.
static void DibujarPiezaLlu(TipoPiezaLluvia tipo, Vector3 centro, float escala, float giro, bool enPila)
{
    float r = 0.30f * escala;

    if (tipo == PIEZA_LLUVIA_RARA)
    {
        if (enPila)
        {
            EstrellaSolidaLlu(centro, { std::cos(giro), 0.0f, std::sin(giro) }, { -std::sin(giro), 0.0f, std::cos(giro) }, r * 1.35f, r * 0.42f, DORADA_LLU);
        }
        else
        {
            EstrellaSolidaLlu(centro, { std::cos(giro), 0.0f, std::sin(giro) }, { 0.0f, 1.0f, 0.0f }, r * 1.35f, r * 0.42f, DORADA_LLU);
        }
    }
    else if (tipo == PIEZA_LLUVIA_PELIGROSA)
    {
        MeteoroLlu(centro, r * 1.1f, giro);
    }
    else
    {
        CristalLlu(centro, r * 1.05f, enPila ? r * 0.62f : r * 1.35f, giro, FRAGMENTO_LLU);
    }
}


static Color ColorTipoLlu(TipoPiezaLluvia tipo)
{
    if (tipo == PIEZA_LLUVIA_PELIGROSA) return Color{ 255, 64, 44, 255 };
    if (tipo == PIEZA_LLUVIA_RARA) return DORADA_LLU;
    return Color{ 196, 244, 255, 255 };
}


static void DibujarBasesLlu(
    const MinijuegoLluviaApilada& m,
    const Participante participantes[],
    int limite
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 b = ObtenerBaseLluvia(i);
        Color color = participantes[i].color;
        float R = RADIO_BASE_LLUVIA;

        // Base de piedra al ras con el anillo y la estrella del jugador.
        PrismaLlu({ b.x, 0.0f, b.z }, R + 0.06f, R, 0.05f, 32, MezclaLlu(PIEDRA_CLARA_LLU, color, 0.12f));
        AnilloLlu({ b.x, 0.052f, b.z }, 0.0f, R * 0.84f, MezclaLlu(LOSA_LLU, color, 0.30f), 32);
        AnilloLlu({ b.x, 0.054f, b.z }, R * 0.86f, R - 0.02f, color, 32);
        AnilloLlu({ b.x, 0.056f, b.z }, R * 0.56f, R * 0.62f, MezclaLlu(color, WHITE, 0.2f), 32);
        EstrellaPlanaLlu({ b.x, 0.058f, b.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, R * 0.46f, R * 0.2f, MezclaLlu(color, WHITE, 0.35f));

        // Cuatro mojones bajos con la luz del jugador.
        for (int k = 0; k < 4; k++)
        {
            float a = 0.7854f + 1.5708f * (float)k;
            Vector3 p = { b.x + std::cos(a) * (R + 0.12f), 0.0f, b.z + std::sin(a) * (R + 0.12f) };
            PrismaLlu(p, 0.1f, 0.08f, 0.32f, 6, PIEDRA_CLARA_LLU);
            EsferaLlu({ p.x, 0.38f, p.z }, 0.07f, MezclaLlu(color, WHITE, 0.4f), 6, 2);
        }
    }
}


static Vector3 PosicionFrascoLlu(int i)
{
    Vector3 b = ObtenerBaseLluvia(i);
    return { b.x < 0.0f ? -8.75f : 8.75f, 1.12f, b.z };
}


static float LlenadoFrascoLlu(const MinijuegoLluviaApilada& m, int i)
{
    return Limitar01Llu((float)m.estadosJugadores[i].aseguradas / 24.0f);
}


// Frasco de estrellas sobre el muro: se llena con lo asegurado.
static void DibujarFrascosLlu(const MinijuegoLluviaApilada& m, const Participante participantes[], int limite)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 p = PosicionFrascoLlu(i);
        Color color = participantes[i].color;
        PrismaLlu(p, 0.26f, 0.24f, 0.06f, 12, BRONCE_OSCURO_LLU);
        float lleno = LlenadoFrascoLlu(m, i);

        if (lleno > 0.0f)
        {
            PrismaLlu({ p.x, p.y + 0.06f, p.z }, 0.19f, 0.19f, 0.58f * lleno, 12, MezclaLlu(FRAGMENTO_LLU, DORADA_LLU, 0.35f));
        }

        PrismaLlu({ p.x, p.y + 0.68f, p.z }, 0.17f, 0.2f, 0.08f, 12, color);
        PrismaLlu({ p.x, p.y + 0.76f, p.z }, 0.2f, 0.12f, 0.06f, 12, TonoLlu(color, 0.8f));
    }
}


static void DibujarVidrioFrascosLlu(const MinijuegoLluviaApilada& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 p = PosicionFrascoLlu(i);
        PrismaLlu({ p.x, p.y + 0.06f, p.z }, 0.23f, 0.21f, 0.62f, 12, Color{ 200, 226, 255, 70 }, false);
    }
}


// Donde va a caer cada pieza: un blanco que crece y se llena.
static void DibujarMarcadoresLlu(const MinijuegoLluviaApilada& m, float t)
{
    for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
    {
        const PiezaCayendoLluvia& pieza = m.piezasCayendo[n];

        // Al terminar la logica las congela en el aire: no se muestran.
        if (!pieza.activa || m.fase == FASE_LLUVIA_TERMINADO)
        {
            continue;
        }

        float avance = Limitar01Llu(pieza.tiempo / pieza.duracion);
        float radio = 0.30f + 0.70f * avance;
        Color color = ColorTipoLlu(pieza.tipo);
        Vector3 c = { pieza.x, 0.03f + 0.0005f * (float)n, pieza.z };

        ResplandorSueloLlu(c, radio * 1.05f, AlfaLlu(MezclaLlu(color, JUNTA_LLU, 0.2f), 70.0f + 110.0f * avance));
        AnilloLlu({ c.x, c.y + 0.002f, c.z }, radio * 0.84f, radio, AlfaLlu(color, 230.0f), 28);
        AnilloLlu({ c.x, c.y + 0.002f, c.z }, radio * 0.42f, radio * 0.5f, AlfaLlu(color, 170.0f), 20);

        if (pieza.tipo == PIEZA_LLUVIA_PELIGROSA)
        {
            // Una cruz roja: no se confunde nunca con una pieza buena.
            float pulso = 0.75f + 0.25f * std::sin(t * 16.0f);
            float l = radio * 0.72f;
            float g = 0.06f + 0.04f * avance;
            Color cruz = AlfaLlu(color, 255.0f * pulso);
            rlBegin(RL_QUADS);

            for (int k = 0; k < 2; k++)
            {
                float a = 0.7854f + 1.5708f * (float)k;
                Vector3 d = { std::cos(a), 0.0f, std::sin(a) };
                Vector3 p = { -d.z * g, 0.0f, d.x * g };
                float y = c.y + 0.004f;
                QuadLlu(
                    { c.x - d.x * l - p.x, y, c.z - d.z * l - p.z },
                    { c.x - d.x * l + p.x, y, c.z - d.z * l + p.z },
                    { c.x + d.x * l + p.x, y, c.z + d.z * l + p.z },
                    { c.x + d.x * l - p.x, y, c.z + d.z * l - p.z },
                    cruz
                );
            }

            rlEnd();
        }
        else if (pieza.tipo == PIEZA_LLUVIA_RARA)
        {
            EstrellaPlanaLlu({ c.x, c.y + 0.004f, c.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, radio * 0.38f, radio * 0.16f, AlfaLlu(color, 230.0f));
        }
    }

    // Quemaduras de los meteoros.
    for (int k = 0; k < MAX_MARCAS_LLUVIA; k++)
    {
        const MarcaImpactoLluvia& marca = m.marcas[k];

        if (marca.vida <= 0.0f)
        {
            continue;
        }

        ResplandorSueloLlu({ marca.x, 0.025f, marca.z }, 0.85f, Color{ 26, 22, 30, (unsigned char)(170.0f * marca.vida) }, 1.15f);
    }
}


static void DibujarJugadoresLlu(
    const MinijuegoLluviaApilada& m,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    bool mostrarDebug
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];
        Color color = participantes[i].color;

        if (!jugador.cayendo)
        {
            AnilloLlu({ jugador.posicion.x, 0.07f, jugador.posicion.z }, 0.46f, 0.56f, AlfaLlu(color, 220.0f), 22);
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorModeloCompartido(jugador, visual);

        // La pila se balancea mas cuanto mas alta y tras un golpe.
        const EstadoJugadorLluvia& estado = m.estadosJugadores[i];

        if (!jugador.cayendo)
        {
            float inicioY = AlturaPilaVisualLlu(jugador, 0);
            float amplitud = 0.012f * (0.4f + 0.12f * (float)estado.pila) * (1.0f + 2.5f * m.golpePila[i]);

            for (int k = 0; k < estado.pila; k++)
            {
                float altura = (float)(k + 1);
                float desvioX = std::sin(m.tiempoAnimacion * 2.6f + (float)k * 0.55f + (float)i) * amplitud * altura * altura * 0.5f;
                float desvioZ = std::cos(m.tiempoAnimacion * 2.1f + (float)k * 0.7f) * amplitud * altura * altura * 0.4f;

                desvioX = LimitarLluvia(desvioX, -0.55f, 0.55f);
                desvioZ = LimitarLluvia(desvioZ, -0.55f, 0.55f);

                DibujarPiezaLlu(
                    estado.rara[k] ? PIEZA_LLUVIA_RARA : PIEZA_LLUVIA_NORMAL,
                    {
                        jugador.posicion.x + desvioX,
                        inicioY + (float)k * ALTURA_PIEZA_EN_PILA_LLUVIA,
                        jugador.posicion.z + desvioZ
                    },
                    0.85f,
                    (float)k * 0.7f + (float)i,
                    true
                );
            }
        }

        if (mostrarDebug && !jugador.cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }
}


static void DibujarPiezasLlu(const MinijuegoLluviaApilada& m, float t)
{
    for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
    {
        const PiezaCayendoLluvia& pieza = m.piezasCayendo[n];

        // Al terminar la logica las congela en el aire: no se muestran.
        if (!pieza.activa || m.fase == FASE_LLUVIA_TERMINADO)
        {
            continue;
        }

        DibujarPiezaLlu(pieza.tipo, { pieza.x, pieza.altura + 0.3f, pieza.z }, 1.25f, t * 3.0f + (float)n, false);
    }

    for (int n = 0; n < MAX_PIEZAS_SUELTAS_LLUVIA; n++)
    {
        const PiezaSueltaLluvia& pieza = m.piezasSueltas[n];

        if (!pieza.activa)
        {
            continue;
        }

        // Parpadean cuando estan por desaparecer.
        if (pieza.tiempoVida < 2.0f && ((int)(t * 10.0f)) % 2 == 0)
        {
            continue;
        }

        Color color = pieza.rara ? DORADA_LLU : FRAGMENTO_LLU;
        AnilloLlu({ pieza.posicion.x, 0.035f, pieza.posicion.z }, 0.34f, 0.44f, AlfaLlu(color, 210.0f), 20);
        DibujarPiezaLlu(pieza.rara ? PIEZA_LLUVIA_RARA : PIEZA_LLUVIA_NORMAL, pieza.posicion, 1.0f, t * 2.0f + (float)n, false);
    }
}


static void DibujarParticulasVisualesLlu(const MinijuegoLluviaApilada& m)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_LLUVIA; i++)
    {
        const ParticulaVisualLluvia& p = m.particulasVisuales[i];

        if (!p.activa || p.brillo)
        {
            continue;
        }

        float s = p.tamano * Limitar01Llu(p.vida / 0.3f + 0.3f);
        CuboGiradoLlu(p.posicion, { s * 2.0f, s * 0.6f, s * 1.0f }, p.giro, p.color);
    }
}


//==================================================
// VISUAL: LUCES FALSAS (pasada aditiva al final del 3D)
//==================================================

static void DibujarLucesLlu(
    const MinijuegoLluviaApilada& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const Vector3 luces[],
    int cantidadLuces,
    bool decoracion,
    bool efectos
)
{
    const float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    if (efectos)
    {
        // Faroles de las pilastras: halo y charco tibio en las losas.
        for (int k = 0; k < 4; k++)
        {
            float x = (k % 2 == 0) ? -8.75f : 8.75f;
            float z = (k < 2) ? -5.75f : 5.75f;
            float titileo = 0.92f + 0.08f * std::sin(t * 9.0f + (float)k * 1.7f) * std::sin(t * 4.3f + (float)k);
            Vector3 foco = { x, 1.8f, z };
            ResplandorFrenteLlu(foco, 1.5f * titileo, Color{ 255, 190, 110, 70 });
            ResplandorFrenteLlu(foco, 0.5f * titileo, Color{ 255, 236, 190, 160 });

            for (int r = 0; r < 3; r++)
            {
                float charco = z < 0.0f ? 0.7f : 1.0f;
                ResplandorSueloLlu({ x * 0.94f, 0.02f + 0.002f * (float)r, z * 0.9f }, (1.4f + 0.9f * (float)r) * titileo * charco, Color{ 255, 184, 110, 20 });
            }
        }

        // Farolas y baliza del mastil.
        for (int k = 0; k < cantidadLuces; k++)
        {
            Vector3 luz = luces[k];
            bool baliza = k == cantidadLuces - 1;

            if (baliza)
            {
                if (std::fmod(t, 1.4f) < 0.45f)
                {
                    ResplandorFrenteLlu(luz, 0.9f, Color{ 255, 60, 50, 150 });
                    ResplandorFrenteLlu(luz, 0.25f, Color{ 255, 200, 190, 220 });
                }
                continue;
            }

            float titileo = 0.94f + 0.06f * std::sin(t * 7.0f + (float)k * 2.1f);
            ResplandorFrenteLlu(luz, 1.4f * titileo, Color{ 255, 190, 110, 64 });
            ResplandorFrenteLlu(luz, 0.45f * titileo, Color{ 255, 236, 190, 150 });

            for (int r = 0; r < 3; r++)
            {
                ResplandorSueloLlu({ luz.x, Y_LLANO_LLU + 0.05f + 0.01f * (float)r, luz.z }, (1.2f + 0.9f * (float)r) * titileo, Color{ 255, 184, 110, 26 });
            }
        }

        // Ventanas de la fachada: halo sobre la piedra.
        for (int k = 0; k < CANTIDAD_VENTANAS_LLU; k++)
        {
            float x = -7.2f + 2.4f * (float)k;
            ResplandorLlu({ x, -0.64f, BORDE_Z_LLU + 0.06f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.75f, Color{ 255, 170, 90, 60 }, 1.2f);
            ResplandorSueloLlu({ x, Y_LLANO_LLU + 0.05f, BORDE_Z_LLU + 0.7f }, 0.9f, Color{ 255, 170, 90, 28 }, 1.3f);
        }

        if (decoracion)
        {
            // Interior de la cupula por la ranura y la puerta encendida.
            Vector3 c = CUPULA_LLU;
            float giro = GiroCupulaLlu(t);

            for (int k = 0; k < 3; k++)
            {
                float f = 0.3f + 0.4f * (float)k;
                Vector3 p = { c.x + RADIO_CUPULA_LLU * 0.9f * std::cos(f) * std::cos(giro), c.y + ALTO_TAMBOR_LLU + RADIO_CUPULA_LLU * 0.9f * std::sin(f), c.z + RADIO_CUPULA_LLU * 0.9f * std::cos(f) * std::sin(giro) };
                ResplandorFrenteLlu(p, 1.4f, Color{ 255, 160, 90, 36 });
            }

            float aPuerta = std::atan2(-c.z, -c.x);
            Vector3 puerta = { c.x + std::cos(aPuerta) * (RADIO_CUPULA_LLU + 0.3f), c.y + 1.4f, c.z + std::sin(aPuerta) * (RADIO_CUPULA_LLU + 0.3f) };
            ResplandorFrenteLlu(puerta, 1.6f, Color{ 255, 176, 100, 40 });

            // Baliza roja en la antena.
            if (std::fmod(t + 0.7f, 1.8f) < 0.6f)
            {
                Vector3 punta = PuntaAntenaLlu();
                ResplandorFrenteLlu(punta, 0.9f, Color{ 255, 60, 50, 140 });
            }
        }
    }

    // Bases: brillo del color del jugador, mas fuerte si esta por asegurar.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 b = ObtenerBaseLluvia(i);
        Color color = participantes[i].color;
        bool listo = m.estadosJugadores[i].pila > 0 && JugadorEnBaseLluvia(i, jugadores[i]);
        float pulso = 0.5f + 0.5f * std::sin(t * 8.0f);
        float fuerza = listo ? 0.6f + 0.4f * pulso : 0.35f + 0.08f * std::sin(t * 2.0f + (float)i);
        ResplandorSueloLlu({ b.x, 0.07f, b.z }, RADIO_BASE_LLUVIA * 1.25f, AlfaLlu(color, 70.0f * fuerza));

        for (int k = 0; k < 4; k++)
        {
            float a = 0.7854f + 1.5708f * (float)k;
            Vector3 p = { b.x + std::cos(a) * (RADIO_BASE_LLUVIA + 0.12f), 0.38f, b.z + std::sin(a) * (RADIO_BASE_LLUVIA + 0.12f) };
            ResplandorFrenteLlu(p, 0.32f, AlfaLlu(color, 120.0f));
        }

        // Columna de luz al asegurar.
        float destello = m.destelloBase[i];

        if (destello > 0.0f)
        {
            float alto = 1.0f + (1.0f - destello) * 5.0f;
            DrawCylinderEx({ b.x, 0.06f, b.z }, { b.x, alto, b.z }, RADIO_BASE_LLUVIA * 0.85f, RADIO_BASE_LLUVIA * 0.5f, 20, AlfaLlu(color, 90.0f * destello));
            DrawCylinderEx({ b.x, 0.06f, b.z }, { b.x, alto * 0.7f, b.z }, RADIO_BASE_LLUVIA * 0.4f, 0.1f, 14, AlfaLlu(DORADA_LLU, 110.0f * destello));
        }

        // Frasco: brilla segun lo asegurado.
        float lleno = LlenadoFrascoLlu(m, i);

        if (lleno > 0.0f)
        {
            Vector3 f = PosicionFrascoLlu(i);
            ResplandorFrenteLlu({ f.x, f.y + 0.06f + 0.3f * lleno, f.z }, 0.35f + 0.4f * lleno, AlfaLlu(MezclaLlu(FRAGMENTO_LLU, DORADA_LLU, 0.35f), 90.0f + 60.0f * destello));
        }
    }

    // Piezas en el aire: halo, estela de estrella fugaz y hilo al blanco.
    for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
    {
        const PiezaCayendoLluvia& pieza = m.piezasCayendo[n];

        // Al terminar la logica las congela en el aire: no se muestran.
        if (!pieza.activa || m.fase == FASE_LLUVIA_TERMINADO)
        {
            continue;
        }

        float avance = Limitar01Llu(pieza.tiempo / pieza.duracion);
        Vector3 c = { pieza.x, pieza.altura + 0.3f, pieza.z };
        Color color = pieza.tipo == PIEZA_LLUVIA_PELIGROSA ? Color{ 255, 150, 60, 255 } : (pieza.tipo == PIEZA_LLUVIA_RARA ? DORADA_LLU : FRAGMENTO_LLU);
        float radio = pieza.tipo == PIEZA_LLUVIA_NORMAL ? 0.75f : 1.0f;
        ResplandorFrenteLlu(c, radio, AlfaLlu(color, 90.0f));
        ResplandorFrenteLlu(c, radio * 0.4f, AlfaLlu(MezclaLlu(color, WHITE, 0.5f), 150.0f));

        float largo = 0.8f + 3.2f * avance;
        float ancho = pieza.tipo == PIEZA_LLUVIA_PELIGROSA ? 0.38f : 0.24f;
        rlBegin(RL_QUADS);
        VerticeLlu({ c.x - ancho, c.y, c.z }, AlfaLlu(color, 120.0f));
        VerticeLlu({ c.x + ancho, c.y, c.z }, AlfaLlu(color, 120.0f));
        VerticeLlu({ c.x + ancho * 0.2f, c.y + largo, c.z - largo * 0.15f }, AlfaLlu(color, 0.0f));
        VerticeLlu({ c.x - ancho * 0.2f, c.y + largo, c.z - largo * 0.15f }, AlfaLlu(color, 0.0f));

        // Hilo tenue entre la pieza y su blanco en el piso.
        VerticeLlu({ c.x - 0.03f, 0.05f, c.z }, AlfaLlu(color, 34.0f));
        VerticeLlu({ c.x + 0.03f, 0.05f, c.z }, AlfaLlu(color, 34.0f));
        VerticeLlu({ c.x + 0.03f, c.y, c.z }, AlfaLlu(color, 6.0f));
        VerticeLlu({ c.x - 0.03f, c.y, c.z }, AlfaLlu(color, 6.0f));
        rlEnd();

        if (pieza.tipo == PIEZA_LLUVIA_PELIGROSA)
        {
            ResplandorSueloLlu({ pieza.x, 0.05f, pieza.z }, 0.5f + 0.8f * avance, AlfaLlu(METEORO_LLU, 60.0f * avance));
        }
    }

    // Piezas sueltas y pilas: brillo chico.
    for (int n = 0; n < MAX_PIEZAS_SUELTAS_LLUVIA; n++)
    {
        const PiezaSueltaLluvia& pieza = m.piezasSueltas[n];

        if (pieza.activa)
        {
            ResplandorFrenteLlu(pieza.posicion, 0.55f, AlfaLlu(pieza.rara ? DORADA_LLU : FRAGMENTO_LLU, 80.0f));
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo || jugadores[i].cayendo)
        {
            continue;
        }

        const EstadoJugadorLluvia& estado = m.estadosJugadores[i];

        for (int k = 0; k < estado.pila; k++)
        {
            Vector3 p = { jugadores[i].posicion.x, AlturaPilaVisualLlu(jugadores[i], k), jugadores[i].posicion.z };
            ResplandorFrenteLlu(p, estado.rara[k] ? 0.5f : 0.36f, AlfaLlu(estado.rara[k] ? DORADA_LLU : FRAGMENTO_LLU, 40.0f));
        }
    }

    // Brasas de las quemaduras.
    for (int k = 0; k < MAX_MARCAS_LLUVIA; k++)
    {
        const MarcaImpactoLluvia& marca = m.marcas[k];

        if (marca.vida > 0.0f)
        {
            float v = marca.vida * marca.vida;
            ResplandorSueloLlu({ marca.x, 0.04f, marca.z }, 0.6f, AlfaLlu(METEORO_LLU, 120.0f * v), 1.15f);
        }
    }

    // Chispas que brillan.
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_LLUVIA; i++)
    {
        const ParticulaVisualLluvia& p = m.particulasVisuales[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteLlu(p.posicion, p.tamano * (1.2f + vida), AlfaLlu(p.color, 200.0f * vida));
    }

    // Escarcha que vuela con el viento de la cumbre.
    if (decoracion)
    {
        int motas = (int)(60.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < motas; k++)
        {
            float ciclo = 7.0f + 5.0f * Hash01Llu(k, 801);
            float u = std::fmod(t / ciclo + Hash01Llu(k, 802), 1.0f);
            Vector3 p =
            {
                -16.0f + 32.0f * std::fmod(Hash01Llu(k, 803) + u * 0.35f, 1.0f),
                7.0f - u * 8.0f + std::sin(t * 0.9f + (float)k) * 0.3f,
                -10.0f + 18.0f * Hash01Llu(k, 804)
            };
            float alfa = std::sin(u * 3.1416f);
            ResplandorFrenteLlu(p, 0.07f, Color{ 230, 238, 255, (unsigned char)(150.0f * alfa) });
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoLluviaApilada::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{

    int limite = LimiteJugadoresLluvia(cantidadMaxima);
    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    int alto = GetScreenHeight();

    // Entrada: durante la cuenta la camara baja desde mas arriba y atras.
    Camera3D vista = camara;
    float entrada = 0.0f;

    if (fase == FASE_LLUVIA_PREPARACION)
    {
        float u = Limitar01Llu(tiempoPreparacion / DURACION_PREPARACION_LLUVIA);
        entrada = SuaveLlu(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 5.0f * entrada;
        vista.position.z += 4.0f * entrada;
        vista.position.x += 2.0f * entrada;
    }

    DibujarCieloLlu(t, entrada, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarTerrenoLlu(decoracion ? 1 : 2);
    DibujarFachadaLlu();
    DibujarSueloTerrazaLlu();
    DibujarMurosLlu(decoracion);

    Vector3 luces[8];
    int cantidadLuces = 0;

    if (decoracion)
    {
        DibujarCupulaLlu(t, CalidadDecoracion() == CALIDAD_ALTA);
        DibujarAntenaLlu(t);
        cantidadLuces = DibujarUtileriaLlu(t, paso, luces, 8);
    }

    for (int i = 0; i < cantidadObstaculos; i++)
    {
        DibujarObstaculoLlu(*this, i, t);
    }

    DibujarBasesLlu(*this, participantes, limite);
    DibujarFrascosLlu(*this, participantes, limite);
    DibujarMarcadoresLlu(*this, t);
    DibujarPiezasLlu(*this, t);
    DibujarJugadoresLlu(*this, jugadores, limite, participantes, mostrarDebug);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA);
    DibujarParticulasVisualesLlu(*this);

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
        }
    }

    // Translucidos: vidrio de los frascos y nubes bajas.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    DibujarVidrioFrascosLlu(*this, limite);

    if (decoracion)
    {
        DibujarNubesCercanasLlu(t);
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    DibujarLucesLlu(*this, jugadores, participantes, limite, luces, cantidadLuces, decoracion, efectos);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre cada jugador (por encima de su pila).
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y = AlturaPilaVisualLlu(jugadores[i], estadosJugadores[i].pila) + 0.35f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // «+N» que sube desde la base al asegurar.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || destelloBase[i] <= 0.0f)
        {
            continue;
        }

        float u = 1.0f - destelloBase[i];
        Vector3 b = ObtenerBaseLluvia(i);
        Vector2 s = GetWorldToScreen({ b.x, 1.4f + 1.6f * u, b.z }, vista);
        char texto[16];
        std::snprintf(texto, sizeof(texto), "+%d", sumaBase[i]);
        DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, MezclaLlu(participantes[i].color, WHITE, 0.35f), Limitar01Llu(destelloBase[i] * 2.0f));
    }

    // Aviso de los ultimos segundos.
    if (fase == FASE_LLUVIA_JUGANDO && tiempoRestante <= 8.0f && tiempoRestante > 0.0f)
    {
        float latido = 0.5f + 0.5f * std::sin(t * 7.0f);
        float y = (float)alto * 0.27f;
        DibujarTextoMundoContornoUI("¡ASEGURÁ TU PILA!", (float)GetScreenWidth() * 0.5f, y, UI_TEXTO_GRANDE, Color{ 255, 226, 140, 255 }, 0.65f + 0.35f * latido);
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "LLUVIA APILADA",
        "ATRAPÁ ESTRELLAS Y ASEGURALAS EN TU BASE. ¡ESQUIVÁ LOS METEOROS!",
        resultado
    );

    hud.fase = fase == FASE_LLUVIA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_LLUVIA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_LLUVIA;
    hud.finPorTiempo = true;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        const EstadoJugadorLluvia& estado = estadosJugadores[i];
        std::snprintf(j.valor, sizeof(j.valor), "%d", estado.aseguradas);
        j.valorNumerico = estado.aseguradas;
        j.progreso = Limitar01Llu((float)estado.pila / (float)MAX_PILA_LLUVIA);
        std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), estado.aseguradas == 1 ? "%d ESTRELLA" : "%d ESTRELLAS", estado.aseguradas);

        if (fase == FASE_LLUVIA_JUGANDO && estado.pila > 0)
        {
            std::snprintf(j.estado, sizeof(j.estado), "PILA %d/%d", estado.pila, MAX_PILA_LLUVIA);
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "MOVÉ: %s  GOLPE: %s\nASEGURÁ: %s EN TU BASE",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoLluviaApilada::ObtenerResultado() const
{
    return resultado;
}
