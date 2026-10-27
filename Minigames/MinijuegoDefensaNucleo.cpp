#include "Minigames/MinijuegoDefensaNucleo.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
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
// VARIANTES DE CANCHA (datos de gameplay)
//==================================================
//
// Heredadas de las viejas tematicas: cada partida sortea una forma de
// esquinas y una velocidad del obstaculo. El escenario es siempre el
// mismo estadio; el dibujo de la cancha sigue al chaflan sorteado.

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
    // Tamano del corte de las esquinas: define la forma de la arena.
    float chaflan;

    // Velocidad (rad/s) del obstaculo central.
    float giroObstaculo;
};


static const DatosTemaDefensa TEMAS_DEFENSA[CANTIDAD_TEMAS_DEFENSA] =
{
    { 3.4f, 1.0f },
    { 1.6f, 1.5f },
    { 2.4f, 1.2f },
    { 4.0f, 0.8f },
    { 1.0f, 1.3f }
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


// Solo visual (al final del archivo): no tocan la logica.
static void ReiniciarVisualDefensa(MinijuegoDefensaNucleo& minijuego);

static void ActualizarVisualDefensa(
    MinijuegoDefensaNucleo& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
);


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
    // Un poco mas atras que la cancha para ver gradas, bancos y ciudad.
    camara.position = { 0.0f, 16.5f, 16.5f };
    camara.target = { 0.0f, 0.0f, -1.2f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
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

    ReiniciarVisualDefensa(*this);
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
    ActualizarVisualDefensa(*this, deltaTime, jugadores, cantidadMaxima, participantes);

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
// VISUAL: ESTADO, FEEDBACK Y PARTICULAS
//==================================================
//
// Todo lo que sigue es solo visual: lee la logica, nunca la escribe, y su
// azar sale de semillaVisual (no consume el GetRandomValue de los bots).

static const float DURACION_ANUNCIO_DEFENSA = 1.4f;
static const float DURACION_ANUNCIO_GOL_DEFENSA = 1.7f;
static const float ALTURA_NUCLEO_DEFENSA = 0.55f;
static const float ALTO_POSTE_DEFENSA = 1.5f;
static const float FONDO_ARCO_DEFENSA = 1.7f;      // bolsillo detras de la linea
static const float GROSOR_VALLA_DEFENSA = 0.4f;
static const float ALTO_VALLA_DEFENSA = 0.5f;
static const float BASE_AZOTEA_DEFENSA = -0.3f;    // la cancha es una tarima
static const float BORDE_AZOTEA_X_DEFENSA = 17.4f;
static const float FRENTE_AZOTEA_DEFENSA = 10.5f;
static const float FONDO_AZOTEA_DEFENSA = -16.5f;
static const float RADIO_GIRATORIA_DEFENSA = 2.5f;

static Color MezclaColorDefensa(Color a, Color b, float t);


static float Limitar01Defensa(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float SuaveDefensa(float u)
{
    u = Limitar01Defensa(u);
    return u * u * (3.0f - 2.0f * u);
}


// Azar propio para particulas (xorshift sobre semillaVisual).
static float AzarDefensa(MinijuegoDefensaNucleo& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasDefensa(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


// Lado del arco que defiende cada equipo (-1 izquierda, +1 derecha).
static float LadoArcoEquipoDefensa(int equipo)
{
    return equipo == 0 ? -1.0f : 1.0f;
}


static void EmitirParticulaDefensa(
    MinijuegoDefensaNucleo& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color,
    bool brillo
)
{
    for (int i = 0; i < MAX_PARTICULAS_DEFENSA_NUCLEO; i++)
    {
        ParticulaDefensaNucleo& p = minijuego.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
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


// Chispas en abanico desde un punto.
static void EmitirChispasDefensa(
    MinijuegoDefensaNucleo& minijuego,
    Vector3 centro,
    Color color,
    int base,
    float rapidez,
    bool brillo
)
{
    int cantidad = CantidadParticulasDefensa(base);

    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AzarDefensa(minijuego, 0.0f, 6.2832f);
        float r = AzarDefensa(minijuego, 0.35f, 1.0f) * rapidez;

        EmitirParticulaDefensa(
            minijuego,
            centro,
            { std::cos(angulo) * r, AzarDefensa(minijuego, 0.5f, 1.5f) * rapidez * 0.55f, std::sin(angulo) * r },
            AzarDefensa(minijuego, 0.22f, 0.5f),
            AzarDefensa(minijuego, 0.05f, 0.1f),
            9.0f,
            color,
            brillo
        );
    }
}


// Papelitos que caen con la gravedad (festejo y final).
static void EmitirConfetiDefensa(
    MinijuegoDefensaNucleo& minijuego,
    Vector3 origen,
    Vector3 empuje,
    Color colorEquipo,
    int base
)
{
    int cantidad = CantidadParticulasDefensa(base);
    const Color colores[4] =
    {
        colorEquipo,
        Color{ 255, 214, 92, 255 },
        Color{ 236, 242, 255, 255 },
        Color{ 255, 96, 196, 255 }
    };

    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AzarDefensa(minijuego, 0.0f, 6.2832f);
        float abre = AzarDefensa(minijuego, 0.4f, 2.0f);

        EmitirParticulaDefensa(
            minijuego,
            origen,
            {
                empuje.x + std::cos(angulo) * abre,
                empuje.y * AzarDefensa(minijuego, 0.7f, 1.15f),
                empuje.z + std::sin(angulo) * abre
            },
            AzarDefensa(minijuego, 1.4f, 2.2f),
            AzarDefensa(minijuego, 0.08f, 0.14f),
            5.0f,
            colores[k % 4],
            false
        );
    }
}


// Fuego artificial: esfera de chispas brillantes que caen despacio.
static void EmitirFuegoArtificialDefensa(
    MinijuegoDefensaNucleo& minijuego,
    Vector3 centro,
    Color color,
    int base
)
{
    int cantidad = CantidadParticulasDefensa(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarDefensa(minijuego, 0.0f, 6.2832f);
        float b = AzarDefensa(minijuego, -1.0f, 1.0f);
        float r = std::sqrt(1.0f - b * b);
        float v = AzarDefensa(minijuego, 3.2f, 4.2f);

        EmitirParticulaDefensa(
            minijuego,
            centro,
            { std::cos(a) * r * v, b * v + 0.8f, std::sin(a) * r * v },
            AzarDefensa(minijuego, 0.9f, 1.4f),
            AzarDefensa(minijuego, 0.18f, 0.28f),
            1.6f,
            k % 4 == 0 ? Color{ 255, 255, 255, 255 } : color,
            true
        );
    }
}


static void ReiniciarVisualDefensa(MinijuegoDefensaNucleo& minijuego)
{
    MinijuegoDefensaNucleo& m = minijuego;

    m.semillaVisual = 2654435761u;
    m.tiempoAnuncio = -1.0f;
    m.tiempoAnuncioOro = -1.0f;
    m.tiempoGolVisual = -1.0f;
    m.festejo = 0.0f;
    m.destelloArco[0] = 0.0f;
    m.destelloArco[1] = 0.0f;
    m.destelloObstaculo = 0.0f;
    m.destelloSaque = 0.0f;
    m.golesVistos[0] = 0;
    m.golesVistos[1] = 0;
    m.equipoGolVisual = -1;
    m.juegoVisto = false;
    m.oroVisto = false;
    m.finVisto = false;
    m.nucleoVisto = false;
    m.velocidadVistaX = 0.0f;
    m.velocidadVistaZ = 0.0f;
    m.cargaVista = 0.0f;
    m.cantidadEstela = 0;
    m.relojEstela = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.destelloGolpe[i] = 0.0f;
        m.golpeVisto[i] = 0.0f;
        m.contactoVisto[i] = 0.0f;
    }

    for (int i = 0; i < LARGO_ESTELA_DEFENSA_NUCLEO; i++)
    {
        m.estela[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_DEFENSA_NUCLEO; i++)
    {
        m.particulas[i] = {};
    }
}


// Una vez por frame: detecta los cambios de la logica y lanza el
// feedback (sonido, particulas, anuncios, tribuna). Solo lee la logica.
static void ActualizarVisualDefensa(
    MinijuegoDefensaNucleo& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
)
{
    MinijuegoDefensaNucleo& m = minijuego;
    const float dt = deltaTime;
    const NucleoDefensaNucleo& n = m.nucleo;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int limite = LimiteJugadoresDefensa(cantidadMaxima);
    Vector3 posNucleo = { n.x, ALTURA_NUCLEO_DEFENSA, n.z };

    // Primer saque: «¡A JUGAR!».
    if (m.fase == FASE_DEFENSA_NUCLEO_JUGANDO && !m.juegoVisto)
    {
        m.juegoVisto = true;
        m.tiempoAnuncio = 0.0f;
    }

    if (m.tiempoAnuncio >= 0.0f)
    {
        m.tiempoAnuncio += dt;
        if (m.tiempoAnuncio > DURACION_ANUNCIO_DEFENSA) m.tiempoAnuncio = -1.0f;
    }

    if (m.golDeOro && !m.oroVisto)
    {
        m.oroVisto = true;
        m.tiempoAnuncioOro = 0.0f;
        m.festejo = 0.8f;
    }

    if (m.tiempoAnuncioOro >= 0.0f)
    {
        m.tiempoAnuncioOro += dt * 0.6f;
        if (m.tiempoAnuncioOro > DURACION_ANUNCIO_DEFENSA) m.tiempoAnuncioOro = -1.0f;
    }

    if (m.tiempoGolVisual >= 0.0f && m.tiempoGolVisual < 60.0f)
    {
        m.tiempoGolVisual += dt;
    }

    m.festejo = m.festejo > dt * 0.32f ? m.festejo - dt * 0.32f : 0.0f;
    m.destelloObstaculo = m.destelloObstaculo > dt * 4.0f ? m.destelloObstaculo - dt * 4.0f : 0.0f;
    m.destelloSaque = m.destelloSaque > dt * 2.5f ? m.destelloSaque - dt * 2.5f : 0.0f;

    for (int k = 0; k < 2; k++)
    {
        m.destelloArco[k] = m.destelloArco[k] > dt * 0.8f ? m.destelloArco[k] - dt * 0.8f : 0.0f;
    }

    // Goles: el arco se enciende, la tribuna estalla y hay papelitos.
    for (int k = 0; k < 2; k++)
    {
        if (m.goles[k] <= m.golesVistos[k])
        {
            continue;
        }

        m.golesVistos[k] = m.goles[k];
        m.equipoGolVisual = k;
        m.tiempoGolVisual = 0.0f;
        m.festejo = 1.0f;

        // El equipo k anota en el arco que defiende el otro equipo.
        float lado = LadoArcoEquipoDefensa(1 - k);
        m.destelloArco[lado < 0.0f ? 0 : 1] = 1.0f;
        Color color = ObtenerColorEquipoDefensa(k);
        Vector3 boca = { lado * (LIMITE_X_DEFENSA + 0.3f), 0.8f, LimitarDefensa(n.z, -2.0f, 2.0f) };

        EmitirChispasDefensa(m, boca, color, 26, 6.5f, true);
        EmitirChispasDefensa(m, boca, Color{ 255, 236, 170, 255 }, 12, 5.0f, true);
        EmitirConfetiDefensa(m, { lado * 8.0f, 0.6f, -8.6f }, { -lado * 1.2f, 8.5f, 3.0f }, color, 26);
        EmitirConfetiDefensa(m, { -lado * 6.0f, 0.6f, -8.6f }, { lado * 1.6f, 8.0f, 3.0f }, color, 18);

        if (decoracion)
        {
            EmitirFuegoArtificialDefensa(m, { lado * 22.0f, -1.0f, -21.0f }, color, 30);
        }

        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
        ActivarTemblorCamaraGeneral(0.12f, 0.3f);
    }

    // Saque: destello dorado donde aparece el nucleo.
    if (n.enJuego && !m.nucleoVisto)
    {
        m.destelloSaque = 1.0f;
        m.cantidadEstela = 0;
        EmitirChispasDefensa(m, posNucleo, Color{ 255, 220, 120, 255 }, 14, 3.5f, true);
    }

    // Golpes y toques de cada jugador.
    bool huboContacto = false;

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorDefensaNucleo& e = m.estadosJugadores[i];

        if (e.equipo < 0)
        {
            continue;
        }

        m.destelloGolpe[i] = m.destelloGolpe[i] > dt * 3.5f ? m.destelloGolpe[i] - dt * 3.5f : 0.0f;

        if (e.tiempoGolpeActivo > m.golpeVisto[i] + 0.001f)
        {
            m.destelloGolpe[i] = 1.0f;
        }

        m.golpeVisto[i] = e.tiempoGolpeActivo;

        if (e.cooldownContacto > m.contactoVisto[i] + 0.001f && n.enJuego)
        {
            huboContacto = true;
            Vector3 pj = jugadores[i].posicion;
            Vector3 punto = {
                (pj.x + n.x) * 0.5f,
                ALTURA_NUCLEO_DEFENSA,
                (pj.z + n.z) * 0.5f
            };
            Color color = ObtenerColorEquipoDefensa(e.equipo);
            bool humano = !EsControladoPorBotDefensa(participantes[i]);

            if (n.tiempoCarga > m.cargaVista + 0.5f)
            {
                // Golpe cargado: estallido y el nucleo se pone al rojo.
                EmitirChispasDefensa(m, punto, color, 16, 6.0f, true);
                EmitirChispasDefensa(m, punto, Color{ 255, 190, 90, 255 }, 12, 5.0f, true);
                ReproducirSonidoMinijuego(m.audio, SONIDO_GOLPE);

                if (humano)
                {
                    ActivarTemblorCamaraGeneral(0.07f, 0.16f);
                }
            }
            else
            {
                EmitirChispasDefensa(m, punto, color, 7, 3.2f, true);

                if (humano)
                {
                    ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
                }
            }
        }

        m.contactoVisto[i] = e.cooldownContacto;
    }

    // Rebotes contra el obstaculo y las vallas: el giro brusco de la
    // velocidad sin toque de jugador.
    if (n.enJuego && m.nucleoVisto && !huboContacto)
    {
        float va = std::sqrt(m.velocidadVistaX * m.velocidadVistaX + m.velocidadVistaZ * m.velocidadVistaZ);
        float vb = std::sqrt(n.velocidadX * n.velocidadX + n.velocidadZ * n.velocidadZ);

        if (va > 0.5f && vb > 0.5f)
        {
            float coseno = (m.velocidadVistaX * n.velocidadX + m.velocidadVistaZ * n.velocidadZ) / (va * vb);

            if (coseno < 0.93f)
            {
                float cx = 0.0f;
                float cz = 0.0f;
                PuntoMasCercanoObstaculoDefensa(m.anguloObstaculo, n.x, n.z, cx, cz);
                float dx = n.x - cx;
                float dz = n.z - cz;

                if (dx * dx + dz * dz < 1.1f * 1.1f)
                {
                    m.destelloObstaculo = 1.0f;
                    EmitirChispasDefensa(m, { (n.x + cx) * 0.5f, 0.5f, (n.z + cz) * 0.5f }, Color{ 255, 120, 214, 255 }, 9, 3.6f, true);
                }
                else
                {
                    EmitirChispasDefensa(m, posNucleo, Color{ 140, 236, 255, 255 }, 5, 2.6f, true);
                }
            }
        }
    }

    m.nucleoVisto = n.enJuego;
    m.velocidadVistaX = n.velocidadX;
    m.velocidadVistaZ = n.velocidadZ;
    m.cargaVista = n.tiempoCarga;

    // Estela del nucleo: muestras cada 25 ms.
    if (n.enJuego)
    {
        m.relojEstela += dt;

        if (m.relojEstela >= 0.025f)
        {
            m.relojEstela = 0.0f;

            for (int k = LARGO_ESTELA_DEFENSA_NUCLEO - 1; k > 0; k--)
            {
                m.estela[k] = m.estela[k - 1];
            }

            m.estela[0] = posNucleo;

            if (m.cantidadEstela < LARGO_ESTELA_DEFENSA_NUCLEO)
            {
                m.cantidadEstela++;
            }
        }

        // El nucleo cargado suelta chispas.
        if (n.tiempoCarga > 0.0f && std::fmod(m.tiempoAnimacion, 0.05f) < dt)
        {
            EmitirParticulaDefensa(
                m,
                posNucleo,
                { AzarDefensa(m, -1.0f, 1.0f), AzarDefensa(m, 0.8f, 2.0f), AzarDefensa(m, -1.0f, 1.0f) },
                AzarDefensa(m, 0.25f, 0.45f),
                AzarDefensa(m, 0.06f, 0.1f),
                4.0f,
                Color{ 255, 150, 60, 255 },
                true
            );
        }
    }
    else
    {
        m.cantidadEstela = 0;
    }

    // Ambiente: chispas de energia que suben de los postes de los arcos.
    if (decoracion && std::fmod(m.tiempoAnimacion, 0.16f) < dt)
    {
        float lado = AzarDefensa(m, 0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
        float poste = AzarDefensa(m, 0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
        int equipo = lado < 0.0f ? 0 : 1;

        EmitirParticulaDefensa(
            m,
            { lado * LIMITE_X_DEFENSA, ALTO_POSTE_DEFENSA + 0.15f, poste * MITAD_ARCO_DEFENSA },
            { AzarDefensa(m, -0.2f, 0.2f), AzarDefensa(m, 0.6f, 1.2f), AzarDefensa(m, -0.2f, 0.2f) },
            AzarDefensa(m, 0.6f, 1.0f),
            AzarDefensa(m, 0.05f, 0.08f),
            -0.3f,
            MezclaColorDefensa(ObtenerColorEquipoDefensa(equipo), WHITE, 0.35f),
            true
        );
    }

    // Final: papelitos sobre el equipo ganador y fuegos sobre la ciudad.
    if (m.fase == FASE_DEFENSA_NUCLEO_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        bool primeraVez = !m.finVisto;
        m.finVisto = true;

        if (primeraVez)
        {
            m.festejo = 1.0f;
        }

        if (primeraVez || std::fmod(m.tiempoAnimacion, 0.55f) < dt)
        {
            for (int i = 0; i < limite; i++)
            {
                int equipo = m.estadosJugadores[i].equipo;

                if (equipo < 0 || (!m.empate && equipo != m.equipoGanador))
                {
                    continue;
                }

                Vector3 p = jugadores[i].posicion;
                EmitirConfetiDefensa(m, { p.x, 2.6f, p.z }, { 0.0f, 3.4f, 0.0f }, ObtenerColorEquipoDefensa(equipo), primeraVez ? 30 : 8);
            }
        }

        if (decoracion && (primeraVez || std::fmod(m.tiempoAnimacion, 0.8f) < dt))
        {
            float lado = AzarDefensa(m, 0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
            Color color = m.empate
                ? Color{ 255, 214, 92, 255 }
                : ObtenerColorEquipoDefensa(m.equipoGanador);

            if (AzarDefensa(m, 0.0f, 1.0f) < 0.35f)
            {
                color = Color{ 255, 214, 92, 255 };
            }

            EmitirFuegoArtificialDefensa(
                m,
                { lado * AzarDefensa(m, 19.0f, 25.0f), AzarDefensa(m, -3.0f, 1.0f), AzarDefensa(m, -24.0f, -18.0f) },
                color,
                26
            );
        }

        m.festejo = m.festejo < 0.6f ? 0.6f : m.festejo;
    }

    // La tribuna se agita cuando el nucleo ronda un arco.
    if (n.enJuego && AbsDefensa(n.x) > 7.5f)
    {
        float tension = (AbsDefensa(n.x) - 7.5f) / 3.5f * 0.45f;
        if (m.festejo < tension) m.festejo = tension;
    }

    for (int i = 0; i < MAX_PARTICULAS_DEFENSA_NUCLEO; i++)
    {
        ParticulaDefensaNucleo& p = m.particulas[i];

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

        float freno = 1.0f - Limitar01Defensa(dt * (p.brillo ? 1.6f : 0.9f));
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.velocidad.y -= p.gravedad * dt;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        // Los papelitos se posan en la cancha.
        if (!p.brillo && p.posicion.y < 0.06f && p.velocidad.y < 0.0f && AbsDefensa(p.posicion.x) < 11.4f)
        {
            p.posicion.y = 0.06f;
            p.velocidad = { 0.0f, 0.0f, 0.0f };
            p.gravedad = 0.0f;
        }
    }
}


//==================================================
// PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Estadio en la azotea de un rascacielos, de noche. La cancha es de
// resina azul con lineas de luz cian; la azotea, de hormigon gris azulado;
// la ciudad de abajo se hunde en una bruma violeta que la ilumina desde
// las calles. El dorado queda reservado para el nucleo (el acento); rojo
// y azul solo marcan las zonas de cada equipo (arcos, areas y bancos).

static const Color COLOR_NIEBLA_DEFENSA = { 24, 22, 56, 255 };
static const Color COLOR_BRUMA_DEFENSA = { 74, 46, 108, 255 };
static const Color COLOR_CIELO_ALTO_DEFENSA = { 6, 8, 26, 255 };
static const Color COLOR_CIELO_MEDIO_DEFENSA = { 28, 22, 68, 255 };
static const Color COLOR_CIELO_BAJO_DEFENSA = { 96, 44, 112, 255 };
static const Color COLOR_CANCHA_DEFENSA = { 62, 74, 124, 255 };
static const Color COLOR_JUNTA_DEFENSA = { 32, 38, 74, 255 };
static const Color COLOR_LINEA_DEFENSA = { 186, 244, 255, 255 };
static const Color COLOR_CIAN_DEFENSA = { 70, 226, 255, 255 };
static const Color COLOR_ROSA_DEFENSA = { 255, 84, 196, 255 };
static const Color COLOR_VIOLETA_DEFENSA = { 166, 110, 255, 255 };
static const Color COLOR_NARANJA_DEFENSA = { 255, 168, 70, 255 };
static const Color COLOR_AZOTEA_DEFENSA = { 84, 88, 108, 255 };
static const Color COLOR_METAL_DEFENSA = { 150, 158, 184, 255 };
static const Color COLOR_METAL_OSCURO_DEFENSA = { 60, 64, 88, 255 };
static const Color COLOR_VALLA_DEFENSA = { 36, 40, 74, 255 };
static const Color COLOR_NUCLEO_DEFENSA = { 255, 210, 80, 255 };        // acento
static const Color COLOR_NUCLEO_CARGA_DEFENSA = { 255, 132, 48, 255 };
static const Color COLOR_FACHADA_DEFENSA = { 40, 44, 72, 255 };
static const Color COLOR_FOCO_DEFENSA = { 214, 232, 255, 255 };
static const Color COLOR_VIDRIO_DEFENSA = { 130, 206, 255, 255 };

// Ojo de referencia para la niebla (el de la camara de juego).
static const Vector3 OJO_NIEBLA_DEFENSA = { 0.0f, 16.5f, 16.5f };


static unsigned char CanalDefensa(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoDefensa(Color color, float factor)
{
    return Color{
        CanalDefensa((float)color.r * factor),
        CanalDefensa((float)color.g * factor),
        CanalDefensa((float)color.b * factor),
        color.a
    };
}


static Color MezclaColorDefensa(Color a, Color b, float t)
{
    t = Limitar01Defensa(t);

    return Color{
        CanalDefensa((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalDefensa((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalDefensa((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalDefensa((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color ConAlfaDefensa(Color color, float alfa)
{
    color.a = CanalDefensa(alfa);
    return color;
}


// Las caras de arriba reciben el cielo frio; las laterales, el rebote
// violeta de la ciudad.
static Color SombraDefensa(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalDefensa((float)color.r * factor * 0.97f),
            CanalDefensa((float)color.g * factor * 1.0f),
            CanalDefensa((float)color.b * factor * 1.06f),
            color.a
        };
    }

    return Color{
        CanalDefensa((float)color.r * factor * 1.0f),
        CanalDefensa((float)color.g * factor * 0.92f),
        CanalDefensa((float)color.b * factor * 1.06f),
        color.a
    };
}


static float DistanciaOjoDefensa(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_DEFENSA.x;
    float dy = p.y - OJO_NIEBLA_DEFENSA.y;
    float dz = p.z - OJO_NIEBLA_DEFENSA.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}


// Niebla cercana: lo que esta lejos del estadio se apaga hacia el azul noche.
static Color TinteDefensa(Color color, Vector3 p)
{
    float n = (DistanciaOjoDefensa(p) - 30.0f) / 55.0f;
    Color niebla = COLOR_NIEBLA_DEFENSA;
    niebla.a = color.a;
    return MezclaColorDefensa(color, niebla, n < 0.0f ? 0.0f : (n > 0.5f ? 0.5f : n));
}


// La ciudad: cuanto mas lejos y mas abajo, mas bruma violeta.
static Color TinteLejosDefensa(Color color, Vector3 p)
{
    float n = (DistanciaOjoDefensa(p) - 40.0f) / 190.0f;
    if (p.y < -15.0f) n += (-15.0f - p.y) / 160.0f;
    Color bruma = COLOR_BRUMA_DEFENSA;
    bruma.a = color.a;
    return MezclaColorDefensa(color, bruma, n < 0.0f ? 0.0f : (n > 0.86f ? 0.86f : n));
}


static void VerticeDefensa(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadDefensa(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeDefensa(a, color);
    VerticeDefensa(b, color);
    VerticeDefensa(c, color);
    VerticeDefensa(d, color);
}


static void QuadLejosDefensa(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeDefensa(a, TinteLejosDefensa(color, a));
    VerticeDefensa(b, TinteLejosDefensa(color, b));
    VerticeDefensa(c, TinteLejosDefensa(color, c));
    VerticeDefensa(d, TinteLejosDefensa(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.54). Respeta la matriz actual.
static void CuboDefensa(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadDefensa({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraDefensa(color, 1.0f, true));
    QuadDefensa({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraDefensa(color, 0.78f, false));
    QuadDefensa({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraDefensa(color, 0.54f, false));
    QuadDefensa({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraDefensa(color, 0.62f, false));
    QuadDefensa({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraDefensa(color, 0.70f, false));
    QuadDefensa({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, SombraDefensa(color, 0.45f, false));
    rlEnd();
}


static void CuboNieblaDefensa(Vector3 c, Vector3 t, Color color)
{
    CuboDefensa(c, t, TinteDefensa(color, c));
}


// Caja apoyada (c.y es la base) y girada sobre Y.
static void CuboGiradoDefensa(Vector3 base, Vector3 t, float giroGrados, Color color)
{
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboDefensa({ 0.0f, t.y * 0.5f, 0.0f }, t, TinteDefensa(color, base));
    rlPopMatrix();
}


static float LuzLateralDefensa(float angulo)
{
    return 0.68f + 0.12f * std::sin(angulo) - 0.10f * std::cos(angulo);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (r1).
static void PrismaDefensa(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    float giro = 0.0f
)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = SombraDefensa(color, 1.0f, true);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        Color cara = SombraDefensa(color, LuzLateralDefensa(a0 + paso * 0.5f), false);
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadDefensa(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadDefensa(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }
    }

    rlEnd();
}


// Tubo sombreado entre dos puntos (barras, marcos, patas).
static void TuboDefensa(Vector3 a, Vector3 b, float radio, int lados, Color color)
{
    Vector3 eje = { b.x - a.x, b.y - a.y, b.z - a.z };
    float largo = std::sqrt(eje.x * eje.x + eje.y * eje.y + eje.z * eje.z);

    if (largo < 0.0001f)
    {
        return;
    }

    eje = { eje.x / largo, eje.y / largo, eje.z / largo };
    Vector3 ref = AbsDefensa(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = {
        eje.y * ref.z - eje.z * ref.y,
        eje.z * ref.x - eje.x * ref.z,
        eje.x * ref.y - eje.y * ref.x
    };
    float lu = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
    u = { u.x / lu, u.y / lu, u.z / lu };
    Vector3 v = {
        eje.y * u.z - eje.z * u.y,
        eje.z * u.x - eje.x * u.z,
        eje.x * u.y - eje.y * u.x
    };

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        float am = (a0 + a1) * 0.5f;
        Vector3 d0 = { u.x * std::cos(a0) + v.x * std::sin(a0), u.y * std::cos(a0) + v.y * std::sin(a0), u.z * std::cos(a0) + v.z * std::sin(a0) };
        Vector3 d1 = { u.x * std::cos(a1) + v.x * std::sin(a1), u.y * std::cos(a1) + v.y * std::sin(a1), u.z * std::cos(a1) + v.z * std::sin(a1) };
        Vector3 dm = { u.x * std::cos(am) + v.x * std::sin(am), u.y * std::cos(am) + v.y * std::sin(am), u.z * std::cos(am) + v.z * std::sin(am) };
        float f = 0.66f + 0.28f * dm.y + 0.10f * dm.z - 0.05f * dm.x;
        Color cara = TonoDefensa(color, f);

        QuadDefensa(
            { a.x + d0.x * radio, a.y + d0.y * radio, a.z + d0.z * radio },
            { a.x + d1.x * radio, a.y + d1.y * radio, a.z + d1.z * radio },
            { b.x + d1.x * radio, b.y + d1.y * radio, b.z + d1.z * radio },
            { b.x + d0.x * radio, b.y + d0.y * radio, b.z + d0.z * radio },
            cara
        );
    }

    rlEnd();
}


// Esfera con luz falsa desde arriba y el frente.
static void EsferaDefensa(Vector3 c, float radio, Color color, int anillos = 8, int gajos = 12)
{
    rlBegin(RL_QUADS);

    for (int i = 0; i < anillos; i++)
    {
        float t0 = 3.1416f * (float)i / (float)anillos - 1.5708f;
        float t1 = 3.1416f * (float)(i + 1) / (float)anillos - 1.5708f;

        for (int j = 0; j < gajos; j++)
        {
            float p0 = 6.2832f * (float)j / (float)gajos;
            float p1 = 6.2832f * (float)(j + 1) / (float)gajos;
            Vector3 n[4] = {
                { std::cos(t0) * std::cos(p0), std::sin(t0), std::cos(t0) * std::sin(p0) },
                { std::cos(t0) * std::cos(p1), std::sin(t0), std::cos(t0) * std::sin(p1) },
                { std::cos(t1) * std::cos(p1), std::sin(t1), std::cos(t1) * std::sin(p1) },
                { std::cos(t1) * std::cos(p0), std::sin(t1), std::cos(t1) * std::sin(p0) }
            };

            for (int k = 0; k < 4; k++)
            {
                float luz = 0.5f + 0.32f * n[k].y + 0.22f * n[k].z - 0.08f * n[k].x;
                VerticeDefensa(
                    { c.x + n[k].x * radio, c.y + n[k].y * radio, c.z + n[k].z * radio },
                    TonoDefensa(color, luz)
                );
            }
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloDefensa(Vector3 c, float r0, float r1, Color color, int lados = 32, float desde = 0.0f, float hasta = 6.2832f)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = desde + (hasta - desde) * (float)k / (float)lados;
        float a1 = desde + (hasta - desde) * (float)(k + 1) / (float)lados;

        QuadDefensa(
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
static void ResplandorDefensa(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloDefensa(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorDefensa(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano perpendicular a la vista).
static void ResplandorFrenteDefensa(Vector3 c, float radio, Color color)
{
    ResplandorDefensa(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.73f, -0.68f }, radio, color);
}


// Franja plana sobre el suelo entre dos puntos (x, z).
static void FranjaDefensa(Vector2 a, Vector2 b, float ancho, float y, Color color)
{
    float dx = b.x - a.x;
    float dz = b.y - a.y;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.0001f)
    {
        return;
    }

    float nx = -dz / largo * ancho * 0.5f;
    float nz = dx / largo * ancho * 0.5f;

    rlBegin(RL_QUADS);
    QuadDefensa(
        { a.x + nx, y, a.y + nz },
        { b.x + nx, y, b.y + nz },
        { b.x - nx, y, b.y - nz },
        { a.x - nx, y, a.y - nz },
        color
    );
    rlEnd();
}


static void FranjaDiscontinuaDefensa(Vector2 a, Vector2 b, float ancho, float y, Color color, float trazo = 0.45f, float hueco = 0.3f)
{
    float dx = b.x - a.x;
    float dz = b.y - a.y;
    float largo = std::sqrt(dx * dx + dz * dz);

    for (float s = 0.0f; s < largo; s += trazo + hueco)
    {
        float s1 = s + trazo < largo ? s + trazo : largo;
        FranjaDefensa(
            { a.x + dx * s / largo, a.y + dz * s / largo },
            { a.x + dx * s1 / largo, a.y + dz * s1 / largo },
            ancho,
            y,
            color
        );
    }
}


//==================================================
// GEOMETRIA DE LA CANCHA (para el dibujo)
//==================================================

// Vertice k (0..7) del octogono de la cancha desplazado d hacia afuera.
// Coincide con las paredes y esquinas cortadas de la logica (d = 0).
static Vector2 VerticeContornoDefensa(int k, float d, float chaflan)
{
    float diagonal = LIMITE_X_DEFENSA + LIMITE_Z_DEFENSA - chaflan + d * RAIZ_DOS_DEFENSA;
    float ax = diagonal - (LIMITE_Z_DEFENSA + d);
    float az = diagonal - (LIMITE_X_DEFENSA + d);
    float bx = LIMITE_X_DEFENSA + d;
    float bz = LIMITE_Z_DEFENSA + d;

    switch (k & 7)
    {
        case 0: return { ax, -bz };
        case 1: return { bx, -az };
        case 2: return { bx, az };
        case 3: return { ax, bz };
        case 4: return { -ax, bz };
        case 5: return { -bx, az };
        case 6: return { -bx, -az };
        default: return { -ax, -bz };
    }
}


// Recorta un poligono convexo (x, z) con las cuatro esquinas cortadas.
static int RecortarEsquinasDefensa(Vector2* puntos, int cantidad, float diagonal)
{
    Vector2 temporal[16];

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            int m = 0;

            for (int i = 0; i < cantidad && m < 14; i++)
            {
                Vector2 a = puntos[i];
                Vector2 b = puntos[(i + 1) % cantidad];
                float da = (float)sx * a.x + (float)sz * a.y - diagonal;
                float db = (float)sx * b.x + (float)sz * b.y - diagonal;

                if (da <= 0.0f)
                {
                    temporal[m++] = a;
                }

                if ((da < 0.0f && db > 0.0f) || (da > 0.0f && db < 0.0f))
                {
                    float u = da / (da - db);
                    temporal[m++] = { a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u };
                }
            }

            for (int i = 0; i < m; i++)
            {
                puntos[i] = temporal[i];
            }

            cantidad = m;

            if (cantidad < 3)
            {
                return 0;
            }
        }
    }

    return cantidad;
}


// Media altura util de la cancha en x (borde lateral o esquina cortada).
static float MedioAnchoCanchaDefensa(float x, float chaflan, float margen)
{
    float diagonal = LIMITE_X_DEFENSA + LIMITE_Z_DEFENSA - chaflan - margen * RAIZ_DOS_DEFENSA;
    float lado = LIMITE_Z_DEFENSA - margen;
    float esquina = diagonal - AbsDefensa(x);
    return esquina < lado ? esquina : lado;
}


//==================================================
// CIELO (2D, antes del 3D)
//==================================================

static void DibujarCieloDefensa(float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int medio = (int)((float)alto * 0.42f);

    // Arriba la noche; abajo, la bruma que levanta la ciudad encendida.
    DrawRectangleGradientV(0, 0, ancho, medio, COLOR_CIELO_ALTO_DEFENSA, COLOR_CIELO_MEDIO_DEFENSA);
    DrawRectangleGradientV(0, medio, ancho, alto - medio, COLOR_CIELO_MEDIO_DEFENSA, COLOR_CIELO_BAJO_DEFENSA);

    if (!detalle)
    {
        return;
    }

    for (int i = 0; i < 120; i++)
    {
        float x = Ruido01Defensa(i, 11) * (float)ancho;
        float v = Ruido01Defensa(i, 12);
        float y = v * v * (float)alto * 0.42f;
        float titila = 0.55f + 0.45f * std::sin(t * (1.0f + Ruido01Defensa(i, 13) * 2.5f) + (float)i);
        float alfa = 210.0f * titila * (1.0f - y / ((float)alto * 0.45f));
        int lado = Ruido01Defensa(i, 14) > 0.85f ? 2 : 1;
        DrawRectangle((int)x, (int)y, lado, lado, ConAlfaDefensa(Color{ 236, 236, 255, 255 }, alfa));
    }

    // Luna llena con halo.
    Vector2 luna = { (float)ancho * 0.9f, (float)alto * 0.14f };
    float r = (float)alto * 0.05f;
    DrawCircleV(luna, r * 2.8f, ConAlfaDefensa(Color{ 120, 110, 210, 255 }, 16.0f));
    DrawCircleV(luna, r * 1.8f, ConAlfaDefensa(Color{ 160, 150, 230, 255 }, 28.0f));
    DrawCircleV(luna, r, Color{ 238, 234, 218, 255 });
    DrawCircleV({ luna.x - r * 0.3f, luna.y - r * 0.2f }, r * 0.24f, Color{ 214, 208, 194, 255 });
    DrawCircleV({ luna.x + r * 0.35f, luna.y + r * 0.25f }, r * 0.17f, Color{ 214, 208, 194, 255 });
    DrawCircleV({ luna.x + r * 0.1f, luna.y - r * 0.5f }, r * 0.1f, Color{ 220, 214, 200, 255 });

    // Jirones de nube que cruzan delante de la luna.
    for (int k = 0; k < 4; k++)
    {
        float y = (float)alto * (0.07f + 0.075f * (float)k);
        float x = std::fmod(t * (5.0f + 3.0f * (float)k) + (float)k * 410.0f, (float)ancho + 700.0f) - 350.0f;
        DrawEllipse((int)x, (int)y, 240.0f + 40.0f * (float)k, 9.0f + 2.0f * (float)k, ConAlfaDefensa(Color{ 84, 66, 132, 255 }, 46.0f));
    }
}


//==================================================
// HORIZONTE: CIUDAD, LETREROS Y AUTOS VOLADORES
//==================================================

struct EdificioDefensa
{
    float x = 0.0f;
    float z = 0.0f;
    float ancho = 6.0f;
    float fondo = 6.0f;
    float techo = -10.0f;
    float distancia = 0.0f;
    int semilla = 0;
};


static const int CIUDAD_MIN_I_DEFENSA = -7;
static const int CIUDAD_MAX_I_DEFENSA = 7;
static const int CIUDAD_MIN_J_DEFENSA = -13;
static const int CIUDAD_MAX_J_DEFENSA = 0;
static const float BASE_CIUDAD_DEFENSA = -175.0f;


// Manzana (i, j): devuelve false si alli esta el propio rascacielos.
static bool EdificioCiudadDefensa(int i, int j, EdificioDefensa& e)
{
    int s = (i + 20) * 97 + (j + 30) * 13;
    e.semilla = s;
    e.x = (float)i * 15.0f + (Ruido01Defensa(s, 1) - 0.5f) * 5.0f;
    e.z = (float)j * 15.0f + (Ruido01Defensa(s, 2) - 0.5f) * 5.0f;

    if (e.z > -24.0f && AbsDefensa(e.x) < 26.0f)
    {
        return false;
    }

    if (j == 0 && AbsDefensa(e.x) < 30.0f)
    {
        return false;
    }

    e.distancia = std::sqrt(e.x * e.x + e.z * e.z);
    e.ancho = 6.0f + Ruido01Defensa(s, 3) * 5.0f;
    e.fondo = 6.0f + Ruido01Defensa(s, 4) * 5.0f;

    float h = Ruido01Defensa(s, 5);
    e.techo = -9.0f - h * 34.0f;

    if (e.distancia > 70.0f && Ruido01Defensa(s, 6) > 0.86f)
    {
        e.techo = -4.0f + h * 14.0f;     // torres mas altas que nuestra azotea
    }

    if (e.distancia > 110.0f)
    {
        e.techo += 8.0f;
    }

    return true;
}


static Color ColorVentanaDefensa(int semilla)
{
    float v = Ruido01Defensa(semilla, 41);
    if (v < 0.6f) return Color{ 255, 212, 140, 255 };   // calida
    if (v < 0.93f) return Color{ 150, 220, 255, 255 };   // fria
    return Color{ 255, 120, 210, 255 };                  // neon
}


static Color ColorLetreroDefensa(int semilla)
{
    switch ((int)(Ruido01Defensa(semilla, 51) * 4.0f))
    {
        case 0: return COLOR_CIAN_DEFENSA;
        case 1: return COLOR_ROSA_DEFENSA;
        case 2: return COLOR_VIOLETA_DEFENSA;
        default: return COLOR_NARANJA_DEFENSA;
    }
}


// Ventanas de una cara: filas de focos encendidos al azar fijo.
static void VentanasCaraDefensa(
    Vector3 origen,      // esquina superior izquierda de la cara
    Vector3 dirAncho,    // a lo ancho de la cara (unitario)
    Vector3 normal,      // hacia afuera
    float ancho,
    float techo,
    float pisos,
    int semilla,
    int paso
)
{
    int columnas = (int)(ancho / 1.3f);

    if (columnas < 2)
    {
        return;
    }

    float hueco = ancho / (float)columnas;

    for (int f = 0; f < (int)pisos; f += paso)
    {
        float y1 = techo - 1.0f - (float)f * 1.5f;
        float y0 = y1 - 0.55f;

        for (int c = 0; c < columnas; c++)
        {
            int clave = semilla * 7 + f * 31 + c * 3;

            if (Ruido01Defensa(clave, 40) > 0.42f)
            {
                continue;
            }

            float a0 = hueco * ((float)c + 0.3f);
            float a1 = hueco * ((float)c + 0.7f);
            Vector3 p0 = { origen.x + dirAncho.x * a0 + normal.x * 0.03f, y0, origen.z + dirAncho.z * a0 + normal.z * 0.03f };
            Vector3 p1 = { origen.x + dirAncho.x * a1 + normal.x * 0.03f, y0, origen.z + dirAncho.z * a1 + normal.z * 0.03f };
            Color color = ColorVentanaDefensa(clave);

            QuadLejosDefensa(p0, p1, { p1.x, y1, p1.z }, { p0.x, y1, p0.z }, color);
        }
    }
}


static void DibujarEdificioDefensa(const EdificioDefensa& e, int paso, bool ventanas)
{
    float x0 = e.x - e.ancho * 0.5f, x1 = e.x + e.ancho * 0.5f;
    float z0 = e.z - e.fondo * 0.5f, z1 = e.z + e.fondo * 0.5f;
    float y0 = BASE_CIUDAD_DEFENSA;
    float y1 = e.techo;
    float tono = 0.85f + 0.3f * Ruido01Defensa(e.semilla, 7);
    Color fachada = TonoDefensa(COLOR_FACHADA_DEFENSA, tono);

    if (Ruido01Defensa(e.semilla, 8) > 0.7f)
    {
        fachada = MezclaColorDefensa(fachada, Color{ 60, 40, 84, 255 }, 0.5f);   // vidrio violeta
    }

    rlBegin(RL_QUADS);

    // Techo, frente (+Z) y el lateral que mira hacia la azotea.
    QuadLejosDefensa({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraDefensa(fachada, 1.15f, true));
    QuadLejosDefensa({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraDefensa(fachada, 0.82f, false));

    if (e.x > 0.0f)
    {
        QuadLejosDefensa({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraDefensa(fachada, 0.66f, false));
    }
    else
    {
        QuadLejosDefensa({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraDefensa(fachada, 0.66f, false));
    }

    // Cornisa clara en el borde del techo.
    QuadLejosDefensa({ x0, y1 - 0.5f, z1 + 0.02f }, { x1, y1 - 0.5f, z1 + 0.02f }, { x1, y1, z1 + 0.02f }, { x0, y1, z1 + 0.02f }, TonoDefensa(fachada, 1.5f));

    if (ventanas)
    {
        float pisos = 16.0f;
        VentanasCaraDefensa({ x0, y1, z1 }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, e.ancho, y1, pisos, e.semilla, paso);

        if (e.x > 0.0f)
        {
            VentanasCaraDefensa({ x0, y1, z1 }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, e.fondo, y1, pisos * 0.7f, e.semilla + 5, paso);
        }
        else
        {
            VentanasCaraDefensa({ x1, y1, z1 }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, e.fondo, y1, pisos * 0.7f, e.semilla + 5, paso);
        }
    }
    else
    {
        // Lejos: franjas de pisos encendidos.
        for (int f = 0; f < 3; f++)
        {
            float y = y1 - 2.0f - (float)f * 4.5f - Ruido01Defensa(e.semilla, 20 + f) * 2.0f;
            Color color = ConAlfaDefensa(ColorVentanaDefensa(e.semilla + f), 255.0f);
            QuadLejosDefensa({ x0 + 0.4f, y, z1 + 0.03f }, { x1 - 0.4f, y, z1 + 0.03f }, { x1 - 0.4f, y + 0.6f, z1 + 0.03f }, { x0 + 0.4f, y + 0.6f, z1 + 0.03f }, TonoDefensa(color, 0.8f));
        }
    }

    rlEnd();
}


// Letrero de neon en la terraza de un edificio cercano.
static bool LetreroEdificioDefensa(const EdificioDefensa& e)
{
    return e.distancia < 100.0f && e.techo < 0.0f && Ruido01Defensa(e.semilla, 30) < 0.5f;
}


static bool AntenaEdificioDefensa(const EdificioDefensa& e)
{
    return e.distancia < 120.0f && Ruido01Defensa(e.semilla, 31) < 0.45f;
}


static void DibujarTerrazaDefensa(const EdificioDefensa& e, float t)
{
    float y = e.techo;
    Vector3 c = { e.x, y, e.z };

    if (LetreroEdificioDefensa(e))
    {
        // MODELO FUTURO: letrero de neon de terraza.
        float w = e.ancho * 0.8f;
        float zf = e.z + e.fondo * 0.5f - 0.6f;
        Color metal = TinteLejosDefensa(COLOR_METAL_OSCURO_DEFENSA, c);
        TuboDefensa({ e.x - w * 0.4f, y, zf }, { e.x - w * 0.4f, y + 1.4f, zf }, 0.12f, 4, metal);
        TuboDefensa({ e.x + w * 0.4f, y, zf }, { e.x + w * 0.4f, y + 1.4f, zf }, 0.12f, 4, metal);
        CuboDefensa({ e.x, y + 2.6f, zf }, { w, 2.6f, 0.3f }, TinteLejosDefensa(Color{ 30, 26, 50, 255 }, c));

        // Cara encendida: dos bandas y un simbolo que parpadea.
        Color neon = ColorLetreroDefensa(e.semilla);
        float parpadeo = Ruido01Defensa(e.semilla, 32) < 0.25f && std::fmod(t * 1.7f + (float)e.semilla, 3.0f) < 0.12f ? 0.35f : 1.0f;
        Color cara = TinteLejosDefensa(TonoDefensa(neon, parpadeo), c);
        Color cara2 = TinteLejosDefensa(TonoDefensa(ColorLetreroDefensa(e.semilla + 1), parpadeo * 0.9f), c);
        float zc = zf + 0.17f;

        rlBegin(RL_QUADS);
        QuadDefensa({ e.x - w * 0.45f, y + 3.4f, zc }, { e.x + w * 0.45f, y + 3.4f, zc }, { e.x + w * 0.45f, y + 3.7f, zc }, { e.x - w * 0.45f, y + 3.7f, zc }, cara);
        QuadDefensa({ e.x - w * 0.45f, y + 1.5f, zc }, { e.x + w * 0.45f, y + 1.5f, zc }, { e.x + w * 0.45f, y + 1.8f, zc }, { e.x - w * 0.45f, y + 1.8f, zc }, cara);

        // Barras de «texto» que se desplazan.
        float corrido = std::fmod(t * 1.3f + (float)e.semilla * 0.37f, 1.0f);
        for (int k = 0; k < 5; k++)
        {
            float u = std::fmod((float)k / 5.0f + corrido, 1.0f);
            float xa = e.x - w * 0.4f + u * w * 0.8f;
            float largo = w * (0.06f + 0.08f * Ruido01Defensa(e.semilla + k, 33));
            float xb = xa + largo < e.x + w * 0.42f ? xa + largo : e.x + w * 0.42f;
            QuadDefensa({ xa, y + 2.15f, zc }, { xb, y + 2.15f, zc }, { xb, y + 3.05f, zc }, { xa, y + 3.05f, zc }, cara2);
        }

        rlEnd();
    }
    else if (Ruido01Defensa(e.semilla, 34) < 0.5f)
    {
        // Tanque de agua o caja de ascensores.
        CuboDefensa({ e.x + e.ancho * 0.15f, y + 1.0f, e.z }, { e.ancho * 0.35f, 2.0f, e.fondo * 0.35f }, TinteLejosDefensa(TonoDefensa(COLOR_FACHADA_DEFENSA, 1.2f), c));
    }

    if (AntenaEdificioDefensa(e))
    {
        float alto = 4.0f + Ruido01Defensa(e.semilla, 35) * 4.0f;
        TuboDefensa({ e.x - e.ancho * 0.25f, y, e.z }, { e.x - e.ancho * 0.25f, y + alto, e.z }, 0.1f, 4, TinteLejosDefensa(COLOR_METAL_DEFENSA, c));
    }
}


static void DibujarCiudadDefensa(float t, int paso, bool detalle)
{
    for (int j = CIUDAD_MIN_J_DEFENSA; j <= CIUDAD_MAX_J_DEFENSA; j++)
    {
        for (int i = CIUDAD_MIN_I_DEFENSA; i <= CIUDAD_MAX_I_DEFENSA; i++)
        {
            EdificioDefensa e;

            if (!EdificioCiudadDefensa(i, j, e))
            {
                continue;
            }

            bool cerca = e.distancia < 105.0f;

            if (!detalle && !cerca)
            {
                continue;
            }

            DibujarEdificioDefensa(e, paso, cerca && detalle);

            if (detalle && cerca)
            {
                DibujarTerrazaDefensa(e, t);
            }
        }
    }
}


// Carriles de autos voladores entre los edificios.
struct CarrilDefensa
{
    float fijo;          // z (carril en X) o x (carril en Z)
    float y;
    bool ejeX;
    float velocidad;
    int cantidad;
    float largo;
    float centro;
};


static const int CANTIDAD_CARRILES_DEFENSA = 9;

static const CarrilDefensa CARRILES_DEFENSA[CANTIDAD_CARRILES_DEFENSA] =
{
    { -22.0f, -5.5f, true, 7.0f, 3, 110.0f, 0.0f },
    { -27.0f, -9.5f, true, -10.0f, 3, 120.0f, 0.0f },
    { -44.0f, -15.0f, true, 13.0f, 4, 160.0f, 0.0f },
    { -21.8f, -2.0f, false, 5.5f, 3, 60.0f, -20.0f },
    { 21.8f, -3.0f, false, -6.5f, 3, 60.0f, -20.0f },
    { -26.0f, -7.0f, false, 6.0f, 2, 90.0f, -25.0f },
    { 27.0f, -8.5f, false, -8.0f, 2, 90.0f, -25.0f },
    { -33.0f, -12.0f, false, -9.0f, 2, 100.0f, -30.0f },
    { 34.0f, -2.5f, false, 7.5f, 2, 100.0f, -30.0f }
};


static Vector3 PosicionAutoDefensa(int carril, int k, float t)
{
    const CarrilDefensa& c = CARRILES_DEFENSA[carril];
    float s = std::fmod((float)k * c.largo / (float)c.cantidad + c.velocidad * t + Ruido01Defensa(carril, 60) * c.largo, c.largo);
    if (s < 0.0f) s += c.largo;
    s = s - c.largo * 0.5f + c.centro;
    float flote = 0.25f * std::sin(t * 1.3f + (float)(carril * 5 + k));

    return c.ejeX
        ? Vector3{ s, c.y + flote, c.fijo }
        : Vector3{ c.fijo, c.y + flote, s };
}


static Color ColorAutoDefensa(int carril, int k)
{
    switch ((carril * 3 + k) % 4)
    {
        case 0: return Color{ 226, 230, 240, 255 };
        case 1: return Color{ 255, 200, 70, 255 };    // taxi
        case 2: return Color{ 140, 104, 220, 255 };
        default: return Color{ 70, 190, 200, 255 };
    }
}


// Matriz del auto: +X local es el frente.
static void EntrarAutoDefensa(int carril, Vector3 p)
{
    const CarrilDefensa& c = CARRILES_DEFENSA[carril];
    float giro = c.ejeX ? (c.velocidad > 0.0f ? 0.0f : 180.0f) : (c.velocidad > 0.0f ? -90.0f : 90.0f);
    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
}


static void DibujarAutosVoladoresDefensa(float t)
{
    for (int c = 0; c < CANTIDAD_CARRILES_DEFENSA; c++)
    {
        for (int k = 0; k < CARRILES_DEFENSA[c].cantidad; k++)
        {
            // MODELO FUTURO: auto volador.
            Vector3 p = PosicionAutoDefensa(c, k, t);
            Color carroceria = TinteLejosDefensa(ColorAutoDefensa(c, k), p);
            Color vidrio = TinteLejosDefensa(Color{ 40, 70, 110, 255 }, p);
            Color oscuro = TinteLejosDefensa(Color{ 40, 42, 58, 255 }, p);

            EntrarAutoDefensa(c, p);
            CuboDefensa({ 0.0f, 0.0f, 0.0f }, { 2.0f, 0.42f, 0.9f }, carroceria);
            CuboDefensa({ -0.15f, 0.34f, 0.0f }, { 1.0f, 0.3f, 0.72f }, vidrio);
            CuboDefensa({ -0.85f, 0.1f, 0.0f }, { 0.35f, 0.5f, 1.3f }, oscuro);
            CuboDefensa({ 1.02f, 0.02f, 0.28f }, { 0.06f, 0.12f, 0.22f }, Color{ 255, 255, 236, 255 });
            CuboDefensa({ 1.02f, 0.02f, -0.28f }, { 0.06f, 0.12f, 0.22f }, Color{ 255, 255, 236, 255 });
            CuboDefensa({ -1.03f, 0.05f, 0.0f }, { 0.06f, 0.1f, 0.8f }, Color{ 255, 60, 70, 255 });
            rlPopMatrix();
        }
    }
}


//==================================================
// AZOTEA: LOSAS, BARANDAS DE VIDRIO Y PILONES
//==================================================

static void DibujarAzoteaDefensa(float chaflan)
{
    float y = BASE_AZOTEA_DEFENSA;
    float diagonalExterior = LIMITE_X_DEFENSA + LIMITE_Z_DEFENSA - chaflan + GROSOR_VALLA_DEFENSA * RAIZ_DOS_DEFENSA;

    // Losas de hormigon de 2 x 2 (las que quedan bajo la cancha no se dibujan).
    rlBegin(RL_QUADS);

    for (float x = -BORDE_AZOTEA_X_DEFENSA; x < BORDE_AZOTEA_X_DEFENSA - 0.01f; x += 2.0f)
    {
        for (float z = FONDO_AZOTEA_DEFENSA; z < FRENTE_AZOTEA_DEFENSA - 0.01f; z += 2.0f)
        {
            float x1 = x + 2.0f < BORDE_AZOTEA_X_DEFENSA ? x + 2.0f : BORDE_AZOTEA_X_DEFENSA;
            float z1 = z + 2.0f < FRENTE_AZOTEA_DEFENSA ? z + 2.0f : FRENTE_AZOTEA_DEFENSA;
            bool dentro = true;
            const float xs[2] = { x, x1 };
            const float zs[2] = { z, z1 };

            for (int a = 0; a < 2; a++)
            {
                for (int b = 0; b < 2; b++)
                {
                    if (
                        AbsDefensa(xs[a]) > LIMITE_X_DEFENSA + GROSOR_VALLA_DEFENSA ||
                        AbsDefensa(zs[b]) > LIMITE_Z_DEFENSA + GROSOR_VALLA_DEFENSA ||
                        AbsDefensa(xs[a]) + AbsDefensa(zs[b]) > diagonalExterior
                    )
                    {
                        dentro = false;
                    }
                }
            }

            if (dentro)
            {
                continue;
            }

            int clave = (int)((x + 40.0f) * 3.0f) * 131 + (int)((z + 40.0f) * 3.0f);
            float tono = 0.9f + 0.16f * Ruido01Defensa(clave, 70);
            Color losa = TinteDefensa(TonoDefensa(COLOR_AZOTEA_DEFENSA, tono), { x, y, z });
            Color junta = TinteDefensa(TonoDefensa(COLOR_AZOTEA_DEFENSA, 0.62f), { x, y, z });

            QuadDefensa({ x, y - 0.002f, z1 }, { x1, y - 0.002f, z1 }, { x1, y - 0.002f, z }, { x, y - 0.002f, z }, junta);
            QuadDefensa({ x + 0.05f, y, z1 - 0.05f }, { x1 - 0.05f, y, z1 - 0.05f }, { x1 - 0.05f, y, z + 0.05f }, { x + 0.05f, y, z + 0.05f }, losa);
        }
    }

    rlEnd();

    // Senda amarilla de seguridad alrededor de la cancha.
    Color senda = Color{ 196, 162, 64, 255 };

    for (int k = 0; k < 8; k++)
    {
        Vector2 a = VerticeContornoDefensa(k, 1.2f, chaflan);
        Vector2 b = VerticeContornoDefensa(k + 1, 1.2f, chaflan);
        FranjaDiscontinuaDefensa(a, b, 0.12f, y + 0.02f, senda, 0.8f, 0.5f);
    }

    // Losa de borde: el canto del edificio con el reflejo de la ciudad.
    Color canto = Color{ 70, 72, 94, 255 };
    CuboDefensa({ -BORDE_AZOTEA_X_DEFENSA - 0.3f, y - 0.8f, (FRENTE_AZOTEA_DEFENSA + FONDO_AZOTEA_DEFENSA) * 0.5f }, { 0.6f, 1.6f, FRENTE_AZOTEA_DEFENSA - FONDO_AZOTEA_DEFENSA }, canto);
    CuboDefensa({ BORDE_AZOTEA_X_DEFENSA + 0.3f, y - 0.8f, (FRENTE_AZOTEA_DEFENSA + FONDO_AZOTEA_DEFENSA) * 0.5f }, { 0.6f, 1.6f, FRENTE_AZOTEA_DEFENSA - FONDO_AZOTEA_DEFENSA }, canto);

    // Pretil de hormigon con postes de la baranda.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float xb = (float)lado * (BORDE_AZOTEA_X_DEFENSA - 0.2f);
        CuboNieblaDefensa({ xb, y + 0.22f, (FRENTE_AZOTEA_DEFENSA + FONDO_AZOTEA_DEFENSA) * 0.5f }, { 0.4f, 0.44f, FRENTE_AZOTEA_DEFENSA - FONDO_AZOTEA_DEFENSA }, Color{ 104, 108, 130, 255 });

        for (float z = FONDO_AZOTEA_DEFENSA + 0.5f; z < FRENTE_AZOTEA_DEFENSA; z += 2.0f)
        {
            CuboNieblaDefensa({ xb, y + 0.95f, z }, { 0.08f, 1.0f, 0.08f }, COLOR_METAL_DEFENSA);
        }

        CuboNieblaDefensa({ xb, y + 1.48f, (FRENTE_AZOTEA_DEFENSA + FONDO_AZOTEA_DEFENSA) * 0.5f }, { 0.12f, 0.08f, FRENTE_AZOTEA_DEFENSA - FONDO_AZOTEA_DEFENSA }, COLOR_METAL_DEFENSA);
    }

    CuboNieblaDefensa({ 0.0f, y + 0.22f, FRENTE_AZOTEA_DEFENSA - 0.2f }, { BORDE_AZOTEA_X_DEFENSA * 2.0f, 0.44f, 0.4f }, Color{ 104, 108, 130, 255 });
    CuboNieblaDefensa({ 0.0f, y + 0.22f, FONDO_AZOTEA_DEFENSA + 0.2f }, { BORDE_AZOTEA_X_DEFENSA * 2.0f, 0.44f, 0.4f }, Color{ 104, 108, 130, 255 });
}


//==================================================
// CANCHA: PISO, LINEAS DE LUZ, VALLAS Y PLATAFORMA GIRATORIA
//==================================================

static void DibujarPisoCanchaDefensa(float chaflan)
{
    float diagonal = LIMITE_X_DEFENSA + LIMITE_Z_DEFENSA - chaflan;

    // Base de junta: el octogono entero.
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < 8; k++)
    {
        Vector2 a = VerticeContornoDefensa(k, 0.0f, chaflan);
        Vector2 b = VerticeContornoDefensa(k + 1, 0.0f, chaflan);
        VerticeDefensa({ 0.0f, 0.0f, 0.0f }, COLOR_JUNTA_DEFENSA);
        VerticeDefensa({ b.x, 0.0f, b.y }, COLOR_JUNTA_DEFENSA);
        VerticeDefensa({ a.x, 0.0f, a.y }, COLOR_JUNTA_DEFENSA);
    }

    rlEnd();

    // Paneles de resina 1 x 1 con variacion; cada mitad tomada levemente
    // por el color de su equipo, la franja neutral mas oscura.
    rlBegin(RL_TRIANGLES);

    for (int ix = -11; ix < 11; ix++)
    {
        for (int iz = -7; iz < 7; iz++)
        {
            Vector2 p[16] = {
                { (float)ix + 0.03f, (float)iz + 0.03f },
                { (float)ix + 0.97f, (float)iz + 0.03f },
                { (float)ix + 0.97f, (float)iz + 0.97f },
                { (float)ix + 0.03f, (float)iz + 0.97f }
            };
            int n = RecortarEsquinasDefensa(p, 4, diagonal - 0.04f);

            if (n < 3)
            {
                continue;
            }

            float cx = (float)ix + 0.5f;
            float tono = 0.93f + 0.14f * Ruido01Defensa(ix * 53 + iz * 7, 80);
            if (((ix + iz) & 1) == 0) tono *= 0.97f;
            Color panel = TonoDefensa(COLOR_CANCHA_DEFENSA, tono);

            if (AbsDefensa(cx) < 1.0f)
            {
                panel = TonoDefensa(panel, 0.84f);
            }
            else
            {
                panel = MezclaColorDefensa(panel, ObtenerColorEquipoDefensa(cx < 0.0f ? 0 : 1), 0.08f);
            }

            for (int k = 1; k < n - 1; k++)
            {
                VerticeDefensa({ p[0].x, 0.02f, p[0].y }, panel);
                VerticeDefensa({ p[k + 1].x, 0.02f, p[k + 1].y }, panel);
                VerticeDefensa({ p[k].x, 0.02f, p[k].y }, panel);
            }
        }
    }

    rlEnd();
}


// Plataforma giratoria bajo el obstaculo: gira con el (sincronizada).
static void DibujarGiratoriaDefensa(float angulo, float destello)
{
    const int SECTORES = 12;
    Color a = TonoDefensa(COLOR_CANCHA_DEFENSA, 0.78f);
    Color b = TonoDefensa(COLOR_CANCHA_DEFENSA, 0.92f);
    a = MezclaColorDefensa(a, COLOR_ROSA_DEFENSA, destello * 0.25f);

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < SECTORES; k++)
    {
        float a0 = angulo + 6.2832f * (float)k / (float)SECTORES;
        float a1 = angulo + 6.2832f * (float)(k + 1) / (float)SECTORES;
        Color color = (k & 1) ? a : b;
        VerticeDefensa({ 0.0f, 0.03f, 0.0f }, color);
        VerticeDefensa({ std::cos(a1) * RADIO_GIRATORIA_DEFENSA, 0.03f, std::sin(a1) * RADIO_GIRATORIA_DEFENSA }, color);
        VerticeDefensa({ std::cos(a0) * RADIO_GIRATORIA_DEFENSA, 0.03f, std::sin(a0) * RADIO_GIRATORIA_DEFENSA }, color);
    }

    rlEnd();

    AnilloDefensa({ 0.0f, 0.035f, 0.0f }, 0.9f, 1.0f, TonoDefensa(COLOR_CANCHA_DEFENSA, 0.6f), 24);
}


// Lineas de la cancha. En la pasada normal se dibujan opacas; en la
// aditiva, mas anchas y tenues (el resplandor).
static void DibujarLineasCanchaDefensa(float chaflan, float ensanche, float alfa, float t, float flash, Color colorFlash)
{
    float y = 0.04f + ensanche * 0.02f;
    Color luz = ConAlfaDefensa(MezclaColorDefensa(COLOR_LINEA_DEFENSA, colorFlash, flash), alfa);
    Color cian = ConAlfaDefensa(MezclaColorDefensa(COLOR_CIAN_DEFENSA, colorFlash, flash), alfa);
    float ancho = 0.09f + ensanche;
    float zArco = MITAD_ARCO_DEFENSA + 0.35f;

    // Borde interior: octogono a 0.35 de la valla, abierto frente a los arcos.
    for (int k = 0; k < 8; k++)
    {
        Vector2 a = VerticeContornoDefensa(k, -0.35f, chaflan);
        Vector2 b = VerticeContornoDefensa(k + 1, -0.35f, chaflan);

        if (k == 1 || k == 5)
        {
            if (AbsDefensa(a.y) > zArco)
            {
                FranjaDefensa(a, { a.x, a.y < 0.0f ? -zArco : zArco }, ancho, y, luz);
                FranjaDefensa({ b.x, b.y < 0.0f ? -zArco : zArco }, b, ancho, y, luz);
            }

            continue;
        }

        FranjaDefensa(a, b, ancho, y, luz);
    }

    // Linea central (cortada por el circulo del obstaculo).
    float zCentro = MedioAnchoCanchaDefensa(0.0f, chaflan, 0.35f);
    FranjaDefensa({ 0.0f, -zCentro }, { 0.0f, -2.75f }, ancho, y, cian);
    FranjaDefensa({ 0.0f, 2.75f }, { 0.0f, zCentro }, ancho, y, cian);

    // Circulo del barrido del obstaculo (punta de la barra + su radio).
    float radio = MITAD_LARGO_OBSTACULO_DEFENSA + RADIO_OBSTACULO_DEFENSA;
    AnilloDefensa({ 0.0f, y, 0.0f }, radio - ancho * 0.5f, radio + ancho * 0.5f, cian, 48);

    // Franja neutral: nadie la pisa (cada equipo juega en su mitad).
    for (int s = -1; s <= 1; s += 2)
    {
        float xn = (float)s * 1.0f;
        float zn = MedioAnchoCanchaDefensa(xn, chaflan, 0.35f);
        FranjaDiscontinuaDefensa({ xn, -zn }, { xn, -2.85f }, ancho * 0.8f, y, luz, 0.35f, 0.25f);
        FranjaDiscontinuaDefensa({ xn, 2.85f }, { xn, zn }, ancho * 0.8f, y, luz, 0.35f, 0.25f);
    }

    // Por equipo: limite de su area (no pasan de 9.7), linea de gol y
    // contorno del area chica frente al arco.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        float lado = LadoArcoEquipoDefensa(equipo);
        Color color = ConAlfaDefensa(MezclaColorDefensa(MezclaColorDefensa(ObtenerColorEquipoDefensa(equipo), WHITE, 0.25f), colorFlash, flash), alfa);
        float xl = lado * (LIMITE_X_DEFENSA - 1.3f);
        float zl = MedioAnchoCanchaDefensa(xl, chaflan, 0.35f);

        FranjaDiscontinuaDefensa({ xl, -zl }, { xl, zl }, ancho, y, color, 0.5f, 0.3f);
        FranjaDefensa({ xl, -MITAD_ARCO_DEFENSA }, { lado * LIMITE_X_DEFENSA, -MITAD_ARCO_DEFENSA }, ancho, y, color);
        FranjaDefensa({ xl, MITAD_ARCO_DEFENSA }, { lado * LIMITE_X_DEFENSA, MITAD_ARCO_DEFENSA }, ancho, y, color);
        FranjaDefensa({ lado * LIMITE_X_DEFENSA, -MITAD_ARCO_DEFENSA }, { lado * LIMITE_X_DEFENSA, MITAD_ARCO_DEFENSA }, ancho + 0.08f, y, color);

        // Puntos de saque de cada mitad.
        for (int sz = -1; sz <= 1; sz += 2)
        {
            Vector3 punto = { lado * 5.5f, y, (float)sz * 3.6f };
            AnilloDefensa(punto, 0.32f - ensanche * 0.5f, 0.42f + ensanche * 0.5f, color, 20);
        }
    }

    // Pulso que recorre el borde (luz que corre).
    if (ensanche > 0.0f)
    {
        float recorrido = std::fmod(t * 0.35f, 1.0f);

        for (int k = 0; k < 8; k++)
        {
            if (k == 1 || k == 5) continue;

            Vector2 a = VerticeContornoDefensa(k, -0.35f, chaflan);
            Vector2 b = VerticeContornoDefensa(k + 1, -0.35f, chaflan);
            float u0 = std::fmod(recorrido * 8.0f - (float)k + 8.0f, 8.0f);

            if (u0 < 1.0f)
            {
                float u1 = u0 + 0.25f < 1.0f ? u0 + 0.25f : 1.0f;
                FranjaDefensa(
                    { a.x + (b.x - a.x) * u0, a.y + (b.y - a.y) * u0 },
                    { a.x + (b.x - a.x) * u1, a.y + (b.y - a.y) * u1 },
                    ancho * 2.0f,
                    y + 0.005f,
                    ConAlfaDefensa(COLOR_LINEA_DEFENSA, alfa * 3.0f)
                );
            }
        }
    }
}


// Tramo de valla con cara interior exactamente sobre el borde de la logica.
static void TramoVallaDefensa(Vector2 ai, Vector2 bi, Vector2 ao, Vector2 bo, float t, Color led, float flash)
{
    float y0 = BASE_AZOTEA_DEFENSA;
    float y1 = ALTO_VALLA_DEFENSA;
    float nx = ai.x - ao.x;
    float nz = ai.y - ao.y;
    float angulo = std::atan2(nz, nx);
    Color cara = SombraDefensa(COLOR_VALLA_DEFENSA, LuzLateralDefensa(angulo) + 0.1f, false);
    Color afuera = SombraDefensa(COLOR_VALLA_DEFENSA, LuzLateralDefensa(angulo + 3.1416f), false);
    Color tapa = SombraDefensa(COLOR_METAL_DEFENSA, 1.0f, true);

    rlBegin(RL_QUADS);
    QuadDefensa({ ai.x, y0, ai.y }, { bi.x, y0, bi.y }, { bi.x, y1, bi.y }, { ai.x, y1, ai.y }, cara);
    QuadDefensa({ ai.x, y1, ai.y }, { bi.x, y1, bi.y }, { bo.x, y1, bo.y }, { ao.x, y1, ao.y }, tapa);
    QuadDefensa({ bo.x, y0, bo.y }, { ao.x, y0, ao.y }, { ao.x, y1, ao.y }, { bo.x, y1, bo.y }, afuera);

    // Banda de LEDs en la cara interior, con un brillo que corre.
    float dx = bi.x - ai.x;
    float dz = bi.y - ai.y;
    float largo = std::sqrt(dx * dx + dz * dz);
    float largoN = std::sqrt(nx * nx + nz * nz);
    float ox = nx / largoN * 0.012f;
    float oz = nz / largoN * 0.012f;
    int tramos = (int)(largo / 0.5f) + 1;

    for (int k = 0; k < tramos; k++)
    {
        float u0 = (float)k / (float)tramos;
        float u1 = (float)(k + 1) / (float)tramos;
        float s = (ai.x + ai.y) + largo * u0;
        float onda = 0.5f + 0.5f * std::sin(s * 1.1f - t * 5.0f);
        float brillo = 0.45f + 0.55f * onda * onda * onda;
        Color c = MezclaColorDefensa(TonoDefensa(led, brillo), WHITE, flash * 0.4f);
        Vector3 p0 = { ai.x + dx * u0 + ox, 0.26f, ai.y + dz * u0 + oz };
        Vector3 p1 = { ai.x + dx * u1 + ox, 0.26f, ai.y + dz * u1 + oz };
        QuadDefensa(p0, p1, { p1.x, 0.38f, p1.z }, { p0.x, 0.38f, p0.z }, c);
    }

    rlEnd();
}


static void DibujarVallasDefensa(float chaflan, float t, float flash, Color colorFlash)
{
    Color led = MezclaColorDefensa(COLOR_CIAN_DEFENSA, colorFlash, flash);
    float g = GROSOR_VALLA_DEFENSA;

    for (int k = 0; k < 8; k++)
    {
        Vector2 ai = VerticeContornoDefensa(k, 0.0f, chaflan);
        Vector2 bi = VerticeContornoDefensa(k + 1, 0.0f, chaflan);
        Vector2 ao = VerticeContornoDefensa(k, g, chaflan);
        Vector2 bo = VerticeContornoDefensa(k + 1, g, chaflan);

        if (k == 1 || k == 5)
        {
            // Fondo con la boca del arco: dos tramos desde la esquina
            // hasta el centro de cada poste.
            float signoA = ai.y < 0.0f ? -1.0f : 1.0f;
            float signoB = bi.y < 0.0f ? -1.0f : 1.0f;
            TramoVallaDefensa(ai, { ai.x, signoA * MITAD_ARCO_DEFENSA }, ao, { ao.x, signoA * MITAD_ARCO_DEFENSA }, t, led, flash);
            TramoVallaDefensa({ bi.x, signoB * MITAD_ARCO_DEFENSA }, bi, { bo.x, signoB * MITAD_ARCO_DEFENSA }, bo, t, led, flash);
            continue;
        }

        TramoVallaDefensa(ai, bi, ao, bo, t, led, flash);
    }
}


//==================================================
// ARCOS DE ENERGIA
//==================================================

static Vector3 PuntoArcoEnergiaDefensa(float lado, float u)
{
    float a = u * 3.1416f;
    return {
        lado * LIMITE_X_DEFENSA,
        ALTO_POSTE_DEFENSA + 0.1f + std::sin(a) * 1.0f,
        -std::cos(a) * MITAD_ARCO_DEFENSA
    };
}


static void DibujarArcosDefensa(const MinijuegoDefensaNucleo& m, float t)
{
    for (int equipo = 0; equipo < 2; equipo++)
    {
        float lado = LadoArcoEquipoDefensa(equipo);
        int indice = lado < 0.0f ? 0 : 1;
        float destello = m.destelloArco[indice];
        Color color = ObtenerColorEquipoDefensa(equipo);
        Color claro = MezclaColorDefensa(color, WHITE, 0.35f + destello * 0.5f);
        float x0 = lado * LIMITE_X_DEFENSA;
        float x1 = lado * (LIMITE_X_DEFENSA + FONDO_ARCO_DEFENSA);
        float zLado = MITAD_ARCO_DEFENSA + RADIO_POSTE_DEFENSA;

        // Piso del bolsillo, oscuro con rejilla del color del equipo.
        rlBegin(RL_QUADS);
        Color pisoBolsillo = MezclaColorDefensa(Color{ 26, 28, 52, 255 }, color, 0.18f + destello * 0.3f);
        QuadDefensa({ x0, 0.015f, -zLado }, { x1, 0.015f, -zLado }, { x1, 0.015f, zLado }, { x0, 0.015f, zLado }, pisoBolsillo);
        rlEnd();

        for (int k = 1; k < 4; k++)
        {
            float x = lado * (LIMITE_X_DEFENSA + FONDO_ARCO_DEFENSA * (float)k / 4.0f);
            FranjaDefensa({ x, -zLado }, { x, zLado }, 0.05f, 0.03f, TonoDefensa(color, 0.7f));
        }

        for (int k = -2; k <= 2; k++)
        {
            float z = (float)k * MITAD_ARCO_DEFENSA * 0.4f;
            FranjaDefensa({ x0, z }, { x1, z }, 0.05f, 0.03f, TonoDefensa(color, 0.7f));
        }

        // Paredes del bolsillo (laterales y fondo).
        Color pared = MezclaColorDefensa(COLOR_VALLA_DEFENSA, color, 0.22f);
        float fondoX = lado * (LIMITE_X_DEFENSA + FONDO_ARCO_DEFENSA + 0.15f);
        CuboDefensa({ (x0 + x1) * 0.5f + lado * 0.2f, 0.25f, -(zLado + 0.15f) }, { FONDO_ARCO_DEFENSA + 0.4f, 1.1f, 0.3f }, pared);
        CuboDefensa({ (x0 + x1) * 0.5f + lado * 0.2f, 0.25f, zLado + 0.15f }, { FONDO_ARCO_DEFENSA + 0.4f, 1.1f, 0.3f }, pared);
        CuboDefensa({ fondoX, 0.45f, 0.0f }, { 0.3f, 1.5f, zLado * 2.0f + 0.6f }, pared);

        // Receptor en el fondo: anillo vertical que se enciende con el gol.
        rlPushMatrix();
        rlTranslatef(fondoX - lado * 0.17f, 0.75f, 0.0f);
        rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        AnilloDefensa({ 0.0f, 0.0f, 0.0f }, 0.42f, 0.56f, claro, 28);
        AnilloDefensa({ 0.0f, 0.0f, 0.0f }, 0.0f, 0.2f, MezclaColorDefensa(color, WHITE, 0.7f * destello), 16);
        rlPopMatrix();

        // Postes: cilindros del radio exacto de su colision.
        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 base = { x0, BASE_AZOTEA_DEFENSA, (float)s * MITAD_ARCO_DEFENSA };
            PrismaDefensa(base, RADIO_POSTE_DEFENSA, RADIO_POSTE_DEFENSA, ALTO_POSTE_DEFENSA - BASE_AZOTEA_DEFENSA, 12, Color{ 206, 212, 230, 255 });
            PrismaDefensa({ base.x, 0.18f, base.z }, RADIO_POSTE_DEFENSA + 0.015f, RADIO_POSTE_DEFENSA + 0.015f, 0.12f, 12, claro);
            PrismaDefensa({ base.x, 1.05f, base.z }, RADIO_POSTE_DEFENSA + 0.015f, RADIO_POSTE_DEFENSA + 0.015f, 0.12f, 12, claro);
            PrismaDefensa({ base.x, ALTO_POSTE_DEFENSA, base.z }, RADIO_POSTE_DEFENSA + 0.04f, RADIO_POSTE_DEFENSA * 0.6f, 0.14f, 12, COLOR_METAL_OSCURO_DEFENSA);
            EsferaDefensa({ base.x, ALTO_POSTE_DEFENSA + 0.24f, base.z }, 0.15f, MezclaColorDefensa(claro, WHITE, 0.4f + 0.2f * std::sin(t * 4.0f + (float)s)), 6, 10);
        }

        // Arco de energia: tubo luminoso de poste a poste.
        const int TRAMOS = 14;

        for (int k = 0; k < TRAMOS; k++)
        {
            Vector3 a = PuntoArcoEnergiaDefensa(lado, (float)k / (float)TRAMOS);
            Vector3 b = PuntoArcoEnergiaDefensa(lado, (float)(k + 1) / (float)TRAMOS);
            float pulso = 0.8f + 0.2f * std::sin(t * 6.0f - (float)k * 0.7f);
            TuboDefensa(a, b, 0.07f, 6, TonoDefensa(MezclaColorDefensa(claro, WHITE, 0.3f), 1.0f + 0.25f * pulso));
        }
    }
}


//==================================================
// OBSTACULO GIRATORIO
//==================================================

static void DibujarObstaculoDefensa(const MinijuegoDefensaNucleo& m, float t)
{
    float a = m.anguloObstaculo;
    float ex = std::cos(a) * MITAD_LARGO_OBSTACULO_DEFENSA;
    float ez = std::sin(a) * MITAD_LARGO_OBSTACULO_DEFENSA;
    float y = 0.42f;
    float destello = m.destelloObstaculo;
    Color cuerpo = MezclaColorDefensa(Color{ 222, 228, 244, 255 }, WHITE, destello * 0.5f);
    Color banda = MezclaColorDefensa(COLOR_ROSA_DEFENSA, WHITE, destello * 0.4f);

    DibujarGiratoriaDefensa(a, destello);

    // Barra: capsula del radio exacto de la colision, con bandas de peligro.
    const int TRAMOS = 10;

    for (int k = 0; k < TRAMOS; k++)
    {
        float u0 = (float)k / (float)TRAMOS * 2.0f - 1.0f;
        float u1 = (float)(k + 1) / (float)TRAMOS * 2.0f - 1.0f;
        bool peligro = k == 1 || k == 3 || k == 6 || k == 8;
        TuboDefensa({ ex * u0, y, ez * u0 }, { ex * u1, y, ez * u1 }, RADIO_OBSTACULO_DEFENSA, 10, peligro ? banda : cuerpo);
    }

    // Puntas: emisores esfericos.
    for (int s = -1; s <= 1; s += 2)
    {
        Color punta = MezclaColorDefensa(COLOR_CIAN_DEFENSA, WHITE, 0.3f + 0.3f * std::sin(t * 7.0f + (float)s));
        EsferaDefensa({ ex * (float)s, y, ez * (float)s }, RADIO_OBSTACULO_DEFENSA, punta, 6, 10);
    }

    // Eje central: columna dentro del radio de la barra.
    PrismaDefensa({ 0.0f, 0.0f, 0.0f }, RADIO_OBSTACULO_DEFENSA, RADIO_OBSTACULO_DEFENSA * 0.85f, 1.05f, 10, COLOR_METAL_OSCURO_DEFENSA, a);
    PrismaDefensa({ 0.0f, 0.82f, 0.0f }, RADIO_OBSTACULO_DEFENSA * 0.9f, RADIO_OBSTACULO_DEFENSA * 0.9f, 0.1f, 10, COLOR_CIAN_DEFENSA, a);
    EsferaDefensa({ 0.0f, 1.18f, 0.0f }, 0.18f, MezclaColorDefensa(COLOR_CIAN_DEFENSA, WHITE, 0.5f), 6, 10);
}


//==================================================
// GRADAS, PUBLICO Y PANTALLAS
//==================================================

static const int FILAS_GRADA_DEFENSA = 5;
static const int ASIENTOS_FILA_DEFENSA = 49;
static const float FRENTE_GRADA_DEFENSA = -8.4f;
static const float MEDIO_ANCHO_GRADA_DEFENSA = 15.0f;


static float TopeFilaDefensa(int fila)
{
    return 0.35f + 0.55f * (float)fila;
}


static float ZFilaDefensa(int fila)
{
    return FRENTE_GRADA_DEFENSA - 0.5f - (float)fila;
}


struct PersonaGradaDefensa
{
    Vector3 pies{};
    Color remera{};
    Color piel{};
    float salto = 0.0f;
    bool brazos = false;
    bool barrita = false;        // agita una barrita de luz
    int apoyo = 0;
};


// Hincha de la fila y el asiento dados; false si el asiento esta vacio.
static bool PersonaDefensa(const MinijuegoDefensaNucleo& m, int fila, int asiento, PersonaGradaDefensa& p)
{
    float x = -MEDIO_ANCHO_GRADA_DEFENSA + 0.4f + (float)asiento * 0.6f;

    if (AbsDefensa(AbsDefensa(x) - 5.2f) < 0.45f)
    {
        return false;   // pasillo
    }

    int clave = fila * 131 + asiento * 7;

    if (Ruido01Defensa(clave, 90) < 0.1f)
    {
        return false;
    }

    p.apoyo = x < 0.0f ? 0 : 1;
    p.pies = { x + (Ruido01Defensa(clave, 91) - 0.5f) * 0.12f, TopeFilaDefensa(fila), ZFilaDefensa(fila) - 0.1f };

    float r = Ruido01Defensa(clave, 92);
    Color equipo = ObtenerColorEquipoDefensa(p.apoyo);

    if (AbsDefensa(x) < 1.6f || r > 0.72f)
    {
        const Color varios[4] = {
            Color{ 236, 236, 244, 255 }, Color{ 255, 200, 80, 255 },
            Color{ 150, 110, 230, 255 }, Color{ 80, 200, 170, 255 }
        };
        p.remera = varios[(int)(r * 13.0f) % 4];
    }
    else
    {
        p.remera = MezclaColorDefensa(equipo, r < 0.36f ? WHITE : BLACK, 0.1f + 0.3f * Ruido01Defensa(clave, 93));
    }

    const Color pieles[4] = {
        Color{ 236, 196, 160, 255 }, Color{ 198, 146, 104, 255 },
        Color{ 150, 100, 70, 255 }, Color{ 246, 214, 184, 255 }
    };
    p.piel = pieles[(int)(Ruido01Defensa(clave, 94) * 4.0f) & 3];

    // Euforia: el gol de su equipo los hace saltar y levantar los brazos.
    float euforia = m.festejo * 0.45f;

    if (m.equipoGolVisual == p.apoyo && m.tiempoGolVisual >= 0.0f && m.tiempoGolVisual < 3.5f)
    {
        euforia = m.festejo;
    }

    if (m.fase == FASE_DEFENSA_NUCLEO_TERMINADO && !m.empate && m.equipoGanador == p.apoyo)
    {
        euforia = 1.0f;
    }

    float fase = Ruido01Defensa(clave, 95) * 6.2832f;
    float rapidez = 7.0f + 4.0f * Ruido01Defensa(clave, 96);
    p.salto = (0.03f + euforia * 0.28f) * AbsDefensa(std::sin(m.tiempoAnimacion * rapidez + fase));
    p.brazos = euforia > 0.35f + 0.5f * Ruido01Defensa(clave, 97);
    p.barrita = Ruido01Defensa(clave, 98) < 0.16f;

    return true;
}


static void DibujarPersonaDefensa(const PersonaGradaDefensa& p)
{
    float y = p.pies.y + p.salto;
    Vector3 c = p.pies;
    CuboDefensa({ c.x, y + 0.22f, c.z }, { 0.34f, 0.42f, 0.24f }, TinteDefensa(p.remera, c));
    CuboDefensa({ c.x, y + 0.56f, c.z }, { 0.22f, 0.22f, 0.22f }, TinteDefensa(p.piel, c));

    if (p.brazos)
    {
        CuboDefensa({ c.x - 0.2f, y + 0.6f, c.z }, { 0.08f, 0.36f, 0.08f }, TinteDefensa(p.remera, c));
        CuboDefensa({ c.x + 0.2f, y + 0.6f, c.z }, { 0.08f, 0.36f, 0.08f }, TinteDefensa(p.remera, c));
    }
}


static Vector3 PuntaBarritaDefensa(const PersonaGradaDefensa& p, float t, int semilla)
{
    float vaiven = std::sin(t * 5.0f + (float)semilla) * 0.25f;
    return { p.pies.x + 0.22f + vaiven, p.pies.y + p.salto + 0.95f, p.pies.z };
}


static void DibujarGradaDefensa(const MinijuegoDefensaNucleo& m, float t, int paso, float flash, Color colorFlash)
{
    Color hormigon = Color{ 92, 96, 122, 255 };

    for (int fila = 0; fila < FILAS_GRADA_DEFENSA; fila++)
    {
        float tope = TopeFilaDefensa(fila);
        float zc = ZFilaDefensa(fila);
        float alto = tope - BASE_AZOTEA_DEFENSA;

        // Escalon de hormigon y fila de butacas por sector.
        CuboNieblaDefensa({ 0.0f, BASE_AZOTEA_DEFENSA + alto * 0.5f, zc }, { MEDIO_ANCHO_GRADA_DEFENSA * 2.0f, alto, 1.0f }, TonoDefensa(hormigon, 0.94f + 0.04f * (float)(fila & 1)));

        for (int sector = 0; sector < 3; sector++)
        {
            float xa = sector == 0 ? -MEDIO_ANCHO_GRADA_DEFENSA + 0.1f : (sector == 1 ? -4.75f : 5.65f);
            float xb = sector == 0 ? -5.65f : (sector == 1 ? 4.75f : MEDIO_ANCHO_GRADA_DEFENSA - 0.1f);
            Color butaca = sector == 0
                ? MezclaColorDefensa(ObtenerColorEquipoDefensa(0), hormigon, 0.45f)
                : (sector == 2 ? MezclaColorDefensa(ObtenerColorEquipoDefensa(1), hormigon, 0.45f) : Color{ 108, 84, 150, 255 });
            CuboNieblaDefensa({ (xa + xb) * 0.5f, tope + 0.08f, zc - 0.32f }, { xb - xa, 0.16f, 0.3f }, butaca);
        }

        // Escaleras de los pasillos.
        for (int s = -1; s <= 1; s += 2)
        {
            CuboNieblaDefensa({ (float)s * 5.2f, tope + 0.01f, zc }, { 0.8f, 0.04f, 0.98f }, Color{ 150, 154, 176, 255 });
        }

        // Mejillas laterales.
        for (int s = -1; s <= 1; s += 2)
        {
            CuboNieblaDefensa({ (float)s * (MEDIO_ANCHO_GRADA_DEFENSA + 0.2f), BASE_AZOTEA_DEFENSA + (alto + 0.4f) * 0.5f, zc }, { 0.4f, alto + 0.4f, 1.0f }, Color{ 78, 82, 108, 255 });
        }
    }

    // Muro trasero con la cercha de luces.
    float topeUltima = TopeFilaDefensa(FILAS_GRADA_DEFENSA - 1);
    float zFondo = ZFilaDefensa(FILAS_GRADA_DEFENSA - 1) - 0.7f;
    CuboNieblaDefensa({ 0.0f, (BASE_AZOTEA_DEFENSA + topeUltima + 1.2f) * 0.5f, zFondo }, { MEDIO_ANCHO_GRADA_DEFENSA * 2.0f + 0.8f, topeUltima + 1.2f - BASE_AZOTEA_DEFENSA, 0.4f }, Color{ 70, 74, 100, 255 });
    TuboDefensa({ -MEDIO_ANCHO_GRADA_DEFENSA, topeUltima + 2.4f, zFondo }, { MEDIO_ANCHO_GRADA_DEFENSA, topeUltima + 2.4f, zFondo }, 0.08f, 6, TinteDefensa(COLOR_METAL_DEFENSA, { 0.0f, 3.0f, zFondo }));

    for (int k = -3; k <= 3; k++)
    {
        float x = (float)k * 4.4f;
        TuboDefensa({ x, topeUltima + 1.2f, zFondo }, { x, topeUltima + 2.4f, zFondo }, 0.07f, 6, TinteDefensa(COLOR_METAL_DEFENSA, { x, 3.0f, zFondo }));
        CuboNieblaDefensa({ x, topeUltima + 2.25f, zFondo + 0.25f }, { 0.5f, 0.3f, 0.3f }, COLOR_METAL_OSCURO_DEFENSA);
    }

    // Cinta de LEDs en el frente de la grada.
    float corrido = t * 2.2f;
    rlBegin(RL_QUADS);

    for (int k = 0; k < 60; k++)
    {
        float x0 = -MEDIO_ANCHO_GRADA_DEFENSA + (float)k * 0.5f;
        float x1 = x0 + 0.48f;
        int patron = (int)std::floor((float)k * 0.25f - corrido) & 3;
        Color color = patron == 0 ? COLOR_ROSA_DEFENSA : (patron == 1 ? COLOR_CIAN_DEFENSA : (patron == 2 ? COLOR_VIOLETA_DEFENSA : COLOR_CIAN_DEFENSA));
        color = MezclaColorDefensa(TonoDefensa(color, 0.85f), colorFlash, flash);
        QuadDefensa({ x0, -0.12f, FRENTE_GRADA_DEFENSA + 0.012f }, { x1, -0.12f, FRENTE_GRADA_DEFENSA + 0.012f }, { x1, 0.24f, FRENTE_GRADA_DEFENSA + 0.012f }, { x0, 0.24f, FRENTE_GRADA_DEFENSA + 0.012f }, color);
    }

    rlEnd();

    // Publico.
    for (int fila = 0; fila < FILAS_GRADA_DEFENSA; fila++)
    {
        for (int asiento = 0; asiento < ASIENTOS_FILA_DEFENSA; asiento += paso)
        {
            PersonaGradaDefensa p;

            if (PersonaDefensa(m, fila, asiento, p))
            {
                DibujarPersonaDefensa(p);
            }
        }
    }
}


// Pantalla gigante: dos columnas de cinco luces con el marcador.
static void DibujarPantallaDefensa(const MinijuegoDefensaNucleo& m, float lado, float t)
{
    // MODELO FUTURO: pantalla gigante del estadio.
    float x = lado * 10.4f;
    float z = -14.2f;
    float y0 = 3.5f;
    float y1 = 6.1f;
    float ancho = 4.6f;
    float giro = -lado * 16.0f;

    rlPushMatrix();
    rlTranslatef(x, 0.0f, z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    Color marco = TinteDefensa(COLOR_METAL_OSCURO_DEFENSA, { x, 4.0f, z });
    TuboDefensa({ -1.4f, BASE_AZOTEA_DEFENSA, -0.1f }, { -1.4f, y0, -0.1f }, 0.12f, 6, marco);
    TuboDefensa({ 1.4f, BASE_AZOTEA_DEFENSA, -0.1f }, { 1.4f, y0, -0.1f }, 0.12f, 6, marco);
    CuboDefensa({ 0.0f, (y0 + y1) * 0.5f, -0.15f }, { ancho + 0.3f, y1 - y0 + 0.3f, 0.25f }, marco);

    float gol = m.tiempoGolVisual >= 0.0f && m.tiempoGolVisual < 2.5f ? 1.0f - m.tiempoGolVisual / 2.5f : 0.0f;
    Color fondo = Color{ 14, 16, 36, 255 };

    if (gol > 0.0f && m.equipoGolVisual >= 0)
    {
        float parpadeo = 0.5f + 0.5f * std::sin(m.tiempoGolVisual * 18.0f);
        fondo = MezclaColorDefensa(fondo, ObtenerColorEquipoDefensa(m.equipoGolVisual), gol * (0.4f + 0.4f * parpadeo));
    }

    rlBegin(RL_QUADS);
    QuadDefensa({ -ancho * 0.5f, y0, 0.0f }, { ancho * 0.5f, y0, 0.0f }, { ancho * 0.5f, y1, 0.0f }, { -ancho * 0.5f, y1, 0.0f }, fondo);

    // Marcador: luces de gol de cada equipo (5 por lado).
    for (int equipo = 0; equipo < 2; equipo++)
    {
        float xc = equipo == 0 ? -1.15f : 1.15f;
        Color color = ObtenerColorEquipoDefensa(equipo);

        for (int k = 0; k < GOLES_PARA_GANAR_DEFENSA; k++)
        {
            float yy = y0 + 0.3f + (float)k * 0.42f;
            bool encendida = k < m.goles[equipo];
            Color c = encendida ? MezclaColorDefensa(color, WHITE, 0.2f) : TonoDefensa(color, 0.28f);
            QuadDefensa({ xc - 0.7f, yy, 0.01f }, { xc + 0.7f, yy, 0.01f }, { xc + 0.7f, yy + 0.3f, 0.01f }, { xc - 0.7f, yy + 0.3f, 0.01f }, c);
        }
    }

    // Linea central y barra de tiempo.
    float tiempo = m.golDeOro ? 1.0f : Limitar01Defensa(m.tiempoRestante / DURACION_PARTIDA_DEFENSA);
    Color barra = m.golDeOro ? COLOR_NUCLEO_DEFENSA : COLOR_CIAN_DEFENSA;
    QuadDefensa({ -0.04f, y0 + 0.25f, 0.01f }, { 0.04f, y0 + 0.25f, 0.01f }, { 0.04f, y1 - 0.25f, 0.01f }, { -0.04f, y1 - 0.25f, 0.01f }, Color{ 120, 130, 170, 255 });
    QuadDefensa({ -ancho * 0.45f, y1 - 0.2f, 0.01f }, { -ancho * 0.45f + ancho * 0.9f * tiempo, y1 - 0.2f, 0.01f }, { -ancho * 0.45f + ancho * 0.9f * tiempo, y1 - 0.1f, 0.01f }, { -ancho * 0.45f, y1 - 0.1f, 0.01f }, barra);

    // Linea de barrido.
    float barrido = y0 + std::fmod(t * 0.8f, 1.0f) * (y1 - y0);
    QuadDefensa({ -ancho * 0.5f, barrido, 0.015f }, { ancho * 0.5f, barrido, 0.015f }, { ancho * 0.5f, barrido + 0.04f, 0.015f }, { -ancho * 0.5f, barrido + 0.04f, 0.015f }, Color{ 60, 70, 110, 255 });
    rlEnd();

    rlPopMatrix();
}


//==================================================
// UTILERIA DE LA AZOTEA
//==================================================

static const Vector3 MASTILES_DEFENSA[4] = {
    { -16.6f, 6.4f, -9.4f },
    { 16.6f, 6.4f, -9.4f },
    { -15.9f, 5.4f, 7.6f },
    { 15.9f, 5.4f, 7.6f }
};


static Vector3 ObjetivoMastilDefensa(int k)
{
    float lado = MASTILES_DEFENSA[k].x < 0.0f ? -1.0f : 1.0f;
    return { lado * 5.0f, 0.0f, MASTILES_DEFENSA[k].z < 0.0f ? -2.0f : 2.2f };
}


static void DibujarMastilDefensa(int k)
{
    // MODELO FUTURO: torre de iluminacion.
    Vector3 tope = MASTILES_DEFENSA[k];
    Vector3 base = { tope.x, BASE_AZOTEA_DEFENSA, tope.z };
    Color metal = Color{ 120, 126, 150, 255 };

    PrismaDefensa(base, 0.42f, 0.42f, 0.3f, 6, COLOR_METAL_OSCURO_DEFENSA);
    PrismaDefensa({ base.x, base.y + 0.3f, base.z }, 0.2f, 0.12f, tope.y - base.y - 0.3f, 6, metal);

    for (float y = base.y + 1.2f; y < tope.y - 0.5f; y += 1.1f)
    {
        CuboDefensa({ base.x, y, base.z }, { 0.5f, 0.06f, 0.06f }, metal);
        CuboDefensa({ base.x, y + 0.5f, base.z }, { 0.06f, 0.06f, 0.5f }, metal);
    }

    Vector3 objetivo = ObjetivoMastilDefensa(k);
    float dx = objetivo.x - tope.x;
    float dz = objetivo.z - tope.z;
    float dy = objetivo.y - tope.y;
    float horizontal = std::sqrt(dx * dx + dz * dz);

    rlPushMatrix();
    rlTranslatef(tope.x, tope.y, tope.z);
    rlRotatef(std::atan2(-dz, dx) * RAD2DEG, 0.0f, 1.0f, 0.0f);
    rlRotatef(std::atan2(dy, horizontal) * RAD2DEG, 0.0f, 0.0f, 1.0f);
    CuboDefensa({ -0.1f, 0.0f, 0.0f }, { 0.3f, 1.1f, 1.9f }, COLOR_METAL_OSCURO_DEFENSA);

    for (int f = 0; f < 2; f++)
    {
        for (int c = 0; c < 3; c++)
        {
            CuboDefensa({ 0.07f, -0.25f + 0.5f * (float)f, -0.6f + 0.6f * (float)c }, { 0.06f, 0.38f, 0.48f }, COLOR_FOCO_DEFENSA);
        }
    }

    rlPopMatrix();
}


static void DibujarBancoDefensa(int equipo, float t)
{
    // MODELO FUTURO: banco de suplentes con techo.
    float lado = LadoArcoEquipoDefensa(equipo);
    Color color = ObtenerColorEquipoDefensa(equipo);
    float y = BASE_AZOTEA_DEFENSA;

    // Zona del equipo pintada en la azotea.
    rlBegin(RL_QUADS);
    Color zona = MezclaColorDefensa(COLOR_AZOTEA_DEFENSA, color, 0.32f);
    QuadDefensa({ lado * 12.6f, y + 0.012f, 3.0f }, { lado * 15.3f, y + 0.012f, 3.0f }, { lado * 15.3f, y + 0.012f, -3.0f }, { lado * 12.6f, y + 0.012f, -3.0f }, zona);
    rlEnd();

    FranjaDefensa({ lado * 12.6f, -3.0f }, { lado * 12.6f, 3.0f }, 0.08f, y + 0.02f, MezclaColorDefensa(color, WHITE, 0.3f));

    // Asiento, respaldo y patas.
    Color asiento = MezclaColorDefensa(color, BLACK, 0.15f);
    CuboDefensa({ lado * 14.25f, y + 0.42f, 0.0f }, { 0.55f, 0.1f, 5.0f }, asiento);
    CuboDefensa({ lado * 14.52f, y + 0.75f, 0.0f }, { 0.1f, 0.6f, 5.0f }, asiento);

    for (int k = -2; k <= 2; k++)
    {
        CuboDefensa({ lado * 14.25f, y + 0.18f, (float)k * 1.2f }, { 0.08f, 0.36f, 0.4f }, COLOR_METAL_OSCURO_DEFENSA);
    }

    // Techo: marco de tubos (el vidrio va en la pasada translucida).
    Color marco = COLOR_METAL_DEFENSA;

    for (int s = -1; s <= 1; s++)
    {
        float z = (float)s * 2.6f;
        TuboDefensa({ lado * 14.85f, y, z }, { lado * 14.85f, y + 1.85f, z }, 0.05f, 6, marco);
        TuboDefensa({ lado * 14.85f, y + 1.85f, z }, { lado * 13.1f, y + 1.55f, z }, 0.05f, 6, marco);
    }

    TuboDefensa({ lado * 13.1f, y + 1.55f, -2.6f }, { lado * 13.1f, y + 1.55f, 2.6f }, 0.05f, 6, marco);

    // Conservadora y toallas.
    CuboDefensa({ lado * 13.35f, y + 0.25f, 2.25f }, { 0.5f, 0.5f, 0.45f }, color);
    CuboDefensa({ lado * 13.35f, y + 0.52f, 2.25f }, { 0.52f, 0.06f, 0.47f }, Color{ 236, 238, 246, 255 });
    CuboDefensa({ lado * 14.25f, y + 0.5f, -1.4f }, { 0.4f, 0.05f, 0.3f }, Color{ 236, 238, 246, 255 });
    CuboDefensa({ lado * 14.25f, y + 0.5f, 0.9f }, { 0.4f, 0.05f, 0.3f }, Color{ 236, 238, 246, 255 });

    // Bandera del equipo que flamea.
    Vector3 asta = { lado * 15.6f, y, -3.5f };
    TuboDefensa(asta, { asta.x, y + 2.8f, asta.z }, 0.04f, 6, marco);
    rlBegin(RL_QUADS);

    for (int k = 0; k < 6; k++)
    {
        float u0 = (float)k / 6.0f;
        float u1 = (float)(k + 1) / 6.0f;
        float o0 = std::sin(t * 5.0f - u0 * 5.0f) * 0.18f * u0;
        float o1 = std::sin(t * 5.0f - u1 * 5.0f) * 0.18f * u1;
        Color c = TonoDefensa(color, (k & 1) ? 0.85f : 1.0f);
        QuadDefensa(
            { asta.x - lado * u0 * 1.4f, y + 2.0f, asta.z + o0 },
            { asta.x - lado * u1 * 1.4f, y + 2.0f, asta.z + o1 },
            { asta.x - lado * u1 * 1.4f, y + 2.75f, asta.z + o1 },
            { asta.x - lado * u0 * 1.4f, y + 2.75f, asta.z + o0 },
            c
        );
    }

    rlEnd();
}


static void DibujarAireAcondicionadoDefensa(Vector3 base, float t)
{
    // MODELO FUTURO: equipo de aire acondicionado.
    Color chapa = Color{ 168, 174, 192, 255 };
    CuboDefensa({ base.x, base.y + 0.5f, base.z }, { 1.7f, 1.0f, 1.5f }, chapa);

    for (int k = 0; k < 5; k++)
    {
        CuboDefensa({ base.x - 0.6f + (float)k * 0.3f, base.y + 0.5f, base.z + 0.755f }, { 0.12f, 0.7f, 0.02f }, Color{ 70, 74, 92, 255 });
    }

    AnilloDefensa({ base.x, base.y + 1.01f, base.z }, 0.0f, 0.6f, Color{ 40, 42, 56, 255 }, 20);
    AnilloDefensa({ base.x, base.y + 1.02f, base.z }, 0.56f, 0.64f, chapa, 20);

    rlPushMatrix();
    rlTranslatef(base.x, base.y + 1.04f, base.z);
    rlRotatef(t * 540.0f, 0.0f, 1.0f, 0.0f);

    for (int k = 0; k < 4; k++)
    {
        rlPushMatrix();
        rlRotatef((float)k * 90.0f + 20.0f, 0.0f, 1.0f, 0.0f);
        CuboDefensa({ 0.27f, 0.0f, 0.0f }, { 0.48f, 0.02f, 0.14f }, Color{ 140, 146, 166, 255 });
        rlPopMatrix();
    }

    rlPopMatrix();
    PrismaDefensa({ base.x, base.y + 1.0f, base.z }, 0.1f, 0.1f, 0.08f, 8, COLOR_METAL_OSCURO_DEFENSA);
}


static void DibujarCeldasEnergiaDefensa(Vector3 base, float t)
{
    // MODELO FUTURO: estante de celdas de energia.
    CuboDefensa({ base.x, base.y + 0.06f, base.z }, { 1.5f, 0.12f, 0.6f }, COLOR_METAL_OSCURO_DEFENSA);
    CuboDefensa({ base.x, base.y + 0.95f, base.z }, { 1.5f, 0.08f, 0.6f }, COLOR_METAL_OSCURO_DEFENSA);

    for (int k = -1; k <= 1; k++)
    {
        Vector3 c = { base.x + (float)k * 0.45f, base.y + 0.12f, base.z };
        float pulso = 0.75f + 0.25f * std::sin(t * 3.0f + (float)k * 1.7f);
        PrismaDefensa(c, 0.17f, 0.17f, 0.78f, 8, Color{ 60, 90, 120, 255 });
        PrismaDefensa({ c.x, c.y + 0.12f, c.z }, 0.1f, 0.1f, 0.55f, 8, TonoDefensa(COLOR_CIAN_DEFENSA, pulso));
    }

    TuboDefensa({ base.x - 0.75f, base.y, base.z }, { base.x - 0.75f, base.y + 1.0f, base.z }, 0.04f, 4, COLOR_METAL_DEFENSA);
    TuboDefensa({ base.x + 0.75f, base.y, base.z }, { base.x + 0.75f, base.y + 1.0f, base.z }, 0.04f, 4, COLOR_METAL_DEFENSA);
}


static void DibujarCamaraTvDefensa(Vector3 base)
{
    // MODELO FUTURO: camara de transmision en tripode.
    Vector3 cabeza = { base.x, base.y + 1.45f, base.z };

    for (int k = 0; k < 3; k++)
    {
        float a = (float)k * 2.094f;
        TuboDefensa({ base.x + std::cos(a) * 0.45f, base.y, base.z + std::sin(a) * 0.45f }, { cabeza.x, cabeza.y - 0.15f, cabeza.z }, 0.03f, 4, COLOR_METAL_OSCURO_DEFENSA);
    }

    rlPushMatrix();
    rlTranslatef(cabeza.x, cabeza.y, cabeza.z);
    rlRotatef(155.0f, 0.0f, 1.0f, 0.0f);
    CuboDefensa({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.38f, 0.32f }, Color{ 50, 52, 66, 255 });
    CuboDefensa({ -0.1f, 0.25f, 0.0f }, { 0.3f, 0.1f, 0.06f }, Color{ 90, 94, 110, 255 });
    rlPopMatrix();

    float dirX = std::cos(155.0f * DEG2RAD);
    float dirZ = -std::sin(155.0f * DEG2RAD);
    TuboDefensa({ cabeza.x + dirX * 0.3f, cabeza.y, cabeza.z + dirZ * 0.3f }, { cabeza.x + dirX * 0.75f, cabeza.y, cabeza.z + dirZ * 0.75f }, 0.11f, 8, Color{ 34, 36, 46, 255 });
}


static Vector3 PosicionDronDefensa(int k, float t)
{
    float lado = k == 0 ? -1.0f : 1.0f;
    float a = t * (0.6f + 0.15f * (float)k) + (float)k * 2.0f;
    return {
        lado * (15.9f + std::cos(a) * 0.7f),
        2.6f + 0.25f * std::sin(t * 2.1f + (float)k),
        -1.0f + std::sin(a) * 1.4f
    };
}


static void DibujarDronDefensa(Vector3 p, float t)
{
    // MODELO FUTURO: dron camara.
    CuboDefensa(p, { 0.42f, 0.16f, 0.42f }, Color{ 58, 60, 78, 255 });
    CuboDefensa({ p.x, p.y - 0.14f, p.z + 0.12f }, { 0.14f, 0.12f, 0.14f }, Color{ 30, 30, 40, 255 });

    for (int k = 0; k < 4; k++)
    {
        float a = 0.785f + (float)k * 1.5708f;
        Vector3 brazo = { p.x + std::cos(a) * 0.38f, p.y + 0.04f, p.z + std::sin(a) * 0.38f };
        TuboDefensa(p, brazo, 0.03f, 4, COLOR_METAL_DEFENSA);
        AnilloDefensa({ brazo.x, brazo.y + 0.05f, brazo.z }, 0.0f, 0.2f, ConAlfaDefensa(Color{ 190, 196, 214, 255 }, 255.0f), 10, t * 40.0f, t * 40.0f + 6.2832f);
    }
}


static void DibujarUtileriaDefensa(float t, float chaflan)
{
    float y = BASE_AZOTEA_DEFENSA;

    DibujarBancoDefensa(0, t);
    DibujarBancoDefensa(1, t);

    for (int k = 0; k < 4; k++)
    {
        DibujarMastilDefensa(k);
    }

    DibujarAireAcondicionadoDefensa({ -16.0f, y, -6.3f }, t);
    DibujarAireAcondicionadoDefensa({ 16.0f, y, -6.3f }, t + 0.3f);

    DibujarCeldasEnergiaDefensa({ -13.7f, y, -5.0f }, t);
    DibujarCeldasEnergiaDefensa({ 13.7f, y, 4.7f }, t + 1.0f);

    DibujarCamaraTvDefensa({ 13.9f, y, -5.1f });

    // Plataforma de drones y drones camara.
    Vector3 pad = { -16.0f, y + 0.02f, -1.0f };
    AnilloDefensa(pad, 0.0f, 0.85f, Color{ 52, 56, 76, 255 }, 24);
    AnilloDefensa({ pad.x, pad.y + 0.005f, pad.z }, 0.62f, 0.7f, Color{ 220, 196, 90, 255 }, 24);
    FranjaDefensa({ pad.x - 0.35f, pad.z }, { pad.x + 0.35f, pad.z }, 0.1f, pad.y + 0.01f, Color{ 220, 196, 90, 255 });
    FranjaDefensa({ pad.x, pad.z - 0.35f }, { pad.x, pad.z + 0.35f }, 0.1f, pad.y + 0.01f, Color{ 220, 196, 90, 255 });

    for (int k = 0; k < 2; k++)
    {
        DibujarDronDefensa(PosicionDronDefensa(k, t), t);
    }

    // Cajas de equipo (road cases) y conos.
    CuboGiradoDefensa({ -13.3f, y, 4.7f }, { 0.9f, 0.6f, 0.7f }, 8.0f, Color{ 52, 54, 70, 255 });
    CuboGiradoDefensa({ -13.4f, y + 0.6f, 4.75f }, { 0.7f, 0.45f, 0.55f }, -6.0f, Color{ 70, 72, 92, 255 });
    CuboGiradoDefensa({ -14.3f, y, 5.3f }, { 0.7f, 0.5f, 0.6f }, 24.0f, Color{ 52, 54, 70, 255 });
    CuboGiradoDefensa({ 13.2f, y, -6.6f }, { 0.8f, 0.55f, 0.6f }, -12.0f, Color{ 52, 54, 70, 255 });

    // Pilones de luz en las cuatro esquinas de la cancha.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            Vector2 esquina = VerticeContornoDefensa(sx > 0 ? (sz < 0 ? 0 : 3) : (sz < 0 ? 7 : 4), 1.0f, chaflan);
            Vector3 b = { esquina.x + (float)sx * 1.2f, y, esquina.y };
            PrismaDefensa(b, 0.22f, 0.18f, 1.5f, 6, COLOR_METAL_OSCURO_DEFENSA);
            PrismaDefensa({ b.x, y + 1.5f, b.z }, 0.2f, 0.2f, 0.18f, 6, COLOR_CIAN_DEFENSA);
        }
    }

    // Balizas al frente.
    for (int k = -4; k <= 4; k++)
    {
        float x = (float)k * 2.5f;
        CuboDefensa({ x, y + 0.1f, 8.7f }, { 0.3f, 0.2f, 0.3f }, COLOR_METAL_OSCURO_DEFENSA);
        CuboDefensa({ x, y + 0.23f, 8.7f }, { 0.22f, 0.06f, 0.22f }, COLOR_CIAN_DEFENSA);
    }

    // Antena con baliza y tanque de agua detras de la grada.
    Vector3 antena = { -16.3f, y, -14.0f };
    PrismaDefensa(antena, 0.3f, 0.06f, 9.5f, 4, Color{ 150, 150, 170, 255 });

    Vector3 tanque = { 16.2f, y, -13.6f };
    for (int k = 0; k < 4; k++)
    {
        float a = 0.785f + (float)k * 1.5708f;
        TuboDefensa({ tanque.x + std::cos(a) * 0.7f, y, tanque.z + std::sin(a) * 0.7f }, { tanque.x + std::cos(a) * 0.7f, y + 1.1f, tanque.z + std::sin(a) * 0.7f }, 0.06f, 4, COLOR_METAL_OSCURO_DEFENSA);
    }

    PrismaDefensa({ tanque.x, y + 1.1f, tanque.z }, 0.95f, 0.95f, 1.5f, 14, Color{ 120, 110, 100, 255 });
    PrismaDefensa({ tanque.x, y + 2.6f, tanque.z }, 0.95f, 0.1f, 0.5f, 14, Color{ 96, 88, 82, 255 });
}


//==================================================
// NUCLEO Y JUGADORES
//==================================================

// Donde se ve el nucleo y con que escala (0 = no se ve).
static float PosicionVisualNucleoDefensa(const MinijuegoDefensaNucleo& m, Vector3& p)
{
    const NucleoDefensaNucleo& n = m.nucleo;

    if (m.resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        return 0.0f;
    }

    if (m.fase == FASE_DEFENSA_NUCLEO_PREPARACION)
    {
        // Flota sobre el eje, cargandose, antes del saque.
        p = { 0.0f, 2.1f + 0.15f * std::sin(m.tiempoAnimacion * 3.0f), 0.0f };
        return 1.0f;
    }

    if (n.enJuego)
    {
        p = { n.x, ALTURA_NUCLEO_DEFENSA, n.z };
        return 1.0f;
    }

    // Tras un gol: el receptor del arco lo absorbe.
    if (AbsDefensa(n.x) >= LIMITE_X_DEFENSA - 0.01f && m.tiempoGolVisual >= 0.0f)
    {
        float lado = n.x > 0.0f ? 1.0f : -1.0f;
        float u = SuaveDefensa(m.tiempoGolVisual / 0.6f);
        Vector3 receptor = { lado * (LIMITE_X_DEFENSA + FONDO_ARCO_DEFENSA - 0.2f), 0.75f, 0.0f };
        p = {
            n.x + (receptor.x - n.x) * u,
            ALTURA_NUCLEO_DEFENSA + (receptor.y - ALTURA_NUCLEO_DEFENSA) * u,
            n.z + (receptor.z - n.z) * u
        };
        return 1.0f - u;
    }

    p = { n.x, ALTURA_NUCLEO_DEFENSA, n.z };
    return 0.8f;
}


static void DibujarNucleoDefensa(const MinijuegoDefensaNucleo& m, float t)
{
    Vector3 p{};
    float escala = PosicionVisualNucleoDefensa(m, p);

    if (escala <= 0.01f)
    {
        return;
    }

    float carga = Limitar01Defensa(m.nucleo.tiempoCarga / 0.6f);
    Color color = MezclaColorDefensa(COLOR_NUCLEO_DEFENSA, COLOR_NUCLEO_CARGA_DEFENSA, carga);
    float r = RADIO_NUCLEO_DEFENSA * escala;

    DibujarSombraRetroCircular({ p.x, p.y, p.z }, r * 0.95f, r * 0.95f, 0.05f, 0.42f, 12);
    EsferaDefensa(p, r, TonoDefensa(color, 1.15f), 9, 14);

    // Anillos de contencion que giran.
    for (int k = 0; k < 2; k++)
    {
        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(t * (k == 0 ? 160.0f : -220.0f), 0.0f, 1.0f, 0.0f);
        rlRotatef(k == 0 ? 62.0f : -48.0f, 1.0f, 0.0f, 0.0f);
        AnilloDefensa({ 0.0f, 0.0f, 0.0f }, r * 1.0f, r * 1.1f, MezclaColorDefensa(color, WHITE, 0.6f), 24);
        rlPopMatrix();
    }
}


static void DibujarJugadoresDefensa(
    const MinijuegoDefensaNucleo& m,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    bool mostrarDebug
)
{
    for (int i = 0; i < limite; i++)
    {
        if (m.estadosJugadores[i].equipo < 0)
        {
            continue;
        }

        // Un mando desconectado queda a cargo de la IA.
        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorModeloCompartido(jugadores[i], participanteVisual);

        if (mostrarDebug)
        {
            DrawCircle3D({ jugadores[i].posicion.x, 0.2f, jugadores[i].posicion.z }, RADIO_JUGADOR_DEFENSA, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }
    }
}


static void DibujarParticulasSolidasDefensa(const MinijuegoDefensaNucleo& m)
{
    for (int i = 0; i < MAX_PARTICULAS_DEFENSA_NUCLEO; i++)
    {
        const ParticulaDefensaNucleo& p = m.particulas[i];

        if (!p.activa || p.brillo)
        {
            continue;
        }

        // Papelitos que giran.
        float giro = (float)i * 0.7f + p.vida * 9.0f;
        float s = p.tamano;
        CuboDefensa(p.posicion, { s * (0.6f + 0.4f * std::cos(giro)), s * 0.25f, s }, p.color);
    }
}


//==================================================
// PASADA TRANSLUCIDA (alfa normal, sin escribir profundidad)
//==================================================

static void PuntoCortinaDefensa(float lado, float u, float& z, float& tope)
{
    Vector3 p = PuntoArcoEnergiaDefensa(lado, u);
    z = p.z;
    tope = p.y;
}


static void DibujarTranslucidosDefensa(
    const MinijuegoDefensaNucleo& m,
    const JugadorPrueba jugadores[],
    int limite,
    float t,
    bool decoracion
)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Sombra de la barra (gira con ella).
    float a = m.anguloObstaculo;
    float ex = std::cos(a) * MITAD_LARGO_OBSTACULO_DEFENSA;
    float ez = std::sin(a) * MITAD_LARGO_OBSTACULO_DEFENSA;
    FranjaDefensa({ -ex + 0.12f, -ez + 0.2f }, { ex + 0.12f, ez + 0.2f }, 0.62f, 0.05f, Color{ 8, 8, 20, 90 });

    // Area chica de cada equipo y franja neutral rayada.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        float lado = LadoArcoEquipoDefensa(equipo);
        Color color = ConAlfaDefensa(ObtenerColorEquipoDefensa(equipo), 46.0f);
        float x0 = lado * (LIMITE_X_DEFENSA - 1.3f);
        float x1 = lado * LIMITE_X_DEFENSA;
        rlBegin(RL_QUADS);
        QuadDefensa({ x0, 0.045f, -MITAD_ARCO_DEFENSA }, { x1, 0.045f, -MITAD_ARCO_DEFENSA }, { x1, 0.045f, MITAD_ARCO_DEFENSA }, { x0, 0.045f, MITAD_ARCO_DEFENSA }, color);
        rlEnd();
    }

    float chaflan = TEMAS_DEFENSA[m.indiceTema].chaflan;
    float zNeutral = MedioAnchoCanchaDefensa(1.0f, chaflan, 0.45f);

    for (float z = -zNeutral; z < zNeutral - 0.8f; z += 0.7f)
    {
        if (z > -3.6f && z < 2.8f)
        {
            continue;
        }

        FranjaDefensa({ -0.9f, z }, { 0.9f, z + 0.8f }, 0.16f, 0.045f, Color{ 200, 230, 255, 26 });
    }

    // Cortinas de energia en la boca de cada arco.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        float lado = LadoArcoEquipoDefensa(equipo);
        float destello = m.destelloArco[lado < 0.0f ? 0 : 1];
        Color color = ObtenerColorEquipoDefensa(equipo);
        Color velo = ConAlfaDefensa(MezclaColorDefensa(color, WHITE, 0.2f), 30.0f + destello * 110.0f);
        float x = lado * (LIMITE_X_DEFENSA + 0.02f);
        const int TRAMOS = 14;

        rlBegin(RL_QUADS);

        for (int k = 0; k < TRAMOS; k++)
        {
            float z0, y0, z1, y1;
            PuntoCortinaDefensa(lado, (float)k / (float)TRAMOS, z0, y0);
            PuntoCortinaDefensa(lado, (float)(k + 1) / (float)TRAMOS, z1, y1);
            QuadDefensa({ x, 0.0f, z0 }, { x, 0.0f, z1 }, { x, y1, z1 }, { x, y0, z0 }, velo);
        }

        // Lineas de barrido que suben por la cortina.
        for (int b = 0; b < 3; b++)
        {
            float yb = std::fmod(t * 0.7f + (float)b / 3.0f, 1.0f) * ALTO_POSTE_DEFENSA;
            Color linea = ConAlfaDefensa(MezclaColorDefensa(color, WHITE, 0.5f), 70.0f);
            QuadDefensa({ x, yb, -MITAD_ARCO_DEFENSA + 0.25f }, { x, yb, MITAD_ARCO_DEFENSA - 0.25f }, { x, yb + 0.05f, MITAD_ARCO_DEFENSA - 0.25f }, { x, yb + 0.05f, -MITAD_ARCO_DEFENSA + 0.25f }, linea);
        }

        rlEnd();
    }

    if (decoracion)
    {
        // Techos de vidrio de los bancos.
        for (int equipo = 0; equipo < 2; equipo++)
        {
            float lado = LadoArcoEquipoDefensa(equipo);
            Color vidrio = ConAlfaDefensa(MezclaColorDefensa(ObtenerColorEquipoDefensa(equipo), WHITE, 0.2f), 80.0f);
            float y = BASE_AZOTEA_DEFENSA;
            rlBegin(RL_QUADS);
            QuadDefensa({ lado * 14.85f, y + 1.85f, -2.6f }, { lado * 14.85f, y + 1.85f, 2.6f }, { lado * 13.1f, y + 1.55f, 2.6f }, { lado * 13.1f, y + 1.55f, -2.6f }, vidrio);
            QuadDefensa({ lado * 14.85f, y + 0.9f, -2.6f }, { lado * 14.85f, y + 0.9f, 2.6f }, { lado * 14.85f, y + 1.85f, 2.6f }, { lado * 14.85f, y + 1.85f, -2.6f }, ConAlfaDefensa(vidrio, 50.0f));
            rlEnd();
        }

        // Barandas de vidrio de la azotea.
        Color vidrio = ConAlfaDefensa(COLOR_VIDRIO_DEFENSA, 34.0f);
        float y0 = BASE_AZOTEA_DEFENSA + 0.44f;
        float y1 = BASE_AZOTEA_DEFENSA + 1.45f;
        rlBegin(RL_QUADS);

        for (int s = -1; s <= 1; s += 2)
        {
            float x = (float)s * (BORDE_AZOTEA_X_DEFENSA - 0.2f);
            QuadDefensa({ x, y0, FONDO_AZOTEA_DEFENSA }, { x, y0, FRENTE_AZOTEA_DEFENSA }, { x, y1, FRENTE_AZOTEA_DEFENSA }, { x, y1, FONDO_AZOTEA_DEFENSA }, vidrio);
        }

        QuadDefensa({ -BORDE_AZOTEA_X_DEFENSA, y0, FRENTE_AZOTEA_DEFENSA - 0.2f }, { BORDE_AZOTEA_X_DEFENSA, y0, FRENTE_AZOTEA_DEFENSA - 0.2f }, { BORDE_AZOTEA_X_DEFENSA, y1, FRENTE_AZOTEA_DEFENSA - 0.2f }, { -BORDE_AZOTEA_X_DEFENSA, y1, FRENTE_AZOTEA_DEFENSA - 0.2f }, vidrio);
        rlEnd();
    }

    // Anillo de equipo de cada jugador, recarga del golpe y alcance.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorDefensaNucleo& e = m.estadosJugadores[i];

        if (e.equipo < 0)
        {
            continue;
        }

        Vector3 p = { jugadores[i].posicion.x, 0.055f, jugadores[i].posicion.z };
        Color color = ObtenerColorEquipoDefensa(e.equipo);
        AnilloDefensa(p, RADIO_JUGADOR_DEFENSA - 0.02f, RADIO_JUGADOR_DEFENSA + 0.1f, ConAlfaDefensa(color, 220.0f), 32);

        float listo = e.cooldownGolpe > 0.0f ? 1.0f - e.cooldownGolpe / COOLDOWN_GOLPE_DEFENSA : 1.0f;
        float alfaListo = listo >= 1.0f ? 110.0f + 60.0f * std::sin(t * 6.0f) : 150.0f;
        AnilloDefensa(p, RADIO_JUGADOR_DEFENSA + 0.14f, RADIO_JUGADOR_DEFENSA + 0.2f, ConAlfaDefensa(WHITE, alfaListo), 28, -1.5708f, -1.5708f + 6.2832f * Limitar01Defensa(listo));

        if (e.tiempoGolpeActivo > 0.0f)
        {
            float alcance = RADIO_JUGADOR_DEFENSA + ALCANCE_EXTRA_GOLPE_DEFENSA;
            AnilloDefensa(p, alcance - 0.1f, alcance, ConAlfaDefensa(MezclaColorDefensa(color, WHITE, 0.6f), 230.0f), 36);
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// LUCES FALSAS (pasada aditiva)
//==================================================

static void DibujarLucesCiudadDefensa(float t)
{
    for (int j = CIUDAD_MIN_J_DEFENSA; j <= CIUDAD_MAX_J_DEFENSA; j++)
    {
        for (int i = CIUDAD_MIN_I_DEFENSA; i <= CIUDAD_MAX_I_DEFENSA; i++)
        {
            EdificioDefensa e;

            if (!EdificioCiudadDefensa(i, j, e))
            {
                continue;
            }

            float niebla = 1.0f - Limitar01Defensa((e.distancia - 40.0f) / 120.0f) * 0.6f;

            if (LetreroEdificioDefensa(e))
            {
                Vector3 c = { e.x, e.techo + 2.6f, e.z + e.fondo * 0.5f - 0.2f };
                ResplandorFrenteDefensa(c, e.ancho * 0.75f, ConAlfaDefensa(ColorLetreroDefensa(e.semilla), 60.0f * niebla));
            }

            if (AntenaEdificioDefensa(e) && std::fmod(t + Ruido01Defensa(e.semilla, 36) * 1.6f, 1.6f) < 0.35f)
            {
                float alto = 4.0f + Ruido01Defensa(e.semilla, 35) * 4.0f;
                Vector3 c = { e.x - e.ancho * 0.25f, e.techo + alto, e.z };
                ResplandorFrenteDefensa(c, 1.4f, ConAlfaDefensa(Color{ 255, 50, 60, 255 }, 190.0f * niebla));
            }
        }
    }

    // Autos voladores: faros, luces traseras y brillo de los propulsores.
    for (int c = 0; c < CANTIDAD_CARRILES_DEFENSA; c++)
    {
        for (int k = 0; k < CARRILES_DEFENSA[c].cantidad; k++)
        {
            Vector3 p = PosicionAutoDefensa(c, k, t);
            float niebla = 1.0f - Limitar01Defensa((DistanciaOjoDefensa(p) - 30.0f) / 120.0f) * 0.6f;

            EntrarAutoDefensa(c, p);
            DrawCylinderEx({ 1.05f, 0.0f, 0.0f }, { 6.5f, -0.6f, 0.0f }, 0.15f, 1.1f, 10, ConAlfaDefensa(Color{ 255, 240, 200, 255 }, 26.0f * niebla));
            ResplandorDefensa({ 1.1f, 0.02f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 0.7f, ConAlfaDefensa(WHITE, 170.0f * niebla));
            ResplandorDefensa({ -1.1f, 0.05f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 0.6f, ConAlfaDefensa(Color{ 255, 60, 70, 255 }, 170.0f * niebla));
            ResplandorDefensa({ 0.0f, -0.3f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 1.3f, ConAlfaDefensa(ColorAutoDefensa(c, k + 1), 70.0f * niebla), 1.4f);
            rlPopMatrix();
        }
    }
}


static void DibujarLucesDefensa(
    const MinijuegoDefensaNucleo& m,
    const JugadorPrueba jugadores[],
    int limite,
    float t,
    bool decoracion,
    float flash,
    Color colorFlash
)
{
    float chaflan = TEMAS_DEFENSA[m.indiceTema].chaflan;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Resplandor de las lineas.
    DibujarLineasCanchaDefensa(chaflan, 0.16f, 34.0f + flash * 60.0f, t, flash, colorFlash);

    // Torres de iluminacion: halo, cono y charco en la cancha.
    for (int k = 0; k < 4; k++)
    {
        Vector3 tope = MASTILES_DEFENSA[k];
        Vector3 objetivo = ObjetivoMastilDefensa(k);
        Color foco = COLOR_FOCO_DEFENSA;

        if (decoracion)
        {
            DrawCylinderEx(tope, objetivo, 0.5f, 4.2f, 18, ConAlfaDefensa(foco, 7.0f));
            DrawCylinderEx(tope, objetivo, 0.3f, 2.4f, 14, ConAlfaDefensa(foco, 8.0f));
            ResplandorFrenteDefensa(tope, 1.1f, ConAlfaDefensa(foco, 100.0f));
            ResplandorFrenteDefensa(tope, 0.5f, ConAlfaDefensa(WHITE, 170.0f));
        }

        ResplandorSueloDefensa({ objetivo.x, 0.06f, objetivo.z }, 6.0f, ConAlfaDefensa(foco, 20.0f), 1.3f);
    }

    // Arcos de energia.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        float lado = LadoArcoEquipoDefensa(equipo);
        float destello = m.destelloArco[lado < 0.0f ? 0 : 1];
        Color color = ObtenerColorEquipoDefensa(equipo);

        for (int k = 0; k <= 14; k += 2)
        {
            Vector3 p = PuntoArcoEnergiaDefensa(lado, (float)k / 14.0f);
            ResplandorFrenteDefensa(p, 0.7f + destello * 0.6f, ConAlfaDefensa(color, 70.0f + destello * 120.0f));
        }

        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 emisor = { lado * LIMITE_X_DEFENSA, ALTO_POSTE_DEFENSA + 0.24f, (float)s * MITAD_ARCO_DEFENSA };
            ResplandorFrenteDefensa(emisor, 0.8f, ConAlfaDefensa(MezclaColorDefensa(color, WHITE, 0.4f), 160.0f));
        }

        ResplandorSueloDefensa({ lado * (LIMITE_X_DEFENSA + 0.6f), 0.06f, 0.0f }, 3.4f + destello * 2.0f, ConAlfaDefensa(color, 46.0f + destello * 140.0f), 0.8f);
        ResplandorDefensa(
            { lado * (LIMITE_X_DEFENSA + FONDO_ARCO_DEFENSA - 0.05f), 0.75f, 0.0f },
            { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f },
            1.0f + destello * 1.5f,
            ConAlfaDefensa(MezclaColorDefensa(color, WHITE, 0.3f), 110.0f + destello * 140.0f)
        );
    }

    // Obstaculo: puntas, eje y raya de luz bajo la barra.
    {
        float a = m.anguloObstaculo;
        float ex = std::cos(a) * MITAD_LARGO_OBSTACULO_DEFENSA;
        float ez = std::sin(a) * MITAD_LARGO_OBSTACULO_DEFENSA;
        float destello = m.destelloObstaculo;

        for (int s = -1; s <= 1; s += 2)
        {
            ResplandorFrenteDefensa({ ex * (float)s, 0.42f, ez * (float)s }, 0.75f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 120.0f));
            ResplandorSueloDefensa({ ex * (float)s, 0.06f, ez * (float)s }, 1.0f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 60.0f));
        }

        FranjaDefensa({ -ex, -ez }, { ex, ez }, 1.1f, 0.06f, ConAlfaDefensa(COLOR_ROSA_DEFENSA, 26.0f + destello * 90.0f));
        ResplandorFrenteDefensa({ 0.0f, 1.18f, 0.0f }, 0.6f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 160.0f));

        if (destello > 0.0f)
        {
            ResplandorFrenteDefensa({ 0.0f, 0.5f, 0.0f }, 2.8f, ConAlfaDefensa(COLOR_ROSA_DEFENSA, 80.0f * destello));
        }
    }

    // Nucleo: halo, charco, estela y destello del saque.
    {
        Vector3 p{};
        float escala = PosicionVisualNucleoDefensa(m, p);

        if (escala > 0.01f)
        {
            float carga = Limitar01Defensa(m.nucleo.tiempoCarga / 0.6f);
            Color color = MezclaColorDefensa(COLOR_NUCLEO_DEFENSA, COLOR_NUCLEO_CARGA_DEFENSA, carga);
            float pulso = 0.85f + 0.15f * std::sin(t * 9.0f);

            for (int k = 0; k < m.cantidadEstela; k++)
            {
                float u = 1.0f - (float)k / (float)LARGO_ESTELA_DEFENSA_NUCLEO;
                ResplandorFrenteDefensa(m.estela[k], RADIO_NUCLEO_DEFENSA * (0.5f + 0.9f * u), ConAlfaDefensa(color, 120.0f * u * u));
            }

            ResplandorFrenteDefensa(p, (1.3f + carga * 0.8f) * escala * pulso, ConAlfaDefensa(color, 150.0f));
            ResplandorFrenteDefensa(p, 0.7f * escala, ConAlfaDefensa(MezclaColorDefensa(color, WHITE, 0.5f), 220.0f));
            ResplandorSueloDefensa({ p.x, 0.06f, p.z }, 2.3f * escala, ConAlfaDefensa(color, 80.0f));

            if (m.fase == FASE_DEFENSA_NUCLEO_PREPARACION)
            {
                DrawCylinderEx({ 0.0f, 1.2f, 0.0f }, p, 0.12f, 0.28f, 10, ConAlfaDefensa(COLOR_NUCLEO_DEFENSA, 60.0f));
            }

            if (m.destelloSaque > 0.0f)
            {
                float d = m.destelloSaque;
                AnilloDefensa({ p.x, 0.07f, p.z }, 0.5f + (1.0f - d) * 2.2f, 0.7f + (1.0f - d) * 2.4f, ConAlfaDefensa(COLOR_NUCLEO_DEFENSA, 200.0f * d), 32);
            }
        }
    }

    // Golpes: onda del color del equipo en el suelo.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorDefensaNucleo& e = m.estadosJugadores[i];

        if (e.equipo < 0)
        {
            continue;
        }

        Vector3 p = jugadores[i].posicion;
        Color color = ObtenerColorEquipoDefensa(e.equipo);
        ResplandorSueloDefensa({ p.x, 0.06f, p.z }, 1.2f, ConAlfaDefensa(color, 40.0f));

        float d = m.destelloGolpe[i];

        if (d > 0.0f)
        {
            float r = RADIO_JUGADOR_DEFENSA + ALCANCE_EXTRA_GOLPE_DEFENSA + (1.0f - d) * 0.6f;
            AnilloDefensa({ p.x, 0.07f, p.z }, r - 0.18f, r, ConAlfaDefensa(MezclaColorDefensa(color, WHITE, 0.4f), 220.0f * d), 32);
            ResplandorSueloDefensa({ p.x, 0.07f, p.z }, r, ConAlfaDefensa(color, 90.0f * d));
        }
    }

    if (decoracion)
    {
        // Cinta de LEDs de la grada y pantallas.
        rlBegin(RL_QUADS);
        Color cinta = ConAlfaDefensa(MezclaColorDefensa(COLOR_VIOLETA_DEFENSA, colorFlash, flash), 40.0f + flash * 80.0f);
        Color nada = ConAlfaDefensa(cinta, 0.0f);
        float z = FRENTE_GRADA_DEFENSA + 0.03f;
        VerticeDefensa({ -MEDIO_ANCHO_GRADA_DEFENSA, -0.12f, z }, cinta);
        VerticeDefensa({ MEDIO_ANCHO_GRADA_DEFENSA, -0.12f, z }, cinta);
        VerticeDefensa({ MEDIO_ANCHO_GRADA_DEFENSA, 0.9f, z + 0.4f }, nada);
        VerticeDefensa({ -MEDIO_ANCHO_GRADA_DEFENSA, 0.9f, z + 0.4f }, nada);
        VerticeDefensa({ -MEDIO_ANCHO_GRADA_DEFENSA, 0.0f, z }, cinta);
        VerticeDefensa({ MEDIO_ANCHO_GRADA_DEFENSA, 0.0f, z }, cinta);
        VerticeDefensa({ MEDIO_ANCHO_GRADA_DEFENSA, 0.03f, z + 1.4f }, nada);
        VerticeDefensa({ -MEDIO_ANCHO_GRADA_DEFENSA, 0.03f, z + 1.4f }, nada);
        rlEnd();

        for (int s = -1; s <= 1; s += 2)
        {
            Color pantalla = flash > 0.0f ? colorFlash : COLOR_VIOLETA_DEFENSA;
            ResplandorFrenteDefensa({ (float)s * 10.4f, 4.8f, -14.0f }, 4.0f, ConAlfaDefensa(pantalla, 30.0f + flash * 60.0f));
        }

        // Luces de la cercha apuntando a la cancha.
        float topeUltima = TopeFilaDefensa(FILAS_GRADA_DEFENSA - 1);
        float zFondo = ZFilaDefensa(FILAS_GRADA_DEFENSA - 1) - 0.45f;

        for (int k = -3; k <= 3; k++)
        {
            Vector3 foco = { (float)k * 4.4f, topeUltima + 2.25f, zFondo };
            ResplandorFrenteDefensa(foco, 0.7f, ConAlfaDefensa(COLOR_FOCO_DEFENSA, 150.0f));
        }

        // Barritas de luz del publico.
        for (int fila = 0; fila < FILAS_GRADA_DEFENSA; fila++)
        {
            for (int asiento = 0; asiento < ASIENTOS_FILA_DEFENSA; asiento++)
            {
                PersonaGradaDefensa p;

                if (!PersonaDefensa(m, fila, asiento, p) || !p.barrita)
                {
                    continue;
                }

                Color color = ((fila + asiento) & 1) ? COLOR_CIAN_DEFENSA : COLOR_ROSA_DEFENSA;
                ResplandorFrenteDefensa(PuntaBarritaDefensa(p, t, asiento), 0.28f, ConAlfaDefensa(color, 210.0f));
            }
        }

        // Pilones, balizas, celdas y drones.
        for (int sx = -1; sx <= 1; sx += 2)
        {
            for (int sz = -1; sz <= 1; sz += 2)
            {
                Vector2 esquina = VerticeContornoDefensa(sx > 0 ? (sz < 0 ? 0 : 3) : (sz < 0 ? 7 : 4), 1.0f, chaflan);
                Vector3 b = { esquina.x + (float)sx * 1.2f, BASE_AZOTEA_DEFENSA + 1.6f, esquina.y };
                ResplandorFrenteDefensa(b, 0.9f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 120.0f));
                ResplandorSueloDefensa({ b.x, BASE_AZOTEA_DEFENSA + 0.03f, b.z }, 1.6f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 40.0f));
            }
        }

        for (int k = -4; k <= 4; k++)
        {
            ResplandorFrenteDefensa({ (float)k * 2.5f, BASE_AZOTEA_DEFENSA + 0.3f, 8.7f }, 0.5f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 130.0f));
        }

        ResplandorFrenteDefensa({ -13.7f, BASE_AZOTEA_DEFENSA + 0.6f, -5.0f }, 1.2f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 70.0f));
        ResplandorFrenteDefensa({ 13.7f, BASE_AZOTEA_DEFENSA + 0.6f, 4.7f }, 1.2f, ConAlfaDefensa(COLOR_CIAN_DEFENSA, 70.0f));

        for (int k = 0; k < 2; k++)
        {
            Vector3 d = PosicionDronDefensa(k, t);
            bool rojo = std::fmod(t * 2.0f + (float)k, 1.0f) < 0.5f;
            ResplandorFrenteDefensa({ d.x, d.y - 0.1f, d.z }, 0.35f, ConAlfaDefensa(rojo ? Color{ 255, 70, 80, 255 } : Color{ 90, 255, 140, 255 }, 220.0f));
            ResplandorSueloDefensa({ d.x, BASE_AZOTEA_DEFENSA + 0.03f, d.z }, 1.1f, ConAlfaDefensa(COLOR_FOCO_DEFENSA, 30.0f));
        }

        // Baliza de la antena.
        if (std::fmod(t, 1.4f) < 0.4f)
        {
            ResplandorFrenteDefensa({ -16.3f, BASE_AZOTEA_DEFENSA + 9.6f, -14.0f }, 1.3f, ConAlfaDefensa(Color{ 255, 50, 60, 255 }, 210.0f));
        }

        // Reflectores que barren el cielo desde detras de la grada.
        for (int s = -1; s <= 1; s += 2)
        {
            float barrido = std::sin(t * 0.35f + (float)s * 1.3f) * 0.55f;
            Vector3 origen = { (float)s * 13.5f, TopeFilaDefensa(FILAS_GRADA_DEFENSA - 1) + 1.3f, -14.2f };
            Vector3 dir = { (float)s * 0.35f + barrido, 0.82f, -0.45f };
            float largo = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            Vector3 fin = { origen.x + dir.x / largo * 70.0f, origen.y + dir.y / largo * 70.0f, origen.z + dir.z / largo * 70.0f };
            DrawCylinderEx(origen, fin, 0.3f, 4.0f, 14, ConAlfaDefensa(Color{ 190, 200, 255, 255 }, 10.0f));
        }

        DibujarLucesCiudadDefensa(t);
    }

    // Chispas, fuegos artificiales y chispas de energia.
    for (int i = 0; i < MAX_PARTICULAS_DEFENSA_NUCLEO; i++)
    {
        const ParticulaDefensaNucleo& p = m.particulas[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float u = p.vida / p.vidaMaxima;
        ResplandorFrenteDefensa(p.posicion, p.tamano * (1.6f + 1.4f * u), ConAlfaDefensa(p.color, 230.0f * u));
        ResplandorFrenteDefensa(p.posicion, p.tamano * 0.7f, ConAlfaDefensa(WHITE, 200.0f * u));
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// CAMARA Y DIBUJAR
//==================================================

static Camera3D CamaraDefensa(const MinijuegoDefensaNucleo& m)
{
    Camera3D vista = m.camara;

    // Entrada: durante la cuenta la camara llega desde el horizonte
    // (se ven la ciudad y el cielo) y sube hasta la vista de juego.
    if (m.fase == FASE_DEFENSA_NUCLEO_PREPARACION)
    {
        float u = Limitar01Defensa(m.tiempoPreparacion / DURACION_PREPARACION_DEFENSA);
        float s = SuaveDefensa(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.x -= 6.0f * s;
        vista.position.y -= 6.5f * s;
        vista.position.z += 9.0f * s;
        vista.target.y += 3.0f * s;
    }

    return vista;
}


void MinijuegoDefensaNucleo::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    const float t = tiempoAnimacion;
    int limite = LimiteJugadoresDefensa(cantidadMaxima);
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    float chaflan = TEMAS_DEFENSA[indiceTema].chaflan;

    // Tras un gol, las luces de la cancha laten con el color del equipo.
    float flash = 0.0f;
    Color colorFlash = WHITE;

    if (tiempoGolVisual >= 0.0f && tiempoGolVisual < 1.4f && equipoGolVisual >= 0)
    {
        flash = (1.0f - tiempoGolVisual / 1.4f) * (0.55f + 0.45f * std::sin(tiempoGolVisual * 16.0f));
        colorFlash = ObtenerColorEquipoDefensa(equipoGolVisual);
    }

    Camera3D vista = CamaraDefensa(*this);

    ClearBackground(COLOR_CIELO_MEDIO_DEFENSA);
    DibujarCieloDefensa(t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte y media distancia.
    if (decoracion)
    {
        DibujarCiudadDefensa(t, paso, true);
        DibujarAutosVoladoresDefensa(t);
    }

    DibujarAzoteaDefensa(chaflan);

    if (decoracion)
    {
        DibujarGradaDefensa(*this, t, paso, flash, colorFlash);
        DibujarPantallaDefensa(*this, -1.0f, t);
        DibujarPantallaDefensa(*this, 1.0f, t);
        DibujarUtileriaDefensa(t, chaflan);
    }

    // Arena: piso, lineas, vallas, arcos y obstaculo (siempre).
    DibujarPisoCanchaDefensa(chaflan);
    DibujarLineasCanchaDefensa(chaflan, 0.0f, 255.0f, t, flash, colorFlash);
    DibujarVallasDefensa(chaflan, t, flash, colorFlash);
    DibujarArcosDefensa(*this, t);
    DibujarObstaculoDefensa(*this, t);

    DibujarJugadoresDefensa(*this, jugadores, limite, participantes, mostrarDebug);
    DibujarNucleoDefensa(*this, t);
    DibujarParticulasSolidasDefensa(*this);

    if (mostrarDebug)
    {
        DrawCircle3D({ nucleo.x, 0.25f, nucleo.z }, RADIO_NUCLEO_DEFENSA, { 1.0f, 0.0f, 0.0f }, 90.0f, RED);
    }

    DibujarTranslucidosDefensa(*this, jugadores, limite, t, decoracion);

    if (efectos)
    {
        DibujarLucesDefensa(*this, jugadores, limite, t, decoracion, flash, colorFlash);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].equipo < 0 || !participantes[i].activo)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.75f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Anuncios sobre el mundo.
    if (tiempoAnuncio >= 0.0f)
    {
        float u = tiempoAnuncio / DURACION_ANUNCIO_DEFENSA;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
        float y = (float)alto * 0.34f - u * Px(18.0f);
        DibujarTextoMundoContornoUI("¡A JUGAR!", (float)ancho * 0.5f, y, UI_TEXTO_TITULO, Color{ 255, 220, 110, 255 }, alfa);
        DibujarTextoMundoContornoUI("METÉ EL NÚCLEO EN EL ARCO RIVAL", (float)ancho * 0.5f, y + Px(52.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }

    if (
        tiempoGolVisual >= 0.0f &&
        tiempoGolVisual < DURACION_ANUNCIO_GOL_DEFENSA &&
        equipoGolVisual >= 0
    )
    {
        float u = tiempoGolVisual / DURACION_ANUNCIO_GOL_DEFENSA;
        float alfa = u < 0.08f ? u / 0.08f : (u > 0.75f ? (1.0f - u) / 0.25f : 1.0f);
        float lado = LadoArcoEquipoDefensa(1 - equipoGolVisual);
        Vector2 sobre = GetWorldToScreen({ lado * (LIMITE_X_DEFENSA - 0.6f), 3.4f, 0.0f }, vista);
        float x = sobre.x;
        float margen = Px(150.0f);
        if (x > (float)ancho - margen) x = (float)ancho - margen;
        if (x < margen) x = margen;
        float y = sobre.y - u * Px(26.0f);
        Color color = MezclaColorDefensa(ObtenerColorEquipoDefensa(equipoGolVisual), WHITE, 0.15f);

        DibujarTextoMundoContornoUI("¡GOL!", x, y, UI_TEXTO_TITULO, color, alfa);
        DibujarTextoMundoContornoUI(equipoGolVisual == 0 ? "EQUIPO 1" : "EQUIPO 2", x, y + Px(52.0f), UI_TEXTO_GRANDE, UI_TEXTO_MUNDO, alfa);
    }

    if (tiempoAnuncioOro >= 0.0f && fase != FASE_DEFENSA_NUCLEO_TERMINADO)
    {
        float u = tiempoAnuncioOro / DURACION_ANUNCIO_DEFENSA;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
        float y = (float)alto * 0.34f;
        DibujarTextoMundoContornoUI("¡GOL DE ORO!", (float)ancho * 0.5f, y, UI_TEXTO_TITULO, Color{ 255, 214, 92, 255 }, alfa);
        DibujarTextoMundoContornoUI("EL PRÓXIMO GOL GANA", (float)ancho * 0.5f, y + Px(52.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)ancho * 0.5f, (float)alto * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "DEFENSA NÚCLEO",
        "METÉ EL NÚCLEO EN EL ARCO RIVAL Y DEFENDÉ EL TUYO",
        resultado
    );

    hud.fase = fase == FASE_DEFENSA_NUCLEO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_DEFENSA_NUCLEO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;

    if (golDeOro)
    {
        hud.tiempoRestante = tiempoOro;
        hud.tiempoTotal = DURACION_GOL_ORO_DEFENSA;
    }
    else
    {
        hud.tiempoRestante = tiempoRestante;
        hud.tiempoTotal = DURACION_PARTIDA_DEFENSA;
    }

    for (int i = 0; i < limite; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        int equipo = estadosJugadores[i].equipo;

        if (!h.visible || equipo < 0)
        {
            continue;
        }

        int golesEquipo = goles[equipo];
        h.equipo = equipo;
        h.rol = equipo == 0 ? "EQUIPO 1" : "EQUIPO 2";
        std::snprintf(h.valor, sizeof(h.valor), "%d %s", golesEquipo, golesEquipo == 1 ? "GOL" : "GOLES");
        h.valorNumerico = golesEquipo;
        h.progreso = (float)golesEquipo / (float)GOLES_PARA_GANAR_DEFENSA;

        if (equipoGolVisual == equipo && tiempoGolVisual >= 0.0f && tiempoGolVisual < DURACION_PAUSA_GOL_DEFENSA)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡GOL!");
        }

        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d %s", golesEquipo, golesEquipo == 1 ? "GOL" : "GOLES");

        if (!EsControladoPorBotDefensa(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "MOVÉ: %s\nGOLPE CARGADO: %s",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (fase == FASE_DEFENSA_NUCLEO_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        static char nota[48];
        std::snprintf(nota, sizeof(nota), "EQUIPO 1  %d - %d  EQUIPO 2", goles[0], goles[1]);
        hud.notaFinal = nota;
        hud.finPorTiempo = (!golDeOro && tiempoRestante <= 0.0f) || (golDeOro && tiempoOro <= 0.0f);

        if (!empate)
        {
            hud.tituloFinal = equipoGanador == 0 ? "¡GANA EL EQUIPO 1!" : "¡GANA EL EQUIPO 2!";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoDefensaNucleo::ObtenerResultado() const
{
    return resultado;
}
