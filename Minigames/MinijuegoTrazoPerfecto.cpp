#include "Minigames/MinijuegoTrazoPerfecto.h"

#include "Minigames/EfectosVisualesMinijuegos.h"

#include "Minigames/AudioMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_TRAZO = 2.5f;
static const float DURACION_TRAZO_PERFECTO = 18.0f;
static const float VELOCIDAD_CURSOR_TRAZO = 0.72f;


static const Vector2 PUNTOS_CRISTAL[] =
{
    { 0.00f, -0.92f },
    { 0.66f, -0.30f },
    { 0.50f,  0.62f },
    { 0.00f,  0.92f },
    {-0.50f,  0.62f },
    {-0.66f, -0.30f }
};


static const Vector2 PUNTOS_RAYO[] =
{
    {-0.18f, -0.92f },
    { 0.56f, -0.92f },
    { 0.16f, -0.16f },
    { 0.65f, -0.16f },
    {-0.44f,  0.92f },
    {-0.15f,  0.18f },
    {-0.64f,  0.18f }
};


static const Vector2 PUNTOS_COHETE[] =
{
    { 0.00f, -0.94f },
    { 0.38f, -0.38f },
    { 0.43f,  0.34f },
    { 0.80f,  0.76f },
    { 0.25f,  0.62f },
    { 0.00f,  0.92f },
    {-0.25f,  0.62f },
    {-0.80f,  0.76f },
    {-0.43f,  0.34f },
    {-0.38f, -0.38f }
};


static const Vector2* ObtenerPuntosTrazo(
    FormaTrazoPerfecto forma,
    int& cantidad
)
{
    switch (forma)
    {
        case FORMA_TRAZO_CRISTAL:
            cantidad = sizeof(PUNTOS_CRISTAL) / sizeof(PUNTOS_CRISTAL[0]);
            return PUNTOS_CRISTAL;

        case FORMA_TRAZO_RAYO:
            cantidad = sizeof(PUNTOS_RAYO) / sizeof(PUNTOS_RAYO[0]);
            return PUNTOS_RAYO;

        case FORMA_TRAZO_COCHETE:
            cantidad = sizeof(PUNTOS_COHETE) / sizeof(PUNTOS_COHETE[0]);
            return PUNTOS_COHETE;

        case CANTIDAD_FORMAS_TRAZO:
            break;
    }

    cantidad = 0;
    return nullptr;
}


static const char* NombreFormaTrazo(FormaTrazoPerfecto forma)
{
    switch (forma)
    {
        case FORMA_TRAZO_CRISTAL: return "CRISTAL";
        case FORMA_TRAZO_RAYO: return "RAYO";
        case FORMA_TRAZO_COCHETE: return "COHETE";
        case CANTIDAD_FORMAS_TRAZO: break;
    }

    return "FIGURA";
}


static Vector2 PuntoRecorridoTrazo(
    FormaTrazoPerfecto forma,
    float progreso
)
{
    int cantidad = 0;
    const Vector2* puntos = ObtenerPuntosTrazo(forma, cantidad);

    if (puntos == nullptr || cantidad <= 0)
    {
        return {};
    }

    float longitudTotal = 0.0f;

    for (int i = 0; i < cantidad; i++)
    {
        longitudTotal += Vector2Distance(
            puntos[i],
            puntos[(i + 1) % cantidad]
        );
    }

    float distanciaObjetivo =
        Clamp(progreso, 0.0f, 1.0f) * longitudTotal;

    for (int i = 0; i < cantidad; i++)
    {
        Vector2 inicio = puntos[i];
        Vector2 fin = puntos[(i + 1) % cantidad];
        float longitudSegmento = Vector2Distance(inicio, fin);

        if (
            distanciaObjetivo <= longitudSegmento ||
            i == cantidad - 1
        )
        {
            float avanceSegmento = longitudSegmento > 0.0f
                ? distanciaObjetivo / longitudSegmento
                : 0.0f;

            return Vector2Lerp(
                inicio,
                fin,
                Clamp(avanceSegmento, 0.0f, 1.0f)
            );
        }

        distanciaObjetivo -= longitudSegmento;
    }

    return puntos[0];
}


static int CalcularPuntuacionTrazo(
    const EstadoJugadorTrazoPerfecto& jugador
)
{
    if (jugador.tiempoEvaluado <= 0.0f)
    {
        return 0;
    }

    return (int)std::round(
        Clamp(
            jugador.precisionAcumulada / jugador.tiempoEvaluado,
            0.0f,
            1.0f
        ) * 100.0f
    );
}


static void FinalizarTrazoPerfecto(
    MinijuegoTrazoPerfecto& minijuego
)
{
    int mejorPuntuacion = -1;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorTrazoPerfecto& jugador = minijuego.jugadores[i];
        jugador.puntuacionFinal = CalcularPuntuacionTrazo(jugador);

        if (jugador.puntuacionFinal > mejorPuntuacion)
        {
            mejorPuntuacion = jugador.puntuacionFinal;
            cantidadGanadores = 1;
        }
        else if (jugador.puntuacionFinal == mejorPuntuacion)
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

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                minijuego.jugadores[j].puntuacionFinal >
                    minijuego.jugadores[i].puntuacionFinal
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            minijuego.jugadores[i].puntuacionFinal;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_TRAZO_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static Vector2 EntradaDireccionTrazo(
    const Participante& participante
)
{
    InputMinijuegoParticipante entrada =
        LeerInputMinijuegoParticipante(participante);

    Vector2 direccion =
    {
        (entrada.derecha ? 1.0f : 0.0f) -
            (entrada.izquierda ? 1.0f : 0.0f),
        (entrada.atras ? 1.0f : 0.0f) -
            (entrada.adelante ? 1.0f : 0.0f)
    };

    if (Vector2LengthSqr(direccion) > 1.0f)
    {
        direccion = Vector2Normalize(direccion);
    }

    return direccion;
}


static void ActualizarCursorBotTrazo(
    EstadoJugadorTrazoPerfecto& jugador,
    Vector2 objetivo,
    float tiempoAnimacion,
    float deltaTime
)
{
    // El bot sigue el trazo con retardo (filtro de primer orden).
    if (!jugador.seguimientoIniciado)
    {
        jugador.objetivoSeguido = objetivo;
        jugador.seguimientoIniciado = true;
    }
    else
    {
        float k = deltaTime / jugador.retardoBot;
        if (k > 1.0f) k = 1.0f;
        jugador.objetivoSeguido = Vector2Add(
            jugador.objetivoSeguido,
            Vector2Scale(Vector2Subtract(objetivo, jugador.objetivoSeguido), k)
        );
    }
    objetivo = jugador.objetivoSeguido;

    Vector2 objetivoImperfecto =
    {
        objetivo.x + std::sin(
            tiempoAnimacion * 1.7f + jugador.desfaseBot
        ) * jugador.errorBot,
        objetivo.y + std::cos(
            tiempoAnimacion * 1.35f + jugador.desfaseBot * 0.7f
        ) * jugador.errorBot
    };

    Vector2 diferencia = Vector2Subtract(objetivoImperfecto, jugador.cursor);
    float distancia = Vector2Length(diferencia);

    if (distancia > 0.001f)
    {
        float paso = VELOCIDAD_CURSOR_TRAZO * deltaTime;

        if (paso >= distancia)
        {
            jugador.cursor = objetivoImperfecto;
        }
        else
        {
            jugador.cursor = Vector2Add(
                jugador.cursor,
                Vector2Scale(Vector2Normalize(diferencia), paso)
            );
        }
    }
}


void MinijuegoTrazoPerfecto::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    fase = FASE_TRAZO_PREPARACION;
    forma = FORMA_TRAZO_CRISTAL;
    tiempoPreparacion = DURACION_PREPARACION_TRAZO;
    tiempoTrazo = DURACION_TRAZO_PERFECTO;
    tiempoAnimacion = 0.0f;
    progresoObjetivo = 0.0f;
}


void MinijuegoTrazoPerfecto::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    forma = (FormaTrazoPerfecto)GetRandomValue(
        0,
        CANTIDAD_FORMAS_TRAZO - 1
    );

    Vector2 inicio = PuntoRecorridoTrazo(forma, 0.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].cursor = inicio;
        jugadores[i].huellas[0] = inicio;
        jugadores[i].cantidadHuellas = 1;
        jugadores[i].desfaseBot = GetRandomValue(0, 628) / 100.0f;
        // Habilidad aleatoria por bot y partida (no depende del indice).
        jugadores[i].errorBot = GetRandomValue(30, 80) / 1000.0f;
        jugadores[i].retardoBot = GetRandomValue(150, 300) / 1000.0f;
        jugadores[i].seguimientoIniciado = false;
    }
}


void MinijuegoTrazoPerfecto::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_TRAZO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_TRAZO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_TRAZO_DIBUJANDO;
        }

        return;
    }

    float trazoAntes = tiempoTrazo;
    tiempoTrazo -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, trazoAntes, tiempoTrazo);
    progresoObjetivo = 1.0f -
        Clamp(tiempoTrazo / DURACION_TRAZO_PERFECTO, 0.0f, 1.0f);

    Vector2 objetivo = PuntoRecorridoTrazo(forma, progresoObjetivo);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorTrazoPerfecto& jugador = jugadores[i];

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            ActualizarCursorBotTrazo(
                jugador,
                objetivo,
                tiempoAnimacion,
                deltaTime
            );
        }
        else
        {
            Vector2 direccion = EntradaDireccionTrazo(participantes[i]);
            jugador.cursor = Vector2Add(
                jugador.cursor,
                Vector2Scale(
                    direccion,
                    VELOCIDAD_CURSOR_TRAZO * deltaTime
                )
            );
        }

        jugador.cursor.x = Clamp(jugador.cursor.x, -1.05f, 1.05f);
        jugador.cursor.y = Clamp(jugador.cursor.y, -1.05f, 1.05f);

        float distancia = Vector2Distance(jugador.cursor, objetivo);
        float precision = 1.0f - Clamp(distancia / 0.42f, 0.0f, 1.0f);
        precision *= precision;

        jugador.precisionAcumulada += precision * deltaTime;
        jugador.tiempoEvaluado += deltaTime;
        jugador.tiempoNuevaHuella -= deltaTime;

        if (
            jugador.tiempoNuevaHuella <= 0.0f &&
            jugador.cantidadHuellas < MAX_HUELLAS_TRAZO_PERFECTO
        )
        {
            jugador.huellas[jugador.cantidadHuellas] = jugador.cursor;
            jugador.cantidadHuellas++;
            jugador.tiempoNuevaHuella = 0.075f;
        }
    }

    if (tiempoTrazo <= 0.0f)
    {
        tiempoTrazo = 0.0f;
        progresoObjetivo = 1.0f;
        FinalizarTrazoPerfecto(*this);
    }
}



//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB las mesas de cartografia estelar,
// el pincel / punta luminosa, las estrellas de la figura, los telescopios
// y columnas de laton del observatorio.
//==================================================


static const float MEDIO_LADO_MESA_TRAZO = 3.5f;
static const float ESCALA_FIGURA_TRAZO = 2.75f;
static const float SEPARACION_MESAS_TRAZO = 7.7f;
// Las filas llevan mas aire para que la etiqueta de la fila inferior no
// roce el borde de las mesas de arriba.
static const float SEPARACION_FILAS_TRAZO = 8.7f;
static const float ALTURA_SUPERFICIE_TRAZO = 0.2f;


static Camera3D ObtenerCamaraTrazo(int cantidadParticipantes)
{
    Camera3D camara{};

    if (cantidadParticipantes <= 2)
    {
        camara.position = { 0.0f, 12.5f, 8.5f };
        camara.target = { 0.0f, 0.0f, -0.6f };
    }
    else
    {
        camara.position = { 0.0f, 19.6f, 13.0f };
        camara.target = { 0.0f, 0.0f, -0.1f };
    }

    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 45.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Centro de la mesa de cada jugador (cuadricula de 1 o 2 columnas).
static Vector3 CentroMesaTrazo(int orden, int cantidadParticipantes)
{
    int columnas = cantidadParticipantes <= 2 ? cantidadParticipantes : 2;
    if (columnas < 1) columnas = 1;
    int filas = (cantidadParticipantes + columnas - 1) / columnas;
    if (filas < 1) filas = 1;

    int columna = orden % columnas;
    int fila = orden / columnas;

    return Vector3
    {
        (columna - (columnas - 1) * 0.5f) * SEPARACION_MESAS_TRAZO,
        0.0f,
        (fila - (filas - 1) * 0.5f) * SEPARACION_FILAS_TRAZO
    };
}


// Punto de la figura (-1..1) llevado a la superficie de una mesa.
static Vector3 PuntoEnMesaTrazo(Vector3 centroMesa, Vector2 punto, float altura)
{
    return Vector3
    {
        centroMesa.x + punto.x * ESCALA_FIGURA_TRAZO,
        altura,
        centroMesa.z + punto.y * ESCALA_FIGURA_TRAZO
    };
}


static void DibujarObservatorioTrazo(float tiempo)
{
    // Suelo del observatorio con aros de astrolabio.
    DrawCylinder({ 0.0f, -0.6f, 0.0f }, 17.0f, 17.0f, 0.3f, 48, Color{ 18, 16, 36, 255 });

    for (int aro = 0; aro < 4; aro++)
    {
        DrawCircle3D(
            { 0.0f, -0.28f, 0.0f },
            6.0f + aro * 3.2f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(Color{ 214, 170, 80, 255 }, 0.28f - aro * 0.04f)
        );
    }

    // Marcas zodiacales y columnas de laton en el perimetro.
    for (int k = 0; k < 12; k++)
    {
        float angulo = k * PI / 6.0f;
        float x = std::cos(angulo) * 16.0f;
        float z = std::sin(angulo) * 16.0f;
        DrawCylinder({ x, -0.3f, z }, 0.35f, 0.45f, 3.0f, 8, Color{ 150, 112, 56, 255 });
        DrawSphereEx({ x, 2.9f, z }, 0.3f, 8, 8, Fade(Color{ 255, 220, 140, 255 }, 0.6f + 0.4f * std::sin(tiempo * 2.0f + k)));
    }

    // Telescopio en un lado (tubo inclinado sobre un soporte).
    DrawCylinder({ -13.5f, -0.3f, -3.0f }, 0.5f, 0.9f, 2.2f, 8, Color{ 120, 90, 50, 255 });
    DrawCylinderEx({ -13.5f, 2.0f, -3.0f }, { -10.5f, 4.2f, -5.0f }, 0.55f, 0.75f, 10, Color{ 70, 80, 120, 255 });
    DrawCylinderEx({ -10.5f, 4.2f, -5.0f }, { -10.1f, 4.45f, -5.2f }, 0.8f, 0.8f, 10, Color{ 200, 170, 90, 255 });

    // Polvo de estrellas flotando sobre las mesas.
    for (int s = 0; s < 36; s++)
    {
        float x = std::fmod(s * 7.13f, 26.0f) - 13.0f;
        float z = std::fmod(s * 4.37f, 18.0f) - 9.0f;
        float y = 1.2f + std::fmod(s * 1.91f, 4.0f) + std::sin(tiempo * 0.8f + s) * 0.25f;
        DrawSphereEx({ x, y, z }, 0.05f + 0.03f * std::sin(tiempo * 3.0f + s * 1.7f), 4, 4, Fade(RAYWHITE, 0.6f));
    }
}


// Estrella de cinco puntas horizontal (visible desde la camara cenital).
static void DibujarEstrellaTrazo(
    Vector3 centro,
    float radioExterior,
    float giro,
    Color color
)
{
    const float radioInterior = radioExterior * 0.46f;
    Vector3 vertices[10];

    for (int v = 0; v < 10; v++)
    {
        float angulo = giro + v * PI / 5.0f;
        float radio = v % 2 == 0 ? radioExterior : radioInterior;
        vertices[v] =
        {
            centro.x + std::cos(angulo) * radio,
            centro.y,
            centro.z + std::sin(angulo) * radio
        };
    }

    // Ambos sentidos de giro para que no dependa del culling.
    for (int v = 0; v < 10; v++)
    {
        Vector3 a = vertices[v];
        Vector3 b = vertices[(v + 1) % 10];
        DrawTriangle3D(centro, a, b, color);
        DrawTriangle3D(centro, b, a, color);
    }
}


static void DibujarMesaTrazo(
    Vector3 centro,
    Color colorJugador,
    const Vector2* puntos,
    int cantidadPuntos,
    float tiempo
)
{
    // Mesa: patas, losa y superficie de carta estelar.
    DrawCube({ centro.x, -0.1f, centro.z }, MEDIO_LADO_MESA_TRAZO * 2.0f + 0.4f, 0.3f, MEDIO_LADO_MESA_TRAZO * 2.0f + 0.4f, Color{ 56, 44, 34, 255 });
    DrawCube({ centro.x, 0.15f, centro.z }, MEDIO_LADO_MESA_TRAZO * 2.0f, 0.1f, MEDIO_LADO_MESA_TRAZO * 2.0f, Color{ 14, 18, 44, 255 });

    // Borde luminoso del color del jugador.
    float lado = MEDIO_LADO_MESA_TRAZO;
    DrawCube({ centro.x, 0.18f, centro.z - lado }, lado * 2.0f, 0.06f, 0.12f, colorJugador);
    DrawCube({ centro.x, 0.18f, centro.z + lado }, lado * 2.0f, 0.06f, 0.12f, colorJugador);
    DrawCube({ centro.x - lado, 0.18f, centro.z }, 0.12f, 0.06f, lado * 2.0f, colorJugador);
    DrawCube({ centro.x + lado, 0.18f, centro.z }, 0.12f, 0.06f, lado * 2.0f, colorJugador);

    // Cuadricula tenue.
    for (int g = -2; g <= 2; g++)
    {
        DrawLine3D({ centro.x + g * 1.2f, 0.21f, centro.z - lado + 0.2f }, { centro.x + g * 1.2f, 0.21f, centro.z + lado - 0.2f }, Fade(SKYBLUE, 0.10f));
        DrawLine3D({ centro.x - lado + 0.2f, 0.21f, centro.z + g * 1.2f }, { centro.x + lado - 0.2f, 0.21f, centro.z + g * 1.2f }, Fade(SKYBLUE, 0.10f));
    }

    // Constelacion objetivo: aristas luminosas y estrellas en los vertices.
    for (int p = 0; p < cantidadPuntos; p++)
    {
        Vector3 inicio = PuntoEnMesaTrazo(centro, puntos[p], ALTURA_SUPERFICIE_TRAZO + 0.04f);
        Vector3 fin = PuntoEnMesaTrazo(centro, puntos[(p + 1) % cantidadPuntos], ALTURA_SUPERFICIE_TRAZO + 0.04f);

        DrawCylinderEx(inicio, fin, 0.07f, 0.07f, 4, Fade(Color{ 150, 215, 255, 255 }, 0.55f));
        DrawSphereEx(inicio, 0.13f + 0.03f * std::sin(tiempo * 3.0f + p), 6, 6, Fade(WHITE, 0.9f));
    }
}


void MinijuegoTrazoPerfecto::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 10, 8, 26, 255 });

    int cantidadPuntos = 0;
    const Vector2* puntos = ObtenerPuntosTrazo(forma, cantidadPuntos);
    Vector2 objetivo = PuntoRecorridoTrazo(forma, progresoObjetivo);
    int cantidad = resultado.cantidadParticipantes > 0 ? resultado.cantidadParticipantes : 1;

    Camera3D camara = ObtenerCamaraTrazo(cantidad);
    Vector2 etiquetaSuperior[MAX_PARTICIPANTES]{};
    Vector2 etiquetaInferior[MAX_PARTICIPANTES]{};

    BeginMode3D(camara);

    DibujarObservatorioTrazo(tiempoAnimacion);

    int orden = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 centro = CentroMesaTrazo(orden, cantidad);
        orden++;

        Color color = participantes[i].color;
        const EstadoJugadorTrazoPerfecto& jugador = jugadores[i];

        DibujarMesaTrazo(centro, color, puntos, cantidadPuntos, tiempoAnimacion);

        // Huellas del trazo: tubo luminoso del color del jugador.
        for (int h = 1; h < jugador.cantidadHuellas; h++)
        {
            DrawCylinderEx(
                PuntoEnMesaTrazo(centro, jugador.huellas[h - 1], ALTURA_SUPERFICIE_TRAZO + 0.08f),
                PuntoEnMesaTrazo(centro, jugador.huellas[h], ALTURA_SUPERFICIE_TRAZO + 0.08f),
                0.07f,
                0.07f,
                3,
                Fade(color, 0.75f)
            );
        }

        // Estrella dorada que hay que seguir: contorno oscuro, cuerpo dorado
        // brillante y nucleo blanco, con un haz de luz hasta la mesa.
        Vector3 estrella = PuntoEnMesaTrazo(centro, objetivo, 0.85f);
        float latido = 1.0f + std::sin(tiempoAnimacion * 6.0f) * 0.08f;
        float giroEstrella = tiempoAnimacion * 1.2f;
        DrawCylinderEx(
            PuntoEnMesaTrazo(centro, objetivo, ALTURA_SUPERFICIE_TRAZO),
            estrella,
            0.04f,
            0.10f,
            4,
            Fade(GOLD, 0.45f)
        );
        DrawCircle3D(
            { estrella.x, ALTURA_SUPERFICIE_TRAZO + 0.03f, estrella.z },
            0.5f * latido,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(GOLD, 0.8f)
        );
        DibujarEstrellaTrazo({ estrella.x, estrella.y, estrella.z }, 0.78f * latido, giroEstrella, Color{ 40, 22, 8, 255 });
        DibujarEstrellaTrazo({ estrella.x, estrella.y + 0.02f, estrella.z }, 0.64f * latido, giroEstrella, Color{ 255, 205, 40, 255 });
        DibujarEstrellaTrazo({ estrella.x, estrella.y + 0.04f, estrella.z }, 0.34f * latido, giroEstrella, Color{ 255, 250, 200, 255 });

        // Pincel: cono con la punta sobre el cursor, mas esfera de tinta.
        Vector3 punta = PuntoEnMesaTrazo(centro, jugador.cursor, ALTURA_SUPERFICIE_TRAZO + 0.1f);
        DrawCylinder(punta, 0.22f, 0.0f, 0.8f, 8, color);
        DrawCylinder({ punta.x, punta.y + 0.8f, punta.z }, 0.12f, 0.22f, 0.9f, 8, ColorLerp(color, RAYWHITE, 0.5f));
        DrawSphereEx(punta, 0.14f, 6, 6, WHITE);
        DrawCircle3D({ punta.x, ALTURA_SUPERFICIE_TRAZO + 0.02f, punta.z }, 0.3f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.8f));

        // Linea de error entre pincel y estrella: verde si va bien, roja si no.
        float distancia = Vector2Distance(jugador.cursor, objetivo);
        float calidad = 1.0f - Clamp(distancia / 0.42f, 0.0f, 1.0f);

        if (fase == FASE_TRAZO_DIBUJANDO)
        {
            DrawLine3D(
                { punta.x, punta.y + 0.05f, punta.z },
                { estrella.x, estrella.y, estrella.z },
                ColorLerp(Color{ 255, 70, 60, 255 }, LIME, calidad)
            );
        }

        etiquetaSuperior[i] = GetWorldToScreen(
            { centro.x - MEDIO_LADO_MESA_TRAZO, 0.3f, centro.z - MEDIO_LADO_MESA_TRAZO },
            camara
        );
        etiquetaInferior[i] = GetWorldToScreen(
            { centro.x - MEDIO_LADO_MESA_TRAZO, 0.3f, centro.z + MEDIO_LADO_MESA_TRAZO },
            camara
        );
    }

    EndMode3D();

    int ancho = GetScreenWidth();

    DrawText("TRAZO PERFECTO", 24, 20, 30, GOLD);
    DrawText(
        TextFormat("FIGURA: %s", NombreFormaTrazo(forma)),
        24,
        58,
        19,
        LIGHTGRAY
    );

    if (fase == FASE_TRAZO_PREPARACION)
    {
        DrawText(
            TextFormat("PREPARATE: %.1f", tiempoPreparacion),
            ancho - 260,
            26,
            22,
            RAYWHITE
        );

        const char* numero = TextFormat("%d", (int)std::ceil(tiempoPreparacion));
        DrawText(numero, ancho / 2 - MeasureText(numero, 120) / 2, GetScreenHeight() / 2 - 60, 120, Fade(GOLD, 0.9f));
    }
    else if (fase == FASE_TRAZO_DIBUJANDO)
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoTrazo),
            ancho - 218,
            26,
            22,
            tiempoTrazo <= 5.0f ? ORANGE : RAYWHITE
        );

        const char* ayuda = "MUEVE TU PINCEL (WASD / FLECHAS / STICK) Y SIGUE LA ESTRELLA DORADA";
        DrawText(ayuda, ancho - MeasureText(ayuda, 17) - 24, 60, 17, GOLD);
    }
    else
    {
        DrawText("RESULTADOS", ancho - 190, 26, 22, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), ancho - 190, 60, 17, LIGHTGRAY);
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        int puntuacion = fase == FASE_TRAZO_TERMINADO
            ? jugadores[i].puntuacionFinal
            : CalcularPuntuacionTrazo(jugadores[i]);

        DrawText(
            TextFormat("J%d  PRECISION %d%%", participantes[i].numeroJugador, puntuacion),
            (int)etiquetaSuperior[i].x,
            (int)etiquetaSuperior[i].y - 26,
            20,
            participantes[i].color
        );

        if (fase == FASE_TRAZO_TERMINADO)
        {
            bool ganador = resultado.participantes[i].posicionFinal == 1;
            DrawText(
                ganador
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal),
                (int)etiquetaInferior[i].x,
                (int)etiquetaInferior[i].y + 4,
                22,
                ganador ? GOLD : LIGHTGRAY
            );
        }
    }
}


const ResultadoMinijuego&
MinijuegoTrazoPerfecto::ObtenerResultado() const
{
    return resultado;
}
