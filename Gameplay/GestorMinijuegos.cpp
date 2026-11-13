#include "Gameplay/GestorMinijuegos.h"

#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/UtilidadesMinijuegos.h"


// Formas de llamada de cada columna de LISTA_MINIJUEGOS (leyenda en
// Gameplay/ListaMinijuegos.h). Usan los parametros locales de cada funcion
// del gestor: contexto, deltaTime y mostrarDebug.

#define GM_REINICIAR_JC(m) \
    m.Reiniciar(contexto.jugadores, contexto.cantidadJugadores);
#define GM_REINICIAR_PC(m) \
    m.Reiniciar(contexto.participantes, contexto.cantidadJugadores);
#define GM_REINICIAR_P(m) \
    m.Reiniciar(contexto.participantes);
#define GM_REINICIAR_JPC(m) \
    m.Reiniciar(contexto.jugadores, contexto.participantes, contexto.cantidadJugadores);

#define GM_ACTUALIZAR_P(m) \
    m.Actualizar(deltaTime, contexto.participantes);
#define GM_ACTUALIZAR_CP(m) \
    m.Actualizar(deltaTime, contexto.cantidadJugadores, contexto.participantes);
#define GM_ACTUALIZAR_JCP(m) \
    m.Actualizar(deltaTime, contexto.jugadores, contexto.cantidadJugadores, \
        contexto.participantes);
#define GM_ACTUALIZAR_JCPX(m) \
    m.Actualizar(deltaTime, contexto.jugadores, contexto.cantidadJugadores, \
        contexto.participantes, contexto.particulas, contexto.cantidadParticulas);
#define GM_ACTUALIZAR_JCPXA(m) \
    m.Actualizar(deltaTime, contexto.jugadores, contexto.cantidadJugadores, \
        contexto.participantes, contexto.particulas, contexto.cantidadParticulas, \
        contexto.audio);

#define GM_DIBUJAR_P(m) \
    m.Dibujar(contexto.participantes);
#define GM_DIBUJAR_CP(m) \
    m.Dibujar(contexto.cantidadJugadores, contexto.participantes);
#define GM_DIBUJAR_JCP(m) \
    m.Dibujar(contexto.jugadores, contexto.cantidadJugadores, contexto.participantes);
#define GM_DIBUJAR_JCPD(m) \
    m.Dibujar(contexto.jugadores, contexto.cantidadJugadores, contexto.participantes, \
        mostrarDebug);
#define GM_DIBUJAR_JCPXD(m) \
    m.Dibujar(contexto.jugadores, contexto.cantidadJugadores, contexto.participantes, \
        contexto.particulas, contexto.cantidadParticulas, mostrarDebug);

// Activacion: paso ANTES y DESPUES de ReiniciarJugadoresCompartidos.
#define GM_ANTES_INICIAR_CONFIGURAR(m, r) \
    m.Inicializar(); \
    m.ConfigurarJugadores(contexto.jugadores, contexto.cantidadJugadores);
#define GM_DESPUES_INICIAR_CONFIGURAR(m, r)

#define GM_ANTES_INICIAR_CONFIGURAR_REINICIAR(m, r) GM_ANTES_INICIAR_CONFIGURAR(m, r)
#define GM_DESPUES_INICIAR_CONFIGURAR_REINICIAR(m, r) GM_REINICIAR_##r(m)

#define GM_ANTES_REINICIAR_ANTES(m, r) GM_REINICIAR_##r(m)
#define GM_DESPUES_REINICIAR_ANTES(m, r)

#define GM_ANTES_INICIAR_REINICIAR(m, r) m.Inicializar();
#define GM_DESPUES_INICIAR_REINICIAR(m, r) GM_REINICIAR_##r(m)

#define GM_ANTES_REINICIAR_DESPUES(m, r)
#define GM_DESPUES_REINICIAR_DESPUES(m, r) GM_REINICIAR_##r(m)

#define GM_AUDIO_SI(m) m.audio = contexto.audio;
#define GM_AUDIO_NO(m)

#define GM_IA_SI true
#define GM_IA_NO false

#define GM_DESCARGA_SI(m) m.Descargar();
#define GM_DESCARGA_NO(m)
#define GM_DESCARGA_SOLO_AL_CERRAR(m)

#define GM_CIERRE_SI(m)
#define GM_CIERRE_NO(m)
#define GM_CIERRE_SOLO_AL_CERRAR(m) m.Descargar();


// Generadores: uno por dispatch, todos con la firma de una fila de la lista.

#define GM_X_INICIALIZAR(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    m.Inicializar();

#define GM_X_AUDIO(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    GM_AUDIO_##aud(m)

#define GM_X_ACTIVAR_ANTES(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: GM_ANTES_##act(m, rei) break;

#define GM_X_ACTIVAR_DESPUES(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: GM_DESPUES_##act(m, rei) break;

#define GM_X_REINICIAR(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: GM_REINICIAR_##rei(m) break;

#define GM_X_ACTUALIZAR(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: GM_ACTUALIZAR_##upd(m) break;

#define GM_X_DIBUJAR(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: GM_DIBUJAR_##dib(m) break;

#define GM_X_RESULTADO(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: return &m.ObtenerResultado();

#define GM_X_IA_RECONEXION(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: return GM_IA_##ia;

#define GM_X_DESCARGAR(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    case id: GM_DESCARGA_##desc(m) break;

#define GM_X_CIERRE(id, T, m, act, rei, upd, dib, aud, ia, desc) \
    GM_CIERRE_##desc(m)


void GestorMinijuegos::Inicializar()
{
    LISTA_MINIJUEGOS(GM_X_INICIALIZAR)
}


void GestorMinijuegos::ActivarMinijuego(
    IdMinijuego id,
    ContextoMinijuego& contexto
)
{
    if (!EsIdMinijuegoValido(id))
    {
        return;
    }

    minijuegoActivo = id;

    // Los minijuegos con audio propio reciben el sistema compartido.
    LISTA_MINIJUEGOS(GM_X_AUDIO)

    switch (minijuegoActivo)
    {
        LISTA_MINIJUEGOS(GM_X_ACTIVAR_ANTES)

        case CANTIDAD_MINIJUEGOS:
            break;
    }

    ReiniciarJugadoresCompartidos(contexto);

    switch (minijuegoActivo)
    {
        LISTA_MINIJUEGOS(GM_X_ACTIVAR_DESPUES)

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


void GestorMinijuegos::ReiniciarActivo(
    ContextoMinijuego& contexto
)
{
    switch (minijuegoActivo)
    {
        LISTA_MINIJUEGOS(GM_X_REINICIAR)

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


void GestorMinijuegos::ActualizarActivo(
    float deltaTime,
    ContextoMinijuego& contexto
)
{
    switch (minijuegoActivo)
    {
        LISTA_MINIJUEGOS(GM_X_ACTUALIZAR)

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


void GestorMinijuegos::DibujarActivo(
    const ContextoMinijuego& contexto,
    bool mostrarDebug
) const
{
    switch (minijuegoActivo)
    {
        LISTA_MINIJUEGOS(GM_X_DIBUJAR)

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


const ResultadoMinijuego*
GestorMinijuegos::ObtenerResultadoActivo() const
{
    switch (minijuegoActivo)
    {
        LISTA_MINIJUEGOS(GM_X_RESULTADO)

        case CANTIDAD_MINIJUEGOS:
            break;
    }

    return nullptr;
}


IdMinijuego GestorMinijuegos::ObtenerIdActivo() const
{
    return minijuegoActivo;
}


bool GestorMinijuegos::MantieneControlIAEnReconexion() const
{
    switch (minijuegoActivo)
    {
        LISTA_MINIJUEGOS(GM_X_IA_RECONEXION)

        case CANTIDAD_MINIJUEGOS:
            break;
    }

    return false;
}


void GestorMinijuegos::ReiniciarJugadorCompartido(
    ContextoMinijuego& contexto,
    int indice
) const
{
    if (
        indice < 0 ||
        indice >= contexto.cantidadJugadores
    )
    {
        return;
    }

    ReiniciarJugadorPrueba(
        contexto.jugadores[indice]
    );
}


void GestorMinijuegos::ReiniciarJugadoresCompartidos(
    ContextoMinijuego& contexto
) const
{
    for (int i = 0; i < contexto.cantidadJugadores; i++)
    {
        ReiniciarJugadorCompartido(
            contexto,
            i
        );
    }
}


void GestorMinijuegos::DescargarMinijuego(
    IdMinijuego id
)
{
    switch (id)
    {
        LISTA_MINIJUEGOS(GM_X_DESCARGAR)

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


void GestorMinijuegos::Descargar()
{
    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        DescargarMinijuego(ObtenerIdMinijuegoPorIndice(i));
    }

    // Lo que solo se libera al cerrar (Fabrica 67) va despues de todos.
    LISTA_MINIJUEGOS(GM_X_CIERRE)
}
