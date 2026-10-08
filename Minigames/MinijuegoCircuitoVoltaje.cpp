#include "Minigames/MinijuegoCircuitoVoltaje.h"

#include "Minigames/EfectosVisualesMinijuegos.h"

#include "Minigames/AudioMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/Input.h"

#include <cmath>


//==================================================
// CIRCUITO VOLTAJE - PISTA NOCTURNA ELECTRICA 3D
//==================================================
//
// Logica: cada jugador solo tiene "avance" (vueltas recorridas, 0..4),
// velocidad y estado de trompo. La pista 3D es un ovalo en el plano XZ
// y se calcula a partir del avance: la logica no depende de la decoracion.
// Recta = arriba/abajo del ovalo, curva = extremos izquierdo y derecho.
//==================================================


static const float DURACION_PREPARACION_CIRCUITO = 2.5f;
static const float DURACION_CARRERA_CIRCUITO = 45.0f;
static const float VUELTAS_CIRCUITO = 4.0f;

// Geometria del ovalo: el borde k (0 = exterior, 4 = interior) tiene
// semiejes (RADIO_X_EXTERIOR - k * ANCHO_CARRIL, ...). Cada carril es
// la franja entre dos bordes consecutivos.
static const float RADIO_X_EXTERIOR = 18.0f;
static const float RADIO_Z_EXTERIOR = 10.4f;
static const float ANCHO_CARRIL = 1.6f;
static const float PERALTE_MAXIMO = 1.1f;
static const int SEGMENTOS_PISTA = 64;


static float FraccionVueltaCircuito(float avance)
{
    float fraccion = avance - std::floor(avance);
    return fraccion < 0.0f ? fraccion + 1.0f : fraccion;
}


static float IntensidadCurvaCircuito(float avance)
{
    float angulo =
        -PI / 2.0f +
        FraccionVueltaCircuito(avance) * PI * 2.0f;

    float curva = std::fabs(std::cos(angulo));
    return curva * curva;
}


static float VelocidadSeguraCircuito(float avance)
{
    return 1.0f - IntensidadCurvaCircuito(avance) * 0.42f;
}


static int PuntuacionAvanceCircuito(
    const EstadoJugadorCircuitoVoltaje& jugador
)
{
    return (int)(Clamp(
        jugador.avance,
        0.0f,
        VUELTAS_CIRCUITO
    ) * 10000.0f + 0.5f);
}


static void FinalizarCircuitoVoltaje(
    MinijuegoCircuitoVoltaje& minijuego
)
{
    int mejorAvance = -1;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        int avance = PuntuacionAvanceCircuito(minijuego.jugadores[i]);

        if (avance > mejorAvance)
        {
            mejorAvance = avance;
            cantidadGanadores = 1;
        }
        else if (avance == mejorAvance)
        {
            cantidadGanadores++;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int avance = PuntuacionAvanceCircuito(minijuego.jugadores[i]);
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                PuntuacionAvanceCircuito(minijuego.jugadores[j]) > avance
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            avance / 40 - minijuego.jugadores[i].trompos * 10;

        if (resultadoJugador.puntuacionMinijuego < 0)
        {
            resultadoJugador.puntuacionMinijuego = 0;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_CIRCUITO_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


// IA: acelera mientras va por debajo de la velocidad segura de la zona
// (con un margen distinto por bot) y, de vez en cuando, se pasa de
// acelerador a proposito para que no sea perfecta.
static void ActualizarBotCircuito(
    EstadoJugadorCircuitoVoltaje& jugador,
    int indiceParticipante,
    float tiempoAnimacion,
    bool& acelerando
)
{
    (void)indiceParticipante;
    float velocidadSegura = VelocidadSeguraCircuito(jugador.avance);
    float margen = jugador.margenBot;

    acelerando = jugador.velocidad < velocidadSegura - margen;

    float pulsoError = std::sin(
        tiempoAnimacion * 0.72f + jugador.faseErrorBot
    );

    if (pulsoError > 0.965f)
    {
        acelerando = true;
    }
}


void MinijuegoCircuitoVoltaje::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
        jugadores[i].margenBot = 0.03f + 0.06f * (float)GetRandomValue(0, 1000) / 1000.0f;
        jugadores[i].faseErrorBot = 6.2832f * (float)GetRandomValue(0, 1000) / 1000.0f;
    }

    fase = FASE_CIRCUITO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_CIRCUITO;
    tiempoCarrera = DURACION_CARRERA_CIRCUITO;
    tiempoAnimacion = 0.0f;
}


void MinijuegoCircuitoVoltaje::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


void MinijuegoCircuitoVoltaje::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_CIRCUITO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CIRCUITO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CIRCUITO_CARRERA;
        }

        return;
    }

    float carreraAntes = tiempoCarrera;
    tiempoCarrera -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, carreraAntes, tiempoCarrera);
    bool alguienLlego = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            jugadores[i].llegoMeta
        )
        {
            continue;
        }

        EstadoJugadorCircuitoVoltaje& jugador = jugadores[i];

        if (jugador.tiempoTrompo > 0.0f)
        {
            jugador.tiempoTrompo -= deltaTime;
            jugador.velocidad -= 0.55f * deltaTime;

            if (jugador.velocidad < 0.08f)
            {
                jugador.velocidad = 0.08f;
            }

            continue;
        }

        bool acelerando = false;

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            ActualizarBotCircuito(
                jugador,
                i,
                tiempoAnimacion,
                acelerando
            );
        }
        else
        {
            InputMinijuegoParticipante entrada =
                LeerInputMinijuegoParticipante(participantes[i]);

            acelerando = entrada.adelante;
        }

        if (acelerando)
        {
            jugador.velocidad += 0.78f * deltaTime;
        }
        else
        {
            jugador.velocidad -= 0.52f * deltaTime;
        }

        jugador.velocidad = Clamp(jugador.velocidad, 0.0f, 1.08f);

        float velocidadSegura = VelocidadSeguraCircuito(jugador.avance);

        if (jugador.velocidad > velocidadSegura + 0.075f)
        {
            jugador.tiempoExcesoCurva += deltaTime;
        }
        else
        {
            jugador.tiempoExcesoCurva -= deltaTime * 2.2f;

            if (jugador.tiempoExcesoCurva < 0.0f)
            {
                jugador.tiempoExcesoCurva = 0.0f;
            }
        }

        if (jugador.tiempoExcesoCurva >= 0.33f)
        {
            jugador.tiempoTrompo = 1.05f;
            jugador.tiempoExcesoCurva = 0.0f;
            jugador.velocidad = 0.24f;
            jugador.trompos++;
            ReproducirSonidoMinijuego(audio, SONIDO_GOLPE);
            continue;
        }

        float vueltaAntes = std::floor(jugador.avance);
        jugador.avance += jugador.velocidad * 0.175f * deltaTime;

        if (jugador.avance >= VUELTAS_CIRCUITO)
        {
            jugador.avance = VUELTAS_CIRCUITO;
            jugador.llegoMeta = true;
            alguienLlego = true;
        }
        else if (std::floor(jugador.avance) > vueltaAntes)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
        }
    }

    if (alguienLlego || tiempoCarrera <= 0.0f)
    {
        tiempoCarrera = tiempoCarrera < 0.0f ? 0.0f : tiempoCarrera;
        FinalizarCircuitoVoltaje(*this);
    }
}


//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB los autos, la pista con peralte,
// las barreras, el arco de meta, los postes de luz, las gradas y la torre
// Tesla / pilones del centro.
//==================================================


static Camera3D ObtenerCamaraCircuito()
{
    Camera3D camara{};
    camara.position = { 0.0f, 25.0f, 15.5f };
    camara.target = { 0.0f, 0.0f, 0.8f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Punto del borde k (0 exterior ... 4 interior) del ovalo en un angulo.
// La altura simula el peralte: el exterior sube en las curvas.
static Vector3 PuntoBordeCircuito(float k, float angulo)
{
    float coseno = std::cos(angulo);
    float intensidad = coseno * coseno;

    return Vector3
    {
        coseno * (RADIO_X_EXTERIOR - ANCHO_CARRIL * k),
        PERALTE_MAXIMO * intensidad * (1.0f - k / 4.0f),
        std::sin(angulo) * (RADIO_Z_EXTERIOR - ANCHO_CARRIL * k)
    };
}


static float AnguloSegmentoCircuito(int segmento)
{
    return -PI / 2.0f + segmento * (2.0f * PI / SEGMENTOS_PISTA);
}


static void DibujarCuadroCircuito(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
}


// Franja radial fina (marcas de frenado, linea de meta, etc.).
static void DibujarBandaRadialCircuito(float anguloInicio, float anguloFin, Color color)
{
    Vector3 a = PuntoBordeCircuito(0.0f, anguloInicio);
    Vector3 b = PuntoBordeCircuito(4.0f, anguloInicio);
    Vector3 c = PuntoBordeCircuito(4.0f, anguloFin);
    Vector3 d = PuntoBordeCircuito(0.0f, anguloFin);

    a.y += 0.03f;
    b.y += 0.03f;
    c.y += 0.03f;
    d.y += 0.03f;
    DibujarCuadroCircuito(a, b, c, d, color);
}


static void DibujarPistaCircuito(float tiempo, float lideresVuelta)
{
    // Suelo general nocturno.
    DrawCube({ 0.0f, -0.16f, 0.0f }, 90.0f, 0.1f, 60.0f, Color{ 10, 14, 24, 255 });

    // Infield: fan de triangulos desde el centro hasta el borde interior.
    rlDisableBackfaceCulling();

    for (int s = 0; s < SEGMENTOS_PISTA; s++)
    {
        float a0 = AnguloSegmentoCircuito(s);
        float a1 = AnguloSegmentoCircuito(s + 1);
        Vector3 p0 = PuntoBordeCircuito(4.0f, a0);
        Vector3 p1 = PuntoBordeCircuito(4.0f, a1);
        p0.y -= 0.02f;
        p1.y -= 0.02f;
        DrawTriangle3D({ 0.0f, -0.02f, 0.0f }, p0, p1, Color{ 14, 46, 44, 255 });
    }

    // Asfalto: 4 carriles, cada uno con un tono ligeramente distinto.
    for (int s = 0; s < SEGMENTOS_PISTA; s++)
    {
        float a0 = AnguloSegmentoCircuito(s);
        float a1 = AnguloSegmentoCircuito(s + 1);

        for (int carril = 0; carril < MAX_PARTICIPANTES; carril++)
        {
            unsigned char tono = (unsigned char)(48 + carril * 4 + (s % 2) * 3);
            DibujarCuadroCircuito(
                PuntoBordeCircuito((float)carril, a0),
                PuntoBordeCircuito((float)carril + 1.0f, a0),
                PuntoBordeCircuito((float)carril + 1.0f, a1),
                PuntoBordeCircuito((float)carril, a1),
                Color{ tono, (unsigned char)(tono + 2), (unsigned char)(tono + 10), 255 }
            );
        }

        // Bordillo interior rojo/blanco y barrera exterior.
        Color bordillo = (s % 2 == 0) ? Color{ 230, 60, 70, 255 } : RAYWHITE;
        Vector3 i0 = PuntoBordeCircuito(4.0f, a0);
        Vector3 i1 = PuntoBordeCircuito(4.0f, a1);
        DibujarCuadroCircuito(
            { i0.x, i0.y + 0.04f, i0.z },
            { i1.x, i1.y + 0.04f, i1.z },
            { i1.x * 0.97f, i1.y + 0.04f, i1.z * 0.97f },
            { i0.x * 0.97f, i0.y + 0.04f, i0.z * 0.97f },
            bordillo
        );

        Vector3 o0 = PuntoBordeCircuito(0.0f, a0);
        Vector3 o1 = PuntoBordeCircuito(0.0f, a1);
        DibujarCuadroCircuito(
            { o0.x, -0.1f, o0.z },
            { o1.x, -0.1f, o1.z },
            { o1.x, o1.y + 0.8f, o1.z },
            { o0.x, o0.y + 0.8f, o0.z },
            Color{ 30, 34, 52, 255 }
        );
    }

    rlEnableBackfaceCulling();

    // Franja de neon en lo alto de la barrera (alterna cian y magenta).
    for (int s = 0; s < SEGMENTOS_PISTA; s++)
    {
        Vector3 o0 = PuntoBordeCircuito(0.0f, AnguloSegmentoCircuito(s));
        Vector3 o1 = PuntoBordeCircuito(0.0f, AnguloSegmentoCircuito(s + 1));
        Color neon = ((s / 4) % 2 == 0) ? Color{ 60, 220, 255, 255 } : Color{ 255, 70, 200, 255 };
        o0.y += 0.82f;
        o1.y += 0.82f;
        DrawLine3D(o0, o1, neon);
        o0.y += 0.04f;
        o1.y += 0.04f;
        DrawLine3D(o0, o1, neon);
    }

    // Marcas de frenado (rojo = entrada de curva) y de salida (verde).
    rlDisableBackfaceCulling();
    DibujarBandaRadialCircuito(-PI / 4.0f, -PI / 4.0f + 0.06f, Fade(RED, 0.9f));
    DibujarBandaRadialCircuito(3.0f * PI / 4.0f, 3.0f * PI / 4.0f + 0.06f, Fade(RED, 0.9f));
    DibujarBandaRadialCircuito(PI / 4.0f, PI / 4.0f + 0.06f, Fade(LIME, 0.9f));
    DibujarBandaRadialCircuito(5.0f * PI / 4.0f, 5.0f * PI / 4.0f + 0.06f, Fade(LIME, 0.9f));
    rlEnableBackfaceCulling();

    // Linea de meta a cuadros (en la recta superior, angulo -90 grados).
    float zExterior = -(RADIO_Z_EXTERIOR);
    float zInterior = -(RADIO_Z_EXTERIOR - ANCHO_CARRIL * 4.0f);
    float anchoCelda = (zInterior - zExterior) / 8.0f;

    for (int fila = 0; fila < 8; fila++)
    {
        for (int columna = 0; columna < 2; columna++)
        {
            DrawCube(
                { -0.5f + columna * 1.0f, 0.03f, zExterior + anchoCelda * (fila + 0.5f) },
                1.0f,
                0.02f,
                anchoCelda,
                ((fila + columna) % 2 == 0) ? BLACK : RAYWHITE
            );
        }
    }

    // Arco de meta con un piloto luminoso por cada vuelta del lider.
    float zArcoA = zExterior - 0.5f;
    float zArcoB = zInterior + 0.5f;
    DrawCube({ 0.0f, 1.8f, zArcoA }, 0.6f, 3.6f, 0.6f, Color{ 70, 74, 96, 255 });
    DrawCube({ 0.0f, 1.8f, zArcoB }, 0.6f, 3.6f, 0.6f, Color{ 70, 74, 96, 255 });
    DrawCube({ 0.0f, 3.7f, (zArcoA + zArcoB) * 0.5f }, 0.7f, 0.7f, zArcoB - zArcoA + 0.6f, Color{ 30, 32, 44, 255 });

    for (int vuelta = 0; vuelta < (int)VUELTAS_CIRCUITO; vuelta++)
    {
        bool encendida = (float)vuelta < lideresVuelta;
        float zLuz = zArcoA + (zArcoB - zArcoA) * ((vuelta + 0.5f) / VUELTAS_CIRCUITO);
        DrawSphereEx(
            { 0.0f, 3.7f, zLuz },
            0.26f,
            8,
            8,
            encendida ? Color{ 255, 230, 80, 255 } : Color{ 70, 60, 30, 255 }
        );
    }

    // Postes de luz alrededor de la pista exterior.
    for (int poste = 0; poste < 12; poste++)
    {
        float angulo = poste * (2.0f * PI / 12.0f) + 0.26f;
        Vector3 borde = PuntoBordeCircuito(0.0f, angulo);
        Vector3 base = { borde.x * 1.07f, 0.0f, borde.z * 1.1f };
        float parpadeo = 0.85f + 0.15f * std::sin(tiempo * 5.0f + poste * 1.7f);

        DrawCylinder(base, 0.1f, 0.14f, 3.2f, 6, Color{ 60, 64, 80, 255 });
        DrawSphereEx({ base.x, 3.4f, base.z }, 0.3f, 8, 8, Fade(Color{ 255, 240, 170, 255 }, parpadeo));
        DrawCircle3D({ base.x, 0.03f, base.z }, 1.2f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 255, 240, 170, 255 }, 0.15f));
    }

    // Gradas oscuras con luces de colores (publico) fuera de la pista.
    for (int grada = 0; grada < 10; grada++)
    {
        float angulo = grada * (2.0f * PI / 10.0f) + 0.55f;
        Vector3 borde = PuntoBordeCircuito(0.0f, angulo);
        Vector3 pos = { borde.x * 1.2f, 0.0f, borde.z * 1.3f };
        Color luz = (grada % 3 == 0)
            ? Color{ 255, 90, 200, 255 }
            : ((grada % 3 == 1) ? Color{ 90, 220, 255, 255 } : Color{ 255, 220, 90, 255 });

        DrawCube({ pos.x, 0.6f, pos.z }, 3.0f, 1.2f, 1.4f, Color{ 24, 26, 40, 255 });
        DrawCube({ pos.x, 1.4f, pos.z }, 2.4f, 0.5f, 1.0f, Color{ 32, 34, 52, 255 });

        for (int punto = 0; punto < 5; punto++)
        {
            float parpadeo = 0.5f + 0.5f * std::sin(tiempo * 4.0f + grada + punto * 1.3f);
            DrawSphereEx({ pos.x - 1.0f + punto * 0.5f, 1.8f, pos.z }, 0.08f, 4, 4, Fade(luz, parpadeo));
        }
    }

    // Centro del infield: torre Tesla y pilones con arcos de energia.
    float pulso = 0.6f + 0.4f * std::sin(tiempo * 4.0f);
    DrawCylinder({ 0.0f, 0.0f, 0.0f }, 0.9f, 1.3f, 0.8f, 10, Color{ 55, 60, 82, 255 });
    DrawCylinder({ 0.0f, 0.8f, 0.0f }, 0.25f, 0.5f, 3.4f, 8, Color{ 80, 88, 120, 255 });
    DrawSphereEx({ 0.0f, 4.5f, 0.0f }, 0.75f, 12, 12, Fade(Color{ 120, 220, 255, 255 }, 0.55f + 0.3f * pulso));
    DrawSphereEx({ 0.0f, 4.5f, 0.0f }, 0.38f, 8, 8, WHITE);
    DrawCircle3D({ 0.0f, 0.06f, 0.0f }, 1.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 120, 220, 255, 255 }, 0.7f));

    for (int pilon = 0; pilon < 4; pilon++)
    {
        float angulo = pilon * (PI / 2.0f) + PI / 4.0f;
        Vector3 base = { std::cos(angulo) * 5.2f, 0.0f, std::sin(angulo) * 1.1f };
        DrawCylinder(base, 0.15f, 0.3f, 2.0f, 6, Color{ 70, 76, 104, 255 });
        DrawSphereEx({ base.x, 2.1f, base.z }, 0.22f, 8, 8, Fade(Color{ 255, 240, 120, 255 }, pulso));

        // Arco de energia pilon -> torre (zigzag que cambia con el tiempo).
        Vector3 anterior = { base.x, 2.1f, base.z };

        for (int tramo = 1; tramo <= 5; tramo++)
        {
            float f = tramo / 5.0f;
            float ruido = std::sin(tiempo * 18.0f + pilon * 2.0f + tramo * 3.1f) * 0.35f * (1.0f - f);
            Vector3 siguiente =
            {
                base.x * (1.0f - f) + ruido,
                2.1f + (4.5f - 2.1f) * f + ruido,
                base.z * (1.0f - f) - ruido
            };
            DrawLine3D(anterior, siguiente, Fade(Color{ 160, 230, 255, 255 }, 0.4f + 0.5f * pulso));
            anterior = siguiente;
        }
    }
}


// Auto de carreras: eje local +X = frente, +Z = lado interior de la curva.
static void DibujarAuto3D(
    Vector3 posicion,
    float rumboGrados,
    float peralteGrados,
    float desvioLateral,
    Color color,
    bool enTrompo,
    bool advertencia,
    float velocidad,
    float tiempoAnimacion
)
{
    Color cuerpo = color;

    if (advertencia && ((int)(tiempoAnimacion * 12.0f) % 2 == 0))
    {
        cuerpo = ColorLerp(color, RED, 0.7f);
    }

    Color claro = ColorLerp(color, RAYWHITE, 0.4f);

    // Sombra bajo el auto.
    DrawCylinder({ posicion.x, posicion.y + 0.02f, posicion.z }, 1.0f, 1.0f, 0.02f, 12, Fade(BLACK, 0.4f));

    rlPushMatrix();
    rlTranslatef(posicion.x, posicion.y, posicion.z);
    rlRotatef(-rumboGrados, 0.0f, 1.0f, 0.0f);
    rlRotatef(peralteGrados, 1.0f, 0.0f, 0.0f);
    rlTranslatef(0.0f, 0.0f, desvioLateral);

    if (enTrompo)
    {
        rlRotatef(std::fmod(tiempoAnimacion * 720.0f, 360.0f), 0.0f, 1.0f, 0.0f);
    }

    // Brillo electrico bajo el chasis: mas intenso con la velocidad.
    DrawCube(
        { 0.0f, 0.1f, 0.0f },
        1.7f,
        0.04f,
        0.9f,
        Fade(Color{ 90, 230, 255, 255 }, 0.25f + 0.55f * velocidad)
    );

    // Ruedas.
    Color rueda = Color{ 18, 18, 22, 255 };
    DrawCube({ 0.62f, 0.22f, 0.5f }, 0.5f, 0.4f, 0.24f, rueda);
    DrawCube({ 0.62f, 0.22f, -0.5f }, 0.5f, 0.4f, 0.24f, rueda);
    DrawCube({ -0.62f, 0.22f, 0.5f }, 0.5f, 0.4f, 0.24f, rueda);
    DrawCube({ -0.62f, 0.22f, -0.5f }, 0.5f, 0.4f, 0.24f, rueda);

    // Carroceria, nariz, cabina y alerón.
    DrawCube({ 0.0f, 0.38f, 0.0f }, 1.9f, 0.3f, 0.78f, cuerpo);
    DrawCube({ 0.85f, 0.32f, 0.0f }, 0.55f, 0.18f, 0.55f, claro);
    DrawCube({ -0.1f, 0.62f, 0.0f }, 0.7f, 0.26f, 0.5f, Color{ 20, 30, 52, 255 });
    DrawCube({ -0.9f, 0.68f, 0.0f }, 0.12f, 0.07f, 1.0f, claro);
    DrawCube({ -0.9f, 0.55f, 0.35f }, 0.08f, 0.22f, 0.07f, BLACK);
    DrawCube({ -0.9f, 0.55f, -0.35f }, 0.08f, 0.22f, 0.07f, BLACK);
    DrawCubeWires({ 0.0f, 0.38f, 0.0f }, 1.9f, 0.3f, 0.78f, BLACK);

    // Faros.
    DrawSphereEx({ 1.12f, 0.36f, 0.2f }, 0.09f, 6, 6, Color{ 255, 250, 200, 255 });
    DrawSphereEx({ 1.12f, 0.36f, -0.2f }, 0.09f, 6, 6, Color{ 255, 250, 200, 255 });

    rlPopMatrix();

    // Humo y chispas durante el trompo.
    if (enTrompo)
    {
        for (int i = 0; i < 4; i++)
        {
            float fase = std::fmod(tiempoAnimacion * 1.8f + i * 0.25f, 1.0f);
            DrawSphereEx(
                { posicion.x + std::cos(i * 1.6f) * 0.6f, posicion.y + 0.4f + fase * 1.3f, posicion.z + std::sin(i * 1.6f) * 0.6f },
                0.25f + fase * 0.35f,
                6,
                6,
                Fade(LIGHTGRAY, 0.5f * (1.0f - fase))
            );
        }
    }
}


void MinijuegoCircuitoVoltaje::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 6, 8, 20, 255 });

    // Vuelta del lider (para los pilotos del arco de meta).
    float mejorAvance = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo && jugadores[i].avance > mejorAvance)
        {
            mejorAvance = jugadores[i].avance;
        }
    }

    Camera3D camara = ObtenerCamaraCircuito();
    BeginMode3D(camara);

    DibujarPistaCircuito(tiempoAnimacion, std::floor(mejorAvance) + (fase == FASE_CIRCUITO_PREPARACION ? 0.0f : 1.0f));

    Vector2 etiquetas[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        float angulo =
            -PI / 2.0f +
            FraccionVueltaCircuito(jugadores[i].avance) * PI * 2.0f;
        float k = i + 0.5f;
        float radioX = RADIO_X_EXTERIOR - ANCHO_CARRIL * k;
        float radioZ = RADIO_Z_EXTERIOR - ANCHO_CARRIL * k;
        float intensidad = std::cos(angulo) * std::cos(angulo);

        Vector3 posicion = PuntoBordeCircuito(k, angulo);
        float rumbo = std::atan2(radioZ * std::cos(angulo), -radioX * std::sin(angulo)) * RAD2DEG;
        float velocidadSegura = VelocidadSeguraCircuito(jugadores[i].avance);
        bool enTrompo = jugadores[i].tiempoTrompo > 0.0f;
        bool advertencia =
            !enTrompo &&
            jugadores[i].velocidad > velocidadSegura + 0.075f &&
            fase == FASE_CIRCUITO_CARRERA;
        float desvio = 0.0f;

        if (enTrompo)
        {
            float progreso = 1.0f - jugadores[i].tiempoTrompo / 1.05f;
            desvio = -std::sin(Clamp(progreso, 0.0f, 1.0f) * PI) * 0.5f;
        }

        DibujarAuto3D(
            posicion,
            rumbo,
            intensidad * 12.0f,
            desvio,
            participantes[i].color,
            enTrompo,
            advertencia,
            Clamp(jugadores[i].velocidad, 0.0f, 1.0f),
            tiempoAnimacion
        );

        etiquetas[i] = GetWorldToScreen({ posicion.x, posicion.y + 1.6f, posicion.z }, camara);
    }

    EndMode3D();

    // Etiqueta flotante de cada auto.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            (int)etiquetas[i].x - 12,
            (int)etiquetas[i].y - 10,
            18,
            RAYWHITE
        );
    }

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangle(18, 16, 480, 118, Fade(BLACK, 0.78f));
    DrawText("CIRCUITO VOLTAJE", 32, 28, 30, GOLD);

    if (fase == FASE_CIRCUITO_PREPARACION)
    {
        DrawText("PREPARATE", 32, 68, 22, RAYWHITE);
        DrawText(
            TextFormat("EMPIEZA EN %.1f", tiempoPreparacion),
            32,
            100,
            18,
            LIGHTGRAY
        );

        const char* numero = TextFormat("%d", (int)std::ceil(tiempoPreparacion));
        DrawText(numero, ancho / 2 - MeasureText(numero, 120) / 2, alto / 2 - 60, 120, Fade(GOLD, 0.9f));

        const char* controles = "MANTEN ADELANTE = ACELERAR   |   SUELTA EN LAS CURVAS (MARCA ROJA)";
        DrawText(controles, ancho / 2 - MeasureText(controles, 22) / 2, alto - 60, 22, RAYWHITE);
    }
    else if (fase == FASE_CIRCUITO_CARRERA)
    {
        DrawText("MANTEN ADELANTE; SUELTA EN CURVAS", 32, 68, 19, RAYWHITE);
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoCarrera),
            32,
            100,
            18,
            tiempoCarrera <= 5.0f ? ORANGE : LIGHTGRAY
        );
    }
    else
    {
        DrawText("CARRERA TERMINADA", 32, 68, 22, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), 32, 100, 18, LIGHTGRAY);

        int ganador = -1;
        int cantidadPrimeros = 0;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                resultado.participantes[i].participo &&
                resultado.participantes[i].posicionFinal == 1
            )
            {
                ganador = i;
                cantidadPrimeros++;
            }
        }

        const char* titulo = cantidadPrimeros == 1
            ? TextFormat("GANADOR: J%d", participantes[ganador].numeroJugador)
            : "EMPATE";
        Color colorTitulo = cantidadPrimeros == 1 ? participantes[ganador].color : GOLD;

        DrawRectangle(ancho / 2 - 240, alto / 2 - 50, 480, 90, Fade(BLACK, 0.75f));
        DrawText(titulo, ancho / 2 - MeasureText(titulo, 44) / 2, alto / 2 - 30, 44, colorTitulo);
    }

    int panelX = ancho - 250;
    int panelY = 20;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        DrawRectangle(panelX, panelY, 226, 64, Fade(BLACK, 0.76f));
        DrawRectangle(panelX, panelY, 8, 64, participantes[i].color);

        int vuelta = (int)std::floor(jugadores[i].avance) + 1;
        if (vuelta > (int)VUELTAS_CIRCUITO) vuelta = (int)VUELTAS_CIRCUITO;

        int puesto = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                resultado.participantes[j].participo &&
                jugadores[j].avance > jugadores[i].avance
            )
            {
                puesto++;
            }
        }

        DrawText(
            TextFormat("J%d  VUELTA %d/4", participantes[i].numeroJugador, vuelta),
            panelX + 18,
            panelY + 7,
            18,
            participantes[i].color
        );
        DrawText(TextFormat("#%d", puesto), panelX + 190, panelY + 7, 18, RAYWHITE);

        if (fase == FASE_CIRCUITO_TERMINADO)
        {
            DrawText(
                resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal),
                panelX + 18,
                panelY + 32,
                17,
                resultado.participantes[i].posicionFinal == 1
                    ? GOLD
                    : LIGHTGRAY
            );
        }
        else if (jugadores[i].tiempoTrompo > 0.0f)
        {
            DrawText("TROMPO!", panelX + 18, panelY + 32, 17, RED);
        }
        else
        {
            DrawText(
                TextFormat("VELOCIDAD %d%%", (int)(jugadores[i].velocidad * 100.0f)),
                panelX + 18,
                panelY + 32,
                16,
                LIGHTGRAY
            );
        }

        // Barra de velocidad con la marca del limite seguro en esta zona.
        float limite = VelocidadSeguraCircuito(jugadores[i].avance) + 0.075f;
        bool excedido = jugadores[i].velocidad > limite;
        float relleno = Clamp(jugadores[i].velocidad / 1.08f, 0.0f, 1.0f);
        float marca = Clamp(limite / 1.08f, 0.0f, 1.0f);

        DrawRectangle(panelX + 18, panelY + 54, 190, 5, Fade(WHITE, 0.2f));
        DrawRectangle(
            panelX + 18,
            panelY + 54,
            (int)(190.0f * relleno),
            5,
            excedido ? RED : participantes[i].color
        );
        DrawRectangle(panelX + 18 + (int)(190.0f * marca), panelY + 51, 2, 11, GOLD);

        panelY += 72;
    }
}


const ResultadoMinijuego&
MinijuegoCircuitoVoltaje::ObtenerResultado() const
{
    return resultado;
}
