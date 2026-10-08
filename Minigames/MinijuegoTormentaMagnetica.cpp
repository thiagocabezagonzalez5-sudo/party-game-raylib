#include "Minigames/MinijuegoTormentaMagnetica.h"

#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/AudioMinijuegos.h"
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
    minijuego.tiempoDesdeCambio = 0.0f;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GROUND_POUND);
    minijuego.campoAtrae = !minijuego.campoAtrae;
    minijuego.posicionNucleo = ElegirNuevaPosicionNucleoMagnetico(
        minijuego.posicionNucleo
    );

    float progreso = ProgresoDificultadMagnetica(minijuego);

    // Cada cambio dura entre 3.6 s y 1.8 s (nunca menos, para poder reaccionar).
    minijuego.tiempoHastaCambioCampo = 3.6f - progreso * 1.8f;
    if (minijuego.tiempoHastaCambioCampo < 1.8f)
        minijuego.tiempoHastaCambioCampo = 1.8f;
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
        5.6f,
        (float)GetRandomValue(-500, 500) / 100.0f
    };
    pincho.velocidad =
    {
        0.0f,
        -(7.0f + progreso * 6.0f + (float)GetRandomValue(0, 20) / 10.0f),
        0.0f
    };
    pincho.tiempoVida = 3.0f;
    pincho.aviso = pincho.avisoTotal;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_UI_MOVER);
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

        if (pincho.aviso > 0.0f)
        {
            // Solo la marca en el suelo: todavia no cae ni golpea.
            pincho.aviso -= deltaTime;
            continue;
        }

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
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++) bots[i] = {};
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
    tiempoHastaCambioCampo = 3.5f;
    tiempoDesdeCambio = 99.0f;
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


static float AleatorioMagnetica(float minimo, float maximo)
{
    return minimo + (maximo - minimo) *
        (float)GetRandomValue(0, 1000) / 1000.0f;
}


// El bot nota el cambio de campo ~0.3 s tarde y ubica el nucleo con error,
// asi que durante el retardo empuja en la direccion equivocada. Se mantiene
// cerca de un ancla central, se opone al campo y esquiva pinchos cuando esta
// atento (70% de los chequeos).
static InputMinijuegoParticipante CrearEntradaBotMagnetica(
    MinijuegoTormentaMagnetica& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    EstadoBotTormentaMagnetica& bot = minijuego.bots[indice];

    if (bot.cambiosVistos != minijuego.cambiosCampo)
    {
        if (bot.retardo <= 0.0f) bot.retardo = AleatorioMagnetica(0.22f, 0.45f);
        bot.retardo -= deltaTime;

        if (bot.retardo <= 0.0f)
        {
            bot.cambiosVistos = minijuego.cambiosCampo;
            bot.campoAtrae = minijuego.campoAtrae;
            bot.nucleoPercibido = {
                minijuego.posicionNucleo.x + AleatorioMagnetica(-0.9f, 0.9f),
                0.0f,
                minijuego.posicionNucleo.z + AleatorioMagnetica(-0.9f, 0.9f)
            };
        }
    }

    bot.tiempoAncla -= deltaTime;
    if (bot.tiempoAncla <= 0.0f)
    {
        bot.tiempoAncla = AleatorioMagnetica(1.2f, 2.8f);
        bot.ancla = {
            AleatorioMagnetica(-1.8f, 1.8f), 0.0f,
            AleatorioMagnetica(-1.8f, 1.8f)
        };
    }

    bot.tiempoAtencion -= deltaTime;
    if (bot.tiempoAtencion <= 0.0f)
    {
        bot.tiempoAtencion = AleatorioMagnetica(0.35f, 0.7f);
        bot.atento = GetRandomValue(1, 100) <= 70;
    }

    // Esquivar pincho cercano que ya esta bajo.
    if (bot.atento)
    {
        for (int p = 0; p < MAX_PINCHOS_TORMENTA_MAGNETICA; p++)
        {
            const PinchoTormentaMagnetica& pincho = minijuego.pinchos[p];
            if (!pincho.activo) continue;

            // La marca del suelo se ve desde que aparece; el bot solo reacciona
            // cuando ya es peligrosa (mismo aviso que ve un humano).
            if (pincho.aviso > 0.0f && pincho.aviso > pincho.avisoTotal - 0.30f) continue;

            float dx = jugador.posicion.x - pincho.posicion.x;
            float dz = jugador.posicion.z - pincho.posicion.z;
            float radioEsquiva = pincho.aviso > 0.0f ? 1.3f : 1.1f;
            if (dx * dx + dz * dz > radioEsquiva * radioEsquiva) continue;

            InputMinijuegoParticipante entrada{};
            if (dx >= 0.0f) entrada.derecha = true; else entrada.izquierda = true;
            if (dz >= 0.0f) entrada.atras = true; else entrada.adelante = true;
            return entrada;
        }
    }

    float x = jugador.posicion.x;
    float z = jugador.posicion.z;
    float limiteBorde = MEDIO_LADO_ARENA_MAGNETICA - 1.5f;

    if (std::fabs(x) > limiteBorde || std::fabs(z) > limiteBorde)
    {
        return CrearEntradaBotHaciaObjetivo1v3(
            jugador.posicion, bot.ancla, 0.2f
        );
    }

    // Oponerse al campo percibido.
    float dx = bot.nucleoPercibido.x - x;
    float dz = bot.nucleoPercibido.z - z;
    float distancia = std::sqrt(dx * dx + dz * dz);
    if (distancia < 0.3f) distancia = 0.3f;

    float signo = bot.campoAtrae ? -1.0f : 1.0f;
    float ox = signo * dx / distancia;
    float oz = signo * dz / distancia;

    // Si ya esta cerca del ancla, solo contrarresta; si no, mezcla.
    float ax = bot.ancla.x - x;
    float az = bot.ancla.z - z;
    float da = std::sqrt(ax * ax + az * az);
    if (da > 0.01f)
    {
        float peso = da > 2.5f ? 1.2f : 0.5f;
        ox += ax / da * peso;
        oz += az / da * peso;
    }

    InputMinijuegoParticipante entrada{};
    if (ox < -0.4f) entrada.izquierda = true;
    else if (ox > 0.4f) entrada.derecha = true;
    if (oz < -0.4f) entrada.adelante = true;
    else if (oz > 0.4f) entrada.atras = true;
    return entrada;
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
    tiempoDesdeCambio += deltaTime;
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
        if (participantes[i].esBot)
            entrada = CrearEntradaBotMagnetica(*this, i, jugador, deltaTime);
        else if (participantes[i].conectado)
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
    if (pincho.aviso > 0.0f)
    {
        // Marca que crece durante el aviso: el pincho caera justo aqui.
        float t = 1.0f - pincho.aviso / pincho.avisoTotal;
        float radio = 0.25f + 0.65f * t;
        Color marca = Color{ 255, 90, 70, 255 };
        DrawCylinder({ pincho.posicion.x, 0.03f, pincho.posicion.z }, radio, radio, 0.02f, 20, Fade(marca, 0.18f + 0.30f * t));
        DrawCircle3D({ pincho.posicion.x, 0.05f, pincho.posicion.z }, 0.92f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(marca, 0.55f));
        DrawCircle3D({ pincho.posicion.x, 0.05f, pincho.posicion.z }, radio, { 1.0f, 0.0f, 0.0f }, 90.0f, marca);
        return;
    }

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


// Anillo de color bajo los pies y flecha sobre la cabeza: el modelo del
// jugador es oscuro y sobre el suelo oscuro casi no se distingue.
// MODELO FUTURO: la flecha puede pasar a ser un icono de jugador del GLB.
static void DibujarIndicadorJugadorMagnetica(
    const JugadorPrueba& jugador,
    Color color
)
{
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (jugador.cayendo || pies < -0.35f) return;

    Vector3 centro = { jugador.posicion.x, 0.05f, jugador.posicion.z };
    for (int k = 0; k < 3; k++)
    {
        DrawCircle3D(
            centro,
            0.52f + k * 0.045f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            color
        );
    }

    float rebote = std::sin((float)GetTime() * 5.0f + jugador.posicion.x) * 0.06f;
    float cabeza = pies + 2.15f + rebote;
    DrawCylinderEx(
        { jugador.posicion.x, cabeza, jugador.posicion.z },
        { jugador.posicion.x, cabeza + 0.32f, jugador.posicion.z },
        0.0f,
        0.20f,
        10,
        color
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

    Color colorCampo = campoAtrae
        ? Color{ 69, 206, 239, 255 }
        : Color{ 239, 82, 147, 255 };

    DrawCube(suelo.posicion, suelo.tamano.x, suelo.tamano.y, suelo.tamano.z, suelo.color);
    DrawCubeWires(suelo.posicion, suelo.tamano.x, suelo.tamano.y, suelo.tamano.z, Color{ 23, 30, 43, 255 });

    // Borde neon del color del campo actual: anuncia atraccion/repulsion
    // y marca el limite de la plataforma.
    float mitad = suelo.tamano.x * 0.5f;
    float cima = suelo.posicion.y + suelo.tamano.y * 0.5f;
    float grosor = 0.14f;
    float brillo = 0.80f + 0.20f * std::sin(tiempoAnimacion * 6.0f);
    bool avisoCambio = fase == FASE_MAGNETICA_JUGANDO && tiempoHastaCambioCampo < 0.9f;
    Color neon = Fade(colorCampo, brillo);
    if (avisoCambio)
    {
        bool destello = std::fmod(tiempoAnimacion * 8.0f, 2.0f) < 1.0f;
        neon = destello ? WHITE : Color{ 255, 210, 60, 255 };
    }
    else if (tiempoDesdeCambio < 0.35f)
    {
        neon = WHITE;
    }
    DrawCubeV({ 0.0f, cima + 0.012f, -mitad + grosor * 0.5f }, { suelo.tamano.x, 0.02f, grosor }, neon);
    DrawCubeV({ 0.0f, cima + 0.012f, mitad - grosor * 0.5f }, { suelo.tamano.x, 0.02f, grosor }, neon);
    DrawCubeV({ -mitad + grosor * 0.5f, cima + 0.012f, 0.0f }, { grosor, 0.02f, suelo.tamano.x - grosor * 2.0f }, neon);
    DrawCubeV({ mitad - grosor * 0.5f, cima + 0.012f, 0.0f }, { grosor, 0.02f, suelo.tamano.x - grosor * 2.0f }, neon);

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

        DibujarIndicadorJugadorMagnetica(jugadores[i], participantes[i].color);
        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);
        if (mostrarDebug && !jugadores[i].cayendo)
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
    }

    if (mostrarDebug) DrawBoundingBox(CrearHitboxBloquePrueba(suelo), YELLOW);
    EndMode3D();

    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const float e = (float)alto / 720.0f;

    DrawRectangle(0, 0, ancho, (int)(76 * e), Fade(BLACK, 0.55f));
    DrawText("TORMENTA MAGNETICA", (int)(24 * e), (int)(10 * e), (int)(30 * e), RAYWHITE);
    DrawText(
        campoAtrae
            ? "CAMPO: ATRACCION - MUCHO MAS FUERTE"
            : "CAMPO: REPULSION - ALEJATE DEL BORDE",
        (int)(24 * e),
        (int)(46 * e),
        (int)(19 * e),
        campoAtrae ? Color{ 89, 220, 246, 255 } : Color{ 250, 105, 162, 255 }
    );

    if (fase == FASE_MAGNETICA_JUGANDO)
    {
        int dificultad = (int)std::lround(ProgresoDificultadMagnetica(*this) * 100.0f);
        const char* textoDificultad = TextFormat("DIFICULTAD %d%%", dificultad);
        const char* textoCambio = TextFormat("CAMBIO EN %.1f", tiempoHastaCambioCampo);
        DrawText(
            textoDificultad,
            ancho - MeasureText(textoDificultad, (int)(22 * e)) - (int)(30 * e),
            (int)(12 * e),
            (int)(22 * e),
            GOLD
        );
        DrawText(
            textoCambio,
            ancho - MeasureText(textoCambio, (int)(20 * e)) - (int)(30 * e),
            (int)(44 * e),
            (int)(20 * e),
            tiempoHastaCambioCampo < 0.9f ? Color{ 255, 210, 60, 255 } : LIGHTGRAY
        );

        if (avisoCambio)
        {
            const char* aviso = "CAMBIO DE CAMPO!";
            int tam = (int)(34 * e);
            int x = ancho / 2 - MeasureText(aviso, tam) / 2;
            DrawText(aviso, x + 3, (int)(96 * e) + 3, tam, Fade(BLACK, 0.7f));
            DrawText(aviso, x, (int)(96 * e), tam, Color{ 255, 210, 60, 255 });
        }
        else if (tiempoDesdeCambio < 1.2f)
        {
            const char* aviso = campoAtrae ? "AHORA: ATRACCION" : "AHORA: REPULSION";
            int tam = (int)(30 * e);
            int x = ancho / 2 - MeasureText(aviso, tam) / 2;
            DrawText(aviso, x + 3, (int)(96 * e) + 3, tam, Fade(BLACK, 0.7f));
            DrawText(aviso, x, (int)(96 * e), tam, colorCampo);
        }
    }

    int cantidadTarjetas = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo) cantidadTarjetas++;
    }

    int anchoTarjeta = (int)(170 * e);
    int altoTarjeta = (int)(44 * e);
    int separacion = (int)(14 * e);
    int xTarjeta = ancho / 2 - (cantidadTarjetas * anchoTarjeta + (cantidadTarjetas - 1) * separacion) / 2;
    int yTarjeta = alto - altoTarjeta - (int)(18 * e);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        bool fuera = estadosJugadores[i].eliminado;
        DrawRectangle(xTarjeta, yTarjeta, anchoTarjeta, altoTarjeta, Fade(BLACK, fuera ? 0.40f : 0.62f));
        DrawRectangle(xTarjeta, yTarjeta, (int)(8 * e), altoTarjeta, fuera ? GRAY : participantes[i].color);
        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            xTarjeta + (int)(18 * e),
            yTarjeta + (int)(11 * e),
            (int)(22 * e),
            fuera ? GRAY : participantes[i].color
        );
        DrawText(
            fuera ? "FUERA" : "EN JUEGO",
            xTarjeta + (int)(70 * e),
            yTarjeta + (int)(14 * e),
            (int)(17 * e),
            fuera ? GRAY : RAYWHITE
        );

        xTarjeta += anchoTarjeta + separacion;
    }

    if (fase == FASE_MAGNETICA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        int tamano = (int)(96 * e);
        int x = ancho / 2 - MeasureText(texto, tamano) / 2;
        int y = alto / 2 - (int)(190 * e);
        DrawText(texto, x + 4, y + 4, tamano, Fade(BLACK, 0.7f));
        DrawText(texto, x, y, tamano, GOLD);
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

        int anchoPanel = (int)(520 * e);
        int altoPanel = (int)(262 * e);
        int xPanel = ancho / 2 - anchoPanel / 2;
        int yPanel = (int)(92 * e);
        DrawRectangle(xPanel, yPanel, anchoPanel, altoPanel, Fade(BLACK, 0.82f));

        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, (int)(32 * e)) / 2,
            yPanel + (int)(14 * e),
            (int)(32 * e),
            GOLD
        );

        const char* total = TextFormat("TIEMPO TOTAL: %.1f s", tiempoJugado);
        DrawText(
            total,
            ancho / 2 - MeasureText(total, (int)(20 * e)) / 2,
            yPanel + (int)(52 * e),
            (int)(20 * e),
            LIGHTGRAY
        );

        int fila = yPanel + (int)(88 * e);
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo) continue;
            DrawText(
                TextFormat(
                    "J%d  POSICION %d  SOBREVIVIO %.1f s",
                    participantes[i].numeroJugador,
                    resultado.participantes[i].posicionFinal,
                    (float)estadosJugadores[i].tiempoSobrevividoMs / 1000.0f
                ),
                xPanel + (int)(50 * e),
                fila,
                (int)(21 * e),
                participantes[i].color
            );
            fila += (int)(27 * e);
        }

        DrawText(
            TextoReinicioMinijuego(),
            ancho / 2 - MeasureText(TextoReinicioMinijuego(), (int)(18 * e)) / 2,
            yPanel + altoPanel - (int)(30 * e),
            (int)(18 * e),
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoTormentaMagnetica::ObtenerResultado() const
{
    return resultado;
}
