#include "Minigames/MinijuegoDefensaNucleo.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
#include <ctime>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_DEFENSA = 2.8f;
static const float DURACION_PARTIDA_DEFENSA = 60.0f;
static const float DURACION_GOL_ORO_DEFENSA = 15.0f;
static const float DURACION_PAUSA_GOL_DEFENSA = 1.8f;
static const int GOLES_PARA_GANAR_DEFENSA = 5;

// Arena: el eje X une los dos arcos y el eje Z es el ancho.
static const float LIMITE_X_DEFENSA = 11.0f;
static const float LIMITE_Z_DEFENSA = 7.0f;
static const float MITAD_ARCO_DEFENSA = 2.4f;
static const float RADIO_POSTE_DEFENSA = 0.28f;

static const float RADIO_NUCLEO_DEFENSA = 0.45f;
static const float VELOCIDAD_SAQUE_DEFENSA = 7.5f;
static const float VELOCIDAD_MINIMA_NUCLEO_DEFENSA = 5.5f;
static const float VELOCIDAD_MAXIMA_NUCLEO_DEFENSA = 20.0f;
static const float VELOCIDAD_CRUCERO_NUCLEO_DEFENSA = 11.0f;
static const float PASO_MAXIMO_NUCLEO_DEFENSA = 0.18f;

static const float RADIO_JUGADOR_DEFENSA = 0.62f;
static const float VELOCIDAD_JUGADOR_DEFENSA = 6.4f;
static const float MULTIPLICADOR_SOLITARIO_DEFENSA = 1.3f;
static const float ALTURA_JUGADOR_DEFENSA = 0.70f;

static const float DURACION_GOLPE_DEFENSA = 0.22f;
static const float COOLDOWN_GOLPE_DEFENSA = 0.95f;
static const float ALCANCE_EXTRA_GOLPE_DEFENSA = 0.55f;
static const float COOLDOWN_CONTACTO_DEFENSA = 0.14f;

// Obstaculo central que gira: un segmento con grosor.
static const float MITAD_LARGO_OBSTACULO_DEFENSA = 2.3f;
static const float RADIO_OBSTACULO_DEFENSA = 0.30f;

static const float RAIZ_DOS_DEFENSA = 1.41421356f;


//==================================================
// TEMATICAS (datos de gameplay y visuales por tema)
//==================================================

enum TemaDefensaNucleo
{
    TEMA_ESTACION_ESPACIAL = 0,
    TEMA_CENTRAL_ELECTRICA,
    TEMA_CIUDAD_FUTURISTA,
    TEMA_RUINAS_SUBMARINAS,
    TEMA_AZOTEAS,
    CANTIDAD_TEMAS_DEFENSA
};


struct DatosTemaDefensa
{
    const char* nombre;
    Color fondo;
    Color sueloA;
    Color sueloB;
    Color pared;
    Color acento;
    Color vacio;

    // Tamano del corte de las esquinas: define la forma de la arena.
    float chaflan;

    // Velocidad (rad/s) del obstaculo central.
    float giroObstaculo;
};


static const DatosTemaDefensa TEMAS_DEFENSA[CANTIDAD_TEMAS_DEFENSA] =
{
    {
        "ESTACION ESPACIAL",
        Color{ 8, 10, 26, 255 }, Color{ 74, 86, 110, 255 }, Color{ 62, 73, 96, 255 },
        Color{ 170, 182, 200, 255 }, Color{ 90, 220, 255, 255 }, Color{ 10, 12, 28, 255 },
        3.4f, 1.0f
    },
    {
        "CENTRAL ELECTRICA",
        Color{ 58, 52, 40, 255 }, Color{ 96, 98, 92, 255 }, Color{ 84, 86, 80, 255 },
        Color{ 214, 176, 40, 255 }, Color{ 255, 226, 80, 255 }, Color{ 40, 36, 28, 255 },
        1.6f, 1.5f
    },
    {
        "CIUDAD FUTURISTA",
        Color{ 30, 14, 52, 255 }, Color{ 40, 36, 66, 255 }, Color{ 32, 28, 56, 255 },
        Color{ 90, 80, 150, 255 }, Color{ 255, 70, 200, 255 }, Color{ 18, 8, 34, 255 },
        2.4f, 1.2f
    },
    {
        "RUINAS SUBMARINAS",
        Color{ 8, 52, 70, 255 }, Color{ 150, 140, 104, 255 }, Color{ 134, 126, 94, 255 },
        Color{ 96, 132, 128, 255 }, Color{ 120, 255, 214, 255 }, Color{ 6, 36, 50, 255 },
        4.0f, 0.8f
    },
    {
        "AZOTEAS",
        Color{ 124, 178, 224, 255 }, Color{ 128, 128, 130, 255 }, Color{ 112, 112, 116, 255 },
        Color{ 176, 96, 70, 255 }, Color{ 255, 150, 60, 255 }, Color{ 96, 112, 130, 255 },
        1.0f, 1.3f
    }
};


//==================================================
// UTILIDADES
//==================================================

static float LimitarDefensa(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AbsDefensa(float valor)
{
    return valor < 0.0f ? -valor : valor;
}


// Signo hacia el que ataca el equipo: el equipo 0 defiende el arco
// izquierdo (x negativo) y ataca hacia +X.
static float DireccionAtaqueDefensa(int equipo)
{
    return equipo == 0 ? 1.0f : -1.0f;
}


static Color ObtenerColorEquipoDefensa(int equipo)
{
    return equipo == 0
        ? Color{ 238, 55, 66, 255 }
        : Color{ 40, 159, 224, 255 };
}


static float Ruido01Defensa(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static int LimiteJugadoresDefensa(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static bool EsControladoPorBotDefensa(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


//==================================================
// GEOMETRIA DE LA ARENA (GAMEPLAY)
//==================================================

// Empuja un punto con radio hacia dentro del plano diagonal de una esquina
// cortada. Devuelve true si habia penetracion. La normal sale por salidaNx/Nz.
static bool EmpujarDiagonalDefensa(
    float& x,
    float& z,
    float radio,
    float chaflan,
    float signoX,
    float signoZ,
    float& normalX,
    float& normalZ
)
{
    float s = signoX * x + signoZ * z;
    float limite =
        LIMITE_X_DEFENSA + LIMITE_Z_DEFENSA - chaflan -
        radio * RAIZ_DOS_DEFENSA;

    if (s <= limite)
    {
        return false;
    }

    float exceso = (s - limite) * 0.5f;
    x -= signoX * exceso;
    z -= signoZ * exceso;
    normalX = -signoX / RAIZ_DOS_DEFENSA;
    normalZ = -signoZ / RAIZ_DOS_DEFENSA;

    return true;
}


static void ReflejarNucleoDefensa(
    NucleoDefensaNucleo& nucleo,
    float normalX,
    float normalZ
)
{
    float vn = nucleo.velocidadX * normalX + nucleo.velocidadZ * normalZ;

    if (vn < 0.0f)
    {
        nucleo.velocidadX -= 2.0f * vn * normalX;
        nucleo.velocidadZ -= 2.0f * vn * normalZ;
    }
}


static float VelocidadNucleoDefensa(const NucleoDefensaNucleo& nucleo)
{
    return std::sqrt(
        nucleo.velocidadX * nucleo.velocidadX +
        nucleo.velocidadZ * nucleo.velocidadZ
    );
}


static void FijarVelocidadNucleoDefensa(
    NucleoDefensaNucleo& nucleo,
    float velocidad
)
{
    float actual = VelocidadNucleoDefensa(nucleo);

    if (actual < 0.001f)
    {
        nucleo.velocidadX = velocidad;
        nucleo.velocidadZ = 0.0f;
        return;
    }

    float factor = velocidad / actual;
    nucleo.velocidadX *= factor;
    nucleo.velocidadZ *= factor;
}


static void RebotarCirculoFijoDefensa(
    NucleoDefensaNucleo& nucleo,
    float centroX,
    float centroZ,
    float radioSuma
)
{
    float dx = nucleo.x - centroX;
    float dz = nucleo.z - centroZ;
    float distancia = std::sqrt(dx * dx + dz * dz);

    if (distancia >= radioSuma || distancia < 0.0001f)
    {
        return;
    }

    float nx = dx / distancia;
    float nz = dz / distancia;

    nucleo.x = centroX + nx * radioSuma;
    nucleo.z = centroZ + nz * radioSuma;
    ReflejarNucleoDefensa(nucleo, nx, nz);
}


// Punto mas cercano del obstaculo central (segmento giratorio) a (px, pz).
static void PuntoMasCercanoObstaculoDefensa(
    float angulo,
    float px,
    float pz,
    float& cercanoX,
    float& cercanoZ
)
{
    float ex = std::cos(angulo) * MITAD_LARGO_OBSTACULO_DEFENSA;
    float ez = std::sin(angulo) * MITAD_LARGO_OBSTACULO_DEFENSA;

    // Segmento de (-ex,-ez) a (ex,ez): t en [0,1] sobre su longitud.
    float largo2 = 4.0f * (ex * ex + ez * ez);
    float t = ((px + ex) * (2.0f * ex) + (pz + ez) * (2.0f * ez)) / largo2;
    t = LimitarDefensa(t, 0.0f, 1.0f);

    cercanoX = -ex + t * 2.0f * ex;
    cercanoZ = -ez + t * 2.0f * ez;
}


static void RebotarObstaculoDefensa(
    NucleoDefensaNucleo& nucleo,
    float angulo
)
{
    float cx = 0.0f;
    float cz = 0.0f;
    PuntoMasCercanoObstaculoDefensa(angulo, nucleo.x, nucleo.z, cx, cz);
    RebotarCirculoFijoDefensa(
        nucleo,
        cx,
        cz,
        RADIO_NUCLEO_DEFENSA + RADIO_OBSTACULO_DEFENSA
    );
}


// Rebote del nucleo con paredes, esquinas cortadas y postes.
// Devuelve el equipo que anota (0 o 1) o -1 si no hay gol.
static int ResolverEntornoNucleoDefensa(
    NucleoDefensaNucleo& nucleo,
    float chaflan,
    float anguloObstaculo
)
{
    const float r = RADIO_NUCLEO_DEFENSA;

    // Paredes laterales.
    if (nucleo.z > LIMITE_Z_DEFENSA - r)
    {
        nucleo.z = LIMITE_Z_DEFENSA - r;
        ReflejarNucleoDefensa(nucleo, 0.0f, -1.0f);
    }
    else if (nucleo.z < -LIMITE_Z_DEFENSA + r)
    {
        nucleo.z = -LIMITE_Z_DEFENSA + r;
        ReflejarNucleoDefensa(nucleo, 0.0f, 1.0f);
    }

    // Paredes de fondo, con la abertura del arco.
    if (AbsDefensa(nucleo.z) > MITAD_ARCO_DEFENSA)
    {
        if (nucleo.x > LIMITE_X_DEFENSA - r)
        {
            nucleo.x = LIMITE_X_DEFENSA - r;
            ReflejarNucleoDefensa(nucleo, -1.0f, 0.0f);
        }
        else if (nucleo.x < -LIMITE_X_DEFENSA + r)
        {
            nucleo.x = -LIMITE_X_DEFENSA + r;
            ReflejarNucleoDefensa(nucleo, 1.0f, 0.0f);
        }
    }

    // Esquinas cortadas.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            float nx = 0.0f;
            float nz = 0.0f;

            if (
                EmpujarDiagonalDefensa(
                    nucleo.x, nucleo.z, r, chaflan,
                    (float)sx, (float)sz, nx, nz
                )
            )
            {
                ReflejarNucleoDefensa(nucleo, nx, nz);
            }
        }
    }

    // Postes de los arcos.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            RebotarCirculoFijoDefensa(
                nucleo,
                (float)sx * LIMITE_X_DEFENSA,
                (float)sz * MITAD_ARCO_DEFENSA,
                r + RADIO_POSTE_DEFENSA
            );
        }
    }

    RebotarObstaculoDefensa(nucleo, anguloObstaculo);

    // Gol: el centro cruza la linea del arco por la abertura.
    if (AbsDefensa(nucleo.z) <= MITAD_ARCO_DEFENSA)
    {
        if (nucleo.x > LIMITE_X_DEFENSA) return 0;
        if (nucleo.x < -LIMITE_X_DEFENSA) return 1;
    }

    return -1;
}


// Mantiene a un jugador dentro de su mitad, de las esquinas y fuera del obstaculo.
static void LimitarJugadorDefensa(
    EstadoJugadorDefensaNucleo& estado,
    float& x,
    float& z,
    float chaflan,
    float anguloObstaculo
)
{
    float minimoX = estado.equipo == 0 ? -(LIMITE_X_DEFENSA - 1.3f) : 1.0f;
    float maximoX = estado.equipo == 0 ? -1.0f : (LIMITE_X_DEFENSA - 1.3f);

    x = LimitarDefensa(x, minimoX, maximoX);
    z = LimitarDefensa(
        z,
        -(LIMITE_Z_DEFENSA - 0.8f),
        LIMITE_Z_DEFENSA - 0.8f
    );

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            float nx = 0.0f;
            float nz = 0.0f;

            EmpujarDiagonalDefensa(
                x, z, RADIO_JUGADOR_DEFENSA, chaflan,
                (float)sx, (float)sz, nx, nz
            );
        }
    }

    float cx = 0.0f;
    float cz = 0.0f;
    PuntoMasCercanoObstaculoDefensa(anguloObstaculo, x, z, cx, cz);

    float dx = x - cx;
    float dz = z - cz;
    float distancia = std::sqrt(dx * dx + dz * dz);
    float minimo = RADIO_JUGADOR_DEFENSA + RADIO_OBSTACULO_DEFENSA;

    if (distancia < minimo && distancia > 0.0001f)
    {
        x = cx + dx / distancia * minimo;
        z = cz + dz / distancia * minimo;
    }

    // El empuje del obstaculo no debe sacar al jugador de su mitad.
    x = LimitarDefensa(x, minimoX, maximoX);
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarDefensaNucleo(MinijuegoDefensaNucleo& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    minijuego.empate = minijuego.goles[0] == minijuego.goles[1];
    minijuego.equipoGanador = -1;

    if (!minijuego.empate)
    {
        minijuego.equipoGanador =
            minijuego.goles[0] > minijuego.goles[1] ? 0 : 1;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.desenlace =
        minijuego.empate ? DESENLACE_EMPATE : DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = minijuego.estadosJugadores[i].equipo;
        resultadoJugador.numeroEquipo = equipo;
        resultadoJugador.puntuacionMinijuego =
            equipo >= 0 && equipo < 2 ? minijuego.goles[equipo] : 0;
        resultadoJugador.puntosObtenidos = 0;
        resultadoJugador.posicionFinal =
            minijuego.empate || equipo == minijuego.equipoGanador ? 1 : 2;
    }

    minijuego.nucleo.enJuego = false;
    minijuego.fase = FASE_DEFENSA_NUCLEO_TERMINADO;
}


//==================================================
// SAQUE Y NUCLEO
//==================================================

// Saque desde el centro hacia el equipo que recibio el gol.
static void SacarNucleoDefensa(
    MinijuegoDefensaNucleo& minijuego,
    int equipoReceptor
)
{
    if (equipoReceptor < 0)
    {
        equipoReceptor = GetRandomValue(0, 1);
    }

    float direccion = -DireccionAtaqueDefensa(equipoReceptor);
    float desvio = (float)GetRandomValue(-35, 35) / 100.0f;

    NucleoDefensaNucleo& nucleo = minijuego.nucleo;
    nucleo.x = 0.0f;
    nucleo.z = 0.0f;
    nucleo.velocidadX = direccion;
    nucleo.velocidadZ = desvio;
    FijarVelocidadNucleoDefensa(nucleo, VELOCIDAD_SAQUE_DEFENSA);
    nucleo.tiempoCarga = 0.0f;
    nucleo.enJuego = true;

    // Evita nacer dentro del obstaculo central: sale justo al lado del centro,
    // desde el lado del equipo que anoto y hacia quien recibio el gol.
    nucleo.x = -direccion * 3.4f;
}


// Golpe de un jugador al nucleo. La direccion depende del punto de contacto
// (normal jugador -> nucleo) y del movimiento del jugador.
static void ResolverContactoJugadorDefensa(
    MinijuegoDefensaNucleo& minijuego,
    int indice,
    float jugadorX,
    float jugadorZ
)
{
    EstadoJugadorDefensaNucleo& estado = minijuego.estadosJugadores[indice];
    NucleoDefensaNucleo& nucleo = minijuego.nucleo;

    float dx = nucleo.x - jugadorX;
    float dz = nucleo.z - jugadorZ;
    float distancia = std::sqrt(dx * dx + dz * dz);
    float contacto = RADIO_JUGADOR_DEFENSA + RADIO_NUCLEO_DEFENSA;
    bool golpeando = estado.tiempoGolpeActivo > 0.0f;

    float alcance = contacto + (golpeando ? ALCANCE_EXTRA_GOLPE_DEFENSA : 0.0f);

    if (distancia >= alcance)
    {
        return;
    }

    float ataque = DireccionAtaqueDefensa(estado.equipo);
    float nx = distancia > 0.0001f ? dx / distancia : ataque;
    float nz = distancia > 0.0001f ? dz / distancia : 0.0f;

    if (distancia < contacto)
    {
        nucleo.x = jugadorX + nx * contacto;
        nucleo.z = jugadorZ + nz * contacto;
    }

    if (estado.cooldownContacto > 0.0f)
    {
        // Ya golpeo hace un instante: solo evitamos que lo atraviese.
        ReflejarNucleoDefensa(nucleo, nx, nz);
        return;
    }

    float velocidadActual = VelocidadNucleoDefensa(nucleo);

    float dirX = nx + 0.045f * estado.velocidadX + 0.30f * ataque;
    float dirZ = nz + 0.045f * estado.velocidadZ;
    float longitud = std::sqrt(dirX * dirX + dirZ * dirZ);

    if (longitud < 0.0001f)
    {
        dirX = ataque;
        dirZ = 0.0f;
        longitud = 1.0f;
    }

    float nueva = velocidadActual * 1.07f + 0.5f;

    if (golpeando)
    {
        nueva = velocidadActual * 1.35f + 4.0f;
        nucleo.tiempoCarga = 1.2f;
        estado.tiempoGolpeActivo = 0.0f;
    }

    nueva = LimitarDefensa(
        nueva,
        VELOCIDAD_SAQUE_DEFENSA,
        VELOCIDAD_MAXIMA_NUCLEO_DEFENSA
    );

    nucleo.velocidadX = dirX / longitud * nueva;
    nucleo.velocidadZ = dirZ / longitud * nueva;
    estado.cooldownContacto = COOLDOWN_CONTACTO_DEFENSA;
}


// Devuelve el equipo que anota o -1. Usa subpasos para evitar el tunel.
static int MoverNucleoDefensa(
    MinijuegoDefensaNucleo& minijuego,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    NucleoDefensaNucleo& nucleo = minijuego.nucleo;
    const DatosTemaDefensa& tema = TEMAS_DEFENSA[minijuego.indiceTema];

    // Rozamiento suave: el nucleo rapido se calma y el lento no se detiene.
    float velocidad = VelocidadNucleoDefensa(nucleo);

    if (velocidad > VELOCIDAD_CRUCERO_NUCLEO_DEFENSA)
    {
        velocidad -= 0.6f * deltaTime;
    }

    if (velocidad < VELOCIDAD_MINIMA_NUCLEO_DEFENSA)
    {
        velocidad = VELOCIDAD_MINIMA_NUCLEO_DEFENSA;
    }

    // Si va casi vertical, se le da un empujoncito hacia un arco.
    if (AbsDefensa(nucleo.velocidadX) < 0.22f * velocidad)
    {
        float signo = nucleo.velocidadX >= 0.0f ? 1.0f : -1.0f;
        nucleo.velocidadX += signo * velocidad * 0.6f * deltaTime;
    }

    FijarVelocidadNucleoDefensa(nucleo, velocidad);

    int pasos =
        (int)std::ceil(velocidad * deltaTime / PASO_MAXIMO_NUCLEO_DEFENSA);

    if (pasos < 1) pasos = 1;
    if (pasos > 24) pasos = 24;

    float h = deltaTime / (float)pasos;

    for (int paso = 0; paso < pasos; paso++)
    {
        nucleo.x += nucleo.velocidadX * h;
        nucleo.z += nucleo.velocidadZ * h;

        for (int i = 0; i < limite; i++)
        {
            if (minijuego.estadosJugadores[i].equipo < 0)
            {
                continue;
            }

            ResolverContactoJugadorDefensa(
                minijuego,
                i,
                jugadores[i].posicion.x,
                jugadores[i].posicion.z
            );
        }

        int gol = ResolverEntornoNucleoDefensa(
            nucleo,
            tema.chaflan,
            minijuego.anguloObstaculo
        );

        if (gol >= 0)
        {
            return gol;
        }
    }

    return -1;
}


//==================================================
// IA DE BOTS
//==================================================

// Predice la Z del nucleo al llegar a una X, plegando los rebotes laterales.
// Devuelve false si el nucleo no va hacia esa X.
static bool PredecirZEnXDefensa(
    const NucleoDefensaNucleo& nucleo,
    float xObjetivo,
    float& zPrevista
)
{
    if (AbsDefensa(nucleo.velocidadX) < 0.01f)
    {
        return false;
    }

    float t = (xObjetivo - nucleo.x) / nucleo.velocidadX;

    if (t < 0.0f)
    {
        return false;
    }

    float altura = LIMITE_Z_DEFENSA - 0.5f;
    float tramo = 2.0f * altura;
    float u = std::fmod(nucleo.z + nucleo.velocidadZ * t + altura, 2.0f * tramo);

    if (u < 0.0f) u += 2.0f * tramo;
    if (u > tramo) u = 2.0f * tramo - u;

    zPrevista = u - altura;
    return true;
}


static InputMinijuegoParticipante CrearEntradaBotDefensa(
    MinijuegoDefensaNucleo& minijuego,
    int indice,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    float deltaTime
)
{
    EstadoJugadorDefensaNucleo& estado = minijuego.estadosJugadores[indice];
    const NucleoDefensaNucleo& nucleo = minijuego.nucleo;
    const JugadorPrueba& jugador = jugadores[indice];

    float ataque = DireccionAtaqueDefensa(estado.equipo);
    float propioX = -ataque;

    estado.tiempoDecisionBot -= deltaTime;

    if (estado.tiempoDecisionBot <= 0.0f)
    {
        estado.tiempoDecisionBot = (float)GetRandomValue(16, 30) / 100.0f;

        bool entrando = nucleo.velocidadX * ataque < -0.5f;
        bool enMiMitad = nucleo.x * ataque < 0.0f;

        // Puesto base del equipo: orden segun distancia a la Z del nucleo.
        // El bot mas cercano al punto de intercepcion es el interceptor y
        // el otro cubre el lado contrario del arco.
        float objetivoX = propioX * 6.0f;
        float objetivoZ = 0.0f;
        bool hayObjetivo = false;

        float zIntercepcion = 0.0f;

        if (entrando)
        {
            float lineaX = propioX * 5.0f;

            if (nucleo.x * propioX > 5.0f)
            {
                lineaX = nucleo.x;
            }

            if (PredecirZEnXDefensa(nucleo, lineaX, zIntercepcion))
            {
                // Error de prediccion: bots vencibles.
                zIntercepcion += (float)GetRandomValue(-110, 110) / 100.0f;
                objetivoX = lineaX;
                objetivoZ = zIntercepcion;
                hayObjetivo = true;
            }
        }
        else if (enMiMitad)
        {
            objetivoX = nucleo.x - ataque * 0.95f;
            objetivoZ = nucleo.z + (float)GetRandomValue(-40, 40) / 100.0f;
            zIntercepcion = nucleo.z;
            hayObjetivo = true;
        }

        if (hayObjetivo)
        {
            // Cual compañero bot esta mas cerca del punto? Ese intercepta.
            float miDistancia = AbsDefensa(jugador.posicion.z - objetivoZ);
            bool soyInterceptor = true;

            for (int j = 0; j < limite; j++)
            {
                if (
                    j == indice ||
                    minijuego.estadosJugadores[j].equipo != estado.equipo ||
                    !EsControladoPorBotDefensa(participantes[j])
                )
                {
                    continue;
                }

                float suDistancia = AbsDefensa(jugadores[j].posicion.z - objetivoZ);

                if (
                    suDistancia < miDistancia - 0.15f ||
                    (AbsDefensa(suDistancia - miDistancia) <= 0.15f && j < indice)
                )
                {
                    soyInterceptor = false;
                }
            }

            if (!soyInterceptor)
            {
                // Guardia: cubre el lado opuesto, cerca del arco.
                objetivoX = propioX * 7.2f;
                objetivoZ = zIntercepcion > 0.0f ? -1.9f : 1.9f;
            }
        }
        else
        {
            // Reposicion: dos bots se reparten, uno solo va al centro.
            int ordenEnEquipo = 0;

            for (int j = 0; j < indice; j++)
            {
                if (minijuego.estadosJugadores[j].equipo == estado.equipo)
                {
                    ordenEnEquipo++;
                }
            }

            objetivoZ =
                minijuego.cantidadJugadoresEquipo[estado.equipo] <= 1
                ? 0.0f
                : (ordenEnEquipo == 0 ? -3.0f : 3.0f);
        }

        estado.objetivoX = objetivoX;
        estado.objetivoZ = objetivoZ;
        estado.quiereGolpear = GetRandomValue(1, 100) <= 60;
    }

    InputMinijuegoParticipante entrada{};

    float dx = estado.objetivoX - jugador.posicion.x;
    float dz = estado.objetivoZ - jugador.posicion.z;

    if (dx > 0.28f) entrada.derecha = true;
    if (dx < -0.28f) entrada.izquierda = true;
    if (dz > 0.28f) entrada.atras = true;
    if (dz < -0.28f) entrada.adelante = true;

    // Golpe cargado cuando el nucleo esta a tiro y viene hacia el arco propio
    // o esta cerca; reaccion limitada por quiereGolpear.
    float ndx = nucleo.x - jugador.posicion.x;
    float ndz = nucleo.z - jugador.posicion.z;
    float distancia = std::sqrt(ndx * ndx + ndz * ndz);

    if (
        estado.quiereGolpear &&
        estado.cooldownGolpe <= 0.0f &&
        distancia < RADIO_JUGADOR_DEFENSA + RADIO_NUCLEO_DEFENSA + 0.55f &&
        ndx * ataque > -0.3f
    )
    {
        entrada.saltar = true;
        estado.quiereGolpear = false;
    }

    return entrada;
}


//==================================================
// ACTUALIZACION DE JUGADORES
//==================================================

static void ActualizarJugadoresDefensa(
    MinijuegoDefensaNucleo& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[]
)
{
    const DatosTemaDefensa& tema = TEMAS_DEFENSA[minijuego.indiceTema];

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorDefensaNucleo& estado = minijuego.estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];

        if (estado.cooldownGolpe > 0.0f) estado.cooldownGolpe -= deltaTime;
        if (estado.tiempoGolpeActivo > 0.0f) estado.tiempoGolpeActivo -= deltaTime;
        if (estado.cooldownContacto > 0.0f) estado.cooldownContacto -= deltaTime;

        InputMinijuegoParticipante entrada{};

        if (EsControladoPorBotDefensa(participantes[i]))
        {
            entrada = CrearEntradaBotDefensa(
                minijuego,
                i,
                jugadores,
                participantes,
                limite,
                deltaTime
            );
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        float moverX =
            (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
        float moverZ =
            (entrada.atras ? 1.0f : 0.0f) - (entrada.adelante ? 1.0f : 0.0f);
        float longitud = std::sqrt(moverX * moverX + moverZ * moverZ);

        if (longitud > 0.01f)
        {
            moverX /= longitud;
            moverZ /= longitud;
        }

        // El equipo en inferioridad (2 vs 1) gana algo de velocidad.
        float multiplicador = 1.0f;
        int otro = estado.equipo == 0 ? 1 : 0;

        if (
            minijuego.cantidadJugadoresEquipo[estado.equipo] <
            minijuego.cantidadJugadoresEquipo[otro]
        )
        {
            multiplicador = MULTIPLICADOR_SOLITARIO_DEFENSA;
        }

        float velocidad = VELOCIDAD_JUGADOR_DEFENSA * multiplicador;
        estado.velocidadX = moverX * velocidad;
        estado.velocidadZ = moverZ * velocidad;

        float x = jugador.posicion.x + estado.velocidadX * deltaTime;
        float z = jugador.posicion.z + estado.velocidadZ * deltaTime;

        LimitarJugadorDefensa(estado, x, z, tema.chaflan, minijuego.anguloObstaculo);

        jugador.posicion = { x, ALTURA_JUGADOR_DEFENSA, z };
        jugador.velocidad = { estado.velocidadX, 0.0f, estado.velocidadZ };
        jugador.enSuelo = true;
        jugador.cayendo = false;

        if (longitud > 0.01f)
        {
            jugador.direccionMirada = { moverX, 0.0f, moverZ };
        }

        // Golpe cargado: se activa al pulsar el boton principal (flanco).
        bool boton = entrada.saltar;

        if (
            boton &&
            !estado.botonAnterior &&
            estado.cooldownGolpe <= 0.0f
        )
        {
            estado.tiempoGolpeActivo = DURACION_GOLPE_DEFENSA;
            estado.cooldownGolpe = COOLDOWN_GOLPE_DEFENSA;
        }

        estado.botonAnterior = boton;
    }

    // Separacion entre companeros de equipo.
    for (int i = 0; i < limite; i++)
    {
        for (int j = i + 1; j < limite; j++)
        {
            if (
                minijuego.estadosJugadores[i].equipo < 0 ||
                minijuego.estadosJugadores[i].equipo !=
                    minijuego.estadosJugadores[j].equipo
            )
            {
                continue;
            }

            float dx = jugadores[j].posicion.x - jugadores[i].posicion.x;
            float dz = jugadores[j].posicion.z - jugadores[i].posicion.z;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float minimo = RADIO_JUGADOR_DEFENSA * 2.0f;

            if (distancia < minimo)
            {
                if (distancia < 0.0001f)
                {
                    dx = 0.0f;
                    dz = 1.0f;
                    distancia = 1.0f;
                }

                float empuje = (minimo - distancia) * 0.5f;
                jugadores[i].posicion.x -= dx / distancia * empuje;
                jugadores[i].posicion.z -= dz / distancia * empuje;
                jugadores[j].posicion.x += dx / distancia * empuje;
                jugadores[j].posicion.z += dz / distancia * empuje;
            }
        }
    }
}


//==================================================
// INICIALIZACION
//==================================================

void MinijuegoDefensaNucleo::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;
    nucleo = {};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    // Camara cenital inclinada: +X a la derecha y +Z hacia el espectador.
    camara.position = { 0.0f, 19.5f, 13.0f };
    camara.target = { 0.0f, 0.0f, 0.6f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 47.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_DEFENSA_NUCLEO_PREPARACION;
    goles[0] = 0;
    goles[1] = 0;
    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;
    equipoUltimoGol = -1;
    equipoGanador = -1;
    golDeOro = false;
    empate = false;
    partidaValida = false;
    anguloObstaculo = 0.0f;
    direccionGiroObstaculo = 1.0f;
    tiempoPreparacion = DURACION_PREPARACION_DEFENSA;
    tiempoRestante = DURACION_PARTIDA_DEFENSA;
    tiempoOro = 0.0f;
    tiempoPausaGol = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoDefensaNucleo::Reiniciar(
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

    // Tema pseudoaleatorio segun el reloj del sistema.
    indiceTema =
        (int)(((long long)std::time(nullptr) + GetRandomValue(0, 4)) %
        CANTIDAD_TEMAS_DEFENSA);

    direccionGiroObstaculo = GetRandomValue(0, 1) == 0 ? 1.0f : -1.0f;
    anguloObstaculo = (float)GetRandomValue(0, 314) / 100.0f;

    int limite = LimiteJugadoresDefensa(cantidadMaxima);
    int indices[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            indices[cantidad++] = i;
        }
    }

    resultado.cantidadEquipos = 2;

    if (cantidad < 2)
    {
        for (int k = 0; k < cantidad; k++)
        {
            resultado.participantes[indices[k]].numeroEquipo = 0;
        }

        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_DEFENSA_NUCLEO_TERMINADO;
        return;
    }

    partidaValida = true;

    for (int i = cantidad - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = indices[i];
        indices[i] = indices[otro];
        indices[otro] = temporal;
    }

    // Con 4: 2 vs 2. Con 3: 2 vs 1. Con 2: 1 vs 1 (prueba).
    int enEquipo0 = cantidad >= 4 ? 2 : (cantidad == 3 ? 2 : 1);
    int ordenEquipo[2] = { 0, 0 };

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        int equipo = k < enEquipo0 ? 0 : 1;
        int orden = ordenEquipo[equipo]++;

        estadosJugadores[indice].equipo = equipo;
        cantidadJugadoresEquipo[equipo]++;
        resultado.participantes[indice].numeroEquipo = equipo;

        // Se resuelve despues el reparto en Z segun el tamano del equipo.
        estadosJugadores[indice].objetivoZ = (float)orden;
    }

    for (int k = 0; k < cantidad; k++)
    {
        int indice = indices[k];
        EstadoJugadorDefensaNucleo& estado = estadosJugadores[indice];
        int orden = (int)estado.objetivoZ;
        float ataque = DireccionAtaqueDefensa(estado.equipo);

        float z =
            cantidadJugadoresEquipo[estado.equipo] <= 1
            ? 0.0f
            : (orden == 0 ? -3.0f : 3.0f);

        Vector3 spawn = { -ataque * 6.0f, ALTURA_JUGADOR_DEFENSA, z };

        // Perfil estandar: evita heredar tamano/fisica del minijuego anterior.
        ConfigurarJugadorMinijuegoEstandar(jugadores[indice], spawn);
        jugadores[indice].posicion = spawn;
        jugadores[indice].direccionMirada = { ataque, 0.0f, 0.0f };
        jugadores[indice].enSuelo = true;
        jugadores[indice].cayendo = false;

        estado.objetivoX = spawn.x;
        estado.objetivoZ = z;
        estado.tiempoDecisionBot = (float)GetRandomValue(0, 30) / 100.0f;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoDefensaNucleo::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    // Un tiron de frame grande haria que el nucleo atraviese obstaculos.
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_DEFENSA_NUCLEO_TERMINADO ||
        resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int limite = LimiteJugadoresDefensa(cantidadMaxima);

    if (fase == FASE_DEFENSA_NUCLEO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_DEFENSA_NUCLEO_JUGANDO;
            SacarNucleoDefensa(*this, -1);
        }

        return;
    }

    anguloObstaculo +=
        direccionGiroObstaculo *
        TEMAS_DEFENSA[indiceTema].giroObstaculo *
        deltaTime;

    ActualizarJugadoresDefensa(
        *this,
        deltaTime,
        jugadores,
        limite,
        participantes
    );

    if (nucleo.tiempoCarga > 0.0f)
    {
        nucleo.tiempoCarga -= deltaTime;
    }

    if (fase == FASE_DEFENSA_NUCLEO_PAUSA_GOL)
    {
        tiempoPausaGol -= deltaTime;

        if (tiempoPausaGol <= 0.0f)
        {
            tiempoPausaGol = 0.0f;
            fase = FASE_DEFENSA_NUCLEO_JUGANDO;
            // Saca hacia quien recibio el gol.
            SacarNucleoDefensa(*this, equipoUltimoGol == 0 ? 1 : 0);
        }

        return;
    }

    int gol = MoverNucleoDefensa(*this, jugadores, limite, deltaTime);

    if (gol >= 0)
    {
        goles[gol]++;
        equipoUltimoGol = gol;
        nucleo.velocidadX = 0.0f;
        nucleo.velocidadZ = 0.0f;
        nucleo.enJuego = false;

        if (golDeOro || goles[gol] >= GOLES_PARA_GANAR_DEFENSA)
        {
            FinalizarDefensaNucleo(*this);
            return;
        }

        fase = FASE_DEFENSA_NUCLEO_PAUSA_GOL;
        tiempoPausaGol = DURACION_PAUSA_GOL_DEFENSA;
        return;
    }

    if (!golDeOro)
    {
        float restanteAntes = tiempoRestante;
        tiempoRestante -= deltaTime;
        ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

        if (tiempoRestante <= 0.0f)
        {
            tiempoRestante = 0.0f;

            if (goles[0] != goles[1])
            {
                FinalizarDefensaNucleo(*this);
            }
            else
            {
                golDeOro = true;
                tiempoOro = DURACION_GOL_ORO_DEFENSA;
            }
        }
    }
    else
    {
        tiempoOro -= deltaTime;

        if (tiempoOro <= 0.0f)
        {
            tiempoOro = 0.0f;
            FinalizarDefensaNucleo(*this);
        }
    }
}


//==================================================
// VISUAL: ESCENARIO
//==================================================

// Decorado exterior de cada tematica. Todo con primitivas.
// MODELO FUTURO: cada bloque podria sustituirse por un .glb del entorno
// (estacion, central, ciudad, ruinas, azotea) sin tocar el gameplay.
static void DibujarDecoradoTemaDefensa(int tema, float t)
{
    if (tema == TEMA_ESTACION_ESPACIAL)
    {
        DrawSphere({ -13.0f, 3.0f, -30.0f }, 10.0f, Color{ 40, 90, 170, 255 });
        DrawSphere({ -13.0f, 3.0f, -29.0f }, 10.2f, Fade(Color{ 120, 180, 255, 255 }, 0.25f));

        for (int i = 0; i < 70; i++)
        {
            DrawSphere(
                {
                    -34.0f + Ruido01Defensa(i, 1) * 68.0f,
                    -6.0f + Ruido01Defensa(i, 2) * 30.0f,
                    -28.0f - Ruido01Defensa(i, 3) * 4.0f
                },
                0.07f + Ruido01Defensa(i, 4) * 0.08f,
                RAYWHITE
            );
        }

        for (int lado = -1; lado <= 1; lado += 2)
        {
            DrawCube({ lado * 17.5f, -0.8f, -4.0f }, 6.0f, 0.12f, 9.0f, Color{ 30, 60, 130, 255 });
            DrawCube({ lado * 14.0f, -0.8f, -4.0f }, 1.2f, 0.4f, 0.4f, Color{ 150, 160, 180, 255 });
            DrawCube({ lado * 14.5f, -1.2f, 2.0f }, 7.0f, 1.0f, 1.0f, Color{ 90, 100, 124, 255 });
        }
    }
    else if (tema == TEMA_CENTRAL_ELECTRICA)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            DrawCylinder({ lado * 16.0f, -2.0f, -15.0f }, 3.0f, 4.8f, 15.0f, 20, Color{ 128, 124, 116, 255 });

            for (int i = 0; i < 5; i++)
            {
                float fase = std::fmod(t * 0.5f + i * 0.2f, 1.0f);
                DrawSphere(
                    { lado * 16.0f + std::sin(t + i) * 0.6f, 13.5f + fase * 5.0f, -15.0f },
                    1.4f + fase * 1.6f,
                    Fade(RAYWHITE, 0.30f * (1.0f - fase))
                );
            }

            // Torre de alta tension.
            DrawCube({ lado * 14.0f, 3.0f, -6.0f }, 0.35f, 8.0f, 0.35f, Color{ 70, 72, 78, 255 });
            DrawCube({ lado * 14.0f, 6.5f, -6.0f }, 3.2f, 0.25f, 0.25f, Color{ 70, 72, 78, 255 });
            DrawCube({ lado * 13.0f, 0.4f, 3.5f }, 1.8f, 1.4f, 1.4f, Color{ 90, 100, 96, 255 });
        }

        DrawLine3D({ -14.0f, 6.5f, -6.0f }, { 14.0f, 6.5f, -6.0f }, Color{ 20, 20, 20, 255 });

        for (int i = 0; i < 12; i++)
        {
            DrawCube(
                { -LIMITE_X_DEFENSA + 1.0f + i * 1.9f, 0.1f, -LIMITE_Z_DEFENSA - 1.2f },
                1.0f, 0.2f, 0.9f,
                i % 2 == 0 ? Color{ 230, 190, 30, 255 } : Color{ 30, 30, 30, 255 }
            );
        }
    }
    else if (tema == TEMA_CIUDAD_FUTURISTA)
    {
        for (int i = 0; i < 16; i++)
        {
            float x = -30.0f + i * 4.0f;
            float alto = 5.0f + Ruido01Defensa(i, 7) * 13.0f;
            Color neon = i % 2 == 0 ? Color{ 255, 70, 200, 255 } : Color{ 70, 230, 255, 255 };

            DrawCube({ x, alto / 2.0f - 2.0f, -17.0f - Ruido01Defensa(i, 8) * 4.0f }, 3.2f, alto, 3.2f, Color{ 24, 22, 52, 255 });
            DrawCube({ x, alto - 2.0f, -15.3f - Ruido01Defensa(i, 8) * 4.0f }, 3.2f, 0.18f, 0.12f, neon);
            DrawCube({ x, alto * 0.5f - 2.0f, -15.3f - Ruido01Defensa(i, 8) * 4.0f }, 0.12f, alto * 0.6f, 0.12f, Fade(neon, 0.8f));
        }

        DrawCube({ 0.0f, 9.0f, -26.0f }, 9.0f, 3.0f, 0.3f, Fade(Color{ 255, 70, 200, 255 }, 0.55f + 0.2f * std::sin(t * 3.0f)));

        // Carriles de trafico a los lados.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            DrawCube({ lado * 16.0f, -1.0f, 0.0f }, 3.0f, 0.2f, 24.0f, Color{ 20, 18, 40, 255 });
            DrawCube({ lado * 16.0f, -0.85f, std::fmod(t * 6.0f + lado * 7.0f, 22.0f) - 11.0f }, 0.8f, 0.3f, 1.6f, Color{ 70, 230, 255, 255 });
        }
    }
    else if (tema == TEMA_RUINAS_SUBMARINAS)
    {
        for (int i = 0; i < 9; i++)
        {
            float angulo = -2.6f + i * 0.65f;
            float x = std::cos(angulo) * 17.0f;
            float z = -std::abs(std::sin(angulo)) * 13.0f - 2.0f;
            float alto = 2.0f + Ruido01Defensa(i, 3) * 5.0f;

            DrawCylinder({ x, -1.5f, z }, 0.8f, 0.9f, alto, 10, Color{ 118, 128, 118, 255 });
            DrawCube({ x, -1.5f + alto, z }, 1.9f, 0.4f, 1.9f, Color{ 104, 114, 106, 255 });
            DrawSphere({ x + 1.4f, -1.3f, z + 0.8f }, 0.6f + Ruido01Defensa(i, 5) * 0.5f, i % 2 == 0 ? Color{ 230, 110, 150, 255 } : Color{ 240, 160, 90, 255 });
        }

        // Arco roto al fondo.
        DrawCube({ -4.0f, 2.0f, -19.0f }, 1.4f, 8.0f, 1.4f, Color{ 100, 114, 106, 255 });
        DrawCube({ 4.0f, 1.0f, -19.0f }, 1.4f, 6.0f, 1.4f, Color{ 100, 114, 106, 255 });
        DrawCube({ -1.5f, 6.2f, -19.0f }, 5.5f, 1.2f, 1.4f, Color{ 112, 124, 116, 255 });

        for (int i = 0; i < 24; i++)
        {
            float subida = std::fmod(t * (0.6f + Ruido01Defensa(i, 9) * 0.5f) + Ruido01Defensa(i, 10) * 10.0f, 12.0f);
            DrawSphere(
                { -20.0f + Ruido01Defensa(i, 11) * 40.0f, -1.0f + subida, -6.0f - Ruido01Defensa(i, 12) * 12.0f },
                0.14f + Ruido01Defensa(i, 13) * 0.14f,
                Fade(Color{ 200, 245, 255, 255 }, 0.45f)
            );
        }

        for (int i = 0; i < 4; i++)
        {
            DrawCube({ -12.0f + i * 8.0f, 8.0f, -10.0f }, 1.6f, 20.0f, 3.0f, Fade(Color{ 190, 255, 240, 255 }, 0.07f));
        }
    }
    else
    {
        // Azoteas: edificios vecinos mas bajos, deposito de agua y antenas.
        for (int i = 0; i < 12; i++)
        {
            float x = -28.0f + i * 5.0f;
            float alto = 4.0f + Ruido01Defensa(i, 21) * 8.0f;
            DrawCube({ x, -0.3f - alto / 2.0f, -14.0f - Ruido01Defensa(i, 22) * 5.0f }, 4.2f, alto, 4.2f, i % 2 == 0 ? Color{ 150, 140, 130, 255 } : Color{ 120, 128, 140, 255 });
        }

        for (int pata = 0; pata < 4; pata++)
        {
            DrawCube({ -15.0f + (pata % 2) * 1.6f, 1.2f, -9.0f + (pata / 2) * 1.6f }, 0.2f, 2.4f, 0.2f, Color{ 90, 70, 60, 255 });
        }

        DrawCylinder({ -14.2f, 2.4f, -8.2f }, 1.5f, 1.5f, 2.2f, 16, Color{ 168, 108, 70, 255 });
        DrawCylinder({ -14.2f, 4.6f, -8.2f }, 0.0f, 1.6f, 0.9f, 16, Color{ 120, 78, 52, 255 });
        DrawCube({ 14.5f, 0.6f, -4.0f }, 2.4f, 1.2f, 1.6f, Color{ 190, 192, 196, 255 });
        DrawCube({ 14.5f, 1.3f, -4.0f }, 1.8f, 0.2f, 1.2f, Color{ 120, 124, 130, 255 });
        DrawCylinder({ 15.5f, 0.0f, -8.5f }, 0.1f, 0.1f, 8.0f, 6, Color{ 70, 70, 76, 255 });
        DrawSphere({ 15.5f, 8.1f, -8.5f }, 0.22f, std::sin(t * 4.0f) > 0.0f ? RED : Color{ 90, 20, 20, 255 });
        DrawCube({ 0.0f, 4.0f, -24.0f }, 60.0f, 14.0f, 0.2f, Color{ 150, 200, 235, 255 });
    }
}


// VISUAL: suelo, paredes, arcos y obstaculo central. No contiene reglas.
// MODELO FUTURO: la arena completa (suelo + paredes + chaflanes) puede ser un
// unico .glb; los arcos y el obstaculo central, modelos separados.
static void DibujarEscenarioVisualDefensa(
    const MinijuegoDefensaNucleo& minijuego
)
{
    const DatosTemaDefensa& tema = TEMAS_DEFENSA[minijuego.indiceTema];
    float t = minijuego.tiempoAnimacion;
    float c = tema.chaflan;

    DibujarDecoradoTemaDefensa(minijuego.indiceTema, t);

    // Base y suelo en baldosas.
    DrawCube({ 0.0f, -0.35f, 0.0f }, 2.0f * LIMITE_X_DEFENSA + 1.2f, 0.6f, 2.0f * LIMITE_Z_DEFENSA + 1.2f, tema.vacio);

    for (int i = 0; i < 11; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            DrawCube(
                { -10.0f + i * 2.0f, 0.0f, -6.0f + j * 2.0f },
                2.0f, 0.04f, 2.0f,
                (i + j) % 2 == 0 ? tema.sueloA : tema.sueloB
            );
        }
    }

    // Esquinas cortadas: se tapan con el color del vacio y se pone riel.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            Vector3 a = { sx * LIMITE_X_DEFENSA, 0.05f, sz * LIMITE_Z_DEFENSA };
            Vector3 b = { sx * (LIMITE_X_DEFENSA - c), 0.05f, sz * LIMITE_Z_DEFENSA };
            Vector3 d = { sx * LIMITE_X_DEFENSA, 0.05f, sz * (LIMITE_Z_DEFENSA - c) };

            DrawTriangle3D(a, b, d, tema.vacio);
            DrawTriangle3D(a, d, b, tema.vacio);

            DrawCylinderEx(
                { b.x, 0.5f, b.z },
                { d.x, 0.5f, d.z },
                0.28f,
                0.28f,
                6,
                tema.pared
            );
        }
    }

    // Marcas del campo.
    DrawCube({ 0.0f, 0.045f, 0.0f }, 0.12f, 0.02f, 2.0f * LIMITE_Z_DEFENSA, Fade(tema.acento, 0.5f));
    DrawCircle3D({ 0.0f, 0.05f, 0.0f }, 3.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(tema.acento, 0.55f));

    // Paredes rectas (altura 0.9).
    float largoX = 2.0f * (LIMITE_X_DEFENSA - c);
    DrawCube({ 0.0f, 0.45f, -LIMITE_Z_DEFENSA - 0.2f }, largoX, 0.9f, 0.4f, tema.pared);
    DrawCube({ 0.0f, 0.45f, LIMITE_Z_DEFENSA + 0.2f }, largoX, 0.5f, 0.4f, Fade(tema.pared, 0.9f));
    DrawCube({ 0.0f, 0.95f, -LIMITE_Z_DEFENSA - 0.2f }, largoX, 0.1f, 0.45f, tema.acento);

    float largoZ = LIMITE_Z_DEFENSA - c - MITAD_ARCO_DEFENSA;
    float centroZ = (LIMITE_Z_DEFENSA - c + MITAD_ARCO_DEFENSA) / 2.0f;

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            DrawCube({ sx * (LIMITE_X_DEFENSA + 0.2f), 0.45f, sz * centroZ }, 0.4f, 0.9f, largoZ, tema.pared);
        }
    }

    // Arcos con el color del equipo.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        float lado = equipo == 0 ? -1.0f : 1.0f;
        Color color = ObtenerColorEquipoDefensa(equipo);
        float x = lado * LIMITE_X_DEFENSA;

        DrawCube({ x + lado * 0.9f, 0.04f, 0.0f }, 1.8f, 0.08f, 2.0f * MITAD_ARCO_DEFENSA, Fade(color, 0.85f));
        DrawCube({ x + lado * 1.8f, 0.7f, 0.0f }, 0.2f, 1.4f, 2.0f * MITAD_ARCO_DEFENSA + 0.6f, Fade(color, 0.7f));
        DrawCube({ x + lado * 0.9f, 0.7f, MITAD_ARCO_DEFENSA }, 1.8f, 1.4f, 0.12f, Fade(color, 0.45f));
        DrawCube({ x + lado * 0.9f, 0.7f, -MITAD_ARCO_DEFENSA }, 1.8f, 1.4f, 0.12f, Fade(color, 0.45f));
        DrawCube({ x - lado * 0.7f, 0.05f, 0.0f }, 0.15f, 0.02f, 2.0f * MITAD_ARCO_DEFENSA, color);

        for (int sz = -1; sz <= 1; sz += 2)
        {
            DrawCylinder({ x, 0.0f, sz * MITAD_ARCO_DEFENSA }, RADIO_POSTE_DEFENSA, RADIO_POSTE_DEFENSA, 1.5f, 10, RAYWHITE);
            DrawSphere({ x, 1.55f, sz * MITAD_ARCO_DEFENSA }, 0.34f, color);
        }

        DrawCylinderEx({ x, 1.5f, -MITAD_ARCO_DEFENSA }, { x, 1.5f, MITAD_ARCO_DEFENSA }, 0.12f, 0.12f, 8, color);
    }

    // Obstaculo central giratorio.
    float ex = std::cos(minijuego.anguloObstaculo) * MITAD_LARGO_OBSTACULO_DEFENSA;
    float ez = std::sin(minijuego.anguloObstaculo) * MITAD_LARGO_OBSTACULO_DEFENSA;

    DrawCylinderEx({ -ex, 0.5f, -ez }, { ex, 0.5f, ez }, RADIO_OBSTACULO_DEFENSA, RADIO_OBSTACULO_DEFENSA, 10, tema.pared);
    DrawCylinderEx({ -ex, 0.78f, -ez }, { ex, 0.78f, ez }, 0.08f, 0.08f, 6, tema.acento);
    DrawCylinder({ 0.0f, 0.0f, 0.0f }, 0.55f, 0.65f, 0.9f, 14, tema.acento);
    DrawSphere({ ex, 0.5f, ez }, 0.38f, tema.acento);
    DrawSphere({ -ex, 0.5f, -ez }, 0.38f, tema.acento);

    // Chispas en las puntas en la central electrica.
    if (minijuego.indiceTema == TEMA_CENTRAL_ELECTRICA)
    {
        for (int punta = -1; punta <= 1; punta += 2)
        {
            Vector3 anterior = { punta * ex, 0.9f, punta * ez };

            for (int k = 1; k <= 4; k++)
            {
                Vector3 siguiente =
                {
                    punta * ex + (Ruido01Defensa(k, (int)(minijuego.tiempoAnimacion * 20.0f)) - 0.5f) * 1.2f,
                    0.9f + (float)k * 0.18f,
                    punta * ez + (Ruido01Defensa(k + 9, (int)(minijuego.tiempoAnimacion * 20.0f)) - 0.5f) * 1.2f
                };

                DrawLine3D(anterior, siguiente, Color{ 255, 240, 120, 255 });
                anterior = siguiente;
            }
        }
    }
}


//==================================================
// VISUAL: NUCLEO Y JUGADORES
//==================================================

static void DibujarNucleoDefensa(
    const NucleoDefensaNucleo& nucleo,
    bool visible
)
{
    if (!visible)
    {
        return;
    }

    float velocidad = VelocidadNucleoDefensa(nucleo);
    float rapidez = LimitarDefensa(velocidad / VELOCIDAD_MAXIMA_NUCLEO_DEFENSA, 0.0f, 1.0f);

    DrawCircle3D({ nucleo.x, 0.05f, nucleo.z }, RADIO_NUCLEO_DEFENSA * 1.1f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.5f));

    // Estela proporcional a la velocidad.
    if (velocidad > 0.5f)
    {
        for (int k = 1; k <= 4; k++)
        {
            float atras = (float)k * 0.12f * (0.6f + rapidez * 1.2f);
            DrawSphere(
                {
                    nucleo.x - nucleo.velocidadX / velocidad * atras * 2.0f,
                    0.55f,
                    nucleo.z - nucleo.velocidadZ / velocidad * atras * 2.0f
                },
                RADIO_NUCLEO_DEFENSA * (1.0f - 0.18f * (float)k),
                Fade(Color{ 255, 220, 70, 255 }, 0.30f - 0.06f * (float)k)
            );
        }
    }

    Color nucleoColor = Color{ 255, 232, 70, 255 };

    if (nucleo.tiempoCarga > 0.0f)
    {
        nucleoColor = Color{ 255, 140, 40, 255 };
        DrawSphere({ nucleo.x, 0.55f, nucleo.z }, RADIO_NUCLEO_DEFENSA * 1.55f, Fade(Color{ 255, 120, 30, 255 }, 0.35f));
    }

    DrawSphere({ nucleo.x, 0.55f, nucleo.z }, RADIO_NUCLEO_DEFENSA, nucleoColor);
    DrawSphere({ nucleo.x, 0.55f, nucleo.z }, RADIO_NUCLEO_DEFENSA * 0.55f, RAYWHITE);
    DrawSphereWires({ nucleo.x, 0.55f, nucleo.z }, RADIO_NUCLEO_DEFENSA * 1.05f, 6, 8, Fade(BLACK, 0.7f));
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoDefensaNucleo::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteJugadoresDefensa(cantidadMaxima);
    const DatosTemaDefensa& tema = TEMAS_DEFENSA[indiceTema];

    ClearBackground(tema.fondo);
    BeginMode3D(camara);

    DibujarEscenarioVisualDefensa(*this);

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        const EstadoJugadorDefensaNucleo& estado = estadosJugadores[i];
        const JugadorPrueba& jugador = jugadores[i];
        Color colorEquipo = ObtenerColorEquipoDefensa(estado.equipo);

        DrawCircle3D({ jugador.posicion.x, 0.07f, jugador.posicion.z }, RADIO_JUGADOR_DEFENSA + 0.12f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);
        DrawCircle3D({ jugador.posicion.x, 0.07f, jugador.posicion.z }, RADIO_JUGADOR_DEFENSA + 0.02f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(jugador, participanteVisual);

        if (estado.tiempoGolpeActivo > 0.0f)
        {
            DrawCircle3D({ jugador.posicion.x, 0.12f, jugador.posicion.z }, RADIO_JUGADOR_DEFENSA + ALCANCE_EXTRA_GOLPE_DEFENSA, { 1.0f, 0.0f, 0.0f }, 90.0f, YELLOW);
            DrawCircle3D({ jugador.posicion.x, 0.13f, jugador.posicion.z }, RADIO_JUGADOR_DEFENSA + ALCANCE_EXTRA_GOLPE_DEFENSA - 0.06f, { 1.0f, 0.0f, 0.0f }, 90.0f, WHITE);
        }

        if (mostrarDebug)
        {
            DrawCircle3D({ jugador.posicion.x, 0.2f, jugador.posicion.z }, RADIO_JUGADOR_DEFENSA, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }
    }

    DibujarNucleoDefensa(
        nucleo,
        fase != FASE_DEFENSA_NUCLEO_PREPARACION &&
        resultado.estado != RESULTADO_MINIJUEGO_CANCELADO
    );

    if (fase == FASE_DEFENSA_NUCLEO_PREPARACION)
    {
        // Muestra el nucleo en el centro antes del saque.
        NucleoDefensaNucleo previo{};
        DibujarNucleoDefensa(previo, true);
    }

    if (mostrarDebug)
    {
        DrawCircle3D({ nucleo.x, 0.25f, nucleo.z }, RADIO_NUCLEO_DEFENSA, { 1.0f, 0.0f, 0.0f }, 90.0f, RED);
    }

    EndMode3D();

    // HUD: marcador.
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangle(ancho / 2 - 250, 10, 500, 76, Fade(BLACK, 0.78f));
    DrawRectangle(ancho / 2 - 250, 10, 8, 76, ObtenerColorEquipoDefensa(0));
    DrawRectangle(ancho / 2 + 242, 10, 8, 76, ObtenerColorEquipoDefensa(1));

    const char* marcador = TextFormat("%d  -  %d", goles[0], goles[1]);
    DrawText(marcador, ancho / 2 - MeasureText(marcador, 40) / 2, 14, 40, RAYWHITE);
    DrawText("EQUIPO 1", ancho / 2 - 236, 18, 18, ObtenerColorEquipoDefensa(0));
    DrawText("EQUIPO 2", ancho / 2 + 236 - MeasureText("EQUIPO 2", 18), 18, 18, ObtenerColorEquipoDefensa(1));

    const char* subtitulo = golDeOro
        ? TextFormat("GOL DE ORO  %.1f s", tiempoOro)
        : TextFormat("%s  -  %.0f s  -  A %d GOLES", tema.nombre, tiempoRestante, GOLES_PARA_GANAR_DEFENSA);

    DrawText(
        subtitulo,
        ancho / 2 - MeasureText(subtitulo, 17) / 2,
        62,
        17,
        golDeOro ? GOLD : LIGHTGRAY
    );

    // HUD: controles de cada jugador.
    int y = alto - 30;

    for (int i = limite - 1; i >= 0; i--)
    {
        if (estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        const EstadoJugadorDefensaNucleo& estado = estadosJugadores[i];
        const char* accion =
            estado.cooldownGolpe > 0.0f ? "RECARGANDO" : "GOLPE LISTO";

        const char* linea = EsControladoPorBotDefensa(participantes[i])
            ? TextFormat("J%d BOT  EQ%d", participantes[i].numeroJugador, estado.equipo + 1)
            : TextFormat(
                "J%d  EQ%d  MOVER LIBRE  GOLPE CARGADO [%s]  %s",
                participantes[i].numeroJugador,
                estado.equipo + 1,
                ObtenerTextoBotonPrincipal(participantes[i]),
                accion
            );

        DrawText(
            linea,
            18,
            y,
            17,
            estado.cooldownGolpe > 0.0f && !EsControladoPorBotDefensa(participantes[i])
                ? ORANGE
                : ObtenerColorEquipoDefensa(estado.equipo)
        );

        y -= 22;
    }

    if (fase == FASE_DEFENSA_NUCLEO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, ancho / 2 - MeasureText(texto, 90) / 2, alto / 2 - 50, 90, YELLOW);
    }
    else if (fase == FASE_DEFENSA_NUCLEO_PAUSA_GOL)
    {
        const char* texto = TextFormat("GOL DEL EQUIPO %d", equipoUltimoGol + 1);
        DrawText(
            texto,
            ancho / 2 - MeasureText(texto, 44) / 2,
            alto / 2 - 30,
            44,
            ObtenerColorEquipoDefensa(equipoUltimoGol)
        );
    }
    else if (
        fase == FASE_DEFENSA_NUCLEO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        const int yPanel = (int)(110 * ((float)alto / 720.0f));
        DrawRectangle(ancho / 2 - 330, yPanel, 660, 180, Fade(BLACK, 0.84f));

        const char* titulo = empate
            ? "EMPATE"
            : TextFormat("GANA EL EQUIPO %d", equipoGanador + 1);

        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, 36) / 2,
            yPanel + 28,
            36,
            empate ? YELLOW : ObtenerColorEquipoDefensa(equipoGanador)
        );

        const char* final = TextFormat("%d  -  %d", goles[0], goles[1]);
        DrawText(final, ancho / 2 - MeasureText(final, 34) / 2, yPanel + 80, 34, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), ancho / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2, yPanel + 134, 18, LIGHTGRAY);
    }
    else if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        const char* texto = "SE NECESITAN AL MENOS 2 JUGADORES";
        DrawText(texto, ancho / 2 - MeasureText(texto, 26) / 2, alto / 2, 26, RED);
    }
}


const ResultadoMinijuego& MinijuegoDefensaNucleo::ObtenerResultado() const
{
    return resultado;
}
