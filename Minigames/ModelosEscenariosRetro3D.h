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


inline void DescargarModelosEscenariosRetro3D()
{
    ModelosEscenariosRetro3D& recursos =
        ObtenerModelosEscenariosRetro3D();

    DescargarSlotModeloEscenarioRetro3D(recursos.montanaLava);
    DescargarAlasDescensoNubesRetro3D();
    for (RecursoModeloEscenarioRetro3D& recurso : recursos.descensoNubes)
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
