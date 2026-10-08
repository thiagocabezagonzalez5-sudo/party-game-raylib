#include "Board/CatalogoTableros.h"

#include "Board/MallaTablero.h"

#include "Gameplay/PartidaTablero.h"
#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


static const int ANCHO_VISTA_PREVIA = 640;
static const int ALTO_VISTA_PREVIA = 400;
static const float SEGUNDOS_POR_RONDA_PREVIA = 3.5f;


//==================================================
// CATALOGO
//==================================================

int ObtenerCantidadTableros()
{
    return CANTIDAD_TABLEROS;
}


const DefinicionTablero& ObtenerDefinicionTablero(
    IdTablero id
)
{
    if (id == TABLERO_FORJA_VOLCAN)
    {
        return ObtenerDefinicionTableroForjaVolcan();
    }

    // Cualquier id invalido cae en el primer tablero.
    return ObtenerDefinicionTableroArboleda();
}


const char* ObtenerNombreTablero(IdTablero id)
{
    return ObtenerDefinicionTablero(id).nombre;
}


const char* ObtenerHistoriaTablero(IdTablero id)
{
    return ObtenerDefinicionTablero(id).historia;
}


const char* ObtenerObjetivoTablero(IdTablero id)
{
    return ObtenerDefinicionTablero(id).objetivo;
}


const char* ObtenerGimmickTablero(IdTablero id)
{
    return ObtenerDefinicionTablero(id).gimmick;
}


const char* ObtenerDificultadTablero(IdTablero id)
{
    return ObtenerDefinicionTablero(id).dificultad;
}


Color ObtenerColorTemaTablero(IdTablero id)
{
    return ObtenerDefinicionTablero(id).colorTema;
}


//==================================================
// VISTA PREVIA
//==================================================

// Estado interno de la vista previa. No pertenece a ninguna partida.
static RenderTexture2D texturaVistaPrevia{};
static bool texturaVistaPreviaCargada = false;
static PartidaTablero partidaVistaPrevia;
static int idVistaPrevia = -1;
static int rondaVistaPrevia = 0;


static void PrepararPartidaVistaPrevia(
    IdTablero id
)
{
    const DefinicionTablero& definicion =
        ObtenerDefinicionTablero(id);

    partidaVistaPrevia = PartidaTablero{};
    partidaVistaPrevia.idTablero = id;
    partidaVistaPrevia.definicion = &definicion;

    definicion.Construir(partidaVistaPrevia.tablero);

    if (definicion.cantidadCasillasTrofeo > 0)
    {
        partidaVistaPrevia.casillaTrofeo =
            definicion.casillasTrofeo[0];
    }

    partidaVistaPrevia.rondaActual = 0;
    idVistaPrevia = (int)id;
    rondaVistaPrevia = 0;
}


void DibujarVistaPreviaTablero(
    IdTablero id,
    Rectangle areaPantalla,
    float tiempo
)
{
    if (
        id < 0 ||
        id >= CANTIDAD_TABLEROS ||
        areaPantalla.width <= 1.0f ||
        areaPantalla.height <= 1.0f
    )
    {
        return;
    }

    if (!texturaVistaPreviaCargada)
    {
        texturaVistaPrevia =
            LoadRenderTexture(
                ANCHO_VISTA_PREVIA,
                ALTO_VISTA_PREVIA
            );

        texturaVistaPreviaCargada =
            texturaVistaPrevia.id != 0;

        if (!texturaVistaPreviaCargada)
        {
            return;
        }
    }

    if (idVistaPrevia != (int)id)
    {
        PrepararPartidaVistaPrevia(id);
    }

    const DefinicionTablero& definicion =
        *partidaVistaPrevia.definicion;

    // El gimmick avanza de "ronda" cada pocos segundos para mostrarlo.
    int ronda =
        1 + (int)(tiempo / SEGUNDOS_POR_RONDA_PREVIA);

    if (ronda != rondaVistaPrevia)
    {
        rondaVistaPrevia = ronda;
        partidaVistaPrevia.rondaActual = ronda;

        if (definicion.AlIniciarRonda != nullptr)
        {
            definicion.AlIniciarRonda(partidaVistaPrevia);
        }
    }

    partidaVistaPrevia.tiempoTablero = tiempo;

    if (definicion.ActualizarGimmick != nullptr)
    {
        definicion.ActualizarGimmick(
            partidaVistaPrevia,
            GetFrameTime()
        );
    }

    // Camara orbitando alrededor del objetivo de la camara de partida.
    float dx =
        definicion.camaraPosicion.x - definicion.camaraObjetivo.x;

    float dz =
        definicion.camaraPosicion.z - definicion.camaraObjetivo.z;

    float escalaPrevia = definicion.camaraEscalaVistaPrevia;
    float radio = std::sqrt(dx * dx + dz * dz) * escalaPrevia;
    float angulo = tiempo * 0.35f;

    Camera3D camara{};
    camara.position =
    {
        definicion.camaraObjetivo.x + std::sin(angulo) * radio,
        definicion.camaraObjetivo.y + (definicion.camaraPosicion.y - definicion.camaraObjetivo.y) * escalaPrevia,
        definicion.camaraObjetivo.z + std::cos(angulo) * radio
    };
    camara.target = definicion.camaraObjetivo;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = definicion.camaraFovy;
    camara.projection = CAMERA_PERSPECTIVE;

    BeginTextureMode(texturaVistaPrevia);

    ClearBackground(definicion.colorFondo);

    // BeginMode3D esta redefinido por EfectosVisualesMinijuegos.
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    BeginMode3D(camara);
    DibujarEscenaTablero3D(partidaVistaPrevia);
    EndMode3D();

    EndTextureMode();

    // Recorte centrado para llenar el area sin deformar la imagen.
    float aspectoArea = areaPantalla.width / areaPantalla.height;
    float aspectoTextura =
        (float)ANCHO_VISTA_PREVIA / (float)ALTO_VISTA_PREVIA;

    float anchoOrigen = (float)ANCHO_VISTA_PREVIA;
    float altoOrigen = (float)ALTO_VISTA_PREVIA;

    if (aspectoArea > aspectoTextura)
    {
        altoOrigen = anchoOrigen / aspectoArea;
    }
    else
    {
        anchoOrigen = altoOrigen * aspectoArea;
    }

    Rectangle origen =
    {
        ((float)ANCHO_VISTA_PREVIA - anchoOrigen) * 0.5f,
        ((float)ALTO_VISTA_PREVIA - altoOrigen) * 0.5f,
        anchoOrigen,
        -altoOrigen
    };

    DrawTexturePro(
        texturaVistaPrevia.texture,
        origen,
        areaPantalla,
        Vector2{ 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    DrawRectangleLinesEx(
        areaPantalla,
        3.0f,
        definicion.colorTema
    );
}


void DescargarVistaPreviaTableros()
{
    // Recursos de GPU de los tableros (mallas de decoracion y de ruta).
    for (int i = 0; i < ObtenerCantidadTableros(); i++)
    {
        const DefinicionTablero& definicion =
            ObtenerDefinicionTablero((IdTablero)i);

        if (definicion.DescargarRecursos != nullptr)
        {
            definicion.DescargarRecursos();
        }
    }

    DescargarMallasRutaTablero();
    DescargarMaterialMallasTablero();

    if (texturaVistaPreviaCargada)
    {
        UnloadRenderTexture(texturaVistaPrevia);
        texturaVistaPrevia = RenderTexture2D{};
        texturaVistaPreviaCargada = false;
    }

    idVistaPrevia = -1;
}
