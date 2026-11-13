#include "Minigames/MinijuegoNucleosEnergia.h"

#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_NUCLEOS = 3.0f;
static const int PUNTOS_OBJETIVO_NUCLEOS = 15;
static const float DURACION_PARTIDA_NUCLEOS = 60.0f;
static const float RADIO_RECOLECCION_NUCLEO = 0.82f;
static const float RETRASO_REAPARICION_NUCLEO = 0.38f;
static const float RADIO_RECOLECCION_CAIDO = 0.76f;
static const float GRAVEDAD_NUCLEO_CAIDO = 12.5f;
static const float ALTURA_SUELO_NUCLEO_CAIDO = 0.34f;
static const float VIDA_NUCLEO_CAIDO = 8.0f;
static const float BLOQUEO_DUENIO_NUCLEO_CAIDO = 1.75f;


static const Vector3 PUNTOS_NUCLEOS[] =
{
    { -4.8f, 0.82f, -4.5f }, { 0.0f, 0.82f, -4.8f }, { 4.8f, 0.82f, -4.5f },
    { -4.8f, 0.82f, 0.0f }, { 0.0f, 0.82f, 0.0f }, { 4.8f, 0.82f, 0.0f },
    { -4.8f, 0.82f, 4.5f }, { 0.0f, 0.82f, 4.8f }, { 4.8f, 0.82f, 4.5f },
    { -2.5f, 0.82f, -2.4f }, { 2.5f, 0.82f, -2.4f },
    { -2.5f, 0.82f, 2.4f }, { 2.5f, 0.82f, 2.4f },
    { 0.0f, 0.82f, -2.6f }, { 0.0f, 0.82f, 2.6f },
    { -2.7f, 0.82f, 0.0f }, { 2.7f, 0.82f, 0.0f }
};

static const int CANTIDAD_PUNTOS_NUCLEOS =
    sizeof(PUNTOS_NUCLEOS) / sizeof(PUNTOS_NUCLEOS[0]);


static int ValorNucleo(bool especial)
{
    return especial ? 3 : 1;
}


//==================================================
// VISUAL: PALETA Y EFECTOS (no afectan reglas)
//==================================================

// Nucleo normal: cian electrico. Especial: oro. Lo decorativo de la sala
// usa violeta, acero y luces chicas rojas/verdes para no confundirse.
static const Color COLOR_NUCLEO_NORMAL = { 86, 206, 255, 255 };
static const Color COLOR_NUCLEO_ESPECIAL = { 255, 204, 64, 255 };
static const Color COLOR_LUZ_SALA_NUCLEOS = { 156, 92, 255, 255 };


static Color ColorNucleo(bool especial)
{
    return especial ? COLOR_NUCLEO_ESPECIAL : COLOR_NUCLEO_NORMAL;
}


// Azar propio de los efectos: no consume GetRandomValue, asi la secuencia
// aleatoria del juego (posicion y tipo de los nucleos) no cambia por lo visual.
static float AzarNucleos(float minimo, float maximo)
{
    static unsigned int estado = 0x9E3779B9u;
    estado = estado * 1664525u + 1013904223u;
    float u = (float)((estado >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasNucleos(int cantidad)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    int escalada = (int)std::lround((float)cantidad * factor);
    return escalada < 1 ? 1 : escalada;
}


static ParticulaNucleos* ReservarParticulaNucleos(MinijuegoNucleosEnergia& m)
{
    for (int i = 0; i < MAX_PARTICULAS_NUCLEOS; i++)
    {
        if (!m.particulasLocales[i].activa) return &m.particulasLocales[i];
    }

    return nullptr;
}


// Chispas del color del nucleo que saltan hacia arriba y caen rebotando.
static void EmitirChispasNucleos(
    MinijuegoNucleosEnergia& m,
    Vector3 posicion,
    Color color,
    int cantidad,
    float fuerza
)
{
    int total = CantidadParticulasNucleos(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaNucleos* p = ReservarParticulaNucleos(m);
        if (p == nullptr) return;

        float angulo = AzarNucleos(0.0f, 6.2832f);
        float velocidad = AzarNucleos(0.35f, 1.0f) * fuerza;
        float mezcla = AzarNucleos(0.0f, 0.55f);

        *p = {};
        p->activa = true;
        p->gravedad = true;
        p->posicion = posicion;
        p->velocidad =
        {
            std::cos(angulo) * velocidad,
            AzarNucleos(1.6f, 4.4f),
            std::sin(angulo) * velocidad
        };
        p->vidaMaxima = AzarNucleos(0.35f, 0.75f);
        p->vida = p->vidaMaxima;
        p->tamano = AzarNucleos(0.05f, 0.10f);
        p->color = ColorLerp(color, WHITE, mezcla);
    }
}


static void AgregarDestelloNucleos(
    MinijuegoNucleosEnergia& m,
    Vector3 posicion,
    Color color,
    float radio,
    float duracion
)
{
    int elegido = 0;
    float menorVida = 100000.0f;

    for (int i = 0; i < MAX_DESTELLOS_NUCLEOS; i++)
    {
        if (!m.destellos[i].activo) { elegido = i; break; }
        if (m.destellos[i].vida < menorVida)
        {
            menorVida = m.destellos[i].vida;
            elegido = i;
        }
    }

    DestelloNucleos& d = m.destellos[elegido];
    d.activo = true;
    d.posicion = posicion;
    d.color = color;
    d.radio = radio;
    d.vidaMaxima = duracion;
    d.vida = duracion;
}


static void AgregarAvisoNucleos(
    MinijuegoNucleosEnergia& m,
    Vector3 posicion,
    int valor,
    bool especial
)
{
    int elegido = 0;
    float menorVida = 100000.0f;

    for (int i = 0; i < MAX_AVISOS_NUCLEOS; i++)
    {
        if (!m.avisos[i].activo) { elegido = i; break; }
        if (m.avisos[i].vida < menorVida)
        {
            menorVida = m.avisos[i].vida;
            elegido = i;
        }
    }

    AvisoNucleos& a = m.avisos[elegido];
    a.activo = true;
    a.posicion = posicion;
    a.vida = 1.0f;
    a.valor = valor;
    a.especial = especial;
}


// Motas violetas que suben de los generadores del fondo (fuera de la arena,
// nunca con forma ni color de nucleo).
static void ActualizarAmbienteNucleos(MinijuegoNucleosEnergia& m, float deltaTime)
{
    if (FactorCalidadGrafica(CalidadParticulas()) < 0.5f) return;

    m.temporizadorAmbiente -= deltaTime;
    if (m.temporizadorAmbiente > 0.0f) return;
    m.temporizadorAmbiente = AzarNucleos(0.10f, 0.22f);

    ParticulaNucleos* p = ReservarParticulaNucleos(m);
    if (p == nullptr) return;

    const float xGeneradores[3] = { -4.6f, 0.0f, 4.6f };
    *p = {};
    p->activa = true;
    p->gravedad = false;
    p->posicion =
    {
        xGeneradores[(int)AzarNucleos(0.0f, 2.99f)] + AzarNucleos(-0.8f, 0.8f),
        AzarNucleos(1.4f, 2.6f),
        -9.4f + AzarNucleos(-0.6f, 0.6f)
    };
    p->velocidad = { AzarNucleos(-0.15f, 0.15f), AzarNucleos(0.5f, 1.1f), AzarNucleos(-0.05f, 0.05f) };
    p->vidaMaxima = AzarNucleos(1.4f, 2.4f);
    p->vida = p->vidaMaxima;
    p->tamano = AzarNucleos(0.05f, 0.08f);
    p->color = ColorLerp(COLOR_LUZ_SALA_NUCLEOS, WHITE, AzarNucleos(0.0f, 0.4f));
    p->color.a = 190;
}


static void ActualizarVisualesNucleos(MinijuegoNucleosEnergia& m, float deltaTime)
{
    ActualizarAmbienteNucleos(m, deltaTime);

    for (int i = 0; i < MAX_PARTICULAS_NUCLEOS; i++)
    {
        ParticulaNucleos& p = m.particulasLocales[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        if (p.gravedad) p.velocidad.y -= 11.0f * deltaTime;

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (p.gravedad && p.posicion.y < 0.05f)
        {
            p.posicion.y = 0.05f;
            p.velocidad.y *= -0.35f;
            p.velocidad.x *= 0.6f;
            p.velocidad.z *= 0.6f;
        }
    }

    for (int i = 0; i < MAX_DESTELLOS_NUCLEOS; i++)
    {
        DestelloNucleos& d = m.destellos[i];
        if (!d.activo) continue;
        d.vida -= deltaTime;
        if (d.vida <= 0.0f) d.activo = false;
    }

    for (int i = 0; i < MAX_AVISOS_NUCLEOS; i++)
    {
        AvisoNucleos& a = m.avisos[i];
        if (!a.activo) continue;
        a.vida -= deltaTime;
        if (a.vida <= 0.0f) a.activo = false;
    }

    // Estela de los nucleos que salen despedidos por un golpe.
    if (FactorCalidadGrafica(CalidadParticulas()) >= 0.5f)
    {
        for (int n = 0; n < MAX_NUCLEOS_CAIDOS; n++)
        {
            const NucleoEnergiaCaido& caido = m.nucleosCaidos[n];
            if (!caido.activo || caido.posicion.y < ALTURA_SUELO_NUCLEO_CAIDO + 0.25f) continue;
            if (AzarNucleos(0.0f, 1.0f) > 0.55f) continue;

            ParticulaNucleos* p = ReservarParticulaNucleos(m);
            if (p == nullptr) break;

            *p = {};
            p->activa = true;
            p->gravedad = false;
            p->posicion = caido.posicion;
            p->velocidad = { 0.0f, 0.2f, 0.0f };
            p->vidaMaxima = 0.28f;
            p->vida = p->vidaMaxima;
            p->tamano = caido.especial ? 0.10f : 0.08f;
            p->color = ColorLerp(ColorNucleo(caido.especial), WHITE, 0.3f);
        }
    }
}


static bool PuntoOcupadoPorOtroNucleo(
    const MinijuegoNucleosEnergia& minijuego,
    int indiceIgnorado,
    Vector3 punto
)
{
    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        if (i == indiceIgnorado || !minijuego.nucleos[i].activo) continue;

        float dx = minijuego.nucleos[i].posicion.x - punto.x;
        float dz = minijuego.nucleos[i].posicion.z - punto.z;
        if (dx * dx + dz * dz < 1.6f) return true;
    }

    return false;
}


// Un nucleo no debe reaparecer debajo de un jugador: se lo llevaria en el
// mismo frame sin moverse (punto gratis).
static bool PuntoOcupadoPorJugador(
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    Vector3 punto
)
{
    if (jugadores == nullptr || participantes == nullptr)
    {
        return false;
    }

    const float RADIO_LIBRE = RADIO_RECOLECCION_NUCLEO + 0.6f;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo || jugadores[i].cayendo) continue;

        float dx = jugadores[i].posicion.x - punto.x;
        float dz = jugadores[i].posicion.z - punto.z;
        if (dx * dx + dz * dz < RADIO_LIBRE * RADIO_LIBRE) return true;
    }

    return false;
}


static void ReubicarNucleo(
    MinijuegoNucleosEnergia& minijuego,
    int indice,
    const JugadorPrueba jugadores[] = nullptr,
    const Participante participantes[] = nullptr,
    int limite = 0
)
{
    int puntoElegido = GetRandomValue(0, CANTIDAD_PUNTOS_NUCLEOS - 1);

    for (int intento = 0; intento < 24; intento++)
    {
        int candidato = GetRandomValue(0, CANTIDAD_PUNTOS_NUCLEOS - 1);

        if (
            !PuntoOcupadoPorOtroNucleo(minijuego, indice, PUNTOS_NUCLEOS[candidato]) &&
            !PuntoOcupadoPorJugador(jugadores, participantes, limite, PUNTOS_NUCLEOS[candidato])
        )
        {
            puntoElegido = candidato;
            break;
        }
    }

    NucleoEnergia& nucleo = minijuego.nucleos[indice];
    nucleo.posicion = PUNTOS_NUCLEOS[puntoElegido];
    nucleo.especial = GetRandomValue(1, 100) <= 22;
    nucleo.activo = true;
    nucleo.tiempoReaparicion = 0.0f;
    nucleo.faseFlotacion = (float)GetRandomValue(0, 628) / 100.0f;
    nucleo.tiempoAparicion = minijuego.tiempoAnimacion;
}


static void ConfigurarArenaNucleos(MinijuegoNucleosEnergia& minijuego)
{
    minijuego.cantidadBloques = 0;

    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_NUCLEOS,
        { 0.0f, -0.45f, 0.0f }, { 14.0f, 0.90f, 14.0f }, Color{ 44, 62, 83, 255 });
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_NUCLEOS,
        { -7.15f, 0.55f, 0.0f }, { 0.30f, 2.0f, 14.6f }, Color{ 45, 125, 154, 255 });
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_NUCLEOS,
        { 7.15f, 0.55f, 0.0f }, { 0.30f, 2.0f, 14.6f }, Color{ 45, 125, 154, 255 });
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_NUCLEOS,
        { 0.0f, 0.55f, -7.15f }, { 14.0f, 2.0f, 0.30f }, Color{ 45, 125, 154, 255 });
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques, MAX_BLOQUES_NUCLEOS,
        { 0.0f, 0.55f, 7.15f }, { 14.0f, 2.0f, 0.30f }, Color{ 45, 125, 154, 255 });
}


static void AgregarAlHistorialNucleos(
    MinijuegoNucleosEnergia& minijuego,
    int jugador,
    bool especial
)
{
    if (jugador < 0 || jugador >= MAX_PARTICIPANTES) return;

    HistorialNucleosJugador& historial = minijuego.historial[jugador];

    if (historial.cantidad >= MAX_HISTORIAL_NUCLEOS)
    {
        for (int i = 1; i < MAX_HISTORIAL_NUCLEOS; i++)
            historial.especiales[i - 1] = historial.especiales[i];

        historial.cantidad = MAX_HISTORIAL_NUCLEOS - 1;
    }

    historial.especiales[historial.cantidad++] = especial;
}


static int BuscarSlotNucleoCaido(MinijuegoNucleosEnergia& minijuego)
{
    int mejor = 0;
    float menorVida = 100000.0f;

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++)
    {
        if (!minijuego.nucleosCaidos[i].activo) return i;

        if (minijuego.nucleosCaidos[i].tiempoVida < menorVida)
        {
            menorVida = minijuego.nucleosCaidos[i].tiempoVida;
            mejor = i;
        }
    }

    return mejor;
}


static void SoltarNucleoPorGolpe(
    MinijuegoNucleosEnergia& minijuego,
    int jugador,
    Vector3 origen,
    bool especial,
    int indiceSalida
)
{
    int slot = BuscarSlotNucleoCaido(minijuego);
    NucleoEnergiaCaido& caido = minijuego.nucleosCaidos[slot];

    float angulo =
        (float)GetRandomValue(0, 6283) / 1000.0f + indiceSalida * 1.27f;
    float fuerza = (float)GetRandomValue(30, 48) / 10.0f;

    caido = {};
    caido.activo = true;
    caido.especial = especial;
    caido.jugadorBloqueado = jugador;
    caido.tiempoBloqueo = BLOQUEO_DUENIO_NUCLEO_CAIDO;
    caido.tiempoVida = VIDA_NUCLEO_CAIDO;

    // Se separa el nucleo del centro del jugador antes de empezar a comprobar
    // recogidas. Esto, junto al bloqueo del duenio, evita que lo absorba en el
    // mismo frame en que sale despedido.
    caido.posicion =
    {
        origen.x + std::cos(angulo) * 0.55f,
        origen.y + 0.55f,
        origen.z + std::sin(angulo) * 0.55f
    };

    caido.velocidad =
    {
        std::cos(angulo) * fuerza,
        (float)GetRandomValue(50, 68) / 10.0f,
        std::sin(angulo) * fuerza
    };
}


static void PerderUltimosNucleosPorGolpe(
    MinijuegoNucleosEnergia& minijuego,
    int jugador,
    Vector3 posicionJugador,
    int maximoPerder,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (jugador < 0 || jugador >= MAX_PARTICIPANTES || maximoPerder <= 0) return;

    HistorialNucleosJugador& historial = minijuego.historial[jugador];
    int cantidadPerder = historial.cantidad < maximoPerder
        ? historial.cantidad
        : maximoPerder;

    int puntosAntes = minijuego.puntuaciones[jugador];
    bool soltoEspecial = false;

    for (int i = 0; i < cantidadPerder; i++)
    {
        int indiceHistorial = historial.cantidad - 1;
        bool especial = historial.especiales[indiceHistorial];
        historial.cantidad--;
        minijuego.puntuaciones[jugador] -= ValorNucleo(especial);
        if (minijuego.puntuaciones[jugador] < 0) minijuego.puntuaciones[jugador] = 0;

        SoltarNucleoPorGolpe(
            minijuego,
            jugador,
            posicionJugador,
            especial,
            i
        );

        // Chispas del color de cada nucleo que se escapa.
        EmitirChispasNucleos(minijuego, posicionJugador, ColorNucleo(especial), 7, 3.2f);
        soltoEspecial = soltoEspecial || especial;
    }

    if (cantidadPerder > 0)
    {
        CrearParticulasImpactoGolpe(
            particulas,
            cantidadParticulas,
            posicionJugador
        );

        // Destello blanco-rojizo en el impacto, numero que baja y un temblor
        // leve (ActivarTemblorCamaraGeneral respeta "reducir movimiento").
        AgregarDestelloNucleos(
            minijuego,
            { posicionJugador.x, 0.0f, posicionJugador.z },
            Color{ 255, 120, 96, 255 },
            1.9f,
            0.42f
        );
        AgregarAvisoNucleos(
            minijuego,
            posicionJugador,
            minijuego.puntuaciones[jugador] - puntosAntes,
            soltoEspecial
        );
        ActivarTemblorCamaraGeneral(0.10f, 0.20f);
    }
}


static bool JugadorPuedeRecogerNucleo(
    const Participante& participante,
    const JugadorPrueba& jugador
)
{
    return
        participante.activo &&
        participante.conectado &&
        !jugador.cayendo;
}


static bool JugadorTocaPuntoNucleo(
    const JugadorPrueba& jugador,
    Vector3 punto,
    float radio
)
{
    float dx = jugador.posicion.x - punto.x;
    float dz = jugador.posicion.z - punto.z;
    float diferenciaY = std::fabs(jugador.posicion.y - punto.y);

    return dx * dx + dz * dz <= radio * radio && diferenciaY <= 1.25f;
}


static void RegistrarRecogidaNucleo(
    MinijuegoNucleosEnergia& minijuego,
    int jugador,
    bool especial,
    Vector3 posicion,
    ParticulaTierra particulas[],
    int cantidadParticulas,
    AudioJuego* audio
)
{
    minijuego.puntuaciones[jugador] += ValorNucleo(especial);
    AgregarAlHistorialNucleos(minijuego, jugador, especial);

    if (audio != nullptr)
    {
        audio->ReproducirSonido(
            especial ? SONIDO_RECOGER_NUCLEO_ESPECIAL : SONIDO_RECOGER_NUCLEO
        );
    }

    CrearParticulasImpactoGolpe(particulas, cantidadParticulas, posicion);

    // Chispas del color del nucleo, onda de luz en el suelo y "+N".
    Color color = ColorNucleo(especial);
    EmitirChispasNucleos(minijuego, posicion, color, especial ? 26 : 14, especial ? 3.0f : 2.2f);
    AgregarDestelloNucleos(
        minijuego,
        { posicion.x, 0.0f, posicion.z },
        color,
        especial ? 2.1f : 1.4f,
        especial ? 0.55f : 0.38f
    );
    AgregarAvisoNucleos(minijuego, posicion, ValorNucleo(especial), especial);
}


static void ActualizarNucleosCaidos(
    MinijuegoNucleosEnergia& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas,
    AudioJuego* audio
)
{
    for (int n = 0; n < MAX_NUCLEOS_CAIDOS; n++)
    {
        NucleoEnergiaCaido& caido = minijuego.nucleosCaidos[n];
        if (!caido.activo) continue;

        caido.tiempoVida -= deltaTime;
        caido.tiempoBloqueo -= deltaTime;
        if (caido.tiempoBloqueo < 0.0f) caido.tiempoBloqueo = 0.0f;

        if (caido.tiempoVida <= 0.0f)
        {
            caido.activo = false;
            continue;
        }

        caido.velocidad.y -= GRAVEDAD_NUCLEO_CAIDO * deltaTime;
        caido.posicion.x += caido.velocidad.x * deltaTime;
        caido.posicion.y += caido.velocidad.y * deltaTime;
        caido.posicion.z += caido.velocidad.z * deltaTime;

        if (caido.posicion.y <= ALTURA_SUELO_NUCLEO_CAIDO)
        {
            caido.posicion.y = ALTURA_SUELO_NUCLEO_CAIDO;

            if (caido.velocidad.y < -1.4f)
                caido.velocidad.y *= -0.28f;
            else
                caido.velocidad.y = 0.0f;

            caido.velocidad.x *= 0.93f;
            caido.velocidad.z *= 0.93f;
        }

        if (caido.posicion.x < -6.55f)
        {
            caido.posicion.x = -6.55f;
            caido.velocidad.x = std::fabs(caido.velocidad.x) * 0.55f;
        }
        else if (caido.posicion.x > 6.55f)
        {
            caido.posicion.x = 6.55f;
            caido.velocidad.x = -std::fabs(caido.velocidad.x) * 0.55f;
        }

        if (caido.posicion.z < -6.55f)
        {
            caido.posicion.z = -6.55f;
            caido.velocidad.z = std::fabs(caido.velocidad.z) * 0.55f;
        }
        else if (caido.posicion.z > 6.55f)
        {
            caido.posicion.z = 6.55f;
            caido.velocidad.z = -std::fabs(caido.velocidad.z) * 0.55f;
        }

        for (int i = 0; i < limite; i++)
        {
            if (!JugadorPuedeRecogerNucleo(participantes[i], jugadores[i])) continue;

            if (i == caido.jugadorBloqueado)
            {
                // El jugador golpeado no puede recuperar el mismo nucleo
                // mientras esta en el aire, aunque el timer ya haya vencido.
                bool sigueEnAire =
                    caido.posicion.y > ALTURA_SUELO_NUCLEO_CAIDO + 0.10f;

                if (caido.tiempoBloqueo > 0.0f || sigueEnAire) continue;
            }

            if (!JugadorTocaPuntoNucleo(
                jugadores[i],
                caido.posicion,
                RADIO_RECOLECCION_CAIDO
            ))
            {
                continue;
            }

            RegistrarRecogidaNucleo(
                minijuego,
                i,
                caido.especial,
                caido.posicion,
                particulas,
                cantidadParticulas,
                audio
            );

            caido.activo = false;
            break;
        }
    }
}


// IA de bots: va por el recurso mas rentable (los especiales valen 3, asi
// que cuentan como "mas cerca") y golpea a un rival cercano que lleve
// varios puntos. Un pequeno desfase por jugador evita que todos los bots
// vayan al mismo nucleo y que golpeen siempre en cuanto pueden.
static InputMinijuegoParticipante CrearEntradaBotNucleos(
    const MinijuegoNucleosEnergia& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    int indice
)
{
    const JugadorPrueba& yo = jugadores[indice];

    if (yo.cayendo)
    {
        return InputMinijuegoParticipante{};
    }

    float t = minijuego.tiempoAnimacion;

    // 1) Rival cercano con botin: golpearlo.
    int victima = -1;
    float mejorDistancia = 9.0f;

    for (int j = 0; j < limite; j++)
    {
        if (
            j == indice ||
            !participantes[j].activo ||
            !minijuego.resultado.participantes[j].participo ||
            jugadores[j].cayendo ||
            minijuego.puntuaciones[j] < 3
        )
        {
            continue;
        }

        float dx = jugadores[j].posicion.x - yo.posicion.x;
        float dz = jugadores[j].posicion.z - yo.posicion.z;
        float distancia = dx * dx + dz * dz;

        if (distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
            victima = j;
        }
    }

    bool decidioAtacar = std::sin(t * 0.9f + indice * 2.1f) > -0.2f;

    if (victima >= 0 && decidioAtacar)
    {
        InputMinijuegoParticipante entrada = CrearEntradaBotHaciaObjetivo1v3(
            yo.posicion,
            jugadores[victima].posicion,
            0.3f
        );

        if (
            mejorDistancia < 2.2f &&
            !yo.golpeando &&
            yo.cooldownGolpe <= 0.0f
        )
        {
            entrada.golpear = true;
        }

        return entrada;
    }

    // 2) Nucleo mas rentable (en pie o caido).
    bool hayObjetivo = false;
    Vector3 objetivo{};
    float mejorCosto = 100000.0f;

    for (int n = 0; n < MAX_NUCLEOS_ENERGIA; n++)
    {
        if (!minijuego.nucleos[n].activo) continue;

        float dx = minijuego.nucleos[n].posicion.x - yo.posicion.x;
        float dz = minijuego.nucleos[n].posicion.z - yo.posicion.z;
        float costo = std::sqrt(dx * dx + dz * dz) /
            (minijuego.nucleos[n].especial ? 2.2f : 1.0f);
        costo += std::fmod((float)(n * 7 + indice * 3), 5.0f) * 0.15f;

        if (costo < mejorCosto)
        {
            mejorCosto = costo;
            objetivo = minijuego.nucleos[n].posicion;
            hayObjetivo = true;
        }
    }

    for (int n = 0; n < MAX_NUCLEOS_CAIDOS; n++)
    {
        const NucleoEnergiaCaido& caido = minijuego.nucleosCaidos[n];

        if (!caido.activo) continue;
        if (caido.jugadorBloqueado == indice && caido.tiempoBloqueo > 0.0f) continue;

        float dx = caido.posicion.x - yo.posicion.x;
        float dz = caido.posicion.z - yo.posicion.z;
        float costo = std::sqrt(dx * dx + dz * dz) /
            (caido.especial ? 2.2f : 1.0f);

        if (costo < mejorCosto)
        {
            mejorCosto = costo;
            objetivo = caido.posicion;
            hayObjetivo = true;
        }
    }

    if (!hayObjetivo)
    {
        objetivo = { 0.0f, yo.posicion.y, 0.0f };
    }

    return CrearEntradaBotHaciaObjetivo1v3(yo.posicion, objetivo, 0.25f);
}


static bool AlguienAlcanzoObjetivoNucleos(
    const MinijuegoNucleosEnergia& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.puntuaciones[i] >= PUNTOS_OBJETIVO_NUCLEOS
        )
        {
            return true;
        }
    }

    return false;
}


// Mejor puntaje; si empatan, el que tenia mas antes del frame final y, luego,
// el que recibio menos golpes.
static bool MejorResultadoNucleos(
    const MinijuegoNucleosEnergia& minijuego,
    int a,
    int b
)
{
    if (minijuego.puntuaciones[a] != minijuego.puntuaciones[b])
        return minijuego.puntuaciones[a] > minijuego.puntuaciones[b];
    if (minijuego.puntuacionesPrevias[a] != minijuego.puntuacionesPrevias[b])
        return minijuego.puntuacionesPrevias[a] > minijuego.puntuacionesPrevias[b];
    return minijuego.golpesRecibidos[a] < minijuego.golpesRecibidos[b];
}


static void FinalizarNucleos(
    MinijuegoNucleosEnergia& minijuego,
    AudioJuego* audio
)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO) return;

    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo) continue;

        bool superado = false;

        for (int j = 0; j < MAX_PARTICIPANTES && !superado; j++)
        {
            superado =
                j != i &&
                minijuego.resultado.participantes[j].participo &&
                MejorResultadoNucleos(minijuego, j, i);
        }

        if (!superado) cantidadGanadores++;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo) continue;

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                MejorResultadoNucleos(minijuego, j, i)
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = minijuego.puntuaciones[i];
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_NUCLEOS_TERMINADO;
    if (audio != nullptr) audio->ReproducirSonido(SONIDO_RESULTADO);
}


void MinijuegoNucleosEnergia::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;
    fase = FASE_NUCLEOS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_NUCLEOS;
    tiempoRestante = DURACION_PARTIDA_NUCLEOS;
    tiempoAnimacion = 0.0f;
    ultimoNumeroCuenta = -1;
    ultimoNumeroAlerta = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        puntuaciones[i] = 0;
        puntuacionesPrevias[i] = 0;
        golpesRecibidos[i] = 0;
        historial[i] = {};
    }

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++) nucleosCaidos[i] = {};
    for (int i = 0; i < MAX_PARTICULAS_NUCLEOS; i++) particulasLocales[i] = {};
    for (int i = 0; i < MAX_DESTELLOS_NUCLEOS; i++) destellos[i] = {};
    for (int i = 0; i < MAX_AVISOS_NUCLEOS; i++) avisos[i] = {};
    temporizadorAmbiente = 0.0f;

    ConfigurarArenaNucleos(*this);

    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        nucleos[i] = {};
        ReubicarNucleo(*this, i);

        // Entrada en ola durante la cuenta (solo visual).
        nucleos[i].tiempoAparicion = 0.35f + 0.16f * (float)i;
    }

    camara.position = { 0.0f, 12.0f, 13.8f };
    camara.target = { 0.0f, 0.45f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoNucleosEnergia::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -4.8f, 1.0f, 4.8f }, { 4.8f, 1.0f, 4.8f },
        { -4.8f, 1.0f, -4.8f }, { 4.8f, 1.0f, -4.8f }
    };

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawns[i]);
}


void MinijuegoNucleosEnergia::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    Inicializar();
    ConfigurarJugadores(jugadores, cantidadMaxima);
}


void MinijuegoNucleosEnergia::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas,
    AudioJuego* audio
)
{
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoMinijuego(
            resultado,
            participantes,
            FORMATO_MINIJUEGO_INDIVIDUAL
        );
    }

    // La animacion y los efectos siguen durante el fin (tablon final).
    // tiempoAnimacion lo lee la IA, pero solo mientras se juega.
    tiempoAnimacion += deltaTime;
    ActualizarVisualesNucleos(*this, deltaTime);

    if (fase == FASE_NUCLEOS_TERMINADO) return;

    if (fase == FASE_NUCLEOS_PREPARACION)
    {
        int numeroCuenta = (int)std::ceil(tiempoPreparacion);

        if (numeroCuenta > 0 && numeroCuenta != ultimoNumeroCuenta)
        {
            ultimoNumeroCuenta = numeroCuenta;
            if (audio != nullptr) audio->ReproducirSonido(SONIDO_CUENTA_REGRESIVA);
        }

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_NUCLEOS_JUGANDO;
            if (audio != nullptr) audio->ReproducirSonido(SONIDO_INICIO_MINIJUEGO);
        }

        return;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
        puntuacionesPrevias[i] = puntuaciones[i];

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo) continue;

        InputMinijuegoParticipante entrada{};
        if (participantes[i].esBot)
            entrada = CrearEntradaBotNucleos(
                *this, jugadores, participantes, limite, i
            );
        else if (participantes[i].conectado)
            entrada = LeerInputMinijuegoParticipante(participantes[i]);

        ActualizarJugadorPruebaNormal(
            jugadores[i],
            entrada,
            bloques,
            cantidadBloques,
            particulas,
            cantidadParticulas,
            true,
            true,
            deltaTime
        );
    }

    float ralentizacionAntes[MAX_PARTICIPANTES]{};
    for (int i = 0; i < limite; i++)
        ralentizacionAntes[i] = jugadores[i].tiempoRalentizado;

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        participantes,
        limite,
        particulas,
        cantidadParticulas
    );

    for (int i = 0; i < limite; i++)
    {
        if (jugadores[i].golpeSueloRecibido)
        {
            golpesRecibidos[i]++;
            PerderUltimosNucleosPorGolpe(
                *this, i, jugadores[i].posicion, 5, particulas, cantidadParticulas
            );
            continue;
        }

        if (jugadores[i].tiempoRalentizado > ralentizacionAntes[i] + 0.20f)
        {
            golpesRecibidos[i]++;
            PerderUltimosNucleosPorGolpe(
                *this, i, jugadores[i].posicion, 3, particulas, cantidadParticulas
            );
        }
    }

    for (int n = 0; n < MAX_NUCLEOS_ENERGIA; n++)
    {
        NucleoEnergia& nucleo = nucleos[n];

        if (!nucleo.activo)
        {
            nucleo.tiempoReaparicion -= deltaTime;
            if (nucleo.tiempoReaparicion <= 0.0f) ReubicarNucleo(*this, n, jugadores, participantes, limite);
            continue;
        }

        for (int i = 0; i < limite; i++)
        {
            if (!JugadorPuedeRecogerNucleo(participantes[i], jugadores[i])) continue;

            if (!JugadorTocaPuntoNucleo(
                jugadores[i], nucleo.posicion, RADIO_RECOLECCION_NUCLEO
            ))
            {
                continue;
            }

            RegistrarRecogidaNucleo(
                *this,
                i,
                nucleo.especial,
                nucleo.posicion,
                particulas,
                cantidadParticulas,
                audio
            );

            nucleo.activo = false;
            nucleo.tiempoReaparicion = RETRASO_REAPARICION_NUCLEO;
            break;
        }
    }

    ActualizarNucleosCaidos(
        *this,
        deltaTime,
        jugadores,
        limite,
        participantes,
        particulas,
        cantidadParticulas,
        audio
    );

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;

    // Alerta de los ultimos 5 segundos (una vez por segundo).
    if (
        tiempoRestante > 0.0f &&
        tiempoRestante <= 5.0f &&
        (int)std::ceil(restanteAntes) != (int)std::ceil(tiempoRestante) &&
        audio != nullptr
    )
    {
        audio->ReproducirSonido(SONIDO_ALERTA_TIEMPO);
    }

    if (AlguienAlcanzoObjetivoNucleos(*this) || tiempoRestante <= 0.0f)
    {
        if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
        FinalizarNucleos(*this, audio);
    }
}


//==================================================
// VISUAL: PRIMITIVAS
//==================================================

static float Hash01Nucleos(int a, int b)
{
    unsigned int h = (unsigned int)a * 73856093u ^ (unsigned int)b * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static Color TinteNucleos(Color c, float f)
{
    float r = c.r * f;
    float g = c.g * f;
    float b = c.b * f;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, c.a };
}


static float LimitarNucleos(float v, float minimo, float maximo)
{
    if (v < minimo) return minimo;
    if (v > maximo) return maximo;
    return v;
}


static float EaseOutBackNucleos(float u)
{
    u = LimitarNucleos(u, 0.0f, 1.0f);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float k = u - 1.0f;
    return 1.0f + c3 * k * k * k + c1 * k * k;
}


// Caja con cada cara a un brillo distinto: volumen sin luces reales.
static void CajaFacetadaNucleos(Vector3 centro, Vector3 tamano, Color color, float brilloTapa)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TinteNucleos(color, brilloTapa);
    Color frente = TinteNucleos(color, 1.0f);
    Color atras = TinteNucleos(color, 0.58f);
    Color derecha = TinteNucleos(color, 0.80f);
    Color izquierda = TinteNucleos(color, 0.66f);

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


static void CajaMetalNucleos(Vector3 centro, Vector3 tamano, Color color)
{
    CajaFacetadaNucleos(centro, tamano, color, 1.28f);
    DrawCubeWiresV(centro, tamano, Fade(BLACK, 0.30f));
}


static void RectSueloNucleos(float x0, float z0, float x1, float z1, float y, Color color)
{
    if (x1 < x0) { float t = x0; x0 = x1; x1 = t; }
    if (z1 < z0) { float t = z0; z0 = z1; z1 = t; }

    DrawTriangle3D({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, color);
    DrawTriangle3D({ x0, y, z0 }, { x1, y, z1 }, { x1, y, z0 }, color);
}


// Disco de luz (centro opaco, borde transparente). Solo con mezcla
// aditiva y sin escribir profundidad (ver DibujarResplandoresNucleos).
static void ResplandorNucleos(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 16;
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


static void ResplandorSueloNucleos(float x, float y, float z, float radio, Color color)
{
    ResplandorNucleos({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Disco mirando a la camara fija (eje derecho X y "arriba" perpendicular
// a la direccion de vista).
static void HaloCamaraNucleos(Vector3 centro, float radio, Color color)
{
    ResplandorNucleos(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.767f, -0.642f }, radio, color);
}


static Color ConAlfaNucleos(Color c, float alfa)
{
    c.a = (unsigned char)(LimitarNucleos(alfa, 0.0f, 1.0f) * 255.0f);
    return c;
}


//==================================================
// VISUAL: SALA DE ENERGIA
//==================================================

static const float X_GENERADORES_NUCLEOS[3] = { -4.6f, 0.0f, 4.6f };
static const float Z_GENERADORES_NUCLEOS = -9.6f;
static const float ALTO_GENERADOR_NUCLEOS = 3.7f;


// Generador del fondo: base de acero, cuerpo cilindrico con bandas violetas
// que se encienden de abajo hacia arriba y bobina en la cima. Sin esferas
// ni cian/oro para que nunca se lea como un nucleo.
static void DibujarGeneradorNucleos(float x, float z, float t, int indice)
{
    const Color acero = { 58, 66, 86, 255 };
    const Color aceroClaro = { 96, 108, 132, 255 };

    CajaMetalNucleos({ x, -0.35f, z }, { 2.1f, 1.3f, 2.1f }, Color{ 46, 54, 70, 255 });
    CajaMetalNucleos({ x, 0.42f, z }, { 1.75f, 0.26f, 1.75f }, Color{ 64, 74, 94, 255 });

    DrawCylinderEx({ x, 0.55f, z }, { x, ALTO_GENERADOR_NUCLEOS, z }, 0.78f, 0.70f, 12, acero);

    for (int k = 0; k < 4; k++)
    {
        float y = 1.0f + 0.68f * (float)k;
        float onda = std::sin(t * 3.6f - (float)k * 1.1f + (float)indice * 0.9f);
        float brillo = 0.30f + 0.70f * (onda > 0.0f ? onda : 0.0f);
        Color banda = ColorLerp(Color{ 70, 46, 120, 255 }, Color{ 214, 170, 255, 255 }, brillo);
        float radio = 0.79f - 0.02f * (float)k;
        DrawCylinderEx({ x, y, z }, { x, y + 0.13f, z }, radio, radio, 12, banda);
    }

    DrawCylinderEx({ x, ALTO_GENERADOR_NUCLEOS, z }, { x, ALTO_GENERADOR_NUCLEOS + 0.28f, z }, 0.86f, 0.52f, 12, aceroClaro);
    DrawCylinderEx({ x, ALTO_GENERADOR_NUCLEOS + 0.28f, z }, { x, ALTO_GENERADOR_NUCLEOS + 0.95f, z }, 0.10f, 0.10f, 6, aceroClaro);

    for (int k = 0; k < 3; k++)
    {
        float y = ALTO_GENERADOR_NUCLEOS + 0.40f + 0.18f * (float)k;
        float radio = 0.52f - 0.11f * (float)k;
        DrawCylinderEx({ x, y, z }, { x, y + 0.07f, z }, radio, radio, 10, Color{ 128, 138, 160, 255 });
    }
}


// Arco electrico entre dos generadores: aparece a intervalos, zigzag
// determinista por instante (Dibujar es const y no usa azar).
static void DibujarArcoNucleos(Vector3 a, Vector3 b, float t, int semilla)
{
    int cuadro = (int)(t * 24.0f);
    const int TRAMOS = 9;
    Vector3 anterior = a;

    for (int k = 1; k <= TRAMOS; k++)
    {
        float u = (float)k / (float)TRAMOS;
        Vector3 p =
        {
            a.x + (b.x - a.x) * u,
            a.y + (b.y - a.y) * u,
            a.z + (b.z - a.z) * u
        };

        if (k < TRAMOS)
        {
            p.y += (Hash01Nucleos(cuadro + semilla * 31, k) - 0.5f) * 0.55f;
            p.z += (Hash01Nucleos(cuadro + semilla * 17, k + 40) - 0.5f) * 0.30f;
        }

        DrawLine3D(anterior, p, Color{ 236, 214, 255, 255 });
        DrawLine3D({ anterior.x, anterior.y + 0.02f, anterior.z }, { p.x, p.y + 0.02f, p.z }, Color{ 170, 110, 255, 255 });
        anterior = p;
    }
}


static bool ArcoActivoNucleos(float t, int k)
{
    return std::fmod(t * 0.85f + (float)k * 1.37f, 2.6f) < 0.22f;
}


// Tubo de vidrio con pulsos de energia que corren a lo largo.
static void DibujarTuboEnergiaNucleos(Vector3 a, Vector3 b, float radio, float t, float fase)
{
    DrawCylinderEx(a, b, radio, radio, 8, Color{ 44, 34, 72, 255 });
    DrawCylinderWiresEx(a, b, radio * 1.08f, radio * 1.08f, 8, Fade(Color{ 150, 120, 210, 255 }, 0.35f));

    Vector3 d = { b.x - a.x, b.y - a.y, b.z - a.z };

    for (int k = 0; k < 3; k++)
    {
        float u = std::fmod(t * 0.22f + fase + (float)k / 3.0f, 1.0f);
        float largo = 0.035f;
        float u1 = u + largo > 1.0f ? 1.0f : u + largo;
        Vector3 p0 = { a.x + d.x * u, a.y + d.y * u, a.z + d.z * u };
        Vector3 p1 = { a.x + d.x * u1, a.y + d.y * u1, a.z + d.z * u1 };
        DrawCylinderEx(p0, p1, radio * 1.02f, radio * 1.02f, 8, Color{ 196, 150, 255, 255 });
    }
}


// Fondo: piso de servicio bajo la arena, pared de la sala, generadores,
// tubos y gabinetes laterales. Todo fuera de la zona de juego.
static void DibujarSalaFondoNucleos(float t)
{
    // Piso de servicio con rejilla: la arena no queda flotando en el vacio.
    RectSueloNucleos(-26.0f, -16.0f, 26.0f, 14.0f, -0.95f, Color{ 22, 25, 35, 255 });

    NivelCalidadGrafica calidad = CalidadDecoracion();
    int paso = calidad == CALIDAD_ALTA ? 1 : 2;

    for (int k = -24; k <= 24; k += paso)
    {
        DrawLine3D({ (float)k, -0.94f, -16.0f }, { (float)k, -0.94f, 14.0f }, Color{ 30, 34, 46, 255 });
    }
    for (int k = -16; k <= 14; k += paso)
    {
        DrawLine3D({ -26.0f, -0.94f, (float)k }, { 26.0f, -0.94f, (float)k }, Color{ 30, 34, 46, 255 });
    }

    // Pared de la sala de maquinas (lejos, oscura) con nervios verticales.
    const float zPared = -12.6f;
    DrawTriangle3D({ -26.0f, -1.0f, zPared }, { 26.0f, -1.0f, zPared }, { 26.0f, 12.0f, zPared }, Color{ 26, 30, 44, 255 });
    DrawTriangle3D({ -26.0f, -1.0f, zPared }, { 26.0f, 12.0f, zPared }, { -26.0f, 12.0f, zPared }, Color{ 26, 30, 44, 255 });

    if (calidad == CALIDAD_BAJA)
    {
        return;
    }

    for (int k = -8; k <= 8; k++)
    {
        float x = (float)k * 3.0f;
        CajaFacetadaNucleos({ x, 5.0f, zPared + 0.2f }, { 0.5f, 12.0f, 0.4f }, Color{ 34, 40, 56, 255 }, 1.2f);
    }
    CajaFacetadaNucleos({ 0.0f, 5.6f, zPared + 0.35f }, { 52.0f, 0.35f, 0.3f }, Color{ 40, 46, 62, 255 }, 1.3f);

    // Generadores y arcos entre sus bobinas.
    for (int g = 0; g < 3; g++)
    {
        DibujarGeneradorNucleos(X_GENERADORES_NUCLEOS[g], Z_GENERADORES_NUCLEOS, t, g);
    }

    for (int k = 0; k < 2; k++)
    {
        if (!ArcoActivoNucleos(t, k)) continue;

        float y = ALTO_GENERADOR_NUCLEOS + 0.55f;
        DibujarArcoNucleos(
            { X_GENERADORES_NUCLEOS[k] + 0.5f, y, Z_GENERADORES_NUCLEOS },
            { X_GENERADORES_NUCLEOS[k + 1] - 0.5f, y, Z_GENERADORES_NUCLEOS },
            t,
            k
        );
    }

    // Gabinetes de los generadores laterales lejanos.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 9.4f;
        CajaMetalNucleos({ x, 0.6f, Z_GENERADORES_NUCLEOS }, { 1.8f, 3.0f, 1.4f }, Color{ 50, 58, 76, 255 });

        for (int k = 0; k < 4; k++)
        {
            bool encendida = std::sin(t * 2.3f + (float)k * 1.7f + (float)lado) > -0.2f;
            Color led = (k % 2 == 0) ? Color{ 90, 220, 120, 255 } : Color{ 240, 80, 70, 255 };
            if (!encendida) led = TinteNucleos(led, 0.35f);
            DrawCubeV({ x - 0.5f + 0.33f * (float)k, 1.7f, Z_GENERADORES_NUCLEOS + 0.72f }, { 0.12f, 0.12f, 0.04f }, led);
        }
    }

    // Tubos de energia detras del muro del fondo y sobre los muros laterales.
    DibujarTuboEnergiaNucleos({ -12.0f, 1.95f, -7.75f }, { 12.0f, 1.95f, -7.75f }, 0.17f, t, 0.0f);
    DibujarTuboEnergiaNucleos({ 12.0f, 2.40f, -8.05f }, { -12.0f, 2.40f, -8.05f }, 0.11f, t, 0.5f);
    DibujarTuboEnergiaNucleos({ -7.75f, 1.85f, 7.4f }, { -7.75f, 1.85f, -7.6f }, 0.14f, t, 0.25f);
    DibujarTuboEnergiaNucleos({ 7.75f, 1.85f, 7.4f }, { 7.75f, 1.85f, -7.6f }, 0.14f, t, 0.75f);

    // Abrazaderas de los tubos.
    for (int k = -5; k <= 5; k++)
    {
        float x = (float)k * 2.2f;
        CajaMetalNucleos({ x, 1.95f, -7.75f }, { 0.12f, 0.44f, 0.44f }, Color{ 88, 98, 120, 255 });
    }
    for (int k = -3; k <= 3; k++)
    {
        float z = (float)k * 2.2f;
        CajaMetalNucleos({ -7.75f, 1.85f, z }, { 0.38f, 0.38f, 0.12f }, Color{ 88, 98, 120, 255 });
        CajaMetalNucleos({ 7.75f, 1.85f, z }, { 0.38f, 0.38f, 0.12f }, Color{ 88, 98, 120, 255 });
    }

    // Gabinetes de transformadores a los costados de la arena.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 5; k += paso)
        {
            float z = -5.6f + 2.8f * (float)k;
            float x = (float)lado * 8.9f;
            CajaMetalNucleos({ x, -0.1f, z }, { 1.2f, 1.7f, 1.9f }, Color{ 48, 56, 74, 255 });
            CajaMetalNucleos({ x, 0.82f, z }, { 0.9f, 0.16f, 1.5f }, Color{ 70, 80, 100, 255 });

            // Rejilla de ventilacion y luz de estado (verde/roja, nunca cian u oro).
            for (int r = 0; r < 3; r++)
            {
                float y = -0.5f + 0.22f * (float)r;
                DrawLine3D({ x - (float)lado * 0.61f, y, z - 0.6f }, { x - (float)lado * 0.61f, y, z + 0.6f }, Color{ 24, 28, 38, 255 });
            }

            bool encendida = std::sin(t * 1.7f + (float)k * 2.1f + (float)lado * 0.6f) > -0.4f;
            Color led = (k + (lado > 0 ? 1 : 0)) % 3 == 0 ? Color{ 240, 80, 70, 255 } : Color{ 90, 220, 120, 255 };
            if (!encendida) led = TinteNucleos(led, 0.3f);
            DrawCubeV({ x - (float)lado * 0.62f, 0.45f, z + 0.6f }, { 0.04f, 0.12f, 0.12f }, led);
        }
    }
}


// Suelo de placas de acero con variacion, rejillas de ventilacion,
// canaletas de energia violeta y el emblema central.
static void DibujarSueloNucleos(float t)
{
    DrawCubeV({ 0.0f, -0.46f, 0.0f }, { 14.0f, 0.92f, 14.0f }, Color{ 18, 21, 30, 255 });

    for (int iz = 0; iz < 14; iz++)
    {
        for (int ix = 0; ix < 14; ix++)
        {
            float x = -7.0f + (float)ix + 0.5f;
            float z = -7.0f + (float)iz + 0.5f;
            float altura = 0.004f + Hash01Nucleos(ix, iz) * 0.018f;
            float nx = x / 7.0f;
            float nz = z / 7.0f;
            float borde = 1.0f - 0.28f * (nx * nx * 0.7f + nz * nz * 0.7f);
            float variacion = 0.84f + 0.26f * Hash01Nucleos(ix + 31, iz + 7);
            bool rejilla = Hash01Nucleos(ix + 5, iz + 91) > 0.86f;

            Color base = rejilla ? Color{ 44, 50, 64, 255 } : Color{ 60, 70, 90, 255 };
            Color color = TinteNucleos(base, variacion * borde);
            DrawCubeV({ x, altura - 0.07f, z }, { 0.94f, 0.14f, 0.94f }, color);

            if (rejilla)
            {
                Color ranura = TinteNucleos(color, 0.45f);
                for (int r = -2; r <= 2; r++)
                {
                    float rz = z + (float)r * 0.15f;
                    DrawLine3D({ x - 0.33f, altura + 0.002f, rz }, { x + 0.33f, altura + 0.002f, rz }, ranura);
                }
            }
        }
    }

    // Canaletas desde el centro hacia los cuatro muros.
    const Color canal = { 26, 22, 40, 255 };
    RectSueloNucleos(1.9f, -0.11f, 6.9f, 0.11f, 0.030f, canal);
    RectSueloNucleos(-6.9f, -0.11f, -1.9f, 0.11f, 0.030f, canal);
    RectSueloNucleos(-0.11f, 1.9f, 0.11f, 6.9f, 0.030f, canal);
    RectSueloNucleos(-0.11f, -6.9f, 0.11f, -1.9f, 0.030f, canal);

    // Pulsos violetas que viajan hacia el centro (decorativos, planos).
    for (int k = 0; k < 2; k++)
    {
        float u = std::fmod(t * 0.45f + (float)k * 0.5f, 1.0f);
        float d = 6.9f - u * 5.0f;
        Color pulso = ConAlfaNucleos(Color{ 170, 110, 255, 255 }, 0.55f * (1.0f - u * 0.6f));
        RectSueloNucleos(d - 0.35f, -0.06f, d, 0.06f, 0.034f, pulso);
        RectSueloNucleos(-d, -0.06f, -d + 0.35f, 0.06f, 0.034f, pulso);
        RectSueloNucleos(-0.06f, d - 0.35f, 0.06f, d, 0.034f, pulso);
        RectSueloNucleos(-0.06f, -d, 0.06f, -d + 0.35f, 0.034f, pulso);
    }

    // Emblema central: plataforma octogonal con anillos violetas.
    DrawCylinderEx({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.032f, 0.0f }, 1.85f, 1.78f, 8, Color{ 50, 56, 76, 255 });
    DrawCylinderEx({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.036f, 0.0f }, 1.48f, 1.48f, 8, Color{ 34, 36, 54, 255 });
    DrawCircle3D({ 0.0f, 0.040f, 0.0f }, 1.62f, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 120, 84, 200, 255 });
    DrawCircle3D({ 0.0f, 0.040f, 0.0f }, 1.66f, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 120, 84, 200, 255 });

    // Sombras de contacto al pie de los muros.
    const Color sombra = Fade(BLACK, 0.32f);
    RectSueloNucleos(-7.0f, -7.0f, 7.0f, -6.55f, 0.036f, sombra);
    RectSueloNucleos(-7.0f, -7.0f, -6.6f, 7.0f, 0.036f, sombra);
    RectSueloNucleos(6.6f, -7.0f, 7.0f, 7.0f, 0.036f, sombra);
}


// Muros con volumen: chapa facetada, nervios, baranda superior, tira de
// luz violeta y paneles con luces de estado. El muro frontal se mantiene
// como borde bajo para no tapar a los jugadores (la colision no cambia).
static void DibujarMurosNucleos(const MinijuegoNucleosEnergia& m, float t, bool mostrarDebug)
{
    const Color chapa = { 52, 70, 96, 255 };
    const Color nervio = { 70, 88, 116, 255 };
    const Color baranda = { 96, 114, 142, 255 };
    const Color tira = { 186, 132, 255, 255 };

    for (int i = 1; i < m.cantidadBloques; i++)
    {
        const BloquePrueba& b = m.bloques[i];
        if (mostrarDebug) DrawBoundingBox(CrearHitboxBloquePrueba(b), YELLOW);

        if (i == 4)
        {
            // Borde bajo frontal (misma altura que antes: tapa en y = 0,05).
            CajaMetalNucleos({ b.posicion.x, -0.20f, b.posicion.z }, { b.tamano.x, 0.50f, b.tamano.z }, chapa);
            CajaFacetadaNucleos({ b.posicion.x, 0.058f, b.posicion.z - 0.06f }, { b.tamano.x - 0.4f, 0.016f, 0.05f }, tira, 1.0f);
            continue;
        }

        CajaMetalNucleos(b.posicion, b.tamano, chapa);

        float alto = b.posicion.y + b.tamano.y * 0.5f;
        bool lateral = b.tamano.x < b.tamano.z;
        Vector3 tamBaranda = lateral
            ? Vector3{ b.tamano.x + 0.14f, 0.10f, b.tamano.z }
            : Vector3{ b.tamano.x, 0.10f, b.tamano.z + 0.14f };
        CajaMetalNucleos({ b.posicion.x, alto + 0.05f, b.posicion.z }, tamBaranda, baranda);

        // Cara interior del muro (hacia el centro de la arena).
        float interior = lateral
            ? b.posicion.x - (b.posicion.x > 0.0f ? 1.0f : -1.0f) * b.tamano.x * 0.5f
            : b.posicion.z + b.tamano.z * 0.5f;
        float haciaDentro = lateral ? (b.posicion.x > 0.0f ? -1.0f : 1.0f) : 1.0f;

        // Nervios verticales cada 2,4.
        for (int k = -2; k <= 3; k++)
        {
            float a = -6.0f + 2.4f * (float)(k + 2);
            Vector3 c = lateral
                ? Vector3{ interior + haciaDentro * 0.05f, 0.62f, a }
                : Vector3{ a, 0.62f, interior + 0.05f };
            Vector3 tam = lateral ? Vector3{ 0.10f, 1.62f, 0.24f } : Vector3{ 0.24f, 1.62f, 0.10f };
            CajaMetalNucleos(c, tam, nervio);
        }

        // Tira de luz violeta continua bajo la baranda.
        Vector3 cTira = lateral
            ? Vector3{ interior + haciaDentro * 0.02f, 1.30f, 0.0f }
            : Vector3{ 0.0f, 1.30f, interior + 0.02f };
        Vector3 tTira = lateral ? Vector3{ 0.03f, 0.07f, 13.4f } : Vector3{ 13.4f, 0.07f, 0.03f };
        CajaFacetadaNucleos(cTira, tTira, tira, 1.0f);

        // Paneles de control con dos luces de estado en el muro del fondo.
        if (!lateral && CalidadDecoracion() != CALIDAD_BAJA)
        {
            for (int k = 0; k < 5; k++)
            {
                float x = -4.8f + 2.4f * (float)k;
                CajaFacetadaNucleos({ x, 0.55f, interior + 0.015f }, { 1.7f, 0.85f, 0.03f }, Color{ 36, 46, 66, 255 }, 1.0f);
                DrawCubeWiresV({ x, 0.55f, interior + 0.015f }, { 1.7f, 0.85f, 0.03f }, Fade(BLACK, 0.4f));

                for (int l = 0; l < 2; l++)
                {
                    bool encendida = std::sin(t * 2.0f + (float)(k * 2 + l) * 1.3f) > 0.0f;
                    Color led = l == 0 ? Color{ 90, 220, 120, 255 } : Color{ 240, 80, 70, 255 };
                    if (!encendida) led = TinteNucleos(led, 0.3f);
                    DrawCubeV({ x - 0.55f + 0.22f * (float)l, 0.82f, interior + 0.04f }, { 0.10f, 0.10f, 0.04f }, led);
                }

                // Medidor de barras (acero, no luminoso).
                for (int l = 0; l < 4; l++)
                {
                    float h = 0.12f + 0.30f * (0.5f + 0.5f * std::sin(t * 1.4f + (float)(k * 4 + l)));
                    DrawCubeV({ x + 0.05f + 0.16f * (float)l, 0.28f + h * 0.5f, interior + 0.04f }, { 0.10f, h, 0.03f }, Color{ 112, 124, 150, 255 });
                }
            }
        }
    }

    // Columnas de esquina del fondo con lampara violeta; las del frente
    // son bajas para no tapar.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 7.15f;
        CajaMetalNucleos({ x, 0.70f, -7.15f }, { 0.62f, 2.5f, 0.62f }, Color{ 64, 80, 106, 255 });
        CajaFacetadaNucleos({ x, 2.02f, -7.15f }, { 0.36f, 0.16f, 0.36f }, tira, 1.0f);
        CajaMetalNucleos({ x, -0.12f, 7.15f }, { 0.56f, 0.40f, 0.56f }, Color{ 64, 80, 106, 255 });
    }
}


//==================================================
// VISUAL: NUCLEOS
//==================================================

// Emisor del suelo donde nace un nucleo (solo mientras el nucleo existe).
static void DibujarEmisorNucleos(Vector3 posicion, bool especial, float aparicion)
{
    Color anillo = ColorNucleo(especial);
    float r = especial ? 0.62f : 0.52f;

    DrawCylinderEx({ posicion.x, 0.0f, posicion.z }, { posicion.x, 0.05f, posicion.z }, r, r * 0.9f, 10, Color{ 40, 48, 64, 255 });
    DrawCircle3D({ posicion.x, 0.056f, posicion.z }, r * 0.82f, { 1.0f, 0.0f, 0.0f }, 90.0f, ConAlfaNucleos(anillo, 0.9f * aparicion));
    DrawCircle3D({ posicion.x, 0.056f, posicion.z }, r * 0.74f, { 1.0f, 0.0f, 0.0f }, 90.0f, ConAlfaNucleos(anillo, 0.6f * aparicion));
}


// Nucleo: esfera brillante con reflejo, cascara facetada y anillo(s) en
// orbita. El especial es mas grande, dorado, con dos anillos cruzados y
// destellos en orbita: siempre se distingue del normal.
static void DibujarNucleoVisual(
    Vector3 posicion,
    bool especial,
    float tiempoAnimacion,
    float fase,
    float escala,
    float aparicion,
    float alfa
)
{
    float s = EaseOutBackNucleos(aparicion);
    if (s <= 0.01f) return;

    float baseY = posicion.y;
    float oscilacion = std::sin(tiempoAnimacion * 3.1f + fase) * 0.16f;
    posicion.y += oscilacion - (1.0f - LimitarNucleos(aparicion, 0.0f, 1.0f)) * 0.6f;
    float pulso = 1.0f + std::sin(tiempoAnimacion * 5.0f + fase) * 0.07f;
    float radio = (especial ? 0.48f : 0.34f) * pulso * escala * s;

    Color luz = ColorNucleo(especial);
    Color claro = ColorLerp(luz, WHITE, 0.55f);
    Color cuerpo = ConAlfaNucleos(luz, alfa);

    // Haz fino hasta el suelo: lo ubica sobre el piso desde lejos.
    DrawCylinderEx(
        { posicion.x, 0.0f, posicion.z },
        { posicion.x, baseY - radio, posicion.z },
        0.03f * escala,
        0.03f * escala,
        6,
        ConAlfaNucleos(luz, 0.45f * alfa)
    );

    DrawSphere(posicion, radio, cuerpo);
    (DrawSphere)({ posicion.x - radio * 0.30f, posicion.y + radio * 0.38f, posicion.z + radio * 0.42f }, radio * 0.34f, ConAlfaNucleos(WHITE, 0.85f * alfa));
    DrawSphereWires(posicion, radio * 1.16f, 6, 10, ConAlfaNucleos(claro, 0.40f * alfa));

    // Anillos en orbita.
    int anillos = especial ? 2 : 1;
    for (int k = 0; k < anillos; k++)
    {
        rlPushMatrix();
        rlTranslatef(posicion.x, posicion.y, posicion.z);
        rlRotatef((k == 0 ? 1.0f : -1.3f) * tiempoAnimacion * 110.0f + fase * 57.0f, 0.0f, 1.0f, 0.0f);
        rlRotatef(k == 0 ? 68.0f : 112.0f, 1.0f, 0.0f, 0.0f);
        float r = radio * (especial ? 1.55f : 1.6f);
        DrawCircle3D({ 0.0f, 0.0f, 0.0f }, r, { 1.0f, 0.0f, 0.0f }, 0.0f, ConAlfaNucleos(claro, alfa));
        DrawCircle3D({ 0.0f, 0.0f, 0.0f }, r * 1.05f, { 1.0f, 0.0f, 0.0f }, 0.0f, ConAlfaNucleos(luz, 0.8f * alfa));
        rlPopMatrix();
    }

    if (especial)
    {
        // Destellos dorados en orbita (cubitos girados, no esferas).
        for (int k = 0; k < 4; k++)
        {
            float a = tiempoAnimacion * 2.4f + fase + (float)k * 1.5708f;
            float rr = radio * 1.9f;
            Vector3 p =
            {
                posicion.x + std::cos(a) * rr,
                posicion.y + std::sin(a * 2.0f) * radio * 0.35f,
                posicion.z + std::sin(a) * rr
            };
            float lado = 0.09f * s * (0.8f + 0.2f * std::sin(tiempoAnimacion * 9.0f + (float)k));
            rlPushMatrix();
            rlTranslatef(p.x, p.y, p.z);
            rlRotatef(45.0f, 0.0f, 0.0f, 1.0f);
            rlRotatef(tiempoAnimacion * 160.0f, 0.0f, 1.0f, 0.0f);
            DrawCubeV({ 0.0f, 0.0f, 0.0f }, { lado, lado, lado }, ConAlfaNucleos(Color{ 255, 240, 170, 255 }, alfa));
            rlPopMatrix();
        }
    }
}


static float AparicionNucleo(const NucleoEnergia& nucleo, float t)
{
    return LimitarNucleos((t - nucleo.tiempoAparicion) / 0.45f, 0.0f, 1.0f);
}


// Los caidos parpadean en su ultimo segundo y medio antes de apagarse.
static float AlfaNucleoCaido(const NucleoEnergiaCaido& caido, float t)
{
    if (caido.tiempoVida > 1.6f) return 1.0f;
    return std::sin(t * 22.0f) > 0.0f ? 1.0f : 0.35f;
}


// Anillo de color bajo los pies y flecha sobre la cabeza: el modelo del
// jugador es oscuro y sobre el suelo oscuro casi no se distingue.
// MODELO FUTURO: la flecha puede pasar a ser un icono de jugador del GLB.
static void DibujarIndicadorJugadorNucleos(
    const JugadorPrueba& jugador,
    Color color
)
{
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (jugador.cayendo || pies < -0.35f) return;

    Vector3 centro = { jugador.posicion.x, 0.06f, jugador.posicion.z };
    for (int k = 0; k < 3; k++)
    {
        DrawCircle3D(
            centro,
            0.52f + k * 0.045f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            color
        );
    }

    float rebote = std::sin((float)GetTime() * 5.0f + jugador.posicion.x) * 0.06f;
    float cabeza = pies + 2.15f + rebote;
    DrawCylinderEx(
        { jugador.posicion.x, cabeza, jugador.posicion.z },
        { jugador.posicion.x, cabeza + 0.32f, jugador.posicion.z },
        0.0f,
        0.20f,
        10,
        color
    );
}


static void DibujarDestellosNucleos(const MinijuegoNucleosEnergia& m)
{
    for (int i = 0; i < MAX_DESTELLOS_NUCLEOS; i++)
    {
        const DestelloNucleos& d = m.destellos[i];
        if (!d.activo || d.vidaMaxima <= 0.0f) continue;

        float k = 1.0f - d.vida / d.vidaMaxima;
        float r = d.radio * (0.25f + 0.75f * k);
        Color c = ConAlfaNucleos(d.color, 1.0f - k);
        DrawCircle3D({ d.posicion.x, 0.07f, d.posicion.z }, r, { 1.0f, 0.0f, 0.0f }, 90.0f, c);
        DrawCircle3D({ d.posicion.x, 0.07f, d.posicion.z }, r * 0.94f, { 1.0f, 0.0f, 0.0f }, 90.0f, c);
    }
}


static void DibujarParticulasNucleos(const MinijuegoNucleosEnergia& m)
{
    for (int i = 0; i < MAX_PARTICULAS_NUCLEOS; i++)
    {
        const ParticulaNucleos& p = m.particulasLocales[i];
        if (!p.activa || p.vidaMaxima <= 0.0f) continue;

        float vida = p.vida / p.vidaMaxima;
        float tamano = p.tamano * (0.4f + 0.6f * vida);
        Color color = p.color;
        color.a = (unsigned char)((float)p.color.a * (vida > 0.3f ? 1.0f : vida / 0.3f));
        DrawCubeV(p.posicion, { tamano, tamano, tamano }, color);
    }
}


// Iluminacion falsa: luz de los generadores, tiras de los muros, charcos
// de luz bajo cada nucleo, halos y ondas de recogida/golpe.
static void DibujarResplandoresNucleos(const MinijuegoNucleosEnergia& m, float t)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int g = 0; g < 3; g++)
        {
            float pulso = 0.82f + 0.18f * std::sin(t * 2.2f + (float)g * 1.9f);
            float x = X_GENERADORES_NUCLEOS[g];
            ResplandorNucleos({ x, 2.4f, -12.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 3.4f * pulso, Color{ 130, 70, 230, 70 });
            ResplandorSueloNucleos(x, -0.92f, Z_GENERADORES_NUCLEOS, 2.6f * pulso, Color{ 140, 80, 240, 60 });
            HaloCamaraNucleos({ x, ALTO_GENERADOR_NUCLEOS + 0.6f, Z_GENERADORES_NUCLEOS }, 1.1f * pulso, Color{ 170, 110, 255, 70 });
        }

        for (int k = 0; k < 2; k++)
        {
            if (!ArcoActivoNucleos(t, k)) continue;
            float x = (X_GENERADORES_NUCLEOS[k] + X_GENERADORES_NUCLEOS[k + 1]) * 0.5f;
            HaloCamaraNucleos({ x, ALTO_GENERADOR_NUCLEOS + 0.55f, Z_GENERADORES_NUCLEOS }, 2.4f, Color{ 200, 160, 255, 90 });
        }
    }

    // Tiras violetas de los muros: luz sobre la chapa y el borde del suelo.
    for (int k = 0; k < 5; k++)
    {
        float a = -5.2f + 2.6f * (float)k;
        ResplandorNucleos({ a, 1.30f, -6.97f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.1f, Color{ 150, 90, 255, 46 });
        ResplandorNucleos({ -6.97f, 1.30f, a }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 1.1f, Color{ 150, 90, 255, 40 });
        ResplandorNucleos({ 6.97f, 1.30f, a }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 1.1f, Color{ 150, 90, 255, 40 });
    }

    // Charco de luz y halo de cada nucleo (del color del nucleo).
    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        const NucleoEnergia& n = m.nucleos[i];
        if (!n.activo) continue;

        float aparicion = AparicionNucleo(n, t);
        if (aparicion <= 0.0f) continue;

        Color c = ColorNucleo(n.especial);
        float pulso = 0.85f + 0.15f * std::sin(t * 5.0f + n.faseFlotacion);
        float y = n.posicion.y + std::sin(t * 3.1f + n.faseFlotacion) * 0.16f;
        float rSuelo = (n.especial ? 1.9f : 1.3f) * pulso * aparicion;
        float rHalo = (n.especial ? 1.25f : 0.85f) * pulso * aparicion;

        ResplandorSueloNucleos(n.posicion.x, 0.06f, n.posicion.z, rSuelo, Color{ c.r, c.g, c.b, 95 });
        HaloCamaraNucleos({ n.posicion.x, y, n.posicion.z }, rHalo, Color{ c.r, c.g, c.b, (unsigned char)(n.especial ? 120 : 95) });

        // Columna de luz al aparecer.
        float edad = t - n.tiempoAparicion;
        if (edad >= 0.0f && edad < 0.6f)
        {
            float f = 1.0f - edad / 0.6f;
            HaloCamaraNucleos({ n.posicion.x, 1.4f, n.posicion.z }, 1.6f * f + 0.4f, Color{ c.r, c.g, c.b, (unsigned char)(150.0f * f) });
            DrawCylinderEx({ n.posicion.x, 0.0f, n.posicion.z }, { n.posicion.x, 3.2f, n.posicion.z }, 0.30f * f, 0.05f, 8, Color{ c.r, c.g, c.b, (unsigned char)(110.0f * f) });
        }
    }

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++)
    {
        const NucleoEnergiaCaido& caido = m.nucleosCaidos[i];
        if (!caido.activo) continue;

        Color c = ColorNucleo(caido.especial);
        float alfa = AlfaNucleoCaido(caido, t);
        ResplandorSueloNucleos(caido.posicion.x, 0.06f, caido.posicion.z, caido.especial ? 1.3f : 0.95f, Color{ c.r, c.g, c.b, (unsigned char)(75.0f * alfa) });
        HaloCamaraNucleos(caido.posicion, caido.especial ? 0.95f : 0.65f, Color{ c.r, c.g, c.b, (unsigned char)(85.0f * alfa) });
    }

    for (int i = 0; i < MAX_DESTELLOS_NUCLEOS; i++)
    {
        const DestelloNucleos& d = m.destellos[i];
        if (!d.activo || d.vidaMaxima <= 0.0f) continue;

        float k = d.vida / d.vidaMaxima;
        ResplandorSueloNucleos(d.posicion.x, 0.08f, d.posicion.z, d.radio * (1.2f - 0.4f * k), Color{ d.color.r, d.color.g, d.color.b, (unsigned char)(150.0f * k) });
        HaloCamaraNucleos({ d.posicion.x, 0.9f, d.posicion.z }, d.radio * 0.8f, Color{ d.color.r, d.color.g, d.color.b, (unsigned char)(110.0f * k * k) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


// Numeros "+N" / "-N" que suben y se desvanecen sobre el mundo.
static void DibujarAvisosNucleos(const MinijuegoNucleosEnergia& m)
{
    for (int i = 0; i < MAX_AVISOS_NUCLEOS; i++)
    {
        const AvisoNucleos& a = m.avisos[i];
        if (!a.activo) continue;

        float subida = (1.0f - a.vida) * 0.9f;
        Vector2 p = GetWorldToScreen({ a.posicion.x, a.posicion.y + 1.5f + subida, a.posicion.z }, m.camara);
        if (p.y < Px(140.0f)) continue;

        float alfa = a.vida < 0.35f ? a.vida / 0.35f : 1.0f;
        char texto[16];
        std::snprintf(texto, sizeof(texto), a.valor > 0 ? "+%d" : "%d", a.valor);

        Color color = a.valor < 0
            ? UI_PELIGRO
            : (a.especial ? UI_FOCO : Color{ 150, 226, 255, 255 });
        int token = a.especial || a.valor < -2 ? UI_TEXTO_GRANDE : UI_TEXTO;
        // Corrimiento por ranura: dos avisos simultaneos no se pisan.
        float corrimiento = (float)((i % 3) - 1) * Px(22.0f);
        DibujarTextoMundoContornoUI(texto, p.x + corrimiento, p.y - (float)(i % 2) * Px(14.0f), token, color, alfa);
    }
}


void MinijuegoNucleosEnergia::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    float t = tiempoAnimacion;

    ClearBackground(Color{ 14, 16, 26, 255 });
    BeginMode3D(camara);

    // Las cajas facetadas son triangulos sueltos: sin descarte de caras.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    DibujarSalaFondoNucleos(t);
    DibujarSueloNucleos(t);
    if (mostrarDebug && cantidadBloques > 0) DrawBoundingBox(CrearHitboxBloquePrueba(bloques[0]), YELLOW);
    DibujarMurosNucleos(*this, t, mostrarDebug);
    DibujarDestellosNucleos(*this);

    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        if (!nucleos[i].activo) continue;

        float aparicion = AparicionNucleo(nucleos[i], t);
        if (aparicion <= 0.0f) continue;

        DibujarEmisorNucleos(nucleos[i].posicion, nucleos[i].especial, aparicion);
        DibujarNucleoVisual(
            nucleos[i].posicion,
            nucleos[i].especial,
            t,
            nucleos[i].faseFlotacion,
            1.0f,
            aparicion,
            1.0f
        );
    }

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++)
    {
        if (!nucleosCaidos[i].activo) continue;

        DibujarNucleoVisual(
            nucleosCaidos[i].posicion,
            nucleosCaidos[i].especial,
            t,
            (float)i * 0.41f,
            0.82f,
            1.0f,
            AlfaNucleoCaido(nucleosCaidos[i], t)
        );
    }

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo) continue;
        DibujarIndicadorJugadorNucleos(jugadores[i], participantes[i].color);
        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);
        if (mostrarDebug && !jugadores[i].cayendo)
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
    }

    DibujarParticulasNucleos(*this);
    DibujarResplandoresNucleos(*this, t);

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    EndMode3D();

    DibujarAvisosNucleos(*this);

    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "NÚCLEOS DE ENERGIA",
        "PRIMERO A 15 O EL QUE MÁS JUNTE EN 60 S. GOLPE: EL RIVAL SUELTA 3. GOLPE AL SUELO: SUELTA 5.",
        resultado
    );

    hud.fase = fase == FASE_NUCLEOS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_NUCLEOS_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante > 0.0f ? tiempoRestante : 0.0f;
    hud.tiempoTotal = DURACION_PARTIDA_NUCLEOS;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = tiempoRestante <= 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        // La etiqueta muestra el avance hacia el objetivo y se planta en
        // «15/15» aunque el ultimo frame sume de mas (el tablon final da
        // los puntos reales).
        int avance = puntuaciones[i] < PUNTOS_OBJETIVO_NUCLEOS ? puntuaciones[i] : PUNTOS_OBJETIVO_NUCLEOS;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", avance, PUNTOS_OBJETIVO_NUCLEOS);
        jugador.valorNumerico = puntuaciones[i];
        jugador.progreso = (float)puntuaciones[i] / (float)PUNTOS_OBJETIVO_NUCLEOS;
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d PTS", puntuaciones[i]);

        // Empate en puntos (y en lo juntado antes del ultimo frame): el
        // tablon muestra el desempate de MejorResultadoNucleos.
        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                j != i &&
                resultado.participantes[j].participo &&
                puntuaciones[j] == puntuaciones[i] &&
                puntuacionesPrevias[j] == puntuacionesPrevias[i]
            )
            {
                std::snprintf(
                    jugador.detalleDesempate,
                    sizeof(jugador.detalleDesempate),
                    golpesRecibidos[i] == 1 ? "%d GOLPE" : "%d GOLPES",
                    golpesRecibidos[i]
                );
                hud.notaFinal = "EMPATE EN PUNTOS: QUEDA ARRIBA QUIEN RECIBIÓ MENOS GOLPES";
                break;
            }
        }

        if (!participantes[i].esBot)
        {
            // Golpe: E / SHIFT DER / X. Golpe al suelo: saltar otra vez en el aire.
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "GOLPE: %s\nAIRE+%s: SUELO",
                TextoBotonGolpeHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoNucleosEnergia::ObtenerResultado() const
{
    return resultado;
}
