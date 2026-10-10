#pragma once

#include "Core/RecursosJuego.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <cstring>


//==================================================
// MODELOS COMPARTIDOS DE ESCENARIOS RETRO 3D
//==================================================
//
// Los modelos de decoracion se cargan una sola vez para que todos los
// minijuegos que usan el mismo tema compartan la memoria de GPU. Cada slot
// es opcional: si el archivo no existe, el escenario conserva sus primitivas
// 3D como fallback y el juego sigue funcionando.
//==================================================

struct RecursoModeloEscenarioRetro3D
{
    Model modelo{};
    const char* ruta = nullptr;
    Vector3 dimensiones{};
    bool cargado = false;
    bool cargaIntentada = false;
    int materialColor = -1;
};

enum PivoteModeloEscenarioRetro3D
{
    PIVOTE_ESCENARIO_ORIGINAL,
    PIVOTE_ESCENARIO_CENTRAR_BASE
};

struct DefinicionModeloEscenarioRetro3D
{
    const char* ruta = nullptr;
    PivoteModeloEscenarioRetro3D pivote = PIVOTE_ESCENARIO_ORIGINAL;
    float rotacionX = 0.0f;
    // Indice de primitive/malla del GLB, NO indice interno de material.
    // Puede señalar COLOR_DINAMICO o BOMBILLAS segun el paquete.
    int mallaColor = -1;
};

// Vista del paquete: la memoria pertenece al almacen compartido.
struct PaqueteModelosEscenarioRetro3D
{
    RecursoModeloEscenarioRetro3D* recursos = nullptr;
    const DefinicionModeloEscenarioRetro3D* definiciones = nullptr;
    int cantidad = 0;
};


// Copias CPU de reposo; comparten las dos mallas/VBO del unico modelo ave.
struct AlasDescensoNubesRetro3D
{
    float* vertices[2]{};
    float* normales[2]{};
    int inicioAlas = -1;
    bool intentada = false;
};

// Reposo de una unica malla/VBO por recurso; se restaura tras cada instancia.
struct AnimacionMallaPescaRetro3D
{
    float* vertices = nullptr;
    float* normales = nullptr;
    unsigned char* colores = nullptr;
    bool intentada = false;
};

// Solo copias CPU de reposo; cada pieza mantiene una unica malla/VBO.
struct AnimacionRodillosRetro3D
{
    unsigned char* colores[2]{};
    float* vertices = nullptr;
    bool intentada = false;
};

struct ModelosEscenariosRetro3D
{
    RecursoModeloEscenarioRetro3D montanaLava;
    RecursoModeloEscenarioRetro3D ultimoAsiento[CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D];
    RecursoModeloEscenarioRetro3D cajasPuerto[CANTIDAD_MODELOS_CAJAS_PUERTO_3D];
    RecursoModeloEscenarioRetro3D laberintoJade[CANTIDAD_MODELOS_LABERINTO_JADE_3D];
    RecursoModeloEscenarioRetro3D vetaCristal[CANTIDAD_MODELOS_VETA_CRISTAL_3D];
    RecursoModeloEscenarioRetro3D capsulasBarajadas[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D];
    RecursoModeloEscenarioRetro3D bateoMeteorico[CANTIDAD_MODELOS_BATEO_METEORICO_3D];
    RecursoModeloEscenarioRetro3D racimoToxico[CANTIDAD_MODELOS_RACIMO_TOXICO_3D];
    RecursoModeloEscenarioRetro3D tesoreroCercado[CANTIDAD_MODELOS_TESORERO_CERCADO_3D];
    RecursoModeloEscenarioRetro3D descensoNubes[CANTIDAD_MODELOS_DESCENSO_NUBES_3D];
    RecursoModeloEscenarioRetro3D voleaMagma[CANTIDAD_MODELOS_VOLEA_MAGMA_3D];
    RecursoModeloEscenarioRetro3D parejasGlaciar[CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D];
    RecursoModeloEscenarioRetro3D esferasCanon[CANTIDAD_MODELOS_ESFERAS_CANON_3D];
    RecursoModeloEscenarioRetro3D pescaIslena[CANTIDAD_MODELOS_PESCA_ISLENA_3D];
    RecursoModeloEscenarioRetro3D rodillosNeon[CANTIDAD_MODELOS_RODILLOS_NEON_3D];
    AnimacionRodillosRetro3D animacionesRodillos[4]; // suelo, dos LEDs, linea
    AnimacionMallaPescaRetro3D animacionesPesca[5]; // gaviota, humo y tres colas
    AlasDescensoNubesRetro3D alasNubes;
    bool inicializados = false;
};


inline ModelosEscenariosRetro3D& ObtenerModelosEscenariosRetro3D()
{
    static ModelosEscenariosRetro3D recursos;
    return recursos;
}


inline BoundingBox RotarLimitesModeloEscenarioRetro3D(
    BoundingBox limites,
    Matrix rotacion
)
{
    Vector3 esquinas[8] =
    {
        { limites.min.x, limites.min.y, limites.min.z },
        { limites.max.x, limites.min.y, limites.min.z },
        { limites.min.x, limites.max.y, limites.min.z },
        { limites.max.x, limites.max.y, limites.min.z },
        { limites.min.x, limites.min.y, limites.max.z },
        { limites.max.x, limites.min.y, limites.max.z },
        { limites.min.x, limites.max.y, limites.max.z },
        { limites.max.x, limites.max.y, limites.max.z }
    };

    Vector3 primera = Vector3Transform(esquinas[0], rotacion);
    BoundingBox rotados = { primera, primera };

    for (int i = 1; i < 8; i++)
    {
        Vector3 esquina = Vector3Transform(esquinas[i], rotacion);

        if (esquina.x < rotados.min.x) rotados.min.x = esquina.x;
        if (esquina.y < rotados.min.y) rotados.min.y = esquina.y;
        if (esquina.z < rotados.min.z) rotados.min.z = esquina.z;

        if (esquina.x > rotados.max.x) rotados.max.x = esquina.x;
        if (esquina.y > rotados.max.y) rotados.max.y = esquina.y;
        if (esquina.z > rotados.max.z) rotados.max.z = esquina.z;
    }

    return rotados;
}


inline void PrepararSlotModeloEscenarioRetro3D(
    RecursoModeloEscenarioRetro3D& recurso,
    const char* ruta,
    float rotacionX,
    PivoteModeloEscenarioRetro3D pivote = PIVOTE_ESCENARIO_ORIGINAL,
    int mallaColor = -1
)
{
    if (recurso.cargaIntentada) return;
    recurso.cargaIntentada = true;
    recurso.ruta = ruta;

    if (ruta == nullptr || !FileExists(ruta))
    {
        TraceLog(LOG_WARNING, "Modelo de escenario ausente; se usan primitivas: %s",
            ruta != nullptr ? ruta : "(sin ruta)");
        return;
    }

    recurso.modelo = LoadModel(ruta);

    if (recurso.modelo.meshCount <= 0 || recurso.modelo.meshes == nullptr ||
        recurso.modelo.materialCount <= 0 || recurso.modelo.materials == nullptr ||
        recurso.modelo.meshMaterial == nullptr)
    {
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar el modelo de escenario: %s",
            ruta
        );
        return;
    }

    for (int i = 0; i < recurso.modelo.meshCount; i++)
    {
        int material = recurso.modelo.meshMaterial[i];
        if (recurso.modelo.meshes[i].vertexCount <= 0 ||
            recurso.modelo.meshes[i].vertices == nullptr ||
            material < 0 || material >= recurso.modelo.materialCount ||
            recurso.modelo.materials[material].maps == nullptr)
        {
            UnloadModel(recurso.modelo);
            recurso.modelo = {};
            TraceLog(LOG_WARNING, "Malla/material de escenario invalido: %s", ruta);
            return;
        }
    }

    if (mallaColor >= 0)
    {
        if (mallaColor >= recurso.modelo.meshCount)
        {
            UnloadModel(recurso.modelo);
            recurso.modelo = {};
            TraceLog(LOG_WARNING, "Falta la malla del color de estado: %s", ruta);
            return;
        }
        recurso.materialColor = recurso.modelo.meshMaterial[mallaColor];
    }

    // Las piezas modulares ya llevan su origen y alturas locales correctos.
    // LoadModel conserva las transformaciones importadas y colores de vertice.
    if (pivote == PIVOTE_ESCENARIO_ORIGINAL)
    {
        BoundingBox limites = GetModelBoundingBox(recurso.modelo);
        recurso.dimensiones = Vector3Subtract(limites.max, limites.min);
        if (!std::isfinite(recurso.dimensiones.x) || !std::isfinite(recurso.dimensiones.y) ||
            !std::isfinite(recurso.dimensiones.z) || recurso.dimensiones.x <= 0.001f ||
            recurso.dimensiones.y <= 0.001f || recurso.dimensiones.z <= 0.001f)
        {
            UnloadModel(recurso.modelo);
            recurso.modelo = {};
            TraceLog(LOG_WARNING, "El modelo de escenario no tiene dimensiones validas: %s", ruta);
            return;
        }
        recurso.cargado = true;
        return;
    }

    Model modeloSinTransformacion = recurso.modelo;
    modeloSinTransformacion.transform = MatrixIdentity();

    BoundingBox limites = GetModelBoundingBox(modeloSinTransformacion);
    Matrix rotacion = MatrixRotateX(rotacionX * DEG2RAD);
    BoundingBox limitesRotados =
        RotarLimitesModeloEscenarioRetro3D(limites, rotacion);

    recurso.dimensiones =
    {
        limitesRotados.max.x - limitesRotados.min.x,
        limitesRotados.max.y - limitesRotados.min.y,
        limitesRotados.max.z - limitesRotados.min.z
    };

    if (
        recurso.dimensiones.x <= 0.001f ||
        recurso.dimensiones.y <= 0.001f ||
        recurso.dimensiones.z <= 0.001f
    )
    {
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.dimensiones = {};

        TraceLog(
            LOG_WARNING,
            "El modelo de escenario no tiene dimensiones validas: %s",
            ruta
        );
        return;
    }

    Vector3 centroBase =
    {
        (limitesRotados.min.x + limitesRotados.max.x) * 0.5f,
        limitesRotados.min.y,
        (limitesRotados.min.z + limitesRotados.max.z) * 0.5f
    };

    Matrix centrarBase = MatrixTranslate(
        -centroBase.x,
        -centroBase.y,
        -centroBase.z
    );

    recurso.modelo.transform = MatrixMultiply(rotacion, centrarBase);
    recurso.cargado = true;
}

inline void CargarPaqueteModelosEscenarioRetro3D(PaqueteModelosEscenarioRetro3D paquete)
{
    for (int i = 0; i < paquete.cantidad; i++)
    {
        const DefinicionModeloEscenarioRetro3D& d = paquete.definiciones[i];
        PrepararSlotModeloEscenarioRetro3D(
            paquete.recursos[i], d.ruta, d.rotacionX, d.pivote, d.mallaColor);
    }
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteUltimoAsientoRetro3D()
{
    // Los indices de color pertenecen a la version 1 del manifest del paquete.
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_ULTIMO_ASIENTO_3D[i];
        definiciones[MODELO_ASIENTO_BOMBILLA].mallaColor = 0;
        definiciones[MODELO_ASIENTO_TAZA].mallaColor = 2;
        definiciones[MODELO_ASIENTO_VALLA].mallaColor = 2;
        definiciones[MODELO_ASIENTO_NORIA_CABINA].mallaColor = 3;
        definiciones[MODELO_ASIENTO_GLOBO].mallaColor = 0;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().ultimoAsiento,
        definiciones, CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D };
}

inline bool DibujarModeloEscenarioRetro3D(
    RecursoModeloEscenarioRetro3D& recurso,
    Vector3 posicion,
    Vector3 ejeRotacion,
    float anguloGrados,
    Vector3 escala,
    Color colorEstado = WHITE
)
{
    if (!recurso.cargado) return false;

    Color anterior{};
    if (recurso.materialColor >= 0)
    {
        MaterialMap& mapa = recurso.modelo.materials[recurso.materialColor].maps[MATERIAL_MAP_DIFFUSE];
        anterior = mapa.color;
        mapa.color = colorEstado;
    }

    // Los parentesis evitan la macro de SombrasRetro para personajes. Cada
    // escenario decide sus sombras en coordenadas del mundo, sin heredar una
    // mancha de tamano humano para arena, techo o rueda. DrawModelEx compone
    // la transformacion importada y la instancia una sola vez.
    (DrawModelEx)(recurso.modelo, posicion, ejeRotacion, anguloGrados, escala, WHITE);

    if (recurso.materialColor >= 0)
        recurso.modelo.materials[recurso.materialColor].maps[MATERIAL_MAP_DIFFUSE].color = anterior;
    return true;
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteCajasPuertoRetro3D()
{
    // Primitivas de COLOR_DINAMICO segun manifest/visor, resueltas mediante
    // meshMaterial por el cargador. Todos los pivotes quedan originales.
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_CAJAS_PUERTO_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_CAJAS_PUERTO_3D[i];
        definiciones[MODELO_CAJAS_CONTENEDOR_CUERPO].mallaColor = 1;
        definiciones[MODELO_CAJAS_PUERTA_IZQUIERDA].mallaColor = 0;
        definiciones[MODELO_CAJAS_PUERTA_DERECHA].mallaColor = 0;
        definiciones[MODELO_CAJAS_CONTENEDOR_DECORACION].mallaColor = 1;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().cajasPuerto,
        definiciones, CANTIDAD_MODELOS_CAJAS_PUERTO_3D };
}

inline bool DibujarModeloCajasPuertoRetro3D(
    ModeloCajasPuerto3D pieza,
    Vector3 posicion,
    Vector3 escala = { 1.0f, 1.0f, 1.0f },
    Color colorEstado = WHITE
)
{
    return DibujarModeloEscenarioRetro3D(
        ObtenerModelosEscenariosRetro3D().cajasPuerto[pieza],
        posicion, { 0.0f, 1.0f, 0.0f }, 0.0f, escala, colorEstado);
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteLaberintoJadeRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_LABERINTO_JADE_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_LABERINTO_JADE_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_LABERINTO_JADE_3D[i];
        // Primitivas de COLOR_DINAMICO del paquete v1. El cargador obtiene
        // el indice real del material con meshMaterial, sin tenir la pieza.
        definiciones[MODELO_JADE_BANDA].mallaColor = 0;
        definiciones[MODELO_JADE_CHECKPOINT].mallaColor = 0;
        definiciones[MODELO_JADE_FLECHA].mallaColor = 0;
        definiciones[MODELO_JADE_BOQUILLA].mallaColor = 2;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().laberintoJade,
        definiciones, CANTIDAD_MODELOS_LABERINTO_JADE_3D };
}

inline bool DibujarModeloLaberintoJadeRetro3D(
    ModeloLaberintoJade3D pieza,
    Vector3 posicion,
    float anguloY = 0.0f,
    Vector3 escala = { 1.0f, 1.0f, 1.0f },
    Color colorEstado = WHITE
)
{
    // Dentro de rlPushMatrix recibe coordenadas LOCALES: DrawModelEx hereda
    // la matriz del tablero. No sumar su centro ni repetir su inclinacion.
    return DibujarModeloEscenarioRetro3D(
        ObtenerModelosEscenariosRetro3D().laberintoJade[pieza],
        posicion, { 0.0f, 1.0f, 0.0f }, anguloY, escala, colorEstado);
}

inline bool DibujarModeloUltimoAsientoRetro3D(
    ModeloUltimoAsiento3D pieza,
    Vector3 posicion,
    float anguloGrados = 0.0f,
    Vector3 ejeRotacion = { 0.0f, 1.0f, 0.0f },
    Vector3 escala = { 1.0f, 1.0f, 1.0f },
    Color colorEstado = WHITE
)
{
    return DibujarModeloEscenarioRetro3D(
        ObtenerModelosEscenariosRetro3D().ultimoAsiento[pieza],
        posicion, ejeRotacion, anguloGrados, escala, colorEstado);
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteVetaCristalRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_VETA_CRISTAL_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_VETA_CRISTAL_3D[i];
        // Unica primitive COLOR_DINAMICO del paquete v1. Se resuelve con
        // meshMaterial; el resto del paquete conserva sus materiales.
        definiciones[MODELO_VETA_MARCA].mallaColor = 0;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().vetaCristal,
        definiciones, CANTIDAD_MODELOS_VETA_CRISTAL_3D };
}

inline bool DibujarModeloVetaCristalRetro3D(
    ModeloVetaCristal3D pieza,
    Vector3 posicion,
    float anguloY = 0.0f,
    Vector3 escala = { 1.0f, 1.0f, 1.0f },
    Color colorEstado = WHITE
)
{
    // Coordenadas del mundo; no se combinan con otra matriz de rlgl.
    return DibujarModeloEscenarioRetro3D(
        ObtenerModelosEscenariosRetro3D().vetaCristal[pieza],
        posicion, { 0.0f, 1.0f, 0.0f }, anguloY, escala, colorEstado);
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteCapsulasBarajadasRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_CAPSULAS_BARAJADAS_3D[i];
        // Primitivas COLOR_DINAMICO del GLB v1; NO indices de material.
        definiciones[MODELO_CAPSULAS_TUBO].mallaColor = 1;
        definiciones[MODELO_CAPSULAS_BALIZA].mallaColor = 2;
        definiciones[MODELO_CAPSULAS_BANDA].mallaColor = 0;
        definiciones[MODELO_CAPSULAS_TAPA].mallaColor = 3;
        definiciones[MODELO_CAPSULAS_MARCADOR].mallaColor = 0;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().capsulasBarajadas,
        definiciones, CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D };
}

inline bool DibujarModeloCapsulasBarajadasRetro3D(
    ModeloCapsulasBarajadas3D pieza,
    Vector3 posicion,
    float anguloGrados = 0.0f,
    Vector3 ejeRotacion = { 0.0f, 1.0f, 0.0f },
    Vector3 escala = { 1.0f, 1.0f, 1.0f },
    Color colorEstado = WHITE,
    int mallaOmitida = -1
)
{
    RecursoModeloEscenarioRetro3D& recurso =
        ObtenerModelosEscenariosRetro3D().capsulasBarajadas[pieza];
    if (!recurso.cargado) return false;
    if (mallaOmitida < 0)
        return DibujarModeloEscenarioRetro3D(recurso, posicion, ejeRotacion,
            anguloGrados, escala, colorEstado);

    // Monitor y tubo incluyen barras/burbujas estaticas. Sus animaciones
    // siguen dibujandose en C++: omitir solo esa malla del GLB evita duplicarlas.
    // Esta vista no es propietaria: comparte meshes, VBO y materiales. No se
    // carga, sube ni descarga memoria al dibujar, ni se modifica el Model real.
    RecursoModeloEscenarioRetro3D vista = recurso;
    vista.modelo.meshCount = 1;
    for (int i = 0; i < recurso.modelo.meshCount; i++)
    {
        if (i == mallaOmitida) continue;
        vista.modelo.meshes = &recurso.modelo.meshes[i];
        vista.modelo.meshMaterial = &recurso.modelo.meshMaterial[i];
        DibujarModeloEscenarioRetro3D(vista, posicion, ejeRotacion,
            anguloGrados, escala, colorEstado);
    }
    return true;
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteBateoMeteoricoRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_BATEO_METEORICO_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_BATEO_METEORICO_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_BATEO_METEORICO_3D[i];
        // Primitivas del GLB v1, resueltas con meshMaterial al cargar.
        definiciones[MODELO_BATEO_CARRIL].mallaColor = 3;
        definiciones[MODELO_BATEO_CAMPO].mallaColor = 4;
        definiciones[MODELO_BATEO_BATE].mallaColor = 1;
        definiciones[MODELO_BATEO_FAROL].mallaColor = 2; // BOMBILLAS
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().bateoMeteorico,
        definiciones, CANTIDAD_MODELOS_BATEO_METEORICO_3D };
}

inline bool DibujarModeloBateoMeteoricoRetro3D(
    ModeloBateoMeteorico3D pieza,
    Vector3 posicion,
    float anguloGrados = 0.0f,
    Vector3 ejeRotacion = { 0.0f, 1.0f, 0.0f },
    Vector3 escala = { 1.0f, 1.0f, 1.0f },
    Color colorEstado = WHITE
)
{
    return DibujarModeloEscenarioRetro3D(
        ObtenerModelosEscenariosRetro3D().bateoMeteorico[pieza],
        posicion, ejeRotacion, anguloGrados, escala, colorEstado);
}

inline bool DibujarFarolBateoMeteoricoRetro3D(Vector3 posicion, float parpadeo)
{
    RecursoModeloEscenarioRetro3D& recurso =
        ObtenerModelosEscenariosRetro3D().bateoMeteorico[MODELO_BATEO_FAROL];
    if (!recurso.cargado) return false;

    // Vista sin propiedad: comparte VBO/materiales y conserva el Model real.
    // Solo la bombilla pulsa alrededor de su centro local; poste/herrajes fijos.
    RecursoModeloEscenarioRetro3D vista = recurso;
    vista.modelo.meshCount = 1;
    for (int i = 0; i < recurso.modelo.meshCount; i++)
    {
        vista.modelo.meshes = &recurso.modelo.meshes[i];
        vista.modelo.meshMaterial = &recurso.modelo.meshMaterial[i];
        bool bombilla = i == 2;
        float escala = bombilla ? 1.0f + (0.03f / 0.18f) * parpadeo : 1.0f;
        Vector3 origen = posicion;
        if (bombilla) origen.y += 1.97f * (1.0f - escala);
        Color brillo = { 255, 230, (unsigned char)(150 + 60 * parpadeo), 255 };
        DibujarModeloEscenarioRetro3D(vista, origen, {0,1,0}, 0,
            {escala,escala,escala}, brillo);
    }
    return true;
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteRacimoToxicoRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_RACIMO_TOXICO_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_RACIMO_TOXICO_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_RACIMO_TOXICO_3D[i];
        // No hay COLOR_DINAMICO en este paquete. Solo BOMBILLAS cambia
        // con el brillo del insecto y el pulso del turno, via meshMaterial.
        definiciones[MODELO_RACIMO_LUCIERNAGA].mallaColor = 1;
        definiciones[MODELO_RACIMO_ARO].mallaColor = 1;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().racimoToxico,
        definiciones, CANTIDAD_MODELOS_RACIMO_TOXICO_3D };
}

inline bool DibujarModeloRacimoToxicoRetro3D(
    ModeloRacimoToxico3D pieza,
    Vector3 posicion,
    float anguloGrados = 0.0f,
    Vector3 ejeRotacion = { 0.0f, 1.0f, 0.0f },
    Vector3 escala = { 1.0f, 1.0f, 1.0f },
    Color colorEstado = WHITE
)
{
    return DibujarModeloEscenarioRetro3D(
        ObtenerModelosEscenariosRetro3D().racimoToxico[pieza],
        posicion, ejeRotacion, anguloGrados, escala, colorEstado);
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteTesoreroCercadoRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_TESORERO_CERCADO_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_TESORERO_CERCADO_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_TESORERO_CERCADO_3D[i];
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().tesoreroCercado,
        definiciones, CANTIDAD_MODELOS_TESORERO_CERCADO_3D };
}

inline void CargarPaqueteTesoreroCercadoRetro3D()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteTesoreroCercadoRetro3D();
    CargarPaqueteModelosEscenarioRetro3D(paquete);
    // Las piezas animadas necesitan estas primitives del GLB v1. Validar
    // antes de dibujar evita mezclar vistas parciales con su fallback.
    const int mallasEsperadas[] = {4,5,4,4,4,7,7,7,6,6,5,5,5,4,3,3};
    for (int i = 0; i < paquete.cantidad; i++)
    {
        RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];
        if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i]) continue;
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.cargado = false;
        TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
    }
}

inline bool DibujarModeloTesoreroCercadoRetro3D(
    ModeloTesoreroCercado3D pieza, Vector3 posicion, float anguloY = 0.0f,
    Vector3 escala = {1,1,1}, int malla = -1, bool colorDinamico = false,
    Color colorEstado = WHITE
)
{
    RecursoModeloEscenarioRetro3D& recurso =
        ObtenerModelosEscenariosRetro3D().tesoreroCercado[pieza];
    if (!recurso.cargado) return false;
    RecursoModeloEscenarioRetro3D vista = recurso;
    if (malla >= 0)
    {
        if (malla >= recurso.modelo.meshCount) return false;
        // Vista sin propiedad: comparte VBO, materiales y pivote importado.
        vista.modelo.meshCount = 1;
        vista.modelo.meshes = &recurso.modelo.meshes[malla];
        vista.modelo.meshMaterial = &recurso.modelo.meshMaterial[malla];
        // No hay COLOR_DINAMICO en el paquete. Solo las barras/travesanos
        // del aviso y la opacidad de la marca reciben el estado del juego.
        if (colorDinamico) vista.materialColor = recurso.modelo.meshMaterial[malla];
    }
    return DibujarModeloEscenarioRetro3D(vista, posicion, {0,1,0}, anguloY, escala, colorEstado);
}

inline void DescargarAlasDescensoNubesRetro3D()
{
    AlasDescensoNubesRetro3D& alas = ObtenerModelosEscenariosRetro3D().alasNubes;
    for (int i = 0; i < 2; i++)
    {
        MemFree(alas.vertices[i]);
        MemFree(alas.normales[i]);
    }
    alas = {};
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteDescensoNubesRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_DESCENSO_NUBES_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_DESCENSO_NUBES_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_DESCENSO_NUBES_3D[i];
        // En ambos planeadores solo la primitive 0 es COLOR_DINAMICO.
        definiciones[MODELO_NUBES_PLANEADOR].mallaColor = 0;
        definiciones[MODELO_NUBES_PLANEADOR_FRENADO].mallaColor = 0;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().descensoNubes,
        definiciones, CANTIDAD_MODELOS_DESCENSO_NUBES_3D };
}

inline void CargarPaqueteDescensoNubesRetro3D()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteDescensoNubesRetro3D();
    CargarPaqueteModelosEscenarioRetro3D(paquete);
    const int mallasEsperadas[] = {6,6,3,3,2,3,5,3,2,2,4,3,4,5,4,4,4,4,7};
    for (int i = 0; i < paquete.cantidad; i++)
    {
        RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];
        if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i]) continue;
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.cargado = false;
        TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
    }

    AlasDescensoNubesRetro3D& alas = ObtenerModelosEscenariosRetro3D().alasNubes;
    RecursoModeloEscenarioRetro3D& ave = paquete.recursos[MODELO_NUBES_AVE];
    if (alas.intentada || !ave.cargado) return;
    alas.intentada = true;
    // GLB v1: primitive 0 comienza con el cuerpo (|X|<=.22), seguido
    // de las alas interiores. Primitive 1 contiene las alas exteriores.
    Mesh& cuerpo = ave.modelo.meshes[0];
    for (int v = 0; v < cuerpo.vertexCount; v++)
    {
        if (std::fabs(cuerpo.vertices[v * 3]) <= 0.30f) continue;
        alas.inicioAlas = v / 3 * 3;
        break;
    }
    bool valida = alas.inicioAlas > 0;
    for (int i = 0; i < 2 && valida; i++)
    {
        Mesh& malla = ave.modelo.meshes[i];
        if (!malla.normals || !malla.vboId || !malla.vboId[0] || !malla.vboId[2])
        { valida = false; break; }
        unsigned int bytes = (unsigned int)malla.vertexCount * 3 * sizeof(float);
        alas.vertices[i] = (float*)MemAlloc(bytes);
        alas.normales[i] = (float*)MemAlloc(bytes);
        valida = alas.vertices[i] && alas.normales[i];
        if (valida)
        {
            std::memcpy(alas.vertices[i], malla.vertices, bytes);
            std::memcpy(alas.normales[i], malla.normals, bytes);
        }
    }
    if (!valida)
    {
        DescargarAlasDescensoNubesRetro3D();
        alas.intentada = true;
        UnloadModel(ave.modelo);
        ave.modelo = {};
        ave.cargado = false;
        TraceLog(LOG_WARNING, "No se pudo preparar el aleteo; se usan primitivas: %s", ave.ruta);
    }
}

inline bool DibujarModeloDescensoNubesRetro3D(
    ModeloDescensoNubes3D pieza, Vector3 posicion, float angulo = 0.0f,
    Vector3 eje = {0,1,0}, Vector3 escala = {1,1,1}, Color color = WHITE,
    int malla = -1, bool opacidadViento = false)
{
    RecursoModeloEscenarioRetro3D& recurso =
        ObtenerModelosEscenariosRetro3D().descensoNubes[pieza];
    if (!recurso.cargado) return false;
    RecursoModeloEscenarioRetro3D vista = recurso;
    if (malla >= 0)
    {
        if (malla >= recurso.modelo.meshCount) return false;
        vista.modelo.meshCount = 1;
        vista.modelo.meshes = &recurso.modelo.meshes[malla];
        vista.modelo.meshMaterial = &recurso.modelo.meshMaterial[malla];
        // La banda conserva su RGB original; solo reduce la opacidad.
        if (opacidadViento && pieza == MODELO_NUBES_BANDA)
        {
            vista.materialColor = recurso.modelo.meshMaterial[malla];
            color = recurso.modelo.materials[vista.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
            color.a = (unsigned char)(255 * 0.22f);
        }
    }
    return DibujarModeloEscenarioRetro3D(vista, posicion, eje, angulo, escala, color);
}

inline bool DibujarAveDescensoNubesRetro3D(Vector3 posicion, float aleteo)
{
    RecursoModeloEscenarioRetro3D& ave =
        ObtenerModelosEscenariosRetro3D().descensoNubes[MODELO_NUBES_AVE];
    AlasDescensoNubesRetro3D& alas = ObtenerModelosEscenariosRetro3D().alasNubes;
    if (!ave.cargado || !alas.vertices[0] || !alas.vertices[1]) return false;
    const float pendiente = aleteo / 0.95f;
    for (int i = 0; i < 2; i++)
    {
        Mesh& malla = ave.modelo.meshes[i];
        for (int v = i == 0 ? alas.inicioAlas : 0; v < malla.vertexCount; v++)
        {
            int p = v * 3;
            float x = alas.vertices[i][p];
            malla.vertices[p + 1] = alas.vertices[i][p + 1] + std::fabs(x) * pendiente;
            // Normal del cizallamiento: inversa transpuesta, sin mover cuerpo.
            Vector3 normal = {alas.normales[i][p] - (x < 0 ? -pendiente : pendiente) * alas.normales[i][p + 1],
                alas.normales[i][p + 1], alas.normales[i][p + 2]};
            normal = Vector3Normalize(normal);
            malla.normals[p] = normal.x;
            malla.normals[p + 1] = normal.y;
            malla.normals[p + 2] = normal.z;
        }
        int bytes = malla.vertexCount * 3 * (int)sizeof(float);
        UpdateMeshBuffer(malla, 0, malla.vertices, bytes, 0);
        UpdateMeshBuffer(malla, 2, malla.normals, bytes, 0);
    }
    DibujarModeloDescensoNubesRetro3D(MODELO_NUBES_AVE, posicion);
    // Una instancia nunca deja su aleteo aplicado a la siguiente.
    for (int i = 0; i < 2; i++)
    {
        Mesh& malla = ave.modelo.meshes[i];
        int bytes = malla.vertexCount * 3 * (int)sizeof(float);
        std::memcpy(malla.vertices, alas.vertices[i], bytes);
        std::memcpy(malla.normals, alas.normales[i], bytes);
        UpdateMeshBuffer(malla, 0, malla.vertices, bytes, 0);
        UpdateMeshBuffer(malla, 2, malla.normals, bytes, 0);
    }
    return true;
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteVoleaMagmaRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_VOLEA_MAGMA_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_VOLEA_MAGMA_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_VOLEA_MAGMA_3D[i];
        // El paquete no tiene COLOR_DINAMICO. Son los materiales de las
        // primitives que ya cambiaban en C++: roca/lava, cadena y burbuja.
        // Se resuelve el indice real por meshMaterial; el resto queda intacto.
        definiciones[MODELO_VOLEA_ROCA].mallaColor = 0;
        definiciones[MODELO_VOLEA_ROCA_CALIENTE].mallaColor = 0;
        definiciones[MODELO_VOLEA_RED].mallaColor = 0;
        definiciones[MODELO_VOLEA_POSTE].mallaColor = 3;
        definiciones[MODELO_VOLEA_BURBUJA].mallaColor = 0;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().voleaMagma,
        definiciones, CANTIDAD_MODELOS_VOLEA_MAGMA_3D };
}

inline void CargarPaqueteVoleaMagmaRetro3D()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteVoleaMagmaRetro3D();
    CargarPaqueteModelosEscenarioRetro3D(paquete);
    const int mallasEsperadas[] = {3,8,3,5,4,6,6,3,5,4,4,4,2,3,3,1,2};
    for (int i = 0; i < paquete.cantidad; i++)
    {
        RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];
        if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i]) continue;
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.cargado = false;
        TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
    }
}

inline Color ColorMaterialVoleaMagmaRetro3D(ModeloVoleaMagma3D pieza, int malla)
{
    const RecursoModeloEscenarioRetro3D& recurso = ObtenerModelosEscenariosRetro3D().voleaMagma[pieza];
    if (!recurso.cargado || malla < 0 || malla >= recurso.modelo.meshCount) return WHITE;
    return recurso.modelo.materials[recurso.modelo.meshMaterial[malla]].maps[MATERIAL_MAP_DIFFUSE].color;
}

inline bool DibujarModeloVoleaMagmaRetro3D(
    ModeloVoleaMagma3D pieza, Vector3 posicion, Vector3 escala = {1,1,1},
    Color colorEstado = WHITE, int malla = -1, bool materialEfecto = false)
{
    RecursoModeloEscenarioRetro3D& recurso = ObtenerModelosEscenariosRetro3D().voleaMagma[pieza];
    if (!recurso.cargado) return false;
    RecursoModeloEscenarioRetro3D vista = recurso;
    if (malla >= 0)
    {
        if (malla >= recurso.modelo.meshCount) return false;
        vista.modelo.meshCount = 1;
        vista.modelo.meshes = &recurso.modelo.meshes[malla];
        vista.modelo.meshMaterial = &recurso.modelo.meshMaterial[malla];
        // Vistas sin propiedad: permiten desvanecer solo el efecto elegido
        // sin duplicar mallas, alterar herrajes ni dejar un material modificado.
        vista.materialColor = materialEfecto ? recurso.modelo.meshMaterial[malla] : -1;
    }
    return DibujarModeloEscenarioRetro3D(vista, posicion, {0,1,0}, 0, escala, colorEstado);
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteParejasGlaciarRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_PAREJAS_GLACIAR_3D[i];
        // GLB v1: indices de primitive, resueltos mediante meshMaterial.
        // COLOR_JUGADOR solo colorea la marca; aurora conserva el pulso C++.
        definiciones[MODELO_GLACIAR_BLOQUE_EMPAREJADO].mallaColor = 3;
        definiciones[MODELO_GLACIAR_SIMBOLO_7].mallaColor = 0;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().parejasGlaciar,
        definiciones, CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D };
}

inline void CargarPaqueteParejasGlaciarRetro3D()
{
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaqueteParejasGlaciarRetro3D();
    CargarPaqueteModelosEscenarioRetro3D(paquete);
    const int mallasEsperadas[] = {2,3,3,4,4,2,1,1,1,1,1,1,1,2,3,3,3,4,3,2,2,2,2,2};
    for (int i = 0; i < paquete.cantidad; i++)
    {
        RecursoModeloEscenarioRetro3D& recurso = paquete.recursos[i];
        if (!recurso.cargado || recurso.modelo.meshCount == mallasEsperadas[i]) continue;
        UnloadModel(recurso.modelo);
        recurso.modelo = {};
        recurso.cargado = false;
        TraceLog(LOG_WARNING, "Primitives inesperadas en escenario; se usan primitivas: %s", recurso.ruta);
    }
}

inline bool DibujarModeloParejasGlaciarRetro3D(
    ModeloParejasGlaciar3D pieza, Vector3 posicion, Vector3 escala = {1,1,1},
    Color colorEstado = WHITE, int mallaOmitida = -1, float opacidad = 1.0f,
    float anguloY = 0.0f)
{
    RecursoModeloEscenarioRetro3D& recurso = ObtenerModelosEscenariosRetro3D().parejasGlaciar[pieza];
    if (!recurso.cargado) return false;
    if (mallaOmitida < 0 && opacidad >= 1.0f)
        return DibujarModeloEscenarioRetro3D(recurso, posicion, {0,1,0}, anguloY, escala, colorEstado);

    // Vista sin propiedad: ocultar la talla al revelar, o desvanecer las
    // bandas sin duplicar VBO/materiales. Cada dibujo restaura el material.
    RecursoModeloEscenarioRetro3D vista = recurso;
    vista.modelo.meshCount = 1;
    for (int i = 0; i < recurso.modelo.meshCount; i++)
    {
        if (i == mallaOmitida) continue;
        vista.modelo.meshes = &recurso.modelo.meshes[i];
        vista.modelo.meshMaterial = &recurso.modelo.meshMaterial[i];
        Color color = colorEstado;
        vista.materialColor = recurso.materialColor;
        if (opacidad < 1.0f)
        {
            vista.materialColor = recurso.modelo.meshMaterial[i];
            color = recurso.modelo.materials[vista.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
            color.a = (unsigned char)(color.a * opacidad);
        }
        DibujarModeloEscenarioRetro3D(vista, posicion, {0,1,0}, anguloY, escala, color);
    }
    return true;
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteEsferasCanonRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_ESFERAS_CANON_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_ESFERAS_CANON_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_ESFERAS_CANON_3D[i];
        // Unico COLOR_DINAMICO: primitive 0 del aro, via meshMaterial.
        definiciones[MODELO_ESFERAS_ARO].mallaColor = 0;
        definidas = true;
    }
    return { ObtenerModelosEscenariosRetro3D().esferasCanon,
        definiciones, CANTIDAD_MODELOS_ESFERAS_CANON_3D };
}

inline bool DibujarModeloEsferasCanonRetro3D(
    ModeloEsferasCanon3D pieza, Vector3 posicion, float anguloGrados = 0.0f,
    Vector3 eje = {0,1,0}, Vector3 escala = {1,1,1}, Color colorEstado = WHITE)
{
    return DibujarModeloEscenarioRetro3D(
        ObtenerModelosEscenariosRetro3D().esferasCanon[pieza],
        posicion, eje, anguloGrados, escala, colorEstado);
}

inline PaqueteModelosEscenarioRetro3D ObtenerPaquetePescaIslenaRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_PESCA_ISLENA_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_PESCA_ISLENA_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_PESCA_ISLENA_3D[i];
        definiciones[MODELO_PESCA_CURSOR].mallaColor = 0; // COLOR_DINAMICO
        definiciones[MODELO_PESCA_CORCHO].mallaColor = 1; // tapa: color del jugador
        definidas = true;
    }
    return {ObtenerModelosEscenariosRetro3D().pescaIslena,
        definiciones, CANTIDAD_MODELOS_PESCA_ISLENA_3D};
}

inline void DescargarAnimacionesPescaIslenaRetro3D()
{
    for (auto& a : ObtenerModelosEscenariosRetro3D().animacionesPesca)
    {
        MemFree(a.vertices); MemFree(a.normales); MemFree(a.colores);
        a = {};
    }
}

inline void CargarPaquetePescaIslenaRetro3D()
{
    auto paquete = ObtenerPaquetePescaIslenaRetro3D();
    CargarPaqueteModelosEscenarioRetro3D(paquete);
    const int esperadas[] = {2,5,2,4,4,3,5,3,2,3,1,3,3,4,4,4,3,2};
    for (int i = 0; i < paquete.cantidad; i++)
    {
        auto& r = paquete.recursos[i];
        if (!r.cargado || r.modelo.meshCount == esperadas[i]) continue;
        UnloadModel(r.modelo); r.modelo = {}; r.cargado = false;
        TraceLog(LOG_WARNING,"Primitives inesperadas en Pesca; se usan primitivas: %s",r.ruta);
    }
    const int piezas[] = {MODELO_PESCA_GAVIOTA,MODELO_PESCA_HUMO,
        MODELO_PESCA_PEQUENO,MODELO_PESCA_MEDIANO,MODELO_PESCA_DORADO};
    for (int i = 0; i < 5; i++)
    {
        auto& a = ObtenerModelosEscenariosRetro3D().animacionesPesca[i];
        auto& r = paquete.recursos[piezas[i]];
        if (a.intentada || !r.cargado) continue;
        a.intentada = true;
        Mesh& m = r.modelo.meshes[i < 2 ? 0 : 1];
        bool valida = m.normals && m.colors && m.vboId && m.vboId[0] && m.vboId[2] &&
            (i != 1 || (m.vertexCount == 960 && m.vboId[3]));
        unsigned int bytes = (unsigned int)m.vertexCount * 3 * sizeof(float);
        if (valida)
        {
            a.vertices = (float*)MemAlloc(bytes); a.normales = (float*)MemAlloc(bytes);
            if (i == 1) a.colores = (unsigned char*)MemAlloc(m.vertexCount * 4);
            valida = a.vertices && a.normales && (i != 1 || a.colores);
        }
        if (valida)
        {
            std::memcpy(a.vertices,m.vertices,bytes); std::memcpy(a.normales,m.normals,bytes);
            if (a.colores) std::memcpy(a.colores,m.colors,m.vertexCount * 4);
        }
        else
        {
            MemFree(a.vertices); MemFree(a.normales); MemFree(a.colores); a = {}; a.intentada = true;
            UnloadModel(r.modelo); r.modelo = {}; r.cargado = false;
            TraceLog(LOG_WARNING,"No se pudo preparar la animacion de Pesca; se usan primitivas: %s",r.ruta);
        }
    }
}

inline bool DibujarModeloPescaIslenaRetro3D(ModeloPescaIslena3D pieza,
    Vector3 posicion, float angulo = 0, Vector3 eje = {0,1,0},
    Vector3 escala = {1,1,1}, Color color = WHITE)
{
    return DibujarModeloEscenarioRetro3D(ObtenerModelosEscenariosRetro3D().pescaIslena[pieza],
        posicion,eje,angulo,escala,color);
}

inline bool DibujarModeloAnimadoPescaIslenaRetro3D(ModeloPescaIslena3D pieza,
    Vector3 posicion, float yaw, float movimiento)
{
    int indice = pieza == MODELO_PESCA_GAVIOTA ? 0 : pieza == MODELO_PESCA_HUMO ? 1 :
        2 + pieza - MODELO_PESCA_PEQUENO;
    auto& r = ObtenerModelosEscenariosRetro3D().pescaIslena[pieza];
    auto& a = ObtenerModelosEscenariosRetro3D().animacionesPesca[indice];
    if (!r.cargado || !a.vertices) return false;
    Mesh& m = r.modelo.meshes[indice < 2 ? 0 : 1];
    const float radios[] = {.28f,.42f,.5f};
    const Vector3 centros[] = {{0,0,0},{.28f,.65f,0},{-.16f,1.6f,0},{.19f,2.55f,0}};
    const float humoRadios[] = {.58f,.78f,.98f,1.2f};
    for (int v = 0; v < m.vertexCount; v++)
    {
        int p = v * 3; float x = a.vertices[p];
        Vector3 normal = {a.normales[p],a.normales[p+1],a.normales[p+2]};
        if (indice == 0)
        {
            float peso = std::fmin(1.0f,std::fmax(0.0f,(std::fabs(x)-.24f)/.66f));
            m.vertices[p+1] = a.vertices[p+1] + movimiento * peso;
            if (peso > 0 && peso < 1) normal.x -= (x < 0 ? -1 : 1) * movimiento/.66f * normal.y;
        }
        else if (indice == 1)
        {
            // GLB v1: cuatro ellipsoides consecutivos de 240 vertices.
            int h = v / 240; float fase = std::fmod(movimiento*.25f+h*.25f,1.0f);
            float escala = (.6f+fase*.9f)/humoRadios[h];
            m.vertices[p] = (x-centros[h].x)*escala + .5f*std::sin(movimiento+h);
            m.vertices[p+1] = (a.vertices[p+1]-centros[h].y)*escala/.65f + .8f+fase*4;
            m.vertices[p+2] = a.vertices[p+2]*escala;
            normal.y *= .65f;
            m.colors[v*4+3] = (unsigned char)(a.colores[v*4+3]*.7f*(1-fase));
        }
        else
        {
            float radio = radios[indice-2], largo = .76f*radio;
            float peso = std::fmin(1.0f,std::fmax(0.0f,(-x-.86f*radio)/largo));
            // Coleo original en X de mundo, convertido al espacio local del pez.
            float dx = std::cos(yaw*DEG2RAD)*movimiento, dz = std::sin(yaw*DEG2RAD)*movimiento;
            m.vertices[p] = x + dx*peso; m.vertices[p+2] = a.vertices[p+2] + dz*peso;
            if (peso > 0 && peso < 1) normal.x = (normal.x+dz/largo*normal.z)/(1-dx/largo);
        }
        normal = Vector3Normalize(normal);
        m.normals[p] = normal.x; m.normals[p+1] = normal.y; m.normals[p+2] = normal.z;
    }
    int bytes = m.vertexCount * 3 * (int)sizeof(float);
    UpdateMeshBuffer(m,0,m.vertices,bytes,0); UpdateMeshBuffer(m,2,m.normals,bytes,0);
    if (a.colores) UpdateMeshBuffer(m,3,m.colors,m.vertexCount*4,0);
    DibujarModeloPescaIslenaRetro3D(pieza,posicion,yaw);
    // La siguiente instancia y la siguiente ronda reciben siempre el reposo.
    std::memcpy(m.vertices,a.vertices,bytes); std::memcpy(m.normals,a.normales,bytes);
    UpdateMeshBuffer(m,0,m.vertices,bytes,0); UpdateMeshBuffer(m,2,m.normals,bytes,0);
    if (a.colores)
    {
        std::memcpy(m.colors,a.colores,m.vertexCount*4);
        UpdateMeshBuffer(m,3,m.colors,m.vertexCount*4,0);
    }
    return true;
}

inline void InicializarModelosEscenariosRetro3D()
{
    ModelosEscenariosRetro3D& recursos =
        ObtenerModelosEscenariosRetro3D();

    if (recursos.inicializados)
    {
        return;
    }

    PrepararSlotModeloEscenarioRetro3D(
        recursos.montanaLava,
        RUTA_MODELO_MONTANA_LAVA_3D,
        ROTACION_X_MODELO_MONTANA_LAVA_3D,
        PIVOTE_ESCENARIO_CENTRAR_BASE
    );

    recursos.inicializados = true;
}


inline bool DibujarModeloMontanaLavaEscenarioRetro3D(
    Vector3 centroBase,
    float anchoObjetivo,
    float alturaObjetivo,
    float anguloY
)
{
    // El escenario opcional historico tambien se solicita cuando se dibuja.
    InicializarModelosEscenariosRetro3D();
    RecursoModeloEscenarioRetro3D& recurso =
        ObtenerModelosEscenariosRetro3D().montanaLava;

    if (!recurso.cargado)
    {
        return false;
    }

    float escalaAncho = anchoObjetivo / recurso.dimensiones.x;
    float escalaAlto = alturaObjetivo / recurso.dimensiones.y;
    float escala =
        escalaAncho < escalaAlto
        ? escalaAncho
        : escalaAlto;

    if (!std::isfinite(escala) || escala <= 0.001f)
    {
        return false;
    }

    DrawModelEx(
        recurso.modelo,
        centroBase,
        { 0.0f, 1.0f, 0.0f },
        anguloY,
        { escala, escala, escala },
        WHITE
    );

    return true;
}


inline void DescargarSlotModeloEscenarioRetro3D(
    RecursoModeloEscenarioRetro3D& recurso
)
{
    if (recurso.cargado)
    {
        UnloadModel(recurso.modelo);
    }

    recurso = {};
}


inline PaqueteModelosEscenarioRetro3D ObtenerPaqueteRodillosNeonRetro3D()
{
    static DefinicionModeloEscenarioRetro3D definiciones[CANTIDAD_MODELOS_RODILLOS_NEON_3D];
    static bool definidas = false;
    if (!definidas)
    {
        for (int i = 0; i < CANTIDAD_MODELOS_RODILLOS_NEON_3D; i++)
            definiciones[i].ruta = RUTAS_MODELOS_RODILLOS_NEON_3D[i];
        // Primitives COLOR_DINAMICO, resueltos mediante meshMaterial.
        definiciones[MODELO_RODILLOS_MARCO].mallaColor = 0;
        definiciones[MODELO_RODILLOS_BOTON].mallaColor = 2;
        definidas = true;
    }
    return {ObtenerModelosEscenariosRetro3D().rodillosNeon,
        definiciones, CANTIDAD_MODELOS_RODILLOS_NEON_3D};
}

inline void DescargarAnimacionesRodillosNeonRetro3D()
{
    for (auto& a : ObtenerModelosEscenariosRetro3D().animacionesRodillos)
    {
        MemFree(a.colores[0]); MemFree(a.colores[1]); MemFree(a.vertices);
        a = {};
    }
}

inline void CargarPaqueteRodillosNeonRetro3D()
{
    auto paquete = ObtenerPaqueteRodillosNeonRetro3D();
    CargarPaqueteModelosEscenarioRetro3D(paquete);
    const int esperadas[] = {4,3,7,2,4,4,2,3,2,2,2,2,4,4,1,1,1,4,5,4,4};
    for (int i = 0; i < paquete.cantidad; i++)
    {
        auto& r = paquete.recursos[i];
        if (!r.cargado) continue;
        if (r.modelo.meshCount != esperadas[i])
        {
            UnloadModel(r.modelo); r.modelo = {}; r.cargado = false;
            TraceLog(LOG_WARNING, "Paquete Rodillos Neon incompatible; se usan primitivas: %s", r.ruta);
        }
    }
    const int piezas[] = {MODELO_RODILLOS_SUELO, MODELO_RODILLOS_LED_CIAN,
        MODELO_RODILLOS_LED_ROSA, MODELO_RODILLOS_LINEA};
    for (int i = 0; i < 4; i++)
    {
        auto& a = ObtenerModelosEscenariosRetro3D().animacionesRodillos[i];
        auto& r = paquete.recursos[piezas[i]];
        if (a.intentada) continue;
        a.intentada = true;
        if (!r.cargado) continue;
        bool valido = true;
        if (i == 3)
        {
            Mesh& m = r.modelo.meshes[1];
            valido = m.vboId && m.vboId[0];
            if (valido)
            {
                a.vertices = static_cast<float*>(MemAlloc(m.vertexCount * 3 * sizeof(float)));
                valido = a.vertices != nullptr;
                if (valido) std::memcpy(a.vertices, m.vertices, m.vertexCount * 3 * sizeof(float));
            }
        }
        else for (int j = 0; j < 2 && valido; j++)
        {
            Mesh& m = r.modelo.meshes[j + 1];
            valido = m.colors && m.vboId && m.vboId[3] && m.vertexCount % 36 == 0;
            if (valido)
            {
                a.colores[j] = static_cast<unsigned char*>(MemAlloc(m.vertexCount * 4));
                valido = a.colores[j] != nullptr;
                if (valido) std::memcpy(a.colores[j], m.colors, m.vertexCount * 4);
            }
        }
        if (!valido)
        {
            MemFree(a.vertices); a.vertices = nullptr;
            for (auto& c : a.colores) { MemFree(c); c = nullptr; }
            UnloadModel(r.modelo); r.modelo = {}; r.cargado = false;
            TraceLog(LOG_WARNING, "No se puede animar la pieza de Rodillos Neon; se usan primitivas: %s", r.ruta);
        }
    }
}

inline bool DibujarModeloRodillosNeonRetro3D(ModeloRodillosNeon3D pieza,
    Vector3 posicion = {}, float angulo = 0, Vector3 eje = {0,1,0},
    Vector3 escala = {1,1,1}, Color color = WHITE)
{
    return DibujarModeloEscenarioRetro3D(ObtenerModelosEscenariosRetro3D().rodillosNeon[pieza],
        posicion, eje, angulo, escala, color);
}

// Pulsos sobre el alpha original: conserva RGB/materiales y restaura el VBO.
// La linea mueve solo los bordes amarillos, sin escalar las flechas.
inline bool DibujarModeloAnimadoRodillosNeonRetro3D(ModeloRodillosNeon3D pieza,
    Vector3 posicion, float tiempo, float parametro = 0)
{
    auto& r = ObtenerModelosEscenariosRetro3D().rodillosNeon[pieza];
    if (!r.cargado) return false;
    int indice = pieza == MODELO_RODILLOS_SUELO ? 0 : pieza == MODELO_RODILLOS_LED_CIAN ? 1 :
        pieza == MODELO_RODILLOS_LED_ROSA ? 2 : 3;
    auto& a = ObtenerModelosEscenariosRetro3D().animacionesRodillos[indice];
    if (indice == 3)
    {
        Mesh& m = r.modelo.meshes[1];
        for (int v = 0; v < m.vertexCount; v++)
        {
            float y = a.vertices[v*3+1];
            if (std::fabs(y) > .2f)
                m.vertices[v*3+1] = std::copysign(parametro + std::fabs(y) - .23f, y);
        }
        UpdateMeshBuffer(m, 0, m.vertices, m.vertexCount * 3 * sizeof(float), 0);
    }
    else for (int j = 0; j < 2; j++)
    {
        Mesh& m = r.modelo.meshes[j+1];
        for (int inicio = 0; inicio < m.vertexCount; inicio += 36)
        {
            float brillo = 1.0f;
            if (indice == 0)
            {
                // Horizontales (Z fijo) mantienen su color. Verticales: X=2*i.
                float minimoZ = m.vertices[inicio*3+2], maximoZ = minimoZ;
                for (int q = inicio; q < inicio+36 && q < m.vertexCount; q++)
                {
                    float z = m.vertices[q*3+2];
                    if (z < minimoZ) minimoZ = z;
                    if (z > maximoZ) maximoZ = z;
                }
                if (maximoZ - minimoZ > 1.0f)
                {
                    float x = 0;
                    for (int q = inicio; q < inicio+36; q++) x += m.vertices[q*3];
                    float columna = std::round(x / 36.0f / 2.0f);
                    brillo = .5f + .5f * std::sin(tiempo * 2.0f + columna * .5f);
                }
            }
            else
            {
                float y = 0;
                for (int q = inicio; q < inicio+36; q++) y += m.vertices[q*3+1];
                float led = std::round((y / 36.0f - 1.2f) / 1.25f);
                brillo = .5f + .5f * std::sin(tiempo * 3.0f - led * .6f + parametro);
            }
            for (int v = inicio; v < inicio + 36; v++)
                m.colors[v*4+3] = static_cast<unsigned char>(a.colores[j][v*4+3] * (.35f + .65f*brillo));
        }
        UpdateMeshBuffer(m, 3, m.colors, m.vertexCount * 4, 0);
    }
    bool dibujado = DibujarModeloRodillosNeonRetro3D(pieza, posicion);
    if (indice == 3)
    {
        Mesh& m = r.modelo.meshes[1];
        std::memcpy(m.vertices, a.vertices, m.vertexCount * 3 * sizeof(float));
        UpdateMeshBuffer(m, 0, m.vertices, m.vertexCount * 3 * sizeof(float), 0);
    }
    else for (int j = 0; j < 2; j++)
    {
        Mesh& m = r.modelo.meshes[j+1];
        std::memcpy(m.colors, a.colores[j], m.vertexCount * 4);
        UpdateMeshBuffer(m, 3, m.colors, m.vertexCount * 4, 0);
    }
    return dibujado;
}

// Solo la cara del simbolo cambia durante el glitch; luces/herrajes intactos.
inline bool DibujarSimboloRodillosNeonRetro3D(int tipo, bool glitch, Vector3 posicion,
    float anguloX, Color colorGlitch)
{
    auto& r = ObtenerModelosEscenariosRetro3D().rodillosNeon[MODELO_RODILLOS_TRIANGULO + tipo];
    auto vista = r;
    if (glitch && r.cargado) vista.materialColor = r.modelo.meshMaterial[0];
    return DibujarModeloEscenarioRetro3D(vista, posicion, {1,0,0}, anguloX, {1,1,1}, colorGlitch);
}

inline void DescargarModelosEscenariosRetro3D()
{
    ModelosEscenariosRetro3D& recursos =
        ObtenerModelosEscenariosRetro3D();

    DescargarSlotModeloEscenarioRetro3D(recursos.montanaLava);
    DescargarAlasDescensoNubesRetro3D();
    DescargarAnimacionesPescaIslenaRetro3D();
    DescargarAnimacionesRodillosNeonRetro3D();
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.rodillosNeon)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.descensoNubes)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.voleaMagma)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.parejasGlaciar)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.esferasCanon)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.pescaIslena)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.ultimoAsiento)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.cajasPuerto)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.laberintoJade)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.vetaCristal)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.capsulasBarajadas)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.bateoMeteorico)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.racimoToxico)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.tesoreroCercado)
        DescargarSlotModeloEscenarioRetro3D(recurso);
    recursos.inicializados = false;
}
