#include "Minigames/MinijuegoNucleosEnergia.h"

#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


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


static void ReubicarNucleo(MinijuegoNucleosEnergia& minijuego, int indice)
{
    int puntoElegido = GetRandomValue(0, CANTIDAD_PUNTOS_NUCLEOS - 1);

    for (int intento = 0; intento < 24; intento++)
    {
        int candidato = GetRandomValue(0, CANTIDAD_PUNTOS_NUCLEOS - 1);

        if (!PuntoOcupadoPorOtroNucleo(
            minijuego,
            indice,
            PUNTOS_NUCLEOS[candidato]
        ))
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
    }

    if (cantidadPerder > 0)
    {
        CrearParticulasImpactoGolpe(
            particulas,
            cantidadParticulas,
            posicionJugador
        );
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

    ConfigurarArenaNucleos(*this);

    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        nucleos[i] = {};
        ReubicarNucleo(*this, i);
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

    if (fase == FASE_NUCLEOS_TERMINADO) return;

    tiempoAnimacion += deltaTime;

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
            if (nucleo.tiempoReaparicion <= 0.0f) ReubicarNucleo(*this, n);
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


static void DibujarNucleoVisual(
    Vector3 posicion,
    bool especial,
    float tiempoAnimacion,
    float fase,
    float escala = 1.0f
)
{
    float baseY = posicion.y;
    float oscilacion = std::sin(tiempoAnimacion * 3.1f + fase) * 0.16f;
    posicion.y += oscilacion;
    float pulso = 1.0f + std::sin(tiempoAnimacion * 5.0f + fase) * 0.08f;
    float radio = (especial ? 0.48f : 0.34f) * pulso * escala;
    Color luz = especial ? GOLD : SKYBLUE;

    // Halo en el suelo y haz de luz fino: se ven desde lejos sobre el suelo oscuro.
    for (int k = 0; k < 2; k++)
    {
        DrawCircle3D(
            { posicion.x, 0.02f, posicion.z },
            radio * (1.9f + 0.5f * k) * pulso,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(luz, 0.75f - 0.3f * k)
        );
    }
    DrawCylinderEx(
        { posicion.x, 0.0f, posicion.z },
        { posicion.x, baseY - radio, posicion.z },
        0.025f * escala,
        0.025f * escala,
        6,
        Fade(luz, 0.55f)
    );

    DrawSphere(posicion, radio, luz);
    DrawSphereWires(
        posicion,
        radio + 0.08f * escala,
        8,
        12,
        especial ? ORANGE : RAYWHITE
    );
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

    Vector3 centro = { jugador.posicion.x, 0.05f, jugador.posicion.z };
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


void MinijuegoNucleosEnergia::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    ClearBackground(Color{ 18, 29, 48, 255 });
    BeginMode3D(camara);

    for (int i = 0; i < cantidadBloques; i++)
    {
        const BloquePrueba& bloque = bloques[i];

        // El muro frontal (el ultimo) tapa a los jugadores cercanos a la camara:
        // se dibuja como un borde bajo. La colision no cambia.
        if (i == 4 && !mostrarDebug)
        {
            DrawCubeV(
                { bloque.posicion.x, -0.20f, bloque.posicion.z },
                { bloque.tamano.x, 0.50f, bloque.tamano.z },
                Color{ 45, 125, 154, 255 }
            );
            continue;
        }

        DrawCube(bloque.posicion, bloque.tamano.x, bloque.tamano.y, bloque.tamano.z, bloque.color);
        DrawCubeWires(bloque.posicion, bloque.tamano.x, bloque.tamano.y, bloque.tamano.z, Fade(BLACK, 0.65f));
        if (mostrarDebug) DrawBoundingBox(CrearHitboxBloquePrueba(bloque), YELLOW);

        if (i == 0)
        {
            // Rejilla del suelo y emblema central para dar escala y lectura.
            for (int k = -6; k <= 6; k += 2)
            {
                DrawLine3D({ (float)k, 0.01f, -6.9f }, { (float)k, 0.01f, 6.9f }, Color{ 70, 94, 122, 255 });
                DrawLine3D({ -6.9f, 0.01f, (float)k }, { 6.9f, 0.01f, (float)k }, Color{ 70, 94, 122, 255 });
            }
            DrawCircle3D({ 0.0f, 0.012f, 0.0f }, 1.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 96, 140, 176, 255 });
            DrawCircle3D({ 0.0f, 0.012f, 0.0f }, 1.66f, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 96, 140, 176, 255 });
        }
    }

    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        if (nucleos[i].activo)
        {
            DibujarNucleoVisual(
                nucleos[i].posicion,
                nucleos[i].especial,
                tiempoAnimacion,
                nucleos[i].faseFlotacion
            );
        }
    }

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++)
    {
        if (!nucleosCaidos[i].activo) continue;

        DibujarNucleoVisual(
            nucleosCaidos[i].posicion,
            nucleosCaidos[i].especial,
            tiempoAnimacion,
            (float)i * 0.41f,
            0.82f
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

    EndMode3D();

    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const float e = (float)alto / 720.0f;

    DrawRectangle(0, 0, ancho, (int)(76 * e), Fade(BLACK, 0.55f));
    DrawText("NUCLEOS DE ENERGIA", (int)(24 * e), (int)(10 * e), (int)(30 * e), RAYWHITE);
    DrawText(
        "PRIMERO A 15 PUNTOS O MAS PUNTOS EN 60 S. GOLPE: SUELTA 3. SALTO EN EL AIRE: GOLPE AL SUELO, SUELTA 5.",
        (int)(24 * e),
        (int)(46 * e),
        (int)(18 * e),
        Color{ 166, 195, 218, 255 }
    );

    if (fase == FASE_NUCLEOS_JUGANDO)
    {
        const char* reloj = TextFormat("%.0f", tiempoRestante > 0.0f ? tiempoRestante : 0.0f);
        int tamanoReloj = (int)(46 * e);
        DrawText(
            reloj,
            ancho - MeasureText(reloj, tamanoReloj) - (int)(30 * e),
            (int)(14 * e),
            tamanoReloj,
            tiempoRestante <= 10.0f ? Color{ 255, 90, 80, 255 } : RAYWHITE
        );
    }

    int cantidadTarjetas = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo) cantidadTarjetas++;
    }

    int anchoTarjeta = (int)(220 * e);
    int altoTarjeta = (int)(50 * e);
    int separacion = (int)(14 * e);
    int xTarjeta = ancho / 2 - (cantidadTarjetas * anchoTarjeta + (cantidadTarjetas - 1) * separacion) / 2;
    int yTarjeta = alto - altoTarjeta - (int)(18 * e);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        float proporcion = (float)puntuaciones[i] / (float)PUNTOS_OBJETIVO_NUCLEOS;
        if (proporcion > 1.0f) proporcion = 1.0f;
        if (proporcion < 0.0f) proporcion = 0.0f;

        DrawRectangle(xTarjeta, yTarjeta, anchoTarjeta, altoTarjeta, Fade(BLACK, 0.62f));
        DrawRectangle(xTarjeta, yTarjeta, (int)(8 * e), altoTarjeta, participantes[i].color);
        DrawText(
            TextFormat("J%d%s", participantes[i].numeroJugador, participantes[i].esBot ? " BOT" : ""),
            xTarjeta + (int)(18 * e),
            yTarjeta + (int)(6 * e),
            (int)(20 * e),
            participantes[i].color
        );
        if (fase == FASE_NUCLEOS_JUGANDO && !participantes[i].esBot)
        {
            const char* golpe = participantes[i].control == CONTROL_GAMEPAD
                ? "B"
                : (participantes[i].control == CONTROL_TECLADO_FLECHAS ? "SHIFT DER" : "E");
            DrawText(
                TextFormat("GOLPE: %s | AIRE+%s: SUELO", golpe, ObtenerTextoBotonPrincipal(participantes[i])),
                xTarjeta,
                yTarjeta - (int)(16 * e),
                (int)(12 * e),
                Fade(RAYWHITE, 0.85f)
            );
        }

        const char* puntos = TextFormat("%d / %d", puntuaciones[i], PUNTOS_OBJETIVO_NUCLEOS);
        DrawText(
            puntos,
            xTarjeta + anchoTarjeta - MeasureText(puntos, (int)(20 * e)) - (int)(12 * e),
            yTarjeta + (int)(6 * e),
            (int)(20 * e),
            RAYWHITE
        );

        int xBarra = xTarjeta + (int)(18 * e);
        int yBarra = yTarjeta + (int)(35 * e);
        int anchoBarra = anchoTarjeta - (int)(32 * e);
        DrawRectangle(xBarra, yBarra, anchoBarra, (int)(8 * e), Fade(WHITE, 0.25f));
        DrawRectangle(xBarra, yBarra, (int)(anchoBarra * proporcion), (int)(8 * e), participantes[i].color);

        xTarjeta += anchoTarjeta + separacion;
    }

    if (fase == FASE_NUCLEOS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        int tamano = (int)(96 * e);
        int x = ancho / 2 - MeasureText(texto, tamano) / 2;
        int y = alto / 2 - (int)(190 * e);
        DrawText(texto, x + 4, y + 4, tamano, Fade(BLACK, 0.7f));
        DrawText(texto, x, y, tamano, GOLD);
    }
    else if (fase == FASE_NUCLEOS_TERMINADO)
    {
        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(
            resultado, ganadores, MAX_PARTICIPANTES
        );

        const char* titulo = resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                    ? participantes[ganadores[0]].numeroJugador
                    : 0
            );

        int anchoPanel = (int)(520 * e);
        int altoPanel = (int)(220 * e);
        int xPanel = ancho / 2 - anchoPanel / 2;
        int yPanel = (int)(92 * e);
        DrawRectangle(xPanel, yPanel, anchoPanel, altoPanel, Fade(BLACK, 0.82f));

        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, (int)(32 * e)) / 2,
            yPanel + (int)(14 * e),
            (int)(32 * e),
            GOLD
        );

        int fila = yPanel + (int)(62 * e);
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo) continue;
            DrawText(
                TextFormat(
                    "J%d  POS %d  %d pts",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    puntuaciones[i]
                ),
                xPanel + (int)(140 * e),
                fila,
                (int)(21 * e),
                participantes[i].color
            );
            fila += (int)(27 * e);
        }

        DrawText(
            TextoReinicioMinijuego(),
            ancho / 2 - MeasureText(TextoReinicioMinijuego(), (int)(18 * e)) / 2,
            yPanel + altoPanel - (int)(30 * e),
            (int)(18 * e),
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoNucleosEnergia::ObtenerResultado() const
{
    return resultado;
}
