#include "Minigames/MinijuegoTormentaMagnetica.h"

#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_MAGNETICA = 3.0f;
static const float ESCALA_DIFICULTAD_MAGNETICA = 42.0f;
static const float MEDIO_LADO_ARENA_MAGNETICA = 5.75f;
static const float RADIO_NUCLEO_MAGNETICO = 0.72f;


static float Limitar01Magnetica(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float ProgresoDificultadMagnetica(
    const MinijuegoTormentaMagnetica& minijuego
)
{
    return Limitar01Magnetica(
        minijuego.tiempoJugado / ESCALA_DIFICULTAD_MAGNETICA
    );
}


static bool JugadorSobreArenaMagnetica(const JugadorPrueba& jugador)
{
    float margen = jugador.tamano.x * 0.18f;
    return
        std::fabs(jugador.posicion.x) <= MEDIO_LADO_ARENA_MAGNETICA - margen &&
        std::fabs(jugador.posicion.z) <= MEDIO_LADO_ARENA_MAGNETICA - margen;
}


static int ContarVivosMagnetica(const MinijuegoTormentaMagnetica& minijuego)
{
    int vivos = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            vivos++;
        }
    }
    return vivos;
}


static Vector3 ElegirNuevaPosicionNucleoMagnetico(Vector3 anterior)
{
    const Vector3 posiciones[] =
    {
        { 0.0f, 0.85f, 0.0f },
        { -3.25f, 0.85f, -2.80f }, { 3.25f, 0.85f, -2.80f },
        { -3.25f, 0.85f, 2.80f }, { 3.25f, 0.85f, 2.80f },
        { 0.0f, 0.85f, -3.75f }, { 0.0f, 0.85f, 3.75f }
    };

    const int cantidad = sizeof(posiciones) / sizeof(posiciones[0]);
    int elegido = GetRandomValue(0, cantidad - 1);

    for (int intento = 0; intento < 12; intento++)
    {
        int candidato = GetRandomValue(0, cantidad - 1);
        float dx = posiciones[candidato].x - anterior.x;
        float dz = posiciones[candidato].z - anterior.z;

        if (dx * dx + dz * dz > 2.0f)
        {
            elegido = candidato;
            break;
        }
    }

    return posiciones[elegido];
}


static void CambiarCampoMagnetico(MinijuegoTormentaMagnetica& minijuego)
{
    minijuego.cambiosCampo++;
    minijuego.campoAtrae = !minijuego.campoAtrae;
    minijuego.posicionNucleo = ElegirNuevaPosicionNucleoMagnetico(
        minijuego.posicionNucleo
    );

    float progreso = ProgresoDificultadMagnetica(minijuego);

    // Empieza bastante rapido y termina cambiando casi cuatro veces mas
    // seguido. Ya no hay un techo de tiempo de partida.
    minijuego.tiempoHastaCambioCampo = 3.05f - progreso * 2.25f;
    if (minijuego.tiempoHastaCambioCampo < 0.80f)
        minijuego.tiempoHastaCambioCampo = 0.80f;
}


static void AplicarCampoMagneticoAJugador(
    const MinijuegoTormentaMagnetica& minijuego,
    JugadorPrueba& jugador,
    float deltaTime
)
{
    float dx = minijuego.posicionNucleo.x - jugador.posicion.x;
    float dz = minijuego.posicionNucleo.z - jugador.posicion.z;
    float distancia = std::sqrt(dx * dx + dz * dz);
    if (distancia < 0.18f) distancia = 0.18f;

    float nx = dx / distancia;
    float nz = dz / distancia;
    if (!minijuego.campoAtrae)
    {
        nx *= -1.0f;
        nz *= -1.0f;
    }

    float cercania = 1.0f - distancia / 9.0f;
    cercania = Limitar01Magnetica(cercania);
    float progreso = ProgresoDificultadMagnetica(minijuego);

    float fuerzaBase = minijuego.campoAtrae ? 22.0f : 27.0f;
    float fuerza =
        fuerzaBase *
        (1.0f + progreso * 1.05f) *
        (0.52f + cercania * 0.68f);

    jugador.empuje.x += nx * fuerza * deltaTime;
    jugador.empuje.z += nz * fuerza * deltaTime;
}


static int BuscarSlotPincho(MinijuegoTormentaMagnetica& minijuego)
{
    for (int i = 0; i < MAX_PINCHOS_TORMENTA_MAGNETICA; i++)
    {
        if (!minijuego.pinchos[i].activo) return i;
    }

    int masViejo = 0;
    float menorVida = minijuego.pinchos[0].tiempoVida;
    for (int i = 1; i < MAX_PINCHOS_TORMENTA_MAGNETICA; i++)
    {
        if (minijuego.pinchos[i].tiempoVida < menorVida)
        {
            menorVida = minijuego.pinchos[i].tiempoVida;
            masViejo = i;
        }
    }
    return masViejo;
}


static void CrearPinchoMagnetico(MinijuegoTormentaMagnetica& minijuego)
{
    int indice = BuscarSlotPincho(minijuego);
    PinchoTormentaMagnetica& pincho = minijuego.pinchos[indice];
    float progreso = ProgresoDificultadMagnetica(minijuego);

    pincho = {};
    pincho.activo = true;
    pincho.posicion =
    {
        (float)GetRandomValue(-500, 500) / 100.0f,
        (float)GetRandomValue(780, 980) / 100.0f,
        (float)GetRandomValue(-500, 500) / 100.0f
    };
    pincho.velocidad =
    {
        0.0f,
        -(7.0f + progreso * 6.0f + (float)GetRandomValue(0, 20) / 10.0f),
        0.0f
    };
    pincho.tiempoVida = 3.0f;
}


static bool PinchoTocaJugador(
    const PinchoTormentaMagnetica& pincho,
    const JugadorPrueba& jugador
)
{
    BoundingBox caja = CrearHitboxJugadorPrueba(jugador);
    const float radio = 0.38f;

    return
        pincho.posicion.x + radio >= caja.min.x &&
        pincho.posicion.x - radio <= caja.max.x &&
        pincho.posicion.y + 0.55f >= caja.min.y &&
        pincho.posicion.y - 0.55f <= caja.max.y &&
        pincho.posicion.z + radio >= caja.min.z &&
        pincho.posicion.z - radio <= caja.max.z;
}


static void ActualizarPinchosMagneticos(
    MinijuegoTormentaMagnetica& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    float progreso = ProgresoDificultadMagnetica(minijuego);
    minijuego.tiempoHastaPincho -= deltaTime;

    if (minijuego.tiempoHastaPincho <= 0.0f)
    {
        CrearPinchoMagnetico(minijuego);
        minijuego.tiempoHastaPincho = 1.25f - progreso * 0.88f;
        if (minijuego.tiempoHastaPincho < 0.34f)
            minijuego.tiempoHastaPincho = 0.34f;
    }

    for (int p = 0; p < MAX_PINCHOS_TORMENTA_MAGNETICA; p++)
    {
        PinchoTormentaMagnetica& pincho = minijuego.pinchos[p];
        if (!pincho.activo) continue;

        pincho.tiempoVida -= deltaTime;
        pincho.posicion.x += pincho.velocidad.x * deltaTime;
        pincho.posicion.y += pincho.velocidad.y * deltaTime;
        pincho.posicion.z += pincho.velocidad.z * deltaTime;

        bool golpeo = false;

        for (int i = 0; i < limite; i++)
        {
            if (
                !minijuego.resultado.participantes[i].participo ||
                minijuego.estadosJugadores[i].eliminado ||
                !participantes[i].conectado ||
                jugadores[i].cayendo
            )
            {
                continue;
            }

            if (!PinchoTocaJugador(pincho, jugadores[i])) continue;

            float dx = jugadores[i].posicion.x - minijuego.posicionNucleo.x;
            float dz = jugadores[i].posicion.z - minijuego.posicionNucleo.z;
            float largo = std::sqrt(dx * dx + dz * dz);
            if (largo < 0.10f)
            {
                dx = 1.0f;
                dz = 0.0f;
                largo = 1.0f;
            }

            dx /= largo;
            dz /= largo;
            jugadores[i].empuje.x += dx * (8.5f + progreso * 5.0f);
            jugadores[i].empuje.z += dz * (8.5f + progreso * 5.0f);
            jugadores[i].velocidad.y = 6.0f + progreso * 2.0f;
            jugadores[i].enSuelo = false;
            jugadores[i].tiempoRalentizado = 0.45f;
            jugadores[i].multiplicadorRalentizacion = 0.35f;

            CrearParticulasImpactoGolpe(
                particulas,
                cantidadParticulas,
                jugadores[i].posicion
            );
            golpeo = true;
            break;
        }

        if (
            golpeo ||
            pincho.posicion.y <= 0.48f ||
            pincho.tiempoVida <= 0.0f
        )
        {
            pincho.activo = false;
        }
    }
}


static void FinalizarTormentaMagnetica(MinijuegoTormentaMagnetica& minijuego)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO) return;

    int vivos = ContarVivosMagnetica(minijuego);
    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        vivos == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;

    int tiempoFinalMs = (int)std::lround(minijuego.tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];
        if (!resultadoJugador.participo) continue;

        EstadoJugadorTormentaMagnetica& estado = minijuego.estadosJugadores[i];
        if (!estado.eliminado)
        {
            estado.posicionFinal = 1;
            estado.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estado.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = estado.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_MAGNETICA_TERMINADO;
}


void MinijuegoTormentaMagnetica::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++) estadosJugadores[i] = {};
    for (int i = 0; i < MAX_PINCHOS_TORMENTA_MAGNETICA; i++) pinchos[i] = {};

    suelo = {};
    suelo.posicion = { 0.0f, -0.40f, 0.0f };
    suelo.posicionInicial = suelo.posicion;
    suelo.tamano = { 12.2f, 0.80f, 12.2f };
    suelo.color = Color{ 57, 67, 82, 255 };
    suelo.activaColision = true;

    posicionNucleo = { 0.0f, 0.85f, 0.0f };
    campoAtrae = true;
    fase = FASE_MAGNETICA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_MAGNETICA;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;
    tiempoHastaCambioCampo = 2.8f;
    tiempoHastaPincho = 1.05f;
    tiempoAnimacion = 0.0f;
    cambiosCampo = 0;

    camara.position = { 0.0f, 10.8f, 13.6f };
    camara.target = { 0.0f, 0.25f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 49.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoTormentaMagnetica::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    const Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -3.7f, 1.05f, 3.7f }, { 3.7f, 1.05f, 3.7f },
        { -3.7f, 1.05f, -3.7f }, { 3.7f, 1.05f, -3.7f }
    };

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;
    for (int i = 0; i < limite; i++)
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawns[i]);
}


void MinijuegoTormentaMagnetica::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    Inicializar();
    ConfigurarJugadores(jugadores, cantidadMaxima);
}


void MinijuegoTormentaMagnetica::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoMinijuego(
            resultado,
            participantes,
            FORMATO_MINIJUEGO_INDIVIDUAL
        );
    }

    if (fase == FASE_MAGNETICA_TERMINADO) return;
    tiempoAnimacion += deltaTime;

    if (fase == FASE_MAGNETICA_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        tiempoPreparacion -= deltaTime;
        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_MAGNETICA_JUGANDO;
        }
        return;
    }

    tiempoJugado += deltaTime;
    tiempoHastaCambioCampo -= deltaTime;
    if (tiempoHastaCambioCampo <= 0.0f) CambiarCampoMagnetico(*this);

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;
    int vivosAntes = ContarVivosMagnetica(*this);

    for (int i = 0; i < limite; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        AplicarCampoMagneticoAJugador(*this, jugador, deltaTime);

        InputMinijuegoParticipante entrada{};
        if (participantes[i].conectado)
            entrada = LeerInputMinijuegoParticipante(participantes[i]);

        BloquePrueba sueloJugador = suelo;
        sueloJugador.activaColision = JugadorSobreArenaMagnetica(jugador);

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            true,
            false,
            deltaTime
        );

        if (!JugadorSobreArenaMagnetica(jugador) && jugador.posicion.y < -1.25f)
            jugador.cayendo = true;
    }

    ActualizarPinchosMagneticos(
        *this,
        deltaTime,
        jugadores,
        limite,
        participantes,
        particulas,
        cantidadParticulas
    );

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        participantes,
        limite,
        particulas,
        cantidadParticulas
    );

    int eliminados = 0;
    for (int i = 0; i < limite; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminados++;
        }
    }

    int posicionEliminados = vivosAntes - eliminados + 1;
    if (posicionEliminados < 1) posicionEliminados = 1;
    int tiempoMs = (int)std::lround(tiempoJugado * 1000.0f);

    for (int i = 0; i < limite; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal = posicionEliminados;
            estadosJugadores[i].tiempoSobrevividoMs = tiempoMs;
        }
    }

    if (vivosAntes - eliminados <= 1)
        FinalizarTormentaMagnetica(*this);
}


static void DibujarNucleoMagnetico(Vector3 posicion, bool atrae, float tiempo)
{
    float pulso = 1.0f + std::sin(tiempo * 5.2f) * 0.10f;
    Color color = atrae
        ? Color{ 69, 206, 239, 255 }
        : Color{ 239, 82, 147, 255 };

    DrawSphere(posicion, RADIO_NUCLEO_MAGNETICO * pulso, color);
    DrawSphereWires(
        posicion,
        RADIO_NUCLEO_MAGNETICO * 1.26f * pulso,
        10,
        14,
        RAYWHITE
    );

    for (int i = 0; i < 3; i++)
    {
        float radio = 1.1f + i * 0.55f + std::sin(tiempo * 4.0f + i) * 0.12f;
        DrawCircle3D(
            { posicion.x, 0.025f, posicion.z },
            radio,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(color, 0.42f - i * 0.08f)
        );
    }
}


static void DibujarPinchoMagnetico(const PinchoTormentaMagnetica& pincho)
{
    DrawCylinderEx(
        { pincho.posicion.x, pincho.posicion.y + 0.55f, pincho.posicion.z },
        { pincho.posicion.x, pincho.posicion.y - 0.55f, pincho.posicion.z },
        0.38f,
        0.025f,
        8,
        Color{ 180, 188, 201, 255 }
    );
    DrawSphere(
        { pincho.posicion.x, pincho.posicion.y + 0.56f, pincho.posicion.z },
        0.15f,
        Color{ 83, 95, 111, 255 }
    );
}


void MinijuegoTormentaMagnetica::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    ClearBackground(Color{ 19, 24, 36, 255 });
    BeginMode3D(camara);

    DrawCube(suelo.posicion, suelo.tamano.x, suelo.tamano.y, suelo.tamano.z, suelo.color);
    DrawCubeWires(suelo.posicion, suelo.tamano.x, suelo.tamano.y, suelo.tamano.z, Color{ 23, 30, 43, 255 });

    for (int i = -4; i <= 4; i++)
    {
        DrawCube(
            { (float)i * 1.25f, 0.025f, 0.0f },
            0.045f,
            0.035f,
            11.2f,
            Fade(Color{ 106, 130, 157, 255 }, 0.55f)
        );
    }

    DibujarNucleoMagnetico(posicionNucleo, campoAtrae, tiempoAnimacion);

    for (int i = 0; i < MAX_PINCHOS_TORMENTA_MAGNETICA; i++)
    {
        if (pinchos[i].activo) DibujarPinchoMagnetico(pinchos[i]);
    }

    DibujarParticulasTierra(particulas, cantidadParticulas);

    int limite = cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || estadosJugadores[i].eliminado)
            continue;

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);
        if (mostrarDebug && !jugadores[i].cayendo)
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
    }

    if (mostrarDebug) DrawBoundingBox(CrearHitboxBloquePrueba(suelo), YELLOW);
    EndMode3D();

    DrawText("TORMENTA MAGNETICA", 24, 22, 30, RAYWHITE);
    DrawText(
        campoAtrae
            ? "CAMPO: ATRACCION - MUCHO MAS FUERTE"
            : "CAMPO: REPULSION - ALEJATE DEL BORDE",
        24,
        60,
        19,
        campoAtrae ? Color{ 89, 220, 246, 255 } : Color{ 250, 105, 162, 255 }
    );

    if (fase == FASE_MAGNETICA_JUGANDO)
    {
        int dificultad = (int)std::lround(ProgresoDificultadMagnetica(*this) * 100.0f);
        DrawText(
            TextFormat("DIFICULTAD: %d%%", dificultad),
            GetScreenWidth() - 235,
            24,
            22,
            GOLD
        );
        DrawText(
            TextFormat("CAMBIO EN: %.1f", tiempoHastaCambioCampo),
            GetScreenWidth() - 205,
            55,
            18,
            LIGHTGRAY
        );
    }

    if (fase == FASE_MAGNETICA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 84) / 2,
            GetScreenHeight() / 2 - 60,
            84,
            GOLD
        );
    }
    else if (fase == FASE_MAGNETICA_TERMINADO)
    {
        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(
            resultado, ganadores, MAX_PARTICIPANTES
        );
        const char* titulo = resultado.desenlace == DESENLACE_EMPATE
            ? "EMPATE"
            : TextFormat(
                "GANADOR: JUGADOR %d",
                cantidadGanadores == 1
                    ? participantes[ganadores[0]].numeroJugador
                    : 0
            );

        DrawRectangle(
            GetScreenWidth() / 2 - 305,
            GetScreenHeight() / 2 - 125,
            610,
            250,
            Fade(BLACK, 0.90f)
        );
        DrawText(
            titulo,
            GetScreenWidth() / 2 - MeasureText(titulo, 34) / 2,
            GetScreenHeight() / 2 - 82,
            34,
            GOLD
        );
        DrawText(
            "R PARA REINICIAR",
            GetScreenWidth() / 2 - MeasureText("R PARA REINICIAR", 21) / 2,
            GetScreenHeight() / 2 + 72,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoTormentaMagnetica::ObtenerResultado() const
{
    return resultado;
}
