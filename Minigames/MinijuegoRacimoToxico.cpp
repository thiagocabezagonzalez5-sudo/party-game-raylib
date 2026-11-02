#include "Minigames/MinijuegoRacimoToxico.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// RACIMO TOXICO
//==================================================
//
// Por turnos, cada jugador toma 1 o 2 frutos del racimo. Las bayas
// toxicas (posiciones fijas, visibles) quitan una vida. El fruto dorado
// hace que se salte el turno del siguiente jugador.
//==================================================


static const float DURACION_PREPARACION_RACIMO = 3.0f;
static const float DURACION_TURNO_RACIMO = 5.0f;
static const float DURACION_PAUSA_RACIMO = 1.3f;
static const float DURACION_PARTIDA_RACIMO = 75.0f;
static const float DURACION_VUELO_RACIMO = 0.9f;
static const float Z_BALSAS_RACIMO = 3.5f;
static const float SEPARACION_BALSAS_RACIMO = 5.0f;
static const float PI_RACIMO = 3.14159265f;
static const int POSICIONES_TOXICAS_RACIMO[3] = { 5, 11, 17 };


// Del paquete GLB del pantano solo se cargan las piezas jugables: los
// tres frutos y la balsa (tramo seguido del enum) y el aro del turno. El
// decorado del pantano se construye con primitivas propias de la escena.
static_assert(MODELO_RACIMO_FRUTO_NORMAL + 3 == MODELO_RACIMO_BALSA,
    "Frutos y balsa deben ir seguidos en el paquete de Racimo");

static void CargarPiezasJugablesRacimo()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteRacimoToxicoRetro3D();

    PaqueteModelosEscenarioRetro3D frutos = paquete;
    frutos.recursos += MODELO_RACIMO_FRUTO_NORMAL;
    frutos.definiciones += MODELO_RACIMO_FRUTO_NORMAL;
    frutos.cantidad = MODELO_RACIMO_BALSA - MODELO_RACIMO_FRUTO_NORMAL + 1;
    CargarPaqueteModelosEscenarioRetro3D(frutos);

    PaqueteModelosEscenarioRetro3D aro = paquete;
    aro.recursos += MODELO_RACIMO_ARO;
    aro.definiciones += MODELO_RACIMO_ARO;
    aro.cantidad = 1;
    CargarPaqueteModelosEscenarioRetro3D(aro);
}


//==================================================
// LOGICA
//==================================================


static bool EsControlBotRacimo(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static int ContarVivosRacimo(const MinijuegoRacimoToxico& minijuego)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static int SiguienteVivoRacimo(const MinijuegoRacimoToxico& minijuego, int desde)
{
    for (int paso = 1; paso <= MAX_PARTICIPANTES; paso++)
    {
        int indice = (desde + paso) % MAX_PARTICIPANTES;

        if (
            minijuego.resultado.participantes[indice].participo &&
            minijuego.estadosJugadores[indice].vivo
        )
        {
            return indice;
        }
    }

    return desde;
}


static void ReponerRacimo(MinijuegoRacimoToxico& minijuego)
{
    for (int i = 0; i < TOTAL_FRUTAS_RACIMO; i++)
    {
        minijuego.tipoFruta[i] = FRUTA_RACIMO_NORMAL;
    }

    for (int posicion : POSICIONES_TOXICAS_RACIMO)
    {
        minijuego.tipoFruta[posicion] = FRUTA_RACIMO_TOXICA;
    }

    // Un fruto dorado aparece a veces (nunca sobre una baya).
    if (GetRandomValue(0, 99) < 65)
    {
        for (int intento = 0; intento < 32; intento++)
        {
            int indice = GetRandomValue(1, TOTAL_FRUTAS_RACIMO - 2);

            if (minijuego.tipoFruta[indice] == FRUTA_RACIMO_NORMAL)
            {
                minijuego.tipoFruta[indice] = FRUTA_RACIMO_DORADA;
                break;
            }
        }
    }

    minijuego.frente = 0;
}


static void IniciarTurnoRacimo(MinijuegoRacimoToxico& minijuego)
{
    minijuego.fase = FASE_RACIMO_TURNO;
    minijuego.tiempoTurno = DURACION_TURNO_RACIMO;
    minijuego.seleccion = 1;
    minijuego.tiempoBot = GetRandomValue(90, 180) / 100.0f;
    minijuego.accionPrevia = true;
}


static void FinalizarRacimo(MinijuegoRacimoToxico& minijuego)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int ganadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorRacimoToxico& estado = minijuego.estadosJugadores[i];

        if (estado.vivo)
        {
            int posicion = 1;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    minijuego.resultado.participantes[j].participo &&
                    minijuego.estadosJugadores[j].vivo &&
                    minijuego.estadosJugadores[j].vidas > estado.vidas
                )
                {
                    posicion++;
                }
            }

            resultadoJugador.posicionFinal = posicion;
        }
        else
        {
            resultadoJugador.posicionFinal = estado.posicionEliminacion;
        }

        resultadoJugador.puntuacionMinijuego =
            estado.vidas * 100 + estado.frutasSeguras;

        if (resultadoJugador.posicionFinal == 1)
        {
            ganadores++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        ganadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_RACIMO_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static Vector3 PosicionFrutaRacimo(int indice, float tiempo)
{
    return Vector3{
        (indice % 2 == 0 ? -0.36f : 0.36f) + std::sin(tiempo * 1.5f + (float)indice) * 0.03f,
        1.5f + (float)indice * 0.42f,
        0.0f
    };
}


static void TomarFrutasRacimo(MinijuegoRacimoToxico& minijuego, int cantidad)
{
    int jugador = minijuego.turno;
    EstadoJugadorRacimoToxico& estado = minijuego.estadosJugadores[jugador];

    bool toxica = false;
    bool dorada = false;
    int tomadas = 0;

    for (int i = 0; i < MAX_VUELOS_RACIMO; i++)
    {
        minijuego.vuelos[i].activo = false;
    }

    for (int k = 0; k < cantidad && minijuego.frente < TOTAL_FRUTAS_RACIMO; k++)
    {
        int tipo = minijuego.tipoFruta[minijuego.frente];

        VueloFrutaRacimo& vuelo = minijuego.vuelos[k];
        vuelo.activo = true;
        vuelo.tipo = tipo;
        vuelo.jugador = jugador;
        vuelo.progreso = 0.0f;
        vuelo.origen = PosicionFrutaRacimo(minijuego.frente, minijuego.tiempoAnimacion);

        if (tipo == FRUTA_RACIMO_TOXICA)
            toxica = true;
        else if (tipo == FRUTA_RACIMO_DORADA)
            dorada = true;
        else
            estado.frutasSeguras++;

        minijuego.frente++;
        tomadas++;
    }

    minijuego.ultimoJugador = jugador;
    minijuego.ultimoEvento = EVENTO_RACIMO_SEGURO;
    minijuego.jugadorSaltado = -1;

    if (tomadas > 0)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RECOGER_OBJETO);
    }

    if (toxica)
    {
        int vivosAntes = ContarVivosRacimo(minijuego);
        estado.vidas--;
        estado.sacudida = 0.8f;

        if (estado.vidas <= 0)
        {
            estado.vidas = 0;
            estado.vivo = false;
            estado.posicionEliminacion = vivosAntes;
            minijuego.ultimoEvento = EVENTO_RACIMO_ELIMINADO;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
        }
        else
        {
            minijuego.ultimoEvento = EVENTO_RACIMO_TOXICA;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        }
    }
    else if (dorada)
    {
        minijuego.ultimoEvento = EVENTO_RACIMO_DORADA;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
    }

    if (dorada && ContarVivosRacimo(minijuego) > 1)
    {
        minijuego.saltarSiguiente = true;
    }

    minijuego.fase = FASE_RACIMO_ANIMACION;
    minijuego.tiempoPausa = DURACION_PAUSA_RACIMO;
}


static int ElegirTomaBotRacimo(const MinijuegoRacimoToxico& minijuego)
{
    int restante = TOTAL_FRUTAS_RACIMO - minijuego.frente;

    if (restante <= 1)
    {
        return 1;
    }

    // Error aleatorio para que no sea perfecto.
    if (GetRandomValue(0, 99) < 25)
    {
        return GetRandomValue(1, 2);
    }

    int f = minijuego.frente;
    bool seguro1 = minijuego.tipoFruta[f] != FRUTA_RACIMO_TOXICA;
    bool seguro2 = seguro1 && minijuego.tipoFruta[f + 1] != FRUTA_RACIMO_TOXICA;

    if (!seguro1)
    {
        return 1;
    }

    // Dejar al siguiente frente a la baya.
    if (minijuego.tipoFruta[f + 1] == FRUTA_RACIMO_TOXICA)
    {
        return 1;
    }

    if (seguro2 && f + 2 < TOTAL_FRUTAS_RACIMO && minijuego.tipoFruta[f + 2] == FRUTA_RACIMO_TOXICA)
    {
        return 2;
    }

    if (minijuego.tipoFruta[f] == FRUTA_RACIMO_DORADA)
    {
        return 1;
    }

    if (seguro2 && minijuego.tipoFruta[f + 1] == FRUTA_RACIMO_DORADA)
    {
        return 2;
    }

    return seguro2 ? GetRandomValue(1, 2) : 1;
}


static void ActualizarTurnoRacimo(
    MinijuegoRacimoToxico& minijuego,
    float deltaTime,
    Participante participantes[]
)
{
    int restante = TOTAL_FRUTAS_RACIMO - minijuego.frente;
    int maximo = restante < 2 ? restante : 2;
    if (maximo < 1) maximo = 1;

    float antes = minijuego.tiempoTurno;
    minijuego.tiempoTurno -= deltaTime;
    ActualizarAudioAlertaTiempo(minijuego.audio, antes, minijuego.tiempoTurno, 2.0f);

    const Participante& participante = participantes[minijuego.turno];

    if (EsControlBotRacimo(participante))
    {
        minijuego.tiempoBot -= deltaTime;

        if (minijuego.tiempoBot <= 0.0f)
        {
            TomarFrutasRacimo(minijuego, ElegirTomaBotRacimo(minijuego));
            return;
        }
    }
    else
    {
        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participante);

        if (entrada.izquierda)
        {
            minijuego.seleccion = 1;
        }
        else if (entrada.derecha)
        {
            minijuego.seleccion = 2;
        }

        if (minijuego.seleccion > maximo)
        {
            minijuego.seleccion = maximo;
        }

        bool confirmar = entrada.golpear && !minijuego.accionPrevia;
        minijuego.accionPrevia = entrada.golpear;

        if (confirmar)
        {
            TomarFrutasRacimo(minijuego, minijuego.seleccion);
            return;
        }
    }

    if (minijuego.tiempoTurno <= 0.0f)
    {
        TomarFrutasRacimo(minijuego, 1);
    }
}


static void ActualizarAnimacionRacimo(MinijuegoRacimoToxico& minijuego, float deltaTime)
{
    minijuego.tiempoPausa -= deltaTime;

    if (minijuego.tiempoPausa > 0.0f)
    {
        return;
    }

    if (ContarVivosRacimo(minijuego) <= 1)
    {
        FinalizarRacimo(minijuego);
        return;
    }

    minijuego.racimoRepuesto = false;

    if (minijuego.frente >= TOTAL_FRUTAS_RACIMO)
    {
        ReponerRacimo(minijuego);
        minijuego.racimoRepuesto = true;
    }

    int siguiente = SiguienteVivoRacimo(minijuego, minijuego.turno);

    if (minijuego.saltarSiguiente)
    {
        minijuego.saltarSiguiente = false;
        minijuego.jugadorSaltado = siguiente;
        siguiente = SiguienteVivoRacimo(minijuego, siguiente);
    }

    minijuego.turno = siguiente;
    IniciarTurnoRacimo(minijuego);
}


static void ActualizarEfectosRacimo(MinijuegoRacimoToxico& minijuego, float deltaTime)
{
    for (int i = 0; i < MAX_VUELOS_RACIMO; i++)
    {
        VueloFrutaRacimo& vuelo = minijuego.vuelos[i];

        if (!vuelo.activo)
        {
            continue;
        }

        vuelo.progreso += deltaTime / DURACION_VUELO_RACIMO;

        if (vuelo.progreso >= 1.0f)
        {
            vuelo.activo = false;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float& sacudida = minijuego.estadosJugadores[i].sacudida;

        if (sacudida > 0.0f)
        {
            sacudida -= deltaTime;
            if (sacudida < 0.0f) sacudida = 0.0f;
        }
    }
}


//==================================================
// INTERFAZ
//==================================================


void MinijuegoRacimoToxico::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        posicionBalsaX[i] = 0.0f;
    }

    for (int i = 0; i < MAX_VUELOS_RACIMO; i++)
    {
        vuelos[i] = {};
    }

    camara.position = { 0.0f, 9.5f, 20.0f };
    camara.target = { 0.0f, 5.2f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_RACIMO_PREPARACION;
    turno = -1;
    seleccion = 1;
    ultimoJugador = -1;
    jugadorSaltado = -1;
    ultimoEvento = EVENTO_RACIMO_NINGUNO;
    saltarSiguiente = false;
    accionPrevia = false;
    racimoRepuesto = false;
    tiempoTurno = DURACION_TURNO_RACIMO;
    tiempoPausa = 0.0f;
    tiempoRestante = DURACION_PARTIDA_RACIMO;
    tiempoPreparacion = DURACION_PREPARACION_RACIMO;
    tiempoAnimacion = 0.0f;
    tiempoBot = 0.0f;

    for (int i = 0; i < MAX_PARTICULAS_RACIMO; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        destelloJugador[i] = 0.0f;
        caidaJugador[i] = 0.0f;
    }

    tomaVisual = 0;
    tiempoRebroteVisual = -1.0f;
    tiempoFinVisual = 0.0f;

    ReponerRacimo(*this);
}


void MinijuegoRacimoToxico::Reiniciar(
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

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_RACIMO_TERMINADO;
        return;
    }

    CargarPiezasJugablesRacimo();

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
    int ranura = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        estadosJugadores[i].vivo = true;
        estadosJugadores[i].vidas = VIDAS_RACIMO;

        float x = ((float)ranura - ((float)cantidad - 1.0f) * 0.5f) * SEPARACION_BALSAS_RACIMO;
        posicionBalsaX[i] = x;
        ranura++;

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            Vector3{ x, 0.9f, Z_BALSAS_RACIMO }
        );
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
    }

    turno = indices[GetRandomValue(0, cantidad - 1)];
}


static void ActualizarPartidaRacimo(
    MinijuegoRacimoToxico& minijuego,
    float deltaTime,
    Participante participantes[]
)
{
    minijuego.tiempoAnimacion += deltaTime;

    if (
        minijuego.fase == FASE_RACIMO_TERMINADO ||
        minijuego.resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    ActualizarEfectosRacimo(minijuego, deltaTime);

    if (minijuego.fase == FASE_RACIMO_PREPARACION)
    {
        float preparacionAntes = minijuego.tiempoPreparacion;
        minijuego.tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(minijuego.audio, preparacionAntes, minijuego.tiempoPreparacion);

        if (minijuego.tiempoPreparacion <= 0.0f)
        {
            minijuego.tiempoPreparacion = 0.0f;
            IniciarTurnoRacimo(minijuego);
        }

        return;
    }

    minijuego.tiempoRestante -= deltaTime;

    if (minijuego.tiempoRestante <= 0.0f)
    {
        minijuego.tiempoRestante = 0.0f;
        FinalizarRacimo(minijuego);
        return;
    }

    if (minijuego.fase == FASE_RACIMO_TURNO)
    {
        ActualizarTurnoRacimo(minijuego, deltaTime, participantes);
    }
    else if (minijuego.fase == FASE_RACIMO_ANIMACION)
    {
        ActualizarAnimacionRacimo(minijuego, deltaTime);
    }
}


// Lo que el visual compara antes y despues de cada paso de la logica.
struct InstantaneaRacimo
{
    FaseRacimoToxico fase = FASE_RACIMO_PREPARACION;
    int frente = 0;
    bool vivo[MAX_PARTICIPANTES] = {};
    bool vueloActivo[MAX_VUELOS_RACIMO] = {};
    int vueloTipo[MAX_VUELOS_RACIMO] = {};
    int vueloJugador[MAX_VUELOS_RACIMO] = {};
    float vueloProgreso[MAX_VUELOS_RACIMO] = {};
};

// Definidas con el resto del visual, al final del archivo.
static void TomarInstantaneaRacimo(const MinijuegoRacimoToxico& m, InstantaneaRacimo& s);
static void ActualizarVisualRacimo(MinijuegoRacimoToxico& m, const InstantaneaRacimo& s, float deltaTime);


void MinijuegoRacimoToxico::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    (void)cantidadMaxima;

    InstantaneaRacimo antes;
    TomarInstantaneaRacimo(*this, antes);

    ActualizarPartidaRacimo(*this, deltaTime, participantes);

    ActualizarVisualRacimo(*this, antes, deltaTime);
}

//==================================================
// VISUAL: PANTANO DE NOCHE
//==================================================
//
// Capas, de adelante hacia atras: el agua con las balsas, nenufares y
// juncos; un anillo de utileria (estacas con farol, troncos, rocas con
// musgo, hongos que brillan, canoa, ranas); el arbol gigante de cuya rama
// cuelga el racimo y las cabanas sobre pilotes; dos lineas de cipreses
// que se pierden en la niebla verde; y el cielo con luna y estrellas.
// Todo se arma con primitivas agrupadas por rlgl: los unicos GLB son las
// piezas jugables (frutos, balsas y aro del turno). El azar visual sale
// de semillaVisual o de un hash por indice; nunca de GetRandomValue.
//==================================================


static const Color COLOR_NIEBLA_RACIMO = { 50, 84, 70, 255 };
static const Color COLOR_AGUA_RACIMO = { 20, 46, 44, 255 };
static const Color COLOR_AGUA_CLARA_RACIMO = { 58, 98, 86, 255 };
static const Color COLOR_CORTEZA_RACIMO = { 92, 72, 56, 255 };
static const Color COLOR_CORTEZA_OSCURA_RACIMO = { 60, 48, 40, 255 };
static const Color COLOR_MUSGO_RACIMO = { 88, 126, 66, 255 };
static const Color COLOR_HOJA_RACIMO = { 42, 88, 56, 255 };
static const Color COLOR_HOJA_CLARA_RACIMO = { 90, 146, 70, 255 };
static const Color COLOR_BARBA_RACIMO = { 132, 150, 118, 255 };
static const Color COLOR_TALLO_RACIMO = { 78, 104, 52, 255 };
static const Color COLOR_MADERA_RACIMO = { 132, 96, 62, 255 };
static const Color COLOR_MADERA_OSCURA_RACIMO = { 88, 64, 46, 255 };
static const Color COLOR_PAJA_RACIMO = { 140, 124, 76, 255 };
static const Color COLOR_PIEDRA_RACIMO = { 96, 104, 98, 255 };
static const Color COLOR_HIERRO_RACIMO = { 58, 60, 64, 255 };
static const Color COLOR_LUZ_RACIMO = { 255, 196, 110, 255 };
static const Color COLOR_TOXICO_RACIMO = { 212, 76, 240, 255 };
static const Color COLOR_DORADO_RACIMO = { 255, 212, 72, 255 };
static const Color COLOR_HONGO_RACIMO = { 112, 255, 200, 255 };
static const Color COLOR_LUCIERNAGA_RACIMO = { 216, 255, 120, 255 };
static const Color COLOR_FRUTO_RACIMO = { 176, 220, 86, 255 };

// Ojo de la camara de juego (para la niebla) y ejes de la pantalla en el
// mundo (para los resplandores que miran a la camara).
static const Vector3 OJO_RACIMO = { 0.0f, 9.5f, 20.0f };
static const Vector3 FRENTE_U_RACIMO = { 1.0f, 0.0f, 0.0f };
static const Vector3 FRENTE_V_RACIMO = { 0.0f, 0.9777f, -0.2102f };

static const float Y_RAMA_RACIMO = 12.0f;
static const float ESCALA_FRUTO_RACIMO = 1.45f;
static const float DURACION_CAIDA_RACIMO = 1.6f;
static const float DURACION_REBROTE_RACIMO = 1.6f;
static const float DURACION_FIN_VISUAL_RACIMO = 3.2f;

// Luna (fraccion de la pantalla) y pendiente de su reflejo en el agua:
// x del reflejo = PENDIENTE * profundidad desde la camara.
static const float LUNA_X_RACIMO = 0.92f;
static const float LUNA_Y_RACIMO = 0.115f;
static const float PENDIENTE_LUNA_RACIMO = 0.696f;


//--------------------------------------------------
// Utilidades de color y geometria
//--------------------------------------------------

static float Limitar01Racimo(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static unsigned char CanalRacimo(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoRacimo(Color color, float factor)
{
    return Color{
        CanalRacimo((float)color.r * factor),
        CanalRacimo((float)color.g * factor),
        CanalRacimo((float)color.b * factor),
        color.a
    };
}


static Color MezclaRacimo(Color a, Color b, float t)
{
    t = Limitar01Racimo(t);

    return Color{
        CanalRacimo((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalRacimo((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalRacimo((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalRacimo((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaRacimo(Color color, float alfa)
{
    color.a = CanalRacimo(255.0f * alfa);
    return color;
}


static float Ruido01Racimo(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float SuaveRacimo(float u)
{
    u = Limitar01Racimo(u);
    return u * u * (3.0f - 2.0f * u);
}


// Niebla verde: lo lejano se funde con el aire del pantano, y lo bajo y
// lejano un poco antes (la niebla se asienta sobre el agua).
static float CantidadNieblaRacimo(Vector3 p)
{
    float dx = p.x - OJO_RACIMO.x;
    float dy = p.y - OJO_RACIMO.y;
    float dz = p.z - OJO_RACIMO.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 22.0f) / 56.0f;

    if (d > 26.0f && p.y < 5.0f)
    {
        n += (5.0f - p.y) / 5.0f * 0.14f;
    }

    return n < 0.0f ? 0.0f : (n > 0.9f ? 0.9f : n);
}


static Color TinteRacimo(Color color, Vector3 p, float emisivo = 0.0f)
{
    Color niebla = COLOR_NIEBLA_RACIMO;
    niebla.a = color.a;
    return MezclaRacimo(color, niebla, CantidadNieblaRacimo(p) * (1.0f - 0.65f * emisivo));
}


static Vector3 SumaRacimo(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaRacimo(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaRacimo(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }


static Vector3 NormalizarRacimo(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 0.0001f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static Vector3 CruzRacimo(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}


static void VerticeRacimo(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadRacimo(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeRacimo(a, color);
    VerticeRacimo(b, color);
    VerticeRacimo(c, color);
    VerticeRacimo(d, color);
}


static void QuadNieblaRacimo(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeRacimo(a, TinteRacimo(color, a));
    VerticeRacimo(b, TinteRacimo(color, b));
    VerticeRacimo(c, TinteRacimo(color, c));
    VerticeRacimo(d, TinteRacimo(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.55), sin niebla. Respeta la matriz de rlgl.
static void CuboCrudoRacimo(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadRacimo({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, color);
    QuadRacimo({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoRacimo(color, 0.78f));
    QuadRacimo({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoRacimo(color, 0.55f));
    QuadRacimo({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoRacimo(color, 0.62f));
    QuadRacimo({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoRacimo(color, 0.70f));
    QuadRacimo({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoRacimo(color, 0.45f));
    rlEnd();
}


// La misma caja con la niebla del lugar (tinte segun su centro).
static void CuboRacimo(Vector3 c, Vector3 t, Color color)
{
    CuboCrudoRacimo(c, t, TinteRacimo(color, c));
}


// Caja girada alrededor de Y (grados); la niebla se toma en el mundo.
static void CuboGiradoRacimo(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    Color tenido = TinteRacimo(color, c);
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboCrudoRacimo({ 0.0f, 0.0f, 0.0f }, t, tenido);
    rlPopMatrix();
}


// Luz de la escena: la luna, de arriba, algo de frente y de la derecha.
static float LuzNormalRacimo(Vector3 n)
{
    const float LX = 0.32f, LY = 0.80f, LZ = 0.50f;
    float d = n.x * LX + n.y * LY + n.z * LZ;
    return 0.54f + 0.48f * Limitar01Racimo(d);
}


// Prisma (o tronco de cono) entre dos puntos, con niebla por vertice.
// emisivo acerca las caras a la luz plena (vidrios, frutos, hongos).
static void PrismaRacimo(
    Vector3 a,
    Vector3 b,
    float r0,
    float r1,
    int lados,
    Color color,
    float emisivo = 0.0f,
    float giro = 0.0f
)
{
    Vector3 eje = NormalizarRacimo(RestaRacimo(b, a));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarRacimo(CruzRacimo(eje, apoyo));
    Vector3 v = CruzRacimo(eje, u);
    float paso = 6.2832f / (float)lados;
    float luzTapa = LuzNormalRacimo(eje);
    Color tapa = TonoRacimo(color, luzTapa + (1.0f - luzTapa) * emisivo);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaRacimo(EscalaRacimo(u, std::cos(a0)), EscalaRacimo(v, std::sin(a0)));
        Vector3 d1 = SumaRacimo(EscalaRacimo(u, std::cos(a1)), EscalaRacimo(v, std::sin(a1)));
        Vector3 dm = SumaRacimo(EscalaRacimo(u, std::cos(am)), EscalaRacimo(v, std::sin(am)));

        float f = LuzNormalRacimo(dm);
        f = f + (1.0f - f) * emisivo;
        if (k & 1) f *= 0.93f;
        Color cara = TonoRacimo(color, f);

        Vector3 p0 = SumaRacimo(a, EscalaRacimo(d0, r0));
        Vector3 p1 = SumaRacimo(a, EscalaRacimo(d1, r0));
        Vector3 p2 = SumaRacimo(b, EscalaRacimo(d1, r1));
        Vector3 p3 = SumaRacimo(b, EscalaRacimo(d0, r1));

        QuadNieblaRacimo(p0, p1, p2, p3, cara);

        if (r1 > 0.001f)
        {
            Color tapaNiebla = TinteRacimo(tapa, b, emisivo);
            QuadRacimo(b, p2, p3, b, tapaNiebla);
        }
    }

    rlEnd();
}


// Roca o mata facetada: esfera de pocos lados con radios irregulares.
static void RocaRacimo(Vector3 c, Vector3 r, int semilla, Color color, float irregular = 0.14f, float emisivo = 0.0f)
{
    const int ANILLOS = 5;
    const int LADOS = 8;

    rlBegin(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        for (int k = 0; k < LADOS; k++)
        {
            int aa[4] = { a, a, a + 1, a + 1 };
            int kk[4] = { k, k + 1, k + 1, k };

            for (int q = 0; q < 4; q++)
            {
                float th = -1.5708f + 3.1416f * (float)aa[q] / (float)ANILLOS;
                float ph = 6.2832f * (float)kk[q] / (float)LADOS + (float)semilla;
                float f = 1.0f;

                if (aa[q] != 0 && aa[q] != ANILLOS)
                {
                    f = 1.0f - irregular + 2.0f * irregular * Ruido01Racimo(aa[q] * 31 + (kk[q] % LADOS), semilla);
                }

                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = { c.x + n.x * r.x * f, c.y + n.y * r.y * f, c.z + n.z * r.z * f };
                float luz = LuzNormalRacimo(n);
                luz = luz + (1.0f - luz) * emisivo;
                Color cv = TonoRacimo(color, luz * (0.93f + 0.12f * Ruido01Racimo(aa[q] * 7 + kk[q], semilla + 5)));
                VerticeRacimo(p, TinteRacimo(cv, p, emisivo));
            }
        }
    }

    rlEnd();
}


// Disco o sector horizontal relleno (nenufares, charcos, tapas).
static void SectorRacimo(Vector3 c, float radio, float a0, float a1, Color color, int lados = 18)
{
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < lados; k++)
    {
        float b0 = a0 + (a1 - a0) * (float)k / (float)lados;
        float b1 = a0 + (a1 - a0) * (float)(k + 1) / (float)lados;
        VerticeRacimo(c, color);
        VerticeRacimo({ c.x + std::cos(b1) * radio, c.y, c.z + std::sin(b1) * radio }, color);
        VerticeRacimo({ c.x + std::cos(b0) * radio, c.y, c.z + std::sin(b0) * radio }, color);
    }

    rlEnd();
}


// Corona horizontal (ondas en el agua, bordes de nenufar).
static void AnilloRacimo(Vector3 c, float r0, float r1, Color color, int lados = 24)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadRacimo(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Hoja: rombo entre la base y la punta, ancho segun el vector lado.
static void HojaRacimo(Vector3 base, Vector3 punta, Vector3 lado, Color color)
{
    Vector3 medio = SumaRacimo(base, EscalaRacimo(RestaRacimo(punta, base), 0.42f));
    Vector3 izquierda = SumaRacimo(medio, lado);
    Vector3 derecha = RestaRacimo(medio, lado);

    rlBegin(RL_QUADS);
    QuadNieblaRacimo(base, izquierda, punta, derecha, color);
    rlEnd();
}


// Tira vertical que se afina (musgo colgante, juncos, lianas).
static void TiraRacimo(Vector3 arriba, Vector3 abajo, float anchoArriba, float anchoAbajo, Color colorArriba, Color colorAbajo)
{
    rlBegin(RL_QUADS);
    VerticeRacimo({ arriba.x - anchoArriba, arriba.y, arriba.z }, TinteRacimo(colorArriba, arriba));
    VerticeRacimo({ abajo.x - anchoAbajo, abajo.y, abajo.z }, TinteRacimo(colorAbajo, abajo));
    VerticeRacimo({ abajo.x + anchoAbajo, abajo.y, abajo.z }, TinteRacimo(colorAbajo, abajo));
    VerticeRacimo({ arriba.x + anchoArriba, arriba.y, arriba.z }, TinteRacimo(colorArriba, arriba));
    VerticeRacimo({ arriba.x, arriba.y, arriba.z - anchoArriba }, TinteRacimo(colorArriba, arriba));
    VerticeRacimo({ abajo.x, abajo.y, abajo.z - anchoAbajo }, TinteRacimo(colorAbajo, abajo));
    VerticeRacimo({ abajo.x, abajo.y, abajo.z + anchoAbajo }, TinteRacimo(colorAbajo, abajo));
    VerticeRacimo({ arriba.x, arriba.y, arriba.z + anchoArriba }, TinteRacimo(colorArriba, arriba));
    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorRacimo(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloRacimo(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorRacimo(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Reflejo sobre el agua: estirado hacia la camara (eje z).
static void ReflejoAguaRacimo(Vector3 c, float radio, Color color, float estirado)
{
    ResplandorRacimo(c, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, radio, color, estirado);
}


static void ResplandorFrenteRacimo(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorRacimo(c, FRENTE_U_RACIMO, FRENTE_V_RACIMO, radio, color, estirado);
}


// Cono de luz (aditivo): del foco hacia el charco, se apaga en la base.
static void ConoLuzRacimo(Vector3 foco, Vector3 baseCentro, float radio, Color color)
{
    const int LADOS = 16;
    Vector3 eje = NormalizarRacimo(RestaRacimo(baseCentro, foco));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarRacimo(CruzRacimo(eje, apoyo));
    Vector3 v = CruzRacimo(eje, u);

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaRacimo(baseCentro, SumaRacimo(EscalaRacimo(u, std::cos(a0) * radio), EscalaRacimo(v, std::sin(a0) * radio)));
        Vector3 p1 = SumaRacimo(baseCentro, SumaRacimo(EscalaRacimo(u, std::cos(a1) * radio), EscalaRacimo(v, std::sin(a1) * radio)));

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(foco.x, foco.y, foco.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p1.x, p1.y, p1.z);
    }
    rlEnd();
}


// Charco de luz sobre el agua: discos concentricos muy tenues.
static void CharcoLuzRacimo(Vector3 c, float radio, Color color, float intensidad)
{
    for (int r = 0; r < 3; r++)
    {
        float f = 0.55f + 0.45f * (float)r;
        ResplandorSueloRacimo({ c.x, 0.03f + 0.002f * (float)r, c.z }, radio * f, AlfaRacimo(color, (0.05f + 0.03f * (float)(2 - r)) * intensidad));
    }
}


//--------------------------------------------------
// Balsas: altura, temblor y faroles de vida
//--------------------------------------------------

// Bamboleo y hundimiento originales de la balsa (la eliminada se hunde).
static float AlturaBalsaRacimo(const MinijuegoRacimoToxico& m, int i, float t)
{
    float hundimiento = m.estadosJugadores[i].vivo ? 0.0f : -0.55f;
    return 0.08f + hundimiento + std::sin(t * 2.0f + m.posicionBalsaX[i]) * 0.04f;
}


static float TemblorBalsaRacimo(const MinijuegoRacimoToxico& m, int i, float t)
{
    if (m.estadosJugadores[i].sacudida <= 0.0f)
    {
        return 0.0f;
    }

    return std::sin(t * 50.0f) * 0.12f * ObtenerFactorMovimientoCamaraMinijuegos();
}


// Un farol por vida en las esquinas delanteras de la balsa.
static Vector3 LinternaVidaRacimo(const MinijuegoRacimoToxico& m, int i, int vida, float t)
{
    float lado = vida == 0 ? -1.0f : 1.0f;
    return {
        m.posicionBalsaX[i] + TemblorBalsaRacimo(m, i, t) + lado * 1.3f,
        AlturaBalsaRacimo(m, i, t) + 1.05f,
        Z_BALSAS_RACIMO + 0.72f
    };
}


// Las vidas que se ven: el farol se apaga cuando la baya llega a las
// manos, no cuando se arranca del racimo.
static int VidasVisiblesRacimo(const MinijuegoRacimoToxico& m, int i)
{
    int vidas = m.estadosJugadores[i].vidas;

    for (int k = 0; k < MAX_VUELOS_RACIMO; k++)
    {
        if (m.vuelos[k].activo && m.vuelos[k].jugador == i && m.vuelos[k].tipo == FRUTA_RACIMO_TOXICA)
        {
            vidas++;
        }
    }

    return vidas > VIDAS_RACIMO ? VIDAS_RACIMO : vidas;
}


//--------------------------------------------------
// Estado visual: instantanea, particulas y efectos
//--------------------------------------------------

static void TomarInstantaneaRacimo(const MinijuegoRacimoToxico& m, InstantaneaRacimo& s)
{
    s.fase = m.fase;
    s.frente = m.frente;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        s.vivo[i] = m.estadosJugadores[i].vivo;
    }

    for (int k = 0; k < MAX_VUELOS_RACIMO; k++)
    {
        s.vueloActivo[k] = m.vuelos[k].activo;
        s.vueloTipo[k] = m.vuelos[k].tipo;
        s.vueloJugador[k] = m.vuelos[k].jugador;
        s.vueloProgreso[k] = m.vuelos[k].progreso;
    }
}


static float AzarRacimo(MinijuegoRacimoToxico& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasRacimo(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


static void EmitirParticulaRacimo(
    MinijuegoRacimoToxico& m,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    int elegida = 0;
    float menorVida = 1e9f;

    for (int i = 0; i < MAX_PARTICULAS_RACIMO; i++)
    {
        if (!m.particulas[i].activa)
        {
            elegida = i;
            menorVida = -1.0f;
            break;
        }

        if (m.particulas[i].vida < menorVida)
        {
            menorVida = m.particulas[i].vida;
            elegida = i;
        }
    }

    ParticulaRacimo& p = m.particulas[elegida];
    p.activa = true;
    p.tipo = tipo;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.vida = vida;
    p.vidaMaxima = vida;
    p.tamano = tamano;
    p.giro = AzarRacimo(m, 0.0f, 360.0f);
    p.color = color;
}


static Vector3 DestinoVueloRacimo(const MinijuegoRacimoToxico& m, int jugador)
{
    return { m.posicionBalsaX[jugador], 2.0f, Z_BALSAS_RACIMO };
}


// El fruto llega a las manos de quien lo tomo.
static void EfectoLlegadaRacimo(MinijuegoRacimoToxico& m, int jugador, int tipo, bool eliminado)
{
    if (jugador < 0 || jugador >= MAX_PARTICIPANTES)
    {
        return;
    }

    Vector3 p = DestinoVueloRacimo(m, jugador);
    m.destelloJugador[jugador] = 1.0f;

    if (tipo == FRUTA_RACIMO_TOXICA)
    {
        // Nube de veneno violeta y gotas que salpican.
        int humo = CantidadParticulasRacimo(16);
        for (int k = 0; k < humo; k++)
        {
            float a = AzarRacimo(m, 0.0f, 6.2832f);
            float r = AzarRacimo(m, 0.4f, 1.4f);
            Color c = k % 3 == 0 ? Color{ 150, 230, 90, 255 } : Color{ 170, 70, 210, 255 };
            EmitirParticulaRacimo(m, PARTICULA_RACIMO_HUMO, p,
                { std::cos(a) * r, AzarRacimo(m, 0.6f, 1.8f), std::sin(a) * r * 0.6f },
                AzarRacimo(m, 0.9f, 1.5f), AzarRacimo(m, 0.28f, 0.5f), c);
        }

        int gotas = CantidadParticulasRacimo(10);
        for (int k = 0; k < gotas; k++)
        {
            float a = AzarRacimo(m, 0.0f, 6.2832f);
            EmitirParticulaRacimo(m, PARTICULA_RACIMO_GOTA, p,
                { std::cos(a) * 2.2f, AzarRacimo(m, 2.0f, 4.0f), std::sin(a) * 1.4f },
                1.2f, AzarRacimo(m, 0.06f, 0.1f), COLOR_TOXICO_RACIMO);
        }

        // Se apaga el farol de la vida perdida: humo gris que sube.
        int apagado = m.estadosJugadores[jugador].vidas;
        if (!eliminado && apagado >= 0 && apagado < VIDAS_RACIMO)
        {
            Vector3 farol = LinternaVidaRacimo(m, jugador, apagado, m.tiempoAnimacion);
            int bocanadas = CantidadParticulasRacimo(6);
            for (int k = 0; k < bocanadas; k++)
            {
                EmitirParticulaRacimo(m, PARTICULA_RACIMO_HUMO, { farol.x, farol.y + 0.2f, farol.z },
                    { AzarRacimo(m, -0.3f, 0.3f), AzarRacimo(m, 0.6f, 1.2f), AzarRacimo(m, -0.2f, 0.2f) },
                    AzarRacimo(m, 0.8f, 1.3f), AzarRacimo(m, 0.18f, 0.3f), Color{ 150, 156, 150, 255 });
            }
        }
        return;
    }

    // Fruto sano o dorado: chispas que saltan.
    bool dorado = tipo == FRUTA_RACIMO_DORADA;
    int chispas = CantidadParticulasRacimo(dorado ? 22 : 10);
    for (int k = 0; k < chispas; k++)
    {
        float a = AzarRacimo(m, 0.0f, 6.2832f);
        float r = AzarRacimo(m, 1.0f, dorado ? 3.2f : 2.2f);
        Color c = dorado
            ? (k % 3 == 0 ? Color{ 255, 250, 210, 255 } : COLOR_DORADO_RACIMO)
            : (k % 2 == 0 ? COLOR_FRUTO_RACIMO : Color{ 236, 255, 170, 255 });
        EmitirParticulaRacimo(m, PARTICULA_RACIMO_CHISPA, p,
            { std::cos(a) * r, AzarRacimo(m, 1.5f, 4.0f), std::sin(a) * r * 0.6f },
            AzarRacimo(m, 0.5f, 0.9f), AzarRacimo(m, 0.08f, dorado ? 0.18f : 0.12f), c);
    }
}


static void ActualizarParticulasRacimo(MinijuegoRacimoToxico& m, float dt)
{
    for (int i = 0; i < MAX_PARTICULAS_RACIMO; i++)
    {
        ParticulaRacimo& p = m.particulas[i];
        if (!p.activa) continue;

        p.vida -= dt;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        switch (p.tipo)
        {
        case PARTICULA_RACIMO_CHISPA:
            p.velocidad.y -= 6.0f * dt;
            p.velocidad = EscalaRacimo(p.velocidad, 1.0f - 1.2f * dt);
            break;
        case PARTICULA_RACIMO_HUMO:
            p.velocidad = EscalaRacimo(p.velocidad, 1.0f - 1.6f * dt);
            p.velocidad.y += 0.5f * dt;
            p.tamano += 0.55f * dt;
            break;
        case PARTICULA_RACIMO_GOTA:
            p.velocidad.y -= 13.0f * dt;
            break;
        case PARTICULA_RACIMO_CONFETI:
            p.velocidad.y -= 5.0f * dt;
            if (p.velocidad.y < -1.7f) p.velocidad.y = -1.7f;
            p.velocidad.x += std::sin(p.vida * 5.0f + p.giro) * 2.0f * dt;
            p.giro += 360.0f * dt;
            break;
        case PARTICULA_RACIMO_HOJA:
            p.velocidad.y -= 2.0f * dt;
            if (p.velocidad.y < -0.8f) p.velocidad.y = -0.8f;
            p.velocidad.x = std::sin(p.vida * 3.0f + p.giro) * 0.7f;
            p.giro += 120.0f * dt;
            break;
        default:
            break;
        }

        p.posicion = SumaRacimo(p.posicion, EscalaRacimo(p.velocidad, dt));

        if (p.tipo != PARTICULA_RACIMO_HUMO && p.tipo != PARTICULA_RACIMO_CHISPA && p.posicion.y < 0.0f)
        {
            p.activa = false;
        }
    }
}


static void ActualizarVisualRacimo(MinijuegoRacimoToxico& m, const InstantaneaRacimo& s, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (m.destelloJugador[i] > 0.0f)
        {
            m.destelloJugador[i] -= deltaTime * 2.2f;
            if (m.destelloJugador[i] < 0.0f) m.destelloJugador[i] = 0.0f;
        }

        if (m.caidaJugador[i] > 0.0f && m.caidaJugador[i] < DURACION_CAIDA_RACIMO)
        {
            m.caidaJugador[i] += deltaTime;
        }

        // Un reinicio devuelve al jugador a su balsa.
        if (m.estadosJugadores[i].vivo)
        {
            m.caidaJugador[i] = 0.0f;
        }
        else if (s.vivo[i])
        {
            // Eliminado: la balsa se hunde y el jugador cae al agua con un
            // salpicon y un temblor fuerte.
            m.caidaJugador[i] = 0.001f;
            Vector3 agua = { m.posicionBalsaX[i], 0.2f, Z_BALSAS_RACIMO };
            int salpicon = CantidadParticulasRacimo(20);
            for (int k = 0; k < salpicon; k++)
            {
                float a = AzarRacimo(m, 0.0f, 6.2832f);
                float r = AzarRacimo(m, 1.0f, 2.6f);
                EmitirParticulaRacimo(m, PARTICULA_RACIMO_GOTA, agua,
                    { std::cos(a) * r, AzarRacimo(m, 3.5f, 6.5f), std::sin(a) * r * 0.7f },
                    1.4f, AzarRacimo(m, 0.07f, 0.13f), Color{ 170, 214, 196, 255 });
            }
            ReproducirSonidoMinijuego(m.audio, SONIDO_CAIDA);
            ActivarTemblorCamaraGeneral(0.2f, 0.32f);
        }
    }

    // Frutos que llegan a destino (o vuelos reemplazados por una toma nueva).
    for (int k = 0; k < MAX_VUELOS_RACIMO; k++)
    {
        bool llego = s.vueloActivo[k] &&
            (!m.vuelos[k].activo || m.vuelos[k].progreso < s.vueloProgreso[k]);

        if (llego)
        {
            int j = s.vueloJugador[k];
            bool eliminado = j >= 0 && j < MAX_PARTICIPANTES && !m.estadosJugadores[j].vivo;
            EfectoLlegadaRacimo(m, j, s.vueloTipo[k], eliminado && s.vueloTipo[k] == FRUTA_RACIMO_TOXICA);
        }
    }

    // Toma nueva: cuantos frutos y hojas que caen del tallo.
    if (m.fase == FASE_RACIMO_ANIMACION && m.frente > s.frente)
    {
        m.tomaVisual = m.frente - s.frente;

        for (int k = s.frente; k < m.frente; k++)
        {
            Vector3 p = PosicionFrutaRacimo(k, m.tiempoAnimacion);
            EmitirParticulaRacimo(m, PARTICULA_RACIMO_HOJA, { p.x * 0.6f, p.y + 0.2f, 0.05f },
                { AzarRacimo(m, -0.4f, 0.4f), 0.4f, AzarRacimo(m, 0.1f, 0.6f) },
                3.0f, AzarRacimo(m, 0.16f, 0.24f), COLOR_HOJA_CLARA_RACIMO);
        }
    }

    // El racimo rebrota entero: chispas verdes a lo largo del tallo.
    if (s.frente > 0 && m.frente == 0 && m.racimoRepuesto)
    {
        m.tiempoRebroteVisual = 0.0f;
        int chispas = CantidadParticulasRacimo(24);
        for (int k = 0; k < chispas; k++)
        {
            float y = AzarRacimo(m, 1.4f, 10.6f);
            EmitirParticulaRacimo(m, PARTICULA_RACIMO_CHISPA, { AzarRacimo(m, -0.5f, 0.5f), y, AzarRacimo(m, -0.3f, 0.3f) },
                { AzarRacimo(m, -1.2f, 1.2f), AzarRacimo(m, 0.5f, 2.0f), AzarRacimo(m, -0.5f, 0.8f) },
                AzarRacimo(m, 0.6f, 1.1f), AzarRacimo(m, 0.08f, 0.14f), k % 2 ? COLOR_FRUTO_RACIMO : COLOR_LUCIERNAGA_RACIMO);
        }
    }

    if (m.tiempoRebroteVisual >= 0.0f)
    {
        m.tiempoRebroteVisual += deltaTime;
        if (m.tiempoRebroteVisual > DURACION_REBROTE_RACIMO) m.tiempoRebroteVisual = -1.0f;
    }

    // Fin: confeti sobre los ganadores.
    if (m.fase == FASE_RACIMO_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (s.fase != FASE_RACIMO_TERMINADO)
        {
            m.tiempoFinVisual = 0.0f;
        }

        if (m.tiempoFinVisual < DURACION_FIN_VISUAL_RACIMO)
        {
            m.tiempoFinVisual += deltaTime;
            const Color colores[5] =
            {
                { 255, 96, 96, 255 }, { 96, 164, 255, 255 }, { 116, 222, 124, 255 },
                { 255, 216, 84, 255 }, { 232, 124, 255, 255 }
            };
            float ritmo = 40.0f * FactorCalidadGrafica(CalidadParticulas());

            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1)
                {
                    continue;
                }

                float cantidad = ritmo * deltaTime;
                while (cantidad > 0.0f)
                {
                    if (cantidad < 1.0f && AzarRacimo(m, 0.0f, 1.0f) > cantidad) break;
                    cantidad -= 1.0f;
                    Vector3 p = { m.posicionBalsaX[i] + AzarRacimo(m, -1.6f, 1.6f), AzarRacimo(m, 4.5f, 6.0f), Z_BALSAS_RACIMO + AzarRacimo(m, -0.8f, 0.8f) };
                    int c = (int)AzarRacimo(m, 0.0f, 4.99f);
                    EmitirParticulaRacimo(m, PARTICULA_RACIMO_CONFETI, p,
                        { AzarRacimo(m, -1.0f, 1.0f), AzarRacimo(m, 0.5f, 2.0f), AzarRacimo(m, -0.4f, 0.4f) },
                        AzarRacimo(m, 2.0f, 3.0f), AzarRacimo(m, 0.1f, 0.16f), colores[c]);
                }
            }
        }
    }

    ActualizarParticulasRacimo(m, deltaTime);
}


//--------------------------------------------------
// Cielo (2D): degrade nocturno, estrellas, luna y nubes finas
//--------------------------------------------------

static void DibujarCieloRacimo(float t, bool efectos)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int a1 = (int)((float)alto * 0.18f);
    int a2 = (int)((float)alto * 0.36f);

    DrawRectangleGradientV(0, 0, ancho, a1, Color{ 6, 10, 24, 255 }, Color{ 16, 30, 44, 255 });
    DrawRectangleGradientV(0, a1, ancho, a2 - a1, Color{ 16, 30, 44, 255 }, Color{ 50, 84, 72, 255 });
    DrawRectangle(0, a2, ancho, alto - a2, Color{ 46, 78, 66, 255 });

    // Estrellas que titilan (no sobre la luna).
    Vector2 luna = { (float)ancho * LUNA_X_RACIMO, (float)alto * LUNA_Y_RACIMO };
    float radioLuna = (float)alto * 0.042f;

    for (int k = 0; k < 90; k++)
    {
        float x = Ruido01Racimo(k, 701) * (float)ancho;
        float y = Ruido01Racimo(k, 702) * (float)alto * 0.3f;
        float dx = x - luna.x, dy = y - luna.y;
        if (dx * dx + dy * dy < radioLuna * radioLuna * 9.0f) continue;

        float titila = efectos ? 0.5f + 0.5f * std::sin(t * (0.8f + 2.2f * Ruido01Racimo(k, 703)) + (float)k) : 0.8f;
        float alfa = (0.25f + 0.65f * titila) * (1.0f - y / ((float)alto * 0.36f));
        int lado = Ruido01Racimo(k, 704) > 0.86f ? 2 : 1;
        DrawRectangle((int)x, (int)y, lado, lado, AlfaRacimo(Color{ 226, 240, 220, 255 }, alfa));
    }

    // Luna con halo verdoso y mares.
    DrawCircleV(luna, radioLuna * 4.2f, AlfaRacimo(Color{ 150, 210, 170, 255 }, 0.05f));
    DrawCircleV(luna, radioLuna * 2.6f, AlfaRacimo(Color{ 170, 220, 180, 255 }, 0.08f));
    DrawCircleV(luna, radioLuna * 1.5f, AlfaRacimo(Color{ 210, 236, 200, 255 }, 0.14f));
    DrawCircleV(luna, radioLuna, Color{ 236, 240, 206, 255 });
    DrawCircleV({ luna.x - radioLuna * 0.3f, luna.y - radioLuna * 0.2f }, radioLuna * 0.26f, Color{ 212, 218, 186, 255 });
    DrawCircleV({ luna.x + radioLuna * 0.32f, luna.y + radioLuna * 0.22f }, radioLuna * 0.18f, Color{ 214, 220, 188, 255 });
    DrawCircleV({ luna.x + radioLuna * 0.05f, luna.y + radioLuna * 0.48f }, radioLuna * 0.12f, Color{ 218, 222, 192, 255 });

    // Nubes finas que cruzan despacio.
    for (int k = 0; k < 6; k++)
    {
        float velocidad = efectos ? 6.0f + 5.0f * Ruido01Racimo(k, 711) : 0.0f;
        float base = Ruido01Racimo(k, 712) * ((float)ancho + 500.0f);
        float x = std::fmod(base + t * velocidad, (float)ancho + 500.0f) - 250.0f;
        float y = (float)alto * (0.06f + 0.2f * Ruido01Racimo(k, 713));
        float rx = (float)ancho * (0.09f + 0.07f * Ruido01Racimo(k, 714));
        float ry = (float)alto * 0.012f;
        Color c = AlfaRacimo(Color{ 96, 130, 120, 255 }, 0.22f);
        DrawEllipse((int)x, (int)y, rx, ry, c);
        DrawEllipse((int)(x + rx * 0.4f), (int)(y - ry * 0.8f), rx * 0.55f, ry * 0.9f, c);
    }

    // Bruma verde sobre el horizonte.
    DrawRectangleGradientV(0, (int)((float)alto * 0.24f), ancho, (int)((float)alto * 0.12f),
        AlfaRacimo(Color{ 90, 140, 110, 255 }, 0.0f), AlfaRacimo(Color{ 90, 140, 110, 255 }, 0.3f));
}


//--------------------------------------------------
// Horizonte: dos lineas de cipreses y lomas en la niebla
//--------------------------------------------------

// Una linea de arboles: muro bajo continuo y copas de silueta irregular.
static void LineaArbolesRacimo(float z, float x0, float x1, float paso, float altoMin, float altoMax, int semilla, Color color, float claroX0, float claroX1)
{
    rlBegin(RL_QUADS);

    int n = (int)((x1 - x0) / paso);
    for (int k = 0; k < n; k++)
    {
        float xa = x0 + paso * (float)k;
        float xb = xa + paso;
        float xm = (xa + xb) * 0.5f;
        float claro = (xm > claroX0 && xm < claroX1) ? 0.42f : 1.0f;
        float h = (altoMin + (altoMax - altoMin) * Ruido01Racimo(k, semilla)) * claro;
        float hb = h * (0.42f + 0.12f * Ruido01Racimo(k, semilla + 1));
        float zk = z + (Ruido01Racimo(k, semilla + 2) - 0.5f) * 3.0f;
        float tono = 0.85f + 0.25f * Ruido01Racimo(k, semilla + 3);
        Color c = TonoRacimo(color, tono);

        // Muro de follaje bajo, unido al vecino.
        QuadNieblaRacimo({ xa - paso * 0.3f, -0.5f, zk }, { xb + paso * 0.3f, -0.5f, zk }, { xb + paso * 0.3f, hb, zk }, { xa - paso * 0.3f, hb, zk }, TonoRacimo(c, 0.8f));

        // Copa: rombo alto y un poco torcido (cipres).
        float w = paso * (0.7f + 0.5f * Ruido01Racimo(k, semilla + 4));
        float inclina = (Ruido01Racimo(k, semilla + 5) - 0.5f) * paso * 0.8f;
        QuadNieblaRacimo({ xm - w, hb * 0.9f, zk }, { xm + w * 0.8f, hb, zk }, { xm + inclina * 0.5f + w * 0.35f, h * 0.8f, zk }, { xm + inclina, h, zk }, c);

        // Tronco seco que asoma de vez en cuando.
        if (Ruido01Racimo(k, semilla + 6) > 0.86f)
        {
            float ht = h * 1.15f;
            QuadNieblaRacimo({ xm - 0.12f, hb, zk + 0.1f }, { xm + 0.12f, hb, zk + 0.1f }, { xm + 0.05f + inclina * 0.3f, ht, zk + 0.1f }, { xm - 0.05f + inclina * 0.3f, ht, zk + 0.1f }, TonoRacimo(COLOR_CORTEZA_OSCURA_RACIMO, 0.8f));
        }
    }

    rlEnd();
}


static void DibujarHorizonteRacimo(bool decoracion)
{
    if (decoracion)
    {
        // Lomas lejanas que asoman por el claro de la luna.
        rlBegin(RL_QUADS);
        for (int k = 0; k < 24; k++)
        {
            float xa = -120.0f + 10.0f * (float)k;
            float xb = xa + 10.0f;
            float ha = 8.0f + 6.0f * std::sin((float)k * 0.9f) + 3.0f * Ruido01Racimo(k, 801);
            float hb = 8.0f + 6.0f * std::sin((float)(k + 1) * 0.9f) + 3.0f * Ruido01Racimo(k + 1, 801);
            QuadNieblaRacimo({ xa, -0.5f, -92.0f }, { xb, -0.5f, -92.0f }, { xb, hb, -92.0f }, { xa, ha, -92.0f }, Color{ 40, 64, 58, 255 });
        }
        rlEnd();

        LineaArbolesRacimo(-60.0f, -120.0f, 120.0f, 3.4f, 16.0f, 27.0f, 811, Color{ 30, 54, 46, 255 }, 46.0f, 66.0f);
    }

    // Claro frente a la luna (su reflejo marca la x a cada profundidad).
    LineaArbolesRacimo(-37.0f, -90.0f, 90.0f, 2.6f, 11.0f, 19.0f, 821, Color{ 24, 42, 36, 255 }, 31.0f, 49.0f);
}


//--------------------------------------------------
// Agua del pantano
//--------------------------------------------------

// x del reflejo de la luna a la profundidad z.
static float ReflejoLunaXRacimo(float z)
{
    float profundidad = 0.9777f * (OJO_RACIMO.z - z) + 0.2102f * OJO_RACIMO.y;
    return PENDIENTE_LUNA_RACIMO * profundidad;
}


static Color ColorAguaRacimo(float x, float z, float t, bool efectos)
{
    float ola = 0.5f;
    if (efectos)
    {
        ola = 0.5f + 0.25f * std::sin(x * 0.42f + t * 0.9f) * std::cos(z * 0.55f - t * 0.6f) +
            0.25f * std::sin((x + z) * 0.23f - t * 0.5f);
    }

    // Manchas de algas y agua mas honda, por hash de celda.
    int cx = (int)std::floor(x / 6.0f), cz = (int)std::floor(z / 6.0f);
    float mancha = Ruido01Racimo(cx * 53 + cz * 17, 901);

    Color c = TonoRacimo(COLOR_AGUA_RACIMO, 0.88f + 0.2f * ola);
    if (mancha > 0.7f) c = MezclaRacimo(c, Color{ 34, 64, 40, 255 }, (mancha - 0.7f) * 1.6f);

    // Franja clara donde se refleja la luna.
    float dx = x - ReflejoLunaXRacimo(z);
    float ancho = 1.6f + (OJO_RACIMO.z - z) * 0.05f;
    float luna = std::exp(-(dx * dx) / (2.0f * ancho * ancho));
    c = MezclaRacimo(c, COLOR_AGUA_CLARA_RACIMO, luna * (0.45f + 0.2f * ola));

    return c;
}


static void DibujarAguaRacimo(float t, bool efectos)
{
    // Filas cada vez mas anchas hacia el fondo.
    const float filas[] = { 14.0f, 10.0f, 7.5f, 6.0f, 4.5f, 3.0f, 1.5f, 0.0f, -1.5f, -3.0f, -4.5f, -6.0f,
        -8.0f, -10.0f, -12.5f, -15.0f, -18.0f, -21.0f, -25.0f, -29.0f, -34.0f, -40.0f, -50.0f, -64.0f };
    const int cantidad = (int)(sizeof(filas) / sizeof(filas[0]));
    const float X0 = -72.0f, PASO_X = 3.0f;
    const int COLUMNAS = 48;

    rlBegin(RL_QUADS);

    for (int f = 0; f + 1 < cantidad; f++)
    {
        float za = filas[f], zb = filas[f + 1];

        for (int k = 0; k < COLUMNAS; k++)
        {
            float xa = X0 + PASO_X * (float)k;
            float xb = xa + PASO_X;
            Vector3 p0 = { xa, 0.0f, za }, p1 = { xb, 0.0f, za }, p2 = { xb, 0.0f, zb }, p3 = { xa, 0.0f, zb };
            VerticeRacimo(p0, TinteRacimo(ColorAguaRacimo(xa, za, t, efectos), p0));
            VerticeRacimo(p1, TinteRacimo(ColorAguaRacimo(xb, za, t, efectos), p1));
            VerticeRacimo(p2, TinteRacimo(ColorAguaRacimo(xb, zb, t, efectos), p2));
            VerticeRacimo(p3, TinteRacimo(ColorAguaRacimo(xa, zb, t, efectos), p3));
        }
    }

    rlEnd();
}


//--------------------------------------------------
// Arbol gigante: tronco con raices, rama del racimo y copa
//--------------------------------------------------

static const Vector3 BASE_ARBOL_RACIMO = { -10.0f, 0.0f, -6.0f };

// Puntos de la rama que sostiene el racimo (del tronco a la punta).
static const Vector3 RAMA_RACIMO[6] =
{
    { -9.4f, 10.0f, -5.6f },
    { -6.0f, 11.4f, -3.6f },
    { -2.6f, 12.1f, -1.3f },
    { 0.0f, 12.25f, 0.0f },
    { 3.2f, 12.8f, -0.9f },
    { 6.6f, 13.9f, -2.3f }
};
static const float RADIO_RAMA_RACIMO[6] = { 0.8f, 0.6f, 0.46f, 0.38f, 0.24f, 0.1f };


// Musgo colgante (barba de viejo) que se mece con la brisa.
static void MusgoColganteRacimo(Vector3 arriba, float largo, float t, int semilla, bool efectos)
{
    float vaiven = efectos ? std::sin(t * 0.9f + (float)semilla * 1.3f) * 0.16f : 0.0f;
    Vector3 medio = { arriba.x + vaiven * 0.4f, arriba.y - largo * 0.55f, arriba.z };
    Vector3 abajo = { arriba.x + vaiven, arriba.y - largo, arriba.z + vaiven * 0.3f };
    Color c = TonoRacimo(COLOR_BARBA_RACIMO, 0.85f + 0.25f * Ruido01Racimo(semilla, 41));
    TiraRacimo(arriba, medio, 0.13f, 0.09f, TonoRacimo(c, 0.8f), c);
    TiraRacimo(medio, abajo, 0.09f, 0.015f, c, TonoRacimo(c, 1.1f));
}


static void HongoRacimo(Vector3 base, float escala, float giro)
{
    Vector3 copa = { base.x + std::sin(giro) * 0.06f * escala, base.y + 0.28f * escala, base.z };
    PrismaRacimo(base, copa, 0.05f * escala, 0.04f * escala, 5, Color{ 210, 220, 196, 255 }, 0.4f);
    PrismaRacimo(copa, { copa.x, copa.y + 0.12f * escala, copa.z }, 0.2f * escala, 0.03f * escala, 7, COLOR_HONGO_RACIMO, 0.75f, giro);
}


static Vector3 PosicionHongoArbolRacimo(int k)
{
    const Vector3 hongos[7] =
    {
        { -7.6f, 0.75f, -4.2f }, { -7.3f, 0.6f, -3.9f }, { -12.6f, 0.8f, -4.0f },
        { -8.4f, 1.3f, -7.9f }, { -7.0f, 0.45f, -6.9f }, { -12.0f, 0.5f, -3.1f },
        { -11.9f, 1.15f, -4.6f }
    };
    return hongos[k % 7];
}


static void DibujarArbolGiganteRacimo(float t, bool decoracion, bool efectos)
{
    Vector3 b = BASE_ARBOL_RACIMO;

    // Tronco ensanchado, con algo de inclinacion hacia el racimo.
    const Vector3 tronco[6] =
    {
        { b.x, -0.4f, b.z }, { b.x + 0.1f, 2.2f, b.z }, { b.x + 0.3f, 6.0f, b.z + 0.1f },
        { b.x + 0.4f, 10.5f, b.z + 0.3f }, { b.x + 0.2f, 15.0f, b.z + 0.2f }, { b.x - 0.2f, 20.0f, b.z }
    };
    const float radios[6] = { 2.3f, 1.55f, 1.3f, 1.12f, 1.0f, 0.85f };

    for (int k = 0; k + 1 < 6; k++)
    {
        PrismaRacimo(tronco[k], tronco[k + 1], radios[k], radios[k + 1], 11, k % 2 ? COLOR_CORTEZA_RACIMO : TonoRacimo(COLOR_CORTEZA_RACIMO, 0.94f), 0.0f, 0.2f * (float)k);
    }

    // Vetas de corteza: costillas verticales mas oscuras.
    for (int k = 0; k < 7; k++)
    {
        float a = 0.5f + (float)k * 0.9f;
        float r0 = 1.55f, r1 = 1.0f;
        Vector3 p0 = { b.x + 0.1f + std::cos(a) * r0, 1.8f, b.z + std::sin(a) * r0 };
        Vector3 p1 = { b.x + 0.35f + std::cos(a) * r1, 13.0f, b.z + 0.2f + std::sin(a) * r1 };
        PrismaRacimo(p0, p1, 0.16f, 0.09f, 4, COLOR_CORTEZA_OSCURA_RACIMO);
    }

    // Raices de contrafuerte que bajan al agua.
    for (int k = 0; k < 8; k++)
    {
        float a = (float)k * 0.785f + 0.3f * Ruido01Racimo(k, 51);
        float alcance = 3.4f + 1.4f * Ruido01Racimo(k, 52);
        Vector3 nace = { b.x + std::cos(a) * 1.2f, 2.6f, b.z + std::sin(a) * 1.2f };
        Vector3 codo = { b.x + std::cos(a) * alcance * 0.6f, 0.9f, b.z + std::sin(a) * alcance * 0.6f };
        Vector3 fin = { b.x + std::cos(a) * alcance, -0.3f, b.z + std::sin(a) * alcance };
        PrismaRacimo(nace, codo, 0.62f, 0.42f, 6, COLOR_CORTEZA_RACIMO);
        PrismaRacimo(codo, fin, 0.42f, 0.16f, 6, TonoRacimo(COLOR_CORTEZA_RACIMO, 0.92f));

        if (decoracion && k % 2 == 0)
        {
            // Musgo sobre el lomo de la raiz.
            PrismaRacimo({ codo.x, codo.y + 0.3f, codo.z }, { (codo.x + fin.x) * 0.5f, (codo.y + fin.y) * 0.5f + 0.22f, (codo.z + fin.z) * 0.5f }, 0.22f, 0.12f, 5, COLOR_MUSGO_RACIMO);
        }
    }

    // Rama del racimo.
    for (int k = 0; k + 1 < 6; k++)
    {
        PrismaRacimo(RAMA_RACIMO[k], RAMA_RACIMO[k + 1], RADIO_RAMA_RACIMO[k], RADIO_RAMA_RACIMO[k + 1], 8, COLOR_CORTEZA_RACIMO);
    }

    // Musgo sobre la rama y nudo de donde nace el racimo.
    for (int k = 0; k + 2 < 6; k++)
    {
        Vector3 a = RAMA_RACIMO[k], c = RAMA_RACIMO[k + 1];
        PrismaRacimo({ a.x, a.y + RADIO_RAMA_RACIMO[k] * 0.7f, a.z }, { c.x, c.y + RADIO_RAMA_RACIMO[k + 1] * 0.7f, c.z }, RADIO_RAMA_RACIMO[k] * 0.5f, RADIO_RAMA_RACIMO[k + 1] * 0.5f, 5, COLOR_MUSGO_RACIMO);
    }
    RocaRacimo({ 0.0f, 12.1f, 0.0f }, { 0.42f, 0.34f, 0.42f }, 61, COLOR_CORTEZA_OSCURA_RACIMO);

    // Ramitas secas.
    PrismaRacimo(RAMA_RACIMO[2], { -2.0f, 13.6f, -1.8f }, 0.16f, 0.04f, 5, COLOR_CORTEZA_RACIMO);
    PrismaRacimo(RAMA_RACIMO[4], { 3.8f, 14.1f, -0.3f }, 0.1f, 0.03f, 5, COLOR_CORTEZA_RACIMO);
    PrismaRacimo(RAMA_RACIMO[1], { -5.2f, 10.2f, -2.6f }, 0.14f, 0.04f, 5, COLOR_CORTEZA_RACIMO);

    // Otras ramas hacia la izquierda y arriba.
    PrismaRacimo({ b.x - 0.2f, 13.0f, b.z }, { b.x - 4.8f, 15.4f, b.z - 1.0f }, 0.7f, 0.45f, 8, COLOR_CORTEZA_RACIMO);
    PrismaRacimo({ b.x - 4.8f, 15.4f, b.z - 1.0f }, { b.x - 8.6f, 16.6f, b.z - 2.0f }, 0.45f, 0.18f, 7, COLOR_CORTEZA_RACIMO);
    PrismaRacimo({ b.x + 0.3f, 15.0f, b.z }, { b.x + 3.0f, 18.5f, b.z - 0.5f }, 0.6f, 0.3f, 7, COLOR_CORTEZA_RACIMO);

    // Copa: matas de follaje oscuro, mas claras arriba.
    const Vector3 matas[7] =
    {
        { -10.5f, 18.6f, -6.5f }, { -4.6f, 16.0f, -3.6f }, { 1.6f, 15.6f, -1.8f },
        { -16.0f, 17.0f, -7.6f }, { 5.8f, 15.2f, -2.8f }, { -7.5f, 19.5f, -5.0f }, { -13.0f, 15.2f, -4.2f }
    };
    const Vector3 radiosMata[7] =
    {
        { 6.0f, 2.8f, 4.4f }, { 4.2f, 1.9f, 3.2f }, { 3.7f, 1.6f, 3.0f },
        { 4.6f, 2.0f, 3.6f }, { 3.0f, 1.3f, 2.5f }, { 4.4f, 2.0f, 3.6f }, { 3.4f, 1.5f, 2.8f }
    };

    for (int k = 0; k < 7; k++)
    {
        RocaRacimo(matas[k], radiosMata[k], 70 + k, k % 2 ? COLOR_HOJA_RACIMO : TonoRacimo(COLOR_HOJA_RACIMO, 1.12f), 0.22f);
        RocaRacimo({ matas[k].x + 0.4f, matas[k].y + radiosMata[k].y * 0.45f, matas[k].z + 0.3f }, EscalaRacimo(radiosMata[k], 0.6f), 90 + k, COLOR_HOJA_CLARA_RACIMO, 0.25f);
    }

    if (!decoracion)
    {
        return;
    }

    // Barba de viejo colgando de la rama y de la copa (nunca delante del
    // racimo: el centro queda libre para leer los frutos).
    for (int k = 0; k < 22; k++)
    {
        float u = (float)k / 21.0f;
        int tramo = (int)(u * 4.99f);
        float f = u * 5.0f - (float)tramo;
        Vector3 a = RAMA_RACIMO[tramo], c = RAMA_RACIMO[tramo + 1];
        Vector3 p = { a.x + (c.x - a.x) * f, a.y + (c.y - a.y) * f - 0.25f, a.z + (c.z - a.z) * f };
        if (std::fabs(p.x) < 1.3f) continue;
        float largo = 1.0f + 2.6f * Ruido01Racimo(k, 43);
        MusgoColganteRacimo(p, largo, t, k, efectos);
    }

    for (int k = 0; k < 10; k++)
    {
        const Vector3& mata = matas[k % 7];
        Vector3 p = { mata.x + (Ruido01Racimo(k, 44) - 0.5f) * 5.0f, mata.y - radiosMata[k % 7].y * 0.6f, mata.z + 1.2f + Ruido01Racimo(k, 45) };
        if (std::fabs(p.x) < 1.5f) continue;
        MusgoColganteRacimo(p, 1.4f + 2.2f * Ruido01Racimo(k, 46), t, 30 + k, efectos);
    }

    // Lianas largas junto al tronco.
    for (int k = 0; k < 4; k++)
    {
        float x = b.x + 1.6f + (float)k * 0.7f;
        Vector3 arriba = { x, 14.0f, b.z + 1.4f + (float)k * 0.3f };
        float vaiven = efectos ? std::sin(t * 0.7f + (float)k) * 0.2f : 0.0f;
        Vector3 abajo = { x + vaiven, 4.5f + (float)k * 1.3f, arriba.z };
        PrismaRacimo(arriba, abajo, 0.05f, 0.04f, 4, COLOR_TALLO_RACIMO);
        HojaRacimo(abajo, { abajo.x + 0.3f, abajo.y - 0.5f, abajo.z + 0.1f }, { 0.12f, 0.0f, 0.1f }, COLOR_HOJA_CLARA_RACIMO);
    }

    // Rodillas de cipres y hongos que brillan entre las raices.
    for (int k = 0; k < 9; k++)
    {
        float a = (float)k * 0.7f + 0.2f;
        float r = 4.6f + 1.6f * Ruido01Racimo(k, 53);
        Vector3 p = { b.x + std::cos(a) * r, -0.1f, b.z + std::sin(a) * r * 0.8f };
        if (p.z > 1.2f) continue;
        float h = 0.45f + 0.5f * Ruido01Racimo(k, 54);
        PrismaRacimo(p, { p.x, h, p.z }, 0.22f, 0.06f, 6, COLOR_CORTEZA_RACIMO);
    }

    for (int k = 0; k < 7; k++)
    {
        HongoRacimo(PosicionHongoArbolRacimo(k), 0.9f + 0.5f * Ruido01Racimo(k, 55), (float)k);
    }
}


//--------------------------------------------------
// Media distancia: cipreses, cabanas sobre pilotes y pasarela
//--------------------------------------------------

static void CipresRacimo(Vector3 base, float alto, float radio, int semilla, bool seco, float t, bool efectos)
{
    PrismaRacimo({ base.x, -0.4f, base.z }, { base.x, 1.4f, base.z }, radio * 2.3f, radio * 1.2f, 8, COLOR_CORTEZA_RACIMO);
    PrismaRacimo({ base.x, 1.4f, base.z }, { base.x + 0.2f, alto * 0.6f, base.z }, radio * 1.2f, radio * 0.8f, 8, COLOR_CORTEZA_RACIMO, 0.0f, 0.3f);
    PrismaRacimo({ base.x + 0.2f, alto * 0.6f, base.z }, { base.x, alto, base.z }, radio * 0.8f, radio * 0.35f, 7, COLOR_CORTEZA_RACIMO);

    if (seco)
    {
        for (int k = 0; k < 4; k++)
        {
            float a = (float)k * 1.6f + Ruido01Racimo(k, semilla);
            float y = alto * (0.55f + 0.1f * (float)k);
            float l = alto * (0.18f + 0.1f * Ruido01Racimo(k, semilla + 1));
            PrismaRacimo({ base.x, y, base.z }, { base.x + std::cos(a) * l, y + l * 0.7f, base.z + std::sin(a) * l * 0.5f }, radio * 0.32f, 0.04f, 5, COLOR_CORTEZA_OSCURA_RACIMO);
        }
        return;
    }

    // Copas en capas, achatadas, como los cipreses calvos del pantano.
    for (int k = 0; k < 4; k++)
    {
        float y = alto * (0.62f + 0.13f * (float)k);
        float r = radio * (5.2f - 0.9f * (float)k);
        Vector3 c = { base.x + (Ruido01Racimo(k, semilla + 2) - 0.5f) * radio * 2.0f, y, base.z + (Ruido01Racimo(k, semilla + 3) - 0.5f) * radio };
        RocaRacimo(c, { r, radio * 1.5f, r * 0.8f }, semilla + k, k % 2 ? COLOR_HOJA_RACIMO : TonoRacimo(COLOR_HOJA_RACIMO, 1.15f), 0.25f);

        float largo = 1.0f + 1.8f * Ruido01Racimo(k, semilla + 4);
        MusgoColganteRacimo({ c.x - r * 0.5f, y - radio, c.z + r * 0.5f }, largo, t, semilla + k, efectos);
        MusgoColganteRacimo({ c.x + r * 0.45f, y - radio, c.z + r * 0.4f }, largo * 0.8f, t, semilla + k + 7, efectos);
    }
}


// Escalera de mano entre dos alturas.
static void EscaleraRacimo(Vector3 abajo, Vector3 arriba, float ancho)
{
    PrismaRacimo({ abajo.x - ancho, abajo.y, abajo.z }, { arriba.x - ancho, arriba.y, arriba.z }, 0.05f, 0.05f, 4, COLOR_MADERA_OSCURA_RACIMO);
    PrismaRacimo({ abajo.x + ancho, abajo.y, abajo.z }, { arriba.x + ancho, arriba.y, arriba.z }, 0.05f, 0.05f, 4, COLOR_MADERA_OSCURA_RACIMO);

    for (int k = 1; k < 6; k++)
    {
        float f = (float)k / 6.0f;
        Vector3 p = { abajo.x + (arriba.x - abajo.x) * f, abajo.y + (arriba.y - abajo.y) * f, abajo.z + (arriba.z - abajo.z) * f };
        CuboRacimo(p, { ancho * 2.0f, 0.05f, 0.08f }, COLOR_MADERA_RACIMO);
    }
}


static void BarrilRacimo(Vector3 base, float escala)
{
    PrismaRacimo(base, { base.x, base.y + 0.62f * escala, base.z }, 0.26f * escala, 0.26f * escala, 8, COLOR_MADERA_RACIMO);
    PrismaRacimo({ base.x, base.y + 0.12f * escala, base.z }, { base.x, base.y + 0.17f * escala, base.z }, 0.275f * escala, 0.275f * escala, 8, COLOR_HIERRO_RACIMO);
    PrismaRacimo({ base.x, base.y + 0.45f * escala, base.z }, { base.x, base.y + 0.5f * escala, base.z }, 0.275f * escala, 0.275f * escala, 8, COLOR_HIERRO_RACIMO);
}


// Farol colgante (cuerpo; la luz va en la pasada aditiva).
static void FarolRacimo(Vector3 centro, bool encendido)
{
    Color vidrio = encendido ? Color{ 255, 220, 140, 255 } : Color{ 54, 60, 56, 255 };
    CuboCrudoRacimo(centro, { 0.13f, 0.2f, 0.13f }, vidrio);
    CuboRacimo({ centro.x, centro.y + 0.13f, centro.z }, { 0.22f, 0.05f, 0.22f }, COLOR_HIERRO_RACIMO);
    CuboRacimo({ centro.x, centro.y - 0.12f, centro.z }, { 0.2f, 0.04f, 0.2f }, COLOR_HIERRO_RACIMO);
    PrismaRacimo({ centro.x, centro.y + 0.15f, centro.z }, { centro.x, centro.y + 0.3f, centro.z }, 0.12f, 0.0f, 4, COLOR_HIERRO_RACIMO, 0.0f, 0.785f);

    for (int k = 0; k < 4; k++)
    {
        float sx = k % 2 ? 1.0f : -1.0f, sz = k < 2 ? 1.0f : -1.0f;
        CuboRacimo({ centro.x + sx * 0.08f, centro.y, centro.z + sz * 0.08f }, { 0.025f, 0.22f, 0.025f }, COLOR_HIERRO_RACIMO);
    }
}


// Posiciones de las luces de las cabanas (las usa tambien la pasada de luces).
static Vector3 FarolCabanaRacimo(Vector3 base, float s)
{
    return { base.x - 2.05f * s, 3.75f * s, base.z + 2.15f * s };
}


static void CabanaRacimo(Vector3 base, float s, bool detalle, int semilla)
{
    float x = base.x, z = base.z;
    float piso = 2.6f * s;

    // Pilotes con riostras.
    for (int k = 0; k < 6; k++)
    {
        float px = x + (k % 3 - 1) * 2.0f * s;
        float pz = z + (k < 3 ? 1.75f : -1.4f) * s;
        PrismaRacimo({ px, -0.5f, pz }, { px, piso, pz }, 0.15f * s, 0.13f * s, 6, COLOR_MADERA_OSCURA_RACIMO);
        // Linea de musgo y humedad al ras del agua.
        PrismaRacimo({ px, -0.05f, pz }, { px, 0.35f * s, pz }, 0.165f * s, 0.15f * s, 6, Color{ 52, 74, 52, 255 });
    }
    PrismaRacimo({ x - 2.0f * s, 0.4f, z + 1.75f * s }, { x, piso - 0.2f, z + 1.75f * s }, 0.06f * s, 0.06f * s, 4, COLOR_MADERA_OSCURA_RACIMO);
    PrismaRacimo({ x + 2.0f * s, 0.4f, z + 1.75f * s }, { x, piso - 0.2f, z + 1.75f * s }, 0.06f * s, 0.06f * s, 4, COLOR_MADERA_OSCURA_RACIMO);

    // Plataforma de tablones.
    CuboRacimo({ x, piso, z + 0.2f * s }, { 4.8f * s, 0.2f * s, 4.0f * s }, COLOR_MADERA_RACIMO);
    for (int k = 0; k < 9; k++)
    {
        float tx = x + (-2.15f + 0.54f * (float)k) * s;
        Color c = TonoRacimo(COLOR_MADERA_RACIMO, 0.86f + 0.22f * Ruido01Racimo(k, semilla));
        CuboRacimo({ tx, piso + 0.11f * s, z + 0.2f * s }, { 0.5f * s, 0.03f, 4.0f * s }, c);
    }

    // Cuerpo de la cabana: tablas verticales.
    float yc = piso + 1.4f * s;
    float frente = z + 1.1f * s;
    CuboRacimo({ x, yc, z - 0.3f * s }, { 3.6f * s, 2.6f * s, 2.8f * s }, COLOR_MADERA_OSCURA_RACIMO);
    for (int k = 0; k < 9; k++)
    {
        float tx = x + (-1.6f + 0.4f * (float)k) * s;
        Color c = TonoRacimo(COLOR_MADERA_RACIMO, 0.78f + 0.2f * Ruido01Racimo(k, semilla + 1));
        CuboRacimo({ tx, yc, frente + 0.03f }, { 0.37f * s, 2.6f * s, 0.06f }, c);
    }

    // Ventana encendida con cruz, y puerta.
    CuboRacimo({ x + 0.8f * s, yc + 0.15f * s, frente + 0.08f }, { 1.0f * s, 0.86f * s, 0.06f }, COLOR_MADERA_OSCURA_RACIMO);
    CuboCrudoRacimo({ x + 0.8f * s, yc + 0.15f * s, frente + 0.1f }, { 0.78f * s, 0.64f * s, 0.06f }, Color{ 255, 206, 120, 255 });
    CuboRacimo({ x + 0.8f * s, yc + 0.15f * s, frente + 0.13f }, { 0.06f * s, 0.64f * s, 0.03f }, COLOR_MADERA_OSCURA_RACIMO);
    CuboRacimo({ x + 0.8f * s, yc + 0.15f * s, frente + 0.13f }, { 0.78f * s, 0.06f * s, 0.03f }, COLOR_MADERA_OSCURA_RACIMO);
    CuboRacimo({ x - 0.75f * s, piso + 0.9f * s, frente + 0.08f }, { 0.8f * s, 1.6f * s, 0.06f }, Color{ 64, 44, 32, 255 });
    CuboCrudoRacimo({ x - 0.75f * s, piso + 0.9f * s, frente + 0.115f }, { 0.05f * s, 1.5f * s, 0.02f }, Color{ 255, 190, 110, 255 });

    // Ventana lateral.
    CuboCrudoRacimo({ x + 1.82f * s, yc + 0.2f * s, z - 0.3f * s }, { 0.06f, 0.6f * s, 0.7f * s }, Color{ 240, 188, 110, 255 });

    // Techo de paja a dos aguas con parches de musgo.
    float alero = yc + 1.3f * s;
    float cumbre = alero + 1.6f * s;
    float x0 = x - 2.35f * s, x1 = x + 2.35f * s;
    float zf = z + 1.9f * s, zb = z - 2.5f * s, zc = z - 0.3f * s;
    rlBegin(RL_QUADS);
    QuadNieblaRacimo({ x0, alero, zf }, { x1, alero, zf }, { x1, cumbre, zc }, { x0, cumbre, zc }, COLOR_PAJA_RACIMO);
    QuadNieblaRacimo({ x1, alero, zb }, { x0, alero, zb }, { x0, cumbre, zc }, { x1, cumbre, zc }, TonoRacimo(COLOR_PAJA_RACIMO, 0.6f));
    // Hileras de paja (franjas mas oscuras).
    for (int k = 1; k < 4; k++)
    {
        float f = (float)k / 4.0f;
        float y = alero + (cumbre - alero) * f;
        float zz = zf + (zc - zf) * f;
        QuadNieblaRacimo({ x0, y - 0.06f, zz + 0.07f }, { x1, y - 0.06f, zz + 0.07f }, { x1, y + 0.02f, zz + 0.02f }, { x0, y + 0.02f, zz + 0.02f }, TonoRacimo(COLOR_PAJA_RACIMO, 0.72f));
    }
    // Hastiales.
    QuadNieblaRacimo({ x - 1.8f * s, alero, zf - 0.4f * s }, { x - 1.8f * s, alero, zb + 0.4f * s }, { x - 1.8f * s, cumbre - 0.1f, zc }, { x - 1.8f * s, cumbre - 0.1f, zc }, TonoRacimo(COLOR_MADERA_OSCURA_RACIMO, 0.8f));
    QuadNieblaRacimo({ x + 1.8f * s, alero, zf - 0.4f * s }, { x + 1.8f * s, alero, zb + 0.4f * s }, { x + 1.8f * s, cumbre - 0.1f, zc }, { x + 1.8f * s, cumbre - 0.1f, zc }, TonoRacimo(COLOR_MADERA_OSCURA_RACIMO, 0.7f));
    rlEnd();
    RocaRacimo({ x - 0.9f * s, cumbre - 0.6f * s, z + 0.6f * s }, { 0.8f * s, 0.12f * s, 0.5f * s }, semilla + 3, COLOR_MUSGO_RACIMO, 0.2f);
    RocaRacimo({ x + 1.3f * s, alero + 0.35f * s, z + 1.2f * s }, { 0.6f * s, 0.1f * s, 0.4f * s }, semilla + 4, COLOR_MUSGO_RACIMO, 0.2f);
    PrismaRacimo({ x0, cumbre, zc }, { x1, cumbre, zc }, 0.1f * s, 0.1f * s, 5, COLOR_MADERA_OSCURA_RACIMO);

    // Chimenea de lata.
    PrismaRacimo({ x + 1.2f * s, cumbre - 0.6f * s, z - 0.8f * s }, { x + 1.2f * s, cumbre + 0.9f * s, z - 0.8f * s }, 0.13f * s, 0.13f * s, 6, COLOR_HIERRO_RACIMO);

    // Farol del porche (colgado de un brazo).
    Vector3 farol = FarolCabanaRacimo(base, s);
    PrismaRacimo({ farol.x + 0.15f * s, piso, farol.z }, { farol.x + 0.15f * s, farol.y + 0.75f * s, farol.z }, 0.07f, 0.07f, 4, COLOR_MADERA_OSCURA_RACIMO);
    PrismaRacimo({ farol.x + 0.15f * s, farol.y + 0.7f * s, farol.z }, { farol.x - 0.05f, farol.y + 0.7f * s, farol.z }, 0.04f, 0.04f, 4, COLOR_MADERA_OSCURA_RACIMO);
    FarolRacimo(farol, true);

    if (!detalle)
    {
        return;
    }

    // Baranda del porche.
    for (int k = 0; k < 5; k++)
    {
        float px = x + (-2.3f + 1.15f * (float)k) * s;
        if (k == 2) continue;
        PrismaRacimo({ px, piso, z + 2.1f * s }, { px, piso + 0.85f * s, z + 2.1f * s }, 0.05f, 0.05f, 4, COLOR_MADERA_OSCURA_RACIMO);
    }
    PrismaRacimo({ x - 2.3f * s, piso + 0.82f * s, z + 2.1f * s }, { x - 0.7f * s, piso + 0.82f * s, z + 2.1f * s }, 0.04f, 0.04f, 4, COLOR_MADERA_RACIMO);
    PrismaRacimo({ x + 0.7f * s, piso + 0.82f * s, z + 2.1f * s }, { x + 2.3f * s, piso + 0.82f * s, z + 2.1f * s }, 0.04f, 0.04f, 4, COLOR_MADERA_RACIMO);

    // Escalera al agua, barriles, cajon y red de pesca.
    EscaleraRacimo({ x, -0.3f, z + 2.75f * s }, { x, piso, z + 2.2f * s }, 0.28f * s);
    BarrilRacimo({ x + 1.7f * s, piso + 0.1f * s, z + 1.5f * s }, s);
    BarrilRacimo({ x + 1.15f * s, piso + 0.1f * s, z + 1.7f * s }, s * 0.85f);
    CuboGiradoRacimo({ x - 1.6f * s, piso + 0.36f * s, z + 1.45f * s }, { 0.6f * s, 0.5f * s, 0.6f * s }, 18.0f, COLOR_MADERA_RACIMO);

    Vector3 r0 = { x - 2.42f * s, piso + 0.8f * s, z + 1.6f * s };
    Vector3 r1 = { x - 2.42f * s, piso - 1.2f * s, z - 0.6f * s };
    Color red = TinteRacimo(Color{ 150, 150, 120, 200 }, r0);
    for (int k = 0; k <= 5; k++)
    {
        float f = (float)k / 5.0f;
        DrawLine3D({ r0.x, r0.y - 0.2f * std::sin(f * 3.14f), r0.z + (r1.z - r0.z) * f }, { r0.x, r1.y + 0.4f * std::sin(f * 3.14f), r0.z + (r1.z - r0.z) * f }, red);
        DrawLine3D({ r0.x, r0.y + (r1.y - r0.y) * f, r0.z }, { r0.x, r0.y + (r1.y - r0.y) * f - 0.25f, r1.z }, red);
    }
}


// Pasarela de tablones sobre pilotes, de la cabana hacia el borde.
static void PasarelaRacimo(Vector3 a, Vector3 b, float y, int semilla)
{
    float dx = b.x - a.x, dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);
    int tablas = (int)(largo / 0.55f);
    float giro = std::atan2(dx, dz) * RAD2DEG;

    for (int k = 0; k < tablas; k++)
    {
        float f = ((float)k + 0.5f) / (float)tablas;
        Vector3 p = { a.x + dx * f, y + 0.04f * Ruido01Racimo(k, semilla), a.z + dz * f };
        Color c = TonoRacimo(COLOR_MADERA_RACIMO, 0.78f + 0.25f * Ruido01Racimo(k, semilla + 1));
        CuboGiradoRacimo(p, { 1.3f, 0.09f, 0.48f }, giro, c);
    }

    int pilotes = (int)(largo / 2.6f) + 1;
    for (int k = 0; k <= pilotes; k++)
    {
        float f = (float)k / (float)pilotes;
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float nx = dz / largo * 0.6f * (float)lado, nz = -dx / largo * 0.6f * (float)lado;
            Vector3 p = { a.x + dx * f + nx, 0.0f, a.z + dz * f + nz };
            PrismaRacimo({ p.x, -0.4f, p.z }, { p.x, y + 0.7f, p.z }, 0.09f, 0.08f, 5, COLOR_MADERA_OSCURA_RACIMO);
        }
    }

    // Soga de baranda.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float nx = dz / largo * 0.6f * (float)lado, nz = -dx / largo * 0.6f * (float)lado;
        for (int k = 0; k < pilotes; k++)
        {
            float f0 = (float)k / (float)pilotes, f1 = (float)(k + 1) / (float)pilotes;
            Vector3 p0 = { a.x + dx * f0 + nx, y + 0.65f, a.z + dz * f0 + nz };
            Vector3 p1 = { a.x + dx * f1 + nx, y + 0.65f, a.z + dz * f1 + nz };
            Vector3 m = { (p0.x + p1.x) * 0.5f, y + 0.45f, (p0.z + p1.z) * 0.5f };
            PrismaRacimo(p0, m, 0.025f, 0.025f, 3, Color{ 150, 130, 96, 255 });
            PrismaRacimo(m, p1, 0.025f, 0.025f, 3, Color{ 150, 130, 96, 255 });
        }
    }
}


static const Vector3 CABANA_PRINCIPAL_RACIMO = { 13.0f, 0.0f, -9.5f };
static const Vector3 CABANA_IZQUIERDA_RACIMO = { -20.0f, 0.0f, -17.0f };
static const Vector3 CABANA_LEJANA_RACIMO = { 31.0f, 0.0f, -21.0f };


static void DibujarMediaDistanciaRacimo(float t, int paso, bool efectos)
{
    // Cipreses metidos en el agua, en dos profundidades.
    struct Cipres { Vector3 base; float alto; float radio; bool seco; };
    const Cipres cipreses[11] =
    {
        { { -21.0f, 0.0f, -4.5f }, 17.0f, 0.75f, false },
        { { -27.0f, 0.0f, -11.0f }, 19.0f, 0.8f, false },
        { { -5.0f, 0.0f, -20.0f }, 15.0f, 0.6f, false },
        { { 5.5f, 0.0f, -16.0f }, 11.0f, 0.45f, true },
        { { 19.0f, 0.0f, -13.0f }, 16.0f, 0.65f, false },
        { { 24.0f, 0.0f, -4.0f }, 15.0f, 0.7f, false },
        { { -14.0f, 0.0f, -27.0f }, 17.0f, 0.65f, false },
        { { 10.0f, 0.0f, -29.0f }, 18.0f, 0.65f, false },
        { { -33.0f, 0.0f, -1.0f }, 19.0f, 0.8f, false },
        { { 1.0f, 0.0f, -27.0f }, 10.0f, 0.4f, true },
        { { 34.0f, 0.0f, -9.0f }, 18.0f, 0.75f, false }
    };

    for (int k = 0; k < 11; k += paso)
    {
        CipresRacimo(cipreses[k].base, cipreses[k].alto, cipreses[k].radio, 300 + k * 11, cipreses[k].seco, t, efectos);
    }

    CabanaRacimo(CABANA_PRINCIPAL_RACIMO, 1.0f, true, 401);
    CabanaRacimo(CABANA_IZQUIERDA_RACIMO, 1.15f, paso == 1, 411);
    CabanaRacimo(CABANA_LEJANA_RACIMO, 0.95f, false, 421);

    // Pasarelas: de la cabana principal hacia la derecha y hacia el fondo.
    PasarelaRacimo({ CABANA_PRINCIPAL_RACIMO.x + 2.6f, 0.0f, CABANA_PRINCIPAL_RACIMO.z + 0.4f }, { 34.0f, 0.0f, -7.0f }, 2.45f, 431);
    if (paso == 1)
    {
        PasarelaRacimo({ CABANA_IZQUIERDA_RACIMO.x + 3.0f, 0.0f, CABANA_IZQUIERDA_RACIMO.z + 0.5f }, { -12.5f, 0.0f, -21.0f }, 2.7f, 441);
    }
}


//--------------------------------------------------
// Anillo cercano: nenufares, juncos, troncos, rocas, estacas, canoa
//--------------------------------------------------

struct NenufarRacimo { float x, z, r; bool flor; };

static const NenufarRacimo NENUFARES_RACIMO[] =
{
    { -3.6f, -1.3f, 0.6f, true }, { -2.3f, 0.5f, 0.42f, false }, { 2.9f, -0.7f, 0.55f, true },
    { 4.3f, 0.9f, 0.4f, false }, { 1.5f, 1.6f, 0.34f, false }, { -1.3f, 1.8f, 0.3f, false },
    { -11.6f, 1.2f, 0.62f, true }, { -13.0f, 4.4f, 0.5f, false }, { 11.2f, 0.4f, 0.56f, false },
    { 12.7f, 5.2f, 0.62f, true }, { -6.2f, -2.9f, 0.5f, false }, { 6.8f, -3.6f, 0.5f, true },
    { -4.9f, 5.7f, 0.5f, false }, { 4.8f, 6.1f, 0.58f, true }, { 0.4f, 5.6f, 0.44f, false },
    { 9.7f, 6.3f, 0.5f, false }, { -10.1f, 6.4f, 0.55f, true }, { -2.1f, 6.6f, 0.4f, false },
    { 8.2f, -12.0f, 0.8f, false }, { -4.0f, -13.5f, 0.75f, true }, { 2.6f, -9.0f, 0.6f, false },
    { 15.0f, -15.0f, 0.9f, false }, { -15.0f, -9.5f, 0.85f, true }, { -1.0f, -7.0f, 0.55f, false }
};


static void NenufarDibujoRacimo(const NenufarRacimo& n, int k, float t, bool efectos)
{
    float y = 0.03f + (efectos ? std::sin(t * 1.3f + (float)k) * 0.012f : 0.0f);
    float giro = Ruido01Racimo(k, 61) * 6.2832f;
    Vector3 c = { n.x, y, n.z };
    Color hoja = TonoRacimo(Color{ 64, 124, 70, 255 }, 0.85f + 0.25f * Ruido01Racimo(k, 62));

    SectorRacimo(c, n.r, giro + 0.35f, giro + 6.2832f - 0.1f, TinteRacimo(hoja, c));
    SectorRacimo({ c.x, c.y + 0.005f, c.z }, n.r * 0.55f, giro + 0.6f, giro + 6.0f, TinteRacimo(TonoRacimo(hoja, 1.15f), c), 10);

    if (!n.flor)
    {
        return;
    }

    // Flor de nenufar: petalos rosados y centro amarillo.
    Vector3 f = { n.x + std::cos(giro + 3.1f) * n.r * 0.3f, y + 0.02f, n.z + std::sin(giro + 3.1f) * n.r * 0.3f };
    for (int p = 0; p < 7; p++)
    {
        float a = (float)p * 0.8976f + giro;
        Vector3 punta = { f.x + std::cos(a) * 0.24f, f.y + 0.15f, f.z + std::sin(a) * 0.24f };
        Vector3 lado = { -std::sin(a) * 0.07f, 0.0f, std::cos(a) * 0.07f };
        HojaRacimo(f, punta, lado, p % 2 ? Color{ 246, 196, 220, 255 } : Color{ 255, 226, 236, 255 });
    }
    RocaRacimo({ f.x, f.y + 0.07f, f.z }, { 0.06f, 0.05f, 0.06f }, k, Color{ 255, 214, 90, 255 }, 0.1f, 0.5f);
}


static void RanaRacimo(Vector3 p, float giro, float t, int semilla)
{
    float garganta = 0.5f + 0.5f * std::sin(t * 3.0f + (float)semilla);
    float dx = std::sin(giro), dz = std::cos(giro);
    RocaRacimo({ p.x, p.y + 0.1f, p.z }, { 0.17f, 0.11f, 0.15f }, semilla, Color{ 92, 150, 70, 255 }, 0.12f);
    RocaRacimo({ p.x + dx * 0.11f, p.y + 0.06f, p.z + dz * 0.11f }, { 0.07f + 0.03f * garganta, 0.05f + 0.02f * garganta, 0.07f + 0.03f * garganta }, semilla + 1, Color{ 230, 220, 150, 255 }, 0.05f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float ex = p.x + dx * 0.08f + dz * 0.07f * (float)lado;
        float ez = p.z + dz * 0.08f - dx * 0.07f * (float)lado;
        CuboRacimo({ ex, p.y + 0.21f, ez }, { 0.06f, 0.06f, 0.06f }, Color{ 236, 230, 170, 255 });
        CuboRacimo({ ex + dx * 0.025f, p.y + 0.215f, ez + dz * 0.025f }, { 0.03f, 0.035f, 0.03f }, Color{ 20, 20, 20, 255 });
    }
}


static void JuncosRacimo(Vector3 base, int semilla, float t, bool efectos)
{
    for (int k = 0; k < 8; k++)
    {
        float ox = (Ruido01Racimo(k, semilla) - 0.5f) * 1.3f;
        float oz = (Ruido01Racimo(k, semilla + 1) - 0.5f) * 0.9f;
        float h = 1.1f + 1.3f * Ruido01Racimo(k, semilla + 2);
        float inclina = (Ruido01Racimo(k, semilla + 3) - 0.5f) * 0.6f;
        float vaiven = efectos ? std::sin(t * 1.4f + (float)k + (float)semilla) * 0.07f * h : 0.0f;
        Vector3 pie = { base.x + ox, -0.05f, base.z + oz };
        Vector3 punta = { pie.x + inclina + vaiven, h, pie.z + inclina * 0.3f };
        Color c = k % 3 == 0 ? COLOR_HOJA_CLARA_RACIMO : Color{ 70, 122, 64, 255 };
        TiraRacimo(punta, pie, 0.008f, 0.05f, TonoRacimo(c, 1.15f), TonoRacimo(c, 0.7f));
    }

    // Totoras con su espiga marron.
    for (int k = 0; k < 3; k++)
    {
        float ox = (Ruido01Racimo(k, semilla + 7) - 0.5f) * 1.0f;
        float oz = (Ruido01Racimo(k, semilla + 8) - 0.5f) * 0.7f;
        float h = 1.7f + 0.7f * Ruido01Racimo(k, semilla + 9);
        float vaiven = efectos ? std::sin(t * 1.2f + (float)k * 2.0f + (float)semilla) * 0.08f : 0.0f;
        Vector3 pie = { base.x + ox, -0.05f, base.z + oz };
        Vector3 espiga = { pie.x + vaiven, h, pie.z };
        PrismaRacimo(pie, espiga, 0.025f, 0.02f, 4, Color{ 96, 132, 70, 255 });
        PrismaRacimo({ espiga.x, espiga.y - 0.42f, espiga.z }, { espiga.x + vaiven * 0.3f, espiga.y, espiga.z }, 0.07f, 0.07f, 6, Color{ 104, 66, 40, 255 });
        PrismaRacimo(espiga, { espiga.x + vaiven * 0.5f, espiga.y + 0.22f, espiga.z }, 0.012f, 0.008f, 3, Color{ 120, 140, 90, 255 });
    }
}


static void TroncoFlotanteRacimo(Vector3 a, Vector3 b, float radio, float t, int semilla, bool efectos)
{
    float flota = efectos ? std::sin(t * 1.1f + (float)semilla) * 0.03f : 0.0f;
    a.y += flota;
    b.y += flota;
    PrismaRacimo(a, b, radio, radio * 0.92f, 8, COLOR_CORTEZA_RACIMO);
    // Cortes claros en las puntas.
    PrismaRacimo(a, { a.x + (a.x - b.x) * 0.01f, a.y, a.z + (a.z - b.z) * 0.01f }, radio * 0.85f, radio * 0.85f, 8, Color{ 176, 140, 98, 255 }, 0.2f);
    // Lomo de musgo y hongos.
    PrismaRacimo({ a.x, a.y + radio * 0.75f, a.z }, { b.x, b.y + radio * 0.7f, b.z }, radio * 0.45f, radio * 0.35f, 5, COLOR_MUSGO_RACIMO);
    for (int k = 0; k < 3; k++)
    {
        float f = 0.25f + 0.25f * (float)k;
        Vector3 p = { a.x + (b.x - a.x) * f, a.y + radio * 0.85f, a.z + (b.z - a.z) * f + 0.1f };
        HongoRacimo(p, 0.7f + 0.3f * Ruido01Racimo(k, semilla), (float)k + (float)semilla);
    }
}


static void RocaMusgoRacimo(Vector3 p, float escala, int semilla)
{
    RocaRacimo({ p.x, 0.15f * escala, p.z }, { 0.9f * escala, 0.55f * escala, 0.7f * escala }, semilla, COLOR_PIEDRA_RACIMO, 0.2f);
    RocaRacimo({ p.x - 0.1f * escala, 0.48f * escala, p.z + 0.05f }, { 0.65f * escala, 0.18f * escala, 0.5f * escala }, semilla + 1, COLOR_MUSGO_RACIMO, 0.2f);
}


// Estacas de amarre con farol (fuentes de luz del anillo cercano).
static const Vector3 ESTACAS_RACIMO[2] = { { -11.4f, 0.0f, 2.2f }, { 11.4f, 0.0f, 2.2f } };

static Vector3 FarolEstacaRacimo(int k, float t)
{
    Vector3 e = ESTACAS_RACIMO[k];
    float lado = e.x < 0.0f ? 1.0f : -1.0f;
    return { e.x + lado * 0.62f + std::sin(t * 1.3f + (float)k) * 0.02f, 3.05f, e.z };
}


static void EstacaFarolRacimo(int k, float t)
{
    Vector3 e = ESTACAS_RACIMO[k];
    float lado = e.x < 0.0f ? 1.0f : -1.0f;
    PrismaRacimo({ e.x, -0.5f, e.z }, { e.x, 3.75f, e.z }, 0.16f, 0.13f, 6, COLOR_MADERA_OSCURA_RACIMO);
    PrismaRacimo({ e.x, 3.55f, e.z }, { e.x + lado * 0.75f, 3.6f, e.z }, 0.05f, 0.05f, 4, COLOR_MADERA_OSCURA_RACIMO);
    Vector3 f = FarolEstacaRacimo(k, t);
    PrismaRacimo({ f.x, 3.58f, f.z }, { f.x, f.y + 0.18f, f.z }, 0.012f, 0.012f, 3, COLOR_HIERRO_RACIMO);
    FarolRacimo(f, true);
    // Rollo de soga a media altura.
    for (int v = 0; v < 3; v++)
    {
        PrismaRacimo({ e.x, 1.0f + 0.07f * (float)v, e.z }, { e.x, 1.06f + 0.07f * (float)v, e.z }, 0.2f, 0.2f, 8, Color{ 156, 132, 92, 255 });
    }
}


static void CanoaRacimo(Vector3 c, float giro, float t, bool efectos)
{
    float flota = efectos ? std::sin(t * 1.2f) * 0.03f : 0.0f;
    float dx = std::cos(giro * DEG2RAD), dz = -std::sin(giro * DEG2RAD);
    Vector3 centro = { c.x, 0.12f + flota, c.z };
    CuboGiradoRacimo(centro, { 2.6f, 0.34f, 0.72f }, giro, COLOR_MADERA_OSCURA_RACIMO);
    CuboGiradoRacimo({ centro.x, centro.y + 0.15f, centro.z }, { 2.3f, 0.06f, 0.5f }, giro, Color{ 52, 38, 28, 255 });
    // Proa y popa en punta.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 a = { centro.x + dx * 1.3f * (float)lado, centro.y, centro.z + dz * 1.3f * (float)lado };
        Vector3 b = { centro.x + dx * 1.95f * (float)lado, centro.y + 0.14f, centro.z + dz * 1.95f * (float)lado };
        PrismaRacimo(a, b, 0.3f, 0.03f, 4, COLOR_MADERA_OSCURA_RACIMO, 0.0f, 0.785f);
    }
    // Remo cruzado.
    PrismaRacimo({ centro.x - dx * 0.9f + dz * 0.5f, centro.y + 0.22f, centro.z - dz * 0.9f - dx * 0.5f },
        { centro.x + dx * 0.7f - dz * 0.45f, centro.y + 0.24f, centro.z + dz * 0.7f + dx * 0.45f }, 0.03f, 0.03f, 4, COLOR_MADERA_RACIMO);
}


static void DibujarUtileriaRacimo(float t, int paso, bool efectos)
{
    const int cantidadNenufares = (int)(sizeof(NENUFARES_RACIMO) / sizeof(NENUFARES_RACIMO[0]));
    for (int k = 0; k < cantidadNenufares; k += (paso == 1 || k < 18) ? 1 : paso)
    {
        NenufarDibujoRacimo(NENUFARES_RACIMO[k], k, t, efectos);
    }

    RanaRacimo({ -3.75f, 0.05f, -1.2f }, 0.6f, t, 71);
    if (paso == 1)
    {
        RanaRacimo({ 12.55f, 0.05f, 5.1f }, -0.5f, t, 72);
    }

    const Vector3 juncos[9] =
    {
        { -12.8f, 0.0f, 3.0f }, { -14.8f, 0.0f, 0.4f }, { 13.4f, 0.0f, 2.6f }, { 15.4f, 0.0f, 5.4f },
        { -10.0f, 0.0f, -2.6f }, { 9.6f, 0.0f, -4.4f }, { -6.2f, 0.0f, -4.6f }, { 5.4f, 0.0f, -7.5f },
        { 16.5f, 0.0f, -1.0f }
    };
    for (int k = 0; k < 9; k += paso)
    {
        JuncosRacimo(juncos[k], 500 + k * 13, t, efectos);
    }

    TroncoFlotanteRacimo({ -5.6f, 0.08f, -2.0f }, { -2.4f, 0.08f, -3.3f }, 0.3f, t, 81, efectos);
    TroncoFlotanteRacimo({ 5.4f, 0.08f, -1.5f }, { 8.4f, 0.08f, -2.6f }, 0.27f, t, 82, efectos);

    RocaMusgoRacimo({ -12.4f, 0.0f, 5.8f }, 0.9f, 91);
    RocaMusgoRacimo({ 14.0f, 0.0f, 0.4f }, 1.0f, 92);
    RocaMusgoRacimo({ 7.6f, 0.0f, -6.4f }, 0.8f, 93);
    if (paso == 1)
    {
        RocaMusgoRacimo({ -15.6f, 0.0f, -3.2f }, 1.2f, 94);
        RocaMusgoRacimo({ 3.2f, 0.0f, -11.5f }, 0.9f, 95);
    }

    EstacaFarolRacimo(0, t);
    EstacaFarolRacimo(1, t);

    CanoaRacimo({ 9.2f, 0.0f, -6.4f }, 28.0f, t, efectos);
}


//--------------------------------------------------
// Racimo: tallo, pedicelos, hojas y frutos
//--------------------------------------------------

// Nudo de la rama del arbol del que cuelga el racimo.
static const Vector3 NUDO_RACIMO = { 0.0f, 12.1f, 0.0f };


static ModeloRacimoToxico3D PiezaFrutoRacimo(int tipo)
{
    if (tipo == FRUTA_RACIMO_TOXICA) return MODELO_RACIMO_FRUTO_TOXICO;
    if (tipo == FRUTA_RACIMO_DORADA) return MODELO_RACIMO_FRUTO_DORADO;
    return MODELO_RACIMO_FRUTO_NORMAL;
}


static Color ColorFrutoRacimo(int tipo)
{
    if (tipo == FRUTA_RACIMO_TOXICA) return COLOR_TOXICO_RACIMO;
    if (tipo == FRUTA_RACIMO_DORADA) return COLOR_DORADO_RACIMO;
    return COLOR_FRUTO_RACIMO;
}


// Los frutos brotan en ola (de arriba hacia abajo) durante la cuenta y
// cuando el racimo rebrota; con un pequeno rebote al final.
static float AparicionFrutoRacimo(const MinijuegoRacimoToxico& m, int indice)
{
    if (ObtenerFactorMovimientoCamaraMinijuegos() <= 0.0f)
    {
        return 1.0f;
    }

    float desdeArriba = (float)(TOTAL_FRUTAS_RACIMO - 1 - indice);
    float u = 1.0f;

    if (m.fase == FASE_RACIMO_PREPARACION && m.resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float transcurrido = DURACION_PREPARACION_RACIMO - m.tiempoPreparacion;
        u = (transcurrido - 0.25f - desdeArriba * 0.05f) / 0.5f;
    }
    else if (m.tiempoRebroteVisual >= 0.0f)
    {
        u = (m.tiempoRebroteVisual - desdeArriba * 0.035f) / 0.45f;
    }

    if (u >= 1.0f) return 1.0f;
    if (u <= 0.0f) return 0.0f;
    return 1.0f - (1.0f - u) * (1.0f - u) * std::cos(u * 9.0f);
}


// Fruto jugable: GLB del paquete en su posicion exacta; la escala solo
// anima la aparicion. Sin el GLB, una esfera del color del tipo.
static void DibujarFrutoRacimo(Vector3 p, int tipo, float aparicion)
{
    float s = ESCALA_FRUTO_RACIMO * (aparicion < 0.02f ? 0.02f : aparicion);

    if (!DibujarModeloRacimoToxicoRetro3D(PiezaFrutoRacimo(tipo), p, 0.0f, { 0.0f, 1.0f, 0.0f }, { s, s, s }))
    {
        (DrawSphere)(p, 0.2f * s, ColorFrutoRacimo(tipo));
    }
}


static void DibujarTalloRacimo(const MinijuegoRacimoToxico& m, float t, bool decoracion)
{
    // Tallo principal, del nudo hasta debajo del primer fruto. El tramo ya
    // cosechado queda pelado, con los cabitos secos.
    float yBajo = PosicionFrutaRacimo(0, t).y - 0.45f;
    const int TRAMOS = 8;
    Vector3 previo = NUDO_RACIMO;

    for (int k = 1; k <= TRAMOS; k++)
    {
        float f = (float)k / (float)TRAMOS;
        Vector3 p = { std::sin(f * 4.0f + 0.6f) * 0.05f, NUDO_RACIMO.y + (yBajo - NUDO_RACIMO.y) * f, 0.0f };
        float r0 = 0.15f - 0.07f * ((float)(k - 1) / (float)TRAMOS);
        float r1 = 0.15f - 0.07f * f;
        PrismaRacimo(previo, p, r0, r1, 6, k % 2 ? COLOR_TALLO_RACIMO : TonoRacimo(COLOR_TALLO_RACIMO, 0.9f));
        previo = p;
    }

    // Pedicelos: uno por fruto; los cosechados quedan como cabito seco.
    for (int i = 0; i < TOTAL_FRUTAS_RACIMO; i++)
    {
        Vector3 p = PosicionFrutaRacimo(i, t);
        Vector3 nace = { 0.0f, p.y + 0.24f, 0.0f };

        if (i < m.frente)
        {
            PrismaRacimo(nace, { p.x * 0.35f, p.y + 0.14f, 0.0f }, 0.035f, 0.02f, 4, Color{ 112, 92, 58, 255 });
            continue;
        }

        PrismaRacimo(nace, { p.x * 0.8f, p.y + 0.16f, 0.0f }, 0.04f, 0.03f, 4, COLOR_TALLO_RACIMO);
    }

    // Hojas detras del tallo (nunca delante de los frutos) y zarcillos.
    for (int i = 1; i < TOTAL_FRUTAS_RACIMO; i += 3)
    {
        Vector3 p = PosicionFrutaRacimo(i, t);
        float lado = (i / 3) % 2 == 0 ? -1.0f : 1.0f;
        float vaiven = std::sin(t * 1.3f + (float)i) * 0.05f;
        Vector3 base = { 0.0f, p.y + 0.3f, -0.08f };
        Vector3 punta = { lado * (0.95f + 0.2f * Ruido01Racimo(i, 171)), p.y + 0.55f + vaiven, -0.45f };
        Color c = i % 2 ? COLOR_HOJA_CLARA_RACIMO : TonoRacimo(COLOR_HOJA_CLARA_RACIMO, 0.8f);
        HojaRacimo(base, punta, { 0.0f, 0.13f, 0.1f }, c);
    }

    if (!decoracion)
    {
        return;
    }

    // Hojas grandes en el nudo, como una corona sobre el racimo.
    for (int k = 0; k < 6; k++)
    {
        float a = (float)k * 1.047f + 0.4f;
        Vector3 punta = { std::cos(a) * 1.35f, NUDO_RACIMO.y - 0.55f + std::sin(t * 0.9f + (float)k) * 0.06f, std::sin(a) * 0.9f - 0.2f };
        HojaRacimo({ 0.0f, NUDO_RACIMO.y - 0.1f, 0.0f }, punta, { 0.0f, 0.16f, 0.0f }, k % 2 ? COLOR_HOJA_RACIMO : COLOR_HOJA_CLARA_RACIMO);
    }
}


// Gotas que se desprenden de las bayas toxicas (solidas: se ven en BAJA).
static void DibujarGoteoToxicoRacimo(const MinijuegoRacimoToxico& m, float t)
{
    for (int i = m.frente; i < TOTAL_FRUTAS_RACIMO; i++)
    {
        if (m.tipoFruta[i] != FRUTA_RACIMO_TOXICA)
        {
            continue;
        }

        Vector3 p = PosicionFrutaRacimo(i, t);

        for (int g = 0; g < 2; g++)
        {
            float fase = std::fmod(t * 0.7f + (float)i * 0.37f + (float)g * 0.5f, 1.0f);
            if (fase > 0.85f) continue;
            float caida = fase < 0.3f ? 0.0f : (fase - 0.3f) * (fase - 0.3f) * 9.0f;
            float s = fase < 0.3f ? 0.03f + 0.17f * fase : 0.08f;
            Vector3 gota = { p.x + (g ? 0.06f : -0.05f), p.y - 0.32f - caida, p.z + 0.05f };
            CuboCrudoRacimo(gota, { s, s * 1.4f, s }, Color{ 170, 255, 110, 255 });
        }
    }
}


static void DibujarRacimoRacimo(const MinijuegoRacimoToxico& m, float t, bool decoracion)
{
    DibujarTalloRacimo(m, t, decoracion);

    for (int i = m.frente; i < TOTAL_FRUTAS_RACIMO; i++)
    {
        DibujarFrutoRacimo(PosicionFrutaRacimo(i, t), m.tipoFruta[i], AparicionFrutoRacimo(m, i));
    }

    DibujarGoteoToxicoRacimo(m, t);
}


// Jaula que gira alrededor de los frutos elegidos, del color de quien
// juega (una por fruto: dice cuantos se van a tomar).
static void DibujarSeleccionRacimo(const MinijuegoRacimoToxico& m, const Participante participantes[], float t)
{
    if (m.fase != FASE_RACIMO_TURNO || m.turno < 0)
    {
        return;
    }

    Color color = MezclaRacimo(participantes[m.turno].color, WHITE, 0.25f);

    for (int k = 0; k < m.seleccion && m.frente + k < TOTAL_FRUTAS_RACIMO; k++)
    {
        Vector3 p = PosicionFrutaRacimo(m.frente + k, t);
        float pulso = 1.0f + 0.06f * std::sin(t * 7.0f + (float)k);

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(t * 70.0f + (float)k * 30.0f, 0.0f, 1.0f, 0.0f);
        DrawSphereWires({ 0.0f, 0.0f, 0.0f }, 0.5f * pulso, 5, 10, color);
        rlPopMatrix();
    }
}


//--------------------------------------------------
// Balsas, faroles de vida, banderines y jugadores
//--------------------------------------------------

static void FarolVidaRacimo(Vector3 c, bool encendido)
{
    Color vidrio = encendido ? Color{ 255, 222, 130, 255 } : Color{ 46, 54, 52, 255 };
    CuboCrudoRacimo(c, { 0.24f, 0.32f, 0.24f }, vidrio);
    CuboCrudoRacimo({ c.x, c.y + 0.2f, c.z }, { 0.34f, 0.07f, 0.34f }, COLOR_HIERRO_RACIMO);
    CuboCrudoRacimo({ c.x, c.y - 0.19f, c.z }, { 0.3f, 0.06f, 0.3f }, COLOR_HIERRO_RACIMO);
    PrismaRacimo({ c.x, c.y + 0.23f, c.z }, { c.x, c.y + 0.4f, c.z }, 0.17f, 0.0f, 4, COLOR_HIERRO_RACIMO, 0.0f, 0.785f);

    for (int k = 0; k < 4; k++)
    {
        float sx = k % 2 ? 1.0f : -1.0f;
        float sz = k < 2 ? 1.0f : -1.0f;
        CuboCrudoRacimo({ c.x + sx * 0.13f, c.y, c.z + sz * 0.13f }, { 0.035f, 0.34f, 0.035f }, COLOR_HIERRO_RACIMO);
    }

    if (encendido)
    {
        CuboCrudoRacimo(c, { 0.1f, 0.16f, 0.1f }, Color{ 255, 250, 220, 255 });
    }
}


static void BanderinBalsaRacimo(Vector3 base, Color color, float t, int semilla)
{
    Vector3 tope = { base.x, base.y + 2.1f, base.z };
    PrismaRacimo(base, tope, 0.05f, 0.04f, 5, COLOR_MADERA_OSCURA_RACIMO);
    RocaRacimo({ tope.x, tope.y + 0.06f, tope.z }, { 0.08f, 0.08f, 0.08f }, semilla, Color{ 214, 180, 96, 255 }, 0.05f);

    // Tela triangular en cuatro tramos que ondean.
    const int TRAMOS = 4;
    rlBegin(RL_QUADS);

    for (int k = 0; k < TRAMOS; k++)
    {
        float f0 = (float)k / (float)TRAMOS;
        float f1 = (float)(k + 1) / (float)TRAMOS;
        float o0 = std::sin(t * 4.5f - f0 * 3.5f + (float)semilla) * 0.12f * f0;
        float o1 = std::sin(t * 4.5f - f1 * 3.5f + (float)semilla) * 0.12f * f1;
        float x0 = base.x + 0.04f + f0 * 0.95f;
        float x1 = base.x + 0.04f + f1 * 0.95f;
        float arriba0 = tope.y - 0.04f - f0 * 0.2f, abajo0 = tope.y - 0.58f + f0 * 0.2f;
        float arriba1 = tope.y - 0.04f - f1 * 0.2f, abajo1 = tope.y - 0.58f + f1 * 0.2f;
        Color c = TonoRacimo(color, k % 2 ? 0.86f : 1.0f);

        QuadRacimo({ x0, abajo0, base.z + o0 }, { x1, abajo1, base.z + o1 }, { x1, arriba1, base.z + o1 }, { x0, arriba0, base.z + o0 }, c);
    }

    rlEnd();
}


static void DibujarBalsaRacimo(
    const MinijuegoRacimoToxico& m,
    int i,
    const Participante& participante,
    float t,
    bool decoracion
)
{
    float x = m.posicionBalsaX[i];
    float y = AlturaBalsaRacimo(m, i, t);
    float temblor = TemblorBalsaRacimo(m, i, t);

    // La madera del GLB no se tine: el estado se cuenta con el hundimiento,
    // los faroles de vida y el banderin.
    if (!DibujarModeloRacimoToxicoRetro3D(MODELO_RACIMO_BALSA, { x + temblor, y, Z_BALSAS_RACIMO }))
    {
        for (int k = 0; k < 4; k++)
        {
            Color madera = k % 2 == 0 ? COLOR_MADERA_RACIMO : TonoRacimo(COLOR_MADERA_RACIMO, 0.85f);
            (DrawCube)({ x + temblor, y, Z_BALSAS_RACIMO - 0.8f + 0.55f * (float)k }, 3.2f, 0.24f, 0.5f, madera);
        }
    }

    // Faroles de vida en las esquinas delanteras: uno por vida.
    int vidas = VidasVisiblesRacimo(m, i);

    for (int v = 0; v < VIDAS_RACIMO; v++)
    {
        Vector3 farol = LinternaVidaRacimo(m, i, v, t);
        PrismaRacimo({ farol.x, y + 0.1f, farol.z }, { farol.x, farol.y - 0.2f, farol.z }, 0.05f, 0.045f, 5, COLOR_MADERA_OSCURA_RACIMO);
        FarolVidaRacimo(farol, v < vidas);
    }

    // Banderin del color del jugador en la esquina trasera.
    BanderinBalsaRacimo({ x + temblor - 1.38f, y + 0.1f, Z_BALSAS_RACIMO - 0.72f }, participante.color, t, 13 + i * 7);

    if (decoracion)
    {
        // Rollo de soga y remo sobre la balsa.
        for (int v = 0; v < 2; v++)
        {
            PrismaRacimo({ x + temblor + 1.05f, y + 0.13f + 0.05f * (float)v, Z_BALSAS_RACIMO - 0.5f },
                { x + temblor + 1.05f, y + 0.17f + 0.05f * (float)v, Z_BALSAS_RACIMO - 0.5f }, 0.22f, 0.22f, 8, Color{ 168, 142, 98, 255 });
        }
        PrismaRacimo({ x + temblor - 0.9f, y + 0.16f, Z_BALSAS_RACIMO - 0.62f }, { x + temblor + 0.5f, y + 0.18f, Z_BALSAS_RACIMO - 0.84f }, 0.035f, 0.035f, 4, COLOR_MADERA_RACIMO);
        CuboGiradoRacimo({ x + temblor + 0.62f, y + 0.18f, Z_BALSAS_RACIMO - 0.86f }, { 0.36f, 0.04f, 0.16f }, 9.0f, COLOR_MADERA_RACIMO);
    }

    // Aro del turno (GLB con pulso en su malla de bombillas).
    if (m.fase == FASE_RACIMO_TURNO && i == m.turno)
    {
        float pulso = 0.5f + 0.3f * std::sin(t * 6.0f);

        if (!DibujarModeloRacimoToxicoRetro3D(MODELO_RACIMO_ARO, { x, 0.02f, Z_BALSAS_RACIMO }, 0.0f, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, Fade(WHITE, pulso)))
        {
            (DrawCylinder)({ x, 0.02f, Z_BALSAS_RACIMO }, 2.3f, 2.3f, 0.06f, 20, Fade(participante.color, pulso));
        }
    }
}


// Posicion del jugador dibujado: sobre su balsa; al quedar eliminado
// salta un poco y se hunde en el agua.
static bool PosicionJugadorVisualRacimo(
    const MinijuegoRacimoToxico& m,
    const JugadorPrueba& jugador,
    int i,
    float t,
    Vector3& posicion
)
{
    const EstadoJugadorRacimoToxico& estado = m.estadosJugadores[i];
    float caida = m.caidaJugador[i];

    if (!estado.vivo && (caida <= 0.0f || caida >= DURACION_CAIDA_RACIMO))
    {
        return false;
    }

    float bamboleo = std::sin(t * 2.0f + m.posicionBalsaX[i]) * 0.04f;
    posicion = { m.posicionBalsaX[i] + TemblorBalsaRacimo(m, i, t), 0.2f + jugador.tamano.y * 0.5f + bamboleo, Z_BALSAS_RACIMO };

    if (!estado.vivo)
    {
        float u = caida / DURACION_CAIDA_RACIMO;
        posicion.y += std::sin(Limitar01Racimo(u * 2.2f) * 3.1416f) * 0.6f - u * u * (jugador.tamano.y + 1.2f);
        posicion.z += u * 0.5f;
    }

    return true;
}


static void DibujarJugadoresRacimo(
    const MinijuegoRacimoToxico& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    float t,
    bool mostrarDebug
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 posicion{};
        if (!PosicionJugadorVisualRacimo(m, jugadores[i], i, t, posicion))
        {
            continue;
        }

        JugadorPrueba visual = jugadores[i];
        visual.posicion = posicion;
        visual.direccionMirada = { 0.0f, 0.0f, -1.0f };

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }
    }
}


static Vector3 PosicionVueloRacimo(const MinijuegoRacimoToxico& m, const VueloFrutaRacimo& vuelo, float progreso)
{
    Vector3 destino = DestinoVueloRacimo(m, vuelo.jugador);
    return {
        vuelo.origen.x + (destino.x - vuelo.origen.x) * progreso,
        vuelo.origen.y + (destino.y - vuelo.origen.y) * progreso + std::sin(progreso * PI_RACIMO) * 1.2f,
        vuelo.origen.z + (destino.z - vuelo.origen.z) * progreso
    };
}


static void DibujarVuelosRacimo(const MinijuegoRacimoToxico& m)
{
    for (int k = 0; k < MAX_VUELOS_RACIMO; k++)
    {
        const VueloFrutaRacimo& vuelo = m.vuelos[k];

        if (!vuelo.activo || vuelo.jugador < 0)
        {
            continue;
        }

        DibujarFrutoRacimo(PosicionVueloRacimo(m, vuelo, vuelo.progreso), vuelo.tipo, 1.0f);
    }
}


//--------------------------------------------------
// Luciernagas y particulas
//--------------------------------------------------

static int CantidadLuciernagasRacimo()
{
    return 6 + (int)(30.0f * FactorCalidadGrafica(CalidadParticulas()));
}


// Vuelan lejos del racimo para no confundirse con sus frutos.
static Vector3 PosicionLuciernagaRacimo(int k, float t)
{
    float x = (Ruido01Racimo(k, 151) - 0.5f) * 38.0f;
    if (std::fabs(x) < 3.2f) x += x < 0.0f ? -3.2f : 3.2f;
    float z = -17.0f + 21.0f * Ruido01Racimo(k, 152);
    float y = 0.7f + 5.5f * Ruido01Racimo(k, 153);
    float fase = Ruido01Racimo(k, 154) * 6.2832f;

    return {
        x + std::sin(t * 0.45f + fase) * 1.0f,
        y + std::sin(t * 0.9f + fase * 1.3f) * 0.45f,
        z + std::cos(t * 0.35f + fase) * 0.8f
    };
}


static float BrilloLuciernagaRacimo(int k, float t)
{
    float b = 0.5f + 0.5f * std::sin(t * (1.4f + 1.8f * Ruido01Racimo(k, 155)) + (float)k * 1.7f);
    return b * b;
}


static void DibujarLuciernagasSolidasRacimo(float t)
{
    int cantidad = CantidadLuciernagasRacimo();

    for (int k = 0; k < cantidad; k++)
    {
        float b = BrilloLuciernagaRacimo(k, t);
        CuboCrudoRacimo(PosicionLuciernagaRacimo(k, t), { 0.07f, 0.07f, 0.07f }, MezclaRacimo(Color{ 70, 90, 50, 255 }, COLOR_LUCIERNAGA_RACIMO, b));
    }
}


static void DibujarParticulasSolidasRacimo(const MinijuegoRacimoToxico& m)
{
    for (int i = 0; i < MAX_PARTICULAS_RACIMO; i++)
    {
        const ParticulaRacimo& p = m.particulas[i];

        if (!p.activa || p.tipo == PARTICULA_RACIMO_HUMO)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        float s = p.tamano;

        switch (p.tipo)
        {
        case PARTICULA_RACIMO_CHISPA:
            s *= 0.4f + 0.6f * vida;
            CuboCrudoRacimo(p.posicion, { s, s, s }, p.color);
            break;
        case PARTICULA_RACIMO_GOTA:
            CuboCrudoRacimo(p.posicion, { s, s * 1.5f, s }, p.color);
            break;
        case PARTICULA_RACIMO_CONFETI:
        case PARTICULA_RACIMO_HOJA:
            rlPushMatrix();
            rlTranslatef(p.posicion.x, p.posicion.y, p.posicion.z);
            rlRotatef(p.giro, 0.3f, 1.0f, 0.4f);
            if (p.tipo == PARTICULA_RACIMO_CONFETI)
                CuboCrudoRacimo({ 0.0f, 0.0f, 0.0f }, { s, s * 0.6f, 0.02f }, p.color);
            else
                CuboCrudoRacimo({ 0.0f, 0.0f, 0.0f }, { s, 0.025f, s * 0.5f }, p.color);
            rlPopMatrix();
            break;
        default:
            break;
        }
    }
}


//--------------------------------------------------
// Translucidos: sombras, ondas, niebla verde y humo
//--------------------------------------------------

static void DibujarNieblaRacimo(float t, bool efectos, bool decoracion)
{
    // Bancos de niebla rasante que derivan sobre el agua (mas bajos que
    // el primer fruto: nunca lo tapan desde la camara).
    int bancos = decoracion ? 16 : 7;

    for (int k = 0; k < bancos; k++)
    {
        float deriva = efectos ? t * (0.25f + 0.25f * Ruido01Racimo(k, 181)) : 0.0f;
        float x = std::fmod(Ruido01Racimo(k, 182) * 70.0f + deriva, 70.0f) - 35.0f;
        float z = -26.0f + 30.0f * Ruido01Racimo(k, 183);
        float y = 0.25f + 0.55f * Ruido01Racimo(k, 184);
        float radio = 4.0f + 4.5f * Ruido01Racimo(k, 185);
        ResplandorSueloRacimo({ x, y, z }, radio, AlfaRacimo(Color{ 128, 196, 150, 255 }, 0.15f), 1.9f);
    }

    // Cortinas de niebla entre los cipreses del fondo.
    int cortinas = decoracion ? 9 : 4;

    for (int k = 0; k < cortinas; k++)
    {
        float deriva = efectos ? std::sin(t * 0.12f + (float)k) * 1.5f : 0.0f;
        float x = -36.0f + 72.0f * ((float)k + 0.5f) / (float)cortinas + deriva;
        float z = -15.0f - 16.0f * Ruido01Racimo(k, 186);
        ResplandorFrenteRacimo({ x, 2.2f, z }, 6.5f + 3.0f * Ruido01Racimo(k, 187), AlfaRacimo(Color{ 104, 160, 124, 255 }, 0.2f), 2.2f);
    }
}


static void DibujarTranslucidosRacimo(
    const MinijuegoRacimoToxico& m,
    float t,
    bool efectos,
    bool decoracion
)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        float x = m.posicionBalsaX[i] + TemblorBalsaRacimo(m, i, t);
        float y = AlturaBalsaRacimo(m, i, t);

        if (m.estadosJugadores[i].vivo)
        {
            // Sombra del jugador sobre los tablones.
            SectorRacimo({ x, y + 0.13f, Z_BALSAS_RACIMO }, 0.45f, 0.0f, 6.2832f, Color{ 10, 16, 14, 110 }, 14);

            // Ondas que salen de la balsa.
            if (efectos)
            {
                for (int r = 0; r < 2; r++)
                {
                    float u = std::fmod(t * 0.4f + (float)r * 0.5f + (float)i * 0.23f, 1.0f);
                    float radio = 2.0f + 1.6f * u;
                    AnilloRacimo({ m.posicionBalsaX[i], 0.025f, Z_BALSAS_RACIMO }, radio, radio + 0.07f, AlfaRacimo(Color{ 170, 220, 200, 255 }, 0.24f * (1.0f - u)), 32);
                }
            }
        }
        else
        {
            // Burbujas donde se hundio.
            for (int r = 0; r < 3; r++)
            {
                float u = std::fmod(t * 0.7f + (float)r * 0.33f, 1.0f);
                float ox = (Ruido01Racimo(r + i * 5, 191) - 0.5f) * 1.6f;
                float oz = (Ruido01Racimo(r + i * 5, 192) - 0.5f) * 0.8f;
                float radio = 0.1f + 0.6f * u;
                AnilloRacimo({ m.posicionBalsaX[i] + ox, 0.03f, Z_BALSAS_RACIMO + 0.6f + oz }, radio, radio + 0.05f, AlfaRacimo(Color{ 190, 230, 210, 255 }, 0.4f * (1.0f - u)), 16);
            }
        }
    }

    DibujarNieblaRacimo(t, efectos, decoracion);

    // Humo toxico de las bayas que llegaron a destino.
    for (int i = 0; i < MAX_PARTICULAS_RACIMO; i++)
    {
        const ParticulaRacimo& p = m.particulas[i];
        if (!p.activa || p.tipo != PARTICULA_RACIMO_HUMO) continue;

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteRacimo(p.posicion, p.tamano, AlfaRacimo(p.color, 0.6f * vida));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Luces aditivas
//--------------------------------------------------

static void HaloRacimo(Vector3 p, float radio, Color exterior, Color interior)
{
    ResplandorFrenteRacimo(p, radio, exterior);
    ResplandorFrenteRacimo(p, radio * 0.42f, interior);
}


static void DibujarLucesRacimo(
    const MinijuegoRacimoToxico& m,
    const Participante participantes[],
    float t,
    bool decoracion
)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Faroles de las estacas: halo, cono y charco tibio.
    for (int k = 0; k < 2; k++)
    {
        Vector3 l = FarolEstacaRacimo(k, t);
        float parpadeo = 0.9f + 0.06f * std::sin(t * 11.0f + (float)k * 2.0f) + 0.04f * std::sin(t * 29.0f + (float)k);
        HaloRacimo(l, 1.5f * parpadeo, Color{ 255, 176, 90, 80 }, Color{ 255, 232, 170, 170 });
        ConoLuzRacimo(l, { l.x, 0.05f, l.z }, 1.8f, Color{ 255, 190, 110, 20 });
        CharcoLuzRacimo({ l.x, 0.0f, l.z }, 3.2f * parpadeo, COLOR_LUZ_RACIMO, 1.0f);
        ReflejoAguaRacimo({ l.x, 0.04f, l.z + 2.4f }, 0.5f, Color{ 255, 200, 120, 60 }, 4.0f);
    }

    // Cabanas: farol del porche y ventanas encendidas.
    if (decoracion)
    {
        const Vector3 cabanas[3] = { CABANA_PRINCIPAL_RACIMO, CABANA_IZQUIERDA_RACIMO, CABANA_LEJANA_RACIMO };
        const float escalas[3] = { 1.0f, 1.15f, 0.95f };

        for (int k = 0; k < 3; k++)
        {
            float s = escalas[k];
            Vector3 b = cabanas[k];
            Vector3 l = FarolCabanaRacimo(b, s);
            float parpadeo = 0.9f + 0.1f * std::sin(t * 9.0f + (float)k * 3.0f);
            HaloRacimo(l, 1.4f * s * parpadeo, Color{ 255, 176, 90, 70 }, Color{ 255, 232, 170, 150 });
            CharcoLuzRacimo({ l.x, 0.0f, l.z + 0.8f }, 3.0f * s, COLOR_LUZ_RACIMO, 0.8f);

            float piso = 2.6f * s;
            Vector3 ventana = { b.x + 0.8f * s, piso + 1.55f * s, b.z + 1.3f * s };
            ResplandorFrenteRacimo(ventana, 1.3f * s, Color{ 255, 190, 100, 50 });
            ReflejoAguaRacimo({ ventana.x, 0.04f, b.z + 3.4f * s }, 0.7f * s, Color{ 255, 190, 110, 40 }, 3.0f);
        }

        // Hongos que brillan entre las raices del arbol.
        for (int k = 0; k < 7; k++)
        {
            Vector3 h = PosicionHongoArbolRacimo(k);
            float pulso = 0.75f + 0.25f * std::sin(t * 1.6f + (float)k * 1.9f);
            ResplandorFrenteRacimo({ h.x, h.y + 0.3f, h.z }, 0.9f * pulso, AlfaRacimo(COLOR_HONGO_RACIMO, 0.22f));
            ResplandorSueloRacimo({ h.x, 0.04f, h.z + 0.4f }, 1.3f * pulso, AlfaRacimo(COLOR_HONGO_RACIMO, 0.1f));
        }
    }

    // Destellos de la luna en el agua.
    for (int k = 0; k < 14; k++)
    {
        float z = -34.0f + 3.0f * (float)k + std::sin(t * 0.3f + (float)k) * 0.6f;
        float brillo = 0.5f + 0.5f * std::sin(t * 2.2f + (float)k * 2.7f);
        ReflejoAguaRacimo({ ReflejoLunaXRacimo(z) + std::sin(t * 0.8f + (float)k * 1.3f) * 0.9f, 0.05f, z }, 0.35f + 0.012f * (34.0f + z), AlfaRacimo(Color{ 190, 230, 210, 255 }, 0.18f * brillo), 3.5f);
    }

    // El racimo: veneno violeta, oro y una luz verdosa sobre el agua.
    CharcoLuzRacimo({ 0.0f, 0.0f, 0.6f }, 3.4f, Color{ 160, 230, 120, 255 }, 0.7f);

    for (int i = m.frente; i < TOTAL_FRUTAS_RACIMO; i++)
    {
        int tipo = m.tipoFruta[i];
        if (tipo == FRUTA_RACIMO_NORMAL) continue;

        Vector3 p = PosicionFrutaRacimo(i, t);
        float aparicion = AparicionFrutoRacimo(m, i);

        if (tipo == FRUTA_RACIMO_TOXICA)
        {
            float pulso = 0.8f + 0.2f * std::sin(t * 5.0f + (float)i);
            HaloRacimo(p, 1.05f * pulso * aparicion, Color{ 200, 60, 255, 90 }, Color{ 255, 140, 255, 120 });
            ResplandorFrenteRacimo({ p.x, p.y - 0.35f, p.z + 0.1f }, 0.5f * aparicion, Color{ 150, 255, 90, 60 });
        }
        else
        {
            float giro = t * 40.0f;
            HaloRacimo(p, 1.15f * aparicion, Color{ 255, 200, 60, 90 }, Color{ 255, 246, 190, 150 });

            // Rayos que giran.
            for (int r = 0; r < 4; r++)
            {
                float a = (giro + (float)r * 45.0f) * DEG2RAD;
                Vector3 dir = { std::cos(a) * FRENTE_U_RACIMO.x + std::sin(a) * FRENTE_V_RACIMO.x, std::sin(a) * FRENTE_V_RACIMO.y, std::sin(a) * FRENTE_V_RACIMO.z };
                Vector3 lado = { -std::sin(a) * 0.05f, std::cos(a) * 0.05f * FRENTE_V_RACIMO.y, std::cos(a) * 0.05f * FRENTE_V_RACIMO.z };
                float largo = 0.95f * aparicion;
                rlBegin(RL_QUADS);
                VerticeRacimo(SumaRacimo(p, EscalaRacimo(dir, -largo)), Color{ 255, 220, 100, 0 });
                VerticeRacimo(SumaRacimo(p, lado), Color{ 255, 230, 140, 110 });
                VerticeRacimo(SumaRacimo(p, EscalaRacimo(dir, largo)), Color{ 255, 220, 100, 0 });
                VerticeRacimo(RestaRacimo(p, lado), Color{ 255, 230, 140, 110 });
                rlEnd();
            }
        }
    }

    // Turno: frutos elegidos y foco del color de quien juega.
    if (m.fase == FASE_RACIMO_TURNO && m.turno >= 0)
    {
        Color color = participantes[m.turno].color;
        float pulso = 0.85f + 0.15f * std::sin(t * 7.0f);

        for (int k = 0; k < m.seleccion && m.frente + k < TOTAL_FRUTAS_RACIMO; k++)
        {
            Vector3 p = PosicionFrutaRacimo(m.frente + k, t);
            ResplandorFrenteRacimo(p, 0.95f * pulso, AlfaRacimo(color, 0.42f));
        }

        float x = m.posicionBalsaX[m.turno];
        ConoLuzRacimo({ x, 9.0f, Z_BALSAS_RACIMO - 0.5f }, { x, 0.1f, Z_BALSAS_RACIMO }, 2.4f, AlfaRacimo(color, 0.07f));
        CharcoLuzRacimo({ x, 0.0f, Z_BALSAS_RACIMO }, 3.0f, color, 1.2f * pulso);
    }

    // Faroles de vida de cada balsa.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        int vidas = VidasVisiblesRacimo(m, i);
        for (int v = 0; v < vidas; v++)
        {
            Vector3 l = LinternaVidaRacimo(m, i, v, t);
            float parpadeo = 0.9f + 0.1f * std::sin(t * 13.0f + (float)(i * 2 + v));
            HaloRacimo(l, 0.95f * parpadeo, Color{ 255, 190, 100, 80 }, Color{ 255, 240, 190, 150 });
            ReflejoAguaRacimo({ l.x, 0.04f, l.z + 0.9f }, 0.3f, Color{ 255, 200, 120, 60 }, 3.0f);
        }

        if (m.destelloJugador[i] > 0.0f)
        {
            Vector3 c = { m.posicionBalsaX[i], 1.3f, Z_BALSAS_RACIMO };
            float d = m.destelloJugador[i];
            HaloRacimo(c, 1.8f * d + 0.4f, AlfaRacimo(participantes[i].color, 0.4f * d), AlfaRacimo(WHITE, 0.35f * d));
        }
    }

    // Estela de los frutos en vuelo.
    for (int k = 0; k < MAX_VUELOS_RACIMO; k++)
    {
        const VueloFrutaRacimo& vuelo = m.vuelos[k];
        if (!vuelo.activo || vuelo.jugador < 0) continue;

        Color c = vuelo.tipo == FRUTA_RACIMO_TOXICA ? Color{ 210, 80, 255, 255 }
            : (vuelo.tipo == FRUTA_RACIMO_DORADA ? COLOR_DORADO_RACIMO : COLOR_FRUTO_RACIMO);

        for (int s = 0; s < 6; s++)
        {
            float pr = vuelo.progreso - 0.035f * (float)s;
            if (pr < 0.0f) break;
            ResplandorFrenteRacimo(PosicionVueloRacimo(m, vuelo, pr), 0.5f - 0.06f * (float)s, AlfaRacimo(c, 0.32f - 0.05f * (float)s));
        }
    }

    // Luciernagas.
    int cantidad = CantidadLuciernagasRacimo();
    for (int k = 0; k < cantidad; k++)
    {
        float b = BrilloLuciernagaRacimo(k, t);
        Vector3 p = PosicionLuciernagaRacimo(k, t);
        HaloRacimo(p, 0.55f, AlfaRacimo(COLOR_LUCIERNAGA_RACIMO, 0.32f * b), AlfaRacimo(Color{ 250, 255, 210, 255 }, 0.7f * b));
    }

    // Chispas y confeti con brillo.
    for (int i = 0; i < MAX_PARTICULAS_RACIMO; i++)
    {
        const ParticulaRacimo& p = m.particulas[i];
        if (!p.activa || p.tipo != PARTICULA_RACIMO_CHISPA) continue;

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteRacimo(p.posicion, p.tamano * 3.0f, AlfaRacimo(p.color, 0.45f * vida));
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Anuncios sobre el mundo
//--------------------------------------------------

struct AnuncioRacimo
{
    char texto[48] = "";
    int token = UI_TEXTO;
    Color color = WHITE;
};


static float AlfaAnuncioRacimo(float u)
{
    if (u < 0.0f || u > 1.0f) return 0.0f;
    if (u < 0.12f) return u / 0.12f;
    if (u > 0.75f) return (1.0f - u) / 0.25f;
    return 1.0f;
}


static void DibujarAnunciosRacimo(const MinijuegoRacimoToxico& m, const Participante participantes[])
{
    AnuncioRacimo lineas[3];
    int cantidad = 0;
    int protagonista = -1;
    float alfa = 0.0f;
    float subida = 0.0f;

    if (m.fase == FASE_RACIMO_ANIMACION && m.ultimoJugador >= 0)
    {
        int j = m.ultimoJugador;
        int numero = participantes[j].numeroJugador;
        float u = 1.0f - m.tiempoPausa / DURACION_PAUSA_RACIMO;
        protagonista = j;
        alfa = AlfaAnuncioRacimo(u);
        subida = u;

        switch (m.ultimoEvento)
        {
        case EVENTO_RACIMO_TOXICA:
            std::snprintf(lineas[0].texto, sizeof(lineas[0].texto), "¡BAYA TÓXICA!");
            lineas[0].token = UI_TEXTO_TITULO;
            lineas[0].color = Color{ 236, 130, 255, 255 };
            std::snprintf(lineas[1].texto, sizeof(lineas[1].texto), "J%d PIERDE UNA VIDA", numero);
            lineas[1].color = UI_TEXTO_MUNDO;
            cantidad = 2;
            break;
        case EVENTO_RACIMO_ELIMINADO:
            std::snprintf(lineas[0].texto, sizeof(lineas[0].texto), "¡J%d SE HUNDE!", numero);
            lineas[0].token = UI_TEXTO_TITULO;
            lineas[0].color = Color{ 255, 120, 110, 255 };
            std::snprintf(lineas[1].texto, sizeof(lineas[1].texto), "SE QUEDÓ SIN VIDAS");
            lineas[1].color = UI_TEXTO_MUNDO;
            cantidad = 2;
            break;
        case EVENTO_RACIMO_DORADA:
            std::snprintf(lineas[0].texto, sizeof(lineas[0].texto), "¡FRUTO DORADO!");
            lineas[0].token = UI_TEXTO_TITULO;
            lineas[0].color = Color{ 255, 220, 100, 255 };
            std::snprintf(lineas[1].texto, sizeof(lineas[1].texto), m.saltarSiguiente ? "EL QUE SIGUE PIERDE EL TURNO" : "¡QUÉ SUERTE!");
            lineas[1].color = UI_TEXTO_MUNDO;
            cantidad = 2;
            break;
        default:
            break;
        }
    }
    else if (m.fase == FASE_RACIMO_TURNO && m.turno >= 0)
    {
        float transcurrido = DURACION_TURNO_RACIMO - m.tiempoTurno;
        protagonista = m.turno;
        alfa = AlfaAnuncioRacimo(transcurrido / 1.5f);
        subida = transcurrido / 1.5f;

        if (transcurrido < 1.5f)
        {
            std::snprintf(lineas[cantidad].texto, sizeof(lineas[cantidad].texto), "¡TURNO DE J%d!", participantes[m.turno].numeroJugador);
            lineas[cantidad].token = UI_TEXTO_GRANDE;
            lineas[cantidad].color = MezclaRacimo(participantes[m.turno].color, WHITE, 0.3f);
            cantidad++;

            if (m.jugadorSaltado >= 0)
            {
                std::snprintf(lineas[cantidad].texto, sizeof(lineas[cantidad].texto), "J%d PIERDE EL TURNO", participantes[m.jugadorSaltado].numeroJugador);
                lineas[cantidad].color = Color{ 255, 220, 100, 255 };
                cantidad++;
            }

            if (m.racimoRepuesto)
            {
                std::snprintf(lineas[cantidad].texto, sizeof(lineas[cantidad].texto), "¡EL RACIMO REBROTÓ!");
                lineas[cantidad].color = Color{ 170, 240, 120, 255 };
                cantidad++;
            }
        }
    }

    if (cantidad == 0 || alfa <= 0.0f || protagonista < 0)
    {
        return;
    }

    // Del lado de la balsa de quien protagoniza, sin tapar el racimo.
    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();
    float lado = m.posicionBalsaX[protagonista] < -0.1f ? -1.0f : 1.0f;
    float x = ancho * (0.5f + lado * 0.27f);
    float y = alto * 0.33f - subida * Px(14.0f);

    for (int k = 0; k < cantidad; k++)
    {
        DibujarTextoMundoContornoUI(lineas[k].texto, x, y, lineas[k].token, lineas[k].color, alfa);
        y += lineas[k].token == UI_TEXTO_TITULO ? Px(62.0f) : (lineas[k].token == UI_TEXTO_GRANDE ? Px(40.0f) : Px(30.0f));
    }
}


// «+N» que sube sobre quien tomo frutos sanos.
static void DibujarSumaSeguraRacimo(const MinijuegoRacimoToxico& m, const JugadorPrueba jugadores[], const Camera3D& vista)
{
    if (m.fase != FASE_RACIMO_ANIMACION || m.ultimoJugador < 0 || m.tomaVisual <= 0)
    {
        return;
    }

    if (m.ultimoEvento != EVENTO_RACIMO_SEGURO)
    {
        return;
    }

    float u = 1.0f - m.tiempoPausa / DURACION_PAUSA_RACIMO;
    float alfa = AlfaAnuncioRacimo((u - 0.35f) / 0.65f);
    if (alfa <= 0.0f) return;

    int j = m.ultimoJugador;
    Vector3 cabeza = { m.posicionBalsaX[j], 0.2f + jugadores[j].tamano.y + 1.6f + u * 0.6f, Z_BALSAS_RACIMO };
    Vector2 s = GetWorldToScreen(cabeza, vista);
    char texto[8];
    std::snprintf(texto, sizeof(texto), "+%d", m.tomaVisual);
    DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, Color{ 190, 250, 120, 255 }, alfa);
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoRacimoToxico::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    // Entrada: durante la cuenta la camara baja desde la copa del arbol.
    Camera3D vista = camara;

    if (fase == FASE_RACIMO_PREPARACION && resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Limitar01Racimo((tiempoPreparacion - 0.5f) / (DURACION_PREPARACION_RACIMO - 0.5f));
        float suave = SuaveRacimo(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 4.5f * suave;
        vista.position.z += 6.0f * suave;
        vista.target.y += 2.5f * suave;
    }

    // El bucle del juego no limpia la pantalla: limpiar color y
    // profundidad antes del cielo.
    ClearBackground(Color{ 6, 10, 24, 255 });
    DibujarCieloRacimo(t, efectos);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte y media distancia.
    DibujarHorizonteRacimo(decoracion);
    DibujarAguaRacimo(t, efectos);
    DibujarArbolGiganteRacimo(t, decoracion, efectos);

    if (decoracion)
    {
        DibujarMediaDistanciaRacimo(t, paso, efectos);
        DibujarUtileriaRacimo(t, paso, efectos);
    }
    else
    {
        // BAJA: la arena conserva sus bordes (las estacas con farol que
        // enmarcan las balsas) y las dos cabanas cercanas, sin detalle.
        CabanaRacimo(CABANA_PRINCIPAL_RACIMO, 1.0f, false, 401);
        CabanaRacimo(CABANA_IZQUIERDA_RACIMO, 1.15f, false, 411);
        EstacaFarolRacimo(0, t);
        EstacaFarolRacimo(1, t);
    }

    // Piezas jugables.
    DibujarRacimoRacimo(*this, t, decoracion);
    DibujarSeleccionRacimo(*this, participantes, t);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarBalsaRacimo(*this, i, participantes[i], t, decoracion);
        }
    }

    DibujarJugadoresRacimo(*this, jugadores, participantes, t, mostrarDebug);
    DibujarVuelosRacimo(*this);
    DibujarLuciernagasSolidasRacimo(t);
    DibujarParticulasSolidasRacimo(*this);

    DibujarTranslucidosRacimo(*this, t, efectos, decoracion);

    if (efectos)
    {
        DibujarLucesRacimo(*this, participantes, t, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Cartelitos J1..J4 sobre cada jugador; el de turno dice cuanto toma.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || !estadosJugadores[i].vivo)
        {
            continue;
        }

        Vector3 cabeza{};
        if (!PosicionJugadorVisualRacimo(*this, jugadores[i], i, t, cabeza))
        {
            continue;
        }

        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.5f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        bool deTurno = fase == FASE_RACIMO_TURNO && i == turno;
        char rotulo[24];

        if (deTurno)
            std::snprintf(rotulo, sizeof(rotulo), "J%d: TOMA %d", participantes[i].numeroJugador, seleccion);
        else
            std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    DibujarSumaSeguraRacimo(*this, jugadores, vista);
    DibujarAnunciosRacimo(*this, participantes);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "RACIMO TÓXICO",
        "TOMÁ 1 O 2 FRUTOS POR TURNO Y NO TE COMAS LAS BAYAS VIOLETAS",
        resultado
    );

    hud.fase = fase == FASE_RACIMO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_RACIMO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_RACIMO;
    hud.finPorTiempo = tiempoRestante <= 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];

        if (!h.visible)
        {
            continue;
        }

        const EstadoJugadorRacimoToxico& estado = estadosJugadores[i];
        int vidas = VidasVisiblesRacimo(*this, i);

        std::snprintf(h.valor, sizeof(h.valor), vidas == 1 ? "%d VIDA" : "%d VIDAS", vidas);
        h.valorNumerico = vidas;
        h.eliminado = !estado.vivo && caidaJugador[i] >= DURACION_CAIDA_RACIMO * 0.5f;
        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d %s", estado.frutasSeguras, estado.frutasSeguras == 1 ? "FRUTO" : "FRUTOS");

        if (fase == FASE_RACIMO_TURNO && i == turno)
        {
            std::snprintf(h.estado, sizeof(h.estado), seleccion == 1 ? "¡TOMA 1 FRUTO!" : "¡TOMA 2 FRUTOS!");
            h.progreso = Limitar01Racimo(tiempoTurno / DURACION_TURNO_RACIMO);
        }
        else if (fase == FASE_RACIMO_TURNO && i == jugadorSaltado)
        {
            std::snprintf(h.estado, sizeof(h.estado), "PIERDE EL TURNO");
        }

        if (participantes[i].activo && !EsControlBotRacimo(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "IZQ: 1 FRUTO  DER: 2\nTOMAR: %s",
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoRacimoToxico::ObtenerResultado() const
{
    return resultado;
}
