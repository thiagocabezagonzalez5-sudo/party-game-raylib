#include "Minigames/MinijuegoTrepaMastil.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_MASTIL = 3.0f;
static const float DURACION_PARTIDA_MASTIL = 45.0f;

// Trepa.
static const float ALTURA_META_MASTIL = 10.0f;
static const float AVANCE_AGARRE_MASTIL = 0.07f;
static const float DESLIZ_MISMA_TECLA_MASTIL = 0.08f;
static const float TOPE_AGARRES_POR_SEGUNDO_MASTIL = 9.0f;
static const float LIMITE_SONIDO_AGARRE_MASTIL = 0.125f;

// Olas: empiezan a mitad de la ronda con intensidad creciente.
static const float INICIO_OLAS_MASTIL = 14.0f;
static const float PLENAS_OLAS_MASTIL = 24.0f;
static const float UMBRAL_INCLINACION_MASTIL = 0.35f;
static const float BONUS_FAVORABLE_MASTIL = 1.6f;
static const float FACTOR_EN_CONTRA_MASTIL = 0.4f;
static const float RESBALE_EN_CONTRA_MASTIL = 0.3f;
static const float ANGULO_MAXIMO_BARCO_MASTIL = 6.0f;

// Cuervos.
static const float PRIMER_CUERVO_MASTIL = 6.0f;
static const float AVISO_CUERVO_MASTIL = 1.3f;
static const float VELOCIDAD_CUERVO_MASTIL = 9.0f;
static const float RECORRIDO_CUERVO_MASTIL = 9.0f;
static const float CAIDA_CUERVO_MASTIL = 1.0f;

static const float SEPARACION_MASTILES_MASTIL = 4.4f;
static const float PROFUNDIDAD_JUGADOR_MASTIL = 0.55f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarMastil(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioMastil(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static int LimiteMastil(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static const char* NombreJugadorMastil(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static const char* TextoAccionMastil(const Participante& participante)
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


// Angulo (radianes) con el que se dibuja el barco para una inclinacion dada.
static float AnguloBarcoMastil(float inclinacion)
{
    return -inclinacion * ANGULO_MAXIMO_BARCO_MASTIL * DEG2RAD;
}


// +1 si la inclinacion favorece al trepador, -1 si va en contra, 0 neutro.
static int EfectoInclinacionMastil(const MinijuegoTrepaMastil& m, int indice)
{
    float efecto = m.inclinacion * (float)m.trepadores[indice].lado;

    if (efecto > UMBRAL_INCLINACION_MASTIL) return 1;
    if (efecto < -UMBRAL_INCLINACION_MASTIL) return -1;
    return 0;
}


static void AgregarParticulaMastil(
    MinijuegoTrepaMastil& m,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_MASTIL; i++)
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


static void RafagaMastil(
    MinijuegoTrepaMastil& m,
    float x,
    float y,
    float z,
    Color color,
    int cantidad
)
{
    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AleatorioMastil(0.0f, 2.0f * PI);
        float fuerza = AleatorioMastil(0.8f, 2.6f);

        AgregarParticulaMastil(
            m,
            { x, y, z },
            { std::cos(angulo) * fuerza, AleatorioMastil(0.5f, 2.5f), std::sin(angulo) * fuerza * 0.5f },
            AleatorioMastil(0.3f, 0.6f),
            AleatorioMastil(0.05f, 0.1f),
            color
        );
    }
}


//==================================================
// LOGICA: TREPA
//==================================================

static void AplicarCaidaMastil(MinijuegoTrepaMastil& m, int indice, float metros)
{
    TrepadorMastil& t = m.trepadores[indice];
    t.altura = t.altura - metros < 0.0f ? 0.0f : t.altura - metros;

    if (t.enfriamientoCaida <= 0.0f)
    {
        t.enfriamientoCaida = 0.3f;
        ReproducirSonidoMinijuego(m.audio, SONIDO_CAIDA);
    }
}


// tecla: 1 = salto (mano izquierda), 2 = accion (mano derecha).
static void AplicarAgarreMastil(
    MinijuegoTrepaMastil& m,
    int indice,
    int tecla,
    bool humano
)
{
    TrepadorMastil& t = m.trepadores[indice];

    t.enfriamiento = 1.0f / TOPE_AGARRES_POR_SEGUNDO_MASTIL;
    t.pulso = 1.0f;
    t.mano = tecla == 1 ? -1.0f : 1.0f;

    if (tecla == t.ultimaTecla)
    {
        // Misma mano dos veces: resbala un poco.
        AplicarCaidaMastil(m, indice, DESLIZ_MISMA_TECLA_MASTIL);
        return;
    }

    t.ultimaTecla = tecla;

    float avance = AVANCE_AGARRE_MASTIL;
    int efecto = EfectoInclinacionMastil(m, indice);

    if (efecto > 0)
    {
        avance *= BONUS_FAVORABLE_MASTIL;
    }
    else if (efecto < 0)
    {
        avance *= FACTOR_EN_CONTRA_MASTIL;
    }

    t.altura += avance;

    if (humano && t.enfriamientoSonido <= 0.0f)
    {
        t.enfriamientoSonido = LIMITE_SONIDO_AGARRE_MASTIL;
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
    }
}


static void ActualizarOlasMastil(MinijuegoTrepaMastil& m, float transcurrido)
{
    m.intensidadOlas = LimitarMastil(
        (transcurrido - INICIO_OLAS_MASTIL) / (PLENAS_OLAS_MASTIL - INICIO_OLAS_MASTIL),
        0.0f,
        1.0f
    );

    float onda = std::sin(transcurrido * 0.95f + m.faseOlas) + 0.3f * std::sin(transcurrido * 2.3f + m.faseOlas * 2.0f);
    m.inclinacion = LimitarMastil(m.intensidadOlas * onda * 0.85f, -1.0f, 1.0f);
}


static void ActualizarTrepadorMastil(
    MinijuegoTrepaMastil& m,
    int indice,
    const InputMinijuegoParticipante& entrada,
    bool humano,
    float deltaTime
)
{
    TrepadorMastil& t = m.trepadores[indice];

    if (t.enfriamiento > 0.0f) t.enfriamiento -= deltaTime;
    if (t.enfriamientoSonido > 0.0f) t.enfriamientoSonido -= deltaTime;
    if (t.enfriamientoCaida > 0.0f) t.enfriamientoCaida -= deltaTime;
    if (t.inmunidad > 0.0f) t.inmunidad -= deltaTime;
    if (t.pulso > 0.0f) t.pulso -= deltaTime * 6.0f;
    if (t.tiempoMensaje > 0.0f) t.tiempoMensaje -= deltaTime;

    // En contra de la inclinacion el mastil resbala solo.
    if (EfectoInclinacionMastil(m, indice) < 0 && t.altura > 0.0f)
    {
        t.altura -= RESBALE_EN_CONTRA_MASTIL * deltaTime;

        if (t.altura < 0.0f)
        {
            t.altura = 0.0f;
        }
    }

    if (t.enfriamiento > 0.0f)
    {
        return;
    }

    int tecla = 0;

    if (entrada.saltar && entrada.golpear)
    {
        // Ambas a la vez: se toma la que alterna (no se gana nada extra).
        tecla = t.ultimaTecla == 1 ? 2 : 1;
    }
    else if (entrada.saltar)
    {
        tecla = 1;
    }
    else if (entrada.golpear)
    {
        tecla = 2;
    }

    if (tecla != 0)
    {
        AplicarAgarreMastil(m, indice, tecla, humano);
    }
}


//==================================================
// LOGICA: CUERVOS
//==================================================

static void ActualizarCuervosMastil(
    MinijuegoTrepaMastil& m,
    int limite,
    float transcurrido,
    float deltaTime
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        CuervoMastil& c = m.cuervos[i];
        TrepadorMastil& t = m.trepadores[i];

        if (transcurrido < PRIMER_CUERVO_MASTIL)
        {
            continue;
        }

        if (!c.activo)
        {
            c.proximo -= deltaTime;

            if (c.proximo <= 0.0f)
            {
                // Avisa por el lado por el que va a entrar, a la altura del trepador.
                c.activo = true;
                c.enAviso = true;
                c.tiempoAviso = AVISO_CUERVO_MASTIL;
                c.direccion = GetRandomValue(0, 1) == 0 ? 1.0f : -1.0f;
                c.x = -c.direccion * RECORRIDO_CUERVO_MASTIL;
                c.altura = LimitarMastil(t.altura + AleatorioMastil(1.1f, 2.0f), 1.5f, ALTURA_META_MASTIL + 1.0f);
                c.golpeo = false;
                c.fase = AleatorioMastil(0.0f, 6.0f);
            }

            continue;
        }

        c.fase += deltaTime;

        if (c.enAviso)
        {
            c.tiempoAviso -= deltaTime;

            if (c.tiempoAviso <= 0.0f)
            {
                c.enAviso = false;
            }

            continue;
        }

        c.x += c.direccion * VELOCIDAD_CUERVO_MASTIL * deltaTime;

        if (
            !c.golpeo &&
            t.inmunidad <= 0.0f &&
            !t.llego &&
            std::fabs(c.x) < 0.55f &&
            std::fabs(c.altura - (t.altura + 0.7f)) < 0.65f
        )
        {
            c.golpeo = true;
            t.inmunidad = 1.0f;
            t.ultimaTecla = 0;
            AplicarCaidaMastil(m, i, CAIDA_CUERVO_MASTIL);
            RafagaMastil(m, m.posicionX[i], t.altura + 0.8f, PROFUNDIDAD_JUGADOR_MASTIL, Color{ 40, 40, 50, 255 }, 10);
        }

        if (std::fabs(c.x) > RECORRIDO_CUERVO_MASTIL + 1.0f)
        {
            c.activo = false;
            c.proximo = AleatorioMastil(4.0f, 7.0f);
        }
    }
}


//==================================================
// LOGICA: FIN DE PARTIDA
//==================================================

static void FinalizarPartidaMastil(MinijuegoTrepaMastil& m, int limite)
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
            if (j != i && m.resultado.participantes[j].participo && m.trepadores[j].altura > m.trepadores[i].altura)
            {
                posicion++;
            }
        }

        r.posicionFinal = posicion;
        r.puntuacionMinijuego = (int)(m.trepadores[i].altura * 10.0f);
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
    m.fase = FASE_MASTIL_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// BOTS
//==================================================

static InputMinijuegoParticipante CrearEntradaBotMastil(
    MinijuegoTrepaMastil& m,
    int indice,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoBotMastil& bot = m.bots[indice];
    const TrepadorMastil& t = m.trepadores[indice];

    bot.cambioFrecuencia -= deltaTime;

    if (bot.cambioFrecuencia <= 0.0f)
    {
        bot.frecuencia = AleatorioMastil(4.0f, 7.0f);
        bot.cambioFrecuencia = AleatorioMastil(0.8f, 1.8f);
    }

    int efecto = EfectoInclinacionMastil(m, indice);
    float frecuencia = bot.frecuencia;

    if (bot.aprovechaInclinacion)
    {
        // Se detiene a recuperar el aliento cuando el mastil va en contra.
        if (efecto < 0)
        {
            frecuencia = 0.0f;
        }
        else if (efecto > 0)
        {
            frecuencia *= 1.3f;
        }
    }

    if (frecuencia <= 0.0f)
    {
        bot.proximoToque = 0.15f;
        return entrada;
    }

    bot.proximoToque -= deltaTime;

    if (bot.proximoToque <= 0.0f)
    {
        bot.proximoToque += AleatorioMastil(0.8f, 1.25f) / frecuencia;

        if (bot.proximoToque < 0.0f)
        {
            bot.proximoToque = 0.0f;
        }

        int tecla = t.ultimaTecla == 1 ? 2 : 1;

        // Error ocasional: repite la misma mano.
        if (t.ultimaTecla != 0 && GetRandomValue(0, 99) < 8)
        {
            tecla = t.ultimaTecla;
        }

        entrada.saltar = tecla == 1;
        entrada.golpear = tecla == 2;
    }

    return entrada;
}


//==================================================
// CICLO DE VIDA
//==================================================

void MinijuegoTrepaMastil::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        trepadores[i] = {};
        cuervos[i] = {};
        cuervos[i].proximo = AleatorioMastil(0.0f, 2.5f);
        bots[i] = {};
        bots[i].frecuencia = AleatorioMastil(4.0f, 7.0f);
        bots[i].proximoToque = AleatorioMastil(0.0f, 0.2f);
        bots[i].aprovechaInclinacion = GetRandomValue(0, 1) == 0;
        posicionX[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS_MASTIL; i++)
    {
        particulas[i] = {};
    }

    camara.position = { 0.0f, 6.4f, 19.0f };
    camara.target = { 0.0f, 5.8f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 54.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_MASTIL_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_MASTIL;
    tiempoRestante = DURACION_PARTIDA_MASTIL;
    tiempoAnimacion = 0.0f;
    inclinacion = 0.0f;
    intensidadOlas = 0.0f;
    faseOlas = AleatorioMastil(0.0f, 2.0f * PI);
}


void MinijuegoTrepaMastil::Reiniciar(
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
        fase = FASE_MASTIL_TERMINADO;
        return;
    }

    int limite = LimiteMastil(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        posicionX[i] = ((float)k - (float)(cantidad - 1) * 0.5f) * SEPARACION_MASTILES_MASTIL;
        trepadores[i].lado = (k % 2 == 0) ? -1 : 1;

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], { posicionX[i], 0.7f, PROFUNDIDAD_JUGADOR_MASTIL });
        jugadores[i].direccionMirada = { 0.0f, 0.0f, 1.0f };
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoTrepaMastil::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_MASTIL, deltaTime);

    if (
        fase == FASE_MASTIL_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    int limite = LimiteMastil(cantidadMaxima);

    // El personaje se agarra al mastil: se desplaza hacia la mano que agarra.
    auto sincronizarJugadores = [&]()
    {
        for (int i = 0; i < limite; i++)
        {
            if (!participantes[i].activo)
            {
                continue;
            }

            JugadorPrueba& jugador = jugadores[i];
            const TrepadorMastil& t = trepadores[i];
            float bamboleo = LimitarMastil(t.pulso, 0.0f, 1.0f);

            jugador.posicion =
            {
                posicionX[i] + t.mano * 0.12f * bamboleo,
                t.altura + 0.7f + 0.05f * bamboleo,
                PROFUNDIDAD_JUGADOR_MASTIL
            };
            jugador.velocidad = {};
            jugador.empuje = {};
            jugador.enSuelo = true;
            jugador.cayendo = false;
            jugador.direccionMirada = { 0.0f, 0.0f, 1.0f };
        }
    };

    if (fase == FASE_MASTIL_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_MASTIL_JUGANDO;
        }

        sincronizarJugadores();
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    float transcurrido = DURACION_PARTIDA_MASTIL - tiempoRestante;
    ActualizarOlasMastil(*this, transcurrido);

    bool alguienLlego = false;

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
            entrada = CrearEntradaBotMastil(*this, i, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        ActualizarTrepadorMastil(*this, i, entrada, !controlBot, deltaTime);

        if (trepadores[i].altura >= ALTURA_META_MASTIL && !trepadores[i].llego)
        {
            trepadores[i].llego = true;
            alguienLlego = true;
        }
    }

    ActualizarCuervosMastil(*this, limite, transcurrido, deltaTime);
    sincronizarJugadores();

    if (alguienLlego)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
        FinalizarPartidaMastil(*this, limite);
        return;
    }

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarPartidaMastil(*this, limite);
    }
}


//==================================================
// VISUAL: ESCENARIO (solo decoracion, la logica no depende de esto)
//==================================================

// MODELO FUTURO: casco y cubierta del barco, mastiles con cofa y bandera,
// velas, jarcias, barandilla, cofres y barriles, gaviotas, cuervos, isla
// lejana con palmera y olas del mar: reemplazar cada uno por un GLB.

static float HashMastil(int semilla)
{
    unsigned int x = (unsigned int)semilla * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return (float)(x & 0xFFFFu) / 65535.0f;
}


static void DibujarMarMastil(const MinijuegoTrepaMastil& m)
{
    float t = m.tiempoAnimacion;
    float agitado = 0.4f + 0.6f * m.intensidadOlas;

    // Mar y horizonte.
    DrawCube({ 0.0f, -4.6f, -20.0f }, 120.0f, 0.2f, 90.0f, Color{ 22, 84, 150, 255 });

    for (int k = 0; k < 30; k++)
    {
        float x = std::fmod(HashMastil(k) * 90.0f + t * (1.0f + HashMastil(k + 40)), 90.0f) - 45.0f;
        float z = -4.0f - HashMastil(k + 80) * 40.0f;
        float alto = 0.15f + 0.2f * agitado * (0.5f + 0.5f * std::sin(t * 2.0f + (float)k));
        DrawCube({ x, -4.4f + alto * 0.5f, z }, 2.5f + HashMastil(k + 120) * 2.0f, alto, 0.3f, Fade(WHITE, 0.55f));
    }

    // Isla lejana con palmera.
    DrawCylinder({ -22.0f, -4.5f, -42.0f }, 2.0f, 9.0f, 3.0f, 12, Color{ 214, 190, 120, 255 });
    DrawCylinder({ -22.0f, -1.5f, -42.0f }, 0.0f, 2.2f, 2.5f, 10, Color{ 60, 140, 70, 255 });
    DrawCylinderEx({ -21.0f, -1.5f, -42.0f }, { -21.4f, 2.0f, -42.0f }, 0.18f, 0.12f, 6, Color{ 110, 80, 50, 255 });

    for (int k = 0; k < 5; k++)
    {
        float angulo = (float)k * 1.25f;
        DrawLine3D(
            { -21.4f, 2.0f, -42.0f },
            { -21.4f + std::cos(angulo) * 1.6f, 1.5f, -42.0f + std::sin(angulo) * 1.6f },
            Color{ 50, 150, 60, 255 }
        );
    }

    // Gaviotas.
    for (int g = 0; g < 4; g++)
    {
        float x = std::fmod(t * (1.5f + 0.3f * (float)g) + (float)g * 13.0f, 50.0f) - 25.0f;
        float y = 9.0f + (float)g * 1.6f + std::sin(t + (float)g) * 0.5f;
        float z = -14.0f - (float)g * 3.0f;
        float aleteo = std::sin(t * 6.0f + (float)g * 2.0f) * 0.4f;

        DrawLine3D({ x - 0.7f, y + aleteo, z }, { x, y, z }, WHITE);
        DrawLine3D({ x, y, z }, { x + 0.7f, y + aleteo, z }, WHITE);
    }
}


static void DibujarCubiertaMastil(const MinijuegoTrepaMastil& m)
{
    float ancho = 18.0f;

    // Casco y cubierta.
    DrawCube({ 0.0f, -1.5f, 0.0f }, ancho, 3.0f, 4.6f, Color{ 92, 58, 34, 255 });
    DrawCube({ 0.0f, 0.0f, 0.0f }, ancho, 0.12f, 4.6f, Color{ 160, 112, 70, 255 });

    for (int k = -4; k <= 4; k++)
    {
        DrawCube({ 0.0f, 0.07f, (float)k * 0.5f }, ancho, 0.02f, 0.03f, Color{ 100, 68, 40, 255 });
    }

    DrawCube({ 0.0f, -0.6f, 2.35f }, ancho, 0.1f, 0.1f, Color{ 220, 180, 80, 255 });

    // Barandilla delantera.
    for (int k = -9; k <= 9; k++)
    {
        DrawCube({ (float)k * 1.0f, 0.4f, 2.25f }, 0.1f, 0.8f, 0.1f, Color{ 120, 82, 50, 255 });
    }

    DrawCube({ 0.0f, 0.82f, 2.25f }, ancho, 0.1f, 0.14f, Color{ 140, 96, 58, 255 });

    // Cofres con candado dorado y barriles en los extremos.
    const float cofresX[2] = { -8.2f, 8.2f };

    for (int c = 0; c < 2; c++)
    {
        DrawCube({ cofresX[c], 0.35f, 0.6f }, 1.2f, 0.7f, 0.8f, Color{ 110, 70, 40, 255 });
        DrawCube({ cofresX[c], 0.78f, 0.6f }, 1.2f, 0.2f, 0.8f, Color{ 130, 84, 48, 255 });
        DrawCube({ cofresX[c], 0.5f, 1.02f }, 0.18f, 0.22f, 0.05f, GOLD);
        DrawCube({ cofresX[c], 0.35f, 1.02f }, 1.22f, 0.06f, 0.04f, Color{ 220, 180, 80, 255 });
    }

    DrawCylinder({ -7.0f, 0.0f, 0.9f }, 0.4f, 0.4f, 0.8f, 10, Color{ 120, 80, 48, 255 });
    DrawCylinder({ -7.7f, 0.0f, 1.2f }, 0.35f, 0.35f, 0.7f, 10, Color{ 110, 74, 44, 255 });
    DrawCylinder({ 7.1f, 0.0f, 1.0f }, 0.4f, 0.4f, 0.8f, 10, Color{ 120, 80, 48, 255 });
    (void)m;
}


static void DibujarMastilMastil(
    const MinijuegoTrepaMastil& m,
    int i,
    Color color
)
{
    float x = m.posicionX[i];
    float t = m.tiempoAnimacion;
    const float alto = ALTURA_META_MASTIL + 1.6f;

    // Vela a un lado con franja del color del jugador.
    float vaiven = std::sin(t * 1.5f + (float)i) * 0.05f;
    DrawCube({ x, 6.0f, -0.7f }, 3.2f, 5.6f, 0.08f, Color{ 236, 228, 200, 255 });
    DrawCube({ x + vaiven, 6.0f, -0.62f }, 3.2f, 0.5f, 0.05f, color);
    DrawCube({ x, 3.6f, -0.65f }, 3.2f, 0.04f, 0.04f, Color{ 150, 140, 110, 255 });
    DrawCube({ x, 8.2f, -0.65f }, 3.2f, 0.04f, 0.04f, Color{ 150, 140, 110, 255 });

    // Mastil con travesano y clavijas para agarrarse.
    DrawCylinder({ x, 0.0f, 0.0f }, 0.2f, 0.26f, alto, 8, Color{ 120, 80, 48, 255 });
    DrawCube({ x, 8.8f, -0.1f }, 3.0f, 0.12f, 0.12f, Color{ 110, 74, 44, 255 });

    for (int k = 1; k < (int)ALTURA_META_MASTIL; k++)
    {
        float lado = (k % 2 == 0) ? -1.0f : 1.0f;
        DrawCube({ x + lado * 0.26f, (float)k, 0.1f }, 0.22f, 0.07f, 0.12f, Color{ 70, 46, 28, 255 });
    }

    // Jarcias a cubierta.
    DrawLine3D({ x, 9.4f, 0.0f }, { x - 1.9f, 0.1f, 1.4f }, Color{ 190, 170, 120, 255 });
    DrawLine3D({ x, 9.4f, 0.0f }, { x + 1.9f, 0.1f, 1.4f }, Color{ 190, 170, 120, 255 });
    DrawLine3D({ x, 9.4f, 0.0f }, { x - 1.9f, 0.1f, -1.2f }, Color{ 190, 170, 120, 255 });
    DrawLine3D({ x, 9.4f, 0.0f }, { x + 1.9f, 0.1f, -1.2f }, Color{ 190, 170, 120, 255 });

    // Cofa en la meta con bandera del jugador.
    DrawCylinder({ x, ALTURA_META_MASTIL, 0.0f }, 0.85f, 0.7f, 0.2f, 12, Color{ 140, 96, 58, 255 });
    DrawCircle3D({ x, ALTURA_META_MASTIL + 0.22f, 0.0f }, 0.85f, { 1.0f, 0.0f, 0.0f }, 90.0f, GOLD);
    DrawCylinderEx({ x, ALTURA_META_MASTIL + 0.2f, 0.0f }, { x, ALTURA_META_MASTIL + 1.7f, 0.0f }, 0.04f, 0.04f, 6, Color{ 90, 60, 36, 255 });

    for (int k = 0; k < 4; k++)
    {
        float fx = x + 0.12f + (float)k * 0.2f;
        float onda = std::sin(t * 6.0f - (float)k * 0.9f + (float)i) * 0.06f * (float)(k + 1);
        DrawCube({ fx, ALTURA_META_MASTIL + 1.45f, onda }, 0.2f, 0.45f, 0.04f, color);
    }
}


static void DibujarCuervoMastil(
    const MinijuegoTrepaMastil& m,
    int i
)
{
    const CuervoMastil& c = m.cuervos[i];

    if (!c.activo || c.enAviso)
    {
        return;
    }

    float x = m.posicionX[i] + c.x;
    float y = c.altura;
    float z = PROFUNDIDAD_JUGADOR_MASTIL + 0.2f;
    float aleteo = std::sin(c.fase * 22.0f) * 0.45f;
    float d = c.direccion;

    DrawSphere({ x, y, z }, 0.3f, Color{ 30, 30, 40, 255 });
    DrawSphere({ x + d * 0.3f, y + 0.1f, z }, 0.18f, Color{ 36, 36, 48, 255 });
    DrawCylinderEx({ x + d * 0.42f, y + 0.08f, z }, { x + d * 0.7f, y + 0.02f, z }, 0.07f, 0.0f, 5, ORANGE);
    DrawSphere({ x + d * 0.38f, y + 0.18f, z + 0.1f }, 0.04f, RED);
    DrawLine3D({ x, y, z }, { x - d * 0.1f, y + aleteo, z - 0.7f }, Color{ 20, 20, 28, 255 });
    DrawLine3D({ x, y, z }, { x - d * 0.1f, y + aleteo, z + 0.7f }, Color{ 20, 20, 28, 255 });
    DrawLine3D({ x, y, z }, { x - d * 0.6f, y - 0.1f, z }, Color{ 20, 20, 28, 255 });
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoTrepaMastil::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteMastil(cantidadMaxima);
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    float angulo = AnguloBarcoMastil(inclinacion);

    ClearBackground(Color{ 120, 180, 225, 255 });
    DrawRectangleGradientV(0, 0, anchoPantalla, altoPantalla, Color{ 70, 130, 205, 255 }, Color{ 190, 225, 240, 255 });
    DrawCircle(anchoPantalla - 150, 150, 46.0f, Color{ 255, 244, 190, 255 });

    BeginMode3D(camara);

    DibujarMarMastil(*this);

    // Todo lo que pertenece al barco se balancea junto con la cubierta.
    rlPushMatrix();
    rlRotatef(angulo * RAD2DEG, 0.0f, 0.0f, 1.0f);

    DibujarCubiertaMastil(*this);

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        DibujarMastilMastil(*this, i, participantes[i].color);
        DibujarCuervoMastil(*this, i);

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_MASTIL);

    rlPopMatrix();

    EndMode3D();

    // Avisos de cuervo y etiquetas (se proyectan con la rotacion del barco).
    float coseno = std::cos(angulo);
    float seno = std::sin(angulo);

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const CuervoMastil& c = cuervos[i];

        if (c.activo && c.enAviso)
        {
            float lx = posicionX[i] - c.direccion * 2.6f;
            float ly = c.altura;
            Vector2 pantalla = GetWorldToScreen({ lx * coseno - ly * seno, lx * seno + ly * coseno, 1.0f }, camara);

            if (std::fmod(tiempoAnimacion * 8.0f, 1.0f) < 0.6f)
            {
                DrawRectangle((int)pantalla.x - 16, (int)pantalla.y - 18, 32, 36, Fade(BLACK, 0.7f));
                DrawText("!", (int)pantalla.x - 5, (int)pantalla.y - 16, 32, RED);
            }
        }

        const TrepadorMastil& t = trepadores[i];
        float lx = posicionX[i] + 0.9f;
        float ly = t.altura + 0.7f;
        Vector2 pantalla = GetWorldToScreen({ lx * coseno - ly * seno, lx * seno + ly * coseno, 1.0f }, camara);
        const char* nombre = NombreJugadorMastil(participantes[i], i);
        int ancho = MeasureText(nombre, 18);

        DrawRectangle((int)pantalla.x - 4, (int)pantalla.y - 2, ancho + 8, 22, Fade(BLACK, 0.55f));
        DrawText(nombre, (int)pantalla.x, (int)pantalla.y, 18, participantes[i].color);
    }

    // Cabecera.
    DrawRectangle(14, 12, 560, 92, Fade(BLACK, 0.74f));
    DrawText("TREPA EL MASTIL", 28, 20, 28, GOLD);
    DrawText("ALTERNA SALTO Y ACCION PARA TREPAR: LA MISMA TECLA RESBALA", 28, 54, 15, RAYWHITE);
    DrawText("INCLINACION A TU FAVOR = BONUS   CUERVO (!) = CAES 1 M", 28, 76, 14, LIGHTGRAY);

    if (fase == FASE_MASTIL_JUGANDO)
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

    // Indicador de inclinacion del barco.
    if (fase != FASE_MASTIL_PREPARACION)
    {
        int centroX = anchoPantalla / 2;
        int ancho = 280;

        DrawRectangle(centroX - ancho / 2 - 8, 12, ancho + 16, 46, Fade(BLACK, 0.7f));
        DrawText(
            intensidadOlas > 0.05f ? "OLAS: BARCO INCLINADO" : "MAR EN CALMA",
            centroX - MeasureText(intensidadOlas > 0.05f ? "OLAS: BARCO INCLINADO" : "MAR EN CALMA", 14) / 2,
            16,
            14,
            intensidadOlas > 0.05f ? Color{ 120, 220, 255, 255 } : LIGHTGRAY
        );
        DrawRectangle(centroX - ancho / 2, 38, ancho, 8, Fade(GRAY, 0.6f));
        DrawRectangle(centroX - 1, 34, 2, 16, WHITE);
        DrawRectangle(centroX + (int)(inclinacion * (float)ancho * 0.5f) - 6, 33, 12, 18, Color{ 120, 220, 255, 255 });
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
            const TrepadorMastil& t = trepadores[i];

            DrawRectangle(x, y, anchoTarjeta, 80, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 80, participantes[i].color);
            DrawText(NombreJugadorMastil(participantes[i], i), x + 18, y + 8, 20, participantes[i].color);
            DrawText(TextFormat("%.1f M", t.altura), x + 18, y + 32, 22, RAYWHITE);

            // Progreso hacia la cofa.
            float progreso = LimitarMastil(t.altura / ALTURA_META_MASTIL, 0.0f, 1.0f);
            DrawRectangle(x + 100, y + 36, anchoTarjeta - 116, 10, Fade(GRAY, 0.5f));
            DrawRectangle(x + 100, y + 36, (int)((float)(anchoTarjeta - 116) * progreso), 10, participantes[i].color);

            const char* estado = "";
            Color colorEstado = LIGHTGRAY;
            int efecto = EfectoInclinacionMastil(*this, i);

            if (t.inmunidad > 0.4f)
            {
                estado = "CUERVO!";
                colorEstado = RED;
            }
            else if (fase == FASE_MASTIL_JUGANDO && intensidadOlas > 0.05f)
            {
                if (efecto > 0)
                {
                    estado = "A FAVOR x1.6";
                    colorEstado = LIME;
                }
                else if (efecto < 0)
                {
                    estado = "EN CONTRA";
                    colorEstado = ORANGE;
                }
            }

            DrawText(estado, x + anchoTarjeta - 110, y + 10, 14, colorEstado);

            if (participantes[i].esBot)
            {
                DrawText("CONTROL: BOT", x + 18, y + 60, 14, LIGHTGRAY);
            }
            else
            {
                DrawText(
                    TextFormat("ALTERNA %s / %s", ObtenerTextoBotonPrincipal(participantes[i]), TextoAccionMastil(participantes[i])),
                    x + 18,
                    y + 60,
                    14,
                    LIGHTGRAY
                );
            }

            k++;
        }
    }

    if (fase == FASE_MASTIL_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 110, 96, GOLD);

        const char* ayuda = "ALTERNA SALTO Y ACCION: UNA MANO Y LA OTRA";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 - 10, 20, RAYWHITE);
    }
    else if (
        fase == FASE_MASTIL_TERMINADO &&
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
            titulo = TextFormat("GANA %s", NombreJugadorMastil(participantes[ganadores[0]], ganadores[0]));
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
                        "%d.  %s   %.1f M%s",
                        posicion,
                        NombreJugadorMastil(participantes[i], i),
                        trepadores[i].altura,
                        trepadores[i].llego ? "   LLEGO A LA COFA" : ""
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


const ResultadoMinijuego& MinijuegoTrepaMastil::ObtenerResultado() const
{
    return resultado;
}
