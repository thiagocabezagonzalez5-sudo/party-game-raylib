#pragma once

#include "raylib.h"
#include "rlgl.h"

#include <vector>


//==================================================
// HORNEADO DE MALLAS
//==================================================
//
// Utilidades comunes de los minijuegos que graban su escenario estatico una
// sola vez en mallas de GPU. Mientras un GrabadorMalla esta activo, cada
// minijuego desvia sus vertices hacia el (en lugar de mandarlos a rlgl);
// al terminar se arma una Mesh sin indices (cabe cualquier tamano) y se sube.
//
// Cada recurso tiene su liberacion simetrica, que deja la bandera en estado
// "no valido": llamar dos veces es seguro y el horneado se rehace solo la
// proxima vez que el minijuego se dibuje.
//==================================================

struct GrabadorMalla
{
    bool activa = false;
    std::vector<float> posiciones;      // 3 por vertice
    std::vector<unsigned char> colores; // 4 por vertice
    std::vector<float> texcoords;       // opcional: 2 por vertice (datos para el shader)
};


inline void EmpezarGrabacionMalla(GrabadorMalla& g)
{
    g.activa = true;
    g.posiciones.clear();
    g.colores.clear();
    g.texcoords.clear();
}


inline void GrabarVerticeMalla(GrabadorMalla& g, Vector3 p, Color c)
{
    g.posiciones.push_back(p.x);
    g.posiciones.push_back(p.y);
    g.posiciones.push_back(p.z);
    g.colores.push_back(c.r);
    g.colores.push_back(c.g);
    g.colores.push_back(c.b);
    g.colores.push_back(c.a);
}


// Variante con datos por vertice que la malla guarda en texcoords.
inline void GrabarVerticeMalla(GrabadorMalla& g, Vector3 p, Color c, float u, float v)
{
    GrabarVerticeMalla(g, p, c);
    g.texcoords.push_back(u);
    g.texcoords.push_back(v);
}


// Cierra la grabacion y sube la malla. Devuelve false (sin cargar nada) si
// no llego a juntarse un triangulo. Sin datos por vertice, texcoords = 0.
inline bool TerminarGrabacionMalla(GrabadorMalla& g, Mesh& malla)
{
    g.activa = false;
    int n = (int)(g.posiciones.size() / 3);
    n -= n % 3;

    if (n < 3)
    {
        g.posiciones.clear();
        g.colores.clear();
        g.texcoords.clear();
        return false;
    }

    bool conDatos = g.texcoords.size() >= (size_t)n * 2;

    malla = {};
    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * (int)sizeof(float)));
    malla.texcoords = (float*)MemAlloc((unsigned int)(n * 2 * (int)sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    for (int i = 0; i < n * 3; i++) malla.vertices[i] = g.posiciones[(size_t)i];
    for (int i = 0; i < n * 4; i++) malla.colors[i] = g.colores[(size_t)i];
    for (int i = 0; i < n * 2; i++) malla.texcoords[i] = conDatos ? g.texcoords[(size_t)i] : 0.0f;

    UploadMesh(&malla, false);
    g.posiciones.clear();
    g.colores.clear();
    g.texcoords.clear();
    return true;
}


inline void LiberarMallaHorneada(Mesh& malla, bool& valida)
{
    if (valida) UnloadMesh(malla);
    valida = false;
    malla = {};
}


inline void AsegurarMaterialHorneado(Material& material, bool& listo)
{
    if (listo) return;
    material = LoadMaterialDefault();
    listo = true;
}


// Libera el material sin tocar su shader: el por defecto es de raylib y uno
// propio se descarga aparte con LiberarShaderHorneado.
inline void LiberarMaterialHorneado(Material& material, bool& listo)
{
    if (listo && material.maps != nullptr)
    {
        material.shader.id = rlGetShaderIdDefault();
        material.shader.locs = rlGetShaderLocsDefault();
        UnloadMaterial(material);
    }

    material = {};
    listo = false;
}


inline void LiberarShaderHorneado(Shader& shader)
{
    if (shader.id != 0 && shader.id != rlGetShaderIdDefault())
    {
        UnloadShader(shader);
    }

    shader = {};
}
