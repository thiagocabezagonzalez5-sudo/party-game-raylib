#include "Minigames/MinijuegoLluviaApilada.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
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
// TEMATICAS (datos para escenario, colores y piezas)
//==================================================

enum FormaPiezaLluvia
{
    FORMA_LLUVIA_ESFERA = 0,
    FORMA_LLUVIA_CUBO,
    FORMA_LLUVIA_CILINDRO,
    FORMA_LLUVIA_PIRAMIDE
};


struct TemaLluvia
{
    const char* nombre;
    const char* piezas;
    const char* peligro;

    Color cielo;
    Color plano;
    Color suelo;
    Color lineas;
    Color muro;
    Color obstaculoA;
    Color obstaculoB;
    Color pieza;
    Color piezaRara;
    Color piezaPeligro;

    FormaPiezaLluvia formaPieza;
    FormaPiezaLluvia formaRara;
    FormaPiezaLluvia formaPeligro;
};


static const TemaLluvia TEMAS_LLUVIA[CANTIDAD_TEMAS_LLUVIA] =
{
    {
        "TEMPLO ANTIGUO", "IDOLOS DE JADE", "PIEDRAS DE TRAMPA",
        Color{ 120, 170, 150, 255 }, Color{ 48, 84, 52, 255 },
        Color{ 196, 170, 122, 255 }, Color{ 150, 126, 88, 255 },
        Color{ 120, 118, 100, 255 }, Color{ 170, 160, 130, 255 },
        Color{ 140, 128, 104, 255 },
        Color{ 70, 190, 120, 255 }, Color{ 255, 210, 70, 255 },
        Color{ 60, 56, 62, 255 },
        FORMA_LLUVIA_CILINDRO, FORMA_LLUVIA_ESFERA, FORMA_LLUVIA_CUBO
    },
    {
        "DESIERTO", "GEMAS DE AMBAR", "CACTUS RODANTES",
        Color{ 238, 206, 150, 255 }, Color{ 222, 190, 124, 255 },
        Color{ 214, 178, 112, 255 }, Color{ 190, 152, 90, 255 },
        Color{ 188, 140, 84, 255 }, Color{ 196, 150, 96, 255 },
        Color{ 60, 130, 70, 255 },
        Color{ 255, 150, 40, 255 }, Color{ 60, 220, 210, 255 },
        Color{ 40, 90, 50, 255 },
        FORMA_LLUVIA_PIRAMIDE, FORMA_LLUVIA_ESFERA, FORMA_LLUVIA_ESFERA
    },
    {
        "OBSERVATORIO", "FRAGMENTOS DE ESTRELLA", "METEOROS",
        Color{ 14, 16, 42, 255 }, Color{ 20, 24, 50, 255 },
        Color{ 52, 58, 86, 255 }, Color{ 96, 120, 190, 255 },
        Color{ 70, 80, 120, 255 }, Color{ 96, 108, 150, 255 },
        Color{ 80, 92, 134, 255 },
        Color{ 120, 230, 255, 255 }, Color{ 230, 160, 255, 255 },
        Color{ 96, 60, 48, 255 },
        FORMA_LLUVIA_PIRAMIDE, FORMA_LLUVIA_ESFERA, FORMA_LLUVIA_ESFERA
    },
    {
        "PUERTO", "CAJAS DE CARGA", "BARRILES EXPLOSIVOS",
        Color{ 150, 200, 232, 255 }, Color{ 40, 100, 150, 255 },
        Color{ 150, 112, 72, 255 }, Color{ 112, 80, 50, 255 },
        Color{ 120, 84, 54, 255 }, Color{ 168, 120, 70, 255 },
        Color{ 140, 100, 62, 255 },
        Color{ 214, 170, 100, 255 }, Color{ 245, 245, 235, 255 },
        Color{ 120, 36, 36, 255 },
        FORMA_LLUVIA_CUBO, FORMA_LLUVIA_ESFERA, FORMA_LLUVIA_CILINDRO
    },
    {
        "MINA", "PEPITAS DE MINERAL", "DINAMITA",
        Color{ 26, 20, 20, 255 }, Color{ 54, 44, 38, 255 },
        Color{ 100, 82, 62, 255 }, Color{ 128, 128, 136, 255 },
        Color{ 84, 66, 48, 255 }, Color{ 110, 80, 50, 255 },
        Color{ 96, 94, 100, 255 },
        Color{ 150, 160, 190, 255 }, Color{ 90, 230, 240, 255 },
        Color{ 170, 40, 36, 255 },
        FORMA_LLUVIA_PIRAMIDE, FORMA_LLUVIA_PIRAMIDE, FORMA_LLUVIA_CILINDRO
    }
};


static const TemaLluvia& ObtenerTemaLluvia(int tema)
{
    if (tema < 0 || tema >= CANTIDAD_TEMAS_LLUVIA) tema = 0;
    return TEMAS_LLUVIA[tema];
}


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
    case 0: // Templo: cuatro columnas y una estela central.
        AgregarObstaculoLluvia(minijuego, -2.8f, -2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 2.8f, -2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, -2.8f, 2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 2.8f, 2.0f, 1.2f, 3.4f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 0.0f, 0.0f, 1.8f, 2.4f, 1.8f, false);
        break;
    case 1: // Desierto: rocas y cactus.
        AgregarObstaculoLluvia(minijuego, -3.2f, -1.8f, 2.4f, 2.2f, 2.0f, false);
        AgregarObstaculoLluvia(minijuego, 3.4f, 1.6f, 1.1f, 3.0f, 1.1f, true);
        AgregarObstaculoLluvia(minijuego, 0.4f, 2.6f, 2.0f, 2.0f, 1.6f, false);
        AgregarObstaculoLluvia(minijuego, 2.8f, -2.6f, 1.0f, 2.6f, 1.0f, true);
        AgregarObstaculoLluvia(minijuego, -3.0f, 2.0f, 1.0f, 2.6f, 1.0f, true);
        break;
    case 2: // Observatorio: pedestal del telescopio y consolas.
        AgregarObstaculoLluvia(minijuego, 0.0f, 0.0f, 2.4f, 2.6f, 2.4f, true);
        AgregarObstaculoLluvia(minijuego, -4.2f, 0.0f, 1.5f, 2.0f, 1.5f, false);
        AgregarObstaculoLluvia(minijuego, 4.2f, 0.0f, 1.5f, 2.0f, 1.5f, false);
        AgregarObstaculoLluvia(minijuego, 0.0f, -3.4f, 1.2f, 2.0f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 0.0f, 3.4f, 1.2f, 2.0f, 1.2f, true);
        break;
    case 3: // Puerto: cajas apiladas, barril y bolardo.
        AgregarObstaculoLluvia(minijuego, -3.0f, -2.0f, 2.0f, 2.0f, 2.0f, false);
        AgregarObstaculoLluvia(minijuego, -1.0f, -2.4f, 1.8f, 2.0f, 1.8f, false);
        AgregarObstaculoLluvia(minijuego, 3.2f, 1.6f, 1.4f, 2.0f, 1.4f, true);
        AgregarObstaculoLluvia(minijuego, 0.5f, 2.8f, 1.2f, 2.0f, 1.2f, true);
        AgregarObstaculoLluvia(minijuego, 3.2f, -2.2f, 2.0f, 2.0f, 2.0f, false);
        break;
    default: // Mina: vagoneta central y cuatro pilares de madera.
        AgregarObstaculoLluvia(minijuego, -3.2f, -2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, 3.2f, -2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, -3.2f, 2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, 3.2f, 2.4f, 0.9f, 3.2f, 0.9f, false);
        AgregarObstaculoLluvia(minijuego, 0.0f, 0.0f, 3.2f, 2.0f, 1.6f, false);
        break;
    }

    const TemaLluvia& tema = ObtenerTemaLluvia(minijuego.tema);

    // Suelo (cara superior en y = 0).
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 0.0f, -0.45f, 0.0f }, { 18.0f, 0.90f, 12.0f }, tema.suelo);

    // Muros bajos de los cuatro lados (ademas hay un clamp de seguridad).
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { -8.75f, 0.5f, 0.0f }, { 0.5f, 1.0f, 12.0f }, tema.muro);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 8.75f, 0.5f, 0.0f }, { 0.5f, 1.0f, 12.0f }, tema.muro);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 0.0f, 0.5f, -5.75f }, { 18.0f, 1.0f, 0.5f }, tema.muro);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
        { 0.0f, 0.5f, 5.75f }, { 18.0f, 1.0f, 0.5f }, tema.muro);

    for (int i = 0; i < minijuego.cantidadObstaculos; i++)
    {
        const ObstaculoLluvia& obstaculo = minijuego.obstaculos[i];
        AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_LLUVIA,
            obstaculo.posicion, obstaculo.tamano, tema.obstaculoA);
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

    // Tematica pseudoaleatoria segun los segundos del reloj del sistema.
    tema = (int)(std::time(nullptr) % CANTIDAD_TEMAS_LLUVIA);
    ConfigurarMapaLluvia(*this);

    camara.position = { 0.0f, 14.8f, 11.8f };
    camara.target = { 0.0f, 0.0f, 0.3f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_LLUVIA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_LLUVIA;
    tiempoRestante = DURACION_PARTIDA_LLUVIA;
    tiempoAnimacion = 0.0f;
    tiempoSiguientePieza = 0.0f;
    tiempoJugado = 0.0f;
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
// VISUAL: PIEZAS
//==================================================

// Dibuja una pieza sin sombra automatica (la sombra de caida es el marcador
// del suelo, que es la que debe leerse con claridad).
static void DibujarPiezaLluvia(
    int temaIndice,
    TipoPiezaLluvia tipo,
    Vector3 centro,
    float escala
)
{
    const TemaLluvia& tema = ObtenerTemaLluvia(temaIndice);
    Color color = tema.pieza;
    FormaPiezaLluvia forma = tema.formaPieza;

    if (tipo == PIEZA_LLUVIA_RARA)
    {
        color = tema.piezaRara;
        forma = tema.formaRara;
    }
    else if (tipo == PIEZA_LLUVIA_PELIGROSA)
    {
        color = tema.piezaPeligro;
        forma = tema.formaPeligro;
    }

    float radio = 0.30f * escala;

    switch (forma)
    {
    case FORMA_LLUVIA_ESFERA:
        DrawSphereEx(centro, radio, 8, 10, color);
        break;
    case FORMA_LLUVIA_CUBO:
        DrawCubeV(centro, { radio * 1.8f, radio * 1.8f, radio * 1.8f }, color);
        DrawCubeWires(centro, radio * 1.8f, radio * 1.8f, radio * 1.8f, Fade(BLACK, 0.55f));
        break;
    case FORMA_LLUVIA_CILINDRO:
        DrawCylinderEx(
            { centro.x, centro.y - radio, centro.z },
            { centro.x, centro.y + radio, centro.z },
            radio * 0.8f, radio * 0.8f, 10, color
        );
        break;
    default:
        DrawCylinderEx(
            { centro.x, centro.y - radio, centro.z },
            { centro.x, centro.y + radio * 1.1f, centro.z },
            radio * 1.05f, 0.0f, 4, color
        );
        break;
    }

    if (tipo == PIEZA_LLUVIA_PELIGROSA)
    {
        // Contorno rojo: lo peligroso se distingue siempre de un vistazo.
        DrawSphereWires(centro, radio * 1.45f, 6, 8, Color{ 255, 50, 40, 255 });
    }
    else if (tipo == PIEZA_LLUVIA_RARA)
    {
        DrawSphereWires(centro, radio * 1.35f, 6, 8, Color{ 255, 255, 255, 200 });
    }
}


static Color ColorMarcadorLluvia(TipoPiezaLluvia tipo)
{
    if (tipo == PIEZA_LLUVIA_PELIGROSA) return Color{ 255, 50, 40, 255 };
    if (tipo == PIEZA_LLUVIA_RARA) return Color{ 255, 215, 60, 255 };
    return Color{ 255, 255, 255, 255 };
}


//==================================================
// VISUAL: ESCENARIO
//==================================================
//
// Helpers sin sombra automatica para decorado lejano (las sombras retro
// automaticas solo tienen sentido para objetos apoyados en la arena).

static void CuboDecorLluvia(Vector3 posicion, float ancho, float alto, float largo, Color color)
{
    DrawCubeV(posicion, { ancho, alto, largo }, color);
}


static void EsferaDecorLluvia(Vector3 posicion, float radio, Color color)
{
    DrawSphereEx(posicion, radio, 10, 12, color);
}


static void CilindroDecorLluvia(Vector3 base, float radio, float alto, Color color, int lados = 12)
{
    DrawCylinderEx(base, { base.x, base.y + alto, base.z }, radio, radio, lados, color);
}


static void ConoDecorLluvia(Vector3 base, float radio, float alto, int lados, Color color)
{
    DrawCylinderEx(base, { base.x, base.y + alto, base.z }, radio, 0.0f, lados, color);
}


static void DibujarFondoTemploLluvia(const TemaLluvia& tema)
{
    (void)tema;
    Color arenisca = Color{ 176, 150, 104, 255 };
    Color sombra = Color{ 140, 118, 84, 255 };

    // Piramide escalonada al fondo.
    CuboDecorLluvia({ 0.0f, 1.5f, -14.0f }, 15.0f, 3.0f, 6.0f, arenisca);
    CuboDecorLluvia({ 0.0f, 4.0f, -14.0f }, 11.0f, 2.0f, 4.4f, sombra);
    CuboDecorLluvia({ 0.0f, 6.0f, -14.0f }, 7.0f, 2.0f, 3.0f, arenisca);
    CuboDecorLluvia({ 0.0f, 7.7f, -14.0f }, 3.4f, 1.4f, 1.8f, sombra);

    // Columnas exteriores y ruinas.
    for (int i = 0; i < 6; i++)
    {
        float lado = i < 3 ? -1.0f : 1.0f;
        float z = -6.0f + (float)(i % 3) * 5.0f;
        float altura = 4.5f - (float)(i % 2) * 1.5f;
        CilindroDecorLluvia({ lado * 11.5f, -0.9f, z }, 0.8f, altura + 0.9f, arenisca);
        CuboDecorLluvia({ lado * 11.5f, altura, z }, 2.0f, 0.4f, 2.0f, sombra);
    }

    // Vegetacion de jungla.
    for (int i = 0; i < 8; i++)
    {
        float x = -17.0f + (float)i * 4.8f;
        float z = -9.0f - (float)((i * 7) % 4);
        CilindroDecorLluvia({ x, -0.9f, z }, 0.35f, 3.2f, Color{ 92, 64, 40, 255 }, 8);
        EsferaDecorLluvia({ x, 3.8f, z }, 1.8f, Color{ 38, 110, 56, 255 });
        EsferaDecorLluvia({ x + 0.9f, 3.2f, z + 0.5f }, 1.3f, Color{ 52, 130, 66, 255 });
    }

    // Brasero en las esquinas de la arena.
    for (int i = 0; i < 4; i++)
    {
        float x = (i % 2 == 0) ? -9.4f : 9.4f;
        float z = (i < 2) ? -6.4f : 6.4f;
        CilindroDecorLluvia({ x, 0.0f, z }, 0.35f, 1.2f, Color{ 90, 84, 78, 255 }, 8);
        EsferaDecorLluvia({ x, 1.45f, z }, 0.42f, Color{ 255, 150, 40, 255 });
    }
}


static void DibujarFondoDesiertoLluvia(const TemaLluvia& tema)
{
    (void)tema;

    EsferaDecorLluvia({ -14.0f, -2.2f, -9.0f }, 6.0f, Color{ 230, 198, 128, 255 });
    EsferaDecorLluvia({ 11.0f, -2.6f, -12.0f }, 7.0f, Color{ 236, 204, 134, 255 });
    EsferaDecorLluvia({ 0.0f, -4.5f, -18.0f }, 9.0f, Color{ 226, 192, 122, 255 });

    ConoDecorLluvia({ -7.5f, -0.9f, -15.0f }, 5.5f, 6.5f, 4, Color{ 204, 164, 90, 255 });
    ConoDecorLluvia({ 6.5f, -0.9f, -17.0f }, 4.4f, 5.4f, 4, Color{ 212, 172, 98, 255 });

    // Sol.
    EsferaDecorLluvia({ 15.0f, 11.0f, -22.0f }, 2.6f, Color{ 255, 236, 140, 255 });

    // Cactus y rocas decorativas fuera de la arena.
    for (int i = 0; i < 7; i++)
    {
        float lado = (i % 2 == 0) ? -1.0f : 1.0f;
        float x = lado * (11.5f + (float)(i % 3) * 1.8f);
        float z = -7.0f + (float)i * 2.2f;
        CilindroDecorLluvia({ x, -0.9f, z }, 0.4f, 3.0f, Color{ 52, 120, 64, 255 }, 8);
        CilindroDecorLluvia({ x + 0.5f, 0.6f, z }, 0.25f, 1.0f, Color{ 52, 120, 64, 255 }, 8);
    }

    // Antorchas de arenisca en las esquinas.
    for (int i = 0; i < 4; i++)
    {
        float x = (i % 2 == 0) ? -9.4f : 9.4f;
        float z = (i < 2) ? -6.4f : 6.4f;
        CuboDecorLluvia({ x, 0.7f, z }, 0.7f, 1.4f, 0.7f, Color{ 190, 140, 80, 255 });
        EsferaDecorLluvia({ x, 1.6f, z }, 0.3f, Color{ 255, 170, 50, 255 });
    }
}


static void DibujarFondoObservatorioLluvia(const TemaLluvia& tema)
{
    (void)tema;

    // Estrellas en posiciones deterministas.
    for (int i = 0; i < 46; i++)
    {
        float x = -34.0f + (float)((i * 37) % 68);
        float y = 5.0f + (float)((i * 13) % 20);
        float z = -26.0f;
        float tam = 0.18f + 0.1f * (float)(i % 3);
        CuboDecorLluvia({ x, y, z }, tam, tam, tam, Color{ 240, 240, 255, 255 });
    }

    EsferaDecorLluvia({ -16.0f, 12.0f, -24.0f }, 2.4f, Color{ 230, 232, 245, 255 });

    // Cupula del observatorio con su rendija.
    EsferaDecorLluvia({ 0.0f, -2.0f, -15.0f }, 8.0f, Color{ 84, 98, 140, 255 });
    CuboDecorLluvia({ 0.0f, 3.2f, -7.4f }, 1.8f, 7.0f, 0.4f, Color{ 12, 14, 30, 255 });
    CilindroDecorLluvia({ 0.0f, -0.9f, -15.0f }, 8.2f, 1.4f, Color{ 66, 76, 112, 255 }, 20);

    // Tubo de un telescopio gigante al fondo.
    DrawCylinderEx(
        { 7.0f, 0.0f, -10.0f }, { 11.0f, 7.0f, -13.0f },
        0.9f, 0.7f, 12, Color{ 150, 158, 190, 255 }
    );
    CilindroDecorLluvia({ 7.0f, -0.9f, -10.0f }, 1.2f, 1.6f, Color{ 70, 78, 110, 255 });

    // Luces azules en las esquinas.
    for (int i = 0; i < 4; i++)
    {
        float x = (i % 2 == 0) ? -9.4f : 9.4f;
        float z = (i < 2) ? -6.4f : 6.4f;
        CilindroDecorLluvia({ x, 0.0f, z }, 0.25f, 1.3f, Color{ 90, 100, 140, 255 }, 8);
        EsferaDecorLluvia({ x, 1.5f, z }, 0.32f, Color{ 90, 220, 255, 255 });
    }
}


static void DibujarFondoPuertoLluvia(const TemaLluvia& tema)
{
    (void)tema;

    // Casco de un barco amarrado al fondo, con cabina y mastil.
    CuboDecorLluvia({ -5.0f, 0.2f, -13.0f }, 14.0f, 3.2f, 4.2f, Color{ 112, 40, 40, 255 });
    CuboDecorLluvia({ -5.0f, 2.2f, -13.0f }, 14.0f, 0.4f, 4.4f, Color{ 190, 170, 130, 255 });
    CuboDecorLluvia({ -8.0f, 3.6f, -13.0f }, 3.0f, 2.4f, 2.6f, Color{ 220, 220, 214, 255 });
    CilindroDecorLluvia({ -3.0f, 2.4f, -13.0f }, 0.22f, 8.0f, Color{ 100, 76, 52, 255 }, 8);
    CuboDecorLluvia({ -3.0f, 7.0f, -12.8f }, 4.0f, 3.6f, 0.1f, Color{ 238, 232, 214, 255 });

    // Grua del muelle.
    CilindroDecorLluvia({ 10.5f, -0.9f, -9.0f }, 0.6f, 9.0f, Color{ 220, 170, 40, 255 }, 8);
    CuboDecorLluvia({ 8.0f, 8.4f, -9.0f }, 6.0f, 0.5f, 0.5f, Color{ 220, 170, 40, 255 });
    CuboDecorLluvia({ 6.0f, 6.6f, -9.0f }, 0.1f, 3.6f, 0.1f, Color{ 60, 60, 60, 255 });

    // Faro lejano.
    CilindroDecorLluvia({ 16.0f, -1.4f, -18.0f }, 1.4f, 9.0f, Color{ 236, 236, 236, 255 }, 12);
    CilindroDecorLluvia({ 16.0f, 3.0f, -18.0f }, 1.45f, 1.6f, Color{ 200, 50, 50, 255 }, 12);
    EsferaDecorLluvia({ 16.0f, 8.0f, -18.0f }, 0.9f, Color{ 255, 240, 150, 255 });

    // Pilotes del muelle y barriles/bolardos en las esquinas.
    for (int i = 0; i < 4; i++)
    {
        float x = (i % 2 == 0) ? -9.4f : 9.4f;
        float z = (i < 2) ? -6.4f : 6.4f;
        CilindroDecorLluvia({ x, -1.4f, z }, 0.35f, 2.8f, Color{ 90, 64, 42, 255 }, 8);
        EsferaDecorLluvia({ x, 1.6f, z }, 0.28f, Color{ 255, 220, 120, 255 });
    }
}


static void DibujarFondoMinaLluvia(const TemaLluvia& tema)
{
    (void)tema;

    // Pared de roca del fondo.
    for (int i = 0; i < 9; i++)
    {
        float x = -20.0f + (float)i * 5.0f;
        float radio = 3.4f + (float)((i * 5) % 3) * 0.9f;
        EsferaDecorLluvia({ x, 1.0f, -11.0f - (float)(i % 2) * 2.0f }, radio, Color{ 76, 70, 70, 255 });
    }

    // Estalactitas colgando (cono invertido).
    for (int i = 0; i < 10; i++)
    {
        float x = -16.0f + (float)i * 3.6f;
        DrawCylinderEx(
            { x, 14.0f, -9.0f }, { x, 10.5f + (float)(i % 3), -9.0f },
            0.9f, 0.0f, 6, Color{ 88, 80, 78, 255 }
        );
    }

    // Marcos de madera a los lados.
    for (int i = 0; i < 2; i++)
    {
        float lado = i == 0 ? -1.0f : 1.0f;
        CuboDecorLluvia({ lado * 11.0f, 2.2f, -2.0f }, 0.7f, 6.2f, 0.7f, Color{ 104, 76, 48, 255 });
        CuboDecorLluvia({ lado * 11.0f, 2.2f, 4.0f }, 0.7f, 6.2f, 0.7f, Color{ 104, 76, 48, 255 });
        CuboDecorLluvia({ lado * 11.0f, 5.4f, 1.0f }, 0.7f, 0.7f, 7.0f, Color{ 104, 76, 48, 255 });
    }

    // Farolillos en las esquinas.
    for (int i = 0; i < 4; i++)
    {
        float x = (i % 2 == 0) ? -9.4f : 9.4f;
        float z = (i < 2) ? -6.4f : 6.4f;
        CilindroDecorLluvia({ x, 0.0f, z }, 0.2f, 1.5f, Color{ 90, 66, 44, 255 }, 8);
        EsferaDecorLluvia({ x, 1.7f, z }, 0.3f, Color{ 255, 212, 110, 255 });
    }
}


// Suelo de la arena: lineas propias de cada tematica sobre la cara superior.
static void DibujarSueloLluvia(const TemaLluvia& tema, int temaIndice)
{
    DrawCubeV({ 0.0f, -0.45f, 0.0f }, { 18.0f, 0.90f, 12.0f }, tema.suelo);

    const float Y = 0.012f;

    switch (temaIndice)
    {
    case 0: // Losas de templo.
        for (int i = -4; i <= 4; i++)
        {
            DrawLine3D({ (float)i * 2.0f, Y, -5.5f }, { (float)i * 2.0f, Y, 5.5f }, tema.lineas);
        }
        for (int i = -2; i <= 2; i++)
        {
            DrawLine3D({ -8.5f, Y, (float)i * 2.2f }, { 8.5f, Y, (float)i * 2.2f }, tema.lineas);
        }
        break;
    case 1: // Ondas de duna.
        for (int i = -4; i <= 4; i++)
        {
            DrawLine3D({ -8.5f, Y, (float)i * 1.2f }, { 8.5f, Y, (float)i * 1.2f + 0.4f }, tema.lineas);
        }
        break;
    case 2: // Anillos de cupula.
        DrawCircle3D({ 0.0f, Y, 0.0f }, 2.5f, { 1.0f, 0.0f, 0.0f }, 90.0f, tema.lineas);
        DrawCircle3D({ 0.0f, Y, 0.0f }, 4.5f, { 1.0f, 0.0f, 0.0f }, 90.0f, tema.lineas);
        DrawCircle3D({ 0.0f, Y, 0.0f }, 5.4f, { 1.0f, 0.0f, 0.0f }, 90.0f, tema.lineas);
        break;
    case 3: // Tablones del muelle.
        for (int i = -10; i <= 10; i++)
        {
            DrawLine3D({ (float)i * 0.85f, Y, -5.5f }, { (float)i * 0.85f, Y, 5.5f }, tema.lineas);
        }
        break;
    default: // Rieles con durmientes.
        for (int i = -7; i <= 7; i++)
        {
            CuboDecorLluvia({ (float)i * 1.15f, 0.02f, 0.0f }, 0.22f, 0.04f, 2.1f, Color{ 90, 66, 44, 255 });
        }
        CuboDecorLluvia({ 0.0f, 0.06f, -0.8f }, 17.0f, 0.06f, 0.12f, tema.lineas);
        CuboDecorLluvia({ 0.0f, 0.06f, 0.8f }, 17.0f, 0.06f, 0.12f, tema.lineas);
        break;
    }
}


// MODELO FUTURO: todo lo que hay entre "DibujarEscenarioVisualLluvia" y sus
// helpers (suelo, muros, fondo y decorado de cada tematica) puede sustituirse
// por un .glb de escenario sin tocar el gameplay: la colision sale solo de
// "bloques" y "obstaculos", y las bases/limites de las constantes de arriba.
static void DibujarEscenarioVisualLluvia(const MinijuegoLluviaApilada& minijuego)
{
    const TemaLluvia& tema = ObtenerTemaLluvia(minijuego.tema);

    DrawPlane({ 0.0f, -0.92f, -4.0f }, { 120.0f, 90.0f }, tema.plano);

    switch (minijuego.tema)
    {
    case 0: DibujarFondoTemploLluvia(tema); break;
    case 1: DibujarFondoDesiertoLluvia(tema); break;
    case 2: DibujarFondoObservatorioLluvia(tema); break;
    case 3: DibujarFondoPuertoLluvia(tema); break;
    default: DibujarFondoMinaLluvia(tema); break;
    }

    DibujarSueloLluvia(tema, minijuego.tema);

    // Muros bajos: bloques 1 a 4 (el 0 es el suelo).
    for (int i = 1; i < 5 && i < minijuego.cantidadBloques; i++)
    {
        const BloquePrueba& bloque = minijuego.bloques[i];
        DrawCubeV(bloque.posicion, bloque.tamano, bloque.color);
        DrawCubeWires(
            bloque.posicion, bloque.tamano.x, bloque.tamano.y, bloque.tamano.z,
            Fade(BLACK, 0.5f)
        );
    }

    // Obstaculos con detalle tematico.
    for (int i = 0; i < minijuego.cantidadObstaculos; i++)
    {
        const ObstaculoLluvia& obstaculo = minijuego.obstaculos[i];
        Color color = (i % 2 == 0) ? tema.obstaculoA : tema.obstaculoB;
        Vector3 pie = { obstaculo.posicion.x, 0.0f, obstaculo.posicion.z };
        float alto = obstaculo.tamano.y;

        if (minijuego.tema == 1 && obstaculo.redondo)
            color = tema.obstaculoB; // cactus verdes

        if (obstaculo.redondo)
        {
            DrawCylinder(pie, obstaculo.tamano.x * 0.5f, obstaculo.tamano.x * 0.5f, alto, 12, color);
        }
        else
        {
            DrawCube(obstaculo.posicion, obstaculo.tamano.x, alto, obstaculo.tamano.z, color);
            DrawCubeWires(obstaculo.posicion, obstaculo.tamano.x, alto, obstaculo.tamano.z, Fade(BLACK, 0.55f));
        }

        switch (minijuego.tema)
        {
        case 0: // capitel / remate
            CuboDecorLluvia(
                { pie.x, alto + 0.1f, pie.z },
                obstaculo.tamano.x + 0.4f, 0.25f, obstaculo.tamano.z + 0.4f,
                Color{ 190, 178, 146, 255 }
            );
            break;
        case 1: // brazo de cactus
            if (obstaculo.redondo)
            {
                CilindroDecorLluvia({ pie.x + 0.55f, alto * 0.5f, pie.z }, 0.2f, 0.9f, color, 8);
            }
            break;
        case 2: // luz superior
            EsferaDecorLluvia({ pie.x, alto + 0.25f, pie.z }, 0.28f, Color{ 90, 220, 255, 255 });
            break;
        case 3: // cinchos de caja/barril
            DrawCubeWires(
                { pie.x, alto * 0.5f, pie.z },
                obstaculo.tamano.x + 0.04f, alto * 0.5f, obstaculo.tamano.z + 0.04f,
                Color{ 60, 40, 24, 255 }
            );
            break;
        default: // carga de mineral sobre la vagoneta
            if (i == minijuego.cantidadObstaculos - 1)
            {
                EsferaDecorLluvia({ pie.x - 0.7f, alto + 0.2f, pie.z }, 0.45f, Color{ 150, 160, 190, 255 });
                EsferaDecorLluvia({ pie.x + 0.2f, alto + 0.28f, pie.z + 0.1f }, 0.55f, Color{ 120, 130, 160, 255 });
                EsferaDecorLluvia({ pie.x + 0.9f, alto + 0.18f, pie.z - 0.1f }, 0.42f, Color{ 90, 230, 240, 255 });
            }
            break;
        }
    }
}


//==================================================
// DIBUJO
//==================================================

static void DibujarBasesLluvia(
    const MinijuegoLluviaApilada& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!minijuego.resultado.participantes[i].participo) continue;

        Vector3 base = ObtenerBaseLluvia(i);
        Color color = participantes[i].color;
        bool brillo =
            minijuego.estadosJugadores[i].pila > 0 &&
            JugadorEnBaseLluvia(i, jugadores[i]);
        float pulso = 0.5f + 0.5f * std::sin(minijuego.tiempoAnimacion * 8.0f);

        DrawCylinderEx(
            { base.x, 0.02f, base.z }, { base.x, 0.07f, base.z },
            RADIO_BASE_LLUVIA, RADIO_BASE_LLUVIA, 24,
            Fade(color, brillo ? 0.55f + 0.3f * pulso : 0.38f)
        );
        DrawCircle3D(
            { base.x, 0.09f, base.z }, RADIO_BASE_LLUVIA,
            { 1.0f, 0.0f, 0.0f }, 90.0f, color
        );
        DrawCircle3D(
            { base.x, 0.09f, base.z }, RADIO_BASE_LLUVIA * 0.7f,
            { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.7f)
        );

        // Cuatro postes de la base.
        for (int k = 0; k < 4; k++)
        {
            float angulo = (float)k * PI * 0.5f + PI * 0.25f;
            Vector3 poste =
            {
                base.x + std::cos(angulo) * RADIO_BASE_LLUVIA,
                0.0f,
                base.z + std::sin(angulo) * RADIO_BASE_LLUVIA
            };
            CilindroDecorLluvia(poste, 0.07f, 0.55f, color, 6);
        }

        if (minijuego.estadosJugadores[i].tiempoAvisoDeposito > 0.0f)
        {
            float t = minijuego.estadosJugadores[i].tiempoAvisoDeposito;
            DrawCylinderEx(
                { base.x, 0.1f, base.z }, { base.x, 0.1f + (0.9f - t) * 3.0f, base.z },
                RADIO_BASE_LLUVIA * 0.9f, RADIO_BASE_LLUVIA * 0.9f, 16,
                Fade(GOLD, t * 0.5f)
            );
        }
    }
}


static void DibujarMarcadoresCaidaLluvia(const MinijuegoLluviaApilada& minijuego)
{
    for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
    {
        const PiezaCayendoLluvia& pieza = minijuego.piezasCayendo[n];
        if (!pieza.activa) continue;

        float avance = LimitarLluvia(pieza.tiempo / pieza.duracion, 0.0f, 1.0f);
        float radio = 0.30f + 0.70f * avance;
        Color color = ColorMarcadorLluvia(pieza.tipo);
        Vector3 centro = { pieza.x, 0.03f, pieza.z };

        DrawCylinderEx(
            { pieza.x, 0.022f, pieza.z }, { pieza.x, 0.032f, pieza.z },
            radio, radio, 20, Fade(color, 0.20f + 0.35f * avance)
        );
        DrawCircle3D(centro, radio, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
        DrawCircle3D(centro, radio * 0.55f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.7f));

        if (pieza.tipo == PIEZA_LLUVIA_PELIGROSA)
        {
            // Una "X" roja: no se puede confundir con una pieza buena.
            float d = radio * 0.7f;
            DrawLine3D({ pieza.x - d, 0.05f, pieza.z - d }, { pieza.x + d, 0.05f, pieza.z + d }, color);
            DrawLine3D({ pieza.x - d, 0.05f, pieza.z + d }, { pieza.x + d, 0.05f, pieza.z - d }, color);
        }
    }
}


void MinijuegoLluviaApilada::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    const TemaLluvia& tema = ObtenerTemaLluvia(this->tema);
    int limite = LimiteJugadoresLluvia(cantidadMaxima);

    ClearBackground(tema.cielo);
    BeginMode3D(camara);

    DibujarEscenarioVisualLluvia(*this);
    DibujarBasesLluvia(*this, jugadores, participantes, limite);
    DibujarMarcadoresCaidaLluvia(*this);

    // Piezas en el aire.
    for (int n = 0; n < MAX_PIEZAS_CAYENDO_LLUVIA; n++)
    {
        const PiezaCayendoLluvia& pieza = piezasCayendo[n];
        if (!pieza.activa) continue;

        DibujarPiezaLluvia(
            this->tema, pieza.tipo,
            { pieza.x, pieza.altura + 0.3f, pieza.z }, 1.25f
        );
    }

    // Piezas sueltas en el suelo.
    for (int n = 0; n < MAX_PIEZAS_SUELTAS_LLUVIA; n++)
    {
        const PiezaSueltaLluvia& pieza = piezasSueltas[n];
        if (!pieza.activa) continue;

        // Parpadean cuando estan a punto de desaparecer.
        if (pieza.tiempoVida < 2.0f && ((int)(tiempoAnimacion * 10.0f)) % 2 == 0) continue;

        DibujarPiezaLluvia(
            this->tema,
            pieza.rara ? PIEZA_LLUVIA_RARA : PIEZA_LLUVIA_NORMAL,
            pieza.posicion, 1.0f
        );
        DrawCircle3D(
            { pieza.posicion.x, 0.04f, pieza.posicion.z }, 0.42f,
            { 1.0f, 0.0f, 0.0f }, 90.0f,
            Fade(pieza.rara ? tema.piezaRara : tema.pieza, 0.8f)
        );
    }

    // Jugadores y su pila balanceandose.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        const EstadoJugadorLluvia& estado = estadosJugadores[i];

        if (!jugadores[i].cayendo)
        {
            float inicioY =
                jugadores[i].posicion.y + jugadores[i].tamano.y * 0.5f + 0.25f;
            float amplitud = 0.012f * (0.4f + 0.12f * (float)estado.pila);

            for (int k = 0; k < estado.pila; k++)
            {
                float altura = (float)(k + 1);
                float desvioX = std::sin(tiempoAnimacion * 2.6f + (float)k * 0.55f + (float)i) * amplitud * altura * altura * 0.5f;
                float desvioZ = std::cos(tiempoAnimacion * 2.1f + (float)k * 0.7f) * amplitud * altura * altura * 0.4f;

                desvioX = LimitarLluvia(desvioX, -0.55f, 0.55f);
                desvioZ = LimitarLluvia(desvioZ, -0.55f, 0.55f);

                DibujarPiezaLluvia(
                    this->tema,
                    estado.rara[k] ? PIEZA_LLUVIA_RARA : PIEZA_LLUVIA_NORMAL,
                    {
                        jugadores[i].posicion.x + desvioX,
                        inicioY + (float)k * ALTURA_PIEZA_EN_PILA_LLUVIA,
                        jugadores[i].posicion.z + desvioZ
                    },
                    0.85f
                );
            }
        }

        if (mostrarDebug && !jugadores[i].cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA);

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
        }
    }

    EndMode3D();

    //----------------------------------------------
    // HUD
    //----------------------------------------------
    const float e = (float)GetScreenHeight() / 720.0f;
    DrawRectangle((int)(14 * e), (int)(12 * e), (int)(900 * e), (int)(104 * e), Fade(BLACK, 0.72f));
    DrawText(TextFormat("LLUVIA APILADA - %s", tema.nombre), (int)(26 * e), (int)(20 * e), (int)(28 * e), GOLD);
    DrawText(
        TextFormat(
            "ATRAPA %s: MAS PILA = MAS LENTO Y TORPE. SOLO CUENTAN LAS ASEGURADAS (RARA = 3).",
            tema.piezas
        ),
        (int)(26 * e), (int)(52 * e), (int)(15 * e), RAYWHITE
    );
    DrawText(
        TextFormat(
            "ASEGURA: BOTON PRINCIPAL EN TU BASE DE COLOR  |  GOLPEA PARA TIRAR LA PILA RIVAL  |  EVITA %s (ROJO)",
            tema.peligro
        ),
        (int)(26 * e), (int)(72 * e), (int)(13 * e), Color{ 255, 190, 170, 255 }
    );
    DrawText(
        "LA SOMBRA EN EL SUELO MARCA DONDE CAE. LA PILA NO ASEGURADA AL TERMINAR SE PIERDE.",
        (int)(26 * e), (int)(88 * e), (int)(13 * e), Color{ 190, 215, 240, 255 }
    );

    if (fase == FASE_LLUVIA_JUGANDO)
    {
        const char* textoTiempo = TextFormat("TIEMPO %.1f", tiempoRestante);
        DrawRectangle(GetScreenWidth() - 210, 12, 196, 44, Fade(BLACK, 0.72f));
        DrawText(textoTiempo, GetScreenWidth() - 196, 22, 24, tiempoRestante <= 8.0f ? RED : GOLD);

        if (tiempoRestante <= 8.0f && ((int)(tiempoAnimacion * 4.0f)) % 2 == 0)
        {
            const char* aviso = "ASEGURA TU PILA!";
            DrawText(
                aviso,
                GetScreenWidth() / 2 - MeasureText(aviso, 30) / 2,
                GetScreenHeight() - 80, 30, RED
            );
        }
    }

    // La lista va abajo a la izquierda: arriba tapaba la base del J3.
    int filasHud = 0;
    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo) filasHud++;
    }

    int fila = GetScreenHeight() - 14 - 27 * filasHud;
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        const char* boton = ObtenerTextoBotonPrincipal(participantes[i]);
        const EstadoJugadorLluvia& estado = estadosJugadores[i];

        DrawRectangle(14, fila - 3, 470, 24, Fade(BLACK, 0.55f));
        DrawText(
            TextFormat(
                "J%d%s  ASEGURADAS %d   PILA %d/%d   [%s]",
                participantes[i].numeroJugador,
                participantes[i].esBot ? " BOT" : "",
                estado.aseguradas,
                estado.pila,
                MAX_PILA_LLUVIA,
                boton
            ),
            22, fila, 18, participantes[i].color
        );
        fila += 27;
    }

    if (fase == FASE_LLUVIA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 88) / 2,
            GetScreenHeight() / 2 - 54, 88, GOLD
        );
    }
    else if (
        fase == FASE_LLUVIA_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(
            resultado, ganadores, MAX_PARTICIPANTES
        );

        const char* titulo = resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                    ? participantes[ganadores[0]].numeroJugador
                    : 0
            );

        int alto = 190 + 29 * resultado.cantidadParticipantes;
        int arriba = (int)(140 * ((float)GetScreenHeight() / 720.0f));

        DrawRectangle(GetScreenWidth() / 2 - 315, arriba, 630, alto, Fade(BLACK, 0.84f));
        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            arriba + 22, 34, GOLD
        );

        int filaFinal = arriba + 76;
        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo) continue;

            DrawText(
                TextFormat(
                    "J%d  PUESTO %d  %d ASEGURADAS",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    estadosJugadores[i].aseguradas
                ),
                GetScreenWidth() / 2 - 190, filaFinal, 21, participantes[i].color
            );
            filaFinal += 29;
        }

        DrawText(
            TextoReinicioMinijuego(),
            GetScreenWidth() / 2 - MeasureText(TextoReinicioMinijuego(), 21) / 2,
            filaFinal + 14, 21, RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoLluviaApilada::ObtenerResultado() const
{
    return resultado;
}
