#include "Minigames/MinijuegoGuardianRuinas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
#include <cstdio>


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

static const Color COLOR_GUARDIAN = { 120, 220, 255, 255 };
static const Color COLOR_ATACANTES = { 255, 170, 60, 255 };


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


static unsigned int HashIndice(int k)
{
    return (((unsigned int)k * 2654435761u) >> 8) & 0xFFu;
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
// VISUAL: RUINAS CUBIERTAS DE VEGETACION
//==================================================
//
// MODELO FUTURO: portal de piedra con runas, columnas rotas con enredaderas,
// estatuas caidas, escalinata del templo, losas de la plaza, arboles
// gigantes, orbes de energia con estela y escudo del guardian en GLB. La
// logica (colisiones de ActualizarOrbes y POSICIONES_COLUMNAS) no depende
// de estas funciones.

static const Color COLOR_PIEDRA = { 128, 134, 116, 255 };
static const Color COLOR_PIEDRA_OSCURA = { 96, 104, 90, 255 };
static const Color COLOR_MUSGO = { 70, 130, 64, 255 };
static const Color COLOR_PORTAL = { 100, 240, 220, 255 };


static void DibujarArbol(float x, float z, float escala, unsigned int h)
{
    float alto = (5.0f + (float)(h % 4)) * escala;

    DrawCylinder({ x, -0.5f, z }, 0.7f * escala, 1.0f * escala, alto, 8, Color{ 84, 62, 40, 255 });
    DrawSphere({ x, alto + 0.5f, z }, 3.2f * escala, Color{ 34, 104, 48, 255 });
    DrawSphere({ x + 1.8f * escala, alto - 0.3f, z + 0.8f }, 2.4f * escala, Color{ 46, 126, 56, 255 });
    DrawSphere({ x - 1.7f * escala, alto - 0.6f, z - 0.6f }, 2.2f * escala, Color{ 40, 116, 52, 255 });
}


static void DibujarPlaza()
{
    // Suelo de losas y juntas.
    DrawCube({ 0.0f, -0.2f, -2.0f }, 2.0f * X_PLAZA, 0.4f, 24.0f, Color{ 104, 112, 96, 255 });

    for (int k = 0; k <= 8; k++)
    {
        DrawCube({ -X_PLAZA + (float)k * (2.0f * X_PLAZA / 8.0f), 0.005f, -2.0f }, 0.06f, 0.01f, 24.0f, Color{ 70, 78, 66, 255 });
    }

    for (int k = 0; k <= 11; k++)
    {
        DrawCube({ 0.0f, 0.005f, -14.0f + (float)k * 2.0f + 2.0f }, 2.0f * X_PLAZA, 0.01f, 0.06f, Color{ 70, 78, 66, 255 });
    }

    // Manchas de musgo.
    for (int k = 0; k < 14; k++)
    {
        unsigned int h = HashIndice(k + 3);
        float x = -8.0f + (float)(h % 160) * 0.1f;
        float z = -11.0f + (float)(HashIndice(k + 40) % 190) * 0.1f;

        DrawCube({ x, 0.012f, z }, 1.2f + (float)(h % 3) * 0.4f, 0.01f, 1.0f + (float)(h % 4) * 0.3f, Fade(COLOR_MUSGO, 0.7f));
    }

    // Muros laterales bajos y derruidos.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 8; k++)
        {
            unsigned int h = HashIndice(k + (lado + 1) * 9);
            float z = -11.0f + (float)k * 2.6f;
            float alto = 0.8f + (float)(h % 5) * 0.35f;

            DrawCube({ (float)lado * (X_PLAZA + 0.5f), alto * 0.5f, z }, 1.0f, alto, 2.4f, COLOR_PIEDRA_OSCURA);
            DrawCube({ (float)lado * (X_PLAZA + 0.5f), alto + 0.05f, z }, 1.1f, 0.12f, 2.5f, COLOR_MUSGO);
        }

        // Arbustos y estatua caida.
        for (int k = 0; k < 5; k++)
        {
            DrawSphere({ (float)lado * (X_PLAZA - 0.2f), 0.5f, -9.0f + (float)k * 4.2f }, 0.7f, Color{ 48, 120, 56, 255 });
        }

        DrawCube({ (float)lado * 7.4f, 0.4f, 7.6f }, 2.6f, 0.8f, 0.9f, Color{ 150, 154, 138, 255 });
        DrawSphere({ (float)lado * 8.9f, 0.55f, 7.6f }, 0.55f, Color{ 150, 154, 138, 255 });
    }
}


static void DibujarPortal(const MinijuegoGuardianRuinas& m)
{
    float t = m.tiempoAnimacion;
    float brillo = 0.55f + 0.2f * std::sin(t * 3.0f) + (m.tiempoGol > 0.0f ? 0.4f : 0.0f);

    // Escalinata y templo al fondo.
    for (int k = 0; k < 4; k++)
    {
        DrawCube({ 0.0f, 0.15f + (float)k * 0.3f, Z_FONDO - 1.0f - (float)k * 1.0f }, 11.0f - (float)k * 0.6f, 0.3f + (float)k * 0.6f, 1.0f, COLOR_PIEDRA);
    }

    DrawCube({ 0.0f, 5.0f, -18.0f }, 24.0f, 10.0f, 2.0f, COLOR_PIEDRA_OSCURA);
    DrawCube({ 0.0f, 10.2f, -18.0f }, 25.0f, 0.5f, 2.4f, COLOR_MUSGO);

    // Muro del fondo a ambos lados del portal.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube({ (float)lado * 6.4f, 1.4f, Z_FONDO - 0.4f }, 6.2f, 2.8f, 0.8f, COLOR_PIEDRA);
        DrawCube({ (float)lado * 6.4f, 2.85f, Z_FONDO - 0.4f }, 6.3f, 0.12f, 0.9f, COLOR_MUSGO);
    }

    // Marco del portal.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube({ (float)lado * (PORTAL_MEDIO + 0.5f), 2.6f, Z_FONDO - 0.4f }, 1.0f, 5.2f, 1.0f, COLOR_PIEDRA);
        DrawCube({ (float)lado * (PORTAL_MEDIO + 0.5f), 5.3f, Z_FONDO - 0.4f }, 1.3f, 0.4f, 1.3f, COLOR_PIEDRA_OSCURA);
    }

    DrawCube({ 0.0f, 5.5f, Z_FONDO - 0.4f }, 2.0f * (PORTAL_MEDIO + 1.0f), 0.9f, 1.1f, COLOR_PIEDRA);
    DrawCube({ 0.0f, 6.0f, Z_FONDO - 0.4f }, 2.0f * (PORTAL_MEDIO + 1.0f), 0.12f, 1.2f, COLOR_MUSGO);

    // Velo de energia y linea de gol.
    DrawCube({ 0.0f, 2.5f, Z_FONDO - 0.2f }, 2.0f * PORTAL_MEDIO, 5.0f, 0.12f, Fade(COLOR_PORTAL, 0.5f * brillo));
    DrawCube({ 0.0f, 0.03f, Z_FONDO + 0.2f }, 2.0f * PORTAL_MEDIO, 0.04f, 0.3f, Fade(COLOR_PORTAL, brillo));

    for (int k = 0; k < 4; k++)
    {
        DrawCube(
            { 0.0f, 0.8f + (float)k * 1.1f + 0.1f * std::sin(t * 2.0f + (float)k), Z_FONDO - 0.12f },
            1.5f - 0.2f * (float)k, 0.1f, 0.05f, Fade(WHITE, 0.5f)
        );
    }

    // Runas del marco.
    for (int k = 0; k < 3; k++)
    {
        DrawCube({ -(PORTAL_MEDIO + 0.5f), 1.2f + (float)k * 1.3f, Z_FONDO + 0.12f }, 0.3f, 0.3f, 0.05f, Fade(COLOR_PORTAL, brillo));
        DrawCube({ PORTAL_MEDIO + 0.5f, 1.2f + (float)k * 1.3f, Z_FONDO + 0.12f }, 0.3f, 0.3f, 0.05f, Fade(COLOR_PORTAL, brillo));
    }
}


static void DibujarColumnas(const MinijuegoGuardianRuinas& m)
{
    float t = m.tiempoAnimacion;

    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        const ColumnaGuardian& columna = m.columnas[c];
        float x = columna.x;
        float z = columna.z;

        if (columna.estado == COLUMNA_GUARDIAN_CAIDA)
        {
            // Tambores caidos y escombros.
            unsigned int h = HashIndice(c + 11);
            float angulo = (float)h * 0.025f;
            float dx = std::cos(angulo) * 1.8f;
            float dz = std::sin(angulo) * 1.8f;

            DrawCylinderEx({ x - dx * 0.5f, 0.45f, z - dz * 0.5f }, { x + dx * 0.5f, 0.45f, z + dz * 0.5f }, 0.5f, 0.5f, 10, COLOR_PIEDRA);
            DrawCylinderEx({ x + dx * 0.6f, 0.4f, z + dz * 0.6f }, { x + dx * 1.2f, 0.4f, z + dz * 1.2f }, 0.45f, 0.45f, 10, COLOR_PIEDRA_OSCURA);
            DrawCube({ x, 0.2f, z }, 1.0f, 0.4f, 1.0f, COLOR_PIEDRA_OSCURA);
            continue;
        }

        float temblor = 0.0f;
        Color color = COLOR_PIEDRA;

        if (columna.estado == COLUMNA_GUARDIAN_AVISO)
        {
            temblor = 0.07f * std::sin(t * 60.0f);
            color = Color{ 190, 120, 100, 255 };

            float pulso = 0.5f + 0.5f * std::sin(t * 12.0f);

            DrawCircle3D({ x, 0.05f, z }, 1.2f + 0.3f * pulso, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(RED, 0.5f + 0.4f * pulso));
            DrawCircle3D({ x, 0.06f, z }, 0.9f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(ORANGE, 0.7f));
        }

        DrawCylinder({ x + temblor, 0.0f, z }, 0.5f, 0.6f, 4.0f, 12, color);
        DrawCube({ x + temblor, 0.15f, z }, 1.5f, 0.3f, 1.5f, COLOR_PIEDRA_OSCURA);
        DrawCube({ x + temblor, 4.15f, z }, 1.6f, 0.3f, 1.6f, COLOR_PIEDRA_OSCURA);
        DrawCylinder({ x + temblor, 3.0f, z }, 0.62f, 0.62f, 0.15f, 12, COLOR_MUSGO);

        // Enredaderas.
        for (int v = 0; v < 3; v++)
        {
            float a = (float)v * 2.1f + (float)c;

            DrawLine3D(
                { x + std::cos(a) * 0.62f, 4.0f, z + std::sin(a) * 0.62f },
                { x + std::cos(a + 0.5f) * 0.66f, 1.6f + (float)v * 0.4f, z + std::sin(a + 0.5f) * 0.66f },
                Color{ 50, 150, 70, 255 }
            );
        }
    }
}


static void DibujarOrbes(const MinijuegoGuardianRuinas& m)
{
    float t = m.tiempoAnimacion;

    for (int k = 0; k < MAX_ORBES_GUARDIAN; k++)
    {
        const OrbeGuardian& orbe = m.orbes[k];

        if (!orbe.activo)
        {
            continue;
        }

        Color nucleo = orbe.devuelto ? Color{ 255, 170, 70, 255 } : Color{ 190, 250, 255, 255 };
        Color halo = orbe.devuelto ? Color{ 255, 100, 40, 255 } : Color{ 80, 200, 255, 255 };
        float velocidad = std::sqrt(orbe.vx * orbe.vx + orbe.vz * orbe.vz);
        float dx = velocidad > 0.01f ? orbe.vx / velocidad : 0.0f;
        float dz = velocidad > 0.01f ? orbe.vz / velocidad : 0.0f;
        float pulso = 1.0f + 0.12f * std::sin(t * 20.0f + (float)k);

        DrawSphere({ orbe.x, 0.9f, orbe.z }, RADIO_ORBE * pulso, nucleo);
        DrawSphere({ orbe.x, 0.9f, orbe.z }, RADIO_ORBE * 1.8f, Fade(halo, 0.25f));

        for (int s = 1; s <= 3; s++)
        {
            DrawSphere(
                { orbe.x - dx * 0.35f * (float)s, 0.9f, orbe.z - dz * 0.35f * (float)s },
                RADIO_ORBE * (1.0f - 0.22f * (float)s),
                Fade(halo, 0.5f - 0.13f * (float)s)
            );
        }

        // Sombra en el suelo.
        DrawCircle3D({ orbe.x, 0.03f, orbe.z }, 0.3f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.4f));
    }
}


static void DibujarEscudo(const MinijuegoGuardianRuinas& m)
{
    if (m.indiceGuardian < 0)
    {
        return;
    }

    bool embistiendo = m.embestidaTiempo > 0.0f;
    float z = ZEscudo(m);
    Color color = embistiendo ? Color{ 255, 180, 70, 255 } : COLOR_GUARDIAN;

    DrawCube({ m.guardianX, 1.0f, z }, 2.0f * ESCUDO_MEDIO, 1.5f, 2.0f * ESCUDO_FONDO * 0.5f, Fade(color, 0.55f));
    DrawCubeWires({ m.guardianX, 1.0f, z }, 2.0f * ESCUDO_MEDIO, 1.5f, ESCUDO_FONDO, color);
    DrawCube({ m.guardianX, 0.2f, z }, 2.0f * ESCUDO_MEDIO + 0.2f, 0.2f, 0.4f, COLOR_PIEDRA_OSCURA);

    // Linea de embestida disponible.
    if (m.embestidaRecarga <= 0.0f)
    {
        DrawCircle3D({ m.guardianX, 0.05f, GUARDIAN_Z }, 0.8f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(COLOR_GUARDIAN, 0.8f));
    }
}


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

    ClearBackground(Color{ 26, 52, 40, 255 });
    BeginMode3D(camara);

    // Selva alrededor de la plaza.
    DrawCube({ 0.0f, -0.8f, -10.0f }, 120.0f, 1.0f, 90.0f, Color{ 34, 78, 46, 255 });

    for (int k = 0; k < 9; k++)
    {
        unsigned int h = HashIndice(k + 70);

        DibujarArbol(-30.0f + (float)k * 7.5f, -28.0f - (float)(h % 5), 1.4f + 0.1f * (float)(h % 4), h);
    }

    for (int k = 0; k < 5; k++)
    {
        unsigned int h = HashIndice(k + 90);

        DibujarArbol(-19.0f - (float)(h % 3), -12.0f + (float)k * 6.0f, 1.0f, h);
        DibujarArbol(19.0f + (float)(h % 3), -10.0f + (float)k * 6.0f, 1.0f, h + 3);
    }

    DibujarPlaza();
    DibujarPortal(*this);
    DibujarColumnas(*this);

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        Color colorEquipo = estado.equipo == 0 ? COLOR_GUARDIAN : COLOR_ATACANTES;

        DrawCircle3D({ jugadores[i].posicion.x, 0.06f, jugadores[i].posicion.z }, 0.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, colorEquipo);

        // Linea de punteria de los atacantes humanos.
        if (
            estado.equipo == 1 &&
            fase == FASE_GUARDIAN_JUGANDO &&
            !JugadorEsBot(participantes[i])
        )
        {
            DrawLine3D(
                { jugadores[i].posicion.x, 0.1f, jugadores[i].posicion.z },
                { estado.apuntarX, 0.1f, Z_FONDO },
                Fade(colorEquipo, 0.5f)
            );
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = colorEquipo;
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawCubeWires(jugadores[i].posicion, 0.8f, 1.4f, 0.8f, LIME);
        }
    }

    DibujarEscudo(*this);
    DibujarOrbes(*this);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_GUARDIAN);

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    int centro = anchoPantalla / 2;

    // Etiquetas y barras de recarga sobre los jugadores.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGuardian& estado = estadosJugadores[i];

        if (estado.equipo < 0)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.3f, jugadores[i].posicion.z },
            camara
        );
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        const char* texto = estado.equipo == 0 ? TextFormat("J%d GUARDIAN", numero) : TextFormat("J%d", numero);
        Color colorEquipo = estado.equipo == 0 ? COLOR_GUARDIAN : COLOR_ATACANTES;

        DrawText(texto, (int)pantalla.x - MeasureText(texto, 16) / 2, (int)pantalla.y - 14, 16, colorEquipo);

        if (estado.equipo == 1)
        {
            float carga = 1.0f - Acotar(estado.recarga / RECARGA_LANZAR, 0.0f, 1.0f);

            DrawRectangle((int)pantalla.x - 20, (int)pantalla.y + 4, 40, 5, Fade(BLACK, 0.7f));
            DrawRectangle((int)pantalla.x - 20, (int)pantalla.y + 4, (int)(40.0f * carga), 5, estado.aturdimiento > 0.0f ? RED : colorEquipo);
        }
    }

    // Marcador.
    DrawRectangle(centro - 230, 8, 460, 70, Fade(BLACK, 0.65f));
    DrawText("ORBES EN EL PORTAL", centro - MeasureText("ORBES EN EL PORTAL", 16) / 2, 12, 16, COLOR_ATACANTES);

    const char* marcador = TextFormat("%d / %d", goles, metaOrbes);
    DrawText(marcador, centro - MeasureText(marcador, 34) / 2, 30, 34, tiempoGol > 0.0f ? GOLD : RAYWHITE);
    DrawRectangle(centro - 200, 68, 400, 6, Fade(DARKGRAY, 0.8f));
    DrawRectangle(centro - 200, 68, (int)(400.0f * Acotar((float)goles / (float)metaOrbes, 0.0f, 1.0f)), 6, COLOR_ATACANTES);

    DrawText(
        TextFormat("TIEMPO %d", (int)std::ceil(tiempoRestante)),
        24, 20, 30,
        tiempoRestante <= 10.0f ? RED : RAYWHITE
    );
    DrawText(TextFormat("BLOQUEOS %d", bloqueos), 24, 56, 18, COLOR_GUARDIAN);

    for (int c = 0; c < MAX_COLUMNAS_GUARDIAN; c++)
    {
        if (columnas[c].estado == COLUMNA_GUARDIAN_AVISO && (int)(tiempoAnimacion * 6.0f) % 2 == 0)
        {
            const char* aviso = "UNA COLUMNA VA A CAER!";
            DrawText(aviso, centro - MeasureText(aviso, 26) / 2, 92, 26, ORANGE);
            break;
        }
    }

    DrawRectangle(0, altoPantalla - 32, anchoPantalla, 32, Fade(BLACK, 0.7f));

    const char* ayuda = "GUARDIAN: IZQ/DER MOVER, ACCION EMBESTIR   ATACANTES: MOVER + ACCION (E / SHIFT DER / B) LANZA, IZQ/DER AL LANZAR DESVIAN";
    DrawText(ayuda, centro - MeasureText(ayuda, 14) / 2, altoPantalla - 24, 14, RAYWHITE);

    if (fase == FASE_GUARDIAN_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, centro - MeasureText(texto, 96) / 2, altoPantalla / 2 - 130, 96, GOLD);

        int linea = 0;

        for (int i = 0; i < limite; i++)
        {
            if (estadosJugadores[i].equipo < 0 || JugadorEsBot(participantes[i]))
            {
                continue;
            }

            const char* boton = participantes[i].control == CONTROL_GAMEPAD
                ? "B"
                : (participantes[i].control == CONTROL_TECLADO_FLECHAS ? "SHIFT DER" : "E");

            DrawText(
                TextFormat(
                    "J%d %s: %s",
                    participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1,
                    estadosJugadores[i].equipo == 0 ? "GUARDIAN EMBESTIDA" : "LANZAR ORBE",
                    boton
                ),
                28, altoPantalla / 2 + 40 + linea * 22, 20,
                estadosJugadores[i].equipo == 0 ? COLOR_GUARDIAN : COLOR_ATACANTES
            );
            linea++;
        }
    }
    else if (
        fase == FASE_GUARDIAN_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int ancho = 520;
        int alto = 170;
        int py = altoPantalla / 2 - alto / 2 - 40;

        DrawRectangle(centro - ancho / 2, py, ancho, alto, Fade(BLACK, 0.9f));

        const char* titulo = equipoGanador == 0 ? "GANA EL GUARDIAN" : "GANAN LOS ATACANTES";
        Color colorTitulo = equipoGanador == 0 ? COLOR_GUARDIAN : COLOR_ATACANTES;

        DrawText(titulo, centro - MeasureText(titulo, 32) / 2, py + 16, 32, colorTitulo);

        const char* detalle = TextFormat("ORBES %d / %d     BLOQUEOS %d", goles, metaOrbes, bloqueos);
        DrawText(detalle, centro - MeasureText(detalle, 24) / 2, py + 70, 24, RAYWHITE);
        DrawText(
            TextoReinicioMinijuego(),
            centro - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + alto - 34, 18, RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoGuardianRuinas::ObtenerResultado() const
{
    return resultado;
}
