#include "Minigames/MinijuegoPasarelasVacio.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>


static const float DURACION_PREPARACION_PASARELAS = 2.5f;
static const float DURACION_CARRERA_PASARELAS = 35.0f;
static const float Z_INICIO_PASARELAS = 5.6f;
static const float Z_META_PASARELAS = -30.8f;
// Segundos de pisada acumulada (de cualquier jugador) que aguanta cada plataforma.
static const float RESISTENCIA_PLATAFORMA_PASARELAS = 1.5f;


static float LimitarPasarelas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float CalcularProgresoPasarelas(const JugadorPrueba& jugador)
{
    return LimitarPasarelas(
        Z_INICIO_PASARELAS - jugador.posicion.z,
        0.0f,
        Z_INICIO_PASARELAS - Z_META_PASARELAS
    );
}


static bool JugadorSobrePlataformaPasarelas(
    const JugadorPrueba& jugador,
    const BloquePrueba& plataforma
)
{
    if (!plataforma.activaColision || jugador.cayendo)
    {
        return false;
    }

    BoundingBox caja = CrearHitboxBloquePrueba(plataforma);
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;

    return
        jugador.posicion.x >= caja.min.x - 0.08f &&
        jugador.posicion.x <= caja.max.x + 0.08f &&
        jugador.posicion.z >= caja.min.z - 0.08f &&
        jugador.posicion.z <= caja.max.z + 0.08f &&
        std::fabs(pies - caja.max.y) <= 0.16f;
}


static int BuscarPlataformaSoportePasarelas(
    const MinijuegoPasarelasVacio& minijuego,
    const JugadorPrueba& jugador
)
{
    for (int i = 0; i < minijuego.cantidadPlataformas; i++)
    {
        if (JugadorSobrePlataformaPasarelas(jugador, minijuego.plataformas[i]))
        {
            return i;
        }
    }

    return -1;
}


static int PuntuacionPasarelas(
    const MinijuegoPasarelasVacio& minijuego,
    int indiceJugador
)
{
    const EstadoJugadorPasarelasVacio& estado =
        minijuego.estadosJugadores[indiceJugador];

    int puntuacion = (int)std::lround(estado.progresoMaximo * 1000.0f);

    if (estado.llegoMeta)
    {
        puntuacion += 100000;
    }
    else if (!estado.eliminado)
    {
        puntuacion += 100;
    }

    return puntuacion;
}


static int ContarCorredoresActivos(
    const MinijuegoPasarelasVacio& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            !minijuego.estadosJugadores[i].llegoMeta
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void FinalizarPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    bool empateForzado
)
{
    int cantidadPrimeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int posicion = 1;
        int puntuacion = PuntuacionPasarelas(minijuego, i);

        if (!empateForzado)
        {
            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    minijuego.resultado.participantes[j].participo &&
                    PuntuacionPasarelas(minijuego, j) > puntuacion
                )
                {
                    posicion++;
                }
            }
        }

        minijuego.estadosJugadores[i].posicionFinal = posicion;
        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego = puntuacion;

        if (posicion == 1)
        {
            cantidadPrimeros++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadPrimeros == 1
            ? DESENLACE_CON_GANADOR
            : DESENLACE_EMPATE;
    minijuego.fase = FASE_PASARELAS_TERMINADO;
}


static InputMinijuegoParticipante CrearEntradaBotPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    int indiceJugador,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorPasarelasVacio& estado =
        minijuego.estadosJugadores[indiceJugador];

    if (estado.indiceObjetivoBot >= minijuego.cantidadPlataformas)
    {
        estado.indiceObjetivoBot = minijuego.cantidadPlataformas - 1;
    }

    const BloquePrueba& objetivo =
        minijuego.plataformas[estado.indiceObjetivoBot];

    int soporte = BuscarPlataformaSoportePasarelas(minijuego, jugador);

    // Al llegar a una plataforma nueva: distancia de salto propia (error de
    // timing) y 15% de probabilidad de dudar entre 0.2 y 0.6 s.
    if (soporte >= 0 && jugador.enSuelo && soporte != estado.ultimoSoporteBot)
    {
        bool primera = estado.ultimoSoporteBot < 0;
        estado.ultimoSoporteBot = soporte;
        estado.desfaseXBot = GetRandomValue(-130, 130) / 100.0f;
        estado.distanciaSaltoBot = GetRandomValue(50, 95) / 100.0f;
        if (primera)
            estado.pausaBot = GetRandomValue(0, 450) / 1000.0f;
        else if (GetRandomValue(1, 100) <= 15)
            estado.pausaBot = GetRandomValue(200, 600) / 1000.0f;
    }

    if (estado.pausaBot > 0.0f)
    {
        estado.pausaBot -= deltaTime;
        return InputMinijuegoParticipante{};
    }

    if (jugador.posicion.x < objetivo.posicion.x + estado.desfaseXBot - 0.22f)
        entrada.derecha = true;
    else if (jugador.posicion.x > objetivo.posicion.x + estado.desfaseXBot + 0.22f)
        entrada.izquierda = true;

    entrada.adelante = true;

    if (soporte >= 0 && jugador.enSuelo)
    {
        const BloquePrueba& plataforma = minijuego.plataformas[soporte];
        float bordeDelantero = plataforma.posicion.z - plataforma.tamano.z * 0.5f;

        if (jugador.posicion.z <= bordeDelantero + estado.distanciaSaltoBot)
        {
            entrada.saltar = true;
        }

        if (
            soporte >= estado.indiceObjetivoBot &&
            estado.indiceObjetivoBot < minijuego.cantidadPlataformas - 1
        )
        {
            estado.indiceObjetivoBot = soporte + 1;
        }
    }

    return entrada;
}


static void ActualizarDerrumbePlataformas(
    MinijuegoPasarelasVacio& minijuego,
    const JugadorPrueba jugadores[],
    float deltaTime
)
{
    for (int plataforma = 1; plataforma < minijuego.cantidadPlataformas - 1; plataforma++)
    {
        EstadoPlataformaPasarelas& estado =
            minijuego.estadosPlataformas[plataforma];
        BloquePrueba& bloque = minijuego.plataformas[plataforma];

        // El presupuesto de estabilidad solo se gasta mientras alguien la
        // pisa: llegar tarde a una plataforma ya no la encuentra rota por
        // los pasos de otros, pero quedarse parado o dudar si la agota.
        estado.ocupada = false;

        if (!estado.cayendo)
        {
            for (int jugador = 0; jugador < MAX_PARTICIPANTES; jugador++)
            {
                if (
                    minijuego.resultado.participantes[jugador].participo &&
                    !minijuego.estadosJugadores[jugador].eliminado &&
                    JugadorSobrePlataformaPasarelas(jugadores[jugador], bloque)
                )
                {
                    estado.ocupada = true;
                    break;
                }
            }
        }

        if (estado.ocupada)
        {
            estado.activada = true;
            estado.tiempoDerrumbe -= deltaTime;

            if (estado.tiempoDerrumbe <= 0.0f)
            {
                estado.tiempoDerrumbe = 0.0f;
                estado.cayendo = true;
                estado.velocidadCaida = 0.8f;
                bloque.activaColision = false;
            }
        }

        if (estado.cayendo)
        {
            estado.velocidadCaida += 10.0f * deltaTime;
            bloque.posicion.y -= estado.velocidadCaida * deltaTime;
        }
    }
}


void MinijuegoPasarelasVacio::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_PLATAFORMAS_PASARELAS; i++)
    {
        estadosPlataformas[i] = {};
        estadosPlataformas[i].tiempoDerrumbe = RESISTENCIA_PLATAFORMA_PASARELAS;
        plataformas[i] = {};
    }

    cantidadPlataformas = 0;

    const Vector3 posiciones[MAX_PLATAFORMAS_PASARELAS] =
    {
        {  0.0f, -0.30f,   4.8f },
        { -0.8f, -0.30f,  -0.2f },
        {  1.0f, -0.30f,  -4.7f },
        { -1.3f, -0.30f,  -9.2f },
        {  1.4f, -0.30f, -13.7f },
        { -1.0f, -0.30f, -18.2f },
        {  1.1f, -0.30f, -22.7f },
        {  0.0f, -0.30f, -27.2f },
        {  0.0f, -0.30f, -32.0f }
    };

    const Vector3 tamanos[MAX_PLATAFORMAS_PASARELAS] =
    {
        { 8.4f, 0.60f, 4.6f },
        { 6.8f, 0.60f, 3.2f },
        { 6.2f, 0.60f, 3.2f },
        { 5.8f, 0.60f, 3.1f },
        { 5.4f, 0.60f, 3.0f },
        { 6.0f, 0.60f, 3.0f },
        { 5.5f, 0.60f, 3.0f },
        { 7.0f, 0.60f, 3.3f },
        { 8.4f, 0.60f, 4.2f }
    };

    for (int i = 0; i < MAX_PLATAFORMAS_PASARELAS; i++)
    {
        AgregarBloquePrueba(
            plataformas,
            cantidadPlataformas,
            MAX_PLATAFORMAS_PASARELAS,
            posiciones[i],
            tamanos[i],
            i == MAX_PLATAFORMAS_PASARELAS - 1
                ? Color{ 93, 218, 177, 255 }
                : Color{ 98, 118, 190, 255 }
        );
    }

    camara.position = { 10.5f, 27.0f, 26.6f };
    camara.target = { 0.0f, -0.2f, 4.1f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 19.0f;
    camara.projection = CAMERA_ORTHOGRAPHIC;

    fase = FASE_PASARELAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PASARELAS;
    tiempoRestante = DURACION_CARRERA_PASARELAS;
    tiempoAnimacion = 0.0f;
}


void MinijuegoPasarelasVacio::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    const float posicionesX[MAX_PARTICIPANTES] =
    {
        -2.7f, -0.9f, 0.9f, 2.7f
    };

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            { posicionesX[i], 0.70f, Z_INICIO_PASARELAS }
        );
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
    }
}


// Camara ortografica que sigue al grupo: se acerca cuando los jugadores van
// juntos y se aleja lo justo cuando se separan, para que nunca queden
// diminutos ni salgan de pantalla.
static void ActualizarCamaraPasarelas(
    MinijuegoPasarelasVacio& minijuego,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    float deltaTime
)
{
    float zMinima = 1000.0f;
    float zMaxima = -1000.0f;
    int contados = 0;

    for (int i = 0; i < cantidadMaxima && i < MAX_PARTICIPANTES; i++)
    {
        if (
            !minijuego.resultado.participantes[i].participo ||
            minijuego.estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        float z = jugadores[i].posicion.z;
        if (z < zMinima) zMinima = z;
        if (z > zMaxima) zMaxima = z;
        contados++;
    }

    if (contados == 0)
    {
        return;
    }

    float dispersion = zMaxima - zMinima;
    float zFoco = (zMinima + zMaxima) * 0.5f - 1.5f;
    float fovDeseado = 19.0f + dispersion * 0.55f;
    if (fovDeseado > 34.0f) fovDeseado = 34.0f;

    float suavizado = 1.0f - std::exp(-4.0f * deltaTime);

    float objetivoAnterior = minijuego.camara.target.z;
    float objetivoNuevo = objetivoAnterior + (zFoco - objetivoAnterior) * suavizado;

    minijuego.camara.target = { 0.0f, -0.2f, objetivoNuevo };
    minijuego.camara.position =
    {
        10.5f,
        27.0f,
        objetivoNuevo + 22.5f
    };
    minijuego.camara.fovy +=
        (fovDeseado - minijuego.camara.fovy) * suavizado;
}


void MinijuegoPasarelasVacio::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    tiempoAnimacion += deltaTime;
    ActualizarCamaraPasarelas(*this, jugadores, cantidadMaxima, deltaTime);

    if (fase == FASE_PASARELAS_TERMINADO)
    {
        return;
    }

    if (fase == FASE_PASARELAS_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        for (int i = 0; i < cantidadMaxima; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PASARELAS_CARRERA;
        }

        return;
    }

    tiempoRestante -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;

    ActualizarDerrumbePlataformas(*this, jugadores, deltaTime);

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorPasarelasVacio& estado = estadosJugadores[i];
        JugadorPrueba& jugador = jugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estado.eliminado ||
            estado.llegoMeta
        )
        {
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotPasarelas(*this, i, jugador, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        entrada.golpear = false;

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            plataformas,
            cantidadPlataformas,
            particulas,
            cantidadParticulas,
            true,
            false,
            deltaTime
        );

        float progreso = CalcularProgresoPasarelas(jugador);
        if (progreso > estado.progresoMaximo)
        {
            estado.progresoMaximo = progreso;
        }

        if (
            jugador.posicion.z <= Z_META_PASARELAS &&
            jugador.enSuelo &&
            JugadorSobrePlataformaPasarelas(
                jugador,
                plataformas[cantidadPlataformas - 1]
            )
        )
        {
            estado.llegoMeta = true;
            FinalizarPasarelas(*this, false);
            return;
        }

        if (jugador.posicion.y < -3.2f)
        {
            jugador.cayendo = true;
            estado.eliminado = true;
        }
    }

    Participante participantesFisica[MAX_PARTICIPANTES];
    for (int i = 0; i < limite; i++)
    {
        participantesFisica[i] = participantes[i];
        participantesFisica[i].conectado = true;
    }

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantesFisica,
        limite
    );

    if (ContarCorredoresActivos(*this) <= 0)
    {
        FinalizarPasarelas(*this, true);
    }
    else if (tiempoRestante <= 0.0f)
    {
        FinalizarPasarelas(*this, false);
    }
}


static Color ColorPlataformaPasarelas(
    const MinijuegoPasarelasVacio& minijuego,
    int indice
)
{
    if (indice == minijuego.cantidadPlataformas - 1)
    {
        return Color{ 93, 218, 177, 255 };
    }

    const EstadoPlataformaPasarelas& estado =
        minijuego.estadosPlataformas[indice];

    if (estado.cayendo)
    {
        return Color{ 87, 75, 112, 255 };
    }

    Color base = indice % 2 == 0
        ? Color{ 91, 126, 213, 255 }
        : Color{ 124, 99, 205, 255 };

    // El color avisa cuanto aguanta: azul entera, naranja a medias, roja casi rota.
    float gastado = 1.0f - LimitarPasarelas(
        estado.tiempoDerrumbe / RESISTENCIA_PLATAFORMA_PASARELAS,
        0.0f,
        1.0f
    );
    Color color = gastado < 0.5f
        ? ColorLerp(base, Color{ 255, 168, 74, 255 }, gastado * 2.0f)
        : ColorLerp(Color{ 255, 168, 74, 255 }, Color{ 244, 76, 66, 255 }, (gastado - 0.5f) * 2.0f);

    if (estado.ocupada && std::sin(minijuego.tiempoAnimacion * (10.0f + gastado * 16.0f)) > 0.0f)
    {
        color = ColorLerp(color, WHITE, 0.28f);
    }

    return color;
}


void MinijuegoPasarelasVacio::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    ClearBackground(Color{ 11, 13, 31, 255 });
    BeginMode3D(camara);

    DrawPlane(
        { 0.0f, -8.0f, -13.0f },
        { 90.0f, 90.0f },
        Color{ 7, 8, 20, 255 }
    );

    for (int i = 0; i < 46; i++)
    {
        float x = -22.0f + (float)((i * 17) % 44);
        float y = 2.0f + (float)((i * 11) % 16);
        float z = 8.0f - (float)((i * 23) % 52);
        DrawSphere({ x, y, z }, 0.045f + 0.018f * (float)(i % 3), Fade(RAYWHITE, 0.72f));
    }

    for (int i = 0; i < cantidadPlataformas; i++)
    {
        const BloquePrueba& plataforma = plataformas[i];
        Color color = ColorPlataformaPasarelas(*this, i);

        DrawCube(
            plataforma.posicion,
            plataforma.tamano.x,
            plataforma.tamano.y,
            plataforma.tamano.z,
            color
        );
        DrawCubeWires(
            plataforma.posicion,
            plataforma.tamano.x,
            plataforma.tamano.y,
            plataforma.tamano.z,
            Fade(RAYWHITE, 0.70f)
        );

        DrawCube(
            {
                plataforma.posicion.x,
                plataforma.posicion.y + plataforma.tamano.y * 0.52f,
                plataforma.posicion.z
            },
            plataforma.tamano.x * 0.86f,
            0.035f,
            plataforma.tamano.z * 0.80f,
            Fade(RAYWHITE, 0.22f)
        );

        if (mostrarDebug && plataforma.activaColision)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(plataforma), YELLOW);
        }
    }

    Vector3 meta = plataformas[cantidadPlataformas - 1].posicion;
    DrawCube({ meta.x - 3.2f, 1.55f, meta.z - 0.9f }, 0.35f, 3.7f, 0.35f, GOLD);
    DrawCube({ meta.x + 3.2f, 1.55f, meta.z - 0.9f }, 0.35f, 3.7f, 0.35f, GOLD);
    DrawCube({ meta.x, 3.25f, meta.z - 0.9f }, 6.75f, 0.32f, 0.35f, GOLD);

    DibujarParticulasTierra(particulas, cantidadParticulas);

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        // Un mando desconectado queda temporalmente a cargo de la IA.
        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(jugadores[i], participanteVisual);

        if (mostrarDebug && !jugadores[i].cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    EndMode3D();

    DrawRectangle(18, 16, 620, 108, Fade(BLACK, 0.78f));
    DrawText("PASARELAS DEL VACIO", 32, 28, 30, Color{ 151, 202, 255, 255 });
    DrawText("CORRE A LA META: LAS PLATAFORMAS SE ROMPEN SI LAS PISAS MUCHO", 32, 68, 16, RAYWHITE);

    const char* ayuda = "MOVER: WASD / FLECHAS / STICK   SALTAR: ESPACIO / ENTER / A   NO TE QUEDES QUIETO: LA PLATAFORMA SE PONE ROJA Y CAE";
    DrawRectangle(0, GetScreenHeight() - 38, GetScreenWidth(), 38, Fade(BLACK, 0.78f));
    DrawText(ayuda, GetScreenWidth() / 2 - MeasureText(ayuda, 16) / 2, GetScreenHeight() - 29, 16, RAYWHITE);

    if (fase == FASE_PASARELAS_CARRERA)
    {
        DrawText(
            TextFormat("TIEMPO %.1f", tiempoRestante),
            32,
            96,
            18,
            tiempoRestante <= 5.0f ? RED : GOLD
        );
    }

    if (fase == FASE_PASARELAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 92) / 2,
            GetScreenHeight() / 2 - 56,
            92,
            GOLD
        );
    }
    else if (fase == FASE_PASARELAS_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 310,
            GetScreenHeight() / 2 - 118,
            620,
            236,
            Fade(BLACK, 0.91f)
        );

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(
            resultado,
            ganadores,
            MAX_PARTICIPANTES
        );

        const char* titulo = resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE EN LA CARRERA"
            : TextFormat(
                "GANA J%d",
                cantidadGanadores > 0
                    ? participantes[ganadores[0]].numeroJugador
                    : 0
            );

        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 82,
            34,
            GOLD
        );

        DrawText(
            TextoReinicioMinijuego(),
            GetScreenWidth() / 2 - MeasureText(TextoReinicioMinijuego(), 21) / 2,
            GetScreenHeight() / 2 + 62,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoPasarelasVacio::ObtenerResultado() const
{
    return resultado;
}
