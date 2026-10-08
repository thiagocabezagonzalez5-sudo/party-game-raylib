#pragma once

#include "raylib.h"

#include <vector>


//==================================================
// MALLAS DE TABLERO
//==================================================
//
// Geometria decorativa que no cambia (o cambia rara vez) se construye una
// vez en CPU con ConstructorMalla, se sube a la GPU como MallaGpu y se
// dibuja con una sola llamada. Las primitivas replican la geometria y el
// sentido de caras de DrawSphereEx / DrawCylinderEx de raylib, de modo que
// el resultado es el mismo que el modo inmediato pero sin repetir el
// trabajo de CPU en cada frame.
//
// Toda MallaGpu debe liberarse con DescargarMalla (los tableros lo hacen
// desde DefinicionTablero::DescargarRecursos, que se invoca al cerrar el
// juego en DescargarVistaPreviaTableros).
//==================================================

// Transformacion simple aplicada a lo que se emite: escala, giro en Y
// (grados) y traslacion, en ese orden.
struct TransformacionMalla
{
    float tx = 0.0f;
    float ty = 0.0f;
    float tz = 0.0f;
    float giroY = 0.0f;
    float sx = 1.0f;
    float sy = 1.0f;
    float sz = 1.0f;
};


struct ConstructorMalla
{
    std::vector<float> posiciones;
    std::vector<unsigned char> colores;

    TransformacionMalla transformacion;
    bool usarTransformacion = false;

    // Resta anillos/cortes/lados a esferas y cilindros (LOD en calidad media/baja).
    int reduccionDetalle = 0;

    void Limpiar();

    int CantidadVertices() const;

    void EstablecerTransformacion(const TransformacionMalla& t);
    void QuitarTransformacion();

    void Vertice(float x, float y, float z, Color color);

    void Triangulo(Vector3 a, Vector3 b, Vector3 c, Color color);

    // Dos caras (equivale a dibujar con el culling desactivado).
    void TrianguloDobleCara(Vector3 a, Vector3 b, Vector3 c, Color color);

    // Como DrawSphereEx.
    void Esfera(Vector3 centro, float radio, int anillos, int cortes, Color color);

    // Como DrawCylinderEx (eje vertical u oblicuo).
    void Cilindro(
        Vector3 inicio, Vector3 fin,
        float radioInicio, float radioFin,
        int lados, Color color
    );

    // Caja alineada a los ejes, con las dos caras (piezas finas).
    void Caja(Vector3 centro, Vector3 tamano, Color color);
};


struct MallaGpu
{
    Mesh malla{};
    bool cargada = false;
};


// Sube la malla (o actualiza sus buffers si el numero de vertices coincide).
void ActualizarMallaGpu(MallaGpu& destino, const ConstructorMalla& origen);

// Solo reescribe los colores (mismo numero de vertices).
void ActualizarColoresMallaGpu(MallaGpu& destino, const ConstructorMalla& origen);

void DescargarMalla(MallaGpu& malla);

// Dibuja la malla trasladada. Vacia antes el lote inmediato para conservar
// el orden de dibujo respecto al resto de la escena.
void DibujarMallaGpu(
    const MallaGpu& malla,
    float desplazamientoX = 0.0f,
    float desplazamientoY = 0.0f,
    float desplazamientoZ = 0.0f
);

// Libera el material compartido de las mallas (se crea al primer uso).
void DescargarMaterialMallasTablero();
