#include "Minigames/MinijuegoBateoMeteorico.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
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
#include <initializer_list>


//==================================================
// CONSTANTES (REGLAS)
//==================================================

static const float DURACION_PREPARACION_BATEO = 3.0f;
static const float DURACION_AVISO_BATEO = 1.1f;
static const float DURACION_PAUSA_BATEO = 0.8f;
// Tope duro de seguridad: la partida nunca pasa de este tiempo.
static const float TIEMPO_LIMITE_BATEO = 75.0f;
static const float TOPE_ETAPA_VUELO = 7.0f;

static const float VENTANA_GOLPE = 0.28f;
static const float VENTANA_PERFECTO = 0.05f;
static const float DURACION_SWING = 0.26f;

static const int PUNTOS_CASTIGO_ROJO = 50;
static const float DISTANCIA_CENTRO_ANILLOS = 20.5f;

// Geometria logica del carril (el jugador esta en +Z, el canon en -Z).
static const float Z_CANON = -12.0f;
static const float Y_CANON = 5.0f;
static const float Z_PUNTO_DULCE = -0.7f;
static const float Y_PUNTO_DULCE = 1.4f;
static const float Z_JUGADOR = 0.7f;
static const float SEPARACION_CARRILES = 6.0f;
static const float ANCHO_CARRIL = 5.0f;

static const Color COLORES_JUGADORES_BATEO[MAX_PARTICIPANTES] =
{
    { 235, 80, 80, 255 },
    { 80, 140, 240, 255 },
    { 90, 205, 115, 255 },
    { 245, 205, 70, 255 }
};


//==================================================
// UTILIDADES LOGICAS
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


// Aproximacion de una normal estandar (suma de 4 uniformes, Irwin-Hall).
static float AleatorioGaussiano()
{
    float suma = 0.0f;

    for (int i = 0; i < 4; i++)
    {
        suma += Aleatorio01();
    }

    return (suma - 2.0f) / 0.577f;
}


static bool JugadorEsBot(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static bool EstadoTerminal(EstadoLanzamientoBateo estado)
{
    return
        estado == LANZAMIENTO_PASADO ||
        estado == LANZAMIENTO_ATERRIZADO ||
        estado == LANZAMIENTO_EXPLOTADO;
}


static int PuntosPorDistancia(float distancia)
{
    float diferencia = std::fabs(distancia - DISTANCIA_CENTRO_ANILLOS);

    if (diferencia < 1.8f) return 100;
    if (diferencia < 4.2f) return 60;
    if (diferencia < 8.0f) return 30;

    return 10;
}


// Posicion del meteorito que llega hacia el bateador.
static Vector3 PosicionMeteoritoEntrante(
    float carrilX,
    const LanzamientoBateo& lanzamiento,
    float reloj
)
{
    float limite = (lanzamiento.duracion + 0.45f) / lanzamiento.duracion;
    float u = Acotar(reloj / lanzamiento.duracion, 0.0f, limite);
    float f = std::pow(u, lanzamiento.exponente);
    float x = carrilX + lanzamiento.curva * std::sin(PI * Acotar(f, 0.0f, 1.0f));
    float y = Y_CANON + (Y_PUNTO_DULCE - Y_CANON) * f;

    if (y < 0.45f) y = 0.45f;

    return { x, y, Z_CANON + (Z_PUNTO_DULCE - Z_CANON) * f };
}


// Posicion del meteorito ya golpeado (arco hacia el cielo y aterrizaje).
static Vector3 PosicionMeteoritoGolpeado(const EstadoJugadorBateo& e, float s)
{
    s = Acotar(s, 0.0f, 1.0f);
    float apice = 3.0f + 0.35f * e.distancia;

    return
    {
        e.puntoGolpe.x + e.desvio * s,
        Y_PUNTO_DULCE + apice * 4.0f * s * (1.0f - s),
        e.puntoGolpe.z - e.distancia * s
    };
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

static void ConfigurarCamaraBateo(MinijuegoBateoMeteorico& m)
{
    m.camara.up = { 0.0f, 1.0f, 0.0f };
    m.camara.projection = CAMERA_PERSPECTIVE;

    // Picada de 33 grados: los anillos de puntaje se leen a los costados
    // de los canones y alrededor de la cubierta se ve el espacio abierto.
    // Mas carriles, camara mas lejos por el mismo eje.
    const float picada = 33.0f * DEG2RAD;
    float distancia = 33.0f;
    float zObjetivo = -13.0f;

    if (m.cantidadCarriles <= 2)
    {
        distancia = 24.0f;
        zObjetivo = -10.0f;
    }
    else if (m.cantidadCarriles == 3)
    {
        distancia = 28.5f;
        zObjetivo = -11.5f;
    }

    m.camara.target = { 0.0f, 0.0f, zObjetivo };
    m.camara.position =
    {
        0.0f,
        distancia * std::sin(picada),
        zObjetivo + distancia * std::cos(picada)
    };
    m.camara.fovy = 54.0f;
}


// Solo las piezas jugables del paquete GLB (carril, campo, canon,
// meteoritos, bate y aro): el decorado se construye con primitivas.
static_assert(MODELO_BATEO_FAROL + 1 == MODELO_BATEO_CARRIL &&
    MODELO_BATEO_ARO + 1 == CANTIDAD_MODELOS_BATEO_METEORICO_3D,
    "Las piezas jugables de Bateo deben ser el tramo final del paquete");

static PaqueteModelosEscenarioRetro3D PaquetePiezasJugablesBateo()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteBateoMeteoricoRetro3D();
    paquete.recursos += MODELO_BATEO_CARRIL;
    paquete.definiciones += MODELO_BATEO_CARRIL;
    paquete.cantidad -= MODELO_BATEO_CARRIL;
    return paquete;
}


void MinijuegoBateoMeteorico::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadCarriles = 0;
    ronda = 0;
    etapa = ETAPA_BATEO_AVISO;
    fase = FASE_BATEO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_BATEO;
    tiempoRestante = TIEMPO_LIMITE_BATEO;
    tiempoEtapa = 0.0f;
    tiempoAnimacion = 0.0f;

    for (int i = 0; i < MAX_CHISPAS_BATEO; i++) chispas[i] = {};
    for (int i = 0; i < MAX_AVISOS_BATEO; i++) avisos[i] = {};
    for (int i = 0; i < MAX_PARTICIPANTES; i++) destelloJugador[i] = 0.0f;
    semillaVisual = 7919u;
    tiempoVisual = 0.0f;
    tiempoAnuncioInicio = -1.0f;
    tiempoFin = -1.0f;

    // Lanzamientos base, mezclados: normal, curvas a ambos lados, rapido y
    // "lento enganoso" (parece lento y acelera al final).
    LanzamientoBateo base[LANZAMIENTOS_BATEO] =
    {
        { 1.20f, 0.0f, 1.0f },
        { 0.95f, 1.5f, 1.0f },
        { 1.10f, -1.5f, 1.0f },
        { 1.55f, 0.0f, 2.4f },
        { 0.85f, 0.8f, 1.0f }
    };

    for (int i = LANZAMIENTOS_BATEO - 1; i > 0; i--)
    {
        int j = GetRandomValue(0, i);
        LanzamientoBateo temporal = base[i];
        base[i] = base[j];
        base[j] = temporal;
    }

    for (int i = 0; i < LANZAMIENTOS_BATEO; i++)
    {
        lanzamientos[i] = base[i];
    }

    ConfigurarCamaraBateo(*this);
}


void MinijuegoBateoMeteorico::Reiniciar(
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

    int total = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            total++;
        }
    }

    if (total < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_BATEO_TERMINADO;
        return;
    }

    cantidadCarriles = total;
    CargarPaqueteModelosEscenarioRetro3D(PaquetePiezasJugablesBateo());
    ConfigurarCamaraBateo(*this);

    int carril = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        EstadoJugadorBateo& e = estadosJugadores[i];
        e.carril = carril;
        e.carrilX = ((float)carril - (float)(total - 1) * 0.5f) * SEPARACION_CARRILES;
        e.sigmaTiming = 0.05f + 0.07f * Aleatorio01();

        // Un meteorito dorado y uno rojo en lanzamientos distintos.
        int dorado = GetRandomValue(0, LANZAMIENTOS_BATEO - 1);
        int rojo = GetRandomValue(0, LANZAMIENTOS_BATEO - 2);

        if (rojo >= dorado)
        {
            rojo++;
        }

        for (int k = 0; k < LANZAMIENTOS_BATEO; k++)
        {
            e.tipos[k] = METEORITO_NORMAL;
        }

        e.tipos[dorado] = METEORITO_DORADO;
        e.tipos[rojo] = METEORITO_ROJO;

        jugadores[i].posicion = { e.carrilX, 0.0f, Z_JUGADOR };
        jugadores[i].posicionSpawn = jugadores[i].posicion;
        carril++;
    }
}


//==================================================
// LOGICA DE GOLPE Y LANZAMIENTO
//==================================================

struct SonidosFrameBateo
{
    bool impacto = false;
    bool acierto = false;
    bool error = false;
    bool explosion = false;
};


static void IniciarMensaje(EstadoJugadorBateo& e, MensajeBateo mensaje, int puntos)
{
    e.mensaje = mensaje;
    e.puntosMensaje = puntos;
    e.tiempoMensaje = 2.0f;
}


static void RealizarGolpe(
    EstadoJugadorBateo& e,
    const LanzamientoBateo& lanzamiento,
    bool esHumano,
    SonidosFrameBateo& sonidos
)
{
    e.yaGolpeo = true;
    e.tiempoSwing = DURACION_SWING;

    float desfase = e.reloj - lanzamiento.duracion;
    float error = std::fabs(desfase);

    if (error > VENTANA_GOLPE)
    {
        // Balanceo fuera de tiempo: el meteorito sigue de largo.
        if (e.tipoActual == METEORITO_ROJO)
        {
            IniciarMensaje(e, MENSAJE_BATEO_EVITADO, 0);
        }
        else
        {
            IniciarMensaje(e, MENSAJE_BATEO_FALLO, 0);

            if (esHumano)
            {
                sonidos.error = true;
            }
        }

        return;
    }

    Vector3 posicionGolpe = PosicionMeteoritoEntrante(e.carrilX, lanzamiento, e.reloj);
    e.puntoGolpe = posicionGolpe;
    e.tiempoImpacto = 0.5f;
    e.tiempoPostEvento = 0.0f;

    if (e.tipoActual == METEORITO_ROJO)
    {
        // El meteorito inestable explota en las manos del bateador.
        e.estado = LANZAMIENTO_EXPLOTADO;
        int antes = e.puntos;
        e.puntos -= PUNTOS_CASTIGO_ROJO;

        if (e.puntos < 0) e.puntos = 0;

        e.puntoAterrizaje = posicionGolpe;
        IniciarMensaje(e, MENSAJE_BATEO_ROJO, e.puntos - antes);

        if (esHumano)
        {
            sonidos.error = true;
        }

        return;
    }

    float calidad = 1.0f;

    if (error > VENTANA_PERFECTO)
    {
        calidad = 1.0f - (error - VENTANA_PERFECTO) / (VENTANA_GOLPE - VENTANA_PERFECTO);
    }

    e.calidad = Acotar(calidad, 0.0f, 1.0f);
    e.distancia = 3.0f + 19.0f * e.calidad;
    e.duracionVuelo = 0.9f + 0.7f * e.calidad;
    e.tiempoVuelo = 0.0f;
    // Golpe temprano desvia a la izquierda, tarde a la derecha.
    e.desvio = Acotar(desfase / VENTANA_GOLPE, -1.0f, 1.0f) * 1.4f;
    e.estado = LANZAMIENTO_GOLPEADO;

    int calidadEntera = (int)(e.calidad * 1000.0f);

    if (calidadEntera > e.mejorGolpe)
    {
        e.mejorGolpe = calidadEntera;
    }

    if (error <= VENTANA_PERFECTO)
    {
        e.golpesPerfectos++;
    }

    if (esHumano)
    {
        if (error <= VENTANA_PERFECTO)
        {
            sonidos.acierto = true;
        }
        else
        {
            sonidos.impacto = true;
        }
    }
}


static void AterrizarMeteorito(
    EstadoJugadorBateo& e,
    bool esHumano,
    SonidosFrameBateo& sonidos
)
{
    e.estado = LANZAMIENTO_ATERRIZADO;
    e.tiempoPostEvento = 0.0f;
    e.puntoAterrizaje = PosicionMeteoritoGolpeado(e, 1.0f);
    e.puntoAterrizaje.y = 0.05f;

    int puntos = PuntosPorDistancia(e.distancia);
    bool dorado = e.tipoActual == METEORITO_DORADO;

    if (dorado)
    {
        puntos *= 2;

        if (esHumano)
        {
            sonidos.explosion = true;
        }
    }

    e.puntos += puntos;

    MensajeBateo mensaje = MENSAJE_BATEO_FLOJO;

    if (e.calidad >= 0.95f)
    {
        mensaje = MENSAJE_BATEO_PERFECTO;
    }
    else if (puntos >= 60)
    {
        mensaje = MENSAJE_BATEO_BUENO;
    }
    else if (puntos >= 30)
    {
        mensaje = MENSAJE_BATEO_REGULAR;
    }

    IniciarMensaje(e, mensaje, puntos);
}


static void PrepararLanzamiento(
    MinijuegoBateoMeteorico& m,
    EstadoJugadorBateo& e,
    int ronda
)
{
    e.estado = LANZAMIENTO_EN_VUELO;
    e.tipoActual = e.tipos[ronda];
    e.reloj = 0.0f;
    e.yaGolpeo = false;
    e.tiempoVuelo = 0.0f;
    e.tiempoPostEvento = 0.0f;

    // IA: evita el rojo el 70% de las veces; el resto golpea con error gaussiano.
    e.botGolpeara = e.tipoActual != METEORITO_ROJO || Aleatorio01() >= 0.7f;

    float instante = m.lanzamientos[ronda].duracion + AleatorioGaussiano() * e.sigmaTiming;
    e.instanteGolpeBot = instante < 0.05f ? 0.05f : instante;
}


static bool TodosTerminaron(const MinijuegoBateoMeteorico& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        if (
            m.resultado.participantes[i].participo &&
            !EstadoTerminal(m.estadosJugadores[i].estado)
        )
        {
            return false;
        }
    }

    return true;
}


static void FinalizarBateo(MinijuegoBateoMeteorico& m)
{
    if (m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int cantidadPrimeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorBateo& a = m.estadosJugadores[i];
        int posicion = 1;

        // Mayor puntaje; si empatan, mejor golpe individual; si no, empate.
        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (j == i || !m.resultado.participantes[j].participo)
            {
                continue;
            }

            const EstadoJugadorBateo& b = m.estadosJugadores[j];

            if (
                b.puntos > a.puntos ||
                (b.puntos == a.puntos && b.mejorGolpe > a.mejorGolpe)
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = a.puntos;
        resultadoJugador.puntosObtenidos = 0;

        if (posicion == 1)
        {
            cantidadPrimeros++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace =
        cantidadPrimeros == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    m.fase = FASE_BATEO_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoBateoMeteorico::ActualizarPartida(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;

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

    if (fase == FASE_BATEO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_BATEO_JUGANDO;
            etapa = ETAPA_BATEO_AVISO;
            tiempoEtapa = 0.0f;
        }

        return;
    }

    if (fase != FASE_BATEO_JUGANDO)
    {
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarBateo(*this);
        return;
    }

    tiempoEtapa += deltaTime;
    SonidosFrameBateo sonidos;

    // Animaciones y mensajes que corren siempre.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorBateo& e = estadosJugadores[i];

        if (e.tiempoSwing > 0.0f) e.tiempoSwing -= deltaTime;
        if (e.tiempoImpacto > 0.0f) e.tiempoImpacto -= deltaTime;
        if (e.tiempoMensaje > 0.0f) e.tiempoMensaje -= deltaTime;

        if (
            e.estado == LANZAMIENTO_PASADO ||
            e.estado == LANZAMIENTO_EXPLOTADO ||
            e.estado == LANZAMIENTO_ATERRIZADO
        )
        {
            e.tiempoPostEvento += deltaTime;
        }
    }

    if (etapa == ETAPA_BATEO_AVISO)
    {
        if (tiempoEtapa >= DURACION_AVISO_BATEO)
        {
            for (int i = 0; i < limite; i++)
            {
                if (resultado.participantes[i].participo)
                {
                    PrepararLanzamiento(*this, estadosJugadores[i], ronda);
                }
            }

            ReproducirSonidoMinijuego(audio, SONIDO_DISPARO);
            etapa = ETAPA_BATEO_VUELO;
            tiempoEtapa = 0.0f;
        }

        return;
    }

    if (etapa == ETAPA_BATEO_VUELO)
    {
        const LanzamientoBateo& lanzamiento = lanzamientos[ronda];

        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            EstadoJugadorBateo& e = estadosJugadores[i];
            bool esBot = JugadorEsBot(participantes[i]);

            if (e.estado == LANZAMIENTO_EN_VUELO)
            {
                e.reloj += deltaTime;

                bool quiereGolpear = false;

                if (!e.yaGolpeo)
                {
                    if (esBot)
                    {
                        quiereGolpear = e.botGolpeara && e.reloj >= e.instanteGolpeBot;
                    }
                    else
                    {
                        InputMinijuegoParticipante entrada =
                            LeerInputMinijuegoParticipante(participantes[i]);
                        quiereGolpear = entrada.golpear;
                    }
                }

                if (quiereGolpear)
                {
                    RealizarGolpe(e, lanzamiento, !esBot, sonidos);
                }

                // El meteorito pasa de largo si nadie lo golpeo a tiempo.
                if (
                    e.estado == LANZAMIENTO_EN_VUELO &&
                    e.reloj > lanzamiento.duracion + VENTANA_GOLPE
                )
                {
                    e.estado = LANZAMIENTO_PASADO;
                    e.tiempoPostEvento = 0.0f;

                    if (!e.yaGolpeo)
                    {
                        if (e.tipoActual == METEORITO_ROJO)
                        {
                            IniciarMensaje(e, MENSAJE_BATEO_EVITADO, 0);
                        }
                        else
                        {
                            IniciarMensaje(e, MENSAJE_BATEO_SIN_GOLPE, 0);

                            if (!esBot)
                            {
                                sonidos.error = true;
                            }
                        }
                    }
                }
            }
            else if (e.estado == LANZAMIENTO_GOLPEADO)
            {
                e.tiempoVuelo += deltaTime;
                e.reloj += deltaTime;

                if (e.tiempoVuelo >= e.duracionVuelo)
                {
                    AterrizarMeteorito(e, !esBot, sonidos);
                }
            }
            else if (e.estado == LANZAMIENTO_PASADO)
            {
                e.reloj += deltaTime;
            }
        }

        if (TodosTerminaron(*this, limite))
        {
            etapa = ETAPA_BATEO_PAUSA;
            tiempoEtapa = 0.0f;
        }
        else if (tiempoEtapa > TOPE_ETAPA_VUELO)
        {
            // Seguridad: cierra a la fuerza lo que siga en el aire.
            for (int i = 0; i < limite; i++)
            {
                if (!resultado.participantes[i].participo)
                {
                    continue;
                }

                EstadoJugadorBateo& e = estadosJugadores[i];

                if (e.estado == LANZAMIENTO_GOLPEADO)
                {
                    AterrizarMeteorito(e, !JugadorEsBot(participantes[i]), sonidos);
                }
                else if (!EstadoTerminal(e.estado))
                {
                    e.estado = LANZAMIENTO_PASADO;
                    e.tiempoPostEvento = 0.0f;
                }
            }

            etapa = ETAPA_BATEO_PAUSA;
            tiempoEtapa = 0.0f;
        }
    }
    else if (etapa == ETAPA_BATEO_PAUSA)
    {
        if (tiempoEtapa >= DURACION_PAUSA_BATEO)
        {
            ronda++;

            if (ronda >= LANZAMIENTOS_BATEO)
            {
                FinalizarBateo(*this);
            }
            else
            {
                for (int i = 0; i < limite; i++)
                {
                    if (resultado.participantes[i].participo)
                    {
                        estadosJugadores[i].estado = LANZAMIENTO_PENDIENTE;
                        estadosJugadores[i].yaGolpeo = false;
                    }
                }

                etapa = ETAPA_BATEO_AVISO;
                tiempoEtapa = 0.0f;
            }
        }
    }

    // Un solo sonido de cada tipo por frame para no saturar con 4 carriles.
    if (sonidos.acierto)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
    }
    else if (sonidos.impacto)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_IMPACTO);
    }

    if (sonidos.error)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ERROR);
    }

    if (sonidos.explosion)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_EXPLOSION);
    }
}


//==================================================
// VISUAL: PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Plataforma de observacion en orbita: cubierta de placas de casco con
// juntas y remaches, un carril GLB por jugador, cupula con telescopio,
// antena de radar, alas de paneles solares y brazo de atraque con una
// lanzadera; mas alla, cinturon de asteroides en dos profundidades, un
// planeta anillado enorme con su luna, nebulosa, Via Lactea y una lluvia
// de meteoritos que nace de un radiante. El decorado es todo primitivas
// (rlgl); solo las piezas jugables usan sus GLB. La logica de tiempos,
// trayectorias y puntaje no depende de nada de esto.
//==================================================

static const Color COLOR_NIEBLA_BATEO = { 34, 30, 78, 255 };
static const Color COLOR_PLACA_BATEO = { 104, 114, 138, 255 };
static const Color COLOR_PLACA_OSCURA_BATEO = { 78, 86, 108, 255 };
static const Color COLOR_JUNTA_BATEO = { 42, 46, 64, 255 };
static const Color COLOR_CASCO_BATEO = { 184, 190, 206, 255 };
static const Color COLOR_CASCO_OSCURO_BATEO = { 58, 64, 86, 255 };
static const Color COLOR_FRANJA_BATEO = { 236, 130, 58, 255 };
static const Color COLOR_SOLAR_BATEO = { 40, 66, 150, 255 };
static const Color COLOR_LUZ_BATEO = { 255, 232, 196, 255 };
static const Color COLOR_VENTANA_BATEO = { 255, 212, 150, 255 };
static const Color COLOR_ACENTO_BATEO = { 120, 255, 222, 255 };
static const Color COLOR_ROCA_BATEO = { 116, 100, 94, 255 };

// Colores de los anillos de puntaje (los mismos del campo GLB).
static const Color COLOR_ANILLO_100_BATEO = { 255, 214, 86, 255 };
static const Color COLOR_ANILLO_60_BATEO = { 190, 126, 255, 255 };
static const Color COLOR_ANILLO_30_BATEO = { 100, 160, 245, 255 };
static const Color COLOR_ANILLO_10_BATEO = { 176, 186, 206, 255 };

static const float Z_FRENTE_CUBIERTA_BATEO = 8.0f;
static const float Z_FONDO_CUBIERTA_BATEO = -33.5f;
static const float Y_CUBIERTA_BATEO = -0.03f;
static const float Y_CALCO_BATEO = 0.075f;
static const float DURACION_ANUNCIO_BATEO = 1.8f;

// La camara mira en picada: el planeta queda abajo a la derecha, junto a
// la esquina del fondo de la cubierta, y la luna arriba a la izquierda.
static const Vector3 CENTRO_PLANETA_BATEO = { 148.0f, -95.0f, -259.0f };
static const float RADIO_PLANETA_BATEO = 60.0f;
static const Vector3 CENTRO_LUNA_BATEO = { -150.0f, -22.0f, -300.0f };
static const float RADIO_LUNA_BATEO = 11.0f;
// Sol fuera de cuadro, a la izquierda y algo de frente.
static const Vector3 SOL_BATEO = { -0.72f, 0.38f, 0.58f };


static float Limitar01Bateo(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static unsigned char CanalBateo(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoBateo(Color color, float factor)
{
    return Color{
        CanalBateo((float)color.r * factor),
        CanalBateo((float)color.g * factor),
        CanalBateo((float)color.b * factor),
        color.a
    };
}


static Color MezclaBateo(Color a, Color b, float t)
{
    t = Limitar01Bateo(t);

    return Color{
        CanalBateo((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalBateo((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalBateo((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalBateo((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaBateo(Color color, float alfa)
{
    color.a = CanalBateo(255.0f * alfa);
    return color;
}


// Azar visual determinista por indice (nunca toca GetRandomValue).
static float Ruido01Bateo(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Ojo de la niebla y ejes de pantalla de la vista del frame: los fija
// Dibujar antes de BeginMode3D.
static Vector3 ojoBateo = { 0.0f, 11.0f, 15.5f };
static Vector3 frenteUBateo = { 1.0f, 0.0f, 0.0f };
static Vector3 frenteVBateo = { 0.0f, 1.0f, 0.0f };


// Bruma indigo del espacio: lo lejano se funde con la nebulosa.
static float CantidadNieblaBateo(Vector3 p)
{
    float dx = p.x - ojoBateo.x;
    float dy = p.y - ojoBateo.y;
    float dz = p.z - ojoBateo.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 30.0f) / 150.0f;
    if (n < 0.0f) return 0.0f;
    return n > 0.84f ? 0.84f : n;
}


static Color TinteBateo(Color color, Vector3 p)
{
    Color niebla = COLOR_NIEBLA_BATEO;
    niebla.a = color.a;
    return MezclaBateo(color, niebla, CantidadNieblaBateo(p));
}


static void VerticeBateo(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadBateo(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeBateo(a, color);
    VerticeBateo(b, color);
    VerticeBateo(c, color);
    VerticeBateo(d, color);
}


static void QuadNieblaBateo(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeBateo(a, TinteBateo(color, a));
    VerticeBateo(b, TinteBateo(color, b));
    VerticeBateo(c, TinteBateo(color, c));
    VerticeBateo(d, TinteBateo(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.55). Respeta la matriz actual de rlgl.
static void CuboBateo(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadBateo({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, color);
    QuadBateo({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoBateo(color, 0.78f));
    QuadBateo({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoBateo(color, 0.55f));
    QuadBateo({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoBateo(color, 0.62f));
    QuadBateo({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoBateo(color, 0.70f));
    QuadBateo({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoBateo(color, 0.45f));
    rlEnd();
}


static void CuboNieblaBateo(Vector3 c, Vector3 t, Color color)
{
    CuboBateo(c, t, TinteBateo(color, c));
}


// Caja girada alrededor de Y (grados).
static void CuboGiradoBateo(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboBateo({ 0.0f, 0.0f, 0.0f }, t, color);
    rlPopMatrix();
}


static Vector3 SumaBateo(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaBateo(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaBateo(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
static float PuntoBateo(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }


static Vector3 NormalizarBateo(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 0.0001f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static Vector3 CruzBateo(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}


// Luz de la escena: de arriba, de la izquierda (el sol) y algo de frente.
static float LuzNormalBateo(Vector3 n)
{
    const float LX = -0.42f, LY = 0.80f, LZ = 0.43f;
    float d = n.x * LX + n.y * LY + n.z * LZ;
    return 0.55f + 0.47f * Limitar01Bateo(d);
}


// Prisma (o tronco de cono) entre dos puntos cualesquiera. emisivo acerca
// todas las caras a la luz plena (vidrios, lamparas).
static void PrismaEjeBateo(
    Vector3 a,
    Vector3 b,
    float r0,
    float r1,
    int lados,
    Color color,
    float giro = 0.0f,
    float emisivo = 0.0f,
    bool niebla = false
)
{
    Vector3 eje = NormalizarBateo(RestaBateo(b, a));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarBateo(CruzBateo(eje, apoyo));
    Vector3 v = CruzBateo(eje, u);
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaBateo(EscalaBateo(u, std::cos(a0)), EscalaBateo(v, std::sin(a0)));
        Vector3 d1 = SumaBateo(EscalaBateo(u, std::cos(a1)), EscalaBateo(v, std::sin(a1)));
        Vector3 dm = SumaBateo(EscalaBateo(u, std::cos(am)), EscalaBateo(v, std::sin(am)));

        float f = LuzNormalBateo(dm);
        f = f + (1.0f - f) * emisivo;
        if (k & 1) f *= 0.95f;
        Color cara = TonoBateo(color, f);

        Vector3 p0 = SumaBateo(a, EscalaBateo(d0, r0));
        Vector3 p1 = SumaBateo(a, EscalaBateo(d1, r0));
        Vector3 p2 = SumaBateo(b, EscalaBateo(d1, r1));
        Vector3 p3 = SumaBateo(b, EscalaBateo(d0, r1));

        if (niebla) QuadNieblaBateo(p0, p1, p2, p3, cara);
        else QuadBateo(p0, p1, p2, p3, cara);

        if (r1 > 0.001f)
        {
            Color tapa = TonoBateo(color, LuzNormalBateo(eje) + (1.0f - LuzNormalBateo(eje)) * emisivo);
            if (niebla) tapa = TinteBateo(tapa, b);
            QuadBateo(b, p2, p3, b, tapa);
        }
    }

    rlEnd();
}


// Esfera o casquete facetado (latitudes en radianes) con luz por vertice.
static void EsferaBateo(
    Vector3 c,
    Vector3 r,
    int anillos,
    int lados,
    Color color,
    float latMin = -1.5708f,
    float latMax = 1.5708f,
    float emisivo = 0.0f,
    bool niebla = false
)
{
    for (int a = 0; a < anillos; a++)
    {
        rlBegin(RL_QUADS);

        for (int k = 0; k < lados; k++)
        {
            int aa[4] = { a, a, a + 1, a + 1 };
            int kk[4] = { k, k + 1, k + 1, k };

            for (int q = 0; q < 4; q++)
            {
                float th = latMin + (latMax - latMin) * (float)aa[q] / (float)anillos;
                float ph = 6.2832f * (float)kk[q] / (float)lados;
                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = { c.x + n.x * r.x, c.y + n.y * r.y, c.z + n.z * r.z };
                float f = LuzNormalBateo(n);
                f = f + (1.0f - f) * emisivo;
                Color cv = TonoBateo(color, f);
                VerticeBateo(p, niebla ? TinteBateo(cv, p) : cv);
            }
        }

        rlEnd();
    }
}


// Roca facetada (asteroide, meteorito de utileria): esfera de pocos
// lados con radios irregulares, girada en Y sin mover la luz.
static void RocaBateo(Vector3 c, Vector3 r, int semilla, Color color, float giro = 0.0f, bool niebla = false)
{
    const int ANILLOS = 5;
    const int LADOS = 8;
    float cg = std::cos(giro);
    float sg = std::sin(giro);

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
                float f = aa[q] == 0 || aa[q] == ANILLOS
                    ? 0.95f
                    : 0.82f + 0.36f * Ruido01Bateo(aa[q] * 31 + (kk[q] % LADOS), semilla);
                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 local = { n.x * r.x * f, n.y * r.y * f, n.z * r.z * f };
                Vector3 girado = { local.x * cg - local.z * sg, local.y, local.x * sg + local.z * cg };
                Vector3 nGirada = { n.x * cg - n.z * sg, n.y, n.x * sg + n.z * cg };
                Vector3 p = SumaBateo(c, girado);
                Color cv = TonoBateo(color, LuzNormalBateo(nGirada) * (0.92f + 0.14f * Ruido01Bateo(aa[q] * 7 + kk[q], semilla + 5)));
                VerticeBateo(p, niebla ? TinteBateo(cv, p) : cv);
            }
        }
    }

    rlEnd();
}


// Corona en un plano cualquiera (u, v): color0 en el borde interior y
// color1 en el exterior.
static void AnilloPlanoBateo(Vector3 c, Vector3 u, Vector3 v, float r0, float r1, Color color0, Color color1, int lados = 28, float giro = 0.0f)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + 6.2832f * (float)k / (float)lados;
        float a1 = giro + 6.2832f * (float)(k + 1) / (float)lados;
        Vector3 d0 = SumaBateo(EscalaBateo(u, std::cos(a0)), EscalaBateo(v, std::sin(a0)));
        Vector3 d1 = SumaBateo(EscalaBateo(u, std::cos(a1)), EscalaBateo(v, std::sin(a1)));

        VerticeBateo(SumaBateo(c, EscalaBateo(d0, r1)), color1);
        VerticeBateo(SumaBateo(c, EscalaBateo(d0, r0)), color0);
        VerticeBateo(SumaBateo(c, EscalaBateo(d1, r0)), color0);
        VerticeBateo(SumaBateo(c, EscalaBateo(d1, r1)), color1);
    }

    rlEnd();
}


static void AnilloSueloBateo(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    AnilloPlanoBateo(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, r0, r1, color, color, lados);
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorBateo(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloBateo(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorBateo(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


static void ResplandorFrenteBateo(Vector3 c, float radio, Color color)
{
    ResplandorBateo(c, frenteUBateo, frenteVBateo, radio, color);
}


// Destello en cruz de cuatro puntas (estrellas, dorado, chispas).
static void DestelloCruzBateo(Vector3 c, float largo, Color color, float giro = 0.0f)
{
    Vector3 u = SumaBateo(EscalaBateo(frenteUBateo, std::cos(giro)), EscalaBateo(frenteVBateo, std::sin(giro)));
    Vector3 v = SumaBateo(EscalaBateo(frenteUBateo, -std::sin(giro)), EscalaBateo(frenteVBateo, std::cos(giro)));
    ResplandorBateo(c, u, v, largo * 0.12f, color, 8.0f);
    ResplandorBateo(c, v, u, largo * 0.12f, color, 8.0f);
}


// Cono de luz (aditivo): del foco hacia el charco, se apaga en la base.
static void ConoLuzBateo(Vector3 foco, Vector3 baseCentro, float radio, Color color)
{
    const int LADOS = 16;
    Vector3 eje = NormalizarBateo(RestaBateo(baseCentro, foco));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarBateo(CruzBateo(eje, apoyo));
    Vector3 v = CruzBateo(eje, u);

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaBateo(baseCentro, SumaBateo(EscalaBateo(u, std::cos(a0) * radio), EscalaBateo(v, std::sin(a0) * radio)));
        Vector3 p1 = SumaBateo(baseCentro, SumaBateo(EscalaBateo(u, std::cos(a1) * radio), EscalaBateo(v, std::sin(a1) * radio)));

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(foco.x, foco.y, foco.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p1.x, p1.y, p1.z);
    }
    rlEnd();
}


// Linea gruesa horizontal sobre el suelo (juntas, pintura, marcas).
static void TrazoSueloBateo(float xa, float za, float xb, float zb, float y, float ancho, Color color)
{
    float dx = xb - xa, dz = zb - za;
    float l = std::sqrt(dx * dx + dz * dz);
    if (l < 0.0001f) return;
    float nx = -dz / l * ancho * 0.5f, nz = dx / l * ancho * 0.5f;

    rlBegin(RL_QUADS);
    QuadBateo({ xa + nx, y, za + nz }, { xb + nx, y, zb + nz }, { xb - nx, y, zb - nz }, { xa - nx, y, za - nz }, color);
    rlEnd();
}


// Viga fina (barandas, reticulados, antenas).
static void VigaBateo(Vector3 a, Vector3 b, float grosor, Color color, bool niebla = true)
{
    PrismaEjeBateo(a, b, grosor, grosor, 4, color, 0.785f, 0.0f, niebla);
}


//==================================================
// VISUAL: DISPOSICION DE LA PLATAFORMA
//==================================================

struct DisposicionBateo
{
    float mitadCarriles = 11.7f;    // borde exterior de los carriles
    float mitadCubierta = 17.1f;    // borde de la cubierta
    float margen = 14.4f;           // centro del margen de utileria
};


static DisposicionBateo CalcularDisposicionBateo(const MinijuegoBateoMeteorico& m)
{
    int cantidad = m.cantidadCarriles >= 2 ? m.cantidadCarriles : 2;
    DisposicionBateo d;
    d.mitadCarriles = (float)(cantidad - 1) * 0.5f * SEPARACION_CARRILES + ANCHO_CARRIL * 0.5f + 0.2f;
    d.mitadCubierta = d.mitadCarriles + 5.4f;
    d.margen = d.mitadCarriles + 2.7f;
    return d;
}


static const int CANTIDAD_FOCOS_BATEO = 4;


static float LadoFocoBateo(int indice)
{
    return indice % 2 == 0 ? -1.0f : 1.0f;
}


static Vector3 BaseFocoBateo(const DisposicionBateo& d, int indice)
{
    float s = LadoFocoBateo(indice);
    float z = indice < 2 ? -3.0f : -18.0f;
    return { s * (d.mitadCubierta - 0.9f), 0.0f, z };
}


static Vector3 CabezaFocoBateo(const DisposicionBateo& d, int indice)
{
    Vector3 b = BaseFocoBateo(d, indice);
    return { b.x - LadoFocoBateo(indice) * 0.8f, 6.7f, b.z };
}


static Vector3 CharcoFocoBateo(const DisposicionBateo& d, int indice)
{
    Vector3 b = BaseFocoBateo(d, indice);
    return { LadoFocoBateo(indice) * d.mitadCarriles * 0.5f, 0.02f, b.z - 1.5f };
}


// Los focos se encienden en ola durante la cuenta regresiva.
static float EncendidoFocoBateo(const MinijuegoBateoMeteorico& m, int indice)
{
    if (m.fase != FASE_BATEO_PREPARACION)
    {
        return 1.0f;
    }

    float transcurrido = DURACION_PREPARACION_BATEO - m.tiempoPreparacion;
    return Limitar01Bateo((transcurrido - 0.3f - 0.32f * (float)indice) * 4.0f);
}


static Color ColorTipoMeteoritoBateo(TipoMeteoritoBateo tipo)
{
    if (tipo == METEORITO_DORADO) return Color{ 255, 214, 96, 255 };
    if (tipo == METEORITO_ROJO) return Color{ 255, 62, 48, 255 };
    return Color{ 255, 150, 72, 255 };
}


static Color ColorPuntosBateo(int puntos)
{
    if (puntos >= 100) return COLOR_ANILLO_100_BATEO;
    if (puntos >= 60) return COLOR_ANILLO_60_BATEO;
    if (puntos >= 30) return COLOR_ANILLO_30_BATEO;
    return COLOR_ANILLO_10_BATEO;
}


// Posicion del meteorito en vuelo (entrante o golpeado) segun el estado.
static bool PosicionMeteoritoVisibleBateo(
    const MinijuegoBateoMeteorico& m,
    const EstadoJugadorBateo& e,
    float atras,
    Vector3& salida
)
{
    if (m.fase != FASE_BATEO_JUGANDO || m.ronda >= LANZAMIENTOS_BATEO)
    {
        return false;
    }

    const LanzamientoBateo& lanzamiento = m.lanzamientos[m.ronda];

    if (
        e.estado == LANZAMIENTO_EN_VUELO ||
        (e.estado == LANZAMIENTO_PASADO && e.reloj <= lanzamiento.duracion + 0.45f)
    )
    {
        float reloj = e.reloj - atras;
        if (reloj < 0.0f) return false;
        salida = PosicionMeteoritoEntrante(e.carrilX, lanzamiento, reloj);
        return true;
    }

    if (e.estado == LANZAMIENTO_GOLPEADO)
    {
        float s = e.tiempoVuelo / e.duracionVuelo - atras / e.duracionVuelo;
        if (s < 0.0f) return false;
        salida = PosicionMeteoritoGolpeado(e, s);
        return true;
    }

    return false;
}


//==================================================
// VISUAL: CIELO, PLANETA Y CINTURON DE ASTEROIDES
//==================================================

// Fondo 2D: espacio profundo con nebulosa, Via Lactea, estrellas que
// titilan y una lluvia de meteoritos que sale de un radiante.
static void DibujarFondoBateo(float t, int paso)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float w = (float)ancho;
    float h = (float)alto;
    int medio = (int)(h * 0.42f);

    DrawRectangleGradientV(0, 0, ancho, medio, Color{ 5, 6, 20, 255 }, Color{ 28, 22, 64, 255 });
    DrawRectangleGradientV(0, medio, ancho, alto - medio, Color{ 28, 22, 64, 255 }, Color{ 10, 10, 30, 255 });

    // Nebulosa: nubes de gas magenta, violeta y turquesa.
    struct NubeBateo { float x, y, r; Color c; };
    const NubeBateo nubes[] =
    {
        { 0.16f, 0.14f, 0.30f, { 160, 62, 176, 48 } },
        { 0.33f, 0.06f, 0.22f, { 96, 72, 200, 42 } },
        { 0.05f, 0.32f, 0.20f, { 60, 136, 190, 36 } },
        { 0.58f, 0.03f, 0.24f, { 176, 72, 150, 30 } },
        { 0.94f, 0.18f, 0.22f, { 70, 110, 196, 32 } },
        { 0.74f, 0.30f, 0.18f, { 120, 60, 170, 26 } }
    };

    for (const NubeBateo& n : nubes)
    {
        Color fuera = n.c;
        fuera.a = 0;
        DrawCircleGradient({ n.x * w, n.y * h }, n.r * w, n.c, fuera);
    }

    // Via Lactea: banda diagonal de polvo y estrellas finas.
    Vector2 inicioBanda = { -0.05f * w, 0.40f * h };
    Vector2 finBanda = { 1.05f * w, 0.00f * h };
    Vector2 dirBanda = { finBanda.x - inicioBanda.x, finBanda.y - inicioBanda.y };
    float largoBanda = std::sqrt(dirBanda.x * dirBanda.x + dirBanda.y * dirBanda.y);
    Vector2 normalBanda = { -dirBanda.y / largoBanda, dirBanda.x / largoBanda };

    for (int k = 0; k < 9; k++)
    {
        float u = ((float)k + 0.5f) / 9.0f;
        float x = inicioBanda.x + dirBanda.x * u;
        float y = inicioBanda.y + dirBanda.y * u;
        DrawCircleGradient({ x, y }, 0.075f * w, Color{ 200, 180, 255, 20 }, Color{ 200, 180, 255, 0 });
    }

    for (int k = 0; k < 260; k += paso)
    {
        float u = Ruido01Bateo(k, 301);
        float desvio = (Ruido01Bateo(k, 302) - 0.5f) * (Ruido01Bateo(k, 303) + 0.2f) * 0.16f * h;
        float x = inicioBanda.x + dirBanda.x * u + normalBanda.x * desvio;
        float y = inicioBanda.y + dirBanda.y * u + normalBanda.y * desvio;
        unsigned char a = (unsigned char)(60.0f + 120.0f * Ruido01Bateo(k, 304));
        DrawRectangle((int)x, (int)y, 1, 1, Color{ 236, 226, 255, a });
    }

    // Estrellas sueltas que titilan; algunas con destello en cruz.
    for (int k = 0; k < 170; k += paso)
    {
        float x = Ruido01Bateo(k, 311) * w;
        float y = Ruido01Bateo(k, 312) * h;
        float brillo = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(t * (0.8f + 2.2f * Ruido01Bateo(k, 313)) + (float)k));
        float tipo = Ruido01Bateo(k, 314);
        Color c = tipo < 0.2f ? Color{ 170, 200, 255, 255 } : (tipo > 0.85f ? Color{ 255, 220, 170, 255 } : Color{ 245, 245, 255, 255 });
        int lado = Ruido01Bateo(k, 315) > 0.8f ? 2 : 1;
        DrawRectangle((int)x, (int)y, lado, lado, AlfaBateo(c, brillo));

        if (k % 12 == 0)
        {
            float largo = (3.0f + 4.0f * brillo) * w / 1280.0f;
            Color cruz = AlfaBateo(c, 0.5f * brillo);
            DrawLineEx({ x - largo, y + 0.5f }, { x + largo + 1.0f, y + 0.5f }, 1.0f, cruz);
            DrawLineEx({ x + 0.5f, y - largo }, { x + 0.5f, y + largo + 1.0f }, 1.0f, cruz);
        }
    }

    // Lluvia de meteoritos: estelas que nacen de un radiante arriba a la
    // derecha (como las lluvias reales) y cruzan el cielo.
    Vector2 radiante = { 0.86f * w, -0.06f * h };
    float escala = w / 1280.0f;

    for (int k = 0; k < 11; k++)
    {
        float periodo = 2.4f + 2.6f * Ruido01Bateo(k, 321);
        float u = std::fmod(t + Ruido01Bateo(k, 322) * periodo, periodo) / 0.85f;

        if (u >= 1.0f)
        {
            continue;
        }

        float angulo = 3.1416f * (0.56f + 0.36f * Ruido01Bateo(k, 323));
        Vector2 dir = { std::cos(angulo), std::sin(angulo) };
        float inicio = (0.10f + 0.40f * Ruido01Bateo(k, 324)) * w;
        float avance = inicio + u * (0.22f + 0.1f * Ruido01Bateo(k, 325)) * w;
        Vector2 cabeza = { radiante.x + dir.x * avance, radiante.y + dir.y * avance };
        float largo = (70.0f + 90.0f * Ruido01Bateo(k, 326)) * escala * Limitar01Bateo(u * 4.0f);
        float alfa = std::sin(u * 3.1416f);

        for (int s = 0; s < 6; s++)
        {
            float a0 = (float)s / 6.0f;
            float a1 = (float)(s + 1) / 6.0f;
            Vector2 p0 = { cabeza.x - dir.x * largo * a0, cabeza.y - dir.y * largo * a0 };
            Vector2 p1 = { cabeza.x - dir.x * largo * a1, cabeza.y - dir.y * largo * a1 };
            Color c = s < 2 ? Color{ 255, 244, 220, 255 } : Color{ 255, 190, 140, 255 };
            DrawLineEx(p0, p1, (2.6f - 0.36f * (float)s) * escala, AlfaBateo(c, alfa * (1.0f - a0) * 0.9f));
        }

        DrawCircleGradient(cabeza, 7.0f * escala, AlfaBateo(Color{ 255, 236, 200, 255 }, 0.55f * alfa), Color{ 255, 200, 150, 0 });
        DrawCircleV(cabeza, 1.8f * escala, AlfaBateo(WHITE, alfa));
    }
}


static Color ColorBandaPlanetaBateo(float latitud, float longitud)
{
    float onda = latitud * 7.5f
        + 0.22f * std::sin(longitud * 3.0f + latitud * 11.0f)
        + 0.10f * std::sin(longitud * 7.0f - latitud * 5.0f);
    float banda = 0.5f + 0.5f * std::sin(onda * 3.1f);
    Color claro = { 244, 218, 172, 255 };
    Color oscuro = { 200, 128, 84, 255 };
    Color c = MezclaBateo(claro, oscuro, banda * 0.85f);

    // Una banda ecuatorial mas oscura y polos frios.
    c = MezclaBateo(c, Color{ 170, 98, 70, 255 }, Limitar01Bateo(1.0f - std::fabs(latitud + 0.08f) * 9.0f) * 0.5f);
    c = MezclaBateo(c, Color{ 178, 160, 156, 255 }, Limitar01Bateo((std::fabs(latitud) - 0.72f) * 3.0f));
    return c;
}


// Cuerpo celeste iluminado por el sol (planeta o luna).
static void DibujarCuerpoCelesteBateo(Vector3 c, float r, bool planeta, float t)
{
    const int ANILLOS = planeta ? 26 : 12;
    const int LADOS = planeta ? 44 : 20;
    Vector3 haciaOjo = NormalizarBateo(RestaBateo(ojoBateo, c));
    Vector3 sol = NormalizarBateo(SOL_BATEO);

    for (int a = 0; a < ANILLOS; a++)
    {
        rlBegin(RL_QUADS);

        for (int k = 0; k < LADOS; k++)
        {
            int aa[4] = { a, a, a + 1, a + 1 };
            int kk[4] = { k, k + 1, k + 1, k };

            for (int q = 0; q < 4; q++)
            {
                float th = -1.5708f + 3.1416f * (float)aa[q] / (float)ANILLOS;
                float ph = 6.2832f * (float)kk[q] / (float)LADOS;
                // Eje del planeta inclinado como sus anillos.
                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = SumaBateo(c, EscalaBateo(n, r));
                float difusa = Limitar01Bateo(PuntoBateo(n, sol) * 1.1f + 0.06f);
                float borde = 1.0f - Limitar01Bateo(PuntoBateo(n, haciaOjo));
                Color base;

                if (planeta)
                {
                    base = ColorBandaPlanetaBateo(std::sin(th), ph + t * 0.01f);
                }
                else
                {
                    float crater = Ruido01Bateo((aa[q] * 53 + kk[q] % LADOS) * 3, 77);
                    base = TonoBateo(Color{ 206, 204, 214, 255 }, 0.78f + 0.3f * crater);
                }

                Color col = TonoBateo(base, 0.12f + 0.95f * difusa);
                // Atmosfera: el borde iluminado se aclara.
                col = MezclaBateo(col, Color{ 255, 214, 170, 255 }, (planeta ? 0.45f : 0.15f) * borde * borde * difusa);
                col = MezclaBateo(col, COLOR_NIEBLA_BATEO, 0.10f);
                VerticeBateo(p, col);
            }
        }

        rlEnd();
    }
}


// Anillos del planeta: varias bandas con la division oscura de Cassini.
// Translucidos; la mitad de atras queda tapada por el planeta.
static void DibujarAnillosPlanetaBateo()
{
    Vector3 c = CENTRO_PLANETA_BATEO;
    float r = RADIO_PLANETA_BATEO;
    Vector3 normal = NormalizarBateo({ 0.30f, 1.0f, 0.50f });
    Vector3 u = NormalizarBateo(CruzBateo(normal, { 0.0f, 0.0f, 1.0f }));
    Vector3 v = CruzBateo(normal, u);

    struct BandaBateo { float r0, r1; Color c0, c1; };
    const BandaBateo bandas[] =
    {
        { 1.30f, 1.48f, { 150, 128, 112, 40 }, { 176, 150, 124, 110 } },
        { 1.50f, 1.70f, { 214, 190, 152, 170 }, { 236, 212, 170, 200 } },
        { 1.70f, 1.84f, { 226, 198, 156, 190 }, { 200, 172, 136, 150 } },
        { 1.90f, 2.10f, { 196, 172, 140, 150 }, { 214, 190, 156, 120 } },
        { 2.12f, 2.24f, { 170, 150, 128, 80 }, { 160, 140, 120, 20 } }
    };

    for (const BandaBateo& b : bandas)
    {
        AnilloPlanoBateo(c, u, v, b.r0 * r, b.r1 * r, b.c0, b.c1, 80);
    }
}


// Asteroides: un cinturon lejano de rocas chicas y unos pocos grandes a
// media distancia que giran despacio.
static void DibujarAsteroidesBateo(float t, int paso)
{
    for (int k = 0; k < 40; k += paso)
    {
        // Detras del planeta: donde se cruzan, el planeta los tapa.
        float x = (Ruido01Bateo(k, 401) - 0.5f) * 560.0f;
        float z = -330.0f - 90.0f * Ruido01Bateo(k, 402);
        float y = -30.0f + x * 0.05f + (Ruido01Bateo(k, 403) - 0.5f) * 18.0f;
        float tam = 1.4f + 3.6f * Ruido01Bateo(k, 404);
        Color c = TonoBateo(COLOR_ROCA_BATEO, 0.8f + 0.3f * Ruido01Bateo(k, 405));
        RocaBateo({ x, y, z }, { tam, tam * 0.75f, tam * 0.9f }, k + 11, c, t * 0.05f * (1.0f + Ruido01Bateo(k, 406)), true);
    }

    struct AsteroideBateo { float x, y, z, tam; };
    const AsteroideBateo cercanos[] =
    {
        // Ninguno delante del planeta (abajo a la derecha).
        { -52.0f, -8.0f, -62.0f, 4.2f },
        { -34.0f, -14.0f, -88.0f, 6.0f },
        { -70.0f, 4.0f, -96.0f, 5.0f },
        { 30.0f, -30.0f, -58.0f, 3.6f },
        { 72.0f, -58.0f, -96.0f, 5.6f },
        { 14.0f, -22.0f, -112.0f, 4.4f },
        { -12.0f, -24.0f, -96.0f, 3.2f },
        { -22.0f, 10.0f, -128.0f, 3.0f }
    };

    int indice = 0;

    for (const AsteroideBateo& a : cercanos)
    {
        float bob = std::sin(t * 0.3f + (float)indice * 1.7f) * 0.5f;
        Color c = TonoBateo(COLOR_ROCA_BATEO, 0.9f + 0.2f * Ruido01Bateo(indice, 410));
        RocaBateo(
            { a.x, a.y + bob, a.z },
            { a.tam, a.tam * 0.72f, a.tam * 0.86f },
            indice * 13 + 5,
            c,
            t * (0.06f + 0.04f * (float)(indice % 3)),
            true
        );

        // Piedritas que lo acompanan.
        if (paso == 1)
        {
            for (int q = 0; q < 3; q++)
            {
                float ang = t * 0.12f + (float)q * 2.1f + (float)indice;
                Vector3 p = { a.x + std::cos(ang) * a.tam * 1.8f, a.y + bob + std::sin(ang * 1.3f) * a.tam * 0.6f, a.z + std::sin(ang) * a.tam * 1.4f };
                float s = a.tam * 0.16f;
                RocaBateo(p, { s, s * 0.8f, s }, indice * 7 + q, c, ang, true);
            }
        }

        indice++;
    }
}


//==================================================
// VISUAL: ESTACION (MEDIA DISTANCIA)
//==================================================

// Ala de paneles solares: viga que sale de la cubierta y celdas azules
// con retícula y brillo del sol.
static void DibujarAlaSolarBateo(const DisposicionBateo& d, float lado, float zEje, int paso)
{
    float x0 = lado * (d.mitadCubierta + 0.4f);
    float x1 = lado * (d.mitadCubierta + 40.0f);
    float y = -0.9f;
    VigaBateo({ x0, y, zEje }, { x1, y, zEje }, 0.28f, COLOR_CASCO_BATEO);

    // Juntas de la viga.
    for (int k = 1; k < 8; k++)
    {
        float x = lado * (d.mitadCubierta + 0.4f + (float)k * 5.0f);
        CuboNieblaBateo({ x, y, zEje }, { 0.5f, 0.75f, 0.75f }, COLOR_CASCO_OSCURO_BATEO);
    }

    const float inclinacion = 0.32f;

    for (int k = 0; k < 7; k += paso)
    {
        float xc = lado * (d.mitadCubierta + 3.4f + (float)k * 5.2f);

        for (int cara = -1; cara <= 1; cara += 2)
        {
            float zc = zEje + (float)cara * 3.3f;
            float hx = 2.35f;
            float hz = 2.75f;
            float dy = std::sin(inclinacion) * hz;
            float dz = std::cos(inclinacion) * hz;

            // Marco claro.
            rlBegin(RL_QUADS);
            QuadNieblaBateo(
                { xc - hx - 0.12f, y - dy - 0.04f, zc + dz + 0.12f },
                { xc + hx + 0.12f, y - dy - 0.04f, zc + dz + 0.12f },
                { xc + hx + 0.12f, y + dy - 0.04f, zc - dz - 0.12f },
                { xc - hx - 0.12f, y + dy - 0.04f, zc - dz - 0.12f },
                COLOR_CASCO_BATEO
            );
            rlEnd();

            // Celdas: 3 x 4 con brillo que crece hacia el sol.
            rlBegin(RL_QUADS);
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    float u0 = -hx + (float)i * (2.0f * hx / 3.0f) + 0.05f;
                    float u1 = u0 + 2.0f * hx / 3.0f - 0.1f;
                    float v0 = -1.0f + (float)j * 0.5f + 0.02f;
                    float v1 = v0 + 0.46f;
                    float brillo = 0.85f + 0.3f * Limitar01Bateo(0.5f - lado * (u0 / hx) * 0.3f + (float)j * 0.08f);
                    Color celda = TonoBateo(COLOR_SOLAR_BATEO, brillo * (0.94f + 0.12f * Ruido01Bateo(k * 31 + i * 7 + j + cara * 3, 420)));
                    Color reflejo = MezclaBateo(celda, Color{ 150, 190, 255, 255 }, 0.22f);
                    Vector3 a = { xc + u0, y - dy * v0, zc + dz * v0 };
                    Vector3 b = { xc + u1, y - dy * v0, zc + dz * v0 };
                    Vector3 c = { xc + u1, y - dy * v1, zc + dz * v1 };
                    Vector3 e = { xc + u0, y - dy * v1, zc + dz * v1 };
                    VerticeBateo(a, TinteBateo(reflejo, a));
                    VerticeBateo(b, TinteBateo(celda, b));
                    VerticeBateo(c, TinteBateo(celda, c));
                    VerticeBateo(e, TinteBateo(reflejo, e));
                }
            }
            rlEnd();
        }
    }
}


// Cupula del observatorio con su telescopio: la ranura y el tubo giran
// despacio siguiendo la lluvia de meteoritos.
static Vector3 CentroCupulaBateo(const DisposicionBateo& d)
{
    return { -(d.mitadCubierta + 10.5f), 0.0f, -25.0f };
}


static float RumboCupulaBateo(float t)
{
    return 0.55f + 0.32f * std::sin(t * 0.13f);
}


static Vector3 BocaTelescopioBateo(const DisposicionBateo& d, float t)
{
    Vector3 c = CentroCupulaBateo(d);
    float rumbo = RumboCupulaBateo(t);
    float elevacion = 0.82f + 0.08f * std::sin(t * 0.21f);
    Vector3 dir = { std::cos(elevacion) * std::sin(rumbo), std::sin(elevacion), -std::cos(elevacion) * std::cos(rumbo) };
    Vector3 pivote = { c.x, 3.6f, c.z };
    return SumaBateo(pivote, EscalaBateo(dir, 7.4f));
}


static void DibujarObservatorioBateo(const DisposicionBateo& d, float t)
{
    Vector3 c = CentroCupulaBateo(d);

    // Plataforma redonda y pasarela hasta la cubierta.
    PrismaEjeBateo({ c.x, -1.2f, c.z }, { c.x, -0.05f, c.z }, 7.2f, 7.2f, 28, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);
    PrismaEjeBateo({ c.x, -2.6f, c.z }, { c.x, -1.2f, c.z }, 3.0f, 6.6f, 16, TonoBateo(COLOR_CASCO_OSCURO_BATEO, 0.8f), 0.0f, 0.0f, true);
    float xPasarela0 = -d.mitadCubierta;
    float xPasarela1 = c.x + 6.8f;
    CuboNieblaBateo({ (xPasarela0 + xPasarela1) * 0.5f, -0.2f, c.z }, { xPasarela0 - xPasarela1, 0.3f, 2.4f }, COLOR_PLACA_OSCURA_BATEO);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float z = c.z + (float)lado * 1.15f;
        VigaBateo({ xPasarela0, 0.9f, z }, { xPasarela1, 0.9f, z }, 0.05f, COLOR_CASCO_BATEO);
        for (int k = 0; k <= 3; k++)
        {
            float x = xPasarela0 + (xPasarela1 - xPasarela0) * (float)k / 3.0f;
            VigaBateo({ x, -0.05f, z }, { x, 0.9f, z }, 0.05f, COLOR_CASCO_BATEO);
        }
    }

    // Tambor con franja de ventanas.
    PrismaEjeBateo({ c.x, 0.0f, c.z }, { c.x, 2.6f, c.z }, 4.3f, 4.3f, 24, COLOR_CASCO_BATEO, 0.0f, 0.0f, true);
    PrismaEjeBateo({ c.x, 1.25f, c.z }, { c.x, 1.65f, c.z }, 4.34f, 4.34f, 24, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);

    for (int k = 0; k < 12; k++)
    {
        float a = 6.2832f * (float)k / 12.0f;
        Vector3 p = { c.x + std::cos(a) * 4.37f, 1.45f, c.z + std::sin(a) * 4.37f };
        Vector3 tangente = { -std::sin(a) * 0.3f, 0.0f, std::cos(a) * 0.3f };
        rlBegin(RL_QUADS);
        QuadBateo(
            { p.x - tangente.x, 1.32f, p.z - tangente.z },
            { p.x + tangente.x, 1.32f, p.z + tangente.z },
            { p.x + tangente.x, 1.58f, p.z + tangente.z },
            { p.x - tangente.x, 1.58f, p.z - tangente.z },
            TinteBateo(COLOR_VENTANA_BATEO, p)
        );
        rlEnd();
    }

    // Cupula con gajos y ranura que gira con el rumbo del telescopio.
    Vector3 centroCupula = { c.x, 2.6f, c.z };
    EsferaBateo(centroCupula, { 4.4f, 4.4f, 4.4f }, 6, 16, Color{ 214, 220, 232, 255 }, 0.0f, 1.5708f, 0.0f, true);

    float rumbo = RumboCupulaBateo(t);
    Vector3 rumboDir = { std::sin(rumbo), 0.0f, -std::cos(rumbo) };
    Vector3 lateral = { std::cos(rumbo), 0.0f, std::sin(rumbo) };
    Color ranura = TinteBateo(Color{ 16, 18, 34, 255 }, centroCupula);
    rlBegin(RL_QUADS);
    for (int k = 0; k < 8; k++)
    {
        float th0 = 0.05f + 1.45f * (float)k / 8.0f;
        float th1 = 0.05f + 1.45f * (float)(k + 1) / 8.0f;
        float ancho0 = 0.85f;
        float ancho1 = 0.85f;
        Vector3 a0 = SumaBateo(centroCupula, SumaBateo(EscalaBateo(rumboDir, std::cos(th0) * 4.43f), Vector3{ 0.0f, std::sin(th0) * 4.43f, 0.0f }));
        Vector3 a1 = SumaBateo(centroCupula, SumaBateo(EscalaBateo(rumboDir, std::cos(th1) * 4.43f), Vector3{ 0.0f, std::sin(th1) * 4.43f, 0.0f }));
        QuadBateo(
            SumaBateo(a0, EscalaBateo(lateral, -ancho0)),
            SumaBateo(a0, EscalaBateo(lateral, ancho0)),
            SumaBateo(a1, EscalaBateo(lateral, ancho1)),
            SumaBateo(a1, EscalaBateo(lateral, -ancho1)),
            ranura
        );
    }
    rlEnd();

    // Telescopio: tubo con anillo dorado en la boca y contrapeso.
    Vector3 pivote = { c.x, 3.6f, c.z };
    Vector3 boca = BocaTelescopioBateo(d, t);
    Vector3 dir = NormalizarBateo(RestaBateo(boca, pivote));
    Vector3 cola = SumaBateo(pivote, EscalaBateo(dir, -1.6f));
    PrismaEjeBateo(cola, boca, 0.75f, 0.9f, 12, Color{ 226, 230, 240, 255 }, 0.0f, 0.0f, true);
    PrismaEjeBateo(SumaBateo(boca, EscalaBateo(dir, -0.5f)), SumaBateo(boca, EscalaBateo(dir, 0.05f)), 1.0f, 1.0f, 12, Color{ 230, 180, 76, 255 }, 0.0f, 0.2f, true);
    PrismaEjeBateo(SumaBateo(pivote, EscalaBateo(dir, 1.5f)), SumaBateo(pivote, EscalaBateo(dir, 2.6f)), 0.98f, 0.98f, 12, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);

    // Antena y baliza en la punta de la cupula.
    VigaBateo({ c.x + 1.2f, 6.6f, c.z + 1.6f }, { c.x + 1.2f, 9.2f, c.z + 1.6f }, 0.06f, COLOR_CASCO_BATEO);
}


static Vector3 CentroRadarBateo(const DisposicionBateo& d)
{
    return { d.mitadCubierta + 7.5f, 0.0f, -17.0f };
}


// Torre reticulada con plato de radar que gira.
static void DibujarRadarBateo(const DisposicionBateo& d, float t)
{
    Vector3 c = CentroRadarBateo(d);

    PrismaEjeBateo({ c.x, -1.0f, c.z }, { c.x, -0.05f, c.z }, 3.4f, 3.4f, 16, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);
    CuboNieblaBateo({ (d.mitadCubierta + c.x - 3.3f) * 0.5f, -0.2f, c.z }, { c.x - 3.3f - d.mitadCubierta, 0.3f, 2.0f }, COLOR_PLACA_OSCURA_BATEO);

    const float alto = 9.0f;
    Vector3 patas[4] = { { -1.4f, 0.0f, -1.4f }, { 1.4f, 0.0f, -1.4f }, { 1.4f, 0.0f, 1.4f }, { -1.4f, 0.0f, 1.4f } };

    for (int k = 0; k < 4; k++)
    {
        Vector3 abajo = { c.x + patas[k].x, 0.0f, c.z + patas[k].z };
        Vector3 arriba = { c.x + patas[k].x * 0.3f, alto, c.z + patas[k].z * 0.3f };
        VigaBateo(abajo, arriba, 0.1f, COLOR_CASCO_BATEO);

        // Diagonales entre patas.
        Vector3 siguiente = patas[(k + 1) % 4];
        for (int n = 0; n < 4; n++)
        {
            float u0 = (float)n / 4.0f;
            float u1 = (float)(n + 1) / 4.0f;
            float e0 = 1.0f - 0.7f * u0;
            float e1 = 1.0f - 0.7f * u1;
            Vector3 a = { c.x + patas[k].x * e0, alto * u0, c.z + patas[k].z * e0 };
            Vector3 b = { c.x + siguiente.x * e1, alto * u1, c.z + siguiente.z * e1 };
            VigaBateo(a, b, 0.05f, COLOR_CASCO_BATEO);
        }
    }

    // Plato: cono abierto que gira y mira al cielo.
    float giro = t * 0.7f;
    Vector3 cabeza = { c.x, alto + 0.4f, c.z };
    Vector3 mira = NormalizarBateo({ std::cos(giro), 0.7f, std::sin(giro) });
    CuboNieblaBateo(cabeza, { 0.9f, 0.6f, 0.9f }, COLOR_CASCO_OSCURO_BATEO);
    Vector3 fondo = SumaBateo(cabeza, EscalaBateo(mira, 0.4f));
    Vector3 borde = SumaBateo(cabeza, EscalaBateo(mira, 1.1f));
    PrismaEjeBateo(fondo, borde, 0.4f, 2.1f, 18, Color{ 168, 176, 196, 255 }, giro, 0.0f, true);
    VigaBateo(fondo, SumaBateo(cabeza, EscalaBateo(mira, 2.4f)), 0.06f, COLOR_CASCO_OSCURO_BATEO);
}


// Brazo de atraque que sale del borde del fondo con una lanzadera.
static const Vector3 PUNTA_BRAZO_BATEO = { -7.0f, -3.6f, -64.0f };


static void DibujarBrazoAtraqueBateo(float t)
{
    (void)t;
    Vector3 a = { -7.0f, -1.6f, Z_FONDO_CUBIERTA_BATEO };
    Vector3 b = PUNTA_BRAZO_BATEO;
    Vector3 esquinas[4] = { { -1.0f, 0.8f, 0.0f }, { 1.0f, 0.8f, 0.0f }, { 1.0f, -0.8f, 0.0f }, { -1.0f, -0.8f, 0.0f } };

    for (int k = 0; k < 4; k++)
    {
        VigaBateo(SumaBateo(a, esquinas[k]), SumaBateo(b, esquinas[k]), 0.1f, COLOR_CASCO_BATEO);
    }

    const int TRAMOS = 9;
    for (int n = 0; n < TRAMOS; n++)
    {
        float u0 = (float)n / (float)TRAMOS;
        float u1 = (float)(n + 1) / (float)TRAMOS;
        Vector3 p0 = SumaBateo(a, EscalaBateo(RestaBateo(b, a), u0));
        Vector3 p1 = SumaBateo(a, EscalaBateo(RestaBateo(b, a), u1));
        VigaBateo(SumaBateo(p0, esquinas[0]), SumaBateo(p1, esquinas[1]), 0.05f, COLOR_CASCO_BATEO);
        VigaBateo(SumaBateo(p0, esquinas[3]), SumaBateo(p1, esquinas[2]), 0.05f, COLOR_CASCO_BATEO);
        VigaBateo(SumaBateo(p0, esquinas[1]), SumaBateo(p1, esquinas[2]), 0.05f, COLOR_CASCO_BATEO);
        VigaBateo(SumaBateo(p0, esquinas[0]), SumaBateo(p1, esquinas[3]), 0.05f, COLOR_CASCO_BATEO);
    }

    // Anillo de atraque y lanzadera amarrada.
    Vector3 anillo = { b.x, b.y, b.z - 0.6f };
    PrismaEjeBateo(anillo, { anillo.x, anillo.y, anillo.z - 0.8f }, 2.0f, 2.0f, 16, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);

    Vector3 nave = { b.x, b.y, b.z - 6.4f };
    // MODELO FUTURO: lanzadera de la estacion.
    CuboNieblaBateo(nave, { 3.0f, 2.2f, 9.0f }, Color{ 226, 228, 236, 255 });
    CuboNieblaBateo({ nave.x, nave.y + 1.2f, nave.z + 0.4f }, { 2.0f, 0.4f, 6.0f }, COLOR_CASCO_BATEO);
    CuboNieblaBateo({ nave.x, nave.y - 0.3f, nave.z }, { 9.0f, 0.25f, 3.6f }, COLOR_CASCO_BATEO);
    CuboNieblaBateo({ nave.x, nave.y + 0.1f, nave.z - 3.0f }, { 3.1f, 0.6f, 1.2f }, COLOR_FRANJA_BATEO);
    PrismaEjeBateo({ nave.x, nave.y, nave.z - 4.5f }, { nave.x, nave.y, nave.z - 6.8f }, 1.2f, 0.2f, 8, Color{ 226, 228, 236, 255 }, 0.0f, 0.0f, true);
    for (int lado = -1; lado <= 1; lado += 2)
    {
        PrismaEjeBateo({ nave.x + (float)lado * 0.8f, nave.y, nave.z + 4.4f }, { nave.x + (float)lado * 0.8f, nave.y, nave.z + 5.2f }, 0.55f, 0.7f, 8, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);
    }
}


//==================================================
// VISUAL: CUBIERTA (ARENA) Y BORDES
//==================================================

static void DibujarCubiertaBateo(const DisposicionBateo& d, bool detalle)
{
    float H = d.mitadCubierta;
    float zf = Z_FRENTE_CUBIERTA_BATEO;
    float zb = Z_FONDO_CUBIERTA_BATEO;
    float y = Y_CUBIERTA_BATEO;
    const float LADO = 2.0f;

    // Placas de casco de 2 x 2 con brillo variable; la franja de los
    // bateadores es chapa mas clara.
    int columnas = (int)std::ceil(2.0f * H / LADO);
    int filas = (int)std::ceil((zf - zb) / LADO);

    rlBegin(RL_QUADS);
    for (int ix = 0; ix < columnas; ix++)
    {
        float xa = -H + (float)ix * LADO;
        float xb = xa + LADO > H ? H : xa + LADO;

        for (int iz = 0; iz < filas; iz++)
        {
            float za = zf - (float)iz * LADO;
            float zc = za - LADO < zb ? zb : za - LADO;
            float cz = (za + zc) * 0.5f;
            Color base = cz > -2.6f ? COLOR_PLACA_BATEO : COLOR_PLACA_OSCURA_BATEO;
            float variacion = 0.9f + 0.18f * Ruido01Bateo(ix * 37 + iz, 11);
            QuadNieblaBateo({ xa, y, za }, { xb, y, za }, { xb, y, zc }, { xa, y, zc }, TonoBateo(base, variacion));
        }
    }
    rlEnd();

    // Juntas entre placas.
    for (int ix = 1; ix < columnas; ix++)
    {
        float x = -H + (float)ix * LADO;
        TrazoSueloBateo(x, zf, x, zb, y + 0.004f, 0.07f, TinteBateo(COLOR_JUNTA_BATEO, { x, 0.0f, -12.0f }));
    }

    for (int iz = 1; iz < filas; iz++)
    {
        float z = zf - (float)iz * LADO;
        TrazoSueloBateo(-H, z, H, z, y + 0.004f, 0.07f, TinteBateo(COLOR_JUNTA_BATEO, { 0.0f, 0.0f, z }));
    }

    // Remaches en las esquinas de las placas.
    if (detalle)
    {
        rlBegin(RL_QUADS);
        for (int ix = 0; ix <= columnas; ix++)
        {
            for (int iz = 0; iz <= filas; iz++)
            {
                float x = -H + (float)ix * LADO;
                float z = zf - (float)iz * LADO;
                Color remache = TinteBateo(COLOR_CASCO_BATEO, { x, 0.0f, z });

                for (int s = -1; s <= 1; s += 2)
                {
                    float rx = x + (float)s * 0.22f;
                    float rz = z - 0.22f;
                    QuadBateo({ rx - 0.05f, y + 0.006f, rz + 0.05f }, { rx + 0.05f, y + 0.006f, rz + 0.05f }, { rx + 0.05f, y + 0.006f, rz - 0.05f }, { rx - 0.05f, y + 0.006f, rz - 0.05f }, remache);
                }
            }
        }
        rlEnd();
    }

    // Franja de peligro naranja y oscura junto al borde (lados y fondo).
    rlBegin(RL_QUADS);
    for (int s = -1; s <= 1; s += 2)
    {
        float xi = (float)s * (H - 0.75f);
        float xe = (float)s * (H - 0.3f);
        int n = 0;

        for (float z = zf; z > zb + 0.6f; z -= 0.6f)
        {
            Color c = n % 2 == 0 ? COLOR_FRANJA_BATEO : Color{ 40, 40, 52, 255 };
            QuadNieblaBateo({ xi, y + 0.008f, z }, { xe, y + 0.008f, z - 0.3f }, { xe, y + 0.008f, z - 0.9f }, { xi, y + 0.008f, z - 0.6f }, c);
            n++;
        }
    }
    {
        int n = 0;
        for (float x = -H + 0.3f; x < H - 0.9f; x += 0.6f)
        {
            Color c = n % 2 == 0 ? COLOR_FRANJA_BATEO : Color{ 40, 40, 52, 255 };
            float z0 = zb + 0.75f;
            float z1 = zb + 0.3f;
            QuadNieblaBateo({ x, y + 0.008f, z0 }, { x + 0.6f, y + 0.008f, z0 }, { x + 0.9f, y + 0.008f, z1 }, { x + 0.3f, y + 0.008f, z1 }, c);
            n++;
        }
    }
    rlEnd();

    // Bordillo y espesor del casco con su franja y ventanas encendidas.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * (H - 0.15f);
        CuboNieblaBateo({ x, 0.1f, (zf + zb) * 0.5f }, { 0.3f, 0.26f, zf - zb }, COLOR_CASCO_BATEO);

        rlBegin(RL_QUADS);
        float xc = (float)s * H;
        QuadNieblaBateo({ xc, y, zf }, { xc, y, zb }, { xc, -1.8f, zb }, { xc, -1.8f, zf }, TonoBateo(COLOR_CASCO_OSCURO_BATEO, 0.85f));
        QuadNieblaBateo({ xc + (float)s * 0.01f, -0.35f, zf }, { xc + (float)s * 0.01f, -0.35f, zb }, { xc + (float)s * 0.01f, -0.55f, zb }, { xc + (float)s * 0.01f, -0.55f, zf }, COLOR_FRANJA_BATEO);

        for (float z = zf - 1.5f; z > zb + 1.0f; z -= 2.5f)
        {
            Vector3 p = { xc + (float)s * 0.012f, -1.05f, z };
            QuadBateo({ p.x, -0.9f, z }, { p.x, -0.9f, z - 0.9f }, { p.x, -1.25f, z - 0.9f }, { p.x, -1.25f, z }, TinteBateo(COLOR_VENTANA_BATEO, p));
        }
        rlEnd();
    }

    CuboNieblaBateo({ 0.0f, 0.1f, zb + 0.15f }, { 2.0f * H, 0.26f, 0.3f }, COLOR_CASCO_BATEO);
    rlBegin(RL_QUADS);
    QuadNieblaBateo({ -H, y, zb }, { H, y, zb }, { H, -1.8f, zb }, { -H, -1.8f, zb }, TonoBateo(COLOR_CASCO_OSCURO_BATEO, 0.7f));
    QuadNieblaBateo({ -H, -0.35f, zb - 0.01f }, { H, -0.35f, zb - 0.01f }, { H, -0.55f, zb - 0.01f }, { -H, -0.55f, zb - 0.01f }, COLOR_FRANJA_BATEO);
    rlEnd();

    // Barandas en los lados y el fondo, con postes cada 3 unidades.
    Color baranda = COLOR_CASCO_BATEO;
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * (H - 0.15f);
        VigaBateo({ x, 1.15f, zf }, { x, 1.15f, zb + 0.15f }, 0.06f, baranda);
        VigaBateo({ x, 0.62f, zf }, { x, 0.62f, zb + 0.15f }, 0.04f, baranda);

        for (float z = zf - 1.0f; z > zb; z -= 3.0f)
        {
            VigaBateo({ x, 0.2f, z }, { x, 1.2f, z }, 0.07f, baranda);
        }
    }

    VigaBateo({ -H + 0.15f, 1.15f, zb + 0.15f }, { H - 0.15f, 1.15f, zb + 0.15f }, 0.06f, baranda);
    VigaBateo({ -H + 0.15f, 0.62f, zb + 0.15f }, { H - 0.15f, 0.62f, zb + 0.15f }, 0.04f, baranda);
    for (float x = -H + 0.15f; x < H; x += 3.0f)
    {
        VigaBateo({ x, 0.2f, zb + 0.15f }, { x, 1.2f, zb + 0.15f }, 0.07f, baranda);
    }

    // Reticulado bajo la cubierta, visible por los costados.
    if (detalle)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            float xa = (float)s * (H - 0.5f);
            float xb = (float)s * (H - 3.5f);
            VigaBateo({ xb, -5.0f, zf }, { xb, -5.0f, zb }, 0.22f, COLOR_CASCO_OSCURO_BATEO);

            for (float z = zf - 2.0f; z > zb; z -= 6.0f)
            {
                VigaBateo({ xa, -1.8f, z }, { xb, -5.0f, z - 3.0f }, 0.12f, COLOR_CASCO_OSCURO_BATEO);
                VigaBateo({ xa, -1.8f, z - 6.0f }, { xb, -5.0f, z - 3.0f }, 0.12f, COLOR_CASCO_OSCURO_BATEO);
            }
        }
    }
}


//==================================================
// VISUAL: UTILERIA (ANILLO CERCANO)
//==================================================

static void CajaCargaBateo(Vector3 base, Vector3 t, float giro, Color color)
{
    Vector3 c = { base.x, base.y + t.y * 0.5f, base.z };
    Color tinte = TinteBateo(color, c);
    Color marco = TinteBateo(COLOR_CASCO_BATEO, c);

    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    CuboBateo({ 0.0f, 0.0f, 0.0f }, t, tinte);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            CuboBateo({ (float)sx * (t.x * 0.5f - 0.05f), 0.0f, (float)sz * (t.z * 0.5f - 0.05f) }, { 0.12f, t.y + 0.02f, 0.12f }, marco);
        }
    }

    CuboBateo({ 0.0f, t.y * 0.2f, 0.0f }, { t.x + 0.03f, 0.12f, t.z + 0.03f }, TinteBateo(COLOR_FRANJA_BATEO, c));
    rlPopMatrix();
}


static void TanqueBateo(Vector3 base, float radio, float alto, Color color)
{
    Vector3 pie = { base.x, base.y + 0.25f, base.z };
    Vector3 tope = { base.x, base.y + alto, base.z };
    PrismaEjeBateo(base, pie, radio * 0.8f, radio * 0.8f, 10, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);
    PrismaEjeBateo(pie, tope, radio, radio, 14, color, 0.0f, 0.0f, true);
    EsferaBateo(tope, { radio, radio * 0.55f, radio }, 4, 14, color, 0.0f, 1.5708f, 0.0f, true);
    PrismaEjeBateo({ base.x, base.y + alto * 0.35f, base.z }, { base.x, base.y + alto * 0.35f + 0.2f, base.z }, radio * 1.04f, radio * 1.04f, 14, COLOR_FRANJA_BATEO, 0.0f, 0.0f, true);
    PrismaEjeBateo({ base.x, base.y + alto * 0.78f, base.z }, { base.x, base.y + alto * 0.78f + 0.12f, base.z }, radio * 1.04f, radio * 1.04f, 14, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);

    Vector3 valvula = { base.x, tope.y + radio * 0.55f, base.z };
    PrismaEjeBateo(valvula, { valvula.x, valvula.y + 0.3f, valvula.z }, 0.08f, 0.08f, 6, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);
    CuboNieblaBateo({ valvula.x, valvula.y + 0.34f, valvula.z }, { 0.4f, 0.06f, 0.06f }, COLOR_FRANJA_BATEO);
}


// Consola de control con pantalla de barras que suben y bajan.
static void ConsolaBateo(Vector3 base, float giro, float t, int semilla)
{
    Color casco = TinteBateo(COLOR_CASCO_OSCURO_BATEO, base);
    Color tapa = TinteBateo(COLOR_CASCO_BATEO, base);

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    CuboBateo({ 0.0f, 0.45f, 0.0f }, { 1.5f, 0.9f, 0.7f }, casco);

    rlPushMatrix();
    rlTranslatef(0.0f, 0.98f, 0.1f);
    rlRotatef(30.0f, 1.0f, 0.0f, 0.0f);
    CuboBateo({ 0.0f, 0.0f, 0.0f }, { 1.6f, 0.12f, 0.95f }, tapa);

    rlBegin(RL_QUADS);
    QuadBateo({ -0.66f, 0.065f, 0.4f }, { 0.66f, 0.065f, 0.4f }, { 0.66f, 0.065f, -0.4f }, { -0.66f, 0.065f, -0.4f }, Color{ 16, 34, 52, 255 });

    for (int b = 0; b < 6; b++)
    {
        float alto = 0.15f + 0.55f * (0.5f + 0.5f * std::sin(t * 2.2f + (float)b * 1.3f + (float)semilla));
        float x0 = -0.56f + (float)b * 0.19f;
        Color barra = b % 3 == 0 ? Color{ 255, 190, 110, 255 } : Color{ 110, 200, 255, 255 };
        QuadBateo({ x0, 0.07f, 0.32f }, { x0 + 0.12f, 0.07f, 0.32f }, { x0 + 0.12f, 0.07f, 0.32f - alto }, { x0, 0.07f, 0.32f - alto }, barra);
    }
    rlEnd();
    rlPopMatrix();

    // Botonera.
    for (int b = 0; b < 4; b++)
    {
        Color luz = (int)(t * 2.0f + (float)(b + semilla)) % 3 == 0 ? Color{ 255, 120, 90, 255 } : Color{ 120, 230, 160, 255 };
        CuboBateo({ -0.5f + (float)b * 0.33f, 0.92f, 0.37f }, { 0.12f, 0.06f, 0.06f }, luz);
    }

    rlPopMatrix();
}


// Carro con la carga de meteoritos que alimenta los canones.
static void CarroMeteoritosBateo(Vector3 base, float giro)
{
    Color casco = TinteBateo(Color{ 90, 104, 130, 255 }, base);
    Color oscuro = TinteBateo(COLOR_CASCO_OSCURO_BATEO, base);

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            PrismaEjeBateo({ (float)sx * 0.72f, 0.24f, (float)sz * 0.85f }, { (float)sx * 0.9f, 0.24f, (float)sz * 0.85f }, 0.24f, 0.24f, 10, oscuro);
        }
    }

    CuboBateo({ 0.0f, 0.52f, 0.0f }, { 1.6f, 0.14f, 2.3f }, oscuro);
    CuboBateo({ 0.0f, 0.88f, 1.1f }, { 1.6f, 0.62f, 0.1f }, casco);
    CuboBateo({ 0.0f, 0.88f, -1.1f }, { 1.6f, 0.62f, 0.1f }, casco);
    CuboBateo({ 0.75f, 0.88f, 0.0f }, { 0.1f, 0.62f, 2.3f }, casco);
    CuboBateo({ -0.75f, 0.88f, 0.0f }, { 0.1f, 0.62f, 2.3f }, casco);
    CuboBateo({ 0.0f, 1.0f, 1.16f }, { 1.62f, 0.12f, 0.04f }, COLOR_FRANJA_BATEO);

    for (int k = 0; k < 7; k++)
    {
        float x = -0.42f + 0.42f * (float)(k % 3);
        float z = -0.7f + 0.48f * (float)(k / 2);
        float r = 0.26f + 0.1f * Ruido01Bateo(k, 430);
        RocaBateo({ x, 1.0f + 0.08f * (float)(k % 2), z }, { r, r * 0.85f, r }, 60 + k, COLOR_ROCA_BATEO, (float)k);
    }

    // Asa.
    PrismaEjeBateo({ -0.5f, 1.2f, 1.6f }, { 0.5f, 1.2f, 1.6f }, 0.05f, 0.05f, 6, oscuro);
    PrismaEjeBateo({ -0.5f, 1.2f, 1.6f }, { -0.5f, 0.85f, 1.15f }, 0.05f, 0.05f, 6, oscuro);
    PrismaEjeBateo({ 0.5f, 1.2f, 1.6f }, { 0.5f, 0.85f, 1.15f }, 0.05f, 0.05f, 6, oscuro);

    rlPopMatrix();
}


static void BobinaBateo(Vector3 base, float giro)
{
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    Color disco = TinteBateo(COLOR_CASCO_OSCURO_BATEO, base);
    PrismaEjeBateo({ -0.5f, 0.62f, 0.0f }, { -0.4f, 0.62f, 0.0f }, 0.62f, 0.62f, 14, disco);
    PrismaEjeBateo({ 0.4f, 0.62f, 0.0f }, { 0.5f, 0.62f, 0.0f }, 0.62f, 0.62f, 14, disco);
    PrismaEjeBateo({ -0.4f, 0.62f, 0.0f }, { 0.4f, 0.62f, 0.0f }, 0.48f, 0.48f, 14, TinteBateo(Color{ 210, 104, 48, 255 }, base));
    rlPopMatrix();
}


static void MastilAntenaBateo(Vector3 base, float alto)
{
    Vector3 tope = { base.x, base.y + alto, base.z };
    CuboNieblaBateo({ base.x, 0.15f, base.z }, { 0.7f, 0.3f, 0.7f }, COLOR_CASCO_OSCURO_BATEO);
    PrismaEjeBateo(base, tope, 0.1f, 0.06f, 6, COLOR_CASCO_BATEO, 0.0f, 0.0f, true);
    VigaBateo({ base.x - 0.6f, base.y + alto * 0.7f, base.z }, { base.x + 0.6f, base.y + alto * 0.7f, base.z }, 0.04f, COLOR_CASCO_BATEO);
    VigaBateo({ base.x - 0.4f, base.y + alto * 0.85f, base.z }, { base.x + 0.4f, base.y + alto * 0.85f, base.z }, 0.04f, COLOR_CASCO_BATEO);
    EsferaBateo(tope, { 0.14f, 0.14f, 0.14f }, 3, 8, Color{ 255, 70, 60, 255 }, -1.5708f, 1.5708f, 0.8f);
}


// Poste de foco: base, mastil, brazo y cabezal que apunta a los carriles.
static void FocoBateo(const DisposicionBateo& d, int indice, float encendido)
{
    Vector3 base = BaseFocoBateo(d, indice);
    Vector3 cabeza = CabezaFocoBateo(d, indice);
    Vector3 charco = CharcoFocoBateo(d, indice);
    float s = LadoFocoBateo(indice);

    CuboNieblaBateo({ base.x, 0.2f, base.z }, { 0.9f, 0.4f, 0.9f }, COLOR_CASCO_OSCURO_BATEO);
    PrismaEjeBateo({ base.x, 0.4f, base.z }, { base.x, cabeza.y + 0.2f, base.z }, 0.16f, 0.12f, 8, COLOR_CASCO_BATEO, 0.0f, 0.0f, true);
    VigaBateo({ base.x, cabeza.y + 0.15f, base.z }, { cabeza.x + s * 0.2f, cabeza.y + 0.15f, cabeza.z }, 0.07f, COLOR_CASCO_BATEO);

    Vector3 dir = NormalizarBateo(RestaBateo(charco, cabeza));
    PrismaEjeBateo(SumaBateo(cabeza, EscalaBateo(dir, -0.35f)), SumaBateo(cabeza, EscalaBateo(dir, 0.25f)), 0.38f, 0.5f, 8, COLOR_CASCO_OSCURO_BATEO, 0.0f, 0.0f, true);
    Color lente = MezclaBateo(Color{ 60, 62, 76, 255 }, COLOR_LUZ_BATEO, encendido);
    PrismaEjeBateo(SumaBateo(cabeza, EscalaBateo(dir, 0.25f)), SumaBateo(cabeza, EscalaBateo(dir, 0.28f)), 0.44f, 0.44f, 8, lente, 0.0f, 1.0f);
}


// Dron de mantenimiento que flota sobre el margen.
static Vector3 PosicionDronBateo(const DisposicionBateo& d, float t)
{
    return { -d.margen + 0.6f * std::sin(t * 0.5f), 3.4f + 0.25f * std::sin(t * 1.7f), -8.5f + 1.2f * std::sin(t * 0.31f) };
}


static void DronBateo(Vector3 p, float t)
{
    Color casco = TinteBateo(COLOR_CASCO_BATEO, p);
    CuboGiradoBateo(p, { 0.7f, 0.26f, 0.7f }, t * 20.0f, casco);
    CuboGiradoBateo({ p.x, p.y - 0.2f, p.z }, { 0.3f, 0.16f, 0.3f }, t * 20.0f, TinteBateo(COLOR_CASCO_OSCURO_BATEO, p));

    for (int k = 0; k < 4; k++)
    {
        float a = 0.785f + 1.5708f * (float)k + t * 0.35f;
        Vector3 brazo = { p.x + std::cos(a) * 0.62f, p.y + 0.08f, p.z + std::sin(a) * 0.62f };
        VigaBateo(p, brazo, 0.04f, COLOR_CASCO_OSCURO_BATEO);
        AnilloSueloBateo({ brazo.x, brazo.y + 0.05f, brazo.z }, 0.0f, 0.3f, Color{ 140, 150, 170, 150 }, 10);
    }
}


static void DibujarUtileriaBateo(const MinijuegoBateoMeteorico& m, const DisposicionBateo& d, float t, int paso)
{
    (void)paso;

    for (int i = 0; i < CANTIDAD_FOCOS_BATEO; i++)
    {
        FocoBateo(d, i, EncendidoFocoBateo(m, i));
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        float xm = s * d.margen;

        // Pila de cajas de carga junto a los bateadores.
        CajaCargaBateo({ xm + s * 0.4f, 0.0f, 3.4f }, { 1.6f, 1.2f, 1.4f }, 8.0f * s, Color{ 72, 118, 128, 255 });
        CajaCargaBateo({ xm - s * 1.2f, 0.0f, 4.6f }, { 1.2f, 0.9f, 1.1f }, -14.0f * s, Color{ 176, 112, 72, 255 });
        CajaCargaBateo({ xm + s * 0.3f, 1.2f, 3.3f }, { 1.1f, 0.8f, 1.0f }, 30.0f * s, Color{ 140, 146, 160, 255 });

        // Consola mirando al carril.
        ConsolaBateo({ xm - s * 0.4f, 0.0f, -0.6f }, 90.0f * s - 20.0f * s, t, lado + 2);

        // Tanques de oxigeno con caneria.
        TanqueBateo({ xm + s * 0.8f, 0.0f, -8.8f }, 0.75f, 2.4f, Color{ 222, 226, 234, 255 });
        TanqueBateo({ xm - s * 0.7f, 0.0f, -9.6f }, 0.6f, 1.8f, Color{ 222, 226, 234, 255 });
        VigaBateo({ xm + s * 0.8f, 0.5f, -8.8f }, { s * (d.mitadCubierta - 0.4f), 0.5f, -8.8f }, 0.07f, COLOR_FRANJA_BATEO);

        // Carro de meteoritos para los canones.
        CarroMeteoritosBateo({ xm, 0.0f, -13.8f }, 90.0f + 12.0f * s);

        // Bobina de cable y antena en la esquina del fondo.
        BobinaBateo({ xm + s * 0.5f, 0.0f, -25.5f }, 30.0f * s);
        CajaCargaBateo({ xm - s * 0.8f, 0.0f, -28.5f }, { 1.3f, 1.0f, 1.3f }, 20.0f * s, Color{ 72, 118, 128, 255 });
        MastilAntenaBateo({ s * (d.mitadCubierta - 1.0f), 0.0f, -31.8f }, 6.5f);
    }

    DronBateo(PosicionDronBateo(d, t), t);
}


//==================================================
// VISUAL: PIEZAS JUGABLES (GLB COMPARTIDOS Y FALLBACK POR PIEZA)
//==================================================
// Los pivotes y la escala del paquete se conservan: carril, campo, canon,
// meteoritos, bate y aro siguen la geometria logica exacta.
//==================================================

static void DibujarMeteorito(
    Vector3 posicion,
    TipoMeteoritoBateo tipo,
    float t,
    float escala
)
{
    float pulso = 0.5f + 0.5f * std::sin(t * 14.0f);
    // Una sombra para la superficie, nunca para cada halo o detalle. El
    // helper GLB evita la sombra automatica de tamano humano de DrawModelEx.
    float radio = (tipo == METEORITO_NORMAL ? 0.45f : 0.5f) * escala;
    if (radio >= 0.10f * EscalaUmbralSombraPropRetro())
        DibujarSombraRetroCircular(posicion,radio*0.92f,radio*0.72f,
            0.018f,0.27f,LadosSombraPropRetro());

    if (tipo == METEORITO_DORADO)
    {
        if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_METEORITO_DORADO,
            posicion, 0, {0,1,0}, {escala,escala,escala}))
            (DrawSphere)(posicion, 0.5f * escala, Color{ 255, 205, 60, 255 });
    }
    else if (tipo == METEORITO_ROJO)
    {
        float tamano = (1.0f + 0.12f * pulso) * escala;
        if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_METEORITO_ROJO,
            posicion, 0, {0,1,0}, {tamano,tamano,tamano}))
            (DrawSphere)(posicion, (0.5f + 0.06f * pulso) * escala, Color{ 220, 40, 40, 255 });
    }
    else
    {
        if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_METEORITO_NORMAL,
            posicion, 0, {0,1,0}, {escala,escala,escala}))
        {
            (DrawSphere)(posicion, 0.45f * escala, Color{ 176, 110, 72, 255 });
            (DrawSphere)({ posicion.x + 0.1f, posicion.y + 0.1f, posicion.z }, 0.2f * escala, Color{ 222, 150, 100, 255 });
        }
    }
}


// Corona de peligro del meteorito rojo: dientes rojos y blancos que giran
// alrededor (se lee aunque el meteorito sea chico en pantalla).
static void CoronaPeligroBateo(Vector3 p, float t)
{
    const int DIENTES = 10;
    float giro = t * 3.0f;
    float r0 = 0.72f;
    float r1 = 0.98f + 0.06f * std::sin(t * 14.0f);

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < DIENTES; k++)
    {
        float a0 = giro + 6.2832f * (float)k / (float)DIENTES;
        float a1 = a0 + 6.2832f / (float)DIENTES * 0.5f;
        float am = (a0 + a1) * 0.5f;
        Vector3 b0 = SumaBateo(p, SumaBateo(EscalaBateo(frenteUBateo, std::cos(a0) * r0), EscalaBateo(frenteVBateo, std::sin(a0) * r0)));
        Vector3 b1 = SumaBateo(p, SumaBateo(EscalaBateo(frenteUBateo, std::cos(a1) * r0), EscalaBateo(frenteVBateo, std::sin(a1) * r0)));
        Vector3 punta = SumaBateo(p, SumaBateo(EscalaBateo(frenteUBateo, std::cos(am) * r1), EscalaBateo(frenteVBateo, std::sin(am) * r1)));
        Color c = k % 2 == 0 ? Color{ 255, 56, 44, 255 } : Color{ 255, 236, 226, 255 };
        VerticeBateo(b0, c);
        VerticeBateo(b1, c);
        VerticeBateo(punta, c);
    }
    rlEnd();
}


// Corona del dorado: rayos finos dorados y blancos que giran despacio (se
// distingue del rojo por la forma, no solo por el color).
static void CoronaDoradaBateo(Vector3 p, float t)
{
    const int RAYOS = 8;
    float giro = -t * 1.6f;

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < RAYOS; k++)
    {
        float a = giro + 6.2832f * (float)k / (float)RAYOS;
        float ancho = 0.16f;
        float r0 = 0.62f;
        float r1 = (k % 2 == 0 ? 1.25f : 0.95f) + 0.08f * std::sin(t * 9.0f + (float)k);
        Vector3 b0 = SumaBateo(p, SumaBateo(EscalaBateo(frenteUBateo, std::cos(a - ancho) * r0), EscalaBateo(frenteVBateo, std::sin(a - ancho) * r0)));
        Vector3 b1 = SumaBateo(p, SumaBateo(EscalaBateo(frenteUBateo, std::cos(a + ancho) * r0), EscalaBateo(frenteVBateo, std::sin(a + ancho) * r0)));
        Vector3 punta = SumaBateo(p, SumaBateo(EscalaBateo(frenteUBateo, std::cos(a) * r1), EscalaBateo(frenteVBateo, std::sin(a) * r1)));
        Color c = k % 2 == 0 ? Color{ 255, 214, 86, 255 } : Color{ 255, 248, 214, 255 };
        VerticeBateo(b0, c);
        VerticeBateo(b1, c);
        VerticeBateo(punta, c);
    }
    rlEnd();
}


static void DibujarCarril(
    const MinijuegoBateoMeteorico& m,
    int indice,
    const JugadorPrueba& jugador,
    const Participante& participante,
    bool mostrarDebug
)
{
    const EstadoJugadorBateo& e = m.estadosJugadores[indice];
    Color colorJugador = COLORES_JUGADORES_BATEO[indice % MAX_PARTICIPANTES];
    float x = e.carrilX;
    float t = m.tiempoAnimacion;

    // Plataforma del bateador y campo de aterrizaje con sus anillos.
    if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_CARRIL,
        {x,0,0}, 0, {0,1,0}, {1,1,1}, colorJugador))
    {
        DrawCube({ x, -0.2f, -5.0f }, ANCHO_CARRIL + 0.4f, 0.4f, 20.0f, Color{ 62, 68, 92, 255 });
        for (float lado : {-1.0f, 1.0f})
            DrawCube({x + lado * (ANCHO_CARRIL * 0.5f + 0.1f),0.02f,-5},
                0.16f,0.06f,20.0f,colorJugador);
    }
    if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_CAMPO,
        {x,0,0}, 0, {0,1,0}, {1,1,1}, colorJugador))
    {
        DrawCube({ x, -0.25f, -22.0f }, ANCHO_CARRIL + 0.4f, 0.3f, 16.0f, Color{ 44, 50, 74, 255 });
        for (float lado : {-1.0f, 1.0f})
            DrawCube({x + lado * (ANCHO_CARRIL * 0.5f + 0.1f),0.02f,-23},
                0.16f,0.06f,16.0f,colorJugador);

        // Anillos puntuados (100 / 60 / 30) sobre el campo.
        float zc = -DISTANCIA_CENTRO_ANILLOS;
        DrawCube({ x, 0.03f, zc }, ANCHO_CARRIL, 0.02f, 16.0f, Color{ 52, 96, 170, 255 });
        DrawCube({ x, 0.045f, zc }, ANCHO_CARRIL, 0.02f, 8.4f, Color{ 150, 90, 210, 255 });
        DrawCube({ x, 0.06f, zc }, ANCHO_CARRIL, 0.02f, 3.6f, Color{ 255, 210, 80, 255 });
    }

    // Canon sobre su torre apuntando al punto dulce.
    Vector3 boca = { x, Y_CANON, Z_CANON };
    Vector3 destino = { x, Y_PUNTO_DULCE, Z_PUNTO_DULCE };
    Vector3 direccion =
    {
        destino.x - boca.x,
        destino.y - boca.y,
        destino.z - boca.z
    };
    float longitud = std::sqrt(direccion.x * direccion.x + direccion.y * direccion.y + direccion.z * direccion.z);
    direccion = { direccion.x / longitud, direccion.y / longitud, direccion.z / longitud };

    if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_CANON_BASE, {x,0,Z_CANON-0.8f}))
    {
        DrawCube({ x, 2.0f, Z_CANON - 0.8f }, 2.0f, 4.0f, 2.0f, Color{ 78, 84, 110, 255 });
        DrawCube({ x, 0.2f, Z_CANON - 0.8f }, 2.8f, 0.4f, 2.8f, Color{ 58, 64, 88, 255 });
        DrawSphere({ x, Y_CANON - 0.3f, Z_CANON - 0.4f }, 0.95f, Color{ 120, 128, 154, 255 });
    }
    // +Y local hacia la direccion original del canon, sin matriz rlgl adicional.
    Vector3 eje = Vector3CrossProduct({0,1,0}, direccion);
    if (Vector3Length(eje) < 0.001f) eje = {1,0,0};
    float anguloCanon = std::acos(Acotar(direccion.y,-1,1)) * RAD2DEG;
    if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_CANON_TUBO,
        {x,Y_CANON-0.3f,Z_CANON-0.4f}, anguloCanon, eje))
    {
        DrawCylinderEx(
            { boca.x, boca.y - 0.3f, boca.z - 0.4f },
            { boca.x + direccion.x * 2.2f, boca.y - 0.3f + direccion.y * 2.2f, boca.z - 0.4f + direccion.z * 2.2f },
            0.55f, 0.7f, 12, Color{ 150, 158, 184, 255 }
        );
    }

    // Banda del color del jugador en la torre del canon (marca su carril
    // tambien de lejos).
    CuboBateo({ x, 0.55f, Z_CANON - 0.8f }, { 2.3f, 0.16f, 2.3f }, colorJugador);

    // Punto dulce: aro brillante donde debe estar el meteorito al golpear.
    float pulso = 0.5f + 0.5f * std::sin(t * 8.0f);
    float escalaAro = (0.55f + 0.06f * pulso) / 0.55f;
    if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_ARO,
        {x,Y_PUNTO_DULCE,Z_PUNTO_DULCE}, 0, {0,1,0}, {escalaAro,escalaAro,1}))
    {
        DrawCircle3D(
            { x, Y_PUNTO_DULCE, Z_PUNTO_DULCE },
            0.55f + 0.06f * pulso, { 0.0f, 0.0f, 1.0f }, 0.0f,
            Color{ 120, 255, 220, 255 }
        );
    }

    if (mostrarDebug)
    {
        DrawSphereWires({ x, Y_PUNTO_DULCE, Z_PUNTO_DULCE }, 0.5f, 6, 6, LIME);
    }

    // Meteorito segun el estado del lanzamiento.
    if (m.fase == FASE_BATEO_JUGANDO && m.ronda < LANZAMIENTOS_BATEO)
    {
        Vector3 posicion{};

        if (PosicionMeteoritoVisibleBateo(m, e, 0.0f, posicion))
        {
            DibujarMeteorito(posicion, e.tipoActual, t, 1.0f);

            if (e.tipoActual == METEORITO_ROJO)
            {
                CoronaPeligroBateo(posicion, t);
            }
            else if (e.tipoActual == METEORITO_DORADO)
            {
                CoronaDoradaBateo(posicion, t);
            }
        }
    }

    // Jugador (pies sobre la plataforma) y bate.
    JugadorPrueba visualJugador = jugador;
    visualJugador.posicion = { x, jugador.tamano.y * 0.5f, Z_JUGADOR };
    visualJugador.direccionMirada = { 0.0f, 0.0f, -1.0f };
    visualJugador.velocidad = { 0.0f, 0.0f, 0.0f };
    visualJugador.enSuelo = true;
    visualJugador.cayendo = false;

    Participante visual = participante;
    visual.conectado = true;
    visual.color = colorJugador;

    DibujarJugadorModeloCompartido(visualJugador, visual);

    float angulo = -70.0f;

    if (e.tiempoSwing > 0.0f)
    {
        float avance = DURACION_SWING - e.tiempoSwing;
        angulo = -60.0f + Acotar(avance / 0.12f, 0.0f, 1.0f) * 140.0f;
    }

    if (!DibujarModeloBateoMeteoricoRetro3D(MODELO_BATEO_BATE,
        {x+0.4f,1.05f,Z_JUGADOR}, angulo, {0,1,0}, {1,1,1}, colorJugador))
    {
        rlPushMatrix();
        rlTranslatef(x + 0.4f, 1.05f, Z_JUGADOR);
        rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
        DrawCylinderEx({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -0.5f }, 0.06f, 0.07f, 8, Color{ 120, 80, 50, 255 });
        DrawCylinderEx({ 0.0f, 0.0f, -0.5f }, { 0.0f, 0.0f, -1.45f }, 0.1f, 0.16f, 10, colorJugador);
        rlPopMatrix();
    }
}


//==================================================
// VISUAL: CALCOS TRANSLUCIDOS Y LUCES ADITIVAS
//==================================================

static void DibujarCalcosCarrilBateo(const MinijuegoBateoMeteorico& m, int indice, float t)
{
    const EstadoJugadorBateo& e = m.estadosJugadores[indice];
    Color colorJugador = COLORES_JUGADORES_BATEO[indice % MAX_PARTICIPANTES];
    float x = e.carrilX;
    float y = Y_CALCO_BATEO;

    // Plataforma circular del bateador con el anillo de su color.
    float destello = m.destelloJugador[indice];
    ResplandorSueloBateo({ x, y, Z_JUGADOR }, 1.5f, Color{ 8, 8, 20, 130 });
    AnilloSueloBateo({ x, y + 0.002f, Z_JUGADOR }, 0.0f, 1.0f, AlfaBateo(TonoBateo(colorJugador, 0.45f), 0.55f), 24);
    AnilloSueloBateo({ x, y + 0.004f, Z_JUGADOR }, 1.0f, 1.16f + 0.12f * destello, colorJugador, 28);
    AnilloSueloBateo({ x, y + 0.004f, Z_JUGADOR }, 0.5f, 0.56f, AlfaBateo(colorJugador, 0.6f), 20);

    // Linea de bateo a la altura del aro y franja de la ventana de golpe
    // (desde donde el golpe ya cuenta hasta el punto dulce).
    TrazoSueloBateo(x - 2.3f, Z_PUNTO_DULCE, x + 2.3f, Z_PUNTO_DULCE, y + 0.006f, 0.12f, AlfaBateo(COLOR_ACENTO_BATEO, 0.8f));

    for (int k = -1; k <= 1; k += 2)
    {
        TrazoSueloBateo(x + (float)k * 2.3f, Z_PUNTO_DULCE - 0.3f, x + (float)k * 2.3f, Z_PUNTO_DULCE + 0.3f, y + 0.006f, 0.12f, AlfaBateo(COLOR_ACENTO_BATEO, 0.8f));
    }

    bool mostrarVentana =
        m.fase == FASE_BATEO_JUGANDO &&
        m.ronda < LANZAMIENTOS_BATEO &&
        (m.etapa == ETAPA_BATEO_AVISO || e.estado == LANZAMIENTO_EN_VUELO);

    if (mostrarVentana)
    {
        const LanzamientoBateo& l = m.lanzamientos[m.ronda];
        float inicio = l.duracion - VENTANA_GOLPE;
        if (inicio < 0.0f) inicio = 0.0f;
        Vector3 a = PosicionMeteoritoEntrante(x, l, inicio);
        float latido = 0.8f + 0.2f * std::sin(t * 6.0f);

        rlBegin(RL_QUADS);
        const int TRAMOS = 6;
        for (int k = 0; k < TRAMOS; k++)
        {
            float u0 = (float)k / (float)TRAMOS;
            float u1 = (float)(k + 1) / (float)TRAMOS;
            float z0 = a.z + (Z_PUNTO_DULCE - a.z) * u0;
            float z1 = a.z + (Z_PUNTO_DULCE - a.z) * u1;
            float x0 = a.x + (x - a.x) * u0;
            float x1 = a.x + (x - a.x) * u1;
            Color c0 = AlfaBateo(COLOR_ACENTO_BATEO, 0.32f * u0 * latido);
            Color c1 = AlfaBateo(COLOR_ACENTO_BATEO, 0.32f * u1 * latido);
            VerticeBateo({ x0 - 0.8f, y + 0.003f, z0 }, c0);
            VerticeBateo({ x0 + 0.8f, y + 0.003f, z0 }, c0);
            VerticeBateo({ x1 + 0.8f, y + 0.003f, z1 }, c1);
            VerticeBateo({ x1 - 0.8f, y + 0.003f, z1 }, c1);
        }
        rlEnd();

        // Huella de la trayectoria: puntos desde el canon hasta el aro.
        rlBegin(RL_QUADS);
        for (int k = 1; k < 16; k++)
        {
            Vector3 p = PosicionMeteoritoEntrante(x, l, l.duracion * (float)k / 16.0f);
            float r = 0.07f;
            Color c = AlfaBateo(COLOR_ACENTO_BATEO, 0.35f);
            QuadBateo({ p.x - r, y + 0.003f, p.z + r }, { p.x + r, y + 0.003f, p.z + r }, { p.x + r, y + 0.003f, p.z - r }, { p.x - r, y + 0.003f, p.z - r }, c);
        }
        rlEnd();
    }

    // Lineas de los anillos de puntaje sobre el campo (100 / 60 / 30).
    float zc = -DISTANCIA_CENTRO_ANILLOS;
    const float mitades[3] = { 1.8f, 4.2f, 8.0f };
    const Color colores[3] = { COLOR_ANILLO_100_BATEO, COLOR_ANILLO_60_BATEO, COLOR_ANILLO_30_BATEO };

    for (int b = 0; b < 3; b++)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            float z = zc + (float)s * mitades[b];
            TrazoSueloBateo(x - 2.5f, z, x + 2.5f, z, y + 0.01f, 0.14f, AlfaBateo(colores[b], 0.85f));
        }
    }

    // Diana del centro del 100.
    AnilloSueloBateo({ x, y + 0.012f, zc }, 0.75f, 0.92f, AlfaBateo(COLOR_ANILLO_100_BATEO, 0.85f), 24);
    AnilloSueloBateo({ x, y + 0.012f, zc }, 0.0f, 0.3f, AlfaBateo(WHITE, 0.6f), 16);

    // Crater donde cayo el ultimo meteorito.
    if (e.estado == LANZAMIENTO_ATERRIZADO)
    {
        Vector3 p = e.puntoAterrizaje;
        ResplandorSueloBateo({ p.x, y + 0.014f, p.z }, 1.1f, Color{ 10, 8, 18, 170 });
        AnilloSueloBateo({ p.x, y + 0.016f, p.z }, 0.55f, 0.75f, Color{ 40, 32, 36, 200 }, 14);
    }
}


// Franjas de puntaje de lado a lado de la cubierta: se leen tambien entre
// los carriles y a los costados de los canones, que tapan el centro.
static void DibujarFranjasPuntajeBateo(const MinijuegoBateoMeteorico& m)
{
    DisposicionBateo d = CalcularDisposicionBateo(m);
    float x0 = -d.mitadCarriles - 1.7f;
    float x1 = d.mitadCarriles + 1.7f;
    float zc = -DISTANCIA_CENTRO_ANILLOS;
    float y = Y_CALCO_BATEO - 0.01f;

    struct FranjaBateo { float a, b; Color c; };
    const FranjaBateo franjas[] =
    {
        { zc - 1.8f, zc + 1.8f, COLOR_ANILLO_100_BATEO },
        { zc + 1.8f, zc + 4.2f, COLOR_ANILLO_60_BATEO },
        { zc - 4.2f, zc - 1.8f, COLOR_ANILLO_60_BATEO },
        { zc + 4.2f, zc + 8.0f, COLOR_ANILLO_30_BATEO },
        { zc - 8.0f, zc - 4.2f, COLOR_ANILLO_30_BATEO }
    };

    rlBegin(RL_QUADS);
    for (const FranjaBateo& f : franjas)
    {
        QuadBateo({ x0, y, f.b }, { x1, y, f.b }, { x1, y, f.a }, { x0, y, f.a }, AlfaBateo(f.c, 0.16f));

        // En los margenes, fuera de los carriles, la franja es plena.
        for (int s = -1; s <= 1; s += 2)
        {
            float xa = (float)s * d.mitadCarriles;
            float xb = (float)s * (d.mitadCarriles + 1.7f);
            QuadBateo({ xa, y + 0.003f, f.b }, { xb, y + 0.003f, f.b }, { xb, y + 0.003f, f.a }, { xa, y + 0.003f, f.a }, AlfaBateo(f.c, 0.62f));
        }
    }
    rlEnd();
}


static void DibujarTranslucidosBateo(const MinijuegoBateoMeteorico& m, int limite, float t)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    DibujarFranjasPuntajeBateo(m);

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo)
        {
            DibujarCalcosCarrilBateo(m, i, t);
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarChispasSolidasBateo(const MinijuegoBateoMeteorico& m)
{
    for (int i = 0; i < MAX_CHISPAS_BATEO; i++)
    {
        const ChispaBateo& c = m.chispas[i];
        if (!c.activa || c.brilla) continue;

        float vida = Limitar01Bateo(c.vida / c.vidaMaxima);
        float tam = c.tamano * (0.5f + 0.5f * vida);
        CuboGiradoBateo(c.posicion, { tam, tam * 0.7f, tam }, c.giro, c.color);
    }
}


static void DibujarLucesMeteoritoBateo(const MinijuegoBateoMeteorico& m, const EstadoJugadorBateo& e, float t)
{
    Vector3 cabeza{};

    if (!PosicionMeteoritoVisibleBateo(m, e, 0.0f, cabeza))
    {
        return;
    }

    Color c = ColorTipoMeteoritoBateo(e.tipoActual);

    // Estela de fuego: discos cada vez mas chicos hacia atras.
    for (int k = 9; k >= 1; k--)
    {
        Vector3 p{};
        if (!PosicionMeteoritoVisibleBateo(m, e, 0.028f * (float)k, p)) continue;
        float u = (float)k / 10.0f;
        Color estela = MezclaBateo(c, Color{ 255, 90, 40, 255 }, u * 0.6f);
        if (e.tipoActual == METEORITO_DORADO) estela = MezclaBateo(c, Color{ 255, 250, 210, 255 }, 0.3f);
        ResplandorFrenteBateo(p, 0.62f * (1.0f - u * 0.75f), AlfaBateo(estela, 0.55f * (1.0f - u)));
    }

    if (e.tipoActual == METEORITO_DORADO)
    {
        float titila = 0.75f + 0.25f * std::sin(t * 11.0f);
        ResplandorFrenteBateo(cabeza, 1.5f * titila, AlfaBateo(c, 0.55f));
        ResplandorFrenteBateo(cabeza, 0.7f, AlfaBateo(Color{ 255, 250, 220, 255 }, 0.6f));
        DestelloCruzBateo(cabeza, 2.6f * titila, AlfaBateo(Color{ 255, 244, 200, 255 }, 0.8f), t * 1.5f);

        // Chispitas doradas que quedan atras.
        for (int k = 0; k < 5; k++)
        {
            Vector3 p{};
            if (!PosicionMeteoritoVisibleBateo(m, e, 0.05f + 0.045f * (float)k, p)) continue;
            float a = t * 9.0f + (float)k * 2.4f;
            p.x += std::cos(a) * 0.35f;
            p.y += std::sin(a) * 0.35f;
            ResplandorFrenteBateo(p, 0.16f, AlfaBateo(Color{ 255, 236, 160, 255 }, 0.8f));
        }
    }
    else if (e.tipoActual == METEORITO_ROJO)
    {
        float latido = 0.5f + 0.5f * std::sin(t * 14.0f);
        ResplandorFrenteBateo(cabeza, 1.3f + 0.3f * latido, AlfaBateo(c, 0.45f + 0.25f * latido));
        ResplandorFrenteBateo(cabeza, 0.6f, AlfaBateo(Color{ 255, 140, 110, 255 }, 0.4f));
    }
    else
    {
        ResplandorFrenteBateo(cabeza, 1.0f, AlfaBateo(c, 0.35f));
    }

    // Luz que el meteorito echa sobre el carril.
    ResplandorSueloBateo({ cabeza.x, Y_CALCO_BATEO + 0.02f, cabeza.z }, 1.4f, AlfaBateo(c, 0.16f));
}


static void DibujarLucesCarrilBateo(const MinijuegoBateoMeteorico& m, int indice, float t)
{
    const EstadoJugadorBateo& e = m.estadosJugadores[indice];
    Color colorJugador = COLORES_JUGADORES_BATEO[indice % MAX_PARTICIPANTES];
    float x = e.carrilX;
    float y = Y_CALCO_BATEO + 0.02f;

    // Bordes del carril con brillo de su color.
    for (int s = -1; s <= 1; s += 2)
    {
        float xb = x + (float)s * (ANCHO_CARRIL * 0.5f + 0.1f);
        TrazoSueloBateo(xb, 4.6f, xb, -29.6f, y, 0.42f, AlfaBateo(colorJugador, 0.10f));
        TrazoSueloBateo(xb, 4.6f, xb, -29.6f, y + 0.002f, 0.1f, AlfaBateo(colorJugador, 0.22f));
    }

    float destello = m.destelloJugador[indice];
    ResplandorSueloBateo({ x, y, Z_JUGADOR }, 1.8f + 0.8f * destello, AlfaBateo(colorJugador, 0.16f + 0.35f * destello));

    if (destello > 0.0f)
    {
        ResplandorFrenteBateo({ x, 1.1f, Z_JUGADOR }, 1.2f + 0.8f * (1.0f - destello), AlfaBateo(MezclaBateo(colorJugador, WHITE, 0.4f), 0.45f * destello));
    }

    // El aro se enciende a medida que el meteorito entra en la ventana.
    float cerca = 0.0f;

    if (m.fase == FASE_BATEO_JUGANDO && m.ronda < LANZAMIENTOS_BATEO && e.estado == LANZAMIENTO_EN_VUELO)
    {
        float desfase = std::fabs(e.reloj - m.lanzamientos[m.ronda].duracion);
        cerca = Limitar01Bateo(1.0f - desfase / VENTANA_GOLPE);
    }

    Vector3 aro = { x, Y_PUNTO_DULCE, Z_PUNTO_DULCE };
    float pulso = 0.5f + 0.5f * std::sin(t * 8.0f);
    ResplandorFrenteBateo(aro, 1.0f + 0.5f * cerca, AlfaBateo(COLOR_ACENTO_BATEO, 0.12f + 0.06f * pulso + 0.4f * cerca));
    AnilloPlanoBateo(aro, frenteUBateo, frenteVBateo, 0.6f, 0.78f + 0.2f * cerca, AlfaBateo(COLOR_ACENTO_BATEO, 0.3f + 0.5f * cerca), AlfaBateo(COLOR_ACENTO_BATEO, 0.0f), 24);
    ResplandorSueloBateo({ x, y, Z_PUNTO_DULCE }, 1.4f, AlfaBateo(COLOR_ACENTO_BATEO, 0.10f + 0.22f * cerca));

    // Diana del 100: brilla suave.
    ResplandorSueloBateo({ x, y, -DISTANCIA_CENTRO_ANILLOS }, 2.4f, AlfaBateo(COLOR_ANILLO_100_BATEO, 0.10f + 0.04f * pulso));

    // Boca del canon: se carga con el color del meteorito que viene y
    // fogonazo al disparar.
    Vector3 destino = { x, Y_PUNTO_DULCE, Z_PUNTO_DULCE };
    Vector3 tubo = { x, Y_CANON - 0.3f, Z_CANON - 0.4f };
    Vector3 dir = NormalizarBateo(RestaBateo(destino, { x, Y_CANON, Z_CANON }));
    Vector3 boca = SumaBateo(tubo, EscalaBateo(dir, 2.4f));

    if (m.fase == FASE_BATEO_JUGANDO && m.ronda < LANZAMIENTOS_BATEO)
    {
        Color tipo = ColorTipoMeteoritoBateo(e.tipos[m.ronda]);

        if (m.etapa == ETAPA_BATEO_AVISO)
        {
            float carga = Limitar01Bateo(m.tiempoEtapa / DURACION_AVISO_BATEO);
            float vibra = 0.9f + 0.1f * std::sin(t * 30.0f);
            ResplandorFrenteBateo(boca, (0.3f + 1.0f * carga) * vibra, AlfaBateo(tipo, 0.35f + 0.4f * carga));
            ResplandorFrenteBateo(boca, 0.25f + 0.3f * carga, AlfaBateo(WHITE, 0.5f * carga));
        }
        else if (m.etapa == ETAPA_BATEO_VUELO && m.tiempoEtapa < 0.3f)
        {
            float k = m.tiempoEtapa / 0.3f;
            ResplandorFrenteBateo(boca, 1.0f + 1.6f * k, AlfaBateo(tipo, 0.8f * (1.0f - k)));
            ResplandorSueloBateo({ x, y, Z_CANON + 1.5f }, 3.0f, AlfaBateo(tipo, 0.3f * (1.0f - k)));
        }
    }

    DibujarLucesMeteoritoBateo(m, e, t);

    // Destello del bate al pegarle.
    if (e.tiempoImpacto > 0.0f && e.estado != LANZAMIENTO_EXPLOTADO)
    {
        float k = 1.0f - e.tiempoImpacto / 0.5f;
        ResplandorFrenteBateo(e.puntoGolpe, 0.5f + 1.4f * k, AlfaBateo(WHITE, 0.85f * (1.0f - k)));
        DestelloCruzBateo(e.puntoGolpe, 3.0f * (1.0f - k * 0.5f), AlfaBateo(Color{ 255, 250, 230, 255 }, 0.8f * (1.0f - k)), 0.4f);
    }

    // Explosion del meteorito rojo en las manos del bateador.
    if (e.estado == LANZAMIENTO_EXPLOTADO && e.tiempoPostEvento < 0.9f && m.fase == FASE_BATEO_JUGANDO)
    {
        float k = e.tiempoPostEvento / 0.9f;
        ResplandorFrenteBateo(e.puntoAterrizaje, 1.0f + 3.4f * k, AlfaBateo(Color{ 255, 120, 50, 255 }, 0.8f * (1.0f - k)));
        ResplandorFrenteBateo(e.puntoAterrizaje, 0.6f + 1.6f * k, AlfaBateo(Color{ 255, 230, 180, 255 }, 0.8f * (1.0f - k)));
        ResplandorSueloBateo({ e.puntoAterrizaje.x, y, e.puntoAterrizaje.z }, 2.0f + 2.5f * k, AlfaBateo(Color{ 255, 80, 40, 255 }, 0.4f * (1.0f - k)));
    }

    // Aterrizaje: onda del color del anillo y brasas del crater.
    if (e.estado == LANZAMIENTO_ATERRIZADO)
    {
        Color c = ColorPuntosBateo(PuntosPorDistancia(e.distancia));
        Vector3 p = { e.puntoAterrizaje.x, y + 0.01f, e.puntoAterrizaje.z };

        if (e.tiempoPostEvento < 1.2f)
        {
            float k = e.tiempoPostEvento / 1.2f;
            AnilloSueloBateo(p, 0.4f + 3.0f * k, 0.7f + 3.4f * k, AlfaBateo(c, 0.7f * (1.0f - k)), 32);
            ResplandorFrenteBateo({ p.x, 0.8f, p.z }, 1.2f + 1.8f * k, AlfaBateo(c, 0.6f * (1.0f - k)));
        }

        ResplandorSueloBateo(p, 1.2f, AlfaBateo(Color{ 255, 130, 60, 255 }, 0.22f + 0.06f * std::sin(t * 5.0f + (float)indice)));
    }
}


static void DibujarLucesBateo(
    const MinijuegoBateoMeteorico& m,
    const DisposicionBateo& d,
    int limite,
    float t,
    float ta,
    bool decoracion,
    bool efectos
)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Atmosfera del planeta y de la luna (el disco queda tapado por el
    // cuerpo y solo asoma el borde).
    if (efectos)
    {
        ResplandorFrenteBateo(CENTRO_PLANETA_BATEO, RADIO_PLANETA_BATEO * 1.2f, Color{ 255, 190, 140, 70 });
        if (decoracion) ResplandorFrenteBateo(CENTRO_LUNA_BATEO, RADIO_LUNA_BATEO * 1.5f, Color{ 190, 200, 255, 40 });
    }

    // Focos: halo, cono y charco calido sobre los carriles.
    for (int i = 0; i < CANTIDAD_FOCOS_BATEO; i++)
    {
        float encendido = EncendidoFocoBateo(m, i);
        if (!efectos || !decoracion || encendido <= 0.0f) continue;

        Vector3 cabeza = CabezaFocoBateo(d, i);
        Vector3 charco = CharcoFocoBateo(d, i);
        Vector3 dir = NormalizarBateo(RestaBateo(charco, cabeza));
        Vector3 lente = SumaBateo(cabeza, EscalaBateo(dir, 0.3f));
        float parpadeo = (0.94f + 0.06f * std::sin(t * 9.0f + (float)i * 2.1f)) * encendido;

        ResplandorFrenteBateo(lente, 1.4f * parpadeo, AlfaBateo(COLOR_LUZ_BATEO, 0.35f));
        ResplandorFrenteBateo(lente, 0.55f * parpadeo, AlfaBateo(WHITE, 0.7f));
        ConoLuzBateo(lente, charco, 3.4f, AlfaBateo(COLOR_LUZ_BATEO, 0.07f * encendido));

        for (int r = 0; r < 3; r++)
        {
            ResplandorSueloBateo({ charco.x, 0.1f + 0.002f * (float)r, charco.z }, (2.6f + 1.8f * (float)r) * parpadeo, AlfaBateo(COLOR_LUZ_BATEO, 0.07f * encendido), 1.2f);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo)
        {
            DibujarLucesCarrilBateo(m, i, ta);
        }
    }

    // Balizas que titilan: antenas, cupula, radar y brazo de atraque.
    float rojo = std::sin(t * 3.0f) > 0.2f ? 1.0f : 0.15f;
    for (int lado = -1; lado <= 1 && efectos && decoracion; lado += 2)
    {
        Vector3 punta = { (float)lado * (d.mitadCubierta - 1.0f), 6.5f, -31.8f };
        ResplandorFrenteBateo(punta, 0.9f, AlfaBateo(Color{ 255, 70, 60, 255 }, 0.6f * rojo));
    }

    if (efectos && decoracion)
    {
        Vector3 cupula = CentroCupulaBateo(d);
        ResplandorFrenteBateo({ cupula.x + 1.2f, 9.3f, cupula.z + 1.6f }, 1.0f, AlfaBateo(Color{ 255, 70, 60, 255 }, 0.6f * (1.15f - rojo)));
        Vector3 radar = CentroRadarBateo(d);
        ResplandorFrenteBateo({ radar.x, 9.8f, radar.z }, 1.0f, AlfaBateo(Color{ 255, 70, 60, 255 }, 0.6f * rojo));

        // Ventanas del tambor de la cupula.
        ResplandorFrenteBateo({ cupula.x, 1.5f, cupula.z + 4.2f }, 3.0f, Color{ 255, 200, 140, 26 });

        // Luces de navegacion del brazo y tobera de la lanzadera.
        Vector3 punta = PUNTA_BRAZO_BATEO;
        float verde = std::sin(t * 2.2f + 1.0f) > 0.3f ? 1.0f : 0.1f;
        ResplandorFrenteBateo({ punta.x - 4.5f, punta.y - 0.3f, punta.z - 6.4f }, 1.2f, AlfaBateo(Color{ 255, 70, 60, 255 }, 0.7f * verde));
        ResplandorFrenteBateo({ punta.x + 4.5f, punta.y - 0.3f, punta.z - 6.4f }, 1.2f, AlfaBateo(Color{ 80, 255, 120, 255 }, 0.7f * verde));
        float tobera = 0.85f + 0.15f * std::sin(t * 17.0f);
        for (int lado = -1; lado <= 1; lado += 2)
        {
            ResplandorFrenteBateo({ punta.x + (float)lado * 0.8f, punta.y, punta.z - 1.0f }, 1.3f * tobera, Color{ 120, 170, 255, 120 });
        }

        for (int k = 0; k < 9; k++)
        {
            float u = ((float)k + 0.5f) / 9.0f;
            Vector3 p = { -7.0f, -1.6f + (punta.y + 1.6f) * u, Z_FONDO_CUBIERTA_BATEO + (punta.z - Z_FONDO_CUBIERTA_BATEO) * u };
            float on = std::sin(t * 4.0f - (float)k * 0.7f) > 0.6f ? 1.0f : 0.2f;
            ResplandorFrenteBateo({ p.x, p.y + 0.9f, p.z }, 0.5f, AlfaBateo(Color{ 255, 200, 120, 255 }, 0.5f * on));
        }

        // Dron: ojo y haz hacia el piso.
        Vector3 dron = PosicionDronBateo(d, t);
        ResplandorFrenteBateo({ dron.x, dron.y - 0.25f, dron.z }, 0.45f, AlfaBateo(COLOR_ACENTO_BATEO, 0.7f));
        ConoLuzBateo({ dron.x, dron.y - 0.3f, dron.z }, { dron.x, 0.05f, dron.z }, 0.9f, AlfaBateo(COLOR_ACENTO_BATEO, 0.06f));
        ResplandorSueloBateo({ dron.x, 0.1f, dron.z }, 1.2f, AlfaBateo(COLOR_ACENTO_BATEO, 0.12f));

        // Pantallas de las consolas.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 p = { (float)lado * (d.margen - 0.4f), 1.3f, -0.4f };
            ResplandorFrenteBateo(p, 1.0f, Color{ 110, 200, 255, 40 });
        }
    }

    // Luces de las barandas.
    for (int s = -1; s <= 1 && efectos; s += 2)
    {
        float x = (float)s * (d.mitadCubierta - 0.15f);
        int n = 0;
        for (float z = Z_FRENTE_CUBIERTA_BATEO - 1.0f; z > Z_FONDO_CUBIERTA_BATEO; z -= 3.0f)
        {
            float on = 0.6f + 0.4f * std::sin(t * 2.0f + (float)n * 0.9f);
            ResplandorFrenteBateo({ x, 1.28f, z }, 0.32f, AlfaBateo(Color{ 170, 200, 255, 255 }, 0.5f * on));
            n++;
        }
    }

    // Chispas con brillo.
    for (int i = 0; i < MAX_CHISPAS_BATEO; i++)
    {
        const ChispaBateo& c = m.chispas[i];
        if (!c.activa || !c.brilla) continue;
        float vida = Limitar01Bateo(c.vida / c.vidaMaxima);
        ResplandorFrenteBateo(c.posicion, c.tamano * (1.4f + vida), AlfaBateo(c.color, 0.9f * vida));
    }

    // Polvo estelar que flota sobre la plataforma.
    int motas = efectos ? (int)(48.0f * FactorCalidadGrafica(CalidadParticulas())) : 0;
    for (int k = 0; k < motas; k++)
    {
        float ciclo = 9.0f + 6.0f * Ruido01Bateo(k, 990);
        float u = std::fmod(t / ciclo + Ruido01Bateo(k, 991), 1.0f);
        Vector3 p =
        {
            (Ruido01Bateo(k, 992) - 0.5f) * 2.0f * (d.mitadCubierta + 6.0f) + (u - 0.5f) * 6.0f,
            0.6f + 7.0f * Ruido01Bateo(k, 993) + std::sin(t * 0.7f + (float)k) * 0.4f,
            8.0f - 44.0f * Ruido01Bateo(k, 994)
        };
        float alfa = std::sin(u * 3.1416f);
        Color c = k % 3 == 0 ? Color{ 200, 170, 255, 255 } : Color{ 170, 220, 255, 255 };
        ResplandorFrenteBateo(p, 0.08f, AlfaBateo(c, 0.7f * alfa));
    }

    // Meteoritos sueltos de la lluvia que cruzan cerca del cinturon.
    if (efectos && decoracion)
    {
        for (int k = 0; k < 4; k++)
        {
            float periodo = 5.0f + 3.0f * Ruido01Bateo(k, 995);
            float u = std::fmod(t + Ruido01Bateo(k, 996) * periodo, periodo) / 2.2f;
            if (u >= 1.0f) continue;

            Vector3 inicio = { 60.0f - 40.0f * Ruido01Bateo(k, 997), 30.0f + 10.0f * Ruido01Bateo(k, 998), -70.0f - 50.0f * Ruido01Bateo(k, 999) };
            Vector3 dir = NormalizarBateo({ -1.0f, -0.55f, 0.15f });
            Vector3 cabeza = SumaBateo(inicio, EscalaBateo(dir, u * 90.0f));
            float alfa = std::sin(u * 3.1416f);

            for (int q = 0; q < 8; q++)
            {
                Vector3 p = SumaBateo(cabeza, EscalaBateo(dir, -1.4f * (float)q));
                ResplandorFrenteBateo(p, 1.1f * (1.0f - (float)q / 9.0f), AlfaBateo(Color{ 255, 170, 100, 255 }, 0.5f * alfa * (1.0f - (float)q / 8.0f)));
            }
            ResplandorFrenteBateo(cabeza, 0.7f, AlfaBateo(Color{ 255, 240, 210, 255 }, 0.8f * alfa));
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: CHISPAS, AVISOS Y DETECCION DE EVENTOS
//==================================================

static float AzarVisualBateo(MinijuegoBateoMeteorico& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadChispasBateo(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return n < 1 ? 1 : n;
}


static void EmitirChispaBateo(
    MinijuegoBateoMeteorico& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool brilla
)
{
    // Un hueco libre o, si no hay, la chispa que menos vida le queda.
    int indice = 0;
    float menor = 1000.0f;

    for (int i = 0; i < MAX_CHISPAS_BATEO; i++)
    {
        if (!m.chispas[i].activa)
        {
            indice = i;
            break;
        }

        if (m.chispas[i].vida < menor)
        {
            menor = m.chispas[i].vida;
            indice = i;
        }
    }

    ChispaBateo& c = m.chispas[indice];
    c.activa = true;
    c.posicion = posicion;
    c.velocidad = velocidad;
    c.color = color;
    c.vida = vida;
    c.vidaMaxima = vida;
    c.tamano = tamano;
    c.gravedad = gravedad;
    c.giro = AzarVisualBateo(m, 0.0f, 360.0f);
    c.brilla = brilla;
}


// Estallido esferico de chispas (brilla) o esquirlas (solidas).
static void EstallidoBateo(
    MinijuegoBateoMeteorico& m,
    Vector3 p,
    int base,
    Color a,
    Color b,
    float velocidad,
    float subida,
    float vida,
    float tamano,
    float gravedad,
    bool brilla
)
{
    int n = CantidadChispasBateo(base);

    for (int k = 0; k < n; k++)
    {
        float ang = AzarVisualBateo(m, 0.0f, 6.2832f);
        float v = AzarVisualBateo(m, 0.35f, 1.0f) * velocidad;
        Vector3 vel =
        {
            std::cos(ang) * v,
            AzarVisualBateo(m, 0.2f, 1.0f) * subida,
            std::sin(ang) * v
        };
        Color c = MezclaBateo(a, b, AzarVisualBateo(m, 0.0f, 1.0f));
        EmitirChispaBateo(m, p, vel, c, vida * AzarVisualBateo(m, 0.7f, 1.2f), tamano * AzarVisualBateo(m, 0.7f, 1.3f), gravedad, brilla);
    }
}


static void AgregarAvisoBateo(
    MinijuegoBateoMeteorico& m,
    Vector3 posicion,
    const char* texto,
    Color color,
    bool grande,
    float vida
)
{
    int indice = 0;
    float menor = 1000.0f;

    for (int i = 0; i < MAX_AVISOS_BATEO; i++)
    {
        if (!m.avisos[i].activo)
        {
            indice = i;
            break;
        }

        if (m.avisos[i].vida < menor)
        {
            menor = m.avisos[i].vida;
            indice = i;
        }
    }

    AvisoBateo& a = m.avisos[indice];
    a.activo = true;
    a.posicion = posicion;
    a.vida = vida;
    a.vidaMaxima = vida;
    std::snprintf(a.texto, sizeof(a.texto), "%s", texto);
    a.color = color;
    a.grande = grande;
}


static Vector3 BocaCanonBateo(float x)
{
    Vector3 destino = { x, Y_PUNTO_DULCE, Z_PUNTO_DULCE };
    Vector3 tubo = { x, Y_CANON - 0.3f, Z_CANON - 0.4f };
    Vector3 dir = NormalizarBateo(RestaBateo(destino, { x, Y_CANON, Z_CANON }));
    return SumaBateo(tubo, EscalaBateo(dir, 2.4f));
}


// Lugar de los rotulos de un carril: sobre la linea de bateo, por delante
// del cartelito J# del bateador.
static Vector3 PuntoRotuloCarrilBateo(const EstadoJugadorBateo& e)
{
    return { e.carrilX, 2.3f, Z_PUNTO_DULCE - 2.2f };
}


static void EfectoGolpeBateo(MinijuegoBateoMeteorico& m, int i)
{
    const EstadoJugadorBateo& e = m.estadosJugadores[i];
    Color colorJugador = COLORES_JUGADORES_BATEO[i % MAX_PARTICIPANTES];
    Color tipo = ColorTipoMeteoritoBateo(e.tipoActual);

    m.destelloJugador[i] = 1.0f;
    EstallidoBateo(m, e.puntoGolpe, 14, colorJugador, WHITE, 4.5f, 3.5f, 0.45f, 0.12f, 6.0f, true);
    EstallidoBateo(m, e.puntoGolpe, 8, tipo, Color{ 255, 240, 200, 255 }, 3.0f, 2.5f, 0.5f, 0.1f, 5.0f, true);
    // Esquirlas de roca que se desprenden del meteorito.
    EstallidoBateo(m, e.puntoGolpe, 6, COLOR_ROCA_BATEO, Color{ 150, 110, 80, 255 }, 2.4f, 3.0f, 0.9f, 0.12f, 9.0f, false);

    if (e.calidad >= 0.95f)
    {
        EstallidoBateo(m, e.puntoGolpe, 16, COLOR_ANILLO_100_BATEO, WHITE, 6.0f, 4.0f, 0.6f, 0.14f, 3.0f, true);
    }
}


static void EfectoExplosionBateo(MinijuegoBateoMeteorico& m, int i)
{
    const EstadoJugadorBateo& e = m.estadosJugadores[i];
    Vector3 p = e.puntoAterrizaje;

    EstallidoBateo(m, p, 26, Color{ 255, 70, 40, 255 }, Color{ 255, 220, 120, 255 }, 6.5f, 5.0f, 0.6f, 0.16f, 4.0f, true);
    EstallidoBateo(m, p, 10, Color{ 70, 60, 64, 255 }, Color{ 140, 50, 40, 255 }, 4.0f, 4.0f, 1.1f, 0.16f, 9.0f, false);

    // Humo oscuro que sube despacio.
    int humo = CantidadChispasBateo(8);
    for (int k = 0; k < humo; k++)
    {
        Vector3 vel = { AzarVisualBateo(m, -0.6f, 0.6f), AzarVisualBateo(m, 0.8f, 1.8f), AzarVisualBateo(m, -0.6f, 0.6f) };
        EmitirChispaBateo(m, p, vel, Color{ 74, 66, 80, 255 }, AzarVisualBateo(m, 0.9f, 1.4f), AzarVisualBateo(m, 0.3f, 0.5f), -0.4f, false);
    }

    ActivarTemblorCamaraGeneral(0.25f, 0.3f);
}


static void EfectoAterrizajeBateo(MinijuegoBateoMeteorico& m, int i)
{
    const EstadoJugadorBateo& e = m.estadosJugadores[i];
    Vector3 p = { e.puntoAterrizaje.x, 0.15f, e.puntoAterrizaje.z };
    Color anillo = ColorPuntosBateo(PuntosPorDistancia(e.distancia));
    bool dorado = e.tipoActual == METEORITO_DORADO;

    EstallidoBateo(m, p, 12, anillo, WHITE, 3.6f, 4.5f, 0.6f, 0.13f, 6.0f, true);
    EstallidoBateo(m, p, 10, COLOR_ROCA_BATEO, Color{ 170, 120, 84, 255 }, 3.0f, 4.0f, 1.0f, 0.15f, 10.0f, false);

    if (dorado)
    {
        EstallidoBateo(m, { p.x, 0.6f, p.z }, 24, COLOR_ANILLO_100_BATEO, Color{ 255, 250, 220, 255 }, 5.0f, 6.0f, 0.9f, 0.15f, 4.0f, true);
    }
}


static void AvisoMensajeBateo(MinijuegoBateoMeteorico& m, int i)
{
    const EstadoJugadorBateo& e = m.estadosJugadores[i];
    Vector3 rotulo = PuntoRotuloCarrilBateo(e);
    char texto[24];

    switch (e.mensaje)
    {
        case MENSAJE_BATEO_PERFECTO:
        case MENSAJE_BATEO_BUENO:
        case MENSAJE_BATEO_REGULAR:
        case MENSAJE_BATEO_FLOJO:
        {
            Color c = MezclaBateo(ColorPuntosBateo(PuntosPorDistancia(e.distancia)), WHITE, 0.15f);
            if (e.tipoActual == METEORITO_DORADO)
            {
                std::snprintf(texto, sizeof(texto), "+%d  x2", e.puntosMensaje);
                c = COLOR_ANILLO_100_BATEO;
            }
            else
            {
                std::snprintf(texto, sizeof(texto), "+%d", e.puntosMensaje);
            }
            AgregarAvisoBateo(m, { e.puntoAterrizaje.x, 1.4f, e.puntoAterrizaje.z }, texto, c, true, 1.6f);

            if (e.mensaje == MENSAJE_BATEO_PERFECTO)
            {
                AgregarAvisoBateo(m, rotulo, "¡PERFECTO!", COLOR_ANILLO_100_BATEO, false, 1.4f);
            }
            break;
        }
        case MENSAJE_BATEO_ROJO:
            std::snprintf(texto, sizeof(texto), "%d", e.puntosMensaje);
            AgregarAvisoBateo(m, { e.puntoGolpe.x, e.puntoGolpe.y + 1.2f, e.puntoGolpe.z }, texto, Color{ 255, 96, 80, 255 }, true, 1.6f);
            AgregarAvisoBateo(m, rotulo, "¡ERA ROJO!", Color{ 255, 140, 120, 255 }, false, 1.4f);
            break;
        case MENSAJE_BATEO_FALLO:
            AgregarAvisoBateo(m, rotulo, "¡FALLASTE!", Color{ 210, 214, 228, 255 }, false, 1.2f);
            break;
        case MENSAJE_BATEO_SIN_GOLPE:
            AgregarAvisoBateo(m, rotulo, "SIN GOLPE", Color{ 210, 214, 228, 255 }, false, 1.2f);
            break;
        case MENSAJE_BATEO_EVITADO:
            AgregarAvisoBateo(m, rotulo, "¡BIEN EVITADO!", Color{ 140, 240, 170, 255 }, false, 1.3f);
            break;
        default:
            break;
    }
}


static void EmitirConfetiBateo(MinijuegoBateoMeteorico& m, int limite)
{
    int cantidad = CantidadChispasBateo(36);

    for (int i = 0; i < limite; i++)
    {
        const ResultadoParticipante& r = m.resultado.participantes[i];

        if (!r.participo || r.posicionFinal != 1)
        {
            continue;
        }

        Color colorJugador = COLORES_JUGADORES_BATEO[i % MAX_PARTICIPANTES];
        Vector3 p = { m.estadosJugadores[i].carrilX, 2.2f, Z_JUGADOR };

        for (int k = 0; k < cantidad; k++)
        {
            float a = AzarVisualBateo(m, 0.0f, 6.2832f);
            float v = AzarVisualBateo(m, 0.5f, 2.6f);
            Vector3 vel = { std::cos(a) * v, AzarVisualBateo(m, 3.0f, 6.5f), std::sin(a) * v };
            Color c = k % 3 == 0 ? colorJugador : (k % 3 == 1 ? COLOR_ANILLO_100_BATEO : Color{ 240, 240, 255, 255 });
            EmitirChispaBateo(m, p, vel, c, AzarVisualBateo(m, 1.4f, 2.4f), AzarVisualBateo(m, 0.08f, 0.14f), 3.2f, false);
        }
    }
}


// Foto de la partida antes de un paso: comparandola con la de despues se
// sabe que paso (disparo, golpe, explosion, aterrizaje) sin tocar reglas.
struct InstantaneaBateo
{
    FaseBateoMeteorico fase = FASE_BATEO_PREPARACION;
    EtapaRondaBateo etapa = ETAPA_BATEO_AVISO;
    EstadoLanzamientoBateo estado[MAX_PARTICIPANTES]{};
    float tiempoMensaje[MAX_PARTICIPANTES]{};
};


static void TomarInstantaneaBateo(const MinijuegoBateoMeteorico& m, InstantaneaBateo& s)
{
    s.fase = m.fase;
    s.etapa = m.etapa;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        s.estado[i] = m.estadosJugadores[i].estado;
        s.tiempoMensaje[i] = m.estadosJugadores[i].tiempoMensaje;
    }
}


static void ActualizarVisualBateo(
    MinijuegoBateoMeteorico& m,
    const InstantaneaBateo& s,
    float deltaTime,
    int limite
)
{
    m.tiempoVisual += deltaTime;
    bool explosion = false;

    // Disparo de los canones: fogonazo y humo en cada boca.
    bool disparo = s.etapa == ETAPA_BATEO_AVISO && m.etapa == ETAPA_BATEO_VUELO && m.fase == FASE_BATEO_JUGANDO;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorBateo& e = m.estadosJugadores[i];

        if (disparo)
        {
            Vector3 boca = BocaCanonBateo(e.carrilX);
            EstallidoBateo(m, boca, 8, ColorTipoMeteoritoBateo(e.tipoActual), WHITE, 2.4f, 2.0f, 0.35f, 0.1f, 2.0f, true);

            int humo = CantidadChispasBateo(5);
            for (int k = 0; k < humo; k++)
            {
                Vector3 vel = { AzarVisualBateo(m, -0.5f, 0.5f), AzarVisualBateo(m, 0.4f, 1.1f), AzarVisualBateo(m, 0.3f, 1.4f) };
                EmitirChispaBateo(m, boca, vel, Color{ 150, 150, 170, 255 }, AzarVisualBateo(m, 0.6f, 1.0f), AzarVisualBateo(m, 0.22f, 0.36f), -0.3f, false);
            }
        }

        if (s.estado[i] != e.estado)
        {
            if (e.estado == LANZAMIENTO_GOLPEADO)
            {
                EfectoGolpeBateo(m, i);
            }
            else if (e.estado == LANZAMIENTO_EXPLOTADO)
            {
                EfectoExplosionBateo(m, i);
                explosion = true;
            }
            else if (e.estado == LANZAMIENTO_ATERRIZADO)
            {
                EfectoAterrizajeBateo(m, i);
            }
        }

        if (e.tiempoMensaje > s.tiempoMensaje[i] + 0.5f)
        {
            AvisoMensajeBateo(m, i);
        }

        // El dorado deja chispitas y el rojo suelta brasas en vuelo.
        Vector3 p{};
        if (
            (e.tipoActual == METEORITO_DORADO || e.tipoActual == METEORITO_ROJO) &&
            PosicionMeteoritoVisibleBateo(m, e, 0.0f, p) &&
            AzarVisualBateo(m, 0.0f, 1.0f) < 0.7f * FactorCalidadGrafica(CalidadParticulas())
        )
        {
            bool dorado = e.tipoActual == METEORITO_DORADO;
            Vector3 vel = { AzarVisualBateo(m, -0.8f, 0.8f), AzarVisualBateo(m, -0.2f, 1.0f), AzarVisualBateo(m, -0.8f, 0.8f) };
            Color c = dorado ? Color{ 255, 230, 140, 255 } : Color{ 255, 90, 50, 255 };
            EmitirChispaBateo(m, p, vel, c, 0.45f, dorado ? 0.11f : 0.09f, 1.5f, true);
        }

        if (m.destelloJugador[i] > 0.0f)
        {
            m.destelloJugador[i] -= deltaTime * 2.5f;
            if (m.destelloJugador[i] < 0.0f) m.destelloJugador[i] = 0.0f;
        }
    }

    if (explosion)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_EXPLOSION);
    }

    // Anuncio de inicio y confeti del final.
    if (s.fase == FASE_BATEO_PREPARACION && m.fase == FASE_BATEO_JUGANDO)
    {
        m.tiempoAnuncioInicio = 0.0f;
    }
    else if (m.tiempoAnuncioInicio >= 0.0f)
    {
        m.tiempoAnuncioInicio += deltaTime;
        if (m.tiempoAnuncioInicio > DURACION_ANUNCIO_BATEO) m.tiempoAnuncioInicio = -1.0f;
    }

    if (m.fase == FASE_BATEO_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        float antes = m.tiempoFin;
        m.tiempoFin = m.tiempoFin < 0.0f ? 0.0f : m.tiempoFin + deltaTime;

        if (antes < 0.0f || (antes < 0.8f && m.tiempoFin >= 0.8f))
        {
            EmitirConfetiBateo(m, limite);
        }
    }

    for (int i = 0; i < MAX_CHISPAS_BATEO; i++)
    {
        ChispaBateo& c = m.chispas[i];
        if (!c.activa) continue;

        c.vida -= deltaTime;
        if (c.vida <= 0.0f)
        {
            c.activa = false;
            continue;
        }

        c.velocidad.y -= c.gravedad * deltaTime;
        c.posicion.x += c.velocidad.x * deltaTime;
        c.posicion.y += c.velocidad.y * deltaTime;
        c.posicion.z += c.velocidad.z * deltaTime;
        c.giro += 240.0f * deltaTime;

        if (c.posicion.y < 0.06f)
        {
            c.posicion.y = 0.06f;
            c.velocidad.y = -c.velocidad.y * 0.3f;
            c.velocidad.x *= 0.6f;
            c.velocidad.z *= 0.6f;
        }
    }

    for (int i = 0; i < MAX_AVISOS_BATEO; i++)
    {
        if (!m.avisos[i].activo) continue;
        m.avisos[i].vida -= deltaTime;
        if (m.avisos[i].vida <= 0.0f) m.avisos[i].activo = false;
    }
}


void MinijuegoBateoMeteorico::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    InstantaneaBateo antes;
    TomarInstantaneaBateo(*this, antes);

    ActualizarPartida(deltaTime, jugadores, cantidadMaxima, participantes);

    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    ActualizarVisualBateo(*this, antes, deltaTime, limite);
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoBateoMeteorico::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    const float t = tiempoVisual;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    float movimiento = ObtenerFactorMovimientoCamaraMinijuegos();


    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    Camera3D vista = camara;

    if (fase == FASE_BATEO_PREPARACION && resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Limitar01Bateo((tiempoPreparacion - 0.6f) / (DURACION_PREPARACION_BATEO - 0.6f));
        float suave = u * u * (3.0f - 2.0f * u) * movimiento;
        vista.position.y += 5.0f * suave;
        vista.position.z += 6.0f * suave;
        vista.target.y += 2.0f * suave;
    }

    // Ojo de la niebla y ejes de pantalla para los halos.
    ojoBateo = vista.position;
    Vector3 frente = NormalizarBateo(RestaBateo(vista.target, vista.position));
    frenteUBateo = NormalizarBateo(CruzBateo(frente, { 0.0f, 1.0f, 0.0f }));
    frenteVBateo = CruzBateo(frenteUBateo, frente);

    DisposicionBateo d = CalcularDisposicionBateo(*this);

    // El bucle principal no limpia la pantalla: el fondo pinta todo.
    ClearBackground(Color{ 5, 6, 20, 255 });
    DibujarFondoBateo(t, paso);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte: planeta anillado, luna y cinturon de asteroides.
    DibujarCuerpoCelesteBateo(CENTRO_PLANETA_BATEO, RADIO_PLANETA_BATEO, true, t);
    DibujarAnillosPlanetaBateo();

    if (decoracion)
    {
        DibujarCuerpoCelesteBateo(CENTRO_LUNA_BATEO, RADIO_LUNA_BATEO, false, t);
        DibujarAsteroidesBateo(t, paso);

        // Media distancia: la estacion alrededor de la cubierta.
        DibujarAlaSolarBateo(d, -1.0f, -6.0f, paso);
        DibujarAlaSolarBateo(d, 1.0f, -6.0f, paso);
        DibujarBrazoAtraqueBateo(t);
        DibujarObservatorioBateo(d, t);
        DibujarRadarBateo(d, t);
    }

    // Arena: la cubierta con sus bordes.
    DibujarCubiertaBateo(d, decoracion);

    // Anillo cercano.
    if (decoracion)
    {
        DibujarUtileriaBateo(*this, d, t, paso);
    }

    // Piezas jugables.
    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarCarril(*this, i, jugadores[i], participantes[i], mostrarDebug);
        }
    }

    DibujarChispasSolidasBateo(*this);
    DibujarTranslucidosBateo(*this, limite, tiempoAnimacion);
    DibujarLucesBateo(*this, d, limite, t, tiempoAnimacion, decoracion, efectos);

    rlEnableBackfaceCulling();
    EndMode3D();

    float limiteAbajo = (float)(alto - MargenBarraRondaMinijuegos()) - Px(40.0f);

    // Puntaje de cada anillo, al costado del primer carril.
    if (resultado.estado != RESULTADO_MINIJUEGO_CANCELADO && fase != FASE_BATEO_TERMINADO)
    {
        float xRotulo = -d.mitadCarriles - 0.9f;
        struct RotuloAnilloBateo { float z; const char* texto; Color color; };
        const RotuloAnilloBateo rotulos[] =
        {
            { -DISTANCIA_CENTRO_ANILLOS, "100", COLOR_ANILLO_100_BATEO },
            { -DISTANCIA_CENTRO_ANILLOS + 3.0f, "60", COLOR_ANILLO_60_BATEO },
            { -DISTANCIA_CENTRO_ANILLOS + 6.1f, "30", COLOR_ANILLO_30_BATEO },
            { -DISTANCIA_CENTRO_ANILLOS + 11.0f, "10", COLOR_ANILLO_10_BATEO }
        };

        for (const RotuloAnilloBateo& r : rotulos)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                Vector2 s = GetWorldToScreen({ (float)lado * xRotulo, 0.3f, r.z }, vista);
                DibujarTextoMundoContornoUI(r.texto, s.x, s.y - Px(14.0f), UI_TEXTO, MezclaBateo(r.color, WHITE, 0.2f));
            }
        }
    }

    // Cartelitos J1..J4 sobre cada bateador.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo)
        {
            continue;
        }

        Vector2 p = GetWorldToScreen({ estadosJugadores[i].carrilX, jugadores[i].tamano.y + 0.75f, Z_JUGADOR }, vista);

        char rotulo[8];
        std::snprintf(
            rotulo,
            sizeof(rotulo),
            "J%d",
            participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1
        );

        OpcionesEtiquetaUI op;
        op.borde = COLORES_JUGADORES_BATEO[i % MAX_PARTICIPANTES];
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Aviso del canon: que meteorito viene en cada carril.
    if (fase == FASE_BATEO_JUGANDO && ronda < LANZAMIENTOS_BATEO)
    {
        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            const EstadoJugadorBateo& e = estadosJugadores[i];
            TipoMeteoritoBateo tipo = etapa == ETAPA_BATEO_AVISO ? e.tipos[ronda] : e.tipoActual;
            bool mostrar = etapa == ETAPA_BATEO_AVISO || (etapa == ETAPA_BATEO_VUELO && e.estado == LANZAMIENTO_EN_VUELO);

            if (!mostrar || tipo == METEORITO_NORMAL)
            {
                continue;
            }

            Vector2 s = GetWorldToScreen({ e.carrilX, Y_CANON + 2.0f, Z_CANON - 0.6f }, vista);
            float pulso = 0.75f + 0.25f * std::sin(tiempoAnimacion * 12.0f);

            if (tipo == METEORITO_DORADO)
            {
                DibujarTextoMundoContornoUI("DORADO x2", s.x, s.y, UI_TEXTO, COLOR_ANILLO_100_BATEO, pulso);
            }
            else
            {
                DibujarTextoMundoContornoUI("¡NO LO BATEES!", s.x, s.y, UI_TEXTO, Color{ 255, 110, 96, 255 }, pulso);
            }
        }
    }

    // Rotulos que suben: «+60», «¡PERFECTO!», «-50».
    for (int i = 0; i < MAX_AVISOS_BATEO; i++)
    {
        const AvisoBateo& aviso = avisos[i];
        if (!aviso.activo) continue;

        float u = 1.0f - aviso.vida / aviso.vidaMaxima;
        Vector3 p = { aviso.posicion.x, aviso.posicion.y + u * 1.2f, aviso.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);
        float alfa = aviso.vida < 0.35f ? aviso.vida / 0.35f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;
        if (s.y > limiteAbajo) s.y = limiteAbajo;

        DibujarTextoMundoContornoUI(aviso.texto, s.x, s.y, aviso.grande ? UI_TEXTO_GRANDE : UI_TEXTO, aviso.color, alfa);
    }

    // Anuncios grandes.
    float yAnuncio = (float)alto * 0.27f;

    if (tiempoAnuncioInicio >= 0.0f && fase == FASE_BATEO_JUGANDO)
    {
        float u = tiempoAnuncioInicio / DURACION_ANUNCIO_BATEO;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
        float y = yAnuncio - u * Px(16.0f);
        DibujarTextoMundoContornoUI("¡A BATEAR!", (float)ancho * 0.5f, y, UI_TEXTO_TITULO, COLOR_ACENTO_BATEO, alfa);
        DibujarTextoMundoContornoUI("PEGALE CUANDO EL METEORITO CRUCE EL ARO", (float)ancho * 0.5f, y + Px(52.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }
    else if (fase == FASE_BATEO_JUGANDO && etapa == ETAPA_BATEO_AVISO && ronda < LANZAMIENTOS_BATEO)
    {
        float u = Limitar01Bateo(tiempoEtapa / DURACION_AVISO_BATEO);
        float alfa = u < 0.15f ? u / 0.15f : (u > 0.8f ? (1.0f - u) / 0.2f : 1.0f);
        char texto[32];

        if (ronda == LANZAMIENTOS_BATEO - 1)
        {
            std::snprintf(texto, sizeof(texto), "¡ÚLTIMO LANZAMIENTO!");
        }
        else
        {
            std::snprintf(texto, sizeof(texto), "LANZAMIENTO %d", ronda + 1);
        }

        DibujarTextoMundoContornoUI(texto, (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 236, 232, 255, 255 }, alfa);
    }

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)ancho * 0.5f, (float)alto * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "BATEO METEÓRICO",
        "BATEÁ LOS METEORITOS A LOS ANILLOS: DORADO x2, ROJO NO",
        resultado
    );

    hud.fase = fase == FASE_BATEO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_BATEO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;

    // La partida se mide en lanzamientos, no en segundos.
    int lanzamientoActual = ronda < LANZAMIENTOS_BATEO ? ronda + 1 : LANZAMIENTOS_BATEO;
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "LANZAMIENTO";
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", lanzamientoActual, LANZAMIENTOS_BATEO);
    float avance = (float)ronda;
    if (fase == FASE_BATEO_JUGANDO && etapa == ETAPA_BATEO_PAUSA) avance += 1.0f;
    if (fase == FASE_BATEO_TERMINADO) avance = (float)LANZAMIENTOS_BATEO;
    hud.progresoMedidor = Limitar01Bateo(avance / (float)LANZAMIENTOS_BATEO);

    int mejorPuntaje = -1;
    int conMejorPuntaje = 0;

    for (int i = 0; i < limite; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        const EstadoJugadorBateo& e = estadosJugadores[i];

        if (!h.visible)
        {
            continue;
        }

        if (e.puntos > mejorPuntaje)
        {
            mejorPuntaje = e.puntos;
            conMejorPuntaje = 1;
        }
        else if (e.puntos == mejorPuntaje)
        {
            conMejorPuntaje++;
        }

        std::snprintf(h.valor, sizeof(h.valor), "%d PTS", e.puntos);
        h.valorNumerico = e.puntos;
        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d PTS", e.puntos);
        std::snprintf(h.detalleDesempate, sizeof(h.detalleDesempate), "MEJOR GOLPE %d%%", e.mejorGolpe / 10);

        if (fase == FASE_BATEO_JUGANDO && e.tiempoMensaje > 0.0f && e.mensaje == MENSAJE_BATEO_PERFECTO)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡PERFECTO!");
        }

        if (!JugadorEsBot(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "BATEÁ: %s",
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    if (fase == FASE_BATEO_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (resultado.desenlace == DESENLACE_CON_GANADOR && conMejorPuntaje > 1)
        {
            hud.notaFinal = "EMPATE EN PUNTOS: DESEMPATA EL MEJOR GOLPE";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoBateoMeteorico::ObtenerResultado() const
{
    return resultado;
}
