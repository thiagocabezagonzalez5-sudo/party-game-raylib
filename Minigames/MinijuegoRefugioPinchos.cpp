#include "Minigames/MinijuegoRefugioPinchos.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_PINCHOS = 2.5f;
static const float DURACION_PARTIDA_PINCHOS = 20.0f;
static const float DURACION_AVISO_PINCHOS = 0.72f;
static const float DURACION_ATAQUE_PINCHOS = 0.72f;
static const float COOLDOWN_ATAQUE_PINCHOS = 0.52f;
static const float BLOQUEO_CAMBIO_PINCHOS = 0.18f;

static const float LIMITE_X_SALA_PINCHOS = 4.65f;
static const float LIMITE_Z_SALA_PINCHOS = 3.85f;
static const float RADIO_TALADRO_PINCHOS = 0.34f;
static const float LARGO_RETRAIDO_TALADRO = 1.05f;
static const float MARGEN_CHOQUE_COBERTURA = 0.10f;


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
    const float xs[4] = { -2.70f, -0.90f, 0.90f, 2.70f };
    const float zs[3] = { -2.00f, 0.0f, 2.00f };

    int candidatos[12]{};
    for (int i = 0; i < 12; i++) candidatos[i] = i;
    MezclarEnterosPinchos(candidatos, 12);

    for (int i = 0; i < 4; i++)
    {
        int indice = candidatos[i];
        int ix = indice % 4;
        int iz = indice / 4;

        AgregarBloquePrueba(
            minijuego.bloques,
            minijuego.cantidadBloques,
            5,
            { xs[ix], 0.85f, zs[iz] },
            { 1.50f, 1.70f, 1.70f },
            Color{ 82, 88, 98, 255 }
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

        if (candidato.x < -4.25f) candidato.x = -4.25f;
        if (candidato.x > 4.25f) candidato.x = 4.25f;
        if (candidato.z < -3.45f) candidato.z = -3.45f;
        if (candidato.z > 3.45f) candidato.z = 3.45f;

        float dx = candidato.x - posicionJugador.x;
        float dz = candidato.z - posicionJugador.z;
        float distancia = dx * dx + dz * dz;

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
    if (jugador.posicion.x < -LIMITE_X_SALA_PINCHOS)
        jugador.posicion.x = -LIMITE_X_SALA_PINCHOS;
    if (jugador.posicion.x > LIMITE_X_SALA_PINCHOS)
        jugador.posicion.x = LIMITE_X_SALA_PINCHOS;
    if (jugador.posicion.z < -LIMITE_Z_SALA_PINCHOS)
        jugador.posicion.z = -LIMITE_Z_SALA_PINCHOS;
    if (jugador.posicion.z > LIMITE_Z_SALA_PINCHOS)
        jugador.posicion.z = LIMITE_Z_SALA_PINCHOS;
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
            origen = { carril, 0.82f, -4.45f };
            vectorDireccion = { 0.0f, 0.0f, 1.0f };
            break;
        case PINCHOS_DESDE_ABAJO:
            origen = { carril, 0.82f, 4.45f };
            vectorDireccion = { 0.0f, 0.0f, -1.0f };
            break;
        case PINCHOS_DESDE_IZQUIERDA:
            origen = { -5.05f, 0.82f, carril };
            vectorDireccion = { 1.0f, 0.0f, 0.0f };
            break;
        case PINCHOS_DESDE_DERECHA:
            origen = { 5.05f, 0.82f, carril };
            vectorDireccion = { -1.0f, 0.0f, 0.0f };
            break;
    }
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

    float maximo =
        direccion == PINCHOS_DESDE_ARRIBA ||
        direccion == PINCHOS_DESDE_ABAJO
        ? 8.90f
        : 10.10f;

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

        if (
            alineado &&
            distancia > 0.0f &&
            distancia < maximo
        )
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

    float avance =
        1.0f - minijuego.tiempoFase / DURACION_ATAQUE_PINCHOS;

    if (avance < 0.0f) avance = 0.0f;
    if (avance > 1.0f) avance = 1.0f;

    // Entra rapidamente y despues queda frenado contra la primera pared.
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

    return LARGO_RETRAIDO_TALADRO +
        (maximo - LARGO_RETRAIDO_TALADRO) * progreso;
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

    float largo = ObtenerLargoTaladroActual(
        minijuego,
        direccion,
        carril,
        true
    );

    float dx = jugador.posicion.x - origen.x;
    float dz = jugador.posicion.z - origen.z;
    float longitudinal = dx * dir.x + dz * dir.z;
    float lateral = std::fabs(dx * dir.z - dz * dir.x);
    float radioJugador = jugador.tamano.x * 0.43f;

    return
        longitudinal >= 0.18f &&
        longitudinal <= largo + radioJugador &&
        lateral <= RADIO_TALADRO_PINCHOS + radioJugador &&
        jugador.posicion.y - jugador.tamano.y * 0.5f < 1.45f;
}


static void ResolverImpactosTaladros(
    MinijuegoRefugioPinchos& minijuego,
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    const float carrilesVerticales[4] = { -3.45f, -1.15f, 1.15f, 3.45f };
    const float carrilesHorizontales[4] = { -2.40f, -0.80f, 0.80f, 2.40f };

    const float* carriles =
        minijuego.direccionAviso == PINCHOS_DESDE_ARRIBA ||
        minijuego.direccionAviso == PINCHOS_DESDE_ABAJO
        ? carrilesVerticales
        : carrilesHorizontales;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            minijuego.estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        for (int carril = 0; carril < 4; carril++)
        {
            if (
                TaladroTocaJugador(
                    minijuego,
                    minijuego.direccionAviso,
                    carriles[carril],
                    jugadores[i]
                )
            )
            {
                minijuego.estadosJugadores[i].eliminado = true;
                jugadores[i].cayendo = true;
                jugadores[i].velocidad = {};
                jugadores[i].empuje = {};
                break;
            }
        }
    }
}


static void FinalizarRefugioPinchos(
    MinijuegoRefugioPinchos& minijuego,
    bool ganaSolo
)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO) return;

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo) continue;

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;

        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego =
            esSolo
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
        5,
        { 0.0f, -0.35f, 0.0f },
        { 10.5f, 0.70f, 8.8f },
        Color{ 108, 86, 66, 255 }
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

    camara.position = { 0.0f, 10.8f, 10.8f };
    camara.target = { 0.0f, 0.35f, 0.0f };
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

    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
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
        fase = FASE_PINCHOS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    Vector3 spawnsEquipo[4] =
    {
        { -3.2f, 0.95f, 2.8f },
        {  3.2f, 0.95f, 2.8f },
        { -3.2f, 0.95f, -2.8f },
        {  3.2f, 0.95f, -2.8f }
    };

    int cursorSpawn = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        bool esSolo = i == indiceSolo;
        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;

        if (esSolo)
        {
            ConfigurarJugadorMinijuegoEstandar(
                jugadores[i],
                { -5.8f, 1.0f, -4.7f }
            );
            jugadores[i].cayendo = true;
            continue;
        }

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            spawnsEquipo[cursorSpawn % 4]
        );
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
    if (
        fase == FASE_PINCHOS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

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
        if (
            i == indiceSolo ||
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot)
        {
            if (
                fase == FASE_PINCHOS_AVISO ||
                fase == FASE_PINCHOS_ATAQUE
            )
            {
                estadosBots[i].objetivo = ObtenerPuntoSeguroBotPinchos(
                    *this,
                    jugadores[i].posicion
                );
            }

            entrada = CrearEntradaBotHaciaObjetivo1v3(
                jugadores[i].posicion,
                estadosBots[i].objetivo
            );
        }
        else if (participantes[i].conectado)
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        entrada.saltar = false;
        entrada.golpear = false;

        ActualizarJugadorPruebaNormal(
            jugadores[i],
            entrada,
            bloques,
            cantidadBloques,
            particulas,
            cantidadParticulas,
            false,
            false,
            deltaTime
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
                iniciar = LeerDireccionHumanaPinchos(
                    participantes[indiceSolo],
                    nuevaDireccion
                );
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
                        estadosBots[i].objetivo = ObtenerPuntoSeguroBotPinchos(
                            *this,
                            jugadores[i].posicion
                        );
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

            if (
                LeerDireccionHumanaPinchos(participantes[indiceSolo], cambio) &&
                cambio != direccionAviso
            )
            {
                direccionAviso = cambio;
            }
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
    {
        FinalizarRefugioPinchos(
            *this,
            ContarRivalesVivosPinchos(*this) <= 0
        );
    }
}


static void DibujarCuevaLocalPinchos()
{
    DrawCube({ -6.0f, 2.35f, 0.0f }, 1.7f, 6.0f, 10.5f, Color{ 58, 51, 48, 255 });
    DrawCube({  6.0f, 2.35f, 0.0f }, 1.7f, 6.0f, 10.5f, Color{ 58, 51, 48, 255 });
    DrawCube({ 0.0f, 2.55f, -5.15f }, 13.0f, 6.4f, 1.4f, Color{ 52, 47, 45, 255 });
    DrawCube({ 0.0f, 6.65f, -2.65f }, 13.0f, 1.1f, 4.4f, Color{ 48, 44, 43, 255 });

    for (int i = 0; i < 8; i++)
    {
        float x = -5.0f + (float)i * 1.42f;
        float z = -4.2f + (float)(i % 2) * 0.7f;
        DrawCylinderEx(
            { x, 6.2f, z },
            { x, 5.15f - (float)(i % 3) * 0.22f, z },
            0.26f,
            0.02f,
            7,
            Color{ 76, 68, 62, 255 }
        );
    }
}


static void DibujarTaladroIndividual(
    const MinijuegoRefugioPinchos& minijuego,
    DireccionPinchos direccion,
    float carril,
    bool activo
)
{
    Vector3 origen{};
    Vector3 dir{};
    ObtenerOrigenDireccionTaladro(direccion, carril, origen, dir);

    float largo = ObtenerLargoTaladroActual(
        minijuego,
        direccion,
        carril,
        activo
    );

    float largoPunta = 0.78f;
    if (largoPunta > largo * 0.42f) largoPunta = largo * 0.42f;
    float largoCuerpo = largo - largoPunta;

    Vector3 finCuerpo =
    {
        origen.x + dir.x * largoCuerpo,
        origen.y,
        origen.z + dir.z * largoCuerpo
    };

    Vector3 punta =
    {
        origen.x + dir.x * largo,
        origen.y,
        origen.z + dir.z * largo
    };

    Color cuerpo = activo
        ? Color{ 119, 124, 132, 255 }
        : Color{ 73, 77, 84, 255 };

    DrawCylinderEx(
        origen,
        finCuerpo,
        RADIO_TALADRO_PINCHOS * 0.78f,
        RADIO_TALADRO_PINCHOS * 0.78f,
        10,
        cuerpo
    );

    DrawCylinderEx(
        finCuerpo,
        punta,
        RADIO_TALADRO_PINCHOS * 1.28f,
        0.025f,
        12,
        activo
            ? Color{ 196, 202, 211, 255 }
            : Color{ 125, 130, 138, 255 }
    );

    for (int aro = 0; aro < 3; aro++)
    {
        float t = (float)(aro + 1) / 4.0f;
        Vector3 centro =
        {
            finCuerpo.x + (punta.x - finCuerpo.x) * t,
            origen.y,
            finCuerpo.z + (punta.z - finCuerpo.z) * t
        };

        DrawSphere(
            centro,
            RADIO_TALADRO_PINCHOS * (0.95f - t * 0.55f),
            Color{ 102, 107, 116, 255 }
        );
    }
}


static void DibujarTodosLosTaladros(
    const MinijuegoRefugioPinchos& minijuego
)
{
    const float verticales[4] = { -3.45f, -1.15f, 1.15f, 3.45f };
    const float horizontales[4] = { -2.40f, -0.80f, 0.80f, 2.40f };

    for (int direccion = 0; direccion < 4; direccion++)
    {
        DireccionPinchos dir = (DireccionPinchos)direccion;
        const float* carriles =
            dir == PINCHOS_DESDE_ARRIBA || dir == PINCHOS_DESDE_ABAJO
            ? verticales
            : horizontales;

        bool activo =
            minijuego.fase == FASE_PINCHOS_ATAQUE &&
            minijuego.direccionAviso == dir;

        for (int i = 0; i < 4; i++)
        {
            DibujarTaladroIndividual(minijuego, dir, carriles[i], activo);
        }
    }
}


void MinijuegoRefugioPinchos::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    // El tema global dibujaba un segundo juego de taladros encima de los del
    // minijuego. Lo anulamos aqui y dibujamos una unica cueva con 16 taladros
    // fisicos, usando la misma longitud para dibujo y colision.
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    ClearBackground(Color{ 38, 31, 29, 255 });
    BeginMode3D(camara);

    DibujarCuevaLocalPinchos();

    DrawCube(
        { 0.0f, -0.70f, 0.0f },
        11.5f,
        0.70f,
        9.8f,
        Color{ 65, 54, 48, 255 }
    );

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
            BLACK
        );

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloque), YELLOW);
        }
    }

    DibujarTodosLosTaladros(*this);

    DrawCube(
        { -5.4f, 1.2f, -4.0f },
        1.25f,
        1.10f,
        1.25f,
        participantes[indiceSolo >= 0 ? indiceSolo : 0].color
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == indiceSolo ||
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);
    }

    if (fase == FASE_PINCHOS_AVISO)
    {
        Color aviso = Fade(RED, 0.48f);

        if (direccionAviso == PINCHOS_DESDE_ARRIBA)
            DrawCube({ 0.0f, 0.03f, -4.25f }, 9.6f, 0.06f, 0.22f, aviso);
        else if (direccionAviso == PINCHOS_DESDE_ABAJO)
            DrawCube({ 0.0f, 0.03f, 4.25f }, 9.6f, 0.06f, 0.22f, aviso);
        else if (direccionAviso == PINCHOS_DESDE_IZQUIERDA)
            DrawCube({ -5.05f, 0.03f, 0.0f }, 0.22f, 0.06f, 8.0f, aviso);
        else
            DrawCube({ 5.05f, 0.03f, 0.0f }, 0.22f, 0.06f, 8.0f, aviso);
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
            24,
            60,
            20,
            participantes[indiceSolo].color
        );
    }

    DrawText(
        "SOLO: elige un lado | EQUIPO: escondete detras de los bloques",
        24,
        88,
        18,
        LIGHTGRAY
    );

    if (
        fase != FASE_PINCHOS_PREPARACION &&
        fase != FASE_PINCHOS_TERMINADO
    )
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoRestante),
            GetScreenWidth() - 190,
            24,
            24,
            tiempoRestante <= 5.0f ? RED : GOLD
        );
    }

    if (fase == FASE_PINCHOS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
            140,
            84,
            GOLD
        );
    }
    else if (fase == FASE_PINCHOS_AVISO)
    {
        const char* aviso = TextFormat(
            "PELIGRO DESDE %s",
            NombreDireccionPinchos(direccionAviso)
        );

        DrawText(
            aviso,
            GetScreenWidth() / 2 - MeasureText(aviso, 32) / 2,
            128,
            32,
            RED
        );
    }

    if (fase == FASE_PINCHOS_TERMINADO)
    {
        bool ganaSolo = ContarRivalesVivosPinchos(*this) <= 0;
        const char* titulo = ganaSolo
            ? "GANA EL CONTROLADOR"
            : "GANA EL EQUIPO";

        DrawRectangle(
            GetScreenWidth() / 2 - 280,
            GetScreenHeight() / 2 - 90,
            560,
            180,
            Fade(BLACK, 0.90f)
        );

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 48,
            34,
            GOLD
        );

        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 20) / 2,
            GetScreenHeight() / 2 + 22,
            20,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoRefugioPinchos::ObtenerResultado() const
{
    return resultado;
}
