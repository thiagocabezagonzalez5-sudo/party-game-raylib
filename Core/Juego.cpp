#include "Core/Juego.h"

#include "raylib.h"


//==================================================
// INICIALIZAR
//==================================================

void Juego::Inicializar()
{
    //------------------------------
    // VENTANA PROVISIONAL
    //------------------------------
    // Primero Raylib necesita crear una
    // ventana para poder consultar el monitor.

    InitWindow(
        1280,
        720,
        "Juego de Party"
    );


    /*
        Evitamos que ESC cierre el programa.

        Nosotros vamos a manejar ESC
        desde nuestros menus.
    */

    SetExitKey(
        KEY_NULL
    );


    //------------------------------
    // RESOLUCIONES
    //------------------------------

    InicializarResoluciones();


    //------------------------------
    // FPS
    //------------------------------

        //==================================================
    // RESOLUCION NATIVA DEL MONITOR
    //==================================================

    int monitor =
        GetCurrentMonitor();

    int anchoMonitor =
        GetMonitorWidth(
            monitor
        );

    int altoMonitor =
        GetMonitorHeight(
            monitor
        );

    int indiceNativo = 0;


    // Buscar cual de nuestras resoluciones
    // coincide con la resolucion del monitor
    for (
        int i = 0;
        i < cantidadResoluciones;
        i++
    )
    {
        if (
            resoluciones[i].ancho ==
                anchoMonitor &&
            resoluciones[i].alto ==
                altoMonitor
        )
        {
            indiceNativo = i;

            break;
        }
    }


    //==================================================
    // CARGAR CONFIGURACION GUARDADA
    //==================================================

    /*
        Primero usamos los valores por defecto
        definidos en ConfiguracionJuego.

        Luego intentamos reemplazarlos por los
        guardados en config.ini.
    */

    config =
        ConfiguracionJuego{};


    CargarConfiguracion(
        rutaConfiguracion,
        config
    );


    //==================================================
    // VALIDAR RESOLUCION
    //==================================================

    /*
        Puede pasar que el archivo config.ini
        tenga guardado un indice que ya no existe.

        Por ejemplo:
        - cambio de monitor
        - cambio de lista de resoluciones
    */

    if (
        config.indiceResolucion < 0 ||
        config.indiceResolucion >=
            cantidadResoluciones
    )
    {
        config.indiceResolucion =
            indiceNativo;
    }


    //==================================================
    // VALIDAR FPS
    //==================================================

    if (
        config.indiceFPS < 0 ||
        config.indiceFPS >=
            CANTIDAD_OPCIONES_FPS
    )
    {
        // indice 1 = 60 FPS
        config.indiceFPS = 1;
    }


    //==================================================
    // VALIDAR MODO DE VENTANA
    //==================================================

    if (
        config.modoVentana <
            MODO_VENTANA ||
        config.modoVentana >
            MODO_SIN_BORDES
    )
    {
        config.modoVentana =
            MODO_PANTALLA_COMPLETA;
    }


    //==================================================
    // PRIMER INICIO
    //==================================================

    /*
        Si config.ini no existe todavía,
        CargarConfiguracion devuelve false.

        En ese caso queremos arrancar en:
        - pantalla completa
        - resolucion nativa
        - 60 FPS
    */

    bool configuracionEncontrada =
        FileExists(
            rutaConfiguracion
        );


    if (!configuracionEncontrada)
    {
        config.modoVentana =
            MODO_PANTALLA_COMPLETA;

        config.indiceResolucion =
            indiceNativo;

        config.indiceFPS =
            1;
    }


    //==================================================
    // APLICAR FPS
    //==================================================

    SetTargetFPS(
        opcionesFPS[
            config.indiceFPS
        ]
    );


    //==================================================
    // APLICAR MODO DE VENTANA
    //==================================================

    /*
        Raylib acaba de crear una ventana normal.

        Por eso nuestro modo actual antes de
        aplicar la configuracion es VENTANA.
    */

    ModoVentana modoActual =
        MODO_VENTANA;


    AplicarModoVentana(
        modoActual,
        config.modoVentana,
        resoluciones[
            config.indiceResolucion
        ]
    );


    /*
        Guardamos el resultado por si
        AplicarModoVentana modifico algo.
    */

    config.modoVentana =
        modoActual;


    //==================================================
    // AUDIO
    //==================================================

    /*
        Inicializamos el dispositivo ahora.

        La musica pesada NO se carga aca.
        La vamos a cargar mientras se muestra
        el logo.
    */

    audio.Inicializar();


    audio.AplicarVolumenMusica(
        config.volumenMusica
    );


    audio.AplicarVolumenSonidos(
        config.volumenSonidos
    );


    //==================================================
    // MENU DE CONFIGURACION
    //==================================================

    menuConfiguracion
        .Inicializar();


    //==================================================
    // LOGO DEL CREADOR
    //==================================================

    pantallaLogo.Inicializar(
        "Assets/UI/LogoCreador.png"
    );


    //==================================================
    // MENU PRINCIPAL
    //==================================================

    /*
        NO llamamos:

        menuPrincipal.Inicializar();

        porque esa funcion carga el GIF.

        Queremos cargarlo DESPUES de que
        el logo ya haya aparecido en pantalla.
    */

    menuPreparado =
        false;

    cargaMenuSolicitada =
        false;


    //==================================================
    // ESTADO INICIAL
    //==================================================

    estado =
        ESTADO_LOGO;


    cerrarJuego =
        false;
}


//==================================================
// CREAR LISTA DE RESOLUCIONES
//==================================================

void Juego::InicializarResoluciones()
{
    cantidadResoluciones = 0;


    int monitor =
        GetCurrentMonitor();


    int anchoMonitor =
        GetMonitorWidth(
            monitor
        );


    int altoMonitor =
        GetMonitorHeight(
            monitor
        );


    //------------------------------
    // RESOLUCIONES COMUNES
    //------------------------------

    Resolucion candidatas[] =
    {
        { 800, 600 },

        { 1024, 576 },

        { 1280, 720 },

        { 1366, 768 },

        { 1600, 900 },

        { 1920, 1080 },

        { 2560, 1440 },

        { 3840, 2160 }
    };


    const int cantidadCandidatas =
        sizeof(candidatas) /
        sizeof(candidatas[0]);


    //------------------------------
    // SOLO LAS QUE CABEN EN MONITOR
    //------------------------------

    for (
        int i = 0;
        i < cantidadCandidatas;
        i++
    )
    {
        if (
            candidatas[i].ancho <=
                anchoMonitor &&
            candidatas[i].alto <=
                altoMonitor
        )
        {
            AgregarResolucion(
                resoluciones,
                cantidadResoluciones,
                MAX_RESOLUCIONES,
                candidatas[i].ancho,
                candidatas[i].alto
            );
        }
    }


    //------------------------------
    // ASEGURAR RESOLUCION NATIVA
    //------------------------------

    AgregarResolucion(
        resoluciones,
        cantidadResoluciones,
        MAX_RESOLUCIONES,
        anchoMonitor,
        altoMonitor
    );
}


//==================================================
// ACTUALIZAR
//==================================================

void Juego::Actualizar(
    float deltaTime
)
{
    audio.Actualizar();

    switch (estado)
    {
        //==================================================
        // LOGO
        //==================================================
        case ESTADO_LOGO:
        {
            //------------------------------------------
            // ACTUALIZAR LOGO
            //------------------------------------------

            pantallaLogo.Actualizar(
                deltaTime
            );


            //------------------------------------------
            // PASO 1:
            // MOSTRAR "CARGANDO..."
            //------------------------------------------

            /*
                Esperamos hasta que el logo ya haya
                aparecido completamente.

                Cuando llegamos a 1 segundo activamos
                la pantalla de carga, pero todavía
                NO cargamos nada.

                Así damos un frame para que
                "CARGANDO..." realmente aparezca.
            */

            if (
                !menuPreparado &&
                !cargaMenuSolicitada &&
                pantallaLogo.tiempo >= 1.0f
            )
            {
                cargaMenuSolicitada = true;

                break;
            }


            //------------------------------------------
            // PASO 2:
            // CARGAR RECURSOS
            //------------------------------------------

            if (
                cargaMenuSolicitada &&
                !menuPreparado
            )
            {
                //------------------------------
                // GIF
                //------------------------------

                menuPrincipal.Inicializar();


                //------------------------------
                // MUSICA
                //------------------------------

                audio.CargarMusicaMenu(
                    "Assets/Audio/MusicaMenu.mp3"
                );


                //------------------------------
                // LISTO
                //------------------------------

                menuPreparado = true;

                cargaMenuSolicitada = false;


                /*
                    Mantenemos la intro todavía
                    un rato después de cargar.

                    Así un frame muy lento debido
                    al GIF no nos salta directamente
                    al menú.
                */

                pantallaLogo.tiempo = 1.5f;
            }


            //------------------------------------------
            // TERMINAR INTRO
            //------------------------------------------

            if (
                menuPreparado &&
                pantallaLogo.Termino()
            )
            {
                //------------------------------
                // FADE MENU
                //------------------------------

                menuPrincipal.PrepararEntrada(
                    true
                );


                //------------------------------
                // MUSICA
                //------------------------------

                audio.ReproducirMusicaMenu();


                //------------------------------
                // MENU PRINCIPAL
                //------------------------------

                estado = ESTADO_MENU;
            }


            break;
        }
        //==================================================
        // MENU
        //==================================================

        case ESTADO_MENU:
        {
            menuPrincipal
                .Actualizar(deltaTime);


            //------------------------------
            // JUGAR
            //------------------------------

            if (
                menuPrincipal
                    .empezarJuego
            )
            {
                menuPrincipal
                    .empezarJuego =
                    false;


                estado =
                    ESTADO_SELECCION_JUGADORES;
            }


            //------------------------------
            // CONFIGURACION
            //------------------------------

            if (
                menuPrincipal
                    .abrirConfiguracion
            )
            {
                menuPrincipal
                    .abrirConfiguracion =
                    false;


                /*
                    IMPORTANTE:

                    NO volvemos a llamar
                    Inicializar().

                    Si lo hicieramos,
                    perderiamos los cambios
                    de resolucion/FPS.
                */

                menuConfiguracion.Inicializar();

                estado =
                    ESTADO_CONFIGURACION;
            }


            //------------------------------
            // SALIR
            //------------------------------

            if (
                menuPrincipal.salir
            )
            {
                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );

                cerrarJuego =
                    true;
            }


            break;
        }


        //==================================================
        // CONFIGURACION
        //==================================================

        case ESTADO_CONFIGURACION:
        {
            menuConfiguracion.Actualizar(
                config,
                resoluciones,
                cantidadResoluciones,
                opcionesFPS,
                CANTIDAD_OPCIONES_FPS,
                audio
            );

            if (menuConfiguracion.configuracionCambiada)
            {
                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );

                menuConfiguracion.configuracionCambiada = false;
            }

            if (menuConfiguracion.volver)
            {
                menuConfiguracion.volver = false;

                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );

                menuPrincipal.PrepararEntrada(false);

                estado = ESTADO_MENU;
            }

            break;
        }


        //==================================================
        // SELECCION
        //==================================================

        case ESTADO_SELECCION_JUGADORES:
        {
            // Futuro

            break;
        }


        //==================================================
        // PARTIDA
        //==================================================

        case ESTADO_PARTIDA:
        {
            // partida.Actualizar(deltaTime);

            break;
        }


        //==================================================
        // MINIJUEGO
        //==================================================

        case ESTADO_MINIJUEGO:
        {
            // partida.ActualizarMinijuego(deltaTime);

            break;
        }


        //==================================================
        // RESULTADO
        //==================================================

        case ESTADO_RESULTADO:
        {
            // partida.ActualizarResultado(deltaTime);

            break;
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void Juego::Dibujar()
{
    switch (estado)
    {
        //==================================================
        // LOGO
        //==================================================
        case ESTADO_LOGO:
        {
            //------------------------------
            // LOGO
            //------------------------------

            pantallaLogo.Dibujar();


            //------------------------------
            // CARGANDO
            //------------------------------

            if (
                cargaMenuSolicitada &&
                !menuPreparado
            )
            {
                const char* texto =
                    "CARGANDO...";


                int tamano =
                    24;


                int ancho =
                    MeasureText(
                        texto,
                        tamano
                    );


                DrawText(
                    texto,

                    GetScreenWidth() / 2 -
                        ancho / 2,

                    GetScreenHeight() -
                        100,

                    tamano,

                    BLACK
                );
            }


            break;
        }
        //==================================================
        // MENU
        //==================================================

        case ESTADO_MENU:
        {
            ClearBackground(
                RAYWHITE
            );


            menuPrincipal
                .Dibujar();


            break;
        }


        //==================================================
        // CONFIGURACION
        //==================================================

        case ESTADO_CONFIGURACION:
        {
            menuConfiguracion.Dibujar(
                config,
                resoluciones,
                opcionesFPS
            );

            break;
        }


        //==================================================
        // SELECCION
        //==================================================

        case ESTADO_SELECCION_JUGADORES:
        {
            ClearBackground(
                RAYWHITE
            );


            DrawText(
                "Seleccion de jugadores",
                200,
                200,
                30,
                BLACK
            );


            break;
        }


        //==================================================
        // PARTIDA
        //==================================================

        case ESTADO_PARTIDA:
        {
            ClearBackground(
                SKYBLUE
            );


            break;
        }


        //==================================================
        // MINIJUEGO
        //==================================================

        case ESTADO_MINIJUEGO:
        {
            ClearBackground(
                SKYBLUE
            );


            break;
        }


        //==================================================
        // RESULTADO
        //==================================================

        case ESTADO_RESULTADO:
        {
            ClearBackground(
                BLACK
            );


            break;
        }
    }


    //==================================================
    // CONTADOR DE FPS GLOBAL
    //==================================================

    if (
        config.mostrarFPS
    )
    {
        const char* texto =
            TextFormat(
                "FPS: %d",
                GetFPS()
            );


        int anchoTexto =
            MeasureText(
                texto,
                20
            );


        DrawText(
            texto,
            GetScreenWidth() -
                anchoTexto -
                20,
            20,
            20,
            DARKGREEN
        );
    }
}


//==================================================
// CERRAR
//==================================================

bool Juego::DebeCerrar()
{
    return cerrarJuego;
}


//==================================================
// DESCARGAR
//==================================================

void Juego::Descargar()
{
    GuardarConfiguracion(
        rutaConfiguracion,
        config
    );

    audio.Descargar();

    pantallaLogo.Descargar();

    menuPrincipal.Descargar();
}