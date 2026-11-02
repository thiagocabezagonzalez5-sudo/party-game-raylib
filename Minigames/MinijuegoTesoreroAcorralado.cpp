#include "Minigames/MinijuegoTesoreroAcorralado.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_TESORERO = 3.0f;
static const float DURACION_PARTIDA_TESORERO = 40.0f;

static const int MONEDAS_INICIALES_TESORERO = 100;
static const int MONEDAS_GOLPE_TESORERO = 5;
static const int MONEDAS_POUND_TESORERO = 12;
static const float MULTIPLICADOR_TESORERO = 1.15f;
static const float INMUNIDAD_TESORERO = 1.0f;

// Patio de armas: el suelo mide 18 x 14.
static const float MITAD_SUELO_X_TESORERO = 9.0f;
static const float MITAD_SUELO_Z_TESORERO = 7.0f;
static const float LIMITE_X_TESORERO = 8.4f;
static const float LIMITE_Z_TESORERO = 6.4f;
static const float MITAD_ALTO_JUGADOR_TESORERO = 0.7f;

// Torre del homenaje (obstaculo central).
static const float MITAD_TORRE_TESORERO = 1.3f;
static const float ALTO_TORRE_TESORERO = 2.2f;

// Rejas levadizas.
static const float ALTO_REJA_TESORERO = 2.4f;
static const float TIEMPO_ABIERTA_TESORERO = 3.0f;
static const float TIEMPO_AVISO_TESORERO = 0.9f;
static const float TIEMPO_CERRADA_TESORERO = 3.2f;
static const float VELOCIDAD_REJA_TESORERO = 2.5f;

// Golpes.
static const float ALCANCE_GOLPE_TESORERO = 1.55f;
static const float RADIO_POUND_TESORERO = 1.05f;

// Monedas.
static const float GRAVEDAD_MONEDA_TESORERO = 20.0f;
static const float SUELO_MONEDA_TESORERO = 0.25f;
static const float RADIO_RECOGER_TESORERO = 0.85f;
static const float ESPERA_RECOGER_TESORERO = 0.35f;
static const float VIDA_MONEDA_TESORERO = 9.0f;
static const float INTERVALO_SONIDO_MONEDA_TESORERO = 0.08f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarTesorero(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AbsTesorero(float valor)
{
    return valor < 0.0f ? -valor : valor;
}


static float AleatorioTesorero(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}


static float Ruido01Tesorero(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static int LimiteJugadoresTesorero(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static bool EsControladoPorBotTesorero(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static int ContarMonedasSueltasTesorero(const MinijuegoTesoreroAcorralado& minijuego)
{
    int total = 0;

    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        if (minijuego.monedas[i].activa)
        {
            total++;
        }
    }

    return total;
}


// Lo que la logica cambio en un paso: los efectos visuales comparan el
// antes y el despues sin tocar las reglas.
struct InstantaneaTesorero
{
    FaseTesoreroAcorralado fase = FASE_TESORERO_PREPARACION;
    int monedasTesorero = 0;
    bool monedaActiva[MAX_MONEDAS_TESORERO]{};
    float edadMoneda[MAX_MONEDAS_TESORERO]{};
    Vector3 posicionMoneda[MAX_MONEDAS_TESORERO]{};
    float alturaReja[MAX_REJAS_TESORERO]{};
};

static void TomarInstantaneaTesorero(const MinijuegoTesoreroAcorralado& m, InstantaneaTesorero& s);

static void ActualizarVisualTesorero(
    MinijuegoTesoreroAcorralado& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    const InstantaneaTesorero& antes
);


// Solo las piezas jugables del paquete GLB (marco y hoja de las rejas,
// marca de aviso y moneda): el escenario se construye con primitivas.
static_assert(
    MODELO_TESORERO_MARCO_REJA + 4 == CANTIDAD_MODELOS_TESORERO_CERCADO_3D,
    "Las piezas jugables de Tesorero deben ser el tramo final del paquete");

static void CargarPiezasJugablesTesorero()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteTesoreroCercadoRetro3D();
    paquete.recursos += MODELO_TESORERO_MARCO_REJA;
    paquete.definiciones += MODELO_TESORERO_MARCO_REJA;
    paquete.cantidad -= MODELO_TESORERO_MARCO_REJA;
    CargarPaqueteModelosEscenarioRetro3D(paquete);

    // Las piezas animadas necesitan estas primitives del GLB v1.
    const int mallasEsperadas[4] = { 5, 4, 3, 3 };
    for (int i = 0; i < paquete.cantidad; i++)
    {
        RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];
        if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i]) continue;
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.cargado = false;
        TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
    }
}


//==================================================
// DATOS LOGICOS DEL MAPA
//==================================================
//
// Bloques: 0 suelo, 1 torre del homenaje, 2..5 rejas (solo colisionan
// cuando estan bajadas).

static void ConstruirMapaTesorero(MinijuegoTesoreroAcorralado& minijuego)
{
    minijuego.cantidadBloques = 0;

    AgregarBloquePrueba(
        minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_TESORERO,
        { 0.0f, -0.5f, 0.0f },
        { MITAD_SUELO_X_TESORERO * 2.0f, 1.0f, MITAD_SUELO_Z_TESORERO * 2.0f },
        Color{ 120, 118, 112, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_TESORERO,
        { 0.0f, ALTO_TORRE_TESORERO * 0.5f, 0.0f },
        { MITAD_TORRE_TESORERO * 2.0f, ALTO_TORRE_TESORERO, MITAD_TORRE_TESORERO * 2.0f },
        Color{ 150, 146, 138, 255 }
    );

    static const float POSICIONES_X[MAX_REJAS_TESORERO] = { -5.0f, 5.0f, 0.0f, 0.0f };
    static const float POSICIONES_Z[MAX_REJAS_TESORERO] = { 0.0f, 0.0f, -4.2f, 4.2f };
    static const float MITADES_X[MAX_REJAS_TESORERO] = { 0.2f, 0.2f, 2.0f, 2.0f };
    static const float MITADES_Z[MAX_REJAS_TESORERO] = { 2.0f, 2.0f, 0.2f, 0.2f };

    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        RejaTesorero& reja = minijuego.rejas[k];
        reja = {};
        reja.x = POSICIONES_X[k];
        reja.z = POSICIONES_Z[k];
        reja.mitadX = MITADES_X[k];
        reja.mitadZ = MITADES_Z[k];
        reja.estado = REJA_TESORERO_ABIERTA;
        reja.tiempoEstado = 2.0f + 1.2f * (float)k;

        AgregarBloquePrueba(
            minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_TESORERO,
            { reja.x, ALTO_REJA_TESORERO * 0.5f, reja.z },
            { reja.mitadX * 2.0f, ALTO_REJA_TESORERO, reja.mitadZ * 2.0f },
            Color{ 60, 60, 66, 255 }
        );

        minijuego.bloques[2 + k].activaColision = false;
    }
}


// Hay algun jugador dentro (o casi) de la huella de la reja.
static bool HayJugadorEnRejaTesorero(
    const RejaTesorero& reja,
    const JugadorPrueba jugadores[],
    const MinijuegoTesoreroAcorralado& minijuego,
    int limite,
    float margen
)
{
    for (int i = 0; i < limite; i++)
    {
        if (minijuego.estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        if (
            AbsTesorero(jugadores[i].posicion.x - reja.x) < reja.mitadX + 0.4f + margen &&
            AbsTesorero(jugadores[i].posicion.z - reja.z) < reja.mitadZ + 0.4f + margen
        )
        {
            return true;
        }
    }

    return false;
}


static void ActualizarRejasTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        RejaTesorero& reja = minijuego.rejas[k];
        reja.tiempoEstado -= deltaTime;

        if (reja.estado == REJA_TESORERO_ABIERTA && reja.tiempoEstado <= 0.0f)
        {
            reja.estado = REJA_TESORERO_AVISO;
            reja.tiempoEstado = TIEMPO_AVISO_TESORERO;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);
        }
        else if (reja.estado == REJA_TESORERO_AVISO && reja.tiempoEstado <= 0.0f)
        {
            // No baja sobre un jugador: espera a que despeje.
            if (HayJugadorEnRejaTesorero(reja, jugadores, minijuego, limite, 0.0f))
            {
                reja.tiempoEstado = 0.0f;
            }
            else
            {
                reja.estado = REJA_TESORERO_CERRADA;
                reja.tiempoEstado = TIEMPO_CERRADA_TESORERO;
            }
        }
        else if (reja.estado == REJA_TESORERO_CERRADA && reja.tiempoEstado <= 0.0f)
        {
            reja.estado = REJA_TESORERO_ABIERTA;
            reja.tiempoEstado = TIEMPO_ABIERTA_TESORERO;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);
        }

        float objetivo = reja.estado == REJA_TESORERO_CERRADA ? 1.0f : 0.0f;

        if (reja.altura < objetivo)
        {
            reja.altura += VELOCIDAD_REJA_TESORERO * deltaTime;
            if (reja.altura > objetivo) reja.altura = objetivo;
        }
        else if (reja.altura > objetivo)
        {
            reja.altura -= VELOCIDAD_REJA_TESORERO * deltaTime;
            if (reja.altura < objetivo) reja.altura = objetivo;
        }

        minijuego.bloques[2 + k].activaColision =
            reja.estado == REJA_TESORERO_CERRADA &&
            reja.altura > 0.6f &&
            !HayJugadorEnRejaTesorero(reja, jugadores, minijuego, limite, -0.1f);
    }
}


//==================================================
// MONEDAS
//==================================================

static void SoltarMonedasTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    const JugadorPrueba& tesorero,
    int cantidad
)
{
    if (cantidad > minijuego.monedasTesorero)
    {
        cantidad = minijuego.monedasTesorero;
    }

    minijuego.monedasTesorero -= cantidad;
    minijuego.monedasPerdidas += cantidad;

    for (int k = 0; k < cantidad; k++)
    {
        int indice = -1;
        float mayorEdad = -1.0f;

        for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
        {
            if (!minijuego.monedas[i].activa)
            {
                indice = i;
                break;
            }

            if (minijuego.monedas[i].edad > mayorEdad)
            {
                mayorEdad = minijuego.monedas[i].edad;
                indice = i;
            }
        }

        if (indice < 0)
        {
            break;
        }

        float angulo = AleatorioTesorero(0.0f, 6.2831853f);
        float velocidad = AleatorioTesorero(1.8f, 4.8f);

        MonedaTesorero& moneda = minijuego.monedas[indice];
        moneda = {};
        moneda.activa = true;
        moneda.x = tesorero.posicion.x;
        moneda.y = tesorero.posicion.y + 0.4f;
        moneda.z = tesorero.posicion.z;
        moneda.velocidadX = std::cos(angulo) * velocidad;
        moneda.velocidadY = AleatorioTesorero(4.0f, 6.5f);
        moneda.velocidadZ = std::sin(angulo) * velocidad;
        moneda.tiempoSinRecoger = ESPERA_RECOGER_TESORERO;
    }
}


static void ActualizarMonedasTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        MonedaTesorero& moneda = minijuego.monedas[i];

        if (!moneda.activa)
        {
            continue;
        }

        moneda.edad += deltaTime;

        if (moneda.edad >= VIDA_MONEDA_TESORERO)
        {
            moneda.activa = false;
            continue;
        }

        if (moneda.tiempoSinRecoger > 0.0f)
        {
            moneda.tiempoSinRecoger -= deltaTime;
        }

        moneda.velocidadY -= GRAVEDAD_MONEDA_TESORERO * deltaTime;
        moneda.x += moneda.velocidadX * deltaTime;
        moneda.y += moneda.velocidadY * deltaTime;
        moneda.z += moneda.velocidadZ * deltaTime;

        if (moneda.y < SUELO_MONEDA_TESORERO)
        {
            moneda.y = SUELO_MONEDA_TESORERO;
            moneda.velocidadY = moneda.velocidadY < -1.5f ? -moneda.velocidadY * 0.4f : 0.0f;

            float freno = 1.0f - 4.0f * deltaTime;
            if (freno < 0.0f) freno = 0.0f;
            moneda.velocidadX *= freno;
            moneda.velocidadZ *= freno;
        }

        // Muros del patio.
        if (AbsTesorero(moneda.x) > LIMITE_X_TESORERO)
        {
            moneda.x = LimitarTesorero(moneda.x, -LIMITE_X_TESORERO, LIMITE_X_TESORERO);
            moneda.velocidadX = -moneda.velocidadX * 0.5f;
        }

        if (AbsTesorero(moneda.z) > LIMITE_Z_TESORERO)
        {
            moneda.z = LimitarTesorero(moneda.z, -LIMITE_Z_TESORERO, LIMITE_Z_TESORERO);
            moneda.velocidadZ = -moneda.velocidadZ * 0.5f;
        }

        // La torre y las rejas bajadas sacan a la moneda de su huella.
        for (int b = 1; b < minijuego.cantidadBloques; b++)
        {
            const BloquePrueba& bloque = minijuego.bloques[b];

            if (!bloque.activaColision)
            {
                continue;
            }

            float mitadX = bloque.tamano.x * 0.5f + 0.12f;
            float mitadZ = bloque.tamano.z * 0.5f + 0.12f;
            float dx = moneda.x - bloque.posicion.x;
            float dz = moneda.z - bloque.posicion.z;

            if (AbsTesorero(dx) >= mitadX || AbsTesorero(dz) >= mitadZ)
            {
                continue;
            }

            if (mitadX - AbsTesorero(dx) < mitadZ - AbsTesorero(dz))
            {
                moneda.x = bloque.posicion.x + (dx >= 0.0f ? mitadX : -mitadX);
                moneda.velocidadX = -moneda.velocidadX * 0.4f;
            }
            else
            {
                moneda.z = bloque.posicion.z + (dz >= 0.0f ? mitadZ : -mitadZ);
                moneda.velocidadZ = -moneda.velocidadZ * 0.4f;
            }
        }
    }
}


static void RecogerMonedasTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    const JugadorPrueba jugadores[],
    int limite
)
{
    for (int m = 0; m < MAX_MONEDAS_TESORERO; m++)
    {
        MonedaTesorero& moneda = minijuego.monedas[m];

        if (!moneda.activa || moneda.tiempoSinRecoger > 0.0f || moneda.y > 1.8f)
        {
            continue;
        }

        int mejor = -1;
        float mejorDistancia = RADIO_RECOGER_TESORERO;

        for (int i = 0; i < limite; i++)
        {
            if (minijuego.estadosJugadores[i].equipo < 0 || jugadores[i].cayendo)
            {
                continue;
            }

            float dx = jugadores[i].posicion.x - moneda.x;
            float dz = jugadores[i].posicion.z - moneda.z;
            float distancia = std::sqrt(dx * dx + dz * dz);

            if (distancia < mejorDistancia)
            {
                mejorDistancia = distancia;
                mejor = i;
            }
        }

        if (mejor < 0)
        {
            continue;
        }

        moneda.activa = false;

        // El tesorero recupera la moneda; el trio se la queda.
        if (minijuego.estadosJugadores[mejor].equipo == 0)
        {
            minijuego.monedasTesorero++;
            minijuego.monedasPerdidas--;
        }
        else
        {
            minijuego.estadosJugadores[mejor].monedasRobadas++;
        }

        if (minijuego.cooldownSonidoMoneda <= 0.0f)
        {
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_MONEDA);
            minijuego.cooldownSonidoMoneda = INTERVALO_SONIDO_MONEDA_TESORERO;
        }
    }
}


//==================================================
// GOLPES AL TESORERO
//==================================================

static void ResolverGolpesTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    JugadorPrueba jugadores[],
    int limite
)
{
    int t = minijuego.indiceTesorero;

    if (t < 0)
    {
        return;
    }

    JugadorPrueba& tesorero = jugadores[t];

    // Golpe horizontal de un miembro del trio.
    for (int i = 0; i < limite; i++)
    {
        JugadorPrueba& atacante = jugadores[i];

        if (
            i == t ||
            minijuego.estadosJugadores[i].equipo != 1 ||
            atacante.cayendo ||
            atacante.aplastado ||
            !atacante.golpeando ||
            atacante.golpeYaConecto ||
            tesorero.cayendo ||
            tesorero.aplastado ||
            tesorero.tiempoInmunidad > 0.0f
        )
        {
            continue;
        }

        float dx = tesorero.posicion.x - atacante.posicion.x;
        float dz = tesorero.posicion.z - atacante.posicion.z;
        float distancia = std::sqrt(dx * dx + dz * dz);

        if (distancia < 0.001f || distancia > ALCANCE_GOLPE_TESORERO)
        {
            continue;
        }

        if (AbsTesorero(tesorero.posicion.y - atacante.posicion.y) > 1.0f)
        {
            continue;
        }

        float normalX = dx / distancia;
        float normalZ = dz / distancia;
        float frente =
            normalX * atacante.direccionMirada.x +
            normalZ * atacante.direccionMirada.z;

        if (frente < 0.25f)
        {
            continue;
        }

        tesorero.empuje.x += normalX * FUERZA_GOLPE_JUGADOR_ESTANDAR;
        tesorero.empuje.z += normalZ * FUERZA_GOLPE_JUGADOR_ESTANDAR;
        tesorero.tiempoRalentizado = DURACION_RALENTIZACION_GOLPE;
        tesorero.multiplicadorRalentizacion = MULTIPLICADOR_VELOCIDAD_RALENTIZADO;
        tesorero.tiempoInmunidad = INMUNIDAD_TESORERO;
        atacante.golpeYaConecto = true;

        CrearParticulasImpactoGolpe(
            minijuego.particulas,
            MAX_PARTICULAS_TIERRA,
            { tesorero.posicion.x, tesorero.posicion.y + 0.2f, tesorero.posicion.z }
        );

        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GOLPE);
        SoltarMonedasTesorero(minijuego, tesorero, MONEDAS_GOLPE_TESORERO);
    }

    // Ground pound: aplasta a los rivales cercanos.
    for (int i = 0; i < limite; i++)
    {
        JugadorPrueba& atacante = jugadores[i];

        if (minijuego.estadosJugadores[i].equipo < 0 || !atacante.impactoGolpeSuelo)
        {
            continue;
        }

        atacante.impactoGolpeSuelo = false;

        for (int j = 0; j < limite; j++)
        {
            JugadorPrueba& objetivo = jugadores[j];

            if (
                j == i ||
                minijuego.estadosJugadores[j].equipo < 0 ||
                minijuego.estadosJugadores[j].equipo == minijuego.estadosJugadores[i].equipo ||
                objetivo.cayendo ||
                objetivo.aplastado ||
                objetivo.tiempoInmunidad > 0.0f
            )
            {
                continue;
            }

            float dx = objetivo.posicion.x - atacante.posicion.x;
            float dz = objetivo.posicion.z - atacante.posicion.z;

            if (
                std::sqrt(dx * dx + dz * dz) > RADIO_POUND_TESORERO ||
                AbsTesorero(objetivo.posicion.y - atacante.posicion.y) > 1.15f
            )
            {
                continue;
            }

            objetivo.aplastado = true;
            objetivo.tiempoAplastado = DURACION_APLASTADO_GROUND_POUND;
            objetivo.golpeando = false;
            objetivo.preparandoGolpeSuelo = false;
            objetivo.tiempoPreparacionGolpeSuelo = 0.0f;
            objetivo.golpeSueloActivo = false;
            objetivo.golpeSueloRecibido = true;
            objetivo.velocidad.x = 0.0f;
            objetivo.velocidad.z = 0.0f;
            objetivo.empuje = {};

            if (j == t)
            {
                objetivo.tiempoInmunidad = INMUNIDAD_TESORERO;
                SoltarMonedasTesorero(minijuego, objetivo, MONEDAS_POUND_TESORERO);
            }
        }
    }
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarTesorero(MinijuegoTesoreroAcorralado& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    // El tesorero gana si conserva mas de la mitad de sus monedas.
    minijuego.ganaTesorero =
        minijuego.monedasTesorero * 2 > MONEDAS_INICIALES_TESORERO;

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = minijuego.estadosJugadores[i].equipo;
        bool esTesorero = i == minijuego.indiceTesorero;

        resultadoJugador.numeroEquipo = esTesorero ? 0 : 1;
        resultadoJugador.puntuacionMinijuego = esTesorero
            ? minijuego.monedasTesorero
            : minijuego.estadosJugadores[i].monedasRobadas;
        resultadoJugador.puntosObtenidos = 0;
        resultadoJugador.posicionFinal =
            (esTesorero == minijuego.ganaTesorero) && equipo >= 0 ? 1 : 2;
    }

    minijuego.fase = FASE_TESORERO_TERMINADO;
}


//==================================================
// IA DE BOTS
//==================================================

// El segmento (ax,az)-(bx,bz) cruza la caja (cx,cz) +- (mx,mz). Devuelve el
// parametro t de entrada o -1 si no cruza.
static float SegmentoCruzaCajaTesorero(
    float ax, float az, float bx, float bz,
    float cx, float cz, float mx, float mz
)
{
    float tMin = 0.0f;
    float tMax = 1.0f;
    float origen[2] = { ax, az };
    float direccion[2] = { bx - ax, bz - az };
    float minimo[2] = { cx - mx, cz - mz };
    float maximo[2] = { cx + mx, cz + mz };

    for (int e = 0; e < 2; e++)
    {
        if (AbsTesorero(direccion[e]) < 0.0001f)
        {
            if (origen[e] < minimo[e] || origen[e] > maximo[e])
            {
                return -1.0f;
            }

            continue;
        }

        float t1 = (minimo[e] - origen[e]) / direccion[e];
        float t2 = (maximo[e] - origen[e]) / direccion[e];

        if (t1 > t2)
        {
            float temporal = t1;
            t1 = t2;
            t2 = temporal;
        }

        if (t1 > tMin) tMin = t1;
        if (t2 < tMax) tMax = t2;

        if (tMin > tMax)
        {
            return -1.0f;
        }
    }

    return tMin;
}


// Si un obstaculo (torre o reja bajada) corta el camino, desvia el objetivo
// hacia la esquina inflada que menos rodeo cause.
static void RodearObstaculosTesorero(
    const MinijuegoTesoreroAcorralado& minijuego,
    float desdeX, float desdeZ,
    float& objetivoX, float& objetivoZ
)
{
    int peor = -1;
    float peorT = 2.0f;

    for (int b = 1; b < minijuego.cantidadBloques; b++)
    {
        const BloquePrueba& bloque = minijuego.bloques[b];

        if (!bloque.activaColision)
        {
            continue;
        }

        float t = SegmentoCruzaCajaTesorero(
            desdeX, desdeZ, objetivoX, objetivoZ,
            bloque.posicion.x, bloque.posicion.z,
            bloque.tamano.x * 0.5f + 0.8f, bloque.tamano.z * 0.5f + 0.8f
        );

        if (t >= 0.0f && t < peorT)
        {
            peorT = t;
            peor = b;
        }
    }

    if (peor < 0)
    {
        return;
    }

    const BloquePrueba& bloque = minijuego.bloques[peor];
    float mitadX = bloque.tamano.x * 0.5f + 1.0f;
    float mitadZ = bloque.tamano.z * 0.5f + 1.0f;
    float mejorCoste = 1.0e9f;
    float mejorX = objetivoX;
    float mejorZ = objetivoZ;

    for (int k = 0; k < 4; k++)
    {
        float cx = bloque.posicion.x + (k % 2 == 0 ? -mitadX : mitadX);
        float cz = bloque.posicion.z + (k < 2 ? -mitadZ : mitadZ);

        if (AbsTesorero(cx) > LIMITE_X_TESORERO || AbsTesorero(cz) > LIMITE_Z_TESORERO)
        {
            continue;
        }

        float coste =
            std::sqrt((cx - desdeX) * (cx - desdeX) + (cz - desdeZ) * (cz - desdeZ)) +
            std::sqrt((cx - objetivoX) * (cx - objetivoX) + (cz - objetivoZ) * (cz - objetivoZ));

        if (coste < mejorCoste)
        {
            mejorCoste = coste;
            mejorX = cx;
            mejorZ = cz;
        }
    }

    objetivoX = mejorX;
    objetivoZ = mejorZ;
}


static bool PuntoEnObstaculoTesorero(
    const MinijuegoTesoreroAcorralado& minijuego,
    float x, float z,
    float margen
)
{
    for (int b = 1; b < minijuego.cantidadBloques; b++)
    {
        const BloquePrueba& bloque = minijuego.bloques[b];

        if (
            bloque.activaColision &&
            AbsTesorero(x - bloque.posicion.x) < bloque.tamano.x * 0.5f + margen &&
            AbsTesorero(z - bloque.posicion.z) < bloque.tamano.z * 0.5f + margen
        )
        {
            return true;
        }
    }

    return false;
}


// Tesorero: elige la direccion que mas lo aleja de los perseguidores,
// prefiriendo zonas abiertas y evitando esquinas y obstaculos.
static void DecidirHuidaTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    int indice,
    const JugadorPrueba jugadores[],
    int limite
)
{
    EstadoJugadorTesorero& estado = minijuego.estadosJugadores[indice];
    const JugadorPrueba& yo = jugadores[indice];

    float mejorPuntaje = -1.0e9f;
    float mejorX = yo.posicion.x;
    float mejorZ = yo.posicion.z;

    for (int k = 0; k < 16; k++)
    {
        float angulo = (float)k * 0.3926991f;
        float px = LimitarTesorero(yo.posicion.x + std::cos(angulo) * 3.2f, -LIMITE_X_TESORERO, LIMITE_X_TESORERO);
        float pz = LimitarTesorero(yo.posicion.z + std::sin(angulo) * 3.2f, -LIMITE_Z_TESORERO, LIMITE_Z_TESORERO);

        if (PuntoEnObstaculoTesorero(minijuego, px, pz, 0.9f))
        {
            continue;
        }

        float cruce = 1.0f;

        for (int b = 1; b < minijuego.cantidadBloques; b++)
        {
            const BloquePrueba& bloque = minijuego.bloques[b];

            if (
                bloque.activaColision &&
                SegmentoCruzaCajaTesorero(
                    yo.posicion.x, yo.posicion.z, px, pz,
                    bloque.posicion.x, bloque.posicion.z,
                    bloque.tamano.x * 0.5f + 0.6f, bloque.tamano.z * 0.5f + 0.6f
                ) >= 0.0f
            )
            {
                cruce = 0.0f;
            }
        }

        if (cruce <= 0.0f)
        {
            continue;
        }

        float distanciaMinima = 20.0f;

        for (int j = 0; j < limite; j++)
        {
            if (minijuego.estadosJugadores[j].equipo != 1)
            {
                continue;
            }

            float cx = jugadores[j].posicion.x + jugadores[j].velocidad.x * 0.4f;
            float cz = jugadores[j].posicion.z + jugadores[j].velocidad.z * 0.4f;
            float d = std::sqrt((px - cx) * (px - cx) + (pz - cz) * (pz - cz));

            if (d < distanciaMinima)
            {
                distanciaMinima = d;
            }
        }

        float holguraX = LIMITE_X_TESORERO - AbsTesorero(px);
        float holguraZ = LIMITE_Z_TESORERO - AbsTesorero(pz);
        float holgura = LimitarTesorero(holguraX < holguraZ ? holguraX : holguraZ, 0.0f, 3.0f);
        float esquina = (holguraX < 2.0f && holguraZ < 2.0f) ? 3.0f : 0.0f;

        float bonoMoneda = 0.0f;

        for (int m = 0; m < MAX_MONEDAS_TESORERO; m++)
        {
            const MonedaTesorero& moneda = minijuego.monedas[m];

            if (
                moneda.activa &&
                distanciaMinima > 3.5f &&
                AbsTesorero(moneda.x - px) < 1.5f &&
                AbsTesorero(moneda.z - pz) < 1.5f
            )
            {
                bonoMoneda = 2.0f;
            }
        }

        float puntaje =
            distanciaMinima +
            holgura * 0.8f -
            esquina +
            bonoMoneda +
            AleatorioTesorero(-0.4f, 0.4f);

        if (puntaje > mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejorX = px;
            mejorZ = pz;
        }
    }

    estado.objetivoX = mejorX;
    estado.objetivoZ = mejorZ;
}


static void DecidirPersecucionTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    int indice,
    const JugadorPrueba jugadores[]
)
{
    EstadoJugadorTesorero& estado = minijuego.estadosJugadores[indice];
    const JugadorPrueba& yo = jugadores[indice];
    const JugadorPrueba& tesorero = jugadores[minijuego.indiceTesorero];

    float dxT = tesorero.posicion.x - yo.posicion.x;
    float dzT = tesorero.posicion.z - yo.posicion.z;
    float distanciaTesorero = std::sqrt(dxT * dxT + dzT * dzT);
    bool tesoreroProtegido = tesorero.tiempoInmunidad > 0.0f || tesorero.aplastado;

    // Monedas sueltas cercanas: se priorizan si el tesorero esta protegido
    // o si la moneda esta mucho mas cerca que el.
    int mejorMoneda = -1;
    float mejorDistancia = 4.0f;

    for (int m = 0; m < MAX_MONEDAS_TESORERO; m++)
    {
        const MonedaTesorero& moneda = minijuego.monedas[m];

        if (!moneda.activa || moneda.y > 2.0f || GetRandomValue(1, 100) <= 8)
        {
            continue;
        }

        float dx = moneda.x - yo.posicion.x;
        float dz = moneda.z - yo.posicion.z;
        float distancia = std::sqrt(dx * dx + dz * dz);

        if (
            distancia < mejorDistancia &&
            (tesoreroProtegido || distancia < distanciaTesorero * 0.6f)
        )
        {
            mejorDistancia = distancia;
            mejorMoneda = m;
        }
    }

    float objetivoX;
    float objetivoZ;

    if (mejorMoneda >= 0)
    {
        objetivoX = minijuego.monedas[mejorMoneda].x;
        objetivoZ = minijuego.monedas[mejorMoneda].z;
    }
    else
    {
        // Prediccion leve y reparto lateral para cercar al tesorero.
        objetivoX = tesorero.posicion.x + tesorero.velocidad.x * 0.35f;
        objetivoZ = tesorero.posicion.z + tesorero.velocidad.z * 0.35f;

        if (distanciaTesorero > 3.0f)
        {
            float orden = (float)(indice % 3) - 1.0f;
            float largo = distanciaTesorero > 0.01f ? distanciaTesorero : 1.0f;
            objetivoX += (-dzT / largo) * orden * 1.6f;
            objetivoZ += (dxT / largo) * orden * 1.6f;
        }
    }

    objetivoX = LimitarTesorero(objetivoX, -LIMITE_X_TESORERO, LIMITE_X_TESORERO);
    objetivoZ = LimitarTesorero(objetivoZ, -LIMITE_Z_TESORERO, LIMITE_Z_TESORERO);
    estado.objetivoX = objetivoX;
    estado.objetivoZ = objetivoZ;
}


static InputMinijuegoParticipante CrearEntradaBotTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    int indice,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    EstadoJugadorTesorero& estado = minijuego.estadosJugadores[indice];
    const JugadorPrueba& yo = jugadores[indice];
    bool esTesorero = estado.equipo == 0;

    estado.tiempoDecision -= deltaTime;
    estado.cooldownAccionBot -= deltaTime;

    if (estado.tiempoRodeo > 0.0f)
    {
        estado.tiempoRodeo -= deltaTime;
    }

    if (estado.tiempoDecision <= 0.0f)
    {
        estado.tiempoDecision = AleatorioTesorero(0.25f, 0.35f);

        // Detecta quedarse atascado y corta con un rodeo aleatorio.
        float movido = std::sqrt(
            (yo.posicion.x - estado.ultimaX) * (yo.posicion.x - estado.ultimaX) +
            (yo.posicion.z - estado.ultimaZ) * (yo.posicion.z - estado.ultimaZ)
        );
        estado.ultimaX = yo.posicion.x;
        estado.ultimaZ = yo.posicion.z;

        if (movido < 0.2f && !yo.aplastado)
        {
            estado.tiempoSinProgreso += 0.3f;
        }
        else
        {
            estado.tiempoSinProgreso = 0.0f;
        }

        if (estado.tiempoSinProgreso > 0.9f)
        {
            estado.tiempoSinProgreso = 0.0f;
            estado.tiempoRodeo = 0.8f;
            estado.rodeoX = AleatorioTesorero(-LIMITE_X_TESORERO, LIMITE_X_TESORERO);
            estado.rodeoZ = AleatorioTesorero(-LIMITE_Z_TESORERO, LIMITE_Z_TESORERO);
        }

        if (esTesorero)
        {
            DecidirHuidaTesorero(minijuego, indice, jugadores, limite);
        }
        else
        {
            DecidirPersecucionTesorero(minijuego, indice, jugadores);
        }

        if (estado.tiempoRodeo <= 0.0f)
        {
            RodearObstaculosTesorero(
                minijuego, yo.posicion.x, yo.posicion.z,
                estado.objetivoX, estado.objetivoZ
            );
        }
    }

    float objetivoX = estado.tiempoRodeo > 0.0f ? estado.rodeoX : estado.objetivoX;
    float objetivoZ = estado.tiempoRodeo > 0.0f ? estado.rodeoZ : estado.objetivoZ;

    InputMinijuegoParticipante entrada{};

    float dx = objetivoX - yo.posicion.x;
    float dz = objetivoZ - yo.posicion.z;

    if (dx > 0.2f) entrada.derecha = true;
    if (dx < -0.2f) entrada.izquierda = true;
    if (dz > 0.2f) entrada.atras = true;
    if (dz < -0.2f) entrada.adelante = true;

    if (esTesorero)
    {
        // Defensa: si lo alcanzan, salta y cae con ground pound.
        float cercano = 99.0f;

        for (int j = 0; j < limite; j++)
        {
            if (minijuego.estadosJugadores[j].equipo != 1)
            {
                continue;
            }

            float cx = jugadores[j].posicion.x - yo.posicion.x;
            float cz = jugadores[j].posicion.z - yo.posicion.z;
            float d = std::sqrt(cx * cx + cz * cz);

            if (d < cercano)
            {
                cercano = d;
            }
        }

        if (
            estado.cooldownAccionBot <= 0.0f &&
            !yo.aplastado &&
            !yo.preparandoGolpeSuelo &&
            !yo.golpeSueloActivo
        )
        {
            if (yo.enSuelo && cercano < 1.3f && GetRandomValue(1, 100) <= 25)
            {
                entrada.saltar = true;
                estado.cooldownAccionBot = 0.12f;
            }
            else if (!yo.enSuelo && cercano < 1.2f)
            {
                entrada.saltar = true;
                estado.cooldownAccionBot = 3.0f;
            }
        }

        return entrada;
    }

    // Trio: golpea cuando el tesorero esta al alcance y de frente.
    const JugadorPrueba& tesorero = jugadores[minijuego.indiceTesorero];
    float tx = tesorero.posicion.x - yo.posicion.x;
    float tz = tesorero.posicion.z - yo.posicion.z;
    float distancia = std::sqrt(tx * tx + tz * tz);

    if (
        estado.cooldownAccionBot <= 0.0f &&
        distancia < 1.35f &&
        distancia > 0.01f &&
        tesorero.tiempoInmunidad <= 0.0f &&
        !tesorero.aplastado
    )
    {
        float frente = (tx * yo.direccionMirada.x + tz * yo.direccionMirada.z) / distancia;

        if (frente > 0.4f)
        {
            entrada.golpear = true;
            estado.cooldownAccionBot = AleatorioTesorero(0.4f, 0.8f);
        }
    }

    return entrada;
}


//==================================================
// ACTUALIZACION DE JUGADORES
//==================================================

static void ActualizarJugadoresTesorero(
    MinijuegoTesoreroAcorralado& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[]
)
{
    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorTesorero& estado = minijuego.estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        InputMinijuegoParticipante entrada{};

        if (EsControladoPorBotTesorero(participantes[i]))
        {
            entrada = CrearEntradaBotTesorero(minijuego, i, jugadores, limite, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        // El tesorero solo huye: no tiene golpe horizontal.
        if (estado.equipo == 0)
        {
            entrada.golpear = false;
        }

        float velocidadBase = jugador.velocidadMovimiento;

        // En 1 vs 1 el tesorero no corre mas rapido: un solo cazador
        // nunca podria alcanzarlo.
        int cazadores = 0;
        for (int k = 0; k < limite; k++)
        {
            if (minijuego.resultado.participantes[k].participo &&
                minijuego.estadosJugadores[k].equipo == 1)
            {
                cazadores++;
            }
        }
        float multiplicador = cazadores >= 2 ? MULTIPLICADOR_TESORERO : 1.0f;

        if (estado.equipo == 0)
        {
            jugador.velocidadMovimiento = velocidadBase * multiplicador;
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            minijuego.bloques,
            minijuego.cantidadBloques,
            minijuego.particulas,
            MAX_PARTICULAS_TIERRA,
            true,
            true,
            deltaTime
        );

        jugador.velocidadMovimiento = velocidadBase;

        // Red de seguridad: nunca sale del patio.
        jugador.posicion.x = LimitarTesorero(jugador.posicion.x, -LIMITE_X_TESORERO, LIMITE_X_TESORERO);
        jugador.posicion.z = LimitarTesorero(jugador.posicion.z, -LIMITE_Z_TESORERO, LIMITE_Z_TESORERO);

        if (jugador.cayendo || jugador.posicion.y < -3.0f)
        {
            ReiniciarJugadorPrueba(jugador);
        }
    }
}


//==================================================
// INICIALIZACION
//==================================================

void MinijuegoTesoreroAcorralado::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        monedas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_TIERRA; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_CHISPAS_TESORERO; i++)
    {
        chispas[i] = {};
    }

    for (int i = 0; i < MAX_AVISOS_TESORERO; i++)
    {
        avisos[i] = {};
    }

    semillaVisual = 2654435761u;
    destelloTesorero = 0.0f;
    tipoAnuncio = 0;
    tiempoAnuncio = -1.0f;
    tiempoFinal = 0.0f;
    avisoOroDado = false;

    ConstruirMapaTesorero(*this);

    // Camara diagonal que encuadra todo el patio y deja ver, detras de la
    // muralla del fondo, la torre del homenaje y el valle.
    camara.position = { 0.0f, 15.0f, 14.0f };
    camara.target = { 0.0f, 0.3f, -0.8f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_TESORERO_PREPARACION;
    indiceTesorero = -1;
    monedasTesorero = MONEDAS_INICIALES_TESORERO;
    monedasPerdidas = 0;
    ganaTesorero = false;
    partidaValida = false;

    tiempoPreparacion = DURACION_PREPARACION_TESORERO;
    tiempoRestante = DURACION_PARTIDA_TESORERO;
    tiempoAnimacion = 0.0f;
    cooldownSonidoMoneda = 0.0f;
}


void MinijuegoTesoreroAcorralado::Reiniciar(
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

    int limite = LimiteJugadoresTesorero(cantidadMaxima);
    int indices[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            indices[cantidad++] = i;
        }
    }

    resultado.cantidadEquipos = 2;

    if (cantidad < 2)
    {
        for (int k = 0; k < cantidad; k++)
        {
            resultado.participantes[indices[k]].numeroEquipo = 0;
        }

        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_TESORERO_TERMINADO;
        return;
    }

    CargarPiezasJugablesTesorero();
    partidaValida = true;
    indiceTesorero = indices[GetRandomValue(0, cantidad - 1)];

    static const float SPAWNS_TRIO_Z[3] = { -3.5f, 0.0f, 3.5f };
    int ordenTrio = 0;

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        bool esTesorero = indice == indiceTesorero;
        EstadoJugadorTesorero& estado = estadosJugadores[indice];

        estado.equipo = esTesorero ? 0 : 1;
        resultado.participantes[indice].numeroEquipo = esTesorero ? 0 : 1;

        Vector3 spawn = esTesorero
            ? Vector3{ 7.6f, MITAD_ALTO_JUGADOR_TESORERO, 0.0f }
            : Vector3{ -7.6f, MITAD_ALTO_JUGADOR_TESORERO, SPAWNS_TRIO_Z[ordenTrio % 3] };

        if (!esTesorero)
        {
            ordenTrio++;
        }

        ConfigurarJugadorMinijuegoEstandar(jugadores[indice], spawn);
        jugadores[indice].direccionMirada = { esTesorero ? -1.0f : 1.0f, 0.0f, 0.0f };
        jugadores[indice].enSuelo = true;

        estado.objetivoX = spawn.x;
        estado.objetivoZ = spawn.z;
        estado.ultimaX = spawn.x;
        estado.ultimaZ = spawn.z;
        estado.tiempoDecision = AleatorioTesorero(0.0f, 0.3f);
        estado.cooldownAccionBot = 0.0f;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoTesoreroAcorralado::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    InstantaneaTesorero antes;
    TomarInstantaneaTesorero(*this, antes);

    ActualizarLogica(deltaTime, jugadores, cantidadMaxima, participantes);
    ActualizarVisualTesorero(
        *this,
        deltaTime,
        jugadores,
        LimiteJugadoresTesorero(cantidadMaxima),
        participantes,
        antes
    );
}


void MinijuegoTesoreroAcorralado::ActualizarLogica(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_TESORERO_TERMINADO ||
        resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int limite = LimiteJugadoresTesorero(cantidadMaxima);

    if (fase == FASE_TESORERO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_TESORERO_JUGANDO;
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (cooldownSonidoMoneda > 0.0f)
    {
        cooldownSonidoMoneda -= deltaTime;
    }

    ActualizarRejasTesorero(*this, jugadores, limite, deltaTime);
    ActualizarJugadoresTesorero(*this, deltaTime, jugadores, limite, participantes);

    // Un humano desconectado lo controla la IA: debe seguir colisionando.
    Participante efectivos[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectivos[i] = participantes[i];

        if (efectivos[i].activo)
        {
            efectivos[i].conectado = true;
        }
    }

    ResolverGolpesTesorero(*this, jugadores, limite);
    ResolverColisionesJugadoresSinEmpuje(jugadores, efectivos, limite);
    ActualizarMonedasTesorero(*this, deltaTime);
    RecogerMonedasTesorero(*this, jugadores, limite);
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA, deltaTime);

    // Si ya no puede llegar a la mitad ni recuperando todo, gana el trio.
    bool imposible =
        (monedasTesorero + ContarMonedasSueltasTesorero(*this)) * 2 <=
        MONEDAS_INICIALES_TESORERO;

    if (tiempoRestante <= 0.0f || imposible)
    {
        tiempoRestante = tiempoRestante < 0.0f ? 0.0f : tiempoRestante;
        FinalizarTesorero(*this);
    }
}


//==================================================
// VISUAL: PALETA Y NIEBLA
//==================================================
//
// Patio de armas de un castillo encaramado en un risco, al atardecer:
// losas de piedra, murallas con adarve y almenas, torres con tejado de
// pizarra y gallardetes, torre del homenaje con ventanas encendidas, foso
// que cae en cascada al valle, aldea, campos y sierras en la calima. El
// oro es el acento: solo lo llevan las monedas, el cofre y el tesorero.

static const Color COLOR_CIELO_ALTO_TESORERO = { 92, 84, 148, 255 };
static const Color COLOR_CIELO_BAJO_TESORERO = { 236, 170, 140, 255 };
static const Color COLOR_NIEBLA_TESORERO = { 206, 156, 150, 255 };
static const Color COLOR_PIEDRA_TESORERO = { 182, 168, 148, 255 };
static const Color COLOR_PIEDRA_OSCURA_TESORERO = { 132, 120, 110, 255 };
static const Color COLOR_LOSA_TESORERO = { 140, 132, 132, 255 };
static const Color COLOR_JUNTA_TESORERO = { 74, 66, 72, 255 };
static const Color COLOR_MADERA_TESORERO = { 124, 82, 52, 255 };
static const Color COLOR_MADERA_OSCURA_TESORERO = { 84, 56, 40, 255 };
static const Color COLOR_HIERRO_TESORERO = { 66, 64, 72, 255 };
static const Color COLOR_PIZARRA_TESORERO = { 72, 78, 112, 255 };
static const Color COLOR_PASTO_TESORERO = { 112, 128, 70, 255 };
static const Color COLOR_ROCA_TESORERO = { 138, 116, 108, 255 };
static const Color COLOR_AGUA_TESORERO = { 52, 96, 112, 255 };
static const Color COLOR_TELA_TESORERO = { 108, 56, 134, 255 };
static const Color COLOR_CREMA_TESORERO = { 238, 222, 188, 255 };
static const Color COLOR_PAJA_TESORERO = { 214, 182, 104, 255 };
static const Color COLOR_ORO_TESORERO = { 255, 204, 64, 255 };
static const Color COLOR_ORO_CLARO_TESORERO = { 255, 238, 150, 255 };
static const Color COLOR_FUEGO_TESORERO = { 255, 150, 60, 255 };
static const Color COLOR_AVISO_TESORERO = { 255, 84, 44, 255 };

static const float DURACION_AVISO_TESORERO = 1.1f;
static const float DURACION_ANUNCIO_TESORERO = 1.8f;
static const float Y_VALLE_TESORERO = -16.0f;

// Ojo de referencia para la niebla (la camara de juego).
static const Vector3 OJO_NIEBLA_TESORERO = { 0.0f, 15.0f, 14.0f };


static float Limitar01VisualTesorero(float valor)
{
    return valor < 0.0f ? 0.0f : (valor > 1.0f ? 1.0f : valor);
}


static unsigned char CanalTesorero(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoTesorero(Color color, float factor)
{
    return Color{
        CanalTesorero((float)color.r * factor),
        CanalTesorero((float)color.g * factor),
        CanalTesorero((float)color.b * factor),
        color.a
    };
}


// Caras de arriba con la luz tibia del ocaso; laterales en penumbra lila.
static Color CaraTesorero(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalTesorero((float)color.r * factor * 1.06f),
            CanalTesorero((float)color.g * factor * 0.97f),
            CanalTesorero((float)color.b * factor * 0.88f),
            color.a
        };
    }

    return Color{
        CanalTesorero((float)color.r * factor * 0.9f),
        CanalTesorero((float)color.g * factor * 0.87f),
        CanalTesorero((float)color.b * factor * 1.04f),
        color.a
    };
}


static Color MezclaTesorero(Color a, Color b, float t)
{
    t = Limitar01VisualTesorero(t);

    return Color{
        CanalTesorero((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalTesorero((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalTesorero((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalTesorero((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaTesorero(Color color, float alfa)
{
    color.a = CanalTesorero(255.0f * alfa);
    return color;
}


// Calima del valle: lo lejano se funde con el aire rosado del ocaso.
static float CantidadNieblaTesorero(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_TESORERO.x;
    float dy = p.y - OJO_NIEBLA_TESORERO.y;
    float dz = p.z - OJO_NIEBLA_TESORERO.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 26.0f) / 80.0f;
    return n < 0.0f ? 0.0f : (n > 0.9f ? 0.9f : n);
}


static Color TinteTesorero(Color color, Vector3 p)
{
    Color niebla = COLOR_NIEBLA_TESORERO;
    niebla.a = color.a;
    return MezclaTesorero(color, niebla, CantidadNieblaTesorero(p));
}


//==================================================
// VISUAL: PRIMITIVAS
//==================================================

static void VerticeTesorero(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadTesorero(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTesorero(a, color);
    VerticeTesorero(b, color);
    VerticeTesorero(c, color);
    VerticeTesorero(d, color);
}


// Quad con la niebla calculada por vertice (suelos grandes y laderas).
static void QuadNieblaTesorero(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTesorero(a, TinteTesorero(color, a));
    VerticeTesorero(b, TinteTesorero(color, b));
    VerticeTesorero(c, TinteTesorero(color, c));
    VerticeTesorero(d, TinteTesorero(color, d));
}


static Vector3 SumaTesorero(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 EscalaTesorero(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.56). Sin niebla; respeta la matriz actual.
static void CuboCrudoTesorero(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadTesorero({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, CaraTesorero(color, 1.0f, true));
    QuadTesorero({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, CaraTesorero(color, 0.78f, false));
    QuadTesorero({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, CaraTesorero(color, 0.56f, false));
    QuadTesorero({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, CaraTesorero(color, 0.62f, false));
    QuadTesorero({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, CaraTesorero(color, 0.70f, false));
    QuadTesorero({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, CaraTesorero(color, 0.45f, false));
    rlEnd();
}


static void CuboTesorero(Vector3 c, Vector3 t, Color color)
{
    CuboCrudoTesorero(c, t, TinteTesorero(color, c));
}


// Caja girada en Y alrededor de su centro.
static void CuboGiradoTesorero(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    Color tinte = TinteTesorero(color, c);
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboCrudoTesorero({ 0.0f, 0.0f, 0.0f }, t, tinte);
    rlPopMatrix();
}


// Luz de las caras laterales segun su orientacion en XZ.
static float LuzLateralTesorero(float angulo)
{
    return 0.70f + 0.12f * std::sin(angulo) - 0.10f * std::cos(angulo + 0.5f);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (r1).
static void PrismaTesorero(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    float giro = 0.0f
)
{
    color = TinteTesorero(color, base);
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapa = CaraTesorero(color, 1.0f, true);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        Color cara = CaraTesorero(color, LuzLateralTesorero(a0 + paso * 0.5f), false);
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadTesorero(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadTesorero(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapa);
        }
    }

    rlEnd();
}


// Esfera (o elipsoide) con luz por normal: copas, bolsas, balas de canon.
static void EsferaTesorero(Vector3 c, Vector3 r, Color color, int anillos = 5, int lados = 9)
{
    color = TinteTesorero(color, c);
    rlBegin(RL_QUADS);

    for (int i = 0; i < anillos; i++)
    {
        float p0 = -1.5708f + 3.1416f * (float)i / (float)anillos;
        float p1 = -1.5708f + 3.1416f * (float)(i + 1) / (float)anillos;
        float pm = (p0 + p1) * 0.5f;

        for (int k = 0; k < lados; k++)
        {
            float a0 = 6.2832f * (float)k / (float)lados;
            float a1 = 6.2832f * (float)(k + 1) / (float)lados;
            float am = (a0 + a1) * 0.5f;
            float ny = std::sin(pm);
            float nx = std::cos(pm) * std::cos(am);
            float nz = std::cos(pm) * std::sin(am);
            float luz = 0.58f + 0.30f * ny + 0.14f * nz - 0.10f * nx;
            Color cara = CaraTesorero(color, luz, ny > 0.55f);

            QuadTesorero(
                { c.x + std::cos(p0) * std::cos(a0) * r.x, c.y + std::sin(p0) * r.y, c.z + std::cos(p0) * std::sin(a0) * r.z },
                { c.x + std::cos(p0) * std::cos(a1) * r.x, c.y + std::sin(p0) * r.y, c.z + std::cos(p0) * std::sin(a1) * r.z },
                { c.x + std::cos(p1) * std::cos(a1) * r.x, c.y + std::sin(p1) * r.y, c.z + std::cos(p1) * std::sin(a1) * r.z },
                { c.x + std::cos(p1) * std::cos(a0) * r.x, c.y + std::sin(p1) * r.y, c.z + std::cos(p1) * std::sin(a0) * r.z },
                cara
            );
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloTesorero(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadTesorero(
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
static void ResplandorTesorero(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloTesorero(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorTesorero(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano inclinado como la vista).
static void ResplandorFrenteTesorero(Vector3 c, float radio, Color color)
{
    ResplandorTesorero(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.71f, 0.70f }, radio, color);
}


// Destello en cruz (brillo de moneda u oro).
static void DestelloTesorero(Vector3 c, float largo, Color color, float giro)
{
    Vector3 u = { std::cos(giro), 0.0f, 0.0f };
    Vector3 v = { 0.0f, 0.71f, 0.70f };
    float s = std::sin(giro);
    Vector3 a = { u.x * largo, v.y * s * largo, v.z * s * largo };
    Vector3 b = { -std::sin(giro) * largo, v.y * std::cos(giro) * largo, v.z * std::cos(giro) * largo };
    float g = largo * 0.12f;
    Color c0 = color;
    c0.a = 0;

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < 2; k++)
    {
        Vector3 eje = k == 0 ? a : b;
        Vector3 lado = k == 0 ? EscalaTesorero(b, g / largo) : EscalaTesorero(a, g / largo);

        for (int s2 = -1; s2 <= 1; s2 += 2)
        {
            Vector3 punta = SumaTesorero(c, EscalaTesorero(eje, (float)s2));
            VerticeTesorero(SumaTesorero(c, lado), color);
            VerticeTesorero(SumaTesorero(c, EscalaTesorero(lado, -1.0f)), color);
            VerticeTesorero(punta, c0);
        }
    }
    rlEnd();
}


// Sillares sobre una cara plana: o = esquina inferior, u = eje del largo,
// v = eje del alto, n = normal hacia afuera de la cara.
static void SillaresTesorero(
    Vector3 o,
    Vector3 u,
    Vector3 v,
    Vector3 n,
    float largo,
    float alto,
    float hilada,
    float sillar,
    Color piedra,
    float luz,
    int semilla
)
{
    Vector3 centro = SumaTesorero(o, SumaTesorero(EscalaTesorero(u, largo * 0.5f), EscalaTesorero(v, alto * 0.5f)));
    Color junta = TinteTesorero(CaraTesorero(MezclaTesorero(piedra, COLOR_JUNTA_TESORERO, 0.55f), luz, false), centro);
    Vector3 off = EscalaTesorero(n, 0.012f);
    const float J = 0.035f;

    rlBegin(RL_QUADS);
    QuadTesorero(
        o,
        SumaTesorero(o, EscalaTesorero(u, largo)),
        SumaTesorero(o, SumaTesorero(EscalaTesorero(u, largo), EscalaTesorero(v, alto))),
        SumaTesorero(o, EscalaTesorero(v, alto)),
        junta
    );

    int filas = (int)std::ceil(alto / hilada);

    for (int f = 0; f < filas; f++)
    {
        float y0 = (float)f * hilada + J;
        float y1 = (float)(f + 1) * hilada - J;
        if (y1 > alto - J) y1 = alto - J;
        if (y1 <= y0) continue;

        float x = -sillar * 0.5f * Ruido01Tesorero(f, semilla + 3) - (f % 2 == 0 ? 0.0f : sillar * 0.45f);
        int k = 0;

        while (x < largo && k < 64)
        {
            float l = sillar * (0.7f + 0.6f * Ruido01Tesorero(f * 41 + k, semilla));
            float xa = (x < 0.0f ? 0.0f : x) + J;
            float xb = (x + l > largo ? largo : x + l) - J;

            if (xb > xa)
            {
                float h = Ruido01Tesorero(f * 59 + k * 7, semilla + 1);
                Color c = TonoTesorero(piedra, 0.86f + 0.18f * h);
                if (Ruido01Tesorero(f * 13 + k * 31, semilla + 2) > 0.88f) c = MezclaTesorero(c, Color{ 120, 128, 92, 255 }, 0.3f);
                if (f == 0) c = TonoTesorero(c, 0.86f);
                c = TinteTesorero(CaraTesorero(c, luz, false), centro);

                Vector3 a = SumaTesorero(SumaTesorero(o, off), SumaTesorero(EscalaTesorero(u, xa), EscalaTesorero(v, y0)));
                Vector3 b = SumaTesorero(SumaTesorero(o, off), SumaTesorero(EscalaTesorero(u, xb), EscalaTesorero(v, y0)));
                Vector3 cc = SumaTesorero(SumaTesorero(o, off), SumaTesorero(EscalaTesorero(u, xb), EscalaTesorero(v, y1)));
                Vector3 d = SumaTesorero(SumaTesorero(o, off), SumaTesorero(EscalaTesorero(u, xa), EscalaTesorero(v, y1)));
                QuadTesorero(a, b, cc, d, c);

                // Filo de luz arriba de cada sillar.
                Vector3 e = SumaTesorero(d, EscalaTesorero(v, -0.04f));
                Vector3 g = SumaTesorero(cc, EscalaTesorero(v, -0.04f));
                QuadTesorero(e, g, SumaTesorero(cc, EscalaTesorero(n, 0.002f)), SumaTesorero(d, EscalaTesorero(n, 0.002f)), TonoTesorero(c, 1.12f));
            }

            x += l;
            k++;
        }
    }

    rlEnd();
}


// Hilera de almenas entre a y b (eje X o Z) apoyada en y.
static void AlmenasTesorero(Vector3 a, Vector3 b, float grosor, float alto, float ancho, float hueco, Color color)
{
    bool enX = std::fabs(b.x - a.x) > std::fabs(b.z - a.z);
    float largo = enX ? std::fabs(b.x - a.x) : std::fabs(b.z - a.z);
    int cantidad = (int)((largo + hueco) / (ancho + hueco));
    if (cantidad < 1) cantidad = 1;
    float sobrante = largo - ((float)cantidad * ancho + (float)(cantidad - 1) * hueco);
    float inicio = (enX ? (a.x < b.x ? a.x : b.x) : (a.z < b.z ? a.z : b.z)) + sobrante * 0.5f + ancho * 0.5f;

    for (int k = 0; k < cantidad; k++)
    {
        float p = inicio + (float)k * (ancho + hueco);
        Color c = TonoTesorero(color, 0.92f + 0.12f * Ruido01Tesorero(k, (int)(a.x * 7.0f + a.z * 13.0f) + 400));
        Vector3 centro = enX
            ? Vector3{ p, a.y + alto * 0.5f, a.z }
            : Vector3{ a.x, a.y + alto * 0.5f, p };
        Vector3 tam = enX
            ? Vector3{ ancho, alto, grosor }
            : Vector3{ grosor, alto, ancho };
        CuboTesorero(centro, tam, c);
    }
}


// Tela colgada de una pared que ondea: arriba = centro del borde
// superior, n = normal de la pared (hacia donde se infla).
static void EstandarteTesorero(
    Vector3 arriba,
    Vector3 n,
    float ancho,
    float largo,
    float t,
    float fase,
    Color tela,
    Color franja
)
{
    const int TRAMOS = 8;
    Vector3 u = { std::fabs(n.z) > 0.5f ? 1.0f : 0.0f, 0.0f, std::fabs(n.x) > 0.5f ? 1.0f : 0.0f };

    // Barra de madera de la que cuelga.
    CuboTesorero(SumaTesorero(arriba, EscalaTesorero(n, 0.08f)), { u.x * (ancho + 0.3f) + std::fabs(n.x) * 0.1f, 0.09f, u.z * (ancho + 0.3f) + std::fabs(n.z) * 0.1f }, COLOR_MADERA_OSCURA_TESORERO);

    rlBegin(RL_QUADS);
    for (int k = 0; k < TRAMOS; k++)
    {
        float s0 = (float)k / (float)TRAMOS;
        float s1 = (float)(k + 1) / (float)TRAMOS;
        float o0 = 0.06f + std::sin(t * 2.4f + fase + s0 * 4.0f) * 0.12f * s0;
        float o1 = 0.06f + std::sin(t * 2.4f + fase + s1 * 4.0f) * 0.12f * s1;
        float d0 = std::sin(t * 1.7f + fase + s0 * 3.0f) * 0.05f * s0;
        float d1 = std::sin(t * 1.7f + fase + s1 * 3.0f) * 0.05f * s1;
        float y0 = arriba.y - largo * s0;
        float y1 = arriba.y - largo * s1;
        bool enFranja = k == 1 || k == TRAMOS - 3;
        float luz = 0.80f + 0.18f * std::sin(t * 2.4f + fase + s0 * 4.0f + 0.9f);
        Color c = TinteTesorero(CaraTesorero(enFranja ? franja : tela, luz, false), arriba);
        float media = ancho * 0.5f;

        Vector3 p0 = SumaTesorero({ arriba.x, y0, arriba.z }, SumaTesorero(EscalaTesorero(n, o0), EscalaTesorero(u, d0)));
        Vector3 p1 = SumaTesorero({ arriba.x, y1, arriba.z }, SumaTesorero(EscalaTesorero(n, o1), EscalaTesorero(u, d1)));

        if (k < TRAMOS - 1)
        {
            QuadTesorero(
                SumaTesorero(p0, EscalaTesorero(u, -media)), SumaTesorero(p0, EscalaTesorero(u, media)),
                SumaTesorero(p1, EscalaTesorero(u, media)), SumaTesorero(p1, EscalaTesorero(u, -media)), c);
        }
        else
        {
            // Cola de golondrina.
            Vector3 medio = EscalaTesorero(SumaTesorero(p0, p1), 0.5f);
            QuadTesorero(SumaTesorero(p0, EscalaTesorero(u, -media)), p0, medio, SumaTesorero(p1, EscalaTesorero(u, -media)), c);
            QuadTesorero(p0, SumaTesorero(p0, EscalaTesorero(u, media)), SumaTesorero(p1, EscalaTesorero(u, media)), medio, c);
        }
    }
    rlEnd();

    // Emblema: torre crema en el centro.
    Vector3 centro = SumaTesorero({ arriba.x, arriba.y - largo * 0.48f, arriba.z }, EscalaTesorero(n, 0.06f + std::sin(t * 2.4f + fase + 1.9f) * 0.06f + 0.012f));
    Color emblema = TinteTesorero(CaraTesorero(franja, 0.92f, false), centro);
    float e = ancho * 0.16f;
    rlBegin(RL_QUADS);
    QuadTesorero(SumaTesorero(centro, SumaTesorero(EscalaTesorero(u, -e), { 0.0f, -e * 1.4f, 0.0f })), SumaTesorero(centro, SumaTesorero(EscalaTesorero(u, e), { 0.0f, -e * 1.4f, 0.0f })),
        SumaTesorero(centro, SumaTesorero(EscalaTesorero(u, e), { 0.0f, e, 0.0f })), SumaTesorero(centro, SumaTesorero(EscalaTesorero(u, -e), { 0.0f, e, 0.0f })), emblema);
    for (int k = -1; k <= 1; k++)
    {
        Vector3 m = SumaTesorero(centro, SumaTesorero(EscalaTesorero(u, (float)k * e * 0.75f), { 0.0f, e, 0.0f }));
        QuadTesorero(SumaTesorero(m, EscalaTesorero(u, -e * 0.25f)), SumaTesorero(m, EscalaTesorero(u, e * 0.25f)),
            SumaTesorero(m, SumaTesorero(EscalaTesorero(u, e * 0.25f), { 0.0f, e * 0.5f, 0.0f })), SumaTesorero(m, SumaTesorero(EscalaTesorero(u, -e * 0.25f), { 0.0f, e * 0.5f, 0.0f })), emblema);
    }
    rlEnd();
}


// Gallardete horizontal que flamea en un mastil.
static void GallardeteTesorero(Vector3 punta, float largo, float t, float fase, Color tela)
{
    const int TRAMOS = 6;
    rlBegin(RL_QUADS);
    for (int k = 0; k < TRAMOS; k++)
    {
        float s0 = (float)k / (float)TRAMOS;
        float s1 = (float)(k + 1) / (float)TRAMOS;
        float a0 = 0.32f * (1.0f - s0);
        float a1 = 0.32f * (1.0f - s1);
        float o0 = std::sin(t * 4.2f + fase - s0 * 5.0f) * 0.22f * s0;
        float o1 = std::sin(t * 4.2f + fase - s1 * 5.0f) * 0.22f * s1;
        float luz = 0.82f + 0.2f * std::sin(t * 4.2f + fase - s0 * 5.0f + 1.2f);
        Color c = TinteTesorero(CaraTesorero(tela, luz, false), punta);
        Vector3 p0 = { punta.x + largo * s0, punta.y, punta.z + o0 };
        Vector3 p1 = { punta.x + largo * s1, punta.y, punta.z + o1 };
        QuadTesorero({ p0.x, p0.y - a0, p0.z }, { p1.x, p1.y - a1, p1.z }, { p1.x, p1.y + 0.02f, p1.z }, { p0.x, p0.y + 0.02f, p0.z }, c);
    }
    rlEnd();
}


//==================================================
// VISUAL: CIELO, VALLE Y RISCO
//==================================================

// Fondo 2D: el aire del ocaso que se ve mas alla del valle.
static void DibujarCieloTesorero()
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    DrawRectangleGradientV(0, 0, ancho, alto / 2, COLOR_CIELO_ALTO_TESORERO, COLOR_CIELO_BAJO_TESORERO);
    DrawRectangleGradientV(0, alto / 2, ancho, alto - alto / 2, COLOR_CIELO_BAJO_TESORERO, COLOR_NIEBLA_TESORERO);
}


// Loma redondeada del valle: una cinta de picos suaves que se hunde en
// la calima. z = fila, alto = altura maxima sobre el valle.
static void LomasTesorero(float z, float profundidad, float alto, float x0, float x1, int semilla, Color color)
{
    const int TRAMOS = 34;
    float paso = (x1 - x0) / (float)TRAMOS;

    rlBegin(RL_QUADS);
    for (int k = 0; k < TRAMOS; k++)
    {
        float xa = x0 + paso * (float)k;
        float xb = xa + paso;
        float ha = alto * (0.35f + 0.65f * (0.5f + 0.5f * std::sin(xa * 0.11f + (float)semilla) * std::sin(xa * 0.047f + (float)semilla * 1.7f)));
        float hb = alto * (0.35f + 0.65f * (0.5f + 0.5f * std::sin(xb * 0.11f + (float)semilla) * std::sin(xb * 0.047f + (float)semilla * 1.7f)));
        ha += 0.18f * alto * Ruido01Tesorero(k, semilla);
        hb += 0.18f * alto * Ruido01Tesorero(k + 1, semilla);

        float y = Y_VALLE_TESORERO;
        Vector3 pa = { xa, y + ha, z };
        Vector3 pb = { xb, y + hb, z };
        Color ladera = CaraTesorero(color, 0.92f + 0.1f * Ruido01Tesorero(k, semilla + 5), false);
        Color cresta = CaraTesorero(color, 1.08f, true);

        // Ladera de frente (hacia la camara) y lomo de arriba.
        QuadNieblaTesorero({ xa, y, z + profundidad }, { xb, y, z + profundidad }, { pb.x, pb.y, z + profundidad * 0.35f }, { pa.x, pa.y, z + profundidad * 0.35f }, ladera);
        QuadNieblaTesorero({ pa.x, pa.y, z + profundidad * 0.35f }, { pb.x, pb.y, z + profundidad * 0.35f }, pb, pa, cresta);
        QuadNieblaTesorero(pa, pb, { xb, y, z - profundidad * 0.5f }, { xa, y, z - profundidad * 0.5f }, TonoTesorero(cresta, 0.86f));
    }
    rlEnd();
}


static void CasaValleTesorero(Vector3 base, float ancho, float fondo, float alto, float giro, int semilla)
{
    Color pared = Ruido01Tesorero(semilla, 1) > 0.5f ? Color{ 222, 204, 168, 255 } : Color{ 196, 176, 150, 255 };
    Color techo = Ruido01Tesorero(semilla, 2) > 0.4f ? Color{ 156, 88, 66, 255 } : COLOR_PIZARRA_TESORERO;

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    Color paredT = TinteTesorero(pared, base);
    Color techoT = TinteTesorero(techo, base);
    CuboCrudoTesorero({ 0.0f, alto * 0.5f, 0.0f }, { ancho, alto, fondo }, paredT);

    // Tejado a dos aguas.
    float cumbre = alto + ancho * 0.45f;
    float a = ancho * 0.5f + 0.15f;
    float f = fondo * 0.5f + 0.15f;
    rlBegin(RL_QUADS);
    QuadTesorero({ -a, alto - 0.05f, f }, { 0.0f, cumbre, f }, { 0.0f, cumbre, -f }, { -a, alto - 0.05f, -f }, CaraTesorero(techoT, 1.0f, true));
    QuadTesorero({ a, alto - 0.05f, f }, { 0.0f, cumbre, f }, { 0.0f, cumbre, -f }, { a, alto - 0.05f, -f }, CaraTesorero(techoT, 0.74f, false));
    rlEnd();
    rlBegin(RL_TRIANGLES);
    VerticeTesorero({ -ancho * 0.5f, alto, fondo * 0.5f }, CaraTesorero(paredT, 0.78f, false));
    VerticeTesorero({ ancho * 0.5f, alto, fondo * 0.5f }, CaraTesorero(paredT, 0.78f, false));
    VerticeTesorero({ 0.0f, cumbre - 0.1f, fondo * 0.5f }, CaraTesorero(paredT, 0.78f, false));
    rlEnd();

    // Ventana encendida al frente.
    Color luz = TinteTesorero(Color{ 255, 200, 120, 255 }, base);
    rlBegin(RL_QUADS);
    QuadTesorero({ -0.2f, alto * 0.42f, fondo * 0.5f + 0.01f }, { 0.2f, alto * 0.42f, fondo * 0.5f + 0.01f }, { 0.2f, alto * 0.72f, fondo * 0.5f + 0.01f }, { -0.2f, alto * 0.72f, fondo * 0.5f + 0.01f }, luz);
    rlEnd();

    rlPopMatrix();
}


static void ArbolTesorero(Vector3 base, float alto, int semilla, bool simple)
{
    Color copa = Ruido01Tesorero(semilla, 3) > 0.7f ? Color{ 150, 132, 64, 255 } : Color{ 74, 98, 58, 255 };
    PrismaTesorero(base, 0.16f * alto * 0.3f, 0.1f * alto * 0.3f, alto * 0.45f, 5, COLOR_MADERA_OSCURA_TESORERO);
    float r = alto * 0.34f;
    int lados = simple ? 6 : 8;
    EsferaTesorero({ base.x, base.y + alto * 0.62f, base.z }, { r, r * 0.9f, r }, copa, simple ? 3 : 4, lados);

    if (!simple)
    {
        EsferaTesorero({ base.x + r * 0.5f, base.y + alto * 0.48f, base.z + r * 0.3f }, { r * 0.7f, r * 0.62f, r * 0.7f }, TonoTesorero(copa, 1.08f), 3, lados);
    }
}


static void DibujarValleTesorero(float t, bool detalle, int paso)
{
    const float Y = Y_VALLE_TESORERO;

    // Campos en damero que la calima va borrando.
    const Color campos[4] =
    {
        { 132, 142, 76, 255 }, { 180, 160, 96, 255 }, { 108, 120, 66, 255 }, { 152, 122, 86, 255 }
    };
    const float CELDA = 9.0f;

    rlBegin(RL_QUADS);
    for (int i = -12; i < 12; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            float x0 = (float)i * CELDA;
            float x1 = x0 + CELDA;
            float z1 = 22.0f - (float)j * CELDA;
            float z0 = z1 - CELDA;

            // Bajo el risco no hace falta dibujar.
            if (x1 > -19.0f && x0 < 19.0f && z0 > -14.0f) continue;

            Color c = campos[(int)(Ruido01Tesorero(i * 31 + j, 70) * 3.99f)];
            c = TonoTesorero(c, 0.92f + 0.12f * Ruido01Tesorero(i, j + 71));
            QuadNieblaTesorero({ x0, Y, z1 }, { x1, Y, z1 }, { x1, Y, z0 }, { x0, Y, z0 }, CaraTesorero(c, 1.0f, true));

            if (detalle)
            {
                // Cerco vivo (seto) en el borde de cada campo.
                Color seto = CaraTesorero(Color{ 70, 90, 56, 255 }, 0.95f, true);
                QuadNieblaTesorero({ x0, Y + 0.02f, z1 }, { x1, Y + 0.02f, z1 }, { x1, Y + 0.02f, z1 - 0.35f }, { x0, Y + 0.02f, z1 - 0.35f }, seto);
            }
        }
    }
    rlEnd();

    // Rio que serpentea por el valle y refleja el cielo.
    rlBegin(RL_QUADS);
    for (int k = 0; k < 40; k++)
    {
        float za = -14.0f - (float)k * 2.6f;
        float zb = za - 2.6f;
        float xa = -26.0f + 9.0f * std::sin(za * 0.07f) + 4.0f * std::sin(za * 0.17f);
        float xb = -26.0f + 9.0f * std::sin(zb * 0.07f) + 4.0f * std::sin(zb * 0.17f);
        float brillo = detalle ? 0.5f + 0.5f * std::sin(t * 1.6f + za * 0.6f) : 0.5f;
        Color agua = MezclaTesorero(Color{ 120, 128, 160, 255 }, Color{ 236, 190, 170, 255 }, 0.25f + 0.25f * brillo);
        QuadNieblaTesorero({ xa - 1.5f, Y + 0.03f, za }, { xa + 1.5f, Y + 0.03f, za }, { xb + 1.5f, Y + 0.03f, zb }, { xb - 1.5f, Y + 0.03f, zb }, agua);
    }
    rlEnd();

    // Dos cordones de lomas: el de atras mas palido por la calima.
    LomasTesorero(-66.0f, 14.0f, 9.0f, -150.0f, 150.0f, 7, Color{ 128, 120, 132, 255 });
    LomasTesorero(-48.0f, 12.0f, 6.0f, -120.0f, 120.0f, 3, Color{ 96, 112, 78, 255 });

    if (!detalle)
    {
        return;
    }

    // Aldea al pie del risco.
    const Vector3 casas[12] =
    {
        { -34.0f, Y, -26.0f }, { -30.0f, Y, -30.0f }, { -37.0f, Y, -33.0f }, { -28.0f, Y, -24.0f },
        { -40.0f, Y, -27.0f }, { -33.0f, Y, -38.0f },
        { 27.0f, Y, -22.0f }, { 31.0f, Y, -27.0f }, { 25.0f, Y, -30.0f }, { 35.0f, Y, -24.0f },
        { 29.0f, Y, -35.0f }, { 38.0f, Y, -31.0f }
    };
    for (int k = 0; k < 12; k += paso)
    {
        float ancho = 2.2f + 1.2f * Ruido01Tesorero(k, 80);
        CasaValleTesorero(casas[k], ancho, 2.0f + 1.0f * Ruido01Tesorero(k, 81), 1.6f + 0.8f * Ruido01Tesorero(k, 82), 30.0f * Ruido01Tesorero(k, 83) - 15.0f, k * 5 + 1);
    }

    // Campanario de la aldea.
    PrismaTesorero({ 33.0f, Y, -30.0f }, 1.0f, 0.9f, 6.0f, 4, Color{ 210, 192, 160, 255 }, 0.785f);
    PrismaTesorero({ 33.0f, Y + 6.0f, -30.0f }, 1.3f, 0.0f, 3.0f, 4, COLOR_PIZARRA_TESORERO, 0.785f);

    // Bosquecillos.
    for (int k = 0; k < 36; k += paso)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        Vector3 p = { lado * (22.0f + 34.0f * Ruido01Tesorero(k, 90)), Y, -16.0f - 36.0f * Ruido01Tesorero(k, 91) };
        if (k % 5 == 0) p = { -60.0f + 120.0f * Ruido01Tesorero(k, 92), Y, -40.0f - 8.0f * Ruido01Tesorero(k, 93) };
        ArbolTesorero(p, 3.0f + 2.0f * Ruido01Tesorero(k, 94), k, true);
    }
}


// Paredes del risco: estratos de roca que caen del patio al valle.
static void CaraRiscoTesorero(Vector3 a, Vector3 b, Vector3 n, int semilla)
{
    const int COLUMNAS = 10;
    const float BANDA = 2.0f;
    int bandas = (int)(-Y_VALLE_TESORERO / BANDA);

    rlBegin(RL_QUADS);
    for (int c = 0; c < COLUMNAS; c++)
    {
        float u0 = (float)c / (float)COLUMNAS;
        float u1 = (float)(c + 1) / (float)COLUMNAS;
        Vector3 p0 = { a.x + (b.x - a.x) * u0, 0.0f, a.z + (b.z - a.z) * u0 };
        Vector3 p1 = { a.x + (b.x - a.x) * u1, 0.0f, a.z + (b.z - a.z) * u1 };

        for (int k = 0; k < bandas; k++)
        {
            float y0 = -(float)k * BANDA;
            float y1 = y0 - BANDA;
            // Cada estrato sobresale distinto: la pared queda escalonada.
            float salida = 0.25f * (float)k + 0.6f * Ruido01Tesorero(c * 17 + k, semilla);
            float tono = 0.82f + 0.2f * Ruido01Tesorero(k * 7 + c, semilla + 1);
            Color roca = k % 3 == 1 ? Color{ 160, 132, 118, 255 } : COLOR_ROCA_TESORERO;
            Color cara = CaraTesorero(TonoTesorero(roca, tono), 0.78f, false);
            Vector3 o = EscalaTesorero(n, salida);
            Vector3 q0 = SumaTesorero(p0, o);
            Vector3 q1 = SumaTesorero(p1, o);

            QuadNieblaTesorero({ q0.x, y0, q0.z }, { q1.x, y0, q1.z }, { q1.x, y1, q1.z }, { q0.x, y1, q0.z }, cara);

            // Cornisa del estrato (arriba, a la luz).
            Vector3 r0 = SumaTesorero(p0, EscalaTesorero(n, 0.25f * (float)(k - 1) + 0.3f));
            Vector3 r1 = SumaTesorero(p1, EscalaTesorero(n, 0.25f * (float)(k - 1) + 0.3f));
            if (k > 0)
            {
                QuadNieblaTesorero({ r0.x, y0, r0.z }, { r1.x, y0, r1.z }, { q1.x, y0, q1.z }, { q0.x, y0, q0.z }, CaraTesorero(TonoTesorero(roca, tono * 1.1f), 1.0f, true));
            }
        }
    }
    rlEnd();
}


// Explanada del risco alrededor del castillo (pasto con matas).
static void ParcelaPastoTesorero(float x0, float x1, float z0, float z1, int semilla)
{
    const float CELDA = 1.6f;
    int nx = (int)std::ceil((x1 - x0) / CELDA);
    int nz = (int)std::ceil((z1 - z0) / CELDA);

    rlBegin(RL_QUADS);
    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < nz; j++)
        {
            float a0 = x0 + (float)i * CELDA;
            float a1 = a0 + CELDA > x1 ? x1 : a0 + CELDA;
            float b0 = z0 + (float)j * CELDA;
            float b1 = b0 + CELDA > z1 ? z1 : b0 + CELDA;
            float h = Ruido01Tesorero(i * 23 + j, semilla);
            Color c = TonoTesorero(COLOR_PASTO_TESORERO, 0.88f + 0.16f * h);
            if (Ruido01Tesorero(i * 5 + j * 11, semilla + 1) > 0.86f) c = MezclaTesorero(c, Color{ 150, 128, 90, 255 }, 0.45f);
            QuadNieblaTesorero({ a0, 0.0f, b1 }, { a1, 0.0f, b1 }, { a1, 0.0f, b0 }, { a0, 0.0f, b0 }, CaraTesorero(c, 1.0f, true));
        }
    }
    rlEnd();
}


static void DibujarRiscoTesorero(bool detalle)
{
    // Explanada: franjas a los lados del foso y detras de la muralla.
    ParcelaPastoTesorero(-19.5f, -13.8f, -14.0f, 16.0f, 50);
    ParcelaPastoTesorero(13.8f, 19.5f, -14.0f, 16.0f, 51);
    ParcelaPastoTesorero(-11.0f, 11.0f, -14.0f, -9.2f, 52);

    // Camino de tierra que baja hacia la aldea.
    rlBegin(RL_QUADS);
    for (int k = 0; k < 8; k++)
    {
        float za = -9.6f - (float)k * 0.6f;
        float zb = za - 0.6f;
        float xa = 6.2f + (float)k * 0.25f;
        Color tierra = CaraTesorero(TonoTesorero(Color{ 160, 132, 96, 255 }, 0.92f + 0.08f * Ruido01Tesorero(k, 53)), 1.0f, true);
        QuadNieblaTesorero({ xa, 0.01f, za }, { xa + 1.6f, 0.01f, za }, { xa + 1.85f, 0.01f, zb }, { xa + 0.25f, 0.01f, zb }, tierra);
    }
    rlEnd();

    // Paredes de roca: atras y a los dos lados.
    CaraRiscoTesorero({ -19.5f, 0.0f, -14.0f }, { 19.5f, 0.0f, -14.0f }, { 0.0f, 0.0f, -1.0f }, 60);
    CaraRiscoTesorero({ -19.5f, 0.0f, 16.0f }, { -19.5f, 0.0f, -14.0f }, { -1.0f, 0.0f, 0.0f }, 61);
    CaraRiscoTesorero({ 19.5f, 0.0f, -14.0f }, { 19.5f, 0.0f, 16.0f }, { 1.0f, 0.0f, 0.0f }, 62);

    // Borde de pasto colgante sobre el risco.
    rlBegin(RL_QUADS);
    Color borde = CaraTesorero(TonoTesorero(COLOR_PASTO_TESORERO, 0.8f), 0.9f, false);
    QuadNieblaTesorero({ -19.6f, 0.02f, -14.1f }, { 19.6f, 0.02f, -14.1f }, { 19.6f, -0.35f, -14.2f }, { -19.6f, -0.35f, -14.2f }, borde);
    QuadNieblaTesorero({ -19.6f, 0.02f, 16.0f }, { -19.6f, 0.02f, -14.1f }, { -19.7f, -0.35f, -14.1f }, { -19.7f, -0.35f, 16.0f }, borde);
    QuadNieblaTesorero({ 19.6f, 0.02f, 16.0f }, { 19.6f, 0.02f, -14.1f }, { 19.7f, -0.35f, -14.1f }, { 19.7f, -0.35f, 16.0f }, borde);
    rlEnd();

    if (!detalle)
    {
        return;
    }

    // Rocas sueltas en el borde.
    for (int k = 0; k < 10; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        Vector3 p = { lado * (14.6f + 4.4f * Ruido01Tesorero(k, 54)), 0.0f, -13.0f + 26.0f * Ruido01Tesorero(k, 55) };
        if (k >= 7) p = { -9.0f + 18.0f * Ruido01Tesorero(k, 56), 0.0f, -13.4f };
        float r = 0.35f + 0.35f * Ruido01Tesorero(k, 57);
        EsferaTesorero({ p.x, r * 0.4f, p.z }, { r * 1.3f, r, r }, TonoTesorero(COLOR_ROCA_TESORERO, 0.9f + 0.15f * Ruido01Tesorero(k, 58)), 3, 6);
    }
}


//==================================================
// VISUAL: FOSO Y CASCADAS
//==================================================

static const float X_FOSO_INTERIOR_TESORERO = 11.0f;
static const float X_FOSO_EXTERIOR_TESORERO = 13.6f;
static const float Y_AGUA_TESORERO = -0.75f;


static void DibujarFosoTesorero(float t, bool animado)
{
    const float XI = X_FOSO_INTERIOR_TESORERO;
    const float XE = X_FOSO_EXTERIOR_TESORERO;
    const float Y = Y_AGUA_TESORERO;

    for (int s = -1; s <= 1; s += 2)
    {
        float lado = (float)s;

        // Paredes de contencion del foso (la de afuera se ve desde el patio).
        SillaresTesorero({ lado * XE, Y - 0.2f, 16.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { -lado, 0.0f, 0.0f }, 30.0f, -Y + 0.2f, 0.38f, 0.9f, COLOR_PIEDRA_OSCURA_TESORERO, 0.74f, 300 + s);
        rlBegin(RL_QUADS);
        QuadNieblaTesorero({ lado * XI, Y - 0.2f, 16.0f }, { lado * XI, Y - 0.2f, -14.0f }, { lado * XI, 0.0f, -14.0f }, { lado * XI, 0.0f, 16.0f }, CaraTesorero(COLOR_PIEDRA_OSCURA_TESORERO, 0.66f, false));
        rlEnd();

        // Albardilla del borde exterior.
        CuboTesorero({ lado * (XE + 0.18f), 0.06f, 1.0f }, { 0.36f, 0.12f, 30.0f }, COLOR_PIEDRA_TESORERO);

        // Agua en tramos: corre hacia el fondo y cae por el risco.
        rlBegin(RL_QUADS);
        for (int k = 0; k < 15; k++)
        {
            float z0 = 16.0f - (float)k * 2.0f;
            float z1 = z0 - 2.0f;
            Color agua = TonoTesorero(COLOR_AGUA_TESORERO, 0.94f + 0.08f * Ruido01Tesorero(k, 310 + s));
            QuadNieblaTesorero({ lado * XI, Y, z0 }, { lado * XE, Y, z0 }, { lado * XE, Y, z1 }, { lado * XI, Y, z1 }, CaraTesorero(agua, 1.0f, true));
        }

        // Reflejo del cielo cerca de la orilla de afuera.
        Color reflejo = AlfaTesorero(COLOR_CIELO_BAJO_TESORERO, 0.0f);
        Color reflejoFuerte = MezclaTesorero(COLOR_AGUA_TESORERO, COLOR_CIELO_BAJO_TESORERO, 0.45f);
        VerticeTesorero({ lado * (XE - 0.8f), Y + 0.005f, 16.0f }, TinteTesorero(MezclaTesorero(reflejoFuerte, reflejo, 1.0f), { lado * XE, Y, 16.0f }));
        VerticeTesorero({ lado * XE, Y + 0.005f, 16.0f }, TinteTesorero(reflejoFuerte, { lado * XE, Y, 16.0f }));
        VerticeTesorero({ lado * XE, Y + 0.005f, -14.0f }, TinteTesorero(reflejoFuerte, { lado * XE, Y, -14.0f }));
        VerticeTesorero({ lado * (XE - 0.8f), Y + 0.005f, -14.0f }, TinteTesorero(MezclaTesorero(reflejoFuerte, reflejo, 1.0f), { lado * XE, Y, -14.0f }));

        if (animado)
        {
            // Ondas que corren hacia la cascada.
            for (int k = 0; k < 14; k++)
            {
                float u = std::fmod(t * 0.18f + Ruido01Tesorero(k, 320 + s), 1.0f);
                float z = 16.0f - 30.0f * u;
                float x = lado * (XI + 0.3f + 2.0f * Ruido01Tesorero(k, 321 + s));
                float largo = 0.6f + 0.8f * Ruido01Tesorero(k, 322);
                float alfa = std::sin(u * 3.1416f) * 0.55f;
                Color c = TinteTesorero(AlfaTesorero(Color{ 214, 226, 228, 255 }, alfa), { x, Y, z });
                QuadTesorero({ x - 0.05f, Y + 0.01f, z }, { x + 0.05f, Y + 0.01f, z }, { x + 0.05f, Y + 0.01f, z - largo }, { x - 0.05f, Y + 0.01f, z - largo }, c);
            }
        }
        rlEnd();

        // Cascada: el foso se derrama por el risco hacia el valle.
        float xc = lado * (XI + XE) * 0.5f;
        float media = (XE - XI) * 0.5f;
        rlBegin(RL_QUADS);
        const int TRAMOS = 8;
        for (int k = 0; k < TRAMOS; k++)
        {
            float y0 = Y + (Y_VALLE_TESORERO - Y) * (float)k / (float)TRAMOS;
            float y1 = Y + (Y_VALLE_TESORERO - Y) * (float)(k + 1) / (float)TRAMOS;
            float z0 = -14.0f - 0.3f - 0.18f * (float)k;
            float z1 = -14.0f - 0.3f - 0.18f * (float)(k + 1);
            Color caida = MezclaTesorero(COLOR_AGUA_TESORERO, Color{ 220, 230, 232, 255 }, 0.35f + 0.05f * (float)k);
            QuadNieblaTesorero({ xc - media, y0, z0 }, { xc + media, y0, z0 }, { xc + media * 1.1f, y1, z1 }, { xc - media * 1.1f, y1, z1 }, CaraTesorero(caida, 0.95f, false));

            if (animado)
            {
                // Hilos de espuma que bajan.
                for (int h = 0; h < 5; h++)
                {
                    float u = std::fmod(t * 0.9f + Ruido01Tesorero(h, 330 + s) + (float)k * 0.13f, 1.0f);
                    float xh = xc - media + 2.0f * media * (0.1f + 0.8f * Ruido01Tesorero(h * 3 + k, 331));
                    float ya = y0 + (y1 - y0) * u;
                    float yb = ya + (y1 - y0) * 0.35f;
                    Color espuma = TinteTesorero(Color{ 240, 244, 246, 200 }, { xh, ya, z0 });
                    QuadTesorero({ xh - 0.07f, ya, z0 - 0.02f }, { xh + 0.07f, ya, z0 - 0.02f }, { xh + 0.07f, yb, z1 - 0.02f }, { xh - 0.07f, yb, z1 - 0.02f }, espuma);
                }
            }
        }
        rlEnd();
    }
}


//==================================================
// VISUAL: PATIO DE ARMAS (ARENA)
//==================================================

// Losas a soga en hileras, juntas oscuras, musgo en las orillas y
// penumbra al pie de los muros. Ocupa exactamente el suelo logico.
static void DibujarPatioTesorero(const MinijuegoTesoreroAcorralado& m, bool detalle)
{
    const float X = MITAD_SUELO_X_TESORERO;
    const float Z = MITAD_SUELO_Z_TESORERO;
    const float FILA = 0.7f;
    const int FILAS = 20;
    const float J = 0.035f;

    rlBegin(RL_QUADS);
    QuadTesorero({ -X, 0.0f, Z }, { X, 0.0f, Z }, { X, 0.0f, -Z }, { -X, 0.0f, -Z }, COLOR_JUNTA_TESORERO);

    // Frente del bordillo del patio (se ve sobre el parapeto bajo).
    QuadTesorero({ -X, -0.4f, Z }, { X, -0.4f, Z }, { X, 0.0f, Z }, { -X, 0.0f, Z }, CaraTesorero(COLOR_PIEDRA_OSCURA_TESORERO, 0.7f, false));

    for (int f = 0; f < FILAS; f++)
    {
        float z0 = -Z + FILA * (float)f + J;
        float z1 = -Z + FILA * (float)(f + 1) - J;
        float x = -X - 0.9f * Ruido01Tesorero(f, 10);
        int k = 0;

        while (x < X && k < 40)
        {
            float largo = 0.85f + 0.75f * Ruido01Tesorero(f * 37 + k, 11);
            float xa = (x < -X ? -X : x) + J;
            float xb = (x + largo > X ? X : x + largo) - J;

            if (xb > xa)
            {
                float h = Ruido01Tesorero(f * 53 + k * 7, 12);
                Color c = TonoTesorero(COLOR_LOSA_TESORERO, 0.9f + 0.15f * h);
                float r = Ruido01Tesorero(f * 29 + k * 3, 13);
                if (r > 0.9f) c = MezclaTesorero(c, Color{ 178, 156, 126, 255 }, 0.45f);
                else if (r < 0.08f) c = MezclaTesorero(c, Color{ 122, 120, 128, 255 }, 0.4f);

                // Musgo y penumbra junto a los muros.
                float cx = (xa + xb) * 0.5f;
                float cz = (z0 + z1) * 0.5f;
                float borde = X - std::fabs(cx) < Z - std::fabs(cz) ? X - std::fabs(cx) : Z - std::fabs(cz);
                if (borde < 1.0f && Ruido01Tesorero(f * 11 + k, 14) > 0.55f) c = MezclaTesorero(c, Color{ 112, 128, 80, 255 }, 0.3f);
                c = TonoTesorero(c, 0.86f + 0.14f * Limitar01VisualTesorero(borde / 1.8f));

                Color arriba = CaraTesorero(c, 1.0f, true);
                QuadTesorero({ xa, 0.004f, z1 }, { xb, 0.004f, z1 }, { xb, 0.004f, z0 }, { xa, 0.004f, z0 }, arriba);

                if (detalle)
                {
                    // Bisel: filo claro al fondo, sombra al frente.
                    QuadTesorero({ xa, 0.006f, z0 + 0.06f }, { xb, 0.006f, z0 + 0.06f }, { xb, 0.006f, z0 }, { xa, 0.006f, z0 }, TonoTesorero(arriba, 1.08f));
                    QuadTesorero({ xa, 0.006f, z1 }, { xb, 0.006f, z1 }, { xb, 0.006f, z1 - 0.06f }, { xa, 0.006f, z1 - 0.06f }, TonoTesorero(arriba, 0.86f));

                    // Grieta.
                    if (Ruido01Tesorero(f * 3 + 2, k * 9 + 13) < 0.08f)
                    {
                        Color g = TonoTesorero(arriba, 0.62f);
                        float mx = (xa + xb) * 0.5f;
                        QuadTesorero({ xa + 0.1f, 0.008f, z0 + 0.12f }, { mx, 0.008f, cz - 0.015f }, { mx, 0.008f, cz + 0.015f }, { xa + 0.1f, 0.008f, z0 + 0.14f }, g);
                        QuadTesorero({ mx, 0.008f, cz - 0.015f }, { xb - 0.12f, 0.008f, z1 - 0.08f }, { xb - 0.12f, 0.008f, z1 - 0.06f }, { mx, 0.008f, cz + 0.015f }, g);
                    }
                }
            }

            x += largo;
            k++;
        }
    }

    // Losas de la plaza del tesoro: un marco de piedra clara alrededor del
    // pedestal central (fuera de su huella).
    const float P = MITAD_TORRE_TESORERO;
    Color marco = CaraTesorero(Color{ 176, 162, 140, 255 }, 1.0f, true);
    QuadTesorero({ -P - 0.45f, 0.009f, P + 0.45f }, { P + 0.45f, 0.009f, P + 0.45f }, { P + 0.45f, 0.009f, P }, { -P - 0.45f, 0.009f, P }, marco);
    QuadTesorero({ -P - 0.45f, 0.009f, -P }, { P + 0.45f, 0.009f, -P }, { P + 0.45f, 0.009f, -P - 0.45f }, { -P - 0.45f, 0.009f, -P - 0.45f }, marco);
    QuadTesorero({ -P - 0.45f, 0.009f, P }, { -P, 0.009f, P }, { -P, 0.009f, -P }, { -P - 0.45f, 0.009f, -P }, TonoTesorero(marco, 0.96f));
    QuadTesorero({ P, 0.009f, P }, { P + 0.45f, 0.009f, P }, { P + 0.45f, 0.009f, -P }, { P, 0.009f, -P }, TonoTesorero(marco, 0.96f));

    // Umbrales de las rejas: piedra oscura con la ranura de hierro donde
    // clavan los barrotes (exactamente la huella de cada reja).
    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        const RejaTesorero& r = m.rejas[k];
        bool alLargoDeZ = r.mitadZ > r.mitadX;
        float mx = r.mitadX + 0.22f;
        float mz = r.mitadZ + 0.22f;
        Color umbral = CaraTesorero(Color{ 112, 104, 98, 255 }, 1.0f, true);
        QuadTesorero({ r.x - mx, 0.011f, r.z + mz }, { r.x + mx, 0.011f, r.z + mz }, { r.x + mx, 0.011f, r.z - mz }, { r.x - mx, 0.011f, r.z - mz }, umbral);

        Color ranura = Color{ 40, 38, 44, 255 };
        float lx = alLargoDeZ ? 0.06f : r.mitadX;
        float lz = alLargoDeZ ? r.mitadZ : 0.06f;
        QuadTesorero({ r.x - lx, 0.013f, r.z + lz }, { r.x + lx, 0.013f, r.z + lz }, { r.x + lx, 0.013f, r.z - lz }, { r.x - lx, 0.013f, r.z - lz }, ranura);
    }

    rlEnd();
}


//==================================================
// VISUAL: MURALLAS, TORRES Y TORRE DEL HOMENAJE
//==================================================

static const float X_MURO_INTERIOR_TESORERO = 9.0f;
static const float X_MURO_EXTERIOR_TESORERO = 11.0f;
static const float ALTO_MURO_LATERAL_TESORERO = 2.3f;
static const float ALTO_MURO_FONDO_TESORERO = 3.3f;
static const float Z_MURO_FONDO_TESORERO = -7.0f;


// Llama de antorcha o brasero: dos lenguas que titilan.
static void LlamaTesorero(Vector3 base, float escala, float t, int semilla)
{
    float a = 1.0f + 0.16f * std::sin(t * 13.0f + (float)semilla * 1.7f) + 0.08f * std::sin(t * 29.0f + (float)semilla);
    float b = 1.0f + 0.2f * std::sin(t * 17.0f + (float)semilla * 2.3f);
    PrismaTesorero(base, 0.13f * escala, 0.0f, 0.42f * escala * a, 6, COLOR_FUEGO_TESORERO, t * 2.0f);
    PrismaTesorero({ base.x, base.y + 0.02f, base.z }, 0.08f * escala, 0.0f, 0.3f * escala * b, 5, Color{ 255, 226, 130, 255 }, -t * 3.0f);
}


// Antorcha en un soporte de hierro, clavada en una pared con normal n.
static Vector3 PosicionAntorchaTesorero(int k)
{
    if (k < 6)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        float z = -4.6f + (float)(k / 2) * 4.6f;
        return { lado * (X_MURO_INTERIOR_TESORERO - 0.3f), 1.95f, z };
    }

    float lado = k == 6 ? -1.0f : 1.0f;
    return { lado * 6.2f, 2.3f, Z_MURO_FONDO_TESORERO + 0.3f };
}


static void AntorchaTesorero(int k, float t)
{
    Vector3 p = PosicionAntorchaTesorero(k);
    Vector3 n = k < 6
        ? Vector3{ p.x < 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f }
        : Vector3{ 0.0f, 0.0f, 1.0f };
    Vector3 pared = SumaTesorero(p, EscalaTesorero(n, -0.3f));

    // Placa y brazo de hierro.
    CuboTesorero({ pared.x + n.x * 0.03f, p.y - 0.42f, pared.z + n.z * 0.03f }, { std::fabs(n.x) > 0.5f ? 0.06f : 0.24f, 0.36f, std::fabs(n.z) > 0.5f ? 0.06f : 0.24f }, COLOR_HIERRO_TESORERO);
    CuboTesorero({ pared.x + n.x * 0.16f, p.y - 0.36f, pared.z + n.z * 0.16f }, { std::fabs(n.x) > 0.5f ? 0.28f : 0.05f, 0.05f, std::fabs(n.z) > 0.5f ? 0.28f : 0.05f }, COLOR_HIERRO_TESORERO);

    // Mango y canasto.
    PrismaTesorero({ p.x, p.y - 0.55f, p.z }, 0.04f, 0.06f, 0.5f, 5, COLOR_MADERA_OSCURA_TESORERO);
    PrismaTesorero({ p.x, p.y - 0.08f, p.z }, 0.07f, 0.12f, 0.12f, 6, COLOR_HIERRO_TESORERO);
    LlamaTesorero({ p.x, p.y + 0.02f, p.z }, 1.0f, t, k);
}


static void TorreRedondaTesorero(Vector3 base, float radio, float alto, bool tejado, float t, int semilla)
{
    // Hiladas alternadas para que se lean los sillares.
    int hiladas = (int)(alto / 0.55f);
    float h = alto / (float)hiladas;

    for (int c = 0; c < hiladas; c++)
    {
        float tono = (c % 2 == 0 ? 0.96f : 1.04f) * (0.96f + 0.06f * Ruido01Tesorero(c, semilla));
        PrismaTesorero({ base.x, base.y + h * (float)c, base.z }, radio, radio, h, 14, TonoTesorero(COLOR_PIEDRA_TESORERO, tono), c % 2 == 0 ? 0.0f : 0.224f);
    }

    // Saeteras hacia el patio.
    rlBegin(RL_QUADS);
    for (int k = 0; k < 2; k++)
    {
        float a = (base.x < 0.0f ? 0.25f : 0.75f) * 3.1416f + (k == 0 ? 0.0f : (base.x < 0.0f ? -0.6f : 0.6f));
        float ca = std::cos(a), sa = std::sin(a);
        Vector3 c = { base.x + ca * (radio + 0.01f), 0.0f, base.z + sa * (radio + 0.01f) };
        Vector3 u = { -sa * 0.07f, 0.0f, ca * 0.07f };
        for (int f = 0; f < (alto > 4.0f ? 2 : 1); f++)
        {
            float y0 = base.y + alto * (0.35f + 0.3f * (float)f);
            Color hueco = TinteTesorero(Color{ 36, 30, 36, 255 }, c);
            QuadTesorero({ c.x - u.x, y0, c.z - u.z }, { c.x + u.x, y0, c.z + u.z }, { c.x + u.x, y0 + 0.6f, c.z + u.z }, { c.x - u.x, y0 + 0.6f, c.z - u.z }, hueco);
        }
    }
    rlEnd();

    // Matacan (corona saliente).
    PrismaTesorero({ base.x, base.y + alto, base.z }, radio, radio + 0.3f, 0.32f, 14, COLOR_PIEDRA_OSCURA_TESORERO);
    PrismaTesorero({ base.x, base.y + alto + 0.32f, base.z }, radio + 0.3f, radio + 0.3f, 0.22f, 14, COLOR_PIEDRA_TESORERO);

    float yTope = base.y + alto + 0.54f;

    if (tejado)
    {
        PrismaTesorero({ base.x, yTope, base.z }, radio + 0.5f, radio + 0.42f, 0.18f, 14, TonoTesorero(COLOR_PIZARRA_TESORERO, 0.8f));
        PrismaTesorero({ base.x, yTope + 0.18f, base.z }, radio + 0.42f, 0.0f, radio * 2.1f, 14, COLOR_PIZARRA_TESORERO);
        float punta = yTope + 0.18f + radio * 2.1f;
        PrismaTesorero({ base.x, punta - 0.1f, base.z }, 0.04f, 0.03f, 1.1f, 4, COLOR_HIERRO_TESORERO);
        GallardeteTesorero({ base.x, punta + 0.9f, base.z }, 1.3f, t, (float)semilla, COLOR_TELA_TESORERO);
    }
    else
    {
        // Almenas en anillo.
        PrismaTesorero({ base.x, yTope - 0.01f, base.z }, radio - 0.1f, radio - 0.1f, 0.02f, 14, TonoTesorero(COLOR_PIEDRA_TESORERO, 0.9f));
        for (int k = 0; k < 10; k++)
        {
            float a = 6.2832f * (float)k / 10.0f;
            Vector3 c = { base.x + std::cos(a) * (radio + 0.1f), yTope + 0.25f, base.z + std::sin(a) * (radio + 0.1f) };
            CuboGiradoTesorero(c, { 0.3f, 0.5f, 0.55f }, -a * RAD2DEG, TonoTesorero(COLOR_PIEDRA_TESORERO, 0.95f + 0.08f * Ruido01Tesorero(k, semilla)));
        }
    }
}


static void PuertaFondoTesorero()
{
    const float Z = Z_MURO_FONDO_TESORERO;
    const float R = 1.35f;
    const float Y_ARCO = 2.0f;

    // Hoja de madera con tablones, flejes y clavos.
    rlBegin(RL_QUADS);
    Color oscuro = TinteTesorero(Color{ 34, 26, 26, 255 }, { 0.0f, 1.0f, Z });
    QuadTesorero({ -R - 0.05f, 0.0f, Z + 0.01f }, { R + 0.05f, 0.0f, Z + 0.01f }, { R + 0.05f, Y_ARCO, Z + 0.01f }, { -R - 0.05f, Y_ARCO, Z + 0.01f }, oscuro);
    for (int k = 0; k < 7; k++)
    {
        float x0 = -R + 2.0f * R * (float)k / 7.0f + 0.02f;
        float x1 = -R + 2.0f * R * (float)(k + 1) / 7.0f - 0.02f;
        float xm = (x0 + x1) * 0.5f;
        float techo = Y_ARCO + std::sqrt(R * R - xm * xm) - 0.1f;
        Color tabla = CaraTesorero(TonoTesorero(COLOR_MADERA_TESORERO, 0.86f + 0.14f * Ruido01Tesorero(k, 120)), 0.8f, false);
        QuadTesorero({ x0, 0.0f, Z + 0.03f }, { x1, 0.0f, Z + 0.03f }, { x1, techo, Z + 0.03f }, { x0, techo, Z + 0.03f }, tabla);
    }
    for (int b = 0; b < 3; b++)
    {
        float y = 0.45f + 0.8f * (float)b;
        Color fleje = CaraTesorero(COLOR_HIERRO_TESORERO, 0.9f, false);
        QuadTesorero({ -R + 0.05f, y, Z + 0.05f }, { R - 0.05f, y, Z + 0.05f }, { R - 0.05f, y + 0.12f, Z + 0.05f }, { -R + 0.05f, y + 0.12f, Z + 0.05f }, fleje);
    }
    // Junta entre las dos hojas.
    QuadTesorero({ -0.03f, 0.0f, Z + 0.06f }, { 0.03f, 0.0f, Z + 0.06f }, { 0.03f, Y_ARCO + R - 0.1f, Z + 0.06f }, { -0.03f, Y_ARCO + R - 0.1f, Z + 0.06f }, oscuro);
    rlEnd();

    // Jambas y dovelas del arco.
    for (int s = -1; s <= 1; s += 2)
    {
        CuboTesorero({ (float)s * (R + 0.22f), Y_ARCO * 0.5f, Z + 0.12f }, { 0.44f, Y_ARCO, 0.3f }, COLOR_PIEDRA_TESORERO);
    }

    Color dovela = TinteTesorero(COLOR_PIEDRA_TESORERO, { 0.0f, 3.0f, Z });
    for (int k = 0; k < 9; k++)
    {
        float a = 3.1416f * ((float)k + 0.5f) / 9.0f;
        Vector3 c = { std::cos(a) * (R + 0.22f), Y_ARCO + std::sin(a) * (R + 0.22f), Z + 0.12f };
        rlPushMatrix();
        rlTranslatef(c.x, c.y, c.z);
        rlRotatef(a * RAD2DEG - 90.0f, 0.0f, 0.0f, 1.0f);
        CuboCrudoTesorero({ 0.0f, 0.0f, 0.0f }, { 0.4f, 0.46f, 0.3f }, k == 4 ? TonoTesorero(dovela, 1.1f) : TonoTesorero(dovela, 0.94f + 0.08f * Ruido01Tesorero(k, 121)));
        rlPopMatrix();
    }
}


static void DibujarMurallasTesorero(float t, bool detalle)
{
    const float XI = X_MURO_INTERIOR_TESORERO;
    const float XE = X_MURO_EXTERIOR_TESORERO;
    const float ALTO = ALTO_MURO_LATERAL_TESORERO;
    const float AF = ALTO_MURO_FONDO_TESORERO;
    const float ZF = Z_MURO_FONDO_TESORERO;

    // Torre del homenaje detras de la muralla del fondo.
    CuboTesorero({ 0.0f, 4.5f, -11.6f }, { 10.0f, 9.0f, 4.8f }, COLOR_PIEDRA_TESORERO);
    SillaresTesorero({ -5.0f, AF - 0.1f, -9.2f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 10.0f, 9.0f - AF + 0.1f, 0.56f, 1.25f, COLOR_PIEDRA_TESORERO, 0.82f, 140);
    AlmenasTesorero({ -5.0f, 9.0f, -9.45f }, { 5.0f, 9.0f, -9.45f }, 0.5f, 0.6f, 0.7f, 0.55f, COLOR_PIEDRA_TESORERO);

    // Ventanales encendidos (el halo va con las luces).
    rlBegin(RL_QUADS);
    for (int k = 0; k < 4; k++)
    {
        float x = k < 2 ? -3.2f : 3.2f;
        float y = k % 2 == 0 ? 4.4f : 6.6f;
        Color marco = TinteTesorero(Color{ 60, 46, 44, 255 }, { x, y, -9.2f });
        Color vidrio = Color{ 255, 196, 112, 255 };
        QuadTesorero({ x - 0.42f, y - 0.06f, -9.18f }, { x + 0.42f, y - 0.06f, -9.18f }, { x + 0.42f, y + 1.12f, -9.18f }, { x - 0.42f, y + 1.12f, -9.18f }, marco);
        QuadTesorero({ x - 0.32f, y, -9.17f }, { x + 0.32f, y, -9.17f }, { x + 0.32f, y + 0.92f, -9.17f }, { x - 0.32f, y + 0.92f, -9.17f }, vidrio);
        QuadTesorero({ x - 0.03f, y, -9.16f }, { x + 0.03f, y, -9.16f }, { x + 0.03f, y + 0.92f, -9.16f }, { x - 0.03f, y + 0.92f, -9.16f }, marco);
    }
    rlEnd();

    EstandarteTesorero({ 0.0f, 8.6f, -9.2f }, { 0.0f, 0.0f, 1.0f }, 2.0f, 4.4f, t, 0.0f, COLOR_TELA_TESORERO, COLOR_CREMA_TESORERO);

    for (int s = -1; s <= 1; s += 2)
    {
        TorreRedondaTesorero({ (float)s * 5.0f, 0.0f, -9.7f }, 1.0f, 9.6f, true, t, 150 + s);
    }

    // Muralla del fondo con la puerta del tesoro.
    CuboTesorero({ 0.0f, AF * 0.5f, ZF - 1.1f }, { 18.4f, AF, 2.2f }, COLOR_PIEDRA_TESORERO);
    SillaresTesorero({ -9.2f, 0.0f, ZF }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 18.4f, AF, 0.44f, 0.95f, COLOR_PIEDRA_TESORERO, 0.8f, 160);
    CuboTesorero({ 0.0f, AF + 0.06f, ZF - 1.1f }, { 18.4f, 0.12f, 2.2f }, TonoTesorero(COLOR_PIEDRA_TESORERO, 1.06f));
    AlmenasTesorero({ -9.0f, AF + 0.12f, ZF - 0.2f }, { 9.0f, AF + 0.12f, ZF - 0.2f }, 0.4f, 0.55f, 0.7f, 0.5f, COLOR_PIEDRA_TESORERO);
    PuertaFondoTesorero();

    EstandarteTesorero({ -3.4f, AF - 0.25f, ZF }, { 0.0f, 0.0f, 1.0f }, 1.1f, 2.2f, t, 1.3f, COLOR_TELA_TESORERO, COLOR_CREMA_TESORERO);
    EstandarteTesorero({ 3.4f, AF - 0.25f, ZF }, { 0.0f, 0.0f, 1.0f }, 1.1f, 2.2f, t, 2.6f, COLOR_TELA_TESORERO, COLOR_CREMA_TESORERO);

    // Murallas laterales con adarve y almenas hacia afuera.
    for (int s = -1; s <= 1; s += 2)
    {
        float lado = (float)s;
        CuboTesorero({ lado * (XI + XE) * 0.5f, (ALTO - 0.95f) * 0.5f, -0.3f }, { XE - XI, ALTO + 0.95f, 15.4f }, COLOR_PIEDRA_TESORERO);
        SillaresTesorero({ lado * XI, 0.0f, -7.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { -lado, 0.0f, 0.0f }, 14.0f, ALTO, 0.42f, 0.95f, COLOR_PIEDRA_TESORERO, s < 0 ? 0.76f : 0.88f, 170 + s);

        // Losas del adarve.
        rlBegin(RL_QUADS);
        for (int k = 0; k < 19; k++)
        {
            float z0 = -7.6f + 0.8f * (float)k + 0.03f;
            float z1 = z0 + 0.74f;
            Color losa = CaraTesorero(TonoTesorero(COLOR_PIEDRA_TESORERO, 0.9f + 0.12f * Ruido01Tesorero(k, 180 + s)), 1.0f, true);
            QuadTesorero({ lado * (XI + 0.2f), ALTO + 0.004f, z1 }, { lado * (XE - 0.4f), ALTO + 0.004f, z1 }, { lado * (XE - 0.4f), ALTO + 0.004f, z0 }, { lado * (XI + 0.2f), ALTO + 0.004f, z0 }, losa);
        }
        rlEnd();

        CuboTesorero({ lado * (XI + 0.1f), ALTO + 0.12f, -0.3f }, { 0.2f, 0.24f, 15.0f }, TonoTesorero(COLOR_PIEDRA_TESORERO, 1.04f));
        CuboTesorero({ lado * (XE - 0.2f), ALTO + 0.25f, -0.3f }, { 0.4f, 0.5f, 15.4f }, COLOR_PIEDRA_TESORERO);
        AlmenasTesorero({ lado * (XE - 0.2f), ALTO + 0.5f, -7.6f }, { lado * (XE - 0.2f), ALTO + 0.5f, 7.0f }, 0.4f, 0.55f, 0.62f, 0.5f, COLOR_PIEDRA_TESORERO);
    }

    // Parapeto bajo del frente: deja ver la ultima fila del patio.
    CuboTesorero({ 0.0f, 0.12f, 7.35f }, { 18.4f, 1.0f, 0.7f }, COLOR_PIEDRA_TESORERO);
    AlmenasTesorero({ -9.0f, 0.62f, 7.45f }, { 9.0f, 0.62f, 7.45f }, 0.5f, 0.26f, 0.9f, 0.6f, COLOR_PIEDRA_TESORERO);

    // Torres de esquina: altas con tejado al fondo, bajas con brasero al frente.
    for (int s = -1; s <= 1; s += 2)
    {
        TorreRedondaTesorero({ (float)s * 10.6f, -0.95f, -8.6f }, 1.75f, 7.0f, true, t, 190 + s);
        TorreRedondaTesorero({ (float)s * 10.6f, -0.95f, 7.6f }, 1.45f, 3.6f, false, t, 195 + s);

        // Brasero sobre la torre baja.
        Vector3 b = { (float)s * 10.6f, 3.19f, 7.6f };
        PrismaTesorero(b, 0.18f, 0.12f, 0.45f, 6, COLOR_HIERRO_TESORERO);
        PrismaTesorero({ b.x, b.y + 0.45f, b.z }, 0.25f, 0.42f, 0.22f, 8, COLOR_HIERRO_TESORERO);
        PrismaTesorero({ b.x, b.y + 0.62f, b.z }, 0.36f, 0.3f, 0.04f, 8, Color{ 120, 52, 30, 255 });
        LlamaTesorero({ b.x, b.y + 0.64f, b.z }, 2.0f, t, 20 + s);
    }

    for (int k = 0; k < 8; k++)
    {
        AntorchaTesorero(k, t);
    }

    // Estandartes en los muros laterales, entre antorchas.
    if (detalle)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            for (int k = 0; k < 2; k++)
            {
                float z = -2.3f + 4.6f * (float)k;
                EstandarteTesorero({ (float)s * XI, ALTO - 0.15f, z }, { -(float)s, 0.0f, 0.0f }, 0.9f, 1.6f, t, (float)(k * 2 + s), COLOR_TELA_TESORERO, COLOR_CREMA_TESORERO);
            }
        }
    }
}


//==================================================
// VISUAL: UTILERIA
//==================================================

static void BarrilTesorero(Vector3 base, float escala, int semilla)
{
    Color madera = TonoTesorero(COLOR_MADERA_TESORERO, 0.9f + 0.18f * Ruido01Tesorero(semilla, 200));
    float r = 0.3f * escala;
    float h = 0.8f * escala;
    PrismaTesorero(base, r * 0.85f, r, h * 0.5f, 10, madera);
    PrismaTesorero({ base.x, base.y + h * 0.5f, base.z }, r, r * 0.85f, h * 0.5f, 10, madera);
    PrismaTesorero({ base.x, base.y + h * 0.16f, base.z }, r * 0.95f, r * 0.97f, 0.06f * escala, 10, COLOR_HIERRO_TESORERO);
    PrismaTesorero({ base.x, base.y + h * 0.78f, base.z }, r * 0.97f, r * 0.95f, 0.06f * escala, 10, COLOR_HIERRO_TESORERO);
    PrismaTesorero({ base.x, base.y + h - 0.01f, base.z }, r * 0.82f, r * 0.82f, 0.02f, 10, TonoTesorero(madera, 1.15f));
}


static void CajaTesorero(Vector3 base, float lado, float giro, int semilla)
{
    Color madera = TonoTesorero(Color{ 160, 118, 74, 255 }, 0.88f + 0.18f * Ruido01Tesorero(semilla, 201));
    Vector3 c = { base.x, base.y + lado * 0.5f, base.z };
    CuboGiradoTesorero(c, { lado, lado, lado }, giro, madera);

    // Marco de listones en las aristas de arriba.
    Color liston = TonoTesorero(madera, 0.72f);
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    Color t = TinteTesorero(liston, c);
    float m = lado * 0.5f;
    CuboCrudoTesorero({ 0.0f, m - 0.04f, m - 0.04f }, { lado + 0.02f, 0.09f, 0.09f }, t);
    CuboCrudoTesorero({ 0.0f, -m + 0.04f, m - 0.04f }, { lado + 0.02f, 0.09f, 0.09f }, t);
    CuboCrudoTesorero({ m - 0.04f, 0.0f, m - 0.04f }, { 0.09f, lado, 0.09f }, t);
    CuboCrudoTesorero({ -m + 0.04f, 0.0f, m - 0.04f }, { 0.09f, lado, 0.09f }, t);
    rlPopMatrix();
}


static void SacoTesorero(Vector3 base, float escala, Color tela)
{
    EsferaTesorero({ base.x, base.y + 0.22f * escala, base.z }, { 0.26f * escala, 0.24f * escala, 0.22f * escala }, tela, 4, 8);
    PrismaTesorero({ base.x, base.y + 0.4f * escala, base.z }, 0.08f * escala, 0.12f * escala, 0.14f * escala, 6, TonoTesorero(tela, 0.9f));
}


static void BalasCanonTesorero(Vector3 base)
{
    const float R = 0.13f;
    for (int f = 0; f < 3; f++)
    {
        int n = 3 - f;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                float x = base.x + ((float)i - (float)(n - 1) * 0.5f) * R * 2.0f;
                float z = base.z + ((float)j - (float)(n - 1) * 0.5f) * R * 2.0f;
                EsferaTesorero({ x, base.y + R + (float)f * R * 1.5f, z }, { R, R, R }, Color{ 58, 58, 64, 255 }, 3, 6);
            }
        }
    }
}


// Ballesta de asedio mirando hacia afuera (direccion = signo en X).
static void BallestaTesorero(Vector3 base, float direccion)
{
    CuboTesorero({ base.x, base.y + 0.15f, base.z }, { 0.7f, 0.3f, 0.7f }, COLOR_MADERA_OSCURA_TESORERO);
    CuboTesorero({ base.x + direccion * 0.15f, base.y + 0.42f, base.z }, { 1.2f, 0.12f, 0.18f }, COLOR_MADERA_TESORERO);
    CuboTesorero({ base.x + direccion * 0.55f, base.y + 0.46f, base.z }, { 0.12f, 0.12f, 1.2f }, COLOR_MADERA_TESORERO);
    CuboTesorero({ base.x + direccion * 0.1f, base.y + 0.52f, base.z }, { 1.3f, 0.04f, 0.04f }, Color{ 200, 190, 170, 255 });
    CuboTesorero({ base.x + direccion * 0.82f, base.y + 0.52f, base.z }, { 0.16f, 0.07f, 0.07f }, COLOR_HIERRO_TESORERO);
}


static void LanzasTesorero(Vector3 base, bool alLargoDeZ)
{
    float ux = alLargoDeZ ? 0.0f : 1.0f;
    float uz = alLargoDeZ ? 1.0f : 0.0f;
    CuboTesorero({ base.x, base.y + 0.5f, base.z }, { ux * 1.2f + uz * 0.12f, 0.08f, uz * 1.2f + ux * 0.12f }, COLOR_MADERA_OSCURA_TESORERO);
    for (int s = -1; s <= 1; s += 2)
    {
        CuboTesorero({ base.x + ux * (float)s * 0.55f, base.y + 0.3f, base.z + uz * (float)s * 0.55f }, { 0.08f, 0.6f, 0.08f }, COLOR_MADERA_OSCURA_TESORERO);
    }
    for (int k = 0; k < 4; k++)
    {
        float o = -0.42f + 0.28f * (float)k;
        Vector3 p = { base.x + ux * o, base.y, base.z + uz * o };
        CuboTesorero({ p.x, p.y + 0.8f, p.z }, { 0.045f, 1.6f, 0.045f }, COLOR_MADERA_TESORERO);
        PrismaTesorero({ p.x, p.y + 1.6f, p.z }, 0.06f, 0.0f, 0.22f, 4, Color{ 196, 196, 204, 255 });
    }
}


static void FardoTesorero(Vector3 base, float giro, int semilla)
{
    Color paja = TonoTesorero(COLOR_PAJA_TESORERO, 0.9f + 0.15f * Ruido01Tesorero(semilla, 202));
    Vector3 c = { base.x, base.y + 0.25f, base.z };
    CuboGiradoTesorero(c, { 0.9f, 0.5f, 0.55f }, giro, paja);
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    Color cuerda = TinteTesorero(Color{ 140, 104, 60, 255 }, c);
    CuboCrudoTesorero({ -0.22f, 0.0f, 0.0f }, { 0.05f, 0.52f, 0.57f }, cuerda);
    CuboCrudoTesorero({ 0.22f, 0.0f, 0.0f }, { 0.05f, 0.52f, 0.57f }, cuerda);
    rlPopMatrix();
}


static void RuedaTesorero(Vector3 c, float radio, float girox)
{
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    rlRotatef(girox, 0.0f, 1.0f, 0.0f);
    Color m = TinteTesorero(COLOR_MADERA_OSCURA_TESORERO, c);
    rlBegin(RL_QUADS);
    for (int k = 0; k < 12; k++)
    {
        float a0 = 6.2832f * (float)k / 12.0f;
        float a1 = 6.2832f * (float)(k + 1) / 12.0f;
        Color cara = CaraTesorero(m, LuzLateralTesorero(a0), false);
        QuadTesorero({ std::cos(a0) * radio, -0.06f, std::sin(a0) * radio }, { std::cos(a1) * radio, -0.06f, std::sin(a1) * radio },
            { std::cos(a1) * radio, 0.06f, std::sin(a1) * radio }, { std::cos(a0) * radio, 0.06f, std::sin(a0) * radio }, cara);
        QuadTesorero({ std::cos(a0) * radio, 0.06f, std::sin(a0) * radio }, { std::cos(a1) * radio, 0.06f, std::sin(a1) * radio },
            { std::cos(a1) * radio * 0.8f, 0.06f, std::sin(a1) * radio * 0.8f }, { std::cos(a0) * radio * 0.8f, 0.06f, std::sin(a0) * radio * 0.8f }, CaraTesorero(m, 1.0f, true));
    }
    for (int k = 0; k < 4; k++)
    {
        float a = 0.785f * (float)k;
        float ca = std::cos(a) * radio * 0.8f, sa = std::sin(a) * radio * 0.8f;
        float px = -std::sin(a) * 0.04f, pz = std::cos(a) * 0.04f;
        QuadTesorero({ -ca - px, 0.065f, -sa - pz }, { ca - px, 0.065f, sa - pz }, { ca + px, 0.065f, sa + pz }, { -ca + px, 0.065f, -sa + pz }, CaraTesorero(m, 0.9f, true));
    }
    rlEnd();
    rlPopMatrix();
}


// Carreta con heno o con barriles, girada en Y.
static void CarretaTesorero(Vector3 base, float giro, bool conHeno, int semilla)
{
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    Color m = TinteTesorero(COLOR_MADERA_TESORERO, base);
    CuboCrudoTesorero({ 0.0f, 0.6f, 0.0f }, { 2.0f, 0.14f, 1.1f }, m);
    CuboCrudoTesorero({ 0.0f, 0.82f, 0.53f }, { 2.0f, 0.32f, 0.06f }, TonoTesorero(m, 0.9f));
    CuboCrudoTesorero({ 0.0f, 0.82f, -0.53f }, { 2.0f, 0.32f, 0.06f }, TonoTesorero(m, 0.9f));
    CuboCrudoTesorero({ -0.97f, 0.82f, 0.0f }, { 0.06f, 0.32f, 1.1f }, TonoTesorero(m, 0.86f));
    CuboCrudoTesorero({ 1.5f, 0.55f, 0.25f }, { 1.4f, 0.07f, 0.07f }, TonoTesorero(m, 0.8f));
    CuboCrudoTesorero({ 1.5f, 0.55f, -0.25f }, { 1.4f, 0.07f, 0.07f }, TonoTesorero(m, 0.8f));

    if (conHeno)
    {
        Color paja = TinteTesorero(COLOR_PAJA_TESORERO, base);
        CuboCrudoTesorero({ 0.0f, 1.0f, 0.0f }, { 1.86f, 0.5f, 1.0f }, paja);
        CuboCrudoTesorero({ 0.1f, 1.35f, 0.0f }, { 1.4f, 0.3f, 0.8f }, TonoTesorero(paja, 1.06f));
    }

    rlPopMatrix();

    float c = std::cos(giro * DEG2RAD), s = std::sin(giro * DEG2RAD);
    for (int k = -1; k <= 1; k += 2)
    {
        // Ruedas en los costados (eje Z local).
        Vector3 local = { -0.2f, 0.45f, (float)k * 0.62f };
        Vector3 mundo = { base.x + local.x * c + local.z * s, base.y + local.y, base.z - local.x * s + local.z * c };
        RuedaTesorero(mundo, 0.45f, giro);
    }

    if (!conHeno)
    {
        Vector3 p = { base.x - 0.4f * c, base.y + 0.67f, base.z + 0.4f * s };
        BarrilTesorero(p, 0.8f, semilla);
        BarrilTesorero({ base.x + 0.3f * c, base.y + 0.67f, base.z - 0.3f * s }, 0.8f, semilla + 1);
    }
}


// Tienda de lona a rayas violeta y crema.
static void TiendaTesorero(Vector3 base, float radio, float t, int semilla)
{
    float alto = radio * 1.5f;
    const int LADOS = 10;
    Color telas[2] = { COLOR_TELA_TESORERO, COLOR_CREMA_TESORERO };
    float ondea = std::sin(t * 1.8f + (float)semilla) * 0.03f;

    rlBegin(RL_QUADS);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Color c = TinteTesorero(CaraTesorero(telas[k % 2], LuzLateralTesorero((a0 + a1) * 0.5f), false), base);
        float r0 = radio * (1.0f + ondea);
        QuadTesorero({ base.x + std::cos(a0) * r0, base.y, base.z + std::sin(a0) * r0 }, { base.x + std::cos(a1) * r0, base.y, base.z + std::sin(a1) * r0 },
            { base.x + std::cos(a1) * radio * 0.92f, base.y + alto * 0.45f, base.z + std::sin(a1) * radio * 0.92f }, { base.x + std::cos(a0) * radio * 0.92f, base.y + alto * 0.45f, base.z + std::sin(a0) * radio * 0.92f }, c);
        Color techo = TinteTesorero(CaraTesorero(telas[k % 2], 0.85f + 0.2f * std::sin(a0 + 1.0f), true), base);
        QuadTesorero({ base.x + std::cos(a0) * radio * 0.92f, base.y + alto * 0.45f, base.z + std::sin(a0) * radio * 0.92f }, { base.x + std::cos(a1) * radio * 0.92f, base.y + alto * 0.45f, base.z + std::sin(a1) * radio * 0.92f },
            { base.x, base.y + alto, base.z }, { base.x, base.y + alto, base.z }, techo);
    }
    rlEnd();

    PrismaTesorero({ base.x, base.y + alto, base.z }, 0.03f, 0.03f, 0.5f, 4, COLOR_MADERA_OSCURA_TESORERO);
    GallardeteTesorero({ base.x, base.y + alto + 0.45f, base.z }, 0.7f, t, (float)semilla, COLOR_CREMA_TESORERO);
}


static void DianaTesorero(Vector3 base)
{
    CuboTesorero({ base.x - 0.3f, base.y + 0.6f, base.z - 0.1f }, { 0.08f, 1.2f, 0.08f }, COLOR_MADERA_OSCURA_TESORERO);
    CuboTesorero({ base.x + 0.3f, base.y + 0.6f, base.z - 0.1f }, { 0.08f, 1.2f, 0.08f }, COLOR_MADERA_OSCURA_TESORERO);
    Vector3 c = { base.x, base.y + 1.0f, base.z };
    const Color anillos[4] = { COLOR_PAJA_TESORERO, COLOR_CREMA_TESORERO, COLOR_TELA_TESORERO, COLOR_CREMA_TESORERO };
    for (int k = 0; k < 4; k++)
    {
        float r = 0.55f - 0.13f * (float)k;
        rlPushMatrix();
        rlTranslatef(c.x, c.y, c.z + 0.01f * (float)k);
        rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        AnilloTesorero({ 0.0f, 0.0f, 0.0f }, 0.0f, r, TinteTesorero(CaraTesorero(anillos[k], 0.9f, false), c), 16);
        rlPopMatrix();
    }
}


static void LenaTesorero(Vector3 base, int semilla)
{
    for (int f = 0; f < 3; f++)
    {
        for (int k = 0; k < 4 - f; k++)
        {
            Vector3 c = { base.x + ((float)k - (float)(3 - f) * 0.5f) * 0.26f, base.y + 0.12f + (float)f * 0.22f, base.z };
            Color m = TonoTesorero(COLOR_MADERA_TESORERO, 0.85f + 0.2f * Ruido01Tesorero(f * 5 + k, semilla));
            CuboTesorero(c, { 0.22f, 0.22f, 1.1f }, m);
            rlBegin(RL_QUADS);
            Color corte = TinteTesorero(Color{ 214, 176, 120, 255 }, c);
            QuadTesorero({ c.x - 0.09f, c.y - 0.09f, c.z + 0.556f }, { c.x + 0.09f, c.y - 0.09f, c.z + 0.556f }, { c.x + 0.09f, c.y + 0.09f, c.z + 0.556f }, { c.x - 0.09f, c.y + 0.09f, c.z + 0.556f }, corte);
            rlEnd();
        }
    }
}


static void CobertizoTesorero(Vector3 base, float ancho, float fondo, float alto, int semilla)
{
    CuboTesorero({ base.x, base.y + alto * 0.5f, base.z }, { ancho, alto, fondo }, Color{ 196, 178, 150, 255 });
    // Entramado de madera.
    for (int k = -1; k <= 1; k++)
    {
        CuboTesorero({ base.x + (float)k * ancho * 0.45f, base.y + alto * 0.5f, base.z + fondo * 0.5f + 0.02f }, { 0.14f, alto, 0.05f }, COLOR_MADERA_OSCURA_TESORERO);
    }
    CuboTesorero({ base.x, base.y + alto * 0.55f, base.z + fondo * 0.5f + 0.02f }, { ancho, 0.12f, 0.05f }, COLOR_MADERA_OSCURA_TESORERO);

    float cumbre = base.y + alto + ancho * 0.32f;
    float a = ancho * 0.5f + 0.25f;
    float f = fondo * 0.5f + 0.25f;
    Color techo = TinteTesorero(TonoTesorero(COLOR_PIZARRA_TESORERO, 0.95f + 0.1f * Ruido01Tesorero(semilla, 203)), base);
    rlBegin(RL_QUADS);
    QuadTesorero({ base.x - a, base.y + alto - 0.1f, base.z + f }, { base.x + a, base.y + alto - 0.1f, base.z + f }, { base.x + a, cumbre, base.z }, { base.x - a, cumbre, base.z }, CaraTesorero(techo, 0.92f, true));
    QuadTesorero({ base.x - a, base.y + alto - 0.1f, base.z - f }, { base.x + a, base.y + alto - 0.1f, base.z - f }, { base.x + a, cumbre, base.z }, { base.x - a, cumbre, base.z }, CaraTesorero(techo, 0.7f, false));
    rlEnd();
}


// Adarve: lo que guardan los centinelas en lo alto de las murallas.
static void DibujarAdarveTesorero(float t, int paso)
{
    (void)t;
    const float Y = ALTO_MURO_LATERAL_TESORERO;

    for (int s = -1; s <= 1; s += 2)
    {
        float lado = (float)s;
        float x = lado * 9.85f;
        int base = s < 0 ? 0 : 3;

        for (int k = 0; k < 6; k += paso)
        {
            int tipo = (k + base) % 6;
            float z = -6.0f + 2.35f * (float)k;

            switch (tipo)
            {
                case 0:
                    BarrilTesorero({ x, Y, z - 0.3f }, 1.0f, k + s * 10);
                    BarrilTesorero({ x + lado * 0.15f, Y, z + 0.4f }, 0.9f, k + s * 10 + 1);
                    break;
                case 1:
                    CajaTesorero({ x, Y, z }, 0.7f, 8.0f, k + s);
                    CajaTesorero({ x, Y + 0.7f, z + 0.05f }, 0.5f, -12.0f, k + s + 3);
                    break;
                case 2:
                    BalasCanonTesorero({ x, Y, z });
                    break;
                case 3:
                    BallestaTesorero({ x, Y, z }, lado);
                    break;
                case 4:
                    SacoTesorero({ x - lado * 0.1f, Y, z - 0.3f }, 1.0f, Color{ 198, 176, 132, 255 });
                    SacoTesorero({ x + lado * 0.15f, Y, z + 0.2f }, 0.9f, Color{ 186, 164, 122, 255 });
                    SacoTesorero({ x, Y + 0.32f, z }, 0.85f, Color{ 204, 184, 140, 255 });
                    break;
                default:
                    LanzasTesorero({ x, Y, z }, true);
                    break;
            }
        }
    }
}


// Explanada de afuera: campamento, carretas, leña y diana de practica.
static void DibujarAfuerasTesorero(float t, int paso)
{
    TiendaTesorero({ -16.4f, 0.0f, -6.0f }, 1.5f, t, 1);
    TiendaTesorero({ -17.2f, 0.0f, 0.4f }, 1.2f, t, 2);
    CarretaTesorero({ -15.6f, 0.0f, 4.6f }, 70.0f, true, 3);
    FardoTesorero({ -17.8f, 0.0f, -2.6f }, 20.0f, 4);
    FardoTesorero({ -17.4f, 0.5f, -2.5f }, 5.0f, 5);

    DianaTesorero({ 16.6f, 0.0f, -5.4f });
    CarretaTesorero({ 16.2f, 0.0f, 0.2f }, -100.0f, false, 6);
    LenaTesorero({ 17.4f, 0.0f, 3.8f }, 7);
    FardoTesorero({ 15.4f, 0.0f, -2.2f }, -30.0f, 8);

    if (paso == 1)
    {
        TiendaTesorero({ 17.6f, 0.0f, -9.6f }, 1.1f, t, 9);
        FardoTesorero({ -15.0f, 0.0f, -10.4f }, 40.0f, 10);
        LenaTesorero({ -18.0f, 0.0f, 8.4f }, 11);
    }

    // Arboles en la explanada.
    const Vector3 arboles[6] =
    {
        { -18.4f, 0.0f, -11.6f }, { 18.6f, 0.0f, -12.4f }, { -18.6f, 0.0f, 10.8f },
        { 18.4f, 0.0f, 9.4f }, { 15.0f, 0.0f, -12.8f }, { -14.6f, 0.0f, 12.0f }
    };
    for (int k = 0; k < 6; k += paso)
    {
        ArbolTesorero(arboles[k], 3.4f + 1.2f * Ruido01Tesorero(k, 210), 40 + k, false);
    }

    // Cobertizos detras de la muralla (asoman sus tejados).
    CobertizoTesorero({ -7.6f, 0.0f, -11.8f }, 3.0f, 2.6f, 2.6f, 1);
    CobertizoTesorero({ 7.8f, 0.0f, -12.0f }, 2.6f, 2.4f, 2.4f, 2);
}


//==================================================
// VISUAL: PEDESTAL DEL TESORO (OBSTACULO CENTRAL)
//==================================================

// Ocupa exactamente la caja de colision (2.6 x 2.2 x 2.6); el cofre y la
// bandera van sobre el techo.
static void DibujarPedestalTesorero(float t)
{
    const float P = MITAD_TORRE_TESORERO;
    const float H = ALTO_TORRE_TESORERO;

    CuboTesorero({ 0.0f, 0.15f, 0.0f }, { P * 2.0f, 0.3f, P * 2.0f }, COLOR_PIEDRA_OSCURA_TESORERO);
    CuboTesorero({ 0.0f, (H - 0.3f) * 0.5f + 0.15f, 0.0f }, { P * 2.0f - 0.16f, H - 0.3f - 0.24f, P * 2.0f - 0.16f }, COLOR_PIEDRA_TESORERO);
    SillaresTesorero({ -P + 0.08f, 0.3f, P - 0.08f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, P * 2.0f - 0.16f, H - 0.54f, 0.34f, 0.62f, COLOR_PIEDRA_TESORERO, 0.8f, 230);
    CuboTesorero({ 0.0f, H - 0.12f, 0.0f }, { P * 2.0f, 0.24f, P * 2.0f }, TonoTesorero(COLOR_PIEDRA_TESORERO, 1.08f));

    // Puertita de la camara del tesoro con cerradura de oro.
    CuboTesorero({ 0.0f, 0.86f, P - 0.06f }, { 0.78f, 1.1f, 0.06f }, COLOR_HIERRO_TESORERO);
    rlBegin(RL_QUADS);
    for (int k = 0; k < 3; k++)
    {
        float y = 0.45f + 0.36f * (float)k;
        Color fleje = TinteTesorero(Color{ 92, 88, 98, 255 }, { 0.0f, y, P });
        QuadTesorero({ -0.39f, y, P - 0.02f }, { 0.39f, y, P - 0.02f }, { 0.39f, y + 0.06f, P - 0.02f }, { -0.39f, y + 0.06f, P - 0.02f }, fleje);
    }
    rlEnd();
    CuboTesorero({ 0.0f, 0.92f, P - 0.01f }, { 0.14f, 0.18f, 0.04f }, COLOR_ORO_TESORERO);

    // Almenas en las esquinas del techo.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            CuboTesorero({ (float)sx * (P - 0.22f), H + 0.2f, (float)sz * (P - 0.22f) }, { 0.44f, 0.4f, 0.44f }, COLOR_PIEDRA_TESORERO);
        }
    }

    // Monton de oro y cofre abierto.
    EsferaTesorero({ 0.0f, H, 0.1f }, { 0.78f, 0.34f, 0.7f }, COLOR_ORO_TESORERO, 4, 10);
    CuboTesorero({ 0.0f, H + 0.3f, -0.25f }, { 0.92f, 0.44f, 0.56f }, COLOR_MADERA_TESORERO);
    CuboTesorero({ 0.0f, H + 0.3f, -0.25f }, { 0.96f, 0.08f, 0.6f }, COLOR_HIERRO_TESORERO);
    CuboTesorero({ 0.0f, H + 0.5f, -0.25f }, { 0.86f, 0.06f, 0.5f }, COLOR_ORO_CLARO_TESORERO);

    // Tapa abierta hacia atras.
    rlPushMatrix();
    rlTranslatef(0.0f, H + 0.52f, -0.53f);
    rlRotatef(-70.0f, 1.0f, 0.0f, 0.0f);
    CuboCrudoTesorero({ 0.0f, 0.0f, 0.28f }, { 0.92f, 0.08f, 0.56f }, CaraTesorero(COLOR_MADERA_TESORERO, 0.9f, false));
    rlPopMatrix();

    // Monedas sueltas sobre el monton.
    for (int k = 0; k < 6; k++)
    {
        float a = (float)k * 1.05f;
        Vector3 p = { std::cos(a) * 0.55f, H + 0.17f + 0.05f * Ruido01Tesorero(k, 231), 0.15f + std::sin(a) * 0.45f };
        PrismaTesorero(p, 0.11f, 0.11f, 0.03f, 8, COLOR_ORO_CLARO_TESORERO);
    }

    // Asta con gallardete en una esquina.
    PrismaTesorero({ P - 0.22f, H + 0.4f, -P + 0.22f }, 0.04f, 0.03f, 1.6f, 4, COLOR_MADERA_OSCURA_TESORERO);
    GallardeteTesorero({ P - 0.22f, H + 1.9f, -P + 0.22f }, 0.9f, t, 5.0f, COLOR_TELA_TESORERO);
}


//==================================================
// VISUAL: REJAS Y MONEDAS (PIEZAS JUGABLES)
//==================================================
//
// Las rejas y las monedas siguen usando los GLB del paquete (marco, hoja
// movil, aviso y moneda) con sus pivotes y transformaciones originales;
// si falta alguno, se dibuja su respaldo de primitivas.

static float AltoMarcoRejaTesorero()
{
    const auto& recurso = ObtenerModelosEscenariosRetro3D().tesoreroCercado[MODELO_TESORERO_MARCO_REJA];
    return recurso.cargado && recurso.dimensiones.y > 0.5f ? recurso.dimensiones.y : 5.4f;
}


static void DibujarRejaTesorero(const RejaTesorero& reja, float t)
{
    float largoX = reja.mitadX * 2.0f;
    float largoZ = reja.mitadZ * 2.0f;
    bool alLargoDeZ = reja.mitadZ > reja.mitadX;
    float baseY = (1.0f - reja.altura) * 2.6f;
    bool aviso = reja.estado == REJA_TESORERO_AVISO;
    bool parpadeo = aviso && std::sin(t * 18.0f) > 0.0f;
    Color hierro = parpadeo ? Color{ 230, 60, 50, 255 } : Color{ 62, 62, 70, 255 };
    float rotacion = alLargoDeZ ? 90.0f : 0.0f;

    // Postes de piedra con dintel.
    if (!DibujarModeloTesoreroCercadoRetro3D(MODELO_TESORERO_MARCO_REJA, { reja.x, 0, reja.z }, rotacion))
    {
        for (int k = -1; k <= 1; k += 2)
        {
            float px = reja.x + (alLargoDeZ ? 0.0f : (float)k * (reja.mitadX + 0.15f));
            float pz = reja.z + (alLargoDeZ ? (float)k * (reja.mitadZ + 0.15f) : 0.0f);
            (DrawCube)({ px, 2.7f, pz }, 0.5f, 5.4f, 0.5f, COLOR_PIEDRA_TESORERO);
        }
        (DrawCube)({ reja.x, 5.3f, reja.z }, alLargoDeZ ? 0.5f : largoX + 0.8f, 0.3f, alLargoDeZ ? largoZ + 0.8f : 0.5f, COLOR_PIEDRA_OSCURA_TESORERO);
    }

    // Hoja movil: barrotes y travesanos.
    const auto& recursoReja = ObtenerModelosEscenariosRetro3D().tesoreroCercado[MODELO_TESORERO_REJA];
    if (recursoReja.cargado)
    {
        for (int i = 0; i < recursoReja.modelo.meshCount; i++)
        {
            // Solo hierro e hierro_oscuro de barras/travesanos parpadean;
            // puntas y herrajes conservan su material. El helper lo restaura.
            bool dinamico = parpadeo && (i == 0 || i == 2);
            DibujarModeloTesoreroCercadoRetro3D(MODELO_TESORERO_REJA,
                { reja.x, baseY, reja.z }, rotacion, { 1, 1, 1 }, i, dinamico, hierro);
        }
    }
    else
    {
        const int BARRAS = 6;

        for (int b = 0; b < BARRAS; b++)
        {
            float f = ((float)b + 0.5f) / (float)BARRAS - 0.5f;
            float bx = reja.x + (alLargoDeZ ? 0.0f : f * largoX);
            float bz = reja.z + (alLargoDeZ ? f * largoZ : 0.0f);
            (DrawCube)({ bx, baseY + ALTO_REJA_TESORERO * 0.5f, bz }, 0.12f, ALTO_REJA_TESORERO, 0.12f, hierro);
        }

        (DrawCube)({ reja.x, baseY + ALTO_REJA_TESORERO * 0.8f, reja.z }, alLargoDeZ ? 0.14f : largoX, 0.12f, alLargoDeZ ? largoZ : 0.14f, hierro);
        (DrawCube)({ reja.x, baseY + ALTO_REJA_TESORERO * 0.25f, reja.z }, alLargoDeZ ? 0.14f : largoX, 0.12f, alLargoDeZ ? largoZ : 0.14f, hierro);
    }

    // Marca en el suelo: la reja esta por bajar.
    if (aviso)
    {
        float pulso = 0.5f + 0.5f * std::sin(t * 18.0f);
        const auto& recursoAviso = ObtenerModelosEscenariosRetro3D().tesoreroCercado[MODELO_TESORERO_AVISO];
        if (recursoAviso.cargado)
        {
            for (int i = 0; i < recursoAviso.modelo.meshCount; i++)
            {
                int material = recursoAviso.modelo.meshMaterial[i];
                Color color = recursoAviso.modelo.materials[material].maps[MATERIAL_MAP_DIFFUSE].color;
                DibujarModeloTesoreroCercadoRetro3D(MODELO_TESORERO_AVISO,
                    { reja.x, 0.03f, reja.z }, rotacion, { 1, 1, 1 }, i, true, Fade(color, 0.25f + 0.3f * pulso));
            }
        }
        else
        {
            (DrawCube)({ reja.x, 0.05f, reja.z }, largoX + 0.6f, 0.05f, largoZ + 0.6f, Fade(COLOR_AVISO_TESORERO, 0.25f + 0.3f * pulso));
        }
    }

    // Farol de senales sobre el dintel: tibio abierta, rojo cerrada.
    float alto = AltoMarcoRejaTesorero();
    Vector3 farol = { reja.x, alto + 0.05f, reja.z };
    PrismaTesorero(farol, 0.16f, 0.14f, 0.08f, 6, COLOR_HIERRO_TESORERO);
    Color vidrio = reja.estado == REJA_TESORERO_ABIERTA
        ? Color{ 255, 226, 168, 255 }
        : (aviso ? (parpadeo ? COLOR_AVISO_TESORERO : Color{ 120, 50, 40, 255 }) : Color{ 214, 64, 48, 255 });
    PrismaTesorero({ farol.x, farol.y + 0.08f, farol.z }, 0.11f, 0.11f, 0.24f, 6, vidrio);
    PrismaTesorero({ farol.x, farol.y + 0.32f, farol.z }, 0.17f, 0.0f, 0.16f, 6, COLOR_HIERRO_TESORERO);
}


static bool MonedaVisibleTesorero(const MonedaTesorero& moneda, float t)
{
    return moneda.activa && !(moneda.edad > VIDA_MONEDA_TESORERO - 2.0f && std::sin(t * 18.0f) < 0.0f);
}


static void DibujarMonedaTesorero(const MonedaTesorero& moneda, float t)
{
    if (!MonedaVisibleTesorero(moneda, t))
    {
        return;
    }

    float angulo = t * 6.0f + moneda.x * 3.0f;
    Vector3 eje = { std::cos(angulo) * 0.04f, 0.0f, std::sin(angulo) * 0.04f };
    Vector3 centro = { moneda.x, moneda.y + 0.12f, moneda.z };

    // La cara del GLB mira +Z; el cilindro original tenia su normal en
    // (cos(angulo),0,sin(angulo)). Conservar esa orientacion y velocidad.
    if (DibujarModeloTesoreroCercadoRetro3D(MODELO_TESORERO_MONEDA,
        centro, 90.0f - angulo * RAD2DEG)) return;

    (DrawCylinderEx)(
        { centro.x - eje.x, centro.y, centro.z - eje.z },
        { centro.x + eje.x, centro.y, centro.z + eje.z },
        0.2f, 0.2f, 10, COLOR_ORO_TESORERO
    );
}


//==================================================
// VISUAL: JUGADORES
//==================================================

static bool EsTesoreroVisible(const MinijuegoTesoreroAcorralado& m, int i)
{
    return i == m.indiceTesorero && m.estadosJugadores[i].equipo == 0;
}


// Corona de oro y bolsa del tesoro a la espalda: el rol se lee de lejos.
static void DibujarInsigniasTesorero(const MinijuegoTesoreroAcorralado& m, const JugadorPrueba& jugador)
{
    if (jugador.cayendo)
    {
        return;
    }

    float t = m.tiempoAnimacion;
    float aplastado = jugador.aplastado ? 0.55f : 1.0f;
    float giro = ObtenerAnguloModeloJugadorCompartido(jugador);
    Vector3 p = jugador.posicion;
    float cabeza = p.y + 1.05f * aplastado;

    // Corona.
    Vector3 c = { p.x, cabeza + 0.04f * std::sin(t * 5.0f), p.z };
    PrismaTesorero(c, 0.25f, 0.28f, 0.16f, 10, COLOR_ORO_TESORERO, giro * DEG2RAD);
    for (int k = 0; k < 5; k++)
    {
        float a = giro * DEG2RAD + 6.2832f * (float)k / 5.0f;
        Vector3 punta = { c.x + std::cos(a) * 0.25f, c.y + 0.16f, c.z + std::sin(a) * 0.25f };
        PrismaTesorero(punta, 0.07f, 0.0f, 0.17f, 4, COLOR_ORO_CLARO_TESORERO);
    }
    float fx = std::sin(giro * DEG2RAD);
    float fz = std::cos(giro * DEG2RAD);
    EsferaTesorero({ c.x + fx * 0.27f, c.y + 0.09f, c.z + fz * 0.27f }, { 0.06f, 0.06f, 0.06f }, COLOR_TELA_TESORERO, 3, 6);

    // Bolsa de monedas a la espalda: crece o se achica con el oro que le queda.
    float lleno = 0.6f + 0.4f * Limitar01VisualTesorero((float)m.monedasTesorero / (float)MONEDAS_INICIALES_TESORERO);
    float vaiven = std::sin(t * 9.0f) * 0.03f;
    Vector3 b = { p.x - fx * 0.42f, p.y + 0.3f * aplastado + vaiven, p.z - fz * 0.42f };
    Color arpillera = Color{ 176, 134, 88, 255 };
    EsferaTesorero(b, { 0.36f * lleno, 0.4f * lleno, 0.36f * lleno }, arpillera, 4, 9);
    PrismaTesorero({ b.x, b.y + 0.34f * lleno, b.z }, 0.1f, 0.15f, 0.14f, 6, TonoTesorero(arpillera, 0.9f));
    PrismaTesorero({ b.x, b.y + 0.33f * lleno, b.z }, 0.12f, 0.12f, 0.05f, 6, COLOR_ORO_TESORERO);
    EsferaTesorero({ b.x, b.y + 0.5f * lleno, b.z }, { 0.15f, 0.08f, 0.15f }, COLOR_ORO_TESORERO, 3, 7);
    PrismaTesorero({ b.x + fx * 0.36f * lleno, b.y - 0.05f, b.z + fz * 0.36f * lleno }, 0.1f, 0.1f, 0.02f, 8, COLOR_ORO_CLARO_TESORERO);
}


static void DibujarAnillosJugadoresTesorero(
    const MinijuegoTesoreroAcorralado& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    float t = m.tiempoAnimacion;

    for (int i = 0; i < limite; i++)
    {
        if (m.estadosJugadores[i].equipo < 0 || jugadores[i].cayendo)
        {
            continue;
        }

        const JugadorPrueba& j = jugadores[i];
        float pies = j.posicion.y - j.tamano.y * 0.5f;
        float escala = 1.0f - Limitar01VisualTesorero(pies / 4.0f) * 0.35f;
        Vector3 c = { j.posicion.x, 0.03f, j.posicion.z };
        Color color = participantes[i].color;

        if (EsTesoreroVisible(m, i))
        {
            // Doble aro de oro con muescas que giran.
            float pulso = 1.0f + 0.05f * std::sin(t * 6.0f);
            AnilloTesorero(c, 0.0f, 0.56f * escala, AlfaTesorero(color, 0.3f), 20);
            AnilloTesorero({ c.x, 0.034f, c.z }, 0.6f * escala * pulso, 0.74f * escala * pulso, COLOR_ORO_TESORERO, 28);
            AnilloTesorero({ c.x, 0.036f, c.z }, 0.48f * escala, 0.55f * escala, color, 24);

            rlBegin(RL_QUADS);
            for (int k = 0; k < 6; k++)
            {
                float a = t * 1.6f + 6.2832f * (float)k / 6.0f;
                float r0 = 0.8f * escala * pulso, r1 = 0.94f * escala * pulso;
                float ca = std::cos(a), sa = std::sin(a), ca2 = std::cos(a + 0.22f), sa2 = std::sin(a + 0.22f);
                QuadTesorero({ c.x + ca * r0, 0.036f, c.z + sa * r0 }, { c.x + ca * r1, 0.036f, c.z + sa * r1 }, { c.x + ca2 * r1, 0.036f, c.z + sa2 * r1 }, { c.x + ca2 * r0, 0.036f, c.z + sa2 * r0 }, COLOR_ORO_CLARO_TESORERO);
            }
            rlEnd();
        }
        else
        {
            float pulso = 1.0f + 0.04f * std::sin(t * 5.0f + j.posicion.x);
            AnilloTesorero(c, 0.0f, 0.5f * escala, AlfaTesorero(color, 0.27f), 20);
            AnilloTesorero({ c.x, 0.034f, c.z }, 0.5f * escala * pulso, 0.62f * escala * pulso, color, 24);
        }
    }
}


//==================================================
// VISUAL: TRANSLUCIDOS, PARTICULAS Y LUCES
//==================================================

// Penumbra al pie de los muros y del pedestal, sombras de rejas, anillos
// de jugadores y sombra de las monedas.
static void DibujarTranslucidosTesorero(
    const MinijuegoTesoreroAcorralado& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    float t = m.tiempoAnimacion;
    const float X = MITAD_SUELO_X_TESORERO;
    const float Z = MITAD_SUELO_Z_TESORERO;
    const float P = MITAD_TORRE_TESORERO;
    Color sombra = { 40, 28, 40, 110 };
    Color nada = { 40, 28, 40, 0 };

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    rlBegin(RL_QUADS);
    VerticeTesorero({ -X, 0.016f, -Z }, sombra); VerticeTesorero({ -X, 0.016f, Z }, sombra);
    VerticeTesorero({ -X + 1.1f, 0.016f, Z }, nada); VerticeTesorero({ -X + 1.1f, 0.016f, -Z }, nada);
    VerticeTesorero({ X, 0.016f, -Z }, sombra); VerticeTesorero({ X, 0.016f, Z }, sombra);
    VerticeTesorero({ X - 1.1f, 0.016f, Z }, nada); VerticeTesorero({ X - 1.1f, 0.016f, -Z }, nada);
    VerticeTesorero({ -X, 0.016f, -Z }, sombra); VerticeTesorero({ X, 0.016f, -Z }, sombra);
    VerticeTesorero({ X, 0.016f, -Z + 1.5f }, nada); VerticeTesorero({ -X, 0.016f, -Z + 1.5f }, nada);

    // El sol bajo entra por el fondo: el pedestal proyecta sombra al frente.
    Color sombraP = { 40, 28, 40, 120 };
    VerticeTesorero({ -P, 0.016f, P }, sombraP); VerticeTesorero({ P, 0.016f, P }, sombraP);
    VerticeTesorero({ P + 0.4f, 0.016f, P + 2.2f }, nada); VerticeTesorero({ -P + 0.4f, 0.016f, P + 2.2f }, nada);
    rlEnd();

    // Rejas cerradas: sombra larga hacia la camara.
    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        const RejaTesorero& r = m.rejas[k];
        float a = r.altura;
        if (a < 0.05f) continue;
        Color s = { 40, 28, 40, (unsigned char)(90.0f * a) };
        Color s0 = s;
        s0.a = 0;
        bool alLargoDeZ = r.mitadZ > r.mitadX;
        rlBegin(RL_QUADS);
        if (alLargoDeZ)
        {
            VerticeTesorero({ r.x - 0.1f, 0.017f, r.z - r.mitadZ }, s); VerticeTesorero({ r.x + 0.1f, 0.017f, r.z - r.mitadZ }, s);
            VerticeTesorero({ r.x + 0.5f, 0.017f, r.z + r.mitadZ + 1.4f * a }, s0); VerticeTesorero({ r.x + 0.1f, 0.017f, r.z + r.mitadZ + 1.4f * a }, s0);
        }
        else
        {
            VerticeTesorero({ r.x - r.mitadX, 0.017f, r.z }, s); VerticeTesorero({ r.x + r.mitadX, 0.017f, r.z }, s);
            VerticeTesorero({ r.x + r.mitadX + 0.3f, 0.017f, r.z + 1.6f * a }, s0); VerticeTesorero({ r.x - r.mitadX + 0.3f, 0.017f, r.z + 1.6f * a }, s0);
        }
        rlEnd();
    }

    // Sombra de las monedas en el suelo (mas chica cuanto mas alto).
    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        const MonedaTesorero& moneda = m.monedas[i];
        if (!MonedaVisibleTesorero(moneda, t)) continue;
        float alto = Limitar01VisualTesorero((moneda.y - SUELO_MONEDA_TESORERO) / 2.5f);
        AnilloTesorero({ moneda.x, 0.02f, moneda.z }, 0.0f, 0.2f * (1.0f - 0.5f * alto), Color{ 40, 28, 40, (unsigned char)(110.0f * (1.0f - alto)) }, 12);

        // Sin luces aditivas, un aro dorado marca la moneda en el suelo.
        if (CalidadEfectos() == CALIDAD_BAJA)
        {
            AnilloTesorero({ moneda.x, 0.022f, moneda.z }, 0.26f, 0.34f, AlfaTesorero(COLOR_ORO_TESORERO, 0.7f), 14);
        }
    }

    DibujarAnillosJugadoresTesorero(m, jugadores, participantes, limite);

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarChispasTesorero(const MinijuegoTesoreroAcorralado& m)
{
    for (int i = 0; i < MAX_CHISPAS_TESORERO; i++)
    {
        const ChispaTesorero& p = m.chispas[i];
        if (!p.activa) continue;

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;

        if (p.tipo == CHISPA_TESORERO_MONEDA)
        {
            rlPushMatrix();
            rlTranslatef(p.posicion.x, p.posicion.y, p.posicion.z);
            rlRotatef(p.giro, 1.0f, 0.0f, 0.3f);
            CuboCrudoTesorero({ 0.0f, 0.0f, 0.0f }, { p.tamano, 0.03f, p.tamano }, p.color);
            rlPopMatrix();
        }
        else if (p.tipo == CHISPA_TESORERO_CONFETI)
        {
            rlPushMatrix();
            rlTranslatef(p.posicion.x, p.posicion.y, p.posicion.z);
            rlRotatef(p.giro, 0.4f, 1.0f, 0.2f);
            CuboCrudoTesorero({ 0.0f, 0.0f, 0.0f }, { p.tamano, 0.02f, p.tamano * 0.6f }, p.color);
            rlPopMatrix();
        }
        else
        {
            float tam = p.tamano * (0.4f + 0.6f * vida);
            CuboCrudoTesorero(p.posicion, { tam, tam, tam }, p.color);
        }
    }
}


// Aves que planean sobre el valle.
static void DibujarAvesTesorero(float t)
{
    Color ave = TinteTesorero(Color{ 46, 38, 52, 255 }, { 0.0f, 0.0f, -30.0f });
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < 7; k++)
    {
        float radio = 8.0f + 6.0f * Ruido01Tesorero(k, 240);
        float a = t * (0.18f + 0.08f * Ruido01Tesorero(k, 241)) + 6.2832f * Ruido01Tesorero(k, 242);
        Vector3 centro = { (k % 2 == 0 ? -22.0f : 22.0f) + 6.0f * Ruido01Tesorero(k, 243), -4.0f + 6.0f * Ruido01Tesorero(k, 244), -26.0f + 8.0f * Ruido01Tesorero(k, 245) };
        Vector3 p = { centro.x + std::cos(a) * radio, centro.y + std::sin(a * 2.0f) * 0.4f, centro.z + std::sin(a) * radio * 0.6f };
        float aleteo = std::sin(t * 7.0f + (float)k * 1.3f) * 0.35f;
        float dx = -std::sin(a), dz = std::cos(a) * 0.6f;
        float l = std::sqrt(dx * dx + dz * dz);
        dx /= l; dz /= l;
        float ex = -dz * 0.7f, ez = dx * 0.7f;
        VerticeTesorero({ p.x + dx * 0.25f, p.y, p.z + dz * 0.25f }, ave);
        VerticeTesorero({ p.x + ex, p.y + aleteo, p.z + ez }, ave);
        VerticeTesorero({ p.x - dx * 0.1f, p.y, p.z - dz * 0.1f }, ave);
        VerticeTesorero({ p.x + dx * 0.25f, p.y, p.z + dz * 0.25f }, ave);
        VerticeTesorero({ p.x - dx * 0.1f, p.y, p.z - dz * 0.1f }, ave);
        VerticeTesorero({ p.x - ex, p.y + aleteo, p.z - ez }, ave);
    }
    rlEnd();
}


// Bancos de bruma que se deslizan por el valle.
static void DibujarBrumaTesorero(float t)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlBegin(RL_QUADS);
    for (int k = 0; k < 10; k++)
    {
        float u = std::fmod(t * 0.012f + Ruido01Tesorero(k, 250), 1.0f);
        float x = -80.0f + 160.0f * u;
        float z = -18.0f - 40.0f * Ruido01Tesorero(k, 251);
        float y = Y_VALLE_TESORERO + 1.5f + 3.0f * Ruido01Tesorero(k, 252);
        float ancho = 14.0f + 10.0f * Ruido01Tesorero(k, 253);
        float fondo = 6.0f + 4.0f * Ruido01Tesorero(k, 254);
        float alfa = std::sin(u * 3.1416f) * 0.45f;
        Color c = AlfaTesorero(Color{ 238, 206, 200, 255 }, alfa);
        Color c0 = AlfaTesorero(c, 0.0f);
        VerticeTesorero({ x - ancho, y, z }, c0);
        VerticeTesorero({ x, y, z + fondo }, c);
        VerticeTesorero({ x + ancho, y, z }, c0);
        VerticeTesorero({ x, y, z - fondo }, c);
    }
    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarLucesTesorero(
    const MinijuegoTesoreroAcorralado& m,
    const JugadorPrueba jugadores[],
    int limite
)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Antorchas: halo en la llama y charco tibio en las losas.
    for (int k = 0; k < 8; k++)
    {
        Vector3 p = PosicionAntorchaTesorero(k);
        float pulso = 0.86f + 0.1f * std::sin(t * 9.0f + (float)k * 2.1f) + 0.05f * std::sin(t * 23.0f + (float)k);
        ResplandorFrenteTesorero({ p.x, p.y + 0.2f, p.z }, 1.1f * pulso, Color{ 255, 140, 60, 110 });
        ResplandorFrenteTesorero({ p.x, p.y + 0.15f, p.z }, 0.42f * pulso, Color{ 255, 220, 140, 160 });

        Vector3 charco = k < 6
            ? Vector3{ p.x + (p.x < 0.0f ? 1.3f : -1.3f), 0.02f, p.z }
            : Vector3{ p.x, 0.02f, p.z + 1.2f };
        for (int r = 0; r < 3; r++)
        {
            ResplandorSueloTesorero({ charco.x, charco.y + 0.002f * (float)r, charco.z }, (1.3f + 0.8f * (float)r) * pulso, Color{ 255, 150, 70, 30 });
        }

        // Luz sobre la pared detras de la llama.
        Vector3 n = k < 6 ? Vector3{ p.x < 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 0.0f, 1.0f };
        Vector3 pared = SumaTesorero(p, EscalaTesorero(n, -0.28f));
        Vector3 u = std::fabs(n.x) > 0.5f ? Vector3{ 0.0f, 0.0f, 1.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
        ResplandorTesorero({ pared.x, pared.y + 0.1f, pared.z }, u, { 0.0f, 1.0f, 0.0f }, 1.2f * pulso, Color{ 255, 150, 70, 46 });
    }

    // Braseros de las torres bajas y sus chispas.
    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 b = { (float)s * 10.6f, 3.95f, 7.6f };
        float pulso = 0.85f + 0.15f * std::sin(t * 8.0f + (float)s) + 0.05f * std::sin(t * 21.0f);
        ResplandorFrenteTesorero(b, 1.9f * pulso, Color{ 255, 140, 60, 110 });
        ResplandorFrenteTesorero(b, 0.7f * pulso, Color{ 255, 220, 140, 150 });
        ResplandorSueloTesorero({ (float)s * 9.0f, 0.025f, 6.2f }, 2.4f * pulso, Color{ 255, 150, 70, 34 });

        int chispas = (int)(10.0f * FactorCalidadGrafica(CalidadParticulas()));
        for (int k = 0; k < chispas; k++)
        {
            float ciclo = 1.4f + 0.8f * Ruido01Tesorero(k, 260 + s);
            float u = std::fmod(t / ciclo + Ruido01Tesorero(k, 262 + s), 1.0f);
            Vector3 p = { b.x + std::sin(t * 2.0f + (float)k) * 0.35f * u, b.y + 2.6f * u, b.z + std::cos(t * 1.6f + (float)k) * 0.2f * u };
            ResplandorFrenteTesorero(p, 0.08f, Color{ 255, 190, 90, (unsigned char)(230.0f * (1.0f - u)) });
        }
    }

    // Ventanales de la torre del homenaje.
    for (int k = 0; k < 4; k++)
    {
        float x = k < 2 ? -3.2f : 3.2f;
        float y = (k % 2 == 0 ? 4.4f : 6.6f) + 0.46f;
        float pulso = 0.9f + 0.1f * std::sin(t * 3.0f + (float)k * 1.7f);
        ResplandorTesorero({ x, y, -9.1f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.0f * pulso, Color{ 255, 170, 90, 80 });
    }

    // Brillo del cofre del tesoro.
    float latido = 0.8f + 0.2f * std::sin(t * 2.6f);
    ResplandorFrenteTesorero({ 0.0f, ALTO_TORRE_TESORERO + 0.45f, -0.1f }, 1.2f * latido, Color{ 255, 200, 80, 90 });
    ResplandorSueloTesorero({ 0.0f, ALTO_TORRE_TESORERO + 0.02f, 0.0f }, 1.3f, Color{ 255, 200, 80, 40 });
    DestelloTesorero({ 0.25f, ALTO_TORRE_TESORERO + 0.6f, -0.1f }, 0.35f * latido, Color{ 255, 240, 190, 200 }, t * 0.8f);

    // Faroles de las rejas.
    float altoMarco = AltoMarcoRejaTesorero();
    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        const RejaTesorero& r = m.rejas[k];
        Vector3 farol = { r.x, altoMarco + 0.25f, r.z };
        bool aviso = r.estado == REJA_TESORERO_AVISO;
        float pulso = 0.5f + 0.5f * std::sin(t * 18.0f);

        if (r.estado == REJA_TESORERO_ABIERTA)
        {
            ResplandorFrenteTesorero(farol, 0.5f, Color{ 255, 220, 160, 90 });
        }
        else
        {
            ResplandorFrenteTesorero(farol, aviso ? 0.6f + 0.5f * pulso : 0.55f, Color{ 255, 80, 50, (unsigned char)(aviso ? 90.0f + 100.0f * pulso : 90.0f) });
        }

        if (aviso)
        {
            // Resplandor rojo sobre el umbral: aca baja la reja.
            bool alLargoDeZ = r.mitadZ > r.mitadX;
            Color c = { 255, 70, 40, (unsigned char)(50.0f + 60.0f * pulso) };
            ResplandorTesorero({ r.x, 0.03f, r.z }, alLargoDeZ ? Vector3{ 0.0f, 0.0f, 1.0f } : Vector3{ 1.0f, 0.0f, 0.0f },
                alLargoDeZ ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 0.0f, 1.0f }, 0.9f, c, (r.mitadX > r.mitadZ ? r.mitadX : r.mitadZ) * 1.5f);
        }
    }

    // Monedas sueltas: halo, charco dorado y destello que gira.
    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        const MonedaTesorero& moneda = m.monedas[i];
        if (!MonedaVisibleTesorero(moneda, t)) continue;
        Vector3 c = { moneda.x, moneda.y + 0.12f, moneda.z };
        float brillo = 0.75f + 0.25f * std::sin(t * 7.0f + (float)i * 1.3f);
        ResplandorFrenteTesorero(c, 0.75f * brillo, Color{ 255, 200, 70, 150 });
        ResplandorSueloTesorero({ c.x, 0.024f, c.z }, 0.95f, Color{ 255, 190, 70, 56 });
        DestelloTesorero({ c.x, c.y + 0.05f, c.z + 0.05f }, 0.42f * brillo, Color{ 255, 246, 200, 230 }, t * 2.0f + (float)i);
    }

    // Aura del tesorero y destello de cada golpe.
    int ti = m.indiceTesorero;
    if (ti >= 0 && ti < limite && m.estadosJugadores[ti].equipo == 0 && !jugadores[ti].cayendo)
    {
        Vector3 p = jugadores[ti].posicion;
        float pulso = 0.85f + 0.15f * std::sin(t * 4.0f);
        ResplandorSueloTesorero({ p.x, 0.026f, p.z }, 1.4f * pulso, Color{ 255, 196, 70, 46 });
        ResplandorFrenteTesorero({ p.x, p.y + 0.9f, p.z }, 0.6f * pulso, Color{ 255, 210, 90, 70 });

        if (m.destelloTesorero > 0.0f)
        {
            float d = m.destelloTesorero;
            ResplandorFrenteTesorero({ p.x, p.y + 0.2f, p.z + 0.2f }, 0.6f + 1.4f * d, Color{ 255, 236, 180, (unsigned char)(170.0f * d) });
        }
    }

    // Brillo de las chispas de oro y del confeti dorado.
    for (int i = 0; i < MAX_CHISPAS_TESORERO; i++)
    {
        const ChispaTesorero& p = m.chispas[i];
        if (!p.activa || p.tipo == CHISPA_TESORERO_POLVO || p.tipo == CHISPA_TESORERO_CONFETI) continue;
        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        ResplandorFrenteTesorero(p.posicion, 0.22f, AlfaTesorero(p.color, 0.6f * vida));
    }

    // Motas de polvo dorado en el aire tibio del ocaso.
    int motas = (int)(26.0f * FactorCalidadGrafica(CalidadParticulas()));
    for (int k = 0; k < motas; k++)
    {
        float ciclo = 9.0f + 6.0f * Ruido01Tesorero(k, 270);
        float u = std::fmod(t / ciclo + Ruido01Tesorero(k, 271), 1.0f);
        Vector3 p =
        {
            -9.0f + 18.0f * Ruido01Tesorero(k, 272) + std::sin(t * 0.6f + (float)k) * 0.5f,
            0.4f + 3.6f * u,
            -7.0f + 14.0f * Ruido01Tesorero(k, 273)
        };
        float alfa = std::sin(u * 3.1416f);
        ResplandorFrenteTesorero(p, 0.05f, Color{ 255, 226, 170, (unsigned char)(170.0f * alfa) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: EFECTOS (SE ACTUALIZAN CON EL JUEGO)
//==================================================
//
// Solo leen lo que hizo la logica (antes y despues de cada paso) y usan
// su propia semilla: no consumen GetRandomValue ni tocan las reglas.

static float AzarVisualTesorero(MinijuegoTesoreroAcorralado& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasTesorero(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static void EmitirChispaTesorero(
    MinijuegoTesoreroAcorralado& m,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    int indice = -1;
    float menorVida = 1e9f;

    for (int i = 0; i < MAX_CHISPAS_TESORERO; i++)
    {
        if (!m.chispas[i].activa)
        {
            indice = i;
            break;
        }

        if (m.chispas[i].vida < menorVida)
        {
            menorVida = m.chispas[i].vida;
            indice = i;
        }
    }

    ChispaTesorero& p = m.chispas[indice];
    p.activa = true;
    p.tipo = tipo;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.vida = vida;
    p.vidaMaxima = vida;
    p.tamano = tamano;
    p.giro = AzarVisualTesorero(m, 0.0f, 360.0f);
    p.color = color;
}


static void AgregarAvisoTesorero(MinijuegoTesoreroAcorralado& m, int jugador, Vector3 posicion, int cantidad)
{
    // Las monedas que se juntan seguidas suman en el mismo cartel.
    for (int i = 0; i < MAX_AVISOS_TESORERO; i++)
    {
        AvisoTesorero& a = m.avisos[i];
        if (a.activo && a.jugador == jugador && (a.cantidad > 0) == (cantidad > 0) && a.vida > DURACION_AVISO_TESORERO - 0.45f)
        {
            a.cantidad += cantidad;
            a.posicion = posicion;
            a.vida = DURACION_AVISO_TESORERO;
            return;
        }
    }

    int indice = 0;
    for (int i = 0; i < MAX_AVISOS_TESORERO; i++)
    {
        if (!m.avisos[i].activo)
        {
            indice = i;
            break;
        }
        if (m.avisos[i].vida < m.avisos[indice].vida) indice = i;
    }

    AvisoTesorero& a = m.avisos[indice];
    a.activo = true;
    a.jugador = jugador;
    a.posicion = posicion;
    a.cantidad = cantidad;
    a.vida = DURACION_AVISO_TESORERO;
}


static void TomarInstantaneaTesorero(const MinijuegoTesoreroAcorralado& m, InstantaneaTesorero& s)
{
    s.fase = m.fase;
    s.monedasTesorero = m.monedasTesorero;

    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        s.monedaActiva[i] = m.monedas[i].activa;
        s.edadMoneda[i] = m.monedas[i].edad;
        s.posicionMoneda[i] = { m.monedas[i].x, m.monedas[i].y, m.monedas[i].z };
    }

    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        s.alturaReja[k] = m.rejas[k].altura;
    }
}


static void ActualizarVisualTesorero(
    MinijuegoTesoreroAcorralado& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    const InstantaneaTesorero& antes
)
{
    // Chispas, avisos y anuncios en curso.
    for (int i = 0; i < MAX_CHISPAS_TESORERO; i++)
    {
        ChispaTesorero& p = m.chispas[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float gravedad = p.tipo == CHISPA_TESORERO_CONFETI ? 2.2f : (p.tipo == CHISPA_TESORERO_POLVO ? -0.6f : 9.0f);
        p.velocidad.y -= gravedad * deltaTime;
        if (p.tipo == CHISPA_TESORERO_CONFETI)
        {
            p.velocidad.x *= 1.0f - 1.5f * deltaTime;
            p.velocidad.z *= 1.0f - 1.5f * deltaTime;
        }
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;
        p.giro += 540.0f * deltaTime;

        if (p.posicion.y < 0.05f && p.tipo != CHISPA_TESORERO_POLVO)
        {
            p.posicion.y = 0.05f;
            p.velocidad.y = -p.velocidad.y * 0.35f;
            p.velocidad.x *= 0.6f;
            p.velocidad.z *= 0.6f;
        }
    }

    for (int i = 0; i < MAX_AVISOS_TESORERO; i++)
    {
        AvisoTesorero& a = m.avisos[i];
        if (!a.activo) continue;
        a.vida -= deltaTime;
        if (a.vida <= 0.0f) a.activo = false;
    }

    if (m.tiempoAnuncio >= 0.0f)
    {
        m.tiempoAnuncio += deltaTime;
        if (m.tiempoAnuncio > DURACION_ANUNCIO_TESORERO) m.tiempoAnuncio = -1.0f;
    }

    m.destelloTesorero -= deltaTime * 2.5f;
    if (m.destelloTesorero < 0.0f) m.destelloTesorero = 0.0f;

    if (!m.partidaValida)
    {
        return;
    }

    int t = m.indiceTesorero;

    // Arranque: se reparten los roles.
    if (antes.fase == FASE_TESORERO_PREPARACION && m.fase == FASE_TESORERO_JUGANDO)
    {
        m.tipoAnuncio = 1;
        m.tiempoAnuncio = 0.0f;
    }

    // Monedas recogidas: brillo del color de quien la junto.
    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        if (!antes.monedaActiva[i] || m.monedas[i].activa || antes.edadMoneda[i] + deltaTime >= VIDA_MONEDA_TESORERO)
        {
            continue;
        }

        Vector3 p = antes.posicionMoneda[i];
        int quien = -1;
        float mejor = 1e9f;
        for (int j = 0; j < limite; j++)
        {
            if (m.estadosJugadores[j].equipo < 0) continue;
            float dx = jugadores[j].posicion.x - p.x;
            float dz = jugadores[j].posicion.z - p.z;
            float d = dx * dx + dz * dz;
            if (d < mejor) { mejor = d; quien = j; }
        }

        Color color = quien == t || quien < 0 ? COLOR_ORO_TESORERO : participantes[quien].color;
        int n = CantidadParticulasTesorero(6);
        for (int k = 0; k < n; k++)
        {
            float a = AzarVisualTesorero(m, 0.0f, 6.2832f);
            float v = AzarVisualTesorero(m, 0.8f, 2.0f);
            EmitirChispaTesorero(m, CHISPA_TESORERO_BRILLO, { p.x, p.y + 0.2f, p.z }, { std::cos(a) * v, AzarVisualTesorero(m, 2.0f, 3.6f), std::sin(a) * v }, 0.5f, 0.07f, MezclaTesorero(color, WHITE, 0.3f));
        }

        if (quien >= 0)
        {
            AgregarAvisoTesorero(m, quien, jugadores[quien].posicion, 1);
        }
    }

    // Monedas soltadas: el tesorero recibio un golpe.
    int soltadas = 0;
    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        if (m.monedas[i].activa && (!antes.monedaActiva[i] || m.monedas[i].edad < antes.edadMoneda[i]))
        {
            soltadas++;
        }
    }

    if (soltadas > 0 && t >= 0)
    {
        Vector3 p = jugadores[t].posicion;
        m.destelloTesorero = 1.0f;
        AgregarAvisoTesorero(m, t, p, -soltadas);

        int n = CantidadParticulasTesorero(soltadas > MONEDAS_GOLPE_TESORERO ? 22 : 12);
        for (int k = 0; k < n; k++)
        {
            float a = AzarVisualTesorero(m, 0.0f, 6.2832f);
            float v = AzarVisualTesorero(m, 1.5f, 4.0f);
            EmitirChispaTesorero(m, CHISPA_TESORERO_MONEDA, { p.x, p.y + 0.5f, p.z }, { std::cos(a) * v, AzarVisualTesorero(m, 3.0f, 6.0f), std::sin(a) * v }, AzarVisualTesorero(m, 0.6f, 1.0f), 0.13f, k % 3 == 0 ? COLOR_ORO_CLARO_TESORERO : COLOR_ORO_TESORERO);
        }
        for (int k = 0; k < CantidadParticulasTesorero(8); k++)
        {
            float a = AzarVisualTesorero(m, 0.0f, 6.2832f);
            EmitirChispaTesorero(m, CHISPA_TESORERO_BRILLO, { p.x, p.y + 0.3f, p.z }, { std::cos(a) * 4.0f, AzarVisualTesorero(m, 0.5f, 2.5f), std::sin(a) * 4.0f }, 0.3f, 0.08f, Color{ 255, 250, 230, 255 });
        }

        if (soltadas > MONEDAS_GOLPE_TESORERO)
        {
            m.tipoAnuncio = 2;
            m.tiempoAnuncio = 0.0f;
            ActivarTemblorCamaraGeneral(0.18f, 0.3f);
        }

        // Primera vez que el tesoro baja de 65: aviso de peligro.
        if (!m.avisoOroDado && m.monedasTesorero <= 65 && m.monedasTesorero * 2 > MONEDAS_INICIALES_TESORERO)
        {
            m.avisoOroDado = true;
            if (m.tipoAnuncio != 2 || m.tiempoAnuncio < 0.0f)
            {
                m.tipoAnuncio = 3;
                m.tiempoAnuncio = 0.0f;
            }
        }
    }

    // Una reja termina de bajar: polvo contra el umbral.
    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        const RejaTesorero& r = m.rejas[k];
        if (antes.alturaReja[k] >= 1.0f || r.altura < 1.0f) continue;

        bool alLargoDeZ = r.mitadZ > r.mitadX;
        int n = CantidadParticulasTesorero(10);
        for (int i = 0; i < n; i++)
        {
            float f = AzarVisualTesorero(m, -1.0f, 1.0f);
            Vector3 p = { r.x + (alLargoDeZ ? 0.0f : f * r.mitadX), 0.1f, r.z + (alLargoDeZ ? f * r.mitadZ : 0.0f) };
            float lado = AzarVisualTesorero(m, -1.0f, 1.0f);
            Vector3 v = { alLargoDeZ ? lado * 1.2f : 0.0f, AzarVisualTesorero(m, 0.3f, 0.9f), alLargoDeZ ? 0.0f : lado * 1.2f };
            EmitirChispaTesorero(m, CHISPA_TESORERO_POLVO, p, v, AzarVisualTesorero(m, 0.5f, 0.9f), AzarVisualTesorero(m, 0.12f, 0.22f), Color{ 196, 182, 164, 255 });
        }
    }

    // Final: confeti sobre los ganadores y lluvia de oro si gano el tesorero.
    if (m.fase == FASE_TESORERO_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        float antesFinal = m.tiempoFinal;
        m.tiempoFinal += deltaTime;
        int pulsos = (int)(m.tiempoFinal / 0.3f) - (int)(antesFinal / 0.3f);
        if (antesFinal == 0.0f) pulsos = 1;

        if (m.tiempoFinal < 3.0f && pulsos > 0)
        {
            for (int i = 0; i < limite; i++)
            {
                if (m.estadosJugadores[i].equipo < 0 || m.resultado.participantes[i].posicionFinal != 1) continue;

                Vector3 p = jugadores[i].posicion;
                for (int k = 0; k < CantidadParticulasTesorero(7); k++)
                {
                    float a = AzarVisualTesorero(m, 0.0f, 6.2832f);
                    Color c = k % 2 == 0 ? participantes[i].color : (k % 3 == 0 ? COLOR_CREMA_TESORERO : COLOR_ORO_TESORERO);
                    EmitirChispaTesorero(m, CHISPA_TESORERO_CONFETI, { p.x, p.y + 1.6f, p.z }, { std::cos(a) * 2.0f, AzarVisualTesorero(m, 2.5f, 4.5f), std::sin(a) * 2.0f }, 2.2f, 0.14f, c);
                }
            }

            if (m.ganaTesorero)
            {
                for (int k = 0; k < CantidadParticulasTesorero(4); k++)
                {
                    float a = AzarVisualTesorero(m, 0.0f, 6.2832f);
                    EmitirChispaTesorero(m, CHISPA_TESORERO_MONEDA, { 0.0f, ALTO_TORRE_TESORERO + 0.6f, -0.2f }, { std::cos(a) * 1.6f, AzarVisualTesorero(m, 4.0f, 6.0f), std::sin(a) * 1.6f }, 1.2f, 0.14f, COLOR_ORO_TESORERO);
                }
            }
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

static void DibujarEscenarioTesorero(const MinijuegoTesoreroAcorralado& m)
{
    float t = m.tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    bool animado = CalidadEfectos() != CALIDAD_BAJA;

    // Horizonte, risco y foso.
    DibujarValleTesorero(t, decoracion, paso);
    DibujarRiscoTesorero(decoracion);
    DibujarFosoTesorero(t, animado);

    // Media distancia: murallas, torres y torre del homenaje.
    DibujarMurallasTesorero(t, decoracion);

    // Arena: patio y pedestal del tesoro.
    DibujarPatioTesorero(m, decoracion);
    DibujarPedestalTesorero(t);

    if (!decoracion)
    {
        return;
    }

    // Anillo cercano: adarves y explanada.
    DibujarAdarveTesorero(t, paso);
    DibujarAfuerasTesorero(t, paso);
    DibujarAvesTesorero(t);
}


void MinijuegoTesoreroAcorralado::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteJugadoresTesorero(cantidadMaxima);
    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    Camera3D vista = camara;

    if (fase == FASE_TESORERO_PREPARACION && partidaValida)
    {
        float u = Limitar01VisualTesorero(tiempoPreparacion / DURACION_PREPARACION_TESORERO);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 4.0f * suave;
        vista.position.z += 3.0f * suave;
        vista.target.z -= 1.5f * suave;
    }

    ClearBackground(COLOR_NIEBLA_TESORERO);
    DibujarCieloTesorero();

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarEscenarioTesorero(*this);

    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        DibujarRejaTesorero(rejas[k], t);
    }

    for (int i = 0; i < MAX_MONEDAS_TESORERO; i++)
    {
        DibujarMonedaTesorero(monedas[i], t);
    }

    DibujarTranslucidosTesorero(*this, jugadores, participantes, limite);

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        Participante visual = participantes[i];
        visual.conectado = true;

        if (EsTesoreroVisible(*this, i) && destelloTesorero > 0.0f)
        {
            // Sacudida corta al recibir el golpe.
            JugadorPrueba sacudido = jugadores[i];
            sacudido.posicion.x += std::sin(t * 70.0f) * 0.07f * destelloTesorero;
            DibujarJugadorModeloCompartido(sacudido, visual);
            DibujarInsigniasTesorero(*this, sacudido);
        }
        else
        {
            DibujarJugadorModeloCompartido(jugadores[i], visual);
            if (EsTesoreroVisible(*this, i)) DibujarInsigniasTesorero(*this, jugadores[i]);
        }

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA);
    DibujarChispasTesorero(*this);

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        DibujarBrumaTesorero(t);
    }

    if (CalidadEfectos() != CALIDAD_BAJA)
    {
        DibujarLucesTesorero(*this, jugadores, limite);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // «¡CUIDADO!» en el vano de la reja que esta por bajar.
    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        if (fase != FASE_TESORERO_JUGANDO || rejas[k].estado != REJA_TESORERO_AVISO) continue;
        Vector2 s = GetWorldToScreen({ rejas[k].x, 1.3f, rejas[k].z }, vista);
        float pulso = 0.75f + 0.25f * std::sin(t * 18.0f);
        DibujarTextoMundoContornoUI("¡CUIDADO!", s.x, s.y, UI_TEXTO, Color{ 255, 136, 100, 255 }, pulso);
    }

    // Carteles J1..J4 (y el rol del tesorero al empezar).
    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo < 0 || jugadores[i].cayendo || !participantes[i].activo)
        {
            continue;
        }

        bool esTesorero = EsTesoreroVisible(*this, i);
        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + (esTesorero ? 1.05f : 0.75f);
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        Rectangle placa = DibujarEtiquetaUI(rotulo, p, 0.0f, op);

        bool presentar = fase == FASE_TESORERO_PREPARACION ||
            (fase == FASE_TESORERO_JUGANDO && tiempoRestante > DURACION_PARTIDA_TESORERO - 2.5f);

        if (esTesorero && presentar)
        {
            OpcionesEtiquetaUI rol;
            rol.borde = COLOR_ORO_TESORERO;
            DibujarEtiquetaUI("TESORERO", { p.x, placa.y - Px(14.0f) }, 0.0f, rol);
        }
    }

    // «+1» / «-5»: monedas que se juntan o se pierden.
    for (int i = 0; i < MAX_AVISOS_TESORERO; i++)
    {
        const AvisoTesorero& a = avisos[i];
        if (!a.activo || a.jugador < 0) continue;

        float u = 1.0f - a.vida / DURACION_AVISO_TESORERO;
        Vector3 p = { a.posicion.x, a.posicion.y + 1.9f + u * 1.0f, a.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);
        float alfa = a.vida < 0.35f ? a.vida / 0.35f : 1.0f;

        char texto[16];
        if (a.cantidad > 0) std::snprintf(texto, sizeof(texto), "+%d", a.cantidad);
        else std::snprintf(texto, sizeof(texto), "-%d", -a.cantidad);

        Color color = a.cantidad < 0
            ? Color{ 255, 214, 96, 255 }
            : (a.jugador == indiceTesorero ? COLOR_ORO_CLARO_TESORERO : MezclaTesorero(participantes[a.jugador].color, WHITE, 0.35f));
        DibujarTextoMundoContornoUI(texto, s.x, s.y, a.cantidad < 0 ? UI_TEXTO_GRANDE : UI_TEXTO, color, alfa);
    }

    // Anuncios grandes.
    if (tiempoAnuncio >= 0.0f && fase != FASE_TESORERO_PREPARACION)
    {
        float u = tiempoAnuncio;
        float alfa = u < 0.15f ? u / 0.15f : (u > DURACION_ANUNCIO_TESORERO - 0.4f ? (DURACION_ANUNCIO_TESORERO - u) / 0.4f : 1.0f);
        float y = (float)alto * 0.24f;
        const char* texto = tipoAnuncio == 1
            ? "¡ATRAPEN AL TESORERO!"
            : (tipoAnuncio == 2 ? "¡GOLPE AL SUELO!" : "¡EL TESORO SE ACHICA!");
        Color color = tipoAnuncio == 3 ? Color{ 255, 170, 120, 255 } : COLOR_ORO_CLARO_TESORERO;
        DibujarTextoMundoContornoUI(texto, (float)ancho * 0.5f, y, UI_TEXTO_GRANDE, color, alfa);
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)ancho * 0.5f, (float)alto * 0.45f, UI_TEXTO_GRANDE, Color{ 255, 170, 120, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "TESORERO CERCADO",
        "PERSEGUÍ AL TESORERO Y GOLPEALO PARA QUE SUELTE MONEDAS. ÉL GANA SI CONSERVA MÁS DE 50.",
        resultado
    );

    hud.fase = fase == FASE_TESORERO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_TESORERO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_TESORERO;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = tiempoRestante <= 0.0f;

    int cazadores = 0;
    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo == 1) cazadores++;
    }
    const char* rolCazador = cazadores == 3 ? "TRÍO" : "CAZADOR";

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible || i >= limite || estadosJugadores[i].equipo < 0) continue;

        const Participante& participante = participantes[i];
        bool esTesorero = i == indiceTesorero;

        if (esTesorero)
        {
            j.equipo = 0;
            j.rol = "TESORERO";
            j.valorNumerico = monedasTesorero;
            j.progreso = Limitar01VisualTesorero((float)monedasTesorero / (float)MONEDAS_INICIALES_TESORERO);
            std::snprintf(j.valor, sizeof(j.valor), "%d/%d", monedasTesorero, MONEDAS_INICIALES_TESORERO);
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "CONSERVÓ %d", monedasTesorero);
            if (fase == FASE_TESORERO_JUGANDO && destelloTesorero > 0.3f) std::snprintf(j.estado, sizeof(j.estado), "¡GOLPEADO!");
        }
        else
        {
            int robadas = estadosJugadores[i].monedasRobadas;
            j.equipo = 1;
            j.rol = rolCazador;
            j.valorNumerico = robadas;
            std::snprintf(j.valor, sizeof(j.valor), "%d", robadas);
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), robadas == 1 ? "ROBÓ 1 MONEDA" : "ROBÓ %d MONEDAS", robadas);
        }

        if (!participante.esBot && participante.conectado)
        {
            if (esTesorero)
            {
                std::snprintf(j.controles, sizeof(j.controles), "HUÍ: %s  SALTÁ: %s\nSALTO EN EL AIRE: APLASTÁ", TextoMoverHud(participante), ObtenerTextoBotonPrincipal(participante));
            }
            else
            {
                std::snprintf(j.controles, sizeof(j.controles), "GOLPEÁ: %s  SALTÁ: %s\nSALTO EN EL AIRE: APLASTÁ", TextoBotonGolpeHud(participante), ObtenerTextoBotonPrincipal(participante));
            }
        }
    }

    char nota[64];
    if (fase == FASE_TESORERO_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (ganaTesorero) hud.tituloFinal = "¡GANA EL TESORERO!";
        else if (cazadores == 3) hud.tituloFinal = "¡GANA EL TRÍO!";
        else if (cazadores == 1) hud.tituloFinal = "¡GANA EL CAZADOR!";
        else hud.tituloFinal = "¡GANAN LOS CAZADORES!";

        std::snprintf(nota, sizeof(nota), "EL TESORERO CONSERVÓ %d DE %d MONEDAS", monedasTesorero, MONEDAS_INICIALES_TESORERO);
        hud.notaFinal = nota;
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoTesoreroAcorralado::ObtenerResultado() const
{
    return resultado;
}
