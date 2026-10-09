#include "Minigames/MinijuegoCapsulasBarajadas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_CAPSULAS = 3.0f;
static const float DURACION_MOSTRAR_CAPSULAS = 2.4f;
static const float PAUSA_INICIAL_BARAJADO_CAPSULAS = 0.5f;
static const float DURACION_ELEGIR_CAPSULAS = 4.0f;
static const float DURACION_REVELAR_CAPSULAS = 2.6f;

// Apuesta doble: confirmar dentro de este tiempo vale 2 puntos.
static const float VENTANA_APUESTA_DOBLE_CAPSULAS = 1.0f;

static const int CAPSULAS_POR_RONDA[RONDAS_CAPSULAS] = { 3, 3, 4, 4, 5 };

// Geometria de la mesa y la pasarela.
static const float ALTURA_MESA_CAPSULAS = 1.01f;
static const float ALTURA_PASARELA_CAPSULAS = 0.3f;
static const float Z_PASARELA_CAPSULAS = 4.8f;
static const float Z_MARCADOR_CAPSULAS = 1.5f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarCapsulas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float SuavizadoCapsulas(float t)
{
    t = LimitarCapsulas(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}


static float AleatorioCapsulas(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static float SeparacionSlotCapsulas(int cantidad)
{
    if (cantidad <= 3) return 2.8f;
    if (cantidad == 4) return 2.5f;
    return 2.1f;
}


static float XSlotCapsulas(int slot, int cantidad)
{
    return ((float)slot - (float)(cantidad - 1) * 0.5f) * SeparacionSlotCapsulas(cantidad);
}


static float DuracionIntercambioCapsulas(int ronda)
{
    float duracion = 0.62f - 0.08f * (float)ronda;
    return duracion < 0.28f ? 0.28f : duracion;
}


static int LimiteCapsulas(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static const char* NombreJugadorCapsulas(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static void AsignarColoresCapsulas(
    MinijuegoCapsulasBarajadas& m,
    const Participante participantes[]
)
{
    const Color respaldo[] =
    {
        Color{ 232, 62, 62, 255 },
        Color{ 62, 124, 238, 255 },
        Color{ 66, 202, 96, 255 },
        Color{ 246, 206, 52, 255 },
        Color{ 232, 92, 204, 255 },
        Color{ 250, 144, 44, 255 }
    };
    const int cantidadRespaldo = (int)(sizeof(respaldo) / sizeof(respaldo[0]));

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.coloresJugadores[i] = participantes[i].color;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!participantes[i].activo || !participantes[i].esBot)
        {
            continue;
        }

        for (int c = 0; c < cantidadRespaldo; c++)
        {
            bool repetido = false;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (j == i || !participantes[j].activo)
                {
                    continue;
                }

                const Color& otro = (!participantes[j].esBot || j < i)
                    ? m.coloresJugadores[j]
                    : participantes[j].color;

                if (
                    otro.r == respaldo[c].r &&
                    otro.g == respaldo[c].g &&
                    otro.b == respaldo[c].b
                )
                {
                    repetido = true;
                }
            }

            if (!repetido)
            {
                m.coloresJugadores[i] = respaldo[c];
                break;
            }
        }
    }
}


static int IndiceVisualCapsulas(
    const MinijuegoCapsulasBarajadas& m,
    int indice,
    int& total
)
{
    int posicion = 0;
    total = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        if (i == indice)
        {
            posicion = total;
        }

        total++;
    }

    return posicion;
}


//==================================================
// LOGICA (datos logicos, sin dependencia visual)
//==================================================

static int CapsulaEnSlotCapsulas(const MinijuegoCapsulasBarajadas& m, int slot)
{
    for (int c = 0; c < m.cantidadCapsulas; c++)
    {
        if (m.slotDe[c] == slot)
        {
            return c;
        }
    }

    return 0;
}


static void IniciarRondaCapsulas(MinijuegoCapsulasBarajadas& m)
{
    m.cantidadCapsulas = CAPSULAS_POR_RONDA[m.ronda];
    m.subfase = SUBFASE_CAPSULAS_MOSTRAR;
    m.tiempoSubfase = 0.0f;
    m.revelado = 0.0f;
    m.premio = GetRandomValue(0, m.cantidadCapsulas - 1);
    m.swapsRestantes = 5 + m.ronda * 2;
    m.swapA = -1;
    m.swapB = -1;
    m.swapTiempo = 0.0f;
    m.pausaBarajado = PAUSA_INICIAL_BARAJADO_CAPSULAS;
    m.ultimoParSlotA = -1;
    m.ultimoParSlotB = -1;

    for (int c = 0; c < MAX_CAPSULAS; c++)
    {
        m.slotDe[c] = c;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCapsulas& estado = m.estados[i];
        estado.marcador = m.cantidadCapsulas / 2;
        estado.confirmado = false;
        estado.tiempoConfirmacion = 0.0f;
        estado.slotElegido = -1;
        estado.acerto = false;
        estado.puntosRonda = 0;
    }
}


static void IniciarEleccionCapsulas(MinijuegoCapsulasBarajadas& m)
{
    m.subfase = SUBFASE_CAPSULAS_ELEGIR;
    m.tiempoSubfase = 0.0f;

    int slotCorrecto = m.slotDe[m.premio];
    int probabilidad = 60 - 8 * m.ronda;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCapsulas& estado = m.estados[i];
        estado.botPaso = 0.0f;
        estado.botConfirmar = AleatorioCapsulas(0.6f, 2.5f);

        if (GetRandomValue(0, 99) < probabilidad)
        {
            estado.botObjetivo = slotCorrecto;
        }
        else
        {
            int incorrecto = GetRandomValue(0, m.cantidadCapsulas - 2);

            if (incorrecto >= slotCorrecto)
            {
                incorrecto++;
            }

            // Error humano plausible: suele elegir una capsula contigua a la correcta.
            if (GetRandomValue(0, 99) < 70)
            {
                int lado = GetRandomValue(0, 1) == 0 ? -1 : 1;
                int vecino = slotCorrecto + lado;

                if (vecino < 0 || vecino >= m.cantidadCapsulas)
                {
                    vecino = slotCorrecto - lado;
                }

                if (vecino >= 0 && vecino < m.cantidadCapsulas && vecino != slotCorrecto)
                {
                    incorrecto = vecino;
                }
            }

            estado.botObjetivo = incorrecto;
        }
    }
}


static void ConfirmarCapsulas(MinijuegoCapsulasBarajadas& m, int indice)
{
    EstadoJugadorCapsulas& estado = m.estados[indice];

    estado.confirmado = true;
    estado.slotElegido = estado.marcador;
    estado.tiempoConfirmacion = m.tiempoSubfase;
}


static void IniciarRevelacionCapsulas(MinijuegoCapsulasBarajadas& m)
{
    m.subfase = SUBFASE_CAPSULAS_REVELAR;
    m.tiempoSubfase = 0.0f;

    int slotCorrecto = m.slotDe[m.premio];
    bool algunAcierto = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        EstadoJugadorCapsulas& estado = m.estados[i];

        if (estado.confirmado && estado.slotElegido == slotCorrecto)
        {
            estado.acerto = true;
            estado.puntosRonda =
                estado.tiempoConfirmacion <= VENTANA_APUESTA_DOBLE_CAPSULAS ? 2 : 1;
            estado.puntos += estado.puntosRonda;
            estado.aciertos++;
            estado.tiempoAciertos += estado.tiempoConfirmacion;
            algunAcierto = true;
        }
    }

    ReproducirSonidoMinijuego(m.audio, algunAcierto ? SONIDO_ACIERTO : SONIDO_ERROR);
}


static void FinalizarCapsulas(MinijuegoCapsulasBarajadas& m, int limite)
{
    int mejores = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < limite; j++)
        {
            if (j == i || !m.participa[j])
            {
                continue;
            }

            const EstadoJugadorCapsulas& a = m.estados[j];
            const EstadoJugadorCapsulas& b = m.estados[i];

            if (
                a.puntos > b.puntos ||
                (a.puntos == b.puntos && a.tiempoAciertos < b.tiempoAciertos - 0.001f)
            )
            {
                posicion++;
            }
        }

        m.resultado.participantes[i].posicionFinal = posicion;
        m.resultado.participantes[i].puntuacionMinijuego = m.estados[i].puntos;

        if (posicion == 1)
        {
            mejores++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = mejores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;
    m.fase = FASE_CAPSULAS_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


static void ActualizarBarajadoCapsulas(MinijuegoCapsulasBarajadas& m, float deltaTime)
{
    if (m.pausaBarajado > 0.0f)
    {
        m.pausaBarajado -= deltaTime;
        return;
    }

    if (m.swapA < 0)
    {
        if (m.swapsRestantes <= 0)
        {
            IniciarEleccionCapsulas(m);
            return;
        }

        int slotA = 0;
        int slotB = 0;

        for (int intento = 0; intento < 20; intento++)
        {
            slotA = GetRandomValue(0, m.cantidadCapsulas - 1);
            slotB = GetRandomValue(0, m.cantidadCapsulas - 2);

            if (slotB >= slotA)
            {
                slotB++;
            }

            bool repetido =
                (slotA == m.ultimoParSlotA && slotB == m.ultimoParSlotB) ||
                (slotA == m.ultimoParSlotB && slotB == m.ultimoParSlotA);

            if (!repetido)
            {
                break;
            }
        }

        m.ultimoParSlotA = slotA;
        m.ultimoParSlotB = slotB;
        m.swapA = CapsulaEnSlotCapsulas(m, slotA);
        m.swapB = CapsulaEnSlotCapsulas(m, slotB);
        m.swapTiempo = 0.0f;
        m.swapsRestantes--;
        ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
        return;
    }

    m.swapTiempo += deltaTime;

    if (m.swapTiempo >= DuracionIntercambioCapsulas(m.ronda))
    {
        int auxiliar = m.slotDe[m.swapA];
        m.slotDe[m.swapA] = m.slotDe[m.swapB];
        m.slotDe[m.swapB] = auxiliar;
        m.swapA = -1;
        m.swapB = -1;
        m.swapTiempo = 0.0f;
    }
}


static void ActualizarEleccionJugadorCapsulas(
    MinijuegoCapsulasBarajadas& m,
    int indice,
    const Participante& participante,
    float deltaTime
)
{
    EstadoJugadorCapsulas& estado = m.estados[indice];
    bool automatico = participante.esBot || !participante.conectado;
    InputMinijuegoParticipante entrada{};

    if (!automatico)
    {
        entrada = LeerInputMinijuegoParticipante(participante);
    }

    bool izquierdaNueva = entrada.izquierda && !estado.izquierdaPrevia;
    bool derechaNueva = entrada.derecha && !estado.derechaPrevia;
    estado.izquierdaPrevia = entrada.izquierda;
    estado.derechaPrevia = entrada.derecha;

    if (estado.confirmado)
    {
        return;
    }

    if (automatico)
    {
        estado.botPaso -= deltaTime;

        if (estado.marcador != estado.botObjetivo && estado.botPaso <= 0.0f)
        {
            estado.marcador += estado.botObjetivo > estado.marcador ? 1 : -1;
            estado.botPaso = 0.12f;
        }

        if (
            m.tiempoSubfase >= estado.botConfirmar &&
            estado.marcador == estado.botObjetivo
        )
        {
            ConfirmarCapsulas(m, indice);
        }

        return;
    }

    int anterior = estado.marcador;

    if (izquierdaNueva && estado.marcador > 0)
    {
        estado.marcador--;
    }

    if (derechaNueva && estado.marcador < m.cantidadCapsulas - 1)
    {
        estado.marcador++;
    }

    if (estado.marcador != anterior)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
    }

    if (entrada.golpear || entrada.saltar)
    {
        ConfirmarCapsulas(m, indice);
    }
}


//==================================================
// CICLO DE VIDA
//==================================================

void MinijuegoCapsulasBarajadas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estados[i] = {};
        participa[i] = false;
        coloresJugadores[i] = LIGHTGRAY;
    }

    camara.position = { 0.0f, 6.6f, 11.2f };
    camara.target = { 0.0f, 1.0f, 2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CAPSULAS_PREPARACION;
    ronda = 0;
    tiempoPreparacion = DURACION_PREPARACION_CAPSULAS;
    tiempoAnimacion = 0.0f;
    IniciarRondaCapsulas(*this);
}


void MinijuegoCapsulasBarajadas::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    // Inicializar se llama para todos los minijuegos al arrancar. Cargar
    // solo al activar este laboratorio; la cache sobrevive a nuevas rondas.
    CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteCapsulasBarajadasRetro3D());
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    AsignarColoresCapsulas(*this, participantes);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_CAPSULAS_TERMINADO;
        return;
    }

    int limite = LimiteCapsulas(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        Vector3 spawn =
        {
            ((float)k - (float)(cantidad - 1) * 0.5f) * 2.4f,
            ALTURA_PASARELA_CAPSULAS + 0.72f,
            Z_PASARELA_CAPSULAS
        };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        participa[i] = true;
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoCapsulasBarajadas::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_CAPSULAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_CAPSULAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CAPSULAS_JUGANDO;
        }

        return;
    }

    int limite = LimiteCapsulas(cantidadMaxima);
    float tiempoAntes = tiempoSubfase;
    tiempoSubfase += deltaTime;

    if (subfase == SUBFASE_CAPSULAS_MOSTRAR)
    {
        if (tiempoSubfase >= DURACION_MOSTRAR_CAPSULAS)
        {
            subfase = SUBFASE_CAPSULAS_BARAJAR;
            tiempoSubfase = 0.0f;
        }
    }
    else if (subfase == SUBFASE_CAPSULAS_BARAJAR)
    {
        ActualizarBarajadoCapsulas(*this, deltaTime);
    }
    else if (subfase == SUBFASE_CAPSULAS_ELEGIR)
    {
        bool todosConfirmaron = true;

        for (int i = 0; i < limite; i++)
        {
            if (!participa[i])
            {
                continue;
            }

            ActualizarEleccionJugadorCapsulas(*this, i, participantes[i], deltaTime);

            if (!estados[i].confirmado)
            {
                todosConfirmaron = false;
            }
        }

        ActualizarAudioAlertaTiempo(
            audio,
            DURACION_ELEGIR_CAPSULAS - tiempoAntes,
            DURACION_ELEGIR_CAPSULAS - tiempoSubfase,
            3.0f
        );

        if (todosConfirmaron || tiempoSubfase >= DURACION_ELEGIR_CAPSULAS)
        {
            IniciarRevelacionCapsulas(*this);
        }
    }
    else
    {
        revelado = LimitarCapsulas(revelado + deltaTime * 2.0f, 0.0f, 1.0f);

        if (tiempoSubfase >= DURACION_REVELAR_CAPSULAS)
        {
            ronda++;

            if (ronda >= RONDAS_CAPSULAS)
            {
                FinalizarCapsulas(*this, limite);
            }
            else
            {
                IniciarRondaCapsulas(*this);
            }
        }
    }

    // Los jugadores no se mueven: solo celebran un acierto con saltitos.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        float rebote = 0.0f;

        if (subfase == SUBFASE_CAPSULAS_REVELAR && estados[i].acerto)
        {
            rebote = std::fabs(std::sin(tiempoSubfase * 9.0f)) * 0.35f;
        }

        jugadores[i].posicion.y = ALTURA_PASARELA_CAPSULAS + 0.72f + rebote;
    }
}


//==================================================
// VISUAL: LABORATORIO (solo decoracion, la logica no depende de esto)
//==================================================

// Piezas GLB del paquete original con fallback individual. Solo dibujo:
// los slots, tiempos, seleccion y puntuacion no dependen de estas mallas.

static Vector3 PosicionCapsulaCapsulas(const MinijuegoCapsulasBarajadas& m, int c)
{
    float x = XSlotCapsulas(m.slotDe[c], m.cantidadCapsulas);
    float z = 0.0f;

    if (m.swapA >= 0 && (c == m.swapA || c == m.swapB))
    {
        int otra = c == m.swapA ? m.swapB : m.swapA;
        float destino = XSlotCapsulas(m.slotDe[otra], m.cantidadCapsulas);
        float s = SuavizadoCapsulas(m.swapTiempo / DuracionIntercambioCapsulas(m.ronda));
        float arco = std::sin(PI * s) * 1.3f;

        x = x + (destino - x) * s;
        z = c == m.swapA ? arco : -arco;
    }

    return { x, ALTURA_MESA_CAPSULAS, z };
}


static Vector3 PosicionManoCapsulas(const MinijuegoCapsulasBarajadas& m, bool& llevaNucleo)
{
    const Vector3 reposo = { 5.6f, 4.6f, -0.8f };
    llevaNucleo = false;

    if (m.fase != FASE_CAPSULAS_JUGANDO || m.subfase != SUBFASE_CAPSULAS_MOSTRAR)
    {
        return reposo;
    }

    float u = m.tiempoSubfase / DURACION_MOSTRAR_CAPSULAS;
    float xp = XSlotCapsulas(m.premio, m.cantidadCapsulas);
    Vector3 sobre = { xp, 4.0f, 0.0f };
    Vector3 dentro = { xp, 2.9f, 0.0f };

    llevaNucleo = u < 0.6f;

    auto mezclar = [](Vector3 a, Vector3 b, float t)
    {
        return Vector3{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
    };

    if (u < 0.35f) return mezclar(reposo, sobre, SuavizadoCapsulas(u / 0.35f));
    if (u < 0.60f) return mezclar(sobre, dentro, SuavizadoCapsulas((u - 0.35f) / 0.25f));
    if (u < 0.75f) return dentro;
    return mezclar(dentro, reposo, SuavizadoCapsulas((u - 0.75f) / 0.25f));
}


static float TapaAbiertaCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    if (m.fase != FASE_CAPSULAS_JUGANDO || m.subfase != SUBFASE_CAPSULAS_MOSTRAR)
    {
        return 0.0f;
    }

    float u = m.tiempoSubfase / DURACION_MOSTRAR_CAPSULAS;

    if (u < 0.8f) return 1.0f;
    return 1.0f - LimitarCapsulas((u - 0.8f) / 0.2f, 0.0f, 1.0f);
}


static void DibujarSalaCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    // Suelo y marcas.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_SALA, { 0.0f, 0.0f, 0.0f }))
    {
        DrawPlane({ 0.0f, 0.0f, 0.0f }, { 40.0f, 40.0f }, Color{ 188, 196, 204, 255 });

        for (int k = -6; k <= 6; k++)
            DrawCube({ (float)k * 2.0f, 0.005f, 0.0f }, 0.05f, 0.01f, 30.0f, Color{ 150, 158, 168, 255 });
        for (int k = -4; k <= 6; k++)
            DrawCube({ 0.0f, 0.005f, (float)k * 2.0f }, 26.0f, 0.01f, 0.05f, Color{ 150, 158, 168, 255 });
        DrawCube({ 0.0f, 4.0f, -8.4f }, 26.0f, 8.0f, 0.4f, Color{ 66, 86, 98, 255 });
    }

    // Pared del fondo con monitores.
    for (int k = 0; k < 4; k++)
    {
        float x = -7.5f + (float)k * 5.0f;
        // Primitive 2 (menta) contiene solo las seis barras fijas del GLB.
        // Se conserva marco/pantalla; las barras siguen el seno original.
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MONITOR,
            { x, 4.6f, -8.1f }, 0.0f, { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f }, WHITE, 2))
        {
            DrawCube({ x, 4.6f, -8.1f }, 3.4f, 2.0f, 0.15f, Color{ 24, 28, 34, 255 });
            DrawCube({ x, 4.6f, -8.0f }, 3.1f, 1.7f, 0.05f, Color{ 10, 40, 36, 255 });
        }

        for (int b = 0; b < 6; b++)
        {
            float altura = 0.3f + 0.6f * (0.5f + 0.5f * std::sin(m.tiempoAnimacion * 2.0f + (float)(b + k * 3)));
            // Decoracion sobre la pared: no proyecta sombras en el suelo.
            (DrawCube)({ x - 1.2f + (float)b * 0.48f, 4.6f - 0.8f + altura * 0.5f, -7.96f }, 0.3f, altura, 0.04f, Color{ 80, 255, 170, 255 });
        }
    }

    // Mostrador con tubos de ensayo burbujeantes.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MOSTRADOR, { 0.0f, 0.0f, -6.4f }))
        DrawCube({ 0.0f, 0.5f, -6.4f }, 22.0f, 1.0f, 1.4f, Color{ 110, 122, 134, 255 });

    for (int k = 0; k < 8; k++)
    {
        float x = -8.4f + (float)k * 2.4f;
        Color liquido = ColorFromHSV((float)k * 47.0f + 100.0f, 0.7f, 0.95f);

        // Primitive 4 (BOMBILLAS) son burbujas fijas. Omitirlas y conservar
        // las dos que suben con tiempoAnimacion, sin cargas adicionales.
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_TUBO,
            { x, 1.0f, -6.4f }, 0.0f, { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f }, Fade(liquido, 0.85f), 4))
        {
            DrawCylinder({ x, 1.0f, -6.4f }, 0.3f, 0.3f, 1.5f, 10, Fade(WHITE, 0.3f));
            DrawCylinder({ x, 1.0f, -6.4f }, 0.24f, 0.24f, 0.9f, 10, Fade(liquido, 0.85f));
            DrawCylinderWires({ x, 1.0f, -6.4f }, 0.3f, 0.3f, 1.5f, 8, Color{ 220, 235, 245, 255 });
        }

        for (int b = 0; b < 2; b++)
        {
            float fase = std::fmod(m.tiempoAnimacion * 0.6f + (float)b * 0.5f + (float)k * 0.13f, 1.0f);
            DrawSphereEx({ x + (b == 0 ? 0.06f : -0.06f), 1.1f + fase * 0.8f, -6.4f }, 0.07f, 4, 4, Fade(WHITE, 0.8f));
        }
    }

    // Balizas de alerta en las esquinas.
    bool alerta = m.fase == FASE_CAPSULAS_JUGANDO && m.subfase == SUBFASE_CAPSULAS_BARAJAR;
    bool elegir = m.fase == FASE_CAPSULAS_JUGANDO && m.subfase == SUBFASE_CAPSULAS_ELEGIR;
    Color luz = Color{ 70, 70, 80, 255 };

    if (alerta && std::fmod(m.tiempoAnimacion * 4.0f, 1.0f) < 0.5f)
    {
        luz = Color{ 255, 150, 30, 255 };
    }
    else if (elegir)
    {
        luz = Color{ 80, 255, 140, 255 };
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BALIZA,
            { (float)lado * 10.5f, 5.6f, -7.8f }, 0.0f,
            { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, luz))
        {
            DrawCylinder({ (float)lado * 10.5f, 5.6f, -7.8f }, 0.3f, 0.3f, 0.5f, 8, Color{ 50, 54, 62, 255 });
            DrawSphereEx({ (float)lado * 10.5f, 6.3f, -7.8f }, 0.35f, 8, 8, luz);
        }
    }
}


static bool DibujarSegmentoBrazoCapsulas(Vector3 inicio, Vector3 fin)
{
    Vector3 direccion = Vector3Subtract(fin, inicio);
    float longitud = Vector3Length(direccion);
    if (longitud < 0.001f) return false;
    Vector3 unidad = Vector3Scale(direccion, 1.0f / longitud);
    Vector3 eje = Vector3CrossProduct({ 0.0f, 1.0f, 0.0f }, unidad);
    float coseno = LimitarCapsulas(unidad.y, -1.0f, 1.0f);
    float angulo = std::acos(coseno) * RAD2DEG;
    // Incluye los casos paralelo y antiparalelo a +Y.
    if (Vector3Length(eje) < 0.001f) eje = { 1.0f, 0.0f, 0.0f };
    else eje = Vector3Normalize(eje);
    return DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_SEGMENTO,
        inicio, angulo, eje, { 1.0f, longitud, 1.0f });
}


static void DibujarMesaCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    // Mesa de acero.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MESA, { 0.0f, 0.0f, 0.0f }))
    {
        DrawCube({ 0.0f, ALTURA_MESA_CAPSULAS - 0.06f, 0.0f }, 11.4f, 0.12f, 3.6f, Color{ 172, 182, 194, 255 });
        DrawCubeWires({ 0.0f, ALTURA_MESA_CAPSULAS - 0.06f, 0.0f }, 11.4f, 0.12f, 3.6f, Color{ 90, 98, 110, 255 });

        for (int sx = -1; sx <= 1; sx += 2)
        {
            for (int sz = -1; sz <= 1; sz += 2)
            {
                DrawCylinder({ (float)sx * 5.3f, 0.0f, (float)sz * 1.5f }, 0.14f, 0.14f, ALTURA_MESA_CAPSULAS - 0.12f, 8, Color{ 120, 128, 140, 255 });
            }
        }
    }

    // Pasarela del publico con borde de precaucion.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_PASARELA,
        { 0.0f, 0.0f, Z_PASARELA_CAPSULAS }))
    {
        DrawCube({ 0.0f, ALTURA_PASARELA_CAPSULAS * 0.5f, Z_PASARELA_CAPSULAS }, 11.0f, ALTURA_PASARELA_CAPSULAS, 2.6f, Color{ 96, 104, 116, 255 });

        for (int k = 0; k < 22; k++)
        {
            Color franja = (k % 2 == 0) ? Color{ 250, 210, 40, 255 } : Color{ 30, 30, 34, 255 };
            DrawCube({ -5.25f + (float)k * 0.5f, ALTURA_PASARELA_CAPSULAS + 0.005f, Z_PASARELA_CAPSULAS - 1.2f }, 0.5f, 0.012f, 0.2f, franja);
        }
    }

    // Brazo robotico.
    bool llevaNucleo = false;
    Vector3 mano = PosicionManoCapsulas(m, llevaNucleo);
    Vector3 hombro = { 0.0f, 4.2f, -3.4f };
    Vector3 codo =
    {
        (hombro.x + mano.x) * 0.5f,
        (hombro.y + mano.y) * 0.5f + 1.3f,
        (hombro.z + mano.z) * 0.5f - 0.3f
    };

    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_BASE, { 0.0f, 0.0f, -3.4f }))
    {
        DrawCylinder({ 0.0f, 0.0f, -3.4f }, 0.6f, 0.8f, 3.6f, 12, Color{ 80, 90, 104, 255 });
        DrawSphereEx(hombro, 0.45f, 10, 10, Color{ 240, 140, 40, 255 });
    }
    if (!DibujarSegmentoBrazoCapsulas(hombro, codo))
        DrawCylinderEx(hombro, codo, 0.24f, 0.2f, 8, Color{ 240, 140, 40, 255 });
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_ARTICULACION, codo))
        DrawSphereEx(codo, 0.3f, 8, 8, Color{ 60, 66, 78, 255 });
    if (!DibujarSegmentoBrazoCapsulas(codo, mano))
        DrawCylinderEx(codo, mano, 0.2f, 0.13f, 8, Color{ 240, 140, 40, 255 });
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_PINZA, mano))
    {
        DrawCube({ mano.x - 0.2f, mano.y - 0.15f, mano.z }, 0.08f, 0.4f, 0.2f, Color{ 60, 66, 78, 255 });
        DrawCube({ mano.x + 0.2f, mano.y - 0.15f, mano.z }, 0.08f, 0.4f, 0.2f, Color{ 60, 66, 78, 255 });
    }

    // Capsulas.
    float tapa = TapaAbiertaCapsulas(m);
    float elevacion = SuavizadoCapsulas(m.revelado);

    for (int c = 0; c < m.cantidadCapsulas; c++)
    {
        Vector3 p = PosicionCapsulaCapsulas(m, c);
        float y0 = p.y + elevacion * 1.7f;
        Color banda = ColorFromHSV((float)c * 68.0f, 0.75f, 0.95f);

        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_CUERPO, { p.x, y0, p.z }))
        {
            (DrawCylinder)({ p.x, y0, p.z }, 0.8f, 0.8f, 1.5f, 18, Color{ 190, 198, 208, 255 });
            DrawCylinderWires({ p.x, y0, p.z }, 0.8f, 0.8f, 1.5f, 12, Color{ 80, 90, 104, 255 });
        }
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BANDA,
            { p.x, y0, p.z }, 0.0f, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, banda))
            (DrawCylinder)({ p.x, y0 + 0.45f, p.z }, 0.83f, 0.83f, 0.28f, 18, banda);
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_TAPA,
            { p.x, y0 + 1.5f + tapa * 1.3f, p.z }, 0.0f,
            { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, banda))
        {
            (DrawCylinder)({ p.x, y0 + 1.5f + tapa * 1.3f, p.z }, 0.62f, 0.82f, 0.22f, 18, Color{ 150, 160, 174, 255 });
            DrawSphereEx({ p.x, y0 + 1.75f + tapa * 1.3f, p.z }, 0.14f, 6, 6, banda);
        }
        // Una sombra sobre la mesa, tambien en fallback. No proyectar las
        // tres piezas por separado sobre el suelo debajo de la mesa.
        DibujarSombraRetroCircular({ p.x, y0, p.z }, 0.76f, 0.76f, 1.06f);
    }

    // Nucleo brillante: en la mano, o sobre la mesa bajo la capsula premiada.
    Vector3 nucleo;

    if (llevaNucleo)
    {
        nucleo = { mano.x, mano.y - 0.5f, mano.z };
    }
    else
    {
        Vector3 p = PosicionCapsulaCapsulas(m, m.premio);
        nucleo = { p.x, p.y + 0.4f, p.z };
    }

    float pulso = 1.0f + 0.12f * std::sin(m.tiempoAnimacion * 8.0f);

    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_NUCLEO,
        nucleo, 0.0f, { 0.0f, 1.0f, 0.0f }, { pulso, pulso, pulso }))
    {
        DrawSphereEx(nucleo, 0.32f * pulso, 10, 10, Color{ 120, 255, 170, 255 });
        DrawSphereWires(nucleo, 0.46f * pulso, 6, 6, Fade(Color{ 160, 255, 210, 255 }, 0.8f));
    }

    if (m.subfase == SUBFASE_CAPSULAS_MOSTRAR && m.fase == FASE_CAPSULAS_JUGANDO)
    {
        float u = m.tiempoSubfase / DURACION_MOSTRAR_CAPSULAS;

        if (u >= 0.6f && u < 0.8f)
        {
            Vector3 p = PosicionCapsulaCapsulas(m, m.premio);
            DrawSphereEx({ p.x, p.y + 1.6f, p.z }, 0.6f * (1.0f - (u - 0.6f) / 0.2f), 8, 8, Fade(Color{ 160, 255, 210, 255 }, 0.6f));
        }
    }
}


static void DibujarMarcadoresCapsulas(
    const MinijuegoCapsulasBarajadas& m,
    const JugadorPrueba jugadores[],
    int limite
)
{
    if (
        m.fase != FASE_CAPSULAS_JUGANDO ||
        (m.subfase != SUBFASE_CAPSULAS_ELEGIR && m.subfase != SUBFASE_CAPSULAS_REVELAR)
    )
    {
        return;
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        const EstadoJugadorCapsulas& estado = m.estados[i];
        int slot = m.subfase == SUBFASE_CAPSULAS_REVELAR ? estado.slotElegido : estado.marcador;

        if (slot < 0)
        {
            continue;
        }

        int total = 0;
        int k = IndiceVisualCapsulas(m, i, total);
        float x = XSlotCapsulas(slot, m.cantidadCapsulas) + ((float)k - (float)(total - 1) * 0.5f) * 0.42f;
        float y = 3.5f + 0.08f * std::sin(m.tiempoAnimacion * 4.0f + (float)k);
        Color color = m.coloresJugadores[i];

        if (m.subfase == SUBFASE_CAPSULAS_REVELAR)
        {
            color = estado.acerto ? Color{ 90, 255, 130, 255 } : Color{ 255, 80, 80, 255 };
        }

        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MARCADOR,
            { x, y, Z_MARCADOR_CAPSULAS }, 0.0f, { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f }, color))
        {
            (DrawCylinder)({ x, y, Z_MARCADOR_CAPSULAS }, 0.24f, 0.0f, 0.55f, 10, color);
            DrawSphereEx({ x, y + 0.72f, Z_MARCADOR_CAPSULAS }, 0.2f, 8, 8, m.coloresJugadores[i]);
        }

        if (estado.confirmado)
        {
            DrawSphereWires({ x, y + 0.72f, Z_MARCADOR_CAPSULAS }, 0.32f, 6, 6, WHITE);
        }

        DrawLine3D(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 0.8f, jugadores[i].posicion.z },
            { x, y + 0.72f, Z_MARCADOR_CAPSULAS },
            Fade(m.coloresJugadores[i], estado.confirmado ? 0.9f : 0.45f)
        );
    }
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoCapsulasBarajadas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteCapsulas(cantidadMaxima);
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    ClearBackground(Color{ 30, 44, 56, 255 });
    BeginMode3D(camara);

    DibujarSalaCapsulas(*this);
    DibujarMesaCapsulas(*this);

    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = coloresJugadores[i];

        DrawCircle3D(
            { jugadores[i].posicion.x, ALTURA_PASARELA_CAPSULAS + 0.02f, jugadores[i].posicion.z },
            0.62f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            coloresJugadores[i]
        );
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarMarcadoresCapsulas(*this, jugadores, limite);

    if (mostrarDebug)
    {
        for (int c = 0; c < cantidadCapsulas; c++)
        {
            Vector3 p = PosicionCapsulaCapsulas(*this, c);
            DrawCubeWires({ p.x, p.y + 0.75f, p.z }, 1.6f, 1.5f, 1.6f, YELLOW);
        }
    }

    EndMode3D();

    // Etiquetas sobre los jugadores.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.35f, jugadores[i].posicion.z },
            camara
        );
        const char* nombre = NombreJugadorCapsulas(participantes[i], i);
        int ancho = MeasureText(nombre, 18);

        DrawRectangle((int)pantalla.x - ancho / 2 - 4, (int)pantalla.y - 2, ancho + 8, 22, Fade(BLACK, 0.55f));
        DrawText(nombre, (int)pantalla.x - ancho / 2, (int)pantalla.y, 18, coloresJugadores[i]);

        if (
            fase == FASE_CAPSULAS_JUGANDO &&
            subfase == SUBFASE_CAPSULAS_REVELAR
        )
        {
            const char* texto = estados[i].acerto
                ? TextFormat("+%d", estados[i].puntosRonda)
                : "X";
            DrawText(
                texto,
                (int)pantalla.x - MeasureText(texto, 30) / 2,
                (int)pantalla.y - 34,
                30,
                estados[i].acerto ? LIME : RED
            );
        }
    }

    // Cabecera.
    DrawRectangle(14, 12, 480, 66, Fade(BLACK, 0.74f));
    DrawText("CAPSULAS BARAJADAS", 28, 18, 28, GOLD);
    DrawText(
        TextFormat("RONDA %d/%d    CAPSULAS: %d", ronda < RONDAS_CAPSULAS ? ronda + 1 : RONDAS_CAPSULAS, RONDAS_CAPSULAS, cantidadCapsulas),
        28,
        50,
        18,
        RAYWHITE
    );

    if (fase == FASE_CAPSULAS_JUGANDO)
    {
        const char* aviso = "OBSERVA DONDE VA EL NUCLEO";
        Color colorAviso = RAYWHITE;

        if (subfase == SUBFASE_CAPSULAS_BARAJAR)
        {
            aviso = "BARAJANDO... SIGUE LA CAPSULA";
            colorAviso = ORANGE;
        }
        else if (subfase == SUBFASE_CAPSULAS_ELEGIR)
        {
            float restante = DURACION_ELEGIR_CAPSULAS - tiempoSubfase;
            aviso = TextFormat("ELIGE!  %.1f", restante > 0.0f ? restante : 0.0f);
            colorAviso = LIME;

            DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
            DrawText(
                TextFormat("TIEMPO %.1f", restante > 0.0f ? restante : 0.0f),
                anchoPantalla - 200,
                22,
                26,
                restante <= 1.0f ? RED : GOLD
            );
        }
        else if (subfase == SUBFASE_CAPSULAS_REVELAR)
        {
            aviso = "RESULTADO";
            colorAviso = GOLD;
        }

        int a = MeasureText(aviso, 26);
        DrawRectangle(anchoPantalla / 2 - a / 2 - 14, 12, a + 28, 40, Fade(BLACK, 0.8f));
        DrawText(aviso, anchoPantalla / 2 - a / 2, 20, 26, colorAviso);

        if (subfase == SUBFASE_CAPSULAS_ELEGIR)
        {
            bool doble = tiempoSubfase <= VENTANA_APUESTA_DOBLE_CAPSULAS;
            const char* ayuda = doble
                ? "APUESTA DOBLE: CONFIRMA YA Y VALE 2 PUNTOS!"
                : "IZQ/DER: MOVER MARCADOR   CONFIRMAR: BOTON PRINCIPAL";
            int b = MeasureText(ayuda, 20);
            DrawRectangle(anchoPantalla / 2 - b / 2 - 10, 56, b + 20, 28, Fade(BLACK, 0.7f));
            DrawText(ayuda, anchoPantalla / 2 - b / 2, 60, 20, doble ? YELLOW : LIGHTGRAY);
        }
    }

    // Tarjetas inferiores.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participa[i]) activos++;
    }

    if (activos > 0)
    {
        int separacion = 10;
        int anchoTarjeta = (anchoPantalla - 28 - separacion * (activos - 1)) / activos;

        if (anchoTarjeta > 300) anchoTarjeta = 300;

        int xInicial = (anchoPantalla - (anchoTarjeta * activos + separacion * (activos - 1))) / 2;
        int k = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!participa[i])
            {
                continue;
            }

            int x = xInicial + k * (anchoTarjeta + separacion);
            int y = altoPantalla - 70;

            DrawRectangle(x, y, anchoTarjeta, 60, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 60, coloresJugadores[i]);
            DrawText(NombreJugadorCapsulas(participantes[i], i), x + 18, y + 6, 18, coloresJugadores[i]);
            DrawText(TextFormat("%d PTS", estados[i].puntos), x + anchoTarjeta - 90, y + 6, 20, RAYWHITE);

            const char* estado = "ESPERANDO";
            Color colorEstado = LIGHTGRAY;

            if (fase == FASE_CAPSULAS_JUGANDO && subfase == SUBFASE_CAPSULAS_ELEGIR)
            {
                estado = estados[i].confirmado ? "CONFIRMADO" : "ELIGIENDO";
                colorEstado = estados[i].confirmado ? LIME : YELLOW;
            }

            if (participantes[i].esBot)
            {
                DrawText(TextFormat("%s  (BOT)", estado), x + 18, y + 34, 16, colorEstado);
            }
            else
            {
                DrawText(
                    TextFormat("%s  OK: %s", estado, ObtenerTextoBotonPrincipal(participantes[i])),
                    x + 18,
                    y + 34,
                    16,
                    colorEstado
                );
            }

            k++;
        }
    }

    if (fase == FASE_CAPSULAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 150, 96, GOLD);

        const char* ayuda = "SIGUE LA CAPSULA CON EL NUCLEO. IZQ/DER MUEVE, BOTON PRINCIPAL CONFIRMA";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 - 40, 20, RAYWHITE);
    }
    else if (
        fase == FASE_CAPSULAS_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 40;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorCapsulas(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (!participa[i] || resultado.participantes[i].posicionFinal != posicion)
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d pts  (%d aciertos, %.1f s)",
                        posicion,
                        NombreJugadorCapsulas(participantes[i], i),
                        estados[i].puntos,
                        estados[i].aciertos,
                        estados[i].tiempoAciertos
                    ),
                    px + 30,
                    py + 62 + fila * 30,
                    20,
                    coloresJugadores[i]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            anchoPantalla / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoCapsulasBarajadas::ObtenerResultado() const
{
    return resultado;
}
