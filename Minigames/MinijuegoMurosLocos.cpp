#include "Minigames/MinijuegoMurosLocos.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_MUROS = 3.0f;
static const float ESCALA_DIFICULTAD_MUROS = 35.0f;
static const float ANCHO_ARENA_MUROS = 11.4f;
static const float LARGO_ARENA_MUROS = 10.2f;
static const float LIMITE_X_MUROS = ANCHO_ARENA_MUROS / 2.0f;
static const float LIMITE_Z_MUROS = LARGO_ARENA_MUROS / 2.0f;
static const float Z_INICIO_MURO = -6.9f;
static const float Z_FIN_MURO = 6.9f;
static const float ALTO_MURO = 2.45f;
static const float GROSOR_MURO = 0.42f;
static const float FUERZA_MURO = 10.8f;
static const float COOLDOWN_IMPACTO_MURO = 0.30f;


// Efectos solo visuales / de audio (definidos con el dibujo, al final).
static void ActualizarVisualMuros(
    MinijuegoMurosLocos& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[]
);

static void EfectoGolpeMuros(
    MinijuegoMurosLocos& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    Color colorJugador
);

static void EfectoEliminacionMuros(
    MinijuegoMurosLocos& minijuego,
    int indice,
    const JugadorPrueba& jugador
);

static void EmitirPolvoMuros(
    MinijuegoMurosLocos& minijuego,
    Vector3 pies,
    int base
);


static float Limitar01Muros(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static int ContarVivosMuros(const MinijuegoMurosLocos& minijuego)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static bool JugadorSobreArenaMuros(const JugadorPrueba& jugador)
{
    float margen = jugador.tamano.x * 0.16f;

    return
        std::fabs(jugador.posicion.x) <= LIMITE_X_MUROS - margen &&
        std::fabs(jugador.posicion.z) <= LIMITE_Z_MUROS - margen;
}


static bool XEstaEnHuecoMuro(
    const MuroLoco& muro,
    float x,
    float radioJugador
)
{
    for (int i = 0; i < muro.cantidadHuecos; i++)
    {
        float medio = muro.anchoHueco / 2.0f;

        if (
            x - radioJugador >= muro.centrosHueco[i] - medio &&
            x + radioJugador <= muro.centrosHueco[i] + medio
        )
        {
            return true;
        }
    }

    return false;
}


static void PrepararNuevoMuro(MinijuegoMurosLocos& minijuego)
{
    float progreso = Limitar01Muros(
        minijuego.tiempoJugado / ESCALA_DIFICULTAD_MUROS
    );

    minijuego.numeroMuro++;
    minijuego.muro = {};
    minijuego.muro.z = Z_INICIO_MURO;
    minijuego.muro.velocidad = 4.1f + progreso * 4.6f;
    minijuego.muro.anchoHueco = 2.65f - progreso * 0.82f;

    if (minijuego.muro.anchoHueco < 1.72f)
    {
        minijuego.muro.anchoHueco = 1.72f;
    }

    minijuego.muro.cantidadHuecos =
        progreso > 0.52f && GetRandomValue(0, 100) < 38
        ? 2
        : 1;

    if (minijuego.muro.cantidadHuecos == 1)
    {
        minijuego.muro.centrosHueco[0] =
            (float)GetRandomValue(-34, 34) / 10.0f;
    }
    else
    {
        minijuego.muro.centrosHueco[0] =
            (float)GetRandomValue(-38, -15) / 10.0f;
        minijuego.muro.centrosHueco[1] =
            (float)GetRandomValue(15, 38) / 10.0f;
    }
}


static void CrearImpactoMuro(
    JugadorPrueba& jugador,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    // El muro ya corrige la posicion de forma continua. Este impulso es
    // solamente el golpe extra de contacto, siempre en la direccion del muro.
    jugador.empuje.z += FUERZA_MURO * 0.42f;

    CrearParticulasImpactoGolpe(
        particulas,
        cantidadParticulas,
        { jugador.posicion.x, jugador.posicion.y + 0.12f, jugador.posicion.z }
    );
}


static bool ResolverColisionMuroJugador(
    const MuroLoco& muro,
    JugadorPrueba& jugador
)
{
    if (jugador.cayendo) return false;

    float radioJugador = jugador.tamano.x * 0.46f;

    bool alturaPeligrosa =
        jugador.posicion.y - jugador.tamano.y / 2.0f < ALTO_MURO &&
        jugador.posicion.y + jugador.tamano.y / 2.0f > 0.0f;

    if (
        !alturaPeligrosa ||
        XEstaEnHuecoMuro(muro, jugador.posicion.x, radioJugador)
    )
    {
        return false;
    }

    float caraDelantera =
        muro.z + GROSOR_MURO * 0.5f + radioJugador;

    float caraTrasera =
        muro.z - GROSOR_MURO * 0.5f - radioJugador;

    // Los jugadores empiezan delante del muro. Si el frente movil alcanza su
    // centro, los dejamos pegados a la cara delantera. De este modo nunca
    // pueden atravesarlo aunque el muro avance varios centimetros por frame.
    bool alcanzado =
        jugador.posicion.z >= caraTrasera &&
        jugador.posicion.z <= caraDelantera + 0.08f;

    if (!alcanzado) return false;

    jugador.posicion.z = caraDelantera + 0.012f;

    if (jugador.velocidad.z < muro.velocidad)
    {
        jugador.velocidad.z = muro.velocidad;
    }

    if (jugador.empuje.z < muro.velocidad * 0.35f)
    {
        jugador.empuje.z = muro.velocidad * 0.35f;
    }

    return true;
}


// IA simple: al aparecer cada muro el bot elige el hueco mas cercano, se
// ubica en una fila propia (para no estorbarse) y reacciona con retraso. La
// probabilidad de equivocarse sube con la dificultad, asi que siempre hay
// un ganador antes de que los muros se vuelvan imposibles.
static InputMinijuegoParticipante CrearEntradaBotMuros(
    MinijuegoMurosLocos& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    EstadoJugadorMurosLocos& bot = minijuego.estadosJugadores[indice];

    if (bot.muroBot != minijuego.numeroMuro)
    {
        const float filasZ[MAX_JUGADORES_PRUEBA] =
        {
            2.6f, 1.5f, 0.4f, 3.7f
        };

        bool primeraVez = bot.muroBot < 0;
        bot.muroBot = minijuego.numeroMuro;
        bot.retrasoBot = (float)GetRandomValue(40, 320) / 1000.0f;

        if (primeraVez)
        {
            bot.objetivoZ =
                filasZ[indice % MAX_JUGADORES_PRUEBA] +
                (float)GetRandomValue(-20, 20) / 100.0f;
            bot.objetivoX = jugador.posicion.x;
        }

        const MuroLoco& muro = minijuego.muro;
        float mejorCentro = muro.centrosHueco[0];

        for (int i = 1; i < muro.cantidadHuecos; i++)
        {
            if (
                std::fabs(muro.centrosHueco[i] - jugador.posicion.x) <
                std::fabs(mejorCentro - jugador.posicion.x)
            )
            {
                mejorCentro = muro.centrosHueco[i];
            }
        }

        float margen = muro.anchoHueco * 0.5f - jugador.tamano.x * 0.46f - 0.12f;
        if (margen < 0.05f) margen = 0.05f;
        if (margen > 0.45f) margen = 0.45f;

        float progreso = Limitar01Muros(
            minijuego.tiempoJugado / ESCALA_DIFICULTAD_MUROS
        );
        float probabilidadError = 0.05f + progreso * 0.30f;

        if ((float)GetRandomValue(0, 999) / 1000.0f < probabilidadError)
        {
            // Error: se queda donde estaba o corre hacia un lado al azar.
            bot.objetivoX = (float)GetRandomValue(-38, 38) / 10.0f;
        }
        else
        {
            bot.objetivoX =
                mejorCentro +
                (float)GetRandomValue(-1000, 1000) / 1000.0f * margen;
        }
    }

    if (bot.retrasoBot > 0.0f)
    {
        bot.retrasoBot -= deltaTime;
        return InputMinijuegoParticipante{};
    }

    return CrearEntradaBotHaciaObjetivo1v3(
        jugador.posicion,
        { bot.objetivoX, 0.0f, bot.objetivoZ },
        0.14f
    );
}


static void FinalizarMuros(MinijuegoMurosLocos& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO) return;

    int vivos = ContarVivosMuros(minijuego);

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        vivos == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;

    int tiempoFinalMs = (int)std::lround(minijuego.tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo) continue;

        EstadoJugadorMurosLocos& estadoJugador = minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_MUROS_TERMINADO;
}


void MinijuegoMurosLocos::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++) estadosJugadores[i] = {};

    tiempoAnimacion = 0.0f;
    semillaVisual = 2891336453u;
    confetiLanzado = false;
    muroDerrumbado = 0;
    muroAnunciado = 0;
    tiempoAnuncio = -1.0f;
    tipoAnuncio = 0;
    dobleHuecoVisto = false;
    for (int i = 0; i < MAX_PARTICULAS_MUROS; i++) particulasEfecto[i] = {};
    for (int i = 0; i < MAX_AVISOS_MUROS; i++) avisos[i] = {};

    suelo = {};
    suelo.posicion = { 0.0f, -0.35f, 0.0f };
    suelo.posicionInicial = suelo.posicion;
    suelo.tamano = { ANCHO_ARENA_MUROS, 0.70f, LARGO_ARENA_MUROS };
    suelo.color = Color{ 232, 206, 150, 255 };
    suelo.activaColision = true;

    fase = FASE_MUROS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_MUROS;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;
    tiempoEntreMuros = 0.30f;
    numeroMuro = 0;

    muro = {};
    PrepararNuevoMuro(*this);

    // Solo encuadre: algo mas baja que antes para que entren las ruinas,
    // las dunas y la franja de cielo con calima detras del patio.
    camara.position = { 0.0f, 7.4f, 11.4f };
    camara.target = { 0.0f, 0.9f, -2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 56.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoMurosLocos::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.4f, 1.05f, 2.8f },
        {  2.4f, 1.05f, 2.8f },
        { -0.9f, 1.05f, 1.2f },
        {  0.9f, 1.05f, 1.2f }
    };

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawns[i]);
    }
}


void MinijuegoMurosLocos::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participaban[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participaban[i] = resultado.participantes[i].participo;
    }

    Inicializar();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo = participaban[i];
        if (participaban[i]) resultado.cantidadParticipantes++;
    }

    ConfigurarJugadores(jugadores, cantidadMaxima);
}


void MinijuegoMurosLocos::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoMinijuego(
            resultado,
            participantes,
            FORMATO_MINIJUEGO_INDIVIDUAL
        );
    }

    // Animaciones y particulas siguen tambien con el minijuego terminado.
    ActualizarVisualMuros(*this, deltaTime, jugadores);

    if (fase == FASE_MUROS_TERMINADO) return;

    if (fase == FASE_MUROS_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_MUROS_JUGANDO;
        }

        return;
    }

    tiempoJugado += deltaTime;
    int vivosAntes = ContarVivosMuros(*this);

    if (tiempoEntreMuros > 0.0f)
    {
        tiempoEntreMuros -= deltaTime;
    }
    else
    {
        muro.z += muro.velocidad * deltaTime;

        if (muro.z > Z_FIN_MURO)
        {
            float progreso = Limitar01Muros(
                tiempoJugado / ESCALA_DIFICULTAD_MUROS
            );

            tiempoEntreMuros = 0.52f - progreso * 0.27f;
            if (tiempoEntreMuros < 0.20f) tiempoEntreMuros = 0.20f;
            PrepararNuevoMuro(*this);
        }
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        EstadoJugadorMurosLocos& estadoJugador = estadosJugadores[i];
        JugadorPrueba& jugador = jugadores[i];

        if (estadoJugador.cooldownImpacto > 0.0f)
        {
            estadoJugador.cooldownImpacto -= deltaTime;
            if (estadoJugador.cooldownImpacto < 0.0f)
                estadoJugador.cooldownImpacto = 0.0f;
        }

        InputMinijuegoParticipante entrada{};
        if (participantes[i].esBot || !participantes[i].conectado)
            entrada = CrearEntradaBotMuros(*this, i, jugador, deltaTime);
        else
            entrada = LeerInputMinijuegoParticipante(participantes[i]);

        BloquePrueba sueloJugador = suelo;
        sueloJugador.activaColision = JugadorSobreArenaMuros(jugador);

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            true,
            false,
            deltaTime
        );

        // Solo visual / audio: polvo al aterrizar y sonido de salto.
        if (jugador.enSuelo && !estadoJugador.enSueloAnterior)
        {
            EmitirPolvoMuros(*this, jugador.posicion, 5);
        }
        else if (
            !jugador.enSuelo &&
            estadoJugador.enSueloAnterior &&
            jugador.velocidad.y > 0.5f &&
            !participantes[i].esBot
        )
        {
            ReproducirSonidoMinijuego(audio, SONIDO_SALTO);
        }

        estadoJugador.enSueloAnterior = jugador.enSuelo;

        if (tiempoEntreMuros <= 0.0f)
        {
            bool contacto = ResolverColisionMuroJugador(muro, jugador);

            if (contacto && estadoJugador.cooldownImpacto <= 0.0f)
            {
                CrearImpactoMuro(jugador, particulas, cantidadParticulas);
                estadoJugador.cooldownImpacto = COOLDOWN_IMPACTO_MURO;
                EfectoGolpeMuros(*this, i, jugador, participantes[i].color);
            }
        }

        if (
            !JugadorSobreArenaMuros(jugador) &&
            jugador.posicion.y < -2.1f
        )
        {
            jugador.cayendo = true;
        }
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        participantes,
        cantidadMaxima,
        particulas,
        cantidadParticulas
    );

    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminados++;
        }
    }

    int posicion = vivosAntes - eliminados + 1;
    int tiempoMs = (int)std::lround(tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal = posicion;
            estadosJugadores[i].tiempoSobrevividoMs = tiempoMs;
            EfectoEliminacionMuros(*this, i, jugadores[i]);
        }
    }

    // Tope duro de seguridad: la ronda nunca queda abierta indefinidamente.
    if (vivosAntes - eliminados <= 1 || tiempoJugado > 150.0f)
    {
        FinalizarMuros(*this);
    }
}



//==================================================
// EFECTOS VISUALES (sin efecto en la logica)
//==================================================

// Fondo del foso de arena que rodea el patio.
static const float NIVEL_FOSO_MUROS = -4.0f;

// Borde exterior del foso (lo de afuera es la explanada de ruinas).
static const float FOSO_X_MUROS = 7.4f;
static const float FOSO_Z_MUROS = 7.6f;

// Alto visual total del muro: baja hasta el fondo del foso, asi sube
// desde la ranura del fondo y se hunde en el foso del frente.
static const float BASE_MURO_VISUAL = NIVEL_FOSO_MUROS - 0.2f;

static const float DURACION_AVISO_MUROS = 1.6f;
static const float DURACION_ANUNCIO_MUROS = 1.7f;

// Acento de la escena: el turquesa de la loza, reservado para el hueco.
static const Color COLOR_ACENTO_MUROS = { 64, 214, 200, 255 };
static const Color COLOR_ACENTO_CLARO_MUROS = { 170, 255, 236, 255 };


static float Limitar01VisualMuros(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


// Hash determinista para la decoracion (misma escena en cada partida).
static float Hash01Muros(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarMuros(MinijuegoMurosLocos& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasMuros(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static void EmitirParticulaMuros(
    MinijuegoMurosLocos& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    float freno,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_MUROS; i++)
    {
        ParticulaMurosLocos& p = minijuego.particulasEfecto[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.freno = freno;
        p.color = color;
        return;
    }
}


// Polvillo de arena al aterrizar.
static void EmitirPolvoMuros(
    MinijuegoMurosLocos& minijuego,
    Vector3 pies,
    int base
)
{
    int cantidad = CantidadParticulasMuros(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarMuros(minijuego, 0.0f, 6.2832f);
        float r = AzarMuros(minijuego, 0.8f, 2.0f);

        EmitirParticulaMuros(
            minijuego,
            { pies.x + std::cos(a) * 0.25f, 0.06f, pies.z + std::sin(a) * 0.25f },
            { std::cos(a) * r, AzarMuros(minijuego, 0.3f, 1.1f), std::sin(a) * r },
            AzarMuros(minijuego, 0.35f, 0.65f),
            AzarMuros(minijuego, 0.08f, 0.15f),
            1.5f,
            3.5f,
            Color{ 238, 214, 170, 255 }
        );
    }
}


// Empujon del muro: terrones de adobe, polvo y chispas del jugador.
static void EfectoGolpeMuros(
    MinijuegoMurosLocos& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    Color colorJugador
)
{
    minijuego.estadosJugadores[indice].destelloGolpe = 1.0f;

    int cantidad = CantidadParticulasMuros(16);
    float zMuro = jugador.posicion.z - jugador.tamano.x * 0.46f;

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarMuros(minijuego, 0.4f, 2.74f);
        float r = AzarMuros(minijuego, 1.5f, 4.2f);
        Color color = k % 3 == 0
            ? Color{ 196, 128, 80, 255 }
            : (k % 3 == 1 ? Color{ 236, 210, 164, 255 } : colorJugador);

        EmitirParticulaMuros(
            minijuego,
            { jugador.posicion.x + AzarMuros(minijuego, -0.3f, 0.3f), jugador.posicion.y + AzarMuros(minijuego, -0.3f, 0.5f), zMuro },
            { std::cos(a) * r, AzarMuros(minijuego, 1.2f, 4.0f), std::sin(a) * r * 0.8f + 1.0f },
            AzarMuros(minijuego, 0.35f, 0.7f),
            AzarMuros(minijuego, 0.07f, 0.14f),
            10.0f,
            1.8f,
            color
        );
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GOLPE);
}


// Eliminado: sigue cayendo al foso (solo dibujo), levanta arena y se
// hunde; aparece «¡J# AL FOSO!» en el borde por donde salio.
static void EfectoEliminacionMuros(
    MinijuegoMurosLocos& minijuego,
    int indice,
    const JugadorPrueba& jugador
)
{
    EstadoJugadorMurosLocos& estado = minijuego.estadosJugadores[indice];

    estado.tiempoCaida = 0.0f;
    estado.posicionCaida = jugador.posicion;
    estado.velocidadCaida =
    {
        jugador.velocidad.x * 0.5f,
        jugador.velocidad.y < -2.0f ? jugador.velocidad.y : -2.0f,
        (jugador.velocidad.z + jugador.empuje.z) * 0.35f
    };
    estado.tocoArena = false;
    estado.hundimiento = 0.0f;

    float bx = jugador.posicion.x;
    float bz = jugador.posicion.z;
    if (bx > LIMITE_X_MUROS) bx = LIMITE_X_MUROS;
    if (bx < -LIMITE_X_MUROS) bx = -LIMITE_X_MUROS;
    if (bz > LIMITE_Z_MUROS) bz = LIMITE_Z_MUROS;
    if (bz < -LIMITE_Z_MUROS) bz = -LIMITE_Z_MUROS;

    for (int i = 0; i < MAX_AVISOS_MUROS; i++)
    {
        AvisoMurosLocos& aviso = minijuego.avisos[i];

        if (aviso.activo)
        {
            continue;
        }

        aviso.activo = true;
        aviso.jugador = indice;
        aviso.posicion = { bx, 0.6f, bz };
        aviso.vida = DURACION_AVISO_MUROS;
        break;
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
}


// Nube de arena cuando un eliminado cae al fondo del foso.
static void EmitirArenaFosoMuros(
    MinijuegoMurosLocos& minijuego,
    Vector3 centro
)
{
    int cantidad = CantidadParticulasMuros(22);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarMuros(minijuego, 0.0f, 6.2832f);
        float r = AzarMuros(minijuego, 0.6f, 2.4f);
        Color color = k % 2 == 0 ? Color{ 240, 216, 172, 255 } : Color{ 220, 186, 136, 255 };

        EmitirParticulaMuros(
            minijuego,
            { centro.x + std::cos(a) * 0.4f, NIVEL_FOSO_MUROS + 0.2f, centro.z + std::sin(a) * 0.4f },
            { std::cos(a) * r, AzarMuros(minijuego, 3.5f, 7.5f), std::sin(a) * r },
            AzarMuros(minijuego, 0.9f, 1.5f),
            AzarMuros(minijuego, 0.18f, 0.36f),
            5.0f,
            1.4f,
            color
        );
    }
}


// El muro llega al foso del frente: polvareda a lo largo del borde.
static void EmitirDerrumbeMuros(MinijuegoMurosLocos& minijuego)
{
    int cantidad = CantidadParticulasMuros(26);

    for (int k = 0; k < cantidad; k++)
    {
        float x = AzarMuros(minijuego, -LIMITE_X_MUROS, LIMITE_X_MUROS);

        if (XEstaEnHuecoMuro(minijuego.muro, x, 0.0f))
        {
            continue;
        }

        EmitirParticulaMuros(
            minijuego,
            { x, AzarMuros(minijuego, -1.6f, -0.2f), LIMITE_Z_MUROS + AzarMuros(minijuego, 0.4f, 1.4f) },
            { AzarMuros(minijuego, -0.8f, 0.8f), AzarMuros(minijuego, 2.5f, 5.0f), AzarMuros(minijuego, 0.3f, 1.6f) },
            AzarMuros(minijuego, 0.7f, 1.2f),
            AzarMuros(minijuego, 0.2f, 0.4f),
            4.0f,
            1.5f,
            k % 3 == 0 ? Color{ 200, 134, 86, 255 } : Color{ 236, 210, 166, 255 }
        );
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ATERRIZAJE);
}


static void EmitirConfetiMuros(
    MinijuegoMurosLocos& minijuego,
    Vector3 centro
)
{
    const Color colores[6] =
    {
        { 255, 214, 84, 255 }, { 64, 214, 200, 255 }, { 232, 110, 70, 255 },
        { 90, 96, 200, 255 }, { 150, 220, 130, 255 }, { 250, 246, 236, 255 }
    };

    int cantidad = CantidadParticulasMuros(100);

    for (int k = 0; k < cantidad; k++)
    {
        EmitirParticulaMuros(
            minijuego,
            {
                centro.x + AzarMuros(minijuego, -2.6f, 2.6f),
                AzarMuros(minijuego, 3.6f, 5.8f),
                centro.z + AzarMuros(minijuego, -2.0f, 2.0f)
            },
            {
                AzarMuros(minijuego, -0.8f, 0.8f),
                AzarMuros(minijuego, -0.4f, 1.2f),
                AzarMuros(minijuego, -0.6f, 0.6f)
            },
            AzarMuros(minijuego, 2.4f, 3.8f),
            AzarMuros(minijuego, 0.07f, 0.12f),
            1.6f,
            1.2f,
            colores[k % 6]
        );
    }
}


static void ActualizarVisualMuros(
    MinijuegoMurosLocos& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[]
)
{
    minijuego.tiempoAnimacion += deltaTime;

    for (int i = 0; i < MAX_PARTICULAS_MUROS; i++)
    {
        ParticulaMurosLocos& p = minijuego.particulasEfecto[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f || p.posicion.y < NIVEL_FOSO_MUROS - 0.5f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        float freno = std::exp(-p.freno * deltaTime);
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;
    }

    for (int i = 0; i < MAX_AVISOS_MUROS; i++)
    {
        AvisoMurosLocos& aviso = minijuego.avisos[i];

        if (!aviso.activo)
        {
            continue;
        }

        aviso.vida -= deltaTime;
        if (aviso.vida <= 0.0f) aviso.activo = false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorMurosLocos& estado = minijuego.estadosJugadores[i];

        estado.destelloGolpe -= deltaTime * 2.4f;
        if (estado.destelloGolpe < 0.0f) estado.destelloGolpe = 0.0f;

        if (estado.tiempoCaida < 0.0f)
        {
            continue;
        }

        estado.tiempoCaida += deltaTime;

        if (!estado.tocoArena)
        {
            estado.velocidadCaida.y -= 16.0f * deltaTime;
            estado.posicionCaida.x += estado.velocidadCaida.x * deltaTime;
            estado.posicionCaida.y += estado.velocidadCaida.y * deltaTime;
            estado.posicionCaida.z += estado.velocidadCaida.z * deltaTime;

            // No atraviesa las paredes del foso.
            float limiteX = FOSO_X_MUROS - 0.45f;
            float limiteZ = FOSO_Z_MUROS - 0.45f;
            if (estado.posicionCaida.x > limiteX) estado.posicionCaida.x = limiteX;
            if (estado.posicionCaida.x < -limiteX) estado.posicionCaida.x = -limiteX;
            if (estado.posicionCaida.z > limiteZ) estado.posicionCaida.z = limiteZ;
            if (estado.posicionCaida.z < -limiteZ) estado.posicionCaida.z = -limiteZ;

            if (estado.posicionCaida.y - 0.7f <= NIVEL_FOSO_MUROS)
            {
                estado.posicionCaida.y = NIVEL_FOSO_MUROS + 0.7f;
                estado.tocoArena = true;
                EmitirArenaFosoMuros(minijuego, estado.posicionCaida);
            }
        }
        else
        {
            estado.hundimiento += deltaTime / 1.3f;
            if (estado.hundimiento > 1.0f) estado.hundimiento = 1.0f;
        }
    }

    // El muro pasa el borde del frente y se derrumba en el foso.
    if (
        minijuego.fase == FASE_MUROS_JUGANDO &&
        minijuego.tiempoEntreMuros <= 0.0f &&
        minijuego.muro.z > LIMITE_Z_MUROS + 0.35f &&
        minijuego.muroDerrumbado != minijuego.numeroMuro
    )
    {
        minijuego.muroDerrumbado = minijuego.numeroMuro;
        EmitirDerrumbeMuros(minijuego);
    }

    // Anuncios: el primer muro, el primer muro de dos huecos y cada cinco.
    if (minijuego.tiempoAnuncio >= 0.0f)
    {
        minijuego.tiempoAnuncio += deltaTime;
        if (minijuego.tiempoAnuncio > DURACION_ANUNCIO_MUROS) minijuego.tiempoAnuncio = -1.0f;
    }

    if (
        minijuego.fase == FASE_MUROS_JUGANDO &&
        minijuego.tiempoEntreMuros <= 0.0f &&
        minijuego.numeroMuro != minijuego.muroAnunciado
    )
    {
        minijuego.muroAnunciado = minijuego.numeroMuro;
        int tipo = 0;

        if (minijuego.numeroMuro == 1) tipo = 1;
        else if (minijuego.muro.cantidadHuecos == 2 && !minijuego.dobleHuecoVisto) tipo = 2;
        else if (minijuego.numeroMuro % 5 == 0) tipo = 3;

        if (minijuego.muro.cantidadHuecos == 2) minijuego.dobleHuecoVisto = true;

        if (tipo != 0)
        {
            minijuego.tipoAnuncio = tipo;
            minijuego.tiempoAnuncio = 0.0f;
        }
    }

    // Confeti sobre el ultimo que quedo en el patio.
    if (minijuego.fase == FASE_MUROS_TERMINADO && !minijuego.confetiLanzado)
    {
        minijuego.confetiLanzado = true;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                minijuego.resultado.participantes[i].participo &&
                !minijuego.estadosJugadores[i].eliminado
            )
            {
                EmitirConfetiMuros(minijuego, jugadores[i].posicion);
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
            }
        }
    }
}


//==================================================
// PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Ruinas de adobe al mediodia: barro cocido ocre, arenisca clara, loza
// turquesa (acento, solo en los marcos del hueco), telas indigo y una
// calima dorada que se come lo lejano. Sol alto, un poco detras.

static const Color COLOR_CALIMA_MUROS = { 238, 214, 178, 255 };
static const Color COLOR_ADOBE_MUROS = { 198, 134, 86, 255 };
static const Color COLOR_ADOBE_OSCURO_MUROS = { 156, 100, 64, 255 };
static const Color COLOR_ARENISCA_MUROS = { 228, 194, 142, 255 };
static const Color COLOR_ARENA_MUROS = { 230, 196, 144, 255 };
static const Color COLOR_BALDOSA_MUROS = { 206, 170, 124, 255 };
static const Color COLOR_JUNTA_MUROS = { 142, 104, 72, 255 };
static const Color COLOR_MADERA_MUROS = { 112, 76, 50, 255 };
static const Color COLOR_INDIGO_MUROS = { 62, 66, 124, 255 };
static const Color COLOR_TERRACOTA_MUROS = { 180, 94, 60, 255 };
static const Color COLOR_PALMA_MUROS = { 178, 144, 76, 255 };

// Ojo de referencia para la niebla (la camara de juego).
static const Vector3 OJO_NIEBLA_MUROS = { 0.0f, 7.4f, 11.4f };


static unsigned char CanalMuros(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoMuros(Color color, float factor)
{
    return Color{
        CanalMuros((float)color.r * factor),
        CanalMuros((float)color.g * factor),
        CanalMuros((float)color.b * factor),
        color.a
    };
}


// Caras de arriba con sol blanco calido; las laterales, sombra azulada.
static Color SombraMuros(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalMuros((float)color.r * factor * 1.04f),
            CanalMuros((float)color.g * factor * 1.02f),
            CanalMuros((float)color.b * factor * 0.96f),
            color.a
        };
    }

    return Color{
        CanalMuros((float)color.r * factor * 0.95f),
        CanalMuros((float)color.g * factor * 0.97f),
        CanalMuros((float)color.b * factor * 1.06f),
        color.a
    };
}


static Color MezclaMuros(Color a, Color b, float t)
{
    t = Limitar01VisualMuros(t);

    return Color{
        CanalMuros((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalMuros((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalMuros((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalMuros((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


// Calima: lo lejano se funde con el polvo dorado del aire.
static float CantidadNieblaMuros(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_MUROS.x;
    float dy = p.y - OJO_NIEBLA_MUROS.y;
    float dz = p.z - OJO_NIEBLA_MUROS.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 16.0f) / 70.0f;
    return n < 0.0f ? 0.0f : (n > 0.9f ? 0.9f : n);
}


static Color TinteMuros(Color color, Vector3 p)
{
    Color niebla = COLOR_CALIMA_MUROS;
    niebla.a = color.a;
    return MezclaMuros(color, niebla, CantidadNieblaMuros(p));
}


static void VerticeMuros(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadMuros(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeMuros(a, color);
    VerticeMuros(b, color);
    VerticeMuros(c, color);
    VerticeMuros(d, color);
}


// Quad con la calima calculada por vertice (suelos y dunas grandes).
static void QuadNieblaMuros(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeMuros(a, TinteMuros(color, a));
    VerticeMuros(b, TinteMuros(color, b));
    VerticeMuros(c, TinteMuros(color, c));
    VerticeMuros(d, TinteMuros(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.80, izquierda 0.72,
// derecha 0.62, fondo 0.56). Respeta la matriz actual.
static void CuboMuros(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadMuros({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraMuros(color, 1.0f, true));
    QuadMuros({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraMuros(color, 0.80f, false));
    QuadMuros({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraMuros(color, 0.56f, false));
    QuadMuros({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraMuros(color, 0.62f, false));
    QuadMuros({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraMuros(color, 0.72f, false));
    QuadMuros({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, SombraMuros(color, 0.45f, false));
    rlEnd();
}


// Igual que CuboMuros pero con la calima del lugar.
static void CuboNieblaMuros(Vector3 c, Vector3 t, Color color)
{
    CuboMuros(c, t, TinteMuros(color, c));
}


// Luz falsa de las caras laterales segun su orientacion (angulo en XZ:
// 0 = +X, 90 = +Z hacia la camara).
static float LuzLateralMuros(float angulo)
{
    return 0.70f + 0.12f * std::sin(angulo) - 0.08f * std::cos(angulo + 0.6f);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (r1).
static void PrismaMuros(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    float giro = 0.0f,
    bool estriado = false
)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = SombraMuros(color, 1.0f, true);
    Color tapaAbajo = SombraMuros(color, 0.45f, false);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float f = LuzLateralMuros(a0 + paso * 0.5f);
        if (estriado && (k & 1)) f *= 0.9f;
        Color cara = SombraMuros(color, f, false);

        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadMuros(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadMuros(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }

        if (r0 > 0.001f)
        {
            Vector3 centro = { base.x, base.y, base.z };
            QuadMuros(centro, { base.x + c0 * r0, base.y, base.z + s0 * r0 }, { base.x + c1 * r0, base.y, base.z + s1 * r0 }, centro, tapaAbajo);
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloMuros(Vector3 c, float r0, float r1, Color color, int lados = 32)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadMuros(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano; rx, rz radios).
static void ResplandorMuros(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloMuros(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorMuros(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano inclinado como la vista).
static void ResplandorFrenteMuros(Vector3 c, float radio, Color color)
{
    ResplandorMuros(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.89f, 0.45f }, radio, color);
}


// Cupula de arena (montículo, duna cercana): luz por normal y calima.
static void DunaMuros(Vector3 c, float radioX, float radioZ, float alto, Color color)
{
    const int ANILLOS = 5;
    const int LADOS = 16;
    const float LX = 0.36f, LY = 0.86f, LZ = -0.36f;

    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        float t0 = 1.5708f * (float)a / (float)ANILLOS;
        float t1 = 1.5708f * (float)(a + 1) / (float)ANILLOS;

        for (int k = 0; k < LADOS; k++)
        {
            float p0 = 6.2832f * (float)k / (float)LADOS;
            float p1 = 6.2832f * (float)(k + 1) / (float)LADOS;
            Vector3 v[4];
            float tt[4] = { t0, t0, t1, t1 };
            float pp[4] = { p0, p1, p1, p0 };

            for (int q = 0; q < 4; q++)
            {
                float ct = std::cos(tt[q]);
                float st = std::sin(tt[q]);
                float nx = ct * std::cos(pp[q]);
                float nz = ct * std::sin(pp[q]);
                v[q] = { c.x + nx * radioX, c.y + st * alto, c.z + nz * radioZ };

                float luz = nx * LX + st * LY + nz * LZ;
                float f = 0.66f + 0.38f * Limitar01VisualMuros(luz);
                Color cv = SombraMuros(color, f, luz > 0.6f);
                VerticeMuros(v[q], TinteMuros(cv, v[q]));
            }
        }
    }

    rlEnd();
}


//==================================================
// LADRILLOS DE ADOBE
//==================================================

// Cara de ladrillos: rectangulo vertical que arranca en "origen", corre
// "largo" unidades por "eje" (horizontal) entre y0 e y1, con relieve hacia
// "normal". Primero la junta, despues cada ladrillo con su variacion,
// un filo de luz arriba y una sombra abajo.
static void LadrillosMuros(
    Vector3 origen,
    Vector3 eje,
    float largo,
    float y0,
    float y1,
    Vector3 normal,
    float anchoLadrillo,
    float altoLadrillo,
    Color ladrillo,
    Color junta,
    float luz,
    int semilla,
    bool niebla
)
{
    if (largo <= 0.01f || y1 - y0 <= 0.01f) return;

    auto Punto = [&](float s, float y, float relieve) -> Vector3
    {
        return {
            origen.x + eje.x * s + normal.x * relieve,
            y,
            origen.z + eje.z * s + normal.z * relieve
        };
    };

    Color colorJunta = SombraMuros(junta, luz, false);
    if (niebla) colorJunta = TinteMuros(colorJunta, Punto(largo * 0.5f, (y0 + y1) * 0.5f, 0.0f));

    rlBegin(RL_QUADS);
    QuadMuros(Punto(0.0f, y0, 0.0f), Punto(largo, y0, 0.0f), Punto(largo, y1, 0.0f), Punto(0.0f, y1, 0.0f), colorJunta);

    // Coordenada global de la fila para que el aparejo no "salte" cuando
    // la cara cambia de largo (huecos del muro).
    float baseS = origen.x * eje.x + origen.z * eje.z;
    int filas = (int)std::ceil((y1 - y0) / altoLadrillo - 0.01f);
    const float JUNTA = 0.035f;

    for (int f = 0; f < filas; f++)
    {
        float fy0 = y1 - altoLadrillo * (float)(f + 1);
        float fy1 = y1 - altoLadrillo * (float)f;
        if (fy0 < y0) fy0 = y0;
        if (fy1 - fy0 < JUNTA * 2.0f) continue;

        float desfase = (f & 1) ? anchoLadrillo * 0.5f : 0.0f;
        float sGlobal0 = baseS;
        int primera = (int)std::floor((sGlobal0 - desfase) / anchoLadrillo);

        for (int col = primera; ; col++)
        {
            float g0 = (float)col * anchoLadrillo + desfase;
            float g1 = g0 + anchoLadrillo;
            float s0 = g0 - baseS;
            float s1 = g1 - baseS;
            if (s0 >= largo) break;
            if (s0 < 0.0f) s0 = 0.0f;
            if (s1 > largo) s1 = largo;
            if (s1 - s0 < JUNTA * 2.0f) continue;

            float h = Hash01Muros(col * 7 + semilla, f * 13 + semilla * 3);
            float var = 0.88f + 0.22f * h;
            Color c = TonoMuros(ladrillo, var);
            if (Hash01Muros(col + semilla * 5, f + 77) > 0.9f) c = MezclaMuros(c, junta, 0.35f);

            Vector3 a = Punto(s0 + JUNTA, fy0 + JUNTA, 0.012f);
            Vector3 b = Punto(s1 - JUNTA, fy0 + JUNTA, 0.012f);
            Vector3 cc = Punto(s1 - JUNTA, fy1 - JUNTA, 0.012f);
            Vector3 d = Punto(s0 + JUNTA, fy1 - JUNTA, 0.012f);
            Color cara = SombraMuros(c, luz, false);
            if (niebla) cara = TinteMuros(cara, a);
            QuadMuros(a, b, cc, d, cara);

            // Filo de luz arriba (el sol cae desde arriba).
            Color filo = SombraMuros(c, luz * 1.16f, true);
            if (niebla) filo = TinteMuros(filo, a);
            float alto = fy1 - fy0;
            QuadMuros(
                Punto(s0 + JUNTA, fy1 - JUNTA - alto * 0.16f, 0.014f),
                Punto(s1 - JUNTA, fy1 - JUNTA - alto * 0.16f, 0.014f),
                cc,
                d,
                filo
            );
        }
    }

    rlEnd();
}


//==================================================
// CIELO Y HORIZONTE
//==================================================

// Cielo de mediodia (2D, antes del 3D): celeste palido arriba que se
// vuelve polvo dorado hacia el horizonte. Sol alto con su resplandor.
static void DibujarCieloMuros(const Camera3D& vista, float t)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    float horizonte = GetWorldToScreen({ 0.0f, 0.0f, -400.0f }, vista).y;
    if (horizonte < (float)alto * 0.10f) horizonte = (float)alto * 0.10f;
    if (horizonte > (float)alto * 0.55f) horizonte = (float)alto * 0.55f;

    int h1 = (int)(horizonte * 0.55f);
    int h2 = (int)horizonte;

    DrawRectangleGradientV(0, 0, ancho, h1, Color{ 112, 168, 214, 255 }, Color{ 176, 204, 214, 255 });
    DrawRectangleGradientV(0, h1, ancho, h2 - h1, Color{ 176, 204, 214, 255 }, Color{ 236, 220, 190, 255 });
    DrawRectangleGradientV(0, h2, ancho, alto - h2, Color{ 236, 220, 190, 255 }, COLOR_CALIMA_MUROS);

    // Sol de mediodia: alto, apenas a la izquierda; disco blanco y halo.
    Vector2 sol = { (float)ancho * 0.30f, (float)alto * 0.02f };
    float pulso = 1.0f + 0.025f * std::sin(t * 0.8f);
    DrawCircleGradient(sol, (float)alto * 0.55f * pulso, Fade(Color{ 255, 246, 220, 255 }, 0.35f), Fade(Color{ 255, 240, 210, 255 }, 0.0f));
    DrawCircleGradient(sol, (float)alto * 0.16f, Fade(Color{ 255, 252, 236, 255 }, 0.9f), Fade(Color{ 255, 246, 220, 255 }, 0.0f));
    DrawCircleV(sol, (float)alto * 0.05f, Color{ 255, 253, 242, 255 });

    // Velos de nube muy finos que cruzan despacio.
    for (int k = 0; k < 5; k++)
    {
        float vel = 5.0f + 4.0f * Hash01Muros(k, 20);
        float x = std::fmod(Hash01Muros(k, 21) * (float)(ancho + 600) + t * vel, (float)(ancho + 600)) - 300.0f;
        float y = (float)h1 * (0.3f + 0.9f * Hash01Muros(k, 22));
        float largo = 160.0f + 180.0f * Hash01Muros(k, 23);
        DrawEllipse((int)x, (int)y, largo, 4.0f + 3.0f * Hash01Muros(k, 24), Fade(Color{ 255, 250, 240, 255 }, 0.45f));
        DrawEllipse((int)(x + largo * 0.3f), (int)(y - 5.0f), largo * 0.45f, 2.5f, Fade(Color{ 255, 250, 240, 255 }, 0.3f));
    }
}


// Cordon de dunas: laderas que miran a la camara con luz segun la
// pendiente y una cresta clara. zFrente es el pie, zCresta la cima.
static void CordonDunasMuros(
    float zFrente,
    float zCresta,
    float xMin,
    float xMax,
    float paso,
    float altoBase,
    float amplitud,
    int semilla,
    Color color
)
{
    rlBegin(RL_QUADS);

    for (float x = xMin; x < xMax; x += paso)
    {
        float x1 = x + paso;
        float f1 = 0.11f + 0.05f * Hash01Muros(semilla, 1);
        float f2 = 0.047f + 0.02f * Hash01Muros(semilla, 2);
        auto Altura = [&](float xx) -> float
        {
            float o = 0.5f + 0.5f * std::sin(xx * f1 + (float)semilla);
            float p = 0.5f + 0.5f * std::sin(xx * f2 + (float)semilla * 2.3f);
            return altoBase + amplitud * (0.35f * o + 0.65f * p * p);
        };

        float h0 = Altura(x);
        float h1 = Altura(x1);
        float pendiente = (h1 - h0) / paso;

        // Ladera iluminada si mira hacia el sol (a la izquierda).
        float luz = 0.86f - 0.5f * pendiente;
        if (luz < 0.64f) luz = 0.64f;
        if (luz > 1.04f) luz = 1.04f;
        Color ladera = SombraMuros(color, luz, luz > 0.9f);

        float zc0 = zCresta + std::sin(x * 0.21f + (float)semilla) * 2.0f;
        float zc1 = zCresta + std::sin(x1 * 0.21f + (float)semilla) * 2.0f;

        QuadNieblaMuros({ x, 0.0f, zFrente }, { x1, 0.0f, zFrente }, { x1, h1, zc1 }, { x, h0, zc0 }, ladera);

        // Cresta: filo claro y ladera de atras en sombra.
        Color cresta = SombraMuros(color, 1.08f, true);
        QuadNieblaMuros({ x, h0 - 0.18f, zc0 + 0.3f }, { x1, h1 - 0.18f, zc1 + 0.3f }, { x1, h1, zc1 }, { x, h0, zc0 }, cresta);
        Color atras = SombraMuros(color, 0.7f, false);
        QuadNieblaMuros({ x, h0, zc0 }, { x1, h1, zc1 }, { x1, 0.0f, zc1 - 12.0f }, { x, 0.0f, zc0 - 12.0f }, atras);
    }

    rlEnd();
}


// Zigurat de adobe lejano, desdibujado por la calima.
static void ZiguratLejanoMuros(Vector3 base, float escala)
{
    for (int k = 0; k < 4; k++)
    {
        float lado = escala * (4.0f - (float)k * 0.85f);
        float alto = escala * 0.9f;
        CuboNieblaMuros({ base.x, base.y + alto * ((float)k + 0.5f), base.z }, { lado, alto, lado }, COLOR_ADOBE_MUROS);
    }

    // Rampa frontal.
    CuboNieblaMuros({ base.x, base.y + escala * 1.4f, base.z + escala * 2.0f }, { escala * 0.7f, escala * 0.3f, escala * 1.6f }, COLOR_ARENISCA_MUROS);
}


// Suelo del desierto lejano, dunas en dos planos y ruinas en la calima.
static void DibujarHorizonteMuros(float t)
{
    (void)t;

    // Llanura de arena con calima por vertice, en franjas.
    rlBegin(RL_QUADS);
    const float zs[6] = { -11.0f, -24.0f, -45.0f, -80.0f, -140.0f, -260.0f };
    for (int i = 0; i < 5; i++)
    {
        QuadNieblaMuros(
            { -260.0f, -0.02f, zs[i] }, { 260.0f, -0.02f, zs[i] },
            { 260.0f, -0.02f, zs[i + 1] }, { -260.0f, -0.02f, zs[i + 1] },
            TonoMuros(COLOR_ARENA_MUROS, 0.96f)
        );
    }
    rlEnd();

    bool simple = CalidadDecoracion() != CALIDAD_ALTA;

    // Plano lejano: dunas altas y palidas.
    CordonDunasMuros(-58.0f, -70.0f, -150.0f, 150.0f, simple ? 10.0f : 5.0f, 3.0f, 9.0f, 3, Color{ 232, 186, 132, 255 });

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        ZiguratLejanoMuros({ -38.0f, 0.0f, -80.0f }, 2.6f);

        // Obelisco quebrado a la derecha.
        CuboNieblaMuros({ 30.0f, 4.0f, -64.0f }, { 1.6f, 8.0f, 1.6f }, COLOR_ARENISCA_MUROS);
        PrismaMuros({ 30.0f, 8.0f, -64.0f }, 1.0f, 0.2f, 1.6f, 4, TinteMuros(COLOR_ARENISCA_MUROS, { 30.0f, 8.0f, -64.0f }), 0.785f);
    }

    // Plano cercano: dunas mas bajas y ocres.
    CordonDunasMuros(-26.0f, -34.0f, -90.0f, 90.0f, simple ? 6.0f : 3.0f, 1.2f, 4.6f, 7, Color{ 222, 164, 104, 255 });
}


// Buitres que planean en circulos sobre las ruinas.
static void DibujarBuitresMuros(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    for (int k = 0; k < 3; k++)
    {
        float radio = 5.0f + 2.0f * Hash01Muros(k, 30);
        float a = t * (0.32f + 0.08f * (float)k) + (float)k * 2.1f;
        Vector3 c = { -6.0f + 9.0f * (float)k, 11.5f + 1.2f * Hash01Muros(k, 31), -22.0f - 6.0f * (float)k };
        Vector3 p = { c.x + std::cos(a) * radio, c.y + std::sin(a * 2.0f) * 0.3f, c.z + std::sin(a) * radio * 0.5f };
        float rumbo = a + 1.5708f;
        float aleteo = std::sin(t * 2.0f + (float)k) * 0.12f;
        Color color = TinteMuros(Color{ 58, 44, 40, 255 }, p);

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(-rumbo * RAD2DEG, 0.0f, 1.0f, 0.0f);
        rlRotatef(std::sin(a) * 18.0f, 1.0f, 0.0f, 0.0f);

        rlBegin(RL_TRIANGLES);
        for (int s = -1; s <= 1; s += 2)
        {
            float fs = (float)s;
            VerticeMuros({ 0.0f, 0.0f, 0.35f }, color);
            VerticeMuros({ 1.5f * fs, 0.25f + aleteo, -0.1f }, color);
            VerticeMuros({ 0.0f, 0.0f, -0.25f }, color);
        }
        VerticeMuros({ -0.12f, 0.0f, -0.2f }, color);
        VerticeMuros({ 0.12f, 0.0f, -0.2f }, color);
        VerticeMuros({ 0.0f, 0.0f, -0.6f }, color);
        rlEnd();
        rlPopMatrix();
    }
}


//==================================================
// RUINAS: MEDIA DISTANCIA
//==================================================

// Palos de madera que asoman del adobe (torons), en grilla.
static void ToronesMuros(float x0, float x1, float y0, float y1, float zCara, float pasoX, float pasoY, int semilla)
{
    for (float y = y0; y <= y1; y += pasoY)
    {
        int fila = (int)(y * 10.0f);
        float desfase = (fila & 1) ? pasoX * 0.5f : 0.0f;

        for (float x = x0 + desfase; x <= x1; x += pasoX)
        {
            if (Hash01Muros((int)(x * 10.0f) + semilla, fila) < 0.18f) continue;
            CuboNieblaMuros({ x, y, zCara + 0.22f }, { 0.12f, 0.12f, 0.44f }, COLOR_MADERA_MUROS);
        }
    }
}


// Pinaculo de adobe (remate en punta tipico de la arquitectura de barro).
static void PinaculoMuros(Vector3 base, float ancho, float alto, Color color)
{
    Color c = TinteMuros(color, base);
    CuboMuros({ base.x, base.y + alto * 0.3f, base.z }, { ancho, alto * 0.6f, ancho }, c);
    PrismaMuros({ base.x, base.y + alto * 0.6f, base.z }, ancho * 0.7f, 0.04f, alto * 0.4f, 4, c, 0.785f);
}


// Paño de muralla con arco: dos machones, el relleno sobre el arco en
// rebanadas y una rosca de dovelas claras.
static void MuroConArcoMuros(
    float xCentro,
    float zCentro,
    float ancho,
    float alto,
    float anchoAbertura,
    float altoImposta,
    float profundidad,
    int semilla
)
{
    float r = anchoAbertura * 0.5f;
    float machon = (ancho - anchoAbertura) * 0.5f;
    Color adobe = COLOR_ADOBE_MUROS;
    float zCara = zCentro + profundidad * 0.5f;

    for (int s = -1; s <= 1; s += 2)
    {
        float xm = xCentro + (float)s * (r + machon * 0.5f);
        CuboNieblaMuros({ xm, alto * 0.5f, zCentro }, { machon, alto, profundidad }, adobe);
        LadrillosMuros({ xm - machon * 0.5f, 0.0f, zCara }, { 1.0f, 0.0f, 0.0f }, machon, 0.0f, alto, { 0.0f, 0.0f, 1.0f }, 0.9f, 0.42f, adobe, COLOR_ADOBE_OSCURO_MUROS, 0.82f, semilla + s, true);
    }

    // Relleno sobre el arco por rebanadas verticales.
    const int REBANADAS = 10;
    for (int k = 0; k < REBANADAS; k++)
    {
        float u0 = -r + anchoAbertura * (float)k / (float)REBANADAS;
        float u1 = u0 + anchoAbertura / (float)REBANADAS;
        float um = (u0 + u1) * 0.5f;
        float yArco = altoImposta + std::sqrt(r * r - um * um > 0.0f ? r * r - um * um : 0.0f);
        float h = alto - yArco;
        if (h <= 0.02f) continue;
        CuboNieblaMuros({ xCentro + um, yArco + h * 0.5f, zCentro }, { u1 - u0 + 0.01f, h, profundidad }, adobe);
    }

    // Rosca de dovelas de arenisca.
    const int DOVELAS = 9;
    for (int k = 0; k < DOVELAS; k++)
    {
        float a = 3.1416f * ((float)k + 0.5f) / (float)DOVELAS;
        Vector3 p = { xCentro + std::cos(a) * (r + 0.12f), altoImposta + std::sin(a) * (r + 0.12f), zCara + 0.04f };
        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(a * RAD2DEG - 90.0f, 0.0f, 0.0f, 1.0f);
        CuboMuros({ 0.0f, 0.0f, 0.0f }, { 3.1416f * r / (float)DOVELAS * 0.9f, 0.3f, 0.12f }, TinteMuros(COLOR_ARENISCA_MUROS, p));
        rlPopMatrix();
    }

    // Impostas.
    for (int s = -1; s <= 1; s += 2)
    {
        CuboNieblaMuros({ xCentro + (float)s * (r + 0.1f), altoImposta, zCara + 0.04f }, { 0.4f, 0.16f, 0.14f }, COLOR_ARENISCA_MUROS);
    }
}


// Torre de adobe ataludada con ventanas, torons y pinaculos.
static void TorreMuros(Vector3 base, float lado, float alto, bool rota, int semilla)
{
    const int TRAMOS = 3;
    float altoTramo = alto / (float)TRAMOS;

    for (int k = 0; k < TRAMOS; k++)
    {
        float l = lado * (1.0f - 0.07f * (float)k);
        float y0 = base.y + altoTramo * (float)k;
        float h = altoTramo;

        if (rota && k == TRAMOS - 1)
        {
            // Tramo derrumbado: mitad izquierda mas alta que la derecha.
            CuboNieblaMuros({ base.x - l * 0.25f, y0 + h * 0.5f, base.z }, { l * 0.5f, h, l }, COLOR_ADOBE_MUROS);
            CuboNieblaMuros({ base.x + l * 0.25f, y0 + h * 0.22f, base.z }, { l * 0.5f, h * 0.44f, l }, COLOR_ADOBE_MUROS);
            continue;
        }

        CuboNieblaMuros({ base.x, y0 + h * 0.5f, base.z }, { l, h, l }, COLOR_ADOBE_MUROS);
        LadrillosMuros({ base.x - l * 0.5f, 0.0f, base.z + l * 0.5f }, { 1.0f, 0.0f, 0.0f }, l, y0, y0 + h, { 0.0f, 0.0f, 1.0f }, 0.9f, 0.45f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.8f, semilla + k, true);

        // Ventanita oscura con dintel de madera.
        Vector3 v = { base.x + (k == 1 ? -l * 0.18f : l * 0.12f), y0 + h * 0.55f, base.z + l * 0.5f + 0.02f };
        Color hueco = TinteMuros(Color{ 54, 34, 26, 255 }, v);
        rlBegin(RL_QUADS);
        QuadMuros({ v.x - 0.25f, v.y - 0.45f, v.z }, { v.x + 0.25f, v.y - 0.45f, v.z }, { v.x + 0.25f, v.y + 0.45f, v.z }, { v.x - 0.25f, v.y + 0.45f, v.z }, hueco);
        rlEnd();
        CuboNieblaMuros({ v.x, v.y + 0.52f, v.z + 0.04f }, { 0.75f, 0.1f, 0.12f }, COLOR_MADERA_MUROS);

        ToronesMuros(base.x - l * 0.42f, base.x + l * 0.42f, y0 + 0.4f, y0 + h - 0.3f, base.z + l * 0.5f, 0.75f, 1.1f, semilla * 3 + k);
    }

    if (!rota)
    {
        float l = lado * (1.0f - 0.07f * (float)(TRAMOS - 1));
        float yTope = base.y + alto;
        CuboNieblaMuros({ base.x, yTope + 0.08f, base.z }, { l + 0.2f, 0.16f, l + 0.2f }, COLOR_ARENISCA_MUROS);

        for (int s = -1; s <= 1; s += 2)
        {
            for (int q = -1; q <= 1; q += 2)
            {
                PinaculoMuros({ base.x + (float)s * l * 0.4f, yTope + 0.16f, base.z + (float)q * l * 0.4f }, 0.36f, 0.9f, COLOR_ADOBE_MUROS);
            }
        }
    }
}


// Fachada del templo en ruinas al fondo, con arcos, contrafuertes y
// dos torres. MODELO FUTURO: templo de adobe GLB.
static void DibujarFachadaMuros()
{
    const float Z = -15.0f;
    const float PROF = 1.2f;
    const float ANCHO_PANO = 4.0f;

    for (int k = -7; k <= 6; k++)
    {
        float x0 = (float)k * ANCHO_PANO;
        float xc = x0 + ANCHO_PANO * 0.5f;
        float h = 4.2f + 2.6f * Hash01Muros(k, 50);

        if (k == -1 || k == 0 || k == 2 || k == -4)
        {
            // Paños con arco: por ellos se ven las dunas.
            MuroConArcoMuros(xc, Z, ANCHO_PANO, h + 0.8f, 2.4f, 2.4f, PROF, k * 5);
        }
        else if (Hash01Muros(k, 51) < 0.25f)
        {
            // Paño derrumbado: escalones de adobe y un monticulo de arena.
            CuboNieblaMuros({ xc - 0.9f, 1.3f, Z }, { 2.2f, 2.6f, PROF }, COLOR_ADOBE_MUROS);
            CuboNieblaMuros({ xc + 1.0f, 0.7f, Z }, { 2.0f, 1.4f, PROF }, COLOR_ADOBE_MUROS);
            DunaMuros({ xc, 0.0f, Z + 1.0f }, 2.6f, 1.6f, 1.1f, COLOR_ARENA_MUROS);
        }
        else
        {
            // Paño macizo con remate dentado.
            CuboNieblaMuros({ xc, h * 0.5f, Z }, { ANCHO_PANO, h, PROF }, COLOR_ADOBE_MUROS);
            LadrillosMuros({ x0, 0.0f, Z + PROF * 0.5f }, { 1.0f, 0.0f, 0.0f }, ANCHO_PANO, 0.0f, h, { 0.0f, 0.0f, 1.0f }, 0.9f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.82f, k * 11, true);
            ToronesMuros(x0 + 0.4f, x0 + ANCHO_PANO - 0.4f, 1.0f, h - 0.6f, Z + PROF * 0.5f, 0.9f, 1.25f, k);

            for (int d = 0; d < 4; d++)
            {
                float hx = x0 + 0.5f + (float)d * 1.0f;
                float alto = 0.35f + 0.35f * Hash01Muros(k * 4 + d, 52);
                CuboNieblaMuros({ hx, h + alto * 0.5f, Z }, { 0.55f, alto, PROF * 0.9f }, COLOR_ADOBE_MUROS);
            }
        }

        // Contrafuerte entre paños (uno de cada dos), con pinaculo.
        if ((k & 1) == 0)
        {
            float hc = 4.8f + 1.2f * Hash01Muros(k, 53);
            CuboNieblaMuros({ x0, hc * 0.5f, Z + 0.3f }, { 0.7f, hc, PROF + 0.6f }, TonoMuros(COLOR_ADOBE_MUROS, 0.94f));
            PinaculoMuros({ x0, hc, Z + 0.3f }, 0.44f, 0.8f, TonoMuros(COLOR_ADOBE_MUROS, 0.94f));
        }
    }

    // Banda de arena acumulada al pie de la fachada.
    for (int k = -6; k <= 6; k++)
    {
        DunaMuros({ (float)k * 4.2f + 1.3f * Hash01Muros(k, 54), 0.0f, Z + 1.0f }, 2.4f + Hash01Muros(k, 55), 1.2f, 0.5f + 0.4f * Hash01Muros(k, 56), COLOR_ARENA_MUROS);
    }

    TorreMuros({ -13.0f, 0.0f, -13.4f }, 3.2f, 9.6f, false, 61);
    TorreMuros({ 13.6f, 0.0f, -13.8f }, 3.0f, 7.6f, true, 67);
}


// Columna (rota o entera) de tambores de arenisca.
static void ColumnaMuros(Vector3 base, float alto, bool rota, int semilla)
{
    Color piedra = TinteMuros(COLOR_ARENISCA_MUROS, base);
    CuboMuros({ base.x, base.y + 0.15f, base.z }, { 0.9f, 0.3f, 0.9f }, TonoMuros(piedra, 0.92f));

    float y = base.y + 0.3f;
    int tambores = (int)(alto / 0.7f);

    for (int k = 0; k < tambores; k++)
    {
        float var = 0.92f + 0.12f * Hash01Muros(semilla, k);
        PrismaMuros({ base.x, y, base.z }, 0.32f, 0.31f, 0.68f, 10, TonoMuros(piedra, var), (float)k * 0.2f, true);
        y += 0.7f;
    }

    if (rota)
    {
        // Quiebre: un trozo inclinado y un tambor caido al pie.
        rlPushMatrix();
        rlTranslatef(base.x, y, base.z);
        rlRotatef(14.0f, 0.3f, 0.0f, 1.0f);
        PrismaMuros({ 0.0f, 0.0f, 0.0f }, 0.31f, 0.28f, 0.32f, 10, TonoMuros(piedra, 0.95f), 0.0f, true);
        rlPopMatrix();

        rlPushMatrix();
        rlTranslatef(base.x + 0.7f + 0.3f * Hash01Muros(semilla, 9), base.y + 0.31f, base.z + 0.5f);
        rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        rlRotatef(30.0f + 60.0f * Hash01Muros(semilla, 8), 1.0f, 0.0f, 0.0f);
        PrismaMuros({ 0.0f, -0.34f, 0.0f }, 0.31f, 0.31f, 0.68f, 10, TonoMuros(piedra, 0.9f), 0.0f, true);
        rlPopMatrix();
    }
    else
    {
        // Capitel.
        CuboMuros({ base.x, y + 0.12f, base.z }, { 0.85f, 0.24f, 0.85f }, piedra);
        CuboMuros({ base.x, y + 0.3f, base.z }, { 1.0f, 0.14f, 1.0f }, TonoMuros(piedra, 1.04f));
    }
}


// Columnatas a los costados de la explanada.
static void DibujarColumnatasMuros(int paso)
{
    const float zs[6] = { -10.5f, -7.5f, -4.5f, -1.5f, 1.5f, 4.5f };

    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * 9.4f;

        for (int k = 0; k < 6; k += paso)
        {
            int semilla = k * 3 + (s > 0 ? 40 : 0);
            float h = Hash01Muros(semilla, 1);
            // Las que sostienen el arquitrabe quedan enteras.
            bool sostiene = s > 0 ? (k == 0 || k == 1) : (k == 2 || k == 3);
            bool rota = h < 0.55f && !sostiene;
            float alto = rota ? 1.4f + 2.0f * h : 3.6f;
            ColumnaMuros({ x, 0.0f, zs[k] }, alto, rota, semilla);
        }

        // Arquitrabe que todavia une dos columnas enteras.
        if (paso == 1)
        {
            CuboNieblaMuros({ x, 4.25f, (float)s > 0 ? -9.0f : -3.0f }, { 1.0f, 0.5f, 3.9f }, COLOR_ARENISCA_MUROS);
        }
    }

    // Muros bajos laterales detras de las columnas.
    for (int s = -1; s <= 1; s += 2)
    {
        for (int k = 0; k < 4; k += paso)
        {
            float z = -11.0f + (float)k * 3.6f;
            float h = 1.2f + 1.6f * Hash01Muros(k, s > 0 ? 70 : 71);
            float x = (float)s * 12.6f;
            CuboNieblaMuros({ x, h * 0.5f, z }, { 1.0f, h, 3.2f }, COLOR_ADOBE_MUROS);
            LadrillosMuros({ x - (float)s * 0.5f, 0.0f, z + (float)s * 1.6f }, { 0.0f, 0.0f, -(float)s }, 3.2f, 0.0f, h, { -(float)s, 0.0f, 0.0f }, 0.9f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, s > 0 ? 0.68f : 0.76f, k * 9 + s, true);
        }

        // Dunas que se comen los costados.
        DunaMuros({ (float)s * 17.0f, 0.0f, -6.0f }, 5.0f, 7.0f, 2.2f, COLOR_ARENA_MUROS);
        DunaMuros({ (float)s * 16.0f, 0.0f, 4.0f }, 4.0f, 5.0f, 1.6f, COLOR_ARENA_MUROS);
    }
}


//==================================================
// RUINAS: ANILLO CERCANO
//==================================================

// Palmera seca: tronco anillado que se inclina y hojas pardas caidas.
// MODELO FUTURO: palmera seca GLB.
static void PalmeraSecaMuros(Vector3 base, float alto, float inclinacion, float t, int semilla)
{
    const int TRAMOS = 7;
    Vector3 p = base;
    float dx = std::cos(inclinacion);
    float dz = std::sin(inclinacion);
    Color tronco = TinteMuros(Color{ 138, 104, 72, 255 }, base);

    for (int k = 0; k < TRAMOS; k++)
    {
        float u = (float)(k + 1) / (float)TRAMOS;
        float curva = u * u * 1.4f;
        Vector3 q = { base.x + dx * curva, base.y + alto * u, base.z + dz * curva };
        float r0 = 0.2f - 0.07f * (float)k / (float)TRAMOS;

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        float ix = (q.x - p.x) / (alto / (float)TRAMOS);
        float iz = (q.z - p.z) / (alto / (float)TRAMOS);
        rlRotatef(-ix * 40.0f, 0.0f, 0.0f, 1.0f);
        rlRotatef(iz * 40.0f, 1.0f, 0.0f, 0.0f);
        PrismaMuros({ 0.0f, 0.0f, 0.0f }, r0, r0 * 0.86f, alto / (float)TRAMOS + 0.04f, 7, TonoMuros(tronco, k & 1 ? 0.9f : 1.0f));
        rlPopMatrix();
        p = q;
    }

    // Hojas secas que se mecen con el viento.
    const int HOJAS = 8;
    for (int k = 0; k < HOJAS; k++)
    {
        float a = 6.2832f * (float)k / (float)HOJAS + Hash01Muros(semilla, k) * 0.5f;
        float vaiven = std::sin(t * 1.6f + (float)k + (float)semilla) * 0.12f;
        float largo = 1.6f + 0.6f * Hash01Muros(semilla, k + 10);
        float caida = 0.5f + 0.5f * Hash01Muros(semilla, k + 20) + vaiven;
        Color hoja = TinteMuros(k % 3 == 0 ? Color{ 150, 116, 62, 255 } : COLOR_PALMA_MUROS, p);
        Color nervio = TonoMuros(hoja, 0.8f);

        float ca = std::cos(a), sa = std::sin(a);
        Vector3 medio = { p.x + ca * largo * 0.55f, p.y + 0.25f - caida * 0.2f, p.z + sa * largo * 0.55f };
        Vector3 punta = { p.x + ca * largo, p.y - caida * 1.1f, p.z + sa * largo };
        float ox = -sa * 0.24f, oz = ca * 0.24f;

        rlBegin(RL_QUADS);
        QuadMuros({ p.x, p.y, p.z }, { medio.x + ox, medio.y, medio.z + oz }, { medio.x, medio.y - 0.03f, medio.z }, { p.x, p.y, p.z }, hoja);
        QuadMuros({ p.x, p.y, p.z }, { medio.x, medio.y - 0.03f, medio.z }, { medio.x - ox, medio.y, medio.z - oz }, { p.x, p.y, p.z }, nervio);
        QuadMuros({ medio.x + ox, medio.y, medio.z + oz }, { punta.x, punta.y, punta.z }, { punta.x, punta.y, punta.z }, { medio.x - ox, medio.y, medio.z - oz }, hoja);
        rlEnd();
    }

    // Racimo de datiles secos.
    CuboMuros({ p.x, p.y - 0.25f, p.z }, { 0.3f, 0.3f, 0.3f }, TinteMuros(Color{ 120, 70, 40, 255 }, p));
}


// Vasija de barro (perfil por anillos). MODELO FUTURO: vasija GLB.
static void VasijaMuros(Vector3 base, float escala, Color color, bool rota)
{
    Color c = TinteMuros(color, base);
    const float radios[6] = { 0.16f, 0.27f, 0.33f, 0.28f, 0.14f, 0.18f };
    const float alturas[6] = { 0.0f, 0.18f, 0.38f, 0.6f, 0.78f, 0.86f };
    int limite = rota ? 3 : 5;

    for (int k = 0; k < limite; k++)
    {
        PrismaMuros(
            { base.x, base.y + alturas[k] * escala, base.z },
            radios[k] * escala,
            radios[k + 1] * escala,
            (alturas[k + 1] - alturas[k]) * escala,
            12,
            TonoMuros(c, k == 2 ? 1.04f : 1.0f)
        );
    }

    // Franja pintada en la panza.
    PrismaMuros({ base.x, base.y + 0.36f * escala, base.z }, 0.335f * escala, 0.335f * escala, 0.05f * escala, 12, TinteMuros(Color{ 92, 54, 40, 255 }, base));

    if (rota)
    {
        // Tiestos alrededor.
        for (int k = 0; k < 3; k++)
        {
            float a = (float)k * 2.1f;
            CuboMuros({ base.x + std::cos(a) * 0.5f * escala, base.y + 0.03f, base.z + std::sin(a) * 0.5f * escala }, { 0.2f * escala, 0.05f, 0.14f * escala }, c);
        }
    }
}


// Brasero de piedra con fuego (la luz va en la pasada aditiva).
static void BraseroMuros(Vector3 base, float t, int semilla)
{
    Color piedra = TinteMuros(COLOR_ARENISCA_MUROS, base);
    PrismaMuros({ base.x, base.y, base.z }, 0.42f, 0.32f, 0.2f, 8, TonoMuros(piedra, 0.9f));
    PrismaMuros({ base.x, base.y + 0.2f, base.z }, 0.2f, 0.18f, 0.8f, 8, piedra);
    PrismaMuros({ base.x, base.y + 1.0f, base.z }, 0.22f, 0.55f, 0.32f, 10, TonoMuros(piedra, 1.02f));
    AnilloMuros({ base.x, base.y + 1.31f, base.z }, 0.0f, 0.5f, Color{ 60, 36, 28, 255 }, 12);

    // Llamas: conos que titilan.
    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.57f + (float)semilla;
        float h = 0.55f + 0.25f * std::sin(t * (9.0f + (float)k) + (float)semilla);
        Vector3 p = { base.x + std::cos(a) * 0.14f, base.y + 1.3f, base.z + std::sin(a) * 0.14f };
        PrismaMuros(p, 0.16f, 0.0f, h, 6, k % 2 == 0 ? Color{ 255, 150, 50, 255 } : Color{ 255, 206, 90, 255 }, t * 2.0f);
    }
    PrismaMuros({ base.x, base.y + 1.3f, base.z }, 0.12f, 0.0f, 0.95f + 0.2f * std::sin(t * 11.0f), 6, Color{ 255, 236, 160, 255 });
}


// Bandera de tela que ondea (cuelga de "arriba" y baja "largo").
static void TelaOndeanteMuros(Vector3 arriba, float ancho, float largo, float t, float fase, Color tela, Color franja)
{
    const int TRAMOS = 8;

    rlBegin(RL_QUADS);
    for (int k = 0; k < TRAMOS; k++)
    {
        float u0 = (float)k / (float)TRAMOS;
        float u1 = (float)(k + 1) / (float)TRAMOS;
        float o0 = std::sin(t * 2.6f + fase + u0 * 4.0f) * 0.16f * u0;
        float o1 = std::sin(t * 2.6f + fase + u1 * 4.0f) * 0.16f * u1;
        float y0 = arriba.y - largo * u0;
        float y1 = arriba.y - largo * u1;
        bool enFranja = k == 1 || k == TRAMOS - 2;
        Color c = enFranja ? franja : tela;
        float luz = 0.82f + 0.18f * std::sin(t * 2.6f + fase + u0 * 4.0f + 0.8f);
        Color cara = TinteMuros(SombraMuros(c, luz, false), arriba);

        QuadMuros(
            { arriba.x - ancho * 0.5f + o0, y0, arriba.z + o0 },
            { arriba.x + ancho * 0.5f + o0, y0, arriba.z + o0 },
            { arriba.x + ancho * 0.5f + o1, y1, arriba.z + o1 },
            { arriba.x - ancho * 0.5f + o1, y1, arriba.z + o1 },
            cara
        );
    }
    rlEnd();
}


// Toldo de tela a rayas sobre cuatro palos, con borde que flamea.
static void ToldoMuros(Vector3 c, float t)
{
    const float AX = 1.3f, AZ = 1.1f, ALTO = 2.4f;

    for (int s = -1; s <= 1; s += 2)
    {
        for (int q = -1; q <= 1; q += 2)
        {
            CuboNieblaMuros({ c.x + (float)s * AX, ALTO * 0.5f, c.z + (float)q * AZ }, { 0.1f, ALTO, 0.1f }, COLOR_MADERA_MUROS);
        }
    }

    const int FRANJAS = 6;
    rlBegin(RL_QUADS);
    for (int k = 0; k < FRANJAS; k++)
    {
        float x0 = c.x - AX + 2.0f * AX * (float)k / (float)FRANJAS;
        float x1 = x0 + 2.0f * AX / (float)FRANJAS;
        float onda0 = std::sin(t * 3.0f + x0 * 2.0f) * 0.05f;
        float onda1 = std::sin(t * 3.0f + x1 * 2.0f) * 0.05f;
        Color tela = k % 2 == 0 ? COLOR_INDIGO_MUROS : Color{ 236, 222, 196, 255 };
        Color arriba = TinteMuros(SombraMuros(tela, 1.0f, true), c);
        QuadMuros({ x0, ALTO + 0.15f + onda0, c.z - AZ }, { x1, ALTO + 0.15f + onda1, c.z - AZ }, { x1, ALTO - 0.1f + onda1, c.z + AZ }, { x0, ALTO - 0.1f + onda0, c.z + AZ }, arriba);

        // Faldon delantero que flamea.
        float f0 = std::sin(t * 4.0f + x0 * 3.0f) * 0.08f;
        float f1 = std::sin(t * 4.0f + x1 * 3.0f) * 0.08f;
        Color faldon = TinteMuros(SombraMuros(tela, 0.82f, false), c);
        QuadMuros({ x0, ALTO - 0.1f + onda0, c.z + AZ }, { x1, ALTO - 0.1f + onda1, c.z + AZ }, { x1, ALTO - 0.45f, c.z + AZ + 0.06f + f1 }, { x0, ALTO - 0.45f, c.z + AZ + 0.06f + f0 }, faldon);
    }
    rlEnd();

    // Debajo: alfombra, cesto y un cantaro.
    rlBegin(RL_QUADS);
    QuadMuros({ c.x - 0.9f, 0.012f, c.z - 0.7f }, { c.x + 0.9f, 0.012f, c.z - 0.7f }, { c.x + 0.9f, 0.012f, c.z + 0.7f }, { c.x - 0.9f, 0.012f, c.z + 0.7f }, TinteMuros(Color{ 150, 58, 46, 255 }, c));
    QuadMuros({ c.x - 0.7f, 0.016f, c.z - 0.5f }, { c.x + 0.7f, 0.016f, c.z - 0.5f }, { c.x + 0.7f, 0.016f, c.z + 0.5f }, { c.x - 0.7f, 0.016f, c.z + 0.5f }, TinteMuros(Color{ 214, 170, 92, 255 }, c));
    rlEnd();
    PrismaMuros({ c.x - 0.5f, 0.0f, c.z }, 0.28f, 0.34f, 0.36f, 10, TinteMuros(Color{ 176, 134, 78, 255 }, c));
    VasijaMuros({ c.x + 0.5f, 0.0f, c.z - 0.2f }, 0.8f, COLOR_TERRACOTA_MUROS, false);
}


// Bloque caido de arenisca, apenas girado.
static void BloqueCaidoMuros(Vector3 p, Vector3 t, float giro)
{
    rlPushMatrix();
    rlTranslatef(p.x, p.y + t.y * 0.5f, p.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    CuboMuros({ 0.0f, 0.0f, 0.0f }, t, TinteMuros(COLOR_ARENISCA_MUROS, p));
    rlPopMatrix();
}


// Portada de los muros: dos pilares con banderolas indigo sobre la
// ranura del fondo, de donde suben los muros. Dintel con loza.
static void DibujarPortadaMuros(float t)
{
    const float Z = -6.9f;
    const float X = 8.0f;
    const float ALTO = 4.4f;

    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * X;
        CuboMuros({ x, ALTO * 0.5f, Z }, { 1.0f, ALTO, 1.1f }, COLOR_ADOBE_MUROS);
        LadrillosMuros({ x - 0.5f, 0.0f, Z + 0.55f }, { 1.0f, 0.0f, 0.0f }, 1.0f, 0.0f, ALTO, { 0.0f, 0.0f, 1.0f }, 0.5f, 0.4f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.8f, s * 3, false);
        CuboMuros({ x, 0.2f, Z }, { 1.3f, 0.4f, 1.4f }, COLOR_ARENISCA_MUROS);
        PinaculoMuros({ x, ALTO + 0.5f, Z }, 0.6f, 1.1f, COLOR_ADOBE_MUROS);

        // Banderola indigo con franjas de arena.
        TelaOndeanteMuros({ x, ALTO - 0.3f, Z + 0.62f }, 0.7f, 2.6f, t, (float)s * 1.3f, COLOR_INDIGO_MUROS, Color{ 226, 196, 140, 255 });
    }

    // Dintel con viga de madera, banda de loza turquesa y almenas.
    CuboMuros({ 0.0f, ALTO + 0.25f, Z }, { 2.0f * X + 1.0f, 0.7f, 0.9f }, COLOR_ADOBE_MUROS);
    CuboMuros({ 0.0f, ALTO - 0.16f, Z + 0.1f }, { 2.0f * X + 1.2f, 0.16f, 0.9f }, COLOR_MADERA_MUROS);

    rlBegin(RL_QUADS);
    for (int k = 0; k < 30; k++)
    {
        float x0 = -X + 2.0f * X * (float)k / 30.0f;
        float x1 = x0 + 2.0f * X / 30.0f - 0.04f;
        Color c = k % 2 == 0 ? COLOR_ACENTO_MUROS : Color{ 236, 226, 200, 255 };
        QuadMuros({ x0, ALTO + 0.2f, Z + 0.456f }, { x1, ALTO + 0.2f, Z + 0.456f }, { x1, ALTO + 0.42f, Z + 0.456f }, { x0, ALTO + 0.42f, Z + 0.456f }, SombraMuros(c, 0.86f, false));
    }
    rlEnd();

    for (int k = -6; k <= 6; k++)
    {
        PinaculoMuros({ (float)k * 1.2f, ALTO + 0.6f, Z }, 0.36f, 0.6f, COLOR_ADOBE_MUROS);
    }

    // Disco solar de bronce en el centro.
    rlPushMatrix();
    rlTranslatef(0.0f, ALTO + 0.6f, Z + 0.3f);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    PrismaMuros({ 0.0f, 0.0f, 0.0f }, 0.62f, 0.62f, 0.14f, 18, Color{ 214, 160, 72, 255 });
    PrismaMuros({ 0.0f, 0.14f, 0.0f }, 0.32f, 0.3f, 0.06f, 14, COLOR_ACENTO_MUROS);
    rlPopMatrix();
}


//==================================================
// FOSO Y PATIO DE BALDOSAS
//==================================================

// Explanada de arena a nivel del patio alrededor del foso, con losas
// viejas medio enterradas y un bordillo que marca el borde del foso.
static void DibujarExplanadaMuros()
{
    const float X = FOSO_X_MUROS;
    const float Z = FOSO_Z_MUROS;
    Color arena = COLOR_ARENA_MUROS;

    rlBegin(RL_QUADS);
    QuadNieblaMuros({ -40.0f, 0.0f, -Z }, { 40.0f, 0.0f, -Z }, { 40.0f, 0.0f, -11.0f }, { -40.0f, 0.0f, -11.0f }, arena);
    QuadNieblaMuros({ -40.0f, 0.0f, 16.0f }, { 40.0f, 0.0f, 16.0f }, { 40.0f, 0.0f, Z }, { -40.0f, 0.0f, Z }, arena);
    QuadNieblaMuros({ -40.0f, 0.0f, Z }, { -X, 0.0f, Z }, { -X, 0.0f, -Z }, { -40.0f, 0.0f, -Z }, arena);
    QuadNieblaMuros({ X, 0.0f, Z }, { 40.0f, 0.0f, Z }, { 40.0f, 0.0f, -Z }, { X, 0.0f, -Z }, arena);
    rlEnd();

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        // Ondas del viento en la arena.
        rlBegin(RL_QUADS);
        for (int k = 0; k < 40; k++)
        {
            float x = -18.0f + 36.0f * Hash01Muros(k, 80);
            float z = -10.5f + 22.0f * Hash01Muros(k, 81);
            if (std::fabs(x) < X + 0.6f && std::fabs(z) < Z + 0.6f) continue;
            float largo = 1.0f + 1.4f * Hash01Muros(k, 82);
            Color c = TinteMuros(TonoMuros(arena, 0.9f), { x, 0.0f, z });
            for (int r = 0; r < 3; r++)
            {
                float zz = z + (float)r * 0.22f;
                QuadMuros({ x - largo, 0.006f, zz }, { x + largo, 0.006f, zz + 0.1f }, { x + largo, 0.006f, zz + 0.14f }, { x - largo, 0.006f, zz + 0.04f }, c);
            }
        }
        rlEnd();

        // Losas viejas asomando de la arena.
        for (int k = 0; k < 26; k++)
        {
            float x = -14.0f + 28.0f * Hash01Muros(k, 83);
            float z = -10.5f + 20.0f * Hash01Muros(k, 84);
            if (std::fabs(x) < X + 0.7f && std::fabs(z) < Z + 0.7f) continue;
            Color c = TinteMuros(TonoMuros(COLOR_BALDOSA_MUROS, 0.9f + 0.15f * Hash01Muros(k, 85)), { x, 0.0f, z });
            float giro = 20.0f * (Hash01Muros(k, 86) - 0.5f);
            rlPushMatrix();
            rlTranslatef(x, 0.02f, z);
            rlRotatef(giro, 0.0f, 1.0f, 0.0f);
            CuboMuros({ 0.0f, 0.0f, 0.0f }, { 0.85f, 0.05f, 0.75f }, c);
            rlPopMatrix();
        }
    }

    // Bordillo de arenisca alrededor del foso.
    const float B = 0.36f;
    Color bordillo = COLOR_ARENISCA_MUROS;
    for (int k = 0; k < 12; k++)
    {
        float z0 = -Z - B + (2.0f * Z + 2.0f * B) * (float)k / 12.0f;
        float largo = (2.0f * Z + 2.0f * B) / 12.0f - 0.04f;
        Color c = TonoMuros(bordillo, 0.9f + 0.12f * Hash01Muros(k, 87));
        CuboMuros({ -X - B * 0.5f, 0.07f, z0 + largo * 0.5f }, { B, 0.14f, largo }, c);
        CuboMuros({ X + B * 0.5f, 0.07f, z0 + largo * 0.5f }, { B, 0.14f, largo }, TonoMuros(c, 0.97f));
    }
    for (int k = 0; k < 10; k++)
    {
        float x0 = -X + 2.0f * X * (float)k / 10.0f;
        float largo = 2.0f * X / 10.0f - 0.04f;
        Color c = TonoMuros(bordillo, 0.9f + 0.12f * Hash01Muros(k, 88));
        CuboMuros({ x0 + largo * 0.5f, 0.07f, -Z - B * 0.5f }, { largo, 0.14f, B }, c);
        CuboMuros({ x0 + largo * 0.5f, 0.07f, Z + B * 0.5f }, { largo, 0.14f, B }, c);
    }
}


// Foso de arena: paredes de adobe hacia adentro, fondo de arena con
// cascotes y la ranura del fondo por donde suben los muros.
static void DibujarFosoMuros()
{
    const float X = FOSO_X_MUROS;
    const float Z = FOSO_Z_MUROS;
    const float Y = NIVEL_FOSO_MUROS;

    // Fondo de arena, mas oscuro (a la sombra).
    Color fondo = TonoMuros(COLOR_ARENA_MUROS, 0.74f);
    rlBegin(RL_QUADS);
    QuadMuros({ -X, Y, -Z }, { X, Y, -Z }, { X, Y, Z }, { -X, Y, Z }, fondo);
    rlEnd();

    // Paredes interiores del foso (las que ve la camara).
    LadrillosMuros({ -X, 0.0f, Z }, { 0.0f, 0.0f, -1.0f }, 2.0f * Z, Y, 0.0f, { 1.0f, 0.0f, 0.0f }, 0.95f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.74f, 5, false);
    LadrillosMuros({ X, 0.0f, -Z }, { 0.0f, 0.0f, 1.0f }, 2.0f * Z, Y, 0.0f, { -1.0f, 0.0f, 0.0f }, 0.95f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.6f, 6, false);
    LadrillosMuros({ -X, 0.0f, -Z }, { 1.0f, 0.0f, 0.0f }, 2.0f * X, Y, 0.0f, { 0.0f, 0.0f, 1.0f }, 0.95f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.66f, 7, false);

    // Penumbra del foso: las paredes se oscurecen hacia el fondo, asi se
    // lee como una caida y no como un muro que sube.
    rlBegin(RL_QUADS);
    Color luz0 = { 46, 26, 22, 70 };
    Color sombra0 = { 46, 26, 22, 200 };
    const float YL = -1.5f;
    const float E = 0.03f;
    QuadMuros({ -X + E, Y, Z }, { -X + E, Y, -Z }, { -X + E, YL, -Z }, { -X + E, YL, Z }, sombra0);
    QuadMuros({ X - E, Y, -Z }, { X - E, Y, Z }, { X - E, YL, Z }, { X - E, YL, -Z }, sombra0);
    QuadMuros({ -X, Y, -Z + E }, { X, Y, -Z + E }, { X, YL, -Z + E }, { -X, YL, -Z + E }, sombra0);
    rlEnd();
    rlBegin(RL_QUADS);
    VerticeMuros({ -X + E, YL, Z }, sombra0); VerticeMuros({ -X + E, YL, -Z }, sombra0);
    VerticeMuros({ -X + E, 0.0f, -Z }, luz0); VerticeMuros({ -X + E, 0.0f, Z }, luz0);
    VerticeMuros({ X - E, YL, -Z }, sombra0); VerticeMuros({ X - E, YL, Z }, sombra0);
    VerticeMuros({ X - E, 0.0f, Z }, luz0); VerticeMuros({ X - E, 0.0f, -Z }, luz0);
    VerticeMuros({ -X, YL, -Z + E }, sombra0); VerticeMuros({ X, YL, -Z + E }, sombra0);
    VerticeMuros({ X, 0.0f, -Z + E }, luz0); VerticeMuros({ -X, 0.0f, -Z + E }, luz0);
    rlEnd();

    // Ranura de los muros y arena acumulada contra las paredes.
    rlBegin(RL_QUADS);
    Color ranura = Color{ 70, 46, 34, 255 };
    QuadMuros({ -LIMITE_X_MUROS, Y + 0.01f, -6.6f }, { LIMITE_X_MUROS, Y + 0.01f, -6.6f }, { LIMITE_X_MUROS, Y + 0.01f, -7.2f }, { -LIMITE_X_MUROS, Y + 0.01f, -7.2f }, ranura);
    QuadMuros({ -LIMITE_X_MUROS, Y + 0.01f, 6.6f }, { LIMITE_X_MUROS, Y + 0.01f, 6.6f }, { LIMITE_X_MUROS, Y + 0.01f, 7.2f }, { -LIMITE_X_MUROS, Y + 0.01f, 7.2f }, ranura);
    rlEnd();

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 8; k++)
        {
            float z = -6.0f + 12.0f * Hash01Muros(k, 90);
            float s = k % 2 == 0 ? -1.0f : 1.0f;
            DunaMuros({ s * (X - 0.2f), Y, z }, 0.9f, 1.6f, 0.6f, TonoMuros(COLOR_ARENA_MUROS, 0.86f));
        }

        // Cascotes de muros anteriores.
        for (int k = 0; k < 14; k++)
        {
            float x = (k % 2 == 0 ? -1.0f : 1.0f) * (5.9f + 1.0f * Hash01Muros(k, 91));
            float z = -7.0f + 14.0f * Hash01Muros(k, 92);
            BloqueCaidoMuros({ x, Y, z }, { 0.4f + 0.3f * Hash01Muros(k, 93), 0.22f, 0.3f }, 90.0f * Hash01Muros(k, 94));
        }
    }
}


// Patio jugable: baldosas gastadas con juntas, cornisa de arenisca en el
// borde (coincide con la zona real) y caras de adobe hacia el foso.
static void DibujarPatioMuros()
{
    const float X = LIMITE_X_MUROS;
    const float Z = LIMITE_Z_MUROS;
    const float Y = NIVEL_FOSO_MUROS;

    // Caras del patio hacia el foso. La del frente es la que mas se ve.
    LadrillosMuros({ -X, 0.0f, Z }, { 1.0f, 0.0f, 0.0f }, 2.0f * X, Y, -0.34f, { 0.0f, 0.0f, 1.0f }, 0.95f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.8f, 21, false);
    LadrillosMuros({ -X, 0.0f, -Z }, { 0.0f, 0.0f, 1.0f }, 2.0f * Z, Y, -0.34f, { -1.0f, 0.0f, 0.0f }, 0.95f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.7f, 22, false);
    LadrillosMuros({ X, 0.0f, Z }, { 0.0f, 0.0f, -1.0f }, 2.0f * Z, Y, -0.34f, { 1.0f, 0.0f, 0.0f }, 0.95f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.62f, 23, false);
    LadrillosMuros({ X, 0.0f, -Z }, { -1.0f, 0.0f, 0.0f }, 2.0f * X, Y, -0.34f, { 0.0f, 0.0f, -1.0f }, 0.95f, 0.42f, COLOR_ADOBE_MUROS, COLOR_ADOBE_OSCURO_MUROS, 0.56f, 24, false);

    // Cornisa: banda de arenisca bajo el borde, en las cuatro caras.
    rlBegin(RL_QUADS);
    Color corn = COLOR_ARENISCA_MUROS;
    QuadMuros({ -X, -0.34f, Z }, { X, -0.34f, Z }, { X, 0.0f, Z }, { -X, 0.0f, Z }, SombraMuros(corn, 0.84f, false));
    QuadMuros({ -X, -0.34f, -Z }, { -X, -0.34f, Z }, { -X, 0.0f, Z }, { -X, 0.0f, -Z }, SombraMuros(corn, 0.72f, false));
    QuadMuros({ X, -0.34f, Z }, { X, -0.34f, -Z }, { X, 0.0f, -Z }, { X, 0.0f, Z }, SombraMuros(corn, 0.64f, false));
    QuadMuros({ X, -0.34f, -Z }, { -X, -0.34f, -Z }, { -X, 0.0f, -Z }, { X, 0.0f, -Z }, SombraMuros(corn, 0.56f, false));

    // Sombra bajo la cornisa.
    Color sombra = Color{ 90, 56, 40, 255 };
    QuadMuros({ -X, -0.46f, Z + 0.02f }, { X, -0.46f, Z + 0.02f }, { X, -0.34f, Z + 0.02f }, { -X, -0.34f, Z + 0.02f }, sombra);

    // Penumbra hacia el fondo del foso en la cara del frente.
    Color penumbra = { 46, 26, 22, 210 };
    Color sinPenumbra = { 46, 26, 22, 0 };
    VerticeMuros({ -X, Y, Z + 0.03f }, penumbra);
    VerticeMuros({ X, Y, Z + 0.03f }, penumbra);
    VerticeMuros({ X, -1.0f, Z + 0.03f }, sinPenumbra);
    VerticeMuros({ -X, -1.0f, Z + 0.03f }, sinPenumbra);

    // Base de juntas bajo todas las baldosas.
    QuadMuros({ -X, 0.0f, Z }, { X, 0.0f, Z }, { X, 0.0f, -Z }, { -X, 0.0f, -Z }, COLOR_JUNTA_MUROS);
    rlEnd();

    // Losas largas del borde (parte de la zona jugable).
    const float B = 0.3f;
    rlBegin(RL_QUADS);
    for (int k = 0; k < 10; k++)
    {
        float x0 = -X + 2.0f * X * (float)k / 10.0f + 0.03f;
        float x1 = -X + 2.0f * X * (float)(k + 1) / 10.0f - 0.03f;
        for (int s = -1; s <= 1; s += 2)
        {
            Color c = SombraMuros(TonoMuros(COLOR_ARENISCA_MUROS, 0.86f + 0.1f * Hash01Muros(k, 100 + s)), 1.0f, true);
            float za = (float)s * Z;
            float zb = (float)s * (Z - B);
            QuadMuros({ x0, 0.004f, za - (float)s * 0.03f }, { x1, 0.004f, za - (float)s * 0.03f }, { x1, 0.004f, zb + (float)s * 0.02f }, { x0, 0.004f, zb + (float)s * 0.02f }, c);
        }
    }
    for (int k = 0; k < 9; k++)
    {
        float z0 = -(Z - B) + 2.0f * (Z - B) * (float)k / 9.0f + 0.03f;
        float z1 = -(Z - B) + 2.0f * (Z - B) * (float)(k + 1) / 9.0f - 0.03f;
        for (int s = -1; s <= 1; s += 2)
        {
            Color c = SombraMuros(TonoMuros(COLOR_ARENISCA_MUROS, 0.86f + 0.1f * Hash01Muros(k, 110 + s)), 1.0f, true);
            float xa = (float)s * X;
            float xb = (float)s * (X - B);
            QuadMuros({ xa - (float)s * 0.03f, 0.004f, z0 }, { xb + (float)s * 0.02f, 0.004f, z0 }, { xb + (float)s * 0.02f, 0.004f, z1 }, { xa - (float)s * 0.03f, 0.004f, z1 }, c);
        }
    }

    // Filo de luz en el borde del patio.
    Color filo = Color{ 255, 238, 200, 255 };
    QuadMuros({ -X, 0.006f, Z }, { X, 0.006f, Z }, { X, 0.006f, Z - 0.05f }, { -X, 0.006f, Z - 0.05f }, filo);
    QuadMuros({ -X, 0.006f, -Z + 0.05f }, { X, 0.006f, -Z + 0.05f }, { X, 0.006f, -Z }, { -X, 0.006f, -Z }, filo);
    QuadMuros({ -X, 0.006f, -Z }, { -X + 0.05f, 0.006f, -Z }, { -X + 0.05f, 0.006f, Z }, { -X, 0.006f, Z }, filo);
    QuadMuros({ X - 0.05f, 0.006f, -Z }, { X, 0.006f, -Z }, { X, 0.006f, Z }, { X - 0.05f, 0.006f, Z }, filo);
    rlEnd();

    // Baldosas gastadas: variacion, bisel, esquinas rotas y grietas.
    const int COLS = 12;
    const int FILAS = 12;
    const float X0 = -(X - B);
    const float Z0 = -(Z - B);
    const float AX = 2.0f * (X - B) / (float)COLS;
    const float AZ = 2.0f * (Z - B) / (float)FILAS;
    const float INSET = 0.035f;

    rlBegin(RL_QUADS);
    for (int i = 0; i < COLS; i++)
    {
        for (int j = 0; j < FILAS; j++)
        {
            float x0 = X0 + AX * (float)i + INSET;
            float x1 = X0 + AX * (float)(i + 1) - INSET;
            float z0 = Z0 + AZ * (float)j + INSET;
            float z1 = Z0 + AZ * (float)(j + 1) - INSET;

            float h = Hash01Muros(i * 31 + j, 7);
            Color c = TonoMuros(COLOR_BALDOSA_MUROS, 0.9f + 0.17f * h);
            if (Hash01Muros(i, j + 300) > 0.86f) c = MezclaMuros(c, COLOR_TERRACOTA_MUROS, 0.22f);

            // Roseta de terracota en el centro del patio.
            float di = std::fabs((float)i - 5.5f);
            float dj = std::fabs((float)j - 5.5f);
            if (di + dj <= 2.0f) c = MezclaMuros(c, COLOR_TERRACOTA_MUROS, di + dj <= 1.0f ? 0.42f : 0.28f);

            Color arriba = SombraMuros(c, 1.0f, true);
            QuadMuros({ x0, 0.004f, z1 }, { x1, 0.004f, z1 }, { x1, 0.004f, z0 }, { x0, 0.004f, z0 }, arriba);

            // Bisel: filo claro al fondo, sombra al frente.
            QuadMuros({ x0, 0.006f, z0 + 0.05f }, { x1, 0.006f, z0 + 0.05f }, { x1, 0.006f, z0 }, { x0, 0.006f, z0 }, TonoMuros(arriba, 1.08f));
            QuadMuros({ x0, 0.006f, z1 }, { x1, 0.006f, z1 }, { x1, 0.006f, z1 - 0.05f }, { x0, 0.006f, z1 - 0.05f }, TonoMuros(arriba, 0.88f));

            // Esquina rota.
            if (Hash01Muros(i * 7 + 1, j * 5 + 11) < 0.14f)
            {
                float t = 0.16f + 0.1f * Hash01Muros(i, j + 12);
                Color j0 = TonoMuros(COLOR_JUNTA_MUROS, 1.05f);
                QuadMuros({ x1 - t, 0.008f, z1 }, { x1, 0.008f, z1 }, { x1, 0.008f, z1 - t }, { x1, 0.008f, z1 - t }, j0);
            }

            // Grieta en diagonal.
            if (Hash01Muros(i * 3 + 2, j * 9 + 13) < 0.1f)
            {
                Color g = TonoMuros(arriba, 0.66f);
                float mx = (x0 + x1) * 0.5f;
                QuadMuros({ x0 + 0.08f, 0.008f, z0 + 0.1f }, { mx, 0.008f, (z0 + z1) * 0.5f - 0.015f }, { mx, 0.008f, (z0 + z1) * 0.5f + 0.015f }, { x0 + 0.08f, 0.008f, z0 + 0.12f }, g);
                QuadMuros({ mx, 0.008f, (z0 + z1) * 0.5f - 0.015f }, { x1 - 0.12f, 0.008f, z1 - 0.06f }, { x1 - 0.12f, 0.008f, z1 - 0.04f }, { mx, 0.008f, (z0 + z1) * 0.5f + 0.015f }, g);
            }
        }
    }
    rlEnd();

    // Estrella de la roseta (incrustacion de arenisca oscura).
    rlBegin(RL_QUADS);
    Color estrella = SombraMuros(Color{ 150, 82, 56, 255 }, 1.0f, true);
    Color centro = SombraMuros(Color{ 236, 214, 170, 255 }, 1.0f, true);
    QuadMuros({ 0.0f, 0.01f, -1.05f }, { 1.05f, 0.01f, 0.0f }, { 0.0f, 0.01f, 1.05f }, { -1.05f, 0.01f, 0.0f }, estrella);
    QuadMuros({ -0.75f, 0.011f, -0.75f }, { 0.75f, 0.011f, -0.75f }, { 0.75f, 0.011f, 0.75f }, { -0.75f, 0.011f, 0.75f }, TonoMuros(estrella, 0.94f));
    QuadMuros({ 0.0f, 0.012f, -0.5f }, { 0.5f, 0.012f, 0.0f }, { 0.0f, 0.012f, 0.5f }, { -0.5f, 0.012f, 0.0f }, centro);
    rlEnd();

    // Arena que el viento dejo sobre el patio (manchas suaves).
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 10; k++)
        {
            float x = (Hash01Muros(k, 120) - 0.5f) * 2.0f * (X - 0.8f);
            float z = (Hash01Muros(k, 121) - 0.5f) * 2.0f * (Z - 0.8f);
            if (k < 4)
            {
                x = (k & 1 ? 1.0f : -1.0f) * (X - 0.7f);
                z = (k & 2 ? 1.0f : -1.0f) * (Z - 0.7f);
            }
            float r = 0.6f + 0.6f * Hash01Muros(k, 122);
            ResplandorSueloMuros({ x, 0.014f, z }, r, Color{ 238, 212, 164, 120 }, 1.6f);
        }
    }
}


//==================================================
// EL MURO
//==================================================

// Desplazamiento vertical y vuelco del muro (solo dibujo). Sube desde la
// ranura del fondo durante la cuenta y en la pausa entre muros, y al pasar
// el borde del frente se hunde en el foso inclinandose.
static void TransformacionMuroMuros(const MinijuegoMurosLocos& m, float& desplazamiento, float& vuelco)
{
    float altoTotal = ALTO_MURO - BASE_MURO_VISUAL + 0.4f;
    desplazamiento = 0.0f;
    vuelco = 0.0f;

    if (m.fase == FASE_MUROS_PREPARACION)
    {
        float u = Limitar01VisualMuros((DURACION_PREPARACION_MUROS - m.tiempoPreparacion - 1.0f) / 1.6f);
        float suave = u * u * (3.0f - 2.0f * u);
        desplazamiento = -(1.0f - suave) * altoTotal;
        return;
    }

    if (m.tiempoEntreMuros > 0.0f)
    {
        float progreso = Limitar01VisualMuros(m.tiempoJugado / ESCALA_DIFICULTAD_MUROS);
        float pausa = 0.52f - progreso * 0.27f;
        if (pausa < 0.20f) pausa = 0.20f;
        float u = Limitar01VisualMuros(1.0f - m.tiempoEntreMuros / pausa);
        float suave = 1.0f - (1.0f - u) * (1.0f - u);
        desplazamiento = -(1.0f - suave) * altoTotal;
        return;
    }

    if (m.muro.z > LIMITE_Z_MUROS)
    {
        float s = Limitar01VisualMuros((m.muro.z - LIMITE_Z_MUROS) / (Z_FIN_MURO - LIMITE_Z_MUROS));
        desplazamiento = -s * s * altoTotal * 1.1f;
        vuelco = s * 28.0f;
    }
}


// Un tramo de muro de adobe entre dos huecos (coordenadas locales: z = 0
// es el centro del muro). huecoIzq / huecoDer: hay hueco a ese lado.
static void TramoMuroMuros(float xa, float xb, bool huecoIzq, bool huecoDer, int indice)
{
    float ancho = xb - xa;
    if (ancho <= 0.03f) return;

    float xc = (xa + xb) * 0.5f;
    const float COPA = 0.16f;
    float altoCuerpo = ALTO_MURO - COPA;
    float zCara = GROSOR_MURO * 0.5f;

    // Cuerpo de barro (junta) y cara de ladrillos hacia la camara.
    CuboMuros({ xc, (altoCuerpo + BASE_MURO_VISUAL) * 0.5f, 0.0f }, { ancho, altoCuerpo - BASE_MURO_VISUAL, GROSOR_MURO }, COLOR_ADOBE_OSCURO_MUROS);
    LadrillosMuros({ xa, 0.0f, zCara }, { 1.0f, 0.0f, 0.0f }, ancho, 0.0f, altoCuerpo, { 0.0f, 0.0f, 1.0f }, 0.62f, 0.38f, Color{ 208, 136, 84, 255 }, Color{ 138, 84, 54, 255 }, 0.9f, 300 + indice, false);

    // Parte enterrada: barro manchado de arena, mas oscuro hacia abajo.
    rlBegin(RL_QUADS);
    Color arriba = SombraMuros(Color{ 168, 108, 70, 255 }, 0.82f, false);
    Color abajo = SombraMuros(Color{ 120, 84, 60, 255 }, 0.82f, false);
    VerticeMuros({ xa, BASE_MURO_VISUAL, zCara + 0.01f }, abajo);
    VerticeMuros({ xb, BASE_MURO_VISUAL, zCara + 0.01f }, abajo);
    VerticeMuros({ xb, 0.0f, zCara + 0.01f }, arriba);
    VerticeMuros({ xa, 0.0f, zCara + 0.01f }, arriba);
    rlEnd();

    // Borde superior: albardilla de arenisca con almenas redondeadas.
    CuboMuros({ xc, ALTO_MURO - COPA * 0.5f, 0.0f }, { ancho, COPA, GROSOR_MURO + 0.1f }, COLOR_ARENISCA_MUROS);
    rlBegin(RL_QUADS);
    Color filo = Color{ 255, 240, 206, 255 };
    QuadMuros({ xa, ALTO_MURO + 0.002f, zCara + 0.05f }, { xb, ALTO_MURO + 0.002f, zCara + 0.05f }, { xb, ALTO_MURO + 0.002f, zCara - 0.02f }, { xa, ALTO_MURO + 0.002f, zCara - 0.02f }, filo);
    rlEnd();

    float primera = std::ceil((xa + 0.3f) / 0.9f) * 0.9f;
    for (float x = primera; x <= xb - 0.3f; x += 0.9f)
    {
        CuboMuros({ x, ALTO_MURO + 0.08f, 0.0f }, { 0.4f, 0.16f, GROSOR_MURO - 0.06f }, COLOR_ADOBE_MUROS);
        PrismaMuros({ x, ALTO_MURO + 0.16f, 0.0f }, 0.18f, 0.06f, 0.12f, 6, COLOR_ADOBE_MUROS);
    }

    // Jambas del hueco: poste de arenisca con loza turquesa.
    for (int s = 0; s < 2; s++)
    {
        bool hay = s == 0 ? huecoIzq : huecoDer;
        if (!hay) continue;

        float xj = s == 0 ? xa + 0.11f : xb - 0.11f;
        CuboMuros({ xj, (ALTO_MURO + 0.12f + BASE_MURO_VISUAL) * 0.5f, 0.0f }, { 0.22f, ALTO_MURO + 0.12f - BASE_MURO_VISUAL, GROSOR_MURO + 0.12f }, COLOR_ARENISCA_MUROS);

        // Azulejos turquesa en la cara del poste y en su canto interior.
        rlBegin(RL_QUADS);
        float zf = zCara + 0.065f;
        float xCanto = s == 0 ? xa + 0.002f : xb - 0.002f;
        for (int k = 0; k < 6; k++)
        {
            float y0 = 0.1f + (float)k * 0.38f;
            float y1 = y0 + 0.3f;
            Color az = k % 2 == 0 ? COLOR_ACENTO_MUROS : Color{ 238, 230, 206, 255 };
            QuadMuros({ xj - 0.08f, y0, zf }, { xj + 0.08f, y0, zf }, { xj + 0.08f, y1, zf }, { xj - 0.08f, y1, zf }, SombraMuros(az, 0.9f, false));
            QuadMuros({ xCanto, y0, -0.16f }, { xCanto, y0, 0.16f }, { xCanto, y1, 0.16f }, { xCanto, y1, -0.16f }, SombraMuros(az, s == 0 ? 0.7f : 0.64f, false));
        }
        rlEnd();

        // Remate: farolillo de loza.
        PrismaMuros({ xj, ALTO_MURO + 0.12f, 0.0f }, 0.13f, 0.1f, 0.14f, 8, COLOR_ARENISCA_MUROS);
        PrismaMuros({ xj, ALTO_MURO + 0.26f, 0.0f }, 0.1f, 0.03f, 0.16f, 8, COLOR_ACENTO_CLARO_MUROS);
    }
}


static void DibujarMuroLocalMuros(const MuroLoco& muro)
{
    float bordeIzquierdo = -LIMITE_X_MUROS + 0.01f;
    bool huecoAntes = false;
    int indice = 0;

    for (int i = 0; i < muro.cantidadHuecos; i++)
    {
        float inicioHueco = muro.centrosHueco[i] - muro.anchoHueco / 2.0f;
        float finHueco = muro.centrosHueco[i] + muro.anchoHueco / 2.0f;

        if (inicioHueco > bordeIzquierdo)
        {
            TramoMuroMuros(bordeIzquierdo, inicioHueco, huecoAntes, true, indice++);
        }

        bordeIzquierdo = finHueco;
        huecoAntes = true;
    }

    if (bordeIzquierdo < LIMITE_X_MUROS - 0.01f)
    {
        TramoMuroMuros(bordeIzquierdo, LIMITE_X_MUROS - 0.01f, huecoAntes, false, indice);
    }
}


static bool MuroVisibleMuros(const MinijuegoMurosLocos& m)
{
    return m.fase != FASE_MUROS_TERMINADO || m.tiempoEntreMuros <= 0.0f;
}


static void DibujarMuroMuros(const MinijuegoMurosLocos& m)
{
    if (!MuroVisibleMuros(m)) return;

    float desplazamiento = 0.0f;
    float vuelco = 0.0f;
    TransformacionMuroMuros(m, desplazamiento, vuelco);

    rlPushMatrix();
    rlTranslatef(0.0f, desplazamiento + BASE_MURO_VISUAL, m.muro.z);
    rlRotatef(vuelco, 1.0f, 0.0f, 0.0f);
    rlTranslatef(0.0f, -BASE_MURO_VISUAL, 0.0f);
    DibujarMuroLocalMuros(m.muro);
    rlPopMatrix();
}


// Cuanto "esta" el muro sobre el patio (0 escondido, 1 arriba y en juego).
static float PresenciaMuroMuros(const MinijuegoMurosLocos& m)
{
    if (!MuroVisibleMuros(m)) return 0.0f;

    float desplazamiento = 0.0f;
    float vuelco = 0.0f;
    TransformacionMuroMuros(m, desplazamiento, vuelco);
    float altoTotal = ALTO_MURO - BASE_MURO_VISUAL;
    return Limitar01VisualMuros(1.0f + desplazamiento / altoTotal);
}


//==================================================
// JUGADORES Y TRANSLUCIDOS
//==================================================

static void DibujarAnilloJugadorMuros(const JugadorPrueba& jugador, Color color, float t)
{
    if (jugador.cayendo || !JugadorSobreArenaMuros(jugador)) return;

    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (pies < -0.3f) return;
    float alto = pies > 0.0f ? pies : 0.0f;
    float escala = 1.0f - Limitar01VisualMuros(alto / 4.0f) * 0.35f;
    float pulso = 1.0f + 0.04f * std::sin(t * 5.0f + jugador.posicion.x);

    Color relleno = color;
    relleno.a = 70;
    AnilloMuros({ jugador.posicion.x, 0.02f, jugador.posicion.z }, 0.0f, 0.5f * escala, relleno, 20);
    AnilloMuros({ jugador.posicion.x, 0.024f, jugador.posicion.z }, 0.5f * escala * pulso, 0.62f * escala * pulso, color, 24);
}


// Eliminados: caen al foso, levantan arena y se hunden.
static void DibujarCaidasMuros(
    const MinijuegoMurosLocos& m,
    const Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorMurosLocos& e = m.estadosJugadores[i];

        if (!m.resultado.participantes[i].participo || e.tiempoCaida < 0.0f || e.hundimiento >= 1.0f)
        {
            continue;
        }

        Vector3 pies = { e.posicionCaida.x, e.posicionCaida.y - 0.7f - e.hundimiento * 1.7f, e.posicionCaida.z };
        float giro = e.tocoArena ? 0.0f : e.tiempoCaida * 160.0f;
        DibujarModeloJugadorEnPosicion(ObtenerPersonajeParticipante(participantes[i]), pies, giro, participantes[i].color);
    }
}


static void DibujarParticulasMuros(const MinijuegoMurosLocos& m)
{
    for (int i = 0; i < MAX_PARTICULAS_MUROS; i++)
    {
        const ParticulaMurosLocos& p = m.particulasEfecto[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        float tam = p.tamano * (0.5f + 0.5f * vida);
        CuboMuros(p.posicion, { tam, tam, tam }, p.color);
    }
}


// Sombra del muro sobre el patio (sol alto y algo detras) y anillos.
static void DibujarTranslucidosMuros(
    const MinijuegoMurosLocos& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[]
)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    float presencia = PresenciaMuroMuros(m);

    if (presencia > 0.05f && m.muro.z < LIMITE_Z_MUROS)
    {
        float z0 = m.muro.z + GROSOR_MURO * 0.5f;
        float z1 = z0 + 1.3f;
        if (z0 < -LIMITE_Z_MUROS) z0 = -LIMITE_Z_MUROS;
        if (z1 > LIMITE_Z_MUROS) z1 = LIMITE_Z_MUROS;

        if (z1 > z0)
        {
            float bordeIzq = -LIMITE_X_MUROS;
            unsigned char alfa = (unsigned char)(150.0f * presencia);
            Color oscuro = { 70, 40, 34, alfa };
            Color nada = { 70, 40, 34, 0 };

            rlBegin(RL_QUADS);
            for (int h = 0; h <= m.muro.cantidadHuecos; h++)
            {
                float bordeDer = h < m.muro.cantidadHuecos
                    ? m.muro.centrosHueco[h] - m.muro.anchoHueco * 0.5f
                    : LIMITE_X_MUROS;

                if (bordeDer > bordeIzq)
                {
                    VerticeMuros({ bordeIzq, 0.016f, z0 }, oscuro);
                    VerticeMuros({ bordeIzq, 0.016f, z1 }, nada);
                    VerticeMuros({ bordeDer, 0.016f, z1 }, nada);
                    VerticeMuros({ bordeDer, 0.016f, z0 }, oscuro);
                }

                if (h < m.muro.cantidadHuecos)
                {
                    bordeIzq = m.muro.centrosHueco[h] + m.muro.anchoHueco * 0.5f;
                }
            }
            rlEnd();
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo || m.estadosJugadores[i].eliminado) continue;
        DibujarAnilloJugadorMuros(jugadores[i], participantes[i].color, t);
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Arena que el viento arrastra al ras del suelo (estelas) y polvo fino
// suspendido: la calima en movimiento.
static void DibujarArenaVientoMuros(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    int cantidad = (int)(34.0f * FactorCalidadGrafica(CalidadParticulas()));

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlBegin(RL_QUADS);

    for (int k = 0; k < cantidad; k++)
    {
        float ciclo = 4.0f + 3.0f * Hash01Muros(k, 200);
        float u = std::fmod(t / ciclo + Hash01Muros(k, 201), 1.0f);
        float x = -16.0f + 32.0f * u;
        float z = -12.0f + 20.0f * Hash01Muros(k, 202);
        float y = 0.05f + 1.6f * Hash01Muros(k, 203) * Hash01Muros(k, 204);
        if (std::fabs(z) < LIMITE_Z_MUROS && std::fabs(x) < LIMITE_X_MUROS + 1.4f) y += 0.1f;
        float largo = 0.8f + 1.4f * Hash01Muros(k, 205);
        float alfa = std::sin(u * 3.1416f);
        Color c = { 246, 226, 186, (unsigned char)(70.0f * alfa) };
        Color c0 = c;
        c0.a = 0;
        float ondula = std::sin(t * 2.0f + (float)k) * 0.06f;

        VerticeMuros({ x - largo, y + ondula, z }, c0);
        VerticeMuros({ x, y, z - 0.05f }, c);
        VerticeMuros({ x + 0.3f, y + 0.02f, z }, c);
        VerticeMuros({ x, y + 0.04f, z + 0.05f }, c);
    }

    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// LUCES FALSAS
//==================================================

static const Vector3 BRASEROS_MUROS[2] =
{
    { -8.4f, 0.0f, -3.0f },
    { 8.4f, 0.0f, -3.0f }
};


static void DibujarLucesMuros(
    const MinijuegoMurosLocos& m,
    const JugadorPrueba jugadores[]
)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Braseros: halo en la llama y charco calido en la arena.
    for (int i = 0; i < 2; i++)
    {
        Vector3 b = BRASEROS_MUROS[i];
        float pulso = 0.85f + 0.15f * std::sin(t * 9.0f + (float)i * 2.0f) + 0.05f * std::sin(t * 23.0f);
        ResplandorFrenteMuros({ b.x, b.y + 1.75f, b.z + 0.2f }, 1.3f * pulso, Color{ 255, 150, 60, 110 });
        ResplandorFrenteMuros({ b.x, b.y + 1.65f, b.z + 0.2f }, 0.55f * pulso, Color{ 255, 220, 140, 150 });

        for (int r = 0; r < 3; r++)
        {
            ResplandorSueloMuros({ b.x, b.y + 0.02f + 0.002f * (float)r, b.z + 0.3f }, (1.4f + 0.7f * (float)r) * pulso, Color{ 255, 150, 70, 34 });
        }

        // Chispas que suben.
        for (int k = 0; k < 6; k++)
        {
            float ciclo = 1.2f + 0.6f * Hash01Muros(k, 210 + i);
            float u = std::fmod(t / ciclo + Hash01Muros(k, 212 + i), 1.0f);
            Vector3 p = { b.x + std::sin(t * 3.0f + (float)k) * 0.2f * u, b.y + 1.5f + 1.8f * u, b.z + 0.1f };
            ResplandorFrenteMuros(p, 0.07f, Color{ 255, 190, 90, (unsigned char)(220.0f * (1.0f - u)) });
        }
    }

    // El hueco: farolillos de las jambas y senda de luz turquesa por el
    // patio que marca por donde pasar. Aparece con el muro (aviso).
    float presencia = PresenciaMuroMuros(m);

    if (presencia > 0.02f)
    {
        float desplazamiento = 0.0f;
        float vuelco = 0.0f;
        TransformacionMuroMuros(m, desplazamiento, vuelco);
        float pulso = 0.8f + 0.2f * std::sin(t * 8.0f);
        float zMuro = m.muro.z;

        for (int h = 0; h < m.muro.cantidadHuecos; h++)
        {
            float c = m.muro.centrosHueco[h];
            float medio = m.muro.anchoHueco * 0.5f;

            for (int s = -1; s <= 1; s += 2)
            {
                Vector3 farol = { c + (float)s * (medio + 0.11f), ALTO_MURO + 0.36f + desplazamiento, zMuro };
                ResplandorFrenteMuros(farol, 0.5f * pulso, Color{ COLOR_ACENTO_MUROS.r, COLOR_ACENTO_MUROS.g, COLOR_ACENTO_MUROS.b, (unsigned char)(140.0f * presencia) });
            }

            if (zMuro < LIMITE_Z_MUROS)
            {
                float z0 = zMuro + GROSOR_MURO * 0.5f;
                float z1 = LIMITE_Z_MUROS;
                if (z0 < -LIMITE_Z_MUROS) z0 = -LIMITE_Z_MUROS;
                unsigned char alfa = (unsigned char)(110.0f * presencia * pulso);
                Color a = { COLOR_ACENTO_MUROS.r, COLOR_ACENTO_MUROS.g, COLOR_ACENTO_MUROS.b, alfa };
                Color b = a;
                b.a = (unsigned char)((float)alfa * 0.12f);

                rlBegin(RL_QUADS);
                VerticeMuros({ c - medio, 0.02f, z0 }, a);
                VerticeMuros({ c - medio, 0.02f, z1 }, b);
                VerticeMuros({ c + medio, 0.02f, z1 }, b);
                VerticeMuros({ c + medio, 0.02f, z0 }, a);

                // Bordes mas marcados de la senda.
                for (int s = -1; s <= 1; s += 2)
                {
                    float xb = c + (float)s * medio;
                    float xi = xb - (float)s * 0.08f;
                    Color fa = a;
                    fa.a = (unsigned char)((float)alfa * 1.8f > 255.0f ? 255.0f : (float)alfa * 1.8f);
                    VerticeMuros({ xi, 0.022f, z0 }, fa);
                    VerticeMuros({ xi, 0.022f, z1 }, b);
                    VerticeMuros({ xb, 0.022f, z1 }, b);
                    VerticeMuros({ xb, 0.022f, z0 }, fa);
                }
                rlEnd();
            }
        }
    }

    // Destello del empujon sobre cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorMurosLocos& e = m.estadosJugadores[i];
        if (!m.resultado.participantes[i].participo || e.eliminado || e.destelloGolpe <= 0.0f) continue;

        Vector3 p = jugadores[i].posicion;
        ResplandorFrenteMuros({ p.x, p.y + 0.2f, p.z + 0.3f }, 1.3f * e.destelloGolpe + 0.3f, Color{ 255, 236, 190, (unsigned char)(150.0f * e.destelloGolpe) });
    }

    // Haces de sol que entran por los arcos de la fachada.
    const float arcos[4] = { -14.0f, -2.0f, 2.0f, 10.0f };
    for (int k = 0; k < 4; k++)
    {
        float x = arcos[k];
        float vaiven = 0.5f + 0.5f * std::sin(t * 0.4f + (float)k * 1.7f);
        Color c = { 255, 226, 170, (unsigned char)(14.0f + 8.0f * vaiven) };
        Color c0 = c;
        c0.a = 0;

        rlBegin(RL_QUADS);
        VerticeMuros({ x - 1.0f, 3.6f, -15.4f }, c);
        VerticeMuros({ x + 1.0f, 3.6f, -15.4f }, c);
        VerticeMuros({ x + 1.6f, 0.02f, -10.0f }, c0);
        VerticeMuros({ x - 0.4f, 0.02f, -10.0f }, c0);
        rlEnd();
        ResplandorSueloMuros({ x + 0.4f, 0.03f, -12.6f }, 1.6f, Color{ 255, 220, 150, (unsigned char)(26.0f + 10.0f * vaiven) }, 0.8f);
    }

    // Motas de polvo dorado que brillan al sol.
    int motas = (int)(40.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int k = 0; k < motas; k++)
    {
        float ciclo = 10.0f + 6.0f * Hash01Muros(k, 220);
        float u = std::fmod(t / ciclo + Hash01Muros(k, 221), 1.0f);
        Vector3 p =
        {
            -12.0f + 24.0f * u + std::sin(t * 0.7f + (float)k) * 0.4f,
            0.5f + 4.5f * Hash01Muros(k, 222) + std::sin(t * 1.1f + (float)k * 0.7f) * 0.3f,
            -11.0f + 18.0f * Hash01Muros(k, 223)
        };
        float alfa = std::sin(u * 3.1416f);
        ResplandorFrenteMuros(p, 0.06f, Color{ 255, 236, 180, (unsigned char)(200.0f * alfa) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJAR
//==================================================

static void DibujarEscenarioMuros(const MinijuegoMurosLocos& m)
{
    float t = m.tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Horizonte y media distancia.
    DibujarHorizonteMuros(t);
    DibujarBuitresMuros(t);

    if (decoracion)
    {
        DibujarFachadaMuros();
        DibujarColumnatasMuros(paso);
    }

    // Explanada, foso y patio.
    DibujarExplanadaMuros();
    DibujarFosoMuros();
    DibujarPatioMuros();

    if (!decoracion)
    {
        return;
    }

    // Anillo cercano: portada, braseros, palmeras, vasijas, toldo, bloques.
    DibujarPortadaMuros(t);

    for (int i = 0; i < 2; i++)
    {
        BraseroMuros(BRASEROS_MUROS[i], t, i * 3);
    }

    const Vector3 palmeras[5] =
    {
        { -10.6f, 0.0f, -10.4f }, { 11.0f, 0.0f, -9.6f }, { -11.4f, 0.0f, 1.0f },
        { 11.8f, 0.0f, 5.2f }, { -13.2f, 0.0f, -5.0f }
    };
    for (int k = 0; k < 5; k += paso)
    {
        float alto = 4.4f + 1.4f * Hash01Muros(k, 130);
        float inclinacion = k % 2 == 0 ? 0.4f : 2.6f;
        PalmeraSecaMuros(palmeras[k], alto, inclinacion, t, k * 7 + 1);
    }

    const Vector3 vasijas[9] =
    {
        { -8.3f, 0.0f, -8.0f }, { -8.0f, 0.0f, -7.4f }, { -8.6f, 0.0f, -7.1f },
        { 8.4f, 0.0f, -7.6f }, { 8.1f, 0.0f, -8.3f },
        { -8.1f, 0.0f, 3.6f }, { -8.4f, 0.0f, 4.4f },
        { 8.3f, 0.0f, 7.4f }, { -8.2f, 0.0f, 7.6f }
    };
    for (int k = 0; k < 9; k += paso)
    {
        float escala = 0.8f + 0.5f * Hash01Muros(k, 131);
        Color barro = k % 3 == 0 ? COLOR_TERRACOTA_MUROS : (k % 3 == 1 ? Color{ 214, 176, 120, 255 } : Color{ 166, 108, 72, 255 });
        VasijaMuros(vasijas[k], escala, barro, k == 4 || k == 7);
    }

    ToldoMuros({ 9.9f, 0.0f, 2.2f }, t);

    for (int k = 0; k < 8; k += paso)
    {
        float s = k % 2 == 0 ? -1.0f : 1.0f;
        Vector3 p = { s * (8.0f + 1.6f * Hash01Muros(k, 140)), 0.0f, -6.0f + 12.0f * Hash01Muros(k, 141) };
        BloqueCaidoMuros(p, { 0.7f + 0.4f * Hash01Muros(k, 142), 0.4f + 0.2f * Hash01Muros(k, 143), 0.5f }, 60.0f * Hash01Muros(k, 144));
    }
}


void MinijuegoMurosLocos::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;


    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    Camera3D vista = camara;

    if (fase == FASE_MUROS_PREPARACION)
    {
        float u = Limitar01VisualMuros(tiempoPreparacion / DURACION_PREPARACION_MUROS);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 2.6f * suave;
        vista.position.z += 4.0f * suave;
        vista.target.y += 0.8f * suave;
    }

    ClearBackground(COLOR_CALIMA_MUROS);
    DibujarCieloMuros(vista, t);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarEscenarioMuros(*this);
    DibujarMuroMuros(*this);
    DibujarTranslucidosMuros(*this, jugadores, participantes);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || estadosJugadores[i].eliminado)
        {
            continue;
        }

        // Sacudida corta al recibir el muro.
        const EstadoJugadorMurosLocos& e = estadosJugadores[i];
        if (e.destelloGolpe > 0.0f)
        {
            JugadorPrueba sacudido = jugadores[i];
            sacudido.posicion.x += std::sin(t * 70.0f) * 0.06f * e.destelloGolpe;
            DibujarJugadorModeloCompartido(sacudido, participantes[i]);
        }
        else
        {
            DibujarJugadorModeloCompartido(jugadores[i], participantes[i]);
        }

        if (mostrarDebug && !jugadores[i].cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarCaidasMuros(*this, participantes);
    DibujarParticulasTierra(particulas, cantidadParticulas);
    DibujarParticulasMuros(*this);
    DibujarArenaVientoMuros(t);
    DibujarLucesMuros(*this, jugadores);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre los que siguen en el patio.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado ||
            jugadores[i].cayendo ||
            !participantes[i].activo
        )
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

    // «¡J# AL FOSO!» donde cayo cada eliminado.
    for (int i = 0; i < MAX_AVISOS_MUROS; i++)
    {
        const AvisoMurosLocos& aviso = avisos[i];
        if (!aviso.activo) continue;

        float u = 1.0f - aviso.vida / DURACION_AVISO_MUROS;
        Vector3 p = { aviso.posicion.x, aviso.posicion.y + 0.6f + u * 1.2f, aviso.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);

        float limiteAbajo = (float)(alto - MargenBarraRondaMinijuegos()) - Px(70.0f);
        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.y > limiteAbajo) s.y = limiteAbajo;
        if (s.x < Px(150.0f)) s.x = Px(150.0f);
        if (s.x > (float)ancho - Px(150.0f)) s.x = (float)ancho - Px(150.0f);

        float alfa = aviso.vida < 0.45f ? aviso.vida / 0.45f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        char texto[32];
        std::snprintf(texto, sizeof(texto), "¡J%d AL FOSO!", participantes[aviso.jugador].numeroJugador);
        DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, participantes[aviso.jugador].color, alfa);
    }

    // Anuncios del muro.
    if (fase == FASE_MUROS_JUGANDO && tiempoAnuncio >= 0.0f)
    {
        float u = tiempoAnuncio;
        float alfa = u < 0.15f ? u / 0.15f : (u > DURACION_ANUNCIO_MUROS - 0.4f ? (DURACION_ANUNCIO_MUROS - u) / 0.4f : 1.0f);
        float yAnuncio = (float)alto * 0.2f;
        Color acento = { 150, 250, 232, 255 };

        if (tipoAnuncio == 1)
        {
            DibujarTextoMundoContornoUI("¡BUSCÁ EL HUECO!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, acento, alfa);
        }
        else if (tipoAnuncio == 2)
        {
            DibujarTextoMundoContornoUI("¡DOS HUECOS!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, acento, alfa);
        }
        else if (tipoAnuncio == 3)
        {
            char texto[32];
            std::snprintf(texto, sizeof(texto), "¡MURO %d!", muroAnunciado);
            DibujarTextoMundoContornoUI(texto, (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 255, 214, 140, 255 }, alfa);
            DibujarTextoMundoContornoUI("CADA VEZ MÁS RÁPIDOS", (float)ancho * 0.5f, yAnuncio + Px(40.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "MUROS LOCOS",
        "PASÁ POR EL HUECO ANTES DE QUE EL MURO DE ADOBE TE EMPUJE AL FOSO. GANA EL ÚLTIMO EN PIE.",
        resultado
    );

    hud.fase = fase == FASE_MUROS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_MUROS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin limite de tiempo: el medidor muestra el muro y cuanto acelero.
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "MURO";
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d", numeroMuro);
    hud.progresoMedidor = Limitar01VisualMuros(tiempoJugado / ESCALA_DIFICULTAD_MUROS);

    int vivos = ContarVivosMuros(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        const EstadoJugadorMurosLocos& estado = estadosJugadores[i];
        j.eliminado = estado.eliminado;

        int ms = estado.eliminado ? estado.tiempoSobrevividoMs : resultado.participantes[i].puntuacionMinijuego;

        if (estado.eliminado)
        {
            std::snprintf(j.valor, sizeof(j.valor), "%d,%d S", ms / 1000, (ms % 1000) / 100);
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "CAYÓ A LOS %d,%d S", ms / 1000, (ms % 1000) / 100);
        }
        else
        {
            if (fase == FASE_MUROS_JUGANDO && estado.destelloGolpe > 0.2f)
            {
                std::snprintf(j.estado, sizeof(j.estado), "¡EMPUJADO!");
            }

            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "EN PIE");
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "MOVÉ: %s  SALTÁ: %s\nGOLPE: %s",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (fase == FASE_MUROS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (vivos == 0)
        {
            hud.tituloFinal = "¡NADIE QUEDÓ EN EL PATIO!";
            hud.notaFinal = "CAYERON A LA VEZ: COMPARTEN PUESTO";
        }
        else if (vivos > 1)
        {
            hud.notaFinal = "SIGUEN EN PIE AL CERRAR LAS RUINAS: COMPARTEN PUESTO";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoMurosLocos::ObtenerResultado() const
{
    return resultado;
}
