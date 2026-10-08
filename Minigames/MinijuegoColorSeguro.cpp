#include "Minigames/MinijuegoColorSeguro.h"
#include "Minigames/CombateMinijuegos.h"
#include "Minigames/BotsMinijuegos1v3.h"

#include <cmath>


static const float DURACION_PREPARACION_COLOR = 3.0f;
static const float DURACION_TEXTO_YA_COLOR = 0.75f;
static const float RADIO_HEXAGONO_COLOR = 2.35f;

// Color Seguro ya utilizaba un aplastamiento mas corto que el perfil
// estandar. Se conserva de forma explicita al centralizar el impacto.
static const float DURACION_APLASTADO_GROUND_POUND_COLOR = 1.15f;


static Color ObtenerColorPlataforma(int indice)
{
    switch (indice)
    {
        case 0: return RAYWHITE;
        case 1: return Color{ 242, 214, 74, 255 };
        case 2: return Color{ 226, 58, 55, 255 };
        case 3: return Color{ 74, 184, 92, 255 };
        case 4: return Color{ 228, 126, 177, 255 };
        case 5: return Color{ 78, 92, 201, 255 };
        case 6: return Color{ 71, 189, 205, 255 };
    }

    return WHITE;
}


static const char* NombreColorPlataforma(int indice)
{
    switch (indice)
    {
        case 0: return "BLANCO";
        case 1: return "AMARILLO";
        case 2: return "ROJO";
        case 3: return "VERDE";
        case 4: return "ROSA";
        case 5: return "VIOLETA";
        case 6: return "CELESTE";
    }

    return "?";
}


static Color OscurecerColor(Color color, float factor)
{
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    return Color
    {
        (unsigned char)((float)color.r * factor),
        (unsigned char)((float)color.g * factor),
        (unsigned char)((float)color.b * factor),
        color.a
    };
}


static void DibujarPlataformaHexagonal(
    const BloquePrueba& plataforma
)
{
    const float mitadAltura = plataforma.tamano.y / 2.0f;

    Vector3 arriba[6]{};
    Vector3 abajo[6]{};

    for (int i = 0; i < 6; i++)
    {
        float angulo =
            (30.0f + 60.0f * (float)i) * DEG2RAD;

        float x = std::cos(angulo) * RADIO_HEXAGONO_COLOR;
        float z = std::sin(angulo) * RADIO_HEXAGONO_COLOR;

        arriba[i] =
        {
            plataforma.posicion.x + x,
            plataforma.posicion.y + mitadAltura,
            plataforma.posicion.z + z
        };

        abajo[i] =
        {
            plataforma.posicion.x + x,
            plataforma.posicion.y - mitadAltura,
            plataforma.posicion.z + z
        };
    }

    Vector3 centroArriba =
    {
        plataforma.posicion.x,
        plataforma.posicion.y + mitadAltura,
        plataforma.posicion.z
    };

    Vector3 centroAbajo =
    {
        plataforma.posicion.x,
        plataforma.posicion.y - mitadAltura,
        plataforma.posicion.z
    };

    Color lateral = OscurecerColor(plataforma.color, 0.34f);
    Color lateralAlterno = Color{ 38, 40, 45, 255 };
    Color inferior = OscurecerColor(plataforma.color, 0.20f);

    for (int i = 0; i < 6; i++)
    {
        int siguiente = (i + 1) % 6;

        DrawTriangle3D(
            centroArriba,
            arriba[siguiente],
            arriba[i],
            plataforma.color
        );

        DrawTriangle3D(
            centroAbajo,
            abajo[i],
            abajo[siguiente],
            inferior
        );

        Color cara = i % 2 == 0 ? lateral : lateralAlterno;

        DrawTriangle3D(
            arriba[i],
            arriba[siguiente],
            abajo[siguiente],
            cara
        );

        DrawTriangle3D(
            arriba[i],
            abajo[siguiente],
            abajo[i],
            cara
        );

        DrawLine3D(arriba[i], arriba[siguiente], BLACK);
        DrawLine3D(arriba[i], abajo[i], Fade(BLACK, 0.72f));
    }
}


static void DibujarTelevisor(Color colorPantalla)
{
    Vector3 cuerpo = { 0.0f, 4.2f, -8.2f };

    DrawCube(cuerpo, 4.8f, 3.1f, 0.55f, Color{ 35, 35, 42, 255 });
    DrawCubeWires(cuerpo, 4.8f, 3.1f, 0.55f, BLACK);

    DrawCube(
        { 0.0f, 4.2f, -7.90f },
        4.05f,
        2.35f,
        0.08f,
        colorPantalla
    );

    DrawCubeWires(
        { 0.0f, 4.2f, -7.85f },
        4.10f,
        2.40f,
        0.05f,
        RAYWHITE
    );

    DrawCube({ -1.45f, 2.0f, -8.2f }, 0.30f, 1.45f, 0.30f, DARKGRAY);
    DrawCube({ 1.45f, 2.0f, -8.2f }, 0.30f, 1.45f, 0.30f, DARKGRAY);
    DrawCube({ 0.0f, 1.25f, -8.2f }, 4.0f, 0.25f, 1.0f, DARKGRAY);
}


// Anillo de color bajo los pies y flecha sobre la cabeza: el modelo del
// jugador es oscuro y sobre la lava no se distingue quien es quien.
// MODELO FUTURO: la flecha puede pasar a ser un icono de jugador del GLB.
static void DibujarIndicadorJugadorColorSeguro(
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


// Costras de basalto y brasas sobre el lago de lava: sin esto las
// plataformas flotan sobre un plano naranja liso. Usa primitivas V/Ex para
// que las sombras automaticas no ensucien el suelo.
static void DibujarLagoLavaColorSeguro(float tiempo)
{
    const float nivel = -2.20f;

    for (int i = 0; i < 14; i++)
    {
        float a = (float)i * 2.399f;
        float r = 6.0f + std::fmod((float)i * 3.7f, 9.0f);
        Vector3 pos = { std::cos(a) * r, nivel, std::sin(a) * r * 0.8f };
        float ancho = 1.6f + (float)(i % 4) * 0.7f;
        float largo = 1.0f + (float)(i % 3) * 0.6f;

        DrawCubeV(pos, { ancho, 0.05f, largo }, Color{ 96, 36, 26, 255 });

        float brillo = 0.5f + 0.5f * std::sin(tiempo * 1.8f + (float)i);
        DrawCubeV(
            { pos.x + ancho * 0.7f, nivel + 0.02f, pos.z },
            { 0.12f, 0.05f, largo * 1.4f },
            Color{ 255, (unsigned char)(190 + 60 * brillo), 70, 255 }
        );
    }

    // Resplandor bajo las plataformas.
    DrawCylinderEx(
        { 0.0f, nivel, 0.0f },
        { 0.0f, nivel + 0.03f, 0.0f },
        9.5f,
        9.5f,
        36,
        Color{ 255, 200, 80, 255 }
    );
    DrawCylinderEx(
        { 0.0f, nivel + 0.03f, 0.0f },
        { 0.0f, nivel + 0.05f, 0.0f },
        7.0f,
        7.0f,
        36,
        Color{ 255, 160, 45, 255 }
    );
}


static void InicializarResultadoColorSeguro(
    MinijuegoColorSeguro& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarJugadoresVivosColorSeguro(
    const MinijuegoColorSeguro& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int RegistrarEliminacionesColorSeguro(
    MinijuegoColorSeguro& minijuego,
    JugadorPrueba jugadores[],
    int vivosAntes
)
{
    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminados++;
        }
    }

    if (eliminados == 0)
    {
        return 0;
    }

    int posicion = vivosAntes - eliminados + 1;

    int tiempoMs =
        (int)std::lround(
            minijuego.tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            minijuego.estadosJugadores[i].eliminado = true;
            minijuego.estadosJugadores[i].posicionFinal = posicion;
            minijuego.estadosJugadores[i].tiempoSobrevividoMs = tiempoMs;

            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
            jugadores[i].tiempoRalentizado = 0.0f;
        }
    }

    return eliminados;
}


static void RegistrarRondaSobrevividaColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            minijuego.estadosJugadores[i].rondasSobrevividas++;
        }
    }
}


static void FinalizarResultadoColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int vivos = ContarJugadoresVivosColorSeguro(minijuego);

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

        EstadoJugadorColorSeguro& estadoJugador =
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

    minijuego.estado = COLOR_SEGURO_TERMINADO;
}


static void ActualizarDificultadColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    float reduccion =
        (float)(minijuego.numeroRonda - 1) * 0.18f;

    minijuego.duracionElegirPlataforma = 5.0f - reduccion;
    minijuego.duracionCaidaPlataformas = 2.0f - reduccion * 0.22f;

    if (minijuego.duracionElegirPlataforma < 2.0f)
    {
        minijuego.duracionElegirPlataforma = 2.0f;
    }

    if (minijuego.duracionCaidaPlataformas < 1.10f)
    {
        minijuego.duracionCaidaPlataformas = 1.10f;
    }
}


static void ElegirNuevaPlataformaSegura(
    MinijuegoColorSeguro& minijuego
)
{
    int anterior = minijuego.indicePlataformaSegura;

    if (minijuego.cantidadPlataformas <= 1)
    {
        minijuego.indicePlataformaSegura = 0;
    }
    else
    {
        do
        {
            minijuego.indicePlataformaSegura =
                GetRandomValue(0, minijuego.cantidadPlataformas - 1);
        }
        while (minijuego.indicePlataformaSegura == anterior);
    }

    minijuego.fase = FASE_ELEGIR_PLATAFORMA;
    minijuego.tiempoFase = minijuego.duracionElegirPlataforma;
}


static void TirarPlataformasIncorrectas(
    MinijuegoColorSeguro& minijuego
)
{
    for (int i = 0; i < minijuego.cantidadPlataformas; i++)
    {
        BloquePrueba& plataforma = minijuego.plataformas[i];

        if (i == minijuego.indicePlataformaSegura)
        {
            plataforma.activaColision = true;
            plataforma.cayendo = false;
        }
        else
        {
            plataforma.activaColision = true;
            plataforma.cayendo = true;
            plataforma.velocidadCaida = 0.0f;
        }
    }

    minijuego.fase = FASE_CAIDA_PLATAFORMAS;
    minijuego.tiempoFase = minijuego.duracionCaidaPlataformas;
}


static void ActualizarCaidaPlataformas(
    MinijuegoColorSeguro& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < minijuego.cantidadPlataformas; i++)
    {
        BloquePrueba& plataforma = minijuego.plataformas[i];

        if (!plataforma.cayendo)
        {
            continue;
        }

        plataforma.velocidadCaida += 12.0f * deltaTime;
        plataforma.posicion.y -= plataforma.velocidadCaida * deltaTime;
    }
}


static void ActualizarTemblorCamara(
    MinijuegoColorSeguro& minijuego,
    float deltaTime
)
{
    if (minijuego.tiempoTemblorCamara > 0.0f)
    {
        minijuego.tiempoTemblorCamara -= deltaTime;

        float dx =
            (float)GetRandomValue(-1000, 1000) /
            1000.0f * minijuego.intensidadTemblorCamara;

        float dy =
            (float)GetRandomValue(-1000, 1000) /
            1000.0f * minijuego.intensidadTemblorCamara;

        minijuego.camara.position = minijuego.posicionCamaraBase;
        minijuego.camara.target = minijuego.objetivoCamaraBase;
        minijuego.camara.position.x += dx;
        minijuego.camara.position.y += dy;
        minijuego.camara.target.x += dx * 0.45f;
        minijuego.camara.target.y += dy * 0.45f;
    }
    else
    {
        minijuego.tiempoTemblorCamara = 0.0f;
        minijuego.camara.position = minijuego.posicionCamaraBase;
        minijuego.camara.target = minijuego.objetivoCamaraBase;
    }
}


// IA simple: al empezar cada ronda el bot decide (tras un breve retraso de
// reaccion) correr hacia el color indicado por la TV. Con una probabilidad
// que crece por ronda se equivoca de plataforma, para que haya ganador.
static InputMinijuegoParticipante CrearEntradaBotColorSeguro(
    MinijuegoColorSeguro& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    EstadoJugadorColorSeguro& bot = minijuego.estadosJugadores[indice];

    if (bot.rondaBot != minijuego.numeroRonda)
    {
        bot.rondaBot = minijuego.numeroRonda;
        bot.retrasoBot = (float)GetRandomValue(120, 650) / 1000.0f;

        float probabilidadError =
            0.06f + 0.025f * (float)(minijuego.numeroRonda - 1);

        if (probabilidadError > 0.40f)
        {
            probabilidadError = 0.40f;
        }

        int destino = minijuego.indicePlataformaSegura;

        if (
            minijuego.cantidadPlataformas > 1 &&
            (float)GetRandomValue(0, 999) / 1000.0f < probabilidadError
        )
        {
            do
            {
                destino = GetRandomValue(0, minijuego.cantidadPlataformas - 1);
            }
            while (destino == minijuego.indicePlataformaSegura);
        }

        bot.objetivoBot = minijuego.plataformas[destino].posicion;
        bot.objetivoBot.x += (float)GetRandomValue(-800, 800) / 1000.0f;
        bot.objetivoBot.z += (float)GetRandomValue(-700, 700) / 1000.0f;
    }

    if (bot.retrasoBot > 0.0f)
    {
        bot.retrasoBot -= deltaTime;
        return InputMinijuegoParticipante{};
    }

    return CrearEntradaBotHaciaObjetivo1v3(
        jugador.posicion,
        bot.objetivoBot,
        0.25f
    );
}


void MinijuegoColorSeguro::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadPlataformas = 0;

    const float distanciaHorizontal = 4.07032f;
    const float medioHorizontal = distanciaHorizontal / 2.0f;
    const float distanciaZ = RADIO_HEXAGONO_COLOR * 1.5f;

    Vector3 posiciones[CANTIDAD_PLATAFORMAS_COLOR] =
    {
        { 0.0f, 0.0f, 0.0f },
        { -medioHorizontal, 0.0f, -distanciaZ },
        { medioHorizontal, 0.0f, -distanciaZ },
        { distanciaHorizontal, 0.0f, 0.0f },
        { medioHorizontal, 0.0f, distanciaZ },
        { -medioHorizontal, 0.0f, distanciaZ },
        { -distanciaHorizontal, 0.0f, 0.0f }
    };

    for (int i = 0; i < CANTIDAD_PLATAFORMAS_COLOR; i++)
    {
        AgregarBloquePrueba(
            plataformas,
            cantidadPlataformas,
            CANTIDAD_PLATAFORMAS_COLOR,
            posiciones[i],
            Vector3{ 4.10f, 0.60f, 3.65f },
            ObtenerColorPlataforma(i)
        );
    }

    estado = COLOR_SEGURO_PREPARACION;
    fase = FASE_ELEGIR_PLATAFORMA;
    indicePlataformaSegura = -1;
    numeroRonda = 1;

    tiempoPreparacion = DURACION_PREPARACION_COLOR;

    // Se conserva el miembro por compatibilidad con el struct, pero ya no
    // funciona como cuenta regresiva ni puede finalizar la partida.
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;

    duracionElegirPlataforma = 5.0f;
    duracionCaidaPlataformas = 2.0f;
    ActualizarDificultadColorSeguro(*this);
    tiempoFase = duracionElegirPlataforma;

    posicionCamaraBase = { 0.0f, 11.4f, 15.8f };
    objetivoCamaraBase = { 0.0f, 0.45f, -0.75f };

    camara.position = posicionCamaraBase;
    camara.target = objetivoCamaraBase;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 55.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    tiempoTemblorCamara = 0.0f;
    intensidadTemblorCamara = 0.0f;

    ElegirNuevaPlataformaSegura(*this);
}


void MinijuegoColorSeguro::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -0.65f, 1.0f, 0.55f },
        { 0.65f, 1.0f, 0.55f },
        { -0.65f, 1.0f, -0.55f },
        { 0.65f, 1.0f, -0.55f }
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


void MinijuegoColorSeguro::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    ReiniciarBloquesPrueba(plataformas, cantidadPlataformas);

    numeroRonda = 1;
    estado = COLOR_SEGURO_PREPARACION;
    fase = FASE_ELEGIR_PLATAFORMA;
    indicePlataformaSegura = -1;
    tiempoPreparacion = DURACION_PREPARACION_COLOR;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;

    duracionElegirPlataforma = 5.0f;
    duracionCaidaPlataformas = 2.0f;
    ActualizarDificultadColorSeguro(*this);
    ElegirNuevaPlataformaSegura(*this);

    tiempoTemblorCamara = 0.0f;
    camara.position = posicionCamaraBase;
    camara.target = objetivoCamaraBase;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ReiniciarJugadorPrueba(jugadores[i]);
    }
}


void MinijuegoColorSeguro::Actualizar(
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
        InicializarResultadoColorSeguro(*this, participantes);
    }

    if (estado == COLOR_SEGURO_TERMINADO)
    {
        return;
    }

    if (estado == COLOR_SEGURO_PREPARACION)
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
            estado = COLOR_SEGURO_JUGANDO;
        }

        return;
    }

    tiempoJugado += deltaTime;
    tiempoFase -= deltaTime;

    if (
        fase == FASE_ELEGIR_PLATAFORMA &&
        tiempoFase <= 0.0f
    )
    {
        TirarPlataformasIncorrectas(*this);
    }

    if (fase == FASE_CAIDA_PLATAFORMAS)
    {
        ActualizarCaidaPlataformas(*this, deltaTime);
    }

    int vivosAntes = ContarJugadoresVivosColorSeguro(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador = jugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            jugador.tiempoRalentizado = 0.0f;
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotColorSeguro(
                *this,
                i,
                jugador,
                deltaTime
            );
        }
        else if (participantes[i].activo)
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            plataformas,
            cantidadPlataformas,
            particulas,
            cantidadParticulas,
            true,
            false,
            deltaTime
        );
    }

    bool impactoSuelo = ResolverGolpesSuelo(
        jugadores,
        participantes,
        cantidadMaxima,
        DURACION_APLASTADO_GROUND_POUND_COLOR
    );

    if (impactoSuelo)
    {
        tiempoTemblorCamara = 0.24f;
        intensidadTemblorCamara = 0.20f;
    }

    ResolverGolpesJugadoresConEfectos(
        jugadores,
        participantes,
        cantidadMaxima,
        particulas,
        cantidadParticulas
    );

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantes,
        cantidadMaxima
    );

    int eliminados = RegistrarEliminacionesColorSeguro(
        *this,
        jugadores,
        vivosAntes
    );

    int vivosDespues = vivosAntes - eliminados;

    if (vivosDespues <= 1)
    {
        FinalizarResultadoColorSeguro(*this);
        ActualizarTemblorCamara(*this, deltaTime);
        return;
    }

    if (
        fase == FASE_CAIDA_PLATAFORMAS &&
        tiempoFase <= 0.0f
    )
    {
        RegistrarRondaSobrevividaColorSeguro(*this);
        ReiniciarBloquesPrueba(plataformas, cantidadPlataformas);
        numeroRonda++;

        ActualizarDificultadColorSeguro(*this);
        ElegirNuevaPlataformaSegura(*this);
    }

    ActualizarTemblorCamara(*this, deltaTime);
}


void MinijuegoColorSeguro::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 125, 190, 220, 255 });

    BeginMode3D(camara);

    DibujarLagoLavaColorSeguro((float)GetTime());

    for (int i = 0; i < cantidadPlataformas; i++)
    {
        DibujarPlataformaHexagonal(plataformas[i]);

        if (mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxBloquePrueba(plataformas[i]),
                YELLOW
            );
        }
    }

    Color colorPantalla =
        ObtenerColorPlataforma(indicePlataformaSegura);

    DibujarTelevisor(colorPantalla);
    DibujarParticulasTierra(particulas, cantidadParticulas);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);

        if (participantes[i].activo)
        {
            DibujarIndicadorJugadorColorSeguro(
                jugadores[i],
                participantes[i].color
            );
        }

        if (
            mostrarDebug &&
            participantes[i].activo &&
            participantes[i].conectado &&
            !jugadores[i].cayendo
        )
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(jugadores[i]),
                LIME
            );
        }
    }

    EndMode3D();

    // Panel estrecho: no tapa la TV que indica el color seguro.
    DrawRectangle(15, 15, 480, 198, Fade(BLACK, 0.62f));
    DrawRectangle(15, 218, 250, 24 * 4 + 8, Fade(BLACK, 0.62f));

    DrawText("MINIJUEGO 1 - COLOR SEGURO", 25, 25, 30, RAYWHITE);

    DrawText(
        TextFormat(
            "COLOR: %s",
            NombreColorPlataforma(indicePlataformaSegura)
        ),
        25,
        70,
        26,
        colorPantalla
    );

    DrawText(
        TextFormat(
            "RONDA: %d   TIEMPO JUGADO: %.1f s",
            numeroRonda,
            tiempoJugado
        ),
        25,
        105,
        22,
        RAYWHITE
    );

    DrawText(
        fase == FASE_ELEGIR_PLATAFORMA
        ? "CORRE AL COLOR DE LA TV"
        : "SOLO QUEDA LA PLATAFORMA CORRECTA",
        25,
        138,
        20,
        RAYWHITE
    );

    DrawText(
        "SIN LIMITE: CADA RONDA DA MENOS TIEMPO",
        25,
        168,
        17,
        LIGHTGRAY
    );

    DrawText(
        "AIRE: GROUND POUND   E/SHIFT/B: GOLPEAR",
        25,
        190,
        17,
        LIGHTGRAY
    );

    int yEstado = 223;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const char* textoEstado =
            estadosJugadores[i].eliminado
            ? "ELIMINADO"
            : (
                participantes[i].conectado
                ? "EN JUEGO"
                : "SIN CONTROL"
            );

        DrawText(
            TextFormat(
                "J%d %s  RONDAS %d",
                participantes[i].numeroJugador,
                textoEstado,
                estadosJugadores[i].rondasSobrevividas
            ),
            25,
            yEstado,
            18,
            estadosJugadores[i].eliminado
            ? GRAY
            : participantes[i].color
        );

        yEstado += 24;
    }

    if (estado == COLOR_SEGURO_PREPARACION)
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
    else if (
        estado == COLOR_SEGURO_JUGANDO &&
        tiempoJugado < DURACION_TEXTO_YA_COLOR
    )
    {
        const char* texto = "YA";

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            LIME
        );
    }
    else if (estado == COLOR_SEGURO_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 340,
            GetScreenHeight() / 2 - 170,
            680,
            340,
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
            GetScreenHeight() / 2 - 142,
            34,
            GOLD
        );

        int y = GetScreenHeight() / 2 - 88;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s  RONDAS %d",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    estadosJugadores[i].tiempoSobrevividoMs / 1000.0f,
                    estadosJugadores[i].rondasSobrevividas
                ),
                GetScreenWidth() / 2 - 255,
                y,
                21,
                participantes[i].color
            );

            y += 31;
        }

        const char* reiniciar = TextoReinicioMinijuego();

        DrawText(
            reiniciar,
            GetScreenWidth() / 2 - MeasureText(reiniciar, 22) / 2,
            GetScreenHeight() / 2 + 126,
            22,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoColorSeguro::ObtenerResultado() const
{
    return resultado;
}
