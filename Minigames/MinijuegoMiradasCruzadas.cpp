#include "Minigames/MinijuegoMiradasCruzadas.h"

#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_MIRADAS = 2.5f;
static const float DURACION_ELECCION_MIRADAS = 1.65f;
static const float DURACION_RESOLUCION_MIRADAS = 1.15f;
static const int MAX_RONDAS_MIRADAS = 5;


static const char* NombreDireccionMirada(
    DireccionMiradaCruzada direccion
)
{
    switch (direccion)
    {
        case MIRADA_FRENTE: return "FRENTE";
        case MIRADA_IZQUIERDA: return "IZQUIERDA";
        case MIRADA_DERECHA: return "DERECHA";
        case MIRADA_ARRIBA: return "ARRIBA";
        case MIRADA_ABAJO: return "ABAJO";
        case CANTIDAD_DIRECCIONES_MIRADA: break;
    }

    return "?";
}


static int ContarEquipoVivoMiradas(
    const MinijuegoMiradasCruzadas& minijuego
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static bool LeerDireccionMiradaHumana(
    const Participante& participante,
    DireccionMiradaCruzada& direccion
)
{
    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(participante);

    if (entrada.izquierda)
    {
        direccion = MIRADA_IZQUIERDA;
        return true;
    }

    if (entrada.derecha)
    {
        direccion = MIRADA_DERECHA;
        return true;
    }

    if (entrada.arriba)
    {
        direccion = MIRADA_ARRIBA;
        return true;
    }

    if (entrada.abajo)
    {
        direccion = MIRADA_ABAJO;
        return true;
    }

    return false;
}


static void PrepararRondaMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    const Participante participantes[]
)
{
    minijuego.numeroRonda++;
    minijuego.fase = FASE_MIRADAS_ELECCION;
    minijuego.tiempoFase = DURACION_ELECCION_MIRADAS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        minijuego.jugadores[i].direccion = MIRADA_FRENTE;
        minijuego.jugadores[i].eligioDireccion = false;

        if (
            participantes[i].esBot &&
            !minijuego.jugadores[i].eliminado
        )
        {
            ReiniciarBotMiradas(
                minijuego.bots[i],
                0.32f,
                1.05f
            );
        }
    }
}


static void FinalizarMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    bool ganaSolo
)
{
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
            ? 3 - ContarEquipoVivoMiradas(minijuego)
            : (!minijuego.jugadores[i].eliminado ? 1 : 0);
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_MIRADAS_TERMINADO;
}


static void ResolverRondaMiradas(
    MinijuegoMiradasCruzadas& minijuego
)
{
    DireccionMiradaCruzada direccionSolo =
        minijuego.jugadores[minijuego.indiceSolo].direccion;

    int vivosAntes = ContarEquipoVivoMiradas(minijuego);
    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            minijuego.jugadores[i].eliminado
        )
        {
            continue;
        }

        if (minijuego.jugadores[i].direccion == direccionSolo)
        {
            minijuego.jugadores[i].eliminado = true;
            eliminados++;
        }
    }

    int posicion = vivosAntes - eliminados + 1;
    if (posicion < 2) posicion = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.jugadores[i].eliminado &&
            minijuego.jugadores[i].posicionFinal == 0
        )
        {
            minijuego.jugadores[i].posicionFinal = posicion;
        }
    }

    if (ContarEquipoVivoMiradas(minijuego) <= 0)
    {
        FinalizarMiradas(minijuego, true);
        return;
    }

    if (minijuego.numeroRonda >= MAX_RONDAS_MIRADAS)
    {
        FinalizarMiradas(minijuego, false);
        return;
    }

    minijuego.fase = FASE_MIRADAS_RESOLUCION;
    minijuego.tiempoFase = DURACION_RESOLUCION_MIRADAS;
}


void MinijuegoMiradasCruzadas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
        bots[i] = {};
    }

    fase = FASE_MIRADAS_PREPARACION;
    indiceSolo = -1;
    numeroRonda = 0;
    tiempoPreparacion = DURACION_PREPARACION_MIRADAS;
    tiempoFase = 0.0f;
}


void MinijuegoMiradasCruzadas::Reiniciar(
    Participante participantes[]
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
        fase = FASE_MIRADAS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        resultado.participantes[i].numeroEquipo =
            i == indiceSolo ? 0 : 1;
    }
}


void MinijuegoMiradasCruzadas::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    if (
        fase == FASE_MIRADAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_MIRADAS_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            PrepararRondaMiradas(*this, participantes);
        }

        return;
    }

    if (fase == FASE_MIRADAS_ELECCION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                !resultado.participantes[i].participo ||
                (i != indiceSolo && jugadores[i].eliminado)
            )
            {
                continue;
            }

            if (participantes[i].esBot)
            {
                if (
                    ActualizarDecisionBotMiradas(
                        bots[i],
                        deltaTime,
                        CANTIDAD_DIRECCIONES_MIRADA
                    )
                )
                {
                    jugadores[i].direccion =
                        (DireccionMiradaCruzada)bots[i].direccionElegida;
                    jugadores[i].eligioDireccion = true;
                }

                continue;
            }

            if (!participantes[i].conectado)
            {
                continue;
            }

            DireccionMiradaCruzada nueva = jugadores[i].direccion;

            if (LeerDireccionMiradaHumana(participantes[i], nueva))
            {
                jugadores[i].direccion = nueva;
                jugadores[i].eligioDireccion = true;
            }
        }

        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            ResolverRondaMiradas(*this);
        }

        return;
    }

    if (fase == FASE_MIRADAS_RESOLUCION)
    {
        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            PrepararRondaMiradas(*this, participantes);
        }
    }
}


static Vector2 DesplazamientoCaraMiradas(
    DireccionMiradaCruzada direccion,
    float magnitud
)
{
    switch (direccion)
    {
        case MIRADA_IZQUIERDA: return { -magnitud, 0.0f };
        case MIRADA_DERECHA: return { magnitud, 0.0f };
        case MIRADA_ARRIBA: return { 0.0f, -magnitud };
        case MIRADA_ABAJO: return { 0.0f, magnitud };
        case MIRADA_FRENTE:
        case CANTIDAD_DIRECCIONES_MIRADA:
            return { 0.0f, 0.0f };
    }

    return {};
}


static void DibujarCabezaMiradas(
    Vector2 centro,
    float radio,
    Color color,
    DireccionMiradaCruzada direccion,
    bool mostrarDireccion,
    bool eliminado,
    const char* etiqueta
)
{
    float escala = eliminado ? 0.58f : 1.0f;
    radio *= escala;

    DrawCircleV(centro, radio + 7.0f, Fade(BLACK, 0.32f));
    DrawCircleV(centro, radio, eliminado ? Fade(color, 0.42f) : color);
    DrawCircleLines((int)centro.x, (int)centro.y, radio, RAYWHITE);

    DireccionMiradaCruzada visual =
        mostrarDireccion ? direccion : MIRADA_FRENTE;

    Vector2 desplazamiento =
        DesplazamientoCaraMiradas(visual, radio * 0.22f);

    Vector2 ojoIzquierdo =
    {
        centro.x - radio * 0.22f + desplazamiento.x,
        centro.y - radio * 0.14f + desplazamiento.y
    };

    Vector2 ojoDerecho =
    {
        centro.x + radio * 0.22f + desplazamiento.x,
        centro.y - radio * 0.14f + desplazamiento.y
    };

    DrawCircleV(ojoIzquierdo, radio * 0.085f, BLACK);
    DrawCircleV(ojoDerecho, radio * 0.085f, BLACK);

    Vector2 nariz =
    {
        centro.x + desplazamiento.x * 1.45f,
        centro.y + radio * 0.12f + desplazamiento.y * 1.25f
    };

    DrawCircleV(nariz, radio * 0.075f, DARKGRAY);

    if (eliminado)
    {
        DrawText(
            "X",
            (int)(centro.x - MeasureText("X", 34) / 2),
            (int)(centro.y - radio - 42.0f),
            34,
            RED
        );
    }

    DrawText(
        etiqueta,
        (int)(centro.x - MeasureText(etiqueta, 18) / 2),
        (int)(centro.y + radio + 14.0f),
        18,
        RAYWHITE
    );
}


void MinijuegoMiradasCruzadas::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 102, 154, 219, 255 });

    // Fondo original con primitivas: cielo, nubes y arco de color.
    DrawRectangle(0, GetScreenHeight() / 2, GetScreenWidth(), GetScreenHeight() / 2, Color{ 116, 187, 111, 255 });

    for (int i = 0; i < 7; i++)
    {
        Color colores[7] = { RED, ORANGE, YELLOW, GREEN, SKYBLUE, BLUE, PURPLE };
        DrawCircleLines(
            GetScreenWidth() / 2,
            GetScreenHeight() + 150,
            470.0f - i * 8.0f,
            Fade(colores[i], 0.80f)
        );
    }

    DrawText(
        "MIRADAS CRUZADAS - 1 VS 3",
        24,
        20,
        30,
        RAYWHITE
    );

    DrawText(
        "El equipo debe mirar a un lugar distinto del jugador solitario.",
        24,
        58,
        18,
        RAYWHITE
    );

    DrawText(
        "Direcciones: izquierda / derecha / arriba / abajo. Sin tocar nada = frente.",
        24,
        84,
        17,
        LIGHTGRAY
    );

    if (indiceSolo < 0)
    {
        return;
    }

    bool revelar =
        fase == FASE_MIRADAS_RESOLUCION ||
        fase == FASE_MIRADAS_TERMINADO;

    char etiquetaSolo[40]{};
    TextCopy(
        etiquetaSolo,
        TextFormat(
            "J%d SOLO%s",
            participantes[indiceSolo].numeroJugador,
            participantes[indiceSolo].esBot ? " BOT" : ""
        )
    );

    DibujarCabezaMiradas(
        { GetScreenWidth() / 2.0f, 235.0f },
        70.0f,
        participantes[indiceSolo].color,
        jugadores[indiceSolo].direccion,
        revelar,
        false,
        etiquetaSolo
    );

    int posicionesEquipo[3] =
    {
        GetScreenWidth() / 2 - 250,
        GetScreenWidth() / 2,
        GetScreenWidth() / 2 + 250
    };

    int cursor = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == indiceSolo ||
            !resultado.participantes[i].participo ||
            cursor >= 3
        )
        {
            continue;
        }

        char etiqueta[40]{};
        TextCopy(
            etiqueta,
            TextFormat(
                "J%d%s",
                participantes[i].numeroJugador,
                participantes[i].esBot ? " BOT" : ""
            )
        );

        DibujarCabezaMiradas(
            { (float)posicionesEquipo[cursor], 475.0f },
            61.0f,
            participantes[i].color,
            jugadores[i].direccion,
            revelar,
            jugadores[i].eliminado,
            etiqueta
        );

        cursor++;
    }

    if (fase == FASE_MIRADAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 76) / 2,
            310,
            76,
            GOLD
        );
    }
    else if (fase == FASE_MIRADAS_ELECCION)
    {
        DrawText(
            TextFormat("RONDA %d / %d - ELIGE", numeroRonda, MAX_RONDAS_MIRADAS),
            GetScreenWidth() / 2 - MeasureText(TextFormat("RONDA %d / %d - ELIGE", numeroRonda, MAX_RONDAS_MIRADAS), 28) / 2,
            335,
            28,
            GOLD
        );

        DrawRectangle(
            GetScreenWidth() / 2 - 170,
            374,
            340,
            14,
            Fade(BLACK, 0.32f)
        );

        float porcentaje = tiempoFase / DURACION_ELECCION_MIRADAS;
        if (porcentaje < 0.0f) porcentaje = 0.0f;

        DrawRectangle(
            GetScreenWidth() / 2 - 170,
            374,
            (int)(340.0f * porcentaje),
            14,
            porcentaje < 0.25f ? RED : YELLOW
        );
    }
    else if (fase == FASE_MIRADAS_RESOLUCION)
    {
        const char* texto =
            TextFormat(
                "SOLO MIRO: %s",
                NombreDireccionMirada(jugadores[indiceSolo].direccion)
            );

        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 28) / 2,
            335,
            28,
            GOLD
        );
    }
    else if (fase == FASE_MIRADAS_TERMINADO)
    {
        bool ganaSolo = ContarEquipoVivoMiradas(*this) <= 0;
        const char* titulo = ganaSolo
            ? "GANA EL JUGADOR SOLITARIO"
            : "GANA EL EQUIPO";

        DrawRectangle(
            GetScreenWidth() / 2 - 300,
            300,
            600,
            120,
            Fade(BLACK, 0.88f)
        );

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 32) / 2,
            320,
            32,
            GOLD
        );

        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 19) / 2,
            372,
            19,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoMiradasCruzadas::ObtenerResultado() const
{
    return resultado;
}
