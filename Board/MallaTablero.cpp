#include "Board/MallaTablero.h"

#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstring>


static Material materialMallas{};
static bool materialMallasCargado = false;


//==================================================
// CONSTRUCTOR
//==================================================

void ConstructorMalla::Limpiar()
{
    posiciones.clear();
    colores.clear();
    QuitarTransformacion();
}


int ConstructorMalla::CantidadVertices() const
{
    return (int)(posiciones.size() / 3);
}


void ConstructorMalla::EstablecerTransformacion(const TransformacionMalla& t)
{
    transformacion = t;
    usarTransformacion = true;
}


void ConstructorMalla::QuitarTransformacion()
{
    transformacion = TransformacionMalla{};
    usarTransformacion = false;
}


void ConstructorMalla::Vertice(float x, float y, float z, Color color)
{
    if (usarTransformacion)
    {
        const TransformacionMalla& t = transformacion;

        x *= t.sx;
        y *= t.sy;
        z *= t.sz;

        if (t.giroY != 0.0f)
        {
            float a = t.giroY * DEG2RAD;
            float c = std::cos(a);
            float s = std::sin(a);
            float nx = x * c + z * s;
            float nz = -x * s + z * c;

            x = nx;
            z = nz;
        }

        x += t.tx;
        y += t.ty;
        z += t.tz;
    }

    posiciones.push_back(x);
    posiciones.push_back(y);
    posiciones.push_back(z);

    colores.push_back(color.r);
    colores.push_back(color.g);
    colores.push_back(color.b);
    colores.push_back(color.a);
}


void ConstructorMalla::Triangulo(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    Vertice(a.x, a.y, a.z, color);
    Vertice(b.x, b.y, b.z, color);
    Vertice(c.x, c.y, c.z, color);
}


void ConstructorMalla::TrianguloDobleCara(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    Triangulo(a, b, c, color);
    Triangulo(a, c, b, color);
}


void ConstructorMalla::Esfera(
    Vector3 centro, float radio, int anillos, int cortes, Color color
)
{
    if (reduccionDetalle > 0)
    {
        anillos = std::max(2, anillos - reduccionDetalle);
        cortes = std::max(3, cortes - reduccionDetalle);
    }

    // Misma malla y mismo orden que DrawSphereEx de raylib 6: parte del polo
    // superior y baja anillo a anillo girando cada cara alrededor de Y.
    float anguloAnillo = DEG2RAD * (180.0f / (anillos + 1));
    float anguloCorte = DEG2RAD * (360.0f / cortes);
    float cosAnillo = std::cos(anguloAnillo);
    float sinAnillo = std::sin(anguloAnillo);
    float cosCorte = std::cos(anguloCorte);
    float sinCorte = std::sin(anguloCorte);

    Vector3 v[4] = {};
    v[2] = Vector3{ 0.0f, 1.0f, 0.0f };
    v[3] = Vector3{ sinAnillo, cosAnillo, 0.0f };

    auto emitir = [&](const Vector3& p)
    {
        Vertice(
            centro.x + radio * p.x,
            centro.y + radio * p.y,
            centro.z + radio * p.z,
            color
        );
    };

    for (int i = 0; i < anillos + 1; i++)
    {
        for (int j = 0; j < cortes; j++)
        {
            v[0] = v[2];
            v[1] = v[3];

            v[2] = Vector3{ cosCorte * v[2].x - sinCorte * v[2].z, v[2].y, sinCorte * v[2].x + cosCorte * v[2].z };
            v[3] = Vector3{ cosCorte * v[3].x - sinCorte * v[3].z, v[3].y, sinCorte * v[3].x + cosCorte * v[3].z };

            emitir(v[0]);
            emitir(v[3]);
            emitir(v[1]);

            if (i != 0)
            {
                emitir(v[0]);
                emitir(v[2]);
                emitir(v[3]);
            }
        }

        v[2] = v[3];
        v[3] = Vector3{ cosAnillo * v[3].x + sinAnillo * v[3].y, -sinAnillo * v[3].x + cosAnillo * v[3].y, v[3].z };
    }
}


void ConstructorMalla::Cilindro(
    Vector3 inicio, Vector3 fin,
    float radioInicio, float radioFin,
    int lados, Color color
)
{
    lados -= reduccionDetalle;

    if (lados < 3)
    {
        lados = 3;
    }

    Vector3 direccion = Vector3Subtract(fin, inicio);

    if (Vector3Length(direccion) == 0.0f)
    {
        return;
    }

    Vector3 b1 = Vector3Normalize(Vector3Perpendicular(direccion));
    Vector3 b2 = Vector3Normalize(Vector3CrossProduct(b1, direccion));

    float anguloBase = (2.0f * PI) / lados;

    auto anillo = [&](Vector3 centro, float radio, int i)
    {
        float s = std::sin(anguloBase * i) * radio;
        float c = std::cos(anguloBase * i) * radio;

        return Vector3
        {
            centro.x + s * b1.x + c * b2.x,
            centro.y + s * b1.y + c * b2.y,
            centro.z + s * b1.z + c * b2.z
        };
    };

    for (int i = 0; i < lados; i++)
    {
        Vector3 w1 = anillo(inicio, radioInicio, i);
        Vector3 w2 = anillo(inicio, radioInicio, i + 1);
        Vector3 w3 = anillo(fin, radioFin, i);
        Vector3 w4 = anillo(fin, radioFin, i + 1);

        if (radioInicio > 0.0f)
        {
            Triangulo(inicio, w2, w1, color);
        }

        Triangulo(w1, w2, w3, color);
        Triangulo(w4, w3, w2, color);

        if (radioFin > 0.0f)
        {
            Triangulo(fin, w3, w4, color);
        }
    }
}


void ConstructorMalla::Caja(Vector3 centro, Vector3 tamano, Color color)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Vector3 v[8] =
    {
        { x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 },
        { x0, y1, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, { x0, y1, z1 }
    };

    const int caras[6][4] =
    {
        { 4, 7, 6, 5 }, { 0, 1, 2, 3 },
        { 0, 4, 5, 1 }, { 3, 2, 6, 7 },
        { 0, 3, 7, 4 }, { 1, 5, 6, 2 }
    };

    for (const auto& c : caras)
    {
        TrianguloDobleCara(v[c[0]], v[c[1]], v[c[2]], color);
        TrianguloDobleCara(v[c[0]], v[c[2]], v[c[3]], color);
    }
}


//==================================================
// GPU
//==================================================

void ActualizarMallaGpu(MallaGpu& destino, const ConstructorMalla& origen)
{
    int n = origen.CantidadVertices();

    if (destino.cargada && destino.malla.vertexCount == n && n > 0)
    {
        std::memcpy(destino.malla.vertices, origen.posiciones.data(), n * 3 * sizeof(float));
        std::memcpy(destino.malla.colors, origen.colores.data(), n * 4);

        UpdateMeshBuffer(destino.malla, 0, destino.malla.vertices, n * 3 * (int)sizeof(float), 0);
        UpdateMeshBuffer(destino.malla, 3, destino.malla.colors, n * 4, 0);
        return;
    }

    DescargarMalla(destino);

    if (n <= 0)
    {
        return;
    }

    Mesh& malla = destino.malla;

    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    std::memcpy(malla.vertices, origen.posiciones.data(), n * 3 * sizeof(float));
    std::memcpy(malla.colors, origen.colores.data(), n * 4);

    UploadMesh(&malla, true);
    destino.cargada = true;
}


void ActualizarColoresMallaGpu(MallaGpu& destino, const ConstructorMalla& origen)
{
    int n = origen.CantidadVertices();

    if (!destino.cargada || destino.malla.vertexCount != n)
    {
        return;
    }

    std::memcpy(destino.malla.colors, origen.colores.data(), n * 4);
    UpdateMeshBuffer(destino.malla, 3, destino.malla.colors, n * 4, 0);
}


void DescargarMalla(MallaGpu& malla)
{
    if (malla.cargada)
    {
        UnloadMesh(malla.malla);
    }

    malla = MallaGpu{};
}


void DibujarMallaGpu(
    const MallaGpu& malla,
    float desplazamientoX, float desplazamientoY, float desplazamientoZ
)
{
    if (!malla.cargada)
    {
        return;
    }

    if (!materialMallasCargado)
    {
        materialMallas = LoadMaterialDefault();
        materialMallasCargado = true;
    }

    // Conserva el orden respecto a lo que ya esta en el lote inmediato.
    rlDrawRenderBatchActive();

    DrawMesh(
        malla.malla,
        materialMallas,
        MatrixTranslate(desplazamientoX, desplazamientoY, desplazamientoZ)
    );
}


void DescargarMaterialMallasTablero()
{
    if (materialMallasCargado)
    {
        UnloadMaterial(materialMallas);
        materialMallas = Material{};
        materialMallasCargado = false;
    }
}
