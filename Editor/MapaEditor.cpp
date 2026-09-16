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

    // El archivo representa una plantilla de fabrica. Como el minijuego
    // se dibuja en pantalla dividida, ambos equipos reutilizan esta misma
    // distribucion y solo cambia el color de cada equipo.
    AgregarCuboMapa(
        *this,
        "Piso",
        { 0.0f, -0.36f, 0.0f },
        { 18.0f, 0.10f, 10.0f },
        Color{ 78, 82, 88, 255 }
    );

    AgregarCuboMapa(
        *this,
        "ParedFondo",
        { 0.0f, 2.35f, -4.55f },
        { 17.0f, 5.30f, 0.25f },
        Color{ 63, 68, 76, 255 }
    );

    AgregarCuboMapa(
        *this,
        "ParedIzquierda",
        { -8.35f, 2.35f, 0.0f },
        { 0.30f, 5.30f, 10.0f },
        Color{ 55, 60, 68, 255 }
    );

    AgregarCuboMapa(
        *this,
        "Cinta6",
        { -1.80f, 0.20f, -2.35f },
        { 9.40f, 0.42f, 1.22f },
        Color{ 45, 49, 56, 255 }
    );

    AgregarCuboMapa(
        *this,
        "Cinta7",
        { -1.80f, 0.20f, 2.35f },
        { 9.40f, 0.42f, 1.22f },
        Color{ 45, 49, 56, 255 }
    );

    AgregarCuboMapa(
        *this,
        "Mesa",
        { 3.45f, 0.12f, 0.0f },
        { 1.76f, 0.22f, 1.76f },
        Color{ 71, 47, 32, 255 }
    );
}
