#include "Minigames/MinijuegoVetaCristal.h"

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
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_VETA = 3.0f;
static const float DURACION_PARTIDA_VETA = 45.0f;

// Arena: el eje X une las dos mitades y el riel corre a lo largo de Z.
static const float LIMITE_X_VETA = 10.6f;
static const float LIMITE_Z_VETA = 7.2f;
static const float MITAD_SUELO_X_VETA = 12.0f;
static const float MITAD_SUELO_Z_VETA = 8.0f;
static const float MITAD_ALTO_JUGADOR_VETA = 0.7f;
static const float MITAD_ANCHO_JUGADOR_VETA = 0.4f;

// Geodas: la base queda a una altura alcanzable con un salto.
static const float BASE_GEODA_VETA = 2.2f;
static const float MITAD_GEODA_VETA = 0.65f;
static const float MITAD_GEODA_GRANDE_VETA = 0.9f;
static const float RECARGA_GEODA_VETA = 5.0f;
static const float RECARGA_GEODA_GRANDE_VETA = 9.0f;
static const float DURACION_SACUDIDA_VETA = 0.35f;
static const float VENTANA_COORDINACION_VETA = 0.5f;
static const float RADIO_POUND_GEODA_VETA = 1.7f;
static const int PROBABILIDAD_DORADA_VETA = 12;
static const int VALOR_GEMA_AZUL_VETA = 1;
static const int VALOR_GEMA_DORADA_VETA = 3;
static const int VALOR_TESORO_VETA = 6;
static const int VALOR_TESORO_SOLITARIO_VETA = 4;

// Gemas.
static const float GRAVEDAD_GEMA_VETA = 22.0f;
static const float SUELO_GEMA_VETA = 0.28f;
static const float RADIO_RECOGER_VETA = 0.85f;
static const float ESPERA_RECOGER_VETA = 0.45f;
static const float VIDA_GEMA_VETA = 28.0f;

// Vagoneta.
static const float PRIMERA_VAGONETA_VETA = 7.0f;
static const float INTERVALO_VAGONETA_VETA = 12.0f;
static const float AVISO_VAGONETA_VETA = 1.5f;
static const float VELOCIDAD_VAGONETA_VETA = 10.0f;
static const float EXTREMO_VAGONETA_VETA = 9.5f;
static const float MITAD_ANCHO_VAGONETA_VETA = 0.95f;
static const float MITAD_LARGO_VAGONETA_VETA = 1.1f;
static const float DURACION_ATURDIMIENTO_VETA = 1.0f;
static const float DURACION_INMUNIDAD_VETA = 2.4f;
static const int GEMAS_PERDIDAS_VETA = 2;

static const float MULTIPLICADOR_SOLITARIO_VETA = 1.2f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarVeta(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AbsVeta(float valor)
{
    return valor < 0.0f ? -valor : valor;
}


static float AleatorioVeta(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}


// El equipo 0 ocupa la mitad izquierda (x negativo) y el 1 la derecha.
static float LadoEquipoVeta(int equipo)
{
    return equipo == 0 ? -1.0f : 1.0f;
}


static Color ObtenerColorEquipoVeta(int equipo)
{
    return equipo == 0
        ? Color{ 238, 55, 66, 255 }
        : Color{ 40, 159, 224, 255 };
}


static float Ruido01Veta(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Tramo del paquete GLB de la mina con las piezas jugables (vagoneta,
// geodas, gemas y marca), que son las ultimas del enum y van seguidas.
static_assert(MODELO_VETA_RIEL + 1 == MODELO_VETA_VAGONETA &&
    MODELO_VETA_MARCA + 1 == CANTIDAD_MODELOS_VETA_CRISTAL_3D,
    "Las piezas jugables de Veta deben ser el tramo final del paquete");

static PaqueteModelosEscenarioRetro3D PaquetePiezasJugablesVeta()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteVetaCristalRetro3D();
    paquete.recursos += MODELO_VETA_VAGONETA;
    paquete.definiciones += MODELO_VETA_VAGONETA;
    paquete.cantidad -= MODELO_VETA_VAGONETA;
    return paquete;
}


static int LimiteJugadoresVeta(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static bool EsControladoPorBotVeta(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static int PuntajeEquipoVeta(const MinijuegoVetaCristal& minijuego, int equipo)
{
    int total = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.estadosJugadores[i].equipo == equipo)
        {
            total += minijuego.estadosJugadores[i].gemas;
        }
    }

    return total;
}


// Hay vagoneta en marcha o a punto de salir: los bots despejan el riel.
static bool VagonetaAmenazaVeta(const MinijuegoVetaCristal& minijuego)
{
    return minijuego.vagonetaActiva ||
        minijuego.tiempoHastaVagoneta < AVISO_VAGONETA_VETA + 0.1f;
}


//==================================================
// DATOS LOGICOS DEL MAPA (colisiones y geodas)
//==================================================

static void ConstruirMapaVeta(MinijuegoVetaCristal& minijuego)
{
    minijuego.cantidadBloques = 0;

    // 0: suelo de roca de toda la mina.
    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_VETA,
        { 0.0f, -0.5f, 0.0f },
        { MITAD_SUELO_X_VETA * 2.0f, 1.0f, MITAD_SUELO_Z_VETA * 2.0f },
        Color{ 70, 60, 54, 255 }
    );

    // Cinco geodas por mitad: cuatro pequenas y una grande central.
    static const float POSICIONES_X[5] = { 3.8f, 3.8f, 7.6f, 7.6f, 6.0f };
    static const float POSICIONES_Z[5] = { -3.6f, 3.6f, -3.6f, 3.6f, 0.0f };

    for (int lado = 0; lado < 2; lado++)
    {
        float signo = lado == 0 ? -1.0f : 1.0f;

        for (int k = 0; k < 5; k++)
        {
            GeodaVeta& geoda = minijuego.geodas[lado * 5 + k];
            geoda = {};
            geoda.x = signo * POSICIONES_X[k];
            geoda.z = POSICIONES_Z[k];
            geoda.grande = k == 4;

            float mitad = geoda.grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;

            AgregarBloquePrueba(
                minijuego.bloques,
                minijuego.cantidadBloques,
                MAX_BLOQUES_VETA,
                { geoda.x, BASE_GEODA_VETA + mitad, geoda.z },
                { mitad * 2.0f, mitad * 2.0f, mitad * 2.0f },
                Color{ 110, 86, 140, 255 }
            );
        }
    }
}


//==================================================
// GEMAS
//==================================================

static void AgregarGemaVeta(
    MinijuegoVetaCristal& minijuego,
    float x, float y, float z,
    float velocidadX, float velocidadY, float velocidadZ,
    int valor
)
{
    int indice = -1;
    float mayorEdad = -1.0f;

    for (int i = 0; i < MAX_GEMAS_VETA; i++)
    {
        if (!minijuego.gemas[i].activa)
        {
            indice = i;
            break;
        }

        if (minijuego.gemas[i].edad > mayorEdad)
        {
            mayorEdad = minijuego.gemas[i].edad;
            indice = i;
        }
    }

    if (indice < 0)
    {
        return;
    }

    GemaVeta& gema = minijuego.gemas[indice];
    gema = {};
    gema.activa = true;
    gema.x = x;
    gema.y = y;
    gema.z = z;
    gema.velocidadX = velocidadX;
    gema.velocidadY = velocidadY;
    gema.velocidadZ = velocidadZ;
    gema.valor = valor;
    gema.tiempoSinRecoger = ESPERA_RECOGER_VETA;
}


static void LanzarGemasDeGeodaVeta(
    MinijuegoVetaCristal& minijuego,
    const GeodaVeta& geoda,
    int cantidad,
    int valorUnico
)
{
    for (int k = 0; k < cantidad; k++)
    {
        int valor = valorUnico;

        if (valor <= 0)
        {
            valor = GetRandomValue(1, 100) <= PROBABILIDAD_DORADA_VETA
                ? VALOR_GEMA_DORADA_VETA
                : VALOR_GEMA_AZUL_VETA;
        }

        float angulo = AleatorioVeta(0.0f, 6.2831853f);
        float velocidad = AleatorioVeta(1.8f, 4.2f);

        AgregarGemaVeta(
            minijuego,
            geoda.x,
            BASE_GEODA_VETA - 0.2f,
            geoda.z,
            std::cos(angulo) * velocidad,
            AleatorioVeta(2.5f, 5.0f),
            std::sin(angulo) * velocidad,
            valor
        );
    }
}


static void ActualizarGemasVeta(
    MinijuegoVetaCristal& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < MAX_GEMAS_VETA; i++)
    {
        GemaVeta& gema = minijuego.gemas[i];

        if (!gema.activa)
        {
            continue;
        }

        gema.edad += deltaTime;

        if (gema.edad >= VIDA_GEMA_VETA)
        {
            gema.activa = false;
            continue;
        }

        if (gema.tiempoSinRecoger > 0.0f)
        {
            gema.tiempoSinRecoger -= deltaTime;
        }

        gema.velocidadY -= GRAVEDAD_GEMA_VETA * deltaTime;
        gema.x += gema.velocidadX * deltaTime;
        gema.y += gema.velocidadY * deltaTime;
        gema.z += gema.velocidadZ * deltaTime;

        if (gema.y < SUELO_GEMA_VETA)
        {
            gema.y = SUELO_GEMA_VETA;

            if (gema.velocidadY < -1.5f)
            {
                gema.velocidadY = -gema.velocidadY * 0.45f;
            }
            else
            {
                gema.velocidadY = 0.0f;
            }

            float freno = 1.0f - 4.0f * deltaTime;
            if (freno < 0.0f) freno = 0.0f;
            gema.velocidadX *= freno;
            gema.velocidadZ *= freno;
        }

        // Las paredes devuelven la gema a la arena.
        if (gema.x > LIMITE_X_VETA)
        {
            gema.x = LIMITE_X_VETA;
            gema.velocidadX = -gema.velocidadX * 0.5f;
        }
        else if (gema.x < -LIMITE_X_VETA)
        {
            gema.x = -LIMITE_X_VETA;
            gema.velocidadX = -gema.velocidadX * 0.5f;
        }

        if (gema.z > LIMITE_Z_VETA)
        {
            gema.z = LIMITE_Z_VETA;
            gema.velocidadZ = -gema.velocidadZ * 0.5f;
        }
        else if (gema.z < -LIMITE_Z_VETA)
        {
            gema.z = -LIMITE_Z_VETA;
            gema.velocidadZ = -gema.velocidadZ * 0.5f;
        }
    }
}


static void RecogerGemasVeta(
    MinijuegoVetaCristal& minijuego,
    const JugadorPrueba jugadores[],
    int limite
)
{
    for (int g = 0; g < MAX_GEMAS_VETA; g++)
    {
        GemaVeta& gema = minijuego.gemas[g];

        if (!gema.activa || gema.tiempoSinRecoger > 0.0f || gema.y > 1.8f)
        {
            continue;
        }

        float alcance = RADIO_RECOGER_VETA + (gema.valor >= VALOR_TESORO_SOLITARIO_VETA ? 0.2f : 0.0f);
        int mejor = -1;
        float mejorDistancia = alcance;

        for (int i = 0; i < limite; i++)
        {
            EstadoJugadorVeta& estado = minijuego.estadosJugadores[i];

            if (estado.equipo < 0 || estado.tiempoAturdido > 0.0f)
            {
                continue;
            }

            float dx = jugadores[i].posicion.x - gema.x;
            float dz = jugadores[i].posicion.z - gema.z;
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

        EstadoJugadorVeta& estado = minijuego.estadosJugadores[mejor];
        estado.gemas += gema.valor;
        gema.activa = false;

        if (gema.valor >= VALOR_GEMA_DORADA_VETA)
        {
            minijuego.ultimaDoradaEquipo = estado.equipo;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RECOGER_NUCLEO_ESPECIAL);
        }
        else
        {
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RECOGER_OBJETO);
        }
    }
}


//==================================================
// GEODAS
//==================================================

static void GolpearGeodaVeta(
    MinijuegoVetaCristal& minijuego,
    int indiceGeoda,
    int indiceJugador
)
{
    GeodaVeta& geoda = minijuego.geodas[indiceGeoda];
    const EstadoJugadorVeta& golpeador = minijuego.estadosJugadores[indiceJugador];

    geoda.tiempoSacudida = DURACION_SACUDIDA_VETA;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);

    if (!geoda.cargada)
    {
        return;
    }

    if (!geoda.grande)
    {
        LanzarGemasDeGeodaVeta(minijuego, geoda, GetRandomValue(1, 3), 0);
        geoda.cargada = false;
        geoda.tiempoRecarga = RECARGA_GEODA_VETA;
        return;
    }

    // Geoda grande: un equipo de un solo jugador la abre con un golpe; si hay
    // dos companeros, deben golpearla casi a la vez.
    bool abrir = false;
    int valor = VALOR_TESORO_VETA;

    if (minijuego.cantidadJugadoresEquipo[golpeador.equipo] <= 1)
    {
        abrir = true;
        valor = VALOR_TESORO_SOLITARIO_VETA;
    }
    else if (
        geoda.ventanaGolpe > 0.0f &&
        geoda.golpeadorPrevio >= 0 &&
        geoda.golpeadorPrevio != indiceJugador &&
        minijuego.estadosJugadores[geoda.golpeadorPrevio].equipo == golpeador.equipo
    )
    {
        abrir = true;
    }

    if (!abrir)
    {
        geoda.ventanaGolpe = VENTANA_COORDINACION_VETA;
        geoda.golpeadorPrevio = indiceJugador;
        return;
    }

    LanzarGemasDeGeodaVeta(minijuego, geoda, 1, valor);
    geoda.cargada = false;
    geoda.tiempoRecarga = RECARGA_GEODA_GRANDE_VETA;
    geoda.ventanaGolpe = 0.0f;
    geoda.golpeadorPrevio = -1;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RECOGER_NUCLEO_ESPECIAL);
}


static void ActualizarGeodasVeta(
    MinijuegoVetaCristal& minijuego,
    float deltaTime
)
{
    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        GeodaVeta& geoda = minijuego.geodas[g];

        if (geoda.tiempoSacudida > 0.0f)
        {
            geoda.tiempoSacudida -= deltaTime;
        }

        if (geoda.ventanaGolpe > 0.0f)
        {
            geoda.ventanaGolpe -= deltaTime;

            if (geoda.ventanaGolpe <= 0.0f)
            {
                geoda.ventanaGolpe = 0.0f;
                geoda.golpeadorPrevio = -1;
            }
        }

        if (!geoda.cargada)
        {
            geoda.tiempoRecarga -= deltaTime;

            if (geoda.tiempoRecarga <= 0.0f)
            {
                geoda.tiempoRecarga = 0.0f;
                geoda.cargada = true;
            }
        }
    }
}


// Cabezazo: el jugador subia y su cabeza quedo contra la base de la geoda.
static void DetectarGolpesGeodaVeta(
    MinijuegoVetaCristal& minijuego,
    const JugadorPrueba& jugador,
    int indiceJugador,
    float velocidadYAntes
)
{
    if (velocidadYAntes <= 1.0f || jugador.enSuelo)
    {
        return;
    }

    float cabeza = jugador.posicion.y + MITAD_ALTO_JUGADOR_VETA;

    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        const GeodaVeta& geoda = minijuego.geodas[g];
        float mitad = geoda.grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;

        if (
            AbsVeta(jugador.posicion.x - geoda.x) < mitad + MITAD_ANCHO_JUGADOR_VETA &&
            AbsVeta(jugador.posicion.z - geoda.z) < mitad + MITAD_ANCHO_JUGADOR_VETA &&
            cabeza >= BASE_GEODA_VETA - 0.05f &&
            cabeza <= BASE_GEODA_VETA + 0.4f
        )
        {
            GolpearGeodaVeta(minijuego, g, indiceJugador);
            return;
        }
    }
}


// Ground pound cerca de una geoda: tambien la sacude.
static void DetectarPoundGeodaVeta(
    MinijuegoVetaCristal& minijuego,
    const JugadorPrueba& jugador,
    int indiceJugador
)
{
    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        const GeodaVeta& geoda = minijuego.geodas[g];
        float dx = jugador.posicion.x - geoda.x;
        float dz = jugador.posicion.z - geoda.z;

        if (dx * dx + dz * dz < RADIO_POUND_GEODA_VETA * RADIO_POUND_GEODA_VETA)
        {
            GolpearGeodaVeta(minijuego, g, indiceJugador);
            return;
        }
    }
}


//==================================================
// VAGONETA
//==================================================

static void ActualizarVagonetaVeta(
    MinijuegoVetaCristal& minijuego,
    float deltaTime
)
{
    if (!minijuego.vagonetaActiva)
    {
        minijuego.tiempoHastaVagoneta -= deltaTime;

        if (minijuego.tiempoHastaVagoneta <= AVISO_VAGONETA_VETA && !minijuego.avisoVagoneta)
        {
            minijuego.avisoVagoneta = true;
            minijuego.direccionVagoneta = -minijuego.direccionVagoneta;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);
        }

        if (minijuego.tiempoHastaVagoneta <= 0.0f)
        {
            minijuego.vagonetaActiva = true;
            minijuego.vagonetaZ = -minijuego.direccionVagoneta * EXTREMO_VAGONETA_VETA;
        }

        return;
    }

    minijuego.vagonetaZ += minijuego.direccionVagoneta * VELOCIDAD_VAGONETA_VETA * deltaTime;

    if (minijuego.vagonetaZ * minijuego.direccionVagoneta > EXTREMO_VAGONETA_VETA)
    {
        minijuego.vagonetaActiva = false;
        minijuego.avisoVagoneta = false;
        minijuego.tiempoHastaVagoneta = INTERVALO_VAGONETA_VETA;
    }
}


// La vagoneta aturde al jugador y le hace soltar gemas, que pueden caer en
// el lado rival.
static void ChocarVagonetaVeta(
    MinijuegoVetaCristal& minijuego,
    JugadorPrueba jugadores[],
    int limite
)
{
    if (!minijuego.vagonetaActiva)
    {
        return;
    }

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorVeta& estado = minijuego.estadosJugadores[i];
        JugadorPrueba& jugador = jugadores[i];

        if (
            estado.equipo < 0 ||
            estado.tiempoInmune > 0.0f ||
            jugador.posicion.y - MITAD_ALTO_JUGADOR_VETA > 1.0f
        )
        {
            continue;
        }

        if (
            AbsVeta(jugador.posicion.x) > MITAD_ANCHO_VAGONETA_VETA + MITAD_ANCHO_JUGADOR_VETA ||
            AbsVeta(jugador.posicion.z - minijuego.vagonetaZ) >
                MITAD_LARGO_VAGONETA_VETA + MITAD_ANCHO_JUGADOR_VETA
        )
        {
            continue;
        }

        estado.tiempoAturdido = DURACION_ATURDIMIENTO_VETA;
        estado.tiempoInmune = DURACION_INMUNIDAD_VETA;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GOLPE);

        float empuje = jugador.posicion.x >= 0.0f ? 1.0f : -1.0f;
        jugador.empuje.x += empuje * 7.0f;
        jugador.velocidad.y = 0.0f;

        int perdidas = estado.gemas < GEMAS_PERDIDAS_VETA ? estado.gemas : GEMAS_PERDIDAS_VETA;
        estado.gemas -= perdidas;

        for (int k = 0; k < perdidas; k++)
        {
            // Casi siempre salen hacia el lado rival: se transfieren.
            float direccion = GetRandomValue(1, 100) <= 65
                ? -LadoEquipoVeta(estado.equipo)
                : LadoEquipoVeta(estado.equipo);

            AgregarGemaVeta(
                minijuego,
                jugador.posicion.x,
                1.2f,
                jugador.posicion.z,
                direccion * AleatorioVeta(3.0f, 6.0f),
                AleatorioVeta(4.0f, 6.0f),
                AleatorioVeta(-2.0f, 2.0f),
                VALOR_GEMA_AZUL_VETA
            );
        }
    }
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarVeta(MinijuegoVetaCristal& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    int puntaje0 = PuntajeEquipoVeta(minijuego, 0);
    int puntaje1 = PuntajeEquipoVeta(minijuego, 1);

    minijuego.equipoGanador = -1;

    if (puntaje0 != puntaje1)
    {
        minijuego.equipoGanador = puntaje0 > puntaje1 ? 0 : 1;
    }
    else if (minijuego.ultimaDoradaEquipo >= 0)
    {
        minijuego.equipoGanador = minijuego.ultimaDoradaEquipo;
    }

    minijuego.empate = minijuego.equipoGanador < 0;

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.desenlace =
        minijuego.empate ? DESENLACE_EMPATE : DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = minijuego.estadosJugadores[i].equipo;
        resultadoJugador.numeroEquipo = equipo;
        resultadoJugador.puntuacionMinijuego =
            equipo == 0 ? puntaje0 : (equipo == 1 ? puntaje1 : 0);
        resultadoJugador.puntosObtenidos = 0;
        resultadoJugador.posicionFinal =
            minijuego.empate || equipo == minijuego.equipoGanador ? 1 : 2;
    }

    minijuego.fase = FASE_VETA_TERMINADO;
}


//==================================================
// IA DE BOTS
//==================================================

static InputMinijuegoParticipante CrearEntradaBotVeta(
    MinijuegoVetaCristal& minijuego,
    int indice,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    EstadoJugadorVeta& estado = minijuego.estadosJugadores[indice];
    const JugadorPrueba& jugador = jugadores[indice];
    float lado = LadoEquipoVeta(estado.equipo);
    bool amenaza = VagonetaAmenazaVeta(minijuego);

    estado.tiempoDecision -= deltaTime;
    estado.cooldownSaltoBot -= deltaTime;

    if (estado.tiempoDecision <= 0.0f)
    {
        estado.tiempoDecision = AleatorioVeta(0.18f, 0.38f);
        estado.objetivoGeoda = -1;
        estado.retardoSalto = -1.0f;
        estado.esperaGrande = 0.0f;

        if (amenaza && AbsVeta(jugador.posicion.x) < 3.2f)
        {
            // Sale del riel hacia su mitad.
            estado.objetivoX = lado * 5.0f;
            estado.objetivoZ = jugador.posicion.z;
        }
        else
        {
            // 1) Gemas sueltas cercanas (las valiosas pesan mas).
            int mejorGema = -1;
            float mejorPuntaje = 1.0e9f;

            for (int g = 0; g < MAX_GEMAS_VETA; g++)
            {
                const GemaVeta& gema = minijuego.gemas[g];

                if (!gema.activa || gema.y > 2.0f)
                {
                    continue;
                }

                if (amenaza && AbsVeta(gema.x) < 2.6f)
                {
                    continue;
                }

                if (GetRandomValue(1, 100) <= 8)
                {
                    continue;
                }

                float dx = gema.x - jugador.posicion.x;
                float dz = gema.z - jugador.posicion.z;
                float distancia2 = dx * dx + dz * dz;

                if (distancia2 > 49.0f)
                {
                    continue;
                }

                float puntaje = distancia2 / (float)gema.valor;

                if (puntaje < mejorPuntaje)
                {
                    mejorPuntaje = puntaje;
                    mejorGema = g;
                }
            }

            if (mejorGema >= 0)
            {
                estado.objetivoX = minijuego.gemas[mejorGema].x;
                estado.objetivoZ = minijuego.gemas[mejorGema].z;
            }
            else
            {
                // 2) Geoda cargada mas cercana de su mitad.
                bool hayCompanero = false;

                for (int j = 0; j < limite; j++)
                {
                    if (
                        j != indice &&
                        minijuego.estadosJugadores[j].equipo == estado.equipo
                    )
                    {
                        hayCompanero = true;
                    }
                }

                int mejorGeoda = -1;
                float mejorDistancia2 = 1.0e9f;

                for (int g = 0; g < MAX_GEODAS_VETA; g++)
                {
                    const GeodaVeta& geoda = minijuego.geodas[g];

                    if (!geoda.cargada || geoda.x * lado <= 0.0f)
                    {
                        continue;
                    }

                    if (geoda.grande && hayCompanero && GetRandomValue(1, 100) > 35)
                    {
                        continue;
                    }

                    bool tomada = false;

                    for (int j = 0; j < limite; j++)
                    {
                        if (
                            j != indice &&
                            minijuego.estadosJugadores[j].equipo == estado.equipo &&
                            minijuego.estadosJugadores[j].objetivoGeoda == g &&
                            !geoda.grande
                        )
                        {
                            tomada = true;
                        }
                    }

                    if (tomada)
                    {
                        continue;
                    }

                    float dx = geoda.x - jugador.posicion.x;
                    float dz = geoda.z - jugador.posicion.z;
                    float distancia2 = dx * dx + dz * dz;

                    if (distancia2 < mejorDistancia2)
                    {
                        mejorDistancia2 = distancia2;
                        mejorGeoda = g;
                    }
                }

                if (mejorGeoda >= 0)
                {
                    estado.objetivoGeoda = mejorGeoda;
                    estado.objetivoX = minijuego.geodas[mejorGeoda].x + AleatorioVeta(-0.12f, 0.12f);
                    estado.objetivoZ = minijuego.geodas[mejorGeoda].z + AleatorioVeta(-0.12f, 0.12f);
                }
                else
                {
                    estado.objetivoX = lado * 5.0f;
                    estado.objetivoZ = 0.0f;
                }
            }
        }
    }

    InputMinijuegoParticipante entrada{};

    float dx = estado.objetivoX - jugador.posicion.x;
    float dz = estado.objetivoZ - jugador.posicion.z;

    if (dx > 0.15f) entrada.derecha = true;
    if (dx < -0.15f) entrada.izquierda = true;
    if (dz > 0.15f) entrada.atras = true;
    if (dz < -0.15f) entrada.adelante = true;

    if (estado.objetivoGeoda < 0)
    {
        return entrada;
    }

    const GeodaVeta& geoda = minijuego.geodas[estado.objetivoGeoda];

    if (!geoda.cargada)
    {
        estado.objetivoGeoda = -1;
        estado.tiempoDecision = 0.0f;
        return entrada;
    }

    float gx = geoda.x - jugador.posicion.x;
    float gz = geoda.z - jugador.posicion.z;
    float distancia = std::sqrt(gx * gx + gz * gz);

    if (
        distancia > 0.4f ||
        !jugador.enSuelo ||
        jugador.preparandoGolpeSuelo ||
        estado.cooldownSaltoBot > 0.0f
    )
    {
        return entrada;
    }

    bool saltar = false;

    if (!geoda.grande || minijuego.cantidadJugadoresEquipo[estado.equipo] <= 1)
    {
        saltar = true;
    }
    else
    {
        // Geoda grande: espera a que el companero este debajo y salta con
        // un pequeno desfase aleatorio.
        bool companeroDebajo = false;

        for (int j = 0; j < limite; j++)
        {
            if (j == indice || minijuego.estadosJugadores[j].equipo != estado.equipo)
            {
                continue;
            }

            float cx = geoda.x - jugadores[j].posicion.x;
            float cz = geoda.z - jugadores[j].posicion.z;

            if (cx * cx + cz * cz < 1.3f * 1.3f)
            {
                companeroDebajo = true;
            }
        }

        estado.esperaGrande += deltaTime;

        if (companeroDebajo && estado.retardoSalto < 0.0f)
        {
            estado.retardoSalto = AleatorioVeta(0.0f, 0.15f);
        }
        else if (estado.esperaGrande > 2.2f && estado.retardoSalto < 0.0f)
        {
            estado.retardoSalto = 0.0f;
        }

        if (estado.retardoSalto >= 0.0f)
        {
            estado.retardoSalto -= deltaTime;

            if (estado.retardoSalto <= 0.0f)
            {
                saltar = true;
            }
        }
    }

    if (saltar)
    {
        entrada.saltar = true;
        estado.retardoSalto = -1.0f;
        estado.esperaGrande = 0.0f;
        estado.cooldownSaltoBot = 0.8f;
    }

    return entrada;
}


//==================================================
// ACTUALIZACION DE JUGADORES
//==================================================

static void ActualizarJugadoresVeta(
    MinijuegoVetaCristal& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[]
)
{
    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorVeta& estado = minijuego.estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];

        if (estado.tiempoAturdido > 0.0f) estado.tiempoAturdido -= deltaTime;
        if (estado.tiempoInmune > 0.0f) estado.tiempoInmune -= deltaTime;

        InputMinijuegoParticipante entrada{};

        if (EsControladoPorBotVeta(participantes[i]))
        {
            entrada = CrearEntradaBotVeta(minijuego, i, jugadores, limite, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        entrada.golpear = false;

        if (estado.tiempoAturdido > 0.0f)
        {
            entrada = InputMinijuegoParticipante{};
        }

        // El equipo en inferioridad (2 vs 1) se mueve algo mas rapido.
        int otro = estado.equipo == 0 ? 1 : 0;
        float velocidadBase = jugador.velocidadMovimiento;

        if (
            minijuego.cantidadJugadoresEquipo[estado.equipo] <
            minijuego.cantidadJugadoresEquipo[otro]
        )
        {
            jugador.velocidadMovimiento = velocidadBase * MULTIPLICADOR_SOLITARIO_VETA;
        }

        float velocidadYAntes = jugador.velocidad.y;

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

        // Red de seguridad: nunca sale de la mina.
        jugador.posicion.x = LimitarVeta(jugador.posicion.x, -LIMITE_X_VETA, LIMITE_X_VETA);
        jugador.posicion.z = LimitarVeta(jugador.posicion.z, -LIMITE_Z_VETA, LIMITE_Z_VETA);

        if (jugador.cayendo || jugador.posicion.y < -3.0f)
        {
            ReiniciarJugadorPrueba(jugador);
            continue;
        }

        if (estado.tiempoAturdido <= 0.0f)
        {
            DetectarGolpesGeodaVeta(minijuego, jugador, i, velocidadYAntes);
        }

        if (jugador.impactoGolpeSuelo)
        {
            jugador.impactoGolpeSuelo = false;
            DetectarPoundGeodaVeta(minijuego, jugador, i);
        }
    }
}


//==================================================
// INICIALIZACION
//==================================================

void MinijuegoVetaCristal::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_GEMAS_VETA; i++)
    {
        gemas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_TIERRA; i++)
    {
        particulas[i] = {};
    }

    ConstruirMapaVeta(*this);

    // Camara cenital diagonal fija que encuadra ambas mitades.
    camara.position = { 0.0f, 18.0f, 14.5f };
    camara.target = { 0.0f, 0.8f, 0.4f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_VETA_PREPARACION;
    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;
    ultimaDoradaEquipo = -1;
    equipoGanador = -1;
    empate = false;
    partidaValida = false;

    vagonetaActiva = false;
    avisoVagoneta = false;
    vagonetaZ = 0.0f;
    direccionVagoneta = 1.0f;
    tiempoHastaVagoneta = PRIMERA_VAGONETA_VETA;

    tiempoPreparacion = DURACION_PREPARACION_VETA;
    tiempoRestante = DURACION_PARTIDA_VETA;
    tiempoAnimacion = 0.0f;

    // Estado visual: nada de esto toca la partida.
    for (int i = 0; i < MAX_CHISPAS_VETA; i++) chispas[i] = {};
    for (int i = 0; i < MAX_AVISOS_VETA; i++) avisos[i] = {};
    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        destelloGeoda[g] = 0.0f;
        grietaGeoda[g] = 0.0f;
    }
    for (int i = 0; i < MAX_PARTICIPANTES; i++) destelloJugador[i] = 0.0f;
    marcadorVisual[0] = marcadorVisual[1] = 0.0f;
    semillaVisual = 0x5EED7A1u;
    tiempoAnuncioInicio = -1.0f;
    tiempoFin = -1.0f;
    vagonetaVisible = false;
    vagonetaVisualZ = 0.0f;
    vagonetaVisualDireccion = 1.0f;
    giroRuedas = 0.0f;
}


void MinijuegoVetaCristal::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    // ZonaPruebas inicializa todos los minijuegos al arrancar. Solicitar el
    // paquete solo al entrar a esta mina; reiniciar reutiliza las mallas.
    // Solo las piezas jugables: el decorado se dibuja con primitivas.
    CargarPaqueteModelosEscenarioRetro3D(PaquetePiezasJugablesVeta());
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    int limite = LimiteJugadoresVeta(cantidadMaxima);
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
        fase = FASE_VETA_TERMINADO;
        return;
    }

    partidaValida = true;

    for (int i = cantidad - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = indices[i];
        indices[i] = indices[otro];
        indices[otro] = temporal;
    }

    // Con 4: 2 vs 2. Con 3: 2 vs 1. Con 2: 1 vs 1.
    int enEquipo0 = cantidad >= 3 ? 2 : 1;
    int ordenEquipo[2] = { 0, 0 };
    int ordenJugador[MAX_PARTICIPANTES]{};

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        int equipo = k < enEquipo0 ? 0 : 1;

        estadosJugadores[indice].equipo = equipo;
        ordenJugador[indice] = ordenEquipo[equipo]++;
        cantidadJugadoresEquipo[equipo]++;
        resultado.participantes[indice].numeroEquipo = equipo;
    }

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        EstadoJugadorVeta& estado = estadosJugadores[indice];
        float lado = LadoEquipoVeta(estado.equipo);

        float z = cantidadJugadoresEquipo[estado.equipo] <= 1
            ? 0.0f
            : (ordenJugador[indice] == 0 ? -1.8f : 1.8f);

        Vector3 spawn = { lado * 3.2f, MITAD_ALTO_JUGADOR_VETA, z };

        jugadores[indice].posicionSpawn = spawn;
        ReiniciarJugadorPrueba(jugadores[indice]);
        jugadores[indice].posicion = spawn;
        jugadores[indice].direccionMirada = { -lado, 0.0f, 0.0f };
        jugadores[indice].enSuelo = true;
        jugadores[indice].cayendo = false;

        estado.objetivoX = spawn.x;
        estado.objetivoZ = z;
        estado.tiempoDecision = AleatorioVeta(0.0f, 0.3f);
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoVetaCristal::ActualizarPartida(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_VETA_TERMINADO ||
        resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int limite = LimiteJugadoresVeta(cantidadMaxima);

    if (fase == FASE_VETA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_VETA_JUGANDO;
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    ActualizarVagonetaVeta(*this, deltaTime);
    ActualizarJugadoresVeta(*this, deltaTime, jugadores, limite, participantes);
    // Un humano desconectado lo controla la IA: debe seguir colisionando.
    Participante efectivos[MAX_PARTICIPANTES];
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectivos[i] = participantes[i];
        if (efectivos[i].activo) efectivos[i].conectado = true;
    }
    ResolverColisionesPelotas(jugadores, efectivos, limite);
    ActualizarGeodasVeta(*this, deltaTime);
    ChocarVagonetaVeta(*this, jugadores, limite);
    ActualizarGemasVeta(*this, deltaTime);
    RecogerGemasVeta(*this, jugadores, limite);
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA, deltaTime);

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarVeta(*this);
    }
}


//==================================================
// VISUAL: PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Mina de cristales de noche: galeria de roca violacea entibada con vigas,
// lamparas de minero ambar y vetas de cristal azul y violeta. Detras de la
// arena, el borde de la galeria cae a un abismo de cristales gigantes que
// cruza el puente de caballetes de la vagoneta. Todo se dibuja con
// triangulos propios (rlgl): ningun limite ni colision sale del dibujo.

static const Color COLOR_NIEBLA_VETA = { 46, 36, 92, 255 };
static const Color COLOR_ROCA_VETA = { 92, 82, 108, 255 };
static const Color COLOR_ROCA_OSCURA_VETA = { 62, 54, 80, 255 };
static const Color COLOR_LOSA_VETA = { 124, 116, 132, 255 };
static const Color COLOR_JUNTA_VETA = { 54, 48, 66, 255 };
static const Color COLOR_GRAVA_VETA = { 98, 92, 104, 255 };
static const Color COLOR_MADERA_VETA = { 136, 92, 56, 255 };
static const Color COLOR_MADERA_OSCURA_VETA = { 96, 64, 40, 255 };
static const Color COLOR_HIERRO_VETA = { 80, 84, 98, 255 };
static const Color COLOR_ACERO_VETA = { 176, 182, 198, 255 };
static const Color COLOR_CRISTAL_AZUL_VETA = { 86, 204, 255, 255 };
static const Color COLOR_CRISTAL_VIOLETA_VETA = { 184, 116, 255, 255 };
static const Color COLOR_LAMPARA_VETA = { 255, 196, 112, 255 };
static const Color COLOR_ORO_VETA = { 255, 202, 76, 255 };

// Ojo de referencia de la niebla: la camara de juego.
static const Vector3 OJO_NIEBLA_VETA = { 0.0f, 18.0f, 14.5f };

// Plano que mira a la camara (para halos y chispas).
static const Vector3 FRENTE_U_VETA = { 1.0f, 0.0f, 0.0f };
static const Vector3 FRENTE_V_VETA = { 0.0f, 0.634f, -0.773f };

static const float DURACION_AVISO_VETA = 1.2f;
static const float DURACION_ANUNCIO_VETA = 1.6f;


static float Limitar01Veta(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static unsigned char CanalVeta(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoVeta(Color color, float factor)
{
    return Color{
        CanalVeta((float)color.r * factor),
        CanalVeta((float)color.g * factor),
        CanalVeta((float)color.b * factor),
        color.a
    };
}


static Color MezclaVeta(Color a, Color b, float t)
{
    t = Limitar01Veta(t);

    return Color{
        CanalVeta((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalVeta((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalVeta((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalVeta((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaVeta(Color color, float alfa)
{
    color.a = CanalVeta(255.0f * alfa);
    return color;
}


// Niebla indigo: lo lejano y lo hondo del abismo se funden con el aire.
static float CantidadNieblaVeta(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_VETA.x;
    float dy = p.y - OJO_NIEBLA_VETA.y;
    float dz = p.z - OJO_NIEBLA_VETA.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 30.0f) / 46.0f;
    if (p.y < 0.0f) n += Limitar01Veta(-p.y / 16.0f) * 0.3f;
    return n < 0.0f ? 0.0f : (n > 0.9f ? 0.9f : n);
}


static Color TinteVeta(Color color, Vector3 p, float emisivo = 0.0f)
{
    Color niebla = COLOR_NIEBLA_VETA;
    niebla.a = color.a;
    return MezclaVeta(color, niebla, CantidadNieblaVeta(p) * (1.0f - 0.6f * emisivo));
}


static void VerticeVeta(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadVeta(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeVeta(a, color);
    VerticeVeta(b, color);
    VerticeVeta(c, color);
    VerticeVeta(d, color);
}


static void QuadNieblaVeta(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeVeta(a, TinteVeta(color, a));
    VerticeVeta(b, TinteVeta(color, b));
    VerticeVeta(c, TinteVeta(color, c));
    VerticeVeta(d, TinteVeta(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.55). Respeta la matriz actual de rlgl.
static void CuboVeta(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadVeta({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, color);
    QuadVeta({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoVeta(color, 0.78f));
    QuadVeta({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoVeta(color, 0.55f));
    QuadVeta({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoVeta(color, 0.62f));
    QuadVeta({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoVeta(color, 0.70f));
    QuadVeta({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoVeta(color, 0.45f));
    rlEnd();
}


static void CuboNieblaVeta(Vector3 c, Vector3 t, Color color)
{
    CuboVeta(c, t, TinteVeta(color, c));
}


// Roca de la galeria: se oscurece con la altura hasta perderse en la
// penumbra del techo (las lamparas solo alumbran abajo).
static Color PenumbraVeta(Color color, float y)
{
    float n = Limitar01Veta((y - 1.2f) / 7.0f);
    return MezclaVeta(color, Color{ 26, 20, 46, color.a }, n * 0.78f);
}


static void CuboRocaVeta(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;
    Color arriba = PenumbraVeta(color, y1);
    Color abajo = PenumbraVeta(color, y0);

    rlBegin(RL_QUADS);
    QuadVeta({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, TonoVeta(arriba, 0.9f));

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
        Color b = TonoVeta(abajo, caras[k]);
        Color a = TonoVeta(arriba, caras[k]);
        VerticeVeta(q[k][0], b);
        VerticeVeta(q[k][1], b);
        VerticeVeta(q[k][2], a);
        VerticeVeta(q[k][3], a);
    }
    rlEnd();
}


// Caja girada alrededor de Y (grados).
static void CuboGiradoVeta(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboVeta({ 0.0f, 0.0f, 0.0f }, t, color);
    rlPopMatrix();
}


static Vector3 SumaVeta(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaVeta(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaVeta(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }


static Vector3 NormalizarVeta(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 0.0001f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static Vector3 CruzVeta(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}


// Luz de la escena: de arriba, algo de frente y de la izquierda.
static float LuzNormalVeta(Vector3 n)
{
    const float LX = -0.30f, LY = 0.82f, LZ = 0.48f;
    float d = n.x * LX + n.y * LY + n.z * LZ;
    return 0.56f + 0.46f * Limitar01Veta(d);
}


// Prisma (o tronco de cono) entre dos puntos cualesquiera. emisivo acerca
// todas las caras a la luz plena (cristales, vidrios).
static void PrismaEjeVeta(
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
    Vector3 eje = NormalizarVeta(RestaVeta(b, a));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarVeta(CruzVeta(eje, apoyo));
    Vector3 v = CruzVeta(eje, u);
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaVeta(EscalaVeta(u, std::cos(a0)), EscalaVeta(v, std::sin(a0)));
        Vector3 d1 = SumaVeta(EscalaVeta(u, std::cos(a1)), EscalaVeta(v, std::sin(a1)));
        Vector3 dm = SumaVeta(EscalaVeta(u, std::cos(am)), EscalaVeta(v, std::sin(am)));

        float f = LuzNormalVeta(dm);
        f = f + (1.0f - f) * emisivo;
        if (k & 1) f *= 0.94f;
        Color cara = TonoVeta(color, f);

        Vector3 p0 = SumaVeta(a, EscalaVeta(d0, r0));
        Vector3 p1 = SumaVeta(a, EscalaVeta(d1, r0));
        Vector3 p2 = SumaVeta(b, EscalaVeta(d1, r1));
        Vector3 p3 = SumaVeta(b, EscalaVeta(d0, r1));

        if (niebla) QuadNieblaVeta(p0, p1, p2, p3, cara);
        else QuadVeta(p0, p1, p2, p3, cara);

        if (r1 > 0.001f)
        {
            Color tapa = TonoVeta(color, LuzNormalVeta(eje) + (1.0f - LuzNormalVeta(eje)) * emisivo);
            if (niebla) tapa = TinteVeta(tapa, b);
            QuadVeta(b, p2, p3, b, tapa);
        }
    }

    rlEnd();
}


// Cristal: prisma hexagonal con punta.
static void CristalVeta(Vector3 base, Vector3 direccion, float largo, float radio, Color color, float giro = 0.0f, bool niebla = false)
{
    Vector3 d = NormalizarVeta(direccion);
    Vector3 medio = SumaVeta(base, EscalaVeta(d, largo * 0.72f));
    Vector3 punta = SumaVeta(base, EscalaVeta(d, largo));

    PrismaEjeVeta(base, medio, radio * 0.85f, radio, 6, color, giro, 0.55f, niebla);
    PrismaEjeVeta(medio, punta, radio, 0.0f, 6, MezclaVeta(color, WHITE, 0.18f), giro, 0.6f, niebla);
}


// Racimo de cristales que nace de un punto de la roca.
static void RacimoCristalVeta(Vector3 base, Vector3 normal, float escala, Color color, int semilla, bool niebla = false)
{
    Vector3 n = NormalizarVeta(normal);
    Vector3 apoyo = std::fabs(n.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarVeta(CruzVeta(n, apoyo));
    Vector3 v = CruzVeta(n, u);

    for (int k = 0; k < 5; k++)
    {
        float a = (float)k * 1.3f + Ruido01Veta(k, semilla) * 2.0f;
        float abre = k == 0 ? 0.0f : 0.35f + 0.4f * Ruido01Veta(k, semilla + 1);
        Vector3 d = SumaVeta(n, SumaVeta(EscalaVeta(u, std::cos(a) * abre), EscalaVeta(v, std::sin(a) * abre)));
        float largo = escala * (k == 0 ? 1.0f : 0.45f + 0.4f * Ruido01Veta(k, semilla + 2));
        float radio = escala * (k == 0 ? 0.2f : 0.13f);
        Vector3 pie = SumaVeta(base, SumaVeta(EscalaVeta(u, std::cos(a) * abre * escala * 0.25f), EscalaVeta(v, std::sin(a) * abre * escala * 0.25f)));
        Color c = k % 3 == 2 ? MezclaVeta(color, WHITE, 0.25f) : color;
        CristalVeta(pie, d, largo, radio, c, a, niebla);
    }
}


// Roca facetada: esfera de pocos lados con radios irregulares.
static float RadioRocaVeta(int anillo, int lado, int lados, int anillos, int semilla, float irregular)
{
    if (anillo == 0 || anillo == anillos) return 1.0f - irregular * 0.3f;
    return 1.0f - irregular + 2.0f * irregular * Ruido01Veta(anillo * 31 + (lado % lados), semilla);
}


static void RocaVeta(Vector3 c, Vector3 r, int semilla, Color color, float irregular = 0.12f, bool niebla = false)
{
    const int ANILLOS = 5;
    const int LADOS = 8;

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
                float f = RadioRocaVeta(aa[q], kk[q], LADOS, ANILLOS, semilla, irregular);
                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = { c.x + n.x * r.x * f, c.y + n.y * r.y * f, c.z + n.z * r.z * f };
                Color cv = TonoVeta(color, LuzNormalVeta(n) * (0.94f + 0.12f * Ruido01Veta(aa[q] * 7 + kk[q], semilla + 5)));
                VerticeVeta(p, niebla ? TinteVeta(cv, p) : cv);
            }
        }
    }

    rlEnd();
}


// Disco o corona horizontal.
static void AnilloVeta(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadVeta(
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
static void ResplandorVeta(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloVeta(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorVeta(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


static void ResplandorFrenteVeta(Vector3 c, float radio, Color color)
{
    ResplandorVeta(c, FRENTE_U_VETA, FRENTE_V_VETA, radio, color);
}


// Cono de luz (aditivo): del foco hacia el charco, se apaga en la base.
static void ConoLuzVeta(Vector3 foco, Vector3 baseCentro, float radio, Color color)
{
    const int LADOS = 16;
    Vector3 eje = NormalizarVeta(RestaVeta(baseCentro, foco));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarVeta(CruzVeta(eje, apoyo));
    Vector3 v = CruzVeta(eje, u);

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaVeta(baseCentro, SumaVeta(EscalaVeta(u, std::cos(a0) * radio), EscalaVeta(v, std::sin(a0) * radio)));
        Vector3 p1 = SumaVeta(baseCentro, SumaVeta(EscalaVeta(u, std::cos(a1) * radio), EscalaVeta(v, std::sin(a1) * radio)));

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
static void TrazoSueloVeta(float xa, float za, float xb, float zb, float y, float ancho, Color color)
{
    float dx = xb - xa, dz = zb - za;
    float l = std::sqrt(dx * dx + dz * dz);
    if (l < 0.0001f) return;
    float nx = -dz / l * ancho * 0.5f, nz = dx / l * ancho * 0.5f;

    rlBegin(RL_QUADS);
    QuadVeta({ xa + nx, y, za + nz }, { xb + nx, y, zb + nz }, { xb - nx, y, zb - nz }, { xa - nx, y, za - nz }, color);
    rlEnd();
}


// Cadena: eslabones alternados entre dos puntos.
static void CadenaVeta(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 d = RestaVeta(b, a);
    float l = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    int eslabones = (int)(l / (grosor * 3.2f));
    if (eslabones < 2) eslabones = 2;

    for (int k = 0; k < eslabones; k++)
    {
        Vector3 p0 = SumaVeta(a, EscalaVeta(d, (float)k / (float)eslabones));
        Vector3 p1 = SumaVeta(a, EscalaVeta(d, ((float)k + 0.85f) / (float)eslabones));
        PrismaEjeVeta(p0, p1, grosor * (k & 1 ? 0.6f : 1.0f), grosor * (k & 1 ? 0.6f : 1.0f), 4, color, k & 1 ? 0.0f : 0.785f);
    }
}


static Color ColorEquipoVisualVeta(int equipo)
{
    return ObtenerColorEquipoVeta(equipo < 0 ? 0 : equipo);
}


static Color ColorCristalGeodaVeta(bool grande)
{
    // Los mismos tonos que la marca de suelo del GLB.
    return grande ? Color{ 255, 200, 70, 255 } : Color{ 90, 220, 255, 255 };
}


static Color ColorGemaVeta(int valor)
{
    if (valor >= VALOR_TESORO_SOLITARIO_VETA) return Color{ 214, 120, 255, 255 };
    if (valor >= VALOR_GEMA_DORADA_VETA) return Color{ 255, 205, 60, 255 };
    return Color{ 80, 178, 255, 255 };
}


//==================================================
// VISUAL: CHISPAS, AVISOS Y DETECCION DE EVENTOS
//==================================================

static float AzarVisualVeta(MinijuegoVetaCristal& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadChispasVeta(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return n < 1 ? 1 : n;
}


static void EmitirChispaVeta(
    MinijuegoVetaCristal& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool brilla
)
{
    int indice = 0;
    float menor = 1000.0f;

    for (int i = 0; i < MAX_CHISPAS_VETA; i++)
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

    ChispaVeta& c = m.chispas[indice];
    c.activa = true;
    c.posicion = posicion;
    c.velocidad = velocidad;
    c.color = color;
    c.vida = vida;
    c.vidaMaxima = vida;
    c.tamano = tamano;
    c.gravedad = gravedad;
    c.giro = AzarVisualVeta(m, 0.0f, 360.0f);
    c.brilla = brilla;
}


static void AgregarAvisoVeta(MinijuegoVetaCristal& m, Vector3 posicion, int tipo, int valor, int equipo)
{
    int indice = 0;
    float menor = 1000.0f;

    for (int i = 0; i < MAX_AVISOS_VETA; i++)
    {
        if (!m.avisos[i].activo)
        {
            indice = i;
            break;
        }

        if (m.avisos[i].vida < menor)
        {
            menor = m.avisos[i].vida;
            indice = i;
        }
    }

    AvisoVeta& a = m.avisos[indice];
    a.activo = true;
    a.posicion = posicion;
    a.vida = tipo == 1 ? DURACION_ANUNCIO_VETA : DURACION_AVISO_VETA;
    a.tipo = tipo;
    a.valor = valor;
    a.equipo = equipo;
}


// Cabezazo: esquirlas de roca; si la geoda se abre, lluvia de cristal.
static void EfectoGolpeGeodaVeta(MinijuegoVetaCristal& m, int indice, bool abierta)
{
    const GeodaVeta& geoda = m.geodas[indice];
    float mitad = geoda.grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;
    Color cristal = ColorCristalGeodaVeta(geoda.grande);
    Color roca = geoda.grande ? Color{ 140, 118, 150, 255 } : Color{ 120, 108, 138, 255 };

    m.destelloGeoda[indice] = 1.0f;
    m.grietaGeoda[indice] = abierta ? 1.0f : Limitar01Veta(m.grietaGeoda[indice] + 0.45f);

    int cantidad = CantidadChispasVeta(abierta ? 20 : 7);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualVeta(m, 0.0f, 6.2832f);
        float v = AzarVisualVeta(m, 1.4f, 4.4f);
        Vector3 p = { geoda.x + std::cos(a) * mitad * 0.7f, BASE_GEODA_VETA + AzarVisualVeta(m, 0.0f, mitad), geoda.z + std::sin(a) * mitad * 0.7f };
        Vector3 vel = { std::cos(a) * v, AzarVisualVeta(m, -1.0f, 3.5f), std::sin(a) * v };
        bool brillo = abierta && (k % 3 == 0);
        Color c = abierta && (k % 2 == 0) ? cristal : roca;
        EmitirChispaVeta(m, p, vel, c, AzarVisualVeta(m, 0.5f, 0.95f), AzarVisualVeta(m, 0.07f, 0.16f), 14.0f, brillo);
    }

    if (abierta && !geoda.grande)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
    }
}


static void EfectoTesoroVeta(MinijuegoVetaCristal& m, int indice, int equipo)
{
    const GeodaVeta& geoda = m.geodas[indice];
    int cantidad = CantidadChispasVeta(26);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarVisualVeta(m, 0.0f, 6.2832f);
        float v = AzarVisualVeta(m, 1.0f, 3.2f);
        Vector3 p = { geoda.x, BASE_GEODA_VETA + MITAD_GEODA_GRANDE_VETA, geoda.z };
        Vector3 vel = { std::cos(a) * v, AzarVisualVeta(m, 1.5f, 5.0f), std::sin(a) * v };
        Color c = k % 2 == 0 ? COLOR_ORO_VETA : COLOR_CRISTAL_VIOLETA_VETA;
        EmitirChispaVeta(m, p, vel, c, AzarVisualVeta(m, 0.7f, 1.3f), AzarVisualVeta(m, 0.1f, 0.2f), 5.0f, true);
    }

    AgregarAvisoVeta(m, { geoda.x, BASE_GEODA_VETA + MITAD_GEODA_GRANDE_VETA * 2.0f + 0.3f, geoda.z }, 1, 0, equipo);
}


static void EfectoRecogerVeta(MinijuegoVetaCristal& m, int jugador, Vector3 p, int cantidad, int equipo)
{
    Color colorEquipo = ColorEquipoVisualVeta(equipo);
    int chispas = CantidadChispasVeta(6 + cantidad * 2);

    for (int k = 0; k < chispas; k++)
    {
        float a = AzarVisualVeta(m, 0.0f, 6.2832f);
        float v = AzarVisualVeta(m, 0.6f, 2.0f);
        Vector3 vel = { std::cos(a) * v, AzarVisualVeta(m, 2.0f, 4.5f), std::sin(a) * v };
        Color c = k % 2 == 0 ? MezclaVeta(colorEquipo, WHITE, 0.3f) : Color{ 255, 240, 190, 255 };
        EmitirChispaVeta(m, { p.x, p.y + 0.1f, p.z }, vel, c, AzarVisualVeta(m, 0.4f, 0.7f), AzarVisualVeta(m, 0.08f, 0.14f), 6.0f, true);
    }

    m.destelloJugador[jugador] = 1.0f;
    AgregarAvisoVeta(m, { p.x, p.y + 1.3f, p.z }, 0, cantidad, equipo);
}


static void EfectoVagonetaVeta(MinijuegoVetaCristal& m, Vector3 p, int perdidas, int equipo)
{
    int polvo = CantidadChispasVeta(12);

    for (int k = 0; k < polvo; k++)
    {
        float a = AzarVisualVeta(m, 0.0f, 6.2832f);
        float v = AzarVisualVeta(m, 1.0f, 3.0f);
        Vector3 vel = { std::cos(a) * v, AzarVisualVeta(m, 0.5f, 2.5f), std::sin(a) * v };
        Color c = k % 3 == 0 ? Color{ 255, 226, 120, 255 } : Color{ 150, 140, 150, 255 };
        EmitirChispaVeta(m, { p.x, 0.4f, p.z }, vel, c, AzarVisualVeta(m, 0.4f, 0.8f), AzarVisualVeta(m, 0.1f, 0.2f), 8.0f, k % 3 == 0);
    }

    if (perdidas > 0)
    {
        AgregarAvisoVeta(m, { p.x, p.y + 1.3f, p.z }, 2, perdidas, equipo);
    }

    ActivarTemblorCamaraGeneral(0.2f, 0.3f);
}


static void EmitirConfetiVeta(MinijuegoVetaCristal& m, const JugadorPrueba jugadores[], int limite)
{
    int cantidad = CantidadChispasVeta(40);

    for (int i = 0; i < limite; i++)
    {
        int equipo = m.estadosJugadores[i].equipo;

        if (equipo < 0 || (!m.empate && equipo != m.equipoGanador))
        {
            continue;
        }

        Color colorEquipo = ColorEquipoVisualVeta(equipo);

        for (int k = 0; k < cantidad; k++)
        {
            float a = AzarVisualVeta(m, 0.0f, 6.2832f);
            float v = AzarVisualVeta(m, 0.5f, 2.6f);
            Vector3 p = { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.6f, jugadores[i].posicion.z };
            Vector3 vel = { std::cos(a) * v, AzarVisualVeta(m, 3.0f, 6.5f), std::sin(a) * v };
            Color c = k % 3 == 0 ? colorEquipo : (k % 3 == 1 ? COLOR_ORO_VETA : Color{ 240, 240, 255, 255 });
            EmitirChispaVeta(m, p, vel, c, AzarVisualVeta(m, 1.4f, 2.4f), AzarVisualVeta(m, 0.08f, 0.14f), 3.2f, false);
        }
    }
}


// Foto de la partida antes de un paso: comparandola con la de despues se
// sabe que paso (cabezazo, gema, vagoneta) sin tocar las reglas.
struct InstantaneaVeta
{
    FaseVetaCristal fase = FASE_VETA_PREPARACION;
    bool cargada[MAX_GEODAS_VETA]{};
    float sacudida[MAX_GEODAS_VETA]{};
    float ventana[MAX_GEODAS_VETA]{};
    int gemasJugador[MAX_PARTICIPANTES]{};
    float aturdido[MAX_PARTICIPANTES]{};
    bool gemaActiva[MAX_GEMAS_VETA]{};
    Vector3 gemaPosicion[MAX_GEMAS_VETA]{};
    float gemaEdad[MAX_GEMAS_VETA]{};
    int gemaValor[MAX_GEMAS_VETA]{};
};


static void TomarInstantaneaVeta(const MinijuegoVetaCristal& m, InstantaneaVeta& s)
{
    s.fase = m.fase;

    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        s.cargada[g] = m.geodas[g].cargada;
        s.sacudida[g] = m.geodas[g].tiempoSacudida;
        s.ventana[g] = m.geodas[g].ventanaGolpe;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        s.gemasJugador[i] = m.estadosJugadores[i].gemas;
        s.aturdido[i] = m.estadosJugadores[i].tiempoAturdido;
    }

    for (int k = 0; k < MAX_GEMAS_VETA; k++)
    {
        s.gemaActiva[k] = m.gemas[k].activa;
        s.gemaPosicion[k] = { m.gemas[k].x, m.gemas[k].y, m.gemas[k].z };
        s.gemaEdad[k] = m.gemas[k].edad;
        s.gemaValor[k] = m.gemas[k].valor;
    }
}


static void ActualizarVisualVeta(
    MinijuegoVetaCristal& m,
    const InstantaneaVeta& s,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int limite
)
{
    // Geodas: cabezazos, aperturas y recargas.
    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        const GeodaVeta& geoda = m.geodas[g];

        if (geoda.tiempoSacudida > s.sacudida[g] + 0.001f)
        {
            bool abierta = s.cargada[g] && !geoda.cargada;
            EfectoGolpeGeodaVeta(m, g, abierta);

            if (abierta && geoda.grande)
            {
                int equipo = geoda.x < 0.0f ? 0 : 1;
                EfectoTesoroVeta(m, g, equipo);
            }
        }

        if (!s.cargada[g] && geoda.cargada)
        {
            // Volvio a crecer: destello suave y sin grietas.
            m.grietaGeoda[g] = 0.0f;
            m.destelloGeoda[g] = 0.6f;
        }

        // La grieta de un primer golpe a la grande se cierra si nadie ayuda.
        if (geoda.cargada && geoda.ventanaGolpe <= 0.0f && m.grietaGeoda[g] > 0.0f)
        {
            m.grietaGeoda[g] -= deltaTime * 0.6f;
            if (m.grietaGeoda[g] < 0.0f) m.grietaGeoda[g] = 0.0f;
        }

        if (m.destelloGeoda[g] > 0.0f)
        {
            m.destelloGeoda[g] -= deltaTime * 3.0f;
            if (m.destelloGeoda[g] < 0.0f) m.destelloGeoda[g] = 0.0f;
        }
    }

    // Gemas que desaparecen recogidas (no por vencimiento): destello.
    for (int k = 0; k < MAX_GEMAS_VETA; k++)
    {
        if (s.gemaActiva[k] && !m.gemas[k].activa && s.gemaEdad[k] + deltaTime < VIDA_GEMA_VETA - 0.01f)
        {
            Color c = ColorGemaVeta(s.gemaValor[k]);
            int n = CantidadChispasVeta(5);

            for (int q = 0; q < n; q++)
            {
                float a = AzarVisualVeta(m, 0.0f, 6.2832f);
                EmitirChispaVeta(m, s.gemaPosicion[k], { std::cos(a) * 1.4f, AzarVisualVeta(m, 1.5f, 3.0f), std::sin(a) * 1.4f }, c, 0.45f, 0.1f, 4.0f, true);
            }
        }
    }

    // Jugadores: suma de gemas o golpe de la vagoneta.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorVeta& estado = m.estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        Vector3 p = jugadores[i].posicion;

        if (estado.tiempoAturdido > s.aturdido[i] + 0.5f)
        {
            EfectoVagonetaVeta(m, p, s.gemasJugador[i] - estado.gemas, estado.equipo);
        }
        else if (estado.gemas > s.gemasJugador[i])
        {
            EfectoRecogerVeta(m, i, p, estado.gemas - s.gemasJugador[i], estado.equipo);
        }

        if (m.destelloJugador[i] > 0.0f)
        {
            m.destelloJugador[i] -= deltaTime * 2.5f;
            if (m.destelloJugador[i] < 0.0f) m.destelloJugador[i] = 0.0f;
        }
    }

    // Anuncios de inicio y confeti del final.
    if (s.fase == FASE_VETA_PREPARACION && m.fase == FASE_VETA_JUGANDO)
    {
        m.tiempoAnuncioInicio = 0.0f;
    }
    else if (m.tiempoAnuncioInicio >= 0.0f)
    {
        m.tiempoAnuncioInicio += deltaTime;
        if (m.tiempoAnuncioInicio > DURACION_ANUNCIO_VETA) m.tiempoAnuncioInicio = -1.0f;
    }

    if (m.fase == FASE_VETA_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        float antes = m.tiempoFin;
        m.tiempoFin = m.tiempoFin < 0.0f ? 0.0f : m.tiempoFin + deltaTime;

        if (antes < 0.0f || (antes < 0.8f && m.tiempoFin >= 0.8f))
        {
            EmitirConfetiVeta(m, jugadores, limite);
        }
    }

    // El acopio de cada equipo se llena de a poco hacia su puntaje.
    for (int e = 0; e < 2; e++)
    {
        float objetivo = (float)PuntajeEquipoVeta(m, e);
        float k = deltaTime * 4.0f;
        if (k > 1.0f) k = 1.0f;
        m.marcadorVisual[e] += (objetivo - m.marcadorVisual[e]) * k;
    }

    // Vagoneta visual: se ve llegar por el puente durante el aviso y sigue
    // de largo al salir del tramo que golpea.
    if (m.vagonetaActiva)
    {
        m.vagonetaVisible = true;
        m.vagonetaVisualZ = m.vagonetaZ;
        m.vagonetaVisualDireccion = m.direccionVagoneta;
    }
    else if (m.avisoVagoneta)
    {
        float falta = m.tiempoHastaVagoneta > 0.0f ? m.tiempoHastaVagoneta : 0.0f;
        m.vagonetaVisible = true;
        m.vagonetaVisualDireccion = m.direccionVagoneta;
        m.vagonetaVisualZ = -m.direccionVagoneta * (EXTREMO_VAGONETA_VETA + falta * VELOCIDAD_VAGONETA_VETA);
    }
    else if (m.vagonetaVisible)
    {
        m.vagonetaVisualZ += m.vagonetaVisualDireccion * VELOCIDAD_VAGONETA_VETA * deltaTime;
        if (m.vagonetaVisualZ * m.vagonetaVisualDireccion > 34.0f) m.vagonetaVisible = false;
    }

    if (m.vagonetaVisible)
    {
        m.giroRuedas += VELOCIDAD_VAGONETA_VETA / 0.3f * deltaTime * 57.3f;
        if (m.giroRuedas > 360.0f) m.giroRuedas -= 360.0f;

        // Chispas de las ruedas sobre los rieles.
        if (m.vagonetaActiva && AzarVisualVeta(m, 0.0f, 1.0f) < 0.6f * FactorCalidadGrafica(CalidadParticulas()))
        {
            float lado = AzarVisualVeta(m, 0.0f, 1.0f) < 0.5f ? -0.6f : 0.6f;
            float atras = -m.vagonetaVisualDireccion * 0.7f;
            Vector3 p = { lado, 0.22f, m.vagonetaVisualZ + atras };
            Vector3 v = { lado * 1.5f, AzarVisualVeta(m, 1.0f, 2.6f), -m.vagonetaVisualDireccion * AzarVisualVeta(m, 1.0f, 3.0f) };
            EmitirChispaVeta(m, p, v, Color{ 255, 210, 120, 255 }, 0.3f, 0.06f, 12.0f, true);
        }
    }

    // Chispas y avisos.
    for (int i = 0; i < MAX_CHISPAS_VETA; i++)
    {
        ChispaVeta& c = m.chispas[i];
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

        if (c.posicion.y < 0.04f)
        {
            c.posicion.y = 0.04f;
            c.velocidad.y = -c.velocidad.y * 0.3f;
            c.velocidad.x *= 0.6f;
            c.velocidad.z *= 0.6f;
        }
    }

    for (int i = 0; i < MAX_AVISOS_VETA; i++)
    {
        if (!m.avisos[i].activo) continue;
        m.avisos[i].vida -= deltaTime;
        if (m.avisos[i].vida <= 0.0f) m.avisos[i].activo = false;
    }
}


void MinijuegoVetaCristal::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    InstantaneaVeta antes;
    TomarInstantaneaVeta(*this, antes);

    ActualizarPartida(deltaTime, jugadores, cantidadMaxima, participantes);

    ActualizarVisualVeta(*this, antes, deltaTime, jugadores, LimiteJugadoresVeta(cantidadMaxima));
}


//==================================================
// VISUAL: ABISMO, PUENTE Y TECHO (HORIZONTE)
//==================================================

// Altura del borde superior de la vista a la profundidad z (camara de juego).
static float AlturaBordeVistaVeta(float z)
{
    return 18.0f - (14.5f - z) * 0.479f;
}


// Fondo 2D: el aire del abismo, oscuro arriba y con el brillo de los
// cristales del fondo cerca del borde de la galeria.
static void DibujarFondoVeta(float t)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int medio = (int)((float)alto * 0.34f);

    DrawRectangleGradientV(0, 0, ancho, medio, Color{ 18, 14, 42, 255 }, Color{ 60, 44, 112, 255 });
    DrawRectangle(0, medio, ancho, alto - medio, Color{ 60, 44, 112, 255 });

    // Motas lejanas del abismo que titilan como estrellas.
    for (int k = 0; k < 46; k++)
    {
        float x = Ruido01Veta(k, 501) * (float)ancho;
        float y = Ruido01Veta(k, 502) * (float)medio;
        float brillo = 0.4f + 0.6f * (0.5f + 0.5f * std::sin(t * (1.0f + Ruido01Veta(k, 503) * 2.0f) + (float)k));
        Color c = k % 3 == 0 ? Color{ 200, 150, 255, 255 } : Color{ 150, 220, 255, 255 };
        DrawRectangle((int)x, (int)y, 2, 2, AlfaVeta(c, 0.55f * brillo));
    }
}


// Aguja de cristal gigante que sube del fondo del abismo.
static void AgujaCristalVeta(Vector3 base, float alto, float radio, Color color, float inclinX, float inclinZ, float giro)
{
    Vector3 punta = { base.x + inclinX * alto, base.y + alto, base.z + inclinZ * alto };
    CristalVeta(base, RestaVeta(punta, base), alto, radio, color, giro, true);
}


static void DibujarAbismoVeta(float t, bool decoracion, int paso)
{
    (void)t;

    // Agujas de cristal gigantes en dos profundidades.
    for (int k = 0; k < 14; k += decoracion ? 1 : 2)
    {
        bool lejana = k >= 7;
        float u = Ruido01Veta(k, 610);
        float x = (u - 0.5f) * (lejana ? 70.0f : 52.0f);
        float z = lejana ? -30.0f - 14.0f * Ruido01Veta(k, 611) : -15.0f - 8.0f * Ruido01Veta(k, 611);

        // Que no tapen el puente ni el borde de la galeria.
        if (!lejana && std::fabs(x) < 6.0f) x = x < 0.0f ? x - 6.0f : x + 6.0f;

        float techo = AlturaBordeVistaVeta(z);
        float alto = techo + 30.0f - (1.0f + 6.0f * Ruido01Veta(k, 612));
        float radio = (lejana ? 2.6f : 1.7f) + 1.2f * Ruido01Veta(k, 613);
        Color c = (k % 3 == 1) ? COLOR_CRISTAL_VIOLETA_VETA : COLOR_CRISTAL_AZUL_VETA;
        c = MezclaVeta(c, COLOR_NIEBLA_VETA, lejana ? 0.35f : 0.15f);
        AgujaCristalVeta({ x, -30.0f, z }, alto, radio, c, (Ruido01Veta(k, 614) - 0.5f) * 0.25f, (Ruido01Veta(k, 615) - 0.5f) * 0.15f, (float)k);

        // Racimo menor al pie de las cercanas.
        if (!lejana && decoracion)
        {
            AgujaCristalVeta({ x + 2.2f, -30.0f, z + 1.4f }, alto * 0.7f, radio * 0.6f, MezclaVeta(c, WHITE, 0.1f), 0.12f, 0.05f, (float)k + 1.0f);
        }
    }

    // Paredes del abismo a los costados: cornisas escalonadas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int s = 0; s < 6; s++)
        {
            float z = -11.0f - (float)s * 5.5f;
            float techo = AlturaBordeVistaVeta(z) + 4.0f;
            float x = (float)lado * (16.0f + 3.0f * Ruido01Veta(s, 620 + lado) + (float)s * 1.6f);
            float alto = techo + 30.0f;
            Color roca = s % 2 == 0 ? COLOR_ROCA_VETA : COLOR_ROCA_OSCURA_VETA;
            CuboNieblaVeta({ x, techo - alto * 0.5f, z }, { 9.0f, alto, 5.8f }, roca);

            // Cornisa saliente con cristales.
            float yCornisa = -3.0f - 6.0f * Ruido01Veta(s, 622 + lado);
            CuboNieblaVeta({ x - (float)lado * 4.8f, yCornisa, z }, { 1.8f, 1.2f, 4.6f }, COLOR_ROCA_VETA);

            if (decoracion && (s % paso) == 0)
            {
                Color c = (s + lado) % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA;
                RacimoCristalVeta({ x - (float)lado * 5.2f, yCornisa + 0.5f, z }, { -(float)lado * 0.4f, 1.0f, 0.2f }, 2.4f, c, s * 7 + lado, true);
            }
        }
    }

    // Techo: estalactitas que cuelgan sobre el abismo (asoman arriba).
    if (decoracion)
    {
        for (int k = 0; k < 16; k += paso)
        {
            float z = -20.0f - 18.0f * Ruido01Veta(k, 630);
            float x = (Ruido01Veta(k, 631) - 0.5f) * 56.0f;
            float punta = AlturaBordeVistaVeta(z) - (1.5f + 4.0f * Ruido01Veta(k, 632));
            float radio = 1.2f + 1.6f * Ruido01Veta(k, 633);
            Vector3 arriba = { x, punta + 18.0f, z };
            Vector3 abajo = { x + 0.3f, punta, z };
            PrismaEjeVeta(arriba, abajo, radio, 0.0f, 6, COLOR_ROCA_OSCURA_VETA, (float)k, 0.0f, true);

            if (k % 3 == 0)
            {
                Color c = k % 2 == 0 ? COLOR_CRISTAL_VIOLETA_VETA : COLOR_CRISTAL_AZUL_VETA;
                CristalVeta({ x + 0.3f, punta + 1.6f, z }, { 0.1f, -1.0f, 0.0f }, 2.4f, 0.35f, c, 0.0f, true);
            }
        }
    }
}


// Puente de caballetes por el que llega la vagoneta desde el fondo.
static void DibujarPuenteVeta(bool decoracion)
{
    const float Z_BORDE = -9.6f;
    const float Z_FIN = -60.0f;

    rlBegin(RL_QUADS);
    // Tablero: grava sobre vigas.
    QuadNieblaVeta({ -1.3f, 0.06f, Z_BORDE }, { 1.3f, 0.06f, Z_BORDE }, { 1.3f, 0.06f, Z_FIN }, { -1.3f, 0.06f, Z_FIN }, COLOR_GRAVA_VETA);
    QuadNieblaVeta({ -1.45f, -0.4f, Z_BORDE }, { -1.45f, 0.06f, Z_BORDE }, { -1.45f, 0.06f, Z_FIN }, { -1.45f, -0.4f, Z_FIN }, TonoVeta(COLOR_MADERA_OSCURA_VETA, 0.8f));
    QuadNieblaVeta({ 1.45f, -0.4f, Z_BORDE }, { 1.45f, 0.06f, Z_BORDE }, { 1.45f, 0.06f, Z_FIN }, { 1.45f, -0.4f, Z_FIN }, TonoVeta(COLOR_MADERA_OSCURA_VETA, 0.7f));
    rlEnd();

    // Durmientes y rieles.
    for (float z = Z_BORDE - 0.4f; z > -26.0f; z -= 0.75f)
    {
        CuboNieblaVeta({ 0.0f, 0.1f, z }, { 2.0f, 0.08f, 0.26f }, COLOR_MADERA_VETA);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        rlBegin(RL_QUADS);
        float x = (float)lado * 0.6f;
        QuadNieblaVeta({ x - 0.05f, 0.2f, Z_BORDE }, { x + 0.05f, 0.2f, Z_BORDE }, { x + 0.05f, 0.2f, Z_FIN }, { x - 0.05f, 0.2f, Z_FIN }, COLOR_ACERO_VETA);
        QuadNieblaVeta({ x - 0.05f, 0.13f, Z_BORDE }, { x - 0.05f, 0.2f, Z_BORDE }, { x - 0.05f, 0.2f, Z_FIN }, { x - 0.05f, 0.13f, Z_FIN }, TonoVeta(COLOR_ACERO_VETA, 0.6f));
        rlEnd();

        // Baranda de troncos.
        for (float z = Z_BORDE - 1.6f; z > -26.0f; z -= 3.2f)
        {
            CuboNieblaVeta({ (float)lado * 1.4f, 0.5f, z }, { 0.16f, 0.9f, 0.16f }, COLOR_MADERA_OSCURA_VETA);
        }
        PrismaEjeVeta({ (float)lado * 1.4f, 0.9f, Z_BORDE }, { (float)lado * 1.4f, 0.9f, -26.0f }, 0.06f, 0.06f, 4, COLOR_MADERA_VETA, 0.0f, 0.0f, true);
    }

    // Caballetes: patas inclinadas y cruces hasta lo hondo.
    if (decoracion)
    {
        for (float z = Z_BORDE - 2.2f; z > -40.0f; z -= 4.2f)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                PrismaEjeVeta({ (float)lado * 1.3f, 0.0f, z }, { (float)lado * 3.4f, -26.0f, z }, 0.18f, 0.22f, 4, COLOR_MADERA_OSCURA_VETA, 0.785f, 0.0f, true);
            }

            for (int nivel = 0; nivel < 3; nivel++)
            {
                float y0 = -2.0f - (float)nivel * 6.0f;
                float y1 = y0 - 5.0f;
                float x0 = 1.3f + 2.1f * (-y0 / 26.0f);
                float x1 = 1.3f + 2.1f * (-y1 / 26.0f);
                PrismaEjeVeta({ -x0, y0, z }, { x1, y1, z }, 0.09f, 0.09f, 4, COLOR_MADERA_VETA, 0.785f, 0.0f, true);
                PrismaEjeVeta({ x0, y0, z }, { -x1, y1, z }, 0.09f, 0.09f, 4, COLOR_MADERA_VETA, 0.785f, 0.0f, true);
            }
        }
    }
}


//==================================================
// VISUAL: GALERIA (MEDIA DISTANCIA)
//==================================================

// Paredes de roca de los costados con estratos, cornisas y vetas.
static void DibujarParedesVeta(float t, bool decoracion, int paso)
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;

        for (int k = 0; k < 12; k++)
        {
            float z = -10.6f + (float)k * 2.0f;
            float alto = 4.6f + 3.2f * Ruido01Veta(k, 700 + lado);
            float sale = 0.25f * Ruido01Veta(k, 702 + lado);
            Color roca = MezclaVeta(COLOR_ROCA_VETA, COLOR_ROCA_OSCURA_VETA, Ruido01Veta(k, 704 + lado));

            CuboRocaVeta({ s * (15.6f - sale), alto * 0.5f, z }, { 6.0f + sale * 2.0f, alto, 2.06f }, roca);

            // Estrato claro y cornisa baja.
            CuboVeta({ s * (12.62f - sale), 1.4f + 1.8f * Ruido01Veta(k, 706 + lado), z }, { 0.14f, 0.3f, 2.0f }, MezclaVeta(roca, Color{ 160, 146, 170, 255 }, 0.4f));
            CuboRocaVeta({ s * 12.95f, 0.4f + 0.3f * Ruido01Veta(k, 708 + lado), z + 0.2f }, { 0.9f, 0.8f + 0.6f * Ruido01Veta(k, 710 + lado), 1.7f }, COLOR_ROCA_OSCURA_VETA);

            // Bloque superior retranqueado: silueta irregular.
            if (decoracion)
            {
                float extra = 1.5f + 2.5f * Ruido01Veta(k, 712 + lado);
                CuboRocaVeta({ s * 17.2f, alto + extra * 0.5f, z + 0.3f }, { 3.2f, extra, 1.8f }, TonoVeta(roca, 0.92f));

                // Estalagmitas y algun cristal sobre la cornisa alta.
                if ((k + lado) % 3 != 0)
                {
                    float xm = s * (13.6f + 1.2f * Ruido01Veta(k, 716 + lado));
                    float hm = 1.2f + 1.8f * Ruido01Veta(k, 717 + lado);
                    Color cm = PenumbraVeta(COLOR_ROCA_VETA, alto + hm * 0.4f);
                    PrismaEjeVeta({ xm, alto, z - 0.3f }, { xm + 0.1f, alto + hm, z - 0.3f }, 0.45f, 0.0f, 6, cm, (float)k);
                }
                else
                {
                    Color c = lado < 0 ? COLOR_CRISTAL_VIOLETA_VETA : COLOR_CRISTAL_AZUL_VETA;
                    RacimoCristalVeta({ s * 13.4f, alto, z }, { -s * 0.3f, 1.0f, 0.2f }, 0.8f, TonoVeta(c, 0.8f), k * 3 + lado);
                }
            }

            // Veta luminosa en zigzag sobre la cara de la pared.
            if (decoracion && (k % 2) == 0)
            {
                Color veta = (k / 2 + lado) % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA;
                float pulso = 0.75f + 0.25f * std::sin(t * 1.6f + (float)k);
                Color c = TonoVeta(veta, pulso);
                float y = 2.4f + 1.6f * Ruido01Veta(k, 714 + lado);
                float xc = s * (12.6f - sale) - s * 0.02f;

                rlBegin(RL_QUADS);
                for (int q = 0; q < 4; q++)
                {
                    float za = z - 0.9f + (float)q * 0.45f;
                    float zb = za + 0.45f;
                    float ya = y + ((q & 1) ? 0.25f : -0.2f);
                    float yb = y + ((q & 1) ? -0.2f : 0.25f);
                    QuadVeta({ xc, ya - 0.05f, za }, { xc, yb - 0.05f, zb }, { xc, yb + 0.05f, zb }, { xc, ya + 0.05f, za }, c);
                }
                rlEnd();
            }
        }

        // Racimos de cristal incrustados en la base de la pared.
        if (decoracion)
        {
            const float zs[6] = { -8.6f, -4.4f, -0.9f, 3.3f, 5.6f, 9.4f };
            for (int k = 0; k < 6; k += paso)
            {
                Color c = (k + lado) % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA;
                float y = 0.2f + 1.8f * Ruido01Veta(k, 720 + lado);
                RacimoCristalVeta({ s * 12.7f, y, zs[k] }, { -s * 0.9f, 0.7f, 0.15f }, 0.9f + 0.5f * Ruido01Veta(k, 721 + lado), c, k * 13 + lado);
            }
        }
    }

    // Borde del fondo: roca entre el piso y el abismo, y columnas de roca
    // que enmarcan la boca del abismo en las esquinas.
    rlBegin(RL_QUADS);
    QuadVeta({ -12.6f, 0.0f, -7.95f }, { 12.6f, 0.0f, -7.95f }, { 12.6f, 0.0f, -9.6f }, { -12.6f, 0.0f, -9.6f }, COLOR_ROCA_OSCURA_VETA);
    rlEnd();

    for (int k = 0; k < 14; k++)
    {
        float x = -12.0f + (float)k * 1.85f;
        if (std::fabs(x) < 1.6f) continue;
        float h = 0.18f + 0.2f * Ruido01Veta(k, 730);
        CuboVeta({ x, h * 0.5f - 0.05f, -9.45f }, { 1.9f, h, 0.5f }, MezclaVeta(COLOR_ROCA_VETA, COLOR_ROCA_OSCURA_VETA, Ruido01Veta(k, 731)));
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        CuboRocaVeta({ s * 14.4f, 1.0f, -11.2f }, { 4.6f, 12.0f, 3.4f }, COLOR_ROCA_VETA);
        CuboRocaVeta({ s * 13.6f, 3.0f, -10.4f }, { 2.4f, 8.0f, 1.6f }, COLOR_ROCA_OSCURA_VETA);

        if (decoracion)
        {
            RacimoCristalVeta({ s * 12.6f, 0.6f, -10.0f }, { -s * 0.7f, 0.8f, 0.3f }, 1.6f, lado < 0 ? COLOR_CRISTAL_VIOLETA_VETA : COLOR_CRISTAL_AZUL_VETA, 41 + lado);
        }
    }
}


// Entibado: postes, sombreretes y tornapuntas contra las paredes, con la
// lampara de minero colgando de cada uno.
static const int CANTIDAD_LAMPARAS_VETA = 8;


static Vector3 PosicionLamparaVeta(int indice, float t)
{
    float lado = indice < 4 ? -1.0f : 1.0f;
    static const float ZS[4] = { -6.4f, -2.2f, 2.2f, 6.4f };
    float z = ZS[indice % 4];
    float vaiven = std::sin(t * 1.3f + (float)indice * 1.7f) * 0.12f * ObtenerFactorMovimientoCamaraMinijuegos();
    return { lado * 11.55f + vaiven * 0.3f, 3.35f, z + vaiven };
}


static void DibujarEntibadoVeta(float t)
{
    for (int i = 0; i < CANTIDAD_LAMPARAS_VETA; i++)
    {
        float s = i < 4 ? -1.0f : 1.0f;
        Vector3 lampara = PosicionLamparaVeta(i, t);
        float z = i % 4 == 0 ? -6.4f : (i % 4 == 1 ? -2.2f : (i % 4 == 2 ? 2.2f : 6.4f));

        // Poste, zapata y sombrerete que entra en la roca.
        CuboVeta({ s * 12.3f, 2.15f, z }, { 0.36f, 4.3f, 0.36f }, COLOR_MADERA_VETA);
        CuboVeta({ s * 12.3f, 0.08f, z }, { 0.5f, 0.16f, 0.5f }, COLOR_MADERA_OSCURA_VETA);
        CuboVeta({ s * 12.55f, 4.32f, z }, { 1.9f, 0.32f, 0.42f }, COLOR_MADERA_OSCURA_VETA);
        PrismaEjeVeta({ s * 12.3f, 3.2f, z }, { s * 12.95f, 4.1f, z }, 0.09f, 0.09f, 4, COLOR_MADERA_VETA, 0.785f);

        // Abrazaderas de hierro.
        CuboVeta({ s * 12.3f, 3.7f, z }, { 0.4f, 0.08f, 0.4f }, COLOR_HIERRO_VETA);
        CuboVeta({ s * 12.3f, 1.0f, z }, { 0.4f, 0.08f, 0.4f }, COLOR_HIERRO_VETA);

        // Lampara: gancho, cadena y farol con vidrio encendido.
        // MODELO FUTURO: farol de minero.
        Vector3 gancho = { s * 11.6f, 4.16f, z };
        CadenaVeta(gancho, { lampara.x, lampara.y + 0.32f, lampara.z }, 0.03f, COLOR_HIERRO_VETA);
        float parpadeo = 0.9f + 0.1f * std::sin(t * 13.0f + (float)i * 2.3f);
        CuboVeta({ lampara.x, lampara.y + 0.26f, lampara.z }, { 0.3f, 0.06f, 0.3f }, COLOR_HIERRO_VETA);
        CuboVeta({ lampara.x, lampara.y, lampara.z }, { 0.22f, 0.4f, 0.22f }, TonoVeta(COLOR_LAMPARA_VETA, parpadeo));
        CuboVeta({ lampara.x, lampara.y - 0.24f, lampara.z }, { 0.28f, 0.08f, 0.28f }, COLOR_HIERRO_VETA);
        for (int q = 0; q < 4; q++)
        {
            float qx = (q & 1) ? 0.12f : -0.12f;
            float qz = (q & 2) ? 0.12f : -0.12f;
            CuboVeta({ lampara.x + qx, lampara.y, lampara.z + qz }, { 0.03f, 0.44f, 0.03f }, COLOR_HIERRO_VETA);
        }
    }
}


//==================================================
// VISUAL: ARENA (SUELO, BORDILLO Y VIA)
//==================================================

static void DibujarSueloVeta(float t)
{
    (void)t;

    // Piso de la galeria fuera de la arena: roca compactada.
    rlBegin(RL_QUADS);
    QuadVeta({ -12.7f, -0.01f, 12.0f }, { 12.7f, -0.01f, 12.0f }, { 12.7f, -0.01f, -7.95f }, { -12.7f, -0.01f, -7.95f }, Color{ 74, 66, 88, 255 });
    QuadVeta({ -11.0f, 0.0f, 7.6f }, { 11.0f, 0.0f, 7.6f }, { 11.0f, 0.0f, -7.6f }, { -11.0f, 0.0f, -7.6f }, COLOR_JUNTA_VETA);
    rlEnd();

    // Grava suelta alrededor.
    for (int k = 0; k < 40; k++)
    {
        float x = (Ruido01Veta(k, 800) - 0.5f) * 25.0f;
        float z = -7.9f + 19.0f * Ruido01Veta(k, 801);
        if (std::fabs(x) < 11.4f && z < 7.95f) continue;
        float r = 0.08f + 0.12f * Ruido01Veta(k, 802);
        CuboGiradoVeta({ x, r * 0.4f, z }, { r * 2.0f, r * 0.8f, r * 1.6f }, 90.0f * Ruido01Veta(k, 803), MezclaVeta(COLOR_ROCA_VETA, COLOR_GRAVA_VETA, Ruido01Veta(k, 804)));
    }

    // Losas irregulares de cada mitad, con un leve tinte del equipo.
    const int FILAS = 12;
    const float Z0 = -7.6f;
    const float ALTO_FILA = 15.2f / (float)FILAS;

    rlBegin(RL_QUADS);

    for (int lado = 0; lado < 2; lado++)
    {
        float s = lado == 0 ? -1.0f : 1.0f;
        Color tinte = ObtenerColorEquipoVeta(lado);

        for (int f = 0; f < FILAS; f++)
        {
            float za = Z0 + (float)f * ALTO_FILA;
            float zb = za + ALTO_FILA;
            float x = 1.32f;
            int piedra = 0;

            while (x < 11.0f - 0.01f)
            {
                int id = (lado * 97 + f) * 31 + piedra;
                float ancho = 1.1f + 0.9f * Ruido01Veta(id, 810);
                if (piedra == 0) ancho *= 0.4f + 0.6f * Ruido01Veta(f, 811 + lado);
                if (x + ancho > 11.0f || 11.0f - (x + ancho) < 0.5f) ancho = 11.0f - x;

                float brillo = 0.88f + 0.2f * Ruido01Veta(id, 812);
                Color c = MezclaVeta(TonoVeta(COLOR_LOSA_VETA, brillo), tinte, 0.13f);
                float xa = s * (x + 0.05f), xb = s * (x + ancho - 0.05f);
                float y = 0.012f + 0.01f * Ruido01Veta(id, 813);

                QuadVeta({ xa, y, zb - 0.05f }, { xb, y, zb - 0.05f }, { xb, y, za + 0.05f }, { xa, y, za + 0.05f }, c);

                // Canto frontal apenas visible: da relieve a la losa.
                Color canto = TonoVeta(c, 0.7f);
                QuadVeta({ xa, 0.0f, zb - 0.05f }, { xb, 0.0f, zb - 0.05f }, { xb, y, zb - 0.05f }, { xa, y, zb - 0.05f }, canto);

                x += ancho;
                piedra++;
            }
        }
    }

    rlEnd();

    // Grietas finas y astillas de cristal embutidas en algunas losas.
    for (int k = 0; k < 26; k++)
    {
        float s = k % 2 == 0 ? -1.0f : 1.0f;
        float x = s * (1.8f + 8.8f * Ruido01Veta(k, 820));
        float z = -7.0f + 14.0f * Ruido01Veta(k, 821);
        float a = 6.2832f * Ruido01Veta(k, 822);
        float l = 0.3f + 0.4f * Ruido01Veta(k, 823);

        if (k % 3 == 0)
        {
            Color c = k % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA;
            CuboGiradoVeta({ x, 0.03f, z }, { 0.16f, 0.04f, 0.08f }, a * 57.3f, TonoVeta(c, 0.85f));
            CuboGiradoVeta({ x + 0.12f, 0.03f, z + 0.05f }, { 0.1f, 0.04f, 0.06f }, a * 57.3f + 40.0f, TonoVeta(c, 0.75f));
        }
        else
        {
            float x2 = x + std::cos(a) * l, z2 = z + std::sin(a) * l;
            TrazoSueloVeta(x, z, x2, z2, 0.026f, 0.035f, Color{ 70, 62, 80, 255 });
            TrazoSueloVeta(x2, z2, x2 + std::cos(a + 0.7f) * l * 0.6f, z2 + std::sin(a + 0.7f) * l * 0.6f, 0.026f, 0.03f, Color{ 70, 62, 80, 255 });
        }
    }

    // Bordillo de durmientes viejos alrededor de la arena (cara interna en
    // el limite real del cuerpo: x = 10.6 + 0.4, z = 7.2 + 0.4).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        CuboVeta({ s * 11.18f, 0.16f, 0.0f }, { 0.36f, 0.32f, 15.9f }, COLOR_MADERA_OSCURA_VETA);
        CuboVeta({ s * 11.18f, 0.33f, 0.0f }, { 0.4f, 0.04f, 15.9f }, COLOR_MADERA_VETA);

        for (int q = 0; q < 2; q++)
        {
            float zc = q == 0 ? -7.78f : 7.78f;
            float alto = q == 0 ? 0.32f : 0.22f;
            CuboVeta({ s * 6.2f, alto * 0.5f, zc }, { 9.6f, alto, 0.36f }, COLOR_MADERA_OSCURA_VETA);
            CuboVeta({ s * 6.2f, alto + 0.02f, zc }, { 9.6f, 0.04f, 0.4f }, COLOR_MADERA_VETA);
        }

        // Clavos de via.
        for (int k = 0; k < 8; k++)
        {
            CuboVeta({ s * 11.18f, 0.36f, -7.0f + (float)k * 2.0f }, { 0.08f, 0.04f, 0.08f }, COLOR_HIERRO_VETA);
        }
    }
}


static void DibujarViaVeta(const MinijuegoVetaCristal& m, float t)
{
    (void)m;
    (void)t;

    // Balasto de grava con piedritas.
    CuboVeta({ 0.0f, 0.03f, 1.2f }, { 2.6f, 0.06f, 21.6f }, COLOR_GRAVA_VETA);

    for (int k = 0; k < 60; k++)
    {
        float x = (Ruido01Veta(k, 840) - 0.5f) * 2.4f;
        float z = -9.4f + 21.0f * Ruido01Veta(k, 841);
        CuboGiradoVeta({ x, 0.07f, z }, { 0.1f, 0.04f, 0.08f }, 80.0f * Ruido01Veta(k, 842), TonoVeta(COLOR_GRAVA_VETA, 0.75f + 0.4f * Ruido01Veta(k, 843)));
    }

    // Franjas de peligro en los bordes del carril (amarillo y negro).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 1.36f;
        CuboVeta({ x, 0.035f, 0.0f }, { 0.12f, 0.03f, 15.2f }, Color{ 40, 36, 44, 255 });

        for (int k = 0; k < 19; k++)
        {
            float z = -7.2f + (float)k * 0.8f;
            CuboVeta({ x, 0.05f, z }, { 0.13f, 0.02f, 0.4f }, Color{ 236, 188, 56, 255 });
        }
    }

    // Durmientes, placas y rieles.
    for (float z = -9.4f; z < 11.8f; z += 0.75f)
    {
        CuboVeta({ 0.0f, 0.1f, z }, { 2.0f, 0.08f, 0.26f }, MezclaVeta(COLOR_MADERA_VETA, COLOR_MADERA_OSCURA_VETA, Ruido01Veta((int)(z * 10.0f) + 100, 850)));
        CuboVeta({ -0.6f, 0.15f, z }, { 0.22f, 0.03f, 0.2f }, COLOR_HIERRO_VETA);
        CuboVeta({ 0.6f, 0.15f, z }, { 0.22f, 0.03f, 0.2f }, COLOR_HIERRO_VETA);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 0.6f;
        CuboVeta({ x, 0.2f, 1.1f }, { 0.1f, 0.1f, 21.4f }, TonoVeta(COLOR_ACERO_VETA, 0.8f));
        CuboVeta({ x, 0.255f, 1.1f }, { 0.12f, 0.012f, 21.4f }, Color{ 220, 226, 238, 255 });
    }
}


//==================================================
// VISUAL: UTILERIA (ANILLO CERCANO)
//==================================================

static void CajonVeta(Vector3 base, float lado, float giro, Color madera)
{
    Vector3 c = { base.x, base.y + lado * 0.5f, base.z };
    CuboGiradoVeta(c, { lado, lado, lado }, giro, madera);
    // Listones y esquineros.
    CuboGiradoVeta({ c.x, c.y + lado * 0.5f + 0.005f, c.z }, { lado * 1.02f, 0.02f, lado * 0.16f }, giro, TonoVeta(madera, 0.7f));
    CuboGiradoVeta({ c.x, c.y, c.z }, { lado * 1.04f, lado * 0.14f, lado * 1.04f }, giro, TonoVeta(madera, 0.75f));
}


static void BarrilVeta(Vector3 base, float escala, Color madera)
{
    float r = 0.32f * escala;
    float alto = 0.85f * escala;
    PrismaEjeVeta(base, { base.x, base.y + alto * 0.5f, base.z }, r * 0.88f, r, 10, madera);
    PrismaEjeVeta({ base.x, base.y + alto * 0.5f, base.z }, { base.x, base.y + alto, base.z }, r, r * 0.88f, 10, madera);
    PrismaEjeVeta({ base.x, base.y + alto * 0.2f, base.z }, { base.x, base.y + alto * 0.27f, base.z }, r * 0.97f, r * 0.99f, 10, COLOR_HIERRO_VETA);
    PrismaEjeVeta({ base.x, base.y + alto * 0.73f, base.z }, { base.x, base.y + alto * 0.8f, base.z }, r * 0.99f, r * 0.97f, 10, COLOR_HIERRO_VETA);
}


static void PicoVeta(Vector3 base, float giro)
{
    // Pico apoyado: cabo de madera y cabeza de hierro.
    float c = std::cos(giro), s = std::sin(giro);
    Vector3 arriba = { base.x + c * 0.25f, base.y + 0.95f, base.z + s * 0.25f };
    PrismaEjeVeta(base, arriba, 0.035f, 0.035f, 5, COLOR_MADERA_VETA);
    PrismaEjeVeta({ arriba.x - s * 0.36f, arriba.y - 0.08f, arriba.z + c * 0.36f }, { arriba.x + s * 0.36f, arriba.y - 0.08f, arriba.z - c * 0.36f }, 0.05f, 0.05f, 4, COLOR_HIERRO_VETA);
}


static void PalaVeta(Vector3 base, float giro)
{
    float c = std::cos(giro), s = std::sin(giro);
    Vector3 arriba = { base.x + c * 0.3f, base.y + 1.05f, base.z + s * 0.3f };
    PrismaEjeVeta({ base.x, base.y + 0.3f, base.z }, arriba, 0.03f, 0.03f, 5, COLOR_MADERA_VETA);
    CuboGiradoVeta({ base.x, base.y + 0.17f, base.z }, { 0.26f, 0.32f, 0.04f }, -giro * 57.3f, COLOR_HIERRO_VETA);
}


static void SacoVeta(Vector3 base, float escala, int semilla)
{
    RocaVeta({ base.x, base.y + 0.22f * escala, base.z }, { 0.3f * escala, 0.24f * escala, 0.26f * escala }, semilla, Color{ 168, 146, 112, 255 }, 0.06f);
    CuboVeta({ base.x, base.y + 0.48f * escala, base.z }, { 0.14f * escala, 0.1f * escala, 0.14f * escala }, Color{ 140, 118, 88, 255 });
}


static void PilaRocasVeta(Vector3 base, float escala, int semilla, bool conCristal)
{
    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.9f + (float)semilla;
        float r = (0.22f + 0.14f * Ruido01Veta(k, semilla)) * escala;
        RocaVeta({ base.x + std::cos(a) * 0.3f * escala, base.y + r * 0.6f, base.z + std::sin(a) * 0.3f * escala }, { r, r * 0.8f, r }, semilla + k, COLOR_ROCA_VETA, 0.15f);
    }

    if (conCristal)
    {
        RacimoCristalVeta({ base.x, base.y + 0.2f * escala, base.z }, { 0.0f, 1.0f, 0.2f }, 0.55f * escala, semilla % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA, semilla);
    }
}


static void CarretillaVeta(Vector3 base, float giroGrados)
{
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboVeta({ 0.0f, 0.42f, 0.0f }, { 0.62f, 0.3f, 0.9f }, Color{ 104, 110, 124, 255 });
    CuboVeta({ 0.0f, 0.58f, 0.0f }, { 0.66f, 0.04f, 0.94f }, COLOR_HIERRO_VETA);
    PrismaEjeVeta({ -0.08f, 0.18f, 0.5f }, { 0.08f, 0.18f, 0.5f }, 0.17f, 0.17f, 10, Color{ 50, 50, 58, 255 });
    PrismaEjeVeta({ -0.25f, 0.45f, -0.4f }, { -0.3f, 0.5f, -1.0f }, 0.03f, 0.03f, 4, COLOR_MADERA_VETA);
    PrismaEjeVeta({ 0.25f, 0.45f, -0.4f }, { 0.3f, 0.5f, -1.0f }, 0.03f, 0.03f, 4, COLOR_MADERA_VETA);
    RacimoCristalVeta({ 0.0f, 0.55f, 0.05f }, { 0.0f, 1.0f, 0.1f }, 0.4f, COLOR_CRISTAL_VIOLETA_VETA, 77);
    rlPopMatrix();
}


static void RolloSogaVeta(Vector3 base)
{
    for (int k = 0; k < 3; k++)
    {
        AnilloVeta({ base.x, base.y + 0.04f + (float)k * 0.06f, base.z }, 0.12f, 0.3f - (float)k * 0.03f, TonoVeta(Color{ 176, 146, 96, 255 }, 0.9f + 0.1f * (float)k), 14);
    }
}


// Torno de extraccion: rueda que gira y un balde que sube y baja.
static void TornoVeta(Vector3 base, float t)
{
    float giro = t * 40.0f * ObtenerFactorMovimientoCamaraMinijuegos();

    // Caballete en A.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = base.x + (float)lado * 0.7f;
        PrismaEjeVeta({ x, base.y, base.z + 0.5f }, { x, base.y + 1.7f, base.z }, 0.07f, 0.07f, 4, COLOR_MADERA_VETA, 0.785f);
        PrismaEjeVeta({ x, base.y, base.z - 0.5f }, { x, base.y + 1.7f, base.z }, 0.07f, 0.07f, 4, COLOR_MADERA_VETA, 0.785f);
    }

    // Tambor con soga y rueda de rayos.
    Vector3 eje0 = { base.x - 0.75f, base.y + 1.6f, base.z };
    Vector3 eje1 = { base.x + 0.75f, base.y + 1.6f, base.z };
    PrismaEjeVeta(eje0, eje1, 0.06f, 0.06f, 6, COLOR_HIERRO_VETA);
    PrismaEjeVeta({ base.x - 0.4f, eje0.y, base.z }, { base.x + 0.3f, eje0.y, base.z }, 0.24f, 0.24f, 10, Color{ 176, 146, 96, 255 }, giro * 0.0174f);

    Vector3 centroRueda = { base.x + 0.55f, eje0.y, base.z };
    PrismaEjeVeta({ centroRueda.x - 0.04f, centroRueda.y, centroRueda.z }, { centroRueda.x + 0.04f, centroRueda.y, centroRueda.z }, 0.72f, 0.72f, 16, COLOR_MADERA_OSCURA_VETA, giro * 0.0174f);
    PrismaEjeVeta({ centroRueda.x - 0.06f, centroRueda.y, centroRueda.z }, { centroRueda.x + 0.06f, centroRueda.y, centroRueda.z }, 0.6f, 0.6f, 16, COLOR_MADERA_VETA, giro * 0.0174f);

    for (int k = 0; k < 4; k++)
    {
        float a = giro * 0.0174f + (float)k * 0.785f;
        Vector3 d = { 0.0f, std::cos(a) * 0.66f, std::sin(a) * 0.66f };
        PrismaEjeVeta({ centroRueda.x + 0.08f, centroRueda.y - d.y, centroRueda.z - d.z }, { centroRueda.x + 0.08f, centroRueda.y + d.y, centroRueda.z + d.z }, 0.04f, 0.04f, 4, COLOR_MADERA_OSCURA_VETA);
    }

    // Soga a la polea del borde y balde sobre el abismo.
    float sube = 0.5f + 0.5f * std::sin(t * 0.5f);
    Vector3 polea = { base.x, base.y + 1.9f, base.z - 1.6f };
    PrismaEjeVeta({ base.x, eje0.y + 0.24f, base.z }, polea, 0.02f, 0.02f, 4, Color{ 176, 146, 96, 255 });
    PrismaEjeVeta({ base.x, base.y, base.z - 1.3f }, polea, 0.06f, 0.06f, 4, COLOR_MADERA_OSCURA_VETA);
    PrismaEjeVeta({ base.x, polea.y + 0.12f, polea.z - 0.15f }, { base.x, polea.y + 0.12f, polea.z + 0.15f }, 0.14f, 0.14f, 10, COLOR_HIERRO_VETA);
    float yBalde = base.y - 3.5f + 4.3f * sube;
    PrismaEjeVeta({ polea.x, polea.y - 0.1f, polea.z - 0.14f }, { polea.x, yBalde + 0.5f, polea.z - 0.14f }, 0.02f, 0.02f, 4, Color{ 176, 146, 96, 255 });
    PrismaEjeVeta({ polea.x, yBalde, polea.z - 0.14f }, { polea.x, yBalde + 0.45f, polea.z - 0.14f }, 0.2f, 0.26f, 10, Color{ 104, 110, 124, 255 });
    RacimoCristalVeta({ polea.x, yBalde + 0.4f, polea.z - 0.14f }, { 0.0f, 1.0f, 0.0f }, 0.3f, COLOR_CRISTAL_AZUL_VETA, 91);
}


// Senal de via: poste con dos lamparas (verde libre, roja alternando en
// el aviso y el paso de la vagoneta). Devuelve donde va la luz.
static void SenalViaVeta(Vector3 base, float alto, bool peligro, float t)
{
    CuboVeta({ base.x, base.y + alto * 0.5f, base.z }, { 0.12f, alto, 0.12f }, Color{ 60, 62, 72, 255 });
    CuboVeta({ base.x, base.y + alto + 0.2f, base.z }, { 0.3f, 0.6f, 0.2f }, Color{ 30, 30, 36, 255 });

    bool alterna = std::sin(t * 12.0f) > 0.0f;
    Color rojaApagada = { 90, 30, 30, 255 };
    Color verdeApagada = { 30, 70, 40, 255 };
    Color roja = peligro ? (alterna ? Color{ 255, 70, 50, 255 } : rojaApagada) : rojaApagada;
    Color verde = peligro ? verdeApagada : Color{ 90, 255, 130, 255 };

    CuboVeta({ base.x, base.y + alto + 0.34f, base.z + 0.11f }, { 0.16f, 0.16f, 0.04f }, roja);
    CuboVeta({ base.x, base.y + alto + 0.06f, base.z + 0.11f }, { 0.16f, 0.16f, 0.04f }, verde);
}


static bool PeligroViaVeta(const MinijuegoVetaCristal& m)
{
    return m.avisoVagoneta || m.vagonetaActiva;
}


// Bandera de equipo en un mastil del borde del abismo.
static void BanderaEquipoVeta(Vector3 base, int equipo, float t)
{
    Color color = ObtenerColorEquipoVeta(equipo);
    float s = equipo == 0 ? 1.0f : -1.0f;   // ondea hacia el centro
    float alto = 4.2f;
    PrismaEjeVeta(base, { base.x, base.y + alto, base.z }, 0.06f, 0.05f, 6, COLOR_MADERA_OSCURA_VETA);
    CuboVeta({ base.x, base.y + alto + 0.08f, base.z }, { 0.16f, 0.16f, 0.16f }, COLOR_ORO_VETA);

    const int TRAMOS = 8;
    float largo = 1.7f;
    float movimiento = ObtenerFactorMovimientoCamaraMinijuegos();

    rlBegin(RL_QUADS);
    for (int k = 0; k < TRAMOS; k++)
    {
        float u0 = (float)k / (float)TRAMOS;
        float u1 = (float)(k + 1) / (float)TRAMOS;
        float o0 = std::sin(t * 4.0f - u0 * 5.0f) * 0.16f * u0 * movimiento;
        float o1 = std::sin(t * 4.0f - u1 * 5.0f) * 0.16f * u1 * movimiento;
        float x0 = base.x + s * u0 * largo;
        float x1 = base.x + s * u1 * largo;
        float y0 = base.y + alto - 0.05f;
        float y1 = y0 - 1.0f + 0.15f * u1;
        float y1a = y0 - 1.0f + 0.15f * u0;
        Color c = TonoVeta(color, 0.85f + 0.15f * std::sin(t * 4.0f - u0 * 5.0f));
        QuadVeta({ x0, y1a, base.z + o0 }, { x1, y1, base.z + o1 }, { x1, y0, base.z + o1 }, { x0, y0, base.z + o0 }, c);

        // Franja clara con el rombo de la gema.
        if (k == 3 || k == 4)
        {
            Color claro = MezclaVeta(color, WHITE, 0.75f);
            float ym = (y0 + y1) * 0.5f;
            QuadVeta({ x0, ym - 0.18f, base.z + o0 + 0.01f }, { x1, ym - 0.18f, base.z + o1 + 0.01f }, { x1, ym + 0.18f, base.z + o1 + 0.01f }, { x0, ym + 0.18f, base.z + o0 + 0.01f }, claro);
        }
    }
    rlEnd();
}


// Acopio de cada equipo: vagoneta estacionada que se llena con su puntaje.
static void AcopioEquipoVeta(Vector3 base, int equipo, float puntaje, float t)
{
    (void)t;
    Color color = ObtenerColorEquipoVeta(equipo);
    float s = equipo == 0 ? -1.0f : 1.0f;

    // Tramo de via muerta.
    for (int k = -1; k <= 1; k++)
    {
        CuboVeta({ base.x, 0.05f, base.z + (float)k * 0.55f }, { 1.0f, 0.06f, 0.18f }, COLOR_MADERA_OSCURA_VETA);
    }

    // Caja de chapa con franja del equipo.
    CuboVeta({ base.x, 0.55f, base.z }, { 0.9f, 0.62f, 1.3f }, Color{ 96, 102, 116, 255 });
    CuboVeta({ base.x, 0.88f, base.z }, { 0.98f, 0.06f, 1.38f }, COLOR_HIERRO_VETA);
    CuboVeta({ base.x - s * 0.46f, 0.55f, base.z }, { 0.03f, 0.24f, 1.32f }, color);
    for (int k = -1; k <= 1; k += 2)
    {
        PrismaEjeVeta({ base.x - 0.5f, 0.2f, base.z + (float)k * 0.42f }, { base.x + 0.5f, 0.2f, base.z + (float)k * 0.42f }, 0.17f, 0.17f, 10, Color{ 46, 46, 54, 255 });
    }

    // Cristales que asoman segun el puntaje (lleno a 30 gemas).
    float lleno = Limitar01Veta(puntaje / 30.0f);
    int cristales = (int)(lleno * 9.0f + 0.5f);
    for (int k = 0; k < cristales; k++)
    {
        float cx = base.x + (Ruido01Veta(k, 900 + equipo) - 0.5f) * 0.6f;
        float cz = base.z + (Ruido01Veta(k, 902 + equipo) - 0.5f) * 1.0f;
        Color c = k % 4 == 3 ? COLOR_ORO_VETA : (k % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA);
        CristalVeta({ cx, 0.65f, cz }, { (Ruido01Veta(k, 904) - 0.5f) * 0.6f, 1.0f, (Ruido01Veta(k, 905) - 0.5f) * 0.6f }, 0.35f + 0.25f * lleno, 0.08f, c, (float)k);
    }
}


static void DibujarUtileriaVeta(const MinijuegoVetaCristal& m, float t, int paso)
{
    // Borde del abismo: torno, barandas, senales del puente y banderas.
    TornoVeta({ -6.0f, 0.0f, -8.7f }, t);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;

        // Baranda de postes y soga sobre el abismo.
        for (int k = 0; k < 7; k++)
        {
            float x = s * (2.2f + (float)k * 1.6f);
            CuboVeta({ x, 0.5f, -9.4f }, { 0.14f, 1.0f, 0.14f }, COLOR_MADERA_VETA);
        }
        PrismaEjeVeta({ s * 2.2f, 0.85f, -9.4f }, { s * 11.8f, 0.85f, -9.4f }, 0.04f, 0.04f, 4, Color{ 176, 146, 96, 255 });
        PrismaEjeVeta({ s * 2.2f, 0.45f, -9.4f }, { s * 11.8f, 0.45f, -9.4f }, 0.04f, 0.04f, 4, Color{ 176, 146, 96, 255 });

        // Portico de entrada al puente.
        CuboVeta({ s * 1.75f, 1.7f, -9.3f }, { 0.32f, 3.4f, 0.32f }, COLOR_MADERA_VETA);
        SenalViaVeta({ s * 2.15f, 0.0f, -8.6f }, 1.6f, PeligroViaVeta(m), t + (float)lado * 0.04f);
        SenalViaVeta({ s * 1.9f, 0.0f, 8.55f }, 0.7f, PeligroViaVeta(m), t);

        // Bandera de cada equipo sobre su mitad.
        BanderaEquipoVeta({ s * 10.4f, 0.0f, -9.0f }, lado < 0 ? 0 : 1, t + (float)lado);
    }

    CuboVeta({ 0.0f, 3.5f, -9.3f }, { 4.2f, 0.36f, 0.4f }, COLOR_MADERA_OSCURA_VETA);
    CuboVeta({ 0.0f, 2.98f, -9.12f }, { 1.6f, 0.5f, 0.06f }, Color{ 196, 160, 104, 255 });
    CuboVeta({ -0.4f, 2.98f, -9.08f }, { 0.36f, 0.24f, 0.03f }, COLOR_CRISTAL_AZUL_VETA);
    CuboVeta({ 0.4f, 2.98f, -9.08f }, { 0.36f, 0.24f, 0.03f }, COLOR_CRISTAL_VIOLETA_VETA);

    // Grua de pluma con un cubo de cristales que se mece sobre el abismo.
    {
        Vector3 b = { 6.2f, 0.0f, -8.8f };
        float vaiven = std::sin(t * 0.9f) * 0.25f * ObtenerFactorMovimientoCamaraMinijuegos();
        CuboVeta({ b.x, 1.6f, b.z }, { 0.3f, 3.2f, 0.3f }, COLOR_MADERA_VETA);
        PrismaEjeVeta({ b.x, 3.0f, b.z }, { b.x + 0.4f, 3.3f, b.z - 2.6f }, 0.08f, 0.08f, 4, COLOR_MADERA_OSCURA_VETA, 0.785f);
        PrismaEjeVeta({ b.x, 1.6f, b.z }, { b.x + 0.4f, 3.3f, b.z - 2.6f }, 0.05f, 0.05f, 4, COLOR_MADERA_VETA, 0.785f);
        Vector3 punta = { b.x + 0.4f, 3.25f, b.z - 2.6f };
        Vector3 cubo = { punta.x + vaiven, 1.4f, punta.z + vaiven * 0.4f };
        PrismaEjeVeta(punta, { cubo.x, cubo.y + 0.45f, cubo.z }, 0.02f, 0.02f, 4, Color{ 176, 146, 96, 255 });
        PrismaEjeVeta({ cubo.x, cubo.y, cubo.z }, { cubo.x, cubo.y + 0.45f, cubo.z }, 0.24f, 0.3f, 10, Color{ 104, 110, 124, 255 });
        RacimoCristalVeta({ cubo.x, cubo.y + 0.4f, cubo.z }, { 0.0f, 1.0f, 0.0f }, 0.36f, COLOR_CRISTAL_VIOLETA_VETA, 93);
        CajonVeta({ b.x + 0.9f, 0.0f, b.z + 0.3f }, 0.55f, 12.0f, COLOR_MADERA_VETA);
    }

    // Acopios de equipo junto a cada pared (centro de cada mitad).
    AcopioEquipoVeta({ -11.95f, 0.0f, 0.0f }, 0, m.marcadorVisual[0], t);
    AcopioEquipoVeta({ 11.95f, 0.0f, 0.0f }, 1, m.marcadorVisual[1], t);

    // Franjas laterales entre postes.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        CajonVeta({ s * 11.9f, 0.0f, -4.4f }, 0.6f, 10.0f * s, COLOR_MADERA_VETA);
        CajonVeta({ s * 11.85f, 0.6f, -4.35f }, 0.42f, -8.0f * s, MezclaVeta(COLOR_MADERA_VETA, COLOR_MADERA_OSCURA_VETA, 0.4f));
        BarrilVeta({ s * 11.95f, 0.0f, 4.4f }, 0.9f, COLOR_MADERA_VETA);
        if (paso == 1)
        {
            PicoVeta({ s * 12.1f, 0.0f, -3.2f }, lado < 0 ? 0.3f : 2.8f);
            SacoVeta({ s * 11.9f, 0.0f, 5.5f }, 0.9f, 31 + lado);
            PalaVeta({ s * 12.15f, 0.0f, 3.4f }, lado < 0 ? -0.2f : 3.3f);
            PilaRocasVeta({ s * 11.95f, 0.0f, -7.2f }, 0.8f, 51 + lado, true);
        }
    }

    // Frente: utileria baja que no tapa la arena.
    BarrilVeta({ -9.4f, 0.0f, 8.8f }, 0.9f, COLOR_MADERA_VETA);
    BarrilVeta({ -8.7f, 0.0f, 9.2f }, 0.8f, MezclaVeta(COLOR_MADERA_VETA, COLOR_MADERA_OSCURA_VETA, 0.5f));
    CajonVeta({ 8.6f, 0.0f, 8.9f }, 0.7f, 18.0f, COLOR_MADERA_VETA);
    CajonVeta({ 9.5f, 0.0f, 9.1f }, 0.55f, -10.0f, MezclaVeta(COLOR_MADERA_VETA, COLOR_MADERA_OSCURA_VETA, 0.4f));
    PilaRocasVeta({ -5.2f, 0.0f, 9.0f }, 1.0f, 61, true);
    PilaRocasVeta({ 4.6f, 0.0f, 9.3f }, 0.9f, 62, true);

    if (paso == 1)
    {
        CarretillaVeta({ -11.6f, 0.0f, 9.0f }, 30.0f);
        RolloSogaVeta({ 3.2f, 0.0f, 8.7f });
        PicoVeta({ 6.6f, 0.0f, 8.5f }, 0.6f);
        SacoVeta({ -3.4f, 0.0f, 8.9f }, 1.0f, 71);
        SacoVeta({ -2.8f, 0.0f, 9.4f }, 0.8f, 72);
        PalaVeta({ 10.6f, 0.0f, 8.6f }, 2.2f);
        BarrilVeta({ -10.6f, 0.0f, -8.6f }, 0.8f, COLOR_MADERA_VETA);
        CajonVeta({ 9.0f, 0.0f, -8.6f }, 0.6f, 25.0f, COLOR_MADERA_VETA);
        SacoVeta({ -3.6f, 0.0f, -8.7f }, 0.9f, 73);
    }
}


//==================================================
// VISUAL: PIEZAS JUGABLES (GEODAS, GEMAS, VAGONETA)
//==================================================

// Grieta que recorre la cascara: puntos sobre la esfera, algo por fuera.
static void GrietaGeodaVeta(Vector3 c, float rx, float ry, int semilla, float largo, Color color)
{
    float th = -0.2f + 0.9f * Ruido01Veta(semilla, 960);
    float ph = 0.6f + 1.9f * Ruido01Veta(semilla, 961);   // lado de la camara
    int tramos = 2 + (int)(largo * 4.0f);

    rlBegin(RL_QUADS);
    for (int k = 0; k < tramos; k++)
    {
        float th2 = th + (Ruido01Veta(semilla * 7 + k, 962) - 0.5f) * 0.6f;
        float ph2 = ph + (Ruido01Veta(semilla * 7 + k, 963) - 0.3f) * 0.5f;
        Vector3 a = { c.x + std::cos(th) * std::cos(ph) * rx * 1.06f, c.y + std::sin(th) * ry * 1.06f, c.z + std::cos(th) * std::sin(ph) * rx * 1.06f };
        Vector3 b = { c.x + std::cos(th2) * std::cos(ph2) * rx * 1.06f, c.y + std::sin(th2) * ry * 1.06f, c.z + std::cos(th2) * std::sin(ph2) * rx * 1.06f };
        float g = 0.035f * (1.0f - (float)k / (float)tramos) + 0.012f;
        QuadVeta({ a.x, a.y - g, a.z }, { b.x, b.y - g, b.z }, { b.x, b.y + g, b.z }, { a.x, a.y + g, a.z }, color);
        th = th2;
        ph = ph2;
    }
    rlEnd();
}


// Grieta en zigzag sobre la cara frontal (+z) de la geoda GLB: baja desde
// el borde de arriba y se abre en una rama.
static void GrietaCaraVeta(Vector3 c, float mitad, int semilla, float largo, Color color)
{
    float zCara = c.z + mitad * 0.95f;
    float x = c.x + (Ruido01Veta(semilla, 970) - 0.5f) * mitad * 1.2f;
    float y = c.y + mitad * 0.85f;
    int tramos = 2 + (int)(largo * 3.0f);
    float paso = mitad * 1.4f / (float)(tramos + 1);

    rlBegin(RL_QUADS);
    for (int k = 0; k < tramos; k++)
    {
        float x2 = x + (Ruido01Veta(semilla * 7 + k, 971) - 0.5f) * mitad * 0.5f;
        float y2 = y - paso;
        float g = 0.03f * (1.0f - (float)k / (float)tramos) + 0.012f;
        QuadVeta({ x - g, y, zCara }, { x2 - g, y2, zCara }, { x2 + g, y2, zCara }, { x + g, y, zCara }, color);

        if (k == 1)
        {
            float xr = x2 + mitad * 0.3f * (Ruido01Veta(semilla, 972) < 0.5f ? -1.0f : 1.0f);
            QuadVeta({ x2 - 0.012f, y2, zCara }, { xr - 0.012f, y2 - paso * 0.7f, zCara }, { xr + 0.012f, y2 - paso * 0.7f, zCara }, { x2 + 0.012f, y2, zCara }, color);
        }

        x = x2;
        y = y2;
    }
    rlEnd();
}


// Geoda colgada: cascara de roca facetada en una cuna de hierro, con
// cristales que asoman. Cargada brilla; agotada queda abierta y oscura
// con los cristales volviendo a crecer. La cara de abajo coincide con la
// base del bloque logico (BASE_GEODA_VETA).
// MODELO FUTURO: geoda pequena, geoda grande y geoda abierta.
static void DibujarGeodaVeta(const MinijuegoVetaCristal& m, int indice, float t, float aparicion)
{
    const GeodaVeta& geoda = m.geodas[indice];
    float mitad = geoda.grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;
    float sacudida = geoda.tiempoSacudida > 0.0f
        ? std::sin(t * 60.0f) * 0.07f * (geoda.tiempoSacudida / DURACION_SACUDIDA_VETA)
        : 0.0f;
    float recargaTotal = geoda.grande ? RECARGA_GEODA_GRANDE_VETA : RECARGA_GEODA_VETA;
    float carga = geoda.cargada ? 1.0f : Limitar01Veta(1.0f - geoda.tiempoRecarga / recargaTotal);
    float destello = m.destelloGeoda[indice];
    float grieta = m.grietaGeoda[indice];

    // Al recibir el golpe sube un poco y vuelve.
    float rebote = destello * destello * 0.12f;
    Vector3 c = { geoda.x + sacudida, BASE_GEODA_VETA + mitad + rebote, geoda.z };
    float rx = mitad * 1.1f * aparicion;
    float ry = mitad * aparicion;

    Color cristal = ColorCristalGeodaVeta(geoda.grande);
    Color roca = geoda.grande ? Color{ 134, 112, 150, 255 } : Color{ 118, 106, 148, 255 };
    if (!geoda.cargada) roca = TonoVeta(roca, 0.6f);
    roca = MezclaVeta(roca, WHITE, destello * 0.35f);

    // Cadenas a un gancho y soga que sube a la oscuridad.
    Vector3 gancho = { c.x, c.y + mitad + 0.95f, c.z };
    for (int k = 0; k < 3; k++)
    {
        float a = 0.5f + (float)k * 2.094f;
        Vector3 borde = { c.x + std::cos(a) * rx * 0.95f, c.y + 0.05f, c.z + std::sin(a) * rx * 0.95f };
        CadenaVeta(borde, gancho, 0.035f, COLOR_HIERRO_VETA);
    }
    PrismaEjeVeta(gancho, { geoda.x, gancho.y + 1.1f, geoda.z }, 0.035f, 0.035f, 4, Color{ 150, 124, 84, 255 });
    CuboVeta(gancho, { 0.14f, 0.14f, 0.14f }, COLOR_HIERRO_VETA);

    Color colorBrote = MezclaVeta(TonoVeta(cristal, 0.6f), Color{ 120, 120, 140, 255 }, 0.5f * (1.0f - carga));

    // Pieza GLB centrada en el bloque logico. La agotada comparte malla
    // entre la pequena y la grande y se escala alrededor del centro.
    bool modelo = false;
    if (geoda.cargada)
    {
        modelo = DibujarModeloVetaCristalRetro3D(
            geoda.grande ? MODELO_VETA_GEODA_GRANDE : MODELO_VETA_GEODA_PEQUENA,
            c, 0.0f, { aparicion, aparicion, aparicion });
    }
    else
    {
        float escala = mitad / MITAD_GEODA_VETA * aparicion;
        modelo = DibujarModeloVetaCristalRetro3D(MODELO_VETA_GEODA_AGOTADA, c, 0.0f, { escala, escala, escala });
    }

    if (modelo)
    {
        // Agotada: el cristal vuelve a crecer encima mientras recarga.
        if (!geoda.cargada && carga > 0.15f)
        {
            CristalVeta({ c.x, c.y + mitad * 0.8f, c.z }, { 0.1f, 1.0f, 0.15f }, mitad * 0.75f * carga, mitad * 0.2f, colorBrote, 0.5f);
        }

        // Grietas sobre la cara que mira a la camara.
        if (grieta > 0.02f)
        {
            Color colorGrieta = geoda.cargada ? MezclaVeta(cristal, WHITE, 0.45f) : Color{ 40, 32, 52, 255 };
            int cantidad = 1 + (int)(grieta * 3.0f);

            for (int k = 0; k < cantidad; k++)
            {
                GrietaCaraVeta(c, mitad * aparicion, indice * 5 + k, grieta, colorGrieta);
            }
        }

        return;
    }

    // Sin GLB: cascara de roca facetada, cuna de hierro y halo.
    RocaVeta(c, { rx, ry, rx }, indice * 17 + 3, roca, 0.08f);
    PrismaEjeVeta({ c.x, c.y - 0.08f, c.z }, { c.x, c.y + 0.08f, c.z }, rx * 1.04f, rx * 1.04f, 10, COLOR_HIERRO_VETA);

    // Cristales que asoman: cargada, grandes y vivos; agotada, brotes.
    int cristales = geoda.grande ? 9 : 7;
    float crecimiento = geoda.cargada ? 1.0f : 0.15f + 0.6f * carga * carga;
    Color colorCristal = geoda.cargada ? cristal : colorBrote;
    colorCristal = MezclaVeta(colorCristal, WHITE, destello * 0.5f);

    for (int k = 0; k < cristales; k++)
    {
        float a = (float)k * 6.2832f / (float)cristales + Ruido01Veta(k, indice + 940) * 0.5f;
        float e = -0.35f + 0.9f * Ruido01Veta(k, indice + 941);
        Vector3 d = { std::cos(e) * std::cos(a), std::sin(e), std::cos(e) * std::sin(a) };
        Vector3 pie = { c.x + d.x * rx * 0.8f, c.y + d.y * ry * 0.8f, c.z + d.z * rx * 0.8f };
        float largo = mitad * (0.45f + 0.4f * Ruido01Veta(k, indice + 942)) * crecimiento * aparicion;
        Color cc = (geoda.grande && k % 3 == 1) ? MezclaVeta(COLOR_CRISTAL_VIOLETA_VETA, WHITE, destello * 0.5f) : colorCristal;
        CristalVeta(pie, d, largo, mitad * 0.15f * aparicion, cc, a);
    }

    // Corona de cristal arriba: se ve desde la camara y dice «cargada».
    if (geoda.cargada)
    {
        Vector3 cima = { c.x, c.y + ry * 0.7f, c.z };
        CristalVeta(cima, { 0.0f, 1.0f, 0.15f }, mitad * 0.95f * aparicion, mitad * 0.24f * aparicion, MezclaVeta(cristal, WHITE, 0.2f + destello * 0.4f), (float)indice);
        CristalVeta({ cima.x + rx * 0.3f, cima.y - 0.05f, cima.z }, { 0.5f, 1.0f, 0.2f }, mitad * 0.6f * aparicion, mitad * 0.17f * aparicion, colorCristal, 1.0f);
        CristalVeta({ cima.x - rx * 0.3f, cima.y - 0.05f, cima.z + 0.05f }, { -0.5f, 1.0f, 0.3f }, mitad * 0.55f * aparicion, mitad * 0.16f * aparicion, geoda.grande ? COLOR_CRISTAL_VIOLETA_VETA : colorCristal, 2.0f);
    }
    else
    {
        // Boca abierta y vacia: hueco oscuro con brotes en el fondo.
        Vector3 boca = { c.x, c.y + ry * 0.82f, c.z };
        PrismaEjeVeta({ boca.x, boca.y - 0.02f, boca.z }, { boca.x, boca.y + 0.03f, boca.z }, rx * 0.62f, rx * 0.66f, 9, Color{ 34, 28, 46, 255 }, (float)indice);
        if (carga > 0.25f)
        {
            CristalVeta({ boca.x, boca.y, boca.z }, { 0.1f, 1.0f, 0.1f }, mitad * 0.6f * carga, mitad * 0.18f, colorCristal, 0.5f);
        }
    }

    // Grietas: tras un golpe a la grande y siempre en la geoda abierta.
    if (grieta > 0.02f)
    {
        Color colorGrieta = geoda.cargada
            ? MezclaVeta(cristal, WHITE, 0.4f)
            : Color{ 40, 32, 52, 255 };
        int cantidad = 1 + (int)(grieta * 3.0f);

        for (int k = 0; k < cantidad; k++)
        {
            GrietaGeodaVeta(c, rx, ry, indice * 5 + k, grieta, colorGrieta);
        }
    }

    (DrawSphere)(c, mitad * 1.05f * aparicion, Fade(geoda.cargada ? cristal : Color{ 120, 110, 150, 255 }, geoda.cargada ? 0.16f : 0.08f));
}


// Marca de suelo del GLB bajo cada geoda cargada (anillo que late; la
// grande lleva un segundo anillo). Sin GLB, circulos de primitivas.
static void DibujarMarcaModeloVeta(const GeodaVeta& geoda, float t)
{
    if (!geoda.cargada)
    {
        return;
    }

    Color cristal = ColorCristalGeodaVeta(geoda.grande);
    float pulso = 0.5f + 0.5f * std::sin(t * 3.0f + geoda.x);
    float escalaMarca = (0.55f + 0.08f * pulso) / 0.55f;

    if (!DibujarModeloVetaCristalRetro3D(MODELO_VETA_MARCA, { geoda.x, 0.04f, geoda.z }, 0.0f,
        { escalaMarca, 1.0f, escalaMarca }, Fade(cristal, 0.8f)))
    {
        DrawCircle3D({ geoda.x, 0.04f, geoda.z }, 0.55f + 0.08f * pulso, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(cristal, 0.8f));
    }

    if (geoda.grande)
    {
        float escalaGrande = 0.85f / 0.55f;
        if (!DibujarModeloVetaCristalRetro3D(MODELO_VETA_MARCA, { geoda.x, 0.05f, geoda.z }, 0.0f,
            { escalaGrande, 1.0f, escalaGrande }, Fade(cristal, 0.6f)))
        {
            DrawCircle3D({ geoda.x, 0.05f, geoda.z }, 0.85f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(cristal, 0.6f));
        }
    }
}


// Gema facetada: corona baja y pabellon en punta.
static void GemaFacetadaVeta(Vector3 c, float r, float giro, Color color)
{
    const int LADOS = 6;
    Vector3 arriba = { c.x, c.y + r * 0.55f, c.z };
    Vector3 abajo = { c.x, c.y - r * 1.05f, c.z };
    float yAnillo = c.y + r * 0.1f;

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = giro + 6.2832f * (float)k / (float)LADOS;
        float a1 = giro + 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = { c.x + std::cos(a0) * r, yAnillo, c.z + std::sin(a0) * r };
        Vector3 p1 = { c.x + std::cos(a1) * r, yAnillo, c.z + std::sin(a1) * r };
        float luz = 0.82f + 0.18f * std::sin((a0 + a1) * 0.5f + 1.2f);

        Color corona = MezclaVeta(TonoVeta(color, luz), WHITE, (k & 1) ? 0.12f : 0.32f);
        VerticeVeta(arriba, MezclaVeta(color, WHITE, 0.55f));
        VerticeVeta(p0, corona);
        VerticeVeta(p1, corona);

        Color pabellon = TonoVeta(color, (k & 1) ? 0.62f : 0.8f);
        VerticeVeta(abajo, TonoVeta(color, 0.5f));
        VerticeVeta(p1, pabellon);
        VerticeVeta(p0, pabellon);
    }
    rlEnd();
}


static float RadioGemaVeta(int valor)
{
    return valor >= VALOR_TESORO_SOLITARIO_VETA ? 0.46f : (valor >= VALOR_GEMA_DORADA_VETA ? 0.36f : 0.27f);
}


static bool GemaVisibleVeta(const GemaVeta& gema, float t)
{
    // Parpadeo en los ultimos segundos de vida.
    return !(gema.edad > VIDA_GEMA_VETA - 3.0f && std::sin(t * 18.0f) < 0.0f);
}


static Vector3 PosicionGemaVeta(const GemaVeta& gema, float t)
{
    return { gema.x, gema.y + 0.08f * std::sin(t * 5.0f + gema.x * 2.0f), gema.z };
}


// Gema GLB (azul, dorada o violeta) que gira; sin GLB, gema facetada.
static void DibujarGemaVeta(const GemaVeta& gema, float t)
{
    if (!GemaVisibleVeta(gema, t))
    {
        return;
    }

    float r = RadioGemaVeta(gema.valor);
    Vector3 p = PosicionGemaVeta(gema, t);
    ModeloVetaCristal3D pieza = gema.valor >= VALOR_TESORO_SOLITARIO_VETA
        ? MODELO_VETA_GEMA_VIOLETA
        : (gema.valor >= VALOR_GEMA_DORADA_VETA ? MODELO_VETA_GEMA_DORADA : MODELO_VETA_GEMA_AZUL);

    if (DibujarModeloVetaCristalRetro3D(pieza, p, std::fmod(t * 120.0f + gema.x * 40.0f, 360.0f), { 1.3f, 1.3f, 1.3f }))
    {
        return;
    }

    Color color = ColorGemaVeta(gema.valor);
    GemaFacetadaVeta(p, r, t * 2.2f + gema.x, color);
    (DrawSphere)(p, r * 1.4f, Fade(color, 0.18f));
}


// Vagoneta de mineral cargada de cristales, con farol al frente.
// MODELO FUTURO: vagoneta de la veta.
static void DibujarVagonetaVeta(const MinijuegoVetaCristal& m, float z, float direccion)
{
    float d = direccion < 0.0f ? -1.0f : 1.0f;

    // El GLB trae caja, ruedas, carga y farol; su origen esta al ras del
    // suelo y mira hacia +z.
    if (DibujarModeloVetaCristalRetro3D(MODELO_VETA_VAGONETA, { 0.0f, 0.0f, z }, d < 0.0f ? 180.0f : 0.0f))
    {
        return;
    }

    float giro = m.giroRuedas * 0.0174f;
    Color chapa = { 214, 156, 58, 255 };

    // Chasis y ruedas sobre los rieles (x = +-0.6).
    CuboVeta({ 0.0f, 0.42f, z }, { 1.5f, 0.14f, 1.9f }, COLOR_HIERRO_VETA);
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int eje = -1; eje <= 1; eje += 2)
        {
            float x = (float)lado * 0.6f;
            float zr = z + (float)eje * 0.62f;
            PrismaEjeVeta({ x - (float)lado * 0.08f, 0.3f, zr }, { x + (float)lado * 0.08f, 0.3f, zr }, 0.28f, 0.28f, 12, Color{ 44, 44, 52, 255 }, giro);
            PrismaEjeVeta({ x + (float)lado * 0.08f, 0.3f, zr }, { x + (float)lado * 0.1f, 0.3f, zr }, 0.12f, 0.12f, 6, COLOR_ACERO_VETA, giro);
        }
    }

    // Caja de madera con flejes, mas ancha arriba (ancho real: +-0.95).
    CuboVeta({ 0.0f, 0.82f, z }, { 1.7f, 0.7f, 2.1f }, chapa);

    // Borde de hierro (solo el marco) y el mineral violeta que llena la caja.
    CuboVeta({ -0.88f, 1.21f, z }, { 0.14f, 0.1f, 2.2f }, COLOR_HIERRO_VETA);
    CuboVeta({ 0.88f, 1.21f, z }, { 0.14f, 0.1f, 2.2f }, COLOR_HIERRO_VETA);
    CuboVeta({ 0.0f, 1.21f, z - 1.03f }, { 1.9f, 0.1f, 0.14f }, COLOR_HIERRO_VETA);
    CuboVeta({ 0.0f, 1.21f, z + 1.03f }, { 1.9f, 0.1f, 0.14f }, COLOR_HIERRO_VETA);
    CuboVeta({ 0.0f, 1.19f, z }, { 1.6f, 0.04f, 1.94f }, Color{ 74, 56, 104, 255 });
    for (int k = 0; k < 6; k++)
    {
        float rx = (Ruido01Veta(k, 986) - 0.5f) * 1.1f;
        float rz = z + (Ruido01Veta(k, 987) - 0.5f) * 1.5f;
        RocaVeta({ rx, 1.22f, rz }, { 0.22f, 0.12f, 0.2f }, 5 + k, Color{ 96, 76, 128, 255 }, 0.15f);
    }

    for (int k = -1; k <= 1; k++)
    {
        CuboVeta({ 0.0f, 0.82f, z + (float)k * 0.7f }, { 1.74f, 0.72f, 0.1f }, COLOR_HIERRO_VETA);
    }
    for (int k = 0; k < 3; k++)
    {
        CuboVeta({ 0.0f, 0.6f + (float)k * 0.2f, z }, { 1.72f, 0.02f, 2.12f }, TonoVeta(chapa, 0.8f));
    }

    // Carga: cristales azules y violetas.
    for (int k = 0; k < 9; k++)
    {
        float cx = (Ruido01Veta(k, 980) - 0.5f) * 1.2f;
        float cz = z + (Ruido01Veta(k, 981) - 0.5f) * 1.5f;
        Color c = k % 3 == 0 ? COLOR_CRISTAL_VIOLETA_VETA : COLOR_CRISTAL_AZUL_VETA;
        CristalVeta({ cx, 1.1f, cz }, { (Ruido01Veta(k, 982) - 0.5f) * 0.8f, 1.0f, (Ruido01Veta(k, 983) - 0.5f) * 0.8f }, 0.5f + 0.35f * Ruido01Veta(k, 984), 0.13f, c, (float)k);
    }

    // Farol delantero y paragolpes.
    CuboVeta({ 0.0f, 0.52f, z + d * 1.1f }, { 1.6f, 0.16f, 0.12f }, COLOR_HIERRO_VETA);
    CuboVeta({ 0.0f, 1.02f, z + d * 1.11f }, { 0.34f, 0.3f, 0.14f }, Color{ 50, 50, 58, 255 });
    CuboVeta({ 0.0f, 1.02f, z + d * 1.19f }, { 0.24f, 0.2f, 0.04f }, Color{ 255, 240, 190, 255 });
    (DrawSphere)({ 0.0f, 1.02f, z + d * 1.24f }, 0.2f, Fade(Color{ 255, 236, 170, 255 }, 0.5f));
}


//==================================================
// VISUAL: JUGADORES, TRANSLUCIDOS Y LUCES
//==================================================

static void DibujarChispasSolidasVeta(const MinijuegoVetaCristal& m)
{
    for (int i = 0; i < MAX_CHISPAS_VETA; i++)
    {
        const ChispaVeta& c = m.chispas[i];
        if (!c.activa || c.brilla) continue;

        float vida = Limitar01Veta(c.vida / c.vidaMaxima);
        float tam = c.tamano * (0.5f + 0.5f * vida);
        CuboGiradoVeta(c.posicion, { tam, tam * 0.7f, tam }, c.giro, c.color);
    }
}


// Marca de salto bajo cada geoda: cuadro del tamano exacto de la zona
// donde el cabezazo cuenta (mitad + ancho del jugador).
static void MarcaGeodaVeta(const MinijuegoVetaCristal& m, int indice, float t)
{
    const GeodaVeta& geoda = m.geodas[indice];
    float mitad = geoda.grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;
    float h = mitad + MITAD_ANCHO_JUGADOR_VETA;
    float y = 0.03f;
    Color cristal = ColorCristalGeodaVeta(geoda.grande);
    float recargaTotal = geoda.grande ? RECARGA_GEODA_GRANDE_VETA : RECARGA_GEODA_VETA;
    float carga = geoda.cargada ? 1.0f : Limitar01Veta(1.0f - geoda.tiempoRecarga / recargaTotal);
    float pulso = 0.5f + 0.5f * std::sin(t * 3.0f + (float)indice);

    // Sombra de la geoda sobre el piso.
    ResplandorSueloVeta({ geoda.x, y, geoda.z }, mitad * 1.3f, Color{ 10, 6, 20, 120 });

    Color borde;
    if (geoda.cargada)
    {
        borde = AlfaVeta(cristal, 0.55f + 0.35f * pulso);

        // Coordinacion: tras el primer golpe a la grande, el cuadro titila
        // con el color del equipo que la golpeo.
        if (geoda.grande && geoda.ventanaGolpe > 0.0f && geoda.golpeadorPrevio >= 0)
        {
            int equipo = m.estadosJugadores[geoda.golpeadorPrevio].equipo;
            bool alterna = std::sin(t * 30.0f) > 0.0f;
            borde = alterna ? ColorEquipoVisualVeta(equipo) : WHITE;
        }

        rlBegin(RL_QUADS);
        Color relleno = AlfaVeta(cristal, 0.1f + 0.06f * pulso);
        QuadVeta({ geoda.x - h, y, geoda.z + h }, { geoda.x + h, y, geoda.z + h }, { geoda.x + h, y, geoda.z - h }, { geoda.x - h, y, geoda.z - h }, relleno);
        rlEnd();
    }
    else
    {
        borde = Color{ 150, 146, 168, 110 };
    }

    // Borde del cuadro: en recarga se enciende de a poco (perimetro).
    float g = 0.08f;
    Vector3 esquinas[4] =
    {
        { geoda.x - h, y + 0.004f, geoda.z + h },
        { geoda.x + h, y + 0.004f, geoda.z + h },
        { geoda.x + h, y + 0.004f, geoda.z - h },
        { geoda.x - h, y + 0.004f, geoda.z - h }
    };

    for (int lado = 0; lado < 4; lado++)
    {
        Vector3 a = esquinas[lado];
        Vector3 b = esquinas[(lado + 1) % 4];
        TrazoSueloVeta(a.x, a.z, b.x, b.z, y + 0.004f, g, borde);

        if (!geoda.cargada)
        {
            float tramo = Limitar01Veta(carga * 4.0f - (float)lado);
            if (tramo > 0.0f)
            {
                TrazoSueloVeta(a.x, a.z, a.x + (b.x - a.x) * tramo, a.z + (b.z - a.z) * tramo, y + 0.008f, g * 1.4f, AlfaVeta(cristal, 0.9f));
            }
        }
    }

    // Rombo central: «saltá aqui».
    if (geoda.cargada)
    {
        float r = 0.32f + 0.05f * pulso;
        rlBegin(RL_QUADS);
        QuadVeta({ geoda.x, y + 0.006f, geoda.z - r }, { geoda.x + r * 0.7f, y + 0.006f, geoda.z }, { geoda.x, y + 0.006f, geoda.z + r }, { geoda.x - r * 0.7f, y + 0.006f, geoda.z }, AlfaVeta(MezclaVeta(cristal, WHITE, 0.3f), 0.75f));
        rlEnd();
    }
}


static void DibujarTranslucidosVeta(
    const MinijuegoVetaCristal& m,
    const JugadorPrueba jugadores[],
    int limite,
    float t
)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Penumbra al pie del bordillo y de las paredes: da hondura al piso.
    {
        Color oscuro = { 14, 8, 28, 110 };
        Color nada = { 14, 8, 28, 0 };
        rlBegin(RL_QUADS);
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float s = (float)lado;
            VerticeVeta({ s * 11.0f, 0.026f, -7.6f }, oscuro);
            VerticeVeta({ s * 11.0f, 0.026f, 7.6f }, oscuro);
            VerticeVeta({ s * 10.0f, 0.026f, 7.6f }, nada);
            VerticeVeta({ s * 10.0f, 0.026f, -7.6f }, nada);

            VerticeVeta({ s * 1.3f, 0.026f, -7.6f }, oscuro);
            VerticeVeta({ s * 11.0f, 0.026f, -7.6f }, oscuro);
            VerticeVeta({ s * 11.0f, 0.026f, -6.6f }, nada);
            VerticeVeta({ s * 1.3f, 0.026f, -6.6f }, nada);

            VerticeVeta({ s * 12.7f, 0.004f, -7.95f }, oscuro);
            VerticeVeta({ s * 12.7f, 0.004f, 12.0f }, oscuro);
            VerticeVeta({ s * 11.4f, 0.004f, 12.0f }, nada);
            VerticeVeta({ s * 11.4f, 0.004f, -7.95f }, nada);
        }
        rlEnd();
    }

    // Linea del equipo alrededor de su mitad.
    for (int e = 0; e < 2; e++)
    {
        float s = e == 0 ? -1.0f : 1.0f;
        Color c = AlfaVeta(ObtenerColorEquipoVeta(e), 0.75f);
        float xi = s * 1.62f, xe = s * 10.72f;
        float zf = 7.32f, zb = -7.32f;
        TrazoSueloVeta(xi, zb, xe, zb, 0.028f, 0.1f, c);
        TrazoSueloVeta(xi, zf, xe, zf, 0.028f, 0.1f, c);
        TrazoSueloVeta(xe, zb, xe, zf, 0.028f, 0.1f, c);
        TrazoSueloVeta(xi, zb, xi, zf, 0.028f, 0.06f, AlfaVeta(c, 0.35f));
    }

    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        MarcaGeodaVeta(m, g, t);
    }

    // Carril en peligro: se tine de rojo y laten chevrones hacia donde va.
    if (PeligroViaVeta(m))
    {
        float d = m.direccionVagoneta;
        float parpadeo = 0.5f + 0.5f * std::sin(t * 14.0f);
        rlBegin(RL_QUADS);
        Color rojo = { 255, 50, 40, (unsigned char)(50.0f + 70.0f * parpadeo) };
        QuadVeta({ -1.3f, 0.07f, 9.4f }, { 1.3f, 0.07f, 9.4f }, { 1.3f, 0.07f, -9.4f }, { -1.3f, 0.07f, -9.4f }, rojo);
        rlEnd();

        for (int k = 0; k < 10; k++)
        {
            float u = std::fmod((float)k / 10.0f + t * 0.9f, 1.0f);
            float zc = -d * 8.6f + d * u * 17.2f;
            float alfa = std::sin(u * 3.1416f);
            Color c = { 255, 220, 120, (unsigned char)(200.0f * alfa) };
            TrazoSueloVeta(-0.55f, zc - d * 0.35f, 0.0f, zc + d * 0.15f, 0.08f, 0.14f, c);
            TrazoSueloVeta(0.55f, zc - d * 0.35f, 0.0f, zc + d * 0.15f, 0.08f, 0.14f, c);
        }
    }

    // Sombras de las gemas.
    for (int k = 0; k < MAX_GEMAS_VETA; k++)
    {
        const GemaVeta& gema = m.gemas[k];
        if (!gema.activa || !GemaVisibleVeta(gema, t)) continue;
        float r = RadioGemaVeta(gema.valor);
        float altura = Limitar01Veta(gema.y / 3.0f);
        ResplandorSueloVeta({ gema.x, 0.04f, gema.z }, r * (1.2f - 0.4f * altura), Color{ 8, 4, 18, (unsigned char)(150.0f * (1.0f - altura * 0.6f)) });
    }

    // Sombra de la vagoneta.
    if (m.vagonetaVisible || m.vagonetaActiva)
    {
        float z = m.vagonetaActiva ? m.vagonetaZ : m.vagonetaVisualZ;
        if (z > -9.6f)
        {
            rlBegin(RL_QUADS);
            Color sombra = { 8, 4, 18, 120 };
            QuadVeta({ -1.05f, 0.075f, z + 1.2f }, { 1.05f, 0.075f, z + 1.2f }, { 1.05f, 0.075f, z - 1.2f }, { -1.05f, 0.075f, z - 1.2f }, sombra);
            rlEnd();
        }
    }

    // Anillo del equipo bajo cada jugador.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorVeta& estado = m.estadosJugadores[i];
        if (estado.equipo < 0) continue;

        const JugadorPrueba& j = jugadores[i];
        float pies = j.posicion.y - MITAD_ALTO_JUGADOR_VETA;
        float escala = 1.0f - Limitar01Veta(pies / 3.0f) * 0.35f;
        float pulso = 1.0f + 0.05f * std::sin(t * 5.0f + (float)i);
        Color c = ColorEquipoVisualVeta(estado.equipo);
        ResplandorSueloVeta({ j.posicion.x, 0.048f, j.posicion.z }, 0.62f * escala, Color{ 8, 4, 18, 150 });
        AnilloVeta({ j.posicion.x, 0.05f, j.posicion.z }, 0.0f, 0.48f * escala, AlfaVeta(c, 0.22f), 20);
        AnilloVeta({ j.posicion.x, 0.055f, j.posicion.z }, 0.5f * escala * pulso, 0.64f * escala * pulso, c, 24);
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarLucesVeta(
    const MinijuegoVetaCristal& m,
    const JugadorPrueba jugadores[],
    int limite,
    float t,
    bool decoracion
)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Lamparas de minero: halo, cono y charco ambar.
    for (int i = 0; i < CANTIDAD_LAMPARAS_VETA; i++)
    {
        Vector3 l = PosicionLamparaVeta(i, t);
        float parpadeo = 0.88f + 0.08f * std::sin(t * 13.0f + (float)i * 2.3f) + 0.04f * std::sin(t * 31.0f + (float)i);
        ResplandorFrenteVeta(l, 1.2f * parpadeo, Color{ 255, 170, 80, 90 });
        ResplandorFrenteVeta(l, 0.45f * parpadeo, Color{ 255, 230, 170, 170 });
        Vector3 charco = { l.x + (l.x < 0.0f ? 1.0f : -1.0f) * 0.4f, 0.03f, l.z };
        ConoLuzVeta(l, charco, 1.6f, Color{ 255, 190, 110, 22 });
        for (int r = 0; r < 3; r++)
        {
            ResplandorSueloVeta({ charco.x, 0.03f + 0.002f * (float)r, charco.z }, (1.3f + 0.8f * (float)r) * parpadeo, Color{ 255, 168, 84, 44 });
        }
    }

    // Racimos de las paredes: charcos frios azul y violeta.
    if (decoracion)
    {
        const float zs[6] = { -8.6f, -4.4f, -0.9f, 3.3f, 5.6f, 9.4f };
        for (int lado = -1; lado <= 1; lado += 2)
        {
            for (int k = 0; k < 6; k++)
            {
                Color c = (k + lado) % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA;
                float pulso = 0.75f + 0.25f * std::sin(t * 1.8f + (float)k * 1.3f + (float)lado);
                Vector3 p = { (float)lado * 12.3f, 0.04f, zs[k] };
                ResplandorSueloVeta(p, 1.8f * pulso, AlfaVeta(c, 0.12f));
                ResplandorFrenteVeta({ p.x, 1.0f, p.z }, 1.0f * pulso, AlfaVeta(c, 0.16f));
            }
        }
    }

    // Geodas cargadas: brillo propio y charco bajo la marca.
    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        const GeodaVeta& geoda = m.geodas[g];
        float mitad = geoda.grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;
        Color cristal = ColorCristalGeodaVeta(geoda.grande);
        Vector3 c = { geoda.x, BASE_GEODA_VETA + mitad, geoda.z };
        float destello = m.destelloGeoda[g];

        if (geoda.cargada)
        {
            float pulso = 0.8f + 0.2f * std::sin(t * 3.0f + (float)g);
            ResplandorFrenteVeta({ c.x, c.y + mitad * 0.6f, c.z }, mitad * 1.9f * pulso, AlfaVeta(cristal, 0.22f));
            ResplandorSueloVeta({ geoda.x, 0.04f, geoda.z }, mitad * 2.2f, AlfaVeta(cristal, 0.14f * pulso));
        }

        if (destello > 0.0f)
        {
            ResplandorFrenteVeta(c, mitad * (1.6f + 1.4f * (1.0f - destello)), AlfaVeta(MezclaVeta(cristal, WHITE, 0.5f), 0.6f * destello));
        }
    }

    // Gemas: halo y destellito en cruz.
    for (int k = 0; k < MAX_GEMAS_VETA; k++)
    {
        const GemaVeta& gema = m.gemas[k];
        if (!gema.activa || !GemaVisibleVeta(gema, t)) continue;
        Vector3 p = PosicionGemaVeta(gema, t);
        float r = RadioGemaVeta(gema.valor);
        Color c = ColorGemaVeta(gema.valor);
        float titila = 0.6f + 0.4f * std::sin(t * 6.0f + (float)k * 1.7f);
        ResplandorFrenteVeta(p, r * 3.0f, AlfaVeta(c, 0.3f));
        ResplandorVeta({ p.x, p.y + r * 0.3f, p.z }, FRENTE_U_VETA, FRENTE_V_VETA, r * 2.2f * titila, AlfaVeta(WHITE, 0.5f), 0.12f);
        ResplandorVeta({ p.x, p.y + r * 0.3f, p.z }, FRENTE_V_VETA, FRENTE_U_VETA, r * 2.2f * titila, AlfaVeta(WHITE, 0.5f), 0.12f);
        ResplandorSueloVeta({ gema.x, 0.045f, gema.z }, r * 2.4f, AlfaVeta(c, 0.18f));
    }

    // Vagoneta: farol con cono por delante y charco que avanza.
    if (m.vagonetaVisible || m.vagonetaActiva)
    {
        float z = m.vagonetaActiva ? m.vagonetaZ : m.vagonetaVisualZ;
        float d = m.vagonetaActiva ? m.direccionVagoneta : m.vagonetaVisualDireccion;
        Vector3 farol = { 0.0f, 1.02f, z + d * 1.22f };
        ResplandorFrenteVeta(farol, 0.9f, Color{ 255, 230, 170, 200 });
        ConoLuzVeta(farol, { 0.0f, 0.05f, z + d * 5.5f }, 1.8f, Color{ 255, 220, 150, 40 });
        ResplandorSueloVeta({ 0.0f, 0.08f, z + d * 4.2f }, 2.4f, Color{ 255, 210, 140, 60 }, 0.6f);
    }

    // Senales de via en rojo durante el peligro.
    if (PeligroViaVeta(m) && std::sin(t * 12.0f) > 0.0f)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            ResplandorFrenteVeta({ (float)lado * 2.15f, 1.94f, -8.48f }, 0.8f, Color{ 255, 60, 40, 170 });
            ResplandorFrenteVeta({ (float)lado * 1.9f, 1.04f, 8.67f }, 0.6f, Color{ 255, 60, 40, 170 });
            ResplandorSueloVeta({ (float)lado * 1.9f, 0.08f, 8.4f }, 1.2f, Color{ 255, 60, 40, 50 });
        }
    }

    // Destello al juntar gemas.
    for (int i = 0; i < limite; i++)
    {
        float d = m.destelloJugador[i];
        if (m.estadosJugadores[i].equipo < 0 || d <= 0.0f) continue;
        Vector3 p = jugadores[i].posicion;
        Color c = MezclaVeta(ColorEquipoVisualVeta(m.estadosJugadores[i].equipo), WHITE, 0.4f);
        ResplandorFrenteVeta({ p.x, p.y + 0.3f, p.z }, 1.0f + 0.6f * (1.0f - d), AlfaVeta(c, 0.5f * d));
    }

    // Chispas con brillo.
    for (int i = 0; i < MAX_CHISPAS_VETA; i++)
    {
        const ChispaVeta& c = m.chispas[i];
        if (!c.activa || !c.brilla) continue;
        float vida = Limitar01Veta(c.vida / c.vidaMaxima);
        ResplandorFrenteVeta(c.posicion, c.tamano * (1.2f + vida), AlfaVeta(c.color, 0.9f * vida));
    }

    // Motas de cristal que suben del abismo y polvo en el aire.
    int motas = (int)(46.0f * FactorCalidadGrafica(CalidadParticulas()));
    for (int k = 0; k < motas; k++)
    {
        float ciclo = 7.0f + 5.0f * Ruido01Veta(k, 990);
        float u = std::fmod(t / ciclo + Ruido01Veta(k, 991), 1.0f);
        bool abismo = k % 3 != 0;
        Vector3 p;

        if (abismo)
        {
            p = { (Ruido01Veta(k, 992) - 0.5f) * 40.0f + std::sin(t * 0.6f + (float)k) * 0.6f, -14.0f + 16.0f * u, -12.0f - 22.0f * Ruido01Veta(k, 993) };
        }
        else
        {
            p = { (Ruido01Veta(k, 992) - 0.5f) * 22.0f + std::sin(t * 0.7f + (float)k) * 0.4f, 0.4f + 4.0f * u, -7.0f + 15.0f * Ruido01Veta(k, 993) };
        }

        float alfa = std::sin(u * 3.1416f);
        Color c = k % 2 == 0 ? COLOR_CRISTAL_AZUL_VETA : COLOR_CRISTAL_VIOLETA_VETA;
        ResplandorFrenteVeta(p, abismo ? 0.22f : 0.07f, AlfaVeta(c, (abismo ? 0.6f : 0.7f) * alfa));
    }

    // Brillo del fondo del abismo.
    ResplandorVeta({ 0.0f, -16.0f, -26.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 26.0f, Color{ 120, 90, 220, 40 }, 1.6f);

    EndBlendMode();
    rlEnableDepthMask();
}


static void DibujarJugadoresVeta(
    const MinijuegoVetaCristal& m,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    float t,
    bool mostrarDebug
)
{
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorVeta& estado = m.estadosJugadores[i];
        if (estado.equipo < 0) continue;

        Participante visual = participantes[i];
        visual.conectado = true;

        JugadorPrueba dibujado = jugadores[i];
        if (estado.tiempoAturdido > 0.0f)
        {
            dibujado.posicion.x += std::sin(t * 40.0f) * 0.05f * ObtenerFactorMovimientoCamaraMinijuegos();
        }

        // Inmune tras el choque: parpadea.
        bool ocultar = estado.tiempoInmune > 0.0f && estado.tiempoAturdido <= 0.0f && std::sin(t * 30.0f) > 0.6f;
        if (!ocultar)
        {
            DibujarJugadorModeloCompartido(dibujado, visual);
        }

        // Aturdido: estrellitas que giran sobre la cabeza.
        if (estado.tiempoAturdido > 0.0f)
        {
            for (int k = 0; k < 3; k++)
            {
                float a = t * 7.0f + (float)k * 2.094f;
                Vector3 p = { dibujado.posicion.x + std::cos(a) * 0.45f, dibujado.posicion.y + 1.05f, dibujado.posicion.z + std::sin(a) * 0.45f };
                CuboGiradoVeta(p, { 0.14f, 0.14f, 0.04f }, a * 57.3f, Color{ 255, 226, 90, 255 });
                CuboGiradoVeta(p, { 0.04f, 0.14f, 0.14f }, a * 57.3f, Color{ 255, 226, 90, 255 });
            }
        }

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoVetaCristal::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteJugadoresVeta(cantidadMaxima);
    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;


    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    Camera3D vista = camara;
    float aparicion = 1.0f;

    if (fase == FASE_VETA_PREPARACION && resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Limitar01Veta((tiempoPreparacion - 0.6f) / (DURACION_PREPARACION_VETA - 0.6f));
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 4.0f * suave;
        vista.position.z += 5.0f * suave;
        vista.target.z -= 1.5f * suave;

        // Las geodas aparecen en ola con un pequeno rebote.
        float v = Limitar01Veta((DURACION_PREPARACION_VETA - tiempoPreparacion) / 1.4f);
        aparicion = v < 1.0f ? 1.0f - (1.0f - v) * (1.0f - v) * std::cos(v * 9.0f) : 1.0f;
        if (ObtenerFactorMovimientoCamaraMinijuegos() <= 0.0f) aparicion = 1.0f;
        if (aparicion < 0.05f) aparicion = 0.05f;
    }

    DibujarFondoVeta(t);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte y techo: el abismo de cristales y el puente.
    if (decoracion)
    {
        DibujarAbismoVeta(t, decoracion, paso);
    }
    DibujarPuenteVeta(decoracion);

    // Media distancia: paredes de la galeria y entibado.
    DibujarParedesVeta(t, decoracion, paso);
    DibujarEntibadoVeta(t);

    // Arena.
    DibujarSueloVeta(t);
    DibujarViaVeta(*this, t);

    // Anillo cercano.
    if (decoracion)
    {
        DibujarUtileriaVeta(*this, t, paso);
    }

    // Piezas jugables.
    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        float ola = aparicion;
        if (aparicion < 1.0f)
        {
            float retraso = (float)(g % 5) * 0.08f;
            ola = Limitar01Veta(aparicion * 1.3f - retraso);
            if (ola < 0.05f) ola = 0.05f;
        }
        DibujarGeodaVeta(*this, g, t, ola);
        DibujarMarcaModeloVeta(geodas[g], t);

        if (mostrarDebug)
        {
            float mitad = geodas[g].grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;
            DrawCubeWires({ geodas[g].x, BASE_GEODA_VETA + mitad, geodas[g].z }, mitad * 2.0f, mitad * 2.0f, mitad * 2.0f, LIME);
        }
    }

    for (int g = 0; g < MAX_GEMAS_VETA; g++)
    {
        if (gemas[g].activa)
        {
            DibujarGemaVeta(gemas[g], t);
        }
    }

    if (vagonetaActiva)
    {
        DibujarVagonetaVeta(*this, vagonetaZ, direccionVagoneta);
    }
    else if (vagonetaVisible)
    {
        DibujarVagonetaVeta(*this, vagonetaVisualZ, vagonetaVisualDireccion);
    }

    DibujarJugadoresVeta(*this, jugadores, limite, participantes, t, mostrarDebug);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA);
    DibujarChispasSolidasVeta(*this);

    DibujarTranslucidosVeta(*this, jugadores, limite, t);

    if (efectos)
    {
        DibujarLucesVeta(*this, jugadores, limite, t, decoracion);
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
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Rotulos que suben: «+3», «-2», «¡TESORO!».
    for (int i = 0; i < MAX_AVISOS_VETA; i++)
    {
        const AvisoVeta& aviso = avisos[i];
        if (!aviso.activo) continue;

        float total = aviso.tipo == 1 ? DURACION_ANUNCIO_VETA : DURACION_AVISO_VETA;
        float u = 1.0f - aviso.vida / total;
        Vector3 p = { aviso.posicion.x, aviso.posicion.y + u * 1.0f, aviso.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);
        float alfa = aviso.vida < 0.35f ? aviso.vida / 0.35f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        char texto[24];
        Color color = MezclaVeta(ColorEquipoVisualVeta(aviso.equipo), WHITE, 0.25f);
        int token = UI_TEXTO_GRANDE;

        if (aviso.tipo == 0)
        {
            std::snprintf(texto, sizeof(texto), "+%d", aviso.valor);
        }
        else if (aviso.tipo == 2)
        {
            std::snprintf(texto, sizeof(texto), "-%d", aviso.valor);
            color = Color{ 255, 150, 130, 255 };
        }
        else
        {
            std::snprintf(texto, sizeof(texto), "¡TESORO!");
            color = Color{ 255, 214, 110, 255 };
            token = UI_TEXTO_TITULO;
        }

        DibujarTextoMundoContornoUI(texto, s.x, s.y, token, color, alfa);
    }

    // Geoda grande golpeada una vez: pide el segundo cabezazo.
    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        const GeodaVeta& geoda = geodas[g];
        if (!geoda.grande || !geoda.cargada || geoda.ventanaGolpe <= 0.0f || geoda.golpeadorPrevio < 0) continue;

        Vector2 s = GetWorldToScreen({ geoda.x, BASE_GEODA_VETA + MITAD_GEODA_GRANDE_VETA * 2.0f + 0.6f, geoda.z }, vista);
        Color c = MezclaVeta(ColorEquipoVisualVeta(estadosJugadores[geoda.golpeadorPrevio].equipo), WHITE, 0.3f);
        DibujarTextoMundoContornoUI("¡OTRO GOLPE!", s.x, s.y, UI_TEXTO, c, Limitar01Veta(geoda.ventanaGolpe / 0.15f));
    }

    // Anuncios grandes.
    if (tiempoAnuncioInicio >= 0.0f && fase == FASE_VETA_JUGANDO)
    {
        float u = tiempoAnuncioInicio / DURACION_ANUNCIO_VETA;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
        float y = (float)alto * 0.3f - u * Px(16.0f);
        DibujarTextoMundoContornoUI("¡A MINAR!", (float)ancho * 0.5f, y, UI_TEXTO_TITULO, Color{ 140, 226, 255, 255 }, alfa);
        DibujarTextoMundoContornoUI("CABECEÁ LAS GEODAS DE TU LADO", (float)ancho * 0.5f, y + Px(52.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }

    if (fase == FASE_VETA_JUGANDO && avisoVagoneta && !vagonetaActiva)
    {
        // Sobre la boca por donde llega: el puente o el frente.
        float d = direccionVagoneta;
        float zBoca = d > 0.0f ? -8.6f : 7.2f;
        Vector2 s = GetWorldToScreen({ 0.0f, 2.4f, zBoca }, vista);
        float limiteAbajo = (float)(alto - MargenBarraRondaMinijuegos()) - Px(90.0f);
        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.y > limiteAbajo) s.y = limiteAbajo;
        float pulso = 0.75f + 0.25f * std::sin(t * 14.0f);
        DibujarTextoMundoContornoUI("¡VAGONETA!", s.x, s.y, UI_TEXTO_TITULO, Color{ 255, 120, 90, 255 }, pulso);
        DibujarTextoMundoContornoUI("¡SALÍ DE LAS VÍAS!", s.x, s.y + Px(48.0f), UI_TEXTO, UI_TEXTO_MUNDO, pulso);
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)ancho * 0.5f, (float)alto * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "VETA DE CRISTAL",
        "CABECEÁ LAS GEODAS, JUNTÁ GEMAS Y ESQUIVÁ LA VAGONETA",
        resultado
    );

    hud.fase = fase == FASE_VETA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_VETA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_VETA;

    int puntaje[2] = { PuntajeEquipoVeta(*this, 0), PuntajeEquipoVeta(*this, 1) };

    for (int i = 0; i < limite; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        const EstadoJugadorVeta& estado = estadosJugadores[i];

        if (!h.visible || estado.equipo < 0)
        {
            continue;
        }

        int total = puntaje[estado.equipo];
        h.equipo = estado.equipo;
        h.rol = estado.equipo == 0 ? "EQUIPO 1" : "EQUIPO 2";
        std::snprintf(h.valor, sizeof(h.valor), "%d %s", total, total == 1 ? "GEMA" : "GEMAS");
        h.valorNumerico = total;

        if (estado.tiempoAturdido > 0.0f && fase == FASE_VETA_JUGANDO)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡ATURDIDO!");
        }

        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "JUNTÓ %d", estado.gemas);

        if (!EsControladoPorBotVeta(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "MOVÉ: %s  SALTÁ: %s\n2º SALTO: PISOTÓN",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    if (fase == FASE_VETA_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        static char nota[56];
        hud.finPorTiempo = true;

        if (!empate && puntaje[0] == puntaje[1])
        {
            std::snprintf(nota, sizeof(nota), "%d - %d: DESEMPATA LA ÚLTIMA GEMA DORADA", puntaje[0], puntaje[1]);
        }
        else
        {
            std::snprintf(nota, sizeof(nota), "EQUIPO 1  %d - %d  EQUIPO 2", puntaje[0], puntaje[1]);
        }

        hud.notaFinal = nota;

        if (!empate)
        {
            hud.tituloFinal = equipoGanador == 0 ? "¡GANA EL EQUIPO 1!" : "¡GANA EL EQUIPO 2!";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoVetaCristal::ObtenerResultado() const
{
    return resultado;
}
