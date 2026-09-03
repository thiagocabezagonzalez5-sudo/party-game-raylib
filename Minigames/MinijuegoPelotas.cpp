#include "Minigames/MinijuegoPelotas.h"

#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_PELOTAS = 3.0f;
static const float DURACION_PARTIDA_PELOTAS = 60.0f;
static const float DURACION_TEXTO_YA_PELOTAS = 0.75f;

// Fisica propia de este minijuego.
// La velocidad que consigue el jugador por movimiento queda limitada,
// pero un choque puede lanzarlo temporalmente por encima de ese valor.
static const float VELOCIDAD_MAXIMA_PELOTAS = 9.0f;
static const float ACELERACION_MAXIMA_PELOTAS = 6.0f;
static const float MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS = 1.65f;
static const float VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS = 15.0f;

// Arena circular. El bloque cuadrado sigue existiendo como soporte
// vertical interno, pero cada jugador decide si ese suelo esta activo
// segun su posicion respecto del circulo.
static const float RADIO_ARENA_PELOTAS = 5.25f;
static const int SEGMENTOS_ARENA_PELOTAS = 48;
static const float ALTURA_BASE_MONTANA_PELOTAS = -2.10f;
static const float CRECIMIENTO_RADIO_MONTANA_PELOTAS = 1.15f;


static float MagnitudHorizontalPelotas(
    float x,
    float z
)
{
    return sqrtf(x * x + z * z);
}


static void LimitarMovimientoPelota(
    JugadorPrueba& jugador,
    float velocidadAnteriorX,
    float velocidadAnteriorZ,
    float deltaTime
)
{
    float velocidadAnterior =
        MagnitudHorizontalPelotas(
            velocidadAnteriorX,
            velocidadAnteriorZ
        );

    float velocidadActual =
        MagnitudHorizontalPelotas(
            jugador.velocidad.x,
            jugador.velocidad.z
        );

    if (velocidadActual <= 0.001f)
    {
        return;
    }

    float limitePorAceleracion =
        velocidadAnterior +
        ACELERACION_MAXIMA_PELOTAS * deltaTime;

    float limiteFinal =
        limitePorAceleracion < VELOCIDAD_MAXIMA_PELOTAS
        ? limitePorAceleracion
        : VELOCIDAD_MAXIMA_PELOTAS;

    if (velocidadActual > limiteFinal)
    {
        float factor =
            limiteFinal /
            velocidadActual;

        jugador.velocidad.x *= factor;
        jugador.velocidad.z *= factor;
    }
}


static void AplicarEmpujePendientePelotas(
    JugadorPrueba& jugador,
    Vector3 empujePendiente
)
{
    jugador.velocidad.x +=
        empujePendiente.x;

    jugador.velocidad.z +=
        empujePendiente.z;

    float velocidad =
        MagnitudHorizontalPelotas(
            jugador.velocidad.x,
            jugador.velocidad.z
        );

    if (
        velocidad >
        VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS
    )
    {
        float factor =
            VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS /
            velocidad;

        jugador.velocidad.x *= factor;
        jugador.velocidad.z *= factor;
    }
}


static bool JugadorSobreArenaCircularPelotas(
    const JugadorPrueba& jugador
)
{
    float distancia =
        MagnitudHorizontalPelotas(
            jugador.posicion.x,
            jugador.posicion.z
        );

    float margenJugador =
        jugador.tamano.x * 0.18f;

    return
        distancia <=
        RADIO_ARENA_PELOTAS - margenJugador;
}


static void ResolverColisionMontanaPelotas(
    JugadorPrueba& jugador,
    Vector3 posicionAnterior
)
{
    float radioPelota =
        jugador.tamano.x / 2.0f;

    float distancia =
        MagnitudHorizontalPelotas(
            jugador.posicion.x,
            jugador.posicion.z
        );

    // Colision lateral simplificada del monticulo. No intenta seguir
    // cada triangulo visual: crea un volumen conico suficientemente
    // parecido para que una pelota que cae por el borde no atraviese
    // la nieve y aparezca dentro de la montana.
    bool tocaAlturaMontana =
        jugador.posicion.y - radioPelota < 0.0f &&
        jugador.posicion.y + radioPelota >
            ALTURA_BASE_MONTANA_PELOTAS;

    if (tocaAlturaMontana)
    {
        float progresoAltura =
            -jugador.posicion.y /
            -ALTURA_BASE_MONTANA_PELOTAS;

        if (progresoAltura < 0.0f)
        {
            progresoAltura = 0.0f;
        }

        if (progresoAltura > 1.0f)
        {
            progresoAltura = 1.0f;
        }

        float radioMontana =
            RADIO_ARENA_PELOTAS +
            CRECIMIENTO_RADIO_MONTANA_PELOTAS *
                progresoAltura;

        float distanciaMinima =
            radioMontana +
            radioPelota * 0.82f;

        float zonaCercanaAlBorde =
            RADIO_ARENA_PELOTAS -
            radioPelota * 0.45f;

        if (
            distancia > zonaCercanaAlBorde &&
            distancia < distanciaMinima
        )
        {
            float normalX = 1.0f;
            float normalZ = 0.0f;

            if (distancia > 0.001f)
            {
                normalX =
                    jugador.posicion.x /
                    distancia;

                normalZ =
                    jugador.posicion.z /
                    distancia;
            }
            else
            {
                float distanciaAnterior =
                    MagnitudHorizontalPelotas(
                        posicionAnterior.x,
                        posicionAnterior.z
                    );

                if (distanciaAnterior > 0.001f)
                {
                    normalX =
                        posicionAnterior.x /
                        distanciaAnterior;

                    normalZ =
                        posicionAnterior.z /
                        distanciaAnterior;
                }
            }

            jugador.posicion.x =
                normalX * distanciaMinima;

            jugador.posicion.z =
                normalZ * distanciaMinima;

            float velocidadHaciaCentro =
                jugador.velocidad.x * normalX +
                jugador.velocidad.z * normalZ;

            if (velocidadHaciaCentro < 0.0f)
            {
                jugador.velocidad.x -=
                    normalX * velocidadHaciaCentro;

                jugador.velocidad.z -=
                    normalZ * velocidadHaciaCentro;
            }
        }
    }

    // Seguridad inferior: si por algun impulso futuro una pelota que
    // ya esta debajo intenta subir atravesando la base, la detenemos.
    float radioBase =
        RADIO_ARENA_PELOTAS +
        CRECIMIENTO_RADIO_MONTANA_PELOTAS;

    float parteSuperiorAnterior =
        posicionAnterior.y + radioPelota;

    float parteSuperiorActual =
        jugador.posicion.y + radioPelota;

    if (
        posicionAnterior.y < ALTURA_BASE_MONTANA_PELOTAS &&
        jugador.velocidad.y > 0.0f &&
        parteSuperiorAnterior <= ALTURA_BASE_MONTANA_PELOTAS &&
        parteSuperiorActual >= ALTURA_BASE_MONTANA_PELOTAS &&
        distancia <= radioBase + radioPelota
    )
    {
        jugador.posicion.y =
            ALTURA_BASE_MONTANA_PELOTAS -
            radioPelota -
            0.001f;

        jugador.velocidad.y = 0.0f;
    }
}


static void PotenciarEmpujesPelotas(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            jugadores[i].cayendo
        )
        {
            continue;
        }

        jugadores[i].empuje.x *=
            MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS;

        jugadores[i].empuje.z *=
            MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS;
    }
}


//==================================================
// ARENA DE NIEVE
//==================================================

static float IrregularidadBordePelotas(
    int indice
)
{
    float angulo =
        (2.0f * PI * (float)indice) /
        (float)SEGMENTOS_ARENA_PELOTAS;

    return
        1.0f +
        0.018f * sinf(angulo * 5.0f) +
        0.012f * cosf(angulo * 9.0f);
}


static Vector3 PuntoCircularPelotas(
    int indice,
    float radio,
    float altura
)
{
    float angulo =
        (2.0f * PI * (float)indice) /
        (float)SEGMENTOS_ARENA_PELOTAS;

    float irregularidad =
        IrregularidadBordePelotas(indice);

    return Vector3
    {
        cosf(angulo) * radio * irregularidad,
        altura,
        sinf(angulo) * radio * irregularidad
    };
}


static void DibujarMontanaNievePelotas(
    bool mostrarDebug
)
{
    const Color nieveSuperior =
        Color{ 246, 250, 252, 255 };

    const Color nieveSombra =
        Color{ 207, 225, 235, 255 };

    const Color hieloClaro =
        Color{ 153, 190, 208, 255 };

    const Color hieloOscuro =
        Color{ 93, 132, 153, 255 };

    Vector3 centroSuperior =
        { 0.0f, 0.015f, 0.0f };

    for (
        int i = 0;
        i < SEGMENTOS_ARENA_PELOTAS;
        i++
    )
    {
        int siguiente =
            (i + 1) %
            SEGMENTOS_ARENA_PELOTAS;

        Vector3 cimaA =
            PuntoCircularPelotas(
                i,
                RADIO_ARENA_PELOTAS,
                0.0f
            );

        Vector3 cimaB =
            PuntoCircularPelotas(
                siguiente,
                RADIO_ARENA_PELOTAS,
                0.0f
            );

        Vector3 nieveA =
            PuntoCircularPelotas(
                i,
                RADIO_ARENA_PELOTAS + 0.50f,
                -0.55f
            );

        Vector3 nieveB =
            PuntoCircularPelotas(
                siguiente,
                RADIO_ARENA_PELOTAS + 0.50f,
                -0.55f
            );

        Vector3 baseA =
            PuntoCircularPelotas(
                i,
                RADIO_ARENA_PELOTAS + 1.15f,
                ALTURA_BASE_MONTANA_PELOTAS
            );

        Vector3 baseB =
            PuntoCircularPelotas(
                siguiente,
                RADIO_ARENA_PELOTAS + 1.15f,
                ALTURA_BASE_MONTANA_PELOTAS
            );

        DrawTriangle3D(
            centroSuperior,
            cimaB,
            cimaA,
            nieveSuperior
        );

        DrawTriangle3D(
            cimaA,
            cimaB,
            nieveB,
            i % 2 == 0
            ? nieveSuperior
            : nieveSombra
        );

        DrawTriangle3D(
            cimaA,
            nieveB,
            nieveA,
            i % 2 == 0
            ? nieveSuperior
            : nieveSombra
        );

        DrawTriangle3D(
            nieveA,
            nieveB,
            baseB,
            i % 2 == 0
            ? hieloClaro
            : hieloOscuro
        );

        DrawTriangle3D(
            nieveA,
            baseB,
            baseA,
            i % 2 == 0
            ? hieloClaro
            : hieloOscuro
        );

        DrawLine3D(
            cimaA,
            cimaB,
            Fade(SKYBLUE, 0.45f)
        );
    }

    for (int i = 0; i < 12; i++)
    {
        float angulo =
            (2.0f * PI * (float)i) /
            12.0f;

        float radio =
            RADIO_ARENA_PELOTAS + 0.15f;

        Vector3 posicion =
        {
            cosf(angulo) * radio,
            -0.28f,
            sinf(angulo) * radio
        };

        DrawSphere(
            posicion,
            0.38f + 0.08f * (float)(i % 3),
            Fade(RAYWHITE, 0.96f)
        );
    }

    if (mostrarDebug)
    {
        for (
            int i = 0;
            i < SEGMENTOS_ARENA_PELOTAS;
            i++
        )
        {
            int siguiente =
                (i + 1) %
                SEGMENTOS_ARENA_PELOTAS;

            float anguloA =
                (2.0f * PI * (float)i) /
                (float)SEGMENTOS_ARENA_PELOTAS;

            float anguloB =
                (2.0f * PI * (float)siguiente) /
                (float)SEGMENTOS_ARENA_PELOTAS;

            DrawLine3D(
                Vector3
                {
                    cosf(anguloA) * RADIO_ARENA_PELOTAS,
                    0.07f,
                    sinf(anguloA) * RADIO_ARENA_PELOTAS
                },
                Vector3
                {
                    cosf(anguloB) * RADIO_ARENA_PELOTAS,
                    0.07f,
                    sinf(anguloB) * RADIO_ARENA_PELOTAS
                },
                RED
            );
        }
    }
}


//==================================================
// RESULTADO
//==================================================

static void InicializarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarJugadoresVivosPelotas(
    const MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void FinalizarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int cantidadVivos =
        ContarJugadoresVivosPelotas(
            minijuego,
            participantes
        );

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        cantidadVivos == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(
            minijuego.tiempoJugado *
            1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !minijuego.resultado
                .participantes[i]
                .participo
        )
        {
            continue;
        }

        EstadoJugadorPelotas& estadoJugador =
            minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs =
                tiempoFinalMs;
        }

        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        resultadoJugador.posicionFinal =
            estadoJugador.posicionFinal;

        resultadoJugador.numeroEquipo = -1;

        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;

        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase =
        FASE_PELOTAS_TERMINADO;
}


//==================================================
// INICIALIZAR
//==================================================

void MinijuegoPelotas::Inicializar()
{
    resultado = {};
    resultado.formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    fase =
        FASE_PELOTAS_PREPARACION;

    tiempoPreparacion =
        DURACION_PREPARACION_PELOTAS;

    tiempoRestante =
        DURACION_PARTIDA_PELOTAS;

    tiempoJugado =
        0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadBloques =
        0;

    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        1,
        Vector3
        {
            0.0f,
            -0.5f,
            0.0f
        },
        Vector3
        {
            13.0f,
            1.0f,
            13.0f
        },
        RAYWHITE
    );

    camara.position =
    {
        0.0f,
        9.0f,
        14.2f
    };

    camara.target =
    {
        0.0f,
        -0.15f,
        0.0f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy =
        52.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;
}


void MinijuegoPelotas::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.2f, 0.65f, 2.2f },
        { 2.2f, 0.65f, 2.2f },
        { -2.2f, 0.65f, -2.2f },
        { 2.2f, 0.65f, -2.2f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        jugadores[i].posicionSpawn =
            spawns[i];

        jugadores[i].tamano =
        {
            1.30f,
            1.30f,
            1.30f
        };

        jugadores[i].velocidadMovimiento =
            VELOCIDAD_MAXIMA_PELOTAS;

        jugadores[i].fuerzaSalto =
            0.0f;

        jugadores[i].gravedad =
            18.0f;

        jugadores[i].duracionRespawn =
            1.2f;

        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


void MinijuegoPelotas::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participantesAnteriores[
        MAX_PARTICIPANTES
    ]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantesAnteriores[i] =
            resultado.participantes[i].participo;
    }

    resultado = {};
    resultado.formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo =
            participantesAnteriores[i];

        if (participantesAnteriores[i])
        {
            resultado.cantidadParticipantes++;
        }

        estadosJugadores[i] = {};
    }

    fase =
        FASE_PELOTAS_PREPARACION;

    tiempoPreparacion =
        DURACION_PREPARACION_PELOTAS;

    tiempoRestante =
        DURACION_PARTIDA_PELOTAS;

    tiempoJugado =
        0.0f;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoPelotas::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoPelotas(
            *this,
            participantes
        );
    }

    if (fase == FASE_PELOTAS_TERMINADO)
    {
        return;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -=
            deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PELOTAS_JUGANDO;
        }

        return;
    }

    tiempoRestante -=
        deltaTime;

    if (tiempoRestante < 0.0f)
    {
        tiempoRestante = 0.0f;
    }

    tiempoJugado =
        DURACION_PARTIDA_PELOTAS -
        tiempoRestante;

    int vivosAntes =
        ContarJugadoresVivosPelotas(
            *this,
            participantes
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador =
            jugadores[i];

        if (estadosJugadores[i].eliminado)
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            continue;
        }

        if (
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(
                participantes[i]
            );

        float velocidadAnteriorX =
            jugador.velocidad.x;

        float velocidadAnteriorZ =
            jugador.velocidad.z;

        Vector3 posicionAnterior =
            jugador.posicion;

        Vector3 empujePendiente =
            jugador.empuje;

        jugador.empuje = {};

        BloquePrueba sueloJugador =
            bloques[0];

        sueloJugador.activaColision =
            JugadorSobreArenaCircularPelotas(
                jugador
            );

        ActualizarJugadorPrueba(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            false,
            true,
            false,
            deltaTime
        );

        LimitarMovimientoPelota(
            jugador,
            velocidadAnteriorX,
            velocidadAnteriorZ,
            deltaTime
        );

        AplicarEmpujePendientePelotas(
            jugador,
            empujePendiente
        );

        ResolverColisionMontanaPelotas(
            jugador,
            posicionAnterior
        );
    }

    ResolverColisionesPelotas(
        jugadores,
        participantes,
        cantidadMaxima
    );

    PotenciarEmpujesPelotas(
        jugadores,
        participantes,
        cantidadMaxima
    );

    int eliminadosEsteFrame =
        0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminadosEsteFrame++;
        }
    }

    int posicionEliminados =
        vivosAntes -
        eliminadosEsteFrame +
        1;

    int tiempoSobrevividoMs =
        (int)std::lround(
            tiempoJugado *
            1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado =
                true;

            estadosJugadores[i].posicionFinal =
                posicionEliminados;

            estadosJugadores[i]
                .tiempoSobrevividoMs =
                tiempoSobrevividoMs;

            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }
    }

    int vivosDespues =
        vivosAntes -
        eliminadosEsteFrame;

    if (
        vivosDespues <= 1 ||
        tiempoRestante <= 0.0f
    )
    {
        FinalizarResultadoPelotas(
            *this,
            participantes
        );
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoPelotas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(
        Color
        {
            181,
            220,
            238,
            255
        }
    );

    BeginMode3D(
        camara
    );

    DibujarMontanaNievePelotas(
        mostrarDebug
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        DibujarJugadorPelotaPrueba(
            jugadores[i],
            participantes[i]
        );

        if (
            mostrarDebug &&
            participantes[i].activo &&
            participantes[i].conectado &&
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

    EndMode3D();

    DrawText(
        "MINIJUEGO 2 - PELOTAS / EMPUJONES",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        "EMPUJA A LOS DEMAS FUERA DE LA MONTANA",
        25,
        70,
        22,
        BLACK
    );

    DrawText(
        "NIEVE RESBALADIZA - ACELERA Y CONSERVA LA INERCIA",
        25,
        102,
        20,
        BLACK
    );

    DrawText(
        "MAS VELOCIDAD = MAS FUERZA DE EMPUJE",
        25,
        132,
        20,
        MAROON
    );

    if (fase == FASE_PELOTAS_JUGANDO)
    {
        DrawText(
            TextFormat(
                "TIEMPO: %.1f",
                tiempoRestante
            ),
            GetScreenWidth() - 210,
            26,
            26,
            tiempoRestante <= 10.0f
            ? RED
            : DARKBLUE
        );
    }

    int posicionY =
        168;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        float velocidad =
            MagnitudHorizontalPelotas(
                jugadores[i].velocidad.x,
                jugadores[i].velocidad.z
            );

        DrawText(
            TextFormat(
                "J%d VELOCIDAD: %.1f / %.1f",
                participantes[i].numeroJugador,
                velocidad,
                VELOCIDAD_MAXIMA_PELOTAS
            ),
            25,
            posicionY,
            18,
            participantes[i].color
        );

        posicionY +=
            24;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
    {
        int numero =
            (int)std::ceil(
                tiempoPreparacion
            );

        const char* texto =
            TextFormat(
                "%d",
                numero
            );

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            ORANGE
        );
    }
    else if (
        fase == FASE_PELOTAS_JUGANDO &&
        tiempoJugado <
            DURACION_TEXTO_YA_PELOTAS
    )
    {
        const char* texto =
            "YA";

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            LIME
        );
    }
    else if (fase == FASE_PELOTAS_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 330,
            GetScreenHeight() / 2 - 155,
            660,
            310,
            Fade(BLACK, 0.90f)
        );

        int indicesGanadores[
            MAX_PARTICIPANTES
        ]{};

        int cantidadGanadores =
            ObtenerIndicesGanadores(
                resultado,
                indicesGanadores,
                MAX_PARTICIPANTES
            );

        const char* titulo =
            resultado.desenlace ==
                DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                ? participantes[
                    indicesGanadores[0]
                  ].numeroJugador
                : 0
            );

        DrawText(
            titulo,
            GetScreenWidth() / 2 -
                MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 130,
            34,
            GOLD
        );

        int y =
            GetScreenHeight() / 2 -
            75;

        for (
            int i = 0;
            i < MAX_PARTICIPANTES;
            i++
        )
        {
            if (
                !resultado.participantes[i]
                    .participo
            )
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s",
                    participantes[i]
                        .numeroJugador,
                    resultado
                        .participantes[i]
                        .posicionFinal,
                    resultado
                        .participantes[i]
                        .puntuacionMinijuego /
                        1000.0f
                ),
                GetScreenWidth() / 2 -
                    210,
                y,
                22,
                participantes[i].color
            );

            y +=
                30;
        }

        const char* reiniciar =
            "R PARA REINICIAR";

        DrawText(
            reiniciar,
            GetScreenWidth() / 2 -
                MeasureText(
                    reiniciar,
                    22
                ) /
                2,
            GetScreenHeight() / 2 +
                112,
            22,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoPelotas::ObtenerResultado() const
{
    return resultado;
}
