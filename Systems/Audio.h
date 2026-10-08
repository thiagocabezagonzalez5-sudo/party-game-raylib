#pragma once

#include "raylib.h"


enum TipoSonidoJuego
{
    SONIDO_UI_MOVER = 0,
    SONIDO_UI_CONFIRMAR,
    SONIDO_CUENTA_REGRESIVA,
    SONIDO_INICIO_MINIJUEGO,
    SONIDO_RECOGER_NUCLEO,
    SONIDO_RECOGER_NUCLEO_ESPECIAL,
    SONIDO_ALERTA_TIEMPO,
    SONIDO_RESULTADO,

    // UI
    SONIDO_UI_CANCELAR,

    // Jugador
    SONIDO_SALTO,
    SONIDO_ATERRIZAJE,
    SONIDO_GOLPE,
    SONIDO_CAIDA,
    SONIDO_GROUND_POUND,

    // Tablero
    SONIDO_DADO,
    SONIDO_PASO_TABLERO,
    SONIDO_MONEDA,
    SONIDO_COMPRA,
    SONIDO_CASILLA_POSITIVA,
    SONIDO_CASILLA_NEGATIVA,
    SONIDO_EVENTO_TABLERO,
    SONIDO_RULETA_TICK,

    // Minijuegos: eventos comunes que cualquier minijuego puede reutilizar
    SONIDO_RECOGER_OBJETO,
    SONIDO_EXPLOSION,
    SONIDO_BOTON,
    SONIDO_DISPARO,
    SONIDO_PLATAFORMA,
    SONIDO_IMPACTO,
    SONIDO_ACIERTO,
    SONIDO_ERROR,
    SONIDO_ELIMINADO,

    CANTIDAD_SONIDOS_JUEGO
};


// Musica compartida por categoria. No hace falta una pista por minijuego:
// si falta el archivo de una categoria, sigue sonando la pista actual.
enum CategoriaMusica
{
    MUSICA_MENU = 0,
    MUSICA_TABLERO,
    MUSICA_MINIJUEGO,
    MUSICA_RESULTADO,
    CANTIDAD_CATEGORIAS_MUSICA
};


struct AudioJuego
{
    Music musicas[CANTIDAD_CATEGORIAS_MUSICA]{};
    bool musicasCargadas[CANTIDAD_CATEGORIAS_MUSICA]{};

    // Categoria que suena ahora; -1 mientras todavia no empezo la musica.
    int musicaActual = -1;

    Sound sonidos[CANTIDAD_SONIDOS_JUEGO]{};
    bool sonidosCargados[CANTIDAD_SONIDOS_JUEGO]{};

    bool dispositivoInicializado = false;

    float volumenMusica = 0.35f;
    float volumenSonidos = 0.60f;

    void Inicializar();

    // Carga la musica del menu (ruta indicada) y las demas categorias
    // que existan en Assets/Audio/Musica/.
    void CargarMusicaMenu(
        const char* ruta
    );

    void ReproducirMusicaMenu();

    // Cambia de pista solo si la categoria tiene archivo y ya hay musica
    // sonando. Llamarla cada frame es seguro: si no cambia, no hace nada.
    void SeleccionarMusica(
        CategoriaMusica categoria
    );

    void ReproducirSonido(
        TipoSonidoJuego tipo
    );

    void Actualizar();

    void AplicarVolumenMusica(
        float volumen
    );

    void AplicarVolumenSonidos(
        float volumen
    );

    void Descargar();
};
