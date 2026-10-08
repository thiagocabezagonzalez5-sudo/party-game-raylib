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
    MINIJUEGO_TERRITORIO_CONQUISTA,
    MINIJUEGO_DEFENSA_NUCLEO,
    MINIJUEGO_LLUVIA_APILADA,
    MINIJUEGO_CUERDA_ACANTILADO,
    MINIJUEGO_ULTIMO_ASIENTO,
    MINIJUEGO_CAJAS_PUERTO,
    MINIJUEGO_LABERINTO_INCLINADO,
    MINIJUEGO_VETA_CRISTAL,
    MINIJUEGO_CAPSULAS_BARAJADAS,
    MINIJUEGO_BATEO_METEORICO,
    MINIJUEGO_RACIMO_TOXICO,
    MINIJUEGO_TESORERO_ACORRALADO,
    MINIJUEGO_DESCENSO_NUBES,
    MINIJUEGO_VOLEA_MAGMA,
    MINIJUEGO_PAREJAS_GLACIAR,
    MINIJUEGO_ESFERAS_CANON,
    MINIJUEGO_PESCA_ISLA,
    MINIJUEGO_RODILLOS_NEON,
    MINIJUEGO_BOLAS_AZUCAR,
    MINIJUEGO_GRUA_CHATARRA,
    MINIJUEGO_PISOTON_PLAGAS,
    MINIJUEGO_BALSAS_RAPIDO,
    MINIJUEGO_AUTOS_GLOBO,
    MINIJUEGO_SENDERO_INVISIBLE,
    MINIJUEGO_BANQUETE_TURBO,
    MINIJUEGO_TUBERIAS_DESIERTO,
    MINIJUEGO_TREPA_MASTIL,
    MINIJUEGO_GUARDIAN_RUINAS,

    // Debe ser siempre el ultimo valor: el catalogo, el selector y la
    // ruleta recorren los minijuegos desde 0 hasta CANTIDAD_MINIJUEGOS.
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

    // Formatos 2 vs 2 estrictos: solo se juegan con 2 o 4 participantes.
    bool requiereParDeJugadores = false;
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

// Indica si el minijuego puede jugarse con esa cantidad de participantes
// (el tablero no debe sortear uno que quedaria esperando jugadores).
bool MinijuegoAdmiteCantidadJugadores(
    IdMinijuego id,
    int cantidadJugadores
);
