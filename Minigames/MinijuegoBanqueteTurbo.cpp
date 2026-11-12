#include "Minigames/MinijuegoBanqueteTurbo.h"

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
#include <vector>


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

static void LiberarMallasBanquete();


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

    // Visual: el comedor horneado se rehace (puede cambiar la calidad).
    LiberarMallasBanquete();
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
// VISUAL: COMEDOR ORBITAL (solo decoracion; la logica no depende de esto)
//==================================================
//
// Cafeteria retro-futurista dentro de una estacion espacial. Interior
// acogedor de paneles crema con molduras turquesa; al fondo, un ventanal
// enorme por el que se ve el limbo de la Tierra (dia a la derecha, noche
// con luces de ciudades a la izquierda), la Luna y las estrellas. Tubos
// neumaticos cruzan el techo llevando capsulas de comida a la cocina de la
// izquierda; a la derecha, el cartel del menu (que ensena las tres
// raciones) y el dispensador de bebidas. Cada jugador come en su mesa
// magnetica flotante con su bandeja; el robot camarero va y viene por su
// carril luminoso. Lo estatico se hornea en mallas (Tierra, sala y
// utileria) y se rehace al cambiar la calidad.
//
// MODELO FUTURO: robot camarero, mesa magnetica con bandeja, raciones
// (tubo normal, picante y dorada), dispensador de bebidas, carro de
// bandejas, torre hidroponica y capsulas neumaticas.

static const Color ESPACIO_BT = { 4, 6, 18, 255 };
static const Color ESPACIO_ALTO_BT = { 10, 12, 34, 255 };
static const Color NIEBLA_BT = { 34, 40, 66, 255 };

static const Color CREMA_BT = { 222, 214, 196, 255 };
static const Color CREMA_OSCURA_BT = { 178, 170, 154, 255 };
static const Color TURQUESA_BT = { 52, 168, 166, 255 };
static const Color TURQUESA_OSCURA_BT = { 30, 102, 112, 255 };
static const Color ACERO_BT = { 160, 170, 186, 255 };
static const Color ACERO_OSCURO_BT = { 74, 82, 102, 255 };
static const Color GRAFITO_BT = { 44, 50, 66, 255 };
static const Color SUELO_BT = { 104, 110, 128, 255 };
static const Color JUNTA_BT = { 66, 72, 90, 255 };
static const Color CIRUELA_BT = { 120, 82, 136, 255 };     // tapizados
static const Color PLANTA_BT = { 76, 160, 92, 255 };
static const Color LUZ_SERVICIO_BT = { 255, 186, 96, 255 }; // ambar de servicio
static const Color LUZ_FRIA_BT = { 120, 222, 255, 255 };    // magnetismo y robot
static const Color PICANTE_BT = { 236, 46, 34, 255 };       // acento: peligro
static const Color DORADA_BT = { 255, 200, 48, 255 };       // acento: x2

// Camara de la vista.
static const Vector3 OJO_BT = { 0.0f, 5.5f, 11.7f };
static const Vector3 MIRA_BT = { 0.0f, 1.85f, -1.0f };
static const float FOV_BT = 46.0f;

// Sala.
static const float PARED_Z_BT = -7.5f;       // cara del muro del ventanal
static const float VENTANA_X_BT = 10.4f;     // medio ancho del hueco
static const float VENTANA_Y0_BT = 0.95f;
static const float VENTANA_Y1_BT = 6.9f;
static const float CHAFLAN_VENTANA_BT = 1.1f;
static const float VIDRIO_Z_BT = -7.82f;
static const float TECHO_Y_BT = 8.8f;
static const float SALA_X_BT = 16.5f;
static const float SALA_Z_BT = 11.0f;

// Carril del robot y tubos neumaticos.
static const float CARRIL_Z_BT = -3.0f;
static const float TUBO_Z_BT = -5.3f;
static const float TUBO_Y_BT[2] = { 6.45f, 6.95f };

// Tierra (vista desde la estacion: solo el casquete hacia la camara).
static const Vector3 CENTRO_TIERRA_BT = { 12.0f, -60.0f, -100.0f };
static const float RADIO_TIERRA_BT = 62.0f;


static float Ruido01BT(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}


struct GrabacionBT
{
    bool activa = false;
    std::vector<float> posiciones;
    std::vector<unsigned char> colores;
};

static GrabacionBT g_grabBT;
static Vector3 g_ojoBT = OJO_BT;
static Vector3 g_derBT = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaBT = { 0.0f, 1.0f, 0.0f };
static float g_emisBT = 0.0f;              // 1 = superficie luminosa: sin bruma


static float Lim01BT(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalBT(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoBT(Color c, float f)
{
    return { CanalBT((float)c.r * f), CanalBT((float)c.g * f), CanalBT((float)c.b * f), c.a };
}


static Color MezclaBT(Color a, Color b, float t)
{
    t = Lim01BT(t);
    return {
        CanalBT((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalBT((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalBT((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalBT((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaBT(Color c, float alfa)
{
    c.a = CanalBT((float)c.a * Lim01BT(alfa));
    return c;
}


static Color VariarBT(Color c, int indice, int semilla, float amplitud)
{
    return TonoBT(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01BT(indice, semilla));
}


// Niebla de color: el fondo de la sala se funde con la penumbra azulada
// que deja la luz de la Tierra.
static Color NieblaBT(Color c, Vector3 p)
{
    float dx = p.x - g_ojoBT.x, dy = p.y - g_ojoBT.y, dz = p.z - g_ojoBT.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01BT((d - 15.0f) / 26.0f) * 0.42f;
    n *= 1.0f - g_emisBT;
    Color r = MezclaBT(c, NIEBLA_BT, n);
    r.a = c.a;
    return r;
}


static void VBT(Vector3 p, Color c)
{
    if (g_grabBT.activa)
    {
        g_grabBT.posiciones.push_back(p.x);
        g_grabBT.posiciones.push_back(p.y);
        g_grabBT.posiciones.push_back(p.z);
        g_grabBT.colores.push_back(c.r);
        g_grabBT.colores.push_back(c.g);
        g_grabBT.colores.push_back(c.b);
        g_grabBT.colores.push_back(c.a);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteBT()
{
    if (!g_grabBT.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteBT()
{
    if (!g_grabBT.activa) rlEnd();
}


static void TriBT(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VBT(a, NieblaBT(color, a));
    VBT(b, NieblaBT(color, b));
    VBT(c, NieblaBT(color, c));
}


static void QuadBT(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriBT(a, b, c, color);
    TriBT(a, c, d, color);
}


static void Quad4BT(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VBT(a, NieblaBT(ca, a)); VBT(b, NieblaBT(cb, b)); VBT(c, NieblaBT(cc, c));
    VBT(a, NieblaBT(ca, a)); VBT(c, NieblaBT(cc, c)); VBT(d, NieblaBT(cd, d));
}


// Quad sin bruma con un color por vertice (luces aditivas y sombras).
static void QuadColBT(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VBT(a, ca); VBT(b, cb); VBT(c, cc);
    VBT(a, ca); VBT(c, cc); VBT(d, cd);
}


// Sombreado por caras de la guia (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62) interpolado para normales cualesquiera.
static float LuzBT(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f) return 1.0f;

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.5f) +
        (n.z > 0.0f ? az * 0.78f : az * 0.6f) +
        (n.x < 0.0f ? ax * 0.70f : ax * 0.62f);

    return f / suma;
}


static Color CaraBT(Color c, Vector3 n)
{
    return TonoBT(c, LuzBT(n));
}


// Caja con medio-ejes arbitrarios: caras sombreadas por normal.
static void CajaEjesBT(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
{
    auto P = [&](float sx, float sy, float sz)
    {
        return Vector3{
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    };

    Vector3 nx = Vector3Normalize(ex), ny = Vector3Normalize(ey), nz = Vector3Normalize(ez);

    QuadBT(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraBT(color, ny));
    QuadBT(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraBT(color, nz));
    QuadBT(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraBT(color, Vector3Negate(nz)));
    QuadBT(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraBT(color, Vector3Negate(nx)));
    QuadBT(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraBT(color, nx));

    if (fondo)
    {
        QuadBT(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraBT(color, Vector3Negate(ny)));
    }
}


static void CajaBT(Vector3 c, Vector3 t, Color color)
{
    CajaEjesBT(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, c.y - t.y * 0.5f > 0.05f);
}


// Caja girada en Y (grados).
static void CajaGiradaBT(Vector3 c, Vector3 t, float giroY, Color color)
{
    float a = giroY * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };
    CajaEjesBT(c, Vector3Scale(ux, t.x * 0.5f), { 0.0f, t.y * 0.5f, 0.0f }, Vector3Scale(uz, t.z * 0.5f), color, true);
}


// Viga de seccion rectangular entre dos puntos (ancho x alto).
static void VigaBT(Vector3 a, Vector3 b, float ancho, float alto, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesBT(centro, Vector3Scale(v, ancho * 0.5f), Vector3Scale(w, alto * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas.
static void CilBT(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    eje = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, ref));
    Vector3 v = Vector3CrossProduct(eje, u);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = 0.5f * (a0 + a1);
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));

        QuadBT(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraBT(color, nm)
        );

        if (tapas)
        {
            TriBT(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraBT(color, eje));
            TriBT(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraBT(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide (o media cupula con desde = 0) con caras sombreadas.
static void BolaBT(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float desde = -0.5f * PI)
{
    for (int i = 0; i < anillos; i++)
    {
        float la0 = desde + (0.5f * PI - desde) * (float)i / (float)anillos;
        float la1 = desde + (0.5f * PI - desde) * (float)(i + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float lo0 = 2.0f * PI * (float)k / (float)lados;
            float lo1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float lam = 0.5f * (la0 + la1), lom = 0.5f * (lo0 + lo1);
            Vector3 n = { std::cos(lam) * std::cos(lom), std::sin(lam), std::cos(lam) * std::sin(lom) };
            auto P = [&](float la, float lo)
            {
                return Vector3{ c.x + r.x * std::cos(la) * std::cos(lo), c.y + r.y * std::sin(la), c.z + r.z * std::cos(la) * std::sin(lo) };
            };
            QuadBT(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraBT(color, n));
        }
    }
}


// Disco horizontal con color de centro y de borde (sin bruma si se pide).
static void DiscoBT(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, bool bruma = true)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };

        if (bruma)
        {
            VBT(c, NieblaBT(centro, c)); VBT(p1, NieblaBT(borde, p1)); VBT(p0, NieblaBT(borde, p0));
        }
        else
        {
            VBT(c, centro); VBT(p1, borde); VBT(p0, borde);
        }
    }
}


// Anillo horizontal entre r0 y r1 (sin bruma: marcas y luces).
static void AnilloBT(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera, float desde = 0.0f, float hasta = 2.0f * PI)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = desde + (hasta - desde) * (float)k / (float)lados;
        float a1 = desde + (hasta - desde) * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColBT(i0, i1, o1, o0, dentro, dentro, fuera, fuera);
    }
}


// Disco que mira a la camara con centro y borde de distinto color (halos).
static void HaloBT(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derBT, std::cos(a0) * radio), Vector3Scale(g_arribaBT, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derBT, std::cos(a1) * radio), Vector3Scale(g_arribaBT, std::sin(a1) * radio)));
        VBT(c, centro); VBT(p0, borde); VBT(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas).
static void PuntoBT(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derBT, r), u = Vector3Scale(g_arribaBT, r);
    QuadColBT(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Tira que mira a la camara entre dos puntos (cables, haces, estelas).
static void TiraBT(Vector3 a, Vector3 b, float anchoA, float anchoB, Color ca, Color cb)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 haciaOjo = Vector3Subtract(g_ojoBT, a);
    Vector3 lado = Vector3CrossProduct(eje, haciaOjo);
    float largo = Vector3Length(lado);

    if (largo < 0.00001f) return;

    lado = Vector3Scale(lado, 1.0f / largo);
    Vector3 la = Vector3Scale(lado, anchoA * 0.5f), lb = Vector3Scale(lado, anchoB * 0.5f);
    QuadColBT(Vector3Subtract(a, la), Vector3Add(a, la), Vector3Add(b, lb), Vector3Subtract(b, lb), ca, ca, cb, cb);
}


// Rectangulo vertical en el plano z (cara hacia +z) con color por esquina.
static void PanelZBT(float x0, float y0, float x1, float y1, float z, Color abajo, Color arriba)
{
    Quad4BT({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, abajo, abajo, arriba, arriba);
}


// Rectangulo horizontal (x0..x1, z0..z1) a la altura y.
static void PisoBT(float x0, float z0, float x1, float z1, float y, Color c)
{
    QuadBT({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, c);
}


// Prisma de planta octogonal (rectangulo con esquinas achaflanadas).
static void PrismaChaflanBT(Vector3 c, float hx, float hz, float chaflan, float alto, Color tapa, Color lado)
{
    Vector3 p[8] =
    {
        { c.x - hx + chaflan, 0.0f, c.z - hz }, { c.x + hx - chaflan, 0.0f, c.z - hz },
        { c.x + hx, 0.0f, c.z - hz + chaflan }, { c.x + hx, 0.0f, c.z + hz - chaflan },
        { c.x + hx - chaflan, 0.0f, c.z + hz }, { c.x - hx + chaflan, 0.0f, c.z + hz },
        { c.x - hx, 0.0f, c.z + hz - chaflan }, { c.x - hx, 0.0f, c.z - hz + chaflan }
    };
    float y0 = c.y, y1 = c.y + alto;
    Vector3 centro = { c.x, y1, c.z };

    for (int k = 0; k < 8; k++)
    {
        Vector3 a = p[k], b = p[(k + 1) % 8];
        Vector3 n = Vector3Normalize({ b.z - a.z, 0.0f, a.x - b.x });   // hacia afuera
        QuadBT({ a.x, y0, a.z }, { b.x, y0, b.z }, { b.x, y1, b.z }, { a.x, y1, a.z }, CaraBT(lado, n));
        TriBT(centro, { b.x, y1, b.z }, { a.x, y1, a.z }, tapa);
    }
}


//--------------------------------------------------
// Horno: lo estatico se graba una vez en mallas
//--------------------------------------------------

struct HornoBT
{
    bool listo = false;
    int clave = -1;
    Mesh tierra{};
    bool tierraValida = false;
    Mesh nubes{};
    bool nubesValidas = false;
    Mesh sala{};
    bool salaValida = false;
    Mesh utileria{};
    bool utileriaValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoBT g_hornoBT;


static void EmpezarGrabacionBT()
{
    g_grabBT.activa = true;
    g_grabBT.posiciones.clear();
    g_grabBT.colores.clear();
}


// Cierra la grabacion y sube la malla (sin indices: cabe cualquier tamano).
static bool TerminarGrabacionBT(Mesh& malla)
{
    g_grabBT.activa = false;
    int n = (int)(g_grabBT.posiciones.size() / 3);
    n -= n % 3;

    if (n < 3)
    {
        g_grabBT.posiciones.clear();
        g_grabBT.colores.clear();
        return false;
    }

    malla = {};
    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * (int)sizeof(float)));
    malla.texcoords = (float*)MemAlloc((unsigned int)(n * 2 * (int)sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    for (int i = 0; i < n * 3; i++) malla.vertices[i] = g_grabBT.posiciones[(size_t)i];
    for (int i = 0; i < n * 4; i++) malla.colors[i] = g_grabBT.colores[(size_t)i];
    for (int i = 0; i < n * 2; i++) malla.texcoords[i] = 0.0f;

    UploadMesh(&malla, false);
    g_grabBT.posiciones.clear();
    g_grabBT.colores.clear();
    return true;
}


static void LiberarMallaBT(Mesh& malla, bool& valida)
{
    if (valida) UnloadMesh(malla);
    valida = false;
    malla = {};
}


static void LiberarMallasBanquete()
{
    LiberarMallaBT(g_hornoBT.tierra, g_hornoBT.tierraValida);
    LiberarMallaBT(g_hornoBT.nubes, g_hornoBT.nubesValidas);
    LiberarMallaBT(g_hornoBT.sala, g_hornoBT.salaValida);
    LiberarMallaBT(g_hornoBT.utileria, g_hornoBT.utileriaValida);
    g_hornoBT.listo = false;
    g_hornoBT.clave = -1;
}


static void TierraEstaticaBT(bool detalle, bool nubes);
static void SalaEstaticaBT(bool decoracion);
static void UtileriaEstaticaBT(int paso);


static int ClaveHornoBT()
{
    return (int)CalidadDecoracion();
}


static void HornearBT()
{
    int clave = ClaveHornoBT();

    if ((g_hornoBT.listo && g_hornoBT.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasBanquete();

    if (!g_hornoBT.materialListo)
    {
        g_hornoBT.material = LoadMaterialDefault();
        g_hornoBT.materialListo = true;
    }

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    Vector3 ojo = g_ojoBT;
    g_ojoBT = OJO_BT;

    EmpezarGrabacionBT();
    TierraEstaticaBT(decoracion, false);
    g_hornoBT.tierraValida = TerminarGrabacionBT(g_hornoBT.tierra);

    if (decoracion)
    {
        EmpezarGrabacionBT();
        TierraEstaticaBT(paso == 1, true);
        g_hornoBT.nubesValidas = TerminarGrabacionBT(g_hornoBT.nubes);
    }

    EmpezarGrabacionBT();
    SalaEstaticaBT(decoracion);
    g_hornoBT.salaValida = TerminarGrabacionBT(g_hornoBT.sala);

    if (decoracion)
    {
        EmpezarGrabacionBT();
        UtileriaEstaticaBT(paso);
        g_hornoBT.utileriaValida = TerminarGrabacionBT(g_hornoBT.utileria);
    }

    g_ojoBT = ojo;
    g_hornoBT.clave = clave;
    g_hornoBT.listo = true;
}


static void DibujarMallaBT(const Mesh& malla, bool valida, Matrix transformacion = MatrixIdentity())
{
    if (!g_hornoBT.listo || !valida) return;

    rlDrawRenderBatchActive();
    DrawMesh(malla, g_hornoBT.material, transformacion);
    rlDrawRenderBatchActive();
}


//--------------------------------------------------
// La Tierra: casquete orientado a la camara
//--------------------------------------------------

static float RuidoValorBT(float x, float y, float z)
{
    int xi = (int)std::floor(x), yi = (int)std::floor(y), zi = (int)std::floor(z);
    float fx = x - (float)xi, fy = y - (float)yi, fz = z - (float)zi;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    fz = fz * fz * (3.0f - 2.0f * fz);

    auto H = [&](int a, int b, int c)
    {
        return Ruido01BT(a * 73856093 ^ b * 19349663 ^ c * 83492791, 917);
    };

    float x00 = H(xi, yi, zi) + (H(xi + 1, yi, zi) - H(xi, yi, zi)) * fx;
    float x10 = H(xi, yi + 1, zi) + (H(xi + 1, yi + 1, zi) - H(xi, yi + 1, zi)) * fx;
    float x01 = H(xi, yi, zi + 1) + (H(xi + 1, yi, zi + 1) - H(xi, yi, zi + 1)) * fx;
    float x11 = H(xi, yi + 1, zi + 1) + (H(xi + 1, yi + 1, zi + 1) - H(xi, yi + 1, zi + 1)) * fx;
    float y0 = x00 + (x10 - x00) * fy;
    float y1 = x01 + (x11 - x01) * fy;
    return y0 + (y1 - y0) * fz;
}


static float FbmBT(Vector3 p, float escala)
{
    return 0.56f * RuidoValorBT(p.x * escala, p.y * escala, p.z * escala) +
        0.29f * RuidoValorBT(p.x * escala * 2.13f + 5.1f, p.y * escala * 2.13f, p.z * escala * 2.13f) +
        0.15f * RuidoValorBT(p.x * escala * 4.37f, p.y * escala * 4.37f + 9.3f, p.z * escala * 4.37f);
}


// Color de un punto de la superficie (n: normal unitaria).
static Color ColorTierraBT(Vector3 n, bool nubes, int indice)
{
    // Sol desde la derecha y algo de frente: la izquierda cae en la noche.
    const Vector3 sol = Vector3Normalize({ 0.9f, 0.55f, 0.35f });
    float luz = Vector3DotProduct(n, sol);
    float dia = Lim01BT((luz + 0.12f) / 0.42f);

    float tierra = FbmBT(n, 2.6f);
    float latitud = std::fabs(n.y);

    if (nubes)
    {
        float c = FbmBT({ n.x * 1.7f + 3.0f, n.y * 3.4f, n.z * 1.7f }, 3.1f);
        float a = Lim01BT((c - 0.5f) / 0.16f);
        Color nube = MezclaBT(Color{ 40, 48, 78, 255 }, Color{ 250, 252, 255, 255 }, 0.15f + 0.85f * dia);
        return AlfaBT(nube, a * 0.82f);
    }

    Color c;

    if (tierra > 0.53f)
    {
        float seco = Lim01BT((0.35f - std::fabs(n.y - 0.18f)) / 0.2f) * Lim01BT((tierra - 0.6f) / 0.1f);
        c = MezclaBT(Color{ 70, 136, 66, 255 }, Color{ 196, 168, 108, 255 }, seco);
        c = MezclaBT(c, Color{ 44, 96, 52, 255 }, Lim01BT((0.58f - tierra) / 0.05f) * 0.5f);
    }
    else
    {
        float costa = Lim01BT((tierra - 0.45f) / 0.08f);
        c = MezclaBT(Color{ 18, 58, 150, 255 }, Color{ 38, 128, 196, 255 }, costa);
    }

    if (latitud > 0.82f) c = MezclaBT(c, Color{ 236, 242, 250, 255 }, Lim01BT((latitud - 0.82f) / 0.06f));

    Color noche = TonoBT(c, 0.08f);
    noche = MezclaBT(noche, Color{ 6, 10, 30, 255 }, 0.6f);

    // Luces de ciudades en la noche, sobre la tierra firme.
    if (tierra > 0.55f && Ruido01BT(indice, 431) > 0.72f)
    {
        noche = MezclaBT(noche, Color{ 255, 196, 110, 255 }, 0.55f + 0.4f * Ruido01BT(indice, 432));
    }

    Color r = MezclaBT(noche, c, dia);

    // Brillo del terminador (amanecer anaranjado).
    float borde = 1.0f - std::fabs(luz + 0.02f) / 0.12f;
    if (borde > 0.0f) r = MezclaBT(r, Color{ 255, 150, 90, 255 }, borde * 0.35f);

    return r;
}


static void TierraEstaticaBT(bool detalle, bool nubes)
{
    g_emisBT = 1.0f;
    Vector3 C = CENTRO_TIERRA_BT;
    float R = RADIO_TIERRA_BT * (nubes ? 1.006f : 1.0f);
    Vector3 w = Vector3Normalize(Vector3Subtract(OJO_BT, C));
    Vector3 u = Vector3Normalize(Vector3CrossProduct({ 0.0f, 1.0f, 0.0f }, w));
    Vector3 v = Vector3CrossProduct(w, u);
    float distancia = Vector3Distance(OJO_BT, C);
    float tMax = std::acos(RADIO_TIERRA_BT / distancia) + 0.01f;

    int anillos = detalle ? 46 : 26;
    int lados = detalle ? 120 : 64;

    auto Normal = [&](float t, float f)
    {
        return Vector3Add(Vector3Scale(w, std::cos(t)), Vector3Add(Vector3Scale(u, std::sin(t) * std::cos(f)), Vector3Scale(v, std::sin(t) * std::sin(f))));
    };

    for (int i = 0; i < anillos; i++)
    {
        // Mas anillos cerca del horizonte, que es lo que se ve.
        float s0 = (float)i / (float)anillos, s1 = (float)(i + 1) / (float)anillos;
        float t0 = tMax * std::sqrt(s0), t1 = tMax * std::sqrt(s1);

        for (int k = 0; k < lados; k++)
        {
            float f0 = 2.0f * PI * (float)k / (float)lados;
            float f1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            Vector3 n00 = Normal(t0, f0), n01 = Normal(t0, f1), n11 = Normal(t1, f1), n10 = Normal(t1, f0);
            int base = i * 1000 + k;
            Color c00 = ColorTierraBT(n00, nubes, base);
            Color c01 = ColorTierraBT(n01, nubes, base + 1);
            Color c11 = ColorTierraBT(n11, nubes, base + 1001);
            Color c10 = ColorTierraBT(n10, nubes, base + 1000);

            if (nubes && c00.a + c01.a + c11.a + c10.a == 0) continue;

            QuadColBT(
                Vector3Add(C, Vector3Scale(n00, R)), Vector3Add(C, Vector3Scale(n01, R)),
                Vector3Add(C, Vector3Scale(n11, R)), Vector3Add(C, Vector3Scale(n10, R)),
                c00, c01, c11, c10);
        }
    }

    // Atmosfera: anillo alrededor del limbo, mas brillante del lado del sol.
    if (!nubes)
    {
        float rh = RADIO_TIERRA_BT * std::sin(tMax - 0.01f);
        Vector3 ch = Vector3Add(C, Vector3Scale(w, RADIO_TIERRA_BT * std::cos(tMax - 0.01f) + 0.5f));
        int segmentos = detalle ? 160 : 90;

        for (int k = 0; k < segmentos; k++)
        {
            float f0 = 2.0f * PI * (float)k / (float)segmentos;
            float f1 = 2.0f * PI * (float)(k + 1) / (float)segmentos;
            Vector3 d0 = Vector3Add(Vector3Scale(u, std::cos(f0)), Vector3Scale(v, std::sin(f0)));
            Vector3 d1 = Vector3Add(Vector3Scale(u, std::cos(f1)), Vector3Scale(v, std::sin(f1)));
            float sol = Lim01BT(0.5f + 0.5f * std::cos(f0 - 0.35f));
            Color fuerte = MezclaBT(Color{ 70, 110, 220, 255 }, Color{ 150, 214, 255, 255 }, sol);
            Color medio = AlfaBT(fuerte, 0.55f + 0.35f * sol);
            Color nada = AlfaBT(fuerte, 0.0f);

            // Banda interior (el limbo se aclara) y halo exterior.
            QuadColBT(
                Vector3Add(ch, Vector3Scale(d0, rh * 0.965f)), Vector3Add(ch, Vector3Scale(d1, rh * 0.965f)),
                Vector3Add(ch, Vector3Scale(d1, rh * 1.0f)), Vector3Add(ch, Vector3Scale(d0, rh * 1.0f)),
                AlfaBT(fuerte, 0.0f), AlfaBT(fuerte, 0.0f), medio, medio);
            QuadColBT(
                Vector3Add(ch, Vector3Scale(d0, rh * 1.0f)), Vector3Add(ch, Vector3Scale(d1, rh * 1.0f)),
                Vector3Add(ch, Vector3Scale(d1, rh * 1.05f)), Vector3Add(ch, Vector3Scale(d0, rh * 1.05f)),
                medio, medio, nada, nada);
        }
    }

    g_emisBT = 0.0f;
}


//--------------------------------------------------
// Sala: piso, muro del ventanal, techo y paredes
//--------------------------------------------------

static void SalaEstaticaBT(bool decoracion)
{
    LoteBT();

    // Piso de placas de cubierta con juntas.
    const float placa = 1.6f;
    int columnas = (int)(2.0f * SALA_X_BT / placa) + 1;
    int filas = (int)((SALA_Z_BT - PARED_Z_BT) / placa) + 1;
    PisoBT(-SALA_X_BT, PARED_Z_BT, SALA_X_BT, SALA_Z_BT, -0.002f, JUNTA_BT);

    for (int fz = 0; fz < filas; fz++)
    {
        for (int cx = 0; cx < columnas; cx++)
        {
            float x0 = -SALA_X_BT + (float)cx * placa, z0 = PARED_Z_BT + (float)fz * placa;
            float x1 = x0 + placa, z1 = z0 + placa;

            if (x1 > SALA_X_BT) x1 = SALA_X_BT;
            if (z1 > SALA_Z_BT) z1 = SALA_Z_BT;

            Color c = VariarBT(SUELO_BT, cx * 61 + fz, 11, 0.07f);

            // Carril del robot: franja de placas mas oscuras.
            if (z1 > CARRIL_Z_BT - 0.9f && z0 < CARRIL_Z_BT + 0.9f) c = MezclaBT(c, GRAFITO_BT, 0.55f);

            float j = 0.035f;
            Quad4BT({ x0 + j, 0.0f, z0 + j }, { x0 + j, 0.0f, z1 - j }, { x1 - j, 0.0f, z1 - j }, { x1 - j, 0.0f, z0 + j },
                c, TonoBT(c, 0.94f), TonoBT(c, 0.9f), TonoBT(c, 0.97f));

            // Remaches en las esquinas.
            if (decoracion && (cx + fz) % 2 == 0)
            {
                PisoBT(x0 + 0.12f, z0 + 0.12f, x0 + 0.2f, z0 + 0.2f, 0.004f, TonoBT(c, 1.18f));
                PisoBT(x1 - 0.2f, z1 - 0.2f, x1 - 0.12f, z1 - 0.12f, 0.004f, TonoBT(c, 1.18f));
            }
        }
    }

    // Bordes del carril: guias metalicas.
    CajaBT({ 0.0f, 0.02f, CARRIL_Z_BT - 0.95f }, { 2.0f * SALA_X_BT, 0.04f, 0.1f }, ACERO_BT);
    CajaBT({ 0.0f, 0.02f, CARRIL_Z_BT + 0.95f }, { 2.0f * SALA_X_BT, 0.04f, 0.1f }, ACERO_BT);

    // Alfombra del comedor delante de las mesas: ribete crema y planetitas.
    {
        float ax = 10.5f, az0 = 2.5f, az1 = 3.9f;
        PisoBT(-ax, az0, ax, az1, 0.006f, CREMA_OSCURA_BT);
        Quad4BT({ -ax + 0.18f, 0.008f, az0 + 0.18f }, { -ax + 0.18f, 0.008f, az1 - 0.18f }, { ax - 0.18f, 0.008f, az1 - 0.18f }, { ax - 0.18f, 0.008f, az0 + 0.18f },
            TonoBT(CIRUELA_BT, 0.78f), TonoBT(CIRUELA_BT, 0.72f), TonoBT(CIRUELA_BT, 0.72f), TonoBT(CIRUELA_BT, 0.78f));
        PisoBT(-ax + 0.32f, az0 + 0.32f, ax - 0.32f, az0 + 0.38f, 0.01f, TURQUESA_BT);
        PisoBT(-ax + 0.32f, az1 - 0.38f, ax - 0.32f, az1 - 0.32f, 0.01f, TURQUESA_BT);

        for (int k = 0; k < 14; k++)
        {
            float px = -ax + 0.75f + (float)k * 1.5f;
            float pz = (az0 + az1) * 0.5f;
            DiscoBT({ px, 0.011f, pz }, 0.22f, 0.22f, 12, k % 2 == 0 ? TonoBT(CREMA_BT, 0.85f) : TonoBT(TURQUESA_BT, 0.9f), k % 2 == 0 ? TonoBT(CREMA_BT, 0.75f) : TonoBT(TURQUESA_BT, 0.8f));
            AnilloBT({ px, 0.013f, pz }, 0.3f, 0.34f, 14, NieblaBT(TonoBT(CREMA_BT, 0.7f), { px, 0.0f, pz }), NieblaBT(TonoBT(CREMA_BT, 0.7f), { px, 0.0f, pz }), -0.4f, PI - 0.9f);
        }
    }

    // Muro del ventanal: zocalo, paneles y hueco achaflanado.
    float z = PARED_Z_BT;
    float x0 = -VENTANA_X_BT, x1 = VENTANA_X_BT, y0 = VENTANA_Y0_BT, y1 = VENTANA_Y1_BT, ch = CHAFLAN_VENTANA_BT;
    Color pared = CREMA_BT;
    Color zocalo = TURQUESA_OSCURA_BT;

    PanelZBT(-SALA_X_BT, 0.0f, SALA_X_BT, 0.9f, z, TonoBT(zocalo, 0.8f), zocalo);
    PanelZBT(-SALA_X_BT, y1, SALA_X_BT, TECHO_Y_BT, z, TonoBT(pared, 0.82f), TonoBT(pared, 0.7f));
    PanelZBT(-SALA_X_BT, 0.9f, x0, y1, z, TonoBT(pared, 0.84f), TonoBT(pared, 0.8f));
    PanelZBT(x1, 0.9f, SALA_X_BT, y1, z, TonoBT(pared, 0.84f), TonoBT(pared, 0.8f));
    PanelZBT(x0, 0.9f, x1, y0, z, TonoBT(pared, 0.84f), TonoBT(pared, 0.84f));
    // Esquinas que rellenan el chaflan.
    TriBT({ x0, y0, z }, { x0 + ch, y0, z }, { x0, y0 + ch, z }, TonoBT(pared, 0.84f));
    TriBT({ x1 - ch, y0, z }, { x1, y0, z }, { x1, y0 + ch, z }, TonoBT(pared, 0.84f));
    TriBT({ x0, y1, z }, { x0, y1 - ch, z }, { x0 + ch, y1, z }, TonoBT(pared, 0.8f));
    TriBT({ x1, y1 - ch, z }, { x1, y1, z }, { x1 - ch, y1, z }, TonoBT(pared, 0.8f));

    // Moldura del zocalo.
    CajaBT({ 0.0f, 0.92f, z + 0.04f }, { 2.0f * SALA_X_BT, 0.08f, 0.1f }, TURQUESA_BT);

    // Juntas de paneles en los muros laterales del ventanal.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 3; k++)
        {
            float x = (float)lado * (VENTANA_X_BT + 1.0f + (float)k * 2.0f);
            CajaBT({ x, (0.9f + TECHO_Y_BT) * 0.5f, z + 0.02f }, { 0.06f, TECHO_Y_BT - 0.9f, 0.04f }, CREMA_OSCURA_BT);
        }
    }

    // Marco del hueco: octogono con jamba, bisel y moldura turquesa.
    Vector3 oct[8] =
    {
        { x0 + ch, y0, 0.0f }, { x1 - ch, y0, 0.0f }, { x1, y0 + ch, 0.0f }, { x1, y1 - ch, 0.0f },
        { x1 - ch, y1, 0.0f }, { x0 + ch, y1, 0.0f }, { x0, y1 - ch, 0.0f }, { x0, y0 + ch, 0.0f }
    };
    Vector3 centroV = { 0.0f, (y0 + y1) * 0.5f, 0.0f };

    for (int k = 0; k < 8; k++)
    {
        Vector3 a = oct[k], b = oct[(k + 1) % 8];
        Vector3 da = Vector3Normalize({ a.x - centroV.x, a.y - centroV.y, 0.0f });
        Vector3 db = Vector3Normalize({ b.x - centroV.x, b.y - centroV.y, 0.0f });
        Vector3 medio = Vector3Normalize({ (a.x + b.x) * 0.5f - centroV.x, (a.y + b.y) * 0.5f - centroV.y, 0.0f });
        Color jamba = CaraBT(ACERO_OSCURO_BT, Vector3Negate(medio));

        // Jamba (espesor del muro).
        QuadBT({ a.x, a.y, z }, { b.x, b.y, z }, { b.x, b.y, z - 0.55f }, { a.x, a.y, z - 0.55f }, jamba);

        // Bisel saliente alrededor del hueco.
        float e = 0.42f;
        Vector3 ae = { a.x + da.x * e, a.y + da.y * e, z + 0.14f }, be = { b.x + db.x * e, b.y + db.y * e, z + 0.14f };
        Vector3 ai = { a.x, a.y, z + 0.14f }, bi = { b.x, b.y, z + 0.14f };
        QuadBT(ai, bi, be, ae, CaraBT(ACERO_BT, { 0.0f, 0.3f, 1.0f }));
        QuadBT({ ae.x, ae.y, z }, { be.x, be.y, z }, be, ae, CaraBT(ACERO_BT, medio));
        QuadBT({ a.x, a.y, z }, { b.x, b.y, z }, bi, ai, CaraBT(ACERO_OSCURO_BT, Vector3Negate(medio)));

        // Filete turquesa interior.
        float f = 0.08f;
        Vector3 af = { a.x + da.x * f, a.y + da.y * f, z + 0.145f }, bf = { b.x + db.x * f, b.y + db.y * f, z + 0.145f };
        Vector3 ag = { a.x + da.x * (f + 0.06f), a.y + da.y * (f + 0.06f), z + 0.145f }, bg = { b.x + db.x * (f + 0.06f), b.y + db.y * (f + 0.06f), z + 0.145f };
        QuadBT(af, bf, bg, ag, TURQUESA_BT);
    }

    // Parteluces del ventanal con remaches.
    for (int k = -2; k <= 2; k++)
    {
        if (k == 0) continue;

        float x = (float)k * 4.16f + (k > 0 ? -2.08f : 2.08f);
        CajaBT({ x, (y0 + y1) * 0.5f, VIDRIO_Z_BT + 0.08f }, { 0.24f, y1 - y0, 0.22f }, ACERO_OSCURO_BT);
        CajaBT({ x, (y0 + y1) * 0.5f, VIDRIO_Z_BT + 0.2f }, { 0.1f, y1 - y0, 0.04f }, ACERO_BT);
    }

    // Travesano bajo y repisa / banco del ventanal.
    CajaBT({ 0.0f, y0 + 0.12f, VIDRIO_Z_BT + 0.06f }, { 2.0f * x1, 0.24f, 0.2f }, ACERO_OSCURO_BT);
    CajaBT({ 0.0f, 0.45f, z + 0.38f }, { 2.0f * x1 - 2.4f, 0.9f, 0.76f }, TURQUESA_OSCURA_BT);
    CajaBT({ 0.0f, 0.96f, z + 0.42f }, { 2.0f * x1 - 2.2f, 0.12f, 0.9f }, ACERO_BT);

    if (decoracion)
    {
        // Almohadones del banco.
        for (int k = 0; k < 6; k++)
        {
            float x = -7.2f + (float)k * 2.88f;
            CajaBT({ x, 1.1f, z + 0.42f }, { 2.6f, 0.16f, 0.72f }, VariarBT(CIRUELA_BT, k, 21, 0.06f));
            CajaBT({ x, 1.45f, z + 0.1f }, { 2.6f, 0.55f, 0.16f }, TonoBT(VariarBT(CIRUELA_BT, k, 21, 0.06f), 0.92f));
        }
    }

    // Dintel: caja con la fila de luces de servicio.
    CajaBT({ 0.0f, 7.9f, z + 0.35f }, { 2.0f * SALA_X_BT, 1.8f, 0.7f }, TonoBT(CREMA_BT, 0.86f));
    CajaBT({ 0.0f, 7.05f, z + 0.72f }, { 2.0f * SALA_X_BT, 0.1f, 0.06f }, TURQUESA_BT);

    // Techo y paredes laterales (se ven en la entrada de camara).
    QuadBT({ -SALA_X_BT, TECHO_Y_BT, PARED_Z_BT }, { SALA_X_BT, TECHO_Y_BT, PARED_Z_BT },
        { SALA_X_BT, TECHO_Y_BT, SALA_Z_BT }, { -SALA_X_BT, TECHO_Y_BT, SALA_Z_BT }, TonoBT(GRAFITO_BT, 0.8f));

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * SALA_X_BT;
        Color c = TonoBT(CREMA_BT, lado < 0 ? 0.74f : 0.66f);
        QuadBT({ x, 0.0f, PARED_Z_BT }, { x, 0.0f, SALA_Z_BT }, { x, TECHO_Y_BT, SALA_Z_BT }, { x, TECHO_Y_BT, PARED_Z_BT }, c);
        QuadBT({ x, 0.0f, PARED_Z_BT }, { x, 0.0f, SALA_Z_BT }, { x, 0.9f, SALA_Z_BT }, { x, 0.9f, PARED_Z_BT }, TonoBT(zocalo, 0.8f));
    }

    FinLoteBT();
}


// Cocina: puesto saliente en el muro izquierdo con la ventanilla de pase.
static const float COCINA_X0_BT = -15.9f;
static const float COCINA_X1_BT = -11.4f;
// Cartel del menu y dispensador de bebidas a la derecha.
static const float MENU_X0_BT = 11.3f;
static const float MENU_X1_BT = 15.8f;
static const float MENU_Y0_BT = 3.05f;
static const float MENU_Y1_BT = 5.75f;
// Torres hidroponicas a los lados del ventanal y mesita del salon.
static const float TORRE_X_BT = 9.75f;
static const float TORRE_Z_BT = -6.35f;
static const Vector3 MESITA_BT = { 0.0f, 0.0f, -5.35f };


static void TorreHidroponicaBT(float x, float z, int semilla, int paso)
{
    CilBT({ x, 0.0f, z }, { x, 0.62f, z }, 0.5f, 0.56f, 14, CREMA_BT);
    CilBT({ x, 0.62f, z }, { x, 0.68f, z }, 0.58f, 0.58f, 14, TURQUESA_BT);
    BolaBT({ x, 0.68f, z }, { 0.5f, 0.16f, 0.5f }, Color{ 92, 70, 54, 255 }, 2, 12, 0.0f);
    CilBT({ x, 0.68f, z }, { x, 3.4f, z }, 0.14f, 0.12f, 10, ACERO_BT);

    // Cogollos de hojas escalonados alrededor de la columna.
    int niveles = paso == 1 ? 6 : 4;

    for (int k = 0; k < niveles; k++)
    {
        float y = 0.95f + (float)k * (2.3f / (float)niveles);

        for (int h = 0; h < 4; h++)
        {
            float a = (float)h * PI * 0.5f + (float)k * 0.8f + Ruido01BT(k * 4 + h, semilla) * 0.5f;
            float r = 0.28f + 0.12f * Ruido01BT(k * 4 + h, semilla + 1);
            Color hoja = VariarBT(PLANTA_BT, k * 4 + h, semilla + 2, 0.12f);
            BolaBT({ x + std::cos(a) * r, y + 0.1f * Ruido01BT(h, semilla + 3), z + std::sin(a) * r }, { 0.26f, 0.15f, 0.26f }, hoja, 3, 8);
        }

        CilBT({ x, y - 0.06f, z }, { x, y, z }, 0.2f, 0.2f, 10, CREMA_OSCURA_BT);
    }

    // Aro de la luz de cultivo.
    CilBT({ x, 3.4f, z }, { x, 3.5f, z }, 0.62f, 0.62f, 16, ACERO_OSCURO_BT);
}


static void BanquitoBT(float x, float z)
{
    CilBT({ x, 0.0f, z }, { x, 0.04f, z }, 0.32f, 0.32f, 12, ACERO_OSCURO_BT);
    CilBT({ x, 0.04f, z }, { x, 0.6f, z }, 0.06f, 0.06f, 8, ACERO_BT);
    CilBT({ x, 0.6f, z }, { x, 0.78f, z }, 0.34f, 0.36f, 14, CIRUELA_BT);
    BolaBT({ x, 0.78f, z }, { 0.36f, 0.06f, 0.36f }, TonoBT(CIRUELA_BT, 1.12f), 2, 14, 0.0f);
}


static void UtileriaEstaticaBT(int paso)
{
    float z = PARED_Z_BT;

    LoteBT();

    //---- Cocina con ventanilla de pase ----
    {
        float x0 = COCINA_X0_BT, x1 = COCINA_X1_BT;
        float hondo = 0.75f;

        // Azulejos del fondo de la ventanilla.
        for (int fila = 0; fila < 6; fila++)
        {
            for (int col = 0; col < 11; col++)
            {
                float ax = x0 + 0.35f + (float)col * 0.35f, ay = 1.2f + (float)fila * 0.38f;
                Color az = (fila + col) % 5 == 0 ? TURQUESA_BT : VariarBT(Color{ 236, 230, 214, 255 }, fila * 11 + col, 31, 0.04f);
                PanelZBT(ax + 0.02f, ay + 0.02f, ax + 0.33f, ay + 0.36f, z + 0.015f, TonoBT(az, 0.92f), az);
            }
        }

        // Estantes con cuencos y frascos.
        for (int e = 0; e < 2; e++)
        {
            float y = 2.05f + (float)e * 0.75f;
            CajaBT({ (x0 + x1) * 0.5f, y, z + 0.2f }, { x1 - x0 - 0.8f, 0.06f, 0.4f }, ACERO_BT);

            for (int k = 0; k < 6; k++)
            {
                float px = x0 + 0.75f + (float)k * 0.55f;
                if (Ruido01BT(k + e * 7, 33) > 0.5f)
                {
                    CilBT({ px, y + 0.03f, z + 0.2f }, { px, y + 0.24f, z + 0.2f }, 0.13f, 0.13f, 10, VariarBT(Color{ 160, 210, 220, 255 }, k, 34 + e, 0.15f));
                    CilBT({ px, y + 0.24f, z + 0.2f }, { px, y + 0.29f, z + 0.2f }, 0.14f, 0.14f, 10, CIRUELA_BT);
                }
                else
                {
                    BolaBT({ px, y + 0.18f, z + 0.2f }, { 0.2f, 0.15f, 0.2f }, VariarBT(CREMA_BT, k, 35 + e, 0.06f), 2, 10);
                }
            }
        }

        // Marco saliente: jambas, dintel y techito.
        CajaBT({ x0 + 0.2f, 2.4f, z + hondo * 0.5f }, { 0.4f, 4.8f, hondo }, TonoBT(CREMA_BT, 0.95f));
        CajaBT({ x1 - 0.2f, 2.4f, z + hondo * 0.5f }, { 0.4f, 4.8f, hondo }, TonoBT(CREMA_BT, 0.95f));
        CajaBT({ (x0 + x1) * 0.5f, 4.15f, z + hondo * 0.5f }, { x1 - x0, 0.7f, hondo }, CREMA_BT);
        CajaBT({ (x0 + x1) * 0.5f, 3.82f, z + hondo + 0.02f }, { x1 - x0 - 0.1f, 0.1f, 0.06f }, TURQUESA_BT);
        CajaBT({ (x0 + x1) * 0.5f, 4.56f, z + hondo * 0.5f + 0.08f }, { x1 - x0 + 0.3f, 0.12f, hondo + 0.3f }, TURQUESA_OSCURA_BT);

        // Lamparas termicas bajo el dintel.
        for (int k = 0; k < 2; k++)
        {
            float lx = x0 + 1.4f + (float)k * 1.7f;
            CilBT({ lx, 3.8f, z + 0.42f }, { lx, 3.55f, z + 0.42f }, 0.08f, 0.26f, 12, ACERO_OSCURO_BT);
        }

        // Mostrador de acero y mueble bajo.
        CajaBT({ (x0 + x1) * 0.5f, 0.52f, z + 0.5f }, { x1 - x0, 1.04f, 1.0f }, TURQUESA_OSCURA_BT);
        CajaBT({ (x0 + x1) * 0.5f, 1.12f, z + 0.58f }, { x1 - x0 + 0.2f, 0.14f, 1.2f }, ACERO_BT);

        for (int k = 0; k < 3; k++)
        {
            float px = x0 + 0.8f + (float)k * 1.45f;
            CajaBT({ px, 0.55f, z + 1.01f }, { 1.25f, 0.8f, 0.03f }, TonoBT(TURQUESA_OSCURA_BT, 1.15f));
            CajaBT({ px, 0.85f, z + 1.05f }, { 0.5f, 0.05f, 0.05f }, ACERO_BT);
        }

        // Sobre el mostrador: platos con campana y una pila de bandejas.
        for (int k = 0; k < 2; k++)
        {
            float px = x0 + 1.0f + (float)k * 1.25f;
            CilBT({ px, 1.19f, z + 0.65f }, { px, 1.22f, z + 0.65f }, 0.34f, 0.34f, 14, Color{ 236, 238, 244, 255 });
            BolaBT({ px, 1.22f, z + 0.65f }, { 0.27f, 0.24f, 0.27f }, Color{ 214, 222, 234, 255 }, 3, 12, 0.0f);
            BolaBT({ px, 1.48f, z + 0.65f }, { 0.05f, 0.05f, 0.05f }, ACERO_OSCURO_BT, 2, 6);
        }

        for (int k = 0; k < 6; k++)
        {
            CajaBT({ x1 - 0.9f, 1.22f + (float)k * 0.055f, z + 0.65f }, { 0.9f, 0.04f, 0.6f }, k % 2 == 0 ? Color{ 206, 214, 228, 255 } : Color{ 190, 198, 214, 255 });
        }

        // Dispensador neumatico sobre la cocina: aca llegan los tubos.
        float dx = -13.65f;
        CajaBT({ dx, 5.35f, z + 0.35f }, { 1.6f, 1.3f, 0.7f }, ACERO_BT);
        CajaBT({ dx, 5.35f, z + 0.71f }, { 1.3f, 1.0f, 0.04f }, GRAFITO_BT);
        CilBT({ dx, 5.35f, z + 0.72f }, { dx, 5.35f, z + 0.78f }, 0.38f, 0.38f, 16, ACERO_OSCURO_BT);
        CilBT({ dx, 6.0f, z + 0.35f }, { dx, 6.1f, z + 0.35f }, 0.5f, 0.5f, 12, TURQUESA_OSCURA_BT);
    }

    //---- Cartel del menu, dispensador de bebidas y reciclado ----
    {
        // Marco del cartel (la pantalla se dibuja aparte, animada).
        float cx = (MENU_X0_BT + MENU_X1_BT) * 0.5f, cy = (MENU_Y0_BT + MENU_Y1_BT) * 0.5f;
        CajaBT({ cx, cy, z + 0.12f }, { MENU_X1_BT - MENU_X0_BT + 0.3f, MENU_Y1_BT - MENU_Y0_BT + 0.3f, 0.24f }, ACERO_OSCURO_BT);
        CajaBT({ cx, MENU_Y1_BT + 0.25f, z + 0.18f }, { MENU_X1_BT - MENU_X0_BT + 0.5f, 0.2f, 0.36f }, TURQUESA_BT);
        CilBT({ MENU_X0_BT + 0.4f, MENU_Y1_BT + 0.35f, z + 0.18f }, { MENU_X0_BT + 0.4f, 7.0f, z + 0.18f }, 0.03f, 0.03f, 4, ACERO_BT);
        CilBT({ MENU_X1_BT - 0.4f, MENU_Y1_BT + 0.35f, z + 0.18f }, { MENU_X1_BT - 0.4f, 7.0f, z + 0.18f }, 0.03f, 0.03f, 4, ACERO_BT);

        // Dispensador de bebidas: mueble con tres tanques de jugo.
        float bx0 = 11.6f, bx1 = 13.9f;
        CajaBT({ (bx0 + bx1) * 0.5f, 0.75f, z + 0.42f }, { bx1 - bx0, 1.5f, 0.84f }, CREMA_BT);
        CajaBT({ (bx0 + bx1) * 0.5f, 0.75f, z + 0.85f }, { bx1 - bx0 - 0.3f, 0.9f, 0.03f }, TonoBT(CREMA_BT, 0.9f));
        CajaBT({ (bx0 + bx1) * 0.5f, 1.53f, z + 0.45f }, { bx1 - bx0 + 0.1f, 0.08f, 0.94f }, TURQUESA_BT);
        CajaBT({ (bx0 + bx1) * 0.5f, 2.7f, z + 0.22f }, { bx1 - bx0, 0.25f, 0.44f }, TURQUESA_OSCURA_BT);

        for (int k = 0; k < 3; k++)
        {
            float tx = bx0 + 0.42f + (float)k * 0.72f;
            CilBT({ tx, 1.57f, z + 0.55f }, { tx, 1.65f, z + 0.55f }, 0.3f, 0.3f, 14, ACERO_OSCURO_BT);
            CilBT({ tx, 2.5f, z + 0.55f }, { tx, 2.58f, z + 0.55f }, 0.3f, 0.3f, 14, ACERO_OSCURO_BT);
            CajaBT({ tx, 1.25f, z + 0.9f }, { 0.12f, 0.12f, 0.12f }, ACERO_BT);
            CajaBT({ tx, 0.42f, z + 0.92f }, { 0.5f, 0.04f, 0.2f }, ACERO_OSCURO_BT);
        }

        // Cesto de reciclado.
        float rx = 15.0f;
        CilBT({ rx, 0.0f, z + 0.6f }, { rx, 1.1f, z + 0.6f }, 0.42f, 0.46f, 14, TURQUESA_OSCURA_BT);
        BolaBT({ rx, 1.1f, z + 0.6f }, { 0.48f, 0.2f, 0.48f }, TURQUESA_BT, 2, 14, 0.0f);
        CajaBT({ rx, 1.25f, z + 0.98f }, { 0.4f, 0.06f, 0.1f }, GRAFITO_BT);
    }

    //---- Torres hidroponicas a los lados del ventanal ----
    TorreHidroponicaBT(-TORRE_X_BT, TORRE_Z_BT, 41, paso);
    TorreHidroponicaBT(TORRE_X_BT, TORRE_Z_BT, 47, paso);

    //---- Mesita del salon con banquitos (las tazas flotan aparte) ----
    {
        Vector3 c = MESITA_BT;
        CilBT({ c.x, 0.0f, c.z }, { c.x, 0.08f, c.z }, 0.55f, 0.5f, 16, GRAFITO_BT);
        CilBT({ c.x, 0.08f, c.z }, { c.x, 0.3f, c.z }, 0.3f, 0.2f, 12, ACERO_OSCURO_BT);
        CilBT({ c.x, 0.3f, c.z }, { c.x, 0.34f, c.z }, 0.22f, 0.22f, 12, TURQUESA_BT);
        CilBT({ c.x, 0.86f, c.z }, { c.x, 0.96f, c.z }, 0.95f, 0.95f, 20, Color{ 236, 232, 222, 255 });
        CilBT({ c.x, 0.8f, c.z }, { c.x, 0.86f, c.z }, 0.88f, 0.9f, 20, TURQUESA_BT);

        if (paso == 1)
        {
            BanquitoBT(c.x - 1.45f, c.z + 0.15f);
            BanquitoBT(c.x + 1.45f, c.z + 0.15f);
        }
    }

    //---- Soportes de los tubos neumaticos ----
    for (int k = 0; k < 11; k++)
    {
        float x = -13.65f + (float)k * 2.95f;
        CilBT({ x, TECHO_Y_BT, TUBO_Z_BT }, { x, TUBO_Y_BT[0] - 0.22f, TUBO_Z_BT }, 0.035f, 0.035f, 4, ACERO_OSCURO_BT);

        for (int t = 0; t < 2; t++)
        {
            Vector3 c = { x, TUBO_Y_BT[t], TUBO_Z_BT };
            CilBT({ x - 0.07f, c.y, c.z }, { x + 0.07f, c.y, c.z }, 0.2f, 0.2f, 10, ACERO_BT);
        }
    }

    // Codos de bajada a la cocina.
    for (int t = 0; t < 2; t++)
    {
        float y = TUBO_Y_BT[t];
        float dx = -13.65f + (t == 0 ? -0.32f : 0.32f);
        CilBT({ dx, y - 0.02f, TUBO_Z_BT }, { dx, y + 0.02f, TUBO_Z_BT }, 0.21f, 0.21f, 10, ACERO_BT);
    }

    FinLoteBT();
}


//--------------------------------------------------
// Camara y espacio (2D, antes del 3D)
//--------------------------------------------------

static Camera3D CamaraVistaBT(const MinijuegoBanqueteTurbo& m)
{
    Camera3D c = m.camara;
    c.position = OJO_BT;
    c.target = MIRA_BT;
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = FOV_BT;
    c.projection = CAMERA_PERSPECTIVE;

    // Con menos comensales la camara se acerca para que las mesas llenen
    // el cuadro (4 mesas: tramo de 8.3 a cada lado; 2 mesas: 4.2).
    float tramo = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (m.resultado.participantes[i].participo && std::fabs(m.posicionX[i]) > tramo) tramo = std::fabs(m.posicionX[i]);
    }

    float acercar = Lim01BT((6.3f - tramo) / 4.2f) * 3.2f;
    Vector3 haciaMira = Vector3Normalize(Vector3Subtract(MIRA_BT, OJO_BT));
    c.position = Vector3Add(c.position, Vector3Scale(haciaMira, acercar));
    c.target.y -= acercar * 0.06f;

    // Entrada: la camara entra al comedor desde atras y de costado
    // durante la cuenta regresiva.
    if (m.fase == FASE_BANQUETE_PREPARACION && m.resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        float u = Lim01BT(1.0f - m.tiempoPreparacion / DURACION_PREPARACION_BANQUETE);
        float e = 1.0f - u;
        e = e * e * (3.0f - 2.0f * e) * ObtenerFactorMovimientoCamaraMinijuegos();
        c.position.z += 6.0f * e;
        c.position.y += 1.6f * e;
        c.position.x += 4.5f * e;
        c.target.x += 1.2f * e;
    }

    return AplicarTemblorGeneralACamara(c);
}


static void DibujarEspacioBT(const Camera3D& vista, float t, bool decoracion)
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float s = (float)h / 720.0f;

    DrawRectangleGradientV(0, 0, w, h, ESPACIO_ALTO_BT, ESPACIO_BT);

    // Estrellas fijas al cielo (proyectadas desde muy lejos).
    int cantidad = decoracion ? 260 : 120;

    for (int k = 0; k < cantidad; k++)
    {
        Vector3 p = { -120.0f + 240.0f * Ruido01BT(k, 51), -10.0f + 90.0f * Ruido01BT(k, 52), -150.0f };
        Vector2 q = GetWorldToScreen(p, vista);

        if (q.x < 0.0f || q.y < 0.0f || q.x > (float)w || q.y > (float)h) continue;

        float brillo = 0.45f + 0.55f * Ruido01BT(k, 53);
        if (decoracion && k % 5 == 0) brillo *= 0.6f + 0.4f * std::sin(t * (1.5f + Ruido01BT(k, 54) * 2.0f) + (float)k);
        float r = (Ruido01BT(k, 55) > 0.92f ? 2.0f : 1.0f) * s;
        Color c = MezclaBT(Color{ 200, 214, 255, 255 }, Color{ 255, 236, 210, 255 }, Ruido01BT(k, 56));
        DrawRectangle((int)q.x, (int)q.y, (int)r + 1, (int)r + 1, AlfaBT(c, brillo));
    }

    // Luna en lo alto, a la izquierda del ventanal.
    Vector2 luna = GetWorldToScreen({ -34.0f, 30.0f, -110.0f }, vista);
    float rl = 24.0f * s;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(luna, rl * 2.6f, Fade(Color{ 160, 180, 230, 255 }, 0.16f), Fade(Color{ 160, 180, 230, 255 }, 0.0f));
    EndBlendMode();
    DrawCircleV(luna, rl, Color{ 214, 214, 222, 255 });
    DrawCircleV({ luna.x - rl * 0.3f, luna.y + rl * 0.2f }, rl * 0.28f, Color{ 180, 180, 192, 255 });
    DrawCircleV({ luna.x + rl * 0.35f, luna.y - rl * 0.3f }, rl * 0.18f, Color{ 186, 186, 198, 255 });
    DrawCircleV({ luna.x + rl * 0.1f, luna.y + rl * 0.5f }, rl * 0.14f, Color{ 184, 184, 196, 255 });
    // Lado en sombra (el sol esta a la derecha).
    DrawCircleSector(luna, rl + 1.0f, 100.0f, 260.0f, 24, Fade(Color{ 20, 22, 40, 255 }, 0.55f));

    // Sol fuera de cuadro arriba a la derecha: resplandor.
    Vector2 sol = GetWorldToScreen({ 60.0f, 34.0f, -120.0f }, vista);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(sol, 260.0f * s, Fade(Color{ 255, 236, 200, 255 }, 0.22f), Fade(Color{ 255, 236, 200, 255 }, 0.0f));
    DrawCircleGradient(sol, 70.0f * s, Fade(Color{ 255, 250, 236, 255 }, 0.7f), Fade(Color{ 255, 250, 236, 255 }, 0.0f));
    EndBlendMode();
}


//--------------------------------------------------
// Raciones
//--------------------------------------------------

// Medidas de la mesa magnetica (centrada en posicionX, ZONA_MESA).
static const float MESA_MX_BT = 1.55f;
static const float MESA_MZ_BT = 0.8f;
static const float MESA_Y_BT = 0.9f;         // cara de arriba de la losa
static const float GROSOR_MESA_BT = 0.13f;


// Tubo de comida espacial tendido sobre la bandeja. (x, y, z) es el centro
// del tubo entero; fraccion acorta el tubo desde la punta mordida; giro
// (radianes) lo hace dar vueltas al salir despedido.
static void DibujarRacionBT(
    Vector3 c,
    const RacionBanquete& racion,
    float fraccion,
    float t,
    float escala,
    float giro,
    bool detalle
)
{
    float radio = 0.19f * escala;
    float largoTotal = LARGO_TUBO_BANQUETE * 0.92f * escala;
    float largo = largoTotal * (0.08f + 0.92f * Lim01BT(fraccion));
    Vector3 eje = { std::cos(giro), std::sin(giro), 0.0f };
    Vector3 arriba = { -std::sin(giro), std::cos(giro), 0.0f };
    Vector3 a = Vector3Add(c, Vector3Scale(eje, -largoTotal * 0.5f));
    Vector3 b = Vector3Add(a, Vector3Scale(eje, largo));
    int lados = detalle ? 12 : 8;

    Color cuerpo = ColorRacionBanquete(racion);
    bool picante = racion.tipo == RACION_BANQUETE_PICANTE;
    bool dorada = racion.tipo == RACION_BANQUETE_DORADA;

    if (picante)
    {
        float latido = 0.5f + 0.5f * std::sin(t * 14.0f);
        cuerpo = MezclaBT(PICANTE_BT, Color{ 255, 96, 60, 255 }, latido * 0.6f);
    }
    else if (dorada)
    {
        cuerpo = DORADA_BT;
    }

    Color tapa = dorada ? Color{ 255, 236, 160, 255 } : Color{ 214, 220, 232, 255 };

    // Tapa de aluminio con el pliegue y el cuerpo del tubo.
    CilBT(Vector3Add(a, Vector3Scale(eje, -0.16f * escala)), a, radio * 0.78f, radio * 1.02f, lados, tapa);
    CilBT(Vector3Add(a, Vector3Scale(eje, -0.2f * escala)), Vector3Add(a, Vector3Scale(eje, -0.15f * escala)), radio * 0.55f, radio * 0.55f, lados, TonoBT(tapa, 0.8f));
    CilBT(a, b, radio, radio, lados, cuerpo, false);

    // Punta mordida: cupula mas oscura con dos mordiscos.
    Color relleno = TonoBT(MezclaBT(cuerpo, Color{ 255, 240, 220, 255 }, 0.25f), 0.85f);
    BolaBT(b, { radio * 0.55f + std::fabs(eje.x) * 0.0f, radio * 0.96f, radio * 0.96f }, relleno, 3, lados);

    if (fraccion < 0.98f)
    {
        BolaBT(Vector3Add(b, Vector3Add(Vector3Scale(arriba, radio * 0.7f), Vector3Scale(eje, -0.02f))), { radio * 0.35f, radio * 0.35f, radio * 0.35f }, TonoBT(relleno, 0.75f), 2, 6);
        BolaBT(Vector3Add(b, Vector3Add(Vector3Scale(arriba, -radio * 0.5f), { 0.0f, 0.0f, radio * 0.6f })), { radio * 0.3f, radio * 0.3f, radio * 0.3f }, TonoBT(relleno, 0.75f), 2, 6);
    }

    // Etiqueta del envase (si todavia queda tubo debajo).
    float pe = largoTotal * 0.22f;

    if (largo > pe + 0.14f * escala)
    {
        Vector3 e0 = Vector3Add(a, Vector3Scale(eje, pe));
        Vector3 e1 = Vector3Add(a, Vector3Scale(eje, pe + 0.14f * escala));
        Color etiqueta = picante ? Color{ 255, 214, 70, 255 } : (dorada ? Color{ 255, 252, 236, 255 } : Color{ 246, 240, 226, 255 });
        CilBT(e0, e1, radio * 1.06f, radio * 1.06f, lados, etiqueta, false);

        if (picante)
        {
            // Segunda franja: llamas naranjas.
            Vector3 f0 = Vector3Add(a, Vector3Scale(eje, pe + 0.24f * escala));
            Vector3 f1 = Vector3Add(a, Vector3Scale(eje, pe + 0.32f * escala));
            if (largo > pe + 0.32f * escala) CilBT(f0, f1, radio * 1.05f, radio * 1.05f, lados, Color{ 255, 140, 30, 255 }, false);
        }
    }

    // Brillo especular (franja clara a lo largo).
    Vector3 brillo0 = Vector3Add(a, Vector3Add(Vector3Scale(arriba, radio * 0.8f), { 0.0f, 0.0f, radio * 0.45f }));
    Vector3 brillo1 = Vector3Add(brillo0, Vector3Scale(eje, largo * 0.85f));
    VigaBT(brillo0, brillo1, radio * 0.28f, radio * 0.12f, MezclaBT(cuerpo, WHITE, dorada ? 0.75f : 0.55f));

    // Icono flotante: aji picante o estrella dorada.
    Vector3 icono = Vector3Add(c, Vector3Scale(arriba, radio + 0.42f * escala + 0.05f * std::sin(t * 4.0f)));

    if (picante)
    {
        float sacudida = 0.06f * std::sin(t * 30.0f);
        Vector3 p0 = { icono.x - 0.22f * escala + sacudida, icono.y + 0.08f * escala, icono.z };
        Vector3 p1 = { icono.x - 0.02f * escala + sacudida, icono.y, icono.z };
        Vector3 p2 = { icono.x + 0.16f * escala + sacudida, icono.y - 0.1f * escala, icono.z };
        Vector3 p3 = { icono.x + 0.26f * escala + sacudida, icono.y - 0.24f * escala, icono.z };
        Color aji = Color{ 230, 30, 26, 255 };
        CilBT(p0, p1, 0.1f * escala, 0.11f * escala, 8, aji);
        CilBT(p1, p2, 0.11f * escala, 0.08f * escala, 8, aji);
        CilBT(p2, p3, 0.08f * escala, 0.01f * escala, 8, TonoBT(aji, 0.9f));
        CilBT(p0, { p0.x - 0.1f * escala, p0.y + 0.12f * escala, p0.z }, 0.05f * escala, 0.03f * escala, 6, Color{ 70, 170, 60, 255 });
        BolaBT({ p0.x - 0.02f * escala, p0.y + 0.03f * escala, p0.z }, { 0.09f * escala, 0.05f * escala, 0.09f * escala }, Color{ 60, 150, 54, 255 }, 2, 6);
    }
    else if (dorada)
    {
        float g = t * 2.4f;
        Vector3 ux = { std::cos(g) * 0.26f * escala, 0.0f, std::sin(g) * 0.26f * escala };
        Vector3 uy = { 0.0f, 0.26f * escala, 0.0f };

        for (int k = 0; k < 5; k++)
        {
            float a0 = PI * 0.5f + 2.0f * PI * (float)k / 5.0f;
            float a1 = a0 + 2.0f * PI / 10.0f;
            float a2 = a0 - 2.0f * PI / 10.0f;
            Vector3 punta = Vector3Add(icono, Vector3Add(Vector3Scale(ux, std::cos(a0)), Vector3Scale(uy, std::sin(a0))));
            Vector3 v1 = Vector3Add(icono, Vector3Add(Vector3Scale(ux, std::cos(a1) * 0.42f), Vector3Scale(uy, std::sin(a1) * 0.42f)));
            Vector3 v2 = Vector3Add(icono, Vector3Add(Vector3Scale(ux, std::cos(a2) * 0.42f), Vector3Scale(uy, std::sin(a2) * 0.42f)));
            TriBT(icono, v2, punta, Color{ 255, 226, 90, 255 });
            TriBT(icono, punta, v1, Color{ 255, 196, 40, 255 });
        }
    }
}


//--------------------------------------------------
// Mesas magneticas, bandejas y comensales
//--------------------------------------------------

static float FlotacionMesaBT(int i, float t)
{
    return 0.018f * std::sin(t * 2.2f + (float)i * 1.7f);
}


static void DibujarMesaBT(const MinijuegoBanqueteTurbo& m, int i, Color colorJugador, bool detalle)
{
    const PuestoBanquete& p = m.puestos[i];
    float t = m.tiempoAnimacion;
    float x = m.posicionX[i];
    float z = ZONA_MESA_BANQUETE;
    float f = FlotacionMesaBT(i, t);
    float yTapa = MESA_Y_BT + f;

    // Base magnetica en el piso y pedestal corto.
    CilBT({ x, 0.0f, z }, { x, 0.12f, z }, 0.82f, 0.78f, 20, GRAFITO_BT);
    CilBT({ x, 0.12f, z }, { x, 0.34f, z }, 0.56f, 0.4f, 16, ACERO_OSCURO_BT);
    CilBT({ x, 0.34f, z }, { x, 0.38f, z }, 0.42f, 0.42f, 16, TURQUESA_BT);

    // Losa flotante octogonal con canto del color del jugador.
    PrismaChaflanBT({ x, yTapa - GROSOR_MESA_BT, z }, MESA_MX_BT, MESA_MZ_BT, 0.32f, GROSOR_MESA_BT, Color{ 236, 232, 222, 255 }, ACERO_BT);
    PrismaChaflanBT({ x, yTapa - GROSOR_MESA_BT - 0.05f, z }, MESA_MX_BT - 0.08f, MESA_MZ_BT - 0.08f, 0.28f, 0.05f, colorJugador, colorJugador);
    // Emisor bajo la losa.
    CilBT({ x, yTapa - GROSOR_MESA_BT - 0.16f, z }, { x, yTapa - GROSOR_MESA_BT - 0.05f, z }, 0.3f, 0.46f, 16, ACERO_OSCURO_BT);

    // Faldon delantero: tablero de bocados (segmentos encendidos = bocados
    // que faltan), enmarcado con el color del jugador.
    float zf = z + MESA_MZ_BT + 0.01f;
    float y1 = yTapa - GROSOR_MESA_BT - 0.02f, y0 = y1 - 0.28f;
    CajaBT({ x, (y0 + y1) * 0.5f, zf - 0.03f }, { 2.1f, y1 - y0 + 0.06f, 0.05f }, colorJugador);
    CajaBT({ x, (y0 + y1) * 0.5f, zf }, { 1.96f, y1 - y0 - 0.04f, 0.03f }, Color{ 18, 22, 36, 255 });

    const RacionBanquete& racion = m.secuencia[p.indiceRacion >= 0 ? p.indiceRacion : 0];
    int total = p.bocadosRacion > 0 ? p.bocadosRacion : 1;
    float ancho = 1.82f / (float)total;
    Color encendido = ColorRacionBanquete(racion);

    if (racion.tipo == RACION_BANQUETE_PICANTE && std::fmod(t * 6.0f, 1.0f) < 0.5f) encendido = Color{ 255, 150, 60, 255 };

    g_emisBT = 1.0f;

    for (int k = 0; k < total; k++)
    {
        bool lleno = k < p.bocadosRestantes;
        float sx = x - 0.91f + ancho * ((float)k + 0.5f);
        Color c = lleno ? encendido : Color{ 40, 46, 64, 255 };
        QuadColBT({ sx - ancho * 0.36f, y0 + 0.06f, zf + 0.02f }, { sx + ancho * 0.36f, y0 + 0.06f, zf + 0.02f },
            { sx + ancho * 0.36f, y1 - 0.06f, zf + 0.02f }, { sx - ancho * 0.36f, y1 - 0.06f, zf + 0.02f }, c, c, c, c);
    }

    g_emisBT = 0.0f;

    // Bandeja con borde y compartimentos.
    float yb = yTapa;
    Color bandeja = Color{ 206, 214, 228, 255 };
    CajaBT({ x, yb + 0.02f, z + 0.02f }, { 2.2f, 0.04f, 1.12f }, bandeja);
    CajaBT({ x, yb + 0.06f, z + 0.58f }, { 2.2f, 0.06f, 0.05f }, TonoBT(bandeja, 0.9f));
    CajaBT({ x, yb + 0.06f, z - 0.54f }, { 2.2f, 0.06f, 0.05f }, TonoBT(bandeja, 0.9f));
    CajaBT({ x - 1.1f, yb + 0.06f, z + 0.02f }, { 0.05f, 0.06f, 1.12f }, TonoBT(bandeja, 0.9f));
    CajaBT({ x + 1.1f, yb + 0.06f, z + 0.02f }, { 0.05f, 0.06f, 1.12f }, TonoBT(bandeja, 0.9f));

    if (detalle)
    {
        // Vaso magnetico con pajita y potecito de postre en la fila de atras.
        Vector3 vaso = { x + 0.82f, yb + 0.04f, z - 0.3f };
        CilBT(vaso, { vaso.x, vaso.y + 0.3f, vaso.z }, 0.1f, 0.12f, 10, Color{ 230, 236, 244, 255 });
        CilBT({ vaso.x, vaso.y + 0.3f, vaso.z }, { vaso.x, vaso.y + 0.33f, vaso.z }, 0.12f, 0.12f, 10, TURQUESA_BT);
        CilBT({ vaso.x + 0.02f, vaso.y + 0.3f, vaso.z }, { vaso.x + 0.08f, vaso.y + 0.52f, vaso.z }, 0.015f, 0.015f, 4, Color{ 255, 120, 170, 255 });
        Vector3 pote = { x - 0.84f, yb + 0.04f, z - 0.32f };
        CilBT(pote, { pote.x, pote.y + 0.12f, pote.z }, 0.12f, 0.14f, 10, CIRUELA_BT);
        BolaBT({ pote.x, pote.y + 0.12f, pote.z }, { 0.12f, 0.08f, 0.12f }, Color{ 250, 236, 214, 255 }, 2, 8, 0.0f);
    }

    // Cola de proximas raciones en la repisa de atras (avisa de las picantes).
    for (int k = 1; k <= 2; k++)
    {
        const RacionBanquete& siguiente = m.secuencia[(p.indiceRacion + k) % LARGO_SECUENCIA_BANQUETE];
        Vector3 q = { x - 0.32f + (float)(k - 1) * 0.6f, yb + 0.12f, z - 0.36f };
        DibujarRacionBT(q, siguiente, 1.0f, t, 0.3f, 0.0f, false);
    }

    // Racion actual: desliza desde el robot hasta la bandeja.
    float llegada = Lim01BT(p.llegada);
    float inversa = 1.0f - llegada;
    float fraccion = p.bocadosRacion > 0 ? (float)p.bocadosRestantes / (float)p.bocadosRacion : 1.0f;
    float pulso = 1.0f + 0.1f * Lim01BT(p.pulsoBocado);

    if (p.bocadosRestantes > 0 && p.indiceRacion >= 0)
    {
        Vector3 c = { x, ALTURA_BANDEJA_BANQUETE + 0.2f + f + inversa * inversa * 0.9f, z + 0.12f - inversa * 2.4f };
        DibujarRacionBT(c, racion, fraccion, t, pulso, 0.0f, detalle);
    }

    // Racion rechazada saliendo despedida hacia atras, dando vueltas.
    if (p.salida > 0.0f)
    {
        float avance = 1.0f - p.salida / DURACION_RECHAZO_BANQUETE;
        Vector3 c = { x + avance * 0.6f, ALTURA_BANDEJA_BANQUETE + 0.2f + avance * 2.4f - avance * avance * 0.6f, z - avance * 2.2f };
        DibujarRacionBT(c, p.racionSalida, 1.0f, t, 1.0f - avance * 0.4f, avance * 7.0f, false);
    }
}


//--------------------------------------------------
// Robot camarero
//--------------------------------------------------

static void DibujarRobotBT(const MinijuegoBanqueteTurbo& m, bool detalle)
{
    float t = m.tiempoAnimacion;
    float inclinacion = Lim01BT(std::fabs(m.robotObjetivoX - m.robotX) / 4.0f) * (m.robotObjetivoX > m.robotX ? -1.0f : 1.0f);
    float x = m.robotX;
    float y = 2.45f + 0.1f * std::sin(t * 2.5f);
    float z = CARRIL_Z_BT;
    float lean = inclinacion * 0.28f;                // la cabeza se adelanta al moverse
    Color esmalte = Color{ 240, 238, 230, 255 };
    Color oscuro = Color{ 26, 32, 50, 255 };

    // Propulsor: tobera bajo el cuerpo.
    CilBT({ x, y - 0.95f, z }, { x, y - 0.55f, z }, 0.26f, 0.42f, 14, ACERO_OSCURO_BT);
    CilBT({ x, y - 1.0f, z }, { x, y - 0.95f, z }, 0.22f, 0.26f, 14, LUZ_FRIA_BT);

    // Cuerpo de huevo con cinturon turquesa y pechera.
    BolaBT({ x + lean * 0.3f, y, z }, { 0.58f, 0.66f, 0.5f }, esmalte, detalle ? 6 : 4, detalle ? 14 : 10);
    CilBT({ x + lean * 0.3f, y - 0.2f, z }, { x + lean * 0.3f, y - 0.06f, z }, 0.6f, 0.6f, 16, TURQUESA_BT, false);
    CajaBT({ x + lean * 0.35f, y + 0.18f, z + 0.47f }, { 0.44f, 0.3f, 0.04f }, oscuro);

    // Corazon/pantalla del pecho que late.
    g_emisBT = 1.0f;
    float latido = 0.6f + 0.4f * std::sin(t * 5.0f);
    CajaBT({ x + lean * 0.35f, y + 0.18f, z + 0.5f }, { 0.3f * latido, 0.06f, 0.02f }, LUZ_FRIA_BT);
    CajaBT({ x + lean * 0.35f, y + 0.18f, z + 0.5f }, { 0.06f, 0.18f * latido, 0.02f }, LUZ_FRIA_BT);
    g_emisBT = 0.0f;

    // Monito de mozo.
    Vector3 cuello = { x + lean * 0.6f, y + 0.6f, z + 0.3f };
    TriBT(cuello, { cuello.x - 0.2f, cuello.y + 0.09f, cuello.z + 0.06f }, { cuello.x - 0.2f, cuello.y - 0.09f, cuello.z + 0.06f }, oscuro);
    TriBT(cuello, { cuello.x + 0.2f, cuello.y - 0.09f, cuello.z + 0.06f }, { cuello.x + 0.2f, cuello.y + 0.09f, cuello.z + 0.06f }, oscuro);
    BolaBT({ cuello.x, cuello.y, cuello.z + 0.04f }, { 0.05f, 0.05f, 0.05f }, Color{ 60, 66, 90, 255 }, 2, 6);

    // Cabeza: cupula con visor oscuro, ojos y orejas.
    Vector3 cabeza = { x + lean, y + 0.98f, z };
    CilBT({ x + lean * 0.7f, y + 0.6f, z }, { x + lean * 0.9f, y + 0.72f, z }, 0.14f, 0.14f, 8, ACERO_OSCURO_BT);
    BolaBT(cabeza, { 0.42f, 0.36f, 0.38f }, esmalte, detalle ? 5 : 4, detalle ? 14 : 10);
    BolaBT({ cabeza.x, cabeza.y, cabeza.z + 0.12f }, { 0.34f, 0.24f, 0.28f }, oscuro, 3, 12);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        CilBT({ cabeza.x + (float)lado * 0.38f, cabeza.y, cabeza.z }, { cabeza.x + (float)lado * 0.48f, cabeza.y, cabeza.z }, 0.12f, 0.1f, 10, TURQUESA_BT);
    }

    // Ojos: parpadean y, al servir, se vuelven arcos contentos.
    g_emisBT = 1.0f;
    bool parpado = std::fmod(t, 3.1f) < 0.12f;
    bool contento = m.robotBrazo > 0.0f;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 ojo = { cabeza.x + (float)lado * 0.13f, cabeza.y + 0.02f, cabeza.z + 0.39f };

        if (contento)
        {
            TiraBT({ ojo.x - 0.07f, ojo.y - 0.02f, ojo.z }, { ojo.x, ojo.y + 0.04f, ojo.z }, 0.035f, 0.035f, LUZ_FRIA_BT, LUZ_FRIA_BT);
            TiraBT({ ojo.x, ojo.y + 0.04f, ojo.z }, { ojo.x + 0.07f, ojo.y - 0.02f, ojo.z }, 0.035f, 0.035f, LUZ_FRIA_BT, LUZ_FRIA_BT);
        }
        else
        {
            float alto = parpado ? 0.012f : 0.07f;
            QuadColBT({ ojo.x - 0.045f, ojo.y - alto, ojo.z }, { ojo.x + 0.045f, ojo.y - alto, ojo.z },
                { ojo.x + 0.045f, ojo.y + alto, ojo.z }, { ojo.x - 0.045f, ojo.y + alto, ojo.z },
                LUZ_FRIA_BT, LUZ_FRIA_BT, LUZ_FRIA_BT, LUZ_FRIA_BT);
        }
    }

    // Antena con foco ambar que titila.
    Vector3 punta = { cabeza.x + lean * 0.4f, cabeza.y + 0.62f, cabeza.z };
    CilBT({ cabeza.x, cabeza.y + 0.32f, cabeza.z }, punta, 0.025f, 0.02f, 6, ACERO_BT);
    Color foco = std::fmod(t * 1.6f, 1.0f) < 0.5f ? LUZ_SERVICIO_BT : TonoBT(LUZ_SERVICIO_BT, 0.45f);
    BolaBT(punta, { 0.07f, 0.07f, 0.07f }, foco, 3, 8);
    g_emisBT = 0.0f;

    // Brazo izquierdo: bandeja con campana en alto.
    Vector3 hombroI = { x - 0.6f, y + 0.22f, z };
    Vector3 manoI = { x - 0.95f, y + 0.55f, z + 0.15f };
    CilBT(hombroI, { x - 0.88f, y + 0.0f, z + 0.1f }, 0.09f, 0.08f, 8, ACERO_BT);
    CilBT({ x - 0.88f, y + 0.0f, z + 0.1f }, manoI, 0.08f, 0.07f, 8, ACERO_BT);
    CilBT({ manoI.x, manoI.y, manoI.z }, { manoI.x, manoI.y + 0.04f, manoI.z }, 0.42f, 0.42f, 16, Color{ 200, 208, 222, 255 });
    BolaBT({ manoI.x, manoI.y + 0.04f, manoI.z }, { 0.32f, 0.26f, 0.32f }, Color{ 220, 226, 238, 255 }, 3, 12, 0.0f);
    BolaBT({ manoI.x, manoI.y + 0.32f, manoI.z }, { 0.05f, 0.05f, 0.05f }, ACERO_OSCURO_BT, 2, 6);

    // Brazo derecho telescopico: se estira hasta la mesa que acaba de servir.
    float extension = Lim01BT(m.robotBrazo * 2.0f);
    extension = extension * extension * (3.0f - 2.0f * extension);
    Vector3 hombroD = { x + 0.6f, y + 0.22f, z };
    Vector3 reposo = { x + 0.86f, y - 0.25f, z + 0.25f };
    Vector3 destino = { x + 0.3f, ALTURA_BANDEJA_BANQUETE + 0.55f, z + (ZONA_MESA_BANQUETE - z) * 0.78f };
    Vector3 manoD = Vector3Lerp(reposo, destino, extension);
    Vector3 tramo1 = Vector3Lerp(hombroD, manoD, 0.4f);
    Vector3 tramo2 = Vector3Lerp(hombroD, manoD, 0.72f);
    CilBT(hombroD, tramo1, 0.1f, 0.1f, 8, ACERO_BT);
    CilBT(tramo1, tramo2, 0.08f, 0.08f, 8, TonoBT(ACERO_BT, 1.08f));
    CilBT(tramo2, manoD, 0.065f, 0.065f, 8, TonoBT(ACERO_BT, 1.15f));
    BolaBT(tramo1, { 0.11f, 0.11f, 0.11f }, TURQUESA_BT, 2, 8);
    BolaBT(tramo2, { 0.09f, 0.09f, 0.09f }, TURQUESA_BT, 2, 8);
    BolaBT(manoD, { 0.12f, 0.1f, 0.12f }, esmalte, 2, 8);

    for (int k = -1; k <= 1; k += 2)
    {
        CilBT(manoD, { manoD.x + (float)k * 0.08f, manoD.y - 0.14f, manoD.z + 0.06f }, 0.03f, 0.02f, 4, ACERO_OSCURO_BT);
    }
}


//--------------------------------------------------
// Vida de la sala (opaco, por frame)
//--------------------------------------------------

// Recorrido del tubo neumatico t (0..1 desde la pared derecha hasta la cocina).
static Vector3 PuntoTuboBT(int tubo, float s)
{
    float y = TUBO_Y_BT[tubo];
    float dx = -13.65f + (tubo == 0 ? -0.32f : 0.32f);
    Vector3 p[4] =
    {
        { SALA_X_BT, y, TUBO_Z_BT }, { dx, y, TUBO_Z_BT },
        { dx, y, PARED_Z_BT + 0.35f }, { dx, 6.08f, PARED_Z_BT + 0.35f }
    };
    float largos[3], total = 0.0f;

    for (int k = 0; k < 3; k++)
    {
        largos[k] = Vector3Distance(p[k], p[k + 1]);
        total += largos[k];
    }

    float d = Lim01BT(s) * total;

    for (int k = 0; k < 3; k++)
    {
        if (d <= largos[k] || k == 2)
        {
            return Vector3Lerp(p[k], p[k + 1], Lim01BT(d / largos[k]));
        }

        d -= largos[k];
    }

    return p[3];
}


// Rectangulo emisivo en el plano z (pantallas).
static void RectZBT(float x0, float y0, float x1, float y1, float z, Color c)
{
    QuadColBT({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, c, c, c, c);
}


static void DiscoZBT(float cx, float cy, float r, float z, Color c, int lados = 12)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados, a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        VBT({ cx, cy, z }, c);
        VBT({ cx + std::cos(a0) * r, cy + std::sin(a0) * r, z }, c);
        VBT({ cx + std::cos(a1) * r, cy + std::sin(a1) * r, z }, c);
    }
}


// Pantalla del menu: ensena las tres raciones (normal 1, picante: salta,
// dorada x2) con un barrido de brillo.
static void DibujarMenuBT(float t)
{
    float z = PARED_Z_BT + 0.25f;
    float x0 = MENU_X0_BT, x1 = MENU_X1_BT, y0 = MENU_Y0_BT, y1 = MENU_Y1_BT;
    float ancho = (x1 - x0) / 3.0f;

    RectZBT(x0, y0, x1, y1, z, Color{ 16, 22, 40, 255 });

    for (int k = 0; k < 3; k++)
    {
        float px0 = x0 + (float)k * ancho + 0.08f, px1 = px0 + ancho - 0.16f;
        float cx = (px0 + px1) * 0.5f, cy = (y0 + y1) * 0.5f + 0.25f;
        bool resalta = (int)std::fmod(t * 0.6f, 3.0f) == k;
        Color fondo = resalta ? Color{ 36, 52, 84, 255 } : Color{ 24, 32, 56, 255 };
        RectZBT(px0, y0 + 0.08f, px1, y1 - 0.08f, z + 0.005f, fondo);

        if (k == 0)
        {
            // Tubo normal y un punto.
            Color c = Color{ 90, 220, 140, 255 };
            RectZBT(cx - 0.42f, cy - 0.16f, cx + 0.42f, cy + 0.16f, z + 0.01f, c);
            DiscoZBT(cx + 0.42f, cy, 0.16f, z + 0.01f, c);
            RectZBT(cx - 0.56f, cy - 0.13f, cx - 0.42f, cy + 0.13f, z + 0.01f, Color{ 214, 220, 232, 255 });
            DiscoZBT(cx, cy - 0.75f, 0.11f, z + 0.01f, LUZ_SERVICIO_BT);
        }
        else if (k == 1)
        {
            // Aji con flecha de salto.
            bool alerta = std::fmod(t * 3.0f, 1.0f) < 0.5f;
            Color c = alerta ? PICANTE_BT : Color{ 255, 120, 70, 255 };
            DiscoZBT(cx - 0.2f, cy + 0.05f, 0.17f, z + 0.01f, c);
            DiscoZBT(cx + 0.05f, cy - 0.03f, 0.15f, z + 0.01f, c);
            DiscoZBT(cx + 0.27f, cy - 0.15f, 0.1f, z + 0.01f, c);
            RectZBT(cx - 0.36f, cy + 0.12f, cx - 0.26f, cy + 0.32f, z + 0.012f, Color{ 80, 190, 80, 255 });
            // Flecha hacia arriba = salta.
            Color f = Color{ 240, 244, 255, 255 };
            float fy = cy - 0.85f + 0.08f * std::sin(t * 6.0f);
            QuadColBT({ cx - 0.06f, fy - 0.2f, z + 0.01f }, { cx + 0.06f, fy - 0.2f, z + 0.01f }, { cx + 0.06f, fy + 0.08f, z + 0.01f }, { cx - 0.06f, fy + 0.08f, z + 0.01f }, f, f, f, f);
            VBT({ cx - 0.2f, fy + 0.06f, z + 0.01f }, f); VBT({ cx + 0.2f, fy + 0.06f, z + 0.01f }, f); VBT({ cx, fy + 0.28f, z + 0.01f }, f);
            if (alerta) RectZBT(px0, y1 - 0.16f, px1, y1 - 0.08f, z + 0.012f, PICANTE_BT);
        }
        else
        {
            // Estrella dorada y dos puntos.
            for (int s = 0; s < 5; s++)
            {
                float a0 = PI * 0.5f + 2.0f * PI * (float)s / 5.0f;
                float a1 = a0 + 2.0f * PI / 10.0f, a2 = a0 - 2.0f * PI / 10.0f;
                Vector3 c0 = { cx, cy, z + 0.01f };
                VBT(c0, DORADA_BT);
                VBT({ cx + std::cos(a2) * 0.16f, cy + std::sin(a2) * 0.16f, z + 0.01f }, DORADA_BT);
                VBT({ cx + std::cos(a0) * 0.4f, cy + std::sin(a0) * 0.4f, z + 0.01f }, DORADA_BT);
                VBT(c0, DORADA_BT);
                VBT({ cx + std::cos(a0) * 0.4f, cy + std::sin(a0) * 0.4f, z + 0.01f }, DORADA_BT);
                VBT({ cx + std::cos(a1) * 0.16f, cy + std::sin(a1) * 0.16f, z + 0.01f }, DORADA_BT);
            }

            DiscoZBT(cx - 0.16f, cy - 0.75f, 0.11f, z + 0.01f, LUZ_SERVICIO_BT);
            DiscoZBT(cx + 0.16f, cy - 0.75f, 0.11f, z + 0.01f, LUZ_SERVICIO_BT);
        }
    }

    // Barrido de brillo.
    float bx = x0 + std::fmod(t * 1.3f, 1.0f) * (x1 - x0 + 1.0f) - 0.5f;
    if (bx > x0 && bx < x1 - 0.2f) RectZBT(bx, y0 + 0.05f, bx + 0.2f, y1 - 0.05f, z + 0.02f, Color{ 120, 160, 210, 60 });
}


static void DibujarVidaBT(const MinijuegoBanqueteTurbo& m, bool decoracion, int paso)
{
    float t = m.tiempoAnimacion;

    LoteBT();

    // Luces de servicio en el dintel: se encienden en cadena.
    for (int k = 0; k < 16; k++)
    {
        float x = -15.0f + (float)k * 2.0f;
        float fase = std::fmod(t * 1.8f - (float)k * 0.18f, 1.0f);
        Color c = fase < 0.25f ? LUZ_SERVICIO_BT : TonoBT(LUZ_SERVICIO_BT, 0.42f);
        g_emisBT = 1.0f;
        BolaBT({ x, 7.22f, PARED_Z_BT + 0.72f }, { 0.13f, 0.09f, 0.08f }, c, 2, 8, 0.0f);
        g_emisBT = 0.0f;
    }

    // Lamparas colgantes sobre cada mesa.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        float x = m.posicionX[i], z = ZONA_MESA_BANQUETE;
        CilBT({ x, TECHO_Y_BT + 1.0f, z }, { x, 5.55f, z }, 0.02f, 0.02f, 4, GRAFITO_BT, false);
        BolaBT({ x, 5.25f, z }, { 0.38f, 0.27f, 0.38f }, CREMA_BT, 3, 14, 0.0f);
        CilBT({ x, 5.22f, z }, { x, 5.28f, z }, 0.39f, 0.39f, 14, TURQUESA_BT, false);
        g_emisBT = 1.0f;
        BolaBT({ x, 5.22f, z }, { 0.22f, 0.08f, 0.22f }, Color{ 255, 236, 190, 255 }, 2, 12);
        g_emisBT = 0.0f;
    }

    if (decoracion)
    {
        // Pantalla del menu.
        g_emisBT = 1.0f;
        DibujarMenuBT(t);

        // Jugo en los tanques del dispensador (el nivel ondula).
        const Color jugos[3] = { Color{ 255, 140, 60, 255 }, Color{ 120, 210, 90, 255 }, Color{ 220, 90, 160, 255 } };

        for (int k = 0; k < 3; k++)
        {
            float tx = 11.6f + 0.42f + (float)k * 0.72f;
            float nivel = 2.05f + 0.06f * std::sin(t * 1.4f + (float)k * 2.0f);
            CilBT({ tx, 1.65f, PARED_Z_BT + 0.55f }, { tx, nivel, PARED_Z_BT + 0.55f }, 0.24f, 0.24f, 12, jugos[k]);
        }

        // Pilotos de la cocina y del dispensador neumatico.
        Color piloto = std::fmod(t * 2.0f, 1.0f) < 0.5f ? Color{ 110, 255, 170, 255 } : Color{ 40, 120, 80, 255 };
        BolaBT({ -13.65f + 0.62f, 5.85f, PARED_Z_BT + 0.72f }, { 0.06f, 0.06f, 0.03f }, piloto, 2, 6);
        BolaBT({ -13.65f - 0.62f, 5.85f, PARED_Z_BT + 0.72f }, { 0.06f, 0.06f, 0.03f }, LUZ_SERVICIO_BT, 2, 6);
        g_emisBT = 0.0f;

        // Tazas magneticas que flotan sobre la mesita del salon.
        for (int k = 0; k < 3; k++)
        {
            float a = t * 0.45f + (float)k * 2.094f;
            Vector3 c = { MESITA_BT.x + std::cos(a) * 0.48f, 1.3f + 0.1f * std::sin(t * 1.7f + (float)k), MESITA_BT.z + std::sin(a) * 0.36f };
            CilBT(c, { c.x, c.y + 0.2f, c.z }, 0.1f, 0.12f, 10, k == 1 ? TURQUESA_BT : CREMA_BT);
            CilBT({ c.x, c.y + 0.17f, c.z }, { c.x, c.y + 0.19f, c.z }, 0.11f, 0.11f, 10, Color{ 120, 70, 50, 255 });
            CilBT({ c.x + 0.12f, c.y + 0.06f, c.z }, { c.x + 0.12f, c.y + 0.15f, c.z }, 0.035f, 0.035f, 6, k == 1 ? TURQUESA_BT : CREMA_BT);
            // Gotita que flota encima.
            BolaBT({ c.x, c.y + 0.36f + 0.05f * std::sin(t * 3.0f + (float)k), c.z }, { 0.04f, 0.04f, 0.04f }, Color{ 150, 92, 70, 255 }, 2, 6);
        }

        // Satelite que cruza muy lejos, del otro lado del vidrio.
        float recorrido = std::fmod(t * 1.1f, 110.0f);
        Vector3 s = { -55.0f + recorrido, 7.5f, -48.0f };
        CajaBT(s, { 0.9f, 0.5f, 0.5f }, Color{ 200, 200, 210, 255 });
        CajaBT({ s.x - 1.3f, s.y, s.z }, { 1.5f, 0.05f, 0.7f }, Color{ 50, 80, 170, 255 });
        CajaBT({ s.x + 1.3f, s.y, s.z }, { 1.5f, 0.05f, 0.7f }, Color{ 50, 80, 170, 255 });
        CilBT({ s.x, s.y + 0.25f, s.z }, { s.x, s.y + 0.55f, s.z }, 0.03f, 0.18f, 6, Color{ 220, 220, 228, 255 });
    }

    // Capsulas de comida viajando por los tubos neumaticos.
    int capsulas = paso == 1 ? 7 : 4;
    const Color tapas[4] = { Color{ 90, 220, 140, 255 }, Color{ 120, 170, 255, 255 }, Color{ 255, 150, 200, 255 }, LUZ_SERVICIO_BT };

    for (int tubo = 0; tubo < 2; tubo++)
    {
        for (int k = 0; k < capsulas; k++)
        {
            float s = std::fmod((float)k / (float)capsulas + t * (tubo == 0 ? 0.075f : 0.06f) + (float)tubo * 0.37f, 1.0f);
            Vector3 p = PuntoTuboBT(tubo, s);
            Vector3 q = PuntoTuboBT(tubo, s + 0.012f);
            Vector3 eje = Vector3Subtract(q, p);
            float l = Vector3Length(eje);

            if (l < 0.0001f) continue;

            eje = Vector3Scale(eje, 0.2f / l);
            Color c = tapas[(k + tubo) % 4];
            CilBT(Vector3Subtract(p, eje), Vector3Add(p, eje), 0.12f, 0.12f, 8, Color{ 226, 230, 238, 255 });
            CilBT(Vector3Add(p, eje), Vector3Add(p, Vector3Scale(eje, 1.4f)), 0.12f, 0.09f, 8, c);
            CilBT(Vector3Subtract(p, Vector3Scale(eje, 1.4f)), Vector3Subtract(p, eje), 0.09f, 0.12f, 8, c);
        }
    }

    FinLoteBT();
}


//--------------------------------------------------
// Translucido: vidrio del ventanal y tubos
//--------------------------------------------------

static void DibujarTraslucidoBT(float t, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    LoteBT();

    // Vidrio con reflejos diagonales que se desplazan muy despacio.
    float z = VIDRIO_Z_BT;
    Color tinte = Color{ 140, 190, 255, 14 };
    QuadColBT({ -VENTANA_X_BT, VENTANA_Y0_BT, z }, { VENTANA_X_BT, VENTANA_Y0_BT, z }, { VENTANA_X_BT, VENTANA_Y1_BT, z }, { -VENTANA_X_BT, VENTANA_Y1_BT, z },
        AlfaBT(tinte, 1.6f), AlfaBT(tinte, 1.6f), tinte, tinte);

    for (int k = 0; k < 4; k++)
    {
        float x = -9.0f + (float)k * 5.3f + std::fmod(t * 0.15f, 1.0f) * 0.6f;
        float ancho = k % 2 == 0 ? 0.7f : 0.3f;
        Color r = Color{ 220, 236, 255, (unsigned char)(k % 2 == 0 ? 16 : 24) };
        QuadColBT({ x, VENTANA_Y0_BT + 0.3f, z + 0.01f }, { x + ancho, VENTANA_Y0_BT + 0.3f, z + 0.01f },
            { x + ancho + 2.2f, VENTANA_Y1_BT - 0.3f, z + 0.01f }, { x + 2.2f, VENTANA_Y1_BT - 0.3f, z + 0.01f }, r, r, AlfaBT(r, 0.2f), AlfaBT(r, 0.2f));
    }

    // Tubos neumaticos de vidrio (por tramos del recorrido).
    if (decoracion)
    {
        for (int tubo = 0; tubo < 2; tubo++)
        {
            float y = TUBO_Y_BT[tubo];
            float dx = -13.65f + (tubo == 0 ? -0.32f : 0.32f);
            Color vidrio = Color{ 190, 230, 255, 46 };
            CilBT({ SALA_X_BT, y, TUBO_Z_BT }, { dx, y, TUBO_Z_BT }, 0.17f, 0.17f, 10, vidrio, false);
            CilBT({ dx, y, TUBO_Z_BT }, { dx, y, PARED_Z_BT + 0.35f }, 0.17f, 0.17f, 10, vidrio, false);
            CilBT({ dx, y, PARED_Z_BT + 0.35f }, { dx, 6.08f, PARED_Z_BT + 0.35f }, 0.17f, 0.17f, 10, vidrio, false);
            // Brillo a lo largo del tubo.
            TiraBT({ SALA_X_BT, y + 0.1f, TUBO_Z_BT + 0.08f }, { dx, y + 0.1f, TUBO_Z_BT + 0.08f }, 0.04f, 0.04f, Color{ 255, 255, 255, 50 }, Color{ 255, 255, 255, 50 });
        }
    }

    FinLoteBT();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Luces falsas aditivas
//--------------------------------------------------

static void ConoLuzBT(Vector3 arriba, Vector3 abajo, float r0, float r1, Color c)
{
    // Abanico de tiras que mira a la camara: se lee como un cono.
    TiraBT(arriba, abajo, r0 * 2.0f, r1 * 2.0f, c, AlfaBT(c, 0.0f));
    TiraBT(arriba, abajo, r0 * 1.0f, r1 * 1.1f, c, AlfaBT(c, 0.15f));
}


static void DibujarLucesBT(const MinijuegoBanqueteTurbo& m, bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteBT();

    // Luz de la Tierra que entra por el ventanal: charco azulado en el piso.
    for (int r = 0; r < 3; r++)
    {
        float e = 1.0f - (float)r * 0.28f;
        DiscoBT({ 0.0f, 0.012f + 0.002f * (float)r, PARED_Z_BT + 1.4f }, 11.5f * e, 4.2f * e, 28, Color{ 70, 120, 200, 16 }, Color{ 70, 120, 200, 0 }, false);
    }

    // Banda de luz sobre el banco del ventanal.
    QuadColBT({ -9.6f, 1.2f, PARED_Z_BT + 0.2f }, { 9.6f, 1.2f, PARED_Z_BT + 0.2f }, { 9.6f, 3.2f, PARED_Z_BT + 0.6f }, { -9.6f, 3.2f, PARED_Z_BT + 0.6f },
        Color{ 80, 130, 220, 18 }, Color{ 80, 130, 220, 18 }, Color{ 80, 130, 220, 0 }, Color{ 80, 130, 220, 0 });

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const PuestoBanquete& p = m.puestos[i];
        float x = m.posicionX[i], z = ZONA_MESA_BANQUETE;
        float yTapa = MESA_Y_BT + FlotacionMesaBT(i, t);

        // Lampara colgante: halo, cono y charco calido sobre la mesa.
        Color calida = Color{ 255, 214, 150, 255 };
        HaloBT({ x, 5.1f, z }, 0.55f, AlfaBT(calida, 0.4f), AlfaBT(calida, 0.0f));
        ConoLuzBT({ x, 5.1f, z }, { x, yTapa + 0.1f, z }, 0.3f, 1.4f, AlfaBT(calida, 0.04f));
        DiscoBT({ x, yTapa + 0.075f, z }, 1.5f, 0.75f, 20, AlfaBT(calida, 0.12f), AlfaBT(calida, 0.0f), false);
        DiscoBT({ x, 0.015f, z + 0.1f }, 2.3f, 1.6f, 20, AlfaBT(calida, 0.06f), AlfaBT(calida, 0.0f), false);

        // Levitacion: columna cian entre la base y la losa, y aro en el piso.
        float pulso = 0.7f + 0.3f * std::sin(t * 4.0f + (float)i);
        Color fria = AlfaBT(LUZ_FRIA_BT, 0.22f * pulso);
        TiraBT({ x, 0.38f, z }, { x, yTapa - GROSOR_MESA_BT - 0.16f, z }, 0.85f, 0.65f, fria, AlfaBT(fria, 0.4f));
        AnilloBT({ x, 0.02f, z }, 0.82f, 1.2f, 24, AlfaBT(LUZ_FRIA_BT, 0.18f * pulso), AlfaBT(LUZ_FRIA_BT, 0.0f));

        // Racion peligrosa o valiosa: resplandor rojo que late o dorado.
        if (p.indiceRacion >= 0 && p.bocadosRestantes > 0 && p.llegada > 0.5f)
        {
            const RacionBanquete& r = m.secuencia[p.indiceRacion];
            Vector3 c = { x, ALTURA_BANDEJA_BANQUETE + 0.2f, z + 0.12f };

            if (r.tipo == RACION_BANQUETE_PICANTE)
            {
                float l = 0.55f + 0.45f * std::sin(t * 14.0f);
                HaloBT(c, 1.2f, AlfaBT(PICANTE_BT, 0.32f * l), AlfaBT(PICANTE_BT, 0.0f), 18);
                DiscoBT({ x, yTapa + 0.08f, z + 0.12f }, 1.2f, 0.6f, 18, AlfaBT(PICANTE_BT, 0.3f * l), AlfaBT(PICANTE_BT, 0.0f), false);

                // Calor que sube en ondas.
                for (int k = 0; k < 3; k++)
                {
                    float s = std::fmod(t * 1.4f + (float)k * 0.33f, 1.0f);
                    float ox = (float)(k - 1) * 0.45f + 0.08f * std::sin(t * 6.0f + (float)k);
                    TiraBT({ x + ox, c.y + 0.2f + s * 0.9f, c.z }, { x + ox + 0.1f * std::sin(t * 8.0f + (float)k), c.y + 0.45f + s * 0.9f, c.z }, 0.1f, 0.02f,
                        AlfaBT(Color{ 255, 120, 40, 255 }, 0.5f * (1.0f - s)), AlfaBT(Color{ 255, 200, 60, 255 }, 0.0f));
                }
            }
            else if (r.tipo == RACION_BANQUETE_DORADA)
            {
                float l = 0.7f + 0.3f * std::sin(t * 5.0f);
                HaloBT(c, 1.1f, AlfaBT(DORADA_BT, 0.3f * l), AlfaBT(DORADA_BT, 0.0f), 18);
                DiscoBT({ x, yTapa + 0.08f, z + 0.12f }, 1.2f, 0.6f, 18, AlfaBT(DORADA_BT, 0.22f * l), AlfaBT(DORADA_BT, 0.0f), false);

                for (int k = 0; k < 4; k++)
                {
                    float a = t * 2.5f + (float)k * PI * 0.5f;
                    float brillo = Lim01BT(std::sin(t * 7.0f + (float)k * 1.3f));
                    Vector3 q = { c.x + std::cos(a) * 0.95f, c.y + 0.25f + 0.2f * std::sin(a * 1.3f), c.z + std::sin(a) * 0.35f };
                    HaloBT(q, 0.12f * brillo + 0.02f, AlfaBT(Color{ 255, 250, 220, 255 }, 0.9f * brillo), AlfaBT(Color{ 255, 220, 120, 255 }, 0.0f), 8);
                }
            }
        }

        // Destello del color del jugador al terminar una racion.
        if (p.tiempoMensaje > 0.0f)
        {
            float f = Lim01BT(p.tiempoMensaje / 0.8f);
            float r = 0.6f + (1.0f - f) * 1.4f;
            AnilloBT({ x, yTapa + 0.09f, z + 0.1f }, r * 0.8f, r, 24, AlfaBT(Color{ 255, 240, 200, 255 }, 0.0f), AlfaBT(Color{ 255, 240, 200, 255 }, 0.5f * f));
            HaloBT({ x, yTapa + 0.4f, z + 0.1f }, 0.9f * f + 0.2f, AlfaBT(Color{ 255, 250, 230, 255 }, 0.4f * f), AlfaBT(Color{ 255, 250, 230, 255 }, 0.0f));
        }
    }

    // Robot: chorro del propulsor y charco cian en el carril.
    {
        float x = m.robotX, y = 2.45f + 0.1f * std::sin(t * 2.5f);
        float parpadeo = 0.8f + 0.2f * std::sin(t * 37.0f);
        ConoLuzBT({ x, y - 1.0f, CARRIL_Z_BT }, { x, y - 1.9f, CARRIL_Z_BT }, 0.22f, 0.05f, AlfaBT(LUZ_FRIA_BT, 0.5f * parpadeo));
        HaloBT({ x, y - 1.02f, CARRIL_Z_BT }, 0.4f, AlfaBT(LUZ_FRIA_BT, 0.5f), AlfaBT(LUZ_FRIA_BT, 0.0f));
        DiscoBT({ x, 0.02f, CARRIL_Z_BT }, 1.5f, 1.0f, 20, AlfaBT(LUZ_FRIA_BT, 0.14f * parpadeo), AlfaBT(LUZ_FRIA_BT, 0.0f), false);
        HaloBT({ x + 0.0f, y + 1.6f, CARRIL_Z_BT }, 0.18f, AlfaBT(LUZ_SERVICIO_BT, std::fmod(t * 1.6f, 1.0f) < 0.5f ? 0.6f : 0.15f), AlfaBT(LUZ_SERVICIO_BT, 0.0f), 8);
    }

    // Carril: flechas de luz que corren hacia donde va el robot.
    for (int k = 0; k < 18; k++)
    {
        float x = -15.3f + (float)k * 1.8f;
        float onda = 0.5f + 0.5f * std::sin(t * 4.0f - x * 0.6f * (m.robotObjetivoX >= m.robotX ? 1.0f : -1.0f));
        Color c = AlfaBT(LUZ_FRIA_BT, 0.05f + 0.12f * onda);
        QuadColBT({ x - 0.35f, 0.015f, CARRIL_Z_BT - 0.06f }, { x + 0.35f, 0.015f, CARRIL_Z_BT - 0.06f },
            { x + 0.35f, 0.015f, CARRIL_Z_BT + 0.06f }, { x - 0.35f, 0.015f, CARRIL_Z_BT + 0.06f }, c, c, c, c);
    }

    // Luces de servicio del dintel.
    for (int k = 0; k < 16; k++)
    {
        float fase = std::fmod(t * 1.8f - (float)k * 0.18f, 1.0f);
        if (fase >= 0.25f) continue;
        HaloBT({ -15.0f + (float)k * 2.0f, 7.2f, PARED_Z_BT + 0.82f }, 0.38f, AlfaBT(LUZ_SERVICIO_BT, 0.5f), AlfaBT(LUZ_SERVICIO_BT, 0.0f), 10);
    }

    if (decoracion)
    {
        // Lamparas termicas de la cocina.
        for (int k = 0; k < 2; k++)
        {
            float lx = COCINA_X0_BT + 1.4f + (float)k * 1.7f;
            HaloBT({ lx, 3.5f, PARED_Z_BT + 0.45f }, 0.4f, AlfaBT(LUZ_SERVICIO_BT, 0.5f), AlfaBT(LUZ_SERVICIO_BT, 0.0f));
            ConoLuzBT({ lx, 3.5f, PARED_Z_BT + 0.45f }, { lx, 1.2f, PARED_Z_BT + 0.6f }, 0.25f, 0.8f, AlfaBT(LUZ_SERVICIO_BT, 0.1f));
            DiscoBT({ lx, 1.2f, PARED_Z_BT + 0.62f }, 0.9f, 0.5f, 14, AlfaBT(LUZ_SERVICIO_BT, 0.2f), AlfaBT(LUZ_SERVICIO_BT, 0.0f), false);
        }

        // Luz de cultivo violeta de las torres.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = (float)lado * TORRE_X_BT;
            float l = 0.85f + 0.15f * std::sin(t * 1.3f + (float)lado);
            HaloBT({ x, 3.42f, TORRE_Z_BT }, 0.9f, AlfaBT(Color{ 200, 110, 255, 255 }, 0.3f * l), AlfaBT(Color{ 200, 110, 255, 255 }, 0.0f));
            ConoLuzBT({ x, 3.4f, TORRE_Z_BT }, { x, 0.7f, TORRE_Z_BT }, 0.55f, 0.75f, AlfaBT(Color{ 190, 110, 255, 255 }, 0.08f * l));
        }

        // Resplandor del cartel del menu.
        HaloBT({ (MENU_X0_BT + MENU_X1_BT) * 0.5f, (MENU_Y0_BT + MENU_Y1_BT) * 0.5f, PARED_Z_BT + 0.5f }, 3.0f,
            Color{ 70, 110, 200, 30 }, Color{ 70, 110, 200, 0 }, 18);
    }

    FinLoteBT();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Particulas: migajas de la logica, gotitas flotantes y confeti final
//--------------------------------------------------

static float g_finBT = -1.0f;    // tiempoAnimacion en que termino la partida


static void DibujarParticulasBT(const MinijuegoBanqueteTurbo& m, int limite, bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    LoteBT();

    for (int i = 0; i < MAX_PARTICULAS_BANQUETE; i++)
    {
        const ParticulaTierra& p = m.particulas[i];

        if (!p.activa || p.vidaMaxima <= 0.0f) continue;

        float f = Lim01BT(p.vida / p.vidaMaxima);
        PuntoBT(p.posicion, p.tamano * (0.6f + 0.6f * f), AlfaBT(MezclaBT(p.color, WHITE, 0.15f), 0.4f + 0.6f * f));
    }

    // Gotitas de jugo que flotan en microgravedad cerca del ventanal.
    if (decoracion)
    {
        int cantidad = (int)(26.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < cantidad; k++)
        {
            float s = std::fmod(Ruido01BT(k, 61) + t * (0.02f + 0.02f * Ruido01BT(k, 62)), 1.0f);
            Vector3 q =
            {
                -10.0f + 20.0f * Ruido01BT(k, 63) + 0.3f * std::sin(t * 0.7f + (float)k),
                1.2f + 5.0f * s,
                -6.6f + 3.0f * Ruido01BT(k, 64)
            };
            float alfa = std::sin(s * PI) * 0.7f;
            Color c = k % 3 == 0 ? Color{ 255, 150, 200, 255 } : (k % 3 == 1 ? Color{ 150, 230, 255, 255 } : Color{ 255, 210, 140, 255 });
            PuntoBT(q, 0.035f + 0.02f * Ruido01BT(k, 65), AlfaBT(c, alfa));
        }
    }

    // Confeti sobre el ganador.
    if (m.fase == FASE_BANQUETE_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (g_finBT < 0.0f || g_finBT > t) g_finBT = t;

        float te = t - g_finBT;
        const Color colores[5] = { Color{ 255, 90, 90, 255 }, Color{ 90, 170, 255, 255 }, Color{ 110, 230, 120, 255 }, DORADA_BT, Color{ 255, 140, 230, 255 } };
        int cantidad = (int)(70.0f * FactorCalidadGrafica(CalidadParticulas())) + 20;

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1) continue;

            for (int k = 0; k < cantidad; k++)
            {
                float caida = std::fmod(te * (0.9f + 0.5f * Ruido01BT(k, 71)) + Ruido01BT(k, 72) * 3.5f, 3.5f);
                Vector3 q =
                {
                    m.posicionX[i] + (Ruido01BT(k, 73) - 0.5f) * 3.2f + 0.25f * std::sin(te * 3.0f + (float)k),
                    5.2f - caida,
                    ZONA_JUGADOR_BANQUETE + 0.6f + (Ruido01BT(k, 74) - 0.5f) * 1.8f
                };
                float giro = te * 6.0f + (float)k;
                Vector3 d = Vector3Scale(g_derBT, 0.07f * std::cos(giro));
                Vector3 u = Vector3Scale(g_arribaBT, 0.045f);
                Color c = colores[k % 5];
                QuadColBT(Vector3Subtract(Vector3Subtract(q, d), u), Vector3Subtract(Vector3Add(q, d), u),
                    Vector3Add(Vector3Add(q, d), u), Vector3Add(Vector3Subtract(q, d), u), c, c, c, c);
            }
        }
    }
    else
    {
        g_finBT = -1.0f;
    }

    FinLoteBT();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Anuncios sobre el mundo y cartelitos J#
//--------------------------------------------------

static void DibujarAnunciosBT(
    const MinijuegoBanqueteTurbo& m,
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    float t = m.tiempoAnimacion;
    float escala = EscalaUI();

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const PuestoBanquete& p = m.puestos[i];
        const Participante& participante = participantes[i];
        int numero = participante.numeroJugador > 0 ? participante.numeroJugador : i + 1;
        float x = m.posicionX[i];

        // Cartelito J# sobre la cabeza.
        Vector2 cabeza = GetWorldToScreen({ x, 2.95f, ZONA_JUGADOR_BANQUETE }, vista);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participante.color;
        opciones.pico = true;
        Rectangle placa = DibujarEtiquetaUI(TextFormat("J%d", numero), cabeza, 0.0f, opciones);
        float arriba = placa.y - 16.0f * escala;

        if (m.fase != FASE_BANQUETE_JUGANDO) continue;

        const RacionBanquete& racion = m.secuencia[p.indiceRacion >= 0 ? p.indiceRacion : 0];

        if (p.tos > 0.0f)
        {
            float sacudida = std::sin(t * 40.0f) * 3.0f * escala;
            DibujarTextoMundoContornoUI("¡COF, COF!", cabeza.x + sacudida, arriba, UI_TEXTO, Color{ 255, 150, 120, 255 });
        }
        else if (p.rechazo > 0.0f)
        {
            float f = Lim01BT(p.rechazo / DURACION_RECHAZO_BANQUETE);
            bool zafo = p.racionSalida.tipo == RACION_BANQUETE_PICANTE;
            DibujarTextoMundoContornoUI(zafo ? "¡ZAFASTE!" : "RECHAZADA", cabeza.x, arriba - (1.0f - f) * 12.0f * escala, UI_TEXTO,
                zafo ? Color{ 150, 255, 170, 255 } : Color{ 210, 214, 224, 255 }, Lim01BT(f * 2.0f));
        }
        else if (racion.tipo == RACION_BANQUETE_PICANTE && p.bocadosRestantes > 0 && p.llegada >= 1.0f)
        {
            float salto = std::fabs(std::sin(t * 9.0f)) * 5.0f * escala;
            Color c = std::fmod(t * 6.0f, 1.0f) < 0.5f ? Color{ 255, 80, 60, 255 } : Color{ 255, 170, 70, 255 };
            DibujarTextoMundoContornoUI("¡PICANTE! ¡SALTÁ!", cabeza.x, arriba - salto, UI_TEXTO, c);
        }
        else if (racion.tipo == RACION_BANQUETE_DORADA && p.bocadosRestantes > 0 && p.llegada >= 1.0f)
        {
            DibujarTextoMundoContornoUI("DORADA x2", cabeza.x, arriba, UI_TEXTO, Color{ 255, 220, 90, 255 });
        }

        // +1 / +2 que sube desde la bandeja al terminar una racion.
        if (p.tiempoMensaje > 0.0f)
        {
            float f = Lim01BT(p.tiempoMensaje / 0.8f);
            int previa = (p.indiceRacion - 1 + LARGO_SECUENCIA_BANQUETE) % LARGO_SECUENCIA_BANQUETE;
            bool doble = m.secuencia[previa].tipo == RACION_BANQUETE_DORADA;
            Vector2 q = GetWorldToScreen({ x + 1.0f, MESA_Y_BT + 0.6f + (1.0f - f) * 0.9f, ZONA_MESA_BANQUETE + 0.3f }, vista);
            DibujarTextoMundoContornoUI(doble ? "+2" : "+1", q.x, q.y, UI_TEXTO_GRANDE,
                doble ? Color{ 255, 220, 90, 255 } : MezclaBT(participante.color, WHITE, 0.35f), Lim01BT(f * 2.5f));
        }
    }
}


//--------------------------------------------------
// HUD
//--------------------------------------------------

static void PrepararHudBT(
    const MinijuegoBanqueteTurbo& m,
    const Participante participantes[],
    int limite,
    DatosHudMinijuego& hud
)
{
    PrepararDatosHudMinijuego(
        hud,
        "BANQUETE TURBO",
        "¡MACHACÁ PARA COMER! RECHAZÁ LAS PICANTES CON SALTO. LAS DORADAS VALEN DOBLE.",
        m.resultado
    );

    hud.fase = m.fase == FASE_BANQUETE_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_BANQUETE_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_BANQUETE;
    hud.segundosAlerta = 6.0f;
    hud.finPorTiempo = true;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        JugadorHudMinijuego& jugador = hud.jugadores[i];
        const Participante& participante = participantes[i];
        const PuestoBanquete& p = m.puestos[i];
        const RacionBanquete& racion = m.secuencia[p.indiceRacion >= 0 ? p.indiceRacion : 0];

        jugador.visible = true;
        jugador.valorNumerico = p.puntos;
        std::snprintf(jugador.valor, sizeof(jugador.valor), p.puntos == 1 ? "1 PTO" : "%d PTS", p.puntos);
        jugador.progreso = p.bocadosRacion > 0 ? Lim01BT(1.0f - (float)p.bocadosRestantes / (float)p.bocadosRacion) : 0.0f;
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), p.puntos == 1 ? "1 PTO" : "%d PTS", p.puntos);
        std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "%d BOCADOS", p.bocadosTotales);

        if (m.fase == FASE_BANQUETE_JUGANDO)
        {
            if (p.tos > 0.0f)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡COF, COF!");
            else if (p.rechazo > 0.0f)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡RECHAZADA!");
            else if (racion.tipo == RACION_BANQUETE_PICANTE)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡PICANTE!");
            else if (racion.tipo == RACION_BANQUETE_DORADA)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "DORADA x2");
            else
                std::snprintf(jugador.estado, sizeof(jugador.estado), "%d BOCADOS", p.bocadosTotales);
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "COMÉ: %s\nRECHAZÁ: %s",
                TextoBotonGolpeHud(participante),
                ObtenerTextoBotonPrincipal(participante)
            );
        }
    }

    // Nota de desempate solo si los primeros empataron en puntos.
    if (m.fase == FASE_BANQUETE_TERMINADO)
    {
        int mejor = -1, empatados = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo) continue;

            if (m.puestos[i].puntos > mejor) { mejor = m.puestos[i].puntos; empatados = 1; }
            else if (m.puestos[i].puntos == mejor) empatados++;
        }

        if (empatados > 1) hud.notaFinal = "EMPATE EN PUNTOS: GANA QUIEN DIO MÁS BOCADOS";
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoBanqueteTurbo::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteBanquete(cantidadMaxima);
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(ESPACIO_BT);

    Camera3D vista = CamaraVistaBT(*this);
    g_ojoBT = OJO_BT;
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derBT = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaBT = Vector3CrossProduct(g_derBT, frente);

    DibujarEspacioBT(vista, tiempoAnimacion, decoracion);
    HornearBT();

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarMallaBT(g_hornoBT.tierra, g_hornoBT.tierraValida);
    DibujarMallaBT(g_hornoBT.nubes, g_hornoBT.nubesValidas);
    DibujarMallaBT(g_hornoBT.sala, g_hornoBT.salaValida);
    DibujarMallaBT(g_hornoBT.utileria, g_hornoBT.utileriaValida);
    DibujarVidaBT(*this, decoracion, paso);

    // Robot, mesas y sombras de los comensales (un solo lote).
    LoteBT();

    if (resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarRobotBT(*this, decoracion);
    }

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        DibujarMesaBT(*this, i, participantes[i].color, decoracion);

        if (sombras)
        {
            DiscoBT({ jugadores[i].posicion.x, 0.012f, ZONA_JUGADOR_BANQUETE }, 0.62f, 0.42f, 16, Color{ 10, 12, 24, 120 }, Color{ 10, 12, 24, 0 }, false);
        }
    }

    FinLoteBT();

    // Comensales con el modelo compartido: de pie tras su mesa; al toser se
    // ponen colorados.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        const JugadorPrueba& j = jugadores[i];
        const PuestoBanquete& p = puestos[i];
        Color color = participantes[i].color;

        if (p.tos > 0.0f)
        {
            color = MezclaBT(color, Color{ 255, 60, 40, 255 }, 0.45f + 0.25f * std::sin(tiempoAnimacion * 18.0f));
        }

        DibujarModeloJugadorEnPosicion({ j.posicion.x, j.posicion.y - 1.0f, j.posicion.z }, ObtenerAnguloModeloJugadorCompartido(j), color);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(j), LIME);
        }
    }

    // Translucido, particulas y luces al final.
    DibujarTraslucidoBT(tiempoAnimacion, decoracion);
    DibujarParticulasBT(*this, limite, decoracion);

    if (efectos)
    {
        DibujarLucesBT(*this, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    if (resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarAnunciosBT(*this, participantes, limite, vista);
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    DatosHudMinijuego hud;
    PrepararHudBT(*this, participantes, limite, hud);
    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoBanqueteTurbo::ObtenerResultado() const
{
    return resultado;
}

