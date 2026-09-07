#include "Minigames/MinijuegoRefugioPinchos.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_PINCHOS = 2.5f;
static const float DURACION_PARTIDA_PINCHOS = 20.0f;
static const float DURACION_AVISO_PINCHOS = 0.72f;
static const float DURACION_ATAQUE_PINCHOS = 0.52f;
static const float COOLDOWN_ATAQUE_PINCHOS = 0.48f;
static const float BLOQUEO_CAMBIO_PINCHOS = 0.18f;


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


static const char* NombreDireccionPinchos(
    DireccionPinchos direccion
)
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


static bool RefugioProtegeJugador(
    const BloquePrueba& refugio,
    const JugadorPrueba& jugador,
    DireccionPinchos direccion
)
{
    const float margenLateral =
        jugador.tamano.x * 0.45f;

    if (
        direccion == PINCHOS_DESDE_ARRIBA ||
        direccion == PINCHOS_DESDE_ABAJO
    )
    {
        bool alineado =
            std::fabs(
                jugador.posicion.x -
                refugio.posicion.x
            ) <= refugio.tamano.x / 2.0f + margenLateral;

        if (!alineado)
        {
            return false;
        }

        if (direccion == PINCHOS_DESDE_ARRIBA)
        {
            return refugio.posicion.z < jugador.posicion.z;
        }

        return refugio.posicion.z > jugador.posicion.z;
    }

    bool alineado =
        std::fabs(
            jugador.posicion.z -
            refugio.posicion.z
        ) <= refugio.tamano.z / 2.0f + margenLateral;

    if (!alineado)
    {
        return false;
    }

    if (direccion == PINCHOS_DESDE_IZQUIERDA)
    {
        return refugio.posicion.x < jugador.posicion.x;
    }

    return refugio.posicion.x > jugador.posicion.x;
}


static bool JugadorEstaProtegidoPinchos(
    const MinijuegoRefugioPinchos& minijuego,
    const JugadorPrueba& jugador
)
{
    for (int i = 1; i < minijuego.cantidadBloques; i++)
    {
        if (
            RefugioProtegeJugador(
                minijuego.bloques[i],
                jugador,
                minijuego.direccionAviso
            )
        )
        {
            return true;
        }
    }

    return false;
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

        const float separacion = 1.05f;

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


static void LimitarJugadorSalaPinchos(
    JugadorPrueba& jugador
)
{
    if (jugador.posicion.x < -4.65f) jugador.posicion.x = -4.65f;
    if (jugador.posicion.x > 4.65f) jugador.posicion.x = 4.65f;
    if (jugador.posicion.z < -3.85f) jugador.posicion.z = -3.85f;
    if (jugador.posicion.z > 3.85f) jugador.posicion.z = 3.85f;
}


static void FinalizarRefugioPinchos(
    MinijuegoRefugioPinchos& minijuego,
    bool ganaSolo
)
{
    if (
        minijuego.resultado.estado ==
        RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        return;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

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

    const Vector3 posicionesRefugio[4] =
    {
        { -2.1f, 0.80f, -1.55f },
        {  2.1f, 0.80f, -1.55f },
        { -2.1f, 0.80f,  1.55f },
        {  2.1f, 0.80f,  1.55f }
    };

    for (int i = 0; i < 4; i++)
    {
        AgregarBloquePrueba(
            bloques,
            cantidadBloques,
            5,
            posicionesRefugio[i],
            { 1.35f, 1.60f, 1.35f },
            Color{ 82, 88, 98, 255 }
        );
    }

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
    int cantidad =
        ObtenerIndicesParticipantesActivos(
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
        { -3.2f, 0.95f,  2.8f },
        {  3.2f, 0.95f,  2.8f },
        { -3.2f, 0.95f, -2.8f },
        {  3.2f, 0.95f, -2.8f }
    };

    int cursorSpawn = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

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

    // Los rivales se mueven siempre. Solo la IA de estos dos
    // minijuegos rompe la regla global de bots inmoviles.
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
                estadosBots[i].objetivo =
                    ObtenerPuntoSeguroBotPinchos(
                        *this,
                        jugadores[i].posicion
                    );
            }

            entrada = CrearEntradaBotHaciaObjetivo1v3(
                jugadores[i].posicion,
                estadosBots[i].objetivo
            );
        }
        else
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
            bool iniciar = false;
            DireccionPinchos nuevaDireccion = direccionAviso;

            if (participantes[indiceSolo].esBot)
            {
                if (tiempoFase >= 0.45f)
                {
                    nuevaDireccion = (DireccionPinchos)
                        ElegirDireccionAtaqueBotPinchos();
                    iniciar = true;
                }
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
                        estadosBots[i].objetivo =
                            ObtenerPuntoSeguroBotPinchos(
                                *this,
                                jugadores[i].posicion
                            );
                    }
                }
            }
        }

        tiempoFase += deltaTime;
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
                LeerDireccionHumanaPinchos(
                    participantes[indiceSolo],
                    cambio
                ) &&
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
        float progreso =
            1.0f - tiempoFase / DURACION_ATAQUE_PINCHOS;

        if (!ataqueResuelto && progreso >= 0.52f)
        {
            ataqueResuelto = true;

            int vivosAntes = ContarRivalesVivosPinchos(*this);
            int eliminados = 0;

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

                if (!JugadorEstaProtegidoPinchos(*this, jugadores[i]))
                {
                    estadosJugadores[i].eliminado = true;
                    eliminados++;
                    jugadores[i].cayendo = true;
                    jugadores[i].velocidad = {};
                    jugadores[i].empuje = {};
                }
            }

            int posicion = vivosAntes - eliminados + 1;
            if (posicion < 2) posicion = 2;

            for (int i = 0; i < cantidadMaxima; i++)
            {
                if (
                    i != indiceSolo &&
                    estadosJugadores[i].eliminado &&
                    estadosJugadores[i].posicionFinal == 0
                )
                {
                    estadosJugadores[i].posicionFinal = posicion;
                }
            }

            if (ContarRivalesVivosPinchos(*this) <= 0)
            {
                FinalizarRefugioPinchos(*this, true);
                return;
            }
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


static void DibujarPistonesPinchos(
    DireccionPinchos direccion,
    float progreso
)
{
    if (progreso < 0.0f) progreso = 0.0f;
    if (progreso > 1.0f) progreso = 1.0f;

    const float carriles[4] =
    {
        -3.6f,
        -1.2f,
        1.2f,
        3.6f
    };

    for (int i = 0; i < 4; i++)
    {
        Vector3 posicion{};
        Vector3 tamano{};

        if (
            direccion == PINCHOS_DESDE_ARRIBA ||
            direccion == PINCHOS_DESDE_ABAJO
        )
        {
            float origen =
                direccion == PINCHOS_DESDE_ARRIBA
                ? -5.4f
                : 5.4f;

            float signo =
                direccion == PINCHOS_DESDE_ARRIBA
                ? 1.0f
                : -1.0f;

            posicion =
            {
                carriles[i],
                0.82f,
                origen + signo * progreso * 8.8f
            };

            tamano = { 0.72f, 0.72f, 2.8f };
        }
        else
        {
            float origen =
                direccion == PINCHOS_DESDE_IZQUIERDA
                ? -6.2f
                : 6.2f;

            float signo =
                direccion == PINCHOS_DESDE_IZQUIERDA
                ? 1.0f
                : -1.0f;

            posicion =
            {
                origen + signo * progreso * 10.2f,
                0.82f,
                carriles[i] * 0.86f
            };

            tamano = { 2.8f, 0.72f, 0.72f };
        }

        DrawCube(
            posicion,
            tamano.x,
            tamano.y,
            tamano.z,
            Color{ 75, 78, 84, 255 }
        );

        DrawCubeWires(
            posicion,
            tamano.x,
            tamano.y,
            tamano.z,
            BLACK
        );
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

    ClearBackground(Color{ 42, 33, 30, 255 });

    BeginMode3D(camara);

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

    // Consola elevada del jugador solitario.
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
        Color aviso = Fade(RED, 0.72f);

        if (direccionAviso == PINCHOS_DESDE_ARRIBA)
            DrawCube({0.0f, 0.03f, -4.25f}, 9.6f, 0.08f, 0.35f, aviso);
        else if (direccionAviso == PINCHOS_DESDE_ABAJO)
            DrawCube({0.0f, 0.03f, 4.25f}, 9.6f, 0.08f, 0.35f, aviso);
        else if (direccionAviso == PINCHOS_DESDE_IZQUIERDA)
            DrawCube({-5.05f, 0.03f, 0.0f}, 0.35f, 0.08f, 8.0f, aviso);
        else
            DrawCube({5.05f, 0.03f, 0.0f}, 0.35f, 0.08f, 8.0f, aviso);
    }
    else if (fase == FASE_PINCHOS_ATAQUE)
    {
        float progreso =
            1.0f - tiempoFase / DURACION_ATAQUE_PINCHOS;

        DibujarPistonesPinchos(direccionAviso, progreso);
    }

    EndMode3D();

    DrawText("REFUGIO DE PINCHOS - 1 VS 3", 24, 22, 30, RAYWHITE);

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
        "SOLO: elige un lado | EQUIPO: usa los bloques como cobertura",
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
        const char* aviso =
            TextFormat("PELIGRO DESDE %s", NombreDireccionPinchos(direccionAviso));

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
