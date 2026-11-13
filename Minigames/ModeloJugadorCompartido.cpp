#include "Minigames/ModeloJugadorCompartido.h"

#include <cmath>


// Velocidad del ciclo de pasos (radianes por segundo a marcha completa).
static const float VELOCIDAD_FASE_PASOS = 11.0f;

// Tramos de la animacion de caida dentro del aturdimiento.
static const float DURACION_CAER = 0.16f;
static const float DURACION_LEVANTARSE = 0.30f;


// Sombra de contacto: la misma que estampaba antes DrawModelEx (macro de
// SombrasRetro) bajo el modelo GLB, con el radio segun la escala.
static void DibujarSombraContactoPersonaje(
    Vector3 pies,
    float escala
)
{
    float radioX = escala * 0.52f;
    float radioZ = escala * 0.44f;

    DibujarSombraRetroCircular(
        pies,
        radioX < 0.24f ? 0.24f : radioX,
        radioZ < 0.20f ? 0.20f : radioZ
    );
}


int ObtenerPersonajeParticipante(
    const Participante& participante
)
{
    if (
        participante.personajeSeleccionado >= 0 &&
        participante.personajeSeleccionado < CANTIDAD_PERSONAJES_BLOQUE
    )
    {
        return participante.personajeSeleccionado;
    }

    return NormalizarPersonajeBloque(participante.numeroJugador - 1);
}


float ObtenerAnguloModeloJugadorCompartido(
    const JugadorPrueba& jugador
)
{
    float x = jugador.direccionMirada.x;
    float z = jugador.direccionMirada.z;

    // Aturdido: mira hacia quien lo golpeo, asi cae de espaldas en la
    // direccion del empuje.
    if (jugador.tiempoAturdido > 0.0f)
    {
        float gx = jugador.direccionGolpeRecibido.x;
        float gz = jugador.direccionGolpeRecibido.z;

        if (std::fabs(gx) > 0.001f || std::fabs(gz) > 0.001f)
        {
            x = -gx;
            z = -gz;
        }
    }

    if (
        std::fabs(x) < 0.001f &&
        std::fabs(z) < 0.001f
    )
    {
        return 0.0f;
    }

    return
        std::atan2(x, z) *
        RAD2DEG;
}


PosePersonajeBloque CrearPoseReposoPersonaje()
{
    PosePersonajeBloque pose;
    pose.tiempo = (float)GetTime();
    return pose;
}


PosePersonajeBloque CrearPoseJugadorPrueba(
    const JugadorPrueba& jugador
)
{
    PosePersonajeBloque pose = CrearPoseReposoPersonaje();

    bool aturdido = jugador.tiempoAturdido > 0.0f;

    // Marcha segun la velocidad horizontal real (no la entrada).
    if (jugador.enSuelo && !aturdido)
    {
        float velocidad = std::sqrt(
            jugador.velocidad.x * jugador.velocidad.x +
            jugador.velocidad.z * jugador.velocidad.z
        );

        float referencia =
            jugador.velocidadMovimiento > 1.0f
            ? jugador.velocidadMovimiento
            : 1.0f;

        float intensidad = velocidad / referencia;
        pose.caminar = intensidad > 1.0f ? 1.0f : intensidad;
    }

    pose.faseCaminar = pose.tiempo * VELOCIDAD_FASE_PASOS;
    pose.enAire = !jugador.enSuelo && !jugador.cayendo;

    // Golpe: la animacion completa (anticipacion + impacto + recuperacion).
    // Los minijuegos que solo marcan "golpeando" en un jugador visual
    // muestran el brazo extendido.
    if (jugador.tiempoAnimacionGolpe > 0.0f)
    {
        pose.golpe =
            1.0f -
            jugador.tiempoAnimacionGolpe / DURACION_ANIMACION_GOLPE;
    }
    else if (jugador.golpeando)
    {
        pose.golpe = 0.42f;
    }

    if (aturdido)
    {
        pose.aturdido = true;

        float duracion =
            jugador.duracionAturdido > jugador.tiempoAturdido
            ? jugador.duracionAturdido
            : jugador.tiempoAturdido;

        float transcurrido = duracion - jugador.tiempoAturdido;
        float restante = jugador.tiempoAturdido;

        if (jugador.derribado)
        {
            float caida = 1.0f;

            if (transcurrido < DURACION_CAER)
            {
                caida = transcurrido / DURACION_CAER;
            }
            else if (restante < DURACION_LEVANTARSE)
            {
                caida = restante / DURACION_LEVANTARSE;
            }

            pose.caida = caida;
        }
        else
        {
            // Golpe leve: se echa atras rapido y vuelve durante el resto.
            const float IDA = 0.12f;

            pose.tambaleo =
                transcurrido < IDA
                ? transcurrido / IDA
                : restante / (duracion - IDA > 0.01f ? duracion - IDA : 0.01f);
        }

        pose.golpe = -1.0f;
    }

    if (jugador.aplastado)
    {
        pose.escalaVertical = 0.28f;
        pose.golpe = -1.0f;
    }

    if (jugador.tiempoInmunidad > 0.0f)
    {
        int fase = (int)(jugador.tiempoInmunidad * 12.0f);
        pose.alpha = fase % 2 == 0 ? 0.22f : 1.0f;
    }

    return pose;
}


void DibujarSombraModeloJugadorCompartido(
    const JugadorPrueba& jugador
)
{
    if (jugador.cayendo || !jugador.enSuelo)
    {
        return;
    }

    Vector3 posicionSombra =
    {
        jugador.posicion.x,
        jugador.posicion.y -
            jugador.tamano.y * 0.5f +
            0.012f,
        jugador.posicion.z
    };

    float radio =
        jugador.tamano.x * 0.48f;

    DrawCylinder(
        posicionSombra,
        radio,
        radio,
        0.018f,
        10,
        Fade(BLACK, 0.28f)
    );
}


void DibujarJugadorModeloCompartido(
    const JugadorPrueba& jugador,
    const Participante& participante
)
{
    if (
        !participante.activo ||
        !participante.conectado ||
        jugador.cayendo
    )
    {
        return;
    }

    DibujarSombraModeloJugadorCompartido(
        jugador
    );

    Vector3 pies =
        jugador.posicion;

    pies.y -=
        jugador.tamano.y * 0.5f;

    pies.y += 0.04f;

    DibujarSombraContactoPersonaje(pies, ESCALA_MODELO_JUGADOR_3D);

    DibujarPersonajeBloque(
        ObtenerPersonajeParticipante(participante),
        pies,
        ObtenerAnguloModeloJugadorCompartido(jugador),
        ESCALA_MODELO_JUGADOR_3D,
        participante.color,
        CrearPoseJugadorPrueba(jugador)
    );
}


void DibujarModeloJugadorEnPosicion(
    int personaje,
    Vector3 posicionPies,
    float anguloY,
    Color color,
    float escala
)
{
    DibujarSombraContactoPersonaje(posicionPies, escala);

    DibujarPersonajeBloque(
        personaje,
        posicionPies,
        anguloY,
        escala,
        color,
        CrearPoseReposoPersonaje()
    );
}


void DibujarModeloJugadorEnPosicion(
    int personaje,
    Vector3 posicionPies,
    float anguloY,
    Color color,
    float escala,
    const PosePersonajeBloque& pose
)
{
    DibujarSombraContactoPersonaje(posicionPies, escala);

    DibujarPersonajeBloque(
        personaje,
        posicionPies,
        anguloY,
        escala,
        color,
        pose
    );
}
