#include "Minigames/MinijuegoConteoExplosivo.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>

// SombrasRetro.h estampa una mancha en cada DrawCube/DrawSphere/DrawCylinder.
// El hangar tiene iluminacion propia: usamos las primitivas puras y
// dibujamos a mano las sombras que importan (las de los drones).
#undef DrawCube
#undef DrawSphere
#undef DrawCylinder


//==================================================
// CONTEO EXPLOSIVO
//==================================================
//
// Desafio: contar cuantos drones vuelan dentro de la jaula de pruebas y
// responder el numero. Gana quien queda mas cerca del total.
//
// Lo que NO debe filtrarse:
//  - el total por el aspecto de los drones: todos son identicos (mismo
//    modelo, color y luces; las fases de flotacion y helice son al azar e
//    independientes del indice y del total);
//  - las respuestas ajenas antes de revelar: durante la respuesta cada
//    humano ve solo su numero en su etiqueta; los bots muestran
//    «PENSANDO» / «LISTO», nunca su numero;
//  - nada decorativo vuela ni se parece a un dron (no hay drones en
//    estanterias ni objetos voladores en el fondo).
//==================================================


static const float DURACION_PREPARACION_CONTEO = 2.5f;
static const float DURACION_OBSERVACION_CONTEO = 8.0f;
static const float DURACION_RESPUESTA_CONTEO = 7.0f;

// Revelacion (segundos desde que termina el tiempo de respuesta). El
// resultado se cierra al terminar el tiempo; esto es solo la puesta en
// escena previa al tablon final.
static const float T_CAIDA_PERSIANA_CONTEO = 0.22f;
static const float T_SUBIDA_PERSIANA_CONTEO = 0.38f;
static const float T_INICIO_APARICION_CONTEO = 0.40f;
static const float DURACION_APARICION_CONTEO = 1.35f;
static const float T_TOTAL_REVELADO_CONTEO = 1.90f;
static const float T_FIN_REVELACION_CONTEO = 2.60f;

// Jaula de pruebas (los drones rebotan dentro de +-5,7 x +-3,5).
static const float JAULA_X_CONTEO = 6.35f;
static const float JAULA_Z_CONTEO = 4.10f;
static const float JAULA_ALTO_CONTEO = 4.20f;
static const float ALTO_PLATAFORMA_CONTEO = 0.10f;

static const float Y_TITULO_RESPUESTA_CONTEO = 214.0f;
static const float Y_TITULO_REVELACION_CONTEO = 196.0f;


//==================================================
// UTILIDADES
//==================================================

static float AzarConteo(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 10000) / 10000.0f;
}


static float Limitar01Conteo(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static float Hash01Conteo(int a, int b)
{
    unsigned int h = (unsigned int)(a * 73856093) ^ (unsigned int)(b * 19349663);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float EaseOutBackConteo(float t)
{
    t = Limitar01Conteo(t);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}


static Color TinteConteo(Color c, float f)
{
    float r = c.r * f;
    float g = c.g * f;
    float b = c.b * f;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, c.a };
}


// Momento (desde el fin de la respuesta) en que reaparece el dron k.
static float TiempoAparicionDronConteo(int indice, int cantidad)
{
    if (cantidad <= 0) return T_INICIO_APARICION_CONTEO;
    return T_INICIO_APARICION_CONTEO + DURACION_APARICION_CONTEO * (float)indice / (float)cantidad;
}


static float FactorMovimientoConteo()
{
    return ObtenerFactorMovimientoCamaraMinijuegos();
}


//==================================================
// DRONES (logica)
//==================================================

static void PrepararDronesConteo(
    MinijuegoConteoExplosivo& minijuego
)
{
    minijuego.cantidadDrones = GetRandomValue(
        12,
        MAX_DRONES_CONTEO_EXPLOSIVO
    );

    for (int i = 0; i < minijuego.cantidadDrones; i++)
    {
        DronConteoExplosivo& dron = minijuego.drones[i];

        dron.posicion =
        {
            GetRandomValue(-540, 540) / 100.0f,
            GetRandomValue(90, 250) / 100.0f,
            GetRandomValue(-330, 330) / 100.0f
        };

        float velocidadX = GetRandomValue(-120, 120) / 100.0f;
        float velocidadZ = GetRandomValue(-105, 105) / 100.0f;

        if (std::fabs(velocidadX) < 0.38f)
        {
            velocidadX = velocidadX < 0.0f ? -0.62f : 0.62f;
        }

        if (std::fabs(velocidadZ) < 0.32f)
        {
            velocidadZ = velocidadZ < 0.0f ? -0.54f : 0.54f;
        }

        dron.velocidad = { velocidadX, 0.0f, velocidadZ };
        dron.faseFlotacion = GetRandomValue(0, 628) / 100.0f;
        dron.faseHelice = GetRandomValue(0, 628) / 100.0f;
        dron.inclinacionX = 0.0f;
        dron.inclinacionZ = 0.0f;
    }
}


static void ActualizarDronesConteo(
    MinijuegoConteoExplosivo& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < minijuego.cantidadDrones; i++)
    {
        DronConteoExplosivo& dron = minijuego.drones[i];

        dron.posicion.x += dron.velocidad.x * deltaTime;
        dron.posicion.z += dron.velocidad.z * deltaTime;

        if (dron.posicion.x < -5.7f || dron.posicion.x > 5.7f)
        {
            dron.posicion.x = Clamp(dron.posicion.x, -5.7f, 5.7f);
            dron.velocidad.x *= -1.0f;
        }

        if (dron.posicion.z < -3.5f || dron.posicion.z > 3.5f)
        {
            dron.posicion.z = Clamp(dron.posicion.z, -3.5f, 3.5f);
            dron.velocidad.z *= -1.0f;
        }
    }
}


// Inclinacion visual hacia la direccion de avance (suavizada para que el
// rebote no sea un salto seco).
static void ActualizarInclinacionDronesConteo(
    MinijuegoConteoExplosivo& minijuego,
    float deltaTime,
    bool enVuelo
)
{
    float k = 1.0f - std::exp(-6.0f * deltaTime);

    for (int i = 0; i < minijuego.cantidadDrones; i++)
    {
        DronConteoExplosivo& dron = minijuego.drones[i];
        float objetivoX = enVuelo ? dron.velocidad.z * 9.0f : 0.0f;
        float objetivoZ = enVuelo ? -dron.velocidad.x * 9.0f : 0.0f;
        dron.inclinacionX += (objetivoX - dron.inclinacionX) * k;
        dron.inclinacionZ += (objetivoZ - dron.inclinacionZ) * k;
    }
}


static void FinalizarConteo(
    MinijuegoConteoExplosivo& minijuego
)
{
    int mejorDiferencia = 1000;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorConteoExplosivo& jugador = minijuego.jugadores[i];
        jugador.diferencia = std::abs(
            jugador.respuesta - minijuego.cantidadDrones
        );

        if (jugador.diferencia < mejorDiferencia)
        {
            mejorDiferencia = jugador.diferencia;
            cantidadGanadores = 1;
        }
        else if (jugador.diferencia == mejorDiferencia)
        {
            cantidadGanadores++;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                minijuego.jugadores[j].diferencia <
                    minijuego.jugadores[i].diferencia
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            100 - minijuego.jugadores[i].diferencia * 15;

        if (resultadoJugador.puntuacionMinijuego < 0)
        {
            resultadoJugador.puntuacionMinijuego = 0;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_CONTEO_TERMINADO;

    // SONIDO_RESULTADO suena al cerrar la revelacion (ver Actualizar).
    minijuego.tiempoFase = 0.0f;
    minijuego.dronesRevelados = 0;
    minijuego.totalRevelado = false;
    minijuego.resultadoAnunciado = false;
}


//==================================================
// PARTICULAS (pool fijo, escalado por calidad)
//==================================================

static int CantidadParticulasConteo(int cantidad)
{
    int escalada = (int)std::lround((float)cantidad * FactorCalidadGrafica(CalidadParticulas()));
    return escalada < 1 ? 1 : escalada;
}


static ParticulaConteoExplosivo* ReservarParticulaConteo(MinijuegoConteoExplosivo& m)
{
    for (int i = 0; i < MAX_PARTICULAS_CONTEO_EXPLOSIVO; i++)
    {
        if (!m.particulas[i].activa) return &m.particulas[i];
    }

    return nullptr;
}


static void EmitirParticulasConteo(
    MinijuegoConteoExplosivo& m,
    TipoParticulaConteoExplosivo tipo,
    Vector3 origen,
    float dispersion,
    int cantidad,
    const Color* colores,
    int cantidadColores
)
{
    int total = CantidadParticulasConteo(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaConteoExplosivo* p = ReservarParticulaConteo(m);
        if (p == nullptr) return;

        float angulo = AzarConteo(0.0f, 6.2832f);
        float radio = AzarConteo(0.0f, dispersion);

        *p = {};
        p->activa = true;
        p->tipo = tipo;
        p->posicion = { origen.x + std::cos(angulo) * radio, origen.y, origen.z + std::sin(angulo) * radio };
        p->giro = AzarConteo(0.0f, 360.0f);

        switch (tipo)
        {
            case PARTICULA_CONTEO_CHISPA:
            {
                float v = AzarConteo(1.2f, 3.0f);
                p->velocidad = { std::cos(angulo) * v, AzarConteo(0.8f, 3.2f), std::sin(angulo) * v };
                p->vidaMaxima = AzarConteo(0.25f, 0.55f);
                p->tamano = AzarConteo(0.04f, 0.07f);
                p->color = Color{ 190, (unsigned char)GetRandomValue(225, 250), 255, 255 };
                break;
            }

            case PARTICULA_CONTEO_CONFETI:
            {
                float v = AzarConteo(1.0f, 3.4f);
                p->velocidad = { std::cos(angulo) * v, AzarConteo(4.0f, 7.5f), std::sin(angulo) * v * 0.7f };
                p->vidaMaxima = AzarConteo(1.3f, 2.1f);
                p->tamano = AzarConteo(0.08f, 0.13f);
                p->color = colores != nullptr && cantidadColores > 0
                    ? colores[GetRandomValue(0, cantidadColores - 1)]
                    : Color{ 255, 226, 96, 255 };
                break;
            }

            case PARTICULA_CONTEO_HUMO:
            {
                p->velocidad = { std::cos(angulo) * 0.5f, AzarConteo(0.6f, 1.4f), std::sin(angulo) * 0.5f };
                p->vidaMaxima = AzarConteo(1.0f, 1.7f);
                p->tamano = AzarConteo(0.12f, 0.20f);
                unsigned char tono = (unsigned char)GetRandomValue(70, 105);
                p->color = Color{ tono, tono, (unsigned char)(tono + 8), 170 };
                break;
            }

            case PARTICULA_CONTEO_POLVO:
            {
                float v = AzarConteo(0.6f, 1.8f);
                p->velocidad = { std::cos(angulo) * v, AzarConteo(0.2f, 0.8f), std::sin(angulo) * v };
                p->vidaMaxima = AzarConteo(0.5f, 0.95f);
                p->tamano = AzarConteo(0.10f, 0.20f);
                unsigned char tono = (unsigned char)GetRandomValue(110, 140);
                p->color = Color{ tono, (unsigned char)(tono - 6), (unsigned char)(tono - 16), 180 };
                break;
            }
        }

        p->vida = p->vidaMaxima;
    }
}


static void ActualizarParticulasConteo(MinijuegoConteoExplosivo& m, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICULAS_CONTEO_EXPLOSIVO; i++)
    {
        ParticulaConteoExplosivo& p = m.particulas[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        switch (p.tipo)
        {
            case PARTICULA_CONTEO_CHISPA:
                p.velocidad.y -= 7.5f * deltaTime;
                break;

            case PARTICULA_CONTEO_CONFETI:
            {
                p.velocidad.y -= 6.0f * deltaTime;
                float freno = std::exp(-2.2f * deltaTime);
                p.velocidad.x *= freno;
                p.velocidad.z *= freno;
                if (p.velocidad.y < -1.3f) p.velocidad.y = -1.3f;
                p.giro += 420.0f * deltaTime;
                break;
            }

            case PARTICULA_CONTEO_HUMO:
            {
                float freno = std::exp(-1.4f * deltaTime);
                p.velocidad.x *= freno;
                p.velocidad.z *= freno;
                p.tamano += 0.16f * deltaTime;
                break;
            }

            case PARTICULA_CONTEO_POLVO:
            {
                float freno = std::exp(-3.0f * deltaTime);
                p.velocidad.x *= freno;
                p.velocidad.z *= freno;
                p.velocidad.y -= 0.6f * deltaTime;
                break;
            }
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (p.posicion.y < ALTO_PLATAFORMA_CONTEO + 0.02f)
        {
            p.posicion.y = ALTO_PLATAFORMA_CONTEO + 0.02f;
            p.velocidad.y = 0.0f;
        }
    }
}


//==================================================
// REVELACION (efectos, solo visual)
//==================================================

static void ActualizarRevelacionConteo(
    MinijuegoConteoExplosivo& m,
    const Participante participantes[]
)
{
    // Los drones reaparecen de a uno: chispa y "tic" por cada uno.
    while (
        m.dronesRevelados < m.cantidadDrones &&
        m.tiempoFase >= TiempoAparicionDronConteo(m.dronesRevelados, m.cantidadDrones)
    )
    {
        const DronConteoExplosivo& dron = m.drones[m.dronesRevelados];
        EmitirParticulasConteo(m, PARTICULA_CONTEO_CHISPA, dron.posicion, 0.15f, 5, nullptr, 0);
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
        m.dronesRevelados++;
    }

    if (!m.totalRevelado && m.tiempoFase >= T_TOTAL_REVELADO_CONTEO)
    {
        m.totalRevelado = true;

        Color colores[MAX_PARTICIPANTES + 1];
        int cantidadColores = 0;
        colores[cantidadColores++] = Color{ 255, 226, 96, 255 };

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (m.resultado.participantes[i].participo && m.jugadores[i].diferencia == 0)
            {
                colores[cantidadColores++] = participantes[i].color;
            }
        }

        if (cantidadColores > 1)
        {
            // Alguien la emboco: confeti con los colores de quienes acertaron.
            EmitirParticulasConteo(m, PARTICULA_CONTEO_CONFETI, { 0.0f, 0.6f, 0.0f }, 3.5f, 90, colores, cantidadColores);
            EmitirParticulasConteo(m, PARTICULA_CONTEO_CHISPA, { 0.0f, 1.2f, 0.0f }, 1.0f, 24, nullptr, 0);
            ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
            ActivarTemblorCamaraGeneral(0.07f, 0.22f);
        }
        else
        {
            // Nadie acerto: humo en la jaula y sacudida.
            for (int k = 0; k < 6; k++)
            {
                float x = -4.5f + 1.8f * (float)k;
                EmitirParticulasConteo(m, PARTICULA_CONTEO_HUMO, { x, 0.3f, AzarConteo(-2.5f, 2.5f) }, 0.8f, 6, nullptr, 0);
            }
            ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
            ActivarTemblorCamaraGeneral(0.20f, 0.36f);
        }
    }

    if (!m.resultadoAnunciado && m.tiempoFase >= T_FIN_REVELACION_CONTEO)
    {
        m.resultadoAnunciado = true;
        ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
    }
}


//==================================================
// CICLO DE VIDA
//==================================================

void MinijuegoConteoExplosivo::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_CONTEO_EXPLOSIVO; i++)
    {
        particulas[i] = {};
    }

    camara.position = { 0.0f, 9.0f, 11.0f };
    camara.target = { 0.0f, 1.1f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CONTEO_PREPARACION;
    cantidadDrones = 0;
    tiempoFase = DURACION_PREPARACION_CONTEO;
    tiempoAnimacion = 0.0f;
    dronesRevelados = 0;
    persianaAsentada = false;
    totalRevelado = false;
    resultadoAnunciado = false;
}


void MinijuegoConteoExplosivo::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    PrepararDronesConteo(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].respuesta = 17;
        jugadores[i].retrasoBot = GetRandomValue(35, 160) / 100.0f;
    }
}


void MinijuegoConteoExplosivo::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    ActualizarParticulasConteo(*this, deltaTime);

    bool enVuelo =
        fase == FASE_CONTEO_PREPARACION ||
        fase == FASE_CONTEO_OBSERVACION;

    if (enVuelo)
    {
        ActualizarDronesConteo(*this, deltaTime);
    }

    ActualizarInclinacionDronesConteo(*this, deltaTime, enVuelo);

    if (fase == FASE_CONTEO_TERMINADO)
    {
        tiempoFase += deltaTime;
        ActualizarRevelacionConteo(*this, participantes);
        return;
    }

    float faseAntes = tiempoFase;
    tiempoFase -= deltaTime;

    if (fase == FASE_CONTEO_PREPARACION)
    {
        ActualizarAudioCuentaRegresiva(audio, faseAntes, tiempoFase);
    }
    else if (fase == FASE_CONTEO_RESPUESTA)
    {
        ActualizarAudioAlertaTiempo(audio, faseAntes, tiempoFase, 3.0f);
    }

    if (fase == FASE_CONTEO_PREPARACION)
    {
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CONTEO_OBSERVACION;
            tiempoFase = DURACION_OBSERVACION_CONTEO;
        }

        return;
    }

    if (fase == FASE_CONTEO_OBSERVACION)
    {
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CONTEO_RESPUESTA;
            ReproducirSonidoMinijuego(audio, SONIDO_INICIO_MINIJUEGO);
            tiempoFase = DURACION_RESPUESTA_CONTEO;
            persianaAsentada = false;
        }

        return;
    }

    // La persiana que tapa la jaula golpea el piso: polvo y temblor leve.
    if (!persianaAsentada && DURACION_RESPUESTA_CONTEO - tiempoFase >= T_CAIDA_PERSIANA_CONTEO)
    {
        persianaAsentada = true;
        ReproducirSonidoMinijuego(audio, SONIDO_IMPACTO);
        ActivarTemblorCamaraGeneral(0.10f, 0.22f);

        for (int k = 0; k < 8; k++)
        {
            float x = -JAULA_X_CONTEO + 2.0f * JAULA_X_CONTEO * ((float)k + 0.5f) / 8.0f;
            EmitirParticulasConteo(*this, PARTICULA_CONTEO_POLVO, { x, 0.2f, JAULA_Z_CONTEO + 0.15f }, 0.3f, 3, nullptr, 0);
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorConteoExplosivo& jugador = jugadores[i];

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            jugador.retrasoBot -= deltaTime;

            if (!jugador.botRespondio && jugador.retrasoBot <= 0.0f)
            {
                int margen = (int)std::ceil((float)cantidadDrones * 0.3f);
                if (margen < 2) margen = 2;

                // 10% exacto; el resto falla por 1..margen (nunca 0).
                int error = 0;
                if (GetRandomValue(0, 99) >= 10)
                {
                    error = GetRandomValue(1, margen);
                    if (GetRandomValue(0, 1) == 0) error = -error;
                }

                jugador.respuesta = cantidadDrones + error;
                jugador.respuesta = Clamp(jugador.respuesta, 0, 30);
                jugador.botRespondio = true;
            }

            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participantes[i]);

        if (entrada.golpear)
        {
            jugador.respuesta++;
            ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
        }

        if (entrada.saltar)
        {
            jugador.respuesta--;
            ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
        }

        // Direcciones: derecha/izquierda +-1 (mantener acelera), arriba/abajo +-5.
        int direccion = 0;
        if (entrada.derecha) direccion = 1;
        else if (entrada.izquierda) direccion = -1;
        else if (entrada.adelante) direccion = 5;
        else if (entrada.atras) direccion = -5;

        if (direccion == 0)
        {
            jugador.direccionMantenida = 0;
            jugador.tiempoMantenido = 0.0f;
        }
        else
        {
            if (direccion != jugador.direccionMantenida)
            {
                jugador.direccionMantenida = direccion;
                jugador.tiempoMantenido = 0.0f;
                jugador.proximoPaso = 0.0f;
            }

            jugador.tiempoMantenido += deltaTime;
            jugador.proximoPaso -= deltaTime;

            if (jugador.proximoPaso <= 0.0f)
            {
                jugador.respuesta += direccion;
                ReproducirSonidoMinijuego(audio, SONIDO_BOTON);

                float espera = 0.30f - jugador.tiempoMantenido * 0.12f;
                if (espera < 0.07f) espera = 0.07f;
                jugador.proximoPaso = jugador.tiempoMantenido <= 0.0f ? 0.35f : espera;
            }
        }

        jugador.respuesta = Clamp(jugador.respuesta, 0, 30);
    }

    if (tiempoFase <= 0.0f)
    {
        FinalizarConteo(*this);
    }
}


//==================================================
// VISUAL: PRIMITIVAS
//==================================================

// Caja con cada cara a un brillo distinto (luz desde arriba y de frente).
static void CajaFacetadaConteo(Vector3 centro, Vector3 tamano, Color color, float brilloTapa = 1.25f)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TinteConteo(color, brilloTapa);
    Color frente = TinteConteo(color, 0.82f);
    Color atras = TinteConteo(color, 0.58f);
    Color derecha = TinteConteo(color, 0.66f);
    Color izquierda = TinteConteo(color, 0.72f);

    DrawTriangle3D({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, tapa);
    DrawTriangle3D({ x0, y1, z0 }, { x1, y1, z1 }, { x1, y1, z0 }, tapa);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, atras);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, atras);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, derecha);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, derecha);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, izquierda);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z1 }, { x0, y1, z0 }, izquierda);
}


static void CajaConBordeConteo(Vector3 centro, Vector3 tamano, Color color)
{
    CajaFacetadaConteo(centro, tamano, color);
    DrawCubeWiresV(centro, tamano, Fade(BLACK, 0.28f));
}


static void RectSueloConteo(float x0, float z0, float x1, float z1, float y, Color color)
{
    DrawTriangle3D({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, color);
    DrawTriangle3D({ x0, y, z0 }, { x1, y, z1 }, { x1, y, z0 }, color);
}


// Anillo pintado en el piso.
static void AnilloSueloConteo(float cx, float cz, float r0, float r1, float y, Color color, int lados)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;
        Vector3 i0 = { cx + std::cos(a0) * r0, y, cz + std::sin(a0) * r0 };
        Vector3 i1 = { cx + std::cos(a1) * r0, y, cz + std::sin(a1) * r0 };
        Vector3 e0 = { cx + std::cos(a0) * r1, y, cz + std::sin(a0) * r1 };
        Vector3 e1 = { cx + std::cos(a1) * r1, y, cz + std::sin(a1) * r1 };
        DrawTriangle3D(i0, e0, e1, color);
        DrawTriangle3D(i0, e1, i1, color);
    }
}


// Cuadrilatero vertical (plano z constante) con degradado de abajo a arriba.
static void DegradadoVerticalConteo(float x0, float x1, float y0, float y1, float z, Color abajo, Color arriba)
{
    rlBegin(RL_QUADS);
    rlColor4ub(abajo.r, abajo.g, abajo.b, abajo.a);
    rlVertex3f(x0, y0, z);
    rlVertex3f(x1, y0, z);
    rlColor4ub(arriba.r, arriba.g, arriba.b, arriba.a);
    rlVertex3f(x1, y1, z);
    rlVertex3f(x0, y1, z);
    rlEnd();
}


// Disco de luz suave (centro opaco, borde transparente). Solo con mezcla
// aditiva y sin escribir profundidad.
static void ResplandorConteo(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 14;
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
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloConteo(float x, float z, float y, float radio, Color color)
{
    ResplandorConteo({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Disco de frente a la camara (aprox.: la camara mira hacia -Z y abajo).
static void ResplandorFrontalConteo(Vector3 centro, float radio, Color color)
{
    ResplandorConteo(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.82f, 0.57f }, radio, color);
}


//==================================================
// VISUAL: LUCES DE PISTA (compartidas por escena y resplandores)
//==================================================

static const int LUCES_BORDE_X_CONTEO = 13;     // por lado largo
static const int LUCES_BORDE_Z_CONTEO = 8;      // por lado corto
static const int LUCES_APROXIMACION_CONTEO = 11;


// Luces del borde de la plataforma: una ola ambar que da la vuelta.
static float BrilloLuzBordeConteo(float recorrido, float t)
{
    float ola = std::cos(recorrido * 0.55f - t * 4.2f);
    ola = ola > 0.0f ? ola * ola * ola * ola : 0.0f;
    return 0.35f + 0.65f * ola;
}


static Vector3 PosicionLuzBordeConteo(int indice, float* recorrido)
{
    const float x0 = -JAULA_X_CONTEO - 0.25f;
    const float z0 = -JAULA_Z_CONTEO - 0.25f;
    const float ancho = 2.0f * (JAULA_X_CONTEO + 0.25f);
    const float fondo = 2.0f * (JAULA_Z_CONTEO + 0.25f);
    const float y = ALTO_PLATAFORMA_CONTEO + 0.02f;

    int lado = indice / LUCES_BORDE_X_CONTEO;
    int k = indice % LUCES_BORDE_X_CONTEO;

    // 0: frente, 1: fondo (13 luces); 2: izquierda, 3: derecha (8 luces).
    if (lado <= 1)
    {
        float u = (float)k / (float)(LUCES_BORDE_X_CONTEO - 1);
        *recorrido = lado == 0 ? u * ancho : ancho + fondo + (1.0f - u) * ancho;
        return { x0 + u * ancho, y, lado == 0 ? -z0 : z0 };
    }

    int j = indice - 2 * LUCES_BORDE_X_CONTEO;
    lado = j / LUCES_BORDE_Z_CONTEO;
    k = j % LUCES_BORDE_Z_CONTEO;
    float u = ((float)k + 0.5f) / (float)LUCES_BORDE_Z_CONTEO;
    *recorrido = lado == 0 ? 2.0f * ancho + fondo + u * fondo : ancho + (1.0f - u) * fondo;
    return { lado == 0 ? x0 : -x0, y, z0 + u * fondo };
}


static int CantidadLucesBordeConteo()
{
    return 2 * LUCES_BORDE_X_CONTEO + 2 * LUCES_BORDE_Z_CONTEO;
}


// Luces de aproximacion hacia el porton: un destello que corre.
static Vector3 PosicionLuzAproximacionConteo(int k)
{
    float z = -JAULA_Z_CONTEO - 0.9f - (float)k * 1.6f;
    return { 0.0f, 0.03f, z };
}


static float BrilloLuzAproximacionConteo(int k, float t)
{
    float ciclo = std::fmod(t * 1.4f, 1.0f) * (float)(LUCES_APROXIMACION_CONTEO + 3);
    float d = std::fabs(ciclo - (float)k);
    return 0.25f + 0.75f * Limitar01Conteo(1.0f - d * 0.8f);
}


//==================================================
// VISUAL: HANGAR
//==================================================
//
// MODELO FUTURO: el hangar, la consola y la utileria podrian pasar a GLB;
// por ahora todo es primitiva y se ve bien solo.
//==================================================

// Exterior visto por el porton: cielo nocturno, pista y ciudad lejana.
// Nada vuela ni brilla como un dron.
static void DibujarExteriorConteo(float t)
{
    DegradadoVerticalConteo(-60.0f, 60.0f, -2.0f, 34.0f, -62.0f, Color{ 52, 62, 104, 255 }, Color{ 10, 12, 30, 255 });

    // Pista exterior.
    RectSueloConteo(-60.0f, -62.0f, 60.0f, -9.0f, -0.02f, Color{ 24, 27, 36, 255 });
    RectSueloConteo(-3.2f, -60.0f, 3.2f, -9.0f, -0.015f, Color{ 32, 35, 46, 255 });

    for (int k = 0; k < 12; k++)
    {
        float z = -11.0f - (float)k * 4.0f;
        RectSueloConteo(-0.12f, z - 1.2f, 0.12f, z, -0.01f, Color{ 150, 150, 140, 255 });
    }

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Ciudad lejana: siluetas con ventanas encendidas (fijas).
    for (int e = 0; e < 18; e += 1)
    {
        float alto = 4.0f + 7.0f * Hash01Conteo(e, 4);
        float ancho = 2.4f + 2.2f * Hash01Conteo(e, 7);
        float x = -40.0f + (float)e * 4.7f;
        float z = -50.0f - 6.0f * Hash01Conteo(e, 11);
        Color tono = TinteConteo(Color{ 26, 30, 50, 255 }, 0.85f + 0.3f * Hash01Conteo(e, 2));
        CajaFacetadaConteo({ x, alto * 0.5f, z }, { ancho, alto, 2.0f }, tono, 1.1f);

        for (int f = 0; f < (int)(alto / 1.1f); f += paso)
        {
            for (int c = 0; c < 3; c++)
            {
                if (Hash01Conteo(e * 31 + f, c) < 0.55f) continue;
                float wx = x - ancho * 0.3f + (float)c * ancho * 0.3f;
                Color luz = Hash01Conteo(f, e + c) > 0.5f ? Color{ 255, 214, 130, 255 } : Color{ 170, 210, 255, 255 };
                DrawCubeV({ wx, 0.7f + (float)f * 1.1f, z + 1.02f }, { 0.34f, 0.42f, 0.04f }, Fade(luz, 0.75f));
            }
        }
    }

    // Balizas al borde de la pista exterior, apoyadas en el piso.
    for (int k = 0; k < 10; k += paso)
    {
        float z = -12.0f - (float)k * 4.5f;
        float brillo = 0.55f + 0.45f * std::sin(t * 2.0f + (float)k * 0.7f);
        for (int lado = -1; lado <= 1; lado += 2)
        {
            DrawCubeV({ (float)lado * 3.6f, 0.06f, z }, { 0.16f, 0.12f, 0.16f }, Fade(Color{ 120, 190, 255, 255 }, 0.45f + 0.5f * brillo));
        }
    }
}


static void DibujarSueloConteo()
{
    // Base continua (las juntas quedan en el tono oscuro).
    RectSueloConteo(-16.0f, -9.0f, 16.0f, 9.0f, -0.01f, Color{ 26, 29, 36, 255 });

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        RectSueloConteo(-16.0f, -9.0f, 16.0f, 9.0f, 0.0f, Color{ 44, 48, 58, 255 });
        return;
    }

    // Losas de hormigon con variacion por hash.
    for (int ix = -8; ix < 8; ix++)
    {
        for (int iz = -9; iz < 9; iz += 2)
        {
            float x0 = (float)ix * 2.0f;
            float z0 = (float)iz;
            float v = 0.86f + 0.22f * Hash01Conteo(ix, iz);
            Color losa = TinteConteo(Color{ 46, 50, 60, 255 }, v);
            RectSueloConteo(x0 + 0.04f, z0 + 0.04f, x0 + 1.96f, z0 + 1.96f, 0.0f, losa);

            // Mancha de aceite ocasional.
            if (Hash01Conteo(ix * 7, iz * 3) > 0.86f)
            {
                AnilloSueloConteo(x0 + 1.0f, z0 + 1.0f, 0.0f, 0.45f, 0.004f, Fade(Color{ 14, 14, 18, 255 }, 0.45f), 8);
            }
        }
    }

    // Lineas de seguridad amarillas alrededor de la plataforma.
    Color amarillo = Color{ 214, 170, 70, 255 };
    float mx = JAULA_X_CONTEO + 1.1f;
    float mz = JAULA_Z_CONTEO + 1.0f;
    RectSueloConteo(-mx, mz - 0.10f, mx, mz + 0.10f, 0.006f, amarillo);
    RectSueloConteo(-mx, -mz - 0.10f, -0.9f, -mz + 0.10f, 0.006f, amarillo);
    RectSueloConteo(0.9f, -mz - 0.10f, mx, -mz + 0.10f, 0.006f, amarillo);
    RectSueloConteo(-mx - 0.10f, -mz, -mx + 0.10f, mz, 0.006f, amarillo);
    RectSueloConteo(mx - 0.10f, -mz, mx + 0.10f, mz, 0.006f, amarillo);

    // Calle de rodaje hacia el porton.
    RectSueloConteo(-0.9f, -9.0f, -0.75f, -mz, 0.006f, amarillo);
    RectSueloConteo(0.75f, -9.0f, 0.9f, -mz, 0.006f, amarillo);
}


static void DibujarPlataformaConteo(float t)
{
    const float x = JAULA_X_CONTEO + 0.45f;
    const float z = JAULA_Z_CONTEO + 0.45f;
    const float y = ALTO_PLATAFORMA_CONTEO;

    CajaFacetadaConteo({ 0.0f, y * 0.5f, 0.0f }, { 2.0f * x, y, 2.0f * z }, Color{ 38, 54, 80, 255 }, 1.15f);

    // Superficie interior un poco mas clara y rejilla de calibracion.
    RectSueloConteo(-JAULA_X_CONTEO, -JAULA_Z_CONTEO, JAULA_X_CONTEO, JAULA_Z_CONTEO, y + 0.002f, Color{ 44, 64, 94, 255 });

    for (int g = -6; g <= 6; g++)
    {
        DrawLine3D({ (float)g, y + 0.006f, -JAULA_Z_CONTEO }, { (float)g, y + 0.006f, JAULA_Z_CONTEO }, Fade(Color{ 120, 200, 255, 255 }, 0.13f));
    }

    for (int g = -4; g <= 4; g++)
    {
        DrawLine3D({ -JAULA_X_CONTEO, y + 0.006f, (float)g }, { JAULA_X_CONTEO, y + 0.006f, (float)g }, Fade(Color{ 120, 200, 255, 255 }, 0.13f));
    }

    // Marca central de despegue.
    AnilloSueloConteo(0.0f, 0.0f, 1.45f, 1.62f, y + 0.008f, Fade(Color{ 200, 225, 255, 255 }, 0.35f), 28);
    AnilloSueloConteo(0.0f, 0.0f, 2.75f, 2.82f, y + 0.008f, Fade(Color{ 200, 225, 255, 255 }, 0.16f), 36);

    // Borde con franjas de peligro (amarillo / negro en diagonal simple).
    Color amarillo = Color{ 214, 170, 70, 255 };
    Color negro = Color{ 30, 30, 34, 255 };
    const float banda = 0.30f;
    const float tramo = 0.55f;
    int tramosX = (int)(2.0f * x / tramo);
    int tramosZ = (int)(2.0f * z / tramo);

    for (int k = 0; k < tramosX; k++)
    {
        float a = -x + (float)k * tramo;
        Color c = (k % 2 == 0) ? amarillo : negro;
        RectSueloConteo(a, z - banda, a + tramo, z, y + 0.004f, c);
        RectSueloConteo(a, -z, a + tramo, -z + banda, y + 0.004f, c);
    }

    for (int k = 0; k < tramosZ; k++)
    {
        float a = -z + (float)k * tramo;
        Color c = (k % 2 == 0) ? amarillo : negro;
        RectSueloConteo(-x, a, -x + banda, a + tramo, y + 0.004f, c);
        RectSueloConteo(x - banda, a, x, a + tramo, y + 0.004f, c);
    }

    // Luces empotradas del borde.
    for (int i = 0; i < CantidadLucesBordeConteo(); i++)
    {
        float recorrido = 0.0f;
        Vector3 p = PosicionLuzBordeConteo(i, &recorrido);
        float brillo = BrilloLuzBordeConteo(recorrido, t);
        Color luz = TinteConteo(Color{ 255, 196, 96, 255 }, 0.45f + 0.55f * brillo);
        DrawCubeV(p, { 0.16f, 0.05f, 0.16f }, luz);
    }

    // Luces de aproximacion (embutidas en el piso).
    for (int k = 0; k < LUCES_APROXIMACION_CONTEO; k++)
    {
        Vector3 p = PosicionLuzAproximacionConteo(k);
        if (p.z < -8.8f) break;
        float b = BrilloLuzAproximacionConteo(k, t);
        DrawCubeV(p, { 0.22f, 0.04f, 0.14f }, TinteConteo(Color{ 190, 225, 255, 255 }, 0.4f + 0.6f * b));
    }
}


// Postes de la jaula. Los de atras son altos (quedan detras de los
// drones); los de adelante son bolardos bajos, para no tapar ningun dron.
static void DibujarJaulaConteo(float t)
{
    Color metal = Color{ 72, 80, 98, 255 };
    const float y0 = ALTO_PLATAFORMA_CONTEO;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float px = (float)lado * JAULA_X_CONTEO;

        // Poste trasero con baliza roja.
        CajaConBordeConteo({ px, y0 + JAULA_ALTO_CONTEO * 0.5f, -JAULA_Z_CONTEO }, { 0.24f, JAULA_ALTO_CONTEO, 0.24f }, metal);
        float baliza = 0.55f + 0.45f * std::sin(t * 3.0f);
        DrawCubeV({ px, y0 + JAULA_ALTO_CONTEO + 0.10f, -JAULA_Z_CONTEO }, { 0.18f, 0.16f, 0.18f }, TinteConteo(Color{ 255, 80, 60, 255 }, 0.5f + 0.5f * baliza));

        // Bolardo delantero.
        CajaConBordeConteo({ px, y0 + 0.25f, JAULA_Z_CONTEO }, { 0.26f, 0.50f, 0.26f }, Color{ 196, 156, 64, 255 });
        CajaFacetadaConteo({ px, y0 + 0.34f, JAULA_Z_CONTEO }, { 0.28f, 0.08f, 0.28f }, Color{ 34, 34, 38, 255 });
    }

    // Red trasera y laterales (nunca quedan entre la camara y un dron).
    Color red = Fade(Color{ 150, 200, 255, 255 }, 0.16f);
    const float yTope = y0 + JAULA_ALTO_CONTEO;

    for (int k = 0; k <= 12; k++)
    {
        float x = -JAULA_X_CONTEO + (float)k * (2.0f * JAULA_X_CONTEO / 12.0f);
        DrawLine3D({ x, y0, -JAULA_Z_CONTEO }, { x, yTope, -JAULA_Z_CONTEO }, red);
    }

    for (int k = 0; k <= 6; k++)
    {
        float y = y0 + (float)k * (JAULA_ALTO_CONTEO / 6.0f);
        DrawLine3D({ -JAULA_X_CONTEO, y, -JAULA_Z_CONTEO }, { JAULA_X_CONTEO, y, -JAULA_Z_CONTEO }, red);
        DrawLine3D({ -JAULA_X_CONTEO, y, -JAULA_Z_CONTEO }, { -JAULA_X_CONTEO, y, JAULA_Z_CONTEO }, red);
        DrawLine3D({ JAULA_X_CONTEO, y, -JAULA_Z_CONTEO }, { JAULA_X_CONTEO, y, JAULA_Z_CONTEO }, red);
    }

    for (int k = 0; k <= 8; k++)
    {
        float z = -JAULA_Z_CONTEO + (float)k * (2.0f * JAULA_Z_CONTEO / 8.0f);
        DrawLine3D({ -JAULA_X_CONTEO, y0, z }, { -JAULA_X_CONTEO, yTope, z }, red);
        DrawLine3D({ JAULA_X_CONTEO, y0, z }, { JAULA_X_CONTEO, yTope, z }, red);
    }

    // Aristas de arriba y del frente: solo lineas finas y tenues.
    Color arista = Fade(Color{ 150, 210, 255, 255 }, 0.30f);
    DrawLine3D({ -JAULA_X_CONTEO, yTope, JAULA_Z_CONTEO }, { JAULA_X_CONTEO, yTope, JAULA_Z_CONTEO }, arista);
    DrawLine3D({ -JAULA_X_CONTEO, yTope, -JAULA_Z_CONTEO }, { -JAULA_X_CONTEO, yTope, JAULA_Z_CONTEO }, arista);
    DrawLine3D({ JAULA_X_CONTEO, yTope, -JAULA_Z_CONTEO }, { JAULA_X_CONTEO, yTope, JAULA_Z_CONTEO }, arista);
    DrawLine3D({ -JAULA_X_CONTEO, y0 + 0.5f, JAULA_Z_CONTEO }, { -JAULA_X_CONTEO, yTope, JAULA_Z_CONTEO }, arista);
    DrawLine3D({ JAULA_X_CONTEO, y0 + 0.5f, JAULA_Z_CONTEO }, { JAULA_X_CONTEO, yTope, JAULA_Z_CONTEO }, arista);
}


// Muro del fondo con el porton abierto, costillas y lamparas.
static void DibujarMuroFondoConteo(float t)
{
    const float z = -9.3f;
    const float puertaX = 7.6f;
    const float puertaAlto = 8.0f;
    Color chapa = Color{ 44, 52, 70, 255 };

    CajaFacetadaConteo({ -14.0f, 6.0f, z }, { 12.8f, 12.0f, 0.6f }, chapa, 1.1f);
    CajaFacetadaConteo({ 14.0f, 6.0f, z }, { 12.8f, 12.0f, 0.6f }, chapa, 1.1f);
    CajaFacetadaConteo({ 0.0f, puertaAlto + 2.0f, z }, { 2.0f * puertaX, 4.0f, 0.6f }, chapa, 1.1f);

    // Costillas verticales de la chapa.
    for (int k = 0; k < 8; k++)
    {
        float x = puertaX + 0.8f + (float)k * 1.5f;
        for (int lado = -1; lado <= 1; lado += 2)
        {
            Color c = TinteConteo(chapa, 0.92f + 0.12f * Hash01Conteo(k, lado + 3));
            CajaFacetadaConteo({ (float)lado * x, 6.0f, z + 0.36f }, { 0.22f, 12.0f, 0.12f }, c, 1.1f);
        }
    }

    // Marco del porton con franjas de peligro.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * puertaX;
        CajaConBordeConteo({ x, puertaAlto * 0.5f, z + 0.2f }, { 0.5f, puertaAlto, 0.8f }, Color{ 64, 72, 92, 255 });

        for (int f = 0; f < 6; f++)
        {
            Color c = (f % 2 == 0) ? Color{ 214, 170, 70, 255 } : Color{ 30, 30, 34, 255 };
            DrawCubeV({ x - (float)lado * 0.27f, 0.25f + (float)f * 0.5f, z + 0.3f }, { 0.04f, 0.5f, 0.6f }, c);
        }

        // Hoja del porton corrida, con ventanas.
        CajaConBordeConteo({ x + (float)lado * 2.0f, puertaAlto * 0.5f, z + 0.75f }, { 3.6f, puertaAlto, 0.25f }, Color{ 58, 66, 86, 255 });
        for (int f = 0; f < 2; f++)
        {
            DrawCubeV({ x + (float)lado * 2.0f, 2.2f + (float)f * 2.6f, z + 0.89f }, { 2.6f, 0.9f, 0.04f }, Color{ 30, 44, 70, 255 });
        }
    }

    CajaConBordeConteo({ 0.0f, puertaAlto + 0.2f, z + 0.2f }, { 2.0f * puertaX + 0.5f, 0.4f, 0.8f }, Color{ 64, 72, 92, 255 });

    // Lamparas de pared (carcasa + lente encendida).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 2; k++)
        {
            float x = (float)lado * (puertaX + 1.6f + (float)k * 4.0f);
            CajaFacetadaConteo({ x, 4.6f, z + 0.55f }, { 0.9f, 0.35f, 0.5f }, Color{ 36, 40, 50, 255 });
            float b = 0.92f + 0.08f * std::sin(t * 13.0f + (float)k * 2.1f + (float)lado);
            DrawCubeV({ x, 4.42f, z + 0.62f }, { 0.7f, 0.05f, 0.36f }, TinteConteo(Color{ 255, 220, 160, 255 }, b));
        }
    }
}


// Paredes laterales y columnas con reflectores.
static void DibujarLateralesConteo(float t)
{
    Color chapa = Color{ 40, 47, 64, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 13.0f;
        CajaFacetadaConteo({ x, 6.0f, -1.5f }, { 0.6f, 12.0f, 15.6f }, chapa, 1.1f);

        for (int k = 0; k < 3; k++)
        {
            float z = -6.0f + (float)k * 5.0f;
            float cx = (float)lado * 10.2f;

            // Columna de reticulado: dos perfiles y riostras.
            CajaConBordeConteo({ cx, 4.5f, z }, { 0.42f, 9.0f, 0.42f }, Color{ 70, 78, 96, 255 });
            CajaFacetadaConteo({ cx, 0.15f, z }, { 0.8f, 0.3f, 0.8f }, Color{ 54, 60, 74, 255 });

            // Reflector apuntando a la jaula.
            float rx = cx - (float)lado * 0.45f;
            CajaFacetadaConteo({ rx, 6.2f, z }, { 0.5f, 0.5f, 0.6f }, Color{ 36, 40, 50, 255 });
            float b = 0.9f + 0.1f * std::sin(t * 9.0f + (float)k);
            DrawCubeV({ rx - (float)lado * 0.26f, 6.2f, z }, { 0.04f, 0.36f, 0.44f }, TinteConteo(Color{ 220, 236, 255, 255 }, b));
        }
    }
}


// Utileria a los costados de la plataforma (solo decoracion, nada vuela).
static void DibujarUtileriaConteo(float t)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();
    if (calidad == CALIDAD_BAJA) return;

    // Izquierda: cajones de madera apilados y tambores.
    Color madera = Color{ 128, 92, 54, 255 };
    const Vector3 cajones[] =
    {
        { -8.6f, 0.45f, -2.6f }, { -9.6f, 0.45f, -2.4f }, { -9.1f, 1.35f, -2.5f },
        { -8.7f, 0.45f, 2.9f }
    };

    for (int i = 0; i < 4; i++)
    {
        Color c = TinteConteo(madera, 0.85f + 0.25f * Hash01Conteo(i, 5));
        CajaConBordeConteo(cajones[i], { 0.9f, 0.9f, 0.9f }, c);
        DrawLine3D({ cajones[i].x - 0.45f, cajones[i].y, cajones[i].z + 0.452f }, { cajones[i].x + 0.45f, cajones[i].y, cajones[i].z + 0.452f }, Fade(BLACK, 0.35f));
        DrawLine3D({ cajones[i].x - 0.45f, cajones[i].y - 0.45f, cajones[i].z + 0.452f }, { cajones[i].x + 0.45f, cajones[i].y + 0.45f, cajones[i].z + 0.452f }, Fade(BLACK, 0.25f));
    }

    for (int i = 0; i < 3; i++)
    {
        Vector3 base = { -9.0f + (float)i * 0.62f, 0.0f, 0.6f + (float)(i % 2) * 0.5f };
        Color tambor = i == 1 ? Color{ 60, 110, 150, 255 } : Color{ 160, 66, 46, 255 };
        DibujarSombraRetroCircular({ base.x, 0.4f, base.z }, 0.34f, 0.34f, 0.012f, 0.3f, 8);
        DrawCylinder({ base.x, 0.0f, base.z }, 0.28f, 0.28f, 0.82f, 10, tambor);
        DrawCylinder({ base.x, 0.26f, base.z }, 0.29f, 0.29f, 0.05f, 10, TinteConteo(tambor, 0.7f));
        DrawCylinder({ base.x, 0.58f, base.z }, 0.29f, 0.29f, 0.05f, 10, TinteConteo(tambor, 0.7f));
    }

    // Derecha: consola de control con pantallas de telemetria (ondas, no
    // puntos: no muestran cantidades).
    CajaConBordeConteo({ 8.9f, 0.5f, 0.6f }, { 1.4f, 1.0f, 3.2f }, Color{ 58, 66, 84, 255 });
    CajaFacetadaConteo({ 8.9f, 1.02f, 0.6f }, { 1.5f, 0.06f, 3.3f }, Color{ 80, 90, 110, 255 });

    for (int s = 0; s < 2; s++)
    {
        float zc = -0.1f + (float)s * 1.4f;
        CajaFacetadaConteo({ 9.25f, 1.55f, zc }, { 0.12f, 0.9f, 1.2f }, Color{ 30, 34, 44, 255 });
        DrawCubeV({ 9.18f, 1.55f, zc }, { 0.02f, 0.76f, 1.06f }, Color{ 18, 46, 64, 255 });

        Vector3 anterior = { 9.16f, 1.55f, zc - 0.5f };
        for (int k = 1; k <= 12; k++)
        {
            float u = (float)k / 12.0f;
            float y = 1.55f + 0.22f * std::sin(u * 9.0f + t * (2.0f + (float)s));
            Vector3 punto = { 9.16f, y, zc - 0.5f + u };
            DrawLine3D(anterior, punto, Color{ 110, 230, 210, 255 });
            anterior = punto;
        }
    }

    // Carro de herramientas.
    CajaConBordeConteo({ 8.6f, 0.55f, -3.2f }, { 0.9f, 0.8f, 0.6f }, Color{ 170, 54, 44, 255 });
    for (int k = 0; k < 3; k++)
    {
        DrawLine3D({ 8.15f, 0.3f + (float)k * 0.25f, -2.89f }, { 9.05f, 0.3f + (float)k * 0.25f, -2.89f }, Fade(BLACK, 0.4f));
    }

    if (calidad != CALIDAD_ALTA) return;

    // Mangueras enrolladas y conos (detalle extra).
    AnilloSueloConteo(-8.4f, 4.6f, 0.25f, 0.5f, 0.05f, Color{ 30, 120, 90, 255 }, 14);
    for (int k = 0; k < 3; k++)
    {
        float x = 7.6f + (float)k * 0.7f;
        DrawCylinder({ x, 0.0f, 4.9f }, 0.02f, 0.2f, 0.5f, 8, Color{ 230, 120, 40, 255 });
        DrawCylinder({ x, 0.2f, 4.9f }, 0.12f, 0.13f, 0.07f, 8, Color{ 235, 235, 230, 255 });
    }
}


//==================================================
// VISUAL: DRON (todos iguales)
//==================================================

static Vector3 PosicionVisualDronConteo(const DronConteoExplosivo& dron, float t)
{
    Vector3 p = dron.posicion;
    p.y += std::sin(t * 2.8f + dron.faseFlotacion) * 0.14f * (0.4f + 0.6f * FactorMovimientoConteo());
    return p;
}


static void DibujarDronConteo(const DronConteoExplosivo& dron, float t, float escala)
{
    if (escala <= 0.01f) return;

    Vector3 p = PosicionVisualDronConteo(dron, t);

    // Sombra en la plataforma.
    DibujarSombraRetroCircular({ p.x, ALTO_PLATAFORMA_CONTEO + 0.6f, p.z }, 0.34f * escala, 0.34f * escala, ALTO_PLATAFORMA_CONTEO + 0.012f, 0.22f, 8);

    const Color cuerpo = Color{ 136, 146, 164, 255 };
    const Color brazo = Color{ 66, 74, 90, 255 };
    const Color oscuro = Color{ 28, 30, 38, 255 };

    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(dron.inclinacionX, 1.0f, 0.0f, 0.0f);
    rlRotatef(dron.inclinacionZ, 0.0f, 0.0f, 1.0f);
    rlScalef(escala, escala, escala);

    // Cuerpo y cupula.
    CajaConBordeConteo({ 0.0f, 0.0f, 0.0f }, { 0.36f, 0.13f, 0.36f }, cuerpo);
    CajaFacetadaConteo({ 0.0f, 0.095f, 0.0f }, { 0.22f, 0.06f, 0.22f }, Color{ 74, 84, 104, 255 });
    DrawCubeV({ 0.0f, -0.09f, 0.14f }, { 0.09f, 0.07f, 0.09f }, oscuro);
    DrawCubeV({ 0.0f, -0.09f, 0.186f }, { 0.045f, 0.035f, 0.01f }, Color{ 90, 160, 220, 255 });

    // Patines.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCubeV({ (float)lado * 0.12f, -0.16f, 0.0f }, { 0.03f, 0.03f, 0.40f }, oscuro);
        DrawCubeV({ (float)lado * 0.12f, -0.11f, 0.10f }, { 0.02f, 0.08f, 0.02f }, oscuro);
        DrawCubeV({ (float)lado * 0.12f, -0.11f, -0.10f }, { 0.02f, 0.08f, 0.02f }, oscuro);
    }

    // Brazos en X con motores y helices.
    rlPushMatrix();
    rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    CajaFacetadaConteo({ 0.0f, 0.02f, 0.0f }, { 0.94f, 0.045f, 0.07f }, brazo);
    CajaFacetadaConteo({ 0.0f, 0.02f, 0.0f }, { 0.07f, 0.045f, 0.94f }, brazo);
    rlPopMatrix();

    float giro = (t * 26.0f + dron.faseHelice) * 57.2958f;
    const float d = 0.47f * 0.7071f;
    const Vector3 motores[4] = { { d, 0.0f, d }, { -d, 0.0f, d }, { d, 0.0f, -d }, { -d, 0.0f, -d } };

    for (int m = 0; m < 4; m++)
    {
        Vector3 c = motores[m];
        DrawCylinder({ c.x, -0.02f, c.z }, 0.055f, 0.055f, 0.09f, 8, oscuro);

        rlPushMatrix();
        rlTranslatef(c.x, 0.085f, c.z);
        rlRotatef(m % 2 == 0 ? giro : -giro, 0.0f, 1.0f, 0.0f);
        DrawCubeV({ 0.0f, 0.0f, 0.0f }, { 0.40f, 0.012f, 0.05f }, Color{ 200, 206, 214, 255 });
        rlPopMatrix();
    }

    // Luces de navegacion fijas, iguales en todos: rojo a babor, verde a
    // estribor y blanca arriba.
    DrawCubeV({ -0.19f, 0.0f, 0.0f }, { 0.04f, 0.04f, 0.04f }, Color{ 255, 70, 60, 255 });
    DrawCubeV({ 0.19f, 0.0f, 0.0f }, { 0.04f, 0.04f, 0.04f }, Color{ 70, 255, 120, 255 });
    DrawCubeV({ 0.0f, 0.14f, 0.0f }, { 0.05f, 0.03f, 0.05f }, Color{ 230, 245, 255, 255 });

    rlPopMatrix();
}


// Discos borrosos de las helices (transparentes: van despues de lo opaco).
static void DibujarDiscosHeliceConteo(const DronConteoExplosivo& dron, float t, float escala)
{
    if (escala <= 0.01f) return;

    Vector3 p = PosicionVisualDronConteo(dron, t);
    const float d = 0.47f * 0.7071f;
    const Vector3 motores[4] = { { d, 0.0f, d }, { -d, 0.0f, d }, { d, 0.0f, -d }, { -d, 0.0f, -d } };

    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(dron.inclinacionX, 1.0f, 0.0f, 0.0f);
    rlRotatef(dron.inclinacionZ, 0.0f, 0.0f, 1.0f);
    rlScalef(escala, escala, escala);

    for (int m = 0; m < 4; m++)
    {
        DrawCylinder({ motores[m].x, 0.08f, motores[m].z }, 0.20f, 0.20f, 0.008f, 12, Fade(Color{ 220, 230, 240, 255 }, 0.16f));
    }

    rlPopMatrix();
}


//==================================================
// VISUAL: PERSIANA (tapa la jaula durante la respuesta)
//==================================================

static void DibujarPersianaConteo(float desplazamiento, float t)
{
    const float y0 = ALTO_PLATAFORMA_CONTEO + desplazamiento;
    const float alto = JAULA_ALTO_CONTEO + 0.05f;
    const float ex = JAULA_X_CONTEO + 0.05f;
    const float ez = JAULA_Z_CONTEO + 0.05f;
    const float grosor = 0.08f;
    Color acero = Color{ 50, 58, 76, 255 };

    CajaFacetadaConteo({ 0.0f, y0 + alto * 0.5f, ez }, { 2.0f * ex, alto, grosor }, acero, 1.1f);
    CajaFacetadaConteo({ 0.0f, y0 + alto, 0.0f }, { 2.0f * ex, grosor, 2.0f * ez }, acero, 1.2f);
    CajaFacetadaConteo({ -ex, y0 + alto * 0.5f, 0.0f }, { grosor, alto, 2.0f * ez }, acero, 1.1f);
    CajaFacetadaConteo({ ex, y0 + alto * 0.5f, 0.0f }, { grosor, alto, 2.0f * ez }, acero, 1.1f);
    CajaFacetadaConteo({ 0.0f, y0 + alto * 0.5f, -ez }, { 2.0f * ex, alto, grosor }, acero, 1.1f);

    // Listones horizontales del frente y de los costados.
    Color liston = Fade(BLACK, 0.35f);
    for (int k = 1; k < 12; k++)
    {
        float y = y0 + (float)k * (alto / 12.0f);
        DrawLine3D({ -ex, y, ez + grosor * 0.5f + 0.004f }, { ex, y, ez + grosor * 0.5f + 0.004f }, liston);
        DrawLine3D({ -ex - grosor * 0.5f - 0.004f, y, -ez }, { -ex - grosor * 0.5f - 0.004f, y, ez }, liston);
        DrawLine3D({ ex + grosor * 0.5f + 0.004f, y, -ez }, { ex + grosor * 0.5f + 0.004f, y, ez }, liston);
    }

    // Listones del techo (a lo largo de X).
    for (int k = 1; k < 16; k++)
    {
        float z = -ez + (float)k * (2.0f * ez / 16.0f);
        DrawLine3D({ -ex, y0 + alto + grosor * 0.5f + 0.004f, z }, { ex, y0 + alto + grosor * 0.5f + 0.004f, z }, liston);
    }

    // Zocalo con franjas de peligro.
    int tramos = 24;
    float ancho = 2.0f * ex / (float)tramos;
    for (int k = 0; k < tramos; k++)
    {
        Color c = (k % 2 == 0) ? Color{ 214, 170, 70, 255 } : Color{ 30, 30, 34, 255 };
        DrawCubeV({ -ex + ancho * ((float)k + 0.5f), y0 + 0.2f, ez + grosor * 0.5f + 0.01f }, { ancho, 0.36f, 0.02f }, c);
    }

    // Candados de luz ambar que titilan (todos a la vez).
    float b = 0.5f + 0.5f * std::sin(t * 7.0f);
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCubeV({ (float)lado * (ex - 0.6f), y0 + alto - 0.35f, ez + grosor * 0.5f + 0.02f }, { 0.36f, 0.24f, 0.04f }, TinteConteo(Color{ 255, 150, 60, 255 }, 0.45f + 0.55f * b));
    }
}


//==================================================
// VISUAL: PARTICULAS Y RESPLANDORES
//==================================================

static void DibujarParticulasConteo(const MinijuegoConteoExplosivo& m)
{
    for (int i = 0; i < MAX_PARTICULAS_CONTEO_EXPLOSIVO; i++)
    {
        const ParticulaConteoExplosivo& p = m.particulas[i];
        if (!p.activa) continue;

        float vida = Limitar01Conteo(p.vida / p.vidaMaxima);
        Color c = p.color;

        switch (p.tipo)
        {
            case PARTICULA_CONTEO_CHISPA:
                c.a = (unsigned char)((float)p.color.a * (vida > 0.3f ? 1.0f : vida / 0.3f));
                DrawCubeV(p.posicion, { p.tamano, p.tamano, p.tamano }, c);
                break;

            case PARTICULA_CONTEO_CONFETI:
                c.a = (unsigned char)((float)p.color.a * (vida > 0.25f ? 1.0f : vida / 0.25f));
                rlPushMatrix();
                rlTranslatef(p.posicion.x, p.posicion.y, p.posicion.z);
                rlRotatef(p.giro, 0.6f, 1.0f, 0.3f);
                DrawCubeV({ 0.0f, 0.0f, 0.0f }, { p.tamano, p.tamano * 0.15f, p.tamano * 0.6f }, c);
                rlPopMatrix();
                break;

            case PARTICULA_CONTEO_HUMO:
            case PARTICULA_CONTEO_POLVO:
            {
                c.a = (unsigned char)((float)p.color.a * vida * 0.8f);
                float s = p.tamano * (1.5f - 0.5f * vida);
                if (p.tipo == PARTICULA_CONTEO_HUMO) DrawSphereEx(p.posicion, s, 4, 6, c);
                else DrawCubeV(p.posicion, { s, s, s }, c);
                break;
            }
        }
    }
}


static void DibujarResplandoresConteo(const MinijuegoConteoExplosivo& m, bool dronesVisibles)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    const float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Charcos de luz de los reflectores y de las lamparas de pared.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 3; k++)
        {
            float z = -6.0f + (float)k * 5.0f;
            ResplandorSueloConteo((float)lado * 8.4f, z, 0.012f, 2.6f, Color{ 150, 180, 230, 34 });
        }

        for (int k = 0; k < 2; k++)
        {
            float x = (float)lado * (7.6f + 1.6f + (float)k * 4.0f);
            ResplandorSueloConteo(x, -8.0f, 0.012f, 2.4f, Color{ 255, 200, 130, 46 });
            ResplandorConteo({ x, 4.0f, -8.9f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.6f, Color{ 255, 210, 150, 40 });
        }
    }

    // Luz fria que entra por el porton.
    ResplandorSueloConteo(0.0f, -8.0f, 0.012f, 5.0f, Color{ 80, 110, 190, 40 });

    // Resplandor de la plataforma.
    ResplandorSueloConteo(0.0f, 0.0f, ALTO_PLATAFORMA_CONTEO + 0.012f, 6.5f, Color{ 60, 130, 220, 26 });

    // Luces de borde y de aproximacion.
    for (int i = 0; i < CantidadLucesBordeConteo(); i++)
    {
        float recorrido = 0.0f;
        Vector3 p = PosicionLuzBordeConteo(i, &recorrido);
        float b = BrilloLuzBordeConteo(recorrido, t);
        ResplandorSueloConteo(p.x, p.z, p.y + 0.01f, 0.30f + 0.25f * b, Color{ 255, 170, 70, (unsigned char)(40 + 90 * b) });
    }

    for (int k = 0; k < LUCES_APROXIMACION_CONTEO; k++)
    {
        Vector3 p = PosicionLuzAproximacionConteo(k);
        if (p.z < -8.8f) break;
        float b = BrilloLuzAproximacionConteo(k, t);
        ResplandorSueloConteo(p.x, p.z, 0.02f, 0.3f + 0.45f * b, Color{ 150, 200, 255, (unsigned char)(30 + 110 * b) });
    }

    // Balizas de los postes traseros.
    float baliza = 0.55f + 0.45f * std::sin(t * 3.0f);
    for (int lado = -1; lado <= 1; lado += 2)
    {
        ResplandorFrontalConteo({ (float)lado * JAULA_X_CONTEO, ALTO_PLATAFORMA_CONTEO + JAULA_ALTO_CONTEO + 0.1f, -JAULA_Z_CONTEO + 0.15f }, 0.55f, Color{ 255, 70, 50, (unsigned char)(60 + 80 * baliza) });
    }

    // Pantallas de la consola.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        ResplandorSueloConteo(8.6f, 0.6f, 0.012f, 1.6f, Color{ 60, 200, 190, 36 });
    }

    // Drones: estela y halo de la luz superior (iguales en todos).
    if (dronesVisibles)
    {
        for (int i = 0; i < m.cantidadDrones; i++)
        {
            float escala = 1.0f;
            if (m.fase == FASE_CONTEO_TERMINADO)
            {
                if (i >= m.dronesRevelados) continue;
                float edad = m.tiempoFase - TiempoAparicionDronConteo(i, m.cantidadDrones);
                escala = Limitar01Conteo(EaseOutBackConteo(edad / 0.28f));

                // Destello al reaparecer.
                if (edad < 0.3f)
                {
                    Vector3 p = PosicionVisualDronConteo(m.drones[i], t);
                    ResplandorFrontalConteo(p, 0.9f * (1.0f - edad / 0.3f) + 0.2f, Color{ 160, 220, 255, (unsigned char)(170 * (1.0f - edad / 0.3f)) });
                }
            }

            const DronConteoExplosivo& dron = m.drones[i];
            Vector3 p = PosicionVisualDronConteo(dron, t);
            ResplandorFrontalConteo({ p.x, p.y + 0.15f * escala, p.z }, 0.22f * escala, Color{ 220, 240, 255, 90 });
            ResplandorSueloConteo(p.x, p.z, ALTO_PLATAFORMA_CONTEO + 0.016f, 0.55f * escala, Color{ 90, 170, 255, 34 });

            if (m.fase != FASE_CONTEO_TERMINADO)
            {
                for (int k = 1; k <= 5; k++)
                {
                    float f = 1.0f - (float)k / 6.0f;
                    Vector3 q = { p.x - dron.velocidad.x * 0.09f * (float)k, p.y - 0.02f, p.z - dron.velocidad.z * 0.09f * (float)k };
                    float s = 0.13f * f;
                    DrawCubeV(q, { s, s * 0.5f, s }, Color{ 110, 200, 255, (unsigned char)(70.0f * f) });
                }
            }
        }
    }

    // Chispas con brillo extra.
    for (int i = 0; i < MAX_PARTICULAS_CONTEO_EXPLOSIVO; i++)
    {
        const ParticulaConteoExplosivo& p = m.particulas[i];
        if (!p.activa || p.tipo != PARTICULA_CONTEO_CHISPA) continue;
        float vida = Limitar01Conteo(p.vida / p.vidaMaxima);
        ResplandorFrontalConteo(p.posicion, 0.14f, Color{ 150, 220, 255, (unsigned char)(120 * vida) });
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Conos de los reflectores (muy tenues; aditivos, no tapan nada).
static void DibujarConosLuzConteo()
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 3; k++)
        {
            float z = -6.0f + (float)k * 5.0f;
            float rx = (float)lado * 9.5f;
            DrawCylinderEx({ rx, 6.2f, z }, { (float)lado * 7.6f, 0.05f, z }, 0.18f, 1.6f, 10, Color{ 120, 150, 200, 10 });
        }
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoConteoExplosivo::Dibujar(
    const Participante participantes[]
) const
{
    const float t = tiempoAnimacion;
    const float movimiento = FactorMovimientoConteo();

    ClearBackground(Color{ 12, 14, 24, 255 });

    // Persiana: cae al empezar la respuesta y sube al revelar.
    float desplazamientoPersiana = -1.0f;   // < 0: no se dibuja

    if (fase == FASE_CONTEO_RESPUESTA)
    {
        float k = Limitar01Conteo((DURACION_RESPUESTA_CONTEO - tiempoFase) / T_CAIDA_PERSIANA_CONTEO);
        desplazamientoPersiana = (1.0f - k * k) * 4.8f * movimiento;
    }
    else if (fase == FASE_CONTEO_TERMINADO && tiempoFase < T_SUBIDA_PERSIANA_CONTEO)
    {
        float k = tiempoFase / T_SUBIDA_PERSIANA_CONTEO;
        desplazamientoPersiana = movimiento > 0.0f ? k * k * 5.2f : -1.0f;
    }

    bool dronesVisibles =
        fase == FASE_CONTEO_PREPARACION ||
        fase == FASE_CONTEO_OBSERVACION ||
        fase == FASE_CONTEO_TERMINADO;

    BeginMode3D(camara);
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    DibujarExteriorConteo(t);
    DibujarSueloConteo();
    DibujarMuroFondoConteo(t);
    DibujarLateralesConteo(t);
    DibujarUtileriaConteo(t);
    DibujarPlataformaConteo(t);
    DibujarJaulaConteo(t);

    if (dronesVisibles)
    {
        for (int i = 0; i < cantidadDrones; i++)
        {
            float escala = 1.0f;

            if (fase == FASE_CONTEO_TERMINADO)
            {
                if (i >= dronesRevelados) continue;
                float edad = tiempoFase - TiempoAparicionDronConteo(i, cantidadDrones);
                escala = movimiento > 0.0f ? EaseOutBackConteo(edad / 0.28f) : 1.0f;
            }

            DibujarDronConteo(drones[i], t, escala);
        }
    }

    if (desplazamientoPersiana >= 0.0f)
    {
        DibujarPersianaConteo(desplazamientoPersiana, t);
    }

    DibujarParticulasConteo(*this);

    if (dronesVisibles)
    {
        for (int i = 0; i < cantidadDrones; i++)
        {
            if (fase == FASE_CONTEO_TERMINADO && i >= dronesRevelados) continue;
            float escala = 1.0f;
            if (fase == FASE_CONTEO_TERMINADO && movimiento > 0.0f)
            {
                escala = EaseOutBackConteo((tiempoFase - TiempoAparicionDronConteo(i, cantidadDrones)) / 0.28f);
            }
            DibujarDiscosHeliceConteo(drones[i], t, escala);
        }
    }

    DibujarConosLuzConteo();
    DibujarResplandoresConteo(*this, dronesVisibles);

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    EndMode3D();

    //----------------------------------------------
    // Textos sobre el mundo
    //----------------------------------------------
    const float centroX = (float)GetScreenWidth() * 0.5f;

    if (fase == FASE_CONTEO_RESPUESTA)
    {
        float desde = DURACION_RESPUESTA_CONTEO - tiempoFase;
        float alfa = Limitar01Conteo((desde - 0.15f) / 0.25f);
        float y = RectRef(0.0f, Y_TITULO_RESPUESTA_CONTEO, 0.0f, 0.0f).y;
        DibujarTextoMundoContornoUI("¿CUÁNTOS VISTE?", centroX, y, UI_TEXTO_TITULO, UI_FOCO, alfa);
        DibujarTextoMundoContornoUI("TU NÚMERO QUEDA FIJO AL TERMINAR EL TIEMPO", centroX, y + Px(62.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }
    else if (fase == FASE_CONTEO_TERMINADO && tiempoFase < T_FIN_REVELACION_CONTEO + 0.3f)
    {
        float alfa = 1.0f - Limitar01Conteo((tiempoFase - T_FIN_REVELACION_CONTEO) / 0.3f);
        float y = RectRef(0.0f, Y_TITULO_REVELACION_CONTEO, 0.0f, 0.0f).y;

        // Numero sobre cada dron al reaparecer.
        for (int i = 0; i < dronesRevelados && !totalRevelado; i++)
        {
            float edad = tiempoFase - TiempoAparicionDronConteo(i, cantidadDrones);
            if (edad > 0.7f) continue;

            Vector3 p = PosicionVisualDronConteo(drones[i], t);
            Vector2 pantalla = GetWorldToScreen({ p.x, p.y + 0.45f, p.z }, camara);
            char numero[8];
            std::snprintf(numero, sizeof(numero), "%d", i + 1);
            DibujarTextoMundoContornoUI(numero, pantalla.x, pantalla.y - Px(10.0f + 14.0f * edad), UI_TEXTO, UI_TEXTO_MUNDO, 1.0f - edad / 0.7f);
        }

        if (!totalRevelado)
        {
            if (dronesRevelados > 0)
            {
                char contador[8];
                std::snprintf(contador, sizeof(contador), "%d", dronesRevelados);
                float edad = tiempoFase - TiempoAparicionDronConteo(dronesRevelados - 1, cantidadDrones);
                int token = (int)(72.0f + 14.0f * Limitar01Conteo(1.0f - edad / 0.12f) * movimiento);
                DibujarTextoMundoContornoUI(contador, centroX, y - Px(8.0f), token, UI_TEXTO_MUNDO);
            }
        }
        else
        {
            float edad = tiempoFase - T_TOTAL_REVELADO_CONTEO;
            float pulso = Limitar01Conteo(1.0f - edad / 0.25f) * movimiento;
            char total[32];
            std::snprintf(total, sizeof(total), "HABIA %d DRONES", cantidadDrones);
            DibujarTextoMundoContornoUI(total, centroX, y, (int)(UI_TEXTO_TITULO + 12.0f * pulso), UI_FOCO, alfa);

            // Quienes acertaron justo.
            char linea[64] = "";
            int exactos[MAX_PARTICIPANTES];
            int cantidadExactos = 0;

            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (resultado.participantes[i].participo && jugadores[i].diferencia == 0)
                {
                    exactos[cantidadExactos++] = participantes[i].numeroJugador;
                }
            }

            if (cantidadExactos == 0)
            {
                std::snprintf(linea, sizeof(linea), "NADIE ACERTÓ: GANA QUIEN QUEDÓ MÁS CERCA");
            }
            else
            {
                int usado = std::snprintf(linea, sizeof(linea), "¡EXACTO: ");
                for (int k = 0; k < cantidadExactos && usado < (int)sizeof(linea); k++)
                {
                    const char* union_ = k == 0 ? "" : (k == cantidadExactos - 1 ? " Y " : ", ");
                    usado += std::snprintf(linea + usado, sizeof(linea) - usado, "%sJ%d", union_, exactos[k]);
                }
                if (usado < (int)sizeof(linea)) std::snprintf(linea + usado, sizeof(linea) - usado, "!");
            }

            DibujarTextoMundoContornoUI(linea, centroX, y + Px(68.0f), UI_TEXTO_GRANDE, cantidadExactos > 0 ? UI_POSITIVO : UI_TEXTO_MUNDO, alfa);
        }
    }

    //----------------------------------------------
    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    //----------------------------------------------
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CONTEO EXPLOSIVO",
        "CONTÁ LOS DRONES DE LA JAULA. GANA QUIEN DIGA EL NÚMERO MÁS CERCANO AL TOTAL.",
        resultado
    );

    bool revelando = fase == FASE_CONTEO_TERMINADO && tiempoFase < T_FIN_REVELACION_CONTEO;

    if (fase == FASE_CONTEO_PREPARACION)
    {
        hud.fase = HUD_MINIJUEGO_CUENTA;
    }
    else if (fase == FASE_CONTEO_TERMINADO && !revelando)
    {
        hud.fase = HUD_MINIJUEGO_FIN;
    }
    else
    {
        hud.fase = HUD_MINIJUEGO_JUGANDO;
    }

    hud.cuentaRestante = fase == FASE_CONTEO_PREPARACION ? tiempoFase : 0.0f;

    if (fase == FASE_CONTEO_TERMINADO)
    {
        // Durante la revelacion el cartel del medio cuenta los drones.
        hud.reloj = HUD_RELOJ_MEDIDOR;
        hud.etiquetaMedidor = "DRONES";
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d", dronesRevelados);
        hud.progresoMedidor = cantidadDrones > 0 ? (float)dronesRevelados / (float)cantidadDrones : 0.0f;
    }
    else
    {
        hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
        hud.tiempoRestante = tiempoFase > 0.0f ? tiempoFase : 0.0f;
        hud.tiempoTotal = fase == FASE_CONTEO_RESPUESTA ? DURACION_RESPUESTA_CONTEO : DURACION_OBSERVACION_CONTEO;
        hud.segundosAlerta = fase == FASE_CONTEO_RESPUESTA ? 3.0f : 0.0f;
    }

    hud.finPorTiempo = false;

    bool hayEmpate = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const Participante& participante = participantes[i];
        const EstadoJugadorConteoExplosivo& estado = jugadores[i];
        bool esHumano = !participante.esBot && participante.conectado;

        switch (fase)
        {
            case FASE_CONTEO_PREPARACION:
            case FASE_CONTEO_OBSERVACION:
                std::snprintf(jugador.estado, sizeof(jugador.estado), "CONTANDO");
                break;

            case FASE_CONTEO_RESPUESTA:
                if (esHumano)
                {
                    // Cada humano ve solo su numero, en su etiqueta.
                    std::snprintf(jugador.valor, sizeof(jugador.valor), "%02d", estado.respuesta);
                    jugador.valorNumerico = estado.respuesta;

                    std::snprintf(
                        jugador.controles,
                        sizeof(jugador.controles),
                        "+1: %s   -1: %s\nLADOS 1  ARRIBA/ABAJO 5",
                        TextoBotonGolpeHud(participante),
                        ObtenerTextoBotonPrincipal(participante)
                    );
                }
                else
                {
                    // Bots: nunca su numero antes de revelar.
                    std::snprintf(jugador.valor, sizeof(jugador.valor), "?");
                    std::snprintf(jugador.estado, sizeof(jugador.estado), estado.botRespondio ? "LISTO" : "PENSANDO");
                    jugador.valorNumerico = estado.botRespondio ? 1 : 0;
                }
                break;

            case FASE_CONTEO_TERMINADO:
            {
                int signado = estado.respuesta - cantidadDrones;
                std::snprintf(jugador.valor, sizeof(jugador.valor), "%d", estado.respuesta);
                jugador.valorNumerico = totalRevelado ? 1 : 0;

                if (totalRevelado)
                {
                    if (signado == 0)
                        std::snprintf(jugador.estado, sizeof(jugador.estado), "¡EXACTO!");
                    else if (signado > 0)
                        std::snprintf(jugador.estado, sizeof(jugador.estado), "SE PASÓ POR %d", signado);
                    else
                        std::snprintf(jugador.estado, sizeof(jugador.estado), "LE FALTARON %d", -signado);
                }

                if (signado == 0)
                    std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "DIJO %d: EXACTO", estado.respuesta);
                else
                    std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "DIJO %d (%+d)", estado.respuesta, signado);

                for (int j = 0; j < MAX_PARTICIPANTES; j++)
                {
                    if (j != i && resultado.participantes[j].participo && jugadores[j].diferencia == estado.diferencia)
                    {
                        hayEmpate = true;
                        std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "A %d DEL TOTAL", estado.diferencia);
                        break;
                    }
                }
                break;
            }
        }
    }

    // Nota al pie del tablon (buffer propio: nunca un puntero de TextFormat).
    static char notaFinal[96];
    if (fase == FASE_CONTEO_TERMINADO)
    {
        std::snprintf(
            notaFinal,
            sizeof(notaFinal),
            hayEmpate
                ? "HABIA %d DRONES. A IGUAL DISTANCIA DEL TOTAL SE COMPARTE EL PUESTO."
                : "HABIA %d DRONES. GANA QUIEN QUEDÓ MÁS CERCA.",
            cantidadDrones
        );
        hud.notaFinal = notaFinal;
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoConteoExplosivo::ObtenerResultado() const
{
    return resultado;
}
