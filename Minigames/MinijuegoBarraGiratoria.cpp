#include "Minigames/MinijuegoBarraGiratoria.h"

#include "Minigames/BotsMinijuegos1v3.h"
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
// La barra superior queda por encima de la cabeza de un jugador parado. Solo
// entra en su hitbox cuando el jugador salta o queda elevado por un impacto.
static const float ALTURA_FINAL_SEGUNDA_BARRA = 2.38f;
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


// Tiempo hasta que el borde delantero de una barra alcanza la posicion
// angular del jugador, y duracion del cruce sobre ese punto.
static void CalcularCruceBarraBot(
    float anguloBarra,
    float omega,
    float theta,
    float radioSeguro,
    float radioContacto,
    float& tiempoContacto,
    float& duracionCruce
)
{
    float delta = std::fmod(theta - anguloBarra, PI);
    if (delta < 0.0f) delta += PI;

    float razon = radioContacto / radioSeguro;
    float mitadAngulo = std::asin(razon < 1.0f ? razon : 1.0f);

    tiempoContacto = (delta - mitadAngulo) / omega;
    if (delta > PI - mitadAngulo) tiempoContacto = 0.0f;

    duracionCruce = 2.0f * mitadAngulo / omega;
}


// IA simple: cada bot orbita a un radio propio (lejos del centro, donde la
// barra pasa mas rapido y deja menos tiempo de contacto). Mientras la barra
// es lenta corre por delante de ella; cuando se vuelve rapida salta justo
// antes del cruce. El instante del salto lleva un error aleatorio, asi que
// algunos fallan y la partida siempre termina con un ganador.
static InputMinijuegoParticipante CrearEntradaBotBarra(
    MinijuegoBarraGiratoria& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    EstadoJugadorBarraGiratoria& bot = minijuego.estadosJugadores[indice];
    InputMinijuegoParticipante entrada{};

    if (bot.radioOrbitaBot <= 0.0f)
    {
        bot.radioOrbitaBot = (float)GetRandomValue(390, 450) / 100.0f;
        bot.errorSaltoBot = (float)GetRandomValue(-100, 100) / 1000.0f;
    }

    if (bot.enfriamientoSaltoBot > 0.0f)
    {
        bot.enfriamientoSaltoBot -= deltaTime;
    }

    float x = jugador.posicion.x;
    float z = jugador.posicion.z;
    float radio = MagnitudHorizontalBarra(x, z);
    float theta = std::atan2(z, x);

    float radioSeguro = radio > 0.8f ? radio : 0.8f;
    float radioContacto = RADIO_COLISION_BARRA + jugador.tamano.x * 0.34f;

    float omega = minijuego.velocidadAngular;
    float tiempoContacto = 0.0f;
    float duracionCruce = 0.0f;

    CalcularCruceBarraBot(
        minijuego.anguloBarra,
        omega,
        theta,
        radioSeguro,
        radioContacto,
        tiempoContacto,
        duracionCruce
    );

    bool barraRapida = omega * radioSeguro > 3.9f;

    float anguloObjetivo = theta;

    if (!barraRapida && tiempoContacto < 1.1f)
    {
        anguloObjetivo = theta + 0.40f;
    }

    InputMinijuegoParticipante movimiento =
        CrearEntradaBotHaciaObjetivo1v3(
            jugador.posicion,
            {
                std::cos(anguloObjetivo) * bot.radioOrbitaBot,
                0.0f,
                std::sin(anguloObjetivo) * bot.radioOrbitaBot
            },
            0.16f
        );

    entrada = movimiento;

    // Instante ideal de salto respecto de la barra base.
    float saltoIdeal = tiempoContacto + duracionCruce * 0.5f - 0.40f;
    float desfase = 0.0f;

    // La segunda barra solo golpea a quien esta en el aire (de 0.10 a 0.70 s
    // tras saltar). Si su cruce coincide con ese tramo, el bot adelanta o
    // retrasa el salto dentro de la holgura que deja la barra base.
    if (barraRapida && minijuego.segundaBarraLista)
    {
        float omega2 = minijuego.velocidadAngularSegunda;
        float contacto2 = 0.0f;
        float cruce2 = 0.0f;

        CalcularCruceBarraBot(
            minijuego.anguloSegundaBarra,
            omega2,
            theta,
            radioSeguro,
            radioContacto,
            contacto2,
            cruce2
        );

        float holgura = 0.35f - duracionCruce;
        if (holgura < 0.0f) holgura = 0.0f;

        const float candidatos[3] = { 0.0f, holgura * 0.4f, -holgura * 0.4f };

        for (int c = 0; c < 3; c++)
        {
            float salto = saltoIdeal + candidatos[c];
            if (salto < 0.0f) salto = 0.0f;

            bool choca = false;

            for (int vuelta = 0; vuelta < 3; vuelta++)
            {
                float inicio = contacto2 + (float)vuelta * PI / omega2;

                if (inicio <= salto + 0.70f && inicio + cruce2 >= salto + 0.10f)
                {
                    choca = true;
                    break;
                }
            }

            if (!choca)
            {
                desfase = candidatos[c];
                break;
            }
        }
    }

    if (
        barraRapida &&
        jugador.enSuelo &&
        bot.enfriamientoSaltoBot <= 0.0f &&
        saltoIdeal + desfase + bot.errorSaltoBot <= 0.0f
    )
    {
        entrada.saltar = true;
        bot.enfriamientoSaltoBot = 0.62f;
        bot.errorSaltoBot = (float)GetRandomValue(-100, 100) / 1000.0f;
    }

    return entrada;
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

    // La barra base acelera sin depender de un temporizador de fin. Al cabo
    // de suficiente tiempo supera claramente la velocidad lineal estandar de
    // los jugadores incluso cerca del centro de la arena.
    float aumentoVelocidad = tiempoJugado * 0.12f;
    if (aumentoVelocidad > 5.20f)
    {
        aumentoVelocidad = 5.20f;
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

        if (estadoJugador.tiempoStunBarra <= 0.0f)
        {
            if (participantes[i].esBot || !participantes[i].conectado)
            {
                entrada = CrearEntradaBotBarra(
                    *this,
                    i,
                    jugador,
                    deltaTime
                );
            }
            else
            {
                entrada = LeerInputMinijuegoParticipante(participantes[i]);
            }
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

    // Tope duro de seguridad: la ronda nunca queda abierta indefinidamente.
    if (vivosDespues <= 1 || tiempoJugado > 150.0f)
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


// Anillo de color bajo los pies y flecha sobre la cabeza: el modelo del
// jugador es oscuro y sobre la arena gris no se distingue quien es quien.
// MODELO FUTURO: la flecha puede pasar a ser un icono de jugador del GLB.
static void DibujarIndicadorJugadorBarra(
    const JugadorPrueba& jugador,
    Color color
)
{
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (jugador.cayendo || pies < -0.35f) return;

    Vector3 centro = { jugador.posicion.x, pies + 0.04f, jugador.posicion.z };
    for (int k = 0; k < 3; k++)
    {
        DrawCircle3D(
            centro,
            0.52f + k * 0.045f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            color
        );
    }

    float rebote = std::sin((float)GetTime() * 5.0f + jugador.posicion.x) * 0.06f;
    float cabeza = pies + 2.15f + rebote;
    DrawCylinderEx(
        { jugador.posicion.x, cabeza, jugador.posicion.z },
        { jugador.posicion.x, cabeza + 0.32f, jugador.posicion.z },
        0.0f,
        0.20f,
        10,
        color
    );
}


// Pilar de la arena sobre un mar de nubes, borde de advertencia y marcas
// en el suelo. Solo visual: usa primitivas V/Ex para no generar sombras
// automaticas sobre los elementos grandes.
// MODELO FUTURO: reemplazar por una plataforma industrial GLB.
static void DibujarEntornoBarra(float tiempo)
{
    DrawCylinderEx(
        { 0.0f, -0.64f, 0.0f },
        { 0.0f, -14.0f, 0.0f },
        RADIO_ARENA_BARRA - 0.6f,
        2.6f,
        32,
        Color{ 58, 62, 72, 255 }
    );

    for (int k = 0; k < 4; k++)
    {
        float y = -1.6f - k * 1.5f;
        float radio = RADIO_ARENA_BARRA - 0.54f - k * 0.45f;
        DrawCylinderEx(
            { 0.0f, y, 0.0f },
            { 0.0f, y - 0.14f, 0.0f },
            radio,
            radio,
            32,
            Color{ 232, 178, 48, 255 }
        );
    }

    // Mar de nubes bajo la arena, con deriva lenta.
    for (int i = 0; i < 18; i++)
    {
        float a = (float)i * 2.399f;
        float r = 10.0f + std::fmod((float)i * 5.3f, 14.0f);
        float x = std::cos(a) * r + std::sin(tiempo * 0.15f + (float)i) * 0.8f;
        float z = std::sin(a) * r * 0.8f - 2.0f;
        float tam = 3.0f + (float)(i % 4) * 1.2f;
        Color nube = i % 2 == 0
            ? Color{ 246, 248, 252, 255 }
            : Color{ 214, 230, 244, 255 };

        DrawCubeV({ x, -7.5f - (float)(i % 3), z }, { tam, 1.1f, tam * 0.7f }, nube);
        DrawCubeV({ x + tam * 0.25f, -6.8f - (float)(i % 3), z }, { tam * 0.6f, 0.9f, tam * 0.5f }, nube);
    }

    // Borde de advertencia alrededor de la arena.
    for (int i = 0; i < 40; i++)
    {
        float a = (float)i / 40.0f * 2.0f * PI;
        Vector3 pos =
        {
            std::cos(a) * (RADIO_ARENA_BARRA - 0.18f),
            0.05f,
            std::sin(a) * (RADIO_ARENA_BARRA - 0.18f)
        };
        DrawCubeV(
            pos,
            { 0.42f, 0.04f, 0.42f },
            i % 2 == 0 ? Color{ 240, 196, 40, 255 } : Color{ 30, 32, 38, 255 }
        );
    }

    DrawCircle3D({ 0.0f, 0.045f, 0.0f }, 2.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(RAYWHITE, 0.30f));
    DrawCircle3D({ 0.0f, 0.045f, 0.0f }, 3.9f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(RAYWHITE, 0.20f));
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

    DibujarEntornoBarra((float)GetTime());

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

    float altoEje = segundaBarraAparecio ? 2.90f : 1.10f;

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
        DibujarIndicadorJugadorBarra(jugadores[i], participantes[i].color);

        if (mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(jugadores[i]),
                LIME
            );
        }
    }

    EndMode3D();

    DrawRectangle(15, 15, 700, 148, Fade(BLACK, 0.55f));
    DrawRectangle(15, 170, 270, 24 * 4 + 8, Fade(BLACK, 0.55f));
    if (fase == FASE_BARRA_JUGANDO) DrawRectangle(GetScreenWidth() - 345, 17, 335, 36, Fade(BLACK, 0.55f));

    DrawText("BARRA GIRATORIA", 25, 25, 30, RAYWHITE);
    DrawText(
        "ULTIMO EN LA PLATAFORMA GANA. A LOS 10 s CAE UNA SEGUNDA BARRA.",
        25,
        66,
        19,
        LIGHTGRAY
    );

    DrawText(
        "LA BARRA SUPERIOR SOLO ALCANZA A QUIEN SALTA; LA BASE ACELERA SIN PARAR.",
        25,
        92,
        16,
        SKYBLUE
    );

    DrawText(
        "GOLPE: E / SHIFT / B   |   GROUND POUND: SALTO EN EL AIRE",
        25,
        116,
        16,
        SKYBLUE
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
            SKYBLUE
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
                ORANGE
            );
        }
        else if (!segundaBarraLista)
        {
            DrawText(
                "SEGUNDA BARRA: CAYENDO",
                GetScreenWidth() - 300,
                25,
                20,
                Color{ 255, 110, 100, 255 }
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
                Color{ 255, 110, 100, 255 }
            );
        }
    }

    int yEstado = 175;

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
            TextoReinicioMinijuego(),
            GetScreenWidth() / 2 - MeasureText(TextoReinicioMinijuego(), 22) / 2,
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
