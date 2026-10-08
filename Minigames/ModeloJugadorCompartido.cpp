#include "Minigames/ModeloJugadorCompartido.h"

#include "Minigames/TransformacionModeloJugador.h"

#include "raymath.h"

#include <cmath>
#include <cstring>


struct RecursoModeloJugadorCompartido
{
    Model modelo{};
    bool modeloCargado = false;

    ModelAnimation* animaciones = nullptr;
    int cantidadAnimaciones = 0;
    int indiceIdle = -1;

    int ultimoFotogramaIdleAplicado = -1;
};


static RecursoModeloJugadorCompartido recursoModeloJugador;


static bool NombreAnimacionEsIdleCompartido(
    const char* nombre
)
{
    if (nombre == nullptr)
    {
        return false;
    }

    return
        std::strstr(nombre, "Idle") != nullptr ||
        std::strstr(nombre, "idle") != nullptr ||
        std::strstr(nombre, "IDLE") != nullptr;
}


void InicializarModeloJugadorCompartido()
{
    RecursoModeloJugadorCompartido& recurso =
        recursoModeloJugador;

    if (recurso.modeloCargado)
    {
        return;
    }

    if (!FileExists(RUTA_MODELO_JUGADOR_3D))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro el modelo compartido del jugador: %s",
            RUTA_MODELO_JUGADOR_3D
        );
        return;
    }

    recurso.modelo =
        LoadModel(RUTA_MODELO_JUGADOR_3D);

    recurso.modeloCargado =
        recurso.modelo.meshCount > 0;

    if (!recurso.modeloCargado)
    {
        recurso.modelo = {};

        TraceLog(
            LOG_WARNING,
            "No se pudo cargar el modelo compartido del jugador"
        );

        return;
    }

    PrepararTransformacionModeloJugador(
        recurso.modelo
    );

    recurso.animaciones =
        LoadModelAnimations(
            RUTA_MODELO_JUGADOR_3D,
            &recurso.cantidadAnimaciones
        );

    recurso.indiceIdle = -1;
    recurso.ultimoFotogramaIdleAplicado = -1;

    for (
        int i = 0;
        recurso.animaciones != nullptr &&
        i < recurso.cantidadAnimaciones;
        i++
    )
    {
        if (
            NombreAnimacionEsIdleCompartido(
                recurso.animaciones[i].name
            )
        )
        {
            recurso.indiceIdle = i;
            break;
        }
    }

    if (
        recurso.indiceIdle < 0 &&
        recurso.animaciones != nullptr &&
        recurso.cantidadAnimaciones == 1
    )
    {
        recurso.indiceIdle = 0;
    }
}


bool ModeloJugadorCompartidoCargado()
{
    return recursoModeloJugador.modeloCargado;
}


const Model* ObtenerModeloJugadorCompartidoRender()
{
    if (!recursoModeloJugador.modeloCargado)
    {
        return nullptr;
    }

    return &recursoModeloJugador.modelo;
}


int ObtenerCantidadAnimacionesModeloJugadorCompartido()
{
    return recursoModeloJugador.cantidadAnimaciones;
}


bool AnimacionIdleModeloJugadorCompartidoActiva()
{
    const RecursoModeloJugadorCompartido& recurso =
        recursoModeloJugador;

    if (
        !recurso.modeloCargado ||
        recurso.animaciones == nullptr ||
        recurso.indiceIdle < 0 ||
        recurso.indiceIdle >= recurso.cantidadAnimaciones
    )
    {
        return false;
    }

    const ModelAnimation& animacion =
        recurso.animaciones[recurso.indiceIdle];

    return
        animacion.keyframeCount > 0 &&
        IsModelAnimationValid(
            recurso.modelo,
            animacion
        );
}


const char* ObtenerNombreIdleModeloJugadorCompartido()
{
    if (!AnimacionIdleModeloJugadorCompartidoActiva())
    {
        return "";
    }

    return recursoModeloJugador
        .animaciones[recursoModeloJugador.indiceIdle]
        .name;
}


int ObtenerCantidadFotogramasIdleModeloJugadorCompartido()
{
    if (!AnimacionIdleModeloJugadorCompartidoActiva())
    {
        return 0;
    }

    return recursoModeloJugador
        .animaciones[recursoModeloJugador.indiceIdle]
        .keyframeCount;
}


void AplicarFotogramaIdleModeloJugadorCompartido(
    int fotograma
)
{
    RecursoModeloJugadorCompartido& recurso =
        recursoModeloJugador;

    if (!AnimacionIdleModeloJugadorCompartidoActiva())
    {
        return;
    }

    ModelAnimation& animacion =
        recurso.animaciones[recurso.indiceIdle];

    int cantidadFotogramas =
        animacion.keyframeCount;

    if (cantidadFotogramas <= 0)
    {
        return;
    }

    int fotogramaNormalizado =
        fotograma % cantidadFotogramas;

    if (fotogramaNormalizado < 0)
    {
        fotogramaNormalizado += cantidadFotogramas;
    }

    if (
        recurso.ultimoFotogramaIdleAplicado ==
        fotogramaNormalizado
    )
    {
        return;
    }

    UpdateModelAnimation(
        recurso.modelo,
        animacion,
        fotogramaNormalizado
    );

    recurso.ultimoFotogramaIdleAplicado =
        fotogramaNormalizado;
}


void ActualizarAnimacionModeloJugadorCompartido()
{
    if (!AnimacionIdleModeloJugadorCompartidoActiva())
    {
        return;
    }

    const int FPS_ANIMACION = 30;

    int cantidadFotogramas =
        ObtenerCantidadFotogramasIdleModeloJugadorCompartido();

    if (cantidadFotogramas <= 0)
    {
        return;
    }

    int fotograma =
        (int)(GetTime() * FPS_ANIMACION) %
        cantidadFotogramas;

    AplicarFotogramaIdleModeloJugadorCompartido(
        fotograma
    );
}


float ObtenerAnguloModeloJugadorCompartido(
    const JugadorPrueba& jugador
)
{
    float x = jugador.direccionMirada.x;
    float z = jugador.direccionMirada.z;

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


// DrawModelEx multiplica la textura por el tinte: con el color puro del
// jugador (p.ej. azul oscuro) el personaje queda casi negro. Se aclara el
// tinte hacia el blanco para que se vea la textura y se reconozca el color.
static Color TinteModeloJugador(
    Color color
)
{
    const float mezcla = 0.60f;

    return Color
    {
        (unsigned char)(color.r + (255 - color.r) * mezcla),
        (unsigned char)(color.g + (255 - color.g) * mezcla),
        (unsigned char)(color.b + (255 - color.b) * mezcla),
        color.a
    };
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

    InicializarModeloJugadorCompartido();

    const Model* modelo =
        ObtenerModeloJugadorCompartidoRender();

    if (modelo == nullptr)
    {
        DrawCube(
            jugador.posicion,
            jugador.tamano.x,
            jugador.tamano.y,
            jugador.tamano.z,
            participante.color
        );

        return;
    }

    ActualizarAnimacionModeloJugadorCompartido();

    float alpha = 1.0f;

    if (jugador.tiempoInmunidad > 0.0f)
    {
        int fase =
            (int)(jugador.tiempoInmunidad * 12.0f);

        alpha =
            fase % 2 == 0
            ? 0.22f
            : 1.0f;
    }

    Color color =
        Fade(TinteModeloJugador(participante.color), alpha);

    Vector3 posicion =
        jugador.posicion;

    posicion.y -=
        jugador.tamano.y * 0.5f;

    posicion.y += 0.04f;

    float escalaY =
        jugador.aplastado
        ? 0.28f
        : 1.0f;

    Vector3 escala =
    {
        ESCALA_MODELO_JUGADOR_3D,
        ESCALA_MODELO_JUGADOR_3D * escalaY,
        ESCALA_MODELO_JUGADOR_3D
    };

    DrawModelEx(
        *modelo,
        posicion,
        { 0.0f, 1.0f, 0.0f },
        ObtenerAnguloModeloJugadorCompartido(
            jugador
        ),
        escala,
        color
    );

    if (
        jugador.golpeando &&
        !jugador.aplastado
    )
    {
        Vector3 golpe =
        {
            jugador.posicion.x +
                jugador.direccionMirada.x * 0.72f,
            jugador.posicion.y + 0.12f,
            jugador.posicion.z +
                jugador.direccionMirada.z * 0.72f
        };

        DrawSphere(
            golpe,
            0.18f,
            Fade(color, 0.88f)
        );
    }
}


void DibujarModeloJugadorEnPosicion(
    Vector3 posicionPies,
    float anguloY,
    Color color,
    float escala
)
{
    InicializarModeloJugadorCompartido();

    const Model* modelo =
        ObtenerModeloJugadorCompartidoRender();

    if (modelo == nullptr)
    {
        float factorEscala =
            escala /
            ESCALA_MODELO_JUGADOR_3D;

        float anchoFallback =
            0.78f * factorEscala;

        float altoFallback =
            1.44f * factorEscala;

        DrawCube(
            {
                posicionPies.x,
                posicionPies.y +
                    altoFallback * 0.5f,
                posicionPies.z
            },
            anchoFallback,
            altoFallback,
            anchoFallback,
            color
        );

        return;
    }

    ActualizarAnimacionModeloJugadorCompartido();

    DrawModelEx(
        *modelo,
        posicionPies,
        { 0.0f, 1.0f, 0.0f },
        anguloY,
        { escala, escala, escala },
        TinteModeloJugador(color)
    );
}


void DescargarModeloJugadorCompartido()
{
    RecursoModeloJugadorCompartido& recurso =
        recursoModeloJugador;

    if (recurso.animaciones != nullptr)
    {
        UnloadModelAnimations(
            recurso.animaciones,
            recurso.cantidadAnimaciones
        );

        recurso.animaciones = nullptr;
        recurso.cantidadAnimaciones = 0;
        recurso.indiceIdle = -1;
        recurso.ultimoFotogramaIdleAplicado = -1;
    }

    if (recurso.modeloCargado)
    {
        UnloadModel(
            recurso.modelo
        );

        recurso.modelo = {};
        recurso.modeloCargado = false;
    }
}
