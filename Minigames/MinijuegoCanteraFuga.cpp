#include "Minigames/MinijuegoCanteraFuga.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>


static const float DURACION_PREPARACION_CANTERA = 2.5f;
static const float DURACION_PARTIDA_CANTERA = 30.0f;
static const float LIMITE_X_CANTERA = 4.25f;
static const float Z_CIMA_CANTERA = -9.25f;
static const float Z_BASE_CANTERA = 9.25f;
static const float VELOCIDAD_ESCALADOR_CANTERA = 4.25f;
static const float DURACION_ATURDIDO_CANTERA = 1.05f;
static const float RECARGA_ROCA_CANTERA = 0.82f;


static float LimitarCantera(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AlturaPendienteCantera(float z)
{
    float progreso = (Z_BASE_CANTERA - z) / (Z_BASE_CANTERA - Z_CIMA_CANTERA);
    progreso = LimitarCantera(progreso, 0.0f, 1.0f);
    return progreso * 5.0f;
}


static float CalcularProgresoCantera(float z)
{
    return LimitarCantera(
        (Z_BASE_CANTERA - z) / (Z_BASE_CANTERA - Z_CIMA_CANTERA),
        0.0f,
        1.0f
    );
}


static int BuscarEscaladorMasAdelantado(
    const MinijuegoCanteraFuga& minijuego,
    const JugadorPrueba jugadores[]
)
{
    int mejor = -1;
    float mejorZ = 100000.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo
        )
        {
            continue;
        }

        if (jugadores[i].posicion.z < mejorZ)
        {
            mejorZ = jugadores[i].posicion.z;
            mejor = i;
        }
    }

    return mejor;
}


static void FinalizarCantera(
    MinijuegoCanteraFuga& minijuego,
    bool ganaSolo
)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;

        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego = esSolo
            ? minijuego.impactosSolo * 100
            : (int)std::lround(
                minijuego.estadosJugadores[i].progresoMaximo * 1000.0f
            );
    }

    minijuego.fase = FASE_CANTERA_TERMINADO;
}


static void LanzarRocaCantera(MinijuegoCanteraFuga& minijuego)
{
    if (minijuego.cooldownRoca > 0.0f)
    {
        return;
    }

    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        RocaCanteraFuga& roca = minijuego.rocas[i];

        if (roca.activa)
        {
            continue;
        }

        roca.activa = true;
        roca.radio = GetRandomValue(64, 82) / 100.0f;
        roca.posicion =
        {
            minijuego.posicionTolvaX,
            AlturaPendienteCantera(Z_CIMA_CANTERA) + roca.radio,
            Z_CIMA_CANTERA + 0.35f
        };
        roca.velocidadX = GetRandomValue(-65, 65) / 100.0f;
        roca.velocidadZ = GetRandomValue(61, 73) / 10.0f;
        minijuego.cooldownRoca = RECARGA_ROCA_CANTERA;
        return;
    }
}


static InputMinijuegoParticipante CrearEntradaBotEscalador(
    const MinijuegoCanteraFuga& minijuego,
    int indiceJugador,
    const JugadorPrueba& jugador
)
{
    InputMinijuegoParticipante entrada{};
    entrada.adelante = true;

    float objetivoX = (indiceJugador - 1.5f) * 1.25f;
    float peligroMasCercano = 100000.0f;
    const RocaCanteraFuga* rocaPeligrosa = nullptr;

    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        const RocaCanteraFuga& roca = minijuego.rocas[i];

        if (!roca.activa)
        {
            continue;
        }

        float dz = jugador.posicion.z - roca.posicion.z;
        float dx = std::fabs(jugador.posicion.x - roca.posicion.x);

        if (dz > -0.8f && dz < 4.5f && dx < 1.55f && dz < peligroMasCercano)
        {
            peligroMasCercano = dz;
            rocaPeligrosa = &roca;
        }
    }

    if (rocaPeligrosa != nullptr)
    {
        objetivoX = jugador.posicion.x <= rocaPeligrosa->posicion.x
            ? -LIMITE_X_CANTERA + 0.35f
            : LIMITE_X_CANTERA - 0.35f;
    }

    if (jugador.posicion.x < objetivoX - 0.18f)
        entrada.derecha = true;
    else if (jugador.posicion.x > objetivoX + 0.18f)
        entrada.izquierda = true;

    return entrada;
}


static void ActualizarControlSoloCantera(
    MinijuegoCanteraFuga& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    Participante participantes[]
)
{
    bool lanzar = false;
    Participante& participanteSolo = participantes[minijuego.indiceSolo];

    if (participanteSolo.esBot || !participanteSolo.conectado)
    {
        minijuego.tiempoDecisionBot -= deltaTime;

        int objetivo = BuscarEscaladorMasAdelantado(minijuego, jugadores);

        if (objetivo >= 0)
        {
            float xObjetivo = jugadores[objetivo].posicion.x;
            float paso = 5.2f * deltaTime;

            if (minijuego.posicionTolvaX < xObjetivo - paso)
                minijuego.posicionTolvaX += paso;
            else if (minijuego.posicionTolvaX > xObjetivo + paso)
                minijuego.posicionTolvaX -= paso;
            else
                minijuego.posicionTolvaX = xObjetivo;
        }

        if (minijuego.tiempoDecisionBot <= 0.0f)
        {
            lanzar = true;
            minijuego.tiempoDecisionBot = GetRandomValue(72, 126) / 100.0f;
        }
    }
    else
    {
        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participanteSolo);

        float direccion =
            (entrada.derecha ? 1.0f : 0.0f) -
            (entrada.izquierda ? 1.0f : 0.0f);

        minijuego.posicionTolvaX += direccion * 5.2f * deltaTime;
        lanzar = entrada.golpear;
    }

    minijuego.posicionTolvaX = LimitarCantera(
        minijuego.posicionTolvaX,
        -LIMITE_X_CANTERA + 0.42f,
        LIMITE_X_CANTERA - 0.42f
    );

    JugadorPrueba& jugadorSolo = jugadores[minijuego.indiceSolo];
    jugadorSolo.posicion =
    {
        minijuego.posicionTolvaX,
        AlturaPendienteCantera(Z_CIMA_CANTERA) + jugadorSolo.tamano.y * 0.5f,
        Z_CIMA_CANTERA - 1.45f
    };
    jugadorSolo.direccionMirada = { 0.0f, 0.0f, 1.0f };
    jugadorSolo.enSuelo = true;
    jugadorSolo.cayendo = false;

    if (lanzar)
    {
        LanzarRocaCantera(minijuego);
    }
}


static void ActualizarRocasCantera(
    MinijuegoCanteraFuga& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[]
)
{
    for (int rocaIndice = 0; rocaIndice < MAX_ROCAS_CANTERA_FUGA; rocaIndice++)
    {
        RocaCanteraFuga& roca = minijuego.rocas[rocaIndice];

        if (!roca.activa)
        {
            continue;
        }

        roca.velocidadZ += 1.85f * deltaTime;
        roca.posicion.x += roca.velocidadX * deltaTime;
        roca.posicion.z += roca.velocidadZ * deltaTime;

        float limiteRoca = LIMITE_X_CANTERA - roca.radio;

        if (roca.posicion.x < -limiteRoca)
        {
            roca.posicion.x = -limiteRoca;
            roca.velocidadX = std::fabs(roca.velocidadX) + 0.35f;
        }
        else if (roca.posicion.x > limiteRoca)
        {
            roca.posicion.x = limiteRoca;
            roca.velocidadX = -std::fabs(roca.velocidadX) - 0.35f;
        }

        roca.posicion.y = AlturaPendienteCantera(roca.posicion.z) + roca.radio;

        if (roca.posicion.z > Z_BASE_CANTERA + 1.8f)
        {
            roca.activa = false;
            continue;
        }

        for (int jugadorIndice = 0; jugadorIndice < MAX_PARTICIPANTES; jugadorIndice++)
        {
            if (
                jugadorIndice == minijuego.indiceSolo ||
                !minijuego.resultado.participantes[jugadorIndice].participo ||
                minijuego.estadosJugadores[jugadorIndice].tiempoAturdido > 0.0f
            )
            {
                continue;
            }

            JugadorPrueba& jugador = jugadores[jugadorIndice];
            float dx = jugador.posicion.x - roca.posicion.x;
            float dz = jugador.posicion.z - roca.posicion.z;
            float radioChoque = roca.radio + jugador.tamano.x * 0.46f;

            if (dx * dx + dz * dz > radioChoque * radioChoque)
            {
                continue;
            }

            EstadoJugadorCanteraFuga& estado =
                minijuego.estadosJugadores[jugadorIndice];
            estado.tiempoAturdido = DURACION_ATURDIDO_CANTERA;
            estado.impulsoLateral = dx >= 0.0f ? 2.1f : -2.1f;
            estado.impactosRecibidos++;
            minijuego.impactosSolo++;

            jugador.aplastado = true;
            jugador.tiempoAplastado = DURACION_ATURDIDO_CANTERA;
            roca.velocidadX -= estado.impulsoLateral * 0.34f;
        }
    }
}


void MinijuegoCanteraFuga::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        rocas[i] = {};
    }

    camara.position = { 11.8f, 12.7f, 16.8f };
    camara.target = { 0.0f, 2.15f, -0.8f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 49.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CANTERA_PREPARACION;
    indiceSolo = -1;
    impactosSolo = 0;
    posicionTolvaX = 0.0f;
    cooldownRoca = 0.0f;
    tiempoDecisionBot = 0.9f;
    tiempoPreparacion = DURACION_PREPARACION_CANTERA;
    tiempoRestante = DURACION_PARTIDA_CANTERA;
    tiempoAnimacion = 0.0f;
}


void MinijuegoCanteraFuga::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
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
        fase = FASE_CANTERA_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    const float spawnsX[MAX_PARTICIPANTES] =
    {
        -2.8f, -0.95f, 0.95f, 2.8f
    };

    int cursorEquipo = 0;
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool esSolo = i == indiceSolo;
        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;

        Vector3 spawn = esSolo
            ? Vector3{ 0.0f, AlturaPendienteCantera(Z_CIMA_CANTERA) + 0.70f, Z_CIMA_CANTERA - 1.45f }
            : Vector3{
                spawnsX[cursorEquipo % MAX_PARTICIPANTES],
                AlturaPendienteCantera(Z_BASE_CANTERA - 0.45f) + 0.70f,
                Z_BASE_CANTERA - 0.45f
            };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = esSolo
            ? Vector3{ 0.0f, 0.0f, 1.0f }
            : Vector3{ 0.0f, 0.0f, -1.0f };

        if (!esSolo)
        {
            cursorEquipo++;
        }
    }
}


void MinijuegoCanteraFuga::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_CANTERA_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_CANTERA_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CANTERA_JUGANDO;
        }

        return;
    }

    tiempoRestante -= deltaTime;
    cooldownRoca -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
    if (cooldownRoca < 0.0f) cooldownRoca = 0.0f;

    ActualizarControlSoloCantera(*this, deltaTime, jugadores, participantes);

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (i == indiceSolo || !resultado.participantes[i].participo)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        EstadoJugadorCanteraFuga& estado = estadosJugadores[i];

        if (estado.tiempoAturdido > 0.0f)
        {
            estado.tiempoAturdido -= deltaTime;
            jugador.posicion.z += 3.7f * deltaTime;
            jugador.posicion.x += estado.impulsoLateral * deltaTime;

            if (estado.tiempoAturdido <= 0.0f)
            {
                estado.tiempoAturdido = 0.0f;
                jugador.aplastado = false;
                jugador.tiempoAplastado = 0.0f;
            }
        }
        else
        {
            InputMinijuegoParticipante entrada{};

            if (participantes[i].esBot || !participantes[i].conectado)
            {
                entrada = CrearEntradaBotEscalador(*this, i, jugador);
            }
            else
            {
                entrada = LeerInputMinijuegoParticipante(participantes[i]);
            }

            float moverX =
                (entrada.derecha ? 1.0f : 0.0f) -
                (entrada.izquierda ? 1.0f : 0.0f);
            float moverZ =
                (entrada.atras ? 1.0f : 0.0f) -
                (entrada.adelante ? 1.0f : 0.0f);
            float longitud = std::sqrt(moverX * moverX + moverZ * moverZ);

            if (longitud > 0.01f)
            {
                moverX /= longitud;
                moverZ /= longitud;
                jugador.posicion.x += moverX * VELOCIDAD_ESCALADOR_CANTERA * deltaTime;
                jugador.posicion.z += moverZ * VELOCIDAD_ESCALADOR_CANTERA * deltaTime;
                jugador.direccionMirada = { moverX, 0.0f, moverZ };
            }
        }

        jugador.posicion.x = LimitarCantera(
            jugador.posicion.x,
            -LIMITE_X_CANTERA,
            LIMITE_X_CANTERA
        );
        jugador.posicion.z = LimitarCantera(
            jugador.posicion.z,
            Z_CIMA_CANTERA,
            Z_BASE_CANTERA
        );
        jugador.posicion.y = AlturaPendienteCantera(jugador.posicion.z) + 0.70f;
        jugador.enSuelo = true;
        jugador.cayendo = false;

        float progreso = CalcularProgresoCantera(jugador.posicion.z);
        if (progreso > estado.progresoMaximo)
        {
            estado.progresoMaximo = progreso;
        }

        if (jugador.posicion.z <= Z_CIMA_CANTERA + 0.08f)
        {
            FinalizarCantera(*this, false);
            return;
        }
    }

    ActualizarRocasCantera(*this, deltaTime, jugadores);

    if (tiempoRestante <= 0.0f)
    {
        FinalizarCantera(*this, true);
    }
}


static void DibujarPendienteCantera()
{
    // El dibujo y la colision comparten extremos y funcion de altura.
    Vector3 abajoIzquierda = { -4.75f, AlturaPendienteCantera(Z_BASE_CANTERA), Z_BASE_CANTERA };
    Vector3 abajoDerecha = { 4.75f, AlturaPendienteCantera(Z_BASE_CANTERA), Z_BASE_CANTERA };
    Vector3 arribaIzquierda = { -4.75f, AlturaPendienteCantera(Z_CIMA_CANTERA), Z_CIMA_CANTERA };
    Vector3 arribaDerecha = { 4.75f, AlturaPendienteCantera(Z_CIMA_CANTERA), Z_CIMA_CANTERA };

    Color suelo = Color{ 159, 113, 71, 255 };
    DrawTriangle3D(abajoIzquierda, arribaDerecha, arribaIzquierda, suelo);
    DrawTriangle3D(abajoIzquierda, abajoDerecha, arribaDerecha, suelo);
    DrawCube({ 0.0f, -0.2f, Z_BASE_CANTERA + 0.275f }, 9.5f, 0.4f, 0.55f, suelo);

    for (int linea = 0; linea <= 10; linea++)
    {
        float t = linea / 10.0f;
        float z = Z_BASE_CANTERA + (Z_CIMA_CANTERA - Z_BASE_CANTERA) * t;
        float y = AlturaPendienteCantera(z) + 0.018f;

        DrawLine3D(
            { -4.72f, y, z },
            { 4.72f, y, z },
            Fade(Color{ 78, 55, 42, 255 }, 0.58f)
        );
    }

    DrawCylinderEx(
        { -5.05f, 0.15f, Z_BASE_CANTERA },
        { -5.05f, 5.15f, Z_CIMA_CANTERA },
        0.18f,
        0.18f,
        10,
        Color{ 74, 69, 66, 255 }
    );
    DrawCylinderEx(
        { 5.05f, 0.15f, Z_BASE_CANTERA },
        { 5.05f, 5.15f, Z_CIMA_CANTERA },
        0.18f,
        0.18f,
        10,
        Color{ 74, 69, 66, 255 }
    );
}


static void DibujarTolvaCantera(float x, Color colorSolo)
{
    DrawCube({ 0.0f, AlturaPendienteCantera(Z_CIMA_CANTERA) - 0.225f, Z_CIMA_CANTERA - 1.2f }, 10.4f, 0.45f, 2.4f, Color{ 83, 77, 72, 255 });
    DrawCube({ x, 5.55f, -9.85f }, 1.75f, 1.35f, 1.30f, Color{ 67, 71, 78, 255 });
    DrawCube({ x, 5.62f, -9.14f }, 1.10f, 0.74f, 0.18f, colorSolo);
    DrawCylinderEx(
        { x, 5.12f, -9.25f },
        { x, 5.12f, -8.72f },
        0.44f,
        0.63f,
        12,
        Color{ 112, 116, 124, 255 }
    );
}


void MinijuegoCanteraFuga::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 188, 142, 90, 255 });
    BeginMode3D(camara);

    DrawPlane(
        { 0.0f, -0.22f, 0.0f },
        { 85.0f, 85.0f },
        Color{ 122, 91, 63, 255 }
    );

    DibujarPendienteCantera();

    Color colorSolo = indiceSolo >= 0
        ? participantes[indiceSolo].color
        : RED;
    DibujarTolvaCantera(posicionTolvaX, colorSolo);

    for (int i = 0; i < 18; i++)
    {
        float x = -13.0f + (float)((i * 19) % 26);
        float z = -13.0f + (float)((i * 11) % 27);
        float radio = 0.32f + 0.12f * (float)(i % 4);
        DrawSphere(
            { x, radio - 0.05f, z },
            radio,
            i % 2 == 0
                ? Color{ 91, 75, 64, 255 }
                : Color{ 109, 86, 68, 255 }
        );
    }

    for (int i = 0; i < MAX_ROCAS_CANTERA_FUGA; i++)
    {
        if (!rocas[i].activa) continue;

        DrawSphere(rocas[i].posicion, rocas[i].radio, Color{ 77, 69, 65, 255 });
        DrawSphereWires(rocas[i].posicion, rocas[i].radio, 8, 8, Color{ 145, 127, 111, 255 });

        if (mostrarDebug)
        {
            DrawSphereWires(rocas[i].posicion, rocas[i].radio + 0.42f, 8, 8, RED);
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    EndMode3D();

    DrawRectangle(18, 16, 720, 112, Fade(BLACK, 0.79f));
    DrawText("CANTERA EN FUGA - 1 VS 3", 32, 28, 30, GOLD);

    if (indiceSolo >= 0)
    {
        DrawText(
            TextFormat(
                "J%d CONTROLA LA TOLVA%s",
                participantes[indiceSolo].numeroJugador,
                participantes[indiceSolo].esBot ? " (BOT)" : ""
            ),
            32,
            68,
            19,
            participantes[indiceSolo].color
        );
    }

    DrawText(
        "SOLO: IZQ/DER + E / SHIFT DER / B  |  EQUIPO: SUBE Y ESQUIVA",
        32,
        96,
        17,
        RAYWHITE
    );

    if (fase == FASE_CANTERA_JUGANDO)
    {
        DrawText(
            TextFormat("TIEMPO %.1f", tiempoRestante),
            GetScreenWidth() - 190,
            28,
            23,
            tiempoRestante <= 5.0f ? RED : GOLD
        );
    }

    if (fase == FASE_CANTERA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 88) / 2,
            GetScreenHeight() / 2 - 54,
            88,
            GOLD
        );
    }
    else if (fase == FASE_CANTERA_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        bool ganaSolo =
            resultado.participantes[indiceSolo].posicionFinal == 1;
        const char* titulo = ganaSolo
            ? "GANA EL OPERADOR"
            : "GANAN LOS ESCALADORES";

        DrawRectangle(
            GetScreenWidth() / 2 - 290,
            GetScreenHeight() / 2 - 92,
            580,
            184,
            Fade(BLACK, 0.91f)
        );
        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 51,
            34,
            GOLD
        );
        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 21) / 2,
            GetScreenHeight() / 2 + 25,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoCanteraFuga::ObtenerResultado() const
{
    return resultado;
}
