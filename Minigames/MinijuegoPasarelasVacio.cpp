#include "Minigames/MinijuegoPasarelasVacio.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
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


//==================================================
// PASARELAS DEL VACIO - LOSAS RUNICAS EN LA NEBULOSA
//==================================================
//
// Logica: plataformas (BloquePrueba) que se gastan mientras alguien las
// pisa y caen al vacio. Visual: al final del archivo, separado de la
// logica (estado propio con semilla visual; nunca consume el
// GetRandomValue de los bots).
//==================================================


static const float DURACION_PREPARACION_PASARELAS = 2.5f;
static const float DURACION_CARRERA_PASARELAS = 35.0f;
static const float Z_INICIO_PASARELAS = 5.6f;
static const float Z_META_PASARELAS = -30.8f;
// Segundos de pisada acumulada (de cualquier jugador) que aguanta cada plataforma.
static const float RESISTENCIA_PLATAFORMA_PASARELAS = 1.5f;


static float LimitarPasarelas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float CalcularProgresoPasarelas(const JugadorPrueba& jugador)
{
    return LimitarPasarelas(
        Z_INICIO_PASARELAS - jugador.posicion.z,
        0.0f,
        Z_INICIO_PASARELAS - Z_META_PASARELAS
    );
}


static bool JugadorSobrePlataformaPasarelas(
    const JugadorPrueba& jugador,
    const BloquePrueba& plataforma
)
{
    if (!plataforma.activaColision || jugador.cayendo)
    {
        return false;
    }

    BoundingBox caja = CrearHitboxBloquePrueba(plataforma);
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;

    return
        jugador.posicion.x >= caja.min.x - 0.08f &&
        jugador.posicion.x <= caja.max.x + 0.08f &&
        jugador.posicion.z >= caja.min.z - 0.08f &&
        jugador.posicion.z <= caja.max.z + 0.08f &&
        std::fabs(pies - caja.max.y) <= 0.16f;
}


static int BuscarPlataformaSoportePasarelas(
    const MinijuegoPasarelasVacio& minijuego,
    const JugadorPrueba& jugador
)
{
    for (int i = 0; i < minijuego.cantidadPlataformas; i++)
    {
        if (JugadorSobrePlataformaPasarelas(jugador, minijuego.plataformas[i]))
        {
            return i;
        }
    }

    return -1;
}


static int PuntuacionPasarelas(
    const MinijuegoPasarelasVacio& minijuego,
    int indiceJugador
)
{
    const EstadoJugadorPasarelasVacio& estado =
        minijuego.estadosJugadores[indiceJugador];

    int puntuacion = (int)std::lround(estado.progresoMaximo * 1000.0f);

    if (estado.llegoMeta)
    {
        puntuacion += 100000;
    }
    else if (!estado.eliminado)
    {
        puntuacion += 100;
    }

    return puntuacion;
}


static int ContarCorredoresActivos(
    const MinijuegoPasarelasVacio& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            !minijuego.estadosJugadores[i].llegoMeta
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void FinalizarPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    bool empateForzado
)
{
    int cantidadPrimeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int posicion = 1;
        int puntuacion = PuntuacionPasarelas(minijuego, i);

        if (!empateForzado)
        {
            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    minijuego.resultado.participantes[j].participo &&
                    PuntuacionPasarelas(minijuego, j) > puntuacion
                )
                {
                    posicion++;
                }
            }
        }

        minijuego.estadosJugadores[i].posicionFinal = posicion;
        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego = puntuacion;

        if (posicion == 1)
        {
            cantidadPrimeros++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadPrimeros == 1
            ? DESENLACE_CON_GANADOR
            : DESENLACE_EMPATE;
    minijuego.fase = FASE_PASARELAS_TERMINADO;
}


static InputMinijuegoParticipante CrearEntradaBotPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    int indiceJugador,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorPasarelasVacio& estado =
        minijuego.estadosJugadores[indiceJugador];

    if (estado.indiceObjetivoBot >= minijuego.cantidadPlataformas)
    {
        estado.indiceObjetivoBot = minijuego.cantidadPlataformas - 1;
    }

    const BloquePrueba& objetivo =
        minijuego.plataformas[estado.indiceObjetivoBot];

    int soporte = BuscarPlataformaSoportePasarelas(minijuego, jugador);

    // Al llegar a una plataforma nueva: distancia de salto propia (error de
    // timing) y 15% de probabilidad de dudar entre 0.2 y 0.6 s.
    if (soporte >= 0 && jugador.enSuelo && soporte != estado.ultimoSoporteBot)
    {
        bool primera = estado.ultimoSoporteBot < 0;
        estado.ultimoSoporteBot = soporte;
        estado.desfaseXBot = GetRandomValue(-130, 130) / 100.0f;
        estado.distanciaSaltoBot = GetRandomValue(50, 95) / 100.0f;
        if (primera)
            estado.pausaBot = GetRandomValue(0, 450) / 1000.0f;
        else if (GetRandomValue(1, 100) <= 15)
            estado.pausaBot = GetRandomValue(200, 600) / 1000.0f;
    }

    if (estado.pausaBot > 0.0f)
    {
        estado.pausaBot -= deltaTime;
        return InputMinijuegoParticipante{};
    }

    if (jugador.posicion.x < objetivo.posicion.x + estado.desfaseXBot - 0.22f)
        entrada.derecha = true;
    else if (jugador.posicion.x > objetivo.posicion.x + estado.desfaseXBot + 0.22f)
        entrada.izquierda = true;

    entrada.adelante = true;

    if (soporte >= 0 && jugador.enSuelo)
    {
        const BloquePrueba& plataforma = minijuego.plataformas[soporte];
        float bordeDelantero = plataforma.posicion.z - plataforma.tamano.z * 0.5f;

        if (jugador.posicion.z <= bordeDelantero + estado.distanciaSaltoBot)
        {
            entrada.saltar = true;
        }

        if (
            soporte >= estado.indiceObjetivoBot &&
            estado.indiceObjetivoBot < minijuego.cantidadPlataformas - 1
        )
        {
            estado.indiceObjetivoBot = soporte + 1;
        }
    }

    return entrada;
}


static void ActualizarDerrumbePlataformas(
    MinijuegoPasarelasVacio& minijuego,
    const JugadorPrueba jugadores[],
    float deltaTime
)
{
    for (int plataforma = 1; plataforma < minijuego.cantidadPlataformas - 1; plataforma++)
    {
        EstadoPlataformaPasarelas& estado =
            minijuego.estadosPlataformas[plataforma];
        BloquePrueba& bloque = minijuego.plataformas[plataforma];

        // El presupuesto de estabilidad solo se gasta mientras alguien la
        // pisa: llegar tarde a una plataforma ya no la encuentra rota por
        // los pasos de otros, pero quedarse parado o dudar si la agota.
        estado.ocupada = false;

        if (!estado.cayendo)
        {
            for (int jugador = 0; jugador < MAX_PARTICIPANTES; jugador++)
            {
                if (
                    minijuego.resultado.participantes[jugador].participo &&
                    !minijuego.estadosJugadores[jugador].eliminado &&
                    JugadorSobrePlataformaPasarelas(jugadores[jugador], bloque)
                )
                {
                    estado.ocupada = true;
                    break;
                }
            }
        }

        if (estado.ocupada)
        {
            estado.activada = true;
            estado.tiempoDerrumbe -= deltaTime;

            if (estado.tiempoDerrumbe <= 0.0f)
            {
                estado.tiempoDerrumbe = 0.0f;
                estado.cayendo = true;
                estado.velocidadCaida = 0.8f;
                bloque.activaColision = false;
            }
        }

        if (estado.cayendo)
        {
            estado.velocidadCaida += 10.0f * deltaTime;
            bloque.posicion.y -= estado.velocidadCaida * deltaTime;
        }
    }
}


static void ReiniciarVisualPasarelas(MinijuegoPasarelasVacio& minijuego);

static void ActualizarVisualPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
);


void MinijuegoPasarelasVacio::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_PLATAFORMAS_PASARELAS; i++)
    {
        estadosPlataformas[i] = {};
        estadosPlataformas[i].tiempoDerrumbe = RESISTENCIA_PLATAFORMA_PASARELAS;
        plataformas[i] = {};
    }

    cantidadPlataformas = 0;

    const Vector3 posiciones[MAX_PLATAFORMAS_PASARELAS] =
    {
        {  0.0f, -0.30f,   4.8f },
        { -0.8f, -0.30f,  -0.2f },
        {  1.0f, -0.30f,  -4.7f },
        { -1.3f, -0.30f,  -9.2f },
        {  1.4f, -0.30f, -13.7f },
        { -1.0f, -0.30f, -18.2f },
        {  1.1f, -0.30f, -22.7f },
        {  0.0f, -0.30f, -27.2f },
        {  0.0f, -0.30f, -32.0f }
    };

    const Vector3 tamanos[MAX_PLATAFORMAS_PASARELAS] =
    {
        { 8.4f, 0.60f, 4.6f },
        { 6.8f, 0.60f, 3.2f },
        { 6.2f, 0.60f, 3.2f },
        { 5.8f, 0.60f, 3.1f },
        { 5.4f, 0.60f, 3.0f },
        { 6.0f, 0.60f, 3.0f },
        { 5.5f, 0.60f, 3.0f },
        { 7.0f, 0.60f, 3.3f },
        { 8.4f, 0.60f, 4.2f }
    };

    for (int i = 0; i < MAX_PLATAFORMAS_PASARELAS; i++)
    {
        AgregarBloquePrueba(
            plataformas,
            cantidadPlataformas,
            MAX_PLATAFORMAS_PASARELAS,
            posiciones[i],
            tamanos[i],
            i == MAX_PLATAFORMAS_PASARELAS - 1
                ? Color{ 93, 218, 177, 255 }
                : Color{ 98, 118, 190, 255 }
        );
    }

    camara.position = { 10.5f, 27.0f, 26.6f };
    camara.target = { 0.0f, -0.2f, 4.1f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 19.0f;
    camara.projection = CAMERA_ORTHOGRAPHIC;

    fase = FASE_PASARELAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PASARELAS;
    tiempoRestante = DURACION_CARRERA_PASARELAS;
    tiempoAnimacion = 0.0f;

    ReiniciarVisualPasarelas(*this);
}


void MinijuegoPasarelasVacio::Reiniciar(
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

    const float posicionesX[MAX_PARTICIPANTES] =
    {
        -2.7f, -0.9f, 0.9f, 2.7f
    };

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            { posicionesX[i], 0.70f, Z_INICIO_PASARELAS }
        );
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
    }
}


// Camara ortografica que sigue al grupo: se acerca cuando los jugadores van
// juntos y se aleja lo justo cuando se separan, para que nunca queden
// diminutos ni salgan de pantalla.
static void ActualizarCamaraPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    float deltaTime
)
{
    float zMinima = 1000.0f;
    float zMaxima = -1000.0f;
    int contados = 0;

    for (int i = 0; i < cantidadMaxima && i < MAX_PARTICIPANTES; i++)
    {
        if (
            !minijuego.resultado.participantes[i].participo ||
            minijuego.estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        float z = jugadores[i].posicion.z;
        if (z < zMinima) zMinima = z;
        if (z > zMaxima) zMaxima = z;
        contados++;
    }

    if (contados == 0)
    {
        return;
    }

    float dispersion = zMaxima - zMinima;
    float zFoco = (zMinima + zMaxima) * 0.5f - 1.5f;
    float fovDeseado = 19.0f + dispersion * 0.55f;
    if (fovDeseado > 34.0f) fovDeseado = 34.0f;

    float suavizado = 1.0f - std::exp(-4.0f * deltaTime);

    float objetivoAnterior = minijuego.camara.target.z;
    float objetivoNuevo = objetivoAnterior + (zFoco - objetivoAnterior) * suavizado;

    minijuego.camara.target = { 0.0f, -0.2f, objetivoNuevo };
    minijuego.camara.position =
    {
        10.5f,
        27.0f,
        objetivoNuevo + 22.5f
    };
    minijuego.camara.fovy +=
        (fovDeseado - minijuego.camara.fovy) * suavizado;
}


void MinijuegoPasarelasVacio::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    tiempoAnimacion += deltaTime;
    ActualizarCamaraPasarelas(*this, jugadores, cantidadMaxima, deltaTime);
    ActualizarVisualPasarelas(*this, deltaTime, jugadores, cantidadMaxima, participantes);

    if (fase == FASE_PASARELAS_TERMINADO)
    {
        return;
    }

    if (fase == FASE_PASARELAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        for (int i = 0; i < cantidadMaxima; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PASARELAS_CARRERA;
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante, 5.0f);

    ActualizarDerrumbePlataformas(*this, jugadores, deltaTime);

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorPasarelasVacio& estado = estadosJugadores[i];
        JugadorPrueba& jugador = jugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estado.eliminado ||
            estado.llegoMeta
        )
        {
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotPasarelas(*this, i, jugador, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        entrada.golpear = false;

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            plataformas,
            cantidadPlataformas,
            particulas,
            cantidadParticulas,
            true,
            false,
            deltaTime
        );

        float progreso = CalcularProgresoPasarelas(jugador);
        if (progreso > estado.progresoMaximo)
        {
            estado.progresoMaximo = progreso;
        }

        if (
            jugador.posicion.z <= Z_META_PASARELAS &&
            jugador.enSuelo &&
            JugadorSobrePlataformaPasarelas(
                jugador,
                plataformas[cantidadPlataformas - 1]
            )
        )
        {
            estado.llegoMeta = true;
            FinalizarPasarelas(*this, false);
            return;
        }

        if (jugador.posicion.y < -3.2f)
        {
            jugador.cayendo = true;
            estado.eliminado = true;
        }
    }

    Participante participantesFisica[MAX_PARTICIPANTES];
    for (int i = 0; i < limite; i++)
    {
        participantesFisica[i] = participantes[i];
        participantesFisica[i].conectado = true;
    }

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantesFisica,
        limite
    );

    if (ContarCorredoresActivos(*this) <= 0)
    {
        FinalizarPasarelas(*this, true);
    }
    else if (tiempoRestante <= 0.0f)
    {
        FinalizarPasarelas(*this, false);
    }
}


//==================================================
// VISUAL: PASARELAS RUNICAS EN UNA NEBULOSA
//==================================================
//
// Capas: losas de piedra flotante con runas (arena) -> anclas, cadenas,
// isletas con braseros, cristales y arboles astrales, monolitos (anillo
// cercano) -> islas suspendidas con ruinas, cascadas de polvo estelar y
// un anillo runico gigante (media distancia) -> islas hundidas en dos
// profundidades y nubes de nebulosa (horizonte) -> cielo de nebulosa con
// estrellas y una galaxia (fondo 2D).
//
// La camara de juego (ActualizarCamaraPasarelas) es ortografica; aca se
// convierte en una perspectiva equivalente en el foco para que el vacio
// tenga profundidad. Las losas se dibujan exactamente sobre su BloquePrueba.
//==================================================

static const float DURACION_ANUNCIO_PASARELAS = 1.8f;
static const float DURACION_AVISO_PASARELAS = 1.8f;
static const float FOV_VISTA_PASARELAS = 30.0f;
static const float FONDO_CAIDA_PASARELAS = -70.0f;

// Paleta: piedra lila y roca violeta sobre la bruma de la nebulosa; las
// runas (cian) cuentan el desgaste; el oro se reserva para la meta.
static const Color COLOR_NIEBLA_PASARELAS = { 40, 24, 74, 255 };
static const Color COLOR_PIEDRA_PASARELAS = { 134, 126, 164, 255 };
static const Color COLOR_PIEDRA_CLARA_PASARELAS = { 176, 168, 202, 255 };
static const Color COLOR_JUNTA_PASARELAS = { 58, 48, 84, 255 };
static const Color COLOR_ROCA_PASARELAS = { 96, 80, 124, 255 };
static const Color COLOR_ROCA_OSCURA_PASARELAS = { 60, 48, 84, 255 };
static const Color COLOR_MUSGO_PASARELAS = { 72, 120, 124, 255 };
static const Color COLOR_HIERRO_PASARELAS = { 74, 76, 98, 255 };
static const Color COLOR_RUNA_PASARELAS = { 120, 236, 255, 255 };
static const Color COLOR_GASTADA_PASARELAS = { 255, 168, 74, 255 };
static const Color COLOR_ROTA_PASARELAS = { 244, 76, 66, 255 };
static const Color COLOR_ORO_PASARELAS = { 255, 206, 110, 255 };
static const Color COLOR_LLAMA_PASARELAS = { 250, 120, 220, 255 };   // fuego astral
static const Color COLOR_CRISTAL_PASARELAS = { 176, 140, 255, 255 };
static const Color COLOR_HOJA_PASARELAS = { 255, 150, 206, 255 };

// Base de la camara de juego (ojo - mira) y sus ejes en pantalla.
static const Vector3 DIRECCION_VISTA_PASARELAS = { 10.5f, 27.2f, 22.5f };
static const Vector3 DERECHA_VISTA_PASARELAS = { 0.906f, 0.0f, -0.423f };
static const Vector3 ARRIBA_VISTA_PASARELAS = { -0.313f, 0.674f, -0.670f };

// Ojo para la niebla (se fija al empezar cada Dibujar).
static Vector3 ojoNieblaPasarelas = { 10.5f, 27.0f, 26.6f };


static float Limitar01Pasarelas(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float SuavePasarelas(float u)
{
    u = Limitar01Pasarelas(u);
    return u * u * (3.0f - 2.0f * u);
}


// Hash determinista para la decoracion (misma escena en cada partida).
static float Hash01Pasarelas(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarPasarelas(MinijuegoPasarelasVacio& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasPasarelas(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


// Cuanto se gasto la losa: 0 entera, 1 a punto de caer.
static float GastadoPasarelas(const MinijuegoPasarelasVacio& m, int indice)
{
    if (indice <= 0 || indice >= m.cantidadPlataformas - 1)
    {
        return 0.0f;
    }

    return 1.0f - Limitar01Pasarelas(
        m.estadosPlataformas[indice].tiempoDerrumbe / RESISTENCIA_PLATAFORMA_PASARELAS
    );
}


// Color de las runas: cian entera, naranja a medias, roja casi rota (y
// titila mientras alguien la pisa). La meta brilla en oro.
static Color ColorRunaPasarelas(const MinijuegoPasarelasVacio& m, int indice)
{
    if (indice == m.cantidadPlataformas - 1)
    {
        return COLOR_ORO_PASARELAS;
    }

    const EstadoPlataformaPasarelas& estado = m.estadosPlataformas[indice];

    if (estado.cayendo)
    {
        return Color{ 90, 70, 110, 255 };
    }

    float gastado = GastadoPasarelas(m, indice);
    Color color = gastado < 0.5f
        ? ColorLerp(COLOR_RUNA_PASARELAS, COLOR_GASTADA_PASARELAS, gastado * 2.0f)
        : ColorLerp(COLOR_GASTADA_PASARELAS, COLOR_ROTA_PASARELAS, (gastado - 0.5f) * 2.0f);

    if (estado.ocupada && std::sin(m.tiempoAnimacion * (10.0f + gastado * 16.0f)) > 0.0f)
    {
        color = ColorLerp(color, WHITE, 0.35f);
    }

    return color;
}


//==================================================
// EFECTOS (estado visual propio)
//==================================================

static void EmitirParticulaPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color,
    bool brillo
)
{
    for (int i = 0; i < MAX_PARTICULAS_PASARELAS; i++)
    {
        ParticulaPasarelasVacio& p = minijuego.particulasEfecto[i];

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


// Chispas que suben de las runas cuando alguien pisa la losa por primera vez.
static void EmitirChispasRunaPasarelas(MinijuegoPasarelasVacio& m, Vector3 cima, Vector3 tamano, Color color)
{
    int cantidad = CantidadParticulasPasarelas(16);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarPasarelas(m, 0.0f, 6.2832f);
        float r = AzarPasarelas(m, 0.3f, 1.0f) * (tamano.z < tamano.x ? tamano.z : tamano.x) * 0.3f;
        EmitirParticulaPasarelas(
            m,
            { cima.x + std::cos(a) * r, cima.y + 0.05f, cima.z + std::sin(a) * r },
            { std::cos(a) * 0.4f, AzarPasarelas(m, 1.4f, 2.8f), std::sin(a) * 0.4f },
            AzarPasarelas(m, 0.5f, 0.9f),
            AzarPasarelas(m, 0.06f, 0.1f),
            -0.6f,
            color,
            true
        );
    }
}


static void EmitirPolvoPasarelas(MinijuegoPasarelasVacio& m, Vector3 centro, int base, float radio)
{
    int cantidad = CantidadParticulasPasarelas(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarPasarelas(m, 0.0f, 6.2832f);
        float r = AzarPasarelas(m, 0.0f, radio);
        EmitirParticulaPasarelas(
            m,
            { centro.x + std::cos(a) * r, centro.y + 0.05f, centro.z + std::sin(a) * r },
            { std::cos(a) * AzarPasarelas(m, 0.4f, 1.2f), AzarPasarelas(m, 0.3f, 0.9f), std::sin(a) * AzarPasarelas(m, 0.4f, 1.2f) },
            AzarPasarelas(m, 0.4f, 0.8f),
            AzarPasarelas(m, 0.08f, 0.16f),
            1.2f,
            ColorLerp(COLOR_PIEDRA_CLARA_PASARELAS, COLOR_PIEDRA_PASARELAS, AzarPasarelas(m, 0.0f, 1.0f)),
            false
        );
    }
}


// Desplome: cascotes de la losa, polvo y chispas de runa apagada.
static void EmitirEscombrosPasarelas(MinijuegoPasarelasVacio& m, const BloquePrueba& bloque)
{
    int cantidad = CantidadParticulasPasarelas(26);

    for (int k = 0; k < cantidad; k++)
    {
        Vector3 p = {
            bloque.posicion.x + AzarPasarelas(m, -0.5f, 0.5f) * bloque.tamano.x,
            bloque.posicion.y + AzarPasarelas(m, -0.3f, 0.3f),
            bloque.posicion.z + AzarPasarelas(m, -0.5f, 0.5f) * bloque.tamano.z
        };

        EmitirParticulaPasarelas(
            m,
            p,
            { (p.x - bloque.posicion.x) * 0.5f, AzarPasarelas(m, 0.5f, 2.6f), (p.z - bloque.posicion.z) * 0.6f },
            AzarPasarelas(m, 1.0f, 1.8f),
            AzarPasarelas(m, 0.14f, 0.32f),
            9.0f,
            k % 3 == 0 ? COLOR_ROCA_PASARELAS : COLOR_PIEDRA_PASARELAS,
            false
        );
    }

    EmitirPolvoPasarelas(m, { bloque.posicion.x, bloque.posicion.y + 0.3f, bloque.posicion.z }, 14, bloque.tamano.x * 0.45f);
}


static void EmitirChispasColorPasarelas(MinijuegoPasarelasVacio& m, Vector3 centro, Color color, int base, float fuerza)
{
    int cantidad = CantidadParticulasPasarelas(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarPasarelas(m, 0.0f, 6.2832f);
        float v = AzarPasarelas(m, 0.4f, 1.0f) * fuerza;
        EmitirParticulaPasarelas(
            m,
            centro,
            { std::cos(a) * v, AzarPasarelas(m, 0.6f, 1.6f) * fuerza, std::sin(a) * v },
            AzarPasarelas(m, 0.5f, 1.0f),
            AzarPasarelas(m, 0.07f, 0.13f),
            4.0f,
            k % 3 == 0 ? Color{ 255, 250, 236, 255 } : color,
            true
        );
    }
}


static void EmitirConfetiPasarelas(MinijuegoPasarelasVacio& m, Vector3 centro)
{
    const Color colores[6] =
    {
        { 255, 214, 84, 255 }, { 120, 236, 255, 255 }, { 250, 120, 220, 255 },
        { 176, 140, 255, 255 }, { 150, 230, 160, 255 }, { 250, 246, 236, 255 }
    };

    int cantidad = CantidadParticulasPasarelas(110);

    for (int k = 0; k < cantidad; k++)
    {
        EmitirParticulaPasarelas(
            m,
            {
                centro.x + AzarPasarelas(m, -2.4f, 2.4f),
                centro.y + AzarPasarelas(m, 3.0f, 5.0f),
                centro.z + AzarPasarelas(m, -1.8f, 1.8f)
            },
            { AzarPasarelas(m, -0.8f, 0.8f), AzarPasarelas(m, -0.4f, 1.2f), AzarPasarelas(m, -0.6f, 0.6f) },
            AzarPasarelas(m, 2.4f, 3.6f),
            AzarPasarelas(m, 0.07f, 0.12f),
            1.4f,
            colores[k % 6],
            false
        );
    }
}


static void AgregarAvisoPasarelas(MinijuegoPasarelasVacio& m, int jugador, bool meta, Vector3 posicion)
{
    for (int i = 0; i < MAX_AVISOS_PASARELAS; i++)
    {
        AvisoPasarelasVacio& aviso = m.avisos[i];

        if (aviso.activo)
        {
            continue;
        }

        aviso.activo = true;
        aviso.meta = meta;
        aviso.jugador = jugador;
        aviso.posicion = posicion;
        aviso.vida = DURACION_AVISO_PASARELAS;
        return;
    }
}


static void ReiniciarVisualPasarelas(MinijuegoPasarelasVacio& m)
{
    m.semillaVisual = 2654435761u;
    m.tiempoAnuncio = -1.0f;
    m.carreraVista = false;
    m.finVisto = false;

    for (int i = 0; i < MAX_PLATAFORMAS_PASARELAS; i++)
    {
        m.activadaVista[i] = false;
        m.caidaVista[i] = false;
        m.grietaVista[i] = false;
        m.tiempoDesplome[i] = 0.0f;
        m.destelloPlataforma[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.eliminadoVisto[i] = false;
        m.metaVista[i] = false;
        m.enSueloVisto[i] = false;
        m.destelloMeta[i] = 0.0f;
        m.caidas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_PASARELAS; i++)
    {
        m.particulasEfecto[i] = {};
    }

    for (int i = 0; i < MAX_AVISOS_PASARELAS; i++)
    {
        m.avisos[i] = {};
    }
}


// Una vez por frame: detecta los cambios de la logica y lanza el
// feedback (sonido, particulas, anuncios). Solo lee la logica.
static void ActualizarVisualPasarelas(
    MinijuegoPasarelasVacio& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
)
{
    if (m.fase == FASE_PASARELAS_CARRERA && !m.carreraVista)
    {
        m.carreraVista = true;
        m.tiempoAnuncio = 0.0f;
    }

    if (m.tiempoAnuncio >= 0.0f)
    {
        m.tiempoAnuncio += deltaTime;

        if (m.tiempoAnuncio > DURACION_ANUNCIO_PASARELAS)
        {
            m.tiempoAnuncio = -1.0f;
        }
    }

    // Losas: primera pisada, grieta a mitad de vida y desplome.
    for (int i = 0; i < m.cantidadPlataformas; i++)
    {
        const EstadoPlataformaPasarelas& estado = m.estadosPlataformas[i];
        const BloquePrueba& bloque = m.plataformas[i];
        Vector3 cima = { bloque.posicion.x, bloque.posicion.y + bloque.tamano.y * 0.5f, bloque.posicion.z };

        if (m.destelloPlataforma[i] > 0.0f)
        {
            m.destelloPlataforma[i] -= deltaTime * 2.2f;
        }

        if (estado.activada && !m.activadaVista[i])
        {
            m.activadaVista[i] = true;
            m.destelloPlataforma[i] = 1.0f;
            EmitirChispasRunaPasarelas(m, cima, bloque.tamano, COLOR_RUNA_PASARELAS);
            ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
        }

        if (!estado.cayendo && !m.grietaVista[i] && GastadoPasarelas(m, i) >= 0.55f)
        {
            m.grietaVista[i] = true;
            EmitirPolvoPasarelas(m, cima, 10, 1.2f);
        }

        if (estado.cayendo && !m.caidaVista[i])
        {
            m.caidaVista[i] = true;
            m.tiempoDesplome[i] = 0.0f;
            EmitirEscombrosPasarelas(m, bloque);
            ActivarTemblorCamaraGeneral(0.08f, 0.2f);
            ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
        }

        if (estado.cayendo)
        {
            m.tiempoDesplome[i] += deltaTime;
        }
    }

    // Jugadores: aterrizajes, caidas al vacio y llegada a la meta.
    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const JugadorPrueba& j = jugadores[i];
        const EstadoJugadorPasarelasVacio& e = m.estadosJugadores[i];
        Vector3 pies = { j.posicion.x, j.posicion.y - j.tamano.y * 0.5f, j.posicion.z };

        if (!e.eliminado)
        {
            if (j.enSuelo && !m.enSueloVisto[i] && m.carreraVista)
            {
                EmitirPolvoPasarelas(m, pies, 6, 0.4f);
            }

            m.enSueloVisto[i] = j.enSuelo;
        }

        if (e.eliminado && !m.eliminadoVisto[i])
        {
            m.eliminadoVisto[i] = true;

            CaidaJugadorPasarelas& caida = m.caidas[i];
            caida.activa = true;
            caida.posicion = pies;
            caida.velocidad = { j.velocidad.x * 0.4f, -5.0f, j.velocidad.z * 0.4f };
            caida.tiempo = 0.0f;
            caida.angulo = ObtenerAnguloModeloJugadorCompartido(j);

            EmitirChispasColorPasarelas(m, j.posicion, participantes[i].color, 14, 2.0f);
            AgregarAvisoPasarelas(m, i, false, { j.posicion.x, -0.4f, j.posicion.z });
            ReproducirSonidoMinijuego(m.audio, SONIDO_CAIDA);
        }

        if (e.llegoMeta && !m.metaVista[i])
        {
            m.metaVista[i] = true;
            m.destelloMeta[i] = 1.0f;
            EmitirChispasColorPasarelas(m, j.posicion, COLOR_ORO_PASARELAS, 26, 3.0f);
            EmitirChispasColorPasarelas(m, j.posicion, participantes[i].color, 12, 2.4f);
            AgregarAvisoPasarelas(m, i, true, j.posicion);
            ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
        }

        if (m.destelloMeta[i] > 0.0f)
        {
            m.destelloMeta[i] -= deltaTime * 1.5f;
        }

        CaidaJugadorPasarelas& caida = m.caidas[i];

        if (caida.activa)
        {
            caida.tiempo += deltaTime;
            caida.velocidad.y -= 14.0f * deltaTime;
            if (caida.velocidad.y < -24.0f) caida.velocidad.y = -24.0f;
            caida.posicion = Vector3Add(caida.posicion, Vector3Scale(caida.velocidad, deltaTime));

            if (caida.posicion.y < FONDO_CAIDA_PASARELAS)
            {
                caida.activa = false;
            }
        }
    }

    // Final: confeti sobre el ganador.
    if (m.fase == FASE_PASARELAS_TERMINADO && !m.finVisto)
    {
        m.finVisto = true;
        ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidad = ObtenerIndicesGanadores(m.resultado, ganadores, MAX_PARTICIPANTES);

        for (int k = 0; k < cantidad; k++)
        {
            int g = ganadores[k];

            if (g >= 0 && g < limite && !m.estadosJugadores[g].eliminado)
            {
                EmitirConfetiPasarelas(m, jugadores[g].posicion);
            }
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_PASARELAS; i++)
    {
        ParticulaPasarelasVacio& p = m.particulasEfecto[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        float freno = 1.0f - 0.9f * deltaTime;
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, deltaTime));
    }

    for (int i = 0; i < MAX_AVISOS_PASARELAS; i++)
    {
        AvisoPasarelasVacio& aviso = m.avisos[i];

        if (aviso.activo)
        {
            aviso.vida -= deltaTime;
            if (aviso.vida <= 0.0f) aviso.activo = false;
        }
    }
}


//==================================================
// PRIMITIVAS: COLOR, NIEBLA Y VOLUMEN POR CARAS
//==================================================

static unsigned char CanalPasarelas(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoPasarelas(Color color, float factor)
{
    return Color{
        CanalPasarelas((float)color.r * factor),
        CanalPasarelas((float)color.g * factor),
        CanalPasarelas((float)color.b * factor),
        color.a
    };
}


static Color MezclaPasarelas(Color a, Color b, float t)
{
    t = Limitar01Pasarelas(t);

    return Color{
        CanalPasarelas((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalPasarelas((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalPasarelas((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalPasarelas((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color ConAlfaPasarelas(Color color, float alfa)
{
    color.a = CanalPasarelas(alfa * 255.0f);
    return color;
}


// Caras de arriba con la luz fria de la nebulosa; los costados, algo lilas.
static Color SombraPasarelas(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalPasarelas((float)color.r * factor * 1.02f),
            CanalPasarelas((float)color.g * factor * 1.0f),
            CanalPasarelas((float)color.b * factor * 1.05f),
            color.a
        };
    }

    return Color{
        CanalPasarelas((float)color.r * factor * 0.95f),
        CanalPasarelas((float)color.g * factor * 0.94f),
        CanalPasarelas((float)color.b * factor * 1.08f),
        color.a
    };
}


// Niebla de color por distancia al ojo y por profundidad en el vacio.
static float NieblaEnPasarelas(Vector3 p)
{
    float n = (Vector3Distance(p, ojoNieblaPasarelas) - 40.0f) / 120.0f;

    if (p.y < -2.0f)
    {
        n += (-2.0f - p.y) / 42.0f;
    }

    if (n < 0.0f) return 0.0f;
    return n > 0.9f ? 0.9f : n;
}


static Color NieblaPasarelas(Color color, float cantidad)
{
    Color niebla = COLOR_NIEBLA_PASARELAS;
    niebla.a = color.a;
    return MezclaPasarelas(color, niebla, cantidad);
}


static Color TintePasarelas(Color color, Vector3 p)
{
    return NieblaPasarelas(color, NieblaEnPasarelas(p));
}


static void VerticePasarelas(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadPasarelas(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticePasarelas(a, color);
    VerticePasarelas(b, color);
    VerticePasarelas(c, color);
    VerticePasarelas(d, color);
}


// Caja con caras sombreadas en coordenadas locales (respeta la matriz).
static void CuboLocalPasarelas(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadPasarelas({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraPasarelas(color, 1.0f, true));
    QuadPasarelas({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraPasarelas(color, 0.78f, false));
    QuadPasarelas({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraPasarelas(color, 0.56f, false));
    QuadPasarelas({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraPasarelas(color, 0.62f, false));
    QuadPasarelas({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraPasarelas(color, 0.70f, false));
    QuadPasarelas({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, SombraPasarelas(color, 0.45f, false));
    rlEnd();
}


// Caja en coordenadas de mundo con niebla segun su centro.
static void CuboPasarelas(Vector3 c, Vector3 t, Color color)
{
    CuboLocalPasarelas(c, t, TintePasarelas(color, c));
}


// Luz falsa de una cara lateral segun hacia donde mira (angulo en XZ):
// frente (+Z) 0.78, izquierda 0.70, derecha 0.62, fondo 0.54.
static float LuzLateralPasarelas(float angulo)
{
    return 0.66f + 0.12f * std::sin(angulo) - 0.04f * std::cos(angulo);
}


// Prisma o tronco de cono (coordenadas locales): de base.y con radio r0
// a base.y + alto con radio r1.
static void PrismaLocalPasarelas(Vector3 base, float r0, float r1, float alto, int lados, Color color)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);
        Color cara = SombraPasarelas(color, LuzLateralPasarelas((a0 + a1) * 0.5f), false);

        QuadPasarelas(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, base.y + alto, base.z + s1 * r1 },
            { base.x + c0 * r1, base.y + alto, base.z + s0 * r1 },
            cara
        );
    }

    rlEnd();

    if (r1 > 0.001f)
    {
        Color tapa = SombraPasarelas(color, 1.0f, true);
        rlBegin(RL_TRIANGLES);

        for (int k = 0; k < lados; k++)
        {
            float a0 = 6.2832f * (float)k / (float)lados;
            float a1 = 6.2832f * (float)(k + 1) / (float)lados;
            VerticePasarelas({ base.x, base.y + alto, base.z }, tapa);
            VerticePasarelas({ base.x + std::cos(a1) * r1, base.y + alto, base.z + std::sin(a1) * r1 }, tapa);
            VerticePasarelas({ base.x + std::cos(a0) * r1, base.y + alto, base.z + std::sin(a0) * r1 }, tapa);
        }

        rlEnd();
    }
}


static void PrismaPasarelas(Vector3 base, float r0, float r1, float alto, int lados, Color color)
{
    PrismaLocalPasarelas(base, r0, r1, alto, lados, TintePasarelas(color, base));
}


// Anillo plano horizontal (sin niebla: se usa para marcas y luces).
static void AnilloPasarelas(Vector3 c, float r0, float r1, Color color, int lados = 32)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadPasarelas(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Disco de luz con borde que se apaga (u y v definen el plano).
static void ResplandorPasarelas(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
    }

    rlEnd();
}


static void ResplandorSueloPasarelas(Vector3 c, float radio, Color color)
{
    ResplandorPasarelas(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Halo que mira a la camara.
static void ResplandorFrentePasarelas(Vector3 c, float radio, Color color)
{
    ResplandorPasarelas(c, DERECHA_VISTA_PASARELAS, ARRIBA_VISTA_PASARELAS, radio, color);
}


// Barra orientada de a hasta b (eslabones, vigas, rayos). "girar" la
// rota 90 grados sobre su eje (eslabones alternados de una cadena).
static void BarraPasarelas(Vector3 a, Vector3 b, float ancho, float grueso, bool girar, Color color)
{
    Vector3 d = Vector3Subtract(b, a);
    float largo = Vector3Length(d);

    if (largo < 0.0001f)
    {
        return;
    }

    d = Vector3Scale(d, 1.0f / largo);
    Vector3 referencia = std::fabs(d.y) > 0.92f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    Vector3 s = Vector3Normalize(Vector3CrossProduct(d, referencia));
    Vector3 u = Vector3CrossProduct(s, d);

    if (girar)
    {
        Vector3 temp = s;
        s = u;
        u = Vector3Negate(temp);
    }

    Vector3 sa = Vector3Scale(s, ancho * 0.5f);
    Vector3 ua = Vector3Scale(u, grueso * 0.5f);

    Vector3 a0 = Vector3Subtract(Vector3Subtract(a, sa), ua);
    Vector3 a1 = Vector3Subtract(Vector3Add(a, sa), ua);
    Vector3 a2 = Vector3Add(Vector3Add(a, sa), ua);
    Vector3 a3 = Vector3Add(Vector3Subtract(a, sa), ua);
    Vector3 b0 = Vector3Subtract(Vector3Subtract(b, sa), ua);
    Vector3 b1 = Vector3Subtract(Vector3Add(b, sa), ua);
    Vector3 b2 = Vector3Add(Vector3Add(b, sa), ua);
    Vector3 b3 = Vector3Add(Vector3Subtract(b, sa), ua);

    float luzU = 0.72f + 0.26f * u.y;
    float luzS = 0.70f + 0.12f * s.z - 0.06f * s.x;

    rlBegin(RL_QUADS);
    QuadPasarelas(a3, a2, b2, b3, SombraPasarelas(color, luzU, u.y > 0.5f));
    QuadPasarelas(a0, b0, b1, a1, SombraPasarelas(color, 1.44f - luzU, false));
    QuadPasarelas(a1, b1, b2, a2, SombraPasarelas(color, luzS, false));
    QuadPasarelas(a0, a3, b3, b0, SombraPasarelas(color, 1.36f - luzS, false));
    QuadPasarelas(a0, a1, a2, a3, SombraPasarelas(color, 0.6f, false));
    QuadPasarelas(b0, b3, b2, b1, SombraPasarelas(color, 0.6f, false));
    rlEnd();
}


// Cadena de hierro entre dos puntos con comba hacia abajo.
// MODELO FUTURO: eslabon de cadena GLB instanciado.
static void CadenaPasarelas(Vector3 a, Vector3 b, float comba, int eslabones, Color color)
{
    Vector3 anterior = a;

    for (int k = 1; k <= eslabones; k++)
    {
        float u = (float)k / (float)eslabones;
        Vector3 p = Vector3Lerp(a, b, u);
        p.y -= comba * 4.0f * u * (1.0f - u);

        Vector3 medio = Vector3Lerp(anterior, p, 0.5f);
        BarraPasarelas(
            Vector3Lerp(anterior, medio, -0.15f),
            Vector3Lerp(p, medio, -0.15f),
            0.13f,
            0.05f,
            (k % 2) == 0,
            TintePasarelas(color, p)
        );
        anterior = p;
    }
}


//==================================================
// CIELO DE NEBULOSA (2D, ANTES DEL 3D)
//==================================================

// Nebulosa de colores con franjas de polvo oscuro, estrellas que titilan,
// una galaxia espiral y estrellas fugaces. Se desplaza apenas con la
// carrera (paralaje) para que el fondo acompane el avance.
static void DibujarCieloPasarelas(const MinijuegoPasarelasVacio& m, float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float w = (float)ancho;
    float h = (float)alto;
    float e = h / 720.0f;
    float avance = 4.1f - m.camara.target.z;
    float paraX = -avance * 2.0f * e;
    float paraY = avance * 3.6f * e;

    int h0 = (int)(h * 0.34f);
    int h1 = (int)(h * 0.70f);
    DrawRectangleGradientV(0, 0, ancho, h0, Color{ 16, 10, 40, 255 }, Color{ 44, 20, 76, 255 });
    DrawRectangleGradientV(0, h0, ancho, h1 - h0, Color{ 44, 20, 76, 255 }, Color{ 30, 26, 86, 255 });
    DrawRectangleGradientV(0, h1, ancho, alto - h1, Color{ 30, 26, 86, 255 }, Color{ 14, 30, 64, 255 });

    // Nubes de la nebulosa: grandes y tenues, con nucleos mas vivos.
    const Color tonos[5] =
    {
        { 196, 64, 168, 255 }, { 46, 170, 196, 255 }, { 116, 78, 214, 255 },
        { 240, 120, 150, 255 }, { 60, 90, 200, 255 }
    };

    int nubes = detalle ? 16 : 8;

    for (int k = 0; k < nubes; k++)
    {
        float x = (-0.15f + 1.3f * Hash01Pasarelas(k, 101)) * w + paraX * 0.6f;
        float y = (-0.25f + 1.3f * Hash01Pasarelas(k, 102)) * h + paraY * 0.6f;
        float r = (0.22f + 0.32f * Hash01Pasarelas(k, 103)) * h;
        float respira = 1.0f + 0.05f * std::sin(t * 0.3f + (float)k);
        Color c = tonos[k % 5];
        float a = 0.16f + 0.16f * Hash01Pasarelas(k, 104);
        DrawCircleGradient({ x, y }, r * respira, Fade(c, a), Fade(c, 0.0f));
        DrawCircleGradient({ x + r * 0.25f, y - r * 0.12f }, r * 0.45f, Fade(MezclaPasarelas(c, WHITE, 0.2f), a * 0.8f), Fade(c, 0.0f));
    }

    // Franjas de polvo oscuro que cruzan en diagonal.
    if (detalle)
    {
        for (int franja = 0; franja < 3; franja++)
        {
            float x0 = (0.1f + 0.35f * (float)franja) * w + paraX * 0.7f;
            float y0 = (0.05f + 0.2f * Hash01Pasarelas(franja, 110)) * h + paraY * 0.7f;

            for (int k = 0; k < 9; k++)
            {
                float u = (float)k / 8.0f;
                Vector2 p = { x0 + u * w * 0.32f, y0 + u * h * 0.42f + std::sin(u * 5.0f + (float)franja) * h * 0.04f };
                DrawCircleGradient(p, h * (0.07f + 0.04f * Hash01Pasarelas(franja * 9 + k, 111)), Fade(Color{ 10, 6, 24, 255 }, 0.34f), Fade(Color{ 10, 6, 24, 255 }, 0.0f));
            }
        }
    }

    // Galaxia espiral lejana.
    Vector2 galaxia = { w * 0.84f + paraX * 0.3f, h * 0.17f + paraY * 0.3f };
    DrawCircleGradient(galaxia, 60.0f * e, Fade(Color{ 255, 220, 236, 255 }, 0.35f), Fade(Color{ 200, 120, 220, 255 }, 0.0f));

    for (int brazo = 0; brazo < 2; brazo++)
    {
        for (int k = 0; k < 34; k++)
        {
            float ang = (float)brazo * 3.1416f + (float)k * 0.19f + t * 0.02f;
            float radio = (3.0f + (float)k * 2.2f) * e;
            float px = std::cos(ang) * radio;
            float py = std::sin(ang) * radio * 0.42f;
            Vector2 p = { galaxia.x + px * 0.92f - py * 0.38f, galaxia.y + px * 0.38f + py * 0.92f };
            float a = 0.55f * (1.0f - (float)k / 34.0f);
            DrawCircleV(p, (2.4f - (float)k * 0.045f) * e, Fade(k % 3 == 0 ? Color{ 160, 200, 255, 255 } : Color{ 255, 200, 240, 255 }, a));
        }
    }

    DrawCircleV(galaxia, 3.0f * e, Color{ 255, 246, 236, 255 });

    // Estrellas en tres planos de paralaje.
    int estrellas = detalle ? 190 : 90;

    for (int k = 0; k < estrellas; k++)
    {
        float plano = 0.3f + 0.35f * (float)(k % 3);
        float x = std::fmod(Hash01Pasarelas(k, 1) * w + paraX * plano + w * 4.0f, w);
        float y = std::fmod(Hash01Pasarelas(k, 2) * h + paraY * plano + h * 4.0f, h);
        float brillo = 0.35f + 0.5f * (0.5f + 0.5f * std::sin(t * (0.8f + 2.0f * Hash01Pasarelas(k, 3)) + (float)k));
        float r = (0.8f + 1.3f * Hash01Pasarelas(k, 4)) * e;
        Color c = k % 5 == 0 ? Color{ 200, 230, 255, 255 } : (k % 7 == 0 ? Color{ 255, 220, 240, 255 } : Color{ 255, 250, 244, 255 });
        DrawCircleV({ x, y }, r, Fade(c, brillo));

        if (k % 23 == 0)
        {
            float largo = (7.0f + 5.0f * brillo) * e;
            DrawCircleGradient({ x, y }, largo * 0.9f, Fade(c, 0.25f * brillo), Fade(c, 0.0f));
            DrawLineEx({ x - largo, y }, { x + largo, y }, 1.0f * e, Fade(c, 0.6f * brillo));
            DrawLineEx({ x, y - largo }, { x, y + largo }, 1.0f * e, Fade(c, 0.6f * brillo));
        }
    }

    // Estrella fugaz cada pocos segundos.
    if (detalle)
    {
        float ciclo = 5.5f;
        int indice = (int)(t / ciclo);
        float u = (t - (float)indice * ciclo) / 0.9f;

        if (u < 1.0f)
        {
            Vector2 desde = { (0.15f + 0.6f * Hash01Pasarelas(indice, 120)) * w, (0.05f + 0.3f * Hash01Pasarelas(indice, 121)) * h };
            Vector2 hacia = { desde.x - 220.0f * e, desde.y + 120.0f * e };
            Vector2 cabeza = { desde.x + (hacia.x - desde.x) * u, desde.y + (hacia.y - desde.y) * u };
            Vector2 cola = { desde.x + (hacia.x - desde.x) * (u - 0.35f > 0.0f ? u - 0.35f : 0.0f), desde.y + (hacia.y - desde.y) * (u - 0.35f > 0.0f ? u - 0.35f : 0.0f) };
            float a = std::sin(u * 3.1416f);
            DrawLineEx(cola, cabeza, 2.0f * e, Fade(Color{ 255, 240, 250, 255 }, 0.8f * a));
            DrawCircleGradient(cabeza, 6.0f * e, Fade(WHITE, 0.7f * a), Fade(WHITE, 0.0f));
        }
    }
}


//==================================================
// ISLAS FLOTANTES (HORIZONTE Y MEDIA DISTANCIA)
//==================================================

// Isla flotante: cima con musgo y borde de tierra, y una raiz de roca en
// cono irregular que termina en punta. c es el centro de la cima.
// MODELO FUTURO: isla flotante GLB (tres variantes).
static void IslaPasarelas(Vector3 c, float radio, float alto, int semilla, Color cima, bool detalle)
{
    const int MAXIMO = 11;
    int n = detalle ? 11 : 7;

    Vector3 anillos[4][MAXIMO];
    const float alturas[4] = { 0.0f, -0.16f, -0.5f, -0.8f };
    const float escalas[4] = { 1.0f, 0.95f, 0.62f, 0.3f };

    for (int k = 0; k < n; k++)
    {
        float a = 6.2832f * (float)k / (float)n + Hash01Pasarelas(semilla, k) * 0.3f;
        float r = radio * (0.82f + 0.3f * Hash01Pasarelas(semilla, k + 40));

        for (int anillo = 0; anillo < 4; anillo++)
        {
            float jitter = anillo >= 2 ? 0.8f + 0.4f * Hash01Pasarelas(semilla + anillo, k + 80) : 1.0f;
            float rr = r * escalas[anillo] * jitter;
            anillos[anillo][k] = { c.x + std::cos(a) * rr, c.y + alturas[anillo] * alto, c.z + std::sin(a) * rr };
        }
    }

    Vector3 punta = {
        c.x + (Hash01Pasarelas(semilla, 7) - 0.5f) * radio * 0.4f,
        c.y - alto,
        c.z + (Hash01Pasarelas(semilla, 8) - 0.5f) * radio * 0.4f
    };

    Color tierra = MezclaPasarelas(cima, COLOR_ROCA_PASARELAS, 0.55f);
    float niebla = NieblaEnPasarelas(c);
    float nieblaBaja = NieblaEnPasarelas(punta);

    // Cima (abanico) con variacion por gajo.
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < n; k++)
    {
        int s = (k + 1) % n;
        Color gajo = NieblaPasarelas(SombraPasarelas(TonoPasarelas(cima, 0.92f + 0.14f * Hash01Pasarelas(semilla, k + 120)), 1.0f, true), niebla);
        VerticePasarelas({ c.x, c.y + 0.06f * radio * 0.2f, c.z }, gajo);
        VerticePasarelas(anillos[0][s], gajo);
        VerticePasarelas(anillos[0][k], gajo);
    }

    rlEnd();

    // Banda de tierra y raiz de roca.
    rlBegin(RL_QUADS);

    for (int anillo = 0; anillo < 3; anillo++)
    {
        float oscuro = anillo == 0 ? 1.0f : (anillo == 1 ? 0.92f : 0.74f);
        float cantidad = anillo == 0 ? niebla : niebla + (nieblaBaja - niebla) * (float)anillo / 3.0f;
        Color base = anillo == 0 ? tierra : COLOR_ROCA_PASARELAS;

        for (int k = 0; k < n; k++)
        {
            int s = (k + 1) % n;
            float a = std::atan2((anillos[anillo][k].z + anillos[anillo][s].z) * 0.5f - c.z, (anillos[anillo][k].x + anillos[anillo][s].x) * 0.5f - c.x);
            float variacion = 0.9f + 0.18f * Hash01Pasarelas(semilla + anillo * 31, k);
            Color cara = NieblaPasarelas(SombraPasarelas(base, LuzLateralPasarelas(a) * oscuro * variacion, false), cantidad);
            QuadPasarelas(anillos[anillo][k], anillos[anillo][s], anillos[anillo + 1][s], anillos[anillo + 1][k], cara);
        }
    }

    rlEnd();

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < n; k++)
    {
        int s = (k + 1) % n;
        float a = std::atan2((anillos[3][k].z + anillos[3][s].z) * 0.5f - c.z, (anillos[3][k].x + anillos[3][s].x) * 0.5f - c.x);
        Color cara = NieblaPasarelas(SombraPasarelas(COLOR_ROCA_OSCURA_PASARELAS, LuzLateralPasarelas(a), false), nieblaBaja);
        VerticePasarelas(anillos[3][k], cara);
        VerticePasarelas(anillos[3][s], cara);
        VerticePasarelas(punta, cara);
    }

    rlEnd();
}


static float FlotePasarelas(float t, int semilla, float amplitud)
{
    return std::sin(t * (0.35f + 0.2f * Hash01Pasarelas(semilla, 5)) + Hash01Pasarelas(semilla, 6) * 6.2832f) * amplitud;
}


// Islas hundidas en dos profundidades: siluetas cada vez mas brumosas.
static void DibujarIslasHundidasPasarelas(float t, int paso)
{
    for (int k = 0; k < 26; k += paso)
    {
        bool lejana = k >= 12;
        int s = 300 + k;
        float x = lejana ? -110.0f + 150.0f * Hash01Pasarelas(s, 1) : -72.0f + 98.0f * Hash01Pasarelas(s, 1);
        float z = lejana ? -150.0f + 185.0f * Hash01Pasarelas(s, 2) : -112.0f + 140.0f * Hash01Pasarelas(s, 2);
        float y = lejana ? -54.0f : -32.0f;
        y += Hash01Pasarelas(s, 3) * 6.0f + FlotePasarelas(t, s, 0.5f);

        // Nada justo debajo del camino cercano: ahi se ve la caida.
        if (!lejana && x > -12.0f && x < 12.0f && z > -40.0f)
        {
            x -= 20.0f;
        }

        float radio = (lejana ? 7.0f : 4.5f) * (0.7f + 0.6f * Hash01Pasarelas(s, 4));
        IslaPasarelas({ x, y, z }, radio, radio * 1.5f, s, COLOR_MUSGO_PASARELAS, false);
    }
}


// Nubes de la nebulosa en el fondo del vacio (pasada aditiva propia).
static void DibujarNubesVacioPasarelas(float t, float nivel, int cantidad, float alfa, int semillaBase)
{
    const Color tonos[3] = { { 196, 64, 168, 255 }, { 46, 170, 196, 255 }, { 116, 78, 214, 255 } };

    for (int k = 0; k < cantidad; k++)
    {
        int s = semillaBase + k;
        float deriva = std::sin(t * 0.05f + (float)k) * 3.0f;
        Vector3 c = {
            -95.0f + 140.0f * Hash01Pasarelas(s, 1) + deriva,
            nivel + Hash01Pasarelas(s, 3) * 8.0f,
            -140.0f + 170.0f * Hash01Pasarelas(s, 2)
        };
        float radio = 14.0f + 16.0f * Hash01Pasarelas(s, 4);
        float pulso = 0.85f + 0.15f * std::sin(t * 0.4f + (float)k * 1.7f);
        ResplandorSueloPasarelas(c, radio, ConAlfaPasarelas(tonos[k % 3], alfa * pulso));
    }
}


// Columna rota (local a la base): fuste acanalado, capitel y tambor caido.
// MODELO FUTURO: columna de ruina GLB.
static void ColumnaRotaPasarelas(Vector3 base, float alto, float radio, float niebla)
{
    Color piedra = NieblaPasarelas(COLOR_PIEDRA_CLARA_PASARELAS, niebla);
    CuboLocalPasarelas({ base.x, base.y + 0.12f, base.z }, { radio * 2.6f, 0.24f, radio * 2.6f }, piedra);
    PrismaLocalPasarelas({ base.x, base.y + 0.24f, base.z }, radio, radio * 0.92f, alto, 8, piedra);
    CuboLocalPasarelas({ base.x, base.y + alto + 0.36f, base.z }, { radio * 2.4f, 0.22f, radio * 2.4f }, TonoPasarelas(piedra, 0.95f));
}


// Arco de ruina: dos pilares y dintel de bloques.
// MODELO FUTURO: arco de piedra runica GLB.
static void ArcoRuinaPasarelas(Vector3 base, float ancho, float alto, float giro, float niebla)
{
    Color piedra = NieblaPasarelas(COLOR_PIEDRA_PASARELAS, niebla);
    Color clara = NieblaPasarelas(COLOR_PIEDRA_CLARA_PASARELAS, niebla);

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * ancho * 0.5f;

        for (int k = 0; k < 4; k++)
        {
            float h = alto / 4.0f;
            CuboLocalPasarelas({ x, h * ((float)k + 0.5f), 0.0f }, { 0.7f - 0.04f * (float)k, h * 0.94f, 0.7f }, k % 2 == 0 ? piedra : clara);
        }
    }

    for (int k = 0; k < 5; k++)
    {
        float u = ((float)k + 0.5f) / 5.0f - 0.5f;
        float y = alto + 0.3f + (0.25f - u * u) * 1.6f;
        CuboLocalPasarelas({ u * (ancho + 0.6f), y, 0.0f }, { (ancho + 0.6f) / 5.0f * 0.95f, 0.5f, 0.74f }, k == 2 ? clara : piedra);
    }

    rlPopMatrix();
}


// Arbol astral: tronco retorcido y copa de hojas luminosas rosadas.
// MODELO FUTURO: arbol astral GLB.
static void ArbolAstralPasarelas(Vector3 base, float alto, float t, int semilla, float niebla)
{
    Color tronco = NieblaPasarelas(Color{ 82, 60, 92, 255 }, niebla);
    Vector3 p0 = base;
    Vector3 p1 = { base.x + 0.25f, base.y + alto * 0.45f, base.z - 0.1f };
    Vector3 p2 = { base.x - 0.15f, base.y + alto * 0.8f, base.z + 0.15f };
    BarraPasarelas(p0, p1, 0.36f, 0.36f, false, tronco);
    BarraPasarelas(p1, p2, 0.28f, 0.28f, false, tronco);
    BarraPasarelas(p1, { p1.x + 0.8f, p1.y + 0.7f, p1.z + 0.2f }, 0.16f, 0.16f, false, tronco);

    float vaiven = std::sin(t * 0.9f + (float)semilla) * 0.06f;

    for (int k = 0; k < 5; k++)
    {
        float a = (float)k * 1.26f + Hash01Pasarelas(semilla, k);
        Vector3 c = {
            p2.x + std::cos(a) * 0.6f + vaiven,
            p2.y + 0.2f + Hash01Pasarelas(semilla, k + 10) * 0.6f,
            p2.z + std::sin(a) * 0.6f
        };
        Color hoja = NieblaPasarelas(TonoPasarelas(COLOR_HOJA_PASARELAS, 0.82f + 0.2f * Hash01Pasarelas(semilla, k + 20)), niebla);
        DrawSphereEx(c, 0.5f + 0.2f * Hash01Pasarelas(semilla, k + 30), 4, 7, hoja);
    }
}


// Racimo de cristales lila (prismas inclinados).
// MODELO FUTURO: racimo de cristal GLB.
static void CristalesPasarelas(Vector3 base, float escala, int semilla, float niebla)
{
    for (int k = 0; k < 5; k++)
    {
        float a = (float)k * 1.25f + Hash01Pasarelas(semilla, k) * 0.8f;
        float alto = escala * (0.7f + 0.9f * Hash01Pasarelas(semilla, k + 10)) * (k == 0 ? 1.5f : 1.0f);
        float inclina = k == 0 ? 0.0f : 18.0f + 14.0f * Hash01Pasarelas(semilla, k + 20);
        Color c = NieblaPasarelas(k % 2 == 0 ? COLOR_CRISTAL_PASARELAS : MezclaPasarelas(COLOR_CRISTAL_PASARELAS, COLOR_RUNA_PASARELAS, 0.45f), niebla);

        rlPushMatrix();
        rlTranslatef(base.x + (k == 0 ? 0.0f : std::cos(a) * escala * 0.25f), base.y, base.z + (k == 0 ? 0.0f : std::sin(a) * escala * 0.25f));
        rlRotatef(a * RAD2DEG, 0.0f, 1.0f, 0.0f);
        rlRotatef(inclina, 0.0f, 0.0f, 1.0f);
        PrismaLocalPasarelas({ 0.0f, 0.0f, 0.0f }, escala * 0.2f, escala * 0.17f, alto * 0.75f, 6, c);
        PrismaLocalPasarelas({ 0.0f, alto * 0.75f, 0.0f }, escala * 0.17f, 0.0f, alto * 0.3f, 6, MezclaPasarelas(c, WHITE, 0.2f));
        rlPopMatrix();
    }
}


// Brasero de piedra con cuenco (la llama va en la pasada de luces).
// MODELO FUTURO: brasero runico GLB.
static void BraseroPasarelas(Vector3 base, float niebla)
{
    Color piedra = NieblaPasarelas(COLOR_PIEDRA_PASARELAS, niebla);
    Color hierro = NieblaPasarelas(COLOR_HIERRO_PASARELAS, niebla);
    PrismaLocalPasarelas(base, 0.42f, 0.3f, 0.3f, 8, piedra);
    PrismaLocalPasarelas({ base.x, base.y + 0.3f, base.z }, 0.18f, 0.18f, 0.8f, 8, piedra);
    PrismaLocalPasarelas({ base.x, base.y + 1.1f, base.z }, 0.22f, 0.5f, 0.32f, 10, hierro);
    AnilloPasarelas({ base.x, base.y + 1.43f, base.z }, 0.0f, 0.42f, NieblaPasarelas(Color{ 90, 40, 80, 255 }, niebla), 10);
}


// Farol astral sobre poste (jaula de hierro, luz en la pasada aditiva).
// MODELO FUTURO: farol colgante GLB.
static void FarolPasarelas(Vector3 base, float alto, float niebla)
{
    Color hierro = NieblaPasarelas(COLOR_HIERRO_PASARELAS, niebla);
    PrismaLocalPasarelas(base, 0.12f, 0.09f, alto, 6, hierro);
    CuboLocalPasarelas({ base.x + 0.3f, base.y + alto - 0.05f, base.z }, { 0.7f, 0.08f, 0.08f }, hierro);

    Vector3 jaula = { base.x + 0.58f, base.y + alto - 0.45f, base.z };
    CuboLocalPasarelas({ jaula.x, jaula.y + 0.3f, jaula.z }, { 0.36f, 0.06f, 0.36f }, hierro);
    CuboLocalPasarelas({ jaula.x, jaula.y - 0.3f, jaula.z }, { 0.36f, 0.06f, 0.36f }, hierro);

    for (int k = 0; k < 4; k++)
    {
        float dx = (k % 2 == 0 ? -0.16f : 0.16f);
        float dz = (k < 2 ? -0.16f : 0.16f);
        CuboLocalPasarelas({ jaula.x + dx, jaula.y, jaula.z + dz }, { 0.04f, 0.6f, 0.04f }, hierro);
    }

    CuboLocalPasarelas(jaula, { 0.22f, 0.4f, 0.22f }, NieblaPasarelas(Color{ 200, 250, 255, 255 }, niebla * 0.5f));
}


static Vector3 PosicionFarolLuzPasarelas(Vector3 base, float alto)
{
    return { base.x + 0.58f, base.y + alto - 0.45f, base.z };
}


//==================================================
// MEDIA DISTANCIA: ISLAS SUSPENDIDAS Y ANILLO RUNICO
//==================================================

struct IslaDecoradaPasarelas
{
    Vector3 centro;
    float radio;
    int tipo;           // 0 templo, 1 arboles, 2 cristales, 3 torre en ruinas
};

static const int CANTIDAD_ISLAS_MEDIAS_PASARELAS = 12;

static const IslaDecoradaPasarelas ISLAS_MEDIAS_PASARELAS[CANTIDAD_ISLAS_MEDIAS_PASARELAS] =
{
    { { -21.0f, -10.0f,   2.0f }, 6.0f, 0 },
    { { -18.0f, -13.0f, -17.0f }, 5.0f, 1 },
    { { -25.0f,  -9.0f, -35.0f }, 7.0f, 3 },
    { { -16.0f, -14.0f, -52.0f }, 5.0f, 2 },
    { {  18.0f, -12.0f,  -8.0f }, 5.0f, 1 },
    { {  17.0f, -15.0f, -28.0f }, 6.0f, 0 },
    { {  21.0f, -11.0f, -47.0f }, 5.0f, 2 },
    { { -31.0f, -16.0f,  16.0f }, 6.0f, 1 },
    { {  15.0f, -13.0f,  12.0f }, 4.0f, 2 },
    { {   3.0f, -19.0f, -64.0f }, 6.0f, 0 },
    { {  -3.0f, -22.0f, -22.0f }, 5.0f, 1 },
    { { -36.0f, -12.0f, -62.0f }, 6.0f, 3 }
};


static Vector3 PosicionIslaMediaPasarelas(int k, float t)
{
    Vector3 c = ISLAS_MEDIAS_PASARELAS[k].centro;
    c.y += FlotePasarelas(t, 400 + k, 0.45f);
    return c;
}


// Torre en ruinas: tambores de piedra cada vez mas chicos y la cima rota.
// MODELO FUTURO: torre runica en ruinas GLB.
static void TorreRuinaPasarelas(Vector3 base, float alto, float niebla)
{
    Color piedra = NieblaPasarelas(COLOR_PIEDRA_PASARELAS, niebla);
    Color clara = NieblaPasarelas(COLOR_PIEDRA_CLARA_PASARELAS, niebla);
    float y = base.y;

    for (int k = 0; k < 4; k++)
    {
        float h = alto * 0.25f;
        float lado = 2.2f - 0.3f * (float)k;
        CuboLocalPasarelas({ base.x, y + h * 0.5f, base.z }, { lado, h * 0.92f, lado }, k % 2 == 0 ? piedra : clara);
        CuboLocalPasarelas({ base.x, y + h * 0.5f, base.z + lado * 0.5f + 0.01f }, { 0.4f, h * 0.5f, 0.04f }, NieblaPasarelas(COLOR_JUNTA_PASARELAS, niebla));
        y += h;
    }

    CuboLocalPasarelas({ base.x - 0.4f, y + 0.4f, base.z - 0.3f }, { 0.6f, 0.8f, 0.6f }, piedra);
    CuboLocalPasarelas({ base.x + 0.5f, y + 0.2f, base.z + 0.2f }, { 0.5f, 0.4f, 0.5f }, clara);
}


static void DibujarIslasMediasPasarelas(float t, int paso)
{
    for (int k = 0; k < CANTIDAD_ISLAS_MEDIAS_PASARELAS; k += paso)
    {
        const IslaDecoradaPasarelas& isla = ISLAS_MEDIAS_PASARELAS[k];
        Vector3 c = PosicionIslaMediaPasarelas(k, t);
        float r = isla.radio;
        int s = 400 + k;
        float niebla = NieblaEnPasarelas(c);

        IslaPasarelas(c, r, r * 1.7f, s, COLOR_MUSGO_PASARELAS, true);

        switch (isla.tipo)
        {
            case 0:
                ArcoRuinaPasarelas({ c.x - r * 0.15f, c.y, c.z }, 2.6f, 2.8f, 30.0f + 60.0f * Hash01Pasarelas(s, 9), niebla);
                for (int j = 0; j < 3; j++)
                {
                    float a = (float)j * 2.1f + 0.5f;
                    ColumnaRotaPasarelas({ c.x + std::cos(a) * r * 0.55f, c.y, c.z + std::sin(a) * r * 0.55f }, 1.2f + 1.2f * Hash01Pasarelas(s, j + 12), 0.32f, niebla);
                }
                break;

            case 1:
                for (int j = 0; j < 3; j++)
                {
                    float a = (float)j * 2.1f + Hash01Pasarelas(s, j);
                    ArbolAstralPasarelas({ c.x + std::cos(a) * r * 0.4f, c.y, c.z + std::sin(a) * r * 0.4f }, 2.4f + 1.0f * Hash01Pasarelas(s, j + 3), t, s + j, niebla);
                }
                break;

            case 2:
                CristalesPasarelas(c, 1.8f, s, niebla);
                CristalesPasarelas({ c.x + r * 0.45f, c.y, c.z - r * 0.2f }, 0.9f, s + 1, niebla);
                CristalesPasarelas({ c.x - r * 0.4f, c.y, c.z + r * 0.3f }, 1.0f, s + 2, niebla);
                break;

            default:
                TorreRuinaPasarelas(c, 5.0f, niebla);
                ColumnaRotaPasarelas({ c.x + r * 0.5f, c.y, c.z + r * 0.3f }, 1.6f, 0.34f, niebla);
                ArbolAstralPasarelas({ c.x - r * 0.45f, c.y, c.z + r * 0.35f }, 2.2f, t, s, niebla);
                break;
        }
    }
}


// Anillo runico gigante y roto que gira despacio a la izquierda del camino.
// MODELO FUTURO: anillo de piedras runicas GLB.
static const Vector3 ANILLO_RUNICO_PASARELAS = { -24.0f, -5.0f, -30.0f };
static const float RADIO_ANILLO_RUNICO_PASARELAS = 9.0f;
static const float GIRO_ANILLO_RUNICO_PASARELAS = -32.0f;
static const int PIEDRAS_ANILLO_RUNICO_PASARELAS = 18;


static bool PiedraFaltaAnilloPasarelas(int k)
{
    return k == 4 || k == 5 || k == 13;
}


static Vector3 PiedraAnilloRunicoPasarelas(int k, float t)
{
    float giro = t * 4.0f * DEG2RAD;
    float a = 6.2832f * (float)k / (float)PIEDRAS_ANILLO_RUNICO_PASARELAS + giro;
    float x = std::cos(a) * RADIO_ANILLO_RUNICO_PASARELAS;
    float y = std::sin(a) * RADIO_ANILLO_RUNICO_PASARELAS;
    float fi = GIRO_ANILLO_RUNICO_PASARELAS * DEG2RAD;
    return { ANILLO_RUNICO_PASARELAS.x + x * std::cos(fi), ANILLO_RUNICO_PASARELAS.y + y, ANILLO_RUNICO_PASARELAS.z - x * std::sin(fi) };
}


static void DibujarAnilloRunicoPasarelas(float t)
{
    float niebla = NieblaEnPasarelas(ANILLO_RUNICO_PASARELAS);
    Color piedra = NieblaPasarelas(COLOR_PIEDRA_PASARELAS, niebla);
    Color clara = NieblaPasarelas(COLOR_PIEDRA_CLARA_PASARELAS, niebla);
    Color runa = NieblaPasarelas(MezclaPasarelas(COLOR_JUNTA_PASARELAS, COLOR_RUNA_PASARELAS, 0.7f), niebla * 0.6f);

    rlPushMatrix();
    rlTranslatef(ANILLO_RUNICO_PASARELAS.x, ANILLO_RUNICO_PASARELAS.y, ANILLO_RUNICO_PASARELAS.z);
    rlRotatef(GIRO_ANILLO_RUNICO_PASARELAS, 0.0f, 1.0f, 0.0f);
    rlRotatef(t * 4.0f, 0.0f, 0.0f, 1.0f);

    for (int k = 0; k < PIEDRAS_ANILLO_RUNICO_PASARELAS; k++)
    {
        if (PiedraFaltaAnilloPasarelas(k))
        {
            continue;
        }

        rlPushMatrix();
        rlRotatef(360.0f * (float)k / (float)PIEDRAS_ANILLO_RUNICO_PASARELAS, 0.0f, 0.0f, 1.0f);
        CuboLocalPasarelas({ RADIO_ANILLO_RUNICO_PASARELAS, 0.0f, 0.0f }, { 1.3f, 2.6f, 1.1f }, k % 2 == 0 ? piedra : clara);
        CuboLocalPasarelas({ RADIO_ANILLO_RUNICO_PASARELAS, 0.0f, 0.56f }, { 0.5f, 1.4f, 0.04f }, runa);
        rlPopMatrix();
    }

    rlPopMatrix();
}


//==================================================
// ANILLO CERCANO: ISLETAS, MONOLITOS Y ANCLAS
//==================================================

static const int CANTIDAD_ISLETAS_PASARELAS = 16;

// tipo: 0 brasero, 1 cristales, 2 arbol, 3 columnas, 4 arco, 5 farol
static const IslaDecoradaPasarelas ISLETAS_PASARELAS[CANTIDAD_ISLETAS_PASARELAS] =
{
    { {  -8.6f, -2.2f,   3.0f }, 1.8f, 0 },
    { {   8.2f, -2.8f,   1.0f }, 1.6f, 1 },
    { {  -9.2f, -2.9f,  -6.8f }, 1.9f, 2 },
    { {   8.8f, -2.1f, -10.5f }, 1.7f, 0 },
    { {  -8.4f, -2.5f, -15.8f }, 2.0f, 3 },
    { {   8.4f, -3.0f, -19.6f }, 1.8f, 2 },
    { {  -9.0f, -2.1f, -24.6f }, 1.7f, 0 },
    { {   9.0f, -2.5f, -27.0f }, 1.6f, 1 },
    { {  -9.6f, -2.7f, -34.5f }, 2.2f, 4 },
    { {   9.2f, -2.3f, -35.5f }, 1.8f, 0 },
    { {   7.8f, -2.6f,   9.0f }, 1.5f, 5 },
    { {  -8.0f, -3.0f,  10.5f }, 1.7f, 1 },
    { {  -8.2f, -2.4f,  -1.5f }, 1.4f, 5 },
    { {   8.0f, -2.6f, -31.0f }, 1.4f, 5 },
    { {   8.6f, -2.4f, -14.8f }, 1.4f, 5 },
    { {  -8.4f, -2.6f, -29.6f }, 1.5f, 1 }
};

static const float ALTO_FAROL_PASARELAS = 2.4f;


static Vector3 PosicionIsletaPasarelas(int k, float t)
{
    Vector3 c = ISLETAS_PASARELAS[k].centro;
    c.y += FlotePasarelas(t, 700 + k, 0.18f);
    return c;
}


static void DibujarIsletasPasarelas(float t, int paso)
{
    for (int k = 0; k < CANTIDAD_ISLETAS_PASARELAS; k++)
    {
        const IslaDecoradaPasarelas& isla = ISLETAS_PASARELAS[k];

        // En calidad media quedan los braseros (luces) y la mitad del resto.
        if (paso > 1 && isla.tipo != 0 && k % 2 == 1)
        {
            continue;
        }

        Vector3 c = PosicionIsletaPasarelas(k, t);
        int s = 700 + k;
        float niebla = NieblaEnPasarelas(c);

        IslaPasarelas(c, isla.radio, isla.radio * 1.8f, s, COLOR_MUSGO_PASARELAS, true);

        // Cascotes sueltos en el borde.
        for (int j = 0; j < 3; j++)
        {
            float a = (float)j * 2.3f + Hash01Pasarelas(s, j + 50) * 2.0f;
            float e = 0.18f + 0.16f * Hash01Pasarelas(s, j + 60);
            CuboLocalPasarelas(
                { c.x + std::cos(a) * isla.radio * 0.7f, c.y + e * 0.5f, c.z + std::sin(a) * isla.radio * 0.7f },
                { e * 1.4f, e, e * 1.1f },
                NieblaPasarelas(COLOR_PIEDRA_PASARELAS, niebla)
            );
        }

        switch (isla.tipo)
        {
            case 0:
                BraseroPasarelas(c, niebla);
                break;

            case 1:
                CristalesPasarelas(c, 1.0f, s, niebla);
                break;

            case 2:
                ArbolAstralPasarelas(c, 2.6f, t, s, niebla);
                break;

            case 3:
                ColumnaRotaPasarelas({ c.x - 0.5f, c.y, c.z - 0.3f }, 1.8f, 0.3f, niebla);
                ColumnaRotaPasarelas({ c.x + 0.6f, c.y, c.z + 0.4f }, 0.9f, 0.3f, niebla);
                CuboLocalPasarelas({ c.x + 0.2f, c.y + 0.22f, c.z - 0.9f }, { 0.9f, 0.44f, 0.5f }, NieblaPasarelas(COLOR_PIEDRA_CLARA_PASARELAS, niebla));
                break;

            case 4:
                ArcoRuinaPasarelas(c, 1.8f, 1.9f, 70.0f, niebla);
                break;

            default:
                FarolPasarelas({ c.x - 0.3f, c.y, c.z }, ALTO_FAROL_PASARELAS, niebla);
                break;
        }
    }
}


struct MonolitoPasarelasDatos
{
    Vector3 centro;
    float alto;
};

static const int CANTIDAD_MONOLITOS_PASARELAS = 10;

static const MonolitoPasarelasDatos MONOLITOS_PASARELAS[CANTIDAD_MONOLITOS_PASARELAS] =
{
    { { -6.6f, 1.2f,   1.0f }, 2.4f },
    { {  6.8f, 0.6f,  -2.5f }, 2.0f },
    { { -7.0f, 0.8f, -11.5f }, 2.8f },
    { {  6.6f, 1.4f,  -7.0f }, 2.2f },
    { { -6.8f, 1.6f, -20.5f }, 2.4f },
    { {  7.0f, 0.8f, -16.6f }, 2.6f },
    { { -7.2f, 1.0f, -28.6f }, 2.2f },
    { {  6.8f, 1.2f, -24.0f }, 2.0f },
    { {  6.8f, 1.0f,   5.6f }, 2.2f },
    { { -6.4f, 1.4f, -38.0f }, 2.6f }
};


static Vector3 PosicionMonolitoPasarelas(int k, float t)
{
    Vector3 c = MONOLITOS_PASARELAS[k].centro;
    c.y += FlotePasarelas(t, 900 + k, 0.3f);
    return c;
}


// Monolitos runicos que flotan y giran despacio junto al camino, con
// piedritas en orbita. MODELO FUTURO: monolito runico GLB.
static void DibujarMonolitosPasarelas(float t, int paso)
{
    for (int k = 0; k < CANTIDAD_MONOLITOS_PASARELAS; k += paso)
    {
        Vector3 c = PosicionMonolitoPasarelas(k, t);
        float alto = MONOLITOS_PASARELAS[k].alto;
        float niebla = NieblaEnPasarelas(c);
        Color piedra = NieblaPasarelas(COLOR_PIEDRA_PASARELAS, niebla);
        Color clara = NieblaPasarelas(COLOR_PIEDRA_CLARA_PASARELAS, niebla);
        Color roca = NieblaPasarelas(COLOR_ROCA_PASARELAS, niebla);
        Color runa = NieblaPasarelas(MezclaPasarelas(COLOR_JUNTA_PASARELAS, COLOR_RUNA_PASARELAS, 0.75f), niebla);

        rlPushMatrix();
        rlTranslatef(c.x, c.y, c.z);
        rlRotatef(t * 12.0f + 40.0f * (float)k, 0.0f, 1.0f, 0.0f);
        rlRotatef(std::sin(t * 0.7f + (float)k) * 5.0f, 0.0f, 0.0f, 1.0f);

        CuboLocalPasarelas({ 0.0f, 0.0f, 0.0f }, { 0.95f, alto, 0.42f }, piedra);
        CuboLocalPasarelas({ 0.0f, alto * 0.5f + 0.12f, 0.0f }, { 0.72f, 0.24f, 0.34f }, clara);
        CuboLocalPasarelas({ 0.08f, -alto * 0.5f - 0.18f, 0.0f }, { 0.6f, 0.36f, 0.34f }, roca);

        for (int cara = -1; cara <= 1; cara += 2)
        {
            float z = (float)cara * 0.215f;
            CuboLocalPasarelas({ 0.0f, alto * 0.25f, z }, { 0.5f, 0.06f, 0.02f }, runa);
            CuboLocalPasarelas({ 0.0f, alto * 0.08f, z }, { 0.06f, 0.4f, 0.02f }, runa);
            CuboLocalPasarelas({ -0.14f, -alto * 0.15f, z }, { 0.06f, 0.3f, 0.02f }, runa);
            CuboLocalPasarelas({ 0.12f, -alto * 0.2f, z }, { 0.26f, 0.06f, 0.02f }, runa);
        }

        rlPopMatrix();

        for (int j = 0; j < 3; j++)
        {
            float a = t * 0.8f + (float)j * 2.1f + (float)k;
            float e = 0.12f + 0.06f * (float)j;
            CuboLocalPasarelas({ c.x + std::cos(a) * 0.95f, c.y - alto * 0.3f + std::sin(a * 1.3f) * 0.2f, c.z + std::sin(a) * 0.95f }, { e, e, e }, roca);
        }
    }
}


// Punto de anclaje en el costado de una losa y la roca que la sujeta.
static Vector3 RocaAnclaPasarelas(const BloquePrueba& bloque, int indice, int lado)
{
    float semilla = Hash01Pasarelas(indice * 2 + (lado > 0 ? 1 : 0), 77);
    return {
        bloque.posicion.x + (float)lado * (bloque.tamano.x * 0.5f + 2.6f + 0.8f * semilla),
        -6.4f - 1.4f * semilla,
        bloque.posicion.z + (semilla - 0.5f) * 1.2f
    };
}


// Desplazamiento de una losa en la cuenta regresiva: suben del vacio en
// ola con un pequeno rebote (solo visual; la colision ya esta en su sitio).
static float DesplazamientoAparicionPasarelas(const MinijuegoPasarelasVacio& m, int indice)
{
    if (indice <= 0 || m.fase != FASE_PASARELAS_PREPARACION || ObtenerFactorMovimientoCamaraMinijuegos() <= 0.0f)
    {
        return 0.0f;
    }

    float transcurrido = DURACION_PREPARACION_PASARELAS - m.tiempoPreparacion;
    float u = (transcurrido - 0.12f * (float)indice) / 0.75f;

    if (u >= 1.0f) return 0.0f;
    if (u <= 0.0f) return 8.0f;

    float v = u - 1.0f;
    float e = 1.0f + 2.70158f * v * v * v + 1.70158f * v * v;
    return (1.0f - e) * 8.0f;
}


// Cadenas que atan cada losa a rocas ancla mas abajo. Si la losa cae, la
// cadena queda colgando de la roca.
static void DibujarAnclasPasarelas(const MinijuegoPasarelasVacio& m, float t, int paso)
{
    for (int i = 0; i < m.cantidadPlataformas; i++)
    {
        const BloquePrueba& bloque = m.plataformas[i];
        const EstadoPlataformaPasarelas& estado = m.estadosPlataformas[i];
        float baja = DesplazamientoAparicionPasarelas(m, i);
        float yAncla = bloque.posicion.y - baja - 0.02f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 roca = RocaAnclaPasarelas(bloque, i, lado);
            roca.y += FlotePasarelas(t, 1000 + i * 2 + lado, 0.12f);
            int s = 1000 + i * 2 + (lado > 0 ? 1 : 0);

            IslaPasarelas(roca, 1.0f, 1.8f, s, COLOR_MUSGO_PASARELAS, false);
            PrismaPasarelas(roca, 0.16f, 0.12f, 0.7f, 6, COLOR_HIERRO_PASARELAS);
            Vector3 poste = { roca.x, roca.y + 0.62f, roca.z };

            if (estado.cayendo)
            {
                // Cadena cortada colgando de la roca.
                CadenaPasarelas(poste, { poste.x - (float)lado * 0.4f, poste.y - 3.2f, poste.z + 0.2f }, 0.2f, 12, COLOR_HIERRO_PASARELAS);
                continue;
            }

            int cadenas = paso > 1 ? 1 : 2;

            for (int c = 0; c < cadenas; c++)
            {
                float dz = cadenas == 1 ? 0.0f : (c == 0 ? -0.28f : 0.28f) * bloque.tamano.z;
                Vector3 ancla = {
                    bloque.posicion.x + (float)lado * (bloque.tamano.x * 0.5f + 0.05f),
                    yAncla,
                    bloque.posicion.z + dz
                };

                CuboPasarelas(ancla, { 0.1f, 0.34f, 0.34f }, COLOR_HIERRO_PASARELAS);
                int eslabones = (int)(Vector3Distance(ancla, poste) / 0.26f);
                CadenaPasarelas(ancla, poste, 0.7f, eslabones, COLOR_HIERRO_PASARELAS);
            }
        }
    }
}


//==================================================
// ARENA: LOSAS DE PIEDRA FLOTANTE CON RUNAS
//==================================================

static float RadioRunaPasarelas(Vector3 tamano)
{
    float a = tamano.x - 0.52f;
    float b = tamano.z - 0.52f;
    return (a < b ? a : b) * 0.36f;
}


static Vector3 CentroRunaPasarelas(Vector3 cima, bool meta)
{
    return { cima.x, cima.y, cima.z + (meta ? -0.25f : 0.0f) };
}


// Trazo plano sobre la losa (grietas, glifos, lineas) de p0 a p1.
static void TrazoPasarelas(Vector3 p0, Vector3 p1, float ancho, Color color)
{
    float dx = p1.x - p0.x;
    float dz = p1.z - p0.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.0001f)
    {
        return;
    }

    float nx = -dz / largo * ancho * 0.5f;
    float nz = dx / largo * ancho * 0.5f;
    QuadPasarelas(
        { p0.x + nx, p0.y, p0.z + nz },
        { p1.x + nx, p1.y, p1.z + nz },
        { p1.x - nx, p1.y, p1.z - nz },
        { p0.x - nx, p0.y, p0.z - nz },
        color
    );
}


// Losa completa en coordenadas locales de su BloquePrueba (c = centro de
// la caja de colision): costados con hiladas, bordillo de piedra clara,
// baldosas, circulo de runas, grietas segun el desgaste y raiz de roca.
// MODELO FUTURO: losa runica flotante GLB (con variante partida).
static void DibujarLosaPasarelas(
    Vector3 c,
    Vector3 tam,
    float niebla,
    Color runa,
    float gastado,
    int semilla,
    bool detalle,
    float raiz,
    bool meta
)
{
    float x0 = c.x - tam.x * 0.5f, x1 = c.x + tam.x * 0.5f;
    float z0 = c.z - tam.z * 0.5f, z1 = c.z + tam.z * 0.5f;
    float yTop = c.y + tam.y * 0.5f;
    float yBase = c.y - tam.y * 0.5f;
    float yMedio = yTop - tam.y * 0.42f;

    Color piedra = meta ? MezclaPasarelas(COLOR_PIEDRA_PASARELAS, COLOR_ORO_PASARELAS, 0.16f) : COLOR_PIEDRA_PASARELAS;
    if (gastado > 0.5f) piedra = MezclaPasarelas(piedra, COLOR_ROTA_PASARELAS, (gastado - 0.5f) * 0.3f);
    piedra = NieblaPasarelas(piedra, niebla);
    Color clara = NieblaPasarelas(meta ? MezclaPasarelas(COLOR_PIEDRA_CLARA_PASARELAS, COLOR_ORO_PASARELAS, 0.3f) : COLOR_PIEDRA_CLARA_PASARELAS, niebla);
    Color junta = NieblaPasarelas(COLOR_JUNTA_PASARELAS, niebla);
    Color roca = NieblaPasarelas(COLOR_ROCA_PASARELAS, niebla);

    // Tapa de juntas y costados en dos hiladas (cornisa clara arriba).
    rlBegin(RL_QUADS);
    QuadPasarelas({ x0, yTop, z1 }, { x1, yTop, z1 }, { x1, yTop, z0 }, { x0, yTop, z0 }, SombraPasarelas(junta, 1.0f, true));

    const float luces[4] = { 0.78f, 0.62f, 0.56f, 0.70f };   // +Z, +X, -Z, -X

    for (int hilada = 0; hilada < 2; hilada++)
    {
        float ya = hilada == 0 ? yMedio : yBase;
        float yb = hilada == 0 ? yTop : yMedio;
        Color base = hilada == 0 ? clara : piedra;

        QuadPasarelas({ x0, ya, z1 }, { x1, ya, z1 }, { x1, yb, z1 }, { x0, yb, z1 }, SombraPasarelas(base, luces[0], false));
        QuadPasarelas({ x1, ya, z1 }, { x1, ya, z0 }, { x1, yb, z0 }, { x1, yb, z1 }, SombraPasarelas(base, luces[1], false));
        QuadPasarelas({ x1, ya, z0 }, { x0, ya, z0 }, { x0, yb, z0 }, { x1, yb, z0 }, SombraPasarelas(base, luces[2], false));
        QuadPasarelas({ x0, ya, z0 }, { x0, ya, z1 }, { x0, yb, z1 }, { x0, yb, z0 }, SombraPasarelas(base, luces[3], false));
    }

    // Junta entre hiladas y bloques del frente.
    float yj = yMedio + 0.015f;
    QuadPasarelas({ x0, yj - 0.03f, z1 + 0.005f }, { x1, yj - 0.03f, z1 + 0.005f }, { x1, yj, z1 + 0.005f }, { x0, yj, z1 + 0.005f }, SombraPasarelas(junta, 0.8f, false));
    QuadPasarelas({ x1 + 0.005f, yj - 0.03f, z1 }, { x1 + 0.005f, yj - 0.03f, z0 }, { x1 + 0.005f, yj, z0 }, { x1 + 0.005f, yj, z1 }, SombraPasarelas(junta, 0.7f, false));

    int bloquesFrente = (int)(tam.x / 1.1f);
    for (int k = 1; k < bloquesFrente; k++)
    {
        float x = x0 + tam.x * (float)k / (float)bloquesFrente + ((k % 2) ? 0.15f : -0.1f);
        QuadPasarelas({ x - 0.015f, yBase, z1 + 0.006f }, { x + 0.015f, yBase, z1 + 0.006f }, { x + 0.015f, yMedio, z1 + 0.006f }, { x - 0.015f, yMedio, z1 + 0.006f }, SombraPasarelas(junta, 0.75f, false));
    }

    int bloquesLado = (int)(tam.z / 1.1f);
    for (int k = 1; k < bloquesLado; k++)
    {
        float z = z0 + tam.z * (float)k / (float)bloquesLado;
        QuadPasarelas({ x1 + 0.006f, yBase, z - 0.015f }, { x1 + 0.006f, yBase, z + 0.015f }, { x1 + 0.006f, yMedio, z + 0.015f }, { x1 + 0.006f, yMedio, z - 0.015f }, SombraPasarelas(junta, 0.65f, false));
    }

    QuadPasarelas({ x0, yBase, z0 }, { x1, yBase, z0 }, { x1, yBase, z1 }, { x0, yBase, z1 }, SombraPasarelas(roca, 0.45f, false));
    rlEnd();

    // Bordillo: piedras claras en todo el perimetro (marca el borde real).
    float ySup = yTop + 0.004f;
    const float BORDE = 0.24f;
    rlBegin(RL_QUADS);

    int piezasX = (int)(tam.x / 0.95f);
    if (piezasX < 2) piezasX = 2;

    for (int k = 0; k < piezasX; k++)
    {
        float a = x0 + tam.x * (float)k / (float)piezasX + 0.02f;
        float b = x0 + tam.x * (float)(k + 1) / (float)piezasX - 0.02f;
        float v1 = 0.92f + 0.14f * Hash01Pasarelas(semilla, k + 200);
        float v2 = 0.92f + 0.14f * Hash01Pasarelas(semilla, k + 300);
        QuadPasarelas({ a, ySup, z1 - 0.02f }, { b, ySup, z1 - 0.02f }, { b, ySup, z1 - BORDE }, { a, ySup, z1 - BORDE }, SombraPasarelas(clara, v1, true));
        QuadPasarelas({ a, ySup, z0 + BORDE }, { b, ySup, z0 + BORDE }, { b, ySup, z0 + 0.02f }, { a, ySup, z0 + 0.02f }, SombraPasarelas(clara, v2, true));
    }

    int piezasZ = (int)((tam.z - BORDE * 2.0f) / 0.95f);
    if (piezasZ < 1) piezasZ = 1;

    for (int k = 0; k < piezasZ; k++)
    {
        float a = z0 + BORDE + (tam.z - BORDE * 2.0f) * (float)k / (float)piezasZ + 0.02f;
        float b = z0 + BORDE + (tam.z - BORDE * 2.0f) * (float)(k + 1) / (float)piezasZ - 0.02f;
        float v1 = 0.92f + 0.14f * Hash01Pasarelas(semilla, k + 400);
        float v2 = 0.92f + 0.14f * Hash01Pasarelas(semilla, k + 500);
        QuadPasarelas({ x0 + 0.02f, ySup, b }, { x0 + BORDE, ySup, b }, { x0 + BORDE, ySup, a }, { x0 + 0.02f, ySup, a }, SombraPasarelas(clara, v1, true));
        QuadPasarelas({ x1 - BORDE, ySup, b }, { x1 - 0.02f, ySup, b }, { x1 - 0.02f, ySup, a }, { x1 - BORDE, ySup, a }, SombraPasarelas(clara, v2, true));
    }

    // Baldosas interiores a soga (filas corridas media pieza).
    float xi0 = x0 + BORDE + 0.03f, xi1 = x1 - BORDE - 0.03f;
    float zi0 = z0 + BORDE + 0.03f, zi1 = z1 - BORDE - 0.03f;
    int filas = (int)((zi1 - zi0) / 0.9f + 0.5f);
    if (filas < 2) filas = 2;
    int columnas = (int)((xi1 - xi0) / 1.05f + 0.5f);
    if (columnas < 2) columnas = 2;
    float anchoBaldosa = (xi1 - xi0) / (float)columnas;
    float altoFila = (zi1 - zi0) / (float)filas;

    for (int f = 0; f < filas; f++)
    {
        float za = zi0 + altoFila * (float)f + 0.025f;
        float zb = zi0 + altoFila * (float)(f + 1) - 0.025f;
        float corrimiento = (f % 2) ? anchoBaldosa * 0.5f : 0.0f;

        for (int k = 0; k <= columnas; k++)
        {
            float a = xi0 + anchoBaldosa * (float)k - corrimiento + 0.025f;
            float b = a + anchoBaldosa - 0.05f;
            if (a < xi0) a = xi0;
            if (b > xi1) b = xi1;
            if (b - a < 0.12f) continue;

            float variacion = 0.88f + 0.22f * Hash01Pasarelas(semilla + f * 31, k);
            QuadPasarelas({ a, ySup, zb }, { b, ySup, zb }, { b, ySup, za }, { a, ySup, za }, SombraPasarelas(piedra, variacion, true));
        }
    }

    rlEnd();

    // Circulo de runas: canales tallados que brillan con el estado.
    Vector3 centro = CentroRunaPasarelas({ c.x, yTop + 0.01f, c.z }, meta);
    float radio = RadioRunaPasarelas(tam);
    Color canal = NieblaPasarelas(MezclaPasarelas(COLOR_JUNTA_PASARELAS, runa, 0.82f), niebla);

    AnilloPasarelas(centro, radio - 0.09f, radio, canal, 36);
    AnilloPasarelas(centro, radio * 0.55f - 0.06f, radio * 0.55f, canal, 28);

    rlBegin(RL_QUADS);

    for (int k = 0; k < 8; k++)
    {
        float a = 6.2832f * (float)k / 8.0f + 0.39f;
        float ca = std::cos(a), sa = std::sin(a);
        Vector3 p0 = { centro.x + ca * radio * 0.62f, centro.y, centro.z + sa * radio * 0.62f };
        Vector3 p1 = { centro.x + ca * radio * 0.86f, centro.y, centro.z + sa * radio * 0.86f };
        TrazoPasarelas(p0, p1, 0.06f, canal);

        if (k % 2 == 0)
        {
            Vector3 m0 = { centro.x + ca * radio * 0.76f - sa * 0.1f, centro.y, centro.z + sa * radio * 0.76f + ca * 0.1f };
            Vector3 m1 = { centro.x + ca * radio * 0.76f + sa * 0.1f, centro.y, centro.z + sa * radio * 0.76f - ca * 0.1f };
            TrazoPasarelas(m0, m1, 0.05f, canal);
        }
    }

    // Grietas que crecen con el desgaste.
    if (gastado > 0.12f)
    {
        int grietas = (int)(gastado * 7.0f);
        if (grietas > 7) grietas = 7;
        Color oscura = NieblaPasarelas(Color{ 26, 16, 36, 255 }, niebla);
        float limiteX = tam.x * 0.5f - BORDE;
        float limiteZ = tam.z * 0.5f - BORDE;

        for (int g = 0; g < grietas; g++)
        {
            float a = 6.2832f * Hash01Pasarelas(semilla + 7, g);
            Vector3 p = { centro.x + std::cos(a) * radio * 0.4f, yTop + 0.016f, centro.z + std::sin(a) * radio * 0.4f };

            for (int tramo = 0; tramo < 3; tramo++)
            {
                a += (Hash01Pasarelas(semilla + g * 5, tramo) - 0.5f) * 1.1f;
                float largo = 0.35f + 0.3f * Hash01Pasarelas(semilla + g, tramo + 20);
                Vector3 q = { p.x + std::cos(a) * largo, p.y, p.z + std::sin(a) * largo };
                if (q.x < c.x - limiteX) q.x = c.x - limiteX;
                if (q.x > c.x + limiteX) q.x = c.x + limiteX;
                if (q.z < c.z - limiteZ) q.z = c.z - limiteZ;
                if (q.z > c.z + limiteZ) q.z = c.z + limiteZ;
                TrazoPasarelas(p, q, 0.06f - 0.012f * (float)tramo, oscura);
                p = q;
            }
        }
    }

    rlEnd();

    // Raiz de roca bajo la losa (cono irregular hasta una punta).
    if (raiz > 0.0f)
    {
        const int N = 12;
        Vector3 anillos[3][N];
        const float escalas[3] = { 1.0f, 0.74f, 0.4f };
        const float bajadas[3] = { 0.0f, 0.36f, 0.72f };
        float hx = tam.x * 0.5f, hz = tam.z * 0.5f;

        for (int k = 0; k < N; k++)
        {
            float a = 6.2832f * (float)k / (float)N + 0.26f;
            float ca = std::cos(a), sa = std::sin(a);
            float tx = std::fabs(ca) > 0.0001f ? hx / std::fabs(ca) : 1000.0f;
            float tz = std::fabs(sa) > 0.0001f ? hz / std::fabs(sa) : 1000.0f;
            float borde = tx < tz ? tx : tz;

            for (int anillo = 0; anillo < 3; anillo++)
            {
                float jitter = anillo == 0 ? 1.0f : 0.82f + 0.36f * Hash01Pasarelas(semilla + anillo * 13, k);
                float r = borde * escalas[anillo] * jitter;
                anillos[anillo][k] = { c.x + ca * r, yBase - raiz * bajadas[anillo], c.z + sa * r };
            }
        }

        Vector3 punta = { c.x + (Hash01Pasarelas(semilla, 90) - 0.5f) * 0.8f, yBase - raiz, c.z + (Hash01Pasarelas(semilla, 91) - 0.5f) * 0.6f };

        rlBegin(RL_QUADS);

        for (int anillo = 0; anillo < 2; anillo++)
        {
            for (int k = 0; k < N; k++)
            {
                int s = (k + 1) % N;
                float a = 6.2832f * ((float)k + 0.5f) / (float)N + 0.26f;
                float luz = LuzLateralPasarelas(a) * (anillo == 0 ? 0.95f : 0.78f) * (0.9f + 0.18f * Hash01Pasarelas(semilla + anillo, k + 40));
                QuadPasarelas(anillos[anillo][k], anillos[anillo][s], anillos[anillo + 1][s], anillos[anillo + 1][k], SombraPasarelas(roca, luz, false));
            }
        }

        rlEnd();

        Color oscura = NieblaPasarelas(COLOR_ROCA_OSCURA_PASARELAS, niebla);
        rlBegin(RL_TRIANGLES);

        for (int k = 0; k < N; k++)
        {
            int s = (k + 1) % N;
            float a = 6.2832f * ((float)k + 0.5f) / (float)N + 0.26f;
            Color cara = SombraPasarelas(oscura, LuzLateralPasarelas(a), false);
            VerticePasarelas(anillos[2][k], cara);
            VerticePasarelas(anillos[2][s], cara);
            VerticePasarelas(punta, cara);
        }

        rlEnd();

        // Cristales colgantes bajo la raiz.
        if (detalle)
        {
            Color cristal = NieblaPasarelas(COLOR_CRISTAL_PASARELAS, niebla);

            for (int k = 0; k < 3; k++)
            {
                Vector3 p = anillos[1][(k * 4 + semilla) % N];
                p.y += 0.05f;
                PrismaLocalPasarelas(p, 0.14f, 0.0f, -0.7f - 0.4f * Hash01Pasarelas(semilla, k + 60), 5, cristal);
            }
        }
    }
}


static void DibujarLosasPasarelas(const MinijuegoPasarelasVacio& m, float t, bool detalle)
{
    for (int i = 0; i < m.cantidadPlataformas; i++)
    {
        const BloquePrueba& bloque = m.plataformas[i];
        const EstadoPlataformaPasarelas& estado = m.estadosPlataformas[i];
        bool meta = i == m.cantidadPlataformas - 1;
        bool extremo = i == 0 || meta;
        int semilla = 50 + i * 7;

        Vector3 c = bloque.posicion;
        c.y -= DesplazamientoAparicionPasarelas(m, i);
        float niebla = NieblaEnPasarelas(c);

        if (estado.cayendo)
        {
            if (c.y < FONDO_CAIDA_PASARELAS)
            {
                continue;
            }

            // Se parte en dos mitades que se abren y giran mientras caen.
            float td = m.tiempoDesplome[i];
            Color apagada = ColorRunaPasarelas(m, i);

            for (int mitad = -1; mitad <= 1; mitad += 2)
            {
                rlPushMatrix();
                rlTranslatef(c.x + (float)mitad * (bloque.tamano.x * 0.25f + td * 0.7f), c.y, c.z + (float)mitad * td * 0.2f);
                rlRotatef((float)mitad * td * 38.0f, 0.0f, 0.0f, 1.0f);
                rlRotatef((float)mitad * td * 14.0f, 1.0f, 0.0f, 0.0f);
                DibujarLosaPasarelas(
                    { 0.0f, 0.0f, 0.0f },
                    { bloque.tamano.x * 0.5f - 0.04f, bloque.tamano.y, bloque.tamano.z },
                    niebla,
                    apagada,
                    1.0f,
                    semilla + mitad,
                    false,
                    1.1f,
                    false
                );
                rlPopMatrix();
            }

            continue;
        }

        // Tiembla cuando esta por romperse y alguien la pisa.
        float gastado = GastadoPasarelas(m, i);

        if (estado.ocupada && gastado > 0.45f)
        {
            float f = (gastado - 0.45f) * ObtenerFactorMovimientoCamaraMinijuegos();
            c.x += std::sin(t * 55.0f + (float)i) * 0.035f * f;
            c.z += std::cos(t * 47.0f + (float)i) * 0.025f * f;
        }

        DibujarLosaPasarelas(c, bloque.tamano, niebla, ColorRunaPasarelas(m, i), gastado, semilla, detalle, extremo ? 3.4f : 2.4f, meta);
    }
}


// Columna de bloques sobre una repisa de roca por debajo del nivel de
// la losa (fuera de la zona pisable).
static void PilarSantuarioPasarelas(Vector3 base, float alto, bool oro)
{
    CuboPasarelas({ base.x, base.y - 0.25f, base.z }, { 1.2f, 0.5f, 1.2f }, COLOR_ROCA_PASARELAS);
    IslaPasarelas({ base.x, base.y - 0.5f, base.z }, 0.75f, 1.6f, (int)(base.z * 10.0f) + (base.x > 0.0f ? 1 : 0), COLOR_ROCA_PASARELAS, false);

    for (int k = 0; k < 4; k++)
    {
        float h = alto / 4.0f;
        CuboPasarelas({ base.x, base.y + h * ((float)k + 0.5f), base.z }, { 0.68f - 0.03f * (float)k, h * 0.94f, 0.68f - 0.03f * (float)k }, k % 2 == 0 ? COLOR_PIEDRA_PASARELAS : COLOR_PIEDRA_CLARA_PASARELAS);
    }

    if (oro)
    {
        CuboPasarelas({ base.x, base.y + alto * 0.5f, base.z }, { 0.74f, 0.12f, 0.74f }, COLOR_ORO_PASARELAS);
        CuboPasarelas({ base.x, base.y + alto - 0.1f, base.z }, { 0.78f, 0.14f, 0.78f }, COLOR_ORO_PASARELAS);
    }
}


static const float Z_ARCO_META_PASARELAS = Z_META_PASARELAS;
static const float X_PILAR_SANTUARIO_PASARELAS = 4.85f;
static const float ALTO_PILAR_META_PASARELAS = 3.2f;
static const float ALTO_PILAR_SALIDA_PASARELAS = 2.4f;
static const float Z_PILAR_SALIDA_PASARELAS = 6.4f;


// Salida: dos pilares con braseros. Meta: arco dorado sobre la linea.
// MODELO FUTURO: arco de meta runico GLB.
static void DibujarSantuariosPasarelas(const MinijuegoPasarelasVacio& m)
{
    const BloquePrueba& salida = m.plataformas[0];
    const BloquePrueba& meta = m.plataformas[m.cantidadPlataformas - 1];
    float ySalida = salida.posicion.y + salida.tamano.y * 0.5f - 0.5f;
    float yMeta = meta.posicion.y + meta.tamano.y * 0.5f - 0.5f - DesplazamientoAparicionPasarelas(m, m.cantidadPlataformas - 1);
    float yTopMeta = yMeta + 0.5f;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 pilar = { (float)lado * X_PILAR_SANTUARIO_PASARELAS, ySalida, Z_PILAR_SALIDA_PASARELAS };
        PilarSantuarioPasarelas(pilar, ALTO_PILAR_SALIDA_PASARELAS, false);
        BraseroPasarelas({ pilar.x, pilar.y + ALTO_PILAR_SALIDA_PASARELAS - 1.1f, pilar.z }, NieblaEnPasarelas(pilar));

        PilarSantuarioPasarelas({ (float)lado * X_PILAR_SANTUARIO_PASARELAS, yMeta, Z_ARCO_META_PASARELAS }, ALTO_PILAR_META_PASARELAS + 0.5f, true);
    }

    // Arco rebajado de dovelas con clave dorada.
    float rx = X_PILAR_SANTUARIO_PASARELAS;
    float ry = 1.7f;
    float yArranque = yMeta + ALTO_PILAR_META_PASARELAS + 0.5f;
    float niebla = NieblaEnPasarelas({ 0.0f, yArranque, Z_ARCO_META_PASARELAS });
    const int DOVELAS = 11;

    for (int k = 0; k < DOVELAS; k++)
    {
        float th = 3.1416f * ((float)k + 0.5f) / (float)DOVELAS;
        float x = std::cos(th) * rx;
        float y = yArranque + std::sin(th) * ry;
        float tangente = std::atan2(ry * std::cos(th), -rx * std::sin(th)) * RAD2DEG;
        Color color = k == DOVELAS / 2 ? COLOR_ORO_PASARELAS : (k % 2 == 0 ? COLOR_PIEDRA_PASARELAS : COLOR_PIEDRA_CLARA_PASARELAS);

        rlPushMatrix();
        rlTranslatef(x, y, Z_ARCO_META_PASARELAS);
        rlRotatef(tangente, 0.0f, 0.0f, 1.0f);
        CuboLocalPasarelas({ 0.0f, 0.0f, 0.0f }, { 1.05f, 0.62f, 0.72f }, NieblaPasarelas(color, niebla));
        rlPopMatrix();
    }

    // Linea de meta dorada sobre la losa.
    float x0 = meta.posicion.x - meta.tamano.x * 0.5f + 0.02f;
    float x1 = meta.posicion.x + meta.tamano.x * 0.5f - 0.02f;
    Color oro = NieblaPasarelas(COLOR_ORO_PASARELAS, niebla);
    rlBegin(RL_QUADS);

    for (int k = 0; k < 14; k++)
    {
        float a = x0 + (x1 - x0) * (float)k / 14.0f;
        float b = x0 + (x1 - x0) * (float)(k + 1) / 14.0f;
        Color tramo = k % 2 == 0 ? oro : TonoPasarelas(oro, 0.78f);
        QuadPasarelas({ a, yTopMeta + 0.018f, Z_META_PASARELAS + 0.13f }, { b, yTopMeta + 0.018f, Z_META_PASARELAS + 0.13f }, { b, yTopMeta + 0.018f, Z_META_PASARELAS - 0.13f }, { a, yTopMeta + 0.018f, Z_META_PASARELAS - 0.13f }, tramo);
    }

    rlEnd();
}


//==================================================
// JUGADORES, PARTICULAS Y LUCES
//==================================================

// Marcas de salida (una por jugador) y anillo del color de cada uno bajo
// los pies mientras pisa una losa.
static void DibujarMarcasJugadoresPasarelas(
    const MinijuegoPasarelasVacio& m,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    float t
)
{
    const float posicionesX[MAX_PARTICIPANTES] = { -2.7f, -0.9f, 0.9f, 2.7f };
    const BloquePrueba& salida = m.plataformas[0];
    float ySalida = salida.posicion.y + salida.tamano.y * 0.5f + 0.02f;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Color color = participantes[i].color;
        AnilloPasarelas({ posicionesX[i], ySalida, Z_INICIO_PASARELAS }, 0.44f, 0.58f, ConAlfaPasarelas(color, 0.75f), 24);
        AnilloPasarelas({ posicionesX[i], ySalida, Z_INICIO_PASARELAS }, 0.0f, 0.44f, ConAlfaPasarelas(color, 0.18f), 24);

        const JugadorPrueba& j = jugadores[i];

        if (m.estadosJugadores[i].eliminado || j.cayendo || !j.enSuelo)
        {
            continue;
        }

        float pies = j.posicion.y - j.tamano.y * 0.5f + 0.022f;
        float pulso = 1.0f + 0.05f * std::sin(t * 5.0f + (float)i);
        AnilloPasarelas({ j.posicion.x, pies, j.posicion.z }, 0.0f, 0.5f, ConAlfaPasarelas(color, 0.26f), 20);
        AnilloPasarelas({ j.posicion.x, pies + 0.002f, j.posicion.z }, 0.5f * pulso, 0.62f * pulso, ConAlfaPasarelas(color, 0.9f), 24);
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Eliminados: siguen cayendo hacia la nebulosa dando vueltas.
static void DibujarCaidasPasarelas(const MinijuegoPasarelasVacio& m, const Participante participantes[])
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const CaidaJugadorPasarelas& caida = m.caidas[i];

        if (!m.resultado.participantes[i].participo || !caida.activa)
        {
            continue;
        }

        rlPushMatrix();
        rlTranslatef(caida.posicion.x, caida.posicion.y, caida.posicion.z);
        rlRotatef(caida.tiempo * 300.0f, 1.0f, 0.0f, 0.4f);
        DibujarModeloJugadorEnPosicion({ 0.0f, -0.7f, 0.0f }, caida.angulo + caida.tiempo * 90.0f, participantes[i].color);
        rlPopMatrix();
    }
}


static void DibujarParticulasPasarelas(const MinijuegoPasarelasVacio& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PASARELAS; i++)
    {
        const ParticulaPasarelasVacio& p = m.particulasEfecto[i];

        if (!p.activa || p.brillo)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        float tam = p.tamano * (0.45f + 0.55f * vida);
        CuboLocalPasarelas(p.posicion, { tam, tam, tam }, TintePasarelas(p.color, p.posicion));
    }
}


static void EmpezarAditivoPasarelas()
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
}


static void TerminarAditivoPasarelas()
{
    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Llama astral: halo grande, lenguas que titilan y nucleo blanco.
static void LlamaPasarelas(Vector3 base, float escala, float t, int semilla, Color color)
{
    float parpadeo = 0.85f + 0.15f * std::sin(t * 13.0f + (float)semilla) * std::sin(t * 7.3f + (float)semilla * 2.0f);
    ResplandorFrentePasarelas({ base.x, base.y + 0.35f * escala, base.z }, 1.7f * escala * parpadeo, ConAlfaPasarelas(color, 0.22f));

    for (int k = 0; k < 3; k++)
    {
        float y = base.y + (0.12f + 0.24f * (float)k) * escala + std::sin(t * 9.0f + (float)k + (float)semilla) * 0.03f;
        float r = (0.44f - 0.11f * (float)k) * escala * parpadeo;
        Vector3 p = { base.x + std::sin(t * 6.0f + (float)(k + semilla)) * 0.05f, y, base.z };
        ResplandorFrentePasarelas(p, r, ConAlfaPasarelas(MezclaPasarelas(color, WHITE, 0.22f * (float)k), 0.6f));
    }

    ResplandorFrentePasarelas({ base.x, base.y + 0.16f * escala, base.z }, 0.17f * escala, ConAlfaPasarelas(WHITE, 0.75f));
}


// Charco de luz en un suelo (discos concentricos muy tenues).
static void CharcoLuzPasarelas(Vector3 c, float radio, Color color, float intensidad)
{
    ResplandorSueloPasarelas(c, radio, ConAlfaPasarelas(color, 0.05f * intensidad));
    ResplandorSueloPasarelas(c, radio * 0.55f, ConAlfaPasarelas(color, 0.07f * intensidad));
}


// Cascada de polvo estelar que cae del borde de una isla.
static void CascadaPolvoPasarelas(Vector3 origen, float ancho, float largo, float t, Color color)
{
    const int TRAMOS = 12;
    Vector3 u = Vector3Scale(DERECHA_VISTA_PASARELAS, ancho * 0.5f);

    rlBegin(RL_QUADS);

    for (int k = 0; k < TRAMOS; k++)
    {
        float a = (float)k / (float)TRAMOS;
        float b = (float)(k + 1) / (float)TRAMOS;
        float ya = origen.y - a * largo;
        float yb = origen.y - b * largo;
        float ensancha = 1.0f + b * 0.8f;
        float bandaA = 0.55f + 0.45f * std::sin(a * 9.0f - t * 3.0f);
        float bandaB = 0.55f + 0.45f * std::sin(b * 9.0f - t * 3.0f);
        Color ca = ConAlfaPasarelas(color, 0.22f * (1.0f - a) * bandaA);
        Color cb = ConAlfaPasarelas(color, 0.22f * (1.0f - b) * bandaB);

        VerticePasarelas({ origen.x - u.x * (1.0f + a * 0.8f), ya, origen.z - u.z * (1.0f + a * 0.8f) }, ca);
        VerticePasarelas({ origen.x + u.x * (1.0f + a * 0.8f), ya, origen.z + u.z * (1.0f + a * 0.8f) }, ca);
        VerticePasarelas({ origen.x + u.x * ensancha, yb, origen.z + u.z * ensancha }, cb);
        VerticePasarelas({ origen.x - u.x * ensancha, yb, origen.z - u.z * ensancha }, cb);
    }

    rlEnd();
}


static void DibujarLucesPasarelas(
    const MinijuegoPasarelasVacio& m,
    const JugadorPrueba jugadores[],
    int limite,
    bool decoracion
)
{
    float t = m.tiempoAnimacion;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    EmpezarAditivoPasarelas();

    // Runas de cada losa: charco, canales encendidos y destello.
    for (int i = 0; i < m.cantidadPlataformas; i++)
    {
        const BloquePrueba& bloque = m.plataformas[i];
        const EstadoPlataformaPasarelas& estado = m.estadosPlataformas[i];

        if (estado.cayendo)
        {
            continue;
        }

        bool meta = i == m.cantidadPlataformas - 1;
        float yTop = bloque.posicion.y + bloque.tamano.y * 0.5f - DesplazamientoAparicionPasarelas(m, i);
        Vector3 centro = CentroRunaPasarelas({ bloque.posicion.x, yTop + 0.03f, bloque.posicion.z }, meta);
        float radio = RadioRunaPasarelas(bloque.tamano);
        Color runa = ColorRunaPasarelas(m, i);
        float destello = m.destelloPlataforma[i] > 0.0f ? m.destelloPlataforma[i] : 0.0f;
        float pulso = 0.8f + 0.2f * std::sin(t * 1.8f + (float)i * 0.9f);
        float extra = estado.ocupada ? 0.25f : 0.0f;

        CharcoLuzPasarelas(centro, radio * 2.8f, runa, 1.4f + destello * 3.0f + extra * 2.0f);
        AnilloPasarelas(centro, radio - 0.13f, radio + 0.05f, ConAlfaPasarelas(runa, (0.32f + extra) * pulso + 0.5f * destello), 36);
        AnilloPasarelas(centro, radio * 0.55f - 0.1f, radio * 0.55f + 0.03f, ConAlfaPasarelas(runa, (0.22f + extra) * pulso + 0.4f * destello), 28);

        if (destello > 0.0f)
        {
            float r = radio * (1.0f + (1.0f - destello) * 1.4f);
            AnilloPasarelas(centro, r - 0.12f, r, ConAlfaPasarelas(runa, 0.6f * destello), 40);
        }
    }

    // Brillo dorado de la meta: charco, linea, portal y columna de luz.
    {
        const BloquePrueba& meta = m.plataformas[m.cantidadPlataformas - 1];
        float baja = DesplazamientoAparicionPasarelas(m, m.cantidadPlataformas - 1);
        float yTop = meta.posicion.y + meta.tamano.y * 0.5f - baja;
        Vector3 centro = CentroRunaPasarelas({ meta.posicion.x, yTop + 0.03f, meta.posicion.z }, true);
        float pulso = 0.85f + 0.15f * std::sin(t * 2.2f);

        CharcoLuzPasarelas(centro, 5.0f, COLOR_ORO_PASARELAS, 1.6f * pulso);

        // Linea de meta encendida: degrade que se apaga hacia los lados.
        float x0 = meta.posicion.x - meta.tamano.x * 0.5f;
        float x1 = meta.posicion.x + meta.tamano.x * 0.5f;
        Color linea = ConAlfaPasarelas(COLOR_ORO_PASARELAS, 0.35f * pulso);
        Color borde = ConAlfaPasarelas(COLOR_ORO_PASARELAS, 0.0f);
        float y = yTop + 0.03f;
        rlBegin(RL_QUADS);
        VerticePasarelas({ x0, y, Z_META_PASARELAS + 0.5f }, borde);
        VerticePasarelas({ x1, y, Z_META_PASARELAS + 0.5f }, borde);
        VerticePasarelas({ x1, y, Z_META_PASARELAS }, linea);
        VerticePasarelas({ x0, y, Z_META_PASARELAS }, linea);
        VerticePasarelas({ x0, y, Z_META_PASARELAS }, linea);
        VerticePasarelas({ x1, y, Z_META_PASARELAS }, linea);
        VerticePasarelas({ x1, y, Z_META_PASARELAS - 0.5f }, borde);
        VerticePasarelas({ x0, y, Z_META_PASARELAS - 0.5f }, borde);
        rlEnd();

        float yArranque = yTop + ALTO_PILAR_META_PASARELAS;
        ResplandorPasarelas({ 0.0f, yTop + 2.2f, Z_ARCO_META_PASARELAS }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 4.6f, ConAlfaPasarelas(COLOR_ORO_PASARELAS, 0.08f * pulso));

        for (int k = 0; k < 11; k++)
        {
            float th = 3.1416f * ((float)k + 0.5f) / 11.0f;
            Vector3 p = { std::cos(th) * X_PILAR_SANTUARIO_PASARELAS, yArranque + std::sin(th) * 1.7f, Z_ARCO_META_PASARELAS + 0.4f };
            ResplandorFrentePasarelas(p, 0.8f, ConAlfaPasarelas(COLOR_ORO_PASARELAS, 0.12f + 0.06f * std::sin(t * 3.0f + (float)k)));
        }

        // Chispas que suben dentro del portal.
        for (int k = 0; k < 22; k++)
        {
            float u = std::fmod(t * (0.25f + 0.2f * Hash01Pasarelas(k, 140)) + Hash01Pasarelas(k, 141), 1.0f);
            Vector3 p = { -4.0f + 8.0f * Hash01Pasarelas(k, 142), yTop + 0.2f + u * 4.6f, Z_ARCO_META_PASARELAS + (Hash01Pasarelas(k, 143) - 0.5f) * 0.6f };
            ResplandorFrentePasarelas(p, 0.12f, ConAlfaPasarelas(COLOR_ORO_PASARELAS, 0.8f * std::sin(u * 3.1416f)));
        }

        Vector3 pie = { centro.x, yTop + 0.05f, centro.z };
        DrawCylinderEx(pie, { pie.x, pie.y + 28.0f, pie.z }, 1.1f, 2.4f, 16, Fade(COLOR_ORO_PASARELAS, 0.05f * pulso));
        DrawCylinderEx(pie, { pie.x, pie.y + 28.0f, pie.z }, 0.35f, 0.7f, 10, Fade(Color{ 255, 240, 200, 255 }, 0.07f * pulso));
    }

    // Braseros de la salida con su charco sobre la losa.
    {
        const BloquePrueba& salida = m.plataformas[0];
        float yTop = salida.posicion.y + salida.tamano.y * 0.5f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 llama = { (float)lado * X_PILAR_SANTUARIO_PASARELAS, yTop - 0.5f + ALTO_PILAR_SALIDA_PASARELAS + 0.35f, Z_PILAR_SALIDA_PASARELAS };
            LlamaPasarelas(llama, 1.0f, t, lado + 3, COLOR_LLAMA_PASARELAS);
            CharcoLuzPasarelas({ (float)lado * 3.4f, yTop + 0.03f, Z_PILAR_SALIDA_PASARELAS - 0.4f }, 3.4f, COLOR_LLAMA_PASARELAS, 1.2f);
        }
    }

    if (decoracion)
    {
        // Braseros y faroles de las isletas.
        for (int k = 0; k < CANTIDAD_ISLETAS_PASARELAS; k++)
        {
            const IslaDecoradaPasarelas& isla = ISLETAS_PASARELAS[k];

            if (paso > 1 && isla.tipo != 0 && k % 2 == 1)
            {
                continue;
            }

            Vector3 c = PosicionIsletaPasarelas(k, t);

            if (isla.tipo == 0)
            {
                LlamaPasarelas({ c.x, c.y + 1.45f, c.z }, 1.1f, t, k, COLOR_LLAMA_PASARELAS);
                CharcoLuzPasarelas({ c.x, c.y + 0.03f, c.z }, isla.radio * 1.6f, COLOR_LLAMA_PASARELAS, 1.6f);
            }
            else if (isla.tipo == 5)
            {
                Vector3 luz = PosicionFarolLuzPasarelas({ c.x - 0.3f, c.y, c.z }, ALTO_FAROL_PASARELAS);
                float titila = 0.85f + 0.15f * std::sin(t * 5.0f + (float)k * 1.7f);
                ResplandorFrentePasarelas(luz, 1.3f * titila, ConAlfaPasarelas(COLOR_RUNA_PASARELAS, 0.22f));
                ResplandorFrentePasarelas(luz, 0.4f, ConAlfaPasarelas(WHITE, 0.5f));
                DrawCylinderEx(luz, { luz.x, c.y + 0.05f, luz.z }, 0.12f, 0.9f, 10, Fade(COLOR_RUNA_PASARELAS, 0.05f));
                CharcoLuzPasarelas({ luz.x, c.y + 0.03f, luz.z }, 1.8f, COLOR_RUNA_PASARELAS, 1.5f);
            }
            else if (isla.tipo == 1)
            {
                ResplandorFrentePasarelas({ c.x, c.y + 0.8f, c.z }, 1.4f, ConAlfaPasarelas(COLOR_CRISTAL_PASARELAS, 0.12f));
            }
            else if (isla.tipo == 2)
            {
                ResplandorFrentePasarelas({ c.x, c.y + 2.6f, c.z }, 1.8f, ConAlfaPasarelas(COLOR_HOJA_PASARELAS, 0.1f));
            }
        }

        // Glifos de los monolitos.
        for (int k = 0; k < CANTIDAD_MONOLITOS_PASARELAS; k += paso)
        {
            Vector3 c = PosicionMonolitoPasarelas(k, t);
            float pulso = 0.6f + 0.4f * std::sin(t * 1.3f + (float)k * 1.1f);
            ResplandorFrentePasarelas(c, 1.3f, ConAlfaPasarelas(COLOR_RUNA_PASARELAS, 0.1f * pulso));
        }

        // Anillo runico: brillo de cada piedra y velo en el centro.
        float fi = GIRO_ANILLO_RUNICO_PASARELAS * DEG2RAD;
        ResplandorPasarelas(ANILLO_RUNICO_PASARELAS, { std::cos(fi), 0.0f, -std::sin(fi) }, { 0.0f, 1.0f, 0.0f }, RADIO_ANILLO_RUNICO_PASARELAS * 0.95f, ConAlfaPasarelas(COLOR_CRISTAL_PASARELAS, 0.08f));

        for (int k = 0; k < PIEDRAS_ANILLO_RUNICO_PASARELAS; k++)
        {
            if (PiedraFaltaAnilloPasarelas(k)) continue;
            float pulso = 0.5f + 0.5f * std::sin(t * 2.0f - (float)k * 0.6f);
            ResplandorFrentePasarelas(PiedraAnilloRunicoPasarelas(k, t), 1.1f, ConAlfaPasarelas(COLOR_RUNA_PASARELAS, 0.06f + 0.1f * pulso));
        }

        // Cascadas de polvo estelar.
        for (int k = 0; k < CANTIDAD_ISLAS_MEDIAS_PASARELAS; k += paso)
        {
            if (k % 3 != 0) continue;

            Vector3 c = PosicionIslaMediaPasarelas(k, t);
            float r = ISLAS_MEDIAS_PASARELAS[k].radio * 0.85f;
            Vector3 borde = { c.x + 0.42f * r, c.y - 0.2f, c.z + 0.9f * r };
            CascadaPolvoPasarelas(borde, 1.4f, 24.0f, t, k % 2 == 0 ? COLOR_HOJA_PASARELAS : COLOR_RUNA_PASARELAS);
        }
    }

    // Polvo de estrellas que sube del vacio alrededor del foco.
    {
        int cantidad = (int)(70.0f * FactorCalidadGrafica(CalidadParticulas()));
        float foco = m.camara.target.z;

        for (int k = 0; k < cantidad; k++)
        {
            float u = std::fmod(t * (0.04f + 0.05f * Hash01Pasarelas(k, 151)) + Hash01Pasarelas(k, 152), 1.0f);
            Vector3 p = {
                -17.0f + 34.0f * Hash01Pasarelas(k, 153) + std::sin(t * 0.5f + (float)k) * 0.4f,
                -16.0f + u * 22.0f,
                foco - 26.0f + 40.0f * Hash01Pasarelas(k, 154)
            };

            // Fuera de las losas: no ensucia la lectura del camino.
            if (p.y > -1.0f && p.x > -5.0f && p.x < 5.0f)
            {
                continue;
            }

            Color c = k % 3 == 0 ? COLOR_RUNA_PASARELAS : (k % 3 == 1 ? COLOR_HOJA_PASARELAS : Color{ 255, 246, 236, 255 });
            ResplandorFrentePasarelas(p, 0.14f + 0.08f * Hash01Pasarelas(k, 155), ConAlfaPasarelas(c, 0.55f * std::sin(u * 3.1416f)));
        }
    }

    // Particulas con brillo (chispas de runa, chispas de la meta).
    for (int i = 0; i < MAX_PARTICULAS_PASARELAS; i++)
    {
        const ParticulaPasarelasVacio& p = m.particulasEfecto[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        ResplandorFrentePasarelas(p.posicion, p.tamano * 2.2f, ConAlfaPasarelas(p.color, 0.7f * vida));
    }

    // Destello dorado sobre quien llego a la meta.
    for (int i = 0; i < limite; i++)
    {
        if (m.destelloMeta[i] > 0.0f)
        {
            ResplandorFrentePasarelas(jugadores[i].posicion, 2.4f * (1.5f - m.destelloMeta[i] * 0.5f), ConAlfaPasarelas(COLOR_ORO_PASARELAS, 0.4f * m.destelloMeta[i]));
        }
    }

    TerminarAditivoPasarelas();
}


// Perspectiva equivalente a la camara ortografica de juego en el foco
// (misma altura de cuadro) para que el vacio tenga profundidad. Durante
// la cuenta la camara baja desde mas lejos y mas de costado.
static Camera3D CamaraVistaPasarelas(const MinijuegoPasarelasVacio& m)
{
    Vector3 direccion = Vector3Normalize(DIRECCION_VISTA_PASARELAS);
    float distancia = m.camara.fovy * 0.5f / std::tan(FOV_VISTA_PASARELAS * 0.5f * DEG2RAD);
    Vector3 mira = m.camara.target;

    if (m.fase == FASE_PASARELAS_PREPARACION)
    {
        float u = Limitar01Pasarelas(m.tiempoPreparacion / DURACION_PREPARACION_PASARELAS);
        float s = SuavePasarelas(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        float giro = 0.45f * s;
        float dx = direccion.x * std::cos(giro) - direccion.z * std::sin(giro);
        float dz = direccion.x * std::sin(giro) + direccion.z * std::cos(giro);
        direccion = Vector3Normalize({ dx, direccion.y * (1.0f - 0.3f * s), dz });
        distancia += 12.0f * s;
        mira.z -= 5.0f * s;
    }

    Camera3D vista{};
    vista.target = mira;
    vista.position = Vector3Add(mira, Vector3Scale(direccion, distancia));
    vista.up = { 0.0f, 1.0f, 0.0f };
    vista.fovy = FOV_VISTA_PASARELAS;
    vista.projection = CAMERA_PERSPECTIVE;
    return vista;
}


void MinijuegoPasarelasVacio::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;

    Camera3D vista = CamaraVistaPasarelas(*this);
    ojoNieblaPasarelas = vista.position;

    ClearBackground(COLOR_NIEBLA_PASARELAS);
    DibujarCieloPasarelas(*this, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte: nubes del fondo del vacio e islas hundidas.
    if (decoracion)
    {
        if (efectos)
        {
            EmpezarAditivoPasarelas();
            DibujarNubesVacioPasarelas(t, -84.0f, 16 / paso, 0.16f, 500);
            TerminarAditivoPasarelas();
        }

        DibujarIslasHundidasPasarelas(t, paso);

        if (efectos)
        {
            EmpezarAditivoPasarelas();
            DibujarNubesVacioPasarelas(t, -44.0f, 10 / paso, 0.08f, 600);
            TerminarAditivoPasarelas();
        }

        // Media distancia y anillo cercano.
        DibujarIslasMediasPasarelas(t, paso);
        DibujarAnilloRunicoPasarelas(t);
        DibujarIsletasPasarelas(t, paso);
        DibujarMonolitosPasarelas(t, paso);
    }

    // Arena: anclas, losas y santuarios de salida y meta.
    DibujarAnclasPasarelas(*this, t, decoracion ? paso : 2);
    DibujarLosasPasarelas(*this, t, decoracion);
    DibujarSantuariosPasarelas(*this);

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadPlataformas; i++)
        {
            if (plataformas[i].activaColision)
            {
                DrawBoundingBox(CrearHitboxBloquePrueba(plataformas[i]), YELLOW);
            }
        }
    }

    // Jugadores.
    DibujarMarcasJugadoresPasarelas(*this, jugadores, limite, participantes, t);

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || estadosJugadores[i].eliminado)
        {
            continue;
        }

        // Un mando desconectado queda temporalmente a cargo de la IA.
        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorModeloCompartido(jugadores[i], participanteVisual);

        if (mostrarDebug && !jugadores[i].cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarCaidasPasarelas(*this, participantes);
    DibujarParticulasTierra(particulas, cantidadParticulas);
    DibujarParticulasPasarelas(*this);

    if (efectos)
    {
        DibujarLucesPasarelas(*this, jugadores, limite, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre los que siguen en carrera.
    for (int i = 0; i < limite; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado ||
            !participantes[i].activo
        )
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

    // «¡J# AL VACÍO!» y «¡J# LLEGÓ!» donde ocurrio.
    for (int i = 0; i < MAX_AVISOS_PASARELAS; i++)
    {
        const AvisoPasarelasVacio& aviso = avisos[i];
        if (!aviso.activo) continue;

        float u = 1.0f - aviso.vida / DURACION_AVISO_PASARELAS;
        Vector3 p = { aviso.posicion.x, aviso.posicion.y + 1.6f + u * 1.2f, aviso.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);

        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.y > Px(600.0f)) s.y = Px(600.0f);
        if (s.x < Px(160.0f)) s.x = Px(160.0f);
        if (s.x > (float)ancho - Px(160.0f)) s.x = (float)ancho - Px(160.0f);

        float alfa = aviso.vida < 0.45f ? aviso.vida / 0.45f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        char texto[32];
        std::snprintf(
            texto,
            sizeof(texto),
            aviso.meta ? "¡J%d LLEGÓ!" : "¡J%d AL VACÍO!",
            participantes[aviso.jugador].numeroJugador
        );
        DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, aviso.meta ? COLOR_ORO_PASARELAS : participantes[aviso.jugador].color, alfa);
    }

    // Largada.
    if (tiempoAnuncio >= 0.0f && fase == FASE_PASARELAS_CARRERA)
    {
        float u = tiempoAnuncio / DURACION_ANUNCIO_PASARELAS;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
        float y = (float)alto * 0.3f - u * Px(18.0f);
        DibujarTextoMundoContornoUI("¡A CORRER!", (float)ancho * 0.5f, y, UI_TEXTO_TITULO, Color{ 150, 240, 255, 255 }, alfa);
        DibujarTextoMundoContornoUI("NO TE QUEDES QUIETO: LAS LOSAS SE ROMPEN", (float)ancho * 0.5f, y + Px(50.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "PASARELAS DEL VACÍO",
        "CORRÉ HASTA EL ARCO DORADO: LAS LOSAS SE GASTAN MIENTRAS LAS PISÁS Y CAEN",
        resultado
    );

    hud.fase = fase == FASE_PASARELAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_PASARELAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = fase == FASE_PASARELAS_PREPARACION ? DURACION_CARRERA_PASARELAS : tiempoRestante;
    hud.tiempoTotal = DURACION_CARRERA_PASARELAS;

    const float recorrido = Z_INICIO_PASARELAS - Z_META_PASARELAS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        if (!h.visible) continue;

        const EstadoJugadorPasarelasVacio& e = estadosJugadores[i];
        int porcentaje = (int)std::lround(e.progresoMaximo / recorrido * 100.0f);
        if (porcentaje > 100) porcentaje = 100;

        if (e.llegoMeta)
        {
            std::snprintf(h.valor, sizeof(h.valor), "META");
            std::snprintf(h.estado, sizeof(h.estado), "¡EN LA META!");
            std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "LLEGÓ A LA META");
        }
        else
        {
            std::snprintf(h.valor, sizeof(h.valor), "%d %%", porcentaje);

            if (e.eliminado)
            {
                std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "CAYÓ AL %d %%", porcentaje);
            }
            else
            {
                std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d %% DEL CAMINO", porcentaje);
            }
        }

        h.valorNumerico = e.llegoMeta ? 101 : porcentaje;
        h.progreso = e.llegoMeta ? 1.0f : (float)porcentaje / 100.0f;
        h.eliminado = e.eliminado;

        // Aviso en la etiqueta cuando la losa que pisa esta por romperse.
        if (i < limite && fase == FASE_PASARELAS_CARRERA && !e.eliminado && !e.llegoMeta)
        {
            int soporte = BuscarPlataformaSoportePasarelas(*this, jugadores[i]);

            if (soporte >= 0 && GastadoPasarelas(*this, soporte) > 0.5f)
            {
                std::snprintf(h.estado, sizeof(h.estado), "¡SE ROMPE!");
            }
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "MOVÉ: %s\nSALTÁ: %s",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    if (fase == FASE_PASARELAS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        bool alguienLlego = false;
        bool alguienEnPie = false;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo) continue;
            if (estadosJugadores[i].llegoMeta) alguienLlego = true;
            if (!estadosJugadores[i].eliminado) alguienEnPie = true;
        }

        if (!alguienEnPie)
        {
            hud.tituloFinal = "¡TODOS AL VACÍO!";
            hud.notaFinal = "NADIE LLEGÓ: COMPARTEN PUESTO";
        }
        else if (!alguienLlego)
        {
            hud.finPorTiempo = true;
            hud.notaFinal = "NADIE LLEGÓ: GANA QUIEN MÁS AVANZÓ";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoPasarelasVacio::ObtenerResultado() const
{
    return resultado;
}
