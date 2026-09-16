#include "Editor/MapaEditor.h"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>


static void CopiarTextoSeguro(
    char destino[],
    int capacidad,
    const char* origen
)
{
    if (
        destino == nullptr ||
        capacidad <= 0
    )
    {
        return;
    }

    if (origen == nullptr)
    {
        destino[0] = '\0';
        return;
    }

    std::strncpy(
        destino,
        origen,
        capacidad - 1
    );

    destino[capacidad - 1] = '\0';
}


static const char* TextoTipoObjetoMapa(
    TipoObjetoMapaEditor tipo
)
{
    switch (tipo)
    {
        case OBJETO_MAPA_EDITOR_CINTA_67:
            return "CINTA67";

        case OBJETO_MAPA_EDITOR_MESA_67:
            return "MESA67";

        case OBJETO_MAPA_EDITOR_CUBO:
        default:
            return "CUBO";
    }
}


static bool ConvertirTipoObjetoMapa(
    const std::string& texto,
    TipoObjetoMapaEditor& tipo
)
{
    if (texto == "CUBO")
    {
        tipo = OBJETO_MAPA_EDITOR_CUBO;
        return true;
    }

    if (texto == "CINTA67")
    {
        tipo = OBJETO_MAPA_EDITOR_CINTA_67;
        return true;
    }

    if (texto == "MESA67")
    {
        tipo = OBJETO_MAPA_EDITOR_MESA_67;
        return true;
    }

    return false;
}


static bool AgregarObjetoMapa(
    MapaEditor& mapa,
    TipoObjetoMapaEditor tipo,
    const char* nombre,
    Vector3 posicion,
    Vector3 escala,
    Color color
)
{
    if (
        mapa.cantidadObjetos < 0 ||
        mapa.cantidadObjetos >= MAX_OBJETOS_MAPA_EDITOR
    )
    {
        return false;
    }

    ObjetoMapaEditor& objeto =
        mapa.objetos[mapa.cantidadObjetos];

    objeto = {};
    objeto.tipo = tipo;

    CopiarTextoSeguro(
        objeto.nombre,
        MAX_NOMBRE_OBJETO_EDITOR,
        nombre
    );

    objeto.transform.translation = posicion;

    objeto.transform.rotation =
    {
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    objeto.transform.scale = escala;
    objeto.color = color;

    mapa.cantidadObjetos++;

    return true;
}


bool MapaEditor::Cargar(
    const char* ruta
)
{
    CopiarTextoSeguro(
        rutaArchivo,
        MAX_RUTA_MAPA_EDITOR,
        ruta
    );

    std::ifstream archivo(rutaArchivo);

    if (!archivo.is_open())
    {
        CrearMapaFabrica67();

        CopiarTextoSeguro(
            rutaArchivo,
            MAX_RUTA_MAPA_EDITOR,
            ruta
        );

        return false;
    }

    std::string linea;

    if (!std::getline(archivo, linea))
    {
        return false;
    }

    if (linea != "PARTY_MAPA_V1")
    {
        return false;
    }

    cantidadObjetos = 0;

    while (std::getline(archivo, linea))
    {
        if (
            linea.empty() ||
            linea[0] == '#'
        )
        {
            continue;
        }

        std::istringstream datos(linea);

        std::string tipoTexto;
        std::string nombre;

        Vector3 posicion{};
        Quaternion rotacion{};
        Vector3 escala{};

        int rojo = 255;
        int verde = 255;
        int azul = 255;
        int alfa = 255;

        datos
            >> tipoTexto
            >> nombre
            >> posicion.x
            >> posicion.y
            >> posicion.z
            >> rotacion.x
            >> rotacion.y
            >> rotacion.z
            >> rotacion.w
            >> escala.x
            >> escala.y
            >> escala.z
            >> rojo
            >> verde
            >> azul
            >> alfa;

        if (datos.fail())
        {
            continue;
        }

        TipoObjetoMapaEditor tipo;

        if (!ConvertirTipoObjetoMapa(tipoTexto, tipo))
        {
            continue;
        }

        if (cantidadObjetos >= MAX_OBJETOS_MAPA_EDITOR)
        {
            break;
        }

        ObjetoMapaEditor& objeto =
            objetos[cantidadObjetos];

        objeto = {};
        objeto.tipo = tipo;

        CopiarTextoSeguro(
            objeto.nombre,
            MAX_NOMBRE_OBJETO_EDITOR,
            nombre.c_str()
        );

        objeto.transform.translation = posicion;
        objeto.transform.rotation = rotacion;
        objeto.transform.scale = escala;

        objeto.color =
        {
            (unsigned char)rojo,
            (unsigned char)verde,
            (unsigned char)azul,
            (unsigned char)alfa
        };

        cantidadObjetos++;
    }

    return cantidadObjetos > 0;
}


bool MapaEditor::Guardar() const
{
    if (rutaArchivo[0] == '\0')
    {
        return false;
    }

    std::ofstream archivo(rutaArchivo);

    if (!archivo.is_open())
    {
        return false;
    }

    archivo << "PARTY_MAPA_V1\n";
    archivo << "# tipo nombre px py pz qx qy qz qw sx sy sz r g b a\n";

    archivo
        << std::fixed
        << std::setprecision(4);

    for (int i = 0; i < cantidadObjetos; i++)
    {
        const ObjetoMapaEditor& objeto =
            objetos[i];

        archivo
            << TextoTipoObjetoMapa(objeto.tipo) << ' '
            << objeto.nombre << ' '
            << objeto.transform.translation.x << ' '
            << objeto.transform.translation.y << ' '
            << objeto.transform.translation.z << ' '
            << objeto.transform.rotation.x << ' '
            << objeto.transform.rotation.y << ' '
            << objeto.transform.rotation.z << ' '
            << objeto.transform.rotation.w << ' '
            << objeto.transform.scale.x << ' '
            << objeto.transform.scale.y << ' '
            << objeto.transform.scale.z << ' '
            << (int)objeto.color.r << ' '
            << (int)objeto.color.g << ' '
            << (int)objeto.color.b << ' '
            << (int)objeto.color.a
            << '\n';
    }

    return archivo.good();
}


void MapaEditor::CrearMapaFabrica67()
{
    cantidadObjetos = 0;

    // Esta plantilla contiene los objetos espaciales reales de Fabrica 67.
    // CINTA67 y MESA67 son objetos compuestos: el editor dibuja todos sus
    // detalles a partir de un unico Transform para poder moverlos completos.
    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "Piso",
        { 0.0f, -0.36f, 0.0f },
        { 18.0f, 0.10f, 10.0f },
        Color{ 78, 82, 88, 255 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "ParedFondo",
        { 0.0f, 2.35f, -4.55f },
        { 17.0f, 5.30f, 0.25f },
        Color{ 63, 68, 76, 255 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "ParedIzquierda",
        { -8.35f, 2.35f, 0.0f },
        { 0.30f, 5.30f, 10.0f },
        Color{ 55, 60, 68, 255 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CINTA_67,
        "Cinta6",
        { -1.80f, 0.20f, -2.35f },
        { 9.40f, 0.42f, 1.22f },
        Color{ 45, 49, 56, 255 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CINTA_67,
        "Cinta7",
        { -1.80f, 0.20f, 2.35f },
        { 9.40f, 0.42f, 1.22f },
        Color{ 45, 49, 56, 255 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_MESA_67,
        "Mesa",
        { 3.45f, 0.12f, 0.0f },
        { 1.76f, 0.22f, 1.76f },
        Color{ 71, 47, 32, 255 }
    );

    // Marcadores logicos. Se guardan como cubos para que puedan seleccionarse
    // con el mismo gizmo sin agregar otra dependencia o formato especial.
    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "SpawnSolo",
        { 1.35f, 0.16f, 0.0f },
        { 0.35f, 0.35f, 0.35f },
        Color{ 72, 214, 112, 180 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "Spawn6",
        { 1.35f, 0.16f, -0.72f },
        { 0.35f, 0.35f, 0.35f },
        Color{ 255, 153, 51, 180 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "Spawn7",
        { 1.35f, 0.16f, 0.72f },
        { 0.35f, 0.35f, 0.35f },
        Color{ 80, 180, 255, 180 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "CamaraPos",
        { 7.80f, 10.70f, 12.40f },
        { 0.65f, 0.65f, 0.65f },
        Color{ 220, 90, 255, 180 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "CamaraObjetivo",
        { -1.0f, 0.55f, 0.0f },
        { 0.45f, 0.45f, 0.45f },
        Color{ 255, 120, 220, 180 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "ZonaRecogida6",
        { 0.765f, 0.96f, -2.35f },
        { 0.54f, 1.00f, 1.07f },
        Color{ 255, 220, 70, 80 }
    );

    AgregarObjetoMapa(
        *this,
        OBJETO_MAPA_EDITOR_CUBO,
        "ZonaRecogida7",
        { 0.765f, 0.96f, 2.35f },
        { 0.54f, 1.00f, 1.07f },
        Color{ 255, 220, 70, 80 }
    );
}
