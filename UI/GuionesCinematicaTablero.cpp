#include "UI/Cinematicas.h"


//==================================================
// GUIONES DE LA PRESENTACION DE CADA TABLERO
//==================================================
//
// Solo DATOS: quien presenta el tablero y que dice en cada plano. La
// camara, los saltos del guia, el resaltado de casillas y el globo de
// dialogo son comunes (CinematicaTablero en UI/Cinematicas.cpp).
//
// Cada frase tiene que coincidir con la regla REAL de la casilla
// (Docs/ReglasTableros.md y el ResolverCasilla de cada Board/TableroXxx.cpp).
// Frases cortas: la duracion de cada plano sale del largo del texto.
//
//   Tablero            Guia     Personalidad
//   Isla Arboleda      PIXI     exploradora entusiasta
//   Forja del Volcan   BRICO    minero gruñon y trabajador
//   Mercado Flotante   MOMO     comerciante picaro
//   Torre del Reloj    ROBOTO   robot preciso
//   Feria Nocturna     PIXI     primera vez en la feria, deslumbrada
//==================================================

static const GuionCinematicaTablero GUION_ARBOLEDA =
{
    PERSONAJE_PIXI,
    {
        { PLANO_PANORAMA, CASILLA_NEUTRA, GESTO_NINGUNO,
          "¡Hola, hola! Soy Pixi. ¡Bienvenidos a Isla Arboleda!" },
        { PLANO_GUIA, CASILLA_NEUTRA, GESTO_SALUDAR,
          "El Gran Roble regala trofeos dorados a quien cuide su isla." },
        { PLANO_TROFEO, CASILLA_NEUTRA, GESTO_SENALAR,
          "¡Ahí está! Cuesta 20 monedas y después brota en otro claro." },
        { PLANO_CASILLA, CASILLA_POSITIVA, GESTO_PRESENTAR,
          "En las rondas pares el bosque florece: ¡las verdes pagan +5!" },
        { PLANO_CASILLA, CASILLA_EVENTO, GESTO_SENALAR,
          "¡Exclamación! El Roble tira un regalo bueno y uno malo... ¡o abejas!" },
        { PLANO_CASILLA, CASILLA_REGALO, GESTO_SENALAR,
          "Una caja con moño: monedas o un objeto, de regalo." },
        { PLANO_CASILLA, CASILLA_TIENDA, GESTO_PRESENTAR,
          "Acá atiende Don Cambalache: objetos a cambio de monedas." },
        { PLANO_CASILLA, CASILLA_DUELO, GESTO_SENALAR,
          "Con las espadas retás a alguien a un duelo... ¡y apuestan!" },
        { PLANO_CASILLA, CASILLA_INTERCAMBIO, GESTO_SENALAR,
          "Las flechas: tres dados deciden quién da, recibe o intercambia." },
        { PLANO_CASILLA, CASILLA_CAOS, GESTO_MANOS_CABEZA,
          "¿Y el rayo? La ruleta de castigos. ¡Mejor esquivala!" },
        { PLANO_DESPEDIDA, CASILLA_NEUTRA, GESTO_FESTEJAR,
          "¡Mochilas listas! ¡A explorar!" },
    }
};


static const GuionCinematicaTablero GUION_FORJA =
{
    PERSONAJE_BRICO,
    {
        { PLANO_PANORAMA, CASILLA_NEUTRA, GESTO_NINGUNO,
          "¡Casco puesto! Soy Brico. Esto es la Forja del Volcán." },
        { PLANO_GUIA, CASILLA_NEUTRA, GESTO_MANOS_CABEZA,
          "Cada ronda caen dos llamas: -5 monedas. ¡En la última, -10!" },
        { PLANO_TROFEO, CASILLA_NEUTRA, GESTO_SENALAR,
          "El trofeo de oro fundido cuesta 20 monedas. ¡A picar piedra!" },
        { PLANO_LANDMARK, CASILLA_NEUTRA, GESTO_PRESENTAR,
          "En las rondas pares el puente se levanta y cierra el atajo." },
        { PLANO_CASILLA, CASILLA_EVENTO, GESTO_SENALAR,
          "Exclamación: veta de oro, duende ladrón... ¡o un géiser que te lanza!" },
        { PLANO_CASILLA, CASILLA_CAOS, GESTO_MANOS_CABEZA,
          "El rayo es la ruleta del caos. ¡Ahí se pierde en serio!" },
        { PLANO_CASILLA, CASILLA_TIENDA, GESTO_PRESENTAR,
          "Don Cambalache vende objetos. Traé monedas, que no fía." },
        { PLANO_CASILLA, CASILLA_REGALO, GESTO_SENALAR,
          "Caja con moño: monedas o un objeto gratis. ¡Eso me gusta!" },
        { PLANO_CASILLA, CASILLA_DUELO, GESTO_SENALAR,
          "Espadas cruzadas: elegís rival, apuestan y se enfrentan." },
        { PLANO_CASILLA, CASILLA_INTERCAMBIO, GESTO_SENALAR,
          "Flechas: los dados mueven monedas o trofeos entre dos jugadores." },
        { PLANO_DESPEDIDA, CASILLA_NEUTRA, GESTO_FESTEJAR,
          "¡Basta de charla! ¡A trabajar!" },
    }
};


static const GuionCinematicaTablero GUION_MERCADO =
{
    PERSONAJE_MOMO,
    {
        { PLANO_PANORAMA, CASILLA_NEUTRA, GESTO_NINGUNO,
          "Psst... soy Momo. Bienvenidos al Mercado Flotante." },
        { PLANO_GUIA, CASILLA_NEUTRA, GESTO_PRESENTAR,
          "Al atardecer las barcas llenan los canales. ¡Hay negocio!" },
        { PLANO_CASILLA, CASILLA_NEUTRA, GESTO_SENALAR,
          "Las azules son puestos: abrilos por 3 y los rivales te pagan 3." },
        { PLANO_TROFEO, CASILLA_NEUTRA, GESTO_SENALAR,
          "El trofeo del puerto cuesta 20 monedas. ¡Andá juntando!" },
        { PLANO_CASILLA, CASILLA_EVENTO, GESTO_SENALAR,
          "La exclamación hace sonar la campana: ¡feria o inspección!" },
        { PLANO_CASILLA, CASILLA_TIENDA, GESTO_PRESENTAR,
          "Don Cambalache vende objetos... y no acepta pescado." },
        { PLANO_CASILLA, CASILLA_REGALO, GESTO_SENALAR,
          "Caja con moño: monedas u objetos gratis. ¡Lo mejor!" },
        { PLANO_CASILLA, CASILLA_INTERCAMBIO, GESTO_SENALAR,
          "Flechas: los dados reparten monedas o trofeos... ¡o los cambian!" },
        { PLANO_CASILLA, CASILLA_DUELO, GESTO_SENALAR,
          "Espadas: retá a alguien, apuesten y que gane el mejor." },
        { PLANO_CASILLA, CASILLA_CAOS, GESTO_MANOS_CABEZA,
          "El rayo... ruleta de castigos. Yo que vos, la esquivo." },
        { PLANO_DESPEDIDA, CASILLA_NEUTRA, GESTO_FESTEJAR,
          "¡Abran sus puestos! ¡El mercado está abierto!" },
    }
};


static const GuionCinematicaTablero GUION_RELOJ =
{
    PERSONAJE_ROBOTO,
    {
        { PLANO_PANORAMA, CASILLA_NEUTRA, GESTO_NINGUNO,
          "Bip. Unidad Roboto en línea. Esto es la Torre del Reloj." },
        { PLANO_GUIA, CASILLA_NEUTRA, GESTO_SALUDAR,
          "El relojero premia a quien le dé cuerda al reloj más antiguo." },
        { PLANO_TROFEO, CASILLA_NEUTRA, GESTO_SENALAR,
          "Trofeo de bronce: 20 monedas. Cálculo exacto." },
        { PLANO_LANDMARK, CASILLA_NEUTRA, GESTO_PRESENTAR,
          "Desde la ronda 2 la esfera gira: las casillas de color se mueven." },
        { PLANO_CASILLA, CASILLA_EVENTO, GESTO_SENALAR,
          "Exclamación: la campanada da cuerda y +3... o retrocedés 3." },
        { PLANO_CASILLA, CASILLA_INTERCAMBIO, GESTO_SENALAR,
          "Flechas: tres dados deciden quién da, recibe o intercambia." },
        { PLANO_CASILLA, CASILLA_DUELO, GESTO_SENALAR,
          "Espadas: duelo uno contra uno con apuesta. Modo combate." },
        { PLANO_CASILLA, CASILLA_REGALO, GESTO_SENALAR,
          "Caja con moño: monedas u objeto. Alegría garantizada." },
        { PLANO_CASILLA, CASILLA_TIENDA, GESTO_PRESENTAR,
          "Tienda de Don Cambalache: monedas por objetos." },
        { PLANO_CASILLA, CASILLA_CAOS, GESTO_MANOS_CABEZA,
          "Rayo detectado: ruleta de castigos. ¡Error, error!" },
        { PLANO_DESPEDIDA, CASILLA_NEUTRA, GESTO_FESTEJAR,
          "Sincronizando relojes... ¡Tic, tac, a jugar!" },
    }
};


static const GuionCinematicaTablero GUION_FERIA =
{
    PERSONAJE_PIXI,
    {
        { PLANO_PANORAMA, CASILLA_NEUTRA, GESTO_NINGUNO,
          "¡Guau, la Feria Nocturna! Soy Pixi, ¡vengan conmigo!" },
        { PLANO_GUIA, CASILLA_NEUTRA, GESTO_PRESENTAR,
          "De noche todo brilla... ¡y cada moneda cuenta!" },
        { PLANO_CASILLA, CASILLA_NEGATIVA, GESTO_MANOS_CABEZA,
          "¡Ojo! Lo que perdés en las rojas cae al frasco del pozo." },
        { PLANO_CASILLA, CASILLA_EVENTO, GESTO_SENALAR,
          "En la exclamación girás la Gran Rueda: ¡podés ganar el pozo!" },
        { PLANO_TROFEO, CASILLA_NEUTRA, GESTO_SENALAR,
          "Desde la ronda 2 el trofeo se remata: 15, 20 o 25 monedas." },
        { PLANO_CASILLA, CASILLA_REGALO, GESTO_SENALAR,
          "¡Una caja con moño! Monedas o un objeto de regalo." },
        { PLANO_CASILLA, CASILLA_DUELO, GESTO_SENALAR,
          "Espadas: retá a alguien a un duelo y apuesten." },
        { PLANO_CASILLA, CASILLA_TIENDA, GESTO_PRESENTAR,
          "Don Cambalache abrió su puesto: objetos por monedas." },
        { PLANO_CASILLA, CASILLA_INTERCAMBIO, GESTO_SENALAR,
          "Flechas: los dados deciden quién da, recibe o intercambia." },
        { PLANO_CASILLA, CASILLA_CAOS, GESTO_MANOS_CABEZA,
          "El rayo es la ruleta de castigos. ¡Qué miedo!" },
        { PLANO_DESPEDIDA, CASILLA_NEUTRA, GESTO_FESTEJAR,
          "¡Que empiece la función!" },
    }
};


const GuionCinematicaTablero& ObtenerGuionCinematicaTablero(
    IdTablero id
)
{
    switch (id)
    {
        case TABLERO_FORJA_VOLCAN: return GUION_FORJA;
        case TABLERO_MERCADO_FLOTANTE: return GUION_MERCADO;
        case TABLERO_TORRE_RELOJ: return GUION_RELOJ;
        case TABLERO_FERIA_NOCTURNA: return GUION_FERIA;
        case TABLERO_ISLA_ARBOLEDA:
        case CANTIDAD_TABLEROS:
            break;
    }

    return GUION_ARBOLEDA;
}
