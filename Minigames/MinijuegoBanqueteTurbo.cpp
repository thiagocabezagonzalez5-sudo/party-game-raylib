#include "Minigames/MinijuegoBanqueteTurbo.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_BANQUETE = 3.0f;
static const float DURACION_PARTIDA_BANQUETE = 30.0f;

// Tope anti-macro: mas de ~14 pulsaciones por segundo no suman.
static const float TOPE_BOCADOS_POR_SEGUNDO_BANQUETE = 14.0f;
static const float LIMITE_SONIDO_BOCADO_BANQUETE = 0.125f;

static const float DURACION_LLEGADA_BANQUETE = 0.45f;
static const float DURACION_TOS_BANQUETE = 1.2f;
static const float DURACION_RECHAZO_BANQUETE = 0.4f;

static const int PROBABILIDAD_PICANTE_BANQUETE = 20;
static const int PROBABILIDAD_DORADA_BANQUETE = 12;

static const float SEPARACION_PUESTOS_BANQUETE = 4.2f;
static const float ZONA_JUGADOR_BANQUETE = -0.3f;
static const float ZONA_MESA_BANQUETE = 1.3f;
static const float ALTURA_BANDEJA_BANQUETE = 0.98f;
static const float LARGO_TUBO_BANQUETE = 1.6f;

static const int CANTIDAD_COLORES_TUBO_BANQUETE = 6;


//==================================================
// UTILIDADES
//==================================================

static float LimitarBanquete(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioBanquete(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static int LimiteBanquete(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static const char* NombreJugadorBanquete(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static const char* TextoBocadoBanquete(const Participante& participante)
{
    if (participante.control == CONTROL_GAMEPAD)
    {
        return "B";
    }

    if (participante.control == CONTROL_TECLADO_FLECHAS)
    {
        return "SHIFT DER";
    }

    return "E";
}


static Color ColorTuboBanquete(int indice)
{
    const Color paleta[CANTIDAD_COLORES_TUBO_BANQUETE] =
    {
        Color{ 80, 200, 120, 255 },
        Color{ 90, 160, 255, 255 },
        Color{ 190, 120, 255, 255 },
        Color{ 80, 230, 230, 255 },
        Color{ 255, 130, 200, 255 },
        Color{ 170, 230, 90, 255 }
    };

    return paleta[indice % CANTIDAD_COLORES_TUBO_BANQUETE];
}


static Color ColorRacionBanquete(const RacionBanquete& racion)
{
    if (racion.tipo == RACION_BANQUETE_DORADA) return Color{ 255, 205, 50, 255 };
    if (racion.tipo == RACION_BANQUETE_PICANTE) return Color{ 230, 50, 40, 255 };
    return ColorTuboBanquete(racion.colorTubo);
}


static void AgregarParticulaBanquete(
    MinijuegoBanqueteTurbo& m,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_BANQUETE; i++)
    {
        if (m.particulas[i].activa)
        {
            continue;
        }

        m.particulas[i].activa = true;
        m.particulas[i].posicion = posicion;
        m.particulas[i].velocidad = velocidad;
        m.particulas[i].vida = vida;
        m.particulas[i].vidaMaxima = vida;
        m.particulas[i].tamano = tamano;
        m.particulas[i].color = color;
        return;
    }
}


static void MigajasBanquete(MinijuegoBanqueteTurbo& m, int puesto, Color color, int cantidad, float altura)
{
    for (int k = 0; k < cantidad; k++)
    {
        AgregarParticulaBanquete(
            m,
            { m.posicionX[puesto] + AleatorioBanquete(-0.3f, 0.3f), altura, ZONA_MESA_BANQUETE - 0.3f },
            { AleatorioBanquete(-1.2f, 1.2f), AleatorioBanquete(1.2f, 2.8f), AleatorioBanquete(0.3f, 1.4f) },
            AleatorioBanquete(0.3f, 0.6f),
            AleatorioBanquete(0.05f, 0.1f),
            color
        );
    }
}


//==================================================
// LOGICA: RACIONES
//==================================================

static void GenerarSecuenciaBanquete(MinijuegoBanqueteTurbo& m)
{
    // La secuencia es la misma para todos: ninguna mesa tiene mejor suerte.
    for (int k = 0; k < LARGO_SECUENCIA_BANQUETE; k++)
    {
        RacionBanquete& racion = m.secuencia[k];
        int dado = GetRandomValue(0, 99);

        racion.colorTubo = GetRandomValue(0, CANTIDAD_COLORES_TUBO_BANQUETE - 1);

        if (k > 0 && dado < PROBABILIDAD_PICANTE_BANQUETE)
        {
            racion.tipo = RACION_BANQUETE_PICANTE;
            racion.bocados = GetRandomValue(10, 12);
        }
        else if (k > 0 && dado < PROBABILIDAD_PICANTE_BANQUETE + PROBABILIDAD_DORADA_BANQUETE)
        {
            racion.tipo = RACION_BANQUETE_DORADA;
            racion.bocados = GetRandomValue(14, 16);
        }
        else
        {
            racion.tipo = RACION_BANQUETE_NORMAL;
            racion.bocados = GetRandomValue(12, 15);
        }
    }
}


static void ServirRacionBanquete(MinijuegoBanqueteTurbo& m, int puesto)
{
    PuestoBanquete& p = m.puestos[puesto];
    EstadoBotBanquete& bot = m.bots[puesto];

    p.indiceRacion = (p.indiceRacion + 1) % LARGO_SECUENCIA_BANQUETE;

    const RacionBanquete& racion = m.secuencia[p.indiceRacion];
    p.bocadosRestantes = racion.bocados;
    p.bocadosRacion = racion.bocados;
    p.llegada = 0.0f;
    p.tiempoRacion = 0.0f;

    // El bot detecta la picante solo a veces y tarda en reaccionar.
    bot.reaccionActiva = false;

    if (racion.tipo == RACION_BANQUETE_PICANTE && AleatorioBanquete(0.0f, 1.0f) < bot.probabilidadDetectar)
    {
        bot.reaccionActiva = true;
        bot.reaccion = AleatorioBanquete(0.2f, 0.45f);
    }

    m.robotPuesto = puesto;
    m.robotObjetivoX = m.posicionX[puesto];
    m.robotBrazo = 0.5f;
}


static void CompletarRacionBanquete(MinijuegoBanqueteTurbo& m, int puesto)
{
    PuestoBanquete& p = m.puestos[puesto];
    const RacionBanquete& racion = m.secuencia[p.indiceRacion];
    Color color = ColorRacionBanquete(racion);

    p.raciones++;
    p.tiempoMensaje = 0.8f;
    MigajasBanquete(m, puesto, color, 12, ALTURA_BANDEJA_BANQUETE + 0.2f);

    if (racion.tipo == RACION_BANQUETE_DORADA)
    {
        p.puntos += 2;
        p.doradas++;
        ReproducirSonidoMinijuego(m.audio, SONIDO_RECOGER_NUCLEO_ESPECIAL);
    }
    else
    {
        p.puntos += 1;
        ReproducirSonidoMinijuego(m.audio, SONIDO_RECOGER_OBJETO);
    }

    if (racion.tipo == RACION_BANQUETE_PICANTE)
    {
        // Pica: tos durante 1.2 s sin poder comer.
        p.tos = DURACION_TOS_BANQUETE;
        p.picantesComidas++;
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);

        for (int k = 0; k < 8; k++)
        {
            AgregarParticulaBanquete(
                m,
                { m.posicionX[puesto], 1.9f, ZONA_JUGADOR_BANQUETE + 0.5f },
                { AleatorioBanquete(-1.0f, 1.0f), AleatorioBanquete(0.5f, 1.6f), AleatorioBanquete(0.4f, 1.6f) },
                0.7f,
                0.12f,
                Color{ 200, 200, 210, 255 }
            );
        }
    }

    ServirRacionBanquete(m, puesto);
}


static void RechazarRacionBanquete(MinijuegoBanqueteTurbo& m, int puesto)
{
    PuestoBanquete& p = m.puestos[puesto];
    const RacionBanquete& racion = m.secuencia[p.indiceRacion];

    p.racionSalida = racion;
    p.salida = DURACION_RECHAZO_BANQUETE;
    p.rechazo = DURACION_RECHAZO_BANQUETE;

    if (racion.tipo == RACION_BANQUETE_PICANTE)
    {
        p.picantesRechazadas++;
        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
    }

    MigajasBanquete(m, puesto, ColorRacionBanquete(racion), 5, ALTURA_BANDEJA_BANQUETE + 0.3f);
    ServirRacionBanquete(m, puesto);
}


static void ActualizarPuestoBanquete(
    MinijuegoBanqueteTurbo& m,
    int puesto,
    const InputMinijuegoParticipante& entrada,
    bool humano,
    float deltaTime
)
{
    PuestoBanquete& p = m.puestos[puesto];

    if (p.tos > 0.0f) p.tos -= deltaTime;
    if (p.rechazo > 0.0f) p.rechazo -= deltaTime;
    if (p.enfriamientoBocado > 0.0f) p.enfriamientoBocado -= deltaTime;
    if (p.enfriamientoSonido > 0.0f) p.enfriamientoSonido -= deltaTime;
    if (p.pulsoBocado > 0.0f) p.pulsoBocado -= deltaTime * 6.0f;
    if (p.tiempoMensaje > 0.0f) p.tiempoMensaje -= deltaTime;
    if (p.salida > 0.0f) p.salida -= deltaTime;

    p.tiempoRacion += deltaTime;
    p.llegada = LimitarBanquete(p.llegada + deltaTime / DURACION_LLEGADA_BANQUETE, 0.0f, 1.0f);

    if (p.tos > 0.0f || p.rechazo > 0.0f)
    {
        return;
    }

    // El rechazo tiene prioridad sobre el bocado.
    if (entrada.saltar)
    {
        RechazarRacionBanquete(m, puesto);
        return;
    }

    if (
        entrada.golpear &&
        p.enfriamientoBocado <= 0.0f &&
        p.llegada >= 1.0f
    )
    {
        p.enfriamientoBocado = 1.0f / TOPE_BOCADOS_POR_SEGUNDO_BANQUETE;
        p.pulsoBocado = 1.0f;
        p.bocadosRestantes--;
        p.bocadosTotales++;

        if (humano && p.enfriamientoSonido <= 0.0f)
        {
            p.enfriamientoSonido = LIMITE_SONIDO_BOCADO_BANQUETE;
            ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
        }

        if (GetRandomValue(0, 100) < 35)
        {
            MigajasBanquete(m, puesto, ColorRacionBanquete(m.secuencia[p.indiceRacion]), 1, ALTURA_BANDEJA_BANQUETE + 0.25f);
        }

        if (p.bocadosRestantes <= 0)
        {
            CompletarRacionBanquete(m, puesto);
        }
    }
}


//==================================================
// LOGICA: FIN DE PARTIDA
//==================================================

static bool MejorQueBanquete(const MinijuegoBanqueteTurbo& m, int a, int b)
{
    if (m.puestos[a].puntos != m.puestos[b].puntos)
    {
        return m.puestos[a].puntos > m.puestos[b].puntos;
    }

    return m.puestos[a].bocadosTotales > m.puestos[b].bocadosTotales;
}


static void FinalizarPartidaBanquete(MinijuegoBanqueteTurbo& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        ResultadoParticipante& r = m.resultado.participantes[i];

        if (!r.participo)
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < limite; j++)
        {
            if (j != i && m.resultado.participantes[j].participo && MejorQueBanquete(m, j, i))
            {
                posicion++;
            }
        }

        r.posicionFinal = posicion;
        r.puntuacionMinijuego = m.puestos[i].puntos;
    }

    int ganadores = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo && m.resultado.participantes[i].posicionFinal == 1)
        {
            ganadores++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = ganadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    m.fase = FASE_BANQUETE_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// BOTS
//==================================================

static InputMinijuegoParticipante CrearEntradaBotBanquete(
    MinijuegoBanqueteTurbo& m,
    int indice,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoBotBanquete& bot = m.bots[indice];

    bot.cambioFrecuencia -= deltaTime;

    if (bot.cambioFrecuencia <= 0.0f)
    {
        bot.frecuencia = AleatorioBanquete(7.0f, 11.0f);
        bot.cambioFrecuencia = AleatorioBanquete(0.8f, 1.6f);
    }

    bot.proximoToque -= deltaTime;

    if (bot.proximoToque <= 0.0f)
    {
        entrada.golpear = true;
        bot.proximoToque += AleatorioBanquete(0.75f, 1.3f) / bot.frecuencia;

        // Evita rafagas si el bot quedo muy atrasado.
        if (bot.proximoToque < 0.0f)
        {
            bot.proximoToque = 0.0f;
        }
    }

    if (bot.reaccionActiva)
    {
        bot.reaccion -= deltaTime;

        if (bot.reaccion <= 0.0f)
        {
            bot.reaccionActiva = false;
            entrada.saltar = true;
        }
    }

    return entrada;
}


//==================================================
// CICLO DE VIDA
//==================================================

void MinijuegoBanqueteTurbo::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        puestos[i] = {};
        puestos[i].indiceRacion = -1;
        bots[i] = {};
        bots[i].probabilidadDetectar = AleatorioBanquete(0.6f, 0.8f);
        bots[i].frecuencia = AleatorioBanquete(7.0f, 11.0f);
        bots[i].proximoToque = AleatorioBanquete(0.0f, 0.2f);
        posicionX[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS_BANQUETE; i++)
    {
        particulas[i] = {};
    }

    GenerarSecuenciaBanquete(*this);

    camara.position = { 0.0f, 4.6f, 12.5f };
    camara.target = { 0.0f, 1.5f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 45.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_BANQUETE_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_BANQUETE;
    tiempoRestante = DURACION_PARTIDA_BANQUETE;
    tiempoAnimacion = 0.0f;
    robotX = 0.0f;
    robotObjetivoX = 0.0f;
    robotPuesto = -1;
    robotBrazo = 0.0f;
}


void MinijuegoBanqueteTurbo::Reiniciar(
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

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_BANQUETE_TERMINADO;
        return;
    }

    int limite = LimiteBanquete(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        posicionX[i] = ((float)k - (float)(cantidad - 1) * 0.5f) * SEPARACION_PUESTOS_BANQUETE;

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], { posicionX[i], 1.0f, ZONA_JUGADOR_BANQUETE });
        jugadores[i].direccionMirada = { 0.0f, 0.0f, 1.0f };

        ServirRacionBanquete(*this, i);
    }

    robotPuesto = -1;
    robotX = 0.0f;
    robotObjetivoX = 0.0f;
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoBanqueteTurbo::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_BANQUETE, deltaTime);

    // El robot se desliza hacia el ultimo puesto servido.
    robotX += (robotObjetivoX - robotX) * LimitarBanquete(deltaTime * 6.0f, 0.0f, 1.0f);

    if (robotBrazo > 0.0f)
    {
        robotBrazo -= deltaTime;
    }

    if (
        fase == FASE_BANQUETE_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    int limite = LimiteBanquete(cantidadMaxima);

    // El personaje come: sube un poco con cada bocado y tiembla al toser.
    auto sincronizarJugadores = [&]()
    {
        for (int i = 0; i < limite; i++)
        {
            if (!participantes[i].activo)
            {
                continue;
            }

            JugadorPrueba& jugador = jugadores[i];
            const PuestoBanquete& p = puestos[i];
            float temblor = p.tos > 0.0f ? std::sin(tiempoAnimacion * 60.0f) * 0.06f : 0.0f;
            float masticar = LimitarBanquete(p.pulsoBocado, 0.0f, 1.0f) * 0.1f;

            jugador.posicion = { posicionX[i] + temblor, 1.0f + masticar, ZONA_JUGADOR_BANQUETE };
            jugador.velocidad = {};
            jugador.empuje = {};
            jugador.enSuelo = true;
            jugador.cayendo = false;
            jugador.direccionMirada = { 0.0f, 0.0f, 1.0f };
        }
    };

    if (fase == FASE_BANQUETE_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        for (int i = 0; i < limite; i++)
        {
            if (participantes[i].activo)
            {
                puestos[i].llegada = LimitarBanquete(puestos[i].llegada + deltaTime / DURACION_LLEGADA_BANQUETE, 0.0f, 1.0f);
            }
        }

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_BANQUETE_JUGANDO;
        }

        sincronizarJugadores();
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        bool controlBot = participantes[i].esBot || !participantes[i].conectado;
        InputMinijuegoParticipante entrada{};

        if (controlBot)
        {
            entrada = CrearEntradaBotBanquete(*this, i, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        ActualizarPuestoBanquete(*this, i, entrada, !controlBot, deltaTime);
    }

    sincronizarJugadores();

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarPartidaBanquete(*this, limite);
    }
}


//==================================================
// VISUAL: ESCENARIO (solo decoracion, la logica no depende de esto)
//==================================================

// MODELO FUTURO: ventanal con la Tierra y estrellas, estructura del comedor,
// tuberias del techo, mesas magneticas con bandejas, tubos de comida
// (normal, picante, dorada), robot camarero y taburetes: reemplazar cada uno
// por un GLB.

static float HashBanquete(int semilla)
{
    unsigned int x = (unsigned int)semilla * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return (float)(x & 0xFFFFu) / 65535.0f;
}


static void DibujarFondoBanquete(const MinijuegoBanqueteTurbo& m)
{
    float t = m.tiempoAnimacion;

    // Estrellas tras el ventanal.
    for (int k = 0; k < 70; k++)
    {
        float x = (HashBanquete(k * 3 + 1) * 2.0f - 1.0f) * 42.0f;
        float y = -8.0f + HashBanquete(k * 3 + 2) * 34.0f;
        float brillo = 0.55f + 0.45f * std::sin(t * 2.0f + (float)k);
        DrawSphere({ x, y, -36.0f }, 0.07f + HashBanquete(k * 3 + 3) * 0.1f, Fade(WHITE, brillo));
    }

    // La Tierra.
    const Vector3 centro = { -7.0f, 7.0f, -24.0f };
    const float radio = 10.0f;
    DrawSphere(centro, radio, Color{ 28, 86, 190, 255 });

    for (int k = 0; k < 6; k++)
    {
        float dx = (HashBanquete(k * 5 + 200) * 2.0f - 1.0f) * 0.6f;
        float dy = (HashBanquete(k * 5 + 201) * 2.0f - 1.0f) * 0.6f;
        float dz = std::sqrt(LimitarBanquete(1.0f - dx * dx - dy * dy, 0.0f, 1.0f));
        DrawSphere(
            { centro.x + dx * radio * 0.99f, centro.y + dy * radio * 0.99f, centro.z + dz * radio * 0.99f },
            1.6f + HashBanquete(k + 210) * 1.2f,
            Color{ 70, 150, 70, 255 }
        );
    }

    DrawSphere(centro, radio * 1.05f, Fade(Color{ 120, 190, 255, 255 }, 0.2f));

    // Pared con ventanal.
    Color pared = Color{ 40, 46, 70, 255 };
    Color neon = Color{ 80, 220, 255, 255 };

    DrawCube({ 0.0f, 0.75f, -8.0f }, 34.0f, 1.5f, 0.6f, pared);
    DrawCube({ 0.0f, 10.2f, -8.0f }, 34.0f, 3.4f, 0.6f, pared);
    DrawCube({ -13.5f, 6.0f, -8.0f }, 7.0f, 9.0f, 0.6f, pared);
    DrawCube({ 13.5f, 6.0f, -8.0f }, 7.0f, 9.0f, 0.6f, pared);

    for (int k = -2; k <= 2; k++)
    {
        DrawCube({ (float)k * 4.5f, 6.0f, -7.8f }, 0.25f, 9.0f, 0.3f, Color{ 30, 34, 54, 255 });
    }

    DrawCube({ 0.0f, 1.55f, -7.7f }, 34.0f, 0.08f, 0.1f, Fade(neon, 0.8f));
    DrawCube({ 0.0f, 8.48f, -7.7f }, 20.0f, 0.08f, 0.1f, Fade(neon, 0.6f));

    // Suelo y franjas de luz.
    DrawCube({ 0.0f, -0.25f, -1.0f }, 40.0f, 0.5f, 22.0f, Color{ 34, 38, 58, 255 });

    for (int k = -4; k <= 4; k++)
    {
        DrawCube({ (float)k * 3.0f, 0.005f, -1.0f }, 0.06f, 0.01f, 22.0f, Fade(neon, 0.25f));
    }

    // Tuberias del techo.
    for (int k = 0; k < 3; k++)
    {
        float y = 9.0f - (float)k * 0.7f;
        DrawCylinderEx({ -18.0f, y, -5.0f + (float)k * 0.8f }, { 18.0f, y, -5.0f + (float)k * 0.8f }, 0.22f, 0.22f, 8, Color{ 90, 98, 130, 255 });
    }

    DrawCylinderEx({ -18.0f, 9.0f, -3.4f }, { 18.0f, 9.0f, -3.4f }, 0.06f, 0.06f, 6, Fade(neon, 0.8f));
}


static void DibujarTuboBanquete(
    float x,
    float y,
    float z,
    const RacionBanquete& racion,
    float fraccion,
    float tiempo,
    float escala
)
{
    Color color = ColorRacionBanquete(racion);
    float largo = LARGO_TUBO_BANQUETE * LimitarBanquete(fraccion, 0.05f, 1.0f);
    float radio = 0.2f * escala;
    float x0 = x - LARGO_TUBO_BANQUETE * 0.5f;
    bool parpadeo = std::fmod(tiempo * 8.0f, 1.0f) < 0.5f;

    if (racion.tipo == RACION_BANQUETE_PICANTE && parpadeo)
    {
        color = Color{ 255, 120, 90, 255 };
    }

    DrawCylinderEx({ x0, y, z }, { x0 + largo, y, z }, radio, radio, 10, color);
    DrawSphere({ x0, y, z }, radio, color);
    DrawSphere({ x0 + largo, y, z }, radio * 0.9f, color);

    // Anillos del envase.
    DrawCylinderEx({ x0 + largo * 0.3f, y, z }, { x0 + largo * 0.34f, y, z }, radio * 1.1f, radio * 1.1f, 10, Color{ 230, 235, 245, 255 });

    if (racion.tipo == RACION_BANQUETE_PICANTE)
    {
        for (int k = 0; k < 3; k++)
        {
            float fx = x0 + largo * (0.2f + 0.3f * (float)k);
            DrawSphere({ fx, y + radio + 0.12f + 0.06f * std::sin(tiempo * 12.0f + (float)k), z }, 0.08f, parpadeo ? ORANGE : RED);
        }
    }
    else if (racion.tipo == RACION_BANQUETE_DORADA)
    {
        for (int k = 0; k < 3; k++)
        {
            float angulo = tiempo * 3.0f + (float)k * 2.1f;
            DrawSphere({ x + std::cos(angulo) * 0.6f, y + 0.3f + 0.1f * std::sin(tiempo * 6.0f + (float)k), z + std::sin(angulo) * 0.3f }, 0.05f, WHITE);
        }
    }
}


static void DibujarRobotBanquete(const MinijuegoBanqueteTurbo& m)
{
    float t = m.tiempoAnimacion;
    float x = m.robotX;
    float y = 2.5f + 0.1f * std::sin(t * 2.5f);
    float z = -3.0f;

    // Propulsores.
    DrawCylinderEx({ x, y - 0.6f, z }, { x, y - 1.2f, z }, 0.3f, 0.0f, 8, Fade(Color{ 120, 230, 255, 255 }, 0.8f));

    // Torso y cabeza.
    DrawCube({ x, y, z }, 1.0f, 1.1f, 0.7f, Color{ 225, 230, 240, 255 });
    DrawCube({ x, y + 0.1f, z + 0.36f }, 0.5f, 0.4f, 0.04f, Color{ 80, 220, 255, 255 });
    DrawSphere({ x, y + 0.95f, z }, 0.42f, Color{ 235, 238, 246, 255 });
    DrawCube({ x, y + 0.98f, z + 0.3f }, 0.5f, 0.2f, 0.18f, Color{ 24, 30, 48, 255 });
    DrawSphere({ x - 0.12f, y + 0.98f, z + 0.4f }, 0.05f, Color{ 120, 255, 220, 255 });
    DrawSphere({ x + 0.12f, y + 0.98f, z + 0.4f }, 0.05f, Color{ 120, 255, 220, 255 });
    DrawCylinderEx({ x, y + 1.3f, z }, { x, y + 1.7f, z }, 0.03f, 0.03f, 4, Color{ 150, 160, 190, 255 });
    DrawSphere({ x, y + 1.72f, z }, 0.07f, std::fmod(t * 2.0f, 1.0f) < 0.5f ? RED : Color{ 90, 30, 30, 255 });

    // Brazos: extendidos hacia la mesa que acaba de servir.
    float extension = LimitarBanquete(m.robotBrazo * 2.0f, 0.0f, 1.0f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 hombro = { x + 0.6f * (float)lado, y + 0.3f, z };
        Vector3 mano =
        {
            hombro.x + (float)lado * 0.3f * (1.0f - extension),
            y - 0.6f + extension * (ALTURA_BANDEJA_BANQUETE + 0.6f - (y - 0.6f)),
            z + extension * (ZONA_MESA_BANQUETE - z) * 0.8f
        };

        DrawCylinderEx(hombro, mano, 0.1f, 0.08f, 6, Color{ 150, 160, 190, 255 });
        DrawSphere(mano, 0.12f, Color{ 80, 220, 255, 255 });
    }
}


static void DibujarPuestoBanquete(
    const MinijuegoBanqueteTurbo& m,
    int i,
    Color colorJugador
)
{
    const PuestoBanquete& p = m.puestos[i];
    float x = m.posicionX[i];
    Color neon = Color{ 80, 220, 255, 255 };

    // Taburete del jugador.
    DrawCylinder({ x, 0.0f, ZONA_JUGADOR_BANQUETE }, 0.45f, 0.5f, 0.3f, 12, Color{ 60, 68, 100, 255 });
    DrawCircle3D({ x, 0.32f, ZONA_JUGADOR_BANQUETE }, 0.5f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorJugador);

    // Mesa magnetica con borde luminoso.
    DrawCylinder({ x, 0.0f, ZONA_MESA_BANQUETE }, 0.35f, 0.55f, 0.8f, 10, Color{ 60, 70, 104, 255 });
    DrawCube({ x, 0.86f, ZONA_MESA_BANQUETE }, 3.2f, 0.12f, 1.6f, Color{ 70, 80, 118, 255 });
    DrawCube({ x, 0.93f, ZONA_MESA_BANQUETE + 0.8f }, 3.2f, 0.04f, 0.05f, colorJugador);
    DrawCube({ x, 0.93f, ZONA_MESA_BANQUETE - 0.8f }, 3.2f, 0.04f, 0.05f, Fade(neon, 0.8f));
    DrawCube({ x - 1.6f, 0.93f, ZONA_MESA_BANQUETE }, 0.05f, 0.04f, 1.6f, Fade(neon, 0.8f));
    DrawCube({ x + 1.6f, 0.93f, ZONA_MESA_BANQUETE }, 0.05f, 0.04f, 1.6f, Fade(neon, 0.8f));

    // Bandeja.
    DrawCube({ x, 0.95f, ZONA_MESA_BANQUETE }, 2.6f, 0.05f, 1.0f, Color{ 205, 212, 225, 255 });

    // Cola de proximas raciones al fondo de la mesa (avisa de las picantes).
    for (int k = 1; k <= 2; k++)
    {
        const RacionBanquete& siguiente = m.secuencia[(p.indiceRacion + k) % LARGO_SECUENCIA_BANQUETE];
        float qx = x - 0.9f + (float)(k - 1) * 1.5f;
        DibujarTuboBanquete(qx + LARGO_TUBO_BANQUETE * 0.25f, 1.1f, ZONA_MESA_BANQUETE - 0.55f, siguiente, 1.0f, m.tiempoAnimacion, 0.45f);
    }

    // Racion actual (desliza desde el robot al llegar).
    const RacionBanquete& racion = m.secuencia[p.indiceRacion >= 0 ? p.indiceRacion : 0];
    float llegada = LimitarBanquete(p.llegada, 0.0f, 1.0f);
    float inversa = 1.0f - llegada;
    float fraccion = p.bocadosRacion > 0 ? (float)p.bocadosRestantes / (float)p.bocadosRacion : 1.0f;
    float pulso = 1.0f + 0.12f * LimitarBanquete(p.pulsoBocado, 0.0f, 1.0f);

    if (p.bocadosRestantes > 0)
    {
        DibujarTuboBanquete(
            x,
            ALTURA_BANDEJA_BANQUETE + 0.2f + inversa * inversa * 0.9f,
            ZONA_MESA_BANQUETE - inversa * 2.4f,
            racion,
            fraccion,
            m.tiempoAnimacion,
            pulso
        );
    }

    // Racion rechazada saliendo disparada.
    if (p.salida > 0.0f)
    {
        float avance = 1.0f - p.salida / DURACION_RECHAZO_BANQUETE;
        DibujarTuboBanquete(
            x,
            ALTURA_BANDEJA_BANQUETE + 0.2f + avance * 2.0f,
            ZONA_MESA_BANQUETE - avance * 1.5f,
            p.racionSalida,
            1.0f,
            m.tiempoAnimacion,
            1.0f - avance * 0.5f
        );
    }
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoBanqueteTurbo::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteBanquete(cantidadMaxima);
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    ClearBackground(Color{ 6, 8, 20, 255 });
    BeginMode3D(camara);

    DibujarFondoBanquete(*this);
    DibujarRobotBanquete(*this);

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        DibujarPuestoBanquete(*this, i, participantes[i].color);

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_BANQUETE);

    EndMode3D();

    // Contador grande, barra de racion y avisos sobre cada puesto.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const PuestoBanquete& p = puestos[i];
        const RacionBanquete& racion = secuencia[p.indiceRacion >= 0 ? p.indiceRacion : 0];

        Vector2 cabeza = GetWorldToScreen({ posicionX[i], 3.0f, ZONA_JUGADOR_BANQUETE }, camara);
        const char* contador = TextFormat("%d", p.puntos);
        int anchoContador = MeasureText(contador, 44);

        DrawRectangle((int)cabeza.x - 70, (int)cabeza.y - 64, 140, 62, Fade(BLACK, 0.6f));
        DrawText(contador, (int)cabeza.x - anchoContador / 2, (int)cabeza.y - 62, 44, participantes[i].color);
        DrawText("RACIONES", (int)cabeza.x - MeasureText("RACIONES", 14) / 2, (int)cabeza.y - 16, 14, LIGHTGRAY);

        const char* nombre = NombreJugadorBanquete(participantes[i], i);
        DrawText(nombre, (int)cabeza.x - MeasureText(nombre, 18) / 2, (int)cabeza.y + 2, 18, participantes[i].color);

        // Barra de la racion actual.
        Vector2 mesa = GetWorldToScreen({ posicionX[i], 0.6f, ZONA_MESA_BANQUETE + 0.9f }, camara);
        float fraccion = p.bocadosRacion > 0 ? (float)p.bocadosRestantes / (float)p.bocadosRacion : 0.0f;

        DrawRectangle((int)mesa.x - 62, (int)mesa.y, 124, 14, Fade(BLACK, 0.7f));
        DrawRectangle((int)mesa.x - 60, (int)mesa.y + 2, (int)(120.0f * LimitarBanquete(fraccion, 0.0f, 1.0f)), 10, ColorRacionBanquete(racion));

        const char* estado = nullptr;
        Color colorEstado = RAYWHITE;

        if (fase == FASE_BANQUETE_JUGANDO || fase == FASE_BANQUETE_PREPARACION)
        {
            if (p.tos > 0.0f)
            {
                estado = "COF COF!";
                colorEstado = Color{ 255, 120, 90, 255 };
            }
            else if (p.rechazo > 0.0f)
            {
                estado = "RECHAZADA";
                colorEstado = LIGHTGRAY;
            }
            else if (racion.tipo == RACION_BANQUETE_PICANTE && p.bocadosRestantes > 0)
            {
                estado = participantes[i].esBot ? "PICANTE!" : TextFormat("PICANTE! SALTA: %s", ObtenerTextoBotonPrincipal(participantes[i]));
                colorEstado = std::fmod(tiempoAnimacion * 6.0f, 1.0f) < 0.5f ? RED : ORANGE;
            }
            else if (racion.tipo == RACION_BANQUETE_DORADA && p.bocadosRestantes > 0)
            {
                estado = "DORADA x2";
                colorEstado = GOLD;
            }
            else if (p.tiempoMensaje > 0.0f)
            {
                estado = "BIEN!";
                colorEstado = LIME;
            }
        }

        if (estado != nullptr)
        {
            int ancho = MeasureText(estado, 18);
            DrawRectangle((int)mesa.x - ancho / 2 - 5, (int)mesa.y + 18, ancho + 10, 24, Fade(BLACK, 0.7f));
            DrawText(estado, (int)mesa.x - ancho / 2, (int)mesa.y + 21, 18, colorEstado);
        }
    }

    // Cabecera.
    DrawRectangle(14, 12, 560, 92, Fade(BLACK, 0.74f));
    DrawText("BANQUETE TURBO", 28, 20, 28, GOLD);
    DrawText("MACHACA EL BOTON DE COMER: CADA PULSACION ES UN BOCADO", 28, 54, 15, RAYWHITE);
    DrawText("RACION ROJA = PICANTE: RECHAZALA CON SALTO   DORADA VALE x2", 28, 76, 14, LIGHTGRAY);

    if (fase == FASE_BANQUETE_JUGANDO)
    {
        DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
        DrawText(
            TextFormat("TIEMPO %.1f", tiempoRestante),
            anchoPantalla - 200,
            22,
            26,
            tiempoRestante <= 6.0f ? RED : GOLD
        );
    }

    // Tarjetas inferiores.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo) activos++;
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
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            int x = xInicial + k * (anchoTarjeta + separacion);
            int y = altoPantalla - 92;
            const PuestoBanquete& p = puestos[i];

            DrawRectangle(x, y, anchoTarjeta, 80, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 80, participantes[i].color);
            DrawText(NombreJugadorBanquete(participantes[i], i), x + 18, y + 8, 20, participantes[i].color);
            DrawText(TextFormat("%d PTS", p.puntos), x + 18, y + 32, 20, RAYWHITE);
            DrawText(TextFormat("BOCADOS %d", p.bocadosTotales), x + anchoTarjeta - 120, y + 12, 14, LIGHTGRAY);
            DrawText(TextFormat("DORADAS %d", p.doradas), x + anchoTarjeta - 120, y + 34, 14, GOLD);

            if (participantes[i].esBot)
            {
                DrawText("CONTROL: BOT", x + 18, y + 60, 14, LIGHTGRAY);
            }
            else
            {
                DrawText(
                    TextFormat("COMER: %s   RECHAZAR: %s", TextoBocadoBanquete(participantes[i]), ObtenerTextoBotonPrincipal(participantes[i])),
                    x + 18,
                    y + 60,
                    14,
                    LIGHTGRAY
                );
            }

            k++;
        }
    }

    if (fase == FASE_BANQUETE_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 150, 96, GOLD);

        const char* ayuda = "MACHACA EL BOTON DE COMER Y VIGILA LAS RACIONES ROJAS";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 - 50, 20, RAYWHITE);
    }
    else if (
        fase == FASE_BANQUETE_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 20;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorBanquete(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (
                    !resultado.participantes[i].participo ||
                    resultado.participantes[i].posicionFinal != posicion
                )
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d PTS   (%d BOCADOS)",
                        posicion,
                        NombreJugadorBanquete(participantes[i], i),
                        puestos[i].puntos,
                        puestos[i].bocadosTotales
                    ),
                    px + 40,
                    py + 62 + fila * 30,
                    22,
                    participantes[i].color
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


const ResultadoMinijuego& MinijuegoBanqueteTurbo::ObtenerResultado() const
{
    return resultado;
}
