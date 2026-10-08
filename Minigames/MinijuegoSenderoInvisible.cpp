#include "Minigames/MinijuegoSenderoInvisible.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <cstdio>


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


void MinijuegoSenderoInvisible::Reiniciar(
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
// VISUAL: CEMENTERIO NOCTURNO
//==================================================
//
// MODELO FUTURO: losas de piedra agrietadas (GLB), lapidas y cruces,
// mausoleos, verja de hierro con picas, faroles con llama verde, arboles
// secos, fuegos fatuos con particulas, luna y niebla volumetrica. La
// logica (GenerarTrazado y el estado de las losas) no depende de ellas.

static const Color COLOR_LAPIDA = { 96, 100, 112, 255 };
static const Color COLOR_HIERRO = { 22, 24, 30, 255 };
static const Color COLOR_LLAMA = { 110, 255, 150, 255 };


static void DibujarCielo(const Camera3D& camara)
{
    Vector3 adelante = Vector3Normalize(Vector3Subtract(camara.target, camara.position));
    Vector3 derecha = Vector3Normalize(Vector3CrossProduct(adelante, camara.up));
    Vector3 arriba = Vector3CrossProduct(derecha, adelante);

    // Estrellas repartidas en la parte alta de la vista.
    for (int k = 0; k < 50; k++)
    {
        float u = ((float)HashIndice(k) / 255.0f - 0.5f) * 2.8f;
        float v = ((float)HashIndice(k + 100) / 255.0f) * 1.0f + 0.15f;
        Vector3 direccion = Vector3Normalize(
            Vector3Add(adelante, Vector3Add(Vector3Scale(derecha, u), Vector3Scale(arriba, v)))
        );

        DrawSphere(Vector3Add(camara.position, Vector3Scale(direccion, 160.0f)), 0.35f, Color{ 230, 235, 255, 255 });
    }

    // Luna enorme arriba a la izquierda.
    Vector3 direccionLuna = Vector3Normalize(
        Vector3Add(adelante, Vector3Add(Vector3Scale(derecha, -0.55f), Vector3Scale(arriba, 0.42f)))
    );
    Vector3 luna = Vector3Add(camara.position, Vector3Scale(direccionLuna, 150.0f));

    DrawSphere(luna, 22.0f, Fade(Color{ 150, 170, 230, 255 }, 0.18f));
    DrawSphere(luna, 17.0f, Color{ 238, 238, 214, 255 });
    DrawSphere(Vector3Add(luna, Vector3Scale(adelante, -3.0f)), 3.2f, Color{ 214, 214, 192, 255 });
}


static void DibujarLapida(float x, float z, unsigned int h)
{
    float alto = 1.0f + (float)(h % 5) * 0.12f;
    float giro = ((float)(h % 7) - 3.0f) * 0.04f;

    DrawCube({ x, -0.5f + alto * 0.5f, z }, 0.8f, alto, 0.25f, COLOR_LAPIDA);
    DrawCube({ x + giro, -0.5f + alto + 0.1f, z }, 0.7f, 0.2f, 0.25f, Color{ 108, 112, 124, 255 });

    if (h % 3 == 0)
    {
        DrawCube({ x, -0.5f + alto + 0.55f, z }, 0.14f, 0.8f, 0.14f, COLOR_LAPIDA);
        DrawCube({ x, -0.5f + alto + 0.7f, z }, 0.5f, 0.14f, 0.14f, COLOR_LAPIDA);
    }
}


static void DibujarMausoleo(float x, float z, float escala)
{
    DrawCube({ x, 1.6f * escala - 0.5f, z }, 4.0f * escala, 3.2f * escala, 3.0f * escala, Color{ 88, 92, 104, 255 });
    DrawCylinder({ x, 3.1f * escala - 0.5f, z }, 0.0f, 3.1f * escala, 1.6f * escala, 4, Color{ 70, 76, 92, 255 });
    DrawCube({ x, 1.0f * escala - 0.5f, z + 1.52f * escala }, 1.2f * escala, 2.0f * escala, 0.1f, Color{ 18, 20, 26, 255 });
}


static void DibujarArbolSeco(float x, float z, float escala)
{
    Color tronco = { 44, 36, 40, 255 };

    DrawCylinder({ x, -0.5f, z }, 0.28f * escala, 0.45f * escala, 5.0f * escala, 6, tronco);
    DrawCylinderEx({ x, 3.5f * escala, z }, { x - 1.8f * escala, 5.8f * escala, z }, 0.12f * escala, 0.2f * escala, 5, tronco);
    DrawCylinderEx({ x, 4.2f * escala, z }, { x + 1.6f * escala, 6.4f * escala, z + 0.4f }, 0.1f * escala, 0.18f * escala, 5, tronco);
    DrawCylinderEx({ x, 2.8f * escala, z }, { x + 1.2f * escala, 4.0f * escala, z - 0.5f }, 0.08f * escala, 0.15f * escala, 5, tronco);
}


static void DibujarFarolVerde(float x, float z, float tiempo)
{
    float parpadeo = 0.75f + 0.25f * std::sin(tiempo * 7.0f + x);

    DrawCylinder({ x, -0.5f, z }, 0.06f, 0.1f, 2.6f, 6, COLOR_HIERRO);
    DrawCube({ x, 2.3f, z }, 0.4f, 0.4f, 0.4f, Color{ 30, 34, 40, 255 });
    DrawSphere({ x, 2.3f, z }, 0.2f * parpadeo + 0.05f, COLOR_LLAMA);
    DrawSphere({ x, 2.3f, z }, 0.7f, Fade(COLOR_LLAMA, 0.12f * parpadeo));
}


static void DibujarCementerioDeFondo(const MinijuegoSenderoInvisible& m)
{
    float t = m.tiempoAnimacion;

    // Terraza del camposanto detras de las cuadriculas.
    DrawCube({ 0.0f, -1.0f, -28.0f }, 120.0f, 1.0f, 36.0f, Color{ 22, 28, 34, 255 });
    DrawCube({ 0.0f, -1.0f, -9.6f }, 120.0f, 1.0f, 0.8f, Color{ 38, 44, 52, 255 });

    // Lapidas en filas.
    for (int fila = 0; fila < 4; fila++)
    {
        for (int k = 0; k < 11; k++)
        {
            int clave = fila * 11 + k;
            unsigned int h = HashIndice(clave);

            if (h % 5 == 0)
            {
                continue;
            }

            float x = -30.0f + (float)k * 6.0f + (float)(h % 3);
            float z = -14.0f - (float)fila * 5.0f;

            DibujarLapida(x, z, h);
        }
    }

    DibujarMausoleo(-14.0f, -24.0f, 1.2f);
    DibujarMausoleo(12.0f, -27.0f, 1.0f);
    DibujarMausoleo(30.0f, -22.0f, 1.3f);

    for (int k = 0; k < 6; k++)
    {
        float x = -32.0f + (float)k * 13.0f;

        DibujarArbolSeco(x, -19.0f - (float)(HashIndice(k) % 8), 1.0f + 0.1f * (float)(k % 3));
    }

    // Verja de hierro con picas en el borde.
    for (int k = 0; k < 36; k++)
    {
        float x = -28.0f + (float)k * 1.6f;

        DrawCylinder({ x, -0.5f, -9.4f }, 0.05f, 0.05f, 1.8f, 5, COLOR_HIERRO);
        DrawCylinder({ x, 1.3f, -9.4f }, 0.0f, 0.1f, 0.3f, 5, COLOR_HIERRO);
    }

    DrawCube({ 0.0f, 0.5f, -9.4f }, 57.6f, 0.08f, 0.08f, COLOR_HIERRO);
    DrawCube({ 0.0f, -0.1f, -9.4f }, 57.6f, 0.08f, 0.08f, COLOR_HIERRO);

    // Faroles con llama verde.
    for (int k = 0; k < 7; k++)
    {
        DibujarFarolVerde(-27.0f + (float)k * 9.0f, -9.0f, t + (float)k);
    }
}


static Color ColorBaseLosa(
    const MinijuegoSenderoInvisible& m,
    const Participante participantes[],
    int carril,
    int fila,
    int columna
)
{
    unsigned int h = HashIndice(fila * COLUMNAS_SENDERO + columna + carril * 61);
    unsigned char v = (unsigned char)(70 + h % 16);

    if (fila == 0)
    {
        Color propio = ColorJugador(participantes, m.jugadorDeCarril[carril]);

        return Color
        {
            (unsigned char)((propio.r + v) / 2),
            (unsigned char)((propio.g + v) / 2),
            (unsigned char)((propio.b + v) / 2),
            255
        };
    }

    if (fila == FILAS_SENDERO - 1)
    {
        return Color{ (unsigned char)(v + 40), (unsigned char)(v + 70), (unsigned char)(v + 20), 255 };
    }

    return Color{ v, (unsigned char)(v + 4), (unsigned char)(v + 12), 255 };
}


// Intensidad de luz verde de una losa para un carril (fuegos fatuos o farol).
static float LuzLosa(const MinijuegoSenderoInvisible& m, int carril, int indice)
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

    const EstadoJugadorSendero& estado = m.estadosJugadores[m.jugadorDeCarril[carril]];

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


static void DibujarLosas(
    const MinijuegoSenderoInvisible& m,
    const Participante participantes[]
)
{
    float t = m.tiempoAnimacion;

    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        for (int fila = 0; fila < FILAS_SENDERO; fila++)
        {
            for (int columna = 0; columna < COLUMNAS_SENDERO; columna++)
            {
                int indice = IndiceLosa(fila, columna);
                const LosaSendero& losa = m.losas[carril][indice];

                if (losa.estado == LOSA_SENDERO_CAIDA)
                {
                    continue;
                }

                const BloquePrueba& bloque = m.bloques[carril][indice];
                Vector3 centro = bloque.posicion;
                Color color = ColorBaseLosa(m, participantes, carril, fila, columna);

                if (losa.estado == LOSA_SENDERO_TEMBLANDO)
                {
                    centro.x += 0.05f * std::sin(t * 70.0f);
                    centro.y -= 0.03f;
                    color = Color{ (unsigned char)(color.r + 50), color.g, color.b, 255 };
                }

                DrawCube(centro, bloque.tamano.x, bloque.tamano.y, bloque.tamano.z, color);

                float luz = LuzLosa(m, carril, indice);

                if (luz > 0.0f)
                {
                    DrawCube(
                        { centro.x, 0.012f, centro.z },
                        bloque.tamano.x - 0.1f, 0.02f, bloque.tamano.z - 0.1f,
                        Fade(COLOR_LLAMA, 0.75f * luz)
                    );
                }

                if (m.grieta[indice] && fila > 0 && fila < FILAS_SENDERO - 1)
                {
                    Color grieta = Color{ 200, 110, 255, 255 };
                    float x = centro.x;
                    float z = centro.z;

                    DrawLine3D({ x - 0.5f, 0.03f, z - 0.2f }, { x - 0.1f, 0.03f, z + 0.1f }, grieta);
                    DrawLine3D({ x - 0.1f, 0.03f, z + 0.1f }, { x + 0.15f, 0.03f, z - 0.25f }, grieta);
                    DrawLine3D({ x + 0.15f, 0.03f, z - 0.25f }, { x + 0.5f, 0.03f, z + 0.15f }, grieta);
                    DrawLine3D({ x - 0.1f, 0.03f, z + 0.1f }, { x - 0.2f, 0.03f, z + 0.5f }, grieta);
                    DrawLine3D({ x - 0.5f, 0.035f, z - 0.17f }, { x - 0.1f, 0.035f, z + 0.13f }, grieta);
                }
            }
        }
    }
}


static void DibujarFuegosFatuosYFaroles(const MinijuegoSenderoInvisible& m)
{
    float t = m.tiempoAnimacion;

    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        const EstadoJugadorSendero& estado = m.estadosJugadores[m.jugadorDeCarril[carril]];

        // Fuego fatuo que recorre la ruta durante la memorizacion.
        if (m.fase == FASE_SENDERO_MEMORIZAR && m.cantidadSecuencia > 1)
        {
            float avance =
                (1.0f - m.tiempoMemorizar / DURACION_MEMORIZAR_SENDERO) *
                (float)(m.cantidadSecuencia - 1);
            int a = (int)avance;

            if (a >= m.cantidadSecuencia - 1)
            {
                a = m.cantidadSecuencia - 2;
            }

            float f = avance - (float)a;
            int ia = m.secuencia[a];
            int ib = m.secuencia[a + 1];
            float x = XColumna(m, carril, ia % COLUMNAS_SENDERO) * (1.0f - f) + XColumna(m, carril, ib % COLUMNAS_SENDERO) * f;
            float z = ZFila(ia / COLUMNAS_SENDERO) * (1.0f - f) + ZFila(ib / COLUMNAS_SENDERO) * f;
            float y = 0.9f + 0.15f * std::sin(t * 9.0f);

            DrawSphere({ x, y, z }, 0.22f, COLOR_LLAMA);
            DrawSphere({ x, y, z }, 0.5f, Fade(COLOR_LLAMA, 0.2f));
            DrawSphere({ x, y - 0.3f, z + 0.35f }, 0.12f, Fade(COLOR_LLAMA, 0.5f));
        }

        // Farol recogible del tramo medio.
        if (m.fase != FASE_SENDERO_PREPARACION && !estado.farolRecogido)
        {
            int indice = m.ruta[m.farolIndice];
            float x = XColumna(m, carril, indice % COLUMNAS_SENDERO);
            float z = ZFila(indice / COLUMNAS_SENDERO);
            float y = 0.55f + 0.1f * std::sin(t * 3.0f + (float)carril);

            DrawCube({ x, y, z }, 0.3f, 0.4f, 0.3f, Color{ 30, 34, 40, 255 });
            DrawSphere({ x, y, z }, 0.14f, COLOR_LLAMA);
            DrawSphere({ x, y, z }, 0.42f, Fade(COLOR_LLAMA, 0.16f));
            DrawCube({ x, y + 0.3f, z }, 0.06f, 0.2f, 0.06f, COLOR_HIERRO);
        }

        // Arco de meta de cada cuadricula.
        float cx = XColumna(m, carril, m.columnaMeta);
        float zm = ZFila(FILAS_SENDERO - 1) - 0.4f;
        DrawCube({ cx - 1.6f, 1.4f, zm }, 0.18f, 2.8f, 0.18f, COLOR_HIERRO);
        DrawCube({ cx + 1.6f, 1.4f, zm }, 0.18f, 2.8f, 0.18f, COLOR_HIERRO);
        DrawCube({ cx, 2.8f, zm }, 3.4f, 0.14f, 0.14f, COLOR_HIERRO);
        DrawSphere({ cx - 1.6f, 2.95f, zm }, 0.15f, COLOR_LLAMA);
        DrawSphere({ cx + 1.6f, 2.95f, zm }, 0.15f, COLOR_LLAMA);
    }
}


static void DibujarLosasCayendo(const MinijuegoSenderoInvisible& m)
{
    for (int carril = 0; carril < m.cantidadCarriles; carril++)
    {
        for (int k = 0; k < LOSAS_SENDERO; k++)
        {
            const LosaSendero& losa = m.losas[carril][k];

            if (losa.estado != LOSA_SENDERO_CAIDA || losa.tiempo > 1.4f)
            {
                continue;
            }

            const BloquePrueba& bloque = m.bloques[carril][k];
            float caida = 0.5f * 16.0f * losa.tiempo * losa.tiempo;
            float alfa = Acotar(1.0f - losa.tiempo / 1.4f, 0.0f, 1.0f);

            // Trozos que se despegan y caen al abismo.
            for (int p = 0; p < 4; p++)
            {
                float dx = (p % 2 == 0 ? -1.0f : 1.0f) * (0.2f + 0.5f * losa.tiempo);
                float dz = (p < 2 ? -1.0f : 1.0f) * (0.2f + 0.4f * losa.tiempo);

                DrawCube(
                    { bloque.posicion.x + dx, bloque.posicion.y - caida - (float)p * 0.1f, bloque.posicion.z + dz },
                    0.55f, 0.35f, 0.55f,
                    Fade(Color{ 90, 94, 108, 255 }, alfa)
                );
            }

        }
    }
}


static void DibujarNiebla(const MinijuegoSenderoInvisible& m)
{
    float t = m.tiempoAnimacion;

    DrawCube({ 0.0f, -2.6f, -1.0f }, 90.0f, 0.05f, 40.0f, Fade(Color{ 120, 140, 170, 255 }, 0.30f));
    DrawCube({ 0.0f, -5.0f, -1.0f }, 90.0f, 0.05f, 40.0f, Fade(Color{ 70, 90, 120, 255 }, 0.45f));
    DrawCube({ 0.0f, -8.0f, -1.0f }, 90.0f, 0.05f, 40.0f, Fade(Color{ 30, 40, 60, 255 }, 0.8f));

    for (int k = 0; k < 14; k++)
    {
        float x = -26.0f + (float)k * 4.0f + 2.0f * std::sin(t * 0.4f + (float)k);
        float z = -4.0f + (float)(HashIndice(k) % 12) - 6.0f;

        DrawSphere({ x, -1.6f - 0.3f * (float)(k % 3), z }, 2.6f, Fade(Color{ 150, 170, 200, 255 }, 0.12f));
    }
}


void MinijuegoSenderoInvisible::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    ClearBackground(Color{ 8, 10, 24, 255 });
    BeginMode3D(camara);

    DibujarCielo(camara);
    DibujarCementerioDeFondo(*this);

    if (partidaValida)
    {
        DibujarLosas(*this, participantes);
        DibujarFuegosFatuosYFaroles(*this);
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorSendero& estado = estadosJugadores[i];

        if (estado.carril < 0)
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

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_SENDERO);

    if (partidaValida)
    {
        DibujarLosasCayendo(*this);
    }

    DibujarNiebla(*this);

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    int centro = anchoPantalla / 2;

    // Etiquetas sobre cada cuadricula.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorSendero& estado = estadosJugadores[i];

        if (estado.carril < 0)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen({ XCarril(*this, estado.carril), 0.4f, ZFila(0) + 1.9f }, camara);
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        const char* texto = TextFormat("J%d%s", numero, JugadorEsBot(participantes[i]) ? " BOT" : "");

        DrawText(texto, (int)pantalla.x - MeasureText(texto, 22) / 2, (int)pantalla.y, 22, ColorJugador(participantes, i));

        if (estado.farolTiempo > 0.0f)
        {
            const char* farol = TextFormat("FAROL %.1f", estado.farolTiempo);
            DrawText(farol, (int)pantalla.x - MeasureText(farol, 16) / 2, (int)pantalla.y + 24, 16, COLOR_LLAMA);
        }
    }

    // Marcador: progreso por jugador.
    int filas = 0;

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].carril >= 0) filas++;
    }

    int panelAncho = 420;
    int px = centro - panelAncho / 2;
    DrawRectangle(px, 8, panelAncho, 14 + filas * 24, Fade(BLACK, 0.65f));

    int fila = 0;
    float total = (float)(cantidadRuta + 1);

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorSendero& estado = estadosJugadores[i];

        if (estado.carril < 0)
        {
            continue;
        }

        int y = 14 + fila * 24;
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        float avance = Acotar((float)estado.progreso / total, 0.0f, 1.0f);

        DrawText(TextFormat("J%d", numero), px + 12, y, 18, ColorJugador(participantes, i));
        DrawRectangle(px + 60, y + 2, 260, 16, Fade(DARKGRAY, 0.8f));
        DrawRectangle(px + 60, y + 2, (int)(260.0f * avance), 16, ColorJugador(participantes, i));
        DrawRectangleLines(px + 60, y + 2, 260, 16, RAYWHITE);
        DrawText(TextFormat("CAIDAS %d", estado.caidas), px + 332, y + 2, 16, LIGHTGRAY);
        fila++;
    }

    DrawText(
        TextFormat("TIEMPO %d", (int)std::ceil(tiempoRestante)),
        24, 20, 30,
        tiempoRestante <= 10.0f ? RED : RAYWHITE
    );

    const char* ayuda = "MOVER: WASD / FLECHAS / STICK   SALTAR: ESPACIO / ENTER / A   BUSCA LA RUTA SEGURA: LAS GRIETAS MUESTRAN LOSAS FALSAS";
    DrawRectangle(0, altoPantalla - 38, GetScreenWidth(), 38, Fade(BLACK, 0.78f));
    DrawText(ayuda, centro - MeasureText(ayuda, 16) / 2, altoPantalla - 29, 16, RAYWHITE);

    if (fase == FASE_SENDERO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, centro - MeasureText(texto, 96) / 2, altoPantalla / 2 - 160, 96, GOLD);

        const char* aviso = "PREPARATE: MEMORIZA EL CAMINO DE LOS FUEGOS FATUOS";
        DrawText(aviso, centro - MeasureText(aviso, 22) / 2, altoPantalla / 2 - 50, 22, COLOR_LLAMA);
    }
    else if (fase == FASE_SENDERO_MEMORIZAR)
    {
        const char* texto = TextFormat("MEMORIZA LA RUTA  %.1f", tiempoMemorizar);
        DrawText(texto, centro - MeasureText(texto, 34) / 2, altoPantalla / 2 - 150, 34, COLOR_LLAMA);
    }
    else if (fase == FASE_SENDERO_JUGANDO && tiempoYa > 0.0f)
    {
        DrawText("YA!", centro - MeasureText("YA!", 80) / 2, altoPantalla / 2 - 150, 80, GOLD);
    }
    else if (
        fase == FASE_SENDERO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAlto = 110 + filas * 26;
        int ancho = 480;
        int py = altoPantalla / 2 - panelAlto / 2 - 20;

        DrawRectangle(centro - ancho / 2, py, ancho, panelAlto, Fade(BLACK, 0.9f));

        const char* titulo = empate ? "EMPATE EN EL CEMENTERIO" : "FIN DE LA CARRERA";
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
                const char* texto = TextFormat(
                    "%d.  J%d   %s",
                    posicion,
                    numero,
                    estadosJugadores[i].llego ? "LLEGO A LA META" : TextFormat("RUTA %d / %d", estadosJugadores[i].progreso, cantidadRuta + 1)
                );

                DrawText(texto, centro - ancho / 2 + 40, py + 60 + linea * 26, 20, ColorJugador(participantes, i));
                linea++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            centro - MeasureText(TextoReinicioMinijuego(), 16) / 2,
            py + panelAlto - 28,
            16,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoSenderoInvisible::ObtenerResultado() const
{
    return resultado;
}
