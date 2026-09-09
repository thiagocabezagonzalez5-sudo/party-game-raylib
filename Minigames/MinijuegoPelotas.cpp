#include "Minigames/MinijuegoPelotas.h"

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_PELOTAS = 3.0f;
static const float DURACION_TEXTO_YA_PELOTAS = 0.75f;

static const float VELOCIDAD_MAXIMA_PELOTAS = 9.0f;
static const float ACELERACION_MAXIMA_PELOTAS = 6.0f;
static const float MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS = 1.40f;
static const float VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS = 13.5f;

// La colision usa exactamente el mismo borde irregular que se dibuja. El
// margen se aplica al centro de la pelota para que empiece a caer cuando una
// parte importante del personaje ya sobrepaso la cornisa visual.
static const float RADIO_ARENA_PELOTAS = 6.35f;
static const float FACTOR_MARGEN_CENTRO_PELOTAS = 0.43f;
static const int SEGMENTOS_ARENA_PELOTAS = 48;
static const float ALTURA_BASE_MONTANA_PELOTAS = -2.10f;
static const float CRECIMIENTO_RADIO_MONTANA_PELOTAS = 1.15f;


static float MagnitudHorizontalPelotas(float x, float z)
{
    return std::sqrt(x * x + z * z);
}


static float FactorIrregularidadBordePelotas(float angulo)
{
    return
        1.0f +
        0.018f * std::sin(angulo * 5.0f) +
        0.012f * std::cos(angulo * 9.0f);
}


static float RadioVisualBordePelotas(float x, float z)
{
    float angulo = std::atan2(z, x);
    return RADIO_ARENA_PELOTAS * FactorIrregularidadBordePelotas(angulo);
}


static float RadioSoportePelotas(
    float x,
    float z,
    float tamanoJugador
)
{
    return
        RadioVisualBordePelotas(x, z) -
        tamanoJugador * FACTOR_MARGEN_CENTRO_PELOTAS;
}


static void LimitarMovimientoPelota(
    JugadorPrueba& jugador,
    float velocidadAnteriorX,
    float velocidadAnteriorZ,
    float deltaTime
)
{
    float anterior = MagnitudHorizontalPelotas(
        velocidadAnteriorX,
        velocidadAnteriorZ
    );

    float actual = MagnitudHorizontalPelotas(
        jugador.velocidad.x,
        jugador.velocidad.z
    );

    if (actual <= 0.001f)
    {
        return;
    }

    float limiteAceleracion =
        anterior + ACELERACION_MAXIMA_PELOTAS * deltaTime;

    float limite =
        limiteAceleracion < VELOCIDAD_MAXIMA_PELOTAS
        ? limiteAceleracion
        : VELOCIDAD_MAXIMA_PELOTAS;

    if (actual > limite)
    {
        float factor = limite / actual;
        jugador.velocidad.x *= factor;
        jugador.velocidad.z *= factor;
    }
}


static void AplicarEmpujePendientePelotas(
    JugadorPrueba& jugador,
    Vector3 empujePendiente
)
{
    jugador.velocidad.x += empujePendiente.x;
    jugador.velocidad.z += empujePendiente.z;

    float velocidad = MagnitudHorizontalPelotas(
        jugador.velocidad.x,
        jugador.velocidad.z
    );

    if (velocidad > VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS)
    {
        float factor = VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS / velocidad;
        jugador.velocidad.x *= factor;
        jugador.velocidad.z *= factor;
    }
}


static bool JugadorSobreArenaCircularPelotas(
    const JugadorPrueba& jugador
)
{
    float distancia = MagnitudHorizontalPelotas(
        jugador.posicion.x,
        jugador.posicion.z
    );

    float radioSoporte = RadioSoportePelotas(
        jugador.posicion.x,
        jugador.posicion.z,
        jugador.tamano.x
    );

    return distancia <= radioSoporte;
}


static void ResolverColisionMontanaPelotas(
    JugadorPrueba& jugador,
    Vector3 posicionAnterior
)
{
    float radioPelota = jugador.tamano.x / 2.0f;
    float distancia = MagnitudHorizontalPelotas(
        jugador.posicion.x,
        jugador.posicion.z
    );

    bool tocaAlturaMontana =
        jugador.posicion.y - radioPelota < 0.0f &&
        jugador.posicion.y + radioPelota > ALTURA_BASE_MONTANA_PELOTAS;

    if (tocaAlturaMontana)
    {
        float progresoAltura =
            -jugador.posicion.y / -ALTURA_BASE_MONTANA_PELOTAS;

        if (progresoAltura < 0.0f) progresoAltura = 0.0f;
        if (progresoAltura > 1.0f) progresoAltura = 1.0f;

        float radioSuperior = RadioVisualBordePelotas(
            jugador.posicion.x,
            jugador.posicion.z
        );

        float radioMontana =
            radioSuperior +
            CRECIMIENTO_RADIO_MONTANA_PELOTAS * progresoAltura;

        float distanciaMinima =
            radioMontana + radioPelota * 0.82f;

        float zonaCercanaAlBorde =
            radioSuperior - radioPelota * 0.45f;

        if (
            distancia > zonaCercanaAlBorde &&
            distancia < distanciaMinima
        )
        {
            float normalX = 1.0f;
            float normalZ = 0.0f;

            if (distancia > 0.001f)
            {
                normalX = jugador.posicion.x / distancia;
                normalZ = jugador.posicion.z / distancia;
            }
            else
            {
                float anterior = MagnitudHorizontalPelotas(
                    posicionAnterior.x,
                    posicionAnterior.z
                );

                if (anterior > 0.001f)
                {
                    normalX = posicionAnterior.x / anterior;
                    normalZ = posicionAnterior.z / anterior;
                }
            }

            jugador.posicion.x = normalX * distanciaMinima;
            jugador.posicion.z = normalZ * distanciaMinima;

            float velocidadHaciaCentro =
                jugador.velocidad.x * normalX +
                jugador.velocidad.z * normalZ;

            if (velocidadHaciaCentro < 0.0f)
            {
                jugador.velocidad.x -= normalX * velocidadHaciaCentro;
                jugador.velocidad.z -= normalZ * velocidadHaciaCentro;
            }
        }
    }

    float radioBase =
        RadioVisualBordePelotas(jugador.posicion.x, jugador.posicion.z) +
        CRECIMIENTO_RADIO_MONTANA_PELOTAS;

    float parteSuperiorAnterior = posicionAnterior.y + radioPelota;
    float parteSuperiorActual = jugador.posicion.y + radioPelota;

    if (
        posicionAnterior.y < ALTURA_BASE_MONTANA_PELOTAS &&
        jugador.velocidad.y > 0.0f &&
        parteSuperiorAnterior <= ALTURA_BASE_MONTANA_PELOTAS &&
        parteSuperiorActual >= ALTURA_BASE_MONTANA_PELOTAS &&
        distancia <= radioBase + radioPelota
    )
    {
        jugador.posicion.y =
            ALTURA_BASE_MONTANA_PELOTAS - radioPelota - 0.001f;
        jugador.velocidad.y = 0.0f;
    }
}


static void PotenciarEmpujesPelotas(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            jugadores[i].cayendo
        )
        {
            continue;
        }

        jugadores[i].empuje.x *= MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS;
        jugadores[i].empuje.z *= MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS;
    }
}


static float IrregularidadBordePelotas(int indice)
{
    float angulo =
        (2.0f * PI * (float)indice) /
        (float)SEGMENTOS_ARENA_PELOTAS;

    return FactorIrregularidadBordePelotas(angulo);
}


static Vector3 PuntoCircularPelotas(
    int indice,
    float radio,
    float altura
)
{
    float angulo =
        (2.0f * PI * (float)indice) /
        (float)SEGMENTOS_ARENA_PELOTAS;

    float irregularidad = IrregularidadBordePelotas(indice);

    return
    {
        std::cos(angulo) * radio * irregularidad,
        altura,
        std::sin(angulo) * radio * irregularidad
    };
}


static Vector3 PuntoLimiteSoportePelotas(
    int indice,
    float tamanoJugador,
    float altura
)
{
    float angulo =
        (2.0f * PI * (float)indice) /
        (float)SEGMENTOS_ARENA_PELOTAS;

    float radio =
        RADIO_ARENA_PELOTAS * FactorIrregularidadBordePelotas(angulo) -
        tamanoJugador * FACTOR_MARGEN_CENTRO_PELOTAS;

    return
    {
        std::cos(angulo) * radio,
        altura,
        std::sin(angulo) * radio
    };
}


static void DibujarMontanaNievePelotas(bool mostrarDebug)
{
    const Color nieveSuperior = Color{ 246, 250, 252, 255 };
    const Color nieveSombra = Color{ 207, 225, 235, 255 };
    const Color hieloClaro = Color{ 153, 190, 208, 255 };
    const Color hieloOscuro = Color{ 93, 132, 153, 255 };
    Vector3 centroSuperior = { 0.0f, 0.015f, 0.0f };

    for (int i = 0; i < SEGMENTOS_ARENA_PELOTAS; i++)
    {
        int siguiente = (i + 1) % SEGMENTOS_ARENA_PELOTAS;

        Vector3 cimaA = PuntoCircularPelotas(
            i,
            RADIO_ARENA_PELOTAS,
            0.0f
        );
        Vector3 cimaB = PuntoCircularPelotas(
            siguiente,
            RADIO_ARENA_PELOTAS,
            0.0f
        );
        Vector3 nieveA = PuntoCircularPelotas(
            i,
            RADIO_ARENA_PELOTAS + 0.50f,
            -0.55f
        );
        Vector3 nieveB = PuntoCircularPelotas(
            siguiente,
            RADIO_ARENA_PELOTAS + 0.50f,
            -0.55f
        );
        Vector3 baseA = PuntoCircularPelotas(
            i,
            RADIO_ARENA_PELOTAS + 1.15f,
            ALTURA_BASE_MONTANA_PELOTAS
        );
        Vector3 baseB = PuntoCircularPelotas(
            siguiente,
            RADIO_ARENA_PELOTAS + 1.15f,
            ALTURA_BASE_MONTANA_PELOTAS
        );

        DrawTriangle3D(centroSuperior, cimaB, cimaA, nieveSuperior);
        DrawTriangle3D(
            cimaA,
            cimaB,
            nieveB,
            i % 2 == 0 ? nieveSuperior : nieveSombra
        );
        DrawTriangle3D(
            cimaA,
            nieveB,
            nieveA,
            i % 2 == 0 ? nieveSuperior : nieveSombra
        );
        DrawTriangle3D(
            nieveA,
            nieveB,
            baseB,
            i % 2 == 0 ? hieloClaro : hieloOscuro
        );
        DrawTriangle3D(
            nieveA,
            baseB,
            baseA,
            i % 2 == 0 ? hieloClaro : hieloOscuro
        );
        DrawLine3D(cimaA, cimaB, Fade(SKYBLUE, 0.45f));
    }

    if (mostrarDebug)
    {
        const float TAMANO_PELOTA_DEBUG = 1.30f;

        for (int i = 0; i < SEGMENTOS_ARENA_PELOTAS; i++)
        {
            int siguiente = (i + 1) % SEGMENTOS_ARENA_PELOTAS;

            DrawLine3D(
                PuntoLimiteSoportePelotas(i, TAMANO_PELOTA_DEBUG, 0.07f),
                PuntoLimiteSoportePelotas(siguiente, TAMANO_PELOTA_DEBUG, 0.07f),
                RED
            );
        }
    }
}


static void InicializarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarJugadoresVivosPelotas(
    const MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void FinalizarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    int vivos = ContarJugadoresVivosPelotas(minijuego, participantes);

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        vivos == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(minijuego.tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorPelotas& estadoJugador = minijuego.estadosJugadores[i];

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

    minijuego.fase = FASE_PELOTAS_TERMINADO;
}


void MinijuegoPelotas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;
    fase = FASE_PELOTAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PELOTAS;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadBloques = 0;

    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        1,
        { 0.0f, -0.5f, 0.0f },
        { 13.2f, 1.0f, 13.2f },
        RAYWHITE
    );

    camara.position = { 0.0f, 9.0f, 14.2f };
    camara.target = { 0.0f, -0.15f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 52.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoPelotas::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.2f, 0.65f,  2.2f },
        {  2.2f, 0.65f,  2.2f },
        { -2.2f, 0.65f, -2.2f },
        {  2.2f, 0.65f, -2.2f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        jugadores[i].posicionSpawn = spawns[i];
        jugadores[i].tamano = { 1.30f, 1.30f, 1.30f };
        jugadores[i].velocidadMovimiento = VELOCIDAD_MAXIMA_PELOTAS;
        jugadores[i].fuerzaSalto = 0.0f;
        jugadores[i].gravedad = 18.0f;
        jugadores[i].duracionRespawn = 1.2f;
        ReiniciarJugadorPrueba(jugadores[i]);
    }
}


void MinijuegoPelotas::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participantesAnteriores[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantesAnteriores[i] = resultado.participantes[i].participo;
    }

    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo = participantesAnteriores[i];
        if (participantesAnteriores[i]) resultado.cantidadParticipantes++;
        estadosJugadores[i] = {};
    }

    fase = FASE_PELOTAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PELOTAS;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ReiniciarJugadorPrueba(jugadores[i]);
    }
}


void MinijuegoPelotas::Actualizar(
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
        InicializarResultadoPelotas(*this, participantes);
    }

    if (fase == FASE_PELOTAS_TERMINADO)
    {
        return;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
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
            fase = FASE_PELOTAS_JUGANDO;
        }

        return;
    }

    tiempoJugado += deltaTime;

    int vivosAntes = ContarJugadoresVivosPelotas(*this, participantes);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador = jugadores[i];

        if (estadosJugadores[i].eliminado)
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            continue;
        }

        if (!participantes[i].activo || !participantes[i].conectado)
        {
            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participantes[i]);

        float velocidadAnteriorX = jugador.velocidad.x;
        float velocidadAnteriorZ = jugador.velocidad.z;
        Vector3 posicionAnterior = jugador.posicion;
        Vector3 empujePendiente = jugador.empuje;
        jugador.empuje = {};

        BloquePrueba sueloJugador = bloques[0];
        sueloJugador.activaColision = JugadorSobreArenaCircularPelotas(jugador);

        ActualizarJugadorPrueba(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            false,
            true,
            false,
            deltaTime
        );

        if (!JugadorSobreArenaCircularPelotas(jugador))
        {
            jugador.enSuelo = false;
        }

        LimitarMovimientoPelota(
            jugador,
            velocidadAnteriorX,
            velocidadAnteriorZ,
            deltaTime
        );

        AplicarEmpujePendientePelotas(jugador, empujePendiente);
        ResolverColisionMontanaPelotas(jugador, posicionAnterior);
    }

    ResolverColisionesPelotas(jugadores, participantes, cantidadMaxima);
    PotenciarEmpujesPelotas(jugadores, participantes, cantidadMaxima);

    int eliminadosEsteFrame = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminadosEsteFrame++;
        }
    }

    int posicionEliminados = vivosAntes - eliminadosEsteFrame + 1;
    int tiempoSobrevividoMs =
        (int)std::lround(tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal = posicionEliminados;
            estadosJugadores[i].tiempoSobrevividoMs = tiempoSobrevividoMs;
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }
    }

    int vivosDespues = vivosAntes - eliminadosEsteFrame;

    if (vivosDespues <= 1)
    {
        FinalizarResultadoPelotas(*this, participantes);
    }
}


void MinijuegoPelotas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 181, 220, 238, 255 });
    BeginMode3D(camara);

    DibujarMontanaNievePelotas(mostrarDebug);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        DibujarJugadorPelotaPrueba(jugadores[i], participantes[i]);

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

    DrawText("MINIJUEGO 2 - PELOTAS / EMPUJONES", 25, 25, 30, BLACK);
    DrawText("EMPUJA A LOS DEMAS FUERA DE LA MONTANA", 25, 70, 22, BLACK);
    DrawText("NIEVE RESBALADIZA - ACELERA Y CONSERVA LA INERCIA", 25, 102, 20, BLACK);
    DrawText("MAS VELOCIDAD = MAS FUERZA DE EMPUJE", 25, 132, 20, MAROON);

    int posicionY = 168;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        float velocidad = MagnitudHorizontalPelotas(
            jugadores[i].velocidad.x,
            jugadores[i].velocidad.z
        );

        DrawText(
            TextFormat(
                "J%d VELOCIDAD: %.1f / %.1f",
                participantes[i].numeroJugador,
                velocidad,
                VELOCIDAD_MAXIMA_PELOTAS
            ),
            25,
            posicionY,
            18,
            participantes[i].color
        );

        posicionY += 24;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
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
        fase == FASE_PELOTAS_JUGANDO &&
        tiempoJugado < DURACION_TEXTO_YA_PELOTAS
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
    else if (fase == FASE_PELOTAS_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 330,
            GetScreenHeight() / 2 - 155,
            660,
            310,
            Fade(BLACK, 0.90f)
        );

        int indicesGanadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(
            resultado,
            indicesGanadores,
            MAX_PARTICIPANTES
        );

        const char* titulo =
            resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                    ? participantes[indicesGanadores[0]].numeroJugador
                    : 0
            );

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 130,
            34,
            GOLD
        );

        int y = GetScreenHeight() / 2 - 75;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    resultado.participantes[i].puntuacionMinijuego / 1000.0f
                ),
                GetScreenWidth() / 2 - 210,
                y,
                22,
                participantes[i].color
            );

            y += 30;
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
MinijuegoPelotas::ObtenerResultado() const
{
    return resultado;
}
