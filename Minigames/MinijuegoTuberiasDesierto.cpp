#include "Minigames/MinijuegoTuberiasDesierto.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
#include <cstdio>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_TUBERIAS = 3.0f;
static const float DURACION_OBSERVAR = 5.0f;
static const float DURACION_ELEGIR = 8.0f;
static const float DURACION_COMPUERTA = 1.0f;
static const float DURACION_FLUJO = 3.5f;
static const float DURACION_REVELAR = 3.0f;

static const int PUNTOS_ORO = 3;
static const int PUNTOS_MIRAJE = -1;

static const int ENTRADAS_POR_RONDA[RONDAS_TUBERIAS] = { 4, 4, 5, 5, 6 };
static const int ETAPAS_POR_RONDA[RONDAS_TUBERIAS] = { 3, 4, 5, 6, 7 };
static const int PROBABILIDAD_SEGUNDO_CRUCE[RONDAS_TUBERIAS] = { 0, 20, 40, 55, 70 };

// Geometria de la pared (el mundo mira hacia -Z; la camara esta en +Z).
static const float Y_ENTRADA = 12.8f;
static const float Y_INICIO = 12.2f;
static const float Y_FIN = 3.6f;
static const float Z_TUBERIA = -2.3f;
static const float Y_REPISA = 1.8f;
static const float Z_PLATAFORMA_JUGADORES = 2.8f;
static const float ALTURA_CENTRO_JUGADOR = 0.7f;


//==================================================
// UTILIDADES
//==================================================

static float Acotar(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float Aleatorio01()
{
    return (float)GetRandomValue(0, 10000) / 10000.0f;
}


static bool JugadorEsBot(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static unsigned int HashIndice(int k)
{
    return (((unsigned int)k * 2654435761u) >> 8) & 0xFFu;
}


static float SeparacionCarriles(int cantidad)
{
    if (cantidad <= 4) return 3.4f;
    if (cantidad == 5) return 3.0f;

    return 2.7f;
}


static float XCarril(int cantidad, int carril)
{
    return ((float)carril - 0.5f * (float)(cantidad - 1)) * SeparacionCarriles(cantidad);
}


static float AlturaEtapa(const MinijuegoTuberiasDesierto& m)
{
    return (Y_INICIO - Y_FIN) / (float)m.etapas;
}


static float ProbabilidadAcierto(int ronda)
{
    return 0.70f - 0.30f * (float)ronda / (float)(RONDAS_TUBERIAS - 1);
}


//==================================================
// DATOS LOGICOS: ENTRAMADO
//==================================================

static int AplicarCruce(int carril, int a, int b)
{
    if (a >= 0)
    {
        if (carril == a) return a + 1;
        if (carril == a + 1) return a;
    }

    if (b >= 0)
    {
        if (carril == b) return b + 1;
        if (carril == b + 1) return b;
    }

    return carril;
}


static void GenerarEntramado(MinijuegoTuberiasDesierto& m)
{
    int n = ENTRADAS_POR_RONDA[m.ronda];
    int etapas = ETAPAS_POR_RONDA[m.ronda];

    m.cantidadEntradas = n;
    m.etapas = etapas;

    for (int intento = 0; intento < 20; intento++)
    {
        int previo = -1;

        for (int e = 0; e < n; e++)
        {
            m.carril[e][0] = e;
        }

        for (int s = 0; s < etapas; s++)
        {
            int a = GetRandomValue(0, n - 2);

            for (int k = 0; k < 6 && a == previo; k++)
            {
                a = GetRandomValue(0, n - 2);
            }

            int b = -1;

            if (GetRandomValue(0, 99) < PROBABILIDAD_SEGUNDO_CRUCE[m.ronda])
            {
                for (int k = 0; k < 6; k++)
                {
                    int candidato = GetRandomValue(0, n - 2);

                    if (candidato - a >= 2 || a - candidato >= 2)
                    {
                        b = candidato;
                        break;
                    }
                }
            }

            for (int e = 0; e < n; e++)
            {
                m.carril[e][s + 1] = AplicarCruce(m.carril[e][s], a, b);
            }

            previo = a;
        }

        bool identidad = true;

        for (int e = 0; e < n; e++)
        {
            if (m.carril[e][etapas] != e)
            {
                identidad = false;
            }
        }

        if (!identidad)
        {
            break;
        }
    }

    m.oroCantaro = GetRandomValue(0, n - 1);
    m.mirajeCantaro = (m.oroCantaro + 1 + GetRandomValue(0, n - 2)) % n;

    for (int e = 0; e < n; e++)
    {
        if (m.carril[e][etapas] == m.oroCantaro)
        {
            m.entradaGanadora = e;
        }
    }
}


//==================================================
// RONDAS Y FASES
//==================================================

static void CambiarFase(MinijuegoTuberiasDesierto& m, FaseTuberiasDesierto fase)
{
    m.fase = fase;
    m.tiempoFase = 0.0f;

    if (fase == FASE_TUBERIAS_ELEGIR)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_INICIO_MINIJUEGO);

        // Los bots deciden entre 1 y 4 s; aciertan menos en rondas avanzadas.
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            EstadoJugadorTuberias& estado = m.estadosJugadores[i];

            if (!estado.participa)
            {
                continue;
            }

            bool acierta = Aleatorio01() < ProbabilidadAcierto(m.ronda);
            int objetivo = m.entradaGanadora;

            if (!acierta)
            {
                objetivo = GetRandomValue(0, m.cantidadEntradas - 2);

                if (objetivo >= m.entradaGanadora)
                {
                    objetivo++;
                }
            }

            estado.objetivo = objetivo;
            estado.tiempoDecision = 1.0f + 3.0f * Aleatorio01();
            estado.tiempoPaso = 0.3f + 0.5f * Aleatorio01();
        }
    }
    else if (fase == FASE_TUBERIAS_COMPUERTA)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
    }
}


static void IniciarRonda(MinijuegoTuberiasDesierto& m)
{
    GenerarEntramado(m);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorTuberias& estado = m.estadosJugadores[i];

        estado.seleccion = m.cantidadEntradas / 2;
        estado.bloqueado = false;
        estado.deltaRonda = 0;
    }

    m.progresoAgua = 0.0f;
    m.compuerta = 0.0f;
    m.mirajeRevelado = false;
    CambiarFase(m, FASE_TUBERIAS_OBSERVAR);
}


static void ResolverRonda(MinijuegoTuberiasDesierto& m)
{
    bool hayAcierto = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorTuberias& estado = m.estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        int cantaro = m.carril[estado.seleccion][m.etapas];

        estado.deltaRonda = 0;
        estado.aciertoUltima = false;

        if (cantaro == m.oroCantaro)
        {
            estado.deltaRonda = PUNTOS_ORO;
            estado.aciertoUltima = true;
            hayAcierto = true;
        }
        else if (cantaro == m.mirajeCantaro)
        {
            estado.deltaRonda = PUNTOS_MIRAJE;
        }

        estado.puntos += estado.deltaRonda;
    }

    m.mirajeRevelado = true;
    ReproducirSonidoMinijuego(m.audio, hayAcierto ? SONIDO_ACIERTO : SONIDO_ERROR);
}


static int PuntajeFinal(const EstadoJugadorTuberias& estado)
{
    return estado.puntos * 2 + (estado.aciertoUltima ? 1 : 0);
}


static void FinalizarTuberias(MinijuegoTuberiasDesierto& m)
{
    int ganadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorTuberias& estado = m.estadosJugadores[i];
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                j != i &&
                m.resultado.participantes[j].participo &&
                PuntajeFinal(m.estadosJugadores[j]) > PuntajeFinal(estado)
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego = estado.puntos;
        resultadoJugador.puntosObtenidos = 0;

        if (posicion == 1)
        {
            ganadores++;
        }
    }

    m.empate = ganadores > 1;
    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = m.empate ? DESENLACE_EMPATE : DESENLACE_CON_GANADOR;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
    m.fase = FASE_TUBERIAS_TERMINADO;
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

void MinijuegoTuberiasDesierto::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    ronda = 0;
    cantidadEntradas = ENTRADAS_POR_RONDA[0];
    etapas = ETAPAS_POR_RONDA[0];
    progresoAgua = 0.0f;
    compuerta = 0.0f;
    mirajeRevelado = false;
    empate = false;
    partidaValida = false;

    camara.position = { 0.0f, 10.5f, 27.0f };
    camara.target = { 0.0f, 7.4f, -2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_TUBERIAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_TUBERIAS;
    tiempoFase = 0.0f;
    tiempoAnimacion = 0.0f;

    GenerarEntramado(*this);
}


void MinijuegoTuberiasDesierto::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            estadosJugadores[i].participa = true;
            estadosJugadores[i].seleccion = cantidadEntradas / 2;
            cantidad++;
        }
    }

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_TUBERIAS_TERMINADO;
        return;
    }

    partidaValida = true;

    int orden = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        float x = ((float)orden - 0.5f * (float)(cantidad - 1)) * 1.2f;
        Vector3 spawn = { x, ALTURA_CENTRO_JUGADOR, Z_PLATAFORMA_JUGADORES };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].posicion = spawn;
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugadores[i].enSuelo = true;
        jugadores[i].cayendo = false;
        orden++;
    }
}


//==================================================
// ACTUALIZACION
//==================================================

static void ActualizarSeleccionJugador(
    MinijuegoTuberiasDesierto& m,
    EstadoJugadorTuberias& estado,
    const Participante& participante,
    float deltaTime
)
{
    if (estado.bloqueado)
    {
        return;
    }

    if (JugadorEsBot(participante))
    {
        if (m.tiempoFase >= estado.tiempoDecision)
        {
            estado.seleccion = estado.objetivo;
            estado.bloqueado = true;
            return;
        }

        estado.tiempoPaso -= deltaTime;

        if (estado.tiempoPaso <= 0.0f && estado.seleccion != estado.objetivo)
        {
            estado.seleccion += estado.objetivo > estado.seleccion ? 1 : -1;
            estado.tiempoPaso = 0.25f + 0.2f * Aleatorio01();
        }

        return;
    }

    InputMinijuegoParticipante entrada = LeerInputMinijuegoParticipante(participante);

    if (entrada.izquierda && !estado.izquierdaPrevia && estado.seleccion > 0)
    {
        estado.seleccion--;
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
    }

    if (entrada.derecha && !estado.derechaPrevia && estado.seleccion < m.cantidadEntradas - 1)
    {
        estado.seleccion++;
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
    }

    estado.izquierdaPrevia = entrada.izquierda;
    estado.derechaPrevia = entrada.derecha;

    if (entrada.golpear)
    {
        estado.bloqueado = true;
        ReproducirSonidoMinijuego(m.audio, SONIDO_UI_CONFIRMAR);
    }
}


// Los jugadores caminan sobre la plataforma hasta quedar frente a su entrada.
static void ColocarJugadores(
    MinijuegoTuberiasDesierto& m,
    JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorTuberias& estado = m.estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        int mismos = 0;
        int puesto = 0;

        for (int j = 0; j < limite; j++)
        {
            if (m.estadosJugadores[j].participa && m.estadosJugadores[j].seleccion == estado.seleccion)
            {
                if (j < i) puesto++;
                mismos++;
            }
        }

        float objetivoX =
            XCarril(m.cantidadEntradas, estado.seleccion) +
            ((float)puesto - 0.5f * (float)(mismos - 1)) * 0.9f;

        JugadorPrueba& jugador = jugadores[i];
        float t = Acotar(10.0f * deltaTime, 0.0f, 1.0f);

        jugador.posicion.x += (objetivoX - jugador.posicion.x) * t;
        jugador.posicion.y = ALTURA_CENTRO_JUGADOR;
        jugador.posicion.z = Z_PLATAFORMA_JUGADORES;
        jugador.velocidad = {};
        jugador.direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugador.enSuelo = true;
        jugador.cayendo = false;
    }
}


void MinijuegoTuberiasDesierto::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    if (deltaTime > 0.05f)
    {
        deltaTime = 0.05f;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    tiempoAnimacion += deltaTime;

    if (!partidaValida)
    {
        return;
    }

    ColocarJugadores(*this, jugadores, limite, deltaTime);

    if (fase == FASE_TUBERIAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            IniciarRonda(*this);
        }

        return;
    }

    float faseAntes = tiempoFase;
    tiempoFase += deltaTime;

    if (fase == FASE_TUBERIAS_OBSERVAR)
    {
        if (tiempoFase >= DURACION_OBSERVAR)
        {
            CambiarFase(*this, FASE_TUBERIAS_ELEGIR);
        }
    }
    else if (fase == FASE_TUBERIAS_ELEGIR)
    {
        bool todos = true;

        for (int i = 0; i < limite; i++)
        {
            EstadoJugadorTuberias& estado = estadosJugadores[i];

            if (!estado.participa)
            {
                continue;
            }

            ActualizarSeleccionJugador(*this, estado, participantes[i], deltaTime);

            if (!estado.bloqueado)
            {
                todos = false;
            }
        }

        ActualizarAudioAlertaTiempo(
            audio,
            DURACION_ELEGIR - faseAntes,
            DURACION_ELEGIR - tiempoFase,
            3.0f
        );

        if (todos && tiempoFase < DURACION_ELEGIR - 0.6f)
        {
            tiempoFase = DURACION_ELEGIR - 0.6f;
        }

        if (tiempoFase >= DURACION_ELEGIR)
        {
            for (int i = 0; i < limite; i++)
            {
                estadosJugadores[i].bloqueado = true;
            }

            CambiarFase(*this, FASE_TUBERIAS_COMPUERTA);
        }
    }
    else if (fase == FASE_TUBERIAS_COMPUERTA)
    {
        compuerta = Acotar(tiempoFase / DURACION_COMPUERTA, 0.0f, 1.0f);

        if (tiempoFase >= DURACION_COMPUERTA)
        {
            compuerta = 1.0f;
            CambiarFase(*this, FASE_TUBERIAS_FLUJO);
        }
    }
    else if (fase == FASE_TUBERIAS_FLUJO)
    {
        progresoAgua = Acotar(tiempoFase / DURACION_FLUJO, 0.0f, 1.0f);

        if (tiempoFase >= DURACION_FLUJO)
        {
            progresoAgua = 1.0f;
            ResolverRonda(*this);
            CambiarFase(*this, FASE_TUBERIAS_REVELAR);
        }
    }
    else if (fase == FASE_TUBERIAS_REVELAR)
    {
        if (tiempoFase >= DURACION_REVELAR)
        {
            if (ronda >= RONDAS_TUBERIAS - 1)
            {
                FinalizarTuberias(*this);
            }
            else
            {
                ronda++;
                IniciarRonda(*this);
            }
        }
    }
}


//==================================================
// VISUAL: PARED DE ACUEDUCTOS EN EL DESIERTO
//==================================================
//
// MODELO FUTURO: pared de arenisca con relieves, cisterna del oasis con
// palmeras, acueductos y tuberias de piedra, columnas, canteros de barro
// y oro, compuertas, cactus, dunas y sol. La logica (GenerarEntramado y
// los puntos del recorrido) no depende de estas funciones.

static const Color COLOR_ARENISCA = { 214, 178, 118, 255 };
static const Color COLOR_ARENISCA_OSCURA = { 176, 138, 86, 255 };
static const Color COLOR_TUBERIA = { 196, 128, 82, 255 };
static const Color COLOR_AGUA_TUBERIA = { 60, 170, 235, 255 };
static const Color COLOR_ORO = { 255, 205, 60, 255 };


// Punto del recorrido de la entrada e en el parametro u: u < 0 tramo de
// entrada, 0..etapas entramado, u > etapas tramo final hacia el cantaro.
static Vector3 PuntoTuberia(const MinijuegoTuberiasDesierto& m, int e, float u)
{
    int n = m.cantidadEntradas;
    float alturaEtapa = AlturaEtapa(m);

    if (u <= 0.0f)
    {
        return { XCarril(n, m.carril[e][0]), Y_INICIO - u * 2.0f, Z_TUBERIA };
    }

    if (u >= (float)m.etapas)
    {
        return
        {
            XCarril(n, m.carril[e][m.etapas]),
            Y_FIN - (u - (float)m.etapas) * 2.0f,
            Z_TUBERIA
        };
    }

    int s = (int)u;
    float fraccion = u - (float)s;
    int a = m.carril[e][s];
    int b = m.carril[e][s + 1];
    float w = Acotar((fraccion - 0.2f) / 0.6f, 0.0f, 1.0f);
    float suave = w * w * (3.0f - 2.0f * w);
    float profundidad = 0.0f;

    if (a != b)
    {
        profundidad = (b > a ? 0.5f : -0.5f) * std::sin(3.14159265f * w);
    }

    return
    {
        XCarril(n, a) + (XCarril(n, b) - XCarril(n, a)) * suave,
        Y_INICIO - ((float)s + fraccion) * alturaEtapa,
        Z_TUBERIA + profundidad
    };
}


static void DibujarCielo()
{
    // Sol abrasador.
    DrawSphere({ -26.0f, 22.0f, -60.0f }, 11.0f, Color{ 255, 224, 130, 255 });
    DrawSphere({ -26.0f, 22.0f, -60.0f }, 16.0f, Fade(Color{ 255, 190, 80, 255 }, 0.3f));
    DrawSphere({ -26.0f, 22.0f, -60.0f }, 24.0f, Fade(Color{ 255, 170, 70, 255 }, 0.14f));
}


static void DibujarPalmera(float x, float y, float z, float escala)
{
    DrawCylinder({ x, y, z }, 0.25f * escala, 0.4f * escala, 4.0f * escala, 7, Color{ 120, 84, 48, 255 });

    Vector3 copa = { x, y + 4.0f * escala, z };

    DrawSphere(copa, 0.5f * escala, Color{ 70, 150, 60, 255 });

    for (int k = 0; k < 6; k++)
    {
        float angulo = (float)k * 1.0472f;

        DrawCylinderEx(
            copa,
            { copa.x + std::cos(angulo) * 2.0f * escala, copa.y - 0.7f * escala, copa.z + std::sin(angulo) * 2.0f * escala },
            0.12f * escala, 0.02f, 4, Color{ 60, 160, 66, 255 }
        );
    }
}


static void DibujarCactus(float x, float z, float escala)
{
    Color verde = { 62, 140, 74, 255 };

    DrawCylinder({ x, 0.0f, z }, 0.35f * escala, 0.4f * escala, 2.6f * escala, 8, verde);
    DrawSphere({ x, 2.6f * escala, z }, 0.35f * escala, verde);
    DrawCylinder({ x - 0.75f * escala, 1.0f * escala, z }, 0.18f * escala, 0.2f * escala, 1.0f * escala, 6, verde);
    DrawCube({ x - 0.4f * escala, 1.0f * escala, z }, 0.7f * escala, 0.3f * escala, 0.3f * escala, verde);
    DrawCylinder({ x + 0.75f * escala, 1.5f * escala, z }, 0.18f * escala, 0.2f * escala, 0.9f * escala, 6, verde);
    DrawCube({ x + 0.4f * escala, 1.5f * escala, z }, 0.7f * escala, 0.3f * escala, 0.3f * escala, verde);
}


static void DibujarEscenario(const MinijuegoTuberiasDesierto& m)
{
    float t = m.tiempoAnimacion;

    // Suelo y dunas.
    DrawCube({ 0.0f, -0.6f, -30.0f }, 220.0f, 1.0f, 160.0f, Color{ 226, 190, 128, 255 });
    DrawSphere({ -34.0f, -6.0f, -14.0f }, 14.0f, Color{ 232, 196, 134, 255 });
    DrawSphere({ 36.0f, -7.0f, -18.0f }, 16.0f, Color{ 222, 184, 122, 255 });
    DrawSphere({ 4.0f, -12.0f, -50.0f }, 24.0f, Color{ 228, 192, 130, 255 });
    DrawSphere({ -60.0f, -8.0f, -40.0f }, 20.0f, Color{ 218, 180, 118, 255 });

    // Pared de arenisca.
    DrawCube({ 0.0f, 7.5f, -3.2f }, 30.0f, 15.0f, 1.2f, COLOR_ARENISCA);
    DrawCube({ 0.0f, 15.2f, -3.2f }, 31.0f, 0.6f, 1.8f, COLOR_ARENISCA_OSCURA);
    DrawCube({ 0.0f, 13.9f, -2.55f }, 28.0f, 0.2f, 0.1f, COLOR_ARENISCA_OSCURA);
    DrawCube({ 0.0f, 0.4f, -2.55f }, 28.0f, 0.2f, 0.1f, COLOR_ARENISCA_OSCURA);

    for (int k = 0; k < 8; k++)
    {
        float x = -13.0f + (float)k * 3.7f;

        DrawCube({ x, 14.6f, -2.55f }, 0.7f, 0.5f, 0.08f, Color{ 150, 112, 68, 255 });
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 14.2f;

        DrawCube({ x, 7.5f, -2.5f }, 1.0f, 14.0f, 0.4f, COLOR_ARENISCA_OSCURA);

        // Columnas rotas de las ruinas.
        DrawCylinder({ (float)lado * 18.0f, 0.0f, -1.0f }, 0.9f, 1.0f, 6.0f, 10, Color{ 200, 164, 108, 255 });
        DrawCylinder({ (float)lado * 21.0f, 0.0f, 1.0f }, 0.9f, 1.0f, 3.2f, 10, Color{ 190, 154, 100, 255 });
        DrawCube({ (float)lado * 18.0f, 6.1f, -1.0f }, 2.4f, 0.4f, 2.4f, Color{ 190, 154, 100, 255 });
        DibujarCactus((float)lado * 11.5f, 4.5f, 1.0f);
    }

    // Oasis-cisterna con palmeras.
    DrawCylinder({ 0.0f, 15.5f, -3.2f }, 4.2f, 4.4f, 1.2f, 20, COLOR_ARENISCA_OSCURA);
    DrawCylinder({ 0.0f, 16.6f, -3.2f }, 3.6f, 3.6f, 0.15f, 20, Color{ 50, 190, 215, 255 });
    DrawCylinder({ 0.0f, 16.74f + 0.04f * std::sin(t * 2.0f), -3.2f }, 3.0f, 3.0f, 0.04f, 20, Fade(Color{ 150, 235, 245, 255 }, 0.6f));
    DibujarPalmera(-6.8f, 15.2f, -3.2f, 1.1f);
    DibujarPalmera(6.4f, 15.2f, -3.4f, 1.0f);
    DibujarPalmera(-10.0f, 15.2f, -3.2f, 0.8f);

    // Colector sobre las entradas.
    float xIzq = XCarril(m.cantidadEntradas, 0);
    float xDer = XCarril(m.cantidadEntradas, m.cantidadEntradas - 1);

    DrawCylinderEx({ xIzq, 13.7f, Z_TUBERIA }, { xDer, 13.7f, Z_TUBERIA }, 0.32f, 0.32f, 8, COLOR_ARENISCA_OSCURA);
    DrawCylinderEx({ 0.0f, 15.0f, -3.0f }, { 0.0f, 13.7f, Z_TUBERIA }, 0.4f, 0.4f, 8, COLOR_ARENISCA_OSCURA);

    // Repisa de cantaros y plataforma de los jugadores.
    DrawCube({ 0.0f, 0.9f, -1.9f }, 28.0f, 1.8f, 2.4f, Color{ 190, 154, 100, 255 });
    DrawCube({ 0.0f, -0.25f, 3.0f }, 26.0f, 0.5f, 6.5f, Color{ 224, 188, 126, 255 });
    DrawCube({ 0.0f, -0.24f, 6.3f }, 26.0f, 0.5f, 0.2f, COLOR_ARENISCA_OSCURA);

    // Espejismo en el horizonte.
    for (int k = 0; k < 4; k++)
    {
        float x = -20.0f + (float)k * 14.0f + 2.0f * std::sin(t * 0.7f + (float)k);

        DrawCube({ x, 1.0f + 0.3f * std::sin(t * 1.5f + (float)k), -28.0f }, 8.0f, 0.15f, 0.2f, Fade(Color{ 160, 220, 235, 255 }, 0.4f));
    }
}


static void DibujarTuberias(const MinijuegoTuberiasDesierto& m)
{
    int n = m.cantidadEntradas;
    const float paso = 0.2f;
    float uMin = -0.3f;
    float uMax = (float)m.etapas + 0.3f;
    float uAgua = uMin + m.progresoAgua * (uMax - uMin);
    bool pulso = m.fase == FASE_TUBERIAS_REVELAR;
    float t = m.tiempoAnimacion;

    for (int e = 0; e < n; e++)
    {
        bool ganadora = e == m.entradaGanadora;

        Vector3 anterior = PuntoTuberia(m, e, uMin);
        Vector3 anteriorAgua = anterior;

        for (float u = uMin + paso; u < uMax + paso * 0.5f; u += paso)
        {
            float uu = u > uMax ? uMax : u;
            Vector3 actual = PuntoTuberia(m, e, uu);

            DrawCylinderEx(anterior, actual, 0.36f, 0.36f, 6, COLOR_TUBERIA);

            if (m.progresoAgua > 0.0f && uu <= uAgua + 0.0001f)
            {
                Color agua = COLOR_AGUA_TUBERIA;

                if (pulso && ganadora)
                {
                    float brillo = 0.5f + 0.5f * std::sin(t * 8.0f);
                    agua = Color{ 255, (unsigned char)(200 + 40 * brillo), 80, 255 };
                }

                DrawCylinderEx(anterior, actual, 0.24f, 0.24f, 6, agua);
                anteriorAgua = actual;
            }

            anterior = actual;
        }

        // Frente de agua.
        if (m.progresoAgua > 0.0f && m.progresoAgua < 1.0f)
        {
            DrawSphere(anteriorAgua, 0.3f, Fade(WHITE, 0.8f));
        }
    }

    // Columnas por delante de algunos tramos rectos.
    for (int k = 1; k < m.etapas; k++)
    {
        int e = (int)(HashIndice(k + m.ronda * 7) % (unsigned int)n);
        Vector3 centro = PuntoTuberia(m, e, (float)k);

        DrawCylinder({ centro.x, centro.y - 0.24f, -1.4f }, 0.5f, 0.5f, 0.48f, 10, Color{ 218, 184, 128, 255 });
        DrawCylinder({ centro.x, centro.y - 0.28f, -1.4f }, 0.62f, 0.62f, 0.07f, 10, COLOR_ARENISCA_OSCURA);
        DrawCylinder({ centro.x, centro.y + 0.22f, -1.4f }, 0.62f, 0.62f, 0.07f, 10, COLOR_ARENISCA_OSCURA);
    }
}


static void DibujarCantaro(
    const MinijuegoTuberiasDesierto& m,
    int lugar
)
{
    float t = m.tiempoAnimacion;
    float x = XCarril(m.cantidadEntradas, lugar);
    float z = -1.9f;
    bool oro = lugar == m.oroCantaro;
    bool miraje = lugar == m.mirajeCantaro;
    Color color = Color{ 178, 98, 58, 255 };

    if (miraje && m.mirajeRevelado)
    {
        // El espejismo se disipa en humo de arena.
        float vida = Acotar(m.tiempoFase / 1.2f, 0.0f, 1.0f);

        for (int k = 0; k < 4; k++)
        {
            float angulo = (float)k * 1.57f + t;

            DrawSphere(
                { x + std::cos(angulo) * 0.5f * vida, Y_REPISA + 0.7f + 1.2f * vida, z + std::sin(angulo) * 0.5f * vida },
                0.35f * (1.0f - 0.5f * vida),
                Fade(Color{ 230, 200, 140, 255 }, 1.0f - vida)
            );
        }

        DrawCylinder({ x, Y_REPISA, z }, 0.7f, 0.7f, 0.05f, 12, Color{ 150, 110, 70, 255 });
        return;
    }

    if (miraje)
    {
        float onda = 0.07f * std::sin(t * 9.0f + (float)lugar);

        x += onda;
        color = Fade(Color{ 250, 226, 140, 255 }, 0.6f + 0.2f * std::sin(t * 5.0f));
    }
    else if (oro)
    {
        color = COLOR_ORO;
    }

    DrawSphere({ x, Y_REPISA + 0.75f, z }, 0.75f, color);
    DrawCylinder({ x, Y_REPISA + 1.2f, z }, 0.34f, 0.5f, 0.5f, 10, color);
    DrawCylinder({ x, Y_REPISA + 1.65f, z }, 0.52f, 0.34f, 0.14f, 10, color);

    if (oro)
    {
        // Destellos del cantaro dorado.
        for (int k = 0; k < 3; k++)
        {
            float angulo = t * 2.0f + (float)k * 2.09f;

            DrawCube(
                { x + std::cos(angulo) * 0.9f, Y_REPISA + 1.0f + 0.5f * std::sin(t * 3.0f + (float)k), z + 0.8f },
                0.12f, 0.12f, 0.12f, Color{ 255, 250, 200, 255 }
            );
        }
    }

    // Agua que llena el cantaro al llegar.
    if (m.progresoAgua >= 1.0f && !miraje)
    {
        DrawCylinder({ x, Y_REPISA + 1.55f, z }, 0.3f, 0.3f, 0.12f, 10, oro ? Color{ 255, 240, 140, 255 } : COLOR_AGUA_TUBERIA);
    }
}


static void DibujarCompuertas(const MinijuegoTuberiasDesierto& m)
{
    for (int e = 0; e < m.cantidadEntradas; e++)
    {
        float x = XCarril(m.cantidadEntradas, e);
        float subida = 0.9f * m.compuerta;

        DrawCylinder({ x, Y_ENTRADA - 0.3f, Z_TUBERIA }, 0.5f, 0.5f, 0.4f, 8, COLOR_ARENISCA_OSCURA);
        DrawCube({ x, Y_ENTRADA + 0.2f + subida, Z_TUBERIA + 0.5f }, 1.0f, 0.5f, 0.15f, Color{ 120, 90, 60, 255 });
    }
}


// Los selectores que coinciden en una entrada se reparten en horizontal.
static float DesplazamientoSelector(const MinijuegoTuberiasDesierto& m, int i, int limite, int& puestoSalida)
{
    int mismos = 0;
    int puesto = 0;

    for (int j = 0; j < limite; j++)
    {
        if (m.estadosJugadores[j].participa && m.estadosJugadores[j].seleccion == m.estadosJugadores[i].seleccion)
        {
            if (j < i) puesto++;
            mismos++;
        }
    }

    puestoSalida = puesto;
    return ((float)puesto - 0.5f * (float)(mismos - 1)) * 1.3f;
}


static void DibujarSelectores(
    const MinijuegoTuberiasDesierto& m,
    int limite,
    const Participante participantes[]
)
{
    float t = m.tiempoAnimacion;

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorTuberias& estado = m.estadosJugadores[i];

        if (!estado.participa || m.fase == FASE_TUBERIAS_PREPARACION)
        {
            continue;
        }

        int puesto = 0;
        float x = XCarril(m.cantidadEntradas, estado.seleccion) + DesplazamientoSelector(m, i, limite, puesto);
        float y = Y_ENTRADA + 1.4f + (estado.bloqueado ? 0.0f : 0.12f * std::sin(t * 6.0f));
        Color color = participantes[i].color;

        DrawCylinder({ x, y, Z_TUBERIA + 0.6f }, 0.45f, 0.0f, 0.8f, 8, color);

        if (estado.bloqueado)
        {
            DrawSphere({ x, y + 0.9f, Z_TUBERIA + 0.6f }, 0.2f, color);
        }
    }
}


void MinijuegoTuberiasDesierto::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    ClearBackground(Color{ 250, 206, 140, 255 });
    BeginMode3D(camara);

    DibujarCielo();
    DibujarEscenario(*this);
    DibujarTuberias(*this);
    DibujarCompuertas(*this);

    for (int lugar = 0; lugar < cantidadEntradas; lugar++)
    {
        DibujarCantaro(*this, lugar);
    }

    for (int i = 0; i < limite; i++)
    {
        if (!estadosJugadores[i].participa)
        {
            continue;
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawCubeWires(jugadores[i].posicion, 0.8f, 1.4f, 0.8f, LIME);
        }
    }

    DibujarSelectores(*this, limite, participantes);

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    int centro = anchoPantalla / 2;

    // Etiquetas de jugador sobre su selector.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorTuberias& estado = estadosJugadores[i];

        if (!estado.participa || fase == FASE_TUBERIAS_PREPARACION || fase == FASE_TUBERIAS_TERMINADO)
        {
            continue;
        }

        int puestoEtiqueta = 0;
        Vector2 pantalla = GetWorldToScreen(
            { XCarril(cantidadEntradas, estado.seleccion) + DesplazamientoSelector(*this, i, limite, puestoEtiqueta), Y_ENTRADA + 3.3f + 1.0f * (float)(puestoEtiqueta % 2), Z_TUBERIA + 0.6f },
            camara
        );
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        const char* texto = TextFormat("J%d", numero);

        DrawText(texto, (int)pantalla.x - MeasureText(texto, 18) / 2, (int)pantalla.y, 18, participantes[i].color);
    }

    // Puntos ganados en la ronda sobre cada jugador.
    if (fase == FASE_TUBERIAS_REVELAR)
    {
        for (int i = 0; i < limite; i++)
        {
            if (!estadosJugadores[i].participa)
            {
                continue;
            }

            Vector2 pantalla = GetWorldToScreen(
                { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.4f, jugadores[i].posicion.z },
                camara
            );
            int delta = estadosJugadores[i].deltaRonda;
            const char* texto = delta > 0 ? TextFormat("+%d", delta) : TextFormat("%d", delta);
            Color color = delta > 0 ? GOLD : (delta < 0 ? RED : LIGHTGRAY);

            DrawText(texto, (int)pantalla.x - MeasureText(texto, 34) / 2, (int)pantalla.y - 20, 34, color);
        }
    }

    // Marcador.
    int filas = 0;

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].participa) filas++;
    }

    DrawRectangle(12, 10, 210, 40 + filas * 24, Fade(BLACK, 0.6f));
    DrawText(TextFormat("RONDA %d / %d", ronda + 1 > RONDAS_TUBERIAS ? RONDAS_TUBERIAS : ronda + 1, RONDAS_TUBERIAS), 22, 16, 22, RAYWHITE);

    int fila = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!estadosJugadores[i].participa)
        {
            continue;
        }

        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;

        DrawText(
            TextFormat("J%d   %d PUNTOS", numero, estadosJugadores[i].puntos),
            22, 48 + fila * 24, 20, participantes[i].color
        );
        fila++;
    }

    const char* aviso = "";
    Color colorAviso = RAYWHITE;

    if (fase == FASE_TUBERIAS_OBSERVAR)
    {
        aviso = TextFormat("OBSERVA LAS TUBERIAS: %d", (int)std::ceil(DURACION_OBSERVAR - tiempoFase));
        colorAviso = Color{ 90, 50, 20, 255 };
    }
    else if (fase == FASE_TUBERIAS_ELEGIR)
    {
        aviso = TextFormat("ELIGE UNA ENTRADA: %d", (int)std::ceil(DURACION_ELEGIR - tiempoFase));
        colorAviso = Color{ 90, 50, 20, 255 };
    }
    else if (fase == FASE_TUBERIAS_COMPUERTA || fase == FASE_TUBERIAS_FLUJO)
    {
        aviso = "EL AGUA CORRE POR LAS TUBERIAS...";
        colorAviso = Color{ 20, 80, 150, 255 };
    }
    else if (fase == FASE_TUBERIAS_REVELAR)
    {
        aviso = "CANTARO DORADO +3   ESPEJISMO -1";
        colorAviso = Color{ 120, 60, 10, 255 };
    }

    DrawText(aviso, centro - MeasureText(aviso, 30) / 2, 14, 30, colorAviso);

    const char* ayuda = "IZQ / DER: MOVER SELECTOR   ACCION (E / SHIFT DER / B): CONFIRMAR   BUSCA LA TUBERIA QUE LLEGA AL CANTARO DORADO";
    DrawRectangle(0, altoPantalla - 32, anchoPantalla, 32, Fade(BLACK, 0.7f));
    DrawText(ayuda, centro - MeasureText(ayuda, 14) / 2, altoPantalla - 24, 14, RAYWHITE);

    if (fase == FASE_TUBERIAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, centro - MeasureText(texto, 96) / 2, altoPantalla / 2 - 130, 96, Color{ 140, 60, 10, 255 });

        int linea = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!estadosJugadores[i].participa || JugadorEsBot(participantes[i]))
            {
                continue;
            }

            const char* boton = participantes[i].control == CONTROL_GAMEPAD
                ? "B"
                : (participantes[i].control == CONTROL_TECLADO_FLECHAS ? "SHIFT DER" : "E");

            DrawText(
                TextFormat("J%d CONFIRMAR: %s", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1, boton),
                centro - 130, altoPantalla / 2 + 30 + linea * 24, 20, participantes[i].color
            );
            linea++;
        }
    }
    else if (
        fase == FASE_TUBERIAS_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int ancho = 480;
        int alto = 110 + filas * 26;
        int py = altoPantalla / 2 - alto / 2 - 20;

        DrawRectangle(centro - ancho / 2, py, ancho, alto, Fade(BLACK, 0.9f));

        const char* titulo = empate ? "EMPATE EN EL OASIS" : "FIN DE LAS RONDAS";
        DrawText(titulo, centro - MeasureText(titulo, 30) / 2, py + 14, 30, empate ? YELLOW : GOLD);

        int linea = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (!resultado.participantes[i].participo || resultado.participantes[i].posicionFinal != posicion)
                {
                    continue;
                }

                int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;

                DrawText(
                    TextFormat("%d.  J%d   %d PUNTOS", posicion, numero, estadosJugadores[i].puntos),
                    centro - ancho / 2 + 40, py + 60 + linea * 26, 20, participantes[i].color
                );
                linea++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            centro - MeasureText(TextoReinicioMinijuego(), 16) / 2,
            py + alto - 28, 16, RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoTuberiasDesierto::ObtenerResultado() const
{
    return resultado;
}
