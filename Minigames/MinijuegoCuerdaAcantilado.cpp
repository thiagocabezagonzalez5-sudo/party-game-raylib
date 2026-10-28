#include "Minigames/MinijuegoCuerdaAcantilado.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// GAMEPLAY: constantes
//==================================================
//
// Tema elegido: ACANTILADOS (banco del equipo: canon, pantano, barco,
// acantilados, castillo; elegido con segundos del reloj % 5). El hueco
// central es el mar, muy abajo. El bando arrastrado cae al agua.
//
// Mecanica: tiron por RITMO. Un pulso recorre una barra de ida y vuelta;
// pulsar el boton principal con el pulso dentro de la ventana central da
// un tiron fuerte. Fuera de ella: tiron debil y tropiezo breve. Tres
// aciertos seguidos (racha) dan un tiron extra.
//==================================================

static const float DURACION_PREPARACION_CUERDA = 2.8f;
static const float DURACION_PARTIDA_CUERDA = 40.0f;
static const float UMBRAL_EMPATE_CUERDA = 0.06f;

// Ritmo: periodo del pulso y semiancho de la ventana (en unidades de barra
// 0..1, centro 0.5). Ambos se endurecen con el progreso de la partida.
static const float PERIODO_INICIAL_CUERDA = 1.55f;
static const float PERIODO_FINAL_CUERDA = 0.90f;
static const float VENTANA_INICIAL_CUERDA = 0.15f;
static const float VENTANA_FINAL_CUERDA = 0.085f;

static const float DURACION_TROPIEZO_CUERDA = 0.55f;
static const float RECARGA_TIRON_CUERDA = 0.28f;
static const float DURACION_FEEDBACK_CUERDA = 0.7f;

// Desplazamiento del marcador (rango -1..1) por tiron.
static const float TIRON_BASE_CUERDA = 0.045f;
static const float FACTOR_TIRON_DEBIL_CUERDA = 0.25f;
static const float BONUS_RACHA_CUERDA = 0.6f;
static const int ACIERTOS_PARA_RACHA_CUERDA = 3;

// Bots.
static const float PROB_FALLO_BOT_CUERDA = 0.20f;
static const float PROB_FALLO_BOT_SOLO_CUERDA = 0.10f;
static const float PROB_OMITIR_BOT_CUERDA = 0.08f;

// Escenario: X de las plataformas y lineas de victoria.
static const float BORDE_ACANTILADO_CUERDA = 3.6f;
static const float LINEA_VICTORIA_X_CUERDA = 3.2f;
static const float DESPLAZAMIENTO_CUERPOS_CUERDA = 1.3f;
static const float X_BASE_SOLO_CUERDA = -5.0f;
static const float X_BASE_TRIO_CUERDA = 4.9f;
static const float SEPARACION_TRIO_CUERDA = 1.45f;
static const float ALTURA_MAR_CUERDA = -7.0f;
static const float ALTURA_MANOS_CUERDA = 0.95f;
static const float OFFSET_CENTRO_JUGADOR_CUERDA = 0.70f;


static float LimitarCuerda(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float MezclarCuerda(float a, float b, float t)
{
    return a + (b - a) * t;
}


static float ProgresoPartidaCuerda(const MinijuegoCuerdaAcantilado& minijuego)
{
    return LimitarCuerda(
        minijuego.tiempoJuego / DURACION_PARTIDA_CUERDA,
        0.0f,
        1.0f
    );
}


static float PeriodoPulsoCuerda(const MinijuegoCuerdaAcantilado& minijuego)
{
    return MezclarCuerda(
        PERIODO_INICIAL_CUERDA,
        PERIODO_FINAL_CUERDA,
        ProgresoPartidaCuerda(minijuego)
    );
}


static float VentanaJugadorCuerda(
    const MinijuegoCuerdaAcantilado& minijuego,
    int indice
)
{
    float ventana = MezclarCuerda(
        VENTANA_INICIAL_CUERDA,
        VENTANA_FINAL_CUERDA,
        ProgresoPartidaCuerda(minijuego)
    );

    if (indice == minijuego.indiceSolo)
    {
        ventana *= minijuego.factorVentanaSolo;
    }

    return ventana;
}


// Onda triangular 0..1..0 a partir de una fase creciente.
static float PosicionPulsoCuerda(float fase)
{
    float f = fase - std::floor(fase);
    return f < 0.5f ? f * 2.0f : 2.0f - f * 2.0f;
}


static bool EsBotCuerda(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


//==================================================
// GAMEPLAY: posiciones y resultado
//==================================================

// Posicion de los pies sin animacion de caida.
static Vector3 PosicionBasePiesCuerda(
    const MinijuegoCuerdaAcantilado& minijuego,
    int indice
)
{
    const EstadoJugadorCuerdaAcantilado& estado =
        minijuego.estadosJugadores[indice];

    float baseX = estado.equipo == 0
        ? X_BASE_SOLO_CUERDA
        : X_BASE_TRIO_CUERDA + SEPARACION_TRIO_CUERDA * estado.ordenEnEquipo;

    // El marcador arrastra los cuerpos hacia el lado perdedor.
    float x = baseX + minijuego.marcador * DESPLAZAMIENTO_CUERPOS_CUERDA;

    return { x, 0.0f, 0.0f };
}


// Incluye la animacion de caida del bando perdedor tras el final.
static Vector3 PosicionPiesCuerda(
    const MinijuegoCuerdaAcantilado& minijuego,
    int indice
)
{
    Vector3 pies = PosicionBasePiesCuerda(minijuego, indice);

    if (
        minijuego.fase != FASE_CUERDA_TERMINADO ||
        minijuego.empate
    )
    {
        return pies;
    }

    const EstadoJugadorCuerdaAcantilado& estado =
        minijuego.estadosJugadores[indice];

    bool perdedor = (estado.equipo == 0) == !minijuego.ganaSolo;

    if (!perdedor)
    {
        return pies;
    }

    float t = minijuego.tiempoFin - 0.25f - 0.12f * estado.ordenEnEquipo;

    if (t <= 0.0f)
    {
        return pies;
    }

    // Se desliza hacia el hueco y luego cae con gravedad.
    float haciaCentro = estado.equipo == 0 ? 1.0f : -1.0f;
    pies.x += haciaCentro * 2.4f * t;
    pies.y = -9.0f * t * t;

    if (pies.y < ALTURA_MAR_CUERDA - 1.2f)
    {
        pies.y = ALTURA_MAR_CUERDA - 1.2f;
    }

    return pies;
}


static void FinalizarCuerda(
    MinijuegoCuerdaAcantilado& minijuego,
    bool empate,
    bool ganaSolo
)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.desenlace = empate
        ? DESENLACE_EMPATE
        : DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;
    minijuego.empate = empate;
    minijuego.ganaSolo = ganaSolo;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = empate || esSolo == ganaSolo;

        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego =
            minijuego.estadosJugadores[i].aciertos * 100;
    }

    minijuego.fase = FASE_CUERDA_TERMINADO;
    minijuego.tiempoFin = 0.0f;
}


//==================================================
// GAMEPLAY: ritmo, tirones y bots
//==================================================

static void AplicarTironCuerda(
    MinijuegoCuerdaAcantilado& minijuego,
    int indice,
    bool acierto
)
{
    EstadoJugadorCuerdaAcantilado& estado = minijuego.estadosJugadores[indice];
    float fuerza = 0.0f;

    if (acierto)
    {
        estado.aciertos++;
        estado.racha++;
        estado.tiempoRecarga = RECARGA_TIRON_CUERDA;
        estado.feedback = FEEDBACK_CUERDA_ACIERTO;
        fuerza = 1.0f;

        if (estado.racha >= ACIERTOS_PARA_RACHA_CUERDA)
        {
            // Racha: tiron extra y se reinicia el contador.
            fuerza += BONUS_RACHA_CUERDA;
            estado.racha = 0;
            estado.feedback = FEEDBACK_CUERDA_RACHA;
        }
    }
    else
    {
        estado.fallos++;
        estado.racha = 0;
        estado.tiempoTropiezo = DURACION_TROPIEZO_CUERDA;
        estado.feedback = FEEDBACK_CUERDA_TROPIEZO;
        fuerza = FACTOR_TIRON_DEBIL_CUERDA;
    }

    estado.tiempoFeedback = DURACION_FEEDBACK_CUERDA;
    estado.retroceso = 1.0f;

    if (indice == minijuego.indiceSolo)
    {
        fuerza *= minijuego.fuerzaSolo;
    }

    // El solitario tira hacia -1 y el trio hacia +1.
    float direccion = estado.equipo == 0 ? -1.0f : 1.0f;
    minijuego.marcadorObjetivo = LimitarCuerda(
        minijuego.marcadorObjetivo + direccion * TIRON_BASE_CUERDA * fuerza,
        -1.0f,
        1.0f
    );
}


static float ElegirObjetivoBotCuerda(
    const MinijuegoCuerdaAcantilado& minijuego,
    int indice
)
{
    bool esSolo = indice == minijuego.indiceSolo;
    float ventana = VentanaJugadorCuerda(minijuego, indice);
    bool soloContraVarios = esSolo && minijuego.cantidadRivales > 1;
    float probFallo = soloContraVarios
        ? PROB_FALLO_BOT_SOLO_CUERDA
        : PROB_FALLO_BOT_CUERDA;
    float signo = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;

    if (GetRandomValue(0, 99) < (int)(probFallo * 100.0f))
    {
        // Error de timing: apunta fuera de la ventana.
        return 0.5f + signo * ventana * (GetRandomValue(130, 230) / 100.0f);
    }

    float precision = soloContraVarios ? 0.55f : 0.80f;
    return 0.5f + signo * ventana * precision *
        (GetRandomValue(0, 100) / 100.0f);
}


// El bot pulsa cuando el pulso cruza su posicion objetivo.
static bool DecidirPulsoBotCuerda(
    MinijuegoCuerdaAcantilado& minijuego,
    int indice,
    float pulsoActual
)
{
    EstadoJugadorCuerdaAcantilado& estado = minijuego.estadosJugadores[indice];

    float a = estado.pulsoAnterior - estado.objetivoBot;
    float b = pulsoActual - estado.objetivoBot;
    bool cruza = a * b <= 0.0f && estado.pulsoAnterior != pulsoActual;

    estado.pulsoAnterior = pulsoActual;

    if (!cruza)
    {
        return false;
    }

    estado.objetivoBot = ElegirObjetivoBotCuerda(minijuego, indice);

    if (GetRandomValue(0, 99) < (int)(PROB_OMITIR_BOT_CUERDA * 100.0f))
    {
        return false;
    }

    return true;
}


static void ActualizarJugadorCuerda(
    MinijuegoCuerdaAcantilado& minijuego,
    int indice,
    float deltaTime,
    const Participante& participante
)
{
    EstadoJugadorCuerdaAcantilado& estado = minijuego.estadosJugadores[indice];

    estado.fasePulso += deltaTime / PeriodoPulsoCuerda(minijuego);
    float pulso = PosicionPulsoCuerda(estado.fasePulso);

    estado.tiempoTropiezo = fmaxf(0.0f, estado.tiempoTropiezo - deltaTime);
    estado.tiempoRecarga = fmaxf(0.0f, estado.tiempoRecarga - deltaTime);
    estado.tiempoFeedback = fmaxf(0.0f, estado.tiempoFeedback - deltaTime);
    estado.retroceso = fmaxf(0.0f, estado.retroceso - deltaTime * 3.2f);

    bool pulsa = false;

    if (EsBotCuerda(participante))
    {
        pulsa = DecidirPulsoBotCuerda(minijuego, indice, pulso);
    }
    else
    {
        pulsa = LeerInputMinijuegoParticipante(participante).saltar;
    }

    // Durante el tropiezo el jugador no puede tirar.
    if (!pulsa || estado.tiempoTropiezo > 0.0f)
    {
        return;
    }

    bool acierto =
        estado.tiempoRecarga <= 0.0f &&
        std::fabs(pulso - 0.5f) <= VentanaJugadorCuerda(minijuego, indice);

    AplicarTironCuerda(minijuego, indice, acierto);
}


//==================================================
// Interfaz publica
//==================================================

static void ReiniciarVisualCuerda(MinijuegoCuerdaAcantilado& m);
static void ActualizarVisualCuerda(
    MinijuegoCuerdaAcantilado& m,
    float deltaTime,
    int limite,
    const Participante participantes[]
);


void MinijuegoCuerdaAcantilado::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    // Camara de frente y algo alta: los dos acantilados, el canal con el
    // mar y el sol poniendose al fondo. Solo visual.
    camara.position = { 1.4f, 2.6f, 21.0f };
    camara.target = { 1.4f, -0.7f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 36.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CUERDA_PREPARACION;
    indiceSolo = -1;
    cantidadRivales = 0;
    marcador = 0.0f;
    marcadorObjetivo = 0.0f;
    fuerzaSolo = 1.0f;
    factorVentanaSolo = 1.0f;
    empate = false;
    ganaSolo = false;
    tiempoPreparacion = DURACION_PREPARACION_CUERDA;
    tiempoJuego = 0.0f;
    tiempoFin = 0.0f;
    tiempoAnimacion = 0.0f;

    ReiniciarVisualCuerda(*this);
}


void MinijuegoCuerdaAcantilado::Reiniciar(
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

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_CUERDA_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;
    cantidadRivales = cantidad - 1;

    // Handicap: con mas rivales, el solitario tira mas fuerte y tiene una
    // ventana mas ancha (1 vs 1 queda sin ventaja).
    const float fuerzaPorRivales[4] = { 1.0f, 1.03f, 1.7f, 2.55f };
    const float ventanaPorRivales[4] = { 1.0f, 1.0f, 1.12f, 1.22f };
    int nivel = cantidadRivales < 3 ? cantidadRivales : 3;
    fuerzaSolo = fuerzaPorRivales[nivel];
    factorVentanaSolo = ventanaPorRivales[nivel];

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
    int ordenTrio = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool esSolo = i == indiceSolo;
        EstadoJugadorCuerdaAcantilado& estado = estadosJugadores[i];

        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;
        estado.equipo = esSolo ? 0 : 1;
        estado.ordenEnEquipo = esSolo ? 0 : ordenTrio++;
        estado.fasePulso = 0.19f * (float)i;
        estado.pulsoAnterior = PosicionPulsoCuerda(estado.fasePulso);
        estado.objetivoBot = 0.5f;

        Vector3 pies = PosicionBasePiesCuerda(*this, i);
        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            { pies.x, OFFSET_CENTRO_JUGADOR_CUERDA, pies.z }
        );
        jugadores[i].direccionMirada = esSolo
            ? Vector3{ 1.0f, 0.0f, 0.0f }
            : Vector3{ -1.0f, 0.0f, 0.0f };
        jugadores[i].enSuelo = true;
        jugadores[i].cayendo = false;
    }
}


void MinijuegoCuerdaAcantilado::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        return;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    if (fase == FASE_CUERDA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CUERDA_JUGANDO;
        }
    }
    else if (fase == FASE_CUERDA_JUGANDO)
    {
        float restanteAntes = DURACION_PARTIDA_CUERDA - tiempoJuego;
        tiempoJuego += deltaTime;
        ActualizarAudioAlertaTiempo(audio, restanteAntes, DURACION_PARTIDA_CUERDA - tiempoJuego);

        for (int i = 0; i < limite; i++)
        {
            if (resultado.participantes[i].participo)
            {
                ActualizarJugadorCuerda(*this, i, deltaTime, participantes[i]);
            }
        }

        // El marcador sigue suavemente al objetivo (sin saltos bruscos).
        marcador += (marcadorObjetivo - marcador) *
            fminf(1.0f, 10.0f * deltaTime);

        if (marcadorObjetivo <= -1.0f && marcador <= -0.985f)
        {
            marcador = -1.0f;
            FinalizarCuerda(*this, false, true);
        }
        else if (marcadorObjetivo >= 1.0f && marcador >= 0.985f)
        {
            marcador = 1.0f;
            FinalizarCuerda(*this, false, false);
        }
        else if (tiempoJuego >= DURACION_PARTIDA_CUERDA)
        {
            if (std::fabs(marcador) < UMBRAL_EMPATE_CUERDA)
                FinalizarCuerda(*this, true, false);
            else
                FinalizarCuerda(*this, false, marcador < 0.0f);
        }
    }
    else
    {
        tiempoFin += deltaTime;

        for (int i = 0; i < limite; i++)
        {
            EstadoJugadorCuerdaAcantilado& estado = estadosJugadores[i];
            estado.tiempoFeedback = fmaxf(0.0f, estado.tiempoFeedback - deltaTime);
            estado.retroceso = fmaxf(0.0f, estado.retroceso - deltaTime * 3.2f);
        }
    }

    // La posicion de los jugadores deriva siempre del estado de gameplay.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 pies = PosicionPiesCuerda(*this, i);
        jugadores[i].posicion =
        {
            pies.x,
            pies.y + OFFSET_CENTRO_JUGADOR_CUERDA,
            pies.z
        };
        jugadores[i].enSuelo = pies.y >= 0.0f;
        jugadores[i].cayendo = pies.y < 0.0f;
    }

    ActualizarVisualCuerda(*this, deltaTime, limite, participantes);
}


//==================================================
// VISUAL: ACANTILADOS CON FARO AL ATARDECER
//==================================================
//
// Dos promontorios de roca estratificada separados por un canal de mar.
// El solitario tira desde el de la izquierda y el trio desde el de la
// derecha (con el faro y la casa del farero). Al fondo, por el canal, el
// sol se pone sobre el horizonte. Nada de esto toca la logica: las
// posiciones salen de PosicionBasePiesCuerda y el borde dibujado de cada
// acantilado coincide con BORDE_ACANTILADO_CUERDA donde estan los
// jugadores. El azar visual usa semillaVisual.
//
// MODELO FUTURO: faro, casa del farero, banco, catalejo de monedas,
// farolas, salvavidas, estaca con rollo de soga, mojones con banderin,
// pinos inclinados, torre en ruinas, farallones y gaviotas podrian ser
// .glb de Assets/Modelos/Escenarios sin tocar el gameplay.
//==================================================

static const Vector3 OJO_CUE = { 1.4f, 2.6f, 21.0f };
static const float SOL_X_CUE = -1.2f;
static const float Z_HORIZONTE_CUE = -420.0f;
static const float Z_MOJON_CUE = -0.95f;

static const Vector3 FARO_CUE = { 16.6f, 0.0f, -11.5f };
static const float ALTO_TORRE_FARO_CUE = 5.6f;
static const Vector3 CASA_CUE = { 11.2f, 0.0f, -14.8f };

static const Color CIELO_ALTO_CUE = { 52, 56, 118, 255 };
static const Color CIELO_MEDIO_CUE = { 196, 104, 122, 255 };
static const Color CIELO_HORIZONTE_CUE = { 255, 178, 112, 255 };
static const Color NIEBLA_CUE = { 226, 150, 136, 255 };
static const Color SOL_CUE = { 255, 228, 156, 255 };

static const Color MAR_CERCA_CUE = { 26, 70, 98, 255 };
static const Color MAR_MEDIO_CUE = { 60, 92, 132, 255 };
static const Color MAR_LEJOS_CUE = { 214, 136, 128, 255 };
static const Color ESPUMA_CUE = { 248, 242, 232, 255 };

static const Color PASTO_CUE = { 104, 146, 70, 255 };
static const Color PASTO_OSCURO_CUE = { 70, 108, 56, 255 };
static const Color PASTO_CLARO_CUE = { 150, 172, 84, 255 };
static const Color TIERRA_CUE = { 148, 110, 76, 255 };

static const Color ROCA_PALIDA_CUE = { 232, 202, 158, 255 };
static const Color ROCA_CREMA_CUE = { 214, 176, 128, 255 };
static const Color ROCA_OCRE_CUE = { 194, 134, 88, 255 };
static const Color ROCA_OXIDO_CUE = { 164, 98, 72, 255 };
static const Color ROCA_GRIS_CUE = { 142, 120, 122, 255 };
static const Color ROCA_HUMEDA_CUE = { 72, 70, 70, 255 };
static const Color PIEDRA_CUE = { 150, 140, 132, 255 };

static const Color MADERA_CUE = { 136, 96, 64, 255 };
static const Color MADERA_OSCURA_CUE = { 90, 62, 44, 255 };
static const Color CAL_CUE = { 238, 230, 216, 255 };
static const Color FRANJA_FARO_CUE = { 46, 56, 76, 255 };
static const Color COBRE_CUE = { 76, 132, 118, 255 };
static const Color TEJA_CUE = { 150, 82, 66, 255 };
static const Color HIERRO_CUE = { 52, 50, 58, 255 };
static const Color LUZ_CALIDA_CUE = { 255, 204, 128, 255 };
static const Color CUERDA_CUE = { 214, 178, 116, 255 };
static const Color PANUELO_CUE = { 255, 248, 230, 255 };
static const Color ORO_CUE = { 255, 200, 70, 255 };


//==================================================
// VISUAL: UTILIDADES
//==================================================

static float Limitar01Cue(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalCue(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoCue(Color c, float f)
{
    return Color{ CanalCue((float)c.r * f), CanalCue((float)c.g * f), CanalCue((float)c.b * f), c.a };
}


static Color MezclaCue(Color a, Color b, float t)
{
    t = Limitar01Cue(t);
    return Color{
        CanalCue((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalCue((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalCue((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalCue((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaCue(Color c, float alfa)
{
    c.a = CanalCue(alfa);
    return c;
}


static float SuaveCue(float u)
{
    u = Limitar01Cue(u);
    return u * u * (3.0f - 2.0f * u);
}


// Hash determinista en [0, 1) para decorar sin parpadeos.
static float Hash01Cue(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 3266489917u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFFu) / 16777216.0f;
}


// Ruido de valor suave (interpolado) para contornos y manchas de pasto.
static float RuidoCue(float x, float z, int capa)
{
    float fx = std::floor(x);
    float fz = std::floor(z);
    int ix = (int)fx;
    int iz = (int)fz;
    float u = SuaveCue(x - fx);
    float v = SuaveCue(z - fz);
    float a = Hash01Cue(ix + capa * 131, iz);
    float b = Hash01Cue(ix + 1 + capa * 131, iz);
    float c = Hash01Cue(ix + capa * 131, iz + 1);
    float d = Hash01Cue(ix + 1 + capa * 131, iz + 1);
    return (a + (b - a) * u) + ((c + (d - c) * u) - (a + (b - a) * u)) * v;
}


// Azar propio del dibujo: nunca consume el GetRandomValue de la logica.
static float AzarCue(MinijuegoCuerdaAcantilado& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFFFu) / 16777216.0f;
    return minimo + (maximo - minimo) * u;
}


// Atardecer: lo lejano se funde con la calima rosada del horizonte.
static Color TinteCue(Color c, Vector3 p)
{
    float dx = p.x - OJO_CUE.x;
    float dy = p.y - OJO_CUE.y;
    float dz = p.z - OJO_CUE.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 24.0f) / 150.0f;
    Color niebla = NIEBLA_CUE;
    niebla.a = c.a;
    return MezclaCue(c, niebla, n < 0.0f ? 0.0f : (n > 0.78f ? 0.78f : n));
}


// Luz falsa por normal: el sol bajo entra de la izquierda y algo de frente.
static float LuzNormalCue(Vector3 n)
{
    float d = n.x * -0.56f + n.y * 0.70f + n.z * 0.44f;
    float f = 0.66f + 0.34f * d;
    return f < 0.48f ? 0.48f : f;
}


static float LuzLateralCue(float angulo)
{
    return LuzNormalCue({ std::cos(angulo), 0.0f, std::sin(angulo) });
}


static void VerticeCue(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadCue(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeCue(a, color);
    VerticeCue(b, color);
    VerticeCue(c, color);
    VerticeCue(d, color);
}


// Caja sobre tres ejes (medios lados ya escalados), caras sombreadas.
static void CajaEjesCue(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color)
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
    QuadCue(v[6], v[7], v[3], v[2], TonoCue(color, 1.0f));     // arriba
    QuadCue(v[4], v[5], v[7], v[6], TonoCue(color, 0.78f));    // frente
    QuadCue(v[1], v[0], v[2], v[3], TonoCue(color, 0.56f));    // fondo
    QuadCue(v[5], v[1], v[3], v[7], TonoCue(color, 0.62f));    // derecha
    QuadCue(v[0], v[4], v[6], v[2], TonoCue(color, 0.70f));    // izquierda
    QuadCue(v[0], v[1], v[5], v[4], TonoCue(color, 0.46f));    // abajo
    rlEnd();
}


static void CuboCue(Vector3 c, Vector3 t, Color color)
{
    CajaEjesCue(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color);
}


static void CuboGiradoCue(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    float a = giroGrados * DEG2RAD;
    float co = std::cos(a);
    float si = std::sin(a);

    CajaEjesCue(
        c,
        { co * t.x * 0.5f, 0.0f, -si * t.x * 0.5f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { si * t.z * 0.5f, 0.0f, co * t.z * 0.5f },
        color
    );
}


// Base ortonormal alrededor de un eje (para tubos y aros orientados).
static void BaseEjeCue(Vector3 dir, Vector3& ex, Vector3& ey)
{
    Vector3 ayuda = std::fabs(dir.y) > 0.9f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    ex = Vector3Normalize(Vector3CrossProduct(ayuda, dir));
    ey = Vector3CrossProduct(dir, ex);
}


// Tubo (tronco de cono) entre dos puntos con caras sombreadas por normal.
static void TuboCue(Vector3 a, Vector3 b, float r0, float r1, Color color, int lados = 8, bool tapa = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 ex, ey;
    BaseEjeCue(dir, ex, ey);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        float am = (a0 + a1) * 0.5f;
        Vector3 n0 = Vector3Add(Vector3Scale(ex, std::cos(a0)), Vector3Scale(ey, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(ex, std::cos(a1)), Vector3Scale(ey, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(ex, std::cos(am)), Vector3Scale(ey, std::sin(am)));
        Color cara = TonoCue(color, LuzNormalCue(nm));

        QuadCue(
            Vector3Add(a, Vector3Scale(n0, r0)),
            Vector3Add(b, Vector3Scale(n0, r1)),
            Vector3Add(b, Vector3Scale(n1, r1)),
            Vector3Add(a, Vector3Scale(n1, r0)),
            cara
        );

        if (tapa)
        {
            Color c = TonoCue(color, LuzNormalCue(dir));
            QuadCue(b, Vector3Add(b, Vector3Scale(n1, r1)), Vector3Add(b, Vector3Scale(n0, r1)), b, c);
        }
    }

    rlEnd();
}


static void VigaCue(Vector3 a, Vector3 b, float grosor, Color color)
{
    TuboCue(a, b, grosor * 0.5f, grosor * 0.5f, color, 4, false);
}


// Tronco de cono vertical (r0 abajo, r1 arriba) con tapa arriba.
static void PrismaCue(Vector3 base, float r0, float r1, float alto, int lados, Color color, bool tapa = true)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = TonoCue(color, 1.0f);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        Color cara = TonoCue(color, LuzLateralCue(a0 + paso * 0.5f));
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadCue(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            cara
        );

        if (tapa && r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadCue(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloCue(Vector3 c, float r0, float r1, Color color, int lados = 24)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadCue(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Aro con grosor en un plano cualquiera (u, v). colorB alterna gajos.
static void AroCue(Vector3 c, Vector3 u, Vector3 v, float radio, float grosor, Color color, Color colorB, int lados = 20)
{
    Vector3 n = Vector3Normalize(Vector3CrossProduct(u, v));

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        Vector3 d0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 d1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Color base = ((k / (lados / 4 > 0 ? lados / 4 : 1)) % 2 == 0) ? color : colorB;
        Color cara = TonoCue(base, LuzNormalCue(Vector3Normalize(Vector3Add(d0, d1))));
        Vector3 h = Vector3Scale(n, grosor * 0.5f);

        Vector3 p0 = Vector3Add(c, Vector3Scale(d0, radio));
        Vector3 p1 = Vector3Add(c, Vector3Scale(d1, radio));
        QuadCue(Vector3Subtract(p0, h), Vector3Add(p0, h), Vector3Add(p1, h), Vector3Subtract(p1, h), cara);

        Vector3 q0 = Vector3Add(c, Vector3Scale(d0, radio - grosor));
        Vector3 q1 = Vector3Add(c, Vector3Scale(d1, radio - grosor));
        Color plano = TonoCue(base, LuzNormalCue(n));
        QuadCue(Vector3Add(q0, h), Vector3Add(p0, h), Vector3Add(p1, h), Vector3Add(q1, h), plano);
        QuadCue(Vector3Subtract(q0, h), Vector3Subtract(p0, h), Vector3Subtract(p1, h), Vector3Subtract(q1, h), TonoCue(plano, 0.8f));
        QuadCue(Vector3Subtract(q0, h), Vector3Add(q0, h), Vector3Add(q1, h), Vector3Subtract(q1, h), TonoCue(cara, 0.8f));
    }

    rlEnd();
}


// Casquete (elipsoide superior) sobre c: copas, cupulas, matas, nubes.
static void DomoCue(Vector3 c, float rx, float ry, float rz, Color color, int lados = 12, int anillos = 4)
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
            float signo = ry < 0.0f ? -1.0f : 1.0f;
            Color cara = TonoCue(color, LuzNormalCue({ std::cos(fm) * std::cos(tm), signo * std::sin(fm), std::cos(fm) * std::sin(tm) }));

            QuadCue(
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


static void EsferaCue(Vector3 c, float r, Color color, int lados = 10, int anillos = 3)
{
    DomoCue(c, r, r, r, color, lados, anillos);
    DomoCue(c, r, -r, r, TonoCue(color, 0.84f), lados, anillos);
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorCue(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloCue(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorCue(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (vista casi de frente, algo desde arriba).
static void ResplandorFrenteCue(Vector3 c, float radio, Color color)
{
    ResplandorCue(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.97f, 0.24f }, radio, color);
}


// Cuadrado que mira a la camara (espuma, polvo, confeti).
static void PlaquitaCue(Vector3 c, float lado, float giroGrados, Color color)
{
    float a = giroGrados * DEG2RAD;
    float co = std::cos(a) * lado * 0.5f;
    float si = std::sin(a) * lado * 0.5f;
    Vector3 u = { co, si * 0.97f, si * 0.24f };
    Vector3 v = { -si, co * 0.97f, co * 0.24f };

    QuadCue(
        { c.x - u.x - v.x, c.y - u.y - v.y, c.z - u.z - v.z },
        { c.x + u.x - v.x, c.y + u.y - v.y, c.z + u.z - v.z },
        { c.x + u.x + v.x, c.y + u.y + v.y, c.z + u.z + v.z },
        { c.x - u.x + v.x, c.y - u.y + v.y, c.z - u.z + v.z },
        color
    );
}


static int CantidadParticulasCue(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


//==================================================
// VISUAL: CONTORNO DE LOS ACANTILADOS
//==================================================
//
// Cada promontorio es un contorno abierto (planta en XZ) que se recorre
// del fondo al canal y del canal al frente. Se subdivide con un temblor
// determinista salvo en el tramo donde tiran los jugadores, que queda
// exacto sobre x = +-BORDE_ACANTILADO_CUERDA. El pasto se rellena en
// abanico desde un ancla interior.

static const int MAX_CONTORNO_CUE = 240;
static const int CANTIDAD_ESTRATOS_CUE = 9;

static const Vector2 CONTORNO_DERECHO_CUE[] =
{
    { 36.0f, -22.0f }, { 29.0f, -22.6f }, { 22.0f, -21.4f }, { 16.5f, -22.0f },
    { 12.5f, -20.6f }, { 9.2f, -19.4f }, { 6.8f, -17.2f }, { 5.4f, -14.6f },
    { 4.5f, -12.2f }, { 4.3f, -9.6f }, { 4.6f, -7.4f }, { 4.0f, -5.2f },
    { 3.75f, -3.4f }, { 3.6f, -2.4f }, { 3.6f, 0.0f }, { 3.6f, 1.9f },
    { 3.85f, 2.9f }, { 4.6f, 3.7f }, { 5.9f, 3.95f }, { 7.4f, 3.6f },
    { 8.9f, 4.1f }, { 10.6f, 3.8f }, { 12.4f, 4.35f }, { 14.2f, 3.9f },
    { 16.4f, 4.5f }, { 18.8f, 4.0f }, { 21.5f, 4.7f }, { 25.0f, 4.2f },
    { 29.0f, 4.8f }, { 36.0f, 4.4f }
};

static const Vector2 CONTORNO_IZQUIERDO_CUE[] =
{
    { -36.0f, -21.0f }, { -30.0f, -21.8f }, { -24.0f, -20.6f }, { -18.0f, -21.4f },
    { -13.5f, -19.8f }, { -10.4f, -18.8f }, { -7.8f, -16.6f }, { -6.1f, -14.2f },
    { -5.0f, -11.8f }, { -4.4f, -9.2f }, { -4.8f, -7.0f }, { -4.1f, -4.8f },
    { -3.7f, -3.2f }, { -3.6f, -2.4f }, { -3.6f, 0.0f }, { -3.6f, 1.7f },
    { -3.9f, 2.8f }, { -4.7f, 3.6f }, { -6.0f, 4.1f }, { -7.6f, 3.7f },
    { -9.3f, 4.3f }, { -11.0f, 3.9f }, { -12.9f, 4.5f }, { -14.8f, 4.0f },
    { -17.0f, 4.6f }, { -19.5f, 4.1f }, { -22.5f, 4.8f }, { -26.0f, 4.3f },
    { -30.0f, 4.9f }, { -36.0f, 4.4f }
};

static const float ALTO_ESTRATO_CUE[CANTIDAD_ESTRATOS_CUE] =
{
    0.45f, 0.85f, 0.6f, 1.1f, 0.7f, 1.2f, 0.9f, 1.0f, 1.5f
};

// Cuanto sobresale (+) o se mete (-) cada estrato respecto del borde.
static const float SALIENTE_ESTRATO_CUE[CANTIDAD_ESTRATOS_CUE] =
{
    0.0f, -0.16f, 0.12f, -0.30f, 0.06f, -0.18f, 0.16f, -0.08f, 0.22f
};

static const Color COLOR_ESTRATO_CUE[CANTIDAD_ESTRATOS_CUE] =
{
    ROCA_PALIDA_CUE, ROCA_OCRE_CUE, ROCA_CREMA_CUE, ROCA_OXIDO_CUE,
    ROCA_GRIS_CUE, ROCA_CREMA_CUE, ROCA_OCRE_CUE, ROCA_OXIDO_CUE,
    ROCA_HUMEDA_CUE
};


struct ContornoCue
{
    int cantidad = 0;
    Vector2 punto[MAX_CONTORNO_CUE];
    Vector2 normal[MAX_CONTORNO_CUE];
    bool juego[MAX_CONTORNO_CUE];       // tramo donde tiran los jugadores
    Vector2 ancla{};
};


static ContornoCue contornosCue[2];
static bool contornosListosCue = false;


static bool EnTramoDeJuegoCue(Vector2 p)
{
    return std::fabs(p.x) < 3.95f && p.y > -2.6f && p.y < 2.3f;
}


static void PrepararContornosCue()
{
    if (contornosListosCue)
    {
        return;
    }

    for (int lado = 0; lado < 2; lado++)
    {
        const Vector2* base = lado == 0 ? CONTORNO_IZQUIERDO_CUE : CONTORNO_DERECHO_CUE;
        int cantidadBase = lado == 0
            ? (int)(sizeof(CONTORNO_IZQUIERDO_CUE) / sizeof(Vector2))
            : (int)(sizeof(CONTORNO_DERECHO_CUE) / sizeof(Vector2));
        ContornoCue& c = contornosCue[lado];
        c.cantidad = 0;
        c.ancla = { lado == 0 ? -36.0f : 36.0f, -9.0f };

        for (int i = 0; i < cantidadBase - 1 && c.cantidad < MAX_CONTORNO_CUE - 1; i++)
        {
            Vector2 a = base[i];
            Vector2 b = base[i + 1];
            float largo = Vector2Distance(a, b);
            int partes = (int)std::ceil(largo / 0.9f);
            Vector2 perp = { (b.y - a.y) / largo, -(b.x - a.x) / largo };

            for (int k = 0; k < partes && c.cantidad < MAX_CONTORNO_CUE - 1; k++)
            {
                float u = (float)k / (float)partes;
                Vector2 p = { a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u };

                if (k > 0 && !EnTramoDeJuegoCue(p))
                {
                    float temblor = (Hash01Cue(c.cantidad + lado * 517, 71) - 0.5f) * 0.36f;
                    p.x += perp.x * temblor;
                    p.y += perp.y * temblor;
                }

                c.punto[c.cantidad++] = p;
            }
        }

        c.punto[c.cantidad++] = base[cantidadBase - 1];

        // Normales hacia afuera (lejos del ancla).
        for (int i = 0; i < c.cantidad; i++)
        {
            Vector2 a = c.punto[i > 0 ? i - 1 : i];
            Vector2 b = c.punto[i < c.cantidad - 1 ? i + 1 : i];
            Vector2 n = Vector2Normalize({ b.y - a.y, -(b.x - a.x) });
            Vector2 haciaAfuera = Vector2Subtract(c.punto[i], c.ancla);

            if (n.x * haciaAfuera.x + n.y * haciaAfuera.y < 0.0f)
            {
                n = Vector2Negate(n);
            }

            c.normal[i] = n;
            c.juego[i] = EnTramoDeJuegoCue(c.punto[i]);
        }
    }

    contornosListosCue = true;
}


// Corrimiento del estrato k en el punto i (el de arriba es el borde exacto).
static float SalienteCue(int lado, int k, int i)
{
    if (k == 0)
    {
        return 0.0f;
    }

    const ContornoCue& c = contornosCue[lado];
    float ruido = (RuidoCue((float)i * 0.45f, (float)k * 3.1f, lado + 5) - 0.5f) * 0.5f;
    float v = SALIENTE_ESTRATO_CUE[k] + ruido;

    // Bajo los jugadores no sobresale nada: la caida queda limpia.
    if (c.juego[i] && v > 0.04f)
    {
        v = 0.04f;
    }

    return v;
}


static Vector3 PuntoEstratoCue(int lado, int k, int i, float y)
{
    const ContornoCue& c = contornosCue[lado];
    float s = SalienteCue(lado, k, i);
    return { c.punto[i].x + c.normal[i].x * s, y, c.punto[i].y + c.normal[i].y * s };
}


// Punto dentro del promontorio (abanico ancla + contorno).
static bool DentroDeAcantiladoCue(int lado, float x, float z)
{
    const ContornoCue& c = contornosCue[lado];
    bool dentro = false;
    int n = c.cantidad + 1;

    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        Vector2 pi = i < c.cantidad ? c.punto[i] : c.ancla;
        Vector2 pj = j < c.cantidad ? c.punto[j] : c.ancla;

        if (((pi.y > z) != (pj.y > z)) && (x < (pj.x - pi.x) * (z - pi.y) / (pj.y - pi.y) + pi.x))
        {
            dentro = !dentro;
        }
    }

    // El abanico cubre tambien el borde lejano (fuera de cuadro).
    return dentro;
}


//==================================================
// VISUAL: ESTADO Y EFECTOS (una vez por frame desde Actualizar)
//==================================================

// Pies para el dibujo. Igual que la logica mientras se juega; al final, el
// bando perdedor se arrastra por el pasto (mismo avance horizontal que la
// logica) y cae recien al pasar el borde, en vez de hundirse en la roca.
static Vector3 PiesVisualesCuerda(const MinijuegoCuerdaAcantilado& m, int indice)
{
    Vector3 base = PosicionBasePiesCuerda(m, indice);
    Vector3 pies = base;

    if (m.fase != FASE_CUERDA_TERMINADO || m.empate)
    {
        return pies;
    }

    const EstadoJugadorCuerdaAcantilado& estado = m.estadosJugadores[indice];
    bool perdedor = (estado.equipo == 0) == !m.ganaSolo;

    if (!perdedor)
    {
        return pies;
    }

    float t = m.tiempoFin - 0.25f - 0.12f * (float)estado.ordenEnEquipo;

    if (t <= 0.0f)
    {
        return pies;
    }

    float haciaCentro = estado.equipo == 0 ? 1.0f : -1.0f;
    pies.x += haciaCentro * 2.4f * t;

    float distanciaBorde = std::fabs(base.x) - BORDE_ACANTILADO_CUERDA;
    float tBorde = (distanciaBorde > 0.0f ? distanciaBorde : 0.0f) / 2.4f;

    if (t > tBorde)
    {
        float c = t - tBorde;
        pies.y = -9.0f * c * c;

        if (pies.y < ALTURA_MAR_CUERDA - 1.2f)
        {
            pies.y = ALTURA_MAR_CUERDA - 1.2f;
        }
    }

    return pies;
}


static bool EsPerdedorCue(const MinijuegoCuerdaAcantilado& m, int indice)
{
    if (m.fase != FASE_CUERDA_TERMINADO || m.empate)
    {
        return false;
    }

    return (m.estadosJugadores[indice].equipo == 0) == !m.ganaSolo;
}


static void EmitirParticulaCue(
    MinijuegoCuerdaAcantilado& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool brillo,
    bool papel = false
)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_CUERDA; i++)
    {
        ParticulaVisualCuerda& p = m.particulasVisuales[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.papel = papel;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.color = color;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.giro = AzarCue(m, 0.0f, 360.0f);
        return;
    }
}


// Rocio de particulas hacia arriba; empuje corre el abanico en X.
static void EstallidoCue(
    MinijuegoCuerdaAcantilado& m,
    Vector3 centro,
    int cantidad,
    Color color,
    float fuerza,
    float empujeX,
    float vida,
    float tamano,
    float gravedad,
    bool brillo
)
{
    int n = CantidadParticulasCue(cantidad);

    for (int k = 0; k < n; k++)
    {
        float a = AzarCue(m, 0.0f, 6.2832f);
        float f = AzarCue(m, 0.3f, 1.0f) * fuerza;
        Vector3 v = { std::cos(a) * f + empujeX, AzarCue(m, 0.6f, 1.4f) * fuerza, std::sin(a) * f * 0.6f };
        EmitirParticulaCue(m, centro, v, color, vida * AzarCue(m, 0.7f, 1.2f), tamano * AzarCue(m, 0.7f, 1.3f), gravedad, brillo);
    }
}


static void ReiniciarVisualCuerda(MinijuegoCuerdaAcantilado& m)
{
    m.semillaVisual = 2246822519u;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.aciertosVistos[i] = 0;
        m.fallosVistos[i] = 0;
        m.destelloTiron[i] = 0.0f;
        m.anuncioRacha[i] = 0.0f;
        m.caidaVista[i] = false;
        m.chapuzonVisto[i] = false;
    }

    for (int i = 0; i < MAX_PARTICULAS_VISUALES_CUERDA; i++)
    {
        m.particulasVisuales[i] = {};
    }

    m.esperaOla = 0.4f;
    m.esperaConfeti = 0.0f;
    m.finVisto = false;
}


// Una ola que rompe contra la base de un acantilado: rocio blanco.
static void RomperOlaCue(MinijuegoCuerdaAcantilado& m)
{
    PrepararContornosCue();

    int lado = AzarCue(m, 0.0f, 1.0f) < 0.5f ? 0 : 1;
    const ContornoCue& c = contornosCue[lado];

    // Solo las caras que se ven: el canal y el frente.
    int i = (int)AzarCue(m, (float)c.cantidad * 0.3f, (float)c.cantidad * 0.82f);
    i = i < 0 ? 0 : (i >= c.cantidad ? c.cantidad - 1 : i);

    float saliente = SalienteCue(lado, CANTIDAD_ESTRATOS_CUE - 1, i);
    Vector3 p =
    {
        c.punto[i].x + c.normal[i].x * (saliente + 0.25f),
        ALTURA_MAR_CUERDA + 0.1f,
        c.punto[i].y + c.normal[i].y * (saliente + 0.25f)
    };

    int n = CantidadParticulasCue(9);

    for (int k = 0; k < n; k++)
    {
        float f = AzarCue(m, 0.4f, 1.0f);
        Vector3 v =
        {
            c.normal[i].x * AzarCue(m, 0.3f, 1.4f) + AzarCue(m, -0.4f, 0.4f),
            AzarCue(m, 2.6f, 5.4f) * f,
            c.normal[i].y * AzarCue(m, 0.3f, 1.4f) + AzarCue(m, -0.4f, 0.4f)
        };
        EmitirParticulaCue(m, p, v, AlfaCue(ESPUMA_CUE, 220.0f), AzarCue(m, 0.9f, 1.5f), AzarCue(m, 0.16f, 0.32f), 7.0f, false);
    }
}


static void ActualizarVisualCuerda(
    MinijuegoCuerdaAcantilado& m,
    float deltaTime,
    int limite,
    const Participante participantes[]
)
{
    // Particulas.
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_CUERDA; i++)
    {
        ParticulaVisualCuerda& p = m.particulasVisuales[i];

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
        p.velocidad.x *= 1.0f - 0.8f * deltaTime;
        p.velocidad.z *= 1.0f - 0.8f * deltaTime;
        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, deltaTime));
        p.giro += 220.0f * deltaTime;

        // La espuma y el polvo no atraviesan el mar.
        if (p.posicion.y < ALTURA_MAR_CUERDA && p.velocidad.y < 0.0f)
        {
            p.activa = false;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.destelloTiron[i] = fmaxf(0.0f, m.destelloTiron[i] - deltaTime * 2.5f);
        m.anuncioRacha[i] = fmaxf(0.0f, m.anuncioRacha[i] - deltaTime);
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCuerdaAcantilado& estado = m.estadosJugadores[i];
        Vector3 pies = PiesVisualesCuerda(m, i);
        bool humano = !participantes[i].esBot && participantes[i].conectado;

        // El cuerpo tira hacia atras: el polvo sale de los talones.
        float atras = estado.equipo == 0 ? -1.0f : 1.0f;

        if (estado.aciertos > m.aciertosVistos[i])
        {
            m.aciertosVistos[i] = estado.aciertos;
            m.destelloTiron[i] = 1.0f;
            EstallidoCue(m, { pies.x + atras * 0.25f, 0.08f, pies.z }, 6, TIERRA_CUE, 1.3f, atras * 1.2f, 0.55f, 0.12f, 6.0f, false);
            EstallidoCue(m, { pies.x, ALTURA_MANOS_CUERDA, pies.z }, 5, MezclaCue(participantes[i].color, WHITE, 0.3f), 1.6f, 0.0f, 0.5f, 0.09f, 2.0f, true);

            if (estado.feedback == FEEDBACK_CUERDA_RACHA)
            {
                m.anuncioRacha[i] = 1.2f;
                EstallidoCue(m, { pies.x, 1.4f, pies.z }, 14, ORO_CUE, 3.0f, 0.0f, 0.8f, 0.11f, 3.0f, true);
                ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
            }
            else if (humano)
            {
                ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
            }
        }

        if (estado.fallos > m.fallosVistos[i])
        {
            m.fallosVistos[i] = estado.fallos;
            EstallidoCue(m, { pies.x - atras * 0.35f, 0.1f, pies.z }, 8, Color{ 168, 152, 132, 255 }, 1.4f, -atras * 0.6f, 0.7f, 0.15f, 5.0f, false);

            if (humano)
            {
                ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
            }
        }

        // Caida del bando perdedor: piedritas en el borde y chapuzon.
        if (EsPerdedorCue(m, i))
        {
            if (pies.y < -0.05f && !m.caidaVista[i])
            {
                m.caidaVista[i] = true;
                ReproducirSonidoMinijuego(m.audio, SONIDO_CAIDA);
                ActivarTemblorCamaraGeneral(0.10f, 0.22f);
                EstallidoCue(m, { pies.x, 0.0f, pies.z }, 10, ROCA_OCRE_CUE, 1.2f, -atras * 0.8f, 1.4f, 0.14f, 9.0f, false);
            }

            if (pies.y <= ALTURA_MAR_CUERDA && !m.chapuzonVisto[i])
            {
                m.chapuzonVisto[i] = true;
                EstallidoCue(m, { pies.x, ALTURA_MAR_CUERDA + 0.1f, pies.z }, 26, AlfaCue(ESPUMA_CUE, 235.0f), 4.2f, 0.0f, 1.3f, 0.26f, 8.0f, false);
            }
        }
    }

    // Confeti sobre el bando ganador (o sobre todos si hay empate).
    if (m.fase == FASE_CUERDA_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        m.finVisto = true;
        m.esperaConfeti -= deltaTime;

        if (m.tiempoFin > 0.9f && m.tiempoFin < 7.0f && m.esperaConfeti <= 0.0f)
        {
            m.esperaConfeti = 0.12f;
            const Color papeles[5] =
            {
                Color{ 255, 226, 120, 255 }, Color{ 255, 255, 255, 255 }, Color{ 120, 220, 240, 255 },
                Color{ 250, 150, 200, 255 }, Color{ 170, 240, 140, 255 }
            };

            for (int i = 0; i < limite; i++)
            {
                if (!m.resultado.participantes[i].participo || EsPerdedorCue(m, i))
                {
                    continue;
                }

                Vector3 pies = PiesVisualesCuerda(m, i);
                int n = CantidadParticulasCue(3);

                for (int k = 0; k < n; k++)
                {
                    Color c = AzarCue(m, 0.0f, 1.0f) < 0.3f ? participantes[i].color : papeles[(int)AzarCue(m, 0.0f, 4.99f)];
                    Vector3 p = { pies.x + AzarCue(m, -0.9f, 0.9f), 3.6f + AzarCue(m, 0.0f, 0.8f), pies.z + AzarCue(m, -0.6f, 0.6f) };
                    Vector3 v = { AzarCue(m, -0.8f, 0.8f), AzarCue(m, -0.2f, 0.6f), AzarCue(m, -0.3f, 0.3f) };
                    EmitirParticulaCue(m, p, v, c, AzarCue(m, 2.0f, 2.8f), AzarCue(m, 0.09f, 0.14f), 1.6f, false, true);
                }
            }
        }
    }

    // Olas que rompen contra la base de los acantilados.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        m.esperaOla -= deltaTime;

        if (m.esperaOla <= 0.0f)
        {
            m.esperaOla = AzarCue(m, 0.2f, 0.5f) / FactorCalidadGrafica(CalidadParticulas());
            RomperOlaCue(m);
        }
    }
}


//==================================================
// VISUAL: CIELO (2D, antes del 3D)
//==================================================

static float HorizontePantallaCue(const Camera3D& vista)
{
    float alto = (float)GetScreenHeight();
    float y = GetWorldToScreen({ SOL_X_CUE, ALTURA_MAR_CUERDA, Z_HORIZONTE_CUE }, vista).y;
    return y < alto * 0.08f ? alto * 0.08f : (y > alto * 0.9f ? alto * 0.9f : y);
}


static void DibujarNubeCue(float cx, float cy, float ancho, float alto, Color cuerpo, Color borde, int semilla)
{
    // Panza encendida por el sol bajo y cuerpo en bocanadas.
    DrawEllipse((int)cx, (int)(cy + alto * 0.28f), ancho * 0.5f, alto * 0.42f, borde);

    for (int p = 0; p < 4; p++)
    {
        float px = cx - ancho * 0.34f + (float)p * ancho * 0.22f;
        float py = cy - alto * 0.18f * Hash01Cue(semilla, p);
        float rx = ancho * (0.17f + 0.06f * Hash01Cue(semilla, p + 9));
        float ry = alto * (0.45f + 0.25f * Hash01Cue(semilla, p + 17));
        DrawEllipse((int)px, (int)py, rx, ry, cuerpo);
    }

    DrawEllipse((int)cx, (int)(cy + alto * 0.12f), ancho * 0.42f, alto * 0.22f, MezclaCue(cuerpo, borde, 0.5f));
}


static void DibujarCieloCue(const Camera3D& vista, float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float yH = HorizontePantallaCue(vista);
    int y1 = (int)(yH * 0.5f);

    DrawRectangleGradientV(0, 0, ancho, y1, CIELO_ALTO_CUE, CIELO_MEDIO_CUE);
    DrawRectangleGradientV(0, y1, ancho, (int)yH - y1 + 1, CIELO_MEDIO_CUE, CIELO_HORIZONTE_CUE);
    DrawRectangle(0, (int)yH, ancho, alto - (int)yH, CIELO_HORIZONTE_CUE);

    // Primeras estrellas en lo alto.
    if (detalle)
    {
        for (int k = 0; k < 46; k++)
        {
            float x = Hash01Cue(k, 11) * (float)ancho;
            float y = Hash01Cue(k, 12) * yH * 0.42f;
            float brillo = (1.0f - y / (yH * 0.42f)) * (0.55f + 0.45f * std::sin(t * (1.5f + Hash01Cue(k, 13) * 2.0f) + (float)k));
            DrawCircleV({ x, y }, 0.8f + 0.9f * Hash01Cue(k, 14), AlfaCue(WHITE, 200.0f * Limitar01Cue(brillo)));
        }
    }

    // Sol poniendose sobre el horizonte del canal, con franjas de calor.
    Vector2 h = GetWorldToScreen({ SOL_X_CUE, ALTURA_MAR_CUERDA, Z_HORIZONTE_CUE }, vista);
    float r = (float)alto * 0.075f;
    Vector2 sol = { h.x, yH - r * 0.32f };

    DrawCircleGradient(sol, r * 6.5f, AlfaCue(Color{ 255, 168, 112, 255 }, 96.0f), AlfaCue(Color{ 255, 150, 120, 255 }, 0.0f));
    DrawCircleGradient(sol, r * 2.4f, AlfaCue(Color{ 255, 214, 150, 255 }, 160.0f), AlfaCue(Color{ 255, 190, 130, 255 }, 0.0f));
    DrawCircleV(sol, r, SOL_CUE);
    DrawCircleV(sol, r * 0.78f, Color{ 255, 244, 204, 255 });

    for (int b = 0; b < 4; b++)
    {
        float by = sol.y + r * (0.08f + 0.22f * (float)b);
        float grosor = r * (0.04f + 0.035f * (float)b);
        float u = (by - (float)y1) / (yH - (float)y1);
        DrawRectangle((int)(sol.x - r - 1.0f), (int)by, (int)(r * 2.0f + 2.0f), (int)(grosor + 1.0f), MezclaCue(CIELO_MEDIO_CUE, CIELO_HORIZONTE_CUE, u));
    }

    if (!detalle)
    {
        return;
    }

    // Nubes alargadas que derivan despacio (las de abajo, mas encendidas).
    for (int k = 0; k < 11; k++)
    {
        float velocidad = 4.0f + 6.0f * Hash01Cue(k, 21);
        float recorrido = (float)ancho * 1.5f;
        float x = std::fmod(Hash01Cue(k, 22) * recorrido + t * velocidad, recorrido) - (float)ancho * 0.25f;
        float altura = 0.12f + 0.78f * Hash01Cue(k, 23);
        float y = yH * altura;
        float largo = (float)ancho * (0.09f + 0.14f * Hash01Cue(k, 24));
        float grueso = largo * (0.12f + 0.08f * Hash01Cue(k, 25));
        Color cuerpo = MezclaCue(Color{ 128, 90, 140, 255 }, Color{ 236, 140, 136, 255 }, altura);
        Color borde = MezclaCue(Color{ 236, 150, 150, 255 }, Color{ 255, 206, 150, 255 }, altura);
        DibujarNubeCue(x, y, largo, grueso, cuerpo, borde, k);
    }
}


//==================================================
// VISUAL: MAR, HORIZONTE Y FARALLONES
//==================================================

static Color ColorMarCue(float x, float z, float t, bool olas)
{
    float d = Limitar01Cue((14.0f - z) / 434.0f);
    Color c = MezclaCue(MAR_CERCA_CUE, MAR_MEDIO_CUE, SuaveCue(d * 5.0f));
    c = MezclaCue(c, MAR_LEJOS_CUE, std::pow(d, 0.55f));

    if (olas)
    {
        float cerca = 1.0f - Limitar01Cue(d * 7.0f);
        float w = std::sin(x * 0.8f + t * 1.2f + z * 0.45f) * std::sin(z * 1.1f - t * 1.7f + x * 0.25f);
        c = TonoCue(c, 1.0f + 0.14f * w * cerca);
    }

    return c;
}


static void DibujarMarCue(float t, bool olas)
{
    const int FILAS = 40;
    const int COLUMNAS = 64;
    float y = ALTURA_MAR_CUERDA;

    rlBegin(RL_QUADS);

    for (int j = 0; j < FILAS; j++)
    {
        float u0 = (float)j / (float)FILAS;
        float u1 = (float)(j + 1) / (float)FILAS;
        float z0 = 26.0f - 446.0f * std::pow(u0, 1.9f);
        float z1 = 26.0f - 446.0f * std::pow(u1, 1.9f);

        for (int i = 0; i < COLUMNAS; i++)
        {
            float s0 = -1.0f + 2.0f * (float)i / (float)COLUMNAS;
            float s1 = -1.0f + 2.0f * (float)(i + 1) / (float)COLUMNAS;
            float x0 = 300.0f * s0 * std::fabs(s0);
            float x1 = 300.0f * s1 * std::fabs(s1);

            VerticeCue({ x0, y, z0 }, ColorMarCue(x0, z0, t, olas));
            VerticeCue({ x1, y, z0 }, ColorMarCue(x1, z0, t, olas));
            VerticeCue({ x1, y, z1 }, ColorMarCue(x1, z1, t, olas));
            VerticeCue({ x0, y, z1 }, ColorMarCue(x0, z1, t, olas));
        }
    }

    rlEnd();
}


// Altura de una silueta lejana: cae a pique en la punta que mira al canal.
static float AlturaSiluetaCue(float x, float xPunta, float xFin, float hMin, float hMax, int capa)
{
    float h = hMin + (hMax - hMin) * RuidoCue(x * 0.045f, 0.5f, capa);
    h += 2.5f * (RuidoCue(x * 0.18f, 1.5f, capa + 40) - 0.5f);
    float e = std::fabs(x - xPunta);
    float f = std::fabs(xFin - x);
    return h * (0.15f + 0.85f * SuaveCue(e / 7.0f)) * (0.3f + 0.7f * SuaveCue(f / 30.0f));
}


static void DibujarSiluetaCue(float z, float xPunta, float xFin, float hMin, float hMax, int capa, Color color)
{
    float paso = 3.0f;
    float sentido = xFin > xPunta ? 1.0f : -1.0f;
    int pasos = (int)(std::fabs(xFin - xPunta) / paso);

    rlBegin(RL_QUADS);

    for (int k = 0; k < pasos; k++)
    {
        float xa = xPunta + sentido * paso * (float)k;
        float xb = xa + sentido * paso;
        float ha = AlturaSiluetaCue(xa, xPunta, xFin, hMin, hMax, capa);
        float hb = AlturaSiluetaCue(xb, xPunta, xFin, hMin, hMax, capa);
        float vetaA = 0.94f + 0.12f * Hash01Cue(k, capa);
        Color arriba = TinteCue(TonoCue(color, 1.12f * vetaA), { xa, ha, z });
        Color abajo = TinteCue(MezclaCue(color, NIEBLA_CUE, 0.35f), { xa, 0.0f, z });

        VerticeCue({ xa, ALTURA_MAR_CUERDA, z }, abajo);
        VerticeCue({ xb, ALTURA_MAR_CUERDA, z }, abajo);
        VerticeCue({ xb, ALTURA_MAR_CUERDA + hb, z }, arriba);
        VerticeCue({ xa, ALTURA_MAR_CUERDA + ha, z }, arriba);
    }

    rlEnd();
}


static Vector3 FaroLejanoCue()
{
    float x = -42.0f;
    float h = AlturaSiluetaCue(x, -26.0f, -170.0f, 6.0f, 15.0f, 3);
    return { x, ALTURA_MAR_CUERDA + h, -121.0f };
}


static void DibujarHorizonteCue(float t, bool completo)
{
    // Capa lejana (mas palida) y capa cercana de promontorios.
    if (completo)
    {
        DibujarSiluetaCue(-232.0f, -62.0f, -320.0f, 10.0f, 30.0f, 1, Color{ 186, 116, 134, 255 });
        DibujarSiluetaCue(-236.0f, 72.0f, 330.0f, 9.0f, 26.0f, 2, Color{ 190, 120, 136, 255 });
    }

    DibujarSiluetaCue(-120.0f, -26.0f, -170.0f, 6.0f, 15.0f, 3, Color{ 120, 74, 96, 255 });
    DibujarSiluetaCue(-126.0f, 30.0f, 190.0f, 5.0f, 14.0f, 4, Color{ 126, 78, 98, 255 });

    // Islotes bajos junto al sol.
    DomoCue({ -36.0f, ALTURA_MAR_CUERDA, -300.0f }, 15.0f, 3.4f, 6.0f, TinteCue(Color{ 150, 92, 112, 255 }, { -36.0f, 0.0f, -300.0f }), 10, 3);
    DomoCue({ 30.0f, ALTURA_MAR_CUERDA, -320.0f }, 9.0f, 2.2f, 5.0f, TinteCue(Color{ 150, 92, 112, 255 }, { 30.0f, 0.0f, -320.0f }), 10, 3);

    // Faro lejano en la punta del promontorio izquierdo.
    Vector3 f = FaroLejanoCue();
    PrismaCue(f, 0.9f, 0.7f, 4.5f, 6, TinteCue(CAL_CUE, f));

    // Velero que cruza muy despacio.
    if (completo)
    {
        float x = -70.0f + std::fmod(t * 0.9f, 140.0f);
        float z = -160.0f;
        Color casco = TinteCue(Color{ 70, 52, 60, 255 }, { x, 0.0f, z });
        Color vela = TinteCue(Color{ 250, 226, 210, 255 }, { x, 0.0f, z });
        CuboCue({ x, ALTURA_MAR_CUERDA + 0.35f, z }, { 3.6f, 0.7f, 1.0f }, casco);
        rlBegin(RL_TRIANGLES);
        VerticeCue({ x - 0.2f, ALTURA_MAR_CUERDA + 0.8f, z }, vela);
        VerticeCue({ x + 1.5f, ALTURA_MAR_CUERDA + 0.8f, z }, vela);
        VerticeCue({ x - 0.2f, ALTURA_MAR_CUERDA + 5.0f, z }, vela);
        VerticeCue({ x - 0.4f, ALTURA_MAR_CUERDA + 0.8f, z }, TonoCue(vela, 0.85f));
        VerticeCue({ x - 0.4f, ALTURA_MAR_CUERDA + 4.2f, z }, TonoCue(vela, 0.85f));
        VerticeCue({ x - 1.9f, ALTURA_MAR_CUERDA + 0.8f, z }, TonoCue(vela, 0.85f));
        rlEnd();
    }
}


// Farallon de roca estratificada con un copete de pasto.
static void DibujarFarallonCue(Vector3 base, float radio, float alto, int semilla)
{
    float y = base.y;
    int k = 0;

    while (y < base.y + alto - 0.01f)
    {
        float capa = 0.7f + 0.8f * Hash01Cue(semilla, k);
        if (y + capa > base.y + alto) capa = base.y + alto - y;
        float u0 = (y - base.y) / alto;
        float u1 = (y + capa - base.y) / alto;
        float r0 = radio * (1.0f - 0.32f * u0) * (0.92f + 0.14f * Hash01Cue(semilla, k + 30));
        float r1 = radio * (1.0f - 0.32f * u1) * (0.92f + 0.14f * Hash01Cue(semilla, k + 31));
        Color c = COLOR_ESTRATO_CUE[1 + (k + semilla) % (CANTIDAD_ESTRATOS_CUE - 2)];
        if (k == 0) c = ROCA_HUMEDA_CUE;
        PrismaCue({ base.x, y, base.z }, r0, r1, capa, 7, TinteCue(c, { base.x, y, base.z }));
        y += capa;
        k++;
    }

    DomoCue({ base.x, y, base.z }, radio * 0.72f, 0.45f, radio * 0.72f, TinteCue(PASTO_CUE, { base.x, y, base.z }), 7, 2);
}


static void DibujarFarallonesCue()
{
    DibujarFarallonCue({ -15.5f, ALTURA_MAR_CUERDA - 0.6f, -40.0f }, 2.8f, 12.6f, 3);
    DibujarFarallonCue({ -26.0f, ALTURA_MAR_CUERDA - 0.6f, -50.0f }, 2.0f, 9.0f, 5);
    DibujarFarallonCue({ 19.5f, ALTURA_MAR_CUERDA - 0.6f, -44.0f }, 2.4f, 10.8f, 7);

    // Arco natural a la derecha.
    Color roca = TinteCue(ROCA_OCRE_CUE, { 27.0f, 0.0f, -37.0f });
    DibujarFarallonCue({ 24.0f, ALTURA_MAR_CUERDA - 0.6f, -36.0f }, 1.6f, 7.6f, 9);
    DibujarFarallonCue({ 30.5f, ALTURA_MAR_CUERDA - 0.6f, -37.0f }, 1.7f, 7.2f, 11);
    CuboCue({ 27.2f, 0.9f, -36.5f }, { 7.6f, 1.6f, 2.4f }, roca);
    CuboCue({ 27.2f, 1.78f, -36.5f }, { 7.0f, 0.2f, 2.0f }, TinteCue(PASTO_CUE, { 27.0f, 2.0f, -36.5f }));
}


//==================================================
// VISUAL: ACANTILADOS (roca estratificada y pasto)
//==================================================

static void DibujarParedesAcantiladoCue(int lado)
{
    const ContornoCue& c = contornosCue[lado];
    float yArriba = 0.0f;

    for (int k = 0; k < CANTIDAD_ESTRATOS_CUE; k++)
    {
        float yAbajo = yArriba - ALTO_ESTRATO_CUE[k];

        rlBegin(RL_QUADS);

        for (int i = 0; i < c.cantidad - 1; i++)
        {
            Vector3 a0 = PuntoEstratoCue(lado, k, i, yArriba);
            Vector3 b0 = PuntoEstratoCue(lado, k, i + 1, yArriba);
            Vector3 a1 = PuntoEstratoCue(lado, k, i, yAbajo);
            Vector3 b1 = PuntoEstratoCue(lado, k, i + 1, yAbajo);
            Vector2 n = Vector2Normalize(Vector2Add(c.normal[i], c.normal[i + 1]));
            float luz = LuzNormalCue({ n.x, 0.0f, n.y });
            float veta = 0.9f + 0.2f * Hash01Cue(i * 13 + k * 7, lado + 3);
            Color cara = TonoCue(COLOR_ESTRATO_CUE[k], luz * veta);
            Color sombra = TonoCue(cara, 0.82f);

            VerticeCue(a0, TinteCue(cara, a0));
            VerticeCue(a1, TinteCue(sombra, a1));
            VerticeCue(b1, TinteCue(sombra, b1));
            VerticeCue(b0, TinteCue(cara, b0));
        }

        // Repisa entre este estrato y el de abajo (o su panza si se mete).
        if (k < CANTIDAD_ESTRATOS_CUE - 1)
        {
            for (int i = 0; i < c.cantidad - 1; i++)
            {
                Vector3 p0 = PuntoEstratoCue(lado, k, i, yAbajo);
                Vector3 p1 = PuntoEstratoCue(lado, k, i + 1, yAbajo);
                Vector3 q0 = PuntoEstratoCue(lado, k + 1, i, yAbajo);
                Vector3 q1 = PuntoEstratoCue(lado, k + 1, i + 1, yAbajo);
                bool sale = SalienteCue(lado, k + 1, i) > SalienteCue(lado, k, i);
                Color repisa = sale ? TonoCue(COLOR_ESTRATO_CUE[k + 1], 1.04f) : TonoCue(COLOR_ESTRATO_CUE[k], 0.5f);

                VerticeCue(p0, TinteCue(repisa, p0));
                VerticeCue(p1, TinteCue(repisa, p1));
                VerticeCue(q1, TinteCue(repisa, q1));
                VerticeCue(q0, TinteCue(repisa, q0));
            }
        }

        rlEnd();
        yArriba = yAbajo;
    }
}


static float TonoPastoCue(float x, float z)
{
    return 0.62f * RuidoCue(x * 0.2f, z * 0.2f, 61) + 0.38f * RuidoCue(x * 0.7f, z * 0.7f, 62);
}


static Color ColorPastoCue(float x, float z)
{
    float u = TonoPastoCue(x, z);
    Color c = u < 0.5f
        ? MezclaCue(PASTO_OSCURO_CUE, PASTO_CUE, u * 2.0f)
        : MezclaCue(PASTO_CUE, PASTO_CLARO_CUE, (u - 0.5f) * 2.0f);
    return TinteCue(c, { x, 0.0f, z });
}


static const int CELDAS_X_PASTO_CUE = 27;
static const int CELDAS_Z_PASTO_CUE = 30;


static void DibujarPastoAcantiladoCue(int lado, bool detalle)
{
    const ContornoCue& c = contornosCue[lado];
    Vector3 ancla = { c.ancla.x, 0.0f, c.ancla.y };

    // Abanico de base y, encima, una malla con manchas suaves.
    rlBegin(RL_TRIANGLES);

    for (int i = 0; i < c.cantidad - 1; i++)
    {
        Vector3 a = { c.punto[i].x, 0.0f, c.punto[i].y };
        Vector3 b = { c.punto[i + 1].x, 0.0f, c.punto[i + 1].y };
        VerticeCue(ancla, ColorPastoCue(ancla.x, ancla.z));
        VerticeCue(a, ColorPastoCue(a.x, a.z));
        VerticeCue(b, ColorPastoCue(b.x, b.z));
    }

    rlEnd();

    if (detalle)
    {
        // Que celdas de la malla caen enteras dentro (se calcula una vez).
        static bool celdaDentro[2][CELDAS_X_PASTO_CUE][CELDAS_Z_PASTO_CUE];
        static bool celdasListas[2] = { false, false };
        float s = lado == 0 ? -1.0f : 1.0f;

        if (!celdasListas[lado])
        {
            for (int ix = 0; ix < CELDAS_X_PASTO_CUE; ix++)
            {
                for (int iz = 0; iz < CELDAS_Z_PASTO_CUE; iz++)
                {
                    float x0 = s * (3.4f + 0.9f * (float)ix);
                    float x1 = s * (3.4f + 0.9f * (float)(ix + 1));
                    float gz = -22.0f + 0.9f * (float)iz;
                    celdaDentro[lado][ix][iz] =
                        DentroDeAcantiladoCue(lado, x0, gz) && DentroDeAcantiladoCue(lado, x1, gz) &&
                        DentroDeAcantiladoCue(lado, x0, gz + 0.9f) && DentroDeAcantiladoCue(lado, x1, gz + 0.9f);
                }
            }

            celdasListas[lado] = true;
        }

        rlBegin(RL_QUADS);

        for (int ix = 0; ix < CELDAS_X_PASTO_CUE; ix++)
        {
            for (int iz = 0; iz < CELDAS_Z_PASTO_CUE; iz++)
            {
                if (!celdaDentro[lado][ix][iz])
                {
                    continue;
                }

                float x0 = s * (3.4f + 0.9f * (float)ix);
                float x1 = s * (3.4f + 0.9f * (float)(ix + 1));
                float gz = -22.0f + 0.9f * (float)iz;

                VerticeCue({ x0, 0.004f, gz }, ColorPastoCue(x0, gz));
                VerticeCue({ x0, 0.004f, gz + 0.9f }, ColorPastoCue(x0, gz + 0.9f));
                VerticeCue({ x1, 0.004f, gz + 0.9f }, ColorPastoCue(x1, gz + 0.9f));
                VerticeCue({ x1, 0.004f, gz }, ColorPastoCue(x1, gz));
            }
        }

        rlEnd();
    }

    // Labio de pasto que asoma sobre la roca y matas que cuelgan.
    rlBegin(RL_QUADS);

    for (int i = 0; i < c.cantidad - 1; i++)
    {
        Vector2 na = c.normal[i];
        Vector2 nb = c.normal[i + 1];
        float va = c.juego[i] ? 0.0f : 0.07f;
        float vb = c.juego[i + 1] ? 0.0f : 0.07f;
        Vector3 a = { c.punto[i].x + na.x * va, 0.03f, c.punto[i].y + na.y * va };
        Vector3 b = { c.punto[i + 1].x + nb.x * vb, 0.03f, c.punto[i + 1].y + nb.y * vb };
        Color pasto = TinteCue(PASTO_OSCURO_CUE, a);

        VerticeCue({ c.punto[i].x, 0.03f, c.punto[i].y }, TinteCue(PASTO_CUE, a));
        VerticeCue(a, TinteCue(PASTO_CUE, a));
        VerticeCue(b, TinteCue(PASTO_CUE, b));
        VerticeCue({ c.punto[i + 1].x, 0.03f, c.punto[i + 1].y }, TinteCue(PASTO_CUE, b));

        VerticeCue(a, pasto);
        VerticeCue({ a.x, -0.2f, a.z }, TonoCue(pasto, 0.7f));
        VerticeCue({ b.x, -0.2f, b.z }, TonoCue(pasto, 0.7f));
        VerticeCue(b, pasto);
    }

    rlEnd();

    if (detalle)
    {
        rlBegin(RL_TRIANGLES);

        for (int i = 0; i < c.cantidad - 1; i++)
        {
            if (c.juego[i] || Hash01Cue(i, lado + 120) < 0.6f)
            {
                continue;
            }

            Vector2 n = c.normal[i];
            Vector2 t = { -n.y, n.x };
            float largo = 0.18f + 0.28f * Hash01Cue(i, lado + 121);
            Vector3 p = { c.punto[i].x + n.x * 0.1f, -0.15f, c.punto[i].y + n.y * 0.1f };
            Color mata = TinteCue(Color{ 62, 98, 52, 255 }, p);

            VerticeCue({ p.x - t.x * 0.25f, p.y + 0.12f, p.z - t.y * 0.25f }, mata);
            VerticeCue({ p.x + t.x * 0.25f, p.y + 0.12f, p.z + t.y * 0.25f }, mata);
            VerticeCue({ p.x + n.x * 0.12f, p.y - largo, p.z + n.y * 0.12f }, TonoCue(mata, 0.7f));
        }

        rlEnd();
    }
}


// Tierra pisada donde tira cada bando (sigue el rango real de los pies).
static void DibujarHuellasCue(float xDesde, float xHasta, int semilla)
{
    rlBegin(RL_QUADS);

    for (float x = xDesde; x < xHasta - 0.01f; x += 0.4f)
    {
        float u = (x - xDesde) / (xHasta - xDesde);
        float ancho = 0.55f + 0.25f * Hash01Cue((int)(x * 10.0f), semilla) + 0.1f * std::sin(u * 9.0f);
        float borde = SuaveCue(u * 6.0f) * SuaveCue((1.0f - u) * 6.0f);
        Color centro = AlfaCue(TinteCue(TIERRA_CUE, { x, 0.0f, 0.0f }), 210.0f * borde);
        Color afuera = AlfaCue(centro, 0.0f);
        float x1 = x + 0.4f;

        VerticeCue({ x, 0.01f, -ancho }, afuera);
        VerticeCue({ x, 0.01f, 0.0f }, centro);
        VerticeCue({ x1, 0.01f, 0.0f }, centro);
        VerticeCue({ x1, 0.01f, -ancho }, afuera);

        VerticeCue({ x, 0.01f, 0.0f }, centro);
        VerticeCue({ x, 0.01f, ancho }, afuera);
        VerticeCue({ x1, 0.01f, ancho }, afuera);
        VerticeCue({ x1, 0.01f, 0.0f }, centro);
    }

    rlEnd();
}


//==================================================
// VISUAL: FARO, CASA DEL FARERO Y UTILERIA
//==================================================

static Vector3 LamparaFaroCue()
{
    return { FARO_CUE.x, ALTO_TORRE_FARO_CUE + 0.45f + 0.85f, FARO_CUE.z };
}


static void DibujarFaroCue()
{
    Vector3 b = FARO_CUE;
    float yTorre = 0.45f;

    // Zocalo octogonal con escalones.
    PrismaCue(b, 1.7f, 1.6f, yTorre, 8, TinteCue(PIEDRA_CUE, b));
    CuboCue({ b.x, 0.12f, b.z + 1.75f }, { 1.0f, 0.24f, 0.5f }, TinteCue(PIEDRA_CUE, b));

    // Torre a franjas: cal y azul pizarra.
    const int FRANJAS = 6;
    float altoFranja = ALTO_TORRE_FARO_CUE / (float)FRANJAS;

    for (int k = 0; k < FRANJAS; k++)
    {
        float y0 = yTorre + altoFranja * (float)k;
        float r0 = 1.1f - 0.045f * (y0 - yTorre);
        float r1 = 1.1f - 0.045f * (y0 + altoFranja - yTorre);
        Color c = k % 2 == 0 ? CAL_CUE : FRANJA_FARO_CUE;
        PrismaCue({ b.x, y0, b.z }, r0, r1, altoFranja, 14, TinteCue(c, { b.x, y0, b.z }), false);
    }

    // Puerta y ventanitas hacia el frente.
    CuboCue({ b.x, yTorre + 0.6f, b.z + 1.06f }, { 0.55f, 1.2f, 0.12f }, MADERA_OSCURA_CUE);
    CuboCue({ b.x, yTorre + 1.26f, b.z + 1.07f }, { 0.7f, 0.12f, 0.14f }, TinteCue(PIEDRA_CUE, b));

    for (int k = 0; k < 2; k++)
    {
        float y = yTorre + 2.9f + 2.1f * (float)k;
        float r = 1.1f - 0.045f * (y - yTorre);
        CuboCue({ b.x, y, b.z + r - 0.02f }, { 0.28f, 0.42f, 0.1f }, Color{ 60, 58, 74, 255 });
    }

    // Galeria con baranda.
    float yGaleria = yTorre + ALTO_TORRE_FARO_CUE;
    PrismaCue({ b.x, yGaleria - 0.18f, b.z }, 0.85f, 1.25f, 0.18f, 14, HIERRO_CUE);
    PrismaCue({ b.x, yGaleria, b.z }, 1.25f, 1.25f, 0.08f, 14, TonoCue(HIERRO_CUE, 1.3f));

    for (int k = 0; k < 14; k++)
    {
        float a = 6.2832f * (float)k / 14.0f;
        Vector3 p = { b.x + std::cos(a) * 1.18f, yGaleria + 0.08f, b.z + std::sin(a) * 1.18f };
        VigaCue(p, { p.x, p.y + 0.5f, p.z }, 0.04f, HIERRO_CUE);
    }

    AroCue({ b.x, yGaleria + 0.58f, b.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 1.2f, 0.05f, HIERRO_CUE, HIERRO_CUE, 20);

    // Linterna: base, montantes, lampara y cupula de cobre.
    PrismaCue({ b.x, yGaleria + 0.08f, b.z }, 0.72f, 0.72f, 0.26f, 10, FRANJA_FARO_CUE);
    float yVidrio = yGaleria + 0.34f;

    for (int k = 0; k < 8; k++)
    {
        float a = 6.2832f * (float)k / 8.0f;
        Vector3 p = { b.x + std::cos(a) * 0.62f, yVidrio, b.z + std::sin(a) * 0.62f };
        VigaCue(p, { p.x, yVidrio + 0.95f, p.z }, 0.06f, HIERRO_CUE);
    }

    EsferaCue(LamparaFaroCue(), 0.3f, Color{ 255, 240, 186, 255 }, 10, 3);
    PrismaCue({ b.x, yVidrio + 0.95f, b.z }, 0.82f, 0.08f, 0.75f, 10, COBRE_CUE);
    EsferaCue({ b.x, yVidrio + 1.78f, b.z }, 0.1f, COBRE_CUE, 8, 2);
    VigaCue({ b.x, yVidrio + 1.8f, b.z }, { b.x, yVidrio + 2.35f, b.z }, 0.03f, HIERRO_CUE);
}


// Devuelve el centro de la ventana encendida (para su luz).
static Vector3 DibujarCasaFareroCue()
{
    Vector3 c = CASA_CUE;
    Color cal = TinteCue(CAL_CUE, c);
    Color teja = TinteCue(TEJA_CUE, c);

    CuboCue({ c.x, 0.15f, c.z }, { 3.9f, 0.3f, 3.0f }, TinteCue(PIEDRA_CUE, c));
    CuboCue({ c.x, 1.15f, c.z }, { 3.6f, 1.7f, 2.8f }, cal);

    // Techo a dos aguas con hileras de tejas.
    float yAlero = 2.0f;
    float yCumbre = 3.25f;
    float mx = 2.0f;
    float mz = 1.65f;
    Color frenteTeja = TonoCue(teja, 0.92f);
    Color fondoTeja = TonoCue(teja, 0.6f);

    rlBegin(RL_QUADS);
    QuadCue({ c.x - mx, yAlero, c.z + mz }, { c.x + mx, yAlero, c.z + mz }, { c.x + mx, yCumbre, c.z }, { c.x - mx, yCumbre, c.z }, frenteTeja);
    QuadCue({ c.x - mx, yAlero, c.z - mz }, { c.x - mx, yCumbre, c.z }, { c.x + mx, yCumbre, c.z }, { c.x + mx, yAlero, c.z - mz }, fondoTeja);

    for (int k = 1; k < 5; k++)
    {
        float u = (float)k / 5.0f;
        float y = yAlero + (yCumbre - yAlero) * u + 0.02f;
        float z = c.z + mz * (1.0f - u) + 0.02f;
        QuadCue({ c.x - mx, y, z }, { c.x + mx, y, z }, { c.x + mx, y + 0.05f, z - 0.07f }, { c.x - mx, y + 0.05f, z - 0.07f }, TonoCue(teja, 0.7f));
    }

    rlEnd();

    rlBegin(RL_TRIANGLES);

    for (int s = -1; s <= 1; s += 2)
    {
        float x = c.x + (float)s * 1.8f;
        Color hastial = TonoCue(cal, s < 0 ? 0.74f : 0.62f);
        VerticeCue({ x, yAlero, c.z - 1.4f }, hastial);
        VerticeCue({ x, yAlero, c.z + 1.4f }, hastial);
        VerticeCue({ x, yCumbre - 0.08f, c.z }, hastial);
    }

    rlEnd();

    // Chimenea, puerta, ventanas con postigos y maceta.
    CuboCue({ c.x + 1.0f, 3.1f, c.z - 0.45f }, { 0.46f, 1.3f, 0.46f }, TinteCue(PIEDRA_CUE, c));
    CuboCue({ c.x + 1.0f, 3.78f, c.z - 0.45f }, { 0.56f, 0.1f, 0.56f }, TinteCue(TonoCue(PIEDRA_CUE, 0.8f), c));
    CuboCue({ c.x - 0.9f, 0.95f, c.z + 1.42f }, { 0.66f, 1.3f, 0.08f }, MADERA_OSCURA_CUE);

    Vector3 ventana = { c.x + 0.7f, 1.3f, c.z + 1.42f };
    CuboCue(ventana, { 0.74f, 0.62f, 0.06f }, Color{ 255, 206, 132, 255 });
    CuboCue({ ventana.x - 0.52f, ventana.y, ventana.z + 0.02f }, { 0.26f, 0.68f, 0.06f }, TinteCue(COBRE_CUE, c));
    CuboCue({ ventana.x + 0.52f, ventana.y, ventana.z + 0.02f }, { 0.26f, 0.68f, 0.06f }, TinteCue(COBRE_CUE, c));
    CuboCue({ ventana.x, ventana.y - 0.4f, ventana.z + 0.12f }, { 0.86f, 0.14f, 0.22f }, TinteCue(TEJA_CUE, c));
    DomoCue({ ventana.x, ventana.y - 0.33f, ventana.z + 0.12f }, 0.4f, 0.16f, 0.1f, TinteCue(Color{ 92, 140, 70, 255 }, c), 8, 2);

    // Barriles y cajones del farero.
    Vector3 barriles[3] = { { c.x - 2.6f, 0.0f, c.z + 1.6f }, { c.x - 3.2f, 0.0f, c.z + 1.0f }, { c.x - 2.8f, 0.0f, c.z + 0.3f } };

    for (int k = 0; k < 3; k++)
    {
        Color madera = TinteCue(TonoCue(MADERA_CUE, 0.9f + 0.1f * (float)k), barriles[k]);
        PrismaCue(barriles[k], 0.34f, 0.38f, 0.45f, 10, madera, false);
        PrismaCue({ barriles[k].x, 0.45f, barriles[k].z }, 0.38f, 0.34f, 0.45f, 10, madera);
        PrismaCue({ barriles[k].x, 0.16f, barriles[k].z }, 0.37f, 0.37f, 0.06f, 10, HIERRO_CUE, false);
        PrismaCue({ barriles[k].x, 0.72f, barriles[k].z }, 0.37f, 0.37f, 0.06f, 10, HIERRO_CUE, false);
    }

    CuboGiradoCue({ c.x + 2.6f, 0.3f, c.z + 1.5f }, { 0.65f, 0.6f, 0.65f }, 12.0f, TinteCue(MADERA_CUE, c));
    CuboGiradoCue({ c.x + 2.65f, 0.85f, c.z + 1.45f }, { 0.5f, 0.5f, 0.5f }, -8.0f, TinteCue(TonoCue(MADERA_CUE, 1.1f), c));

    return ventana;
}


static void DibujarBancoCue(Vector3 p, float giro)
{
    Color madera = TinteCue(MADERA_CUE, p);
    float a = giro * DEG2RAD;
    Vector3 fr = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 de = { std::cos(a), 0.0f, -std::sin(a) };

    for (int k = 0; k < 3; k++)
    {
        float d = -0.16f + 0.16f * (float)k;
        CuboGiradoCue({ p.x + fr.x * d, 0.48f, p.z + fr.z * d }, { 1.7f, 0.06f, 0.13f }, giro, TonoCue(madera, 1.0f - 0.05f * (float)k));
    }

    CuboGiradoCue({ p.x - fr.x * 0.28f, 0.82f, p.z - fr.z * 0.28f }, { 1.7f, 0.12f, 0.06f }, giro, madera);
    CuboGiradoCue({ p.x - fr.x * 0.28f, 1.0f, p.z - fr.z * 0.28f }, { 1.7f, 0.12f, 0.06f }, giro, madera);

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 pata = { p.x + de.x * 0.72f * (float)s, 0.0f, p.z + de.z * 0.72f * (float)s };
        VigaCue({ pata.x + fr.x * 0.2f, 0.0f, pata.z + fr.z * 0.2f }, { pata.x + fr.x * 0.2f, 0.48f, pata.z + fr.z * 0.2f }, 0.07f, HIERRO_CUE);
        VigaCue({ pata.x - fr.x * 0.28f, 0.0f, pata.z - fr.z * 0.28f }, { pata.x - fr.x * 0.28f, 1.06f, pata.z - fr.z * 0.28f }, 0.07f, HIERRO_CUE);
        VigaCue({ pata.x - fr.x * 0.28f, 0.66f, pata.z - fr.z * 0.28f }, { pata.x + fr.x * 0.22f, 0.66f, pata.z + fr.z * 0.22f }, 0.06f, HIERRO_CUE);
    }
}


// Catalejo de monedas que mira al canal.
static void DibujarCatalejoCue(Vector3 p, float t)
{
    Color pintura = TinteCue(Color{ 70, 132, 140, 255 }, p);
    float giro = std::sin(t * 0.3f) * 14.0f;

    TuboCue(p, { p.x, 1.0f, p.z }, 0.09f, 0.07f, HIERRO_CUE, 6);
    PrismaCue(p, 0.26f, 0.2f, 0.1f, 8, HIERRO_CUE);
    CuboGiradoCue({ p.x, 1.08f, p.z }, { 0.3f, 0.2f, 0.26f }, giro, pintura);
    CuboGiradoCue({ p.x, 1.24f, p.z }, { 0.52f, 0.24f, 0.38f }, giro, pintura);

    float a = giro * DEG2RAD;
    Vector3 de = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 fr = { std::sin(a), 0.0f, std::cos(a) };

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 o = { p.x + fr.x * 0.1f * (float)s, 1.26f, p.z + fr.z * 0.1f * (float)s };
        TuboCue({ o.x + de.x * 0.24f, o.y, o.z + de.z * 0.24f }, { o.x + de.x * 0.48f, o.y + 0.04f, o.z + de.z * 0.48f }, 0.07f, 0.09f, pintura, 6);
        TuboCue({ o.x - de.x * 0.24f, o.y, o.z - de.z * 0.24f }, { o.x - de.x * 0.36f, o.y, o.z - de.z * 0.36f }, 0.05f, 0.05f, HIERRO_CUE, 6);
    }
}


static void DibujarCartelPeligroCue(Vector3 p)
{
    Color madera = TinteCue(MADERA_OSCURA_CUE, p);
    VigaCue(p, { p.x, 1.5f, p.z }, 0.1f, madera);
    CuboCue({ p.x, 1.25f, p.z + 0.06f }, { 0.96f, 0.6f, 0.06f }, TinteCue(CAL_CUE, p));
    CuboCue({ p.x, 1.25f, p.z + 0.04f }, { 1.04f, 0.68f, 0.04f }, madera);

    // Triangulo de aviso con signo de exclamacion.
    rlBegin(RL_TRIANGLES);
    Color aviso = Color{ 214, 88, 60, 255 };
    VerticeCue({ p.x - 0.22f, 1.03f, p.z + 0.1f }, aviso);
    VerticeCue({ p.x + 0.22f, 1.03f, p.z + 0.1f }, aviso);
    VerticeCue({ p.x, 1.47f, p.z + 0.1f }, aviso);
    rlEnd();
    CuboCue({ p.x, 1.23f, p.z + 0.11f }, { 0.05f, 0.18f, 0.02f }, CAL_CUE);
    CuboCue({ p.x, 1.1f, p.z + 0.11f }, { 0.05f, 0.05f, 0.02f }, CAL_CUE);
}


// Farola de hierro; devuelve el centro del farol (para su luz).
static Vector3 DibujarFarolaCue(Vector3 p, float lado)
{
    TuboCue(p, { p.x, 0.25f, p.z }, 0.14f, 0.1f, HIERRO_CUE, 6);
    TuboCue({ p.x, 0.25f, p.z }, { p.x, 2.35f, p.z }, 0.06f, 0.05f, HIERRO_CUE, 6);
    VigaCue({ p.x, 2.3f, p.z }, { p.x + lado * 0.42f, 2.42f, p.z }, 0.05f, HIERRO_CUE);

    Vector3 farol = { p.x + lado * 0.42f, 2.08f, p.z };
    CuboCue(farol, { 0.24f, 0.3f, 0.24f }, LUZ_CALIDA_CUE);
    PrismaCue({ farol.x, farol.y + 0.15f, farol.z }, 0.2f, 0.02f, 0.18f, 4, HIERRO_CUE);
    CuboCue({ farol.x, farol.y - 0.17f, farol.z }, { 0.28f, 0.05f, 0.28f }, HIERRO_CUE);
    return farol;
}


static void DibujarSalvavidasCue(Vector3 p)
{
    CuboCue({ p.x, 0.75f, p.z }, { 0.14f, 1.5f, 0.14f }, TinteCue(MADERA_CUE, p));
    CuboCue({ p.x, 1.42f, p.z + 0.08f }, { 0.12f, 0.08f, 0.2f }, HIERRO_CUE);
    AroCue({ p.x, 1.02f, p.z + 0.16f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.34f, 0.12f, Color{ 238, 124, 64, 255 }, CAL_CUE, 20);
}


// Estaca con el rollo de soga sobrante de cada bando.
static Vector3 DibujarEstacaCue(Vector3 p)
{
    Color madera = TinteCue(MADERA_OSCURA_CUE, p);
    TuboCue(p, { p.x, 0.7f, p.z }, 0.09f, 0.08f, madera, 6);
    PrismaCue({ p.x, 0.7f, p.z }, 0.08f, 0.02f, 0.08f, 6, madera);

    for (int k = 0; k < 3; k++)
    {
        float r = 0.36f - 0.06f * (float)k;
        AroCue({ p.x + 0.42f, 0.05f + 0.07f * (float)k, p.z + 0.1f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, r, 0.07f, TonoCue(CUERDA_CUE, 0.9f), TonoCue(CUERDA_CUE, 0.78f), 16);
    }

    // Vueltas de soga en la estaca.
    for (int k = 0; k < 3; k++)
    {
        PrismaCue({ p.x, 0.3f + 0.08f * (float)k, p.z }, 0.12f, 0.12f, 0.05f, 8, CUERDA_CUE, false);
    }

    return { p.x, 0.36f, p.z };
}


static void DibujarRocaCue(Vector3 p, float r, int semilla)
{
    Color c = TinteCue(TonoCue(PIEDRA_CUE, 0.86f + 0.2f * Hash01Cue(semilla, 1)), p);
    float rx = r * (0.9f + 0.4f * Hash01Cue(semilla, 2));
    float rz = r * (0.8f + 0.4f * Hash01Cue(semilla, 3));
    DomoCue({ p.x, p.y - r * 0.15f, p.z }, rx, r * 0.8f, rz, c, 8, 3);
    DomoCue({ p.x + rx * 0.7f, p.y - r * 0.1f, p.z + rz * 0.4f }, rx * 0.45f, r * 0.45f, rz * 0.45f, TonoCue(c, 0.92f), 7, 2);
    DomoCue({ p.x - rx * 0.3f, p.y + r * 0.5f, p.z - rz * 0.2f }, rx * 0.35f, r * 0.12f, rz * 0.3f, TinteCue(Color{ 150, 160, 110, 255 }, p), 6, 1);
}


// Mata de tojo con flores amarillas.
static void DibujarMataCue(Vector3 p, float r, int semilla)
{
    DomoCue(p, r, r * 0.75f, r * 0.85f, TinteCue(Color{ 66, 104, 58, 255 }, p), 9, 3);
    DomoCue({ p.x + r * 0.5f, p.y, p.z + r * 0.3f }, r * 0.6f, r * 0.55f, r * 0.6f, TinteCue(Color{ 78, 118, 62, 255 }, p), 8, 3);

    for (int k = 0; k < 9; k++)
    {
        float a = 6.2832f * Hash01Cue(semilla, k);
        float h = 0.25f + 0.6f * Hash01Cue(semilla, k + 20);
        float rr = r * (0.9f - 0.5f * h);
        Vector3 f = { p.x + std::cos(a) * rr, p.y + r * 0.75f * h + 0.04f, p.z + std::sin(a) * rr * 0.9f };
        CuboCue(f, { 0.08f, 0.08f, 0.08f }, Color{ 250, 214, 92, 255 });
    }
}


static void DibujarMojonPiedrasCue(Vector3 p)
{
    for (int k = 0; k < 5; k++)
    {
        float r = 0.42f - 0.07f * (float)k;
        float y = 0.08f + 0.16f * (float)k;
        DomoCue({ p.x + 0.04f * (float)(k % 2), y, p.z }, r, 0.09f, r * 0.85f, TinteCue(TonoCue(PIEDRA_CUE, 0.9f + 0.05f * (float)(k % 3)), p), 8, 2);
        DomoCue({ p.x + 0.04f * (float)(k % 2), y, p.z }, r, -0.08f, r * 0.85f, TinteCue(TonoCue(PIEDRA_CUE, 0.7f), p), 8, 1);
    }
}


// Pino torcido por el viento del mar (se inclina hacia +x).
static void DibujarPinoCue(Vector3 p, float escala, float t, int semilla)
{
    Color tronco = TinteCue(Color{ 96, 70, 56, 255 }, p);
    Color copa = TinteCue(Color{ 50, 84, 62, 255 }, p);
    float vaiven = std::sin(t * 0.9f + (float)semilla) * 0.06f;
    Vector3 a = { p.x + 0.3f * escala, 1.6f * escala, p.z };
    Vector3 b = { p.x + 0.9f * escala + vaiven, 3.0f * escala, p.z - 0.1f };

    TuboCue(p, a, 0.2f * escala, 0.15f * escala, tronco, 6, false);
    TuboCue(a, b, 0.15f * escala, 0.08f * escala, tronco, 6);

    for (int k = 0; k < 4; k++)
    {
        float u = (float)k / 3.0f;
        Vector3 c = { a.x + (b.x - a.x) * u + 0.5f * escala * u, a.y + (b.y - a.y) * u + 0.2f * escala, a.z + 0.25f * (Hash01Cue(semilla, k) - 0.5f) };
        float r = (1.25f - 0.25f * u) * escala;
        DomoCue(c, r, 0.42f * escala, r * 0.8f, TonoCue(copa, 0.9f + 0.15f * u), 10, 3);
        DomoCue(c, r * 0.95f, -0.2f * escala, r * 0.75f, TonoCue(copa, 0.62f), 10, 1);
    }
}


static void DibujarTorreRuinasCue(Vector3 p)
{
    Color piedra = TinteCue(PIEDRA_CUE, p);
    PrismaCue(p, 1.7f, 1.5f, 4.2f, 9, piedra, false);
    PrismaCue({ p.x, 4.2f, p.z }, 1.5f, 1.5f, 0.05f, 9, TonoCue(piedra, 0.6f));

    for (int k = 0; k < 9; k++)
    {
        if (k == 3 || k == 6)
        {
            continue;
        }

        float a = 6.2832f * (float)k / 9.0f;
        float alto = 0.3f + 0.6f * Hash01Cue(k, 77);
        CuboGiradoCue({ p.x + std::cos(a) * 1.35f, 4.2f + alto * 0.5f, p.z + std::sin(a) * 1.35f }, { 0.55f, alto, 0.35f }, -a * RAD2DEG + 90.0f, piedra);
    }

    CuboCue({ p.x, 2.6f, p.z + 1.52f }, { 0.22f, 0.8f, 0.1f }, Color{ 50, 42, 50, 255 });
    CuboCue({ p.x, 0.7f, p.z + 1.6f }, { 0.8f, 1.4f, 0.1f }, Color{ 50, 42, 50, 255 });

    // Piedras caidas.
    for (int k = 0; k < 4; k++)
    {
        CuboGiradoCue({ p.x + 2.0f + 0.6f * (float)k, 0.15f, p.z + 0.8f - 0.5f * (float)k }, { 0.5f, 0.3f, 0.35f }, 30.0f * (float)k, TonoCue(piedra, 0.9f));
    }
}


// Cerca de postes con soga al borde del frente.
static void DibujarCercaCue(float xDesde, float xHasta, float z)
{
    float paso = 1.5f;
    int postes = (int)((xHasta - xDesde) / paso) + 1;

    for (int k = 0; k < postes; k++)
    {
        float x = xDesde + paso * (float)k;
        float zk = z + 0.15f * (Hash01Cue(k, (int)xDesde) - 0.5f);
        float alto = 0.75f + 0.12f * Hash01Cue(k, 33);
        Color madera = TinteCue(MADERA_OSCURA_CUE, { x, 0.0f, zk });
        CuboGiradoCue({ x, alto * 0.5f, zk }, { 0.13f, alto, 0.13f }, 8.0f * (Hash01Cue(k, 34) - 0.5f), madera);

        if (k < postes - 1)
        {
            float x1 = x + paso;
            Vector3 a = { x, alto - 0.12f, zk };
            Vector3 m = { x + paso * 0.5f, alto - 0.3f, zk };
            Vector3 b = { x1, alto - 0.12f, z };
            TuboCue(a, m, 0.025f, 0.025f, TonoCue(CUERDA_CUE, 0.85f), 4, false);
            TuboCue(m, b, 0.025f, 0.025f, TonoCue(CUERDA_CUE, 0.85f), 4, false);
        }
    }
}


// Lomas de pasto al fondo de cada promontorio.
static void DibujarLomasCue()
{
    const Vector4 lomas[] =
    {
        { -13.0f, -15.5f, 6.0f, 1.4f }, { -21.0f, -13.0f, 7.0f, 2.2f }, { -8.5f, -12.5f, 3.0f, 0.8f },
        { 26.0f, -16.0f, 6.0f, 1.8f }, { 28.0f, -8.0f, 5.0f, 2.4f }
    };

    for (int k = 0; k < (int)(sizeof(lomas) / sizeof(lomas[0])); k++)
    {
        Vector3 c = { lomas[k].x, -0.05f, lomas[k].y };
        DomoCue(c, lomas[k].z, lomas[k].w, lomas[k].z * 0.55f, ColorPastoCue(c.x, c.z), 14, 4);
    }
}


// Flores y matas de pasto alto: fuera de las franjas de juego.
static bool LugarLibreCue(float x, float z)
{
    if (std::fabs(z) < 1.3f && x > -7.6f && x < -3.0f) return false;
    if (std::fabs(z) < 1.3f && x > 3.0f && x < 10.6f) return false;
    return true;
}


static void DibujarFloresCue(float t, int paso)
{
    const Color colores[4] =
    {
        Color{ 248, 246, 236, 255 }, Color{ 240, 156, 196, 255 },
        Color{ 196, 156, 236, 255 }, Color{ 252, 232, 150, 255 }
    };

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 220; k += paso)
    {
        int lado = Hash01Cue(k, 300) < 0.5f ? 0 : 1;
        float s = lado == 0 ? -1.0f : 1.0f;
        float x = s * (3.9f + 13.0f * Hash01Cue(k, 301));
        float z = -9.0f + 12.4f * Hash01Cue(k, 302);

        if (!LugarLibreCue(x, z) || !DentroDeAcantiladoCue(lado, x, z + 0.4f))
        {
            continue;
        }

        float vaiven = std::sin(t * 2.0f + x * 0.8f + z) * 0.04f;
        float alto = 0.18f + 0.16f * Hash01Cue(k, 303);
        Color tallo = TinteCue(PASTO_OSCURO_CUE, { x, 0.0f, z });
        VerticeCue({ x - 0.02f, 0.0f, z }, tallo);
        VerticeCue({ x + 0.02f, 0.0f, z }, tallo);
        VerticeCue({ x + vaiven, alto, z }, tallo);

        Color flor = TinteCue(colores[(int)(Hash01Cue(k, 304) * 3.99f)], { x, 0.0f, z });
        Vector3 c = { x + vaiven, alto, z };
        for (int p = 0; p < 4; p++)
        {
            float a0 = 1.5708f * (float)p;
            float a1 = a0 + 1.5708f;
            VerticeCue(c, TonoCue(flor, 1.05f));
            VerticeCue({ c.x + std::cos(a0) * 0.08f, c.y + 0.02f, c.z + std::sin(a0) * 0.08f }, flor);
            VerticeCue({ c.x + std::cos(a1) * 0.08f, c.y + 0.02f, c.z + std::sin(a1) * 0.08f }, TonoCue(flor, 0.9f));
        }
    }

    // Pasto alto en matas que se mece con el viento.
    for (int k = 0; k < 160; k += paso)
    {
        int lado = Hash01Cue(k, 300) < 0.5f ? 0 : 1;
        float s = lado == 0 ? -1.0f : 1.0f;
        float x = s * (3.8f + 16.0f * Hash01Cue(k, 311));
        float z = -12.0f + 15.2f * Hash01Cue(k, 312);

        if (!LugarLibreCue(x, z) || !DentroDeAcantiladoCue(lado, x, z + 0.3f))
        {
            continue;
        }

        Color base = TinteCue(PASTO_OSCURO_CUE, { x, 0.0f, z });
        Color punta = TinteCue(Color{ 186, 190, 104, 255 }, { x, 0.0f, z });

        for (int h = 0; h < 5; h++)
        {
            float dx = 0.14f * ((float)h - 2.0f);
            float alto = 0.35f + 0.25f * Hash01Cue(k, h + 320);
            float vaiven = std::sin(t * 1.8f + x * 0.6f + (float)h * 0.4f) * 0.1f + 0.08f;
            VerticeCue({ x + dx - 0.035f, 0.0f, z }, base);
            VerticeCue({ x + dx + 0.035f, 0.0f, z }, base);
            VerticeCue({ x + dx * 1.6f + vaiven, alto, z + 0.02f }, punta);
        }
    }

    rlEnd();
}


//==================================================
// VISUAL: GAVIOTAS
//==================================================

static void DibujarGaviotaCue(Vector3 p, Vector3 frente, float aleteo, float escala)
{
    Vector3 lado = Vector3Normalize(Vector3CrossProduct({ 0.0f, 1.0f, 0.0f }, frente));
    Color blanco = TinteCue(Color{ 250, 246, 240, 255 }, p);
    Color gris = TinteCue(Color{ 150, 150, 160, 255 }, p);
    Color punta = TinteCue(Color{ 50, 50, 58, 255 }, p);
    float e = escala;

    CajaEjesCue(p, Vector3Scale(frente, 0.2f * e), { 0.0f, 0.06f * e, 0.0f }, Vector3Scale(lado, 0.07f * e), blanco);
    EsferaCue(Vector3Add(p, Vector3Scale(frente, 0.2f * e)), 0.065f * e, blanco, 6, 2);

    rlBegin(RL_QUADS);

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 l = Vector3Scale(lado, (float)s);
        Vector3 hombro = Vector3Add(p, Vector3Scale(l, 0.05f * e));
        Vector3 codo = Vector3Add(p, Vector3Add(Vector3Scale(l, 0.32f * e), { 0.0f, aleteo * 0.14f * e + 0.04f * e, 0.0f }));
        Vector3 ala = Vector3Add(p, Vector3Add(Vector3Scale(l, 0.62f * e), { 0.0f, aleteo * 0.32f * e - 0.02f * e, 0.0f }));
        Vector3 f = Vector3Scale(frente, 0.1f * e);
        Vector3 b = Vector3Scale(frente, -0.08f * e);

        QuadCue(Vector3Add(hombro, f), Vector3Add(codo, f), Vector3Add(codo, b), Vector3Add(hombro, b), gris);
        QuadCue(Vector3Add(codo, f), Vector3Add(ala, Vector3Scale(frente, 0.02f * e)), Vector3Add(ala, Vector3Scale(frente, -0.06f * e)), Vector3Add(codo, b), punta);
    }

    rlEnd();
}


static void DibujarGaviotasCue(float t, int paso)
{
    const Vector4 vuelos[7] =
    {
        { 0.5f, 5.2f, -7.0f, 4.5f }, { -6.0f, 6.4f, -14.0f, 6.0f }, { 7.0f, 7.6f, -19.0f, 5.0f },
        { -12.0f, 4.8f, -24.0f, 4.0f }, { 14.0f, 9.0f, -30.0f, 7.0f }, { -2.0f, 10.5f, -40.0f, 8.0f },
        { 3.0f, -3.5f, -10.0f, 3.0f }
    };

    for (int k = 0; k < 7; k += paso)
    {
        float velocidad = (0.35f + 0.08f * (float)k) * (k % 2 == 0 ? 1.0f : -1.0f);
        float a = t * velocidad + 1.7f * (float)k;
        Vector3 p =
        {
            vuelos[k].x + std::cos(a) * vuelos[k].w,
            vuelos[k].y + std::sin(a * 2.0f) * 0.35f,
            vuelos[k].z + std::sin(a) * vuelos[k].w * 0.55f
        };
        Vector3 frente = Vector3Normalize({ -std::sin(a) * velocidad, 0.0f, std::cos(a) * velocidad * 0.55f });
        float ciclo = std::fmod(t * 0.7f + (float)k * 0.37f, 3.0f);
        float aleteo = ciclo < 1.2f ? std::sin(t * 10.0f + (float)k) : 0.25f;
        DibujarGaviotaCue(p, frente, aleteo, 1.4f);
    }

    // Posadas: en la baranda del faro y en un poste de la cerca.
    float cabeceo = std::sin(t * 3.0f) > 0.7f ? 0.05f : 0.0f;
    Vector3 posadas[2] =
    {
        { FARO_CUE.x + 0.85f, ALTO_TORRE_FARO_CUE + 0.45f + 0.68f, FARO_CUE.z + 0.85f },
        { -11.5f, 0.86f, 3.0f }
    };

    for (int k = 0; k < 2; k++)
    {
        Vector3 p = posadas[k];
        Color blanco = TinteCue(Color{ 250, 246, 240, 255 }, p);
        CuboCue({ p.x, p.y + 0.1f, p.z }, { 0.32f, 0.16f, 0.14f }, blanco);
        CuboCue({ p.x - 0.02f, p.y + 0.14f, p.z }, { 0.24f, 0.08f, 0.16f }, TinteCue(Color{ 150, 150, 160, 255 }, p));
        EsferaCue({ p.x + 0.16f, p.y + 0.24f + cabeceo, p.z }, 0.07f, blanco, 6, 2);
        CuboCue({ p.x + 0.26f, p.y + 0.23f + cabeceo, p.z }, { 0.09f, 0.03f, 0.03f }, Color{ 236, 190, 70, 255 });
        CuboCue({ p.x - 0.2f, p.y + 0.12f, p.z }, { 0.12f, 0.04f, 0.1f }, TinteCue(Color{ 50, 50, 58, 255 }, p));
    }
}


//==================================================
// VISUAL: ELEMENTOS DE JUEGO (mojones, cuerda y jugadores)
//==================================================

// Banderin que flamea desde un mastil; colores en franjas horizontales.
static void DibujarBanderinCue(Vector3 asta, float sentido, const Color colores[], int cantidad, float t)
{
    const int SEGMENTOS = 6;
    float largo = 0.8f;
    float alto = 0.5f;

    rlBegin(RL_QUADS);

    for (int f = 0; f < cantidad; f++)
    {
        float y0 = asta.y - alto * (float)f / (float)cantidad;
        float y1 = asta.y - alto * (float)(f + 1) / (float)cantidad;

        for (int s = 0; s < SEGMENTOS; s++)
        {
            float u0 = (float)s / (float)SEGMENTOS;
            float u1 = (float)(s + 1) / (float)SEGMENTOS;
            float o0 = std::sin(t * 6.0f - u0 * 4.0f) * 0.1f * u0;
            float o1 = std::sin(t * 6.0f - u1 * 4.0f) * 0.1f * u1;
            float caida0 = 0.06f * u0;
            float caida1 = 0.06f * u1;
            Color c = TonoCue(colores[f], 0.9f + 0.12f * std::cos(t * 6.0f - u0 * 4.0f));

            QuadCue(
                { asta.x + sentido * largo * u0, y0 - caida0, asta.z + o0 },
                { asta.x + sentido * largo * u1, y0 - caida1, asta.z + o1 },
                { asta.x + sentido * largo * u1, y1 - caida1, asta.z + o1 },
                { asta.x + sentido * largo * u0, y1 - caida0, asta.z + o0 },
                c
            );
        }
    }

    rlEnd();
}


// Mojones de la linea de victoria de cada bando (x = +-LINEA_VICTORIA_X)
// sobre un espolon de roca, detras de la cuerda.
static void DibujarMojonesCue(const MinijuegoCuerdaAcantilado& m, const Participante participantes[], int limite, float t)
{
    Color colorSolo = m.indiceSolo >= 0 ? participantes[m.indiceSolo].color : CAL_CUE;
    Color trio[MAX_PARTICIPANTES];
    int cantidadTrio = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo && i != m.indiceSolo)
        {
            trio[cantidadTrio++] = participantes[i].color;
        }
    }

    if (cantidadTrio == 0)
    {
        trio[cantidadTrio++] = CAL_CUE;
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        float x = s * LINEA_VICTORIA_X_CUERDA;
        Vector3 c = { x, 0.0f, Z_MOJON_CUE };

        // Espolon: tres bloques que se afinan hacia abajo.
        CuboCue({ s * 3.36f, -0.28f, Z_MOJON_CUE }, { 0.78f, 0.56f, 0.9f }, ROCA_PALIDA_CUE);
        CuboCue({ s * 3.46f, -0.9f, Z_MOJON_CUE - 0.05f }, { 0.55f, 0.7f, 0.7f }, ROCA_OCRE_CUE);
        CuboCue({ s * 3.56f, -1.55f, Z_MOJON_CUE - 0.1f }, { 0.32f, 0.6f, 0.5f }, ROCA_OXIDO_CUE);
        CuboCue({ s * 3.36f, 0.01f, Z_MOJON_CUE }, { 0.8f, 0.04f, 0.92f }, PASTO_CUE);

        // Mastil, base de piedras y banderin del bando.
        DomoCue({ x, 0.02f, c.z }, 0.22f, 0.14f, 0.22f, PIEDRA_CUE, 8, 2);
        TuboCue(c, { x, 2.15f, c.z }, 0.06f, 0.045f, MADERA_OSCURA_CUE, 6);
        EsferaCue({ x, 2.19f, c.z }, 0.08f, ORO_CUE, 8, 2);

        // El de cada bando flamea hacia afuera (no tapa el canal).
        if (lado < 0)
        {
            DibujarBanderinCue({ x, 2.08f, c.z }, -1.0f, &colorSolo, 1, t);
        }
        else
        {
            DibujarBanderinCue({ x, 2.08f, c.z }, 1.0f, trio, cantidadTrio, t + 0.7f);
        }
    }
}


struct EscenaJugadoresCue
{
    bool visible[MAX_PARTICIPANTES];
    Vector3 pies[MAX_PARTICIPANTES];
    Vector3 mano[MAX_PARTICIPANTES];
    Vector3 nudo{};
    bool hayCuerda = false;
};


static void PrepararEscenaJugadoresCue(const MinijuegoCuerdaAcantilado& m, int limite, EscenaJugadoresCue& e)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        e.visible[i] = false;
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 pies = PiesVisualesCuerda(m, i);
        const EstadoJugadorCuerdaAcantilado& estado = m.estadosJugadores[i];

        // Festejo del bando ganador: saltitos (solo dibujo).
        if (m.fase == FASE_CUERDA_TERMINADO && !EsPerdedorCue(m, i) && m.tiempoFin > 1.0f && !m.empate)
        {
            pies.y += std::fabs(std::sin((m.tiempoFin - 1.0f) * 7.0f + (float)i)) * 0.35f;
        }

        // Tropiezo: sacudida corta.
        if (estado.tiempoTropiezo > 0.0f)
        {
            pies.x += std::sin(m.tiempoAnimacion * 55.0f) * 0.05f * (estado.tiempoTropiezo / DURACION_TROPIEZO_CUERDA);
        }

        float haciaRival = estado.equipo == 0 ? 1.0f : -1.0f;
        e.visible[i] = pies.y > ALTURA_MAR_CUERDA - 0.7f;
        e.pies[i] = pies;
        e.mano[i] = { pies.x + haciaRival * 0.3f, pies.y + ALTURA_MANOS_CUERDA, 0.0f };
    }
}


static Vector3 PuntoCuerdaCue(Vector3 a, Vector3 b, float u, float comba, float vibracion, float t)
{
    return
    {
        a.x + (b.x - a.x) * u,
        a.y + (b.y - a.y) * u - comba * std::sin(u * PI) + vibracion * std::sin(t * 42.0f + u * 13.0f) * std::sin(u * PI),
        0.0f
    };
}


// Soga trenzada entre el solitario y el ultimo del trio, colas a las
// estacas y el panuelo que marca el centro (marcador * LINEA_VICTORIA_X).
static void DibujarCuerdaCue(const MinijuegoCuerdaAcantilado& m, int limite, EscenaJugadoresCue& e, Vector3 estacaSolo, Vector3 estacaTrio, float t)
{
    int extremoTrio = -1;
    float xMaximo = -1000.0f;
    float tension = 0.0f;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        tension = fmaxf(tension, m.estadosJugadores[i].retroceso);

        if (i != m.indiceSolo && e.pies[i].x > xMaximo)
        {
            xMaximo = e.pies[i].x;
            extremoTrio = i;
        }
    }

    if (m.indiceSolo < 0 || extremoTrio < 0)
    {
        e.hayCuerda = false;
        return;
    }

    Vector3 a = e.mano[m.indiceSolo];
    Vector3 b = e.mano[extremoTrio];
    float comba = 0.16f - 0.06f * tension;
    float vibracion = 0.03f * tension;
    const int SEGMENTOS = 30;
    Vector3 anterior = a;

    for (int k = 1; k <= SEGMENTOS; k++)
    {
        Vector3 p = PuntoCuerdaCue(a, b, (float)k / (float)SEGMENTOS, comba, vibracion, t);
        TuboCue(anterior, p, 0.05f, 0.05f, k % 2 == 0 ? CUERDA_CUE : TonoCue(CUERDA_CUE, 0.8f), 6, false);
        anterior = p;
    }

    // Colas hasta las estacas (si las manos siguen sobre el pasto).
    Vector3 colas[2][2] = { { a, estacaSolo }, { b, estacaTrio } };

    for (int c = 0; c < 2; c++)
    {
        Vector3 desde = colas[c][0];
        Vector3 hasta = colas[c][1];

        if (desde.y < 0.3f)
        {
            continue;
        }

        Vector3 previo = desde;

        for (int k = 1; k <= 10; k++)
        {
            float u = (float)k / 10.0f;
            Vector3 p =
            {
                desde.x + (hasta.x - desde.x) * u,
                desde.y + (hasta.y - desde.y) * u - 0.7f * std::sin(u * PI),
                hasta.z * u
            };

            if (p.y < 0.05f) p.y = 0.05f;
            TuboCue(previo, p, 0.045f, 0.045f, k % 2 == 0 ? CUERDA_CUE : TonoCue(CUERDA_CUE, 0.8f), 5, false);
            previo = p;
        }
    }

    // Nudo y panuelo en el centro de la soga.
    float xNudo = m.marcador * LINEA_VICTORIA_X_CUERDA;
    float u = (b.x - a.x) != 0.0f ? (xNudo - a.x) / (b.x - a.x) : 0.5f;
    u = Limitar01Cue(u);
    Vector3 nudo = PuntoCuerdaCue(a, b, u, comba, vibracion, t);
    nudo.x = xNudo;
    e.nudo = nudo;
    e.hayCuerda = true;

    EsferaCue(nudo, 0.12f, TonoCue(CUERDA_CUE, 0.7f), 8, 2);

    const int TIRAS = 5;
    float largo = 0.85f;

    rlBegin(RL_QUADS);

    for (int k = 0; k < TIRAS; k++)
    {
        float u0 = (float)k / (float)TIRAS;
        float u1 = (float)(k + 1) / (float)TIRAS;
        float o0 = std::sin(t * 5.0f - u0 * 3.0f) * 0.08f * u0;
        float o1 = std::sin(t * 5.0f - u1 * 3.0f) * 0.08f * u1;
        float w0 = 0.26f - 0.12f * u0;
        float w1 = 0.26f - 0.12f * u1;
        Color c = k == 2 ? ORO_CUE : PANUELO_CUE;
        Vector3 p0 = { nudo.x + o0, nudo.y - 0.06f - largo * u0, 0.06f };
        Vector3 p1 = { nudo.x + o1, nudo.y - 0.06f - largo * u1, 0.06f };

        QuadCue({ p0.x - w0, p0.y, p0.z }, { p0.x + w0, p0.y, p0.z }, { p1.x + w1, p1.y, p1.z }, { p1.x - w1, p1.y, p1.z }, c);
    }

    rlEnd();
}


static void DibujarJugadorInclinadoCue(Vector3 pies, bool haciaDerecha, float inclinacionGrados, Color color, float escala)
{
    // Positivo inclina hacia -X; el trio (a la derecha) se invierte.
    float angulo = haciaDerecha ? inclinacionGrados : -inclinacionGrados;
    float mirada = haciaDerecha ? 90.0f : -90.0f;

    rlPushMatrix();
    rlTranslatef(pies.x, pies.y, pies.z);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);
    DibujarModeloJugadorEnPosicion({ 0.0f, 0.0f, 0.0f }, mirada, color, ESCALA_MODELO_JUGADOR_3D * escala);
    rlPopMatrix();
}


static void DibujarJugadoresCue(
    const MinijuegoCuerdaAcantilado& m,
    const EscenaJugadoresCue& e,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    bool mostrarDebug
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo || !e.visible[i])
        {
            continue;
        }

        const EstadoJugadorCuerdaAcantilado& estado = m.estadosJugadores[i];
        Vector3 pies = e.pies[i];
        Color color = participantes[i].color;
        bool enPasto = pies.y > -0.05f;

        if (enPasto)
        {
            AnilloCue({ pies.x, 0.03f, pies.z }, 0.44f, 0.54f, AlfaCue(color, 210.0f), 22);
            DibujarSombraRetroCircular({ pies.x, pies.y + 0.05f, pies.z }, 0.46f, 0.34f);
        }

        // Retroceso al tirar; el tropiezo lo vence hacia adelante.
        float inclinacion = 10.0f + 16.0f * estado.retroceso;

        if (estado.tiempoTropiezo > 0.0f)
        {
            inclinacion = -20.0f;
        }

        if (m.fase == FASE_CUERDA_PREPARACION)
        {
            inclinacion = 4.0f;
        }
        else if (m.fase == FASE_CUERDA_TERMINADO && !m.empate)
        {
            if (EsPerdedorCue(m, i))
            {
                // Arrastrado por el pasto y despues en caida, girando.
                inclinacion = enPasto ? -28.0f : -30.0f - 260.0f * (m.tiempoFin - 0.6f);
            }
            else if (m.tiempoFin > 1.0f)
            {
                inclinacion = 0.0f;
            }
        }

        float escala = 1.0f + 0.07f * m.destelloTiron[i];
        DibujarJugadorInclinadoCue(pies, estado.equipo == 0, inclinacion, color, escala);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }
}


//==================================================
// VISUAL: PASADA TRASLUCIDA (espuma, bruma, humo, particulas)
//==================================================

static void DibujarEspumaCue(float t, bool animar)
{
    float y = ALTURA_MAR_CUERDA + 0.03f;

    rlBegin(RL_QUADS);

    for (int lado = 0; lado < 2; lado++)
    {
        const ContornoCue& c = contornosCue[lado];

        for (int i = 0; i < c.cantidad - 1; i++)
        {
            Vector3 p0 = PuntoEstratoCue(lado, CANTIDAD_ESTRATOS_CUE - 1, i, y);
            Vector3 p1 = PuntoEstratoCue(lado, CANTIDAD_ESTRATOS_CUE - 1, i + 1, y);
            Vector2 n0 = c.normal[i];
            Vector2 n1 = c.normal[i + 1];
            float w0 = animar ? 0.3f + 0.55f * (0.5f + 0.5f * std::sin(t * 1.5f + (float)i * 0.7f + (float)lado * 2.0f)) : 0.45f;
            float w1 = animar ? 0.3f + 0.55f * (0.5f + 0.5f * std::sin(t * 1.5f + (float)(i + 1) * 0.7f + (float)lado * 2.0f)) : 0.45f;
            Color dentro = AlfaCue(ESPUMA_CUE, 210.0f);
            Color fuera = AlfaCue(ESPUMA_CUE, 0.0f);

            VerticeCue(p0, dentro);
            VerticeCue(p1, dentro);
            VerticeCue({ p1.x + n1.x * w1, y, p1.z + n1.y * w1 }, fuera);
            VerticeCue({ p0.x + n0.x * w0, y, p0.z + n0.y * w0 }, fuera);

            // Segundo cordon de espuma que se aleja de la pared.
            if (animar)
            {
                float d0 = 0.7f + 0.9f * std::fmod(t * 0.35f + (float)i * 0.11f, 1.0f);
                float d1 = 0.7f + 0.9f * std::fmod(t * 0.35f + (float)(i + 1) * 0.11f, 1.0f);

                if (std::fabs(d1 - d0) < 0.5f)
                {
                    float alfa = 70.0f * (1.0f - (d0 - 0.7f) / 0.9f);
                    Color cordon = AlfaCue(ESPUMA_CUE, alfa);
                    VerticeCue({ p0.x + n0.x * d0, y, p0.z + n0.y * d0 }, cordon);
                    VerticeCue({ p1.x + n1.x * d1, y, p1.z + n1.y * d1 }, cordon);
                    VerticeCue({ p1.x + n1.x * (d1 + 0.18f), y, p1.z + n1.y * (d1 + 0.18f) }, fuera);
                    VerticeCue({ p0.x + n0.x * (d0 + 0.18f), y, p0.z + n0.y * (d0 + 0.18f) }, fuera);
                }
            }
        }
    }

    // Crestas de olas que llegan al frente y que suben por el canal.
    if (animar)
    {
        for (int k = 0; k < 5; k++)
        {
            float u = std::fmod(t * 0.08f + (float)k / 5.0f, 1.0f);
            float z = 16.0f - 10.5f * u;
            float alfa = std::sin(u * PI) * 120.0f;
            float grueso = 0.14f + 0.18f * u;

            for (int s = 0; s < 18; s++)
            {
                if (Hash01Cue(s, k * 7 + (int)(t * 0.08f + (float)k / 5.0f)) < 0.3f)
                {
                    continue;
                }

                float x0 = -18.0f + 2.2f * (float)s;
                float x1 = x0 + 1.9f;
                float z0 = z + 0.25f * std::sin(x0 * 0.3f + (float)k);
                float z1 = z + 0.25f * std::sin(x1 * 0.3f + (float)k);
                Color cresta = AlfaCue(ESPUMA_CUE, alfa);
                VerticeCue({ x0, y, z0 }, AlfaCue(cresta, 0.0f));
                VerticeCue({ x1, y, z1 }, AlfaCue(cresta, 0.0f));
                VerticeCue({ x1, y, z1 + grueso }, cresta);
                VerticeCue({ x0, y, z0 + grueso }, cresta);
            }
        }

        for (int k = 0; k < 4; k++)
        {
            float u = std::fmod(t * 0.06f + (float)k / 4.0f, 1.0f);
            float z = -22.0f + 24.0f * u;
            float alfa = std::sin(u * PI) * 55.0f;
            float medio = z > -2.4f ? 3.4f : 3.6f + 0.06f * (-2.4f - z);

            for (int s = 0; s < 8; s++)
            {
                float x0 = -medio + 2.0f * medio * (float)s / 8.0f;
                float x1 = -medio + 2.0f * medio * (float)(s + 1) / 8.0f;
                float z0 = z - 0.5f * (x0 * x0) / (medio * medio);
                float z1 = z - 0.5f * (x1 * x1) / (medio * medio);
                Color cresta = AlfaCue(ESPUMA_CUE, alfa);
                VerticeCue({ x0, y, z0 }, AlfaCue(cresta, 0.0f));
                VerticeCue({ x1, y, z1 }, AlfaCue(cresta, 0.0f));
                VerticeCue({ x1, y, z1 + 0.2f }, cresta);
                VerticeCue({ x0, y, z0 + 0.2f }, cresta);
            }
        }
    }

    // Bruma del horizonte: funde el mar con el cielo.
    Color bruma = AlfaCue(NIEBLA_CUE, 190.0f);
    VerticeCue({ -800.0f, ALTURA_MAR_CUERDA, Z_HORIZONTE_CUE + 4.0f }, bruma);
    VerticeCue({ 800.0f, ALTURA_MAR_CUERDA, Z_HORIZONTE_CUE + 4.0f }, bruma);
    VerticeCue({ 800.0f, ALTURA_MAR_CUERDA + 22.0f, Z_HORIZONTE_CUE + 4.0f }, AlfaCue(bruma, 0.0f));
    VerticeCue({ -800.0f, ALTURA_MAR_CUERDA + 22.0f, Z_HORIZONTE_CUE + 4.0f }, AlfaCue(bruma, 0.0f));

    rlEnd();

    // Espuma alrededor de los farallones.
    const Vector3 farallones[4] =
    {
        { -15.5f, 2.8f, -40.0f }, { -26.0f, 2.0f, -50.0f }, { 19.5f, 2.4f, -44.0f }, { 24.0f, 1.6f, -36.0f }
    };

    for (int k = 0; k < 4; k++)
    {
        float pulso = animar ? 0.5f + 0.5f * std::sin(t * 1.3f + (float)k * 1.9f) : 0.5f;
        AnilloCue({ farallones[k].x, y, farallones[k].z }, farallones[k].y * 0.98f, farallones[k].y * (1.15f + 0.2f * pulso), AlfaCue(ESPUMA_CUE, 150.0f), 14);
    }
}


static void DibujarHumoCue(float t)
{
    Vector3 boca = { CASA_CUE.x + 1.0f, 3.9f, CASA_CUE.z - 0.45f };

    for (int k = 0; k < 7; k++)
    {
        float u = std::fmod(t * 0.16f + (float)k / 7.0f, 1.0f);
        Vector3 p = { boca.x + u * 1.8f + std::sin(u * 6.0f + (float)k) * 0.2f, boca.y + u * 3.4f, boca.z - u * 0.6f };
        float r = 0.22f + 0.55f * u;
        EsferaCue(p, r, AlfaCue(Color{ 214, 190, 200, 255 }, 110.0f * (1.0f - u) * SuaveCue(u * 6.0f)), 8, 2);
    }
}


static void DibujarParticulasCue(const MinijuegoCuerdaAcantilado& m)
{
    // Confeti: papelitos que giran.
    rlBegin(RL_QUADS);

    for (int i = 0; i < MAX_PARTICULAS_VISUALES_CUERDA; i++)
    {
        const ParticulaVisualCuerda& p = m.particulasVisuales[i];

        if (!p.activa || p.brillo || !p.papel)
        {
            continue;
        }

        float alfa = (float)p.color.a * Limitar01Cue(p.vida / 0.4f);
        PlaquitaCue(p.posicion, p.tamano, p.giro, AlfaCue(p.color, alfa));
    }

    rlEnd();

    // Espuma, polvo y piedritas: manchas suaves.
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_CUERDA; i++)
    {
        const ParticulaVisualCuerda& p = m.particulasVisuales[i];

        if (!p.activa || p.brillo || p.papel)
        {
            continue;
        }

        float alfa = (float)p.color.a * Limitar01Cue(p.vida / 0.4f);
        ResplandorFrenteCue(p.posicion, p.tamano * 0.9f, AlfaCue(p.color, alfa));
    }
}


//==================================================
// VISUAL: LUCES FALSAS (pasada aditiva al final del 3D)
//==================================================

static void DibujarLucesCue(
    const MinijuegoCuerdaAcantilado& m,
    const EscenaJugadoresCue& e,
    const Participante participantes[],
    int limite,
    const Vector3 farolas[],
    int cantidadFarolas,
    bool hayCasa,
    Vector3 ventana,
    bool decoracion
)
{
    const float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Camino de destellos del sol sobre el mar, solo a lo lejos.
    rlBegin(RL_QUADS);

    for (int k = 0; k < 80; k++)
    {
        float u = (float)k / 80.0f;
        float z = -48.0f - 360.0f * std::pow(u, 1.4f);
        float f = (OJO_CUE.z - z) / (OJO_CUE.z - Z_HORIZONTE_CUE);
        float x = OJO_CUE.x + (SOL_X_CUE - OJO_CUE.x) * f + (Hash01Cue(k, 1) - 0.5f) * (1.2f + 0.035f * -z);
        float titileo = Hash01Cue(k, (int)(t * 5.0f) + k * 3);

        if (titileo < 0.3f)
        {
            continue;
        }

        float ancho = (0.5f + 0.012f * -z) * (0.6f + Hash01Cue(k, 2));
        float fondo = 0.2f + 0.012f * -z;
        Color brillo = Color{ 255, 214, 150, (unsigned char)(50.0f + 110.0f * titileo * (0.4f + 0.6f * u)) };
        float y = ALTURA_MAR_CUERDA + 0.02f;

        VerticeCue({ x - ancho, y, z - fondo }, brillo);
        VerticeCue({ x - ancho, y, z + fondo }, brillo);
        VerticeCue({ x + ancho, y, z + fondo }, brillo);
        VerticeCue({ x + ancho, y, z - fondo }, brillo);
    }

    rlEnd();

    if (decoracion)
    {
        // Faro: halo de la lampara y dos haces que giran.
        Vector3 lampara = LamparaFaroCue();
        float giro = t * 0.9f;
        ResplandorFrenteCue(lampara, 2.4f, Color{ 255, 206, 140, 70 });
        ResplandorFrenteCue(lampara, 0.75f, Color{ 255, 246, 210, 180 });

        float haciaCamara = std::atan2(OJO_CUE.z - lampara.z, OJO_CUE.x - lampara.x);

        for (int h = 0; h < 2; h++)
        {
            float a = giro + PI * (float)h;
            Vector3 dir = { std::cos(a), -0.04f, std::sin(a) };
            DrawCylinderEx(lampara, Vector3Add(lampara, Vector3Scale(dir, 20.0f)), 0.2f, 2.8f, 12, Color{ 255, 230, 170, 16 });
            DrawCylinderEx(lampara, Vector3Add(lampara, Vector3Scale(dir, 11.0f)), 0.1f, 1.0f, 10, Color{ 255, 240, 200, 22 });

            float d = std::cos(a - haciaCamara);

            if (d > 0.82f)
            {
                float fuerza = (d - 0.82f) / 0.18f;
                ResplandorFrenteCue(lampara, 1.5f + 3.0f * fuerza, Color{ 255, 236, 190, (unsigned char)(140.0f * fuerza) });
            }
        }

        // Farolas: halo y charco tibio en el pasto.
        for (int k = 0; k < cantidadFarolas; k++)
        {
            Vector3 f = farolas[k];
            float titileo = 0.92f + 0.08f * std::sin(t * 9.0f + (float)k * 1.7f) * std::sin(t * 4.3f + (float)k);
            ResplandorFrenteCue(f, 1.2f * titileo, Color{ 255, 190, 110, 76 });
            ResplandorFrenteCue(f, 0.38f * titileo, Color{ 255, 236, 190, 170 });

            for (int r = 0; r < 3; r++)
            {
                ResplandorSueloCue({ f.x, 0.03f + 0.002f * (float)r, f.z }, (1.0f + 0.8f * (float)r) * titileo, Color{ 255, 184, 110, 24 });
            }
        }

        // Ventana de la casa del farero.
        if (hayCasa)
        {
            ResplandorCue({ ventana.x, ventana.y, ventana.z + 0.06f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.9f, Color{ 255, 190, 110, 80 }, 1.3f);
            ResplandorSueloCue({ ventana.x, 0.03f, ventana.z + 1.0f }, 1.7f, Color{ 255, 184, 110, 30 }, 1.3f);
        }

        // Faro lejano que parpadea.
        if (std::fmod(t, 3.0f) < 0.45f)
        {
            Vector3 f = FaroLejanoCue();
            ResplandorFrenteCue({ f.x, f.y + 4.6f, f.z }, 3.0f, Color{ 255, 226, 170, 150 });
        }

        // Polen dorado que flota en la luz del atardecer.
        int motas = (int)(54.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < motas; k++)
        {
            float ciclo = 9.0f + 6.0f * Hash01Cue(k, 801);
            float u = std::fmod(t / ciclo + Hash01Cue(k, 802), 1.0f);
            Vector3 p =
            {
                -15.0f + 32.0f * std::fmod(Hash01Cue(k, 803) + u * 0.4f, 1.0f),
                0.3f + 4.2f * Hash01Cue(k, 804) + std::sin(t * 0.9f + (float)k) * 0.3f,
                -9.0f + 13.0f * Hash01Cue(k, 805)
            };
            float alfa = std::sin(u * PI);
            ResplandorFrenteCue(p, 0.07f, Color{ 255, 226, 160, (unsigned char)(140.0f * alfa) });
        }
    }

    // Panuelo: brilla y, cerca de una linea, toma el color de ese bando.
    if (e.hayCuerda)
    {
        float cerca = Limitar01Cue((std::fabs(m.marcador) - 0.6f) / 0.4f);
        Color halo = Color{ 255, 240, 200, 255 };

        if (cerca > 0.0f && m.indiceSolo >= 0)
        {
            int lider = -1;

            for (int i = 0; i < limite && lider < 0; i++)
            {
                bool esSolo = i == m.indiceSolo;
                if (m.resultado.participantes[i].participo && (m.marcador < 0.0f) == esSolo) lider = i;
            }

            if (lider >= 0)
            {
                halo = MezclaCue(halo, participantes[lider].color, cerca);
            }
        }

        float latido = 0.5f + 0.5f * std::sin(t * (5.0f + 6.0f * cerca));
        ResplandorFrenteCue({ e.nudo.x, e.nudo.y - 0.3f, 0.1f }, 0.6f + 0.25f * cerca * latido, AlfaCue(halo, 60.0f + 70.0f * cerca * latido));

        // El mojon al que se acerca el panuelo tambien late.
        if (cerca > 0.0f)
        {
            float x = (m.marcador < 0.0f ? -1.0f : 1.0f) * LINEA_VICTORIA_X_CUERDA;
            ResplandorFrenteCue({ x, 1.85f, Z_MOJON_CUE }, 0.9f, AlfaCue(halo, 110.0f * cerca * latido));
            ResplandorSueloCue({ x, 0.04f, Z_MOJON_CUE }, 0.9f, AlfaCue(halo, 60.0f * cerca * latido));
        }
    }

    // Tirones: destello en las manos; racha: aro dorado en el pasto.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo || !e.visible[i])
        {
            continue;
        }

        float d = m.destelloTiron[i];

        if (d > 0.0f)
        {
            ResplandorFrenteCue({ e.mano[i].x, e.mano[i].y, 0.2f }, 0.25f + 0.4f * d, AlfaCue(participantes[i].color, 150.0f * d));
        }

        if (m.anuncioRacha[i] > 0.0f && e.pies[i].y > -0.05f)
        {
            float u = m.anuncioRacha[i] / 1.2f;
            ResplandorSueloCue({ e.pies[i].x, 0.05f, e.pies[i].z }, 0.8f + 0.8f * (1.0f - u), AlfaCue(ORO_CUE, 120.0f * u));
        }
    }

    // Chispas con brillo.
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_CUERDA; i++)
    {
        const ParticulaVisualCuerda& p = m.particulasVisuales[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteCue(p.posicion, p.tamano * (1.2f + vida), AlfaCue(p.color, 210.0f * vida));
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: MEDIDOR DE RITMO (sobre la cabeza de cada jugador)
//==================================================

// Riel claro con la ventana verde; la aguja es el pulso real de la logica.
// En rojo durante el tropiezo; atenuada mientras recarga. Tres puntos de
// racha arriba.
static void DibujarMedidorRitmoCue(
    float cx,
    float yAbajo,
    float pulso,
    float ventana,
    bool tropiezo,
    bool recargando,
    float destello,
    int racha,
    float alfa
)
{
    float w = Px(60.0f);
    float h = Px(11.0f);
    float borde = Px(3.0f);
    Rectangle marco = { cx - w * 0.5f - borde, yAbajo - h - borde * 2.0f, w + borde * 2.0f, h + borde * 2.0f };
    Rectangle riel = { cx - w * 0.5f, marco.y + borde, w, h };

    DrawRectangleRounded(marco, 0.6f, 6, Fade(UI_MADERA_OSCURA, alfa));
    DrawRectangleRounded(riel, 0.6f, 6, Fade(UI_PLACA_CLARA, alfa));

    Color verde = tropiezo ? UI_PELIGRO : UI_POSITIVO;
    verde = MezclaCue(verde, WHITE, destello * 0.7f);
    float alfaVentana = recargando && !tropiezo ? 0.45f : 1.0f;
    float x0 = riel.x + (0.5f - ventana) * w;
    float x1 = riel.x + (0.5f + ventana) * w;
    DrawRectangleRec({ x0, riel.y + Px(1.5f), x1 - x0, h - Px(3.0f) }, Fade(verde, alfa * alfaVentana));
    DrawRectangleRec({ cx - Px(1.0f), riel.y + Px(1.5f), Px(2.0f), h - Px(3.0f) }, Fade(UI_TINTA, 0.3f * alfa));

    bool dentro = std::fabs(pulso - 0.5f) <= ventana && !tropiezo;
    float px = riel.x + pulso * w;
    DrawRectangleRounded({ px - Px(2.5f), marco.y - Px(2.0f), Px(5.0f), marco.height + Px(4.0f) }, 0.6f, 4, Fade(UI_TINTA, alfa));
    DrawRectangleRec({ px - Px(1.0f), marco.y, Px(2.0f), marco.height }, Fade(dentro ? UI_FOCO : WHITE, alfa));

    for (int k = 0; k < ACIERTOS_PARA_RACHA_CUERDA; k++)
    {
        Vector2 c = { cx + ((float)k - 0.5f * (float)(ACIERTOS_PARA_RACHA_CUERDA - 1)) * Px(10.0f), marco.y - Px(6.0f) };
        bool lleno = k < racha;
        DrawCircleV(c, Px(3.6f), Fade(UI_TINTA, alfa));
        DrawCircleV(c, Px(2.4f), Fade(lleno ? UI_FOCO : UI_PLACA_CLARA, alfa));
    }
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoCuerdaAcantilado::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    PrepararContornosCue();

    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool alta = CalidadDecoracion() == CALIDAD_ALTA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = alta ? 1 : 2;

    // Entrada: durante la cuenta la camara llega desde atras, alta y
    // corrida a la izquierda, mirando al sol.
    Camera3D vista = camara;

    if (fase == FASE_CUERDA_PREPARACION)
    {
        float u = Limitar01Cue(tiempoPreparacion / DURACION_PREPARACION_CUERDA);
        float entrada = SuaveCue(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.x -= 4.0f * entrada;
        vista.position.y += 2.6f * entrada;
        vista.position.z += 7.0f * entrada;
        vista.target.y += 1.4f * entrada;
    }

    DibujarCieloCue(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarMarCue(t, efectos);

    if (decoracion)
    {
        DibujarHorizonteCue(t, alta);
        DibujarFarallonesCue();
    }

    for (int lado = 0; lado < 2; lado++)
    {
        DibujarParedesAcantiladoCue(lado);
        DibujarPastoAcantiladoCue(lado, decoracion);
    }

    // Tierra pisada: el rango real que recorren los pies de cada bando.
    DibujarHuellasCue(X_BASE_SOLO_CUERDA - DESPLAZAMIENTO_CUERPOS_CUERDA - 0.6f, -BORDE_ACANTILADO_CUERDA, 1);
    DibujarHuellasCue(
        BORDE_ACANTILADO_CUERDA,
        X_BASE_TRIO_CUERDA + SEPARACION_TRIO_CUERDA * 2.0f + DESPLAZAMIENTO_CUERPOS_CUERDA + 0.6f,
        2
    );

    DibujarMojonesCue(*this, participantes, limite, t);
    Vector3 estacaSolo = DibujarEstacaCue({ -8.9f, 0.0f, 0.55f });
    Vector3 estacaTrio = DibujarEstacaCue({ 11.2f, 0.0f, 0.55f });

    Vector3 farolas[2];
    int cantidadFarolas = 0;
    Vector3 ventana{};

    if (decoracion)
    {
        DibujarLomasCue();
        DibujarFaroCue();
        ventana = DibujarCasaFareroCue();
        DibujarTorreRuinasCue({ -16.5f, 0.0f, -14.5f });
        DibujarPinoCue({ -10.8f, 0.0f, -6.2f }, 1.15f, t, 1);
        DibujarPinoCue({ -13.4f, 0.0f, -9.4f }, 1.4f, t, 2);
        DibujarPinoCue({ 7.2f, 0.0f, -10.5f }, 1.0f, t, 3);

        if (alta)
        {
            DibujarPinoCue({ -8.2f, 0.0f, -12.4f }, 0.9f, t, 4);
            DibujarPinoCue({ 21.5f, 0.0f, -6.5f }, 1.2f, t, 5);
        }

        DibujarBancoCue({ -8.6f, 0.0f, -2.6f }, 0.0f);
        DibujarCatalejoCue({ -4.7f, 0.0f, -2.7f }, t);
        DibujarCartelPeligroCue({ -7.7f, 0.0f, 2.95f });
        DibujarSalvavidasCue({ 4.4f, 0.0f, -3.3f });
        farolas[cantidadFarolas++] = DibujarFarolaCue({ -10.2f, 0.0f, 2.5f }, 1.0f);
        farolas[cantidadFarolas++] = DibujarFarolaCue({ 10.4f, 0.0f, 2.2f }, -1.0f);

        DibujarRocaCue({ -12.6f, 0.0f, 1.7f }, 0.5f, 1);
        DibujarRocaCue({ -6.4f, 0.0f, -3.7f }, 0.42f, 2);
        DibujarRocaCue({ 12.6f, 0.0f, -2.4f }, 0.55f, 3);
        DibujarRocaCue({ 5.6f, 0.0f, -3.4f }, 0.38f, 4);
        DibujarMataCue({ -11.8f, 0.0f, -2.8f }, 0.6f, 1);
        DibujarMataCue({ 8.8f, 0.0f, -3.4f }, 0.55f, 2);
        DibujarMataCue({ 14.4f, 0.0f, 1.2f }, 0.6f, 3);
        DibujarMojonPiedrasCue({ -12.0f, 0.0f, 0.2f });

        if (alta)
        {
            DibujarRocaCue({ -13.4f, 0.0f, -1.2f }, 0.6f, 5);
            DibujarRocaCue({ 15.4f, 0.0f, -4.2f }, 0.45f, 6);
            DibujarMataCue({ -9.8f, 0.0f, 3.2f }, 0.45f, 4);
            DibujarMataCue({ 12.4f, 0.0f, 3.1f }, 0.45f, 5);
        }

        DibujarCercaCue(-19.0f, -11.0f, 3.0f);
        DibujarCercaCue(11.6f, 19.6f, 3.1f);
        DibujarFloresCue(t, paso);
        DibujarGaviotasCue(t, paso);
    }

    EscenaJugadoresCue escena;
    PrepararEscenaJugadoresCue(*this, limite, escena);
    DibujarCuerdaCue(*this, limite, escena, estacaSolo, estacaTrio, t);
    DibujarJugadoresCue(*this, escena, jugadores, limite, participantes, mostrarDebug);

    // Traslucidos: espuma, bruma, humo, vidrio del faro y particulas.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    DibujarEspumaCue(t, efectos);

    if (decoracion)
    {
        DibujarHumoCue(t);
        Vector3 b = FARO_CUE;
        PrismaCue({ b.x, 0.45f + ALTO_TORRE_FARO_CUE + 0.34f, b.z }, 0.64f, 0.64f, 0.95f, 10, Color{ 255, 236, 190, 70 }, false);
    }

    DibujarParticulasCue(*this);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    if (efectos)
    {
        DibujarLucesCue(*this, escena, participantes, limite, farolas, cantidadFarolas, decoracion, ventana, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Cartel J# y medidor de ritmo sobre cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !escena.visible[i] || !participantes[i].activo)
        {
            continue;
        }

        const EstadoJugadorCuerdaAcantilado& estado = estadosJugadores[i];
        Vector3 cabeza = { escena.pies[i].x, escena.pies[i].y + 2.25f, 0.0f };
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        Rectangle placa = DibujarEtiquetaUI(rotulo, p, 0.0f, op);

        if (fase != FASE_CUERDA_TERMINADO)
        {
            DibujarMedidorRitmoCue(
                p.x,
                placa.y - Px(4.0f),
                PosicionPulsoCuerda(estado.fasePulso),
                VentanaJugadorCuerda(*this, i),
                estado.tiempoTropiezo > 0.0f,
                estado.tiempoRecarga > 0.0f,
                destelloTiron[i],
                anuncioRacha[i] > 0.0f ? ACIERTOS_PARA_RACHA_CUERDA : estado.racha,
                1.0f
            );
        }

        if (anuncioRacha[i] > 0.0f)
        {
            float u = 1.0f - anuncioRacha[i] / 1.2f;
            DibujarTextoMundoContornoUI("¡RACHA!", p.x, placa.y - Px(58.0f) - Px(22.0f) * u, UI_TEXTO, ORO_CUE, Limitar01Cue(anuncioRacha[i] * 2.5f));
        }
    }

    // «¡AL AGUA!» sobre el canal cuando cae el bando perdedor.
    if (fase == FASE_CUERDA_TERMINADO && !empate && tiempoFin > 0.9f && tiempoFin < 3.4f)
    {
        float x = ganaSolo ? 2.0f : -2.0f;
        Vector2 s = GetWorldToScreen({ x, ALTURA_MAR_CUERDA + 3.2f, 0.0f }, vista);
        float alfa = Limitar01Cue((tiempoFin - 0.9f) * 4.0f) * Limitar01Cue((3.4f - tiempoFin) * 2.0f);
        DibujarTextoMundoContornoUI("¡AL AGUA!", s.x, s.y, UI_TEXTO_GRANDE, Color{ 210, 242, 255, 255 }, alfa);
    }

    // HUD comun.
    const char* nombreBando = cantidadRivales >= 3 ? "TRÍO" : (cantidadRivales == 2 ? "DÚO" : "RIVAL");
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CUERDA DEL ACANTILADO",
        "PULSÁ CON LA AGUJA EN LO VERDE Y TIRÁ AL OTRO BANDO AL MAR",
        resultado
    );

    hud.fase = fase == FASE_CUERDA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CUERDA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = fmaxf(0.0f, DURACION_PARTIDA_CUERDA - tiempoJuego);
    hud.tiempoTotal = DURACION_PARTIDA_CUERDA;
    hud.segundosAlerta = 8.0f;
    hud.finPorTiempo = tiempoJuego >= DURACION_PARTIDA_CUERDA;

    static char tituloFinal[40];

    if (empate)
    {
        hud.tituloFinal = "¡EMPATE!";
    }
    else if (ganaSolo)
    {
        hud.tituloFinal = "¡GANA EL SOLITARIO!";
    }
    else
    {
        std::snprintf(tituloFinal, sizeof(tituloFinal), "¡GANA EL %s!", nombreBando);
        hud.tituloFinal = tituloFinal;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        const EstadoJugadorCuerdaAcantilado& estado = estadosJugadores[i];
        bool esSolo = i == indiceSolo;

        j.equipo = esSolo ? 0 : 1;
        j.rol = esSolo ? "SOLITARIO" : nombreBando;
        std::snprintf(j.valor, sizeof(j.valor), "%d", estado.aciertos);
        j.valorNumerico = estado.aciertos;
        j.progreso = Limitar01Cue(esSolo ? (1.0f - marcador) * 0.5f : (1.0f + marcador) * 0.5f);
        std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), estado.aciertos == 1 ? "%d TIRÓN" : "%d TIRONES", estado.aciertos);

        if (fase == FASE_CUERDA_JUGANDO && estado.tiempoFeedback > 0.0f)
        {
            const char* texto = estado.feedback == FEEDBACK_CUERDA_RACHA
                ? "¡RACHA!"
                : (estado.feedback == FEEDBACK_CUERDA_TROPIEZO ? "TROPIEZO" : "¡TIRÓN!");
            std::snprintf(j.estado, sizeof(j.estado), "%s", texto);
        }

        if (EsPerdedorCue(*this, i) && chapuzonVisto[i])
        {
            j.eliminado = true;
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "TIRÁ: %s\nCON LA AGUJA EN LO VERDE",
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoCuerdaAcantilado::ObtenerResultado() const
{
    return resultado;
}
