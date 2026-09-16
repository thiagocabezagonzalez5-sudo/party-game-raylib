#include "Editor/MapaEditor.h"

#include <cstdio>
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


static bool AgregarCuboMapa(
    MapaEditor& mapa,
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

    objeto.tipo = OBJETO_MAPA_EDITOR_CUBO;

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

        if (
            datos.fail() ||
            tipoTexto != "CUBO"
        )
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

        objeto.tipo = OBJETO_MAPA_EDITOR_CUBO;

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
            << "CUBO "
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

    AgregarCuboMapa(
        *this,
        "Piso",
        { 0.0f, -0.36f, 0.0f },
        { 17.0f, 0.10f, 11.0f },
        Color{ 104, 106, 109, 255 }
    );

    AgregarCuboMapa(
        *this,
        "ParedFondo",
        { 0.0f, 2.30f, -5.15f },
        { 16.0f, 5.20f, 0.25f },
        Color{ 80, 87, 96, 255 }
    );

    AgregarCuboMapa(
        *this,
        "ParedIzquierda",
        { -6.90f, 2.30f, 0.0f },
        { 0.35f, 5.20f, 10.20f },
        Color{ 67, 73, 82, 255 }
    );

    const float CENTRO_CINTA_X = -1.80f;

    const float CARRILES[4] =
    {
        -3.0f,
        -1.0f,
        1.0f,
        3.0f
    };

    for (int i = 0; i < 4; i++)
    {
        char nombre[32] = {};

        std::snprintf(
            nombre,
            sizeof(nombre),
            "Cinta%d",
            i + 1
        );

        AgregarCuboMapa(
            *this,
            nombre,
            { CENTRO_CINTA_X, 0.23f, CARRILES[i] },
            { 7.55f, 0.46f, 1.22f },
            Color{ 55, 61, 70, 255 }
        );
    }

    AgregarCuboMapa(
        *this,
        "Mesa1",
        { 4.45f, 0.82f, -2.0f },
        { 1.85f, 0.22f, 2.55f },
        Color{ 126, 77, 43, 255 }
    );

    AgregarCuboMapa(
        *this,
        "Mesa2",
        { 4.45f, 0.82f, 2.0f },
        { 1.85f, 0.22f, 2.55f },
        Color{ 126, 77, 43, 255 }
    );
}
