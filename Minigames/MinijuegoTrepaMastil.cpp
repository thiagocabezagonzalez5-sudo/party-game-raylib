#include "Minigames/MinijuegoTrepaMastil.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HorneadoMallas.h"
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


static const char* TextoAccionMastil(const Participante& participante)
{
    if (participante.control == CONTROL_GAMEPAD)
    {
        return "X";
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

static void LiberarMallasMastil();


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

    // Visual: el barco horneado se rehace (cambian los mastiles o la calidad).
    LiberarMallasMastil();
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
// VISUAL: CALETA PIRATA AL ATARDECER (solo decoracion; la logica no depende de esto)
//==================================================
//
// Un galeon amarrado en una caleta al caer el sol. Cubierta de tablones
// con brea entre juntas, borda verde azulada con friso dorado y portas
// cerradas; toldilla a popa con la camara del capitan iluminada, farol de
// popa y bandera pirata; castillo de proa con campana, ancla y baupres.
// Cada jugador trepa su mastil: velas recogidas en las vergas, jarcias con
// flechates, marcas de altura cada metro (bandas de bronce a los 5 m), una
// cinta de su color que sube con el y la cofa con friso dorado y su
// bandera. Alrededor, el mar calmo de la caleta refleja el cielo naranja y
// rosado; a la izquierda un fuerte de piedra sobre el promontorio con
// antorchas, a la derecha un pueblo con faroles, muelle y botes; al fondo
// dos planos de sierras en la bruma. Gaviotas, nubes alargadas y el sol
// que se hunde en la boca de la caleta. Lo estatico se hornea en mallas
// (cielo, costa, agua y barco con sus mastiles) y se rehace al cambiar la
// calidad o la cantidad de mastiles.
//
// MODELO FUTURO: galeon (casco, toldilla, castillo de proa), mastil con
// cofa y vergas, cuervo, gaviota, cofre, barril, cañon de cubierta, timon,
// fuerte de la costa, casas del pueblo y bote de remos.

static const Color CIELO_ALTO_TM = { 50, 42, 104, 255 };
static const Color CIELO_MEDIO_TM = { 150, 74, 128, 255 };
static const Color CIELO_ROSA_TM = { 226, 112, 124, 255 };
static const Color CIELO_HORIZONTE_TM = { 255, 160, 98, 255 };
static const Color SOL_TM = { 255, 238, 186, 255 };
static const Color NIEBLA_TM = { 218, 132, 128, 255 };       // bruma rosada
static const Color MAR_HONDO_TM = { 28, 38, 74, 255 };
static const Color MADERA_CUBIERTA_TM = { 176, 124, 80, 255 };
static const Color MADERA_MEDIA_TM = { 128, 80, 50, 255 };
static const Color MADERA_OSCURA_TM = { 84, 50, 36, 255 };
static const Color CASCO_PINTADO_TM = { 40, 92, 100, 255 };  // verde azulado
static const Color BREA_TM = { 36, 30, 36, 255 };
static const Color DORADO_TM = { 232, 182, 78, 255 };
static const Color LONA_TM = { 232, 216, 182, 255 };
static const Color SOGA_TM = { 186, 156, 106, 255 };
static const Color HIERRO_TM = { 58, 58, 66, 255 };
static const Color PIEDRA_TM = { 182, 140, 114, 255 };
static const Color VERDE_TM = { 64, 92, 62, 255 };
static const Color ROJO_TAPA_TM = { 142, 46, 40, 255 };
static const Color LUZ_FAROL_TM = { 255, 184, 100, 255 };
static const Color CUERVO_TM = { 30, 28, 40, 255 };
static const Color PELIGRO_TM = { 255, 76, 62, 255 };       // acento: cuervo
static const Color FAVOR_TM = { 140, 255, 150, 255 };
static const Color CONTRA_TM = { 255, 168, 80, 255 };

// Ojo de referencia para la bruma horneada (la camara de la logica).
static const Vector3 OJO_TM = { 0.0f, 6.4f, 19.0f };

// Medidas del barco (marco del barco: gira con la inclinacion).
static const float MAR_Y_TM = -1.3f;
static const float MANGA_TM = 2.55f;          // media manga en el combes
static const float POPA_X_TM = -12.6f;
static const float TOLDILLA_X_TM = -9.4f;     // frente de la toldilla
static const float TOLDILLA_Y_TM = 1.3f;
static const float CASTILLO_X_TM = 9.7f;      // frente del castillo de proa
static const float CASTILLO_Y_TM = 0.95f;
static const float PROA_X_TM = 14.6f;
static const float RADIO_MASTIL_TM = 0.2f;
static const float COFA_RADIO_TM = 0.9f;
static const float COFA_PARED_TM = 0.5f;
static const float TOPE_MASTIL_TM = 12.6f;

// Sol: bajo, en la boca de la caleta (algo a la derecha del centro).
static const Vector3 SOL_DIR_TM = { 0.2427f, 0.0853f, -0.9663f };


static float Ruido01TM(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static GrabadorMalla g_grabTM;
static Vector3 g_ojoTM = OJO_TM;
static Vector3 g_derTM = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaTM = { 0.0f, 1.0f, 0.0f };
static float g_emisTM = 0.0f;              // 1 = luminoso o cielo: sin bruma


static float Lim01TM(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalTM(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoTM(Color c, float f)
{
    return { CanalTM((float)c.r * f), CanalTM((float)c.g * f), CanalTM((float)c.b * f), c.a };
}


static Color MezclaTM(Color a, Color b, float t)
{
    t = Lim01TM(t);
    return {
        CanalTM((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalTM((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalTM((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalTM((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaTM(Color c, float alfa)
{
    c.a = CanalTM((float)c.a * Lim01TM(alfa));
    return c;
}


static Color VariarTM(Color c, int indice, int semilla, float amplitud)
{
    return TonoTM(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01TM(indice, semilla));
}


// Bruma del atardecer: lo lejano se funde con el rosado del horizonte (y
// con el violeta del cielo si esta alto).
static Color NieblaTM(Color c, Vector3 p)
{
    float dx = p.x - g_ojoTM.x, dy = p.y - g_ojoTM.y, dz = p.z - g_ojoTM.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01TM((d - 30.0f) / 210.0f);
    n = n * (2.0f - n) * 0.82f * (1.0f - g_emisTM);
    Color bruma = MezclaTM(NIEBLA_TM, CIELO_ROSA_TM, Lim01TM(p.y / 45.0f));
    Color r = MezclaTM(c, bruma, n);
    r.a = c.a;
    return r;
}


static void VTM(Vector3 p, Color c)
{
    if (g_grabTM.activa)
    {
        GrabarVerticeMalla(g_grabTM, p, c);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteTM()
{
    if (!g_grabTM.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteTM()
{
    if (!g_grabTM.activa) rlEnd();
}


static void TriTM(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VTM(a, NieblaTM(color, a));
    VTM(b, NieblaTM(color, b));
    VTM(c, NieblaTM(color, c));
}


static void QuadTM(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriTM(a, b, c, color);
    TriTM(a, c, d, color);
}


static void Quad4TM(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VTM(a, NieblaTM(ca, a)); VTM(b, NieblaTM(cb, b)); VTM(c, NieblaTM(cc, c));
    VTM(a, NieblaTM(ca, a)); VTM(c, NieblaTM(cc, c)); VTM(d, NieblaTM(cd, d));
}


// Quad sin bruma con un color por vertice (luces aditivas y sombras).
static void QuadColTM(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VTM(a, ca); VTM(b, cb); VTM(c, cc);
    VTM(a, ca); VTM(c, cc); VTM(d, cd);
}


// Sombreado por caras de la guia (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62) interpolado para normales cualesquiera.
static float LuzTM(Vector3 n)
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


// Luz del atardecer: lo que mira hacia arriba y hacia el sol toma el
// dorado; lo que queda de espaldas, el lila de la sombra.
static Color CaraTM(Color c, Vector3 n)
{
    float f = LuzTM(n);
    float calido = Lim01TM(0.45f + 0.45f * n.y + 0.35f * (n.x * SOL_DIR_TM.x - n.z * 0.35f));
    float r = f * (0.88f + 0.2f * calido);
    float g = f * (0.86f + 0.1f * calido);
    float b = f * (1.0f - 0.16f * calido);
    return { CanalTM((float)c.r * r), CanalTM((float)c.g * g), CanalTM((float)c.b * b), c.a };
}


// Caja con medio-ejes arbitrarios: caras sombreadas por normal.
static void CajaEjesTM(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
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

    QuadTM(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraTM(color, ny));
    QuadTM(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraTM(color, nz));
    QuadTM(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraTM(color, Vector3Negate(nz)));
    QuadTM(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraTM(color, Vector3Negate(nx)));
    QuadTM(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraTM(color, nx));

    if (fondo)
    {
        QuadTM(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraTM(color, Vector3Negate(ny)));
    }
}


static void CajaTM(Vector3 c, Vector3 t, Color color)
{
    CajaEjesTM(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, false);
}


// Caja girada en Y (grados).
static void CajaGiradaTM(Vector3 c, Vector3 t, float giroY, Color color)
{
    float a = giroY * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };
    CajaEjesTM(c, Vector3Scale(ux, t.x * 0.5f), { 0.0f, t.y * 0.5f, 0.0f }, Vector3Scale(uz, t.z * 0.5f), color, false);
}


// Viga de seccion rectangular entre dos puntos (ancho x alto).
static void VigaTM(Vector3 a, Vector3 b, float ancho, float alto, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesTM(centro, Vector3Scale(v, ancho * 0.5f), Vector3Scale(w, alto * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas.
static void CilTM(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
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

        QuadTM(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraTM(color, nm)
        );

        if (tapas)
        {
            TriTM(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraTM(color, eje));
            TriTM(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraTM(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide (o media cupula con desde = 0) con caras sombreadas.
static void BolaTM(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float desde = -0.5f * PI)
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
            QuadTM(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraTM(color, n));
        }
    }
}


// Disco horizontal con color de centro y de borde (sin bruma si se pide).
static void DiscoTM(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, bool bruma = true)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };

        if (bruma)
        {
            VTM(c, NieblaTM(centro, c)); VTM(p1, NieblaTM(borde, p1)); VTM(p0, NieblaTM(borde, p0));
        }
        else
        {
            VTM(c, centro); VTM(p1, borde); VTM(p0, borde);
        }
    }
}


// Anillo horizontal entre r0 y r1 (sin bruma: marcas y luces).
static void AnilloTM(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColTM(i0, i1, o1, o0, dentro, dentro, fuera, fuera);
    }
}


// Disco que mira a la camara con centro y borde de distinto color (halos).
static void HaloTM(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derTM, std::cos(a0) * radio), Vector3Scale(g_arribaTM, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derTM, std::cos(a1) * radio), Vector3Scale(g_arribaTM, std::sin(a1) * radio)));
        VTM(c, centro); VTM(p0, borde); VTM(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas).
static void PuntoTM(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derTM, r), u = Vector3Scale(g_arribaTM, r);
    QuadColTM(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Tira que mira a la camara entre dos puntos (sogas, haces, estelas).
static void TiraTM(Vector3 a, Vector3 b, float anchoA, float anchoB, Color ca, Color cb)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 haciaOjo = Vector3Subtract(g_ojoTM, a);
    Vector3 lado = Vector3CrossProduct(eje, haciaOjo);
    float largo = Vector3Length(lado);

    if (largo < 0.00001f) return;

    lado = Vector3Scale(lado, 1.0f / largo);
    Vector3 la = Vector3Scale(lado, anchoA * 0.5f), lb = Vector3Scale(lado, anchoB * 0.5f);
    QuadColTM(Vector3Subtract(a, la), Vector3Add(a, la), Vector3Add(b, lb), Vector3Subtract(b, lb), ca, ca, cb, cb);
}


// Soga horneada: tira fija hacia el ojo de referencia, con bruma.
static void SogaTM(Vector3 a, Vector3 b, float ancho, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 haciaOjo = Vector3Subtract(OJO_TM, a);
    Vector3 lado = Vector3CrossProduct(eje, haciaOjo);
    float largo = Vector3Length(lado);

    if (largo < 0.00001f) return;

    lado = Vector3Scale(lado, ancho * 0.5f / largo);
    QuadTM(Vector3Subtract(a, lado), Vector3Add(a, lado), Vector3Add(b, lado), Vector3Subtract(b, lado), color);
}


// Rectangulo horizontal (x0..x1, z0..z1) a la altura y.
static void PisoTM(float x0, float z0, float x1, float z1, float y, Color c)
{
    QuadTM({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, c);
}


// Rectangulo vertical en el plano z (cara hacia +z).
static void PanelZTM(float x0, float y0, float x1, float y1, float z, Color abajo, Color arriba)
{
    Quad4TM({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, abajo, abajo, arriba, arriba);
}


//--------------------------------------------------
// Horno: lo estatico se graba una vez en mallas
//--------------------------------------------------

struct HornoTM
{
    bool listo = false;
    int clave = -1;
    Mesh cielo{};          // cupula, nubes y sol, relativos al ojo
    bool cieloValido = false;
    Mesh costa{};          // sierras, fuerte, pueblo, muelle
    bool costaValida = false;
    Mesh agua{};
    bool aguaValida = false;
    Mesh barco{};          // casco, cubierta, mastiles y utileria
    bool barcoValido = false;
    bool materialListo = false;
    Material material{};
};

static HornoTM g_hornoTM;


static void LiberarMallasMastil()
{
    LiberarMallaHorneada(g_hornoTM.cielo, g_hornoTM.cieloValido);
    LiberarMallaHorneada(g_hornoTM.costa, g_hornoTM.costaValida);
    LiberarMallaHorneada(g_hornoTM.agua, g_hornoTM.aguaValida);
    LiberarMallaHorneada(g_hornoTM.barco, g_hornoTM.barcoValido);
    g_hornoTM.listo = false;
    g_hornoTM.clave = -1;
}


static void CieloEstaticoTM(bool decoracion);
static void CostaEstaticaTM(bool decoracion, int paso);
static void AguaEstaticaTM(bool decoracion);
static void BarcoEstaticoTM(const MinijuegoTrepaMastil& m, int limite, bool decoracion, int paso);


// La clave cambia con la calidad y con los mastiles en juego.
static int ClaveHornoTM(const MinijuegoTrepaMastil& m, int limite)
{
    int clave = (int)CalidadDecoracion();

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;
        clave = clave * 31 + (int)std::lround(m.posicionX[i] * 10.0f) + 97;
    }

    return clave & 0x7FFFFFFF;
}


static void HornearTM(const MinijuegoTrepaMastil& m, int limite)
{
    int clave = ClaveHornoTM(m, limite);

    if ((g_hornoTM.listo && g_hornoTM.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasMastil();

    AsegurarMaterialHorneado(g_hornoTM.material, g_hornoTM.materialListo);

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    Vector3 ojo = g_ojoTM;
    g_ojoTM = OJO_TM;

    // El cielo se graba relativo al ojo y sin bruma.
    g_ojoTM = { 0.0f, 0.0f, 0.0f };
    g_emisTM = 1.0f;
    EmpezarGrabacionMalla(g_grabTM);
    CieloEstaticoTM(decoracion);
    g_hornoTM.cieloValido = TerminarGrabacionMalla(g_grabTM, g_hornoTM.cielo);
    g_emisTM = 0.0f;
    g_ojoTM = OJO_TM;

    EmpezarGrabacionMalla(g_grabTM);
    CostaEstaticaTM(decoracion, paso);
    g_hornoTM.costaValida = TerminarGrabacionMalla(g_grabTM, g_hornoTM.costa);

    EmpezarGrabacionMalla(g_grabTM);
    AguaEstaticaTM(decoracion);
    g_hornoTM.aguaValida = TerminarGrabacionMalla(g_grabTM, g_hornoTM.agua);

    EmpezarGrabacionMalla(g_grabTM);
    BarcoEstaticoTM(m, limite, decoracion, paso);
    g_hornoTM.barcoValido = TerminarGrabacionMalla(g_grabTM, g_hornoTM.barco);

    g_ojoTM = ojo;
    g_hornoTM.clave = clave;
    g_hornoTM.listo = true;
}


static void DibujarMallaTM(const Mesh& malla, bool valida, Matrix transformacion = MatrixIdentity())
{
    if (!g_hornoTM.listo || !valida) return;

    rlDrawRenderBatchActive();
    DrawMesh(malla, g_hornoTM.material, transformacion);
    rlDrawRenderBatchActive();
}


//--------------------------------------------------
// Cielo del atardecer (cupula horneada relativa al ojo)
//--------------------------------------------------

static const float RADIO_CIELO_TM = 460.0f;


static Color ColorCieloTM(Vector3 d)
{
    float el = std::asin(LimitarMastil(d.y, -1.0f, 1.0f)) * RAD2DEG;
    Color c;

    if (el < 0.0f) c = MezclaTM(CIELO_HORIZONTE_TM, NIEBLA_TM, Lim01TM(-el / 6.0f));
    else if (el < 7.0f) c = MezclaTM(CIELO_HORIZONTE_TM, CIELO_ROSA_TM, el / 7.0f);
    else if (el < 24.0f) c = MezclaTM(CIELO_ROSA_TM, CIELO_MEDIO_TM, (el - 7.0f) / 17.0f);
    else c = MezclaTM(CIELO_MEDIO_TM, CIELO_ALTO_TM, (el - 24.0f) / 40.0f);

    // Resplandor alrededor del sol (amplio y cercano).
    float s = Vector3DotProduct(d, SOL_DIR_TM);
    if (s > 0.0f)
    {
        float amplio = std::pow(s, 6.0f) * Lim01TM(1.0f - el / 50.0f);
        float cerca = std::pow(s, 60.0f);
        c = MezclaTM(c, Color{ 255, 190, 120, 255 }, amplio * 0.55f);
        c = MezclaTM(c, SOL_TM, cerca * 0.85f);
    }

    c.a = 255;
    return c;
}


static Vector3 DireccionTM(float azimutGrados, float elevacionGrados)
{
    float a = azimutGrados * DEG2RAD, e = elevacionGrados * DEG2RAD;
    return { std::cos(e) * std::sin(a), std::sin(e), -std::cos(e) * std::cos(a) };
}


static void CieloEstaticoTM(bool decoracion)
{
    static const float ELEVACIONES[] = { -14.0f, -3.0f, 0.0f, 1.5f, 3.5f, 6.0f, 9.0f, 13.0f, 18.0f, 25.0f, 34.0f, 46.0f, 62.0f, 90.0f };
    const int anillos = (int)(sizeof(ELEVACIONES) / sizeof(ELEVACIONES[0]));
    const int lados = decoracion ? 72 : 40;

    for (int i = 0; i + 1 < anillos; i++)
    {
        for (int k = 0; k < lados; k++)
        {
            float a0 = -180.0f + 360.0f * (float)k / (float)lados;
            float a1 = -180.0f + 360.0f * (float)(k + 1) / (float)lados;
            Vector3 d00 = DireccionTM(a0, ELEVACIONES[i]), d01 = DireccionTM(a1, ELEVACIONES[i]);
            Vector3 d10 = DireccionTM(a0, ELEVACIONES[i + 1]), d11 = DireccionTM(a1, ELEVACIONES[i + 1]);
            Quad4TM(
                Vector3Scale(d00, RADIO_CIELO_TM), Vector3Scale(d01, RADIO_CIELO_TM),
                Vector3Scale(d11, RADIO_CIELO_TM), Vector3Scale(d10, RADIO_CIELO_TM),
                ColorCieloTM(d00), ColorCieloTM(d01), ColorCieloTM(d11), ColorCieloTM(d10));
        }
    }

    // Disco del sol apenas sobre el horizonte.
    {
        Vector3 c = Vector3Scale(SOL_DIR_TM, RADIO_CIELO_TM * 0.97f);
        Vector3 der = Vector3Normalize(Vector3CrossProduct(SOL_DIR_TM, { 0.0f, 1.0f, 0.0f }));
        Vector3 arr = Vector3CrossProduct(der, SOL_DIR_TM);
        float r = RADIO_CIELO_TM * 0.032f;

        for (int k = 0; k < 28; k++)
        {
            float a0 = 2.0f * PI * (float)k / 28.0f, a1 = 2.0f * PI * (float)(k + 1) / 28.0f;
            Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(der, std::cos(a0) * r), Vector3Scale(arr, std::sin(a0) * r)));
            Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(der, std::cos(a1) * r), Vector3Scale(arr, std::sin(a1) * r)));
            Color borde = MezclaTM(SOL_TM, Color{ 255, 170, 90, 255 }, 0.55f + 0.45f * (0.5f - 0.5f * std::sin(a0)));
            VTM(c, Color{ 255, 250, 228, 255 }); VTM(p0, borde); VTM(p1, borde);
        }
    }

    // Nubes alargadas: panza encendida de naranja, lomo lila.
    int cantidad = decoracion ? 22 : 9;

    for (int n = 0; n < cantidad; n++)
    {
        float az = -95.0f + 190.0f * Ruido01TM(n, 11);
        float el = 4.0f + 30.0f * Ruido01TM(n, 12) * Ruido01TM(n, 13) + 2.0f;
        float ancho = 14.0f + 22.0f * Ruido01TM(n, 14);
        float alto = 1.2f + 1.6f * Ruido01TM(n, 15);
        float r = RADIO_CIELO_TM * 0.94f;
        Vector3 centro = DireccionTM(az, el);
        Vector3 der = Vector3Normalize(Vector3CrossProduct(centro, { 0.0f, 1.0f, 0.0f }));
        Vector3 arr = Vector3CrossProduct(der, centro);
        float cerca = Lim01TM(Vector3DotProduct(centro, SOL_DIR_TM));
        float calor = std::pow(cerca, 4.0f);
        Color panza = MezclaTM(Color{ 246, 140, 120, 235 }, Color{ 255, 214, 150, 245 }, calor);
        Color lomo = MezclaTM(Color{ 120, 70, 120, 220 }, Color{ 196, 104, 128, 230 }, calor * 0.7f);
        panza = MezclaTM(panza, Color{ 150, 86, 136, 220 }, Lim01TM((el - 14.0f) / 20.0f));
        int tramos = 7;

        for (int k = 0; k < tramos; k++)
        {
            float u0 = -1.0f + 2.0f * (float)k / (float)tramos, u1 = -1.0f + 2.0f * (float)(k + 1) / (float)tramos;
            float h0 = alto * (1.0f - u0 * u0) * (0.7f + 0.6f * Ruido01TM(n * 13 + k, 16));
            float h1 = alto * (1.0f - u1 * u1) * (0.7f + 0.6f * Ruido01TM(n * 13 + k + 1, 16));
            auto P = [&](float u, float v)
            {
                Vector3 q = Vector3Add(Vector3Scale(centro, r), Vector3Add(Vector3Scale(der, u * ancho), Vector3Scale(arr, v)));
                return q;
            };
            float base0 = -alto * 0.25f * (1.0f - u0 * u0), base1 = -alto * 0.25f * (1.0f - u1 * u1);
            Quad4TM(P(u0, base0), P(u1, base1), P(u1, h1 * 0.35f), P(u0, h0 * 0.35f), AlfaTM(panza, 0.0f), AlfaTM(panza, 0.0f), panza, panza);
            Quad4TM(P(u0, h0 * 0.35f), P(u1, h1 * 0.35f), P(u1, h1), P(u0, h0), panza, panza, AlfaTM(lomo, 0.0f), AlfaTM(lomo, 0.0f));
        }
    }
}


//--------------------------------------------------
// Agua de la caleta (abanico desde el ojo con el reflejo del cielo)
//--------------------------------------------------

static Color ColorAguaTM(Vector3 p)
{
    Vector3 v = Vector3Normalize(Vector3Subtract(p, OJO_TM));
    float rasante = 1.0f - std::fabs(v.y);
    float fresnel = 0.18f + 0.82f * rasante * rasante * rasante * rasante;
    Vector3 r = { v.x, std::fabs(v.y) * 0.55f, v.z };
    r = Vector3Normalize(r);
    Color cielo = ColorCieloTM(r);
    cielo = TonoTM(cielo, 0.86f);
    Color c = MezclaTM(MAR_HONDO_TM, cielo, fresnel);

    // Columna de reflejo del sol sobre el agua.
    float s = Vector3DotProduct(Vector3Normalize(Vector3{ v.x, -v.y, v.z }), SOL_DIR_TM);
    if (s > 0.0f)
    {
        float lateral = std::fabs(v.x / (-v.z + 0.0001f) - SOL_DIR_TM.x / -SOL_DIR_TM.z);
        float columna = Lim01TM(1.0f - lateral / 0.05f) * Lim01TM(rasante * 1.4f - 0.3f);
        c = MezclaTM(c, Color{ 255, 178, 100, 255 }, columna * 0.75f + std::pow(s, 30.0f) * 0.5f);
    }

    c.a = 255;
    return c;
}


static void AguaEstaticaTM(bool decoracion)
{
    static const float RADIOS[] = { 4.0f, 8.0f, 12.0f, 16.0f, 20.0f, 25.0f, 31.0f, 38.0f, 47.0f, 58.0f, 72.0f, 90.0f, 115.0f, 150.0f, 200.0f, 270.0f, 350.0f, 450.0f };
    const int filas = (int)(sizeof(RADIOS) / sizeof(RADIOS[0]));
    const int columnas = decoracion ? 96 : 56;
    float solAz = std::atan2(SOL_DIR_TM.x, -SOL_DIR_TM.z) * RAD2DEG;

    // Mas columnas cerca del azimut del sol (la columna de reflejo es fina).
    auto Azimut = [&](int k)
    {
        float u = -1.0f + 2.0f * (float)k / (float)columnas;
        float a = u * std::fabs(u) * 88.0f + solAz * (1.0f - u * u);
        return a;
    };

    g_emisTM = 1.0f;

    for (int f = 0; f + 1 < filas; f++)
    {
        for (int k = 0; k < columnas; k++)
        {
            float a0 = Azimut(k) * DEG2RAD, a1 = Azimut(k + 1) * DEG2RAD;
            Vector3 p00 = { OJO_TM.x + std::sin(a0) * RADIOS[f], MAR_Y_TM, OJO_TM.z - std::cos(a0) * RADIOS[f] };
            Vector3 p01 = { OJO_TM.x + std::sin(a1) * RADIOS[f], MAR_Y_TM, OJO_TM.z - std::cos(a1) * RADIOS[f] };
            Vector3 p10 = { OJO_TM.x + std::sin(a0) * RADIOS[f + 1], MAR_Y_TM, OJO_TM.z - std::cos(a0) * RADIOS[f + 1] };
            Vector3 p11 = { OJO_TM.x + std::sin(a1) * RADIOS[f + 1], MAR_Y_TM, OJO_TM.z - std::cos(a1) * RADIOS[f + 1] };
            Quad4TM(p00, p01, p11, p10, ColorAguaTM(p00), ColorAguaTM(p01), ColorAguaTM(p11), ColorAguaTM(p10));
        }
    }

    g_emisTM = 0.0f;
}


//--------------------------------------------------
// Costa: sierras, promontorio con el fuerte, pueblo y muelle
//--------------------------------------------------

// Linea de cumbres entre x0 y x1 a la profundidad z (silueta con degradé).
static void SierraTM(float x0, float x1, float z, float altoBase, float altoVar, int tramos, int semilla, Color pie, Color cumbre)
{
    float yPie = MAR_Y_TM - 0.5f;
    float hPrev = 0.0f;

    for (int k = 0; k <= tramos; k++)
    {
        float u = (float)k / (float)tramos;
        float envolvente = std::sin(u * PI);
        float h = altoBase * (0.35f + 0.65f * envolvente)
            + altoVar * (0.55f * Ruido01TM(k, semilla) + 0.45f * Ruido01TM(k / 3, semilla + 1)) * envolvente;

        if (k > 0)
        {
            float xa = x0 + (x1 - x0) * (float)(k - 1) / (float)tramos;
            float xb = x0 + (x1 - x0) * u;
            float za = z + 6.0f * (Ruido01TM(k - 1, semilla + 2) - 0.5f), zb = z + 6.0f * (Ruido01TM(k, semilla + 2) - 0.5f);
            Quad4TM({ xa, yPie, za }, { xb, yPie, zb }, { xb, yPie + h, zb }, { xa, yPie + hPrev, za }, pie, pie, cumbre, cumbre);
        }

        hPrev = h;
    }
}


// Muro con almenas a lo largo de x (cara hacia +z).
static void MuroAlmenadoTM(float x0, float x1, float z, float y0, float alto, float grosor, int paso, int semilla)
{
    int bloques = (int)((x1 - x0) / 3.0f) + 1;

    for (int k = 0; k < bloques; k++)
    {
        float xa = x0 + (x1 - x0) * (float)k / (float)bloques, xb = x0 + (x1 - x0) * (float)(k + 1) / (float)bloques;
        CajaTM({ 0.5f * (xa + xb), y0 + alto * 0.5f, z }, { xb - xa + 0.02f, alto, grosor }, VariarTM(PIEDRA_TM, k, semilla, 0.06f));
    }

    // Cordon y almenas.
    CajaTM({ 0.5f * (x0 + x1), y0 + alto - 0.15f, z + grosor * 0.5f + 0.1f }, { x1 - x0, 0.3f, 0.2f }, TonoTM(PIEDRA_TM, 1.08f));
    int almenas = (int)((x1 - x0) / 1.6f);

    for (int k = 0; k < almenas; k += paso)
    {
        float x = x0 + 0.8f + 1.6f * (float)k;
        CajaTM({ x, y0 + alto + 0.45f, z + grosor * 0.3f }, { 0.9f, 0.9f, grosor * 0.4f }, VariarTM(PIEDRA_TM, k, semilla + 1, 0.06f));
    }
}


// Torre redonda con almenas (baluarte o garita).
static void TorreRedondaTM(Vector3 base, float radio, float alto, int lados, bool almenada, int semilla)
{
    CilTM({ base.x, base.y, base.z }, { base.x, base.y + alto * 0.25f, base.z }, radio * 1.12f, radio, lados, VariarTM(PIEDRA_TM, 0, semilla, 0.05f), false);
    CilTM({ base.x, base.y + alto * 0.25f, base.z }, { base.x, base.y + alto, base.z }, radio, radio, lados, VariarTM(PIEDRA_TM, 1, semilla, 0.05f), true);
    CilTM({ base.x, base.y + alto - 0.3f, base.z }, { base.x, base.y + alto, base.z }, radio + 0.25f, radio + 0.25f, lados, TonoTM(PIEDRA_TM, 1.08f), false);

    if (almenada)
    {
        for (int k = 0; k < lados; k += 2)
        {
            float a = 2.0f * PI * ((float)k + 0.5f) / (float)lados;
            Vector3 c = { base.x + std::cos(a) * (radio + 0.05f), base.y + alto + 0.4f, base.z + std::sin(a) * (radio + 0.05f) };
            CajaGiradaTM(c, { 0.4f, 0.8f, 0.9f }, -a * RAD2DEG, VariarTM(PIEDRA_TM, k, semilla + 2, 0.06f));
        }
    }
}


static void GaritaTM(Vector3 base, int lados)
{
    CilTM({ base.x, base.y - 0.8f, base.z }, base, 0.2f, 0.85f, lados, PIEDRA_TM, false);
    CilTM(base, { base.x, base.y + 1.8f, base.z }, 0.85f, 0.85f, lados, TonoTM(PIEDRA_TM, 1.05f), false);
    BolaTM({ base.x, base.y + 1.8f, base.z }, { 0.95f, 0.9f, 0.95f }, Color{ 168, 112, 92, 255 }, 3, lados, 0.0f);
    CilTM({ base.x, base.y + 2.6f, base.z }, { base.x, base.y + 3.0f, base.z }, 0.12f, 0.02f, 6, PIEDRA_TM);
    g_emisTM = 1.0f;
    CajaTM({ base.x, base.y + 1.0f, base.z + 0.84f }, { 0.22f, 0.6f, 0.04f }, Color{ 255, 190, 100, 255 });
    g_emisTM = 0.0f;
}


static void PalmeraTM(Vector3 base, float alto, float inclinacion, int semilla, int lados)
{
    Vector3 tope = { base.x + inclinacion * alto, base.y + alto, base.z + 0.2f * alto * (Ruido01TM(semilla, 81) - 0.5f) };
    int tramos = 5;

    for (int k = 0; k < tramos; k++)
    {
        float u0 = (float)k / (float)tramos, u1 = (float)(k + 1) / (float)tramos;
        Vector3 a = { base.x + (tope.x - base.x) * u0 * u0, base.y + alto * u0, base.z + (tope.z - base.z) * u0 };
        Vector3 b = { base.x + (tope.x - base.x) * u1 * u1, base.y + alto * u1, base.z + (tope.z - base.z) * u1 };
        CilTM(a, b, 0.22f - 0.1f * u0, 0.22f - 0.1f * u1, lados, VariarTM(Color{ 120, 92, 70, 255 }, k, semilla, 0.08f), false);
    }

    int hojas = 7;
    for (int h = 0; h < hojas; h++)
    {
        float a = 2.0f * PI * (float)h / (float)hojas + Ruido01TM(semilla, h) * 0.4f;
        Vector3 d = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 medio = { tope.x + d.x * 1.3f, tope.y + 0.35f, tope.z + d.z * 1.3f };
        Vector3 punta = { tope.x + d.x * 2.6f, tope.y - 0.9f, tope.z + d.z * 2.6f };
        Vector3 lado = { -d.z * 0.35f, 0.0f, d.x * 0.35f };
        Color verde = VariarTM(Color{ 62, 110, 64, 255 }, h, semilla + 5, 0.1f);
        QuadTM(tope, Vector3Add(medio, lado), punta, Vector3Subtract(medio, lado), CaraTM(verde, { 0.0f, 1.0f, 0.0f }));
    }
}


// Casa del pueblo con techo a dos aguas y ventanas encendidas.
static void CasaTM(Vector3 base, float ancho, float alto, float fondo, int semilla, bool luces)
{
    static const Color REVOQUES[] =
    {
        { 236, 214, 176, 255 }, { 222, 170, 120, 255 }, { 176, 196, 206, 255 }, { 226, 160, 150, 255 }, { 210, 206, 160, 255 }
    };
    Color pared = REVOQUES[semilla % 5];
    Color teja = VariarTM(Color{ 170, 78, 56, 255 }, semilla, 91, 0.1f);
    CajaTM({ base.x, base.y + alto * 0.5f, base.z }, { ancho, alto, fondo }, pared);

    // Techo a dos aguas (cumbrera a lo largo de x).
    float y0 = base.y + alto, y1 = y0 + fondo * 0.42f;
    float x0 = base.x - ancho * 0.5f - 0.2f, x1 = base.x + ancho * 0.5f + 0.2f;
    float z0 = base.z - fondo * 0.5f - 0.25f, z1 = base.z + fondo * 0.5f + 0.25f;
    QuadTM({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, base.z }, { x0, y1, base.z }, CaraTM(teja, Vector3Normalize({ 0.0f, 1.0f, 1.0f })));
    QuadTM({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, base.z }, { x1, y1, base.z }, CaraTM(teja, Vector3Normalize({ 0.0f, 1.0f, -1.0f })));
    TriTM({ base.x - ancho * 0.5f, y0, base.z - fondo * 0.5f }, { base.x - ancho * 0.5f, y0, base.z + fondo * 0.5f }, { base.x - ancho * 0.5f, y1 - 0.05f, base.z }, CaraTM(pared, { -1.0f, 0.0f, 0.0f }));
    TriTM({ base.x + ancho * 0.5f, y0, base.z + fondo * 0.5f }, { base.x + ancho * 0.5f, y0, base.z - fondo * 0.5f }, { base.x + ancho * 0.5f, y1 - 0.05f, base.z }, CaraTM(pared, { 1.0f, 0.0f, 0.0f }));

    // Puerta y ventanas al frente (algunas encendidas).
    float zf = base.z + fondo * 0.5f + 0.02f;
    PanelZTM(base.x - 0.35f, base.y, base.x + 0.35f, base.y + 1.4f, zf, TonoTM(MADERA_OSCURA_TM, 0.9f), MADERA_OSCURA_TM);
    int ventanas = ancho > 3.2f ? 2 : 1;

    for (int v = 0; v < ventanas; v++)
    {
        float x = base.x + (ventanas == 1 ? 1.0f : (v == 0 ? -1.15f : 1.15f)) * (ancho > 3.2f ? 1.0f : 0.6f);
        bool encendida = luces && Ruido01TM(semilla * 7 + v, 92) > 0.3f;
        float y = base.y + alto * 0.55f;

        if (encendida)
        {
            g_emisTM = 1.0f;
            PanelZTM(x - 0.3f, y - 0.35f, x + 0.3f, y + 0.35f, zf + 0.01f, Color{ 255, 170, 80, 255 }, Color{ 255, 214, 140, 255 });
            g_emisTM = 0.0f;
        }
        else
        {
            PanelZTM(x - 0.3f, y - 0.35f, x + 0.3f, y + 0.35f, zf + 0.01f, Color{ 70, 60, 80, 255 }, Color{ 90, 80, 100, 255 });
        }

        PanelZTM(x - 0.4f, y - 0.45f, x + 0.4f, y - 0.35f, zf + 0.015f, TonoTM(pared, 0.85f), TonoTM(pared, 0.85f));
    }
}


// Altura del cerro del pueblo (a la derecha de la caleta).
static float AlturaCerroTM(float x, float z)
{
    float haciaAtras = Lim01TM((-z - 30.0f) / 80.0f);
    float desdeCaleta = Lim01TM((x - 24.0f) / 30.0f);
    float h = (2.0f + 26.0f * haciaAtras * haciaAtras) * (0.35f + 0.65f * desdeCaleta);
    h += 2.5f * std::sin(x * 0.11f) * std::sin(z * 0.09f) * haciaAtras;
    return MAR_Y_TM + 0.4f + h;
}


static void CerroTM(bool decoracion)
{
    int nx = decoracion ? 20 : 12, nz = decoracion ? 14 : 8;
    float x0 = 22.0f, x1 = 190.0f, z0 = -28.0f, z1 = -125.0f;

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < nz; j++)
        {
            float xa = x0 + (x1 - x0) * (float)i / (float)nx, xb = x0 + (x1 - x0) * (float)(i + 1) / (float)nx;
            float za = z0 + (z1 - z0) * (float)j / (float)nz, zb = z0 + (z1 - z0) * (float)(j + 1) / (float)nz;
            Vector3 p00 = { xa, AlturaCerroTM(xa, za), za }, p10 = { xb, AlturaCerroTM(xb, za), za };
            Vector3 p01 = { xa, AlturaCerroTM(xa, zb), zb }, p11 = { xb, AlturaCerroTM(xb, zb), zb };
            Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(p01, p00), Vector3Subtract(p10, p00)));
            if (n.y < 0.0f) n = Vector3Negate(n);
            Color c = VariarTM(MezclaTM(VERDE_TM, Color{ 120, 110, 70, 255 }, Ruido01TM(i * 31 + j, 93) * 0.4f), i * 17 + j, 94, 0.08f);
            QuadTM(p00, p10, p11, p01, CaraTM(c, n));
        }
    }

    // Playa y orilla.
    for (int i = 0; i < nx; i++)
    {
        float xa = x0 + (x1 - x0) * (float)i / (float)nx, xb = x0 + (x1 - x0) * (float)(i + 1) / (float)nx;
        Vector3 a = { xa, AlturaCerroTM(xa, z0), z0 }, b = { xb, AlturaCerroTM(xb, z0), z0 };
        Quad4TM({ xa, MAR_Y_TM - 0.3f, z0 + 4.0f }, { xb, MAR_Y_TM - 0.3f, z0 + 4.0f }, b, a,
            Color{ 214, 176, 130, 255 }, Color{ 214, 176, 130, 255 }, Color{ 200, 160, 116, 255 }, Color{ 200, 160, 116, 255 });
    }

    // Lado que da a la caleta.
    for (int j = 0; j < nz; j++)
    {
        float za = z0 + (z1 - z0) * (float)j / (float)nz, zb = z0 + (z1 - z0) * (float)(j + 1) / (float)nz;
        Quad4TM({ x0, MAR_Y_TM - 0.3f, za }, { x0, MAR_Y_TM - 0.3f, zb }, { x0, AlturaCerroTM(x0, zb), zb }, { x0, AlturaCerroTM(x0, za), za },
            Color{ 150, 110, 96, 255 }, Color{ 150, 110, 96, 255 }, TonoTM(VERDE_TM, 0.8f), TonoTM(VERDE_TM, 0.8f));
    }
}


static void PuebloTM(bool decoracion, int paso)
{
    int lados = paso == 1 ? 8 : 6;
    int casas = 0;

    for (int fila = 0; fila < 6; fila++)
    {
        for (int k = 0; k < 7; k++)
        {
            int s = fila * 7 + k;
            if (paso > 1 && (s % 2) == 1) continue;
            if (Ruido01TM(s, 95) < 0.25f) continue;

            float z = -34.0f - (float)fila * 9.0f - 3.0f * Ruido01TM(s, 96);
            float x = 30.0f + (float)k * 7.5f + (float)(fila % 2) * 3.5f + 2.0f * Ruido01TM(s, 97);
            float y = AlturaCerroTM(x, z) - 0.3f;
            float ancho = 2.6f + 2.0f * Ruido01TM(s, 98);
            float alto = 2.2f + 1.6f * Ruido01TM(s, 99);
            CasaTM({ x, y, z }, ancho, alto, 2.8f, s, true);
            casas++;
        }
    }

    // Iglesia con campanario.
    {
        float x = 58.0f, z = -66.0f, y = AlturaCerroTM(x, z) - 0.3f;
        CasaTM({ x, y, z }, 6.0f, 4.5f, 4.0f, 0, true);
        CajaTM({ x - 3.8f, y + 4.0f, z + 0.5f }, { 2.0f, 8.0f, 2.0f }, Color{ 236, 220, 190, 255 });
        g_emisTM = 1.0f;
        PanelZTM(x - 4.2f, y + 6.0f, x - 3.4f, y + 7.2f, z + 1.52f, Color{ 255, 180, 90, 255 }, Color{ 255, 220, 150, 255 });
        g_emisTM = 0.0f;
        CilTM({ x - 3.8f, y + 8.0f, z + 0.5f }, { x - 3.8f, y + 10.6f, z + 0.5f }, 1.45f, 0.05f, 4, Color{ 170, 78, 56, 255 });
    }

    if (decoracion)
    {
        for (int k = 0; k < 9; k += paso)
        {
            float x = 27.0f + 8.0f * (float)k + 3.0f * Ruido01TM(k, 101);
            float z = -31.0f - 6.0f * Ruido01TM(k, 102);
            PalmeraTM({ x, AlturaCerroTM(x, z) - 0.2f, z }, 5.0f + 2.0f * Ruido01TM(k, 103), 0.08f + 0.06f * Ruido01TM(k, 104), 110 + k, lados);
        }
    }

    (void)casas;
}


// Muelle de madera del pueblo con faroles y botes.
static const Vector3 FAROLES_MUELLE_TM[] = { { 21.2f, 1.2f, -15.0f }, { 24.8f, 1.2f, -15.0f }, { 21.2f, 1.2f, -23.0f } };

static void MuelleTM(int paso)
{
    float y = MAR_Y_TM + 0.9f;
    PisoTM(21.0f, -31.0f, 25.0f, -14.0f, y, CaraTM(MADERA_CUBIERTA_TM, { 0.0f, 1.0f, 0.0f }));

    for (int k = 0; k < 11; k++)
    {
        float z = -31.0f + 1.5f * (float)k;
        PisoTM(21.0f, z + 1.45f, 25.0f, z + 1.5f, y + 0.005f, TonoTM(MADERA_OSCURA_TM, 0.9f));
    }

    CajaTM({ 23.0f, y - 0.1f, -14.0f }, { 4.0f, 0.2f, 0.15f }, MADERA_OSCURA_TM);

    for (int k = 0; k < 6; k++)
    {
        float z = -30.0f + 3.2f * (float)k;
        CajaTM({ 21.1f, y - 1.0f, z }, { 0.3f, 2.6f, 0.3f }, MADERA_OSCURA_TM);
        CajaTM({ 24.9f, y - 1.0f, z }, { 0.3f, 2.6f, 0.3f }, MADERA_OSCURA_TM);
    }

    for (int k = 0; k < 3; k++)
    {
        Vector3 f = FAROLES_MUELLE_TM[k];
        CajaTM({ f.x, 0.5f * (y + f.y - 0.18f), f.z }, { 0.14f, f.y - 0.18f - y, 0.14f }, MADERA_OSCURA_TM);
        g_emisTM = 1.0f;
        CajaTM({ f.x, f.y, f.z }, { 0.28f, 0.36f, 0.28f }, Color{ 255, 206, 120, 255 });
        g_emisTM = 0.0f;
    }

    // Botes amarrados.
    for (int b = 0; b < (paso == 1 ? 3 : 2); b++)
    {
        float x = b == 1 ? 27.4f : 18.8f;
        float z = -18.0f - 5.0f * (float)b;
        Color casco = b == 0 ? Color{ 70, 120, 150, 255 } : (b == 1 ? Color{ 170, 70, 56, 255 } : Color{ 220, 200, 160, 255 });
        CajaTM({ x, MAR_Y_TM + 0.15f, z }, { 1.2f, 0.4f, 3.2f }, casco);
        CajaTM({ x, MAR_Y_TM + 0.36f, z }, { 1.0f, 0.05f, 2.8f }, MADERA_MEDIA_TM);
        BolaTM({ x, MAR_Y_TM + 0.12f, z + 1.6f }, { 0.6f, 0.3f, 0.5f }, casco, 2, 6);
    }
}


// Promontorio de la izquierda con el fuerte.
static const Vector3 ANTORCHAS_FUERTE_TM[] =
{
    { -33.0f, 18.6f, -36.4f }, { -45.0f, 15.3f, -38.6f }, { -61.0f, 15.3f, -38.6f }, { -77.0f, 15.3f, -38.6f }, { -50.0f, 23.0f, -48.5f }
};

static void FuerteTM(bool decoracion, int paso)
{
    int lados = paso == 1 ? 18 : 12;

    // Acantilado: columnas de roca de ancho, saliente y altura variables,
    // con estratos claros y oscuros, pedregal al pie y matas en la cornisa.
    {
        float x = -23.0f;
        int k = 0;

        while (x > -150.0f)
        {
            float ancho = 2.2f + 2.6f * Ruido01TM(k, 111);
            float frente = -36.5f - 2.4f * Ruido01TM(k, 112) - (x > -30.0f ? 2.0f : 0.0f);
            float alto = 11.0f + 1.8f * Ruido01TM(k, 113);
            float tono = 0.82f + 0.3f * Ruido01TM(k, 114);
            const float cortes[4] = { 0.0f, 0.34f, 0.58f, 1.0f };
            const Color capas[3] = { Color{ 104, 74, 76, 255 }, Color{ 148, 106, 90, 255 }, Color{ 92, 66, 74, 255 } };

            for (int s = 0; s < 3; s++)
            {
                float y0 = MAR_Y_TM - 0.6f + (alto + 0.6f - MAR_Y_TM) * cortes[s];
                float y1 = MAR_Y_TM - 0.6f + (alto + 0.6f - MAR_Y_TM) * cortes[s + 1];
                float retiro = 0.5f * (float)s * Ruido01TM(k * 3 + s, 115);
                Color c = TonoTM(capas[s], tono * (0.94f + 0.12f * Ruido01TM(k * 3 + s, 116)));
                float fz = frente - retiro;
                CajaTM({ x - ancho * 0.5f, 0.5f * (y0 + y1), fz - 10.0f }, { ancho + 0.05f, y1 - y0, 20.0f }, c);
            }

            // Pedregal inclinado al pie.
            Color piedras = TonoTM(Color{ 122, 92, 92, 255 }, tono);
            Vector3 n = Vector3Normalize({ 0.0f, 0.6f, 1.0f });
            QuadTM({ x - ancho, MAR_Y_TM - 0.2f, frente + 3.2f }, { x, MAR_Y_TM - 0.2f, frente + 3.2f }, { x, MAR_Y_TM + 1.6f, frente }, { x - ancho, MAR_Y_TM + 1.6f, frente }, CaraTM(piedras, n));

            // Matas oscuras al borde de la cornisa.
            if (decoracion && Ruido01TM(k, 117) > 0.35f)
            {
                float r = 0.7f + 0.7f * Ruido01TM(k, 118);
                Color verde = VariarTM(Color{ 66, 104, 62, 255 }, k, 119, 0.12f);
                BolaTM({ x - ancho * 0.35f, alto - 0.05f, frente - 0.6f }, { r * 1.2f, r * 0.9f, r }, verde, 4, 8, 0.0f);
                BolaTM({ x - ancho * 0.75f, alto - 0.1f, frente - 0.9f }, { r * 0.9f, r * 0.7f, r * 0.8f }, TonoTM(verde, 0.9f), 4, 8, 0.0f);
            }

            x -= ancho;
            k++;
        }
    }

    // Tapa de pasto sobre el acantilado.
    PisoTM(-150.0f, -62.0f, -23.0f, -37.0f, 11.6f, CaraTM(TonoTM(VERDE_TM, 0.9f), { 0.0f, 1.0f, 0.0f }));

    for (int k = 0; k < (decoracion ? 16 : 6); k++)
    {
        float x = -24.0f - 6.5f * (float)k - 3.0f * Ruido01TM(k, 114);
        float z = -34.0f - 3.5f * Ruido01TM(k, 115);
        float r = 1.0f + 1.6f * Ruido01TM(k, 116);
        BolaTM({ x, MAR_Y_TM + r * 0.3f, z }, { r * 1.3f, r, r }, VariarTM(Color{ 130, 100, 100, 255 }, k, 117, 0.1f), 3, 7);
    }

    // Muralla con almenas y cañoneras.
    MuroAlmenadoTM(-90.0f, -33.0f, -39.5f, 10.5f, 4.7f, 2.2f, paso, 131);

    for (int k = 0; k < 6; k++)
    {
        float x = -40.0f - 9.0f * (float)k;
        CilTM({ x, 14.2f, -39.0f }, { x, 14.3f, -37.6f }, 0.32f, 0.24f, 6, HIERRO_TM);
    }

    // Baluarte redondo en la punta con garita.
    TorreRedondaTM({ -31.0f, 9.6f, -42.0f }, 5.0f, 8.0f, lados, true, 141);
    GaritaTM({ -27.6f, 17.9f, -38.6f }, 8);

    // Torre del homenaje con ventanas encendidas y asta.
    {
        float x = -52.0f, z = -52.0f;
        CajaTM({ x, 12.0f + 6.0f, z }, { 8.0f, 12.0f, 7.0f }, VariarTM(PIEDRA_TM, 3, 142, 0.05f));
        CajaTM({ x, 24.15f, z }, { 8.6f, 0.3f, 7.6f }, TonoTM(PIEDRA_TM, 1.08f));

        for (int k = 0; k < 5; k += (paso == 1 ? 1 : 2))
        {
            CajaTM({ x - 3.6f + 1.8f * (float)k, 24.75f, z + 3.4f }, { 0.9f, 0.9f, 0.6f }, PIEDRA_TM);
        }

        g_emisTM = 1.0f;
        PanelZTM(x - 2.4f, 18.6f, x - 1.4f, 20.2f, z + 3.52f, Color{ 255, 170, 80, 255 }, Color{ 255, 214, 140, 255 });
        PanelZTM(x + 1.4f, 18.6f, x + 2.4f, 20.2f, z + 3.52f, Color{ 255, 170, 80, 255 }, Color{ 255, 214, 140, 255 });
        g_emisTM = 0.0f;
        PanelZTM(x - 0.5f, 14.8f, x + 0.5f, 16.0f, z + 3.52f, Color{ 60, 50, 70, 255 }, Color{ 70, 60, 80, 255 });
        CilTM({ x, 24.3f, z }, { x, 30.5f, z }, 0.12f, 0.08f, 6, MADERA_OSCURA_TM);
    }

    // Antorchas: soportes (la llama es una luz aditiva aparte).
    for (int k = 0; k < 5; k++)
    {
        Vector3 a = ANTORCHAS_FUERTE_TM[k];
        CilTM({ a.x, a.y - 0.9f, a.z }, { a.x, a.y - 0.1f, a.z }, 0.08f, 0.14f, 6, MADERA_OSCURA_TM);
    }
}


static void CostaEstaticaTM(bool decoracion, int paso)
{
    // Horizonte: dos planos de sierras en la bruma e islotes en la boca.
    Color lejos = Color{ 196, 122, 142, 255 }, lejosCumbre = Color{ 214, 140, 150, 255 };
    Color medio = Color{ 112, 72, 108, 255 }, medioCumbre = Color{ 140, 86, 116, 255 };
    SierraTM(-620.0f, -60.0f, -330.0f, 40.0f, 46.0f, decoracion ? 40 : 20, 151, lejos, lejosCumbre);
    SierraTM(140.0f, 640.0f, -330.0f, 44.0f, 50.0f, decoracion ? 40 : 20, 152, lejos, lejosCumbre);
    SierraTM(-300.0f, -40.0f, -165.0f, 18.0f, 22.0f, decoracion ? 34 : 16, 153, medio, medioCumbre);
    SierraTM(70.0f, 330.0f, -175.0f, 22.0f, 24.0f, decoracion ? 34 : 16, 154, medio, medioCumbre);
    SierraTM(-70.0f, -18.0f, -240.0f, 3.0f, 4.0f, 10, 155, lejos, lejosCumbre);

    CerroTM(decoracion);

    if (decoracion)
    {
        PuebloTM(decoracion, paso);
        MuelleTM(paso);
    }

    FuerteTM(decoracion, paso);
}


//--------------------------------------------------
// Barco: casco, cubiertas, toldilla y castillo de proa
//--------------------------------------------------

// Media manga a la altura de la cubierta: popa algo mas angosta y proa
// que se cierra en curva hasta la roda.
static float MediaMangaTM(float x)
{
    if (x > 8.0f)
    {
        float u = Lim01TM((x - 8.0f) / (PROA_X_TM - 8.0f));
        return MANGA_TM * std::sqrt(Lim01TM(1.0f - u * u));
    }

    if (x < -10.0f)
    {
        float u = Lim01TM((-10.0f - x) / (-10.0f - POPA_X_TM));
        return MANGA_TM - 0.42f * u * u;
    }

    return MANGA_TM;
}


// Altura de la obra muerta maciza del lado de la camara (por encima van
// balaustres, asi se ve a los trepadores al pie del mastil).
static float AltoBordaTM(float x)
{
    if (x <= TOLDILLA_X_TM) return TOLDILLA_Y_TM + 0.3f;
    if (x < TOLDILLA_X_TM + 0.4f) return 0.3f + (TOLDILLA_Y_TM) * (1.0f - (x - TOLDILLA_X_TM) / 0.4f);
    if (x >= CASTILLO_X_TM) return CASTILLO_Y_TM + 0.3f + 0.6f * Lim01TM((x - 12.6f) / 2.0f);
    if (x > CASTILLO_X_TM - 0.4f) return 0.3f + CASTILLO_Y_TM * (x - (CASTILLO_X_TM - 0.4f)) / 0.4f;
    return 0.3f;
}


// Altura del piso en cada tramo (combes, toldilla o castillo).
static float PisoBarcoTM(float x)
{
    if (x <= TOLDILLA_X_TM) return TOLDILLA_Y_TM;
    if (x >= CASTILLO_X_TM) return CASTILLO_Y_TM;
    return 0.0f;
}


// Factor de manga segun la altura: tumblehome hacia arriba y fondo que se
// cierra hacia la quilla.
static float PerfilCascoTM(float y)
{
    if (y >= 0.0f) return 1.0f - 0.02f * Lim01TM(y / 2.0f);
    if (y >= -0.8f) return 1.0f + 0.05f * (-y / 0.8f);
    if (y >= -1.4f) return 1.05f;
    return 1.05f - 0.5f * Lim01TM((-1.4f - y) / 1.9f);
}


static void CascoTM(bool decoracion)
{
    static const float XS[] =
    {
        -12.6f, -11.6f, -10.6f, -9.8f, -9.4f, -9.0f, -7.8f, -6.6f, -5.4f, -4.2f, -3.0f, -1.8f, -0.6f,
        0.6f, 1.8f, 3.0f, 4.2f, 5.4f, 6.6f, 7.8f, 8.6f, 9.3f, 9.7f, 10.4f, 11.2f, 12.0f, 12.7f,
        13.3f, 13.8f, 14.25f, 14.6f
    };
    const int n = (int)(sizeof(XS) / sizeof(XS[0]));

    auto P = [](float x, float y)
    {
        return Vector3{ x, y, MediaMangaTM(x) * PerfilCascoTM(y) };
    };

    for (int s = 0; s + 1 < n; s++)
    {
        float x0 = XS[s], x1 = XS[s + 1];
        float pendiente = (MediaMangaTM(x1) - MediaMangaTM(x0)) / (x1 - x0);
        Vector3 normal = Vector3Normalize({ -pendiente, 0.0f, 1.0f });
        float top0 = AltoBordaTM(x0), top1 = AltoBordaTM(x1);

        // Obra muerta pintada en tracas.
        int tracas = 6;
        for (int k = 0; k < tracas; k++)
        {
            float f0 = (float)k / (float)tracas, f1 = (float)(k + 1) / (float)tracas;
            float ya0 = 0.06f + (top0 - 0.06f) * f0, yb0 = 0.06f + (top0 - 0.06f) * f1;
            float ya1 = 0.06f + (top1 - 0.06f) * f0, yb1 = 0.06f + (top1 - 0.06f) * f1;
            Color c = CaraTM(VariarTM(CASCO_PINTADO_TM, s * 7 + k, 21, 0.06f), normal);
            QuadTM(P(x0, ya0), P(x1, ya1), P(x1, yb1), P(x0, yb0), c);
        }

        // Regala de la borda.
        Color regala = CaraTM(MADERA_OSCURA_TM, { 0.0f, 1.0f, 0.0f });
        Vector3 a0 = P(x0, top0), a1 = P(x1, top1);
        QuadTM(a0, a1, { a1.x, a1.y, a1.z - 0.14f }, { a0.x, a0.y, a0.z - 0.14f }, regala);

        // Friso dorado.
        QuadTM(P(x0, -0.06f), P(x1, -0.06f), P(x1, 0.06f), P(x0, 0.06f), CaraTM(DORADO_TM, normal));

        // Tablazon oscura con tracas que varian.
        for (int k = 0; k < 4; k++)
        {
            float ya = -0.06f - 0.18f * (float)k, yb = ya - 0.18f;
            Color c = CaraTM(VariarTM(MADERA_OSCURA_TM, s * 11 + k, 22, 0.1f), normal);
            QuadTM(P(x0, yb), P(x1, yb), P(x1, ya), P(x0, ya), c);
        }

        // Cinta ocre y obra viva de brea.
        QuadTM(P(x0, -0.92f), P(x1, -0.92f), P(x1, -0.78f), P(x0, -0.78f), CaraTM(Color{ 196, 132, 58, 255 }, normal));
        QuadTM(P(x0, -1.4f), P(x1, -1.4f), P(x1, -0.92f), P(x0, -0.92f), CaraTM(BREA_TM, normal));
        QuadTM(P(x0, -3.3f), P(x1, -3.3f), P(x1, -1.4f), P(x0, -1.4f), CaraTM(TonoTM(BREA_TM, 0.8f), normal));
    }

    // Portas cerradas con tapa roja, marco y bisagra.
    for (int k = 0; k < 8; k++)
    {
        float x = -7.7f + 2.2f * (float)k;
        float z = MediaMangaTM(x) * PerfilCascoTM(-0.45f) + 0.03f;
        CajaTM({ x, -0.46f, z }, { 0.56f, 0.5f, 0.05f }, MADERA_OSCURA_TM);
        CajaTM({ x, -0.46f, z + 0.04f }, { 0.44f, 0.38f, 0.05f }, VariarTM(ROJO_TAPA_TM, k, 23, 0.06f));
        CajaTM({ x, -0.32f, z + 0.08f }, { 0.4f, 0.04f, 0.03f }, HIERRO_TM);
    }

    // Galeria de popa con ventanas encendidas (se ve de costado).
    {
        float z = MediaMangaTM(-11.4f) + 0.05f;
        CajaTM({ -11.35f, 0.95f, z + 0.12f }, { 2.0f, 1.1f, 0.3f }, CASCO_PINTADO_TM);
        CajaTM({ -11.35f, 1.55f, z + 0.18f }, { 2.2f, 0.12f, 0.42f }, DORADO_TM);
        CajaTM({ -11.35f, 0.35f, z + 0.18f }, { 2.2f, 0.12f, 0.42f }, DORADO_TM);
        g_emisTM = 1.0f;

        for (int k = 0; k < 3; k++)
        {
            float x = -12.0f + 0.65f * (float)k;
            PanelZTM(x - 0.22f, 0.62f, x + 0.22f, 1.3f, z + 0.28f, Color{ 255, 170, 80, 255 }, Color{ 255, 220, 140, 255 });
        }

        g_emisTM = 0.0f;

        for (int k = 0; k < 4; k++)
        {
            CajaTM({ -12.33f + 0.65f * (float)k, 0.96f, z + 0.29f }, { 0.07f, 0.72f, 0.03f }, DORADO_TM);
        }
    }

    // Roda y tajamar con mascaron dorado bajo el baupres.
    VigaTM({ PROA_X_TM - 0.1f, -1.4f, 0.0f }, { PROA_X_TM + 0.15f, 1.9f, 0.0f }, 0.22f, 0.3f, MADERA_OSCURA_TM);
    VigaTM({ PROA_X_TM + 0.1f, -0.2f, 0.0f }, { PROA_X_TM + 1.1f, 0.5f, 0.0f }, 0.2f, 0.34f, MADERA_OSCURA_TM);
    BolaTM({ PROA_X_TM + 0.95f, 0.55f, 0.0f }, { 0.32f, 0.42f, 0.26f }, DORADO_TM, 4, 8);
    BolaTM({ PROA_X_TM + 1.05f, 1.02f, 0.0f }, { 0.18f, 0.2f, 0.17f }, DORADO_TM, 3, 8);

    // Ancla en su serviola, del lado de la camara.
    {
        float x = 12.1f, z = MediaMangaTM(12.1f) + 0.16f;
        VigaTM({ x, 1.55f, z - 0.5f }, { x, 1.55f, z + 0.35f }, 0.22f, 0.22f, MADERA_OSCURA_TM);
        VigaTM({ x, 1.4f, z + 0.25f }, { x, -0.45f, z + 0.25f }, 0.12f, 0.12f, HIERRO_TM);
        VigaTM({ x - 0.55f, -0.15f, z + 0.25f }, { x, -0.6f, z + 0.25f }, 0.1f, 0.1f, HIERRO_TM);
        VigaTM({ x + 0.55f, -0.15f, z + 0.25f }, { x, -0.6f, z + 0.25f }, 0.1f, 0.1f, HIERRO_TM);
        VigaTM({ x - 0.45f, 1.15f, z + 0.25f }, { x + 0.45f, 1.15f, z + 0.25f }, 0.1f, 0.1f, MADERA_MEDIA_TM);
        CilTM({ x, 1.4f, z + 0.25f }, { x, 1.4f, z + 0.32f }, 0.12f, 0.12f, 8, HIERRO_TM);
    }

    (void)decoracion;
}


// Tablones de un piso entre x0 y x1 (con brea en las juntas).
static void TablonesTM(float x0, float x1, float y, int semilla)
{
    const float ancho = 0.25f;
    int filas = (int)(2.0f * MANGA_TM / ancho);

    for (int f = 0; f < filas; f++)
    {
        float z0 = -MANGA_TM + (float)f * ancho, z1 = z0 + ancho - 0.025f;
        float desfase = (float)(f % 3) * 0.8f;
        float x = x0;
        int pieza = 0;

        while (x < x1 - 0.01f)
        {
            float fin = (pieza == 0) ? x0 + 2.4f - desfase : x + 2.4f;
            if (fin > x1) fin = x1;

            // Recorte a la manga en la proa y la popa.
            float zMin = -MediaMangaTM(0.5f * (x + fin)) + 0.04f, zMax = -zMin;
            float za = z0 < zMin ? zMin : z0, zb = z1 > zMax ? zMax : z1;

            if (zb > za)
            {
                Color c = CaraTM(VariarTM(MADERA_CUBIERTA_TM, semilla * 997 + f * 31 + pieza, 31, 0.09f), { 0.0f, 1.0f, 0.0f });
                PisoTM(x + 0.012f, za, fin - 0.012f, zb, y, c);
            }

            x = fin;
            pieza++;
        }
    }

    // Fondo de brea entre juntas.
    PisoTM(x0, -MANGA_TM, x1, MANGA_TM, y - 0.01f, TonoTM(BREA_TM, 1.2f));
}


// Balaustrada del lado de la camara: balaustres y pasamanos.
static void BalaustradaTM(float x0, float x1, float yBase, float yTope, float z, int paso)
{
    int cantidad = (int)((x1 - x0) / 0.5f);

    for (int k = 0; k <= cantidad; k += paso)
    {
        float x = x0 + (x1 - x0) * (float)k / (float)(cantidad > 0 ? cantidad : 1);
        CajaTM({ x, 0.5f * (yBase + yTope), z }, { 0.07f, yTope - yBase, 0.07f }, MADERA_MEDIA_TM);
    }

    CajaTM({ 0.5f * (x0 + x1), yTope + 0.05f, z }, { x1 - x0 + 0.1f, 0.1f, 0.16f }, MADERA_OSCURA_TM);
}


static void CubiertasTM(bool decoracion, int paso)
{
    // Combes, toldilla y castillo de proa.
    TablonesTM(TOLDILLA_X_TM, CASTILLO_X_TM, 0.0f, 1);
    TablonesTM(POPA_X_TM, TOLDILLA_X_TM, TOLDILLA_Y_TM, 2);
    TablonesTM(CASTILLO_X_TM, PROA_X_TM - 0.1f, CASTILLO_Y_TM, 3);

    // Borda del fondo: cara interior ocre y regala.
    Color interior = Color{ 150, 66, 50, 255 };
    for (int s = 0; s < 14; s++)
    {
        float x0 = POPA_X_TM + (PROA_X_TM - 0.6f - POPA_X_TM) * (float)s / 14.0f;
        float x1 = POPA_X_TM + (PROA_X_TM - 0.6f - POPA_X_TM) * (float)(s + 1) / 14.0f;
        float y0a = PisoBarcoTM(x0 + 0.01f), y0b = PisoBarcoTM(x1 - 0.01f);
        float z0 = -MediaMangaTM(x0) + 0.08f, z1 = -MediaMangaTM(x1) + 0.08f;
        float alto = 0.95f;
        Quad4TM({ x0, -0.02f, z0 }, { x1, -0.02f, z1 }, { x1, y0b + alto, z1 }, { x0, y0a + alto, z0 },
            CaraTM(TonoTM(interior, 0.8f), { 0.0f, 0.0f, 1.0f }), CaraTM(TonoTM(interior, 0.8f), { 0.0f, 0.0f, 1.0f }),
            CaraTM(VariarTM(interior, s, 41, 0.05f), { 0.0f, 0.0f, 1.0f }), CaraTM(VariarTM(interior, s, 41, 0.05f), { 0.0f, 0.0f, 1.0f }));
        VigaTM({ x0, y0a + alto + 0.05f, z0 - 0.02f }, { x1, y0b + alto + 0.05f, z1 - 0.02f }, 0.18f, 0.1f, MADERA_OSCURA_TM);

        // Cuadernas a la vista.
        CajaTM({ x0, 0.5f * (y0a + alto), z0 + 0.05f }, { 0.1f, y0a + alto, 0.08f }, MADERA_MEDIA_TM);
    }

    // Balaustradas del lado de la camara.
    BalaustradaTM(TOLDILLA_X_TM + 0.4f, CASTILLO_X_TM - 0.4f, 0.3f, 0.86f, MANGA_TM - 0.07f, paso);
    BalaustradaTM(POPA_X_TM + 0.3f, TOLDILLA_X_TM, TOLDILLA_Y_TM + 0.3f, TOLDILLA_Y_TM + 0.9f, MediaMangaTM(-11.0f) - 0.07f, paso);
    BalaustradaTM(CASTILLO_X_TM, 12.4f, CASTILLO_Y_TM + 0.3f, CASTILLO_Y_TM + 0.88f, MediaMangaTM(11.0f) - 0.07f, paso);

    // Frente de la toldilla: camara del capitan con puerta y ventanas.
    {
        float x = TOLDILLA_X_TM;
        Vector3 n = { 1.0f, 0.0f, 0.0f };

        for (int k = 0; k < 10; k++)
        {
            float z0 = -MANGA_TM + 0.08f + (2.0f * MANGA_TM - 0.16f) * (float)k / 10.0f;
            float z1 = -MANGA_TM + 0.08f + (2.0f * MANGA_TM - 0.16f) * (float)(k + 1) / 10.0f;
            QuadTM({ x, 0.0f, z0 }, { x, 0.0f, z1 }, { x, TOLDILLA_Y_TM, z1 }, { x, TOLDILLA_Y_TM, z0 }, CaraTM(VariarTM(MADERA_MEDIA_TM, k, 42, 0.08f), n));
        }

        CajaTM({ x + 0.04f, 0.56f, 0.0f }, { 0.08f, 1.12f, 0.92f }, MADERA_OSCURA_TM);
        CajaTM({ x + 0.09f, 0.58f, 0.32f }, { 0.04f, 0.08f, 0.08f }, DORADO_TM);
        g_emisTM = 1.0f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            float zc = 1.45f * (float)lado;
            Quad4TM({ x + 0.03f, 0.48f, zc - 0.3f }, { x + 0.03f, 0.48f, zc + 0.3f }, { x + 0.03f, 1.0f, zc + 0.3f }, { x + 0.03f, 1.0f, zc - 0.3f },
                Color{ 255, 160, 70, 255 }, Color{ 255, 160, 70, 255 }, Color{ 255, 222, 150, 255 }, Color{ 255, 222, 150, 255 });
        }

        g_emisTM = 0.0f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            float zc = 1.45f * (float)lado;
            CajaTM({ x + 0.05f, 0.74f, zc }, { 0.04f, 0.6f, 0.05f }, DORADO_TM);
            CajaTM({ x + 0.05f, 0.74f, zc }, { 0.04f, 0.05f, 0.64f }, DORADO_TM);
            CajaTM({ x + 0.06f, 1.04f, zc }, { 0.08f, 0.07f, 0.72f }, MADERA_OSCURA_TM);
            CajaTM({ x + 0.06f, 0.44f, zc }, { 0.08f, 0.07f, 0.72f }, MADERA_OSCURA_TM);
        }

        // Balaustrada al borde de la toldilla y escala.
        for (int k = 0; k <= 10; k += paso)
        {
            float z = -MANGA_TM + 0.2f + (2.0f * MANGA_TM - 0.4f) * (float)k / 10.0f;
            CajaTM({ x - 0.05f, TOLDILLA_Y_TM + 0.3f, z }, { 0.07f, 0.6f, 0.07f }, MADERA_MEDIA_TM);
        }

        CajaTM({ x - 0.05f, TOLDILLA_Y_TM + 0.65f, 0.0f }, { 0.16f, 0.1f, 2.0f * MANGA_TM - 0.3f }, MADERA_OSCURA_TM);

        for (int k = 0; k < 5; k++)
        {
            float f = (float)(k + 1) / 6.0f;
            CajaTM({ x + 1.1f - 1.1f * f, TOLDILLA_Y_TM * f - 0.04f, -1.85f }, { 0.26f, 0.06f, 0.7f }, MADERA_CUBIERTA_TM);
        }

        VigaTM({ x + 1.15f, 0.0f, -2.22f }, { x, TOLDILLA_Y_TM + 0.6f, -2.22f }, 0.07f, 0.12f, MADERA_OSCURA_TM);
        VigaTM({ x + 1.15f, 0.0f, -1.48f }, { x, TOLDILLA_Y_TM + 0.6f, -1.48f }, 0.07f, 0.12f, MADERA_OSCURA_TM);
    }

    // Frente del castillo de proa con puertita y escala.
    {
        float x = CASTILLO_X_TM;
        Vector3 n = { -1.0f, 0.0f, 0.0f };

        for (int k = 0; k < 10; k++)
        {
            float z0 = -MANGA_TM + 0.1f + (2.0f * MANGA_TM - 0.2f) * (float)k / 10.0f;
            float z1 = -MANGA_TM + 0.1f + (2.0f * MANGA_TM - 0.2f) * (float)(k + 1) / 10.0f;
            QuadTM({ x, 0.0f, z1 }, { x, 0.0f, z0 }, { x, CASTILLO_Y_TM, z0 }, { x, CASTILLO_Y_TM, z1 }, CaraTM(VariarTM(MADERA_MEDIA_TM, k, 43, 0.08f), n));
        }

        CajaTM({ x - 0.04f, 0.42f, 0.5f }, { 0.08f, 0.84f, 0.7f }, MADERA_OSCURA_TM);

        for (int k = 0; k <= 10; k += paso)
        {
            float z = -MANGA_TM + 0.3f + (2.0f * MANGA_TM - 0.6f) * (float)k / 10.0f;
            CajaTM({ x + 0.05f, CASTILLO_Y_TM + 0.28f, z }, { 0.07f, 0.56f, 0.07f }, MADERA_MEDIA_TM);
        }

        CajaTM({ x + 0.05f, CASTILLO_Y_TM + 0.6f, 0.0f }, { 0.16f, 0.1f, 2.0f * MANGA_TM - 0.5f }, MADERA_OSCURA_TM);

        for (int k = 0; k < 4; k++)
        {
            float f = (float)(k + 1) / 5.0f;
            CajaTM({ x - 0.9f + 0.9f * f, CASTILLO_Y_TM * f - 0.04f, -1.85f }, { 0.24f, 0.06f, 0.7f }, MADERA_CUBIERTA_TM);
        }
    }

    // Toldilla: timon, farol de popa y asta de la bandera.
    {
        float y = TOLDILLA_Y_TM;
        CajaTM({ -11.0f, y + 0.4f, 0.0f }, { 0.3f, 0.8f, 0.3f }, MADERA_OSCURA_TM);
        CilTM({ -10.86f, y + 1.05f, 0.0f }, { -10.7f, y + 1.05f, 0.0f }, 0.1f, 0.1f, 8, DORADO_TM);
        int rayos = 8;

        for (int k = 0; k < rayos; k++)
        {
            float a = 2.0f * PI * (float)k / (float)rayos;
            Vector3 c = { -10.74f, y + 1.05f, 0.0f };
            Vector3 punta = { c.x, c.y + std::sin(a) * 0.68f, c.z + std::cos(a) * 0.68f };
            VigaTM(c, punta, 0.05f, 0.05f, MADERA_MEDIA_TM);

            float b = 2.0f * PI * (float)(k + 1) / (float)rayos;
            VigaTM({ c.x, c.y + std::sin(a) * 0.5f, c.z + std::cos(a) * 0.5f }, { c.x, c.y + std::sin(b) * 0.5f, c.z + std::cos(b) * 0.5f }, 0.06f, 0.06f, MADERA_OSCURA_TM);
        }

        // Fanales de popa.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float z = 1.4f * (float)lado;
            CajaTM({ -12.35f, y + 0.9f, z }, { 0.12f, 1.8f, 0.12f }, MADERA_OSCURA_TM);
            CajaTM({ -12.35f, y + 1.82f, z }, { 0.36f, 0.06f, 0.36f }, DORADO_TM);
            g_emisTM = 1.0f;
            CajaTM({ -12.35f, y + 2.12f, z }, { 0.3f, 0.55f, 0.3f }, Color{ 255, 206, 120, 255 });
            g_emisTM = 0.0f;
            CilTM({ -12.35f, y + 2.39f, z }, { -12.35f, y + 2.7f, z }, 0.24f, 0.02f, 6, DORADO_TM);
        }

        CilTM({ -12.85f, y, 0.0f }, { -12.85f, y + 6.0f, 0.0f }, 0.07f, 0.05f, 6, MADERA_OSCURA_TM);
    }

    // Castillo: campana con su yugo.
    {
        float y = CASTILLO_Y_TM;
        CajaTM({ 10.7f, y + 0.6f, -0.55f }, { 0.12f, 1.2f, 0.12f }, MADERA_OSCURA_TM);
        CajaTM({ 10.7f, y + 0.6f, 0.55f }, { 0.12f, 1.2f, 0.12f }, MADERA_OSCURA_TM);
        CajaTM({ 10.7f, y + 1.24f, 0.0f }, { 0.18f, 0.14f, 1.4f }, MADERA_OSCURA_TM);
        CilTM({ 10.7f, y + 1.15f, 0.0f }, { 10.7f, y + 0.66f, 0.0f }, 0.1f, 0.26f, 10, DORADO_TM);
        CilTM({ 10.7f, y + 0.66f, 0.0f }, { 10.7f, y + 0.6f, 0.0f }, 0.28f, 0.28f, 10, TonoTM(DORADO_TM, 0.85f));
    }

    // Baupres con su tope.
    CilTM({ 12.6f, CASTILLO_Y_TM + 0.4f, 0.0f }, { 19.6f, 4.3f, 0.0f }, 0.2f, 0.08f, 8, MADERA_MEDIA_TM);
    CilTM({ 15.0f, 2.35f, 0.0f }, { 15.4f, 2.52f, 0.0f }, 0.24f, 0.22f, 8, HIERRO_TM);

    (void)decoracion;
}


//--------------------------------------------------
// Mastiles: marcas de altura, vergas con velas recogidas, cofa y jarcias
//--------------------------------------------------

// Vela recogida sobre su verga: rollo de lona con tomadores.
static void VelaRecogidaTM(float x, float y, float largo, float radio, int lados)
{
    int tramos = 6;
    float z = -0.22f;

    for (int k = 0; k < tramos; k++)
    {
        float u0 = -1.0f + 2.0f * (float)k / (float)tramos, u1 = -1.0f + 2.0f * (float)(k + 1) / (float)tramos;
        float r0 = radio * (0.45f + 0.55f * std::sqrt(Lim01TM(1.0f - u0 * u0 * 0.85f)));
        float r1 = radio * (0.45f + 0.55f * std::sqrt(Lim01TM(1.0f - u1 * u1 * 0.85f)));
        float caida0 = 0.05f * (1.0f - u0 * u0), caida1 = 0.05f * (1.0f - u1 * u1);
        CilTM({ x + u0 * largo * 0.5f, y - 0.14f - caida0, z }, { x + u1 * largo * 0.5f, y - 0.14f - caida1, z }, r0, r1, lados, VariarTM(LONA_TM, k, (int)(x * 10.0f) + 51, 0.04f), k == 0 || k == tramos - 1);
    }

    for (int k = 1; k < 6; k++)
    {
        float u = -1.0f + 2.0f * (float)k / 6.0f;
        float r = radio * (0.45f + 0.55f * std::sqrt(Lim01TM(1.0f - u * u * 0.85f))) + 0.015f;
        float yc = y - 0.14f - 0.05f * (1.0f - u * u);
        CilTM({ x + u * largo * 0.5f - 0.03f, yc, z }, { x + u * largo * 0.5f + 0.03f, yc, z }, r, r, lados, SOGA_TM, false);
    }

    // Verga con sus penoles.
    CilTM({ x - largo * 0.5f - 0.15f, y, -0.3f }, { x + largo * 0.5f + 0.15f, y, -0.3f }, 0.06f, 0.06f, 6, MADERA_MEDIA_TM);
    CilTM({ x - 0.12f, y, -0.3f }, { x + 0.12f, y, -0.3f }, 0.1f, 0.1f, 6, MADERA_OSCURA_TM);

    // Marchapie colgando bajo la verga.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        Vector3 a = { x + s * 0.2f, y - 0.1f, -0.15f };
        Vector3 m = { x + s * largo * 0.28f, y - 0.42f, -0.12f };
        Vector3 b = { x + s * (largo * 0.5f + 0.1f), y - 0.12f, -0.18f };
        SogaTM(a, m, 0.025f, TonoTM(SOGA_TM, 0.7f));
        SogaTM(m, b, 0.025f, TonoTM(SOGA_TM, 0.7f));
    }
}


static void MastilTM(float x, bool decoracion, int paso)
{
    int lados = decoracion ? 12 : 8;
    float cofa = ALTURA_META_MASTIL;

    // Fogonadura en la cubierta.
    CajaTM({ x, 0.08f, 0.0f }, { 0.7f, 0.16f, 0.7f }, MADERA_OSCURA_TM);
    CilTM({ x, 0.0f, 0.0f }, { x, 0.32f, 0.0f }, 0.3f, 0.26f, lados, MADERA_OSCURA_TM);

    // Palo macho y mastelero.
    Color madera = Color{ 158, 108, 66, 255 };
    CilTM({ x, 0.0f, 0.0f }, { x, cofa + 0.2f, 0.0f }, RADIO_MASTIL_TM, RADIO_MASTIL_TM * 0.82f, lados, madera, false);
    CilTM({ x, cofa, 0.0f }, { x, TOPE_MASTIL_TM, 0.0f }, 0.12f, 0.08f, lados, madera);
    CilTM({ x, TOPE_MASTIL_TM, 0.0f }, { x, TOPE_MASTIL_TM + 0.12f, 0.0f }, 0.13f, 0.13f, lados, DORADO_TM);

    // Marcas de altura: zuncho de hierro cada metro y bandas de bronce a
    // los 5 m (la cofa, a los 10 m, es la meta).
    for (int k = 1; k < (int)ALTURA_META_MASTIL; k++)
    {
        float y = (float)k;
        float r = RADIO_MASTIL_TM * (1.0f - 0.18f * y / cofa) + 0.02f;
        bool bronce = (k % 5) == 0;
        float alto = bronce ? 0.16f : 0.07f;
        CilTM({ x, y - alto * 0.5f, 0.0f }, { x, y + alto * 0.5f, 0.0f }, r + (bronce ? 0.015f : 0.0f), r + (bronce ? 0.015f : 0.0f), lados, bronce ? DORADO_TM : HIERRO_TM, false);
    }

    // Cabillas para agarrarse, alternadas a cada lado.
    for (int k = 1; k < 20; k++)
    {
        float y = 0.5f * (float)k;
        float s = (k % 2 == 0) ? -1.0f : 1.0f;
        float r = RADIO_MASTIL_TM * (1.0f - 0.18f * y / cofa);
        VigaTM({ x + s * (r - 0.02f), y, 0.06f }, { x + s * (r + 0.26f), y + 0.02f, 0.06f }, 0.06f, 0.06f, MADERA_OSCURA_TM);
    }

    // Vergas con velas recogidas.
    VelaRecogidaTM(x, 4.4f, 3.7f, 0.2f, decoracion ? 8 : 6);
    VelaRecogidaTM(x, 8.2f, 3.1f, 0.17f, decoracion ? 8 : 6);
    VelaRecogidaTM(x, 11.5f, 2.0f, 0.12f, 6);

    // Cofa: baos, piso y pared de duelas con regala dorada.
    CajaTM({ x, cofa - 0.16f, 0.0f }, { 1.9f, 0.1f, 0.16f }, MADERA_OSCURA_TM);
    CajaTM({ x, cofa - 0.16f, 0.0f }, { 0.16f, 0.1f, 1.9f }, MADERA_OSCURA_TM);
    CilTM({ x, cofa - 0.1f, 0.0f }, { x, cofa, 0.0f }, COFA_RADIO_TM, COFA_RADIO_TM, 16, MADERA_OSCURA_TM);
    int duelas = 16;

    for (int k = 0; k < duelas; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)duelas, a1 = 2.0f * PI * (float)(k + 1) / (float)duelas;
        float am = 0.5f * (a0 + a1);
        Vector3 n = { std::cos(am), 0.0f, std::sin(am) };
        Vector3 p0 = { x + std::cos(a0) * COFA_RADIO_TM, cofa, std::sin(a0) * COFA_RADIO_TM };
        Vector3 p1 = { x + std::cos(a1) * COFA_RADIO_TM, cofa, std::sin(a1) * COFA_RADIO_TM };
        Color c = CaraTM(VariarTM(MADERA_MEDIA_TM, k, 61, 0.1f), n);
        QuadTM(p0, p1, { p1.x, cofa + COFA_PARED_TM, p1.z }, { p0.x, cofa + COFA_PARED_TM, p0.z }, c);
        Color dentro = CaraTM(TonoTM(MADERA_OSCURA_TM, 0.9f), Vector3Negate(n));
        float ri = COFA_RADIO_TM - 0.06f;
        QuadTM({ x + std::cos(a1) * ri, cofa, std::sin(a1) * ri }, { x + std::cos(a0) * ri, cofa, std::sin(a0) * ri },
            { x + std::cos(a0) * ri, cofa + COFA_PARED_TM, std::sin(a0) * ri }, { x + std::cos(a1) * ri, cofa + COFA_PARED_TM, std::sin(a1) * ri }, dentro);
    }

    CilTM({ x, cofa + COFA_PARED_TM - 0.02f, 0.0f }, { x, cofa + COFA_PARED_TM + 0.08f, 0.0f }, COFA_RADIO_TM + 0.04f, COFA_RADIO_TM + 0.04f, 16, DORADO_TM, false);
    CilTM({ x, cofa + 0.12f, 0.0f }, { x, cofa + 0.18f, 0.0f }, COFA_RADIO_TM + 0.02f, COFA_RADIO_TM + 0.02f, 16, HIERRO_TM, false);
    AnilloTM({ x, cofa + COFA_PARED_TM + 0.081f, 0.0f }, COFA_RADIO_TM - 0.06f, COFA_RADIO_TM + 0.04f, 16, CaraTM(DORADO_TM, { 0.0f, 1.0f, 0.0f }), CaraTM(DORADO_TM, { 0.0f, 1.0f, 0.0f }));

    // Jarcias: obenques de la cofa a la regala con flechates.
    Color soga = TonoTM(SOGA_TM, 0.62f);
    float zFrente = MANGA_TM - 0.07f, zFondo = -MANGA_TM + 0.1f;
    const float desvios[3] = { 1.15f, 1.55f, 1.95f };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;

        for (int cara = 0; cara < 2; cara++)
        {
            float zBase = cara == 0 ? zFrente : zFondo;
            float yBase = cara == 0 ? 0.92f : 1.0f;
            float zTope = cara == 0 ? 0.35f : -0.35f;
            Vector3 arriba[3], abajo[3];

            for (int k = 0; k < 3; k++)
            {
                arriba[k] = { x + s * (0.55f + 0.1f * (float)k), cofa - 0.12f, zTope };
                abajo[k] = { x + s * desvios[k], yBase, zBase };
                SogaTM(arriba[k], abajo[k], 0.035f, soga);
                CajaTM({ abajo[k].x, yBase + 0.02f, zBase }, { 0.08f, 0.14f, 0.08f }, MADERA_OSCURA_TM);
            }

            // Flechates (escalones de soga entre los obenques).
            for (int f = 1; f < 20; f += (cara == 0 ? 1 : 2) * paso)
            {
                float u = (float)f / 20.0f;
                Vector3 p0 = Vector3Lerp(abajo[0], arriba[0], u);
                Vector3 p2 = Vector3Lerp(abajo[2], arriba[2], u);
                SogaTM(p0, p2, 0.02f, TonoTM(SOGA_TM, 0.55f));
            }
        }

        // Obenques del mastelero.
        SogaTM({ x, TOPE_MASTIL_TM - 0.3f, 0.0f }, { x + s * 0.75f, cofa + 0.1f, 0.3f }, 0.025f, soga);
        SogaTM({ x, TOPE_MASTIL_TM - 0.3f, 0.0f }, { x + s * 0.75f, cofa + 0.1f, -0.3f }, 0.025f, soga);
    }
}


//--------------------------------------------------
// Utileria de cubierta
//--------------------------------------------------

static void BarrilTM(Vector3 base, float radio, float alto, int semilla, int lados)
{
    Color duelas = VariarTM(Color{ 142, 92, 54, 255 }, semilla, 71, 0.08f);
    CilTM(base, { base.x, base.y + alto * 0.5f, base.z }, radio * 0.86f, radio, lados, duelas, false);
    CilTM({ base.x, base.y + alto * 0.5f, base.z }, { base.x, base.y + alto, base.z }, radio, radio * 0.86f, lados, duelas, false);
    DiscoTM({ base.x, base.y + alto, base.z }, radio * 0.86f, radio * 0.86f, lados, CaraTM(TonoTM(duelas, 0.9f), { 0.0f, 1.0f, 0.0f }), CaraTM(TonoTM(duelas, 0.75f), { 0.0f, 1.0f, 0.0f }));

    for (int k = 0; k < 2; k++)
    {
        float y = base.y + alto * (k == 0 ? 0.2f : 0.8f);
        float r = radio * 0.94f + 0.012f;
        CilTM({ base.x, y - 0.035f, base.z }, { base.x, y + 0.035f, base.z }, r, r, lados, HIERRO_TM, false);
    }
}


static void CofreTM(Vector3 base, float giro, bool abierto, int lados)
{
    float a = giro * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) }, uz = { std::sin(a), 0.0f, std::cos(a) };
    Color madera = Color{ 120, 66, 40, 255 };
    Vector3 c = { base.x, base.y + 0.25f, base.z };
    CajaEjesTM(c, Vector3Scale(ux, 0.5f), { 0.0f, 0.25f, 0.0f }, Vector3Scale(uz, 0.32f), madera);

    // Herrajes y cerradura.
    for (int k = -1; k <= 1; k += 2)
    {
        Vector3 q = Vector3Add(c, Vector3Scale(ux, 0.32f * (float)k));
        CajaEjesTM(q, Vector3Scale(ux, 0.04f), { 0.0f, 0.26f, 0.0f }, Vector3Scale(uz, 0.33f), HIERRO_TM);
    }

    CajaEjesTM(Vector3Add(Vector3Add(c, Vector3Scale(uz, 0.33f)), { 0.0f, 0.12f, 0.0f }), Vector3Scale(ux, 0.07f), { 0.0f, 0.08f, 0.0f }, Vector3Scale(uz, 0.02f), DORADO_TM);

    if (abierto)
    {
        // Monedas asomando y tapa levantada hacia atras.
        BolaTM({ base.x, base.y + 0.5f, base.z }, { 0.42f, 0.12f, 0.26f }, DORADO_TM, 2, lados, 0.0f);
        Vector3 bisagra = Vector3Add(Vector3Add(c, Vector3Scale(uz, -0.32f)), { 0.0f, 0.25f, 0.0f });
        CajaEjesTM(Vector3Add(bisagra, { 0.0f, 0.25f, 0.0f }), Vector3Scale(ux, 0.5f), { 0.0f, 0.25f, 0.0f }, Vector3Scale(uz, 0.05f), TonoTM(madera, 1.1f));
    }
    else
    {
        // Tapa abombada.
        for (int k = 0; k < lados / 2; k++)
        {
            float b0 = PI * (float)k / (float)(lados / 2), b1 = PI * (float)(k + 1) / (float)(lados / 2);
            Vector3 n = Vector3Add(Vector3Scale(uz, std::cos(0.5f * (b0 + b1))), { 0.0f, std::sin(0.5f * (b0 + b1)), 0.0f });
            auto P = [&](float s, float b)
            {
                return Vector3Add(Vector3Add(c, Vector3Scale(ux, 0.5f * s)), Vector3Add(Vector3Scale(uz, 0.32f * std::cos(b)), { 0.0f, 0.25f + 0.16f * std::sin(b), 0.0f }));
            };
            QuadTM(P(-1, b0), P(1, b0), P(1, b1), P(-1, b1), CaraTM(madera, n));
        }
    }
}


static void CajonTM(Vector3 base, Vector3 t, float giro, int semilla)
{
    Color madera = VariarTM(Color{ 170, 128, 82, 255 }, semilla, 72, 0.08f);
    Vector3 c = { base.x, base.y + t.y * 0.5f, base.z };
    CajaGiradaTM(c, t, giro, madera);
    CajaGiradaTM(c, { t.x + 0.02f, 0.08f, t.z + 0.02f }, giro, TonoTM(madera, 0.75f));
    CajaGiradaTM({ c.x, c.y + t.y * 0.5f - 0.04f, c.z }, { t.x + 0.02f, 0.08f, t.z + 0.02f }, giro, TonoTM(madera, 0.8f));
}


static void RolloSogaTM(Vector3 base, float radio, int lados)
{
    for (int k = 0; k < 3; k++)
    {
        float r = radio * (1.0f - 0.18f * (float)k);
        CilTM({ base.x, base.y + 0.08f * (float)k, base.z }, { base.x, base.y + 0.08f * (float)(k + 1), base.z }, r, r * 0.97f, lados, VariarTM(SOGA_TM, k, 73, 0.08f), false);
        AnilloTM({ base.x, base.y + 0.08f * (float)(k + 1), base.z }, r * 0.55f, r * 0.97f, lados, CaraTM(TonoTM(SOGA_TM, 0.8f), { 0.0f, 1.0f, 0.0f }), CaraTM(SOGA_TM, { 0.0f, 1.0f, 0.0f }));
    }

    DiscoTM({ base.x, base.y + 0.2f, base.z }, radio * 0.42f, radio * 0.42f, lados, TonoTM(SOGA_TM, 0.35f), TonoTM(SOGA_TM, 0.45f));
}


// Cañon de cubierta apuntando por la borda de la camara.
static void CanonTM(Vector3 base, int lados)
{
    CajaTM({ base.x, base.y + 0.22f, base.z }, { 0.62f, 0.26f, 0.9f }, MADERA_OSCURA_TM);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = -1; k <= 1; k += 2)
        {
            Vector3 c = { base.x + 0.33f * (float)lado, base.y + 0.13f, base.z + 0.3f * (float)k };
            CilTM({ c.x - 0.04f, c.y, c.z }, { c.x + 0.04f, c.y, c.z }, 0.13f, 0.13f, lados, MADERA_MEDIA_TM);
        }
    }

    CilTM({ base.x, base.y + 0.46f, base.z - 0.55f }, { base.x, base.y + 0.5f, base.z + 0.75f }, 0.2f, 0.13f, lados, HIERRO_TM);
    CilTM({ base.x, base.y + 0.5f, base.z + 0.66f }, { base.x, base.y + 0.5f, base.z + 0.78f }, 0.16f, 0.16f, lados, TonoTM(HIERRO_TM, 1.2f));
    BolaTM({ base.x, base.y + 0.46f, base.z - 0.58f }, { 0.13f, 0.13f, 0.13f }, HIERRO_TM, 3, lados);
}


static void BalasTM(Vector3 base, int lados)
{
    const float r = 0.11f;

    for (int k = 0; k < 3; k++)
    {
        for (int f = 0; f < 3 - k; f++)
        {
            for (int g = 0; g < 3 - k - f; g++)
            {
                Vector3 c = { base.x + (float)f * 2.0f * r + (float)k * r + (float)g * r, base.y + r + (float)k * r * 1.6f, base.z + (float)g * 1.73f * r + (float)k * 0.58f * r };
                BolaTM(c, { r, r, r }, TonoTM(HIERRO_TM, 0.9f), 3, lados);
            }
        }
    }
}


static void SacoTM(Vector3 base, float giro, int semilla)
{
    Color lona = VariarTM(Color{ 196, 170, 126, 255 }, semilla, 74, 0.06f);
    BolaTM({ base.x, base.y + 0.22f, base.z }, { 0.32f, 0.24f, 0.26f }, lona, 3, 8);
    BolaTM({ base.x + 0.05f * std::cos(giro), base.y + 0.46f, base.z }, { 0.12f, 0.1f, 0.12f }, TonoTM(lona, 0.9f), 2, 6);
}


static void CabrestanteTM(Vector3 base, int lados)
{
    CilTM(base, { base.x, base.y + 0.7f, base.z }, 0.34f, 0.26f, lados, MADERA_MEDIA_TM);
    CilTM({ base.x, base.y + 0.7f, base.z }, { base.x, base.y + 0.82f, base.z }, 0.36f, 0.36f, lados, MADERA_OSCURA_TM);

    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * PI * 0.25f + 0.3f;
        Vector3 d = { std::cos(a) * 0.85f, 0.0f, std::sin(a) * 0.85f };
        VigaTM({ base.x - d.x, base.y + 0.76f, base.z - d.z }, { base.x + d.x, base.y + 0.76f, base.z + d.z }, 0.06f, 0.06f, MADERA_OSCURA_TM);
    }
}


static void BaldeTM(Vector3 base, int lados)
{
    CilTM(base, { base.x, base.y + 0.3f, base.z }, 0.15f, 0.19f, lados, Color{ 150, 104, 62, 255 }, false);
    DiscoTM({ base.x, base.y + 0.26f, base.z }, 0.17f, 0.17f, lados, Color{ 70, 90, 120, 255 }, Color{ 60, 76, 104, 255 });
    VigaTM({ base.x + 0.25f, base.y, base.z - 0.1f }, { base.x - 0.05f, base.y + 1.1f, base.z - 0.2f }, 0.04f, 0.04f, MADERA_MEDIA_TM);
}


// Farol colgado de la borda del fondo (su luz se agrega aparte).
static void FarolBordaTM(float x, int lados)
{
    float z = -MediaMangaTM(x) + 0.1f;
    float y = PisoBarcoTM(x) + 0.95f;
    VigaTM({ x, y, z }, { x, y + 0.9f, z }, 0.07f, 0.07f, MADERA_OSCURA_TM);
    VigaTM({ x, y + 0.9f, z }, { x, y + 0.9f, z + 0.35f }, 0.06f, 0.06f, MADERA_OSCURA_TM);
    CilTM({ x, y + 0.72f, z + 0.35f }, { x, y + 0.86f, z + 0.35f }, 0.16f, 0.04f, lados, HIERRO_TM);
    g_emisTM = 1.0f;
    CajaTM({ x, y + 0.55f, z + 0.35f }, { 0.2f, 0.3f, 0.2f }, Color{ 255, 210, 130, 255 });
    g_emisTM = 0.0f;
    CajaTM({ x, y + 0.38f, z + 0.35f }, { 0.24f, 0.05f, 0.24f }, HIERRO_TM);
}


static const float FAROLES_BORDA_TM[] = { -8.8f, -4.4f, 0.0f, 4.4f, 8.8f };


// Hay lugar si nada tapa a un trepador al pie de su mastil.
static bool LibreTM(const MinijuegoTrepaMastil& m, int limite, float x, float z, float radio)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        float margen = z > 0.3f ? 1.3f : 0.75f;
        if (std::fabs(x - m.posicionX[i]) < radio + margen) return false;
    }

    return true;
}


static void UtileriaTM(const MinijuegoTrepaMastil& m, int limite, int paso)
{
    int lados = paso == 1 ? 12 : 8;

    struct Prop { float x, z; int tipo; float radio; };
    static const Prop PROPS[] =
    {
        { -8.3f, -1.9f, 0, 0.6f }, { -5.6f, -1.95f, 2, 0.6f }, { -4.4f, -1.55f, 1, 0.55f }, { -3.2f, -2.0f, 3, 0.4f },
        { -1.0f, -1.9f, 0, 0.6f }, { 0.0f, -1.2f, 6, 0.5f }, { 1.2f, -2.0f, 7, 0.45f }, { 3.3f, -1.95f, 2, 0.6f },
        { 4.5f, -1.6f, 8, 0.55f }, { 5.7f, -2.0f, 3, 0.4f }, { 7.9f, -1.9f, 0, 0.6f }, { 8.5f, -0.9f, 9, 0.3f },
        { -2.2f, -1.7f, 8, 0.55f }, { 2.2f, -1.7f, 1, 0.55f }, { -6.6f, -1.7f, 3, 0.4f }, { 6.6f, -1.75f, 7, 0.45f },
        { -8.6f, 1.45f, 4, 0.5f }, { -4.4f, 1.45f, 4, 0.5f }, { 4.4f, 1.45f, 4, 0.5f }, { 0.0f, 1.7f, 5, 0.45f },
        { -2.2f, 1.5f, 4, 0.5f }, { 2.2f, 1.65f, 5, 0.45f }, { 8.3f, 1.6f, 0, 0.55f }, { 6.6f, 1.5f, 4, 0.5f }
    };
    const int cantidad = (int)(sizeof(PROPS) / sizeof(PROPS[0]));

    for (int k = 0; k < cantidad; k += 1)
    {
        const Prop& p = PROPS[k];

        if (paso > 1 && (k % 3) == 2) continue;
        if (!LibreTM(m, limite, p.x, p.z, p.radio)) continue;

        Vector3 b = { p.x, 0.0f, p.z };

        switch (p.tipo)
        {
            case 0:
                BarrilTM({ b.x - 0.3f, 0.0f, b.z }, 0.3f, 0.78f, k, lados);
                BarrilTM({ b.x + 0.32f, 0.0f, b.z + 0.1f }, 0.3f, 0.78f, k + 1, lados);
                BarrilTM({ b.x + 0.02f, 0.78f, b.z + 0.04f }, 0.28f, 0.72f, k + 2, lados);
                break;
            case 1:
                CofreTM(b, 8.0f, true, lados);
                break;
            case 2:
                CajonTM(b, { 0.9f, 0.7f, 0.8f }, 6.0f, k);
                CajonTM({ b.x + 0.1f, 0.7f, b.z }, { 0.6f, 0.5f, 0.6f }, -12.0f, k + 3);
                break;
            case 3:
                RolloSogaTM(b, 0.4f, lados);
                break;
            case 4:
                CanonTM(b, lados);
                if (p.x < 0.0f) BalasTM({ b.x + 0.5f, 0.0f, b.z - 0.5f }, 6);
                break;
            case 5:
                CofreTM(b, -6.0f, false, lados);
                break;
            case 6:
                CabrestanteTM(b, lados);
                break;
            case 7:
                SacoTM(b, 0.4f, k);
                SacoTM({ b.x + 0.45f, 0.0f, b.z + 0.15f }, 1.3f, k + 1);
                SacoTM({ b.x + 0.2f, 0.36f, b.z + 0.05f }, 2.1f, k + 2);
                break;
            case 8:
                CofreTM(b, -10.0f, false, lados);
                RolloSogaTM({ b.x + 0.75f, 0.0f, b.z - 0.15f }, 0.3f, lados);
                break;
            default:
                BaldeTM(b, lados);
                break;
        }
    }

    for (int k = 0; k < 5; k++)
    {
        if (LibreTM(m, limite, FAROLES_BORDA_TM[k], -2.0f, 0.0f) || paso == 1)
        {
            FarolBordaTM(FAROLES_BORDA_TM[k], lados);
        }
    }
}


//--------------------------------------------------
// Barco completo (horneado en el marco del barco)
//--------------------------------------------------

static void BarcoEstaticoTM(const MinijuegoTrepaMastil& m, int limite, bool decoracion, int paso)
{
    CascoTM(decoracion);
    CubiertasTM(decoracion, paso);

    float primero = 99.0f, ultimo = -99.0f;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        MastilTM(m.posicionX[i], decoracion, paso);
        if (m.posicionX[i] < primero) primero = m.posicionX[i];
        if (m.posicionX[i] > ultimo) ultimo = m.posicionX[i];
    }

    // Estays entre topes y al baupres; amarras a popa.
    if (ultimo > -99.0f)
    {
        Color soga = TonoTM(SOGA_TM, 0.6f);
        SogaTM({ ultimo, TOPE_MASTIL_TM - 0.2f, 0.0f }, { 19.4f, 4.25f, 0.0f }, 0.04f, soga);
        SogaTM({ ultimo, ALTURA_META_MASTIL - 0.2f, 0.0f }, { 17.0f, 3.2f, 0.0f }, 0.04f, soga);
        SogaTM({ primero, TOPE_MASTIL_TM - 0.2f, 0.0f }, { -12.85f, TOLDILLA_Y_TM + 6.0f, 0.0f }, 0.035f, soga);
        SogaTM({ -12.85f, TOLDILLA_Y_TM + 5.9f, 0.0f }, { POPA_X_TM + 0.2f, TOLDILLA_Y_TM + 0.9f, 1.8f }, 0.03f, soga);

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo) continue;

            float siguiente = 99.0f;
            for (int j = 0; j < limite; j++)
            {
                if (m.resultado.participantes[j].participo && m.posicionX[j] > m.posicionX[i] + 0.1f && m.posicionX[j] < siguiente) siguiente = m.posicionX[j];
            }

            if (siguiente < 99.0f)
            {
                SogaTM({ m.posicionX[i], TOPE_MASTIL_TM - 0.2f, 0.0f }, { siguiente, ALTURA_META_MASTIL - 0.15f, 0.0f }, 0.035f, soga);
            }
        }
    }

    if (decoracion)
    {
        UtileriaTM(m, limite, paso);
    }
    else
    {
        // En BAJA quedan solo los faroles de la borda.
        for (int k = 0; k < 5; k += 2) FarolBordaTM(FAROLES_BORDA_TM[k], 6);
    }
}


//--------------------------------------------------
// Camara de la vista
//--------------------------------------------------

static Camera3D CamaraVistaTM(const MinijuegoTrepaMastil& m)
{
    Camera3D c = m.camara;

    // Un poco mas atras y mirando mas alto que la camara de la logica:
    // las cofas y banderas quedan por debajo del tendedero del HUD.
    c.position = { 0.0f, 6.5f, 21.5f };
    c.target = { 0.0f, 6.6f, 0.0f };

    // Con menos mastiles la camara se acerca un poco.
    float tramo = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (m.resultado.participantes[i].participo && std::fabs(m.posicionX[i]) > tramo) tramo = std::fabs(m.posicionX[i]);
    }

    float acercar = Lim01TM((6.6f - tramo) / 4.4f) * 1.9f;
    c.position.z -= acercar;
    c.target.y -= acercar * 0.2f;

    // Entrada: baja desde las cofas mientras corre la cuenta regresiva.
    if (m.fase == FASE_MASTIL_PREPARACION && m.resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        float u = Lim01TM(1.0f - m.tiempoPreparacion / DURACION_PREPARACION_MASTIL);
        float e = 1.0f - u;
        e = e * e * (3.0f - 2.0f * e) * ObtenerFactorMovimientoCamaraMinijuegos();
        c.position.z += 7.0f * e;
        c.position.y += 4.5f * e;
        c.position.x -= 3.0f * e;
        c.target.y += 2.5f * e;
    }

    return AplicarTemblorGeneralACamara(c);
}


//--------------------------------------------------
// Banderas que flamean (viento hacia +x)
//--------------------------------------------------

static Vector3 PuntoBanderaTM(Vector3 asta, float largo, float alto, float u, float v, float t, float fase, float fuerza)
{
    float onda = std::sin(t * 6.0f - u * 5.5f + fase) * (0.1f + 0.14f * u) * fuerza;
    float caida = (1.0f - fuerza) * u * u * alto * 0.5f;
    return { asta.x + u * largo * (1.0f - 0.05f * std::fabs(onda)), asta.y - v * alto - caida + 0.04f * onda * u, asta.z + onda };
}


static void BanderaTM(Vector3 asta, float largo, float alto, Color color, float t, float fase, int columnas, float fuerza = 1.0f)
{
    int filas = 2;

    for (int i = 0; i < columnas; i++)
    {
        float u0 = (float)i / (float)columnas, u1 = (float)(i + 1) / (float)columnas;
        float luz = 0.82f + 0.18f * std::cos(t * 6.0f - (u0 + u1) * 2.75f + fase);

        for (int j = 0; j < filas; j++)
        {
            float v0 = (float)j / (float)filas, v1 = (float)(j + 1) / (float)filas;
            Color c = TonoTM(color, luz);
            QuadColTM(PuntoBanderaTM(asta, largo, alto, u0, v0, t, fase, fuerza), PuntoBanderaTM(asta, largo, alto, u1, v0, t, fase, fuerza),
                PuntoBanderaTM(asta, largo, alto, u1, v1, t, fase, fuerza), PuntoBanderaTM(asta, largo, alto, u0, v1, t, fase, fuerza), c, c, c, c);
        }
    }
}


// Bandera pirata: paño negro con calavera y tibias.
static void BanderaPirataTM(Vector3 asta, float t)
{
    const float largo = 1.7f, alto = 1.1f;
    BanderaTM(asta, largo, alto, Color{ 26, 24, 30, 255 }, t, 1.3f, 8, 0.85f);

    auto P = [&](float u, float v, float dz)
    {
        Vector3 p = PuntoBanderaTM(asta, largo, alto, u, v, t, 1.3f, 0.85f);
        p.z += dz;
        return p;
    };

    Color hueso = Color{ 236, 228, 210, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float dz = 0.015f * (float)lado;
        // Tibias cruzadas.
        QuadColTM(P(0.3f, 0.78f, dz), P(0.34f, 0.84f, dz), P(0.66f, 0.5f, dz), P(0.62f, 0.44f, dz), hueso, hueso, hueso, hueso);
        QuadColTM(P(0.62f, 0.84f, dz), P(0.66f, 0.78f, dz), P(0.34f, 0.44f, dz), P(0.3f, 0.5f, dz), hueso, hueso, hueso, hueso);
        // Craneo y mandibula.
        QuadColTM(P(0.37f, 0.52f, dz * 1.4f), P(0.59f, 0.52f, dz * 1.4f), P(0.59f, 0.2f, dz * 1.4f), P(0.37f, 0.2f, dz * 1.4f), hueso, hueso, hueso, hueso);
        QuadColTM(P(0.41f, 0.62f, dz * 1.4f), P(0.55f, 0.62f, dz * 1.4f), P(0.55f, 0.5f, dz * 1.4f), P(0.41f, 0.5f, dz * 1.4f), hueso, hueso, hueso, hueso);
        Color ojo = Color{ 26, 24, 30, 255 };
        QuadColTM(P(0.4f, 0.4f, dz * 1.8f), P(0.46f, 0.4f, dz * 1.8f), P(0.46f, 0.31f, dz * 1.8f), P(0.4f, 0.31f, dz * 1.8f), ojo, ojo, ojo, ojo);
        QuadColTM(P(0.5f, 0.4f, dz * 1.8f), P(0.56f, 0.4f, dz * 1.8f), P(0.56f, 0.31f, dz * 1.8f), P(0.5f, 0.31f, dz * 1.8f), ojo, ojo, ojo, ojo);
    }
}


//--------------------------------------------------
// Dinamico en el marco del barco: cintas, banderas y cuervos
//--------------------------------------------------

static void DibujarCuervoTM(Vector3 c, float direccion, float fase, bool detalle)
{
    float d = direccion;
    float aleteo = std::sin(fase * 22.0f);
    int lados = detalle ? 8 : 6;

    BolaTM(c, { 0.36f, 0.22f, 0.22f }, CUERVO_TM, 3, lados);
    BolaTM({ c.x + d * 0.34f, c.y + 0.1f, c.z }, { 0.17f, 0.16f, 0.16f }, TonoTM(CUERVO_TM, 1.15f), 3, lados);
    CilTM({ c.x + d * 0.46f, c.y + 0.08f, c.z }, { c.x + d * 0.72f, c.y + 0.03f, c.z }, 0.07f, 0.0f, 5, Color{ 240, 170, 50, 255 });
    PuntoTM({ c.x + d * 0.42f, c.y + 0.16f, c.z + 0.15f }, 0.035f, Color{ 255, 236, 120, 255 });
    PuntoTM({ c.x + d * 0.425f, c.y + 0.16f, c.z + 0.16f }, 0.016f, Color{ 200, 20, 20, 255 });

    // Alas que baten y cola en abanico.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        Vector3 hombro = { c.x + d * 0.05f, c.y + 0.08f, c.z + s * 0.12f };
        Vector3 codo = { c.x - d * 0.1f, c.y + 0.1f + aleteo * 0.28f, c.z + s * 0.55f };
        Vector3 punta = { c.x - d * 0.42f, c.y + 0.05f + aleteo * 0.55f, c.z + s * 0.95f };
        Vector3 atras = { c.x - d * 0.3f, c.y + 0.02f, c.z + s * 0.15f };
        Color ala = CaraTM(TonoTM(CUERVO_TM, 1.1f), { 0.0f, 1.0f, 0.0f });
        QuadColTM(hombro, codo, punta, atras, ala, ala, TonoTM(ala, 0.8f), ala);
    }

    Color cola = TonoTM(CUERVO_TM, 0.9f);
    QuadColTM({ c.x - d * 0.28f, c.y + 0.02f, c.z - 0.06f }, { c.x - d * 0.28f, c.y + 0.02f, c.z + 0.06f },
        { c.x - d * 0.68f, c.y - 0.06f, c.z + 0.2f }, { c.x - d * 0.68f, c.y - 0.06f, c.z - 0.2f }, cola, cola, cola, cola);
}


static void DibujarBarcoDinamicoTM(
    const MinijuegoTrepaMastil& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    bool detalle,
    bool sombras
)
{
    float t = m.tiempoAnimacion;

    LoteTM();

    // Bandera pirata en el asta de popa.
    BanderaPirataTM({ -12.85f, TOLDILLA_Y_TM + 5.95f, 0.0f }, t);

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const TrepadorMastil& tr = m.trepadores[i];
        Color color = participantes[i].color;
        Color claro = MezclaTM(color, WHITE, 0.25f);
        float x = m.posicionX[i];

        // Banda del color del jugador al pie del mastil y aro en cubierta.
        CilTM({ x, 0.34f, 0.0f }, { x, 0.62f, 0.0f }, RADIO_MASTIL_TM + 0.015f, RADIO_MASTIL_TM + 0.012f, 10, color, false);
        AnilloTM({ x, 0.014f, 0.0f }, 0.62f, 0.78f, 24, AlfaTM(color, 0.85f), AlfaTM(color, 0.85f));

        // Cinta de progreso: sube por el frente del mastil hasta los pies.
        float tope = LimitarMastil(tr.altura, 0.0f, ALTURA_META_MASTIL);
        if (tope > 0.65f)
        {
            float r0 = RADIO_MASTIL_TM + 0.02f;
            float r1 = RADIO_MASTIL_TM * (1.0f - 0.18f * tope / ALTURA_META_MASTIL) + 0.02f;
            QuadColTM({ x - 0.06f, 0.65f, r0 }, { x + 0.06f, 0.65f, r0 }, { x + 0.06f, tope, r1 }, { x - 0.06f, tope, r1 }, color, color, claro, claro);
        }

        // Franja del color en la pared de la cofa (la meta de cada uno).
        CilTM({ x, ALTURA_META_MASTIL + 0.22f, 0.0f }, { x, ALTURA_META_MASTIL + 0.36f, 0.0f }, COFA_RADIO_TM + 0.025f, COFA_RADIO_TM + 0.025f, 16, color, false);

        // Bandera del jugador en el tope.
        float fuerza = 0.75f + 0.25f * m.intensidadOlas;
        CilTM({ x, TOPE_MASTIL_TM + 0.12f, 0.0f }, { x, TOPE_MASTIL_TM + 0.32f, 0.0f }, 0.07f, 0.07f, 6, DORADO_TM);
        BanderaTM({ x + 0.08f, TOPE_MASTIL_TM + 0.05f, 0.0f }, 1.35f, 0.78f, color, t, (float)i * 1.7f, detalle ? 8 : 5, fuerza);

        // Sombra del trepador en la cubierta (se desvanece al subir).
        if (sombras)
        {
            float a = Lim01TM(1.0f - tr.altura / 2.5f);
            if (a > 0.01f)
            {
                DiscoTM({ jugadores[i].posicion.x, 0.016f, PROFUNDIDAD_JUGADOR_MASTIL }, 0.55f, 0.38f, 14, Color{ 20, 12, 24, (unsigned char)(130.0f * a) }, Color{ 20, 12, 24, 0 }, false);
            }
        }

        // Cuervo en vuelo.
        const CuervoMastil& c = m.cuervos[i];
        if (m.fase == FASE_MASTIL_JUGANDO && c.activo && !c.enAviso)
        {
            DibujarCuervoTM({ x + c.x, c.altura, PROFUNDIDAD_JUGADOR_MASTIL + 0.2f }, c.direccion, c.fase, detalle);
        }

        // Olas: flecha al costado que cuenta para que lado conviene trepar.
        if (m.fase == FASE_MASTIL_JUGANDO && m.intensidadOlas > 0.05f && !tr.llego)
        {
            int efecto = EfectoInclinacionMastil(m, i);
            Color f = efecto > 0 ? FAVOR_TM : (efecto < 0 ? CONTRA_TM : Color{ 220, 214, 200, 255 });
            f = AlfaTM(f, 0.35f + 0.65f * m.intensidadOlas);
            float s = (float)tr.lado;
            float bx = x + s * 0.72f, by = tr.altura + 1.05f, bz = PROFUNDIDAD_JUGADOR_MASTIL + 0.15f;
            float latido = efecto > 0 ? 0.08f * std::sin(t * 12.0f) : 0.0f;

            for (int k = 0; k < 2; k++)
            {
                float ox = bx + s * (0.26f * (float)k + latido);
                VTM({ ox, by + 0.2f, bz }, f); VTM({ ox + s * 0.22f, by, bz }, f); VTM({ ox, by - 0.2f, bz }, f);
                VTM({ ox, by + 0.2f, bz }, f); VTM({ ox, by - 0.2f, bz }, f); VTM({ ox - s * 0.08f, by, bz }, f);
            }
        }
    }

    FinLoteTM();
}


//--------------------------------------------------
// Dinamico en el mundo: agua, amarra, gaviotas y bandera del fuerte
//--------------------------------------------------

static void DibujarMundoDinamicoTM(const MinijuegoTrepaMastil& m, float angulo, bool decoracion, bool efectos)
{
    float t = m.tiempoAnimacion;
    float seno = std::sin(angulo), coseno = std::cos(angulo);
    auto Girar = [&](Vector3 p)
    {
        return Vector3{ p.x * coseno - p.y * seno, p.x * seno + p.y * coseno, p.z };
    };

    LoteTM();

    // Pilote de amarre y amarra desde la popa (sigue el balanceo).
    {
        Vector3 pilote = { -17.2f, MAR_Y_TM, 3.0f };
        CilTM({ pilote.x, MAR_Y_TM - 0.5f, pilote.z }, { pilote.x, MAR_Y_TM + 1.5f, pilote.z }, 0.28f, 0.24f, 8, MADERA_OSCURA_TM);
        CilTM({ pilote.x + 0.5f, MAR_Y_TM - 0.5f, pilote.z - 0.3f }, { pilote.x + 0.45f, MAR_Y_TM + 1.1f, pilote.z - 0.3f }, 0.24f, 0.2f, 8, MADERA_MEDIA_TM);
        CilTM({ pilote.x, MAR_Y_TM + 1.0f, pilote.z }, { pilote.x, MAR_Y_TM + 1.2f, pilote.z }, 0.3f, 0.3f, 8, SOGA_TM);
        Vector3 popa = Girar({ POPA_X_TM + 0.3f, TOLDILLA_Y_TM + 0.4f, 1.9f });
        Vector3 nudo = { pilote.x, MAR_Y_TM + 1.1f, pilote.z };
        Vector3 medio = Vector3Lerp(popa, nudo, 0.5f);
        medio.y -= 0.45f;
        Color soga = TonoTM(SOGA_TM, 0.7f);
        TiraTM(popa, medio, 0.06f, 0.06f, soga, soga);
        TiraTM(medio, nudo, 0.06f, 0.06f, soga, soga);
    }

    // Bandera del fuerte.
    BanderaTM({ -52.0f, 30.4f, -52.0f }, 3.2f, 2.0f, Color{ 200, 170, 60, 255 }, t * 0.8f, 2.0f, decoracion ? 6 : 4);

    // Gaviotas que planean en ronda sobre la caleta.
    int gaviotas = decoracion ? 8 : 3;
    for (int g = 0; g < gaviotas; g++)
    {
        float radio = 8.0f + 6.0f * Ruido01TM(g, 161);
        float vel = (0.25f + 0.15f * Ruido01TM(g, 162)) * (g % 2 == 0 ? 1.0f : -1.0f);
        float a = t * vel + 6.28f * Ruido01TM(g, 163);
        Vector3 centro = { -14.0f + 34.0f * Ruido01TM(g, 164), 14.0f + 7.0f * Ruido01TM(g, 165), -14.0f - 18.0f * Ruido01TM(g, 166) };
        Vector3 p = { centro.x + std::cos(a) * radio, centro.y + 0.6f * std::sin(a * 2.0f), centro.z + std::sin(a) * radio * 0.5f };
        float aleteo = std::sin(t * (5.0f + Ruido01TM(g, 167) * 2.0f) + (float)g) * 0.35f;
        float e = 0.55f;
        Color blanco = Color{ 248, 236, 230, 255 }, punta = Color{ 70, 62, 76, 255 };
        Vector3 izq = { p.x - 0.5f * e, p.y + 0.12f * e + aleteo * e, p.z }, der = { p.x + 0.5f * e, p.y + 0.12f * e + aleteo * e, p.z };
        Vector3 izq2 = { p.x - 0.95f * e, p.y + aleteo * e * 1.6f - 0.05f, p.z }, der2 = { p.x + 0.95f * e, p.y + aleteo * e * 1.6f - 0.05f, p.z };
        TiraTM(p, izq, 0.14f * e, 0.1f * e, blanco, blanco);
        TiraTM(p, der, 0.14f * e, 0.1f * e, blanco, blanco);
        TiraTM(izq, izq2, 0.1f * e, 0.03f, blanco, punta);
        TiraTM(der, der2, 0.1f * e, 0.03f, blanco, punta);
        PuntoTM(p, 0.09f, blanco);
    }

    FinLoteTM();

    if (!efectos) return;

    // Agua: destellos del sol, vetas de reflejo y espuma al pie del casco.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteTM();

    float pendiente = SOL_DIR_TM.x / -SOL_DIR_TM.z;
    int destellos = decoracion ? 90 : 45;

    for (int k = 0; k < destellos; k++)
    {
        float u = Ruido01TM(k, 171);
        float dist = 6.0f + 260.0f * u * u;
        float z = OJO_TM.z - dist;
        float ancho = 0.25f + dist * 0.02f;
        float x = OJO_TM.x + pendiente * dist + (Ruido01TM(k, 172) - 0.5f) * (1.2f + dist * 0.045f) + std::sin(t * 0.6f + (float)k) * 0.3f;
        float brillo = std::sin(t * (2.0f + 3.0f * Ruido01TM(k, 173)) + 6.28f * Ruido01TM(k, 174));
        if (brillo <= 0.2f) continue;
        Color c = AlfaTM(Color{ 255, 214, 150, 255 }, (brillo - 0.2f) * 0.8f);
        float y = MAR_Y_TM + 0.02f;
        QuadColTM({ x - ancho, y, z - 0.05f - dist * 0.002f }, { x + ancho, y, z - 0.05f - dist * 0.002f }, { x + ancho, y, z + 0.05f + dist * 0.002f }, { x - ancho, y, z + 0.05f + dist * 0.002f }, c, c, c, c);
    }

    // Vetas de luz que se mueven despacio por la caleta.
    for (int k = 0; k < (decoracion ? 28 : 12); k++)
    {
        float dist = 8.0f + 120.0f * Ruido01TM(k, 175);
        float z = OJO_TM.z - dist;
        float largo = 3.0f + dist * 0.12f;
        float x = std::fmod(Ruido01TM(k, 176) * 200.0f + t * (0.4f + 0.4f * Ruido01TM(k, 177)), 200.0f) - 100.0f;
        float a = 0.06f * (0.5f + 0.5f * std::sin(t * 0.8f + (float)k));
        Color c = AlfaTM(Color{ 255, 170, 170, 255 }, a);
        float y = MAR_Y_TM + 0.015f, g = 0.04f + dist * 0.003f;
        QuadColTM({ x - largo, y, z - g }, { x + largo, y, z - g }, { x + largo, y, z + g }, { x - largo, y, z + g }, AlfaTM(c, 0.0f), c, c, AlfaTM(c, 0.0f));
    }

    // Espuma al pie del casco del lado de la camara (mas viva con las olas).
    float espuma = 0.18f + 0.3f * m.intensidadOlas;
    for (int k = 0; k < 34; k++)
    {
        float x0 = POPA_X_TM + (PROA_X_TM - POPA_X_TM) * (float)k / 34.0f;
        float x1 = POPA_X_TM + (PROA_X_TM - POPA_X_TM) * (float)(k + 1) / 34.0f;
        float z0 = MediaMangaTM(x0) * 1.05f + 0.05f, z1 = MediaMangaTM(x1) * 1.05f + 0.05f;
        float o0 = 0.12f + 0.1f * std::sin(t * 2.2f + x0 * 1.7f), o1 = 0.12f + 0.1f * std::sin(t * 2.2f + x1 * 1.7f);
        float y = MAR_Y_TM + 0.03f;
        Color c = AlfaTM(Color{ 255, 236, 226, 255 }, espuma * (0.7f + 0.3f * std::sin(t * 3.0f + (float)k)));
        QuadColTM({ x0, y, z0 }, { x1, y, z1 }, { x1, y, z1 + o1 + 0.25f }, { x0, y, z0 + o0 + 0.25f }, c, c, AlfaTM(c, 0.0f), AlfaTM(c, 0.0f));
    }

    // Olas: crestas blancas que cruzan la caleta.
    if (m.intensidadOlas > 0.02f)
    {
        for (int k = 0; k < (decoracion ? 40 : 18); k++)
        {
            float dist = 6.0f + 70.0f * Ruido01TM(k, 178);
            float z = OJO_TM.z - dist;
            float ciclo = std::fmod(t * 0.35f + Ruido01TM(k, 179), 1.0f);
            float x = -60.0f + 120.0f * Ruido01TM(k, 180) + ciclo * 8.0f;
            float a = std::sin(ciclo * PI) * 0.35f * m.intensidadOlas;
            float largo = 1.0f + dist * 0.04f, g = 0.06f + dist * 0.004f;
            Color c = AlfaTM(Color{ 255, 240, 232, 255 }, a);
            float y = MAR_Y_TM + 0.03f;
            QuadColTM({ x - largo, y, z - g }, { x + largo, y, z - g }, { x + largo, y, z + g }, { x - largo, y, z + g }, AlfaTM(c, 0.0f), c, c, AlfaTM(c, 0.0f));
        }
    }

    FinLoteTM();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Luces falsas aditivas
//--------------------------------------------------

static void ConoLuzTM(Vector3 arriba, Vector3 abajo, float r0, float r1, Color c)
{
    TiraTM(arriba, abajo, r0 * 2.0f, r1 * 2.0f, c, AlfaTM(c, 0.0f));
    TiraTM(arriba, abajo, r0 * 1.0f, r1 * 1.1f, c, AlfaTM(c, 0.15f));
}


// Reflejo vertical de una luz sobre el agua.
static void ReflejoAguaTM(float x, float z, float largo, float ancho, Color c, float t, int semilla)
{
    float y = MAR_Y_TM + 0.02f;

    for (int k = 0; k < 5; k++)
    {
        float u = (float)k / 5.0f;
        float zz = z + largo * u;
        float ox = 0.12f * std::sin(t * 3.0f + (float)(k + semilla));
        float a = (1.0f - u) * (0.6f + 0.4f * std::sin(t * 5.0f + (float)k * 1.7f + (float)semilla));
        Color cc = AlfaTM(c, a);
        float w = ancho * (1.0f - 0.4f * u);
        QuadColTM({ x + ox - w, y, zz }, { x + ox + w, y, zz }, { x + ox + w, y, zz + largo * 0.16f }, { x + ox - w, y, zz + largo * 0.16f }, cc, cc, AlfaTM(cc, 0.0f), AlfaTM(cc, 0.0f));
    }
}


static void DibujarLucesTM(const MinijuegoTrepaMastil& m, int limite, const Camera3D& vista, float angulo, bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteTM();

    // Resplandor del sol (los mastiles y la costa lo tapan).
    {
        Vector3 sol = Vector3Add(vista.position, Vector3Scale(SOL_DIR_TM, 300.0f));
        HaloTM(sol, 60.0f, Color{ 255, 190, 120, 60 }, Color{ 255, 150, 90, 0 }, 28);
        HaloTM(sol, 22.0f, Color{ 255, 236, 190, 120 }, Color{ 255, 200, 140, 0 }, 24);
    }

    // Antorchas del fuerte que titilan.
    for (int k = 0; k < 5; k++)
    {
        Vector3 a = ANTORCHAS_FUERTE_TM[k];
        float l = 0.75f + 0.25f * std::sin(t * 9.0f + (float)k * 2.1f) * std::sin(t * 4.3f + (float)k);
        HaloTM({ a.x, a.y + 0.2f, a.z + 0.3f }, 2.4f * l, AlfaTM(LUZ_FAROL_TM, 0.4f), AlfaTM(LUZ_FAROL_TM, 0.0f));
        HaloTM({ a.x, a.y + 0.15f, a.z + 0.35f }, 0.7f, AlfaTM(Color{ 255, 236, 170, 255 }, 0.9f * l), AlfaTM(Color{ 255, 140, 60, 255 }, 0.0f), 10);
    }

    if (decoracion)
    {
        // Calor de las ventanas del pueblo y faroles del muelle con su reflejo.
        HaloTM({ 52.0f, 6.0f, -45.0f }, 16.0f, Color{ 255, 170, 100, 26 }, Color{ 255, 150, 90, 0 }, 20);
        HaloTM({ 70.0f, 12.0f, -70.0f }, 18.0f, Color{ 255, 170, 100, 20 }, Color{ 255, 150, 90, 0 }, 20);

        for (int k = 0; k < 3; k++)
        {
            Vector3 f = FAROLES_MUELLE_TM[k];
            float l = 0.85f + 0.15f * std::sin(t * 6.0f + (float)k * 1.9f);
            HaloTM(f, 1.3f * l, AlfaTM(LUZ_FAROL_TM, 0.5f), AlfaTM(LUZ_FAROL_TM, 0.0f), 12);
            ReflejoAguaTM(f.x, f.z + 0.3f, 6.0f, 0.3f, AlfaTM(LUZ_FAROL_TM, 0.35f), t, k);
        }

        // Ventanas del fuerte.
        HaloTM({ -53.9f, 19.4f, -48.0f }, 2.0f, Color{ 255, 180, 100, 60 }, Color{ 255, 160, 90, 0 }, 12);
        HaloTM({ -50.1f, 19.4f, -48.0f }, 2.0f, Color{ 255, 180, 100, 60 }, Color{ 255, 160, 90, 0 }, 12);
    }

    // Reflejos del barco sobre el agua (galeria de popa y fanales).
    ReflejoAguaTM(-11.4f, MediaMangaTM(-11.4f) + 0.6f, 5.0f, 0.9f, AlfaTM(LUZ_FAROL_TM, 0.4f), t, 7);

    FinLoteTM();

    // Luces que viajan con el barco.
    rlPushMatrix();
    rlRotatef(angulo * RAD2DEG, 0.0f, 0.0f, 1.0f);
    LoteTM();

    // Galeria de popa y camara del capitan.
    HaloTM({ -11.35f, 0.95f, MediaMangaTM(-11.4f) + 0.5f }, 1.6f, Color{ 255, 180, 100, 70 }, Color{ 255, 160, 90, 0 }, 16);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float zc = 1.45f * (float)lado;
        HaloTM({ TOLDILLA_X_TM + 0.15f, 0.75f, zc }, 0.9f, Color{ 255, 180, 100, 80 }, Color{ 255, 160, 90, 0 }, 12);
        DiscoTM({ TOLDILLA_X_TM + 1.0f, 0.02f, zc }, 1.3f, 0.7f, 16, Color{ 255, 170, 90, 40 }, Color{ 255, 170, 90, 0 }, false);
    }

    // Fanales de popa con charco en la toldilla.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float z = 1.4f * (float)lado;
        float l = 0.88f + 0.12f * std::sin(t * 7.0f + (float)lado);
        Vector3 c = { -12.35f, TOLDILLA_Y_TM + 2.12f, z };
        HaloTM(c, 1.5f * l, AlfaTM(LUZ_FAROL_TM, 0.45f), AlfaTM(LUZ_FAROL_TM, 0.0f), 14);
        HaloTM(c, 0.45f, AlfaTM(Color{ 255, 240, 190, 255 }, 0.7f), AlfaTM(LUZ_FAROL_TM, 0.0f), 10);
        DiscoTM({ -11.6f, TOLDILLA_Y_TM + 0.02f, z * 0.7f }, 1.6f, 1.1f, 16, AlfaTM(LUZ_FAROL_TM, 0.16f * l), AlfaTM(LUZ_FAROL_TM, 0.0f), false);
    }

    // Faroles de la borda con su charco en cubierta.
    for (int k = 0; k < 5; k++)
    {
        if (!decoracion && (k % 2) == 1) continue;

        float x = FAROLES_BORDA_TM[k];
        float z = -MediaMangaTM(x) + 0.45f;
        float y = PisoBarcoTM(x) + 1.5f;
        float l = 0.86f + 0.14f * std::sin(t * 6.5f + (float)k * 1.3f);
        HaloTM({ x, y, z }, 1.0f * l, AlfaTM(LUZ_FAROL_TM, 0.42f), AlfaTM(LUZ_FAROL_TM, 0.0f), 12);
        HaloTM({ x, y, z }, 0.3f, AlfaTM(Color{ 255, 240, 190, 255 }, 0.6f), AlfaTM(LUZ_FAROL_TM, 0.0f), 8);
        DiscoTM({ x, PisoBarcoTM(x) + 0.02f, z + 0.6f }, 1.5f, 1.0f, 16, AlfaTM(LUZ_FAROL_TM, 0.12f * l), AlfaTM(LUZ_FAROL_TM, 0.0f), false);
    }

    // Cofas: destello dorado y columna de luz al llegar.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const TrepadorMastil& tr = m.trepadores[i];
        float x = m.posicionX[i];
        float cerca = Lim01TM((tr.altura - 7.0f) / 3.0f);

        if (tr.llego)
        {
            float l = 0.8f + 0.2f * std::sin(t * 6.0f);
            HaloTM({ x, ALTURA_META_MASTIL + 0.8f, 0.3f }, 2.6f * l, Color{ 255, 220, 130, 90 }, Color{ 255, 200, 110, 0 }, 18);
            ConoLuzTM({ x, ALTURA_META_MASTIL + 6.0f, 0.0f }, { x, ALTURA_META_MASTIL + 0.4f, 0.0f }, 0.5f, 1.2f, Color{ 255, 230, 160, 60 });
        }
        else if (cerca > 0.0f)
        {
            float l = 0.6f + 0.4f * std::sin(t * 8.0f);
            AnilloTM({ x, ALTURA_META_MASTIL + COFA_PARED_TM + 0.09f, 0.0f }, COFA_RADIO_TM, COFA_RADIO_TM + 0.35f * cerca, 20, AlfaTM(DORADO_TM, 0.5f * cerca * l), AlfaTM(DORADO_TM, 0.0f));
        }

        // Aviso del cuervo: linea de puntos roja a la altura por donde va a cruzar.
        const CuervoMastil& c = m.cuervos[i];
        if (m.fase == FASE_MASTIL_JUGANDO && c.activo && c.enAviso)
        {
            float parpadeo = std::fmod(t * 8.0f, 1.0f) < 0.6f ? 1.0f : 0.35f;
            float z = PROFUNDIDAD_JUGADOR_MASTIL + 0.25f;

            for (int k = 0; k < 14; k++)
            {
                float u = (float)k / 14.0f;
                float px = x - c.direccion * (RECORRIDO_CUERVO_MASTIL * (1.0f - u)) + c.direccion * 0.6f * u;
                float desliz = std::fmod(t * 3.0f, 1.0f) * 0.5f * c.direccion;
                px += desliz;
                Color cc = AlfaTM(PELIGRO_TM, (0.25f + 0.55f * u) * parpadeo);
                QuadColTM({ px - 0.18f, c.altura - 0.05f, z }, { px + 0.18f, c.altura - 0.05f, z }, { px + 0.18f, c.altura + 0.05f, z }, { px - 0.18f, c.altura + 0.05f, z }, cc, cc, cc, cc);
            }

            HaloTM({ x, c.altura, z }, 0.9f, AlfaTM(PELIGRO_TM, 0.35f * parpadeo), AlfaTM(PELIGRO_TM, 0.0f), 14);
        }

        // Golpe del cuervo: destello rojo breve.
        if (tr.inmunidad > 0.6f)
        {
            float f = (tr.inmunidad - 0.6f) / 0.4f;
            HaloTM({ x, tr.altura + 1.0f, PROFUNDIDAD_JUGADOR_MASTIL + 0.3f }, 1.4f * f + 0.3f, AlfaTM(PELIGRO_TM, 0.5f * f), AlfaTM(PELIGRO_TM, 0.0f), 14);
        }
    }

    FinLoteTM();
    rlPopMatrix();

    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Particulas: plumas de la logica, polvo dorado, toques y confeti
//--------------------------------------------------

static float g_finTM = -1.0f;    // tiempoAnimacion en que termino la partida


static void DibujarParticulasTM(const MinijuegoTrepaMastil& m, int limite, float angulo, bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Polvo dorado al sol (en el mundo).
    if (decoracion)
    {
        LoteTM();
        int cantidad = (int)(40.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < cantidad; k++)
        {
            float s = std::fmod(Ruido01TM(k, 181) + t * (0.015f + 0.015f * Ruido01TM(k, 182)), 1.0f);
            Vector3 q =
            {
                -16.0f + 32.0f * Ruido01TM(k, 183) + 0.5f * std::sin(t * 0.5f + (float)k),
                1.0f + 12.0f * s,
                -4.0f + 6.0f * Ruido01TM(k, 184)
            };
            float a = std::sin(s * PI) * (0.4f + 0.3f * std::sin(t * 3.0f + (float)k));
            PuntoTM(q, 0.03f + 0.02f * Ruido01TM(k, 185), AlfaTM(Color{ 255, 220, 150, 255 }, a));
        }

        FinLoteTM();
    }

    rlPushMatrix();
    rlRotatef(angulo * RAD2DEG, 0.0f, 0.0f, 1.0f);
    LoteTM();

    // Particulas de la logica (plumas del cuervo).
    for (int i = 0; i < MAX_PARTICULAS_MASTIL; i++)
    {
        const ParticulaTierra& p = m.particulas[i];

        if (!p.activa || p.vidaMaxima <= 0.0f) continue;

        float f = Lim01TM(p.vida / p.vidaMaxima);
        Vector3 d = Vector3Scale(g_derTM, p.tamano * 1.6f), u = Vector3Scale(g_arribaTM, p.tamano * 0.6f);
        Color c = AlfaTM(MezclaTM(p.color, Color{ 90, 80, 110, 255 }, 0.3f), 0.5f + 0.5f * f);
        QuadColTM(Vector3Subtract(Vector3Subtract(p.posicion, d), u), Vector3Subtract(Vector3Add(p.posicion, d), u),
            Vector3Add(Vector3Add(p.posicion, d), u), Vector3Add(Vector3Subtract(p.posicion, d), u), c, c, c, c);
    }

    // Toque de cada agarre: soplo de polvo en la mano.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const TrepadorMastil& tr = m.trepadores[i];
        if (tr.pulso <= 0.0f) continue;

        float f = Lim01TM(tr.pulso);
        Vector3 mano = { m.posicionX[i] + tr.mano * 0.32f, tr.altura + 1.25f, PROFUNDIDAD_JUGADOR_MASTIL + 0.1f };

        for (int k = 0; k < 4; k++)
        {
            float a = (float)k * 1.57f + (float)i;
            Vector3 q = { mano.x + std::cos(a) * (1.0f - f) * 0.3f, mano.y + std::sin(a) * (1.0f - f) * 0.3f, mano.z };
            PuntoTM(q, 0.05f * f + 0.02f, AlfaTM(Color{ 255, 240, 210, 255 }, 0.7f * f));
        }
    }

    // Confeti sobre el ganador, desde la cofa.
    if (m.fase == FASE_MASTIL_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (g_finTM < 0.0f || g_finTM > t) g_finTM = t;

        float te = t - g_finTM;
        const Color colores[5] = { Color{ 255, 90, 90, 255 }, Color{ 90, 170, 255, 255 }, Color{ 110, 230, 120, 255 }, DORADO_TM, Color{ 255, 140, 230, 255 } };
        int cantidad = (int)(70.0f * FactorCalidadGrafica(CalidadParticulas())) + 20;

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1) continue;

            float base = m.trepadores[i].altura + 3.2f;

            for (int k = 0; k < cantidad; k++)
            {
                float caida = std::fmod(te * (0.9f + 0.5f * Ruido01TM(k, 191)) + Ruido01TM(k, 192) * 4.0f, 4.0f);
                Vector3 q =
                {
                    m.posicionX[i] + (Ruido01TM(k, 193) - 0.5f) * 3.4f + 0.25f * std::sin(te * 3.0f + (float)k),
                    base - caida,
                    0.6f + (Ruido01TM(k, 194) - 0.5f) * 1.8f
                };
                float giro = te * 6.0f + (float)k;
                Vector3 d = Vector3Scale(g_derTM, 0.08f * std::cos(giro));
                Vector3 u = Vector3Scale(g_arribaTM, 0.05f);
                Color c = colores[k % 5];
                QuadColTM(Vector3Subtract(Vector3Subtract(q, d), u), Vector3Subtract(Vector3Add(q, d), u),
                    Vector3Add(Vector3Add(q, d), u), Vector3Add(Vector3Subtract(q, d), u), c, c, c, c);
            }
        }
    }
    else
    {
        g_finTM = -1.0f;
    }

    FinLoteTM();
    rlPopMatrix();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Anuncios sobre el mundo y cartelitos J#
//--------------------------------------------------

static void DibujarAnunciosTM(
    const MinijuegoTrepaMastil& m,
    const Participante participantes[],
    int limite,
    const Camera3D& vista,
    float angulo
)
{
    float t = m.tiempoAnimacion;
    float escala = EscalaUI();
    float seno = std::sin(angulo), coseno = std::cos(angulo);
    auto Pantalla = [&](float x, float y, float z)
    {
        return GetWorldToScreen({ x * coseno - y * seno, x * seno + y * coseno, z }, vista);
    };

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const TrepadorMastil& tr = m.trepadores[i];
        const Participante& participante = participantes[i];
        int numero = participante.numeroJugador > 0 ? participante.numeroJugador : i + 1;
        float x = m.posicionX[i];

        // Cartelito J# sobre la cabeza.
        Vector2 cabeza = Pantalla(x, tr.altura + 2.05f, PROFUNDIDAD_JUGADOR_MASTIL);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participante.color;
        opciones.pico = true;
        Rectangle placa = DibujarEtiquetaUI(TextFormat("J%d", numero), cabeza, -angulo * RAD2DEG, opciones);
        float arriba = placa.y - 14.0f * escala;

        // Aviso del cuervo: «!» que late del lado por donde entra.
        const CuervoMastil& c = m.cuervos[i];
        if (m.fase == FASE_MASTIL_JUGANDO && c.activo && c.enAviso)
        {
            Vector2 aviso = Pantalla(x - c.direccion * 1.5f, c.altura + 0.15f, PROFUNDIDAD_JUGADOR_MASTIL + 0.3f);
            float latido = 1.0f + 0.25f * std::fabs(std::sin(t * 10.0f));
            float a = std::fmod(t * 8.0f, 1.0f) < 0.6f ? 1.0f : 0.55f;
            DibujarTextoMundoContornoUI("!", aviso.x, aviso.y - 18.0f * escala * latido, UI_TEXTO_GRANDE, PELIGRO_TM, a);
            DibujarTextoMundoContornoUI("¡CUERVO!", aviso.x, aviso.y + 14.0f * escala, UI_TEXTO_PEQUENO, Color{ 255, 150, 130, 255 }, a);
        }

        if (m.fase == FASE_MASTIL_JUGANDO)
        {
            if (tr.inmunidad > 0.3f)
            {
                float f = Lim01TM((tr.inmunidad - 0.3f) / 0.7f);
                float sacudida = std::sin(t * 40.0f) * 3.0f * escala * f;
                DibujarTextoMundoContornoUI("¡PICOTAZO! -1 M", cabeza.x + sacudida, arriba - (1.0f - f) * 14.0f * escala, UI_TEXTO, Color{ 255, 120, 100, 255 }, Lim01TM(f * 2.0f));
            }
            else if (tr.enfriamientoCaida > 0.05f)
            {
                DibujarTextoMundoContornoUI("¡RESBALÓN!", cabeza.x, arriba, UI_TEXTO_PEQUENO, Color{ 210, 214, 224, 255 }, Lim01TM(tr.enfriamientoCaida / 0.15f));
            }
            else if (m.intensidadOlas > 0.05f)
            {
                int efecto = EfectoInclinacionMastil(m, i);
                if (efecto > 0)
                {
                    float salto = std::fabs(std::sin(t * 9.0f)) * 4.0f * escala;
                    DibujarTextoMundoContornoUI("¡A FAVOR!", cabeza.x, arriba - salto, UI_TEXTO_PEQUENO, FAVOR_TM);
                }
                else if (efecto < 0)
                {
                    DibujarTextoMundoContornoUI("EN CONTRA", cabeza.x, arriba, UI_TEXTO_PEQUENO, CONTRA_TM);
                }
            }
        }
        else if (m.fase == FASE_MASTIL_TERMINADO && tr.llego)
        {
            float salto = std::fabs(std::sin(t * 5.0f)) * 6.0f * escala;
            DibujarTextoMundoContornoUI("¡EN LA COFA!", cabeza.x, arriba - salto, UI_TEXTO, Color{ 255, 220, 110, 255 });
        }
    }

    // Anuncio del oleaje cuando empiezan a subir las olas.
    if (m.fase == FASE_MASTIL_JUGANDO)
    {
        float transcurrido = DURACION_PARTIDA_MASTIL - m.tiempoRestante;
        float u = (transcurrido - INICIO_OLAS_MASTIL) / 3.2f;

        if (u > 0.0f && u < 1.0f)
        {
            float a = Lim01TM(u * 6.0f) * Lim01TM((1.0f - u) * 4.0f);
            float y = (float)GetScreenHeight() * 0.27f - (1.0f - Lim01TM(u * 5.0f)) * 20.0f * escala;
            DibujarTextoMundoContornoUI("¡OLEAJE!", (float)GetScreenWidth() * 0.5f, y, UI_TEXTO_GRANDE, Color{ 150, 230, 255, 255 }, a);
            DibujarTextoMundoContornoUI("TREPÁ CUANDO TU MÁSTIL SE INCLINE PARA TU LADO", (float)GetScreenWidth() * 0.5f, y + 34.0f * escala, UI_TEXTO, Color{ 255, 244, 220, 255 }, a);
        }
    }
}


//--------------------------------------------------
// HUD
//--------------------------------------------------

static void PrepararHudTM(
    const MinijuegoTrepaMastil& m,
    const Participante participantes[],
    int limite,
    DatosHudMinijuego& hud
)
{
    PrepararDatosHudMinijuego(
        hud,
        "TREPÁ EL MÁSTIL",
        "¡ALTERNÁ SALTO Y ACCIÓN PARA TREPAR! APROVECHÁ LAS OLAS Y ESQUIVÁ LOS CUERVOS.",
        m.resultado
    );

    hud.fase = m.fase == FASE_MASTIL_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_MASTIL_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_MASTIL;
    hud.segundosAlerta = 6.0f;

    bool alguienLlego = false;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        JugadorHudMinijuego& jugador = hud.jugadores[i];
        const Participante& participante = participantes[i];
        const TrepadorMastil& t = m.trepadores[i];

        if (t.llego) alguienLlego = true;

        jugador.visible = true;
        jugador.valorNumerico = (int)t.altura;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%.1f M", t.altura);
        jugador.progreso = Lim01TM(t.altura / ALTURA_META_MASTIL);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%.1f M", t.altura);

        if (t.llego)
        {
            std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "¡EN LA COFA!");
        }

        if (m.fase == FASE_MASTIL_JUGANDO)
        {
            int efecto = EfectoInclinacionMastil(m, i);

            if (t.inmunidad > 0.4f)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡CUERVO! -1 M");
            else if (m.intensidadOlas > 0.05f && efecto > 0)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "A FAVOR x1.6");
            else if (m.intensidadOlas > 0.05f && efecto < 0)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "EN CONTRA");
            else if (m.cuervos[i].activo && m.cuervos[i].enAviso)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡CUIDADO!");
        }
        else if (m.fase == FASE_MASTIL_TERMINADO && t.llego)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "¡EN LA COFA!");
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "TREPÁ ALTERNANDO\n%s Y %s",
                ObtenerTextoBotonPrincipal(participante),
                TextoAccionMastil(participante)
            );
        }
    }

    hud.finPorTiempo = !alguienLlego;
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoTrepaMastil::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteMastil(cantidadMaxima);
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(CIELO_HORIZONTE_TM);

    Camera3D vista = CamaraVistaTM(*this);
    g_ojoTM = OJO_TM;
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derTM = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaTM = Vector3CrossProduct(g_derTM, frente);

    HornearTM(*this, limite);

    float angulo = AnguloBarcoMastil(inclinacion);
    Matrix giro = MatrixRotateZ(angulo);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Opaco: costa, agua y barco (este gira con las olas).
    DibujarMallaTM(g_hornoTM.costa, g_hornoTM.costaValida);
    DibujarMallaTM(g_hornoTM.agua, g_hornoTM.aguaValida);
    DibujarMallaTM(g_hornoTM.barco, g_hornoTM.barcoValido, giro);

    rlPushMatrix();
    rlRotatef(angulo * RAD2DEG, 0.0f, 0.0f, 1.0f);

    if (resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarBarcoDinamicoTM(*this, jugadores, participantes, limite, decoracion, sombras);
    }

    // Trepadores con el modelo compartido, abrazados al mastil (de
    // espaldas, apenas girados hacia su lado). Tras un picotazo parpadean.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        const JugadorPrueba& j = jugadores[i];
        const TrepadorMastil& tr = trepadores[i];
        Color color = participantes[i].color;

        if (tr.inmunidad > 0.0f && std::fmod(tiempoAnimacion * 14.0f, 1.0f) < 0.4f)
        {
            color = MezclaTM(color, WHITE, 0.6f);
        }

        float giroY = tr.llego ? 0.0f : 180.0f - 18.0f * (float)tr.lado;
        DibujarModeloJugadorEnPosicion({ j.posicion.x, j.posicion.y - 0.7f, j.posicion.z }, giroY, color);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(j), LIME);
        }
    }

    rlPopMatrix();

    DibujarMundoDinamicoTM(*this, angulo, decoracion, efectos);

    // Cielo despues de lo opaco: la prueba de profundidad descarta lo tapado.
    DibujarMallaTM(g_hornoTM.cielo, g_hornoTM.cieloValido, MatrixTranslate(vista.position.x, vista.position.y, vista.position.z));

    DibujarParticulasTM(*this, limite, angulo, decoracion);

    if (efectos)
    {
        DibujarLucesTM(*this, limite, vista, angulo, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    if (resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarAnunciosTM(*this, participantes, limite, vista, angulo);
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    DatosHudMinijuego hud;
    PrepararHudTM(*this, participantes, limite, hud);
    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoTrepaMastil::ObtenerResultado() const
{
    return resultado;
}


void MinijuegoTrepaMastil::Descargar()
{
    LiberarMallasMastil();
    LiberarMaterialHorneado(g_hornoTM.material, g_hornoTM.materialListo);
}
