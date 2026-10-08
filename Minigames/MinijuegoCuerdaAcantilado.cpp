#include "Minigames/MinijuegoCuerdaAcantilado.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>


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

void MinijuegoCuerdaAcantilado::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    // Camara lateral/diagonal: ambos acantilados y el mar en cuadro.
    camara.position = { 1.0f, 3.2f, 17.5f };
    camara.target = { 0.0f, -0.4f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
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
}


//==================================================
// VISUAL: escenario de acantilados
//==================================================

// MODELO FUTURO: los acantilados, el faro, las rocas del mar, las gaviotas
// y los mojones de meta podrian reemplazarse por un .glb de escenario
// (Assets/Modelos/Escenarios/acantilados.glb) sin tocar el gameplay.
static void DibujarEscenarioVisualAcantilados(float tiempo)
{
    Color piedra = Color{ 120, 108, 98, 255 };
    Color piedraOscura = Color{ 92, 82, 76, 255 };
    Color pasto = Color{ 96, 154, 78, 255 };

    // Mar (el hueco central) y espuma animada.
    DrawPlane(
        { 0.0f, ALTURA_MAR_CUERDA, -4.0f },
        { 90.0f, 60.0f },
        Color{ 36, 110, 168, 255 }
    );

    for (int i = 0; i < 9; i++)
    {
        float x = -7.0f + 1.8f * (float)i + std::sin(tiempo * 1.3f + i) * 0.5f;
        float z = -2.0f + (float)((i * 5) % 7);
        DrawCube(
            { x, ALTURA_MAR_CUERDA + 0.03f, z },
            1.3f,
            0.05f,
            0.12f,
            Color{ 214, 236, 248, 255 }
        );
    }

    // Acantilados: bloque de roca, capa de pasto y estratos salientes.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        float centroX = s * (BORDE_ACANTILADO_CUERDA + 4.4f);

        DrawCube({ centroX, -4.65f, -3.0f }, 8.8f, 9.0f, 9.0f, piedra);
        DrawCube({ centroX, -0.12f, -3.0f }, 8.8f, 0.24f, 9.0f, pasto);

        for (int k = 0; k < 4; k++)
        {
            float y = -1.4f - 1.7f * (float)k;
            float saliente = 0.25f + 0.18f * (float)((k * 3) % 3);
            DrawCube(
                { s * (BORDE_ACANTILADO_CUERDA - saliente * 0.5f + 0.05f), y, -2.6f },
                saliente,
                0.7f,
                8.2f,
                k % 2 == 0 ? piedraOscura : piedra
            );
        }

        // Rocas y matas sobre la hierba.
        for (int k = 0; k < 5; k++)
        {
            float x = s * (BORDE_ACANTILADO_CUERDA + 1.2f + 1.6f * (float)k);
            float z = -2.8f - 0.5f * (float)(k % 2);
            DrawSphere({ x, 0.12f, z }, 0.3f + 0.08f * (float)(k % 3), piedraOscura);
            DrawCube({ x + 0.7f, 0.12f, z + 4.4f }, 0.35f, 0.24f, 0.35f, Color{ 66, 122, 58, 255 });
        }
    }

    // Faro sobre el acantilado derecho.
    {
        float x = 10.0f;
        float z = -3.6f;

        for (int k = 0; k < 4; k++)
        {
            DrawCylinder(
                { x, 0.0f + 0.9f * (float)k, z },
                0.55f - 0.07f * (float)k,
                0.62f - 0.07f * (float)k,
                0.9f,
                10,
                k % 2 == 0 ? Color{ 238, 238, 232, 255 } : Color{ 196, 62, 56, 255 }
            );
        }

        DrawCylinder({ x, 3.6f, z }, 0.45f, 0.45f, 0.5f, 10, Color{ 255, 226, 120, 255 });
        DrawCylinder({ x, 4.1f, z }, 0.0f, 0.6f, 0.6f, 10, Color{ 120, 52, 48, 255 });
    }

    // Torres de roca en el mar y montanas lejanas.
    for (int i = 0; i < 5; i++)
    {
        float x = -14.0f + 6.5f * (float)i;
        float alto = 3.0f + 1.2f * (float)((i * 2) % 3);
        DrawCylinder(
            { x, ALTURA_MAR_CUERDA, -13.0f - (float)(i % 2) * 3.0f },
            0.9f,
            1.7f,
            alto + 1.0f,
            8,
            piedraOscura
        );
    }

    DrawCylinder({ -16.0f, ALTURA_MAR_CUERDA, -30.0f }, 0.0f, 12.0f, 14.0f, 6, Color{ 124, 150, 176, 255 });
    DrawCylinder({ 14.0f, ALTURA_MAR_CUERDA, -34.0f }, 0.0f, 14.0f, 17.0f, 6, Color{ 138, 162, 188, 255 });

    // Nubes.
    for (int i = 0; i < 4; i++)
    {
        float x = -16.0f + 11.0f * (float)i + std::fmod(tiempo * 0.35f, 44.0f);
        if (x > 24.0f) x -= 44.0f;
        float y = 8.0f + 1.2f * (float)(i % 2);
        Color nube = Color{ 250, 250, 252, 255 };
        DrawSphere({ x, y, -16.0f }, 1.7f, nube);
        DrawSphere({ x + 1.6f, y - 0.2f, -16.0f }, 1.3f, nube);
        DrawSphere({ x - 1.5f, y - 0.3f, -16.0f }, 1.2f, nube);
    }

    // Gaviotas.
    for (int i = 0; i < 4; i++)
    {
        float a = tiempo * (0.5f + 0.1f * (float)i) + 1.7f * (float)i;
        float x = std::cos(a) * (4.0f + (float)i);
        float y = 2.6f + 0.6f * (float)i + std::sin(a * 2.0f) * 0.3f;
        float z = -5.0f + std::sin(a) * 3.0f;
        float aleteo = std::sin(tiempo * 9.0f + (float)i) * 0.18f;

        DrawCube({ x, y, z }, 0.3f, 0.12f, 0.12f, RAYWHITE);
        DrawCube({ x, y + aleteo, z - 0.22f }, 0.12f, 0.04f, 0.34f, RAYWHITE);
        DrawCube({ x, y + aleteo, z + 0.22f }, 0.12f, 0.04f, 0.34f, RAYWHITE);
    }
}


// Mojones de meta, centro y bandera del marcador.
static void DibujarMarcadoresVisualCuerda(
    float marcador,
    Color colorSolo,
    Color colorTrio
)
{
    Color madera = Color{ 110, 78, 50, 255 };

    for (int lado = -1; lado <= 1; lado++)
    {
        float x = (float)lado * LINEA_VICTORIA_X_CUERDA;
        Color bandera = lado < 0 ? colorSolo : (lado > 0 ? colorTrio : RAYWHITE);
        float alto = lado == 0 ? 1.2f : 2.1f;

        DrawCylinder({ x, 0.0f, -1.1f }, 0.07f, 0.07f, alto, 6, madera);
        DrawCube({ x + 0.28f, alto - 0.2f, -1.1f }, 0.5f, 0.3f, 0.05f, bandera);
    }

    float x = marcador * LINEA_VICTORIA_X_CUERDA;
    DrawCube({ x, ALTURA_MANOS_CUERDA - 0.45f, 0.0f }, 0.12f, 0.5f, 0.12f, Color{ 230, 60, 52, 255 });
    DrawSphere({ x, ALTURA_MANOS_CUERDA, 0.0f }, 0.2f, Color{ 230, 60, 52, 255 });
}


// Cuerda entre las manos del solitario y el miembro mas lejano del trio.
static void DibujarCuerdaVisual(Vector3 inicio, Vector3 fin)
{
    const int segmentos = 18;
    Vector3 anterior = inicio;

    for (int i = 1; i <= segmentos; i++)
    {
        float t = (float)i / (float)segmentos;
        Vector3 punto =
        {
            MezclarCuerda(inicio.x, fin.x, t),
            MezclarCuerda(inicio.y, fin.y, t) - 0.12f * std::sin(t * PI),
            0.0f
        };

        DrawCylinderEx(anterior, punto, 0.045f, 0.045f, 6, Color{ 196, 160, 98, 255 });
        anterior = punto;
    }
}


// Dibuja un jugador con el modelo compartido, inclinado respecto a los pies.
static void DibujarJugadorInclinadoCuerda(
    Vector3 pies,
    bool haciaDerecha,
    float inclinacionGrados,
    Color color
)
{
    // Positivo inclina hacia -X; el trio (a la derecha) se invierte.
    float angulo = haciaDerecha ? inclinacionGrados : -inclinacionGrados;
    float mirada = haciaDerecha ? 90.0f : -90.0f;

    rlPushMatrix();
    rlTranslatef(pies.x, pies.y, pies.z);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);
    DibujarModeloJugadorEnPosicion({ 0.0f, 0.0f, 0.0f }, mirada, color);
    rlPopMatrix();
}


static void DibujarSalpicaduraCuerda(float x, float tiempoDesdeImpacto)
{
    if (tiempoDesdeImpacto < 0.0f || tiempoDesdeImpacto > 1.4f)
    {
        return;
    }

    float alfa = 1.0f - tiempoDesdeImpacto / 1.4f;

    for (int k = 0; k < 6; k++)
    {
        float a = (float)k * 1.047f;
        float radio = 0.3f + tiempoDesdeImpacto * 1.6f;
        float altura = 2.2f * tiempoDesdeImpacto - 3.5f * tiempoDesdeImpacto * tiempoDesdeImpacto;
        if (altura < 0.0f) altura = 0.0f;

        DrawSphere(
            { x + std::cos(a) * radio, ALTURA_MAR_CUERDA + altura, std::sin(a) * radio },
            0.13f,
            Fade(Color{ 230, 245, 255, 255 }, alfa)
        );
    }

    DrawCylinder(
        { x, ALTURA_MAR_CUERDA, 0.0f },
        0.3f + tiempoDesdeImpacto * 1.4f,
        0.35f + tiempoDesdeImpacto * 1.6f,
        0.06f,
        16,
        Fade(Color{ 235, 248, 255, 255 }, alfa * 0.7f)
    );
}


void MinijuegoCuerdaAcantilado::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 142, 200, 238, 255 });
    BeginMode3D(camara);

    DibujarEscenarioVisualAcantilados(tiempoAnimacion);

    Color colorSolo = indiceSolo >= 0 ? participantes[indiceSolo].color : RED;
    Color colorTrio = Color{ 250, 250, 250, 255 };

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo && i != indiceSolo)
        {
            colorTrio = participantes[i].color;
            break;
        }
    }

    DibujarMarcadoresVisualCuerda(marcador, colorSolo, colorTrio);

    // Puntas de la cuerda: manos del solitario y del trio mas lejano.
    bool hayExtremos = false;
    Vector3 manoSolo{};
    Vector3 manoFinal{};
    float xFinal = -1000.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 pies = { jugadores[i].posicion.x,
            jugadores[i].posicion.y - OFFSET_CENTRO_JUGADOR_CUERDA, 0.0f };

        if (i == indiceSolo)
        {
            manoSolo = { pies.x + 0.3f, pies.y + ALTURA_MANOS_CUERDA, 0.0f };
            hayExtremos = true;
        }
        else if (pies.x > xFinal)
        {
            xFinal = pies.x;
            manoFinal = { pies.x - 0.3f, pies.y + ALTURA_MANOS_CUERDA, 0.0f };
        }
    }

    if (hayExtremos && xFinal > -999.0f)
    {
        DibujarCuerdaVisual(manoSolo, manoFinal);
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCuerdaAcantilado& estado = estadosJugadores[i];
        Vector3 pies =
        {
            jugadores[i].posicion.x,
            jugadores[i].posicion.y - OFFSET_CENTRO_JUGADOR_CUERDA,
            0.0f
        };

        // Retroceso base al tirar; el tropiezo lo inclina hacia adelante.
        float inclinacion = 10.0f + 16.0f * estado.retroceso;

        if (estado.tiempoTropiezo > 0.0f)
        {
            inclinacion = -20.0f;
        }

        if (fase == FASE_CUERDA_PREPARACION)
        {
            inclinacion = 4.0f;
        }
        else if (pies.y < -0.05f)
        {
            // En caida: el cuerpo gira.
            inclinacion = 30.0f + 260.0f * tiempoFin;
        }

        bool trio = estado.equipo == 1;
        DibujarJugadorInclinadoCuerda(pies, !trio, inclinacion, participantes[i].color);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    // Salpicaduras donde caen los perdedores.
    if (fase == FASE_CUERDA_TERMINADO && !empate)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            const EstadoJugadorCuerdaAcantilado& estado = estadosJugadores[i];
            bool perdedor = (estado.equipo == 0) == !ganaSolo;

            if (perdedor)
            {
                float impacto = 0.25f + 0.12f * (float)estado.ordenEnEquipo + 0.88f;
                DibujarSalpicaduraCuerda(jugadores[i].posicion.x, tiempoFin - impacto);
            }
        }
    }

    EndMode3D();

    // ---------------- HUD ----------------
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangle(18, 14, 560, 62, Fade(BLACK, 0.75f));
    DrawText("CUERDA DEL ACANTILADO - 1 VS 3", 30, 22, 26, GOLD);

    if (indiceSolo >= 0)
    {
        DrawText(
            TextFormat(
                "SOLITARIO: J%d%s  |  PULSA EN LA ZONA VERDE",
                participantes[indiceSolo].numeroJugador,
                participantes[indiceSolo].esBot ? " (BOT)" : ""
            ),
            30,
            52,
            16,
            RAYWHITE
        );
    }

    // Barra de tension de la cuerda.
    int barraX = ancho / 2 - 220;
    int barraY = 88;
    DrawRectangle(barraX - 4, barraY - 4, 448, 28, Fade(BLACK, 0.75f));
    DrawRectangle(barraX, barraY, 440, 20, Color{ 70, 70, 80, 255 });
    DrawRectangle(barraX, barraY, 220, 20, Fade(colorSolo, 0.55f));
    DrawRectangle(barraX + 220, barraY, 220, 20, Fade(colorTrio, 0.55f));
    int marcadorPx = barraX + 220 + (int)(marcador * 218.0f);
    DrawRectangle(marcadorPx - 4, barraY - 6, 8, 32, Color{ 230, 60, 52, 255 });
    DrawText("SOLO", barraX + 4, barraY + 30, 14, RAYWHITE);
    DrawText("TRIO", barraX + 404, barraY + 30, 14, RAYWHITE);

    if (fase == FASE_CUERDA_JUGANDO)
    {
        float restante = DURACION_PARTIDA_CUERDA - tiempoJuego;
        if (restante < 0.0f) restante = 0.0f;

        DrawRectangle(ancho - 212, 16, 196, 40, Fade(BLACK, 0.75f));
        DrawText(
            TextFormat("TIEMPO %.1f", restante),
            ancho - 200,
            24,
            24,
            restante <= 8.0f ? RED : GOLD
        );
    }

    // Paneles de ritmo por jugador.
    int activos = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo) activos++;
    }

    // En ventanas angostas los paneles se achican para no salir de pantalla.
    int panelAncho = 232;
    if (activos > 0)
    {
        int maximoPanel = (GetScreenWidth() - 40) / activos - 8;
        if (panelAncho > maximoPanel) panelAncho = maximoPanel;
    }
    int panelAlto = 92;
    int inicioX = ancho / 2 - (activos * (panelAncho + 8)) / 2;
    int columna = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCuerdaAcantilado& estado = estadosJugadores[i];
        int px = inicioX + columna * (panelAncho + 8);
        int py = alto - panelAlto - 12;
        columna++;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.8f));
        DrawRectangleLines(px, py, panelAncho, panelAlto, participantes[i].color);

        DrawText(
            TextFormat(
                "J%d %s%s",
                participantes[i].numeroJugador,
                i == indiceSolo ? "SOLO" : "TRIO",
                participantes[i].esBot ? " (BOT)" : ""
            ),
            px + 8,
            py + 6,
            16,
            participantes[i].color
        );
        DrawText(
            TextFormat("BOTON: %s", ObtenerTextoBotonPrincipal(participantes[i])),
            px + 8,
            py + 26,
            14,
            RAYWHITE
        );

        // Barra de ritmo con la ventana de acierto en verde.
        int bx = px + 8;
        int by = py + 48;
        int bw = panelAncho - 16;
        float ventana = VentanaJugadorCuerda(*this, i);

        DrawRectangle(bx, by, bw, 16, Color{ 60, 60, 70, 255 });
        DrawRectangle(
            bx + (int)((0.5f - ventana) * (float)bw),
            by,
            (int)(ventana * 2.0f * (float)bw),
            16,
            estado.tiempoTropiezo > 0.0f ? Color{ 120, 60, 60, 255 } : Color{ 70, 200, 90, 255 }
        );

        float pulso = PosicionPulsoCuerda(estado.fasePulso);
        DrawRectangle(bx + (int)(pulso * (float)(bw - 4)), by - 3, 4, 22, RAYWHITE);

        const char* texto = "";
        Color colorTexto = RAYWHITE;

        if (estado.tiempoFeedback > 0.0f)
        {
            if (estado.feedback == FEEDBACK_CUERDA_ACIERTO)
            {
                texto = "ACIERTO";
                colorTexto = LIME;
            }
            else if (estado.feedback == FEEDBACK_CUERDA_RACHA)
            {
                texto = "RACHA! TIRON EXTRA";
                colorTexto = GOLD;
            }
            else if (estado.feedback == FEEDBACK_CUERDA_TROPIEZO)
            {
                texto = "TROPIEZO";
                colorTexto = RED;
            }
        }
        else
        {
            texto = TextFormat("RACHA %d/%d", estado.racha, ACIERTOS_PARA_RACHA_CUERDA);
        }

        DrawText(texto, px + 8, py + 70, 15, colorTexto);
    }

    if (fase == FASE_CUERDA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            ancho / 2 - MeasureText(texto, 88) / 2,
            alto / 2 - 100,
            88,
            GOLD
        );
    }
    else if (
        fase == FASE_CUERDA_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        tiempoFin > 1.2f
    )
    {
        const char* titulo = empate
            ? "EMPATE"
            : (ganaSolo ? "GANA EL SOLITARIO" : "GANA EL TRIO");

        DrawRectangle(ancho / 2 - 290, alto / 2 - 150, 580, 130, Fade(BLACK, 0.9f));
        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, 36) / 2,
            alto / 2 - 125,
            36,
            GOLD
        );
        DrawText(
            TextoReinicioMinijuego(),
            ancho / 2 - MeasureText(TextoReinicioMinijuego(), 21) / 2,
            alto / 2 - 65,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoCuerdaAcantilado::ObtenerResultado() const
{
    return resultado;
}
