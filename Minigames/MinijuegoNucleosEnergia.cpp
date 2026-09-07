#include "Minigames/MinijuegoNucleosEnergia.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_NUCLEOS = 3.0f;
static const float DURACION_PARTIDA_NUCLEOS = 35.0f;
static const float RADIO_RECOLECCION_NUCLEO = 0.82f;
static const float RETRASO_REAPARICION_NUCLEO = 0.38f;


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

        float dx =
            minijuego.nucleos[i].posicion.x -
            punto.x;

        float dz =
            minijuego.nucleos[i].posicion.z -
            punto.z;

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
    int puntoElegido =
        GetRandomValue(
            0,
            CANTIDAD_PUNTOS_NUCLEOS - 1
        );

    for (int intento = 0; intento < 24; intento++)
    {
        int candidato =
            GetRandomValue(
                0,
                CANTIDAD_PUNTOS_NUCLEOS - 1
            );

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

    NucleoEnergia& nucleo =
        minijuego.nucleos[indice];

    nucleo.posicion =
        PUNTOS_NUCLEOS[puntoElegido];

    int probabilidadEspecial =
        minijuego.tiempoRestante <= 10.0f
        ? 30
        : 16;

    nucleo.especial =
        GetRandomValue(1, 100) <=
        probabilidadEspecial;

    nucleo.activo = true;
    nucleo.tiempoReaparicion = 0.0f;
    nucleo.faseFlotacion =
        (float)GetRandomValue(0, 628) /
        100.0f;
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
        Vector3{ 0.0f, -0.45f, 0.0f },
        Vector3{ 14.0f, 0.90f, 14.0f },
        Color{ 44, 62, 83, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        Vector3{ -7.15f, 0.55f, 0.0f },
        Vector3{ 0.30f, 2.0f, 14.6f },
        Color{ 45, 125, 154, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        Vector3{ 7.15f, 0.55f, 0.0f },
        Vector3{ 0.30f, 2.0f, 14.6f },
        Color{ 45, 125, 154, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        Vector3{ 0.0f, 0.55f, -7.15f },
        Vector3{ 14.0f, 2.0f, 0.30f },
        Color{ 45, 125, 154, 255 }
    );

    AgregarBloquePrueba(
        minijuego.bloques,
        minijuego.cantidadBloques,
        MAX_BLOQUES_NUCLEOS,
        Vector3{ 0.0f, 0.55f, 7.15f },
        Vector3{ 14.0f, 2.0f, 0.30f },
        Color{ 45, 125, 154, 255 }
    );
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
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        if (minijuego.puntuaciones[i] > mejorPuntaje)
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

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

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
                minijuego.puntuaciones[j] >
                    minijuego.puntuaciones[i]
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            minijuego.puntuaciones[i];
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase =
        FASE_NUCLEOS_TERMINADO;

    if (audio != nullptr)
    {
        audio->ReproducirSonido(
            SONIDO_RESULTADO
        );
    }
}


void MinijuegoNucleosEnergia::Inicializar()
{
    resultado = {};
    resultado.formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    fase = FASE_NUCLEOS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_NUCLEOS;
    tiempoRestante = DURACION_PARTIDA_NUCLEOS;
    tiempoAnimacion = 0.0f;
    ultimoNumeroCuenta = -1;
    ultimoNumeroAlerta = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        puntuaciones[i] = 0;
    }

    ConfigurarArenaNucleos(*this);

    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        nucleos[i] = {};
        ReubicarNucleo(*this, i);
    }

    camara.position =
        { 0.0f, 12.0f, 13.8f };

    camara.target =
        { 0.0f, 0.45f, 0.0f };

    camara.up =
        { 0.0f, 1.0f, 0.0f };

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
        { 4.8f, 1.0f, 4.8f },
        { -4.8f, 1.0f, -4.8f },
        { 4.8f, 1.0f, -4.8f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            spawns[i]
        );
    }
}


void MinijuegoNucleosEnergia::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    Inicializar();

    ConfigurarJugadores(
        jugadores,
        cantidadMaxima
    );
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
        int numeroCuenta =
            (int)std::ceil(
                tiempoPreparacion
            );

        if (
            numeroCuenta > 0 &&
            numeroCuenta != ultimoNumeroCuenta
        )
        {
            ultimoNumeroCuenta = numeroCuenta;

            if (audio != nullptr)
            {
                audio->ReproducirSonido(
                    SONIDO_CUENTA_REGRESIVA
                );
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
                audio->ReproducirSonido(
                    SONIDO_INICIO_MINIJUEGO
                );
            }
        }

        return;
    }

    tiempoRestante -= deltaTime;

    if (tiempoRestante < 0.0f)
    {
        tiempoRestante = 0.0f;
    }

    int segundoAlerta =
        (int)std::ceil(
            tiempoRestante
        );

    if (
        segundoAlerta > 0 &&
        segundoAlerta <= 5 &&
        segundoAlerta != ultimoNumeroAlerta
    )
    {
        ultimoNumeroAlerta = segundoAlerta;

        if (audio != nullptr)
        {
            audio->ReproducirSonido(
                SONIDO_ALERTA_TIEMPO
            );
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

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(
                participantes[i]
            );

        entrada.golpear = false;

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

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantes,
        limite
    );

    for (int n = 0; n < MAX_NUCLEOS_ENERGIA; n++)
    {
        NucleoEnergia& nucleo =
            nucleos[n];

        if (!nucleo.activo)
        {
            nucleo.tiempoReaparicion -=
                deltaTime;

            if (nucleo.tiempoReaparicion <= 0.0f)
            {
                ReubicarNucleo(
                    *this,
                    n
                );
            }

            continue;
        }

        for (int i = 0; i < limite; i++)
        {
            if (
                !participantes[i].activo ||
                participantes[i].esBot ||
                jugadores[i].cayendo
            )
            {
                continue;
            }

            float dx =
                jugadores[i].posicion.x -
                nucleo.posicion.x;

            float dz =
                jugadores[i].posicion.z -
                nucleo.posicion.z;

            float distanciaCuadrada =
                dx * dx + dz * dz;

            float diferenciaY =
                std::fabs(
                    jugadores[i].posicion.y -
                    nucleo.posicion.y
                );

            if (
                distanciaCuadrada >
                    RADIO_RECOLECCION_NUCLEO *
                    RADIO_RECOLECCION_NUCLEO ||
                diferenciaY > 1.25f
            )
            {
                continue;
            }

            int valor =
                nucleo.especial
                ? 3
                : 1;

            puntuaciones[i] += valor;

            if (audio != nullptr)
            {
                audio->ReproducirSonido(
                    nucleo.especial
                    ? SONIDO_RECOGER_NUCLEO_ESPECIAL
                    : SONIDO_RECOGER_NUCLEO
                );
            }

            CrearParticulasImpactoGolpe(
                particulas,
                cantidadParticulas,
                nucleo.posicion
            );

            nucleo.activo = false;
            nucleo.tiempoReaparicion =
                RETRASO_REAPARICION_NUCLEO;

            break;
        }
    }

    if (tiempoRestante <= 0.0f)
    {
        FinalizarNucleos(
            *this,
            audio
        );
    }
}


void MinijuegoNucleosEnergia::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    ClearBackground(
        Color{ 18, 29, 48, 255 }
    );

    BeginMode3D(camara);

    for (int i = 0; i < cantidadBloques; i++)
    {
        const BloquePrueba& bloque =
            bloques[i];

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
            DrawBoundingBox(
                CrearHitboxBloquePrueba(bloque),
                YELLOW
            );
        }
    }

    for (int i = 0; i < MAX_NUCLEOS_ENERGIA; i++)
    {
        const NucleoEnergia& nucleo =
            nucleos[i];

        if (!nucleo.activo)
        {
            continue;
        }

        float oscilacion =
            std::sin(
                tiempoAnimacion * 3.1f +
                nucleo.faseFlotacion
            ) *
            0.16f;

        Vector3 posicionVisual =
            nucleo.posicion;

        posicionVisual.y += oscilacion;

        float pulso =
            1.0f +
            std::sin(
                tiempoAnimacion * 5.0f +
                nucleo.faseFlotacion
            ) *
            0.08f;

        float radio =
            (nucleo.especial ? 0.48f : 0.34f) *
            pulso;

        Color color =
            nucleo.especial
            ? GOLD
            : SKYBLUE;

        DrawSphere(
            posicionVisual,
            radio,
            color
        );

        DrawSphereWires(
            posicionVisual,
            radio + 0.08f,
            8,
            12,
            nucleo.especial
            ? ORANGE
            : RAYWHITE
        );

        DrawCube(
            Vector3{
                posicionVisual.x,
                0.08f,
                posicionVisual.z
            },
            nucleo.especial ? 0.72f : 0.54f,
            0.03f,
            nucleo.especial ? 0.72f : 0.54f,
            Fade(color, 0.45f)
        );
    }

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        DibujarJugadorCuboPrueba(
            jugadores[i],
            participantes[i]
        );

        if (
            mostrarDebug &&
            participantes[i].activo &&
            !jugadores[i].cayendo
        )
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(
                    jugadores[i]
                ),
                LIME
            );
        }
    }

    DrawGrid(14, 1.0f);
    EndMode3D();

    DrawText(
        "NUCLEOS DE ENERGIA",
        24,
        22,
        31,
        RAYWHITE
    );

    DrawText(
        "AZUL +1   DORADO +3",
        25,
        60,
        19,
        LIGHTGRAY
    );

    DrawText(
        TextFormat(
            "TIEMPO: %02d",
            (int)std::ceil(tiempoRestante)
        ),
        GetScreenWidth() - 190,
        26,
        25,
        tiempoRestante <= 5.0f
        ? ORANGE
        : RAYWHITE
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        int x =
            25 + i * 155;

        Color colorJugador =
            participantes[i].color;

        DrawRectangle(
            x,
            96,
            140,
            48,
            Fade(BLACK, 0.58f)
        );

        DrawRectangleLines(
            x,
            96,
            140,
            48,
            colorJugador
        );

        DrawText(
            TextFormat(
                "J%d  %d",
                i + 1,
                puntuaciones[i]
            ),
            x + 12,
            108,
            22,
            colorJugador
        );
    }

    if (fase == FASE_NUCLEOS_PREPARACION)
    {
        int numero =
            (int)std::ceil(
                tiempoPreparacion
            );

        const char* texto =
            numero > 0
            ? TextFormat("%d", numero)
            : "YA";

        int tamano = 72;

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, tamano) / 2,
            GetScreenHeight() / 2 - 60,
            tamano,
            GOLD
        );
    }

    if (fase == FASE_NUCLEOS_TERMINADO)
    {
        DrawRectangle(
            0,
            0,
            GetScreenWidth(),
            GetScreenHeight(),
            Fade(BLACK, 0.62f)
        );

        const char* titulo =
            resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : "RESULTADO";

        DrawText(
            titulo,
            GetScreenWidth() / 2 -
                MeasureText(titulo, 48) / 2,
            190,
            48,
            GOLD
        );

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores =
            ObtenerIndicesGanadores(
                resultado,
                ganadores,
                MAX_PARTICIPANTES
            );

        if (cantidadGanadores == 1)
        {
            const char* ganador =
                TextFormat(
                    "GANA J%d CON %d PUNTOS",
                    ganadores[0] + 1,
                    puntuaciones[ganadores[0]]
                );

            DrawText(
                ganador,
                GetScreenWidth() / 2 -
                    MeasureText(ganador, 30) / 2,
                270,
                30,
                participantes[ganadores[0]].color
            );
        }
        else
        {
            const char* empate =
                TextFormat(
                    "%d JUGADORES COMPARTEN EL PRIMER PUESTO",
                    cantidadGanadores
                );

            DrawText(
                empate,
                GetScreenWidth() / 2 -
                    MeasureText(empate, 26) / 2,
                270,
                26,
                RAYWHITE
            );
        }

        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 -
                MeasureText("R PARA REINICIAR", 22) / 2,
            340,
            22,
            LIGHTGRAY
        );
    }

    DrawText(
        "MOVER + SALTAR | TOCA LOS NUCLEOS ANTES QUE LOS DEMAS",
        25,
        GetScreenHeight() - 92,
        18,
        LIGHTGRAY
    );
}
