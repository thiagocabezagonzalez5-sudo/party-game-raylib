// Prueba de la logica de Sendero Invisible (sin pantalla):
// - correr en linea recta NO llega a la meta salvo que esa recta sea el
//   sendero (antes las losas falsas tardaban 0.3 s en caer y se cruzaban);
// - caminar justo sobre el borde entre dos columnas no evita la caida;
// - recorrer el sendero real llega a la meta sin caidas;
// - el tope de tiempo termina la partida con empate si nadie avanza.
// Los jugadores son humanos sin control (CONTROL_NINGUNO) y la prueba los
// mueve fijando su impulso cada frame (velocidad = impulso sin entrada).
#include "Minigames/MinijuegoSenderoInvisible.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>

static int errores = 0;

static void Comprobar(bool condicion, const char* formato, ...)
{
    if (!condicion)
    {
        char texto[256];
        va_list argumentos;
        va_start(argumentos, formato);
        std::vsnprintf(texto, sizeof(texto), formato, argumentos);
        va_end(argumentos);
        std::fprintf(stderr, "FALLO: %s\n", texto);
        errores++;
    }
}

static const float DT = 1.0f / 60.0f;
static const float PASO = 1.5f;
static const float VELOCIDAD = 5.0f;

static MinijuegoSenderoInvisible juego;
static JugadorPrueba jugadores[MAX_PARTICIPANTES];
static Participante participantes[MAX_PARTICIPANTES];

static float XCarrilPrueba(int carril)
{
    return ((float)carril - 0.5f * (float)(juego.cantidadCarriles - 1)) * 10.0f;
}

static float XColumnaPrueba(int carril, int columna)
{
    return XCarrilPrueba(carril) + ((float)columna - 2.5f) * PASO;
}

static float ZFilaPrueba(int fila)
{
    return 6.0f - (float)fila * PASO;
}

static void Preparar(unsigned int semilla)
{
    SetRandomSeed(semilla);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i] = Participante{};
        jugadores[i] = JugadorPrueba{};
    }

    for (int i = 0; i < 2; i++)
    {
        participantes[i].activo = true;
        participantes[i].conectado = true;
        participantes[i].esBot = false;
        participantes[i].control = CONTROL_NINGUNO;
        participantes[i].numeroJugador = i + 1;
    }

    juego.Inicializar();
    juego.Reiniciar(jugadores, participantes, MAX_PARTICIPANTES);
}

// Avanza hasta la fase de juego sin mover a nadie.
static void EsperarCarrera()
{
    int f = 0;

    while (juego.fase != FASE_SENDERO_JUGANDO && f < 60 * 10)
    {
        juego.Actualizar(DT, jugadores, MAX_PARTICIPANTES, participantes);
        f++;
    }
}

static void Paso(Vector3 impulsoJugador0)
{
    jugadores[0].empuje = impulsoJugador0;
    juego.Actualizar(DT, jugadores, MAX_PARTICIPANTES, participantes);
}

// La recta de la columna "columna" (filas 1..8) es toda segura?
static bool RectaSegura(int columna)
{
    for (int fila = 1; fila < FILAS_SENDERO - 1; fila++)
    {
        if (!juego.segura[fila * COLUMNAS_SENDERO + columna]) return false;
    }

    return true;
}


static void ProbarCorrerEnLineaRecta(unsigned int semilla, float desplazamientoX, const char* nombre)
{
    Preparar(semilla);
    EsperarCarrera();

    const EstadoJugadorSendero& estado = juego.estadosJugadores[0];
    int carril = estado.carril;

    // Ubicacion de la carrera: desde la salida, a lo largo de x fija.
    float x = XCarrilPrueba(carril) + desplazamientoX;
    jugadores[0].posicion.x = x;

    int columna = (int)std::floor((x - XCarrilPrueba(carril)) / PASO + 3.0f);
    bool rectaSegura = RectaSegura(columna);

    // Corre siempre hacia la meta (-Z) corrigiendo la x tras cada reaparicion.
    for (int f = 0; f < 60 * 30 && juego.fase == FASE_SENDERO_JUGANDO; f++)
    {
        float corregirX = (x - jugadores[0].posicion.x) * 6.0f;
        Paso({ corregirX, 0.0f, -VELOCIDAD });
    }

    if (rectaSegura)
    {
        Comprobar(estado.llego, "%s semilla %u: la recta era el sendero y debia llegar", nombre, semilla);
    }
    else
    {
        Comprobar(!estado.llego, "%s semilla %u: corriendo fuera del sendero NO debe llegar a la meta", nombre, semilla);
        Comprobar(estado.caidas > 0, "%s semilla %u: pisar losas falsas provoca caidas", nombre, semilla);
    }
}


static void ProbarSenderoReal(unsigned int semilla)
{
    Preparar(semilla);
    EsperarCarrera();

    const EstadoJugadorSendero& estado = juego.estadosJugadores[0];
    int carril = estado.carril;

    // Recorre la secuencia oficial (salida, tramos y meta) losa por losa.
    for (int k = 0; k < juego.cantidadSecuencia && juego.fase == FASE_SENDERO_JUGANDO; k++)
    {
        int indice = juego.secuencia[k];
        float objetivoX = XColumnaPrueba(carril, indice % COLUMNAS_SENDERO);
        float objetivoZ = ZFilaPrueba(indice / COLUMNAS_SENDERO);

        for (int f = 0; f < 60 * 3 && juego.fase == FASE_SENDERO_JUGANDO; f++)
        {
            float dx = objetivoX - jugadores[0].posicion.x;
            float dz = objetivoZ - jugadores[0].posicion.z;
            float d = std::sqrt(dx * dx + dz * dz);

            if (d < 0.05f) break;

            float v = d < 0.4f ? d * 8.0f : VELOCIDAD;
            Paso({ dx / d * v, 0.0f, dz / d * v });
        }
    }

    Comprobar(estado.llego, "Semilla %u: siguiendo el sendero real se llega a la meta", semilla);
    Comprobar(estado.caidas == 0, "Semilla %u: el sendero real no tiene losas falsas (%d caidas)", semilla, estado.caidas);
    Comprobar(estado.progreso == juego.cantidadRuta + 1, "Semilla %u: progreso completo", semilla);
    Comprobar(juego.fase == FASE_SENDERO_TERMINADO, "Semilla %u: llegar termina la partida", semilla);
    Comprobar(juego.resultado.participantes[0].posicionFinal == 1 && juego.resultado.participantes[1].posicionFinal == 2,
        "Semilla %u: gana quien llega", semilla);
}


static void ProbarTopeDeTiempo()
{
    Preparar(7);
    EsperarCarrera();

    int f = 0;

    while (juego.fase == FASE_SENDERO_JUGANDO && f < 60 * 70)
    {
        Paso({ 0.0f, 0.0f, 0.0f });
        f++;
    }

    float segundos = f * DT;
    Comprobar(juego.fase == FASE_SENDERO_TERMINADO, "El tope de tiempo termina la partida");
    Comprobar(std::fabs(segundos - 60.0f) < 0.2f, "La carrera dura 60 s (%.2f)", segundos);
    Comprobar(juego.empate, "Sin avances, empate");
}


int main()
{
    SetTraceLogLevel(LOG_WARNING);

    for (unsigned int semilla = 1; semilla <= 20; semilla++)
    {
        // En el eje del carril (borde exacto entre las columnas 2 y 3).
        ProbarCorrerEnLineaRecta(semilla, 0.0f, "Borde entre columnas");
        // Por el centro de una columna.
        ProbarCorrerEnLineaRecta(semilla, -0.75f, "Centro de columna");
        ProbarSenderoReal(semilla);
    }

    ProbarTopeDeTiempo();

    std::printf("Verificacion Sendero Invisible: %d errores\n", errores);
    return errores == 0 ? 0 : 1;
}
