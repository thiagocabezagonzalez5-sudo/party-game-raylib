#include "Minigames/MinijuegoVetaCristal.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


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
}


void MinijuegoVetaCristal::Reiniciar(
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

void MinijuegoVetaCristal::Actualizar(
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
// VISUAL: ESCENARIO
//==================================================

static Color ColorGemaVeta(int valor)
{
    if (valor >= VALOR_TESORO_SOLITARIO_VETA) return Color{ 214, 120, 255, 255 };
    if (valor >= VALOR_GEMA_DORADA_VETA) return Color{ 255, 205, 60, 255 };
    return Color{ 80, 170, 255, 255 };
}


// Tunel de roca: suelo, paredes, vigas de madera, lamparas y vetas.
// MODELO FUTURO: suelo y paredes de la mina, vigas y postes de madera,
// lamparas colgantes y las vetas brillantes de las paredes (.glb); rieles
// con travesanos; la vagoneta con ruedas y carga de cristales.
static void DibujarTunelVeta(const MinijuegoVetaCristal& minijuego)
{
    float t = minijuego.tiempoAnimacion;
    const Color roca = Color{ 58, 50, 46, 255 };
    const Color madera = Color{ 120, 82, 46, 255 };
    const Color maderaOscura = Color{ 82, 56, 32, 255 };

    // Suelo con una tonalidad suave por equipo.
    DrawCube({ 0.0f, -0.5f, 0.0f }, MITAD_SUELO_X_VETA * 2.0f, 1.0f, MITAD_SUELO_Z_VETA * 2.0f, Color{ 76, 66, 58, 255 });
    DrawCube({ -6.6f, 0.01f, 0.0f }, 10.8f, 0.02f, MITAD_SUELO_Z_VETA * 2.0f, Fade(ObtenerColorEquipoVeta(0), 0.16f));
    DrawCube({ 6.6f, 0.01f, 0.0f }, 10.8f, 0.02f, MITAD_SUELO_Z_VETA * 2.0f, Fade(ObtenerColorEquipoVeta(1), 0.16f));

    for (int i = 0; i < 14; i++)
    {
        DrawCube(
            { -10.0f + Ruido01Veta(i, 1) * 20.0f, 0.02f, -7.0f + Ruido01Veta(i, 2) * 14.0f },
            0.6f + Ruido01Veta(i, 3) * 0.8f,
            0.03f,
            0.5f + Ruido01Veta(i, 4) * 0.7f,
            Color{ 54, 46, 42, 255 }
        );
    }

    // Paredes: fondo, laterales y un borde bajo al frente.
    DrawCube({ 0.0f, 2.8f, -8.6f }, 26.0f, 5.6f, 1.2f, roca);
    DrawCube({ -12.6f, 2.2f, 0.0f }, 1.2f, 4.4f, 17.0f, roca);
    DrawCube({ 12.6f, 2.2f, 0.0f }, 1.2f, 4.4f, 17.0f, roca);
    DrawCube({ 0.0f, 0.25f, 8.5f }, 26.0f, 0.5f, 0.6f, Color{ 48, 42, 38, 255 });

    // Vetas brillantes en las paredes.
    for (int i = 0; i < 26; i++)
    {
        float x = -11.5f + Ruido01Veta(i, 11) * 23.0f;
        float y = 0.7f + Ruido01Veta(i, 12) * 4.3f;
        float pulso = 0.6f + 0.4f * std::sin(t * 2.0f + (float)i);
        int tipo = i % 3;
        Color color = tipo == 0
            ? Color{ 90, 220, 255, 255 }
            : (tipo == 1 ? Color{ 255, 205, 60, 255 } : Color{ 200, 120, 255, 255 });

        DrawSphereEx({ x, y, -7.95f }, 0.12f + Ruido01Veta(i, 13) * 0.14f, 4, 4, Fade(color, pulso));
    }

    for (int i = 0; i < 8; i++)
    {
        float lado = i % 2 == 0 ? -1.0f : 1.0f;
        float z = -7.0f + Ruido01Veta(i, 21) * 14.0f;
        float y = 0.8f + Ruido01Veta(i, 22) * 3.0f;
        Color color = i % 3 == 0 ? Color{ 90, 220, 255, 255 } : Color{ 255, 205, 60, 255 };

        DrawSphereEx({ lado * 11.95f, y, z }, 0.14f + Ruido01Veta(i, 23) * 0.12f, 4, 4, color);
    }

    // Vigas de madera: portico del fondo y postes laterales.
    for (int k = -1; k <= 1; k += 2)
    {
        DrawCube({ k * 9.0f, 2.4f, -7.8f }, 0.5f, 4.8f, 0.5f, madera);
        DrawCube({ k * 3.0f, 2.4f, -7.8f }, 0.5f, 4.8f, 0.5f, madera);
    }

    DrawCube({ 0.0f, 4.8f, -7.8f }, 19.0f, 0.5f, 0.5f, maderaOscura);

    for (int k = -1; k <= 1; k += 2)
    {
        for (int z = -5; z <= 5; z += 5)
        {
            DrawCube({ k * 11.7f, 2.0f, (float)z }, 0.4f, 4.0f, 0.4f, madera);
            DrawCube({ k * 11.7f, 4.1f, (float)z }, 0.5f, 0.3f, 1.4f, maderaOscura);

            // Lampara colgante.
            DrawSphere({ k * 11.0f, 3.7f, (float)z }, 0.2f, Color{ 255, 224, 130, 255 });
            DrawSphere({ k * 11.0f, 3.7f, (float)z }, 0.55f, Fade(Color{ 255, 200, 90, 255 }, 0.18f));
        }
    }

    // Riel central: lecho, travesanos y rieles.
    DrawCube({ 0.0f, 0.015f, 0.0f }, 2.4f, 0.03f, MITAD_SUELO_Z_VETA * 2.0f, Color{ 42, 36, 32, 255 });

    for (int z = -7; z <= 7; z++)
    {
        DrawCube({ 0.0f, 0.07f, (float)z }, 1.9f, 0.08f, 0.28f, maderaOscura);
    }

    DrawCube({ -0.55f, 0.14f, 0.0f }, 0.1f, 0.1f, MITAD_SUELO_Z_VETA * 2.0f, Color{ 150, 154, 162, 255 });
    DrawCube({ 0.55f, 0.14f, 0.0f }, 0.1f, 0.1f, MITAD_SUELO_Z_VETA * 2.0f, Color{ 150, 154, 162, 255 });

    // Aviso del carril peligroso.
    if (minijuego.avisoVagoneta || minijuego.vagonetaActiva)
    {
        float parpadeo = 0.5f + 0.5f * std::sin(t * 14.0f);
        DrawCube(
            { 0.0f, 0.1f, 0.0f },
            2.5f,
            0.05f,
            MITAD_SUELO_Z_VETA * 2.0f,
            Fade(Color{ 255, 50, 40, 255 }, 0.18f + 0.30f * parpadeo)
        );
    }
}


static void DibujarVagonetaVeta(const MinijuegoVetaCristal& minijuego)
{
    if (!minijuego.vagonetaActiva)
    {
        return;
    }

    float z = minijuego.vagonetaZ;
    float d = minijuego.direccionVagoneta;

    DrawCube({ 0.0f, 0.75f, z }, 1.7f, 0.8f, 2.2f, Color{ 128, 92, 56, 255 });
    DrawCubeWires({ 0.0f, 0.75f, z }, 1.7f, 0.8f, 2.2f, Color{ 40, 30, 22, 255 });
    DrawCube({ 0.0f, 0.4f, z }, 1.8f, 0.14f, 2.3f, Color{ 90, 94, 102, 255 });

    for (int k = 0; k < 3; k++)
    {
        DrawSphereEx(
            { -0.4f + 0.4f * (float)k, 1.2f, z + (float)(k - 1) * 0.4f },
            0.22f, 4, 4, ColorGemaVeta(k == 1 ? VALOR_GEMA_DORADA_VETA : VALOR_GEMA_AZUL_VETA)
        );
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube({ lado * 0.85f, 0.25f, z - 0.7f }, 0.14f, 0.34f, 0.34f, Color{ 30, 30, 34, 255 });
        DrawCube({ lado * 0.85f, 0.25f, z + 0.7f }, 0.14f, 0.34f, 0.34f, Color{ 30, 30, 34, 255 });
    }

    DrawSphere({ 0.0f, 0.95f, z + d * 1.2f }, 0.18f, Color{ 255, 240, 160, 255 });
}


static void DibujarGeodaVeta(const GeodaVeta& geoda, float t, bool mostrarDebug)
{
    float mitad = geoda.grande ? MITAD_GEODA_GRANDE_VETA : MITAD_GEODA_VETA;
    float sacudida = geoda.tiempoSacudida > 0.0f
        ? std::sin(t * 60.0f) * 0.07f * (geoda.tiempoSacudida / DURACION_SACUDIDA_VETA)
        : 0.0f;
    Vector3 centro = { geoda.x + sacudida, BASE_GEODA_VETA + mitad, geoda.z };
    Color cristal = geoda.grande ? Color{ 255, 200, 70, 255 } : Color{ 90, 220, 255, 255 };

    if (geoda.cargada)
    {
        float pulso = 0.5f + 0.5f * std::sin(t * 3.0f + geoda.x);

        DrawCube(centro, mitad * 2.0f, mitad * 2.0f, mitad * 2.0f, Color{ 112, 88, 142, 255 });
        DrawSphere(centro, mitad * 0.95f, Fade(cristal, 0.25f + 0.25f * pulso));
        DrawCubeWires(centro, mitad * 2.04f, mitad * 2.04f, mitad * 2.04f, cristal);

        for (int k = 0; k < 4; k++)
        {
            float sx = (k % 2 == 0 ? -1.0f : 1.0f) * mitad;
            float sz = (k < 2 ? -1.0f : 1.0f) * mitad;
            DrawSphereEx({ centro.x + sx, centro.y + mitad * 0.55f, centro.z + sz }, mitad * 0.28f, 4, 4, cristal);
        }

        // Marca en el suelo: aqui se salta.
        DrawCircle3D({ geoda.x, 0.04f, geoda.z }, 0.55f + 0.08f * pulso, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(cristal, 0.8f));

        if (geoda.grande)
        {
            DrawCircle3D({ geoda.x, 0.05f, geoda.z }, 0.85f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(cristal, 0.6f));
        }
    }
    else
    {
        DrawCube(centro, mitad * 2.0f, mitad * 2.0f, mitad * 2.0f, Color{ 66, 62, 70, 255 });
        DrawCubeWires(centro, mitad * 2.04f, mitad * 2.04f, mitad * 2.04f, Color{ 30, 28, 34, 255 });
    }

    if (mostrarDebug)
    {
        DrawCubeWires(centro, mitad * 2.0f, mitad * 2.0f, mitad * 2.0f, LIME);
    }
}


static void DibujarGemaVeta(const GemaVeta& gema, float t)
{
    // Parpadeo en los ultimos segundos de vida.
    if (gema.edad > VIDA_GEMA_VETA - 3.0f && std::sin(t * 18.0f) < 0.0f)
    {
        return;
    }

    float radio = gema.valor >= VALOR_TESORO_SOLITARIO_VETA ? 0.38f : (gema.valor >= VALOR_GEMA_DORADA_VETA ? 0.28f : 0.2f);
    Color color = ColorGemaVeta(gema.valor);
    float flota = gema.y + 0.08f * std::sin(t * 5.0f + gema.x * 2.0f);

    DrawCircle3D({ gema.x, 0.05f, gema.z }, radio, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.4f));
    DrawSphereEx({ gema.x, flota, gema.z }, radio, 4, 4, color);

    if (gema.valor >= VALOR_GEMA_DORADA_VETA)
    {
        DrawSphere({ gema.x, flota, gema.z }, radio * 1.6f, Fade(color, 0.22f));
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

    ClearBackground(Color{ 16, 12, 12, 255 });
    BeginMode3D(camara);

    DibujarTunelVeta(*this);

    for (int g = 0; g < MAX_GEODAS_VETA; g++)
    {
        DibujarGeodaVeta(geodas[g], tiempoAnimacion, mostrarDebug);
    }

    DibujarVagonetaVeta(*this);

    for (int g = 0; g < MAX_GEMAS_VETA; g++)
    {
        if (gemas[g].activa)
        {
            DibujarGemaVeta(gemas[g], tiempoAnimacion);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorVeta& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];
        Color colorEquipo = ObtenerColorEquipoVeta(estado.equipo);

        DrawCircle3D({ jugador.posicion.x, 0.07f, jugador.posicion.z }, 0.72f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);
        DrawCircle3D({ jugador.posicion.x, 0.07f, jugador.posicion.z }, 0.62f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(jugador, participanteVisual);

        if (estado.tiempoAturdido > 0.0f)
        {
            for (int k = 0; k < 3; k++)
            {
                float angulo = tiempoAnimacion * 9.0f + (float)k * 2.094f;
                DrawSphere(
                    {
                        jugador.posicion.x + std::cos(angulo) * 0.5f,
                        jugador.posicion.y + 1.1f,
                        jugador.posicion.z + std::sin(angulo) * 0.5f
                    },
                    0.1f,
                    YELLOW
                );
            }
        }

        if (mostrarDebug)
        {
            DrawCubeWires(jugador.posicion, jugador.tamano.x, jugador.tamano.y, jugador.tamano.z, LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TIERRA);

    EndMode3D();

    // HUD: marcador.
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangle(ancho / 2 - 250, 10, 500, 76, Fade(BLACK, 0.78f));
    DrawRectangle(ancho / 2 - 250, 10, 8, 76, ObtenerColorEquipoVeta(0));
    DrawRectangle(ancho / 2 + 242, 10, 8, 76, ObtenerColorEquipoVeta(1));

    int puntaje0 = PuntajeEquipoVeta(*this, 0);
    int puntaje1 = PuntajeEquipoVeta(*this, 1);

    const char* marcador = TextFormat("%d  -  %d", puntaje0, puntaje1);
    DrawText(marcador, ancho / 2 - MeasureText(marcador, 40) / 2, 14, 40, RAYWHITE);
    DrawText("EQUIPO 1", ancho / 2 - 236, 18, 18, ObtenerColorEquipoVeta(0));
    DrawText("EQUIPO 2", ancho / 2 + 236 - MeasureText("EQUIPO 2", 18), 18, 18, ObtenerColorEquipoVeta(1));

    bool aviso = (avisoVagoneta || vagonetaActiva) && std::sin(tiempoAnimacion * 14.0f) > -0.3f;
    const char* subtitulo = aviso
        ? "CUIDADO: VAGONETA"
        : TextFormat("VETA DE CRISTAL  -  %.0f s", tiempoRestante);

    DrawText(
        subtitulo,
        ancho / 2 - MeasureText(subtitulo, 17) / 2,
        62,
        17,
        aviso ? RED : LIGHTGRAY
    );

    // Gemas que lleva cada jugador, sobre su cabeza.
    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += 1.2f;
        Vector2 pantalla = GetWorldToScreen(cabeza, camara);
        const char* texto = TextFormat("%d", estadosJugadores[i].gemas);

        DrawText(
            texto,
            (int)pantalla.x - MeasureText(texto, 20) / 2,
            (int)pantalla.y - 10,
            20,
            ObtenerColorEquipoVeta(estadosJugadores[i].equipo)
        );
    }

    // HUD: controles de cada jugador.
    int y = alto - 30;

    for (int i = limite - 1; i >= 0; i--)
    {
        if (estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        const EstadoJugadorVeta& estado = estadosJugadores[i];

        const char* linea = EsControladoPorBotVeta(participantes[i])
            ? TextFormat("J%d BOT  EQ%d", participantes[i].numeroJugador, estado.equipo + 1)
            : TextFormat(
                "J%d  EQ%d  SALTA [%s] DEBAJO DE UNA GEODA  2o SALTO EN AIRE: GOLPE AL SUELO",
                participantes[i].numeroJugador,
                estado.equipo + 1,
                ObtenerTextoBotonPrincipal(participantes[i])
            );

        DrawText(linea, 18, y, 17, ObtenerColorEquipoVeta(estado.equipo));
        y -= 22;
    }

    if (fase == FASE_VETA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, ancho / 2 - MeasureText(texto, 90) / 2, alto / 2 - 50, 90, YELLOW);

        const char* pista = "GOLPEA GEODAS CON LA CABEZA  -  GEODA GRANDE: DOS A LA VEZ  -  EVITA LA VAGONETA";
        DrawText(pista, ancho / 2 - MeasureText(pista, 18) / 2, alto / 2 + 60, 18, RAYWHITE);
    }
    else if (
        fase == FASE_VETA_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        DrawRectangle(ancho / 2 - 330, alto / 2 - 90, 660, 180, Fade(BLACK, 0.92f));

        const char* titulo = empate
            ? "EMPATE"
            : TextFormat("GANA EL EQUIPO %d", equipoGanador + 1);

        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, 36) / 2,
            alto / 2 - 62,
            36,
            empate ? YELLOW : ObtenerColorEquipoVeta(equipoGanador)
        );

        const char* final = TextFormat("%d  -  %d", puntaje0, puntaje1);
        DrawText(final, ancho / 2 - MeasureText(final, 34) / 2, alto / 2 - 10, 34, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), ancho / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2, alto / 2 + 44, 18, LIGHTGRAY);
    }
    else if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        const char* texto = "SE NECESITAN AL MENOS 2 JUGADORES";
        DrawText(texto, ancho / 2 - MeasureText(texto, 26) / 2, alto / 2, 26, RED);
    }
}


const ResultadoMinijuego& MinijuegoVetaCristal::ObtenerResultado() const
{
    return resultado;
}
