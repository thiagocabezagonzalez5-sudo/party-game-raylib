#include "Minigames/MinijuegoBarraGiratoria.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_BARRA = 3.0f;
static const float RADIO_ARENA_BARRA = 5.4f;
static const float LONGITUD_MEDIA_BARRA = 5.65f;
static const float ALTURA_BARRA = 0.82f;
static const float RADIO_COLISION_BARRA = 0.34f;

static const float FUERZA_IMPACTO_BARRA = 12.8f;
static const float FUERZA_IMPACTO_BARRA_SUPERIOR = 14.6f;
static const float IMPULSO_VERTICAL_BARRA = 7.0f;
static const float IMPULSO_VERTICAL_BARRA_SUPERIOR = 7.6f;
static const float COOLDOWN_IMPACTO_BARRA = 0.42f;
static const float DURACION_STUN_BARRA = 0.36f;
static const float DURACION_RALENTIZACION_BARRA = 0.78f;
static const float MULTIPLICADOR_RALENTIZACION_BARRA = 0.68f;

static const float TIEMPO_APARICION_SEGUNDA_BARRA = 10.0f;
static const float ALTURA_INICIAL_SEGUNDA_BARRA = 8.0f;
static const float ALTURA_FINAL_SEGUNDA_BARRA = 1.58f;
static const float VELOCIDAD_CAIDA_SEGUNDA_BARRA = 7.2f;
static const float MULTIPLICADOR_VELOCIDAD_SEGUNDA_BARRA = 1.5f;


static float MagnitudHorizontalBarra(float x, float z)
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

    float margen = jugador.tamano.x * 0.18f;

    return distancia <= RADIO_ARENA_BARRA - margen;
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
        proyeccion = -LONGITUD_MEDIA_BARRA;

    if (proyeccion > LONGITUD_MEDIA_BARRA)
        proyeccion = LONGITUD_MEDIA_BARRA;

    float puntoX = direccionX * proyeccion;
    float puntoZ = direccionZ * proyeccion;

    float dx = jugador.posicion.x - puntoX;
    float dz = jugador.posicion.z - puntoZ;

    return std::sqrt(dx * dx + dz * dz);
}


static bool BarraTocaJugador(
    const JugadorPrueba& jugador,
    float angulo,
    float altura
)
{
    float pies =
        jugador.posicion.y -
        jugador.tamano.y / 2.0f;

    float cabeza =
        jugador.posicion.y +
        jugador.tamano.y / 2.0f;

    bool solapaVertical =
        cabeza >= altura - RADIO_COLISION_BARRA &&
        pies <= altura + RADIO_COLISION_BARRA;

    if (!solapaVertical)
    {
        return false;
    }

    return
        DistanciaJugadorALineaBarra(jugador, angulo) <=
        RADIO_COLISION_BARRA + jugador.tamano.x * 0.34f;
}


static void AplicarImpactoBarra(
    JugadorPrueba& jugador,
    EstadoJugadorBarraGiratoria& estadoJugador,
    float alturaImpacto,
    float fuerza,
    float impulsoVertical,
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

    jugador.empuje.x += normalX * fuerza;
    jugador.empuje.z += normalZ * fuerza;
    jugador.velocidad.y = impulsoVertical;
    jugador.enSuelo = false;

    if (jugador.tiempoRalentizado < DURACION_RALENTIZACION_BARRA)
    {
        jugador.tiempoRalentizado = DURACION_RALENTIZACION_BARRA;
    }

    if (
        jugador.multiplicadorRalentizacion <= 0.0f ||
        jugador.multiplicadorRalentizacion > MULTIPLICADOR_RALENTIZACION_BARRA
    )
    {
        jugador.multiplicadorRalentizacion =
            MULTIPLICADOR_RALENTIZACION_BARRA;
    }

    estadoJugador.tiempoStunBarra = DURACION_STUN_BARRA;
    estadoJugador.cooldownImpacto = COOLDOWN_IMPACTO_BARRA;

    CrearParticulasImpactoGolpe(
        particulas,
        cantidadParticulas,
        {
            jugador.posicion.x,
            alturaImpacto,
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

    int vivos = ContarVivosBarra(minijuego);

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
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
            estadoJugador.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_BARRA_TERMINADO;
}


void MinijuegoBarraGiratoria::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    suelo = {};
    suelo.posicion = { 0.0f, -0.35f, 0.0f };
    suelo.posicionInicial = suelo.posicion;
    suelo.tamano = { 11.6f, 0.70f, 11.6f };
    suelo.color = Color{ 93, 97, 106, 255 };
    suelo.activaColision = true;

    fase = FASE_BARRA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_BARRA;
    tiempoJugado = 0.0f;

    anguloBarra = 0.0f;
    velocidadAngular = 0.9f;

    segundaBarraAparecio = false;
    segundaBarraLista = false;
    alturaSegundaBarra = ALTURA_INICIAL_SEGUNDA_BARRA;
    anguloSegundaBarra = PI / 2.0f;
    velocidadAngularSegunda =
        velocidadAngular * MULTIPLICADOR_VELOCIDAD_SEGUNDA_BARRA;

    camara.position = { 0.0f, 10.5f, 13.8f };
    camara.target = { 0.0f, 0.45f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoBarraGiratoria::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.4f, 1.05f,  2.4f },
        {  2.4f, 1.05f,  2.4f },
        { -2.4f, 1.05f, -2.4f },
        {  2.4f, 1.05f, -2.4f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
            ? cantidadMaxima
            : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawns[i]);
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
        participaban[i] = resultado.participantes[i].participo;
    }

    Inicializar();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo = participaban[i];

        if (participaban[i])
        {
            resultado.cantidadParticipantes++;
        }
    }

    ConfigurarJugadores(jugadores, cantidadMaxima);
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

    tiempoJugado += deltaTime;

    float aumentoVelocidad = tiempoJugado * 0.055f;
    if (aumentoVelocidad > 2.15f)
    {
        aumentoVelocidad = 2.15f;
    }

    velocidadAngular = 0.9f + aumentoVelocidad;
    anguloBarra += velocidadAngular * deltaTime;

    while (anguloBarra > 2.0f * PI)
    {
        anguloBarra -= 2.0f * PI;
    }

    if (
        !segundaBarraAparecio &&
        tiempoJugado >= TIEMPO_APARICION_SEGUNDA_BARRA
    )
    {
        segundaBarraAparecio = true;
        segundaBarraLista = false;
        alturaSegundaBarra = ALTURA_INICIAL_SEGUNDA_BARRA;
        anguloSegundaBarra = anguloBarra + PI / 2.0f;
    }

    if (segundaBarraAparecio)
    {
        velocidadAngularSegunda =
            velocidadAngular * MULTIPLICADOR_VELOCIDAD_SEGUNDA_BARRA;

        anguloSegundaBarra +=
            velocidadAngularSegunda * deltaTime;

        while (anguloSegundaBarra > 2.0f * PI)
        {
            anguloSegundaBarra -= 2.0f * PI;
        }

        if (!segundaBarraLista)
        {
            alturaSegundaBarra -=
                VELOCIDAD_CAIDA_SEGUNDA_BARRA * deltaTime;

            if (alturaSegundaBarra <= ALTURA_FINAL_SEGUNDA_BARRA)
            {
                alturaSegundaBarra = ALTURA_FINAL_SEGUNDA_BARRA;
                segundaBarraLista = true;
                ActivarTemblorCamaraGeneral(0.12f, 0.20f);
            }
        }
    }

    int vivosAntes = ContarVivosBarra(*this);

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

        if (estadoJugador.tiempoStunBarra > 0.0f)
        {
            estadoJugador.tiempoStunBarra -= deltaTime;
            if (estadoJugador.tiempoStunBarra < 0.0f)
            {
                estadoJugador.tiempoStunBarra = 0.0f;
            }
        }

        InputMinijuegoParticipante entrada{};

        if (
            participantes[i].conectado &&
            estadoJugador.tiempoStunBarra <= 0.0f
        )
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        BloquePrueba sueloJugador = suelo;
        sueloJugador.activaColision = JugadorSobreArenaBarra(jugador);

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
            BarraTocaJugador(jugador, anguloBarra, ALTURA_BARRA)
        )
        {
            AplicarImpactoBarra(
                jugador,
                estadoJugador,
                ALTURA_BARRA,
                FUERZA_IMPACTO_BARRA,
                IMPULSO_VERTICAL_BARRA,
                particulas,
                cantidadParticulas
            );
        }
        else if (
            segundaBarraLista &&
            estadoJugador.cooldownImpacto <= 0.0f &&
            !jugador.cayendo &&
            BarraTocaJugador(
                jugador,
                anguloSegundaBarra,
                ALTURA_FINAL_SEGUNDA_BARRA
            )
        )
        {
            AplicarImpactoBarra(
                jugador,
                estadoJugador,
                ALTURA_FINAL_SEGUNDA_BARRA,
                FUERZA_IMPACTO_BARRA_SUPERIOR,
                IMPULSO_VERTICAL_BARRA_SUPERIOR,
                particulas,
                cantidadParticulas
            );
        }

        if (
            !JugadorSobreArenaBarra(jugador) &&
            jugador.posicion.y < -2.0f
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

    int vivosDespues = vivosAntes - eliminados;

    if (vivosDespues <= 1)
    {
        FinalizarBarra(*this);
    }
}


static void DibujarBarraSegmentada(
    float angulo,
    float altura,
    Color colorA,
    Color colorB
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
            { dx * t, altura, dz * t },
            0.42f,
            0.30f,
            0.42f,
            i % 2 == 0 ? colorA : colorB
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

    ClearBackground(Color{ 123, 192, 221, 255 });

    BeginMode3D(camara);

    DrawCylinder(
        { 0.0f, -0.32f, 0.0f },
        RADIO_ARENA_BARRA,
        RADIO_ARENA_BARRA,
        0.64f,
        48,
        Color{ 72, 75, 83, 255 }
    );

    DrawCylinder(
        { 0.0f, -0.01f, 0.0f },
        RADIO_ARENA_BARRA,
        RADIO_ARENA_BARRA,
        0.06f,
        48,
        Color{ 120, 125, 136, 255 }
    );

    DrawCircle3D(
        { 0.0f, 0.03f, 0.0f },
        RADIO_ARENA_BARRA,
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Fade(RAYWHITE, 0.35f)
    );

    float altoEje = segundaBarraAparecio ? 2.10f : 1.10f;

    DrawCylinder(
        { 0.0f, altoEje / 2.0f, 0.0f },
        0.40f,
        0.40f,
        altoEje,
        20,
        DARKGRAY
    );

    DibujarBarraSegmentada(
        anguloBarra,
        ALTURA_BARRA,
        ORANGE,
        Color{ 245, 205, 70, 255 }
    );

    if (segundaBarraAparecio)
    {
        if (!segundaBarraLista)
        {
            DrawCircle3D(
                { 0.0f, 0.035f, 0.0f },
                LONGITUD_MEDIA_BARRA * 0.96f,
                { 1.0f, 0.0f, 0.0f },
                90.0f,
                Fade(RED, 0.24f)
            );
        }

        DibujarBarraSegmentada(
            anguloSegundaBarra,
            alturaSegundaBarra,
            Color{ 216, 68, 80, 255 },
            Color{ 244, 120, 72, 255 }
        );
    }

    DibujarParticulasTierra(particulas, cantidadParticulas);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);

        if (mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(jugadores[i]),
                LIME
            );
        }
    }

    EndMode3D();

    DrawText("BARRA GIRATORIA", 25, 25, 30, BLACK);
    DrawText(
        "ULTIMO EN LA PLATAFORMA GANA. A LOS 10 s CAE UNA SEGUNDA BARRA.",
        25,
        66,
        19,
        DARKGRAY
    );

    DrawText(
        "LAS BARRAS EMPUJAN, STUNEAN 0.36 s Y RALENTIZAN BREVEMENTE.",
        25,
        92,
        16,
        DARKBLUE
    );

    DrawText(
        "GOLPE: E / SHIFT / B   |   GROUND POUND: SALTO EN EL AIRE",
        25,
        116,
        16,
        DARKBLUE
    );

    if (fase == FASE_BARRA_JUGANDO)
    {
        DrawText(
            TextFormat(
                "SUPERVIVENCIA: %.1f s   BARRA BASE: %.2f",
                tiempoJugado,
                velocidadAngular
            ),
            25,
            145,
            19,
            DARKBLUE
        );

        if (!segundaBarraAparecio)
        {
            float falta = TIEMPO_APARICION_SEGUNDA_BARRA - tiempoJugado;
            if (falta < 0.0f) falta = 0.0f;

            DrawText(
                TextFormat("SEGUNDA BARRA EN: %.1f s", falta),
                GetScreenWidth() - 280,
                25,
                20,
                MAROON
            );
        }
        else if (!segundaBarraLista)
        {
            DrawText(
                "SEGUNDA BARRA: CAYENDO",
                GetScreenWidth() - 300,
                25,
                20,
                RED
            );
        }
        else
        {
            DrawText(
                TextFormat(
                    "BARRA SUPERIOR: %.2f  (x1.5)",
                    velocidadAngularSegunda
                ),
                GetScreenWidth() - 335,
                25,
                20,
                RED
            );
        }
    }

    int yEstado = 178;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const char* estadoTexto =
            estadosJugadores[i].eliminado
                ? "FUERA"
                : (
                    estadosJugadores[i].tiempoStunBarra > 0.0f
                        ? "STUN"
                        : "EN JUEGO"
                );

        DrawText(
            TextFormat(
                "J%d %s%s",
                participantes[i].numeroJugador,
                participantes[i].esBot ? "BOT - " : "",
                estadoTexto
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
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
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
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
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

        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 22) / 2,
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
