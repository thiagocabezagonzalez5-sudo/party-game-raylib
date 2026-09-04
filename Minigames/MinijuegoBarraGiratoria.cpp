#include "Minigames/MinijuegoBarraGiratoria.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_BARRA = 3.0f;
static const float DURACION_PARTIDA_BARRA = 45.0f;
static const float RADIO_ARENA_BARRA = 5.4f;
static const float LONGITUD_MEDIA_BARRA = 5.65f;
static const float ALTURA_BARRA = 0.82f;
static const float RADIO_COLISION_BARRA = 0.34f;
static const float COOLDOWN_IMPACTO_BARRA = 0.62f;


static float MagnitudHorizontalBarra(
    float x,
    float z
)
{
    return std::sqrt(x * x + z * z);
}


static bool JugadorSobreArenaBarra(
    const JugadorPrueba& jugador
)
{
    float distancia =
        MagnitudHorizontalBarra(
            jugador.posicion.x,
            jugador.posicion.z
        );

    float margen =
        jugador.tamano.x * 0.18f;

    return distancia <=
        RADIO_ARENA_BARRA - margen;
}


static int ContarVivosBarra(
    const MinijuegoBarraGiratoria& minijuego
)
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


static float DistanciaJugadorALineaBarra(
    const JugadorPrueba& jugador,
    float angulo
)
{
    float direccionX = std::cos(angulo);
    float direccionZ = std::sin(angulo);

    float proyeccion =
        jugador.posicion.x * direccionX +
        jugador.posicion.z * direccionZ;

    if (proyeccion < -LONGITUD_MEDIA_BARRA)
    {
        proyeccion = -LONGITUD_MEDIA_BARRA;
    }

    if (proyeccion > LONGITUD_MEDIA_BARRA)
    {
        proyeccion = LONGITUD_MEDIA_BARRA;
    }

    float puntoX = direccionX * proyeccion;
    float puntoZ = direccionZ * proyeccion;

    float dx = jugador.posicion.x - puntoX;
    float dz = jugador.posicion.z - puntoZ;

    return std::sqrt(dx * dx + dz * dz);
}


static bool BarraTocaJugador(
    const JugadorPrueba& jugador,
    float angulo
)
{
    float pies =
        jugador.posicion.y -
        jugador.tamano.y / 2.0f;

    float cabeza =
        jugador.posicion.y +
        jugador.tamano.y / 2.0f;

    bool solapaVertical =
        cabeza >= ALTURA_BARRA - RADIO_COLISION_BARRA &&
        pies <= ALTURA_BARRA + RADIO_COLISION_BARRA;

    if (!solapaVertical)
    {
        return false;
    }

    return DistanciaJugadorALineaBarra(
        jugador,
        angulo
    ) <= RADIO_COLISION_BARRA + jugador.tamano.x * 0.34f;
}


static void AplicarImpactoBarra(
    JugadorPrueba& jugador,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    float distancia =
        MagnitudHorizontalBarra(
            jugador.posicion.x,
            jugador.posicion.z
        );

    float normalX = 1.0f;
    float normalZ = 0.0f;

    if (distancia > 0.05f)
    {
        normalX = jugador.posicion.x / distancia;
        normalZ = jugador.posicion.z / distancia;
    }

    jugador.empuje.x += normalX * 10.5f;
    jugador.empuje.z += normalZ * 10.5f;
    jugador.velocidad.y = 7.4f;
    jugador.enSuelo = false;

    CrearParticulasImpactoGolpe(
        particulas,
        cantidadParticulas,
        Vector3{
            jugador.posicion.x,
            ALTURA_BARRA,
            jugador.posicion.z
        }
    );
}


static void FinalizarBarra(
    MinijuegoBarraGiratoria& minijuego
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int vivos =
        ContarVivosBarra(minijuego);

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        vivos == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(
            minijuego.tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorBarraGiratoria& estadoJugador =
            minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs =
                tiempoFinalMs;
        }

        resultadoJugador.posicionFinal =
            estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase =
        FASE_BARRA_TERMINADO;
}


void MinijuegoBarraGiratoria::Inicializar()
{
    resultado = {};
    resultado.formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    suelo = {};
    suelo.posicion =
        { 0.0f, -0.35f, 0.0f };
    suelo.posicionInicial =
        suelo.posicion;
    suelo.tamano =
        { 11.6f, 0.70f, 11.6f };
    suelo.color =
        Color{ 79, 126, 91, 255 };
    suelo.activaColision = true;

    fase = FASE_BARRA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_BARRA;
    tiempoRestante = DURACION_PARTIDA_BARRA;
    tiempoJugado = 0.0f;
    anguloBarra = 0.0f;
    velocidadAngular = 0.9f;

    camara.position =
        { 0.0f, 10.5f, 13.8f };
    camara.target =
        { 0.0f, 0.30f, 0.0f };
    camara.up =
        { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection =
        CAMERA_PERSPECTIVE;
}


void MinijuegoBarraGiratoria::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.4f, 1.05f, 2.4f },
        { 2.4f, 1.05f, 2.4f },
        { -2.4f, 1.05f, -2.4f },
        { 2.4f, 1.05f, -2.4f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            spawns[i]
        );
    }
}


void MinijuegoBarraGiratoria::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participaban[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participaban[i] =
            resultado.participantes[i].participo;
    }

    Inicializar();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo =
            participaban[i];

        if (participaban[i])
        {
            resultado.cantidadParticipantes++;
        }
    }

    ConfigurarJugadores(
        jugadores,
        cantidadMaxima
    );
}


void MinijuegoBarraGiratoria::Actualizar(
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

    if (fase == FASE_BARRA_TERMINADO)
    {
        return;
    }

    if (fase == FASE_BARRA_PREPARACION)
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
            fase = FASE_BARRA_JUGANDO;
        }

        return;
    }

    tiempoRestante -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;

    tiempoJugado =
        DURACION_PARTIDA_BARRA -
        tiempoRestante;

    float progreso =
        tiempoJugado /
        DURACION_PARTIDA_BARRA;

    velocidadAngular =
        0.9f + progreso * 1.9f;

    anguloBarra +=
        velocidadAngular * deltaTime;

    if (anguloBarra > 6.283185f)
    {
        anguloBarra -= 6.283185f;
    }

    int vivosAntes =
        ContarVivosBarra(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador = jugadores[i];
        EstadoJugadorBarraGiratoria& estadoJugador =
            estadosJugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estadoJugador.eliminado
        )
        {
            continue;
        }

        if (estadoJugador.cooldownImpacto > 0.0f)
        {
            estadoJugador.cooldownImpacto -= deltaTime;
            if (estadoJugador.cooldownImpacto < 0.0f)
            {
                estadoJugador.cooldownImpacto = 0.0f;
            }
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(
                participantes[i]
            );

        entrada.golpear = false;

        BloquePrueba sueloJugador = suelo;
        sueloJugador.activaColision =
            JugadorSobreArenaBarra(jugador);

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

        if (
            estadoJugador.cooldownImpacto <= 0.0f &&
            !jugador.cayendo &&
            BarraTocaJugador(
                jugador,
                anguloBarra
            )
        )
        {
            AplicarImpactoBarra(
                jugador,
                particulas,
                cantidadParticulas
            );

            estadoJugador.cooldownImpacto =
                COOLDOWN_IMPACTO_BARRA;
        }

        if (
            !JugadorSobreArenaBarra(jugador) &&
            jugador.posicion.y < -2.0f
        )
        {
            jugador.cayendo = true;
        }
    }

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantes,
        cantidadMaxima
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

    int posicion =
        vivosAntes - eliminados + 1;

    int tiempoMs =
        (int)std::lround(
            tiempoJugado * 1000.0f
        );

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

    int vivosDespues =
        vivosAntes - eliminados;

    if (
        vivosDespues <= 1 ||
        tiempoRestante <= 0.0f
    )
    {
        FinalizarBarra(*this);
    }
}


static void DibujarBarraSegmentada(
    float angulo
)
{
    float dx = std::cos(angulo);
    float dz = std::sin(angulo);

    const int segmentos = 32;

    for (int i = 0; i < segmentos; i++)
    {
        float t =
            -LONGITUD_MEDIA_BARRA +
            (LONGITUD_MEDIA_BARRA * 2.0f) *
            ((float)i / (float)(segmentos - 1));

        DrawCube(
            Vector3{
                dx * t,
                ALTURA_BARRA,
                dz * t
            },
            0.42f,
            0.30f,
            0.42f,
            i % 2 == 0
                ? ORANGE
                : Color{ 245, 205, 70, 255 }
        );
    }
}


void MinijuegoBarraGiratoria::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(
        Color{ 123, 192, 221, 255 }
    );

    BeginMode3D(camara);

    DrawCylinder(
        Vector3{ 0.0f, -0.25f, 0.0f },
        RADIO_ARENA_BARRA,
        RADIO_ARENA_BARRA + 0.30f,
        0.50f,
        48,
        Color{ 74, 121, 83, 255 }
    );

    DrawCylinder(
        Vector3{ 0.0f, 0.02f, 0.0f },
        RADIO_ARENA_BARRA,
        RADIO_ARENA_BARRA,
        0.10f,
        48,
        Color{ 104, 174, 112, 255 }
    );

    DrawCylinder(
        Vector3{ 0.0f, 0.52f, 0.0f },
        0.40f,
        0.40f,
        1.10f,
        20,
        DARKGRAY
    );

    DibujarBarraSegmentada(
        anguloBarra
    );

    DibujarParticulasTierra(
        particulas,
        cantidadParticulas
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        DibujarJugadorCuboPrueba(
            jugadores[i],
            participantes[i]
        );

        if (mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(
                    jugadores[i]
                ),
                LIME
            );
        }
    }

    EndMode3D();

    DrawText(
        "BARRA GIRATORIA",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        "SALTA LA BARRA Y NO TE CAIGAS DE LA ARENA",
        25,
        66,
        20,
        DARKGRAY
    );

    if (fase == FASE_BARRA_JUGANDO)
    {
        DrawText(
            TextFormat(
                "TIEMPO: %.1f   VELOCIDAD: %.1f",
                tiempoRestante,
                velocidadAngular
            ),
            25,
            96,
            20,
            DARKBLUE
        );
    }

    int yEstado = 128;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        DrawText(
            TextFormat(
                "J%d %s%s",
                participantes[i].numeroJugador,
                participantes[i].esBot ? "BOT - " : "",
                estadosJugadores[i].eliminado
                    ? "FUERA"
                    : "EN JUEGO"
            ),
            25,
            yEstado,
            18,
            estadosJugadores[i].eliminado
                ? DARKGRAY
                : participantes[i].color
        );

        yEstado += 24;
    }

    if (fase == FASE_BARRA_PREPARACION)
    {
        int numero =
            (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto =
            TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            ORANGE
        );
    }
    else if (fase == FASE_BARRA_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 330,
            GetScreenHeight() / 2 - 155,
            660,
            310,
            Fade(BLACK, 0.90f)
        );

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores =
            ObtenerIndicesGanadores(
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
            GetScreenWidth() / 2 -
                MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 128,
            34,
            GOLD
        );

        int y = GetScreenHeight() / 2 - 70;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POS %d   %.2f s%s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    resultado.participantes[i].puntuacionMinijuego / 1000.0f,
                    participantes[i].esBot ? "  BOT" : ""
                ),
                GetScreenWidth() / 2 - 205,
                y,
                21,
                participantes[i].color
            );

            y += 29;
        }

        const char* reiniciar =
            "R PARA REINICIAR";

        DrawText(
            reiniciar,
            GetScreenWidth() / 2 -
                MeasureText(reiniciar, 22) / 2,
            GetScreenHeight() / 2 + 112,
            22,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoBarraGiratoria::ObtenerResultado() const
{
    return resultado;
}
