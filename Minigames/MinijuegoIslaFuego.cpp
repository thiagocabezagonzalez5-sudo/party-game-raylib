#include "Minigames/MinijuegoIslaFuego.h"

#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"

#include <cmath>


static const float DURACION_PREPARACION_ISLA = 3.0f;
static const float ESCALA_DIFICULTAD_ISLA = 30.0f;
static const float DURACION_TEXTO_YA_ISLA = 0.75f;

static const float RADIO_ISLA = 5.2f;
static const float FUERZA_EXPLOSION_NORMAL = 5.8f;
static const float FUERZA_EXPLOSION_ESPECIAL = 8.6f;
static const float DURACION_ATURDIMIENTO = 0.75f;
static const float MARGEN_TRAS_ATURDIMIENTO = 1.2f;

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


static bool LanzarProyectilIsla(
    MinijuegoIslaFuego& minijuego,
    const JugadorPrueba jugadores[],
    bool especial
)
{
    float progresoAviso = ObtenerProgresoPartidaIsla(minijuego);
    float avisoEstimado = AVISO_BOMBA_INICIAL +
        (AVISO_BOMBA_FINAL - AVISO_BOMBA_INICIAL) * progresoAviso + 0.10f;
    float radioProteccion = (especial ? RADIO_EMPUJE_ESPECIAL : RADIO_EMPUJE_NORMAL) + 0.9f;
    Vector3 punto = PuntoAleatorioIsla(especial ? 0.62f : 0.84f);
    bool valido = false;

    for (int intento = 0; intento < 24 && !valido; intento++)
    {
        valido = true;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            const EstadoJugadorIslaFuego& estado = minijuego.estadosJugadores[i];

            if (
                !minijuego.resultado.participantes[i].participo ||
                estado.eliminado ||
                estado.proteccionBomba - avisoEstimado <= 0.0f
            )
            {
                continue;
            }

            if (MagnitudHorizontalIsla(
                jugadores[i].posicion.x - punto.x,
                jugadores[i].posicion.z - punto.z) < radioProteccion)
            {
                valido = false;
                break;
            }
        }

        if (!valido) punto = PuntoAleatorioIsla(especial ? 0.62f : 0.84f);
    }

    if (!valido)
    {
        return false;
    }

    minijuego.proyectil = {};
    minijuego.proyectil.activo = true;
    minijuego.proyectil.especial = especial;
    minijuego.proyectil.puntoImpacto = punto;

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
    return true;
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
            minijuego.estadosJugadores[i].proteccionBomba > 0.0f ||
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
        minijuego.estadosJugadores[i].proteccionBomba =
            minijuego.estadosJugadores[i].tiempoAturdido + MARGEN_TRAS_ATURDIMIENTO;
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
            estado.proteccionBomba > 0.0f ||
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
        { -2.7f, 1.05f, 2.7f }, { 2.7f, 1.05f, 2.7f },
        { -2.7f, 1.05f, -2.7f }, { 2.7f, 1.05f, -2.7f }
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
        bots[i] = {};
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


static float AleatorioIsla(float minimo, float maximo)
{
    return minimo + (maximo - minimo) *
        (float)GetRandomValue(0, 1000) / 1000.0f;
}


// El bot reacciona a la bomba avisada tras 0.25-0.45 s, ubica el impacto con
// 0.5-1 m de error y a veces (15%) ni la ve. Si cree estar en peligro huye
// del punto percibido; si no, deriva cerca del centro lejos del borde.
static InputMinijuegoParticipante CrearEntradaBotIsla(
    MinijuegoIslaFuego& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    EstadoBotIslaFuego& bot = minijuego.bots[indice];
    const ProyectilIslaFuego& proyectil = minijuego.proyectil;

    if (!proyectil.activo)
    {
        bot.vioProyectil = false;
        bot.retardo = 0.0f;
    }
    else if (!bot.vioProyectil)
    {
        if (bot.retardo <= 0.0f)
        {
            bot.retardo = AleatorioIsla(0.25f, 0.45f);
            bot.distraido = GetRandomValue(1, 100) <= 15;
        }

        bot.retardo -= deltaTime;
        if (bot.retardo <= 0.0f)
        {
            bot.vioProyectil = true;
            float angulo = AleatorioIsla(0.0f, 6.2832f);
            float error = AleatorioIsla(0.5f, 1.0f);
            bot.puntoPercibido = {
                proyectil.puntoImpacto.x + std::cos(angulo) * error,
                0.0f,
                proyectil.puntoImpacto.z + std::sin(angulo) * error
            };

            float radioPeligro = proyectil.especial
                ? RADIO_EMPUJE_ESPECIAL
                : RADIO_EMPUJE_NORMAL;
            float dx = jugador.posicion.x - bot.puntoPercibido.x;
            float dz = jugador.posicion.z - bot.puntoPercibido.z;
            float distancia = MagnitudHorizontalIsla(dx, dz);

            bot.destino = jugador.posicion;

            if (!bot.distraido && distancia < radioPeligro + 0.7f)
            {
                if (distancia < 0.05f)
                {
                    dx = 1.0f;
                    dz = 0.0f;
                    distancia = 1.0f;
                }

                float salida = radioPeligro + AleatorioIsla(0.3f, 1.1f);
                Vector3 destino = {
                    bot.puntoPercibido.x + dx / distancia * salida,
                    0.0f,
                    bot.puntoPercibido.z + dz / distancia * salida
                };

                float limite = RADIO_ISLA - 0.9f;
                float norma = MagnitudHorizontalIsla(destino.x, destino.z);
                if (norma > limite)
                {
                    destino.x = destino.x / norma * limite;
                    destino.z = destino.z / norma * limite;
                }
                bot.destino = destino;
            }
        }
    }

    if (proyectil.activo && bot.vioProyectil)
    {
        return CrearEntradaBotHaciaObjetivo1v3(
            jugador.posicion, bot.destino, 0.15f
        );
    }

    bot.tiempoDeriva -= deltaTime;
    if (bot.tiempoDeriva <= 0.0f)
    {
        bot.tiempoDeriva = AleatorioIsla(0.6f, 1.6f);
        float distancia = MagnitudHorizontalIsla(
            jugador.posicion.x, jugador.posicion.z
        );

        if (distancia > 3.4f || GetRandomValue(1, 100) <= 40)
            bot.destino = PuntoAleatorioIsla(0.55f);
        else
            bot.destino = jugador.posicion;
    }

    return CrearEntradaBotHaciaObjetivo1v3(
        jugador.posicion, bot.destino, 0.25f
    );
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

        if (estado.proteccionBomba > 0.0f)
        {
            estado.proteccionBomba -= deltaTime;
            if (estado.proteccionBomba < 0.0f) estado.proteccionBomba = 0.0f;
        }

        if (estado.tiempoAturdido > 0.0f)
        {
            estado.tiempoAturdido -= deltaTime;
            if (estado.tiempoAturdido < 0.0f) estado.tiempoAturdido = 0.0f;
        }

        InputMinijuegoParticipante entrada{};
        if (estado.tiempoAturdido <= 0.0f && !estado.impactoDirecto)
        {
            if (participantes[i].esBot)
                entrada = CrearEntradaBotIsla(*this, i, jugador, deltaTime);
            else
                entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }
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

            if (!LanzarProyectilIsla(*this, jugadores, especial))
            {
                tiempoHastaSiguienteDisparo = 0.15f;
            }
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

    // Sombra del punto de caida + anillo de aviso que se cierra al acercarse
    // el impacto (no revela el radio real de la explosion).
    float progreso = ObtenerProgresoProyectil(proyectil);
    float radioSombra = proyectil.especial ? 0.66f : 0.48f;
    DrawCylinder(
        centro,
        radioSombra,
        radioSombra,
        0.012f,
        32,
        Fade(proyectil.especial ? MAROON : BLACK, 0.30f + (1.0f - progreso) * 0.25f)
    );

    Color aviso = proyectil.especial
        ? Color{ 255, 60, 50, 255 }
        : Color{ 255, 170, 40, 255 };
    float radioAviso = radioSombra * (1.15f + progreso * 1.6f);
    for (int k = 0; k < 2; k++)
    {
        DrawCircle3D(
            { centro.x, 0.03f, centro.z },
            radioAviso + k * 0.035f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(aviso, 0.95f)
        );
    }

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


// Anillo de color bajo los pies y flecha sobre la cabeza para distinguir
// jugadores (el modelo compartido es oscuro).
// MODELO FUTURO: la flecha puede pasar a ser un icono de jugador del GLB.
static void DibujarIndicadorJugadorIsla(
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
    ClearBackground(Color{ 150, 208, 238, 255 });
    BeginMode3D(camara);

    // Mar con espuma alrededor de la isla (decoracion lejana, sin sombra).
    DrawCylinderEx(
        { 0.0f, -2.6f, 0.0f },
        { 0.0f, -2.5f, 0.0f },
        70.0f,
        70.0f,
        48,
        Color{ 46, 118, 170, 255 }
    );
    for (int k = 0; k < 3; k++)
    {
        float onda = (float)GetTime() * 0.5f + k * 0.33f;
        float radioOnda = RADIO_ISLA + 0.8f + (onda - std::floor(onda)) * 3.2f;
        float opacidad = 0.55f * (1.0f - (onda - std::floor(onda)));
        DrawCircle3D(
            { 0.0f, -2.48f, 0.0f },
            radioOnda,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(RAYWHITE, opacidad)
        );
    }

    // Isla flotante: roca que se afina hacia abajo y capa de tierra arriba.
    DrawCylinderEx(
        { 0.0f, -2.5f, 0.0f },
        { 0.0f, -0.30f, 0.0f },
        RADIO_ISLA * 0.38f,
        RADIO_ISLA + 0.12f,
        32,
        Color{ 96, 84, 76, 255 }
    );
    DrawCylinder(
        { 0.0f, -0.30f, 0.0f },
        RADIO_ISLA + 0.12f,
        RADIO_ISLA + 0.12f,
        0.12f,
        64,
        Color{ 72, 76, 84, 255 }
    );
    DrawCylinder(
        { 0.0f, -0.18f, 0.0f },
        RADIO_ISLA,
        RADIO_ISLA,
        0.18f,
        64,
        Color{ 188, 172, 138, 255 }
    );

    // Marcas de arena para leer distancias y el borde peligroso.
    for (int k = 1; k <= 2; k++)
    {
        DrawCircle3D(
            { 0.0f, 0.006f, 0.0f },
            RADIO_ISLA * 0.31f * k,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(Color{ 142, 124, 96, 255 }, 0.65f)
        );
    }
    DrawCircle3D(
        { 0.0f, 0.006f, 0.0f },
        RADIO_ISLA - 0.05f,
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Color{ 120, 98, 74, 255 }
    );
    DrawCircle3D(
        { 0.0f, 0.006f, 0.0f },
        RADIO_ISLA - 0.14f,
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Color{ 120, 98, 74, 255 }
    );

    // Piedras del borde (decoracion, sin estampar sombra).
    int piedras = (int)(18 * FactorCalidadGrafica(CalidadDecoracion()));
    if (piedras < 6) piedras = 6;
    for (int k = 0; k < piedras; k++)
    {
        float angulo = (2.0f * PI * k) / piedras;
        float lado = 0.20f + 0.08f * (float)(k % 3);
        DrawCubeV(
            {
                std::cos(angulo) * (RADIO_ISLA + 0.02f),
                0.05f + lado * 0.2f,
                std::sin(angulo) * (RADIO_ISLA + 0.02f)
            },
            { lado, lado * 0.7f, lado },
            k % 2 == 0 ? Color{ 128, 124, 120, 255 } : Color{ 104, 100, 98, 255 }
        );
    }

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
        DrawSphere(
            {
                posicion.x,
                posicion.y + ObtenerRadioCuerpoProyectil(proyectil) + 0.42f,
                posicion.z
            },
            0.09f + 0.03f * std::sin((float)GetTime() * 30.0f),
            ORANGE
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
        DibujarIndicadorJugadorIsla(jugadores[i], participantes[i].color);
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

    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const float e = (float)alto / 720.0f;

    DrawRectangle(0, 0, ancho, (int)(76 * e), Fade(BLACK, 0.55f));
    DrawText("ISLA BAJO FUEGO", (int)(24 * e), (int)(10 * e), (int)(30 * e), RAYWHITE);
    DrawText(
        "LA SOMBRA Y EL ANILLO MARCAN DONDE CAE LA BOMBA. QUE NO TE SAQUE DE LA ISLA",
        (int)(24 * e),
        (int)(46 * e),
        (int)(18 * e),
        Color{ 200, 224, 240, 255 }
    );

    if (fase == FASE_ISLA_FUEGO_JUGANDO)
    {
        int intensidad = (int)std::lround(
            ObtenerProgresoPartidaIsla(*this) * 100.0f
        );
        const char* textoIntensidad = TextFormat("INTENSIDAD %d%%", intensidad);
        int tamanoIntensidad = (int)(24 * e);
        DrawText(
            textoIntensidad,
            ancho - MeasureText(textoIntensidad, tamanoIntensidad) - (int)(30 * e),
            (int)(24 * e),
            tamanoIntensidad,
            Color{ 255, 190, 90, 255 }
        );
    }

    int cantidadTarjetas = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo) cantidadTarjetas++;
    }

    int anchoTarjeta = (int)(190 * e);
    int altoTarjeta = (int)(44 * e);
    int separacion = (int)(14 * e);
    int xTarjeta = ancho / 2 - (cantidadTarjetas * anchoTarjeta + (cantidadTarjetas - 1) * separacion) / 2;
    int yTarjeta = alto - altoTarjeta - (int)(18 * e);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        bool fuera = estadosJugadores[i].eliminado;
        const char* estadoTexto = fuera
            ? "FUERA"
            : (estadosJugadores[i].impactoDirecto
                ? "VOLANDO"
                : (estadosJugadores[i].tiempoAturdido > 0.0f ? "ATURDIDO" : "EN JUEGO"));

        DrawRectangle(xTarjeta, yTarjeta, anchoTarjeta, altoTarjeta, Fade(BLACK, fuera ? 0.40f : 0.62f));
        DrawRectangle(xTarjeta, yTarjeta, (int)(8 * e), altoTarjeta, fuera ? GRAY : participantes[i].color);
        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            xTarjeta + (int)(18 * e),
            yTarjeta + (int)(11 * e),
            (int)(22 * e),
            fuera ? GRAY : participantes[i].color
        );
        DrawText(
            estadoTexto,
            xTarjeta + (int)(70 * e),
            yTarjeta + (int)(14 * e),
            (int)(17 * e),
            fuera ? GRAY : RAYWHITE
        );

        xTarjeta += anchoTarjeta + separacion;
    }

    if (fase == FASE_ISLA_FUEGO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        int tamano = (int)(96 * e);
        int x = ancho / 2 - MeasureText(texto, tamano) / 2;
        int y = alto / 2 - (int)(190 * e);
        DrawText(texto, x + 4, y + 4, tamano, Fade(BLACK, 0.7f));
        DrawText(texto, x, y, tamano, ORANGE);
    }
    else if (
        fase == FASE_ISLA_FUEGO_JUGANDO &&
        tiempoJugado < DURACION_TEXTO_YA_ISLA
    )
    {
        const char* texto = "YA";
        int tamano = (int)(96 * e);
        int x = ancho / 2 - MeasureText(texto, tamano) / 2;
        int y = alto / 2 - (int)(190 * e);
        DrawText(texto, x + 4, y + 4, tamano, Fade(BLACK, 0.7f));
        DrawText(texto, x, y, tamano, LIME);
    }
    else if (fase == FASE_ISLA_FUEGO_TERMINADO)
    {
        int anchoPanel = (int)(520 * e);
        int altoPanel = (int)(220 * e);
        int xPanel = ancho / 2 - anchoPanel / 2;
        int yPanel = (int)(92 * e);
        DrawRectangle(xPanel, yPanel, anchoPanel, altoPanel, Fade(BLACK, 0.82f));

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
            ancho / 2 - MeasureText(titulo, (int)(32 * e)) / 2,
            yPanel + (int)(14 * e),
            (int)(32 * e),
            GOLD
        );

        int y = yPanel + (int)(62 * e);
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
                xPanel + (int)(100 * e),
                y,
                (int)(21 * e),
                participantes[i].color
            );
            y += (int)(27 * e);
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
MinijuegoIslaFuego::ObtenerResultado() const
{
    return resultado;
}
