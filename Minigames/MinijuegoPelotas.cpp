#include "Minigames/MinijuegoPelotas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_PELOTAS = 3.0f;
static const float DURACION_TEXTO_YA_PELOTAS = 0.75f;
static const float DURACION_PARTIDA_PELOTAS = 60.0f;
static const float GRACIA_INICIAL_PELOTAS = 1.2f;

static const float VELOCIDAD_MAXIMA_PELOTAS = 9.0f;
static const float ACELERACION_MAXIMA_PELOTAS = 6.0f;
static const float MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS = 1.40f;
static const float VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS = 13.5f;

// La colision superior usa exactamente el mismo borde irregular que se
// dibuja. La colision lateral conserva ese radio hacia abajo: la montana
// visual se abre hacia la base, pero su nucleo solido no debe empujar a los
// jugadores hasta el contorno exterior decorativo.
static const float RADIO_ARENA_PELOTAS = 6.35f;
static const int SEGMENTOS_ARENA_PELOTAS = 48;
static const float ALTURA_BASE_MONTANA_PELOTAS = -2.10f;
static const float ALTURA_DEBUG_COLISION_MONTANA_PELOTAS = -1.70f;


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
    float z
)
{
    return RadioVisualBordePelotas(x, z);
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
    const JugadorPrueba& jugador,
    const Camera3D& camara
)
{
    // La linea debug esta debajo del suelo. Proyectamos la posicion del
    // jugador desde la camara hasta ese mismo plano antes de comprobar el
    // radio; asi el borde visible y el punto donde desaparece el suelo
    // coinciden exactamente en pantalla.
    float divisor = -camara.position.y;
    float factorProyeccion = 1.0f;

    if (std::fabs(divisor) > 0.001f)
    {
        factorProyeccion =
            (ALTURA_DEBUG_COLISION_MONTANA_PELOTAS - camara.position.y) /
            divisor;
    }

    float xPlanoColision =
        camara.position.x +
        (jugador.posicion.x - camara.position.x) * factorProyeccion;

    float zPlanoColision =
        camara.position.z +
        (jugador.posicion.z - camara.position.z) * factorProyeccion;

    float distancia = MagnitudHorizontalPelotas(
        xPlanoColision,
        zPlanoColision
    );

    float radioSoporte = RadioSoportePelotas(
        xPlanoColision,
        zPlanoColision
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
        float radioColision = RadioVisualBordePelotas(
            jugador.posicion.x,
            jugador.posicion.z
        );

        float distanciaMinima =
            radioColision + radioPelota * 0.82f;

        float zonaCercanaAlBorde =
            radioColision - radioPelota * 0.45f;

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

    float radioBase = RadioVisualBordePelotas(
        jugador.posicion.x,
        jugador.posicion.z
    );

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


static Vector3 PuntoContornoColisionMontanaPelotas(int indice)
{
    return PuntoCircularPelotas(
        indice,
        RADIO_ARENA_PELOTAS,
        ALTURA_DEBUG_COLISION_MONTANA_PELOTAS
    );
}


static void DibujarMontanaNievePelotas()
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

        // Borde de hielo azul: marca donde termina la zona segura.
        Vector3 bordeIntA = PuntoCircularPelotas(i, RADIO_ARENA_PELOTAS - 0.60f, 0.04f);
        Vector3 bordeIntB = PuntoCircularPelotas(siguiente, RADIO_ARENA_PELOTAS - 0.60f, 0.04f);
        Vector3 bordeExtA = PuntoCircularPelotas(i, RADIO_ARENA_PELOTAS - 0.04f, 0.04f);
        Vector3 bordeExtB = PuntoCircularPelotas(siguiente, RADIO_ARENA_PELOTAS - 0.04f, 0.04f);
        Color hieloBorde = i % 2 == 0
            ? Color{ 120, 178, 214, 255 }
            : Color{ 96, 154, 196, 255 };

        DrawTriangle3D(bordeIntA, bordeIntB, bordeExtB, hieloBorde);
        DrawTriangle3D(bordeIntA, bordeExtB, bordeExtA, hieloBorde);
    }

    // Anillos de referencia en el centro de la cumbre.
    DrawCircle3D({ 0.0f, 0.05f, 0.0f }, 1.2f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(hieloOscuro, 0.75f));
    DrawCircle3D({ 0.0f, 0.05f, 0.0f }, 1.25f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(hieloOscuro, 0.75f));
    DrawCircle3D({ 0.0f, 0.05f, 0.0f }, 3.3f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(hieloClaro, 0.80f));
}


// Anillo de color bajo los pies y flecha sobre la cabeza: el modelo del
// jugador es oscuro y sobre la nieve no se distingue quien es quien.
// MODELO FUTURO: la flecha puede pasar a ser un icono de jugador del GLB.
static void DibujarIndicadorJugadorPelotas(
    const JugadorPrueba& jugador,
    Color color
)
{
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (jugador.cayendo || pies < -0.35f) return;

    Vector3 centro = { jugador.posicion.x, 0.075f, jugador.posicion.z };
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


static void DibujarContornoColisionMontanaPelotas(
    const Camera3D& camara
)
{
    for (int i = 0; i < SEGMENTOS_ARENA_PELOTAS; i++)
    {
        int siguiente = (i + 1) % SEGMENTOS_ARENA_PELOTAS;

        Vector2 inicio = GetWorldToScreen(
            PuntoContornoColisionMontanaPelotas(i),
            camara
        );

        Vector2 fin = GetWorldToScreen(
            PuntoContornoColisionMontanaPelotas(siguiente),
            camara
        );

        DrawLineEx(inicio, fin, 3.0f, RED);
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


// IA de bots: evita el borde, y si esta a salvo embiste a un rival desde
// el lado del centro para empujarlo hacia afuera. Un ruido lento le quita
// perfeccion y evita que se quede atascado.
static InputMinijuegoParticipante CrearEntradaBotPelotas(
    MinijuegoPelotas& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int indice,
    float deltaTime
)
{
    const JugadorPrueba& yo = jugadores[indice];
    EstadoBotPelotas& bot = minijuego.bots[indice];
    float t = minijuego.tiempoJugado;
    float distanciaCentro = MagnitudHorizontalPelotas(yo.posicion.x, yo.posicion.z);

    // Peligro: cerca del borde se vuelve al centro (con anticipacion por inercia).
    float proyectadaX = yo.posicion.x + yo.velocidad.x * 0.35f;
    float proyectadaZ = yo.posicion.z + yo.velocidad.z * 0.35f;
    float distanciaProyectada = MagnitudHorizontalPelotas(proyectadaX, proyectadaZ);

    bool peligro = distanciaCentro > 4.3f || distanciaProyectada > 4.7f;

    if (!peligro)
    {
        bot.esperaBorde = 0.0f;
    }
    else
    {
        if (bot.esperaBorde <= 0.0f)
            bot.retardoBorde = (float)GetRandomValue(200, 400) / 1000.0f;
        bot.esperaBorde += deltaTime;
    }

    if (peligro && bot.esperaBorde >= bot.retardoBorde)
    {
        return CrearEntradaBotHaciaObjetivo1v3(
            yo.posicion,
            { 0.0f, yo.posicion.y, 0.0f },
            0.25f
        );
    }

    int objetivo = -1;
    float mejor = 1000000.0f;

    for (int j = 0; j < MAX_PARTICIPANTES; j++)
    {
        if (
            j == indice ||
            !participantes[j].activo ||
            !minijuego.resultado.participantes[j].participo ||
            minijuego.estadosJugadores[j].eliminado ||
            jugadores[j].cayendo
        )
        {
            continue;
        }

        float dx = jugadores[j].posicion.x - yo.posicion.x;
        float dz = jugadores[j].posicion.z - yo.posicion.z;
        float distancia = dx * dx + dz * dz;

        if (distancia < mejor)
        {
            mejor = distancia;
            objetivo = j;
        }
    }

    if (objetivo < 0)
    {
        return CrearEntradaBotHaciaObjetivo1v3(yo.posicion, { 0.0f, yo.posicion.y, 0.0f }, 0.5f);
    }

    Vector3 rival = jugadores[objetivo].posicion;
    float distanciaRival = MagnitudHorizontalPelotas(rival.x, rival.z);
    float haciaFueraX = distanciaRival > 0.001f ? rival.x / distanciaRival : 1.0f;
    float haciaFueraZ = distanciaRival > 0.001f ? rival.z / distanciaRival : 0.0f;

    // Estoy del lado del centro respecto al rival?
    float lado =
        (yo.posicion.x - rival.x) * haciaFueraX +
        (yo.posicion.z - rival.z) * haciaFueraZ;

    Vector3 meta = rival;

    if (lado > -0.4f)
    {
        // Rodear: ponerse entre el centro y el rival antes de embestir.
        meta.x = rival.x - haciaFueraX * 2.4f;
        meta.z = rival.z - haciaFueraZ * 2.4f;
    }

    // Error lento de punteria y pausas cortas de duda.
    meta.x += std::sin(t * 1.3f + bot.faseX) * bot.amplitudError;
    meta.z += std::cos(t * 1.1f + bot.faseZ) * bot.amplitudError;

    if (std::sin(t * 0.7f + bot.faseDuda) > 0.93f)
    {
        meta = { 0.0f, yo.posicion.y, 0.0f };
    }

    return CrearEntradaBotHaciaObjetivo1v3(yo.posicion, meta, 0.3f);
}


static void FinalizarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[]
)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    int vivos = ContarJugadoresVivosPelotas(minijuego, participantes);
    minijuego.terminoPorTiempo = vivos > 1;

    // Desempate por tiempo agotado: entre los que siguen en pie gana quien
    // esta mas cerca del centro de la montana.
    if (vivos > 1)
    {
        float distancias[MAX_PARTICIPANTES]{};

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            distancias[i] = MagnitudHorizontalPelotas(
                jugadores[i].posicion.x,
                jugadores[i].posicion.z
            );
        }

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                !participantes[i].activo ||
                !minijuego.resultado.participantes[i].participo ||
                minijuego.estadosJugadores[i].eliminado
            )
            {
                continue;
            }

            int posicion = 1;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    j != i &&
                    participantes[j].activo &&
                    minijuego.resultado.participantes[j].participo &&
                    !minijuego.estadosJugadores[j].eliminado &&
                    distancias[j] < distancias[i] - 0.05f
                )
                {
                    posicion++;
                }
            }

            minijuego.estadosJugadores[i].posicionFinal = posicion;
            minijuego.estadosJugadores[i].tiempoSobrevividoMs =
                (int)std::lround(minijuego.tiempoJugado * 1000.0f);
        }
    }

    int primeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            (minijuego.estadosJugadores[i].eliminado
                ? minijuego.estadosJugadores[i].posicionFinal == 1
                : (vivos > 1
                    ? minijuego.estadosJugadores[i].posicionFinal == 1
                    : true))
        )
        {
            primeros++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        primeros == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);

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
            if (vivos <= 1)
            {
                estadoJugador.posicionFinal = 1;
            }

            estadoJugador.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_PELOTAS_TERMINADO;
}


static void SortearBotsPelotas(MinijuegoPelotas& minijuego)
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        EstadoBotPelotas& bot = minijuego.bots[i];
        bot = {};
        bot.faseX = (float)GetRandomValue(0, 6283) / 1000.0f;
        bot.faseZ = (float)GetRandomValue(0, 6283) / 1000.0f;
        bot.faseDuda = (float)GetRandomValue(0, 6283) / 1000.0f;
        bot.amplitudError = (float)GetRandomValue(600, 1400) / 1000.0f;
    }
}


void MinijuegoPelotas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;
    fase = FASE_PELOTAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PELOTAS;
    tiempoRestante = DURACION_PARTIDA_PELOTAS;
    tiempoJugado = 0.0f;
    terminoPorTiempo = false;
    SortearBotsPelotas(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadBloques = 0;

    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        1,
        { 0.0f, -0.5f, 2.25f },
        { 16.0f, 1.0f, 16.0f },
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
    tiempoRestante = DURACION_PARTIDA_PELOTAS;
    tiempoJugado = 0.0f;
    terminoPorTiempo = false;
    SortearBotsPelotas(*this);

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

        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

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

        // Gracia inicial: los bots esperan para que un humano se oriente.
        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot)
        {
            if (tiempoJugado >= GRACIA_INICIAL_PELOTAS + bots[i].faseDuda * 0.08f)
            {
                entrada = CrearEntradaBotPelotas(*this, jugadores, participantes, i, deltaTime);
            }
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        float velocidadAnteriorX = jugador.velocidad.x;
        float velocidadAnteriorZ = jugador.velocidad.z;
        Vector3 posicionAnterior = jugador.posicion;
        Vector3 empujePendiente = jugador.empuje;
        jugador.empuje = {};

        BloquePrueba sueloJugador = bloques[0];
        sueloJugador.activaColision =
            JugadorSobreArenaCircularPelotas(jugador, camara);

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

        bool sobreArenaDespuesDeMover =
            JugadorSobreArenaCircularPelotas(jugador, camara);

        if (!sobreArenaDespuesDeMover)
        {
            if (sueloJugador.activaColision && jugador.enSuelo)
            {
                jugador.velocidad.y -= jugador.gravedad * deltaTime;
                jugador.posicion.y += jugador.velocidad.y * deltaTime;
            }

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
    if (tiempoJugado >= GRACIA_INICIAL_PELOTAS)
    {
        PotenciarEmpujesPelotas(jugadores, participantes, cantidadMaxima);
    }

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

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (eliminadosEsteFrame > 0)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_CAIDA);
    }

    if (vivosDespues <= 1 || tiempoRestante <= 0.0f)
    {
        if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
        FinalizarResultadoPelotas(*this, jugadores, participantes);
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

    DibujarMontanaNievePelotas();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        DibujarIndicadorJugadorPelotas(jugadores[i], participantes[i].color);
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

    if (mostrarDebug)
    {
        DibujarContornoColisionMontanaPelotas(camara);
    }

    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const float e = (float)alto / 720.0f;

    // Barra superior translucida: el texto no compite con las montanas.
    DrawRectangle(0, 0, ancho, (int)(76 * e), Fade(BLACK, 0.55f));
    DrawText("PELOTAS - EMPUJONES", (int)(24 * e), (int)(10 * e), (int)(30 * e), RAYWHITE);
    DrawText(
        "EMPUJA A LOS DEMAS FUERA. MAS VELOCIDAD = MAS EMPUJE. SI SE ACABA EL TIEMPO GANA EL MAS CERCANO AL CENTRO",
        (int)(24 * e),
        (int)(46 * e),
        (int)(18 * e),
        Color{ 200, 224, 240, 255 }
    );

    if (fase == FASE_PELOTAS_JUGANDO)
    {
        const char* reloj = TextFormat("%.0f", tiempoRestante > 0.0f ? tiempoRestante : 0.0f);
        int tamanoReloj = (int)(46 * e);
        DrawText(
            reloj,
            ancho - MeasureText(reloj, tamanoReloj) - (int)(30 * e),
            (int)(14 * e),
            tamanoReloj,
            tiempoRestante <= 10.0f ? Color{ 255, 90, 80, 255 } : RAYWHITE
        );
    }

    // Tarjetas inferiores: color del jugador y barra de velocidad.
    int cantidadTarjetas = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (participantes[i].activo && participantes[i].conectado) cantidadTarjetas++;
    }

    int anchoTarjeta = (int)(210 * e);
    int altoTarjeta = (int)(50 * e);
    int separacion = (int)(14 * e);
    int xTarjeta = ancho / 2 - (cantidadTarjetas * anchoTarjeta + (cantidadTarjetas - 1) * separacion) / 2;
    int yTarjeta = alto - altoTarjeta - (int)(18 * e);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!participantes[i].activo || !participantes[i].conectado) continue;

        bool fuera = estadosJugadores[i].eliminado;
        float velocidad = MagnitudHorizontalPelotas(jugadores[i].velocidad.x, jugadores[i].velocidad.z);
        float proporcion = velocidad / VELOCIDAD_MAXIMA_PELOTAS;
        if (proporcion > 1.0f) proporcion = 1.0f;

        DrawRectangle(xTarjeta, yTarjeta, anchoTarjeta, altoTarjeta, Fade(BLACK, fuera ? 0.40f : 0.62f));
        DrawRectangle(xTarjeta, yTarjeta, (int)(8 * e), altoTarjeta, fuera ? GRAY : participantes[i].color);
        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            xTarjeta + (int)(18 * e),
            yTarjeta + (int)(6 * e),
            (int)(22 * e),
            fuera ? GRAY : participantes[i].color
        );

        if (fuera)
        {
            DrawText("FUERA", xTarjeta + (int)(80 * e), yTarjeta + (int)(10 * e), (int)(20 * e), GRAY);
        }
        else
        {
            int xBarra = xTarjeta + (int)(18 * e);
            int yBarra = yTarjeta + (int)(34 * e);
            int anchoBarra = anchoTarjeta - (int)(32 * e);
            DrawRectangle(xBarra, yBarra, anchoBarra, (int)(8 * e), Fade(WHITE, 0.25f));
            DrawRectangle(xBarra, yBarra, (int)(anchoBarra * proporcion), (int)(8 * e), participantes[i].color);
            DrawText(
                TextFormat("%.1f", velocidad),
                xTarjeta + anchoTarjeta - MeasureText(TextFormat("%.1f", velocidad), (int)(18 * e)) - (int)(12 * e),
                yTarjeta + (int)(8 * e),
                (int)(18 * e),
                RAYWHITE
            );
        }

        xTarjeta += anchoTarjeta + separacion;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        int tamano = (int)(96 * e);
        int x = ancho / 2 - MeasureText(texto, tamano) / 2;
        int y = alto / 2 - (int)(150 * e);
        DrawText(texto, x + 4, y + 4, tamano, Fade(BLACK, 0.7f));
        DrawText(texto, x, y, tamano, ORANGE);
    }
    else if (
        fase == FASE_PELOTAS_JUGANDO &&
        tiempoJugado < DURACION_TEXTO_YA_PELOTAS
    )
    {
        const char* texto = "YA";
        int tamano = (int)(96 * e);
        int x = ancho / 2 - MeasureText(texto, tamano) / 2;
        int y = alto / 2 - (int)(150 * e);
        DrawText(texto, x + 4, y + 4, tamano, Fade(BLACK, 0.7f));
        DrawText(texto, x, y, tamano, LIME);
    }
    else if (fase == FASE_PELOTAS_TERMINADO)
    {
        // Panel compacto en la parte alta: deja ver la cumbre y al ganador.
        int anchoPanel = (int)(330 * e);
        int altoPanel = (int)(232 * e);
        int xPanel = ancho - anchoPanel - (int)(16 * e);
        int yPanel = (int)(92 * e);
        DrawRectangle(xPanel, yPanel, anchoPanel, altoPanel, Fade(BLACK, 0.82f));

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
            xPanel + anchoPanel / 2 - MeasureText(titulo, (int)(24 * e)) / 2,
            yPanel + (int)(14 * e),
            (int)(24 * e),
            GOLD
        );

        const char* motivo = terminoPorTiempo
            ? "TIEMPO AGOTADO: GANA EL MAS CERCANO AL CENTRO"
            : "QUEDA UN SOLO JUGADOR EN PIE";
        DrawText(
            motivo,
            xPanel + anchoPanel / 2 - MeasureText(motivo, (int)(12 * e)) / 2,
            yPanel + (int)(46 * e),
            (int)(12 * e),
            LIGHTGRAY
        );

        int y = yPanel + (int)(72 * e);
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo) continue;

            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    resultado.participantes[i].puntuacionMinijuego / 1000.0f
                ),
                xPanel + (int)(22 * e),
                y,
                (int)(18 * e),
                participantes[i].color
            );
            y += (int)(26 * e);
        }

        DrawText(
            TextoReinicioMinijuego(),
            xPanel + anchoPanel / 2 - MeasureText(TextoReinicioMinijuego(), (int)(15 * e)) / 2,
            yPanel + altoPanel - (int)(26 * e),
            (int)(15 * e),
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoPelotas::ObtenerResultado() const
{
    return resultado;
}
