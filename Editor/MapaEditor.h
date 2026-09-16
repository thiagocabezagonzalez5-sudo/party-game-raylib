#pragma once

#include "raylib.h"

static const int MAX_OBJETOS_MAPA_EDITOR = 128;
static const int MAX_NOMBRE_OBJETO_EDITOR = 64;
static const int MAX_RUTA_MAPA_EDITOR = 256;


enum TipoObjetoMapaEditor
{
    OBJETO_MAPA_EDITOR_CUBO = 0,
    OBJETO_MAPA_EDITOR_CINTA_67,
    OBJETO_MAPA_EDITOR_MESA_67
};


struct ObjetoMapaEditor
{
    TipoObjetoMapaEditor tipo = OBJETO_MAPA_EDITOR_CUBO;

    char nombre[MAX_NOMBRE_OBJETO_EDITOR] = {};

    Transform transform{};

    Color color = WHITE;
};


struct MapaEditor
{
    ObjetoMapaEditor objetos[MAX_OBJETOS_MAPA_EDITOR];

    int cantidadObjetos = 0;

    char rutaArchivo[MAX_RUTA_MAPA_EDITOR] = {};

    bool Cargar(const char* ruta);
    bool Guardar() const;

    void CrearMapaFabrica67();
};
