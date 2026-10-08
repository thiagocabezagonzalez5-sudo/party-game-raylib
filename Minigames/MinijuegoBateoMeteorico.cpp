#include "Minigames/MinijuegoBateoMeteorico.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>


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


static const char* NombreJugadorBateo(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
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

    if (m.cantidadCarriles <= 2)
    {
        m.camara.position = { 0.0f, 7.0f, 12.0f };
        m.camara.target = { 0.0f, 0.2f, -12.0f };
        m.camara.fovy = 50.0f;
    }
    else
    {
        m.camara.position = { 0.0f, 8.0f, 13.0f };
        m.camara.target = { 0.0f, 1.6f, -12.0f };
        m.camara.fovy = 58.0f;
    }
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

void MinijuegoBateoMeteorico::Actualizar(
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
// VISUAL (MODELO FUTURO)
//==================================================
// MODELO FUTURO: reemplazar por GLB la cupula del observatorio con su
// telescopio gigante, el planeta con anillos, los carriles/plataformas de
// roca, los cañones lanzadores, los meteoritos (normal, dorado, rojo) y el
// bate. La logica (tiempos, ventana de golpe, puntaje por distancia) no
// depende de ninguna de estas decoraciones.
//==================================================

static float Hash01(int n)
{
    float v = std::sin((float)n * 12.9898f) * 43758.5453f;

    return v - std::floor(v);
}


static void DibujarCieloNocturno(float t)
{
    // Estrellas dispersas con parpadeo suave.
    for (int i = 0; i < 90; i++)
    {
        float x = -110.0f + 220.0f * Hash01(i * 3 + 1);
        float y = 10.0f + 62.0f * Hash01(i * 3 + 2);
        float brillo = 0.55f + 0.45f * std::sin(t * (1.5f + Hash01(i) * 2.5f) + (float)i);
        float lado = 0.25f + 0.25f * Hash01(i * 3 + 3);

        DrawCube(
            { x, y, -125.0f },
            lado, lado, lado,
            Color{ 255, 255, (unsigned char)(190 + 60 * brillo), (unsigned char)(130 + 120 * brillo) }
        );
    }

    // Constelaciones: 5 grupos de 5 estrellas unidas por lineas.
    for (int g = 0; g < 5; g++)
    {
        float centroX = -80.0f + 40.0f * (float)g;
        float centroY = 30.0f + 18.0f * Hash01(g * 17 + 5);
        Vector3 anterior{};

        for (int k = 0; k < 5; k++)
        {
            Vector3 punto =
            {
                centroX + 10.0f * (Hash01(g * 31 + k * 7 + 11) - 0.5f) * 2.0f,
                centroY + 8.0f * (Hash01(g * 29 + k * 5 + 13) - 0.5f) * 2.0f,
                -124.0f
            };

            DrawSphere(punto, 0.55f, Color{ 190, 215, 255, 255 });

            if (k > 0)
            {
                DrawLine3D(anterior, punto, Color{ 110, 140, 210, 255 });
            }

            anterior = punto;
        }
    }

    // Planeta con anillos al fondo.
    Vector3 planeta = { 34.0f, 34.0f, -112.0f };
    DrawSphere(planeta, 13.0f, Color{ 214, 150, 96, 255 });
    DrawSphere({ planeta.x - 3.0f, planeta.y + 3.0f, planeta.z + 8.0f }, 9.0f, Color{ 232, 178, 120, 255 });

    for (int i = 0; i < 4; i++)
    {
        float radio = 18.0f + 2.2f * (float)i;
        DrawCircle3D(planeta, radio, { 1.0f, 0.2f, 0.0f }, 72.0f, Color{ 190, 170, 140, 255 });
    }

    // Luna.
    DrawSphere({ -62.0f, 52.0f, -118.0f }, 5.0f, Color{ 225, 228, 240, 255 });
}


static void DibujarObservatorio()
{
    // Cupula principal con ranura y telescopio gigante.
    Vector3 cupula = { -36.0f, -3.0f, -64.0f };
    DrawCylinder({ cupula.x, -4.0f, cupula.z }, 15.0f, 15.0f, 11.0f, 20, Color{ 70, 78, 104, 255 });
    DrawSphere({ cupula.x, 7.0f, cupula.z }, 15.0f, Color{ 86, 96, 128, 255 });
    DrawCube({ cupula.x + 4.0f, 15.0f, cupula.z + 9.0f }, 4.0f, 14.0f, 8.0f, Color{ 20, 24, 44, 255 });
    DrawCylinderEx(
        { cupula.x + 4.0f, 12.0f, cupula.z + 9.0f },
        { cupula.x + 16.0f, 34.0f, cupula.z - 6.0f },
        2.2f, 3.2f, 14, Color{ 150, 158, 178, 255 }
    );
    DrawCylinderEx(
        { cupula.x + 14.5f, 31.0f, cupula.z - 3.5f },
        { cupula.x + 17.0f, 35.5f, cupula.z - 7.0f },
        3.4f, 3.6f, 14, Color{ 236, 190, 80, 255 }
    );

    // Segunda cupula pequena.
    DrawCylinder({ 44.0f, -4.0f, -76.0f }, 10.0f, 10.0f, 9.0f, 18, Color{ 66, 74, 98, 255 });
    DrawSphere({ 44.0f, 5.0f, -76.0f }, 10.0f, Color{ 82, 92, 122, 255 });
    DrawCube({ 44.0f, 8.0f, -66.5f }, 2.5f, 10.0f, 2.0f, Color{ 20, 24, 44, 255 });
}


static void DibujarCumbre(float t)
{
    // Roca de la cumbre.
    DrawCube({ 0.0f, -1.2f, -25.0f }, 90.0f, 2.0f, 90.0f, Color{ 38, 42, 58, 255 });
    DrawCube({ 0.0f, -1.0f, -70.0f }, 160.0f, 1.0f, 40.0f, Color{ 30, 34, 50, 255 });

    // Faroles de luz tenue entre carriles (decorativos).
    for (int i = -2; i <= 2; i++)
    {
        float x = (float)i * SEPARACION_CARRILES - SEPARACION_CARRILES * 0.5f + 0.0f;
        float parpadeo = 0.5f + 0.5f * std::sin(t * 3.0f + (float)i);

        DrawCylinder({ x, 0.0f, 3.2f }, 0.08f, 0.1f, 1.8f, 8, Color{ 90, 94, 110, 255 });
        DrawSphere(
            { x, 1.95f, 3.2f },
            0.18f + 0.03f * parpadeo,
            Color{ 255, 230, (unsigned char)(150 + 60 * parpadeo), 255 }
        );
    }
}


static void DibujarMeteorito(
    Vector3 posicion,
    TipoMeteoritoBateo tipo,
    float t,
    float escala
)
{
    float pulso = 0.5f + 0.5f * std::sin(t * 14.0f);

    if (tipo == METEORITO_DORADO)
    {
        DrawSphere(posicion, 0.5f * escala, Color{ 255, 205, 60, 255 });
        DrawSphere(posicion, 0.72f * escala, Color{ 255, 220, 100, (unsigned char)(50 + 60 * pulso) });
    }
    else if (tipo == METEORITO_ROJO)
    {
        DrawSphere(posicion, (0.5f + 0.06f * pulso) * escala, Color{ 220, 40, 40, 255 });
        DrawSphereWires(posicion, (0.72f + 0.08f * pulso) * escala, 6, 6, Color{ 255, 120, 90, 255 });
    }
    else
    {
        DrawSphere(posicion, 0.45f * escala, Color{ 176, 110, 72, 255 });
        DrawSphere({ posicion.x + 0.1f, posicion.y + 0.1f, posicion.z }, 0.2f * escala, Color{ 222, 150, 100, 255 });
    }
}


static Color ColorEstela(TipoMeteoritoBateo tipo)
{
    if (tipo == METEORITO_DORADO) return Color{ 255, 200, 70, 255 };
    if (tipo == METEORITO_ROJO) return Color{ 240, 70, 60, 255 };

    return Color{ 255, 160, 80, 255 };
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

    // Plataforma del bateador y campo de aterrizaje.
    DrawCube({ x, -0.2f, -5.0f }, ANCHO_CARRIL + 0.4f, 0.4f, 20.0f, Color{ 62, 68, 92, 255 });
    DrawCube({ x, -0.25f, -22.0f }, ANCHO_CARRIL + 0.4f, 0.3f, 16.0f, Color{ 44, 50, 74, 255 });
    DrawCube({ x - ANCHO_CARRIL * 0.5f - 0.1f, 0.02f, -13.0f }, 0.16f, 0.06f, 36.0f, colorJugador);
    DrawCube({ x + ANCHO_CARRIL * 0.5f + 0.1f, 0.02f, -13.0f }, 0.16f, 0.06f, 36.0f, colorJugador);

    // Anillos puntuados (100 / 60 / 30) sobre el campo.
    float zc = -DISTANCIA_CENTRO_ANILLOS;
    DrawCube({ x, 0.03f, zc }, ANCHO_CARRIL, 0.02f, 16.0f, Color{ 52, 96, 170, 255 });
    DrawCube({ x, 0.045f, zc }, ANCHO_CARRIL, 0.02f, 8.4f, Color{ 150, 90, 210, 255 });
    DrawCube({ x, 0.06f, zc }, ANCHO_CARRIL, 0.02f, 3.6f, Color{ 255, 210, 80, 255 });

    // Resplandor del anillo en que cayo el ultimo meteorito.
    if (e.estado == LANZAMIENTO_ATERRIZADO && e.tiempoPostEvento < 0.9f)
    {
        float k = e.tiempoPostEvento / 0.9f;
        float radio = (e.tipoActual == METEORITO_DORADO ? 1.6f : 1.0f) * (0.4f + 2.0f * k);
        DrawCircle3D(
            { e.puntoAterrizaje.x, 0.1f, e.puntoAterrizaje.z },
            radio, { 1.0f, 0.0f, 0.0f }, 90.0f,
            Fade(e.tipoActual == METEORITO_DORADO ? GOLD : WHITE, 1.0f - k)
        );
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

    DrawCube({ x, 2.0f, Z_CANON - 0.8f }, 2.0f, 4.0f, 2.0f, Color{ 78, 84, 110, 255 });
    DrawCube({ x, 0.2f, Z_CANON - 0.8f }, 2.8f, 0.4f, 2.8f, Color{ 58, 64, 88, 255 });
    DrawSphere({ x, Y_CANON - 0.3f, Z_CANON - 0.4f }, 0.95f, Color{ 120, 128, 154, 255 });
    DrawCylinderEx(
        { boca.x, boca.y - 0.3f, boca.z - 0.4f },
        { boca.x + direccion.x * 2.2f, boca.y - 0.3f + direccion.y * 2.2f, boca.z - 0.4f + direccion.z * 2.2f },
        0.55f, 0.7f, 12, Color{ 150, 158, 184, 255 }
    );

    // La boca del canon anticipa el tipo de meteorito de la ronda.
    if (m.fase == FASE_BATEO_JUGANDO && m.etapa == ETAPA_BATEO_AVISO && m.ronda < LANZAMIENTOS_BATEO)
    {
        float carga = Acotar(m.tiempoEtapa / DURACION_AVISO_BATEO, 0.0f, 1.0f);
        Color brillo = ColorEstela(e.tipos[m.ronda]);
        DrawSphere(
            { boca.x + direccion.x * 2.4f, boca.y - 0.3f + direccion.y * 2.4f, boca.z - 0.4f + direccion.z * 2.4f },
            0.15f + 0.4f * carga,
            brillo
        );
    }

    // Punto dulce: aro brillante donde debe estar el meteorito al golpear.
    float pulso = 0.5f + 0.5f * std::sin(t * 8.0f);
    DrawCircle3D(
        { x, Y_PUNTO_DULCE, Z_PUNTO_DULCE },
        0.55f + 0.06f * pulso, { 0.0f, 0.0f, 1.0f }, 0.0f,
        Color{ 120, 255, 220, 255 }
    );

    if (mostrarDebug)
    {
        DrawSphereWires({ x, Y_PUNTO_DULCE, Z_PUNTO_DULCE }, 0.5f, 6, 6, LIME);
    }

    // Meteorito segun el estado del lanzamiento.
    if (m.fase == FASE_BATEO_JUGANDO && m.ronda < LANZAMIENTOS_BATEO)
    {
        const LanzamientoBateo& lanzamiento = m.lanzamientos[m.ronda];

        if (
            e.estado == LANZAMIENTO_EN_VUELO ||
            (e.estado == LANZAMIENTO_PASADO && e.reloj <= lanzamiento.duracion + 0.45f)
        )
        {
            Vector3 posicion = PosicionMeteoritoEntrante(x, lanzamiento, e.reloj);
            Color estela = ColorEstela(e.tipoActual);

            for (int k = 1; k <= 4; k++)
            {
                float anterior = e.reloj - 0.05f * (float)k;

                if (anterior < 0.0f) break;

                Vector3 punto = PosicionMeteoritoEntrante(x, lanzamiento, anterior);
                DrawSphere(punto, 0.34f - 0.06f * (float)k, Fade(estela, 0.6f - 0.12f * (float)k));
            }

            DibujarMeteorito(posicion, e.tipoActual, t, 1.0f);
        }
        else if (e.estado == LANZAMIENTO_GOLPEADO)
        {
            float s = e.tiempoVuelo / e.duracionVuelo;
            Color estela = ColorEstela(e.tipoActual);

            for (int k = 1; k <= 5; k++)
            {
                float anterior = s - 0.04f * (float)k;

                if (anterior < 0.0f) break;

                DrawSphere(
                    PosicionMeteoritoGolpeado(e, anterior),
                    0.38f - 0.05f * (float)k,
                    Fade(estela, 0.7f - 0.12f * (float)k)
                );
            }

            DibujarMeteorito(PosicionMeteoritoGolpeado(e, s), e.tipoActual, t, 1.0f);
        }
        else if (e.estado == LANZAMIENTO_EXPLOTADO && e.tiempoPostEvento < 0.7f)
        {
            float k = e.tiempoPostEvento / 0.7f;
            DrawSphere(e.puntoAterrizaje, 0.6f + 2.4f * k, Fade(Color{ 255, 70, 40, 255 }, 0.7f * (1.0f - k)));
        }
    }

    // Destello del impacto del bate.
    if (e.tiempoImpacto > 0.0f && e.estado != LANZAMIENTO_EXPLOTADO)
    {
        float k = 1.0f - e.tiempoImpacto / 0.5f;
        DrawSphere(e.puntoGolpe, 0.3f + 0.9f * k, Fade(WHITE, 0.8f * (1.0f - k)));
    }

    // Jugador y bate.
    JugadorPrueba visualJugador = jugador;
    visualJugador.posicion = { x, 0.0f, Z_JUGADOR };
    visualJugador.direccionMirada = { 0.0f, 0.0f, -1.0f };
    visualJugador.velocidad = { 0.0f, 0.0f, 0.0f };
    visualJugador.enSuelo = true;
    visualJugador.cayendo = false;

    Participante visual = participante;
    visual.conectado = true;
    visual.color = colorJugador;

    DrawCircle3D({ x, 0.04f, Z_JUGADOR }, 0.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorJugador);
    DibujarJugadorCuboPrueba(visualJugador, visual);

    float angulo = -70.0f;

    if (e.tiempoSwing > 0.0f)
    {
        float avance = DURACION_SWING - e.tiempoSwing;
        angulo = -60.0f + Acotar(avance / 0.12f, 0.0f, 1.0f) * 140.0f;
    }

    rlPushMatrix();
    rlTranslatef(x + 0.4f, 1.05f, Z_JUGADOR);
    rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
    DrawCylinderEx({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -0.5f }, 0.06f, 0.07f, 8, Color{ 120, 80, 50, 255 });
    DrawCylinderEx({ 0.0f, 0.0f, -0.5f }, { 0.0f, 0.0f, -1.45f }, 0.1f, 0.16f, 10, colorJugador);
    rlPopMatrix();
}


//==================================================
// DIBUJO
//==================================================

static const char* TextoMensajeBateo(MensajeBateo mensaje)
{
    switch (mensaje)
    {
        case MENSAJE_BATEO_PERFECTO: return "PERFECTO";
        case MENSAJE_BATEO_BUENO: return "BUEN GOLPE";
        case MENSAJE_BATEO_REGULAR: return "REGULAR";
        case MENSAJE_BATEO_FLOJO: return "FLOJO";
        case MENSAJE_BATEO_FALLO: return "FALLASTE";
        case MENSAJE_BATEO_SIN_GOLPE: return "SIN GOLPE";
        case MENSAJE_BATEO_ROJO: return "ROJO! -50";
        case MENSAJE_BATEO_EVITADO: return "EVITADO";
        default: return "";
    }
}


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

    ClearBackground(Color{ 8, 10, 30, 255 });
    BeginMode3D(camara);

    DibujarCieloNocturno(tiempoAnimacion);
    DibujarObservatorio();
    DibujarCumbre(tiempoAnimacion);

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarCarril(*this, i, jugadores[i], participantes[i], mostrarDebug);
        }
    }

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    DrawText("BATEO METEORICO", 28, 20, 28, GOLD);
    {
        const char* subtitulo = "ANILLOS: 100 / 60 / 30 / 10   DORADO x2   ROJO: NO LO GOLPEES (-50)";
        DrawRectangle(20, 50, MeasureText(subtitulo, 20) + 16, 28, Fade(BLACK, 0.7f));
        DrawText(subtitulo, 28, 54, 20, Color{ 225, 240, 255, 255 });
    }

    const char* textoRonda = TextFormat(
        "LANZAMIENTO %d / %d",
        ronda < LANZAMIENTOS_BATEO ? ronda + 1 : LANZAMIENTOS_BATEO,
        LANZAMIENTOS_BATEO
    );
    DrawText(textoRonda, anchoPantalla - MeasureText(textoRonda, 24) - 28, 22, 24, GOLD);

    int activos = 0;

    // Tarjetas por carril, alineadas con cada carril en pantalla.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        activos++;

        const EstadoJugadorBateo& e = estadosJugadores[i];
        Color colorJugador = COLORES_JUGADORES_BATEO[i % MAX_PARTICIPANTES];
        Vector2 base = GetWorldToScreen({ e.carrilX, 0.0f, Z_JUGADOR }, camara);
        int anchoTarjeta = 210;
        int altoTarjeta = 104;
        int px = (int)base.x - anchoTarjeta / 2;
        int py = altoPantalla - altoTarjeta - 12;

        if (px < 6) px = 6;
        if (px + anchoTarjeta > anchoPantalla - 6) px = anchoPantalla - 6 - anchoTarjeta;

        DrawRectangle(px, py, anchoTarjeta, altoTarjeta, Fade(BLACK, 0.7f));
        DrawRectangleLines(px, py, anchoTarjeta, altoTarjeta, colorJugador);
        DrawText(NombreJugadorBateo(participantes[i], i), px + 10, py + 6, 20, colorJugador);
        DrawText(TextFormat("%d", e.puntos), px + anchoTarjeta - 10 - MeasureText(TextFormat("%d", e.puntos), 24), py + 4, 24, RAYWHITE);

        // Lanzamientos restantes como puntos (los gastados quedan vacios).
        int restantes = LANZAMIENTOS_BATEO - ronda;

        if (fase == FASE_BATEO_JUGANDO && etapa == ETAPA_BATEO_PAUSA)
        {
            restantes--;
        }

        if (fase == FASE_BATEO_TERMINADO) restantes = 0;
        if (restantes < 0) restantes = 0;

        for (int k = 0; k < LANZAMIENTOS_BATEO; k++)
        {
            Vector2 centro = { (float)(px + 18 + k * 22), (float)(py + 42) };
            DrawCircleV(centro, 7.0f, k < restantes ? colorJugador : Fade(GRAY, 0.5f));
        }

        DrawText(TextFormat("RESTAN %d", restantes), px + 130, py + 34, 16, LIGHTGRAY);

        if (e.tiempoMensaje > 0.0f)
        {
            const char* texto = TextoMensajeBateo(e.mensaje);
            Color colorTexto = e.mensaje == MENSAJE_BATEO_PERFECTO ? GOLD : RAYWHITE;

            if (e.mensaje == MENSAJE_BATEO_ROJO || e.mensaje == MENSAJE_BATEO_FALLO || e.mensaje == MENSAJE_BATEO_SIN_GOLPE)
            {
                colorTexto = RED;
            }

            DrawText(texto, px + 10, py + 62, 18, colorTexto);

            if (e.puntosMensaje != 0)
            {
                DrawText(TextFormat("%+d", e.puntosMensaje), px + anchoTarjeta - 70, py + 62, 18, colorTexto);
            }
        }
        else if (!JugadorEsBot(participantes[i]))
        {
            DrawText(TextFormat("GOLPE: %s", ObtenerTextoBotonPrincipal(participantes[i])), px + 10, py + 64, 16, LIGHTGRAY);
        }
        else
        {
            DrawText("AUTOMATICO", px + 10, py + 64, 16, LIGHTGRAY);
        }

        // Puntos flotantes en el lugar donde aterrizo el meteorito.
        if (
            e.tiempoMensaje > 1.2f &&
            (e.estado == LANZAMIENTO_ATERRIZADO || e.estado == LANZAMIENTO_EXPLOTADO) &&
            e.puntosMensaje != 0
        )
        {
            float subida = (2.0f - e.tiempoMensaje) * 1.2f;
            Vector2 pantalla = GetWorldToScreen(
                { e.puntoAterrizaje.x, e.puntoAterrizaje.y + 1.0f + subida, e.puntoAterrizaje.z },
                camara
            );
            const char* texto = TextFormat("%+d", e.puntosMensaje);
            DrawText(texto, (int)pantalla.x - MeasureText(texto, 28) / 2, (int)pantalla.y, 28, e.puntosMensaje > 0 ? GOLD : RED);
        }
    }

    if (fase == FASE_BATEO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 110, 96, GOLD);

        const char* ayuda = "PULSA GOLPE cuando el meteorito llegue al aro verde";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 22) / 2, altoPantalla / 2 - 10, 22, RAYWHITE);
    }
    else if (fase == FASE_BATEO_JUGANDO && etapa == ETAPA_BATEO_AVISO)
    {
        const char* aviso = "PREPARATE...";
        DrawText(aviso, anchoPantalla / 2 - MeasureText(aviso, 30) / 2, altoPantalla / 2 - 150, 30, RAYWHITE);
    }
    else if (
        fase == FASE_BATEO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 40;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE EN EL OBSERVATORIO";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorBateo(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (
                    !resultado.participantes[i].participo ||
                    resultado.participantes[i].posicionFinal != posicion
                )
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d puntos   (mejor golpe %d%%)",
                        posicion,
                        NombreJugadorBateo(participantes[i], i),
                        estadosJugadores[i].puntos,
                        estadosJugadores[i].mejorGolpe / 10
                    ),
                    px + 30,
                    py + 62 + fila * 30,
                    22,
                    COLORES_JUGADORES_BATEO[i % MAX_PARTICIPANTES]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            anchoPantalla / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoBateoMeteorico::ObtenerResultado() const
{
    return resultado;
}
