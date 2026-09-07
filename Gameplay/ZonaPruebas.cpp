#include "Gameplay/ZonaPruebas.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"


static const char* NombreModoPrueba(
    ModoZonaPruebas modo
)
{
    switch (modo)
    {
        case PRUEBA_ZONA_PRINCIPAL: return "1 - ZONA PRINCIPAL";
        case PRUEBA_COLOR_SEGURO: return "2 - COLOR SEGURO";
        case PRUEBA_PELOTAS_EMPUJON: return "3 - PELOTAS / EMPUJONES";
        case PRUEBA_MODELOS: return "4 - PRUEBA DE MODELOS";
        case PRUEBA_TRONCO_COORDINADO: return "5 - TRONCO COORDINADO";
        case PRUEBA_FABRICA_67: return "6 - FABRICA 67";
        case PRUEBA_TABLERO: return "7 - TABLERO";
        case PRUEBA_ISLA_FUEGO: return "8 - ISLA BAJO FUEGO";
        case PRUEBA_CAPITAN_MANDA: return "9 - CAPITAN MANDA";
        case PRUEBA_BARRA_GIRATORIA: return "0 - BARRA GIRATORIA";
        case PRUEBA_NUCLEOS_ENERGIA: return "F10 - NUCLEOS DE ENERGIA";
        case PRUEBA_REFUGIO_PINCHOS: return "F11 - REFUGIO DE PINCHOS";
        case PRUEBA_MIRADAS_CRUZADAS: return "F12 - MIRADAS CRUZADAS";
        case PRUEBA_MUROS_LOCOS: return "F9 - MUROS LOCOS";
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
        "SALTO EN EL AIRE: GOLPE AL SUELO   E/SHIFT/B: GOLPEAR",
        25,
        100,
        18,
        DARKGRAY
    );
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

    for (int i = 0; i < MAX_PARTICULAS_TIERRA; i++)
    {
        particulas[i].activa = false;
    }

    ConfigurarZonaPrincipal(*this);

    minijuegoColor.Inicializar();
    minijuegoPelotas.Inicializar();
    minijuegoTronco.Inicializar();
    minijuego67.Inicializar();
    minijuegoIslaFuego.Inicializar();
    minijuegoCapitanManda.Inicializar();
    minijuegoBarraGiratoria.Inicializar();
    minijuegoNucleosEnergia.Inicializar();
    minijuegoRefugioPinchos.Inicializar();
    minijuegoMiradasCruzadas.Inicializar();
    minijuegoMurosLocos.Inicializar();

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
            break;

        case PRUEBA_COLOR_SEGURO:
            minijuegoColor.Inicializar();
            minijuegoColor.ConfigurarJugadores(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_PELOTAS_EMPUJON:
            minijuegoPelotas.Inicializar();
            minijuegoPelotas.ConfigurarJugadores(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_MODELOS:
            pruebaModelos.Inicializar();
            break;

        case PRUEBA_TRONCO_COORDINADO:
            minijuegoTronco.Inicializar();
            minijuegoTronco.ConfigurarJugadores(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_FABRICA_67:
            minijuego67.Inicializar();
            break;

        case PRUEBA_TABLERO:
            prototipoTablero.Reiniciar();
            break;

        case PRUEBA_ISLA_FUEGO:
            minijuegoIslaFuego.Inicializar();
            minijuegoIslaFuego.ConfigurarJugadores(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_CAPITAN_MANDA:
            minijuegoCapitanManda.Reiniciar(participantes);
            break;

        case PRUEBA_BARRA_GIRATORIA:
            minijuegoBarraGiratoria.Inicializar();
            minijuegoBarraGiratoria.ConfigurarJugadores(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_NUCLEOS_ENERGIA:
            minijuegoNucleosEnergia.Inicializar();
            minijuegoNucleosEnergia.ConfigurarJugadores(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Inicializar();
            break;

        case PRUEBA_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Inicializar();
            break;

        case PRUEBA_MUROS_LOCOS:
            minijuegoMurosLocos.Inicializar();
            minijuegoMurosLocos.ConfigurarJugadores(
                jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;
    }

    if (modoActual != PRUEBA_MODELOS)
    {
        ReiniciarJugadores();
    }

    if (modoActual == PRUEBA_TRONCO_COORDINADO)
    {
        minijuegoTronco.Reiniciar(
            participantes,
            MAX_JUGADORES_PRUEBA
        );
    }
    else if (modoActual == PRUEBA_FABRICA_67)
    {
        minijuego67.Reiniciar(
            participantes,
            MAX_JUGADORES_PRUEBA
        );
    }
    else if (modoActual == PRUEBA_REFUGIO_PINCHOS)
    {
        minijuegoRefugioPinchos.Reiniciar(
            jugadores,
            participantes,
            MAX_JUGADORES_PRUEBA
        );
    }
    else if (modoActual == PRUEBA_MIRADAS_CRUZADAS)
    {
        minijuegoMiradasCruzadas.Reiniciar(participantes);
    }
}


void ZonaPruebas::ReiniciarJugador(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= MAX_JUGADORES_PRUEBA
    )
    {
        return;
    }

    ReiniciarJugadorPrueba(jugadores[indice]);
}


void ZonaPruebas::ReiniciarJugadores()
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        ReiniciarJugador(i);
    }
}


static void ReiniciarModoActual(
    ZonaPruebas& zona
)
{
    switch (zona.modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            ConfigurarJugadoresPrincipal(zona);
            break;

        case PRUEBA_COLOR_SEGURO:
            zona.minijuegoColor.Reiniciar(
                zona.jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_PELOTAS_EMPUJON:
            zona.minijuegoPelotas.Reiniciar(
                zona.jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_MODELOS:
            zona.pruebaModelos.Reiniciar();
            break;

        case PRUEBA_TRONCO_COORDINADO:
            zona.minijuegoTronco.Reiniciar(
                zona.participantes,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_FABRICA_67:
            zona.minijuego67.Reiniciar(
                zona.participantes,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_TABLERO:
            zona.prototipoTablero.Reiniciar();
            break;

        case PRUEBA_ISLA_FUEGO:
            zona.minijuegoIslaFuego.Reiniciar(
                zona.jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_CAPITAN_MANDA:
            zona.minijuegoCapitanManda.Reiniciar(zona.participantes);
            break;

        case PRUEBA_BARRA_GIRATORIA:
            zona.minijuegoBarraGiratoria.Reiniciar(
                zona.jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_NUCLEOS_ENERGIA:
            zona.minijuegoNucleosEnergia.Reiniciar(
                zona.jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_REFUGIO_PINCHOS:
            zona.minijuegoRefugioPinchos.Reiniciar(
                zona.jugadores,
                zona.participantes,
                MAX_JUGADORES_PRUEBA
            );
            break;

        case PRUEBA_MIRADAS_CRUZADAS:
            zona.minijuegoMiradasCruzadas.Reiniciar(zona.participantes);
            break;

        case PRUEBA_MUROS_LOCOS:
            zona.minijuegoMurosLocos.Reiniciar(
                zona.jugadores,
                MAX_JUGADORES_PRUEBA
            );
            break;
    }
}


void ZonaPruebas::Actualizar(
    float deltaTime
)
{
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
        if (IsKeyPressed(KEY_TWO)) { CambiarModo(PRUEBA_COLOR_SEGURO); return; }
        if (IsKeyPressed(KEY_THREE)) { CambiarModo(PRUEBA_PELOTAS_EMPUJON); return; }
        if (IsKeyPressed(KEY_FOUR)) { CambiarModo(PRUEBA_MODELOS); return; }
        if (IsKeyPressed(KEY_FIVE)) { CambiarModo(PRUEBA_TRONCO_COORDINADO); return; }
        if (IsKeyPressed(KEY_SIX)) { CambiarModo(PRUEBA_FABRICA_67); return; }
        if (IsKeyPressed(KEY_SEVEN)) { CambiarModo(PRUEBA_TABLERO); return; }
        if (IsKeyPressed(KEY_EIGHT)) { CambiarModo(PRUEBA_ISLA_FUEGO); return; }
        if (IsKeyPressed(KEY_NINE)) { CambiarModo(PRUEBA_CAPITAN_MANDA); return; }
        if (IsKeyPressed(KEY_ZERO)) { CambiarModo(PRUEBA_BARRA_GIRATORIA); return; }
        if (IsKeyPressed(KEY_F9)) { CambiarModo(PRUEBA_MUROS_LOCOS); return; }
        if (IsKeyPressed(KEY_F10)) { CambiarModo(PRUEBA_NUCLEOS_ENERGIA); return; }
        if (IsKeyPressed(KEY_F11)) { CambiarModo(PRUEBA_REFUGIO_PINCHOS); return; }
        if (IsKeyPressed(KEY_F12)) { CambiarModo(PRUEBA_MIRADAS_CRUZADAS); return; }
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

            if (participantes[i].conectado)
            {
                ReiniciarJugadorPrueba(jugadores[i]);
            }
            else
            {
                jugadores[i].velocidad = {};
                jugadores[i].empuje = {};
                jugadores[i].cayendo = false;
                jugadores[i].enSuelo = false;
            }
        }
    }

    switch (modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            ActualizarZonaPrincipal(*this, deltaTime);
            break;

        case PRUEBA_COLOR_SEGURO:
            minijuegoColor.Actualizar(
                deltaTime,
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA
            );
            break;

        case PRUEBA_PELOTAS_EMPUJON:
            minijuegoPelotas.Actualizar(
                deltaTime,
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA
            );
            break;

        case PRUEBA_MODELOS:
            pruebaModelos.Actualizar(deltaTime);
            break;

        case PRUEBA_TRONCO_COORDINADO:
            minijuegoTronco.Actualizar(
                deltaTime,
                MAX_JUGADORES_PRUEBA,
                participantes
            );
            break;

        case PRUEBA_FABRICA_67:
            minijuego67.Actualizar(
                deltaTime,
                MAX_JUGADORES_PRUEBA,
                participantes
            );
            break;

        case PRUEBA_TABLERO:
            prototipoTablero.Actualizar(deltaTime);
            break;

        case PRUEBA_ISLA_FUEGO:
            minijuegoIslaFuego.Actualizar(
                deltaTime,
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA
            );
            break;

        case PRUEBA_CAPITAN_MANDA:
            minijuegoCapitanManda.Actualizar(
                deltaTime,
                participantes
            );
            break;

        case PRUEBA_BARRA_GIRATORIA:
            minijuegoBarraGiratoria.Actualizar(
                deltaTime,
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA
            );
            break;

        case PRUEBA_NUCLEOS_ENERGIA:
            minijuegoNucleosEnergia.Actualizar(
                deltaTime,
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA,
                audio
            );
            break;

        case PRUEBA_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Actualizar(
                deltaTime,
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA
            );
            break;

        case PRUEBA_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Actualizar(
                deltaTime,
                participantes
            );
            break;

        case PRUEBA_MUROS_LOCOS:
            minijuegoMurosLocos.Actualizar(
                deltaTime,
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA
            );
            break;
    }
}


void ZonaPruebas::Dibujar() const
{
    switch (modoActual)
    {
        case PRUEBA_ZONA_PRINCIPAL:
            DibujarZonaPrincipal(*this);
            break;

        case PRUEBA_COLOR_SEGURO:
            minijuegoColor.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA,
                mostrarDebug
            );
            break;

        case PRUEBA_PELOTAS_EMPUJON:
            minijuegoPelotas.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                mostrarDebug
            );
            break;

        case PRUEBA_MODELOS:
            pruebaModelos.Dibujar();
            break;

        case PRUEBA_TRONCO_COORDINADO:
            minijuegoTronco.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes
            );
            break;

        case PRUEBA_FABRICA_67:
            minijuego67.Dibujar(
                MAX_JUGADORES_PRUEBA,
                participantes
            );
            break;

        case PRUEBA_TABLERO:
            prototipoTablero.Dibujar(mostrarDebug);
            break;

        case PRUEBA_ISLA_FUEGO:
            minijuegoIslaFuego.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA,
                mostrarDebug
            );
            break;

        case PRUEBA_CAPITAN_MANDA:
            minijuegoCapitanManda.Dibujar(participantes);
            break;

        case PRUEBA_BARRA_GIRATORIA:
            minijuegoBarraGiratoria.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA,
                mostrarDebug
            );
            break;

        case PRUEBA_NUCLEOS_ENERGIA:
            minijuegoNucleosEnergia.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                mostrarDebug
            );
            break;

        case PRUEBA_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                mostrarDebug
            );
            break;

        case PRUEBA_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Dibujar(participantes);
            break;

        case PRUEBA_MUROS_LOCOS:
            minijuegoMurosLocos.Dibujar(
                jugadores,
                MAX_JUGADORES_PRUEBA,
                participantes,
                particulas,
                MAX_PARTICULAS_TIERRA,
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
        GetScreenHeight() - 158,
        GetScreenWidth() - 36,
        136,
        Fade(RAYWHITE, 0.86f)
    );

    DrawText(
        "1 PRINCIPAL  2 COLOR  3 PELOTAS  4 MODELOS  5 TRONCO  6 FABRICA  7 TABLERO  8 ISLA  9 CAPITAN  0 BARRA",
        30,
        GetScreenHeight() - 143,
        14,
        BLACK
    );

    DrawText(
        "F9 MUROS   F10 NUCLEOS   F11 PINCHOS   F12 MIRADAS",
        30,
        GetScreenHeight() - 116,
        15,
        DARKBLUE
    );

    DrawText(
        TextFormat("ACTUAL: %s", NombreModoPrueba(modoActual)),
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


void ZonaPruebas::Descargar()
{
    minijuego67.Descargar();
    pruebaModelos.Descargar();
}
