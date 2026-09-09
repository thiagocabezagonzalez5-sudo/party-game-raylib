#include "Minigames/MinijuegoIslaFuego.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"

#include <cmath>


static const float DURACION_PREPARACION_ISLA = 3.0f;
static const float ESCALA_DIFICULTAD_ISLA = 30.0f;
static const float DURACION_TEXTO_YA_ISLA = 0.75f;

static const float RADIO_ISLA = 5.2f;
static const float FUERZA_EXPLOSION_NORMAL = 5.8f;
static const float FUERZA_EXPLOSION_ESPECIAL = 8.6f;
static const float DURACION_ATURDIMIENTO = 0.75f;

// La explosion ahora tiene dos zonas distintas. El centro elimina y la
// corona exterior solo empuja. La corona es mas chica que antes para que el
// peligro se concentre alrededor del punto de impacto.
static const float RADIO_ELIMINACION_NORMAL = 1.42f;
static const float RADIO_ELIMINACION_ESPECIAL = 2.28f;
static const float RADIO_EMPUJE_NORMAL = 1.90f;
static const float RADIO_EMPUJE_ESPECIAL = 2.80f;

static const float FUERZA_DIRECTA_NORMAL = 19.0f;
static const float FUERZA_DIRECTA_ESPECIAL = 23.0f;
static const float SALTO_DIRECTO_NORMAL = 10.0f;
static const float SALTO_DIRECTO_ESPECIAL = 12.5f;
static const float DURACION_VUELO_DIRECTO = 0.48f;

static const float AVISO_BOMBA_INICIAL = 1.30f;
static const float AVISO_BOMBA_FINAL = 0.55f;
static const float PAUSA_BOMBA_MIN_INICIAL = 1.05f;
static const float PAUSA_BOMBA_MAX_INICIAL = 1.55f;
static const float PAUSA_BOMBA_MIN_FINAL = 0.34f;
static const float PAUSA_BOMBA_MAX_FINAL = 0.64f;


static float Limitar01Isla(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float MagnitudHorizontalIsla(float x, float z)
{
    return std::sqrt(x * x + z * z);
}


static float ObtenerProgresoPartidaIsla(
    const MinijuegoIslaFuego& minijuego
)
{
    return Limitar01Isla(
        minijuego.tiempoJugado / ESCALA_DIFICULTAD_ISLA
    );
}


static bool JugadorSobreIsla(const JugadorPrueba& jugador)
{
    float distancia = MagnitudHorizontalIsla(
        jugador.posicion.x,
        jugador.posicion.z
    );
    float margen = jugador.tamano.x * 0.18f;
    return distancia <= RADIO_ISLA - margen;
}


static Vector3 PuntoAleatorioIsla(float porcentajeRadio)
{
    float angulo = (float)GetRandomValue(0, 6283) / 1000.0f;
    float radio = (float)GetRandomValue(0, 1000) / 1000.0f;
    radio = std::sqrt(radio) * RADIO_ISLA * porcentajeRadio;

    return {
        std::cos(angulo) * radio,
        0.02f,
        std::sin(angulo) * radio
    };
}


static float ObtenerProgresoProyectil(const ProyectilIslaFuego& proyectil)
{
    if (proyectil.duracionAviso <= 0.0f) return 0.0f;
    return Limitar01Isla(
        proyectil.tiempoHastaImpacto / proyectil.duracionAviso
    );
}


static Vector3 ObtenerPosicionProyectil(const ProyectilIslaFuego& proyectil)
{
    float progreso = ObtenerProgresoProyectil(proyectil);
    return {
        proyectil.puntoImpacto.x,
        0.55f + progreso * 9.5f,
        proyectil.puntoImpacto.z
    };
}


static float ObtenerRadioCuerpoProyectil(const ProyectilIslaFuego& proyectil)
{
    return proyectil.especial ? 0.68f : 0.42f;
}


static bool ProyectilTocaJugador(
    const ProyectilIslaFuego& proyectil,
    const JugadorPrueba& jugador
)
{
    Vector3 posicion = ObtenerPosicionProyectil(proyectil);
    float radio = ObtenerRadioCuerpoProyectil(proyectil);
    BoundingBox caja = CrearHitboxJugadorPrueba(jugador);

    return
        posicion.x + radio >= caja.min.x &&
        posicion.x - radio <= caja.max.x &&
        posicion.y + radio >= caja.min.y &&
        posicion.y - radio <= caja.max.y &&
        posicion.z + radio >= caja.min.z &&
        posicion.z - radio <= caja.max.z;
}


static void CrearParticulasExplosionIsla(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    Vector3 posicion,
    bool especial
)
{
    if (particulas == nullptr || cantidadMaxima <= 0) return;

    int objetivo = especial ? 52 : 32;
    int creadas = 0;

    for (int i = 0; i < cantidadMaxima && creadas < objetivo; i++)
    {
        ParticulaTierra& particula = particulas[i];
        if (particula.activa) continue;

        float angulo = (float)GetRandomValue(0, 6283) / 1000.0f;
        float velocidad = (float)GetRandomValue(
            especial ? 35 : 22,
            especial ? 85 : 58
        ) / 10.0f;

        particula.activa = true;
        particula.posicion = {
            posicion.x + (float)GetRandomValue(-20, 20) / 100.0f,
            posicion.y + (float)GetRandomValue(0, 28) / 100.0f,
            posicion.z + (float)GetRandomValue(-20, 20) / 100.0f
        };
        particula.velocidad = {
            std::cos(angulo) * velocidad,
            (float)GetRandomValue(28, especial ? 78 : 58) / 10.0f,
            std::sin(angulo) * velocidad
        };
        particula.vidaMaxima =
            (float)GetRandomValue(32, especial ? 78 : 58) / 100.0f;
        particula.vida = particula.vidaMaxima;
        particula.tamano =
            (float)GetRandomValue(especial ? 12 : 8, especial ? 28 : 19) /
            100.0f;
        int variante = GetRandomValue(0, 2);
        particula.color = variante == 0
            ? ORANGE
            : (variante == 1 ? YELLOW : GOLD);
        creadas++;
    }
}


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


static int ContarVivosIsla(const MinijuegoIslaFuego& minijuego)
{
    int cantidad = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
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


static void FinalizarResultadoIsla(MinijuegoIslaFuego& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO) return;

    int vivos = ContarVivosIsla(minijuego);
    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        vivos == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;

    int tiempoFinalMs = (int)std::lround(minijuego.tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];
        if (!resultadoJugador.participo) continue;

        EstadoJugadorIslaFuego& estadoJugador = minijuego.estadosJugadores[i];
        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_ISLA_FUEGO_TERMINADO;
}


static void LanzarProyectilIsla(
    MinijuegoIslaFuego& minijuego,
    bool especial
)
{
    minijuego.proyectil = {};
    minijuego.proyectil.activo = true;
    minijuego.proyectil.especial = especial;
    minijuego.proyectil.puntoImpacto = PuntoAleatorioIsla(especial ? 0.62f : 0.84f);

    float progreso = ObtenerProgresoPartidaIsla(minijuego);
    float duracionNormal =
        AVISO_BOMBA_INICIAL +
        (AVISO_BOMBA_FINAL - AVISO_BOMBA_INICIAL) * progreso;

    minijuego.proyectil.duracionAviso = especial
        ? duracionNormal + 0.10f
        : duracionNormal;

    if (minijuego.proyectil.duracionAviso < 0.50f)
        minijuego.proyectil.duracionAviso = 0.50f;

    minijuego.proyectil.tiempoHastaImpacto = minijuego.proyectil.duracionAviso;
    minijuego.proyectil.radioExplosion = especial
        ? RADIO_EMPUJE_ESPECIAL
        : RADIO_EMPUJE_NORMAL;
}


static void ProgramarSiguienteDisparo(MinijuegoIslaFuego& minijuego)
{
    float progreso = ObtenerProgresoPartidaIsla(minijuego);

    float minimo =
        PAUSA_BOMBA_MIN_INICIAL +
        (PAUSA_BOMBA_MIN_FINAL - PAUSA_BOMBA_MIN_INICIAL) * progreso;
    float maximo =
        PAUSA_BOMBA_MAX_INICIAL +
        (PAUSA_BOMBA_MAX_FINAL - PAUSA_BOMBA_MAX_INICIAL) * progreso;

    if (minimo < 0.30f) minimo = 0.30f;
    if (maximo < minimo + 0.10f) maximo = minimo + 0.10f;

    minijuego.tiempoHastaSiguienteDisparo =
        (float)GetRandomValue((int)(minimo * 100.0f), (int)(maximo * 100.0f)) /
        100.0f;
}


static void ConvertirEnImpactoDirecto(
    MinijuegoIslaFuego& minijuego,
    int indice,
    JugadorPrueba& jugador,
    bool especial,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    EstadoJugadorIslaFuego& estado = minijuego.estadosJugadores[indice];
    if (estado.impactoDirecto || estado.eliminado) return;

    float dx = jugador.posicion.x - minijuego.proyectil.puntoImpacto.x;
    float dz = jugador.posicion.z - minijuego.proyectil.puntoImpacto.z;
    float longitud = MagnitudHorizontalIsla(dx, dz);

    if (longitud < 0.10f)
    {
        float angulo = (float)GetRandomValue(0, 6283) / 1000.0f;
        dx = std::cos(angulo);
        dz = std::sin(angulo);
        longitud = 1.0f;
    }

    dx /= longitud;
    dz /= longitud;

    float fuerza = especial ? FUERZA_DIRECTA_ESPECIAL : FUERZA_DIRECTA_NORMAL;
    jugador.empuje.x = dx * fuerza;
    jugador.empuje.z = dz * fuerza;
    jugador.velocidad.y = especial ? SALTO_DIRECTO_ESPECIAL : SALTO_DIRECTO_NORMAL;
    jugador.enSuelo = false;
    jugador.golpeSueloActivo = false;
    jugador.preparandoGolpeSuelo = false;
    jugador.golpeando = false;

    estado.impactoDirecto = true;
    estado.tiempoHastaEliminacionDirecta = DURACION_VUELO_DIRECTO;
    estado.tiempoAturdido = DURACION_VUELO_DIRECTO;

    CrearParticulasExplosionIsla(
        particulas,
        cantidadParticulas,
        jugador.posicion,
        true
    );
}


static void AplicarExplosionIsla(
    MinijuegoIslaFuego& minijuego,
    JugadorPrueba jugadores[],
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    const ProyectilIslaFuego& proyectil = minijuego.proyectil;

    CrearParticulasExplosionIsla(
        particulas,
        cantidadParticulas,
        proyectil.puntoImpacto,
        proyectil.especial
    );

    float radioEliminacion = proyectil.especial
        ? RADIO_ELIMINACION_ESPECIAL
        : RADIO_ELIMINACION_NORMAL;
    float radioEmpuje = proyectil.especial
        ? RADIO_EMPUJE_ESPECIAL
        : RADIO_EMPUJE_NORMAL;
    float fuerzaBase = proyectil.especial
        ? FUERZA_EXPLOSION_ESPECIAL
        : FUERZA_EXPLOSION_NORMAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
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

        JugadorPrueba& jugador = jugadores[i];
        float dx = jugador.posicion.x - proyectil.puntoImpacto.x;
        float dz = jugador.posicion.z - proyectil.puntoImpacto.z;
        float distancia = MagnitudHorizontalIsla(dx, dz);

        if (distancia <= radioEliminacion)
        {
            ConvertirEnImpactoDirecto(
                minijuego,
                i,
                jugador,
                proyectil.especial,
                particulas,
                cantidadParticulas
            );
            continue;
        }

        if (distancia > radioEmpuje || !jugador.enSuelo) continue;

        if (distancia < 0.05f)
        {
            dx = 1.0f;
            dz = 0.0f;
            distancia = 1.0f;
        }

        float nx = dx / distancia;
        float nz = dz / distancia;
        float anchoCorona = radioEmpuje - radioEliminacion;
        float cercania = anchoCorona > 0.01f
            ? 1.0f - (distancia - radioEliminacion) / anchoCorona
            : 1.0f;
        cercania = Limitar01Isla(cercania);
        float fuerza = fuerzaBase * (0.55f + 0.45f * cercania);

        jugador.empuje.x += nx * fuerza;
        jugador.empuje.z += nz * fuerza;
        jugador.velocidad.y = 3.2f + cercania * 1.8f;
        jugador.enSuelo = false;
        minijuego.estadosJugadores[i].tiempoAturdido =
            DURACION_ATURDIMIENTO + (proyectil.especial ? 0.20f : 0.0f);
    }
}


static bool AplicarImpactosDirectosCaida(
    MinijuegoIslaFuego& minijuego,
    JugadorPrueba jugadores[],
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    bool huboContacto = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorIslaFuego& estado = minijuego.estadosJugadores[i];

        if (
            !minijuego.resultado.participantes[i].participo ||
            estado.eliminado ||
            estado.impactoDirecto ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        if (!ProyectilTocaJugador(minijuego.proyectil, jugadores[i])) continue;

        huboContacto = true;
        ConvertirEnImpactoDirecto(
            minijuego,
            i,
            jugadores[i],
            minijuego.proyectil.especial,
            particulas,
            cantidadParticulas
        );
    }

    return huboContacto;
}


void MinijuegoIslaFuego::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;
    for (int i = 0; i < MAX_PARTICIPANTES; i++) estadosJugadores[i] = {};

    suelo = {};
    suelo.posicion = { 0.0f, -0.30f, 0.0f };
    suelo.posicionInicial = suelo.posicion;
    suelo.tamano = { 11.5f, 0.60f, 11.5f };
    suelo.color = Color{ 138, 142, 148, 255 };
    suelo.activaColision = true;

    fase = FASE_ISLA_FUEGO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_ISLA;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;
    tiempoHastaSiguienteDisparo = 0.70f;
    disparoFinalRealizado = false;
    proyectil = {};

    camara.position = { 0.0f, 10.0f, 14.0f };
    camara.target = { 0.0f, 0.15f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoIslaFuego::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -1.7f, 1.05f, 1.7f }, { 1.7f, 1.05f, 1.7f },
        { -1.7f, 1.05f, -1.7f }, { 1.7f, 1.05f, -1.7f }
    };

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;
    for (int i = 0; i < limite; i++)
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawns[i]);
}


void MinijuegoIslaFuego::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participaban[MAX_PARTICIPANTES]{};
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
        participaban[i] = resultado.participantes[i].participo;

    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo = participaban[i];
        if (participaban[i]) resultado.cantidadParticipantes++;
        estadosJugadores[i] = {};
    }

    fase = FASE_ISLA_FUEGO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_ISLA;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;
    tiempoHastaSiguienteDisparo = 0.70f;
    disparoFinalRealizado = false;
    proyectil = {};

    for (int i = 0; i < cantidadMaxima; i++) ReiniciarJugadorPrueba(jugadores[i]);
}


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
        InicializarResultadoIsla(*this, participantes);

    if (fase == FASE_ISLA_FUEGO_TERMINADO) return;

    if (fase == FASE_ISLA_FUEGO_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -= deltaTime;
        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_ISLA_FUEGO_JUGANDO;
        }
        return;
    }

    tiempoJugado += deltaTime;
    int vivosAntes = ContarVivosIsla(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador = jugadores[i];
        EstadoJugadorIslaFuego& estado = estadosJugadores[i];

        if (!resultado.participantes[i].participo || estado.eliminado)
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            continue;
        }

        if (!participantes[i].conectado) continue;

        if (estado.tiempoAturdido > 0.0f)
        {
            estado.tiempoAturdido -= deltaTime;
            if (estado.tiempoAturdido < 0.0f) estado.tiempoAturdido = 0.0f;
        }

        InputMinijuegoParticipante entrada{};
        if (estado.tiempoAturdido <= 0.0f && !estado.impactoDirecto)
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        entrada.golpear = false;

        BloquePrueba sueloJugador = suelo;
        sueloJugador.activaColision = JugadorSobreIsla(jugador);

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            !estado.impactoDirecto,
            false,
            deltaTime
        );

        if (estado.impactoDirecto)
        {
            estado.tiempoHastaEliminacionDirecta -= deltaTime;
            if (estado.tiempoHastaEliminacionDirecta <= 0.0f)
            {
                estado.tiempoHastaEliminacionDirecta = 0.0f;
                jugador.cayendo = true;
            }
        }
    }

    ResolverColisionesJugadoresSinEmpuje(jugadores, participantes, cantidadMaxima);

    if (proyectil.activo)
    {
        proyectil.tiempoHastaImpacto -= deltaTime;

        bool directo = AplicarImpactosDirectosCaida(
            *this,
            jugadores,
            participantes,
            particulas,
            cantidadParticulas
        );

        if (directo || proyectil.tiempoHastaImpacto <= 0.0f)
        {
            AplicarExplosionIsla(
                *this,
                jugadores,
                participantes,
                particulas,
                cantidadParticulas
            );
            proyectil.activo = false;
            ProgramarSiguienteDisparo(*this);
        }
    }
    else
    {
        tiempoHastaSiguienteDisparo -= deltaTime;

        if (tiempoHastaSiguienteDisparo <= 0.0f)
        {
            int probEspecial = 0;
            if (tiempoJugado > 8.0f)
            {
                probEspecial = 12 + (int)((tiempoJugado - 8.0f) * 0.8f);
                if (probEspecial > 38) probEspecial = 38;
            }

            bool especial =
                probEspecial > 0 &&
                GetRandomValue(1, 100) <= probEspecial;

            LanzarProyectilIsla(*this, especial);
        }
    }

    int eliminadosEsteFrame = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
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

    int posicionEliminados = vivosAntes - eliminadosEsteFrame + 1;
    int tiempoMs = (int)std::lround(tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal = posicionEliminados;
            estadosJugadores[i].tiempoSobrevividoMs = tiempoMs;
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }
    }

    if (vivosAntes - eliminadosEsteFrame <= 1)
        FinalizarResultadoIsla(*this);
}


static void DibujarObjetivoBombaIsla(
    const ProyectilIslaFuego& proyectil,
    bool mostrarDebug
)
{
    Vector3 centro = {
        proyectil.puntoImpacto.x,
        0.015f,
        proyectil.puntoImpacto.z
    };

    // Normalmente solo se ve una sombra discreta del punto de caida.
    DrawCylinder(
        centro,
        proyectil.especial ? 0.66f : 0.48f,
        proyectil.especial ? 0.66f : 0.48f,
        0.012f,
        32,
        Fade(proyectil.especial ? MAROON : BLACK, 0.22f)
    );

    if (!mostrarDebug) return;

    float radioEliminacion = proyectil.especial
        ? RADIO_ELIMINACION_ESPECIAL
        : RADIO_ELIMINACION_NORMAL;
    float radioEmpuje = proyectil.especial
        ? RADIO_EMPUJE_ESPECIAL
        : RADIO_EMPUJE_NORMAL;

    DrawCircle3D(
        centro,
        radioEliminacion,
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        RED
    );

    DrawCircle3D(
        centro,
        radioEmpuje,
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Fade(YELLOW, 0.75f)
    );
}


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
    ClearBackground(Color{ 106, 178, 216, 255 });
    BeginMode3D(camara);

    DrawCylinder(
        { 0.0f, -0.18f, 0.0f },
        RADIO_ISLA,
        RADIO_ISLA,
        0.18f,
        64,
        Color{ 150, 154, 160, 255 }
    );
    DrawCylinder(
        { 0.0f, -0.30f, 0.0f },
        RADIO_ISLA + 0.12f,
        RADIO_ISLA + 0.12f,
        0.12f,
        64,
        Color{ 72, 76, 84, 255 }
    );
    DrawCircle3D(
        { 0.0f, 0.005f, 0.0f },
        RADIO_ISLA,
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Fade(DARKGRAY, 0.80f)
    );

    if (proyectil.activo)
    {
        DibujarObjetivoBombaIsla(proyectil, mostrarDebug);
        Vector3 posicion = ObtenerPosicionProyectil(proyectil);

        DrawSphere(
            posicion,
            ObtenerRadioCuerpoProyectil(proyectil),
            proyectil.especial ? MAROON : DARKGRAY
        );
        DrawCylinder(
            {
                posicion.x,
                posicion.y + ObtenerRadioCuerpoProyectil(proyectil) + 0.12f,
                posicion.z
            },
            0.035f,
            0.035f,
            0.25f,
            8,
            BROWN
        );

        if (mostrarDebug)
        {
            DrawSphereWires(
                posicion,
                ObtenerRadioCuerpoProyectil(proyectil),
                10,
                10,
                YELLOW
            );
        }
    }

    DibujarParticulasTierra(particulas, cantidadParticulas);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (estadosJugadores[i].eliminado) continue;
        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);

        if (
            mostrarDebug &&
            participantes[i].activo &&
            participantes[i].conectado &&
            !jugadores[i].cayendo
        )
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    EndMode3D();

    DrawText("MINIJUEGO 8 - ISLA BAJO FUEGO", 25, 25, 30, BLACK);
    DrawText(
        "LA SOMBRA MARCA LA CAIDA. EN DEBUG SE VEN LOS DOS RADIOS DE EXPLOSION.",
        25,
        68,
        19,
        BLACK
    );

    if (fase == FASE_ISLA_FUEGO_JUGANDO)
    {
        int intensidad = (int)std::lround(
            ObtenerProgresoPartidaIsla(*this) * 100.0f
        );
        DrawText(
            TextFormat("INTENSIDAD: %d%%", intensidad),
            GetScreenWidth() - 235,
            25,
            23,
            DARKBLUE
        );
    }

    int yEstado = 100;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        const char* estadoTexto = estadosJugadores[i].eliminado
            ? "FUERA"
            : (estadosJugadores[i].impactoDirecto
                ? "VOLANDO"
                : (estadosJugadores[i].tiempoAturdido > 0.0f ? "ATURDIDO" : "EN JUEGO"));

        DrawText(
            TextFormat("J%d: %s", participantes[i].numeroJugador, estadoTexto),
            25,
            yEstado,
            18,
            estadosJugadores[i].eliminado ? DARKGRAY : participantes[i].color
        );
        yEstado += 24;
    }

    if (fase == FASE_ISLA_FUEGO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
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
        const char* texto = "YA";
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            LIME
        );
    }
    else if (fase == FASE_ISLA_FUEGO_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 330,
            GetScreenHeight() / 2 - 155,
            660,
            310,
            Fade(BLACK, 0.90f)
        );

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

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 130,
            34,
            GOLD
        );

        int y = GetScreenHeight() / 2 - 72;
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo) continue;
            DrawText(
                TextFormat(
                    "J%d  POSICION %d  %.3f s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    resultado.participantes[i].puntuacionMinijuego / 1000.0f
                ),
                GetScreenWidth() / 2 - 210,
                y,
                22,
                participantes[i].color
            );
            y += 30;
        }

        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 22) / 2,
            GetScreenHeight() / 2 + 112,
            22,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoIslaFuego::ObtenerResultado() const
{
    return resultado;
}
