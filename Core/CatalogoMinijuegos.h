#pragma once

#include "raylib.h"


enum IdMinijuego
{
    MINIJUEGO_COLOR_SEGURO = 0,
    MINIJUEGO_PELOTAS,
    MINIJUEGO_TRONCO,
    MINIJUEGO_FABRICA_67,
    MINIJUEGO_ISLA_FUEGO,
    MINIJUEGO_CAPITAN_MANDA,
    MINIJUEGO_BARRA_GIRATORIA,
    MINIJUEGO_NUCLEOS_ENERGIA,
    MINIJUEGO_REFUGIO_PINCHOS,
    MINIJUEGO_MIRADAS_CRUZADAS,
    MINIJUEGO_MUROS_LOCOS,
    MINIJUEGO_TORMENTA_MAGNETICA,
    MINIJUEGO_CONTEO_EXPLOSIVO,
    MINIJUEGO_PASO_SILENCIOSO,
    MINIJUEGO_CIRCUITO_VOLTAJE,
    MINIJUEGO_TRAZO_PERFECTO,
    MINIJUEGO_CARGA_INESTABLE,
    MINIJUEGO_SECUENCIA_NEON,
    MINIJUEGO_INTERRUPTORES_CAOS,
    MINIJUEGO_TANQUES_PLASMA,
    MINIJUEGO_PASARELAS_VACIO,
    MINIJUEGO_CANTERA_FUGA,
    CANTIDAD_MINIJUEGOS
};


struct DatosMinijuegoCatalogo
{
    IdMinijuego id = MINIJUEGO_COLOR_SEGURO;
    const char* nombre = "";
    const char* descripcion = "";
    Color color{};
    const char* etiquetaZonaPruebas = "";
    bool disponibleEnTablero = false;
};


bool EsIdMinijuegoValido(
    IdMinijuego id
);

const DatosMinijuegoCatalogo& ObtenerDatosMinijuego(
    IdMinijuego id
);

const DatosMinijuegoCatalogo& ObtenerDatosMinijuegoPorIndice(
    int indice
);

IdMinijuego ObtenerIdMinijuegoPorIndice(
    int indice
);

int ObtenerCantidadMinijuegosDisponiblesTablero();

IdMinijuego ObtenerMinijuegoDisponibleTablero(
    int indiceDisponible
);
