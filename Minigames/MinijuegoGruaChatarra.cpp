#include "Minigames/MinijuegoGruaChatarra.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


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


static const char* TextoBotonAccionGrua(const Participante& participante)
{
    if (participante.control == CONTROL_GAMEPAD)
    {
        return "B";
    }

    return participante.control == CONTROL_TECLADO_FLECHAS ? "SHIFT DER" : "E";
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

    // Camara cenital diagonal que encuadra el pozo y las tolvas.
    camara.position = { 0.0f, 16.0f, 11.5f };
    camara.target = { 0.0f, 0.0f, -0.3f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_GRUA_PREPARACION;
    cantidadPuestos = 0;
    partidaValida = false;
    empate = false;

    tiempoPreparacion = DURACION_PREPARACION_GRUA;
    tiempoRestante = DURACION_PARTIDA_GRUA;
    tiempoAnimacion = 0.0f;
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

void MinijuegoGruaChatarra::Actualizar(
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
// VISUAL: ESCENARIO
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


static void DibujarObjetoGrua(int tipo, float x, float y, float z, float giro)
{
    float rad = giro * 0.0174533f;

    if (tipo == OBJETO_GRUA_TUERCA)
    {
        DrawCylinder({ x, y, z }, 0.38f, 0.38f, 0.22f, 6, Color{ 170, 176, 186, 255 });
        DrawCylinder({ x, y + 0.2f, z }, 0.16f, 0.16f, 0.04f, 6, Color{ 50, 52, 58, 255 });
    }
    else if (tipo == OBJETO_GRUA_ENGRANAJE)
    {
        DrawCylinder({ x, y, z }, 0.5f, 0.5f, 0.2f, 10, Color{ 224, 140, 52, 255 });
        DrawCylinder({ x, y + 0.18f, z }, 0.18f, 0.18f, 0.06f, 8, Color{ 70, 60, 54, 255 });

        for (int k = 0; k < 6; k++)
        {
            float a = rad + (float)k * 1.0471976f;
            DrawCube({ x + std::cos(a) * 0.55f, y + 0.1f, z + std::sin(a) * 0.55f }, 0.18f, 0.18f, 0.18f, Color{ 224, 140, 52, 255 });
        }
    }
    else if (tipo == OBJETO_GRUA_MOTOR)
    {
        DrawCube({ x, y + 0.4f, z }, 0.95f, 0.7f, 0.7f, Color{ 90, 110, 140, 255 });
        DrawCubeWires({ x, y + 0.4f, z }, 0.95f, 0.7f, 0.7f, Color{ 30, 36, 50, 255 });
        DrawCylinder({ x - 0.25f, y + 0.75f, z }, 0.12f, 0.12f, 0.3f, 6, Color{ 60, 66, 76, 255 });
        DrawCylinder({ x + 0.25f, y + 0.75f, z }, 0.12f, 0.12f, 0.3f, 6, Color{ 60, 66, 76, 255 });
    }
    else if (tipo == OBJETO_GRUA_BATERIA)
    {
        float brillo = 0.5f + 0.5f * std::sin(rad * 0.4f);
        DrawCube({ x, y + 0.35f, z }, 0.6f, 0.7f, 0.45f, Color{ 255, 205, 60, 255 });
        DrawCube({ x - 0.15f, y + 0.78f, z }, 0.1f, 0.12f, 0.1f, Color{ 60, 60, 66, 255 });
        DrawCube({ x + 0.15f, y + 0.78f, z }, 0.1f, 0.12f, 0.1f, Color{ 60, 60, 66, 255 });
        DrawSphere({ x, y + 0.4f, z }, 0.62f, Fade(Color{ 255, 220, 90, 255 }, 0.12f + 0.12f * brillo));
    }
    else
    {
        DrawCylinder({ x, y, z }, 0.32f, 0.32f, 0.55f, 8, Color{ 120, 60, 40, 255 });
        DrawCylinder({ x, y + 0.2f, z }, 0.335f, 0.335f, 0.1f, 8, Color{ 220, 190, 40, 255 });
    }
}


// Desguace: pozo de chatarra, pilas, autos aplastados, engranajes gigantes,
// prensa, cintas, tolvas, focos y grua con cables.
// MODELO FUTURO: pilas de chatarra y autos aplastados, engranajes gigantes,
// prensa hidraulica, cintas transportadoras, tolvas con embudo, focos
// industriales, garra magnetica y los objetos (tuerca, engranaje, motor,
// bateria, cartucho) como GLB.
static void DibujarDesgueceGrua(const MinijuegoGruaChatarra& minijuego)
{
    float t = minijuego.tiempoAnimacion;
    const Color metal = Color{ 84, 88, 98, 255 };
    const Color metalOscuro = Color{ 52, 54, 62, 255 };
    const Color oxido = Color{ 140, 78, 50, 255 };

    // Suelo del pozo y del patio.
    DrawCube({ 0.0f, -0.25f, 0.2f }, 21.0f, 0.5f, 15.0f, Color{ 66, 60, 56, 255 });
    DrawCube({ 0.0f, -0.18f, 0.2f }, 18.2f, 0.4f, 11.2f, Color{ 78, 66, 58, 255 });

    // Escombros del pozo (bajos para no tapar los objetos).
    for (int i = 0; i < 46; i++)
    {
        float x = -8.8f + Ruido01Grua(i, 1) * 17.6f;
        float z = -5.0f + Ruido01Grua(i, 2) * 10.4f;
        float tam = 0.18f + Ruido01Grua(i, 3) * 0.3f;
        Color color = i % 3 == 0 ? oxido : (i % 3 == 1 ? Color{ 98, 100, 108, 255 } : Color{ 60, 56, 52, 255 });

        DrawCube({ x, 0.05f + tam * 0.4f, z }, tam * 1.6f, tam * 0.5f, tam * 1.1f, color);
    }

    // Muro del fondo con engranajes gigantes.
    DrawCube({ 0.0f, 3.0f, -9.2f }, 24.0f, 6.0f, 0.8f, metalOscuro);

    for (int g = 0; g < 2; g++)
    {
        float cx = g == 0 ? -7.0f : 7.5f;
        float giro = t * (g == 0 ? 0.6f : -0.5f);

        DrawCylinderEx({ cx, 3.4f, -8.7f }, { cx, 3.4f, -8.2f }, 2.0f, 2.0f, 16, Color{ 110, 82, 54, 255 });
        DrawCylinderEx({ cx, 3.4f, -8.2f }, { cx, 3.4f, -8.0f }, 0.6f, 0.6f, 8, metalOscuro);

        for (int k = 0; k < 12; k++)
        {
            float a = giro + (float)k * 0.5235988f;
            DrawCube({ cx + std::cos(a) * 2.15f, 3.4f + std::sin(a) * 2.15f, -8.45f }, 0.5f, 0.5f, 0.5f, Color{ 110, 82, 54, 255 });
        }
    }

    // Prensa hidraulica a la derecha del fondo.
    float prensa = 0.5f + 0.5f * std::sin(t * 1.4f);
    DrawCube({ 10.6f, 2.6f, -3.5f }, 1.2f, 5.2f, 1.2f, metal);
    DrawCube({ 10.6f, 5.0f, -3.5f }, 2.4f, 0.6f, 1.6f, metalOscuro);
    DrawCube({ 10.6f, 4.0f - prensa * 1.5f, -3.5f }, 2.0f, 0.5f, 1.4f, Color{ 190, 150, 50, 255 });
    DrawCube({ 10.6f, 0.2f, -3.5f }, 2.4f, 0.4f, 1.8f, metalOscuro);

    // Autos aplastados a la izquierda del fondo.
    for (int k = 0; k < 3; k++)
    {
        Color carroceria = k == 0 ? Color{ 170, 50, 46, 255 } : (k == 1 ? Color{ 60, 100, 160, 255 } : Color{ 190, 170, 70, 255 });
        float y = 0.3f + (float)k * 0.5f;

        DrawCube({ -10.8f, y, -3.5f + (float)(k % 2) * 0.2f }, 1.6f, 0.45f, 3.0f, carroceria);
        DrawCube({ -10.8f, y + 0.3f, -3.5f }, 1.2f, 0.2f, 1.6f, Color{ 50, 60, 70, 255 });
    }

    // Cintas transportadoras laterales con franjas en movimiento.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 9.9f;
        DrawCube({ x, 0.2f, 1.6f }, 1.2f, 0.4f, 8.0f, metalOscuro);

        for (int k = 0; k < 8; k++)
        {
            float z = -2.2f + std::fmod((float)k * 1.0f + t * 1.2f * (float)lado, 8.0f);
            DrawCube({ x, 0.43f, z + 0.0f }, 1.0f, 0.06f, 0.18f, Color{ 220, 190, 50, 255 });
        }
    }

    // Pilas de chatarra al frente.
    for (int i = 0; i < 9; i++)
    {
        float x = -10.0f + (float)i * 2.5f;
        float alto = 0.8f + Ruido01Grua(i, 7) * 1.0f;
        DrawCube({ x, alto * 0.5f, 6.6f }, 1.8f, alto, 1.2f, i % 2 == 0 ? oxido : Color{ 96, 98, 106, 255 });
    }

    // Focos industriales.
    for (int k = 0; k < 4; k++)
    {
        float x = -8.0f + (float)k * 5.4f;
        DrawCube({ x, 3.0f, 8.0f }, 0.2f, 6.0f, 0.2f, metal);
        DrawSphere({ x, 6.0f, 7.8f }, 0.3f, Color{ 255, 236, 170, 255 });
        DrawSphere({ x, 6.0f, 7.8f }, 0.8f, Fade(Color{ 255, 230, 150, 255 }, 0.14f));
    }
}


static void DibujarTolvasGrua(
    const MinijuegoGruaChatarra& minijuego,
    const Participante participantes[],
    int limite
)
{
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& estado = minijuego.estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        float x = TolvaXGrua(minijuego, estado.puesto);
        Color color = participantes[i].color;

        DrawCube({ x, 0.6f, Z_TOLVA_GRUA }, 2.4f, 1.2f, 1.6f, Color{ 60, 62, 70, 255 });
        DrawCube({ x, 1.25f, Z_TOLVA_GRUA + 0.78f }, 2.4f, 0.12f, 0.1f, color);
        DrawCylinder({ x, 1.2f, Z_TOLVA_GRUA }, 1.2f, 0.7f, 0.5f, 8, color);

        // Puesto del operador.
        DrawCube({ x, 0.3f, Z_TOLVA_GRUA - 1.5f }, 2.0f, 0.6f, 1.6f, Color{ 90, 94, 104, 255 });
        DrawCube({ x, 0.65f, Z_TOLVA_GRUA - 2.2f }, 1.6f, 0.9f, 0.2f, Color{ 52, 54, 62, 255 });
    }
}


static void DibujarGarraGrua(
    const EstadoJugadorGrua& estado,
    Color color,
    float t
)
{
    float altura = AlturaGarraGrua(estado);
    float x = estado.x;
    float z = estado.z;

    // Apertura de las pinzas.
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

    // Sombra / marca de la garra en el suelo.
    DrawCircle3D({ x, 0.04f, z }, 0.55f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.9f));
    DrawCircle3D({ x, 0.045f, z }, 0.08f, { 1.0f, 0.0f, 0.0f }, 90.0f, WHITE);

    // Cable desde el carro del puente grua.
    DrawLine3D({ x, 8.0f, z }, { x, altura + 0.35f, z }, Color{ 40, 40, 46, 255 });
    DrawCube({ x, 8.0f, z }, 0.7f, 0.3f, 0.7f, Color{ 60, 62, 70, 255 });

    // Iman y pinzas.
    DrawCylinder({ x, altura, z }, 0.45f, 0.5f, 0.3f, 12, color);
    DrawCylinder({ x, altura + 0.3f, z }, 0.2f, 0.2f, 0.2f, 8, Color{ 60, 62, 70, 255 });

    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.5707963f + 0.7853982f;
        DrawCube({ x + std::cos(a) * apertura * 0.7f, altura - 0.2f, z + std::sin(a) * apertura * 0.7f }, 0.1f, 0.5f, 0.1f, Color{ 190, 190, 200, 255 });
    }

    if (estado.estado == GARRA_GRUA_ACCION)
    {
        bool lleva =
            estado.tipoCapturado >= 0 &&
            !estado.depositado &&
            estado.tiempo >= T_CERRAR_GRUA;

        if (lleva)
        {
            DibujarObjetoGrua(estado.tipoCapturado, x, altura - 0.75f, z, t * 40.0f);
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

    ClearBackground(Color{ 34, 34, 40, 255 });
    BeginMode3D(camara);

    DibujarDesgueceGrua(*this);
    DibujarTolvasGrua(*this, participantes, limite);

    for (int k = 0; k < MAX_OBJETOS_GRUA; k++)
    {
        if (objetos[k].activo)
        {
            DibujarObjetoGrua(objetos[k].tipo, objetos[k].x, 0.1f, objetos[k].z, objetos[k].giro);

            if (mostrarDebug)
            {
                DrawCircle3D({ objetos[k].x, 0.08f, objetos[k].z }, TOLERANCIA_OBJETO_GRUA[objetos[k].tipo], { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
            }
        }
    }

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        DibujarGarraGrua(estado, participantes[i].color, tiempoAnimacion);

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], participanteVisual);
    }

    EndMode3D();

    // HUD: marcadores.
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (estadosJugadores[i].participa)
        {
            cantidad++;
        }
    }

    int anchoPanel = 150 * cantidad + 20;
    int panelX = ancho / 2 - anchoPanel / 2;

    DrawRectangle(panelX, 10, anchoPanel, 78, Fade(BLACK, 0.78f));

    const char* titulo = TextFormat("GRUA DE CHATARRA  -  %.0f s", tiempoRestante);
    DrawText(titulo, ancho / 2 - MeasureText(titulo, 18) / 2, 14, 18, RAYWHITE);

    int columna = 0;

    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        int x = panelX + 10 + columna * 150;
        const char* nombre = TextFormat("J%d", participantes[i].numeroJugador);

        DrawRectangle(x, 40, 6, 40, participantes[i].color);
        DrawText(nombre, x + 12, 40, 18, participantes[i].color);
        DrawText(TextFormat("%d", estado.puntos), x + 12, 58, 26, RAYWHITE);
        columna++;
    }

    // Mensajes sobre cada garra.
    for (int i = 0; i < limite; i++)
    {
        const EstadoJugadorGrua& estado = estadosJugadores[i];

        if (!estado.participa || estado.tiempoMensaje <= 0.0f)
        {
            continue;
        }

        const char* texto = "";
        Color color = RAYWHITE;

        if (estado.mensaje == 1)
        {
            texto = TextFormat("+%d", estado.valorMensaje);
            color = LIME;
        }
        else if (estado.mensaje == 2)
        {
            texto = "BLOQUEADA";
            color = ORANGE;
        }
        else if (estado.mensaje == 3)
        {
            texto = "-2";
            color = RED;
        }
        else
        {
            texto = "FALLO";
            color = RED;
        }

        Vector2 pantalla = GetWorldToScreen({ estado.x, ALTURA_ALTA_GRUA + 0.8f, estado.z }, camara);
        DrawText(texto, (int)pantalla.x - MeasureText(texto, 20) / 2, (int)pantalla.y - 10, 20, color);
    }

    // HUD: controles.
    int y = alto - 34;
    for (int i = limite - 1; i >= 0; i--)
    {
        const EstadoJugadorGrua& estado = estadosJugadores[i];

        if (!estado.participa)
        {
            continue;
        }

        const char* estadoTexto = estado.estado == GARRA_GRUA_ACCION ? "OCUPADA" : "LISTA";
        const char* linea = EsControladoPorBotGrua(participantes[i])
            ? TextFormat("J%d BOT  %d PUNTOS", participantes[i].numeroJugador, estado.puntos)
            : TextFormat(
                "J%d  MOVER LA GARRA  BAJAR [%s] O SALTO [%s]  %s",
                participantes[i].numeroJugador,
                TextoBotonAccionGrua(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i]),
                estadoTexto
            );

        DrawRectangle(8, y - 3, MeasureText(linea, 18) + 20, 26, Fade(BLACK, 0.8f));
        DrawText(linea, 18, y, 18, participantes[i].color);
        y -= 28;
    }

    if (fase == FASE_GRUA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, ancho / 2 - MeasureText(texto, 90) / 2, alto / 2 - 50, 90, YELLOW);

        const char* pista = "CENTRA LA GARRA SOBRE EL OBJETO Y BAJA  -  BATERIA 8  MOTOR 5  ENGRANAJE 3  TUERCA 1  CARTUCHO -2";
        DrawText(pista, ancho / 2 - MeasureText(pista, 18) / 2, alto / 2 + 60, 18, RAYWHITE);
    }
    else if (
        fase == FASE_GRUA_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        DrawRectangle(ancho / 2 - 300, alto / 2 - 150, 600, 300, Fade(BLACK, 0.92f));

        const char* encabezado = empate ? "EMPATE" : "RESULTADO";
        DrawText(encabezado, ancho / 2 - MeasureText(encabezado, 32) / 2, alto / 2 - 130, 32, RAYWHITE);

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (!resultado.participantes[i].participo || resultado.participantes[i].posicionFinal != posicion)
                {
                    continue;
                }

                const char* fila = TextFormat(
                    "%d.  J%d  %d puntos",
                    posicion,
                    participantes[i].numeroJugador,
                    estadosJugadores[i].puntos
                );

                DrawText(fila, ancho / 2 - 120, alto / 2 - 80 + (posicion - 1) * 34, 26, participantes[i].color);
            }
        }

        DrawText(TextoReinicioMinijuego(), ancho / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2, alto / 2 + 120, 18, LIGHTGRAY);
    }
    else if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        const char* texto = "SE NECESITAN AL MENOS 2 JUGADORES";
        DrawText(texto, ancho / 2 - MeasureText(texto, 26) / 2, alto / 2, 26, RED);
    }
}


const ResultadoMinijuego& MinijuegoGruaChatarra::ObtenerResultado() const
{
    return resultado;
}
