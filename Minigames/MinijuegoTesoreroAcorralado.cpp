#include "Minigames/MinijuegoTesoreroAcorralado.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


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


static Color ColorEquipoTesorero(int equipo)
{
    return equipo == 0
        ? Color{ 255, 205, 60, 255 }
        : Color{ 238, 55, 66, 255 };
}


static const char* TextoBotonGolpeTesorero(const Participante& participante)
{
    if (participante.control == CONTROL_GAMEPAD)
    {
        return "B";
    }

    return participante.control == CONTROL_TECLADO_FLECHAS ? "SHIFT DER" : "E";
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

    ConstruirMapaTesorero(*this);

    // Camara cenital diagonal que encuadra todo el patio.
    camara.position = { 0.0f, 17.5f, 13.5f };
    camara.target = { 0.0f, 0.4f, 0.6f };
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
// VISUAL: ESCENARIO
//==================================================

// Patio de armas: foso, muros con almenas, torres de esquina, estandartes,
// antorchas, puerta del fondo y torre del homenaje.
// MODELO FUTURO: muros y almenas, torres de esquina con techo conico,
// torre del homenaje, estandartes de tela, antorchas, foso con agua, y las
// rejas levadizas (reja + poleas) como GLB animables; suelo de losas.
static void DibujarPatioTesorero(const MinijuegoTesoreroAcorralado& minijuego)
{
    float t = minijuego.tiempoAnimacion;
    const Color piedra = Color{ 132, 128, 122, 255 };
    const Color piedraOscura = Color{ 96, 92, 90, 255 };
    const Color madera = Color{ 110, 74, 42, 255 };

    // Foso de agua alrededor del castillo.
    DrawCube({ 0.0f, -0.9f, 0.0f }, 36.0f, 1.6f, 28.0f, Color{ 38, 92, 140, 255 });

    for (int i = 0; i < 12; i++)
    {
        float x = -15.0f + Ruido01Tesorero(i, 1) * 30.0f;
        float z = -11.0f + Ruido01Tesorero(i, 2) * 22.0f;

        if (AbsTesorero(x) < 10.5f && AbsTesorero(z) < 8.5f)
        {
            continue;
        }

        DrawCube({ x + std::sin(t + (float)i) * 0.4f, -0.08f, z }, 1.6f, 0.02f, 0.25f, Fade(RAYWHITE, 0.35f));
    }

    // Suelo de losas.
    DrawCube({ 0.0f, -0.5f, 0.0f }, MITAD_SUELO_X_TESORERO * 2.0f, 1.0f, MITAD_SUELO_Z_TESORERO * 2.0f, Color{ 118, 114, 108, 255 });

    for (int ix = 0; ix < 9; ix++)
    {
        for (int iz = 0; iz < 7; iz++)
        {
            if ((ix + iz) % 2 == 0)
            {
                DrawCube(
                    { -8.0f + (float)ix * 2.0f, 0.01f, -6.0f + (float)iz * 2.0f },
                    1.96f, 0.02f, 1.96f,
                    Color{ 134, 130, 124, 255 }
                );
            }
        }
    }

    // Muros: fondo alto, laterales medios y frente bajo para ver la arena.
    DrawCube({ 0.0f, 1.3f, -7.6f }, 20.4f, 2.6f, 1.2f, piedra);
    DrawCube({ -9.6f, 1.1f, 0.0f }, 1.2f, 2.2f, 15.2f, piedra);
    DrawCube({ 9.6f, 1.1f, 0.0f }, 1.2f, 2.2f, 15.2f, piedra);
    DrawCube({ 0.0f, 0.25f, 7.6f }, 20.4f, 0.5f, 1.2f, piedra);

    // Almenas.
    for (int i = 0; i < 17; i++)
    {
        DrawCube({ -9.6f + (float)i * 1.2f, 2.85f, -7.6f }, 0.7f, 0.5f, 1.2f, piedraOscura);
    }

    for (int i = 0; i < 12; i++)
    {
        float z = -6.6f + (float)i * 1.2f;
        DrawCube({ -9.6f, 2.45f, z }, 1.2f, 0.5f, 0.7f, piedraOscura);
        DrawCube({ 9.6f, 2.45f, z }, 1.2f, 0.5f, 0.7f, piedraOscura);
    }

    // Puerta del fondo.
    DrawCube({ 0.0f, 1.1f, -6.95f }, 2.4f, 2.2f, 0.2f, Color{ 40, 30, 24, 255 });
    DrawCube({ -1.4f, 1.3f, -6.95f }, 0.4f, 2.6f, 0.4f, piedraOscura);
    DrawCube({ 1.4f, 1.3f, -6.95f }, 0.4f, 2.6f, 0.4f, piedraOscura);
    DrawCube({ 0.0f, 2.6f, -6.95f }, 3.2f, 0.4f, 0.4f, piedraOscura);

    // Torres de esquina con techo conico.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            float altura = sz < 0 ? 4.4f : 2.4f;
            Vector3 base = { sx * 10.4f, 0.0f, sz * 8.2f };

            DrawCylinder(base, 1.5f, 1.5f, altura, 12, piedra);
            DrawCylinder({ base.x, altura, base.z }, 0.0f, 1.8f, 1.6f, 12, Color{ 150, 52, 48, 255 });
        }
    }

    // Estandartes sobre el muro del fondo.
    for (int k = 0; k < 5; k++)
    {
        float x = -7.2f + (float)k * 3.6f;
        float vaiven = std::sin(t * 2.0f + (float)k) * 0.12f;
        Color tela = k % 2 == 0 ? Color{ 200, 40, 50, 255 } : Color{ 240, 190, 50, 255 };

        DrawCube({ x, 3.6f, -7.2f }, 0.1f, 1.6f, 0.1f, madera);
        DrawCube({ x + vaiven, 3.35f, -7.0f }, 0.8f, 1.0f, 0.06f, tela);
    }

    // Antorchas.
    for (int k = 0; k < 6; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        float z = -4.5f + (float)(k / 2) * 4.5f;
        float llama = 0.17f + 0.04f * std::sin(t * 12.0f + (float)k * 2.0f);

        DrawCube({ lado * 8.8f, 1.0f, z }, 0.2f, 1.2f, 0.2f, madera);
        DrawSphere({ lado * 8.8f, 1.8f, z }, llama, Color{ 255, 170, 50, 255 });
        DrawSphere({ lado * 8.8f, 1.8f, z }, llama * 3.0f, Fade(Color{ 255, 150, 40, 255 }, 0.16f));
    }

    // Torre del homenaje: cuerpo, almenas y estandarte.
    DrawCube({ 0.0f, ALTO_TORRE_TESORERO * 0.5f, 0.0f }, MITAD_TORRE_TESORERO * 2.0f, ALTO_TORRE_TESORERO, MITAD_TORRE_TESORERO * 2.0f, Color{ 148, 142, 134, 255 });
    DrawCubeWires({ 0.0f, ALTO_TORRE_TESORERO * 0.5f, 0.0f }, MITAD_TORRE_TESORERO * 2.0f, ALTO_TORRE_TESORERO, MITAD_TORRE_TESORERO * 2.0f, Color{ 70, 66, 62, 255 });

    for (int k = 0; k < 8; k++)
    {
        float a = (float)k * 0.7853982f;
        DrawCube(
            { std::cos(a) * 1.15f, ALTO_TORRE_TESORERO + 0.2f, std::sin(a) * 1.15f },
            0.4f, 0.4f, 0.4f, piedraOscura
        );
    }

    DrawCube({ 0.0f, ALTO_TORRE_TESORERO + 0.9f, 0.0f }, 0.08f, 1.4f, 0.08f, madera);
    DrawCube({ 0.35f, ALTO_TORRE_TESORERO + 1.3f, 0.0f }, 0.7f, 0.45f, 0.05f, Color{ 255, 205, 60, 255 });
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

    // Postes de piedra en los extremos.
    for (int k = -1; k <= 1; k += 2)
    {
        float px = reja.x + (alLargoDeZ ? 0.0f : (float)k * (reja.mitadX + 0.15f));
        float pz = reja.z + (alLargoDeZ ? (float)k * (reja.mitadZ + 0.15f) : 0.0f);
        DrawCube({ px, 2.7f, pz }, 0.5f, 5.4f, 0.5f, Color{ 112, 108, 102, 255 });
    }

    // Barras y travesanos de la reja.
    int barras = 6;

    for (int b = 0; b < barras; b++)
    {
        float f = ((float)b + 0.5f) / (float)barras - 0.5f;
        float bx = reja.x + (alLargoDeZ ? 0.0f : f * largoX);
        float bz = reja.z + (alLargoDeZ ? f * largoZ : 0.0f);

        DrawCube({ bx, baseY + ALTO_REJA_TESORERO * 0.5f, bz }, 0.12f, ALTO_REJA_TESORERO, 0.12f, hierro);
    }

    DrawCube({ reja.x, baseY + ALTO_REJA_TESORERO * 0.8f, reja.z }, alLargoDeZ ? 0.14f : largoX, 0.12f, alLargoDeZ ? largoZ : 0.14f, hierro);
    DrawCube({ reja.x, baseY + ALTO_REJA_TESORERO * 0.25f, reja.z }, alLargoDeZ ? 0.14f : largoX, 0.12f, alLargoDeZ ? largoZ : 0.14f, hierro);

    // Aviso en el suelo: la reja esta por bajar.
    if (aviso)
    {
        float pulso = 0.5f + 0.5f * std::sin(t * 18.0f);
        DrawCube(
            { reja.x, 0.05f, reja.z },
            largoX + 0.6f, 0.05f, largoZ + 0.6f,
            Fade(Color{ 255, 50, 40, 255 }, 0.25f + 0.3f * pulso)
        );
    }
}


static void DibujarMonedaTesorero(const MonedaTesorero& moneda, float t)
{
    if (moneda.edad > VIDA_MONEDA_TESORERO - 2.0f && std::sin(t * 18.0f) < 0.0f)
    {
        return;
    }

    float angulo = t * 6.0f + moneda.x * 3.0f;
    Vector3 eje = { std::cos(angulo) * 0.04f, 0.0f, std::sin(angulo) * 0.04f };
    Vector3 centro = { moneda.x, moneda.y + 0.12f, moneda.z };

    DrawCircle3D({ moneda.x, 0.04f, moneda.z }, 0.2f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.4f));
    DrawCylinderEx(
        { centro.x - eje.x, centro.y, centro.z - eje.z },
        { centro.x + eje.x, centro.y, centro.z + eje.z },
        0.2f, 0.2f, 8, Color{ 255, 210, 50, 255 }
    );
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoTesoreroAcorralado::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteJugadoresTesorero(cantidadMaxima);

    ClearBackground(Color{ 112, 168, 214, 255 });
    BeginMode3D(camara);

    DibujarPatioTesorero(*this);

    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        DibujarRejaTesorero(rejas[k], tiempoAnimacion);
    }

    for (int m = 0; m < MAX_MONEDAS_TESORERO; m++)
    {
        if (monedas[m].activa)
        {
            DibujarMonedaTesorero(monedas[m], tiempoAnimacion);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorTesorero& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];
        Color colorEquipo = ColorEquipoTesorero(estado.equipo);

        DrawCircle3D({ jugador.posicion.x, 0.07f, jugador.posicion.z }, 0.72f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);
        DrawCircle3D({ jugador.posicion.x, 0.07f, jugador.posicion.z }, 0.62f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);

        // Parpadeo durante la inmunidad.
        bool visible = !(jugador.tiempoInmunidad > 0.0f && std::sin(tiempoAnimacion * 40.0f) < 0.0f);

        if (visible)
        {
            Participante participanteVisual = participantes[i];
            participanteVisual.conectado = true;
            DibujarJugadorCuboPrueba(jugador, participanteVisual);
        }

        if (estado.equipo == 0)
        {
            // Bolsa de oro sobre el tesorero.
            float flota = 1.35f + 0.1f * std::sin(tiempoAnimacion * 4.0f);
            DrawSphere({ jugador.posicion.x, jugador.posicion.y + flota, jugador.posicion.z }, 0.22f, Color{ 255, 210, 50, 255 });
            DrawSphere({ jugador.posicion.x, jugador.posicion.y + flota, jugador.posicion.z }, 0.4f, Fade(Color{ 255, 210, 50, 255 }, 0.25f));
        }

        if (mostrarDebug)
        {
            DrawCubeWires(jugador.posicion, jugador.tamano.x, jugador.tamano.y, jugador.tamano.z, LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA);

    EndMode3D();

    // HUD: monedas del tesorero.
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int panelX = ancho / 2 - 260;

    DrawRectangle(panelX, 10, 520, 84, Fade(BLACK, 0.78f));
    DrawRectangle(panelX, 10, 8, 84, ColorEquipoTesorero(0));
    DrawRectangle(panelX + 512, 10, 8, 84, ColorEquipoTesorero(1));

    const char* titulo = TextFormat("TESORERO  %d / %d", monedasTesorero, MONEDAS_INICIALES_TESORERO);
    DrawText(titulo, ancho / 2 - MeasureText(titulo, 26) / 2, 16, 26, ColorEquipoTesorero(0));

    // Barra con la marca del 50%.
    float proporcion = (float)monedasTesorero / (float)MONEDAS_INICIALES_TESORERO;
    if (proporcion < 0.0f) proporcion = 0.0f;
    if (proporcion > 1.0f) proporcion = 1.0f;

    DrawRectangle(panelX + 30, 50, 460, 14, Color{ 50, 50, 56, 255 });
    DrawRectangle(
        panelX + 30, 50, (int)(460.0f * proporcion), 14,
        monedasTesorero * 2 > MONEDAS_INICIALES_TESORERO ? Color{ 255, 205, 60, 255 } : Color{ 238, 55, 66, 255 }
    );
    DrawRectangle(panelX + 30 + 230 - 1, 46, 3, 22, RAYWHITE);

    bool aviso = false;

    for (int k = 0; k < MAX_REJAS_TESORERO; k++)
    {
        if (rejas[k].estado == REJA_TESORERO_AVISO)
        {
            aviso = true;
        }
    }

    const char* subtitulo = aviso
        ? "CUIDADO: BAJAN LAS REJAS"
        : TextFormat("CONSERVAR MAS DE 50  -  %.0f s  -  SUELTAS %d", tiempoRestante, ContarMonedasSueltasTesorero(*this));

    DrawText(
        subtitulo,
        ancho / 2 - MeasureText(subtitulo, 17) / 2,
        70,
        17,
        aviso ? ORANGE : LIGHTGRAY
    );

    // HUD: controles de cada jugador.
    int y = alto - 30;

    for (int i = limite - 1; i >= 0; i--)
    {
        const EstadoJugadorTesorero& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        const char* linea;

        if (EsControladoPorBotTesorero(participantes[i]))
        {
            linea = TextFormat(
                "J%d BOT  %s",
                participantes[i].numeroJugador,
                estado.equipo == 0 ? "TESORERO" : "TRIO"
            );
        }
        else if (estado.equipo == 0)
        {
            linea = TextFormat(
                "J%d TESORERO  HUYE Y USA LAS REJAS  SALTO [%s]  2o SALTO EN AIRE: GOLPE AL SUELO",
                participantes[i].numeroJugador,
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
        else
        {
            linea = TextFormat(
                "J%d TRIO  GOLPE [%s]  SALTO [%s]  2o SALTO: GOLPE AL SUELO  ROBADAS %d",
                participantes[i].numeroJugador,
                TextoBotonGolpeTesorero(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i]),
                estado.monedasRobadas
            );
        }

        DrawText(linea, 18, y, 17, ColorEquipoTesorero(estado.equipo));
        y -= 22;
    }

    if (fase == FASE_TESORERO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, ancho / 2 - MeasureText(texto, 90) / 2, alto / 2 - 50, 90, YELLOW);

        const char* pista = "TRIO: GOLPEA AL TESORERO PARA QUE SUELTE MONEDAS  -  TESORERO: CONSERVA MAS DE LA MITAD";
        DrawText(pista, ancho / 2 - MeasureText(pista, 18) / 2, alto / 2 + 60, 18, RAYWHITE);
    }
    else if (
        fase == FASE_TESORERO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        DrawRectangle(ancho / 2 - 330, alto / 2 - 90, 660, 180, Fade(BLACK, 0.92f));

        const char* ganador = ganaTesorero ? "GANA EL TESORERO" : "GANA EL TRIO";

        DrawText(
            ganador,
            ancho / 2 - MeasureText(ganador, 36) / 2,
            alto / 2 - 62,
            36,
            ganaTesorero ? ColorEquipoTesorero(0) : ColorEquipoTesorero(1)
        );

        const char* final = TextFormat("TESORERO CONSERVA %d DE %d", monedasTesorero, MONEDAS_INICIALES_TESORERO);
        DrawText(final, ancho / 2 - MeasureText(final, 26) / 2, alto / 2 - 10, 26, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), ancho / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2, alto / 2 + 44, 18, LIGHTGRAY);
    }
    else if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        const char* texto = "SE NECESITAN AL MENOS 2 JUGADORES";
        DrawText(texto, ancho / 2 - MeasureText(texto, 26) / 2, alto / 2, 26, RED);
    }
}


const ResultadoMinijuego& MinijuegoTesoreroAcorralado::ObtenerResultado() const
{
    return resultado;
}
