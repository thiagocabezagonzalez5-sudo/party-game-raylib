#include "Core/Juego.h"

#include "raylib.h"
#include "Systems/Input.h"


//==================================================
// PREPARAR DIRECTORIO DE RECURSOS
//==================================================

static void PrepararDirectorioDeRecursos()
{
    // Si Assets ya existe en la carpeta de trabajo,
    // no hace falta cambiar nada. Este es el caso
    // normal cuando se ejecuta desde Visual Studio Code.
    if (DirectoryExists("Assets"))
    {
        return;
    }


    // Al abrir directamente el .exe dentro de build,
    // Windows usa build como carpeta de trabajo.
    // Buscamos entonces Assets un nivel arriba.
    const char* directorioAplicacion =
        GetApplicationDirectory();


    if (
        directorioAplicacion != nullptr &&
        DirectoryExists(
            TextFormat(
                "%s../Assets",
                directorioAplicacion
            )
        )
    )
    {
        bool directorioCambiado =
            ChangeDirectory(
                TextFormat(
                    "%s..",
                    directorioAplicacion
                )
            );


        if (directorioCambiado)
        {
            TraceLog(
                LOG_INFO,
                "Directorio de recursos preparado: %s",
                GetWorkingDirectory()
            );
        }
        else
        {
            TraceLog(
                LOG_WARNING,
                "No se pudo acceder a la carpeta del proyecto"
            );
        }
    }
    else
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro la carpeta Assets junto al ejecutable ni un nivel arriba"
        );
    }
}


//==================================================
// INICIALIZAR
//==================================================

void Juego::Inicializar()
{
    InitWindow(
        1280,
        720,
        "Juego de Party"
    );


    PrepararDirectorioDeRecursos();


    SetExitKey(
        KEY_NULL
    );


    //==================================================
    // RESOLUCIONES
    //==================================================

    InicializarResoluciones();


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


    int indiceNativo =
        0;


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
            indiceNativo =
                i;


            break;
        }
    }


    //==================================================
    // CONFIGURACION
    //==================================================

    config =
        ConfiguracionJuego{};


    bool configEncontrada =
        CargarConfiguracion(
            rutaConfiguracion,
            config
        );


    if (!configEncontrada)
    {
        config.modoVentana =
            MODO_PANTALLA_COMPLETA;


        config.indiceResolucion =
            indiceNativo;


        config.indiceFPS =
            1;
    }


    //------------------------------
    // VALIDAR RESOLUCION
    //------------------------------

    if (
        config.indiceResolucion <
            0 ||

        config.indiceResolucion >=
            cantidadResoluciones
    )
    {
        config.indiceResolucion =
            indiceNativo;
    }


    //------------------------------
    // VALIDAR FPS
    //------------------------------

    if (
        config.indiceFPS <
            0 ||

        config.indiceFPS >=
            CANTIDAD_OPCIONES_FPS
    )
    {
        config.indiceFPS =
            1;
    }


    //------------------------------
    // VALIDAR MODO TECLADO
    //------------------------------

    if (
        config.modoTeclado <
            TECLADO_COMPLETO ||

        config.modoTeclado >
            TECLADO_DIVIDIDO
    )
    {
        config.modoTeclado =
            TECLADO_DIVIDIDO;
    }


    //------------------------------
    // VALIDAR VOLUMEN MUSICA
    //------------------------------

    if (
        config.volumenMusica <
        0.0f
    )
    {
        config.volumenMusica =
            0.0f;
    }


    if (
        config.volumenMusica >
        1.0f
    )
    {
        config.volumenMusica =
            1.0f;
    }


    //------------------------------
    // VALIDAR VOLUMEN SONIDOS
    //------------------------------

    if (
        config.volumenSonidos <
        0.0f
    )
    {
        config.volumenSonidos =
            0.0f;
    }


    if (
        config.volumenSonidos >
        1.0f
    )
    {
        config.volumenSonidos =
            1.0f;
    }


    //==================================================
    // PARTICIPANTES
    //==================================================

    cantidadParticipantes =
        0;


    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        participantes[i] =
            Participante{};


        participantes[i]
            .numeroJugador =
            i + 1;
    }


    ConfigurarControlesParticipantes(
        participantes,
        MAX_PARTICIPANTES,
        config.modoTeclado
    );


    //==================================================
    // FPS
    //==================================================

    SetTargetFPS(
        opcionesFPS[
            config.indiceFPS
        ]
    );


    //==================================================
    // MODO DE VENTANA
    //==================================================

    ModoVentana modoActual =
        MODO_VENTANA;


    AplicarModoVentana(
        modoActual,
        config.modoVentana,
        resoluciones[
            config.indiceResolucion
        ]
    );


    config.modoVentana =
        modoActual;


    //==================================================
    // AUDIO
    //==================================================

    audio.Inicializar();


    audio.AplicarVolumenMusica(
        config.volumenMusica
    );


    audio.AplicarVolumenSonidos(
        config.volumenSonidos
    );


    //==================================================
    // MENUS
    //==================================================

    menuConfiguracion
        .Inicializar();


    seleccionPersonajes
        .Inicializar(
            participantes,
            MAX_PARTICIPANTES
        );


    //==================================================
    // ZONA PRUEBAS
    //==================================================

    zonaPruebas
        .Inicializar(
            participantes,
            cantidadParticipantes
        );


    //==================================================
    // LOGO
    //==================================================

    pantallaLogo.Inicializar(
        "Assets/UI/LogoCreador.png"
    );


    //==================================================
    // CARGA
    //==================================================

    menuPreparado =
        false;


    cargaMenuSolicitada =
        false;


    //==================================================
    // ESTADO
    //==================================================

    estado =
        ESTADO_LOGO;


    cerrarJuego =
        false;
}


//==================================================
// RESOLUCIONES
//==================================================

void Juego::InicializarResoluciones()
{
    cantidadResoluciones =
        0;


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


    for (
        int i = 0;
        i <
        cantidadCandidatas;
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
    // NATIVA
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
    //==================================================
    // AUDIO GLOBAL
    //==================================================

    audio.Actualizar();


    //==================================================
    // ESTADOS
    //==================================================

    switch (estado)
    {
        //==================================================
        // LOGO
        //==================================================

        case ESTADO_LOGO:
        {
            pantallaLogo.Actualizar(
                deltaTime
            );


            //------------------------------
            // SOLICITAR CARGA
            //------------------------------

            if (
                !menuPreparado &&
                !cargaMenuSolicitada &&
                pantallaLogo.tiempo >=
                    1.0f
            )
            {
                cargaMenuSolicitada =
                    true;


                break;
            }


            //------------------------------
            // CARGAR MENU
            //------------------------------

            if (
                cargaMenuSolicitada &&
                !menuPreparado
            )
            {
                menuPrincipal
                    .Inicializar();


                audio.CargarMusicaMenu(
                    "Assets/Audio/MusicaMenu.mp3"
                );


                audio.AplicarVolumenMusica(
                    config.volumenMusica
                );


                menuPreparado =
                    true;


                cargaMenuSolicitada =
                    false;


                pantallaLogo.tiempo =
                    1.5f;
            }


            //------------------------------
            // TERMINAR LOGO
            //------------------------------

            if (
                menuPreparado &&
                pantallaLogo.Termino()
            )
            {
                menuPrincipal
                    .PrepararEntrada(
                        true
                    );


                audio
                    .ReproducirMusicaMenu();


                estado =
                    ESTADO_MENU;
            }


            break;
        }


        //==================================================
        // MENU PRINCIPAL
        //==================================================

        case ESTADO_MENU:
        {
            menuPrincipal
                .Actualizar(
                    deltaTime
                );


            //==================================================
            // EMPEZAR
            //==================================================

            if (
                menuPrincipal
                    .empezarJuego
            )
            {
                menuPrincipal
                    .empezarJuego =
                    false;


                cantidadParticipantes =
                    0;


                ConfigurarControlesParticipantes(
                    participantes,
                    MAX_PARTICIPANTES,
                    config.modoTeclado
                );


                seleccionPersonajes
                    .Inicializar(
                        participantes,
                        MAX_PARTICIPANTES
                    );


                estado =
                    ESTADO_SELECCION_JUGADORES;
            }


            //==================================================
            // CONFIGURACION
            //==================================================

            if (
                menuPrincipal
                    .abrirConfiguracion
            )
            {
                menuPrincipal
                    .abrirConfiguracion =
                    false;


                menuConfiguracion
                    .Inicializar();


                estado =
                    ESTADO_CONFIGURACION;
            }


            //==================================================
            // SALIR
            //==================================================

            if (
                menuPrincipal
                    .salir
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
            menuPrincipal
                .fondo
                .Actualizar(
                    deltaTime
                );


            menuConfiguracion
                .Actualizar(
                    config,
                    resoluciones,
                    cantidadResoluciones,
                    opcionesFPS,
                    CANTIDAD_OPCIONES_FPS,
                    audio
                );


            //------------------------------
            // GUARDAR CAMBIOS
            //------------------------------

            if (
                menuConfiguracion
                    .configuracionCambiada
            )
            {
                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );


                menuConfiguracion
                    .configuracionCambiada =
                    false;
            }


            //------------------------------
            // VOLVER
            //------------------------------

            if (
                menuConfiguracion
                    .volver
            )
            {
                menuConfiguracion
                    .volver =
                    false;


                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );


                menuPrincipal
                    .PrepararEntrada(
                        false
                    );


                estado =
                    ESTADO_MENU;
            }


            break;
        }


        //==================================================
        // SELECCION DE PERSONAJES
        //==================================================

        case ESTADO_SELECCION_JUGADORES:
        {
            menuPrincipal
                .fondo
                .Actualizar(
                    deltaTime
                );


            seleccionPersonajes
                .Actualizar(
                    deltaTime,
                    participantes,
                    MAX_PARTICIPANTES
                );


            if (
                seleccionPersonajes
                    .iniciarPartida
            )
            {
                int cantidadConfirmada =
                    0;


                for (
                    int i = 0;
                    i < MAX_PARTICIPANTES;
                    i++
                )
                {
                    if (participantes[i].activo)
                    {
                        cantidadConfirmada++;
                    }
                }


                if (
                    cantidadConfirmada == 2 ||
                    cantidadConfirmada == 4
                )
                {
                    cantidadParticipantes =
                        cantidadConfirmada;


                    seleccionPersonajes
                        .iniciarPartida =
                        false;


                    zonaPruebas
                        .Inicializar(
                            participantes,
                            cantidadParticipantes
                        );


                    estado =
                        ESTADO_ZONA_PRUEBAS;


                    break;
                }
            }


            if (
                seleccionPersonajes
                    .volverAlMenu
            )
            {
                seleccionPersonajes
                    .volverAlMenu =
                    false;


                menuPrincipal
                    .PrepararEntrada(
                        false
                    );


                estado =
                    ESTADO_MENU;
            }


            break;
        }


        //==================================================
        // ZONA DE PRUEBAS
        //==================================================

        case ESTADO_ZONA_PRUEBAS:
        {
            zonaPruebas
                .Actualizar(
                    deltaTime
                );


            //------------------------------
            // VOLVER AL MENU
            //------------------------------

            if (
                zonaPruebas
                    .volverAlMenu
            )
            {
                zonaPruebas
                    .volverAlMenu =
                    false;


                menuPrincipal
                    .PrepararEntrada(
                        false
                    );


                estado =
                    ESTADO_MENU;
            }


            break;
        }


        //==================================================
        // PARTIDA
        //==================================================

        case ESTADO_PARTIDA:
        {
            break;
        }


        //==================================================
        // MINIJUEGO
        //==================================================

        case ESTADO_MINIJUEGO:
        {
            break;
        }


        //==================================================
        // RESULTADO
        //==================================================

        case ESTADO_RESULTADO:
        {
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
            pantallaLogo
                .Dibujar();


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

                    GetScreenWidth() /
                        2 -
                        ancho /
                        2,

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
            menuPrincipal
                .Dibujar();


            break;
        }


        //==================================================
        // CONFIG
        //==================================================

        case ESTADO_CONFIGURACION:
        {
            menuPrincipal
                .fondo
                .DibujarPantallaCompleta();


            menuConfiguracion
                .Dibujar(
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
            menuPrincipal
                .fondo
                .DibujarPantallaCompleta();


            seleccionPersonajes
                .Dibujar(
                    participantes,
                    MAX_PARTICIPANTES
                );


            break;
        }


        //==================================================
        // ZONA DE PRUEBAS
        //==================================================

        case ESTADO_ZONA_PRUEBAS:
        {
            zonaPruebas
                .Dibujar();


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
    // FPS GLOBAL
    //==================================================

    if (
        config.mostrarFPS &&
        estado != ESTADO_LOGO
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


    //------------------------------
    // ZONA DE PRUEBAS / MODELOS
    //------------------------------

    zonaPruebas
        .Descargar();


    //------------------------------
    // PERSONAJES
    //------------------------------

    seleccionPersonajes
        .Descargar();


    //------------------------------
    // AUDIO
    //------------------------------

    audio.Descargar();


    //------------------------------
    // LOGO
    //------------------------------

    pantallaLogo
        .Descargar();


    //------------------------------
    // MENU
    //------------------------------

    menuPrincipal
        .Descargar();
}
