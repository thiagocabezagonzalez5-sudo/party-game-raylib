#include "Minigames/MinijuegoInterruptoresCaos.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_INTERRUPTORES = 2.5f;
static const float DURACION_TURNO_INTERRUPTORES = 10.0f;
static const float DURACION_SEGURO_INTERRUPTORES = 0.8f;
static const float DURACION_EXPLOSION_INTERRUPTORES = 1.45f;


static int ContarVivosInterruptores(
    const MinijuegoInterruptoresCaos& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int BuscarSiguienteVivoInterruptores(
    const MinijuegoInterruptoresCaos& minijuego,
    int origen
)
{
    int indice = origen;

    for (int intento = 0; intento < MAX_PARTICIPANTES; intento++)
    {
        indice++;
        if (indice >= MAX_PARTICIPANTES) indice = 0;

        if (
            minijuego.resultado.participantes[indice].participo &&
            !minijuego.jugadores[indice].eliminado
        )
        {
            return indice;
        }
    }

    return origen;
}


static int ElegirInterruptorDisponible(
    const MinijuegoInterruptoresCaos& minijuego
)
{
    int disponibles[MAX_INTERRUPTORES_CAOS]{};
    int cantidad = 0;

    for (int i = 0; i < minijuego.cantidadInterruptores; i++)
    {
        if (!minijuego.interruptoresUsados[i])
        {
            disponibles[cantidad] = i;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return 0;
    }

    return disponibles[GetRandomValue(0, cantidad - 1)];
}


static int MoverSeleccionInterruptor(
    const MinijuegoInterruptoresCaos& minijuego,
    int origen,
    int direccion
)
{
    int indice = origen;

    for (int intento = 0; intento < minijuego.cantidadInterruptores; intento++)
    {
        indice += direccion;

        if (indice < 0) indice = minijuego.cantidadInterruptores - 1;
        if (indice >= minijuego.cantidadInterruptores) indice = 0;

        if (!minijuego.interruptoresUsados[indice])
        {
            return indice;
        }
    }

    return origen;
}


static void PrepararRondaInterruptores(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int eliminados =
        minijuego.resultado.cantidadParticipantes -
        ContarVivosInterruptores(minijuego);

    minijuego.cantidadInterruptores = MAX_INTERRUPTORES_CAOS - eliminados;
    if (minijuego.cantidadInterruptores < 3)
    {
        minijuego.cantidadInterruptores = 3;
    }

    for (int i = 0; i < MAX_INTERRUPTORES_CAOS; i++)
    {
        minijuego.interruptoresUsados[i] = false;
    }

    minijuego.interruptorPeligroso = GetRandomValue(
        0,
        minijuego.cantidadInterruptores - 1
    );
}


// Nivel previo (izquierda, derecha) del mando del jugador en turno, para
// mover la seleccion con flanco. Hay un solo minijuego activo a la vez.
static bool direccionPreviaMandoInterruptores[2] = {};


static void PrepararTurnoInterruptores(
    MinijuegoInterruptoresCaos& minijuego,
    int jugador
)
{
    // Una direccion que venia mantenida del turno anterior no cuenta:
    // hay que soltarla para mover.
    direccionPreviaMandoInterruptores[0] = true;
    direccionPreviaMandoInterruptores[1] = true;

    minijuego.jugadorTurno = jugador;
    minijuego.jugadorExplosion = -1;
    minijuego.numeroTurno++;
    minijuego.tiempoTurno = DURACION_TURNO_INTERRUPTORES;
    minijuego.interruptorSeleccionado =
        ElegirInterruptorDisponible(minijuego);
    minijuego.fase = FASE_INTERRUPTORES_ELECCION;

    if (jugador >= 0)
    {
        minijuego.jugadores[jugador].tiempoDecisionBot =
            GetRandomValue(75, 190) / 100.0f;
    }
}


static void FinalizarInterruptores(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int ganador = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            ganador = i;
            minijuego.jugadores[i].posicionFinal = 1;
            break;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        resultadoJugador.posicionFinal =
            minijuego.jugadores[i].posicionFinal;

        resultadoJugador.puntuacionMinijuego =
            (minijuego.resultado.cantidadParticipantes + 1 -
                resultadoJugador.posicionFinal) * 50 +
            minijuego.jugadores[i].turnosSuperados * 10;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        ganador >= 0 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_INTERRUPTORES_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.jugadorTurno = ganador;
}


static void ActivarInterruptor(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int indice = minijuego.interruptorSeleccionado;

    if (
        indice < 0 ||
        indice >= minijuego.cantidadInterruptores ||
        minijuego.interruptoresUsados[indice]
    )
    {
        return;
    }

    if (indice == minijuego.interruptorPeligroso)
    {
        int vivosAntes = ContarVivosInterruptores(minijuego);
        int eliminado = minijuego.jugadorTurno;

        minijuego.jugadores[eliminado].eliminado = true;
        minijuego.jugadores[eliminado].posicionFinal = vivosAntes;
        minijuego.jugadorExplosion = eliminado;
        minijuego.jugadorTurno = -1;
        minijuego.fase = FASE_INTERRUPTORES_EXPLOSION;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_EXPLOSION);
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
        minijuego.tiempoResolucion = DURACION_EXPLOSION_INTERRUPTORES;
        return;
    }

    minijuego.interruptoresUsados[indice] = true;
    minijuego.jugadores[minijuego.jugadorTurno].turnosSuperados++;
    minijuego.fase = FASE_INTERRUPTORES_SEGURO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
    minijuego.tiempoResolucion = DURACION_SEGURO_INTERRUPTORES;
}


// Efectos de la sala de control (definidos con el dibujo, mas abajo).
static void ReiniciarVisualInterruptores(MinijuegoInterruptoresCaos& minijuego);
static void ActualizarVisualInterruptores(
    MinijuegoInterruptoresCaos& minijuego,
    const Participante participantes[],
    float deltaTime
);


void MinijuegoInterruptoresCaos::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_INTERRUPTORES_CAOS; i++)
    {
        interruptoresUsados[i] = false;
    }

    cantidadInterruptores = MAX_INTERRUPTORES_CAOS;
    interruptorPeligroso = 0;
    interruptorSeleccionado = 0;
    jugadorTurno = -1;
    jugadorExplosion = -1;
    numeroTurno = 0;
    fase = FASE_INTERRUPTORES_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_INTERRUPTORES;
    tiempoTurno = DURACION_TURNO_INTERRUPTORES;
    tiempoResolucion = 0.0f;
    tiempoAnimacion = 0.0f;

    ReiniciarVisualInterruptores(*this);
}


void MinijuegoInterruptoresCaos::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    PrepararRondaInterruptores(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            jugadorTurno = i;
            break;
        }
    }
}


void MinijuegoInterruptoresCaos::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    ActualizarVisualInterruptores(*this, participantes, deltaTime);

    if (fase == FASE_INTERRUPTORES_TERMINADO)
    {
        return;
    }

    if (fase == FASE_INTERRUPTORES_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            PrepararTurnoInterruptores(*this, jugadorTurno);
        }

        return;
    }

    if (fase == FASE_INTERRUPTORES_SEGURO)
    {
        tiempoResolucion -= deltaTime;

        if (tiempoResolucion <= 0.0f)
        {
            PrepararTurnoInterruptores(
                *this,
                BuscarSiguienteVivoInterruptores(*this, jugadorTurno)
            );
        }

        return;
    }

    if (fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        tiempoResolucion -= deltaTime;

        if (tiempoResolucion <= 0.0f)
        {
            if (ContarVivosInterruptores(*this) <= 1)
            {
                FinalizarInterruptores(*this);
            }
            else
            {
                int siguiente = BuscarSiguienteVivoInterruptores(
                    *this,
                    jugadorExplosion
                );
                PrepararRondaInterruptores(*this);
                PrepararTurnoInterruptores(*this, siguiente);
            }
        }

        return;
    }

    float turnoAntes = tiempoTurno;
    tiempoTurno -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, turnoAntes, tiempoTurno, 3.0f);

    if (jugadorTurno < 0)
    {
        return;
    }

    bool confirmar = tiempoTurno <= 0.0f;

    if (participantes[jugadorTurno].esBot || !participantes[jugadorTurno].conectado)
    {
        jugadores[jugadorTurno].tiempoDecisionBot -= deltaTime;

        if (jugadores[jugadorTurno].tiempoDecisionBot <= 0.0f)
        {
            interruptorSeleccionado = ElegirInterruptorDisponible(*this);
            confirmar = true;
        }
    }
    else
    {
        int seleccionAntes = interruptorSeleccionado;
        const Participante& participante = participantes[jugadorTurno];

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participante);

        bool moverIzquierda = false;
        bool moverDerecha = false;

        if (participante.control == CONTROL_GAMEPAD)
        {
            // Mando: cruceta o stick, con flanco para no mover cada frame.
            // X es la accion (activar) y B no se usa, asi elegir y activar
            // no comparten boton.
            moverIzquierda = entrada.izquierda && !direccionPreviaMandoInterruptores[0];
            moverDerecha = entrada.derecha && !direccionPreviaMandoInterruptores[1];
            direccionPreviaMandoInterruptores[0] = entrada.izquierda;
            direccionPreviaMandoInterruptores[1] = entrada.derecha;
        }
        else
        {
            moverIzquierda = AccionDireccionalControlPresionada(
                participante,
                CONTROL_DIRECCION_IZQUIERDA
            );
            moverDerecha = AccionDireccionalControlPresionada(
                participante,
                CONTROL_DIRECCION_DERECHA
            );
        }

        if (moverIzquierda)
        {
            interruptorSeleccionado = MoverSeleccionInterruptor(
                *this,
                interruptorSeleccionado,
                -1
            );
        }

        if (moverDerecha)
        {
            interruptorSeleccionado = MoverSeleccionInterruptor(
                *this,
                interruptorSeleccionado,
                1
            );
        }

        if (seleccionAntes != interruptorSeleccionado)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
        }

        confirmar = confirmar || entrada.golpear;
    }

    if (confirmar)
    {
        ActivarInterruptor(*this);
    }
}



//==================================================
// SALA DE CONTROL DE LA CENTRAL (solo visual)
//==================================================
//
// Noche en la central electrica. La sala de control es un balcon
// vidriado sobre la nave de turbinas; al fondo de la nave, por su muro
// de ventanales, se ven las torres de enfriamiento con su vapor, el
// tendido de alta tension y la sierra bajo la luna.
//
// Capas: piso tecnico con las tarimas de los operadores (arena); la
// consola de palancas, racks, escritorios, sillas, bobinas de cable,
// armarios y balizas (anillo cercano); el muro de monitores con el
// medidor de sobrecarga, el aislador colgante y el ventanal (media
// distancia); la nave de turbinas y el exterior (horizonte); el cielo
// nocturno con luna y estrellas, y el cielorraso con sus tubos.
//
// Paleta: gris azulado del piso, verde menta gastado de las consolas,
// crema de los tableros, grafito, verde petroleo de las turbinas y el
// naranja sodio de la nave. Acento: el ambar de la palanca elegida; el
// rojo queda reservado para la sobrecarga.
//
// MODELO FUTURO: consola de palancas con sus tableros de instrumentos,
// muro de monitores con el medidor, aislador colgante, turbogenerador,
// puente grua, rack electrico, escritorio con monitor, silla de
// operador, bobina de cable, armario de cortes, baliza giratoria, torre
// de enfriamiento y torre de alta tension.
//==================================================

static const float Z_PASARELA_INTERRUPTORES = 4.6f;
static const float ALTURA_PASARELA_INTERRUPTORES = 0.6f;
static const float Z_PALANCAS_INTERRUPTORES = -3.2f;
static const float ALTURA_BASE_PALANCA = 1.45f;
static const float LARGO_MANGO_CENTRAL = 1.75f;
static const float Z_TABLEROS_CENTRAL = -4.75f;     // frente de los tableros de cada palanca
static const float Z_MONITORES_CENTRAL = -7.4f;     // cara del muro de monitores
static const float Z_VENTANAL_CENTRAL = -10.0f;     // cara interior del ventanal
static const float X_MURO_CENTRAL = 19.0f;          // muros laterales de la sala
static const float Z_FRENTE_CENTRAL = 13.0f;
static const float Y_DINTEL_CENTRAL = 10.2f;
static const float Y_TECHO_CENTRAL = 11.6f;
static const float Y_NAVE_CENTRAL = -4.5f;          // piso de la nave de turbinas
static const float Z_FONDO_NAVE_CENTRAL = -54.0f;   // muro de ventanales de la nave
static const float X_NAVE_CENTRAL = 34.0f;
static const float Y_EXTERIOR_CENTRAL = -6.0f;
static const Vector3 AISLADOR_CENTRAL = { 0.0f, 8.55f, -6.2f };   // de aca salta el arco
static const Vector3 EMPALME_CENTRAL = { -12.9f, 2.62f, -8.2f };  // empalme flojo del armario de cortes
static const float DURACION_ANUNCIO_INTERRUPTORES = 1.4f;
static const float DURACION_VUELO_INTERRUPTORES = 0.9f;
static const float DURACION_SALTO_INTERRUPTORES = 0.45f;


// Camara fija: la fila de tarimas abajo, la consola al medio y el muro
// de monitores con el ventanal arriba.
static Camera3D CamaraCentral()
{
    Camera3D camara{};
    camara.position = { 0.0f, 11.0f, 19.0f };
    camara.target = { 0.0f, 2.8f, -2.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 44.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


static float Limitar01Central(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float SuaveCentral(float u)
{
    u = Limitar01Central(u);
    return u * u * (3.0f - 2.0f * u);
}


// Hash determinista para la decoracion (la misma sala en cada partida).
static float Hash01Central(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarCentral(MinijuegoInterruptoresCaos& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasCentral(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static bool EsHumanoCentral(const Participante& participante)
{
    return !(participante.esBot || !participante.conectado);
}


// Tarima del participante (segun su orden entre los activos).
static Vector3 PosicionPasarelaInterruptores(
    const MinijuegoInterruptoresCaos& minijuego,
    int indice
)
{
    int orden = 0;
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.resultado.participantes[i].participo)
        {
            if (i < indice) orden++;
            cantidad++;
        }
    }

    if (cantidad < 1) cantidad = 1;

    return Vector3
    {
        (orden - (cantidad - 1) * 0.5f) * 4.2f,
        ALTURA_PASARELA_INTERRUPTORES,
        Z_PASARELA_INTERRUPTORES
    };
}


static float PosicionXPalanca(const MinijuegoInterruptoresCaos& minijuego, int indice)
{
    return (indice - (minijuego.cantidadInterruptores - 1) * 0.5f) * 3.1f;
}


// Carga de la red: sube con cada palanca segura; 1 en la sobrecarga.
static float TensionCentral(const MinijuegoInterruptoresCaos& m)
{
    if (m.fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        return 1.0f;
    }

    int usados = 0;

    for (int i = 0; i < m.cantidadInterruptores; i++)
    {
        if (m.interruptoresUsados[i]) usados++;
    }

    return m.cantidadInterruptores > 1
        ? Limitar01Central((float)usados / (float)(m.cantidadInterruptores - 1))
        : 1.0f;
}


// Angulo del mango en grados: 0 arriba, 55 jalado hacia el operador.
static float AnguloPalancaCentral(const MinijuegoInterruptoresCaos& m, int i)
{
    bool actual =
        i == m.interruptorSeleccionado &&
        (m.fase == FASE_INTERRUPTORES_SEGURO || m.fase == FASE_INTERRUPTORES_EXPLOSION);

    if (actual)
    {
        float duracion = m.fase == FASE_INTERRUPTORES_SEGURO
            ? DURACION_SEGURO_INTERRUPTORES
            : DURACION_EXPLOSION_INTERRUPTORES;
        float progreso = 1.0f - Limitar01Central(m.tiempoResolucion / duracion);
        return 55.0f * SuaveCentral(progreso * 4.0f);
    }

    if (m.interruptoresUsados[i])
    {
        return 55.0f;
    }

    if (m.fase == FASE_INTERRUPTORES_ELECCION && i == m.interruptorSeleccionado)
    {
        return 6.0f + std::sin(m.tiempoAnimacion * 9.0f) * 3.0f + m.pulsoSeleccion * 26.0f;
    }

    return 0.0f;
}


static Vector3 PivotePalancaCentral(const MinijuegoInterruptoresCaos& m, int i)
{
    return { PosicionXPalanca(m, i), ALTURA_BASE_PALANCA + 0.2f, Z_PALANCAS_INTERRUPTORES };
}


// Punta del mango (la perilla), girando hacia el operador.
static Vector3 PuntaPalancaCentral(const MinijuegoInterruptoresCaos& m, int i, float largo = LARGO_MANGO_CENTRAL)
{
    float a = AnguloPalancaCentral(m, i) * DEG2RAD;
    Vector3 p = PivotePalancaCentral(m, i);
    return { p.x, p.y + std::cos(a) * largo, p.z + std::sin(a) * largo };
}


// Aparicion en ola con rebote (al empezar y en cada ronda nueva).
static float EscalaAparicionCentral(const MinijuegoInterruptoresCaos& m, int i)
{
    float u = Limitar01Central((m.aparicion - 0.25f - 0.12f * (float)i) / 0.5f);

    if (u <= 0.0f)
    {
        return 0.0f;
    }

    float v = u - 1.0f;
    return 1.0f + 2.70158f * v * v * v + 1.70158f * v * v;
}


// Pose del operador: pies, giro en Y (0 = mira a la camara) y caida
// hacia atras en grados (90 = tendido boca arriba).
struct PoseOperadorCentral
{
    Vector3 pies{};
    float giro = 180.0f;
    float caida = 0.0f;
};


static PoseOperadorCentral PoseOperadorInterruptores(const MinijuegoInterruptoresCaos& m, int i)
{
    const EstadoJugadorInterruptoresCaos& j = m.jugadores[i];
    PoseOperadorCentral pose;
    pose.pies = PosicionPasarelaInterruptores(m, i);

    if (j.descarga > 0.0f || j.eliminado)
    {
        // La descarga lo levanta, gira en el aire y cae tendido en su tarima.
        float u = j.descarga > 0.0f ? Limitar01Central(j.descarga / DURACION_VUELO_INTERRUPTORES) : 1.0f;
        float s = SuaveCentral(u);
        pose.pies.y += std::sin(u * PI) * 1.9f;
        pose.pies.z -= 0.75f * s;
        pose.pies.y += 0.3f * s;          // tendido sobre la espalda, no dentro de la chapa
        pose.giro = 180.0f + 360.0f * s;
        pose.caida = 90.0f * s;
        return pose;
    }

    if (j.salto > 0.0f)
    {
        pose.pies.y += std::sin((1.0f - j.salto / DURACION_SALTO_INTERRUPTORES) * PI) * 0.55f;
    }

    if (m.fase == FASE_INTERRUPTORES_ELECCION && i == m.jugadorTurno)
    {
        float dx = PosicionXPalanca(m, m.interruptorSeleccionado) - pose.pies.x;
        float dz = Z_PALANCAS_INTERRUPTORES - pose.pies.z;
        pose.giro = std::atan2(dx, dz) * RAD2DEG;
    }

    if (m.fase == FASE_INTERRUPTORES_TERMINADO)
    {
        // El que queda en pie festeja de frente.
        pose.giro = 0.0f;
        pose.pies.y += std::fabs(std::sin(m.tiempoAnimacion * 5.0f)) * 0.4f;
    }

    return pose;
}


static Vector3 PechoOperadorCentral(const MinijuegoInterruptoresCaos& m, int i)
{
    PoseOperadorCentral pose = PoseOperadorInterruptores(m, i);
    float c = std::cos(pose.caida * DEG2RAD);
    float s = std::sin(pose.caida * DEG2RAD);
    return { pose.pies.x, pose.pies.y + 1.05f * c + 0.2f, pose.pies.z + 1.05f * s };
}


//==================================================
// EFECTOS: CHISPAS, HUMO, CONFETI Y ANUNCIOS
//==================================================

static void EmitirParticulaCentral(
    MinijuegoInterruptoresCaos& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color,
    bool brillo,
    bool humo
)
{
    for (int i = 0; i < MAX_PARTICULAS_INTERRUPTORES; i++)
    {
        ParticulaInterruptoresCaos& p = minijuego.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.humo = humo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.color = color;
        return;
    }
}


// Chispas que saltan y caen (pasada aditiva).
static void ChispasCentral(MinijuegoInterruptoresCaos& m, Vector3 punto, Color color, int base, float rapidez)
{
    for (int k = 0; k < CantidadParticulasCentral(base); k++)
    {
        float angulo = AzarCentral(m, 0.0f, 6.2832f);
        float abierto = AzarCentral(m, 0.3f, 1.0f) * rapidez;
        bool blanca = k % 3 == 0;

        EmitirParticulaCentral(
            m,
            punto,
            { std::cos(angulo) * abierto, AzarCentral(m, 0.6f, 1.6f) * rapidez, std::sin(angulo) * abierto },
            AzarCentral(m, 0.35f, 0.8f),
            AzarCentral(m, 0.05f, 0.09f),
            9.0f,
            blanca ? Color{ 255, 250, 230, 255 } : color,
            true,
            false
        );
    }
}


static void HumoCentral(MinijuegoInterruptoresCaos& m, Vector3 punto, int base, float tamano)
{
    for (int k = 0; k < CantidadParticulasCentral(base); k++)
    {
        EmitirParticulaCentral(
            m,
            { punto.x + AzarCentral(m, -0.25f, 0.25f), punto.y, punto.z + AzarCentral(m, -0.25f, 0.25f) },
            { AzarCentral(m, -0.25f, 0.25f), AzarCentral(m, 0.5f, 1.1f), AzarCentral(m, -0.15f, 0.25f) },
            AzarCentral(m, 1.4f, 2.4f),
            tamano * AzarCentral(m, 0.8f, 1.3f),
            0.0f,
            Color{ 70, 72, 80, 255 },
            false,
            true
        );
    }
}


static void ConfetiCentral(MinijuegoInterruptoresCaos& m, Vector3 origen, Color color, int base)
{
    const Color colores[4] =
    {
        color,
        Color{ 255, 190, 60, 255 },
        Color{ 240, 244, 250, 255 },
        Color{ 96, 226, 214, 255 }
    };

    for (int k = 0; k < CantidadParticulasCentral(base); k++)
    {
        float angulo = AzarCentral(m, 0.0f, 6.2832f);
        float rapidez = AzarCentral(m, 0.6f, 2.4f);

        EmitirParticulaCentral(
            m,
            origen,
            { std::cos(angulo) * rapidez, AzarCentral(m, 2.6f, 5.0f), std::sin(angulo) * rapidez * 0.6f + 0.5f },
            AzarCentral(m, 1.8f, 2.6f),
            AzarCentral(m, 0.08f, 0.13f),
            3.6f,
            colores[k % 4],
            false,
            false
        );
    }
}


static void AnunciarCentral(MinijuegoInterruptoresCaos& m, AnuncioInterruptoresCaos tipo, int jugador, int palanca)
{
    m.anuncio = tipo;
    m.tiempoAnuncio = DURACION_ANUNCIO_INTERRUPTORES;
    m.jugadorAnuncio = jugador;
    m.palancaAnuncio = palanca;
}


static void ReiniciarVisualInterruptores(MinijuegoInterruptoresCaos& m)
{
    m.semillaVisual = 2654435761u;
    m.faseVista = FASE_INTERRUPTORES_PREPARACION;
    m.seleccionVista = -1;
    m.pulsoSeleccion = 0.0f;
    m.tensionVista = 0.0f;
    m.sobrecarga = 0.0f;
    m.aparicion = 0.0f;
    m.tiempoFin = 0.0f;
    m.anuncio = ANUNCIO_INTERRUPTORES_NINGUNO;
    m.tiempoAnuncio = 0.0f;
    m.jugadorAnuncio = -1;
    m.palancaAnuncio = -1;
    m.proximaChispa = 2.5f;

    for (int k = 0; k < MAX_INTERRUPTORES_CAOS; k++)
    {
        m.destelloPalanca[k] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS_INTERRUPTORES; i++)
    {
        m.particulas[i] = {};
    }
}


static float DescontarCentral(float valor, float dt)
{
    return valor > dt ? valor - dt : 0.0f;
}


static void ActualizarVisualInterruptores(
    MinijuegoInterruptoresCaos& minijuego,
    const Participante participantes[],
    float deltaTime
)
{
    MinijuegoInterruptoresCaos& m = minijuego;
    float dt = deltaTime;

    m.aparicion += dt;
    m.pulsoSeleccion = DescontarCentral(m.pulsoSeleccion, dt);
    m.tiempoAnuncio = DescontarCentral(m.tiempoAnuncio, dt);
    m.sobrecarga = DescontarCentral(m.sobrecarga, dt / 1.6f);

    for (int k = 0; k < MAX_INTERRUPTORES_CAOS; k++)
    {
        m.destelloPalanca[k] = DescontarCentral(m.destelloPalanca[k], dt * 1.4f);
    }

    m.tensionVista += (TensionCentral(m) - m.tensionVista) * Limitar01Central(dt * 3.0f);

    // Cambios de fase: cada uno con su efecto.
    if (m.fase != m.faseVista)
    {
        FaseInterruptoresCaos antes = m.faseVista;
        m.faseVista = m.fase;
        int palanca = m.interruptorSeleccionado;

        if (m.fase == FASE_INTERRUPTORES_ELECCION)
        {
            if (antes == FASE_INTERRUPTORES_EXPLOSION)
            {
                // Ronda nueva: las palancas vuelven a armarse en ola.
                AnunciarCentral(m, ANUNCIO_INTERRUPTORES_RONDA, m.jugadorTurno, m.cantidadInterruptores);
                m.aparicion = 0.0f;
            }
            else
            {
                AnunciarCentral(m, ANUNCIO_INTERRUPTORES_TURNO, m.jugadorTurno, -1);
            }

            m.seleccionVista = palanca;
        }
        else if (m.fase == FASE_INTERRUPTORES_SEGURO && palanca >= 0 && palanca < MAX_INTERRUPTORES_CAOS)
        {
            m.destelloPalanca[palanca] = 1.0f;
            ChispasCentral(m, PuntaPalancaCentral(m, palanca, 0.3f), Color{ 120, 240, 140, 255 }, 12, 2.4f);

            if (m.jugadorTurno >= 0)
            {
                m.jugadores[m.jugadorTurno].salto = DURACION_SALTO_INTERRUPTORES;
            }

            AnunciarCentral(m, ANUNCIO_INTERRUPTORES_SEGURO, m.jugadorTurno, palanca);
        }
        else if (m.fase == FASE_INTERRUPTORES_EXPLOSION)
        {
            int victima = m.jugadorExplosion;
            m.sobrecarga = 1.0f;
            ActivarTemblorCamaraGeneral(0.5f, 0.7f);

            if (palanca >= 0 && palanca < MAX_INTERRUPTORES_CAOS)
            {
                m.destelloPalanca[palanca] = 1.0f;
                Vector3 base = PivotePalancaCentral(m, palanca);
                ChispasCentral(m, base, Color{ 255, 170, 60, 255 }, 26, 3.4f);
                HumoCentral(m, base, 6, 0.5f);
            }

            ChispasCentral(m, AISLADOR_CENTRAL, Color{ 150, 230, 255, 255 }, 18, 2.6f);

            if (victima >= 0)
            {
                m.jugadores[victima].descarga = 0.0001f;
                m.jugadores[victima].cayo = false;
                ChispasCentral(m, PechoOperadorCentral(m, victima), Color{ 150, 230, 255, 255 }, 24, 3.0f);
            }

            AnunciarCentral(m, ANUNCIO_INTERRUPTORES_SOBRECARGA, victima, palanca);
        }
        else if (m.fase == FASE_INTERRUPTORES_TERMINADO)
        {
            m.tiempoFin = 0.0f;
        }
    }

    if (m.fase == FASE_INTERRUPTORES_ELECCION && m.interruptorSeleccionado != m.seleccionVista)
    {
        m.seleccionVista = m.interruptorSeleccionado;
        m.pulsoSeleccion = 0.22f;
    }

    // Operadores: salto, vuelo de la descarga y humo del chamuscado.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorInterruptoresCaos& j = m.jugadores[i];
        j.salto = DescontarCentral(j.salto, dt);

        if (j.descarga <= 0.0f)
        {
            continue;
        }

        j.descarga += dt;

        if (!j.cayo && j.descarga >= DURACION_VUELO_INTERRUPTORES)
        {
            j.cayo = true;
            ReproducirSonidoMinijuego(m.audio, SONIDO_CAIDA);
            ActivarTemblorCamaraGeneral(0.16f, 0.22f);
            HumoCentral(m, PosicionPasarelaInterruptores(m, i), 5, 0.4f);
        }

        if (j.cayo && std::fmod(m.tiempoAnimacion + 0.17f * (float)i, 0.5f) < dt)
        {
            HumoCentral(m, PechoOperadorCentral(m, i), 1, 0.22f);
        }
    }

    // Fin: confeti sobre el que quedo en pie, en tandas.
    if (m.fase == FASE_INTERRUPTORES_TERMINADO)
    {
        bool primeraVez = m.tiempoFin <= 0.0f;
        bool tanda = primeraVez || std::fmod(m.tiempoFin, 0.6f) < dt;
        m.tiempoFin += dt;
        int ganador = m.jugadorTurno;

        if (tanda && ganador >= 0 && ganador < MAX_PARTICIPANTES)
        {
            Vector3 cabeza = PosicionPasarelaInterruptores(m, ganador);
            cabeza.y += 2.4f;
            ConfetiCentral(m, cabeza, participantes[ganador].color, primeraVez ? 44 : 10);
        }
    }

    // Empalme flojo en el armario de cortes: chispazo cada tanto.
    m.proximaChispa -= dt;

    if (m.proximaChispa <= 0.0f)
    {
        m.proximaChispa = AzarCentral(m, 1.8f, 4.6f) - 1.2f * m.tensionVista;
        ChispasCentral(m, EMPALME_CENTRAL, Color{ 255, 200, 120, 255 }, 7, 1.6f);
    }

    for (int i = 0; i < MAX_PARTICULAS_INTERRUPTORES; i++)
    {
        ParticulaInterruptoresCaos& p = m.particulas[i];

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

        float freno = 1.0f - Limitar01Central(dt * (p.humo ? 0.6f : (p.brillo ? 2.0f : 1.2f)));
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.velocidad.y -= p.gravedad * dt;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        if (!p.humo && p.posicion.y < 0.03f && p.velocidad.y < 0.0f)
        {
            p.posicion.y = 0.03f;
            p.velocidad = { 0.0f, 0.0f, 0.0f };
            p.gravedad = 0.0f;
        }
    }
}


//==================================================
// PALETA, NIEBLA Y PRIMITIVAS
//==================================================

static const Color COLOR_PENUMBRA_CENTRAL = { 18, 24, 38, 255 };     // fondo de la sala
static const Color COLOR_BRUMA_NAVE_CENTRAL = { 46, 38, 38, 255 };   // penumbra de la nave
static const Color COLOR_BRUMA_NOCHE_CENTRAL = { 40, 54, 88, 255 };  // noche afuera
static const Color COLOR_PISO_CENTRAL = { 56, 64, 78, 255 };
static const Color COLOR_JUNTA_CENTRAL = { 34, 38, 48, 255 };
static const Color COLOR_CONSOLA_CENTRAL = { 118, 146, 136, 255 };   // verde menta gastado
static const Color COLOR_CREMA_CENTRAL = { 206, 200, 178, 255 };
static const Color COLOR_GRAFITO_CENTRAL = { 44, 48, 58, 255 };
static const Color COLOR_ACERO_CENTRAL = { 120, 130, 144, 255 };
static const Color COLOR_MURO_CENTRAL = { 92, 104, 118, 255 };
static const Color COLOR_TURBINA_CENTRAL = { 46, 116, 108, 255 };
static const Color COLOR_CARCASA_CENTRAL = { 168, 172, 166, 255 };
static const Color COLOR_HORMIGON_CENTRAL = { 128, 128, 120, 255 };
static const Color COLOR_COBRE_CENTRAL = { 196, 116, 64, 255 };
static const Color COLOR_PELIGRO_CENTRAL = { 214, 150, 40, 255 };     // franjas de precaucion
static const Color COLOR_ACENTO_CENTRAL = { 255, 176, 46, 255 };      // palanca elegida
static const Color COLOR_PANTALLA_CENTRAL = { 86, 226, 206, 255 };
static const Color COLOR_SEGURO_CENTRAL = { 110, 236, 130, 255 };
static const Color COLOR_ALARMA_CENTRAL = { 255, 56, 44, 255 };
static const Color COLOR_TUBO_CENTRAL = { 210, 234, 255, 255 };       // luz de los tubos

static const Vector3 OJO_NIEBLA_CENTRAL = { 0.0f, 11.0f, 19.0f };


static unsigned char CanalCentral(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoCentral(Color color, float factor)
{
    return Color{
        CanalCentral((float)color.r * factor),
        CanalCentral((float)color.g * factor),
        CanalCentral((float)color.b * factor),
        color.a
    };
}


// Luz de tubo fluorescente desde arriba: caras laterales algo azuladas.
static Color SombraCentral(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalCentral((float)color.r * factor * 0.99f),
            CanalCentral((float)color.g * factor * 1.01f),
            CanalCentral((float)color.b * factor * 1.03f),
            color.a
        };
    }

    return Color{
        CanalCentral((float)color.r * factor * 0.93f),
        CanalCentral((float)color.g * factor * 0.96f),
        CanalCentral((float)color.b * factor * 1.04f),
        color.a
    };
}


static Color MezclaCentral(Color a, Color b, float t)
{
    t = Limitar01Central(t);

    return Color{
        CanalCentral((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalCentral((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalCentral((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalCentral((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color ConAlfaCentral(Color color, float alfa)
{
    color.a = CanalCentral(alfa);
    return color;
}


// Niebla por zonas: la sala se apaga hacia la penumbra azul, la nave
// hacia la bruma de sodio y el exterior hacia la noche.
static Color TinteCentral(Color color, Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_CENTRAL.x;
    float dy = p.y - OJO_NIEBLA_CENTRAL.y;
    float dz = p.z - OJO_NIEBLA_CENTRAL.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    Color niebla = COLOR_PENUMBRA_CENTRAL;
    float n = 0.0f;

    if (p.z > Z_VENTANAL_CENTRAL - 0.8f)
    {
        n = Limitar01Central((d - 16.0f) / 40.0f) * 0.32f;
    }
    else if (p.z > Z_FONDO_NAVE_CENTRAL - 1.0f)
    {
        niebla = COLOR_BRUMA_NAVE_CENTRAL;
        n = 0.24f + Limitar01Central((d - 30.0f) / 50.0f) * 0.34f;
    }
    else
    {
        niebla = COLOR_BRUMA_NOCHE_CENTRAL;
        n = 0.25f + Limitar01Central((d - 70.0f) / 460.0f) * 0.6f;
    }

    niebla.a = color.a;
    return MezclaCentral(color, niebla, n);
}


static void VerticeCentral(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadCentral(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeCentral(a, color);
    VerticeCentral(b, color);
    VerticeCentral(c, color);
    VerticeCentral(d, color);
}


// Quad con la niebla calculada por vertice (suelos y muros grandes).
static void QuadNieblaCentral(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeCentral(a, TinteCentral(color, a));
    VerticeCentral(b, TinteCentral(color, b));
    VerticeCentral(c, TinteCentral(color, c));
    VerticeCentral(d, TinteCentral(color, d));
}


// Caja sobre tres ejes (medios lados ya escalados): ey arriba 1.0,
// ez frente 0.78, -ex izquierda 0.70, +ex derecha 0.62, fondo 0.54.
static void CajaEjesCentral(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color)
{
    Vector3 v[8];

    for (int k = 0; k < 8; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sy = (k & 2) ? 1.0f : -1.0f;
        float sz = (k & 4) ? 1.0f : -1.0f;
        v[k] =
        {
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    }

    rlBegin(RL_QUADS);
    QuadCentral(v[6], v[7], v[3], v[2], SombraCentral(color, 1.0f, true));     // arriba
    QuadCentral(v[4], v[5], v[7], v[6], SombraCentral(color, 0.78f, false));   // frente
    QuadCentral(v[1], v[0], v[2], v[3], SombraCentral(color, 0.54f, false));   // fondo
    QuadCentral(v[5], v[1], v[3], v[7], SombraCentral(color, 0.62f, false));   // derecha
    QuadCentral(v[0], v[4], v[6], v[2], SombraCentral(color, 0.70f, false));   // izquierda
    QuadCentral(v[0], v[1], v[5], v[4], SombraCentral(color, 0.45f, false));   // abajo
    rlEnd();
}


// Cubo con niebla segun su centro.
static void CuboCentral(Vector3 c, Vector3 t, Color color)
{
    CajaEjesCentral(
        c,
        { t.x * 0.5f, 0.0f, 0.0f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { 0.0f, 0.0f, t.z * 0.5f },
        TinteCentral(color, c)
    );
}


// Cubo sin niebla (piezas que brillan o se iluminan solas).
static void CuboPuroCentral(Vector3 c, Vector3 t, Color color)
{
    CajaEjesCentral(
        c,
        { t.x * 0.5f, 0.0f, 0.0f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { 0.0f, 0.0f, t.z * 0.5f },
        color
    );
}


static void CuboGiradoCentral(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    float a = giroGrados * DEG2RAD;
    float co = std::cos(a);
    float si = std::sin(a);

    CajaEjesCentral(
        c,
        { co * t.x * 0.5f, 0.0f, -si * t.x * 0.5f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { si * t.z * 0.5f, 0.0f, co * t.z * 0.5f },
        TinteCentral(color, c)
    );
}


// Viga de seccion cuadrada entre dos puntos.
static void VigaCentral(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 c = Vector3Scale(Vector3Add(a, b), 0.5f);
    float g = grosor * 0.5f;
    Color tinte = TinteCentral(color, c);

    if (std::fabs(dir.y) > 0.7f)
    {
        Vector3 ex = Vector3Normalize(Vector3CrossProduct(dir, { 0.0f, 0.0f, 1.0f }));
        Vector3 ez = Vector3CrossProduct(ex, dir);
        if (dir.y < 0.0f) { dir = Vector3Negate(dir); ex = Vector3Negate(ex); }
        CajaEjesCentral(c, Vector3Scale(ex, g), Vector3Scale(dir, largo * 0.5f), Vector3Scale(ez, g), tinte);
    }
    else
    {
        Vector3 ez = Vector3Normalize(Vector3CrossProduct(dir, { 0.0f, 1.0f, 0.0f }));
        Vector3 ey = Vector3CrossProduct(ez, dir);
        if (ey.y < 0.0f) { ey = Vector3Negate(ey); ez = Vector3Negate(ez); }
        if (ez.z < 0.0f) { ez = Vector3Negate(ez); dir = Vector3Negate(dir); }
        CajaEjesCentral(c, Vector3Scale(dir, largo * 0.5f), Vector3Scale(ey, g), Vector3Scale(ez, g), tinte);
    }
}


// Cilindro o tronco de cono entre dos puntos, con caras sombreadas.
static void CilindroEjeCentral(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool niebla = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    if (niebla)
    {
        color = TinteCentral(color, Vector3Scale(Vector3Add(a, b), 0.5f));
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(dir.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 e1 = Vector3Normalize(Vector3CrossProduct(dir, ref));
    Vector3 e2 = Vector3CrossProduct(dir, e1);
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 nm = Vector3Add(Vector3Scale(e1, std::cos(am)), Vector3Scale(e2, std::sin(am)));
        float luz = 0.58f + 0.42f * Limitar01Central(nm.y * 0.62f + nm.z * 0.4f + 0.35f);
        Color cara = SombraCentral(color, luz, false);

        Vector3 o0 = Vector3Add(Vector3Scale(e1, std::cos(a0)), Vector3Scale(e2, std::sin(a0)));
        Vector3 o1 = Vector3Add(Vector3Scale(e1, std::cos(a1)), Vector3Scale(e2, std::sin(a1)));

        QuadCentral(
            Vector3Add(a, Vector3Scale(o0, ra)),
            Vector3Add(a, Vector3Scale(o1, ra)),
            Vector3Add(b, Vector3Scale(o1, rb)),
            Vector3Add(b, Vector3Scale(o0, rb)),
            cara
        );

        // Tapas: la de b iluminada, la de a algo mas oscura.
        if (rb > 0.001f)
        {
            QuadCentral(b, Vector3Add(b, Vector3Scale(o0, rb)), Vector3Add(b, Vector3Scale(o1, rb)), b, SombraCentral(color, 0.9f, true));
        }

        if (ra > 0.001f)
        {
            QuadCentral(a, Vector3Add(a, Vector3Scale(o1, ra)), Vector3Add(a, Vector3Scale(o0, ra)), a, SombraCentral(color, 0.74f, false));
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloCentral(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadCentral(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Sector de corona en un plano vertical que mira a +z (diales y
// medidores). Angulos en grados: 0 = derecha, 90 = arriba.
static void ArcoFrenteCentral(Vector3 c, float r0, float r1, float desde, float hasta, Color color, int lados = 12)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = (desde + (hasta - desde) * (float)k / (float)lados) * DEG2RAD;
        float a1 = (desde + (hasta - desde) * (float)(k + 1) / (float)lados) * DEG2RAD;

        QuadCentral(
            { c.x + std::cos(a0) * r0, c.y + std::sin(a0) * r0, c.z },
            { c.x + std::cos(a0) * r1, c.y + std::sin(a0) * r1, c.z },
            { c.x + std::cos(a1) * r1, c.y + std::sin(a1) * r1, c.z },
            { c.x + std::cos(a1) * r0, c.y + std::sin(a1) * r0, c.z },
            color
        );
    }

    rlEnd();
}


// Rectangulo plano que mira a +z (pantallas, carteles, rendijas).
static void PlacaFrenteCentral(float x0, float y0, float x1, float y1, float z, Color color)
{
    rlBegin(RL_QUADS);
    QuadCentral({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, { x0, y1, z }, color);
    rlEnd();
}


// Linea gruesa en un plano que mira a +z (trazos de los monitores).
static void TrazoFrenteCentral(Vector2 a, Vector2 b, float z, float grosor, Color color)
{
    Vector2 d = { b.x - a.x, b.y - a.y };
    float largo = std::sqrt(d.x * d.x + d.y * d.y);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector2 n = { -d.y / largo * grosor * 0.5f, d.x / largo * grosor * 0.5f };

    rlBegin(RL_QUADS);
    QuadCentral({ a.x - n.x, a.y - n.y, z }, { b.x - n.x, b.y - n.y, z }, { b.x + n.x, b.y + n.y, z }, { a.x + n.x, a.y + n.y, z }, color);
    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorCentral(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloCentral(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorCentral(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano inclinado como la vista).
static void ResplandorFrenteCentral(Vector3 c, float radio, Color color)
{
    ResplandorCentral(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.93f, 0.36f }, radio, color);
}


// Disco solido de borde irregular en el piso (quemaduras, manchas).
static void ManchaPisoCentral(Vector3 c, float radio, int semilla, Color color)
{
    const int LADOS = 12;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float r0 = radio * (0.7f + 0.5f * Hash01Central(semilla, k));
        float r1 = radio * (0.7f + 0.5f * Hash01Central(semilla, (k + 1) % LADOS));

        VerticeCentral(c, color);
        VerticeCentral({ c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 }, ConAlfaCentral(color, 0.0f));
        VerticeCentral({ c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 }, ConAlfaCentral(color, 0.0f));
    }

    rlEnd();
}


// Rayo con zigzag entre dos puntos (cambia rapido con el tiempo).
static void RayoCentral(Vector3 origen, Vector3 destino, float t, float semilla, float ruido, Color color)
{
    Vector3 anterior = origen;
    const int TRAMOS = 9;

    for (int tramo = 1; tramo <= TRAMOS; tramo++)
    {
        float f = (float)tramo / (float)TRAMOS;
        float r = tramo == TRAMOS ? 0.0f : ruido * std::sin(f * PI);
        Vector3 siguiente =
        {
            origen.x + (destino.x - origen.x) * f + std::sin(t * 53.0f + semilla + tramo * 2.3f) * r,
            origen.y + (destino.y - origen.y) * f + std::sin(t * 47.0f + semilla * 1.7f + tramo * 1.1f) * r,
            origen.z + (destino.z - origen.z) * f + std::sin(t * 61.0f + semilla * 0.6f + tramo * 3.7f) * r * 0.6f
        };

        // Tres pasadas corridas para que el trazo tenga cuerpo.
        DrawLine3D(anterior, siguiente, color);
        DrawLine3D({ anterior.x + 0.03f, anterior.y, anterior.z }, { siguiente.x + 0.03f, siguiente.y, siguiente.z }, color);
        DrawLine3D({ anterior.x, anterior.y + 0.03f, anterior.z }, { siguiente.x, siguiente.y + 0.03f, siguiente.z }, ConAlfaCentral(color, color.a * 0.6f));
        anterior = siguiente;
    }
}


//==================================================
// CIELO NOCTURNO Y EXTERIOR
//==================================================

// Cielo (2D, antes del 3D): azul noche arriba, franja verdosa sobre la
// sierra, estrellas que titilan y la luna con su halo.
static void DibujarCieloCentral(const Camera3D& vista, float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    Vector3 lejos = { vista.position.x, vista.position.y - 30.0f, vista.position.z - 700.0f };
    float horizonte = GetWorldToScreen(lejos, vista).y;
    if (horizonte < (float)alto * 0.12f) horizonte = (float)alto * 0.12f;
    if (horizonte > (float)alto * 0.80f) horizonte = (float)alto * 0.80f;

    int h1 = (int)(horizonte * 0.55f);
    int h2 = (int)horizonte;

    DrawRectangleGradientV(0, 0, ancho, h1, Color{ 8, 12, 30, 255 }, Color{ 20, 32, 66, 255 });
    DrawRectangleGradientV(0, h1, ancho, h2 - h1, Color{ 20, 32, 66, 255 }, Color{ 52, 78, 104, 255 });
    DrawRectangleGradientV(0, h2, ancho, alto - h2, Color{ 52, 78, 104, 255 }, Color{ 24, 30, 44, 255 });

    // Luna alta a la izquierda, con halo frio.
    Vector2 luna = GetWorldToScreen({ -150.0f, 70.0f, -620.0f }, vista);
    float radio = (float)alto * 0.035f;
    DrawCircleGradient(luna, radio * 6.0f, Fade(Color{ 150, 180, 230, 255 }, 0.22f), Fade(Color{ 60, 80, 140, 255 }, 0.0f));
    DrawCircleV(luna, radio, Color{ 236, 238, 222, 255 });
    DrawCircleV({ luna.x + radio * 0.3f, luna.y - radio * 0.2f }, radio * 0.25f, Color{ 214, 216, 204, 255 });
    DrawCircleV({ luna.x - radio * 0.35f, luna.y + radio * 0.3f }, radio * 0.18f, Color{ 220, 222, 210, 255 });

    if (!detalle)
    {
        return;
    }

    for (int k = 0; k < 120; k++)
    {
        float x = Hash01Central(k, 1) * (float)ancho;
        float y = Hash01Central(k, 2) * horizonte * 0.95f;
        float brillo = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(t * (1.0f + 2.0f * Hash01Central(k, 3)) + (float)k));
        float tam = Hash01Central(k, 4) > 0.9f ? 2.0f : 1.0f;
        DrawRectangle((int)x, (int)y, (int)tam, (int)tam, Fade(Color{ 220, 230, 255, 255 }, brillo * (1.0f - y / horizonte * 0.6f)));
    }

    // Nubes finas que cruzan despacio, iluminadas por la luna.
    for (int k = 0; k < 6; k++)
    {
        float recorrido = (float)(ancho + 700);
        float x = std::fmod(Hash01Central(k, 21) * recorrido + t * (3.0f + 3.0f * Hash01Central(k, 20)), recorrido) - 350.0f;
        float y = horizonte * (0.25f + 0.6f * Hash01Central(k, 22));
        float largo = 140.0f + 220.0f * Hash01Central(k, 23);
        DrawEllipse((int)x, (int)y, largo, 5.0f + 6.0f * Hash01Central(k, 24), Fade(Color{ 70, 90, 140, 255 }, 0.45f));
        DrawEllipse((int)(x + largo * 0.12f), (int)(y - 2.0f), largo * 0.7f, 3.0f, Fade(Color{ 130, 150, 196, 255 }, 0.35f));
    }
}


// Torre de enfriamiento: hiperboloide de anillos apilados.
static void TorreEnfriamientoCentral(Vector3 base, float radio, float alto, int lados)
{
    const int TRAMOS = 7;

    for (int k = 0; k < TRAMOS; k++)
    {
        float f0 = (float)k / (float)TRAMOS;
        float f1 = (float)(k + 1) / (float)TRAMOS;
        float c0 = (f0 - 0.72f) / 0.72f;
        float c1 = (f1 - 0.72f) / 0.72f;
        float r0 = radio * (0.66f + 0.34f * c0 * c0);
        float r1 = radio * (0.66f + 0.34f * c1 * c1);
        Color hormigon = MezclaCentral(Color{ 132, 132, 128, 255 }, Color{ 168, 160, 150, 255 }, f0);

        CilindroEjeCentral(
            { base.x, base.y + alto * f0, base.z },
            { base.x, base.y + alto * f1, base.z },
            r0, r1, lados, hormigon
        );
    }

    // Patas del borde inferior (oscuras) y franja de luz de mantenimiento.
    CilindroEjeCentral({ base.x, base.y, base.z }, { base.x, base.y + alto * 0.04f, base.z }, radio * 1.02f, radio * 1.0f, lados, Color{ 40, 42, 48, 255 });
}


// Torre de alta tension de reticulado.
static void TorreAltaTensionCentral(Vector3 base, float alto, bool detalle)
{
    Color acero = Color{ 96, 104, 120, 255 };
    float abajo = alto * 0.16f;
    float arriba = alto * 0.04f;

    for (int k = 0; k < 4; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sz = (k & 2) ? 1.0f : -1.0f;
        VigaCentral(
            { base.x + sx * abajo, base.y, base.z + sz * abajo },
            { base.x + sx * arriba, base.y + alto, base.z + sz * arriba },
            alto * 0.018f,
            acero
        );
    }

    // Crucetas: tres brazos de los conductores.
    for (int b = 0; b < 3; b++)
    {
        float y = base.y + alto * (0.62f + 0.14f * (float)b);
        float brazo = alto * (b == 1 ? 0.34f : 0.26f);
        VigaCentral({ base.x - brazo, y, base.z }, { base.x + brazo, y, base.z }, alto * 0.02f, acero);
    }

    if (!detalle)
    {
        return;
    }

    // Diagonales del cuerpo.
    for (int k = 0; k < 4; k++)
    {
        float f0 = 0.12f + 0.2f * (float)k;
        float f1 = f0 + 0.2f;
        float a0 = abajo + (arriba - abajo) * f0;
        float a1 = abajo + (arriba - abajo) * f1;
        VigaCentral({ base.x - a0, base.y + alto * f0, base.z }, { base.x + a1, base.y + alto * f1, base.z }, alto * 0.01f, acero);
        VigaCentral({ base.x + a0, base.y + alto * f0, base.z }, { base.x - a1, base.y + alto * f1, base.z }, alto * 0.01f, acero);
    }
}


static const int TORRES_TENDIDO_CENTRAL = 6;


static Vector3 BaseTorreTendidoCentral(int k)
{
    return { 46.0f + 34.0f * (float)k, Y_EXTERIOR_CENTRAL, -96.0f - 62.0f * (float)k };
}


// Cable con catenaria entre dos puntos.
static void CatenariaCentral(Vector3 a, Vector3 b, float flecha, Color color)
{
    Vector3 anterior = a;

    for (int k = 1; k <= 10; k++)
    {
        float f = (float)k / 10.0f;
        Vector3 p =
        {
            a.x + (b.x - a.x) * f,
            a.y + (b.y - a.y) * f - flecha * 4.0f * f * (1.0f - f),
            a.z + (b.z - a.z) * f
        };
        DrawLine3D(anterior, p, color);
        anterior = p;
    }
}


// Sierra: crestas con hash como franjas de triangulos.
static void SierraCentral(float z, float alturaBase, float amplitud, int picos, int semilla, Color color)
{
    float x0 = -720.0f;
    float paso = 1440.0f / (float)picos;

    rlBegin(RL_QUADS);

    for (int k = 0; k < picos; k++)
    {
        float xa = x0 + paso * (float)k;
        float xb = xa + paso;
        float ya = alturaBase + amplitud * (0.25f + 0.75f * Hash01Central(semilla, k));
        float yb = alturaBase + amplitud * (0.25f + 0.75f * Hash01Central(semilla, k + 1));
        Vector3 a = { xa, Y_EXTERIOR_CENTRAL - 2.0f, z };
        Vector3 b = { xb, Y_EXTERIOR_CENTRAL - 2.0f, z };
        Vector3 c = { xb, yb, z };
        Vector3 d = { xa, ya, z };
        Color pie = TinteCentral(TonoCentral(color, 0.8f), a);
        Color cresta = TinteCentral(color, d);
        VerticeCentral(a, pie);
        VerticeCentral(b, pie);
        VerticeCentral(c, cresta);
        VerticeCentral(d, cresta);
    }

    rlEnd();
}


static void DibujarExteriorCentral(float t, bool detalle)
{
    // Sierra en dos planos: la de atras mas clara por la noche.
    SierraCentral(-640.0f, 26.0f, 70.0f, 28, 11, Color{ 70, 86, 124, 255 });
    SierraCentral(-430.0f, 4.0f, 46.0f, 34, 23, Color{ 34, 44, 64, 255 });

    // Llanura exterior.
    rlBegin(RL_QUADS);
    QuadNieblaCentral(
        { -700.0f, Y_EXTERIOR_CENTRAL, Z_FONDO_NAVE_CENTRAL - 2.0f },
        { 700.0f, Y_EXTERIOR_CENTRAL, Z_FONDO_NAVE_CENTRAL - 2.0f },
        { 700.0f, Y_EXTERIOR_CENTRAL, -440.0f },
        { -700.0f, Y_EXTERIOR_CENTRAL, -440.0f },
        Color{ 30, 36, 42, 255 }
    );
    rlEnd();

    // Torres de enfriamiento.
    TorreEnfriamientoCentral({ -64.0f, Y_EXTERIOR_CENTRAL, -150.0f }, 17.0f, 48.0f, detalle ? 20 : 12);
    TorreEnfriamientoCentral({ -18.0f, Y_EXTERIOR_CENTRAL, -190.0f }, 16.0f, 44.0f, detalle ? 20 : 12);

    if (detalle)
    {
        TorreEnfriamientoCentral({ -110.0f, Y_EXTERIOR_CENTRAL, -230.0f }, 15.0f, 40.0f, 14);
    }

    // Tendido de alta tension hacia la sierra.
    for (int k = 0; k < TORRES_TENDIDO_CENTRAL; k++)
    {
        TorreAltaTensionCentral(BaseTorreTendidoCentral(k), 30.0f, detalle);
    }

    for (int k = 0; k + 1 < TORRES_TENDIDO_CENTRAL; k++)
    {
        Vector3 a = BaseTorreTendidoCentral(k);
        Vector3 b = BaseTorreTendidoCentral(k + 1);

        for (int fase = 0; fase < 3; fase++)
        {
            float y = 30.0f * (0.62f + 0.14f * (float)fase);
            float brazo = 30.0f * (fase == 1 ? 0.34f : 0.26f);

            for (int lado = -1; lado <= 1; lado += 2)
            {
                CatenariaCentral(
                    { a.x + brazo * (float)lado, a.y + y, a.z },
                    { b.x + brazo * (float)lado, b.y + y, b.z },
                    4.0f,
                    TinteCentral(Color{ 90, 100, 118, 255 }, a)
                );
            }
        }
    }

    if (!detalle)
    {
        return;
    }

    // Playa de transformadores al pie de las torres: cajas y porticos.
    for (int k = 0; k < 8; k++)
    {
        float x = 18.0f + 9.0f * (float)k;
        float z = -78.0f - 6.0f * Hash01Central(k, 70);
        CuboCentral({ x, Y_EXTERIOR_CENTRAL + 1.6f, z }, { 4.0f, 3.2f, 3.0f }, Color{ 70, 80, 88, 255 });
        VigaCentral({ x - 2.0f, Y_EXTERIOR_CENTRAL, z - 4.0f }, { x - 2.0f, Y_EXTERIOR_CENTRAL + 8.0f, z - 4.0f }, 0.4f, Color{ 90, 98, 112, 255 });
        VigaCentral({ x + 2.0f, Y_EXTERIOR_CENTRAL, z - 4.0f }, { x + 2.0f, Y_EXTERIOR_CENTRAL + 8.0f, z - 4.0f }, 0.4f, Color{ 90, 98, 112, 255 });
        VigaCentral({ x - 2.4f, Y_EXTERIOR_CENTRAL + 8.0f, z - 4.0f }, { x + 2.4f, Y_EXTERIOR_CENTRAL + 8.0f, z - 4.0f }, 0.4f, Color{ 90, 98, 112, 255 });
    }

    // Chimenea de la sala de calderas con franjas.
    for (int k = 0; k < 6; k++)
    {
        float y0 = Y_EXTERIOR_CENTRAL + 10.0f * (float)k;
        CilindroEjeCentral(
            { 30.0f, y0, -150.0f },
            { 30.0f, y0 + 10.0f, -150.0f },
            4.0f - 0.25f * (float)k,
            3.75f - 0.25f * (float)k,
            14,
            k == 5 || k == 3 ? Color{ 176, 104, 66, 255 } : Color{ 176, 172, 164, 255 }
        );
    }

    (void)t;
}


//==================================================
// NAVE DE TURBINAS
//==================================================

static const float X_TURBINAS_CENTRAL[3] = { -13.0f, 0.0f, 13.0f };
static const float Y_EJE_TURBINA_CENTRAL = Y_NAVE_CENTRAL + 3.9f;


static float ZPuenteGruaCentral(float t)
{
    return -32.0f + std::sin(t * 0.11f) * 9.0f;
}


// Turbogenerador a lo largo de z: excitatriz, volante que gira,
// generador verde petroleo y carcasas de turbina escalonadas.
static void TurbogeneradorCentral(float x, float t, int indice, bool detalle)
{
    float y = Y_EJE_TURBINA_CENTRAL;
    int lados = detalle ? 18 : 12;

    // Basamento de hormigon con franja de precaucion.
    CuboCentral({ x, Y_NAVE_CENTRAL + 0.8f, -28.0f }, { 7.4f, 1.6f, 27.0f }, COLOR_HORMIGON_CENTRAL);
    CuboCentral({ x, Y_NAVE_CENTRAL + 1.62f, -28.0f }, { 7.6f, 0.06f, 27.2f }, TonoCentral(COLOR_HORMIGON_CENTRAL, 0.86f));

    for (int k = 0; k < 14 && detalle; k++)
    {
        float z = -15.2f - 1.9f * (float)k;
        CuboCentral({ x - 3.72f, Y_NAVE_CENTRAL + 1.4f, z }, { 0.06f, 0.3f, 0.95f }, k % 2 == 0 ? COLOR_PELIGRO_CENTRAL : COLOR_GRAFITO_CENTRAL);
        CuboCentral({ x + 3.72f, Y_NAVE_CENTRAL + 1.4f, z }, { 0.06f, 0.3f, 0.95f }, k % 2 == 0 ? COLOR_PELIGRO_CENTRAL : COLOR_GRAFITO_CENTRAL);
    }

    // Pedestales de los cojinetes.
    for (int k = 0; k < 4; k++)
    {
        float z = -16.5f - 7.5f * (float)k;
        CuboCentral({ x, Y_NAVE_CENTRAL + 2.4f, z }, { 2.6f, 1.6f, 1.0f }, COLOR_GRAFITO_CENTRAL);
    }

    // Excitatriz.
    CilindroEjeCentral({ x, y, -14.6f }, { x, y, -17.0f }, 1.15f, 1.15f, lados, COLOR_TURBINA_CENTRAL);

    // Volante con rayos que gira (eje expuesto).
    float giro = t * (2.4f + 0.3f * (float)indice);
    CilindroEjeCentral({ x, y, -17.0f }, { x, y, -17.5f }, 0.3f, 0.3f, 8, COLOR_ACERO_CENTRAL);
    CilindroEjeCentral({ x, y, -17.5f }, { x, y, -17.8f }, 1.7f, 1.7f, lados, Color{ 60, 66, 74, 255 });

    for (int r = 0; r < 4; r++)
    {
        float a = giro + (float)r * PI * 0.5f;
        VigaCentral(
            { x - std::cos(a) * 1.6f, y - std::sin(a) * 1.6f, -17.46f },
            { x + std::cos(a) * 1.6f, y + std::sin(a) * 1.6f, -17.46f },
            0.22f,
            COLOR_PELIGRO_CENTRAL
        );
    }

    // Generador con aletas de refrigeracion arriba.
    CilindroEjeCentral({ x, y, -17.8f }, { x, y, -26.2f }, 2.3f, 2.3f, lados, COLOR_TURBINA_CENTRAL);
    CilindroEjeCentral({ x, y, -17.8f }, { x, y, -18.3f }, 2.45f, 2.45f, lados, TonoCentral(COLOR_TURBINA_CENTRAL, 0.8f));
    CilindroEjeCentral({ x, y, -25.7f }, { x, y, -26.2f }, 2.45f, 2.45f, lados, TonoCentral(COLOR_TURBINA_CENTRAL, 0.8f));

    for (int k = 0; k < 6 && detalle; k++)
    {
        CuboCentral({ x, y + 2.35f, -19.2f - 1.15f * (float)k }, { 2.2f, 0.4f, 0.3f }, TonoCentral(COLOR_TURBINA_CENTRAL, 0.9f));
    }

    // Carcasas de turbina: baja presion (ancha) y alta presion.
    CilindroEjeCentral({ x, y, -26.8f }, { x, y, -34.4f }, 2.9f, 3.0f, lados, COLOR_CARCASA_CENTRAL);
    CilindroEjeCentral({ x, y, -34.4f }, { x, y, -35.0f }, 3.15f, 3.15f, lados, TonoCentral(COLOR_CARCASA_CENTRAL, 0.84f));
    CilindroEjeCentral({ x, y, -35.0f }, { x, y, -40.0f }, 2.3f, 1.9f, lados, COLOR_CARCASA_CENTRAL);

    // Bridas atornilladas.
    for (int k = 0; k < 3; k++)
    {
        float z = -28.6f - 2.4f * (float)k;
        CilindroEjeCentral({ x, y, z }, { x, y, z - 0.22f }, 3.08f, 3.08f, lados, TonoCentral(COLOR_CARCASA_CENTRAL, 0.78f));
    }

    // Caños de vapor que suben hasta el techo.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float cx = x + 1.6f * (float)lado;
        CilindroEjeCentral({ cx, y + 2.0f, -30.5f }, { cx, 16.0f, -30.5f }, 0.55f, 0.55f, 10, Color{ 128, 132, 126, 255 });
        CilindroEjeCentral({ cx, y + 2.0f, -36.5f }, { cx, 16.0f, -36.5f }, 0.42f, 0.42f, 10, Color{ 128, 132, 126, 255 });
    }

    // Baranda amarilla sobre el basamento.
    if (detalle)
    {
        Color baranda = COLOR_PELIGRO_CENTRAL;
        float yb = Y_NAVE_CENTRAL + 1.6f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            float bx = x + 3.5f * (float)lado;
            VigaCentral({ bx, yb + 1.1f, -15.0f }, { bx, yb + 1.1f, -41.0f }, 0.08f, baranda);
            VigaCentral({ bx, yb + 0.55f, -15.0f }, { bx, yb + 0.55f, -41.0f }, 0.06f, baranda);

            for (int k = 0; k < 9; k++)
            {
                float z = -15.0f - 3.25f * (float)k;
                VigaCentral({ bx, yb, z }, { bx, yb + 1.1f, z }, 0.08f, baranda);
            }
        }
    }
}


static void DibujarNaveCentral(float t, bool detalle)
{
    float z0 = Z_VENTANAL_CENTRAL - 0.6f;
    float z1 = Z_FONDO_NAVE_CENTRAL;

    // Piso de hormigon pintado en losas, con lineas de transito.
    rlBegin(RL_QUADS);

    for (int fx = -6; fx < 6; fx++)
    {
        for (int fz = 0; fz < 8; fz++)
        {
            float xa = (float)fx * 5.8f;
            float za = z0 - 5.5f * (float)fz;
            float var = 0.9f + 0.16f * Hash01Central(fx + 40, fz + 80);
            QuadNieblaCentral(
                { xa + 0.05f, Y_NAVE_CENTRAL, za - 0.05f },
                { xa + 5.75f, Y_NAVE_CENTRAL, za - 0.05f },
                { xa + 5.75f, Y_NAVE_CENTRAL, za - 5.45f },
                { xa + 0.05f, Y_NAVE_CENTRAL, za - 5.45f },
                TonoCentral(Color{ 74, 80, 74, 255 }, var)
            );
        }
    }

    QuadNieblaCentral(
        { -X_NAVE_CENTRAL, Y_NAVE_CENTRAL - 0.02f, z0 },
        { X_NAVE_CENTRAL, Y_NAVE_CENTRAL - 0.02f, z0 },
        { X_NAVE_CENTRAL, Y_NAVE_CENTRAL - 0.02f, z1 },
        { -X_NAVE_CENTRAL, Y_NAVE_CENTRAL - 0.02f, z1 },
        Color{ 52, 56, 54, 255 }
    );

    // Pasillos pintados entre turbinas.
    for (int k = -1; k <= 1; k += 2)
    {
        float xc = 6.5f * (float)k;
        QuadNieblaCentral(
            { xc - 1.4f, Y_NAVE_CENTRAL + 0.01f, z0 },
            { xc - 1.2f, Y_NAVE_CENTRAL + 0.01f, z0 },
            { xc - 1.2f, Y_NAVE_CENTRAL + 0.01f, z1 },
            { xc - 1.4f, Y_NAVE_CENTRAL + 0.01f, z1 },
            COLOR_PELIGRO_CENTRAL
        );
        QuadNieblaCentral(
            { xc + 1.2f, Y_NAVE_CENTRAL + 0.01f, z0 },
            { xc + 1.4f, Y_NAVE_CENTRAL + 0.01f, z0 },
            { xc + 1.4f, Y_NAVE_CENTRAL + 0.01f, z1 },
            { xc + 1.2f, Y_NAVE_CENTRAL + 0.01f, z1 },
            COLOR_PELIGRO_CENTRAL
        );
    }

    rlEnd();

    // Muro de ventanales del fondo: zocalo, parantes y travesanos.
    CuboCentral({ 0.0f, Y_NAVE_CENTRAL + 2.0f, z1 - 0.4f }, { 2.0f * X_NAVE_CENTRAL, 4.0f, 0.8f }, Color{ 96, 92, 86, 255 });
    CuboCentral({ 0.0f, 19.0f, z1 - 0.4f }, { 2.0f * X_NAVE_CENTRAL, 4.0f, 0.8f }, Color{ 80, 80, 78, 255 });

    for (int k = -8; k <= 8; k++)
    {
        float x = (float)k * 4.2f;
        CuboCentral({ x, 8.0f, z1 - 0.3f }, { 0.5f, 18.0f, 0.6f }, Color{ 62, 66, 72, 255 });
    }

    for (int k = 0; k < 3; k++)
    {
        CuboCentral({ 0.0f, 3.2f + 4.6f * (float)k, z1 - 0.3f }, { 2.0f * X_NAVE_CENTRAL, 0.3f, 0.5f }, Color{ 62, 66, 72, 255 });
    }

    // Muros laterales y columnas con lamparas de sodio.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = X_NAVE_CENTRAL * (float)lado;
        CuboCentral({ x + 0.5f * (float)lado, 6.0f, (z0 + z1) * 0.5f }, { 1.0f, 22.0f, z0 - z1 }, Color{ 66, 68, 74, 255 });

        for (int k = 0; k < 5; k++)
        {
            float z = z0 - 2.0f - 9.0f * (float)k;
            float cx = x - 0.6f * (float)lado;
            CuboCentral({ cx, 6.0f, z }, { 1.0f, 22.0f, 1.1f }, Color{ 56, 62, 74, 255 });
            CuboCentral({ cx - 0.5f * (float)lado, 12.2f, z }, { 1.2f, 0.4f, 1.6f }, COLOR_GRAFITO_CENTRAL);
        }

        // Riel del puente grua.
        CuboCentral({ x - 1.4f * (float)lado, 13.2f, (z0 + z1) * 0.5f }, { 0.9f, 0.9f, z0 - z1 }, COLOR_PELIGRO_CENTRAL);
    }

    // Puente grua que va y viene sobre las turbinas.
    {
        float z = ZPuenteGruaCentral(t);
        CuboCentral({ 0.0f, 14.0f, z - 1.2f }, { 2.0f * X_NAVE_CENTRAL - 2.0f, 1.3f, 0.9f }, COLOR_PELIGRO_CENTRAL);
        CuboCentral({ 0.0f, 14.0f, z + 1.2f }, { 2.0f * X_NAVE_CENTRAL - 2.0f, 1.3f, 0.9f }, COLOR_PELIGRO_CENTRAL);

        float xc = std::sin(t * 0.07f) * 8.0f;
        CuboCentral({ xc, 14.9f, z }, { 3.0f, 1.0f, 3.4f }, COLOR_GRAFITO_CENTRAL);
        float largo = 5.0f + std::sin(t * 0.23f) * 1.2f;
        VigaCentral({ xc - 0.2f, 14.4f, z }, { xc - 0.2f, 14.4f - largo, z }, 0.06f, Color{ 40, 40, 44, 255 });
        VigaCentral({ xc + 0.2f, 14.4f, z }, { xc + 0.2f, 14.4f - largo, z }, 0.06f, Color{ 40, 40, 44, 255 });
        CuboCentral({ xc, 14.2f - largo, z }, { 0.9f, 0.7f, 0.5f }, COLOR_PELIGRO_CENTRAL);
        CilindroEjeCentral({ xc, 13.85f - largo, z }, { xc, 13.3f - largo, z }, 0.12f, 0.2f, 6, COLOR_ACERO_CENTRAL);
    }

    for (int u = 0; u < 3; u++)
    {
        TurbogeneradorCentral(X_TURBINAS_CENTRAL[u], t, u, detalle);
    }

    if (!detalle)
    {
        return;
    }

    // Tableros locales y tambores de aceite junto a cada unidad.
    for (int u = 0; u < 3; u++)
    {
        float x = X_TURBINAS_CENTRAL[u] + 5.2f;
        CuboCentral({ x, Y_NAVE_CENTRAL + 1.1f, -20.0f }, { 1.4f, 2.2f, 0.8f }, COLOR_CONSOLA_CENTRAL);
        CuboCentral({ x, Y_NAVE_CENTRAL + 1.6f, -19.58f }, { 0.9f, 0.6f, 0.04f }, COLOR_CREMA_CENTRAL);

        for (int k = 0; k < 3; k++)
        {
            float bx = X_TURBINAS_CENTRAL[u] - 5.0f + 0.9f * (float)k;
            CilindroEjeCentral({ bx, Y_NAVE_CENTRAL, -44.0f }, { bx, Y_NAVE_CENTRAL + 1.3f, -44.0f }, 0.42f, 0.42f, 10, k == 1 ? Color{ 140, 96, 56, 255 } : Color{ 92, 104, 80, 255 });
        }
    }
}


//==================================================
// LA SALA: PISO, MUROS, VENTANAL Y CIELORRASO
//==================================================

// Piso tecnico de baldosas con juntas, variacion por baldosa, la franja
// de precaucion frente a la consola y la canaleta de cables.
static void DibujarPisoCentral()
{
    const float LADO = 1.5f;

    rlBegin(RL_QUADS);

    // Fondo oscuro: lo que asoma entre baldosas son las juntas.
    QuadNieblaCentral(
        { -X_MURO_CENTRAL, -0.01f, Z_FRENTE_CENTRAL },
        { X_MURO_CENTRAL, -0.01f, Z_FRENTE_CENTRAL },
        { X_MURO_CENTRAL, -0.01f, Z_VENTANAL_CENTRAL },
        { -X_MURO_CENTRAL, -0.01f, Z_VENTANAL_CENTRAL },
        COLOR_JUNTA_CENTRAL
    );

    for (int fx = -13; fx < 13; fx++)
    {
        for (int fz = 0; fz < 16; fz++)
        {
            float xa = (float)fx * LADO;
            float za = Z_VENTANAL_CENTRAL + (float)fz * LADO;
            float var = 0.9f + 0.18f * Hash01Central(fx + 50, fz + 7);
            Color baldosa = TonoCentral(COLOR_PISO_CENTRAL, var);

            QuadNieblaCentral(
                { xa + 0.04f, 0.0f, za + LADO - 0.04f },
                { xa + LADO - 0.04f, 0.0f, za + LADO - 0.04f },
                { xa + LADO - 0.04f, 0.0f, za + 0.04f },
                { xa + 0.04f, 0.0f, za + 0.04f },
                baldosa
            );

            // Una de cada tantas tiene rejilla de ventilacion.
            if (Hash01Central(fx + 90, fz + 3) > 0.9f)
            {
                for (int r = 0; r < 4; r++)
                {
                    float zr = za + 0.3f + 0.25f * (float)r;
                    QuadNieblaCentral(
                        { xa + 0.3f, 0.005f, zr + 0.1f },
                        { xa + LADO - 0.3f, 0.005f, zr + 0.1f },
                        { xa + LADO - 0.3f, 0.005f, zr },
                        { xa + 0.3f, 0.005f, zr },
                        COLOR_JUNTA_CENTRAL
                    );
                }
            }
        }
    }

    // Franja de precaucion a lo largo de la consola.
    const float Z0 = -1.85f;
    const float Z1 = -1.45f;

    for (int k = 0; k < 40; k++)
    {
        float x0 = -10.0f + 0.5f * (float)k;
        QuadNieblaCentral(
            { x0, 0.01f, Z1 },
            { x0 + 0.25f, 0.01f, Z1 },
            { x0 + 0.45f, 0.01f, Z0 },
            { x0 + 0.2f, 0.01f, Z0 },
            COLOR_PELIGRO_CENTRAL
        );
    }

    rlEnd();
}


// Muro trasero con el ventanal sobre la nave y muros laterales.
static void DibujarMurosCentral(bool detalle)
{
    float zv = Z_VENTANAL_CENTRAL;
    float ancho = 2.0f * X_MURO_CENTRAL;

    CuboCentral({ 0.0f, 1.22f, zv - 0.3f }, { ancho, 2.44f, 0.6f }, COLOR_MURO_CENTRAL);
    CuboCentral({ 0.0f, 0.15f, zv + 0.03f }, { ancho, 0.3f, 0.08f }, COLOR_GRAFITO_CENTRAL);
    CuboCentral({ 0.0f, 2.5f, zv + 0.05f }, { ancho, 0.12f, 0.5f }, COLOR_ACERO_CENTRAL);
    CuboCentral({ 0.0f, (Y_DINTEL_CENTRAL + Y_TECHO_CENTRAL) * 0.5f, zv - 0.3f }, { ancho, Y_TECHO_CENTRAL - Y_DINTEL_CENTRAL, 0.6f }, COLOR_MURO_CENTRAL);
    CuboCentral({ 0.0f, Y_DINTEL_CENTRAL + 0.05f, zv + 0.02f }, { ancho, 0.16f, 0.12f }, COLOR_GRAFITO_CENTRAL);

    // Parantes y travesano del ventanal.
    for (int k = -5; k <= 5; k++)
    {
        float x = (float)k * 3.6f;
        CuboCentral({ x, (2.56f + Y_DINTEL_CENTRAL) * 0.5f, zv - 0.15f }, { 0.28f, Y_DINTEL_CENTRAL - 2.56f, 0.4f }, COLOR_GRAFITO_CENTRAL);
    }

    CuboCentral({ 0.0f, 8.4f, zv - 0.15f }, { ancho, 0.16f, 0.36f }, COLOR_GRAFITO_CENTRAL);

    // Juntas de los paneles del zocalo.
    if (detalle)
    {
        for (int k = -9; k <= 9; k++)
        {
            CuboCentral({ (float)k * 2.0f + 1.0f, 1.3f, zv + 0.01f }, { 0.05f, 2.0f, 0.04f }, TonoCentral(COLOR_MURO_CENTRAL, 0.7f));
        }
    }

    // Muros laterales con zocalo y paneles.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = X_MURO_CENTRAL * (float)lado;
        float largo = Z_FRENTE_CENTRAL - zv;
        CuboCentral({ x + 0.3f * (float)lado, Y_TECHO_CENTRAL * 0.5f, (Z_FRENTE_CENTRAL + zv) * 0.5f }, { 0.6f, Y_TECHO_CENTRAL, largo }, COLOR_MURO_CENTRAL);
        CuboCentral({ x - 0.03f * (float)lado, 0.15f, (Z_FRENTE_CENTRAL + zv) * 0.5f }, { 0.08f, 0.3f, largo }, COLOR_GRAFITO_CENTRAL);

        for (int k = 0; k < 11 && detalle; k++)
        {
            CuboCentral({ x - 0.01f * (float)lado, 4.0f, zv + 1.0f + 2.2f * (float)k }, { 0.04f, 7.6f, 0.05f }, TonoCentral(COLOR_MURO_CENTRAL, 0.7f));
        }
    }

    // Losa del cielorraso.
    CuboCentral({ 0.0f, Y_TECHO_CENTRAL + 0.2f, (Z_FRENTE_CENTRAL + zv) * 0.5f }, { ancho, 0.4f, Z_FRENTE_CENTRAL - zv }, Color{ 58, 64, 76, 255 });
}


static const int FILAS_TUBOS_CENTRAL = 3;
static const float X_TUBOS_CENTRAL[4] = { -12.0f, -4.0f, 4.0f, 12.0f };
static const float Z_TUBOS_CENTRAL[FILAS_TUBOS_CENTRAL] = { -6.6f, -0.6f, 5.6f };
static const float Y_TUBOS_CENTRAL = 10.6f;


// Cielorraso: tubos fluorescentes, bandejas de cables y el aislador
// colgante de la barra colectora (de ahi salta el arco).
static void DibujarCielorrasoCentral(const MinijuegoInterruptoresCaos& m, bool detalle)
{
    float t = m.tiempoAnimacion;

    for (int fz = 0; fz < FILAS_TUBOS_CENTRAL; fz++)
    {
        for (int fx = 0; fx < 4; fx++)
        {
            float x = X_TUBOS_CENTRAL[fx];
            float z = Z_TUBOS_CENTRAL[fz];
            CuboCentral({ x, Y_TUBOS_CENTRAL + 0.12f, z }, { 2.8f, 0.24f, 0.8f }, COLOR_ACERO_CENTRAL);
            VigaCentral({ x - 1.1f, Y_TUBOS_CENTRAL + 0.24f, z }, { x - 1.1f, Y_TECHO_CENTRAL, z }, 0.04f, COLOR_GRAFITO_CENTRAL);
            VigaCentral({ x + 1.1f, Y_TUBOS_CENTRAL + 0.24f, z }, { x + 1.1f, Y_TECHO_CENTRAL, z }, 0.04f, COLOR_GRAFITO_CENTRAL);

            // Difusor encendido (titila en la sobrecarga).
            float titilar = m.sobrecarga > 0.0f && std::sin(t * 41.0f + (float)(fx * 3 + fz)) > 0.3f ? 0.35f : 1.0f;
            Color luz = TonoCentral(COLOR_TUBO_CENTRAL, titilar);
            rlBegin(RL_QUADS);
            QuadCentral(
                { x - 1.3f, Y_TUBOS_CENTRAL - 0.005f, z + 0.3f },
                { x + 1.3f, Y_TUBOS_CENTRAL - 0.005f, z + 0.3f },
                { x + 1.3f, Y_TUBOS_CENTRAL - 0.005f, z - 0.3f },
                { x - 1.3f, Y_TUBOS_CENTRAL - 0.005f, z - 0.3f },
                luz
            );
            QuadCentral(
                { x - 1.3f, Y_TUBOS_CENTRAL - 0.005f, z + 0.3f },
                { x + 1.3f, Y_TUBOS_CENTRAL - 0.005f, z + 0.3f },
                { x + 1.3f, Y_TUBOS_CENTRAL + 0.2f, z + 0.41f },
                { x - 1.3f, Y_TUBOS_CENTRAL + 0.2f, z + 0.41f },
                TonoCentral(luz, 0.8f)
            );
            rlEnd();
        }
    }

    // Bandejas de cables a lo largo de la sala.
    const Color cables[3] = { Color{ 38, 40, 46, 255 }, Color{ 150, 92, 52, 255 }, Color{ 84, 90, 100, 255 } };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = 8.2f * (float)lado;
        float z0 = Z_VENTANAL_CENTRAL + 0.4f;
        float z1 = Z_FRENTE_CENTRAL;
        float zc = (z0 + z1) * 0.5f;
        float y = 9.6f;

        CuboCentral({ x, y, zc }, { 1.2f, 0.06f, z1 - z0 }, COLOR_ACERO_CENTRAL);
        CuboCentral({ x - 0.58f, y + 0.12f, zc }, { 0.05f, 0.26f, z1 - z0 }, COLOR_ACERO_CENTRAL);
        CuboCentral({ x + 0.58f, y + 0.12f, zc }, { 0.05f, 0.26f, z1 - z0 }, COLOR_ACERO_CENTRAL);

        for (int c = 0; c < 3; c++)
        {
            float cx = x - 0.3f + 0.3f * (float)c;
            CilindroEjeCentral({ cx, y + 0.13f, z0 }, { cx, y + 0.13f, z1 }, 0.1f, 0.1f, 6, cables[c]);
        }

        for (int k = 0; k < 8 && detalle; k++)
        {
            float z = z0 + 0.5f + 3.0f * (float)k;
            VigaCentral({ x - 0.62f, y, z }, { x - 0.62f, Y_TECHO_CENTRAL, z }, 0.04f, COLOR_GRAFITO_CENTRAL);
            VigaCentral({ x + 0.62f, y, z }, { x + 0.62f, Y_TECHO_CENTRAL, z }, 0.04f, COLOR_GRAFITO_CENTRAL);
        }

        // Bajada de cables hasta el muro de monitores.
        for (int c = 0; c < 3; c++)
        {
            float cx = x - 0.3f + 0.3f * (float)c;
            CilindroEjeCentral({ cx, y + 0.13f, z0 + 0.2f }, { cx * 0.86f, 6.5f, Z_MONITORES_CENTRAL - 0.6f }, 0.09f, 0.09f, 6, cables[c]);
        }
    }

    // Barra colectora de cobre con tres columnas de aisladores.
    Vector3 a = AISLADOR_CENTRAL;
    VigaCentral({ a.x - 3.2f, a.y + 0.85f, a.z }, { a.x - 3.2f, Y_TECHO_CENTRAL, a.z }, 0.12f, COLOR_GRAFITO_CENTRAL);
    VigaCentral({ a.x + 3.2f, a.y + 0.85f, a.z }, { a.x + 3.2f, Y_TECHO_CENTRAL, a.z }, 0.12f, COLOR_GRAFITO_CENTRAL);
    CuboCentral({ a.x, a.y + 0.85f, a.z }, { 7.0f, 0.26f, 0.32f }, COLOR_COBRE_CENTRAL);

    for (int k = -1; k <= 1; k++)
    {
        float x = a.x + 2.4f * (float)k;
        float abajo = k == 0 ? a.y + 0.12f : a.y + 0.32f;
        int discos = k == 0 ? 5 : 4;

        for (int d = 0; d < discos; d++)
        {
            float y0 = a.y + 0.72f - 0.12f * (float)d;
            CilindroEjeCentral({ x, y0, a.z }, { x, y0 - 0.07f, a.z }, 0.3f, 0.3f, 10, d % 2 == 0 ? Color{ 214, 204, 178, 255 } : Color{ 120, 70, 48, 255 });
        }

        CilindroEjeCentral({ x, abajo + 0.12f, a.z }, { x, abajo, a.z }, 0.06f, 0.06f, 6, COLOR_COBRE_CENTRAL);
    }

    DrawSphereEx(a, 0.24f, 8, 8, TinteCentral(COLOR_COBRE_CENTRAL, a));
}


//==================================================
// UTILERIA DE LA SALA
//==================================================

// Rack electrico con frente de LEDs que titilan.
static void RackCentral(Vector3 base, float t, int semilla, bool detalle)
{
    CuboCentral({ base.x, base.y + 1.15f, base.z }, { 1.3f, 2.3f, 1.1f }, COLOR_GRAFITO_CENTRAL);
    float zf = base.z + 0.56f;

    rlBegin(RL_QUADS);
    QuadCentral({ base.x - 0.55f, base.y + 0.15f, zf }, { base.x + 0.55f, base.y + 0.15f, zf }, { base.x + 0.55f, base.y + 2.15f, zf }, { base.x - 0.55f, base.y + 2.15f, zf }, TinteCentral(Color{ 30, 34, 42, 255 }, base));

    if (detalle)
    {
        // Ranuras de equipos y sus LEDs.
        for (int f = 0; f < 8; f++)
        {
            float y = base.y + 0.3f + 0.24f * (float)f;
            QuadCentral({ base.x - 0.5f, y, zf + 0.005f }, { base.x + 0.5f, y, zf + 0.005f }, { base.x + 0.5f, y + 0.18f, zf + 0.005f }, { base.x - 0.5f, y + 0.18f, zf + 0.005f }, TinteCentral(Color{ 56, 62, 74, 255 }, base));

            for (int c = 0; c < 4; c++)
            {
                float h = Hash01Central(semilla * 31 + f, c);
                bool prendido = std::sin(t * (1.5f + 5.0f * h) + h * 40.0f) > -0.2f;
                Color led = h > 0.75f ? Color{ 255, 180, 60, 255 } : (h > 0.3f ? Color{ 110, 236, 150, 255 } : Color{ 110, 200, 255, 255 });
                if (!prendido) led = Color{ 40, 46, 52, 255 };
                float x = base.x + 0.08f + 0.1f * (float)c;
                QuadCentral({ x, y + 0.06f, zf + 0.01f }, { x + 0.05f, y + 0.06f, zf + 0.01f }, { x + 0.05f, y + 0.11f, zf + 0.01f }, { x, y + 0.11f, zf + 0.01f }, led);
            }
        }
    }

    rlEnd();
}


// Escritorio lateral con monitor de tubo, teclado, taza y papeles.
static void EscritorioCentral(Vector3 base, float lado, float t, bool detalle)
{
    Color tapa = Color{ 186, 180, 160, 255 };
    CuboCentral({ base.x, base.y + 0.86f, base.z }, { 3.6f, 0.1f, 1.6f }, tapa);
    CuboCentral({ base.x + 1.2f * lado, base.y + 0.41f, base.z }, { 1.0f, 0.82f, 1.4f }, COLOR_CONSOLA_CENTRAL);
    CuboCentral({ base.x - 1.6f * lado, base.y + 0.41f, base.z }, { 0.1f, 0.82f, 1.4f }, COLOR_CONSOLA_CENTRAL);
    CuboCentral({ base.x, base.y + 0.5f, base.z - 0.7f }, { 3.4f, 0.66f, 0.06f }, TonoCentral(COLOR_CONSOLA_CENTRAL, 0.85f));

    // Cajones con manijas.
    for (int k = 0; k < 3 && detalle; k++)
    {
        float y = base.y + 0.2f + 0.24f * (float)k;
        CuboCentral({ base.x + 1.2f * lado, y, base.z + 0.71f }, { 0.4f, 0.03f, 0.03f }, COLOR_ACERO_CENTRAL);
    }

    // Monitor de tubo.
    Vector3 mon = { base.x - 0.35f * lado, base.y + 1.36f, base.z - 0.3f };
    CuboCentral(mon, { 1.1f, 0.88f, 0.9f }, Color{ 168, 164, 148, 255 });
    CuboCentral({ mon.x, mon.y - 0.06f, mon.z - 0.55f }, { 0.8f, 0.6f, 0.3f }, Color{ 140, 136, 124, 255 });
    float zp = mon.z + 0.455f;
    PlacaFrenteCentral(mon.x - 0.42f, mon.y - 0.32f, mon.x + 0.42f, mon.y + 0.32f, zp, Color{ 14, 34, 36, 255 });

    if (detalle)
    {
        // Barras que suben y bajan en la pantalla.
        for (int b = 0; b < 6; b++)
        {
            float alto = 0.12f + 0.4f * (0.5f + 0.5f * std::sin(t * (1.2f + 0.3f * (float)b) + (float)b * 1.3f + lado));
            float x0 = mon.x - 0.36f + 0.125f * (float)b;
            PlacaFrenteCentral(x0, mon.y - 0.27f, x0 + 0.08f, mon.y - 0.27f + alto, zp + 0.005f, Color{ 110, 236, 170, 255 });
        }
    }

    // Teclado, taza y papeles.
    CuboCentral({ mon.x, base.y + 0.93f, base.z + 0.35f }, { 0.95f, 0.05f, 0.32f }, Color{ 150, 146, 132, 255 });

    if (detalle)
    {
        Vector3 taza = { base.x + 1.0f * lado, base.y + 0.91f, base.z + 0.3f };
        CilindroEjeCentral(taza, { taza.x, taza.y + 0.18f, taza.z }, 0.08f, 0.08f, 8, Color{ 228, 224, 212, 255 });
        CuboGiradoCentral({ base.x + 0.6f * lado, base.y + 0.915f, base.z + 0.15f }, { 0.42f, 0.01f, 0.56f }, 14.0f * lado, Color{ 236, 234, 226, 255 });
        CuboGiradoCentral({ base.x + 0.66f * lado, base.y + 0.925f, base.z + 0.12f }, { 0.42f, 0.01f, 0.56f }, -8.0f * lado, Color{ 226, 222, 204, 255 });
        CuboGiradoCentral({ base.x + 1.25f * lado, base.y + 0.93f, base.z - 0.35f }, { 0.34f, 0.04f, 0.5f }, 0.0f, Color{ 40, 42, 48, 255 });
    }
}


// Silla de operador con base de cinco patas.
static void SillaCentral(Vector3 base, float giro, bool detalle)
{
    Color tela = Color{ 48, 64, 66, 255 };

    if (detalle)
    {
        for (int k = 0; k < 5; k++)
        {
            float a = (giro + 72.0f * (float)k) * DEG2RAD;
            VigaCentral({ base.x, base.y + 0.1f, base.z }, { base.x + std::sin(a) * 0.42f, base.y + 0.05f, base.z + std::cos(a) * 0.42f }, 0.06f, COLOR_GRAFITO_CENTRAL);
        }
    }

    CilindroEjeCentral({ base.x, base.y + 0.1f, base.z }, { base.x, base.y + 0.55f, base.z }, 0.05f, 0.05f, 6, COLOR_ACERO_CENTRAL);
    CuboGiradoCentral({ base.x, base.y + 0.62f, base.z }, { 0.72f, 0.14f, 0.7f }, giro, tela);

    float a = giro * DEG2RAD;
    Vector3 respaldo = { base.x + std::sin(a) * 0.36f, base.y + 1.05f, base.z + std::cos(a) * 0.36f };
    CuboGiradoCentral(respaldo, { 0.66f, 0.72f, 0.12f }, giro, tela);
}


// Bobina de madera con cable enrollado (acostada, eje en x).
static void BobinaCentral(Vector3 base, float radio, Color cable)
{
    float y = base.y + radio;
    float medio = radio * 0.55f;
    Color madera = Color{ 150, 108, 66, 255 };

    CilindroEjeCentral({ base.x - medio - 0.08f, y, base.z }, { base.x - medio, y, base.z }, radio, radio, 14, madera);
    CilindroEjeCentral({ base.x - medio, y, base.z }, { base.x + medio, y, base.z }, radio * 0.74f, radio * 0.74f, 14, cable);
    CilindroEjeCentral({ base.x + medio, y, base.z }, { base.x + medio + 0.08f, y, base.z }, radio, radio, 14, madera);

    // Punta suelta del cable sobre el piso.
    VigaCentral({ base.x + medio * 0.5f, y - radio * 0.7f, base.z + radio * 0.3f }, { base.x + medio * 1.6f, 0.05f, base.z + radio * 1.4f }, 0.1f, cable);
}


// Armario de cortes con manija giratoria y cartel de peligro.
static void ArmarioCortesCentral(Vector3 base, bool detalle)
{
    CuboCentral({ base.x, base.y + 1.2f, base.z }, { 2.0f, 2.4f, 1.0f }, Color{ 150, 152, 144, 255 });
    float zf = base.z + 0.51f;
    CuboCentral({ base.x - 0.5f, base.y + 1.2f, zf }, { 0.03f, 2.2f, 0.02f }, TonoCentral(Color{ 150, 152, 144, 255 }, 0.6f));

    // Cartel triangular de riesgo electrico.
    Vector3 c = { base.x + 0.35f, base.y + 1.75f, zf + 0.01f };
    rlBegin(RL_TRIANGLES);
    VerticeCentral({ c.x - 0.32f, c.y - 0.26f, c.z }, TinteCentral(COLOR_PELIGRO_CENTRAL, c));
    VerticeCentral({ c.x + 0.32f, c.y - 0.26f, c.z }, TinteCentral(COLOR_PELIGRO_CENTRAL, c));
    VerticeCentral({ c.x, c.y + 0.3f, c.z }, TinteCentral(COLOR_PELIGRO_CENTRAL, c));
    VerticeCentral({ c.x + 0.02f, c.y + 0.14f, c.z + 0.005f }, COLOR_GRAFITO_CENTRAL);
    VerticeCentral({ c.x - 0.08f, c.y - 0.04f, c.z + 0.005f }, COLOR_GRAFITO_CENTRAL);
    VerticeCentral({ c.x + 0.04f, c.y - 0.02f, c.z + 0.005f }, COLOR_GRAFITO_CENTRAL);
    VerticeCentral({ c.x - 0.02f, c.y + 0.0f, c.z + 0.005f }, COLOR_GRAFITO_CENTRAL);
    VerticeCentral({ c.x + 0.06f, c.y - 0.03f, c.z + 0.005f }, COLOR_GRAFITO_CENTRAL);
    VerticeCentral({ c.x - 0.03f, c.y - 0.2f, c.z + 0.005f }, COLOR_GRAFITO_CENTRAL);
    rlEnd();

    // Manija rotativa.
    Vector3 eje = { base.x + 0.35f, base.y + 1.0f, zf };
    CilindroEjeCentral(eje, { eje.x, eje.y, eje.z + 0.15f }, 0.24f, 0.24f, 10, COLOR_GRAFITO_CENTRAL);
    VigaCentral({ eje.x, eje.y, eje.z + 0.2f }, { eje.x + 0.1f, eje.y - 0.36f, eje.z + 0.2f }, 0.1f, COLOR_PELIGRO_CENTRAL);

    // Empalme flojo arriba, con su caja y cables.
    CuboCentral({ EMPALME_CENTRAL.x, base.y + 2.52f, EMPALME_CENTRAL.z }, { 0.5f, 0.24f, 0.36f }, COLOR_GRAFITO_CENTRAL);

    if (detalle)
    {
        for (int k = 0; k < 6; k++)
        {
            CuboCentral({ base.x - 0.9f + 0.36f * (float)k, base.y + 2.25f, base.z - 0.5f }, { 0.05f, 0.3f, 0.06f }, TonoCentral(COLOR_GRAFITO_CENTRAL, 1.2f));
        }

        CilindroEjeCentral({ EMPALME_CENTRAL.x - 0.15f, base.y + 2.6f, EMPALME_CENTRAL.z }, { EMPALME_CENTRAL.x - 1.4f, 9.6f, Z_VENTANAL_CENTRAL + 0.6f }, 0.07f, 0.07f, 6, Color{ 38, 40, 46, 255 });
        CilindroEjeCentral({ EMPALME_CENTRAL.x + 0.15f, base.y + 2.6f, EMPALME_CENTRAL.z }, { EMPALME_CENTRAL.x + 0.6f, 9.6f, Z_VENTANAL_CENTRAL + 0.6f }, 0.07f, 0.07f, 6, Color{ 150, 92, 52, 255 });
    }
}


// Casilleros de los operarios.
static void CasillerosCentral(Vector3 base, int cantidad, bool detalle)
{
    for (int k = 0; k < cantidad; k++)
    {
        float x = base.x + 0.82f * (float)k;
        Color chapa = TonoCentral(Color{ 104, 126, 122, 255 }, 0.92f + 0.12f * Hash01Central(k, 60));
        CuboCentral({ x, base.y + 1.2f, base.z }, { 0.8f, 2.4f, 0.8f }, chapa);

        if (detalle)
        {
            for (int r = 0; r < 4; r++)
            {
                CuboCentral({ x, base.y + 1.9f + 0.08f * (float)r, base.z + 0.41f }, { 0.4f, 0.03f, 0.02f }, TonoCentral(chapa, 0.6f));
            }

            CuboCentral({ x + 0.28f, base.y + 1.2f, base.z + 0.42f }, { 0.05f, 0.18f, 0.04f }, COLOR_ACERO_CENTRAL);
        }
    }
}


// Pizarra con el esquema unifilar de la central.
static void PizarraCentral(Vector3 base, float giro)
{
    float a = giro * DEG2RAD;
    Vector3 ex = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 ez = { std::sin(a), 0.0f, std::cos(a) };

    VigaCentral(Vector3Add(base, Vector3Scale(ex, -0.9f)), Vector3Add(Vector3Add(base, Vector3Scale(ex, -0.9f)), { 0.0f, 2.4f, 0.0f }), 0.07f, COLOR_ACERO_CENTRAL);
    VigaCentral(Vector3Add(base, Vector3Scale(ex, 0.9f)), Vector3Add(Vector3Add(base, Vector3Scale(ex, 0.9f)), { 0.0f, 2.4f, 0.0f }), 0.07f, COLOR_ACERO_CENTRAL);
    CuboGiradoCentral({ base.x, base.y + 1.65f, base.z }, { 1.9f, 1.2f, 0.06f }, giro, Color{ 228, 230, 226, 255 });

    // Trazos del esquema (un poco delante de la pizarra).
    Vector3 c = Vector3Add({ base.x, base.y + 1.65f, base.z }, Vector3Scale(ez, 0.04f));
    Color tinta = Color{ 40, 70, 110, 255 };
    auto Punto = [&](float u, float v) { return Vector3Add(Vector3Add(c, Vector3Scale(ex, u)), { 0.0f, v, 0.0f }); };

    DrawLine3D(Punto(-0.75f, 0.35f), Punto(0.75f, 0.35f), tinta);
    DrawLine3D(Punto(-0.5f, 0.35f), Punto(-0.5f, -0.3f), tinta);
    DrawLine3D(Punto(0.0f, 0.35f), Punto(0.0f, -0.3f), tinta);
    DrawLine3D(Punto(0.5f, 0.35f), Punto(0.5f, -0.3f), tinta);
    DrawLine3D(Punto(-0.75f, -0.3f), Punto(0.75f, -0.3f), tinta);
    DrawLine3D(Punto(-0.6f, 0.0f), Punto(-0.4f, 0.1f), Color{ 200, 120, 40, 255 });
    DrawLine3D(Punto(0.1f, 0.0f), Punto(-0.1f, 0.1f), tinta);
    DrawLine3D(Punto(0.4f, 0.0f), Punto(0.6f, 0.1f), tinta);
}


// Carro de herramientas con cajones.
static void CarroCentral(Vector3 base, float giro, bool detalle)
{
    Color chapa = Color{ 70, 112, 118, 255 };
    CuboGiradoCentral({ base.x, base.y + 0.55f, base.z }, { 1.0f, 0.9f, 0.6f }, giro, chapa);
    CuboGiradoCentral({ base.x, base.y + 1.02f, base.z }, { 1.06f, 0.05f, 0.64f }, giro, COLOR_GRAFITO_CENTRAL);

    if (detalle)
    {
        CuboGiradoCentral({ base.x - 0.2f, base.y + 1.12f, base.z }, { 0.36f, 0.16f, 0.2f }, giro + 10.0f, COLOR_PELIGRO_CENTRAL);
        VigaCentral({ base.x + 0.1f, base.y + 1.06f, base.z + 0.1f }, { base.x + 0.42f, base.y + 1.06f, base.z - 0.02f }, 0.05f, COLOR_ACERO_CENTRAL);
    }

    for (int k = 0; k < 4; k++)
    {
        float sx = (k & 1) ? 0.4f : -0.4f;
        float sz = (k & 2) ? 0.22f : -0.22f;
        CilindroEjeCentral({ base.x + sx - 0.03f, base.y + 0.06f, base.z + sz }, { base.x + sx + 0.03f, base.y + 0.06f, base.z + sz }, 0.06f, 0.06f, 6, COLOR_GRAFITO_CENTRAL);
    }
}


// Baliza giratoria (la luz se agrega en la pasada aditiva).
static const Vector3 BALIZAS_CENTRAL[4] =
{
    { -13.5f, 9.95f, -9.55f },
    { 13.5f, 9.95f, -9.55f },
    { -X_MURO_CENTRAL + 0.4f, 7.6f, -1.0f },
    { X_MURO_CENTRAL - 0.4f, 7.6f, -1.0f }
};


static Color ColorBalizaCentral(const MinijuegoInterruptoresCaos& m)
{
    return m.sobrecarga > 0.0f || m.fase == FASE_INTERRUPTORES_EXPLOSION ? COLOR_ALARMA_CENTRAL : COLOR_ACENTO_CENTRAL;
}


static void DibujarBalizasCentral(const MinijuegoInterruptoresCaos& m)
{
    for (int k = 0; k < 4; k++)
    {
        Vector3 b = BALIZAS_CENTRAL[k];
        CuboCentral({ b.x, b.y - 0.12f, b.z }, { 0.5f, 0.2f, 0.5f }, COLOR_GRAFITO_CENTRAL);
        CilindroEjeCentral({ b.x, b.y, b.z }, { b.x, b.y + 0.42f, b.z }, 0.24f, 0.18f, 10, MezclaCentral(ColorBalizaCentral(m), WHITE, 0.2f), false);
    }
}


static void DibujarUtileriaCentral(const MinijuegoInterruptoresCaos& m, bool detalle)
{
    float t = m.tiempoAnimacion;

    // Fila de racks a la derecha del muro de monitores.
    for (int k = 0; k < 6; k++)
    {
        RackCentral({ 9.9f + 1.38f * (float)k, 0.0f, -9.3f }, t, k, detalle);
    }

    // A la izquierda: un rack, el armario de cortes y los casilleros.
    RackCentral({ -9.9f, 0.0f, -9.3f }, t, 9, detalle);
    ArmarioCortesCentral({ -12.4f, 0.0f, -8.8f }, detalle);
    CasillerosCentral({ -17.2f, 0.0f, -9.1f }, 4, detalle);

    // Escritorios laterales con sus sillas.
    EscritorioCentral({ -12.6f, 0.0f, -3.6f }, -1.0f, t, detalle);
    EscritorioCentral({ 12.6f, 0.0f, -3.6f }, 1.0f, t, detalle);
    SillaCentral({ -12.2f, 0.0f, -1.9f }, 200.0f, detalle);
    SillaCentral({ 12.9f, 0.0f, -2.0f }, 155.0f, detalle);

    DibujarBalizasCentral(m);

    if (!detalle)
    {
        return;
    }

    BobinaCentral({ -10.8f, 0.0f, 3.4f }, 0.8f, Color{ 40, 42, 48, 255 });
    BobinaCentral({ 11.8f, 0.0f, 2.4f }, 0.65f, Color{ 150, 92, 52, 255 });
    BobinaCentral({ 15.6f, 0.0f, -6.2f }, 0.7f, Color{ 84, 90, 100, 255 });
    PizarraCentral({ -16.0f, 0.0f, -4.6f }, 32.0f);
    CarroCentral({ 10.6f, 0.0f, 0.6f }, -18.0f, detalle);

    // Conos de seguridad junto a la canaleta abierta.
    for (int k = 0; k < 2; k++)
    {
        float x = -10.4f + 0.9f * (float)k;
        CilindroEjeCentral({ x, 0.0f, 0.4f }, { x, 0.7f, 0.4f }, 0.22f, 0.03f, 8, COLOR_PELIGRO_CENTRAL);
        CuboCentral({ x, 0.03f, 0.4f }, { 0.5f, 0.06f, 0.5f }, COLOR_GRAFITO_CENTRAL);
    }

    CuboCentral({ -11.6f, 0.02f, 0.4f }, { 1.4f, 0.04f, 0.6f }, COLOR_JUNTA_CENTRAL);
}


//==================================================
// CONSOLA DE PALANCAS
//==================================================

static const float X_CONSOLA_CENTRAL = 9.4f;        // medio ancho


// Estado de una palanca para colorear perilla, franja y lampara.
enum EstadoPalancaCentral
{
    PALANCA_CENTRAL_LIBRE = 0,
    PALANCA_CENTRAL_ELEGIDA,
    PALANCA_CENTRAL_SEGURA,
    PALANCA_CENTRAL_FATAL
};


static EstadoPalancaCentral EstadoPalancaInterruptores(const MinijuegoInterruptoresCaos& m, int i)
{
    bool actual = i == m.interruptorSeleccionado;

    if (actual && m.fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        return PALANCA_CENTRAL_FATAL;
    }

    if (m.interruptoresUsados[i] || (actual && m.fase == FASE_INTERRUPTORES_SEGURO))
    {
        return PALANCA_CENTRAL_SEGURA;
    }

    if (actual && m.fase == FASE_INTERRUPTORES_ELECCION)
    {
        return PALANCA_CENTRAL_ELEGIDA;
    }

    return PALANCA_CENTRAL_LIBRE;
}


static Color ColorLamparaCentral(const MinijuegoInterruptoresCaos& m, int i)
{
    float t = m.tiempoAnimacion;

    switch (EstadoPalancaInterruptores(m, i))
    {
        case PALANCA_CENTRAL_ELEGIDA:
            return MezclaCentral(COLOR_ACENTO_CENTRAL, Color{ 255, 240, 200, 255 }, 0.3f + 0.3f * std::sin(t * 8.0f));
        case PALANCA_CENTRAL_SEGURA:
            return COLOR_SEGURO_CENTRAL;
        case PALANCA_CENTRAL_FATAL:
            return std::sin(t * 30.0f) > 0.0f ? COLOR_ALARMA_CENTRAL : Color{ 255, 236, 220, 255 };
        case PALANCA_CENTRAL_LIBRE:
            break;
    }

    return Color{ 200, 186, 150, 255 };
}


static void DibujarConsolaCentral(const MinijuegoInterruptoresCaos& m, bool detalle)
{
    float t = m.tiempoAnimacion;
    float ancho = 2.0f * X_CONSOLA_CENTRAL;
    float zf = -2.4f;

    // Cuerpo, zocalo, tapa de goma y canto cromado.
    CuboCentral({ 0.0f, 0.72f, -3.75f }, { ancho, 1.3f, 2.7f }, COLOR_CONSOLA_CENTRAL);
    CuboCentral({ 0.0f, 0.06f, -3.6f }, { ancho - 0.2f, 0.12f, 2.5f }, COLOR_GRAFITO_CENTRAL);
    CuboCentral({ 0.0f, 1.41f, -3.6f }, { ancho + 0.3f, 0.08f, 3.1f }, Color{ 60, 66, 76, 255 });
    CuboCentral({ 0.0f, 1.37f, -2.04f }, { ancho + 0.3f, 0.07f, 0.07f }, COLOR_ACERO_CENTRAL);

    // Tapas de los extremos.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        CuboCentral({ (X_CONSOLA_CENTRAL + 0.15f) * (float)lado, 1.45f, -3.7f }, { 0.3f, 2.9f, 3.2f }, COLOR_GRAFITO_CENTRAL);
    }

    // Banda trasera de instrumentos.
    CuboCentral({ 0.0f, 2.05f, -4.7f }, { ancho, 1.2f, 0.5f }, TonoCentral(COLOR_CONSOLA_CENTRAL, 0.92f));
    CuboCentral({ 0.0f, 2.68f, -4.7f }, { ancho + 0.1f, 0.06f, 0.6f }, COLOR_GRAFITO_CENTRAL);

    // Frente: ocho paneles con botoneras, rejillas y medidores chicos.
    const int PANELES = 8;
    float anchoPanel = (ancho - 0.2f) / (float)PANELES;

    for (int p = 0; p < PANELES; p++)
    {
        float x0 = -X_CONSOLA_CENTRAL + 0.1f + anchoPanel * (float)p;
        float x1 = x0 + anchoPanel;
        float cx = (x0 + x1) * 0.5f;
        Vector3 c = { cx, 0.8f, zf };

        PlacaFrenteCentral(x0 + 0.02f, 0.2f, x0 + 0.06f, 1.3f, zf + 0.004f, TinteCentral(TonoCentral(COLOR_CONSOLA_CENTRAL, 0.6f), c));

        if (!detalle && p % 2 == 1)
        {
            continue;
        }

        int tipo = p % 3;

        if (tipo == 0)
        {
            // Botonera: tres filas de pulsadores, algunos encendidos.
            for (int f = 0; f < 3; f++)
            {
                for (int k = 0; k < 6; k++)
                {
                    float h = Hash01Central(p * 7 + f, k);
                    float bx = x0 + 0.42f + 0.28f * (float)k;
                    float by = 0.55f + 0.24f * (float)f;
                    bool prendido = std::sin(t * (0.8f + 2.0f * h) + h * 30.0f) > 0.2f;
                    Color boton = h > 0.66f ? Color{ 236, 170, 70, 255 } : (h > 0.33f ? Color{ 120, 220, 200, 255 } : Color{ 226, 220, 200, 255 });
                    if (!prendido) boton = TonoCentral(boton, 0.4f);
                    CuboPuroCentral({ bx, by, zf + 0.04f }, { 0.14f, 0.12f, 0.08f }, boton);
                }
            }
        }
        else if (tipo == 1)
        {
            // Rejilla de ventilacion.
            for (int r = 0; r < 7; r++)
            {
                float y = 0.4f + 0.11f * (float)r;
                PlacaFrenteCentral(cx - 0.8f, y, cx + 0.8f, y + 0.05f, zf + 0.004f, TinteCentral(COLOR_GRAFITO_CENTRAL, c));
            }
        }
        else
        {
            // Medidor de aguja en caja crema.
            PlacaFrenteCentral(cx - 0.55f, 0.55f, cx + 0.55f, 1.15f, zf + 0.004f, TinteCentral(COLOR_CREMA_CENTRAL, c));
            ArcoFrenteCentral({ cx, 0.68f, zf + 0.008f }, 0.34f, 0.38f, 150.0f, 30.0f, TinteCentral(COLOR_GRAFITO_CENTRAL, c), 8);
            float a = (110.0f - 40.0f * std::sin(t * 1.3f + (float)p) - 30.0f * m.tensionVista) * DEG2RAD;
            TrazoFrenteCentral({ cx, 0.68f }, { cx + std::cos(a) * 0.36f, 0.68f + std::sin(a) * 0.36f }, zf + 0.012f, 0.025f, COLOR_GRAFITO_CENTRAL);
        }
    }

    if (!detalle)
    {
        return;
    }

    // Llaves de palanca chica y LEDs en la banda trasera.
    for (int k = 0; k < 44; k++)
    {
        float x = -X_CONSOLA_CENTRAL + 0.3f + 0.42f * (float)k;
        float h = Hash01Central(k, 77);
        CuboPuroCentral({ x, 2.4f, -4.43f }, { 0.08f, 0.08f, 0.04f }, TinteCentral(COLOR_CREMA_CENTRAL, { x, 2.4f, -4.4f }));
        VigaCentral({ x, 2.4f, -4.4f }, { x, 2.4f + (h > 0.5f ? 0.1f : -0.1f), -4.32f }, 0.03f, COLOR_ACERO_CENTRAL);

        bool prendido = std::sin(t * (1.0f + 3.0f * h) + h * 20.0f) > 0.0f;
        Color led = h > 0.8f ? Color{ 255, 180, 60, 255 } : Color{ 110, 236, 170, 255 };
        CuboPuroCentral({ x, 1.8f, -4.44f }, { 0.07f, 0.07f, 0.03f }, prendido ? led : TonoCentral(led, 0.3f));
    }
}


// Tablero vertical de cada palanca: dial con aguja y lampara de estado.
static void DibujarTablerosCentral(const MinijuegoInterruptoresCaos& m)
{
    float t = m.tiempoAnimacion;

    for (int i = 0; i < m.cantidadInterruptores; i++)
    {
        float s = EscalaAparicionCentral(m, i);

        if (s <= 0.01f)
        {
            continue;
        }

        float x = PosicionXPalanca(m, i);
        float alto = 1.6f * s;
        float y0 = 2.65f;
        float zf = Z_TABLEROS_CENTRAL;
        Vector3 c = { x, y0 + alto * 0.5f, zf - 0.15f };

        CuboCentral(c, { 2.3f, alto, 0.3f }, COLOR_CREMA_CENTRAL);
        CuboCentral({ x, y0 + alto + 0.04f, zf - 0.15f }, { 2.4f, 0.08f, 0.36f }, COLOR_GRAFITO_CENTRAL);

        if (s < 0.6f)
        {
            continue;
        }

        // Dial: bisel, cara, escala de colores y aguja.
        EstadoPalancaCentral estado = EstadoPalancaInterruptores(m, i);
        Vector3 dial = { x - 0.35f, y0 + 0.78f * s, zf + 0.005f };
        ArcoFrenteCentral(dial, 0.0f, 0.5f, 0.0f, 360.0f, TinteCentral(COLOR_GRAFITO_CENTRAL, c), 16);
        ArcoFrenteCentral({ dial.x, dial.y, dial.z + 0.004f }, 0.0f, 0.43f, 0.0f, 360.0f, TinteCentral(Color{ 240, 236, 222, 255 }, c), 16);
        ArcoFrenteCentral({ dial.x, dial.y, dial.z + 0.008f }, 0.31f, 0.38f, 210.0f, 110.0f, Color{ 90, 190, 110, 255 }, 6);
        ArcoFrenteCentral({ dial.x, dial.y, dial.z + 0.008f }, 0.31f, 0.38f, 110.0f, 30.0f, Color{ 230, 160, 50, 255 }, 5);
        ArcoFrenteCentral({ dial.x, dial.y, dial.z + 0.008f }, 0.31f, 0.38f, 30.0f, -30.0f, Color{ 220, 70, 56, 255 }, 4);

        float lectura = 0.25f + 0.04f * std::sin(t * 2.0f + (float)i);
        if (estado == PALANCA_CENTRAL_SEGURA) lectura = 0.12f;
        if (estado == PALANCA_CENTRAL_ELEGIDA) lectura = 0.5f + 0.08f * std::sin(t * 9.0f);
        if (estado == PALANCA_CENTRAL_FATAL) lectura = 0.98f + 0.03f * std::sin(t * 50.0f);

        float a = (210.0f - 240.0f * lectura) * DEG2RAD;
        TrazoFrenteCentral({ dial.x, dial.y }, { dial.x + std::cos(a) * 0.36f, dial.y + std::sin(a) * 0.36f }, dial.z + 0.012f, 0.04f, Color{ 40, 36, 40, 255 });
        ArcoFrenteCentral({ dial.x, dial.y, dial.z + 0.016f }, 0.0f, 0.06f, 0.0f, 360.0f, Color{ 40, 36, 40, 255 }, 8);

        // Ventanita con barras de carga y placa de numero.
        PlacaFrenteCentral(x + 0.1f, y0 + 0.5f * s, x + 0.95f, y0 + 1.1f * s, zf + 0.005f, TinteCentral(Color{ 18, 30, 34, 255 }, c));

        for (int b = 0; b < 4; b++)
        {
            float h = 0.1f + 0.4f * (0.5f + 0.5f * std::sin(t * (2.0f + (float)b) + (float)(i * 3 + b)));
            float bx = x + 0.18f + 0.19f * (float)b;
            PlacaFrenteCentral(bx, y0 + 0.55f * s, bx + 0.12f, y0 + (0.55f + h) * s, zf + 0.01f, estado == PALANCA_CENTRAL_FATAL ? COLOR_ALARMA_CENTRAL : COLOR_PANTALLA_CENTRAL);
        }

        // Lampara de estado arriba del tablero.
        Vector3 lampara = { x, y0 + alto + 0.08f, zf - 0.15f };
        Color luz = ColorLamparaCentral(m, i);
        CilindroEjeCentral(lampara, { lampara.x, lampara.y + 0.12f, lampara.z }, 0.3f, 0.3f, 10, COLOR_GRAFITO_CENTRAL);
        CilindroEjeCentral({ lampara.x, lampara.y + 0.12f, lampara.z }, { lampara.x, lampara.y + 0.42f, lampara.z }, 0.26f, 0.16f, 10, luz, false);
    }
}


// Palancas: caja con guias, franja de estado, mango y perilla.
static void DibujarPalancasCentral(const MinijuegoInterruptoresCaos& m)
{
    float t = m.tiempoAnimacion;

    for (int i = 0; i < m.cantidadInterruptores; i++)
    {
        float s = EscalaAparicionCentral(m, i);

        if (s <= 0.01f)
        {
            continue;
        }

        float x = PosicionXPalanca(m, i);
        float z = Z_PALANCAS_INTERRUPTORES;
        EstadoPalancaCentral estado = EstadoPalancaInterruptores(m, i);

        CuboCentral({ x, ALTURA_BASE_PALANCA + 0.11f * s, z }, { 1.0f * s, 0.22f * s, 1.25f * s }, COLOR_GRAFITO_CENTRAL);
        CuboCentral({ x - 0.24f * s, ALTURA_BASE_PALANCA + 0.36f * s, z + 0.12f }, { 0.07f, 0.36f * s, 0.95f * s }, COLOR_ACERO_CENTRAL);
        CuboCentral({ x + 0.24f * s, ALTURA_BASE_PALANCA + 0.36f * s, z + 0.12f }, { 0.07f, 0.36f * s, 0.95f * s }, COLOR_ACERO_CENTRAL);

        // Franja de estado a cada lado de la ranura.
        Color franja = ColorLamparaCentral(m, i);
        if (estado == PALANCA_CENTRAL_LIBRE) franja = Color{ 150, 140, 116, 255 };
        CuboPuroCentral({ x - 0.38f * s, ALTURA_BASE_PALANCA + 0.225f * s, z }, { 0.12f * s, 0.02f, 1.0f * s }, franja);
        CuboPuroCentral({ x + 0.38f * s, ALTURA_BASE_PALANCA + 0.225f * s, z }, { 0.12f * s, 0.02f, 1.0f * s }, franja);

        // Mango: varilla de acero, puño de goma, anillo de estado y bocha.
        Vector3 pivote = PivotePalancaCentral(m, i);
        float largo = LARGO_MANGO_CENTRAL * s;
        Vector3 puno = PuntaPalancaCentral(m, i, largo * 0.72f);
        Vector3 anillo = PuntaPalancaCentral(m, i, largo * 0.66f);
        Vector3 punta = PuntaPalancaCentral(m, i, largo);

        CilindroEjeCentral(pivote, puno, 0.07f, 0.07f, 8, Color{ 176, 180, 188, 255 });
        CilindroEjeCentral(anillo, puno, 0.19f, 0.19f, 10, franja, false);
        CilindroEjeCentral(puno, punta, 0.17f, 0.17f, 10, Color{ 34, 34, 40, 255 });

        Color bocha = Color{ 46, 46, 52, 255 };
        if (estado == PALANCA_CENTRAL_ELEGIDA) bocha = MezclaCentral(COLOR_ACENTO_CENTRAL, WHITE, 0.15f + 0.15f * std::sin(t * 8.0f));
        if (estado == PALANCA_CENTRAL_FATAL) bocha = ColorLamparaCentral(m, i);
        if (estado == PALANCA_CENTRAL_SEGURA) bocha = TonoCentral(COLOR_SEGURO_CENTRAL, 0.7f);
        DrawSphereEx(punta, 0.22f * s, 10, 10, bocha);

        // Elegida: anillo ambar sobre la tapa y flecha que la señala.
        if (estado == PALANCA_CENTRAL_ELEGIDA)
        {
            float latido = 1.0f + 0.06f * std::sin(t * 8.0f) + m.pulsoSeleccion;
            AnilloCentral({ x, ALTURA_BASE_PALANCA + 0.01f, z }, 0.78f * latido, 0.9f * latido, COLOR_ACENTO_CENTRAL, 24);

            float y = 5.15f + 0.18f * std::sin(t * 6.0f);
            Vector3 c = { x, y, Z_TABLEROS_CENTRAL + 0.4f };
            rlBegin(RL_TRIANGLES);
            VerticeCentral({ c.x - 0.36f, c.y + 0.36f, c.z }, COLOR_ACENTO_CENTRAL);
            VerticeCentral({ c.x, c.y - 0.12f, c.z }, MezclaCentral(COLOR_ACENTO_CENTRAL, WHITE, 0.4f));
            VerticeCentral({ c.x + 0.36f, c.y + 0.36f, c.z }, COLOR_ACENTO_CENTRAL);
            VerticeCentral({ c.x - 0.36f, c.y + 0.36f, c.z }, TonoCentral(COLOR_ACENTO_CENTRAL, 0.7f));
            VerticeCentral({ c.x + 0.36f, c.y + 0.36f, c.z }, TonoCentral(COLOR_ACENTO_CENTRAL, 0.7f));
            VerticeCentral({ c.x, c.y + 0.2f, c.z - 0.06f }, TonoCentral(COLOR_ACENTO_CENTRAL, 0.7f));
            rlEnd();
        }
    }
}


//==================================================
// MURO DE MONITORES Y MEDIDOR DE SOBRECARGA
//==================================================

// Contenido de un monitor chico (0 osciloscopio, 1 barras, 2 esquema,
// 3 radar). En la sobrecarga muestran estatica roja.
static void PantallaChicaCentral(float cx, float cy, float ancho, float alto, int tipo, int semilla, const MinijuegoInterruptoresCaos& m)
{
    float t = m.tiempoAnimacion;
    float z = Z_MONITORES_CENTRAL + 0.12f;
    float x0 = cx - ancho * 0.5f;
    float y0 = cy - alto * 0.5f;
    Color fondo = Color{ 10, 26, 30, 255 };

    PlacaFrenteCentral(x0, y0, x0 + ancho, y0 + alto, z, fondo);

    if (m.sobrecarga > 0.25f)
    {
        for (int k = 0; k < 14; k++)
        {
            float u = Hash01Central(semilla * 13 + k, (int)(t * 20.0f));
            float v = Hash01Central(semilla * 17 + k, (int)(t * 20.0f) + 5);
            PlacaFrenteCentral(x0 + u * ancho * 0.8f, y0 + v * alto * 0.9f, x0 + u * ancho * 0.8f + ancho * 0.2f, y0 + v * alto * 0.9f + 0.04f, z + 0.004f, Color{ 255, 90, 70, 255 });
        }

        return;
    }

    Color trazo = semilla % 2 == 0 ? COLOR_PANTALLA_CENTRAL : Color{ 130, 236, 140, 255 };
    float zt = z + 0.004f;

    if (tipo == 0)
    {
        // Osciloscopio: la onda se agita con la carga.
        float amp = alto * (0.12f + 0.22f * m.tensionVista);
        Vector2 anterior = { x0 + 0.08f, cy };

        for (int k = 1; k <= 16; k++)
        {
            float f = (float)k / 16.0f;
            float x = x0 + 0.08f + (ancho - 0.16f) * f;
            float y = cy + std::sin(f * 12.0f - t * 4.0f + (float)semilla) * amp;
            TrazoFrenteCentral(anterior, { x, y }, zt, 0.035f, trazo);
            anterior = { x, y };
        }

        TrazoFrenteCentral({ x0 + 0.05f, cy }, { x0 + ancho - 0.05f, cy }, zt - 0.001f, 0.01f, TonoCentral(trazo, 0.35f));
    }
    else if (tipo == 1)
    {
        for (int b = 0; b < 7; b++)
        {
            float h = alto * (0.15f + 0.6f * (0.5f + 0.5f * std::sin(t * (1.0f + 0.4f * (float)b) + (float)(semilla + b))));
            float bx = x0 + 0.12f + (ancho - 0.24f) * (float)b / 7.0f;
            PlacaFrenteCentral(bx, y0 + 0.1f, bx + (ancho - 0.24f) / 7.0f - 0.05f, y0 + 0.1f + h, zt, trazo);
        }
    }
    else if (tipo == 2)
    {
        // Esquema unifilar con un pulso que recorre la linea.
        float yb = cy + alto * 0.22f;
        TrazoFrenteCentral({ x0 + 0.12f, yb }, { x0 + ancho - 0.12f, yb }, zt, 0.03f, trazo);

        for (int k = 0; k < 4; k++)
        {
            float x = x0 + 0.3f + (ancho - 0.6f) * (float)k / 3.0f;
            TrazoFrenteCentral({ x, yb }, { x, cy - alto * 0.25f }, zt, 0.03f, trazo);
            PlacaFrenteCentral(x - 0.08f, cy - alto * 0.35f, x + 0.08f, cy - alto * 0.2f, zt, TonoCentral(trazo, 0.7f));
        }

        float u = std::fmod(t * 0.5f + 0.3f * (float)semilla, 1.0f);
        float px = x0 + 0.12f + (ancho - 0.24f) * u;
        PlacaFrenteCentral(px - 0.06f, yb - 0.06f, px + 0.06f, yb + 0.06f, zt + 0.002f, Color{ 255, 236, 180, 255 });
    }
    else
    {
        // Radar: barrido que gira.
        float r = alto * 0.42f;
        ArcoFrenteCentral({ cx, cy, zt }, r - 0.02f, r, 0.0f, 360.0f, TonoCentral(trazo, 0.5f), 20);
        ArcoFrenteCentral({ cx, cy, zt }, r * 0.5f - 0.015f, r * 0.5f, 0.0f, 360.0f, TonoCentral(trazo, 0.35f), 16);
        float a = -t * 90.0f + (float)semilla * 40.0f;
        ArcoFrenteCentral({ cx, cy, zt + 0.002f }, 0.0f, r, a, a + 40.0f, TonoCentral(trazo, 0.45f), 4);
        TrazoFrenteCentral({ cx, cy }, { cx + std::cos((a + 40.0f) * DEG2RAD) * r, cy + std::sin((a + 40.0f) * DEG2RAD) * r }, zt + 0.004f, 0.03f, trazo);
    }
}


static void DibujarMonitoresCentral(const MinijuegoInterruptoresCaos& m, bool detalle)
{
    float t = m.tiempoAnimacion;
    float z = Z_MONITORES_CENTRAL;

    // Mueble bajo, cuerpo del muro y remate.
    CuboCentral({ 0.0f, 0.55f, z - 0.5f }, { 16.4f, 1.1f, 1.6f }, TonoCentral(COLOR_CONSOLA_CENTRAL, 0.85f));
    CuboCentral({ 0.0f, 3.8f, z - 0.4f }, { 15.6f, 5.6f, 0.8f }, COLOR_GRAFITO_CENTRAL);
    CuboCentral({ 0.0f, 6.66f, z - 0.4f }, { 15.9f, 0.12f, 1.0f }, COLOR_ACERO_CENTRAL);

    // Pantalla grande del centro con el medidor de sobrecarga.
    CuboCentral({ 0.0f, 4.05f, z + 0.05f }, { 6.3f, 4.9f, 0.1f }, Color{ 30, 34, 42, 255 });
    bool alarma = m.sobrecarga > 0.0f && std::sin(t * 18.0f) > -0.2f;
    float zp = z + 0.11f;
    PlacaFrenteCentral(-2.95f, 1.75f, 2.95f, 6.4f, zp, alarma ? Color{ 80, 14, 12, 255 } : Color{ 10, 22, 28, 255 });

    Vector3 centro = { 0.0f, 4.15f, zp + 0.005f };
    const int SEGMENTOS = 24;

    for (int k = 0; k < SEGMENTOS; k++)
    {
        float f = ((float)k + 0.5f) / (float)SEGMENTOS;
        Color c = f < 0.5f ? Color{ 90, 220, 120, 255 } : (f < 0.8f ? Color{ 250, 176, 50, 255 } : Color{ 250, 70, 56, 255 });
        bool lleno = f <= m.tensionVista + 0.02f;
        float a0 = 180.0f - 180.0f * (float)k / (float)SEGMENTOS - 1.0f;
        float a1 = 180.0f - 180.0f * (float)(k + 1) / (float)SEGMENTOS + 1.0f;
        ArcoFrenteCentral(centro, 1.45f, 1.85f, a0, a1, lleno ? c : TonoCentral(c, 0.26f), 2);
    }

    for (int k = 0; k <= 10; k++)
    {
        float a = (180.0f - 18.0f * (float)k) * DEG2RAD;
        TrazoFrenteCentral(
            { std::cos(a) * 1.25f, centro.y + std::sin(a) * 1.25f },
            { std::cos(a) * 1.38f, centro.y + std::sin(a) * 1.38f },
            centro.z, 0.04f, Color{ 200, 220, 220, 255 }
        );
    }

    float temblorAguja = 0.015f * std::sin(t * 23.0f) + (m.sobrecarga > 0.0f ? 0.04f * std::sin(t * 61.0f) : 0.0f);
    float a = (180.0f - 180.0f * Limitar01Central(m.tensionVista + temblorAguja)) * DEG2RAD;
    TrazoFrenteCentral({ 0.0f, centro.y }, { std::cos(a) * 1.7f, centro.y + std::sin(a) * 1.7f }, centro.z + 0.004f, 0.08f, Color{ 250, 248, 240, 255 });
    ArcoFrenteCentral({ 0.0f, centro.y, centro.z + 0.008f }, 0.0f, 0.2f, 0.0f, 360.0f, Color{ 150, 156, 164, 255 }, 10);

    // Onda de la red bajo el medidor: se desordena con la carga.
    {
        Vector2 anterior = { -2.7f, 2.4f };
        float amp = 0.1f + 0.18f * m.tensionVista;
        Color onda = MezclaCentral(COLOR_PANTALLA_CENTRAL, Color{ 250, 120, 70, 255 }, m.tensionVista);

        for (int k = 1; k <= 28; k++)
        {
            float f = (float)k / 28.0f;
            float x = -2.7f + 5.4f * f;
            float y = 2.4f + std::sin(f * 18.0f - t * 6.0f) * amp + std::sin(f * 53.0f + t * 11.0f) * amp * 0.35f * m.tensionVista;
            TrazoFrenteCentral(anterior, { x, y }, centro.z, 0.035f, onda);
            anterior = { x, y };
        }
    }

    // Copia de la consola: un cuadradito por palanca.
    for (int i = 0; i < m.cantidadInterruptores; i++)
    {
        float x = ((float)i - (float)(m.cantidadInterruptores - 1) * 0.5f) * 0.6f;
        EstadoPalancaCentral estado = EstadoPalancaInterruptores(m, i);
        Color c = estado == PALANCA_CENTRAL_LIBRE ? Color{ 70, 90, 96, 255 } : ColorLamparaCentral(m, i);
        PlacaFrenteCentral(x - 0.2f, 6.0f, x + 0.2f, 6.28f, centro.z, c);
    }

    // Monitores chicos a los costados.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int col = 0; col < 2; col++)
        {
            for (int fila = 0; fila < 2; fila++)
            {
                float cx = (4.35f + 2.2f * (float)col) * (float)lado;
                float cy = 3.05f + 1.95f * (float)fila;
                int semilla = (lado + 1) * 4 + col * 2 + fila;
                CuboCentral({ cx, cy, z + 0.05f }, { 2.05f, 1.75f, 0.1f }, Color{ 30, 34, 42, 255 });
                PantallaChicaCentral(cx, cy, 1.85f, 1.55f, detalle ? semilla % 4 : 0, semilla, m);
            }
        }
    }
}


//==================================================
// TARIMAS Y OPERADORES
//==================================================

static void DibujarTarimasCentral(const MinijuegoInterruptoresCaos& m, const Participante participantes[], bool detalle)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 b = PosicionPasarelaInterruptores(m, i);
        bool eliminado = m.jugadores[i].eliminado;
        Color chapa = Color{ 96, 106, 120, 255 };
        float alto = ALTURA_PASARELA_INTERRUPTORES;
        float zf = b.z + 1.2f;

        CuboCentral({ b.x, alto * 0.5f, b.z }, { 3.2f, alto, 2.4f }, chapa);
        CuboCentral({ b.x, alto + 0.01f, b.z }, { 3.1f, 0.02f, 2.3f }, TonoCentral(chapa, 1.12f));
        CuboCentral({ b.x, 0.15f, zf + 0.25f }, { 1.6f, 0.3f, 0.5f }, TonoCentral(chapa, 0.92f));

        rlBegin(RL_QUADS);

        // Chapa semilla: costuras en rombo.
        if (detalle)
        {
            for (int fx = 0; fx < 9; fx++)
            {
                for (int fz = 0; fz < 6; fz++)
                {
                    float x = b.x - 1.3f + 0.32f * (float)fx + ((fz & 1) ? 0.16f : 0.0f);
                    float z = b.z - 0.95f + 0.34f * (float)fz;
                    Color costura = TinteCentral(TonoCentral(chapa, 0.85f), { x, alto, z });
                    QuadCentral({ x - 0.08f, alto + 0.025f, z }, { x, alto + 0.025f, z - 0.03f }, { x + 0.08f, alto + 0.025f, z }, { x, alto + 0.025f, z + 0.03f }, costura);
                }
            }
        }

        // Franja de precaucion en el borde frontal.
        for (int k = 0; k < 12; k++)
        {
            float x0 = b.x - 1.55f + 0.26f * (float)k;
            QuadCentral({ x0, alto + 0.03f, zf - 0.02f }, { x0 + 0.13f, alto + 0.03f, zf - 0.02f }, { x0 + 0.2f, alto + 0.03f, zf - 0.22f }, { x0 + 0.07f, alto + 0.03f, zf - 0.22f }, TinteCentral(COLOR_PELIGRO_CENTRAL, b));
        }

        // Banda de luz del color del jugador al frente (marca su puesto).
        Color banda = eliminado ? Color{ 40, 40, 44, 255 } : participantes[i].color;
        QuadCentral({ b.x - 1.45f, 0.24f, zf + 0.005f }, { b.x + 1.45f, 0.24f, zf + 0.005f }, { b.x + 1.45f, 0.38f, zf + 0.005f }, { b.x - 1.45f, 0.38f, zf + 0.005f }, banda);
        rlEnd();
    }
}


static void DibujarOperadoresCentral(const MinijuegoInterruptoresCaos& m, const Participante participantes[])
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        PoseOperadorCentral pose = PoseOperadorInterruptores(m, i);
        Vector3 base = PosicionPasarelaInterruptores(m, i);
        float altura = pose.pies.y - base.y;
        float radio = 0.45f * (1.0f - Limitar01Central(altura / 3.0f) * 0.5f);
        DibujarSombraRetroCircular({ pose.pies.x, base.y, pose.pies.z + 0.4f * Limitar01Central(pose.caida / 90.0f) }, radio, radio * (1.0f + Limitar01Central(pose.caida / 90.0f)), base.y + 0.04f, 0.32f, 10);

        Color color = participantes[i].color;

        if (m.jugadores[i].eliminado)
        {
            color = MezclaCentral(color, Color{ 36, 36, 40, 255 }, 0.55f);
        }

        // Al recibir la descarga titila entre blanco y su color.
        const EstadoJugadorInterruptoresCaos& j = m.jugadores[i];
        if (j.descarga > 0.0f && j.descarga < 0.7f && std::sin(m.tiempoAnimacion * 50.0f) > 0.0f)
        {
            color = Color{ 220, 244, 255, 255 };
        }

        rlPushMatrix();
        rlTranslatef(pose.pies.x, pose.pies.y, pose.pies.z);
        rlRotatef(pose.caida, 1.0f, 0.0f, 0.0f);
        DibujarModeloJugadorEnPosicion({ 0.0f, 0.0f, 0.0f }, pose.giro, color);
        rlPopMatrix();
    }
}


// Confeti y restos (los cubitos solidos).
static void DibujarParticulasCentral(const MinijuegoInterruptoresCaos& m)
{
    for (int i = 0; i < MAX_PARTICULAS_INTERRUPTORES; i++)
    {
        const ParticulaInterruptoresCaos& p = m.particulas[i];

        if (!p.activa || p.brillo || p.humo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        float s = p.tamano * (0.5f + 0.5f * Limitar01Central(vida * 2.0f));
        CuboPuroCentral(p.posicion, { s, s * 0.4f, s }, p.color);
    }
}


//==================================================
// TRANSLUCIDOS Y LUCES FALSAS
//==================================================

static Vector3 CimaTorreCentral(int k)
{
    const Vector3 cimas[3] =
    {
        { -64.0f, Y_EXTERIOR_CENTRAL + 48.0f, -150.0f },
        { -18.0f, Y_EXTERIOR_CENTRAL + 44.0f, -190.0f },
        { -110.0f, Y_EXTERIOR_CENTRAL + 40.0f, -230.0f }
    };

    return cimas[k];
}


static void DibujarTranslucidosCentral(const MinijuegoInterruptoresCaos& m, const Participante participantes[], bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Vapor de las torres de enfriamiento, iluminado desde abajo.
    if (decoracion)
    {
        for (int k = 0; k < 3; k++)
        {
            Vector3 cima = CimaTorreCentral(k);

            for (int b = 0; b < 9; b++)
            {
                float ciclo = 14.0f + 4.0f * Hash01Central(k, b);
                float u = std::fmod(t / ciclo + Hash01Central(b, k + 40), 1.0f);
                Vector3 p =
                {
                    cima.x + 22.0f * u + 3.0f * std::sin(t * 0.2f + (float)b),
                    cima.y + 4.0f + 34.0f * u,
                    cima.z
                };
                float radio = 9.0f + 16.0f * u;
                float alfa = 70.0f * std::sin(u * PI);
                ResplandorFrenteCentral(p, radio, TinteCentral(Color{ 178, 184, 200, (unsigned char)alfa }, p));
            }
        }
    }

    // Vidrio del ventanal: velo frio y reflejos en diagonal.
    float zv = Z_VENTANAL_CENTRAL - 0.12f;
    rlBegin(RL_QUADS);
    QuadCentral({ -X_MURO_CENTRAL, 2.56f, zv }, { X_MURO_CENTRAL, 2.56f, zv }, { X_MURO_CENTRAL, Y_DINTEL_CENTRAL, zv }, { -X_MURO_CENTRAL, Y_DINTEL_CENTRAL, zv }, Color{ 120, 170, 210, 20 });

    for (int k = -5; k < 5; k++)
    {
        float x0 = (float)k * 3.6f;
        float y = 3.4f + 3.0f * Hash01Central(k + 20, 3);
        QuadCentral({ x0 + 0.5f, y, zv + 0.01f }, { x0 + 0.9f, y, zv + 0.01f }, { x0 + 2.1f, y + 2.2f, zv + 0.01f }, { x0 + 1.7f, y + 2.2f, zv + 0.01f }, Color{ 200, 230, 255, 22 });
        QuadCentral({ x0 + 1.1f, y, zv + 0.01f }, { x0 + 1.25f, y, zv + 0.01f }, { x0 + 2.45f, y + 2.2f, zv + 0.01f }, { x0 + 2.3f, y + 2.2f, zv + 0.01f }, Color{ 200, 230, 255, 16 });
    }

    rlEnd();

    // Anillo de cada operador y quemadura del que recibio la descarga.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 b = PosicionPasarelaInterruptores(m, i);

        if (m.jugadores[i].eliminado)
        {
            ManchaPisoCentral({ b.x, b.y + 0.035f, b.z - 0.1f }, 1.2f, i + 7, Color{ 20, 18, 20, 170 });
            continue;
        }

        bool enTurno = m.fase == FASE_INTERRUPTORES_ELECCION && i == m.jugadorTurno;
        float r = enTurno ? 0.85f + 0.05f * std::sin(t * 6.0f) : 0.75f;
        AnilloCentral({ b.x, b.y + 0.04f, b.z }, r - 0.12f, r, ConAlfaCentral(participantes[i].color, enTurno ? 230.0f : 170.0f), 28);
    }

    // Humo.
    for (int i = 0; i < MAX_PARTICULAS_INTERRUPTORES; i++)
    {
        const ParticulaInterruptoresCaos& p = m.particulas[i];

        if (!p.activa || !p.humo)
        {
            continue;
        }

        float u = 1.0f - p.vida / p.vidaMaxima;
        float alfa = 150.0f * std::sin(Limitar01Central(u) * PI) * (1.0f - u * 0.4f);
        ResplandorFrenteCentral(p.posicion, p.tamano * (1.0f + 2.2f * u), ConAlfaCentral(p.color, alfa));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Hilo de luz del operador en turno a la palanca elegida.
static Vector3 PuntoHiloCentral(Vector3 a, Vector3 b, float u)
{
    Vector3 control = { (a.x + b.x) * 0.5f, std::fmax(a.y, b.y) + 1.6f, (a.z + b.z) * 0.5f };
    float v = 1.0f - u;
    return {
        v * v * a.x + 2.0f * v * u * control.x + u * u * b.x,
        v * v * a.y + 2.0f * v * u * control.y + u * u * b.y,
        v * v * a.z + 2.0f * v * u * control.z + u * u * b.z
    };
}


static void DibujarLucesCentral(const MinijuegoInterruptoresCaos& m, const Participante participantes[], bool decoracion)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Tubos fluorescentes: halo y charco en el piso.
    for (int fz = 0; fz < FILAS_TUBOS_CENTRAL; fz++)
    {
        for (int fx = 0; fx < 4; fx++)
        {
            float x = X_TUBOS_CENTRAL[fx];
            float z = Z_TUBOS_CENTRAL[fz];
            float titilar = m.sobrecarga > 0.0f && std::sin(t * 41.0f + (float)(fx * 3 + fz)) > 0.3f ? 0.3f : 1.0f;
            unsigned char a = (unsigned char)(15.0f * titilar);

            ResplandorFrenteCentral({ x, Y_TUBOS_CENTRAL - 0.1f, z }, 1.8f, Color{ 190, 220, 255, (unsigned char)(50.0f * titilar) });
            ResplandorSueloCentral({ x, 0.02f, z }, 4.6f, Color{ 170, 200, 255, a }, 1.4f);
            ResplandorSueloCentral({ x, 0.025f, z }, 2.4f, Color{ 190, 215, 255, a }, 1.5f);
        }
    }

    // Muro de monitores: resplandor del medidor (del cian al rojo).
    Color medidor = MezclaCentral(COLOR_PANTALLA_CENTRAL, Color{ 255, 150, 60, 255 }, m.tensionVista);
    if (m.sobrecarga > 0.0f) medidor = COLOR_ALARMA_CENTRAL;
    ResplandorFrenteCentral({ 0.0f, 3.9f, Z_MONITORES_CENTRAL + 0.3f }, 4.6f, ConAlfaCentral(medidor, 40.0f + 50.0f * m.sobrecarga));
    ResplandorSueloCentral({ 0.0f, 0.03f, Z_MONITORES_CENTRAL + 1.8f }, 6.0f, ConAlfaCentral(medidor, 26.0f), 1.6f);
    ResplandorSueloCentral({ 0.0f, ALTURA_BASE_PALANCA + 0.02f, -4.4f }, 4.0f, ConAlfaCentral(medidor, 16.0f), 2.2f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        ResplandorFrenteCentral({ 5.45f * (float)lado, 3.6f, Z_MONITORES_CENTRAL + 0.3f }, 3.0f, Color{ 90, 230, 210, 22 });
    }

    // Palancas: lampara de cada tablero y halo de la elegida.
    for (int i = 0; i < m.cantidadInterruptores; i++)
    {
        float s = EscalaAparicionCentral(m, i);

        if (s < 0.6f)
        {
            continue;
        }

        float x = PosicionXPalanca(m, i);
        EstadoPalancaCentral estado = EstadoPalancaInterruptores(m, i);
        Color luz = ColorLamparaCentral(m, i);
        Vector3 lampara = { x, 2.65f + 1.6f * s + 0.38f, Z_TABLEROS_CENTRAL - 0.15f };
        float fuerza = estado == PALANCA_CENTRAL_LIBRE ? 0.35f : 1.0f;

        ResplandorFrenteCentral(lampara, 0.7f, ConAlfaCentral(luz, 140.0f * fuerza));
        ResplandorFrenteCentral(lampara, 0.3f, ConAlfaCentral(WHITE, 90.0f * fuerza));

        if (estado == PALANCA_CENTRAL_ELEGIDA)
        {
            float pulso = 0.85f + 0.15f * std::sin(t * 8.0f) + m.pulsoSeleccion * 2.0f;
            Vector3 punta = PuntaPalancaCentral(m, i);
            ResplandorFrenteCentral(punta, 0.9f * pulso, Color{ 255, 170, 50, 120 });
            ResplandorSueloCentral({ x, ALTURA_BASE_PALANCA + 0.03f, Z_PALANCAS_INTERRUPTORES }, 1.6f * pulso, Color{ 255, 160, 40, 70 });
            DrawCylinderEx({ x, 9.0f, Z_PALANCAS_INTERRUPTORES }, { x, ALTURA_BASE_PALANCA, Z_PALANCAS_INTERRUPTORES }, 0.25f, 1.1f, 12, Color{ 255, 190, 90, 16 });
        }

        if (m.destelloPalanca[i] > 0.0f)
        {
            float f = m.destelloPalanca[i];
            Color c = estado == PALANCA_CENTRAL_FATAL ? COLOR_ALARMA_CENTRAL : COLOR_SEGURO_CENTRAL;
            ResplandorFrenteCentral(PuntaPalancaCentral(m, i), 0.6f + 1.6f * (1.0f - f), ConAlfaCentral(c, 180.0f * f));
            ResplandorSueloCentral({ x, ALTURA_BASE_PALANCA + 0.03f, Z_PALANCAS_INTERRUPTORES }, 1.0f + 2.0f * (1.0f - f), ConAlfaCentral(c, 90.0f * f));
        }
    }

    // Operador en turno: foco cenital y el hilo de luz a su palanca.
    if (m.fase == FASE_INTERRUPTORES_ELECCION && m.jugadorTurno >= 0)
    {
        Vector3 b = PosicionPasarelaInterruptores(m, m.jugadorTurno);
        DrawCylinderEx({ b.x, Y_TUBOS_CENTRAL, b.z }, { b.x, b.y + 0.05f, b.z }, 0.35f, 1.8f, 16, Color{ 255, 236, 200, 16 });
        ResplandorSueloCentral({ b.x, b.y + 0.05f, b.z }, 2.0f, Color{ 255, 236, 200, 60 });
        ResplandorSueloCentral({ b.x, b.y + 0.055f, b.z }, 1.1f, Color{ 255, 236, 200, 40 });

        Vector3 desde = PechoOperadorCentral(m, m.jugadorTurno);
        Vector3 hasta = PuntaPalancaCentral(m, m.interruptorSeleccionado);
        Color color = participantes[m.jugadorTurno].color;
        Vector3 anterior = desde;

        for (int k = 1; k <= 16; k++)
        {
            Vector3 p = PuntoHiloCentral(desde, hasta, (float)k / 16.0f);
            DrawLine3D(anterior, p, ConAlfaCentral(color, 120.0f));
            anterior = p;
        }

        for (int k = 0; k < 10; k++)
        {
            float u = std::fmod((float)k / 10.0f + t * 0.7f, 1.0f);
            ResplandorFrenteCentral(PuntoHiloCentral(desde, hasta, u), 0.12f, ConAlfaCentral(MezclaCentral(color, WHITE, 0.4f), 220.0f * std::sin(u * PI)));
        }
    }

    // Balizas giratorias: mas fuertes cuanto mas cargada esta la red.
    float fuerzaBaliza = 0.3f + 0.7f * std::fmax(m.tensionVista * m.tensionVista, m.sobrecarga);
    float velocidad = m.sobrecarga > 0.0f ? 9.0f : 2.0f + 3.0f * m.tensionVista;
    Color baliza = ColorBalizaCentral(m);

    for (int k = 0; k < 4; k++)
    {
        Vector3 c = BALIZAS_CENTRAL[k];
        c.y += 0.22f;
        float giro = t * velocidad + (float)k * 1.3f;

        for (int haz = 0; haz < 2; haz++)
        {
            float a = giro + (float)haz * PI;
            Vector3 d0 = { std::cos(a - 0.22f), -0.12f, std::sin(a - 0.22f) };
            Vector3 d1 = { std::cos(a + 0.22f), -0.12f, std::sin(a + 0.22f) };
            rlBegin(RL_TRIANGLES);
            VerticeCentral(c, ConAlfaCentral(baliza, 70.0f * fuerzaBaliza));
            VerticeCentral(Vector3Add(c, Vector3Scale(d0, 6.0f)), ConAlfaCentral(baliza, 0.0f));
            VerticeCentral(Vector3Add(c, Vector3Scale(d1, 6.0f)), ConAlfaCentral(baliza, 0.0f));
            rlEnd();
        }

        float hacia = 0.5f + 0.5f * std::sin(giro);
        ResplandorFrenteCentral(c, 0.5f + 0.5f * hacia, ConAlfaCentral(baliza, (90.0f + 120.0f * hacia) * fuerzaBaliza));
    }

    // Aislador: arcos chicos que trepan cuando la red esta cargada.
    if (m.tensionVista > 0.3f && m.fase != FASE_INTERRUPTORES_EXPLOSION)
    {
        for (int k = -1; k <= 1; k += 2)
        {
            if (std::sin(t * 5.3f + (float)k * 2.0f) > 0.75f - 0.3f * m.tensionVista)
            {
                RayoCentral({ AISLADOR_CENTRAL.x + 2.4f * (float)k, AISLADOR_CENTRAL.y + 0.3f, AISLADOR_CENTRAL.z }, AISLADOR_CENTRAL, t, (float)k * 3.0f, 0.25f, Color{ 160, 220, 255, (unsigned char)(200.0f * m.tensionVista) });
            }
        }

        ResplandorFrenteCentral(AISLADOR_CENTRAL, 0.8f, Color{ 150, 210, 255, (unsigned char)(70.0f * m.tensionVista) });
    }

    // Sobrecarga: arcos del aislador al operador y a la palanca.
    if (m.fase == FASE_INTERRUPTORES_EXPLOSION && m.jugadorExplosion >= 0)
    {
        float progreso = 1.0f - Limitar01Central(m.tiempoResolucion / DURACION_EXPLOSION_INTERRUPTORES);
        float fuerza = 1.0f - progreso;
        Vector3 victima = PechoOperadorCentral(m, m.jugadorExplosion);
        Vector3 palanca = PuntaPalancaCentral(m, m.interruptorSeleccionado);

        for (int r = 0; r < 4; r++)
        {
            Color c = r % 2 == 0 ? Color{ 255, 255, 255, 255 } : Color{ 130, 220, 255, 255 };
            RayoCentral(AISLADOR_CENTRAL, victima, t, (float)r * 5.0f, 0.9f, ConAlfaCentral(c, 255.0f * (0.4f + 0.6f * fuerza)));
        }

        RayoCentral(AISLADOR_CENTRAL, palanca, t, 11.0f, 0.6f, Color{ 200, 240, 255, (unsigned char)(220.0f * fuerza) });
        ResplandorFrenteCentral(AISLADOR_CENTRAL, 1.8f, Color{ 180, 230, 255, (unsigned char)(200.0f * fuerza) });
        ResplandorFrenteCentral(victima, 1.2f + 2.0f * progreso, Color{ 160, 220, 255, (unsigned char)(180.0f * fuerza) });
        ResplandorSueloCentral({ 0.0f, 0.04f, 0.0f }, 18.0f, Color{ 140, 200, 255, (unsigned char)(50.0f * fuerza) }, 1.3f);
    }

    // Empalme del armario: chispa tenue permanente.
    ResplandorFrenteCentral(EMPALME_CENTRAL, 0.35f + 0.1f * std::sin(t * 37.0f), Color{ 255, 200, 120, 60 });

    if (decoracion)
    {
        // Nave: lamparas de sodio en las columnas y charcos en el piso.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            for (int k = 0; k < 5; k++)
            {
                float z = Z_VENTANAL_CENTRAL - 2.6f - 9.0f * (float)k;
                float x = (X_NAVE_CENTRAL - 1.6f) * (float)lado;
                Vector3 foco = { x, 11.9f, z };
                ResplandorFrenteCentral(foco, 2.2f, Color{ 255, 170, 80, 70 });
                DrawCylinderEx(foco, { x - 4.0f * (float)lado, Y_NAVE_CENTRAL, z }, 0.5f, 5.0f, 12, Color{ 255, 160, 70, 6 });
                ResplandorSueloCentral({ x - 6.0f * (float)lado, Y_NAVE_CENTRAL + 0.03f, z }, 10.0f, Color{ 255, 150, 70, 36 });
            }
        }

        // Lamparas altas sobre las turbinas.
        for (int u = 0; u < 3; u++)
        {
            for (int k = 0; k < 2; k++)
            {
                Vector3 foco = { X_TURBINAS_CENTRAL[u], 15.4f, -21.0f - 12.0f * (float)k };
                ResplandorFrenteCentral(foco, 1.6f, Color{ 255, 200, 120, 80 });
                ResplandorSueloCentral({ foco.x + 5.0f, Y_NAVE_CENTRAL + 0.03f, foco.z }, 5.0f, Color{ 255, 170, 90, 20 });
                ResplandorSueloCentral({ foco.x, Y_EJE_TURBINA_CENTRAL + 3.1f, foco.z }, 3.5f, Color{ 255, 190, 110, 20 });
            }
        }

        // Exterior: balizas rojas del tendido, luces de las torres y de la playa.
        for (int k = 0; k < TORRES_TENDIDO_CENTRAL; k++)
        {
            Vector3 b = BaseTorreTendidoCentral(k);
            float pulso = std::sin(t * 2.6f + (float)k * 0.9f) > 0.2f ? 1.0f : 0.15f;
            ResplandorFrenteCentral({ b.x, b.y + 31.0f, b.z }, 2.6f, Color{ 255, 70, 50, (unsigned char)(200.0f * pulso) });
        }

        for (int k = 0; k < 3; k++)
        {
            Vector3 cima = CimaTorreCentral(k);
            float pulso = std::sin(t * 1.8f + (float)k * 2.1f) > 0.0f ? 1.0f : 0.2f;
            ResplandorFrenteCentral({ cima.x + 12.0f, cima.y, cima.z }, 2.2f, Color{ 255, 80, 60, (unsigned char)(170.0f * pulso) });
            ResplandorFrenteCentral({ cima.x, cima.y - 30.0f, cima.z + 10.0f }, 10.0f, Color{ 255, 170, 90, 24 });
        }

        for (int k = 0; k < 18; k++)
        {
            Vector3 p = { 14.0f + 80.0f * Hash01Central(k, 90), Y_EXTERIOR_CENTRAL + 3.0f + 6.0f * Hash01Central(k, 91), -72.0f - 40.0f * Hash01Central(k, 92) };
            ResplandorFrenteCentral(p, 1.4f, Color{ 255, 190, 110, 150 });
        }

        // Polvo que flota en los conos de los tubos.
        int motas = (int)(36.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < motas; k++)
        {
            float x = X_TUBOS_CENTRAL[k % 4] + (Hash01Central(k, 200) - 0.5f) * 3.0f;
            float z = Z_TUBOS_CENTRAL[k % FILAS_TUBOS_CENTRAL] + (Hash01Central(k, 201) - 0.5f) * 3.0f;
            float y = 0.5f + std::fmod(Hash01Central(k, 202) * 9.0f + t * (0.15f + 0.1f * Hash01Central(k, 203)), 9.5f);
            Vector3 p = { x + std::sin(t * 0.5f + (float)k) * 0.3f, y, z };
            ResplandorFrenteCentral(p, 0.05f, Color{ 220, 235, 255, 150 });
        }
    }

    // Chispas.
    for (int i = 0; i < MAX_PARTICULAS_INTERRUPTORES; i++)
    {
        const ParticulaInterruptoresCaos& p = m.particulas[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteCentral(p.posicion, p.tamano * 2.6f, ConAlfaCentral(p.color, 230.0f * vida));
        ResplandorFrenteCentral(p.posicion, p.tamano, ConAlfaCentral(WHITE, 200.0f * vida));
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// ANUNCIOS Y HUD
//==================================================

static void DibujarAnunciosCentral(const MinijuegoInterruptoresCaos& m, const Participante participantes[], const Camera3D& vista)
{
    if (m.anuncio == ANUNCIO_INTERRUPTORES_NINGUNO || m.tiempoAnuncio <= 0.0f)
    {
        return;
    }

    float u = 1.0f - m.tiempoAnuncio / DURACION_ANUNCIO_INTERRUPTORES;
    float alfa = Limitar01Central(m.tiempoAnuncio / 0.35f) * Limitar01Central(u / 0.06f);
    float subida = SuaveCentral(u) * Px(16.0f);
    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();
    char texto[48];

    switch (m.anuncio)
    {
        case ANUNCIO_INTERRUPTORES_TURNO:
        {
            int j = m.jugadorAnuncio;

            if (j < 0 || j >= MAX_PARTICIPANTES || m.fase != FASE_INTERRUPTORES_ELECCION)
            {
                return;
            }

            Vector3 b = PosicionPasarelaInterruptores(m, j);
            Vector2 s = GetWorldToScreen({ b.x, b.y + 2.4f, b.z }, vista);
            std::snprintf(texto, sizeof(texto), "¡TURNO DE J%d!", participantes[j].numeroJugador);
            DibujarTextoMundoContornoUI(texto, s.x, s.y - Px(52.0f) - subida, UI_TEXTO_GRANDE, MezclaCentral(participantes[j].color, WHITE, 0.25f), alfa);
            break;
        }

        case ANUNCIO_INTERRUPTORES_SEGURO:
        {
            int p = m.palancaAnuncio;

            if (p < 0 || p >= m.cantidadInterruptores)
            {
                return;
            }

            Vector2 s = GetWorldToScreen({ PosicionXPalanca(m, p), 5.0f, Z_TABLEROS_CENTRAL }, vista);
            DibujarTextoMundoContornoUI("¡SEGURO!", s.x, s.y - Px(20.0f) - subida, UI_TEXTO_GRANDE, Color{ 160, 255, 170, 255 }, alfa);
            break;
        }

        case ANUNCIO_INTERRUPTORES_SOBRECARGA:
        {
            float escala = u < 0.12f ? 1.0f : 0.0f;
            DibujarTextoMundoContornoUI("¡SOBRECARGA!", ancho * 0.5f, alto * 0.3f - escala * Px(6.0f), UI_TEXTO_TITULO, Color{ 255, 96, 64, 255 }, alfa);

            int j = m.jugadorAnuncio;

            if (j >= 0 && j < MAX_PARTICIPANTES)
            {
                std::snprintf(texto, sizeof(texto), "J%d QUEDA FUERA", participantes[j].numeroJugador);
                DibujarTextoMundoContornoUI(texto, ancho * 0.5f, alto * 0.3f + Px(52.0f), UI_TEXTO_GRANDE, Color{ 240, 240, 236, 255 }, alfa);
            }

            break;
        }

        case ANUNCIO_INTERRUPTORES_RONDA:
        {
            DibujarTextoMundoContornoUI("¡NUEVA RONDA!", ancho * 0.5f, alto * 0.3f - subida, UI_TEXTO_TITULO, Color{ 255, 206, 110, 255 }, alfa);
            std::snprintf(texto, sizeof(texto), "QUEDAN %d PALANCAS", m.palancaAnuncio);
            DibujarTextoMundoContornoUI(texto, ancho * 0.5f, alto * 0.3f + Px(52.0f) - subida, UI_TEXTO_GRANDE, Color{ 240, 240, 236, 255 }, alfa);
            break;
        }

        case ANUNCIO_INTERRUPTORES_NINGUNO:
            break;
    }
}


// Movimiento de la eleccion segun el control real del jugador: con
// teclado completo o WASD la seleccion solo se mueve con A/D; con
// flechas, IZQ/DER; con mando, cruceta o stick.
static const char* TextoElegirInterruptores(const Participante& participante)
{
    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
        case CONTROL_TECLADO_FLECHAS:
            return "IZQ/DER";

        case CONTROL_TECLADO_COMPLETO:
        case CONTROL_TECLADO_WASD:
            return "A/D";

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoInterruptoresCaos::Dibujar(
    const Participante participantes[]
) const
{

    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool detalle = CalidadDecoracion() == CALIDAD_ALTA;

    // Entrada: durante la cuenta la camara se acerca desde mas atras, con
    // un leve barrido lateral.
    Camera3D vista = CamaraCentral();

    if (fase == FASE_INTERRUPTORES_PREPARACION)
    {
        float u = Limitar01Central(tiempoPreparacion / DURACION_PREPARACION_INTERRUPTORES);
        float suave = SuaveCentral(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        Vector3 atras = Vector3Subtract(vista.position, vista.target);
        vista.position = Vector3Add(vista.target, Vector3Scale(atras, 1.0f + 0.28f * suave));
        vista.position.x -= 3.0f * suave;
        if (vista.position.y > 11.2f) vista.position.y = 11.2f;   // por debajo del cielorraso
    }

    ClearBackground(COLOR_PENUMBRA_CENTRAL);
    DibujarCieloCentral(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    if (decoracion)
    {
        DibujarExteriorCentral(t, detalle);
        DibujarNaveCentral(t, detalle);
    }

    DibujarMurosCentral(detalle);
    DibujarPisoCentral();
    DibujarCielorrasoCentral(*this, decoracion);

    if (decoracion)
    {
        DibujarUtileriaCentral(*this, detalle);
    }

    DibujarMonitoresCentral(*this, detalle);
    DibujarConsolaCentral(*this, detalle);
    DibujarTablerosCentral(*this);
    DibujarPalancasCentral(*this);
    DibujarTarimasCentral(*this, participantes, detalle);
    DibujarOperadoresCentral(*this, participantes);
    DibujarParticulasCentral(*this);
    DibujarTranslucidosCentral(*this, participantes, decoracion);
    DibujarLucesCentral(*this, participantes, decoracion);

    rlEnableBackfaceCulling();
    EndMode3D();

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Fogonazo de la sobrecarga y bordes rojos mientras dura la alarma.
    if (sobrecarga > 0.8f)
    {
        DrawRectangle(0, 0, ancho, alto, Fade(WHITE, 0.55f * (sobrecarga - 0.8f) / 0.2f));
    }

    if (sobrecarga > 0.0f)
    {
        float a = 0.3f * sobrecarga * (0.6f + 0.4f * std::sin(t * 18.0f));
        DrawRectangleGradientV(0, 0, ancho, alto / 4, Fade(COLOR_ALARMA_CENTRAL, a), Fade(COLOR_ALARMA_CENTRAL, 0.0f));
        DrawRectangleGradientV(0, alto - alto / 4, ancho, alto / 4, Fade(COLOR_ALARMA_CENTRAL, 0.0f), Fade(COLOR_ALARMA_CENTRAL, a));
    }

    // Rotulos J1..J4 sobre cada operador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        PoseOperadorCentral pose = PoseOperadorInterruptores(*this, i);
        float arriba = 2.3f * std::cos(pose.caida * DEG2RAD) + 0.6f;
        Vector2 p = GetWorldToScreen({ pose.pies.x, pose.pies.y + arriba, pose.pies.z + 0.8f * std::sin(pose.caida * DEG2RAD) }, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        op.alfa = jugadores[i].eliminado ? 0.6f : 1.0f;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    DibujarAnunciosCentral(*this, participantes, vista);

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "INTERRUPTORES DEL CAOS",
        "ELEGÍ UNA PALANCA Y ACTIVALA: UNA SOLA PROVOCA LA SOBRECARGA",
        resultado
    );

    hud.fase = fase == FASE_INTERRUPTORES_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_INTERRUPTORES_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = fase == FASE_INTERRUPTORES_ELECCION
        ? (tiempoTurno > 0.0f ? tiempoTurno : 0.0f)
        : DURACION_TURNO_INTERRUPTORES;
    hud.tiempoTotal = DURACION_TURNO_INTERRUPTORES;
    hud.segundosAlerta = 3.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        if (!h.visible) continue;

        const EstadoJugadorInterruptoresCaos& j = jugadores[i];

        std::snprintf(h.valor, sizeof(h.valor), "%d", j.turnosSuperados);
        h.valorNumerico = j.turnosSuperados;
        h.eliminado = j.eliminado;

        if (!j.eliminado && i == jugadorTurno && fase == FASE_INTERRUPTORES_ELECCION)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡SU TURNO!");
        }
        else if (!j.eliminado && i == jugadorTurno && fase == FASE_INTERRUPTORES_SEGURO)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡SE SALVÓ!");
        }

        std::snprintf(
            h.detalleFinal,
            sizeof(h.detalleFinal),
            j.turnosSuperados == 1 ? "%d PALANCA SEGURA" : "%d PALANCAS SEGURAS",
            j.turnosSuperados
        );

        if (EsHumanoCentral(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "ELEGIR: %s\nACTIVAR: %s",
                TextoElegirInterruptores(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoInterruptoresCaos::ObtenerResultado() const
{
    return resultado;
}
