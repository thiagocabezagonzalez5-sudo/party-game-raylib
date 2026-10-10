#include "Minigames/MinijuegoBolasAzucar.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


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
}


void MinijuegoBolasAzucar::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
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
    CargarPaqueteBolasAzucarRetro3D();
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

    ResolverImpactosBolas(*this, jugadores);

    if (ContarVivosBolas(*this) <= 1 || tiempoRestante <= 0.0f)
    {
        tiempoRestante = tiempoRestante < 0.0f ? 0.0f : tiempoRestante;
        FinalizarBolas(*this);
    }
}


//==================================================
// VISUAL
//==================================================
//
// GLB compartidos con pivotes originales. Los datos logicos siguen siendo
// la unica fuente de posiciones, radios y colisiones; fallback por pieza.
//==================================================


static void DibujarPiruletaBolas(float x, float z, Color color, int tipo)
{
    auto pieza = static_cast<ModeloBolasAzucar3D>(MODELO_AZUCAR_PIRULETA_ROSA + tipo);
    if (DibujarModeloBolasAzucarRetro3D(pieza, {x,0,z})) return;
    (DrawCylinder)({ x, 0.0f, z }, 0.12f, 0.12f, 3.6f, 6, Color{ 250, 245, 240, 255 });
    (DrawSphere)({ x, 4.4f, z }, 1.2f, color);
    (DrawCylinder)({ x, 4.35f, z }, 1.22f, 1.22f, 0.12f, 14, Fade(WHITE, 0.8f));
    (DrawSphere)({ x, 4.4f, z }, 0.5f, Fade(WHITE, 0.55f));
}


static void DibujarFondoBolas()
{
    if (!DibujarModeloBolasAzucarRetro3D(MODELO_AZUCAR_FONDO))
        DrawPlane({ 0.0f, -0.55f, 0.0f }, { 140.0f, 140.0f }, Color{ 246, 190, 214, 255 });

    // Montanas de nata con cereza.
    for (int i = 0; i < 3; i++)
    {
        float x = -22.0f + 20.0f * (float)i;
        if (DibujarModeloBolasAzucarRetro3D(MODELO_AZUCAR_MONTANA, {x,-.5f,-30})) continue;
        (DrawCylinder)({ x, -0.5f, -30.0f }, 0.0f, 8.0f, 11.0f, 10, Color{ 255, 250, 245, 255 });
        (DrawSphere)({ x, 10.8f, -30.0f }, 0.7f, Color{ 220, 40, 70, 255 });
        (DrawCylinder)({ x, 2.0f, -30.0f }, 4.2f, 4.8f, 0.4f, 10, Color{ 120, 70, 44, 255 });
    }

    // Arboles de piruleta.
    const Color colores[5] =
    {
        Color{ 240, 80, 140, 255 },
        Color{ 80, 190, 240, 255 },
        Color{ 250, 200, 60, 255 },
        Color{ 150, 110, 230, 255 },
        Color{ 110, 220, 150, 255 }
    };

    for (int i = 0; i < 7; i++)
    {
        float x = -20.0f + 6.5f * (float)i;
        DibujarPiruletaBolas(x, -16.0f, colores[i % 5], i % 5);
    }

    DibujarPiruletaBolas(-16.0f, 4.0f, colores[1], 1);
    DibujarPiruletaBolas(16.0f, -3.0f, colores[3], 3);
    DibujarPiruletaBolas(17.0f, 9.0f, colores[0], 0);

    // Columnas de caramelo.
    for (int i = 0; i < 4; i++)
    {
        float x = (i % 2 == 0 ? -12.5f : 12.5f);
        float z = (i < 2 ? -12.0f : 12.0f);
        if (DibujarModeloBolasAzucarRetro3D(MODELO_AZUCAR_COLUMNA, {x,0,z})) continue;
        (DrawCylinder)({ x, 0.0f, z }, 0.8f, 0.9f, 6.0f, 10, Fade(Color{ 235, 150, 40, 255 }, 0.85f));
        (DrawCylinder)({ x, 6.0f, z }, 1.1f, 1.1f, 0.3f, 10, Color{ 255, 200, 90, 255 });
    }
}


static void DibujarArenaBolas(const MinijuegoBolasAzucar& minijuego)
{
    // Suelo de galleta con pepitas de chocolate.
    if (!DibujarSueloBolasAzucarRetro3D())
        (DrawCube)({ 0.0f, -0.25f, 0.0f }, MITAD_ARENA_BOLAS * 2.0f, 0.5f, MITAD_ARENA_BOLAS * 2.0f, Color{ 208, 152, 92, 255 });

    for (int i = 0; i < 28; i++)
    {
        float x = -8.4f + (float)((i * 37) % 168) / 10.0f;
        float z = -8.4f + (float)((i * 53) % 168) / 10.0f;

        if (!EnAzucarBolas(x, z) && !EnChocolateBolas(x, z))
        {
            if (!DibujarModeloBolasAzucarRetro3D(MODELO_AZUCAR_PEPITA, {x,0,z}))
                (DrawCylinder)({ x, 0.0f, z }, 0.16f, 0.16f, 0.04f, 8, Color{ 94, 56, 36, 255 });
        }
    }

    for (const RectanguloAzucarBolas& zona : ZONAS_AZUCAR_BOLAS)
    {
        auto pieza = zona.zMax - zona.zMin > zona.xMax - zona.xMin
            ? MODELO_AZUCAR_AZUCAR_LATERAL : MODELO_AZUCAR_AZUCAR_EXTREMO;
        if (DibujarModeloBolasAzucarRetro3D(pieza,
            {(zona.xMin+zona.xMax)*.5f, 0, (zona.zMin+zona.zMax)*.5f})) continue;
        (DrawCube)(
            { (zona.xMin + zona.xMax) * 0.5f, 0.02f, (zona.zMin + zona.zMax) * 0.5f },
            zona.xMax - zona.xMin,
            0.05f,
            zona.zMax - zona.zMin,
            Color{ 252, 250, 255, 255 }
        );
    }

    for (int i = 0; i < 3; i++)
    {
        auto pieza = i == 0 ? MODELO_AZUCAR_CHOCOLATE_GRANDE : MODELO_AZUCAR_CHOCOLATE_PEQUENO;
        if (DibujarModeloBolasAzucarRetro3D(pieza, {CHARCOS_BOLAS[i].x,0,CHARCOS_BOLAS[i].z})) continue;
        (DrawCylinder)({ CHARCOS_BOLAS[i].x, 0.0f, CHARCOS_BOLAS[i].z }, CHARCOS_BOLAS[i].radio, CHARCOS_BOLAS[i].radio, 0.06f, 20, Color{ 70, 38, 22, 255 });
        (DrawCylinder)({ CHARCOS_BOLAS[i].x, 0.06f, CHARCOS_BOLAS[i].z }, CHARCOS_BOLAS[i].radio * 0.55f, CHARCOS_BOLAS[i].radio * 0.55f, 0.01f, 16, Color{ 108, 62, 40, 255 });
    }

    // Muros de galleta con glaseado.
    for (int i = 1; i <= 4; i++)
    {
        const BloquePrueba& muro = minijuego.bloques[i];
        Vector3 pie = {muro.posicion.x, muro.posicion.y-muro.tamano.y*.5f, muro.posicion.z};
        float giro = muro.tamano.z > muro.tamano.x ? 90.0f : 0.0f;
        if (DibujarModeloBolasAzucarRetro3D(MODELO_AZUCAR_MURO, pie, giro)) continue;
        (DrawCube)(muro.posicion, muro.tamano.x, muro.tamano.y, muro.tamano.z, Color{ 190, 132, 82, 255 });
        (DrawCube)(
            { muro.posicion.x, muro.posicion.y + muro.tamano.y * 0.5f + 0.05f, muro.posicion.z },
            muro.tamano.x + 0.05f,
            0.12f,
            muro.tamano.z + 0.05f,
            Color{ 255, 226, 238, 255 }
        );
    }

    // Gominolas gigantes.
    const Color coloresGominola[4] =
    {
        Color{ 255, 70, 100, 255 },
        Color{ 90, 210, 90, 255 },
        Color{ 255, 190, 40, 255 },
        Color{ 70, 150, 255, 255 }
    };

    for (int i = 0; i < 4; i++)
    {
        const CirculoBolas& gominola = GOMINOLAS_BOLAS[i];
        auto pieza = static_cast<ModeloBolasAzucar3D>(MODELO_AZUCAR_GOMINOLA_ROJA + i);
        if (DibujarModeloBolasAzucarRetro3D(pieza, {gominola.x,0,gominola.z})) continue;
        (DrawCylinder)({ gominola.x, 0.0f, gominola.z }, gominola.radio, gominola.radio, 1.0f, 14, Fade(coloresGominola[i], 0.9f));
        (DrawSphere)({ gominola.x, 1.0f, gominola.z }, gominola.radio, Fade(coloresGominola[i], 0.9f));
        (DrawSphere)({ gominola.x - 0.25f, 1.3f, gominola.z - 0.2f }, 0.18f, Fade(WHITE, 0.6f));
    }
}


static void DibujarBolaBolas(const BolaAzucar& bola, Color colorDuenio, bool debug)
{
    float alpha = bola.estado == BOLA_AZUCAR_REPOSO
        ? 1.0f - bola.inactiva / 3.0f * 0.6f
        : 1.0f;

    if (!DibujarBolaAzucarRetro3D({bola.x,bola.radio,bola.z}, bola.radio, bola.giro, alpha, colorDuenio))
    {
        (DrawSphere)({ bola.x, bola.radio, bola.z }, bola.radio, Fade(Color{ 252, 250, 255, 255 }, alpha));
        DrawSphereWires({ bola.x, bola.radio, bola.z }, bola.radio * 1.02f, 6, 6, Fade(colorDuenio, 0.7f * alpha));
    }
    // Marcador procedural del giro; sigue siendo un indicador del duenio.
    (DrawSphere)(
        { bola.x + std::cos(bola.giro) * bola.radio * 0.6f, bola.radio + std::sin(bola.giro) * bola.radio * 0.6f, bola.z },
        bola.radio * 0.12f,
        Fade(colorDuenio, alpha)
    );

    if (debug)
    {
        DrawSphereWires({ bola.x, bola.radio, bola.z }, bola.radio + 0.45f, 8, 8, RED);
    }
}


void MinijuegoBolasAzucar::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 255, 214, 230, 255 });
    BeginMode3D(camara);

    DibujarFondoBolas();
    DibujarArenaBolas(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (bolas[i].activa)
        {
            DibujarBolaBolas(bolas[i], participantes[i].color, mostrarDebug);
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || !estadosJugadores[i].vivo)
        {
            continue;
        }

        JugadorPrueba visual = jugadores[i];
        visual.aplastado = estadosJugadores[i].aturdido > 0.0f || estadosJugadores[i].creando > 0.0f;
        visual.tiempoAplastado = 0.3f;

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (estadosJugadores[i].inmunidad > 0.0f && std::fmod(tiempoAnimacion * 12.0f, 2.0f) < 1.0f)
        {
            DrawCubeWires(visual.posicion, 1.0f, 1.6f, 1.0f, WHITE);
        }

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_BOLAS_AZUCAR);

    EndMode3D();

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Vidas sobre cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || !estadosJugadores[i].vivo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.3f, jugadores[i].posicion.z },
            camara
        );

        for (int v = 0; v < VIDAS_BOLAS_AZUCAR; v++)
        {
            int cx = (int)pantalla.x - 16 + v * 16;
            int cy = (int)pantalla.y;

            if (v < estadosJugadores[i].vidas)
                DrawCircle(cx, cy, 6.0f, Color{ 255, 90, 130, 255 });
            else
                DrawCircleLines(cx, cy, 6.0f, GRAY);
        }

        const char* etiqueta = TextFormat("J%d", participantes[i].numeroJugador);
        DrawText(etiqueta, (int)pantalla.x - MeasureText(etiqueta, 16) / 2, (int)pantalla.y - 22, 16, participantes[i].color);
    }

    // Titulo arriba y ayuda abajo: el panel grande tapaba al jugador noroeste.
    DrawRectangle(18, 16, 430, 44, Fade(BLACK, 0.79f));
    DrawText("BOLAS DE AZUCAR", 32, 24, 28, GOLD);

    {
        const char* ayuda[3] =
        {
            "MOVER + ACCION (E / SHIFT DER / B): CREAR BOLA, LUEGO LANZAR",
            "EMPUJA LA BOLA POR EL AZUCAR (BLANCO) PARA QUE CREZCA",
            "CHOCOLATE: DERRITE Y RALENTIZA  |  GOMINOLAS: REBOTAN"
        };

        DrawRectangle(0, alto - 68, ancho, 68, Fade(BLACK, 0.55f));

        for (int k = 0; k < 3; k++)
        {
            DrawText(
                ayuda[k],
                ancho / 2 - MeasureText(ayuda[k], 16) / 2,
                alto - 62 + k * 19,
                16,
                RAYWHITE
            );
        }
    }

    if (fase != FASE_BOLAS_PREPARACION)
    {
        DrawText(
            TextFormat("TIEMPO %.0f", tiempoRestante > 0.0f ? tiempoRestante : 0.0f),
            ancho - 190,
            24,
            23,
            tiempoRestante <= 10.0f ? RED : GOLD
        );
        DrawText(
            TextFormat("EN PIE: %d", ContarVivosBolas(*this)),
            ancho - 190,
            54,
            20,
            RAYWHITE
        );
    }

    if (mensajeTiempo > 0.0f && mensajeJugador >= 0 && fase == FASE_BOLAS_JUGANDO)
    {
        int numero = participantes[mensajeJugador].numeroJugador;
        const char* mensaje = nullptr;

        if (mensajeTipo == 1) mensaje = TextFormat("J%d PIERDE UNA VIDA", numero);
        else if (mensajeTipo == 2) mensaje = TextFormat("J%d QUEDA ATURDIDO", numero);
        else if (mensajeTipo == 3) mensaje = TextFormat("J%d ELIMINADO", numero);

        if (mensaje != nullptr)
        {
            DrawText(mensaje, ancho / 2 - MeasureText(mensaje, 24) / 2, alto - 100, 24, YELLOW);
        }
    }

    if (fase == FASE_BOLAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            ancho / 2 - MeasureText(texto, 88) / 2,
            alto / 2 - 54,
            88,
            GOLD
        );
    }
    else if (
        fase == FASE_BOLAS_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR)
        {
            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (
                    resultado.participantes[i].participo &&
                    resultado.participantes[i].posicionFinal == 1
                )
                {
                    titulo = TextFormat("GANA J%d", participantes[i].numeroJugador);
                }
            }
        }

        DrawRectangle(ancho / 2 - 290, alto / 2 - 92, 580, 184, Fade(BLACK, 0.91f));
        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, 34) / 2,
            alto / 2 - 51,
            34,
            GOLD
        );
        DrawText(
            TextoReinicioMinijuego(),
            ancho / 2 - MeasureText(TextoReinicioMinijuego(), 21) / 2,
            alto / 2 + 25,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoBolasAzucar::ObtenerResultado() const
{
    return resultado;
}
