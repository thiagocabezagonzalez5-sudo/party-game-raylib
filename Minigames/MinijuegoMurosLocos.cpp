#include "Minigames/MinijuegoMurosLocos.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


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

    suelo = {};
    suelo.posicion = { 0.0f, -0.35f, 0.0f };
    suelo.posicionInicial = suelo.posicion;
    suelo.tamano = { ANCHO_ARENA_MUROS, 0.70f, LARGO_ARENA_MUROS };
    suelo.color = Color{ 202, 153, 82, 255 };
    suelo.activaColision = true;

    fase = FASE_MUROS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_MUROS;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;
    tiempoEntreMuros = 0.30f;
    numeroMuro = 0;

    muro = {};
    PrepararNuevoMuro(*this);

    camara.position = { 0.0f, 10.2f, 13.7f };
    camara.target = { 0.0f, 0.35f, -0.3f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
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

    if (fase == FASE_MUROS_TERMINADO) return;

    if (fase == FASE_MUROS_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -= deltaTime;

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
        if (participantes[i].conectado)
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

        if (tiempoEntreMuros <= 0.0f)
        {
            bool contacto = ResolverColisionMuroJugador(muro, jugador);

            if (contacto && estadoJugador.cooldownImpacto <= 0.0f)
            {
                CrearImpactoMuro(jugador, particulas, cantidadParticulas);
                estadoJugador.cooldownImpacto = COOLDOWN_IMPACTO_MURO;
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
        }
    }

    if (vivosAntes - eliminados <= 1)
    {
        FinalizarMuros(*this);
    }
}


//==================================================
// ESCENARIO DESERTICO
//==================================================

static void DibujarEscenarioDesiertoMuros()
{
    DibujarCieloPlanoEscenario3D(
        Color{ 226, 166, 87, 255 },
        -70.0f,
        180.0f,
        62.0f
    );

    DrawSphere(
        { 18.0f, 13.0f, -50.0f },
        3.0f,
        Color{ 255, 226, 117, 255 }
    );

    DrawPlane(
        { 0.0f, -0.72f, -4.0f },
        { 150.0f, 150.0f },
        Color{ 205, 153, 78, 255 }
    );

    const float posiciones[7] =
    {
        -54.0f, -36.0f, -18.0f, 0.0f, 19.0f, 38.0f, 57.0f
    };

    for (int i = 0; i < 7; i++)
    {
        float altura = 5.0f + (float)(i % 3) * 1.2f;
        float ancho = 23.0f + (float)(i % 2) * 5.0f;

        DibujarMontanaPrismaEscenario3D(
            { posiciones[i], -5.0f, -34.0f - (float)(i % 2) * 5.0f },
            ancho,
            altura,
            9.0f,
            i % 2 == 0
                ? Color{ 165, 103, 57, 255 }
                : Color{ 183, 119, 64, 255 },
            Color{ 130, 78, 48, 255 },
            Color{ 165, 103, 57, 255 },
            0.0f
        );
    }

    EstadoEfectosVisualesMinijuegos& efectos =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 30; i++)
    {
        float x = -18.0f + RepetirEscenario3D(
            (float)(i * 7) + efectos.tiempoGlobal * 1.2f,
            36.0f
        );
        float y = 0.15f + RepetirEscenario3D((float)(i * 5), 5.5f);
        float z = -4.0f - RepetirEscenario3D((float)(i * 9), 24.0f);

        DrawSphere(
            { x, y, z },
            0.035f + 0.018f * (float)(i % 3),
            Fade(Color{ 124, 88, 57, 255 }, 0.48f)
        );
    }
}


static Color ColorLadrilloMuro(int fila, int columna, int segmento)
{
    int variante = (fila + columna + segmento) % 4;

    switch (variante)
    {
        case 0: return Color{ 181, 105, 59, 255 };
        case 1: return Color{ 198, 121, 65, 255 };
        case 2: return Color{ 166, 91, 53, 255 };
        default: return Color{ 190, 112, 61, 255 };
    }
}


static void DibujarSegmentoMuro(
    float xCentro,
    float ancho,
    float z,
    int indice
)
{
    if (ancho <= 0.03f) return;

    const int filas = 4;
    const float altoLadrillo = ALTO_MURO / (float)filas;
    const float anchoLadrillo = 0.95f;
    const float junta = 0.035f;
    float bordeIzquierdo = xCentro - ancho / 2.0f;
    float bordeDerecho = xCentro + ancho / 2.0f;

    for (int fila = 0; fila < filas; fila++)
    {
        float desplazamiento = fila % 2 == 0 ? 0.0f : anchoLadrillo * 0.5f;
        float inicio = bordeIzquierdo - desplazamiento;
        int columna = 0;

        for (float x = inicio; x < bordeDerecho; x += anchoLadrillo)
        {
            float izquierda = x > bordeIzquierdo ? x : bordeIzquierdo;
            float derecha = x + anchoLadrillo < bordeDerecho
                ? x + anchoLadrillo
                : bordeDerecho;
            float anchoVisible = derecha - izquierda;

            if (anchoVisible <= junta)
            {
                columna++;
                continue;
            }

            Vector3 centro =
            {
                (izquierda + derecha) * 0.5f,
                altoLadrillo * ((float)fila + 0.5f),
                z
            };

            Color ladrillo = ColorLadrilloMuro(fila, columna, indice);

            DrawCube(
                centro,
                anchoVisible - junta,
                altoLadrillo - junta,
                GROSOR_MURO,
                ladrillo
            );

            DrawCubeWires(
                centro,
                anchoVisible - junta,
                altoLadrillo - junta,
                GROSOR_MURO,
                Color{ 92, 57, 40, 255 }
            );

            columna++;
        }
    }
}


static void DibujarMuroLoco(const MuroLoco& muro)
{
    float bordeIzquierdo = -LIMITE_X_MUROS;
    int segmento = 0;

    for (int i = 0; i < muro.cantidadHuecos; i++)
    {
        float inicioHueco = muro.centrosHueco[i] - muro.anchoHueco / 2.0f;
        float finHueco = muro.centrosHueco[i] + muro.anchoHueco / 2.0f;

        if (inicioHueco > bordeIzquierdo)
        {
            float ancho = inicioHueco - bordeIzquierdo;
            DibujarSegmentoMuro(
                bordeIzquierdo + ancho / 2.0f,
                ancho,
                muro.z,
                segmento++
            );
        }

        bordeIzquierdo = finHueco;
    }

    if (bordeIzquierdo < LIMITE_X_MUROS)
    {
        float ancho = LIMITE_X_MUROS - bordeIzquierdo;
        DibujarSegmentoMuro(
            bordeIzquierdo + ancho / 2.0f,
            ancho,
            muro.z,
            segmento
        );
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
    ClearBackground(Color{ 226, 166, 87, 255 });
    BeginMode3D(camara);

    DibujarEscenarioDesiertoMuros();

    DrawCube(
        suelo.posicion,
        suelo.tamano.x,
        suelo.tamano.y,
        suelo.tamano.z,
        suelo.color
    );

    DrawCubeWires(
        suelo.posicion,
        suelo.tamano.x,
        suelo.tamano.y,
        suelo.tamano.z,
        Color{ 112, 72, 45, 255 }
    );

    if (fase == FASE_MUROS_JUGANDO && tiempoEntreMuros <= 0.0f)
    {
        DibujarMuroLoco(muro);
    }

    DibujarParticulasTierra(particulas, cantidadParticulas);

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);

        if (mostrarDebug && !jugadores[i].cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    EndMode3D();

    DrawText("MUROS LOCOS - RUINAS DEL DESIERTO", 24, 22, 30, DARKBROWN);
    DrawText(
        "BUSCA EL HUECO ANTES DE QUE EL MURO DE ADOBE TE EMPUJE.",
        24,
        60,
        18,
        Color{ 91, 58, 39, 255 }
    );
    DrawText(
        "LOS MUROS SE ACELERAN HASTA QUE SOLO QUEDE UN JUGADOR.",
        24,
        86,
        16,
        Color{ 112, 72, 45, 255 }
    );

    if (fase == FASE_MUROS_JUGANDO)
    {
        DrawText(
            TextFormat("MURO %d", numeroMuro),
            GetScreenWidth() - 190,
            24,
            20,
            DARKBROWN
        );
    }

    if (fase == FASE_MUROS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 88) / 2,
            GetScreenHeight() / 2 - 62,
            88,
            ORANGE
        );
    }
    else if (fase == FASE_MUROS_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 315,
            GetScreenHeight() / 2 - 140,
            630,
            280,
            Fade(BLACK, 0.90f)
        );

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(
            resultado,
            ganadores,
            MAX_PARTICIPANTES
        );

        const char* titulo =
            resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                    ? participantes[ganadores[0]].numeroJugador
                    : 0
            );

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 108,
            34,
            GOLD
        );

        int fila = GetScreenHeight() / 2 - 50;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo) continue;

            DrawText(
                TextFormat(
                    "J%d  POS %d  %.2f s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    resultado.participantes[i].puntuacionMinijuego / 1000.0f
                ),
                GetScreenWidth() / 2 - 170,
                fila,
                21,
                participantes[i].color
            );

            fila += 29;
        }

        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 21) / 2,
            GetScreenHeight() / 2 + 105,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoMurosLocos::ObtenerResultado() const
{
    return resultado;
}
