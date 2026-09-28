#include "Gameplay/ZonaPruebas.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"


static const char* NombreModoPrueba(
    const ZonaPruebas& zona
)
{
    if (zona.modoActual == PRUEBA_MINIJUEGO)
    {
        return ObtenerDatosMinijuego(
            zona.gestorMinijuegos.ObtenerIdActivo()
        ).etiquetaZonaPruebas;
    }

    switch (zona.modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL: return "1 - ZONA PRINCIPAL";
        case PRUEBA_MODELOS: return "4 - PRUEBA DE MODELOS";
        case PRUEBA_TABLERO: return "7 - TABLERO";
        case PRUEBA_MINIJUEGO: break;
    }

    return "PRUEBA";
}


static void ConfigurarZonaPrincipal(
    ZonaPruebas& zona
)
{
    zona.cantidadBloquesPrincipal = 0;

    AgregarBloquePrueba(
        zona.bloquesPrincipal,
        zona.cantidadBloquesPrincipal,
        MAX_BLOQUES_PRUEBA,
        { 0.0f, -0.5f, 0.0f },
        { 14.0f, 1.0f, 14.0f },
        Color{ 105, 105, 115, 255 }
    );

    AgregarBloquePrueba(
        zona.bloquesPrincipal,
        zona.cantidadBloquesPrincipal,
        MAX_BLOQUES_PRUEBA,
        { -3.0f, 0.5f, -2.0f },
        { 2.5f, 1.0f, 2.5f },
        ORANGE
    );

    AgregarBloquePrueba(
        zona.bloquesPrincipal,
        zona.cantidadBloquesPrincipal,
        MAX_BLOQUES_PRUEBA,
        { 2.0f, 1.0f, -1.0f },
        { 3.0f, 2.0f, 3.0f },
        BLUE
    );

    zona.camaraPrincipal.position = { 0.0f, 6.5f, 13.0f };
    zona.camaraPrincipal.target = { 0.0f, 1.0f, -1.0f };
    zona.camaraPrincipal.up = { 0.0f, 1.0f, 0.0f };
    zona.camaraPrincipal.fovy = 50.0f;
    zona.camaraPrincipal.projection = CAMERA_PERSPECTIVE;
}


static void ConfigurarJugadoresPrincipal(
    ZonaPruebas& zona
)
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -1.2f, 1.0f, 4.0f },
        {  1.2f, 1.0f, 4.0f },
        { -2.4f, 1.0f, 3.0f },
        {  2.4f, 1.0f, 3.0f }
    };

    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(
            zona.jugadores[i],
            spawns[i]
        );
    }
}


static void ActualizarZonaPrincipal(
    ZonaPruebas& zona,
    float deltaTime
)
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        const Participante& participante = zona.participantes[i];

        if (!participante.activo)
        {
            continue;
        }

        ActualizarJugadorMinijuegoEstandar(
            zona.jugadores[i],
            participante,
            zona.bloquesPrincipal,
            zona.cantidadBloquesPrincipal,
            zona.particulas,
            MAX_PARTICULAS_TIERRA,
            true,
            deltaTime
        );
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        zona.jugadores,
        zona.participantes,
        MAX_JUGADORES_PRUEBA,
        zona.particulas,
        MAX_PARTICULAS_TIERRA
    );
}


static void DibujarZonaPrincipal(
    const ZonaPruebas& zona
)
{
    ClearBackground(Color{ 125, 190, 220, 255 });

    BeginMode3D(zona.camaraPrincipal);

    for (int i = 0; i < zona.cantidadBloquesPrincipal; i++)
    {
        const BloquePrueba& bloque = zona.bloquesPrincipal[i];

        DrawCube(
            bloque.posicion,
            bloque.tamano.x,
            bloque.tamano.y,
            bloque.tamano.z,
            bloque.color
        );

        DrawCubeWires(
            bloque.posicion,
            bloque.tamano.x,
            bloque.tamano.y,
            bloque.tamano.z,
            BLACK
        );

        if (zona.mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloque), YELLOW);
        }
    }

    DibujarParticulasTierra(
        zona.particulas,
        MAX_PARTICULAS_TIERRA
    );

    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        DibujarJugadorCuboPrueba(
            zona.jugadores[i],
            zona.participantes[i]
        );

        if (
            zona.mostrarDebug &&
            zona.participantes[i].activo &&
            zona.participantes[i].conectado &&
            !zona.jugadores[i].cayendo
        )
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(zona.jugadores[i]),
                LIME
            );
        }
    }

    DrawGrid(30, 1.0f);
    EndMode3D();

    DrawText("ZONA PRINCIPAL DE PRUEBAS", 25, 25, 30, BLACK);
    DrawText(
        "MOVIMIENTO + SALTO + GOLPES + COLISION SOLIDA",
        25,
        70,
        20,
        BLACK
    );
    DrawText(
        "SALTO EN EL AIRE: PREPARA 0.5s Y HACE GROUND POUND",
        25,
        100,
        18,
        DARKGRAY
    );
}


static void ReiniciarEstadoExtendidoJugador(
    JugadorPrueba& jugador
)
{
    jugador.preparandoGolpeSuelo = false;
    jugador.tiempoPreparacionGolpeSuelo = 0.0f;
    jugador.golpeSueloRecibido = false;
    jugador.multiplicadorRalentizacion = 1.0f;
}


static void PrepararTemaVisualZona(
    const ZonaPruebas& zona
)
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    if (zona.modoActual == PRUEBA_MINIJUEGO)
    {
        zona.gestorMinijuegos.PrepararTemaVisualActivo();
    }
}


void ZonaPruebas::Inicializar(
    Participante participantesJuego[],
    int cantidadParticipantesJuego,
    AudioJuego* audioJuego
)
{
    volverAlMenu = false;
    mostrarDebug = false;
    modoCatalogo = false;

    participantes = participantesJuego;
    cantidadParticipantes = cantidadParticipantesJuego;
    audio = audioJuego;

    contextoMinijuego.jugadores = jugadores;
    contextoMinijuego.cantidadJugadores = MAX_JUGADORES_PRUEBA;
    contextoMinijuego.participantes = participantes;
    contextoMinijuego.particulas = particulas;
    contextoMinijuego.cantidadParticulas = MAX_PARTICULAS_TIERRA;
    contextoMinijuego.audio = audio;

    InicializarTexturasTematicasMinijuegos();
    InicializarModelosEscenariosRetro3D();

    for (int i = 0; i < MAX_PARTICULAS_TIERRA; i++)
    {
        particulas[i].activa = false;
    }

    ConfigurarZonaPrincipal(*this);

    gestorMinijuegos.Inicializar();

    prototipoTablero.Inicializar(
        participantes,
        cantidadParticipantes
    );

    CambiarModo(PRUEBA_ZONA_PRINCIPAL);
}


void ZonaPruebas::CambiarModo(
    ModoZonaPruebas nuevoModo
)
{
    modoActual = nuevoModo;

    for (int i = 0; i < MAX_PARTICULAS_TIERRA; i++)
    {
        particulas[i].activa = false;
    }

    switch (modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            ConfigurarZonaPrincipal(*this);
            ConfigurarJugadoresPrincipal(*this);
            gestorMinijuegos.ReiniciarJugadoresCompartidos(
                contextoMinijuego
            );
            break;

        case PRUEBA_MODELOS:
            pruebaModelos.Inicializar();
            break;

        case PRUEBA_TABLERO:
            prototipoTablero.Reiniciar();
            gestorMinijuegos.ReiniciarJugadoresCompartidos(
                contextoMinijuego
            );
            break;

        case PRUEBA_MINIJUEGO:
            break;
    }
}


void ZonaPruebas::CambiarMinijuego(
    IdMinijuego nuevoMinijuego
)
{
    if (!EsIdMinijuegoValido(nuevoMinijuego))
    {
        return;
    }

    modoActual = PRUEBA_MINIJUEGO;

    for (int i = 0; i < MAX_PARTICULAS_TIERRA; i++)
    {
        particulas[i].activa = false;
    }

    gestorMinijuegos.ActivarMinijuego(
        nuevoMinijuego,
        contextoMinijuego
    );
}


static void ReiniciarModoActual(
    ZonaPruebas& zona
)
{
    if (zona.modoActual == PRUEBA_MINIJUEGO)
    {
        zona.gestorMinijuegos.ReiniciarActivo(
            zona.contextoMinijuego
        );
        return;
    }

    switch (zona.modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            ConfigurarJugadoresPrincipal(zona);
            break;

        case PRUEBA_MODELOS:
            zona.pruebaModelos.Reiniciar();
            break;

        case PRUEBA_TABLERO:
            zona.prototipoTablero.Reiniciar();
            break;

        case PRUEBA_MINIJUEGO:
            break;
    }

    if (zona.modoActual != PRUEBA_MODELOS)
    {
        for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
        {
            ReiniciarEstadoExtendidoJugador(zona.jugadores[i]);
        }
    }
}


void ZonaPruebas::Actualizar(
    float deltaTime
)
{
    ActualizarEfectosVisualesMinijuegos(deltaTime);

    if (IsKeyPressed(KEY_ESCAPE))
    {
        volverAlMenu = true;
        return;
    }

    if (IsKeyPressed(KEY_F3))
    {
        mostrarDebug = !mostrarDebug;
    }

    if (!modoCatalogo)
    {
        if (IsKeyPressed(KEY_ONE)) { CambiarModo(PRUEBA_ZONA_PRINCIPAL); return; }
        if (IsKeyPressed(KEY_TWO)) { CambiarMinijuego(MINIJUEGO_COLOR_SEGURO); return; }
        if (IsKeyPressed(KEY_THREE)) { CambiarMinijuego(MINIJUEGO_PELOTAS); return; }
        if (IsKeyPressed(KEY_FOUR)) { CambiarModo(PRUEBA_MODELOS); return; }
        if (IsKeyPressed(KEY_FIVE)) { CambiarMinijuego(MINIJUEGO_TRONCO); return; }
        if (IsKeyPressed(KEY_SIX)) { CambiarMinijuego(MINIJUEGO_FABRICA_67); return; }
        if (IsKeyPressed(KEY_SEVEN)) { CambiarModo(PRUEBA_TABLERO); return; }
        if (IsKeyPressed(KEY_EIGHT)) { CambiarMinijuego(MINIJUEGO_ISLA_FUEGO); return; }
        if (IsKeyPressed(KEY_NINE)) { CambiarMinijuego(MINIJUEGO_CAPITAN_MANDA); return; }
        if (IsKeyPressed(KEY_ZERO)) { CambiarMinijuego(MINIJUEGO_BARRA_GIRATORIA); return; }
        if (IsKeyPressed(KEY_F1)) { CambiarMinijuego(MINIJUEGO_CARGA_INESTABLE); return; }
        if (IsKeyPressed(KEY_F2)) { CambiarMinijuego(MINIJUEGO_SECUENCIA_NEON); return; }
        if (IsKeyPressed(KEY_F4)) { CambiarMinijuego(MINIJUEGO_CIRCUITO_VOLTAJE); return; }
        if (IsKeyPressed(KEY_F5)) { CambiarMinijuego(MINIJUEGO_TRAZO_PERFECTO); return; }
        if (IsKeyPressed(KEY_F6)) { CambiarMinijuego(MINIJUEGO_CONTEO_EXPLOSIVO); return; }
        if (IsKeyPressed(KEY_F7)) { CambiarMinijuego(MINIJUEGO_PASO_SILENCIOSO); return; }
        if (IsKeyPressed(KEY_F8)) { CambiarMinijuego(MINIJUEGO_TORMENTA_MAGNETICA); return; }
        if (IsKeyPressed(KEY_F9)) { CambiarMinijuego(MINIJUEGO_MUROS_LOCOS); return; }
        if (IsKeyPressed(KEY_F10)) { CambiarMinijuego(MINIJUEGO_NUCLEOS_ENERGIA); return; }
        if (IsKeyPressed(KEY_F11)) { CambiarMinijuego(MINIJUEGO_REFUGIO_PINCHOS); return; }
        if (IsKeyPressed(KEY_F12)) { CambiarMinijuego(MINIJUEGO_MIRADAS_CRUZADAS); return; }
        if (IsKeyPressed(KEY_PAGE_UP)) { CambiarMinijuego(MINIJUEGO_INTERRUPTORES_CAOS); return; }
        if (IsKeyPressed(KEY_PAGE_DOWN)) { CambiarMinijuego(MINIJUEGO_TANQUES_PLASMA); return; }
        if (IsKeyPressed(KEY_HOME)) { CambiarMinijuego(MINIJUEGO_PASARELAS_VACIO); return; }
        if (IsKeyPressed(KEY_END)) { CambiarMinijuego(MINIJUEGO_CANTERA_FUGA); return; }
    }

    if (IsKeyPressed(KEY_R))
    {
        ReiniciarModoActual(*this);
        return;
    }

    ActualizarParticulasTierra(
        particulas,
        MAX_PARTICULAS_TIERRA,
        deltaTime
    );

    if (modoActual != PRUEBA_MODELOS)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            bool estabaConectado = participantes[i].conectado;

            ActualizarConexionParticipante(participantes[i]);

            if (
                !participantes[i].activo ||
                participantes[i].esBot ||
                participantes[i].conectado == estabaConectado
            )
            {
                continue;
            }

            // Estos modos reemplazan temporalmente el mando por IA.
            // Reconectar devuelve el control sin teletransportar al inicio.
            if (
                modoActual == PRUEBA_MINIJUEGO &&
                gestorMinijuegos.MantieneControlIAEnReconexion()
            )
            {
                continue;
            }

            if (participantes[i].conectado)
            {
                gestorMinijuegos.ReiniciarJugadorCompartido(
                    contextoMinijuego,
                    i
                );
            }
            else
            {
                jugadores[i].velocidad = {};
                jugadores[i].empuje = {};
                jugadores[i].cayendo = false;
                jugadores[i].enSuelo = false;
                ReiniciarEstadoExtendidoJugador(jugadores[i]);
            }
        }
    }

    switch (modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            ActualizarZonaPrincipal(*this, deltaTime);
            break;

        case PRUEBA_MODELOS:
            pruebaModelos.Actualizar(deltaTime);
            break;

        case PRUEBA_TABLERO:
            prototipoTablero.Actualizar(deltaTime);
            break;

        case PRUEBA_MINIJUEGO:
            gestorMinijuegos.ActualizarActivo(
                deltaTime,
                contextoMinijuego
            );
            break;
    }
}


void ZonaPruebas::Dibujar() const
{
    PrepararTemaVisualZona(*this);

    switch (modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            DibujarZonaPrincipal(*this);
            break;

        case PRUEBA_MODELOS:
            pruebaModelos.Dibujar();
            break;

        case PRUEBA_TABLERO:
            prototipoTablero.Dibujar(mostrarDebug);
            break;

        case PRUEBA_MINIJUEGO:
            gestorMinijuegos.DibujarActivo(
                contextoMinijuego,
                mostrarDebug
            );
            break;
    }

    if (modoCatalogo)
    {
        DrawRectangle(
            18,
            GetScreenHeight() - 54,
            430,
            36,
            Fade(BLACK, 0.60f)
        );

        DrawText(
            "R REINICIAR   F3 DEBUG   ESC VOLVER AL CATALOGO",
            28,
            GetScreenHeight() - 45,
            17,
            RAYWHITE
        );

        return;
    }

    DrawRectangle(
        18,
        GetScreenHeight() - 211,
        GetScreenWidth() - 36,
        189,
        Fade(RAYWHITE, 0.86f)
    );

    DrawText(
        "1 PRINCIPAL  2 COLOR  3 PELOTAS  4 MODELOS  5 TRONCO  6 FABRICA  7 TABLERO  8 ISLA  9 CAPITAN  0 BARRA",
        30,
        GetScreenHeight() - 196,
        14,
        BLACK
    );

    DrawText(
        "F1 CARGA  F2 SECUENCIA  F4 CIRCUITO  F5 TRAZO  F6 CONTEO  F7 SIGILO",
        30,
        GetScreenHeight() - 169,
        15,
        DARKBLUE
    );

    DrawText(
        "F8 MAGNETICA  F9 MUROS  F10 NUCLEOS  F11 TALADROS  F12 MIRADAS  PGUP INTERRUPTORES  PGDN TANQUES",
        30,
        GetScreenHeight() - 142,
        15,
        DARKBLUE
    );

    DrawText(
        "INICIO PASARELAS  FIN CANTERA",
        30,
        GetScreenHeight() - 115,
        15,
        DARKBLUE
    );

    DrawText(
        TextFormat("ACTUAL: %s", NombreModoPrueba(*this)),
        30,
        GetScreenHeight() - 87,
        19,
        DARKBLUE
    );

    DrawText(
        "R REINICIAR   F3 DEBUG   ESC MENU",
        30,
        GetScreenHeight() - 59,
        17,
        DARKGRAY
    );

    if (modoActual != PRUEBA_MODELOS)
    {
        DrawText(
            TextFormat(
                "JUGADORES ACTIVOS: %d / 4",
                cantidadParticipantes
            ),
            GetScreenWidth() - 245,
            GetScreenHeight() - 59,
            17,
            DARKGREEN
        );
    }
}


const ResultadoMinijuego*
ZonaPruebas::ObtenerResultadoMinijuego() const
{
    if (modoActual != PRUEBA_MINIJUEGO)
    {
        return nullptr;
    }

    return gestorMinijuegos.ObtenerResultadoActivo();
}


void ZonaPruebas::Descargar()
{
    gestorMinijuegos.Descargar();
    pruebaModelos.Descargar();
    DescargarModelosEscenariosRetro3D();
    DescargarTexturasTematicasMinijuegos();
}
