#include "Minigames/MinijuegoIslaFuego.h"

#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>

// SombrasRetro.h estampa una mancha en cada DrawCube/DrawSphere/DrawCylinder.
// La escena de la isla tiene luz propia (atardecer): se usan las primitivas
// puras y las sombras que importan se dibujan a mano.
#undef DrawCube
#undef DrawSphere
#undef DrawCylinder


static const float DURACION_PREPARACION_ISLA = 3.0f;
static const float ESCALA_DIFICULTAD_ISLA = 30.0f;

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


//==================================================
// EFECTOS VISUALES (solo lectura de la logica)
//==================================================
//
// Azar propio para los efectos: no consume GetRandomValue, asi la decoracion
// nunca altera la secuencia de bombas ni las decisiones de los bots.
//==================================================

static const float NIVEL_MAR_ISLA = -2.60f;

// Barco de donde salen los morteros (fondo, detras de la isla).
static const Vector3 POSICION_BARCO_ISLA = { 11.8f, NIVEL_MAR_ISLA, -11.0f };


static float AzarVisualIsla(EfectosIslaFuego& efectos, float minimo, float maximo)
{
    efectos.semilla = efectos.semilla * 1664525u + 1013904223u;
    float u = (float)((efectos.semilla >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasIsla(int cantidad)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    int escalada = (int)std::lround((float)cantidad * factor);
    return escalada < 1 ? 1 : escalada;
}


static ParticulaIslaFuego* ReservarParticulaIsla(EfectosIslaFuego& efectos)
{
    for (int i = 0; i < MAX_PARTICULAS_ISLA; i++)
    {
        if (!efectos.particulas[i].activa) return &efectos.particulas[i];
    }
    return nullptr;
}


static void EmitirParticulaIsla(
    EfectosIslaFuego& efectos,
    TipoParticulaIslaFuego tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    ParticulaIslaFuego* p = ReservarParticulaIsla(efectos);
    if (p == nullptr) return;

    *p = {};
    p->activa = true;
    p->tipo = tipo;
    p->posicion = posicion;
    p->velocidad = velocidad;
    p->vidaMaxima = vida;
    p->vida = vida;
    p->tamano = tamano;
    p->color = color;
}


// Abanico radial: velocidad horizontal en [vMin, vMax] y subida en [subeMin, subeMax].
static Vector3 VelocidadAbanicoIsla(
    EfectosIslaFuego& efectos,
    float vMin,
    float vMax,
    float subeMin,
    float subeMax
)
{
    float angulo = AzarVisualIsla(efectos, 0.0f, 6.2832f);
    float v = AzarVisualIsla(efectos, vMin, vMax);
    return { std::cos(angulo) * v, AzarVisualIsla(efectos, subeMin, subeMax), std::sin(angulo) * v };
}


// Chispas, humo y arena levantada. La bola de fuego, el destello y el anillo
// de choque se dibujan a partir de tiempoDesdeExplosion.
static void EmitirExplosionIsla(EfectosIslaFuego& efectos, Vector3 punto, bool especial)
{
    efectos.puntoExplosion = punto;
    efectos.explosionEspecial = especial;
    efectos.tiempoDesdeExplosion = 0.0f;

    float escala = especial ? 1.45f : 1.0f;
    Vector3 base = { punto.x, 0.25f, punto.z };

    int chispas = CantidadParticulasIsla(especial ? 34 : 22);
    for (int i = 0; i < chispas; i++)
    {
        unsigned char g = (unsigned char)AzarVisualIsla(efectos, 150.0f, 235.0f);
        EmitirParticulaIsla(
            efectos, PARTICULA_ISLA_CHISPA, base,
            VelocidadAbanicoIsla(efectos, 2.0f * escala, 6.0f * escala, 2.5f, 7.0f * escala),
            AzarVisualIsla(efectos, 0.30f, 0.65f), AzarVisualIsla(efectos, 0.06f, 0.12f),
            Color{ 255, g, (unsigned char)(g / 4), 255 });
    }

    int arena = CantidadParticulasIsla(especial ? 26 : 18);
    for (int i = 0; i < arena; i++)
    {
        unsigned char tono = (unsigned char)AzarVisualIsla(efectos, 170.0f, 215.0f);
        EmitirParticulaIsla(
            efectos, PARTICULA_ISLA_ARENA, base,
            VelocidadAbanicoIsla(efectos, 1.2f * escala, 4.2f * escala, 3.0f, 6.5f),
            AzarVisualIsla(efectos, 0.55f, 0.95f), AzarVisualIsla(efectos, 0.07f, 0.15f) * escala,
            Color{ tono, (unsigned char)(tono - 22), (unsigned char)(tono - 62), 255 });
    }

    // Humo corto y translucido: sube rapido para no tapar el siguiente aviso.
    int humo = CantidadParticulasIsla(especial ? 9 : 6);
    for (int i = 0; i < humo; i++)
    {
        unsigned char tono = (unsigned char)AzarVisualIsla(efectos, 96.0f, 130.0f);
        Vector3 v = VelocidadAbanicoIsla(efectos, 0.4f, 1.4f * escala, 2.6f, 4.2f);
        EmitirParticulaIsla(
            efectos, PARTICULA_ISLA_HUMO,
            { punto.x + v.x * 0.15f, 0.55f, punto.z + v.z * 0.15f }, v,
            AzarVisualIsla(efectos, 0.40f, 0.60f), AzarVisualIsla(efectos, 0.28f, 0.42f) * escala,
            Color{ tono, (unsigned char)(tono - 6), (unsigned char)(tono - 10), 105 });
    }

    // ActivarTemblorCamaraGeneral ya respeta "reducir movimiento".
    ActivarTemblorCamaraGeneral(especial ? 0.30f : 0.17f, especial ? 0.38f : 0.24f);
}


// Golpe a un jugador: destello y chispas (mas fuertes si es directo).
static void EmitirGolpeIsla(
    EfectosIslaFuego& efectos,
    int indice,
    Vector3 posicion,
    bool directo
)
{
    efectos.tiempoDesdeGolpe[indice] = 0.0f;
    efectos.golpeDirecto[indice] = directo;

    int cantidad = CantidadParticulasIsla(directo ? 18 : 8);
    for (int i = 0; i < cantidad; i++)
    {
        EmitirParticulaIsla(
            efectos, PARTICULA_ISLA_CHISPA,
            { posicion.x, posicion.y + 0.2f, posicion.z },
            VelocidadAbanicoIsla(efectos, 1.0f, directo ? 4.5f : 2.6f, 1.5f, 4.5f),
            AzarVisualIsla(efectos, 0.25f, 0.50f), AzarVisualIsla(efectos, 0.05f, 0.09f),
            directo ? Color{ 255, 120, 60, 255 } : Color{ 255, 236, 150, 255 });
    }

    if (directo) ActivarTemblorCamaraGeneral(0.22f, 0.30f);
}


static void EmitirSalpicaduraIsla(EfectosIslaFuego& efectos, Vector3 punto, float fuerza)
{
    int gotas = CantidadParticulasIsla((int)(26 * fuerza));
    for (int i = 0; i < gotas; i++)
    {
        unsigned char b = (unsigned char)AzarVisualIsla(efectos, 215.0f, 255.0f);
        EmitirParticulaIsla(
            efectos, PARTICULA_ISLA_AGUA,
            { punto.x, NIVEL_MAR_ISLA + 0.05f, punto.z },
            VelocidadAbanicoIsla(efectos, 0.6f, 2.4f * fuerza, 3.5f, 7.5f * fuerza),
            AzarVisualIsla(efectos, 0.6f, 1.1f), AzarVisualIsla(efectos, 0.08f, 0.16f),
            Color{ (unsigned char)(b - 30), b, 255, 235 });
    }
}


// Desaparicion en el aire (impacto directo): humo y confeti del color del jugador.
static void EmitirEliminacionAireIsla(EfectosIslaFuego& efectos, Vector3 punto, Color color)
{
    int humo = CantidadParticulasIsla(10);
    for (int i = 0; i < humo; i++)
    {
        EmitirParticulaIsla(
            efectos, PARTICULA_ISLA_HUMO, punto,
            VelocidadAbanicoIsla(efectos, 0.5f, 1.8f, 0.2f, 1.4f),
            AzarVisualIsla(efectos, 0.45f, 0.70f), AzarVisualIsla(efectos, 0.26f, 0.40f),
            Color{ 225, 220, 210, 150 });
    }

    int confeti = CantidadParticulasIsla(16);
    for (int i = 0; i < confeti; i++)
    {
        EmitirParticulaIsla(
            efectos, PARTICULA_ISLA_CONFETI, punto,
            VelocidadAbanicoIsla(efectos, 1.0f, 3.5f, 1.0f, 4.0f),
            AzarVisualIsla(efectos, 0.7f, 1.2f), AzarVisualIsla(efectos, 0.07f, 0.12f),
            i % 3 == 0 ? Color{ 255, 240, 200, 255 } : color);
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

    // Fogonazo del barco en el mismo instante en que aparece el aviso (no
    // adelanta informacion: sombra y anillo ya estan visibles).
    minijuego.efectos.disparoPendiente = true;
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
    bool especial
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
    jugador.tiempoAnticipacionGolpe = 0.0f;
    jugador.tiempoAnimacionGolpe = 0.0f;

    estado.impactoDirecto = true;
    estado.tiempoHastaEliminacionDirecta = DURACION_VUELO_DIRECTO;
    estado.tiempoAturdido = DURACION_VUELO_DIRECTO;

    EmitirGolpeIsla(minijuego.efectos, indice, jugador.posicion, true);
}


static void AplicarExplosionIsla(
    MinijuegoIslaFuego& minijuego,
    JugadorPrueba jugadores[],
    Participante participantes[]
)
{
    const ProyectilIslaFuego& proyectil = minijuego.proyectil;

    EmitirExplosionIsla(minijuego.efectos, proyectil.puntoImpacto, proyectil.especial);

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
                proyectil.especial
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

        EmitirGolpeIsla(minijuego.efectos, i, jugador.posicion, false);
    }
}


static bool AplicarImpactosDirectosCaida(
    MinijuegoIslaFuego& minijuego,
    JugadorPrueba jugadores[],
    Participante participantes[]
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
            minijuego.proyectil.especial
        );
    }

    return huboContacto;
}


static void ReiniciarEfectosIsla(EfectosIslaFuego& efectos)
{
    unsigned int semilla = efectos.semilla;
    efectos = EfectosIslaFuego{};
    efectos.semilla = semilla;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectos.tiempoDesdeGolpe[i] = 99.0f;
        efectos.tiempoDesdeSalpicadura[i] = 99.0f;
        efectos.tiempoDesdeEliminacion[i] = 99.0f;
    }
}


void MinijuegoIslaFuego::Inicializar()
{
    ReiniciarEfectosIsla(efectos);

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
    ReiniciarEfectosIsla(efectos);

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


// Barco: gira para mostrar el costado de los canones hacia la isla.
static const float GIRO_BARCO_ISLA = -47.0f;


static float BalanceoBarcoIsla(float t)
{
    return 0.08f * std::sin(t * 1.4f);
}


static Vector3 PosicionBocaCanonIsla(int canon, float t)
{
    float giro = GIRO_BARCO_ISLA * DEG2RAD;
    float c = std::cos(giro);
    float s = std::sin(giro);
    Vector3 local = { -1.1f + 1.1f * (float)canon, 0.98f, 1.10f };

    return {
        POSICION_BARCO_ISLA.x + local.x * c + local.z * s,
        POSICION_BARCO_ISLA.y + BalanceoBarcoIsla(t) + local.y,
        POSICION_BARCO_ISLA.z - local.x * s + local.z * c
    };
}


static void ActualizarParticulasIsla(EfectosIslaFuego& efectos, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICULAS_ISLA; i++)
    {
        ParticulaIslaFuego& p = efectos.particulas[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float freno = 1.0f - 1.8f * deltaTime;
        if (freno < 0.0f) freno = 0.0f;

        switch (p.tipo)
        {
            case PARTICULA_ISLA_HUMO:
                p.velocidad.x *= freno;
                p.velocidad.z *= freno;
                p.velocidad.y = p.velocidad.y * (1.0f - 0.9f * deltaTime) + 0.5f * deltaTime;
                break;

            case PARTICULA_ISLA_CHISPA:
                p.velocidad.y -= 10.0f * deltaTime;
                break;

            case PARTICULA_ISLA_ARENA:
            case PARTICULA_ISLA_AGUA:
                p.velocidad.y -= 12.0f * deltaTime;
                break;

            case PARTICULA_ISLA_CONFETI:
                p.velocidad.x *= freno;
                p.velocidad.z *= freno;
                p.velocidad.y -= 4.0f * deltaTime;
                break;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        // Rebote suave sobre la arena; lo que cae al mar desaparece.
        bool sobreIsla = MagnitudHorizontalIsla(p.posicion.x, p.posicion.z) < RADIO_ISLA;
        if (
            sobreIsla && p.posicion.y < 0.04f && p.velocidad.y < 0.0f &&
            (p.tipo == PARTICULA_ISLA_ARENA || p.tipo == PARTICULA_ISLA_CHISPA ||
             p.tipo == PARTICULA_ISLA_CONFETI)
        )
        {
            p.posicion.y = 0.04f;
            p.velocidad.y *= -0.25f;
            p.velocidad.x *= 0.5f;
            p.velocidad.z *= 0.5f;
        }

        if (p.posicion.y < NIVEL_MAR_ISLA) p.activa = false;
    }
}


// Solo efectos: lee el estado de la logica y nunca lo modifica.
static void ActualizarEfectosIsla(
    MinijuegoIslaFuego& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[],
    const Participante participantes[]
)
{
    EfectosIslaFuego& e = minijuego.efectos;
    e.tiempoAnimacion += deltaTime;
    e.tiempoDesdeDisparo += deltaTime;
    e.tiempoDesdeExplosion += deltaTime;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        e.tiempoDesdeGolpe[i] += deltaTime;
        e.tiempoDesdeSalpicadura[i] += deltaTime;
        e.tiempoDesdeEliminacion[i] += deltaTime;
    }

    // Fogonazo y humo del canon que acaba de disparar.
    if (e.disparoPendiente)
    {
        e.disparoPendiente = false;
        e.canonDisparo = (e.canonDisparo + 1) % 3;
        e.tiempoDesdeDisparo = 0.0f;

        Vector3 boca = PosicionBocaCanonIsla(e.canonDisparo, e.tiempoAnimacion);
        float giro = GIRO_BARCO_ISLA * DEG2RAD;
        Vector3 frente = { std::sin(giro), 0.0f, std::cos(giro) };

        int humo = CantidadParticulasIsla(8);
        for (int i = 0; i < humo; i++)
        {
            float empuje = AzarVisualIsla(e, 0.6f, 2.0f);
            EmitirParticulaIsla(
                e, PARTICULA_ISLA_HUMO, boca,
                {
                    frente.x * empuje + AzarVisualIsla(e, -0.3f, 0.3f),
                    AzarVisualIsla(e, 1.0f, 2.4f),
                    frente.z * empuje + AzarVisualIsla(e, -0.3f, 0.3f)
                },
                AzarVisualIsla(e, 0.9f, 1.4f), AzarVisualIsla(e, 0.30f, 0.50f),
                Color{ 226, 218, 206, 170 });
        }

        int chispas = CantidadParticulasIsla(5);
        for (int i = 0; i < chispas; i++)
        {
            EmitirParticulaIsla(
                e, PARTICULA_ISLA_CHISPA, boca,
                {
                    frente.x * 2.5f + AzarVisualIsla(e, -0.8f, 0.8f),
                    AzarVisualIsla(e, 2.5f, 5.0f),
                    frente.z * 2.5f + AzarVisualIsla(e, -0.8f, 0.8f)
                },
                AzarVisualIsla(e, 0.25f, 0.45f), 0.07f, Color{ 255, 200, 90, 255 });
        }
    }

    // Estela de humo y chispas de la mecha mientras la bomba cae. El humo
    // solo sale en lo alto para no tapar la sombra ni el anillo del suelo.
    if (minijuego.proyectil.activo)
    {
        e.temporizadorEstela -= deltaTime;
        if (e.temporizadorEstela <= 0.0f)
        {
            float factor = FactorCalidadGrafica(CalidadParticulas());
            e.temporizadorEstela = 0.035f / (factor < 0.25f ? 0.25f : factor);

            Vector3 pos = ObtenerPosicionProyectil(minijuego.proyectil);
            float radio = ObtenerRadioCuerpoProyectil(minijuego.proyectil);
            Vector3 mecha = { pos.x + 0.10f, pos.y + radio + 0.34f, pos.z + 0.02f };

            EmitirParticulaIsla(
                e, PARTICULA_ISLA_CHISPA, mecha,
                { AzarVisualIsla(e, -1.0f, 1.0f), AzarVisualIsla(e, 0.5f, 2.0f), AzarVisualIsla(e, -1.0f, 1.0f) },
                AzarVisualIsla(e, 0.18f, 0.32f), AzarVisualIsla(e, 0.04f, 0.07f),
                Color{ 255, 206, 96, 255 });

            if (pos.y > 2.8f)
            {
                EmitirParticulaIsla(
                    e, PARTICULA_ISLA_HUMO,
                    { pos.x + AzarVisualIsla(e, -0.08f, 0.08f), pos.y + radio * 0.6f, pos.z + AzarVisualIsla(e, -0.08f, 0.08f) },
                    { AzarVisualIsla(e, -0.15f, 0.15f), AzarVisualIsla(e, 0.2f, 0.5f), AzarVisualIsla(e, -0.15f, 0.15f) },
                    AzarVisualIsla(e, 0.40f, 0.55f), AzarVisualIsla(e, 0.16f, 0.26f) * (minijuego.proyectil.especial ? 1.3f : 1.0f),
                    Color{ 236, 228, 214, 120 });
            }
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo) continue;

        const JugadorPrueba& jugador = jugadores[i];
        const EstadoJugadorIslaFuego& estado = minijuego.estadosJugadores[i];

        // Rastro de humo y fuego del jugador lanzado por un impacto directo.
        if (estado.impactoDirecto && !estado.eliminado)
        {
            e.temporizadorRastro[i] -= deltaTime;
            if (e.temporizadorRastro[i] <= 0.0f)
            {
                e.temporizadorRastro[i] = 0.045f;
                EmitirParticulaIsla(
                    e, PARTICULA_ISLA_HUMO, jugador.posicion,
                    { AzarVisualIsla(e, -0.2f, 0.2f), AzarVisualIsla(e, 0.2f, 0.6f), AzarVisualIsla(e, -0.2f, 0.2f) },
                    AzarVisualIsla(e, 0.28f, 0.40f), AzarVisualIsla(e, 0.14f, 0.22f),
                    Color{ 96, 90, 86, 110 });
                EmitirParticulaIsla(
                    e, PARTICULA_ISLA_CHISPA, jugador.posicion,
                    { AzarVisualIsla(e, -1.0f, 1.0f), AzarVisualIsla(e, 0.0f, 1.5f), AzarVisualIsla(e, -1.0f, 1.0f) },
                    AzarVisualIsla(e, 0.2f, 0.35f), 0.08f, Color{ 255, 150, 60, 255 });
            }
        }

        // Salpicadura al caer al mar.
        if (
            minijuego.fase != FASE_ISLA_FUEGO_PREPARACION &&
            !e.salpico[i] &&
            jugador.posicion.y < NIVEL_MAR_ISLA + 0.05f &&
            MagnitudHorizontalIsla(jugador.posicion.x, jugador.posicion.z) > RADIO_ISLA * 0.8f
        )
        {
            e.salpico[i] = true;
            e.puntoSalpicadura[i] = { jugador.posicion.x, NIVEL_MAR_ISLA, jugador.posicion.z };
            e.tiempoDesdeSalpicadura[i] = 0.0f;
            EmitirSalpicaduraIsla(e, e.puntoSalpicadura[i], 1.0f);
        }

        // Eliminacion: si desaparece en el aire, nube y confeti.
        if (estado.eliminado && !e.eliminadoVisto[i])
        {
            e.eliminadoVisto[i] = true;
            e.tiempoDesdeEliminacion[i] = 0.0f;
            e.puntoEliminacion[i] = jugador.posicion;

            if (!e.salpico[i] && jugador.posicion.y > NIVEL_MAR_ISLA + 0.5f)
            {
                EmitirEliminacionAireIsla(e, jugador.posicion, participantes[i].color);
                ActivarTemblorCamaraGeneral(0.12f, 0.20f);
            }
        }
    }

    ActualizarParticulasIsla(e, deltaTime);
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

    // Animaciones y particulas propias (tambien durante la cuenta y el final).
    ActualizarEfectosIsla(*this, deltaTime, jugadores, participantes);

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
            participantes
        );

        if (directo || proyectil.tiempoHastaImpacto <= 0.0f)
        {
            AplicarExplosionIsla(
                *this,
                jugadores,
                participantes
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


//==================================================
// VISUAL: AYUDAS DE COLOR Y GEOMETRIA
//==================================================
//
// Atardecer: la luz llega desde la izquierda y un poco desde el fondo. Las
// caras se sombrean a mano (sin luces reales) para dar volumen.
//==================================================

static const Color COLOR_ARENA_ISLA = { 218, 194, 148, 255 };
static const Color COLOR_ARENA_BORDE_ISLA = { 192, 162, 118, 255 };
static const Color COLOR_BRUMA_ISLA = { 236, 184, 146, 255 };
static const Color COLOR_CIELO_ISLA = { 104, 138, 192, 255 };
static const float LUZ_X_ISLA = -0.985f;
static const float LUZ_Z_ISLA = -0.174f;


static float Hash01Isla(int a, int b)
{
    unsigned int h = (unsigned int)(a * 73856093) ^ (unsigned int)(b * 19349663);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static Color TinteIsla(Color c, float f)
{
    float r = c.r * f;
    float g = c.g * f;
    float b = c.b * f;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, c.a };
}


static Color MezclaIsla(Color a, Color b, float t)
{
    t = Limitar01Isla(t);
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


static Color ConAlfaIsla(Color c, float alfa)
{
    c.a = (unsigned char)(255.0f * Limitar01Isla(alfa));
    return c;
}


// Brillo de una cara vertical segun su normal horizontal (nx, nz).
static float LuzCaraIsla(float nx, float nz)
{
    return 0.80f + 0.28f * (nx * LUZ_X_ISLA + nz * LUZ_Z_ISLA);
}


static void VerticeIsla(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static float SuavizarIsla(float u)
{
    u = Limitar01Isla(u);
    return 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
}


// Caja con cada cara a un brillo distinto (la luz viene de la izquierda).
static void CajaFacetadaIsla(Vector3 centro, Vector3 tamano, Color color, float brilloTapa)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TinteIsla(color, brilloTapa);
    Color frente = TinteIsla(color, 0.86f);
    Color atras = TinteIsla(color, 0.62f);
    Color derecha = TinteIsla(color, 0.66f);
    Color izquierda = TinteIsla(color, 1.06f);

    DrawTriangle3D({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, tapa);
    DrawTriangle3D({ x0, y1, z0 }, { x1, y1, z1 }, { x1, y1, z0 }, tapa);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, atras);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, atras);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, derecha);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, derecha);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, izquierda);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z1 }, { x0, y1, z0 }, izquierda);
}


// Roca irregular: bloque girado sobre el eje vertical.
static void RocaGiradaIsla(Vector3 centro, Vector3 tamano, float angulo, Color color)
{
    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, centro.z);
    rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
    CajaFacetadaIsla({ 0.0f, 0.0f, 0.0f }, tamano, color, 1.22f);
    rlPopMatrix();
}


static Color ColorRocaIsla(int semilla)
{
    float v = 0.84f + 0.30f * Hash01Isla(semilla, 3);
    Color base = Hash01Isla(semilla, 9) > 0.6f
        ? Color{ 122, 104, 92, 255 }
        : Color{ 108, 96, 90, 255 };
    return TinteIsla(base, v);
}


// Disco de luz suave (centro opaco, borde transparente) sobre un plano
// horizontal. Solo con mezcla aditiva y sin escribir profundidad.
static void ResplandorIsla(Vector3 centro, float radio, Color color)
{
    const int LADOS = 16;
    rlCheckRenderBatchLimit(LADOS * 3);
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        VerticeIsla(centro, color);
        VerticeIsla({ centro.x + std::cos(a0) * radio, centro.y, centro.z + std::sin(a0) * radio }, ConAlfaIsla(color, 0.0f));
        VerticeIsla({ centro.x + std::cos(a1) * radio, centro.y, centro.z + std::sin(a1) * radio }, ConAlfaIsla(color, 0.0f));
    }
    rlEnd();
}


static void AnilloSueloIsla(Vector3 centro, float radio, float grosor, Color color)
{
    for (int k = 0; k < 3; k++)
    {
        DrawCircle3D(centro, radio - grosor * (float)k * 0.5f, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
    }
}


//==================================================
// VISUAL: CIELO Y MAR
//==================================================

static void DibujarCieloIsla()
{
    ClearBackground(COLOR_BRUMA_ISLA);
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    DrawRectangleGradientV(0, 0, ancho, alto / 3, COLOR_CIELO_ISLA, COLOR_BRUMA_ISLA);
}


static float AlturaOlaIsla(float x, float z, float t)
{
    return 0.07f * std::sin(0.55f * x + 1.3f * t) +
        0.05f * std::sin(0.72f * z - 1.1f * t + 0.35f * x) +
        0.03f * std::sin(1.30f * (x + z) + 2.1f * t);
}


// Laguna turquesa junto a la isla -> azul profundo -> bruma en el horizonte.
static Color ColorMarIsla(float radio, float altura)
{
    const Color laguna = { 66, 184, 186, 255 };
    const Color medio = { 40, 124, 170, 255 };
    const Color profundo = { 28, 80, 140, 255 };
    const Color cresta = { 214, 240, 236, 255 };

    Color c = MezclaIsla(laguna, medio, (radio - RADIO_ISLA) / 4.0f);
    c = MezclaIsla(c, profundo, (radio - RADIO_ISLA - 4.0f) / 14.0f);
    c = MezclaIsla(c, cresta, 0.45f * Limitar01Isla((altura - 0.05f) / 0.07f));

    float bruma = Limitar01Isla((radio - 30.0f) / 70.0f);
    c = MezclaIsla(c, COLOR_BRUMA_ISLA, std::pow(bruma, 0.7f));
    return c;
}


static void DibujarMarIsla(float t)
{
    NivelCalidadGrafica calidad = CalidadEfectos();
    int anillos = calidad == CALIDAD_ALTA ? 22 : (calidad == CALIDAD_MEDIA ? 15 : 9);
    int lados = calidad == CALIDAD_ALTA ? 64 : (calidad == CALIDAD_MEDIA ? 48 : 32);
    bool animar = calidad != CALIDAD_BAJA;

    const float radioInterior = RADIO_ISLA * 0.80f;
    const float radioExterior = 110.0f;

    for (int k = 0; k < anillos; k++)
    {
        float u0 = (float)k / (float)anillos;
        float u1 = (float)(k + 1) / (float)anillos;
        float r[2] = {
            radioInterior + (radioExterior - radioInterior) * std::pow(u0, 2.2f),
            radioInterior + (radioExterior - radioInterior) * std::pow(u1, 2.2f)
        };

        rlCheckRenderBatchLimit(lados * 6);
        rlBegin(RL_TRIANGLES);

        for (int s = 0; s < lados; s++)
        {
            Vector3 v[2][2];
            Color c[2][2];

            for (int a = 0; a < 2; a++)
            {
                float angulo = 6.2832f * (float)(s + a) / (float)lados;
                float ca = std::cos(angulo);
                float sa = std::sin(angulo);

                for (int b = 0; b < 2; b++)
                {
                    float x = ca * r[b];
                    float z = sa * r[b];
                    // Calma junto al acantilado, oleaje en la laguna, plano lejos.
                    float amplitud = Limitar01Isla((r[b] - RADIO_ISLA) / 3.0f) *
                        (1.0f - Limitar01Isla((r[b] - 24.0f) / 10.0f));
                    float h = animar ? AlturaOlaIsla(x, z, t) * amplitud : 0.0f;
                    v[a][b] = { x, NIVEL_MAR_ISLA + h, z };
                    c[a][b] = ColorMarIsla(r[b], h);
                }
            }

            VerticeIsla(v[0][0], c[0][0]);
            VerticeIsla(v[0][1], c[0][1]);
            VerticeIsla(v[1][1], c[1][1]);
            VerticeIsla(v[0][0], c[0][0]);
            VerticeIsla(v[1][1], c[1][1]);
            VerticeIsla(v[1][0], c[1][0]);
        }

        rlEnd();
    }
}


// Banda de espuma que rompe contra el pie del acantilado y ondas que se alejan.
static void DibujarEspumaIsla(float t)
{
    const int LADOS = 56;
    float y = NIVEL_MAR_ISLA + 0.035f;

    rlCheckRenderBatchLimit(LADOS * 6);
    rlBegin(RL_TRIANGLES);
    for (int s = 0; s < LADOS; s++)
    {
        float a0 = 6.2832f * (float)s / (float)LADOS;
        float a1 = 6.2832f * (float)(s + 1) / (float)LADOS;
        float interior = RADIO_ISLA * 0.84f;
        float e0 = RADIO_ISLA * 0.86f + 0.75f + 0.28f * std::sin(a0 * 7.0f + t * 1.7f);
        float e1 = RADIO_ISLA * 0.86f + 0.75f + 0.28f * std::sin(a1 * 7.0f + t * 1.7f);
        Color blanco = { 238, 248, 244, 215 };
        Color nada = { 238, 248, 244, 0 };

        VerticeIsla({ std::cos(a0) * interior, y, std::sin(a0) * interior }, blanco);
        VerticeIsla({ std::cos(a0) * e0, y, std::sin(a0) * e0 }, nada);
        VerticeIsla({ std::cos(a1) * e1, y, std::sin(a1) * e1 }, nada);
        VerticeIsla({ std::cos(a0) * interior, y, std::sin(a0) * interior }, blanco);
        VerticeIsla({ std::cos(a1) * e1, y, std::sin(a1) * e1 }, nada);
        VerticeIsla({ std::cos(a1) * interior, y, std::sin(a1) * interior }, blanco);
    }
    rlEnd();

    for (int k = 0; k < 3; k++)
    {
        float onda = t * 0.45f + (float)k / 3.0f;
        float fase = onda - std::floor(onda);
        DrawCircle3D(
            { 0.0f, y + 0.01f, 0.0f },
            RADIO_ISLA + 0.6f + fase * 3.4f,
            { 1.0f, 0.0f, 0.0f }, 90.0f,
            Fade(Color{ 236, 248, 246, 255 }, 0.55f * (1.0f - fase)));
    }
}


// Destellos del sol sobre el agua (aditivo).
static void DibujarDestellosMarIsla(float t)
{
    int cantidad = CalidadEfectos() == CALIDAD_ALTA ? 44 : 24;
    for (int i = 0; i < cantidad; i++)
    {
        float angulo = Hash01Isla(i, 1) * 6.2832f;
        float radio = RADIO_ISLA + 2.0f + Hash01Isla(i, 2) * 22.0f;
        float brillo = std::sin(t * 2.3f + Hash01Isla(i, 3) * 6.2832f);
        if (brillo < 0.55f) continue;

        float intensidad = (brillo - 0.55f) / 0.45f;
        float x = std::cos(angulo) * radio;
        float z = std::sin(angulo) * radio;
        float y = NIVEL_MAR_ISLA + 0.12f;
        float largo = 0.30f * intensidad + 0.08f;
        Color c = { 255, 232, 186, (unsigned char)(200.0f * intensidad) };
        DrawTriangle3D({ x - largo, y, z }, { x, y, z + largo * 0.4f }, { x + largo, y, z }, c);
        DrawTriangle3D({ x - largo, y, z }, { x + largo, y, z }, { x, y, z - largo * 0.4f }, c);
    }
}


//==================================================
// VISUAL: ISLA
//==================================================

// Acantilado de roca facetado: labio de tierra, roca con vetas y pie mojado.
static void DibujarAcantiladoIsla()
{
    const int SEGMENTOS = CalidadDecoracion() == CALIDAD_BAJA ? 28 : 44;
    const int PERFIL = 5;
    const float alturas[PERFIL] = { 0.0f, -0.30f, -1.05f, -1.85f, -2.95f };
    const Color bandas[PERFIL - 1] = {
        { 158, 122, 84, 255 },
        { 134, 110, 94, 255 },
        { 116, 98, 90, 255 },
        { 78, 84, 74, 255 }
    };

    for (int k = 0; k < SEGMENTOS; k++)
    {
        int k1 = (k + 1) % SEGMENTOS;
        float a0 = 6.2832f * (float)k / (float)SEGMENTOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)SEGMENTOS;
        float am = (a0 + a1) * 0.5f;
        float luz = LuzCaraIsla(std::cos(am), std::sin(am)) + 0.10f * (Hash01Isla(k, 7) - 0.5f);

        Vector3 p[2][PERFIL];
        int indices[2] = { k, k1 };
        float angulos[2] = { a0, a1 };

        for (int lado = 0; lado < 2; lado++)
        {
            int idx = indices[lado];
            float radios[PERFIL] = {
                RADIO_ISLA + 0.12f,
                RADIO_ISLA + 0.20f + 0.08f * Hash01Isla(idx, 1),
                RADIO_ISLA + 0.08f + 0.26f * Hash01Isla(idx, 2),
                RADIO_ISLA - 0.08f + 0.30f * Hash01Isla(idx, 3),
                RADIO_ISLA * 0.86f + 0.30f * Hash01Isla(idx, 4)
            };
            for (int j = 0; j < PERFIL; j++)
            {
                p[lado][j] = {
                    std::cos(angulos[lado]) * radios[j],
                    alturas[j],
                    std::sin(angulos[lado]) * radios[j]
                };
            }
        }

        for (int j = 0; j < PERFIL - 1; j++)
        {
            Color c = TinteIsla(bandas[j], luz * (0.92f + 0.14f * Hash01Isla(k, 20 + j)));
            DrawTriangle3D(p[0][j], p[0][j + 1], p[1][j + 1], c);
            DrawTriangle3D(p[0][j], p[1][j + 1], p[1][j], c);
        }

        // Vetas oscuras en algunos tramos de roca.
        if (Hash01Isla(k, 30) > 0.55f)
        {
            Vector3 a = p[0][1];
            Vector3 b = p[1][2];
            DrawLine3D(
                { a.x * 1.004f, a.y - 0.05f, a.z * 1.004f },
                { (a.x + b.x) * 0.502f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.502f },
                Fade(Color{ 52, 40, 36, 255 }, 0.6f));
        }
    }
}


// Arena con leve moteado, mas clara hacia la luz y mas compacta en el borde.
// El moteado es suave para que la sombra y el anillo de aviso sigan
// destacando.
static void DibujarArenaIsla()
{
    const int ANILLOS = CalidadDecoracion() == CALIDAD_BAJA ? 4 : 8;
    const int LADOS = 48;

    for (int i = 0; i < ANILLOS; i++)
    {
        float r0 = RADIO_ISLA * (float)i / (float)ANILLOS;
        float r1 = RADIO_ISLA * (float)(i + 1) / (float)ANILLOS;

        rlCheckRenderBatchLimit(LADOS * 6);
        rlBegin(RL_TRIANGLES);
        for (int s = 0; s < LADOS; s++)
        {
            Vector3 v[2][2];
            Color c[2][2];
            for (int a = 0; a < 2; a++)
            {
                int sa = (s + a) % LADOS;
                float angulo = 6.2832f * (float)sa / (float)LADOS;
                for (int b = 0; b < 2; b++)
                {
                    float r = b == 0 ? r0 : r1;
                    int anillo = i + b;
                    float x = std::cos(angulo) * r;
                    float z = std::sin(angulo) * r;
                    float ruido = anillo == 0 ? 0.0f : (Hash01Isla(anillo, sa) - 0.5f) * 0.07f;
                    float luz = 1.0f + 0.05f * (x * LUZ_X_ISLA + z * LUZ_Z_ISLA) / RADIO_ISLA + ruido;
                    Color col = TinteIsla(COLOR_ARENA_ISLA, luz);
                    col = MezclaIsla(col, COLOR_ARENA_BORDE_ISLA, 0.7f * Limitar01Isla((r - (RADIO_ISLA - 0.8f)) / 0.8f));
                    v[a][b] = { x, 0.0f, z };
                    c[a][b] = col;
                }
            }
            VerticeIsla(v[0][0], c[0][0]);
            VerticeIsla(v[0][1], c[0][1]);
            VerticeIsla(v[1][1], c[1][1]);
            VerticeIsla(v[0][0], c[0][0]);
            VerticeIsla(v[1][1], c[1][1]);
            VerticeIsla(v[1][0], c[1][0]);
        }
        rlEnd();
    }

    // Labio del borde (tierra apisonada) entre la arena y el acantilado.
    for (int s = 0; s < LADOS; s++)
    {
        float a0 = 6.2832f * (float)s / (float)LADOS;
        float a1 = 6.2832f * (float)(s + 1) / (float)LADOS;
        float r0 = RADIO_ISLA;
        float r1 = RADIO_ISLA + 0.12f;
        Color c = TinteIsla(Color{ 172, 138, 98, 255 }, LuzCaraIsla(std::cos(a0), std::sin(a0)));
        DrawTriangle3D({ std::cos(a0) * r0, 0.0f, std::sin(a0) * r0 }, { std::cos(a0) * r1, 0.0f, std::sin(a0) * r1 },
            { std::cos(a1) * r1, 0.0f, std::sin(a1) * r1 }, c);
        DrawTriangle3D({ std::cos(a0) * r0, 0.0f, std::sin(a0) * r0 }, { std::cos(a1) * r1, 0.0f, std::sin(a1) * r1 },
            { std::cos(a1) * r0, 0.0f, std::sin(a1) * r0 }, c);
    }
}


// Marcas de distancia y del borde peligroso (ya existian) + ondulaciones
// del viento y conchitas claras. Nada oscuro ni redondo: no se confunde con
// la sombra de una bomba.
static void DibujarDetallesArenaIsla()
{
    for (int k = 1; k <= 2; k++)
    {
        DrawCircle3D({ 0.0f, 0.006f, 0.0f }, RADIO_ISLA * 0.31f * k,
            { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 150, 124, 92, 255 }, 0.55f));
    }
    DrawCircle3D({ 0.0f, 0.006f, 0.0f }, RADIO_ISLA - 0.05f,
        { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 120, 92, 66, 255 });
    DrawCircle3D({ 0.0f, 0.006f, 0.0f }, RADIO_ISLA - 0.14f,
        { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 120, 92, 66, 255 });

    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    // Ondulaciones cortas de arena.
    Color onda = Fade(Color{ 186, 158, 114, 255 }, 0.55f);
    for (int i = 0; i < 12; i++)
    {
        float angulo = Hash01Isla(i, 40) * 6.2832f;
        float radio = 0.8f + Hash01Isla(i, 41) * (RADIO_ISLA - 1.6f);
        float cx = std::cos(angulo) * radio;
        float cz = std::sin(angulo) * radio;
        float largo = 0.5f + 0.4f * Hash01Isla(i, 42);
        Vector3 previo{};
        for (int p = 0; p <= 6; p++)
        {
            float u = (float)p / 6.0f - 0.5f;
            Vector3 punto = { cx + u * largo * 2.0f, 0.008f, cz + 0.06f * std::sin(u * 9.0f) };
            if (p > 0) DrawLine3D(previo, punto, onda);
            previo = punto;
        }
    }

    // Conchitas y piedritas claras cerca del borde.
    for (int i = 0; i < 14; i++)
    {
        float angulo = Hash01Isla(i, 50) * 6.2832f;
        float radio = 3.6f + Hash01Isla(i, 51) * 1.3f;
        float lado = 0.05f + 0.05f * Hash01Isla(i, 52);
        Color c = Hash01Isla(i, 53) > 0.5f ? Color{ 246, 226, 214, 255 } : Color{ 236, 206, 188, 255 };
        CajaFacetadaIsla({ std::cos(angulo) * radio, lado * 0.3f, std::sin(angulo) * radio },
            { lado, lado * 0.6f, lado * 0.8f }, c, 1.05f);
    }
}


// Piedras del borde: bajas al frente (no tapan la arena) y mas altas al fondo.
static void DibujarRocasBordeIsla()
{
    int piedras = (int)(26 * FactorCalidadGrafica(CalidadDecoracion()));
    if (piedras < 8) piedras = 8;

    for (int k = 0; k < piedras; k++)
    {
        float angulo = 6.2832f * ((float)k + 0.35f * Hash01Isla(k, 60)) / (float)piedras;
        float s = std::sin(angulo);
        float alto = s > 0.2f
            ? 0.12f + 0.08f * Hash01Isla(k, 61)
            : 0.22f + 0.30f * Hash01Isla(k, 61);
        float ancho = 0.28f + 0.26f * Hash01Isla(k, 62);
        float radio = RADIO_ISLA + 0.12f + 0.05f * Hash01Isla(k, 63);

        RocaGiradaIsla(
            { std::cos(angulo) * radio, alto * 0.5f - 0.06f, s * radio },
            { ancho, alto, ancho * 0.8f },
            Hash01Isla(k, 64) * 90.0f,
            ColorRocaIsla(k));
    }
}


// Matas de pasto en el borde del fondo (fuera de la zona jugable).
static void DibujarPastoIsla(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    int matas = CalidadDecoracion() == CALIDAD_ALTA ? 22 : 12;
    for (int k = 0; k < matas; k++)
    {
        float angulo = 3.30f + 2.85f * (float)k / (float)(matas - 1) + 0.05f * Hash01Isla(k, 70);
        float radio = RADIO_ISLA + 0.10f;
        Vector3 base = { std::cos(angulo) * radio, -0.02f, std::sin(angulo) * radio };

        for (int h = 0; h < 4; h++)
        {
            float giro = angulo + (h - 1.5f) * 0.45f;
            float alto = 0.28f + 0.18f * Hash01Isla(k, 71 + h);
            float vaiven = 0.06f * std::sin(t * 2.2f + k * 0.7f + h);
            Vector3 punta = {
                base.x + std::cos(giro) * 0.12f + vaiven,
                base.y + alto,
                base.z + std::sin(giro) * 0.12f
            };
            Vector3 lateral = { -std::sin(giro) * 0.05f, 0.0f, std::cos(giro) * 0.05f };
            Color c = h % 2 == 0 ? Color{ 104, 152, 70, 255 } : Color{ 78, 126, 58, 255 };
            DrawTriangle3D({ base.x - lateral.x, base.y, base.z - lateral.z },
                { base.x + lateral.x, base.y, base.z + lateral.z }, punta, c);
        }
    }
}


// Palmera: tronco curvo anillado, penacho de hojas que se mece y cocos.
static void DibujarPalmeraIsla(Vector3 base, float anguloInclinacion, float alto, float t, int semilla)
{
    const int TRAMOS = 6;
    float dirX = std::cos(anguloInclinacion);
    float dirZ = std::sin(anguloInclinacion);
    float vaiven = std::sin(t * 1.25f + semilla * 1.7f);

    Vector3 previo = base;
    for (int k = 1; k <= TRAMOS; k++)
    {
        float u = (float)k / (float)TRAMOS;
        float curva = 0.9f * u * u + 0.10f * vaiven * u * u;
        Vector3 punto = { base.x + dirX * curva, base.y + alto * u, base.z + dirZ * curva };
        float r0 = 0.15f - 0.05f * (float)(k - 1) / (float)TRAMOS;
        float r1 = 0.15f - 0.05f * u;
        Color c = k % 2 == 0 ? Color{ 146, 106, 70, 255 } : Color{ 120, 86, 58, 255 };
        DrawCylinderEx(previo, punto, r0, r1, 7, c);
        previo = punto;
    }

    Vector3 copa = previo;
    const int HOJAS = 7;
    for (int h = 0; h < HOJAS; h++)
    {
        float giro = 6.2832f * (float)h / (float)HOJAS + semilla * 0.6f +
            0.06f * std::sin(t * 1.8f + h + semilla);
        float dx = std::cos(giro);
        float dz = std::sin(giro);
        float caida = 0.10f * std::sin(t * 1.6f + h * 0.9f + semilla);
        Vector3 lado = { -dz, 0.0f, dx };

        Vector3 p[4] = {
            copa,
            { copa.x + dx * 0.55f, copa.y + 0.22f, copa.z + dz * 0.55f },
            { copa.x + dx * 1.10f, copa.y + 0.02f + caida, copa.z + dz * 1.10f },
            { copa.x + dx * 1.50f, copa.y - 0.45f + caida, copa.z + dz * 1.50f }
        };
        float anchos[4] = { 0.04f, 0.26f, 0.22f, 0.02f };
        Color claro = h % 2 == 0 ? Color{ 92, 160, 72, 255 } : Color{ 80, 144, 64, 255 };
        Color oscuro = TinteIsla(claro, 0.72f);

        for (int s = 0; s < 3; s++)
        {
            Vector3 a0 = { p[s].x + lado.x * anchos[s], p[s].y, p[s].z + lado.z * anchos[s] };
            Vector3 b0 = { p[s].x - lado.x * anchos[s], p[s].y, p[s].z - lado.z * anchos[s] };
            Vector3 a1 = { p[s + 1].x + lado.x * anchos[s + 1], p[s + 1].y, p[s + 1].z + lado.z * anchos[s + 1] };
            Vector3 b1 = { p[s + 1].x - lado.x * anchos[s + 1], p[s + 1].y, p[s + 1].z - lado.z * anchos[s + 1] };
            DrawTriangle3D(p[s], a0, a1, claro);
            DrawTriangle3D(p[s], a1, p[s + 1], claro);
            DrawTriangle3D(p[s], p[s + 1], b1, oscuro);
            DrawTriangle3D(p[s], b1, b0, oscuro);
        }
    }

    for (int c = 0; c < 3; c++)
    {
        float giro = 2.1f * c + semilla;
        DrawSphere({ copa.x + std::cos(giro) * 0.14f, copa.y - 0.12f, copa.z + std::sin(giro) * 0.14f },
            0.09f, Color{ 98, 70, 44, 255 });
    }
}


// Palmeras sobre salientes del acantilado, solo en el fondo y los costados
// traseros: se inclinan hacia afuera y nunca tapan la arena.
static void DibujarPalmerasIsla(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    const float angulos[4] = { 3.42f, 4.24f, 5.10f, 5.80f };
    const float altos[4] = { 2.9f, 3.5f, 3.2f, 2.7f };
    int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 4 : 2;

    for (int i = 0; i < cantidad; i++)
    {
        int indice = cantidad == 4 ? i : i * 2 + 1;
        float a = angulos[indice];
        float radio = RADIO_ISLA + 0.48f;
        Vector3 base = { std::cos(a) * radio, -0.10f, std::sin(a) * radio };

        RocaGiradaIsla({ base.x, -0.32f, base.z }, { 0.95f, 0.50f, 0.80f }, a * 57.3f, ColorRocaIsla(80 + i));
        DibujarSombraRetroCircular({ base.x, 0.0f, base.z }, 0.35f, 0.28f, -0.05f, 0.25f, 8);
        DibujarPalmeraIsla(base, a + 0.25f * (Hash01Isla(i, 81) - 0.5f), altos[indice], t, i);
    }
}


// Rocas que asoman del agua alrededor de la isla, con espuma.
static void DibujarRocasMarIsla(float t)
{
    const int CANTIDAD = 9;
    for (int i = 0; i < CANTIDAD; i++)
    {
        float angulo = 6.2832f * ((float)i + Hash01Isla(i, 90)) / (float)CANTIDAD;
        float radio = RADIO_ISLA + 1.0f + 2.2f * Hash01Isla(i, 91);
        float tam = 0.5f + 0.8f * Hash01Isla(i, 92);
        Vector3 c = { std::cos(angulo) * radio, NIVEL_MAR_ISLA + tam * 0.18f, std::sin(angulo) * radio };

        RocaGiradaIsla(c, { tam, tam * 0.9f, tam * 0.8f }, Hash01Isla(i, 93) * 90.0f, ColorRocaIsla(100 + i));
        if (tam > 0.9f)
        {
            RocaGiradaIsla({ c.x + 0.3f, c.y - 0.1f, c.z - 0.2f }, { tam * 0.6f, tam * 0.6f, tam * 0.5f },
                Hash01Isla(i, 94) * 90.0f, ColorRocaIsla(120 + i));
        }

        float onda = std::sin(t * 2.0f + i * 1.3f) * 0.5f + 0.5f;
        DrawCircle3D({ c.x, NIVEL_MAR_ISLA + 0.06f, c.z }, tam * (0.75f + 0.25f * onda),
            { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 240, 250, 246, 255 }, 0.65f - 0.35f * onda));
    }
}


// Islotes lejanos (perspectiva aerea: se tinen con la bruma).
static void DibujarIslotesIsla(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    struct Islote { float x, z, escala, bruma; };
    const Islote islotes[3] = {
        { -15.5f, -15.0f, 1.0f, 0.30f },
        { 14.0f, -1.5f, 0.9f, 0.12f },
        { -13.5f, 1.5f, 0.70f, 0.12f }
    };

    for (int i = 0; i < 3; i++)
    {
        const Islote& isl = islotes[i];
        float e = isl.escala;
        Color roca = MezclaIsla(Color{ 120, 100, 88, 255 }, COLOR_BRUMA_ISLA, isl.bruma);
        Color arena = MezclaIsla(COLOR_ARENA_ISLA, COLOR_BRUMA_ISLA, isl.bruma);
        Color verde = MezclaIsla(Color{ 84, 140, 70, 255 }, COLOR_BRUMA_ISLA, isl.bruma);

        RocaGiradaIsla({ isl.x, NIVEL_MAR_ISLA + 0.30f * e, isl.z }, { 3.2f * e, 0.9f * e, 2.4f * e }, 12.0f + i * 20.0f, roca);
        RocaGiradaIsla({ isl.x + 0.4f * e, NIVEL_MAR_ISLA + 0.85f * e, isl.z - 0.2f * e }, { 2.2f * e, 0.35f * e, 1.7f * e }, 30.0f + i * 15.0f, arena);
        RocaGiradaIsla({ isl.x - 0.5f * e, NIVEL_MAR_ISLA + 1.05f * e, isl.z - 0.3f * e }, { 1.2f * e, 0.6f * e, 1.0f * e }, 50.0f + i * 10.0f, verde);
        DibujarPalmeraIsla({ isl.x + 0.6f * e, NIVEL_MAR_ISLA + 1.0f * e, isl.z }, 0.6f + i, 2.2f * e, t, 10 + i);

        float onda = std::sin(t * 1.5f + i) * 0.5f + 0.5f;
        DrawCircle3D({ isl.x, NIVEL_MAR_ISLA + 0.05f, isl.z }, 1.9f * e + 0.25f * onda,
            { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 240, 250, 246, 255 }, 0.5f * (1.0f - isl.bruma)));
    }
}


// Barco pirata del fondo: desde sus canones salen los morteros.
static void DibujarBarcoIsla(const EfectosIslaFuego& efectos)
{
    float t = efectos.tiempoAnimacion;
    float rolido = 3.0f * std::sin(t * 1.1f);

    rlPushMatrix();
    rlTranslatef(POSICION_BARCO_ISLA.x, POSICION_BARCO_ISLA.y + BalanceoBarcoIsla(t), POSICION_BARCO_ISLA.z);
    rlRotatef(GIRO_BARCO_ISLA, 0.0f, 1.0f, 0.0f);
    rlRotatef(rolido, 1.0f, 0.0f, 0.0f);

    // Casco, cubierta y castillo de popa.
    CajaFacetadaIsla({ 0.0f, 0.15f, 0.0f }, { 4.0f, 0.70f, 1.40f }, Color{ 96, 60, 40, 255 }, 1.2f);
    CajaFacetadaIsla({ 0.0f, 0.62f, 0.0f }, { 4.3f, 0.26f, 1.52f }, Color{ 124, 80, 50, 255 }, 1.2f);
    CajaFacetadaIsla({ 0.0f, 0.40f, 0.72f }, { 4.0f, 0.09f, 0.06f }, Color{ 214, 170, 70, 255 }, 1.0f);
    CajaFacetadaIsla({ 0.2f, 0.77f, 0.0f }, { 3.2f, 0.05f, 1.30f }, Color{ 160, 120, 76, 255 }, 1.1f);
    CajaFacetadaIsla({ -1.55f, 1.02f, 0.0f }, { 1.1f, 0.55f, 1.40f }, Color{ 112, 74, 48, 255 }, 1.2f);
    CajaFacetadaIsla({ -1.55f, 1.33f, 0.0f }, { 1.25f, 0.08f, 1.50f }, Color{ 74, 48, 32, 255 }, 1.2f);

    rlPushMatrix();
    rlTranslatef(2.15f, 0.42f, 0.0f);
    rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    CajaFacetadaIsla({ 0.0f, 0.0f, 0.0f }, { 0.95f, 0.80f, 0.95f }, Color{ 104, 66, 44, 255 }, 1.2f);
    rlPopMatrix();

    // Ventanitas de la popa.
    for (int v = 0; v < 2; v++)
    {
        CajaFacetadaIsla({ -1.85f + v * 0.55f, 1.05f, 0.71f }, { 0.20f, 0.16f, 0.03f }, Color{ 255, 206, 120, 255 }, 1.0f);
    }

    // Mastil, vela cangreja, foque y bandera.
    DrawCylinderEx({ 0.4f, 0.78f, 0.0f }, { 0.4f, 3.7f, 0.0f }, 0.07f, 0.05f, 8, Color{ 86, 60, 42, 255 });
    DrawCylinderEx({ 0.4f, 1.05f, 0.0f }, { -1.25f, 1.15f, 0.0f }, 0.04f, 0.04f, 6, Color{ 86, 60, 42, 255 });

    float hinchado = 0.18f + 0.04f * std::sin(t * 1.7f);
    Vector3 centroVela = { -0.40f, 2.05f, hinchado };
    Vector3 vela[4] = {
        { 0.36f, 1.12f, 0.0f }, { 0.36f, 3.40f, 0.0f },
        { -1.05f, 2.75f, 0.0f }, { -1.20f, 1.18f, 0.0f }
    };
    Color telaClara = { 240, 228, 204, 255 };
    Color telaOscura = { 206, 192, 168, 255 };
    for (int k = 0; k < 4; k++)
    {
        DrawTriangle3D(centroVela, vela[k], vela[(k + 1) % 4], k % 2 == 0 ? telaClara : telaOscura);
    }
    DrawTriangle3D({ 0.42f, 3.30f, 0.0f }, { 2.35f, 0.85f, 0.05f }, { 0.48f, 1.05f, 0.05f }, telaOscura);

    float flameo = 0.08f * std::sin(t * 6.0f);
    DrawTriangle3D({ 0.4f, 3.70f, 0.0f }, { 0.4f, 3.40f, 0.0f }, { -0.30f, 3.55f + flameo, 0.0f }, Color{ 36, 32, 36, 255 });
    DrawSphere({ 0.12f, 3.55f + flameo * 0.5f, 0.01f }, 0.05f, Color{ 236, 230, 220, 255 });

    // Canones asomando por el costado que mira a la isla.
    for (int c = 0; c < 3; c++)
    {
        float cx = -1.1f + 1.1f * (float)c;
        float retroceso = efectos.canonDisparo == c && efectos.tiempoDesdeDisparo < 0.25f
            ? 0.12f * (1.0f - efectos.tiempoDesdeDisparo / 0.25f)
            : 0.0f;
        DrawCylinderEx({ cx, 0.62f, 0.55f - retroceso }, { cx, 0.98f, 1.10f - retroceso }, 0.13f, 0.10f, 8,
            Color{ 46, 46, 54, 255 });
        DrawCylinderEx({ cx, 0.95f, 1.05f - retroceso }, { cx, 0.99f, 1.12f - retroceso }, 0.12f, 0.12f, 8,
            Color{ 70, 70, 80, 255 });
    }

    rlPopMatrix();

    // Estela de espuma alrededor del casco.
    float onda = std::sin(t * 1.8f) * 0.5f + 0.5f;
    DrawCircle3D({ POSICION_BARCO_ISLA.x, NIVEL_MAR_ISLA + 0.05f, POSICION_BARCO_ISLA.z }, 2.4f + 0.2f * onda,
        { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 240, 250, 246, 255 }, 0.45f));
}


// Gaviotas planeando lejos de la arena.
static void DibujarGaviotasIsla(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    for (int k = 0; k < 4; k++)
    {
        float cx = k < 2 ? -7.5f : 12.0f;
        float cz = k < 2 ? -11.0f : -7.0f;
        float giro = t * (0.35f + 0.05f * k) + k * 2.4f;
        Vector3 p = {
            cx + std::cos(giro) * 3.2f,
            5.4f + 0.4f * std::sin(t * 0.8f + k),
            cz + std::sin(giro) * 2.0f
        };
        Vector3 dir = { -std::sin(giro), 0.0f, std::cos(giro) };
        Vector3 lado = { dir.z, 0.0f, -dir.x };
        float aleteo = 0.22f * std::sin(t * 7.0f + k * 1.9f);
        Color blanco = { 246, 244, 238, 255 };

        Vector3 frente = { p.x + dir.x * 0.10f, p.y, p.z + dir.z * 0.10f };
        Vector3 atras = { p.x - dir.x * 0.10f, p.y, p.z - dir.z * 0.10f };
        Vector3 puntaA = { p.x + lado.x * 0.45f, p.y + aleteo, p.z + lado.z * 0.45f };
        Vector3 puntaB = { p.x - lado.x * 0.45f, p.y + aleteo, p.z - lado.z * 0.45f };
        DrawTriangle3D(frente, puntaA, atras, blanco);
        DrawTriangle3D(frente, atras, puntaB, blanco);
    }
}


//==================================================
// VISUAL: BOMBA, EFECTOS Y PARTICULAS
//==================================================

// Bomba de hierro con brillo falso, cincho, tapa y mecha. La especial es
// mas grande y roja (mismo tamano de cuerpo que define el golpe).
static void DibujarBombaIsla(const ProyectilIslaFuego& proyectil)
{
    Vector3 pos = ObtenerPosicionProyectil(proyectil);
    float r = ObtenerRadioCuerpoProyectil(proyectil);

    Color cuerpo = proyectil.especial ? Color{ 198, 40, 36, 255 } : Color{ 54, 56, 66, 255 };
    Color brillo = proyectil.especial ? Color{ 255, 132, 108, 255 } : Color{ 150, 156, 174, 255 };
    Color cincho = proyectil.especial ? Color{ 96, 22, 22, 255 } : Color{ 32, 32, 38, 255 };

    // La misma mancha que antes estampaba SombrasRetro bajo la bomba.
    DibujarSombraRetroCircular(pos, r * 0.92f, r * 0.72f, 0.018f, 0.27f, LadosSombraPropRetro());

    DrawSphere(pos, r, cuerpo);
    DrawSphere({ pos.x - r * 0.28f, pos.y + r * 0.30f, pos.z + r * 0.30f }, r * 0.62f, MezclaIsla(cuerpo, brillo, 0.45f));
    DrawSphere({ pos.x - r * 0.38f, pos.y + r * 0.42f, pos.z + r * 0.42f }, r * 0.30f, brillo);
    DrawCylinder({ pos.x, pos.y - r * 0.08f, pos.z }, r * 1.02f, r * 1.02f, r * 0.16f, 20, cincho);
    DrawCylinder({ pos.x, pos.y + r * 0.84f, pos.z }, r * 0.30f, r * 0.36f, r * 0.24f, 10, cincho);

    Vector3 m0 = { pos.x, pos.y + r * 1.06f, pos.z };
    Vector3 m1 = { pos.x + 0.05f, pos.y + r + 0.20f, pos.z };
    Vector3 m2 = { pos.x + 0.10f, pos.y + r + 0.34f, pos.z + 0.02f };
    DrawCylinderEx(m0, m1, 0.035f, 0.032f, 6, Color{ 156, 124, 84, 255 });
    DrawCylinderEx(m1, m2, 0.032f, 0.028f, 6, Color{ 156, 124, 84, 255 });
}


// Lo que se dibuja con mezcla aditiva: chispa de la mecha, halo de la bomba
// especial, fogonazos y resplandores.
static void DibujarBrillosBombaIsla(const ProyectilIslaFuego& proyectil, float t)
{
    Vector3 pos = ObtenerPosicionProyectil(proyectil);
    float r = ObtenerRadioCuerpoProyectil(proyectil);
    Vector3 chispa = { pos.x + 0.10f, pos.y + r + 0.40f, pos.z + 0.02f };
    float parpadeo = 0.09f + 0.03f * std::sin(t * 30.0f);

    DrawSphere(chispa, parpadeo, Color{ 255, 210, 110, 255 });
    DrawSphere(chispa, parpadeo * 2.2f, Color{ 255, 140, 50, 70 });
}


// Halo rojo pulsante de la bomba especial (mezcla normal: sigue rojo sobre
// el mar azul). Refuerza que es la bomba grande sin cambiar su tamano.
static void DibujarHaloEspecialIsla(const ProyectilIslaFuego& proyectil, float t)
{
    if (!proyectil.especial) return;

    Vector3 pos = ObtenerPosicionProyectil(proyectil);
    float r = ObtenerRadioCuerpoProyectil(proyectil);
    float pulso = 1.0f + 0.08f * std::sin(t * 14.0f);
    DrawSphere(pos, r * 1.28f * pulso, Color{ 255, 64, 40, 70 });
}


static void DibujarParticulasIsla(const EfectosIslaFuego& efectos, bool aditivas)
{
    for (int i = 0; i < MAX_PARTICULAS_ISLA; i++)
    {
        const ParticulaIslaFuego& p = efectos.particulas[i];
        if (!p.activa) continue;
        if (aditivas != (p.tipo == PARTICULA_ISLA_CHISPA)) continue;

        float u = p.vidaMaxima > 0.0f ? Limitar01Isla(p.vida / p.vidaMaxima) : 0.0f;
        float tam = p.tamano;
        float alfa = 1.0f;

        switch (p.tipo)
        {
            case PARTICULA_ISLA_HUMO:
                tam = p.tamano * (1.0f + (1.0f - u) * 0.9f);
                alfa = u * Limitar01Isla((1.0f - u) * 8.0f);
                break;
            case PARTICULA_ISLA_CHISPA:
                tam = p.tamano * (0.4f + 0.6f * u);
                alfa = u;
                break;
            default:
                tam = p.tamano * (0.5f + 0.5f * u);
                alfa = Limitar01Isla(u * 2.5f);
                break;
        }

        Color c = p.color;
        c.a = (unsigned char)((float)p.color.a * alfa);
        DrawCubeV(p.posicion, { tam, tam, tam }, c);
    }
}


// Bola de fuego y anillo de choque de la ultima explosion.
static void DibujarExplosionIsla(const EfectosIslaFuego& efectos, bool aditiva)
{
    float t = efectos.tiempoDesdeExplosion;
    Vector3 p = efectos.puntoExplosion;
    float escala = efectos.explosionEspecial ? 1.5f : 1.0f;

    if (aditiva)
    {
        if (t < 0.40f)
        {
            float u = t / 0.40f;
            float a = (1.0f - u) * (1.0f - u);
            ResplandorIsla({ p.x, 0.05f, p.z }, 2.6f * escala * (0.6f + 0.4f * u),
                Color{ 255, 150, 60, (unsigned char)(170.0f * a) });
        }
        if (t < 0.18f)
        {
            float u = t / 0.18f;
            DrawSphere({ p.x, 0.5f, p.z }, (0.6f + 0.9f * u) * escala, Color{ 255, 236, 180, (unsigned char)(150.0f * (1.0f - u)) });
        }
        return;
    }

    if (t < 0.45f)
    {
        float u = t / 0.45f;
        float radio = escala * (0.45f + 0.95f * SuavizarIsla(u));
        Vector3 centro = { p.x, 0.30f + 0.70f * u * escala, p.z };
        Color fuego = MezclaIsla(Color{ 255, 244, 196, 255 }, Color{ 255, 112, 40, 255 }, u * 1.5f);
        fuego.a = (unsigned char)(230.0f * (1.0f - u));
        Color humo = Color{ 84, 76, 72, (unsigned char)(120.0f * Limitar01Isla(u * 3.0f) * (1.0f - u)) };
        DrawSphere({ centro.x, centro.y + 0.25f * u, centro.z }, radio * 1.18f, humo);
        DrawSphere(centro, radio * (1.0f - 0.35f * u), fuego);
    }

    // Anillo de choque (claro y que se expande: no se confunde con el aviso
    // naranja, que se cierra). Termina antes de que pueda caer otra bomba.
    if (t < 0.30f)
    {
        float u = t / 0.30f;
        float radio = 0.5f + (2.2f * escala - 0.5f) * (1.0f - (1.0f - u) * (1.0f - u));
        AnilloSueloIsla({ p.x, 0.045f, p.z }, radio, 0.10f, Fade(Color{ 255, 242, 210, 255 }, 0.9f * (1.0f - u)));
    }
}


// Reacciones sobre cada jugador: destello al ser golpeado, estrellitas de
// aturdimiento, aros en el agua al caer y al quedar eliminado.
static void DibujarReaccionesJugadoresIsla(
    const MinijuegoIslaFuego& minijuego,
    const JugadorPrueba jugadores[],
    bool aditiva
)
{
    const EfectosIslaFuego& e = minijuego.efectos;
    float t = e.tiempoAnimacion;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo) continue;

        const JugadorPrueba& jugador = jugadores[i];
        const EstadoJugadorIslaFuego& estado = minijuego.estadosJugadores[i];

        if (aditiva)
        {
            if (e.tiempoDesdeGolpe[i] < 0.28f && !estado.eliminado)
            {
                float u = e.tiempoDesdeGolpe[i] / 0.28f;
                Color c = e.golpeDirecto[i] ? Color{ 255, 110, 50, 255 } : Color{ 255, 232, 170, 255 };
                c.a = (unsigned char)(150.0f * (1.0f - u));
                DrawSphere(jugador.posicion, 0.7f + 0.5f * u, c);
            }
            continue;
        }

        // Estrellitas mientras dura el aturdimiento (no en el vuelo directo).
        if (estado.tiempoAturdido > 0.0f && !estado.impactoDirecto && !estado.eliminado)
        {
            float cabeza = jugador.posicion.y - jugador.tamano.y * 0.5f + 2.05f;
            for (int k = 0; k < 3; k++)
            {
                float a = t * 7.0f + k * 2.094f;
                Vector3 c = { jugador.posicion.x + std::cos(a) * 0.38f, cabeza + 0.05f * std::sin(t * 9.0f + k),
                    jugador.posicion.z + std::sin(a) * 0.38f };
                CajaFacetadaIsla(c, { 0.11f, 0.11f, 0.11f }, Color{ 255, 226, 96, 255 }, 1.1f);
            }
        }

        // Aro de espuma donde cayo al mar.
        if (e.salpico[i] && e.tiempoDesdeSalpicadura[i] < 1.0f)
        {
            float u = e.tiempoDesdeSalpicadura[i];
            AnilloSueloIsla({ e.puntoSalpicadura[i].x, NIVEL_MAR_ISLA + 0.08f, e.puntoSalpicadura[i].z },
                0.4f + 1.6f * SuavizarIsla(u), 0.12f, Fade(Color{ 245, 252, 250, 255 }, 0.9f * (1.0f - u)));
        }

        // Aro rojo de eliminado, siempre sobre el agua (fuera de la isla) para
        // no confundirse con el aviso de la bomba especial.
        if (e.eliminadoVisto[i] && e.tiempoDesdeEliminacion[i] < 1.3f)
        {
            Vector3 punto = e.salpico[i] ? e.puntoSalpicadura[i] : e.puntoEliminacion[i];
            if (MagnitudHorizontalIsla(punto.x, punto.z) > RADIO_ISLA + 0.6f)
            {
                float u = e.tiempoDesdeEliminacion[i] / 1.3f;
                AnilloSueloIsla({ punto.x, NIVEL_MAR_ISLA + 0.10f, punto.z }, 0.5f + 1.3f * SuavizarIsla(u), 0.14f,
                    Fade(Color{ 236, 64, 52, 255 }, 0.95f * (1.0f - u)));
            }
        }
    }
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

    // Contorno oscuro fino a ambos lados del anillo: mismo radio y mismo
    // tiempo, solo mejora el contraste sobre la arena clara.
    DrawCircle3D({ centro.x, 0.028f, centro.z }, radioAviso - 0.03f,
        { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 60, 30, 10, 255 }, 0.45f));
    DrawCircle3D({ centro.x, 0.028f, centro.z }, radioAviso + 0.065f,
        { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 60, 30, 10, 255 }, 0.45f));

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
    const EfectosIslaFuego& e = efectos;
    float t = e.tiempoAnimacion;

    DibujarCieloIsla();

    // Intro de la cuenta: la camara baja y se acerca hasta su lugar fijo.
    Camera3D vista = camara;
    if (fase == FASE_ISLA_FUEGO_PREPARACION)
    {
        float u = 1.0f - tiempoPreparacion / DURACION_PREPARACION_ISLA;
        float resto = (1.0f - SuavizarIsla(u * 1.15f)) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.x += 2.5f * resto;
        vista.position.y += 3.0f * resto;
        vista.position.z += 4.0f * resto;
    }

    BeginMode3D(vista);

    // Escenario hecho a mano: triangulos sin cara trasera definida.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    // Fondo: mar, espuma, islotes, barco y gaviotas.
    DibujarMarIsla(t);
    DibujarEspumaIsla(t);
    DibujarIslotesIsla(t);
    DibujarBarcoIsla(e);
    DibujarRocasMarIsla(t);
    DibujarGaviotasIsla(t);

    // Isla: acantilado, arena, marcas y decoracion del borde.
    DibujarAcantiladoIsla();
    DibujarArenaIsla();
    DibujarDetallesArenaIsla();
    DibujarRocasBordeIsla();
    DibujarPastoIsla(t);
    DibujarPalmerasIsla(t);

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    if (proyectil.activo)
    {
        DibujarObjetivoBombaIsla(proyectil, mostrarDebug);
        DibujarBombaIsla(proyectil);

        if (mostrarDebug)
        {
            DrawSphereWires(
                ObtenerPosicionProyectil(proyectil),
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

    DibujarReaccionesJugadoresIsla(*this, jugadores, false);

    // Translucidos (humo, arena, agua, bola de fuego) sin escribir profundidad.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    DibujarParticulasIsla(e, false);
    DibujarExplosionIsla(e, false);
    if (proyectil.activo) DibujarHaloEspecialIsla(proyectil, t);
    rlDrawRenderBatchActive();

    // Luz calida falsa (aditiva): reflejo del sol, chispas, fogonazos.
    if (FactorCalidadGrafica(CalidadEfectos()) >= 0.5f)
    {
        BeginBlendMode(BLEND_ADDITIVE);
        ResplandorIsla({ -14.0f, NIVEL_MAR_ISLA + 0.10f, -6.0f }, 11.0f, Color{ 255, 186, 120, 46 });
        DibujarDestellosMarIsla(t);
        DibujarParticulasIsla(e, true);
        DibujarExplosionIsla(e, true);
        DibujarReaccionesJugadoresIsla(*this, jugadores, true);
        if (proyectil.activo) DibujarBrillosBombaIsla(proyectil, t);

        if (e.tiempoDesdeDisparo < 0.20f)
        {
            float u = e.tiempoDesdeDisparo / 0.20f;
            Vector3 boca = PosicionBocaCanonIsla(e.canonDisparo, t);
            DrawSphere(boca, 0.20f + 0.45f * u, Color{ 255, 206, 120, (unsigned char)(230.0f * (1.0f - u)) });
        }

        rlDrawRenderBatchActive();
        EndBlendMode();
    }
    else
    {
        // Calidad baja: sin resplandores, pero las chispas y la mecha se ven.
        DibujarParticulasIsla(e, true);
        if (proyectil.activo) DibujarBrillosBombaIsla(proyectil, t);
        rlDrawRenderBatchActive();
    }

    rlEnableDepthMask();

    EndMode3D();

    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "ISLA BAJO FUEGO",
        "LA SOMBRA Y EL ANILLO MARCAN DÓNDE CAE LA BOMBA. QUE NO TE SAQUE DE LA ISLA.",
        resultado
    );

    hud.fase = fase == FASE_ISLA_FUEGO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_ISLA_FUEGO_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin limite de tiempo: el cartel central mide la intensidad.
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "INTENSIDAD";
    hud.progresoMedidor = ObtenerProgresoPartidaIsla(*this);
    std::snprintf(
        hud.valorMedidor,
        sizeof(hud.valorMedidor),
        "%d%%",
        (int)std::lround(hud.progresoMedidor * 100.0f)
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const EstadoJugadorIslaFuego& estado = estadosJugadores[i];
        jugador.eliminado = estado.eliminado;

        if (estado.impactoDirecto)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "VOLANDO");
        }
        else if (estado.tiempoAturdido > 0.0f)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "ATURDIDO");
        }

        // Tiempo sobrevivido con coma decimal: "12,3 S".
        std::snprintf(
            jugador.detalleFinal,
            sizeof(jugador.detalleFinal),
            "%d,%d S",
            resultado.participantes[i].puntuacionMinijuego / 1000,
            (resultado.participantes[i].puntuacionMinijuego % 1000) / 100
        );
    }

    // Tablon final: quien sigue en pie termina con el mismo tiempo que el
    // ultimo en caer, asi que se explica el orden (no cambia el ranking).
    if (hud.fase == HUD_MINIJUEGO_FIN && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!hud.jugadores[i].visible) continue;

            const ResultadoParticipante& yo = resultado.participantes[i];

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (j == i || !hud.jugadores[j].visible) continue;

                const ResultadoParticipante& otro = resultado.participantes[j];
                if (otro.posicionFinal == yo.posicionFinal) continue;
                if (otro.puntuacionMinijuego / 100 != yo.puntuacionMinijuego / 100) continue;

                if (!estadosJugadores[i].eliminado || !estadosJugadores[j].eliminado)
                {
                    std::snprintf(
                        hud.jugadores[i].detalleDesempate,
                        sizeof(hud.jugadores[i].detalleDesempate),
                        "%s",
                        estadosJugadores[i].eliminado ? "CAYÓ" : "EN PIE"
                    );
                    hud.notaFinal = "MISMO TIEMPO: QUEDA ARRIBA QUIEN SIGUE EN PIE";
                }
                else
                {
                    // Dos caidas en la misma decima: las centesimas dan el orden.
                    std::snprintf(
                        hud.jugadores[i].detalleFinal,
                        sizeof(hud.jugadores[i].detalleFinal),
                        "%d,%02d S",
                        yo.puntuacionMinijuego / 1000,
                        (yo.puntuacionMinijuego % 1000) / 10
                    );
                }
            }
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoIslaFuego::ObtenerResultado() const
{
    return resultado;
}
