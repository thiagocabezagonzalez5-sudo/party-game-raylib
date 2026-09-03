#include "Minigames/MinijuegoIslaFuego.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_ISLA = 3.0f;
static const float DURACION_PARTIDA_ISLA = 30.0f;
static const float DURACION_TEXTO_YA_ISLA = 0.75f;

static const float RADIO_ISLA = 5.2f;
static const float FUERZA_EXPLOSION_NORMAL = 5.2f;
static const float FUERZA_EXPLOSION_FINAL = 8.0f;
static const float DURACION_ATURDIMIENTO = 0.75f;

static const float FUERZA_DIRECTA_NORMAL = 19.0f;
static const float FUERZA_DIRECTA_ESPECIAL = 23.0f;
static const float SALTO_DIRECTO_NORMAL = 10.0f;
static const float SALTO_DIRECTO_ESPECIAL = 12.5f;
static const float DURACION_VUELO_DIRECTO = 0.55f;


//==================================================
// UTILIDADES
//==================================================

static float MagnitudHorizontalIsla(
    float x,
    float z
)
{
    return std::sqrt(
        x * x +
        z * z
    );
}


static bool JugadorSobreIsla(
    const JugadorPrueba& jugador
)
{
    float distancia =
        MagnitudHorizontalIsla(
            jugador.posicion.x,
            jugador.posicion.z
        );

    float margen =
        jugador.tamano.x * 0.18f;

    return distancia <= RADIO_ISLA - margen;
}


static Vector3 PuntoAleatorioIsla(
    float porcentajeRadio
)
{
    float angulo =
        (float)GetRandomValue(0, 6283) /
        1000.0f;

    float radio =
        (float)GetRandomValue(0, 1000) /
        1000.0f;

    radio =
        std::sqrt(radio) *
        RADIO_ISLA *
        porcentajeRadio;

    return Vector3
    {
        std::cos(angulo) * radio,
        0.31f,
        std::sin(angulo) * radio
    };
}


static float ObtenerProgresoProyectil(
    const ProyectilIslaFuego& proyectil
)
{
    if (proyectil.duracionAviso <= 0.0f)
    {
        return 0.0f;
    }

    float progreso =
        proyectil.tiempoHastaImpacto /
        proyectil.duracionAviso;

    if (progreso < 0.0f)
    {
        progreso = 0.0f;
    }

    if (progreso > 1.0f)
    {
        progreso = 1.0f;
    }

    return progreso;
}


static Vector3 ObtenerPosicionProyectil(
    const ProyectilIslaFuego& proyectil
)
{
    float progreso =
        ObtenerProgresoProyectil(
            proyectil
        );

    return Vector3
    {
        proyectil.puntoImpacto.x,
        0.55f + progreso * 9.5f,
        proyectil.puntoImpacto.z
    };
}


static float ObtenerRadioCuerpoProyectil(
    const ProyectilIslaFuego& proyectil
)
{
    return proyectil.especial
        ? 0.58f
        : 0.34f;
}


static bool ProyectilTocaJugador(
    const ProyectilIslaFuego& proyectil,
    const JugadorPrueba& jugador
)
{
    Vector3 posicionProyectil =
        ObtenerPosicionProyectil(
            proyectil
        );

    float radio =
        ObtenerRadioCuerpoProyectil(
            proyectil
        );

    BoundingBox cajaJugador =
        CrearHitboxJugadorPrueba(
            jugador
        );

    return
        posicionProyectil.x + radio >= cajaJugador.min.x &&
        posicionProyectil.x - radio <= cajaJugador.max.x &&
        posicionProyectil.y + radio >= cajaJugador.min.y &&
        posicionProyectil.y - radio <= cajaJugador.max.y &&
        posicionProyectil.z + radio >= cajaJugador.min.z &&
        posicionProyectil.z - radio <= cajaJugador.max.z;
}


//==================================================
// PARTICULAS
//==================================================

static void CrearParticulasExplosionIsla(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    Vector3 posicion,
    bool especial
)
{
    if (
        particulas == nullptr ||
        cantidadMaxima <= 0
    )
    {
        return;
    }

    int objetivo =
        especial
        ? 52
        : 32;

    int creadas = 0;

    for (
        int i = 0;
        i < cantidadMaxima && creadas < objetivo;
        i++
    )
    {
        ParticulaTierra& particula =
            particulas[i];

        if (particula.activa)
        {
            continue;
        }

        float angulo =
            (float)GetRandomValue(0, 6283) /
            1000.0f;

        float velocidad =
            (float)GetRandomValue(
                especial ? 35 : 22,
                especial ? 85 : 58
            ) /
            10.0f;

        particula.activa = true;

        particula.posicion =
        {
            posicion.x +
                (float)GetRandomValue(-20, 20) / 100.0f,

            posicion.y +
                (float)GetRandomValue(0, 28) / 100.0f,

            posicion.z +
                (float)GetRandomValue(-20, 20) / 100.0f
        };

        particula.velocidad =
        {
            std::cos(angulo) * velocidad,
            (float)GetRandomValue(28, especial ? 78 : 58) / 10.0f,
            std::sin(angulo) * velocidad
        };

        particula.vidaMaxima =
            (float)GetRandomValue(32, especial ? 78 : 58) /
            100.0f;

        particula.vida =
            particula.vidaMaxima;

        particula.tamano =
            (float)GetRandomValue(
                especial ? 12 : 8,
                especial ? 28 : 19
            ) /
            100.0f;

        int variante =
            GetRandomValue(0, 2);

        particula.color =
            variante == 0
            ? ORANGE
            : (
                variante == 1
                ? YELLOW
                : GOLD
            );

        creadas++;
    }
}


//==================================================
// RESULTADO
//==================================================

static void InicializarResultadoIsla(
    MinijuegoIslaFuego& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarVivosIsla(
    const MinijuegoIslaFuego& minijuego
)
{
    int cantidad = 0;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void FinalizarResultadoIsla(
    MinijuegoIslaFuego& minijuego
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int vivos =
        ContarVivosIsla(
            minijuego
        );

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        vivos == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(
            minijuego.tiempoJugado *
            1000.0f
        );

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorIslaFuego& estadoJugador =
            minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs =
                tiempoFinalMs;
        }

        resultadoJugador.posicionFinal =
            estadoJugador.posicionFinal;

        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase =
        FASE_ISLA_FUEGO_TERMINADO;
}


//==================================================
// PROYECTILES
//==================================================

static void LanzarProyectilIsla(
    MinijuegoIslaFuego& minijuego,
    bool especial
)
{
    minijuego.proyectil = {};

    minijuego.proyectil.activo = true;
    minijuego.proyectil.especial = especial;

    minijuego.proyectil.puntoImpacto =
        PuntoAleatorioIsla(
            especial
            ? 0.46f
            : 0.82f
        );

    minijuego.proyectil.duracionAviso =
        especial
        ? 1.55f
        : 1.18f;

    minijuego.proyectil.tiempoHastaImpacto =
        minijuego.proyectil.duracionAviso;

    minijuego.proyectil.radioExplosion =
        especial
        ? 3.35f
        : 2.05f;

    if (especial)
    {
        minijuego.disparoFinalRealizado = true;
    }
}


static void ProgramarSiguienteDisparo(
    MinijuegoIslaFuego& minijuego
)
{
    float progreso =
        minijuego.tiempoJugado /
        DURACION_PARTIDA_ISLA;

    float minimo =
        1.15f - progreso * 0.42f;

    float maximo =
        1.75f - progreso * 0.48f;

    minijuego.tiempoHastaSiguienteDisparo =
        (float)GetRandomValue(
            (int)(minimo * 100.0f),
            (int)(maximo * 100.0f)
        ) /
        100.0f;
}


static void AplicarExplosionIsla(
    MinijuegoIslaFuego& minijuego,
    JugadorPrueba jugadores[],
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    const ProyectilIslaFuego& proyectil =
        minijuego.proyectil;

    CrearParticulasExplosionIsla(
        particulas,
        cantidadParticulas,
        proyectil.puntoImpacto,
        proyectil.especial
    );

    float fuerzaBase =
        proyectil.especial
        ? FUERZA_EXPLOSION_FINAL
        : FUERZA_EXPLOSION_NORMAL;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (
            !minijuego.resultado.participantes[i].participo ||
            minijuego.estadosJugadores[i].eliminado ||
            minijuego.estadosJugadores[i].impactoDirecto ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        JugadorPrueba& jugador =
            jugadores[i];

        float dx =
            jugador.posicion.x -
            proyectil.puntoImpacto.x;

        float dz =
            jugador.posicion.z -
            proyectil.puntoImpacto.z;

        float distancia =
            MagnitudHorizontalIsla(
                dx,
                dz
            );

        if (
            distancia >
            proyectil.radioExplosion
        )
        {
            continue;
        }

        // El area de explosion puede evitarse saltando.
        // El contacto DIRECTO con la bomba no puede evitarse asi.
        if (!jugador.enSuelo)
        {
            continue;
        }

        if (distancia < 0.05f)
        {
            dx = 1.0f;
            dz = 0.0f;
            distancia = 1.0f;
        }

        float normalX =
            dx / distancia;

        float normalZ =
            dz / distancia;

        float cercania =
            1.0f -
            distancia /
            proyectil.radioExplosion;

        float fuerza =
            fuerzaBase *
            (0.45f + 0.55f * cercania);

        jugador.empuje.x +=
            normalX * fuerza;

        jugador.empuje.z +=
            normalZ * fuerza;

        jugador.velocidad.y =
            3.4f + cercania * 2.0f;

        jugador.enSuelo = false;

        minijuego.estadosJugadores[i].tiempoAturdido =
            DURACION_ATURDIMIENTO +
            (proyectil.especial ? 0.25f : 0.0f);
    }
}


static bool AplicarImpactosDirectos(
    MinijuegoIslaFuego& minijuego,
    JugadorPrueba jugadores[],
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    bool huboContacto = false;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        EstadoJugadorIslaFuego& estadoJugador =
            minijuego.estadosJugadores[i];

        if (
            !minijuego.resultado.participantes[i].participo ||
            estadoJugador.eliminado ||
            estadoJugador.impactoDirecto ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        JugadorPrueba& jugador =
            jugadores[i];

        if (
            !ProyectilTocaJugador(
                minijuego.proyectil,
                jugador
            )
        )
        {
            continue;
        }

        huboContacto = true;

        float direccionX =
            jugador.posicion.x;

        float direccionZ =
            jugador.posicion.z;

        float longitud =
            MagnitudHorizontalIsla(
                direccionX,
                direccionZ
            );

        if (longitud < 0.10f)
        {
            float angulo =
                (float)GetRandomValue(0, 6283) /
                1000.0f;

            direccionX =
                std::cos(angulo);

            direccionZ =
                std::sin(angulo);

            longitud = 1.0f;
        }

        direccionX /= longitud;
        direccionZ /= longitud;

        float fuerza =
            minijuego.proyectil.especial
            ? FUERZA_DIRECTA_ESPECIAL
            : FUERZA_DIRECTA_NORMAL;

        jugador.empuje.x =
            direccionX * fuerza;

        jugador.empuje.z =
            direccionZ * fuerza;

        jugador.velocidad.y =
            minijuego.proyectil.especial
            ? SALTO_DIRECTO_ESPECIAL
            : SALTO_DIRECTO_NORMAL;

        jugador.enSuelo = false;
        jugador.golpeSueloActivo = false;
        jugador.golpeando = false;

        estadoJugador.impactoDirecto = true;
        estadoJugador.tiempoHastaEliminacionDirecta =
            DURACION_VUELO_DIRECTO;
        estadoJugador.tiempoAturdido =
            DURACION_VUELO_DIRECTO;

        CrearParticulasExplosionIsla(
            particulas,
            cantidadParticulas,
            jugador.posicion,
            true
        );
    }

    return huboContacto;
}


//==================================================
// INICIALIZAR / REINICIAR
//==================================================

void MinijuegoIslaFuego::Inicializar()
{
    resultado = {};
    resultado.formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        estadosJugadores[i] = {};
    }

    suelo = {};
    suelo.posicion =
        { 0.0f, -0.45f, 0.0f };
    suelo.posicionInicial =
        suelo.posicion;
    suelo.tamano =
        { 11.5f, 0.90f, 11.5f };
    suelo.color =
        Color{ 88, 128, 76, 255 };
    suelo.activaColision = true;

    fase =
        FASE_ISLA_FUEGO_PREPARACION;

    tiempoPreparacion =
        DURACION_PREPARACION_ISLA;

    tiempoRestante =
        DURACION_PARTIDA_ISLA;

    tiempoJugado = 0.0f;
    tiempoHastaSiguienteDisparo = 0.75f;
    disparoFinalRealizado = false;
    proyectil = {};

    camara.position =
        { 0.0f, 10.0f, 14.0f };
    camara.target =
        { 0.0f, 0.15f, 0.0f };
    camara.up =
        { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection =
        CAMERA_PERSPECTIVE;
}


void MinijuegoIslaFuego::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -1.7f, 1.05f, 1.7f },
        { 1.7f, 1.05f, 1.7f },
        { -1.7f, 1.05f, -1.7f },
        { 1.7f, 1.05f, -1.7f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (
        int i = 0;
        i < limite;
        i++
    )
    {
        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            spawns[i]
        );
    }
}


void MinijuegoIslaFuego::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participaban[MAX_PARTICIPANTES]{};

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        participaban[i] =
            resultado.participantes[i].participo;
    }

    resultado = {};
    resultado.formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        resultado.participantes[i].participo =
            participaban[i];

        if (participaban[i])
        {
            resultado.cantidadParticipantes++;
        }

        estadosJugadores[i] = {};
    }

    fase =
        FASE_ISLA_FUEGO_PREPARACION;

    tiempoPreparacion =
        DURACION_PREPARACION_ISLA;

    tiempoRestante =
        DURACION_PARTIDA_ISLA;

    tiempoJugado = 0.0f;
    tiempoHastaSiguienteDisparo = 0.75f;
    disparoFinalRealizado = false;
    proyectil = {};

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoIslaFuego::Actualizar(
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
        InicializarResultadoIsla(
            *this,
            participantes
        );
    }

    if (fase == FASE_ISLA_FUEGO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_ISLA_FUEGO_PREPARACION)
    {
        for (
            int i = 0;
            i < MAX_PARTICIPANTES;
            i++
        )
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -=
            deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase =
                FASE_ISLA_FUEGO_JUGANDO;
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
        DURACION_PARTIDA_ISLA -
        tiempoRestante;

    int vivosAntes =
        ContarVivosIsla(
            *this
        );

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        JugadorPrueba& jugador =
            jugadores[i];

        EstadoJugadorIslaFuego& estadoJugador =
            estadosJugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estadoJugador.eliminado
        )
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            continue;
        }

        if (!participantes[i].conectado)
        {
            continue;
        }

        if (estadoJugador.tiempoAturdido > 0.0f)
        {
            estadoJugador.tiempoAturdido -=
                deltaTime;

            if (estadoJugador.tiempoAturdido < 0.0f)
            {
                estadoJugador.tiempoAturdido = 0.0f;
            }
        }

        InputMinijuegoParticipante entrada{};

        if (
            estadoJugador.tiempoAturdido <= 0.0f &&
            !estadoJugador.impactoDirecto
        )
        {
            entrada =
                LeerInputMinijuegoParticipante(
                    participantes[i]
                );
        }

        // Isla Bajo Fuego usa movimiento y salto, no golpes.
        entrada.golpear = false;

        BloquePrueba sueloJugador =
            suelo;

        sueloJugador.activaColision =
            JugadorSobreIsla(
                jugador
            );

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            !estadoJugador.impactoDirecto,
            false,
            deltaTime
        );

        if (estadoJugador.impactoDirecto)
        {
            estadoJugador.tiempoHastaEliminacionDirecta -=
                deltaTime;

            if (
                estadoJugador.tiempoHastaEliminacionDirecta <=
                0.0f
            )
            {
                estadoJugador.tiempoHastaEliminacionDirecta =
                    0.0f;

                // Ya se mostro el vuelo provocado por la bomba.
                // Desde este punto cuenta como eliminado.
                jugador.cayendo = true;
            }
        }
    }

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantes,
        cantidadMaxima
    );

    if (proyectil.activo)
    {
        proyectil.tiempoHastaImpacto -=
            deltaTime;

        bool impactoDirecto =
            AplicarImpactosDirectos(
                *this,
                jugadores,
                participantes,
                particulas,
                cantidadParticulas
            );

        if (impactoDirecto)
        {
            // La bomba detona al tocar directamente a un jugador.
            AplicarExplosionIsla(
                *this,
                jugadores,
                participantes,
                particulas,
                cantidadParticulas
            );

            proyectil.activo = false;

            ProgramarSiguienteDisparo(
                *this
            );
        }
        else if (
            proyectil.tiempoHastaImpacto <=
            0.0f
        )
        {
            AplicarExplosionIsla(
                *this,
                jugadores,
                participantes,
                particulas,
                cantidadParticulas
            );

            proyectil.activo = false;

            ProgramarSiguienteDisparo(
                *this
            );
        }
    }
    else
    {
        tiempoHastaSiguienteDisparo -=
            deltaTime;

        if (
            tiempoRestante <= 7.0f &&
            !disparoFinalRealizado
        )
        {
            LanzarProyectilIsla(
                *this,
                true
            );
        }
        else if (
            tiempoHastaSiguienteDisparo <= 0.0f
        )
        {
            LanzarProyectilIsla(
                *this,
                false
            );
        }
    }

    int eliminadosEsteFrame = 0;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (
            resultado.participantes[i].participo &&
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

    int tiempoMs =
        (int)std::lround(
            tiempoJugado *
            1000.0f
        );

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal =
                posicionEliminados;
            estadosJugadores[i].tiempoSobrevividoMs =
                tiempoMs;

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
        FinalizarResultadoIsla(
            *this
        );
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoIslaFuego::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(
        Color{ 106, 178, 216, 255 }
    );

    BeginMode3D(
        camara
    );

    DrawCylinder(
        Vector3{ 0.0f, -0.32f, 0.0f },
        RADIO_ISLA,
        RADIO_ISLA + 0.65f,
        0.75f,
        48,
        Color{ 104, 135, 73, 255 }
    );

    DrawCylinder(
        Vector3{ 0.0f, 0.02f, 0.0f },
        RADIO_ISLA,
        RADIO_ISLA,
        0.12f,
        48,
        Color{ 94, 166, 83, 255 }
    );

    DrawCircle3D(
        Vector3{ 0.0f, 0.09f, 0.0f },
        RADIO_ISLA,
        Vector3{ 1.0f, 0.0f, 0.0f },
        90.0f,
        Fade(DARKGREEN, 0.55f)
    );

    if (proyectil.activo)
    {
        float progreso =
            ObtenerProgresoProyectil(
                proyectil
            );

        float radioAviso =
            proyectil.radioExplosion *
            (
                0.88f +
                0.12f *
                std::sin(
                    proyectil.tiempoHastaImpacto *
                    18.0f
                )
            );

        DrawCircle3D(
            Vector3{
                proyectil.puntoImpacto.x,
                0.12f,
                proyectil.puntoImpacto.z
            },
            radioAviso,
            Vector3{ 1.0f, 0.0f, 0.0f },
            90.0f,
            proyectil.especial
            ? Fade(RED, 0.82f)
            : Fade(ORANGE, 0.78f)
        );

        Vector3 posicionProyectil =
            ObtenerPosicionProyectil(
                proyectil
            );

        DrawSphere(
            posicionProyectil,
            ObtenerRadioCuerpoProyectil(
                proyectil
            ),
            proyectil.especial
            ? MAROON
            : DARKGRAY
        );

        if (mostrarDebug)
        {
            DrawSphereWires(
                posicionProyectil,
                ObtenerRadioCuerpoProyectil(
                    proyectil
                ),
                10,
                10,
                YELLOW
            );
        }
    }

    DibujarParticulasTierra(
        particulas,
        cantidadParticulas
    );

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        DibujarJugadorCuboPrueba(
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
        "MINIJUEGO 8 - ISLA BAJO FUEGO",
        25,
        25,
        30,
        BLACK
    );

    DrawText(
        "EVITA EL AREA - SI LA BOMBA TE TOCA, TE MANDA A VOLAR Y TE ELIMINA",
        25,
        68,
        19,
        BLACK
    );

    if (fase == FASE_ISLA_FUEGO_JUGANDO)
    {
        DrawText(
            TextFormat(
                "TIEMPO: %.1f",
                tiempoRestante
            ),
            GetScreenWidth() - 205,
            25,
            26,
            tiempoRestante <= 7.0f
            ? RED
            : DARKBLUE
        );

        if (
            tiempoRestante <= 7.0f &&
            !disparoFinalRealizado
        )
        {
            DrawText(
                "SE VIENE UN IMPACTO GRANDE",
                25,
                100,
                20,
                MAROON
            );
        }
    }

    int yEstado = 100;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const char* estadoTexto =
            estadosJugadores[i].eliminado
            ? "FUERA"
            : (
                estadosJugadores[i].impactoDirecto
                ? "VOLANDO"
                : (
                    estadosJugadores[i].tiempoAturdido > 0.0f
                    ? "ATURDIDO"
                    : "EN JUEGO"
                )
            );

        DrawText(
            TextFormat(
                "J%d: %s",
                participantes[i].numeroJugador,
                estadoTexto
            ),
            25,
            yEstado,
            18,
            estadosJugadores[i].eliminado
            ? DARKGRAY
            : participantes[i].color
        );

        yEstado += 24;
    }

    if (fase == FASE_ISLA_FUEGO_PREPARACION)
    {
        int numero =
            (int)std::ceil(
                tiempoPreparacion
            );

        if (numero < 1)
        {
            numero = 1;
        }

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
        fase == FASE_ISLA_FUEGO_JUGANDO &&
        tiempoJugado < DURACION_TEXTO_YA_ISLA
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
    else if (
        fase == FASE_ISLA_FUEGO_TERMINADO
    )
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 330,
            GetScreenHeight() / 2 - 155,
            660,
            310,
            Fade(BLACK, 0.90f)
        );

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
                ? participantes[
                    ganadores[0]
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
            GetScreenHeight() / 2 - 72;

        for (
            int i = 0;
            i < MAX_PARTICIPANTES;
            i++
        )
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    resultado.participantes[i].puntuacionMinijuego /
                        1000.0f
                ),
                GetScreenWidth() / 2 - 210,
                y,
                22,
                participantes[i].color
            );

            y += 30;
        }

        const char* reiniciar =
            "R PARA REINICIAR";

        DrawText(
            reiniciar,
            GetScreenWidth() / 2 -
                MeasureText(reiniciar, 22) / 2,
            GetScreenHeight() / 2 + 112,
            22,
            RAYWHITE
        );
    }
}


//==================================================
// RESULTADO PUBLICO
//==================================================

const ResultadoMinijuego&
MinijuegoIslaFuego::ObtenerResultado() const
{
    return resultado;
}
