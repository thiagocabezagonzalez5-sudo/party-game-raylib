#pragma once

#include "raylib.h"

#include <cmath>


//==================================================
// TEMAS VISUALES
//==================================================

enum TemaVisualMinijuego
{
    TEMA_VISUAL_NINGUNO = 0,
    TEMA_VISUAL_LAVA,
    TEMA_VISUAL_NIEVE,
    TEMA_VISUAL_CUEVA,
    TEMA_VISUAL_MAGNETICO
};


//==================================================
// TEXTURAS OPCIONALES
//==================================================
//
// Son puntos de entrada para arte real. Ninguna textura es obligatoria:
// mientras el archivo no exista el escenario sigue usando primitivas 3D.
//==================================================

struct TexturaOpcionalMinijuego
{
    Texture2D textura{};
    const char* ruta = nullptr;
    bool cargada = false;
};


struct TexturasTematicasMinijuegos
{
    TexturaOpcionalMinijuego lavaSuelo;
    TexturaOpcionalMinijuego lavaFondo;

    TexturaOpcionalMinijuego nieveSuelo;
    TexturaOpcionalMinijuego nieveFondo;

    TexturaOpcionalMinijuego cuevaRoca;
    TexturaOpcionalMinijuego cuevaTaladro;

    TexturaOpcionalMinijuego magneticoMetal;
    TexturaOpcionalMinijuego magneticoEnergia;

    bool inicializadas = false;
};


struct EstadoEfectosVisualesMinijuegos
{
    TemaVisualMinijuego tema = TEMA_VISUAL_NINGUNO;

    float tiempoGlobal = 0.0f;

    float tiempoTemblor = 0.0f;
    float duracionTemblor = 0.0f;
    float intensidadTemblor = 0.0f;

    int direccionTaladros = 0;
    float progresoTaladros = 0.0f;
    bool taladrosSeleccionados = false;
};


inline EstadoEfectosVisualesMinijuegos&
ObtenerEstadoEfectosVisualesMinijuegos()
{
    static EstadoEfectosVisualesMinijuegos estado;
    return estado;
}


inline TexturasTematicasMinijuegos&
ObtenerTexturasTematicasMinijuegos()
{
    static TexturasTematicasMinijuegos texturas;
    return texturas;
}


inline void PrepararSlotTexturaMinijuego(
    TexturaOpcionalMinijuego& slot,
    const char* ruta
)
{
    slot = {};
    slot.ruta = ruta;

    if (ruta == nullptr || !FileExists(ruta))
    {
        return;
    }

    slot.textura = LoadTexture(ruta);
    slot.cargada = IsTextureValid(slot.textura);

    if (!slot.cargada)
    {
        slot.textura = {};
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar textura opcional: %s",
            ruta
        );
    }
}


inline void InicializarTexturasTematicasMinijuegos()
{
    TexturasTematicasMinijuegos& texturas =
        ObtenerTexturasTematicasMinijuegos();

    if (texturas.inicializadas)
    {
        return;
    }

    PrepararSlotTexturaMinijuego(
        texturas.lavaSuelo,
        "Assets/Texturas/Minijuegos/ColorSeguro/lava_suelo.png"
    );

    PrepararSlotTexturaMinijuego(
        texturas.lavaFondo,
        "Assets/Texturas/Minijuegos/ColorSeguro/fondo_volcanes.png"
    );

    PrepararSlotTexturaMinijuego(
        texturas.nieveSuelo,
        "Assets/Texturas/Minijuegos/Pelotas/nieve_suelo.png"
    );

    PrepararSlotTexturaMinijuego(
        texturas.nieveFondo,
        "Assets/Texturas/Minijuegos/Pelotas/fondo_montanas.png"
    );

    PrepararSlotTexturaMinijuego(
        texturas.cuevaRoca,
        "Assets/Texturas/Minijuegos/RefugioTaladros/roca_cueva.png"
    );

    PrepararSlotTexturaMinijuego(
        texturas.cuevaTaladro,
        "Assets/Texturas/Minijuegos/RefugioTaladros/metal_taladro.png"
    );

    PrepararSlotTexturaMinijuego(
        texturas.magneticoMetal,
        "Assets/Texturas/Minijuegos/TormentaMagnetica/metal_arena.png"
    );

    PrepararSlotTexturaMinijuego(
        texturas.magneticoEnergia,
        "Assets/Texturas/Minijuegos/TormentaMagnetica/energia.png"
    );

    texturas.inicializadas = true;
}


inline void DescargarSlotTexturaMinijuego(
    TexturaOpcionalMinijuego& slot
)
{
    if (slot.cargada && IsTextureValid(slot.textura))
    {
        UnloadTexture(slot.textura);
    }

    slot = {};
}


inline void DescargarTexturasTematicasMinijuegos()
{
    TexturasTematicasMinijuegos& texturas =
        ObtenerTexturasTematicasMinijuegos();

    if (!texturas.inicializadas)
    {
        return;
    }

    DescargarSlotTexturaMinijuego(texturas.lavaSuelo);
    DescargarSlotTexturaMinijuego(texturas.lavaFondo);
    DescargarSlotTexturaMinijuego(texturas.nieveSuelo);
    DescargarSlotTexturaMinijuego(texturas.nieveFondo);
    DescargarSlotTexturaMinijuego(texturas.cuevaRoca);
    DescargarSlotTexturaMinijuego(texturas.cuevaTaladro);
    DescargarSlotTexturaMinijuego(texturas.magneticoMetal);
    DescargarSlotTexturaMinijuego(texturas.magneticoEnergia);

    texturas.inicializadas = false;
}


//==================================================
// TEMBLOR DE CAMARA GENERAL
//==================================================

inline void ActualizarEfectosVisualesMinijuegos(
    float deltaTime
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    estado.tiempoGlobal += deltaTime;

    if (estado.tiempoTemblor > 0.0f)
    {
        estado.tiempoTemblor -= deltaTime;

        if (estado.tiempoTemblor < 0.0f)
        {
            estado.tiempoTemblor = 0.0f;
        }
    }
}


inline void ActivarTemblorCamaraGeneral(
    float intensidad = 0.16f,
    float duracion = 0.26f
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    if (
        intensidad > estado.intensidadTemblor ||
        estado.tiempoTemblor <= 0.0f
    )
    {
        estado.intensidadTemblor = intensidad;
    }

    if (duracion > estado.tiempoTemblor)
    {
        estado.tiempoTemblor = duracion;
        estado.duracionTemblor = duracion;
    }
}


inline Camera3D AplicarTemblorGeneralACamara(
    Camera3D camara
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    if (estado.tiempoTemblor <= 0.0f)
    {
        return camara;
    }

    float porcentaje =
        estado.duracionTemblor > 0.0f
            ? estado.tiempoTemblor / estado.duracionTemblor
            : 0.0f;

    if (porcentaje < 0.0f) porcentaje = 0.0f;
    if (porcentaje > 1.0f) porcentaje = 1.0f;

    float amplitud =
        estado.intensidadTemblor * porcentaje;

    float dx =
        std::sin(estado.tiempoGlobal * 47.0f) * amplitud;

    float dy =
        std::cos(estado.tiempoGlobal * 61.0f) *
        amplitud * 0.72f;

    float dz =
        std::sin(estado.tiempoGlobal * 37.0f + 0.8f) *
        amplitud * 0.46f;

    camara.position.x += dx;
    camara.position.y += dy;
    camara.position.z += dz;

    camara.target.x += dx * 0.34f;
    camara.target.y += dy * 0.28f;
    camara.target.z += dz * 0.34f;

    return camara;
}


//==================================================
// CONFIGURACION DE TEMAS
//==================================================

inline void SeleccionarTemaVisualMinijuego(
    TemaVisualMinijuego tema
)
{
    ObtenerEstadoEfectosVisualesMinijuegos().tema = tema;
}


inline void ConfigurarTaladrosVisualesMinijuego(
    int direccion,
    float progreso,
    bool seleccionados
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    if (progreso < 0.0f) progreso = 0.0f;
    if (progreso > 1.0f) progreso = 1.0f;

    estado.direccionTaladros = direccion;
    estado.progresoTaladros = progreso;
    estado.taladrosSeleccionados = seleccionados;
}


//==================================================
// LAVA / VOLCANES
//==================================================

inline void DibujarTemaLava()
{
    DrawCube(
        { 0.0f, -4.8f, 0.0f },
        42.0f,
        0.30f,
        42.0f,
        Color{ 238, 73, 22, 255 }
    );

    DrawCube(
        { 0.0f, -4.58f, 0.0f },
        42.0f,
        0.08f,
        42.0f,
        Color{ 255, 132, 25, 255 }
    );

    const Vector3 volcanes[5] =
    {
        { -13.0f, -4.0f, -16.0f },
        {  -6.5f, -4.0f, -19.0f },
        {   2.5f, -4.0f, -20.0f },
        {  10.5f, -4.0f, -17.0f },
        {  16.0f, -4.0f, -13.0f }
    };

    for (int i = 0; i < 5; i++)
    {
        Vector3 base = volcanes[i];
        Vector3 cima =
        {
            base.x,
            5.0f + (i % 2) * 1.4f,
            base.z
        };

        DrawCylinderEx(
            base,
            cima,
            4.4f + (i % 3) * 0.55f,
            1.15f,
            12,
            Color{ 63, 48, 47, 255 }
        );

        DrawCylinderEx(
            { cima.x, cima.y - 0.35f, cima.z },
            { cima.x, cima.y + 0.05f, cima.z },
            1.22f,
            0.88f,
            12,
            Color{ 221, 69, 25, 255 }
        );
    }

    float pulso =
        0.65f +
        std::sin(
            ObtenerEstadoEfectosVisualesMinijuegos().tiempoGlobal *
            2.6f
        ) * 0.10f;

    DrawSphere(
        { 2.5f, 6.0f, -20.0f },
        pulso,
        Color{ 255, 169, 45, 255 }
    );
}


//==================================================
// NIEVE / CUMBRE ALTA
//==================================================

inline void DibujarTemaNieve()
{
    // Dos cuerpos bajo la arena prolongan visualmente la montana muchos
    // metros hacia abajo para que la cumbre se sienta realmente alta.
    DrawCylinderEx(
        { 0.0f, -25.0f, 0.0f },
        { 0.0f, -0.85f, 0.0f },
        12.5f,
        6.6f,
        18,
        Color{ 108, 126, 145, 255 }
    );

    DrawCylinderEx(
        { 0.0f, -5.2f, 0.0f },
        { 0.0f, -0.72f, 0.0f },
        7.7f,
        6.9f,
        18,
        Color{ 225, 237, 244, 255 }
    );

    const Vector3 montanas[6] =
    {
        { -18.0f, -18.0f, -24.0f },
        { -11.0f, -17.0f, -29.0f },
        {  -3.0f, -19.0f, -31.0f },
        {   6.0f, -18.0f, -30.0f },
        {  14.0f, -17.0f, -26.0f },
        {  21.0f, -18.0f, -20.0f }
    };

    for (int i = 0; i < 6; i++)
    {
        Vector3 base = montanas[i];
        Vector3 cima =
        {
            base.x,
            4.5f + (i % 3) * 2.0f,
            base.z
        };

        DrawCylinderEx(
            base,
            cima,
            7.0f,
            0.4f,
            10,
            Color{ 102, 123, 145, 255 }
        );

        DrawCylinderEx(
            { cima.x, cima.y - 4.0f, cima.z },
            cima,
            2.6f,
            0.25f,
            10,
            RAYWHITE
        );
    }
}


//==================================================
// CUEVA + TALADROS
//==================================================

inline void DibujarTaladrosCueva()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    float progreso = estado.progresoTaladros;

    // Durante el aviso el taladro elegido no se queda quieto: asoma y
    // vibra de forma visible. Durante el ataque el progreso viene de la
    // logica de ZonaPruebas y recorre ida y vuelta.
    if (estado.taladrosSeleccionados)
    {
        float vibracion =
            0.075f +
            std::sin(estado.tiempoGlobal * 13.0f) * 0.045f;

        if (vibracion < 0.02f) vibracion = 0.02f;

        progreso += vibracion;
        if (progreso > 0.18f) progreso = 0.18f;
    }

    const float carriles[4] =
    {
        -3.6f,
        -1.2f,
        1.2f,
        3.6f
    };

    for (int i = 0; i < 4; i++)
    {
        Vector3 inicio{};
        Vector3 finCuerpo{};
        Vector3 punta{};

        if (
            estado.direccionTaladros == 0 ||
            estado.direccionTaladros == 1
        )
        {
            float origen =
                estado.direccionTaladros == 0
                    ? -5.75f
                    : 5.75f;

            float signo =
                estado.direccionTaladros == 0
                    ? 1.0f
                    : -1.0f;

            float avance = progreso * 8.8f;

            inicio =
            {
                carriles[i],
                0.82f,
                origen + signo * avance
            };

            finCuerpo =
            {
                carriles[i],
                0.82f,
                inicio.z + signo * 1.65f
            };

            punta =
            {
                carriles[i],
                0.82f,
                finCuerpo.z + signo * 0.85f
            };
        }
        else
        {
            float origen =
                estado.direccionTaladros == 2
                    ? -6.55f
                    : 6.55f;

            float signo =
                estado.direccionTaladros == 2
                    ? 1.0f
                    : -1.0f;

            float avance = progreso * 10.2f;

            inicio =
            {
                origen + signo * avance,
                0.82f,
                carriles[i] * 0.86f
            };

            finCuerpo =
            {
                inicio.x + signo * 1.65f,
                0.82f,
                inicio.z
            };

            punta =
            {
                finCuerpo.x + signo * 0.85f,
                0.82f,
                inicio.z
            };
        }

        Color metal =
            estado.taladrosSeleccionados
                ? Color{ 190, 116, 50, 255 }
                : Color{ 92, 98, 108, 255 };

        DrawCircle3D(
            { finCuerpo.x, 0.025f, finCuerpo.z },
            0.48f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(BLACK, 0.28f)
        );

        DrawCylinderEx(
            inicio,
            finCuerpo,
            0.33f,
            0.33f,
            10,
            metal
        );

        DrawCylinderEx(
            finCuerpo,
            punta,
            0.48f,
            0.02f,
            12,
            Color{ 165, 170, 178, 255 }
        );

        for (int aro = 0; aro < 3; aro++)
        {
            float t = (float)(aro + 1) / 4.0f;

            Vector3 centro =
            {
                finCuerpo.x + (punta.x - finCuerpo.x) * t,
                finCuerpo.y,
                finCuerpo.z + (punta.z - finCuerpo.z) * t
            };

            DrawSphere(
                centro,
                0.37f - t * 0.20f,
                Color{ 105, 110, 118, 255 }
            );
        }
    }
}


inline void DibujarTemaCueva()
{
    // No hay una losa completa sobre el centro porque la camara de este
    // minijuego esta elevada. En su lugar usamos paredes, pilares y roca
    // superior solo en los bordes para crear techo sin tapar la accion.
    DrawCube(
        { -6.8f, 2.4f, 0.0f },
        2.0f,
        7.4f,
        12.0f,
        Color{ 58, 51, 48, 255 }
    );

    DrawCube(
        { 6.8f, 2.4f, 0.0f },
        2.0f,
        7.4f,
        12.0f,
        Color{ 58, 51, 48, 255 }
    );

    DrawCube(
        { 0.0f, 2.7f, -6.2f },
        14.0f,
        7.8f,
        1.6f,
        Color{ 52, 47, 45, 255 }
    );

    DrawCube(
        { -5.2f, 8.3f, -2.0f },
        4.8f,
        1.4f,
        8.5f,
        Color{ 48, 44, 43, 255 }
    );

    DrawCube(
        { 5.2f, 8.3f, -2.0f },
        4.8f,
        1.4f,
        8.5f,
        Color{ 48, 44, 43, 255 }
    );

    const float xEstalactitas[6] =
    {
        -5.3f,
        -3.9f,
        -2.7f,
        2.7f,
        3.9f,
        5.3f
    };

    for (int i = 0; i < 6; i++)
    {
        float z = -4.7f + (i % 2) * 1.35f;

        DrawCylinderEx(
            { xEstalactitas[i], 7.8f, z },
            { xEstalactitas[i], 5.7f - (i % 3) * 0.35f, z },
            0.44f,
            0.04f,
            8,
            Color{ 74, 66, 61, 255 }
        );
    }

    DibujarTaladrosCueva();
}


//==================================================
// ESCENARIO MAGNETICO
//==================================================

inline void DibujarTemaMagnetico()
{
    const Vector3 torres[4] =
    {
        { -8.5f, 1.8f, -8.5f },
        {  8.5f, 1.8f, -8.5f },
        { -8.5f, 1.8f,  8.5f },
        {  8.5f, 1.8f,  8.5f }
    };

    for (int i = 0; i < 4; i++)
    {
        DrawCylinder(
            torres[i],
            0.65f,
            0.65f,
            4.2f,
            10,
            Color{ 63, 73, 92, 255 }
        );

        DrawSphere(
            { torres[i].x, 4.05f, torres[i].z },
            0.58f,
            i % 2 == 0
                ? Color{ 67, 203, 235, 255 }
                : Color{ 236, 91, 149, 255 }
        );
    }
}


inline void DibujarDecoracionTemaVisualMinijuego()
{
    switch (
        ObtenerEstadoEfectosVisualesMinijuegos().tema
    )
    {
        case TEMA_VISUAL_LAVA:
            DibujarTemaLava();
            break;

        case TEMA_VISUAL_NIEVE:
            DibujarTemaNieve();
            break;

        case TEMA_VISUAL_CUEVA:
            DibujarTemaCueva();
            break;

        case TEMA_VISUAL_MAGNETICO:
            DibujarTemaMagnetico();
            break;

        case TEMA_VISUAL_NINGUNO:
            break;
    }
}


//==================================================
// BEGINMODE3D CENTRALIZADO
//==================================================
//
// La llamada real a raylib se declara antes del macro. Todos los archivos
// que incluyen TiposMinijuegos pasan por aqui: eso hace que el temblor del
// ground pound funcione sin duplicar codigo en cada minijuego.
//==================================================

inline void BeginMode3DConEfectosMinijuego(
    Camera3D camara
)
{
    Camera3D camaraFinal =
        AplicarTemblorGeneralACamara(camara);

    BeginMode3D(camaraFinal);
    DibujarDecoracionTemaVisualMinijuego();
}


#define BeginMode3D(...) BeginMode3DConEfectosMinijuego(__VA_ARGS__)
