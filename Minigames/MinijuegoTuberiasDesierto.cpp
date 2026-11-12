#include "Minigames/MinijuegoTuberiasDesierto.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HorneadoMallas.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <vector>


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


static void LiberarMallasTuberias();


void MinijuegoTuberiasDesierto::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    LiberarMallasTuberias();
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
// VISUAL: OASIS AL ATARDECER
//==================================================
//
// Cinco capas: terraza de losas de arenisca con la repisa de los cantaros
// (arena); fachada del acueducto con la red de tuberias, cisterna del
// oasis con palmeras y campamento de caravana (anillo cercano); arcada
// del acueducto y torre-aljibe (media distancia); dunas en dos planos con
// una caravana de camellos (horizonte) y cielo naranja-violeta con el sol
// bajo. Lo estatico se hornea en mallas: la escena (por calidad) y las
// tuberias (por diseno de la ronda). La logica no depende de esta seccion.
//
// MODELO FUTURO: palmera, camello, tienda de caravana, cantaro, farol,
// brasero y compuerta de madera.

static const Color COLOR_FONDO_T = { 150, 78, 106, 255 };
static const Color COLOR_CIELO_ALTO_T = { 46, 38, 98, 255 };
static const Color COLOR_CIELO_MEDIO_T = { 112, 62, 128, 255 };
static const Color COLOR_CIELO_ROSA_T = { 214, 98, 112, 255 };
static const Color COLOR_CIELO_BAJO_T = { 248, 146, 86, 255 };
static const Color COLOR_HORIZONTE_T = { 255, 204, 128, 255 };
static const Color COLOR_BRUMA_T = { 232, 146, 124, 255 };
static const Color COLOR_SOL_T = { 255, 240, 186, 255 };
static const Color COLOR_TINTE_SOL_T = { 255, 166, 92, 255 };
static const Color COLOR_TINTE_SOMBRA_T = { 96, 68, 142, 255 };
static const Color COLOR_ARENISCA_T = { 214, 150, 104, 255 };
static const Color COLOR_ARENISCA_CLARA_T = { 236, 190, 136, 255 };
static const Color COLOR_ARENISCA_OSCURA_T = { 160, 98, 76, 255 };
static const Color COLOR_MURO_T = { 192, 124, 94, 255 };
static const Color COLOR_LOSA_T = { 204, 146, 100, 255 };
static const Color COLOR_JUNTA_T = { 134, 82, 68, 255 };
static const Color COLOR_ARENA_T = { 230, 158, 106, 255 };
static const Color COLOR_TUBO_T = { 242, 214, 168, 255 };
static const Color COLOR_CANAL_T = { 104, 62, 56, 255 };
static const Color COLOR_AGUA_T = { 64, 196, 236, 255 };
static const Color COLOR_AGUA_CLARA_T = { 196, 248, 255, 255 };
static const Color COLOR_ORO_T = { 255, 198, 52, 255 };
static const Color COLOR_ORO_CLARO_T = { 255, 240, 160, 255 };
static const Color COLOR_BARRO_T = { 178, 92, 58, 255 };
static const Color COLOR_MADERA_T = { 108, 66, 46, 255 };
static const Color COLOR_HOJA_T = { 64, 112, 68, 255 };
static const Color COLOR_TRONCO_T = { 118, 78, 56, 255 };
static const Color COLOR_FUEGO_T = { 255, 168, 64, 255 };
static const Color COLOR_TINTA_T = { 78, 40, 30, 255 };

static const float FOV_T = 42.5f;
static const float Z_MURO_T = -3.3f;          // cara del panel de sillares
static const float Z_CANTARO_T = -1.9f;
static const float R_TUBO_T = 0.38f;
static const float Y_COLECTOR_T = 13.55f;     // centro del colector
static const float Z_COLECTOR_T = -2.2f;


//--------------------------------------------------
// Color y ruido (azar visual con semilla propia)
//--------------------------------------------------

static float Lim01T(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static float SuaveT(float v)
{
    v = Lim01T(v);
    return v * v * (3.0f - 2.0f * v);
}


static unsigned char CanalT(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static float Ruido01T(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float RuidoSuaveT(float x, float z, float escala, int semilla)
{
    float fx = x / escala, fz = z / escala;
    int ix = (int)std::floor(fx), iz = (int)std::floor(fz);
    float tx = fx - (float)ix, tz = fz - (float)iz;
    tx = tx * tx * (3.0f - 2.0f * tx);
    tz = tz * tz * (3.0f - 2.0f * tz);
    auto H = [&](int a, int b) { return Ruido01T(a * 7919 + b * 104729 + 50000, semilla); };
    float a = H(ix, iz) + (H(ix + 1, iz) - H(ix, iz)) * tx;
    float b = H(ix, iz + 1) + (H(ix + 1, iz + 1) - H(ix, iz + 1)) * tx;
    return a + (b - a) * tz;
}


static Color TonoT(Color c, float f)
{
    return { CanalT((float)c.r * f), CanalT((float)c.g * f), CanalT((float)c.b * f), c.a };
}


static Color MezclaT(Color a, Color b, float t)
{
    t = Lim01T(t);
    return {
        CanalT((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalT((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalT((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalT((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaT(Color c, float alfa)
{
    c.a = CanalT((float)c.a * Lim01T(alfa));
    return c;
}


static Color VariarT(Color c, int indice, int semilla, float amplitud)
{
    return TonoT(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01T(indice, semilla));
}


//--------------------------------------------------
// Lote de triangulos (inmediato o grabado para hornear)
//--------------------------------------------------

struct LuzHorneadaT
{
    Vector3 p;
    float radio;
    Color color;
};

static const int MAX_LUCES_HORNEADAS_T = 12;

static GrabadorMalla g_grabT;
static Vector3 g_ojoT = { 0.0f, 9.2f, 31.0f };    // camara final (calima)
static Vector3 g_derT = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaT = { 0.0f, 1.0f, 0.0f };
static bool g_sinBrumaT = false;
static float g_brumaExtraT = 0.0f;                // horizonte: mas calima
static LuzHorneadaT g_lucesT[MAX_LUCES_HORNEADAS_T];
static int g_cantidadLucesT = 0;


// Calima de color por distancia y, al hornear, el resplandor de braseros
// y faroles sobre lo que tienen cerca.
static Color NieblaT(Color c, Vector3 p)
{
    if (g_sinBrumaT) return c;

    if (g_grabT.activa && g_cantidadLucesT > 0)
    {
        float r = (float)c.r, g = (float)c.g, b = (float)c.b;

        for (int k = 0; k < g_cantidadLucesT; k++)
        {
            const LuzHorneadaT& l = g_lucesT[k];
            float dx = p.x - l.p.x, dy = p.y - l.p.y, dz = p.z - l.p.z;
            float d2 = dx * dx + dy * dy + dz * dz;

            if (d2 >= l.radio * l.radio) continue;

            float f = 1.0f - std::sqrt(d2) / l.radio;
            f = f * f * 0.5f;
            r += (float)l.color.r * f;
            g += (float)l.color.g * f;
            b += (float)l.color.b * f;
        }

        c = { CanalT(r), CanalT(g), CanalT(b), c.a };
    }

    float dx = p.x - g_ojoT.x, dy = p.y - g_ojoT.y, dz = p.z - g_ojoT.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = std::sqrt(Lim01T((d - 42.0f) / 280.0f)) * 0.8f;
    n = Lim01T(n + g_brumaExtraT);
    Color r = MezclaT(c, COLOR_BRUMA_T, n);
    r.a = c.a;
    return r;
}


static void VT(Vector3 p, Color c)
{
    if (g_grabT.activa)
    {
        GrabarVerticeMalla(g_grabT, p, c);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteT()
{
    if (!g_grabT.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteT()
{
    if (!g_grabT.activa) rlEnd();
}


static void TriT(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VT(a, NieblaT(color, a));
    VT(b, NieblaT(color, b));
    VT(c, NieblaT(color, c));
}


static void QuadT(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    Color ca = NieblaT(color, a), cb = NieblaT(color, b), cc = NieblaT(color, c), cd = NieblaT(color, d);
    VT(a, ca); VT(b, cb); VT(c, cc);
    VT(a, ca); VT(c, cc); VT(d, cd);
}


static void Quad4T(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    Color na = NieblaT(ca, a), nc = NieblaT(cc, c);
    VT(a, na); VT(b, NieblaT(cb, b)); VT(c, nc);
    VT(a, na); VT(c, nc); VT(d, NieblaT(cd, d));
}


// Sin calima, un color por vertice (luces aditivas, sombras, cielo).
static void QuadColoresT(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VT(a, ca); VT(b, cb); VT(c, cc);
    VT(a, ca); VT(c, cc); VT(d, cd);
}


// Sombreado por caras (arriba 1.0, frente 0.78, izquierda 0.70, derecha
// 0.62) adaptado al sol bajo, que entra desde la izquierda y atras.
static float LuzT(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f) return 1.0f;

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.46f) +
        (n.z > 0.0f ? az * 0.80f : az * 0.58f) +
        (n.x > 0.0f ? ax * 0.92f : ax * 0.62f);

    return f / suma;
}


static Color CaraT(Color c, Vector3 n)
{
    Color r = TonoT(c, LuzT(n));

    if (n.x > 0.3f) r = MezclaT(r, COLOR_TINTE_SOL_T, 0.18f * n.x);
    if (n.y > 0.3f) r = MezclaT(r, COLOR_TINTE_SOL_T, 0.07f * n.y);

    float sombra = -n.x > -n.z ? -n.x : -n.z;

    if (sombra > 0.3f) r = MezclaT(r, COLOR_TINTE_SOMBRA_T, 0.16f * sombra);
    if (n.y < -0.3f) r = MezclaT(r, COLOR_TINTE_SOMBRA_T, 0.2f);

    return r;
}


static void CajaEjesT(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
{
    auto P = [&](float sx, float sy, float sz)
    {
        return Vector3{
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    };

    Vector3 nx = Vector3Normalize(ex), ny = Vector3Normalize(ey), nz = Vector3Normalize(ez);

    QuadT(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraT(color, ny));
    QuadT(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraT(color, nz));
    QuadT(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraT(color, Vector3Negate(nz)));
    QuadT(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraT(color, Vector3Negate(nx)));
    QuadT(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraT(color, nx));

    if (fondo)
    {
        QuadT(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraT(color, Vector3Negate(ny)));
    }
}


static void CajaT(Vector3 c, Vector3 t, Color color, bool fondo = false)
{
    CajaEjesT(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, fondo);
}


// Caja por esquinas (min, max).
static void CajaMMT(float x0, float y0, float z0, float x1, float y1, float z1, Color color, bool fondo = false)
{
    CajaT({ 0.5f * (x0 + x1), 0.5f * (y0 + y1), 0.5f * (z0 + z1) }, { x1 - x0, y1 - y0, z1 - z0 }, color, fondo);
}


// Caja girada en Y y ladeada sobre su eje X local (grados).
static void CajaGiradaT(Vector3 c, Vector3 t, float giroY, Color color, float ladeo = 0.0f)
{
    float a = giroY * DEG2RAD;
    float l = ladeo * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz0 = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = { -uz0.x * std::sin(l), std::cos(l), -uz0.z * std::sin(l) };
    Vector3 uz = { uz0.x * std::cos(l), std::sin(l), uz0.z * std::cos(l) };

    CajaEjesT(c, Vector3Scale(ux, t.x * 0.5f), Vector3Scale(uy, t.y * 0.5f), Vector3Scale(uz, t.z * 0.5f), color, false);
}


static void VigaT(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesT(centro, Vector3Scale(v, grosor * 0.5f), Vector3Scale(w, grosor * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


static void CilT(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    eje = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, ref));
    Vector3 v = Vector3CrossProduct(eje, u);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = 0.5f * (a0 + a1);
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));

        QuadT(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraT(color, nm)
        );

        if (tapas)
        {
            if (rb > 0.0001f) TriT(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraT(color, eje));
            if (ra > 0.0001f) TriT(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraT(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide (o cupula con desde = 0) con caras sombreadas.
static void BolaT(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float desde = -0.5f * PI)
{
    for (int i = 0; i < anillos; i++)
    {
        float la0 = desde + (0.5f * PI - desde) * (float)i / (float)anillos;
        float la1 = desde + (0.5f * PI - desde) * (float)(i + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float lo0 = 2.0f * PI * (float)k / (float)lados;
            float lo1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float lam = 0.5f * (la0 + la1), lom = 0.5f * (lo0 + lo1);
            Vector3 n = { std::cos(lam) * std::cos(lom), std::sin(lam), std::cos(lam) * std::sin(lom) };
            auto P = [&](float la, float lo)
            {
                return Vector3{ c.x + r.x * std::cos(la) * std::cos(lo), c.y + r.y * std::sin(la), c.z + r.z * std::cos(la) * std::sin(lo) };
            };
            QuadT(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraT(color, n));
        }
    }
}


// Solido de revolucion (cantaros, vasijas, cupulas): radios r[] a alturas
// y[] sobre la base. desplX[] corre cada anillo (ondas del espejismo).
static void TornoT(Vector3 base, const float* r, const float* y, int n, int lados, Color color, const float* desplX = nullptr)
{
    for (int i = 0; i < n - 1; i++)
    {
        float dx0 = desplX ? desplX[i] : 0.0f;
        float dx1 = desplX ? desplX[i + 1] : 0.0f;
        float dy = y[i + 1] - y[i];
        float dr = r[i] - r[i + 1];

        for (int k = 0; k < lados; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)lados;
            float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float am = 0.5f * (a0 + a1);
            Vector3 nrm = Vector3Normalize({ std::cos(am) * dy, dr, std::sin(am) * dy });
            Vector3 p00 = { base.x + dx0 + std::cos(a0) * r[i], base.y + y[i], base.z + std::sin(a0) * r[i] };
            Vector3 p01 = { base.x + dx0 + std::cos(a1) * r[i], base.y + y[i], base.z + std::sin(a1) * r[i] };
            Vector3 p11 = { base.x + dx1 + std::cos(a1) * r[i + 1], base.y + y[i + 1], base.z + std::sin(a1) * r[i + 1] };
            Vector3 p10 = { base.x + dx1 + std::cos(a0) * r[i + 1], base.y + y[i + 1], base.z + std::sin(a0) * r[i + 1] };
            QuadT(p00, p01, p11, p10, CaraT(color, nrm));
        }
    }
}


// Disco horizontal con color de centro y de borde.
static void DiscoT(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, bool bruma = true)
{
    Color cc = bruma ? NieblaT(centro, c) : centro;

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };
        VT(c, cc);
        VT(p0, bruma ? NieblaT(borde, p0) : borde);
        VT(p1, bruma ? NieblaT(borde, p1) : borde);
    }
}


static void AnilloT(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColoresT(i0, o0, o1, i1, dentro, fuera, fuera, dentro);
    }
}


// Disco que mira a la camara (halos, sol).
static void HaloT(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derT, std::cos(a0) * radio), Vector3Scale(g_arribaT, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derT, std::cos(a1) * radio), Vector3Scale(g_arribaT, std::sin(a1) * radio)));
        VT(c, centro); VT(p0, borde); VT(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas, estrellas).
static void PuntoT(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derT, r), u = Vector3Scale(g_arribaT, r);
    QuadColoresT(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Cuadro vertical de frente (z constante), sin sombreado.
static void FrenteT(float x0, float y0, float x1, float y1, float z, Color color)
{
    QuadT({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, color);
}


//--------------------------------------------------
// Horno: lo estatico se graba en mallas
//--------------------------------------------------

struct HornoTuberias
{
    int claveEscena = -1;
    unsigned long long claveTubos = 0;
    Mesh escena{};
    bool escenaValida = false;
    Mesh tubos{};
    bool tubosValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoTuberias g_hornoT;


static void LiberarMallasTuberias()
{
    LiberarMallaHorneada(g_hornoT.escena, g_hornoT.escenaValida);
    LiberarMallaHorneada(g_hornoT.tubos, g_hornoT.tubosValida);
    g_hornoT.claveEscena = -1;
    g_hornoT.claveTubos = 0;
}


static void AsegurarMaterialT()
{
    AsegurarMaterialHorneado(g_hornoT.material, g_hornoT.materialListo);
}


static void DibujarMallaT(const Mesh& malla, bool valida)
{
    if (!valida) return;

    rlDrawRenderBatchActive();
    DrawMesh(malla, g_hornoT.material, MatrixIdentity());
}


//--------------------------------------------------
// Camara: encuadre fijo y bajada suave en la cuenta
//--------------------------------------------------

static Camera3D CamaraFinalT()
{
    Camera3D c{};
    c.position = { 0.0f, 9.2f, 31.0f };
    c.target = { 0.0f, 7.0f, -2.0f };
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = FOV_T;
    c.projection = CAMERA_PERSPECTIVE;
    return c;
}


static Camera3D CamaraVistaT(const MinijuegoTuberiasDesierto& m)
{
    Camera3D c = CamaraFinalT();

    if (m.fase == FASE_TUBERIAS_PREPARACION && m.partidaValida)
    {
        float u = Lim01T(m.tiempoPreparacion / DURACION_PREPARACION_TUBERIAS);
        float e = SuaveT(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        c.position.x -= 5.0f * e;
        c.position.y += 6.0f * e;
        c.position.z += 13.0f * e;
        c.target.y += 2.0f * e;
    }

    return c;
}


struct BaseCamaraT
{
    Vector3 ojo, f, r, u;
    float tv, th;
};


static BaseCamaraT BaseDeT(const Camera3D& c)
{
    BaseCamaraT b;
    b.ojo = c.position;
    b.f = Vector3Normalize(Vector3Subtract(c.target, c.position));
    b.r = Vector3Normalize(Vector3CrossProduct(b.f, c.up));
    b.u = Vector3CrossProduct(b.r, b.f);
    b.tv = std::tan(c.fovy * 0.5f * DEG2RAD);
    float aspecto = (float)GetScreenWidth() / (float)(GetScreenHeight() > 0 ? GetScreenHeight() : 1);
    b.th = b.tv * aspecto;
    return b;
}


static Vector3 DireccionT(const BaseCamaraT& b, float sx, float sy)
{
    return Vector3Normalize(Vector3Add(b.f, Vector3Add(Vector3Scale(b.r, sx * b.th), Vector3Scale(b.u, sy * b.tv))));
}


//--------------------------------------------------
// Recorridos de las tuberias
//--------------------------------------------------

// Punto del recorrido de la entrada e en el parametro u: u < 0 tramo de
// entrada, 0..etapas entramado, u > etapas tramo final hacia el cantaro.
// En los cruces una tuberia pasa por delante (z+) y la otra por detras.
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


static const float U_MIN_T = -0.3f;
static const float U_PICO_T = 0.12f;          // la tuberia termina en el pico


static float UMaxT(const MinijuegoTuberiasDesierto& m)
{
    return (float)m.etapas + 0.3f;
}


static float UAguaT(const MinijuegoTuberiasDesierto& m)
{
    return U_MIN_T + m.progresoAgua * (UMaxT(m) - U_MIN_T);
}


// Muestras del recorrido: finas en los cruces, gruesas en lo recto.
static int MuestrasTuberiaT(const MinijuegoTuberiasDesierto& m, float uHasta, float* us, int maximo)
{
    int n = 0;
    float uFin = (float)m.etapas + U_PICO_T;

    if (uHasta > uFin) uHasta = uFin;

    us[n++] = U_MIN_T;

    for (int s = 0; s <= m.etapas && n < maximo - 12; s++)
    {
        const float f[10] = { 0.0f, 0.2f, 0.29f, 0.38f, 0.46f, 0.54f, 0.62f, 0.71f, 0.8f, 1.0f };

        for (int k = 0; k < 9 && n < maximo - 2; k++)
        {
            float u = (float)s + f[k];

            if (u <= us[n - 1] + 0.0001f) continue;
            if (u >= uHasta) break;

            us[n++] = u;
        }
    }

    if (uHasta > us[n - 1] + 0.0001f) us[n++] = uHasta;

    return n;
}


// Direccion lateral (en el plano de la pared) del tramo a-b.
static Vector3 LateralT(Vector3 a, Vector3 b)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    float l = std::sqrt(dx * dx + dy * dy);

    if (l < 0.0001f) return { 1.0f, 0.0f, 0.0f };

    return { -dy / l, dx / l, 0.0f };
}


// Tira de frente a lo largo del recorrido (canal, agua, brillo).
static void CintaTuberiaT(const MinijuegoTuberiasDesierto& m, int e, float uDesde, float uHasta, float adelante, float ancho, Color color, float brilloFlujo, float t)
{
    float us[96];
    int n = MuestrasTuberiaT(m, uHasta, us, 96);

    for (int k = 0; k < n - 1; k++)
    {
        if (us[k + 1] <= uDesde) continue;

        float u0 = us[k] < uDesde ? uDesde : us[k];
        Vector3 a = PuntoTuberia(m, e, u0);
        Vector3 b = PuntoTuberia(m, e, us[k + 1]);
        Vector3 l = Vector3Scale(LateralT(a, b), ancho * 0.5f);
        a.z += adelante;
        b.z += adelante;
        Color ca = color, cb = color;

        if (brilloFlujo > 0.0f)
        {
            ca = MezclaT(color, COLOR_AGUA_CLARA_T, brilloFlujo * (0.5f + 0.5f * std::sin(u0 * 7.0f - t * 9.0f)));
            cb = MezclaT(color, COLOR_AGUA_CLARA_T, brilloFlujo * (0.5f + 0.5f * std::sin(us[k + 1] * 7.0f - t * 9.0f)));
        }

        QuadColoresT(Vector3Subtract(a, l), Vector3Add(a, l), Vector3Add(b, l), Vector3Subtract(b, l), ca, ca, cb, cb);
    }
}


// Tuberia de piedra clara: tubo con collarines, canal oscuro al frente y
// sombra proyectada en la pared (el sol entra desde la izquierda).
static void TuboPiedraT(const MinijuegoTuberiasDesierto& m, int e, float uHasta, float radio, bool detalle)
{
    float us[96];
    int n = MuestrasTuberiaT(m, uHasta, us, 96);
    int lados = detalle ? 8 : 6;

    for (int k = 0; k < n - 1; k++)
    {
        Vector3 a = PuntoTuberia(m, e, us[k]);
        Vector3 b = PuntoTuberia(m, e, us[k + 1]);
        Color piedra = VariarT(COLOR_TUBO_T, e * 131 + (int)(us[k] * 3.0f), 11, 0.04f);
        CilT(a, b, radio, radio, lados, piedra, false);

        // Sombra en la pared, solo donde el tubo va pegado a ella.
        if (radio > 0.2f)
        {
            Vector3 l = Vector3Scale(LateralT(a, b), 0.42f);
            Vector3 sa = { a.x - 0.42f, a.y - 0.22f, Z_MURO_T + 0.04f };
            Vector3 sb = { b.x - 0.42f, b.y - 0.22f, Z_MURO_T + 0.04f };
            Color s = Color{ 70, 34, 60, 70 };
            QuadColoresT(Vector3Subtract(sa, l), Vector3Add(sa, l), Vector3Add(sb, l), Vector3Subtract(sb, l), s, s, s, s);
        }
    }

    // Tapas en los extremos.
    Vector3 ini = PuntoTuberia(m, e, U_MIN_T);
    CilT({ ini.x, ini.y + 0.02f, ini.z }, { ini.x, ini.y - 0.02f, ini.z }, radio, radio, lados, COLOR_TUBO_T);

    if (radio < 0.2f) return;

    // Canal oscuro al frente (por ahi se ve correr el agua).
    CintaTuberiaT(m, e, U_MIN_T, uHasta, radio * 1.04f, radio * 1.1f, COLOR_CANAL_T, 0.0f, 0.0f);

    // Collarines en lo recto (cada media etapa entera).
    for (int s = 0; s <= m.etapas; s++)
    {
        float u = (float)s;

        if (u > uHasta) break;

        Vector3 c = PuntoTuberia(m, e, u);
        Color collar = VariarT(COLOR_ARENISCA_T, e * 17 + s, 12, 0.05f);
        CilT({ c.x, c.y + 0.13f, c.z }, { c.x, c.y - 0.13f, c.z }, radio * 1.22f, radio * 1.22f, lados, collar);

        // Mensula que lo sujeta a la pared.
        CajaMMT(c.x - 0.18f, c.y - 0.12f, Z_MURO_T, c.x + 0.18f, c.y + 0.12f, c.z - radio * 0.8f, COLOR_ARENISCA_OSCURA_T);
    }
}


// Clave del diseno de la ronda (rehornea las tuberias al cambiar).
static unsigned long long ClaveTubosT(const MinijuegoTuberiasDesierto& m)
{
    unsigned long long h = 1469598103934665603ull;
    auto Mezclar = [&](int v) { h ^= (unsigned long long)(v + 1000); h *= 1099511628211ull; };

    Mezclar(m.cantidadEntradas);
    Mezclar(m.etapas);
    Mezclar(m.ronda);
    Mezclar((int)CalidadDecoracion());

    for (int e = 0; e < m.cantidadEntradas; e++)
    {
        for (int s = 0; s <= m.etapas; s++) Mezclar(m.carril[e][s]);
    }

    return h;
}


static float XIzqColectorT(const MinijuegoTuberiasDesierto& m)
{
    return XCarril(m.cantidadEntradas, 0) - 1.1f;
}


static float XDerColectorT(const MinijuegoTuberiasDesierto& m)
{
    return XCarril(m.cantidadEntradas, m.cantidadEntradas - 1) + 1.1f;
}


// Colector, marcos de compuerta y placas numeradas (dependen de la
// cantidad de entradas, no del recorrido).
static void ColectorT(const MinijuegoTuberiasDesierto& m)
{
    float x0 = XIzqColectorT(m), x1 = XDerColectorT(m);
    float y0 = Y_COLECTOR_T - 0.42f, y1 = Y_COLECTOR_T + 0.42f;

    // Canal colector con borde y agua quieta arriba.
    CajaMMT(x0, y0, Z_COLECTOR_T - 0.6f, x1, y1, Z_COLECTOR_T + 0.55f, COLOR_ARENISCA_T, true);
    CajaMMT(x0 - 0.12f, y1, Z_COLECTOR_T - 0.66f, x1 + 0.12f, y1 + 0.14f, Z_COLECTOR_T + 0.62f, COLOR_ARENISCA_CLARA_T);
    CajaMMT(x0 - 0.1f, y0 - 0.16f, Z_COLECTOR_T - 0.6f, x1 + 0.1f, y0, Z_COLECTOR_T + 0.6f, COLOR_ARENISCA_OSCURA_T, true);
    FrenteT(x0 + 0.2f, y1 + 0.141f, x1 - 0.2f, y1 + 0.142f, Z_COLECTOR_T, COLOR_AGUA_T);
    QuadT({ x0 + 0.15f, y1 + 0.145f, Z_COLECTOR_T - 0.45f }, { x1 - 0.15f, y1 + 0.145f, Z_COLECTOR_T - 0.45f },
        { x1 - 0.15f, y1 + 0.145f, Z_COLECTOR_T + 0.42f }, { x0 + 0.15f, y1 + 0.145f, Z_COLECTOR_T + 0.42f }, COLOR_AGUA_T);

    // Caño que baja del acueducto al colector, por la izquierda.
    CilT({ -11.6f, 15.2f, Z_COLECTOR_T }, { x0 - 0.05f, Y_COLECTOR_T + 0.1f, Z_COLECTOR_T }, 0.42f, 0.42f, 8, COLOR_ARENISCA_T);

    for (int e = 0; e < m.cantidadEntradas; e++)
    {
        float x = XCarril(m.cantidadEntradas, e);

        // Placa del numero (el digito se escribe en 2D encima).
        CajaMMT(x - 0.42f, Y_COLECTOR_T - 0.34f, Z_COLECTOR_T + 0.55f, x + 0.42f, Y_COLECTOR_T + 0.34f, Z_COLECTOR_T + 0.66f, COLOR_ARENISCA_CLARA_T);

        // Marco de la compuerta bajo el colector.
        CajaMMT(x - 0.62f, Y_ENTRADA - 0.25f, Z_TUBERIA + 0.3f, x - 0.48f, y0, Z_TUBERIA + 0.62f, COLOR_ARENISCA_OSCURA_T);
        CajaMMT(x + 0.48f, Y_ENTRADA - 0.25f, Z_TUBERIA + 0.3f, x + 0.62f, y0, Z_TUBERIA + 0.62f, COLOR_ARENISCA_OSCURA_T);
        CajaMMT(x - 0.62f, Y_ENTRADA - 0.36f, Z_TUBERIA + 0.3f, x + 1.5f, Y_ENTRADA - 0.25f, Z_TUBERIA + 0.62f, COLOR_ARENISCA_OSCURA_T);
    }
}


// Pico de piedra al final de cada tuberia, sobre la boca del cantaro.
static void PicoT(const MinijuegoTuberiasDesierto& m, int e)
{
    Vector3 p = PuntoTuberia(m, e, (float)m.etapas + U_PICO_T);
    CajaMMT(p.x - 0.5f, p.y - 0.42f, Z_TUBERIA - 0.5f, p.x + 0.5f, p.y + 0.12f, Z_TUBERIA + 0.5f, COLOR_ARENISCA_T);
    CajaMMT(p.x - 0.3f, p.y - 0.4f, Z_TUBERIA + 0.5f, p.x + 0.3f, p.y - 0.12f, Z_TUBERIA + 0.86f, COLOR_ARENISCA_CLARA_T);
    FrenteT(p.x - 0.16f, p.y - 0.36f, p.x + 0.16f, p.y - 0.2f, Z_TUBERIA + 0.865f, COLOR_CANAL_T);
}


// Tambores de columna delante de algunos tramos rectos (tapan un poco
// el recorrido: misma regla de siempre).
static void TamboresT(const MinijuegoTuberiasDesierto& m)
{
    int n = m.cantidadEntradas;

    for (int k = 1; k < m.etapas; k++)
    {
        int e = (int)(HashIndice(k + m.ronda * 7) % (unsigned int)n);
        Vector3 c = PuntoTuberia(m, e, (float)k);
        CilT({ c.x, c.y - 0.26f, -1.4f }, { c.x, c.y + 0.26f, -1.4f }, 0.52f, 0.52f, 10, COLOR_ARENISCA_CLARA_T);
        CilT({ c.x, c.y - 0.34f, -1.4f }, { c.x, c.y - 0.26f, -1.4f }, 0.64f, 0.64f, 10, COLOR_ARENISCA_OSCURA_T);
        CilT({ c.x, c.y + 0.26f, -1.4f }, { c.x, c.y + 0.34f, -1.4f }, 0.64f, 0.64f, 10, COLOR_ARENISCA_OSCURA_T);
        CajaMMT(c.x - 0.12f, c.y - 0.1f, Z_MURO_T, c.x + 0.12f, c.y + 0.1f, -1.6f, COLOR_ARENISCA_OSCURA_T);
    }
}


static void TuberiasEstaticasT(const MinijuegoTuberiasDesierto& m, bool detalle)
{
    for (int e = 0; e < m.cantidadEntradas; e++)
    {
        TuboPiedraT(m, e, (float)m.etapas + U_PICO_T, R_TUBO_T, detalle);
        PicoT(m, e);
    }

    TamboresT(m);
}


static void HornearTubosT(const MinijuegoTuberiasDesierto& m)
{
    unsigned long long clave = ClaveTubosT(m);

    if ((g_hornoT.tubosValida && g_hornoT.claveTubos == clave) || !IsWindowReady()) return;

    LiberarMallaHorneada(g_hornoT.tubos, g_hornoT.tubosValida);
    AsegurarMaterialT();
    EmpezarGrabacionMalla(g_grabT);
    ColectorT(m);
    TuberiasEstaticasT(m, CalidadDecoracion() != CALIDAD_BAJA);
    g_hornoT.tubosValida = TerminarGrabacionMalla(g_grabT, g_hornoT.tubos);
    g_hornoT.claveTubos = clave;
}


// Mientras aparecen (inicio de cada ronda) las tuberias crecen desde la
// compuerta hacia el cantaro, en ola de izquierda a derecha.
static float CrecimientoTuberiaT(const MinijuegoTuberiasDesierto& m, int e)
{
    if (m.fase != FASE_TUBERIAS_OBSERVAR) return 1.0f;

    float u = (m.tiempoFase - 0.07f * (float)e) / 0.75f;
    return SuaveT(u);
}


static bool TuberiasCreciendoT(const MinijuegoTuberiasDesierto& m)
{
    return m.fase == FASE_TUBERIAS_OBSERVAR && m.tiempoFase < 0.75f + 0.07f * (float)m.cantidadEntradas;
}


// Las tuberias solo existen desde que empieza la primera ronda (en la
// cuenta el diseno todavia no es el de la partida).
static bool TuberiasVisiblesT(const MinijuegoTuberiasDesierto& m)
{
    return m.partidaValida && m.fase != FASE_TUBERIAS_PREPARACION;
}


//--------------------------------------------------
// Escena estatica: suelo, terraza, repisa y fachada
//--------------------------------------------------

// Arena del desierto: llana junto a la terraza, ondulada lejos.
static float AlturaArenaT(float x, float z)
{
    float ex = x / 30.0f, ez = (z + 12.0f) / 42.0f;
    float lejos = Lim01T(std::sqrt(ex * ex + ez * ez) - 1.0f);
    float onda = RuidoSuaveT(x, z, 26.0f, 7) * 0.7f + RuidoSuaveT(x, z, 9.0f, 8) * 0.3f;
    return -0.4f + lejos * lejos * 7.0f * onda;
}


static Color ColorArenaT(float x, float z)
{
    float v = RuidoSuaveT(x, z, 11.0f, 9);
    Color c = TonoT(COLOR_ARENA_T, 0.9f + 0.16f * v);
    float onda = RuidoSuaveT(x + 3.0f, z, 26.0f, 7);
    return MezclaT(c, COLOR_TINTE_SOMBRA_T, 0.22f * (1.0f - onda) * Lim01T((-z - 10.0f) / 40.0f));
}


static void SueloT(bool decoracion)
{
    float paso = decoracion ? 6.0f : 12.0f;

    for (float z = -180.0f; z < 42.0f; z += paso)
    {
        for (float x = -168.0f; x < 168.0f; x += paso)
        {
            float x1 = x + paso, z1 = z + paso;
            Vector3 a = { x, AlturaArenaT(x, z), z }, b = { x1, AlturaArenaT(x1, z), z };
            Vector3 c = { x1, AlturaArenaT(x1, z1), z1 }, d = { x, AlturaArenaT(x, z1), z1 };
            Quad4T(a, b, c, d, ColorArenaT(x, z), ColorArenaT(x1, z), ColorArenaT(x1, z1), ColorArenaT(x, z1));
        }
    }

    // Ondas del viento en la arena cercana (lineas tenues).
    if (!decoracion) return;

    for (int k = 0; k < 40; k++)
    {
        float x = -40.0f + 80.0f * Ruido01T(k, 21);
        float z = -26.0f + 60.0f * Ruido01T(k, 22);

        if (std::fabs(x) < 13.5f && z > -8.0f && z < 8.0f) continue;

        float largo = 2.0f + 3.0f * Ruido01T(k, 23);
        float y = AlturaArenaT(x, z) + 0.03f;
        Color c = TonoT(COLOR_ARENA_T, 0.8f);
        QuadT({ x, y, z }, { x + largo, y, z + 0.3f }, { x + largo, y, z + 0.42f }, { x, y, z + 0.1f }, c);
    }
}


// Terraza de losas de arenisca (donde estan los jugadores).
static const float X_TERRAZA_T = 12.4f;
static const float Z_TERRAZA_FRENTE_T = 6.4f;
static const float Z_TERRAZA_FONDO_T = -0.7f;


static void TerrazaT(bool decoracion)
{
    // Fondo de las juntas.
    QuadT({ -X_TERRAZA_T, -0.05f, Z_TERRAZA_FONDO_T }, { X_TERRAZA_T, -0.05f, Z_TERRAZA_FONDO_T },
        { X_TERRAZA_T, -0.05f, Z_TERRAZA_FRENTE_T }, { -X_TERRAZA_T, -0.05f, Z_TERRAZA_FRENTE_T }, COLOR_JUNTA_T);

    int filas = 5;
    float fondoFila = (Z_TERRAZA_FRENTE_T - Z_TERRAZA_FONDO_T) / (float)filas;

    for (int f = 0; f < filas; f++)
    {
        float z0 = Z_TERRAZA_FONDO_T + (float)f * fondoFila;
        float desfase = (f % 2) * 0.8f;
        float x = -X_TERRAZA_T;
        int k = 0;

        while (x < X_TERRAZA_T - 0.01f)
        {
            float largo = 1.6f + 0.7f * Ruido01T(f * 40 + k, 31);

            if (k == 0) largo = 0.9f + desfase;
            if (x + largo > X_TERRAZA_T) largo = X_TERRAZA_T - x;

            Color c = VariarT(COLOR_LOSA_T, f * 40 + k, 32, 0.08f);
            c = MezclaT(c, COLOR_ARENISCA_CLARA_T, 0.25f * Ruido01T(f * 40 + k, 33));
            float g = 0.06f;
            QuadT({ x + g, 0.0f, z0 + g }, { x + largo - g, 0.0f, z0 + g },
                { x + largo - g, 0.0f, z0 + fondoFila - g }, { x + g, 0.0f, z0 + fondoFila - g }, CaraT(c, { 0.0f, 1.0f, 0.0f }));

            // Grieta o mancha de arena en algunas losas.
            if (decoracion && Ruido01T(f * 40 + k, 34) < 0.22f)
            {
                float cx = x + largo * (0.3f + 0.4f * Ruido01T(k, 35));
                float cz = z0 + fondoFila * 0.5f;
                DiscoT({ cx, 0.01f, cz }, 0.5f, 0.3f, 7, AlfaT(COLOR_ARENA_T, 0.9f), AlfaT(COLOR_ARENA_T, 0.0f));
            }

            x += largo;
            k++;
        }
    }

    // Bordillo con grosor al frente y a los costados (escalon a la arena).
    for (float x = -X_TERRAZA_T - 0.5f; x < X_TERRAZA_T + 0.5f; x += 1.7f)
    {
        float x1 = x + 1.7f > X_TERRAZA_T + 0.5f ? X_TERRAZA_T + 0.5f : x + 1.7f;
        Color c = VariarT(COLOR_ARENISCA_T, (int)(x * 10.0f), 36, 0.06f);
        CajaMMT(x + 0.03f, -0.45f, Z_TERRAZA_FRENTE_T, x1 - 0.03f, 0.1f, Z_TERRAZA_FRENTE_T + 0.6f, c);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float xa = lado < 0 ? -X_TERRAZA_T - 0.5f : X_TERRAZA_T;
        float xb = xa + 0.5f;

        for (float z = Z_TERRAZA_FONDO_T; z < Z_TERRAZA_FRENTE_T; z += 1.8f)
        {
            float z1 = z + 1.8f > Z_TERRAZA_FRENTE_T ? Z_TERRAZA_FRENTE_T : z + 1.8f;
            Color c = VariarT(COLOR_ARENISCA_T, (int)(z * 10.0f) + lado * 7, 37, 0.06f);
            CajaMMT(xa, -0.45f, z + 0.03f, xb, 0.1f, z1 - 0.03f, c);
        }
    }

    // Escalinata central hacia la arena.
    CajaMMT(-3.2f, -0.45f, Z_TERRAZA_FRENTE_T + 0.6f, 3.2f, -0.18f, Z_TERRAZA_FRENTE_T + 1.3f, COLOR_ARENISCA_CLARA_T);
}


// Repisa de piedra donde descansan los cantaros.
static void RepisaT()
{
    CajaMMT(-9.6f, 0.0f, Z_MURO_T - 0.05f, 9.6f, 1.6f, -0.78f, COLOR_ARENISCA_T);
    CajaMMT(-9.85f, 1.6f, Z_MURO_T - 0.05f, 9.85f, Y_REPISA, -0.56f, COLOR_ARENISCA_CLARA_T);
    CajaMMT(-9.85f, 0.0f, -0.78f, 9.85f, 0.22f, -0.6f, COLOR_ARENISCA_OSCURA_T);

    // Hornacinas en el frente de la repisa.
    for (int k = 0; k < 8; k++)
    {
        float x = -8.4f + (float)k * 2.4f;
        FrenteT(x - 0.75f, 0.42f, x + 0.75f, 1.36f, -0.775f, COLOR_ARENISCA_OSCURA_T);
        FrenteT(x - 0.6f, 0.5f, x + 0.6f, 1.24f, -0.77f, TonoT(COLOR_JUNTA_T, 0.9f));
        TriT({ x - 0.6f, 1.24f, -0.77f }, { x + 0.6f, 1.24f, -0.77f }, { x, 1.4f, -0.77f }, TonoT(COLOR_JUNTA_T, 0.9f));
    }
}


// Fachada del acueducto: panel de sillares, pilastras, cornisa y almenas.
static void FachadaT(bool decoracion)
{
    const float x0 = -9.6f, x1 = 9.6f, y0 = Y_REPISA, y1 = 15.0f;

    // Mortero de fondo y sillares.
    FrenteT(x0, y0, x1, y1, Z_MURO_T - 0.04f, COLOR_JUNTA_T);

    int fila = 0;

    for (float y = y0; y < y1 - 0.01f; y += 0.94f, fila++)
    {
        float ya = y, yb = y + 0.94f > y1 ? y1 : y + 0.94f;
        float x = x0;
        int k = 0;

        while (x < x1 - 0.01f)
        {
            float largo = 1.9f + 1.1f * Ruido01T(fila * 60 + k, 41);

            if (k == 0) largo = 0.6f + 1.2f * Ruido01T(fila, 42);
            if (x + largo > x1) largo = x1 - x;

            Color c = VariarT(COLOR_MURO_T, fila * 60 + k, 43, 0.07f);
            float luz = 0.95f + 0.12f * (x - x0) / (x1 - x0) + 0.05f * (y - y0) / (y1 - y0);
            c = TonoT(c, luz);
            c = MezclaT(c, COLOR_ARENISCA_T, 0.25f * Ruido01T(fila * 60 + k, 44));
            float g = 0.045f;
            FrenteT(x + g, ya + g, x + largo - g, yb - g, Z_MURO_T, CaraT(c, { 0.0f, 0.0f, 1.0f }));

            // Desgaste: borde superior mas claro (luz rasante).
            if (decoracion)
            {
                FrenteT(x + g, yb - g - 0.06f, x + largo - g, yb - g, Z_MURO_T + 0.005f, TonoT(CaraT(c, { 0.0f, 0.0f, 1.0f }), 1.1f));
            }

            x += largo;
            k++;
        }
    }

    // Friso con dentellones bajo la cornisa.
    CajaMMT(x0, y1 - 0.55f, Z_MURO_T - 0.1f, x1, y1, Z_MURO_T + 0.32f, COLOR_ARENISCA_OSCURA_T);

    for (float x = x0 + 0.3f; x < x1 - 0.2f; x += 0.6f)
    {
        CajaMMT(x - 0.14f, y1 - 0.5f, Z_MURO_T + 0.32f, x + 0.14f, y1 - 0.18f, Z_MURO_T + 0.5f, COLOR_ARENISCA_T);
    }

    // Cornisa y almenas redondeadas.
    CajaMMT(-11.6f, y1, -4.0f, 11.6f, y1 + 0.6f, -2.3f, COLOR_ARENISCA_CLARA_T, true);
    CajaMMT(-11.4f, y1 + 0.6f, -3.8f, 11.4f, y1 + 0.85f, -2.6f, COLOR_ARENISCA_T);

    for (float x = -10.8f; x <= 10.81f; x += 1.8f)
    {
        CajaMMT(x - 0.5f, y1 + 0.85f, -3.6f, x + 0.5f, y1 + 1.55f, -2.8f, COLOR_ARENISCA_T);
        CilT({ x, y1 + 1.55f, -3.2f }, { x, y1 + 1.95f, -3.2f }, 0.46f, 0.08f, 8, COLOR_ARENISCA_CLARA_T);
    }

    // Masa del muro detras.
    CajaMMT(-11.6f, 0.0f, -7.5f, 11.6f, y1, Z_MURO_T - 0.05f, COLOR_ARENISCA_OSCURA_T);

    // Pilastras con basa y capitel.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float xa = lado < 0 ? -11.1f : 9.6f;
        float xb = xa + 1.5f;
        CajaMMT(xa, 0.0f, -3.9f, xb, y1, -1.8f, COLOR_ARENISCA_T);
        CajaMMT(xa - 0.2f, 0.0f, -4.0f, xb + 0.2f, 0.8f, -1.6f, COLOR_ARENISCA_CLARA_T);
        CajaMMT(xa - 0.2f, y1 - 0.9f, -4.0f, xb + 0.2f, y1 - 0.5f, -1.6f, COLOR_ARENISCA_CLARA_T);
        CajaMMT(xa - 0.1f, y1 - 0.5f, -3.95f, xb + 0.1f, y1, -1.7f, COLOR_ARENISCA_OSCURA_T);

        // Acanaladuras.
        for (int k = 0; k < 3; k++)
        {
            float x = xa + 0.35f + (float)k * 0.4f;
            FrenteT(x - 0.06f, 1.1f, x + 0.06f, y1 - 1.1f, -1.795f, TonoT(COLOR_ARENISCA_OSCURA_T, 0.9f));
        }

        // Mensula del farol colgante.
        float xm = lado < 0 ? xa - 0.2f : xb + 0.2f;
        VigaT({ xm, 10.6f, -2.6f }, { xm + (float)lado * 1.0f, 10.6f, -2.6f }, 0.16f, COLOR_MADERA_T);
        VigaT({ xm, 10.0f, -2.6f }, { xm + (float)lado * 0.7f, 10.6f, -2.6f }, 0.1f, COLOR_MADERA_T);
    }

    // Ala baja a la derecha (el acueducto sale por la izquierda).
    CajaMMT(11.1f, 0.0f, -5.4f, 15.6f, 7.2f, -3.4f, COLOR_MURO_T);
    CajaMMT(11.0f, 7.2f, -5.5f, 15.8f, 7.65f, -3.25f, COLOR_ARENISCA_CLARA_T);
    FrenteT(12.4f, 0.0f, 14.4f, 3.9f, -3.395f, COLOR_ARENISCA_OSCURA_T);
    FrenteT(12.6f, 0.0f, 14.2f, 3.7f, -3.39f, COLOR_JUNTA_T);

    for (int s = 0; s < 6; s++)
    {
        float a0 = PI * (float)s / 6.0f, a1 = PI * (float)(s + 1) / 6.0f;
        TriT({ 13.4f, 3.7f, -3.39f }, { 13.4f + std::cos(a0) * 0.8f, 3.7f + std::sin(a0) * 0.8f, -3.39f },
            { 13.4f + std::cos(a1) * 0.8f, 3.7f + std::sin(a1) * 0.8f, -3.39f }, COLOR_JUNTA_T);
    }
}


//--------------------------------------------------
// Acueducto de arcos, cisterna del oasis y palmeras
//--------------------------------------------------

// Marco local de la arcada: a lo largo (a), de frente (c) y altura (y).
struct MarcoArcadaT
{
    Vector3 origen, largo, frente;
};


static Vector3 PuntoArcadaT(const MarcoArcadaT& f, float a, float y, float c)
{
    return {
        f.origen.x + f.largo.x * a + f.frente.x * c,
        y,
        f.origen.z + f.largo.z * a + f.frente.z * c
    };
}


// Enjuta con el hueco del arco (medio punto) entre a0 y a1.
static void ArcoArcadaT(const MarcoArcadaT& f, float a0, float a1, float arranque, float techo, float fondo, Color color)
{
    float r = 0.5f * (a1 - a0), ac = 0.5f * (a0 + a1);
    const int k = 7;
    Vector3 nFrente = { -f.frente.x, 0.0f, -f.frente.z };

    for (int i = 0; i < k; i++)
    {
        float t0 = PI - PI * (float)i / (float)k, t1 = PI - PI * (float)(i + 1) / (float)k;
        float ax0 = ac + std::cos(t0) * r, ay0 = arranque + std::sin(t0) * r;
        float ax1 = ac + std::cos(t1) * r, ay1 = arranque + std::sin(t1) * r;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            float c = (float)lado * fondo * 0.5f;
            Vector3 n = lado < 0 ? nFrente : Vector3Negate(nFrente);
            QuadT(PuntoArcadaT(f, ax0, ay0, c), PuntoArcadaT(f, ax1, ay1, c), PuntoArcadaT(f, ax1, techo, c), PuntoArcadaT(f, ax0, techo, c), CaraT(color, n));
        }

        // Intrados (cara de abajo del arco).
        QuadT(PuntoArcadaT(f, ax0, ay0, -fondo * 0.5f), PuntoArcadaT(f, ax1, ay1, -fondo * 0.5f),
            PuntoArcadaT(f, ax1, ay1, fondo * 0.5f), PuntoArcadaT(f, ax0, ay0, fondo * 0.5f), CaraT(TonoT(color, 0.8f), { 0.0f, -1.0f, 0.0f }));
    }
}


static void PilarArcadaT(const MarcoArcadaT& f, float a0, float a1, float y0, float y1, float fondo, Color color)
{
    Vector3 c = PuntoArcadaT(f, 0.5f * (a0 + a1), 0.5f * (y0 + y1), 0.0f);
    CajaEjesT(c, Vector3Scale(f.largo, 0.5f * (a1 - a0)), { 0.0f, 0.5f * (y1 - y0), 0.0f }, Vector3Scale(f.frente, 0.5f * fondo), color);
}


// Acueducto de dos pisos de arcos que llega desde el oasis lejano.
static void ArcadaT(bool decoracion)
{
    MarcoArcadaT f;
    f.origen = { -11.8f, 0.0f, -2.6f };
    Vector3 d = Vector3Normalize({ -1.0f, 0.0f, -0.95f });
    f.largo = d;
    f.frente = { -d.z, 0.0f, d.x };

    const float vano = 4.2f, pilar = 1.2f;
    int tramos = decoracion ? 14 : 6;

    for (int i = 0; i < tramos; i++)
    {
        float a = (float)i * vano;
        Color piedra = VariarT(COLOR_ARENISCA_T, i, 71, 0.05f);

        // Piso bajo: pilares gruesos y arcos grandes.
        PilarArcadaT(f, a, a + pilar, 0.0f, 7.4f, 2.2f, piedra);
        ArcoArcadaT(f, a + pilar, a + vano, 5.0f, 7.4f, 2.2f, piedra);

        // Imposta.
        PilarArcadaT(f, a - 0.1f, a + vano + 0.1f, 7.4f, 7.9f, 2.5f, COLOR_ARENISCA_CLARA_T);

        // Piso alto: arcos mas chicos.
        PilarArcadaT(f, a, a + pilar * 0.8f, 7.9f, 14.2f, 1.6f, piedra);
        ArcoArcadaT(f, a + pilar * 0.8f, a + vano, 11.9f, 14.2f, 1.6f, piedra);

        // Canal de agua arriba con borde.
        PilarArcadaT(f, a - 0.1f, a + vano + 0.1f, 14.2f, 15.0f, 1.9f, COLOR_ARENISCA_CLARA_T);
        PilarArcadaT(f, a - 0.1f, a + vano + 0.1f, 15.0f, 15.5f, 0.35f, piedra);

        if (decoracion && i < 6)
        {
            // Sillares marcados en los pilares cercanos.
            for (int s = 0; s < 6; s++)
            {
                float y = 0.6f + (float)s * 1.15f;
                Vector3 p0 = PuntoArcadaT(f, a + 0.02f, y, -1.11f), p1 = PuntoArcadaT(f, a + pilar - 0.02f, y, -1.11f);
                Vector3 p2 = PuntoArcadaT(f, a + pilar - 0.02f, y + 0.05f, -1.11f), p3 = PuntoArcadaT(f, a + 0.02f, y + 0.05f, -1.11f);
                QuadT(p0, p1, p2, p3, TonoT(piedra, 0.72f));
            }
        }
    }

    // Ultimo pilar y estribo pegado a la fachada.
    float aFin = (float)tramos * vano;
    PilarArcadaT(f, aFin, aFin + pilar, 0.0f, 15.0f, 2.2f, COLOR_ARENISCA_T);
    CajaMMT(-12.6f, 0.0f, -4.6f, -11.0f, 15.6f, -2.0f, COLOR_ARENISCA_T);
}


// Palmeras: tronco horneado (anillado y curvo) y copa animada.
struct PalmaT
{
    float x, z, alto, curva, giro, escala;
};

static const PalmaT PALMAS_T[] = {
    { -15.8f, -1.2f, 10.5f, 1.6f, 0.4f, 1.1f },
    { -21.5f, 1.0f, 8.6f, 2.0f, 2.6f, 1.0f },
    { -18.6f, 8.6f, 7.2f, 1.4f, 4.4f, 0.95f },
    { -25.5f, 6.0f, 11.0f, 1.2f, 1.2f, 1.05f },
    { 13.6f, -9.0f, 11.6f, 1.6f, 3.6f, 1.1f },
    { 23.5f, -3.0f, 9.4f, 2.2f, 5.6f, 1.0f },
    { 27.0f, 5.0f, 10.0f, 1.4f, 3.0f, 1.0f },
    { -31.0f, -10.0f, 12.5f, 1.0f, 0.8f, 1.05f },
    { 34.0f, -18.0f, 12.0f, 1.3f, 2.2f, 1.05f },
    { 40.0f, -26.0f, 11.0f, 1.0f, 4.0f, 1.0f },
    { -42.0f, -30.0f, 13.0f, 1.2f, 1.7f, 1.05f },
    { 46.0f, -40.0f, 12.0f, 1.4f, 5.0f, 1.0f },
};

static const int CANTIDAD_PALMAS_T = (int)(sizeof(PALMAS_T) / sizeof(PALMAS_T[0]));


static Vector3 PuntoTroncoT(const PalmaT& p, float s)
{
    float curva = p.curva * s * s;
    return { p.x + std::cos(p.giro) * curva, AlturaArenaT(p.x, p.z) + p.alto * s, p.z + std::sin(p.giro) * curva };
}


static int PalmasVisiblesT(bool decoracion)
{
    return decoracion ? CANTIDAD_PALMAS_T : 3;
}


static void TroncoPalmaT(const PalmaT& p, int indice)
{
    const int tramos = 9;

    for (int k = 0; k < tramos; k++)
    {
        float s0 = (float)k / (float)tramos, s1 = (float)(k + 1) / (float)tramos;
        float r0 = (0.42f - 0.16f * s0) * p.escala, r1 = (0.42f - 0.16f * s1) * p.escala;
        Color c = (k % 2) ? COLOR_TRONCO_T : TonoT(COLOR_TRONCO_T, 0.82f);
        CilT(PuntoTroncoT(p, s0), PuntoTroncoT(p, s1), r0 * 1.08f, r1, 7, VariarT(c, indice * 13 + k, 72, 0.05f), false);
    }

    // Datiles bajo la copa.
    Vector3 cima = PuntoTroncoT(p, 1.0f);
    BolaT({ cima.x + 0.3f, cima.y - 0.55f, cima.z + 0.2f }, { 0.35f, 0.42f, 0.35f }, Color{ 168, 96, 42, 255 }, 3, 6);
}


// Copa de hojas que se mece con el viento.
static void CopaPalmaT(const PalmaT& p, int indice, float t, float mov)
{
    Vector3 cima = PuntoTroncoT(p, 1.0f);
    const int hojas = 9;
    float viento = 0.08f * mov * std::sin(t * 1.3f + (float)indice * 0.9f);

    for (int h = 0; h < hojas; h++)
    {
        float ang = (float)h / (float)hojas * 2.0f * PI + p.giro * 0.7f + 0.2f * Ruido01T(indice * 20 + h, 73);
        float caida = 0.35f + 0.5f * Ruido01T(indice * 20 + h, 74) + (h % 3 == 0 ? -0.35f : 0.0f);
        float largo = (3.0f + 0.9f * Ruido01T(indice * 20 + h, 75)) * p.escala;
        Vector3 dir = { std::cos(ang), 0.0f, std::sin(ang) };
        Vector3 lat = { -dir.z, 0.0f, dir.x };
        Color verde = VariarT(COLOR_HOJA_T, indice * 20 + h, 76, 0.1f);
        Color verdeOsc = TonoT(verde, 0.72f);
        const int seg = 5;
        Vector3 prev = cima;

        for (int s = 1; s <= seg; s++)
        {
            float u = (float)s / (float)seg;
            float bajada = (caida + viento * (1.0f + std::cos(ang))) * u * u * largo * 0.9f;
            Vector3 q = { cima.x + dir.x * largo * u, cima.y + 0.45f * u * largo * 0.35f - bajada, cima.z + dir.z * largo * u };
            float ancho = 0.55f * std::sin(u * PI * 0.9f + 0.15f) * p.escala;

            // Foliolos a ambos lados (dientes que cuelgan).
            Vector3 l = Vector3Scale(lat, ancho);
            Vector3 medio = Vector3Scale(Vector3Add(prev, q), 0.5f);
            TriT(prev, q, { medio.x + l.x, medio.y - 0.3f * ancho, medio.z + l.z }, verde);
            TriT(prev, q, { medio.x - l.x, medio.y - 0.3f * ancho, medio.z - l.z }, verdeOsc);
            prev = q;
        }
    }
}


// Cisterna del oasis: estanque de piedra con juncos (agua animada aparte).
static const Vector3 CENTRO_CISTERNA_T = { -19.5f, 0.0f, 3.2f };
static const float RX_CISTERNA_T = 5.6f;
static const float RZ_CISTERNA_T = 3.9f;
static const float Y_AGUA_CISTERNA_T = -0.12f;


static Vector3 BordeCisternaT(float ang, float escala)
{
    return { CENTRO_CISTERNA_T.x + std::cos(ang) * RX_CISTERNA_T * escala, 0.0f, CENTRO_CISTERNA_T.z + std::sin(ang) * RZ_CISTERNA_T * escala };
}


static void CisternaT(bool decoracion)
{
    const int lados = 16;

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados, a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 i0 = BordeCisternaT(a0, 1.0f), i1 = BordeCisternaT(a1, 1.0f);
        Vector3 o0 = BordeCisternaT(a0, 1.14f), o1 = BordeCisternaT(a1, 1.14f);
        Color piedra = VariarT(COLOR_ARENISCA_CLARA_T, k, 77, 0.06f);
        Vector3 nf = Vector3Normalize({ std::cos(0.5f * (a0 + a1)), 0.0f, std::sin(0.5f * (a0 + a1)) });

        // Pretil: tapa, cara exterior y cara interior hasta el agua.
        QuadT({ i0.x, 0.3f, i0.z }, { o0.x, 0.3f, o0.z }, { o1.x, 0.3f, o1.z }, { i1.x, 0.3f, i1.z }, CaraT(piedra, { 0.0f, 1.0f, 0.0f }));
        QuadT({ o0.x, -0.45f, o0.z }, { o1.x, -0.45f, o1.z }, { o1.x, 0.3f, o1.z }, { o0.x, 0.3f, o0.z }, CaraT(COLOR_ARENISCA_T, nf));
        QuadT({ i0.x, -1.2f, i0.z }, { i1.x, -1.2f, i1.z }, { i1.x, 0.3f, i1.z }, { i0.x, 0.3f, i0.z }, CaraT(COLOR_ARENISCA_OSCURA_T, Vector3Negate(nf)));
    }

    // Fondo del estanque (se ve a traves del agua).
    DiscoT({ CENTRO_CISTERNA_T.x, -1.2f, CENTRO_CISTERNA_T.z }, RX_CISTERNA_T, RZ_CISTERNA_T, lados, Color{ 40, 96, 104, 255 }, Color{ 60, 88, 80, 255 });

    if (!decoracion) return;

    // Juncos y papiros en el borde.
    for (int k = 0; k < 26; k++)
    {
        float a = 2.0f * PI * Ruido01T(k, 78);
        Vector3 b = BordeCisternaT(a, 1.2f + 0.15f * Ruido01T(k, 79));
        float alto = 0.9f + 1.1f * Ruido01T(k, 80);
        float inclina = 0.25f * (Ruido01T(k, 81) - 0.5f);
        Color c = MezclaT(COLOR_HOJA_T, Color{ 150, 150, 70, 255 }, 0.4f * Ruido01T(k, 82));
        TriT({ b.x - 0.07f, 0.0f, b.z }, { b.x + 0.07f, 0.0f, b.z }, { b.x + inclina, alto, b.z + 0.05f }, c);
        TriT({ b.x, 0.0f, b.z - 0.07f }, { b.x, 0.0f, b.z + 0.07f }, { b.x - inclina, alto * 0.8f, b.z }, TonoT(c, 0.8f));
    }

    // Escalera de piedra al agua y cantaro olvidado.
    Vector3 e = BordeCisternaT(0.15f, 1.07f);
    CajaMMT(e.x - 0.6f, -0.45f, e.z - 0.6f, e.x + 0.4f, 0.32f, e.z + 0.6f, COLOR_ARENISCA_CLARA_T);
}


// Agua de la cisterna: degrade con el cielo, brillos que se mueven y la
// estela del sol (dinamica).
static void AguaCisternaT(float t, bool efectos)
{
    const int lados = 16;
    Color centro = Color{ 112, 176, 196, 210 };
    Color borde = Color{ 70, 130, 150, 230 };
    DiscoT({ CENTRO_CISTERNA_T.x, Y_AGUA_CISTERNA_T, CENTRO_CISTERNA_T.z }, RX_CISTERNA_T, RZ_CISTERNA_T, lados, centro, borde);

    if (!efectos) return;

    // Reflejos naranjas del cielo que tiemblan.
    for (int k = 0; k < 14; k++)
    {
        float u = Ruido01T(k, 83), v = Ruido01T(k, 84);
        float x = CENTRO_CISTERNA_T.x + (u - 0.5f) * RX_CISTERNA_T * 1.4f;
        float z = CENTRO_CISTERNA_T.z + (v - 0.5f) * RZ_CISTERNA_T * 1.3f;
        float largo = 0.6f + 0.9f * Ruido01T(k, 85);
        float b = 0.5f + 0.5f * std::sin(t * (1.6f + Ruido01T(k, 86)) + (float)k * 1.9f);
        float desliz = 0.25f * std::sin(t * 0.7f + (float)k);
        Color c = AlfaT(Color{ 255, 196, 140, 255 }, 0.5f * b);
        float y = Y_AGUA_CISTERNA_T + 0.01f;
        QuadColoresT({ x + desliz, y, z }, { x + desliz + largo, y, z }, { x + desliz + largo, y, z + 0.09f }, { x + desliz, y, z + 0.09f }, c, c, c, c);
    }
}


//--------------------------------------------------
// Campamento de caravana: tienda, camellos, carga
//--------------------------------------------------

static void TiendaT(float cx, float cz, float giro)
{
    float ca = std::cos(giro * DEG2RAD), sa = std::sin(giro * DEG2RAD);
    auto P = [&](float x, float y, float z) { return Vector3{ cx + x * ca + z * sa, y, cz - x * sa + z * ca }; };
    const float ancho = 3.6f, fondo = 2.6f, alto = 3.2f, borde = 1.3f;
    const Color franjas[2] = { Color{ 62, 52, 104, 255 }, Color{ 230, 206, 166, 255 } };
    const int n = 8;

    // Lona a dos aguas con franjas, del alero a la cumbrera.
    for (int k = 0; k < n; k++)
    {
        float x0 = -ancho + 2.0f * ancho * (float)k / (float)n;
        float x1 = -ancho + 2.0f * ancho * (float)(k + 1) / (float)n;
        Color c = franjas[k % 2];
        float hunde = 0.12f * std::sin((float)k / (float)n * PI * 4.0f);
        QuadT(P(x0, borde, fondo), P(x1, borde, fondo), P(x1, alto - hunde, 0.0f), P(x0, alto - hunde, 0.0f), CaraT(c, { 0.0f, 0.7f, 0.7f }));
        QuadT(P(x0, borde, -fondo), P(x0, alto - hunde, 0.0f), P(x1, alto - hunde, 0.0f), P(x1, borde, -fondo), CaraT(c, { 0.0f, 0.7f, -0.7f }));
    }

    // Laterales triangulares, fondo oscuro y alfombra en la entrada.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * ancho;
        TriT(P(x, borde, fondo), P(x, alto, 0.0f), P(x, borde, -fondo), CaraT(franjas[0], { (float)lado, 0.0f, 0.0f }));
        QuadT(P(x, 0.0f, fondo), P(x, 0.0f, -fondo), P(x, borde, -fondo), P(x, borde, fondo), CaraT(franjas[0], { (float)lado, 0.0f, 0.0f }));
    }

    QuadT(P(-ancho, 0.0f, -fondo), P(ancho, 0.0f, -fondo), P(ancho, borde, -fondo), P(-ancho, borde, -fondo), Color{ 40, 30, 50, 255 });
    QuadT(P(-ancho * 0.96f, 0.0f, fondo - 0.05f), P(ancho * 0.96f, 0.0f, fondo - 0.05f), P(ancho * 0.96f, borde, fondo - 0.05f), P(-ancho * 0.96f, borde, fondo - 0.05f), Color{ 46, 32, 46, 255 });
    QuadT(P(-1.4f, 0.02f, fondo), P(1.4f, 0.02f, fondo), P(1.4f, 0.02f, fondo + 1.8f), P(-1.4f, 0.02f, fondo + 1.8f), Color{ 150, 52, 64, 255 });
    QuadT(P(-1.1f, 0.03f, fondo + 0.25f), P(1.1f, 0.03f, fondo + 0.25f), P(1.1f, 0.03f, fondo + 1.55f), P(-1.1f, 0.03f, fondo + 1.55f), Color{ 220, 170, 90, 255 });

    // Postes, cumbrera y vientos.
    VigaT(P(-ancho - 0.2f, 0.0f, 0.0f), P(-ancho - 0.2f, alto + 0.5f, 0.0f), 0.12f, COLOR_MADERA_T);
    VigaT(P(ancho + 0.2f, 0.0f, 0.0f), P(ancho + 0.2f, alto + 0.5f, 0.0f), 0.12f, COLOR_MADERA_T);
    VigaT(P(-ancho - 0.2f, alto + 0.1f, 0.0f), P(ancho + 0.2f, alto + 0.1f, 0.0f), 0.1f, COLOR_MADERA_T);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        VigaT(P((float)lado * (ancho + 0.2f), alto + 0.3f, 0.0f), P((float)lado * (ancho + 2.0f), 0.0f, 0.6f), 0.035f, Color{ 220, 200, 160, 255 });
        VigaT(P((float)lado * ancho, borde, fondo), P((float)lado * (ancho + 0.9f), 0.0f, fondo + 1.0f), 0.03f, Color{ 220, 200, 160, 255 });
    }
}


// Camello echado o de pie, mirando hacia giro (grados).
static void CamelloT(float cx, float cz, float giro, bool echado, int semilla)
{
    float ca = std::cos(giro * DEG2RAD), sa = std::sin(giro * DEG2RAD);
    auto P = [&](float x, float y, float z) { return Vector3{ cx + x * ca + z * sa, AlturaArenaT(cx, cz) + 0.4f + y, cz - x * sa + z * ca }; };
    Color pelo = VariarT(Color{ 196, 146, 98, 255 }, semilla, 87, 0.06f);
    Color oscuro = TonoT(pelo, 0.72f);
    float h = echado ? 0.55f : 1.75f;

    BolaT(P(0.0f, h, 0.0f), { 1.15f, 0.55f, 0.5f }, pelo, 4, 10);
    BolaT(P(-0.05f, h + 0.42f, 0.0f), { 0.48f, 0.5f, 0.4f }, pelo, 3, 8);

    // Cuello y cabeza.
    VigaT(P(0.9f, h + 0.15f, 0.0f), P(1.45f, h + 0.95f, 0.0f), 0.3f, pelo);
    VigaT(P(1.45f, h + 0.95f, 0.0f), P(1.55f, h + 1.35f, 0.0f), 0.26f, pelo);
    CajaGiradaT(P(1.75f, h + 1.38f, 0.0f), { 0.55f, 0.26f, 0.26f }, giro, pelo);
    CajaGiradaT(P(1.6f, h + 1.56f, 0.0f), { 0.08f, 0.14f, 0.2f }, giro, oscuro);

    // Manta de viaje con franjas.
    CajaGiradaT(P(-0.05f, h + 0.3f, 0.0f), { 0.8f, 0.46f, 1.04f }, giro, Color{ 150, 52, 64, 255 });
    CajaGiradaT(P(-0.05f, h + 0.32f, 0.0f), { 0.22f, 0.47f, 1.06f }, giro, Color{ 230, 190, 100, 255 });

    if (echado)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            CajaGiradaT(P(0.55f, 0.12f, (float)lado * 0.45f), { 0.9f, 0.22f, 0.2f }, giro, oscuro);
            CajaGiradaT(P(-0.6f, 0.12f, (float)lado * 0.45f), { 0.9f, 0.22f, 0.2f }, giro, oscuro);
        }
    }
    else
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            VigaT(P(0.75f, h - 0.2f, (float)lado * 0.28f), P(0.8f, -0.4f, (float)lado * 0.3f), 0.17f, oscuro);
            VigaT(P(-0.75f, h - 0.2f, (float)lado * 0.28f), P(-0.8f, -0.4f, (float)lado * 0.3f), 0.17f, oscuro);
        }

        VigaT(P(-1.1f, h + 0.1f, 0.0f), P(-1.25f, h - 0.6f, 0.0f), 0.06f, oscuro);
    }
}


static void CajaCargaT(float x, float z, float tam, float giro, int semilla)
{
    Color madera = VariarT(Color{ 150, 100, 64, 255 }, semilla, 88, 0.08f);
    float y = AlturaArenaT(x, z) + 0.4f;
    CajaGiradaT({ x, y + tam * 0.5f, z }, { tam, tam, tam }, giro, madera);
    CajaGiradaT({ x, y + tam * 0.5f, z }, { tam * 1.02f, tam * 0.14f, tam * 1.02f }, giro, TonoT(madera, 0.7f));
}


static void SacoT(float x, float z, float r, int semilla)
{
    float y = AlturaArenaT(x, z) + 0.4f;
    Color tela = VariarT(Color{ 214, 186, 140, 255 }, semilla, 89, 0.07f);
    BolaT({ x, y + r * 0.6f, z }, { r, r * 0.7f, r * 0.85f }, tela, 3, 8);
    CilT({ x, y + r * 1.15f, z }, { x, y + r * 1.45f, z }, r * 0.3f, r * 0.12f, 6, TonoT(tela, 0.85f));
}


static void VasijaT(float x, float y, float z, float escala, Color color, int lados)
{
    const float r[7] = { 0.25f, 0.42f, 0.5f, 0.38f, 0.2f, 0.18f, 0.24f };
    const float h[7] = { 0.0f, 0.15f, 0.45f, 0.8f, 0.98f, 1.12f, 1.18f };
    float rr[7], hh[7];

    for (int k = 0; k < 7; k++) { rr[k] = r[k] * escala; hh[k] = h[k] * escala; }

    TornoT({ x, y, z }, rr, hh, 7, lados, color);
}


static void CampamentoT(bool decoracion, int paso)
{
    TiendaT(19.0f, -4.6f, -18.0f);
    CamelloT(16.2f, 3.4f, 190.0f, true, 1);

    if (decoracion)
    {
        CamelloT(22.4f, 1.0f, 172.0f, false, 2);
        TiendaT(29.0f, -12.0f, 12.0f);
    }

    // Carga junto a la tienda.
    CajaCargaT(14.6f, -1.6f, 0.9f, 12.0f, 1);
    CajaCargaT(15.4f, -1.2f, 0.7f, -20.0f, 2);
    CajaCargaT(14.9f, -1.5f + 0.0f, 0.55f, 40.0f, 3);
    SacoT(13.9f, 0.2f, 0.45f, 1);
    SacoT(14.6f, 0.6f, 0.38f, 2);

    if (paso == 1)
    {
        SacoT(17.8f, -0.4f, 0.42f, 3);
        CajaCargaT(21.4f, -0.8f, 0.8f, 5.0f, 4);
    }

    // Vasijas apiladas al costado de la terraza.
    int lados = decoracion ? 10 : 7;
    VasijaT(-13.4f, 0.0f, 0.6f, 1.2f, COLOR_BARRO_T, lados);
    VasijaT(-14.3f, 0.0f, 1.3f, 0.9f, TonoT(COLOR_BARRO_T, 1.1f), lados);
    VasijaT(13.4f, 0.0f, 0.9f, 1.1f, Color{ 196, 120, 74, 255 }, lados);
    VasijaT(13.0f, 0.0f, 2.0f, 0.8f, COLOR_BARRO_T, lados);
}


// Braseros de bronce sobre pedestal en las esquinas de la terraza.
static const Vector3 BRASEROS_T[2] = { { -11.6f, 0.0f, 5.6f }, { 11.6f, 0.0f, 5.6f } };


static void BraseroT(Vector3 b)
{
    CajaMMT(b.x - 0.5f, 0.0f, b.z - 0.5f, b.x + 0.5f, 0.3f, b.z + 0.5f, COLOR_ARENISCA_CLARA_T);
    CilT({ b.x, 0.3f, b.z }, { b.x, 1.5f, b.z }, 0.3f, 0.22f, 8, COLOR_ARENISCA_T);
    CilT({ b.x, 1.5f, b.z }, { b.x, 1.7f, b.z }, 0.34f, 0.36f, 8, COLOR_ARENISCA_CLARA_T);
    CilT({ b.x, 1.7f, b.z }, { b.x, 2.15f, b.z }, 0.3f, 0.62f, 10, Color{ 150, 96, 50, 255 });
    DiscoT({ b.x, 2.1f, b.z }, 0.52f, 0.52f, 10, Color{ 255, 150, 60, 255 }, Color{ 120, 40, 20, 255 });
}


// Farol de bronce colgado de la mensula de cada pilastra.
static Vector3 PosicionFarolT(int lado)
{
    float xm = lado < 0 ? -11.3f : 11.3f;
    return { xm + (float)lado * 0.95f, 9.6f, -2.6f };
}


static void FarolT(Vector3 p)
{
    VigaT({ p.x, p.y + 1.0f, p.z }, { p.x, p.y + 0.45f, p.z }, 0.03f, Color{ 60, 50, 46, 255 });
    CilT({ p.x, p.y + 0.45f, p.z }, { p.x, p.y + 0.3f, p.z }, 0.08f, 0.3f, 8, Color{ 150, 100, 50, 255 });
    CilT({ p.x, p.y - 0.25f, p.z }, { p.x, p.y + 0.3f, p.z }, 0.24f, 0.28f, 8, Color{ 255, 206, 120, 255 });
    CilT({ p.x, p.y - 0.4f, p.z }, { p.x, p.y - 0.25f, p.z }, 0.12f, 0.26f, 8, Color{ 150, 100, 50, 255 });
}


//--------------------------------------------------
// Horizonte: dunas en dos planos y aldea lejana
//--------------------------------------------------

static void DunasT(float zBase, float alto, float paso, int semilla, Color luz, Color sombra, float brumaExtra)
{
    g_brumaExtraT = brumaExtra;
    float xSol = std::tan(22.0f * DEG2RAD) * (g_ojoT.z - zBase);

    for (float x = -460.0f; x < 460.0f; x += paso)
    {
        auto Cresta = [&](float xx)
        {
            float a = RuidoSuaveT(xx, 0.0f, 70.0f, semilla);
            float b = RuidoSuaveT(xx, 0.0f, 23.0f, semilla + 1);
            float h = alto * (0.35f + 0.65f * a) * (0.75f + 0.25f * b);
            float valle = 0.45f + 0.55f * Lim01T(std::fabs(xx - xSol) / 90.0f);
            return h * valle;
        };
        float h0 = Cresta(x), h1 = Cresta(x + paso);
        float z0 = zBase + 14.0f * (RuidoSuaveT(x, 0.0f, 50.0f, semilla + 2) - 0.5f);
        float z1 = zBase + 14.0f * (RuidoSuaveT(x + paso, 0.0f, 50.0f, semilla + 2) - 0.5f);
        float pendiente = (h1 - h0) / paso;
        Color cara = MezclaT(sombra, luz, 0.5f - 1.8f * pendiente);
        Color cresta = MezclaT(cara, Color{ 255, 190, 120, 255 }, 0.35f);
        Vector3 a = { x, -0.5f, z0 + 30.0f }, b = { x + paso, -0.5f, z1 + 30.0f };
        Vector3 c = { x + paso, h1, z1 }, d = { x, h0, z0 };
        Quad4T(a, b, c, d, TonoT(cara, 0.86f), TonoT(cara, 0.86f), cresta, cresta);
        // Ladera de atras (cierra la silueta contra el cielo).
        Quad4T(d, c, { x + paso, -0.5f, z1 - 26.0f }, { x, -0.5f, z0 - 26.0f }, cresta, cresta, sombra, sombra);
    }

    g_brumaExtraT = 0.0f;
}


static void CasaAdobeT(float x, float z, float ancho, float alto, float fondo, int semilla)
{
    Color adobe = VariarT(Color{ 206, 140, 98, 255 }, semilla, 91, 0.06f);
    float y = AlturaArenaT(x, z);
    CajaMMT(x - ancho * 0.5f, y - 0.5f, z - fondo * 0.5f, x + ancho * 0.5f, y + alto, z + fondo * 0.5f, adobe);
    CajaMMT(x - ancho * 0.52f, y + alto, z - fondo * 0.52f, x + ancho * 0.52f, y + alto + 0.35f, z + fondo * 0.52f, TonoT(adobe, 1.08f));

    // Ventanitas oscuras.
    for (int k = 0; k < 2; k++)
    {
        float wx = x - ancho * 0.25f + (float)k * ancho * 0.5f;
        FrenteT(wx - 0.3f, y + alto * 0.55f, wx + 0.3f, y + alto * 0.55f + 0.7f, z + fondo * 0.5f + 0.02f, Color{ 70, 40, 50, 255 });
    }
}


static void AljibeT(float x, float z)
{
    float y = AlturaArenaT(x, z);
    Color piedra = COLOR_ARENISCA_T;
    CilT({ x, y - 0.5f, z }, { x, y + 13.0f, z }, 3.4f, 3.0f, 14, piedra, false);
    CilT({ x, y + 13.0f, z }, { x, y + 13.6f, z }, 3.3f, 3.3f, 14, COLOR_ARENISCA_CLARA_T);
    BolaT({ x, y + 13.6f, z }, { 2.8f, 3.2f, 2.8f }, Color{ 236, 214, 186, 255 }, 4, 14, 0.0f);
    CilT({ x, y + 16.6f, z }, { x, y + 17.8f, z }, 0.12f, 0.02f, 6, COLOR_ORO_T);

    for (int k = 0; k < 3; k++)
    {
        float a = -0.4f + (float)k * 0.55f + PI * 0.5f;
        float wy = y + 4.0f + (float)k * 3.0f;
        Vector3 c = { x + std::cos(a) * 3.12f, wy, z + std::sin(a) * 3.12f };
        CajaT(c, { 0.55f, 1.1f, 0.2f }, Color{ 70, 40, 50, 255 });
    }
}


static void HorizonteT(bool decoracion)
{
    // Dunas lejanas (mas calima) y cercanas.
    DunasT(-205.0f, 30.0f, decoracion ? 14.0f : 28.0f, 301, Color{ 236, 150, 116, 255 }, Color{ 150, 92, 122, 255 }, 0.22f);
    DunasT(-95.0f, 13.0f, decoracion ? 9.0f : 18.0f, 311, Color{ 246, 160, 96, 255 }, Color{ 168, 96, 98, 255 }, 0.0f);

    if (!decoracion) return;

    // Aldea de adobe y aljibe a la derecha, media distancia.
    AljibeT(31.0f, -30.0f);
    CasaAdobeT(42.0f, -36.0f, 6.0f, 4.5f, 5.0f, 1);
    CasaAdobeT(49.0f, -32.0f, 5.0f, 6.5f, 5.0f, 2);
    CasaAdobeT(38.0f, -44.0f, 7.0f, 3.8f, 5.0f, 3);
    CasaAdobeT(-48.0f, -48.0f, 6.0f, 4.0f, 5.0f, 4);
}


static void EscenaEstaticaT(bool decoracion, int paso);


static void HornearEscenaT()
{
    int clave = (int)CalidadDecoracion();

    if ((g_hornoT.escenaValida && g_hornoT.claveEscena == clave) || !IsWindowReady()) return;

    LiberarMallaHorneada(g_hornoT.escena, g_hornoT.escenaValida);
    AsegurarMaterialT();

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Resplandor de braseros y faroles horneado en lo que los rodea.
    g_cantidadLucesT = 0;
    g_lucesT[g_cantidadLucesT++] = { { -11.6f, 2.2f, 5.6f }, 5.0f, Color{ 150, 80, 30, 255 } };
    g_lucesT[g_cantidadLucesT++] = { { 11.6f, 2.2f, 5.6f }, 5.0f, Color{ 150, 80, 30, 255 } };

    EmpezarGrabacionMalla(g_grabT);
    EscenaEstaticaT(decoracion, paso);
    g_hornoT.escenaValida = TerminarGrabacionMalla(g_grabT, g_hornoT.escena);
    g_hornoT.claveEscena = clave;
    g_cantidadLucesT = 0;
}


//--------------------------------------------------
// Cielo de atardecer y sol bajo
//--------------------------------------------------

static Color ColorCieloT(float elevacion)
{
    const float el[6] = { -8.0f, 1.0f, 5.0f, 12.0f, 22.0f, 38.0f };
    const Color co[6] = { COLOR_HORIZONTE_T, COLOR_HORIZONTE_T, COLOR_CIELO_BAJO_T, COLOR_CIELO_ROSA_T, COLOR_CIELO_MEDIO_T, COLOR_CIELO_ALTO_T };

    if (elevacion <= el[0]) return co[0];

    for (int k = 0; k < 5; k++)
    {
        if (elevacion <= el[k + 1]) return MezclaT(co[k], co[k + 1], (elevacion - el[k]) / (el[k + 1] - el[k]));
    }

    return co[5];
}


static Vector3 DireccionSolT()
{
    float el = 2.6f * DEG2RAD;
    float az = 22.0f * DEG2RAD;
    return { std::sin(az) * std::cos(el), std::sin(el), -std::cos(az) * std::cos(el) };
}


static void DibujarSolT(const BaseCamaraT& final)
{
    Vector3 sol = Vector3Add(final.ojo, Vector3Scale(DireccionSolT(), 600.0f));
    float r = 600.0f * std::tan(3.4f * DEG2RAD);

    g_sinBrumaT = true;
    LoteT();
    HaloT(sol, r, COLOR_SOL_T, Color{ 255, 214, 140, 255 }, 32);
    FinLoteT();
    g_sinBrumaT = false;
}


static void DibujarCieloT(const Camera3D& vista)
{
    BaseCamaraT b = BaseDeT(vista);
    const int filas = 14, columnas = 6;
    Vector3 dSol = DireccionSolT();

    g_sinBrumaT = true;
    LoteT();

    for (int i = 0; i < filas; i++)
    {
        float sy0 = -1.25f + 2.5f * (float)i / (float)filas;
        float sy1 = -1.25f + 2.5f * (float)(i + 1) / (float)filas;

        for (int j = 0; j < columnas; j++)
        {
            float sx0 = -1.25f + 2.5f * (float)j / (float)columnas;
            float sx1 = -1.25f + 2.5f * (float)(j + 1) / (float)columnas;
            Vector3 d[4] = { DireccionT(b, sx0, sy0), DireccionT(b, sx1, sy0), DireccionT(b, sx1, sy1), DireccionT(b, sx0, sy1) };
            Color c[4];
            Vector3 p[4];

            for (int k = 0; k < 4; k++)
            {
                c[k] = ColorCieloT(std::asin(d[k].y) * RAD2DEG);
                // Mas calido hacia el sol.
                float haciaSol = Lim01T((Vector3DotProduct(d[k], dSol) - 0.75f) / 0.25f);
                c[k] = MezclaT(c[k], COLOR_HORIZONTE_T, 0.45f * haciaSol * haciaSol);
                p[k] = Vector3Add(b.ojo, Vector3Scale(d[k], 650.0f));
            }

            QuadColoresT(p[0], p[1], p[2], p[3], c[0], c[1], c[2], c[3]);
        }
    }

    FinLoteT();
    g_sinBrumaT = false;
}


// Nubes alargadas encendidas de rosa y primeras estrellas (alfa).
static void DibujarNubesT(const BaseCamaraT& final, float t, bool decoracion)
{
    int nubes = decoracion ? 7 : 3;
    float mov = ObtenerFactorMovimientoCamaraMinijuegos() > 0.5f ? 1.0f : 0.3f;

    LoteT();

    for (int i = 0; i < nubes; i++)
    {
        float sy = 0.38f + 0.75f * Ruido01T(i, 211);
        float sx = std::fmod(Ruido01T(i, 212) * 3.6f + t * (0.004f + 0.004f * Ruido01T(i, 213)) * mov, 3.6f) - 1.8f;
        Vector3 c = Vector3Add(final.ojo, Vector3Scale(DireccionT(final, sx, sy), 590.0f));
        float rx = 90.0f + 110.0f * Ruido01T(i, 214), ry = 5.0f + 5.0f * Ruido01T(i, 215);
        float altura = sy;
        Color dentro = MezclaT(Color{ 255, 170, 150, 200 }, Color{ 170, 96, 150, 170 }, Lim01T((altura - 0.4f) / 0.7f));
        Color fuera = AlfaT(dentro, 0.0f);

        for (int lobulo = 0; lobulo < 3; lobulo++)
        {
            Vector3 o = Vector3Add(c, Vector3Add(Vector3Scale(g_derT, ((float)lobulo - 1.0f) * rx * 0.5f), Vector3Scale(g_arribaT, (lobulo == 1 ? 0.4f : 0.0f) * ry)));
            float lx = rx * (lobulo == 1 ? 0.62f : 0.45f), ly = ry * (lobulo == 1 ? 1.2f : 0.8f);

            for (int k = 0; k < 14; k++)
            {
                float a0 = 2.0f * PI * (float)k / 14.0f, a1 = 2.0f * PI * (float)(k + 1) / 14.0f;
                Vector3 p0 = Vector3Add(o, Vector3Add(Vector3Scale(g_derT, std::cos(a0) * lx), Vector3Scale(g_arribaT, std::sin(a0) * ly)));
                Vector3 p1 = Vector3Add(o, Vector3Add(Vector3Scale(g_derT, std::cos(a1) * lx), Vector3Scale(g_arribaT, std::sin(a1) * ly)));
                VT(o, dentro); VT(p0, fuera); VT(p1, fuera);
            }
        }
    }

    FinLoteT();
}


static void DibujarBrilloCieloT(const BaseCamaraT& final, float t, bool decoracion)
{
    Vector3 sol = Vector3Add(final.ojo, Vector3Scale(DireccionSolT(), 600.0f));
    float r = 600.0f * std::tan(3.4f * DEG2RAD);

    LoteT();
    HaloT(sol, r * 6.0f, Color{ 255, 150, 80, 60 }, Color{ 255, 120, 80, 0 }, 32);
    HaloT(sol, r * 2.4f, Color{ 255, 220, 150, 90 }, Color{ 255, 190, 120, 0 }, 32);

    int estrellas = decoracion ? 46 : 18;

    for (int k = 0; k < estrellas; k++)
    {
        float sx = -1.3f + 2.6f * Ruido01T(k, 201);
        float sy = 0.72f + 0.5f * Ruido01T(k, 202);
        Vector3 d = DireccionT(final, sx, sy);
        float brillo = 0.55f + 0.45f * std::sin(t * (1.5f + 2.5f * Ruido01T(k, 203)) + (float)k * 2.1f);
        float tam = 0.5f + 0.7f * Ruido01T(k, 204);
        PuntoT(Vector3Add(final.ojo, Vector3Scale(d, 600.0f)), tam, AlfaT(Color{ 255, 236, 220, 255 }, (0.25f + 0.5f * brillo) * Lim01T((sy - 0.72f) / 0.2f + 0.3f)));
    }

    FinLoteT();
}


//--------------------------------------------------
// Partes vivas: compuertas, agua, cantaros y selectores
//--------------------------------------------------

static void DibujarCompuertasT(const MinijuegoTuberiasDesierto& m)
{
    for (int e = 0; e < m.cantidadEntradas; e++)
    {
        float x = XCarril(m.cantidadEntradas, e) + 0.98f * SuaveT(m.compuerta);
        CajaMMT(x - 0.47f, Y_ENTRADA - 0.24f, Z_TUBERIA + 0.4f, x + 0.47f, Y_COLECTOR_T - 0.44f, Z_TUBERIA + 0.54f, COLOR_MADERA_T);
        CajaMMT(x - 0.47f, Y_ENTRADA - 0.05f, Z_TUBERIA + 0.54f, x + 0.47f, Y_ENTRADA + 0.05f, Z_TUBERIA + 0.58f, TonoT(COLOR_MADERA_T, 0.7f));
        CajaMMT(x - 0.08f, Y_COLECTOR_T - 0.44f, Z_TUBERIA + 0.44f, x + 0.08f, Y_COLECTOR_T - 0.3f, Z_TUBERIA + 0.6f, Color{ 90, 84, 80, 255 });
    }
}


static Color ColorAguaTuboT(const MinijuegoTuberiasDesierto& m, int e)
{
    if (m.fase != FASE_TUBERIAS_REVELAR) return COLOR_AGUA_T;

    float t = Lim01T(m.tiempoFase / 0.6f);

    if (e == m.entradaGanadora)
    {
        float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 7.0f);
        return MezclaT(COLOR_AGUA_T, MezclaT(COLOR_ORO_T, COLOR_ORO_CLARO_T, pulso * 0.6f), t);
    }

    return MezclaT(COLOR_AGUA_T, Color{ 92, 120, 150, 255 }, 0.55f * t);
}


static void DibujarAguaT(const MinijuegoTuberiasDesierto& m)
{
    if (m.progresoAgua <= 0.0f) return;

    float uAgua = UAguaT(m);
    float uPico = (float)m.etapas + U_PICO_T;
    float t = m.tiempoAnimacion;

    LoteT();

    for (int e = 0; e < m.cantidadEntradas; e++)
    {
        Color c = ColorAguaTuboT(m, e);
        float brillo = m.fase == FASE_TUBERIAS_REVELAR && e != m.entradaGanadora ? 0.15f : 0.55f;
        CintaTuberiaT(m, e, U_MIN_T, uAgua < uPico ? uAgua : uPico, R_TUBO_T * 1.1f, R_TUBO_T * 0.9f, c, brillo, t);

        // Chorro del pico a la boca del cantaro.
        if (uAgua > uPico)
        {
            Vector3 p = PuntoTuberia(m, e, uPico);
            float largo = Lim01T((uAgua - uPico) / (UMaxT(m) - uPico));
            float yA = p.y - 0.36f, zA = Z_TUBERIA + 0.86f;
            float yB = Y_REPISA + 1.12f;
            float yFin = yA + (yB - yA) * largo;
            float alfa = m.fase == FASE_TUBERIAS_REVELAR ? 1.0f - Lim01T((m.tiempoFase - 1.4f) / 0.8f) : 1.0f;

            if (alfa > 0.01f)
            {
                float ondula = 0.03f * std::sin(t * 18.0f);
                Color ca = AlfaT(c, 0.9f * alfa), cb = AlfaT(COLOR_AGUA_CLARA_T, 0.8f * alfa);
                QuadColoresT({ p.x - 0.12f, yA, zA }, { p.x + 0.12f, yA, zA },
                    { p.x + 0.09f + ondula, yFin, Z_CANTARO_T + 0.1f }, { p.x - 0.09f + ondula, yFin, Z_CANTARO_T + 0.1f }, ca, ca, cb, cb);
            }
        }
    }

    FinLoteT();
}


// Perfil de cantaro (radios y alturas sobre la repisa).
static const int PERFIL_CANTARO_T = 9;
static const float R_CANTARO_T[PERFIL_CANTARO_T] = { 0.3f, 0.46f, 0.62f, 0.66f, 0.56f, 0.34f, 0.24f, 0.27f, 0.33f };
static const float Y_PERFIL_CANTARO_T[PERFIL_CANTARO_T] = { 0.0f, 0.06f, 0.26f, 0.48f, 0.72f, 0.9f, 1.0f, 1.08f, 1.14f };


static void CuerpoCantaroT(Vector3 base, Color color, Color banda, float escala, const float* despl, int lados)
{
    float r[PERFIL_CANTARO_T], y[PERFIL_CANTARO_T];

    for (int k = 0; k < PERFIL_CANTARO_T; k++)
    {
        r[k] = R_CANTARO_T[k] * escala;
        y[k] = Y_PERFIL_CANTARO_T[k] * escala;
    }

    TornoT(base, r, y, 3, lados, color, despl);
    // Banda pintada en el vientre.
    TornoT(base, r + 2, y + 2, 2, lados, banda, despl ? despl + 2 : nullptr);
    TornoT(base, r + 3, y + 3, PERFIL_CANTARO_T - 3, lados, color, despl ? despl + 3 : nullptr);

    // Asas.
    float dx = despl ? despl[5] : 0.0f;
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = base.x + dx + (float)lado * 0.42f * escala;
        VigaT({ x, base.y + 1.02f * escala, base.z }, { x + (float)lado * 0.2f * escala, base.y + 0.88f * escala, base.z }, 0.08f * escala, color);
        VigaT({ x + (float)lado * 0.2f * escala, base.y + 0.88f * escala, base.z }, { x + (float)lado * 0.12f * escala, base.y + 0.62f * escala, base.z }, 0.08f * escala, color);
    }
}


static void DibujarCantarosT(const MinijuegoTuberiasDesierto& m, bool detalle)
{
    bool visibles = TuberiasVisiblesT(m);
    int lados = detalle ? 12 : 8;
    float t = m.tiempoAnimacion;

    LoteT();

    for (int lugar = 0; lugar < m.cantidadEntradas; lugar++)
    {
        float x = XCarril(m.cantidadEntradas, lugar);
        Vector3 base = { x, Y_REPISA, Z_CANTARO_T };
        bool oro = visibles && lugar == m.oroCantaro;
        bool miraje = visibles && lugar == m.mirajeCantaro;

        // Base de piedra bajo cada cantaro.
        CilT({ x, Y_REPISA - 0.01f, Z_CANTARO_T }, { x, Y_REPISA + 0.06f, Z_CANTARO_T }, 0.62f, 0.58f, lados, COLOR_ARENISCA_OSCURA_T);

        if (miraje) continue;   // se dibuja aparte, translucido

        float salto = 0.0f;

        if (oro && m.fase == FASE_TUBERIAS_REVELAR)
        {
            float u = m.tiempoFase * 3.0f;
            salto = u < PI ? 0.25f * std::sin(u) * ObtenerFactorMovimientoCamaraMinijuegos() : 0.0f;
        }

        Color cuerpo = oro ? COLOR_ORO_T : VariarT(COLOR_BARRO_T, lugar, 51, 0.05f);
        Color banda = oro ? Color{ 196, 118, 30, 255 } : Color{ 236, 206, 160, 255 };
        CuerpoCantaroT({ base.x, base.y + 0.06f + salto, base.z }, cuerpo, banda, 1.0f, nullptr, lados);

        // Agua (o agua dorada) en la boca al llegar.
        if (m.progresoAgua >= 1.0f)
        {
            Color agua = oro ? COLOR_ORO_CLARO_T : COLOR_AGUA_T;
            DiscoT({ x, base.y + 0.06f + salto + 1.1f, Z_CANTARO_T }, 0.25f, 0.25f, 10, COLOR_AGUA_CLARA_T, agua, false);
        }

        // Destellos del cantaro dorado.
        if (oro)
        {
            for (int k = 0; k < 3; k++)
            {
                float a = t * 1.6f + (float)k * 2.09f;
                float vida = std::fmod(t * 0.9f + (float)k * 0.33f, 1.0f);
                float tam = 0.09f * std::sin(vida * PI);
                PuntoT({ x + std::cos(a) * 0.5f, base.y + 0.5f + 0.5f * vida, Z_CANTARO_T + 0.7f }, tam, COLOR_ORO_CLARO_T);
            }
        }
    }

    FinLoteT();
}


// Espejismo: cantaro dorado translucido que ondula como el aire caliente.
// Al revelarse se deshace en un remolino de arena.
static void DibujarEspejismoT(const MinijuegoTuberiasDesierto& m, bool detalle)
{
    if (!TuberiasVisiblesT(m)) return;

    float t = m.tiempoAnimacion;
    float x = XCarril(m.cantidadEntradas, m.mirajeCantaro);
    Vector3 base = { x, Y_REPISA + 0.06f, Z_CANTARO_T };
    float mov = ObtenerFactorMovimientoCamaraMinijuegos() > 0.5f ? 1.0f : 0.4f;

    LoteT();

    if (!m.mirajeRevelado)
    {
        float despl[PERFIL_CANTARO_T];

        for (int k = 0; k < PERFIL_CANTARO_T; k++)
        {
            despl[k] = 0.09f * mov * std::sin(t * 7.0f + (float)k * 1.3f);
        }

        // Dorado que se lava con el reflejo del cielo, como el aire caliente.
        float alfa = 0.46f + 0.12f * std::sin(t * 3.1f);
        float cielo = 0.2f + 0.16f * std::sin(t * 2.3f);
        Color oro = AlfaT(MezclaT(MezclaT(COLOR_ORO_T, COLOR_ORO_CLARO_T, 0.4f), Color{ 196, 214, 255, 255 }, cielo), alfa);
        CuerpoCantaroT(base, oro, AlfaT(Color{ 214, 196, 240, 255 }, alfa), 1.0f, despl, detalle ? 12 : 8);

        // Charco brillante bajo el cantaro (el falso «agua» del espejismo).
        for (int k = 0; k < 4; k++)
        {
            float y = base.y + 0.005f * (float)k;
            float ancho = 0.9f + 0.15f * std::sin(t * 4.0f + (float)k * 1.7f);
            Color c = AlfaT(Color{ 210, 226, 255, 255 }, 0.22f - 0.04f * (float)k);
            float o = 0.06f * std::sin(t * 5.0f + (float)k * 2.0f) * mov;
            QuadColoresT({ x - ancho + o, y, Z_CANTARO_T + 0.35f + 0.12f * (float)k }, { x + ancho + o, y, Z_CANTARO_T + 0.35f + 0.12f * (float)k },
                { x + ancho * 0.8f + o, y, Z_CANTARO_T + 0.43f + 0.12f * (float)k }, { x - ancho * 0.8f + o, y, Z_CANTARO_T + 0.43f + 0.12f * (float)k }, c, c, c, c);
        }

        // Ondas de calor que suben.
        for (int k = 0; k < 3; k++)
        {
            float vida = std::fmod(t * 0.55f + (float)k / 3.0f, 1.0f);
            float y = base.y + 0.5f + 1.6f * vida;
            float a = 0.35f * std::sin(vida * PI);
            Color c = AlfaT(Color{ 255, 236, 190, 255 }, a);

            for (int s = 0; s < 6; s++)
            {
                float xa = x - 0.6f + 0.2f * (float)s, xb = xa + 0.2f;
                float ya = y + 0.07f * std::sin(t * 6.0f + (float)s * 1.4f + (float)k);
                float yb = y + 0.07f * std::sin(t * 6.0f + (float)(s + 1) * 1.4f + (float)k);
                QuadColoresT({ xa, ya, Z_CANTARO_T + 0.75f }, { xb, yb, Z_CANTARO_T + 0.75f },
                    { xb, yb + 0.05f, Z_CANTARO_T + 0.75f }, { xa, ya + 0.05f, Z_CANTARO_T + 0.75f }, c, c, c, c);
            }
        }
    }
    else
    {
        float vida = Lim01T(m.tiempoFase / 1.4f);

        for (int k = 0; k < 14; k++)
        {
            float a = (float)k * 0.9f + t * 2.0f * mov;
            float r = 0.3f + 0.9f * vida * (0.6f + 0.4f * Ruido01T(k, 61));
            float y = base.y + 0.3f + 1.8f * vida * Ruido01T(k, 62) + 0.4f * vida;
            PuntoT({ x + std::cos(a) * r, y, Z_CANTARO_T + std::sin(a) * r * 0.5f + 0.4f },
                0.12f * (1.0f - 0.6f * vida), AlfaT(Color{ 236, 196, 140, 255 }, 1.0f - vida));
        }
    }

    FinLoteT();
}


// Posicion del selector de un jugador (los que coinciden se reparten).
static Vector3 PosicionSelectorT(const MinijuegoTuberiasDesierto& m, int i, int limite, int* puestoSalida = nullptr)
{
    int mismos = 0, puesto = 0;

    for (int j = 0; j < limite; j++)
    {
        if (m.estadosJugadores[j].participa && m.estadosJugadores[j].seleccion == m.estadosJugadores[i].seleccion)
        {
            if (j < i) puesto++;
            mismos++;
        }
    }

    if (puestoSalida) *puestoSalida = puesto;

    float x = XCarril(m.cantidadEntradas, m.estadosJugadores[i].seleccion) + ((float)puesto - 0.5f * (float)(mismos - 1)) * 0.62f;
    return { x, Y_COLECTOR_T + 1.0f, Z_COLECTOR_T + 1.1f };
}


static bool SelectoresVisiblesT(const MinijuegoTuberiasDesierto& m)
{
    return TuberiasVisiblesT(m) && m.fase != FASE_TUBERIAS_TERMINADO;
}


// Flecha del color del jugador que apunta a la entrada elegida.
static void FlechaSelectorT(Vector3 c, Color color, float escala, bool fijada)
{
    float w = 0.34f * escala, h = 0.52f * escala, d = 0.12f * escala;
    Vector3 a = { c.x - w, c.y, c.z }, b = { c.x + w, c.y, c.z }, p = { c.x, c.y - h, c.z };
    Vector3 cuello0 = { c.x - w * 0.45f, c.y, c.z }, cuello1 = { c.x + w * 0.45f, c.y, c.z };
    Color frente = MezclaT(color, WHITE, 0.15f);

    if (fijada)
    {
        // Borde claro detras: «elegida».
        float g = 0.1f * escala;
        TriT({ a.x - g * 1.6f, a.y + g, c.z - d - 0.02f }, { b.x + g * 1.6f, b.y + g, c.z - d - 0.02f }, { p.x, p.y - g * 1.8f, c.z - d - 0.02f }, Color{ 255, 248, 226, 255 });
        CajaMMT(cuello0.x - g, c.y - 0.02f, c.z - d - 0.03f, cuello1.x + g, c.y + 0.42f * escala + g, c.z - d - 0.02f, Color{ 255, 248, 226, 255 });
    }

    g_sinBrumaT = true;
    TriT({ a.x, a.y, c.z + d }, { b.x, b.y, c.z + d }, { p.x, p.y, c.z + d }, frente);
    TriT({ a.x, a.y, c.z - d }, { p.x, p.y, c.z - d }, { b.x, b.y, c.z - d }, TonoT(color, 0.6f));
    QuadT({ a.x, a.y, c.z - d }, { a.x, a.y, c.z + d }, { p.x, p.y, c.z + d }, { p.x, p.y, c.z - d }, TonoT(color, 0.85f));
    QuadT({ b.x, b.y, c.z + d }, { b.x, b.y, c.z - d }, { p.x, p.y, c.z - d }, { p.x, p.y, c.z + d }, TonoT(color, 0.62f));
    QuadT({ a.x, a.y, c.z - d }, { b.x, b.y, c.z - d }, { b.x, b.y, c.z + d }, { a.x, a.y, c.z + d }, TonoT(color, 1.0f));
    g_sinBrumaT = false;
    CajaMMT(cuello0.x, c.y - 0.01f, c.z - d * 0.8f, cuello1.x, c.y + 0.42f * escala, c.z + d * 0.8f, color);
}


static void DibujarSelectoresT(const MinijuegoTuberiasDesierto& m, const Participante participantes[], int limite)
{
    if (!SelectoresVisiblesT(m)) return;

    float t = m.tiempoAnimacion;
    float mov = ObtenerFactorMovimientoCamaraMinijuegos() > 0.5f ? 1.0f : 0.25f;

    LoteT();

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorTuberias& estado = m.estadosJugadores[i];

        if (!estado.participa) continue;

        Vector3 c = PosicionSelectorT(m, i, limite);
        bool fijada = estado.bloqueado;

        if (!fijada && m.fase == FASE_TUBERIAS_ELEGIR)
        {
            c.y += 0.14f * std::sin(t * 6.0f + (float)i) * mov;
        }

        FlechaSelectorT(c, participantes[i].color, 1.0f, fijada);
    }

    FinLoteT();
}


//--------------------------------------------------
// Anuncios, etiquetas y HUD
//--------------------------------------------------

static int NumeroJugadorT(const Participante participantes[], int i)
{
    return participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
}


static void TextoCentradoT(const char* texto, float x, float yCentro, int token, Color color, float alfa = 1.0f)
{
    DibujarTextoMundoContornoUI(texto, x, yCentro - (float)TamanoTexto(token) * 0.5f, token, color, alfa);
}


// Anuncio grande que entra con rebote y se va fundiendo.
static void AnuncioFaseT(const char* texto, const char* bajada, float edad, float duracion, Color color)
{
    if (edad < 0.0f || edad > duracion) return;

    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    float entrada = Lim01T(edad / 0.18f);
    float escala = 0.6f + 0.4f * entrada + 0.08f * std::sin(entrada * PI) * ObtenerFactorMovimientoCamaraMinijuegos();
    float alfa = 1.0f - Lim01T((edad - (duracion - 0.35f)) / 0.35f);
    TextoCentradoT(texto, w * 0.5f, h * 0.36f, (int)(46.0f * escala), color, alfa);

    if (bajada)
    {
        TextoCentradoT(bajada, w * 0.5f, h * 0.36f + 40.0f * EscalaUI(), UI_TEXTO, Color{ 255, 246, 226, 255 }, alfa);
    }
}


static void DibujarAnunciosT(
    const MinijuegoTuberiasDesierto& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    float mov = ObtenerFactorMovimientoCamaraMinijuegos();

    // Numeros de las entradas sobre sus placas.
    if (m.partidaValida && m.fase != FASE_TUBERIAS_TERMINADO)
    {
        for (int e = 0; e < m.cantidadEntradas; e++)
        {
            Vector2 q = GetWorldToScreen({ XCarril(m.cantidadEntradas, e), Y_COLECTOR_T, Z_COLECTOR_T + 0.66f }, vista);
            bool ganadora = m.fase == FASE_TUBERIAS_REVELAR && e == m.entradaGanadora;
            Color c = ganadora ? COLOR_ORO_T : Color{ 255, 244, 222, 255 };
            TextoCentradoT(TextFormat("%d", e + 1), q.x, q.y, ganadora ? UI_TEXTO_GRANDE : UI_TEXTO, c);
        }
    }

    // Cartelito J# sobre cada jugador y puntos de la ronda al revelar.
    for (int i = 0; i < limite; i++)
    {
        if (!m.estadosJugadores[i].participa) continue;

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.6f;
        Vector2 q = GetWorldToScreen(cabeza, vista);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        DibujarEtiquetaUI(TextFormat("J%d", NumeroJugadorT(participantes, i)), q, 0.0f, opciones);

        if (m.fase == FASE_TUBERIAS_REVELAR)
        {
            int delta = m.estadosJugadores[i].deltaRonda;
            float edad = m.tiempoFase;
            int puesto = 0;
            PosicionSelectorT(m, i, limite, &puesto);
            float escalon = 0.75f * (float)puesto;
            Vector2 p = GetWorldToScreen({ cabeza.x, cabeza.y + 0.9f + escalon + 0.5f * Lim01T(edad / 0.6f) * mov, cabeza.z }, vista);
            const char* texto = delta > 0 ? TextFormat("+%d", delta) : TextFormat("%d", delta);
            Color c = delta > 0 ? COLOR_ORO_T : (delta < 0 ? Color{ 255, 120, 100, 255 } : Color{ 236, 226, 214, 255 });
            float alfa = Lim01T(edad / 0.2f) * (1.0f - Lim01T((edad - 2.5f) / 0.5f));
            TextoCentradoT(texto, p.x, p.y, delta != 0 ? UI_TEXTO_GRANDE : UI_TEXTO, c, alfa);
        }
    }

    if (!m.partidaValida) return;

    // Anuncios de fase.
    if (m.fase == FASE_TUBERIAS_OBSERVAR)
    {
        AnuncioFaseT(TextFormat("¡RONDA %d!", m.ronda + 1), "SEGUÍ LAS TUBERÍAS CON LA VISTA",
            m.tiempoFase - (m.ronda == 0 ? 1.1f : 0.1f), 1.3f, COLOR_ORO_CLARO_T);   // en la 1.ª, tras el «¡YA!» del HUD
    }
    else if (m.fase == FASE_TUBERIAS_ELEGIR)
    {
        AnuncioFaseT("¡ELEGÍ UNA ENTRADA!", nullptr, m.tiempoFase, 1.2f, Color{ 255, 236, 200, 255 });
    }
    else if (m.fase == FASE_TUBERIAS_COMPUERTA)
    {
        AnuncioFaseT("¡ABREN LAS COMPUERTAS!", nullptr, m.tiempoFase, 1.0f, COLOR_AGUA_CLARA_T);
    }
    else if (m.fase == FASE_TUBERIAS_REVELAR)
    {
        float edad = m.tiempoFase;
        Vector2 oro = GetWorldToScreen({ XCarril(m.cantidadEntradas, m.oroCantaro), Y_REPISA + 2.3f + 0.4f * Lim01T(edad / 0.5f) * mov, Z_CANTARO_T }, vista);
        Vector2 mir = GetWorldToScreen({ XCarril(m.cantidadEntradas, m.mirajeCantaro), Y_REPISA + 0.7f + 0.3f * Lim01T(edad / 0.5f) * mov, Z_CANTARO_T + 0.6f }, vista);
        float alfa = Lim01T(edad / 0.25f);
        TextoCentradoT("¡ORO!", oro.x, oro.y, UI_TEXTO_GRANDE, COLOR_ORO_T, alfa);
        TextoCentradoT("¡ESPEJISMO!", mir.x, mir.y, UI_TEXTO, Color{ 255, 214, 170, 255 }, alfa);
    }

    // Linea fija de la fase, al pie (sobre la franja de ayuda).
    const char* linea = nullptr;
    Color colorLinea = Color{ 255, 240, 214, 255 };

    if (m.fase == FASE_TUBERIAS_OBSERVAR)
    {
        linea = TextFormat("OBSERVÁ EL RECORRIDO: %d", (int)std::ceil(DURACION_OBSERVAR - m.tiempoFase));
    }
    else if (m.fase == FASE_TUBERIAS_ELEGIR)
    {
        int s = (int)std::ceil(DURACION_ELEGIR - m.tiempoFase);
        linea = TextFormat("ELEGÍ LA ENTRADA DEL CÁNTARO DORADO: %d", s < 0 ? 0 : s);
        if (s <= 3) colorLinea = Color{ 255, 170, 120, 255 };
    }
    else if (m.fase == FASE_TUBERIAS_COMPUERTA || m.fase == FASE_TUBERIAS_FLUJO)
    {
        linea = "EL AGUA CORRE POR LAS TUBERÍAS...";
        colorLinea = COLOR_AGUA_CLARA_T;
    }
    else if (m.fase == FASE_TUBERIAS_REVELAR)
    {
        linea = TextFormat("LA ENTRADA %d LLEVABA AL ORO   ·   ORO +3   ESPEJISMO -1", m.entradaGanadora + 1);
        colorLinea = COLOR_ORO_CLARO_T;
    }

    if (linea)
    {
        TextoCentradoT(linea, w * 0.5f, h - 40.0f * EscalaUI() - (float)MargenBarraRondaMinijuegos(), UI_TEXTO, colorLinea);
    }
}


static void PrepararHudT(const MinijuegoTuberiasDesierto& m, const Participante participantes[], int limite, DatosHudMinijuego& hud)
{
    PrepararDatosHudMinijuego(
        hud,
        "TUBERÍAS DEL DESIERTO",
        "SEGUÍ LAS TUBERÍAS Y ELEGÍ LA ENTRADA QUE LLEVA AL CÁNTARO DORADO (+3). ¡OJO CON EL ESPEJISMO (-1)!",
        m.resultado
    );

    hud.fase = m.fase == FASE_TUBERIAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_TUBERIAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "RONDA";
    int ronda = m.ronda + 1 > RONDAS_TUBERIAS ? RONDAS_TUBERIAS : m.ronda + 1;
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", ronda, RONDAS_TUBERIAS);
    hud.progresoMedidor = (float)ronda / (float)RONDAS_TUBERIAS;
    hud.finPorTiempo = false;

    bool desempate = false;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const EstadoJugadorTuberias& estado = m.estadosJugadores[i];
        const Participante& participante = participantes[i];
        JugadorHudMinijuego& jugador = hud.jugadores[i];

        jugador.visible = true;
        jugador.valorNumerico = estado.puntos;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d PTS", estado.puntos);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), estado.puntos == 1 ? "1 PUNTO" : "%d PUNTOS", estado.puntos);

        if (estado.aciertoUltima)
        {
            std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "ORO AL FINAL");
        }

        for (int j = 0; j < limite; j++)
        {
            if (j != i && m.resultado.participantes[j].participo && m.estadosJugadores[j].puntos == estado.puntos &&
                m.estadosJugadores[j].aciertoUltima != estado.aciertoUltima)
            {
                desempate = true;
            }
        }

        int entrada = estado.seleccion + 1;

        if (m.fase == FASE_TUBERIAS_OBSERVAR)
            std::snprintf(jugador.estado, sizeof(jugador.estado), "OBSERVANDO");
        else if (m.fase == FASE_TUBERIAS_ELEGIR)
            std::snprintf(jugador.estado, sizeof(jugador.estado), estado.bloqueado ? "¡ENTRADA %d!" : "¿ENTRADA %d?", entrada);
        else if (m.fase == FASE_TUBERIAS_COMPUERTA || m.fase == FASE_TUBERIAS_FLUJO)
            std::snprintf(jugador.estado, sizeof(jugador.estado), "ENTRADA %d", entrada);
        else if (m.fase == FASE_TUBERIAS_REVELAR)
            std::snprintf(jugador.estado, sizeof(jugador.estado), estado.deltaRonda > 0 ? "+3 ¡ORO!" : (estado.deltaRonda < 0 ? "-1 ESPEJISMO" : "+0 VACÍO"));

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "ELEGÍ: %s\nCONFIRMÁ: %s",
                TextoMoverHud(participante),
                TextoBotonGolpeHud(participante)
            );
        }
    }

    if (m.fase == FASE_TUBERIAS_TERMINADO && desempate)
    {
        hud.notaFinal = "DESEMPATE: ORO EN LA ÚLTIMA RONDA";
    }
}


//--------------------------------------------------
// Escena completa
//--------------------------------------------------

static void EscenaEstaticaT(bool decoracion, int paso)
{
    SueloT(decoracion);
    TerrazaT(decoracion);
    RepisaT();
    FachadaT(decoracion);
    ArcadaT(decoracion);
    CisternaT(decoracion);

    for (int k = 0; k < PalmasVisiblesT(decoracion); k++)
    {
        TroncoPalmaT(PALMAS_T[k], k);
    }

    // En calidad baja: sin utileria del campamento.
    if (decoracion) CampamentoT(decoracion, paso);

    for (int k = 0; k < 2; k++)
    {
        BraseroT(BRASEROS_T[k]);
    }

    FarolT(PosicionFarolT(-1));
    FarolT(PosicionFarolT(1));
    HorizonteT(decoracion);
}


//--------------------------------------------------
// Vida del lugar: palmeras, agua, caravana, aves, polvo y luces
//--------------------------------------------------

// Caravana de camellos que cruza la cresta de las dunas cercanas.
static void CaravanaT(float t, float mov)
{
    float avance = std::fmod(t * 1.1f * (0.3f + 0.7f * mov), 260.0f);
    Color silueta = Color{ 120, 66, 84, 255 };

    for (int k = 0; k < 6; k++)
    {
        float x = -150.0f + avance - (float)k * 6.5f;
        float z = -84.0f;
        float y = 4.6f + 1.2f * std::sin(x * 0.03f);
        float paso = 0.2f * std::sin(t * 3.0f + (float)k);
        bool persona = k == 0;
        Color c = silueta;

        if (persona)
        {
            CajaT({ x, y + 1.0f, z }, { 0.5f, 2.0f, 0.5f }, c);
            continue;
        }

        CajaT({ x, y + 2.2f, z }, { 3.0f, 1.0f, 0.8f }, c);
        CajaT({ x - 0.2f, y + 2.9f, z }, { 1.2f, 0.6f, 0.8f }, c);
        CajaT({ x + 1.8f, y + 3.1f, z }, { 0.5f, 1.8f, 0.5f }, c);
        CajaT({ x + 2.2f, y + 3.9f, z }, { 0.9f, 0.45f, 0.5f }, c);
        CajaT({ x - 1.0f + paso, y + 0.9f, z }, { 0.3f, 1.8f, 0.3f }, c);
        CajaT({ x + 1.0f - paso, y + 0.9f, z }, { 0.3f, 1.8f, 0.3f }, c);
    }
}


// Bandada de aves que cruza el cielo (siluetas en V que aletean).
static void AvesT(float t, float mov)
{
    for (int k = 0; k < 5; k++)
    {
        float vuelta = std::fmod(t * (2.4f + 0.3f * (float)k) * (0.3f + 0.7f * mov) + (float)k * 9.0f, 140.0f);
        float x = -70.0f + vuelta;
        float y = 30.0f + 3.0f * Ruido01T(k, 95) + 0.6f * std::sin(t * 0.8f + (float)k);
        float z = -60.0f - 6.0f * Ruido01T(k, 96);
        float ala = 0.9f * std::sin(t * 7.0f + (float)k * 1.7f) * (0.3f + 0.7f * mov);
        Color c = Color{ 70, 40, 70, 255 };
        TriT({ x, y, z }, { x - 1.3f, y + ala, z }, { x - 0.4f, y - 0.1f, z }, c);
        TriT({ x, y, z }, { x + 1.3f, y + ala, z }, { x + 0.4f, y - 0.1f, z }, c);
    }
}


// Banderin en el poste de la tienda.
static void BanderinT(float t, float mov)
{
    float ca = std::cos(-18.0f * DEG2RAD), sa = std::sin(-18.0f * DEG2RAD);
    Vector3 base = { 19.0f + 3.8f * ca, 3.7f, -4.6f - 3.8f * sa };
    const int seg = 6;

    for (int s = 0; s < seg; s++)
    {
        float u0 = (float)s / (float)seg, u1 = (float)(s + 1) / (float)seg;
        float o0 = 0.18f * u0 * std::sin(t * 6.0f - u0 * 5.0f) * (0.3f + 0.7f * mov);
        float o1 = 0.18f * u1 * std::sin(t * 6.0f - u1 * 5.0f) * (0.3f + 0.7f * mov);
        float h0 = 0.5f * (1.0f - u0), h1 = 0.5f * (1.0f - u1);
        Color c = (s % 2) ? Color{ 230, 190, 100, 255 } : Color{ 150, 52, 64, 255 };
        QuadT({ base.x + u0 * 1.3f, base.y + h0, base.z + o0 }, { base.x + u1 * 1.3f, base.y + h1, base.z + o1 },
            { base.x + u1 * 1.3f, base.y - h1, base.z + o1 }, { base.x + u0 * 1.3f, base.y - h0, base.z + o0 }, c);
    }
}


static void DibujarVidaT(const MinijuegoTuberiasDesierto& m, bool decoracion, bool efectos)
{
    float t = m.tiempoAnimacion;
    float mov = ObtenerFactorMovimientoCamaraMinijuegos() > 0.5f ? 1.0f : 0.3f;

    LoteT();

    for (int k = 0; k < PalmasVisiblesT(decoracion); k++)
    {
        CopaPalmaT(PALMAS_T[k], k, t, mov);
    }

    if (decoracion)
    {
        CaravanaT(t, mov);
        AvesT(t, mov);
        BanderinT(t, mov);
    }

    FinLoteT();

    // Agua del estanque (alfa sobre su fondo).
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    LoteT();
    AguaCisternaT(t, efectos);
    FinLoteT();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Llamas de los braseros (alfa) y luces aditivas de la escena.
static void LlamaT(Vector3 p, float t, int semilla, float escala)
{
    for (int k = 0; k < 4; k++)
    {
        float vida = std::fmod(t * 1.8f + (float)k * 0.25f + Ruido01T(semilla, 97), 1.0f);
        float x = p.x + 0.18f * std::sin(t * 5.0f + (float)k * 2.0f) * escala;
        float y = p.y + vida * 0.9f * escala;
        float r = (0.42f - 0.32f * vida) * escala;
        Color c = MezclaT(Color{ 255, 236, 150, 255 }, Color{ 255, 90, 30, 255 }, vida);
        HaloT({ x, y, p.z }, r, AlfaT(c, 0.95f), AlfaT(c, 0.0f), 8);
    }
}


static void CharcoLuzT(Vector3 c, float radio, Color color, float intensidad)
{
    for (int r = 0; r < 4; r++)
    {
        float rr = radio * (1.0f - 0.22f * (float)r);
        DiscoT({ c.x, c.y + 0.01f + 0.003f * (float)r, c.z }, rr, rr, 20, AlfaT(color, intensidad), AlfaT(color, 0.0f), false);
    }
}


static void DibujarLucesT(const MinijuegoTuberiasDesierto& m, const JugadorPrueba jugadores[], const Participante participantes[], int limite, bool efectos)
{
    float t = m.tiempoAnimacion;

    // Llamas: en toda calidad (son parte del brasero).
    LoteT();

    for (int k = 0; k < 2; k++)
    {
        LlamaT({ BRASEROS_T[k].x, 2.15f, BRASEROS_T[k].z }, t, k, 1.0f);
    }

    FinLoteT();

    if (!efectos) return;

    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteT();

    for (int k = 0; k < 2; k++)
    {
        Vector3 b = BRASEROS_T[k];
        float parpadeo = 0.85f + 0.15f * std::sin(t * 11.0f + (float)k * 3.0f) * std::sin(t * 7.3f + (float)k);
        HaloT({ b.x, 2.6f, b.z }, 1.3f * parpadeo, Color{ 255, 170, 80, 120 }, Color{ 255, 120, 40, 0 }, 14);
        HaloT({ b.x, 2.6f, b.z }, 2.8f * parpadeo, Color{ 255, 140, 60, 50 }, Color{ 255, 100, 40, 0 }, 14);
        CharcoLuzT({ b.x, 0.0f, b.z - 0.6f }, 4.2f * parpadeo, Color{ 255, 150, 70, 255 }, 0.05f);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 f = PosicionFarolT(lado);
        float parpadeo = 0.9f + 0.1f * std::sin(t * 9.0f + (float)lado);
        HaloT(f, 0.7f * parpadeo, Color{ 255, 210, 130, 140 }, Color{ 255, 180, 90, 0 }, 12);
        HaloT(f, 1.8f * parpadeo, Color{ 255, 170, 90, 50 }, Color{ 255, 150, 70, 0 }, 12);
        CharcoLuzT({ f.x, 0.0f, 0.4f }, 3.0f, Color{ 255, 170, 90, 255 }, 0.035f);
    }

    // Charco de luz dorada del cantaro de oro.
    if (TuberiasVisiblesT(m))
    {
        float x = XCarril(m.cantidadEntradas, m.oroCantaro);
        float pulso = 0.8f + 0.2f * std::sin(t * 4.0f);
        float fuerza = m.fase == FASE_TUBERIAS_REVELAR ? 1.0f : 0.55f;
        HaloT({ x, Y_REPISA + 0.7f, Z_CANTARO_T + 0.6f }, 1.3f * pulso, AlfaT(Color{ 255, 210, 90, 255 }, 0.35f * fuerza), Color{ 255, 190, 60, 0 }, 16);
        CharcoLuzT({ x, Y_REPISA, Z_CANTARO_T + 0.4f }, 1.4f, Color{ 255, 200, 80, 255 }, 0.06f * fuerza);

        // Columna de luz al revelar y recorrido ganador encendido.
        if (m.fase == FASE_TUBERIAS_REVELAR)
        {
            float a = Lim01T(m.tiempoFase / 0.4f) * (1.0f - Lim01T((m.tiempoFase - 2.4f) / 0.6f));
            Color col = AlfaT(Color{ 255, 220, 120, 255 }, 0.22f * a);
            QuadColoresT({ x - 0.7f, Y_REPISA, Z_CANTARO_T + 0.7f }, { x + 0.7f, Y_REPISA, Z_CANTARO_T + 0.7f },
                { x + 1.1f, Y_REPISA + 9.0f, Z_CANTARO_T + 0.7f }, { x - 1.1f, Y_REPISA + 9.0f, Z_CANTARO_T + 0.7f }, col, col, AlfaT(col, 0.0f), AlfaT(col, 0.0f));

            int e = m.entradaGanadora;
            CintaTuberiaT(m, e, U_MIN_T, (float)m.etapas + U_PICO_T, R_TUBO_T * 1.2f, R_TUBO_T * 2.6f, AlfaT(Color{ 255, 200, 80, 255 }, 0.28f * a), 0.0f, t);

            // Destello que recorre el camino correcto.
            float u = U_MIN_T + std::fmod(m.tiempoFase * 0.6f, 1.0f) * ((float)m.etapas + U_PICO_T - U_MIN_T);
            Vector3 p = PuntoTuberia(m, e, u);
            HaloT({ p.x, p.y, p.z + R_TUBO_T * 1.4f }, 0.9f, AlfaT(Color{ 255, 240, 170, 255 }, 0.8f * a), Color{ 255, 200, 80, 0 }, 12);
        }
    }

    // Frente del agua mientras corre.
    if (m.progresoAgua > 0.0f && m.progresoAgua < 1.0f)
    {
        float uPico = (float)m.etapas + U_PICO_T;
        float u = UAguaT(m) < uPico ? UAguaT(m) : uPico;

        for (int e = 0; e < m.cantidadEntradas; e++)
        {
            Vector3 p = PuntoTuberia(m, e, u);
            HaloT({ p.x, p.y, p.z + R_TUBO_T * 1.3f }, 0.5f, Color{ 200, 250, 255, 170 }, Color{ 120, 220, 255, 0 }, 10);
        }
    }

    // Selectores fijados: anillo del color del jugador.
    if (SelectoresVisiblesT(m))
    {
        for (int i = 0; i < limite; i++)
        {
            if (!m.estadosJugadores[i].participa || !m.estadosJugadores[i].bloqueado) continue;

            Vector3 c = PosicionSelectorT(m, i, limite);
            HaloT({ c.x, c.y - 0.2f, c.z - 0.2f }, 0.55f, AlfaT(participantes[i].color, 0.45f), AlfaT(participantes[i].color, 0.0f), 12);
        }
    }

    // Brillo bajo cada jugador (anillo de su color).
    for (int i = 0; i < limite; i++)
    {
        if (!m.estadosJugadores[i].participa) continue;

        Vector3 p = jugadores[i].posicion;
        AnilloT({ p.x, 0.02f, p.z }, 0.42f, 0.62f, 18, AlfaT(participantes[i].color, 0.5f), AlfaT(participantes[i].color, 0.0f));
    }

    // Resplandor calido del sol sobre la terraza (luz rasante).
    QuadColoresT({ 0.0f, 0.015f, -0.6f }, { 12.4f, 0.015f, -0.6f }, { 12.4f, 0.015f, 6.4f }, { 0.0f, 0.015f, 6.4f },
        Color{ 0, 0, 0, 255 }, Color{ 40, 20, 6, 255 }, Color{ 40, 20, 6, 255 }, Color{ 0, 0, 0, 255 });

    FinLoteT();
    rlDrawRenderBatchActive();
    EndBlendMode();
}


// Polvo dorado que flota con la brisa y festejos (analiticos: no guardan
// estado, salen del reloj de la fase).
static void DibujarParticulasT(const MinijuegoTuberiasDesierto& m, const JugadorPrueba jugadores[], const Participante participantes[], int limite)
{
    float t = m.tiempoAnimacion;
    float mov = ObtenerFactorMovimientoCamaraMinijuegos() > 0.5f ? 1.0f : 0.3f;
    int cantidad = (int)(46.0f * FactorCalidadGrafica(CalidadParticulas()));

    LoteT();

    for (int k = 0; k < cantidad; k++)
    {
        float vel = 0.6f + 0.8f * Ruido01T(k, 101);
        float x = std::fmod(Ruido01T(k, 102) * 60.0f + t * vel * mov, 60.0f) - 30.0f;
        float y = 0.4f + 9.0f * Ruido01T(k, 103) + 0.4f * std::sin(t * 0.9f + (float)k);
        float z = -4.0f + 12.0f * Ruido01T(k, 104);
        float a = 0.35f + 0.35f * std::sin(t * 2.0f + (float)k * 1.3f);
        PuntoT({ x, y, z }, 0.035f + 0.03f * Ruido01T(k, 105), AlfaT(Color{ 255, 220, 160, 255 }, a));
    }

    // Chispas doradas sobre quien acerto, al revelar.
    if (m.fase == FASE_TUBERIAS_REVELAR && m.tiempoFase < 2.0f)
    {
        float edad = m.tiempoFase;

        for (int i = 0; i < limite; i++)
        {
            if (!m.estadosJugadores[i].participa || m.estadosJugadores[i].deltaRonda <= 0) continue;

            Vector3 p = jugadores[i].posicion;

            for (int k = 0; k < 18; k++)
            {
                float ang = 2.0f * PI * Ruido01T(k + i * 31, 106);
                float v = 1.6f + 1.6f * Ruido01T(k + i * 31, 107);
                float sube = 4.0f + 2.0f * Ruido01T(k + i * 31, 108);
                Vector3 q = { p.x + std::cos(ang) * v * edad, p.y + 0.8f + sube * edad - 4.9f * edad * edad, p.z + std::sin(ang) * v * edad * 0.6f };
                Color c = (k % 3 == 0) ? participantes[i].color : COLOR_ORO_CLARO_T;
                PuntoT(q, 0.08f, AlfaT(c, 1.0f - edad / 2.0f));
            }
        }
    }

    // Salpicaduras en la boca de cada cantaro cuando llega el agua.
    if (m.progresoAgua >= 1.0f && (m.fase == FASE_TUBERIAS_REVELAR) && m.tiempoFase < 1.0f)
    {
        float edad = m.tiempoFase;

        for (int lugar = 0; lugar < m.cantidadEntradas; lugar++)
        {
            if (lugar == m.mirajeCantaro) continue;

            float x = XCarril(m.cantidadEntradas, lugar);

            for (int k = 0; k < 8; k++)
            {
                float ang = 2.0f * PI * (float)k / 8.0f;
                Vector3 q = { x + std::cos(ang) * 0.8f * edad, Y_REPISA + 1.25f + 2.2f * edad - 4.9f * edad * edad, Z_CANTARO_T + std::sin(ang) * 0.5f * edad };
                PuntoT(q, 0.06f, AlfaT(lugar == m.oroCantaro ? COLOR_ORO_CLARO_T : COLOR_AGUA_CLARA_T, 1.0f - edad));
            }
        }
    }

    // Confeti sobre el ganador al final.
    if (m.fase == FASE_TUBERIAS_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1) continue;

            Vector3 p = jugadores[i].posicion;

            for (int k = 0; k < 30; k++)
            {
                float caida = std::fmod(t * (0.8f + 0.5f * Ruido01T(k, 109)) + Ruido01T(k, 110) * 4.0f, 4.0f);
                float x = p.x + (Ruido01T(k, 111) - 0.5f) * 3.0f + 0.3f * std::sin(t * 3.0f + (float)k) * mov;
                float z = p.z + (Ruido01T(k, 112) - 0.5f) * 2.0f;
                Color c = (k % 4 == 0) ? participantes[i].color : ((k % 4 == 1) ? COLOR_ORO_T : ((k % 4 == 2) ? Color{ 255, 250, 240, 255 } : COLOR_AGUA_T));
                PuntoT({ x, p.y + 4.4f - caida, z }, 0.07f, c);
            }
        }
    }

    FinLoteT();
}


void MinijuegoTuberiasDesierto::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(COLOR_FONDO_T);

    Camera3D final = CamaraFinalT();
    Camera3D vista = CamaraVistaT(*this);
    BaseCamaraT baseFinal = BaseDeT(final);
    BaseCamaraT baseVista = BaseDeT(vista);
    g_ojoT = final.position;
    g_derT = baseVista.r;
    g_arribaT = baseVista.u;

    HornearEscenaT();

    if (TuberiasVisiblesT(*this) && !TuberiasCreciendoT(*this))
    {
        HornearTubosT(*this);
    }

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarMallaT(g_hornoT.escena, g_hornoT.escenaValida);
    DibujarVidaT(*this, decoracion, efectos);

    if (partidaValida)
    {
        if (TuberiasVisiblesT(*this) && !TuberiasCreciendoT(*this))
        {
            DibujarMallaT(g_hornoT.tubos, g_hornoT.tubosValida);
        }
        else
        {
            LoteT();
            ColectorT(*this);

            if (TuberiasVisiblesT(*this))
            {
                for (int e = 0; e < cantidadEntradas; e++)
                {
                    float c = CrecimientoTuberiaT(*this, e);

                    if (c <= 0.0f) continue;

                    float uHasta = U_MIN_T + c * ((float)etapas + U_PICO_T - U_MIN_T);
                    TuboPiedraT(*this, e, uHasta, R_TUBO_T * (0.5f + 0.5f * c), decoracion);

                    if (c >= 1.0f) PicoT(*this, e);
                }

                TamboresT(*this);
            }

            FinLoteT();
        }

        LoteT();
        DibujarCompuertasT(*this);
        FinLoteT();
        DibujarCantarosT(*this, decoracion);
    }

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    for (int i = 0; i < limite; i++)
    {
        if (!estadosJugadores[i].participa) continue;

        JugadorPrueba visual = jugadores[i];
        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;

        // Salto de festejo o sacudida al revelar.
        if (fase == FASE_TUBERIAS_REVELAR)
        {
            float mov = ObtenerFactorMovimientoCamaraMinijuegos();
            int delta = estadosJugadores[i].deltaRonda;

            if (delta > 0)
            {
                float u = tiempoFase * 5.0f;
                visual.posicion.y += u < 2.0f * PI ? std::fabs(std::sin(u)) * 0.55f * mov : 0.0f;
            }
            else if (delta < 0 && tiempoFase < 0.6f)
            {
                visual.posicion.x += std::sin(tiempoFase * 50.0f) * 0.08f * mov;
            }
        }

        DibujarJugadorModeloCompartido(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    if (partidaValida)
    {
        DibujarSelectoresT(*this, participantes, limite);
    }

    // Cielo despues de lo opaco: solo pinta lo que queda libre.
    DibujarSolT(baseFinal);
    DibujarCieloT(vista);
    rlDrawRenderBatchActive();

    rlDisableDepthMask();
    DibujarNubesT(baseFinal, tiempoAnimacion, decoracion);
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    g_sinBrumaT = true;
    DibujarBrilloCieloT(baseFinal, tiempoAnimacion, decoracion);
    g_sinBrumaT = false;
    rlDrawRenderBatchActive();
    EndBlendMode();

    if (partidaValida)
    {
        g_sinBrumaT = true;
        DibujarAguaT(*this);
        DibujarEspejismoT(*this, decoracion);
        g_sinBrumaT = false;
        rlDrawRenderBatchActive();
    }

    g_sinBrumaT = true;
    DibujarLucesT(*this, jugadores, participantes, limite, efectos);
    DibujarParticulasT(*this, jugadores, participantes, limite);
    g_sinBrumaT = false;
    rlDrawRenderBatchActive();

    rlEnableDepthMask();
    rlEnableBackfaceCulling();

    EndMode3D();

    DibujarAnunciosT(*this, jugadores, participantes, limite, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        TextoCentradoT("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    DatosHudMinijuego hud;
    PrepararHudT(*this, participantes, limite, hud);
    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoTuberiasDesierto::ObtenerResultado() const
{
    return resultado;
}




void MinijuegoTuberiasDesierto::Descargar()
{
    LiberarMallasTuberias();
    LiberarMaterialHorneado(g_hornoT.material, g_hornoT.materialListo);
}
