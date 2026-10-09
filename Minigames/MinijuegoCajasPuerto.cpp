#include "Minigames/MinijuegoCajasPuerto.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


//==================================================
// CAJAS DEL PUERTO
//==================================================
//
// 1 vs 3. El trio se esconde en contenedores numerados mientras el
// solitario (en la cabina de la grua) mira hacia otro lado. Luego la grua
// deja caer cargas "chispa" sobre contenedores elegidos.
//==================================================


static const float DURACION_PREPARACION_CAJAS = 3.0f;
static const float DURACION_ESCONDER_CAJAS = 6.0f;
static const float DURACION_CIERRE_CAJAS = 1.0f;
static const float DURACION_ELEGIR_CAJAS = 5.0f;
static const float DURACION_PASO_RESOLUCION_CAJAS = 1.1f;
static const float DURACION_REVELACION_CAJAS = 2.0f;
static const float ESPACIADO_CAJAS = 2.1f;
static const float Z_CONTENEDORES_CAJAS = -2.5f;
static const float Z_FRENTE_CONTENEDORES_CAJAS = -1.5f;
static const float LIMITE_X_CAJAS = 9.6f;
static const float Z_MINIMO_CAJAS = -1.0f;
static const float Z_MAXIMO_CAJAS = 5.5f;
static const float VELOCIDAD_JUGADOR_CAJAS = 5.0f;
static const float ALTURA_GANCHO_CAJAS = 4.2f;
static const int ELECCIONES_POR_RONDA_CAJAS[RONDAS_CAJAS] = { 2, 2, 3 };


//==================================================
// LOGICA
//==================================================


static float LimitarCajas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AcercarCajas(float valor, float objetivo, float paso)
{
    if (valor < objetivo - paso) return valor + paso;
    if (valor > objetivo + paso) return valor - paso;
    return objetivo;
}


static float PosicionXContenedorCajas(int indice)
{
    return ((float)indice - 3.5f) * ESPACIADO_CAJAS;
}


static bool EsControlBotCajas(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static int ContarVivosCajas(const MinijuegoCajasPuerto& minijuego)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static void FinalizarCajas(MinijuegoCajasPuerto& minijuego, bool ganaSolo)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;

        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego = esSolo
            ? minijuego.eliminadosTotal * 100
            : minijuego.estadosJugadores[i].rondasSobrevividas * 100;
    }

    minijuego.fase = FASE_CAJAS_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void IniciarRondaCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[]
)
{
    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        ContenedorCajasPuerto& contenedor = minijuego.contenedores[c];
        contenedor.ocupante = -1;
        contenedor.elegido = false;
        contenedor.revelado = false;
        contenedor.reforzado = false;
        contenedor.caida = 0.0f;
        contenedor.humo = 0.0f;
    }

    minijuego.contenedores[
        GetRandomValue(0, CANTIDAD_CONTENEDORES_CAJAS - 1)
    ].reforzado = true;

    const float spawnsX[MAX_PARTICIPANTES] = { -4.5f, -1.5f, 1.5f, 4.5f };
    int cursorSpawn = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCajasPuerto& estado = minijuego.estadosJugadores[i];
        estado.contenedor = -1;
        estado.accionPrevia = false;
        estado.objetivoBot = -1;
        estado.retrasoBot = GetRandomValue(80, 430) / 100.0f;

        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            !estado.vivo
        )
        {
            continue;
        }

        jugadores[i].posicion =
        {
            spawnsX[cursorSpawn % MAX_PARTICIPANTES],
            jugadores[i].tamano.y * 0.5f,
            4.0f
        };
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugadores[i].enSuelo = true;
        jugadores[i].cayendo = false;
        jugadores[i].aplastado = false;
        cursorSpawn++;
    }

    minijuego.fase = FASE_CAJAS_ESCONDER;
    minijuego.tiempoFase = DURACION_ESCONDER_CAJAS;
    minijuego.cursor = 3;
    minijuego.eleccionesRestantes = ELECCIONES_POR_RONDA_CAJAS[minijuego.ronda - 1];
    minijuego.cantidadElegidos = 0;
    minijuego.eliminadosRonda = 0;
    minijuego.pasoResolucion = 0;
    minijuego.tiempoPaso = 0.0f;
    minijuego.tiempoFinResolucion = 0.0f;
    minijuego.caidaIniciada = false;
    minijuego.detonado = false;
    minijuego.revelacionHecha = false;
    minijuego.botSoloObjetivo = -1;
    minijuego.botSoloTemporizador = 0.6f;
    minijuego.direccionPrevia = 0;
    minijuego.repeticionCursor = 0.0f;
}


static void EntrarContenedorCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[],
    int indiceJugador,
    int indiceContenedor,
    bool conSonido
)
{
    minijuego.estadosJugadores[indiceJugador].contenedor = indiceContenedor;
    minijuego.contenedores[indiceContenedor].ocupante = indiceJugador;

    Vector3 centro = minijuego.contenedores[indiceContenedor].posicion;
    jugadores[indiceJugador].posicion =
    {
        centro.x,
        jugadores[indiceJugador].tamano.y * 0.5f,
        centro.z
    };

    if (conSonido)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);
    }
}


static int ContenedorMasCercanoLibreCajas(
    const MinijuegoCajasPuerto& minijuego,
    const JugadorPrueba& jugador
)
{
    int mejor = -1;
    float mejorDistancia = 1000000.0f;

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        if (minijuego.contenedores[c].ocupante >= 0)
        {
            continue;
        }

        float dx = jugador.posicion.x - minijuego.contenedores[c].posicion.x;
        float dz = jugador.posicion.z - minijuego.contenedores[c].posicion.z;
        float distancia = dx * dx + dz * dz;

        if (distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
            mejor = c;
        }
    }

    return mejor;
}


static void IntentarEntrarCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[],
    int indiceJugador
)
{
    const JugadorPrueba& jugador = jugadores[indiceJugador];

    int indice = (int)std::lround(jugador.posicion.x / ESPACIADO_CAJAS + 3.5f);
    if (indice < 0) indice = 0;
    if (indice >= CANTIDAD_CONTENEDORES_CAJAS) indice = CANTIDAD_CONTENEDORES_CAJAS - 1;

    float dx = std::fabs(
        jugador.posicion.x - minijuego.contenedores[indice].posicion.x
    );

    if (dx > 1.0f || jugador.posicion.z > 0.9f)
    {
        return;
    }

    if (minijuego.contenedores[indice].ocupante >= 0)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        minijuego.estadosJugadores[indiceJugador].objetivoBot = -1;
        return;
    }

    EntrarContenedorCajas(minijuego, jugadores, indiceJugador, indice, true);
}


static int ElegirContenedorBotCajas(
    const MinijuegoCajasPuerto& minijuego,
    const JugadorPrueba& jugador
)
{
    int libres[CANTIDAD_CONTENEDORES_CAJAS]{};
    int cantidad = 0;

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        if (minijuego.contenedores[c].ocupante < 0)
        {
            libres[cantidad] = c;
            cantidad++;
        }
    }

    if (cantidad == 0)
    {
        return -1;
    }

    // Sesgo leve a los alejados: de dos candidatos al azar, el mas lejano.
    int a = libres[GetRandomValue(0, cantidad - 1)];
    int b = libres[GetRandomValue(0, cantidad - 1)];
    float distanciaA = std::fabs(jugador.posicion.x - minijuego.contenedores[a].posicion.x);
    float distanciaB = std::fabs(jugador.posicion.x - minijuego.contenedores[b].posicion.x);

    return distanciaA >= distanciaB ? a : b;
}


static InputMinijuegoParticipante CrearEntradaBotEsconderCajas(
    MinijuegoCajasPuerto& minijuego,
    int indiceJugador,
    const JugadorPrueba& jugador
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorCajasPuerto& estado = minijuego.estadosJugadores[indiceJugador];

    if (
        estado.objetivoBot < 0 ||
        minijuego.contenedores[estado.objetivoBot].ocupante >= 0
    )
    {
        estado.objetivoBot = ElegirContenedorBotCajas(minijuego, jugador);
    }

    if (estado.objetivoBot < 0)
    {
        return entrada;
    }

    float objetivoX = minijuego.contenedores[estado.objetivoBot].posicion.x;
    float objetivoZ = 0.2f;

    if (jugador.posicion.x < objetivoX - 0.2f)
        entrada.derecha = true;
    else if (jugador.posicion.x > objetivoX + 0.2f)
        entrada.izquierda = true;

    if (jugador.posicion.z > objetivoZ + 0.15f)
        entrada.adelante = true;

    float transcurrido = DURACION_ESCONDER_CAJAS - minijuego.tiempoFase;

    if (
        !entrada.izquierda &&
        !entrada.derecha &&
        !entrada.adelante &&
        transcurrido >= estado.retrasoBot
    )
    {
        entrada.golpear = true;
    }

    return entrada;
}


static void ActualizarEsconderCajas(
    MinijuegoCajasPuerto& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCajasPuerto& estado = minijuego.estadosJugadores[i];

        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            !estado.vivo ||
            estado.contenedor >= 0
        )
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        InputMinijuegoParticipante entrada{};

        if (EsControlBotCajas(participantes[i]))
        {
            entrada = CrearEntradaBotEsconderCajas(minijuego, i, jugador);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        float moverX =
            (entrada.derecha ? 1.0f : 0.0f) -
            (entrada.izquierda ? 1.0f : 0.0f);
        float moverZ =
            (entrada.atras ? 1.0f : 0.0f) -
            (entrada.adelante ? 1.0f : 0.0f);
        float longitud = std::sqrt(moverX * moverX + moverZ * moverZ);

        if (longitud > 0.01f)
        {
            moverX /= longitud;
            moverZ /= longitud;
            jugador.posicion.x += moverX * VELOCIDAD_JUGADOR_CAJAS * deltaTime;
            jugador.posicion.z += moverZ * VELOCIDAD_JUGADOR_CAJAS * deltaTime;
            jugador.direccionMirada = { moverX, 0.0f, moverZ };
        }

        jugador.posicion.x = LimitarCajas(jugador.posicion.x, -LIMITE_X_CAJAS, LIMITE_X_CAJAS);
        jugador.posicion.z = LimitarCajas(jugador.posicion.z, Z_MINIMO_CAJAS, Z_MAXIMO_CAJAS);
        jugador.posicion.y = jugador.tamano.y * 0.5f;
        jugador.enSuelo = true;
        jugador.cayendo = false;

        bool accion = entrada.golpear && !estado.accionPrevia;
        estado.accionPrevia = entrada.golpear;

        if (accion)
        {
            IntentarEntrarCajas(minijuego, jugadores, i);
        }
    }

    bool todosEscondidos = true;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo &&
            minijuego.estadosJugadores[i].contenedor < 0
        )
        {
            todosEscondidos = false;
        }
    }

    float antes = minijuego.tiempoFase;
    minijuego.tiempoFase -= deltaTime;
    ActualizarAudioAlertaTiempo(minijuego.audio, antes, minijuego.tiempoFase);

    if (!todosEscondidos && minijuego.tiempoFase > 0.0f)
    {
        return;
    }

    // Quien no se escondio queda dentro del contenedor libre mas cercano.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            !minijuego.estadosJugadores[i].vivo ||
            minijuego.estadosJugadores[i].contenedor >= 0
        )
        {
            continue;
        }

        int libre = ContenedorMasCercanoLibreCajas(minijuego, jugadores[i]);

        if (libre >= 0)
        {
            EntrarContenedorCajas(minijuego, jugadores, i, libre, false);
        }
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);
    minijuego.fase = FASE_CAJAS_CIERRE;
    minijuego.tiempoFase = DURACION_CIERRE_CAJAS;
}


static void IniciarResolucionCajas(MinijuegoCajasPuerto& minijuego)
{
    minijuego.fase = FASE_CAJAS_RESOLVER;
    minijuego.pasoResolucion = 0;
    minijuego.tiempoPaso = 0.0f;
    minijuego.tiempoFinResolucion = 0.0f;
    minijuego.caidaIniciada = false;
    minijuego.detonado = false;
    minijuego.revelacionHecha = false;
}


static void ConfirmarEleccionCajas(MinijuegoCajasPuerto& minijuego)
{
    int c = minijuego.cursor;

    if (
        minijuego.eleccionesRestantes <= 0 ||
        minijuego.cantidadElegidos >= MAX_ELECCIONES_CAJAS ||
        minijuego.contenedores[c].elegido
    )
    {
        return;
    }

    minijuego.contenedores[c].elegido = true;
    minijuego.historialElegidos[c] = true;
    minijuego.ordenElegidos[minijuego.cantidadElegidos] = c;
    minijuego.cantidadElegidos++;
    minijuego.eleccionesRestantes--;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);

    if (minijuego.eleccionesRestantes <= 0)
    {
        IniciarResolucionCajas(minijuego);
    }
}


static void ActualizarSoloBotCajas(MinijuegoCajasPuerto& minijuego, float deltaTime)
{
    if (
        minijuego.botSoloObjetivo < 0 ||
        minijuego.contenedores[minijuego.botSoloObjetivo].elegido
    )
    {
        int candidatos[CANTIDAD_CONTENEDORES_CAJAS]{};
        int cantidad = 0;

        for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
        {
            if (!minijuego.contenedores[c].elegido && !minijuego.historialElegidos[c])
            {
                candidatos[cantidad] = c;
                cantidad++;
            }
        }

        if (cantidad == 0)
        {
            for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
            {
                if (!minijuego.contenedores[c].elegido)
                {
                    candidatos[cantidad] = c;
                    cantidad++;
                }
            }
        }

        if (cantidad == 0)
        {
            return;
        }

        minijuego.botSoloObjetivo = candidatos[GetRandomValue(0, cantidad - 1)];
        minijuego.botSoloTemporizador = 0.2f;
    }

    minijuego.botSoloTemporizador -= deltaTime;

    if (minijuego.botSoloTemporizador > 0.0f)
    {
        return;
    }

    if (minijuego.cursor < minijuego.botSoloObjetivo)
    {
        minijuego.cursor++;
        minijuego.botSoloTemporizador = 0.11f;
    }
    else if (minijuego.cursor > minijuego.botSoloObjetivo)
    {
        minijuego.cursor--;
        minijuego.botSoloTemporizador = 0.11f;
    }
    else
    {
        minijuego.botSoloObjetivo = -1;
        ConfirmarEleccionCajas(minijuego);
        minijuego.botSoloTemporizador = 0.25f;
    }
}


static void ActualizarElegirCajas(
    MinijuegoCajasPuerto& minijuego,
    float deltaTime,
    Participante participantes[]
)
{
    float antes = minijuego.tiempoFase;
    minijuego.tiempoFase -= deltaTime;
    ActualizarAudioAlertaTiempo(minijuego.audio, antes, minijuego.tiempoFase);

    const Participante& participanteSolo = participantes[minijuego.indiceSolo];

    if (EsControlBotCajas(participanteSolo))
    {
        ActualizarSoloBotCajas(minijuego, deltaTime);
    }
    else
    {
        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participanteSolo);

        int direccion = (entrada.derecha ? 1 : 0) - (entrada.izquierda ? 1 : 0);

        if (direccion != 0)
        {
            minijuego.repeticionCursor -= deltaTime;

            if (direccion != minijuego.direccionPrevia || minijuego.repeticionCursor <= 0.0f)
            {
                int nuevo = minijuego.cursor + direccion;

                if (nuevo >= 0 && nuevo < CANTIDAD_CONTENEDORES_CAJAS)
                {
                    minijuego.cursor = nuevo;
                }

                minijuego.repeticionCursor =
                    direccion != minijuego.direccionPrevia ? 0.28f : 0.14f;
            }
        }
        else
        {
            minijuego.repeticionCursor = 0.0f;
        }

        minijuego.direccionPrevia = direccion;

        if (entrada.golpear && !minijuego.accionSoloPrevia)
        {
            ConfirmarEleccionCajas(minijuego);
        }

        minijuego.accionSoloPrevia = entrada.golpear;
    }

    if (minijuego.fase != FASE_CAJAS_ELEGIR || minijuego.tiempoFase > 0.0f)
    {
        return;
    }

    // Se acabo el tiempo: la grua completa sus cargas al azar.
    for (int intento = 0; intento < 64 && minijuego.fase == FASE_CAJAS_ELEGIR; intento++)
    {
        int c = GetRandomValue(0, CANTIDAD_CONTENEDORES_CAJAS - 1);

        if (!minijuego.contenedores[c].elegido)
        {
            minijuego.cursor = c;
            ConfirmarEleccionCajas(minijuego);
        }
    }

    if (minijuego.fase == FASE_CAJAS_ELEGIR)
    {
        IniciarResolucionCajas(minijuego);
    }
}


static void DetonarContenedorCajas(MinijuegoCajasPuerto& minijuego, int indice)
{
    ContenedorCajasPuerto& contenedor = minijuego.contenedores[indice];
    contenedor.humo = 1.0f;
    contenedor.marcas++;
    contenedor.revelado = true;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_EXPLOSION);

    if (contenedor.ocupante < 0)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
        return;
    }

    if (contenedor.reforzado)
    {
        // El ancla dorada protege al ocupante.
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
        return;
    }

    minijuego.estadosJugadores[contenedor.ocupante].vivo = false;
    minijuego.eliminadosRonda++;
    minijuego.eliminadosTotal++;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
}


static void FinalizarRondaCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[]
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo
        )
        {
            minijuego.estadosJugadores[i].rondasSobrevividas++;
            vivos++;
        }
    }

    if (vivos == 0)
    {
        FinalizarCajas(minijuego, true);
    }
    else if (minijuego.ronda >= RONDAS_CAJAS)
    {
        FinalizarCajas(minijuego, false);
    }
    else
    {
        minijuego.ronda++;
        IniciarRondaCajas(minijuego, jugadores);
    }
}


static void ActualizarResolverCajas(
    MinijuegoCajasPuerto& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[]
)
{
    if (minijuego.pasoResolucion < minijuego.cantidadElegidos)
    {
        int c = minijuego.ordenElegidos[minijuego.pasoResolucion];
        minijuego.tiempoPaso += deltaTime;

        if (!minijuego.caidaIniciada && minijuego.tiempoPaso >= 0.35f)
        {
            minijuego.caidaIniciada = true;
            minijuego.contenedores[c].caida = 0.4f;
        }

        if (
            minijuego.caidaIniciada &&
            !minijuego.detonado &&
            minijuego.tiempoPaso >= 0.75f
        )
        {
            minijuego.detonado = true;
            DetonarContenedorCajas(minijuego, c);
        }

        if (minijuego.tiempoPaso >= DURACION_PASO_RESOLUCION_CAJAS)
        {
            minijuego.pasoResolucion++;
            minijuego.tiempoPaso = 0.0f;
            minijuego.caidaIniciada = false;
            minijuego.detonado = false;
        }

        return;
    }

    if (!minijuego.revelacionHecha)
    {
        minijuego.revelacionHecha = true;

        for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
        {
            minijuego.contenedores[c].revelado = true;
        }
    }

    minijuego.tiempoFinResolucion += deltaTime;

    if (minijuego.tiempoFinResolucion >= DURACION_REVELACION_CAJAS)
    {
        FinalizarRondaCajas(minijuego, jugadores);
    }
}


static void ActualizarEfectosCajas(MinijuegoCajasPuerto& minijuego, float deltaTime)
{
    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        ContenedorCajasPuerto& contenedor = minijuego.contenedores[c];

        bool abierto =
            minijuego.fase == FASE_CAJAS_PREPARACION ||
            minijuego.fase == FASE_CAJAS_ESCONDER ||
            contenedor.revelado;

        contenedor.apertura = AcercarCajas(
            contenedor.apertura,
            abierto ? 1.0f : 0.0f,
            2.2f * deltaTime
        );

        if (contenedor.humo > 0.0f)
        {
            contenedor.humo -= 0.75f * deltaTime;
            if (contenedor.humo < 0.0f) contenedor.humo = 0.0f;
        }

        if (contenedor.caida > 0.0f)
        {
            contenedor.caida -= deltaTime;
            if (contenedor.caida < 0.0f) contenedor.caida = 0.0f;
        }
    }

    float objetivoX = 0.0f;

    if (minijuego.fase == FASE_CAJAS_ELEGIR)
    {
        objetivoX = minijuego.contenedores[minijuego.cursor].posicion.x;
    }
    else if (
        minijuego.fase == FASE_CAJAS_RESOLVER &&
        minijuego.pasoResolucion < minijuego.cantidadElegidos
    )
    {
        objetivoX = minijuego.contenedores[
            minijuego.ordenElegidos[minijuego.pasoResolucion]
        ].posicion.x;
    }

    minijuego.ganchoX = AcercarCajas(minijuego.ganchoX, objetivoX, 16.0f * deltaTime);
}


static void ColocarSoloCajas(MinijuegoCajasPuerto& minijuego, JugadorPrueba jugadores[])
{
    if (minijuego.indiceSolo < 0)
    {
        return;
    }

    JugadorPrueba& jugadorSolo = jugadores[minijuego.indiceSolo];
    jugadorSolo.posicion = { minijuego.ganchoX, 6.5f, -5.0f };

    bool mirandoAfuera =
        minijuego.fase == FASE_CAJAS_PREPARACION ||
        minijuego.fase == FASE_CAJAS_ESCONDER ||
        minijuego.fase == FASE_CAJAS_CIERRE;

    jugadorSolo.direccionMirada = mirandoAfuera
        ? Vector3{ 0.0f, 0.0f, -1.0f }
        : Vector3{ 0.0f, 0.0f, 1.0f };
    jugadorSolo.enSuelo = true;
    jugadorSolo.cayendo = false;
}


//==================================================
// INTERFAZ
//==================================================


void MinijuegoCajasPuerto::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        contenedores[c] = {};
        contenedores[c].posicion =
        {
            PosicionXContenedorCajas(c),
            1.1f,
            Z_CONTENEDORES_CAJAS
        };
        historialElegidos[c] = false;
    }

    for (int i = 0; i < MAX_ELECCIONES_CAJAS; i++)
    {
        ordenElegidos[i] = 0;
    }

    camara.position = { 0.0f, 10.5f, 12.5f };
    camara.target = { 0.0f, 3.2f, -1.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 54.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CAJAS_PREPARACION;
    indiceSolo = -1;
    ronda = 1;
    cursor = 3;
    eleccionesRestantes = 0;
    cantidadElegidos = 0;
    pasoResolucion = 0;
    eliminadosRonda = 0;
    eliminadosTotal = 0;
    direccionPrevia = 0;
    botSoloObjetivo = -1;
    accionSoloPrevia = false;
    caidaIniciada = false;
    detonado = false;
    revelacionHecha = false;
    ganchoX = 0.0f;
    repeticionCursor = 0.0f;
    botSoloTemporizador = 0.0f;
    tiempoPaso = 0.0f;
    tiempoFinResolucion = 0.0f;
    tiempoFase = DURACION_ESCONDER_CAJAS;
    tiempoPreparacion = DURACION_PREPARACION_CAJAS;
    tiempoAnimacion = 0.0f;
}


void MinijuegoCajasPuerto::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteCajasPuertoRetro3D());
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
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
        fase = FASE_CAJAS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool esSolo = i == indiceSolo;
        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;
        estadosJugadores[i].vivo = !esSolo;

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            Vector3{ 0.0f, 0.70f, 4.0f }
        );
    }

    IniciarRondaCajas(*this, jugadores);
    fase = FASE_CAJAS_PREPARACION;
    ColocarSoloCajas(*this, jugadores);
}


void MinijuegoCajasPuerto::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)cantidadMaxima;

    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_CAJAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    ActualizarEfectosCajas(*this, deltaTime);

    switch (fase)
    {
    case FASE_CAJAS_PREPARACION:
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CAJAS_ESCONDER;
            tiempoFase = DURACION_ESCONDER_CAJAS;
        }
        break;
    }

    case FASE_CAJAS_ESCONDER:
        ActualizarEsconderCajas(*this, deltaTime, jugadores, participantes);
        break;

    case FASE_CAJAS_CIERRE:
        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CAJAS_ELEGIR;
            tiempoFase = DURACION_ELEGIR_CAJAS;
            botSoloTemporizador = 0.6f;
            accionSoloPrevia = true;
        }
        break;

    case FASE_CAJAS_ELEGIR:
        ActualizarElegirCajas(*this, deltaTime, participantes);
        break;

    case FASE_CAJAS_RESOLVER:
        ActualizarResolverCajas(*this, deltaTime, jugadores);
        break;

    case FASE_CAJAS_TERMINADO:
        break;
    }

    ColocarSoloCajas(*this, jugadores);
}


//==================================================
// VISUAL
//==================================================
//
// El paquete compartido conserva pivotes y colores. Cada pieza que no pueda
// dibujarse mantiene sus primitivas originales, sin cambiar las reglas.
//==================================================


static Color OscurecerCajas(Color color, float factor)
{
    return Color{
        (unsigned char)((float)color.r * factor),
        (unsigned char)((float)color.g * factor),
        (unsigned char)((float)color.b * factor),
        255
    };
}


static Color ColorContenedorCajas(int indice)
{
    const Color colores[CANTIDAD_CONTENEDORES_CAJAS] =
    {
        Color{ 168, 62, 52, 255 },
        Color{ 52, 110, 166, 255 },
        Color{ 70, 140, 84, 255 },
        Color{ 196, 150, 56, 255 },
        Color{ 130, 76, 150, 255 },
        Color{ 60, 150, 156, 255 },
        Color{ 190, 98, 52, 255 },
        Color{ 112, 118, 130, 255 }
    };

    return colores[indice % CANTIDAD_CONTENEDORES_CAJAS];
}


static void DibujarFarolCajas(float x, float z)
{
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_FAROL, { x, 0.0f, z }))
    {
        DrawCylinder({ x, 1.6f, z }, 0.09f, 0.12f, 3.2f, 8, Color{ 52, 54, 60, 255 });
        DrawSphere({ x, 3.3f, z }, 0.22f, Color{ 255, 226, 140, 255 });
    }
    // El halo es un efecto: se conserva tambien con el GLB del farol.
    DrawSphere({ x, 3.3f, z }, 0.5f, Fade(Color{ 255, 220, 120, 255 }, 0.18f));
}


static void DibujarMuelleCajas()
{
    DrawPlane({ 0.0f, -0.9f, -4.0f }, { 120.0f, 90.0f }, Color{ 9, 22, 42, 255 });

    // Muelle de madera.
    bool muelleGLB = DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_MUELLE, { 0.0f, 0.0f, 0.75f });
    if (!muelleGLB)
        DrawCube({ 0.0f, -0.3f, 0.75f }, 22.0f, 0.6f, 14.0f, Color{ 86, 62, 42, 255 });

    for (int i = 0; !muelleGLB && i <= 14; i++)
    {
        float z = -6.0f + (float)i;
        DrawLine3D(
            { -11.0f, 0.01f, z },
            { 11.0f, 0.01f, z },
            Fade(Color{ 40, 28, 20, 255 }, 0.8f)
        );
    }

    for (int i = 0; i < 6; i++)
    {
        float x = -10.0f + 4.0f * (float)i;
        if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_BOLARDO, { x, 0.0f, 7.8f }))
            DrawCylinder({ x, -0.2f, 7.8f }, 0.22f, 0.22f, 0.9f, 8, Color{ 52, 40, 30, 255 });
    }

    // Barco al fondo.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_BARCO, { 0.0f, -0.9f, -17.0f }))
    {
        DrawCube({ 0.0f, 0.4f, -17.0f }, 34.0f, 3.4f, 6.0f, Color{ 52, 36, 44, 255 });
        DrawCube({ 0.0f, -0.4f, -17.0f }, 34.5f, 1.2f, 6.2f, Color{ 120, 40, 40, 255 });
        DrawCube({ 9.0f, 3.6f, -17.0f }, 6.0f, 3.0f, 4.2f, Color{ 150, 154, 160, 255 });
        DrawCylinder({ 10.5f, 6.4f, -17.0f }, 0.6f, 0.8f, 2.6f, 10, Color{ 190, 70, 50, 255 });

        for (int i = 0; i < 6; i++)
        {
            float x = -11.0f + 3.2f * (float)i;
            DrawCube(
                { x, 2.6f, -16.5f },
                2.4f,
                1.6f,
                1.8f,
                OscurecerCajas(ColorContenedorCajas(i), 0.55f)
            );
        }

        for (int i = 0; i < 4; i++)
        {
            DrawSphere(
                { 6.2f + 1.2f * (float)i, 4.3f, -14.9f },
                0.12f,
                Color{ 255, 232, 150, 255 }
            );
        }
    }

    // Misma pila y colores del dibujo original; una sola malla decorativa.
    const Vector3 posicionesDecoracion[] =
    {
        { -12.5f, 0.0f, -5.0f }, { -12.5f, 2.2f, -5.0f },
        { 12.5f, 0.0f, -5.0f }, { 12.5f, 2.2f, -5.0f }
    };
    const int coloresDecoracion[] = { 2, 0, 1, 3 };
    for (int i = 0; i < 4; i++)
    {
        Vector3 p = posicionesDecoracion[i];
        Color color = OscurecerCajas(ColorContenedorCajas(coloresDecoracion[i]), 0.5f);
        if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_CONTENEDOR_DECORACION,
            p, { 1.0f, 1.0f, 1.0f }, color))
            DrawCube({ p.x, p.y + 1.1f, p.z }, 1.9f, 2.2f, 4.0f, color);
    }

    DibujarFarolCajas(-10.5f, 1.0f);
    DibujarFarolCajas(10.5f, 1.0f);
    DibujarFarolCajas(-10.5f, 6.5f);
    DibujarFarolCajas(10.5f, 6.5f);
}


static void DibujarContenedorCajas(const ContenedorCajasPuerto& contenedor, int indice)
{
    Color color = ColorContenedorCajas(indice);
    Vector3 p = contenedor.posicion;
    float frente = p.z + 1.0f;

    // El estado guarda el centro Y=1.1; el cuerpo GLB guarda el suelo Y=0.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_CONTENEDOR_CUERPO,
        { p.x, p.y - 1.1f, p.z }, { 1.0f, 1.0f, 1.0f }, color))
    {
        DrawCube(p, 1.9f, 2.2f, 2.0f, color);
        DrawCubeWires(p, 1.9f, 2.2f, 2.0f, Fade(BLACK, 0.55f));
        DrawCube({ p.x, p.y + 1.12f, p.z }, 1.96f, 0.08f, 2.06f, OscurecerCajas(color, 0.7f));
        DrawCube({ p.x, p.y, frente + 0.01f }, 1.7f, 2.0f, 0.02f, Color{ 6, 6, 10, 255 });
    }

    float escalaPuerta = 1.0f - 0.88f * contenedor.apertura;
    float ancho = 0.85f * escalaPuerta;
    Color puerta = OscurecerCajas(color, 0.82f);
    // Los GLB v1 crecen hacia el lado de su nombre. Se usa el que crece +X
    // en la bisagra izquierda y el que crece -X en la derecha, para cerrar
    // hacia el centro. No se recentran ni se reflejan vertices/herrajes.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_PUERTA_DERECHA,
        { p.x - 0.85f, p.y, frente + 0.08f }, { escalaPuerta, 1.0f, 1.0f }, puerta))
        DrawCube({ p.x - 0.85f + ancho * 0.5f, p.y, frente + 0.06f }, ancho, 2.0f, 0.08f, puerta);
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_PUERTA_IZQUIERDA,
        { p.x + 0.85f, p.y, frente + 0.08f }, { escalaPuerta, 1.0f, 1.0f }, puerta))
        DrawCube({ p.x + 0.85f - ancho * 0.5f, p.y, frente + 0.06f }, ancho, 2.0f, 0.08f, puerta);

    if (contenedor.marcas > 0)
    {
        if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_MARCA_GOLPE, { p.x, p.y + 1.22f, p.z }))
            DrawCube({ p.x, p.y + 1.17f, p.z }, 1.3f, 0.03f, 1.3f, Color{ 18, 16, 16, 255 });
    }

    if (contenedor.reforzado && !DibujarModeloCajasPuertoRetro3D(
        MODELO_CAJAS_ANCLA, { p.x, p.y + 1.16f, p.z }))
    {
        Vector3 ancla = { p.x, p.y + 1.15f, p.z };
        DrawCube({ ancla.x, ancla.y + 0.3f, ancla.z }, 0.12f, 0.6f, 0.12f, GOLD);
        DrawCube({ ancla.x, ancla.y + 0.5f, ancla.z }, 0.5f, 0.1f, 0.12f, GOLD);
        DrawSphere({ ancla.x, ancla.y + 0.72f, ancla.z }, 0.12f, GOLD);
        DrawCube({ ancla.x - 0.22f, ancla.y + 0.08f, ancla.z }, 0.2f, 0.12f, 0.12f, GOLD);
        DrawCube({ ancla.x + 0.22f, ancla.y + 0.08f, ancla.z }, 0.2f, 0.12f, 0.12f, GOLD);
    }
}


static void DibujarHumoCajas(const ContenedorCajasPuerto& contenedor)
{
    if (contenedor.humo <= 0.0f)
    {
        return;
    }

    float t = 1.0f - contenedor.humo;
    Vector3 base = { contenedor.posicion.x, 2.4f + t * 1.4f, contenedor.posicion.z };

    if (contenedor.humo > 0.8f)
    {
        DrawSphere(base, 0.5f + t * 4.0f, Fade(Color{ 255, 190, 70, 255 }, 0.7f));
    }

    DrawSphere(base, 0.6f + t * 1.3f, Fade(Color{ 150, 150, 160, 255 }, contenedor.humo * 0.6f));
    DrawSphere(
        { base.x - 0.5f, base.y + 0.3f, base.z + 0.2f },
        0.45f + t,
        Fade(Color{ 110, 110, 120, 255 }, contenedor.humo * 0.55f)
    );
    DrawSphere(
        { base.x + 0.5f, base.y + 0.2f, base.z - 0.2f },
        0.4f + t,
        Fade(Color{ 130, 130, 140, 255 }, contenedor.humo * 0.55f)
    );
}


static void DibujarGruaCajas(const MinijuegoCajasPuerto& minijuego)
{
    Color acero = Color{ 224, 164, 40, 255 };
    float gx = minijuego.ganchoX;

    bool porticoGLB = DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_PORTICO, { 0.0f, 0.0f, -5.0f });
    for (int lado = -1; !porticoGLB && lado <= 1; lado += 2)
    {
        for (int fondo = 0; fondo < 2; fondo++)
        {
            DrawCube(
                { 10.3f * (float)lado, 2.55f, -5.0f - 1.2f * (float)fondo },
                0.5f,
                5.1f,
                0.5f,
                acero
            );
        }
    }

    if (!porticoGLB)
        DrawCube({ 0.0f, 5.35f, -5.0f }, 21.0f, 0.5f, 0.8f, acero);
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_CARRO, { gx, 5.7f, -5.0f }))
        DrawCube({ gx, 5.7f, -5.0f }, 2.4f, 0.2f, 1.8f, Color{ 70, 74, 82, 255 });
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_CABINA, { gx, 5.7f, -5.0f }))
        DrawCubeWires({ gx, 7.2f, -5.0f }, 2.4f, 2.8f, 1.8f, Fade(acero, 0.6f));

    // Brazo hacia los contenedores, cable y gancho.
    float bob = std::sin(minijuego.tiempoAnimacion * 3.0f) * 0.06f;
    float yGancho = ALTURA_GANCHO_CAJAS + bob;

    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_BRAZO, { gx, 5.55f, -5.0f }))
        DrawCube({ gx, 5.55f, -3.75f }, 0.3f, 0.2f, 2.6f, acero);
    // Cable unitario hacia -Y: mantener su diametro y mover solo el extremo
    // inferior mediante escala Y. Su origen sigue unido a la punta del brazo.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_CABLE,
        { gx, 5.55f, -2.5f }, { 1.0f, 5.55f - yGancho, 1.0f }))
        DrawCylinderEx({ gx, 5.55f, -2.5f }, { gx, yGancho, -2.5f }, 0.04f, 0.04f, 6, LIGHTGRAY);
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_GANCHO, { gx, yGancho, -2.5f }))
        DrawCube({ gx, yGancho - 0.15f, -2.5f }, 0.4f, 0.3f, 0.4f, Color{ 90, 94, 100, 255 });

    bool seleccionando =
        minijuego.fase == FASE_CAJAS_ELEGIR ||
        minijuego.fase == FASE_CAJAS_RESOLVER;

    if (seleccionando && minijuego.fase == FASE_CAJAS_ELEGIR)
    {
        DrawCubeWires(
            minijuego.contenedores[minijuego.cursor].posicion,
            2.1f,
            2.4f,
            2.2f,
            YELLOW
        );
    }

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        const ContenedorCajasPuerto& contenedor = minijuego.contenedores[c];

        if (contenedor.elegido && minijuego.fase == FASE_CAJAS_ELEGIR)
        {
            DrawCubeWires(contenedor.posicion, 2.0f, 2.3f, 2.1f, RED);
        }

        if (contenedor.caida > 0.0f)
        {
            float t = 1.0f - contenedor.caida / 0.4f;
            float y = yGancho - 0.5f + (2.6f - yGancho + 0.5f) * t;
            DrawSphere({ contenedor.posicion.x, y, contenedor.posicion.z }, 0.28f, Color{ 255, 120, 40, 255 });
        }
    }
}


void MinijuegoCajasPuerto::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 6, 10, 22, 255 });
    BeginMode3D(camara);

    DibujarMuelleCajas();

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        DibujarContenedorCajas(contenedores[c], c);

        if (mostrarDebug)
        {
            DrawCubeWires(contenedores[c].posicion, 1.9f, 2.2f, 2.0f, LIME);
        }
    }

    DibujarGruaCajas(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;

        if (i == indiceSolo)
        {
            DibujarJugadorCuboPrueba(jugadores[i], participanteVisual);
            continue;
        }

        const EstadoJugadorCajasPuerto& estado = estadosJugadores[i];
        JugadorPrueba visual = jugadores[i];

        if (estado.contenedor >= 0)
        {
            const ContenedorCajasPuerto& contenedor = contenedores[estado.contenedor];

            if (!contenedor.revelado)
            {
                continue;
            }

            visual.posicion = { contenedor.posicion.x, visual.tamano.y * 0.5f, contenedor.posicion.z + 0.2f };
            visual.aplastado = !estado.vivo;
            visual.tiempoAplastado = 1.0f;
        }
        else if (!estado.vivo)
        {
            continue;
        }

        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }
    }

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        DibujarHumoCajas(contenedores[c]);
    }

    // Niebla del muelle: tapa quien entra a cada contenedor.
    if (fase == FASE_CAJAS_ESCONDER)
    {
        DrawCube(
            { 0.0f, 1.6f, -0.1f },
            22.0f,
            3.4f,
            2.8f,
            Fade(Color{ 14, 24, 44, 255 }, 0.88f)
        );
    }

    EndMode3D();

    // Numeros de contenedor.
    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        Vector2 pantalla = GetWorldToScreen(
            { contenedores[c].posicion.x, 2.9f, contenedores[c].posicion.z + 1.0f },
            camara
        );
        const char* numero = TextFormat("%d", c + 1);
        Color color = contenedores[c].elegido ? RED : RAYWHITE;

        DrawText(
            numero,
            (int)pantalla.x - MeasureText(numero, 24) / 2,
            (int)pantalla.y - 12,
            24,
            color
        );
    }

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangle(18, 16, 450, 90, Fade(BLACK, 0.79f));
    DrawText("CAJAS DEL PUERTO - 1 VS 3", 32, 24, 28, GOLD);

    const char* nombreFase = "";
    switch (fase)
    {
    case FASE_CAJAS_PREPARACION: nombreFase = "PREPARANDO"; break;
    case FASE_CAJAS_ESCONDER: nombreFase = "ESCONDERSE"; break;
    case FASE_CAJAS_CIERRE: nombreFase = "CERRANDO PUERTAS"; break;
    case FASE_CAJAS_ELEGIR: nombreFase = "LA GRUA ELIGE"; break;
    case FASE_CAJAS_RESOLVER: nombreFase = "RESULTADO"; break;
    case FASE_CAJAS_TERMINADO: nombreFase = "FIN"; break;
    }

    DrawText(
        TextFormat("RONDA %d/%d - %s", ronda, RONDAS_CAJAS, nombreFase),
        32,
        58,
        20,
        RAYWHITE
    );

    if (indiceSolo >= 0)
    {
        DrawText(
            TextFormat(
                "J%d EN LA GRUA%s - SOBREVIVIENTES: %d",
                participantes[indiceSolo].numeroJugador,
                participantes[indiceSolo].esBot ? " (BOT)" : "",
                ContarVivosCajas(*this)
            ),
            32,
            82,
            17,
            participantes[indiceSolo].color
        );
    }

    const char* controles = "TRIO: MOVER + ACCION (E / SHIFT DER / B) ENTRA  |  GRUA: IZQ/DER + ACCION";
    int anchoControles = MeasureText(controles, 16);

    DrawRectangle(ancho / 2 - anchoControles / 2 - 12, alto - 86, anchoControles + 24, 26, Fade(BLACK, 0.79f));
    DrawText(controles, ancho / 2 - anchoControles / 2, alto - 82, 16, RAYWHITE);

    if (fase == FASE_CAJAS_ESCONDER || fase == FASE_CAJAS_ELEGIR)
    {
        DrawText(
            TextFormat("TIEMPO %.1f", tiempoFase > 0.0f ? tiempoFase : 0.0f),
            ancho - 190,
            28,
            23,
            tiempoFase <= 3.0f ? RED : GOLD
        );
    }

    if (fase == FASE_CAJAS_ESCONDER)
    {
        const char* aviso = "NIEBLA: LA GRUA NO VE QUIEN ENTRA. ANCLA DORADA = DOBLE FONDO";
        DrawText(aviso, ancho / 2 - MeasureText(aviso, 20) / 2, alto - 46, 20, GOLD);
    }
    else if (fase == FASE_CAJAS_ELEGIR)
    {
        const char* aviso = TextFormat("CARGAS POR SOLTAR: %d", eleccionesRestantes);
        DrawText(aviso, ancho / 2 - MeasureText(aviso, 24) / 2, alto - 46, 24, YELLOW);
    }
    else if (fase == FASE_CAJAS_RESOLVER && revelacionHecha)
    {
        const char* aviso = TextFormat("ELIMINADOS EN LA RONDA: %d", eliminadosRonda);
        DrawText(aviso, ancho / 2 - MeasureText(aviso, 24) / 2, alto - 46, 24, YELLOW);
    }

    if (fase == FASE_CAJAS_PREPARACION)
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
        fase == FASE_CAJAS_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        indiceSolo >= 0
    )
    {
        bool ganaSolo = resultado.participantes[indiceSolo].posicionFinal == 1;
        const char* titulo = ganaSolo
            ? "GANA LA GRUA"
            : "GANA EL TRIO";

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
MinijuegoCajasPuerto::ObtenerResultado() const
{
    return resultado;
}
