#include "Minigames/MinijuegoGuardianRuinas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HorneadoMallas.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <vector>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_GUARDIAN = 3.0f;
static const float DURACION_PARTIDA_GUARDIAN = 45.0f;
static const float PERIODO_COLUMNA = 15.0f;
static const float AVISO_COLUMNA = 2.0f;

// Plaza: el portal esta al norte (-Z), los atacantes entran por el sur.
static const float X_PLAZA = 8.6f;
static const float Z_SUR = 9.0f;
static const float Z_FONDO = -12.3f;
static const float PORTAL_MEDIO = 2.5f;
static const float Z_MIN_ATACANTE = -6.5f;
static const float ALTURA_CENTRO_JUGADOR = 0.7f;

static const float GUARDIAN_Z = -10.6f;
static const float GUARDIAN_LIMITE_X = 3.0f;
static const float GUARDIAN_VELOCIDAD = 10.0f;
static const float ESCUDO_MEDIO = 1.9f;
static const float ESCUDO_FONDO = 0.3f;
static const float ESCUDO_DISTANCIA = 1.1f;
static const float EMBESTIDA_DURACION = 0.3f;
static const float EMBESTIDA_ALCANCE = 2.2f;
static const float EMBESTIDA_RECARGA = 1.0f;

static const float RECARGA_LANZAR = 1.2f;
static const float VELOCIDAD_ORBE = 8.5f;
static const float VELOCIDAD_MAXIMA_ORBE = 20.0f;
static const float RADIO_ORBE = 0.35f;
static const float RADIO_COLUMNA = 0.6f;
static const float VIDA_ORBE = 8.0f;
static const float DURACION_ATURDIMIENTO = 1.0f;

// Visual: libera las mallas horneadas del escenario (se rehacen al dibujar).
static void LiberarMallasGR();


//==================================================
// UTILIDADES
//==================================================

static float Acotar(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float Aleatorio01()
{
    return (float)GetRandomValue(0, 10000) / 10000.0f;
}


static bool JugadorEsBot(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static float DesplazamientoEmbestida(const MinijuegoGuardianRuinas& m)
{
    if (m.embestidaTiempo <= 0.0f)
    {
        return 0.0f;
    }

    float progreso = 1.0f - m.embestidaTiempo / EMBESTIDA_DURACION;

    return EMBESTIDA_ALCANCE * std::sin(3.14159265f * progreso);
}


static float ZEscudo(const MinijuegoGuardianRuinas& m)
{
    return GUARDIAN_Z + ESCUDO_DISTANCIA + DesplazamientoEmbestida(m);
}


// Posiciones de las columnas de la plaza (logica; el dibujo las usa).
static const float POSICIONES_COLUMNAS[MAX_COLUMNAS_GUARDIAN][2] =
{
    { -5.5f, -3.0f }, { 5.5f, -3.0f },
    { -2.8f, 1.2f }, { 2.8f, 1.2f },
    { -6.2f, 4.2f }, { 6.2f, 4.2f }
};


static bool ColumnaSolida(const ColumnaGuardian& columna)
{
    return columna.estado != COLUMNA_GUARDIAN_CAIDA;
}


// Distancia del punto al segmento: sirve para saber si un tiro esta libre.
static bool SegmentoCruzaColumna(
    float x0, float z0, float x1, float z1,
    float cx, float cz, float radio
)
{
    float dx = x1 - x0;
    float dz = z1 - z0;
    float largo2 = dx * dx + dz * dz;
    float t = largo2 > 0.0001f ? ((cx - x0) * dx + (cz - z0) * dz) / largo2 : 0.0f;

    t = Acotar(t, 0.0f, 1.0f);

    float px = x0 + dx * t - cx;
    float pz = z0 + dz * t - cz;

    return px * px + pz * pz < radio * radio;
}


//==================================================
// PARTICULAS
//==================================================

static void EmitirChispas(
    MinijuegoGuardianRuinas& m,
    float x,
    float z,
    int cantidad,
    float fuerza,
    Color color
)
{
    for (int k = 0; k < cantidad; k++)
    {
        for (int i = 0; i < MAX_PARTICULAS_GUARDIAN; i++)
        {
            ParticulaTierra& particula = m.particulas[i];

            if (particula.activa)
            {
                continue;
            }

            particula.activa = true;
            particula.posicion = { x, 0.9f, z };
            particula.velocidad =
            {
                (Aleatorio01() - 0.5f) * fuerza,
                1.0f + Aleatorio01() * fuerza * 0.5f,
                (Aleatorio01() - 0.5f) * fuerza
            };
            particula.vidaMaxima = 0.35f + Aleatorio01() * 0.3f;
            particula.vida = particula.vidaMaxima;
            particula.tamano = 0.07f + Aleatorio01() * 0.06f;
            particula.color = color;
            break;
        }
    }
}


static void SonidoRebote(MinijuegoGuardianRuinas& m)
{
    if (m.tiempoSonidoRebote <= 0.0f)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
        m.tiempoSonidoRebote = 0.08f;
    }
}


//==================================================
// ORBES
//==================================================

static void LanzarOrbe(
    MinijuegoGuardianRuinas& m,
    int indiceJugador,
    const JugadorPrueba& jugador,
    float objetivoX
)
{
    float dx = objetivoX - jugador.posicion.x;
    float dz = Z_FONDO - jugador.posicion.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.1f)
    {
        return;
    }

    dx /= largo;
    dz /= largo;

    float x = jugador.posicion.x + dx * 1.0f;
    float z = jugador.posicion.z + dz * 1.0f;

    // Disparo pegado a una columna: se estrella al nacer.
    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        if (!ColumnaSolida(m.columnas[c]))
        {
            continue;
        }

        float cx = x - m.columnas[c].x;
        float cz = z - m.columnas[c].z;

        if (cx * cx + cz * cz < (RADIO_COLUMNA + RADIO_ORBE) * (RADIO_COLUMNA + RADIO_ORBE))
        {
            EmitirChispas(m, x, z, 6, 3.0f, Color{ 200, 240, 255, 255 });
            SonidoRebote(m);
            return;
        }
    }

    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        OrbeGuardian& orbe = m.orbes[k];

        if (orbe.activo)
        {
            continue;
        }

        orbe = {};
        orbe.activo = true;
        orbe.x = x;
        orbe.z = z;
        orbe.vx = dx * VELOCIDAD_ORBE;
        orbe.vz = dz * VELOCIDAD_ORBE;
        orbe.vida = VIDA_ORBE;
        orbe.dueno = indiceJugador;
        ReproducirSonidoMinijuego(m.audio, SONIDO_DISPARO);
        return;
    }
}


static void ActualizarOrbes(
    MinijuegoGuardianRuinas& m,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite
)
{
    const int subpasos = 2;
    float h = deltaTime / (float)subpasos;
    bool embistiendo = m.embestidaTiempo > 0.0f;
    float sz = ZEscudo(m);

    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        OrbeGuardian& orbe = m.orbes[k];

        if (!orbe.activo)
        {
            continue;
        }

        orbe.vida -= deltaTime;

        if (orbe.vida <= 0.0f)
        {
            orbe.activo = false;
            EmitirChispas(m, orbe.x, orbe.z, 4, 2.0f, Color{ 160, 200, 230, 255 });
            continue;
        }

        for (int p = 0; p < subpasos && orbe.activo; p++)
        {
            orbe.x += orbe.vx * h;
            orbe.z += orbe.vz * h;

            // Paredes laterales y muro sur.
            if (orbe.x > X_PLAZA - RADIO_ORBE)
            {
                orbe.x = X_PLAZA - RADIO_ORBE;
                orbe.vx = -std::fabs(orbe.vx);
                SonidoRebote(m);
            }
            else if (orbe.x < -X_PLAZA + RADIO_ORBE)
            {
                orbe.x = -X_PLAZA + RADIO_ORBE;
                orbe.vx = std::fabs(orbe.vx);
                SonidoRebote(m);
            }

            if (orbe.z > Z_SUR - RADIO_ORBE)
            {
                orbe.z = Z_SUR - RADIO_ORBE;
                orbe.vz = -std::fabs(orbe.vz);
                orbe.devuelto = false;
                SonidoRebote(m);
            }

            // Columnas.
            for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
            {
                const ColumnaGuardian& columna = m.columnas[c];

                if (!ColumnaSolida(columna))
                {
                    continue;
                }

                float dx = orbe.x - columna.x;
                float dz = orbe.z - columna.z;
                float minimo = RADIO_COLUMNA + RADIO_ORBE;
                float d2 = dx * dx + dz * dz;

                if (d2 >= minimo * minimo)
                {
                    continue;
                }

                float d = std::sqrt(d2);
                float nx = d > 0.0001f ? dx / d : 0.0f;
                float nz = d > 0.0001f ? dz / d : 1.0f;
                float producto = orbe.vx * nx + orbe.vz * nz;

                if (producto < 0.0f)
                {
                    orbe.vx -= 2.0f * producto * nx;
                    orbe.vz -= 2.0f * producto * nz;
                }

                orbe.x = columna.x + nx * (minimo + 0.01f);
                orbe.z = columna.z + nz * (minimo + 0.01f);
                EmitirChispas(m, orbe.x, orbe.z, 3, 2.5f, Color{ 190, 235, 255, 255 });
                SonidoRebote(m);
            }

            // Escudo del guardian.
            if (
                orbe.vz < 0.0f &&
                std::fabs(orbe.x - m.guardianX) < ESCUDO_MEDIO + RADIO_ORBE * 0.6f &&
                orbe.z - RADIO_ORBE < sz + ESCUDO_FONDO &&
                orbe.z + RADIO_ORBE > sz - ESCUDO_FONDO
            )
            {
                float velocidad = std::sqrt(orbe.vx * orbe.vx + orbe.vz * orbe.vz);
                velocidad *= embistiendo ? 1.6f : 1.15f;
                velocidad = Acotar(velocidad, 6.0f, VELOCIDAD_MAXIMA_ORBE);

                float vx = (orbe.x - m.guardianX) * 4.0f + orbe.vx * 0.3f;

                vx = Acotar(vx, -0.85f * velocidad, 0.85f * velocidad);
                orbe.vx = vx;
                orbe.vz = std::sqrt(velocidad * velocidad - vx * vx);
                orbe.z = sz + ESCUDO_FONDO + RADIO_ORBE + 0.02f;
                orbe.devuelto = true;
                orbe.fuerte = embistiendo;
                m.bloqueos++;
                EmitirChispas(m, orbe.x, orbe.z, embistiendo ? 10 : 5, 4.0f, embistiendo ? Color{ 255, 190, 80, 255 } : Color{ 160, 240, 255, 255 });
                SonidoRebote(m);
            }

            // Portal (gol) y muro del fondo.
            if (orbe.z - RADIO_ORBE <= Z_FONDO)
            {
                if (std::fabs(orbe.x) < PORTAL_MEDIO)
                {
                    m.goles++;
                    m.tiempoGol = 1.2f;
                    orbe.activo = false;
                    EmitirChispas(m, orbe.x, Z_FONDO, 14, 6.0f, Color{ 120, 255, 220, 255 });
                    ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
                }
                else
                {
                    orbe.z = Z_FONDO + RADIO_ORBE;
                    orbe.vz = std::fabs(orbe.vz);
                    SonidoRebote(m);
                }
            }

            // Orbes devueltos que golpean a un atacante.
            if (orbe.activo && orbe.devuelto)
            {
                for (int i = 0; i < limite; i++)
                {
                    EstadoJugadorGuardian& estado = m.estadosJugadores[i];

                    if (estado.equipo != 1 || estado.aturdimiento > 0.0f)
                    {
                        continue;
                    }

                    float dx = jugadores[i].posicion.x - orbe.x;
                    float dz = jugadores[i].posicion.z - orbe.z;

                    if (dx * dx + dz * dz < 0.85f * 0.85f)
                    {
                        estado.aturdimiento = DURACION_ATURDIMIENTO;
                        orbe.activo = false;
                        EmitirChispas(m, orbe.x, orbe.z, 8, 4.0f, Color{ 255, 150, 80, 255 });
                        SonidoRebote(m);
                        break;
                    }
                }
            }
        }
    }
}


//==================================================
// IA DE BOTS
//==================================================

// Entrada del guardian bot: se mueve hacia el orbe mas peligroso con
// retraso y algo de error.
static InputMinijuegoParticipante CrearEntradaGuardianBot(
    MinijuegoGuardianRuinas& m,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    float sz = ZEscudo(m);
    int amenaza = -1;
    float menorTiempo = 999.0f;
    float proyectado = 0.0f;

    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        const OrbeGuardian& orbe = m.orbes[k];

        if (!orbe.activo || orbe.vz >= -0.1f)
        {
            continue;
        }

        float tiempo = orbe.z > sz ? (orbe.z - sz) / -orbe.vz : 0.0f;
        float x = Acotar(orbe.x + orbe.vx * tiempo, -X_PLAZA, X_PLAZA);

        if (std::fabs(x) > PORTAL_MEDIO + 1.3f)
        {
            continue;
        }

        if (tiempo < menorTiempo)
        {
            menorTiempo = tiempo;
            amenaza = k;
            proyectado = x;
        }
    }

    m.botRetardo -= deltaTime;

    if (amenaza >= 0)
    {
        if (amenaza != m.botAmenaza)
        {
            m.botAmenaza = amenaza;
            m.botRetardo = 0.06f + 0.1f * Aleatorio01();
            m.botRuido = (Aleatorio01() - 0.5f) * 0.8f;
            m.botEmbestir = Aleatorio01() < 0.5f;
        }

        if (m.botRetardo <= 0.0f)
        {
            m.botObjetivoX = proyectado + m.botRuido;
        }

        if (m.botEmbestir && menorTiempo < 0.22f && m.embestidaRecarga <= 0.0f)
        {
            entrada.golpear = true;
            m.botEmbestir = false;
        }
    }
    else
    {
        m.botAmenaza = -1;

        // Sin peligro: se coloca frente a donde estan los atacantes.
        float suma = 0.0f;
        int cuenta = 0;

        for (int i = 0; i < limite; i++)
        {
            if (m.estadosJugadores[i].equipo == 1)
            {
                suma += jugadores[i].posicion.x;
                cuenta++;
            }
        }

        m.botObjetivoX = cuenta > 0 ? 0.35f * suma / (float)cuenta : 0.0f;
    }

    float dx = m.botObjetivoX - m.guardianX;

    entrada.izquierda = dx < -0.2f;
    entrada.derecha = dx > 0.2f;

    return entrada;
}


static bool TiroLibre(const MinijuegoGuardianRuinas& m, float x0, float z0, float objetivoX)
{
    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        if (
            ColumnaSolida(m.columnas[c]) &&
            SegmentoCruzaColumna(x0, z0, objetivoX, Z_FONDO, m.columnas[c].x, m.columnas[c].z, RADIO_COLUMNA + RADIO_ORBE + 0.1f)
        )
        {
            return false;
        }
    }

    return true;
}


// Atacante bot: se mueve a un punto, busca un angulo libre lejos del
// guardian, a veces amaga y lanza.
static InputMinijuegoParticipante CrearEntradaAtacanteBot(
    MinijuegoGuardianRuinas& m,
    EstadoJugadorGuardian& estado,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};

    estado.temporizador -= deltaTime;

    if (estado.temporizador <= 0.0f)
    {
        estado.temporizador = 1.2f + 1.8f * Aleatorio01();
        estado.objetivoX = (Aleatorio01() - 0.5f) * 13.0f;
        estado.objetivoZ = -1.0f + Aleatorio01() * 8.0f;
    }

    float dx = estado.objetivoX - jugador.posicion.x;
    float dz = estado.objetivoZ - jugador.posicion.z;

    if (estado.finta > 0.0f)
    {
        estado.finta -= deltaTime;
        entrada.izquierda = estado.direccionFinta < 0.0f;
        entrada.derecha = estado.direccionFinta > 0.0f;
    }
    else
    {
        entrada.izquierda = dx < -0.4f;
        entrada.derecha = dx > 0.4f;
        entrada.adelante = dz < -0.4f;
        entrada.atras = dz > 0.4f;
    }

    if (estado.recarga > 0.0f || estado.aturdimiento > 0.0f || estado.finta > 0.0f)
    {
        return entrada;
    }

    // Elige el mejor angulo libre: el mas alejado del guardian.
    static const float CANDIDATOS[5] = { -2.2f, -1.1f, 0.0f, 1.1f, 2.2f };
    float mejor = -1.0f;
    float mejorX = 0.0f;

    for (int k = 0; k < 5; k++)
    {
        if (!TiroLibre(m, jugador.posicion.x, jugador.posicion.z, CANDIDATOS[k]))
        {
            continue;
        }

        float puntaje = std::fabs(CANDIDATOS[k] - m.guardianX) + Aleatorio01() * 0.8f;

        if (puntaje > mejor)
        {
            mejor = puntaje;
            mejorX = CANDIDATOS[k];
        }
    }

    if (mejor < 0.0f)
    {
        // Todos los tiros estan bloqueados: cambia de lugar.
        estado.temporizador = 0.0f;
        return entrada;
    }

    if (Aleatorio01() < 0.012f)
    {
        estado.finta = 0.25f;
        estado.direccionFinta = Aleatorio01() < 0.5f ? -1.0f : 1.0f;
        return entrada;
    }

    if (Aleatorio01() > 0.05f)
    {
        return entrada;
    }

    estado.ruido = (Aleatorio01() - 0.5f) * 0.9f;
    estado.apuntarX = Acotar(mejorX + estado.ruido, -PORTAL_MEDIO, PORTAL_MEDIO);
    entrada.golpear = true;

    return entrada;
}


//==================================================
// FIN DE LA PARTIDA
//==================================================

static void FinalizarGuardian(MinijuegoGuardianRuinas& m, int equipoGanador)
{
    m.equipoGanador = equipoGanador;
    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = DESENLACE_CON_GANADOR;
    m.resultado.cantidadEquipos = 2;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = m.estadosJugadores[i].equipo == 0 ? 0 : 1;

        resultadoJugador.numeroEquipo = equipo;
        resultadoJugador.puntuacionMinijuego = equipo == 1 ? m.goles : m.bloqueos;
        resultadoJugador.puntosObtenidos = 0;
        resultadoJugador.posicionFinal = equipo == equipoGanador ? 1 : 2;
    }

    m.fase = FASE_GUARDIAN_TERMINADO;
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

void MinijuegoGuardianRuinas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        orbes[k] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_GUARDIAN; i++)
    {
        particulas[i] = {};
    }

    // Suelo de la plaza y columnas (unicos bloques con colision).
    bloques[0] = {};
    bloques[0].posicion = { 0.0f, -0.5f, -2.0f };
    bloques[0].posicionInicial = bloques[0].posicion;
    bloques[0].tamano = { 2.0f * X_PLAZA + 4.0f, 1.0f, 30.0f };
    bloques[0].color = Color{ 96, 104, 88, 255 };

    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        columnas[c] = {};
        columnas[c].x = POSICIONES_COLUMNAS[c][0];
        columnas[c].z = POSICIONES_COLUMNAS[c][1];

        bloques[c + 1] = {};
        bloques[c + 1].posicion = { columnas[c].x, 2.0f, columnas[c].z };
        bloques[c + 1].posicionInicial = bloques[c + 1].posicion;
        bloques[c + 1].tamano = { 1.2f, 4.0f, 1.2f };
        bloques[c + 1].color = Color{ 120, 128, 110, 255 };
    }

    indiceGuardian = -1;
    cantidadAtacantes = 0;
    metaOrbes = 10;
    guardianX = 0.0f;
    embestidaTiempo = 0.0f;
    embestidaRecarga = 0.0f;
    botObjetivoX = 0.0f;
    botRetardo = 0.0f;
    botRuido = 0.0f;
    botAmenaza = -1;
    botEmbestir = false;

    goles = 0;
    bloqueos = 0;
    equipoGanador = -1;
    partidaValida = false;
    proximaColumna = 0;
    tiempoSonidoRebote = 0.0f;

    camara.position = { 0.0f, 19.0f, 17.0f };
    camara.target = { 0.0f, 0.0f, -2.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_GUARDIAN_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_GUARDIAN;
    tiempoRestante = DURACION_PARTIDA_GUARDIAN;
    tiempoJugado = 0.0f;
    tiempoAnimacion = 0.0f;
    tiempoGol = 0.0f;
}


void MinijuegoGuardianRuinas::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    LiberarMallasGR();  // visual: el escenario horneado se rehace
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );
    resultado.cantidadEquipos = 2;

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            indices[cantidad++] = i;
        }
    }

    if (cantidad < 2)
    {
        for (int k = 0; k < cantidad; k++)
        {
            resultado.participantes[indices[k]].numeroEquipo = 0;
        }

        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_GUARDIAN_TERMINADO;
        return;
    }

    partidaValida = true;
    indiceGuardian = indices[GetRandomValue(0, cantidad - 1)];
    cantidadAtacantes = cantidad - 1;
    metaOrbes = cantidadAtacantes >= 3 ? 10 : (cantidadAtacantes == 2 ? 6 : 3);

    int ordenAtacante = 0;

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];
        EstadoJugadorGuardian& estado = estadosJugadores[i];

        if (i == indiceGuardian)
        {
            estado.equipo = 0;
            resultado.participantes[i].numeroEquipo = 0;
            Vector3 spawn = { 0.0f, ALTURA_CENTRO_JUGADOR, GUARDIAN_Z };

            ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
            jugadores[i].posicion = spawn;
            jugadores[i].direccionMirada = { 0.0f, 0.0f, 1.0f };
            jugadores[i].enSuelo = true;
            continue;
        }

        estado.equipo = 1;
        resultado.participantes[i].numeroEquipo = 1;
        estado.recarga = 0.4f * (float)ordenAtacante;
        estado.temporizador = 0.3f * (float)ordenAtacante;
        estado.apuntarX = 0.0f;

        float x = cantidadAtacantes == 1 ? 0.0f : -5.0f + 10.0f * (float)ordenAtacante / (float)(cantidadAtacantes - 1);
        Vector3 spawn = { x, ALTURA_CENTRO_JUGADOR + 0.1f, 6.5f };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].posicion = spawn;
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugadores[i].enSuelo = false;
        jugadores[i].cayendo = false;
        ordenAtacante++;
    }
}


//==================================================
// ACTUALIZACION
//==================================================

static void ActualizarColumnas(MinijuegoGuardianRuinas& m, float deltaTime)
{
    // Cada 15 s cae una columna; el aviso empieza 2 s antes.
    if (
        m.proximaColumna < 2 &&
        m.tiempoJugado >= PERIODO_COLUMNA * (float)(m.proximaColumna + 1) - AVISO_COLUMNA
    )
    {
        int enteras[MAX_COLUMNAS_GUARDIAN]{};
        int cantidad = 0;

        for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
        {
            if (m.columnas[c].estado == COLUMNA_GUARDIAN_ENTERA)
            {
                enteras[cantidad++] = c;
            }
        }

        if (cantidad > 0)
        {
            ColumnaGuardian& columna = m.columnas[enteras[GetRandomValue(0, cantidad - 1)]];

            columna.estado = COLUMNA_GUARDIAN_AVISO;
            columna.tiempo = AVISO_COLUMNA;
        }

        m.proximaColumna++;
    }

    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        ColumnaGuardian& columna = m.columnas[c];

        if (columna.estado == COLUMNA_GUARDIAN_AVISO)
        {
            columna.tiempo -= deltaTime;

            if (columna.tiempo <= 0.0f)
            {
                columna.estado = COLUMNA_GUARDIAN_CAIDA;
                columna.tiempo = 0.0f;
                m.bloques[c + 1].activaColision = false;
                ReproducirSonidoMinijuego(m.audio, SONIDO_EXPLOSION);
                EmitirChispas(m, columna.x, columna.z, 18, 7.0f, Color{ 190, 180, 150, 255 });
            }
        }
        else if (columna.estado == COLUMNA_GUARDIAN_CAIDA)
        {
            columna.tiempo += deltaTime;
        }
    }
}


void MinijuegoGuardianRuinas::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    if (deltaTime > 0.05f)
    {
        deltaTime = 0.05f;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    tiempoAnimacion += deltaTime;

    if (!partidaValida)
    {
        return;
    }

    bool jugando = fase == FASE_GUARDIAN_JUGANDO;

    if (fase == FASE_GUARDIAN_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_GUARDIAN_JUGANDO;
        }
    }
    else if (jugando)
    {
        float restanteAntes = tiempoRestante;
        tiempoRestante -= deltaTime;
        tiempoJugado += deltaTime;
        ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);
        ActualizarColumnas(*this, deltaTime);
    }

    if (tiempoSonidoRebote > 0.0f) tiempoSonidoRebote -= deltaTime;
    if (tiempoGol > 0.0f) tiempoGol -= deltaTime;

    // Guardian.
    if (indiceGuardian >= 0)
    {
        JugadorPrueba& guardian = jugadores[indiceGuardian];
        InputMinijuegoParticipante entrada{};

        if (jugando)
        {
            entrada = JugadorEsBot(participantes[indiceGuardian])
                ? CrearEntradaGuardianBot(*this, jugadores, limite, deltaTime)
                : LeerInputMinijuegoParticipante(participantes[indiceGuardian]);
        }

        float direccion = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);

        guardianX = Acotar(guardianX + direccion * GUARDIAN_VELOCIDAD * deltaTime, -GUARDIAN_LIMITE_X, GUARDIAN_LIMITE_X);

        if (embestidaRecarga > 0.0f) embestidaRecarga -= deltaTime;

        if (embestidaTiempo > 0.0f)
        {
            embestidaTiempo -= deltaTime;

            if (embestidaTiempo < 0.0f) embestidaTiempo = 0.0f;
        }
        else if (entrada.golpear && embestidaRecarga <= 0.0f)
        {
            embestidaTiempo = EMBESTIDA_DURACION;
            embestidaRecarga = EMBESTIDA_RECARGA;
            ReproducirSonidoMinijuego(audio, SONIDO_GOLPE);
        }

        guardian.posicion = { guardianX, ALTURA_CENTRO_JUGADOR, GUARDIAN_Z + DesplazamientoEmbestida(*this) };
        guardian.velocidad = {};
        guardian.direccionMirada = { 0.0f, 0.0f, 1.0f };
        guardian.enSuelo = true;
        guardian.cayendo = false;
    }

    // Atacantes.
    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorGuardian& estado = estadosJugadores[i];

        if (estado.equipo != 1)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        InputMinijuegoParticipante entrada{};
        bool bot = JugadorEsBot(participantes[i]);

        if (estado.recarga > 0.0f) estado.recarga -= deltaTime;

        if (estado.aturdimiento > 0.0f) estado.aturdimiento -= deltaTime;

        if (jugando)
        {
            entrada = bot
                ? CrearEntradaAtacanteBot(*this, estado, jugador, deltaTime)
                : LeerInputMinijuegoParticipante(participantes[i]);
        }

        bool quiereLanzar = entrada.golpear;
        float desvio = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);

        if (!bot)
        {
            estado.apuntarX = desvio != 0.0f
                ? desvio * 2.3f
                : Acotar(-jugador.posicion.x * 0.15f, -PORTAL_MEDIO, PORTAL_MEDIO);
        }

        entrada.golpear = false;
        entrada.saltar = false;

        if (estado.aturdimiento > 0.0f)
        {
            jugador.tiempoRalentizado = 0.1f;
            jugador.multiplicadorRalentizacion = 0.3f;
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            bloques,
            1 + MAX_COLUMNAS_GUARDIAN,
            particulas,
            MAX_PARTICULAS_GUARDIAN,
            false,
            true,
            deltaTime
        );

        jugador.posicion.x = Acotar(jugador.posicion.x, -X_PLAZA + 0.5f, X_PLAZA - 0.5f);
        jugador.posicion.z = Acotar(jugador.posicion.z, Z_MIN_ATACANTE, Z_SUR - 0.6f);

        if (quiereLanzar && jugando && estado.recarga <= 0.0f && estado.aturdimiento <= 0.0f)
        {
            estado.recarga = RECARGA_LANZAR;
            estado.lanzados++;
            LanzarOrbe(*this, i, jugador, estado.apuntarX);
        }
    }

    if (jugando)
    {
        ActualizarOrbes(*this, deltaTime, jugadores, limite);
    }

    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_GUARDIAN, deltaTime);

    if (jugando)
    {
        if (goles >= metaOrbes)
        {
            FinalizarGuardian(*this, 1);
        }
        else if (tiempoRestante <= 0.0f)
        {
            tiempoRestante = 0.0f;
            FinalizarGuardian(*this, 0);
        }
    }
}



//==================================================
// VISUAL: RUINAS EXTERIORES TOMADAS POR LA SELVA
//==================================================
//
// Mediodia en la selva. La plaza de losas es un patio de una ciudad
// antigua en lo alto de una meseta: muros bajos de sillares cubiertos de
// musgo a los lados (su cara interior es el borde que rebota los orbes),
// un pretil al sur y, al norte, el muro del templo con el portal runico
// (su vano es el arco de gol). Detras sube la piramide escalonada con su
// santuario, abrazada por las raices de una ceiba. A los costados, arboles
// gigantes con raices tabulares, lianas, helechos, estatuas caidas,
// estelas, vasijas, braseros y una cascada que cae de un risco a su poza;
// mas alla la meseta cae a un valle de selva con bruma, un rio y dos
// sierras. Rayos de sol entre las copas, motas de polen, hojas que caen,
// mariposas y loros.
//
// Lo estatico se hornea en mallas (cielo, lejania, mundo y luz de sol) y
// se rehace al reiniciar o al cambiar la calidad. Lo que se mueve
// (columnas, portal, escudo, orbes, braseros, particulas) se dibuja cada
// frame agrupado en pocos lotes de triangulos.
//
// MODELO FUTURO: portal runico, columna rota con enredadera, estatua caida
// (cabeza y cuerpo), estela tallada, vasija, brasero de bronce, ceiba con
// raices tabulares, palmera, helecho, loro, mariposa, escudo del guardian
// y orbe de energia.

static const Color CIELO_ALTO_GR = { 70, 138, 214, 255 };
static const Color CIELO_MEDIO_GR = { 132, 190, 232, 255 };
static const Color CIELO_HORIZONTE_GR = { 214, 236, 228, 255 };
static const Color NIEBLA_GR = { 176, 208, 190, 255 };       // bruma de la selva
static const Color SOL_GR = { 255, 248, 222, 255 };
static const Color PIEDRA_GR = { 156, 150, 126, 255 };       // arenisca gris
static const Color PIEDRA_CLARA_GR = { 192, 184, 156, 255 };
static const Color PIEDRA_OSCURA_GR = { 104, 104, 88, 255 };
static const Color LOSA_GR = { 146, 138, 114, 255 };
static const Color JUNTA_GR = { 70, 78, 56, 255 };
static const Color JADE_GR = { 82, 126, 104, 255 };          // losas del guardian
static const Color MUSGO_GR = { 96, 136, 60, 255 };
static const Color HOJA_GR = { 54, 116, 54, 255 };
static const Color HOJA_CLARA_GR = { 124, 172, 66, 255 };
static const Color HOJA_OSCURA_GR = { 30, 76, 46, 255 };
static const Color TRONCO_GR = { 112, 92, 72, 255 };
static const Color TIERRA_GR = { 92, 80, 56, 255 };
static const Color BRONCE_GR = { 200, 150, 72, 255 };
static const Color TERRACOTA_GR = { 172, 96, 62, 255 };
static const Color RUNA_GR = { 96, 248, 222, 255 };          // acento: energia del portal
static const Color RUNA_CLARA_GR = { 214, 255, 246, 255 };
static const Color ESCUDO_GR = { 255, 214, 110, 255 };       // energia del guardian
static const Color DEVUELTO_GR = { 255, 128, 56, 255 };
static const Color AVISO_GR = { 255, 84, 58, 255 };
static const Color FUEGO_GR = { 255, 160, 64, 255 };

// Ojo de referencia para la bruma horneada y las tiras fijas.
static const Vector3 OJO_GR = { 0.0f, 17.0f, 18.6f };

// Sol alto, a la izquierda y detras del templo: los rayos cruzan la plaza
// hacia la camara.
static const Vector3 SOL_DIR_GR = { -0.36f, 0.84f, -0.41f };

// Bordes dibujados (coinciden con la logica).
static const float MURO_GROSOR_GR = 0.9f;
static const float PORTAL_JAMBA_GR = 1.0f;
static const float PORTAL_ALTO_GR = 5.4f;
static const float Z_MURO_TEMPLO_GR = Z_FONDO - 1.1f;   // espalda del muro norte


static float Ruido01GR(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float Lim01GR(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static float FraccionGR(float v)
{
    return v - std::floor(v);
}


static unsigned char CanalGR(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoGR(Color c, float f)
{
    return { CanalGR((float)c.r * f), CanalGR((float)c.g * f), CanalGR((float)c.b * f), c.a };
}


static Color MezclaGR(Color a, Color b, float t)
{
    t = Lim01GR(t);
    return {
        CanalGR((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalGR((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalGR((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalGR((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaGR(Color c, float alfa)
{
    c.a = CanalGR((float)c.a * Lim01GR(alfa));
    return c;
}


static Color VariarGR(Color c, int indice, int semilla, float amplitud)
{
    return TonoGR(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01GR(indice, semilla));
}


//--------------------------------------------------
// Primitivas: lotes de triangulos o grabacion para hornear
//--------------------------------------------------

static GrabadorMalla g_grabGR;
static Vector3 g_ojoGR = OJO_GR;
static Vector3 g_derGR = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaGR = { 0.0f, 1.0f, 0.0f };
static float g_emisGR = 0.0f;              // 1 = luminoso o cielo: sin bruma


// Bruma verde agua: lo lejano se funde con el aire humedo de la selva.
static Color NieblaGR(Color c, Vector3 p)
{
    float dx = p.x - g_ojoGR.x, dy = p.y - g_ojoGR.y, dz = p.z - g_ojoGR.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01GR((d - 40.0f) / 200.0f);
    n = n * (2.0f - n) * 0.74f * (1.0f - g_emisGR);
    Color bruma = MezclaGR(NIEBLA_GR, CIELO_HORIZONTE_GR, Lim01GR((p.y + 20.0f) / 60.0f));
    Color r = MezclaGR(c, bruma, n);
    r.a = c.a;
    return r;
}


static void VGR(Vector3 p, Color c)
{
    if (g_grabGR.activa)
    {
        GrabarVerticeMalla(g_grabGR, p, c);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteGR()
{
    if (!g_grabGR.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteGR()
{
    if (!g_grabGR.activa) rlEnd();
}


static void TriGR(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VGR(a, NieblaGR(color, a));
    VGR(b, NieblaGR(color, b));
    VGR(c, NieblaGR(color, c));
}


static void QuadGR(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriGR(a, b, c, color);
    TriGR(a, c, d, color);
}


static void Quad4GR(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VGR(a, NieblaGR(ca, a)); VGR(b, NieblaGR(cb, b)); VGR(c, NieblaGR(cc, c));
    VGR(a, NieblaGR(ca, a)); VGR(c, NieblaGR(cc, c)); VGR(d, NieblaGR(cd, d));
}


// Quad sin bruma con un color por vertice (luces aditivas y sombras).
static void QuadColGR(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VGR(a, ca); VGR(b, cb); VGR(c, cc);
    VGR(a, ca); VGR(c, cc); VGR(d, cd);
}


// Sombreado por caras de la guia (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62) interpolado para normales cualesquiera.
static float LuzGR(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f) return 1.0f;

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.5f) +
        (n.z > 0.0f ? az * 0.78f : az * 0.6f) +
        (n.x < 0.0f ? ax * 0.70f : ax * 0.62f);

    return f / suma;
}


// Luz del mediodia: lo que mira al sol toma un dorado tibio; lo que queda
// de espaldas, el verde fresco de la sombra de la selva.
static Color CaraGR(Color c, Vector3 n)
{
    float f = LuzGR(n);
    float sol = Lim01GR(0.35f + 0.65f * (n.x * SOL_DIR_GR.x + n.y * SOL_DIR_GR.y + n.z * SOL_DIR_GR.z));
    float r = f * (0.86f + 0.2f * sol);
    float g = f * (0.92f + 0.12f * sol);
    float b = f * (0.92f + 0.02f * sol);
    return { CanalGR((float)c.r * r), CanalGR((float)c.g * g), CanalGR((float)c.b * b), c.a };
}


// Caja con medio-ejes arbitrarios: caras sombreadas por normal.
static void CajaEjesGR(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
{
    auto P = [&](float sx, float sy, float sz)
    {
        return Vector3{
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    };

    Vector3 nx = Vector3Normalize(ex), ny = Vector3Normalize(ey), nz = Vector3Normalize(ez);

    QuadGR(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraGR(color, ny));
    QuadGR(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraGR(color, nz));
    QuadGR(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraGR(color, Vector3Negate(nz)));
    QuadGR(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraGR(color, Vector3Negate(nx)));
    QuadGR(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraGR(color, nx));

    if (fondo)
    {
        QuadGR(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraGR(color, Vector3Negate(ny)));
    }
}


static void CajaGR(Vector3 c, Vector3 t, Color color)
{
    CajaEjesGR(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, false);
}


// Caja girada en Y (grados) y, opcionalmente, ladeada en X (grados).
static void CajaGiradaGR(Vector3 c, Vector3 t, float giroY, Color color, float ladeo = 0.0f)
{
    float a = giroY * DEG2RAD, b = ladeo * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = { 0.0f, 1.0f, 0.0f };
    Vector3 uy2 = Vector3Add(Vector3Scale(uy, std::cos(b)), Vector3Scale(uz, std::sin(b)));
    Vector3 uz2 = Vector3Subtract(Vector3Scale(uz, std::cos(b)), Vector3Scale(uy, std::sin(b)));
    CajaEjesGR(c, Vector3Scale(ux, t.x * 0.5f), Vector3Scale(uy2, t.y * 0.5f), Vector3Scale(uz2, t.z * 0.5f), color, false);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas.
static void CilGR(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    eje = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, ref));
    Vector3 v = Vector3CrossProduct(eje, u);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = 0.5f * (a0 + a1);
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));

        QuadGR(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraGR(color, nm)
        );

        if (tapas)
        {
            TriGR(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraGR(color, eje));
            TriGR(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraGR(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide (o media cupula con desde = 0) con caras sombreadas.
static void BolaGR(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float desde = -0.5f * PI)
{
    for (int i = 0; i < anillos; i++)
    {
        float la0 = desde + (0.5f * PI - desde) * (float)i / (float)anillos;
        float la1 = desde + (0.5f * PI - desde) * (float)(i + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float lo0 = 2.0f * PI * (float)k / (float)lados;
            float lo1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float lam = 0.5f * (la0 + la1), lom = 0.5f * (lo0 + lo1);
            Vector3 n = { std::cos(lam) * std::cos(lom), std::sin(lam), std::cos(lam) * std::sin(lom) };
            auto P = [&](float la, float lo)
            {
                return Vector3{ c.x + r.x * std::cos(la) * std::cos(lo), c.y + r.y * std::sin(la), c.z + r.z * std::cos(la) * std::sin(lo) };
            };
            QuadGR(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraGR(color, n));
        }
    }
}


// Disco horizontal con color de centro y de borde (sin bruma si se pide).
static void DiscoGR(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, bool bruma = true)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };

        if (bruma)
        {
            VGR(c, NieblaGR(centro, c)); VGR(p1, NieblaGR(borde, p1)); VGR(p0, NieblaGR(borde, p0));
        }
        else
        {
            VGR(c, centro); VGR(p1, borde); VGR(p0, borde);
        }
    }
}


// Anillo horizontal entre r0 y r1 (sin bruma: marcas y luces).
static void AnilloGR(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera, float desde = 0.0f, float hasta = 1.0f)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (desde + (hasta - desde) * (float)k / (float)lados);
        float a1 = 2.0f * PI * (desde + (hasta - desde) * (float)(k + 1) / (float)lados);
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColGR(i0, i1, o1, o0, dentro, dentro, fuera, fuera);
    }
}


// Disco que mira a la camara con centro y borde de distinto color (halos).
static void HaloGR(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derGR, std::cos(a0) * radio), Vector3Scale(g_arribaGR, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derGR, std::cos(a1) * radio), Vector3Scale(g_arribaGR, std::sin(a1) * radio)));
        VGR(c, centro); VGR(p0, borde); VGR(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas), girado un angulo.
static void PuntoGR(Vector3 c, float r, Color color, float giro = 0.0f)
{
    float cg = std::cos(giro), sg = std::sin(giro);
    Vector3 d = Vector3Add(Vector3Scale(g_derGR, r * cg), Vector3Scale(g_arribaGR, r * sg));
    Vector3 u = Vector3Add(Vector3Scale(g_derGR, -r * sg), Vector3Scale(g_arribaGR, r * cg));
    QuadColGR(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Tira que mira a la camara entre dos puntos (haces, estelas).
static void TiraGR(Vector3 a, Vector3 b, float anchoA, float anchoB, Color ca, Color cb)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 haciaOjo = Vector3Subtract(g_ojoGR, a);
    Vector3 lado = Vector3CrossProduct(eje, haciaOjo);
    float largo = Vector3Length(lado);

    if (largo < 0.00001f) return;

    lado = Vector3Scale(lado, 1.0f / largo);
    Vector3 la = Vector3Scale(lado, anchoA * 0.5f), lb = Vector3Scale(lado, anchoB * 0.5f);
    QuadColGR(Vector3Subtract(a, la), Vector3Add(a, la), Vector3Add(b, lb), Vector3Subtract(b, lb), ca, ca, cb, cb);
}


// Tira horneada hacia el ojo de referencia, con bruma (lianas, tallos).
static void LianaGR(Vector3 a, Vector3 b, float ancho, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 haciaOjo = Vector3Subtract(OJO_GR, a);
    Vector3 lado = Vector3CrossProduct(eje, haciaOjo);
    float largo = Vector3Length(lado);

    if (largo < 0.00001f) return;

    lado = Vector3Scale(lado, ancho * 0.5f / largo);
    QuadGR(Vector3Subtract(a, lado), Vector3Add(a, lado), Vector3Add(b, lado), Vector3Subtract(b, lado), color);
}


// Rectangulo horizontal (x0..x1, z0..z1) a la altura y.
static void PisoGR(float x0, float z0, float x1, float z1, float y, Color c)
{
    QuadGR({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, c);
}


// Hoja lanceolada (rombo alargado con nervio) de la base a la punta.
static void HojaGR(Vector3 base, Vector3 punta, float ancho, Color color)
{
    Vector3 eje = Vector3Subtract(punta, base);
    Vector3 lado = Vector3CrossProduct(eje, { 0.0f, 1.0f, 0.0f });
    float largo = Vector3Length(lado);

    if (largo < 0.0001f) lado = { ancho * 0.5f, 0.0f, 0.0f };
    else lado = Vector3Scale(lado, ancho * 0.5f / largo);

    Vector3 medio = Vector3Add(base, Vector3Scale(eje, 0.42f));
    medio.y += Vector3Length(eje) * 0.12f;
    Color claro = TonoGR(color, 1.12f), oscuro = TonoGR(color, 0.82f);
    TriGR(base, Vector3Add(medio, lado), medio, claro);
    TriGR(medio, Vector3Add(medio, lado), punta, claro);
    TriGR(base, medio, Vector3Subtract(medio, lado), oscuro);
    TriGR(medio, punta, Vector3Subtract(medio, lado), oscuro);
}


//--------------------------------------------------
// Horno: lo estatico se graba una vez en mallas
//--------------------------------------------------

struct HornoGR
{
    bool listo = false;
    int clave = -1;
    Mesh cielo{};          // cupula y nubes, relativos al ojo
    bool cieloValido = false;
    Mesh lejos{};          // valle, sierras y cascada
    bool lejosValido = false;
    Mesh mundo{};          // plaza, muros, templo, selva y utileria
    bool mundoValido = false;
    Mesh luz{};            // rayos de sol y manchas de luz (aditivo)
    bool luzValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoGR g_hornoGR;


static void LiberarMallasGR()
{
    LiberarMallaHorneada(g_hornoGR.cielo, g_hornoGR.cieloValido);
    LiberarMallaHorneada(g_hornoGR.lejos, g_hornoGR.lejosValido);
    LiberarMallaHorneada(g_hornoGR.mundo, g_hornoGR.mundoValido);
    LiberarMallaHorneada(g_hornoGR.luz, g_hornoGR.luzValida);
    g_hornoGR.listo = false;
    g_hornoGR.clave = -1;
}


static void CieloEstaticoGR(bool decoracion);
static void LejaniaEstaticaGR(bool decoracion, int paso);
static void MundoEstaticoGR(bool decoracion, int paso);
static void LuzEstaticaGR(bool decoracion);


static void HornearGR()
{
    int clave = (int)CalidadDecoracion() * 4 + (int)CalidadEfectos();

    if ((g_hornoGR.listo && g_hornoGR.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasGR();

    AsegurarMaterialHorneado(g_hornoGR.material, g_hornoGR.materialListo);

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    Vector3 ojo = g_ojoGR;

    // El cielo se graba relativo al ojo y sin bruma.
    g_ojoGR = { 0.0f, 0.0f, 0.0f };
    g_emisGR = 1.0f;
    EmpezarGrabacionMalla(g_grabGR);
    CieloEstaticoGR(decoracion);
    g_hornoGR.cieloValido = TerminarGrabacionMalla(g_grabGR, g_hornoGR.cielo);
    g_emisGR = 0.0f;
    g_ojoGR = OJO_GR;

    EmpezarGrabacionMalla(g_grabGR);
    LejaniaEstaticaGR(decoracion, paso);
    g_hornoGR.lejosValido = TerminarGrabacionMalla(g_grabGR, g_hornoGR.lejos);

    EmpezarGrabacionMalla(g_grabGR);
    MundoEstaticoGR(decoracion, paso);
    g_hornoGR.mundoValido = TerminarGrabacionMalla(g_grabGR, g_hornoGR.mundo);

    if (CalidadEfectos() != CALIDAD_BAJA)
    {
        EmpezarGrabacionMalla(g_grabGR);
        LuzEstaticaGR(decoracion);
        g_hornoGR.luzValida = TerminarGrabacionMalla(g_grabGR, g_hornoGR.luz);
    }

    g_ojoGR = ojo;
    g_hornoGR.clave = clave;
    g_hornoGR.listo = true;
}


static void DibujarMallaGR(const Mesh& malla, bool valida, Matrix transformacion = MatrixIdentity(), Color tinte = WHITE)
{
    if (!g_hornoGR.listo || !valida) return;

    rlDrawRenderBatchActive();
    g_hornoGR.material.maps[MATERIAL_MAP_DIFFUSE].color = tinte;
    DrawMesh(malla, g_hornoGR.material, transformacion);
    g_hornoGR.material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    rlDrawRenderBatchActive();
}


//--------------------------------------------------
// Cielo del mediodia (cupula horneada relativa al ojo)
//--------------------------------------------------

static const float RADIO_CIELO_GR = 460.0f;


static Vector3 DireccionGR(float azimutGrados, float elevacionGrados)
{
    float a = azimutGrados * DEG2RAD, e = elevacionGrados * DEG2RAD;
    return { std::cos(e) * std::sin(a), std::sin(e), -std::cos(e) * std::cos(a) };
}


static Color ColorCieloGR(Vector3 d)
{
    float el = std::asin(d.y < -1.0f ? -1.0f : (d.y > 1.0f ? 1.0f : d.y)) * RAD2DEG;
    Color c;

    // Bajo el horizonte: la bruma luminosa del valle.
    if (el < 0.0f) c = MezclaGR(CIELO_HORIZONTE_GR, NIEBLA_GR, Lim01GR(-el / 14.0f));
    else if (el < 10.0f) c = MezclaGR(CIELO_HORIZONTE_GR, CIELO_MEDIO_GR, el / 10.0f);
    else c = MezclaGR(CIELO_MEDIO_GR, CIELO_ALTO_GR, Lim01GR((el - 10.0f) / 50.0f));

    float s = Vector3DotProduct(d, Vector3Normalize(SOL_DIR_GR));
    if (s > 0.0f) c = MezclaGR(c, SOL_GR, std::pow(s, 10.0f) * 0.6f);

    c.a = 255;
    return c;
}


static void CieloEstaticoGR(bool decoracion)
{
    static const float ELEVACIONES[] = { -40.0f, -20.0f, -10.0f, -5.0f, -2.0f, 0.0f, 3.0f, 7.0f, 13.0f, 22.0f, 34.0f, 50.0f, 70.0f, 90.0f };
    const int anillos = (int)(sizeof(ELEVACIONES) / sizeof(ELEVACIONES[0]));
    const int lados = decoracion ? 48 : 28;

    for (int i = 0; i + 1 < anillos; i++)
    {
        for (int k = 0; k < lados; k++)
        {
            float a0 = -180.0f + 360.0f * (float)k / (float)lados;
            float a1 = -180.0f + 360.0f * (float)(k + 1) / (float)lados;
            Vector3 d00 = DireccionGR(a0, ELEVACIONES[i]), d01 = DireccionGR(a1, ELEVACIONES[i]);
            Vector3 d10 = DireccionGR(a0, ELEVACIONES[i + 1]), d11 = DireccionGR(a1, ELEVACIONES[i + 1]);
            Quad4GR(
                Vector3Scale(d00, RADIO_CIELO_GR), Vector3Scale(d01, RADIO_CIELO_GR),
                Vector3Scale(d11, RADIO_CIELO_GR), Vector3Scale(d10, RADIO_CIELO_GR),
                ColorCieloGR(d00), ColorCieloGR(d01), ColorCieloGR(d11), ColorCieloGR(d10));
        }
    }

    // Cumulos blancos de panza gris celeste cerca del horizonte.
    int cantidad = decoracion ? 18 : 7;

    for (int n = 0; n < cantidad; n++)
    {
        float az = -110.0f + 220.0f * Ruido01GR(n, 11);
        float el = -6.0f + 22.0f * Ruido01GR(n, 12) * Ruido01GR(n, 13);
        float ancho = 16.0f + 24.0f * Ruido01GR(n, 14);
        float alto = 4.0f + 6.0f * Ruido01GR(n, 15);
        float r = RADIO_CIELO_GR * 0.93f;
        Vector3 centro = DireccionGR(az, el);
        Vector3 der = Vector3Normalize(Vector3CrossProduct(centro, { 0.0f, 1.0f, 0.0f }));
        Vector3 arr = Vector3CrossProduct(der, centro);
        Color panza = Color{ 204, 222, 228, 235 };
        Color lomo = Color{ 255, 255, 250, 250 };
        int tramos = 8;

        for (int k = 0; k < tramos; k++)
        {
            float u0 = -1.0f + 2.0f * (float)k / (float)tramos, u1 = -1.0f + 2.0f * (float)(k + 1) / (float)tramos;
            float h0 = alto * (1.0f - u0 * u0) * (0.6f + 0.8f * Ruido01GR(n * 13 + k, 16));
            float h1 = alto * (1.0f - u1 * u1) * (0.6f + 0.8f * Ruido01GR(n * 13 + k + 1, 16));
            auto P = [&](float u, float v)
            {
                return Vector3Add(Vector3Scale(centro, r), Vector3Add(Vector3Scale(der, u * ancho), Vector3Scale(arr, v)));
            };
            Quad4GR(P(u0, 0.0f), P(u1, 0.0f), P(u1, h1 * 0.4f), P(u0, h0 * 0.4f), AlfaGR(panza, 0.0f), AlfaGR(panza, 0.0f), panza, panza);
            Quad4GR(P(u0, h0 * 0.4f), P(u1, h1 * 0.4f), P(u1, h1), P(u0, h0), lomo, lomo, AlfaGR(lomo, 0.0f), AlfaGR(lomo, 0.0f));
        }
    }
}


//--------------------------------------------------
// Lejania: valle de selva, sierras y cascada
//--------------------------------------------------

static const float Y_VALLE_GR = -34.0f;
static const float Z_BORDE_MESETA_GR = -40.0f;


// Copas del valle: lomas de bolas achatadas sobre el piso del valle.
static void CopasValleGR(float z0, float z1, float x0, float x1, float paso, int semilla, Color base)
{
    int fila = 0;

    for (float z = z0; z > z1; z -= paso, fila++)
    {
        for (float x = x0 + (fila % 2) * paso * 0.5f; x < x1; x += paso)
        {
            int k = (int)(x * 7.0f) + fila * 131;
            float r = paso * (0.55f + 0.35f * Ruido01GR(k, semilla));
            float h = Y_VALLE_GR + 2.0f * Ruido01GR(k, semilla + 1) + 3.0f * std::sin(x * 0.03f + z * 0.02f);
            Color c = VariarGR(MezclaGR(base, HOJA_CLARA_GR, 0.25f * Ruido01GR(k, semilla + 2)), k, semilla + 3, 0.08f);
            BolaGR({ x + paso * 0.3f * (Ruido01GR(k, semilla + 4) - 0.5f), h, z }, { r, r * 0.55f, r }, c, 2, 6, 0.0f);
        }
    }
}


// Sierra: perfil de picos en un plano z con caras en sombra y al sol.
static void SierraGR(float z, float x0, float x1, float base, float alto, int semilla, Color color)
{
    const int tramos = 26;
    float prevX = x0, prevY = base + alto * 0.3f;

    for (int k = 1; k <= tramos; k++)
    {
        float x = x0 + (x1 - x0) * (float)k / (float)tramos;
        float y = base + alto * (0.25f + 0.75f * Ruido01GR(k, semilla) * (0.6f + 0.4f * std::sin((float)k * 0.7f + (float)semilla)));
        Color c = (y > prevY) ? CaraGR(color, Vector3Normalize({ -0.5f, 0.6f, 0.6f })) : CaraGR(color, Vector3Normalize({ 0.6f, 0.5f, 0.6f }));
        Color pie = MezclaGR(c, NIEBLA_GR, 0.35f);
        Quad4GR({ prevX, base - 30.0f, z }, { x, base - 30.0f, z }, { x, y, z + 6.0f }, { prevX, prevY, z + 6.0f }, pie, pie, c, c);
        prevX = x;
        prevY = y;
    }
}


static void LejaniaEstaticaGR(bool decoracion, int paso)
{
    // Piso del valle.
    PisoGR(-320.0f, Z_BORDE_MESETA_GR - 6.0f, 320.0f, -380.0f, Y_VALLE_GR - 1.0f, HOJA_OSCURA_GR);

    // Sierras en dos planos (la de atras mas clara por la bruma).
    SierraGR(-330.0f, -420.0f, 420.0f, Y_VALLE_GR, 58.0f, 41, Color{ 92, 124, 116, 255 });
    SierraGR(-215.0f, -300.0f, 300.0f, Y_VALLE_GR, 38.0f, 23, Color{ 58, 98, 66, 255 });

    if (!decoracion)
    {
        return;
    }

    // Copas del valle en tres franjas.
    CopasValleGR(Z_BORDE_MESETA_GR - 8.0f, -110.0f, -180.0f, 180.0f, 9.0f * (float)paso, 51, HOJA_GR);
    CopasValleGR(-110.0f, -205.0f, -260.0f, 260.0f, 15.0f * (float)paso, 61, Color{ 50, 104, 60, 255 });

    // Rio que serpentea por el valle.
    for (int k = 0; k < 24; k++)
    {
        float z0 = -60.0f - (float)k * 7.0f, z1 = z0 - 7.0f;
        float x0 = 38.0f + 22.0f * std::sin((float)k * 0.42f), x1 = 38.0f + 22.0f * std::sin((float)(k + 1) * 0.42f);
        float w0 = 2.5f + (float)k * 0.25f, w1 = w0 + 0.25f;
        Color agua = MezclaGR(Color{ 120, 180, 190, 255 }, CIELO_HORIZONTE_GR, 0.3f);
        QuadGR({ x0 - w0, Y_VALLE_GR + 2.2f, z0 }, { x0 + w0, Y_VALLE_GR + 2.2f, z0 }, { x1 + w1, Y_VALLE_GR + 2.2f, z1 }, { x1 - w1, Y_VALLE_GR + 2.2f, z1 }, agua);
    }

    // Peñones con selva en lo alto, en medio del valle.
    Color roca = Color{ 120, 116, 98, 255 };
    for (int k = 0; k < 6; k++)
    {
        float x = -96.0f + (float)k * 7.0f;
        float alto = 26.0f + 10.0f * Ruido01GR(k, 71);
        CajaGR({ x, Y_VALLE_GR + alto * 0.5f, -150.0f }, { 8.0f, alto, 10.0f }, VariarGR(roca, k, 72, 0.1f));
        BolaGR({ x, Y_VALLE_GR + alto + 1.0f, -150.0f }, { 5.5f, 3.0f, 6.0f }, VariarGR(HOJA_GR, k, 73, 0.1f), 2, 7, 0.0f);
    }
}


//--------------------------------------------------
// Arena: losas de la plaza, zona del guardian y bordes
//--------------------------------------------------

// Mechon de pasto entre juntas (tres hojitas).
static void MechonGR(Vector3 p, float alto, int semilla)
{
    for (int k = 0; k < 3; k++)
    {
        float a = (float)k * 2.1f + 6.0f * Ruido01GR(semilla, 3);
        Vector3 punta = { p.x + std::cos(a) * alto * 0.45f, p.y + alto, p.z + std::sin(a) * alto * 0.45f };
        Color c = VariarGR(MezclaGR(HOJA_CLARA_GR, MUSGO_GR, Ruido01GR(semilla + k, 4)), semilla + k, 5, 0.1f);
        TriGR({ p.x - 0.05f, p.y, p.z }, { p.x + 0.05f, p.y, p.z }, punta, c);
        TriGR({ p.x, p.y, p.z - 0.05f }, { p.x, p.y, p.z + 0.05f }, punta, TonoGR(c, 0.85f));
    }
}


// Losa con juntas: cara superior algo hundida en los bordes y musgo o
// grieta segun el azar visual.
static void LosaGR(float x0, float z0, float x1, float z1, Color base, int k, bool detalle, float yBase = 0.0f)
{
    const float j = 0.05f;
    float y = yBase + 0.012f + 0.01f * Ruido01GR(k, 21);
    Color c = VariarGR(base, k, 22, 0.08f);
    float tibio = Ruido01GR(k, 23);
    c = MezclaGR(c, PIEDRA_CLARA_GR, tibio * 0.18f);

    // Bisel: banda exterior apenas mas oscura.
    Color borde = TonoGR(c, 0.86f);
    QuadGR({ x0 + j, y - 0.006f, z0 + j }, { x0 + j, y - 0.006f, z1 - j }, { x1 - j, y - 0.006f, z1 - j }, { x1 - j, y - 0.006f, z0 + j }, borde);
    QuadGR({ x0 + j + 0.07f, y, z0 + j + 0.07f }, { x0 + j + 0.07f, y, z1 - j - 0.07f }, { x1 - j - 0.07f, y, z1 - j - 0.07f }, { x1 - j - 0.07f, y, z0 + j + 0.07f }, c);

    if (!detalle) return;

    float r = Ruido01GR(k, 24);

    if (r < 0.22f)
    {
        // Grieta en zigzag.
        float ax = x0 + (x1 - x0) * (0.2f + 0.2f * Ruido01GR(k, 25));
        float bx = x0 + (x1 - x0) * (0.6f + 0.3f * Ruido01GR(k, 26));
        float mz = z0 + (z1 - z0) * 0.5f;
        Color g = TonoGR(c, 0.55f);
        QuadGR({ ax, y + 0.002f, z0 + 0.12f }, { ax + 0.04f, y + 0.002f, z0 + 0.12f }, { bx + 0.04f, y + 0.002f, mz }, { bx, y + 0.002f, mz }, g);
        QuadGR({ bx, y + 0.002f, mz }, { bx + 0.04f, y + 0.002f, mz }, { ax + 0.3f, y + 0.002f, z1 - 0.12f }, { ax + 0.26f, y + 0.002f, z1 - 0.12f }, g);
    }
    else if (r < 0.5f)
    {
        // Musgo que avanza desde una esquina.
        float u = 0.3f + 0.35f * Ruido01GR(k, 27);
        bool izq = Ruido01GR(k, 28) < 0.5f;
        float cx = izq ? x0 + 0.1f : x1 - 0.1f;
        float cz = Ruido01GR(k, 29) < 0.5f ? z0 + 0.1f : z1 - 0.1f;
        Color m = AlfaGR(VariarGR(MUSGO_GR, k, 30, 0.12f), 0.85f);
        DiscoGR({ cx, y + 0.003f, cz }, (x1 - x0) * u, (z1 - z0) * u, 7, m, AlfaGR(m, 0.0f));
    }
}


static void PlazaGR(bool detalle)
{
    // Lecho de las juntas: tierra y musgo bajo todas las losas.
    PisoGR(-X_PLAZA, Z_FONDO, X_PLAZA, Z_SUR, -0.004f, JUNTA_GR);

    // Plaza de los atacantes: losas a soga (filas de largo variable).
    int k = 0;
    float z = Z_SUR;
    int fila = 0;

    while (z > Z_MIN_ATACANTE + 0.01f)
    {
        float profundo = 1.15f + 0.35f * Ruido01GR(fila, 31);
        float z1 = z - profundo;
        if (z1 < Z_MIN_ATACANTE + 0.4f) z1 = Z_MIN_ATACANTE;

        float x = -X_PLAZA;
        float desfase = (fila % 2) ? 0.7f : 0.0f;
        bool primera = true;

        while (x < X_PLAZA - 0.01f)
        {
            float largo = primera ? 0.75f + desfase : 1.2f + 0.7f * Ruido01GR(k, 32);
            float x1 = x + largo;
            if (x1 > X_PLAZA - 0.5f) x1 = X_PLAZA;
            LosaGR(x, z1, x1, z, LOSA_GR, k, detalle);
            x = x1;
            k++;
            primera = false;
        }

        z = z1;
        fila++;
    }

    // Zona del guardian: losas grandes de piedra verdosa.
    for (int fz = 0; fz < 4; fz++)
    {
        float za = Z_MIN_ATACANTE - 0.35f - (float)fz * ((Z_MIN_ATACANTE - 0.35f - Z_FONDO) / 4.0f);
        float zb = za - (Z_MIN_ATACANTE - 0.35f - Z_FONDO) / 4.0f;

        for (int fx = 0; fx < 8; fx++)
        {
            float xa = -X_PLAZA + (float)fx * (2.0f * X_PLAZA / 8.0f);
            float xb = xa + 2.0f * X_PLAZA / 8.0f;
            LosaGR(xa, zb, xb, za, JADE_GR, 400 + fz * 8 + fx, detalle);
        }
    }

    // Limite de los atacantes: franja de piedras talladas con glifos.
    {
        float z0 = Z_MIN_ATACANTE - 0.35f, z1 = Z_MIN_ATACANTE;
        CajaGR({ 0.0f, 0.02f, 0.5f * (z0 + z1) }, { 2.0f * X_PLAZA, 0.05f, z1 - z0 }, PIEDRA_CLARA_GR);

        for (int g = 0; g < 16; g++)
        {
            float x = -X_PLAZA + (float)g * (2.0f * X_PLAZA / 16.0f) + X_PLAZA / 16.0f;
            Color tallado = TonoGR(PIEDRA_CLARA_GR, 0.62f);
            float zc = 0.5f * (z0 + z1);
            QuadGR({ x - 0.22f, 0.047f, zc - 0.1f }, { x - 0.22f, 0.047f, zc + 0.1f }, { x + 0.22f, 0.047f, zc + 0.1f }, { x + 0.22f, 0.047f, zc - 0.1f }, tallado);
            QuadGR({ x - 0.06f, 0.049f, zc - 0.06f }, { x - 0.06f, 0.049f, zc + 0.06f }, { x + 0.06f, 0.049f, zc + 0.06f }, { x + 0.06f, 0.049f, zc - 0.06f }, MezclaGR(PIEDRA_CLARA_GR, RUNA_GR, 0.45f));
        }
    }

    // Riel del guardian: canal pulido con tapas de bronce en los topes.
    {
        float zc = GUARDIAN_Z, ancho = 0.9f, lim = GUARDIAN_LIMITE_X + 0.55f;
        CajaGR({ 0.0f, 0.03f, zc }, { 2.0f * lim, 0.05f, ancho }, TonoGR(JADE_GR, 0.78f));
        PisoGR(-lim + 0.1f, zc - 0.08f, lim - 0.1f, zc + 0.08f, 0.058f, TonoGR(JADE_GR, 0.5f));

        for (int lado = -1; lado <= 1; lado += 2)
        {
            CajaGR({ (float)lado * (lim + 0.15f), 0.16f, zc }, { 0.3f, 0.32f, ancho + 0.2f }, BRONCE_GR);
            CajaGR({ (float)lado * (lim + 0.15f), 0.36f, zc }, { 0.18f, 0.08f, ancho }, TonoGR(BRONCE_GR, 1.15f));
        }
    }

    // Linea del portal: umbral de piedra clara dentro del vano.
    CajaGR({ 0.0f, 0.03f, Z_FONDO - 0.55f }, { 2.0f * PORTAL_MEDIO, 0.06f, 1.1f }, TonoGR(PIEDRA_OSCURA_GR, 0.8f));

    if (!detalle) return;

    // Pasto entre las juntas, mas tupido cerca de los muros.
    for (int m = 0; m < 70; m++)
    {
        float u = Ruido01GR(m, 33);
        float x = (u < 0.5f ? -1.0f : 1.0f) * (X_PLAZA - 0.15f - 2.6f * Ruido01GR(m, 34) * Ruido01GR(m, 35));
        float zz = Z_FONDO + 0.4f + (Z_SUR - Z_FONDO - 0.8f) * Ruido01GR(m, 36);
        MechonGR({ x, 0.0f, zz }, 0.14f + 0.12f * Ruido01GR(m, 37), m);
    }
}


// Muro de sillares entre (x0,z0) y (x1,z1) con cara interior en el borde
// real; hiladas con juntas, roturas y lomo de musgo.
static void MuroSillaresGR(float x0, float z0, float x1, float z1, float grosor, float altoMin, float altoMax, int semilla, bool detalle)
{
    bool enX = std::fabs(x1 - x0) > std::fabs(z1 - z0);
    float largo = enX ? std::fabs(x1 - x0) : std::fabs(z1 - z0);
    float dir = enX ? (x1 > x0 ? 1.0f : -1.0f) : (z1 > z0 ? 1.0f : -1.0f);
    float s = 0.0f;
    int k = 0;

    while (s < largo - 0.01f)
    {
        float tramo = 1.1f + 0.6f * Ruido01GR(k, semilla);
        if (s + tramo > largo - 0.4f) tramo = largo - s;
        float m = s + tramo * 0.5f;
        float alto = altoMin + (altoMax - altoMin) * Ruido01GR(k, semilla + 1);
        float cx = enX ? x0 + dir * m : x0;
        float cz = enX ? z0 : z0 + dir * m;
        Vector3 t = enX ? Vector3{ tramo - 0.04f, 0.0f, grosor } : Vector3{ grosor, 0.0f, tramo - 0.04f };

        // Hilada baja continua y alta irregular.
        float bajo = altoMin * 0.62f;
        Color c0 = VariarGR(PIEDRA_GR, k, semilla + 2, 0.07f);
        t.y = bajo;
        CajaGR({ cx, bajo * 0.5f, cz }, t, c0);

        float resto = alto - bajo;
        Color c1 = VariarGR(PIEDRA_GR, k, semilla + 3, 0.09f);
        t.y = resto - 0.03f;
        if (enX) t.x *= 0.96f; else t.z *= 0.96f;
        CajaGR({ cx, bajo + resto * 0.5f + 0.015f, cz }, t, c1);

        // Lomo de musgo con borde colgante.
        Vector3 tm = enX ? Vector3{ tramo * 0.9f, 0.07f, grosor + 0.06f } : Vector3{ grosor + 0.06f, 0.07f, tramo * 0.9f };
        CajaGR({ cx, alto + 0.03f, cz }, tm, VariarGR(MUSGO_GR, k, semilla + 4, 0.1f));

        if (detalle && Ruido01GR(k, semilla + 5) < 0.55f)
        {
            // Mata de hierba o helecho chico sobre el muro.
            MechonGR({ cx + (enX ? 0.3f : 0.0f), alto + 0.05f, cz + (enX ? 0.0f : 0.3f) }, 0.3f, k + semilla);
        }

        s += tramo;
        k++;
    }
}


static void BordesPlazaGR(bool detalle)
{
    // Muros laterales: cara interior exactamente en x = +-X_PLAZA.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (X_PLAZA + MURO_GROSOR_GR * 0.5f);
        MuroSillaresGR(x, Z_FONDO - 1.1f, x, Z_SUR + 0.8f, MURO_GROSOR_GR, 1.15f, 1.9f, lado < 0 ? 101 : 131, detalle);

        // Bloques caidos del lado de afuera.
        for (int b = 0; b < 4; b++)
        {
            float z = -9.0f + (float)b * 5.1f + 1.5f * Ruido01GR(b, lado + 140);
            float giro = 40.0f * (Ruido01GR(b, lado + 141) - 0.5f);
            CajaGiradaGR({ (float)lado * (X_PLAZA + 1.6f + 0.6f * Ruido01GR(b, lado + 142)), 0.26f, z }, { 1.0f, 0.52f, 0.7f }, giro, VariarGR(PIEDRA_GR, b, lado + 143, 0.1f), 8.0f);
        }
    }

    // Pretil sur bajo (cara interior en z = Z_SUR): no tapa a nadie.
    MuroSillaresGR(-X_PLAZA - MURO_GROSOR_GR, Z_SUR + 0.4f, X_PLAZA + MURO_GROSOR_GR, Z_SUR + 0.4f, 0.8f, 0.42f, 0.52f, 171, false);

    // Postes rotos del pretil en las esquinas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (X_PLAZA + 0.45f);
        CajaGR({ x, 0.75f, Z_SUR + 0.4f }, { 1.05f, 1.5f, 1.05f }, PIEDRA_CLARA_GR);
        CajaGR({ x, 1.56f, Z_SUR + 0.4f }, { 1.2f, 0.14f, 1.2f }, TonoGR(PIEDRA_CLARA_GR, 1.04f));
        CajaGR({ x, 1.66f, Z_SUR + 0.4f }, { 1.0f, 0.07f, 1.0f }, MUSGO_GR);
    }
}


//--------------------------------------------------
// Muro del templo y portal runico (marco de piedra)
//--------------------------------------------------

// Bloque con glifo tallado en su cara frontal (+z).
static void GlifoGR(float cx, float cy, float z, float lado, int semilla, Color piedra)
{
    Color hondo = TonoGR(piedra, 0.6f);
    float s = lado * 0.5f;
    int forma = (int)(Ruido01GR(semilla, 201) * 4.0f);

    // Marco del cartucho.
    QuadGR({ cx - s, cy - s, z }, { cx + s, cy - s, z }, { cx + s, cy - s + 0.06f, z }, { cx - s, cy - s + 0.06f, z }, hondo);
    QuadGR({ cx - s, cy + s - 0.06f, z }, { cx + s, cy + s - 0.06f, z }, { cx + s, cy + s, z }, { cx - s, cy + s, z }, hondo);
    QuadGR({ cx - s, cy - s, z }, { cx - s + 0.06f, cy - s, z }, { cx - s + 0.06f, cy + s, z }, { cx - s, cy + s, z }, hondo);
    QuadGR({ cx + s - 0.06f, cy - s, z }, { cx + s, cy - s, z }, { cx + s, cy + s, z }, { cx + s - 0.06f, cy + s, z }, hondo);

    // Figura interior: ojo, espiral, escalon o cruz.
    float r = s * 0.5f;
    if (forma == 0)
    {
        QuadGR({ cx - r, cy - 0.04f, z }, { cx + r, cy - 0.04f, z }, { cx + r, cy + 0.04f, z }, { cx - r, cy + 0.04f, z }, hondo);
        QuadGR({ cx - 0.05f, cy - r, z }, { cx + 0.05f, cy - r, z }, { cx + 0.05f, cy + r, z }, { cx - 0.05f, cy + r, z }, hondo);
    }
    else if (forma == 1)
    {
        QuadGR({ cx - r, cy - r, z }, { cx, cy - r, z }, { cx, cy, z }, { cx - r, cy, z }, hondo);
        QuadGR({ cx, cy, z }, { cx + r, cy, z }, { cx + r, cy + r, z }, { cx, cy + r, z }, hondo);
    }
    else if (forma == 2)
    {
        for (int k = 0; k < 6; k++)
        {
            float a0 = 2.0f * PI * (float)k / 6.0f, a1 = 2.0f * PI * (float)(k + 1) / 6.0f;
            TriGR({ cx, cy, z }, { cx + std::cos(a0) * r, cy + std::sin(a0) * r, z }, { cx + std::cos(a1) * r, cy + std::sin(a1) * r, z }, k % 2 ? hondo : TonoGR(piedra, 0.8f));
        }
    }
    else
    {
        QuadGR({ cx - r, cy - r, z }, { cx + r, cy - r, z }, { cx + r, cy - r * 0.3f, z }, { cx - r, cy - r * 0.3f, z }, hondo);
        QuadGR({ cx - r * 0.3f, cy - r * 0.3f, z }, { cx + r, cy - r * 0.3f, z }, { cx + r, cy + r, z }, { cx - r * 0.3f, cy + r, z }, hondo);
    }
}


static void MuroTemploGR(bool detalle)
{
    float zf = Z_FONDO;                       // cara frontal (borde real)
    float zc = 0.5f * (Z_FONDO + Z_MURO_TEMPLO_GR);
    float grosor = Z_FONDO - Z_MURO_TEMPLO_GR;
    float xj = PORTAL_MEDIO + PORTAL_JAMBA_GR;  // borde exterior de la jamba

    // Muro de contencion de la terraza a ambos lados del portal.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float xa = (float)lado * xj, xb = (float)lado * (X_PLAZA + MURO_GROSOR_GR);
        float cx = 0.5f * (xa + xb), ancho = std::fabs(xb - xa);

        CajaGR({ cx, 0.45f, zc }, { ancho, 0.9f, grosor }, VariarGR(PIEDRA_GR, lado, 210, 0.05f));
        CajaGR({ cx, 1.5f, zc }, { ancho, 1.2f, grosor - 0.1f }, VariarGR(PIEDRA_GR, lado, 211, 0.06f));
        CajaGR({ cx, 2.35f, zc }, { ancho + 0.1f, 0.5f, grosor + 0.1f }, PIEDRA_CLARA_GR);
        CajaGR({ cx, 2.63f, zc }, { ancho, 0.06f, grosor + 0.12f }, MUSGO_GR);

        // Friso de glifos en la hilada media.
        int n = (int)(ancho / 0.9f);
        for (int g = 0; g < n; g++)
        {
            float x = (lado < 0 ? xb : xa) + ((float)g + 0.5f) * (ancho / (float)n);
            GlifoGR(x, 1.5f, zf + 0.06f, 0.62f, g + (lado + 1) * 20, PIEDRA_GR);
        }

        // Juntas verticales del zocalo.
        for (int g = 0; g <= n; g++)
        {
            float x = (lado < 0 ? xb : xa) + (float)g * (ancho / (float)n) + 0.3f;
            QuadGR({ x - 0.02f, 0.02f, zf + 0.055f }, { x + 0.02f, 0.02f, zf + 0.055f }, { x + 0.02f, 0.88f, zf + 0.055f }, { x - 0.02f, 0.88f, zf + 0.055f }, JUNTA_GR);
        }
    }

    // Jambas del portal (su cara interior es el borde del arco de gol).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float cx = (float)lado * (PORTAL_MEDIO + PORTAL_JAMBA_GR * 0.5f);
        CajaGR({ cx, 0.25f, zc + 0.05f }, { PORTAL_JAMBA_GR + 0.3f, 0.5f, grosor + 0.4f }, PIEDRA_OSCURA_GR);

        for (int b = 0; b < 4; b++)
        {
            float y0 = 0.5f + (float)b * ((PORTAL_ALTO_GR - 0.5f) / 4.0f);
            float y1 = y0 + (PORTAL_ALTO_GR - 0.5f) / 4.0f;
            CajaGR({ cx, 0.5f * (y0 + y1), zc + 0.05f }, { PORTAL_JAMBA_GR - 0.02f * (float)(b % 2), y1 - y0 - 0.03f, grosor + 0.2f }, VariarGR(PIEDRA_CLARA_GR, b + lado * 7, 220, 0.06f));
        }
    }

    // Dintel escalonado y cresteria con mascaron.
    CajaGR({ 0.0f, PORTAL_ALTO_GR + 0.45f, zc + 0.05f }, { 2.0f * (xj + 0.4f), 0.9f, grosor + 0.3f }, PIEDRA_CLARA_GR);
    CajaGR({ 0.0f, PORTAL_ALTO_GR + 1.05f, zc }, { 2.0f * xj, 0.3f, grosor }, PIEDRA_GR);
    CajaGR({ 0.0f, PORTAL_ALTO_GR + 1.55f, zc - 0.1f }, { 3.0f, 0.7f, grosor - 0.2f }, PIEDRA_GR);
    CajaGR({ 0.0f, PORTAL_ALTO_GR + 2.1f, zc - 0.1f }, { 1.6f, 0.4f, grosor - 0.3f }, PIEDRA_CLARA_GR);
    CajaGR({ 0.0f, PORTAL_ALTO_GR + 1.12f, zc - 0.05f }, { 2.0f * xj + 0.2f, 0.06f, grosor + 0.1f }, MUSGO_GR);

    // Mascaron: cejas, ojos hundidos (se encienden aparte) y nariz.
    float zm = zf + 0.08f, ym = PORTAL_ALTO_GR + 1.55f;
    CajaGR({ 0.0f, ym + 0.22f, zm }, { 2.2f, 0.12f, 0.16f }, PIEDRA_CLARA_GR);
    CajaGR({ 0.0f, ym - 0.06f, zm + 0.08f }, { 0.24f, 0.4f, 0.2f }, PIEDRA_CLARA_GR);
    for (int lado = -1; lado <= 1; lado += 2)
    {
        CajaGR({ (float)lado * 0.5f, ym + 0.04f, zm - 0.04f }, { 0.4f, 0.22f, 0.06f }, TonoGR(PIEDRA_OSCURA_GR, 0.55f));
        CajaGR({ (float)lado * 1.35f, ym - 0.1f, zm }, { 0.26f, 0.5f, 0.14f }, PIEDRA_CLARA_GR);
    }

    // Fondo del vano: piedra oscura detras del velo.
    CajaGR({ 0.0f, PORTAL_ALTO_GR * 0.5f, Z_MURO_TEMPLO_GR - 0.15f }, { 2.0f * PORTAL_MEDIO, PORTAL_ALTO_GR, 0.3f }, Color{ 34, 44, 44, 255 });
    PisoGR(-PORTAL_MEDIO, Z_MURO_TEMPLO_GR, PORTAL_MEDIO, Z_FONDO - 0.1f, 0.01f, Color{ 52, 62, 58, 255 });

    if (!detalle) return;

    // Enredaderas que cuelgan del muro y del dintel.
    for (int v = 0; v < 14; v++)
    {
        float x = -X_PLAZA + 2.0f * X_PLAZA * Ruido01GR(v, 230);
        if (std::fabs(x) < PORTAL_MEDIO + 0.1f) continue;   // el vano queda libre
        float yTop = std::fabs(x) < xj + 0.4f ? PORTAL_ALTO_GR + 0.9f : 2.6f;
        float largo = 0.8f + 1.4f * Ruido01GR(v, 231);
        Color c = VariarGR(HOJA_GR, v, 232, 0.12f);
        LianaGR({ x, yTop, zf + 0.12f }, { x + 0.1f, yTop - largo, zf + 0.14f }, 0.07f, TonoGR(c, 0.7f));

        for (int h = 0; h < 4; h++)
        {
            float y = yTop - largo * ((float)h + 0.5f) / 4.0f;
            float s = (h % 2) ? 1.0f : -1.0f;
            HojaGR({ x, y, zf + 0.16f }, { x + s * 0.26f, y - 0.12f, zf + 0.3f }, 0.16f, c);
        }
    }
}


//--------------------------------------------------
// Templo: terraza, piramide escalonada y santuario
//--------------------------------------------------

static const float Y_TERRAZA_GR = 2.6f;


static void TemploGR(bool detalle)
{
    float z0 = Z_MURO_TEMPLO_GR;

    // Terraza detras del muro.
    CajaGR({ 0.0f, Y_TERRAZA_GR * 0.5f, z0 - 2.4f }, { 2.0f * (X_PLAZA + MURO_GROSOR_GR) + 0.2f, Y_TERRAZA_GR, 4.8f }, PIEDRA_OSCURA_GR);
    for (int k = 0; k < 9; k++)
    {
        float x = -8.4f + (float)k * 2.1f;
        LosaGR(x - 1.0f, z0 - 4.7f, x + 1.0f, z0 - 0.1f, PIEDRA_GR, 600 + k, detalle, Y_TERRAZA_GR);
    }

    // Piramide: cuatro cuerpos con talud y cornisa.
    const int cuerpos = 4;
    float base = Y_TERRAZA_GR;
    float zFrente = z0 - 4.8f;

    for (int i = 0; i < cuerpos; i++)
    {
        float alto = 2.1f;
        float ancho = 22.0f - (float)i * 3.6f;
        float zf = zFrente - (float)i * 2.2f;
        float fondo = 14.0f - (float)i * 2.2f;
        Color c = VariarGR(PIEDRA_GR, i, 240, 0.05f);

        // Talud: el frente se inclina hacia atras.
        Vector3 a = { -ancho * 0.5f, base, zf }, b = { ancho * 0.5f, base, zf };
        Vector3 cc = { ancho * 0.5f - 0.3f, base + alto, zf - 0.6f }, d = { -ancho * 0.5f + 0.3f, base + alto, zf - 0.6f };
        QuadGR(a, b, cc, d, CaraGR(c, Vector3Normalize({ 0.0f, 0.28f, 1.0f })));
        QuadGR({ -ancho * 0.5f, base, zf - fondo }, a, d, { -ancho * 0.5f + 0.3f, base + alto, zf - fondo }, CaraGR(c, { -1.0f, 0.0f, 0.0f }));
        QuadGR(b, { ancho * 0.5f, base, zf - fondo }, { ancho * 0.5f - 0.3f, base + alto, zf - fondo }, cc, CaraGR(c, { 1.0f, 0.0f, 0.0f }));
        PisoGR(-ancho * 0.5f + 0.3f, zf - fondo, ancho * 0.5f - 0.3f, zf - 0.6f, base + alto, CaraGR(c, { 0.0f, 1.0f, 0.0f }));

        // Cornisa clara con musgo.
        CajaGR({ 0.0f, base + alto - 0.12f, zf - 0.55f }, { ancho - 0.5f, 0.24f, 0.3f }, PIEDRA_CLARA_GR);
        CajaGR({ 0.0f, base + alto + 0.01f, zf - 0.75f }, { ancho - 0.8f, 0.04f, 0.4f }, VariarGR(MUSGO_GR, i, 241, 0.1f));

        // Hiladas del talud.
        for (int h = 1; h < 3; h++)
        {
            float y = base + alto * (float)h / 3.0f;
            float zz = zf - 0.6f * (float)h / 3.0f + 0.02f;
            QuadGR({ -ancho * 0.5f + 0.1f, y - 0.03f, zz }, { ancho * 0.5f - 0.1f, y - 0.03f, zz }, { ancho * 0.5f - 0.1f, y + 0.03f, zz }, { -ancho * 0.5f + 0.1f, y + 0.03f, zz }, TonoGR(c, 0.7f));
        }

        base += alto;
    }

    // Escalinata central con alfardas.
    {
        float y0 = Y_TERRAZA_GR, y1 = base;
        float za = zFrente + 0.8f, zb = zFrente - 2.2f * (float)cuerpos;
        int escalones = 18;

        for (int e = 0; e < escalones; e++)
        {
            float u0 = (float)e / (float)escalones, u1 = (float)(e + 1) / (float)escalones;
            float ya = y0 + (y1 - y0) * u0, yb = y0 + (y1 - y0) * u1;
            float zA = za + (zb - za) * u0, zB = za + (zb - za) * u1;
            Color c = VariarGR(PIEDRA_CLARA_GR, e, 250, 0.06f);
            QuadGR({ -2.2f, ya, zA }, { 2.2f, ya, zA }, { 2.2f, yb, zA }, { -2.2f, yb, zA }, CaraGR(c, { 0.0f, 0.0f, 1.0f }));
            PisoGR(-2.2f, zB, 2.2f, zA, yb, CaraGR(c, { 0.0f, 1.0f, 0.0f }));
        }

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 a = { (float)lado * 2.6f, y0 + 0.4f, za + 0.2f }, b = { (float)lado * 2.6f, y1 + 0.4f, zb };
            CajaEjesGR(Vector3Scale(Vector3Add(a, b), 0.5f), { 0.4f, 0.0f, 0.0f }, { 0.0f, 0.45f, 0.0f }, Vector3Scale(Vector3Subtract(b, a), 0.5f), PIEDRA_GR, false);

            // Cabeza de serpiente al pie de la alfarda.
            Vector3 p = { (float)lado * 2.6f, Y_TERRAZA_GR + 0.45f, za + 0.75f };
            CajaGR(p, { 0.95f, 0.9f, 1.1f }, PIEDRA_CLARA_GR);
            CajaGR({ p.x, p.y - 0.2f, p.z + 0.62f }, { 0.85f, 0.25f, 0.2f }, TonoGR(PIEDRA_CLARA_GR, 0.9f));
            CajaGR({ p.x - 0.24f, p.y + 0.18f, p.z + 0.56f }, { 0.16f, 0.14f, 0.04f }, PIEDRA_OSCURA_GR);
            CajaGR({ p.x + 0.24f, p.y + 0.18f, p.z + 0.56f }, { 0.16f, 0.14f, 0.04f }, PIEDRA_OSCURA_GR);
        }
    }

    // Santuario en la cima: cuarto con puerta oscura y cresteria calada.
    {
        float zs = zFrente - 2.2f * (float)cuerpos - 1.6f;
        CajaGR({ 0.0f, base + 1.7f, zs - 2.0f }, { 8.0f, 3.4f, 4.0f }, PIEDRA_GR);
        CajaGR({ 0.0f, base + 3.55f, zs - 2.0f }, { 8.6f, 0.3f, 4.4f }, PIEDRA_CLARA_GR);
        CajaGR({ 0.0f, base + 1.2f, zs + 0.01f }, { 1.8f, 2.4f, 0.02f }, Color{ 30, 36, 34, 255 });
        for (int lado = -1; lado <= 1; lado += 2)
        {
            CajaGR({ (float)lado * 2.6f, base + 1.2f, zs + 0.01f }, { 0.9f, 1.6f, 0.02f }, Color{ 36, 42, 40, 255 });
        }

        for (int c = 0; c < 5; c++)
        {
            float x = -3.0f + (float)c * 1.5f;
            CajaGR({ x, base + 4.6f, zs - 2.4f }, { 0.9f, 1.8f, 0.6f }, VariarGR(PIEDRA_GR, c, 260, 0.08f));
        }
        CajaGR({ 0.0f, base + 5.6f, zs - 2.4f }, { 7.2f, 0.3f, 0.7f }, PIEDRA_CLARA_GR);
    }

    // Ceiba que abraza la piramide: tronco a un costado y raices que bajan.
    {
        Vector3 pie = { -7.6f, base - 2.1f, zFrente - 8.0f };
        CilGR(pie, { pie.x - 0.6f, pie.y + 12.0f, pie.z - 0.5f }, 1.5f, 1.0f, 10, TRONCO_GR, false);

        for (int r = 0; r < 7; r++)
        {
            float a = -0.4f + (float)r * 0.55f;
            Vector3 p0 = { pie.x + std::cos(a) * 1.0f, pie.y + 1.0f, pie.z + std::sin(a) * 0.4f };
            Vector3 p1 = { pie.x + std::cos(a) * 3.5f, pie.y - 2.0f, pie.z + 2.0f + std::sin(a) * 1.0f };
            Vector3 p2 = { pie.x + std::cos(a) * 5.0f, Y_TERRAZA_GR + 0.2f, zFrente + 0.4f + 1.2f * Ruido01GR(r, 270) };
            if (p2.x > -2.8f) p2.x = -2.8f - 0.4f * (float)r;   // la escalinata queda libre
            Color c = VariarGR(TRONCO_GR, r, 271, 0.1f);
            CilGR(p0, p1, 0.45f, 0.3f, 6, c, false);
            CilGR(p1, p2, 0.3f, 0.12f, 6, c, false);
        }
    }

    // Matas sobre las cornisas.
    if (detalle)
    {
        for (int m = 0; m < 26; m++)
        {
            int i = m % cuerpos;
            float ancho = 22.0f - (float)i * 3.6f;
            float x = (Ruido01GR(m, 280) - 0.5f) * (ancho - 2.0f);
            if (std::fabs(x) < 3.0f) x += x < 0.0f ? -3.0f : 3.0f;
            float y = Y_TERRAZA_GR + 2.1f * (float)(i + 1);
            float z = zFrente - (float)i * 2.2f - 0.9f;
            float r = 0.4f + 0.5f * Ruido01GR(m, 281);
            BolaGR({ x, y + r * 0.3f, z }, { r * 1.3f, r * 0.8f, r }, VariarGR(HOJA_GR, m, 282, 0.15f), 2, 7, 0.0f);
        }
    }
}


//--------------------------------------------------
// Selva: suelo, arboles gigantes, palmeras y helechos
//--------------------------------------------------

static void SueloSelvaGR(bool detalle)
{
    // Suelo de la meseta alrededor de la plaza (deja el hueco de la plaza).
    float xe = X_PLAZA + MURO_GROSOR_GR;
    Color tierra = MezclaGR(TIERRA_GR, MUSGO_GR, 0.45f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 10; k++)
        {
            float za = Z_BORDE_MESETA_GR + (float)k * 7.6f, zb = za + 7.6f;
            for (int j = 0; j < 7; j++)
            {
                float xa = xe + (float)j * 9.0f, xb = xa + 9.0f;
                Color c = VariarGR(MezclaGR(tierra, HOJA_GR, 0.4f * Ruido01GR(k * 7 + j, 300 + lado)), k * 7 + j, 301 + lado, 0.07f);
                if (lado < 0) PisoGR(-xb, za, -xa, zb, -0.02f, c);
                else PisoGR(xa, za, xb, zb, -0.02f, c);
            }
        }
    }

    // Franja frente al pretil y detras del templo.
    PisoGR(-xe, Z_SUR + 0.8f, xe, 40.0f, -0.02f, tierra);
    PisoGR(-xe, Z_BORDE_MESETA_GR, xe, Z_MURO_TEMPLO_GR - 4.8f, -0.02f, tierra);

    // Borde de la meseta: risco que cae al valle.
    for (int k = 0; k < 24; k++)
    {
        float x0 = -66.0f + (float)k * 5.5f, x1 = x0 + 5.5f;
        float z = Z_BORDE_MESETA_GR + 1.2f * Ruido01GR(k, 310);
        Color roca = VariarGR(Color{ 132, 124, 102, 255 }, k, 311, 0.1f);
        QuadGR({ x0, -0.02f, z }, { x1, -0.02f, z }, { x1, Y_VALLE_GR, z - 6.0f }, { x0, Y_VALLE_GR, z - 6.0f }, CaraGR(roca, Vector3Normalize({ 0.0f, -0.2f, -1.0f })));
    }

    if (!detalle) return;

    // Manchas de hojarasca y tierra.
    for (int k = 0; k < 30; k++)
    {
        float lado = Ruido01GR(k, 320) < 0.5f ? -1.0f : 1.0f;
        float x = lado * (xe + 1.0f + 24.0f * Ruido01GR(k, 321));
        float z = -30.0f + 50.0f * Ruido01GR(k, 322);
        Color c = AlfaGR(VariarGR(Ruido01GR(k, 323) < 0.5f ? TIERRA_GR : Color{ 140, 120, 64, 255 }, k, 324, 0.1f), 0.8f);
        DiscoGR({ x, 0.0f, z }, 1.4f + 2.0f * Ruido01GR(k, 325), 1.0f + 1.6f * Ruido01GR(k, 326), 8, c, AlfaGR(c, 0.0f));
    }
}


// Copa de selva: racimo de bolas achatadas en tres verdes.
static void CopaGR(Vector3 c, float radio, int semilla, bool simple)
{
    int n = simple ? 3 : 6;

    for (int b = 0; b < n; b++)
    {
        float a = 2.0f * PI * Ruido01GR(b, semilla);
        float d = b == 0 ? 0.0f : radio * (0.45f + 0.35f * Ruido01GR(b, semilla + 1));
        float r = radio * (b == 0 ? 0.85f : 0.5f + 0.25f * Ruido01GR(b, semilla + 2));
        Vector3 p = { c.x + std::cos(a) * d, c.y + (b == 0 ? 0.4f : -0.3f + 0.9f * Ruido01GR(b, semilla + 3)) * radio * 0.4f, c.z + std::sin(a) * d * 0.8f };
        Color col = b % 3 == 0 ? HOJA_GR : (b % 3 == 1 ? HOJA_OSCURA_GR : MezclaGR(HOJA_GR, HOJA_CLARA_GR, 0.5f));
        BolaGR(p, { r, r * 0.62f, r }, VariarGR(col, b, semilla + 4, 0.08f), simple ? 2 : 3, simple ? 7 : 9, simple ? 0.0f : -0.6f);
    }
}


// Arbol gigante con raices tabulares, ramas, copa y lianas.
static void ArbolGiganteGR(Vector3 pie, float alto, int semilla, bool detalle)
{
    float r = 0.7f + 0.35f * Ruido01GR(semilla, 1);
    Color tronco = VariarGR(TRONCO_GR, semilla, 2, 0.08f);
    Vector3 cima = { pie.x + 0.8f * (Ruido01GR(semilla, 3) - 0.5f), pie.y + alto, pie.z + 0.8f * (Ruido01GR(semilla, 4) - 0.5f) };

    CilGR(pie, cima, r, r * 0.6f, 9, tronco, false);

    // Raices tabulares: aletas delgadas que se abren desde el tronco.
    int raices = detalle ? 5 : 3;
    for (int k = 0; k < raices; k++)
    {
        float a = 2.0f * PI * ((float)k + Ruido01GR(k, semilla + 5) * 0.5f) / (float)raices;
        float largo = 1.4f + 0.8f * Ruido01GR(k, semilla + 6);
        float altoR = 1.4f + 1.4f * Ruido01GR(k, semilla + 7);
        Vector3 d = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 b0 = { pie.x + d.x * r * 0.6f, pie.y, pie.z + d.z * r * 0.6f };
        Vector3 b1 = { pie.x + d.x * (r + largo), pie.y, pie.z + d.z * (r + largo) };
        Vector3 t0 = { pie.x + d.x * r * 0.7f, pie.y + altoR, pie.z + d.z * r * 0.7f };
        Vector3 lado = { -d.z * 0.12f, 0.0f, d.x * 0.12f };
        Color c = CaraGR(tronco, Vector3Normalize({ lado.x, 0.2f, lado.z }));
        Color c2 = CaraGR(tronco, Vector3Normalize({ -lado.x, 0.2f, -lado.z }));
        TriGR(Vector3Add(b0, lado), Vector3Add(b1, lado), Vector3Add(t0, lado), c);
        TriGR(Vector3Subtract(b0, lado), Vector3Subtract(t0, lado), Vector3Subtract(b1, lado), c2);
        QuadGR(Vector3Add(b1, lado), Vector3Subtract(b1, lado), Vector3Subtract(t0, lado), Vector3Add(t0, lado), CaraGR(tronco, Vector3Normalize({ d.x, 0.6f, d.z })));
    }

    // Ramas y copa.
    int ramas = detalle ? 3 : 2;
    for (int k = 0; k < ramas; k++)
    {
        float a = 2.0f * PI * ((float)k / (float)ramas + 0.2f * Ruido01GR(k, semilla + 8));
        float h = alto * (0.72f + 0.12f * (float)k / (float)ramas);
        Vector3 b0 = { pie.x + (cima.x - pie.x) * h / alto, pie.y + h, pie.z + (cima.z - pie.z) * h / alto };
        Vector3 b1 = { b0.x + std::cos(a) * 3.2f, b0.y + 2.2f, b0.z + std::sin(a) * 2.2f };
        CilGR(b0, b1, r * 0.35f, r * 0.18f, 6, tronco, false);
        CopaGR({ b1.x, b1.y + 0.6f, b1.z }, 3.0f + 1.2f * Ruido01GR(k, semilla + 9), semilla + 20 + k * 7, !detalle);
    }
    CopaGR({ cima.x, cima.y + 1.2f, cima.z }, 4.2f + 1.0f * Ruido01GR(semilla, 10), semilla + 50, !detalle);

    if (!detalle) return;

    // Lianas que cuelgan de la copa, con hojitas.
    for (int k = 0; k < 4; k++)
    {
        float a = 2.0f * PI * Ruido01GR(k, semilla + 11);
        float d = 2.0f + 2.0f * Ruido01GR(k, semilla + 12);
        Vector3 top = { cima.x + std::cos(a) * d, cima.y - 0.5f, cima.z + std::sin(a) * d * 0.8f };
        float largo = alto * (0.35f + 0.4f * Ruido01GR(k, semilla + 13));
        Vector3 mid = { top.x + 0.3f, top.y - largo * 0.55f, top.z + 0.2f };
        Vector3 bot = { top.x + 0.1f, top.y - largo, top.z };
        Color c = VariarGR(Color{ 70, 96, 50, 255 }, k, semilla + 14, 0.1f);
        LianaGR(top, mid, 0.09f, c);
        LianaGR(mid, bot, 0.07f, c);
        for (int h = 0; h < 3; h++)
        {
            Vector3 p = Vector3Lerp(top, bot, 0.3f + 0.25f * (float)h);
            HojaGR(p, { p.x + ((h % 2) ? 0.4f : -0.4f), p.y - 0.15f, p.z + 0.2f }, 0.22f, VariarGR(HOJA_GR, h, semilla + 15, 0.1f));
        }
    }
}


// Palmera: tronco curvo en tramos y corona de frondas.
static void PalmeraGR(Vector3 pie, float alto, float inclinacion, int semilla)
{
    Vector3 prev = pie;
    Color tronco = VariarGR(Color{ 140, 118, 86, 255 }, semilla, 1, 0.08f);

    for (int k = 1; k <= 4; k++)
    {
        float u = (float)k / 4.0f;
        Vector3 p = { pie.x + inclinacion * u * u * alto * 0.3f, pie.y + alto * u, pie.z + 0.2f * u };
        CilGR(prev, p, 0.28f - 0.04f * u, 0.24f - 0.04f * u, 6, TonoGR(tronco, (k % 2) ? 1.0f : 0.9f), false);
        prev = p;
    }

    for (int f = 0; f < 8; f++)
    {
        float a = 2.0f * PI * ((float)f + 0.3f * Ruido01GR(f, semilla + 2)) / 8.0f;
        float largo = 2.6f + 0.8f * Ruido01GR(f, semilla + 3);
        Vector3 punta = { prev.x + std::cos(a) * largo, prev.y - 0.9f - 0.6f * Ruido01GR(f, semilla + 4), prev.z + std::sin(a) * largo };
        Vector3 medio = { prev.x + std::cos(a) * largo * 0.5f, prev.y + 0.35f, prev.z + std::sin(a) * largo * 0.5f };
        Color c = VariarGR(MezclaGR(HOJA_GR, HOJA_CLARA_GR, 0.4f), f, semilla + 5, 0.12f);
        HojaGR(prev, medio, 0.7f, c);
        HojaGR(medio, punta, 0.6f, TonoGR(c, 0.92f));
    }
}


// Helecho o planta de hojas anchas: hojas que salen en roseta.
static void HelechoGR(Vector3 p, float escala, int semilla, bool anchas)
{
    int n = anchas ? 6 : 8;

    for (int f = 0; f < n; f++)
    {
        float a = 2.0f * PI * ((float)f + 0.4f * Ruido01GR(f, semilla)) / (float)n;
        float largo = escala * (0.8f + 0.4f * Ruido01GR(f, semilla + 1));
        float altura = escala * (anchas ? 0.9f : 0.5f) * (0.7f + 0.5f * Ruido01GR(f, semilla + 2));
        Vector3 punta = { p.x + std::cos(a) * largo, p.y + altura, p.z + std::sin(a) * largo };
        Color c = VariarGR(anchas ? HOJA_GR : MezclaGR(HOJA_GR, HOJA_CLARA_GR, 0.45f), f, semilla + 3, 0.12f);
        HojaGR(p, punta, escala * (anchas ? 0.55f : 0.28f), c);
    }
}


//--------------------------------------------------
// Utileria de las ruinas
//--------------------------------------------------

// Cabeza colosal caida de costado, con tocado, ojos y orejeras.
static void CabezaCaidaGR(Vector3 p, float giro, float escala)
{
    // Base local: la cabeza yace de espaldas con la cara hacia el cielo.
    float s = escala;
    float a = giro * DEG2RAD, b = -62.0f * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz0 = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = Vector3Add(Vector3Scale({ 0.0f, 1.0f, 0.0f }, std::cos(b)), Vector3Scale(uz0, std::sin(b)));
    Vector3 uz = Vector3Subtract(Vector3Scale(uz0, std::cos(b)), Vector3Scale({ 0.0f, 1.0f, 0.0f }, std::sin(b)));
    Vector3 c = { p.x, p.y + 0.85f * s, p.z };
    auto P = [&](float lx, float ly, float lz)
    {
        return Vector3Add(c, Vector3Add(Vector3Scale(ux, lx * s), Vector3Add(Vector3Scale(uy, ly * s), Vector3Scale(uz, lz * s))));
    };
    auto Pieza = [&](float lx, float ly, float lz, float tx, float ty, float tz, Color col)
    {
        CajaEjesGR(P(lx, ly, lz), Vector3Scale(ux, tx * 0.5f * s), Vector3Scale(uy, ty * 0.5f * s), Vector3Scale(uz, tz * 0.5f * s), col, true);
    };

    Pieza(0.0f, 0.0f, 0.0f, 1.7f, 1.9f, 1.5f, PIEDRA_CLARA_GR);                         // craneo
    Pieza(0.0f, 1.05f, -0.1f, 1.9f, 0.35f, 1.6f, TonoGR(PIEDRA_GR, 1.02f));            // tocado
    Pieza(0.0f, 0.42f, 0.8f, 1.5f, 0.16f, 0.14f, TonoGR(PIEDRA_CLARA_GR, 0.96f));      // cejas
    Pieza(0.0f, -0.02f, 0.86f, 0.26f, 0.55f, 0.24f, PIEDRA_CLARA_GR);                  // nariz
    Pieza(0.0f, -0.52f, 0.78f, 0.7f, 0.12f, 0.1f, TonoGR(PIEDRA_OSCURA_GR, 0.8f));     // boca

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float l = (float)lado;
        Pieza(l * 0.42f, 0.2f, 0.76f, 0.34f, 0.16f, 0.06f, TonoGR(PIEDRA_OSCURA_GR, 0.6f));   // ojos
        Pieza(l * 0.95f, 0.0f, 0.0f, 0.22f, 0.7f, 0.7f, PIEDRA_GR);                          // orejeras
    }

    // Musgo y una mata sobre la mejilla.
    BolaGR(P(0.0f, 0.6f, -0.7f), { 0.9f * s, 0.2f * s, 0.8f * s }, MUSGO_GR, 2, 7, 0.0f);
    HelechoGR({ p.x + ux.x * 1.1f * s, 0.0f, p.z + ux.z * 1.1f * s }, 0.7f * s, 991, true);
}


// Cuerpo de estatua derribado: torso, brazo y pedestal partido.
static void CuerpoCaidoGR(Vector3 p, float giro)
{
    CajaGiradaGR({ p.x, 0.25f, p.z }, { 1.6f, 0.5f, 1.6f }, giro + 10.0f, PIEDRA_OSCURA_GR);
    CajaGiradaGR({ p.x + 1.3f, 0.45f, p.z + 0.4f }, { 2.4f, 0.9f, 1.1f }, giro, PIEDRA_GR);
    CajaGiradaGR({ p.x + 2.8f, 0.35f, p.z + 0.8f }, { 0.8f, 0.6f, 0.7f }, giro + 25.0f, PIEDRA_GR);
    CajaGiradaGR({ p.x + 1.1f, 0.25f, p.z + 1.2f }, { 1.4f, 0.4f, 0.4f }, giro - 20.0f, TonoGR(PIEDRA_GR, 0.92f));
    BolaGR({ p.x + 1.4f, 0.92f, p.z + 0.4f }, { 0.9f, 0.12f, 0.45f }, MUSGO_GR, 2, 7, 0.0f);
}


// Estatua de pie sin cabeza sobre su pedestal.
static void EstatuaGR(Vector3 p, float giro)
{
    CajaGiradaGR({ p.x, 0.4f, p.z }, { 1.5f, 0.8f, 1.5f }, giro, PIEDRA_OSCURA_GR);
    CajaGiradaGR({ p.x, 0.86f, p.z }, { 1.3f, 0.12f, 1.3f }, giro, PIEDRA_CLARA_GR);
    CajaGiradaGR({ p.x, 1.6f, p.z }, { 0.8f, 1.4f, 0.55f }, giro, PIEDRA_GR);
    CajaGiradaGR({ p.x, 2.6f, p.z }, { 1.05f, 0.7f, 0.6f }, giro, PIEDRA_GR);
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float a = giro * DEG2RAD;
        CajaGiradaGR({ p.x + std::cos(a) * 0.62f * (float)lado, 2.15f, p.z - std::sin(a) * 0.62f * (float)lado }, { 0.26f, 1.1f, 0.3f }, giro, TonoGR(PIEDRA_GR, 0.95f));
    }
    CajaGiradaGR({ p.x, 3.0f, p.z }, { 0.5f, 0.14f, 0.4f }, giro, TonoGR(PIEDRA_GR, 0.85f));
    BolaGR({ p.x, 3.05f, p.z }, { 0.5f, 0.12f, 0.35f }, MUSGO_GR, 2, 6, 0.0f);
}


// Estela tallada (losa alta con glifos), algo ladeada.
static void EstelaGR(Vector3 p, float giro, float ladeo, int semilla)
{
    float alto = 2.4f + 0.6f * Ruido01GR(semilla, 1);
    CajaGiradaGR({ p.x, 0.15f, p.z }, { 1.4f, 0.3f, 0.9f }, giro, PIEDRA_OSCURA_GR);
    CajaGiradaGR({ p.x, 0.3f + alto * 0.5f, p.z }, { 1.0f, alto, 0.38f }, giro, VariarGR(PIEDRA_CLARA_GR, semilla, 2, 0.06f), ladeo);

    // Glifos sobre la cara (aprox.: cuadritos hundidos en la cara frontal).
    float a = giro * DEG2RAD, b = ladeo * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = Vector3Add(Vector3Scale({ 0.0f, 1.0f, 0.0f }, std::cos(b)), Vector3Scale(uz, std::sin(b)));
    Vector3 nz = Vector3Subtract(Vector3Scale(uz, std::cos(b)), Vector3Scale({ 0.0f, 1.0f, 0.0f }, std::sin(b)));
    Vector3 c0 = Vector3Add({ p.x, 0.3f + alto * 0.5f, p.z }, Vector3Scale(nz, 0.2f));

    for (int f = 0; f < 4; f++)
    {
        for (int g = 0; g < 2; g++)
        {
            Vector3 c = Vector3Add(c0, Vector3Add(Vector3Scale(ux, -0.22f + 0.44f * (float)g), Vector3Scale(uy, alto * 0.32f - (float)f * alto * 0.2f)));
            Vector3 dx = Vector3Scale(ux, 0.16f), dy = Vector3Scale(uy, 0.16f);
            Color hondo = TonoGR(PIEDRA_CLARA_GR, 0.6f + 0.15f * Ruido01GR(f * 2 + g, semilla + 3));
            QuadGR(Vector3Subtract(Vector3Subtract(c, dx), dy), Vector3Subtract(Vector3Add(c, dx), dy), Vector3Add(Vector3Add(c, dx), dy), Vector3Add(Vector3Subtract(c, dx), dy), hondo);
        }
    }
}


// Vasija de barro (entera o partida).
static void VasijaGR(Vector3 p, float escala, int semilla, bool rota)
{
    Color barro = VariarGR(TERRACOTA_GR, semilla, 1, 0.1f);
    if (rota)
    {
        BolaGR({ p.x, p.y + 0.25f * escala, p.z }, { 0.42f * escala, 0.3f * escala, 0.42f * escala }, barro, 2, 8, 0.0f);
        CajaGiradaGR({ p.x + 0.5f * escala, p.y + 0.05f, p.z + 0.2f }, { 0.3f * escala, 0.06f, 0.2f * escala }, 30.0f, barro);
        return;
    }
    BolaGR({ p.x, p.y + 0.4f * escala, p.z }, { 0.38f * escala, 0.42f * escala, 0.38f * escala }, barro, 4, 9);
    CilGR({ p.x, p.y + 0.72f * escala, p.z }, { p.x, p.y + 0.95f * escala, p.z }, 0.16f * escala, 0.2f * escala, 8, TonoGR(barro, 0.95f));
    CilGR({ p.x, p.y + 0.38f * escala, p.z }, { p.x, p.y + 0.46f * escala, p.z }, 0.395f * escala, 0.395f * escala, 9, Color{ 70, 52, 40, 255 }, false);
}


// Tambor de columna caido (cilindro acostado).
static void TamborGR(Vector3 p, float giro, float radio, float largo)
{
    float a = giro * DEG2RAD;
    Vector3 d = { std::cos(a) * largo * 0.5f, 0.0f, std::sin(a) * largo * 0.5f };
    CilGR({ p.x - d.x, radio, p.z - d.z }, { p.x + d.x, radio, p.z + d.z }, radio, radio, 10, VariarGR(PIEDRA_GR, (int)(p.x * 10.0f), 2, 0.08f));
    BolaGR({ p.x, radio * 1.9f, p.z }, { largo * 0.3f, 0.1f, radio * 0.6f }, MUSGO_GR, 2, 6, 0.0f);
}


static void RocaGR(Vector3 p, float r, int semilla)
{
    Color c = VariarGR(PIEDRA_OSCURA_GR, semilla, 1, 0.1f);
    BolaGR({ p.x, p.y + r * 0.35f, p.z }, { r * 1.2f, r * 0.75f, r }, c, 3, 7, -0.4f);
    BolaGR({ p.x - r * 0.1f, p.y + r * 0.85f, p.z }, { r * 0.9f, r * 0.22f, r * 0.75f }, VariarGR(MUSGO_GR, semilla, 2, 0.1f), 2, 7, 0.0f);
}


// Hongos de repisa al pie de los arboles.
static void HongosGR(Vector3 p, int semilla)
{
    for (int k = 0; k < 3; k++)
    {
        float x = p.x + 0.3f * (Ruido01GR(k, semilla) - 0.5f), z = p.z + 0.3f * (Ruido01GR(k, semilla + 1) - 0.5f);
        float h = 0.12f + 0.12f * Ruido01GR(k, semilla + 2);
        CilGR({ x, 0.0f, z }, { x, h, z }, 0.03f, 0.03f, 5, Color{ 230, 220, 196, 255 }, false);
        BolaGR({ x, h, z }, { 0.11f, 0.07f, 0.11f }, Color{ 214, 120, 70, 255 }, 2, 6, 0.0f);
    }
}


// Posiciones de los braseros (el fuego y la luz se animan aparte).
static const Vector3 BRASEROS_GR[4] =
{
    { -6.0f, Y_TERRAZA_GR + 0.06f, Z_FONDO - 0.55f },
    { 6.0f, Y_TERRAZA_GR + 0.06f, Z_FONDO - 0.55f },
    { -(X_PLAZA + 0.45f), 1.7f, Z_SUR + 0.4f },
    { X_PLAZA + 0.45f, 1.7f, Z_SUR + 0.4f }
};


static void BraseroGR(Vector3 p)
{
    for (int k = 0; k < 3; k++)
    {
        float a = 2.0f * PI * (float)k / 3.0f + 0.5f;
        CilGR({ p.x + std::cos(a) * 0.34f, p.y, p.z + std::sin(a) * 0.34f }, { p.x + std::cos(a) * 0.16f, p.y + 0.42f, p.z + std::sin(a) * 0.16f }, 0.05f, 0.04f, 4, TonoGR(BRONCE_GR, 0.7f), false);
    }
    BolaGR({ p.x, p.y + 0.62f, p.z }, { 0.42f, 0.24f, 0.42f }, BRONCE_GR, 2, 10, -0.5f * PI);
    CilGR({ p.x, p.y + 0.6f, p.z }, { p.x, p.y + 0.66f, p.z }, 0.44f, 0.46f, 10, TonoGR(BRONCE_GR, 1.15f), false);
    DiscoGR({ p.x, p.y + 0.63f, p.z }, 0.38f, 0.38f, 10, Color{ 120, 60, 30, 255 }, Color{ 60, 36, 26, 255 });
}


static void UtileriaGR(int paso)
{
    float xe = X_PLAZA + MURO_GROSOR_GR;

    // Lado oeste: cabeza colosal caida, cuerpo derribado, estela y vasijas.
    CabezaCaidaGR({ -(xe + 2.3f), 0.0f, 3.2f }, 35.0f, 1.0f);
    CuerpoCaidoGR({ -(xe + 3.6f), 0.0f, 6.6f }, -15.0f);
    EstelaGR({ -(xe + 1.4f), 0.0f, -8.4f }, 80.0f, -9.0f, 1);
    EstelaGR({ -(xe + 3.8f), 0.0f, -4.0f }, 70.0f, 12.0f, 2);
    TamborGR({ -(xe + 2.0f), 0.0f, -1.2f }, 20.0f, 0.55f, 1.4f);
    TamborGR({ -(xe + 3.6f), 0.0f, 0.4f }, 75.0f, 0.55f, 1.1f);

    // Lado este: estatua sin cabeza, estela, tambores y vasijas.
    EstatuaGR({ xe + 2.0f, 0.0f, -6.8f }, -80.0f);
    EstelaGR({ xe + 1.5f, 0.0f, 2.2f }, -76.0f, 7.0f, 3);
    TamborGR({ xe + 2.2f, 0.0f, 6.0f }, -30.0f, 0.55f, 1.5f);
    TamborGR({ xe + 3.4f, 0.0f, 7.4f }, 50.0f, 0.55f, 1.0f);
    CabezaCaidaGR({ xe + 3.4f, 0.0f, -1.6f }, -120.0f, 0.7f);

    // Vasijas en grupos junto a los muros.
    static const float VASIJAS[][3] =
    {
        { -1.0f, 8.0f, 0.0f }, { -1.0f, 7.3f, 1.0f }, { -1.0f, 8.6f, 0.0f },
        { 1.0f, -10.6f, 0.0f }, { 1.0f, -11.3f, 1.0f }, { 1.0f, -9.9f, 0.0f },
        { -1.0f, -11.2f, 1.0f }, { 1.0f, 4.0f, 0.0f }
    };
    for (int k = 0; k < (int)(sizeof(VASIJAS) / sizeof(VASIJAS[0])); k += paso)
    {
        float lado = VASIJAS[k][0];
        VasijaGR({ lado * (xe + 0.7f + 0.5f * Ruido01GR(k, 401)), 0.0f, VASIJAS[k][1] }, 0.8f + 0.4f * Ruido01GR(k, 402), k, VASIJAS[k][2] > 0.5f);
    }

    // Rocas con musgo, hongos y helechos al pie de los muros.
    for (int k = 0; k < 16; k += paso)
    {
        float lado = (k % 2) ? 1.0f : -1.0f;
        float z = -11.0f + 20.0f * Ruido01GR(k, 410);
        RocaGR({ lado * (xe + 4.5f + 6.0f * Ruido01GR(k, 411)), 0.0f, z }, 0.5f + 0.6f * Ruido01GR(k, 412), k);
    }

    for (int k = 0; k < 34; k += paso)
    {
        float lado = (k % 2) ? 1.0f : -1.0f;
        float z = Z_FONDO + 22.0f * Ruido01GR(k, 420);
        float x = lado * (xe + 0.4f + 1.2f * Ruido01GR(k, 421));
        HelechoGR({ x, 0.0f, z }, 0.7f + 0.5f * Ruido01GR(k, 422), k + 430, Ruido01GR(k, 423) < 0.35f);
    }

    // Helechos sobre la terraza y frente al pretil.
    for (int k = 0; k < 12; k += paso)
    {
        float x = (Ruido01GR(k, 440) - 0.5f) * 18.0f;
        if (std::fabs(x) < 3.0f) x += x < 0.0f ? -2.6f : 2.6f;
        HelechoGR({ x, Y_TERRAZA_GR, Z_MURO_TEMPLO_GR - 0.5f - 0.6f * Ruido01GR(k, 441) }, 0.6f + 0.3f * Ruido01GR(k, 442), k + 450, false);
    }
    for (int k = 0; k < 10; k += paso)
    {
        float x = -X_PLAZA + 2.0f * X_PLAZA * Ruido01GR(k, 460);
        HelechoGR({ x, 0.0f, Z_SUR + 1.2f + 1.5f * Ruido01GR(k, 461) }, 0.7f + 0.5f * Ruido01GR(k, 462), k + 470, Ruido01GR(k, 463) < 0.4f);
    }

    HongosGR({ -(xe + 6.0f), 0.0f, -5.2f }, 480);
    HongosGR({ xe + 5.4f, 0.0f, -8.0f }, 481);
}


// Arboles de la selva: (x, z, alto).
static const float ARBOLES_GR[][3] =
{
    { -15.0f, -7.0f, 10.5f }, { -17.0f, 3.5f, 12.0f }, { -22.0f, -15.0f, 18.0f }, { -25.0f, 9.0f, 16.0f },
    { -31.0f, -4.0f, 19.0f }, { -19.0f, -28.0f, 17.0f }, { -34.0f, -24.0f, 18.0f }, { -21.0f, 18.0f, 15.0f },
    { 15.5f, -9.5f, 11.0f }, { 16.5f, 2.0f, 12.5f }, { 31.0f, -15.0f, 17.0f }, { 24.0f, 7.0f, 18.0f },
    { 31.0f, -6.0f, 17.0f }, { 18.0f, -30.0f, 16.0f }, { 35.0f, -26.0f, 19.0f }, { 22.0f, 17.0f, 15.0f }
};

// Cascada en un risco de la selva, al noreste de la plaza.
static const Vector3 CASCADA_GR = { 24.0f, 0.0f, -21.0f };
static const float CASCADA_ALTO_GR = 7.5f;

static const float PALMERAS_GR[][3] =
{
    { -11.8f, -13.4f, 7.0f }, { 12.2f, -13.8f, 7.5f }, { -27.0f, 16.0f, 9.0f }, { 28.0f, 14.0f, 8.5f },
    { -38.0f, 4.0f, 10.0f }, { 39.0f, -2.0f, 9.5f }, { -12.0f, 11.5f, 6.5f }, { 12.5f, 12.0f, 6.0f }
};


static void SelvaGR(bool detalle, int paso)
{
    int nArboles = (int)(sizeof(ARBOLES_GR) / sizeof(ARBOLES_GR[0]));

    for (int k = 0; k < nArboles; k++)
    {
        // En BAJA quedan solo los arboles que encuadran la plaza.
        if (!detalle && (k % 4) >= 2) continue;
        if (detalle && paso > 1 && (k % 4) == 3) continue;
        ArbolGiganteGR({ ARBOLES_GR[k][0], 0.0f, ARBOLES_GR[k][1] }, ARBOLES_GR[k][2], 500 + k * 37, detalle);
    }

    if (!detalle) return;

    int nPalmeras = (int)(sizeof(PALMERAS_GR) / sizeof(PALMERAS_GR[0]));
    for (int k = 0; k < nPalmeras; k += paso)
    {
        float inclinacion = PALMERAS_GR[k][0] < 0.0f ? 0.6f : -0.6f;
        PalmeraGR({ PALMERAS_GR[k][0], 0.0f, PALMERAS_GR[k][1] }, PALMERAS_GR[k][2], inclinacion, 700 + k * 13);
    }

    // Sotobosque: helechos grandes y plantas de hojas anchas.
    for (int k = 0; k < 60; k += paso)
    {
        float lado = (k % 2) ? 1.0f : -1.0f;
        float x = lado * (X_PLAZA + 4.0f + 30.0f * Ruido01GR(k, 720));
        float z = Z_BORDE_MESETA_GR + 4.0f + 60.0f * Ruido01GR(k, 721);
        HelechoGR({ x, 0.0f, z }, 1.0f + 1.2f * Ruido01GR(k, 722), k + 730, Ruido01GR(k, 723) < 0.5f);
    }

    // Risco de la cascada con su poza (el agua se anima aparte).
    {
        Color roca = Color{ 124, 120, 100, 255 };
        for (int k = 0; k < 7; k++)
        {
            float x = CASCADA_GR.x - 5.0f + (float)k * 1.7f;
            float alto = CASCADA_ALTO_GR * (0.75f + 0.35f * Ruido01GR(k, 760)) * (1.0f - 0.12f * std::fabs((float)k - 3.0f));
            CajaGiradaGR({ x, alto * 0.5f, CASCADA_GR.z - 1.6f - 0.6f * Ruido01GR(k, 761) }, { 2.0f, alto, 3.0f }, 10.0f * (Ruido01GR(k, 762) - 0.5f), VariarGR(roca, k, 763, 0.1f));
            BolaGR({ x, alto + 0.2f, CASCADA_GR.z - 1.8f }, { 1.4f, 0.6f, 1.6f }, VariarGR(HOJA_GR, k, 764, 0.12f), 2, 7, 0.0f);
        }
        DiscoGR({ CASCADA_GR.x, 0.02f, CASCADA_GR.z + 1.6f }, 3.4f, 2.0f, 16, Color{ 70, 130, 130, 255 }, Color{ 90, 150, 140, 255 });
        for (int k = 0; k < 8; k++)
        {
            float a = 2.0f * PI * (float)k / 8.0f;
            RocaGR({ CASCADA_GR.x + std::cos(a) * 3.6f, 0.0f, CASCADA_GR.z + 1.6f + std::sin(a) * 2.2f }, 0.45f + 0.3f * Ruido01GR(k, 765), 770 + k);
        }
    }

    // Matorral a los costados de la plaza (masas de hojas en dos verdes).
    for (int k = 0; k < 28; k += paso)
    {
        float lado = (k % 2) ? 1.0f : -1.0f;
        float x = lado * (X_PLAZA + 5.5f + 14.0f * Ruido01GR(k, 750));
        float z = -16.0f + 30.0f * Ruido01GR(k, 751);
        float r = 0.9f + 1.1f * Ruido01GR(k, 752);
        Color c = VariarGR(Ruido01GR(k, 753) < 0.5f ? HOJA_GR : MezclaGR(HOJA_GR, HOJA_CLARA_GR, 0.45f), k, 754, 0.1f);
        BolaGR({ x, r * 0.25f, z }, { r * 1.3f, r * 0.8f, r }, c, 3, 8, -0.3f);
        BolaGR({ x + r * 0.7f, r * 0.2f, z + r * 0.3f }, { r * 0.8f, r * 0.55f, r * 0.7f }, TonoGR(c, 1.1f), 2, 7, 0.0f);
    }

    // Arbustos detras del templo.
    for (int k = 0; k < 18; k += paso)
    {
        float x = (Ruido01GR(k, 740) - 0.5f) * 70.0f;
        float z = Z_BORDE_MESETA_GR + 2.0f + 8.0f * Ruido01GR(k, 741);
        float r = 1.4f + 1.8f * Ruido01GR(k, 742);
        BolaGR({ x, r * 0.3f, z }, { r * 1.4f, r, r }, VariarGR(HOJA_GR, k, 743, 0.12f), 3, 8, -0.3f);
    }
}


static void MundoEstaticoGR(bool decoracion, int paso)
{
    PlazaGR(decoracion);
    BordesPlazaGR(decoracion);
    MuroTemploGR(decoracion);
    TemploGR(decoracion);
    SueloSelvaGR(decoracion);
    SelvaGR(decoracion, paso);

    for (int k = 0; k < 4; k++)
    {
        BraseroGR(BRASEROS_GR[k]);
    }

    if (decoracion)
    {
        UtileriaGR(paso);
    }
}


//--------------------------------------------------
// Luz del sol entre las copas (malla aditiva)
//--------------------------------------------------

// Manchas de sol en el suelo: (x, z, radio).
static const float MANCHAS_SOL_GR[][3] =
{
    { -3.6f, 4.6f, 1.9f }, { 4.4f, -1.8f, 2.1f }, { 1.2f, 7.4f, 1.3f }, { -5.6f, -4.2f, 1.4f },
    { 0.6f, -9.2f, 1.6f }, { -13.0f, -1.0f, 2.2f }, { 13.4f, 8.6f, 2.0f }, { 12.0f, -12.0f, 1.8f }
};


static void LuzEstaticaGR(bool decoracion)
{
    int manchas = decoracion ? (int)(sizeof(MANCHAS_SOL_GR) / sizeof(MANCHAS_SOL_GR[0])) : 4;
    Vector3 sol = Vector3Normalize(SOL_DIR_GR);
    Color tibio = { 255, 236, 170, 255 };

    for (int k = 0; k < manchas; k++)
    {
        float x = MANCHAS_SOL_GR[k][0], z = MANCHAS_SOL_GR[k][1], r = MANCHAS_SOL_GR[k][2];
        float y = (std::fabs(x) < X_PLAZA && z > Z_FONDO && z < Z_SUR) ? 0.03f : 0.01f;

        // Charco de luz: discos concentricos (ovalados segun el sol).
        DiscoGR({ x, y, z }, r * 1.3f, r * 1.0f, 18, AlfaGR(tibio, 0.10f), AlfaGR(tibio, 0.0f), false);
        DiscoGR({ x, y + 0.002f, z }, r * 0.75f, r * 0.6f, 16, AlfaGR(tibio, 0.12f), AlfaGR(tibio, 0.02f), false);

        // Moteado de hojas: puntitos de luz alrededor.
        if (decoracion)
        {
            for (int m = 0; m < 6; m++)
            {
                float a = 2.0f * PI * Ruido01GR(m, k * 7 + 801);
                float d = r * (1.1f + 0.8f * Ruido01GR(m, k * 7 + 802));
                float rm = 0.18f + 0.22f * Ruido01GR(m, k * 7 + 803);
                DiscoGR({ x + std::cos(a) * d, y + 0.003f, z + std::sin(a) * d * 0.8f }, rm * 1.2f, rm, 8, AlfaGR(tibio, 0.12f), AlfaGR(tibio, 0.0f), false);
            }
        }

        // Rayo de sol: tira desde las copas hasta la mancha.
        if (k < (decoracion ? 6 : 3))
        {
            Vector3 abajo = { x, 0.0f, z };
            Vector3 arriba = Vector3Add(abajo, Vector3Scale(sol, 30.0f));
            Vector3 medio = Vector3Add(abajo, Vector3Scale(sol, 9.0f));
            TiraGR(arriba, medio, r * 1.8f, r * 1.5f, AlfaGR(tibio, 0.0f), AlfaGR(tibio, 0.045f));
            TiraGR(medio, abajo, r * 1.5f, r * 1.4f, AlfaGR(tibio, 0.045f), AlfaGR(tibio, 0.02f));
            TiraGR(arriba, medio, r * 0.8f, r * 0.7f, AlfaGR(tibio, 0.0f), AlfaGR(tibio, 0.04f));
            TiraGR(medio, abajo, r * 0.7f, r * 0.6f, AlfaGR(tibio, 0.04f), AlfaGR(tibio, 0.015f));
        }
    }
}


//--------------------------------------------------
// Camara de la vista y reloj visual
//--------------------------------------------------

static float g_finGR = 0.0f;        // segundos desde el final (tablon)


static Camera3D VistaGR(const MinijuegoGuardianRuinas& m)
{
    Camera3D v = m.camara;
    v.position = OJO_GR;
    v.target = { 0.0f, 0.2f, -3.2f };
    v.fovy = 50.0f;

    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    if (m.fase == FASE_GUARDIAN_PREPARACION && m.partidaValida)
    {
        float u = Lim01GR(m.tiempoPreparacion / DURACION_PREPARACION_GUARDIAN);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        v.position.y += 5.0f * suave;
        v.position.z += 4.5f * suave;
        v.target.z -= 2.5f * suave;
    }

    return AplicarTemblorGeneralACamara(v);
}


//--------------------------------------------------
// Columnas (se dibujan cada frame: tiemblan y caen)
//--------------------------------------------------

static float AltoColumnaGR(int c)
{
    return 3.2f + 1.2f * Ruido01GR(c, 901);
}


// Fuste en tambores entre la base y la punta (sirve de pie o caido).
static void FusteGR(Vector3 base, Vector3 punta, float radio, int c, Color piedra)
{
    Vector3 eje = Vector3Subtract(punta, base);
    int tambores = 3;

    for (int t = 0; t < tambores; t++)
    {
        float u0 = (float)t / (float)tambores, u1 = (float)(t + 1) / (float)tambores;
        Vector3 a = Vector3Add(base, Vector3Scale(eje, u0 + 0.008f));
        Vector3 b = Vector3Add(base, Vector3Scale(eje, u1 - 0.008f));
        Color col = VariarGR(piedra, c * 5 + t, 902, 0.06f);
        CilGR(a, b, radio * (1.0f - 0.03f * (float)t), radio * (1.0f - 0.03f * (float)(t + 1)), 12, col, t == tambores - 1);

        // Estrias: franjas apenas mas oscuras.
        CilGR(Vector3Add(base, Vector3Scale(eje, u1 - 0.012f)), Vector3Add(base, Vector3Scale(eje, u1 + 0.004f)), radio * 0.93f, radio * 0.93f, 12, JUNTA_GR, false);
    }
}


static void ColumnaGR(const ColumnaGuardian& columna, int c, float t, bool detalle)
{
    float x = columna.x, z = columna.z;
    float alto = AltoColumnaGR(c);
    float r = RADIO_COLUMNA * 0.97f;
    float angulo = 2.0f * PI * Ruido01GR(c, 903);
    Vector3 dirCaida = { std::cos(angulo), 0.0f, std::sin(angulo) };

    // Pedestal (queda aunque la columna caiga).
    CajaGR({ x, 0.15f, z }, { 1.34f, 0.3f, 1.34f }, PIEDRA_OSCURA_GR);
    CajaGR({ x, 0.36f, z }, { 1.2f, 0.12f, 1.2f }, PIEDRA_CLARA_GR);

    if (columna.estado == COLUMNA_GUARDIAN_CAIDA)
    {
        float u = Lim01GR(columna.tiempo / 0.55f);
        float caida = u * u;

        // Muñon partido.
        CilGR({ x, 0.42f, z }, { x, 0.62f, z }, r, r * 0.96f, 12, PIEDRA_GR);
        for (int k = 0; k < 5; k++)
        {
            float a = 2.0f * PI * (float)k / 5.0f;
            Vector3 p = { x + std::cos(a) * r * 0.6f, 0.62f, z + std::sin(a) * r * 0.6f };
            TriGR(p, { x + std::cos(a + 0.6f) * r * 0.95f, 0.62f, z + std::sin(a + 0.6f) * r * 0.95f }, { p.x, 0.62f + 0.12f + 0.1f * Ruido01GR(k, c), p.z }, PIEDRA_CLARA_GR);
        }

        if (u < 1.0f)
        {
            // Se vuelca hacia su lado de caida.
            float giro = caida * 0.5f * PI;
            Vector3 eje = Vector3Add(Vector3Scale({ 0.0f, 1.0f, 0.0f }, std::cos(giro)), Vector3Scale(dirCaida, std::sin(giro)));
            Vector3 base = { x, 0.62f, z };
            FusteGR(base, Vector3Add(base, Vector3Scale(eje, alto - 0.62f)), r, c, PIEDRA_GR);
        }
        else
        {
            // Cascotes bajos (los orbes pasan por encima).
            for (int k = 0; k < 7; k++)
            {
                float d = 0.9f + 2.6f * ((float)k / 7.0f) + 0.3f * Ruido01GR(k, c + 904);
                float lat = 0.6f * (Ruido01GR(k, c + 905) - 0.5f);
                Vector3 p = { x + dirCaida.x * d - dirCaida.z * lat, 0.0f, z + dirCaida.z * d + dirCaida.x * lat };
                float s = 0.28f + 0.2f * Ruido01GR(k, c + 906);
                CajaGiradaGR({ p.x, s * 0.5f, p.z }, { s * 1.6f, s, s * 1.2f }, 360.0f * Ruido01GR(k, c + 907), VariarGR(PIEDRA_GR, k, c + 908, 0.1f), 12.0f * (Ruido01GR(k, c + 909) - 0.5f));
            }
        }
        return;
    }

    // Columna en pie (tiembla durante el aviso).
    float temblor = 0.0f;
    if (columna.estado == COLUMNA_GUARDIAN_AVISO)
    {
        float fuerza = 1.0f - Lim01GR(columna.tiempo / AVISO_COLUMNA) * 0.6f;
        temblor = 0.06f * fuerza * std::sin(t * 55.0f + (float)c);
    }

    Vector3 base = { x, 0.42f, z };
    Vector3 punta = { x + temblor * alto * 0.4f, alto, z + temblor * alto * 0.2f };
    FusteGR(base, punta, r, c, PIEDRA_GR);

    // Remate roto: dientes de piedra o capitel en las mas altas.
    if (alto > 4.0f)
    {
        CajaGR({ punta.x, alto + 0.12f, punta.z }, { 1.3f, 0.24f, 1.3f }, PIEDRA_CLARA_GR);
        CajaGR({ punta.x, alto + 0.27f, punta.z }, { 1.2f, 0.06f, 1.2f }, MUSGO_GR);
    }
    else
    {
        for (int k = 0; k < 6; k++)
        {
            float a = 2.0f * PI * (float)k / 6.0f + 0.3f;
            float h = 0.15f + 0.3f * Ruido01GR(k, c + 910);
            Vector3 p0 = { punta.x + std::cos(a) * r * 0.9f, alto, punta.z + std::sin(a) * r * 0.9f };
            Vector3 p1 = { punta.x + std::cos(a + 1.05f) * r * 0.9f, alto, punta.z + std::sin(a + 1.05f) * r * 0.9f };
            Vector3 cima = { punta.x + std::cos(a + 0.5f) * r * 0.55f, alto + h, punta.z + std::sin(a + 0.5f) * r * 0.55f };
            TriGR(p0, p1, cima, CaraGR(PIEDRA_CLARA_GR, Vector3Normalize({ std::cos(a + 0.5f), 0.6f, std::sin(a + 0.5f) })));
        }
        BolaGR({ punta.x, alto + 0.02f, punta.z }, { r * 0.8f, 0.1f, r * 0.8f }, MUSGO_GR, 2, 8, 0.0f);
    }

    // Banda de musgo y enredadera en espiral con hojas.
    CilGR({ base.x, 0.42f, base.z }, { base.x + temblor * 0.3f, 0.75f, base.z }, r * 1.02f, r * 1.01f, 12, VariarGR(MUSGO_GR, c, 911, 0.08f), false);

    int vueltas = detalle ? 18 : 10;
    float fase = 2.0f * PI * Ruido01GR(c, 912);
    Color tallo = Color{ 64, 104, 48, 255 };

    for (int k = 0; k < vueltas; k++)
    {
        float u0 = (float)k / (float)vueltas, u1 = (float)(k + 1) / (float)vueltas;
        float a0 = fase + u0 * 2.6f * PI, a1 = fase + u1 * 2.6f * PI;
        float y0 = 0.6f + u0 * (alto - 0.9f), y1 = 0.6f + u1 * (alto - 0.9f);
        float s0 = temblor * y0 * 0.4f, s1 = temblor * y1 * 0.4f;
        Vector3 p0 = { x + s0 + std::cos(a0) * (r + 0.04f), y0, z + std::sin(a0) * (r + 0.04f) };
        Vector3 p1 = { x + s1 + std::cos(a1) * (r + 0.04f), y1, z + std::sin(a1) * (r + 0.04f) };
        LianaGR(p0, p1, 0.07f, tallo);

        if (k % 2 == 0)
        {
            Vector3 n = { std::cos(a1), 0.0f, std::sin(a1) };
            Vector3 hoja = Vector3Add(p1, Vector3Add(Vector3Scale(n, 0.26f), { 0.0f, -0.06f, 0.0f }));
            HojaGR(p1, hoja, 0.16f, VariarGR(HOJA_CLARA_GR, k, c + 913, 0.15f));
        }
    }
}


//--------------------------------------------------
// Jugadores: anillos, punteria, escudo del guardian
//--------------------------------------------------

// Escudo del guardian: postes de bronce y cortina de energia curva que
// mira hacia la plaza (+z). En la embestida se enciende en naranja.
static void EscudoOpacoGR(const MinijuegoGuardianRuinas& m, float t)
{
    if (m.indiceGuardian < 0) return;

    float sz = ZEscudo(m);
    float gx = m.guardianX;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = gx + (float)lado * (ESCUDO_MEDIO + 0.05f);
        float flota = 0.05f * std::sin(t * 3.0f + (float)lado);
        CilGR({ x, 0.25f + flota, sz }, { x, 1.75f + flota, sz }, 0.11f, 0.09f, 8, BRONCE_GR);
        BolaGR({ x, 1.86f + flota, sz }, { 0.16f, 0.2f, 0.16f }, TonoGR(BRONCE_GR, 1.1f), 3, 8);
        BolaGR({ x, 0.2f + flota, sz }, { 0.14f, 0.1f, 0.14f }, TonoGR(BRONCE_GR, 0.8f), 2, 8);
    }

    // Barras de bronce arriba y abajo que siguen la curva del escudo y
    // un sol tallado en el centro: se lee su frente.
    const int tramos = 8;
    for (int k = 0; k < tramos; k++)
    {
        float u0 = (float)k / (float)tramos, u1 = (float)(k + 1) / (float)tramos;
        float x0 = gx - ESCUDO_MEDIO + 2.0f * ESCUDO_MEDIO * u0, x1 = gx - ESCUDO_MEDIO + 2.0f * ESCUDO_MEDIO * u1;
        float c0 = 0.22f * (1.0f - (2.0f * u0 - 1.0f) * (2.0f * u0 - 1.0f));
        float c1 = 0.22f * (1.0f - (2.0f * u1 - 1.0f) * (2.0f * u1 - 1.0f));
        Vector3 a = { x0, 0.0f, sz + c0 }, b = { x1, 0.0f, sz + c1 };
        for (int nivel = 0; nivel < 2; nivel++)
        {
            float y = nivel == 0 ? 0.3f : 1.72f;
            Vector3 pa = { a.x, y, a.z }, pb = { b.x, y, b.z };
            Vector3 eje = Vector3Subtract(pb, pa);
            CajaEjesGR(Vector3Scale(Vector3Add(pa, pb), 0.5f), Vector3Scale(eje, 0.5f), { 0.0f, 0.06f, 0.0f }, { 0.0f, 0.0f, 0.07f }, BRONCE_GR, true);
        }
    }

    float zc = sz + 0.24f;
    for (int k = 0; k < 8; k++)
    {
        float a = 2.0f * PI * (float)k / 8.0f + t * 0.5f;
        Vector3 p = { gx + std::cos(a) * 0.32f, 1.0f + std::sin(a) * 0.32f, zc };
        Vector3 q = { gx + std::cos(a) * 0.5f, 1.0f + std::sin(a) * 0.5f, zc };
        CilGR(p, q, 0.04f, 0.0f, 4, TonoGR(BRONCE_GR, 1.1f), false);
    }
    CilGR({ gx, 1.0f, zc - 0.05f }, { gx, 1.0f, zc + 0.04f }, 0.24f, 0.24f, 10, TonoGR(BRONCE_GR, 1.15f));
}


static void EscudoTranslucidoGR(const MinijuegoGuardianRuinas& m, float t)
{
    if (m.indiceGuardian < 0) return;

    bool embistiendo = m.embestidaTiempo > 0.0f;
    float sz = ZEscudo(m);
    float gx = m.guardianX;
    Color base = embistiendo ? Color{ 255, 150, 70, 255 } : ESCUDO_GR;
    const int tramos = 12;

    for (int k = 0; k < tramos; k++)
    {
        float u0 = (float)k / (float)tramos, u1 = (float)(k + 1) / (float)tramos;
        float x0 = gx - ESCUDO_MEDIO + 2.0f * ESCUDO_MEDIO * u0, x1 = gx - ESCUDO_MEDIO + 2.0f * ESCUDO_MEDIO * u1;
        float c0 = 0.22f * (1.0f - (2.0f * u0 - 1.0f) * (2.0f * u0 - 1.0f));
        float c1 = 0.22f * (1.0f - (2.0f * u1 - 1.0f) * (2.0f * u1 - 1.0f));
        float brillo = 0.75f + 0.25f * std::sin(t * 7.0f + (float)k * 0.9f);
        float aAbajo = (embistiendo ? 0.8f : 0.55f) * brillo;
        Color abajo = AlfaGR(base, aAbajo);
        Color arriba = AlfaGR(MezclaGR(base, WHITE, 0.3f), aAbajo * 0.35f);
        QuadColGR({ x0, 0.28f, sz + c0 }, { x1, 0.28f, sz + c1 }, { x1, 1.72f, sz + c1 }, { x0, 1.72f, sz + c0 }, abajo, abajo, arriba, arriba);
    }

    // Bordes brillantes de la cortina.
    for (int k = 0; k < tramos; k++)
    {
        float u0 = (float)k / (float)tramos, u1 = (float)(k + 1) / (float)tramos;
        float x0 = gx - ESCUDO_MEDIO + 2.0f * ESCUDO_MEDIO * u0, x1 = gx - ESCUDO_MEDIO + 2.0f * ESCUDO_MEDIO * u1;
        float c0 = 0.22f * (1.0f - (2.0f * u0 - 1.0f) * (2.0f * u0 - 1.0f)) + 0.01f;
        float c1 = 0.22f * (1.0f - (2.0f * u1 - 1.0f) * (2.0f * u1 - 1.0f)) + 0.01f;
        Color borde = AlfaGR(MezclaGR(base, WHITE, 0.5f), 0.9f);
        QuadColGR({ x0, 0.26f, sz + c0 }, { x1, 0.26f, sz + c1 }, { x1, 0.36f, sz + c1 }, { x0, 0.36f, sz + c0 }, borde, borde, borde, borde);
        QuadColGR({ x0, 1.66f, sz + c0 }, { x1, 1.66f, sz + c1 }, { x1, 1.74f, sz + c1 }, { x0, 1.74f, sz + c0 }, borde, borde, borde, borde);
    }
}


// Marcas en el suelo: anillo de rol, recarga del tiro y alcance de la
// embestida (aviso para el trio).
static void MarcasSueloGR(const MinijuegoGuardianRuinas& m, const JugadorPrueba jugadores[], const Participante participantes[], int limite, float t)
{
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& e = m.estadosJugadores[i];
        if (e.equipo < 0) continue;

        Vector3 p = { jugadores[i].posicion.x, 0.05f, jugadores[i].posicion.z };
        Color c = participantes[i].color;

        if (e.equipo == 0)
        {
            // Sello del guardian: anillo dorado doble con runas que giran.
            AnilloGR(p, 0.78f, 0.9f, 24, AlfaGR(ESCUDO_GR, 0.95f), AlfaGR(ESCUDO_GR, 0.95f));
            AnilloGR(p, 0.62f, 0.7f, 24, AlfaGR(c, 0.9f), AlfaGR(c, 0.9f));
            for (int k = 0; k < 6; k++)
            {
                float a = t * 0.8f + 2.0f * PI * (float)k / 6.0f;
                Vector3 q = { p.x + std::cos(a) * 1.08f, 0.055f, p.z + std::sin(a) * 1.08f };
                AnilloGR(q, 0.0f, 0.07f, 6, AlfaGR(ESCUDO_GR, 0.9f), AlfaGR(ESCUDO_GR, 0.9f));
            }
        }
        else
        {
            // Anillo del color del jugador; el arco claro es la recarga.
            float carga = 1.0f - Lim01GR(e.recarga / RECARGA_LANZAR);
            AnilloGR(p, 0.6f, 0.7f, 24, AlfaGR(TonoGR(c, 0.55f), 0.6f), AlfaGR(TonoGR(c, 0.55f), 0.6f));
            if (carga > 0.01f)
            {
                Color lleno = carga >= 1.0f ? MezclaGR(c, WHITE, 0.25f) : c;
                AnilloGR({ p.x, 0.055f, p.z }, 0.6f, 0.72f, 24, lleno, lleno, -0.25f, -0.25f + carga);
            }
        }
    }

    // Columna por caer: anillos rojos que laten y franja hacia donde cae.
    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        const ColumnaGuardian& col = m.columnas[c];
        if (col.estado != COLUMNA_GUARDIAN_AVISO) continue;

        float pulso = 0.5f + 0.5f * std::sin(t * 14.0f);
        Vector3 p = { col.x, 0.06f, col.z };
        AnilloGR(p, 0.95f + 0.25f * pulso, 1.12f + 0.25f * pulso, 28, AlfaGR(AVISO_GR, 0.75f), AlfaGR(AVISO_GR, 0.0f));
        AnilloGR(p, 0.78f, 0.88f, 28, AlfaGR(AVISO_GR, 0.85f), AlfaGR(AVISO_GR, 0.85f));

        float angulo = 2.0f * PI * Ruido01GR(c, 903);
        Vector3 d = { std::cos(angulo), 0.0f, std::sin(angulo) };
        Vector3 l = { -d.z * 0.5f, 0.0f, d.x * 0.5f };
        for (int k = 0; k < 4; k++)
        {
            float d0 = 1.2f + (float)k * 0.65f, d1 = d0 + 0.4f;
            Color cc = AlfaGR(AVISO_GR, (0.45f - 0.09f * (float)k) * (0.6f + 0.4f * pulso));
            Vector3 a = Vector3Add(p, Vector3Scale(d, d0)), b = Vector3Add(p, Vector3Scale(d, d1));
            QuadColGR(Vector3Subtract(a, l), Vector3Add(a, l), Vector3Add(b, l), Vector3Subtract(b, l), cc, cc, cc, cc);
        }

        // Polvo que se desprende del remate.
        for (int k = 0; k < 6; k++)
        {
            float u = FraccionGR(t * 1.4f + (float)k / 6.0f);
            float a = 2.0f * PI * Ruido01GR(k, c + 920);
            Vector3 q = { col.x + std::cos(a) * 0.5f, AltoColumnaGR(c) * (1.0f - u), col.z + std::sin(a) * 0.5f };
            PuntoGR(q, 0.06f, Color{ 200, 186, 150, (unsigned char)(200.0f * (1.0f - u)) }, (float)k);
        }
    }

    // Alcance de la embestida delante del escudo.
    if (m.indiceGuardian >= 0 && m.fase == FASE_GUARDIAN_JUGANDO)
    {
        bool lista = m.embestidaRecarga <= 0.0f && m.embestidaTiempo <= 0.0f;
        bool embistiendo = m.embestidaTiempo > 0.0f;
        float z0 = GUARDIAN_Z + ESCUDO_DISTANCIA + ESCUDO_FONDO;
        float gx = m.guardianX;

        if (lista || embistiendo)
        {
            float alfa = embistiendo ? 0.55f : 0.16f + 0.08f * std::sin(t * 4.0f);
            Color col = embistiendo ? Color{ 255, 150, 70, 255 } : ESCUDO_GR;

            for (int k = 0; k < 3; k++)
            {
                float zc = z0 + 0.45f + (float)k * 0.62f;
                float avance = embistiendo ? 0.0f : std::fmod(t * 0.9f, 1.0f) * 0.62f;
                zc += avance;
                if (zc > z0 + EMBESTIDA_ALCANCE) continue;
                Color cc = AlfaGR(col, alfa * (1.0f - 0.25f * (float)k));
                // Cheurones que apuntan hacia la plaza.
                QuadColGR({ gx - 1.5f, 0.05f, zc - 0.22f }, { gx - 1.5f, 0.05f, zc - 0.04f }, { gx, 0.05f, zc + 0.3f }, { gx, 0.05f, zc + 0.12f }, cc, cc, cc, cc);
                QuadColGR({ gx, 0.05f, zc + 0.12f }, { gx, 0.05f, zc + 0.3f }, { gx + 1.5f, 0.05f, zc - 0.04f }, { gx + 1.5f, 0.05f, zc - 0.22f }, cc, cc, cc, cc);
            }
        }
    }
}


// Linea de punteria de los atacantes humanos (piedritas de luz).
static void PunteriaGR(const MinijuegoGuardianRuinas& m, const JugadorPrueba jugadores[], const Participante participantes[], int limite, float t)
{
    if (m.fase != FASE_GUARDIAN_JUGANDO) return;

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& e = m.estadosJugadores[i];
        if (e.equipo != 1 || JugadorEsBot(participantes[i])) continue;

        Vector3 a = { jugadores[i].posicion.x, 0.06f, jugadores[i].posicion.z };
        Vector3 b = { e.apuntarX, 0.06f, Z_FONDO };
        float largo = Vector3Distance(a, b);
        int n = (int)(largo / 0.7f);
        float lista = e.recarga <= 0.0f && e.aturdimiento <= 0.0f ? 1.0f : 0.45f;

        for (int k = 1; k < n; k++)
        {
            float u = ((float)k + std::fmod(t * 2.0f, 1.0f)) / (float)n;
            if (u > 1.0f) continue;
            Vector3 p = Vector3Lerp(a, b, u);
            Color c = AlfaGR(MezclaGR(participantes[i].color, WHITE, 0.3f), (0.75f - 0.5f * u) * lista);
            AnilloGR(p, 0.0f, 0.09f, 6, c, c);
        }
    }
}


//--------------------------------------------------
// Portal runico (energia, contador de orbes)
//--------------------------------------------------

// Contador de orbes: piedras runicas en el frente de las jambas, que se
// encienden alternando izquierda y derecha desde abajo.
static Vector3 PosRunaGR(int i, int meta)
{
    int filas = (meta + 1) / 2;
    float paso = filas > 1 ? (PORTAL_ALTO_GR - 1.5f) / (float)(filas - 1) : 0.0f;
    if (paso > 1.0f) paso = 1.0f;
    float lado = (i % 2 == 0) ? -1.0f : 1.0f;
    return { lado * (PORTAL_MEDIO + PORTAL_JAMBA_GR * 0.5f), 0.95f + (float)(i / 2) * paso, Z_FONDO + 0.17f };
}


// Engastes del contador (piedra oscura o runa encendida).
static void RunasOpacasGR(const MinijuegoGuardianRuinas& m)
{
    int meta = m.metaOrbes > 0 ? m.metaOrbes : 1;

    for (int i = 0; i < meta; i++)
    {
        Vector3 p = PosRunaGR(i, meta);
        bool encendida = i < m.goles;
        Color piedra = encendida ? RUNA_GR : TonoGR(PIEDRA_OSCURA_GR, 0.75f);
        Color marco = TonoGR(PIEDRA_OSCURA_GR, 0.55f);
        float g = 0.32f;

        QuadColGR({ p.x - g, p.y - g, p.z }, { p.x + g, p.y - g, p.z }, { p.x + g, p.y + g, p.z }, { p.x - g, p.y + g, p.z }, marco, marco, marco, marco);

        for (int k = 0; k < 8; k++)
        {
            float a0 = 2.0f * PI * (float)k / 8.0f, a1 = 2.0f * PI * (float)(k + 1) / 8.0f;
            Color c = encendida ? MezclaGR(RUNA_CLARA_GR, RUNA_GR, 0.4f + 0.2f * (float)(k % 2)) : TonoGR(piedra, k % 2 ? 1.0f : 0.85f);
            VGR({ p.x, p.y, p.z + 0.02f }, encendida ? RUNA_CLARA_GR : piedra);
            VGR({ p.x + std::cos(a0) * 0.25f, p.y + std::sin(a0) * 0.25f, p.z + 0.02f }, c);
            VGR({ p.x + std::cos(a1) * 0.25f, p.y + std::sin(a1) * 0.25f, p.z + 0.02f }, c);
        }

        // Glifo tallado en las apagadas.
        if (!encendida)
        {
            Color h = TonoGR(PIEDRA_OSCURA_GR, 0.45f);
            QuadColGR({ p.x - 0.12f, p.y - 0.03f, p.z + 0.03f }, { p.x + 0.12f, p.y - 0.03f, p.z + 0.03f }, { p.x + 0.12f, p.y + 0.03f, p.z + 0.03f }, { p.x - 0.12f, p.y + 0.03f, p.z + 0.03f }, h, h, h, h);
            QuadColGR({ p.x - 0.03f, p.y - 0.12f, p.z + 0.03f }, { p.x + 0.03f, p.y - 0.12f, p.z + 0.03f }, { p.x + 0.03f, p.y + 0.12f, p.z + 0.03f }, { p.x - 0.03f, p.y + 0.12f, p.z + 0.03f }, h, h, h, h);
        }
    }
}


// Velo del portal: remolino de energia en el vano (mezcla normal).
static void VeloPortalGR(const MinijuegoGuardianRuinas& m, float t, float progreso)
{
    float z = Z_FONDO - 0.5f;
    float gol = Lim01GR(m.tiempoGol / 1.2f);
    const int nx = 8, ny = 10;
    float cy = PORTAL_ALTO_GR * 0.45f;

    auto ColorEn = [&](float x, float y)
    {
        float dx = x / PORTAL_MEDIO, dy = (y - cy) / (PORTAL_ALTO_GR * 0.55f);
        float r = std::sqrt(dx * dx + dy * dy);
        float a = std::atan2(dy, dx);
        float remolino = 0.5f + 0.5f * std::sin(a * 3.0f - r * 6.0f + t * 3.2f);
        float alfa = 0.42f + 0.25f * remolino + 0.18f * progreso + 0.3f * gol;
        Color c = MezclaGR(Color{ 30, 150, 150, 255 }, RUNA_GR, remolino * 0.7f + 0.3f * progreso);
        c = MezclaGR(c, RUNA_CLARA_GR, Lim01GR(1.0f - r) * 0.5f + gol * 0.6f);
        return AlfaGR(c, alfa);
    };

    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            float x0 = -PORTAL_MEDIO + 2.0f * PORTAL_MEDIO * (float)i / (float)nx, x1 = -PORTAL_MEDIO + 2.0f * PORTAL_MEDIO * (float)(i + 1) / (float)nx;
            float y0 = PORTAL_ALTO_GR * (float)j / (float)ny, y1 = PORTAL_ALTO_GR * (float)(j + 1) / (float)ny;
            QuadColGR({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, ColorEn(x0, y0), ColorEn(x1, y0), ColorEn(x1, y1), ColorEn(x0, y1));
        }
    }
}


//--------------------------------------------------
// Orbes
//--------------------------------------------------

static const float Y_ORBE_GR = 0.9f;


static void ColoresOrbeGR(const OrbeGuardian& o, Color& nucleo, Color& halo)
{
    if (o.devuelto)
    {
        nucleo = o.fuerte ? Color{ 255, 236, 200, 255 } : Color{ 255, 214, 160, 255 };
        halo = o.fuerte ? Color{ 255, 80, 40, 255 } : DEVUELTO_GR;
    }
    else
    {
        nucleo = RUNA_CLARA_GR;
        halo = RUNA_GR;
    }
}


// Sombras de los orbes en el suelo (mezcla normal).
static void SombrasOrbesGR(const MinijuegoGuardianRuinas& m)
{
    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        const OrbeGuardian& o = m.orbes[k];
        if (!o.activo) continue;
        DiscoGR({ o.x, 0.04f, o.z }, 0.34f, 0.3f, 10, Color{ 20, 30, 20, 120 }, Color{ 20, 30, 20, 0 }, false);
    }
}


// Nucleo del orbe (opaco, sin bruma) con puas si fue devuelto.
static void NucleosOrbesGR(const MinijuegoGuardianRuinas& m, float t)
{
    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        const OrbeGuardian& o = m.orbes[k];
        if (!o.activo) continue;

        Color nucleo, halo;
        ColoresOrbeGR(o, nucleo, halo);
        float pulso = 1.0f + 0.1f * std::sin(t * 22.0f + (float)k);
        float r = RADIO_ORBE * 0.72f * pulso * (o.fuerte ? 1.15f : 1.0f);
        Vector3 c = { o.x, Y_ORBE_GR, o.z };

        for (int i = 0; i < 3; i++)
        {
            float la0 = -0.5f * PI + PI * (float)i / 3.0f, la1 = -0.5f * PI + PI * (float)(i + 1) / 3.0f;
            for (int j = 0; j < 8; j++)
            {
                float lo0 = 2.0f * PI * (float)j / 8.0f, lo1 = 2.0f * PI * (float)(j + 1) / 8.0f;
                auto P = [&](float la, float lo) { return Vector3{ c.x + r * std::cos(la) * std::cos(lo), c.y + r * std::sin(la), c.z + r * std::cos(la) * std::sin(lo) }; };
                Color col = MezclaGR(halo, nucleo, 0.45f + 0.35f * std::sin(la1));
                QuadColGR(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), col, col, col, col);
            }
        }

        if (o.devuelto)
        {
            // Puas que giran: peligro para el trio.
            for (int p = 0; p < 6; p++)
            {
                float a = t * 9.0f + 2.0f * PI * (float)p / 6.0f;
                Vector3 d = { std::cos(a), 0.35f * std::sin(a * 2.0f), std::sin(a) };
                Vector3 b0 = Vector3Add(c, Vector3Scale(d, r * 0.8f));
                Vector3 b1 = Vector3Add(c, Vector3Scale(d, r * 1.8f));
                TiraGR(b0, b1, 0.12f, 0.0f, halo, halo);
            }
        }
    }
}


// Halos y estelas aditivas de los orbes.
static void LucesOrbesGR(const MinijuegoGuardianRuinas& m, float t)
{
    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        const OrbeGuardian& o = m.orbes[k];
        if (!o.activo) continue;

        Color nucleo, halo;
        ColoresOrbeGR(o, nucleo, halo);
        Vector3 c = { o.x, Y_ORBE_GR, o.z };
        float v = std::sqrt(o.vx * o.vx + o.vz * o.vz);
        Vector3 dir = v > 0.01f ? Vector3{ o.vx / v, 0.0f, o.vz / v } : Vector3{ 0.0f, 0.0f, 1.0f };
        float largo = 0.6f + v * 0.12f;

        TiraGR(c, Vector3Subtract(c, Vector3Scale(dir, largo)), RADIO_ORBE * 1.6f, 0.0f, AlfaGR(halo, 0.55f), AlfaGR(halo, 0.0f));
        TiraGR(c, Vector3Subtract(c, Vector3Scale(dir, largo * 0.6f)), RADIO_ORBE * 0.8f, 0.0f, AlfaGR(nucleo, 0.7f), AlfaGR(nucleo, 0.0f));
        HaloGR(c, RADIO_ORBE * 2.6f, AlfaGR(halo, 0.35f), AlfaGR(halo, 0.0f), 12);
        HaloGR(c, RADIO_ORBE * 1.2f, AlfaGR(nucleo, 0.5f), AlfaGR(halo, 0.0f), 10);
        DiscoGR({ o.x, 0.05f, o.z }, 1.1f, 1.0f, 12, AlfaGR(halo, 0.16f), AlfaGR(halo, 0.0f), false);
        (void)t;
    }
}


//--------------------------------------------------
// Vida ambiental: hojas, polen, mariposas, loros, cascada
//--------------------------------------------------

// Hojas que caen girando desde las copas (sin estado: dependen del reloj).
static void HojasQueCaenGR(float t, int cantidad)
{
    for (int k = 0; k < cantidad; k++)
    {
        float periodo = 7.0f + 4.0f * Ruido01GR(k, 1001);
        float u = FraccionGR(t / periodo + Ruido01GR(k, 1002));
        float x = -18.0f + 36.0f * Ruido01GR(k, 1003) + 1.2f * std::sin(t * 1.3f + (float)k);
        float z = -14.0f + 24.0f * Ruido01GR(k, 1004) + 0.8f * std::cos(t * 1.1f + (float)k);
        float y = 13.0f * (1.0f - u);
        Color c = Ruido01GR(k, 1005) < 0.3f ? Color{ 214, 176, 70, 255 } : VariarGR(HOJA_CLARA_GR, k, 1006, 0.15f);
        float alfa = Lim01GR(u * 8.0f) * Lim01GR((1.0f - u) * 6.0f);
        PuntoGR({ x, y + 0.05f, z }, 0.075f, AlfaGR(c, alfa), t * 3.0f + (float)k);
    }
}


// Motas de polen que flotan en los rayos de sol (aditivo).
static void PolenGR(float t, int cantidad)
{
    int manchas = 6;

    for (int k = 0; k < cantidad; k++)
    {
        int m = k % manchas;
        float u = FraccionGR(t * 0.05f + Ruido01GR(k, 1011));
        Vector3 base = { MANCHAS_SOL_GR[m][0], 0.0f, MANCHAS_SOL_GR[m][1] };
        Vector3 p = Vector3Add(base, Vector3Scale(Vector3Normalize(SOL_DIR_GR), 1.0f + 12.0f * Ruido01GR(k, 1012)));
        p.x += 0.9f * std::sin(t * 0.7f + (float)k * 1.7f) + (Ruido01GR(k, 1013) - 0.5f) * MANCHAS_SOL_GR[m][2];
        p.y += 0.6f * std::sin(u * 2.0f * PI);
        p.z += 0.6f * std::cos(t * 0.5f + (float)k);
        float brillo = 0.5f + 0.5f * std::sin(t * 3.0f + (float)k * 2.3f);
        PuntoGR(p, 0.045f, Color{ 255, 244, 190, (unsigned char)(70 + 110 * brillo) });
    }
}


// Mariposa: dos alas que aletean.
static void MariposaGR(Vector3 c, float t, float rumbo, Color color)
{
    float aleteo = std::sin(t * 18.0f) * 0.9f;
    Vector3 adelante = { std::cos(rumbo), 0.0f, std::sin(rumbo) };
    Vector3 lado = { -adelante.z, 0.0f, adelante.x };

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 ala = Vector3Add(Vector3Scale(lado, (float)s * 0.16f * std::cos(aleteo)), { 0.0f, 0.16f * std::fabs(std::sin(aleteo)), 0.0f });
        Vector3 p0 = Vector3Add(c, Vector3Scale(adelante, 0.07f));
        Vector3 p1 = Vector3Subtract(c, Vector3Scale(adelante, 0.07f));
        Vector3 p2 = Vector3Add(Vector3Add(c, ala), Vector3Scale(adelante, 0.08f));
        Vector3 p3 = Vector3Subtract(Vector3Add(c, ala), Vector3Scale(adelante, 0.1f));
        QuadColGR(p0, p2, p3, p1, color, color, TonoGR(color, 0.7f), TonoGR(color, 0.7f));
    }
}


static void MariposasGR(float t, int cantidad)
{
    static const Color COLORES[3] = { { 70, 150, 255, 255 }, { 255, 170, 40, 255 }, { 250, 240, 120, 255 } };

    for (int k = 0; k < cantidad; k++)
    {
        float lado = (k % 2) ? 1.0f : -1.0f;
        Vector3 centro = { lado * (X_PLAZA + 2.5f + 3.0f * Ruido01GR(k, 1021)), 1.2f, -8.0f + 14.0f * Ruido01GR(k, 1022) };
        float a = t * (0.6f + 0.3f * Ruido01GR(k, 1023)) + 6.0f * Ruido01GR(k, 1024);
        Vector3 p = { centro.x + std::cos(a) * 1.6f, centro.y + 0.5f * std::sin(a * 2.3f), centro.z + std::sin(a) * 1.2f };
        MariposaGR(p, t + (float)k, a + 0.5f * PI, COLORES[k % 3]);
    }
}


// Cascada: lamina de agua con franjas que bajan, espuma y la poza que
// tiembla (mezcla normal).
static void CascadaGR(float t, bool animada)
{
    float yTop = CASCADA_ALTO_GR * 0.86f;
    float z = CASCADA_GR.z + 0.15f;
    const int columnas = 6, filas = 8;
    float ancho = 2.6f;

    for (int i = 0; i < columnas; i++)
    {
        float x0 = CASCADA_GR.x - ancho * 0.5f + ancho * (float)i / (float)columnas;
        float x1 = x0 + ancho / (float)columnas;

        for (int j = 0; j < filas; j++)
        {
            float v0 = (float)j / (float)filas, v1 = (float)(j + 1) / (float)filas;
            float y0 = yTop * (1.0f - v0), y1 = yTop * (1.0f - v1);
            // La lamina se abre y se adelanta al caer.
            float e0 = 1.0f + 0.25f * v0, e1 = 1.0f + 0.25f * v1;
            float zz0 = z + 0.9f * v0 * v0, zz1 = z + 0.9f * v1 * v1;
            float xa0 = CASCADA_GR.x + (x0 - CASCADA_GR.x) * e0, xb0 = CASCADA_GR.x + (x1 - CASCADA_GR.x) * e0;
            float xa1 = CASCADA_GR.x + (x0 - CASCADA_GR.x) * e1, xb1 = CASCADA_GR.x + (x1 - CASCADA_GR.x) * e1;
            float f0 = animada ? FraccionGR(v0 * 2.5f - t * 1.6f + 0.37f * (float)i) : 0.5f;
            float f1 = animada ? FraccionGR(v1 * 2.5f - t * 1.6f + 0.37f * (float)i) : 0.5f;
            Color c0 = MezclaGR(Color{ 150, 210, 220, 200 }, Color{ 240, 252, 255, 235 }, f0 * f0);
            Color c1 = MezclaGR(Color{ 150, 210, 220, 200 }, Color{ 240, 252, 255, 235 }, f1 * f1);
            QuadColGR({ xa0, y0, zz0 }, { xb0, y0, zz0 }, { xb1, y1, zz1 }, { xa1, y1, zz1 }, c0, c0, c1, c1);
        }
    }

    // Espuma en el pie y ondas en la poza.
    for (int k = 0; k < 7; k++)
    {
        float u = FraccionGR(t * 0.8f + (float)k / 7.0f);
        float x = CASCADA_GR.x + (Ruido01GR(k, 1040) - 0.5f) * 2.6f;
        PuntoGR({ x, 0.3f + u * 1.2f, z + 1.0f }, 0.25f + 0.35f * u, Color{ 245, 252, 255, (unsigned char)(170.0f * (1.0f - u)) }, (float)k);
    }
    for (int k = 0; k < 2; k++)
    {
        float u = FraccionGR(t * 0.5f + 0.5f * (float)k);
        AnilloGR({ CASCADA_GR.x, 0.05f, z + 1.3f }, 0.6f + 2.2f * u, 0.75f + 2.2f * u, 20, Color{ 230, 250, 255, (unsigned char)(140.0f * (1.0f - u)) }, Color{ 230, 250, 255, 0 });
    }
}


// Loros que cruzan sobre la selva lejana (cuerpo y alas).
static void LorosGR(float t, int cantidad)
{
    static const Color PLUMAS[3] = { { 220, 50, 40, 255 }, { 40, 120, 220, 255 }, { 250, 200, 40, 255 } };

    for (int k = 0; k < cantidad; k++)
    {
        float periodo = 16.0f + 6.0f * Ruido01GR(k, 1031);
        float u = FraccionGR(t / periodo + Ruido01GR(k, 1032));
        float dir = (k % 2) ? 1.0f : -1.0f;
        float x = dir * (-50.0f + 100.0f * u);
        float z = -24.0f - 10.0f * Ruido01GR(k, 1033) + 3.0f * std::sin(u * 6.0f);
        float y = 14.0f + 4.0f * Ruido01GR(k, 1034) + 0.8f * std::sin(u * 9.0f + (float)k);
        float aleteo = std::sin(t * 11.0f + (float)k) * 0.7f;
        Color c = PLUMAS[k % 3];
        Vector3 p = { x, y, z };
        Vector3 adelante = { dir, 0.0f, 0.0f };
        TiraGR(Vector3Add(p, Vector3Scale(adelante, 0.35f)), Vector3Subtract(p, Vector3Scale(adelante, 0.55f)), 0.18f, 0.06f, c, TonoGR(c, 0.7f));

        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 punta = { p.x - dir * 0.1f, p.y + 0.4f * std::sin(aleteo), p.z + (float)s * 0.6f * std::cos(aleteo) };
            Color ala = (k % 3 == 0) ? Color{ 40, 110, 210, 255 } : TonoGR(c, 0.85f);
            QuadColGR({ p.x + dir * 0.15f, p.y, p.z }, punta, { punta.x - dir * 0.25f, punta.y, punta.z }, { p.x - dir * 0.15f, p.y, p.z }, ala, ala, ala, ala);
        }
    }
}


// Fuego de un brasero (aditivo) con su charco de luz.
static void FuegoBraseroGR(Vector3 b, float t, int k, bool charco)
{
    float l = 0.78f + 0.22f * std::sin(t * 9.0f + (float)k * 2.1f) * std::sin(t * 4.7f + (float)k);
    Vector3 c = { b.x, b.y + 0.95f, b.z };

    for (int f = 0; f < 4; f++)
    {
        float u = FraccionGR(t * 1.6f + (float)f * 0.25f + (float)k * 0.13f);
        Vector3 p = { c.x + 0.12f * std::sin(t * 5.0f + (float)f * 2.0f), c.y - 0.15f + u * 0.9f, c.z };
        Color col = MezclaGR(Color{ 255, 236, 150, 255 }, Color{ 255, 90, 30, 255 }, u);
        HaloGR(p, (0.34f - 0.2f * u) * l, AlfaGR(col, 0.7f * (1.0f - u)), AlfaGR(col, 0.0f), 8);
    }

    HaloGR(c, 1.7f * l, AlfaGR(FUEGO_GR, 0.28f), AlfaGR(FUEGO_GR, 0.0f), 14);
    if (charco)
    {
        float y = b.y > 1.0f && b.y < 2.0f ? 0.04f : b.y + 0.02f;
        float zc = b.z < Z_FONDO ? Z_FONDO + 1.6f : b.z - 1.4f;
        float yy = b.z < Z_FONDO ? 0.04f : y;
        DiscoGR({ b.x * 0.92f, yy, zc }, 2.6f * l, 2.0f * l, 18, AlfaGR(FUEGO_GR, 0.08f), AlfaGR(FUEGO_GR, 0.0f), false);
    }
}


//--------------------------------------------------
// Particulas de la logica (chispas y polvo) y confeti final
//--------------------------------------------------

static void ParticulasLogicaGR(const MinijuegoGuardianRuinas& m)
{
    for (int i = 0; i < MAX_PARTICULAS_GUARDIAN; i++)
    {
        const ParticulaTierra& p = m.particulas[i];
        if (!p.activa) continue;

        float vida = p.vidaMaxima > 0.0f ? Lim01GR(p.vida / p.vidaMaxima) : 0.0f;
        PuntoGR(p.posicion, p.tamano * (0.6f + 0.6f * vida), AlfaGR(p.color, 0.35f + 0.65f * vida), p.vida * 9.0f);
    }
}


static void ConfetiGR(const MinijuegoGuardianRuinas& m, const JugadorPrueba jugadores[], int limite, float tf)
{
    if (m.fase != FASE_GUARDIAN_TERMINADO || m.resultado.estado != RESULTADO_MINIJUEGO_FINALIZADO) return;

    static const Color COLORES[5] = { { 255, 214, 90, 255 }, { 96, 248, 222, 255 }, { 255, 110, 90, 255 }, { 140, 210, 255, 255 }, { 190, 255, 130, 255 } };
    int cantidad = (int)(70.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& e = m.estadosJugadores[i];
        if (e.equipo < 0 || e.equipo != m.equipoGanador) continue;

        Vector3 base = jugadores[i].posicion;
        for (int k = 0; k < cantidad; k++)
        {
            float u = FraccionGR(tf * 0.35f + Ruido01GR(k, 1101 + i));
            float a = 2.0f * PI * Ruido01GR(k, 1102 + i);
            float r = 0.4f + 1.8f * Ruido01GR(k, 1103 + i);
            Vector3 p = { base.x + std::cos(a) * r + 0.3f * std::sin(tf * 3.0f + (float)k), base.y + 4.0f - 4.2f * u, base.z + std::sin(a) * r };
            PuntoGR(p, 0.07f, COLORES[k % 5], tf * 6.0f + (float)k);
        }
    }
}


//--------------------------------------------------
// Luces aditivas
//--------------------------------------------------

static void LucesGR(const MinijuegoGuardianRuinas& m, const JugadorPrueba jugadores[], int limite, float t, bool decoracion)
{
    int meta = m.metaOrbes > 0 ? m.metaOrbes : 1;
    float progreso = Lim01GR((float)m.goles / (float)meta);
    float gol = Lim01GR(m.tiempoGol / 1.2f);

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Rayos de sol y manchas de luz (malla horneada que respira apenas).
    {
        float respira = 0.86f + 0.14f * std::sin(t * 0.6f);
        unsigned char v = CanalGR(255.0f * respira);
        DibujarMallaGR(g_hornoGR.luz, g_hornoGR.luzValida, MatrixIdentity(), Color{ v, v, v, 255 });
    }

    LoteGR();

    // Portal: resplandor, charco turquesa en la plaza y linea de gol.
    {
        float l = 0.8f + 0.2f * std::sin(t * 3.0f) + 0.5f * gol;
        Vector3 c = { 0.0f, PORTAL_ALTO_GR * 0.45f, Z_FONDO - 0.3f };
        HaloGR(c, (3.4f + 1.4f * progreso) * l, AlfaGR(RUNA_GR, 0.22f + 0.12f * progreso), AlfaGR(RUNA_GR, 0.0f), 22);
        HaloGR(c, 1.6f * l, AlfaGR(RUNA_CLARA_GR, 0.18f + 0.3f * gol), AlfaGR(RUNA_GR, 0.0f), 16);
        DiscoGR({ 0.0f, 0.05f, Z_FONDO + 0.4f }, 3.6f * l, 2.4f * l, 22, AlfaGR(RUNA_GR, 0.16f + 0.1f * progreso), AlfaGR(RUNA_GR, 0.0f), false);

        Color linea = AlfaGR(MezclaGR(RUNA_GR, RUNA_CLARA_GR, gol), 0.7f);
        QuadColGR({ -PORTAL_MEDIO, 0.06f, Z_FONDO - 0.08f }, { PORTAL_MEDIO, 0.06f, Z_FONDO - 0.08f }, { PORTAL_MEDIO, 0.06f, Z_FONDO + 0.1f }, { -PORTAL_MEDIO, 0.06f, Z_FONDO + 0.1f }, linea, linea, linea, linea);

        // Ojos del mascaron.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 ojo = { (float)lado * 0.5f, PORTAL_ALTO_GR + 1.59f, Z_FONDO + 0.1f };
            HaloGR(ojo, 0.35f + 0.25f * progreso + 0.3f * gol, AlfaGR(RUNA_CLARA_GR, 0.4f + 0.4f * progreso), AlfaGR(RUNA_GR, 0.0f), 10);
        }

        // Runas encendidas del contador.
        for (int i = 0; i < m.goles && i < meta; i++)
        {
            bool ultima = i == m.goles - 1 && gol > 0.0f;
            float pulso = 0.8f + 0.2f * std::sin(t * 4.0f + (float)i);
            float r = ultima ? 0.6f + 0.9f * gol : 0.5f * pulso;
            Vector3 p = PosRunaGR(i, meta);
            HaloGR({ p.x, p.y, p.z + 0.1f }, r, AlfaGR(RUNA_GR, ultima ? 0.8f : 0.4f), AlfaGR(RUNA_GR, 0.0f), 12);
        }
    }

    // Braseros.
    for (int k = 0; k < 4; k++)
    {
        FuegoBraseroGR(BRASEROS_GR[k], t, k, decoracion);
    }

    // Escudo del guardian: brillo y estela de la embestida.
    if (m.indiceGuardian >= 0)
    {
        bool embistiendo = m.embestidaTiempo > 0.0f;
        float sz = ZEscudo(m);
        Color col = embistiendo ? Color{ 255, 150, 70, 255 } : ESCUDO_GR;
        DiscoGR({ m.guardianX, 0.05f, sz + 0.2f }, ESCUDO_MEDIO * 1.1f, 0.9f, 18, AlfaGR(col, embistiendo ? 0.3f : 0.12f), AlfaGR(col, 0.0f), false);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = m.guardianX + (float)lado * (ESCUDO_MEDIO + 0.05f);
            float lista = m.embestidaRecarga <= 0.0f ? 1.0f : 0.4f;
            HaloGR({ x, 1.88f, sz }, 0.5f * lista + 0.2f, AlfaGR(col, 0.5f * lista + 0.15f), AlfaGR(col, 0.0f), 10);
        }

        if (embistiendo)
        {
            for (int k = 0; k < 6; k++)
            {
                float x = m.guardianX - ESCUDO_MEDIO + 2.0f * ESCUDO_MEDIO * ((float)k + 0.5f) / 6.0f;
                float y = 0.5f + 1.0f * Ruido01GR(k, (int)(t * 20.0f));
                TiraGR({ x, y, sz - 0.1f }, { x, y, sz - 1.6f }, 0.08f, 0.0f, AlfaGR(col, 0.6f), AlfaGR(col, 0.0f));
            }
        }
    }

    // Aturdidos: estrellitas que giran sobre la cabeza.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& e = m.estadosJugadores[i];
        if (e.equipo != 1 || e.aturdimiento <= 0.0f) continue;
        Vector3 p = jugadores[i].posicion;
        for (int s = 0; s < 3; s++)
        {
            float a = t * 6.0f + 2.0f * PI * (float)s / 3.0f;
            Vector3 q = { p.x + std::cos(a) * 0.45f, p.y + 1.25f, p.z + std::sin(a) * 0.45f };
            HaloGR(q, 0.16f, Color{ 255, 230, 120, 230 }, Color{ 255, 180, 60, 0 }, 6);
        }
    }

    LucesOrbesGR(m, t);

    FinLoteGR();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Anuncios y cartelitos (2D, despues de EndMode3D)
//--------------------------------------------------

static void AnunciosGR(const MinijuegoGuardianRuinas& m, const JugadorPrueba jugadores[], const Participante participantes[], int limite, const Camera3D& vista, float t)
{
    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();
    int meta = m.metaOrbes > 0 ? m.metaOrbes : 1;

    // Columna por caer: aviso sobre ella y anuncio arriba.
    bool hayAviso = false;
    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        const ColumnaGuardian& col = m.columnas[c];
        if (m.fase != FASE_GUARDIAN_JUGANDO || col.estado != COLUMNA_GUARDIAN_AVISO) continue;
        if (col.tiempo > AVISO_COLUMNA - 1.4f) hayAviso = true;
        Vector2 s = GetWorldToScreen({ col.x, AltoColumnaGR(c) + 0.9f, col.z }, vista);
        float pulso = 0.7f + 0.3f * std::sin(t * 16.0f);
        DibujarTextoMundoContornoUI("¡CUIDADO!", s.x, s.y, UI_TEXTO, Color{ 255, 130, 100, 255 }, pulso);
    }
    if (hayAviso)
    {
        DibujarTextoMundoContornoUI("¡SE DERRUMBA UNA COLUMNA!", ancho * 0.5f, alto * 0.22f, UI_TEXTO_GRANDE, Color{ 255, 170, 120, 255 });
    }

    // Orbe al portal: cuenta sobre el arco.
    if (m.tiempoGol > 0.0f && m.fase != FASE_GUARDIAN_PREPARACION)
    {
        float u = 1.0f - m.tiempoGol / 1.2f;
        float alfa = m.tiempoGol < 0.3f ? m.tiempoGol / 0.3f : 1.0f;
        Vector2 s = GetWorldToScreen({ 0.0f, PORTAL_ALTO_GR + 2.6f + u * 0.8f, Z_FONDO }, vista);
        char texto[32];
        if (m.goles >= meta) std::snprintf(texto, sizeof(texto), "¡%d/%d!", m.goles, meta);
        else if (m.goles == meta - 1) std::snprintf(texto, sizeof(texto), "¡%d/%d! ¡FALTA UNO!", m.goles, meta);
        else std::snprintf(texto, sizeof(texto), "¡ORBE! %d/%d", m.goles, meta);
        DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, RUNA_CLARA_GR, alfa);
    }

    // Cartelitos J1..J4 (y el rol del guardian al empezar).
    bool presentar = m.fase == FASE_GUARDIAN_PREPARACION ||
        (m.fase == FASE_GUARDIAN_JUGANDO && m.tiempoJugado < 2.5f);

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& e = m.estadosJugadores[i];
        if (e.equipo < 0 || !participantes[i].activo) continue;

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + (e.equipo == 0 ? 1.0f : 0.75f);
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        Rectangle placa = DibujarEtiquetaUI(rotulo, p, 0.0f, op);

        if (e.equipo == 0 && presentar)
        {
            OpcionesEtiquetaUI rol;
            rol.borde = ESCUDO_GR;
            DibujarEtiquetaUI("GUARDIÁN", { p.x, placa.y - Px(14.0f) }, 0.0f, rol);
        }
    }

    if (m.resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", ancho * 0.5f, alto * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 150, 120, 255 });
    }
}


static void HudGR(const MinijuegoGuardianRuinas& m, const Participante participantes[], int limite)
{
    int meta = m.metaOrbes > 0 ? m.metaOrbes : 1;
    const char* rolAtacante = m.cantidadAtacantes >= 3 ? "TRÍO" : (m.cantidadAtacantes == 2 ? "DÚO" : "ATACANTE");

    char objetivo[128];
    std::snprintf(
        objetivo, sizeof(objetivo),
        "%s: METAN %d ORBES EN EL PORTAL. GUARDIÁN: DEFENDELO CON EL ESCUDO %d SEGUNDOS.",
        rolAtacante, meta, (int)DURACION_PARTIDA_GUARDIAN);

    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(hud, "GUARDIÁN DE RUINAS", objetivo, m.resultado);

    hud.fase = m.fase == FASE_GUARDIAN_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_GUARDIAN_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_GUARDIAN;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = m.equipoGanador == 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible || i >= limite || m.estadosJugadores[i].equipo < 0) continue;

        const EstadoJugadorGuardian& e = m.estadosJugadores[i];
        const Participante& participante = participantes[i];
        bool humano = !participante.esBot && participante.conectado;

        if (e.equipo == 0)
        {
            j.equipo = 0;
            j.rol = "GUARDIÁN";
            j.valorNumerico = m.bloqueos;
            std::snprintf(j.valor, sizeof(j.valor), "%d", m.bloqueos);
            j.progreso = m.embestidaTiempo > 0.0f ? 0.0f : 1.0f - Lim01GR(m.embestidaRecarga / EMBESTIDA_RECARGA);
            if (m.fase == FASE_GUARDIAN_JUGANDO && m.embestidaTiempo > 0.0f) std::snprintf(j.estado, sizeof(j.estado), "¡EMBESTIDA!");
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), m.bloqueos == 1 ? "1 BLOQUEO" : "%d BLOQUEOS", m.bloqueos);

            if (humano)
            {
                std::snprintf(j.controles, sizeof(j.controles), "MOVÉ: %s\nEMBESTÍ: %s", TextoMoverHud(participante), TextoBotonGolpeHud(participante));
            }
        }
        else
        {
            j.equipo = 1;
            j.rol = rolAtacante;
            j.valorNumerico = m.goles;
            std::snprintf(j.valor, sizeof(j.valor), "%d/%d", m.goles, meta);
            j.progreso = Lim01GR((float)m.goles / (float)meta);
            if (m.fase == FASE_GUARDIAN_JUGANDO && e.aturdimiento > 0.0f) std::snprintf(j.estado, sizeof(j.estado), "ATURDIDO");
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), e.lanzados == 1 ? "1 TIRO" : "%d TIROS", e.lanzados);

            if (humano)
            {
                std::snprintf(j.controles, sizeof(j.controles), "MOVÉ: %s  LANZÁ: %s\nIZQ/DER AL LANZAR: DESVÍO", TextoMoverHud(participante), TextoBotonGolpeHud(participante));
            }
        }
    }

    char nota[80];
    if (m.fase == FASE_GUARDIAN_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (m.equipoGanador == 0) hud.tituloFinal = "¡GANA EL GUARDIÁN!";
        else if (m.cantidadAtacantes >= 3) hud.tituloFinal = "¡GANA EL TRÍO!";
        else if (m.cantidadAtacantes == 2) hud.tituloFinal = "¡GANA EL DÚO!";
        else hud.tituloFinal = "¡GANA EL ATACANTE!";

        std::snprintf(nota, sizeof(nota), "ORBES EN EL PORTAL: %d DE %d   BLOQUEOS: %d", m.goles, meta, m.bloqueos);
        hud.notaFinal = nota;
    }

    DibujarHudMinijuego(hud, participantes);
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoGuardianRuinas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    // Reloj visual: sigue corriendo en el tablon final (la logica se para).
    if (fase == FASE_GUARDIAN_TERMINADO)
    {
        float dt = GetFrameTime();
        g_finGR += dt > 0.05f ? 0.05f : dt;
    }
    else
    {
        g_finGR = 0.0f;
    }
    float t = tiempoAnimacion + g_finGR;

    // Una columna que se acaba de derrumbar sacude la camara.
    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        if (columnas[c].estado == COLUMNA_GUARDIAN_CAIDA && columnas[c].tiempo > 0.0f && columnas[c].tiempo < 0.06f)
        {
            ActivarTemblorCamaraGeneral(0.22f, 0.35f);
        }
    }

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(CIELO_HORIZONTE_GR);
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    Camera3D vista = VistaGR(*this);
    g_ojoGR = OJO_GR;
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derGR = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaGR = Vector3CrossProduct(g_derGR, frente);

    HornearGR();

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Opaco horneado: lejania y mundo.
    DibujarMallaGR(g_hornoGR.lejos, g_hornoGR.lejosValido);
    DibujarMallaGR(g_hornoGR.mundo, g_hornoGR.mundoValido);

    // Opaco dinamico: columnas, escudo, engastes del contador y orbes.
    LoteGR();
    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        ColumnaGR(columnas[c], c, t, decoracion);
    }
    if (partidaValida)
    {
        EscudoOpacoGR(*this, t);
    }
    RunasOpacasGR(*this);
    g_emisGR = 1.0f;
    if (!efectos)
    {
        // Sin luces aditivas: llama simple en los braseros.
        for (int k = 0; k < 4; k++)
        {
            Vector3 b = BRASEROS_GR[k];
            float l = 0.85f + 0.15f * std::sin(t * 8.0f + (float)k);
            CilGR({ b.x, b.y + 0.64f, b.z }, { b.x, b.y + 0.64f + 0.6f * l, b.z }, 0.26f, 0.0f, 6, FUEGO_GR, false);
            CilGR({ b.x, b.y + 0.64f, b.z }, { b.x, b.y + 0.64f + 0.35f * l, b.z }, 0.15f, 0.0f, 6, Color{ 255, 236, 150, 255 }, false);
        }
    }
    NucleosOrbesGR(*this, t);
    g_emisGR = 0.0f;
    FinLoteGR();

    // Jugadores con el modelo compartido (los aturdidos parpadean).
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& e = estadosJugadores[i];
        if (e.equipo < 0) continue;

        Participante visual = participantes[i];
        visual.conectado = true;

        if (e.equipo == 1 && e.aturdimiento > 0.0f && std::fmod(t * 12.0f, 1.0f) < 0.45f)
        {
            visual.color = MezclaGR(visual.color, Color{ 200, 200, 200, 255 }, 0.6f);
        }

        DibujarJugadorModeloCompartido(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    // Cielo despues de lo opaco: la prueba de profundidad descarta lo tapado.
    DibujarMallaGR(g_hornoGR.cielo, g_hornoGR.cieloValido, MatrixTranslate(vista.position.x, vista.position.y, vista.position.z));

    // Translucidos: marcas del suelo, sombras, velo, escudo, particulas.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    LoteGR();
    SombrasOrbesGR(*this);
    if (partidaValida)
    {
        MarcasSueloGR(*this, jugadores, participantes, limite, t);
        PunteriaGR(*this, jugadores, participantes, limite, t);
    }
    {
        int meta = metaOrbes > 0 ? metaOrbes : 1;
        VeloPortalGR(*this, t, Lim01GR((float)goles / (float)meta));
    }
    if (partidaValida)
    {
        EscudoTranslucidoGR(*this, t);
    }
    ParticulasLogicaGR(*this);
    if (decoracion)
    {
        float factor = FactorCalidadGrafica(CalidadParticulas());
        HojasQueCaenGR(t, (int)(24.0f * factor));
        CascadaGR(t, efectos);
        MariposasGR(t, CalidadDecoracion() == CALIDAD_ALTA ? 6 : 3);
        LorosGR(t, CalidadDecoracion() == CALIDAD_ALTA ? 5 : 2);
    }
    ConfetiGR(*this, jugadores, limite, g_finGR);
    FinLoteGR();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    if (efectos)
    {
        LucesGR(*this, jugadores, limite, t, decoracion);

        if (decoracion)
        {
            rlDisableDepthMask();
            BeginBlendMode(BLEND_ADDITIVE);
            LoteGR();
            PolenGR(t, (int)(60.0f * FactorCalidadGrafica(CalidadParticulas())));
            FinLoteGR();
            rlDrawRenderBatchActive();
            EndBlendMode();
            rlEnableDepthMask();
        }
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    AnunciosGR(*this, jugadores, participantes, limite, vista, t);
    HudGR(*this, participantes, limite);
}


const ResultadoMinijuego& MinijuegoGuardianRuinas::ObtenerResultado() const
{
    return resultado;
}


void MinijuegoGuardianRuinas::Descargar()
{
    LiberarMallasGR();
    LiberarMaterialHorneado(g_hornoGR.material, g_hornoGR.materialListo);
}
