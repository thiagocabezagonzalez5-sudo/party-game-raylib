#include "Minigames/MinijuegoTrazoPerfecto.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


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


// Efectos solo visuales / de audio (definidos con el dibujo, al final).
static void ActualizarVisualTrazo(
    MinijuegoTrazoPerfecto& minijuego,
    const Participante participantes[],
    float deltaTime
);


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

    semillaVisual = 2654435761u;
    progresoVisto = 0.0f;
    finVisto = false;
    siguienteMancha = 0;

    for (int i = 0; i < MAX_PARTICULAS_TRAZO; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_MANCHAS_TRAZO; i++)
    {
        manchas[i] = {};
    }
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
        ActualizarVisualTrazo(*this, participantes, deltaTime);
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

        ActualizarVisualTrazo(*this, participantes, deltaTime);
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

    ActualizarVisualTrazo(*this, participantes, deltaTime);
}


//==================================================
// VISUAL: ATELIER DE PINTOR AL ATARDECER (sin efecto en la logica)
//==================================================
//
// Un taller en lo alto de una ciudad vieja. Los lienzos de los jugadores
// van en caballetes, en fila delante de un gran ventanal de hierro por el
// que entra el sol del atardecer; detras, los techos, una cupula y las
// torres se funden en la calima rosada. Cada pintor (el jugador) esta de
// espaldas frente a su caballete. La figura objetivo es un boceto a
// carbonilla sobre el lienzo crema; el trazo del jugador, pintura espesa
// de su color con contorno oscuro; la estrella dorada es el acento.
//
// La figura (-1..1) se lleva al plano del lienzo: x a la derecha, y hacia
// abajo (adelante = arriba del lienzo), igual que en la mesa vieja.
//
// MODELO FUTURO: caballete con lienzo y bastidor, pincel, frascos y latas
// de pintura, paleta, busto de yeso, taburete, carrito de pintor, lampara
// colgante esmaltada, maceta con planta, lienzos apilados.
//==================================================

static const float MEDIO_LIENZO_TRAZO = 1.6f;
static const float ESCALA_FIGURA_TRAZO = 1.36f;
static const float INCLINACION_LIENZO_TRAZO = 0.21f;   // radianes hacia atras
static const float Y_BASE_LIENZO_TRAZO = 2.35f;        // borde inferior del lienzo
static const float Z_PINTOR_TRAZO = 1.55f;
static const float Z_MURO_TRAZO = -4.6f;               // cara interior del muro del ventanal
static const float Z_FRENTE_TRAZO = 10.0f;
static const float Y_ALFEIZAR_TRAZO = 0.9f;
static const float Y_DINTEL_TRAZO = 8.8f;
static const float ALTO_TALLER_TRAZO = 10.6f;
static const float DURACION_TEXTO_TRAZO = 0.95f;

enum TextoFlotanteTrazo
{
    TEXTO_TRAZO_NINGUNO = 0,
    TEXTO_TRAZO_BIEN,
    TEXTO_TRAZO_JUSTO,
    TEXTO_TRAZO_UY
};


// Fila de caballetes segun la cantidad de jugadores.
struct DisposicionTrazo
{
    int cantidad = 1;
    float separacion = 5.0f;
    float mitadFila = MEDIO_LIENZO_TRAZO;   // medio ancho de la fila de lienzos
    float muroX = 6.0f;                     // cara interior de los muros laterales
    float ventanaX = 3.0f;                  // medio ancho del ventanal
};


static DisposicionTrazo CalcularDisposicionTrazo(int cantidad)
{
    DisposicionTrazo d;
    d.cantidad = cantidad < 1 ? 1 : cantidad;
    d.separacion = d.cantidad <= 2 ? 5.0f : 3.55f;
    d.mitadFila = (float)(d.cantidad - 1) * 0.5f * d.separacion + MEDIO_LIENZO_TRAZO;
    d.muroX = d.mitadFila + 4.8f;
    d.ventanaX = d.mitadFila + 1.1f;
    return d;
}


static int CantidadParticipantesTrazo(const MinijuegoTrazoPerfecto& m)
{
    return m.resultado.cantidadParticipantes > 0 ? m.resultado.cantidadParticipantes : 1;
}


// Orden en la fila (0..n-1) del participante i.
static int OrdenParticipanteTrazo(const MinijuegoTrazoPerfecto& m, int indice)
{
    int orden = 0;

    for (int i = 0; i < indice && i < MAX_PARTICIPANTES; i++)
    {
        if (m.resultado.participantes[i].participo)
        {
            orden++;
        }
    }

    return orden;
}


// Ejes del lienzo inclinado: derecha, arriba (sobre el lienzo) y frente.
static const Vector3 EJE_DERECHA_TRAZO = { 1.0f, 0.0f, 0.0f };

static Vector3 EjeArribaTrazo()
{
    return { 0.0f, std::cos(INCLINACION_LIENZO_TRAZO), -std::sin(INCLINACION_LIENZO_TRAZO) };
}


static Vector3 EjeFrenteTrazo()
{
    return { 0.0f, std::sin(INCLINACION_LIENZO_TRAZO), std::cos(INCLINACION_LIENZO_TRAZO) };
}


// Punto del marco del lienzo: (lx, ly, lz) sobre derecha, arriba y frente.
static Vector3 LocalLienzoTrazo(Vector3 base, float lx, float ly, float lz)
{
    Vector3 u = EjeArribaTrazo();
    Vector3 n = EjeFrenteTrazo();

    return {
        base.x + lx,
        base.y + u.y * ly + n.y * lz,
        base.z + u.z * ly + n.z * lz
    };
}


// Centro del borde inferior del lienzo del jugador que va en "orden".
static Vector3 BaseLienzoTrazo(const DisposicionTrazo& d, int orden)
{
    float x = ((float)orden - (float)(d.cantidad - 1) * 0.5f) * d.separacion;
    return { x, Y_BASE_LIENZO_TRAZO, 0.0f };
}


static Vector3 CentroLienzoTrazo(const DisposicionTrazo& d, int orden)
{
    return LocalLienzoTrazo(BaseLienzoTrazo(d, orden), 0.0f, MEDIO_LIENZO_TRAZO, 0.0f);
}


// Punto de la figura (-1..1) sobre la cara del lienzo, "sobre" hacia afuera.
static Vector3 PuntoLienzoTrazo(Vector3 centro, Vector2 punto, float sobre)
{
    Vector3 u = EjeArribaTrazo();
    Vector3 n = EjeFrenteTrazo();
    float lx = punto.x * ESCALA_FIGURA_TRAZO;
    float ly = -punto.y * ESCALA_FIGURA_TRAZO;

    return {
        centro.x + lx,
        centro.y + u.y * ly + n.y * sobre,
        centro.z + u.z * ly + n.z * sobre
    };
}


// La fila de lienzos ocupa ~3/4 del ancho (la mitad con dos jugadores,
// para que se vea el taller a los costados).
static Camera3D CamaraTrazo(const DisposicionTrazo& d)
{
    int altoPantalla = GetScreenHeight() > 0 ? GetScreenHeight() : 1;
    float aspecto = (float)GetScreenWidth() / (float)altoPantalla;
    if (aspecto < 1.0f) aspecto = 1.0f;

    const float tanMedioY = std::tan(22.5f * DEG2RAD);
    float ocupa = d.cantidad <= 2 ? 0.48f : 0.76f;
    float anchoVisible = 2.0f * d.mitadFila / ocupa;
    float distancia = anchoVisible / (2.0f * tanMedioY * aspecto);
    float altoVisible = 2.0f * distancia * tanMedioY;

    Vector3 centro = LocalLienzoTrazo({ 0.0f, Y_BASE_LIENZO_TRAZO, 0.0f }, 0.0f, MEDIO_LIENZO_TRAZO, 0.0f);
    const float picada = 15.0f * DEG2RAD;

    Camera3D camara{};
    camara.target = { 0.0f, centro.y - 0.09f * altoVisible, centro.z };
    camara.position =
    {
        0.0f,
        camara.target.y + std::sin(picada) * distancia,
        camara.target.z + std::cos(picada) * distancia
    };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 45.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


static float Limitar01Trazo(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


// Hash determinista para la decoracion (mismo taller en cada partida).
static float Hash01Trazo(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarTrazo(MinijuegoTrazoPerfecto& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasTrazo(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static bool EsHumanoTrazo(const Participante& participante)
{
    return !(participante.esBot || !participante.conectado);
}


//==================================================
// EFECTOS: ESQUINAS, GOTEO, MANCHAS Y CONFETI
//==================================================

static void EmitirParticulaTrazo(
    MinijuegoTrazoPerfecto& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color,
    bool brillo,
    bool mancha
)
{
    for (int i = 0; i < MAX_PARTICULAS_TRAZO; i++)
    {
        ParticulaTrazoPerfecto& p = minijuego.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.mancha = mancha;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.color = color;
        return;
    }
}


static void AgregarManchaTrazo(MinijuegoTrazoPerfecto& minijuego, Vector3 posicion, float radio, Color color)
{
    ManchaTrazoPerfecto& mancha = minijuego.manchas[minijuego.siguienteMancha];
    mancha.activa = true;
    mancha.posicion = { posicion.x, 0.0f, posicion.z };
    mancha.radio = radio;
    mancha.forma = (int)AzarTrazo(minijuego, 0.0f, 997.0f);
    mancha.color = color;
    minijuego.siguienteMancha = (minijuego.siguienteMancha + 1) % MAX_MANCHAS_TRAZO;
}


// Acierto en una esquina: salpicon de pintura que sale del lienzo.
static void EfectoAciertoTrazo(MinijuegoTrazoPerfecto& m, Vector3 punto, Color color)
{
    Vector3 n = EjeFrenteTrazo();
    int cantidad = CantidadParticulasTrazo(14);

    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AzarTrazo(m, 0.0f, 6.2832f);
        float rapidez = AzarTrazo(m, 0.8f, 2.4f);
        float salida = AzarTrazo(m, 0.6f, 2.0f);
        bool blanco = k % 4 == 3;

        EmitirParticulaTrazo(
            m,
            punto,
            {
                std::cos(angulo) * rapidez + n.x * salida,
                std::sin(angulo) * rapidez + n.y * salida + 0.8f,
                n.z * salida
            },
            AzarTrazo(m, 0.6f, 1.1f),
            AzarTrazo(m, 0.05f, 0.1f),
            7.0f,
            blanco ? Color{ 255, 248, 232, 255 } : color,
            false,
            !blanco && k % 3 == 0
        );
    }

    // Chispas doradas en la pasada aditiva.
    for (int k = 0; k < CantidadParticulasTrazo(6); k++)
    {
        float angulo = AzarTrazo(m, 0.0f, 6.2832f);
        EmitirParticulaTrazo(
            m,
            punto,
            { std::cos(angulo) * 1.4f, std::sin(angulo) * 1.4f, n.z * 0.5f },
            AzarTrazo(m, 0.3f, 0.55f),
            AzarTrazo(m, 0.06f, 0.1f),
            0.0f,
            Color{ 255, 214, 120, 255 },
            true,
            false
        );
    }
}


// Error en una esquina: polvo de carbonilla gris que cae del lienzo.
static void EfectoErrorTrazo(MinijuegoTrazoPerfecto& m, Vector3 punto)
{
    Vector3 n = EjeFrenteTrazo();

    for (int k = 0; k < CantidadParticulasTrazo(8); k++)
    {
        EmitirParticulaTrazo(
            m,
            punto,
            { AzarTrazo(m, -0.6f, 0.6f), AzarTrazo(m, -0.2f, 0.5f), n.z * AzarTrazo(m, 0.3f, 0.9f) },
            AzarTrazo(m, 0.5f, 0.9f),
            AzarTrazo(m, 0.04f, 0.08f),
            3.0f,
            Color{ 96, 92, 98, 255 },
            false,
            false
        );
    }
}


static void EmitirConfetiTrazo(
    MinijuegoTrazoPerfecto& m,
    Vector3 origen,
    Color color,
    int base
)
{
    const Color colores[4] =
    {
        color,
        Color{ 255, 206, 72, 255 },
        Color{ 250, 244, 230, 255 },
        Color{ 222, 92, 150, 255 }
    };

    for (int k = 0; k < CantidadParticulasTrazo(base); k++)
    {
        float angulo = AzarTrazo(m, 0.0f, 6.2832f);
        float rapidez = AzarTrazo(m, 0.6f, 2.2f);

        EmitirParticulaTrazo(
            m,
            origen,
            { std::cos(angulo) * rapidez, AzarTrazo(m, 2.4f, 4.6f), std::sin(angulo) * rapidez * 0.6f + 0.6f },
            AzarTrazo(m, 1.6f, 2.4f),
            AzarTrazo(m, 0.07f, 0.12f),
            3.6f,
            colores[k % 4],
            false,
            false
        );
    }
}


// La estrella paso por una esquina de la figura: se mide a cada pintor.
static void EvaluarEsquinaTrazo(
    MinijuegoTrazoPerfecto& m,
    const Participante participantes[],
    Vector2 esquina
)
{
    DisposicionTrazo d = CalcularDisposicionTrazo(CantidadParticipantesTrazo(m));
    bool aciertoHumano = false;
    bool aciertoBot = false;
    bool errorHumano = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorTrazoPerfecto& j = m.jugadores[i];
        Vector3 centro = CentroLienzoTrazo(d, OrdenParticipanteTrazo(m, i));
        float precision = 1.0f - Limitar01Trazo(Vector2Distance(j.cursor, esquina) / 0.42f);
        bool humano = EsHumanoTrazo(participantes[i]);

        if (precision >= 0.6f)
        {
            j.aciertos++;
            j.destello = 0.6f;
            j.salto = 0.35f;
            j.tipoTexto = precision >= 0.85f ? TEXTO_TRAZO_JUSTO : TEXTO_TRAZO_BIEN;
            j.textoFlotante = DURACION_TEXTO_TRAZO;
            EfectoAciertoTrazo(m, PuntoLienzoTrazo(centro, esquina, 0.08f), participantes[i].color);
            aciertoHumano = aciertoHumano || humano;
            aciertoBot = aciertoBot || !humano;
        }
        else if (precision < 0.15f)
        {
            j.sacudida = 0.4f;
            j.tipoTexto = TEXTO_TRAZO_UY;
            j.textoFlotante = DURACION_TEXTO_TRAZO;
            EfectoErrorTrazo(m, PuntoLienzoTrazo(centro, j.cursor, 0.08f));
            errorHumano = errorHumano || humano;
        }
    }

    if (aciertoHumano)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
    }
    else if (errorHumano)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
    }
    else if (aciertoBot)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
    }
}


static void ActualizarVisualTrazo(
    MinijuegoTrazoPerfecto& minijuego,
    const Participante participantes[],
    float deltaTime
)
{
    MinijuegoTrazoPerfecto& m = minijuego;
    float dt = deltaTime;
    DisposicionTrazo d = CalcularDisposicionTrazo(CantidadParticipantesTrazo(m));
    Vector2 objetivo = PuntoRecorridoTrazo(m.forma, m.progresoObjetivo);

    // Esquinas que la estrella cruzo desde el frame anterior.
    if (m.fase != FASE_TRAZO_PREPARACION && m.progresoObjetivo > m.progresoVisto)
    {
        int cantidad = 0;
        const Vector2* puntos = ObtenerPuntosTrazo(m.forma, cantidad);
        float total = 0.0f;

        for (int k = 0; k < cantidad; k++)
        {
            total += Vector2Distance(puntos[k], puntos[(k + 1) % cantidad]);
        }

        float acumulado = 0.0f;

        for (int k = 1; k <= cantidad && total > 0.0f; k++)
        {
            acumulado += Vector2Distance(puntos[k - 1], puntos[k % cantidad]);
            float f = acumulado / total;

            if (f > m.progresoVisto + 0.00001f && f <= m.progresoObjetivo + 0.00001f)
            {
                EvaluarEsquinaTrazo(m, participantes, puntos[k % cantidad]);
            }
        }

        m.progresoVisto = m.progresoObjetivo;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorTrazoPerfecto& j = m.jugadores[i];

        j.destello = j.destello > dt ? j.destello - dt : 0.0f;
        j.sacudida = j.sacudida > dt ? j.sacudida - dt : 0.0f;
        j.salto = j.salto > dt ? j.salto - dt : 0.0f;
        j.textoFlotante = j.textoFlotante > dt ? j.textoFlotante - dt : 0.0f;

        if (m.fase != FASE_TRAZO_DIBUJANDO)
        {
            j.fueraDeLinea = 0.0f;
            continue;
        }

        float cerca = 1.0f - Limitar01Trazo(Vector2Distance(j.cursor, objetivo) / 0.42f);
        j.calidad += (cerca - j.calidad) * Limitar01Trazo(dt * 6.0f);
        j.fueraDeLinea = cerca <= 0.05f ? j.fueraDeLinea + dt : 0.0f;

        // Goteo: de vez en cuando cae una gota del pincel y mancha el piso.
        j.goteo -= dt;

        if (j.goteo <= 0.0f)
        {
            j.goteo = AzarTrazo(m, 0.7f, 1.6f);
            Vector3 centro = CentroLienzoTrazo(d, OrdenParticipanteTrazo(m, i));
            Vector3 punta = PuntoLienzoTrazo(centro, j.cursor, 0.1f);

            EmitirParticulaTrazo(
                m,
                punta,
                { AzarTrazo(m, -0.1f, 0.1f), -0.2f, AzarTrazo(m, 0.15f, 0.45f) },
                3.0f,
                0.06f,
                9.0f,
                participantes[i].color,
                false,
                true
            );
        }
    }

    // Fin: confeti sobre el lienzo de cada ganador, en tandas.
    if (m.fase == FASE_TRAZO_TERMINADO)
    {
        bool primeraVez = !m.finVisto;
        m.finVisto = true;
        bool tanda = primeraVez || std::fmod(m.tiempoAnimacion, 0.55f) < dt;

        for (int i = 0; i < MAX_PARTICIPANTES && tanda; i++)
        {
            if (
                m.resultado.participantes[i].participo &&
                m.resultado.participantes[i].posicionFinal == 1
            )
            {
                Vector3 base = BaseLienzoTrazo(d, OrdenParticipanteTrazo(m, i));
                Vector3 arriba = LocalLienzoTrazo(base, 0.0f, MEDIO_LIENZO_TRAZO * 2.0f + 0.3f, 0.2f);
                EmitirConfetiTrazo(m, arriba, participantes[i].color, primeraVez ? 40 : 9);
            }
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_TRAZO; i++)
    {
        ParticulaTrazoPerfecto& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= dt;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float freno = 1.0f - Limitar01Trazo(dt * (p.brillo ? 3.0f : 1.2f));
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.velocidad.y -= p.gravedad * dt;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        if (p.posicion.y < 0.04f && p.velocidad.y < 0.0f && !p.brillo)
        {
            if (p.mancha)
            {
                AgregarManchaTrazo(m, p.posicion, p.tamano * 1.6f, p.color);
                p.activa = false;
                continue;
            }

            p.posicion.y = 0.04f;
            p.velocidad = { 0.0f, 0.0f, 0.0f };
            p.gravedad = 0.0f;
        }
    }
}


//==================================================
// PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Atardecer: revoque terracota claro, roble color miel, hierro grafito,
// cortinas verde petroleo y la luz naranja del sol que entra de frente.
// Afuera, la ciudad se funde en una calima rosada. Acento: el dorado de
// la estrella que hay que seguir.

static const Color COLOR_PENUMBRA_TRAZO = { 62, 40, 46, 255 };      // fondo del taller
static const Color COLOR_CALIMA_TRAZO = { 248, 164, 128, 255 };     // calima del atardecer
static const Color COLOR_PISO_TRAZO = { 168, 114, 72, 255 };
static const Color COLOR_JUNTA_TRAZO = { 78, 50, 36, 255 };
static const Color COLOR_LONA_TRAZO = { 212, 198, 168, 255 };
static const Color COLOR_REVOQUE_TRAZO = { 214, 172, 136, 255 };
static const Color COLOR_LADRILLO_TRAZO = { 168, 90, 62, 255 };
static const Color COLOR_HIERRO_TRAZO = { 54, 52, 64, 255 };
static const Color COLOR_MADERA_TRAZO = { 118, 76, 48, 255 };
static const Color COLOR_MADERA_CLARA_TRAZO = { 178, 128, 80, 255 };
static const Color COLOR_LINO_TRAZO = { 214, 196, 160, 255 };
static const Color COLOR_LIENZO_TRAZO = { 244, 238, 224, 255 };
static const Color COLOR_CARBON_TRAZO = { 58, 50, 58, 255 };
static const Color COLOR_CORTINA_TRAZO = { 42, 88, 98, 255 };
static const Color COLOR_YESO_TRAZO = { 232, 226, 214, 255 };
static const Color COLOR_LATON_TRAZO = { 196, 150, 74, 255 };
static const Color COLOR_ACENTO_TRAZO = { 255, 198, 44, 255 };

// Pinturas del taller: ninguna es el color de un jugador.
static const Color PINTURAS_TRAZO[6] =
{
    { 214, 62, 150, 255 },     // magenta
    { 38, 176, 176, 255 },     // turquesa
    { 244, 138, 40, 255 },     // naranja
    { 138, 84, 200, 255 },     // violeta
    { 244, 240, 228, 255 },    // blanco
    { 206, 156, 66, 255 }      // ocre
};

// Ojo de referencia para la niebla (la camara de juego con 4 jugadores).
static const Vector3 OJO_NIEBLA_TRAZO = { 0.0f, 6.0f, 12.0f };


static unsigned char CanalTrazo(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoTrazo(Color color, float factor)
{
    return Color{
        CanalTrazo((float)color.r * factor),
        CanalTrazo((float)color.g * factor),
        CanalTrazo((float)color.b * factor),
        color.a
    };
}


// Caras de arriba con luz calida; las laterales, sombra apenas violacea.
static Color SombraTrazo(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalTrazo((float)color.r * factor * 1.04f),
            CanalTrazo((float)color.g * factor * 1.0f),
            CanalTrazo((float)color.b * factor * 0.94f),
            color.a
        };
    }

    return Color{
        CanalTrazo((float)color.r * factor * 0.96f),
        CanalTrazo((float)color.g * factor * 0.93f),
        CanalTrazo((float)color.b * factor * 1.02f),
        color.a
    };
}


static Color MezclaTrazo(Color a, Color b, float t)
{
    t = Limitar01Trazo(t);

    return Color{
        CanalTrazo((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalTrazo((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalTrazo((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalTrazo((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color ConAlfaTrazo(Color color, float alfa)
{
    color.a = CanalTrazo(alfa);
    return color;
}


static float DistanciaOjoTrazo(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_TRAZO.x;
    float dy = p.y - OJO_NIEBLA_TRAZO.y;
    float dz = p.z - OJO_NIEBLA_TRAZO.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}


// Penumbra del taller: los rincones lejanos se apagan hacia el bordo.
static Color TinteTrazo(Color color, Vector3 p)
{
    float n = (DistanciaOjoTrazo(p) - 13.0f) / 30.0f;
    Color niebla = COLOR_PENUMBRA_TRAZO;
    niebla.a = color.a;
    return MezclaTrazo(color, niebla, n < 0.0f ? 0.0f : (n > 0.35f ? 0.35f : n));
}


// Afuera: techos y torres se aclaran hacia la calima del atardecer.
static Color TinteLejosTrazo(Color color, Vector3 p)
{
    float n = (DistanciaOjoTrazo(p) - 30.0f) / 190.0f;
    Color calima = COLOR_CALIMA_TRAZO;
    calima.a = color.a;
    return MezclaTrazo(color, calima, n < 0.0f ? 0.0f : (n > 0.72f ? 0.72f : n));
}


static void VerticeTrazo(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


// Cuadrilatero como dos triangulos (sirve dentro de RL_TRIANGLES).
static void TrianguloQuadTrazo(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTrazo(a, color);
    VerticeTrazo(b, color);
    VerticeTrazo(c, color);
    VerticeTrazo(a, color);
    VerticeTrazo(c, color);
    VerticeTrazo(d, color);
}


static void QuadTrazo(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTrazo(a, color);
    VerticeTrazo(b, color);
    VerticeTrazo(c, color);
    VerticeTrazo(d, color);
}


// Quad con la penumbra calculada por vertice (suelos y muros grandes).
static void QuadNieblaTrazo(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTrazo(a, TinteTrazo(color, a));
    VerticeTrazo(b, TinteTrazo(color, b));
    VerticeTrazo(c, TinteTrazo(color, c));
    VerticeTrazo(d, TinteTrazo(color, d));
}


// Caja sobre tres ejes (medios lados ya escalados): ey arriba 1.0,
// ez frente 0.78, -ex izquierda 0.70, +ex derecha 0.62, fondo 0.54.
static void CajaEjesTrazo(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color)
{
    Vector3 v[8];

    for (int k = 0; k < 8; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sy = (k & 2) ? 1.0f : -1.0f;
        float sz = (k & 4) ? 1.0f : -1.0f;
        v[k] =
        {
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    }

    rlBegin(RL_QUADS);
    QuadTrazo(v[6], v[7], v[3], v[2], SombraTrazo(color, 1.0f, true));     // arriba
    QuadTrazo(v[4], v[5], v[7], v[6], SombraTrazo(color, 0.78f, false));   // frente
    QuadTrazo(v[1], v[0], v[2], v[3], SombraTrazo(color, 0.54f, false));   // fondo
    QuadTrazo(v[5], v[1], v[3], v[7], SombraTrazo(color, 0.62f, false));   // derecha
    QuadTrazo(v[0], v[4], v[6], v[2], SombraTrazo(color, 0.70f, false));   // izquierda
    QuadTrazo(v[0], v[1], v[5], v[4], SombraTrazo(color, 0.45f, false));   // abajo
    rlEnd();
}


static void CuboTrazo(Vector3 c, Vector3 t, Color color)
{
    CajaEjesTrazo(
        c,
        { t.x * 0.5f, 0.0f, 0.0f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { 0.0f, 0.0f, t.z * 0.5f },
        color
    );
}


static void CuboNieblaTrazo(Vector3 c, Vector3 t, Color color)
{
    CuboTrazo(c, t, TinteTrazo(color, c));
}


// Caja girada en Y (grados): muebles y utileria no alineados.
static void CuboGiradoTrazo(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    float a = giroGrados * DEG2RAD;
    float co = std::cos(a);
    float si = std::sin(a);

    CajaEjesTrazo(
        c,
        { co * t.x * 0.5f, 0.0f, -si * t.x * 0.5f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { si * t.z * 0.5f, 0.0f, co * t.z * 0.5f },
        color
    );
}


// Viga de seccion cuadrada entre dos puntos (patas, travesanos).
static void VigaTrazo(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 c = Vector3Scale(Vector3Add(a, b), 0.5f);
    float g = grosor * 0.5f;

    if (std::fabs(dir.y) > 0.7f)
    {
        // Casi vertical: el eje largo hace de "arriba".
        Vector3 ex = Vector3Normalize(Vector3CrossProduct(dir, { 0.0f, 0.0f, 1.0f }));
        Vector3 ez = Vector3CrossProduct(ex, dir);
        if (dir.y < 0.0f) { dir = Vector3Negate(dir); ex = Vector3Negate(ex); }
        CajaEjesTrazo(c, Vector3Scale(ex, g), Vector3Scale(dir, largo * 0.5f), Vector3Scale(ez, g), color);
    }
    else
    {
        Vector3 ez = Vector3Normalize(Vector3CrossProduct(dir, { 0.0f, 1.0f, 0.0f }));
        Vector3 ey = Vector3CrossProduct(ez, dir);
        if (ey.y < 0.0f) { ey = Vector3Negate(ey); ez = Vector3Negate(ez); }
        if (ez.z < 0.0f) { ez = Vector3Negate(ez); dir = Vector3Negate(dir); }
        CajaEjesTrazo(c, Vector3Scale(dir, largo * 0.5f), Vector3Scale(ey, g), Vector3Scale(ez, g), color);
    }
}


// Luz falsa de las caras laterales segun su orientacion (angulo en XZ:
// 0 = +X, 90 = +Z hacia la camara). El sol entra de frente, desde el fondo.
static float LuzLateralTrazo(float angulo)
{
    return 0.70f + 0.10f * std::sin(angulo) - 0.06f * std::cos(angulo);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (r1).
static void PrismaTrazo(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color
)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = SombraTrazo(color, 1.0f, true);
    Color tapaAbajo = SombraTrazo(color, 0.45f, false);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        Color cara = SombraTrazo(color, LuzLateralTrazo(a0 + paso * 0.5f), false);

        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadTrazo(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadTrazo(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }

        if (r0 > 0.001f)
        {
            Vector3 centro = { base.x, base.y, base.z };
            QuadTrazo(centro, { base.x + c0 * r0, base.y, base.z + s0 * r0 }, { base.x + c1 * r0, base.y, base.z + s1 * r0 }, centro, tapaAbajo);
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloTrazo(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadTrazo(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorTrazo(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloTrazo(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorTrazo(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano casi vertical, como la vista).
static void ResplandorFrenteTrazo(Vector3 c, float radio, Color color)
{
    ResplandorTrazo(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.97f, 0.26f }, radio, color);
}


// Disco solido de borde irregular en el piso (manchas de pintura).
static void ManchaPisoTrazo(Vector3 c, float radio, int semilla, Color color)
{
    const int LADOS = 12;
    float radios[LADOS];

    for (int k = 0; k < LADOS; k++)
    {
        radios[k] = radio * (0.7f + 0.5f * Hash01Trazo(semilla, k));
    }

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float r0 = radios[k];
        float r1 = radios[(k + 1) % LADOS];

        VerticeTrazo(c, color);
        VerticeTrazo({ c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 }, color);
        VerticeTrazo({ c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 }, color);
    }

    rlEnd();
}


// Salpicadura completa: mancha central, gotitas alrededor y un chorro.
static void SalpicaduraTrazo(Vector3 c, float radio, int semilla, Color color)
{
    ManchaPisoTrazo(c, radio, semilla, color);

    int gotas = 3 + (int)(Hash01Trazo(semilla, 40) * 4.0f);

    for (int k = 0; k < gotas; k++)
    {
        float angulo = Hash01Trazo(semilla, 41 + k) * 6.2832f;
        float distancia = radio * (1.3f + 1.2f * Hash01Trazo(semilla, 60 + k));
        Vector3 g = { c.x + std::cos(angulo) * distancia, c.y, c.z + std::sin(angulo) * distancia };
        ManchaPisoTrazo(g, radio * (0.12f + 0.16f * Hash01Trazo(semilla, 80 + k)), semilla + k * 7, color);
    }

    // Chorro: una lagrima alargada que sale de la mancha.
    float angulo = Hash01Trazo(semilla, 99) * 6.2832f;
    Vector3 dir = { std::cos(angulo), 0.0f, std::sin(angulo) };
    Vector3 lado = { -dir.z, 0.0f, dir.x };
    float largo = radio * (1.4f + Hash01Trazo(semilla, 98));
    float ancho = radio * 0.22f;

    rlBegin(RL_TRIANGLES);
    VerticeTrazo({ c.x + lado.x * ancho, c.y, c.z + lado.z * ancho }, color);
    VerticeTrazo({ c.x - lado.x * ancho, c.y, c.z - lado.z * ancho }, color);
    VerticeTrazo({ c.x + dir.x * largo, c.y, c.z + dir.z * largo }, color);
    rlEnd();
}


//==================================================
// CIELO Y CIUDAD (lo que se ve por el ventanal)
//==================================================

// Cielo del atardecer (2D, antes del 3D): violeta arriba, rosa y naranja
// hacia el horizonte, sol bajo con bandas de calima, nubes alargadas y
// pajaros que cruzan.
static void DibujarCieloTrazo(const Camera3D& vista, float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    Vector3 lejos = { vista.position.x, vista.position.y, vista.position.z - 600.0f };
    float horizonte = GetWorldToScreen(lejos, vista).y;
    if (horizonte < (float)alto * 0.10f) horizonte = (float)alto * 0.10f;
    if (horizonte > (float)alto * 0.70f) horizonte = (float)alto * 0.70f;

    int h1 = (int)(horizonte * 0.42f);
    int h2 = (int)(horizonte * 0.82f);
    int h3 = (int)horizonte;

    DrawRectangleGradientV(0, 0, ancho, h1, Color{ 58, 40, 96, 255 }, Color{ 150, 66, 124, 255 });
    DrawRectangleGradientV(0, h1, ancho, h2 - h1, Color{ 150, 66, 124, 255 }, Color{ 238, 118, 96, 255 });
    DrawRectangleGradientV(0, h2, ancho, h3 - h2, Color{ 238, 118, 96, 255 }, Color{ 255, 180, 108, 255 });
    DrawRectangleGradientV(0, h3, ancho, alto - h3, Color{ 255, 180, 108, 255 }, Color{ 226, 136, 112, 255 });

    // Sol bajo, un poco a la derecha, casi tocando las torres.
    Vector2 sol = GetWorldToScreen({ 26.0f, 1.0f, -180.0f }, vista);
    float radio = (float)alto * 0.06f;

    DrawCircleGradient(sol, radio * 7.0f, Fade(Color{ 255, 170, 100, 255 }, 0.38f), Fade(Color{ 255, 120, 90, 255 }, 0.0f));
    DrawCircleGradient(sol, radio * 2.4f, Fade(Color{ 255, 220, 150, 255 }, 0.6f), Fade(Color{ 255, 190, 120, 255 }, 0.0f));
    DrawCircleV(sol, radio, Color{ 255, 232, 168, 255 });

    // Bandas de calima que cruzan el disco.
    for (int k = 0; k < 3; k++)
    {
        float y = sol.y + radio * (0.15f + 0.3f * (float)k);
        DrawRectangle((int)(sol.x - radio * 1.3f), (int)y, (int)(radio * 2.6f), 2 + k, Fade(Color{ 244, 140, 100, 255 }, 0.55f));
    }

    if (!detalle)
    {
        return;
    }

    // Nubes alargadas: lomo iluminado de rosa, panza violeta.
    for (int k = 0; k < 9; k++)
    {
        float vel = 4.0f + 4.0f * Hash01Trazo(k, 20);
        float recorrido = (float)(ancho + 800);
        float x = std::fmod(Hash01Trazo(k, 21) * recorrido + t * vel, recorrido) - 400.0f;
        float y = horizonte * (0.2f + 0.62f * Hash01Trazo(k, 22));
        float largo = 120.0f + 240.0f * Hash01Trazo(k, 23);
        float grueso = 6.0f + 8.0f * Hash01Trazo(k, 24);
        float cerca = y / horizonte;

        DrawEllipse((int)x, (int)(y + grueso * 0.4f), largo, grueso, Fade(Color{ 120, 62, 120, 255 }, 0.55f));
        DrawEllipse((int)(x + largo * 0.1f), (int)y, largo * 0.8f, grueso * 0.6f, Fade(MezclaTrazo(Color{ 255, 150, 150, 255 }, Color{ 255, 200, 140, 255 }, cerca), 0.7f));
    }

    // Bandada de pajaros que cruza despacio, aleteando.
    for (int k = 0; k < 6; k++)
    {
        float recorrido = (float)(ancho + 300);
        float x = std::fmod(Hash01Trazo(k, 30) * recorrido + t * (26.0f + 6.0f * Hash01Trazo(k, 31)), recorrido) - 150.0f;
        float y = horizonte * (0.45f + 0.25f * Hash01Trazo(k, 32)) + std::sin(t * 0.8f + (float)k) * 6.0f;
        float aleteo = std::sin(t * 9.0f + (float)k * 1.7f) * 3.0f;
        float e = 5.0f + 2.0f * Hash01Trazo(k, 33);
        Color tinta = Fade(Color{ 70, 40, 70, 255 }, 0.85f);

        DrawLineEx({ x - e, y - aleteo }, { x, y + 1.5f }, 2.0f, tinta);
        DrawLineEx({ x, y + 1.5f }, { x + e, y - aleteo }, 2.0f, tinta);
    }
}


// Silueta plana con degradé: pie hacia la calima, tope con borde de sol.
static void SiluetaTrazo(float x0, float x1, float yBase, float yTope, float z, Color color)
{
    Vector3 a = { x0, yBase, z };
    Vector3 b = { x1, yBase, z };
    Vector3 c = { x1, yTope, z };
    Vector3 d = { x0, yTope, z };
    Color pie = TinteLejosTrazo(MezclaTrazo(color, COLOR_CALIMA_TRAZO, 0.25f), a);
    Color tope = TinteLejosTrazo(color, d);

    rlBegin(RL_QUADS);
    VerticeTrazo(a, pie);
    VerticeTrazo(b, pie);
    VerticeTrazo(c, tope);
    VerticeTrazo(d, tope);
    rlEnd();
}


// Cupula en silueta (media elipse en un plano Z).
static void CupulaSiluetaTrazo(Vector3 centro, float radio, float alto, Color color)
{
    Color c = TinteLejosTrazo(color, centro);
    const int LADOS = 14;

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 3.1416f * (float)k / (float)LADOS;
        float a1 = 3.1416f * (float)(k + 1) / (float)LADOS;
        VerticeTrazo(centro, c);
        VerticeTrazo({ centro.x + std::cos(a0) * radio, centro.y + std::sin(a0) * alto, centro.z }, c);
        VerticeTrazo({ centro.x + std::cos(a1) * radio, centro.y + std::sin(a1) * alto, centro.z }, c);
    }

    rlEnd();
}


// Edificio cercano: caja con terraza, ventanas que se van encendiendo
// a medida que cae la tarde, y a veces un tanque de agua o una antena.
static void EdificioTrazo(int k, float x, float z, float ancho, float fondo, float yTope, float t, bool detalle)
{
    const Color fachadas[4] =
    {
        { 196, 118, 92, 255 },
        { 214, 156, 104, 255 },
        { 176, 104, 104, 255 },
        { 206, 136, 118, 255 }
    };

    const float yPie = -40.0f;
    Color fachada = TinteLejosTrazo(fachadas[k % 4], { x, yTope, z });
    CuboTrazo({ x, (yTope + yPie) * 0.5f, z }, { ancho, yTope - yPie, fondo }, fachada);

    // Pretil de la terraza.
    CuboTrazo({ x, yTope + 0.3f, z }, { ancho + 0.3f, 0.6f, fondo + 0.3f }, TonoTrazo(fachada, 0.9f));

    if (!detalle)
    {
        return;
    }

    // Ventanas en la fachada que mira al taller (algunas encendidas).
    float zCara = z + fondo * 0.5f + 0.05f;
    int columnas = (int)(ancho / 1.9f);
    Color apagada = TinteLejosTrazo(Color{ 92, 58, 72, 255 }, { x, yTope, z });

    rlBegin(RL_QUADS);

    for (int fila = 0; fila < 6; fila++)
    {
        float y = yTope - 1.6f - (float)fila * 2.3f;

        for (int c = 0; c < columnas; c++)
        {
            float cx = x - ancho * 0.5f + 1.0f + (float)c * 1.9f;
            float h = Hash01Trazo(k * 31 + c, fila + 7);
            bool encendida = h > 0.45f && t > (h - 0.45f) * 40.0f;
            Color luz = TinteLejosTrazo(
                MezclaTrazo(Color{ 255, 206, 120, 255 }, Color{ 255, 170, 96, 255 }, Hash01Trazo(c, k)),
                { cx, y, z }
            );

            QuadTrazo(
                { cx - 0.38f, y - 0.6f, zCara },
                { cx + 0.38f, y - 0.6f, zCara },
                { cx + 0.38f, y + 0.6f, zCara },
                { cx - 0.38f, y + 0.6f, zCara },
                encendida ? luz : apagada
            );
        }
    }

    rlEnd();

    float h = Hash01Trazo(k, 3);

    if (h < 0.35f)
    {
        // Tanque de agua sobre patas.
        Vector3 b = { x + ancho * 0.2f, yTope + 0.6f, z };
        Color hierro = TinteLejosTrazo(Color{ 96, 66, 66, 255 }, b);
        CuboTrazo({ b.x, b.y + 0.8f, b.z }, { 1.4f, 1.6f, 1.4f }, hierro);
        PrismaTrazo({ b.x, b.y + 1.6f, b.z }, 1.1f, 1.1f, 2.0f, 8, TinteLejosTrazo(Color{ 150, 96, 74, 255 }, b));
        PrismaTrazo({ b.x, b.y + 3.6f, b.z }, 1.15f, 0.1f, 0.8f, 8, TinteLejosTrazo(Color{ 110, 70, 64, 255 }, b));
    }
    else if (h < 0.6f)
    {
        // Antena y chimenea.
        Vector3 b = { x - ancho * 0.25f, yTope + 0.6f, z };
        Color hierro = TinteLejosTrazo(Color{ 80, 60, 70, 255 }, b);
        CuboTrazo({ b.x, b.y + 2.0f, b.z }, { 0.12f, 4.0f, 0.12f }, hierro);
        CuboTrazo({ b.x, b.y + 3.2f, b.z }, { 1.4f, 0.1f, 0.1f }, hierro);
        CuboTrazo({ b.x + 1.8f, b.y + 0.7f, b.z }, { 0.8f, 1.4f, 0.8f }, TinteLejosTrazo(Color{ 150, 90, 74, 255 }, b));
    }
}


// Ciudad vieja en tres planos: techos cercanos, una cupula con su
// campanario a media distancia y torres lejanas casi fundidas en la calima.
static void DibujarCiudadTrazo(float t, int paso, bool detalle)
{
    // Torres y agujas lejanas.
    for (int k = 0; k < 46; k += paso)
    {
        float x = -190.0f + (float)k * 8.4f + Hash01Trazo(k, 1) * 3.0f;
        float ancho = 4.0f + 6.0f * Hash01Trazo(k, 2);
        float tope = -8.0f + 30.0f * Hash01Trazo(k, 3) * Hash01Trazo(k, 3);
        SiluetaTrazo(x - ancho * 0.5f, x + ancho * 0.5f, -60.0f, tope, -190.0f, Color{ 150, 92, 132, 255 });

        if (Hash01Trazo(k, 4) > 0.8f)
        {
            // Aguja.
            Color c = TinteLejosTrazo(Color{ 150, 92, 132, 255 }, { x, tope, -190.0f });
            rlBegin(RL_TRIANGLES);
            VerticeTrazo({ x - ancho * 0.3f, tope, -190.0f }, c);
            VerticeTrazo({ x + ancho * 0.3f, tope, -190.0f }, c);
            VerticeTrazo({ x, tope + 9.0f, -190.0f }, c);
            rlEnd();
        }
    }

    // Colinas lejanas detras de todo.
    for (int k = 0; k < 12; k += paso)
    {
        float x = -220.0f + (float)k * 40.0f;
        SiluetaTrazo(x, x + 42.0f, -60.0f, -6.0f + 6.0f * Hash01Trazo(k, 9), -205.0f, Color{ 170, 110, 140, 255 });
    }

    // Media distancia: cupula con tambor, linterna y un campanario.
    Color piedra = Color{ 118, 70, 96, 255 };
    SiluetaTrazo(-48.0f, -18.0f, -40.0f, 0.0f, -110.0f, piedra);
    SiluetaTrazo(-42.0f, -24.0f, 0.0f, 5.0f, -110.0f, piedra);
    CupulaSiluetaTrazo({ -33.0f, 5.0f, -110.0f }, 9.0f, 10.0f, piedra);
    SiluetaTrazo(-34.2f, -31.8f, 14.5f, 18.0f, -110.0f, piedra);
    CupulaSiluetaTrazo({ -33.0f, 18.0f, -110.0f }, 1.6f, 1.6f, piedra);
    SiluetaTrazo(-12.0f, -7.0f, -40.0f, 16.0f, -100.0f, piedra);
    SiluetaTrazo(-12.6f, -6.4f, 16.0f, 17.0f, -100.0f, piedra);

    {
        Color c = TinteLejosTrazo(piedra, { -9.5f, 17.0f, -100.0f });
        rlBegin(RL_TRIANGLES);
        VerticeTrazo({ -12.6f, 17.0f, -100.0f }, c);
        VerticeTrazo({ -6.4f, 17.0f, -100.0f }, c);
        VerticeTrazo({ -9.5f, 24.0f, -100.0f }, c);
        rlEnd();
    }

    for (int k = 0; k < 10; k += paso)
    {
        float x = 4.0f + (float)k * 7.0f;
        SiluetaTrazo(x, x + 6.0f, -40.0f, -6.0f + 7.0f * Hash01Trazo(k, 12), -105.0f, Color{ 124, 76, 100, 255 });
    }

    // Techos cercanos, con ventanas y terrazas.
    for (int k = 0; k < 24; k += paso)
    {
        float x = -66.0f + (float)k * 5.8f + (Hash01Trazo(k, 5) - 0.5f) * 1.5f;
        float z = -42.0f - 26.0f * Hash01Trazo(k, 6);
        float ancho = 5.0f + 4.0f * Hash01Trazo(k, 7);
        float tope = -13.0f + 12.0f * Hash01Trazo(k, 8);
        EdificioTrazo(k, x, z, ancho, 6.0f + 4.0f * Hash01Trazo(k, 10), tope, t, detalle);
    }
}


// Cilindro o tronco de cono entre dos puntos, con caras sombreadas
// (pinceles, varillas, rollos de tela).
static void CilindroEjeTrazo(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(dir.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 e1 = Vector3Normalize(Vector3CrossProduct(dir, ref));
    Vector3 e2 = Vector3CrossProduct(dir, e1);
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 nm = Vector3Add(Vector3Scale(e1, std::cos(am)), Vector3Scale(e2, std::sin(am)));
        float luz = 0.6f + 0.4f * Limitar01Trazo(nm.y * 0.6f + nm.z * 0.45f + 0.35f);
        Color cara = SombraTrazo(color, luz, false);

        Vector3 o0 = Vector3Add(Vector3Scale(e1, std::cos(a0)), Vector3Scale(e2, std::sin(a0)));
        Vector3 o1 = Vector3Add(Vector3Scale(e1, std::cos(a1)), Vector3Scale(e2, std::sin(a1)));

        QuadTrazo(
            Vector3Add(a, Vector3Scale(o0, ra)),
            Vector3Add(a, Vector3Scale(o1, ra)),
            Vector3Add(b, Vector3Scale(o1, rb)),
            Vector3Add(b, Vector3Scale(o0, rb)),
            cara
        );

        if (rb > 0.001f)
        {
            QuadTrazo(b, Vector3Add(b, Vector3Scale(o0, rb)), Vector3Add(b, Vector3Scale(o1, rb)), b, SombraTrazo(color, 1.0f, true));
        }
    }

    rlEnd();
}


//==================================================
// EL TALLER: PISO, LONA, MUROS, VENTANAL Y CORTINAS
//==================================================

// Piso de tablones de roble a lo largo (hacia la camara), con juntas
// escalonadas, variacion por tabla y vetas.
static void DibujarPisoTrazo(const DisposicionTrazo& d, int paso)
{
    const float ANCHO_TABLA = 0.62f;
    float x0 = -d.muroX;
    float x1 = d.muroX;
    int tablas = (int)((x1 - x0) / ANCHO_TABLA) + 1;

    rlBegin(RL_QUADS);
    QuadNieblaTrazo({ x0, 0.0f, Z_FRENTE_TRAZO }, { x1, 0.0f, Z_FRENTE_TRAZO }, { x1, 0.0f, Z_MURO_TRAZO }, { x0, 0.0f, Z_MURO_TRAZO }, COLOR_JUNTA_TRAZO);

    for (int i = 0; i < tablas; i++)
    {
        float xa = x0 + (float)i * ANCHO_TABLA + 0.025f;
        float xb = xa + ANCHO_TABLA - 0.05f;
        if (xb > x1) xb = x1;
        if (xa >= xb) continue;

        float z = Z_MURO_TRAZO - Hash01Trazo(i, 1) * 2.0f;
        int tramo = 0;

        while (z < Z_FRENTE_TRAZO)
        {
            float largo = 2.2f + 2.4f * Hash01Trazo(i, tramo + 2);
            float za = z < Z_MURO_TRAZO ? Z_MURO_TRAZO : z + 0.025f;
            float zb = z + largo - 0.025f;
            if (zb > Z_FRENTE_TRAZO) zb = Z_FRENTE_TRAZO;

            float variacion = 0.88f + 0.22f * Hash01Trazo(i * 13 + tramo, 3);
            Color tabla = TonoTrazo(COLOR_PISO_TRAZO, variacion);

            QuadNieblaTrazo({ xa, 0.006f, zb }, { xb, 0.006f, zb }, { xb, 0.006f, za }, { xa, 0.006f, za }, tabla);

            // Vetas: dos hilos mas oscuros a lo largo de la tabla.
            if (paso == 1)
            {
                for (int v = 0; v < 2; v++)
                {
                    float xv = xa + (xb - xa) * (0.25f + 0.5f * Hash01Trazo(i * 7 + tramo, 10 + v));
                    float zv0 = za + (zb - za) * 0.1f * Hash01Trazo(i, 20 + v);
                    float zv1 = zb - (zb - za) * 0.2f * Hash01Trazo(i, 30 + v);
                    QuadNieblaTrazo(
                        { xv - 0.012f, 0.008f, zv1 },
                        { xv + 0.012f, 0.008f, zv1 },
                        { xv + 0.012f, 0.008f, zv0 },
                        { xv - 0.012f, 0.008f, zv0 },
                        TonoTrazo(tabla, 0.84f)
                    );
                }
            }

            z += largo;
            tramo++;
        }
    }

    rlEnd();
}


// Lona de proteccion bajo los caballetes: arrugada (alturas por vertice)
// y con manchones de pintura vieja.
static void DibujarLonaTrazo(const DisposicionTrazo& d, int paso)
{
    float x0 = -(d.mitadFila + 0.8f);
    float x1 = d.mitadFila + 0.8f;
    const float z0 = -1.7f;
    const float z1 = 3.3f;
    int nx = (int)((x1 - x0) / (paso == 1 ? 0.9f : 1.8f)) + 1;
    int nz = paso == 1 ? 6 : 3;

    rlBegin(RL_QUADS);

    for (int i = 0; i < nx; i++)
    {
        for (int k = 0; k < nz; k++)
        {
            float xa = x0 + (x1 - x0) * (float)i / (float)nx;
            float xb = x0 + (x1 - x0) * (float)(i + 1) / (float)nx;
            float za = z0 + (z1 - z0) * (float)k / (float)nz;
            float zb = z0 + (z1 - z0) * (float)(k + 1) / (float)nz;

            float y00 = 0.012f + 0.024f * Hash01Trazo(i, k + 50);
            float y10 = 0.012f + 0.024f * Hash01Trazo(i + 1, k + 50);
            float y01 = 0.012f + 0.024f * Hash01Trazo(i, k + 51);
            float y11 = 0.012f + 0.024f * Hash01Trazo(i + 1, k + 51);

            Color tela = TonoTrazo(COLOR_LONA_TRAZO, 0.9f + 0.14f * Hash01Trazo(i * 5 + k, 60));
            QuadNieblaTrazo({ xa, y01, zb }, { xb, y11, zb }, { xb, y10, za }, { xa, y00, za }, tela);
        }
    }

    // Dobladillo: borde mas oscuro en los cuatro lados.
    Color borde = TonoTrazo(COLOR_LONA_TRAZO, 0.78f);
    QuadNieblaTrazo({ x0, 0.03f, z1 }, { x1, 0.03f, z1 }, { x1, 0.03f, z1 - 0.12f }, { x0, 0.03f, z1 - 0.12f }, borde);
    QuadNieblaTrazo({ x0, 0.03f, z0 + 0.12f }, { x1, 0.03f, z0 + 0.12f }, { x1, 0.03f, z0 }, { x0, 0.03f, z0 }, borde);
    QuadNieblaTrazo({ x0, 0.03f, z1 }, { x0 + 0.12f, 0.03f, z1 }, { x0 + 0.12f, 0.03f, z0 }, { x0, 0.03f, z0 }, borde);
    QuadNieblaTrazo({ x1 - 0.12f, 0.03f, z1 }, { x1, 0.03f, z1 }, { x1, 0.03f, z0 }, { x1 - 0.12f, 0.03f, z0 }, borde);

    rlEnd();
}


// Salpicaduras viejas en la lona y el piso (siempre las mismas).
static void DibujarSalpicadurasTrazo(const DisposicionTrazo& d, int paso)
{
    int cantidad = 40 / paso;

    for (int k = 0; k < cantidad; k++)
    {
        bool enLona = Hash01Trazo(k, 70) < 0.7f;
        float x;
        float z;

        if (enLona)
        {
            x = (Hash01Trazo(k, 71) * 2.0f - 1.0f) * (d.mitadFila + 0.6f);
            z = -1.5f + 4.6f * Hash01Trazo(k, 72);
        }
        else
        {
            x = (Hash01Trazo(k, 71) * 2.0f - 1.0f) * (d.muroX - 0.6f);
            z = Z_MURO_TRAZO + 0.8f + (Z_FRENTE_TRAZO - Z_MURO_TRAZO - 2.0f) * Hash01Trazo(k, 72);
        }

        Color pintura = PINTURAS_TRAZO[k % 6];
        Color base = enLona ? COLOR_LONA_TRAZO : COLOR_PISO_TRAZO;
        Color color = TinteTrazo(MezclaTrazo(pintura, base, 0.32f), { x, 0.0f, z });
        SalpicaduraTrazo({ x, 0.045f, z }, 0.07f + 0.14f * Hash01Trazo(k, 73), k * 17 + 3, color);
    }
}


// Parche de ladrillo a la vista donde se cayo el revoque.
static void LadrillosTrazo(float cx, float cy, float rx, float ry, float z, int semilla)
{
    const float ALTO = 0.24f;
    const float ANCHO = 0.52f;
    int filas = (int)(ry * 2.0f / ALTO) + 1;
    int columnas = (int)(rx * 2.0f / ANCHO) + 2;

    for (int f = 0; f < filas; f++)
    {
        float y = cy - ry + ((float)f + 0.5f) * ALTO;
        float desfase = (f & 1) ? ANCHO * 0.5f : 0.0f;

        for (int c = 0; c < columnas; c++)
        {
            float x = cx - rx + (float)c * ANCHO + desfase;
            float u = (x - cx) / rx;
            float v = (y - cy) / ry;

            // Borde irregular: el revoque se cayo en una mancha redondeada.
            if (u * u + v * v > 0.75f + 0.35f * Hash01Trazo(semilla + c, f))
            {
                continue;
            }

            Color ladrillo = TonoTrazo(COLOR_LADRILLO_TRAZO, 0.84f + 0.24f * Hash01Trazo(semilla + c * 3, f + 9));
            CuboNieblaTrazo({ x, y, z }, { ANCHO - 0.05f, ALTO - 0.05f, 0.06f }, ladrillo);
        }
    }
}


// Muro del fondo con el gran ventanal de hierro: revoque, ladrillo a la
// vista, zocalo, alfeizar, carpinteria en cuadricula y el dintel.
static void DibujarMuroVentanalTrazo(const DisposicionTrazo& d, bool detalle)
{
    const float GROSOR = 0.6f;
    float zc = Z_MURO_TRAZO - GROSOR * 0.5f;
    float vx = d.ventanaX;
    float ancho = 2.0f * d.muroX;

    CuboNieblaTrazo({ 0.0f, Y_ALFEIZAR_TRAZO * 0.5f, zc }, { ancho, Y_ALFEIZAR_TRAZO, GROSOR }, COLOR_REVOQUE_TRAZO);
    CuboNieblaTrazo({ 0.0f, (Y_DINTEL_TRAZO + ALTO_TALLER_TRAZO) * 0.5f, zc }, { ancho, ALTO_TALLER_TRAZO - Y_DINTEL_TRAZO, GROSOR }, COLOR_REVOQUE_TRAZO);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float centro = (float)lado * (d.muroX + vx) * 0.5f;
        CuboNieblaTrazo(
            { centro, (Y_ALFEIZAR_TRAZO + Y_DINTEL_TRAZO) * 0.5f, zc },
            { d.muroX - vx, Y_DINTEL_TRAZO - Y_ALFEIZAR_TRAZO, GROSOR },
            COLOR_REVOQUE_TRAZO
        );
    }

    // Zocalo de madera.
    CuboNieblaTrazo({ 0.0f, 0.18f, Z_MURO_TRAZO + 0.04f }, { ancho, 0.36f, 0.08f }, COLOR_MADERA_TRAZO);

    // Dintel de hierro y alfeizar de madera ancho (con cosas encima).
    CuboNieblaTrazo({ 0.0f, Y_DINTEL_TRAZO + 0.12f, Z_MURO_TRAZO + 0.04f }, { 2.0f * vx + 0.6f, 0.24f, 0.1f }, COLOR_HIERRO_TRAZO);
    CuboNieblaTrazo({ 0.0f, Y_ALFEIZAR_TRAZO - 0.06f, Z_MURO_TRAZO - 0.1f }, { 2.0f * vx + 0.5f, 0.14f, 1.0f }, COLOR_MADERA_CLARA_TRAZO);

    // Carpinteria: marco, montantes y travesanos en cuadricula.
    float zv = Z_MURO_TRAZO - 0.35f;
    int columnas = (int)(2.0f * vx / 1.7f + 0.5f);
    if (columnas < 2) columnas = 2;
    const float filasY[5] = { Y_ALFEIZAR_TRAZO, 2.9f, 4.9f, 6.9f, Y_DINTEL_TRAZO };

    for (int c = 0; c <= columnas; c++)
    {
        float x = -vx + 2.0f * vx * (float)c / (float)columnas;
        float grosor = (c == 0 || c == columnas) ? 0.2f : 0.1f;
        CuboNieblaTrazo({ x, (Y_ALFEIZAR_TRAZO + Y_DINTEL_TRAZO) * 0.5f, zv }, { grosor, Y_DINTEL_TRAZO - Y_ALFEIZAR_TRAZO, 0.14f }, COLOR_HIERRO_TRAZO);
    }

    for (int f = 0; f < 5; f++)
    {
        float grosor = (f == 0 || f == 4) ? 0.2f : (f == 3 ? 0.16f : 0.09f);
        CuboNieblaTrazo({ 0.0f, filasY[f], zv }, { 2.0f * vx, grosor, 0.14f }, COLOR_HIERRO_TRAZO);
    }

    if (!detalle)
    {
        return;
    }

    // Ladrillo a la vista a los costados del ventanal.
    LadrillosTrazo(-(vx + (d.muroX - vx) * 0.45f), 3.2f, 1.1f, 1.0f, Z_MURO_TRAZO + 0.03f, 11);
    LadrillosTrazo(vx + (d.muroX - vx) * 0.55f, 6.4f, 1.3f, 0.9f, Z_MURO_TRAZO + 0.03f, 23);
    LadrillosTrazo(vx + (d.muroX - vx) * 0.3f, 1.6f, 0.8f, 0.5f, Z_MURO_TRAZO + 0.03f, 37);
}


// Muros laterales, techo con vigas (se ve en la entrada de camara).
static void DibujarMurosLateralesTrazo(const DisposicionTrazo& d, bool detalle)
{
    rlBegin(RL_QUADS);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * d.muroX;
        Color muro = TonoTrazo(COLOR_REVOQUE_TRAZO, lado < 0 ? 0.74f : 0.66f);
        QuadNieblaTrazo(
            { x, 0.0f, Z_FRENTE_TRAZO },
            { x, 0.0f, Z_MURO_TRAZO },
            { x, ALTO_TALLER_TRAZO, Z_MURO_TRAZO },
            { x, ALTO_TALLER_TRAZO, Z_FRENTE_TRAZO },
            muro
        );
    }

    // Cielorraso.
    QuadNieblaTrazo(
        { -d.muroX, ALTO_TALLER_TRAZO, Z_MURO_TRAZO },
        { d.muroX, ALTO_TALLER_TRAZO, Z_MURO_TRAZO },
        { d.muroX, ALTO_TALLER_TRAZO, Z_FRENTE_TRAZO },
        { -d.muroX, ALTO_TALLER_TRAZO, Z_FRENTE_TRAZO },
        TonoTrazo(COLOR_REVOQUE_TRAZO, 0.5f)
    );

    rlEnd();

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (d.muroX - 0.04f);
        CuboNieblaTrazo({ x, 0.18f, (Z_MURO_TRAZO + Z_FRENTE_TRAZO) * 0.5f }, { 0.08f, 0.36f, Z_FRENTE_TRAZO - Z_MURO_TRAZO }, COLOR_MADERA_TRAZO);
    }

    if (!detalle)
    {
        return;
    }

    // Vigas del techo.
    for (int k = 0; k < 6; k++)
    {
        float z = Z_MURO_TRAZO + 1.2f + (float)k * 2.4f;
        CuboNieblaTrazo({ 0.0f, ALTO_TALLER_TRAZO - 0.25f, z }, { 2.0f * d.muroX, 0.5f, 0.4f }, COLOR_MADERA_TRAZO);
    }

    // Cuadros colgados cerca del fondo de cada muro lateral.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (d.muroX - 0.08f);
        const float zs[2] = { -3.2f, -0.6f };

        for (int k = 0; k < 2; k++)
        {
            float alto = 1.4f + 0.6f * Hash01Trazo(lado + 5, k);
            float ancho = 1.2f + 0.8f * Hash01Trazo(lado + 7, k);
            Vector3 c = { x, 4.4f + 0.3f * (float)k, zs[k] };
            CuboNieblaTrazo(c, { 0.1f, alto + 0.16f, ancho + 0.16f }, COLOR_MADERA_TRAZO);

            // Pintura abstracta: fondo y dos manchones de color.
            float xc = c.x - (float)lado * 0.06f;
            Color fondo = TinteTrazo(MezclaTrazo(PINTURAS_TRAZO[(k + lado + 6) % 6], COLOR_LIENZO_TRAZO, 0.55f), c);
            Color mancha = TinteTrazo(PINTURAS_TRAZO[(k * 2 + 3) % 6], c);
            rlBegin(RL_QUADS);
            QuadTrazo({ xc, c.y - alto * 0.5f, c.z - ancho * 0.5f }, { xc, c.y - alto * 0.5f, c.z + ancho * 0.5f }, { xc, c.y + alto * 0.5f, c.z + ancho * 0.5f }, { xc, c.y + alto * 0.5f, c.z - ancho * 0.5f }, fondo);
            xc -= (float)lado * 0.01f;
            QuadTrazo({ xc, c.y - alto * 0.3f, c.z - ancho * 0.3f }, { xc, c.y - alto * 0.3f, c.z + ancho * 0.1f }, { xc, c.y + alto * 0.2f, c.z + ancho * 0.1f }, { xc, c.y + alto * 0.2f, c.z - ancho * 0.3f }, mancha);
            rlEnd();
        }
    }
}


// Cortina pesada que cuelga de la barra y ondea con la brisa del
// ventanal: pliegues por franjas y vaiven mayor abajo.
static void CortinaTrazo(float xCentro, float t, int semilla)
{
    const int FRANJAS = 12;
    const int TRAMOS = 6;
    const float ANCHO = 1.6f;
    const float Y_ARRIBA = Y_DINTEL_TRAZO + 0.35f;
    const float Y_ABAJO = 0.15f;
    const float Z_BASE = Z_MURO_TRAZO + 0.4f;

    rlBegin(RL_QUADS);

    for (int f = 0; f < FRANJAS; f++)
    {
        for (int tr = 0; tr < TRAMOS; tr++)
        {
            Vector3 v[4];
            float luz[2];

            for (int e = 0; e < 2; e++)
            {
                float u = (float)(f + e) / (float)FRANJAS;
                luz[e] = 0.72f + 0.28f * std::cos(u * 3.1416f * 6.0f);
            }

            for (int q = 0; q < 4; q++)
            {
                int fi = f + ((q == 1 || q == 2) ? 1 : 0);
                int ti = tr + ((q >= 2) ? 1 : 0);
                float u = (float)fi / (float)FRANJAS;
                float h = (float)ti / (float)TRAMOS;     // 0 arriba .. 1 abajo
                float y = Y_ARRIBA - (Y_ARRIBA - Y_ABAJO) * h;
                float vaiven = std::sin(t * 0.9f + h * 2.2f + (float)semilla) * 0.12f * h * h;
                float pliegue = std::sin(u * 3.1416f * 6.0f) * 0.13f;
                v[q] = { xCentro + (u - 0.5f) * ANCHO + vaiven * 0.6f, y, Z_BASE + pliegue + vaiven };
            }

            Color c0 = TinteTrazo(TonoTrazo(COLOR_CORTINA_TRAZO, luz[0]), v[0]);
            Color c1 = TinteTrazo(TonoTrazo(COLOR_CORTINA_TRAZO, luz[1]), v[1]);
            VerticeTrazo(v[0], c0);
            VerticeTrazo(v[1], c1);
            VerticeTrazo(v[2], c1);
            VerticeTrazo(v[3], c0);
        }
    }

    rlEnd();
}


static void DibujarCortinasTrazo(const DisposicionTrazo& d, float t)
{
    float y = Y_DINTEL_TRAZO + 0.42f;
    float z = Z_MURO_TRAZO + 0.4f;
    float x = d.ventanaX + 1.4f;

    CilindroEjeTrazo({ -x, y, z }, { x, y, z }, 0.05f, 0.05f, 6, COLOR_LATON_TRAZO);
    CilindroEjeTrazo({ -x - 0.18f, y, z }, { -x, y, z }, 0.1f, 0.1f, 6, COLOR_LATON_TRAZO);
    CilindroEjeTrazo({ x, y, z }, { x + 0.18f, y, z }, 0.1f, 0.1f, 6, COLOR_LATON_TRAZO);

    CortinaTrazo(-(d.ventanaX + 0.25f), t, 0);
    CortinaTrazo(d.ventanaX + 0.25f, t, 2);
}


//==================================================
// ANILLO CERCANO: UTILERIA DEL PINTOR
//==================================================

// Frasco de vidrio con pintura hasta la mitad y tapa de lata.
// MODELO FUTURO: frasco de pintura GLB.
static void FrascoTrazo(Vector3 base, float radio, float alto, Color pintura)
{
    PrismaTrazo(base, radio, radio, alto * 0.62f, 8, pintura);
    PrismaTrazo({ base.x, base.y + alto * 0.62f, base.z }, radio, radio * 0.92f, alto * 0.3f, 8, Color{ 196, 214, 212, 255 });
    PrismaTrazo({ base.x, base.y + alto * 0.92f, base.z }, radio * 0.95f, radio * 0.95f, alto * 0.1f, 8, Color{ 150, 146, 150, 255 });
}


// Frasco con pinceles parados (cabos y puntas de colores).
// MODELO FUTURO: pinceles en frasco GLB.
static void PincelesFrascoTrazo(Vector3 base, float escala, int semilla)
{
    PrismaTrazo(base, 0.16f * escala, 0.15f * escala, 0.36f * escala, 8, Color{ 176, 196, 196, 255 });

    for (int k = 0; k < 5; k++)
    {
        float angulo = (float)k * 1.26f + (float)semilla;
        float abre = 0.08f + 0.06f * Hash01Trazo(semilla, k);
        Vector3 pie = { base.x, base.y + 0.05f, base.z };
        Vector3 tope =
        {
            base.x + std::cos(angulo) * abre * escala * 3.0f,
            base.y + (0.8f + 0.25f * Hash01Trazo(semilla, k + 5)) * escala,
            base.z + std::sin(angulo) * abre * escala * 3.0f
        };
        Vector3 virola = Vector3Lerp(pie, tope, 0.82f);
        CilindroEjeTrazo(pie, virola, 0.025f * escala, 0.022f * escala, 5, Hash01Trazo(semilla, k + 9) < 0.5f ? COLOR_MADERA_CLARA_TRAZO : Color{ 60, 60, 70, 255 });
        CilindroEjeTrazo(virola, Vector3Lerp(pie, tope, 0.9f), 0.03f * escala, 0.03f * escala, 5, Color{ 200, 200, 206, 255 });
        CilindroEjeTrazo(Vector3Lerp(pie, tope, 0.9f), tope, 0.035f * escala, 0.008f * escala, 5, PINTURAS_TRAZO[(semilla + k) % 6]);
    }
}


// Paleta de madera con el agujero del pulgar y montoncitos de pintura.
// MODELO FUTURO: paleta GLB.
static void PaletaTrazo(Vector3 centro, float escala, int semilla)
{
    PrismaTrazo(centro, 0.42f * escala, 0.42f * escala, 0.03f, 14, COLOR_MADERA_CLARA_TRAZO);
    PrismaTrazo({ centro.x - 0.22f * escala, centro.y + 0.031f, centro.z + 0.1f * escala }, 0.06f * escala, 0.06f * escala, 0.002f, 8, Color{ 70, 46, 34, 255 });

    for (int k = 0; k < 6; k++)
    {
        float angulo = -1.2f + (float)k * 0.62f;
        Vector3 p = { centro.x + std::cos(angulo) * 0.28f * escala, centro.y + 0.03f, centro.z - std::sin(angulo) * 0.28f * escala };
        PrismaTrazo(p, 0.06f * escala, 0.04f * escala, 0.035f, 6, PINTURAS_TRAZO[(k + semilla) % 6]);
    }
}


// Lata de pintura abierta con chorreadura por el costado.
// MODELO FUTURO: lata de pintura GLB.
static void LataPinturaTrazo(Vector3 base, Color pintura, float escala)
{
    float r = 0.24f * escala;
    float h = 0.34f * escala;
    PrismaTrazo(base, r, r, h, 10, Color{ 168, 170, 178, 255 });
    PrismaTrazo({ base.x, base.y + h - 0.02f, base.z }, r * 0.9f, r * 0.9f, 0.03f, 10, pintura);
    AnilloTrazo({ base.x, base.y + h + 0.012f, base.z }, r * 0.9f, r, Color{ 196, 198, 204, 255 }, 10);
    CuboTrazo({ base.x + r * 0.3f, base.y + h * 0.62f, base.z + r * 0.97f }, { 0.07f * escala, h * 0.75f, 0.02f }, pintura);
    CuboTrazo({ base.x - r * 0.4f, base.y + h * 0.8f, base.z + r * 0.92f }, { 0.05f * escala, h * 0.4f, 0.02f }, pintura);
    // Etiqueta.
    CuboTrazo({ base.x, base.y + h * 0.4f, base.z + r * 0.96f }, { r * 1.1f, h * 0.3f, 0.02f }, Color{ 236, 224, 196, 255 });
}


// Mesa de trabajo con frascos, pinceles, paleta, pomos y trapos.
// MODELO FUTURO: mesa de trabajo del pintor GLB.
static void MesaTrabajoTrazo(Vector3 base, int semilla)
{
    const float ANCHO = 2.6f;
    const float FONDO = 1.2f;
    const float ALTO = 1.02f;

    CuboTrazo({ base.x, base.y + ALTO - 0.05f, base.z }, { ANCHO, 0.1f, FONDO }, COLOR_MADERA_CLARA_TRAZO);
    CuboTrazo({ base.x, base.y + 0.32f, base.z }, { ANCHO - 0.2f, 0.06f, FONDO - 0.2f }, COLOR_MADERA_TRAZO);

    for (int k = 0; k < 4; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sz = (k & 2) ? 1.0f : -1.0f;
        CuboTrazo({ base.x + sx * (ANCHO * 0.5f - 0.1f), base.y + ALTO * 0.5f - 0.05f, base.z + sz * (FONDO * 0.5f - 0.1f) }, { 0.1f, ALTO - 0.1f, 0.1f }, COLOR_MADERA_TRAZO);
    }

    float y = base.y + ALTO;

    for (int k = 0; k < 4; k++)
    {
        FrascoTrazo({ base.x - 0.95f + (float)k * 0.26f, y, base.z - 0.32f + 0.1f * Hash01Trazo(semilla, k) }, 0.1f, 0.26f + 0.06f * Hash01Trazo(semilla, k + 4), PINTURAS_TRAZO[(semilla + k) % 6]);
    }

    PincelesFrascoTrazo({ base.x + 0.15f, y, base.z - 0.28f }, 1.0f, semilla);
    PaletaTrazo({ base.x + 0.65f, y, base.z + 0.18f }, 0.9f, semilla);

    // Pomos de oleo tirados.
    for (int k = 0; k < 4; k++)
    {
        float giro = 40.0f * Hash01Trazo(semilla, k + 20) - 20.0f;
        CuboGiradoTrazo({ base.x - 0.7f + (float)k * 0.22f, y + 0.035f, base.z + 0.3f }, { 0.07f, 0.07f, 0.26f }, giro, Color{ 220, 220, 226, 255 });
        CuboGiradoTrazo({ base.x - 0.7f + (float)k * 0.22f, y + 0.036f, base.z + 0.3f }, { 0.072f, 0.072f, 0.1f }, giro, PINTURAS_TRAZO[(k + 2) % 6]);
    }

    // Trapo manchado que cuelga del borde.
    CuboTrazo({ base.x + 1.1f, y - 0.25f, base.z + FONDO * 0.5f + 0.02f }, { 0.36f, 0.5f, 0.03f }, Color{ 206, 196, 176, 255 });
    CuboTrazo({ base.x + 1.05f, y - 0.3f, base.z + FONDO * 0.5f + 0.04f }, { 0.14f, 0.16f, 0.01f }, PINTURAS_TRAZO[3]);

    // Latas en el estante de abajo.
    for (int k = 0; k < 3; k++)
    {
        LataPinturaTrazo({ base.x - 0.7f + (float)k * 0.65f, base.y + 0.35f, base.z }, PINTURAS_TRAZO[(semilla + k * 2) % 6], 0.85f);
    }
}


// Busto de yeso sobre un pedestal (modelo de los dibujos).
// MODELO FUTURO: busto de yeso GLB.
static void BustoTrazo(Vector3 base)
{
    CuboTrazo({ base.x, base.y + 0.08f, base.z }, { 0.9f, 0.16f, 0.9f }, COLOR_YESO_TRAZO);
    CuboTrazo({ base.x, base.y + 0.75f, base.z }, { 0.7f, 1.2f, 0.7f }, TonoTrazo(COLOR_YESO_TRAZO, 0.94f));
    CuboTrazo({ base.x, base.y + 1.42f, base.z }, { 0.86f, 0.14f, 0.86f }, COLOR_YESO_TRAZO);

    float y = base.y + 1.49f;
    PrismaTrazo({ base.x, y, base.z }, 0.42f, 0.3f, 0.36f, 8, COLOR_YESO_TRAZO);
    PrismaTrazo({ base.x, y + 0.36f, base.z }, 0.12f, 0.11f, 0.2f, 8, COLOR_YESO_TRAZO);
    PrismaTrazo({ base.x, y + 0.54f, base.z }, 0.16f, 0.24f, 0.16f, 8, COLOR_YESO_TRAZO);
    PrismaTrazo({ base.x, y + 0.7f, base.z }, 0.24f, 0.22f, 0.2f, 8, COLOR_YESO_TRAZO);
    PrismaTrazo({ base.x, y + 0.9f, base.z }, 0.22f, 0.08f, 0.14f, 8, COLOR_YESO_TRAZO);
    CuboTrazo({ base.x + 0.06f, y + 0.72f, base.z + 0.22f }, { 0.06f, 0.14f, 0.08f }, COLOR_YESO_TRAZO);
}


// Taburete de tres patas con una paleta y un trapo encima.
// MODELO FUTURO: taburete GLB.
static void TabureteTrazo(Vector3 base, int semilla)
{
    const float ALTO = 0.9f;

    for (int k = 0; k < 3; k++)
    {
        float angulo = 0.5f + (float)k * 2.094f;
        Vector3 pie = { base.x + std::cos(angulo) * 0.42f, base.y, base.z + std::sin(angulo) * 0.42f };
        Vector3 tope = { base.x + std::cos(angulo) * 0.24f, base.y + ALTO - 0.06f, base.z + std::sin(angulo) * 0.24f };
        VigaTrazo(pie, tope, 0.07f, COLOR_MADERA_TRAZO);
    }

    AnilloTrazo({ base.x, base.y + 0.38f, base.z }, 0.3f, 0.35f, COLOR_MADERA_TRAZO, 12);
    PrismaTrazo({ base.x, base.y + ALTO - 0.06f, base.z }, 0.38f, 0.38f, 0.08f, 14, COLOR_MADERA_CLARA_TRAZO);
    PaletaTrazo({ base.x, base.y + ALTO + 0.02f, base.z }, 0.75f, semilla);
}


// Carrito de pintor de dos estantes con ruedas.
// MODELO FUTURO: carrito de pintor GLB.
static void CarritoTrazo(Vector3 base, int semilla)
{
    const float ANCHO = 1.3f;
    const float FONDO = 0.8f;

    for (int k = 0; k < 4; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sz = (k & 2) ? 1.0f : -1.0f;
        Vector3 p = { base.x + sx * (ANCHO * 0.5f - 0.05f), base.y, base.z + sz * (FONDO * 0.5f - 0.05f) };
        CuboTrazo({ p.x, p.y + 0.6f, p.z }, { 0.06f, 1.0f, 0.06f }, COLOR_HIERRO_TRAZO);
        PrismaTrazo({ p.x, p.y, p.z }, 0.07f, 0.07f, 0.1f, 8, Color{ 40, 40, 46, 255 });
    }

    CuboTrazo({ base.x, base.y + 0.35f, base.z }, { ANCHO, 0.05f, FONDO }, TonoTrazo(COLOR_HIERRO_TRAZO, 1.3f));
    CuboTrazo({ base.x, base.y + 1.08f, base.z }, { ANCHO, 0.05f, FONDO }, TonoTrazo(COLOR_HIERRO_TRAZO, 1.3f));

    float y = base.y + 1.1f;
    FrascoTrazo({ base.x - 0.4f, y, base.z - 0.15f }, 0.11f, 0.3f, PINTURAS_TRAZO[(semilla + 1) % 6]);
    FrascoTrazo({ base.x - 0.14f, y, base.z - 0.2f }, 0.09f, 0.24f, PINTURAS_TRAZO[(semilla + 4) % 6]);
    PincelesFrascoTrazo({ base.x + 0.3f, y, base.z - 0.1f }, 0.9f, semilla + 3);
    // Botella de aguarras.
    PrismaTrazo({ base.x + 0.05f, y, base.z + 0.18f }, 0.09f, 0.09f, 0.34f, 8, Color{ 110, 150, 120, 255 });
    PrismaTrazo({ base.x + 0.05f, y + 0.34f, base.z + 0.18f }, 0.09f, 0.03f, 0.12f, 8, Color{ 110, 150, 120, 255 });

    for (int k = 0; k < 2; k++)
    {
        LataPinturaTrazo({ base.x - 0.3f + (float)k * 0.6f, base.y + 0.38f, base.z }, PINTURAS_TRAZO[(semilla + k * 3) % 6], 0.8f);
    }
}


// Lienzos viejos apoyados contra el muro, unos de espaldas (bastidor a la
// vista) y otros pintados.
// MODELO FUTURO: lienzos apilados GLB.
static void LienzosApiladosTrazo(Vector3 base, int semilla)
{
    for (int k = 0; k < 4; k++)
    {
        float ancho = 1.0f + 0.7f * Hash01Trazo(semilla, k);
        float alto = 1.1f + 0.9f * Hash01Trazo(semilla, k + 4);
        float inclina = 0.22f + 0.06f * (float)k;
        float x = base.x + ((float)k - 1.5f) * 0.35f;
        float z = base.z + 0.18f * (float)k;

        Vector3 u = { 0.0f, std::cos(inclina), -std::sin(inclina) };
        Vector3 n = { 0.0f, std::sin(inclina), std::cos(inclina) };
        Vector3 c = { x, base.y + u.y * alto * 0.5f, z + u.z * alto * 0.5f + 0.3f };

        CajaEjesTrazo(c, { ancho * 0.5f, 0.0f, 0.0f }, Vector3Scale(u, alto * 0.5f), Vector3Scale(n, 0.04f), TinteTrazo(COLOR_LINO_TRAZO, c));

        Vector3 cara = Vector3Add(c, Vector3Scale(n, 0.042f));

        if (k % 2 == 0)
        {
            // De espaldas: bastidor de madera en cruz.
            Color madera = TinteTrazo(COLOR_MADERA_CLARA_TRAZO, c);
            VigaTrazo(Vector3Add(cara, Vector3Scale(u, -alto * 0.45f)), Vector3Add(cara, Vector3Scale(u, alto * 0.45f)), 0.07f, madera);
            VigaTrazo({ cara.x - ancho * 0.45f, cara.y, cara.z }, { cara.x + ancho * 0.45f, cara.y, cara.z }, 0.07f, madera);
        }
        else
        {
            // Pintado: cielo, horizonte y un sol.
            Color cielo = TinteTrazo(MezclaTrazo(PINTURAS_TRAZO[(semilla + k) % 6], COLOR_LIENZO_TRAZO, 0.4f), c);
            Color suelo = TinteTrazo(MezclaTrazo(PINTURAS_TRAZO[(semilla + k + 2) % 6], COLOR_LINO_TRAZO, 0.35f), c);
            Vector3 r = { ancho * 0.46f, 0.0f, 0.0f };
            Vector3 a1 = Vector3Scale(u, alto * 0.46f);
            Vector3 medio = Vector3Scale(u, -alto * 0.1f);

            rlBegin(RL_QUADS);
            QuadTrazo(
                Vector3Add(Vector3Subtract(cara, r), medio),
                Vector3Add(Vector3Add(cara, r), medio),
                Vector3Add(Vector3Add(cara, r), a1),
                Vector3Add(Vector3Subtract(cara, r), a1),
                cielo
            );
            QuadTrazo(
                Vector3Subtract(Vector3Subtract(cara, r), a1),
                Vector3Subtract(Vector3Add(cara, r), a1),
                Vector3Add(Vector3Add(cara, r), medio),
                Vector3Add(Vector3Subtract(cara, r), medio),
                suelo
            );
            rlEnd();
        }
    }
}


// Maceta de barro con una planta de hojas grandes que se mecen.
// MODELO FUTURO: maceta con planta GLB.
static void PlantaTrazo(Vector3 base, float t, int semilla)
{
    PrismaTrazo(base, 0.32f, 0.42f, 0.6f, 10, Color{ 184, 96, 64, 255 });
    PrismaTrazo({ base.x, base.y + 0.6f, base.z }, 0.45f, 0.45f, 0.08f, 10, Color{ 196, 108, 72, 255 });
    PrismaTrazo({ base.x, base.y + 0.62f, base.z }, 0.38f, 0.38f, 0.04f, 10, Color{ 70, 48, 36, 255 });

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 9; k++)
    {
        float angulo = (float)k * 2.4f + (float)semilla;
        float largo = 0.9f + 0.6f * Hash01Trazo(semilla, k);
        float alza = 0.8f + 1.0f * Hash01Trazo(semilla, k + 10);
        float mece = std::sin(t * 1.1f + (float)k) * 0.06f;
        Vector3 dir = { std::cos(angulo + mece), 0.0f, std::sin(angulo + mece) };
        Vector3 lado = { -dir.z, 0.0f, dir.x };
        Vector3 pie = { base.x, base.y + 0.66f, base.z };
        Vector3 medio = { pie.x + dir.x * largo * 0.5f, pie.y + alza, pie.z + dir.z * largo * 0.5f };
        Vector3 punta = { pie.x + dir.x * largo, pie.y + alza * 0.75f, pie.z + dir.z * largo };
        float ancho = 0.32f + 0.12f * Hash01Trazo(semilla, k + 20);

        Color hoja = MezclaTrazo(Color{ 58, 104, 58, 255 }, Color{ 96, 140, 70, 255 }, Hash01Trazo(semilla, k + 30));
        Color oscura = TonoTrazo(hoja, 0.72f);
        Vector3 izq = { medio.x + lado.x * ancho, medio.y, medio.z + lado.z * ancho };
        Vector3 der = { medio.x - lado.x * ancho, medio.y, medio.z - lado.z * ancho };

        VerticeTrazo(pie, oscura); VerticeTrazo(izq, hoja); VerticeTrazo(medio, hoja);
        VerticeTrazo(pie, oscura); VerticeTrazo(medio, hoja); VerticeTrazo(der, TonoTrazo(hoja, 0.86f));
        VerticeTrazo(izq, hoja); VerticeTrazo(punta, oscura); VerticeTrazo(medio, hoja);
        VerticeTrazo(medio, hoja); VerticeTrazo(punta, oscura); VerticeTrazo(der, TonoTrazo(hoja, 0.86f));
    }

    rlEnd();
}


// Canasto con lienzos enrollados.
static void CanastoRollosTrazo(Vector3 base, int semilla)
{
    PrismaTrazo(base, 0.42f, 0.5f, 0.7f, 10, Color{ 160, 120, 70, 255 });
    AnilloTrazo({ base.x, base.y + 0.36f, base.z }, 0.46f, 0.5f, Color{ 120, 86, 52, 255 }, 10);

    for (int k = 0; k < 5; k++)
    {
        float angulo = (float)k * 1.25f + (float)semilla;
        Vector3 pie = { base.x + std::cos(angulo) * 0.18f, base.y + 0.1f, base.z + std::sin(angulo) * 0.18f };
        Vector3 tope = { pie.x + std::cos(angulo) * 0.3f, base.y + 1.4f + 0.4f * Hash01Trazo(semilla, k), pie.z + std::sin(angulo) * 0.3f };
        CilindroEjeTrazo(pie, tope, 0.1f, 0.1f, 8, k % 2 ? COLOR_LINO_TRAZO : MezclaTrazo(COLOR_LIENZO_TRAZO, PINTURAS_TRAZO[k % 6], 0.25f));
    }
}


// Escalera de mano apoyada en el muro lateral.
static void EscaleraTrazo(float x, float z, float lado)
{
    for (int k = -1; k <= 1; k += 2)
    {
        VigaTrazo({ x - lado * 1.1f, 0.0f, z + (float)k * 0.3f }, { x, 4.6f, z + (float)k * 0.3f }, 0.08f, COLOR_MADERA_CLARA_TRAZO);
    }

    for (int p = 1; p <= 8; p++)
    {
        float u = (float)p / 9.0f;
        float xp = x - lado * 1.1f * (1.0f - u);
        VigaTrazo({ xp, 4.6f * u, z - 0.3f }, { xp, 4.6f * u, z + 0.3f }, 0.06f, COLOR_MADERA_TRAZO);
    }
}


// Bollos de papel arrugado.
static void PapelArrugadoTrazo(Vector3 base, int semilla)
{
    for (int k = 0; k < 3; k++)
    {
        float s = 0.14f + 0.06f * Hash01Trazo(semilla, k);
        CuboGiradoTrazo(
            { base.x + (Hash01Trazo(semilla, k + 3) - 0.5f) * 0.1f, base.y + s * 0.5f, base.z + (Hash01Trazo(semilla, k + 6) - 0.5f) * 0.1f },
            { s, s * 0.9f, s },
            90.0f * Hash01Trazo(semilla, k + 9),
            Color{ 240, 234, 220, 255 }
        );
    }
}


// Lampara colgante esmaltada: posicion de la pantalla con su vaiven.
static Vector3 PosicionLamparaTrazo(Vector3 cuelgue, float largo, float t, int k)
{
    float angulo = std::sin(t * 0.7f + (float)k * 2.0f) * 0.035f;
    return { cuelgue.x + std::sin(angulo) * largo, cuelgue.y - std::cos(angulo) * largo, cuelgue.z };
}


// MODELO FUTURO: lampara colgante esmaltada GLB.
static void LamparaTrazo(Vector3 cuelgue, float largo, float t, int k)
{
    Vector3 p = PosicionLamparaTrazo(cuelgue, largo, t, k);
    CilindroEjeTrazo(cuelgue, p, 0.02f, 0.02f, 4, Color{ 40, 36, 40, 255 });
    PrismaTrazo({ p.x, p.y - 0.12f, p.z }, 0.08f, 0.08f, 0.14f, 8, Color{ 54, 84, 72, 255 });
    PrismaTrazo({ p.x, p.y - 0.5f, p.z }, 0.55f, 0.12f, 0.4f, 12, Color{ 54, 92, 78, 255 });
    AnilloTrazo({ p.x, p.y - 0.5f, p.z }, 0.46f, 0.55f, Color{ 236, 228, 210, 255 }, 12);
    PrismaTrazo({ p.x, p.y - 0.62f, p.z }, 0.13f, 0.13f, 0.14f, 8, Color{ 255, 236, 190, 255 });
}


// Posiciones de la utileria segun el ancho de la fila.
struct UtileriaTrazo
{
    Vector3 mesa;
    Vector3 carrito;
    Vector3 lamparaMesa;
    Vector3 lamparaCarrito;
};


static UtileriaTrazo PosicionesUtileriaTrazo(const DisposicionTrazo& d)
{
    float a = d.mitadFila;
    UtileriaTrazo u;
    u.mesa = { -(a + 2.2f), 0.0f, -0.9f };
    u.carrito = { a + 2.6f, 0.0f, -1.3f };
    u.lamparaMesa = { u.mesa.x, ALTO_TALLER_TRAZO, u.mesa.z };
    u.lamparaCarrito = { u.carrito.x, ALTO_TALLER_TRAZO, u.carrito.z };
    return u;
}


static const float LARGO_LAMPARA_TRAZO = 4.6f;


static void DibujarUtileriaTrazo(const DisposicionTrazo& d, float t, int paso)
{
    float a = d.mitadFila;
    UtileriaTrazo u = PosicionesUtileriaTrazo(d);

    MesaTrabajoTrazo(u.mesa, 1);
    DibujarSombraRetroRectangular({ u.mesa.x, 0.0f, u.mesa.z }, 2.8f, 1.4f, 0.0f, 0.02f, 0.3f);
    CarritoTrazo(u.carrito, 4);
    DibujarSombraRetroRectangular({ u.carrito.x, 0.0f, u.carrito.z }, 1.4f, 0.9f, 0.0f, 0.02f, 0.28f);
    LamparaTrazo(u.lamparaMesa, LARGO_LAMPARA_TRAZO, t, 0);
    LamparaTrazo(u.lamparaCarrito, LARGO_LAMPARA_TRAZO - 0.3f, t, 1);

    BustoTrazo({ -(a + 0.9f), 0.0f, -3.4f });
    LienzosApiladosTrazo({ a + 1.2f, 0.0f, Z_MURO_TRAZO + 0.1f }, 7);
    TabureteTrazo({ a + 1.15f, 0.0f, 1.3f }, 2);
    DibujarSombraRetroCircular({ a + 1.15f, 0.0f, 1.3f }, 0.45f, 0.45f, 0.02f, 0.28f, 10);
    PincelesFrascoTrazo({ a + 2.3f, 0.0f, 2.3f }, 1.5f, 5);

    LataPinturaTrazo({ -(a + 0.7f), 0.0f, 2.4f }, PINTURAS_TRAZO[0], 1.0f);
    LataPinturaTrazo({ -(a + 1.35f), 0.0f, 2.9f }, PINTURAS_TRAZO[1], 1.15f);
    LataPinturaTrazo({ a + 0.45f, 0.0f, 3.0f }, PINTURAS_TRAZO[2], 0.9f);

    if (paso == 1)
    {
        PlantaTrazo({ -(a + 3.6f), 0.0f, -3.5f }, t, 3);
        CanastoRollosTrazo({ -(a + 3.4f), 0.0f, 1.3f }, 6);
        EscaleraTrazo(d.muroX - 0.05f, -2.4f, 1.0f);
        PlantaTrazo({ a + 3.8f, 0.0f, -3.6f }, t, 8);
    }

    // Papeles arrugados entre los caballetes.
    for (int k = 0; k < d.cantidad + 1; k += paso)
    {
        float x = -a + 2.0f * a * Hash01Trazo(k, 90);
        PapelArrugadoTrazo({ x, 0.03f, 2.7f + 0.5f * Hash01Trazo(k, 91) }, k + 40);
    }

    // Cosas sobre el alfeizar: macetitas y frascos contra el vidrio.
    float yA = Y_ALFEIZAR_TRAZO + 0.01f;
    float zA = Z_MURO_TRAZO - 0.05f;
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (d.ventanaX - 0.7f);
        PrismaTrazo({ x, yA, zA }, 0.16f, 0.2f, 0.28f, 8, Color{ 184, 96, 64, 255 });
        PrismaTrazo({ x, yA + 0.28f, zA }, 0.1f, 0.06f, 0.36f, 6, Color{ 84, 124, 70, 255 });
        FrascoTrazo({ x - (float)lado * 0.5f, yA, zA }, 0.09f, 0.26f, PINTURAS_TRAZO[lado < 0 ? 5 : 1]);
    }
}


//==================================================
// CABALLETES, LIENZOS Y TRAZOS
//==================================================

// Caballete de estudio: mastil, dos patas delanteras abiertas, una trasera,
// travesano, bandeja con la cinta del color del jugador y mordaza arriba.
// MODELO FUTURO: caballete de estudio GLB.
static void DibujarCaballeteTrazo(Vector3 base, Color colorJugador, float t, int semilla, bool detalle)
{
    const float M = MEDIO_LIENZO_TRAZO;
    Vector3 u = EjeArribaTrazo();
    Vector3 n = EjeFrenteTrazo();
    Vector3 r = EJE_DERECHA_TRAZO;
    Color madera = COLOR_MADERA_TRAZO;

    // Mastil detras del lienzo, del piso hasta arriba de la mordaza.
    float lzMastil = -0.16f;
    float lyPiso = -(base.y + n.y * lzMastil) / u.y;
    VigaTrazo(LocalLienzoTrazo(base, 0.0f, lyPiso, lzMastil), LocalLienzoTrazo(base, 0.0f, 2.0f * M + 0.55f, lzMastil), 0.13f, madera);

    // Patas delanteras abiertas y trasera.
    Vector3 pies[2];
    Vector3 topes[2];

    for (int s = 0; s < 2; s++)
    {
        float lado = s == 0 ? -1.0f : 1.0f;
        topes[s] = LocalLienzoTrazo(base, lado * 0.16f, 2.0f * M + 0.35f, -0.22f);
        pies[s] = { base.x + lado * 1.2f, 0.0f, base.z + 0.45f };
        VigaTrazo(pies[s], topes[s], 0.1f, madera);
    }

    VigaTrazo(LocalLienzoTrazo(base, 0.0f, 2.0f * M + 0.3f, -0.26f), { base.x, 0.0f, base.z - 1.9f }, 0.1f, TonoTrazo(madera, 0.9f));

    // Travesano entre las patas, a la altura de las rodillas.
    Vector3 travesano[2];

    for (int s = 0; s < 2; s++)
    {
        float f = 0.95f / topes[s].y;
        travesano[s] = Vector3Lerp(pies[s], topes[s], f);
    }

    VigaTrazo(travesano[0], travesano[1], 0.08f, madera);

    // Bandeja bajo el lienzo, con labio y la cinta del jugador al frente.
    CajaEjesTrazo(LocalLienzoTrazo(base, 0.0f, -0.08f, 0.1f), Vector3Scale(r, 1.1f), Vector3Scale(u, 0.06f), Vector3Scale(n, 0.22f), COLOR_MADERA_CLARA_TRAZO);
    CajaEjesTrazo(LocalLienzoTrazo(base, 0.0f, 0.0f, 0.3f), Vector3Scale(r, 1.1f), Vector3Scale(u, 0.06f), Vector3Scale(n, 0.025f), COLOR_MADERA_CLARA_TRAZO);
    CajaEjesTrazo(LocalLienzoTrazo(base, 0.0f, -0.06f, 0.33f), Vector3Scale(r, 0.95f), Vector3Scale(u, 0.05f), Vector3Scale(n, 0.012f), colorJugador);

    // Lienzo con bastidor: canto de lino (la cara se pinta aparte).
    CajaEjesTrazo(LocalLienzoTrazo(base, 0.0f, M, 0.0f), Vector3Scale(r, M), Vector3Scale(u, M), Vector3Scale(n, 0.06f), COLOR_LINO_TRAZO);

    // Mordaza superior.
    CajaEjesTrazo(LocalLienzoTrazo(base, 0.0f, 2.0f * M + 0.08f, -0.02f), Vector3Scale(r, 0.16f), Vector3Scale(u, 0.12f), Vector3Scale(n, 0.13f), madera);

    if (!detalle)
    {
        return;
    }

    // Pomos de pintura sobre la bandeja.
    for (int k = 0; k < 3; k++)
    {
        float lx = -0.85f + 0.55f * (float)k + 0.2f * Hash01Trazo(semilla, k);
        Vector3 c = LocalLienzoTrazo(base, lx, 0.04f, 0.16f);
        CuboGiradoTrazo(c, { 0.24f, 0.07f, 0.07f }, 20.0f * Hash01Trazo(semilla, k + 3) - 10.0f, Color{ 222, 222, 228, 255 });
        CuboGiradoTrazo({ c.x + 0.1f, c.y, c.z }, { 0.06f, 0.075f, 0.075f }, 0.0f, PINTURAS_TRAZO[(semilla + k) % 6]);
    }

    // Cinta del color del jugador atada a la mordaza: ondea.
    Vector3 nudo = LocalLienzoTrazo(base, 0.18f, 2.0f * M + 0.1f, 0.1f);
    rlBegin(RL_QUADS);

    for (int k = 0; k < 4; k++)
    {
        float y0 = -0.16f * (float)k;
        float y1 = y0 - 0.16f;
        float o0 = std::sin(t * 3.0f + (float)k * 0.9f + (float)semilla) * 0.05f * (float)k;
        float o1 = std::sin(t * 3.0f + (float)(k + 1) * 0.9f + (float)semilla) * 0.05f * (float)(k + 1);
        Color c = TonoTrazo(colorJugador, k % 2 ? 0.86f : 1.0f);
        QuadTrazo(
            { nudo.x + o0, nudo.y + y0, nudo.z + 0.02f },
            { nudo.x + 0.12f + o0, nudo.y + y0, nudo.z + 0.02f },
            { nudo.x + 0.12f + o1, nudo.y + y1, nudo.z + 0.02f + 0.02f * (float)k },
            { nudo.x + o1, nudo.y + y1, nudo.z + 0.02f + 0.02f * (float)k },
            c
        );
    }

    rlEnd();
}


// Banda plana sobre el lienzo entre dos puntos (dentro de RL_TRIANGLES).
static void BandaLienzoTrazo(Vector3 a, Vector3 b, float ancho, Color color)
{
    Vector3 d = Vector3Subtract(b, a);
    if (Vector3LengthSqr(d) < 0.0000001f) return;

    Vector3 p = Vector3Normalize(Vector3CrossProduct(EjeFrenteTrazo(), d));
    p = Vector3Scale(p, ancho * 0.5f);
    TrianguloQuadTrazo(Vector3Subtract(a, p), Vector3Subtract(b, p), Vector3Add(b, p), Vector3Add(a, p), color);
}


// Disco en el plano del lienzo (dentro de RL_TRIANGLES).
static void DiscoLienzoTrazo(Vector3 c, float radio, Color color, int lados = 10)
{
    Vector3 u = EjeArribaTrazo();

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        VerticeTrazo(c, color);
        VerticeTrazo({ c.x + std::cos(a0) * radio, c.y + u.y * std::sin(a0) * radio, c.z + u.z * std::sin(a0) * radio }, color);
        VerticeTrazo({ c.x + std::cos(a1) * radio, c.y + u.y * std::sin(a1) * radio, c.z + u.z * std::sin(a1) * radio }, color);
    }
}


// Cara del lienzo: imprimacion crema con un degradé suave y el borde
// apenas mas oscuro.
static void CaraLienzoTrazo(Vector3 centro)
{
    const float M = MEDIO_LIENZO_TRAZO;
    Vector3 u = EjeArribaTrazo();
    Vector3 n = Vector3Scale(EjeFrenteTrazo(), 0.062f);
    Vector3 c = Vector3Add(centro, n);
    Color arriba = COLOR_LIENZO_TRAZO;
    Color abajo = TonoTrazo(COLOR_LIENZO_TRAZO, 0.93f);
    Color borde = TonoTrazo(COLOR_LIENZO_TRAZO, 0.86f);

    auto P = [&](float lx, float ly)
    {
        return Vector3{ c.x + lx, c.y + u.y * ly, c.z + u.z * ly };
    };

    const float B = 0.1f;
    rlBegin(RL_QUADS);
    // Centro.
    VerticeTrazo(P(-M + B, -M + B), abajo);
    VerticeTrazo(P(M - B, -M + B), abajo);
    VerticeTrazo(P(M - B, M - B), arriba);
    VerticeTrazo(P(-M + B, M - B), arriba);
    // Bordes que se oscurecen hacia afuera.
    VerticeTrazo(P(-M, -M), borde); VerticeTrazo(P(M, -M), borde); VerticeTrazo(P(M - B, -M + B), abajo); VerticeTrazo(P(-M + B, -M + B), abajo);
    VerticeTrazo(P(-M + B, M - B), arriba); VerticeTrazo(P(M - B, M - B), arriba); VerticeTrazo(P(M, M), borde); VerticeTrazo(P(-M, M), borde);
    VerticeTrazo(P(-M, -M), borde); VerticeTrazo(P(-M + B, -M + B), abajo); VerticeTrazo(P(-M + B, M - B), arriba); VerticeTrazo(P(-M, M), borde);
    VerticeTrazo(P(M - B, -M + B), abajo); VerticeTrazo(P(M, -M), borde); VerticeTrazo(P(M, M), borde); VerticeTrazo(P(M - B, M - B), arriba);
    rlEnd();
}


// Boceto a carbonilla de la figura. Durante la cuenta se va dibujando
// solo, en el mismo sentido en que despues corre la estrella.
static void BocetoFiguraTrazo(Vector3 centro, const Vector2* puntos, int cantidad, float fraccion)
{
    if (puntos == nullptr || cantidad <= 0)
    {
        return;
    }

    float total = 0.0f;

    for (int k = 0; k < cantidad; k++)
    {
        total += Vector2Distance(puntos[k], puntos[(k + 1) % cantidad]);
    }

    float visible = total * Limitar01Trazo(fraccion);
    Color difuso = ConAlfaTrazo(COLOR_CARBON_TRAZO, 70.0f);

    for (int pasada = 0; pasada < 2; pasada++)
    {
        float ancho = pasada == 0 ? 0.13f : 0.06f;
        Color color = pasada == 0 ? difuso : COLOR_CARBON_TRAZO;
        float sobre = pasada == 0 ? 0.066f : 0.07f;
        float recorrido = 0.0f;

        rlBegin(RL_TRIANGLES);

        for (int k = 0; k < cantidad && recorrido < visible; k++)
        {
            Vector2 a = puntos[k];
            Vector2 b = puntos[(k + 1) % cantidad];
            float largo = Vector2Distance(a, b);
            float parte = largo > 0.0f ? Limitar01Trazo((visible - recorrido) / largo) : 1.0f;
            Vector2 fin = Vector2Lerp(a, b, parte);

            Vector3 pa = PuntoLienzoTrazo(centro, a, sobre);
            Vector3 pb = PuntoLienzoTrazo(centro, fin, sobre);
            BandaLienzoTrazo(pa, pb, ancho, color);
            DiscoLienzoTrazo(pa, ancho * 0.5f, color, 8);

            if (pasada == 1)
            {
                // Vertice marcado con un punto mas grueso.
                DiscoLienzoTrazo(PuntoLienzoTrazo(centro, a, sobre + 0.002f), 0.07f, COLOR_CARBON_TRAZO, 10);
            }

            recorrido += largo;
        }

        rlEnd();
    }
}


// Trazo del jugador: pintura espesa con contorno oscuro y un brillo
// central, de huella en huella hasta el pincel.
static void TrazoJugadorTrazo(
    Vector3 centro,
    const EstadoJugadorTrazoPerfecto& jugador,
    bool hastaPincel,
    Color color
)
{
    int cantidad = jugador.cantidadHuellas;
    if (cantidad <= 0) return;

    Color contorno = TonoTrazo(color, 0.42f);
    Color brillo = MezclaTrazo(color, Color{ 255, 255, 255, 255 }, 0.38f);

    struct Capa { float ancho; float sobre; Color color; bool juntas; };
    const Capa capas[3] =
    {
        { 0.2f, 0.074f, contorno, true },
        { 0.13f, 0.078f, color, true },
        { 0.04f, 0.082f, brillo, false }
    };

    for (int c = 0; c < 3; c++)
    {
        rlBegin(RL_TRIANGLES);

        int total = hastaPincel ? cantidad + 1 : cantidad;

        for (int h = 0; h < total; h++)
        {
            Vector2 p = h < cantidad ? jugador.huellas[h] : jugador.cursor;
            float presion = 0.9f + 0.2f * Hash01Trazo(h, 5);
            Vector3 b = PuntoLienzoTrazo(centro, p, capas[c].sobre);

            if (h > 0)
            {
                Vector3 a = PuntoLienzoTrazo(centro, jugador.huellas[h - 1], capas[c].sobre);
                BandaLienzoTrazo(a, b, capas[c].ancho * presion, capas[c].color);
            }

            if (capas[c].juntas)
            {
                DiscoLienzoTrazo(b, capas[c].ancho * 0.5f * presion, capas[c].color, 8);
            }
        }

        rlEnd();
    }
}


// Estrella de cinco puntas en el plano del lienzo.
static void EstrellaLienzoTrazo(Vector3 centro, float radio, float giro, Color color)
{
    Vector3 u = EjeArribaTrazo();
    const float interior = radio * 0.46f;
    Vector3 v[10];

    for (int k = 0; k < 10; k++)
    {
        float angulo = giro + (float)k * PI / 5.0f + PI * 0.5f;
        float rr = k % 2 == 0 ? radio : interior;
        float lx = std::cos(angulo) * rr;
        float ly = std::sin(angulo) * rr;
        v[k] = { centro.x + lx, centro.y + u.y * ly, centro.z + u.z * ly };
    }

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 10; k++)
    {
        VerticeTrazo(centro, color);
        VerticeTrazo(v[k], color);
        VerticeTrazo(v[(k + 1) % 10], color);
    }

    rlEnd();
}


// Pincel: punta cargada del color del jugador sobre el cursor, virola y
// cabo que sale hacia la camara.
// MODELO FUTURO: pincel GLB.
static void PincelTrazo(Vector3 punta, Color color)
{
    Vector3 u = EjeArribaTrazo();
    Vector3 n = EjeFrenteTrazo();
    Vector3 dir = Vector3Normalize(Vector3Add(Vector3Add(n, Vector3Scale(EJE_DERECHA_TRAZO, 0.45f)), Vector3Scale(u, -0.35f)));

    Vector3 p1 = Vector3Add(punta, Vector3Scale(dir, 0.26f));
    Vector3 p2 = Vector3Add(punta, Vector3Scale(dir, 0.38f));
    Vector3 p3 = Vector3Add(punta, Vector3Scale(dir, 1.2f));

    CilindroEjeTrazo(punta, p1, 0.015f, 0.08f, 8, color);
    CilindroEjeTrazo(p1, p2, 0.08f, 0.075f, 8, Color{ 206, 206, 214, 255 });
    CilindroEjeTrazo(p2, p3, 0.065f, 0.045f, 8, TonoTrazo(color, 0.7f));
    CilindroEjeTrazo(Vector3Lerp(p2, p3, 0.55f), Vector3Lerp(p2, p3, 0.62f), 0.06f, 0.058f, 8, Color{ 240, 228, 200, 255 });
}


// Escarapela dorada del ganador, clavada en la esquina del lienzo.
static void EscarapelaTrazo(Vector3 base, float escala, Color colorJugador)
{
    if (escala <= 0.01f) return;

    const float M = MEDIO_LIENZO_TRAZO;
    Vector3 n = EjeFrenteTrazo();
    Vector3 c = LocalLienzoTrazo(base, M - 0.45f, 2.0f * M - 0.45f, 0.14f);

    // Cintas que cuelgan.
    Vector3 u = EjeArribaTrazo();
    rlBegin(RL_TRIANGLES);
    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 a = { c.x + 0.05f * (float)s * escala, c.y, c.z };
        Vector3 b = { c.x + (0.12f + 0.12f * (float)s) * escala, c.y - u.y * 0.6f * escala, c.z - u.z * 0.6f * escala };
        BandaLienzoTrazo(a, b, 0.16f * escala, s < 0 ? colorJugador : Color{ 255, 198, 60, 255 });
    }
    rlEnd();

    CilindroEjeTrazo(c, Vector3Add(c, Vector3Scale(n, 0.05f)), 0.3f * escala, 0.3f * escala, 16, Color{ 255, 196, 52, 255 });
    CilindroEjeTrazo(Vector3Add(c, Vector3Scale(n, 0.05f)), Vector3Add(c, Vector3Scale(n, 0.07f)), 0.18f * escala, 0.18f * escala, 16, colorJugador);
    EstrellaLienzoTrazo(Vector3Add(c, Vector3Scale(n, 0.075f)), 0.13f * escala, 0.0f, Color{ 255, 248, 220, 255 });
}


// Estrella objetivo con contorno oscuro (se lee sobre el lienzo crema).
static Vector3 PosicionEstrellaTrazo(Vector3 centro, Vector2 objetivo)
{
    return PuntoLienzoTrazo(centro, objetivo, 0.13f);
}


static void DibujarLienzoJugadorTrazo(
    const MinijuegoTrazoPerfecto& m,
    const Participante participantes[],
    int i,
    const DisposicionTrazo& d,
    const Vector2* puntos,
    int cantidadPuntos,
    bool decoracion
)
{
    float t = m.tiempoAnimacion;
    int orden = OrdenParticipanteTrazo(m, i);
    Vector3 base = BaseLienzoTrazo(d, orden);
    Vector3 centro = CentroLienzoTrazo(d, orden);
    Color color = participantes[i].color;
    const EstadoJugadorTrazoPerfecto& j = m.jugadores[i];

    DibujarCaballeteTrazo(base, color, t, i * 5 + 1, decoracion);
    DibujarSombraRetroRectangular({ base.x, 0.0f, base.z - 0.6f }, 2.5f, 2.3f, 0.0f, 0.05f, 0.26f);
    CaraLienzoTrazo(centro);

    float fraccion = 1.0f;

    if (m.fase == FASE_TRAZO_PREPARACION)
    {
        fraccion = (DURACION_PREPARACION_TRAZO - m.tiempoPreparacion - 0.2f) / 1.6f;
    }

    BocetoFiguraTrazo(centro, puntos, cantidadPuntos, fraccion);
    TrazoJugadorTrazo(centro, j, m.fase == FASE_TRAZO_DIBUJANDO, color);

    Vector2 objetivo = PuntoRecorridoTrazo(m.forma, m.progresoObjetivo);
    float sacudida = std::sin(t * 60.0f) * 0.05f * (j.sacudida / 0.4f);
    Vector2 cursor = { j.cursor.x + sacudida, j.cursor.y };
    Vector3 punta = PuntoLienzoTrazo(centro, cursor, 0.09f);

    // Guia entre el pincel y la estrella: verde si va bien, roja si no.
    if (m.fase == FASE_TRAZO_DIBUJANDO)
    {
        float distancia = Vector2Distance(j.cursor, objetivo);
        float calidad = 1.0f - Limitar01Trazo(distancia / 0.42f);
        Color guia = MezclaTrazo(Color{ 230, 64, 52, 255 }, Color{ 60, 170, 70, 255 }, calidad);

        if (distancia > 0.08f)
        {
            rlBegin(RL_TRIANGLES);
            BandaLienzoTrazo(PuntoLienzoTrazo(centro, j.cursor, 0.09f), PuntoLienzoTrazo(centro, objetivo, 0.09f), 0.035f, guia);
            rlEnd();
        }
    }

    // Estrella que hay que seguir (en el fin se queda en el cierre).
    if (m.fase != FASE_TRAZO_TERMINADO)
    {
        Vector3 estrella = PosicionEstrellaTrazo(centro, objetivo);
        float latido = 1.0f + std::sin(t * 6.0f) * 0.08f;
        float giro = t * 1.2f;
        Vector3 n = EjeFrenteTrazo();
        EstrellaLienzoTrazo(estrella, 0.4f * latido, giro, Color{ 52, 30, 12, 255 });
        EstrellaLienzoTrazo(Vector3Add(estrella, Vector3Scale(n, 0.01f)), 0.32f * latido, giro, COLOR_ACENTO_TRAZO);
        EstrellaLienzoTrazo(Vector3Add(estrella, Vector3Scale(n, 0.02f)), 0.15f * latido, giro, Color{ 255, 250, 210, 255 });
    }

    PincelTrazo(punta, color);

    // Escarapela del ganador.
    if (m.fase == FASE_TRAZO_TERMINADO && m.resultado.participantes[i].posicionFinal == 1)
    {
        float u = Limitar01Trazo((t - DURACION_PREPARACION_TRAZO - DURACION_TRAZO_PERFECTO) * 3.0f);
        EscarapelaTrazo(base, 1.0f + 0.25f * std::sin(u * 3.1416f), color);
    }
}


//==================================================
// PINTORES, TRANSLUCIDOS Y PARTICULAS
//==================================================

// Pies del pintor: frente a su caballete, se corre un poco hacia donde
// va el pincel y salta al acertar una esquina.
static Vector3 PiesPintorTrazo(const MinijuegoTrazoPerfecto& m, int i, const DisposicionTrazo& d)
{
    const EstadoJugadorTrazoPerfecto& j = m.jugadores[i];
    Vector3 base = BaseLienzoTrazo(d, OrdenParticipanteTrazo(m, i));
    float salto = j.salto > 0.0f ? std::sin((1.0f - j.salto / 0.35f) * 3.1416f) * 0.32f : 0.0f;
    return { base.x + j.cursor.x * 0.45f, salto, Z_PINTOR_TRAZO };
}


static void DibujarPintoresTrazo(const MinijuegoTrazoPerfecto& m, const Participante participantes[], const DisposicionTrazo& d)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 pies = PiesPintorTrazo(m, i, d);
        DibujarSombraRetroCircular({ pies.x, 0.0f, pies.z }, 0.42f, 0.42f, 0.05f, 0.32f, 10);

        // De espaldas, mirando su lienzo; al terminar se dan vuelta.
        float angulo = m.fase == FASE_TRAZO_TERMINADO ? 0.0f : 180.0f - m.jugadores[i].cursor.x * 14.0f;
        DibujarModeloJugadorEnPosicion(pies, angulo, participantes[i].color);
    }
}


static void DibujarParticulasTrazo(const MinijuegoTrazoPerfecto& m)
{
    for (int i = 0; i < MAX_PARTICULAS_TRAZO; i++)
    {
        const ParticulaTrazoPerfecto& p = m.particulas[i];

        if (!p.activa || p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        float s = p.tamano * (0.5f + 0.5f * Limitar01Trazo(vida * 2.0f));
        CuboTrazo(p.posicion, { s, s, s }, p.color);
    }
}


// Manchas que dejaron las gotas durante la partida.
static void DibujarManchasTrazo(const MinijuegoTrazoPerfecto& m)
{
    for (int i = 0; i < MAX_MANCHAS_TRAZO; i++)
    {
        const ManchaTrazoPerfecto& mancha = m.manchas[i];

        if (!mancha.activa)
        {
            continue;
        }

        Vector3 p = { mancha.posicion.x, 0.05f + 0.0004f * (float)i, mancha.posicion.z };
        SalpicaduraTrazo(p, mancha.radio, mancha.forma, TonoTrazo(mancha.color, 0.9f));
    }
}


// Vidrios del ventanal (reflejos) y anillos de cada pintor, alfa normal.
static void DibujarTranslucidosTrazo(const MinijuegoTrazoPerfecto& m, const Participante participantes[], const DisposicionTrazo& d)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Vidrio: un velo calido y dos reflejos en diagonal por paño.
    float zv = Z_MURO_TRAZO - 0.36f;
    float vx = d.ventanaX;
    int columnas = (int)(2.0f * vx / 1.7f + 0.5f);
    if (columnas < 2) columnas = 2;

    rlBegin(RL_QUADS);
    QuadTrazo({ -vx, Y_ALFEIZAR_TRAZO, zv }, { vx, Y_ALFEIZAR_TRAZO, zv }, { vx, Y_DINTEL_TRAZO, zv }, { -vx, Y_DINTEL_TRAZO, zv }, Color{ 255, 226, 200, 26 });

    for (int c = 0; c < columnas; c++)
    {
        float x0 = -vx + 2.0f * vx * (float)c / (float)columnas;
        float ancho = 2.0f * vx / (float)columnas;

        for (int f = 0; f < 2; f++)
        {
            float y = 3.0f + 3.6f * (float)f + 0.6f * Hash01Trazo(c, f);
            float desvio = ancho * 0.5f;
            QuadTrazo(
                { x0 + ancho * 0.2f, y, zv + 0.01f },
                { x0 + ancho * 0.36f, y, zv + 0.01f },
                { x0 + ancho * 0.36f + desvio, y + 1.0f, zv + 0.01f },
                { x0 + ancho * 0.2f + desvio, y + 1.0f, zv + 0.01f },
                Color{ 255, 244, 230, 34 }
            );
        }
    }

    rlEnd();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 pies = PiesPintorTrazo(m, i, d);
        AnilloTrazo({ pies.x, 0.06f, pies.z }, 0.56f, 0.7f, ConAlfaTrazo(participantes[i].color, 200.0f), 28);
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// LUCES FALSAS
//==================================================

// Direccion del sol que entra por el ventanal (por unidad de z hacia la
// camara baja 0.62 y se corre 0.18 a la izquierda).
static const float CAIDA_SOL_TRAZO = 0.62f;
static const float DERIVA_SOL_TRAZO = -0.18f;
static const float Y_TOPE_HAZ_TRAZO = 4.6f;


static Vector3 ProyectarSolPisoTrazo(float x, float y)
{
    float avance = y / CAIDA_SOL_TRAZO;
    return { x + DERIVA_SOL_TRAZO * avance, 0.07f, Z_MURO_TRAZO - 0.35f + avance };
}


static void DibujarLucesTrazo(const MinijuegoTrazoPerfecto& m, const Participante participantes[], const DisposicionTrazo& d, bool decoracion)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = m.tiempoAnimacion;
    float vx = d.ventanaX;
    int columnas = (int)(2.0f * vx / 1.7f + 0.5f);
    if (columnas < 2) columnas = 2;
    float zv = Z_MURO_TRAZO - 0.35f;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Resplandor del sol detras del ventanal.
    ResplandorTrazo({ vx * 0.4f, 3.6f, zv - 0.2f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 7.5f, Color{ 255, 150, 80, 60 }, 1.3f);

    // Haces de sol: una lamina por columna de paños, del vidrio al piso,
    // y el charco de luz de cada paño bajo en las tablas.
    float vaiven = 0.85f + 0.15f * std::sin(t * 0.4f);

    for (int c = 0; c < columnas; c++)
    {
        float x0 = -vx + 2.0f * vx * (float)c / (float)columnas + 0.06f;
        float x1 = -vx + 2.0f * vx * (float)(c + 1) / (float)columnas - 0.06f;
        float y0 = Y_ALFEIZAR_TRAZO + 0.1f;
        float y1 = Y_TOPE_HAZ_TRAZO;

        Vector3 a0 = { x0, y1, zv };
        Vector3 a1 = { x1, y1, zv };
        Vector3 b0 = ProyectarSolPisoTrazo(x0, y1);
        Vector3 b1 = ProyectarSolPisoTrazo(x1, y1);
        unsigned char alfa = (unsigned char)(22.0f * vaiven);

        rlBegin(RL_QUADS);
        rlColor4ub(255, 170, 96, alfa); rlVertex3f(a0.x, a0.y, a0.z);
        rlColor4ub(255, 170, 96, alfa); rlVertex3f(a1.x, a1.y, a1.z);
        rlColor4ub(255, 150, 90, 0); rlVertex3f(b1.x, b1.y, b1.z);
        rlColor4ub(255, 150, 90, 0); rlVertex3f(b0.x, b0.y, b0.z);
        rlEnd();

        // Charco: el paño proyectado en el piso.
        Vector3 p00 = ProyectarSolPisoTrazo(x0, y0);
        Vector3 p10 = ProyectarSolPisoTrazo(x1, y0);
        Vector3 p11 = ProyectarSolPisoTrazo(x1, y1);
        Vector3 p01 = ProyectarSolPisoTrazo(x0, y1);
        Color charco = Color{ 255, 160, 90, (unsigned char)(34.0f * vaiven) };
        Color lejos = Color{ 255, 160, 90, (unsigned char)(10.0f * vaiven) };

        rlBegin(RL_QUADS);
        VerticeTrazo(p00, charco);
        VerticeTrazo(p10, charco);
        VerticeTrazo(p11, lejos);
        VerticeTrazo(p01, lejos);
        rlEnd();
    }

    if (decoracion)
    {
        // Lamparas colgantes: halo, cono y charco sobre la mesa y el carrito.
        UtileriaTrazo u = PosicionesUtileriaTrazo(d);
        const Vector3 cuelgues[2] = { u.lamparaMesa, u.lamparaCarrito };
        const float largos[2] = { LARGO_LAMPARA_TRAZO, LARGO_LAMPARA_TRAZO - 0.3f };
        const float apoyo[2] = { 1.03f, 1.15f };

        for (int k = 0; k < 2; k++)
        {
            Vector3 p = PosicionLamparaTrazo(cuelgues[k], largos[k], t, k);
            Vector3 foco = { p.x, p.y - 0.62f, p.z };
            float pulso = 0.94f + 0.06f * std::sin(t * 7.0f + (float)k * 3.0f);

            ResplandorFrenteTrazo(foco, 1.2f * pulso, Color{ 255, 190, 110, 90 });
            ResplandorFrenteTrazo(foco, 0.45f * pulso, Color{ 255, 236, 190, 150 });
            DrawCylinderEx({ foco.x, foco.y - 0.05f, foco.z }, { foco.x, apoyo[k], foco.z }, 0.5f, 1.4f, 16, Color{ 255, 200, 130, 14 });

            for (int r = 0; r < 3; r++)
            {
                ResplandorSueloTrazo({ foco.x, apoyo[k] + 0.01f + 0.002f * (float)r, foco.z }, (0.9f + 0.6f * (float)r) * pulso, Color{ 255, 190, 110, 38 });
                ResplandorSueloTrazo({ foco.x, 0.08f + 0.002f * (float)r, foco.z + 0.6f }, (1.4f + 0.8f * (float)r) * pulso, Color{ 255, 180, 100, 18 });
            }
        }
    }

    // Estrellas: halo dorado sobre cada lienzo; pinceles: brillo del color.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorTrazoPerfecto& j = m.jugadores[i];
        Vector3 centro = CentroLienzoTrazo(d, OrdenParticipanteTrazo(m, i));
        Vector3 u = EjeArribaTrazo();
        Color color = participantes[i].color;

        if (m.fase != FASE_TRAZO_TERMINADO)
        {
            Vector2 objetivo = PuntoRecorridoTrazo(m.forma, m.progresoObjetivo);
            Vector3 estrella = PuntoLienzoTrazo(centro, objetivo, 0.16f);
            float pulso = 0.85f + 0.15f * std::sin(t * 6.0f);
            ResplandorTrazo(estrella, EJE_DERECHA_TRAZO, u, 0.7f * pulso, Color{ 255, 190, 60, 46 });
        }

        ResplandorTrazo(PuntoLienzoTrazo(centro, j.cursor, 0.12f), EJE_DERECHA_TRAZO, u, 0.28f, ConAlfaTrazo(color, 70.0f));

        if (j.destello > 0.0f)
        {
            float f = j.destello / 0.6f;
            ResplandorTrazo(PuntoLienzoTrazo(centro, j.cursor, 0.12f), EJE_DERECHA_TRAZO, u, 0.5f + (1.0f - f) * 0.9f, ConAlfaTrazo(color, 150.0f * f));
        }
    }

    // Chispas de los aciertos.
    for (int i = 0; i < MAX_PARTICULAS_TRAZO; i++)
    {
        const ParticulaTrazoPerfecto& p = m.particulas[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteTrazo(p.posicion, p.tamano * 2.0f, ConAlfaTrazo(p.color, 230.0f * vida));
    }

    // Polvo que flota en los haces de sol.
    if (decoracion)
    {
        int motas = (int)(44.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < motas; k++)
        {
            float ciclo = 12.0f + 8.0f * Hash01Trazo(k, 220);
            float u = std::fmod(t / ciclo + Hash01Trazo(k, 221), 1.0f);
            float y = 0.4f + (Y_TOPE_HAZ_TRAZO - 0.4f) * Hash01Trazo(k, 222);
            Vector3 suelo = ProyectarSolPisoTrazo(-vx + 2.0f * vx * Hash01Trazo(k, 223), y);
            Vector3 p =
            {
                suelo.x + std::sin(t * 0.4f + (float)k) * 0.4f,
                y * (1.0f - u) + 0.3f + std::sin(t * 0.7f + (float)k * 1.3f) * 0.2f,
                Z_MURO_TRAZO + (suelo.z - Z_MURO_TRAZO) * u
            };
            float alfa = std::sin(u * 3.1416f);
            ResplandorFrenteTrazo(p, 0.05f, Color{ 255, 220, 170, (unsigned char)(180.0f * alfa) });
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJAR
//==================================================

static void DibujarEscenarioTrazo(const MinijuegoTrazoPerfecto& m, const DisposicionTrazo& d)
{
    float t = m.tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    DibujarCiudadTrazo(t, decoracion ? paso : 2, decoracion);
    DibujarMuroVentanalTrazo(d, decoracion);
    DibujarMurosLateralesTrazo(d, decoracion);
    DibujarPisoTrazo(d, paso);
    DibujarLonaTrazo(d, paso);

    if (!decoracion)
    {
        return;
    }

    DibujarSalpicadurasTrazo(d, paso);
    DibujarCortinasTrazo(d, t);
    DibujarUtileriaTrazo(d, t, paso);
}


static const char* ObjetivoHudTrazo(FormaTrazoPerfecto forma)
{
    switch (forma)
    {
        case FORMA_TRAZO_CRISTAL: return "SEGUÍ LA ESTRELLA DORADA CON TU PINCEL: PINTÁ EL CRISTAL";
        case FORMA_TRAZO_RAYO: return "SEGUÍ LA ESTRELLA DORADA CON TU PINCEL: PINTÁ EL RAYO";
        case FORMA_TRAZO_COCHETE: return "SEGUÍ LA ESTRELLA DORADA CON TU PINCEL: PINTÁ EL COHETE";
        case CANTIDAD_FORMAS_TRAZO: break;
    }

    return "SEGUÍ LA ESTRELLA DORADA CON TU PINCEL";
}


void MinijuegoTrazoPerfecto::Dibujar(
    const Participante participantes[]
) const
{

    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    DisposicionTrazo d = CalcularDisposicionTrazo(CantidadParticipantesTrazo(*this));

    int cantidadPuntos = 0;
    const Vector2* puntos = ObtenerPuntosTrazo(forma, cantidadPuntos);

    // Entrada: durante la cuenta la camara se acerca desde el fondo del taller.
    Camera3D vista = CamaraTrazo(d);

    if (fase == FASE_TRAZO_PREPARACION)
    {
        float u = Limitar01Trazo(tiempoPreparacion / DURACION_PREPARACION_TRAZO);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        Vector3 atras = Vector3Subtract(vista.position, vista.target);
        vista.position = Vector3Add(vista.target, Vector3Scale(atras, 1.0f + 0.35f * suave));
        vista.position.y += 1.6f * suave;
    }

    ClearBackground(COLOR_PENUMBRA_TRAZO);
    DibujarCieloTrazo(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarEscenarioTrazo(*this, d);
    DibujarManchasTrazo(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarLienzoJugadorTrazo(*this, participantes, i, d, puntos, cantidadPuntos, decoracion);
        }
    }

    DibujarPintoresTrazo(*this, participantes, d);
    DibujarParticulasTrazo(*this);
    DibujarTranslucidosTrazo(*this, participantes, d);
    DibujarLucesTrazo(*this, participantes, d, decoracion);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre cada pintor y los avisos de las esquinas.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo)
        {
            continue;
        }

        const EstadoJugadorTrazoPerfecto& j = jugadores[i];
        Vector3 pies = PiesPintorTrazo(*this, i, d);
        Vector2 p = GetWorldToScreen({ pies.x, pies.y + 2.25f, pies.z }, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);

        if (j.textoFlotante > 0.0f && j.tipoTexto != TEXTO_TRAZO_NINGUNO)
        {
            Vector3 centro = CentroLienzoTrazo(d, OrdenParticipanteTrazo(*this, i));
            Vector2 s = GetWorldToScreen(PuntoLienzoTrazo(centro, j.cursor, 0.2f), vista);
            float u = 1.0f - j.textoFlotante / DURACION_TEXTO_TRAZO;
            float alfa = Limitar01Trazo(j.textoFlotante / 0.3f);
            const char* texto = "¡BIEN!";
            Color color = Color{ 170, 250, 170, 255 };

            if (j.tipoTexto == TEXTO_TRAZO_JUSTO)
            {
                texto = "¡JUSTO!";
                color = Color{ 255, 214, 90, 255 };
            }
            else if (j.tipoTexto == TEXTO_TRAZO_UY)
            {
                texto = "¡UY!";
                color = Color{ 210, 206, 214, 255 };
            }

            DibujarTextoMundoContornoUI(texto, s.x, s.y - Px(26.0f) - u * Px(30.0f), UI_TEXTO, color, alfa);
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(hud, "TRAZO PERFECTO", ObjetivoHudTrazo(forma), resultado);

    hud.fase = fase == FASE_TRAZO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_TRAZO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = fase == FASE_TRAZO_PREPARACION ? DURACION_TRAZO_PERFECTO : tiempoTrazo;
    hud.tiempoTotal = DURACION_TRAZO_PERFECTO;
    hud.segundosAlerta = 5.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        if (!h.visible) continue;

        const EstadoJugadorTrazoPerfecto& j = jugadores[i];
        int precision = fase == FASE_TRAZO_TERMINADO ? j.puntuacionFinal : CalcularPuntuacionTrazo(j);

        std::snprintf(h.valor, sizeof(h.valor), "%d %%", precision);
        h.valorNumerico = j.aciertos;
        h.progreso = Limitar01Trazo((float)precision / 100.0f);

        if (fase == FASE_TRAZO_DIBUJANDO && j.fueraDeLinea > 0.5f)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡TE DESVIÁS!");
        }

        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d %% DE PRECISIÓN", precision);

        if (EsHumanoTrazo(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "PINCEL: %s\nSEGUÍ LA ESTRELLA",
                TextoMoverHud(participantes[i])
            );
        }
    }

    if (fase == FASE_TRAZO_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        hud.finPorTiempo = true;
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoTrazoPerfecto::ObtenerResultado() const
{
    return resultado;
}
