#pragma once

#include "raylib.h"


//==================================================
// PERSONAJES DE BLOQUES
//==================================================
//
// Los cuatro personajes jugables del juego. Son procedurales: se arman con
// cajas sombreadas por cara (geometria propia, sin GLB ni texturas), asi que
// no hay nada que cargar ni descargar y cualquier pantalla puede dibujarlos.
//
// Cada personaje tiene silueta, cara, peinado/accesorio y paleta propios.
// La remera usa el color del PUESTO del jugador: dos jugadores pueden elegir
// el mismo personaje y se siguen distinguiendo por color.
//
// Convenciones (las mismas que tenia el modelo GLB anterior):
// - "pies" es el punto de apoyo en el suelo.
// - Con anguloY = 0 el personaje mira hacia +Z.
// - escala = ESCALA_MODELO_JUGADOR_3D (0.25) es el tamano normal de juego:
//   unos 1.45 de alto, acorde a la hitbox de 0.8 x 1.4 x 0.8.
//==================================================

enum IdPersonajeBloque
{
    PERSONAJE_BRICO = 0,
    PERSONAJE_PIXI,
    PERSONAJE_ROBOTO,
    PERSONAJE_MOMO,

    CANTIDAD_PERSONAJES_BLOQUE
};


// Gestos de brazos para presentaciones y reacciones (cinematicas, tienda,
// casillas de caos). Se mezclan con la pose base segun pesoGesto.
enum GestoPersonajeBloque
{
    GESTO_NINGUNO = 0,
    GESTO_SALUDAR,          // brazo derecho arriba, la mano se agita
    GESTO_SENALAR,          // brazo derecho estirado hacia el frente
    GESTO_PRESENTAR,        // ambos brazos abiertos ("¡miren esto!")
    GESTO_FESTEJAR,         // ambos brazos arriba
    GESTO_MANOS_CABEZA,     // manos a la cabeza (preocupacion)
    GESTO_SORPRESA          // brazos a medio subir, palmas afuera
};


// Estado de animacion. Todas las animaciones son transformaciones de las
// partes del cuerpo calculadas en el momento del dibujo (sin esqueleto).
struct PosePersonajeBloque
{
    // Segundos de reloj: respiracion, parpadeo, coleta, cola, antena.
    float tiempo = 0.0f;

    // Marcha: intensidad 0..1 y fase en radianes del ciclo de pasos.
    float caminar = 0.0f;
    float faseCaminar = 0.0f;

    // Golpe: progreso 0..1 (anticipacion, impacto, recuperacion).
    // Un valor negativo significa que no esta golpeando.
    float golpe = -1.0f;

    // Caida por golpe fuerte: 0 de pie, 1 acostado boca arriba. Al
    // levantarse el valor vuelve de 1 a 0.
    float caida = 0.0f;

    // Golpe leve: 0..1, el cuerpo se echa hacia atras y vuelve.
    float tambaleo = 0.0f;

    // Estrellitas girando sobre la cabeza.
    bool aturdido = false;

    // Saltando o cayendo: brazos arriba y piernas recogidas.
    bool enAire = false;

    // Escala vertical extra (aplastado por un ground pound).
    float escalaVertical = 1.0f;

    // Transparencia general (parpadeo de inmunidad).
    float alpha = 1.0f;

    // Gesto de brazos y su mezcla 0..1 con la pose base.
    GestoPersonajeBloque gesto = GESTO_NINGUNO;
    float pesoGesto = 0.0f;

    // Giro de la cabeza hacia los costados (radianes, + = a su izquierda).
    float giroCabeza = 0.0f;

    // Susto 0..1: ojos muy abiertos sin parpadeo, cejas arriba y boca en O.
    float susto = 0.0f;

    // Hablando: la boca se abre y cierra (cinematicas).
    bool hablando = false;
};


// Vendedor de la tienda: personaje de bloques propio (no es jugable), con
// delantal, gorro a rayas y bigote. Mismas convenciones que los jugadores.
void DibujarVendedorBloque(
    Vector3 pies,
    float anguloY,
    float escala,
    const PosePersonajeBloque& pose
);

const char* ObtenerNombreVendedorBloque();


const char* ObtenerNombrePersonajeBloque(
    int personaje
);

// Frase corta para la seleccion de personajes.
const char* ObtenerLemaPersonajeBloque(
    int personaje
);

// Color distintivo del personaje (pedestales, marcos de la seleccion).
Color ObtenerColorPersonajeBloque(
    int personaje
);

// Devuelve un indice valido 0..CANTIDAD_PERSONAJES_BLOQUE-1.
int NormalizarPersonajeBloque(
    int personaje
);

// Dibuja el personaje completo con su pose. Debe llamarse dentro de
// BeginMode3D. Respeta la matriz rlgl actual (rlPushMatrix del llamador).
void DibujarPersonajeBloque(
    int personaje,
    Vector3 pies,
    float anguloY,
    float escala,
    Color colorJugador,
    const PosePersonajeBloque& pose
);
