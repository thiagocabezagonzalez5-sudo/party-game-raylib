#include "Minigames/MinijuegoRefugioPinchos.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_PINCHOS = 2.5f;
static const float DURACION_PARTIDA_PINCHOS = 20.0f;
static const float DURACION_AVISO_PINCHOS = 0.72f;
static const float DURACION_ATAQUE_PINCHOS = 0.78f;
static const float COOLDOWN_ATAQUE_PINCHOS = 0.50f;
static const float BLOQUEO_CAMBIO_PINCHOS = 0.18f;

// La zona caminable termina antes de los muros visibles. Asi ningun jugador
// puede subirse a las bases de los taladros ni quedar atrapado en ellas.
static const float LIMITE_X_JUGADORES_PINCHOS = 4.00f;
static const float LIMITE_Z_JUGADORES_PINCHOS = 3.02f;

static const float X_MURO_PINCHOS = 4.55f;
static const float Z_MURO_PINCHOS = 3.62f;
static const float RADIO_TALADRO_PINCHOS = 0.54f;
static const float LARGO_RETRAIDO_TALADRO = 0.72f;
static const float MARGEN_CHOQUE_COBERTURA = 0.16f;

static const float CARRILES_VERTICAL_PINCHOS[4] =
{
    -3.15f,
    -1.05f,
    1.05f,
    3.15f
};

static const float CARRILES_HORIZONTAL_PINCHOS[4] =
{
    -2.25f,
    -0.75f,
    0.75f,
    2.25f
};


static int ContarRivalesVivosPinchos(
    const MinijuegoRefugioPinchos& minijuego
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static const char* NombreDireccionPinchos(DireccionPinchos direccion)
{
    switch (direccion)
    {
        case PINCHOS_DESDE_ARRIBA: return "ARRIBA";
        case PINCHOS_DESDE_ABAJO: return "ABAJO";
        case PINCHOS_DESDE_IZQUIERDA: return "IZQUIERDA";
        case PINCHOS_DESDE_DERECHA: return "DERECHA";
    }

    return "?";
}


static bool LeerDireccionHumanaPinchos(
    const Participante& participante,
    DireccionPinchos& direccion
)
{
    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(participante);

    if (entrada.arriba)
    {
        direccion = PINCHOS_DESDE_ARRIBA;
        return true;
    }

    if (entrada.abajo)
    {
        direccion = PINCHOS_DESDE_ABAJO;
        return true;
    }

    if (entrada.izquierda)
    {
        direccion = PINCHOS_DESDE_IZQUIERDA;
        return true;
    }

    if (entrada.derecha)
    {
        direccion = PINCHOS_DESDE_DERECHA;
        return true;
    }

    return false;
}


static float DistanciaCuadradaXZ(Vector3 a, Vector3 b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return dx * dx + dz * dz;
}


static void MezclarEnterosPinchos(int valores[], int cantidad)
{
    for (int i = cantidad - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = valores[i];
        valores[i] = valores[otro];
        valores[otro] = temporal;
    }
}


static void CrearCoberturasAleatoriasPinchos(
    MinijuegoRefugioPinchos& minijuego
)
{
    const Vector3 candidatos[12] =
    {
        { -2.75f, 0.88f, -2.05f },
        {  0.00f, 0.88f, -2.05f },
        {  2.75f, 0.88f, -2.05f },
        { -2.75f, 0.88f,  0.00f },
        {  0.00f, 0.88f,  0.00f },
        {  2.75f, 0.88f,  0.00f },
        { -2.75f, 0.88f,  2.05f },
        {  0.00f, 0.88f,  2.05f },
        {  2.75f, 0.88f,  2.05f },
        { -1.40f, 0.88f, -1.05f },
        {  1.40f, 0.88f,  1.05f },
        {  1.40f, 0.88f, -1.05f }
    };

    int orden[12]{};
    for (int i = 0; i < 12; i++) orden[i] = i;
    MezclarEnterosPinchos(orden, 12);

    Vector3 elegidas[3]{};
    int cantidadElegidas = 0;
    const float DISTANCIA_MINIMA = 2.55f;
    const float DISTANCIA_MINIMA_CUADRADA =
        DISTANCIA_MINIMA * DISTANCIA_MINIMA;

    for (int cursor = 0; cursor < 12 && cantidadElegidas < 3; cursor++)
    {
        Vector3 candidato = candidatos[orden[cursor]];
        bool demasiadoCerca = false;

        for (int i = 0; i < cantidadElegidas; i++)
        {
            if (DistanciaCuadradaXZ(candidato, elegidas[i]) < DISTANCIA_MINIMA_CUADRADA)
            {
                demasiadoCerca = true;
                break;
            }
        }

        if (demasiadoCerca)
        {
            continue;
        }

        elegidas[cantidadElegidas++] = candidato;
    }

    const Vector3 respaldo[3] =
    {
        { -2.65f, 0.88f, -1.95f },
        {  2.65f, 0.88f, -0.10f },
        { -0.15f, 0.88f,  2.05f }
    };

    while (cantidadElegidas < 3)
    {
        elegidas[cantidadElegidas] = respaldo[cantidadElegidas];
        cantidadElegidas++;
    }

    for (int i = 0; i < 3; i++)
    {
        AgregarBloquePrueba(
            minijuego.bloques,
            minijuego.cantidadBloques,
            4,
            elegidas[i],
            { 1.62f, 1.76f, 1.62f },
            Color{ 78, 84, 94, 255 }
        );
    }
}


static Vector3 ObtenerPuntoSeguroBotPinchos(
    const MinijuegoRefugioPinchos& minijuego,
    Vector3 posicionJugador
)
{
    Vector3 mejor = posicionJugador;
    float mejorDistancia = 100000.0f;

    for (int i = 1; i < minijuego.cantidadBloques; i++)
    {
        const BloquePrueba& refugio = minijuego.bloques[i];
        Vector3 candidato = refugio.posicion;
        candidato.y = 0.95f;
        const float separacion = 0.82f;

        switch (minijuego.direccionAviso)
        {
            case PINCHOS_DESDE_ARRIBA:
                candidato.z += refugio.tamano.z / 2.0f + separacion;
                break;
            case PINCHOS_DESDE_ABAJO:
                candidato.z -= refugio.tamano.z / 2.0f + separacion;
                break;
            case PINCHOS_DESDE_IZQUIERDA:
                candidato.x += refugio.tamano.x / 2.0f + separacion;
                break;
            case PINCHOS_DESDE_DERECHA:
                candidato.x -= refugio.tamano.x / 2.0f + separacion;
                break;
        }

        if (candidato.x < -LIMITE_X_JUGADORES_PINCHOS)
            candidato.x = -LIMITE_X_JUGADORES_PINCHOS;
        if (candidato.x > LIMITE_X_JUGADORES_PINCHOS)
            candidato.x = LIMITE_X_JUGADORES_PINCHOS;
        if (candidato.z < -LIMITE_Z_JUGADORES_PINCHOS)
            candidato.z = -LIMITE_Z_JUGADORES_PINCHOS;
        if (candidato.z > LIMITE_Z_JUGADORES_PINCHOS)
            candidato.z = LIMITE_Z_JUGADORES_PINCHOS;

        float distancia = DistanciaCuadradaXZ(candidato, posicionJugador);

        if (distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
            mejor = candidato;
        }
    }

    return mejor;
}


static void LimitarJugadorSalaPinchos(JugadorPrueba& jugador)
{
    if (jugador.posicion.x < -LIMITE_X_JUGADORES_PINCHOS)
    {
        jugador.posicion.x = -LIMITE_X_JUGADORES_PINCHOS;
        if (jugador.velocidad.x < 0.0f) jugador.velocidad.x = 0.0f;
        if (jugador.empuje.x < 0.0f) jugador.empuje.x = 0.0f;
    }

    if (jugador.posicion.x > LIMITE_X_JUGADORES_PINCHOS)
    {
        jugador.posicion.x = LIMITE_X_JUGADORES_PINCHOS;
        if (jugador.velocidad.x > 0.0f) jugador.velocidad.x = 0.0f;
        if (jugador.empuje.x > 0.0f) jugador.empuje.x = 0.0f;
    }

    if (jugador.posicion.z < -LIMITE_Z_JUGADORES_PINCHOS)
    {
        jugador.posicion.z = -LIMITE_Z_JUGADORES_PINCHOS;
        if (jugador.velocidad.z < 0.0f) jugador.velocidad.z = 0.0f;
        if (jugador.empuje.z < 0.0f) jugador.empuje.z = 0.0f;
    }

    if (jugador.posicion.z > LIMITE_Z_JUGADORES_PINCHOS)
    {
        jugador.posicion.z = LIMITE_Z_JUGADORES_PINCHOS;
        if (jugador.velocidad.z > 0.0f) jugador.velocidad.z = 0.0f;
        if (jugador.empuje.z > 0.0f) jugador.empuje.z = 0.0f;
    }

    if (jugador.posicion.y < 0.72f)
    {
        jugador.posicion.y = 0.72f;
        jugador.velocidad.y = 0.0f;
        jugador.enSuelo = true;
        jugador.cayendo = false;
    }
}


static void ObtenerOrigenDireccionTaladro(
    DireccionPinchos direccion,
    float carril,
    Vector3& origen,
    Vector3& vectorDireccion
)
{
    switch (direccion)
    {
        case PINCHOS_DESDE_ARRIBA:
            origen = { carril, 0.84f, -Z_MURO_PINCHOS + 0.10f };
            vectorDireccion = { 0.0f, 0.0f, 1.0f };
            break;
        case PINCHOS_DESDE_ABAJO:
            origen = { carril, 0.84f, Z_MURO_PINCHOS - 0.10f };
            vectorDireccion = { 0.0f, 0.0f, -1.0f };
            break;
        case PINCHOS_DESDE_IZQUIERDA:
            origen = { -X_MURO_PINCHOS + 0.10f, 0.84f, carril };
            vectorDireccion = { 1.0f, 0.0f, 0.0f };
            break;
        case PINCHOS_DESDE_DERECHA:
            origen = { X_MURO_PINCHOS - 0.10f, 0.84f, carril };
            vectorDireccion = { -1.0f, 0.0f, 0.0f };
            break;
    }
}


static float ObtenerAlcanceHastaMuroOpuesto(DireccionPinchos direccion)
{
    return
        direccion == PINCHOS_DESDE_ARRIBA ||
        direccion == PINCHOS_DESDE_ABAJO
        ? (Z_MURO_PINCHOS * 2.0f - 0.34f)
        : (X_MURO_PINCHOS * 2.0f - 0.34f);
}


static float ObtenerAlcanceMaximoTaladro(
    const MinijuegoRefugioPinchos& minijuego,
    DireccionPinchos direccion,
    float carril
)
{
    Vector3 origen{};
    Vector3 dir{};
    ObtenerOrigenDireccionTaladro(direccion, carril, origen, dir);

    float maximo = ObtenerAlcanceHastaMuroOpuesto(direccion);

    for (int i = 1; i < minijuego.cantidadBloques; i++)
    {
        BoundingBox caja = CrearHitboxBloquePrueba(minijuego.bloques[i]);
        bool alineado = false;
        float distancia = maximo;

        if (
            direccion == PINCHOS_DESDE_ARRIBA ||
            direccion == PINCHOS_DESDE_ABAJO
        )
        {
            alineado =
                carril + RADIO_TALADRO_PINCHOS >= caja.min.x &&
                carril - RADIO_TALADRO_PINCHOS <= caja.max.x;

            if (alineado)
            {
                distancia = direccion == PINCHOS_DESDE_ARRIBA
                    ? caja.min.z - origen.z
                    : origen.z - caja.max.z;
            }
        }
        else
        {
            alineado =
                carril + RADIO_TALADRO_PINCHOS >= caja.min.z &&
                carril - RADIO_TALADRO_PINCHOS <= caja.max.z;

            if (alineado)
            {
                distancia = direccion == PINCHOS_DESDE_IZQUIERDA
                    ? caja.min.x - origen.x
                    : origen.x - caja.max.x;
            }
        }

        if (alineado && distancia > 0.0f && distancia < maximo)
        {
            maximo = distancia;
        }
    }

    maximo -= MARGEN_CHOQUE_COBERTURA;
    if (maximo < LARGO_RETRAIDO_TALADRO)
        maximo = LARGO_RETRAIDO_TALADRO;

    return maximo;
}


static float ProgresoExtensionTaladros(
    const MinijuegoRefugioPinchos& minijuego
)
{
    if (minijuego.fase != FASE_PINCHOS_ATAQUE) return 0.0f;

    float avance = 1.0f - minijuego.tiempoFase / DURACION_ATAQUE_PINCHOS;
    if (avance < 0.0f) avance = 0.0f;
    if (avance > 1.0f) avance = 1.0f;

    float extension = avance / 0.58f;
    if (extension > 1.0f) extension = 1.0f;
    return extension;
}


static float ObtenerLargoTaladroActual(
    const MinijuegoRefugioPinchos& minijuego,
    DireccionPinchos direccion,
    float carril,
    bool direccionActiva
)
{
    if (!direccionActiva) return LARGO_RETRAIDO_TALADRO;

    float maximo = ObtenerAlcanceMaximoTaladro(minijuego, direccion, carril);
    float progreso = ProgresoExtensionTaladros(minijuego);
    return LARGO_RETRAIDO_TALADRO + (maximo - LARGO_RETRAIDO_TALADRO) * progreso;
}


static bool TaladroTocaJugador(
    const MinijuegoRefugioPinchos& minijuego,
    DireccionPinchos direccion,
    float carril,
    const JugadorPrueba& jugador
)
{
    Vector3 origen{};
    Vector3 dir{};
    ObtenerOrigenDireccionTaladro(direccion, carril, origen, dir);

    float largo = ObtenerLargoTaladroActual(minijuego, direccion, carril, true);
    float dx = jugador.posicion.x - origen.x;
    float dz = jugador.posicion.z - origen.z;
    float longitudinal = dx * dir.x + dz * dir.z;
    float lateral = std::fabs(dx * dir.z - dz * dir.x);
    float radioJugador = jugador.tamano.x * 0.48f;

    return
        longitudinal >= 0.12f &&
        longitudinal <= largo + radioJugador &&
        lateral <= RADIO_TALADRO_PINCHOS + radioJugador &&
        jugador.posicion.y - jugador.tamano.y * 0.5f < 1.58f;
}


static void ResolverImpactosTaladros(
    MinijuegoRefugioPinchos& minijuego,
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    const float* carriles =
        minijuego.direccionAviso == PINCHOS_DESDE_ARRIBA ||
        minijuego.direccionAviso == PINCHOS_DESDE_ABAJO
        ? CARRILES_VERTICAL_PINCHOS
        : CARRILES_HORIZONTAL_PINCHOS;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            minijuego.estadosJugadores[i].eliminado
        )
            continue;

        for (int carril = 0; carril < 4; carril++)
        {
            if (TaladroTocaJugador(minijuego, minijuego.direccionAviso, carriles[carril], jugadores[i]))
            {
                minijuego.estadosJugadores[i].eliminado = true;
                jugadores[i].velocidad = {};
                jugadores[i].empuje = {};
                break;
            }
        }
    }
}


static void FinalizarRefugioPinchos(MinijuegoRefugioPinchos& minijuego, bool ganaSolo)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO) return;

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];
        if (!resultadoJugador.participo) continue;

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;
        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego = esSolo
            ? 3 - ContarRivalesVivosPinchos(minijuego)
            : (!minijuego.estadosJugadores[i].eliminado ? 1 : 0);
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_PINCHOS_TERMINADO;
}


void MinijuegoRefugioPinchos::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        ReiniciarBotRefugioPinchos(estadosBots[i]);
    }

    cantidadBloques = 0;

    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        4,
        { 0.0f, -0.30f, 0.0f },
        { 8.75f, 0.60f, 6.75f },
        Color{ 80, 82, 88, 255 }
    );

    CrearCoberturasAleatoriasPinchos(*this);

    indiceSolo = -1;
    direccionAviso = PINCHOS_DESDE_ARRIBA;
    fase = FASE_PINCHOS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PINCHOS;
    tiempoRestante = DURACION_PARTIDA_PINCHOS;
    tiempoFase = 0.0f;
    cooldownAtaque = 0.0f;
    ataqueResuelto = false;

    camara.position = { 0.0f, 10.4f, 12.3f };
    camara.target = { 0.0f, 0.25f, -0.20f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoRefugioPinchos::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();

    InicializarResultadoMinijuego(resultado, participantes, FORMATO_MINIJUEGO_EQUIPOS);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(participantes, indices, MAX_PARTICIPANTES);

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_PINCHOS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    Vector3 spawnsEquipo[4] =
    {
        { -2.85f, 0.72f,  1.85f },
        {  2.85f, 0.72f,  1.85f },
        { -2.85f, 0.72f, -1.85f },
        {  2.85f, 0.72f, -1.85f }
    };

    int cursorSpawn = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        bool esSolo = i == indiceSolo;
        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;

        if (esSolo)
        {
            ConfigurarJugadorMinijuegoEstandar(jugadores[i], { 0.0f, 3.25f, -5.25f });
            jugadores[i].cayendo = true;
            continue;
        }

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawnsEquipo[cursorSpawn % 4]);
        cursorSpawn++;
    }
}


void MinijuegoRefugioPinchos::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (fase == FASE_PINCHOS_TERMINADO || resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
        return;

    if (fase == FASE_PINCHOS_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PINCHOS_ESPERANDO;
        }
        return;
    }

    tiempoRestante -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;

    if (cooldownAtaque > 0.0f)
    {
        cooldownAtaque -= deltaTime;
        if (cooldownAtaque < 0.0f) cooldownAtaque = 0.0f;
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (i == indiceSolo || !resultado.participantes[i].participo || estadosJugadores[i].eliminado)
            continue;

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot)
        {
            if (fase == FASE_PINCHOS_AVISO || fase == FASE_PINCHOS_ATAQUE)
                estadosBots[i].objetivo = ObtenerPuntoSeguroBotPinchos(*this, jugadores[i].posicion);

            entrada = CrearEntradaBotHaciaObjetivo1v3(jugadores[i].posicion, estadosBots[i].objetivo);
        }
        else if (participantes[i].conectado)
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        entrada.saltar = false;
        entrada.golpear = false;

        ActualizarJugadorPruebaNormal(
            jugadores[i], entrada, bloques, cantidadBloques,
            particulas, cantidadParticulas, false, false, deltaTime
        );

        LimitarJugadorSalaPinchos(jugadores[i]);
    }

    if (fase == FASE_PINCHOS_ESPERANDO)
    {
        if (cooldownAtaque <= 0.0f)
        {
            DireccionPinchos nuevaDireccion = direccionAviso;
            bool iniciar = false;

            if (participantes[indiceSolo].esBot)
            {
                nuevaDireccion = (DireccionPinchos)
                    ElegirDireccionAtaqueBotPinchos(estadosBots[indiceSolo]);
                iniciar = true;
            }
            else if (participantes[indiceSolo].conectado)
            {
                iniciar = LeerDireccionHumanaPinchos(participantes[indiceSolo], nuevaDireccion);
            }

            if (iniciar)
            {
                direccionAviso = nuevaDireccion;
                fase = FASE_PINCHOS_AVISO;
                tiempoFase = DURACION_AVISO_PINCHOS;

                for (int i = 0; i < cantidadMaxima; i++)
                {
                    if (
                        i != indiceSolo &&
                        resultado.participantes[i].participo &&
                        participantes[i].esBot &&
                        !estadosJugadores[i].eliminado
                    )
                    {
                        estadosBots[i].objetivo = ObtenerPuntoSeguroBotPinchos(*this, jugadores[i].posicion);
                    }
                }
            }
        }
    }
    else if (fase == FASE_PINCHOS_AVISO)
    {
        if (
            !participantes[indiceSolo].esBot &&
            participantes[indiceSolo].conectado &&
            tiempoFase > BLOQUEO_CAMBIO_PINCHOS
        )
        {
            DireccionPinchos cambio = direccionAviso;
            if (LeerDireccionHumanaPinchos(participantes[indiceSolo], cambio) && cambio != direccionAviso)
                direccionAviso = cambio;
        }

        tiempoFase -= deltaTime;
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_PINCHOS_ATAQUE;
            tiempoFase = DURACION_ATAQUE_PINCHOS;
            ataqueResuelto = false;
        }
    }
    else if (fase == FASE_PINCHOS_ATAQUE)
    {
        ResolverImpactosTaladros(*this, jugadores, cantidadMaxima);

        if (ContarRivalesVivosPinchos(*this) <= 0)
        {
            FinalizarRefugioPinchos(*this, true);
            return;
        }

        tiempoFase -= deltaTime;
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_PINCHOS_ESPERANDO;
            tiempoFase = 0.0f;
            cooldownAtaque = COOLDOWN_ATAQUE_PINCHOS;
        }
    }

    if (tiempoRestante <= 0.0f)
        FinalizarRefugioPinchos(*this, ContarRivalesVivosPinchos(*this) <= 0);
}


static void DibujarPlataformaFlotantePinchos()
{
    DrawCube({ 0.0f, -0.36f, 0.0f }, 9.55f, 0.72f, 7.55f, Color{ 47, 50, 57, 255 });
    DrawCube({ 0.0f, 0.015f, 0.0f }, 9.15f, 0.06f, 7.15f, Color{ 121, 105, 82, 255 });
    DrawCube({ 0.0f, 0.08f, -3.52f }, 9.55f, 0.15f, 0.14f, Color{ 83, 69, 55, 255 });
    DrawCube({ 0.0f, 0.08f,  3.52f }, 9.55f, 0.15f, 0.14f, Color{ 83, 69, 55, 255 });
    DrawCube({ -4.72f, 0.08f, 0.0f }, 0.14f, 0.15f, 7.15f, Color{ 83, 69, 55, 255 });
    DrawCube({  4.72f, 0.08f, 0.0f }, 0.14f, 0.15f, 7.15f, Color{ 83, 69, 55, 255 });
}


static void DibujarMurosTaladrosPinchos()
{
    Color muro = Color{ 76, 65, 57, 255 };
    Color borde = Color{ 45, 43, 45, 255 };

    DrawCube({ -X_MURO_PINCHOS, 1.12f, 0.0f }, 0.62f, 2.24f, 7.45f, muro);
    DrawCube({  X_MURO_PINCHOS, 1.12f, 0.0f }, 0.62f, 2.24f, 7.45f, muro);
    DrawCube({ 0.0f, 1.12f, -Z_MURO_PINCHOS }, 9.70f, 2.24f, 0.62f, muro);
    DrawCube({ 0.0f, 1.12f,  Z_MURO_PINCHOS }, 9.70f, 2.24f, 0.62f, muro);

    DrawCube({ -X_MURO_PINCHOS + 0.34f, 2.16f, 0.0f }, 0.08f, 0.16f, 7.05f, borde);
    DrawCube({  X_MURO_PINCHOS - 0.34f, 2.16f, 0.0f }, 0.08f, 0.16f, 7.05f, borde);
    DrawCube({ 0.0f, 2.16f, -Z_MURO_PINCHOS + 0.34f }, 9.15f, 0.16f, 0.08f, borde);
    DrawCube({ 0.0f, 2.16f,  Z_MURO_PINCHOS - 0.34f }, 9.15f, 0.16f, 0.08f, borde);
}


static void DibujarSoportesTaladrosPinchos()
{
    Color base = Color{ 55, 58, 65, 255 };
    Color aro = Color{ 128, 133, 142, 255 };

    for (int i = 0; i < 4; i++)
    {
        float x = CARRILES_VERTICAL_PINCHOS[i];
        float z = CARRILES_HORIZONTAL_PINCHOS[i];

        DrawCylinderEx({ x, 0.84f, -Z_MURO_PINCHOS - 0.20f }, { x, 0.84f, -Z_MURO_PINCHOS + 0.26f }, 0.66f, 0.66f, 12, base);
        DrawSphere({ x, 0.84f, -Z_MURO_PINCHOS + 0.28f }, 0.60f, aro);
        DrawCylinderEx({ x, 0.84f, Z_MURO_PINCHOS + 0.20f }, { x, 0.84f, Z_MURO_PINCHOS - 0.26f }, 0.66f, 0.66f, 12, base);
        DrawSphere({ x, 0.84f, Z_MURO_PINCHOS - 0.28f }, 0.60f, aro);
        DrawCylinderEx({ -X_MURO_PINCHOS - 0.20f, 0.84f, z }, { -X_MURO_PINCHOS + 0.26f, 0.84f, z }, 0.66f, 0.66f, 12, base);
        DrawSphere({ -X_MURO_PINCHOS + 0.28f, 0.84f, z }, 0.60f, aro);
        DrawCylinderEx({ X_MURO_PINCHOS + 0.20f, 0.84f, z }, { X_MURO_PINCHOS - 0.26f, 0.84f, z }, 0.66f, 0.66f, 12, base);
        DrawSphere({ X_MURO_PINCHOS - 0.28f, 0.84f, z }, 0.60f, aro);
    }
}


static void DibujarTaladroIndividual(
    const MinijuegoRefugioPinchos& minijuego,
    DireccionPinchos direccion,
    float carril,
    bool activo,
    bool avisado
)
{
    Vector3 origen{};
    Vector3 dir{};
    ObtenerOrigenDireccionTaladro(direccion, carril, origen, dir);

    float largo = ObtenerLargoTaladroActual(minijuego, direccion, carril, activo);

    if (avisado && !activo)
    {
        float pulso = std::sin((float)GetTime() * 34.0f + carril * 2.3f);
        origen.x += dir.x * pulso * 0.11f;
        origen.z += dir.z * pulso * 0.11f;
    }

    float largoPunta = 1.02f;
    if (largoPunta > largo * 0.46f) largoPunta = largo * 0.46f;
    float largoCuerpo = largo - largoPunta;

    Vector3 finCuerpo = { origen.x + dir.x * largoCuerpo, origen.y, origen.z + dir.z * largoCuerpo };
    Vector3 punta = { origen.x + dir.x * largo, origen.y, origen.z + dir.z * largo };

    Color cuerpo = activo || avisado ? Color{ 112, 119, 129, 255 } : Color{ 66, 71, 80, 255 };

    DrawCylinderEx(origen, finCuerpo, RADIO_TALADRO_PINCHOS * 0.88f, RADIO_TALADRO_PINCHOS * 0.88f, 12, cuerpo);
    DrawCylinderEx(finCuerpo, punta, RADIO_TALADRO_PINCHOS * 1.25f, 0.035f, 14,
        activo ? Color{ 209, 215, 224, 255 } : Color{ 143, 149, 158, 255 });

    for (int aro = 0; aro < 3; aro++)
    {
        float t = (float)(aro + 1) / 4.0f;
        Vector3 centro =
        {
            finCuerpo.x + (punta.x - finCuerpo.x) * t,
            origen.y,
            finCuerpo.z + (punta.z - finCuerpo.z) * t
        };
        DrawSphere(centro, RADIO_TALADRO_PINCHOS * (1.02f - t * 0.58f), Color{ 95, 101, 112, 255 });
    }
}


static void DibujarTodosLosTaladros(const MinijuegoRefugioPinchos& minijuego)
{
    for (int direccion = 0; direccion < 4; direccion++)
    {
        DireccionPinchos dir = (DireccionPinchos)direccion;
        const float* carriles =
            dir == PINCHOS_DESDE_ARRIBA || dir == PINCHOS_DESDE_ABAJO
            ? CARRILES_VERTICAL_PINCHOS
            : CARRILES_HORIZONTAL_PINCHOS;

        bool activo = minijuego.fase == FASE_PINCHOS_ATAQUE && minijuego.direccionAviso == dir;
        bool avisado = minijuego.fase == FASE_PINCHOS_AVISO && minijuego.direccionAviso == dir;

        for (int i = 0; i < 4; i++)
            DibujarTaladroIndividual(minijuego, dir, carriles[i], activo, avisado);
    }
}


static void DibujarConsolaControlPinchos(Color colorJugador)
{
    DrawCube({ 0.0f, 2.20f, -5.05f }, 3.20f, 0.34f, 1.75f, Color{ 48, 51, 58, 255 });
    DrawCube({ 0.0f, 2.72f, -4.72f }, 2.45f, 0.78f, 0.62f, Color{ 65, 69, 78, 255 });
    DrawCube({ 0.0f, 2.82f, -4.38f }, 1.65f, 0.38f, 0.08f, Fade(colorJugador, 0.92f));
    DrawCube({ 0.0f, 3.52f, -5.18f }, 0.82f, 1.18f, 0.82f, colorJugador);
    DrawCubeWires({ 0.0f, 3.52f, -5.18f }, 0.82f, 1.18f, 0.82f, BLACK);
}


void MinijuegoRefugioPinchos::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    ClearBackground(BLACK);
    BeginMode3D(camara);

    DrawPlane({ 0.0f, -7.5f, 0.0f }, { 70.0f, 70.0f }, BLACK);

    DibujarPlataformaFlotantePinchos();
    DibujarMurosTaladrosPinchos();
    DibujarSoportesTaladrosPinchos();

    for (int i = 1; i < cantidadBloques; i++)
    {
        const BloquePrueba& bloque = bloques[i];
        DrawCube(bloque.posicion, bloque.tamano.x, bloque.tamano.y, bloque.tamano.z, bloque.color);
        DrawCube(
            { bloque.posicion.x, bloque.posicion.y + bloque.tamano.y * 0.49f, bloque.posicion.z },
            bloque.tamano.x * 0.90f, 0.08f, bloque.tamano.z * 0.90f,
            Color{ 128, 134, 145, 255 }
        );
        DrawCubeWires(bloque.posicion, bloque.tamano.x, bloque.tamano.y, bloque.tamano.z, BLACK);
        if (mostrarDebug) DrawBoundingBox(CrearHitboxBloquePrueba(bloque), YELLOW);
    }

    DibujarTodosLosTaladros(*this);

    Color colorSolo = indiceSolo >= 0 ? participantes[indiceSolo].color : RED;
    DibujarConsolaControlPinchos(colorSolo);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (i == indiceSolo || !resultado.participantes[i].participo || estadosJugadores[i].eliminado)
            continue;

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);
        if (mostrarDebug) DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
    }

    if (mostrarDebug)
    {
        DrawCubeWires(
            { 0.0f, 0.80f, 0.0f },
            LIMITE_X_JUGADORES_PINCHOS * 2.0f,
            1.60f,
            LIMITE_Z_JUGADORES_PINCHOS * 2.0f,
            PURPLE
        );
    }

    EndMode3D();

    DrawText("REFUGIO DE TALADROS - 1 VS 3", 24, 22, 30, RAYWHITE);

    if (indiceSolo >= 0)
    {
        DrawText(
            TextFormat(
                "J%d ES EL CONTROLADOR%s",
                participantes[indiceSolo].numeroJugador,
                participantes[indiceSolo].esBot ? " (BOT)" : ""
            ),
            24, 60, 20, participantes[indiceSolo].color
        );
    }

    DrawText("SOLO: elige un muro | EQUIPO: usa las 3 coberturas", 24, 88, 18, LIGHTGRAY);

    if (fase != FASE_PINCHOS_PREPARACION && fase != FASE_PINCHOS_TERMINADO)
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoRestante),
            GetScreenWidth() - 190, 24, 24,
            tiempoRestante <= 5.0f ? RED : GOLD
        );
    }

    if (fase == FASE_PINCHOS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(texto, GetScreenWidth() / 2 - MeasureText(texto, 84) / 2, 140, 84, GOLD);
    }
    else if (fase == FASE_PINCHOS_AVISO)
    {
        const char* aviso = TextFormat("TALADROS: %s", NombreDireccionPinchos(direccionAviso));
        DrawText(aviso, GetScreenWidth() / 2 - MeasureText(aviso, 30) / 2, 126, 30, ORANGE);
    }

    if (fase == FASE_PINCHOS_TERMINADO)
    {
        bool ganaSolo = ContarRivalesVivosPinchos(*this) <= 0;
        const char* titulo = ganaSolo ? "GANA EL CONTROLADOR" : "GANA EL EQUIPO";

        DrawRectangle(GetScreenWidth() / 2 - 280, GetScreenHeight() / 2 - 90, 560, 180, Fade(BLACK, 0.90f));
        DrawText(titulo, GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2, GetScreenHeight() / 2 - 48, 34, GOLD);
        DrawText("R PARA REINICIAR", GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 20) / 2, GetScreenHeight() / 2 + 22, 20, RAYWHITE);
    }
}


const ResultadoMinijuego& MinijuegoRefugioPinchos::ObtenerResultado() const
{
    return resultado;
}
