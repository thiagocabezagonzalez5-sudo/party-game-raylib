#include "Minigames/MinijuegoSenderoInvisible.h"

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
#include <cstring>
#include <vector>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_SENDERO = 3.0f;
static const float DURACION_MEMORIZAR_SENDERO = 3.0f;
static const float DURACION_CARRERA_SENDERO = 60.0f;

static const float PASO_LOSA = 1.5f;          // distancia entre centros
static const float SEPARACION_CARRILES = 10.0f;
static const float Z_FILA_CERO = 6.0f;
static const float ALTURA_CENTRO_JUGADOR = 0.7f;

static const float TIEMPO_TEMBLOR = 0.3f;
static const float DURACION_REAPARICION = 0.5f;
static const float DURACION_FAROL = 3.0f;
static const float ALCANCE_FAROL = 0.9f;


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


static int IndiceLosa(int fila, int columna)
{
    return fila * COLUMNAS_SENDERO + columna;
}


static float XCarril(const MinijuegoSenderoInvisible& m, int carril)
{
    return ((float)carril - 0.5f * (float)(m.cantidadCarriles - 1)) * SEPARACION_CARRILES;
}


static float XColumna(const MinijuegoSenderoInvisible& m, int carril, int columna)
{
    return XCarril(m, carril) + ((float)columna - 2.5f) * PASO_LOSA;
}


static float ZFila(int fila)
{
    return Z_FILA_CERO - (float)fila * PASO_LOSA;
}


// Losa bajo una posicion del mundo. Devuelve false si esta fuera.
static bool LosaBajoPosicion(
    const MinijuegoSenderoInvisible& m,
    int carril,
    Vector3 posicion,
    int& fila,
    int& columna
)
{
    columna = (int)std::floor((posicion.x - XCarril(m, carril)) / PASO_LOSA + 3.0f);
    fila = (int)std::floor((Z_FILA_CERO - posicion.z) / PASO_LOSA + 0.5f);

    return
        columna >= 0 && columna < COLUMNAS_SENDERO &&
        fila >= 0 && fila < FILAS_SENDERO;
}


static Color ColorJugador(const Participante participantes[], int i)
{
    return participantes[i].color;
}


//==================================================
// DATOS LOGICOS: TRAZADO SEGURO
//==================================================

static void AgregarRuta(MinijuegoSenderoInvisible& m, int fila, int columna)
{
    if (m.cantidadRuta >= MAX_RUTA_SENDERO)
    {
        return;
    }

    int indice = IndiceLosa(fila, columna);

    m.segura[indice] = true;
    m.ruta[m.cantidadRuta++] = indice;
}


static void GenerarTrazado(MinijuegoSenderoInvisible& m)
{
    for (int k = 0; k < LOSAS_SENDERO; k++)
    {
        m.segura[k] = false;
        m.grieta[k] = false;
        m.indiceProgreso[k] = 0;
        m.indiceSecuencia[k] = -1;
    }

    m.cantidadRuta = 0;
    m.columnaInicio = GetRandomValue(1, 4);

    // La fila 0 (salida) y la ultima (meta) son firmes.
    for (int c = 0; c < COLUMNAS_SENDERO; c++)
    {
        m.segura[IndiceLosa(0, c)] = true;
        m.segura[IndiceLosa(FILAS_SENDERO - 1, c)] = true;
    }

    bool visitada[LOSAS_SENDERO]{};
    int fila = 1;
    int columna = m.columnaInicio;
    int lateralesSeguidos = 0;

    AgregarRuta(m, fila, columna);
    visitada[IndiceLosa(fila, columna)] = true;

    while (fila <= FILAS_SENDERO - 2 && m.cantidadRuta < MAX_RUTA_SENDERO - 1)
    {
        if (lateralesSeguidos < 2 && GetRandomValue(0, 99) < 35)
        {
            int nueva = columna + (GetRandomValue(0, 1) == 0 ? -1 : 1);

            if (
                nueva >= 0 && nueva < COLUMNAS_SENDERO &&
                !visitada[IndiceLosa(fila, nueva)]
            )
            {
                columna = nueva;
                AgregarRuta(m, fila, columna);
                visitada[IndiceLosa(fila, columna)] = true;
                lateralesSeguidos++;
                continue;
            }
        }

        if (fila == FILAS_SENDERO - 2)
        {
            break;
        }

        fila++;
        AgregarRuta(m, fila, columna);
        visitada[IndiceLosa(fila, columna)] = true;
        lateralesSeguidos = 0;
    }

    m.columnaMeta = columna;

    // Progreso: salida 0, tramos 1..N, meta N + 1.
    for (int c = 0; c < COLUMNAS_SENDERO; c++)
    {
        m.indiceProgreso[IndiceLosa(0, c)] = 0;
        m.indiceProgreso[IndiceLosa(FILAS_SENDERO - 1, c)] = m.cantidadRuta + 1;
    }

    m.cantidadSecuencia = 0;
    m.secuencia[m.cantidadSecuencia++] = IndiceLosa(0, m.columnaInicio);
    m.indiceSecuencia[IndiceLosa(0, m.columnaInicio)] = 0;

    for (int k = 0; k < m.cantidadRuta; k++)
    {
        m.indiceProgreso[m.ruta[k]] = k + 1;
        m.indiceSecuencia[m.ruta[k]] = m.cantidadSecuencia;
        m.secuencia[m.cantidadSecuencia++] = m.ruta[k];
    }

    int meta = IndiceLosa(FILAS_SENDERO - 1, m.columnaMeta);
    m.indiceSecuencia[meta] = m.cantidadSecuencia;
    m.secuencia[m.cantidadSecuencia++] = meta;

    m.farolIndice = m.cantidadRuta / 2;

    if (m.farolIndice < 1)
    {
        m.farolIndice = 1;
    }
}


//==================================================
// IA DE BOTS
//==================================================

// Memoria imperfecta: cada losa segura se recuerda con probabilidad
// "precision"; algunas losas falsas vecinas de la ruta se creen seguras.
static void MemorizarBot(const MinijuegoSenderoInvisible& m, EstadoJugadorSendero& estado)
{
    for (int fila = 0; fila < FILAS_SENDERO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_SENDERO; columna++)
        {
            int indice = IndiceLosa(fila, columna);
            bool cree = false;

            if (fila == 0 || fila == FILAS_SENDERO - 1)
            {
                cree = true;
            }
            else if (m.segura[indice])
            {
                cree = Aleatorio01() < estado.precision;
            }
            else
            {
                bool vecinaSegura =
                    (fila > 0 && m.segura[IndiceLosa(fila - 1, columna)]) ||
                    (columna > 0 && m.segura[IndiceLosa(fila, columna - 1)]) ||
                    (columna < COLUMNAS_SENDERO - 1 && m.segura[IndiceLosa(fila, columna + 1)]) ||
                    (fila < FILAS_SENDERO - 1 && m.segura[IndiceLosa(fila + 1, columna)]);

                cree = vecinaSegura && Aleatorio01() < (1.0f - estado.precision) * 0.5f;
            }

            estado.conocimiento[indice] = cree;
        }
    }
}


static void ElegirObjetivoBot(
    const MinijuegoSenderoInvisible& m,
    EstadoJugadorSendero& estado,
    int fila,
    int columna
)
{
    static const int DESPLAZAMIENTOS[4][2] =
    {
        { 1, 0 }, { 0, -1 }, { 0, 1 }, { -1, 0 }
    };

    float mejor = -1.0f;
    int mejorFila = -1;
    int mejorColumna = -1;

    for (int k = 0; k < 4; k++)
    {
        int nf = fila + DESPLAZAMIENTOS[k][0];
        int nc = columna + DESPLAZAMIENTOS[k][1];

        if (nf < 0 || nf >= FILAS_SENDERO || nc < 0 || nc >= COLUMNAS_SENDERO)
        {
            continue;
        }

        int indice = IndiceLosa(nf, nc);

        if (m.grieta[indice])
        {
            continue;
        }

        bool esPrevia = nf == estado.previaFila && nc == estado.previaColumna;
        float puntaje = Aleatorio01() * 1.5f;

        if (estado.conocimiento[indice]) puntaje += (fila == 0 && nf == 0) ? 2.0f : 10.0f;
        if (k == 0) puntaje += 3.0f;
        if (k == 3) puntaje -= 4.0f;
        if (esPrevia) puntaje -= 6.0f;

        if (puntaje > mejor)
        {
            mejor = puntaje;
            mejorFila = nf;
            mejorColumna = nc;
        }
    }

    estado.previaFila = fila;
    estado.previaColumna = columna;
    estado.objetivoFila = mejorFila;
    estado.objetivoColumna = mejorColumna;
    estado.espera = 0.2f + 0.4f * Aleatorio01();
}


static InputMinijuegoParticipante CrearEntradaBotSendero(
    const MinijuegoSenderoInvisible& m,
    EstadoJugadorSendero& estado,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};

    if (jugador.cayendo)
    {
        estado.objetivoFila = -1;
        estado.previaFila = -1;
        estado.previaColumna = -1;
        return entrada;
    }

    if (!jugador.enSuelo)
    {
        return entrada;
    }

    int fila = 0;
    int columna = 0;

    if (!LosaBajoPosicion(m, estado.carril, jugador.posicion, fila, columna))
    {
        return entrada;
    }

    if (estado.objetivoFila < 0)
    {
        ElegirObjetivoBot(m, estado, fila, columna);
    }

    float objetivoX = XColumna(m, estado.carril, estado.objetivoColumna);
    float objetivoZ = ZFila(estado.objetivoFila);
    float dx = objetivoX - jugador.posicion.x;
    float dz = objetivoZ - jugador.posicion.z;

    if (std::fabs(dx) < 0.2f && std::fabs(dz) < 0.2f)
    {
        // Llego: duda un momento y decide el siguiente paso.
        estado.espera -= deltaTime;

        if (estado.espera <= 0.0f)
        {
            ElegirObjetivoBot(m, estado, estado.objetivoFila, estado.objetivoColumna);
        }

        return entrada;
    }

    if (estado.espera > 0.0f)
    {
        estado.espera -= deltaTime;
        return entrada;
    }

    entrada.izquierda = dx < -0.12f;
    entrada.derecha = dx > 0.12f;
    entrada.adelante = dz < -0.12f;
    entrada.atras = dz > 0.12f;

    return entrada;
}


//==================================================
// CAMARA
//==================================================

static void ConfigurarCamara(MinijuegoSenderoInvisible& m)
{
    float mitadAncho = 0.5f * (float)(m.cantidadCarriles < 2 ? 2 : m.cantidadCarriles) * SEPARACION_CARRILES + 2.0f;
    float distancia = mitadAncho / 0.6f;

    if (distancia < 15.0f)
    {
        distancia = 15.0f;
    }

    m.camara.position = { 0.0f, 0.7f * distancia, -0.8f + 0.75f * distancia };
    m.camara.target = { 0.0f, 0.0f, -0.8f };
    m.camara.up = { 0.0f, 1.0f, 0.0f };
    m.camara.fovy = 50.0f;
    m.camara.projection = CAMERA_PERSPECTIVE;
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

void MinijuegoSenderoInvisible::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        jugadorDeCarril[i] = -1;

        for (int k = 0; k < LOSAS_SENDERO; k++)
        {
            losas[i][k] = {};
            bloques[i][k] = {};
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_SENDERO; i++)
    {
        particulas[i] = {};
    }

    cantidadCarriles = 0;
    empate = false;
    partidaValida = false;

    GenerarTrazado(*this);

    fase = FASE_SENDERO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_SENDERO;
    tiempoMemorizar = DURACION_MEMORIZAR_SENDERO;
    tiempoRestante = DURACION_CARRERA_SENDERO;
    tiempoAnimacion = 0.0f;
    tiempoYa = 0.0f;

    ConfigurarCamara(*this);
}


static void LiberarMallasSendero();


void MinijuegoSenderoInvisible::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    LiberarMallasSendero();
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
            cantidad++;
        }
    }

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_SENDERO_TERMINADO;
        return;
    }

    partidaValida = true;
    cantidadCarriles = cantidad;
    ConfigurarCamara(*this);

    int carril = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        EstadoJugadorSendero& estado = estadosJugadores[i];
        estado.carril = carril;
        jugadorDeCarril[carril] = i;
        estado.precision = 0.7f + 0.2f * Aleatorio01();
        MemorizarBot(*this, estado);

        for (int fila = 0; fila < FILAS_SENDERO; fila++)
        {
            for (int columna = 0; columna < COLUMNAS_SENDERO; columna++)
            {
                BloquePrueba& bloque = bloques[carril][IndiceLosa(fila, columna)];
                bloque.posicion = { XColumna(*this, carril, columna), -0.2f, ZFila(fila) };
                bloque.posicionInicial = bloque.posicion;
                bloque.tamano = { PASO_LOSA - 0.14f, 0.4f, PASO_LOSA - 0.14f };
                bloque.color = GRAY;
                bloque.activaColision = true;
            }
        }

        Vector3 spawn = { XCarril(*this, carril), ALTURA_CENTRO_JUGADOR + 0.1f, ZFila(0) };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].duracionRespawn = DURACION_REAPARICION;
        jugadores[i].posicion = spawn;
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugadores[i].enSuelo = false;
        jugadores[i].cayendo = false;

        estado.ultimaFila = 0;
        estado.ultimaColumna = columnaInicio;
        carril++;
    }
}


//==================================================
// FIN DE LA PARTIDA
//==================================================

static float PuntajeFinal(const EstadoJugadorSendero& estado)
{
    return estado.llego
        ? 1000.0f + estado.tiempoFin
        : (float)estado.progreso;
}


static void FinalizarSendero(MinijuegoSenderoInvisible& m)
{
    int ganadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorSendero& estado = m.estadosJugadores[i];
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
        resultadoJugador.puntuacionMinijuego = estado.progreso;
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
    m.fase = FASE_SENDERO_TERMINADO;
}


//==================================================
// ACTUALIZACION
//==================================================

static void ActualizarLosasCaidas(MinijuegoSenderoInvisible& m, float deltaTime)
{
    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        for (int k = 0; k < LOSAS_SENDERO; k++)
        {
            LosaSendero& losa = m.losas[carril][k];

            if (losa.estado == LOSA_SENDERO_TEMBLANDO)
            {
                losa.tiempo -= deltaTime;

                if (losa.tiempo <= 0.0f)
                {
                    losa.estado = LOSA_SENDERO_CAIDA;
                    losa.tiempo = 0.0f;
                    m.bloques[carril][k].activaColision = false;
                }
            }
            else if (losa.estado == LOSA_SENDERO_CAIDA && losa.tiempo < 3.0f)
            {
                losa.tiempo += deltaTime;
            }
        }
    }
}


void MinijuegoSenderoInvisible::Actualizar(
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

    if (!partidaValida || fase == FASE_SENDERO_TERMINADO)
    {
        return;
    }

    bool jugando = fase == FASE_SENDERO_JUGANDO;

    if (fase == FASE_SENDERO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_SENDERO_MEMORIZAR;
        }
    }
    else if (fase == FASE_SENDERO_MEMORIZAR)
    {
        tiempoMemorizar -= deltaTime;

        if (tiempoMemorizar <= 0.0f)
        {
            tiempoMemorizar = 0.0f;
            fase = FASE_SENDERO_JUGANDO;
            tiempoYa = 1.5f;
            ReproducirSonidoMinijuego(audio, SONIDO_INICIO_MINIJUEGO);
        }
    }
    else
    {
        float restanteAntes = tiempoRestante;
        tiempoRestante -= deltaTime;
        ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

        if (tiempoYa > 0.0f) tiempoYa -= deltaTime;
    }

    bool algunoLlego = false;

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorSendero& estado = estadosJugadores[i];

        if (!participantes[i].activo || estado.carril < 0)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        InputMinijuegoParticipante entrada{};

        // Fuera de la carrera todos esperan quietos (la fisica sigue).
        if (jugando && !estado.llego)
        {
            if (JugadorEsBot(participantes[i]))
            {
                entrada = CrearEntradaBotSendero(*this, estado, jugador, deltaTime);
            }
            else
            {
                entrada = LeerInputMinijuegoParticipante(participantes[i]);
            }
        }

        entrada.golpear = false;

        if (!jugador.enSuelo)
        {
            entrada.saltar = false;
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            bloques[estado.carril],
            LOSAS_SENDERO,
            particulas,
            MAX_PARTICULAS_SENDERO,
            true,
            true,
            deltaTime
        );

        if (estado.farolTiempo > 0.0f)
        {
            estado.farolTiempo -= deltaTime;
        }

        if (!jugando || jugador.cayendo || !jugador.enSuelo)
        {
            continue;
        }

        int fila = 0;
        int columna = 0;

        if (!LosaBajoPosicion(*this, estado.carril, jugador.posicion, fila, columna))
        {
            continue;
        }

        int indice = IndiceLosa(fila, columna);

        if (segura[indice])
        {
            estado.ultimaFila = fila;
            estado.ultimaColumna = columna;
            jugador.posicionSpawn =
            {
                XColumna(*this, estado.carril, columna),
                ALTURA_CENTRO_JUGADOR + 0.2f,
                ZFila(fila)
            };

            if (indiceProgreso[indice] > estado.progreso)
            {
                estado.progreso = indiceProgreso[indice];
            }

            if (fila == FILAS_SENDERO - 1 && !estado.llego)
            {
                estado.llego = true;
                estado.tiempoFin = tiempoRestante;
                algunoLlego = true;
                ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
            }

            // Farol del tramo medio.
            if (!estado.farolRecogido)
            {
                int indiceFarol = ruta[farolIndice];
                float dx = jugador.posicion.x - XColumna(*this, estado.carril, indiceFarol % COLUMNAS_SENDERO);
                float dz = jugador.posicion.z - ZFila(indiceFarol / COLUMNAS_SENDERO);

                if (dx * dx + dz * dz < ALCANCE_FAROL * ALCANCE_FAROL)
                {
                    estado.farolRecogido = true;
                    estado.farolTiempo = DURACION_FAROL;
                    ReproducirSonidoMinijuego(audio, SONIDO_RECOGER_OBJETO);
                }
            }
        }
        else
        {
            LosaSendero& losa = losas[estado.carril][indice];

            if (losa.estado == LOSA_SENDERO_FIRME)
            {
                losa.estado = LOSA_SENDERO_TEMBLANDO;
                losa.tiempo = TIEMPO_TEMBLOR;
                grieta[indice] = true;
                estado.caidas++;
                ReproducirSonidoMinijuego(audio, SONIDO_PLATAFORMA);
            }
        }
    }

    ActualizarLosasCaidas(*this, deltaTime);
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_SENDERO, deltaTime);

    if (algunoLlego || (jugando && tiempoRestante <= 0.0f))
    {
        if (tiempoRestante < 0.0f)
        {
            tiempoRestante = 0.0f;
        }

        FinalizarSendero(*this);
    }
}


//==================================================
// VISUAL: CEMENTERIO DE NOCHE
//==================================================
//
// Cada cuadricula es un puente de losas sueltas que cruza una fosa llena de
// niebla, desde la explanada de la entrada (con el color de su jugador)
// hasta el portal de hierro del camposanto. Detras: verja con picas,
// lapidas, cruces, mausoleos, arboles secos y la capilla; mas atras el
// valle con niebla, dos planos de colinas y una luna llena enorme. La luz
// es de luna (fria, desde arriba a la izquierda) y de faroles con llama
// verde, que tambien es el acento: el fuego fatuo que recorre la ruta.
//
// Todo lo estatico se hornea en mallas (escena y losas); solo se rehace
// la de losas cuando cambia una grieta o cae una losa. La logica no
// depende de nada de esta seccion.
//
// MODELO FUTURO: losas de piedra agrietadas, lapidas y cruces, mausoleo,
// capilla, verja y portal de hierro, farol con llama verde, arbol seco,
// cuervo, calabaza, fuego fatuo.

static const Color COLOR_FONDO_S = { 22, 22, 54, 255 };
static const Color COLOR_CIELO_ALTO_S = { 10, 12, 36, 255 };
static const Color COLOR_CIELO_MEDIO_S = { 30, 30, 78, 255 };
static const Color COLOR_CIELO_BAJO_S = { 82, 70, 130, 255 };
static const Color COLOR_HORIZONTE_S = { 120, 104, 156, 255 };
static const Color COLOR_NIEBLA_S = { 66, 70, 116, 255 };
static const Color COLOR_ABISMO_S = { 34, 38, 74, 255 };
static const Color COLOR_BRUMA_S = { 168, 180, 214, 255 };
static const Color COLOR_LUNA_S = { 252, 246, 220, 255 };
static const Color COLOR_TINTE_LUNA_S = { 178, 198, 240, 255 };
static const Color COLOR_PIEDRA_S = { 132, 136, 152, 255 };
static const Color COLOR_PIEDRA_OSCURA_S = { 84, 86, 104, 255 };
static const Color COLOR_MARMOL_S = { 200, 202, 214, 255 };
static const Color COLOR_TIERRA_S = { 62, 54, 62, 255 };
static const Color COLOR_PASTO_S = { 34, 52, 54, 255 };
static const Color COLOR_PASTO_CLARO_S = { 54, 76, 70, 255 };
static const Color COLOR_HIERRO_S = { 34, 34, 46, 255 };
static const Color COLOR_MADERA_S = { 74, 58, 56, 255 };
static const Color COLOR_LLAMA_S = { 120, 255, 172, 255 };
static const Color COLOR_GRIETA_S = { 200, 124, 255, 255 };
static const Color COLOR_META_S = { 255, 214, 120, 255 };
static const Color COLOR_CALABAZA_S = { 230, 118, 38, 255 };

static const float FOV_SENDERO = 46.0f;
static const float INCLINACION_SENDERO = 30.0f;     // grados hacia abajo
static const float Z_BORDE_FRENTE_S = 7.55f;        // explanada de la entrada
static const float Z_BORDE_FONDO_S = -9.0f;         // terraza del portal
static const float Y_TERRENO_S = -0.28f;            // un poco bajo las losas
static const float Z_FIN_CAMPOSANTO_S = -27.0f;     // detras: el valle


static float Lim01S(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalS(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static float Ruido01S(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float RuidoSuaveS(float x, float z, float escala, int semilla)
{
    float fx = x / escala, fz = z / escala;
    int ix = (int)std::floor(fx), iz = (int)std::floor(fz);
    float tx = fx - (float)ix, tz = fz - (float)iz;
    tx = tx * tx * (3.0f - 2.0f * tx);
    tz = tz * tz * (3.0f - 2.0f * tz);
    auto H = [&](int a, int b) { return Ruido01S(a * 7919 + b * 104729 + 50000, semilla); };
    float a = H(ix, iz) + (H(ix + 1, iz) - H(ix, iz)) * tx;
    float b = H(ix, iz + 1) + (H(ix + 1, iz + 1) - H(ix, iz + 1)) * tx;
    return a + (b - a) * tz;
}


static Color TonoS(Color c, float f)
{
    return { CanalS((float)c.r * f), CanalS((float)c.g * f), CanalS((float)c.b * f), c.a };
}


static Color MezclaS(Color a, Color b, float t)
{
    t = Lim01S(t);
    return {
        CanalS((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalS((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalS((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalS((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaS(Color c, float alfa)
{
    c.a = CanalS((float)c.a * Lim01S(alfa));
    return c;
}


static Color VariarS(Color c, int indice, int semilla, float amplitud)
{
    return TonoS(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01S(indice, semilla));
}


//--------------------------------------------------
// Lote de triangulos (inmediato o grabado para hornear)
//--------------------------------------------------

struct LuzHorneadaS
{
    Vector3 p;
    float radio;
    Color color;
};

static const int MAX_LUCES_HORNEADAS_S = 24;

static GrabadorMalla g_grabS;
static Vector3 g_ojoS = { 0.0f, 20.0f, 30.0f };   // camara final (bruma)
static float g_distOjoS = 36.0f;
static Vector3 g_derS = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaS = { 0.0f, 1.0f, 0.0f };
static float g_emisS = 0.0f;                      // 1 = luminoso: casi sin bruma
static bool g_lejanoS = false;                    // horizonte: bruma propia
static bool g_sinBrumaS = false;                  // efectos: color tal cual
static LuzHorneadaS g_lucesS[MAX_LUCES_HORNEADAS_S];
static int g_cantidadLucesS = 0;


// Bruma de color por distancia, niebla del abismo por profundidad y, al
// hornear, el resplandor verde de los faroles sobre lo que tienen cerca.
static Color NieblaS(Color c, Vector3 p)
{
    if (g_sinBrumaS) return c;

    if (g_grabS.activa && g_cantidadLucesS > 0 && g_emisS < 0.5f)
    {
        float r = (float)c.r, g = (float)c.g, b = (float)c.b;

        for (int k = 0; k < g_cantidadLucesS; k++)
        {
            const LuzHorneadaS& l = g_lucesS[k];
            float dx = p.x - l.p.x, dy = p.y - l.p.y, dz = p.z - l.p.z;
            float d2 = dx * dx + dy * dy + dz * dz;

            if (d2 >= l.radio * l.radio) continue;

            float f = 1.0f - std::sqrt(d2) / l.radio;
            f = f * f * 0.62f;
            r += (float)l.color.r * f;
            g += (float)l.color.g * f;
            b += (float)l.color.b * f;
        }

        c = { CanalS(r), CanalS(g), CanalS(b), c.a };
    }

    if (g_lejanoS) return c;

    float dx = p.x - g_ojoS.x, dy = p.y - g_ojoS.y, dz = p.z - g_ojoS.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01S((d - g_distOjoS * 0.9f) / (g_distOjoS * 1.9f)) * 0.72f * (1.0f - 0.7f * g_emisS);
    float h = Lim01S((-p.y - 0.7f) / 5.5f) * 0.92f * (1.0f - 0.6f * g_emisS);
    Color r = MezclaS(c, COLOR_NIEBLA_S, n);
    r = MezclaS(r, COLOR_ABISMO_S, h);
    r.a = c.a;
    return r;
}


static void VS(Vector3 p, Color c)
{
    if (g_grabS.activa)
    {
        GrabarVerticeMalla(g_grabS, p, c);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteS()
{
    if (!g_grabS.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteS()
{
    if (!g_grabS.activa) rlEnd();
}


static void TriS(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VS(a, NieblaS(color, a));
    VS(b, NieblaS(color, b));
    VS(c, NieblaS(color, c));
}


static void QuadS(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    Color ca = NieblaS(color, a), cb = NieblaS(color, b), cc = NieblaS(color, c), cd = NieblaS(color, d);
    VS(a, ca); VS(b, cb); VS(c, cc);
    VS(a, ca); VS(c, cc); VS(d, cd);
}


static void Quad4S(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    Color na = NieblaS(ca, a), nc = NieblaS(cc, c);
    VS(a, na); VS(b, NieblaS(cb, b)); VS(c, nc);
    VS(a, na); VS(c, nc); VS(d, NieblaS(cd, d));
}


// Sin bruma, un color por vertice (luces aditivas, sombras, niebla).
static void QuadColoresS(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VS(a, ca); VS(b, cb); VS(c, cc);
    VS(a, ca); VS(c, cc); VS(d, cd);
}


// Sombreado por caras (arriba 1.0, frente 0.78, izquierda 0.70, derecha
// 0.62) adaptado a la luna, que alumbra desde arriba a la izquierda.
static float LuzS(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f) return 1.0f;

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.42f) +
        (n.z > 0.0f ? az * 0.76f : az * 0.56f) +
        (n.x < 0.0f ? ax * 0.82f : ax * 0.60f);

    return f / suma;
}


static Color CaraS(Color c, Vector3 n)
{
    Color r = TonoS(c, LuzS(n));

    if (n.y > 0.3f) r = MezclaS(r, COLOR_TINTE_LUNA_S, 0.13f * n.y);

    return r;
}


static void CajaEjesS(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
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

    QuadS(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraS(color, ny));
    QuadS(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraS(color, nz));
    QuadS(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraS(color, Vector3Negate(nz)));
    QuadS(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraS(color, Vector3Negate(nx)));
    QuadS(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraS(color, nx));

    if (fondo)
    {
        QuadS(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraS(color, Vector3Negate(ny)));
    }
}


static void CajaS(Vector3 c, Vector3 t, Color color)
{
    CajaEjesS(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, false);
}


// Caja girada en Y y ladeada sobre su eje X local (grados).
static void CajaGiradaS(Vector3 c, Vector3 t, float giroY, Color color, float ladeo = 0.0f)
{
    float a = giroY * DEG2RAD;
    float l = ladeo * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz0 = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = { -uz0.x * std::sin(l), std::cos(l), -uz0.z * std::sin(l) };
    Vector3 uz = { uz0.x * std::cos(l), std::sin(l), uz0.z * std::cos(l) };

    CajaEjesS(c, Vector3Scale(ux, t.x * 0.5f), Vector3Scale(uy, t.y * 0.5f), Vector3Scale(uz, t.z * 0.5f), color, false);
}


static void VigaS(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesS(centro, Vector3Scale(v, grosor * 0.5f), Vector3Scale(w, grosor * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


static void CilS(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
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

        QuadS(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraS(color, nm)
        );

        if (tapas)
        {
            if (rb > 0.0001f) TriS(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraS(color, eje));
            if (ra > 0.0001f) TriS(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraS(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide (o cupula con desde = 0) con caras sombreadas.
static void BolaS(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float desde = -0.5f * PI)
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
            QuadS(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraS(color, n));
        }
    }
}


// Disco horizontal con color de centro y de borde (sin bruma si emis = 1).
static void DiscoS(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, bool bruma = true)
{
    Color cc = bruma ? NieblaS(centro, c) : centro;

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };
        VS(c, cc);
        VS(p0, bruma ? NieblaS(borde, p0) : borde);
        VS(p1, bruma ? NieblaS(borde, p1) : borde);
    }
}


static void AnilloS(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColoresS(i0, o0, o1, i1, dentro, fuera, fuera, dentro);
    }
}


// Disco que mira a la camara (halos).
static void HaloS(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derS, std::cos(a0) * radio), Vector3Scale(g_arribaS, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derS, std::cos(a1) * radio), Vector3Scale(g_arribaS, std::sin(a1) * radio)));
        VS(c, centro); VS(p0, borde); VS(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas, estrellas).
static void PuntoS(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derS, r), u = Vector3Scale(g_arribaS, r);
    QuadColoresS(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Tira plana entre dos puntos, ancho en el plano horizontal (grietas,
// juntas, ramas finas vistas desde arriba).
static void TiraSueloS(Vector3 a, Vector3 b, float ancho, Color color)
{
    Vector3 d = Vector3Subtract(b, a);
    float l = std::sqrt(d.x * d.x + d.z * d.z);

    if (l < 0.0001f) return;

    Vector3 n = { -d.z / l * ancho * 0.5f, 0.0f, d.x / l * ancho * 0.5f };
    QuadS(Vector3Subtract(a, n), Vector3Subtract(b, n), Vector3Add(b, n), Vector3Add(a, n), color);
}


//--------------------------------------------------
// Horno: lo estatico se graba en mallas
//--------------------------------------------------

struct HornoSendero
{
    int claveEscena = -1;
    unsigned long long claveLosas = 0;
    bool losasListas = false;
    Mesh escena{};
    bool escenaValida = false;
    Mesh losas{};
    bool losasValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoSendero g_hornoS;


static void LiberarMallasSendero()
{
    LiberarMallaHorneada(g_hornoS.escena, g_hornoS.escenaValida);
    LiberarMallaHorneada(g_hornoS.losas, g_hornoS.losasValida);
    g_hornoS.claveEscena = -1;
    g_hornoS.claveLosas = 0;
    g_hornoS.losasListas = false;
}


static void AsegurarMaterialS()
{
    AsegurarMaterialHorneado(g_hornoS.material, g_hornoS.materialListo);
}


static void DibujarMallaS(const Mesh& malla, bool valida)
{
    if (!valida) return;

    rlDrawRenderBatchActive();
    DrawMesh(malla, g_hornoS.material, MatrixIdentity());
}


//--------------------------------------------------
// Medidas de la escena
//--------------------------------------------------

static int CarrilesVisualesS(const MinijuegoSenderoInvisible& m)
{
    return m.cantidadCarriles < 2 ? 2 : m.cantidadCarriles;
}


// Mitad del ancho que ocupan las cuadriculas (borde exterior de las losas).
static float MitadAnchoS(const MinijuegoSenderoInvisible& m)
{
    return 0.5f * (float)(CarrilesVisualesS(m) - 1) * SEPARACION_CARRILES + 3.0f * PASO_LOSA - 0.07f;
}


// Borde de la fosa a los costados.
static float BordeFosaXS(const MinijuegoSenderoInvisible& m)
{
    return MitadAnchoS(m) + 1.1f;
}


static float CentroCarrilS(const MinijuegoSenderoInvisible& m, int carril)
{
    return ((float)carril - 0.5f * (float)(CarrilesVisualesS(m) - 1)) * SEPARACION_CARRILES;
}


// Distancia de la camara al centro de las cuadriculas: las cuadriculas
// ocupan cerca del 70 % del ancho; con dos jugadores manda el largo.
static float DistanciaCamaraS(const MinijuegoSenderoInvisible& m)
{
    float d = MitadAnchoS(m) / 0.5f;
    return d < 30.0f ? 30.0f : d;
}


// El centro de las cuadriculas queda siempre un poco bajo el centro de la
// pantalla (arriba cuelga el HUD y se ve el camposanto).
static Camera3D CamaraFinalS(const MinijuegoSenderoInvisible& m)
{
    float d = DistanciaCamaraS(m);
    float a = INCLINACION_SENDERO * DEG2RAD;
    float haciaCentro = (INCLINACION_SENDERO + 3.0f + 0.8f * (float)(CarrilesVisualesS(m) - 2)) * DEG2RAD;
    Vector3 centro = { 0.0f, 0.0f, ZFila(FILAS_SENDERO - 1) * 0.5f + ZFila(0) * 0.5f };
    Camera3D c{};
    c.position = { 0.0f, centro.y + std::sin(haciaCentro) * d, centro.z + std::cos(haciaCentro) * d };
    c.target = { 0.0f, c.position.y - std::sin(a) * d, c.position.z - std::cos(a) * d };
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = FOV_SENDERO;
    c.projection = CAMERA_PERSPECTIVE;
    return c;
}


// Entrada: durante la cuenta la camara baja desde lo alto del portal
// y se acerca, como quien se asoma al borde de la fosa.
static Camera3D CamaraVistaS(const MinijuegoSenderoInvisible& m)
{
    Camera3D c = CamaraFinalS(m);

    if (m.fase == FASE_SENDERO_PREPARACION && m.partidaValida)
    {
        float u = Lim01S(1.0f - m.tiempoPreparacion / DURACION_PREPARACION_SENDERO);
        float e = 1.0f - u;
        e = e * e * (3.0f - 2.0f * e) * ObtenerFactorMovimientoCamaraMinijuegos();
        float d = DistanciaCamaraS(m);
        c.position.y += 0.32f * d * e;
        c.position.z += 0.18f * d * e;
        c.position.x -= 0.12f * d * e;
        c.target.z -= 3.0f * e;
    }

    return c;
}


//--------------------------------------------------
// Losas
//--------------------------------------------------

static const float MITAD_LOSA_S = 0.5f * (PASO_LOSA - 0.14f);
static const float ALTO_LOSA_S = 0.4f;


static Color ColorLosaS(
    const MinijuegoSenderoInvisible& m,
    const Participante participantes[],
    int carril,
    int fila,
    int columna
)
{
    // Misma piedra en todas las cuadriculas: la losa (fila, columna) se ve
    // igual en cada sendero y no da pistas distintas a nadie.
    int k = fila * COLUMNAS_SENDERO + columna;
    Color base = VariarS(COLOR_PIEDRA_S, k, 11, 0.08f);
    float tinte = Ruido01S(k, 12);

    if (tinte < 0.3f) base = MezclaS(base, Color{ 118, 130, 124, 255 }, 0.4f);
    else if (tinte > 0.78f) base = MezclaS(base, Color{ 146, 134, 142, 255 }, 0.4f);

    if (fila == 0)
    {
        int jugador = m.jugadorDeCarril[carril];
        Color propio = jugador >= 0 ? participantes[jugador].color : COLOR_PIEDRA_S;
        base = MezclaS(base, propio, 0.55f);
    }
    else if (fila == FILAS_SENDERO - 1)
    {
        base = VariarS(COLOR_MARMOL_S, k, 13, 0.04f);
    }

    return base;
}


// Fisura de una losa falsa ya descubierta: zigzag de borde a borde con
// dos ramas. La misma forma sirve para la marca oscura y para su brillo.
static void FormaGrietaS(float x, float y, float z, int semilla, float ancho, Color color)
{
    const float h = MITAD_LOSA_S - 0.1f;
    Vector3 p[6];
    bool vertical = Ruido01S(semilla, 31) < 0.5f;

    for (int i = 0; i < 6; i++)
    {
        float u = -h + 2.0f * h * (float)i / 5.0f;
        float v = (Ruido01S(semilla * 7 + i, 32) - 0.5f) * 0.55f;
        if (i == 0 || i == 5) v *= 0.5f;
        p[i] = vertical ? Vector3{ x + v, y, z + u } : Vector3{ x + u, y, z + v };
    }

    for (int i = 0; i < 5; i++)
    {
        float a = ancho * (i == 2 || i == 3 ? 1.0f : 0.75f);
        TiraSueloS(p[i], p[i + 1], a, color);
    }

    for (int r = 0; r < 2; r++)
    {
        Vector3 o = p[2 + r];
        float ang = (Ruido01S(semilla + r, 33) * 2.0f - 1.0f) * 1.2f + (r == 0 ? 0.0f : PI);
        float largo = 0.25f + 0.15f * Ruido01S(semilla + r, 34);
        Vector3 d = vertical
            ? Vector3{ o.x + std::cos(ang) * largo, y, o.z + std::sin(ang) * largo * 0.6f }
            : Vector3{ o.x + std::sin(ang) * largo * 0.6f, y, o.z + std::cos(ang) * largo };
        TiraSueloS(o, d, ancho * 0.6f, color);
    }
}


// Losa de piedra con canto biselado, costados y raiz de roca colgante.
static void LosaS(float x, float y, float z, Color top, int semilla, bool detalle, bool raiz)
{
    const float h = MITAD_LOSA_S, b = 0.075f;
    float y0 = y, yb = y - 0.07f, y1 = y - ALTO_LOSA_S;
    Color lado = TonoS(top, 0.86f);

    Vector3 t00 = { x - h + b, y0, z - h + b }, t10 = { x + h - b, y0, z - h + b };
    Vector3 t11 = { x + h - b, y0, z + h - b }, t01 = { x - h + b, y0, z + h - b };
    Vector3 e00 = { x - h, yb, z - h }, e10 = { x + h, yb, z - h };
    Vector3 e11 = { x + h, yb, z + h }, e01 = { x - h, yb, z + h };
    Vector3 d00 = { x - h, y1, z - h };
    Vector3 d11 = { x + h, y1, z + h }, d01 = { x - h, y1, z + h }, d10 = { x + h, y1, z - h };

    QuadS(t00, t01, t11, t10, CaraS(top, { 0.0f, 1.0f, 0.0f }));
    QuadS(t01, e01, e11, t11, CaraS(TonoS(top, 1.04f), { 0.0f, 0.7f, 0.7f }));
    QuadS(t10, e10, e00, t00, CaraS(top, { 0.0f, 0.7f, -0.7f }));
    QuadS(t00, e00, e01, t01, CaraS(TonoS(top, 1.02f), { -0.7f, 0.7f, 0.0f }));
    QuadS(t11, e11, e10, t10, CaraS(top, { 0.7f, 0.7f, 0.0f }));
    QuadS(e01, d01, d11, e11, CaraS(lado, { 0.0f, 0.0f, 1.0f }));
    QuadS(e00, d00, d01, e01, CaraS(lado, { -1.0f, 0.0f, 0.0f }));
    QuadS(e11, d11, d10, e10, CaraS(lado, { 1.0f, 0.0f, 0.0f }));

    if (raiz)
    {
        Color roca = TonoS(COLOR_PIEDRA_OSCURA_S, 0.8f);
        Vector3 punta = {
            x + (Ruido01S(semilla, 41) - 0.5f) * 0.4f,
            y1 - 0.45f - 0.35f * Ruido01S(semilla, 42),
            z + (Ruido01S(semilla, 43) - 0.5f) * 0.4f
        };
        Vector3 r00 = { x - h + 0.12f, y1, z - h + 0.12f }, r10 = { x + h - 0.12f, y1, z - h + 0.12f };
        Vector3 r11 = { x + h - 0.12f, y1, z + h - 0.12f }, r01 = { x - h + 0.12f, y1, z + h - 0.12f };
        TriS(r01, r11, punta, CaraS(roca, { 0.0f, -0.4f, 1.0f }));
        TriS(r00, r01, punta, CaraS(roca, { -1.0f, -0.4f, 0.0f }));
        TriS(r11, r10, punta, CaraS(roca, { 1.0f, -0.4f, 0.0f }));
    }

    if (!detalle) return;

    // Musgo en una esquina y motas claras: nada que parezca una grieta.
    float ys = y0 + 0.004f;

    if (Ruido01S(semilla, 44) < 0.35f)
    {
        float sx = Ruido01S(semilla, 45) < 0.5f ? -1.0f : 1.0f;
        float sz = Ruido01S(semilla, 46) < 0.5f ? -1.0f : 1.0f;
        Vector3 c = { x + sx * (h - 0.22f), ys, z + sz * (h - 0.22f) };
        Color musgo = MezclaS(top, Color{ 70, 104, 78, 255 }, 0.55f);
        DiscoS(c, 0.2f + 0.08f * Ruido01S(semilla, 47), 0.16f, 7, musgo, MezclaS(musgo, top, 0.6f));
    }

    for (int k = 0; k < 2; k++)
    {
        float px = x + (Ruido01S(semilla * 3 + k, 48) - 0.5f) * 1.0f;
        float pz = z + (Ruido01S(semilla * 3 + k, 49) - 0.5f) * 1.0f;
        float r = 0.05f + 0.04f * Ruido01S(semilla + k, 50);
        QuadS({ px - r, ys, pz - r }, { px - r, ys, pz + r }, { px + r, ys, pz + r }, { px + r, ys, pz - r }, TonoS(top, 1.1f));
    }
}


static bool LosaVisibleS(const MinijuegoSenderoInvisible& m, int carril, int indice)
{
    return m.losas[carril][indice].estado == LOSA_SENDERO_FIRME;
}


static void LosasEstaticasS(const MinijuegoSenderoInvisible& m, const Participante participantes[], bool detalle)
{
    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        for (int fila = 0; fila < FILAS_SENDERO; fila++)
        {
            for (int columna = 0; columna < COLUMNAS_SENDERO; columna++)
            {
                int indice = IndiceLosa(fila, columna);

                if (!LosaVisibleS(m, carril, indice)) continue;

                const BloquePrueba& bloque = m.bloques[carril][indice];
                float x = bloque.posicion.x, z = bloque.posicion.z;
                bool agrietada = m.grieta[indice] && fila > 0 && fila < FILAS_SENDERO - 1;
                Color top = ColorLosaS(m, participantes, carril, fila, columna);

                if (agrietada) top = MezclaS(TonoS(top, 0.6f), Color{ 70, 52, 92, 255 }, 0.45f);

                LosaS(x, 0.0f, z, top, indice * 13 + 7, detalle && !agrietada, true);

                if (agrietada)
                {
                    FormaGrietaS(x, 0.006f, z, indice, 0.17f, Color{ 18, 8, 28, 255 });
                }

                // Meta: filete dorado alrededor de la losa de marmol.
                if (fila == FILAS_SENDERO - 1)
                {
                    float h = MITAD_LOSA_S - 0.14f, y = 0.006f, g = 0.05f;
                    Color oro = Color{ 214, 172, 92, 255 };
                    TiraSueloS({ x - h, y, z - h }, { x + h, y, z - h }, g, oro);
                    TiraSueloS({ x - h, y, z + h }, { x + h, y, z + h }, g, oro);
                    TiraSueloS({ x - h, y, z - h }, { x - h, y, z + h }, g, oro);
                    TiraSueloS({ x + h, y, z - h }, { x + h, y, z + h }, g, oro);
                }

                // Salida: flecha tallada hacia adelante.
                if (fila == 0)
                {
                    Color tallado = TonoS(top, 0.7f);
                    float y = 0.006f;
                    TriS({ x, y, z - 0.42f }, { x - 0.3f, y, z - 0.08f }, { x + 0.3f, y, z - 0.08f }, tallado);
                    QuadS({ x - 0.11f, y, z - 0.08f }, { x - 0.11f, y, z + 0.34f }, { x + 0.11f, y, z + 0.34f }, { x + 0.11f, y, z - 0.08f }, tallado);
                }
            }
        }
    }
}


static unsigned long long ClaveLosasS(const MinijuegoSenderoInvisible& m, const Participante participantes[])
{
    unsigned long long h = 1469598103934665603ull;
    auto Mezclar = [&](unsigned int v) { h ^= (unsigned long long)v; h *= 1099511628211ull; };

    Mezclar((unsigned int)m.cantidadCarriles + 1u);
    Mezclar((unsigned int)CalidadDecoracion());
    Mezclar(m.partidaValida ? 1u : 0u);

    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        int jugador = m.jugadorDeCarril[carril];
        Color c = jugador >= 0 ? participantes[jugador].color : BLACK;
        Mezclar(((unsigned int)c.r << 16) | ((unsigned int)c.g << 8) | (unsigned int)c.b);

        for (int k = 0; k < LOSAS_SENDERO; k++)
        {
            Mezclar(LosaVisibleS(m, carril, k) ? 3u : 5u);
        }
    }

    for (int k = 0; k < LOSAS_SENDERO; k++)
    {
        Mezclar(m.grieta[k] ? 7u : 11u);
    }

    return h;
}


static void HornearLosasS(const MinijuegoSenderoInvisible& m, const Participante participantes[])
{
    if (!IsWindowReady() || !m.partidaValida) return;

    unsigned long long clave = ClaveLosasS(m, participantes);

    if (g_hornoS.losasListas && g_hornoS.claveLosas == clave) return;

    LiberarMallaHorneada(g_hornoS.losas, g_hornoS.losasValida);
    AsegurarMaterialS();

    EmpezarGrabacionMalla(g_grabS);
    LosasEstaticasS(m, participantes, CalidadDecoracion() != CALIDAD_BAJA);
    g_hornoS.losasValida = TerminarGrabacionMalla(g_grabS, g_hornoS.losas);
    g_hornoS.claveLosas = clave;
    g_hornoS.losasListas = true;
}


// Intensidad de luz verde de una losa para un carril (fuego fatuo o farol).
static float LuzLosaS(const MinijuegoSenderoInvisible& m, int carril, int indice)
{
    float luz = 0.0f;

    if (m.fase == FASE_SENDERO_MEMORIZAR)
    {
        int k = m.indiceSecuencia[indice];

        if (k >= 0)
        {
            float avance =
                (1.0f - m.tiempoMemorizar / DURACION_MEMORIZAR_SENDERO) *
                (float)(m.cantidadSecuencia + 1);

            if (avance >= (float)k)
            {
                luz = Acotar(1.0f - (avance - (float)k) / 5.0f, 0.2f, 1.0f);
            }
        }
    }

    int jugador = m.jugadorDeCarril[carril];

    if (jugador < 0) return luz;

    const EstadoJugadorSendero& estado = m.estadosJugadores[jugador];

    if (estado.farolTiempo > 0.0f)
    {
        int primera = m.farolIndice + 1;
        int segunda = m.farolIndice + 2;
        bool coincide =
            (primera < m.cantidadRuta && m.ruta[primera] == indice) ||
            (segunda < m.cantidadRuta && m.ruta[segunda] == indice);

        if (coincide)
        {
            float f = Acotar(estado.farolTiempo / 0.6f, 0.0f, 1.0f);

            if (f > luz) luz = f;
        }
    }

    return luz;
}


// Segundos desde que la losa falsa k se agrieto (por la caida mas
// reciente en cualquier sendero); negativo si sigue intacta.
static float EdadGrietaS(const MinijuegoSenderoInvisible& m, int k, int* carrilCaida = nullptr)
{
    float mejor = -1.0f;

    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        const LosaSendero& losa = m.losas[carril][k];
        float edad = -1.0f;

        if (losa.estado == LOSA_SENDERO_TEMBLANDO) edad = TIEMPO_TEMBLOR - losa.tiempo;
        else if (losa.estado == LOSA_SENDERO_CAIDA) edad = TIEMPO_TEMBLOR + losa.tiempo;

        if (edad >= 0.0f && (mejor < 0.0f || edad < mejor))
        {
            mejor = edad;
            if (carrilCaida) *carrilCaida = carril;
        }
    }

    return mejor;
}


// Losas que tiemblan y trozos que caen al abismo (inmediato, cada frame).
static void DibujarLosasVivasS(const MinijuegoSenderoInvisible& m, const Participante participantes[])
{
    float t = m.tiempoAnimacion;
    float mov = 0.4f + 0.6f * ObtenerFactorMovimientoCamaraMinijuegos();

    LoteS();

    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        for (int k = 0; k < LOSAS_SENDERO; k++)
        {
            const LosaSendero& losa = m.losas[carril][k];

            if (losa.estado == LOSA_SENDERO_FIRME || (losa.estado == LOSA_SENDERO_CAIDA && losa.tiempo >= 1.4f)) continue;

            const BloquePrueba& bloque = m.bloques[carril][k];
            int fila = k / COLUMNAS_SENDERO, columna = k % COLUMNAS_SENDERO;
            Color top = ColorLosaS(m, participantes, carril, fila, columna);

            if (losa.estado == LOSA_SENDERO_TEMBLANDO)
            {
                float f = 1.0f - losa.tiempo / TIEMPO_TEMBLOR;
                float x = bloque.posicion.x + 0.06f * mov * std::sin(t * 70.0f);
                float z = bloque.posicion.z + 0.04f * mov * std::cos(t * 53.0f);
                Color rojo = MezclaS(top, Color{ 150, 92, 120, 255 }, 0.5f * f);
                LosaS(x, -0.05f * f, z, rojo, k * 13 + 7, false, true);
                FormaGrietaS(x, 0.006f - 0.05f * f, z, k, 0.04f + 0.08f * f, Color{ 26, 14, 38, 255 });
            }
            else
            {
                float tt = losa.tiempo;
                float caida = 0.5f * 16.0f * tt * tt;

                for (int p = 0; p < 4; p++)
                {
                    float sx = p % 2 == 0 ? -1.0f : 1.0f;
                    float sz = p < 2 ? -1.0f : 1.0f;
                    Vector3 c = {
                        bloque.posicion.x + sx * (0.32f + 0.5f * tt),
                        -0.2f - caida * (0.85f + 0.1f * (float)p),
                        bloque.posicion.z + sz * (0.32f + 0.4f * tt)
                    };
                    float giro = (float)p * 40.0f + tt * 260.0f * sx;
                    CajaGiradaS(c, { 0.6f, 0.36f, 0.6f }, giro, TonoS(top, 0.9f), tt * 150.0f * sz);
                }

                for (int p = 0; p < 5; p++)
                {
                    float a = (float)p * 1.3f + (float)k;
                    Vector3 c = {
                        bloque.posicion.x + std::cos(a) * (0.3f + 1.1f * tt),
                        -0.1f + 1.2f * tt - caida * 0.7f,
                        bloque.posicion.z + std::sin(a) * (0.3f + 1.0f * tt)
                    };
                    CajaGiradaS(c, { 0.14f, 0.14f, 0.14f }, a * 50.0f + tt * 400.0f, COLOR_PIEDRA_OSCURA_S);
                }
            }
        }
    }

    FinLoteS();
}


//--------------------------------------------------
// Faroles de llama verde (posiciones compartidas por horno y efectos)
//--------------------------------------------------

struct FarolEscenaS
{
    Vector3 llama;
    float alto;          // del suelo a la llama
    int tipo;            // 0 poste de la entrada, 1 pilar del portal, 2 farol del camposanto
};

static const int MAX_FAROLES_S = 24;
static FarolEscenaS g_farolesS[MAX_FAROLES_S];
static int g_cantidadFarolesS = 0;

static const int MAX_CALABAZAS_S = 8;
static Vector3 g_calabazasS[MAX_CALABAZAS_S];   // caras encendidas (horneado)
static int g_cantidadCalabazasS = 0;


static float XPortalS(const MinijuegoSenderoInvisible& m, int carril, int lado)
{
    return CentroCarrilS(m, carril) + (lado == 0 ? -1.0f : 1.0f) * 3.55f;
}


static void CalcularFarolesS(const MinijuegoSenderoInvisible& m, bool decoracion)
{
    g_cantidadFarolesS = 0;
    int n = CarrilesVisualesS(m);
    auto Agregar = [&](Vector3 p, float alto, int tipo)
    {
        if (g_cantidadFarolesS >= MAX_FAROLES_S) return;
        g_farolesS[g_cantidadFarolesS++] = { p, alto, tipo };
    };

    // Postes bajos en la explanada, entre los senderos.
    for (int k = 0; k <= n; k++)
    {
        float x = CentroCarrilS(m, 0) - 0.5f * SEPARACION_CARRILES + (float)k * SEPARACION_CARRILES;
        Agregar({ x, Y_TERRENO_S + 1.75f, Z_BORDE_FRENTE_S + 0.75f }, 1.75f, 0);
    }

    // Pilares del portal de cada sendero.
    for (int c = 0; c < n; c++)
    {
        for (int lado = 0; lado < 2; lado++)
        {
            Agregar({ XPortalS(m, c, lado), Y_TERRENO_S + 3.05f, Z_BORDE_FONDO_S - 0.9f }, 3.05f, 1);
        }
    }

    if (!decoracion) return;

    // Faroles sueltos del camposanto.
    float b = BordeFosaXS(m);
    Agregar({ -b - 4.5f, Y_TERRENO_S + 2.3f, 2.0f }, 2.3f, 2);
    Agregar({ b + 4.5f, Y_TERRENO_S + 2.3f, -1.5f }, 2.3f, 2);
    Agregar({ -b * 0.45f, Y_TERRENO_S + 2.3f, -17.5f }, 2.3f, 2);
    Agregar({ b * 0.5f, Y_TERRENO_S + 2.3f, -19.0f }, 2.3f, 2);
}


//--------------------------------------------------
// Terreno, fosa y bordillo
//--------------------------------------------------

static Color ColorSueloS(float x, float z)
{
    float n = RuidoSuaveS(x, z, 4.0f, 61);
    float n2 = RuidoSuaveS(x, z, 1.4f, 62);
    Color c = MezclaS(TonoS(COLOR_PASTO_S, 0.85f), COLOR_PASTO_CLARO_S, n * 0.9f);

    if (n2 > 0.64f) c = MezclaS(c, COLOR_TIERRA_S, (n2 - 0.64f) * 2.6f);

    return c;
}


static bool DentroFosaS(const MinijuegoSenderoInvisible& m, float x, float z)
{
    return std::fabs(x) < BordeFosaXS(m) - 0.01f && z < Z_BORDE_FRENTE_S - 0.01f && z > Z_BORDE_FONDO_S + 0.01f;
}


static void SueloS(const MinijuegoSenderoInvisible& m, bool decoracion)
{
    float b = BordeFosaXS(m);
    float paso = decoracion ? 1.6f : 3.2f;
    std::vector<float> xs, zs;

    for (float x = -64.0f; x <= 64.01f; x += paso) xs.push_back(x);
    for (float z = Z_FIN_CAMPOSANTO_S; z <= 22.01f; z += paso) zs.push_back(z);

    auto Insertar = [paso](std::vector<float>& v, float valor)
    {
        size_t i = 0;
        while (i < v.size() && v[i] < valor - 0.05f) i++;
        if (i < v.size() && std::fabs(v[i] - valor) < 0.05f) { v[i] = valor; return; }
        if (i < v.size() && std::fabs(v[i] - valor) < paso * 0.3f) { v[i] = valor; return; }
        v.insert(v.begin() + (long)i, valor);
    };

    Insertar(xs, -b);
    Insertar(xs, b);
    Insertar(zs, Z_BORDE_FONDO_S);
    Insertar(zs, Z_BORDE_FRENTE_S);

    for (size_t j = 0; j + 1 < zs.size(); j++)
    {
        for (size_t i = 0; i + 1 < xs.size(); i++)
        {
            float x0 = xs[i], x1 = xs[i + 1], z0 = zs[j], z1 = zs[j + 1];

            if (DentroFosaS(m, 0.5f * (x0 + x1), 0.5f * (z0 + z1))) continue;

            auto Y = [&](float x, float z)
            {
                // Lomas suaves lejos de la fosa.
                float lejos = Lim01S((std::fabs(x) - b - 6.0f) / 20.0f) + Lim01S((-z - 20.0f) / 8.0f) * 0.3f;
                return Y_TERRENO_S + lejos * 1.6f * RuidoSuaveS(x, z, 7.0f, 63);
            };

            Vector3 a = { x0, Y(x0, z0), z0 }, bb = { x0, Y(x0, z1), z1 };
            Vector3 c = { x1, Y(x1, z1), z1 }, d = { x1, Y(x1, z0), z0 };
            Quad4S(a, bb, c, d, ColorSueloS(a.x, a.z), ColorSueloS(bb.x, bb.z), ColorSueloS(c.x, c.z), ColorSueloS(d.x, d.z));
        }
    }
}


// Pared de roca estratificada de la fosa. Recorre de p0 a p1 (borde
// superior) y baja hasta el fondo; "adentro" apunta hacia la fosa.
static void ParedFosaS(Vector3 p0, Vector3 p1, Vector3 adentro, int semilla, float pasoLado)
{
    Vector3 eje = Vector3Subtract(p1, p0);
    float largo = Vector3Length(eje);
    int tramos = (int)(largo / pasoLado) + 1;
    const int bandas = 6;
    const float alturas[bandas + 1] = { 0.0f, -0.55f, -1.5f, -2.6f, -3.9f, -5.6f, -11.0f };
    const Color colores[bandas] = {
        Color{ 74, 64, 70, 255 }, Color{ 104, 100, 116, 255 }, Color{ 84, 80, 98, 255 },
        Color{ 110, 104, 120, 255 }, Color{ 76, 72, 92, 255 }, Color{ 60, 58, 80, 255 }
    };

    for (int b = 0; b < bandas; b++)
    {
        for (int i = 0; i < tramos; i++)
        {
            float u0 = (float)i / (float)tramos, u1 = (float)(i + 1) / (float)tramos;
            auto Punto = [&](float u, int fila)
            {
                Vector3 p = Vector3Add(p0, Vector3Scale(eje, u));
                int clave = (int)(u * 400.0f) + fila * 977 + semilla;
                float sale = (fila == 0 || fila == bandas) ? 0.0f : (0.15f + 0.35f * Ruido01S(clave, 71));
                p = Vector3Add(p, Vector3Scale(adentro, sale));
                p.y += alturas[fila] + (fila > 0 && fila < bandas ? (Ruido01S(clave, 72) - 0.5f) * 0.3f : 0.0f);
                return p;
            };

            Color col = VariarS(colores[b], i + b * 131 + semilla, 73, 0.07f);
            Vector3 a = Punto(u0, b), bb = Punto(u0, b + 1), c = Punto(u1, b + 1), d = Punto(u1, b);
            // Cara de la banda (mira a la fosa) y repisa superior clara.
            QuadS(a, bb, c, d, CaraS(col, adentro));
        }
    }
}


static void FosaS(const MinijuegoSenderoInvisible& m)
{
    float b = BordeFosaXS(m);
    float y = Y_TERRENO_S;

    ParedFosaS({ -b, y, Z_BORDE_FONDO_S }, { b, y, Z_BORDE_FONDO_S }, { 0.0f, 0.0f, 1.0f }, 100, 1.2f);
    ParedFosaS({ -b, y, Z_BORDE_FRENTE_S }, { -b, y, Z_BORDE_FONDO_S }, { 1.0f, 0.0f, 0.0f }, 300, 1.2f);
    ParedFosaS({ b, y, Z_BORDE_FONDO_S }, { b, y, Z_BORDE_FRENTE_S }, { -1.0f, 0.0f, 0.0f }, 500, 1.2f);
    ParedFosaS({ b, y, Z_BORDE_FRENTE_S }, { -b, y, Z_BORDE_FRENTE_S }, { 0.0f, 0.0f, -1.0f }, 700, 2.4f);

    // Fondo de la fosa (lo tapa la niebla).
    QuadS({ -b, -11.0f, Z_BORDE_FONDO_S }, { -b, -11.0f, Z_BORDE_FRENTE_S }, { b, -11.0f, Z_BORDE_FRENTE_S }, { b, -11.0f, Z_BORDE_FONDO_S }, COLOR_ABISMO_S);
}


// Bordillo de bloques de piedra alrededor de la fosa.
static void BordilloS(const MinijuegoSenderoInvisible& m)
{
    float b = BordeFosaXS(m);
    auto Tramo = [&](Vector3 a, Vector3 c, Vector3 afuera, int semilla)
    {
        Vector3 eje = Vector3Subtract(c, a);
        float largo = Vector3Length(eje);
        int n = (int)(largo / 0.95f) + 1;
        float giro = std::atan2(-eje.z, eje.x) * RAD2DEG;

        for (int i = 0; i < n; i++)
        {
            float u = ((float)i + 0.5f) / (float)n;
            Vector3 p = Vector3Add(Vector3Add(a, Vector3Scale(eje, u)), Vector3Scale(afuera, 0.22f));
            float alto = 0.2f + 0.08f * Ruido01S(i + semilla, 81);
            p.y = Y_TERRENO_S + alto * 0.5f - 0.02f;
            Color col = VariarS(Color{ 120, 120, 136, 255 }, i + semilla, 82, 0.1f);
            CajaGiradaS(p, { largo / (float)n - 0.06f, alto, 0.42f }, giro + (Ruido01S(i + semilla, 83) - 0.5f) * 5.0f, col);
        }
    };

    Tramo({ -b, 0.0f, Z_BORDE_FONDO_S }, { b, 0.0f, Z_BORDE_FONDO_S }, { 0.0f, 0.0f, -1.0f }, 1000);
    Tramo({ -b, 0.0f, Z_BORDE_FRENTE_S }, { b, 0.0f, Z_BORDE_FRENTE_S }, { 0.0f, 0.0f, 1.0f }, 2000);
    Tramo({ -b, 0.0f, Z_BORDE_FONDO_S }, { -b, 0.0f, Z_BORDE_FRENTE_S }, { -1.0f, 0.0f, 0.0f }, 3000);
    Tramo({ b, 0.0f, Z_BORDE_FONDO_S }, { b, 0.0f, Z_BORDE_FRENTE_S }, { 1.0f, 0.0f, 0.0f }, 4000);
}


//--------------------------------------------------
// Faroles, portales y verja
//--------------------------------------------------

// Jaula de hierro del farol (la llama se dibuja aparte, viva).
static void JaulaFarolS(Vector3 llama, float escala)
{
    float s = escala;
    Vector3 base = { llama.x, llama.y - 0.32f * s, llama.z };
    CajaS(base, { 0.36f * s, 0.08f * s, 0.36f * s }, COLOR_HIERRO_S);

    for (int k = 0; k < 4; k++)
    {
        float sx = k % 2 == 0 ? -1.0f : 1.0f, sz = k < 2 ? -1.0f : 1.0f;
        CajaS({ llama.x + sx * 0.15f * s, llama.y, llama.z + sz * 0.15f * s }, { 0.04f * s, 0.6f * s, 0.04f * s }, COLOR_HIERRO_S);
    }

    CajaS({ llama.x, llama.y + 0.32f * s, llama.z }, { 0.42f * s, 0.07f * s, 0.42f * s }, COLOR_HIERRO_S);
    CilS({ llama.x, llama.y + 0.35f * s, llama.z }, { llama.x, llama.y + 0.62f * s, llama.z }, 0.26f * s, 0.0f, 4, COLOR_HIERRO_S);
    CilS({ llama.x, llama.y + 0.62f * s, llama.z }, { llama.x, llama.y + 0.74f * s, llama.z }, 0.035f * s, 0.035f * s, 4, COLOR_HIERRO_S);
}


static void FarolPosteS(const FarolEscenaS& f)
{
    Vector3 pie = { f.llama.x, Y_TERRENO_S, f.llama.z };
    CajaS({ pie.x, pie.y + 0.12f, pie.z }, { 0.5f, 0.24f, 0.5f }, COLOR_PIEDRA_OSCURA_S);
    CilS({ pie.x, pie.y + 0.24f, pie.z }, { pie.x, f.llama.y - 0.36f, pie.z }, 0.07f, 0.05f, 6, COLOR_HIERRO_S);
    // Ganchito curvo (voluta) bajo la jaula.
    VigaS({ pie.x, f.llama.y - 0.5f, pie.z }, { pie.x + 0.18f, f.llama.y - 0.62f, pie.z }, 0.04f, COLOR_HIERRO_S);
    JaulaFarolS(f.llama, 1.0f);
}


static void PortalS(const MinijuegoSenderoInvisible& m, int carril, bool decoracion)
{
    float z = Z_BORDE_FONDO_S - 0.9f;
    float y = Y_TERRENO_S;
    float xs[2] = { XPortalS(m, carril, 0), XPortalS(m, carril, 1) };
    float cx = CentroCarrilS(m, carril);
    Color piedra = Color{ 142, 140, 156, 255 };

    for (int lado = 0; lado < 2; lado++)
    {
        float x = xs[lado];
        CajaS({ x, y + 0.2f, z }, { 1.0f, 0.4f, 1.0f }, TonoS(piedra, 0.9f));
        CajaS({ x, y + 1.45f, z }, { 0.74f, 2.1f, 0.74f }, piedra);
        CajaS({ x, y + 2.58f, z }, { 0.92f, 0.16f, 0.92f }, TonoS(piedra, 1.05f));
        if (decoracion)
        {
            // Placa con calavera simpatica tallada en el frente del pilar.
            CajaS({ x, y + 1.6f, z + 0.38f }, { 0.44f, 0.5f, 0.04f }, TonoS(piedra, 0.82f));
            BolaS({ x, y + 1.66f, z + 0.41f }, { 0.13f, 0.13f, 0.05f }, COLOR_MARMOL_S, 2, 8);
        }
    }

    // Arco de hierro con remate y tres picas.
    const int segmentos = 10;
    Vector3 previo{};

    for (int k = 0; k <= segmentos; k++)
    {
        float u = (float)k / (float)segmentos;
        float x = xs[0] + (xs[1] - xs[0]) * u;
        float alto = y + 2.7f + 1.05f * std::sin(u * PI);
        Vector3 p = { x, alto, z };
        if (k > 0) VigaS(previo, p, 0.11f, COLOR_HIERRO_S);
        previo = p;
    }

    for (int k = 0; k <= segmentos; k++)
    {
        float u = (float)k / (float)segmentos;
        float x = xs[0] + (xs[1] - xs[0]) * u;
        float alto = y + 2.7f + 0.7f * std::sin(u * PI);
        if (k > 0 && k < segmentos)
        {
            float tope = y + 2.7f + 1.05f * std::sin(u * PI);
            VigaS({ x, alto, z }, { x, tope, z }, 0.05f, COLOR_HIERRO_S);
        }
    }

    for (int k = 0; k <= segmentos; k++)
    {
        if (k == 0) { previo = { xs[0], y + 2.7f, z }; continue; }
        float u = (float)k / (float)segmentos;
        Vector3 p = { xs[0] + (xs[1] - xs[0]) * u, y + 2.7f + 0.7f * std::sin(u * PI), z };
        VigaS(previo, p, 0.07f, COLOR_HIERRO_S);
        previo = p;
    }

    CilS({ cx, y + 3.75f, z }, { cx, y + 4.35f, z }, 0.05f, 0.05f, 4, COLOR_HIERRO_S);
    VigaS({ cx - 0.22f, y + 4.12f, z }, { cx + 0.22f, y + 4.12f, z }, 0.05f, COLOR_HIERRO_S);

    // Hojas del portal abiertas hacia adentro del camposanto.
    for (int lado = 0; lado < 2; lado++)
    {
        float s = lado == 0 ? 1.0f : -1.0f;
        Vector3 bisagra = { xs[lado] + s * 0.42f, y, z - 0.1f };
        Vector3 dir = Vector3Normalize(Vector3{ s * 0.35f, 0.0f, -0.94f });
        float ancho = 0.5f * (xs[1] - xs[0]) - 0.5f;
        Vector3 fin = Vector3Add(bisagra, Vector3Scale(dir, ancho));

        VigaS({ bisagra.x, y + 0.35f, bisagra.z }, { fin.x, y + 0.35f, fin.z }, 0.07f, COLOR_HIERRO_S);
        VigaS({ bisagra.x, y + 2.1f, bisagra.z }, { fin.x, y + 2.1f, fin.z }, 0.07f, COLOR_HIERRO_S);

        int barras = decoracion ? 9 : 5;
        for (int k = 0; k <= barras; k++)
        {
            Vector3 p = Vector3Add(bisagra, Vector3Scale(dir, ancho * (float)k / (float)barras));
            float tope = y + 2.3f + 0.25f * std::sin((float)k / (float)barras * PI);
            VigaS({ p.x, y + 0.2f, p.z }, { p.x, tope, p.z }, 0.05f, COLOR_HIERRO_S);
            CilS({ p.x, tope, p.z }, { p.x, tope + 0.18f, p.z }, 0.06f, 0.0f, 4, COLOR_HIERRO_S);
        }
    }
}


// Verja de hierro con picas a lo largo del fondo (deja los portales).
static void VerjaS(const MinijuegoSenderoInvisible& m, bool decoracion)
{
    float z = Z_BORDE_FONDO_S - 0.9f;
    float y = Y_TERRENO_S;
    float b = BordeFosaXS(m);
    float limite = b + (decoracion ? 26.0f : 14.0f);
    float paso = decoracion ? 0.34f : 0.6f;
    int n = CarrilesVisualesS(m);

    auto EnPortal = [&](float x)
    {
        for (int c = 0; c < n; c++)
        {
            if (x > XPortalS(m, c, 0) - 0.5f && x < XPortalS(m, c, 1) + 0.5f) return true;
        }
        return false;
    };

    // Tramos de baranda entre portales.
    float inicio = -limite;
    bool dentro = false;

    for (float x = -limite; x <= limite + 0.001f; x += 0.25f)
    {
        bool corta = EnPortal(x) || x >= limite;

        if (!corta && !dentro) { inicio = x; dentro = true; }

        if (corta && dentro)
        {
            VigaS({ inicio, y + 0.45f, z }, { x, y + 0.45f, z }, 0.07f, COLOR_HIERRO_S);
            VigaS({ inicio, y + 1.55f, z }, { x, y + 1.55f, z }, 0.07f, COLOR_HIERRO_S);
            dentro = false;
        }
    }

    int i = 0;

    for (float x = -limite; x <= limite; x += paso, i++)
    {
        if (EnPortal(x)) continue;

        bool poste = i % 12 == 0;
        float alto = poste ? 2.0f : 1.75f + (i % 2 == 0 ? 0.08f : 0.0f);

        if (poste)
        {
            CajaS({ x, y + 1.0f, z }, { 0.16f, 2.0f, 0.16f }, COLOR_HIERRO_S);
            BolaS({ x, y + 2.1f, z }, { 0.11f, 0.11f, 0.11f }, COLOR_HIERRO_S, 2, 6);
        }
        else
        {
            VigaS({ x, y, z }, { x, y + alto, z }, 0.045f, COLOR_HIERRO_S);
            CilS({ x, y + alto, z }, { x, y + alto + 0.2f, z }, 0.07f, 0.0f, 4, COLOR_HIERRO_S);
        }
    }
}


//--------------------------------------------------
// Escena estatica horneada
//--------------------------------------------------

static void EscenaEstaticaS(const MinijuegoSenderoInvisible& m, bool decoracion, int paso);


static int ClaveEscenaS(const MinijuegoSenderoInvisible& m)
{
    return CarrilesVisualesS(m) * 8 + (int)CalidadDecoracion();
}


static void HornearEscenaS(const MinijuegoSenderoInvisible& m)
{
    int clave = ClaveEscenaS(m);

    if ((g_hornoS.escenaValida && g_hornoS.claveEscena == clave) || !IsWindowReady()) return;

    LiberarMallaHorneada(g_hornoS.escena, g_hornoS.escenaValida);
    AsegurarMaterialS();

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Resplandor verde de los faroles horneado en lo que los rodea.
    g_cantidadLucesS = 0;

    for (int k = 0; k < g_cantidadFarolesS && g_cantidadLucesS < MAX_LUCES_HORNEADAS_S; k++)
    {
        g_lucesS[g_cantidadLucesS++] = { g_farolesS[k].llama, g_farolesS[k].tipo == 1 ? 4.2f : 3.6f, Color{ 60, 150, 100, 255 } };
    }

    EmpezarGrabacionMalla(g_grabS);
    EscenaEstaticaS(m, decoracion, paso);
    g_hornoS.escenaValida = TerminarGrabacionMalla(g_grabS, g_hornoS.escena);
    g_hornoS.claveEscena = clave;
    g_cantidadLucesS = 0;
}


//--------------------------------------------------
// Cielo: degrade, luna llena, estrellas y nubes
//--------------------------------------------------

struct BaseCamaraS
{
    Vector3 ojo, f, r, u;
    float tv, th;
};


static BaseCamaraS BaseDeS(const Camera3D& c)
{
    BaseCamaraS b;
    b.ojo = c.position;
    b.f = Vector3Normalize(Vector3Subtract(c.target, c.position));
    b.r = Vector3Normalize(Vector3CrossProduct(b.f, c.up));
    b.u = Vector3CrossProduct(b.r, b.f);
    b.tv = std::tan(c.fovy * 0.5f * DEG2RAD);
    float aspecto = (float)GetScreenWidth() / (float)(GetScreenHeight() > 0 ? GetScreenHeight() : 1);
    b.th = b.tv * aspecto;
    return b;
}


static Vector3 DireccionS(const BaseCamaraS& b, float sx, float sy)
{
    return Vector3Normalize(Vector3Add(b.f, Vector3Add(Vector3Scale(b.r, sx * b.th), Vector3Scale(b.u, sy * b.tv))));
}


static Color ColorCieloS(float elevacion)
{
    const float el[5] = { -6.0f, 2.0f, 9.0f, 18.0f, 34.0f };
    const Color co[5] = { COLOR_HORIZONTE_S, COLOR_HORIZONTE_S, COLOR_CIELO_BAJO_S, COLOR_CIELO_MEDIO_S, COLOR_CIELO_ALTO_S };

    if (elevacion <= el[0]) return co[0];

    for (int k = 0; k < 4; k++)
    {
        if (elevacion <= el[k + 1]) return MezclaS(co[k], co[k + 1], (elevacion - el[k]) / (el[k + 1] - el[k]));
    }

    return co[4];
}


// Franja de cielo visible desde la camara final: depresion (grados bajo
// la horizontal) del borde superior de la pantalla y del muro del fondo.
static void BandaCieloS(Vector3 ojo, float& arriba, float& borde)
{
    arriba = INCLINACION_SENDERO - FOV_SENDERO * 0.5f;
    borde = std::atan2(ojo.y - (Y_TERRENO_S + 0.8f), ojo.z - Z_FIN_CAMPOSANTO_S) * RAD2DEG;
}


static float YPorDepresionS(Vector3 ojo, float z, float depresion)
{
    return ojo.y - (ojo.z - z) * std::tan(depresion * DEG2RAD);
}


// Luna llena enorme saliendo detras de las colinas, a la izquierda.
static Vector3 PosicionLunaS(const BaseCamaraS& final, float& radio)
{
    float arriba = 0.0f, borde = 0.0f;
    BandaCieloS(final.ojo, arriba, borde);
    float el = -(borde - 0.6f * (borde - arriba)) * DEG2RAD;
    float az = -33.0f * DEG2RAD;
    Vector3 d = { std::sin(az) * std::cos(el), std::sin(el), -std::cos(az) * std::cos(el) };
    radio = 560.0f * std::tan(FOV_SENDERO * 0.078f * DEG2RAD);
    return Vector3Add(final.ojo, Vector3Scale(d, 560.0f));
}


// Disco de la luna con mares (opaco, antes del cielo).
static void DibujarLunaS(const BaseCamaraS& final)
{
    float r = 0.0f;
    Vector3 luna = PosicionLunaS(final, r);
    Vector3 cerca = Vector3Scale(final.f, -1.5f);

    g_sinBrumaS = true;
    LoteS();
    HaloS(luna, r, COLOR_LUNA_S, Color{ 232, 222, 196, 255 }, 32);

    const float mx[6] = { -0.32f, 0.18f, 0.36f, -0.1f, 0.05f, -0.45f };
    const float my[6] = { 0.22f, 0.32f, -0.12f, -0.34f, 0.02f, -0.18f };
    const float mr[6] = { 0.26f, 0.2f, 0.17f, 0.22f, 0.12f, 0.1f };

    for (int k = 0; k < 6; k++)
    {
        Vector3 c = Vector3Add(Vector3Add(luna, cerca), Vector3Add(Vector3Scale(g_derS, mx[k] * r), Vector3Scale(g_arribaS, my[k] * r)));
        HaloS(c, mr[k] * r, Color{ 226, 218, 198, 255 }, Color{ 240, 232, 208, 255 }, 12);
    }

    FinLoteS();
    g_sinBrumaS = false;
}


static void DibujarCieloS(const Camera3D& vista)
{
    BaseCamaraS b = BaseDeS(vista);
    const int filas = 12, columnas = 4;

    g_sinBrumaS = true;
    LoteS();

    for (int i = 0; i < filas; i++)
    {
        float sy0 = -1.25f + 2.5f * (float)i / (float)filas;
        float sy1 = -1.25f + 2.5f * (float)(i + 1) / (float)filas;

        for (int j = 0; j < columnas; j++)
        {
            float sx0 = -1.25f + 2.5f * (float)j / (float)columnas;
            float sx1 = -1.25f + 2.5f * (float)(j + 1) / (float)columnas;
            Vector3 d[4] = { DireccionS(b, sx0, sy0), DireccionS(b, sx1, sy0), DireccionS(b, sx1, sy1), DireccionS(b, sx0, sy1) };
            Color c[4];
            Vector3 p[4];

            for (int k = 0; k < 4; k++)
            {
                c[k] = ColorCieloS(std::asin(d[k].y) * RAD2DEG);
                p[k] = Vector3Add(b.ojo, Vector3Scale(d[k], 650.0f));
            }

            QuadColoresS(p[0], p[1], p[2], p[3], c[0], c[1], c[2], c[3]);
        }
    }

    FinLoteS();
    g_sinBrumaS = false;
}


// Estrellas que titilan y resplandor de la luna (aditivo, tras el cielo).
static void DibujarBrilloCieloS(const BaseCamaraS& final, float t, bool decoracion)
{
    float r = 0.0f;
    Vector3 luna = PosicionLunaS(final, r);

    LoteS();
    HaloS(luna, r * 3.4f, Color{ 120, 110, 190, 46 }, Color{ 120, 110, 190, 0 }, 28);
    HaloS(luna, r * 1.7f, Color{ 230, 220, 255, 60 }, Color{ 200, 190, 255, 0 }, 28);

    int estrellas = decoracion ? 110 : 50;

    for (int k = 0; k < estrellas; k++)
    {
        float sx = -1.3f + 2.6f * Ruido01S(k, 201);
        float sy = 0.05f + 1.25f * Ruido01S(k, 202);
        Vector3 d = DireccionS(final, sx, sy);
        float brillo = 0.55f + 0.45f * std::sin(t * (1.5f + 2.5f * Ruido01S(k, 203)) + (float)k * 2.1f);
        float tam = 0.5f + 0.9f * Ruido01S(k, 204);
        Color c = Ruido01S(k, 205) < 0.2f ? Color{ 255, 236, 200, 255 } : Color{ 220, 230, 255, 255 };
        PuntoS(Vector3Add(final.ojo, Vector3Scale(d, 600.0f)), tam, AlfaS(c, 0.35f + 0.6f * brillo));
    }

    FinLoteS();
}


// Nubes finas que cruzan delante de la luna (alfa).
static void DibujarNubesS(const BaseCamaraS& final, float t, bool decoracion)
{
    int nubes = decoracion ? 4 : 2;
    float mov = ObtenerFactorMovimientoCamaraMinijuegos() > 0.5f ? 1.0f : 0.35f;

    LoteS();

    for (int i = 0; i < nubes; i++)
    {
        float sy = 0.42f + 0.42f * Ruido01S(i, 211);
        float sx = std::fmod(Ruido01S(i, 212) * 3.4f + t * (0.006f + 0.006f * Ruido01S(i, 213)) * mov, 3.4f) - 1.7f;
        Vector3 c = Vector3Add(final.ojo, Vector3Scale(DireccionS(final, sx, sy), 590.0f));
        float rx = 120.0f + 90.0f * Ruido01S(i, 214), ry = 9.0f + 7.0f * Ruido01S(i, 215);
        Color dentro = Color{ 52, 46, 98, 170 }, fuera = Color{ 52, 46, 98, 0 };

        for (int lobulo = 0; lobulo < 3; lobulo++)
        {
            Vector3 o = Vector3Add(c, Vector3Add(Vector3Scale(g_derS, ((float)lobulo - 1.0f) * rx * 0.45f), Vector3Scale(g_arribaS, (lobulo == 1 ? 0.5f : 0.0f) * ry)));
            float lx = rx * (lobulo == 1 ? 0.6f : 0.45f), ly = ry * (lobulo == 1 ? 1.2f : 0.8f);

            for (int k = 0; k < 14; k++)
            {
                float a0 = 2.0f * PI * (float)k / 14.0f, a1 = 2.0f * PI * (float)(k + 1) / 14.0f;
                Vector3 p0 = Vector3Add(o, Vector3Add(Vector3Scale(g_derS, std::cos(a0) * lx), Vector3Scale(g_arribaS, std::sin(a0) * ly)));
                Vector3 p1 = Vector3Add(o, Vector3Add(Vector3Scale(g_derS, std::cos(a1) * lx), Vector3Scale(g_arribaS, std::sin(a1) * ly)));
                VS(o, dentro); VS(p0, fuera); VS(p1, fuera);
            }
        }
    }

    FinLoteS();
}


//--------------------------------------------------
// Niebla: pocas capas grandes y suaves
//--------------------------------------------------

static void CapaNieblaS(float x0, float x1, float z0, float z1, float y, int nx, int nz, Color color, float alfa, float t, float velocidad, float escala, int semilla)
{
    auto A = [&](float x, float z)
    {
        float n = RuidoSuaveS(x + t * velocidad, z + t * velocidad * 0.3f, escala, semilla);
        float n2 = RuidoSuaveS(x - t * velocidad * 0.6f, z, escala * 0.45f, semilla + 1);
        float borde = Lim01S((x - x0) / 3.0f) * Lim01S((x1 - x) / 3.0f) * Lim01S((z - z0) / 2.0f) * Lim01S((z1 - z) / 2.0f);
        float v = Lim01S((n * 0.7f + n2 * 0.5f) - 0.15f);
        return AlfaS(color, alfa * v * borde);
    };

    for (int j = 0; j < nz; j++)
    {
        float za = z0 + (z1 - z0) * (float)j / (float)nz;
        float zb = z0 + (z1 - z0) * (float)(j + 1) / (float)nz;

        for (int i = 0; i < nx; i++)
        {
            float xa = x0 + (x1 - x0) * (float)i / (float)nx;
            float xb = x0 + (x1 - x0) * (float)(i + 1) / (float)nx;
            QuadColoresS({ xa, y, za }, { xa, y, zb }, { xb, y, zb }, { xb, y, za }, A(xa, za), A(xa, zb), A(xb, zb), A(xb, za));
        }
    }
}


static void DibujarNieblaS(const MinijuegoSenderoInvisible& m, bool decoracion)
{
    float t = m.tiempoAnimacion * (0.3f + 0.7f * ObtenerFactorMovimientoCamaraMinijuegos());
    float b = BordeFosaXS(m);
    int nx = decoracion ? 28 : 16;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    LoteS();

    // Abismo: un velo profundo y otro mas claro que se arrastra bajo las losas.
    CapaNieblaS(-b, b, Z_BORDE_FONDO_S, Z_BORDE_FRENTE_S, -3.6f, nx, 10, Color{ 92, 100, 156, 255 }, 0.8f, t, 0.5f, 5.0f, 301);
    CapaNieblaS(-b, b, Z_BORDE_FONDO_S, Z_BORDE_FRENTE_S, -1.5f, nx, 10, COLOR_BRUMA_S, 0.3f, t, 0.8f, 3.5f, 311);

    // Niebla baja sobre el camposanto y la explanada.
    if (decoracion)
    {
        CapaNieblaS(-b - 30.0f, b + 30.0f, Z_FIN_CAMPOSANTO_S, Z_BORDE_FONDO_S - 0.4f, Y_TERRENO_S + 0.32f, nx, 8, COLOR_BRUMA_S, 0.3f, t, 0.6f, 4.0f, 321);
    }

    CapaNieblaS(-b - 24.0f, b + 24.0f, Z_BORDE_FRENTE_S + 0.5f, Z_BORDE_FRENTE_S + 12.0f, Y_TERRENO_S + 0.22f, nx, 5, COLOR_BRUMA_S, 0.12f, t, 0.7f, 3.0f, 331);

    FinLoteS();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Fuego fatuo, farol del tramo medio y luces
//--------------------------------------------------

// Posicion del fuego fatuo de un carril segun el avance de la memorizacion.
static Vector3 PosicionFuegoFatuoS(const MinijuegoSenderoInvisible& m, int carril, float avance)
{
    if (avance < 0.0f) avance = 0.0f;

    int a = (int)avance;

    if (a >= m.cantidadSecuencia - 1) a = m.cantidadSecuencia - 2;
    if (a < 0) a = 0;

    float f = Lim01S(avance - (float)a);
    int ia = m.secuencia[a];
    int ib = m.secuencia[a + 1];
    float x = XColumna(m, carril, ia % COLUMNAS_SENDERO) * (1.0f - f) + XColumna(m, carril, ib % COLUMNAS_SENDERO) * f;
    float z = ZFila(ia / COLUMNAS_SENDERO) * (1.0f - f) + ZFila(ib / COLUMNAS_SENDERO) * f;
    return { x, 0.95f, z };
}


static float AvanceFuegoFatuoS(const MinijuegoSenderoInvisible& m)
{
    return (1.0f - m.tiempoMemorizar / DURACION_MEMORIZAR_SENDERO) * (float)(m.cantidadSecuencia - 1);
}


static Vector3 PosicionFarolRutaS(const MinijuegoSenderoInvisible& m, int carril, float t)
{
    int indice = m.ruta[m.farolIndice];
    return {
        XColumna(m, carril, indice % COLUMNAS_SENDERO),
        0.62f + 0.08f * std::sin(t * 3.0f + (float)carril),
        ZFila(indice / COLUMNAS_SENDERO)
    };
}


static bool FarolRutaVisibleS(const MinijuegoSenderoInvisible& m, int jugador)
{
    return m.partidaValida && m.fase != FASE_SENDERO_PREPARACION && jugador >= 0 && !m.estadosJugadores[jugador].farolRecogido;
}


// Seguimiento visual (solo para destellos): tramo previo de cada jugador.
struct SeguimientoVisualS
{
    float ultimoTiempo = -1.0f;
    int progreso[MAX_PARTICIPANTES]{};
    float destello[MAX_PARTICIPANTES]{};
    Vector3 lugar[MAX_PARTICIPANTES]{};
};

static SeguimientoVisualS g_seguimientoS;


static void ActualizarSeguimientoS(const MinijuegoSenderoInvisible& m, const JugadorPrueba jugadores[], int limite)
{
    SeguimientoVisualS& s = g_seguimientoS;
    float dt = m.tiempoAnimacion - s.ultimoTiempo;

    if (s.ultimoTiempo < 0.0f || dt < 0.0f || dt > 1.0f)
    {
        s = SeguimientoVisualS{};
        for (int i = 0; i < limite; i++) s.progreso[i] = m.estadosJugadores[i].progreso;
        dt = 0.0f;
    }

    s.ultimoTiempo = m.tiempoAnimacion;

    for (int i = 0; i < limite; i++)
    {
        if (s.destello[i] > 0.0f) s.destello[i] -= dt;

        if (m.estadosJugadores[i].progreso > s.progreso[i])
        {
            s.destello[i] = 0.6f;
            s.lugar[i] = { jugadores[i].posicion.x, 0.03f, jugadores[i].posicion.z };
        }

        s.progreso[i] = m.estadosJugadores[i].progreso;
    }
}


// Cuervo posado: cuerpo, cabeza que picotea y alas que a veces se sacuden.
static void CuervoS(Vector3 p, float giro, float t, int semilla)
{
    Color pluma = Color{ 30, 26, 44, 255 };
    float a = giro * DEG2RAD;
    Vector3 f = { std::sin(a), 0.0f, std::cos(a) }, l = { std::cos(a), 0.0f, -std::sin(a) };
    float ciclo = std::fmod(t * 0.5f + Ruido01S(semilla, 601) * 7.0f, 4.0f);
    float pico = ciclo < 0.5f ? std::sin(ciclo / 0.5f * PI) : 0.0f;
    float aleteo = ciclo > 2.6f && ciclo < 3.2f ? std::sin((ciclo - 2.6f) / 0.6f * PI * 3.0f) : 0.0f;

    BolaS({ p.x, p.y + 0.18f, p.z }, { 0.16f, 0.15f, 0.16f }, pluma, 3, 7);
    Vector3 cola = Vector3Subtract({ p.x, p.y + 0.16f, p.z }, Vector3Scale(f, 0.3f));
    VigaS({ p.x, p.y + 0.18f, p.z }, cola, 0.09f, pluma);
    Vector3 cabeza = Vector3Add({ p.x, p.y + 0.36f - 0.12f * pico, p.z }, Vector3Scale(f, 0.14f + 0.06f * pico));
    BolaS(cabeza, { 0.1f, 0.1f, 0.1f }, pluma, 2, 6);
    Vector3 punta = Vector3Add(cabeza, Vector3Add(Vector3Scale(f, 0.16f), Vector3{ 0.0f, -0.04f - 0.04f * pico, 0.0f }));
    VigaS(Vector3Add(cabeza, Vector3Scale(f, 0.06f)), punta, 0.04f, Color{ 220, 170, 70, 255 });

    for (int lado = 0; lado < 2; lado++)
    {
        float s = lado == 0 ? -1.0f : 1.0f;
        Vector3 hombro = Vector3Add({ p.x, p.y + 0.22f, p.z }, Vector3Scale(l, s * 0.1f));
        Vector3 ala = Vector3Add(hombro, Vector3Add(Vector3Scale(l, s * (0.08f + 0.3f * std::fabs(aleteo))), Vector3Add(Vector3Scale(f, -0.22f), Vector3{ 0.0f, 0.2f * aleteo, 0.0f })));
        VigaS(hombro, ala, 0.05f, TonoS(pluma, 0.9f));
    }

    // Ojo brillante (simpatico, no amenazante).
    BolaS(Vector3Add(cabeza, Vector3Add(Vector3Scale(f, 0.05f), Vector3Scale(l, 0.07f))), { 0.025f, 0.025f, 0.025f }, Color{ 255, 240, 200, 255 }, 2, 4);
}


// Murcielagos que revolotean frente a la luna.
static void MurcielagosS(const BaseCamaraS& final, float t, int cantidad)
{
    float r = 0.0f;
    Vector3 luna = PosicionLunaS(final, r);
    Vector3 hacia = Vector3Normalize(Vector3Subtract(luna, final.ojo));
    Vector3 centro = Vector3Add(final.ojo, Vector3Scale(hacia, 300.0f));
    float escala = 300.0f / 560.0f * r;
    Color ala = Color{ 26, 22, 46, 255 };

    for (int k = 0; k < cantidad; k++)
    {
        float vel = 0.5f + 0.4f * Ruido01S(k, 611);
        float an = t * vel + Ruido01S(k, 612) * 6.28f;
        float rx = escala * (1.4f + 1.4f * Ruido01S(k, 613)), ry = escala * (0.6f + 0.6f * Ruido01S(k, 614));
        Vector3 p = Vector3Add(centro, Vector3Add(Vector3Scale(g_derS, std::cos(an) * rx), Vector3Scale(g_arribaS, std::sin(an * 1.3f) * ry + escala * 0.3f)));
        float aleteo = std::sin(t * 16.0f + (float)k * 2.0f);
        float w = escala * 0.22f, h = escala * 0.12f * aleteo;
        Vector3 izq = Vector3Add(p, Vector3Add(Vector3Scale(g_derS, -w), Vector3Scale(g_arribaS, h)));
        Vector3 der = Vector3Add(p, Vector3Add(Vector3Scale(g_derS, w), Vector3Scale(g_arribaS, h)));
        Vector3 abajo = Vector3Add(p, Vector3Scale(g_arribaS, -escala * 0.05f));
        Vector3 medio = Vector3Add(p, Vector3Scale(g_arribaS, escala * 0.04f));
        VS(medio, ala); VS(izq, ala); VS(abajo, ala);
        VS(medio, ala); VS(abajo, ala); VS(der, ala);
    }
}


// Partes opacas animadas: jaulas de los faroles que se recogen o se llevan,
// cuervos y papelitos sobre el ganador.
static void DibujarAnimadoS(const MinijuegoSenderoInvisible& m, const JugadorPrueba jugadores[], const Participante participantes[], int limite, bool decoracion)
{
    float t = m.tiempoAnimacion;

    LoteS();

    if (decoracion)
    {
        float b = BordeFosaXS(m);
        float zv = Z_BORDE_FONDO_S - 0.9f;
        float yv = Y_TERRENO_S + 1.6f;
        CuervoS({ -b - 3.3f, yv, zv }, 20.0f, t, 1);
        CuervoS({ b + 5.1f, yv, zv }, -30.0f, t, 2);
        CuervoS({ CentroCarrilS(m, 0) + 5.0f, yv, zv }, 160.0f, t, 3);
        CuervoS({ XPortalS(m, CarrilesVisualesS(m) - 1, 1), Y_TERRENO_S + 2.66f, zv + 0.25f }, -15.0f, t, 4);
    }

    // Papelitos sobre quien gano.
    if (m.fase == FASE_SENDERO_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        const Color papel[5] = { Color{ 255, 214, 90, 255 }, Color{ 140, 255, 180, 255 }, Color{ 200, 140, 255, 255 }, Color{ 255, 140, 120, 255 }, Color{ 140, 200, 255, 255 } };
        int cantidad = (int)(40.0f * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1) continue;

            Vector3 c = jugadores[i].posicion;

            for (int k = 0; k < cantidad; k++)
            {
                float vida = std::fmod(t * (0.35f + 0.2f * Ruido01S(k, 621)) + Ruido01S(k, 622), 1.0f);
                float an = Ruido01S(k, 623) * 6.28f + t * 1.5f;
                float r = 0.3f + 1.4f * Ruido01S(k, 624);
                Vector3 p = { c.x + std::cos(an) * r, c.y + 3.6f - 4.0f * vida, c.z + std::sin(an) * r * 0.7f };
                PuntoS(p, 0.06f, papel[(k + i) % 5]);
            }
        }
        (void)participantes;
    }

    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        int jugador = m.jugadorDeCarril[carril];

        if (FarolRutaVisibleS(m, jugador))
        {
            Vector3 p = PosicionFarolRutaS(m, carril, t);
            JaulaFarolS(p, 0.75f);
            CilS({ p.x, p.y + 0.55f, p.z }, { p.x, p.y + 0.75f, p.z }, 0.08f, 0.08f, 6, COLOR_HIERRO_S, false);
        }

        if (jugador >= 0 && m.estadosJugadores[jugador].farolTiempo > 0.0f && !jugadores[jugador].cayendo)
        {
            Vector3 c = jugadores[jugador].posicion;
            Vector3 p = { c.x + 0.45f, c.y + 1.25f + 0.06f * std::sin(t * 4.0f), c.z };
            JaulaFarolS(p, 0.6f);
        }
    }

    FinLoteS();
}


static float ParpadeoS(float t, int k)
{
    return 0.82f + 0.12f * std::sin(t * 9.0f + (float)k * 1.7f) + 0.06f * std::sin(t * 23.0f + (float)k * 3.1f);
}


// Llama verde: nucleo claro y dos halos.
static void LlamaS(Vector3 p, float escala, float brillo)
{
    HaloS(p, 1.7f * escala * brillo, Color{ 60, 200, 120, 34 }, Color{ 60, 200, 120, 0 }, 14);
    HaloS(p, 0.55f * escala * brillo, Color{ 110, 255, 170, 130 }, Color{ 110, 255, 170, 0 }, 12);
    HaloS({ p.x, p.y + 0.03f * escala, p.z }, 0.16f * escala, Color{ 236, 255, 240, 255 }, Color{ 140, 255, 190, 120 }, 8);
}


static void DibujarLucesS(
    const MinijuegoSenderoInvisible& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    bool decoracion,
    bool efectos
)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    g_sinBrumaS = true;
    LoteS();

    // Faroles de la escena: llama, halo y charco en el suelo (en calidad
    // baja queda solo la llama).
    for (int k = 0; k < g_cantidadFarolesS; k++)
    {
        const FarolEscenaS& f = g_farolesS[k];
        float b = ParpadeoS(t, k);

        if (!efectos)
        {
            HaloS(f.llama, 0.18f, Color{ 236, 255, 240, 255 }, Color{ 140, 255, 190, 120 }, 8);
            continue;
        }

        LlamaS(f.llama, 1.0f, b);
        float yS = Y_TERRENO_S + 0.03f;
        DiscoS({ f.llama.x, yS, f.llama.z }, 2.6f * b, 2.2f * b, 18, Color{ 50, 170, 100, 30 }, Color{ 50, 170, 100, 0 }, false);
    }

    // Brillo de las calabazas talladas.
    for (int k = 0; efectos && k < g_cantidadCalabazasS; k++)
    {
        float b = ParpadeoS(t * 0.7f, 70 + k);
        HaloS(g_calabazasS[k], 0.9f * b, Color{ 255, 170, 60, 70 }, Color{ 255, 150, 50, 0 }, 12);
        DiscoS({ g_calabazasS[k].x, Y_TERRENO_S + 0.03f, g_calabazasS[k].z }, 1.2f, 1.0f, 12, Color{ 255, 150, 60, 34 }, Color{ 255, 150, 60, 0 }, false);
    }

    if (m.partidaValida)
    {
        // Losas iluminadas por el fuego fatuo o por el farol.
        for (int carril = 0; carril < m.cantidadCarriles; carril++)
        {
            for (int k = 0; k < LOSAS_SENDERO; k++)
            {
                if (!LosaVisibleS(m, carril, k)) continue;

                float luz = LuzLosaS(m, carril, k);

                if (luz <= 0.01f) continue;

                const BloquePrueba& bloque = m.bloques[carril][k];
                float x = bloque.posicion.x, z = bloque.posicion.z, y = 0.012f;
                float h = MITAD_LOSA_S - 0.06f, hi = MITAD_LOSA_S - 0.3f;
                Color c = AlfaS(COLOR_LLAMA_S, 0.55f * luz);
                QuadColoresS({ x - h, y, z - h }, { x - h, y, z + h }, { x + h, y, z + h }, { x + h, y, z - h }, c, c, c, c);
                Color ci = AlfaS(Color{ 220, 255, 230, 255 }, 0.35f * luz);
                QuadColoresS({ x - hi, y, z - hi }, { x - hi, y, z + hi }, { x + hi, y, z + hi }, { x + hi, y, z - hi }, ci, ci, ci, ci);
                DiscoS({ x, y, z }, 1.3f, 1.3f, 14, AlfaS(COLOR_LLAMA_S, 0.12f * luz), AlfaS(COLOR_LLAMA_S, 0.0f), false);
            }
        }

        // Grietas descubiertas: brillo violeta que late (destello al aparecer).
        for (int k = 0; k < LOSAS_SENDERO; k++)
        {
            int fila = k / COLUMNAS_SENDERO;

            if (!m.grieta[k] || fila == 0 || fila == FILAS_SENDERO - 1) continue;

            float edad = EdadGrietaS(m, k);
            float destello = edad >= 0.0f && edad < 1.6f ? 1.0f - edad / 1.6f : 0.0f;
            float pulso = 0.55f + 0.2f * std::sin(t * 3.0f + (float)k);

            for (int carril = 0; carril < m.cantidadCarriles; carril++)
            {
                if (!LosaVisibleS(m, carril, k)) continue;

                const BloquePrueba& bloque = m.bloques[carril][k];
                float x = bloque.posicion.x, z = bloque.posicion.z;
                float hl = MITAD_LOSA_S - 0.08f;
                Color velo = AlfaS(COLOR_GRIETA_S, 0.1f * pulso + 0.3f * destello);
                QuadColoresS({ x - hl, 0.012f, z - hl }, { x - hl, 0.012f, z + hl }, { x + hl, 0.012f, z + hl }, { x + hl, 0.012f, z - hl }, velo, velo, velo, velo);
                FormaGrietaS(x, 0.014f, z, k, 0.36f, AlfaS(COLOR_GRIETA_S, 0.3f * pulso + 0.5f * destello));
                FormaGrietaS(x, 0.016f, z, k, 0.08f, AlfaS(Color{ 240, 200, 255, 255 }, 0.55f * pulso + 0.45f * destello));

                if (destello > 0.0f)
                {
                    float r = 0.6f + 1.6f * (1.0f - destello);
                    AnilloS({ x, 0.02f, z }, r * 0.8f, r, 20, AlfaS(COLOR_GRIETA_S, 0.6f * destello), AlfaS(COLOR_GRIETA_S, 0.0f));
                }
            }
        }

        // Fuego fatuo que recorre la ruta segura con su estela.
        if (m.fase == FASE_SENDERO_MEMORIZAR && m.cantidadSecuencia > 1)
        {
            float avance = AvanceFuegoFatuoS(m);

            for (int carril = 0; carril < m.cantidadCarriles; carril++)
            {
                for (int s = 7; s >= 1; s--)
                {
                    Vector3 p = PosicionFuegoFatuoS(m, carril, avance - (float)s * 0.12f);
                    p.y += 0.1f * std::sin(t * 9.0f - (float)s);
                    float a = 1.0f - (float)s / 8.0f;
                    HaloS(p, 0.12f + 0.2f * a, AlfaS(COLOR_LLAMA_S, 0.5f * a), AlfaS(COLOR_LLAMA_S, 0.0f), 10);
                }

                Vector3 p = PosicionFuegoFatuoS(m, carril, avance);
                p.y += 0.12f * std::sin(t * 9.0f);
                HaloS(p, 1.2f, Color{ 80, 230, 140, 60 }, Color{ 80, 230, 140, 0 }, 16);
                HaloS(p, 0.5f, Color{ 140, 255, 190, 170 }, Color{ 140, 255, 190, 0 }, 14);
                HaloS(p, 0.2f, Color{ 245, 255, 245, 255 }, Color{ 180, 255, 210, 160 }, 10);
                DiscoS({ p.x, 0.02f, p.z }, 1.1f, 1.1f, 16, Color{ 90, 240, 150, 60 }, Color{ 90, 240, 150, 0 }, false);
            }
        }

        // Farol del tramo medio (por recoger) y farol en mano.
        for (int carril = 0; carril < m.cantidadCarriles; carril++)
        {
            int jugador = m.jugadorDeCarril[carril];

            if (FarolRutaVisibleS(m, jugador))
            {
                Vector3 p = PosicionFarolRutaS(m, carril, t);
                LlamaS(p, 0.75f, ParpadeoS(t, 40 + carril));
                DiscoS({ p.x, 0.02f, p.z }, 1.2f, 1.2f, 16, Color{ 90, 240, 150, 50 }, Color{ 90, 240, 150, 0 }, false);
            }

            if (jugador >= 0 && m.estadosJugadores[jugador].farolTiempo > 0.0f && !jugadores[jugador].cayendo)
            {
                Vector3 c = jugadores[jugador].posicion;
                Vector3 p = { c.x + 0.45f, c.y + 1.25f + 0.06f * std::sin(t * 4.0f), c.z };
                float apaga = Lim01S(m.estadosJugadores[jugador].farolTiempo / 0.6f);
                LlamaS(p, 0.6f * apaga, ParpadeoS(t, 50 + carril));
                DiscoS({ c.x, 0.02f, c.z }, 2.4f, 2.4f, 18, AlfaS(Color{ 90, 240, 150, 255 }, 0.16f * apaga), AlfaS(Color{ 90, 240, 150, 255 }, 0.0f), false);
            }
        }

        // Destello del color del jugador al pisar un tramo nuevo de la ruta.
        for (int i = 0; i < limite; i++)
        {
            float d = g_seguimientoS.destello[i];

            if (d <= 0.0f || m.estadosJugadores[i].carril < 0) continue;

            float u = 1.0f - d / 0.6f;
            Color c = participantes[i].color;
            Vector3 p = g_seguimientoS.lugar[i];
            AnilloS(p, 0.3f + 0.9f * u, 0.5f + 1.1f * u, 20, AlfaS(c, 0.8f * (1.0f - u)), AlfaS(c, 0.0f));
            HaloS({ p.x, 0.5f + 0.6f * u, p.z }, 0.5f, AlfaS(c, 0.5f * (1.0f - u)), AlfaS(c, 0.0f), 10);
        }

        // Anillo del color de cada jugador bajo sus pies.
        for (int i = 0; i < limite; i++)
        {
            if (m.estadosJugadores[i].carril < 0 || jugadores[i].cayendo) continue;

            Vector3 p = jugadores[i].posicion;
            float altura = p.y - ALTURA_CENTRO_JUGADOR;

            if (altura < -0.3f) continue;

            float a = Lim01S(1.0f - altura / 2.0f);
            Color c = participantes[i].color;
            AnilloS({ p.x, 0.02f, p.z }, 0.42f, 0.6f, 20, AlfaS(c, 0.75f * a), AlfaS(c, 0.1f * a));
        }
    }

    // Luciernagas verdes sobre la fosa y el camposanto.
    int motas = efectos ? (int)((decoracion ? 46.0f : 18.0f) * FactorCalidadGrafica(CalidadParticulas()) + 0.5f) : 0;
    float b = BordeFosaXS(m);

    for (int k = 0; k < motas; k++)
    {
        float x = (Ruido01S(k, 401) * 2.0f - 1.0f) * (b + 10.0f) + 1.2f * std::sin(t * (0.3f + 0.3f * Ruido01S(k, 402)) + (float)k);
        float z = Z_BORDE_FONDO_S - 6.0f + 20.0f * Ruido01S(k, 403) + 0.8f * std::cos(t * 0.4f + (float)k * 1.3f);
        float y = -0.6f + 2.6f * Ruido01S(k, 404) + 0.4f * std::sin(t * 0.8f + (float)k * 2.3f);
        float brillo = 0.5f + 0.5f * std::sin(t * (2.0f + Ruido01S(k, 405) * 2.0f) + (float)k * 4.0f);
        brillo *= brillo;
        Vector3 p = { x, y, z };
        HaloS(p, 0.22f, AlfaS(COLOR_LLAMA_S, 0.35f * brillo), AlfaS(COLOR_LLAMA_S, 0.0f), 8);
        PuntoS(p, 0.04f, AlfaS(Color{ 220, 255, 230, 255 }, 0.9f * brillo));
    }

    FinLoteS();
    g_sinBrumaS = false;
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Anuncios, etiquetas y HUD comun
//--------------------------------------------------

static void DibujarAnunciosS(
    const MinijuegoSenderoInvisible& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    float t = m.tiempoAnimacion;
    float w = (float)GetScreenWidth();
    float h = (float)GetScreenHeight();
    float mov = ObtenerFactorMovimientoCamaraMinijuegos();

    if (m.fase == FASE_SENDERO_MEMORIZAR)
    {
        float u = 1.0f - m.tiempoMemorizar / DURACION_MEMORIZAR_SENDERO;
        float escala = u < 0.12f ? 0.6f + 0.4f * (u / 0.12f) : 1.0f + 0.03f * std::sin(t * 6.0f) * mov;
        DibujarTextoMundoContornoUI("¡MEMORIZÁ EL SENDERO!", w * 0.5f, h * 0.25f, (int)((float)UI_TEXTO_GRANDE * escala), COLOR_LLAMA_S);
        DibujarTextoMundoContornoUI("SEGUÍ AL FUEGO FATUO", w * 0.5f, h * 0.25f + 34.0f * EscalaUI(), UI_TEXTO, Color{ 236, 244, 255, 255 });
    }

    // «¡GRIETA!» sobre la losa que se desmorono, en el sendero de quien cayo.
    for (int k = 0; k < LOSAS_SENDERO; k++)
    {
        if (!m.grieta[k]) continue;

        int carril = -1;
        float edad = EdadGrietaS(m, k, &carril);

        if (edad < 0.0f || edad > 1.3f || carril < 0) continue;

        const BloquePrueba& bloque = m.bloques[carril][k];
        Vector2 q = GetWorldToScreen({ bloque.posicion.x, 1.4f + 0.8f * edad * mov, bloque.posicion.z }, vista);
        float alfa = edad > 0.9f ? (1.3f - edad) / 0.4f : 1.0f;
        DibujarTextoMundoContornoUI("¡GRIETA!", q.x, q.y, UI_TEXTO, COLOR_GRIETA_S, alfa);
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorSendero& estado = m.estadosJugadores[i];

        if (estado.carril < 0 || !m.resultado.participantes[i].participo) continue;

        // «¡FAROL!» al recogerlo.
        if (estado.farolTiempo > DURACION_FAROL - 1.2f)
        {
            float edad = DURACION_FAROL - estado.farolTiempo;
            Vector3 p = jugadores[i].posicion;
            Vector2 q = GetWorldToScreen({ p.x, p.y + 3.0f + 0.5f * edad * mov, p.z }, vista);
            float alfa = edad > 0.8f ? (1.2f - edad) / 0.4f : 1.0f;
            DibujarTextoMundoContornoUI("¡FAROL!", q.x, q.y, UI_TEXTO, COLOR_LLAMA_S, alfa);
        }

        if (jugadores[i].cayendo) continue;

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.55f;
        Vector2 q = GetWorldToScreen(cabeza, vista);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        DibujarEtiquetaUI(TextFormat("J%d", numero), q, 0.0f, opciones);
    }
}


static void PrepararHudS(
    const MinijuegoSenderoInvisible& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    DatosHudMinijuego& hud
)
{
    PrepararDatosHudMinijuego(
        hud,
        "SENDERO INVISIBLE",
        "MEMORIZÁ LA RUTA DEL FUEGO FATUO Y CRUZÁ EL CEMENTERIO. CADA CAÍDA DEJA UNA GRIETA EN ESA LOSA PARA TODOS.",
        m.resultado
    );

    // La memorizacion sigue siendo parte de la espera: el reloj no corre
    // y el «¡YA!» del HUD llega recien cuando se puede caminar.
    bool espera = m.fase == FASE_SENDERO_PREPARACION || m.fase == FASE_SENDERO_MEMORIZAR;
    hud.fase = espera
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_SENDERO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.fase == FASE_SENDERO_PREPARACION ? m.tiempoPreparacion : 0.001f;
    // Memorizando: sin el numero grande del centro, que tapaba el recorrido.
    hud.ocultarNumeroCuenta = m.fase == FASE_SENDERO_MEMORIZAR;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_CARRERA_SENDERO;
    hud.segundosAlerta = 10.0f;

    bool alguno = false;
    int total = m.cantidadRuta + 1;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const EstadoJugadorSendero& estado = m.estadosJugadores[i];
        const Participante& participante = participantes[i];
        JugadorHudMinijuego& jugador = hud.jugadores[i];

        if (estado.llego) alguno = true;

        jugador.visible = true;
        jugador.valorNumerico = estado.progreso;
        jugador.progreso = Lim01S((float)estado.progreso / (float)(total > 0 ? total : 1));
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", estado.progreso, total);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), estado.llego ? "¡LLEGÓ!" : "TRAMO %d/%d", estado.progreso, total);
        std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), estado.caidas == 1 ? "1 CAÍDA" : "%d CAÍDAS", estado.caidas);

        if (estado.llego)
            std::snprintf(jugador.estado, sizeof(jugador.estado), "¡LLEGÓ!");
        else if (jugadores[i].cayendo)
            std::snprintf(jugador.estado, sizeof(jugador.estado), "¡CAYÓ!");
        else if (estado.farolTiempo > 0.0f)
            std::snprintf(jugador.estado, sizeof(jugador.estado), "¡FAROL!");
        else if (estado.caidas > 0)
            std::snprintf(jugador.estado, sizeof(jugador.estado), estado.caidas == 1 ? "1 CAÍDA" : "%d CAÍDAS", estado.caidas);

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MOVÉ: %s\nSALTÁ: %s",
                TextoMoverHud(participante),
                ObtenerTextoBotonPrincipal(participante)
            );
        }
    }

    hud.finPorTiempo = !alguno;
}


//--------------------------------------------------
// Escena completa
//--------------------------------------------------

// Lapida con su montículo: losa redondeada, cruz, obelisco, cruz celta o
// lapida doble. Las viejas se ladean.
static void LapidaS(float x, float z, float giro, int tipo, int semilla, float escala = 1.0f)
{
    float s = escala;
    float y = Y_TERRENO_S;
    float ladeo = (Ruido01S(semilla, 501) - 0.5f) * 16.0f;
    float a = giro * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };
    float tono = Ruido01S(semilla, 502);
    Color piedra = tono < 0.3f ? Color{ 150, 152, 166, 255 } : (tono < 0.75f ? Color{ 118, 120, 136, 255 } : Color{ 120, 130, 120, 255 });
    piedra = VariarS(piedra, semilla, 503, 0.06f);
    auto P = [&](float lx, float ly, float lz) { return Vector3{ x + ux.x * lx + uz.x * lz, y + ly, z + ux.z * lx + uz.z * lz }; };

    // Montículo de tierra con pasto delante de la lapida.
    if (Ruido01S(semilla, 504) < 0.55f)
    {
        BolaS(P(0.0f, -0.05f, 0.75f * s), { 0.5f * s, 0.16f * s, 0.8f * s }, MezclaS(COLOR_TIERRA_S, COLOR_PASTO_S, 0.45f), 2, 7, 0.0f);
    }

    CajaGiradaS(P(0.0f, 0.06f * s, 0.0f), { 0.86f * s, 0.12f * s, 0.42f * s }, giro, TonoS(piedra, 0.85f));

    switch (tipo)
    {
        case 0:
        {
            Vector3 c = P(0.0f, 0.12f * s + 0.48f * s, 0.0f);
            CajaGiradaS(c, { 0.7f * s, 0.96f * s, 0.2f * s }, giro, piedra, ladeo);
            float l = ladeo * DEG2RAD;
            Vector3 arriba = { -uz.x * std::sin(l), std::cos(l), -uz.z * std::sin(l) };
            Vector3 tope = Vector3Add(c, Vector3Scale(arriba, 0.48f * s));
            Vector3 ancho = Vector3Scale(uz, 0.1f * s);
            Vector3 lado = Vector3Scale(ux, 0.35f * s);
            const int pasos = 6;

            for (int k = 0; k < pasos; k++)
            {
                float a0 = PI * (float)k / (float)pasos, a1 = PI * (float)(k + 1) / (float)pasos;
                Vector3 p0 = Vector3Add(tope, Vector3Add(Vector3Scale(lado, std::cos(a0)), Vector3Scale(arriba, std::sin(a0) * 0.32f * s)));
                Vector3 p1 = Vector3Add(tope, Vector3Add(Vector3Scale(lado, std::cos(a1)), Vector3Scale(arriba, std::sin(a1) * 0.32f * s)));
                Vector3 n = Vector3Normalize(Vector3Add(Vector3Scale(ux, std::cos(0.5f * (a0 + a1))), Vector3Scale(arriba, std::sin(0.5f * (a0 + a1)))));
                QuadS(Vector3Subtract(p0, ancho), Vector3Subtract(p1, ancho), Vector3Add(p1, ancho), Vector3Add(p0, ancho), CaraS(piedra, n));
                TriS(Vector3Add(tope, ancho), Vector3Add(p0, ancho), Vector3Add(p1, ancho), CaraS(piedra, uz));
            }

            // Placa con inscripcion (lineas oscuras).
            Vector3 frente = Vector3Add(c, Vector3Scale(uz, 0.105f * s));
            for (int k = 0; k < 3; k++)
            {
                Vector3 q = Vector3Add(frente, Vector3Scale(arriba, (0.15f - 0.13f * (float)k) * s));
                Vector3 dx = Vector3Scale(ux, (k == 0 ? 0.2f : 0.25f) * s), dy = Vector3Scale(arriba, 0.025f * s);
                QuadS(Vector3Subtract(Vector3Subtract(q, dx), dy), Vector3Subtract(Vector3Add(q, dx), dy), Vector3Add(Vector3Add(q, dx), dy), Vector3Add(Vector3Subtract(q, dx), dy), TonoS(piedra, 0.6f));
            }
            break;
        }

        case 1:
        case 3:
        {
            CajaGiradaS(P(0.0f, 0.12f * s + 0.7f * s, 0.0f), { 0.18f * s, 1.4f * s, 0.18f * s }, giro, piedra, ladeo);
            float l = ladeo * DEG2RAD;
            Vector3 arriba = { -uz.x * std::sin(l), std::cos(l), -uz.z * std::sin(l) };
            Vector3 brazo = Vector3Add(P(0.0f, 0.12f * s, 0.0f), Vector3Scale(arriba, 1.02f * s));
            CajaGiradaS(brazo, { 0.78f * s, 0.17f * s, 0.17f * s }, giro, piedra, ladeo);

            if (tipo == 3)
            {
                // Anillo de la cruz celta.
                Vector3 previo{};
                for (int k = 0; k <= 10; k++)
                {
                    float an = 2.0f * PI * (float)k / 10.0f;
                    Vector3 p = Vector3Add(brazo, Vector3Add(Vector3Scale(ux, std::cos(an) * 0.27f * s), Vector3Scale(arriba, std::sin(an) * 0.27f * s)));
                    if (k > 0) VigaS(previo, p, 0.07f * s, piedra);
                    previo = p;
                }
            }
            break;
        }

        case 2:
        {
            CajaGiradaS(P(0.0f, 0.12f * s + 0.2f * s, 0.0f), { 0.5f * s, 0.4f * s, 0.5f * s }, giro, TonoS(piedra, 0.95f));
            Vector3 b0 = P(0.0f, 0.52f * s, 0.0f), b1 = P(0.0f, 1.9f * s, 0.0f);
            CilS(b0, b1, 0.24f * s, 0.15f * s, 4, piedra);
            CilS(b1, P(0.0f, 2.2f * s, 0.0f), 0.15f * s, 0.0f, 4, TonoS(piedra, 1.05f));
            break;
        }

        default:
        {
            // Lapida doble ancha con remate recto.
            CajaGiradaS(P(0.0f, 0.12f * s + 0.38f * s, 0.0f), { 1.3f * s, 0.76f * s, 0.24f * s }, giro, piedra, ladeo * 0.5f);
            CajaGiradaS(P(0.0f, 0.12f * s + 0.8f * s, 0.0f), { 1.42f * s, 0.12f * s, 0.32f * s }, giro, TonoS(piedra, 1.06f), ladeo * 0.5f);
            CajaGiradaS(P(0.0f, 0.12f * s + 1.0f * s, 0.0f), { 0.12f * s, 0.32f * s, 0.12f * s }, giro, piedra, ladeo * 0.5f);
            CajaGiradaS(P(0.0f, 0.12f * s + 1.06f * s, 0.0f), { 0.32f * s, 0.1f * s, 0.12f * s }, giro, piedra, ladeo * 0.5f);
            break;
        }
    }

    // Musgo al pie.
    if (Ruido01S(semilla, 505) < 0.5f)
    {
        BolaS(P(0.25f * s, 0.12f * s, 0.12f * s), { 0.18f * s, 0.1f * s, 0.12f * s }, Color{ 62, 92, 70, 255 }, 2, 5, 0.0f);
    }
}


// Arbol seco retorcido: tronco curvo y ramas que se abren en garras.
static void RamaS(Vector3 base, Vector3 dir, float largo, float grosor, int nivel, int semilla, Color corteza)
{
    const int tramos = 3;
    Vector3 p = base;
    Vector3 d = Vector3Normalize(dir);

    for (int k = 0; k < tramos; k++)
    {
        float giro = (Ruido01S(semilla + k, 511) - 0.5f) * 0.9f;
        d = Vector3Normalize(Vector3{ d.x + giro, d.y + (nivel == 0 ? 0.1f : -0.08f), d.z + (Ruido01S(semilla + k, 512) - 0.5f) * 0.6f });
        Vector3 q = Vector3Add(p, Vector3Scale(d, largo / (float)tramos));
        float g0 = grosor * (1.0f - 0.25f * (float)k), g1 = grosor * (1.0f - 0.25f * (float)(k + 1));
        CilS(p, q, g0, g1, nivel == 0 ? 6 : 4, corteza, false);
        p = q;

        if (nivel < 2 && k >= 1)
        {
            float lado = Ruido01S(semilla + k, 513) < 0.5f ? -1.0f : 1.0f;
            Vector3 nd = Vector3{ d.x + lado * 0.9f, d.y + 0.25f, d.z + (Ruido01S(semilla + k, 514) - 0.5f) * 0.8f };
            RamaS(p, nd, largo * 0.55f, g1 * 0.8f, nivel + 1, semilla * 3 + k * 17, corteza);
        }
    }

    if (nivel == 2)
    {
        // Ramitas en garra.
        for (int k = 0; k < 2; k++)
        {
            Vector3 nd = { d.x + (k == 0 ? -0.6f : 0.6f), d.y + 0.3f, d.z };
            Vector3 q = Vector3Add(p, Vector3Scale(Vector3Normalize(nd), largo * 0.35f));
            CilS(p, q, grosor * 0.4f, 0.01f, 3, corteza, false);
        }
    }
}


static void ArbolSecoS(float x, float z, float escala, int semilla)
{
    Color corteza = VariarS(Color{ 58, 46, 54, 255 }, semilla, 521, 0.1f);
    Vector3 base = { x, Y_TERRENO_S - 0.1f, z };
    // Raices.
    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.57f + Ruido01S(semilla + k, 522);
        CilS({ x, Y_TERRENO_S + 0.35f * escala, z }, { x + std::cos(a) * 0.9f * escala, Y_TERRENO_S - 0.05f, z + std::sin(a) * 0.9f * escala }, 0.16f * escala, 0.03f, 4, corteza, false);
    }

    Vector3 p = base;
    Vector3 d = { 0.0f, 1.0f, 0.0f };

    for (int k = 0; k < 4; k++)
    {
        d = Vector3Normalize(Vector3{ d.x + (Ruido01S(semilla + k, 523) - 0.5f) * 0.6f, 1.0f, d.z + (Ruido01S(semilla + k, 524) - 0.5f) * 0.4f });
        Vector3 q = Vector3Add(p, Vector3Scale(d, 0.95f * escala));
        CilS(p, q, (0.36f - 0.06f * (float)k) * escala, (0.3f - 0.06f * (float)k) * escala, 6, corteza, false);
        p = q;

        if (k >= 2)
        {
            float lado = k == 2 ? -1.0f : 1.0f;
            RamaS(p, { lado * 1.0f, 0.55f, (Ruido01S(semilla + k, 525) - 0.5f) }, 2.2f * escala, 0.15f * escala, 1, semilla * 7 + k, corteza);
        }
    }

    RamaS(p, { 0.25f, 1.0f, 0.0f }, 1.8f * escala, 0.13f * escala, 1, semilla * 11, corteza);
    RamaS(p, { -0.8f, 0.7f, 0.2f }, 1.6f * escala, 0.11f * escala, 1, semilla * 13, corteza);
}


// Mausoleo con escalinata, columnas, fronton y puerta entreabierta con
// resplandor verde.
static void MausoleoS(float x, float z, float escala, int semilla, bool detalle)
{
    float s = escala;
    float y = Y_TERRENO_S;
    Color piedra = VariarS(Color{ 150, 150, 166, 255 }, semilla, 531, 0.06f);
    Color oscura = TonoS(piedra, 0.75f);

    CajaS({ x, y + 0.15f * s, z }, { 4.6f * s, 0.3f * s, 4.2f * s }, oscura);
    CajaS({ x, y + 0.42f * s, z + 0.1f * s }, { 4.2f * s, 0.24f * s, 3.8f * s }, TonoS(piedra, 0.9f));
    CajaS({ x, y + 0.54f * s + 1.5f * s, z - 0.3f * s }, { 3.6f * s, 3.0f * s, 3.0f * s }, piedra);

    // Columnas del porche.
    for (int k = 0; k < 4; k++)
    {
        float cx = x + (-1.5f + (float)k) * s;
        Vector3 b0 = { cx, y + 0.54f * s, z + 1.55f * s };
        CajaS({ cx, b0.y + 0.08f * s, b0.z }, { 0.42f * s, 0.16f * s, 0.42f * s }, TonoS(piedra, 1.05f));
        CilS({ cx, b0.y + 0.16f * s, b0.z }, { cx, b0.y + 2.75f * s, b0.z }, 0.15f * s, 0.13f * s, detalle ? 8 : 5, TonoS(piedra, 1.08f));
        CajaS({ cx, b0.y + 2.84f * s, b0.z }, { 0.42f * s, 0.18f * s, 0.42f * s }, TonoS(piedra, 1.05f));
    }

    // Entablamento y fronton triangular.
    float ye = y + 0.54f * s + 3.0f * s;
    CajaS({ x, ye + 0.15f * s, z + 0.3f * s }, { 3.9f * s, 0.3f * s, 4.0f * s }, TonoS(piedra, 1.04f));
    Vector3 f0 = { x - 2.0f * s, ye + 0.3f * s, z + 2.3f * s }, f1 = { x + 2.0f * s, ye + 0.3f * s, z + 2.3f * s }, f2 = { x, ye + 1.4f * s, z + 2.3f * s };
    Vector3 b0 = { x - 2.0f * s, ye + 0.3f * s, z - 1.7f * s }, b1 = { x + 2.0f * s, ye + 0.3f * s, z - 1.7f * s }, b2 = { x, ye + 1.4f * s, z - 1.7f * s };
    TriS(f0, f1, f2, CaraS(piedra, { 0.0f, 0.0f, 1.0f }));
    Color techo = Color{ 92, 96, 116, 255 };
    QuadS(f0, f2, b2, b0, CaraS(techo, { -0.7f, 0.7f, 0.0f }));
    QuadS(f2, f1, b1, b2, CaraS(techo, { 0.7f, 0.7f, 0.0f }));
    // Tímpano con un circulo (rosetón).
    CajaS({ x, ye + 0.68f * s, z + 2.33f * s }, { 0.5f * s, 0.5f * s, 0.04f * s }, oscura);
    CilS(f2, { x, ye + 1.9f * s, z + 2.3f * s }, 0.06f * s, 0.06f * s, 4, piedra);
    CajaS({ x, ye + 1.75f * s, z + 2.3f * s }, { 0.34f * s, 0.07f * s, 0.07f * s }, piedra);

    // Puerta entreabierta con resplandor verde.
    float yp = y + 0.54f * s;
    CajaS({ x, yp + 1.0f * s, z + 1.22f * s }, { 1.5f * s, 2.0f * s, 0.06f * s }, Color{ 24, 22, 32, 255 });
    g_emisS = 1.0f;
    QuadS({ x - 0.12f * s, yp + 0.02f, z + 1.26f * s }, { x + 0.18f * s, yp + 0.02f, z + 1.26f * s }, { x + 0.18f * s, yp + 1.85f * s, z + 1.26f * s }, { x - 0.12f * s, yp + 1.85f * s, z + 1.26f * s }, Color{ 110, 230, 150, 255 });
    g_emisS = 0.0f;
    CajaGiradaS({ x - 0.4f * s, yp + 0.95f * s, z + 1.34f * s }, { 0.62f * s, 1.86f * s, 0.07f * s }, -12.0f, Color{ 70, 80, 74, 255 });
    CajaGiradaS({ x + 0.55f * s, yp + 0.95f * s, z + 1.45f * s }, { 0.62f * s, 1.86f * s, 0.07f * s }, 28.0f, Color{ 70, 80, 74, 255 });

    if (!detalle) return;

    // Urnas en las esquinas de la escalinata.
    for (int lado = 0; lado < 2; lado++)
    {
        float ux = x + (lado == 0 ? -2.0f : 2.0f) * s;
        Vector3 b = { ux, y + 0.54f * s, z + 1.9f * s };
        CajaS({ ux, b.y + 0.2f * s, b.z }, { 0.38f * s, 0.4f * s, 0.38f * s }, oscura);
        BolaS({ ux, b.y + 0.62f * s, b.z }, { 0.22f * s, 0.24f * s, 0.22f * s }, piedra, 3, 7);
        CilS({ ux, b.y + 0.8f * s, b.z }, { ux, b.y + 0.95f * s, b.z }, 0.1f * s, 0.16f * s, 6, piedra);
    }
}


// Calabaza con cara tallada (la luz de adentro se dibuja viva).
static void CalabazaS(float x, float z, float r, float giro, int semilla)
{
    float y = Y_TERRENO_S + r * 0.7f;
    Color naranja = VariarS(COLOR_CALABAZA_S, semilla, 541, 0.08f);

    for (int k = 0; k < 6; k++)
    {
        float a = (float)k / 6.0f * 2.0f * PI;
        BolaS({ x + std::cos(a) * r * 0.32f, y, z + std::sin(a) * r * 0.32f }, { r * 0.62f, r * 0.72f, r * 0.62f }, TonoS(naranja, k % 2 == 0 ? 1.0f : 0.88f), 3, 7);
    }

    CilS({ x, y + r * 0.62f, z }, { x + 0.05f, y + r * 0.95f, z }, r * 0.1f, r * 0.07f, 5, Color{ 76, 92, 50, 255 });

    // Cara: ojos triangulares y sonrisa dentada en el frente (emisivos).
    float a = giro * DEG2RAD;
    Vector3 f = { std::sin(a), 0.0f, std::cos(a) }, l = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 c = { x + f.x * r * 0.92f, y + r * 0.05f, z + f.z * r * 0.92f };
    Color luz = Color{ 255, 214, 96, 255 };
    auto Q = [&](float u, float v) { return Vector3{ c.x + l.x * u * r, c.y + v * r, c.z + l.z * u * r }; };
    g_emisS = 1.0f;
    TriS(Q(-0.42f, 0.12f), Q(-0.14f, 0.12f), Q(-0.28f, 0.38f), luz);
    TriS(Q(0.14f, 0.12f), Q(0.42f, 0.12f), Q(0.28f, 0.38f), luz);
    QuadS(Q(-0.45f, -0.18f), Q(0.45f, -0.18f), Q(0.3f, -0.38f), Q(-0.3f, -0.38f), luz);
    g_emisS = 0.0f;
    TriS(Q(-0.12f, -0.18f), Q(0.0f, -0.18f), Q(-0.06f, -0.27f), naranja);
    TriS(Q(0.12f, -0.38f), Q(0.24f, -0.38f), Q(0.18f, -0.29f), naranja);
}


static void AgregarCalabazaS(float x, float z, float r, float giro, int semilla)
{
    CalabazaS(x, z, r, giro, semilla);

    if (g_cantidadCalabazasS < MAX_CALABAZAS_S)
    {
        float a = giro * DEG2RAD;
        g_calabazasS[g_cantidadCalabazasS++] = { x + std::sin(a) * r, Y_TERRENO_S + r * 0.7f, z + std::cos(a) * r };
    }
}


// Ataud viejo apoyado contra una lapida y pala del sepulturero.
static void AtaudS(float x, float z, float giro)
{
    Color madera = Color{ 86, 62, 56, 255 };
    CajaGiradaS({ x, Y_TERRENO_S + 0.2f, z }, { 0.8f, 0.4f, 2.0f }, giro, madera);
    CajaGiradaS({ x, Y_TERRENO_S + 0.43f, z }, { 0.9f, 0.08f, 2.1f }, giro + 6.0f, TonoS(madera, 1.1f));
    CajaGiradaS({ x, Y_TERRENO_S + 0.48f, z - 0.2f }, { 0.12f, 0.02f, 0.7f }, giro + 6.0f, Color{ 190, 170, 120, 255 });
    CajaGiradaS({ x, Y_TERRENO_S + 0.48f, z - 0.35f }, { 0.4f, 0.02f, 0.12f }, giro + 6.0f, Color{ 190, 170, 120, 255 });
    // Pala clavada al lado.
    Vector3 p = { x + 0.9f, Y_TERRENO_S, z + 0.3f };
    VigaS({ p.x, p.y + 0.3f, p.z }, { p.x + 0.15f, p.y + 1.5f, p.z - 0.1f }, 0.06f, Color{ 120, 92, 70, 255 });
    CajaGiradaS({ p.x - 0.02f, p.y + 0.15f, p.z + 0.02f }, { 0.32f, 0.4f, 0.05f }, 10.0f, Color{ 110, 114, 124, 255 });
    // Montón de tierra.
    BolaS({ x - 1.2f, Y_TERRENO_S - 0.05f, z + 0.2f }, { 0.8f, 0.45f, 0.9f }, COLOR_TIERRA_S, 3, 8, 0.0f);
}


// Capilla del fondo con campanario y ventanas encendidas.
static void CapillaS(float x, float z, float s)
{
    float y = Y_TERRENO_S;
    Color piedra = Color{ 112, 108, 128, 255 };
    Color techo = Color{ 62, 58, 82, 255 };

    CajaS({ x, y + 2.0f * s, z }, { 5.0f * s, 4.0f * s, 8.0f * s }, piedra);
    Vector3 a0 = { x - 2.8f * s, y + 4.0f * s, z + 4.2f * s }, a1 = { x + 2.8f * s, y + 4.0f * s, z + 4.2f * s }, a2 = { x, y + 7.0f * s, z + 4.2f * s };
    Vector3 b0 = { x - 2.8f * s, y + 4.0f * s, z - 4.2f * s }, b1 = { x + 2.8f * s, y + 4.0f * s, z - 4.2f * s }, b2 = { x, y + 7.0f * s, z - 4.2f * s };
    TriS({ x - 2.5f * s, y + 4.0f * s, z + 4.0f * s }, { x + 2.5f * s, y + 4.0f * s, z + 4.0f * s }, { x, y + 6.6f * s, z + 4.0f * s }, CaraS(piedra, { 0.0f, 0.0f, 1.0f }));
    QuadS(a0, a2, b2, b0, CaraS(techo, { -0.7f, 0.7f, 0.0f }));
    QuadS(a2, a1, b1, b2, CaraS(techo, { 0.7f, 0.7f, 0.0f }));

    // Campanario con aguja.
    float tx = x, tz = z + 3.0f * s;
    CajaS({ tx, y + 5.5f * s, tz }, { 2.0f * s, 11.0f * s, 2.0f * s }, TonoS(piedra, 1.04f));
    CajaS({ tx, y + 11.2f * s, tz }, { 2.4f * s, 0.4f * s, 2.4f * s }, TonoS(piedra, 1.1f));
    CilS({ tx, y + 11.4f * s, tz }, { tx, y + 16.0f * s, tz }, 1.5f * s, 0.0f, 4, techo);
    VigaS({ tx, y + 16.0f * s, tz }, { tx, y + 17.4f * s, tz }, 0.12f * s, COLOR_HIERRO_S);
    VigaS({ tx - 0.4f * s, y + 16.9f * s, tz }, { tx + 0.4f * s, y + 16.9f * s, tz }, 0.12f * s, COLOR_HIERRO_S);

    // Ventanas ojivales encendidas (calidas: la unica luz humana).
    g_emisS = 1.0f;
    Color vidrio = Color{ 255, 200, 110, 255 };
    for (int k = 0; k < 3; k++)
    {
        float wz = z - 2.5f * s + (float)k * 2.0f * s;
        float wx = x - 2.52f * s;
        QuadS({ wx, y + 1.4f * s, wz - 0.35f * s }, { wx, y + 1.4f * s, wz + 0.35f * s }, { wx, y + 2.8f * s, wz + 0.35f * s }, { wx, y + 2.8f * s, wz - 0.35f * s }, vidrio);
        TriS({ wx, y + 2.8f * s, wz - 0.35f * s }, { wx, y + 2.8f * s, wz + 0.35f * s }, { wx, y + 3.3f * s, wz }, vidrio);
    }
    QuadS({ tx - 0.35f * s, y + 8.4f * s, tz + 1.02f * s }, { tx + 0.35f * s, y + 8.4f * s, tz + 1.02f * s }, { tx + 0.35f * s, y + 9.6f * s, tz + 1.02f * s }, { tx - 0.35f * s, y + 9.6f * s, tz + 1.02f * s }, Color{ 140, 255, 180, 255 });
    QuadS({ tx - 0.45f * s, y + 0.0f, tz + 1.02f * s }, { tx + 0.45f * s, y + 0.0f, tz + 1.02f * s }, { tx + 0.45f * s, y + 1.8f * s, tz + 1.02f * s }, { tx - 0.45f * s, y + 1.8f * s, tz + 1.02f * s }, Color{ 230, 170, 90, 255 });
    g_emisS = 0.0f;
}


// Fin del camposanto: muro bajo y caida al valle.
static void BordeValleS(const MinijuegoSenderoInvisible& m, bool decoracion)
{
    float z = Z_FIN_CAMPOSANTO_S;
    float ancho = 72.0f;

    // Ladera que cae al valle con niebla.
    for (int i = 0; i < 24; i++)
    {
        float x0 = -ancho + 2.0f * ancho * (float)i / 24.0f, x1 = -ancho + 2.0f * ancho * (float)(i + 1) / 24.0f;
        float r0 = Ruido01S(i, 551) * 1.5f, r1 = Ruido01S(i + 1, 551) * 1.5f;
        Quad4S({ x0, Y_TERRENO_S, z }, { x1, Y_TERRENO_S, z }, { x1, -18.0f, z - 14.0f - r1 * 4.0f }, { x0, -18.0f, z - 14.0f - r0 * 4.0f },
            COLOR_PASTO_S, COLOR_PASTO_S, Color{ 40, 44, 70, 255 }, Color{ 40, 44, 70, 255 });
    }

    if (!decoracion) return;

    // Muro bajo de piedra con pilares.
    for (float x = -ancho * 0.75f; x < ancho * 0.75f; x += 1.2f)
    {
        int k = (int)((x + 100.0f) * 3.0f);
        float alto = 0.7f + 0.2f * Ruido01S(k, 552);
        if (Ruido01S(k, 553) < 0.08f) alto *= 0.4f;
        CajaS({ x + 0.6f, Y_TERRENO_S + alto * 0.5f, z + 0.3f }, { 1.16f, alto, 0.5f }, VariarS(Color{ 104, 100, 116, 255 }, k, 554, 0.08f));
    }
    (void)m;
}


// Horizonte: dos planos de colinas con pinos y un castillo lejano.
static void HorizonteS(bool decoracion)
{
    g_lejanoS = true;
    const Color lejos = Color{ 84, 76, 124, 255 };
    const Color cerca = Color{ 46, 44, 80, 255 };

    // Las colinas se ubican por angulo desde la camara final, para que
    // siempre quede una franja de cielo con la luna sobre el camposanto.
    float arriba = 0.0f, borde = 0.0f;
    BandaCieloS(g_ojoS, arriba, borde);
    float banda = borde - arriba;

    for (int plano = 0; plano < 2; plano++)
    {
        float z = plano == 0 ? -190.0f : -110.0f;
        float base = YPorDepresionS(g_ojoS, z, borde + 4.0f);
        Color c = plano == 0 ? lejos : cerca;
        int tramos = 48;
        float ancho = (g_ojoS.z - z) * 1.1f;

        auto Alto = [&](float x)
        {
            float n = RuidoSuaveS(x, (float)plano * 50.0f, plano == 0 ? 60.0f : 34.0f, 561);
            float f = plano == 0 ? 0.16f + 0.26f * n : 0.04f + 0.2f * n;
            return YPorDepresionS(g_ojoS, z, borde - f * banda);
        };

        for (int i = 0; i < tramos; i++)
        {
            float x0 = -ancho + 2.0f * ancho * (float)i / (float)tramos;
            float x1 = -ancho + 2.0f * ancho * (float)(i + 1) / (float)tramos;
            float h0 = Alto(x0), h1 = Alto(x1);
            Quad4S({ x0, base, z }, { x1, base, z }, { x1, h1, z }, { x0, h0, z }, MezclaS(c, COLOR_HORIZONTE_S, 0.35f), MezclaS(c, COLOR_HORIZONTE_S, 0.35f), c, c);

            // Pinos en el plano cercano.
            if (plano == 1 && decoracion)
            {
                for (int p = 0; p < 3; p++)
                {
                    float u = Ruido01S(i * 3 + p, 562);
                    float px = x0 + (x1 - x0) * u;
                    float py = h0 + (h1 - h0) * u;
                    float ph = 2.5f + 3.0f * Ruido01S(i * 3 + p, 563);
                    TriS({ px - ph * 0.3f, py - 0.5f, z + 0.5f }, { px + ph * 0.3f, py - 0.5f, z + 0.5f }, { px, py + ph, z + 0.5f }, TonoS(cerca, 0.85f));
                }
            }
        }
    }

    // Castillo en ruinas sobre una loma lejana, a la derecha (silueta).
    float cz = -188.0f;
    float cx = (g_ojoS.z - cz) * std::tan(24.0f * DEG2RAD);
    float cy = YPorDepresionS(g_ojoS, cz, borde - 0.3f * banda);
    Color sil = TonoS(lejos, 0.86f);
    CajaS({ cx, cy - 2.0f, cz }, { 22.0f, 12.0f, 2.0f }, sil);
    for (int k = 0; k < 3; k++)
    {
        float tx = cx - 11.0f + (float)k * 11.0f;
        float th = 10.0f + 4.0f * (float)(k % 2);
        CajaS({ tx, cy - 4.0f + th * 0.5f, cz + 0.5f }, { 4.0f, th + 8.0f, 2.0f }, sil);
        CilS({ tx, cy + th, cz + 0.5f }, { tx, cy + th + 6.0f, cz + 0.5f }, 2.8f, 0.0f, 4, sil);
    }

    g_lejanoS = false;
}


// Tumbas en filas detras de la verja, a los costados y en la explanada.
static void CamposantoS(const MinijuegoSenderoInvisible& m, bool decoracion, int paso)
{
    float b = BordeFosaXS(m);
    int n = CarrilesVisualesS(m);
    int indice = 0;

    auto EnSendero = [&](float x)
    {
        for (int c = 0; c < n; c++)
        {
            if (std::fabs(x - CentroCarrilS(m, c)) < 2.2f) return true;
        }
        return false;
    };

    // Senderos de grava desde cada portal hacia el fondo.
    for (int c = 0; c < n; c++)
    {
        float cx = CentroCarrilS(m, c);
        for (int k = 0; k < 9; k++)
        {
            float z0 = Z_BORDE_FONDO_S - 1.0f - (float)k * 2.0f, z1 = z0 - 2.0f;
            float o0 = 0.6f * std::sin((float)k * 0.7f + (float)c), o1 = 0.6f * std::sin((float)(k + 1) * 0.7f + (float)c);
            Color grava = VariarS(Color{ 106, 100, 108, 255 }, k + c * 17, 571, 0.06f);
            QuadS({ cx + o0 - 1.0f, Y_TERRENO_S + 0.015f, z0 }, { cx + o0 + 1.0f, Y_TERRENO_S + 0.015f, z0 }, { cx + o1 + 1.0f, Y_TERRENO_S + 0.015f, z1 }, { cx + o1 - 1.0f, Y_TERRENO_S + 0.015f, z1 }, grava);
        }
    }

    // Detras de la verja.
    for (int fila = 0; fila < 6; fila++)
    {
        float z = Z_BORDE_FONDO_S - 3.2f - (float)fila * 2.6f;

        for (float x = -b - 22.0f; x < b + 22.0f; x += 2.3f * (float)paso)
        {
            indice++;
            float jx = x + (Ruido01S(indice, 581) - 0.5f) * 0.8f;
            float jz = z + (Ruido01S(indice, 582) - 0.5f) * 0.6f;

            if (EnSendero(jx) || Ruido01S(indice, 583) < 0.18f) continue;

            int tipo = (int)(Ruido01S(indice, 584) * 5.0f);
            if (!decoracion && tipo > 1) tipo = 0;
            LapidaS(jx, jz, (Ruido01S(indice, 585) - 0.5f) * 14.0f, tipo, indice, 0.9f + 0.25f * Ruido01S(indice, 586));
        }
    }

    // A los costados de la fosa.
    for (int lado = 0; lado < 2; lado++)
    {
        float s = lado == 0 ? -1.0f : 1.0f;

        for (int fila = 0; fila < 4; fila++)
        {
            for (int k = 0; k < 6; k += paso)
            {
                indice++;
                float x = s * (b + 2.6f + (float)fila * 2.6f + (Ruido01S(indice, 591) - 0.5f) * 0.6f);
                float z = Z_BORDE_FRENTE_S - 1.5f - (float)k * 2.6f + (Ruido01S(indice, 592) - 0.5f) * 0.8f;

                if (Ruido01S(indice, 593) < 0.22f) continue;

                int tipo = (int)(Ruido01S(indice, 594) * 5.0f);
                if (!decoracion && tipo > 1) tipo = 0;
                LapidaS(x, z, s * -12.0f + (Ruido01S(indice, 595) - 0.5f) * 20.0f, tipo, indice, 0.9f + 0.2f * Ruido01S(indice, 596));
            }
        }
    }

    // Senderos de grava en la explanada, uno hacia la salida de cada puente.
    for (int c = 0; c < n; c++)
    {
        float cx = CentroCarrilS(m, c);
        for (int k = 0; k < 4; k++)
        {
            float z0 = Z_BORDE_FRENTE_S + 0.5f + (float)k * 2.0f, z1 = z0 + 2.0f;
            float o0 = 0.5f * std::sin((float)k * 0.9f + (float)c * 2.0f), o1 = 0.5f * std::sin((float)(k + 1) * 0.9f + (float)c * 2.0f);
            float a0 = 0.9f + 0.12f * (float)k, a1 = 0.9f + 0.12f * (float)(k + 1);
            Color grava = VariarS(Color{ 70, 68, 80, 255 }, k + c * 23, 572, 0.06f);
            QuadS({ cx + o0 - a0, Y_TERRENO_S + 0.015f, z0 }, { cx + o0 + a0, Y_TERRENO_S + 0.015f, z0 }, { cx + o1 + a1, Y_TERRENO_S + 0.015f, z1 }, { cx + o1 - a1, Y_TERRENO_S + 0.015f, z1 }, grava);
        }
    }

    // En la explanada, entre los senderos y hacia los costados.
    for (int k = 0; k < 16; k += paso)
    {
        indice++;
        float s = k % 2 == 0 ? -1.0f : 1.0f;
        float x = s * (b - 6.0f + 2.6f * (float)(k / 2) + Ruido01S(indice, 597));
        float z = Z_BORDE_FRENTE_S + 3.0f + 3.5f * Ruido01S(indice, 598) + (float)(k / 2) * 0.6f;

        if (EnSendero(x)) continue;

        LapidaS(x, z, s * 18.0f + (Ruido01S(indice, 599) - 0.5f) * 30.0f, k % 5, indice, 0.85f);
    }

    for (int c = 0; c + 1 < n; c++)
    {
        indice++;
        float x = CentroCarrilS(m, c) + 0.5f * SEPARACION_CARRILES + (Ruido01S(indice, 600) - 0.5f) * 1.2f;
        LapidaS(x, Z_BORDE_FRENTE_S + 4.2f + 2.0f * Ruido01S(indice, 601), (Ruido01S(indice, 602) - 0.5f) * 30.0f, (c * 2 + 1) % 5, indice, 0.8f);
    }
}


static void EscenaEstaticaS(const MinijuegoSenderoInvisible& m, bool decoracion, int paso)
{
    float b = BordeFosaXS(m);

    SueloS(m, decoracion);
    FosaS(m);
    BordilloS(m);
    BordeValleS(m, decoracion);
    HorizonteS(decoracion);

    for (int k = 0; k < g_cantidadFarolesS; k++)
    {
        if (g_farolesS[k].tipo != 1) FarolPosteS(g_farolesS[k]);
        else JaulaFarolS(g_farolesS[k].llama, 1.0f);
    }

    for (int c = 0; c < CarrilesVisualesS(m); c++)
    {
        PortalS(m, c, decoracion);
    }

    VerjaS(m, decoracion);
    g_cantidadCalabazasS = 0;

    if (!decoracion) return;

    CamposantoS(m, decoracion, paso);

    // Mausoleos y capilla en media distancia.
    MausoleoS(-b * 0.55f - 3.0f, -22.0f, 1.25f, 1, true);
    MausoleoS(b * 0.62f + 2.0f, -21.0f, 1.1f, 2, true);
    MausoleoS(b + 9.0f, -4.0f, 1.0f, 3, paso == 1);
    MausoleoS(-b - 9.5f, -6.0f, 1.05f, 4, paso == 1);
    CapillaS(-b * 0.15f - 4.0f, -24.5f, 1.0f);

    // Arboles secos que enmarcan.
    ArbolSecoS(-b - 4.0f, 6.0f, 1.5f, 11);
    ArbolSecoS(b + 5.0f, 4.5f, 1.35f, 12);
    ArbolSecoS(-b * 0.3f + 2.0f, -15.0f, 1.3f, 13);
    ArbolSecoS(b * 0.35f, -16.5f, 1.45f, 14);
    ArbolSecoS(-b + 0.5f, Z_BORDE_FRENTE_S + 8.5f, 1.25f, 18);
    ArbolSecoS(b + 1.5f, Z_BORDE_FRENTE_S + 9.5f, 1.15f, 19);

    if (paso == 1)
    {
        ArbolSecoS(-b - 12.0f, -14.0f, 1.6f, 15);
        ArbolSecoS(b + 13.0f, -12.0f, 1.5f, 16);
        ArbolSecoS(-b * 0.8f, -19.0f, 1.2f, 17);
    }

    // Calabazas sonrientes, ataud y pala del sepulturero.
    AgregarCalabazaS(-b + 1.5f, Z_BORDE_FRENTE_S + 1.6f, 0.42f, 15.0f, 1);
    AgregarCalabazaS(-b + 2.4f, Z_BORDE_FRENTE_S + 2.1f, 0.3f, -10.0f, 2);
    AgregarCalabazaS(b - 1.2f, Z_BORDE_FRENTE_S + 1.8f, 0.38f, -20.0f, 3);
    AgregarCalabazaS(b + 2.5f, -8.0f, 0.4f, -60.0f, 4);
    AgregarCalabazaS(-b - 2.4f, -1.0f, 0.36f, 50.0f, 5);
    AtaudS(b + 3.2f, Z_BORDE_FRENTE_S + 3.5f, 70.0f);
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoSenderoInvisible::Dibujar(
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
    ClearBackground(COLOR_FONDO_S);

    Camera3D final = CamaraFinalS(*this);
    Camera3D vista = CamaraVistaS(*this);
    BaseCamaraS baseFinal = BaseDeS(final);
    BaseCamaraS baseVista = BaseDeS(vista);
    g_ojoS = final.position;
    g_distOjoS = DistanciaCamaraS(*this);
    g_derS = baseVista.r;
    g_arribaS = baseVista.u;

    CalcularFarolesS(*this, decoracion);
    ActualizarSeguimientoS(*this, jugadores, limite);
    HornearEscenaS(*this);
    HornearLosasS(*this, participantes);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarMallaS(g_hornoS.escena, g_hornoS.escenaValida);

    if (partidaValida)
    {
        DibujarMallaS(g_hornoS.losas, g_hornoS.losasValida);
        DibujarLosasVivasS(*this, participantes);
    }

    DibujarAnimadoS(*this, jugadores, participantes, limite, decoracion);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].carril < 0) continue;

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorModeloCompartido(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_SENDERO);
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    // Cielo despues de lo opaco: solo pinta lo que queda libre.
    DibujarLunaS(baseFinal);
    DibujarCieloS(vista);
    if (decoracion)
    {
        LoteS();
        MurcielagosS(baseFinal, tiempoAnimacion, 5);
        FinLoteS();
    }
    rlDrawRenderBatchActive();

    rlDisableDepthMask();
    DibujarNubesS(baseFinal, tiempoAnimacion, decoracion);
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    g_sinBrumaS = true;
    DibujarBrilloCieloS(baseFinal, tiempoAnimacion, decoracion);
    g_sinBrumaS = false;
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();

    DibujarNieblaS(*this, decoracion);

    // Las marcas jugables (ruta, grietas, farol) se dibujan en toda calidad;
    // con efectos bajos se omiten halos, charcos y luciernagas.
    DibujarLucesS(*this, jugadores, participantes, limite, decoracion, efectos);

    rlEnableBackfaceCulling();

    if (mostrarDebug && partidaValida)
    {
        for (int c = 0; c < cantidadCarriles; c++)
        {
            for (int k = 0; k < LOSAS_SENDERO; k++)
            {
                if (bloques[c][k].activaColision) DrawBoundingBox(CrearHitboxBloquePrueba(bloques[c][k]), YELLOW);
            }
        }
    }

    EndMode3D();

    DibujarAnunciosS(*this, jugadores, participantes, limite, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    DatosHudMinijuego hud;
    PrepararHudS(*this, jugadores, participantes, limite, hud);

    DibujarHudMinijuego(hud, participantes);
}


//@@ FIN DE LA SECCION VISUAL (ancla de edicion)


const ResultadoMinijuego& MinijuegoSenderoInvisible::ObtenerResultado() const
{
    return resultado;
}


void MinijuegoSenderoInvisible::Descargar()
{
    LiberarMallasSendero();
    LiberarMaterialHorneado(g_hornoS.material, g_hornoS.materialListo);
}
