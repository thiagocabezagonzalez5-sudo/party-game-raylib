#include "Minigames/MinijuegoBolasAzucar.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
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
// BOLAS DE AZUCAR
//==================================================
//
// Todos contra todos. Cada jugador crea una bola, la empuja por las
// zonas de azucar para que crezca y la lanza contra los rivales. Los
// charcos de chocolate la derriten y las gominolas la hacen rebotar.
//==================================================


static const float DURACION_PREPARACION_BOLAS = 3.0f;
static const float DURACION_PARTIDA_BOLAS = 60.0f;
static const float MITAD_ARENA_BOLAS = 9.0f;
static const float LIMITE_JUGADOR_BOLAS = 8.55f;
static const float RADIO_MINIMO_BOLA = 0.3f;
static const float RADIO_MAXIMO_BOLA = 1.4f;
static const float RADIO_DANO_BOLA = 0.8f;
static const float DURACION_CREAR_BOLA = 0.5f;
static const float VELOCIDAD_LANZAMIENTO_BOLA = 10.0f;
static const float CRECIMIENTO_POR_UNIDAD_BOLA = 0.1f;
static const float RADIO_GOMINOLA_BOLAS = 0.8f;


//==================================================
// DATOS LOGICOS DE LA ARENA
//==================================================


struct RectanguloAzucarBolas
{
    float xMin;
    float xMax;
    float zMin;
    float zMax;
};


struct CirculoBolas
{
    float x;
    float z;
    float radio;
};


static const RectanguloAzucarBolas ZONAS_AZUCAR_BOLAS[4] =
{
    { -8.0f, -4.0f, -4.5f, 4.5f },
    { 4.0f, 8.0f, -4.5f, 4.5f },
    { -3.0f, 3.0f, -8.0f, -5.5f },
    { -3.0f, 3.0f, 5.5f, 8.0f }
};

static const CirculoBolas CHARCOS_BOLAS[3] =
{
    { 0.0f, 0.0f, 2.2f },
    { -5.5f, -6.5f, 1.4f },
    { 5.5f, 6.5f, 1.4f }
};

static const CirculoBolas GOMINOLAS_BOLAS[4] =
{
    { -3.2f, -3.2f, RADIO_GOMINOLA_BOLAS },
    { 3.2f, -3.2f, RADIO_GOMINOLA_BOLAS },
    { -3.2f, 3.2f, RADIO_GOMINOLA_BOLAS },
    { 3.2f, 3.2f, RADIO_GOMINOLA_BOLAS }
};


static bool EnAzucarBolas(float x, float z)
{
    for (const RectanguloAzucarBolas& zona : ZONAS_AZUCAR_BOLAS)
    {
        if (x >= zona.xMin && x <= zona.xMax && z >= zona.zMin && z <= zona.zMax)
        {
            return true;
        }
    }

    return false;
}


static bool EnChocolateBolas(float x, float z)
{
    for (const CirculoBolas& charco : CHARCOS_BOLAS)
    {
        float dx = x - charco.x;
        float dz = z - charco.z;

        if (dx * dx + dz * dz <= charco.radio * charco.radio)
        {
            return true;
        }
    }

    return false;
}


static float LimitarBolas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static bool EsControlBotBolas(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static int ContarVivosBolas(const MinijuegoBolasAzucar& minijuego)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static void ConfigurarArenaBolas(MinijuegoBolasAzucar& minijuego)
{
    minijuego.cantidadBloques = 0;

    const Color gris = Color{ 200, 140, 90, 255 };
    const float largo = MITAD_ARENA_BOLAS * 2.0f + 0.8f;

    // 0: suelo. 1-4: muros de galleta.
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_BOLAS_AZUCAR, { 0.0f, -0.5f, 0.0f },
        { MITAD_ARENA_BOLAS * 2.0f, 1.0f, MITAD_ARENA_BOLAS * 2.0f }, gris);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_BOLAS_AZUCAR, { -MITAD_ARENA_BOLAS - 0.2f, 1.0f, 0.0f },
        { 0.4f, 2.0f, largo }, gris);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_BOLAS_AZUCAR, { MITAD_ARENA_BOLAS + 0.2f, 1.0f, 0.0f },
        { 0.4f, 2.0f, largo }, gris);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_BOLAS_AZUCAR, { 0.0f, 1.0f, -MITAD_ARENA_BOLAS - 0.2f },
        { largo, 2.0f, 0.4f }, gris);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_BOLAS_AZUCAR, { 0.0f, 1.0f, MITAD_ARENA_BOLAS + 0.2f },
        { largo, 2.0f, 0.4f }, gris);

    // 5-8: gominolas solidas para los jugadores.
    for (const CirculoBolas& gominola : GOMINOLAS_BOLAS)
    {
        AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
            MAX_BLOQUES_BOLAS_AZUCAR, { gominola.x, 0.6f, gominola.z },
            { 1.3f, 1.2f, 1.3f }, gris);
    }
}


static void CrearPolvoAzucarBolas(
    MinijuegoBolasAzucar& minijuego,
    float x,
    float y,
    float z,
    Color color
)
{
    int creadas = 0;

    for (int i = 0; i < MAX_PARTICULAS_BOLAS_AZUCAR && creadas < 14; i++)
    {
        ParticulaTierra& particula = minijuego.particulas[i];

        if (particula.activa)
        {
            continue;
        }

        float angulo = (float)GetRandomValue(0, 628) / 100.0f;
        float fuerza = (float)GetRandomValue(15, 40) / 10.0f;

        particula.activa = true;
        particula.posicion = { x, y, z };
        particula.velocidad = { std::cos(angulo) * fuerza, (float)GetRandomValue(20, 45) / 10.0f, std::sin(angulo) * fuerza };
        particula.vidaMaxima = (float)GetRandomValue(25, 45) / 100.0f;
        particula.vida = particula.vidaMaxima;
        particula.tamano = (float)GetRandomValue(7, 13) / 100.0f;
        particula.color = color;
        creadas++;
    }
}


static void FinalizarBolas(MinijuegoBolasAzucar& minijuego)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int ganadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorBolasAzucar& estado = minijuego.estadosJugadores[i];

        if (estado.vivo)
        {
            int posicion = 1;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    minijuego.resultado.participantes[j].participo &&
                    minijuego.estadosJugadores[j].vivo &&
                    minijuego.estadosJugadores[j].vidas > estado.vidas
                )
                {
                    posicion++;
                }
            }

            resultadoJugador.posicionFinal = posicion;
        }
        else
        {
            resultadoJugador.posicionFinal = estado.posicionEliminacion;
        }

        resultadoJugador.puntuacionMinijuego = estado.vidas * 100;

        if (resultadoJugador.posicionFinal == 1)
        {
            ganadores++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        ganadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_BOLAS_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


//==================================================
// BOTS
//==================================================


static void MoverHaciaBolas(
    InputMinijuegoParticipante& entrada,
    const JugadorPrueba& jugador,
    float objetivoX,
    float objetivoZ
)
{
    float dx = objetivoX - jugador.posicion.x;
    float dz = objetivoZ - jugador.posicion.z;

    if (dx > 0.35f) entrada.derecha = true;
    else if (dx < -0.35f) entrada.izquierda = true;

    if (dz > 0.35f) entrada.atras = true;
    else if (dz < -0.35f) entrada.adelante = true;
}


static bool LineaLibreBolas(float x0, float z0, float x1, float z1, float radio)
{
    float dx = x1 - x0;
    float dz = z1 - z0;
    float longitud = std::sqrt(dx * dx + dz * dz);
    int pasos = (int)(longitud / 0.3f) + 1;

    for (int p = 0; p <= pasos; p++)
    {
        float t = (float)p / (float)pasos;
        float px = x0 + dx * t;
        float pz = z0 + dz * t;

        for (const CirculoBolas& gominola : GOMINOLAS_BOLAS)
        {
            float gx = px - gominola.x;
            float gz = pz - gominola.z;
            float r = gominola.radio + radio + 0.15f;

            if (gx * gx + gz * gz < r * r)
            {
                return false;
            }
        }
    }

    return true;
}


static InputMinijuegoParticipante CrearEntradaBotBolas(
    MinijuegoBolasAzucar& minijuego,
    int indice,
    const JugadorPrueba jugadores[],
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorBolasAzucar& estado = minijuego.estadosJugadores[indice];
    const JugadorPrueba& jugador = jugadores[indice];
    const BolaAzucar& propia = minijuego.bolas[indice];

    estado.botReaccion -= deltaTime;

    if (estado.botReaccion <= 0.0f)
    {
        estado.botReaccion = 0.25f;
        estado.botNota = GetRandomValue(1, 100) > 15;
    }

    // Esquivar bolas ajenas que vienen hacia el bot.
    if (estado.botNota)
    {
        for (int b = 0; b < MAX_PARTICIPANTES; b++)
        {
            const BolaAzucar& bola = minijuego.bolas[b];

            if (b == indice || !bola.activa || bola.estado != BOLA_AZUCAR_LANZADA)
            {
                continue;
            }

            float velocidad = std::sqrt(bola.velocidadX * bola.velocidadX + bola.velocidadZ * bola.velocidadZ);

            if (velocidad < 2.0f)
            {
                continue;
            }

            float relX = jugador.posicion.x - bola.x;
            float relZ = jugador.posicion.z - bola.z;
            float vx = bola.velocidadX / velocidad;
            float vz = bola.velocidadZ / velocidad;
            float adelante = relX * vx + relZ * vz;
            float lateral = relX * (-vz) + relZ * vx;

            if (adelante < 0.0f || adelante > 7.0f || std::fabs(lateral) > bola.radio + 1.2f)
            {
                continue;
            }

            float lado = lateral >= 0.0f ? 1.0f : -1.0f;
            MoverHaciaBolas(
                entrada,
                jugador,
                jugador.posicion.x + (-vz) * lado * 3.0f,
                jugador.posicion.z + vx * lado * 3.0f
            );
            return entrada;
        }
    }

    estado.botExcluidoTiempo -= deltaTime;
    estado.botDesplazamiento -= deltaTime;

    if (estado.botExcluidoTiempo <= 0.0f)
    {
        estado.botRivalExcluido = -1;
    }

    // Desplazamiento lateral tras fallos o con todos los rivales tapados.
    if (estado.botDesplazamiento > 0.0f)
    {
        MoverHaciaBolas(
            entrada,
            jugador,
            jugador.posicion.x + estado.botDesplazaX * 3.0f,
            jugador.posicion.z + estado.botDesplazaZ * 3.0f
        );
        return entrada;
    }

    if (!(propia.activa && propia.estado == BOLA_AZUCAR_EMPUJADA))
    {
        // No crea la bola si naceria dentro de una gominola: se reposiciona hacia una zona de azucar.
        float mx = jugador.direccionMirada.x;
        float mz = jugador.direccionMirada.z;
        float lm = std::sqrt(mx * mx + mz * mz);

        if (lm > 0.01f)
        {
            float sx = jugador.posicion.x + mx / lm * (0.45f + RADIO_MINIMO_BOLA);
            float sz = jugador.posicion.z + mz / lm * (0.45f + RADIO_MINIMO_BOLA);

            if (!LineaLibreBolas(sx, sz, sx, sz, 0.6f))
            {
                int zonaCercana = 0;
                float menor = 1000000.0f;

                for (int z = 0; z < 4; z++)
                {
                    float cx = (ZONAS_AZUCAR_BOLAS[z].xMin + ZONAS_AZUCAR_BOLAS[z].xMax) * 0.5f - jugador.posicion.x;
                    float cz = (ZONAS_AZUCAR_BOLAS[z].zMin + ZONAS_AZUCAR_BOLAS[z].zMax) * 0.5f - jugador.posicion.z;

                    if (cx * cx + cz * cz < menor)
                    {
                        menor = cx * cx + cz * cz;
                        zonaCercana = z;
                    }
                }

                MoverHaciaBolas(
                    entrada,
                    jugador,
                    (ZONAS_AZUCAR_BOLAS[zonaCercana].xMin + ZONAS_AZUCAR_BOLAS[zonaCercana].xMax) * 0.5f,
                    (ZONAS_AZUCAR_BOLAS[zonaCercana].zMin + ZONAS_AZUCAR_BOLAS[zonaCercana].zMax) * 0.5f
                );
                return entrada;
            }
        }

        entrada.golpear = true;
        return entrada;
    }

    // Rival: el mas cercano con linea libre desde la bola (no el excluido).
    int rival = -1;
    int rivalTapado = -1;
    float mejorDistancia = 1000000.0f;
    float mejorTapado = 1000000.0f;
    int vidasRivales = 0;

    for (int j = 0; j < MAX_PARTICIPANTES; j++)
    {
        if (
            j == indice ||
            !minijuego.resultado.participantes[j].participo ||
            !minijuego.estadosJugadores[j].vivo
        )
        {
            continue;
        }

        vidasRivales += minijuego.estadosJugadores[j].vidas;

        if (j == estado.botRivalExcluido)
        {
            continue;
        }

        float dx = jugadores[j].posicion.x - jugador.posicion.x;
        float dz = jugadores[j].posicion.z - jugador.posicion.z;
        float distancia = dx * dx + dz * dz;
        bool libre = LineaLibreBolas(propia.x, propia.z, jugadores[j].posicion.x, jugadores[j].posicion.z, propia.radio);

        if (libre && distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
            rival = j;
        }
        else if (!libre && distancia < mejorTapado)
        {
            mejorTapado = distancia;
            rivalTapado = j;
        }
    }

    bool suficiente = propia.radio >= 0.9f || estado.tiempoEmpujando > 9.0f;

    if (rival < 0 && rivalTapado >= 0 && suficiente)
    {
        // Todos tapados: se desplaza de lado 1-1.5 s para abrir linea.
        float dx = jugadores[rivalTapado].posicion.x - jugador.posicion.x;
        float dz = jugadores[rivalTapado].posicion.z - jugador.posicion.z;
        float longitud = std::sqrt(dx * dx + dz * dz) + 0.001f;
        float lado = GetRandomValue(0, 1) == 0 ? 1.0f : -1.0f;
        estado.botDesplazaX = -dz / longitud * lado;
        estado.botDesplazaZ = dx / longitud * lado;
        estado.botDesplazamiento = 1.0f + 0.5f * (float)GetRandomValue(0, 100) / 100.0f;
        estado.botRivalExcluido = -1;
        return entrada;
    }

    if (rival >= 0)
    {
        float dx = jugadores[rival].posicion.x - propia.x;
        float dz = jugadores[rival].posicion.z - propia.z;
        float distancia = std::sqrt(dx * dx + dz * dz);
        float mirX = jugador.direccionMirada.x;
        float mirZ = jugador.direccionMirada.z;
        float longitudMirada = std::sqrt(mirX * mirX + mirZ * mirZ);

        if (distancia > 0.01f && longitudMirada > 0.01f)
        {
            float alineacion = (dx * mirX + dz * mirZ) / (distancia * longitudMirada);

            if (alineacion > 0.93f && distancia < 8.0f && (suficiente || distancia < 4.5f))
            {
                // Si los rivales no perdieron vidas desde el lanzamiento anterior, cuenta como fallo.
                if (estado.botVidasRef < 0 || vidasRivales < estado.botVidasRef)
                {
                    estado.botFallos = 0;
                }
                else
                {
                    estado.botFallos++;
                }

                estado.botVidasRef = vidasRivales;

                if (estado.botFallos >= 2)
                {
                    float lado = GetRandomValue(0, 1) == 0 ? 1.0f : -1.0f;
                    estado.botFallos = 0;
                    estado.botRivalExcluido = rival;
                    estado.botExcluidoTiempo = 6.0f;
                    estado.botDesplazaX = -dz / distancia * lado;
                    estado.botDesplazaZ = dx / distancia * lado;
                    estado.botDesplazamiento = 1.0f + 0.5f * (float)GetRandomValue(0, 100) / 100.0f;
                }

                entrada.golpear = true;
                return entrada;
            }
        }

        if (suficiente)
        {
            MoverHaciaBolas(entrada, jugador, jugadores[rival].posicion.x, jugadores[rival].posicion.z);
            return entrada;
        }
    }

    // Crecer: rodar la bola a lo largo de una zona de azucar.
    int mejorZona = 0;
    float mejorZonaDistancia = 1000000.0f;

    for (int z = 0; z < 4; z++)
    {
        const RectanguloAzucarBolas& zona = ZONAS_AZUCAR_BOLAS[z];
        float cx = (zona.xMin + zona.xMax) * 0.5f;
        float cz = (zona.zMin + zona.zMax) * 0.5f;
        float dx = cx - propia.x;
        float dz = cz - propia.z;
        float distancia = dx * dx + dz * dz;

        if (distancia < mejorZonaDistancia)
        {
            mejorZonaDistancia = distancia;
            mejorZona = z;
        }
    }

    const RectanguloAzucarBolas& zona = ZONAS_AZUCAR_BOLAS[mejorZona];
    float centroX = (zona.xMin + zona.xMax) * 0.5f;
    float centroZ = (zona.zMin + zona.zMax) * 0.5f;
    float objetivoX = centroX;
    float objetivoZ = centroZ;

    if (EnAzucarBolas(propia.x, propia.z))
    {
        bool largoEnZ = (zona.zMax - zona.zMin) > (zona.xMax - zona.xMin);

        if (largoEnZ)
            objetivoZ = jugador.posicion.z < centroZ ? zona.zMax - 0.5f : zona.zMin + 0.5f;
        else
            objetivoX = jugador.posicion.x < centroX ? zona.xMax - 0.5f : zona.xMin + 0.5f;
    }

    MoverHaciaBolas(entrada, jugador, objetivoX, objetivoZ);
    return entrada;
}


//==================================================
// ACTUALIZACION
//==================================================


static void LanzarBolaBolas(
    MinijuegoBolasAzucar& minijuego,
    int indice,
    const JugadorPrueba& jugador
)
{
    BolaAzucar& bola = minijuego.bolas[indice];

    float dx = bola.x - jugador.posicion.x;
    float dz = bola.z - jugador.posicion.z;
    float longitud = std::sqrt(dx * dx + dz * dz);

    if (longitud < 0.01f)
    {
        dx = jugador.direccionMirada.x;
        dz = jugador.direccionMirada.z;
        longitud = std::sqrt(dx * dx + dz * dz);

        if (longitud < 0.01f)
        {
            dx = 0.0f;
            dz = -1.0f;
            longitud = 1.0f;
        }
    }

    bola.estado = BOLA_AZUCAR_LANZADA;
    bola.velocidadX = dx / longitud * VELOCIDAD_LANZAMIENTO_BOLA;
    bola.velocidadZ = dz / longitud * VELOCIDAD_LANZAMIENTO_BOLA;
    bola.inactiva = 0.0f;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_DISPARO);
}


static void CrearBolaBolas(
    MinijuegoBolasAzucar& minijuego,
    int indice,
    const JugadorPrueba& jugador
)
{
    float dx = jugador.direccionMirada.x;
    float dz = jugador.direccionMirada.z;
    float longitud = std::sqrt(dx * dx + dz * dz);

    if (longitud < 0.01f)
    {
        dx = 0.0f;
        dz = -1.0f;
        longitud = 1.0f;
    }

    BolaAzucar& bola = minijuego.bolas[indice];
    bola = {};
    bola.activa = true;
    bola.estado = BOLA_AZUCAR_EMPUJADA;
    bola.radio = RADIO_MINIMO_BOLA;
    bola.x = jugador.posicion.x + dx / longitud * (0.45f + RADIO_MINIMO_BOLA);
    bola.z = jugador.posicion.z + dz / longitud * (0.45f + RADIO_MINIMO_BOLA);
    minijuego.estadosJugadores[indice].tiempoEmpujando = 0.0f;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RECOGER_OBJETO);
}


static void ActualizarJugadorBolas(
    MinijuegoBolasAzucar& minijuego,
    int indice,
    float deltaTime,
    JugadorPrueba jugadores[],
    Participante participantes[]
)
{
    EstadoJugadorBolasAzucar& estado = minijuego.estadosJugadores[indice];
    JugadorPrueba& jugador = jugadores[indice];

    if (estado.inmunidad > 0.0f)
    {
        estado.inmunidad -= deltaTime;
        if (estado.inmunidad < 0.0f) estado.inmunidad = 0.0f;
    }

    bool bloqueado = estado.aturdido > 0.0f || estado.creando > 0.0f;

    if (estado.aturdido > 0.0f)
    {
        estado.aturdido -= deltaTime;
        if (estado.aturdido < 0.0f) estado.aturdido = 0.0f;
    }

    if (estado.creando > 0.0f)
    {
        estado.creando -= deltaTime;

        if (estado.creando <= 0.0f)
        {
            estado.creando = 0.0f;
            CrearBolaBolas(minijuego, indice, jugador);
        }
    }

    InputMinijuegoParticipante entrada{};
    bool accion = false;

    if (EsControlBotBolas(participantes[indice]))
    {
        if (!bloqueado)
        {
            entrada = CrearEntradaBotBolas(minijuego, indice, jugadores, deltaTime);
            accion = entrada.golpear;
        }
    }
    else
    {
        InputMinijuegoParticipante leida = LeerInputMinijuegoParticipante(participantes[indice]);
        accion = leida.golpear && !estado.accionPrevia && !bloqueado;
        estado.accionPrevia = leida.golpear;

        if (!bloqueado)
        {
            entrada = leida;
        }
    }

    entrada.golpear = false;
    entrada.saltar = false;

    if (accion)
    {
        BolaAzucar& bola = minijuego.bolas[indice];

        if (bola.activa && bola.estado == BOLA_AZUCAR_EMPUJADA)
        {
            LanzarBolaBolas(minijuego, indice, jugador);
        }
        else
        {
            estado.creando = DURACION_CREAR_BOLA;
            entrada = {};
        }
    }

    if (EnChocolateBolas(jugador.posicion.x, jugador.posicion.z))
    {
        jugador.tiempoRalentizado = 0.15f;
        jugador.multiplicadorRalentizacion = 0.5f;
    }

    ActualizarJugadorPruebaNormal(
        jugador,
        entrada,
        minijuego.bloques,
        minijuego.cantidadBloques,
        minijuego.particulas,
        MAX_PARTICULAS_BOLAS_AZUCAR,
        false,
        false,
        deltaTime
    );

    jugador.posicion.x += estado.empujeX * deltaTime;
    jugador.posicion.z += estado.empujeZ * deltaTime;

    float amortiguacion = 1.0f - 5.0f * deltaTime;
    if (amortiguacion < 0.0f) amortiguacion = 0.0f;
    estado.empujeX *= amortiguacion;
    estado.empujeZ *= amortiguacion;

    jugador.posicion.x = LimitarBolas(jugador.posicion.x, -LIMITE_JUGADOR_BOLAS, LIMITE_JUGADOR_BOLAS);
    jugador.posicion.z = LimitarBolas(jugador.posicion.z, -LIMITE_JUGADOR_BOLAS, LIMITE_JUGADOR_BOLAS);

    if (jugador.cayendo || jugador.posicion.y < -3.0f)
    {
        ReiniciarJugadorPrueba(jugador);
    }

    BolaAzucar& bola = minijuego.bolas[indice];

    if (bola.activa && bola.estado == BOLA_AZUCAR_EMPUJADA)
    {
        estado.tiempoEmpujando += deltaTime;
    }
}


static void ActualizarBolaBolas(
    MinijuegoBolasAzucar& minijuego,
    int indice,
    float deltaTime,
    const JugadorPrueba jugadores[]
)
{
    BolaAzucar& bola = minijuego.bolas[indice];

    if (!bola.activa)
    {
        return;
    }

    const EstadoJugadorBolasAzucar& duenio = minijuego.estadosJugadores[indice];
    float x0 = bola.x;
    float z0 = bola.z;

    if (bola.estado == BOLA_AZUCAR_EMPUJADA)
    {
        if (!duenio.vivo)
        {
            bola.activa = false;
            return;
        }

        if (duenio.aturdido > 0.0f)
        {
            bola.estado = BOLA_AZUCAR_REPOSO;
            bola.velocidadX = 0.0f;
            bola.velocidadZ = 0.0f;
            bola.inactiva = 0.0f;
        }
        else
        {
            const JugadorPrueba& jugador = jugadores[indice];
            float dx = jugador.direccionMirada.x;
            float dz = jugador.direccionMirada.z;
            float longitud = std::sqrt(dx * dx + dz * dz);

            if (longitud > 0.01f)
            {
                float distancia = 0.45f + bola.radio;
                bola.x = jugador.posicion.x + dx / longitud * distancia;
                bola.z = jugador.posicion.z + dz / longitud * distancia;
            }
        }
    }
    else if (bola.estado == BOLA_AZUCAR_LANZADA)
    {
        bola.x += bola.velocidadX * deltaTime;
        bola.z += bola.velocidadZ * deltaTime;

        float velocidad = std::sqrt(bola.velocidadX * bola.velocidadX + bola.velocidadZ * bola.velocidadZ);
        float frenado = 2.5f + bola.radio * 1.2f;

        if (EnChocolateBolas(bola.x, bola.z))
        {
            frenado += 6.0f;
        }

        float nueva = velocidad - frenado * deltaTime;

        if (nueva < 1.2f)
        {
            bola.estado = BOLA_AZUCAR_REPOSO;
            bola.velocidadX = 0.0f;
            bola.velocidadZ = 0.0f;
            bola.inactiva = 0.0f;
        }
        else
        {
            float factor = nueva / velocidad;
            bola.velocidadX *= factor;
            bola.velocidadZ *= factor;
        }
    }
    else
    {
        bola.inactiva += deltaTime;

        if (bola.inactiva > 3.0f)
        {
            bola.activa = false;
            return;
        }
    }

    // Rebote en las gominolas.
    if (bola.estado != BOLA_AZUCAR_REPOSO)
    {
        for (const CirculoBolas& gominola : GOMINOLAS_BOLAS)
        {
            float dx = bola.x - gominola.x;
            float dz = bola.z - gominola.z;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float minimo = gominola.radio + bola.radio;

            if (distancia >= minimo)
            {
                continue;
            }

            if (distancia < 0.01f)
            {
                dx = 1.0f;
                dz = 0.0f;
                distancia = 1.0f;
            }

            float nx = dx / distancia;
            float nz = dz / distancia;
            bola.x = gominola.x + nx * minimo;
            bola.z = gominola.z + nz * minimo;

            float velocidad = std::sqrt(bola.velocidadX * bola.velocidadX + bola.velocidadZ * bola.velocidadZ);
            float rebote = velocidad < 4.5f ? 4.5f : velocidad;
            bola.velocidadX = nx * rebote;
            bola.velocidadZ = nz * rebote;
            bola.estado = BOLA_AZUCAR_LANZADA;
        }
    }

    // Muros de la arena.
    float limite = MITAD_ARENA_BOLAS - bola.radio;

    if (bola.x < -limite || bola.x > limite)
    {
        bola.x = LimitarBolas(bola.x, -limite, limite);
        bola.velocidadX = -bola.velocidadX * 0.7f;
    }

    if (bola.z < -limite || bola.z > limite)
    {
        bola.z = LimitarBolas(bola.z, -limite, limite);
        bola.velocidadZ = -bola.velocidadZ * 0.7f;
    }

    float recorrido = std::sqrt((bola.x - x0) * (bola.x - x0) + (bola.z - z0) * (bola.z - z0));

    if (recorrido > 1.0f)
    {
        recorrido = 1.0f;
    }

    bola.giro += recorrido / bola.radio;

    if (EnChocolateBolas(bola.x, bola.z))
    {
        bola.radio -= 0.55f * deltaTime;

        if (bola.radio < 0.22f)
        {
            CrearPolvoAzucarBolas(minijuego, bola.x, 0.3f, bola.z, Color{ 110, 64, 40, 255 });
            bola.activa = false;
            return;
        }
    }
    else if (EnAzucarBolas(bola.x, bola.z))
    {
        bola.radio += recorrido * CRECIMIENTO_POR_UNIDAD_BOLA;

        if (bola.radio > RADIO_MAXIMO_BOLA)
        {
            bola.radio = RADIO_MAXIMO_BOLA;
        }
    }
}


static void EliminarJugadorBolas(
    MinijuegoBolasAzucar& minijuego,
    int indice,
    JugadorPrueba jugadores[]
)
{
    EstadoJugadorBolasAzucar& estado = minijuego.estadosJugadores[indice];

    estado.posicionEliminacion = ContarVivosBolas(minijuego);
    estado.vivo = false;
    estado.vidas = 0;
    minijuego.bolas[indice].activa = false;
    jugadores[indice].posicion = { 0.0f, -30.0f, 0.0f };
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
}


static void ResolverImpactosBolas(
    MinijuegoBolasAzucar& minijuego,
    JugadorPrueba jugadores[]
)
{
    for (int b = 0; b < MAX_PARTICIPANTES; b++)
    {
        BolaAzucar& bola = minijuego.bolas[b];

        if (!bola.activa || bola.estado == BOLA_AZUCAR_REPOSO)
        {
            continue;
        }

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            EstadoJugadorBolasAzucar& objetivo = minijuego.estadosJugadores[j];

            if (
                j == b ||
                !minijuego.resultado.participantes[j].participo ||
                !objetivo.vivo ||
                objetivo.inmunidad > 0.0f
            )
            {
                continue;
            }

            float dx = jugadores[j].posicion.x - bola.x;
            float dz = jugadores[j].posicion.z - bola.z;
            float distancia = std::sqrt(dx * dx + dz * dz);

            if (distancia > bola.radio + 0.45f)
            {
                continue;
            }

            if (distancia < 0.01f)
            {
                dx = 1.0f;
                dz = 0.0f;
                distancia = 1.0f;
            }

            bool grande = bola.radio > RADIO_DANO_BOLA;
            float fuerza = grande ? 7.0f : 4.5f;

            objetivo.empujeX = dx / distancia * fuerza;
            objetivo.empujeZ = dz / distancia * fuerza;
            objetivo.creando = 0.0f;
            minijuego.mensajeJugador = j;
            minijuego.mensajeTiempo = 1.6f;

            CrearPolvoAzucarBolas(minijuego, bola.x, 0.6f, bola.z, Color{ 255, 250, 255, 255 });

            if (grande)
            {
                objetivo.vidas--;
                objetivo.aturdido = 1.1f;
                objetivo.inmunidad = 1.5f;
                minijuego.mensajeTipo = 1;
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);

                if (objetivo.vidas <= 0)
                {
                    EliminarJugadorBolas(minijuego, j, jugadores);
                    minijuego.mensajeTipo = 3;
                }
            }
            else
            {
                objetivo.aturdido = 0.7f;
                objetivo.inmunidad = 0.8f;
                minijuego.mensajeTipo = 2;
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GOLPE);
            }

            bola.activa = false;
            break;
        }
    }
}


//==================================================
// INTERFAZ
//==================================================


// Definidas en la seccion visual.
static void CargarBolaJugableAzucar();
static void LiberarMallasAzucar();


void MinijuegoBolasAzucar::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        bolas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_BOLAS_AZUCAR; i++)
    {
        particulas[i] = {};
    }

    ConfigurarArenaBolas(*this);

    camara.position = { 0.0f, 20.0f, 11.0f };
    camara.target = { 0.0f, 0.0f, 0.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_BOLAS_PREPARACION;
    mensajeJugador = -1;
    mensajeTipo = 0;
    mensajeTiempo = 0.0f;
    tiempoRestante = DURACION_PARTIDA_BOLAS;
    tiempoPreparacion = DURACION_PREPARACION_BOLAS;
    tiempoAnimacion = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        golpeVisual[i] = 99.0f;
        salidaVisual[i] = 99.0f;
        salidaX[i] = 0.0f;
        salidaZ[i] = 0.0f;
    }

    tiempoFin = 0.0f;
}


void MinijuegoBolasAzucar::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    // Cada partida vuelve a hornear el escenario (la calidad puede cambiar).
    LiberarMallasAzucar();
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_BOLAS_TERMINADO;
        return;
    }

    const float esquinasX[MAX_PARTICIPANTES] = { -7.5f, 7.5f, 7.5f, -7.5f };
    CargarBolaJugableAzucar();
    const float esquinasZ[MAX_PARTICIPANTES] = { -7.5f, 7.5f, -7.5f, 7.5f };

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
    int ranura = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        estadosJugadores[i].vivo = true;
        estadosJugadores[i].vidas = VIDAS_BOLAS_AZUCAR;

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            Vector3{ esquinasX[ranura], 0.7f, esquinasZ[ranura] }
        );
        jugadores[i].direccionMirada = { -esquinasX[ranura], 0.0f, -esquinasZ[ranura] };
        ranura++;
    }
}


void MinijuegoBolasAzucar::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    // Relojes solo visuales.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        golpeVisual[i] += deltaTime;
        salidaVisual[i] += deltaTime;
    }

    if (fase == FASE_BOLAS_TERMINADO)
    {
        tiempoFin += deltaTime;
    }

    if (
        fase == FASE_BOLAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_BOLAS_AZUCAR, deltaTime);

    if (mensajeTiempo > 0.0f)
    {
        mensajeTiempo -= deltaTime;
    }

    if (fase == FASE_BOLAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_BOLAS_JUGANDO;
        }

        return;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    // Un humano desconectado se trata como bot; para las funciones
    // compartidas se le considera presente. Los eliminados no colisionan.
    Participante efectivos[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectivos[i] = participantes[i];

        if (efectivos[i].activo)
        {
            efectivos[i].conectado = true;
            efectivos[i].activo = estadosJugadores[i].vivo;
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo && estadosJugadores[i].vivo)
        {
            ActualizarJugadorBolas(*this, i, deltaTime, jugadores, participantes);
        }
    }

    ResolverColisionesJugadoresSinEmpuje(jugadores, efectivos, limite);

    for (int i = 0; i < limite; i++)
    {
        ActualizarBolaBolas(*this, i, deltaTime, jugadores);
    }

    // Solo visual: se anota quien recibe un golpe y donde cae el eliminado.
    int vidasAntes[MAX_PARTICIPANTES];
    float aturdidoAntes[MAX_PARTICIPANTES];
    float xAntes[MAX_PARTICIPANTES];
    float zAntes[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        vidasAntes[i] = estadosJugadores[i].vidas;
        aturdidoAntes[i] = estadosJugadores[i].aturdido;
        xAntes[i] = jugadores[i].posicion.x;
        zAntes[i] = jugadores[i].posicion.z;
    }

    ResolverImpactosBolas(*this, jugadores);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorBolasAzucar& estado = estadosJugadores[i];

        if (estado.vidas < vidasAntes[i] || estado.aturdido > aturdidoAntes[i] + 0.01f)
        {
            golpeVisual[i] = 0.0f;

            if (estado.vidas < vidasAntes[i])
            {
                ActivarTemblorCamaraGeneral(0.14f, 0.24f);
            }
        }

        if (!estado.vivo && vidasAntes[i] > 0 && resultado.participantes[i].participo)
        {
            salidaVisual[i] = 0.0f;
            salidaX[i] = xAntes[i];
            salidaZ[i] = zAntes[i];
        }
    }

    if (ContarVivosBolas(*this) <= 1 || tiempoRestante <= 0.0f)
    {
        tiempoRestante = tiempoRestante < 0.0f ? 0.0f : tiempoRestante;
        FinalizarBolas(*this);
    }
}


//==================================================
// VISUAL: MONTANA DE DULCES NEVADA DE AZUCAR GLAS
//==================================================
//
// Una meseta de galleta en la ladera de una montana de dulces, nevada de
// azucar glas, a la tarde bajo un cielo de algodon de azucar. La arena es
// una losa de galletas cuadradas cercada de barquillos rellenos; el azucar
// glas (hace crecer la bola) son colchones blancos con brillos dorados y
// el chocolate (la derrite) son charcos brillantes con ondas y burbujas.
// Alrededor: piruletas, columnas de caramelo, merengues, malvaviscos,
// bastones, caramelos envueltos, arboles de algodon y faroles de chicle;
// un rio de chocolate con cascada a la izquierda, colinas de nata con
// cerezas, montanas lejanas y un mar de nubes de algodon.
//
// Lo estatico se hornea en mallas (niebla de color en los vertices) y se
// rehornea si cambia la calidad. Todo lo jugable sale de las tablas
// logicas (ZONAS_AZUCAR_BOLAS, CHARCOS_BOLAS, GOMINOLAS_BOLAS, bloques).
// Del paquete GLB solo se carga la bola, que es la pieza jugable.
//==================================================


static const Vector3 OJO_AZ = { 0.0f, 15.0f, 19.6f };
static const Vector3 MIRA_AZ = { 0.0f, -1.0f, -0.4f };
static const float FOV_AZ = 46.0f;

static const Color CIELO_ALTO_AZ = { 128, 142, 226, 255 };
static const Color CIELO_MEDIO_AZ = { 212, 166, 230, 255 };
static const Color CIELO_BAJO_AZ = { 255, 192, 212, 255 };
static const Color HORIZONTE_AZ = { 255, 224, 210, 255 };
static const Color BRUMA_AZ = { 248, 212, 226, 255 };

static const Color GALLETA_AZ = { 214, 156, 90, 255 };
static const Color GALLETA_OSCURA_AZ = { 150, 96, 50, 255 };
static const Color AZUCAR_AZ = { 253, 251, 255, 255 };
static const Color AZUCAR_SOMBRA_AZ = { 216, 208, 240, 255 };
static const Color NIEVE_AZ = { 246, 240, 252, 255 };
static const Color CHOCOLATE_AZ = { 82, 44, 28, 255 };
static const Color CHOCOLATE_BRILLO_AZ = { 160, 102, 70, 255 };
static const Color NATA_AZ = { 255, 242, 230, 255 };
static const Color ROSA_AZ = { 255, 150, 196, 255 };
static const Color MENTA_AZ = { 132, 226, 204, 255 };
static const Color LILA_AZ = { 184, 148, 240, 255 };
static const Color CARAMELO_AZ = { 236, 146, 44, 255 };
static const Color BARQUILLO_AZ = { 238, 192, 124, 255 };
// Acento: los brillos dorados del azucar glas (donde la bola crece).
static const Color ACENTO_AZ = { 255, 232, 130, 255 };
static const Color LUZ_FAROL_AZ = { 255, 210, 140, 255 };

static const Color COLORES_GOMINOLA_AZ[4] =
{
    { 255, 112, 176, 255 },
    { 176, 128, 246, 255 },
    { 64, 206, 196, 255 },
    { 255, 168, 120, 255 }
};

// Faroles de chicle (fuentes de luz con charco).
static const Vector3 FAROLES_AZ[4] =
{
    { -10.7f, 0.0f, -10.8f },
    { 10.8f, 0.0f, -10.8f },
    { 10.8f, 0.0f, 2.2f },
    { -10.7f, 0.0f, 4.6f }
};


//--------------------------------------------------
// Utilidades de color y azar visual (semilla propia)
//--------------------------------------------------

static float Lim01Az(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalAz(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoAz(Color c, float f)
{
    return Color{ CanalAz(c.r * f), CanalAz(c.g * f), CanalAz(c.b * f), c.a };
}


static Color MezclaAz(Color a, Color b, float t)
{
    t = Lim01Az(t);
    return Color{
        CanalAz(a.r + (b.r - a.r) * t),
        CanalAz(a.g + (b.g - a.g) * t),
        CanalAz(a.b + (b.b - a.b) * t),
        CanalAz(a.a + (b.a - a.a) * t)
    };
}


static Color AlfaAz(Color c, float a)
{
    c.a = CanalAz(255.0f * Lim01Az(a));
    return c;
}


static unsigned int HashAz(int a, int b)
{
    unsigned int h = (unsigned int)a * 73856093u ^ (unsigned int)b * 19349663u ^ 0x9E3779B9u;
    h ^= h >> 13;
    h *= 0x5BD1E995u;
    h ^= h >> 15;
    return h;
}


// [0, 1) determinista por indice.
static float AzarAz(int a, int b)
{
    return (float)(HashAz(a, b) & 0xFFFFu) / 65536.0f;
}


static float SuaveAz(float u)
{
    u = Lim01Az(u);
    return u * u * (3.0f - 2.0f * u);
}


//--------------------------------------------------
// Emision de triangulos (inmediata o grabada en malla)
//--------------------------------------------------

struct GrabacionAz
{
    bool activa = false;
    std::vector<float> posiciones;
    std::vector<unsigned char> colores;
};

static GrabacionAz g_grabAz;
static Vector3 g_ojoAz = OJO_AZ;
static float g_emisAz = 0.0f;        // 1 = luminoso: la bruma casi no lo tapa
static bool g_nieblaAz = true;        // false: color exacto
static Vector3 g_derAz = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaAz = { 0.0f, 1.0f, 0.0f };


static Color BrumaAz(Color c, Vector3 p)
{
    if (!g_nieblaAz)
    {
        return c;
    }

    float dx = p.x - g_ojoAz.x;
    float dy = p.y - g_ojoAz.y;
    float dz = p.z - g_ojoAz.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01Az((d - 30.0f) / 170.0f);
    n = n * (2.0f - n) * 0.86f * (1.0f - 0.6f * g_emisAz);

    // Mar de nubes: lo bajo se funde con la bruma.
    if (p.y < -4.0f)
    {
        float bajo = Lim01Az((-4.0f - p.y) / 30.0f) * 0.92f;
        if (bajo > n) n = bajo;
    }

    Color r = MezclaAz(c, BRUMA_AZ, n);
    r.a = c.a;
    return r;
}


static void VAz(Vector3 p, Color c)
{
    if (g_grabAz.activa)
    {
        g_grabAz.posiciones.push_back(p.x);
        g_grabAz.posiciones.push_back(p.y);
        g_grabAz.posiciones.push_back(p.z);
        g_grabAz.colores.push_back(c.r);
        g_grabAz.colores.push_back(c.g);
        g_grabAz.colores.push_back(c.b);
        g_grabAz.colores.push_back(c.a);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteAz()
{
    if (!g_grabAz.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteAz()
{
    if (!g_grabAz.activa) rlEnd();
}


static void TriAz(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VAz(a, BrumaAz(color, a));
    VAz(b, BrumaAz(color, b));
    VAz(c, BrumaAz(color, c));
}


static void QuadAz(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriAz(a, b, c, color);
    TriAz(a, c, d, color);
}


// Un color por vertice, con bruma.
static void QuadVAz(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    Color fa = BrumaAz(ca, a), fb = BrumaAz(cb, b), fc = BrumaAz(cc, c), fd = BrumaAz(cd, d);
    VAz(a, fa); VAz(b, fb); VAz(c, fc);
    VAz(a, fa); VAz(c, fc); VAz(d, fd);
}


// Un color por vertice, sin bruma (luces, sombras suaves y halos).
static void QuadColAz(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VAz(a, ca); VAz(b, cb); VAz(c, cc);
    VAz(a, ca); VAz(c, cc); VAz(d, cd);
}


// Luz del sol de la tarde: arriba 1.0, frente ~0.8, izquierda ~0.7, derecha ~0.6.
static float LuzAz(Vector3 n)
{
    float d = n.x * -0.30f + n.y * 0.84f + n.z * 0.45f;
    return 0.58f + 0.44f * Lim01Az(d);
}


// Caja con caras sombreadas.
static void CajaAz(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    QuadAz({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, color);
    QuadAz({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoAz(color, 0.80f));
    QuadAz({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoAz(color, 0.56f));
    QuadAz({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoAz(color, 0.70f));
    QuadAz({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoAz(color, 0.62f));

    if (y0 > 0.02f)
    {
        QuadAz({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoAz(color, 0.42f));
    }
}


// Caja girada sobre el eje Y (angulo en radianes).
static void CajaGiradaAz(Vector3 c, Vector3 t, float angulo, Color color)
{
    float ca = std::cos(angulo), sa = std::sin(angulo);
    auto P = [&](float x, float y, float z)
    {
        return Vector3{ c.x + x * ca - z * sa, c.y + y, c.z + x * sa + z * ca };
    };
    float hx = t.x * 0.5f, hy = t.y * 0.5f, hz = t.z * 0.5f;
    Vector3 caras[4][4] =
    {
        { P(-hx, -hy, hz), P(hx, -hy, hz), P(hx, hy, hz), P(-hx, hy, hz) },
        { P(hx, -hy, -hz), P(-hx, -hy, -hz), P(-hx, hy, -hz), P(hx, hy, -hz) },
        { P(-hx, -hy, -hz), P(-hx, -hy, hz), P(-hx, hy, hz), P(-hx, hy, -hz) },
        { P(hx, -hy, hz), P(hx, -hy, -hz), P(hx, hy, -hz), P(hx, hy, hz) }
    };
    Vector3 normales[4] = { { -sa, 0, ca }, { sa, 0, -ca }, { -ca, 0, -sa }, { ca, 0, sa } };

    QuadAz(P(-hx, hy, -hz), P(-hx, hy, hz), P(hx, hy, hz), P(hx, hy, -hz), color);
    for (int k = 0; k < 4; k++)
    {
        QuadAz(caras[k][0], caras[k][1], caras[k][2], caras[k][3], TonoAz(color, LuzAz(normales[k])));
    }
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas por normal.
static void CilindroAz(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    eje = Vector3Scale(eje, 1.0f / largo);
    Vector3 referencia = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, referencia));
    Vector3 v = Vector3CrossProduct(eje, u);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = 0.5f * (a0 + a1);
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));
        Color cara = TonoAz(color, LuzAz(nm));

        QuadAz(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            cara
        );

        if (tapas && rb > 0.0001f)
        {
            TriAz(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), TonoAz(color, LuzAz(eje)));
        }
    }
}


// Elipsoide (o casquete desde la latitud phiMin) con luz por vertice.
static void ElipsoideAz(Vector3 c, Vector3 r, int lados, int pisos, Color color, float phiMin = -PI * 0.5f)
{
    auto P = [&](float a, float p)
    {
        return Vector3{ c.x + r.x * std::cos(p) * std::cos(a), c.y + r.y * std::sin(p), c.z + r.z * std::cos(p) * std::sin(a) };
    };
    auto L = [&](float a, float p)
    {
        Vector3 n = { std::cos(p) * std::cos(a), std::sin(p), std::cos(p) * std::sin(a) };
        return TonoAz(color, LuzAz(n));
    };

    for (int j = 0; j < pisos; j++)
    {
        float p0 = phiMin + (PI * 0.5f - phiMin) * (float)j / (float)pisos;
        float p1 = phiMin + (PI * 0.5f - phiMin) * (float)(j + 1) / (float)pisos;

        for (int k = 0; k < lados; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)lados;
            float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            QuadVAz(P(a0, p0), P(a0, p1), P(a1, p1), P(a1, p0), L(a0, p0), L(a0, p1), L(a1, p1), L(a1, p0));
        }
    }
}


// Disco horizontal mirando arriba.
static void DiscoAz(Vector3 c, float r, int lados, Color color)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        TriAz(c, { c.x + std::cos(a1) * r, c.y, c.z + std::sin(a1) * r }, { c.x + std::cos(a0) * r, c.y, c.z + std::sin(a0) * r }, color);
    }
}


// Anillo horizontal con color interior y exterior (degradado radial).
static void AnilloAz(Vector3 c, float r0, float r1, int lados, Color dentro, Color fuera, bool niebla = true)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };

        if (niebla) QuadVAz(i0, i1, o1, o0, dentro, dentro, fuera, fuera);
        else QuadColAz(i0, i1, o1, o0, dentro, dentro, fuera, fuera);
    }
}


// Sombra suave en el suelo (alfa en el centro que se apaga en el borde).
static void SombraAz(float x, float z, float rx, float rz, float alfa)
{
    Color centro = { 70, 40, 70, CanalAz(255.0f * alfa) };
    Color borde = { 70, 40, 70, 0 };
    const int lados = 14;

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 c = { x, 0.0f, z };
        Vector3 m0 = { x + std::cos(a0) * rx * 0.55f, 0.0f, z + std::sin(a0) * rz * 0.55f };
        Vector3 m1 = { x + std::cos(a1) * rx * 0.55f, 0.0f, z + std::sin(a1) * rz * 0.55f };
        Vector3 o0 = { x + std::cos(a0) * rx, 0.0f, z + std::sin(a0) * rz };
        Vector3 o1 = { x + std::cos(a1) * rx, 0.0f, z + std::sin(a1) * rz };
        VAz(c, centro); VAz(m1, centro); VAz(m0, centro);
        QuadColAz(m0, m1, o1, o0, centro, centro, borde, borde);
    }
}


// Billboard (mira a la camara).
static void CartelAz(Vector3 c, float ancho, float alto, Color color)
{
    Vector3 dx = Vector3Scale(g_derAz, ancho * 0.5f);
    Vector3 dy = Vector3Scale(g_arribaAz, alto * 0.5f);
    Vector3 a = Vector3Subtract(Vector3Subtract(c, dx), dy);
    Vector3 b = Vector3Subtract(Vector3Add(c, dx), dy);
    Vector3 d = Vector3Add(Vector3Add(c, dx), dy);
    Vector3 e = Vector3Add(Vector3Subtract(c, dx), dy);
    QuadColAz(a, b, d, e, color, color, color, color);
}


// Halo redondo mirando a la camara: centro con color, borde transparente.
static void HaloAz(Vector3 c, float r, Color color)
{
    Color nada = AlfaAz(color, 0.0f);
    const int lados = 16;

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados, a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derAz, std::cos(a0) * r), Vector3Scale(g_arribaAz, std::sin(a0) * r)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derAz, std::cos(a1) * r), Vector3Scale(g_arribaAz, std::sin(a1) * r)));
        VAz(c, color);
        VAz(p0, nada);
        VAz(p1, nada);
    }
}


// Destello en cruz (cuatro puntas) mirando a la camara.
static void DestelloAz(Vector3 c, float r, Color color)
{
    Color nada = AlfaAz(color, 0.0f);
    Vector3 dx = Vector3Scale(g_derAz, r);
    Vector3 dy = Vector3Scale(g_arribaAz, r);
    Vector3 ex = Vector3Scale(g_derAz, r * 0.18f);
    Vector3 ey = Vector3Scale(g_arribaAz, r * 0.18f);
    QuadColAz(Vector3Subtract(c, dx), Vector3Subtract(c, ey), Vector3Add(c, dx), Vector3Add(c, ey), nada, color, nada, color);
    QuadColAz(Vector3Subtract(c, dy), Vector3Add(c, ex), Vector3Add(c, dy), Vector3Subtract(c, ex), nada, color, nada, color);
}


//--------------------------------------------------
// Pieza jugable GLB: solo la bola
//--------------------------------------------------

static void CargarBolaJugableAzucar()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteBolasAzucarRetro3D();
    PaqueteModelosEscenarioRetro3D tramo = paquete;
    tramo.recursos += MODELO_AZUCAR_BOLA;
    tramo.definiciones += MODELO_AZUCAR_BOLA;
    tramo.cantidad = 1;
    CargarPaqueteModelosEscenarioRetro3D(tramo);

    // Mismas primitives que exige CargarPaqueteBolasAzucarRetro3D.
    RecursoModeloEscenarioRetro3D& bola = paquete.recursos[MODELO_AZUCAR_BOLA];

    if (bola.cargado && bola.modelo.meshCount != 3)
    {
        UnloadModel(bola.modelo);
        bola.modelo = {};
        bola.cargado = false;
        TraceLog(LOG_WARNING, "Paquete Bolas de Azucar incompatible; se usan primitivas: %s", bola.ruta);
    }
}


//--------------------------------------------------
// Horneado del escenario en mallas
//--------------------------------------------------

enum MallaAz
{
    MALLA_AZ_BASE = 0,      // solo profundidad bajo el suelo pintado
    MALLA_AZ_SUELO,         // suelo pintado en orden, sin profundidad
    MALLA_AZ_FONDO,         // colinas, montanas y mar de nubes (con profundidad)
    MALLA_AZ_UTILERIA,      // muros y utileria (opaco, con profundidad)
    MALLA_AZ_CIELO,         // telon del cielo y sol (despues de lo opaco)
    MALLA_AZ_NUBE_A,        // nubes de algodon (se dibujan con traslacion)
    MALLA_AZ_NUBE_B,
    MALLA_AZ_NUBE_C,
    CANTIDAD_MALLAS_AZ
};

struct HornoAz
{
    bool listo = false;
    int clave = -1;
    Mesh mallas[CANTIDAD_MALLAS_AZ]{};
    bool validas[CANTIDAD_MALLAS_AZ]{};
    bool materialListo = false;
    Material material{};
};

static HornoAz g_hornoAz;


static void EmpezarGrabacionAz()
{
    g_grabAz.activa = true;
    g_grabAz.posiciones.clear();
    g_grabAz.colores.clear();
}


// Cierra la grabacion y sube la malla (sin indices: cabe cualquier tamano).
static bool TerminarGrabacionAz(Mesh& malla)
{
    g_grabAz.activa = false;
    int n = (int)(g_grabAz.posiciones.size() / 3);
    n -= n % 3;

    if (n < 3)
    {
        g_grabAz.posiciones.clear();
        g_grabAz.colores.clear();
        return false;
    }

    malla = {};
    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * (int)sizeof(float)));
    malla.texcoords = (float*)MemAlloc((unsigned int)(n * 2 * (int)sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    for (int i = 0; i < n * 3; i++) malla.vertices[i] = g_grabAz.posiciones[(size_t)i];
    for (int i = 0; i < n * 4; i++) malla.colors[i] = g_grabAz.colores[(size_t)i];
    for (int i = 0; i < n * 2; i++) malla.texcoords[i] = 0.0f;

    UploadMesh(&malla, false);
    g_grabAz.posiciones.clear();
    g_grabAz.colores.clear();
    return true;
}


static void LiberarMallasAzucar()
{
    for (int i = 0; i < CANTIDAD_MALLAS_AZ; i++)
    {
        if (g_hornoAz.validas[i]) UnloadMesh(g_hornoAz.mallas[i]);
        g_hornoAz.validas[i] = false;
        g_hornoAz.mallas[i] = {};
    }

    g_hornoAz.listo = false;
    g_hornoAz.clave = -1;
}


static int ClaveHornoAz()
{
    return (int)CalidadDecoracion() * 16 + (int)CalidadEfectos() * 4 + (CalidadSombras() != CALIDAD_BAJA ? 1 : 0);
}


//--------------------------------------------------
// Capa 1: suelo (meseta nevada, losa de galletas, azucar y chocolate)
//--------------------------------------------------

// Rio de chocolate a la izquierda: baja desde la cascada del fondo y pasa
// junto a la arena hacia la camara.
static const float Z_CABECERA_RIO_AZ = -24.6f;
static const float Z_FIN_RIO_AZ = 26.0f;

static float RioXAz(float z)
{
    float x = -14.2f + 0.8f * std::sin(0.21f * z + 0.5f);
    if (z < -8.0f) x -= (-8.0f - z) * 0.18f;
    return x;
}


static float RioAnchoAz(float z)
{
    return 2.6f + 0.25f * std::sin(0.37f * z + 1.3f) + (z < -18.0f ? (-18.0f - z) * 0.12f : 0.0f);
}


// Utileria del anillo cercano y de la media distancia (posiciones fijas).
enum TipoPropAz
{
    PROP_AZ_PIRULETA = 0,
    PROP_AZ_BASTON,
    PROP_AZ_MERENGUE,
    PROP_AZ_MALVAVISCOS,
    PROP_AZ_CARAMELO,
    PROP_AZ_ARBOL,
    PROP_AZ_COLUMNA,
    PROP_AZ_GALLETA,
    PROP_AZ_GOMITAS
};

struct PropAz
{
    TipoPropAz tipo;
    float x;
    float z;
    float escala;
    float giro;
    int variante;
    bool media;     // true: solo en calidad ALTA (paso 2 en MEDIA)
};

static const PropAz PROPS_AZ[] =
{
    // Lado derecho: bosquecillo de piruletas.
    { PROP_AZ_PIRULETA, 13.0f, -5.6f, 1.25f, 0.3f, 0, false },
    { PROP_AZ_PIRULETA, 15.8f, -0.8f, 1.0f, -0.4f, 1, false },
    { PROP_AZ_PIRULETA, 12.6f, 6.4f, 0.9f, 0.6f, 2, false },
    { PROP_AZ_PIRULETA, 18.2f, 4.8f, 1.35f, 0.1f, 3, true },
    { PROP_AZ_BASTON, 11.5f, -2.4f, 0.9f, 0.8f, 0, false },
    { PROP_AZ_BASTON, 14.3f, 9.4f, 1.0f, 2.4f, 1, true },
    { PROP_AZ_MERENGUE, 13.4f, 1.9f, 1.0f, 0.0f, 0, false },
    { PROP_AZ_MERENGUE, 19.6f, -6.4f, 1.3f, 1.0f, 1, true },
    { PROP_AZ_MALVAVISCOS, 11.7f, 9.8f, 1.0f, 0.4f, 0, false },
    { PROP_AZ_MALVAVISCOS, 16.6f, -8.6f, 1.1f, 1.2f, 1, true },
    { PROP_AZ_CARAMELO, 12.1f, -8.2f, 1.0f, 0.7f, 0, false },
    { PROP_AZ_CARAMELO, 16.2f, 11.2f, 1.1f, -0.5f, 1, true },
    { PROP_AZ_GOMITAS, 18.6f, 0.6f, 1.0f, 0.0f, 0, false },
    { PROP_AZ_ARBOL, 21.0f, -2.6f, 1.15f, 0.0f, 0, false },
    { PROP_AZ_ARBOL, 19.6f, 10.4f, 1.0f, 0.0f, 1, true },
    { PROP_AZ_GALLETA, 16.6f, 7.4f, 1.0f, -0.6f, 0, false },
    { PROP_AZ_ARBOL, 23.5f, 5.0f, 1.3f, 0.0f, 2, true },

    // Lado izquierdo: entre la arena y el rio, y del otro lado del rio.
    { PROP_AZ_MALVAVISCOS, -11.4f, -3.8f, 0.9f, 0.2f, 2, false },
    { PROP_AZ_MERENGUE, -11.2f, 9.8f, 0.9f, 0.5f, 2, false },
    { PROP_AZ_CARAMELO, -11.5f, 0.8f, 0.85f, 1.9f, 2, true },
    { PROP_AZ_PIRULETA, -18.4f, -4.2f, 1.2f, -0.3f, 4, false },
    { PROP_AZ_PIRULETA, -17.6f, 5.6f, 1.0f, 0.5f, 1, true },
    { PROP_AZ_ARBOL, -19.8f, 0.8f, 1.1f, 0.0f, 3, false },
    { PROP_AZ_ARBOL, -20.6f, -11.0f, 1.25f, 0.0f, 0, true },
    { PROP_AZ_GALLETA, -18.0f, 10.8f, 1.1f, 0.6f, 1, false },
    { PROP_AZ_BASTON, -17.2f, -9.6f, 1.0f, -0.8f, 2, false },
    { PROP_AZ_GOMITAS, -21.5f, 6.5f, 1.1f, 0.0f, 1, true },

    // Fondo cercano, detras del muro de atras.
    { PROP_AZ_MERENGUE, -6.6f, -11.4f, 0.95f, 0.3f, 0, false },
    { PROP_AZ_MERENGUE, 7.3f, -11.7f, 1.05f, 1.4f, 1, false },
    { PROP_AZ_MALVAVISCOS, -2.0f, -11.3f, 0.85f, 0.9f, 1, false },
    { PROP_AZ_CARAMELO, 3.6f, -11.0f, 0.9f, 0.2f, 0, false },
    { PROP_AZ_BASTON, -8.8f, -12.8f, 1.15f, 0.5f, 0, false },
    { PROP_AZ_BASTON, 9.2f, -13.1f, 1.1f, -2.2f, 1, false },
    { PROP_AZ_GOMITAS, 1.0f, -12.4f, 0.9f, 0.0f, 0, true },
    { PROP_AZ_GALLETA, -4.6f, -13.4f, 0.95f, 0.2f, 0, true },

    // Media distancia: columnas de caramelo, arboles y piruletas grandes.
    { PROP_AZ_COLUMNA, -12.4f, -14.4f, 1.0f, 0.0f, 0, false },
    { PROP_AZ_COLUMNA, 12.8f, -14.0f, 1.05f, 0.7f, 1, false },
    { PROP_AZ_COLUMNA, -3.9f, -16.4f, 0.8f, 0.3f, 0, false },
    { PROP_AZ_COLUMNA, 3.9f, -16.4f, 0.8f, 1.1f, 1, false },
    { PROP_AZ_PIRULETA, -7.6f, -18.6f, 1.7f, 0.2f, 0, false },
    { PROP_AZ_PIRULETA, 8.2f, -19.0f, 1.6f, -0.3f, 3, false },
    { PROP_AZ_ARBOL, -10.8f, -20.2f, 1.4f, 0.0f, 1, true },
    { PROP_AZ_ARBOL, 11.6f, -20.8f, 1.5f, 0.0f, 2, false },
    { PROP_AZ_ARBOL, 17.4f, -15.6f, 1.3f, 0.0f, 0, true },
    { PROP_AZ_ARBOL, 2.0f, -22.6f, 1.6f, 0.0f, 3, true },
    { PROP_AZ_GALLETA, 15.4f, -19.6f, 1.6f, 0.4f, 1, true },
    { PROP_AZ_MERENGUE, -1.6f, -19.4f, 1.6f, 0.0f, 2, true },
    { PROP_AZ_BASTON, 20.4f, -11.0f, 1.4f, 2.0f, 2, true },

    // Esquinas del frente.
    { PROP_AZ_MERENGUE, 11.6f, 12.0f, 0.9f, 0.4f, 1, true },
    { PROP_AZ_GOMITAS, -11.6f, 12.4f, 0.8f, 0.0f, 2, true }
};

static const int CANTIDAD_PROPS_AZ = (int)(sizeof(PROPS_AZ) / sizeof(PROPS_AZ[0]));


static bool PropVisibleAz(int i, int paso)
{
    return !PROPS_AZ[i].media || paso == 1 || (i % 2 == 0);
}


// Radio y altura aproximados (para su sombra).
static void MedidaPropAz(const PropAz& p, float& radio, float& alto)
{
    switch (p.tipo)
    {
        case PROP_AZ_PIRULETA: radio = 0.75f; alto = 3.0f; break;
        case PROP_AZ_BASTON: radio = 0.35f; alto = 2.3f; break;
        case PROP_AZ_MERENGUE: radio = 0.9f; alto = 1.3f; break;
        case PROP_AZ_MALVAVISCOS: radio = 0.8f; alto = 0.9f; break;
        case PROP_AZ_CARAMELO: radio = 0.9f; alto = 0.6f; break;
        case PROP_AZ_ARBOL: radio = 1.4f; alto = 3.8f; break;
        case PROP_AZ_COLUMNA: radio = 1.1f; alto = 7.5f; break;
        case PROP_AZ_GALLETA: radio = 1.0f; alto = 1.8f; break;
        default: radio = 0.9f; alto = 0.9f; break;
    }

    radio *= p.escala;
    alto *= p.escala;
}


// Galleta cuadrada en relieve: bisel claro hacia la luz, oscuro del otro lado.
static void LosaGalletaAz(float x0, float z0, float lado, int ix, int iz, bool decoracion)
{
    const float b = 0.12f;
    float x1 = x0 + lado, z1 = z0 + lado;
    Color c = TonoAz(GALLETA_AZ, 0.91f + 0.15f * AzarAz(ix + 40, iz));

    QuadAz({ x0 + b, 0, z0 + b }, { x0 + b, 0, z1 - b }, { x1 - b, 0, z1 - b }, { x1 - b, 0, z0 + b }, c);
    QuadAz({ x0, 0, z0 }, { x0 + b, 0, z0 + b }, { x1 - b, 0, z0 + b }, { x1, 0, z0 }, TonoAz(c, 0.80f));
    QuadAz({ x0, 0, z1 }, { x1, 0, z1 }, { x1 - b, 0, z1 - b }, { x0 + b, 0, z1 - b }, TonoAz(c, 1.10f));
    QuadAz({ x0, 0, z0 }, { x0, 0, z1 }, { x0 + b, 0, z1 - b }, { x0 + b, 0, z0 + b }, TonoAz(c, 1.05f));
    QuadAz({ x1, 0, z0 }, { x1 - b, 0, z0 + b }, { x1 - b, 0, z1 - b }, { x1, 0, z1 }, TonoAz(c, 0.76f));

    if (!decoracion)
    {
        return;
    }

    // Borde estampado y agujeritos de horneado.
    Color linea = TonoAz(c, 0.86f);
    float e = 0.32f, g = 0.045f;
    QuadAz({ x0 + e, 0, z0 + e }, { x0 + e, 0, z0 + e + g }, { x1 - e, 0, z0 + e + g }, { x1 - e, 0, z0 + e }, linea);
    QuadAz({ x0 + e, 0, z1 - e - g }, { x0 + e, 0, z1 - e }, { x1 - e, 0, z1 - e }, { x1 - e, 0, z1 - e - g }, linea);
    QuadAz({ x0 + e, 0, z0 + e }, { x0 + e, 0, z1 - e }, { x0 + e + g, 0, z1 - e }, { x0 + e + g, 0, z0 + e }, linea);
    QuadAz({ x1 - e - g, 0, z0 + e }, { x1 - e - g, 0, z1 - e }, { x1 - e, 0, z1 - e }, { x1 - e, 0, z0 + e }, linea);

    Color agujero = TonoAz(c, 0.66f);
    float cx = (x0 + x1) * 0.5f, cz = (z0 + z1) * 0.5f;
    const float puntos[5][2] = { { -0.38f, -0.38f }, { 0.38f, -0.38f }, { 0.0f, 0.0f }, { -0.38f, 0.38f }, { 0.38f, 0.38f } };

    for (int k = 0; k < 5; k++)
    {
        float px = cx + puntos[k][0], pz = cz + puntos[k][1];
        float r = 0.075f;
        QuadAz({ px - r, 0, pz }, { px, 0, pz + r }, { px + r, 0, pz }, { px, 0, pz - r }, agujero);
    }
}


static bool CercaDeAzucarAz(float x, float z, float margen)
{
    for (const RectanguloAzucarBolas& zona : ZONAS_AZUCAR_BOLAS)
    {
        if (x >= zona.xMin - margen && x <= zona.xMax + margen && z >= zona.zMin - margen && z <= zona.zMax + margen)
        {
            return true;
        }
    }

    return false;
}


// Colchon de azucar glas: halo suave, cuerpo moteado y borde de cristales
// dorados justo sobre el limite logico.
static void ColchonAzucarAz(const RectanguloAzucarBolas& zona, int indice, bool decoracion)
{
    float x0 = zona.xMin, x1 = zona.xMax, z0 = zona.zMin, z1 = zona.zMax;
    const float h = 0.22f;
    Color lleno = AlfaAz(AZUCAR_AZ, 0.9f);
    Color nada = AlfaAz(AZUCAR_AZ, 0.0f);

    QuadColAz({ x0 - h, 0, z0 - h }, { x0, 0, z0 }, { x1, 0, z0 }, { x1 + h, 0, z0 - h }, nada, lleno, lleno, nada);
    QuadColAz({ x0, 0, z1 }, { x0 - h, 0, z1 + h }, { x1 + h, 0, z1 + h }, { x1, 0, z1 }, lleno, nada, nada, lleno);
    QuadColAz({ x0 - h, 0, z0 - h }, { x0 - h, 0, z1 + h }, { x0, 0, z1 }, { x0, 0, z0 }, nada, nada, lleno, lleno);
    QuadColAz({ x1, 0, z0 }, { x1, 0, z1 }, { x1 + h, 0, z1 + h }, { x1 + h, 0, z0 - h }, lleno, lleno, nada, nada);

    // Cuerpo moteado: celdas con sombras lilas suaves.
    const float celda = 0.5f;
    int nx = (int)((x1 - x0) / celda + 0.5f);
    int nz = (int)((z1 - z0) / celda + 0.5f);
    float cx = (x1 - x0) / (float)nx, cz = (z1 - z0) / (float)nz;

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < nz; j++)
        {
            float a = AzarAz(i + indice * 31, j + 7);
            float b = AzarAz(i + 1 + indice * 31, j + 7);
            float d = AzarAz(i + 1 + indice * 31, j + 8);
            float e = AzarAz(i + indice * 31, j + 8);
            auto Mot = [&](float v)
            {
                return decoracion ? MezclaAz(AZUCAR_AZ, AZUCAR_SOMBRA_AZ, v * v * 0.55f) : AZUCAR_AZ;
            };
            float px0 = x0 + cx * (float)i, px1 = px0 + cx;
            float pz0 = z0 + cz * (float)j, pz1 = pz0 + cz;
            QuadVAz({ px0, 0, pz0 }, { px0, 0, pz1 }, { px1, 0, pz1 }, { px1, 0, pz0 }, Mot(a), Mot(e), Mot(d), Mot(b));
        }
    }

    // Lomo del colchon: claro hacia la luz, sombra lila del otro lado.
    const float l = 0.28f;
    Color claro = AlfaAz(WHITE, 0.85f), cero = AlfaAz(WHITE, 0.0f);
    Color sombra = AlfaAz(Color{ 176, 160, 220, 255 }, 0.55f), sinSombra = AlfaAz(Color{ 176, 160, 220, 255 }, 0.0f);
    QuadColAz({ x0, 0, z1 - l }, { x0, 0, z1 }, { x1, 0, z1 }, { x1, 0, z1 - l }, cero, claro, claro, cero);
    QuadColAz({ x0, 0, z0 }, { x0, 0, z0 + l }, { x1, 0, z0 + l }, { x1, 0, z0 }, sombra, sinSombra, sinSombra, sombra);
    QuadColAz({ x1 - l, 0, z0 }, { x1 - l, 0, z1 }, { x1, 0, z1 }, { x1, 0, z0 }, sinSombra, sinSombra, sombra, sombra);

    // Borde de cristales dorados sobre el limite exacto.
    Color cristal = MezclaAz(ACENTO_AZ, WHITE, 0.25f);
    const float paso = 0.34f, r = 0.055f;
    auto Cristal = [&](float px, float pz)
    {
        QuadAz({ px - r, 0, pz }, { px, 0, pz + r }, { px + r, 0, pz }, { px, 0, pz - r }, cristal);
    };

    for (float x = x0; x <= x1 + 0.001f; x += paso)
    {
        Cristal(x, z0);
        Cristal(x, z1);
    }

    for (float z = z0 + paso; z < z1 - 0.001f; z += paso)
    {
        Cristal(x0, z);
        Cristal(x1, z);
    }
}


// Charco de chocolate: chorreado alrededor, cuerpo con degradado, reborde
// claro justo en el radio logico y brillo de la tarde.
static void CharcoChocolateAz(const CirculoBolas& charco, int indice, bool decoracion)
{
    const int lados = 44;
    float r = charco.radio;
    Vector3 c = { charco.x, 0.0f, charco.z };
    Color chorreado = { 58, 30, 18, 255 };

    if (decoracion)
    {
        for (int k = 0; k < lados; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)lados;
            float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float l0 = r + 0.10f + 0.07f * std::sin(a0 * 5.0f + (float)indice * 2.0f) + 0.04f * std::sin(a0 * 11.0f);
            float l1 = r + 0.10f + 0.07f * std::sin(a1 * 5.0f + (float)indice * 2.0f) + 0.04f * std::sin(a1 * 11.0f);
            TriAz(c, { c.x + std::cos(a1) * l1, 0, c.z + std::sin(a1) * l1 }, { c.x + std::cos(a0) * l0, 0, c.z + std::sin(a0) * l0 }, chorreado);
        }
    }

    Color medio = { 108, 62, 40, 255 };
    AnilloAz(c, r * 0.62f, r, lados, medio, CHOCOLATE_AZ);
    DiscoAz(c, r * 0.62f, lados, medio);

    // Reborde fundido sobre el limite exacto.
    AnilloAz(c, r - 0.07f, r, lados, Color{ 132, 80, 52, 255 }, Color{ 96, 54, 34, 255 });

    // Brillo en media luna hacia el sol.
    Color brillo = AlfaAz(Color{ 214, 160, 124, 255 }, 0.65f);
    Color nada = AlfaAz(brillo, 0.0f);
    for (int k = 0; k < 10; k++)
    {
        float a0 = PI * 0.95f + 0.9f * (float)k / 10.0f;
        float a1 = PI * 0.95f + 0.9f * (float)(k + 1) / 10.0f;
        float ri = r * 0.68f, ro = r * 0.80f;
        Vector3 i0 = { c.x + std::cos(a0) * ri, 0, c.z + std::sin(a0) * ri };
        Vector3 i1 = { c.x + std::cos(a1) * ri, 0, c.z + std::sin(a1) * ri };
        Vector3 o0 = { c.x + std::cos(a0) * ro, 0, c.z + std::sin(a0) * ro };
        Vector3 o1 = { c.x + std::cos(a1) * ro, 0, c.z + std::sin(a1) * ro };
        float f0 = std::sin(PI * (float)k / 10.0f), f1 = std::sin(PI * (float)(k + 1) / 10.0f);
        QuadColAz(i0, i1, o1, o0, AlfaAz(brillo, 0.65f * f0), AlfaAz(brillo, 0.65f * f1), nada, nada);
    }
}


static void SueloAz(bool decoracion, bool sombras)
{
    float m = MITAD_ARENA_BOLAS;

    // Chispitas de colores sobre la nieve.
    if (decoracion)
    {
        const Color chispas[5] = { ROSA_AZ, MENTA_AZ, LILA_AZ, CARAMELO_AZ, Color{ 255, 240, 160, 255 } };

        for (int k = 0; k < 520; k++)
        {
            float x = -34.0f + 68.0f * AzarAz(k, 901);
            float z = -40.0f + 54.0f * AzarAz(k, 902);

            if (std::fabs(x) < m + 0.8f && std::fabs(z) < m + 0.8f) continue;
            if (std::fabs(x - RioXAz(z)) < RioAnchoAz(z) * 0.5f + 0.5f && z > Z_CABECERA_RIO_AZ - 1.0f) continue;

            float a = AzarAz(k, 903) * PI;
            float l = 0.09f, g = 0.03f;
            float dx = std::cos(a) * l, dz = std::sin(a) * l;
            float ex = -std::sin(a) * g, ez = std::cos(a) * g;
            QuadAz({ x - dx - ex, 0, z - dz - ez }, { x - dx + ex, 0, z - dz + ez }, { x + dx + ex, 0, z + dz + ez }, { x + dx - ex, 0, z + dz - ez }, chispas[k % 5]);
        }
    }

    // Rio de chocolate: orilla manchada, cauce y corriente central.
    {
        const float paso = 0.8f;
        Color mancha = { 172, 120, 96, 255 };

        for (float z = Z_CABECERA_RIO_AZ; z < Z_FIN_RIO_AZ; z += paso)
        {
            float za = z, zb = z + paso;
            float xa = RioXAz(za), xb = RioXAz(zb);
            float wa = RioAnchoAz(za) * 0.5f, wb = RioAnchoAz(zb) * 0.5f;
            float oa = wa + 0.45f, ob = wb + 0.45f;
            Color nieve = AlfaAz(mancha, 0.0f);

            QuadVAz({ xa - oa, 0, za }, { xb - ob, 0, zb }, { xb - wb, 0, zb }, { xa - wa, 0, za }, nieve, nieve, mancha, mancha);
            QuadVAz({ xa + wa, 0, za }, { xb + wb, 0, zb }, { xb + ob, 0, zb }, { xa + oa, 0, za }, mancha, mancha, nieve, nieve);
            QuadVAz({ xa - wa, 0, za }, { xb - wb, 0, zb }, { xb, 0, zb }, { xa, 0, za }, CHOCOLATE_AZ, CHOCOLATE_AZ, Color{ 112, 66, 42, 255 }, Color{ 112, 66, 42, 255 });
            QuadVAz({ xa, 0, za }, { xb, 0, zb }, { xb + wb, 0, zb }, { xa + wa, 0, za }, Color{ 112, 66, 42, 255 }, Color{ 112, 66, 42, 255 }, CHOCOLATE_AZ, CHOCOLATE_AZ);
        }

        // Remanso al pie de la cascada.
        float xc = RioXAz(Z_CABECERA_RIO_AZ);
        for (int k = 0; k < 20; k++)
        {
            float a0 = PI + PI * (float)k / 20.0f;
            float a1 = PI + PI * (float)(k + 1) / 20.0f;
            float r = RioAnchoAz(Z_CABECERA_RIO_AZ) * 0.5f;
            TriAz({ xc, 0, Z_CABECERA_RIO_AZ }, { xc + std::cos(a0) * r, 0, Z_CABECERA_RIO_AZ + std::sin(a0) * r * 0.6f }, { xc + std::cos(a1) * r, 0, Z_CABECERA_RIO_AZ + std::sin(a1) * r * 0.6f }, CHOCOLATE_AZ);
        }
    }

    // Losa de galletas cuadradas (juntas de miga entre una y otra).
    QuadAz({ -m - 0.4f, 0, -m - 0.4f }, { -m - 0.4f, 0, m + 0.4f }, { m + 0.4f, 0, m + 0.4f }, { m + 0.4f, 0, -m - 0.4f }, GALLETA_OSCURA_AZ);

    for (int ix = 0; ix < 9; ix++)
    {
        for (int iz = 0; iz < 9; iz++)
        {
            LosaGalletaAz(-m + 2.0f * (float)ix + 0.05f, -m + 2.0f * (float)iz + 0.05f, 1.9f, ix, iz, decoracion);
        }
    }

    // Migas y polvo de azucar que se escapa de los colchones.
    if (decoracion)
    {
        for (int k = 0; k < 360; k++)
        {
            float x = -m + 0.2f + (2.0f * m - 0.4f) * AzarAz(k, 701);
            float z = -m + 0.2f + (2.0f * m - 0.4f) * AzarAz(k, 702);

            if (EnAzucarBolas(x, z) || EnChocolateBolas(x, z)) continue;

            bool polvo = CercaDeAzucarAz(x, z, 0.9f);
            float r = polvo ? 0.05f + 0.06f * AzarAz(k, 703) : 0.035f + 0.03f * AzarAz(k, 703);
            Color c = polvo ? AlfaAz(AZUCAR_AZ, 0.75f) : TonoAz(GALLETA_AZ, 0.62f);

            if (!polvo && (k % 3) != 0) continue;

            QuadColAz({ x - r, 0, z }, { x, 0, z + r }, { x + r, 0, z }, { x, 0, z - r }, c, c, c, c);
        }
    }

    // Sombra de los muros (la luz viene de adelante y a la izquierda).
    if (sombras)
    {
        Color s = { 90, 50, 40, 110 };
        Color n = { 90, 50, 40, 0 };
        QuadColAz({ -m, 0, -m }, { -m, 0, m }, { -m + 0.7f, 0, m }, { -m + 0.7f, 0, -m }, s, s, n, n);
        QuadColAz({ -m, 0, -m }, { -m, 0, -m + 0.5f }, { m, 0, -m + 0.5f }, { m, 0, -m }, s, n, n, s);
        QuadColAz({ m + 0.4f, 0, -m - 0.4f }, { m + 0.4f, 0, m + 0.4f }, { m + 1.1f, 0, m + 0.4f }, { m + 1.1f, 0, -m - 0.4f }, s, s, n, n);
        QuadColAz({ -m - 0.4f, 0, -m - 1.6f }, { -m - 0.4f, 0, -m - 0.4f }, { m + 0.4f, 0, -m - 0.4f }, { m + 0.4f, 0, -m - 1.6f }, n, s, s, n);
    }

    // Colchones de azucar glas: la zona exacta donde la bola crece.
    for (int i = 0; i < 4; i++)
    {
        ColchonAzucarAz(ZONAS_AZUCAR_BOLAS[i], i, decoracion);
    }

    // Charcos de chocolate: el radio exacto que derrite.
    for (int i = 0; i < 3; i++)
    {
        CharcoChocolateAz(CHARCOS_BOLAS[i], i, decoracion);
    }

    // Sombras de las gominolas y de la utileria.
    if (sombras)
    {
        for (const CirculoBolas& g : GOMINOLAS_BOLAS)
        {
            SombraAz(g.x + 0.25f, g.z - 0.3f, g.radio * 1.25f, g.radio * 1.15f, 0.42f);
        }

        if (decoracion)
        {
            int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

            for (int i = 0; i < CANTIDAD_PROPS_AZ; i++)
            {
                if (!PropVisibleAz(i, paso)) continue;
                float radio, alto;
                MedidaPropAz(PROPS_AZ[i], radio, alto);
                float largo = 0.12f * alto;
                SombraAz(PROPS_AZ[i].x + 0.3f * largo, PROPS_AZ[i].z - 0.45f * largo, radio * 1.2f + largo * 0.3f, radio * 1.1f + largo * 0.3f, 0.32f);
            }

            for (const Vector3& f : FAROLES_AZ)
            {
                SombraAz(f.x + 0.3f, f.z - 0.4f, 0.6f, 0.55f, 0.35f);
            }
        }
    }
}


//--------------------------------------------------
// Capa 1b: muros de barquillo y postes de baston
//--------------------------------------------------

static const Color RELLENO_AZ = { 255, 214, 230, 255 };
static const Color GLASEADO_AZ = { 255, 252, 250, 255 };


// Muro de barquillo relleno: capas de oblea y crema, glaseado arriba y
// gotas de glaseado que chorrean por la cara que mira a la camara.
// centro/tamano en el plano; ejeX: el muro corre a lo largo de X.
static void MuroBarquilloAz(Vector3 c, float largo, float grosor, float alto, bool ejeX, int caraGotas, bool decoracion)
{
    const float oblea = 0.2f, crema = 0.07f;
    float y = 0.0f;
    int capa = 0;

    while (y < alto - 0.12f)
    {
        bool esCrema = (capa % 2) == 1;
        float h = esCrema ? crema : oblea;
        if (y + h > alto - 0.12f) h = alto - 0.12f - y;
        Color color = esCrema ? RELLENO_AZ : TonoAz(BARQUILLO_AZ, 0.96f + 0.06f * AzarAz(capa, (int)(c.x * 7.0f + c.z * 3.0f)));
        Vector3 t = ejeX ? Vector3{ largo, h, grosor * (esCrema ? 0.96f : 1.0f) } : Vector3{ grosor * (esCrema ? 0.96f : 1.0f), h, largo };
        CajaAz({ c.x, y + h * 0.5f, c.z }, t, color);
        y += h;
        capa++;
    }

    // Glaseado de arriba, algo mas ancho que el muro.
    Vector3 tg = ejeX ? Vector3{ largo + 0.06f, 0.12f, grosor + 0.12f } : Vector3{ grosor + 0.12f, 0.12f, largo + 0.06f };
    CajaAz({ c.x, alto - 0.06f, c.z }, tg, GLASEADO_AZ);

    if (!decoracion || caraGotas == 0)
    {
        return;
    }

    // Gotas: caraGotas = +1 / -1 indica la cara (+Z/-Z o +X/-X).
    float cara = (float)caraGotas * (grosor * 0.5f + 0.065f);
    int n = (int)(largo / 0.7f);
    int semilla = (int)(c.x * 5.0f + c.z * 11.0f);

    for (int k = 0; k < n; k++)
    {
        if (AzarAz(k, semilla + 30) < 0.35f) continue;
        float s = -largo * 0.5f + 0.35f + 0.7f * (float)k + 0.3f * (AzarAz(k, semilla + 31) - 0.5f);
        float l = 0.05f + 0.16f * AzarAz(k, semilla + 32) * AzarAz(k, semilla + 34);
        float w = 0.06f + 0.05f * AzarAz(k, semilla + 33);
        float y0 = alto - 0.1f, y1 = alto - 0.1f - l;
        Vector3 a, b, d, e, p;

        if (ejeX)
        {
            float z = c.z + cara;
            a = { c.x + s - w, y0, z }; b = { c.x + s + w, y0, z };
            d = { c.x + s + w, y1, z }; e = { c.x + s - w, y1, z };
            p = { c.x + s, y1 - w * 0.9f, z };
        }
        else
        {
            float x = c.x + cara;
            a = { x, y0, c.z + s - w }; b = { x, y0, c.z + s + w };
            d = { x, y1, c.z + s + w }; e = { x, y1, c.z + s - w };
            p = { x, y1 - w * 0.9f, c.z + s };
        }

        Color g = TonoAz(GLASEADO_AZ, 0.94f);
        QuadAz(a, b, d, e, g);
        TriAz(e, d, p, g);
    }
}


// Poste de baston de caramelo a rayas con bola de chicle arriba.
static void PosteBastonAz(float x, float z, float alto, Color bola)
{
    const int tramos = 10;
    float h = alto / (float)tramos;

    for (int k = 0; k < tramos; k++)
    {
        Color c = (k % 2 == 0) ? Color{ 255, 120, 172, 255 } : Color{ 255, 248, 250, 255 };
        CilindroAz({ x, h * (float)k, z }, { x, h * (float)(k + 1), z }, 0.4f, 0.4f, 12, c, k == tramos - 1);
    }

    CilindroAz({ x, alto, z }, { x, alto + 0.1f, z }, 0.46f, 0.46f, 12, GLASEADO_AZ);
    ElipsoideAz({ x, alto + 0.38f, z }, { 0.34f, 0.34f, 0.34f }, 12, 6, bola);
}


static void MurosAz(bool decoracion)
{
    float m = MITAD_ARENA_BOLAS;
    float g = 0.4f;

    // Atras y a los costados, altos; adelante, bajo para ver la ultima fila.
    MuroBarquilloAz({ 0.0f, 0.0f, -m - g * 0.5f }, 2.0f * m + 2.0f * g, g, 1.3f, true, 1, decoracion);
    MuroBarquilloAz({ -m - g * 0.5f, 0.0f, 0.0f }, 2.0f * m, g, 1.05f, false, 1, decoracion);
    MuroBarquilloAz({ m + g * 0.5f, 0.0f, 0.0f }, 2.0f * m, g, 1.05f, false, -1, decoracion);
    MuroBarquilloAz({ 0.0f, 0.0f, m + g * 0.5f }, 2.0f * m + 2.0f * g, g, 0.55f, true, 1, decoracion);

    PosteBastonAz(-m - 0.2f, -m - 0.2f, 1.8f, LILA_AZ);
    PosteBastonAz(m + 0.2f, -m - 0.2f, 1.8f, MENTA_AZ);
    PosteBastonAz(-m - 0.2f, m + 0.2f, 0.9f, MENTA_AZ);
    PosteBastonAz(m + 0.2f, m + 0.2f, 0.9f, LILA_AZ);
}


static const Color COLORES_DULCE_AZ[5] =
{
    { 255, 132, 186, 255 },     // rosa
    { 120, 218, 196, 255 },     // menta
    { 182, 140, 240, 255 },     // lila
    { 255, 164, 84, 255 },      // naranja
    { 120, 196, 250, 255 }      // celeste
};


// Piruleta: disco en espiral sobre un palito, mirando hacia giro.
static void PiruletaAz(const PropAz& p)
{
    float e = p.escala;
    float alto = 2.2f * e;
    float R = 0.78f * e;
    float grosor = 0.2f * e;
    Color color = COLORES_DULCE_AZ[p.variante % 5];
    Color blanco = { 255, 250, 246, 255 };
    Vector3 base = { p.x, 0.0f, p.z };

    CilindroAz(base, { p.x, alto + 0.2f * e, p.z }, 0.07f * e, 0.07f * e, 6, Color{ 250, 244, 236, 255 });

    // Normal del disco y ejes de su plano.
    Vector3 n = { std::sin(p.giro), 0.0f, std::cos(p.giro) };
    Vector3 u = { std::cos(p.giro), 0.0f, -std::sin(p.giro) };
    Vector3 v = { 0.0f, 1.0f, 0.0f };
    Vector3 c = { p.x, alto + R, p.z };
    const int lados = 24, anillos = 5;

    for (int cara = -1; cara <= 1; cara += 2)
    {
        Vector3 off = Vector3Scale(n, grosor * 0.5f * (float)cara);
        Color luz = TonoAz(WHITE, LuzAz(Vector3Scale(n, (float)cara)));

        for (int j = 0; j < anillos; j++)
        {
            float r0 = R * (float)j / (float)anillos, r1 = R * (float)(j + 1) / (float)anillos;

            for (int k = 0; k < lados; k++)
            {
                float a0 = 2.0f * PI * (float)k / (float)lados, a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
                int banda = (int)((float)k / (float)lados * 6.0f + (float)j * 1.0f) % 2;
                Color base = banda == 0 ? color : blanco;
                Color cc = Color{ (unsigned char)(base.r * luz.r / 255), (unsigned char)(base.g * luz.g / 255), (unsigned char)(base.b * luz.b / 255), 255 };
                auto P = [&](float r, float a)
                {
                    return Vector3Add(Vector3Add(c, off), Vector3Add(Vector3Scale(u, std::cos(a) * r), Vector3Scale(v, std::sin(a) * r)));
                };
                QuadAz(P(r0, a0), P(r0, a1), P(r1, a1), P(r1, a0), cc);
            }
        }
    }

    // Canto del disco.
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados, a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 d0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 d1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 dm = Vector3Normalize(Vector3Add(d0, d1));
        Vector3 f = Vector3Scale(n, grosor * 0.5f);
        Vector3 p0 = Vector3Add(c, Vector3Scale(d0, R)), p1 = Vector3Add(c, Vector3Scale(d1, R));
        QuadAz(Vector3Subtract(p0, f), Vector3Subtract(p1, f), Vector3Add(p1, f), Vector3Add(p0, f), TonoAz(MezclaAz(color, blanco, 0.3f), LuzAz(dm)));
    }

    // Monito de cinta al pie del disco.
    Vector3 nudo = { p.x, alto + 0.05f * e, p.z };
    Color cinta = COLORES_DULCE_AZ[(p.variante + 2) % 5];
    TriAz(nudo, Vector3Add(nudo, Vector3Add(Vector3Scale(u, -0.32f * e), Vector3{ 0, 0.16f * e, 0 })), Vector3Add(nudo, Vector3Add(Vector3Scale(u, -0.32f * e), Vector3{ 0, -0.12f * e, 0 })), cinta);
    TriAz(nudo, Vector3Add(nudo, Vector3Add(Vector3Scale(u, 0.32f * e), Vector3{ 0, -0.12f * e, 0 })), Vector3Add(nudo, Vector3Add(Vector3Scale(u, 0.32f * e), Vector3{ 0, 0.16f * e, 0 })), TonoAz(cinta, 0.85f));
}


// Baston de caramelo clavado en la nieve, con su gancho.
static void BastonAz(const PropAz& p)
{
    float e = p.escala;
    float alto = 1.9f * e;
    float r = 0.1f * e;
    Color rayas[2] = { COLORES_DULCE_AZ[p.variante % 2 == 0 ? 0 : 1], Color{ 255, 250, 248, 255 } };
    const int tramos = 12;

    for (int k = 0; k < tramos; k++)
    {
        float y0 = alto * (float)k / (float)tramos, y1 = alto * (float)(k + 1) / (float)tramos;
        CilindroAz({ p.x, y0, p.z }, { p.x, y1, p.z }, r, r, 8, rayas[k % 2], false);
    }

    float R = 0.34f * e;
    Vector3 d = { std::cos(p.giro), 0.0f, std::sin(p.giro) };
    Vector3 centro = { p.x + d.x * R, alto, p.z + d.z * R };
    const int arcos = 8;

    for (int k = 0; k < arcos; k++)
    {
        float a0 = PI * (float)k / (float)arcos, a1 = PI * (float)(k + 1) / (float)arcos;
        Vector3 q0 = { centro.x - d.x * R * std::cos(a0), alto + R * std::sin(a0), centro.z - d.z * R * std::cos(a0) };
        Vector3 q1 = { centro.x - d.x * R * std::cos(a1), alto + R * std::sin(a1), centro.z - d.z * R * std::cos(a1) };
        CilindroAz(q0, q1, r, r, 8, rayas[k % 2], k == arcos - 1);
    }

    // Montoncito de nieve al pie.
    ElipsoideAz({ p.x, 0.0f, p.z }, { 0.32f * e, 0.14f * e, 0.32f * e }, 10, 3, NIEVE_AZ, 0.0f);
}


// Merengue: cono estrellado que gira como una manga pastelera.
static void MerengueAz(const PropAz& p)
{
    float e = p.escala;
    float R = 0.9f * e, H = 1.3f * e;
    const Color tonos[3] = { { 255, 250, 244, 255 }, { 255, 222, 236, 255 }, { 226, 246, 238, 255 } };
    Color color = tonos[p.variante % 3];
    const int lados = 20, pisos = 9;

    auto P = [&](int j, int k)
    {
        float u = (float)j / (float)pisos;
        float a = 2.0f * PI * (float)k / (float)lados + u * 2.2f + p.giro;
        float r = R * std::pow(1.0f - u, 0.85f) * (1.0f + 0.13f * std::cos(a * 8.0f - u * 2.2f * 8.0f));
        float y = H * u;
        return Vector3{ p.x + std::cos(a) * r, y, p.z + std::sin(a) * r };
    };

    for (int j = 0; j < pisos; j++)
    {
        for (int k = 0; k < lados; k++)
        {
            Vector3 a = P(j, k), b = P(j, k + 1), c = P(j + 1, k + 1), d = P(j + 1, k);
            Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(d, a), Vector3Subtract(b, a)));
            if (n.y < 0.0f) n = Vector3Negate(n);
            QuadAz(a, b, c, d, TonoAz(color, LuzAz(n) * 1.04f));
        }
    }
}


// Malvaviscos: cilindros pastel, uno arriba de otro y uno tumbado.
static void MalvaviscosAz(const PropAz& p)
{
    float e = p.escala;
    float r = 0.32f * e, h = 0.42f * e;
    const Color tonos[4] = { { 255, 250, 248, 255 }, { 255, 196, 222, 255 }, { 204, 244, 228, 255 }, { 222, 206, 252, 255 } };
    float ca = std::cos(p.giro), sa = std::sin(p.giro);
    auto Rot = [&](float x, float z) { return Vector3{ p.x + x * ca - z * sa, 0.0f, p.z + x * sa + z * ca }; };

    Vector3 a = Rot(-0.3f * e, 0.0f), b = Rot(0.36f * e, 0.12f * e), c = Rot(0.0f, -0.45f * e);
    CilindroAz(a, { a.x, h, a.z }, r, r, 12, tonos[(p.variante) % 4]);
    CilindroAz({ a.x, h, a.z }, { a.x, 2.0f * h, a.z }, r * 0.97f, r * 0.97f, 12, tonos[(p.variante + 1) % 4]);
    CilindroAz(b, { b.x, h, b.z }, r, r, 12, tonos[(p.variante + 2) % 4]);
    Vector3 d = Vector3Scale(Vector3{ ca, 0, sa }, h * 0.5f);
    CilindroAz({ c.x - d.x, r, c.z - d.z }, { c.x + d.x, r, c.z + d.z }, r, r, 12, tonos[(p.variante + 3) % 4]);
}


// Caramelo envuelto tumbado: cuerpo a rayas y moños del papel.
static void CarameloAz(const PropAz& p)
{
    float e = p.escala;
    float r = 0.3f * e, l = 0.42f * e;
    Vector3 d = { std::cos(p.giro), 0.0f, std::sin(p.giro) };
    Vector3 c = { p.x, r, p.z };
    Color color = COLORES_DULCE_AZ[(p.variante + 3) % 5];
    Color blanco = { 255, 250, 246, 255 };
    const int tramos = 5;

    for (int k = 0; k < tramos; k++)
    {
        float s0 = -l + 2.0f * l * (float)k / (float)tramos, s1 = -l + 2.0f * l * (float)(k + 1) / (float)tramos;
        float f0 = std::sqrt(Lim01Az(1.0f - (s0 / (l * 1.15f)) * (s0 / (l * 1.15f))));
        float f1 = std::sqrt(Lim01Az(1.0f - (s1 / (l * 1.15f)) * (s1 / (l * 1.15f))));
        CilindroAz(Vector3Add(c, Vector3Scale(d, s0)), Vector3Add(c, Vector3Scale(d, s1)), r * (0.55f + 0.45f * f0), r * (0.55f + 0.45f * f1), 12, k % 2 == 0 ? color : blanco, false);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 cuello = Vector3Add(c, Vector3Scale(d, l * (float)lado));
        Vector3 punta = Vector3Add(c, Vector3Scale(d, (l + 0.32f * e) * (float)lado));
        CilindroAz(cuello, Vector3Add(cuello, Vector3Scale(d, 0.08f * e * (float)lado)), r * 0.55f, r * 0.18f, 10, TonoAz(color, 0.9f), false);
        CilindroAz(Vector3Add(cuello, Vector3Scale(d, 0.08f * e * (float)lado)), punta, r * 0.18f, r * 0.62f, 10, MezclaAz(color, blanco, 0.35f), true);
    }
}


// Arbol de algodon de azucar: palito de barquillo y copa esponjosa.
static void ArbolAz(const PropAz& p)
{
    float e = p.escala;
    float alto = 2.0f * e;
    const Color copas[4] = { { 255, 176, 214, 255 }, { 196, 176, 250, 255 }, { 170, 226, 246, 255 }, { 255, 206, 176, 255 } };
    Color copa = copas[p.variante % 4];

    CilindroAz({ p.x, 0.0f, p.z }, { p.x, alto, p.z }, 0.2f * e, 0.12f * e, 8, BARQUILLO_AZ);

    const int bolas = 6;
    for (int k = 0; k < bolas; k++)
    {
        float a = 2.0f * PI * (float)k / (float)bolas + p.x;
        float dist = (k == 0) ? 0.0f : 0.55f * e;
        float r = (k == 0 ? 0.95f : 0.62f + 0.18f * AzarAz(k, (int)(p.x * 10.0f))) * e;
        float y = alto + 0.6f * e + (k == 0 ? 0.35f * e : 0.1f * e * std::sin(a * 2.0f));
        Color c = MezclaAz(copa, WHITE, 0.12f * AzarAz(k, (int)(p.z * 10.0f)));
        ElipsoideAz({ p.x + std::cos(a) * dist, y, p.z + std::sin(a) * dist }, { r, r * 0.85f, r }, 10, 6, c);
    }
}


// Columna de caramelo retorcida con capitel de galleta y nieve encima.
static void ColumnaAz(const PropAz& p)
{
    float e = p.escala;
    float alto = 7.0f * e;
    Color ambar = { 234, 138, 36, 255 };
    Color claro = { 252, 198, 112, 255 };

    CajaAz({ p.x, 0.22f * e, p.z }, { 1.7f * e, 0.44f * e, 1.7f * e }, GALLETA_AZ);
    CajaAz({ p.x, 0.58f * e, p.z }, { 1.4f * e, 0.28f * e, 1.4f * e }, TonoAz(GALLETA_AZ, 1.06f));

    const int tramos = 18;
    float y0 = 0.72f * e, y1 = alto - 0.7f * e;
    float h = (y1 - y0) / (float)tramos;

    for (int k = 0; k < tramos; k++)
    {
        CajaGiradaAz({ p.x, y0 + h * ((float)k + 0.5f), p.z }, { 0.95f * e, h * 1.02f, 0.95f * e }, p.giro + 0.3f * (float)k, (k % 3 == 0) ? claro : ambar);
    }

    CajaAz({ p.x, y1 + 0.16f * e, p.z }, { 1.45f * e, 0.32f * e, 1.45f * e }, TonoAz(GALLETA_AZ, 1.04f));
    CajaAz({ p.x, y1 + 0.46f * e, p.z }, { 1.8f * e, 0.28f * e, 1.8f * e }, GALLETA_AZ);
    ElipsoideAz({ p.x, y1 + 0.6f * e, p.z }, { 0.98f * e, 0.42f * e, 0.98f * e }, 12, 4, NIEVE_AZ, 0.0f);
    ElipsoideAz({ p.x, y1 + 1.12f * e, p.z }, { 0.26f * e, 0.26f * e, 0.26f * e }, 10, 5, Color{ 224, 40, 76, 255 });
}


// Galleta gigante de chips clavada en la nieve, algo inclinada.
static void GalletaGiganteAz(const PropAz& p)
{
    float e = p.escala;
    float R = 1.0f * e;
    Vector3 n = Vector3Normalize(Vector3{ std::sin(p.giro), 0.25f, std::cos(p.giro) });
    Vector3 c = { p.x, R * 0.78f, p.z };
    Vector3 f = Vector3Scale(n, 0.14f * e);

    CilindroAz(Vector3Subtract(c, f), Vector3Add(c, f), R, R, 18, Color{ 218, 160, 92, 255 });

    // Chips sobre la cara que mira a la camara.
    Vector3 u = Vector3Normalize(Vector3CrossProduct(n, Vector3{ 0, 1, 0 }));
    Vector3 v = Vector3CrossProduct(u, n);
    for (int k = 0; k < 9; k++)
    {
        float a = 2.0f * PI * AzarAz(k, (int)(p.x * 9.0f));
        float d = R * (0.2f + 0.65f * AzarAz(k, (int)(p.z * 9.0f)));
        Vector3 q = Vector3Add(Vector3Add(c, Vector3Scale(n, 0.15f * e)), Vector3Add(Vector3Scale(u, std::cos(a) * d), Vector3Scale(v, std::sin(a) * d)));
        float s = 0.09f * e;
        QuadAz(Vector3Subtract(q, Vector3Scale(u, s)), Vector3Subtract(q, Vector3Scale(v, s)), Vector3Add(q, Vector3Scale(u, s)), Vector3Add(q, Vector3Scale(v, s)), CHOCOLATE_AZ);
    }

    ElipsoideAz({ p.x, R * 1.68f, p.z }, { 0.45f * e, 0.12f * e, 0.2f * e }, 10, 3, NIEVE_AZ, 0.0f);
    ElipsoideAz({ p.x, 0.0f, p.z }, { 0.9f * e, 0.22f * e, 0.55f * e }, 12, 3, NIEVE_AZ, 0.0f);
}


// Matita de gomitas: varias gotas de gelatina azucaradas.
static void GomitasAz(const PropAz& p)
{
    float e = p.escala;
    for (int k = 0; k < 5; k++)
    {
        float a = 2.0f * PI * (float)k / 5.0f + p.x;
        float d = (k == 0) ? 0.0f : 0.5f * e;
        float r = (k == 0 ? 0.45f : 0.3f + 0.08f * AzarAz(k, 77)) * e;
        Color c = COLORES_DULCE_AZ[(k + p.variante) % 5];
        ElipsoideAz({ p.x + std::cos(a) * d, 0.0f, p.z + std::sin(a) * d }, { r, r * 1.25f, r }, 10, 4, c, 0.0f);
    }
}


// Farol de chicle: baston retorcido, jaula de caramelo y bola luminosa.
static void FarolAz(Vector3 f)
{
    CajaAz({ f.x, 0.15f, f.z }, { 0.7f, 0.3f, 0.7f }, GALLETA_AZ);

    const int tramos = 12;
    float alto = 3.0f;
    for (int k = 0; k < tramos; k++)
    {
        float y0 = 0.3f + (alto - 0.3f) * (float)k / (float)tramos;
        float y1 = 0.3f + (alto - 0.3f) * (float)(k + 1) / (float)tramos;
        CilindroAz({ f.x, y0, f.z }, { f.x, y1, f.z }, 0.09f, 0.09f, 8, (k % 2 == 0) ? Color{ 255, 250, 248, 255 } : COLORES_DULCE_AZ[2], false);
    }

    CilindroAz({ f.x, alto, f.z }, { f.x, alto + 0.08f, f.z }, 0.36f, 0.36f, 10, CARAMELO_AZ);

    for (int k = 0; k < 4; k++)
    {
        float a = PI * 0.25f + PI * 0.5f * (float)k;
        Vector3 b = { f.x + std::cos(a) * 0.34f, alto + 0.08f, f.z + std::sin(a) * 0.34f };
        CilindroAz(b, { b.x, alto + 0.78f, b.z }, 0.035f, 0.035f, 5, CARAMELO_AZ, false);
    }

    // Globo luminoso (casi sin bruma) y sombrerito.
    float emis = g_emisAz;
    g_emisAz = 1.0f;
    ElipsoideAz({ f.x, alto + 0.44f, f.z }, { 0.3f, 0.32f, 0.3f }, 12, 6, LUZ_FAROL_AZ);
    g_emisAz = emis;
    CilindroAz({ f.x, alto + 0.78f, f.z }, { f.x, alto + 1.1f, f.z }, 0.46f, 0.05f, 10, COLORES_DULCE_AZ[0]);
    ElipsoideAz({ f.x, alto + 1.14f, f.z }, { 0.09f, 0.09f, 0.09f }, 8, 4, GLASEADO_AZ);
}


// Monticulos de nieve de azucar a lo largo de las orillas del rio.
static void OrillasRioAz()
{
    int k = 0;
    for (float z = Z_CABECERA_RIO_AZ + 0.6f; z < Z_FIN_RIO_AZ; z += 0.9f, k++)
    {
        float x = RioXAz(z);
        float w = RioAnchoAz(z) * 0.5f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            if (AzarAz(k, lado + 3) < 0.45f) continue;
            float r = 0.2f + 0.26f * AzarAz(k, lado + 5);
            float px = x + (float)lado * (w + 0.12f + r * 0.5f);
            float pz = z + 0.5f * (AzarAz(k, lado + 9) - 0.5f);
            ElipsoideAz({ px, 0.0f, pz }, { r, 0.1f + 0.1f * AzarAz(k, lado + 13), r * (1.0f + 0.8f * AzarAz(k, lado + 15)) }, 9, 3, NIEVE_AZ, 0.0f);

            // Piedritas de galleta en la orilla.
            if (AzarAz(k, lado + 17) > 0.62f)
            {
                CajaGiradaAz({ px + (float)lado * 0.15f, 0.08f, pz + 0.5f }, { 0.34f, 0.16f, 0.26f }, AzarAz(k, lado) * 3.0f, GALLETA_AZ);
            }
        }
    }
}


// Puente de obleas sobre el rio.
static void PuenteAz(float z)
{
    float x = RioXAz(z);
    float w = RioAnchoAz(z) * 0.5f + 0.9f;
    const int tablas = 9;

    for (int k = 0; k < tablas; k++)
    {
        float s0 = -w + 2.0f * w * (float)k / (float)tablas;
        float s1 = -w + 2.0f * w * (float)(k + 1) / (float)tablas;
        float sm = 0.5f * (s0 + s1);
        float y = 0.28f + 0.42f * (1.0f - (sm / w) * (sm / w));
        CajaAz({ x + sm, y, z }, { (s1 - s0) * 0.92f, 0.14f, 1.3f }, TonoAz(BARQUILLO_AZ, 0.94f + 0.1f * AzarAz(k, 3)));
    }

    // Barandas de baston.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float zz = z + (float)lado * 0.6f;
        for (int k = 0; k <= 4; k++)
        {
            float s = -w + 2.0f * w * (float)k / 4.0f;
            float y = 0.28f + 0.42f * (1.0f - (s / w) * (s / w));
            CilindroAz({ x + s, 0.0f, zz }, { x + s, y + 0.55f, zz }, 0.06f, 0.06f, 6, k % 2 == 0 ? COLORES_DULCE_AZ[0] : GLASEADO_AZ);
        }

        for (int k = 0; k < 8; k++)
        {
            float s0 = -w + 2.0f * w * (float)k / 8.0f, s1 = -w + 2.0f * w * (float)(k + 1) / 8.0f;
            float ya = 0.28f + 0.42f * (1.0f - (s0 / w) * (s0 / w)) + 0.55f;
            float yb = 0.28f + 0.42f * (1.0f - (s1 / w) * (s1 / w)) + 0.55f;
            CilindroAz({ x + s0, ya, zz }, { x + s1, yb, zz }, 0.05f, 0.05f, 6, k % 2 == 0 ? GLASEADO_AZ : COLORES_DULCE_AZ[0], false);
        }
    }
}


// Pena de galleta sandwich (capas de galleta de chocolate y crema), de
// donde cae la cascada de chocolate.
static void PenaCascadaAz()
{
    float xc = RioXAz(Z_CABECERA_RIO_AZ);
    float zf = Z_CABECERA_RIO_AZ - 0.7f;
    const float capas[7] = { 1.1f, 0.4f, 1.0f, 0.4f, 1.1f, 0.4f, 0.9f };
    float y = 0.0f;

    for (int k = 0; k < 7; k++)
    {
        bool crema = (k % 2) == 1;
        float h = capas[k];
        float entra = 0.25f * (float)k;
        Color c = crema ? RELLENO_AZ : TonoAz(GALLETA_AZ, 0.92f + 0.12f * AzarAz(k, 1));
        float x0 = xc - 7.5f + entra * 0.6f + 0.5f * AzarAz(k, 2), x1 = xc + 6.0f - entra + 0.5f * AzarAz(k, 3);
        float z1 = zf - entra * 0.35f;
        CajaAz({ (x0 + x1) * 0.5f, y + h * 0.5f, z1 - 4.5f }, { x1 - x0, h, 9.0f }, c);
        y += h;
    }

    // Nieve encima y chorreado de chocolate en el frente.
    ElipsoideAz({ xc - 1.0f, y, zf - 5.6f }, { 6.6f, 0.7f, 4.2f }, 16, 3, NIEVE_AZ, 0.0f);
    ElipsoideAz({ xc - 4.5f, y + 0.4f, zf - 6.6f }, { 2.0f, 1.4f, 2.0f }, 10, 4, NIEVE_AZ, 0.0f);
}


static void UtileriaAz(bool decoracion)
{
    MurosAz(decoracion);

    if (!decoracion)
    {
        return;
    }

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    for (const Vector3& f : FAROLES_AZ)
    {
        FarolAz(f);
    }

    for (int i = 0; i < CANTIDAD_PROPS_AZ; i++)
    {
        if (!PropVisibleAz(i, paso)) continue;
        const PropAz& p = PROPS_AZ[i];

        switch (p.tipo)
        {
            case PROP_AZ_PIRULETA: PiruletaAz(p); break;
            case PROP_AZ_BASTON: BastonAz(p); break;
            case PROP_AZ_MERENGUE: MerengueAz(p); break;
            case PROP_AZ_MALVAVISCOS: MalvaviscosAz(p); break;
            case PROP_AZ_CARAMELO: CarameloAz(p); break;
            case PROP_AZ_ARBOL: ArbolAz(p); break;
            case PROP_AZ_COLUMNA: ColumnaAz(p); break;
            case PROP_AZ_GALLETA: GalletaGiganteAz(p); break;
            default: GomitasAz(p); break;
        }
    }

    OrillasRioAz();
    PuenteAz(-6.4f);
    PenaCascadaAz();
}


//--------------------------------------------------
// Capas 3-5: base, lejania y cielo
//--------------------------------------------------

static const float Z_BORDE_MESETA_AZ = -44.0f;
static const float Y_MAR_NUBES_AZ = -14.0f;


// Meseta nevada de azucar glas, con sombras lilas y tibias. Es opaca y
// escribe profundidad: el suelo pintado (arena, rio, chispitas) va encima.
static void BaseAz()
{
    const float y = -0.03f;

    auto Nieve = [&](int i, int j)
    {
        float x = (float)i * 2.0f, z = (float)j * 2.0f;
        float ondas = 0.5f + 0.28f * std::sin(0.23f * x + 0.11f * z) + 0.22f * std::sin(0.09f * x - 0.21f * z + 1.3f);
        float v = 0.75f * ondas + 0.25f * AzarAz(i, j);
        float rosa = 0.5f + 0.5f * std::sin(x * 0.05f + z * 0.08f + 0.7f);
        Color c = MezclaAz(NIEVE_AZ, AZUCAR_SOMBRA_AZ, v * 0.42f);
        return MezclaAz(c, Color{ 255, 222, 234, 255 }, rosa * 0.3f);
    };

    for (int ix = -36; ix < 36; ix++)
    {
        for (int iz = -22; iz < 12; iz++)
        {
            float x0 = (float)ix * 2.0f, x1 = x0 + 2.0f;
            float z0 = (float)iz * 2.0f, z1 = z0 + 2.0f;
            QuadVAz({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, Nieve(ix, iz), Nieve(ix, iz + 1), Nieve(ix + 1, iz + 1), Nieve(ix + 1, iz));
        }
    }
}


// Montana lejana: cono con perfil irregular y nieve de azucar en la cima.
static void MontanaAz(float cx, float cz, float baseY, float R, float picoY, Color roca, int semilla)
{
    const int lados = 12;
    const float alturas[4] = { 0.0f, 0.42f, 0.74f, 1.0f };
    const float radios[4] = { 1.0f, 0.62f, 0.3f, 0.0f };
    Vector3 p[4][lados + 1];

    for (int j = 0; j < 4; j++)
    {
        for (int k = 0; k <= lados; k++)
        {
            int kk = k % lados;
            float a = 2.0f * PI * (float)kk / (float)lados + (float)semilla;
            float bulto = 1.0f + 0.22f * (AzarAz(kk + j * 13, semilla) - 0.5f) * 2.0f;
            float r = R * radios[j] * bulto;
            float desvio = (j == 3) ? 0.0f : 0.0f;
            p[j][k] = { cx + std::cos(a) * r + desvio, baseY + (picoY - baseY) * alturas[j] * (j == 3 ? 1.0f : (0.92f + 0.12f * AzarAz(kk, semilla + j))), cz + std::sin(a) * r };
        }
    }

    for (int j = 0; j < 3; j++)
    {
        for (int k = 0; k < lados; k++)
        {
            Vector3 a = p[j][k], b = p[j][k + 1], c = p[j + 1][k + 1], d = p[j + 1][k];
            Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(d, a)));
            if (n.y < 0.0f) n = Vector3Negate(n);
            float f = LuzAz(n);
            Color abajo = j >= 2 ? NIEVE_AZ : (j == 1 ? MezclaAz(roca, NIEVE_AZ, 0.15f) : roca);
            Color arriba = j >= 1 ? NIEVE_AZ : MezclaAz(roca, NIEVE_AZ, 0.15f);
            QuadVAz(a, b, c, d, TonoAz(abajo, f), TonoAz(abajo, f), TonoAz(arriba, f), TonoAz(arriba, f));
        }
    }
}


// Colina de nata: cupula cremosa con nieve de azucar arriba, chocolate que
// chorrea desde la cima y, con decoracion, una cereza.
static void ColinaNataAz(float cx, float cz, float baseY, float R, float alto, int semilla, bool decoracion)
{
    const int lados = 22, pisos = 8;
    Color pie = { 255, 212, 222, 255 };

    auto P = [&](int k, int j)
    {
        float a = 2.0f * PI * (float)(k % lados) / (float)lados;
        float u = (float)j / (float)pisos;
        float ph = u * PI * 0.5f;
        float bulto = 1.0f + 0.06f * std::sin(a * 3.0f + (float)semilla) + 0.04f * std::sin(a * 7.0f);
        return Vector3{ cx + std::cos(a) * std::cos(ph) * R * bulto, baseY + std::sin(ph) * (alto - baseY), cz + std::sin(a) * std::cos(ph) * R * 0.75f * bulto };
    };

    auto C = [&](int k, int j)
    {
        float a = 2.0f * PI * (float)(k % lados) / (float)lados;
        float u = (float)j / (float)pisos;
        float ph = u * PI * 0.5f;
        Color c = MezclaAz(pie, NATA_AZ, Lim01Az(u * 1.6f));
        if (u > 0.72f) c = NIEVE_AZ;

        // Chorreado de chocolate desde la cima.
        if (decoracion)
        {
            float tira = std::sin(a * 6.0f + (float)semilla * 1.7f);
            float largo = 0.5f + 0.18f * std::sin(a * 13.0f + (float)semilla);
            if (tira > 0.55f && u > largo) c = Color{ 104, 58, 38, 255 };
        }

        Vector3 n = { std::cos(a) * std::cos(ph), std::sin(ph), std::sin(a) * std::cos(ph) };
        return TonoAz(c, LuzAz(n));
    };

    for (int j = 0; j < pisos; j++)
    {
        for (int k = 0; k < lados; k++)
        {
            QuadVAz(P(k, j), P(k, j + 1), P(k + 1, j + 1), P(k + 1, j), C(k, j), C(k, j + 1), C(k + 1, j + 1), C(k + 1, j));
        }
    }

    if (decoracion)
    {
        float r = 0.08f * R;
        ElipsoideAz({ cx, alto + r * 0.8f, cz }, { r, r, r }, 10, 6, Color{ 222, 34, 70, 255 });
        CilindroAz({ cx, alto + r * 1.6f, cz }, { cx + r * 0.5f, alto + r * 3.0f, cz }, r * 0.12f, r * 0.08f, 5, Color{ 90, 130, 60, 255 });
    }
}


static void FondoAz(bool decoracion)
{
    // Mar de nubes de algodon bajo la meseta (ondulado en rosa y lila).
    for (int ix = -10; ix < 10; ix++)
    {
        for (int iz = 0; iz < 8; iz++)
        {
            float x0 = (float)ix * 44.0f, x1 = x0 + 44.0f;
            float z0 = Z_BORDE_MESETA_AZ + 6.0f - (float)iz * 44.0f, z1 = z0 - 44.0f;
            auto Mar = [&](float x, float z)
            {
                return MezclaAz(Color{ 255, 226, 236, 255 }, Color{ 226, 206, 246, 255 }, 0.5f + 0.5f * std::sin(x * 0.03f + z * 0.05f));
            };
            QuadVAz({ x0, Y_MAR_NUBES_AZ, z0 }, { x1, Y_MAR_NUBES_AZ, z0 }, { x1, Y_MAR_NUBES_AZ, z1 }, { x0, Y_MAR_NUBES_AZ, z1 }, Mar(x0, z0), Mar(x1, z0), Mar(x1, z1), Mar(x0, z1));
        }
    }

    // Cordillera lejana (mas clara por la bruma) y cercana.
    const Color rocaLejana = { 214, 170, 214, 255 };
    const Color rocaCercana = { 236, 150, 182, 255 };
    int cantidadLejana = decoracion ? 9 : 5;

    for (int k = 0; k < cantidadLejana; k++)
    {
        float x = -260.0f + 520.0f * ((float)k + 0.5f) / (float)cantidadLejana + 20.0f * (AzarAz(k, 51) - 0.5f);
        float z = -215.0f - 30.0f * AzarAz(k, 52);
        MontanaAz(x, z, -70.0f, 46.0f + 20.0f * AzarAz(k, 53), 34.0f + 30.0f * AzarAz(k, 54), rocaLejana, k + 100);
    }

    int cantidadCercana = decoracion ? 8 : 4;
    for (int k = 0; k < cantidadCercana; k++)
    {
        float x = -170.0f + 340.0f * ((float)k + 0.5f) / (float)cantidadCercana + 18.0f * (AzarAz(k, 61) - 0.5f);
        float z = -120.0f - 22.0f * AzarAz(k, 62);
        MontanaAz(x, z, -40.0f, 30.0f + 14.0f * AzarAz(k, 63), 14.0f + 18.0f * AzarAz(k, 64), rocaCercana, k + 200);
    }

    // Colinas de nata en el borde de la meseta (media distancia).
    ColinaNataAz(-34.0f, -52.0f, -16.0f, 15.0f, 9.0f, 1, decoracion);
    ColinaNataAz(-9.0f, -58.0f, -16.0f, 17.0f, 13.0f, 2, decoracion);
    ColinaNataAz(17.0f, -54.0f, -16.0f, 15.0f, 10.0f, 3, decoracion);
    ColinaNataAz(42.0f, -48.0f, -16.0f, 13.0f, 7.5f, 4, decoracion);

    if (decoracion)
    {
        ColinaNataAz(-56.0f, -44.0f, -16.0f, 12.0f, 6.0f, 5, decoracion);
        ColinaNataAz(4.0f, -78.0f, -16.0f, 14.0f, 9.0f, 6, decoracion);
    }
}


// Sol de la tarde: disco con halo, mirando al centro.
static void SolAz(Vector3 c, float r)
{
    Vector3 n = Vector3Normalize(Vector3Negate(Vector3{ c.x, 0.0f, c.z }));
    Vector3 u = Vector3Normalize(Vector3CrossProduct(n, Vector3{ 0, 1, 0 }));
    Vector3 v = { 0, 1, 0 };
    const int lados = 28;
    const float radios[5] = { 0.0f, r, r * 1.15f, r * 1.8f, r * 3.2f };
    const Color colores[5] =
    {
        { 255, 250, 228, 255 }, { 255, 246, 214, 255 }, { 255, 236, 200, 150 }, { 255, 220, 200, 60 }, { 255, 210, 210, 0 }
    };

    for (int j = 0; j < 4; j++)
    {
        for (int k = 0; k < lados; k++)
        {
            float a0 = 2.0f * PI * (float)k / (float)lados, a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            auto P = [&](float rr, float a) { return Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a) * rr), Vector3Scale(v, std::sin(a) * rr))); };
            QuadColAz(P(radios[j], a0), P(radios[j], a1), P(radios[j + 1], a1), P(radios[j + 1], a0), colores[j], colores[j], colores[j + 1], colores[j + 1]);
        }
    }
}


static void CieloAz()
{
    // Telon cilindrico, degradado por altura: bruma, horizonte, rosa, lila y azul.
    const float radio = 320.0f;
    const int lados = 32;
    const float alturas[6] = { -120.0f, -20.0f, 10.0f, 60.0f, 140.0f, 300.0f };
    const Color colores[6] = { BRUMA_AZ, BRUMA_AZ, HORIZONTE_AZ, CIELO_BAJO_AZ, CIELO_MEDIO_AZ, CIELO_ALTO_AZ };
    bool niebla = g_nieblaAz;
    g_nieblaAz = false;

    for (int k = 0; k < lados; k++)
    {
        float a0 = -PI * 0.5f - 1.9f + 3.8f * (float)k / (float)lados;
        float a1 = -PI * 0.5f - 1.9f + 3.8f * (float)(k + 1) / (float)lados;
        float x0 = std::cos(a0) * radio, z0 = std::sin(a0) * radio;
        float x1 = std::cos(a1) * radio, z1 = std::sin(a1) * radio;

        for (int j = 0; j < 5; j++)
        {
            QuadVAz(
                { x0, alturas[j], z0 }, { x1, alturas[j], z1 },
                { x1, alturas[j + 1], z1 }, { x0, alturas[j + 1], z0 },
                colores[j], colores[j], colores[j + 1], colores[j + 1]
            );
        }
    }

    SolAz({ -118.0f, 46.0f, -280.0f }, 15.0f);
    g_nieblaAz = niebla;
}


// Nube de algodon de azucar: racimo de bultos, clara arriba y lila abajo.
static void NubeAz(int semilla, float ancho)
{
    bool niebla = g_nieblaAz;
    g_nieblaAz = false;
    int bultos = 6 + semilla % 3;

    for (int k = 0; k < bultos; k++)
    {
        float u = ((float)k + 0.5f) / (float)bultos - 0.5f;
        float r = ancho * (0.2f + 0.14f * AzarAz(k, semilla)) * (1.0f - 1.1f * std::fabs(u));
        Vector3 c = { u * ancho, r * 0.25f + ancho * 0.05f * AzarAz(k, semilla + 3), ancho * 0.1f * (AzarAz(k, semilla + 5) - 0.5f) };
        Vector3 rr = { r * 1.2f, r * 0.8f, r };
        const int lados = 12, pisos = 6;

        for (int j = 0; j < pisos; j++)
        {
            float p0 = -PI * 0.5f + PI * (float)j / (float)pisos, p1 = -PI * 0.5f + PI * (float)(j + 1) / (float)pisos;
            for (int i = 0; i < lados; i++)
            {
                float a0 = 2.0f * PI * (float)i / (float)lados, a1 = 2.0f * PI * (float)(i + 1) / (float)lados;
                auto P = [&](float a, float p) { return Vector3{ c.x + rr.x * std::cos(p) * std::cos(a), c.y + rr.y * std::sin(p), c.z + rr.z * std::cos(p) * std::sin(a) }; };
                auto Col = [&](float p)
                {
                    float s = 0.5f + 0.5f * std::sin(p);
                    return MezclaAz(Color{ 232, 196, 236, 255 }, Color{ 255, 246, 248, 255 }, s);
                };
                QuadColAz(P(a0, p0), P(a0, p1), P(a1, p1), P(a1, p0), Col(p0), Col(p1), Col(p1), Col(p0));
            }
        }
    }

    g_nieblaAz = niebla;
}


// Graba el escenario estatico para la calidad actual.
static void HornearAz()
{
    int clave = ClaveHornoAz();

    if ((g_hornoAz.listo && g_hornoAz.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasAzucar();

    if (!g_hornoAz.materialListo)
    {
        g_hornoAz.material = LoadMaterialDefault();
        g_hornoAz.materialListo = true;
    }

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    g_ojoAz = OJO_AZ;

    EmpezarGrabacionAz();
    BaseAz();
    g_hornoAz.validas[MALLA_AZ_BASE] = TerminarGrabacionAz(g_hornoAz.mallas[MALLA_AZ_BASE]);

    EmpezarGrabacionAz();
    SueloAz(decoracion, sombras);
    g_hornoAz.validas[MALLA_AZ_SUELO] = TerminarGrabacionAz(g_hornoAz.mallas[MALLA_AZ_SUELO]);

    EmpezarGrabacionAz();
    FondoAz(decoracion);
    g_hornoAz.validas[MALLA_AZ_FONDO] = TerminarGrabacionAz(g_hornoAz.mallas[MALLA_AZ_FONDO]);

    EmpezarGrabacionAz();
    UtileriaAz(decoracion);
    g_hornoAz.validas[MALLA_AZ_UTILERIA] = TerminarGrabacionAz(g_hornoAz.mallas[MALLA_AZ_UTILERIA]);

    EmpezarGrabacionAz();
    CieloAz();
    g_hornoAz.validas[MALLA_AZ_CIELO] = TerminarGrabacionAz(g_hornoAz.mallas[MALLA_AZ_CIELO]);

    if (decoracion)
    {
        for (int k = 0; k < 3; k++)
        {
            EmpezarGrabacionAz();
            NubeAz(k * 7 + 3, 1.0f);
            g_hornoAz.validas[MALLA_AZ_NUBE_A + k] = TerminarGrabacionAz(g_hornoAz.mallas[MALLA_AZ_NUBE_A + k]);
        }
    }

    g_hornoAz.clave = clave;
    g_hornoAz.listo = true;
}


static void DibujarMallaAz(MallaAz malla)
{
    if (g_hornoAz.listo && g_hornoAz.validas[malla])
    {
        DrawMesh(g_hornoAz.mallas[malla], g_hornoAz.material, MatrixIdentity());
    }
}


// Nubes de algodon que se desplazan despacio (una malla por forma, con
// escala y traslacion por instancia).
static void NubesAz(float t)
{
    struct InstanciaNube { float x, y, z, escala, velocidad; int forma; };
    static const InstanciaNube nubes[] =
    {
        { -150.0f, 70.0f, -250.0f, 46.0f, 1.6f, 0 },
        { -40.0f, 92.0f, -270.0f, 58.0f, 1.2f, 1 },
        { 90.0f, 64.0f, -240.0f, 40.0f, 1.8f, 2 },
        { 190.0f, 104.0f, -280.0f, 62.0f, 1.0f, 0 },
        { -230.0f, 120.0f, -260.0f, 52.0f, 1.4f, 2 },
        { 30.0f, 36.0f, -180.0f, 30.0f, 2.2f, 1 },
        { -95.0f, 22.0f, -150.0f, 24.0f, 2.6f, 0 },
        { 120.0f, 18.0f, -160.0f, 26.0f, 2.4f, 2 },
        // Bajas, sobre el mar de nubes.
        { -60.0f, -9.0f, -96.0f, 22.0f, 1.1f, 1 },
        { 52.0f, -10.0f, -88.0f, 20.0f, 1.3f, 0 },
        { -4.0f, -11.0f, -132.0f, 26.0f, 0.9f, 2 }
    };

    for (const InstanciaNube& n : nubes)
    {
        MallaAz malla = (MallaAz)(MALLA_AZ_NUBE_A + n.forma);

        if (!g_hornoAz.listo || !g_hornoAz.validas[malla])
        {
            continue;
        }

        float x = n.x + t * n.velocidad;
        x = std::fmod(x + 300.0f, 600.0f);
        if (x < 0.0f) x += 600.0f;
        x -= 300.0f;

        Matrix m = MatrixMultiply(MatrixScale(n.escala, n.escala, n.escala), MatrixTranslate(x, n.y, n.z));
        DrawMesh(g_hornoAz.mallas[malla], g_hornoAz.material, m);
    }
}


//--------------------------------------------------
// Piezas dinamicas: gominolas, bolas, charcos animados
//--------------------------------------------------

// Gominolas gigantes: radio exacto del rebote, se bambolean como gelatina.
static void GominolasAz(float t)
{
    LoteAz();

    for (int i = 0; i < 4; i++)
    {
        const CirculoBolas& g = GOMINOLAS_BOLAS[i];
        float bamboleo = std::sin(t * 2.6f + (float)i * 1.7f) * 0.04f;
        float r = g.radio;
        Vector3 radios = { r * (1.0f + bamboleo), 1.12f * (1.0f - bamboleo), r * (1.0f + bamboleo) };
        ElipsoideAz({ g.x, 0.0f, g.z }, radios, 16, 7, COLORES_GOMINOLA_AZ[i], 0.0f);

        // Cristales de azucar pegados a la gelatina.
        Color cristal = MezclaAz(COLORES_GOMINOLA_AZ[i], WHITE, 0.75f);
        for (int k = 0; k < 26; k++)
        {
            float a = 2.0f * PI * AzarAz(k, i + 300);
            float p = 0.12f + 1.3f * AzarAz(k, i + 310);
            Vector3 n = { std::cos(p) * std::cos(a), std::sin(p), std::cos(p) * std::sin(a) };
            Vector3 q = { g.x + n.x * radios.x * 1.02f, n.y * radios.y * 1.02f, g.z + n.z * radios.z * 1.02f };
            Vector3 u = Vector3Normalize(Vector3CrossProduct(n, Vector3{ 0.3f, 1.0f, 0.2f }));
            Vector3 v = Vector3CrossProduct(n, u);
            float s = 0.045f;
            QuadAz(Vector3Subtract(q, Vector3Scale(u, s)), Vector3Subtract(q, Vector3Scale(v, s)), Vector3Add(q, Vector3Scale(u, s)), Vector3Add(q, Vector3Scale(v, s)), cristal);
        }

        // Brillo de la gelatina hacia la luz.
        Vector3 b = { g.x - radios.x * 0.38f, radios.y * 0.78f, g.z + radios.z * 0.36f };
        Color brillo = AlfaAz(WHITE, 0.8f);
        Color nada = AlfaAz(WHITE, 0.0f);
        float rb = 0.16f;
        QuadColAz({ b.x - rb, b.y, b.z }, { b.x, b.y + rb * 0.7f, b.z + 0.05f }, { b.x + rb, b.y, b.z }, { b.x, b.y - rb * 0.7f, b.z - 0.05f }, nada, brillo, nada, brillo);
    }

    FinLoteAz();
}


//--------------------------------------------------
// Animado: suelo (sin profundidad, sobre el suelo horneado)
//--------------------------------------------------

static void DecalesSueloAz(
    const MinijuegoBolasAzucar& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    bool efectos
)
{
    float t = m.tiempoAnimacion;
    bool niebla = g_nieblaAz;
    g_nieblaAz = false;
    LoteAz();

    if (efectos)
    {
        // Corriente del rio de chocolate.
        float largo = Z_FIN_RIO_AZ - Z_CABECERA_RIO_AZ;
        Color veta = { 186, 128, 92, 255 };

        for (int k = 0; k < 46; k++)
        {
            float z = Z_CABECERA_RIO_AZ + std::fmod(t * (1.5f + 0.5f * AzarAz(k, 1)) + AzarAz(k, 2) * largo, largo);
            float w = RioAnchoAz(z) * 0.5f;
            float x = RioXAz(z) + (AzarAz(k, 3) - 0.5f) * w * 1.3f;
            float l = 0.5f + 0.6f * AzarAz(k, 4);
            float x2 = RioXAz(z + l) + (AzarAz(k, 3) - 0.5f) * RioAnchoAz(z + l) * 0.65f;
            float fade = Lim01Az((z - Z_CABECERA_RIO_AZ) / 2.0f) * Lim01Az((Z_FIN_RIO_AZ - z) / 3.0f);
            Color c = AlfaAz(veta, 0.55f * fade);
            Color n = AlfaAz(veta, 0.0f);
            float g = 0.05f;
            QuadColAz({ x - g, 0, z }, { x2 - g, 0, z + l }, { x2 + g, 0, z + l }, { x + g, 0, z }, n, c, c, n);
        }

        // Espuma de la cascada en el remanso.
        float xc = RioXAz(Z_CABECERA_RIO_AZ);
        for (int k = 0; k < 3; k++)
        {
            float fase = std::fmod(t * 0.7f + (float)k / 3.0f, 1.0f);
            float r = 0.3f + fase * 1.4f;
            AnilloAz({ xc, 0, Z_CABECERA_RIO_AZ + 0.2f }, r, r + 0.08f, 20, AlfaAz(Color{ 214, 170, 140, 255 }, 0.5f * (1.0f - fase)), AlfaAz(Color{ 214, 170, 140, 255 }, 0.0f), false);
        }

        // Charcos: ondas, burbujas que revientan y un brillo que se pasea.
        for (int i = 0; i < 3; i++)
        {
            const CirculoBolas& c = CHARCOS_BOLAS[i];
            Color onda = { 150, 96, 66, 255 };

            for (int k = 0; k < 2; k++)
            {
                float fase = std::fmod(t * 0.38f + (float)i * 0.31f + (float)k * 0.5f, 1.0f);
                float r = c.radio * (0.12f + 0.74f * fase);
                AnilloAz({ c.x, 0, c.z }, r, r + 0.06f, 28, AlfaAz(onda, 0.5f * (1.0f - fase)), AlfaAz(onda, 0.0f), false);
            }

            for (int k = 0; k < 4; k++)
            {
                float ciclo = 2.4f + 0.8f * AzarAz(k, i + 20);
                float fase = std::fmod(t + AzarAz(k, i + 21) * ciclo, ciclo) / ciclo;
                float ciclos = std::floor((t + AzarAz(k, i + 21) * ciclo) / ciclo);
                float a = 2.0f * PI * AzarAz(k + (int)ciclos * 7, i + 22);
                float d = c.radio * 0.65f * std::sqrt(AzarAz(k + (int)ciclos * 7, i + 23));
                float bx = c.x + std::cos(a) * d, bz = c.z + std::sin(a) * d;

                if (fase < 0.8f)
                {
                    float r = 0.04f + 0.12f * (fase / 0.8f);
                    DiscoAz({ bx, 0, bz }, r, 10, Color{ 120, 72, 46, 255 });
                    DiscoAz({ bx - r * 0.3f, 0, bz - r * 0.3f }, r * 0.35f, 8, AlfaAz(Color{ 230, 190, 160, 255 }, 0.8f));
                }
                else
                {
                    float u = (fase - 0.8f) / 0.2f;
                    float r = 0.16f + 0.25f * u;
                    AnilloAz({ bx, 0, bz }, r, r + 0.04f, 12, AlfaAz(Color{ 190, 136, 100, 255 }, 0.7f * (1.0f - u)), AlfaAz(Color{ 190, 136, 100, 255 }, 0.0f), false);
                }
            }
        }
    }

    // Anillo de cada jugador en el suelo.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo || !m.estadosJugadores[i].vivo)
        {
            continue;
        }

        const Vector3& p = jugadores[i].posicion;
        Color c = participantes[i].color;
        float pulso = m.estadosJugadores[i].inmunidad > 0.0f ? 0.5f + 0.5f * std::sin(t * 18.0f) : 1.0f;
        AnilloAz({ p.x, 0, p.z }, 0.5f, 0.66f, 22, AlfaAz(c, 0.85f * pulso), AlfaAz(c, 0.55f * pulso), false);
        AnilloAz({ p.x, 0, p.z }, 0.66f, 0.8f, 22, AlfaAz(c, 0.4f * pulso), AlfaAz(c, 0.0f), false);
    }

    // Bolas: sombra y aro del dueno; si ya quita vidas, aro grueso que late.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const BolaAzucar& b = m.bolas[i];

        if (!b.activa)
        {
            continue;
        }

        float alfa = b.estado == BOLA_AZUCAR_REPOSO ? 1.0f - b.inactiva / 3.0f * 0.6f : 1.0f;
        SombraAz(b.x + 0.12f, b.z - 0.1f, b.radio * 1.08f, b.radio * 1.0f, 0.45f * alfa);

        if (b.estado == BOLA_AZUCAR_REPOSO)
        {
            continue;
        }

        Color c = participantes[i].color;
        bool grande = b.radio > RADIO_DANO_BOLA;
        float ancho = grande ? 0.2f + 0.06f * std::sin(t * 10.0f) : 0.1f;
        AnilloAz({ b.x, 0, b.z }, b.radio + 0.04f, b.radio + 0.04f + ancho, 28, AlfaAz(c, 0.9f), AlfaAz(c, grande ? 0.5f : 0.2f), false);

        if (grande)
        {
            // Puntas del aro: «esta bola ya quita vidas».
            for (int k = 0; k < 8; k++)
            {
                float a = 2.0f * PI * (float)k / 8.0f + t * 1.5f;
                float r0 = b.radio + 0.2f, r1 = b.radio + 0.48f + 0.06f * std::sin(t * 10.0f);
                Vector3 d = { std::cos(a), 0, std::sin(a) };
                Vector3 e = { -d.z * 0.09f, 0, d.x * 0.09f };
                Vector3 base = { b.x + d.x * r0, 0, b.z + d.z * r0 };
                Vector3 punta = { b.x + d.x * r1, 0, b.z + d.z * r1 };
                VAz(Vector3Add(base, e), AlfaAz(c, 0.9f));
                VAz(punta, AlfaAz(c, 0.9f));
                VAz(Vector3Subtract(base, e), AlfaAz(c, 0.9f));
            }
        }
    }

    FinLoteAz();
    g_nieblaAz = niebla;
}


//--------------------------------------------------
// Animado: cascada, guirnalda y personajes (con profundidad)
//--------------------------------------------------

// Cascada de chocolate que cae de la pena al remanso.
static void CascadaAz(float t, bool efectos)
{
    float xc = RioXAz(Z_CABECERA_RIO_AZ);
    float zf = Z_CABECERA_RIO_AZ - 0.62f;
    const float alto = 6.4f;
    const int columnas = 8, tramos = 12;
    float ancho = 2.4f;

    LoteAz();

    for (int k = 0; k < columnas; k++)
    {
        float s0 = -ancho * 0.5f + ancho * (float)k / (float)columnas;
        float s1 = -ancho * 0.5f + ancho * (float)(k + 1) / (float)columnas;

        for (int j = 0; j < tramos; j++)
        {
            float u0 = (float)j / (float)tramos, u1 = (float)(j + 1) / (float)tramos;
            float y0 = alto * (1.0f - u0), y1 = alto * (1.0f - u1);
            // Labio arriba y caida que se abre hacia el remanso.
            float z0 = zf + 0.55f * std::sqrt(u0) + 0.25f * u0;
            float z1 = zf + 0.55f * std::sqrt(u1) + 0.25f * u1;
            float abre0 = 1.0f + 0.25f * u0, abre1 = 1.0f + 0.25f * u1;
            float vetas0 = efectos ? std::sin(u0 * 9.0f - t * 7.0f + (float)k * 1.9f) : 0.0f;
            float vetas1 = efectos ? std::sin(u1 * 9.0f - t * 7.0f + (float)k * 1.9f) : 0.0f;
            Color c0 = MezclaAz(CHOCOLATE_AZ, CHOCOLATE_BRILLO_AZ, 0.35f + 0.3f * vetas0);
            Color c1 = MezclaAz(CHOCOLATE_AZ, CHOCOLATE_BRILLO_AZ, 0.35f + 0.3f * vetas1);
            QuadVAz({ xc + s0 * abre0, y0, z0 }, { xc + s1 * abre0, y0, z0 }, { xc + s1 * abre1, y1, z1 }, { xc + s0 * abre1, y1, z1 }, c0, c0, c1, c1);
        }
    }

    FinLoteAz();
}


// Guirnalda de banderines de caramelo sobre el muro de atras.
static void GuirnaldaAz(float t, bool efectos)
{
    float m = MITAD_ARENA_BOLAS + 0.2f;
    const int n = 16;
    const float yPoste = 2.5f, caida = 0.55f;

    LoteAz();

    for (int k = 0; k < n; k++)
    {
        float s0 = (float)k / (float)n, s1 = (float)(k + 1) / (float)n;
        float x0 = -m + 2.0f * m * s0, x1 = -m + 2.0f * m * s1;
        float y0 = yPoste - caida * 4.0f * s0 * (1.0f - s0);
        float y1 = yPoste - caida * 4.0f * s1 * (1.0f - s1);
        float z = -m;
        float vaiven = efectos ? std::sin(t * 3.2f + (float)k * 0.9f) * 0.12f : 0.0f;
        Color c = COLORES_DULCE_AZ[k % 5];
        Vector3 a = { x0 + 0.06f, y0, z }, b = { x1 - 0.06f, y1, z };
        Vector3 punta = { (x0 + x1) * 0.5f, (y0 + y1) * 0.5f - 0.48f, z + vaiven };
        TriAz(a, b, punta, c);
        TriAz(a, punta, b, TonoAz(c, 0.8f));

        // Cordel.
        Color cordel = { 255, 246, 236, 255 };
        Vector3 e = { 0, 0.025f, 0 };
        QuadAz(Vector3Subtract(Vector3{ x0, y0, z }, e), Vector3Subtract(Vector3{ x1, y1, z }, e), Vector3Add(Vector3{ x1, y1, z }, e), Vector3Add(Vector3{ x0, y0, z }, e), cordel);
    }

    FinLoteAz();
}


// Estrellitas que giran sobre la cabeza del aturdido.
static void EstrellasAturdidoAz(Vector3 cabeza, float t)
{
    for (int k = 0; k < 3; k++)
    {
        float a = t * 5.0f + 2.0f * PI * (float)k / 3.0f;
        Vector3 p = { cabeza.x + std::cos(a) * 0.42f, cabeza.y + 0.08f * std::sin(a * 2.0f), cabeza.z + std::sin(a) * 0.42f };
        DestelloAz(p, 0.2f, Color{ 255, 240, 150, 255 });
    }
}


// Remolino de azucar donde aparece la bola nueva.
static void RemolinoCreacionAz(Vector3 centro, float progreso, float t)
{
    int n = 7;
    for (int k = 0; k < n; k++)
    {
        float a = t * 9.0f + 2.0f * PI * (float)k / (float)n;
        float r = 0.55f * (1.0f - progreso) + 0.12f;
        Vector3 p = { centro.x + std::cos(a) * r, 0.15f + 0.4f * progreso + 0.1f * std::sin(a * 3.0f), centro.z + std::sin(a) * r };
        CartelAz(p, 0.13f, 0.13f, AlfaAz(AZUCAR_AZ, 0.9f));
    }

    CartelAz({ centro.x, 0.2f + 0.1f * progreso, centro.z }, 0.25f + 0.35f * progreso, 0.25f + 0.35f * progreso, AlfaAz(AZUCAR_AZ, 0.65f));
}


// Estallido de azucar (golpe o salida): chispas del color del jugador.
static void EstallidoAz(Vector3 centro, float u, Color color, int semilla, int cantidad)
{
    if (u < 0.0f || u > 1.0f)
    {
        return;
    }

    for (int k = 0; k < cantidad; k++)
    {
        float a = 2.0f * PI * AzarAz(k, semilla);
        float v = 1.8f + 2.2f * AzarAz(k, semilla + 1);
        float sube = 2.5f + 2.5f * AzarAz(k, semilla + 2);
        float tt = u * 0.9f;
        Vector3 p = { centro.x + std::cos(a) * v * tt, centro.y + sube * tt - 4.5f * tt * tt, centro.z + std::sin(a) * v * tt };
        Color c = (k % 3 == 0) ? AZUCAR_AZ : color;
        float s = 0.14f * (1.0f - u * 0.6f);
        CartelAz(p, s, s, AlfaAz(c, 1.0f - u * u));
    }
}


// Espuma y salpicaduras al pie de la cascada.
static void EspumaCascadaAz(float t)
{
    float xc = RioXAz(Z_CABECERA_RIO_AZ);

    for (int k = 0; k < 14; k++)
    {
        float ciclo = 0.9f + 0.5f * AzarAz(k, 601);
        float fase = std::fmod(t + AzarAz(k, 602) * ciclo, ciclo) / ciclo;
        float x = xc + (AzarAz(k, 603) - 0.5f) * 2.6f;
        float z = Z_CABECERA_RIO_AZ + 0.1f + 0.5f * AzarAz(k, 604);
        float y = 0.15f + 0.9f * fase - 0.6f * fase * fase;
        float s = 0.25f + 0.35f * fase;
        Color c = (k % 2 == 0) ? Color{ 236, 206, 184, 255 } : Color{ 196, 146, 112, 255 };
        CartelAz({ x, y, z }, s, s, AlfaAz(c, 0.75f * (1.0f - fase)));
    }
}


// Confeti sobre el ganador.
static void ConfetiAz(Vector3 centro, float t, int semilla)
{
    const Color colores[6] = { ROSA_AZ, MENTA_AZ, LILA_AZ, CARAMELO_AZ, Color{ 255, 240, 150, 255 }, Color{ 140, 200, 255, 255 } };
    int n = (int)(70.0f * FactorCalidadGrafica(CalidadParticulas())) + 12;

    for (int k = 0; k < n; k++)
    {
        float ciclo = 2.6f + 1.2f * AzarAz(k, semilla);
        float fase = std::fmod(t + AzarAz(k, semilla + 1) * ciclo, ciclo) / ciclo;
        float a = 2.0f * PI * AzarAz(k, semilla + 2);
        float d = 0.4f + 2.6f * AzarAz(k, semilla + 3);
        Vector3 p = { centro.x + std::cos(a) * d + 0.3f * std::sin(t * 3.0f + (float)k), 5.5f - fase * 5.6f, centro.z + std::sin(a) * d };
        float giro = t * 6.0f + (float)k;
        float w = 0.16f * std::fabs(std::cos(giro)) + 0.03f;
        CartelAz(p, w, 0.11f, colores[k % 6]);
    }
}


//--------------------------------------------------
// Animado: transparencias y luces (despues del cielo)
//--------------------------------------------------

static void BrillosAzucarAz(float t)
{
    for (int i = 0; i < 4; i++)
    {
        const RectanguloAzucarBolas& z = ZONAS_AZUCAR_BOLAS[i];
        int n = (int)((z.xMax - z.xMin) * (z.zMax - z.zMin) * 0.5f);

        for (int k = 0; k < n; k++)
        {
            float ciclo = 1.6f + 1.4f * AzarAz(k, i + 400);
            float fase = std::fmod(t + AzarAz(k, i + 401) * ciclo, ciclo) / ciclo;
            int vuelta = (int)std::floor((t + AzarAz(k, i + 401) * ciclo) / ciclo);
            float brillo = std::sin(fase * PI);
            brillo = brillo * brillo * brillo;
            float x = z.xMin + 0.15f + (z.xMax - z.xMin - 0.3f) * AzarAz(k + vuelta * 17, i + 402);
            float zz = z.zMin + 0.15f + (z.zMax - z.zMin - 0.3f) * AzarAz(k + vuelta * 17, i + 403);
            DestelloAz({ x, 0.08f, zz }, 0.08f + 0.2f * brillo, AlfaAz(ACENTO_AZ, brillo));
        }
    }
}


static void NieveAzucarAz(float t)
{
    int n = (int)(170.0f * FactorCalidadGrafica(CalidadParticulas()));
    const float alto = 14.0f;

    for (int k = 0; k < n; k++)
    {
        float velocidad = 0.7f + 0.5f * AzarAz(k, 501);
        float y = alto - std::fmod(t * velocidad + AzarAz(k, 502) * alto, alto);
        float x = -24.0f + 48.0f * AzarAz(k, 503) + 0.5f * std::sin(t * 0.9f + (float)k);
        float z = -26.0f + 40.0f * AzarAz(k, 504) + 0.3f * std::cos(t * 0.7f + (float)k * 1.3f);
        float s = 0.05f + 0.05f * AzarAz(k, 505);
        float alfa = Lim01Az(y / 1.5f) * 0.9f;
        CartelAz({ x, y, z }, s, s, AlfaAz(WHITE, alfa));
    }
}


static void LucesAz(float t, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteAz();

    if (decoracion)
    {
        for (int k = 0; k < 4; k++)
        {
            const Vector3& f = FAROLES_AZ[k];
            float parpadeo = 0.88f + 0.12f * std::sin(t * 7.0f + (float)k * 2.3f) * std::sin(t * 3.1f + (float)k);
            Vector3 globo = { f.x, 3.44f, f.z };
            Color luz = LUZ_FAROL_AZ;

            HaloAz(globo, 0.75f * parpadeo, AlfaAz(luz, 0.55f));
            HaloAz(globo, 1.7f * parpadeo, AlfaAz(luz, 0.22f));

            // Charco de luz en la nieve.
            for (int j = 0; j < 3; j++)
            {
                float r = 1.2f + 1.0f * (float)j;
                AnilloAz({ f.x, 0.02f, f.z }, 0.0f, r * parpadeo, 20, AlfaAz(luz, 0.07f), AlfaAz(luz, 0.0f), false);
            }
        }

        // Resplandor tibio del sol sobre la cascada y el remanso.
        float xc = RioXAz(Z_CABECERA_RIO_AZ);
        AnilloAz({ xc, 0.03f, Z_CABECERA_RIO_AZ + 0.6f }, 0.0f, 2.4f, 20, AlfaAz(Color{ 255, 190, 140, 255 }, 0.08f), AlfaAz(Color{ 255, 190, 140, 255 }, 0.0f), false);
    }

    BrillosAzucarAz(t);

    FinLoteAz();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


static void DibujarBolaAz(const BolaAzucar& bola, Color colorDuenio, bool debug)
{
    float alpha = bola.estado == BOLA_AZUCAR_REPOSO
        ? 1.0f - bola.inactiva / 3.0f * 0.6f
        : 1.0f;

    if (!DibujarBolaAzucarRetro3D({ bola.x, bola.radio, bola.z }, bola.radio, bola.giro, alpha, colorDuenio))
    {
        // Sin el GLB: bola de azucar con primitivas.
        (DrawSphere)({ bola.x, bola.radio, bola.z }, bola.radio, Fade(AZUCAR_AZ, alpha));
        (DrawSphere)(
            { bola.x + std::cos(bola.giro) * bola.radio * 0.7f, bola.radio + std::sin(bola.giro) * bola.radio * 0.7f, bola.z + bola.radio * 0.5f },
            bola.radio * 0.16f,
            Fade(colorDuenio, alpha)
        );
    }

    if (debug)
    {
        DrawSphereWires({ bola.x, bola.radio, bola.z }, bola.radio + 0.45f, 8, 8, RED);
    }
}


//--------------------------------------------------
// Camara
//--------------------------------------------------

// Durante la cuenta la camara arranca baja, mirando la cordillera y el
// cielo de algodon, y sube hasta la vista de juego.
static const Vector3 OJO_INTRO_AZ = { -8.0f, 6.2f, 27.0f };
static const Vector3 MIRA_INTRO_AZ = { 1.5f, 4.2f, -24.0f };

static Camera3D CamaraVistaAz(const MinijuegoBolasAzucar& m)
{
    Camera3D c{};
    c.position = OJO_AZ;
    c.target = MIRA_AZ;
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = FOV_AZ;
    c.projection = CAMERA_PERSPECTIVE;

    if (m.fase == FASE_BOLAS_PREPARACION)
    {
        float u = Lim01Az(m.tiempoPreparacion / DURACION_PREPARACION_BOLAS);
        // Se queda un momento en la vista baja y llega antes del «¡YA!».
        float s = SuaveAz((u - 0.2f) / 0.65f) * ObtenerFactorMovimientoCamaraMinijuegos();
        c.position = Vector3Lerp(OJO_AZ, OJO_INTRO_AZ, s);
        c.target = Vector3Lerp(MIRA_AZ, MIRA_INTRO_AZ, s);
    }

    return AplicarTemblorGeneralACamara(c);
}


//--------------------------------------------------
// Anuncios y HUD
//--------------------------------------------------

static float AlfaAnuncioAz(float u)
{
    if (u < 0.12f) return u / 0.12f;
    if (u > 0.8f) return Lim01Az((1.0f - u) / 0.2f);
    return 1.0f;
}


static Vector3 PosicionVisualJugadorAz(const MinijuegoBolasAzucar& m, const JugadorPrueba jugadores[], int i)
{
    if (!m.estadosJugadores[i].vivo)
    {
        return { m.salidaX[i], 0.7f, m.salidaZ[i] };
    }

    return jugadores[i].posicion;
}


static void AnunciosAz(
    const MinijuegoBolasAzucar& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    const Camera3D& vista
)
{
    if (m.fase != FASE_BOLAS_JUGANDO || m.mensajeTiempo <= 0.0f || m.mensajeJugador < 0 || m.mensajeJugador >= MAX_PARTICIPANTES)
    {
        return;
    }

    int j = m.mensajeJugador;
    int numero = participantes[j].numeroJugador > 0 ? participantes[j].numeroJugador : j + 1;
    float u = Lim01Az(1.0f - m.mensajeTiempo / 1.6f);
    float alfa = AlfaAnuncioAz(u);
    const char* texto = nullptr;
    Color color = WHITE;

    if (m.mensajeTipo == 1) { texto = TextFormat("¡J%d PIERDE UNA VIDA!", numero); color = Color{ 255, 150, 196, 255 }; }
    else if (m.mensajeTipo == 2) { texto = TextFormat("¡J%d ATURDIDO!", numero); color = Color{ 255, 246, 220, 255 }; }
    else if (m.mensajeTipo == 3) { texto = TextFormat("¡J%d ELIMINADO!", numero); color = Color{ 255, 110, 110, 255 }; }

    if (texto == nullptr)
    {
        return;
    }

    Vector3 p = PosicionVisualJugadorAz(m, jugadores, j);
    Vector2 s = GetWorldToScreen({ p.x, p.y + 2.2f, p.z }, vista);
    float alto = (float)GetScreenHeight();
    float ancho = (float)GetScreenWidth();
    float y = s.y - Px(18.0f) - u * Px(22.0f);
    if (y < alto * 0.24f) y = alto * 0.24f;
    float x = s.x;
    float margen = ancho * 0.16f;
    if (x < margen) x = margen;
    if (x > ancho - margen) x = ancho - margen;

    DibujarTextoMundoContornoUI(texto, x, y, m.mensajeTipo == 2 ? UI_TEXTO : UI_TEXTO_GRANDE, color, alfa);
}


static void HudAz(const MinijuegoBolasAzucar& m, const Participante participantes[], int limite)
{
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "BOLAS DE AZÚCAR",
        "RODÁ TU BOLA POR EL AZÚCAR GLAS PARA QUE CREZCA Y LANZALA. ¡EL CHOCOLATE LA DERRITE!",
        m.resultado
    );

    hud.fase = m.fase == FASE_BOLAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_BOLAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante > 0.0f ? m.tiempoRestante : 0.0f;
    hud.tiempoTotal = DURACION_PARTIDA_BOLAS;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = m.fase == FASE_BOLAS_TERMINADO && m.tiempoRestante <= 0.0f;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorBolasAzucar& e = m.estadosJugadores[i];
        const BolaAzucar& bola = m.bolas[i];
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        const Participante& participante = participantes[i];

        jugador.visible = true;
        jugador.eliminado = !e.vivo;
        jugador.valorNumerico = e.vidas;
        std::snprintf(jugador.valor, sizeof(jugador.valor), e.vidas == 1 ? "%d VIDA" : "%d VIDAS", e.vidas);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), e.vidas == 1 ? "%d VIDA" : "%d VIDAS", e.vidas);

        bool propia = bola.activa && bola.estado == BOLA_AZUCAR_EMPUJADA;
        jugador.progreso = e.vivo && propia
            ? Lim01Az((bola.radio - RADIO_MINIMO_BOLA) / (RADIO_MAXIMO_BOLA - RADIO_MINIMO_BOLA))
            : (e.vivo ? 0.0f : -1.0f);

        if (m.fase == FASE_BOLAS_JUGANDO && e.vivo)
        {
            if (e.aturdido > 0.0f) std::snprintf(jugador.estado, sizeof(jugador.estado), "¡ATURDIDO!");
            else if (e.creando > 0.0f) std::snprintf(jugador.estado, sizeof(jugador.estado), "CREANDO BOLA");
            else if (propia && bola.radio > RADIO_DANO_BOLA) std::snprintf(jugador.estado, sizeof(jugador.estado), "¡LANZALA!");
            else if (propia) std::snprintf(jugador.estado, sizeof(jugador.estado), "HACELA CRECER");
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "%s: MOVER\n%s: CREAR / LANZAR",
                TextoMoverHud(participante),
                TextoBotonGolpeHud(participante)
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


//--------------------------------------------------
// Dibujar
//--------------------------------------------------

void MinijuegoBolasAzucar::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
    float t = tiempoAnimacion;

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    // (El telon del cielo cubre el resto; no hace falta un degradado 2D.)
    ClearBackground(BRUMA_AZ);

    Camera3D vista = CamaraVistaAz(*this);
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derAz = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaAz = Vector3CrossProduct(g_derAz, frente);
    HornearAz();

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Base (solo profundidad) y suelo pintado en orden, sin profundidad;
    // encima, lo animado del suelo (rio, charcos, aros y sombras).
    DibujarMallaAz(MALLA_AZ_BASE);
    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    DibujarMallaAz(MALLA_AZ_SUELO);
    DecalesSueloAz(*this, jugadores, participantes, limite, efectos);
    rlDrawRenderBatchActive();
    rlEnableDepthTest();

    // Lejania y utileria (opaco, con profundidad).
    DibujarMallaAz(MALLA_AZ_FONDO);
    DibujarMallaAz(MALLA_AZ_UTILERIA);
    GominolasAz(t);

    if (decoracion)
    {
        CascadaAz(t, efectos);
        GuirnaldaAz(t, efectos);
    }

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (bolas[i].activa)
        {
            DibujarBolaAz(bolas[i], participantes[i].color, mostrarDebug);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorBolasAzucar& estado = estadosJugadores[i];
        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;

        if (!estado.vivo)
        {
            // Salida: sale despedido girando y se deshace en azucar.
            float s = salidaVisual[i];
            if (s < 0.9f)
            {
                JugadorPrueba visual = jugadores[i];
                visual.posicion = { salidaX[i], 0.7f + 8.0f * s - 6.0f * s * s, salidaZ[i] - 2.6f * s };
                visual.velocidad = {};
                visual.direccionMirada = { std::cos(s * 22.0f), 0.0f, std::sin(s * 22.0f) };
                visual.aplastado = false;
                DibujarJugadorCuboPrueba(visual, participanteVisual);
            }
            continue;
        }

        JugadorPrueba visual = jugadores[i];
        visual.aplastado = estado.aturdido > 0.0f || estado.creando > 0.0f;
        visual.tiempoAplastado = 0.3f;
        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }
    }

    // Nubes y cielo despues de lo opaco.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    NubesAz(t);
    DibujarMallaAz(MALLA_AZ_CIELO);
    rlDrawRenderBatchActive();

    // Transparencias: azucar que cae, chispas, remolinos, estrellitas, confeti.
    rlDisableDepthMask();
    LoteAz();

    if (efectos)
    {
        NieveAzucarAz(t);
        if (decoracion) EspumaCascadaAz(t);
    }

    for (int i = 0; i < MAX_PARTICULAS_BOLAS_AZUCAR; i++)
    {
        const ParticulaTierra& p = particulas[i];
        if (!p.activa) continue;
        float u = p.vidaMaxima > 0.0f ? Lim01Az(p.vida / p.vidaMaxima) : 0.0f;
        float s = p.tamano * 1.6f;
        CartelAz(p.posicion, s, s, AlfaAz(p.color, 0.4f + 0.6f * u));
    }

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorBolasAzucar& estado = estadosJugadores[i];
        Color color = participantes[i].color;

        if (!estado.vivo)
        {
            float s = salidaVisual[i];
            EstallidoAz({ salidaX[i], 0.8f, salidaZ[i] }, s / 0.7f, color, i * 40 + 7, 18);
            Vector3 cima = { salidaX[i], 0.7f + 8.0f * 0.9f - 6.0f * 0.81f, salidaZ[i] - 2.6f * 0.9f };
            EstallidoAz(cima, (s - 0.85f) / 0.6f, color, i * 40 + 9, 26);
            continue;
        }

        const Vector3& p = jugadores[i].posicion;
        EstallidoAz({ p.x, p.y + 0.6f, p.z }, golpeVisual[i] / 0.6f, color, i * 40 + 11, 14);

        if (estado.creando > 0.0f && fase == FASE_BOLAS_JUGANDO)
        {
            float dx = jugadores[i].direccionMirada.x, dz = jugadores[i].direccionMirada.z;
            float l = std::sqrt(dx * dx + dz * dz);
            if (l < 0.01f) { dx = 0.0f; dz = -1.0f; l = 1.0f; }
            Vector3 centro = { p.x + dx / l * (0.45f + RADIO_MINIMO_BOLA), 0.0f, p.z + dz / l * (0.45f + RADIO_MINIMO_BOLA) };
            RemolinoCreacionAz(centro, Lim01Az(1.0f - estado.creando / DURACION_CREAR_BOLA), t);
        }
    }

    if (fase == FASE_BOLAS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        for (int i = 0; i < limite; i++)
        {
            if (resultado.participantes[i].participo && resultado.participantes[i].posicionFinal == 1 && estadosJugadores[i].vivo)
            {
                ConfetiAz(jugadores[i].posicion, tiempoFin, i * 50 + 3);
            }
        }
    }

    FinLoteAz();
    rlDrawRenderBatchActive();

    // Estrellitas del aturdido (aditivas) y luces.
    BeginBlendMode(BLEND_ADDITIVE);
    LoteAz();
    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo && estadosJugadores[i].vivo && estadosJugadores[i].aturdido > 0.0f)
        {
            const Vector3& p = jugadores[i].posicion;
            EstrellasAturdidoAz({ p.x, p.y + jugadores[i].tamano.y * 0.5f + 0.25f, p.z }, t);
        }
    }
    FinLoteAz();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();

    if (efectos)
    {
        LucesAz(t, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Cartelitos J1..J4 sobre cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !estadosJugadores[i].vivo)
        {
            continue;
        }

        Vector3 cabeza = { jugadores[i].posicion.x, jugadores[i].posicion.y + jugadores[i].tamano.y * 0.5f + 0.55f, jugadores[i].posicion.z };
        Vector2 p = GetWorldToScreen(cabeza, vista);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        DibujarEtiquetaUI(TextFormat("J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1), p, 0.0f, opciones);
    }

    AnunciosAz(*this, jugadores, participantes, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    HudAz(*this, participantes, limite);
}


const ResultadoMinijuego&
MinijuegoBolasAzucar::ObtenerResultado() const
{
    return resultado;
}
