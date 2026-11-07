#include "Minigames/MinijuegoGruaChatarra.h"

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
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_GRUA = 3.0f;
static const float DURACION_PARTIDA_GRUA = 50.0f;

// Pozo de chatarra y tolvas.
static const float LIMITE_X_GARRA_GRUA = 8.4f;
static const float LIMITE_Z_MIN_GARRA_GRUA = -4.8f;
static const float LIMITE_Z_MAX_GARRA_GRUA = 5.2f;
static const float LIMITE_X_OBJETO_GRUA = 8.0f;
static const float LIMITE_Z_MIN_OBJETO_GRUA = -4.4f;
static const float LIMITE_Z_MAX_OBJETO_GRUA = 4.8f;
static const float SEPARACION_TOLVAS_GRUA = 4.6f;
static const float Z_TOLVA_GRUA = -6.3f;

// Garra: movimiento con inercia leve.
static const float ACELERACION_GARRA_GRUA = 40.0f;
static const float ROZAMIENTO_GARRA_GRUA = 7.0f;
static const float VELOCIDAD_MAXIMA_GARRA_GRUA = 6.0f;
static const float RADIO_GARRA_GRUA = 0.75f;

// Secuencia de la accion (segundos).
static const float T_BAJAR_GRUA = 0.55f;
static const float T_CERRAR_GRUA = 0.80f;
static const float T_SUBIR_GRUA = 1.25f;
static const float T_FIN_FALLO_GRUA = 1.40f;
static const float T_FIN_EXITO_GRUA = 2.50f;
static const float T_LLEGADA_TOLVA_GRUA = 1.85f;
static const float T_SALIDA_TOLVA_GRUA = 2.00f;
static const float BLOQUEO_DISPUTA_GRUA = 1.0f;

static const float ALTURA_ALTA_GRUA = 3.2f;
static const float ALTURA_BAJA_GRUA = 0.75f;

// Objetos: valor y tolerancia de centrado (los grandes piden mas precision).
static const int VALOR_OBJETO_GRUA[CANTIDAD_TIPOS_OBJETO_GRUA] = { 1, 3, 5, 8, -2 };
static const float TOLERANCIA_OBJETO_GRUA[CANTIDAD_TIPOS_OBJETO_GRUA] = { 1.1f, 0.9f, 0.7f, 0.55f, 0.9f };
static const float TIEMPO_REAPARICION_GRUA = 4.0f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarGrua(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioGrua(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}


static float Ruido01Grua(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static int LimiteJugadoresGrua(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static bool EsControladoPorBotGrua(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static float TolvaXGrua(const MinijuegoGruaChatarra& minijuego, int puesto)
{
    return ((float)puesto - (float)(minijuego.cantidadPuestos - 1) * 0.5f) * SEPARACION_TOLVAS_GRUA;
}


//==================================================
// OBJETOS
//==================================================

static int ElegirTipoObjetoGrua()
{
    int dado = GetRandomValue(1, 100);

    if (dado <= 45) return OBJETO_GRUA_TUERCA;
    if (dado <= 70) return OBJETO_GRUA_ENGRANAJE;
    if (dado <= 85) return OBJETO_GRUA_MOTOR;
    if (dado <= 92) return OBJETO_GRUA_BATERIA;
    return OBJETO_GRUA_CARTUCHO;
}


// Coloca el objeto en un punto libre del pozo (separado de los demas).
static void ColocarObjetoGrua(MinijuegoGruaChatarra& minijuego, int indice, int tipo)
{
    ObjetoGrua& objeto = minijuego.objetos[indice];
    float mejorX = 0.0f;
    float mejorZ = 0.0f;
    float mejorHolgura = -1.0f;

    for (int intento = 0; intento < 14; intento++)
    {
        float x = AleatorioGrua(-LIMITE_X_OBJETO_GRUA, LIMITE_X_OBJETO_GRUA);
        float z = AleatorioGrua(LIMITE_Z_MIN_OBJETO_GRUA, LIMITE_Z_MAX_OBJETO_GRUA);
        float holgura = 99.0f;

        for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
        {
            if (k == indice || !minijuego.objetos[k].activo)
            {
                continue;
            }

            float dx = minijuego.objetos[k].x - x;
            float dz = minijuego.objetos[k].z - z;
            float d = std::sqrt(dx * dx + dz * dz);

            if (d < holgura)
            {
                holgura = d;
            }
        }

        if (holgura > mejorHolgura)
        {
            mejorHolgura = holgura;
            mejorX = x;
            mejorZ = z;
        }

        if (holgura >= 1.3f)
        {
            break;
        }
    }

    objeto.activo = true;
    objeto.tipo = tipo;
    objeto.x = mejorX;
    objeto.z = mejorZ;
    objeto.giro = AleatorioGrua(0.0f, 360.0f);
    objeto.tiempoReaparicion = 0.0f;
}


// Objeto activo mas cercano dentro de su tolerancia de centrado.
static int BuscarCandidatoGrua(
    const MinijuegoGruaChatarra& minijuego,
    float x,
    float z,
    float& distancia
)
{
    int mejor = -1;
    float mejorDistancia = 1.0e9f;

    for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
    {
        const ObjetoGrua& objeto = minijuego.objetos[k];

        if (!objeto.activo)
        {
            continue;
        }

        float dx = objeto.x - x;
        float dz = objeto.z - z;
        float d = std::sqrt(dx * dx + dz * dz);

        if (d < TOLERANCIA_OBJETO_GRUA[objeto.tipo] && d < mejorDistancia)
        {
            mejorDistancia = d;
            mejor = k;
        }
    }

    distancia = mejorDistancia;
    return mejor;
}


//==================================================
// SECUENCIA DE LA GARRA
//==================================================

static void IniciarAccionGrua(MinijuegoGruaChatarra& minijuego, int indice)
{
    EstadoJugadorGrua& estado = minijuego.estadosJugadores[indice];

    estado.estado = GARRA_GRUA_ACCION;
    estado.tiempo = 0.0f;
    estado.duracion = T_FIN_FALLO_GRUA;
    estado.origenX = estado.x;
    estado.origenZ = estado.z;
    estado.velocidadX = 0.0f;
    estado.velocidadZ = 0.0f;
    estado.resuelto = false;
    estado.exito = false;
    estado.depositado = false;
    estado.disputaPerdida = false;
    estado.tipoCapturado = -1;
    estado.bloqueo = 0.0f;
    estado.objetivo = -1;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);
}


static void MostrarMensajeGrua(EstadoJugadorGrua& estado, int mensaje)
{
    estado.mensaje = mensaje;
    estado.tiempoMensaje = 1.2f;
}


// Momento en que la garra se cierra: decide la captura y las disputas.
static void ResolverCierreGrua(MinijuegoGruaChatarra& minijuego, int indice, int limite)
{
    EstadoJugadorGrua& estado = minijuego.estadosJugadores[indice];
    estado.resuelto = true;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);

    float distancia = 0.0f;
    int candidato = BuscarCandidatoGrua(minijuego, estado.origenX, estado.origenZ, distancia);

    auto fallar = [&](bool porDisputa)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        MostrarMensajeGrua(estado, porDisputa ? 2 : -1);

        if (porDisputa)
        {
            estado.bloqueo = BLOQUEO_DISPUTA_GRUA;
            estado.duracion = T_FIN_FALLO_GRUA + BLOQUEO_DISPUTA_GRUA;
        }
    };

    if (estado.disputaPerdida)
    {
        fallar(true);
        return;
    }

    if (candidato < 0)
    {
        fallar(false);
        return;
    }

    // Disputa: otra garra que aun baja sobre el mismo objeto.
    for (int j = 0; j < limite; j++)
    {
        const EstadoJugadorGrua& rival = minijuego.estadosJugadores[j];

        if (j == indice || !rival.participa || rival.estado != GARRA_GRUA_ACCION || rival.resuelto)
        {
            continue;
        }

        float distanciaRival = 0.0f;
        int candidatoRival = BuscarCandidatoGrua(minijuego, rival.origenX, rival.origenZ, distanciaRival);

        if (candidatoRival == candidato && distanciaRival < distancia - 0.001f)
        {
            fallar(true);
            return;
        }
    }

    int tipo = minijuego.objetos[candidato].tipo;
    float tolerancia = TOLERANCIA_OBJETO_GRUA[tipo];
    float probabilidad = LimitarGrua(1.3f * (1.0f - distancia / tolerancia), 0.0f, 1.0f);

    if (AleatorioGrua(0.0f, 1.0f) > probabilidad)
    {
        fallar(false);
        return;
    }

    // Captura: el objeto sale del pozo.
    minijuego.objetos[candidato].activo = false;
    minijuego.objetos[candidato].tiempoReaparicion = TIEMPO_REAPARICION_GRUA;
    estado.tipoCapturado = tipo;

    if (tipo == OBJETO_GRUA_CARTUCHO)
    {
        // El cartucho oxidado resta puntos y se descarta.
        estado.puntos += VALOR_OBJETO_GRUA[tipo];
        if (estado.puntos < 0) estado.puntos = 0;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        MostrarMensajeGrua(estado, 3);
        return;
    }

    estado.exito = true;
    estado.duracion = T_FIN_EXITO_GRUA;

    // Las garras que disputaban este objeto quedan bloqueadas.
    for (int j = 0; j < limite; j++)
    {
        EstadoJugadorGrua& rival = minijuego.estadosJugadores[j];

        if (j == indice || !rival.participa || rival.estado != GARRA_GRUA_ACCION || rival.resuelto)
        {
            continue;
        }

        float distanciaRival = 0.0f;

        if (BuscarCandidatoGrua(minijuego, rival.origenX, rival.origenZ, distanciaRival) < 0)
        {
            float dx = minijuego.objetos[candidato].x - rival.origenX;
            float dz = minijuego.objetos[candidato].z - rival.origenZ;

            if (std::sqrt(dx * dx + dz * dz) < tolerancia)
            {
                rival.disputaPerdida = true;
            }
        }
    }
}


static void ActualizarAccionGrua(
    MinijuegoGruaChatarra& minijuego,
    int indice,
    int limite,
    float deltaTime
)
{
    EstadoJugadorGrua& estado = minijuego.estadosJugadores[indice];
    estado.tiempo += deltaTime;

    if (!estado.resuelto && estado.tiempo >= T_BAJAR_GRUA)
    {
        ResolverCierreGrua(minijuego, indice, limite);
    }

    if (estado.exito)
    {
        float tolvaX = TolvaXGrua(minijuego, estado.puesto);

        if (estado.tiempo < T_SUBIR_GRUA)
        {
            estado.x = estado.origenX;
            estado.z = estado.origenZ;
        }
        else if (estado.tiempo < T_LLEGADA_TOLVA_GRUA)
        {
            float f = (estado.tiempo - T_SUBIR_GRUA) / (T_LLEGADA_TOLVA_GRUA - T_SUBIR_GRUA);
            estado.x = estado.origenX + (tolvaX - estado.origenX) * f;
            estado.z = estado.origenZ + (Z_TOLVA_GRUA + 0.4f - estado.origenZ) * f;
        }
        else if (estado.tiempo < T_SALIDA_TOLVA_GRUA)
        {
            estado.x = tolvaX;
            estado.z = Z_TOLVA_GRUA + 0.4f;
        }
        else
        {
            float f = (estado.tiempo - T_SALIDA_TOLVA_GRUA) / (T_FIN_EXITO_GRUA - T_SALIDA_TOLVA_GRUA);
            f = LimitarGrua(f, 0.0f, 1.0f);
            estado.x = tolvaX + (estado.origenX - tolvaX) * f;
            estado.z = Z_TOLVA_GRUA + 0.4f + (estado.origenZ - (Z_TOLVA_GRUA + 0.4f)) * f;
        }

        if (!estado.depositado && estado.tiempo >= T_LLEGADA_TOLVA_GRUA)
        {
            int valor = VALOR_OBJETO_GRUA[estado.tipoCapturado];

            estado.depositado = true;
            estado.puntos += valor;

            if (valor > estado.mejorObjeto)
            {
                estado.mejorObjeto = valor;
            }

            ReproducirSonidoMinijuego(
                minijuego.audio,
                estado.tipoCapturado == OBJETO_GRUA_BATERIA ? SONIDO_RECOGER_NUCLEO_ESPECIAL : SONIDO_ACIERTO
            );
            estado.valorMensaje = valor;
            MostrarMensajeGrua(estado, 1);
        }
    }

    if (estado.tiempo >= estado.duracion)
    {
        estado.estado = GARRA_GRUA_LIBRE;
        estado.x = estado.origenX;
        estado.z = estado.origenZ;
        estado.tipoCapturado = -1;
    }
}


//==================================================
// IA DE BOTS
//==================================================

static InputMinijuegoParticipante CrearEntradaBotGrua(
    MinijuegoGruaChatarra& minijuego,
    int indice,
    int limite,
    float deltaTime
)
{
    EstadoJugadorGrua& estado = minijuego.estadosJugadores[indice];
    InputMinijuegoParticipante entrada{};

    estado.tiempoDecision -= deltaTime;

    bool objetivoValido =
        estado.objetivo >= 0 &&
        minijuego.objetos[estado.objetivo].activo &&
        minijuego.objetos[estado.objetivo].tipo != OBJETO_GRUA_CARTUCHO;

    if (!objetivoValido || estado.tiempoDecision <= 0.0f)
    {
        estado.tiempoDecision = AleatorioGrua(0.4f, 0.7f);

        int mejor = -1;
        float mejorPuntaje = -1.0e9f;

        for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
        {
            const ObjetoGrua& objeto = minijuego.objetos[k];

            if (!objeto.activo || objeto.tipo == OBJETO_GRUA_CARTUCHO)
            {
                continue;
            }

            bool reclamado = false;

            for (int j = 0; j < limite; j++)
            {
                if (j != indice && minijuego.estadosJugadores[j].participa && minijuego.estadosJugadores[j].objetivo == k)
                {
                    reclamado = true;
                }
            }

            if (reclamado || GetRandomValue(1, 100) <= 10)
            {
                continue;
            }

            float dx = objeto.x - estado.x;
            float dz = objeto.z - estado.z;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float puntaje = (float)VALOR_OBJETO_GRUA[objeto.tipo] / (1.0f + distancia * 0.35f);

            if (puntaje > mejorPuntaje)
            {
                mejorPuntaje = puntaje;
                mejor = k;
            }
        }

        if (mejor != estado.objetivo)
        {
            float angulo = AleatorioGrua(0.0f, 6.2831853f);
            float magnitud = AleatorioGrua(0.1f, 0.35f);

            estado.errorX = std::cos(angulo) * magnitud;
            estado.errorZ = std::sin(angulo) * magnitud;
            estado.esperaBajar = AleatorioGrua(0.1f, 0.3f);
        }

        estado.objetivo = mejor;
    }

    if (estado.objetivo < 0)
    {
        return entrada;
    }

    const ObjetoGrua& objeto = minijuego.objetos[estado.objetivo];
    float dx = objeto.x + estado.errorX - estado.x;
    float dz = objeto.z + estado.errorZ - estado.z;

    // El umbral anticipa la inercia: frena antes de llegar.
    float rapidez = std::sqrt(estado.velocidadX * estado.velocidadX + estado.velocidadZ * estado.velocidadZ);
    float umbral = 0.06f + rapidez * 0.13f;

    if (dx > umbral) entrada.derecha = true;
    if (dx < -umbral) entrada.izquierda = true;
    if (dz > umbral) entrada.atras = true;
    if (dz < -umbral) entrada.adelante = true;

    if (std::sqrt(dx * dx + dz * dz) < 0.22f && rapidez < 1.2f)
    {
        estado.esperaBajar -= deltaTime;

        if (estado.esperaBajar <= 0.0f)
        {
            entrada.golpear = true;
        }
    }

    return entrada;
}


//==================================================
// MOVIMIENTO DE GARRAS
//==================================================

static void MoverGarraGrua(
    EstadoJugadorGrua& estado,
    const InputMinijuegoParticipante& entrada,
    float deltaTime
)
{
    float dirX = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
    float dirZ = (entrada.atras ? 1.0f : 0.0f) - (entrada.adelante ? 1.0f : 0.0f);
    float longitud = std::sqrt(dirX * dirX + dirZ * dirZ);

    if (longitud > 0.01f)
    {
        dirX /= longitud;
        dirZ /= longitud;
    }

    estado.velocidadX += dirX * ACELERACION_GARRA_GRUA * deltaTime;
    estado.velocidadZ += dirZ * ACELERACION_GARRA_GRUA * deltaTime;

    float factor = 1.0f - ROZAMIENTO_GARRA_GRUA * deltaTime;
    if (factor < 0.0f) factor = 0.0f;
    estado.velocidadX *= factor;
    estado.velocidadZ *= factor;

    float rapidez = std::sqrt(estado.velocidadX * estado.velocidadX + estado.velocidadZ * estado.velocidadZ);

    if (rapidez > VELOCIDAD_MAXIMA_GARRA_GRUA)
    {
        float escala = VELOCIDAD_MAXIMA_GARRA_GRUA / rapidez;
        estado.velocidadX *= escala;
        estado.velocidadZ *= escala;
    }

    estado.x += estado.velocidadX * deltaTime;
    estado.z += estado.velocidadZ * deltaTime;

    if (estado.x < -LIMITE_X_GARRA_GRUA || estado.x > LIMITE_X_GARRA_GRUA)
    {
        estado.x = LimitarGrua(estado.x, -LIMITE_X_GARRA_GRUA, LIMITE_X_GARRA_GRUA);
        estado.velocidadX = 0.0f;
    }

    if (estado.z < LIMITE_Z_MIN_GARRA_GRUA || estado.z > LIMITE_Z_MAX_GARRA_GRUA)
    {
        estado.z = LimitarGrua(estado.z, LIMITE_Z_MIN_GARRA_GRUA, LIMITE_Z_MAX_GARRA_GRUA);
        estado.velocidadZ = 0.0f;
    }
}


// Las garras no se atraviesan: las libres se apartan de cualquier otra
// que este en el plano del pozo.
static void SepararGarrasGrua(MinijuegoGruaChatarra& minijuego, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorGrua& a = minijuego.estadosJugadores[i];

        if (!a.participa || a.estado != GARRA_GRUA_LIBRE)
        {
            continue;
        }

        for (int j = 0; j < limite; j++)
        {
            const EstadoJugadorGrua& b = minijuego.estadosJugadores[j];

            if (j == i || !b.participa)
            {
                continue;
            }

            // Las garras en entrega viajan por arriba: no estorban.
            float bx = b.estado == GARRA_GRUA_ACCION ? b.origenX : b.x;
            float bz = b.estado == GARRA_GRUA_ACCION ? b.origenZ : b.z;

            if (b.estado == GARRA_GRUA_ACCION && b.exito && b.tiempo >= T_SUBIR_GRUA)
            {
                continue;
            }

            float dx = a.x - bx;
            float dz = a.z - bz;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float minimo = RADIO_GARRA_GRUA * 2.0f;

            if (distancia >= minimo)
            {
                continue;
            }

            if (distancia < 0.0001f)
            {
                dx = 1.0f;
                dz = 0.0f;
                distancia = 1.0f;
            }

            // Si el otro tambien esta libre, se reparte el empuje.
            float empuje = (minimo - distancia) * (b.estado == GARRA_GRUA_LIBRE ? 0.5f : 1.0f);
            a.x += dx / distancia * empuje;
            a.z += dz / distancia * empuje;
            a.x = LimitarGrua(a.x, -LIMITE_X_GARRA_GRUA, LIMITE_X_GARRA_GRUA);
            a.z = LimitarGrua(a.z, LIMITE_Z_MIN_GARRA_GRUA, LIMITE_Z_MAX_GARRA_GRUA);
        }
    }
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarGrua(MinijuegoGruaChatarra& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.cantidadEquipos = 0;

    int cantidadPrimeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorGrua& estado = minijuego.estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        // Posicion competitiva: puntos y, a igualdad, el mejor objeto.
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            const EstadoJugadorGrua& otro = minijuego.estadosJugadores[j];

            if (
                j != i && otro.participa &&
                (otro.puntos > estado.puntos ||
                    (otro.puntos == estado.puntos && otro.mejorObjeto > estado.mejorObjeto))
            )
            {
                posicion++;
            }
        }

        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];
        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = estado.puntos;
        resultadoJugador.puntosObtenidos = 0;

        if (posicion == 1)
        {
            cantidadPrimeros++;
        }
    }

    minijuego.empate = cantidadPrimeros > 1;
    minijuego.resultado.desenlace =
        minijuego.empate ? DESENLACE_EMPATE : DESENLACE_CON_GANADOR;
    minijuego.fase = FASE_GRUA_TERMINADO;
}


//==================================================
// INICIALIZACION
//==================================================

static void LiberarMallasGrua();


void MinijuegoGruaChatarra::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_OBJETOS_GRUA; i++)
    {
        objetos[i] = {};
    }

    // Camara diagonal: pozo, tolvas y, detras, el desguace al atardecer.
    camara.position = { 0.0f, 12.65f, 14.6f };
    camara.target = { 0.0f, 0.6f, -2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 47.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_GRUA_PREPARACION;
    cantidadPuestos = 0;
    partidaValida = false;
    empate = false;

    tiempoPreparacion = DURACION_PREPARACION_GRUA;
    tiempoRestante = DURACION_PARTIDA_GRUA;
    tiempoAnimacion = 0.0f;

    // Estado visual.
    for (int i = 0; i < MAX_PARTICULAS_GRUA; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        destelloTolva[i] = 0.0f;
        sacudidaGarra[i] = 0.0f;
    }

    semillaVisual = 0x5EED6A1Bu;
    tiempoChispasSoldadura = 0.0f;
    tiempoFinVisual = -1.0f;
}


void MinijuegoGruaChatarra::Reiniciar(
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

    int limite = LimiteJugadoresGrua(cantidadMaxima);
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            cantidad++;
        }
    }

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_GRUA_TERMINADO;
        return;
    }

    CargarPiezasJugablesGruaChatarra();
    LiberarMallasGrua();
    partidaValida = true;
    cantidadPuestos = cantidad;

    // Poblacion inicial del pozo.
    static const int INICIAL[CANTIDAD_TIPOS_OBJETO_GRUA] = { 10, 6, 4, 2, 3 };
    int slot = 0;

    for (int tipo = 0; tipo < CANTIDAD_TIPOS_OBJETO_GRUA; tipo++)
    {
        for (int k = 0; k < INICIAL[tipo] && slot < MAX_OBJETOS_GRUA; k++)
        {
            ColocarObjetoGrua(*this, slot++, tipo);
        }
    }

    int puesto = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        EstadoJugadorGrua& estado = estadosJugadores[i];
        estado.participa = true;
        estado.puesto = puesto++;
        estado.x = TolvaXGrua(*this, estado.puesto);
        estado.z = 0.5f;
        estado.origenX = estado.x;
        estado.origenZ = estado.z;
        estado.tiempoDecision = AleatorioGrua(0.0f, 0.4f);

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], { estado.x, 1.0f, Z_TOLVA_GRUA - 1.5f });
        jugadores[i].direccionMirada = { 0.0f, 0.0f, 1.0f };
        jugadores[i].enSuelo = true;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoGruaChatarra::ActualizarLogica(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_GRUA_TERMINADO ||
        resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int limite = LimiteJugadoresGrua(cantidadMaxima);

    // Los operadores se quedan en su puesto, junto a su tolva.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        jugadores[i].posicion = { TolvaXGrua(*this, estado.puesto), 1.0f, Z_TOLVA_GRUA - 1.5f };
        jugadores[i].velocidad = {};
        jugadores[i].enSuelo = true;
        jugadores[i].cayendo = false;
    }

    if (fase == FASE_GRUA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_GRUA_JUGANDO;
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorGrua& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        if (estado.tiempoMensaje > 0.0f)
        {
            estado.tiempoMensaje -= deltaTime;
        }

        if (estado.estado == GARRA_GRUA_ACCION)
        {
            ActualizarAccionGrua(*this, i, limite, deltaTime);
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (EsControladoPorBotGrua(participantes[i]))
        {
            entrada = CrearEntradaBotGrua(*this, i, limite, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        MoverGarraGrua(estado, entrada, deltaTime);

        if (entrada.golpear || entrada.saltar)
        {
            IniciarAccionGrua(*this, i);
        }
    }

    SepararGarrasGrua(*this, limite);

    // Reaparicion de objetos y giro decorativo.
    for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
    {
        ObjetoGrua& objeto = objetos[k];

        if (objeto.activo)
        {
            objeto.giro += 20.0f * deltaTime;
            continue;
        }

        if (objeto.tiempoReaparicion > 0.0f)
        {
            objeto.tiempoReaparicion -= deltaTime;

            if (objeto.tiempoReaparicion <= 0.0f)
            {
                ColocarObjetoGrua(*this, k, ElegirTipoObjetoGrua());
            }
        }
    }

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarGrua(*this);
    }
}


//==================================================
// PIEZAS GLB JUGABLES
//==================================================
// Del paquete solo se cargan las piezas que muestran el estado del juego
// (tramo seguido del enum): tolva del color del jugador, iman, pinzas,
// carro, marca y las cinco piezas de chatarra. El desguace se construye con
// primitivas propias horneadas en mallas.

static_assert(MODELO_GRUA_TOLVA + 9 == MODELO_GRUA_CARTUCHO,
    "Tolva, garra, marca y piezas deben ir seguidas en el paquete Grua de Chatarra");

void CargarPiezasJugablesGruaChatarra()
{
    // Mismas primitives por GLB que exige CargarPaqueteGruaChatarraRetro3D.
    static const int mallasEsperadas[CANTIDAD_MODELOS_GRUA_CHATARRA_3D] =
        { 5,2,5,4,4,4,5,5,5,5,6,5,5,3,2,3,2,3,3,4,3,3 };

    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteGruaChatarraRetro3D();
    PaqueteModelosEscenarioRetro3D tramo = paquete;
    tramo.recursos += MODELO_GRUA_TOLVA;
    tramo.definiciones += MODELO_GRUA_TOLVA;
    tramo.cantidad = MODELO_GRUA_CARTUCHO - MODELO_GRUA_TOLVA + 1;
    CargarPaqueteModelosEscenarioRetro3D(tramo);

    for (int i = MODELO_GRUA_TOLVA; i <= MODELO_GRUA_CARTUCHO; i++)
    {
        RecursoModeloEscenarioRetro3D& r = paquete.recursos[i];
        if (!r.cargado || r.modelo.meshCount == mallasEsperadas[i]) continue;
        UnloadModel(r.modelo);
        r.modelo = {};
        r.cargado = false;
        TraceLog(LOG_WARNING, "Paquete Grua de Chatarra incompatible; se usan primitivas: %s", r.ruta);
    }
}


//==================================================
// VISUAL: ESTADO (particulas y eventos)
//==================================================
// Lo escribe Actualizar despues de la logica comparando una instantanea:
// nunca toca reglas ni consume GetRandomValue.

enum TipoParticulaGrua
{
    PARTICULA_GRUA_POLVO = 0,
    PARTICULA_GRUA_CHISPA,
    PARTICULA_GRUA_PIEZA,
    PARTICULA_GRUA_CONFETI,
    PARTICULA_GRUA_HUMO
};

static const float PERIODO_PRENSA_GRUA = 4.6f;
static const Vector3 PUNTO_SOLDADURA_GRUA = { -12.9f, 0.55f, 4.3f };
static const Vector3 CENTRO_PRENSA_GRUA = { 13.6f, 0.0f, -2.2f };

struct InstantaneaGrua
{
    bool resuelto[MAX_PARTICIPANTES];
    bool depositado[MAX_PARTICIPANTES];
    float tiempoMensaje[MAX_PARTICIPANTES];
    int fase;
};


static float AzarVisualGrua(MinijuegoGruaChatarra& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    return minimo + (maximo - minimo) * (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
}


static int CantidadParticulasGrua(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


static void EmitirParticulaGrua(
    MinijuegoGruaChatarra& m,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    int elegido = -1;
    float menorVida = 1.0e9f;

    for (int i = 0; i < MAX_PARTICULAS_GRUA; i++)
    {
        if (!m.particulas[i].activa)
        {
            elegido = i;
            break;
        }

        if (m.particulas[i].vida < menorVida)
        {
            menorVida = m.particulas[i].vida;
            elegido = i;
        }
    }

    ParticulaGrua& p = m.particulas[elegido];
    p.activa = true;
    p.tipo = tipo;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.vida = vida;
    p.vidaMaxima = vida;
    p.tamano = tamano;
    p.color = color;
}


static void EstallidoGrua(
    MinijuegoGruaChatarra& m,
    int tipo,
    Vector3 centro,
    int cantidad,
    float fuerza,
    float subida,
    float vida,
    float tamano,
    Color a,
    Color b
)
{
    int n = CantidadParticulasGrua(cantidad);

    for (int k = 0; k < n; k++)
    {
        float angulo = AzarVisualGrua(m, 0.0f, 2.0f * PI);
        float rapidez = AzarVisualGrua(m, 0.35f, 1.0f) * fuerza;
        Vector3 v = {
            std::cos(angulo) * rapidez,
            subida * AzarVisualGrua(m, 0.5f, 1.2f),
            std::sin(angulo) * rapidez
        };
        Color c = AzarVisualGrua(m, 0.0f, 1.0f) < 0.5f ? a : b;
        EmitirParticulaGrua(m, tipo, centro, v, vida * AzarVisualGrua(m, 0.7f, 1.2f), tamano * AzarVisualGrua(m, 0.7f, 1.3f), c);
    }
}


static void TomarInstantaneaGrua(const MinijuegoGruaChatarra& m, InstantaneaGrua& s)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        s.resuelto[i] = m.estadosJugadores[i].resuelto;
        s.depositado[i] = m.estadosJugadores[i].depositado;
        s.tiempoMensaje[i] = m.estadosJugadores[i].tiempoMensaje;
    }

    s.fase = (int)m.fase;
}


static void ActualizarVisualGrua(
    MinijuegoGruaChatarra& m,
    const InstantaneaGrua& antes,
    float dt,
    const Participante participantes[],
    int limite
)
{
    if (dt > 0.05f) dt = 0.05f;

    // Eventos de cada garra.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        Color color = participantes[i].color;
        Vector3 suelo = { e.origenX, 0.12f, e.origenZ };

        if (!antes.resuelto[i] && e.resuelto && e.estado == GARRA_GRUA_ACCION)
        {
            // Golpe del iman contra la chatarra.
            EstallidoGrua(m, PARTICULA_GRUA_POLVO, suelo, 8, 1.3f, 0.6f, 0.9f, 0.20f,
                Color{ 150, 128, 108, 255 }, Color{ 118, 102, 92, 255 });

            if (e.tipoCapturado >= 0 && e.tipoCapturado != OBJETO_GRUA_CARTUCHO)
            {
                Vector3 iman = { e.origenX, ALTURA_BAJA_GRUA, e.origenZ };
                EstallidoGrua(m, PARTICULA_GRUA_CHISPA, iman, 12, 2.6f, 2.2f, 0.45f, 0.07f,
                    Color{ 170, 230, 255, 255 }, WHITE);
            }
        }

        if (e.tiempoMensaje > antes.tiempoMensaje[i] + 0.01f)
        {
            if (e.mensaje == 3)
            {
                // Cartucho oxidado: nube de herrumbre.
                Vector3 c = { e.origenX, 0.5f, e.origenZ };
                EstallidoGrua(m, PARTICULA_GRUA_HUMO, c, 10, 0.8f, 0.9f, 1.3f, 0.34f,
                    Color{ 140, 72, 46, 255 }, Color{ 92, 70, 60, 255 });
                EstallidoGrua(m, PARTICULA_GRUA_PIEZA, c, 6, 2.2f, 3.0f, 0.9f, 0.10f,
                    Color{ 120, 60, 40, 255 }, Color{ 70, 50, 44, 255 });
                m.sacudidaGarra[i] = 0.6f;
            }
            else if (e.mensaje == -1 || e.mensaje == 2)
            {
                m.sacudidaGarra[i] = 0.5f;
            }
        }

        if (!antes.depositado[i] && e.depositado)
        {
            // Entrega: la tolva festeja con el color del jugador.
            float tolvaX = TolvaXGrua(m, e.puesto);
            Vector3 boca = { tolvaX, 1.6f, Z_TOLVA_GRUA };
            EstallidoGrua(m, PARTICULA_GRUA_PIEZA, boca, 16, 2.4f, 4.6f, 1.0f, 0.13f,
                color, ColorBrightness(color, 0.45f));
            EstallidoGrua(m, PARTICULA_GRUA_CHISPA, boca, 10, 2.0f, 3.6f, 0.6f, 0.07f,
                Color{ 255, 214, 90, 255 }, WHITE);
            m.destelloTolva[i] = 1.0f;
        }
    }

    // Fin de la partida: confeti sobre el ganador.
    bool terminado = m.fase == FASE_GRUA_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO;

    if (terminado && antes.fase != (int)FASE_GRUA_TERMINADO)
    {
        m.tiempoFinVisual = m.tiempoAnimacion;
    }

    if (terminado && m.tiempoFinVisual >= 0.0f && m.tiempoAnimacion - m.tiempoFinVisual < 3.0f)
    {
        static const Color CONFETI[5] = {
            { 255, 214, 90, 255 }, { 255, 120, 150, 255 }, { 120, 220, 255, 255 },
            { 160, 240, 130, 255 }, { 255, 255, 255, 255 }
        };

        for (int i = 0; i < limite; i++)
        {
            if (!m.estadosJugadores[i].participa || m.resultado.participantes[i].posicionFinal != 1)
            {
                continue;
            }

            float x = TolvaXGrua(m, m.estadosJugadores[i].puesto);
            int n = CantidadParticulasGrua(3);

            for (int k = 0; k < n; k++)
            {
                Vector3 p = { x + AzarVisualGrua(m, -1.6f, 1.6f), 4.6f + AzarVisualGrua(m, 0.0f, 1.0f), Z_TOLVA_GRUA - 1.5f + AzarVisualGrua(m, -1.0f, 1.0f) };
                Vector3 v = { AzarVisualGrua(m, -0.6f, 0.6f), AzarVisualGrua(m, -0.4f, 0.4f), AzarVisualGrua(m, -0.6f, 0.6f) };
                Color c = CONFETI[(int)AzarVisualGrua(m, 0.0f, 4.99f)];
                EmitirParticulaGrua(m, PARTICULA_GRUA_CONFETI, p, v, 2.6f, 0.12f, c);
            }
        }
    }

    // Soldador del anillo cercano: rafagas de chispas con pausas.
    m.tiempoChispasSoldadura -= dt;

    if (m.tiempoChispasSoldadura <= 0.0f)
    {
        bool pausa = AzarVisualGrua(m, 0.0f, 1.0f) < 0.18f;
        m.tiempoChispasSoldadura = pausa ? AzarVisualGrua(m, 0.8f, 1.6f) : AzarVisualGrua(m, 0.05f, 0.16f);

        if (!pausa)
        {
            EstallidoGrua(m, PARTICULA_GRUA_CHISPA, PUNTO_SOLDADURA_GRUA, 4, 1.7f, 2.0f, 0.5f, 0.05f,
                Color{ 255, 220, 140, 255 }, Color{ 255, 250, 220, 255 });
        }
    }

    // Prensa: polvo y chispas cuando el plato toca la chatarra.
    float fasePrensa = std::fmod(m.tiempoAnimacion, PERIODO_PRENSA_GRUA) / PERIODO_PRENSA_GRUA;
    float faseAntes = std::fmod(m.tiempoAnimacion - dt, PERIODO_PRENSA_GRUA) / PERIODO_PRENSA_GRUA;

    if (faseAntes < 0.42f && fasePrensa >= 0.42f)
    {
        Vector3 c = { CENTRO_PRENSA_GRUA.x, 1.25f, CENTRO_PRENSA_GRUA.z + 1.0f };
        EstallidoGrua(m, PARTICULA_GRUA_POLVO, c, 8, 1.6f, 0.6f, 1.1f, 0.26f,
            Color{ 160, 136, 112, 255 }, Color{ 128, 110, 98, 255 });
        EstallidoGrua(m, PARTICULA_GRUA_CHISPA, c, 6, 2.2f, 2.0f, 0.5f, 0.06f,
            Color{ 255, 210, 130, 255 }, WHITE);
    }

    // Destellos que se apagan.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.destelloTolva[i] = m.destelloTolva[i] > dt * 1.6f ? m.destelloTolva[i] - dt * 1.6f : 0.0f;
        m.sacudidaGarra[i] = m.sacudidaGarra[i] > dt ? m.sacudidaGarra[i] - dt : 0.0f;
    }

    // Movimiento de las particulas.
    for (int i = 0; i < MAX_PARTICULAS_GRUA; i++)
    {
        ParticulaGrua& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= dt;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        if (p.tipo == PARTICULA_GRUA_CHISPA || p.tipo == PARTICULA_GRUA_PIEZA)
        {
            p.velocidad.y -= 9.0f * dt;
        }
        else if (p.tipo == PARTICULA_GRUA_CONFETI)
        {
            p.velocidad.y = -0.9f;
            p.velocidad.x += std::sin(m.tiempoAnimacion * 5.0f + (float)i) * 1.5f * dt;
        }
        else if (p.tipo == PARTICULA_GRUA_HUMO)
        {
            p.velocidad.y += 0.4f * dt;
            p.velocidad.x *= 1.0f - 1.5f * dt;
            p.velocidad.z *= 1.0f - 1.5f * dt;
            p.tamano += 0.25f * dt;
        }
        else
        {
            p.velocidad.x *= 1.0f - 2.2f * dt;
            p.velocidad.z *= 1.0f - 2.2f * dt;
            p.velocidad.y -= 0.6f * dt;
            p.tamano += 0.18f * dt;
        }

        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        if (p.posicion.y < 0.04f && p.velocidad.y < 0.0f)
        {
            p.posicion.y = 0.04f;
            p.velocidad.y *= p.tipo == PARTICULA_GRUA_PIEZA ? -0.35f : 0.0f;
            p.velocidad.x *= 0.5f;
            p.velocidad.z *= 0.5f;
        }
    }
}


void MinijuegoGruaChatarra::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    InstantaneaGrua antes{};
    TomarInstantaneaGrua(*this, antes);

    ActualizarLogica(deltaTime, jugadores, cantidadMaxima, participantes);

    if (partidaValida)
    {
        ActualizarVisualGrua(*this, antes, deltaTime, participantes, LimiteJugadoresGrua(cantidadMaxima));
    }
}


//==================================================
// VISUAL: PALETA, CAMARA Y EMISION DE TRIANGULOS
//==================================================
// Desguace al aire libre a la hora del atardecer: el sol cae detras de las
// montanas de chatarra, el cielo pasa de violeta a naranja y las torres de
// las gruas se recortan contra el. Acento reservado: dorado de lo valioso.

static const Color COLOR_CIELO_ALTO_G = { 70, 44, 92, 255 };
static const Color COLOR_CIELO_MEDIO_G = { 178, 84, 98, 255 };
static const Color COLOR_CIELO_BAJO_G = { 240, 132, 82, 255 };
static const Color COLOR_HORIZONTE_G = { 255, 190, 118, 255 };
static const Color COLOR_NIEBLA_G = { 178, 104, 106, 255 };
static const Color COLOR_SOL_G = { 255, 236, 178, 255 };
static const Color COLOR_TINTE_SOL_G = { 255, 176, 112, 255 };
static const Color COLOR_TIERRA_G = { 116, 94, 80, 255 };
static const Color COLOR_TIERRA_OSCURA_G = { 82, 66, 60, 255 };
static const Color COLOR_HORMIGON_G = { 108, 98, 92, 255 };
static const Color COLOR_ACERO_G = { 116, 120, 130, 255 };
static const Color COLOR_ACERO_OSCURO_G = { 60, 62, 72, 255 };
static const Color COLOR_OXIDO_G = { 150, 80, 50, 255 };
static const Color COLOR_OXIDO_OSCURO_G = { 96, 56, 44, 255 };
static const Color COLOR_OCRE_G = { 214, 128, 52, 255 };
static const Color COLOR_GOMA_G = { 36, 34, 38, 255 };
static const Color COLOR_LUZ_G = { 255, 200, 128, 255 };
static const Color COLOR_FUEGO_G = { 255, 128, 44, 255 };
static const Color COLOR_DORADO_G = { 255, 214, 90, 255 };
static const Color COLOR_SILUETA_G = { 88, 50, 72, 255 };
static const Color COLOR_SILUETA_LEJANA_G = { 150, 86, 96, 255 };

// Carrocerias apagadas: nunca los colores de los jugadores.
static const Color COLORES_AUTO_G[6] = {
    { 72, 118, 120, 255 }, { 128, 58, 54, 255 }, { 196, 180, 146, 255 },
    { 98, 120, 150, 255 }, { 118, 120, 72, 255 }, { 150, 112, 86, 255 }
};

static const Vector3 OJO_GRUA = { 0.0f, 12.65f, 14.6f };
static const Vector3 MIRA_GRUA = { 0.0f, 0.6f, -2.0f };
static const float FOV_GRUA = 47.0f;

// Bordes reales del pozo (la garra y la chatarra nunca los cruzan).
static const float X_POZO_G = 9.1f;
static const float Z_POZO_FRENTE_G = 5.8f;
static const float Z_POZO_FONDO_G = -5.05f;

// Torres del cablecam que sostiene cada carro.
static const Vector3 TORRES_GRUA[4] = {
    { -10.9f, 10.6f, -7.6f }, { 10.9f, 10.6f, -7.6f },
    { -10.9f, 10.6f, 7.4f }, { 10.9f, 10.6f, 7.4f }
};


struct GrabacionGrua
{
    bool activa = false;
    std::vector<float> posiciones;
    std::vector<unsigned char> colores;
};

static GrabacionGrua g_grabG;
static Vector3 g_ojoG = OJO_GRUA;    // bruma: siempre desde la camara final
static Vector3 g_camG = OJO_GRUA;    // camara actual (tiras y halos)
static float g_emisG = 0.0f;          // 1 = superficie luminosa: casi no la tapa la bruma
static Vector3 g_derG = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaG = { 0.0f, 1.0f, 0.0f };


static float Lim01G(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalG(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoG(Color c, float f)
{
    return { CanalG((float)c.r * f), CanalG((float)c.g * f), CanalG((float)c.b * f), c.a };
}


static Color MezclaG(Color a, Color b, float t)
{
    t = Lim01G(t);
    return {
        CanalG((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalG((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalG((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalG((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaG(Color c, float alfa)
{
    c.a = CanalG((float)c.a * Lim01G(alfa));
    return c;
}


// Variacion de brillo por pieza (hash determinista).
static Color VariarG(Color c, int indice, int semilla, float amplitud)
{
    return TonoG(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01Grua(indice, semilla));
}


static Color NieblaG(Color c, Vector3 p)
{
    float dx = p.x - g_ojoG.x;
    float dy = p.y - g_ojoG.y;
    float dz = p.z - g_ojoG.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01G((d - 24.0f) / 64.0f) * 0.86f * (1.0f - 0.7f * g_emisG);
    Color r = MezclaG(c, COLOR_NIEBLA_G, n);
    r.a = c.a;
    return r;
}


static void VG(Vector3 p, Color c)
{
    if (g_grabG.activa)
    {
        g_grabG.posiciones.push_back(p.x);
        g_grabG.posiciones.push_back(p.y);
        g_grabG.posiciones.push_back(p.z);
        g_grabG.colores.push_back(c.r);
        g_grabG.colores.push_back(c.g);
        g_grabG.colores.push_back(c.b);
        g_grabG.colores.push_back(c.a);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteG()
{
    if (!g_grabG.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteG()
{
    if (!g_grabG.activa) rlEnd();
}


static void TriG(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VG(a, NieblaG(color, a));
    VG(b, NieblaG(color, b));
    VG(c, NieblaG(color, c));
}


static void QuadG(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriG(a, b, c, color);
    TriG(a, c, d, color);
}


// Quad sin bruma con un color por vertice (luces aditivas y sombras suaves).
static void QuadColoresG(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VG(a, ca); VG(b, cb); VG(c, cc);
    VG(a, ca); VG(c, cc); VG(d, cd);
}


// Sombreado por caras de la guia (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62) interpolado para normales cualesquiera; lo que mira arriba
// recibe la luz tibia del sol bajo.
static float LuzG(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f)
    {
        return 1.0f;
    }

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.42f) +
        (n.z > 0.0f ? az * 0.78f : az * 0.54f) +
        (n.x < 0.0f ? ax * 0.70f : ax * 0.62f);

    return f / suma;
}


static Color CaraG(Color c, Vector3 n)
{
    Color r = TonoG(c, LuzG(n));

    if (n.y > 0.3f)
    {
        r = MezclaG(r, COLOR_TINTE_SOL_G, 0.14f * n.y);
    }

    return r;
}



// Caja con medio-ejes arbitrarios (ex, ey, ez): caras sombreadas por normal.
static void CajaEjesG(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
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

    QuadG(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraG(color, ny));
    QuadG(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraG(color, nz));
    QuadG(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraG(color, Vector3Negate(nz)));
    QuadG(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraG(color, Vector3Negate(nx)));
    QuadG(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraG(color, nx));

    if (fondo)
    {
        QuadG(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraG(color, Vector3Negate(ny)));
    }
}


static void CajaG(Vector3 c, Vector3 t, Color color)
{
    CajaEjesG(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, c.y - t.y * 0.5f > 0.05f);
}


// Caja girada en Y (grados) y, opcionalmente, ladeada sobre su eje largo X.
static void CajaGiradaG(Vector3 c, Vector3 t, float giroY, Color color, float ladeo = 0.0f)
{
    float a = giroY * DEG2RAD;
    float l = ladeo * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz0 = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = { -uz0.x * std::sin(l), std::cos(l), -uz0.z * std::sin(l) };
    Vector3 uz = { uz0.x * std::cos(l), std::sin(l), uz0.z * std::cos(l) };

    CajaEjesG(c, Vector3Scale(ux, t.x * 0.5f), Vector3Scale(uy, t.y * 0.5f), Vector3Scale(uz, t.z * 0.5f), color, true);
}


// Viga de seccion cuadrada entre dos puntos (celosias, barandas, cables gruesos).
static void VigaG(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesG(centro, Vector3Scale(v, grosor * 0.5f), Vector3Scale(w, grosor * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas por normal.
static void CilG(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

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

        QuadG(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraG(color, nm)
        );

        if (tapas)
        {
            TriG(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraG(color, eje));
            TriG(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraG(color, Vector3Negate(eje)));
        }
    }
}


static void BolaG(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8)
{
    for (int i = 0; i < anillos; i++)
    {
        float la0 = -0.5f * PI + PI * (float)i / (float)anillos;
        float la1 = -0.5f * PI + PI * (float)(i + 1) / (float)anillos;

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
            QuadG(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraG(color, n));
        }
    }
}


// Disco horizontal (charcos, manchas, tapas) con color de borde propio.
static void DiscoSueloG(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, bool niebla = true)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };

        if (niebla)
        {
            VG(c, NieblaG(centro, c)); VG(p0, NieblaG(borde, p0)); VG(p1, NieblaG(borde, p1));
        }
        else
        {
            VG(c, centro); VG(p0, borde); VG(p1, borde);
        }
    }
}


static void AnilloSueloG(Vector3 c, float r0, float r1, int lados, Color color, float desde = 0.0f, float hasta = 2.0f * PI)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = desde + (hasta - desde) * (float)k / (float)lados;
        float a1 = desde + (hasta - desde) * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColoresG(i0, o0, o1, i1, color, color, color, color);
    }
}


// Tira delgada entre dos puntos que siempre mira a la camara (cables).
static void TiraG(Vector3 a, Vector3 b, float ancho, Color ca, Color cb)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 vista = Vector3Subtract(Vector3Scale(Vector3Add(a, b), 0.5f), g_camG);
    Vector3 lado = Vector3CrossProduct(eje, vista);
    float l = Vector3Length(lado);

    if (l < 0.0001f)
    {
        return;
    }

    lado = Vector3Scale(lado, ancho * 0.5f / l);
    QuadColoresG(Vector3Subtract(a, lado), Vector3Add(a, lado), Vector3Add(b, lado), Vector3Subtract(b, lado), ca, ca, cb, cb);
}


// Disco que mira a la camara con centro y borde de distinto color (halos).
static void HaloG(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derG, std::cos(a0) * radio), Vector3Scale(g_arribaG, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derG, std::cos(a1) * radio), Vector3Scale(g_arribaG, std::sin(a1) * radio)));
        VG(c, centro); VG(p0, borde); VG(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas).
static void PuntoG(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derG, r), u = Vector3Scale(g_arribaG, r);
    QuadColoresG(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


//--------------------------------------------------
// Horno: lo estatico se graba una vez en dos mallas
//--------------------------------------------------

struct HornoGrua
{
    bool listo = false;
    int clave = -1;
    Mesh suelo{};
    bool sueloValido = false;
    Mesh escena{};
    bool escenaValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoGrua g_hornoG;


static void EmpezarGrabacionG()
{
    g_grabG.activa = true;
    g_grabG.posiciones.clear();
    g_grabG.colores.clear();
}


// Cierra la grabacion y sube la malla (sin indices: cabe cualquier tamano).
static bool TerminarGrabacionG(Mesh& malla)
{
    g_grabG.activa = false;
    int n = (int)(g_grabG.posiciones.size() / 3);
    n -= n % 3;

    if (n < 3)
    {
        g_grabG.posiciones.clear();
        g_grabG.colores.clear();
        return false;
    }

    malla = {};
    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * (int)sizeof(float)));
    malla.texcoords = (float*)MemAlloc((unsigned int)(n * 2 * (int)sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    for (int i = 0; i < n * 3; i++) malla.vertices[i] = g_grabG.posiciones[(size_t)i];
    for (int i = 0; i < n * 4; i++) malla.colors[i] = g_grabG.colores[(size_t)i];
    for (int i = 0; i < n * 2; i++) malla.texcoords[i] = 0.0f;

    UploadMesh(&malla, false);
    g_grabG.posiciones.clear();
    g_grabG.colores.clear();
    return true;
}


static void LiberarMallaG(Mesh& malla, bool& valida)
{
    if (valida) UnloadMesh(malla);
    valida = false;
    malla = {};
}


static void LiberarMallasGrua()
{
    LiberarMallaG(g_hornoG.suelo, g_hornoG.sueloValido);
    LiberarMallaG(g_hornoG.escena, g_hornoG.escenaValida);
    g_hornoG.listo = false;
    g_hornoG.clave = -1;
}


static void SueloEstaticoG(int n, bool decoracion, bool sombras);
static void EscenaEstaticaG(int n, bool decoracion, int paso);


static int ClaveHornoG(int n)
{
    return n * 16 + (int)CalidadDecoracion() * 2 + (CalidadSombras() != CALIDAD_BAJA ? 1 : 0);
}


static void HornearGrua(int n)
{
    int clave = ClaveHornoG(n);

    if ((g_hornoG.listo && g_hornoG.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasGrua();

    if (!g_hornoG.materialListo)
    {
        g_hornoG.material = LoadMaterialDefault();
        g_hornoG.materialListo = true;
    }

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    Vector3 ojo = g_ojoG;
    g_ojoG = OJO_GRUA;

    EmpezarGrabacionG();
    SueloEstaticoG(n, decoracion, sombras);
    g_hornoG.sueloValido = TerminarGrabacionG(g_hornoG.suelo);

    EmpezarGrabacionG();
    EscenaEstaticaG(n, decoracion, paso);
    g_hornoG.escenaValida = TerminarGrabacionG(g_hornoG.escena);

    g_ojoG = ojo;
    g_hornoG.clave = clave;
    g_hornoG.listo = true;
}


// El suelo va sin prueba de profundidad (tierra, losas, manchas y sombras se
// pintan en orden, sin parpadeos entre capas coplanares).
static void DibujarHornoSueloG()
{
    if (!g_hornoG.listo || !g_hornoG.sueloValido)
    {
        return;
    }

    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    DrawMesh(g_hornoG.suelo, g_hornoG.material, MatrixIdentity());
    rlDrawRenderBatchActive();
    rlEnableDepthTest();
}


static void DibujarHornoEscenaG()
{
    if (!g_hornoG.listo || !g_hornoG.escenaValida)
    {
        return;
    }

    rlDrawRenderBatchActive();
    DrawMesh(g_hornoG.escena, g_hornoG.material, MatrixIdentity());
}


static Camera3D CamaraVistaGrua(const MinijuegoGruaChatarra& m)
{
    Camera3D c = m.camara;
    c.position = OJO_GRUA;
    c.target = MIRA_GRUA;
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = FOV_GRUA;
    c.projection = CAMERA_PERSPECTIVE;

    // Entrada: la camara se acerca desde atras y de costado en la cuenta.
    if (m.fase == FASE_GRUA_PREPARACION && m.partidaValida)
    {
        float u = Lim01G(1.0f - m.tiempoPreparacion / DURACION_PREPARACION_GRUA);
        float e = 1.0f - u;
        e = e * e * (3.0f - 2.0f * e) * ObtenerFactorMovimientoCamaraMinijuegos();
        Vector3 atras = Vector3Normalize(Vector3Subtract(OJO_GRUA, MIRA_GRUA));
        c.position = Vector3Add(c.position, Vector3Scale(atras, 7.0f * e));
        c.position.x -= 4.5f * e;
        c.position.y += 1.2f * e;
        c.target.x -= 1.2f * e;
    }

    return c;
}


//==================================================
// VISUAL: PIEZAS JUGABLES (GLB o primitivas)
//==================================================

static float AlturaGarraGrua(const EstadoJugadorGrua& estado)
{
    if (estado.estado == GARRA_GRUA_LIBRE)
    {
        return ALTURA_ALTA_GRUA;
    }

    float t = estado.tiempo;

    if (t < T_BAJAR_GRUA)
    {
        float f = t / T_BAJAR_GRUA;
        return ALTURA_ALTA_GRUA + (ALTURA_BAJA_GRUA - ALTURA_ALTA_GRUA) * f * f;
    }

    if (t < T_CERRAR_GRUA)
    {
        return ALTURA_BAJA_GRUA;
    }

    if (t < T_SUBIR_GRUA)
    {
        float f = (t - T_CERRAR_GRUA) / (T_SUBIR_GRUA - T_CERRAR_GRUA);
        return ALTURA_BAJA_GRUA + (ALTURA_ALTA_GRUA - ALTURA_BAJA_GRUA) * f;
    }

    return ALTURA_ALTA_GRUA;
}


static float AperturaGarraGrua(const EstadoJugadorGrua& estado)
{
    float apertura = 0.6f;

    if (estado.estado == GARRA_GRUA_ACCION)
    {
        if (estado.tiempo >= T_BAJAR_GRUA && estado.tiempo < T_CERRAR_GRUA)
        {
            apertura = 0.6f - 0.35f * (estado.tiempo - T_BAJAR_GRUA) / (T_CERRAR_GRUA - T_BAJAR_GRUA);
        }
        else if (estado.tiempo >= T_CERRAR_GRUA)
        {
            apertura = estado.tipoCapturado >= 0 ? 0.25f : 0.25f + 0.35f * LimitarGrua((estado.tiempo - T_CERRAR_GRUA) / 0.45f, 0.0f, 1.0f);
        }
    }

    return apertura;
}


static bool LlevaObjetoGrua(const EstadoJugadorGrua& estado)
{
    return estado.estado == GARRA_GRUA_ACCION &&
        estado.tipoCapturado >= 0 &&
        !estado.depositado &&
        estado.tiempo >= T_CERRAR_GRUA;
}


// Pivote al pie: igual para la chatarra del pozo y la transportada.
static void DibujarObjetoGrua(int tipo, float x, float y, float z, float giro)
{
    float rad = giro * 0.0174533f;
    const ModeloGruaChatarra3D piezas[] = { MODELO_GRUA_TUERCA,
        MODELO_GRUA_ENGRANAJE, MODELO_GRUA_MOTOR, MODELO_GRUA_BATERIA,
        MODELO_GRUA_CARTUCHO };

    if (DibujarModeloGruaChatarraRetro3D(piezas[tipo], { x, y, z }, giro))
    {
        return;
    }

    if (tipo == OBJETO_GRUA_TUERCA)
    {
        (DrawCylinder)({ x, y, z }, 0.38f, 0.38f, 0.22f, 6, Color{ 170, 176, 186, 255 });
        (DrawCylinder)({ x, y + 0.2f, z }, 0.16f, 0.16f, 0.04f, 6, Color{ 50, 52, 58, 255 });
    }
    else if (tipo == OBJETO_GRUA_ENGRANAJE)
    {
        (DrawCylinder)({ x, y, z }, 0.5f, 0.5f, 0.2f, 10, Color{ 224, 140, 52, 255 });
        (DrawCylinder)({ x, y + 0.18f, z }, 0.18f, 0.18f, 0.06f, 8, Color{ 70, 60, 54, 255 });

        for (int k = 0; k < 6; k++)
        {
            float a = rad + (float)k * 1.0471976f;
            (DrawCube)({ x + std::cos(a) * 0.55f, y + 0.1f, z + std::sin(a) * 0.55f }, 0.18f, 0.18f, 0.18f, Color{ 224, 140, 52, 255 });
        }
    }
    else if (tipo == OBJETO_GRUA_MOTOR)
    {
        (DrawCube)({ x, y + 0.4f, z }, 0.95f, 0.7f, 0.7f, Color{ 90, 110, 140, 255 });
        (DrawCylinder)({ x - 0.25f, y + 0.75f, z }, 0.12f, 0.12f, 0.3f, 6, Color{ 60, 66, 76, 255 });
        (DrawCylinder)({ x + 0.25f, y + 0.75f, z }, 0.12f, 0.12f, 0.3f, 6, Color{ 60, 66, 76, 255 });
    }
    else if (tipo == OBJETO_GRUA_BATERIA)
    {
        (DrawCube)({ x, y + 0.35f, z }, 0.6f, 0.7f, 0.45f, Color{ 255, 205, 60, 255 });
        (DrawCube)({ x - 0.15f, y + 0.78f, z }, 0.1f, 0.12f, 0.1f, Color{ 60, 60, 66, 255 });
        (DrawCube)({ x + 0.15f, y + 0.78f, z }, 0.1f, 0.12f, 0.1f, Color{ 60, 60, 66, 255 });
    }
    else
    {
        (DrawCylinder)({ x, y, z }, 0.32f, 0.32f, 0.55f, 8, Color{ 120, 60, 40, 255 });
        (DrawCylinder)({ x, y + 0.2f, z }, 0.335f, 0.335f, 0.1f, 8, Color{ 220, 190, 40, 255 });
    }
}


// El GLB incluye la plataforma y el respaldo del operador.
static void DibujarTolvaGrua(float x, Color color)
{
    if (DibujarModeloGruaChatarraRetro3D(MODELO_GRUA_TOLVA, { x, 0.0f, Z_TOLVA_GRUA }, 0, { 0, 1, 0 }, { 1, 1, 1 }, color))
    {
        return;
    }

    (DrawCube)({ x, 0.6f, Z_TOLVA_GRUA }, 2.4f, 1.2f, 1.6f, Color{ 60, 62, 70, 255 });
    (DrawCube)({ x, 1.25f, Z_TOLVA_GRUA + 0.78f }, 2.4f, 0.12f, 0.1f, color);
    (DrawCylinder)({ x, 1.2f, Z_TOLVA_GRUA }, 1.2f, 0.7f, 0.5f, 8, color);
    (DrawCube)({ x, 0.15f, Z_TOLVA_GRUA - 1.5f }, 2.0f, 0.3f, 1.6f, Color{ 90, 94, 104, 255 });
    (DrawCube)({ x, 0.65f, Z_TOLVA_GRUA - 2.2f }, 1.6f, 0.9f, 0.2f, Color{ 52, 54, 62, 255 });
}


// Garra: marca en el suelo, cable, carro, iman, pinzas y lo que lleva.
static void DibujarGarraGrua(const EstadoJugadorGrua& estado, Color color, float t)
{
    float altura = AlturaGarraGrua(estado);
    float x = estado.x;
    float z = estado.z;
    float apertura = AperturaGarraGrua(estado);

    if (!DibujarModeloGruaChatarraRetro3D(MODELO_GRUA_MARCA, { x, 0, z }, 0, { 0, 1, 0 }, { 1, 1, 1 }, Fade(color, 0.9f)))
    {
        DrawCircle3D({ x, 0.04f, z }, 0.55f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.9f));
        DrawCircle3D({ x, 0.045f, z }, 0.08f, { 1.0f, 0.0f, 0.0f }, 90.0f, WHITE);
    }

    // Cable desde el carro del cablecam (el grueso se agrega aparte).
    DrawLine3D({ x, 8.0f, z }, { x, altura + 0.35f, z }, Color{ 40, 40, 46, 255 });

    if (!DibujarModeloGruaChatarraRetro3D(MODELO_GRUA_CARRO, { x, 8, z }))
    {
        (DrawCube)({ x, 8.0f, z }, 0.7f, 0.3f, 0.7f, Color{ 60, 62, 70, 255 });
    }

    if (!DibujarModeloGruaChatarraRetro3D(MODELO_GRUA_IMAN, { x, altura, z }, 0, { 0, 1, 0 }, { 1, 1, 1 }, color))
    {
        (DrawCylinder)({ x, altura, z }, 0.45f, 0.5f, 0.3f, 12, color);
        (DrawCylinder)({ x, altura + 0.3f, z }, 0.2f, 0.2f, 0.2f, 8, Color{ 60, 62, 70, 255 });
    }

    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.5707963f + 0.7853982f;

        // La punta local apunta hacia -Z: girar hacia el centro del iman.
        if (!DibujarModeloGruaChatarraRetro3D(MODELO_GRUA_PINZA,
            { x + std::cos(a) * apertura * 0.7f, altura - 0.02f, z + std::sin(a) * apertura * 0.7f },
            90 - a * RAD2DEG))
        {
            (DrawCube)({ x + std::cos(a) * apertura * 0.7f, altura - 0.2f, z + std::sin(a) * apertura * 0.7f }, 0.1f, 0.5f, 0.1f, Color{ 190, 190, 200, 255 });
        }
    }

    if (LlevaObjetoGrua(estado))
    {
        DibujarObjetoGrua(estado.tipoCapturado, x, altura - 0.75f, z, t * 40.0f);
    }
}


//==================================================
// VISUAL: ESCENARIO ESTATICO (horneado)
//==================================================

static const float Z_FIN_SUELO_G = -15.5f;


static void RectSueloG(float x0, float z0, float x1, float z1, float y, Color color)
{
    QuadG({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, color);
}


// Sombra suave en el suelo: el sol esta detras, asi que se estira hacia
// la camara (+Z).
static void SombraSueloG(float x, float z, float rx, float rz, float alfa)
{
    Color centro = AlfaG(Color{ 30, 18, 24, 255 }, alfa);
    Color borde = AlfaG(Color{ 30, 18, 24, 255 }, 0.0f);
    DiscoSueloG({ x, 0.0f, z }, rx, rz, 16, centro, borde, false);
}


// Mancha irregular plana (aceite, oxido, charco) con borde difuso.
static void ManchaSueloG(float x, float z, float r, int semilla, Color centro, Color borde)
{
    const int lados = 12;

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float r0 = r * (0.7f + 0.5f * Ruido01Grua(k, semilla));
        float r1 = r * (0.7f + 0.5f * Ruido01Grua((k + 1) % lados, semilla));
        Vector3 c = { x, 0.0f, z };
        Vector3 p0 = { x + std::cos(a0) * r0, 0.0f, z + std::sin(a0) * r0 * 0.8f };
        Vector3 p1 = { x + std::cos(a1) * r1, 0.0f, z + std::sin(a1) * r1 * 0.8f };
        VG(c, NieblaG(centro, c)); VG(p0, NieblaG(borde, p0)); VG(p1, NieblaG(borde, p1));
    }
}


static bool DentroDeG(float x, float z, float x0, float z0, float x1, float z1)
{
    return x > x0 && x < x1 && z > z0 && z < z1;
}


static void SueloEstaticoG(int n, bool decoracion, bool sombras)
{
    (void)n;
    LoteG();

    // Patio de tierra apisonada por parcelas con variacion.
    const float paso = 2.0f;
    int ix = 0;

    for (float x = -36.0f; x < 36.0f; x += paso, ix++)
    {
        int iz = 0;

        for (float z = Z_FIN_SUELO_G; z < 14.0f; z += paso, iz++)
        {
            int h = ix * 61 + iz * 7;
            Color c = MezclaG(COLOR_TIERRA_G, COLOR_TIERRA_OSCURA_G, 0.15f + 0.35f * Ruido01Grua(h, 3));
            c = VariarG(c, h, 4, 0.06f);
            RectSueloG(x, z, x + paso, z + paso, 0.0f, c);
        }
    }

    // Huellas de neumaticos que rodean el pozo.
    Color huella = AlfaG(Color{ 58, 44, 40, 255 }, 0.55f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int r = 0; r < 2; r++)
        {
            float x = (float)lado * (15.0f + (float)r * 1.1f);
            RectSueloG(x - 0.22f, -12.0f, x + 0.22f, 13.0f, 0.0f, huella);
        }
    }

    for (int r = 0; r < 2; r++)
    {
        float z = 8.4f + (float)r * 1.1f;
        RectSueloG(-17.0f, z - 0.22f, 17.0f, z + 0.22f, 0.0f, huella);
    }

    // Grava suelta.
    int granos = decoracion ? 520 : 160;

    for (int i = 0; i < granos; i++)
    {
        float x = -30.0f + Ruido01Grua(i, 11) * 60.0f;
        float z = Z_FIN_SUELO_G + Ruido01Grua(i, 12) * 28.0f;

        if (DentroDeG(x, z, -9.7f, -5.5f, 9.7f, 6.3f))
        {
            continue;
        }

        float t = 0.05f + Ruido01Grua(i, 13) * 0.09f;
        Color c = Ruido01Grua(i, 14) < 0.5f ? Color{ 140, 118, 100, 255 } : Color{ 70, 56, 52, 255 };
        RectSueloG(x - t, z - t, x + t, z + t, 0.0f, c);
    }

    // Manchas de aceite en el patio.
    for (int i = 0; i < (decoracion ? 14 : 6); i++)
    {
        float x = -26.0f + Ruido01Grua(i, 21) * 52.0f;
        float z = -13.0f + Ruido01Grua(i, 22) * 24.0f;

        if (DentroDeG(x, z, -9.7f, -5.5f, 9.7f, 6.3f))
        {
            continue;
        }

        ManchaSueloG(x, z, 0.5f + Ruido01Grua(i, 23) * 0.9f, i + 40, AlfaG(Color{ 40, 32, 34, 255 }, 0.6f), AlfaG(Color{ 40, 32, 34, 255 }, 0.0f));
    }

    // Charcos que reflejan el cielo del atardecer.
    if (decoracion)
    {
        const float charcos[6][3] = {
            { -13.4f, -8.6f, 1.2f }, { 12.8f, 5.6f, 0.9f }, { 17.6f, -6.4f, 1.4f },
            { -18.0f, 2.4f, 1.0f }, { 6.0f, -11.6f, 1.1f }, { -6.8f, -12.4f, 0.8f }
        };

        for (int i = 0; i < 6; i++)
        {
            float x = charcos[i][0], z = charcos[i][1], r = charcos[i][2];
            ManchaSueloG(x, z, r * 1.15f, i + 70, Color{ 70, 50, 48, 255 }, AlfaG(Color{ 70, 50, 48, 255 }, 0.0f));
            ManchaSueloG(x, z, r, i + 70, Color{ 236, 150, 112, 255 }, Color{ 150, 88, 92, 255 });
            ManchaSueloG(x - r * 0.2f, z - r * 0.15f, r * 0.35f, i + 90, Color{ 255, 206, 150, 255 }, AlfaG(Color{ 255, 206, 150, 255 }, 0.0f));
        }
    }

    // Plataforma de chapa detras del pozo (tolvas y operadores).
    for (int i = 0; i < 16; i++)
    {
        float x0 = -12.0f + (float)i * 1.5f;
        Color c = VariarG(Color{ 92, 92, 100, 255 }, i, 31, 0.05f);
        RectSueloG(x0, -9.6f, x0 + 1.5f, -5.4f, 0.0f, c);
        RectSueloG(x0 - 0.02f, -9.6f, x0 + 0.02f, -5.4f, 0.0f, Color{ 56, 56, 64, 255 });

        // Relieve de lagrimas.
        for (int k = 0; k < 6; k++)
        {
            for (int j = 0; j < 3; j++)
            {
                float lx = x0 + 0.25f + (float)j * 0.5f;
                float lz = -9.3f + (float)k * 0.66f + ((j % 2) ? 0.33f : 0.0f);
                RectSueloG(lx - 0.1f, lz - 0.03f, lx + 0.1f, lz + 0.03f, 0.0f, Color{ 118, 118, 126, 255 });
            }
        }
    }

    // Franja de peligro (ocre y negro) en el borde del pozo.
    for (int i = 0; i < 48; i++)
    {
        float x0 = -12.0f + (float)i * 0.5f;
        Color c = (i % 2 == 0) ? COLOR_OCRE_G : Color{ 34, 30, 34, 255 };
        QuadG({ x0, 0.0f, -5.75f }, { x0 + 0.2f, 0.0f, -5.4f }, { x0 + 0.7f, 0.0f, -5.4f }, { x0 + 0.5f, 0.0f, -5.75f }, c);
    }

    // Fondo del pozo: losas de hormigon con juntas, aceite y oxido.
    const int columnas = 12, filas = 7;
    float anchoLosa = 2.0f * X_POZO_G / (float)columnas;
    float largoLosa = (Z_POZO_FRENTE_G - Z_POZO_FONDO_G) / (float)filas;

    for (int c = 0; c < columnas; c++)
    {
        for (int f = 0; f < filas; f++)
        {
            float x0 = -X_POZO_G + (float)c * anchoLosa;
            float z0 = Z_POZO_FONDO_G + (float)f * largoLosa;
            Color color = VariarG(COLOR_HORMIGON_G, c * 17 + f, 41, 0.07f);
            color = MezclaG(color, Color{ 98, 80, 70, 255 }, 0.35f * Ruido01Grua(c * 17 + f, 42));
            RectSueloG(x0, z0, x0 + anchoLosa, z0 + largoLosa, 0.0f, color);
        }
    }

    Color junta = Color{ 64, 56, 54, 255 };

    for (int c = 0; c <= columnas; c++)
    {
        float x = -X_POZO_G + (float)c * anchoLosa;
        RectSueloG(x - 0.03f, Z_POZO_FONDO_G, x + 0.03f, Z_POZO_FRENTE_G, 0.0f, junta);
    }

    for (int f = 0; f <= filas; f++)
    {
        float z = Z_POZO_FONDO_G + (float)f * largoLosa;
        RectSueloG(-X_POZO_G, z - 0.03f, X_POZO_G, z + 0.03f, 0.0f, junta);
    }

    // Grietas finas.
    if (decoracion)
    {
        for (int i = 0; i < 10; i++)
        {
            float x = -8.0f + Ruido01Grua(i, 51) * 16.0f;
            float z = -4.4f + Ruido01Grua(i, 52) * 9.0f;
            float a = Ruido01Grua(i, 53) * PI;

            for (int k = 0; k < 4; k++)
            {
                float l = 0.35f + 0.3f * Ruido01Grua(i * 4 + k, 54);
                float x1 = x + std::cos(a) * l, z1 = z + std::sin(a) * l;
                TiraG({ x, 0.0f, z }, { x1, 0.0f, z1 }, 0.035f, Color{ 62, 54, 52, 255 }, Color{ 62, 54, 52, 255 });
                x = x1;
                z = z1;
                a += (Ruido01Grua(i * 4 + k, 55) - 0.5f) * 1.4f;
            }
        }
    }

    // Aceite y oxido sobre las losas (suaves: la chatarra manda).
    for (int i = 0; i < (decoracion ? 12 : 5); i++)
    {
        float x = -8.0f + Ruido01Grua(i, 61) * 16.0f;
        float z = -4.4f + Ruido01Grua(i, 62) * 9.4f;
        bool aceite = (i % 3) != 0;
        Color centro = aceite ? AlfaG(Color{ 46, 38, 40, 255 }, 0.5f) : AlfaG(Color{ 150, 82, 50, 255 }, 0.45f);
        ManchaSueloG(x, z, 0.45f + Ruido01Grua(i, 63) * 0.6f, i + 120, centro, AlfaG(centro, 0.0f));
    }

    // Linea de carga pintada (ocre gastado) a lo largo del borde interior.
    Color pintura = AlfaG(Color{ 210, 150, 70, 255 }, 0.55f);
    float m = 0.3f;
    RectSueloG(-X_POZO_G + m, Z_POZO_FONDO_G + m, X_POZO_G - m, Z_POZO_FONDO_G + m + 0.1f, 0.0f, pintura);
    RectSueloG(-X_POZO_G + m, Z_POZO_FRENTE_G - m - 0.1f, X_POZO_G - m, Z_POZO_FRENTE_G - m, 0.0f, pintura);
    RectSueloG(-X_POZO_G + m, Z_POZO_FONDO_G + m, -X_POZO_G + m + 0.1f, Z_POZO_FRENTE_G - m, 0.0f, pintura);
    RectSueloG(X_POZO_G - m - 0.1f, Z_POZO_FONDO_G + m, X_POZO_G - m, Z_POZO_FRENTE_G - m, 0.0f, pintura);

    // Rejillas de desague.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 7.6f;
        RectSueloG(x - 0.45f, -0.3f, x + 0.45f, 0.3f, 0.0f, Color{ 52, 50, 56, 255 });

        for (int k = 0; k < 5; k++)
        {
            float bx = x - 0.36f + (float)k * 0.18f;
            RectSueloG(bx - 0.03f, -0.26f, bx + 0.03f, 0.26f, 0.0f, Color{ 104, 104, 112, 255 });
        }
    }

    // Sombras de la utileria grande (estiradas hacia la camara).
    if (sombras)
    {
        SombraSueloG(-14.6f, -0.6f, 1.9f, 3.0f, 0.42f);
        SombraSueloG(-14.2f, 4.2f, 1.6f, 2.6f, 0.38f);
        SombraSueloG(CENTRO_PRENSA_GRUA.x, CENTRO_PRENSA_GRUA.z + 1.2f, 2.4f, 3.2f, 0.45f);
        SombraSueloG(-16.6f, -9.4f, 3.6f, 2.2f, 0.4f);
        SombraSueloG(17.6f, 5.8f, 2.0f, 1.8f, 0.36f);
        SombraSueloG(16.8f, 0.4f, 1.6f, 1.6f, 0.36f);

        for (int t = 0; t < 2; t++)
        {
            SombraSueloG(TORRES_GRUA[t].x, TORRES_GRUA[t].z + 0.8f, 1.1f, 1.6f, 0.35f);
        }

        // Paredes del pozo: sombra corta sobre las losas.
        RectSueloG(-X_POZO_G, Z_POZO_FONDO_G, X_POZO_G, Z_POZO_FONDO_G + 0.4f, 0.0f, AlfaG(Color{ 30, 18, 24, 255 }, 0.3f));
    }

    FinLoteG();
}


//--------------------------------------------------
// Utileria del anillo cercano
//--------------------------------------------------

static Vector3 LocalG(Vector3 base, float giroY, Vector3 local)
{
    float a = giroY * DEG2RAD;
    float c = std::cos(a), s = std::sin(a);
    return { base.x + local.x * c + local.z * s, base.y + local.y, base.z - local.x * s + local.z * c };
}


// Auto aplastado: losa de carroceria con vidrios, ruedas y oxido.
static void AutoAplastadoG(Vector3 base, float giro, float ladeo, Color color, int semilla, float alto = 0.48f)
{
    Vector3 c = { base.x, base.y + alto * 0.5f, base.z };
    CajaGiradaG(c, { 1.75f, alto, 3.5f }, giro, color, ladeo);

    // Techo arrugado y franja de vidrios.
    CajaGiradaG(LocalG(c, giro, { 0.0f, alto * 0.5f + 0.05f, -0.2f + 0.3f * Ruido01Grua(semilla, 1) }), { 1.3f, 0.1f, 1.5f }, giro + 4.0f * (Ruido01Grua(semilla, 2) - 0.5f), TonoG(color, 0.86f));
    CajaGiradaG(LocalG(c, giro, { 0.0f, 0.02f, -0.2f }), { 1.78f, alto * 0.32f, 1.6f }, giro, Color{ 48, 58, 70, 255 }, ladeo);

    // Ruedas aplastadas y paragolpes.
    for (int k = 0; k < 4; k++)
    {
        float lx = (k % 2 == 0) ? -0.9f : 0.9f;
        float lz = (k < 2) ? -1.15f : 1.15f;
        Vector3 r = LocalG(c, giro, { lx, -alto * 0.18f, lz });
        CajaGiradaG(r, { 0.14f, alto * 0.7f, 0.62f }, giro, COLOR_GOMA_G);
    }

    CajaGiradaG(LocalG(c, giro, { 0.0f, -alto * 0.1f, 1.78f }), { 1.6f, alto * 0.35f, 0.1f }, giro, COLOR_ACERO_G);

    if (Ruido01Grua(semilla, 3) < 0.6f)
    {
        CajaGiradaG(LocalG(c, giro, { 0.5f * (Ruido01Grua(semilla, 4) - 0.5f), alto * 0.5f + 0.01f, 1.0f }), { 0.6f, 0.03f, 0.7f }, giro, COLOR_OXIDO_G);
    }
}


static void PilaAutosG(float x, float z, int cantidad, int semilla, float giroBase)
{
    float y = 0.0f;

    for (int k = 0; k < cantidad; k++)
    {
        int s = semilla * 13 + k;
        float giro = giroBase + (Ruido01Grua(s, 5) - 0.5f) * 16.0f;
        float alto = 0.42f + Ruido01Grua(s, 6) * 0.14f;
        Color color = COLORES_AUTO_G[(semilla + k * 5) % 6];
        float dx = (Ruido01Grua(s, 7) - 0.5f) * 0.3f;
        float dz = (Ruido01Grua(s, 8) - 0.5f) * 0.4f;
        AutoAplastadoG({ x + dx, y, z + dz }, giro, (Ruido01Grua(s, 9) - 0.5f) * 5.0f, VariarG(color, s, 10, 0.08f), s, alto);
        y += alto + 0.04f;
    }
}


static void BarrilG(Vector3 base, Color color, bool tumbado, float giro, int semilla)
{
    Color oscuro = TonoG(color, 0.7f);

    if (!tumbado)
    {
        CilG(base, { base.x, base.y + 0.92f, base.z }, 0.34f, 0.34f, 10, color);
        CilG({ base.x, base.y + 0.28f, base.z }, { base.x, base.y + 0.34f, base.z }, 0.355f, 0.355f, 10, oscuro, false);
        CilG({ base.x, base.y + 0.6f, base.z }, { base.x, base.y + 0.66f, base.z }, 0.355f, 0.355f, 10, oscuro, false);
        DiscoSueloG({ base.x + 0.12f, base.y + 0.925f, base.z + 0.05f }, 0.07f, 0.07f, 6, COLOR_ACERO_OSCURO_G, COLOR_ACERO_OSCURO_G);

        if (Ruido01Grua(semilla, 1) < 0.5f)
        {
            CajaG({ base.x - 0.15f, base.y + 0.45f, base.z + 0.3f }, { 0.22f, 0.3f, 0.06f }, COLOR_OXIDO_G);
        }
        return;
    }

    Vector3 eje = LocalG({ 0, 0, 0 }, giro, { 0.0f, 0.0f, 0.46f });
    Vector3 c = { base.x, base.y + 0.34f, base.z };
    CilG(Vector3Subtract(c, eje), Vector3Add(c, eje), 0.34f, 0.34f, 10, color);
    Vector3 anillo = Vector3Scale(eje, 0.4f);
    CilG(Vector3Subtract(c, anillo), Vector3Add(Vector3Subtract(c, anillo), Vector3Scale(eje, 0.12f)), 0.355f, 0.355f, 10, oscuro, false);
}


static void NeumaticoG(Vector3 base, float r)
{
    CilG(base, { base.x, base.y + 0.28f, base.z }, r, r, 12, COLOR_GOMA_G, false);
    AnilloSueloG({ base.x, base.y + 0.281f, base.z }, r * 0.55f, r, 12, NieblaG(Color{ 54, 50, 56, 255 }, base));
    DiscoSueloG({ base.x, base.y + 0.12f, base.z }, r * 0.55f, r * 0.55f, 10, Color{ 20, 18, 22, 255 }, Color{ 20, 18, 22, 255 });
}


static void PilaNeumaticosG(float x, float z, int cantidad, int semilla)
{
    for (int k = 0; k < cantidad; k++)
    {
        float dx = (Ruido01Grua(semilla * 7 + k, 1) - 0.5f) * 0.12f;
        NeumaticoG({ x + dx, (float)k * 0.29f, z }, 0.44f);
    }
}


static void ConoG(float x, float z)
{
    CajaG({ x, 0.03f, z }, { 0.42f, 0.06f, 0.42f }, Color{ 40, 36, 40, 255 });
    CilG({ x, 0.06f, z }, { x, 0.72f, z }, 0.17f, 0.03f, 8, Color{ 232, 112, 44, 255 });
    CilG({ x, 0.32f, z }, { x, 0.44f, z }, 0.125f, 0.105f, 8, Color{ 240, 232, 220, 255 }, false);
}


static void PaletG(Vector3 base, float giro)
{
    Color madera = Color{ 160, 118, 78, 255 };

    for (int k = 0; k < 3; k++)
    {
        CajaGiradaG(LocalG(base, giro, { -0.5f + (float)k * 0.5f, 0.06f, 0.0f }), { 0.14f, 0.12f, 1.1f }, giro, TonoG(madera, 0.8f));
    }

    for (int k = 0; k < 5; k++)
    {
        CajaGiradaG(LocalG(base, giro, { 0.0f, 0.15f, -0.48f + (float)k * 0.24f }), { 1.2f, 0.05f, 0.16f }, giro, VariarG(madera, k, 3, 0.08f));
    }
}


// Torre de celosia del cablecam con cabezal de focos.
static void TorreCablecamG(Vector3 punta, bool decoracion)
{
    float alto = punta.y + 0.3f;
    float a = 0.55f;
    Color acero = COLOR_OCRE_G;
    Vector3 b = { punta.x, 0.0f, punta.z };

    CajaG({ b.x, 0.15f, b.z }, { 1.8f, 0.3f, 1.8f }, COLOR_HORMIGON_G);

    for (int k = 0; k < 4; k++)
    {
        float sx = (k % 2 == 0) ? -a : a;
        float sz = (k < 2) ? -a : a;
        VigaG({ b.x + sx, 0.3f, b.z + sz }, { b.x + sx * 0.6f, alto, b.z + sz * 0.6f }, 0.13f, acero);
    }

    int niveles = decoracion ? 8 : 3;

    for (int nv = 0; nv < niveles; nv++)
    {
        float y0 = 0.3f + (alto - 0.3f) * (float)nv / (float)niveles;
        float y1 = 0.3f + (alto - 0.3f) * (float)(nv + 1) / (float)niveles;
        float s0 = a * (1.0f - 0.4f * (y0 - 0.3f) / (alto - 0.3f));
        float s1 = a * (1.0f - 0.4f * (y1 - 0.3f) / (alto - 0.3f));
        Vector3 c0[4] = { { b.x - s0, y0, b.z - s0 }, { b.x + s0, y0, b.z - s0 }, { b.x + s0, y0, b.z + s0 }, { b.x - s0, y0, b.z + s0 } };
        Vector3 c1[4] = { { b.x - s1, y1, b.z - s1 }, { b.x + s1, y1, b.z - s1 }, { b.x + s1, y1, b.z + s1 }, { b.x - s1, y1, b.z + s1 } };

        for (int k = 0; k < 4; k++)
        {
            VigaG(c1[k], c1[(k + 1) % 4], 0.07f, acero);

            if (decoracion)
            {
                VigaG(c0[k], c1[(k + 1) % 4], 0.05f, TonoG(acero, 0.85f));
            }
        }
    }

    // Cabezal: polea del cablecam y barra de focos mirando al pozo.
    CajaG({ punta.x, alto + 0.2f, punta.z }, { 1.0f, 0.4f, 1.0f }, COLOR_ACERO_OSCURO_G);
    CilG({ punta.x - 0.3f, alto + 0.5f, punta.z }, { punta.x + 0.3f, alto + 0.5f, punta.z }, 0.3f, 0.3f, 10, COLOR_ACERO_G);

    float haciaPozo = punta.x < 0.0f ? 1.0f : -1.0f;
    float fz = punta.z < 0.0f ? 0.5f : -0.5f;

    for (int k = 0; k < 3; k++)
    {
        Vector3 f = { punta.x + haciaPozo * 0.55f, alto - 0.6f - (float)k * 0.0f, punta.z + fz * 0.4f + ((float)k - 1.0f) * 0.55f };
        CajaG(f, { 0.42f, 0.42f, 0.42f }, COLOR_ACERO_OSCURO_G);
        g_emisG = 1.0f;
        QuadG({ f.x + haciaPozo * 0.215f, f.y - 0.15f, f.z - 0.15f }, { f.x + haciaPozo * 0.215f, f.y - 0.15f, f.z + 0.15f },
            { f.x + haciaPozo * 0.215f, f.y + 0.15f, f.z + 0.15f }, { f.x + haciaPozo * 0.215f, f.y + 0.15f, f.z - 0.15f }, COLOR_SOL_G);
        g_emisG = 0.0f;
    }
}


// Cinta transportadora: bastidor, patas y rodillos (las tablillas se animan).
static void CintaG(float x, float z0, float z1, float alto)
{
    float a = 0.62f;
    Color bastidor = COLOR_OCRE_G;

    CajaG({ x, alto - 0.04f, 0.5f * (z0 + z1) }, { a * 2.0f - 0.16f, 0.08f, z1 - z0 }, COLOR_GOMA_G);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        CajaG({ x + (float)lado * a, alto, 0.5f * (z0 + z1) }, { 0.12f, 0.26f, z1 - z0 }, bastidor);
    }

    for (float z = z0 + 0.4f; z < z1; z += 1.6f)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            CajaG({ x + (float)lado * (a - 0.05f), (alto - 0.12f) * 0.5f, z }, { 0.1f, alto - 0.12f, 0.1f }, COLOR_ACERO_OSCURO_G);
        }

        CajaG({ x, 0.25f, z }, { a * 2.0f, 0.07f, 0.07f }, COLOR_ACERO_OSCURO_G);
    }

    CilG({ x - a, alto - 0.05f, z0 }, { x + a, alto - 0.05f, z0 }, 0.12f, 0.12f, 8, COLOR_ACERO_G);
    CilG({ x - a, alto - 0.05f, z1 }, { x + a, alto - 0.05f, z1 }, 0.12f, 0.12f, 8, COLOR_ACERO_G);
}


// Prensa enfardadora: bastidor fijo (el plato baja en el animado).
static void PrensaG(Vector3 c)
{
    Color pintura = COLOR_OCRE_G;

    CajaG({ c.x, 0.5f, c.z }, { 3.0f, 1.0f, 3.6f }, COLOR_ACERO_OSCURO_G);
    CajaG({ c.x - 1.35f, 1.25f, c.z }, { 0.3f, 1.5f, 3.4f }, pintura);
    CajaG({ c.x + 1.35f, 1.25f, c.z }, { 0.3f, 1.5f, 3.4f }, pintura);
    CajaG({ c.x, 1.25f, c.z - 1.6f }, { 2.4f, 1.5f, 0.25f }, TonoG(pintura, 0.9f));

    for (int k = 0; k < 4; k++)
    {
        float sx = (k % 2 == 0) ? -1.25f : 1.25f;
        float sz = (k < 2) ? -1.45f : 1.45f;
        CajaG({ c.x + sx, 2.6f, c.z + sz }, { 0.28f, 4.2f, 0.28f }, COLOR_ACERO_G);
    }

    CajaG({ c.x, 4.85f, c.z }, { 3.1f, 0.6f, 3.4f }, pintura);
    CilG({ c.x, 5.15f, c.z }, { c.x, 6.0f, c.z }, 0.42f, 0.42f, 12, COLOR_ACERO_OSCURO_G);
    CilG({ c.x, 6.0f, c.z }, { c.x, 6.12f, c.z }, 0.5f, 0.5f, 12, COLOR_ACERO_G);

    // Franjas de peligro en el frente de la camara.
    for (int k = 0; k < 6; k++)
    {
        float x0 = c.x - 1.5f + (float)k * 0.5f;
        Color f = (k % 2 == 0) ? Color{ 236, 176, 60, 255 } : Color{ 34, 30, 34, 255 };
        QuadG({ x0, 0.82f, c.z + 1.81f }, { x0 + 0.5f, 0.82f, c.z + 1.81f }, { x0 + 0.5f, 0.98f, c.z + 1.81f }, { x0, 0.98f, c.z + 1.81f }, f);
    }

    // Tablero con pilotos.
    CajaG({ c.x + 1.9f, 0.7f, c.z + 1.2f }, { 0.5f, 1.4f, 0.4f }, COLOR_ACERO_G);
    g_emisG = 1.0f;
    CajaG({ c.x + 1.9f, 1.15f, c.z + 1.41f }, { 0.12f, 0.12f, 0.03f }, Color{ 120, 230, 120, 255 });
    CajaG({ c.x + 1.9f, 0.95f, c.z + 1.41f }, { 0.12f, 0.12f, 0.03f }, Color{ 255, 120, 80, 255 });
    g_emisG = 0.0f;
}


static void FardoG(Vector3 c, int semilla)
{
    Color base = COLORES_AUTO_G[semilla % 6];
    CajaG(c, { 1.1f, 1.0f, 1.1f }, MezclaG(base, COLOR_OXIDO_G, 0.4f * Ruido01Grua(semilla, 1)));

    for (int k = 0; k < 3; k++)
    {
        float y = c.y - 0.3f + (float)k * 0.3f;
        CajaG({ c.x, y, c.z + 0.56f }, { 1.0f, 0.06f, 0.02f }, VariarG(COLORES_AUTO_G[(semilla + k + 2) % 6], k, semilla, 0.1f));
    }

    CajaG({ c.x + 0.56f, c.y, c.z }, { 0.02f, 1.02f, 0.08f }, COLOR_ACERO_OSCURO_G);
}


static void CasetaG(Vector3 c)
{
    Color chapa = Color{ 160, 150, 128, 255 };

    CajaG({ c.x, 1.35f, c.z }, { 4.6f, 2.5f, 2.6f }, chapa);
    CajaG({ c.x, 2.68f, c.z }, { 4.8f, 0.16f, 2.8f }, COLOR_OXIDO_OSCURO_G);

    for (int k = 0; k < 9; k++)
    {
        float x = c.x - 2.2f + (float)k * 0.55f;
        CajaG({ x, 1.35f, c.z + 1.31f }, { 0.06f, 2.4f, 0.03f }, TonoG(chapa, 0.85f));
    }

    CajaG({ c.x - 1.3f, 1.15f, c.z + 1.33f }, { 0.9f, 1.9f, 0.04f }, Color{ 92, 72, 60, 255 });
    g_emisG = 1.0f;
    CajaG({ c.x + 0.9f, 1.55f, c.z + 1.33f }, { 1.5f, 0.8f, 0.04f }, Color{ 255, 206, 130, 255 });
    g_emisG = 0.0f;
    CajaG({ c.x + 0.9f, 1.55f, c.z + 1.35f }, { 0.05f, 0.8f, 0.04f }, Color{ 70, 60, 56, 255 });
    CajaG({ c.x + 1.9f, 2.1f, c.z + 1.5f }, { 0.7f, 0.5f, 0.4f }, COLOR_ACERO_G);
    CajaG({ c.x - 1.3f, 0.12f, c.z + 1.75f }, { 1.1f, 0.24f, 0.8f }, COLOR_ACERO_OSCURO_G);
    VigaG({ c.x - 2.0f, 2.76f, c.z - 0.8f }, { c.x - 2.0f, 4.6f, c.z - 0.8f }, 0.05f, COLOR_ACERO_OSCURO_G);
    VigaG({ c.x - 2.4f, 4.2f, c.z - 0.8f }, { c.x - 1.6f, 4.2f, c.z - 0.8f }, 0.04f, COLOR_ACERO_OSCURO_G);
}


// Brazo de soldadura sobre un casco de auto viejo.
static void SoldaduraG()
{
    Vector3 p = PUNTO_SOLDADURA_GRUA;
    Vector3 casco = { p.x - 0.75f, 0.0f, p.z + 0.25f };
    Color carro = COLORES_AUTO_G[0];

    for (int k = 0; k < 4; k++)
    {
        float sx = (k % 2 == 0) ? -0.6f : 0.6f;
        float sz = (k < 2) ? -1.2f : 1.2f;
        CajaG({ casco.x + sx, 0.2f, casco.z + sz }, { 0.3f, 0.4f, 0.3f }, COLOR_ACERO_OSCURO_G);
    }

    CajaG({ casco.x, 0.62f, casco.z }, { 1.7f, 0.5f, 3.6f }, carro);
    CajaG({ casco.x, 1.07f, casco.z - 0.2f }, { 1.5f, 0.42f, 1.7f }, TonoG(carro, 0.92f));
    CajaG({ casco.x, 1.06f, casco.z - 0.2f }, { 1.52f, 0.28f, 1.5f }, Color{ 44, 52, 62, 255 });
    CajaG({ casco.x + 0.4f, 0.88f, casco.z + 1.3f }, { 0.6f, 0.03f, 0.6f }, COLOR_OXIDO_G);

    // Tubos de gas en su carrito y brazo articulado.
    Vector3 tubos = { p.x + 1.2f, 0.0f, p.z + 0.9f };
    CajaG({ tubos.x, 0.1f, tubos.z }, { 0.9f, 0.2f, 0.6f }, COLOR_ACERO_OSCURO_G);
    CilG({ tubos.x - 0.2f, 0.2f, tubos.z }, { tubos.x - 0.2f, 1.45f, tubos.z }, 0.16f, 0.16f, 8, Color{ 70, 120, 116, 255 });
    CilG({ tubos.x + 0.2f, 0.2f, tubos.z }, { tubos.x + 0.2f, 1.35f, tubos.z }, 0.16f, 0.16f, 8, Color{ 128, 62, 58, 255 });

    Vector3 b0 = { p.x + 1.0f, 0.0f, p.z - 0.2f };
    Vector3 b1 = { p.x + 1.0f, 1.3f, p.z - 0.2f };
    Vector3 b2 = { p.x + 0.45f, 1.6f, p.z - 0.05f };
    Vector3 b3 = { p.x + 0.05f, p.y + 0.08f, p.z };
    CilG(b0, { b0.x, 0.25f, b0.z }, 0.3f, 0.3f, 10, COLOR_ACERO_OSCURO_G);
    VigaG({ b0.x, 0.25f, b0.z }, b1, 0.2f, COLOR_OCRE_G);
    VigaG(b1, b2, 0.16f, COLOR_OCRE_G);
    VigaG(b2, b3, 0.08f, COLOR_ACERO_G);
    BolaG(b1, { 0.15f, 0.15f, 0.15f }, COLOR_ACERO_OSCURO_G, 3, 6);
    BolaG(b2, { 0.12f, 0.12f, 0.12f }, COLOR_ACERO_OSCURO_G, 3, 6);
}


static void TamborFuegoG(Vector3 base)
{
    CilG(base, { base.x, base.y + 0.95f, base.z }, 0.36f, 0.36f, 10, COLOR_OXIDO_OSCURO_G);
    DiscoSueloG({ base.x, base.y + 0.955f, base.z }, 0.33f, 0.33f, 10, Color{ 40, 22, 20, 255 }, Color{ 40, 22, 20, 255 });

    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.5707963f + 0.4f;
        CajaG({ base.x + std::cos(a) * 0.37f, base.y + 0.3f, base.z + std::sin(a) * 0.37f }, { 0.1f, 0.06f, 0.1f }, COLOR_GOMA_G);
    }
}


static void BarandaG(float x0, float x1, float z, float alto)
{
    for (float x = x0; x <= x1 + 0.01f; x += 1.5f)
    {
        CajaG({ x, alto * 0.5f, z }, { 0.08f, alto, 0.08f }, COLOR_OCRE_G);
    }

    CajaG({ 0.5f * (x0 + x1), alto, z }, { x1 - x0, 0.07f, 0.07f }, COLOR_OCRE_G);
    CajaG({ 0.5f * (x0 + x1), alto * 0.55f, z }, { x1 - x0, 0.05f, 0.05f }, TonoG(COLOR_OCRE_G, 0.85f));
}


static void ContenedorG(Vector3 c, float giro, Color color)
{
    CajaGiradaG({ c.x, c.y + 1.3f, c.z }, { 6.0f, 2.6f, 2.44f }, giro, color);

    for (int k = 0; k < 14; k++)
    {
        Vector3 p = LocalG({ c.x, c.y + 1.3f, c.z }, giro, { -2.8f + (float)k * 0.43f, 0.0f, 1.23f });
        CajaGiradaG(p, { 0.12f, 2.4f, 0.05f }, giro, TonoG(color, 0.82f));
    }

    CajaGiradaG(LocalG({ c.x, c.y + 2.62f, c.z }, giro, { 0.0f, 0.0f, 0.0f }), { 6.02f, 0.06f, 2.46f }, giro, TonoG(color, 1.08f));
}


static void EscenaCercanaG(int n, bool decoracion, int paso)
{
    // Paredes del pozo: hormigon con angulo de acero, del borde real.
    Color hormigon = Color{ 132, 122, 112, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (X_POZO_G + 0.25f);

        for (int k = 0; k < 8; k++)
        {
            float z0 = -5.4f + (float)k * 1.45f;
            float z1 = z0 + 1.45f;
            CajaG({ x, 0.21f, 0.5f * (z0 + z1) }, { 0.5f, 0.42f, z1 - z0 - 0.04f }, VariarG(hormigon, k + lado * 9, 71, 0.06f));
        }

        CajaG({ (float)lado * (X_POZO_G + 0.04f), 0.44f, 0.4f }, { 0.1f, 0.06f, 11.6f }, COLOR_ACERO_G);
    }

    for (int k = 0; k < 13; k++)
    {
        float x0 = -9.6f + (float)k * 1.477f;
        CajaG({ x0 + 0.738f, 0.18f, Z_POZO_FRENTE_G + 0.22f }, { 1.44f, 0.36f, 0.44f }, VariarG(hormigon, k, 72, 0.06f));
        Color franja = (k % 2 == 0) ? Color{ 236, 176, 60, 255 } : Color{ 34, 30, 34, 255 };
        CajaG({ x0 + 0.738f, 0.375f, Z_POZO_FRENTE_G + 0.05f }, { 1.44f, 0.04f, 0.1f }, franja);
    }

    CajaG({ 0.0f, 0.13f, -5.22f }, { 2.0f * X_POZO_G + 1.0f, 0.26f, 0.34f }, COLOR_ACERO_OSCURO_G);
    CajaG({ 0.0f, 0.275f, -5.22f }, { 2.0f * X_POZO_G + 1.0f, 0.03f, 0.36f }, COLOR_ACERO_G);

    // Torres del cablecam (las delanteras quedan casi fuera de cuadro).
    for (int t = 0; t < 4; t++)
    {
        TorreCablecamG(TORRES_GRUA[t], decoracion || t < 2);
    }

    // Baranda detras de los operadores.
    BarandaG(-12.0f, 12.0f, -9.5f, 1.0f);

    // Cintas laterales.
    CintaG(-11.0f, -3.4f, 6.6f, 0.85f);
    CintaG(11.0f, 0.4f, 6.6f, 0.85f);

    // Prensa a la derecha.
    PrensaG(CENTRO_PRENSA_GRUA);

    if (!decoracion)
    {
        return;
    }

    // Pilas de autos aplastados a la izquierda.
    PilaAutosG(-14.6f, -1.6f, 5, 1, 4.0f);
    PilaAutosG(-14.2f, 3.4f, 3, 2, -8.0f);

    if (paso == 1)
    {
        PilaAutosG(-17.6f, 0.6f, 6, 3, 12.0f);
        PilaAutosG(-17.2f, -5.2f, 4, 4, -3.0f);
    }

    SoldaduraG();
    TamborFuegoG({ -11.2f, 0.0f, -6.6f });

    // Fardos de la prensa.
    FardoG({ 16.4f, 0.5f, -0.2f }, 1);
    FardoG({ 17.6f, 0.5f, -0.3f }, 2);
    FardoG({ 16.4f, 0.5f, 1.0f }, 3);
    FardoG({ 17.0f, 1.52f, 0.4f }, 4);

    if (paso == 1)
    {
        FardoG({ 17.6f, 0.5f, 0.95f }, 5);
        FardoG({ 18.8f, 0.5f, -0.3f }, 6);
    }

    CasetaG({ 18.6f, 0.0f, 5.6f });

    // Barriles, neumaticos, conos y palets.
    BarrilG({ -10.6f, 0.0f, 7.6f }, COLOR_OCRE_G, false, 0.0f, 1);
    BarrilG({ -11.4f, 0.0f, 7.9f }, Color{ 70, 112, 110, 255 }, false, 0.0f, 2);
    BarrilG({ -11.0f, 0.0f, 8.6f }, COLOR_OXIDO_G, true, 70.0f, 3);
    BarrilG({ 11.6f, 0.0f, 7.4f }, Color{ 70, 112, 110, 255 }, false, 0.0f, 4);
    BarrilG({ 12.4f, 0.0f, 7.8f }, COLOR_OXIDO_G, false, 0.0f, 5);
    BarrilG({ 10.6f, 0.0f, -6.8f }, COLOR_OCRE_G, false, 0.0f, 6);
    BarrilG({ 10.9f, 0.0f, -7.6f }, COLOR_OXIDO_G, false, 0.0f, 7);
    BarrilG({ 15.4f, 0.0f, -6.0f }, Color{ 70, 112, 110, 255 }, true, 20.0f, 8);

    PilaNeumaticosG(-16.4f, 6.6f, 4, 1);
    PilaNeumaticosG(-15.4f, 7.2f, 3, 2);
    PilaNeumaticosG(14.0f, -6.6f, 5, 3);

    if (paso == 1)
    {
        PilaNeumaticosG(-17.4f, 7.4f, 2, 4);
        NeumaticoG({ -14.8f, 0.0f, 7.9f }, 0.44f);
        PilaNeumaticosG(20.0f, 2.2f, 4, 5);
    }

    ConoG(-9.9f, 6.9f);
    ConoG(9.9f, 6.9f);
    ConoG(10.2f, -5.2f);
    ConoG(-10.3f, -5.0f);

    PaletG({ 15.6f, 0.0f, 3.6f }, 10.0f);
    PaletG({ 15.7f, 0.18f, 3.6f }, 4.0f);
    CajaG({ 15.6f, 0.62f, 3.6f }, { 0.9f, 0.6f, 0.8f }, Color{ 150, 110, 72, 255 });

    // Contenedores detras de la baranda.
    ContenedorG({ -19.0f, 0.0f, -10.4f }, 4.0f, Color{ 124, 60, 54, 255 });
    ContenedorG({ 18.4f, 0.0f, -10.2f }, -6.0f, Color{ 62, 104, 108, 255 });

    if (paso == 1)
    {
        ContenedorG({ 21.0f, 0.0f, -7.0f }, 84.0f, Color{ 150, 118, 74, 255 });
    }

    (void)n;
}


static void EscenaLejanaG(bool decoracion, int paso);


static void EscenaEstaticaG(int n, bool decoracion, int paso)
{
    LoteG();
    EscenaCercanaG(n, decoracion, paso);
    EscenaLejanaG(decoracion, paso);
    FinLoteG();
}


//--------------------------------------------------
// Media distancia y horizonte
//--------------------------------------------------

static Color ColorChatarraG(int h, int semilla)
{
    float r = Ruido01Grua(h, semilla);

    if (r < 0.36f) return VariarG(Color{ 132, 72, 50, 255 }, h, semilla + 1, 0.08f);
    if (r < 0.64f) return VariarG(COLOR_OXIDO_OSCURO_G, h, semilla + 1, 0.08f);
    if (r < 0.84f) return VariarG(Color{ 92, 86, 94, 255 }, h, semilla + 1, 0.08f);
    return TonoG(MezclaG(COLORES_AUTO_G[(h % 2) * 3], COLOR_OXIDO_G, 0.35f), 0.85f);
}


// Monton de chatarra: loma irregular con piezas que asoman.
static void MontonChatarraG(Vector3 c, float rx, float ry, float rz, int semilla, int detalle)
{
    const int lados = 12, anillos = 4;

    auto P = [&](int k, int a)
    {
        if (a >= anillos)
        {
            return Vector3{ c.x + rx * 0.1f * (Ruido01Grua(semilla, 99) - 0.5f), c.y + ry, c.z };
        }

        float lat = 0.5f * PI * (float)a / (float)anillos;
        float lon = 2.0f * PI * (float)(k % lados) / (float)lados;
        float r = 0.8f + 0.36f * Ruido01Grua((k % lados) * 7 + a * 131, semilla);
        float h = a == 0 ? 0.0f : (0.85f + 0.3f * Ruido01Grua((k % lados) * 5 + a * 17, semilla + 3));
        return Vector3{ c.x + std::cos(lon) * std::cos(lat) * rx * r, c.y + std::sin(lat) * ry * h, c.z + std::sin(lon) * std::cos(lat) * rz * r };
    };

    for (int a = 0; a < anillos; a++)
    {
        for (int k = 0; k < lados; k++)
        {
            Vector3 p0 = P(k, a), p1 = P(k + 1, a), p2 = P(k + 1, a + 1), p3 = P(k, a + 1);
            Vector3 fc = Vector3Scale(Vector3Add(Vector3Add(p0, p1), Vector3Add(p2, p3)), 0.25f);
            Vector3 n = Vector3Normalize({ (fc.x - c.x) / (rx * rx), (fc.y - c.y) / (ry * ry), (fc.z - c.z) / (rz * rz) });
            Color color = CaraG(ColorChatarraG(k * 11 + a, semilla), n);

            if (a == anillos - 1)
            {
                TriG(p0, p1, p2, color);
            }
            else
            {
                QuadG(p0, p1, p2, p3, color);
            }
        }
    }

    // Piezas que asoman: chapas, caños, ruedas.
    for (int i = 0; i < detalle; i++)
    {
        int h = semilla * 37 + i;
        float lon = 2.0f * PI * Ruido01Grua(h, 1);
        float lat = 0.5f * PI * (0.15f + 0.7f * Ruido01Grua(h, 2));
        Vector3 p = {
            c.x + std::cos(lon) * std::cos(lat) * rx * 0.92f,
            c.y + std::sin(lat) * ry * 0.92f,
            c.z + std::sin(lon) * std::cos(lat) * rz * 0.92f
        };
        float tipo = Ruido01Grua(h, 3);
        Color color = ColorChatarraG(h, 7);

        if (tipo < 0.55f)
        {
            Vector3 t = { 0.4f + Ruido01Grua(h, 4) * 1.0f, 0.12f + Ruido01Grua(h, 5) * 0.3f, 0.5f + Ruido01Grua(h, 6) * 1.2f };
            CajaGiradaG(p, t, Ruido01Grua(h, 7) * 180.0f, color, (Ruido01Grua(h, 8) - 0.5f) * 60.0f);
        }
        else if (tipo < 0.8f)
        {
            float a = Ruido01Grua(h, 9) * PI;
            float l = 0.6f + Ruido01Grua(h, 10) * 1.0f;
            Vector3 d = { std::cos(a) * l, (Ruido01Grua(h, 11) - 0.3f) * 0.6f, std::sin(a) * l };
            CilG(Vector3Subtract(p, d), Vector3Add(p, d), 0.09f, 0.09f, 6, Color{ 128, 128, 136, 255 }, false);
        }
        else
        {
            float a = Ruido01Grua(h, 12) * PI;
            Vector3 d = { std::cos(a) * 0.14f, 0.0f, std::sin(a) * 0.14f };
            CilG(Vector3Subtract(p, d), Vector3Add(p, d), 0.42f, 0.42f, 10, COLOR_GOMA_G);
        }
    }
}


// Grua de orugas con pluma de celosia e iman: recortada contra el cielo.
static void GruaOrugasG(Vector3 base, bool decoracion)
{
    Color pintura = COLOR_OCRE_G;
    Color oscura = Color{ 60, 50, 58, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        CajaG({ base.x + (float)lado * 1.3f, 0.45f, base.z }, { 0.9f, 0.9f, 4.4f }, COLOR_GOMA_G);
        CajaG({ base.x + (float)lado * 1.3f, 0.92f, base.z }, { 0.95f, 0.06f, 4.2f }, COLOR_ACERO_OSCURO_G);
    }

    CajaG({ base.x, 1.25f, base.z }, { 3.2f, 0.7f, 3.4f }, oscura);
    CajaG({ base.x, 2.4f, base.z + 0.3f }, { 3.0f, 1.6f, 3.6f }, pintura);
    CajaG({ base.x + 0.9f, 3.5f, base.z + 1.4f }, { 1.3f, 1.2f, 1.2f }, pintura);
    g_emisG = 1.0f;
    CajaG({ base.x + 0.9f, 3.6f, base.z + 2.01f }, { 1.1f, 0.6f, 0.03f }, Color{ 255, 196, 128, 255 });
    g_emisG = 0.0f;
    CajaG({ base.x - 0.4f, 2.6f, base.z - 1.9f }, { 2.6f, 1.4f, 1.0f }, oscura);

    // Pluma: dos cordones con celosia hacia arriba a la izquierda.
    Vector3 pie = { base.x - 0.6f, 3.2f, base.z + 1.0f };
    Vector3 punta = { base.x - 9.0f, 15.5f, base.z - 6.0f };
    Vector3 eje = Vector3Subtract(punta, pie);
    Vector3 lat = Vector3Normalize(Vector3CrossProduct(eje, { 0.0f, 1.0f, 0.0f }));
    Vector3 arriba = Vector3Normalize(Vector3CrossProduct(lat, eje));
    int tramos = decoracion ? 14 : 6;

    for (int c = 0; c < 4; c++)
    {
        float sl = (c % 2 == 0) ? -0.45f : 0.45f;
        float su = (c < 2) ? -0.35f : 0.35f;
        float s1 = 0.4f;
        Vector3 o0 = Vector3Add(Vector3Scale(lat, sl), Vector3Scale(arriba, su));
        Vector3 o1 = Vector3Add(Vector3Scale(lat, sl * s1), Vector3Scale(arriba, su * s1));
        VigaG(Vector3Add(pie, o0), Vector3Add(punta, o1), 0.12f, pintura);
    }

    for (int t = 1; t < tramos; t++)
    {
        float f0 = (float)(t - 1) / (float)tramos, f1 = (float)t / (float)tramos;
        float s0 = 1.0f - 0.6f * f0, s1 = 1.0f - 0.6f * f1;
        Vector3 c0 = Vector3Add(pie, Vector3Scale(eje, f0));
        Vector3 c1 = Vector3Add(pie, Vector3Scale(eje, f1));

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 a0 = Vector3Add(c0, Vector3Scale(lat, 0.45f * s0 * (float)lado));
            Vector3 a1 = Vector3Add(c1, Vector3Scale(lat, 0.45f * s1 * (float)lado));
            VigaG(Vector3Add(a0, Vector3Scale(arriba, -0.35f * s0)), Vector3Add(a1, Vector3Scale(arriba, 0.35f * s1)), 0.06f, TonoG(pintura, 0.85f));
        }
    }

    // Tensores al mastil y cable del iman.
    Vector3 mastil = { base.x + 0.2f, 6.0f, base.z - 1.6f };
    VigaG({ base.x - 0.2f, 3.2f, base.z - 1.4f }, mastil, 0.16f, oscura);
    VigaG(mastil, punta, 0.04f, Color{ 40, 36, 42, 255 });
    Vector3 iman = { punta.x, 7.4f, punta.z };
    VigaG({ punta.x, punta.y - 0.2f, punta.z }, { iman.x, iman.y + 0.4f, iman.z }, 0.05f, Color{ 40, 36, 42, 255 });
    CilG({ iman.x, iman.y, iman.z }, { iman.x, iman.y + 0.4f, iman.z }, 1.0f, 1.0f, 12, oscura);
    CajaGiradaG({ iman.x, iman.y - 0.3f, iman.z }, { 1.6f, 0.5f, 1.0f }, 25.0f, COLORES_AUTO_G[3], 12.0f);
}


// Galpon de la trituradora con chimenea (marca el centro del fondo).
static void GalponG(Vector3 c)
{
    Color chapa = Color{ 118, 98, 92, 255 };
    CajaG({ c.x, 1.8f, c.z }, { 7.0f, 3.6f, 4.0f }, chapa);

    for (int k = 0; k < 3; k++)
    {
        float x0 = c.x - 3.5f + (float)k * 2.333f;
        float x1 = x0 + 2.333f;
        Color techo = TonoG(COLOR_OXIDO_OSCURO_G, 1.05f);
        QuadG({ x0, 3.6f, c.z + 2.0f }, { x1, 3.6f, c.z + 2.0f }, { x1, 4.6f, c.z - 0.2f }, { x0, 4.6f, c.z - 0.2f }, CaraG(techo, { 0.0f, 0.9f, 0.4f }));
        g_emisG = 1.0f;
        QuadG({ x0, 4.6f, c.z - 0.2f }, { x1, 4.6f, c.z - 0.2f }, { x1, 3.6f, c.z - 0.25f }, { x0, 3.6f, c.z - 0.25f }, Color{ 255, 200, 130, 255 });
        g_emisG = 0.0f;
    }

    for (int k = 0; k < 10; k++)
    {
        CajaG({ c.x - 3.3f + (float)k * 0.73f, 1.8f, c.z + 2.01f }, { 0.08f, 3.5f, 0.03f }, TonoG(chapa, 0.85f));
    }

    CajaG({ c.x + 1.4f, 1.3f, c.z + 2.02f }, { 2.0f, 2.6f, 0.05f }, Color{ 70, 58, 56, 255 });
    CilG({ c.x - 2.4f, 3.6f, c.z - 0.8f }, { c.x - 2.4f, 9.5f, c.z - 0.8f }, 0.42f, 0.32f, 10, Color{ 120, 84, 74, 255 });
    CilG({ c.x - 2.4f, 8.6f, c.z - 0.8f }, { c.x - 2.4f, 8.9f, c.z - 0.8f }, 0.4f, 0.4f, 10, Color{ 210, 196, 180, 255 }, false);
}


// Siluetas lejanas (contraluz): color fijo por capa, sin bruma.
static void QuadSiluetaG(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    QuadColoresG(a, b, c, d, color, color, color, color);
}


static void CajaSiluetaG(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;
    QuadSiluetaG({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, color);
    QuadSiluetaG({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, TonoG(color, 1.08f));
}


static void VigaSiluetaG(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 vista = Vector3Subtract(Vector3Scale(Vector3Add(a, b), 0.5f), OJO_GRUA);
    Vector3 lado = Vector3CrossProduct(Vector3Subtract(b, a), vista);
    float l = Vector3Length(lado);

    if (l < 0.0001f)
    {
        return;
    }

    lado = Vector3Scale(lado, grosor * 0.5f / l);
    QuadSiluetaG(Vector3Subtract(a, lado), Vector3Add(a, lado), Vector3Add(b, lado), Vector3Subtract(b, lado), color);
}


// Altura del mundo que se ve con una elevacion dada desde la camara final.
static float AlturaElevacionG(float z, float grados)
{
    return OJO_GRUA.y - (OJO_GRUA.z - z) * std::tan(-grados * DEG2RAD);
}


// Pluma de grua torre (fija o la que gira despacio en el animado).
static void PlumaGruaTorreG(Vector3 cima, float giro, float largo, float contra, Color color)
{
    float a = giro * DEG2RAD;
    Vector3 d = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 punta = Vector3Add(cima, Vector3Scale(d, largo));
    Vector3 cola = Vector3Subtract(cima, Vector3Scale(d, contra));
    Vector3 pico = { cima.x, cima.y + largo * 0.16f, cima.z };

    VigaSiluetaG(cola, punta, largo * 0.03f, color);
    VigaSiluetaG({ cola.x, cola.y - largo * 0.025f, cola.z }, { punta.x, punta.y - largo * 0.02f, punta.z }, largo * 0.012f, color);

    for (int k = 0; k < 10; k++)
    {
        float f0 = (float)k / 10.0f, f1 = (float)(k + 1) / 10.0f;
        Vector3 p0 = Vector3Add(cima, Vector3Scale(d, largo * f0));
        Vector3 p1 = Vector3Add(cima, Vector3Scale(d, largo * f1));
        VigaSiluetaG(p0, { p1.x, p1.y - largo * 0.025f, p1.z }, largo * 0.007f, color);
    }

    VigaSiluetaG({ cima.x, cima.y - largo * 0.04f, cima.z }, pico, largo * 0.025f, color);
    VigaSiluetaG(pico, punta, largo * 0.005f, color);
    VigaSiluetaG(pico, cola, largo * 0.005f, color);
    CajaSiluetaG({ cola.x + d.x * contra * 0.15f, cola.y - largo * 0.04f, cola.z }, { largo * 0.08f, largo * 0.07f, largo * 0.06f }, color);

    // Carro y cable del gancho.
    Vector3 carro = Vector3Add(cima, Vector3Scale(d, largo * 0.62f));
    VigaSiluetaG(carro, { carro.x, carro.y - largo * 0.32f, carro.z }, largo * 0.003f, color);
    CajaSiluetaG({ carro.x, carro.y - largo * 0.33f, carro.z }, { largo * 0.03f, largo * 0.03f, largo * 0.03f }, color);
}


static void GruaTorreLejanaG(Vector3 base, float yCima, float ancho, Color color)
{
    VigaSiluetaG({ base.x - ancho * 0.5f, base.y, base.z }, { base.x - ancho * 0.5f, yCima, base.z }, ancho * 0.18f, color);
    VigaSiluetaG({ base.x + ancho * 0.5f, base.y, base.z }, { base.x + ancho * 0.5f, yCima, base.z }, ancho * 0.18f, color);

    int tramos = (int)((yCima - base.y) / ancho);

    for (int k = 0; k < tramos; k++)
    {
        float y0 = base.y + (float)k * ancho, y1 = y0 + ancho;
        float s = (k % 2 == 0) ? 1.0f : -1.0f;
        VigaSiluetaG({ base.x - ancho * 0.5f * s, y0, base.z }, { base.x + ancho * 0.5f * s, y1, base.z }, ancho * 0.08f, color);
    }

    CajaSiluetaG({ base.x, yCima + ancho * 0.5f, base.z }, { ancho * 1.6f, ancho, ancho * 1.4f }, color);
}


// Ciudad industrial y cerros, en tres profundidades de bruma.
static void HorizonteG(bool decoracion)
{
    // Cerros lejanos (dos capas).
    const Color cerros[2] = { { 206, 122, 112, 255 }, { 170, 96, 104, 255 } };
    const float zs[2] = { -170.0f, -115.0f };
    const float elev[2][2] = { { -20.9f, -20.0f }, { -21.5f, -20.6f } };

    for (int capa = 0; capa < 2; capa++)
    {
        float z = zs[capa];
        float base = AlturaElevacionG(z, -24.0f);
        float prev = 0.0f;
        int pasos = 28;

        for (int k = 0; k <= pasos; k++)
        {
            float u = (float)k / (float)pasos;
            float onda = 0.5f + 0.3f * std::sin(u * 9.0f + (float)capa * 2.0f) + 0.2f * std::sin(u * 23.0f + (float)capa);
            float y = AlturaElevacionG(z, elev[capa][0] + (elev[capa][1] - elev[capa][0]) * onda);

            if (k > 0)
            {
                float x0 = -260.0f + (u - 1.0f / (float)pasos) * 520.0f, x1 = -260.0f + u * 520.0f;
                QuadSiluetaG({ x0, base, z }, { x1, base, z }, { x1, y, z }, { x0, prev, z }, cerros[capa]);
            }

            prev = y;
        }
    }

    // Ciudad industrial: naves, tanques y chimeneas.
    Color ciudad = Color{ 132, 74, 90, 255 };
    float z = -78.0f;
    float base = AlturaElevacionG(z, -24.0f);
    int k = 0;

    for (float x = -80.0f; x < 80.0f; k++)
    {
        float w = 3.0f + Ruido01Grua(k, 201) * 6.0f;
        float e = -22.5f + Ruido01Grua(k, 202) * 1.1f;
        float y = AlturaElevacionG(z, e);
        CajaSiluetaG({ x + w * 0.5f, 0.5f * (base + y), z }, { w, y - base, 2.0f }, VariarG(ciudad, k, 203, 0.05f));

        if (decoracion && Ruido01Grua(k, 204) < 0.3f)
        {
            float yc = AlturaElevacionG(z, e + 1.4f + Ruido01Grua(k, 205));
            VigaSiluetaG({ x + w * 0.3f, y, z }, { x + w * 0.3f, yc, z }, 0.9f, ciudad);
        }

        if (decoracion && Ruido01Grua(k, 206) < 0.2f)
        {
            // Tanque de gas al costado de la nave.
            float yt = AlturaElevacionG(z, e + 0.5f);
            CajaSiluetaG({ x + w + 0.2f, 0.5f * (base + yt), z + 1.0f }, { 3.4f, yt - base, 2.0f }, TonoG(ciudad, 0.94f));
        }

        x += w + 0.4f;
    }

    // Gruas torre lejanas recortadas contra el sol.
    float yA = AlturaElevacionG(-42.0f, -18.9f);
    GruaTorreLejanaG({ -3.0f, AlturaElevacionG(-42.0f, -24.0f), -42.0f }, yA, 0.9f, COLOR_SILUETA_G);
    float yB = AlturaElevacionG(-52.0f, -19.3f);
    GruaTorreLejanaG({ 14.0f, AlturaElevacionG(-52.0f, -24.0f), -52.0f }, yB, 1.0f, TonoG(COLOR_SILUETA_G, 1.15f));
    PlumaGruaTorreG({ 14.0f, yB + 1.0f, -52.0f }, 196.0f, 15.0f, 5.0f, TonoG(COLOR_SILUETA_G, 1.15f));

    if (decoracion)
    {
        // Portico de un astillero a la derecha.
        float zp = -60.0f;
        float y0 = AlturaElevacionG(zp, -24.0f), y1 = AlturaElevacionG(zp, -19.6f);
        Color c = TonoG(COLOR_SILUETA_G, 1.3f);
        VigaSiluetaG({ 30.0f, y0, zp }, { 31.5f, y1, zp }, 1.0f, c);
        VigaSiluetaG({ 44.0f, y0, zp }, { 42.5f, y1, zp }, 1.0f, c);
        VigaSiluetaG({ 27.0f, y1, zp }, { 47.0f, y1, zp }, 1.4f, c);
        VigaSiluetaG({ 36.0f, y1, zp }, { 36.0f, y1 - 4.0f, zp }, 0.2f, c);
    }
}


static void EscenaLejanaG(bool decoracion, int paso)
{
    HorizonteG(decoracion);

    // Loma baja continua que tapa el fin del patio: casi a ras en el centro,
    // asi el sol y las gruas lejanas se ven debajo del tendedero.
    for (int k = 0; k < 28; k++)
    {
        float x = -34.0f + (float)k * 2.5f;
        float centro = Lim01G((std::fabs(x) - 6.0f) / 8.0f);
        float alto = 0.38f + 0.22f * Ruido01Grua(k, 301) + 0.9f * centro;
        MontonChatarraG({ x, 0.0f, -15.7f }, 2.0f, alto, 1.3f, 400 + k, 0);
    }

    // Montanas de chatarra a los costados, mas altas hacia afuera.
    const float montones[10][5] = {
        { -10.8f, -13.4f, 3.0f, 1.3f, 2.2f }, { -15.6f, -13.8f, 3.8f, 2.2f, 2.6f }, { -21.4f, -13.2f, 4.4f, 2.8f, 3.0f },
        { -27.4f, -14.0f, 4.8f, 3.2f, 3.3f }, { -33.5f, -13.0f, 4.2f, 3.0f, 3.0f },
        { 10.8f, -13.5f, 3.0f, 1.2f, 2.2f }, { 15.8f, -13.8f, 3.8f, 2.0f, 2.6f }, { 21.6f, -12.9f, 4.4f, 2.7f, 3.0f },
        { 27.6f, -14.0f, 4.8f, 3.1f, 3.3f }, { 33.6f, -13.0f, 4.2f, 3.0f, 3.0f }
    };

    for (int i = 0; i < 10; i++)
    {
        const float* d = montones[i];
        int detalle = decoracion ? (paso == 1 ? 9 : 5) : 0;
        MontonChatarraG({ d[0], 0.0f, d[1] }, d[2], d[3], d[4], 500 + i * 7, detalle);
    }

    if (!decoracion)
    {
        return;
    }

    GruaOrugasG({ -13.6f, 0.0f, -11.6f }, paso == 1);
    GalponG({ 14.0f, 0.0f, -17.2f });

    // Autos sueltos al pie de las montanas.
    AutoAplastadoG({ -6.6f, 0.0f, -11.0f }, 78.0f, 0.0f, COLORES_AUTO_G[2], 801);
    AutoAplastadoG({ 7.2f, 0.0f, -11.2f }, 96.0f, 0.0f, COLORES_AUTO_G[4], 802);
    AutoAplastadoG({ 7.0f, 0.5f, -11.3f }, 84.0f, 3.0f, COLORES_AUTO_G[1], 803);

    if (paso == 1)
    {
        AutoAplastadoG({ -1.4f, 0.0f, -11.6f }, 100.0f, 0.0f, COLORES_AUTO_G[5], 804);
        PilaNeumaticosG(2.6f, -10.8f, 3, 9);
    }
}


//==================================================
// VISUAL: CIELO (2D, detras de todo)
//==================================================

// Pantalla de un punto del cielo a cierta elevacion (desde la camara final).
static Vector2 PuntoCieloG(const Camera3D& vista, float x, float elevacion)
{
    const float z = -60.0f;
    return GetWorldToScreen({ x, AlturaElevacionG(z, elevacion), z }, vista);
}


static void DibujarCieloGrua(const Camera3D& vista, float t, bool decoracion)
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float s = (float)h / 720.0f;

    // Degrade por elevacion: violeta arriba, naranja y dorado sobre la chatarra.
    const float elev[5] = { -4.0f, -13.0f, -17.5f, -20.6f, -23.0f };
    const Color col[5] = { COLOR_CIELO_ALTO_G, Color{ 132, 66, 104, 255 }, COLOR_CIELO_MEDIO_G, COLOR_CIELO_BAJO_G, COLOR_HORIZONTE_G };
    float ys[5];

    for (int k = 0; k < 5; k++)
    {
        ys[k] = PuntoCieloG(vista, 0.0f, elev[k]).y;
    }

    if (ys[0] > 0.0f)
    {
        DrawRectangle(0, 0, w, (int)ys[0] + 1, col[0]);
    }

    for (int k = 0; k < 4; k++)
    {
        int y0 = (int)ys[k], y1 = (int)ys[k + 1];

        if (y1 > y0 && y1 > 0 && y0 < h)
        {
            DrawRectangleGradientV(0, y0, w, y1 - y0 + 1, col[k], col[k + 1]);
        }
    }

    Vector2 sol = PuntoCieloG(vista, -4.0f, -19.7f);
    float radioSol = 23.0f * s;

    // Resplandor del sol y franja dorada del horizonte.
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(sol, radioSol * 9.0f, Fade(Color{ 255, 150, 90, 255 }, 0.22f), Fade(Color{ 255, 150, 90, 255 }, 0.0f));
    DrawCircleGradient(sol, radioSol * 3.5f, Fade(COLOR_SOL_G, 0.35f), Fade(COLOR_SOL_G, 0.0f));

    if (decoracion)
    {
        // Rayos que giran muy despacio.
        for (int k = 0; k < 9; k++)
        {
            float a = (float)k / 9.0f * 2.0f * PI + t * 0.03f;
            float largo = radioSol * (7.0f + 3.0f * Ruido01Grua(k, 901));
            float abre = 0.05f;
            Vector2 p1 = { sol.x + std::cos(a - abre) * largo, sol.y + std::sin(a - abre) * largo };
            Vector2 p2 = { sol.x + std::cos(a + abre) * largo, sol.y + std::sin(a + abre) * largo };
            DrawTriangle(sol, p2, p1, Fade(COLOR_SOL_G, 0.07f));
            DrawTriangle(sol, p1, p2, Fade(COLOR_SOL_G, 0.07f));
        }
    }

    EndBlendMode();
    DrawCircleV(sol, radioSol, Color{ 255, 226, 160, 255 });
    DrawCircleV(sol, radioSol * 0.8f, COLOR_SOL_G);

    // Nubes alargadas: panza iluminada por el sol bajo.
    int nubes = decoracion ? 7 : 3;

    for (int i = 0; i < nubes; i++)
    {
        float e = -6.0f - 12.5f * Ruido01Grua(i, 911);
        float y = PuntoCieloG(vista, 0.0f, e).y;
        float largo = (110.0f + 150.0f * Ruido01Grua(i, 912)) * s;
        float alto = (7.0f + 6.0f * Ruido01Grua(i, 913)) * s;
        float recorrido = (float)w + largo * 2.0f;
        float x = std::fmod(Ruido01Grua(i, 914) * recorrido + t * (4.0f + 5.0f * Ruido01Grua(i, 915)) * s, recorrido) - largo;
        float cercania = Lim01G((e + 6.0f) / -12.5f);
        Color cuerpo = MezclaG(Color{ 120, 62, 100, 255 }, Color{ 196, 92, 96, 255 }, cercania);
        Color panza = MezclaG(Color{ 236, 128, 110, 255 }, Color{ 255, 200, 140, 255 }, cercania);
        DrawEllipse((int)x, (int)y, largo * 0.5f, alto, cuerpo);
        DrawEllipse((int)(x + largo * 0.18f), (int)(y - alto * 0.5f), largo * 0.3f, alto * 0.9f, cuerpo);
        DrawEllipse((int)(x + largo * 0.05f), (int)(y + alto * 0.45f), largo * 0.42f, alto * 0.45f, panza);
    }

    // Bandada que cruza despacio (aleteo).
    if (decoracion)
    {
        float recorrido = (float)w + 240.0f * s;
        float cx = (float)w + 120.0f * s - std::fmod(t * 22.0f * s, recorrido);
        float cy = PuntoCieloG(vista, 0.0f, -18.6f).y;

        for (int b = 0; b < 5; b++)
        {
            float bx = cx + (float)b * 16.0f * s + (float)(b % 2) * 6.0f * s;
            float by = cy + (float)((b * 7) % 5) * 5.0f * s - 8.0f * s;
            float ala = std::sin(t * 9.0f + (float)b * 1.7f) * 3.5f * s;
            Color ave = Color{ 64, 36, 54, 255 };
            DrawLineEx({ bx - 6.0f * s, by - ala }, { bx, by + 1.5f * s }, 1.6f * s, ave);
            DrawLineEx({ bx, by + 1.5f * s }, { bx + 6.0f * s, by - ala }, 1.6f * s, ave);
        }
    }
}


//==================================================
// VISUAL: ANIMADO, MARCAS, CABLES, PARTICULAS Y LUCES
//==================================================

// Altura del plato de la prensa en su ciclo (baja, aprieta y sube).
static float AlturaPlatoPrensaG(float t)
{
    float f = std::fmod(t, PERIODO_PRENSA_GRUA) / PERIODO_PRENSA_GRUA;
    const float arriba = 4.3f, abajo = 1.45f;

    if (f < 0.18f) return arriba;
    if (f < 0.42f)
    {
        float u = (f - 0.18f) / 0.24f;
        return arriba + (abajo - arriba) * u * u;
    }
    if (f < 0.56f) return abajo;

    float u = Lim01G((f - 0.56f) / 0.4f);
    return abajo + (arriba - abajo) * (u * u * (3.0f - 2.0f * u));
}


static bool BalizaEncendidaG(float t, int indice)
{
    return std::fmod(t * 0.9f + (float)indice * 0.37f, 1.0f) < 0.45f;
}


static void DibujarAnimadoGrua(const MinijuegoGruaChatarra& m, bool decoracion, bool efectos)
{
    float t = m.tiempoAnimacion;
    (void)efectos;
    LoteG();

    // Prensa: plato, vastago y la chatarra que se aplasta.
    Vector3 c = CENTRO_PRENSA_GRUA;
    float plato = AlturaPlatoPrensaG(t);
    float carga = plato - 0.2f < 2.2f ? plato - 0.2f : 2.2f;
    CajaG({ c.x, 0.5f * (1.0f + carga), c.z + 0.1f }, { 2.3f, carga - 1.0f, 2.9f }, MezclaG(COLORES_AUTO_G[1], COLOR_OXIDO_G, 0.5f));
    CajaG({ c.x - 0.4f, carga + 0.05f, c.z + 0.6f }, { 0.9f, 0.1f, 1.0f }, COLORES_AUTO_G[3]);
    CilG({ c.x, plato + 0.2f, c.z }, { c.x, 4.6f, c.z }, 0.2f, 0.2f, 10, Color{ 196, 200, 210, 255 }, false);
    CajaG({ c.x, plato, c.z }, { 2.4f, 0.4f, 3.0f }, Color{ 178, 112, 44, 255 });

    // Cintas: tablillas y trozos de chatarra que viajan hacia el fondo.
    const float cintas[2][3] = { { -11.0f, -3.4f, 6.6f }, { 11.0f, 0.4f, 6.6f } };
    int pasoTablilla = decoracion ? 1 : 2;

    for (int k = 0; k < 2; k++)
    {
        float x = cintas[k][0], z0 = cintas[k][1], z1 = cintas[k][2];
        float largo = z1 - z0;
        float avance = std::fmod(t * 0.9f, 0.5f);

        for (int i = 0; i < (int)(largo / 0.5f); i += pasoTablilla)
        {
            float z = z1 - std::fmod((float)i * 0.5f + avance, largo);
            CajaG({ x, 0.86f, z }, { 1.06f, 0.025f, 0.12f }, Color{ 70, 66, 72, 255 });
        }

        for (int i = 0; i < 4; i++)
        {
            float z = z1 - std::fmod((float)i * (largo / 4.0f) + t * 0.9f, largo);
            int h = k * 7 + i;
            CajaGiradaG({ x + (Ruido01Grua(h, 1) - 0.5f) * 0.4f, 0.98f, z }, { 0.5f, 0.2f + 0.15f * Ruido01Grua(h, 2), 0.4f }, Ruido01Grua(h, 3) * 90.0f, ColorChatarraG(h, 5));
        }
    }

    // Balizas de las torres del cablecam.
    for (int k = 0; k < 4; k++)
    {
        Vector3 b = { TORRES_GRUA[k].x, TORRES_GRUA[k].y + 1.05f, TORRES_GRUA[k].z };
        bool on = BalizaEncendidaG(t, k);
        g_emisG = on ? 1.0f : 0.0f;
        CajaG(b, { 0.2f, 0.2f, 0.2f }, on ? Color{ 255, 70, 50, 255 } : Color{ 90, 40, 40, 255 });
        g_emisG = 0.0f;
    }

    FinLoteG();

    // La grua torre del fondo gira despacio.
    if (decoracion)
    {
        rlBegin(RL_TRIANGLES);
        float yA = AlturaElevacionG(-42.0f, -18.9f);
        PlumaGruaTorreG({ -3.0f, yA + 0.9f, -42.0f }, 160.0f + 30.0f * std::sin(t * 0.11f), 13.0f, 4.5f, COLOR_SILUETA_G);
        rlEnd();
    }
}


// Calcomanias sobre el suelo: sombras, aros de valor y miras de las garras.
static void DibujarMarcasSueloGrua(const MinijuegoGruaChatarra& m, const Participante participantes[], int limite)
{
    float t = m.tiempoAnimacion;
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    LoteG();

    for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
    {
        const ObjetoGrua& o = m.objetos[k];

        if (!o.activo)
        {
            continue;
        }

        Vector3 c = { o.x, 0.012f, o.z };
        DiscoSueloG(c, 0.58f, 0.5f, 12, AlfaG(Color{ 26, 18, 22, 255 }, 0.42f), AlfaG(Color{ 26, 18, 22, 255 }, 0.0f), false);

        // El aro marca la tolerancia real de centrado de cada pieza.
        float r = TOLERANCIA_OBJETO_GRUA[o.tipo];
        Vector3 a = { o.x, 0.016f, o.z };

        if (o.tipo == OBJETO_GRUA_BATERIA)
        {
            float pulso = 0.5f + 0.5f * std::sin(t * 4.0f + (float)k);
            DiscoSueloG(a, r, r, 16, AlfaG(COLOR_DORADO_G, 0.28f + 0.12f * pulso), AlfaG(COLOR_DORADO_G, 0.05f), false);
            AnilloSueloG(a, r - 0.07f, r, 20, AlfaG(COLOR_DORADO_G, 0.95f));
        }
        else if (o.tipo == OBJETO_GRUA_MOTOR)
        {
            AnilloSueloG(a, r - 0.06f, r, 20, AlfaG(Color{ 130, 220, 255, 255 }, 0.8f));
        }
        else if (o.tipo == OBJETO_GRUA_ENGRANAJE)
        {
            AnilloSueloG(a, r - 0.04f, r, 20, AlfaG(Color{ 255, 190, 120, 255 }, 0.22f));
        }
        else if (o.tipo == OBJETO_GRUA_CARTUCHO)
        {
            float pulso = 0.6f + 0.4f * std::sin(t * 6.0f + (float)k);

            for (int s = 0; s < 8; s++)
            {
                float a0 = (float)s / 8.0f * 2.0f * PI + t * 0.6f;
                AnilloSueloG(a, r - 0.07f, r, 3, AlfaG(Color{ 255, 70, 60, 255 }, 0.75f * pulso), a0, a0 + 0.45f);
            }
        }
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        Color color = participantes[i].color;
        float altura = AlturaGarraGrua(e);
        float cerca = 1.0f - (altura - ALTURA_BAJA_GRUA) / (ALTURA_ALTA_GRUA - ALTURA_BAJA_GRUA);
        Vector3 c = { e.x, 0.02f, e.z };

        // Sombra del iman: se achica y oscurece cuando baja.
        float rs = 0.62f - 0.12f * cerca;
        DiscoSueloG(c, rs, rs, 14, AlfaG(Color{ 20, 12, 18, 255 }, 0.35f + 0.25f * cerca), AlfaG(Color{ 20, 12, 18, 255 }, 0.0f), false);

        // Mira con cuatro marcas que giran; se cierra si hay pieza a tiro.
        float distancia = 0.0f;
        int candidato = e.estado == GARRA_GRUA_LIBRE ? BuscarCandidatoGrua(m, e.x, e.z, distancia) : -1;
        float giro = t * (candidato >= 0 ? 2.4f : 0.9f) + (float)i;
        float radio = candidato >= 0 ? 0.68f : 0.82f;
        Vector3 cm = { e.x, 0.024f, e.z };
        AnilloSueloG(cm, radio - 0.05f, radio, 24, AlfaG(color, 0.9f));

        for (int s = 0; s < 4; s++)
        {
            float a = giro + (float)s * 0.5f * PI;
            Vector3 d = { std::cos(a), 0.0f, std::sin(a) };
            Vector3 n = { -d.z, 0.0f, d.x };
            Vector3 p0 = Vector3Add(cm, Vector3Scale(d, radio + 0.02f));
            Vector3 p1 = Vector3Add(cm, Vector3Scale(d, radio + 0.3f));
            Color blanco = AlfaG(WHITE, 0.95f);
            QuadColoresG(Vector3Add(p0, Vector3Scale(n, 0.05f)), Vector3Add(p1, Vector3Scale(n, 0.05f)),
                Vector3Subtract(p1, Vector3Scale(n, 0.05f)), Vector3Subtract(p0, Vector3Scale(n, 0.05f)), blanco, color, color, blanco);
        }

        DiscoSueloG({ e.x, 0.026f, e.z }, 0.07f, 0.07f, 8, WHITE, WHITE, false);

        // Corchetes del color del jugador sobre la pieza a tiro.
        if (candidato >= 0)
        {
            const ObjetoGrua& o = m.objetos[candidato];
            float r = TOLERANCIA_OBJETO_GRUA[o.tipo] + 0.12f;
            float latido = 1.0f + 0.06f * std::sin(t * 10.0f);
            r *= latido;

            for (int s = 0; s < 4; s++)
            {
                float a0 = (float)s * 0.5f * PI + 0.25f * PI - 0.32f;
                AnilloSueloG({ o.x, 0.028f, o.z }, r - 0.08f, r, 4, AlfaG(color, 0.95f), a0, a0 + 0.64f);
            }
        }

        // Fallo: cruz roja breve donde bajo la garra.
        if (m.sacudidaGarra[i] > 0.0f)
        {
            float a = Lim01G(m.sacudidaGarra[i] / 0.5f);
            Color rojo = AlfaG(Color{ 255, 70, 60, 255 }, 0.85f * a);
            Vector3 o = { e.origenX, 0.03f, e.origenZ };

            for (int s = 0; s < 2; s++)
            {
                float ang = 0.25f * PI + (float)s * 0.5f * PI;
                Vector3 d = { std::cos(ang) * 0.5f, 0.0f, std::sin(ang) * 0.5f };
                Vector3 n = { -d.z * 0.16f, 0.0f, d.x * 0.16f };
                QuadColoresG(Vector3Add(Vector3Subtract(o, d), n), Vector3Add(Vector3Add(o, d), n),
                    Vector3Subtract(Vector3Add(o, d), n), Vector3Subtract(Vector3Subtract(o, d), n), rojo, rojo, rojo, rojo);
            }
        }
    }

    FinLoteG();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Relleno de cada tolva segun los puntos y piloto del color del jugador.
static void DibujarTolvasExtraGrua(const MinijuegoGruaChatarra& m, const Participante participantes[], int limite)
{
    LoteG();

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        float x = TolvaXGrua(m, e.puesto);
        float lleno = Lim01G((float)e.puntos / 40.0f);

        if (e.puntos > 0)
        {
            BolaG({ x, 1.12f + 0.32f * lleno, Z_TOLVA_GRUA }, { 0.55f + 0.3f * lleno, 0.18f + 0.14f * lleno, 0.42f + 0.2f * lleno }, COLOR_OXIDO_G, 2, 8);

            for (int k = 0; k < 2 + (int)(lleno * 4.0f); k++)
            {
                int h = i * 31 + k;
                CajaGiradaG({ x + (Ruido01Grua(h, 1) - 0.5f) * 1.0f, 1.32f + 0.4f * lleno, Z_TOLVA_GRUA + (Ruido01Grua(h, 2) - 0.5f) * 0.6f },
                    { 0.24f, 0.12f, 0.3f }, Ruido01Grua(h, 3) * 180.0f, ColorChatarraG(h, 9), 20.0f);
            }
        }

        // Piloto sobre un mastil al costado de la tolva.
        Vector3 poste = { x + 1.38f, 0.0f, Z_TOLVA_GRUA + 0.55f };
        CajaG({ poste.x, 1.1f, poste.z }, { 0.08f, 2.2f, 0.08f }, COLOR_ACERO_OSCURO_G);
        g_emisG = 1.0f;
        Color piloto = MezclaG(participantes[i].color, WHITE, 0.25f + 0.5f * m.destelloTolva[i]);
        CajaG({ poste.x, 2.32f, poste.z }, { 0.26f, 0.26f, 0.26f }, piloto);
        g_emisG = 0.0f;
    }

    FinLoteG();
}


// Cable grueso del iman, tensores del cablecam y linea de plomada.
static void DibujarCablesGrua(const MinijuegoGruaChatarra& m, const Participante participantes[], int limite)
{
    LoteG();

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        float altura = AlturaGarraGrua(e);
        Color cable = Color{ 36, 32, 38, 255 };
        TiraG({ e.x, 8.0f, e.z }, { e.x, altura + 0.35f, e.z }, 0.06f, cable, cable);

        Color tensor = Color{ 52, 36, 50, 150 };

        for (int k = 0; k < 4; k++)
        {
            Vector3 polea = { TORRES_GRUA[k].x, TORRES_GRUA[k].y + 0.8f, TORRES_GRUA[k].z };
            TiraG({ e.x, 8.2f, e.z }, polea, 0.026f, tensor, AlfaG(tensor, 0.6f));
        }

        // Plomada punteada del color del jugador hasta su mira.
        if (e.estado == GARRA_GRUA_LIBRE)
        {
            Color c = AlfaG(participantes[i].color, 0.6f);

            for (float y = altura - 0.45f; y > 0.15f; y -= 0.32f)
            {
                TiraG({ e.x, y, e.z }, { e.x, y - 0.16f, e.z }, 0.05f, c, c);
            }
        }
    }

    FinLoteG();
}


static void DibujarParticulasGrua(const MinijuegoGruaChatarra& m, bool aditivas)
{
    LoteG();

    for (int i = 0; i < MAX_PARTICULAS_GRUA; i++)
    {
        const ParticulaGrua& p = m.particulas[i];

        if (!p.activa || (p.tipo == PARTICULA_GRUA_CHISPA) != aditivas)
        {
            continue;
        }

        float f = Lim01G(p.vida / p.vidaMaxima);

        if (p.tipo == PARTICULA_GRUA_CHISPA)
        {
            Vector3 cola = Vector3Subtract(p.posicion, Vector3Scale(p.velocidad, 0.035f));
            Color c = AlfaG(p.color, f);
            TiraG(cola, p.posicion, p.tamano, AlfaG(c, 0.0f), c);
        }
        else if (p.tipo == PARTICULA_GRUA_POLVO || p.tipo == PARTICULA_GRUA_HUMO)
        {
            PuntoG(p.posicion, p.tamano, AlfaG(p.color, f * 0.55f));
        }
        else if (p.tipo == PARTICULA_GRUA_CONFETI)
        {
            float giro = std::sin(m.tiempoAnimacion * 8.0f + (float)i);
            Vector3 d = Vector3Scale(g_derG, p.tamano * giro);
            Vector3 u = Vector3Scale(g_arribaG, p.tamano * 0.6f);
            Color c = AlfaG(p.color, f < 0.3f ? f / 0.3f : 1.0f);
            QuadColoresG(Vector3Subtract(Vector3Subtract(p.posicion, d), u), Vector3Subtract(Vector3Add(p.posicion, d), u),
                Vector3Add(Vector3Add(p.posicion, d), u), Vector3Add(Vector3Subtract(p.posicion, d), u), c, c, c, c);
        }
        else
        {
            PuntoG(p.posicion, p.tamano, AlfaG(p.color, f < 0.3f ? f / 0.3f : 1.0f));
        }
    }

    FinLoteG();
}


// Cono de luz desde un foco hasta su charco en el suelo.
static void ConoLuzG(Vector3 foco, Vector3 suelo, float radio, Color color)
{
    const int lados = 12;
    Vector3 eje = Vector3Normalize(Vector3Subtract(suelo, foco));
    Vector3 ref = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, ref));
    Vector3 v = Vector3CrossProduct(eje, u);
    Color borde = AlfaG(color, 0.0f);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(suelo, Vector3Add(Vector3Scale(u, std::cos(a0) * radio), Vector3Scale(v, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(suelo, Vector3Add(Vector3Scale(u, std::cos(a1) * radio), Vector3Scale(v, std::sin(a1) * radio)));
        VG(foco, color); VG(p0, borde); VG(p1, borde);
    }
}


static void CharcoLuzG(Vector3 c, float radio, Color color, float intensidad)
{
    DiscoSueloG({ c.x, 0.03f, c.z }, radio, radio * 0.85f, 18, AlfaG(color, intensidad), AlfaG(color, 0.0f), false);
    DiscoSueloG({ c.x, 0.032f, c.z }, radio * 0.45f, radio * 0.4f, 14, AlfaG(color, intensidad * 0.8f), AlfaG(color, 0.0f), false);
}


static void DibujarLucesGrua(const MinijuegoGruaChatarra& m, const Participante participantes[], int limite, bool decoracion)
{
    float t = m.tiempoAnimacion;
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteG();

    // Focos de las torres traseras sobre el pozo.
    for (int k = 0; k < 2; k++)
    {
        float lado = TORRES_GRUA[k].x < 0.0f ? 1.0f : -1.0f;
        Vector3 foco = { TORRES_GRUA[k].x + lado * 0.8f, TORRES_GRUA[k].y - 0.6f, TORRES_GRUA[k].z + 0.2f };
        Vector3 charco = { -lado * 4.6f, 0.0f, -0.6f };
        ConoLuzG(foco, charco, 3.0f, AlfaG(COLOR_LUZ_G, 0.05f));
        CharcoLuzG(charco, 4.2f, COLOR_LUZ_G, 0.07f);
        HaloG(foco, 1.1f, AlfaG(COLOR_SOL_G, 0.45f), AlfaG(COLOR_SOL_G, 0.0f));
    }

    // Tambor con fuego: titila y calienta el suelo.
    float llama = 0.75f + 0.15f * std::sin(t * 13.0f) + 0.1f * std::sin(t * 29.0f + 1.3f);
    Vector3 fuego = { -11.2f, 1.25f, -6.6f };
    HaloG(fuego, 1.0f * llama, AlfaG(Color{ 255, 170, 70, 255 }, 0.7f), AlfaG(COLOR_FUEGO_G, 0.0f));
    HaloG({ fuego.x, fuego.y + 0.25f, fuego.z }, 0.45f * llama, AlfaG(Color{ 255, 230, 150, 255 }, 0.8f), AlfaG(Color{ 255, 200, 100, 255 }, 0.0f));
    CharcoLuzG({ fuego.x, 0.0f, fuego.z }, 3.0f * llama, COLOR_FUEGO_G, 0.12f);

    if (decoracion)
    {
        // Arco de soldadura azulado.
        float arco = 0.6f + 0.4f * std::sin(t * 37.0f) * std::sin(t * 11.0f);
        if (m.tiempoChispasSoldadura < 0.2f)
        {
            HaloG(PUNTO_SOLDADURA_GRUA, 0.8f * arco, AlfaG(Color{ 190, 230, 255, 255 }, 0.8f), AlfaG(Color{ 120, 180, 255, 255 }, 0.0f));
            CharcoLuzG(PUNTO_SOLDADURA_GRUA, 1.8f, Color{ 140, 190, 255, 255 }, 0.08f * arco);
        }

        // Ventana de la caseta y del galpon.
        CharcoLuzG({ 19.5f, 0.0f, 7.4f }, 1.8f, COLOR_LUZ_G, 0.07f);
    }

    // Baterias: lo mas valioso brilla en dorado.
    for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
    {
        const ObjetoGrua& o = m.objetos[k];

        if (!o.activo || o.tipo != OBJETO_GRUA_BATERIA)
        {
            continue;
        }

        float pulso = 0.75f + 0.25f * std::sin(t * 4.0f + (float)k);
        HaloG({ o.x, 0.55f, o.z }, 1.0f * pulso, AlfaG(COLOR_DORADO_G, 0.38f), AlfaG(COLOR_DORADO_G, 0.0f));
        CharcoLuzG({ o.x, 0.0f, o.z }, 1.1f, COLOR_DORADO_G, 0.1f * pulso);
    }

    // Pilotos de las tolvas (se encienden al recibir chatarra).
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        float x = TolvaXGrua(m, e.puesto);
        float d = m.destelloTolva[i];
        Color color = participantes[i].color;
        HaloG({ x + 1.38f, 2.32f, Z_TOLVA_GRUA + 0.55f }, 0.5f + 0.9f * d, AlfaG(color, 0.55f + 0.4f * d), AlfaG(color, 0.0f));

        if (d > 0.01f)
        {
            CharcoLuzG({ x, 0.0f, Z_TOLVA_GRUA + 0.6f }, 2.6f, color, 0.22f * d);
            HaloG({ x, 1.9f, Z_TOLVA_GRUA }, 1.6f * d, AlfaG(WHITE, 0.4f * d), AlfaG(color, 0.0f));
        }

        // La garra que lleva una bateria tambien brilla.
        if (LlevaObjetoGrua(e) && e.tipoCapturado == OBJETO_GRUA_BATERIA)
        {
            HaloG({ e.x, AlturaGarraGrua(e) - 0.4f, e.z }, 1.0f, AlfaG(COLOR_DORADO_G, 0.4f), AlfaG(COLOR_DORADO_G, 0.0f));
        }
    }

    // Balizas rojas de las torres.
    for (int k = 0; k < 4; k++)
    {
        if (BalizaEncendidaG(t, k))
        {
            HaloG({ TORRES_GRUA[k].x, TORRES_GRUA[k].y + 1.05f, TORRES_GRUA[k].z }, 0.7f, AlfaG(Color{ 255, 80, 60, 255 }, 0.6f), AlfaG(Color{ 255, 60, 40, 255 }, 0.0f));
        }
    }

    FinLoteG();
    DibujarParticulasGrua(m, true);
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: ANUNCIOS Y HUD
//==================================================

static void DibujarAnunciosGrua(
    const MinijuegoGruaChatarra& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    // Cartelito J# sobre cada operador.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.45f;
        Vector2 p = GetWorldToScreen(cabeza, vista);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        DibujarEtiquetaUI(TextFormat("J%d", numero), p, 0.0f, opciones);
    }

    // Mensajes breves sobre cada garra: suben y se apagan.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa || e.tiempoMensaje <= 0.0f)
        {
            continue;
        }

        float vida = 1.0f - e.tiempoMensaje / 1.2f;
        float alfa = e.tiempoMensaje < 0.35f ? e.tiempoMensaje / 0.35f : 1.0f;
        float escala = vida < 0.12f ? 0.7f + vida * 2.5f : 1.0f;
        const char* texto = "";
        Color color = UI_TEXTO_MUNDO;
        int token = UI_TEXTO_GRANDE;

        if (e.mensaje == 1)
        {
            texto = TextFormat("+%d", e.valorMensaje);
            color = e.valorMensaje >= 8 ? COLOR_DORADO_G : (e.valorMensaje >= 5 ? Color{ 150, 236, 255, 255 } : Color{ 170, 240, 140, 255 });
        }
        else if (e.mensaje == 2)
        {
            texto = "¡BLOQUEADA!";
            color = Color{ 255, 170, 80, 255 };
            token = UI_TEXTO;
        }
        else if (e.mensaje == 3)
        {
            texto = "¡CARTUCHO! -2";
            color = Color{ 255, 110, 96, 255 };
            token = UI_TEXTO;
        }
        else
        {
            texto = "¡FALLÓ!";
            color = Color{ 236, 212, 200, 255 };
            token = UI_TEXTO;
        }

        // La entrega sube desde la tolva; el resto, desde donde bajo la garra.
        Vector3 punto = e.mensaje == 1
            ? Vector3{ TolvaXGrua(m, e.puesto), 2.4f + vida * 1.0f, Z_TOLVA_GRUA + 0.4f }
            : Vector3{ e.origenX, ALTURA_ALTA_GRUA + 0.9f + vida * 0.9f, e.origenZ };
        Vector2 p = GetWorldToScreen(punto, vista);
        DibujarTextoMundoContornoUI(texto, p.x, p.y - (1.0f - escala) * 12.0f, token, color, alfa);

        if (e.mensaje == 1 && e.valorMensaje >= 8)
        {
            DibujarTextoMundoContornoUI("¡BATERÍA!", p.x, p.y - 30.0f * EscalaUI(), UI_TEXTO, COLOR_DORADO_G, alfa);
        }
    }
}


static void PrepararHudGrua(
    const MinijuegoGruaChatarra& m,
    const Participante participantes[],
    int limite,
    DatosHudMinijuego& hud
)
{
    PrepararDatosHudMinijuego(
        hud,
        "GRÚA DE CHATARRA",
        "CENTRÁ LA GARRA, BAJALA Y LLEVÁ LA CHATARRA A TU TOLVA: BATERÍA 8, MOTOR 5, ENGRANAJE 3, TUERCA 1. ¡EL CARTUCHO RESTA 2!",
        m.resultado
    );

    hud.fase = m.fase == FASE_GRUA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_GRUA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_GRUA;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = true;

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& e = m.estadosJugadores[i];

        if (!e.participa)
        {
            continue;
        }

        JugadorHudMinijuego& jugador = hud.jugadores[i];
        const Participante& participante = participantes[i];

        jugador.visible = true;
        jugador.valorNumerico = e.puntos;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d PTS", e.puntos);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d PTS", e.puntos);
        std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "MEJOR PIEZA %d", e.mejorObjeto);

        if (m.fase == FASE_GRUA_JUGANDO)
        {
            if (e.tiempoMensaje > 0.0f && e.mensaje == 1)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡ENTREGÓ +%d!", e.valorMensaje);
            else if (e.tiempoMensaje > 0.0f && e.mensaje == 2)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡BLOQUEADA!");
            else if (e.tiempoMensaje > 0.0f && e.mensaje == 3)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡CARTUCHO! -2");
            else if (e.tiempoMensaje > 0.0f)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡FALLÓ!");
            else if (LlevaObjetoGrua(e))
                std::snprintf(jugador.estado, sizeof(jugador.estado), "A LA TOLVA");
            else if (e.estado == GARRA_GRUA_ACCION)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "BAJANDO");
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MOVÉ: %s\nBAJÁ: %s / %s",
                TextoMoverHud(participante),
                TextoBotonGolpeHud(participante),
                ObtenerTextoBotonPrincipal(participante)
            );
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoGruaChatarra::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteJugadoresGrua(cantidadMaxima);
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(COLOR_HORIZONTE_G);

    Camera3D vista = CamaraVistaGrua(*this);
    g_ojoG = OJO_GRUA;
    g_camG = vista.position;
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derG = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaG = Vector3CrossProduct(g_derG, frente);

    DibujarCieloGrua(vista, tiempoAnimacion, decoracion);
    HornearGrua(cantidadPuestos);

    BeginMode3D(vista);

    // El horneado lleva la bruma calculada desde la camara final.
    rlDisableBackfaceCulling();
    DibujarHornoSueloG();
    DibujarMarcasSueloGrua(*this, participantes, limite);
    rlDrawRenderBatchActive();
    DibujarHornoEscenaG();
    DibujarAnimadoGrua(*this, decoracion, efectos);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    // Chatarra del pozo.
    for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
    {
        if (!objetos[k].activo)
        {
            continue;
        }

        DibujarObjetoGrua(objetos[k].tipo, objetos[k].x, 0.1f, objetos[k].z, objetos[k].giro);

        if (mostrarDebug)
        {
            DrawCircle3D({ objetos[k].x, 0.08f, objetos[k].z }, TOLERANCIA_OBJETO_GRUA[objetos[k].tipo], { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }
    }

    // Tolvas, operadores y garras (en orden de participante).
    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].participa)
        {
            DibujarTolvaGrua(TolvaXGrua(*this, estadosJugadores[i].puesto), participantes[i].color);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (!estadosJugadores[i].participa)
        {
            continue;
        }

        JugadorPrueba visual = jugadores[i];
        visual.direccionMirada = { 0.0f, 0.0f, 1.0f };
        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorModeloCompartido(visual, participanteVisual);
    }

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].participa)
        {
            DibujarGarraGrua(estadosJugadores[i], participantes[i].color, tiempoAnimacion);
        }
    }

    rlDisableBackfaceCulling();
    DibujarTolvasExtraGrua(*this, participantes, limite);
    DibujarCablesGrua(*this, participantes, limite);
    DibujarParticulasGrua(*this, false);
    rlDrawRenderBatchActive();

    if (efectos)
    {
        DibujarLucesGrua(*this, participantes, limite, decoracion);
    }
    else
    {
        DibujarParticulasGrua(*this, true);
        rlDrawRenderBatchActive();
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    DibujarAnunciosGrua(*this, jugadores, participantes, limite, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    DatosHudMinijuego hud;
    PrepararHudGrua(*this, participantes, limite, hud);
    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoGruaChatarra::ObtenerResultado() const
{
    return resultado;
}
