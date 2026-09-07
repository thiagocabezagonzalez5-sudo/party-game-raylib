#include "Minigames/MinijuegoNucleosEnergia.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_NUCLEOS = 3.0f;
static const float DURACION_PARTIDA_NUCLEOS = 35.0f;
static const float RADIO_RECOLECCION_NUCLEO = 0.82f;
static const float RETRASO_REAPARICION_NUCLEO = 0.38f;
static const float RADIO_RECOLECCION_CAIDO = 0.76f;
static const float GRAVEDAD_NUCLEO_CAIDO = 12.5f;
static const float ALTURA_SUELO_NUCLEO_CAIDO = 0.34f;
static const float VIDA_NUCLEO_CAIDO = 8.0f;
static const float BLOQUEO_DUENIO_NUCLEO_CAIDO = 0.85f;


static const Vector3 PUNTOS_NUCLEOS[] =
{
    { -4.8f, 0.82f, -4.5f },
    {  0.0f, 0.82f, -4.8f },
    {  4.8f, 0.82f, -4.5f },
    { -4.8f, 0.82f,  0.0f },
    {  0.0f, 0.82f,  0.0f },
    {  4.8f, 0.82f,  0.0f },
    { -4.8f, 0.82f,  4.5f },
    {  0.0f, 0.82f,  4.8f },
    {  4.8f, 0.82f,  4.5f },
    { -2.5f, 0.82f, -2.4f },
    {  2.5f, 0.82f, -2.4f },
    { -2.5f, 0.82f,  2.4f },
    {  2.5f, 0.82f,  2.4f },
    {  0.0f, 0.82f, -2.6f },
    {  0.0f, 0.82f,  2.6f },
    { -2.7f, 0.82f,  0.0f },
    {  2.7f, 0.82f,  0.0f }
};


static const int CANTIDAD_PUNTOS_NUCLEOS =
    sizeof(PUNTOS_NUCLEOS) /
    sizeof(PUNTOS_NUCLEOS[0]);


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
        if (
            i == indiceIgnorado ||
            !minijuego.nucleos[i].activo
        )
        {
            continue;
        }

        float dx = minijuego.nucleos[i].posicion.x - punto.x;
        float dz = minijuego.nucleos[i].posicion.z - punto.z;

        if (dx * dx + dz * dz < 1.6f)
        {
            return true;
        }
    }

    return false;
}


static void ReubicarNucleo(
    MinijuegoNucleosEnergia& minijuego,
    int indice
)
{
    int puntoElegido = GetRandomValue(0, CANTIDAD_PUNTOS_NUCLEOS - 1);

    for (int intento = 0; intento < 24; intento++)
    {
        int candidato = GetRandomValue(0, CANTIDAD_PUNTOS_NUCLEOS - 1);

        if (
            !PuntoOcupadoPorOtroNucleo(
                minijuego,
                indice,
                PUNTOS_NUCLEOS[candidato]
            )
        )
        {
            puntoElegido = candidato;
            break;
        }
    }

    NucleoEnergia& nucleo = minijuego.nucleos[indice];
    nucleo.posicion = PUNTOS_NUCLEOS[puntoElegido];

    int probabilidadEspecial =
        minijuego.tiempoRestante <= 10.0f
        ? 30
        : 16;

    nucleo.especial =
        GetRandomValue(1, 100) <= probabilidadEspecial;

    nucleo.activo = true;
    nucleo.tiempoReaparicion = 0.0f;
    nucleo.faseFlotacion =
        (float)GetRandomValue(0, 628) / 100.0f;
}


static void ConfigurarArenaNucleos(
    MinijuegoNucleosEnergia& minijuego
)
{
    minijuego.cantidadBloques = 0;

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        { 0.0f, -0.45f, 0.0f },
        { 14.0f, 0.90f, 14.0f },
        Color{ 44, 62, 83, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        { -7.15f, 0.55f, 0.0f },
        { 0.30f, 2.0f, 14.6f },
        Color{ 45, 125, 154, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        { 7.15f, 0.55f, 0.0f },
        { 0.30f, 2.0f, 14.6f },
        Color{ 45, 125, 154, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        { 0.0f, 0.55f, -7.15f },
        { 14.0f, 2.0f, 0.30f },
        Color{ 45, 125, 154, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        { 0.0f, 0.55f, 7.15f },
        { 14.0f, 2.0f, 0.30f },
        Color{ 45, 125, 154, 255 }
    );
}


static void AgregarAlHistorialNucleos(
    MinijuegoNucleosEnergia& minijuego,
    int jugador,
    bool especial
)
{
    if (jugador < 0 || jugador >= MAX_PARTICIPANTES)
    {
        return;
    }

    HistorialNucleosJugador& historial =
        minijuego.historial[jugador];

    if (historial.cantidad >= MAX_HISTORIAL_NUCLEOS)
    {
        for (int i = 1; i < MAX_HISTORIAL_NUCLEOS; i++)
        {
            historial.especiales[i - 1] = historial.especiales[i];
        }

        historial.cantidad = MAX_HISTORIAL_NUCLEOS - 1;
    }

    historial.especiales[historial.cantidad] = especial;
    historial.cantidad++;
}


static int BuscarSlotNucleoCaido(
    MinijuegoNucleosEnergia& minijuego
)
{
    int mejor = 0;
    float menorVida = 100000.0f;

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++)
    {
        if (!minijuego.nucleosCaidos[i].activo)
        {
            return i;
        }

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
        ((float)GetRandomValue(0, 6283) / 1000.0f) +
        indiceSalida * 2.05f;

    float fuerza =
        (float)GetRandomValue(24, 42) / 10.0f;

    caido = {};
    caido.activo = true;
    caido.especial = especial;
    caido.jugadorBloqueado = jugador;
    caido.tiempoBloqueo = BLOQUEO_DUENIO_NUCLEO_CAIDO;
    caido.tiempoVida = VIDA_NUCLEO_CAIDO;
    caido.posicion = origen;
    caido.posicion.y += 0.45f;
    caido.velocidad =
    {
        std::cos(angulo) * fuerza,
        (float)GetRandomValue(45, 62) / 10.0f,
        std::sin(angulo) * fuerza
    };
}


static void PerderUltimosNucleosPorGolpe(
    MinijuegoNucleosEnergia& minijuego,
    int jugador,
    Vector3 posicionJugador,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (jugador < 0 || jugador >= MAX_PARTICIPANTES)
    {
        return;
    }

    HistorialNucleosJugador& historial = minijuego.historial[jugador];

    int cantidadPerder = historial.cantidad < 3
        ? historial.cantidad
        : 3;

    if (cantidadPerder <= 0)
    {
        return;
    }

    for (int i = 0; i < cantidadPerder; i++)
    {
        int indiceHistorial = historial.cantidad - 1;
        bool especial = historial.especiales[indiceHistorial];
        historial.cantidad--;

        minijuego.puntuaciones[jugador] -= ValorNucleo(especial);
        if (minijuego.puntuaciones[jugador] < 0)
        {
            minijuego.puntuaciones[jugador] = 0;
        }

        SoltarNucleoPorGolpe(
            minijuego,
            jugador,
            posicionJugador,
            especial,
            i
        );
    }

    CrearParticulasImpactoGolpe(
        particulas,
        cantidadParticulas,
        posicionJugador
    );
}


static bool JugadorPuedeRecogerNucleo(
    const Participante& participante,
    const JugadorPrueba& jugador
)
{
    return
        participante.activo &&
        participante.conectado &&
        !participante.esBot &&
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

    return
        dx * dx + dz * dz <= radio * radio &&
        diferenciaY <= 1.25f;
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
            especial
                ? SONIDO_RECOGER_NUCLEO_ESPECIAL
                : SONIDO_RECOGER_NUCLEO
        );
    }

    CrearParticulasImpactoGolpe(
        particulas,
        cantidadParticulas,
        posicion
    );
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

        if (!caido.activo)
        {
            continue;
        }

        caido.tiempoVida -= deltaTime;
        caido.tiempoBloqueo -= deltaTime;

        if (caido.tiempoBloqueo < 0.0f)
        {
            caido.tiempoBloqueo = 0.0f;
        }

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
            {
                caido.velocidad.y *= -0.28f;
            }
            else
            {
                caido.velocidad.y = 0.0f;
            }

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
            if (!JugadorPuedeRecogerNucleo(participantes[i], jugadores[i]))
            {
                continue;
            }

            if (
                i == caido.jugadorBloqueado &&
                caido.tiempoBloqueo > 0.0f
            )
            {
                continue;
            }

            if (
                !JugadorTocaPuntoNucleo(
                    jugadores[i],
                    caido.posicion,
                    RADIO_RECOLECCION_CAIDO
                )
            )
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


static void FinalizarNucleos(
    MinijuegoNucleosEnergia& minijuego,
    AudioJuego* audio
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int mejorPuntaje = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.puntuaciones[i] > mejorPuntaje
        )
        {
            mejorPuntaje = minijuego.puntuaciones[i];
        }
    }

    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.puntuaciones[i] == mejorPuntaje
        )
        {
            cantidadGanadores++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
            ? DESENLACE_CON_GANADOR
            : DESENLACE_EMPATE;

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
                minijuego.puntuaciones[j] > minijuego.puntuaciones[i]
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

    if (audio != nullptr)
    {
        audio->ReproducirSonido(SONIDO_RESULTADO);
    }
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
        historial[i] = {};
    }

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++)
    {
        nucleosCaidos[i] = {};
    }

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
        { -4.8f, 1.0f, 4.8f },
        {  4.8f, 1.0f, 4.8f },
        { -4.8f, 1.0f, -4.8f },
        {  4.8f, 1.0f, -4.8f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
            ? cantidadMaxima
            : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawns[i]);
    }
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

    if (fase == FASE_NUCLEOS_TERMINADO)
    {
        return;
    }

    tiempoAnimacion += deltaTime;

    if (fase == FASE_NUCLEOS_PREPARACION)
    {
        int numeroCuenta = (int)std::ceil(tiempoPreparacion);

        if (
            numeroCuenta > 0 &&
            numeroCuenta != ultimoNumeroCuenta
        )
        {
            ultimoNumeroCuenta = numeroCuenta;

            if (audio != nullptr)
            {
                audio->ReproducirSonido(SONIDO_CUENTA_REGRESIVA);
            }
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

            if (audio != nullptr)
            {
                audio->ReproducirSonido(SONIDO_INICIO_MINIJUEGO);
            }
        }

        return;
    }

    tiempoRestante -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;

    int segundoAlerta = (int)std::ceil(tiempoRestante);

    if (
        segundoAlerta > 0 &&
        segundoAlerta <= 5 &&
        segundoAlerta != ultimoNumeroAlerta
    )
    {
        ultimoNumeroAlerta = segundoAlerta;

        if (audio != nullptr)
        {
            audio->ReproducirSonido(SONIDO_ALERTA_TIEMPO);
        }
    }

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
            ? cantidadMaxima
            : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].conectado)
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

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
    {
        ralentizacionAntes[i] = jugadores[i].tiempoRalentizado;
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        participantes,
        limite,
        particulas,
        cantidadParticulas
    );

    for (int i = 0; i < limite; i++)
    {
        if (
            jugadores[i].tiempoRalentizado >
                ralentizacionAntes[i] + 0.20f
        )
        {
            PerderUltimosNucleosPorGolpe(
                *this,
                i,
                jugadores[i].posicion,
                particulas,
                cantidadParticulas
            );
        }
    }

    for (int n = 0; n < MAX_NUCLEOS_ENERGIA; n++)
    {
        NucleoEnergia& nucleo = nucleos[n];

        if (!nucleo.activo)
        {
            nucleo.tiempoReaparicion -= deltaTime;

            if (nucleo.tiempoReaparicion <= 0.0f)
            {
                ReubicarNucleo(*this, n);
            }

            continue;
        }

        for (int i = 0; i < limite; i++)
        {
            if (!JugadorPuedeRecogerNucleo(participantes[i], jugadores[i]))
            {
                continue;
            }

            if (
                !JugadorTocaPuntoNucleo(
                    jugadores[i],
                    nucleo.posicion,
                    RADIO_RECOLECCION_NUCLEO
                )
            )
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

    if (tiempoRestante <= 0.0f)
    {
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
    float oscilacion =
        std::sin(tiempoAnimacion * 3.1f + fase) * 0.16f;

    posicion.y += oscilacion;

    float pulso =
        1.0f +
        std::sin(tiempoAnimacion * 5.0f + fase) * 0.08f;

    float radio =
        (especial ? 0.48f : 0.34f) * pulso * escala;

    DrawSphere(
        posicion,
        radio,
        especial ? GOLD : SKYBLUE
    );

    DrawSphereWires(
        posicion,
        radio + 0.08f * escala,
        8,
        12,
        especial ? ORANGE : RAYWHITE
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

        DrawCube(
            bloque.posicion,
            bloque.tamano.x,
            bloque.tamano.y,
            bloque.tamano.z,
            bloque.color
        );

        DrawCubeWires(
            bloque.posicion,
            bloque.tamano.x,
            bloque.tamano.y,
            bloque.tamano.z,
            Fade(BLACK, 0.65f)
        );

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloque), YELLOW);
        }
    }

    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        if (!nucleos[i].activo)
        {
            continue;
        }

        DibujarNucleoVisual(
            nucleos[i].posicion,
            nucleos[i].especial,
            tiempoAnimacion,
            nucleos[i].faseFlotacion
        );
    }

    for (int i = 0; i < MAX_NUCLEOS_CAIDOS; i++)
    {
        if (!nucleosCaidos[i].activo)
        {
            continue;
        }

        DibujarNucleoVisual(
            nucleosCaidos[i].posicion,
            nucleosCaidos[i].especial,
            tiempoAnimacion,
            (float)i * 0.41f,
            0.82f
        );

        DrawCircle3D(
            { nucleosCaidos[i].posicion.x, 0.03f, nucleosCaidos[i].posicion.z },
            nucleosCaidos[i].especial ? 0.42f : 0.30f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(nucleosCaidos[i].especial ? GOLD : SKYBLUE, 0.50f)
        );
    }

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
            ? cantidadMaxima
            : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);

        if (mostrarDebug && !jugadores[i].cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    EndMode3D();

    DrawText("NUCLEOS DE ENERGIA", 24, 22, 30, RAYWHITE);
    DrawText(
        "RECOLECTA ENERGIA. SI TE GOLPEAN, SUELTAS TUS ULTIMAS 3 RECOGIDAS.",
        24,
        60,
        18,
        LIGHTGRAY
    );

    DrawText(
        "GOLPE: E / SHIFT / B   |   GOLPE AL SUELO: SALTO EN EL AIRE",
        24,
        86,
        16,
        Color{ 166, 195, 218, 255 }
    );

    int y = 120;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        DrawText(
            TextFormat(
                "J%d%s  %d pts",
                participantes[i].numeroJugador,
                participantes[i].esBot ? " BOT" : "",
                puntuaciones[i]
            ),
            24,
            y,
            19,
            participantes[i].color
        );

        y += 25;
    }

    if (fase == FASE_NUCLEOS_JUGANDO)
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoRestante),
            GetScreenWidth() - 190,
            24,
            24,
            tiempoRestante <= 5.0f ? RED : GOLD
        );
    }

    if (fase == FASE_NUCLEOS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            GOLD
        );
    }
    else if (fase == FASE_NUCLEOS_TERMINADO)
    {
        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores =
            ObtenerIndicesGanadores(
                resultado,
                ganadores,
                MAX_PARTICIPANTES
            );

        const char* titulo =
            resultado.desenlace == DESENLACE_EMPATE
                ? "EMPATE"
                : TextFormat(
                    "GANADOR: JUGADOR %d",
                    cantidadGanadores == 1
                        ? participantes[ganadores[0]].numeroJugador
                        : 0
                );

        DrawRectangle(
            GetScreenWidth() / 2 - 315,
            GetScreenHeight() / 2 - 140,
            630,
            280,
            Fade(BLACK, 0.90f)
        );

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 108,
            34,
            GOLD
        );

        int fila = GetScreenHeight() / 2 - 50;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POS %d  %d pts",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    puntuaciones[i]
                ),
                GetScreenWidth() / 2 - 170,
                fila,
                21,
                participantes[i].color
            );

            fila += 29;
        }

        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 21) / 2,
            GetScreenHeight() / 2 + 105,
            21,
            RAYWHITE
        );
    }
}
