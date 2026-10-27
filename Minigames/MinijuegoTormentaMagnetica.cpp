#include "Minigames/MinijuegoTormentaMagnetica.h"

#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_MAGNETICA = 3.0f;
static const float ESCALA_DIFICULTAD_MAGNETICA = 42.0f;
static const float MEDIO_LADO_ARENA_MAGNETICA = 5.75f;
static const float RADIO_NUCLEO_MAGNETICO = 0.72f;


// Efectos solo visuales / de audio (definidos con el dibujo, al final).
static void ActualizarVisualTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[]
);

static void EfectoCambioCampoTormenta(MinijuegoTormentaMagnetica& minijuego);

static void EfectoImpactoPinchoTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 posicion
);

static void EfectoGolpeTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    Color colorJugador
);

static void EfectoEliminacionTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    int indice,
    const JugadorPrueba& jugador
);

static void EmitirSalpicaduraTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 pies
);


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
    minijuego.nucleoAnterior = minijuego.posicionNucleo;
    minijuego.posicionNucleo = ElegirNuevaPosicionNucleoMagnetico(
        minijuego.posicionNucleo
    );

    float progreso = ProgresoDificultadMagnetica(minijuego);

    // Cada cambio dura entre 3.6 s y 1.8 s (nunca menos, para poder reaccionar).
    minijuego.tiempoHastaCambioCampo = 3.6f - progreso * 1.8f;
    if (minijuego.tiempoHastaCambioCampo < 1.8f)
        minijuego.tiempoHastaCambioCampo = 1.8f;

    // Solo visual: medidor del HUD y rayo sobre el nucleo nuevo.
    minijuego.duracionCampo = minijuego.tiempoHastaCambioCampo;
    EfectoCambioCampoTormenta(minijuego);
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
            EfectoGolpeTormenta(minijuego, i, jugadores[i], participantes[i].color);
            golpeo = true;
            break;
        }

        if (!golpeo && pincho.posicion.y <= 0.48f)
            EfectoImpactoPinchoTormenta(minijuego, pincho.posicion);

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

    tiempoVisual = 0.0f;
    duracionCampo = tiempoHastaCambioCampo;
    nucleoAnterior = posicionNucleo;
    semillaVisual = 2891336453u;
    confetiLanzado = false;
    for (int i = 0; i < MAX_PARTICULAS_TORMENTA_MAGNETICA; i++) particulasEfecto[i] = {};
    for (int i = 0; i < MAX_ONDAS_TORMENTA_MAGNETICA; i++) ondas[i] = {};
    for (int i = 0; i < MAX_AVISOS_TORMENTA_MAGNETICA; i++) avisos[i] = {};

    // Mas baja que antes: detras de la plataforma se ve la tormenta.
    camara.position = { 0.0f, 8.4f, 14.2f };
    camara.target = { 0.0f, 0.6f, -1.6f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
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

    // Animaciones y particulas siguen tambien con el minijuego terminado.
    ActualizarVisualTormenta(*this, deltaTime, jugadores);

    if (fase == FASE_MAGNETICA_TERMINADO) return;
    tiempoAnimacion += deltaTime;

    if (fase == FASE_MAGNETICA_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);
        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_MAGNETICA_JUGANDO;
        }
        return;
    }

    tiempoJugado += deltaTime;
    tiempoDesdeCambio += deltaTime;
    float cambioAntes = tiempoHastaCambioCampo;
    tiempoHastaCambioCampo -= deltaTime;

    // Aviso sonoro un poco antes de que el campo se invierta.
    if (cambioAntes > 0.9f && tiempoHastaCambioCampo <= 0.9f && tiempoHastaCambioCampo > 0.0f)
        ReproducirSonidoMinijuego(audio, SONIDO_ALERTA_TIEMPO);

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

        // Solo visual / audio: salpicadura al aterrizar y sonido de salto.
        EstadoJugadorTormentaMagnetica& estadoJugador = estadosJugadores[i];
        if (jugador.enSuelo && !estadoJugador.enSueloAnterior)
        {
            EmitirSalpicaduraTormenta(*this, jugador.posicion);
        }
        else if (
            !jugador.enSuelo &&
            estadoJugador.enSueloAnterior &&
            jugador.velocidad.y > 0.5f &&
            !participantes[i].esBot
        )
        {
            ReproducirSonidoMinijuego(audio, SONIDO_SALTO);
        }
        estadoJugador.enSueloAnterior = jugador.enSuelo;

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
            EfectoEliminacionTormenta(*this, i, jugadores[i]);
        }
    }

    if (vivosAntes - eliminados <= 1)
        FinalizarTormentaMagnetica(*this);
}



//==================================================
// EFECTOS VISUALES (sin efecto en la logica)
//==================================================

// Mar de nubes bajo la torre: ahi se pierden los que caen.
static const float NIVEL_NUBES_TORMENTA = -16.0f;

// Espesor visual de la cubierta (la cara de arriba esta en y = 0).
static const float ESPESOR_CUBIERTA_TORMENTA = 0.55f;

static const float DURACION_AVISO_TORMENTA = 1.6f;

// Colores del campo: acento de la escena, solo en el nucleo, sus ondas
// y la tira de luz del borde.
static const Color COLOR_ATRAE_TORMENTA = { 69, 206, 239, 255 };
static const Color COLOR_REPELE_TORMENTA = { 239, 82, 147, 255 };
static const Color COLOR_AMBAR_TORMENTA = { 255, 160, 48, 255 };
static const Color COLOR_RAYO_TORMENTA = { 196, 210, 255, 255 };


static float Limitar01VisualTormenta(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


// Hash determinista para la decoracion (misma escena en cada partida).
static float Hash01Tormenta(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarTormenta(MinijuegoTormentaMagnetica& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasTormenta(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static Color ColorCampoTormenta(bool atrae)
{
    return atrae ? COLOR_ATRAE_TORMENTA : COLOR_REPELE_TORMENTA;
}


static void EmitirParticulaTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    float freno,
    Color color,
    bool brilla
)
{
    for (int i = 0; i < MAX_PARTICULAS_TORMENTA_MAGNETICA; i++)
    {
        ParticulaTormentaMagnetica& p = minijuego.particulasEfecto[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brilla = brilla;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.freno = freno;
        p.color = color;
        return;
    }
}


static void EmitirOndaTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 posicion,
    float radioMaximo,
    float vida,
    Color color,
    bool haciaAdentro
)
{
    int elegida = 0;
    float menor = 1.0e9f;

    for (int i = 0; i < MAX_ONDAS_TORMENTA_MAGNETICA; i++)
    {
        const OndaTormentaMagnetica& o = minijuego.ondas[i];

        if (!o.activa)
        {
            elegida = i;
            break;
        }

        if (o.vida < menor)
        {
            menor = o.vida;
            elegida = i;
        }
    }

    OndaTormentaMagnetica& onda = minijuego.ondas[elegida];
    onda.activa = true;
    onda.haciaAdentro = haciaAdentro;
    onda.posicion = posicion;
    onda.radioMaximo = radioMaximo;
    onda.vida = vida;
    onda.vidaMaxima = vida;
    onda.color = color;
}


// Chispas electricas: brillan (pasada aditiva) y caen rebotando.
static void EmitirChispasTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 centro,
    int base,
    float fuerza,
    Color color
)
{
    int cantidad = CantidadParticulasTormenta(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarTormenta(minijuego, 0.0f, 6.2832f);
        float r = AzarTormenta(minijuego, 0.4f, 1.0f) * fuerza;
        Color c = k % 3 == 0 ? Color{ 255, 255, 240, 255 } : color;

        EmitirParticulaTormenta(
            minijuego,
            centro,
            { std::cos(a) * r, AzarTormenta(minijuego, 0.6f, 1.4f) * fuerza, std::sin(a) * r },
            AzarTormenta(minijuego, 0.25f, 0.6f),
            AzarTormenta(minijuego, 0.05f, 0.10f),
            14.0f,
            1.2f,
            c,
            true
        );
    }
}


// Salpicadura de agua de lluvia al aterrizar sobre la chapa mojada.
static void EmitirSalpicaduraTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 pies
)
{
    int cantidad = CantidadParticulasTormenta(6);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarTormenta(minijuego, 0.0f, 6.2832f);
        float r = AzarTormenta(minijuego, 0.8f, 1.8f);

        EmitirParticulaTormenta(
            minijuego,
            { pies.x + std::cos(a) * 0.25f, 0.05f, pies.z + std::sin(a) * 0.25f },
            { std::cos(a) * r, AzarTormenta(minijuego, 1.0f, 2.2f), std::sin(a) * r },
            AzarTormenta(minijuego, 0.3f, 0.5f),
            AzarTormenta(minijuego, 0.05f, 0.09f),
            9.0f,
            2.0f,
            Color{ 150, 176, 214, 255 },
            false
        );
    }
}


// El campo se invierte: un rayo cae sobre el nucleo nuevo, onda hacia
// adentro (atrae) o hacia afuera (repele) y chispas del color del campo.
static void EfectoCambioCampoTormenta(MinijuegoTormentaMagnetica& minijuego)
{
    Color color = ColorCampoTormenta(minijuego.campoAtrae);
    Vector3 suelo = { minijuego.posicionNucleo.x, 0.05f, minijuego.posicionNucleo.z };

    EmitirOndaTormenta(minijuego, suelo, 5.5f, 0.9f, color, minijuego.campoAtrae);
    EmitirOndaTormenta(minijuego, suelo, 2.4f, 0.45f, Color{ 230, 236, 255, 255 }, false);
    EmitirOndaTormenta(
        minijuego,
        { minijuego.nucleoAnterior.x, 0.05f, minijuego.nucleoAnterior.z },
        1.6f,
        0.4f,
        Color{ 150, 160, 190, 255 },
        true
    );
    EmitirChispasTormenta(minijuego, minijuego.posicionNucleo, 22, 4.5f, color);
}


// Un pincho se clava en la chapa: chispas ambar y una onda corta.
static void EfectoImpactoPinchoTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 posicion
)
{
    Vector3 suelo = { posicion.x, 0.05f, posicion.z };
    EmitirChispasTormenta(minijuego, { posicion.x, 0.15f, posicion.z }, 9, 3.2f, COLOR_AMBAR_TORMENTA);
    EmitirOndaTormenta(minijuego, suelo, 1.3f, 0.35f, COLOR_AMBAR_TORMENTA, false);
}


// Un pincho alcanza a un jugador: destello, chispas de su color y temblor.
static void EfectoGolpeTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    Color colorJugador
)
{
    minijuego.estadosJugadores[indice].destelloGolpe = 1.0f;

    EmitirChispasTormenta(minijuego, { jugador.posicion.x, jugador.posicion.y + 0.4f, jugador.posicion.z }, 12, 4.0f, colorJugador);
    EmitirChispasTormenta(minijuego, { jugador.posicion.x, jugador.posicion.y + 0.4f, jugador.posicion.z }, 8, 3.0f, COLOR_RAYO_TORMENTA);
    EmitirOndaTormenta(minijuego, { jugador.posicion.x, 0.05f, jugador.posicion.z }, 1.8f, 0.4f, colorJugador, false);

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GOLPE);
    ActivarTemblorCamaraGeneral(0.14f, 0.22f);
}


// Eliminado: sigue cayendo hacia el mar de nubes (solo dibujo) y aparece
// «¡J# AL VACÍO!» sobre el borde por donde salio.
static void EfectoEliminacionTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    int indice,
    const JugadorPrueba& jugador
)
{
    EstadoJugadorTormentaMagnetica& estado = minijuego.estadosJugadores[indice];

    estado.tiempoCaida = 0.0f;
    estado.posicionCaida = jugador.posicion;
    estado.velocidadCaida =
    {
        (jugador.velocidad.x + jugador.empuje.x) * 0.35f,
        jugador.velocidad.y < -2.0f ? jugador.velocidad.y : -2.0f,
        (jugador.velocidad.z + jugador.empuje.z) * 0.35f
    };
    estado.tocoFondo = false;

    float limite = MEDIO_LADO_ARENA_MAGNETICA;
    float bx = jugador.posicion.x;
    float bz = jugador.posicion.z;
    if (bx > limite) bx = limite;
    if (bx < -limite) bx = -limite;
    if (bz > limite) bz = limite;
    if (bz < -limite) bz = -limite;

    for (int i = 0; i < MAX_AVISOS_TORMENTA_MAGNETICA; i++)
    {
        AvisoTormentaMagnetica& aviso = minijuego.avisos[i];

        if (aviso.activo)
        {
            continue;
        }

        aviso.activo = true;
        aviso.jugador = indice;
        aviso.posicion = { bx, 0.6f, bz };
        aviso.vida = DURACION_AVISO_TORMENTA;
        break;
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
}


// El que cae atraviesa el mar de nubes: bocanada gris y un chispazo.
static void EmitirBocanadaNubesTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 centro
)
{
    int cantidad = CantidadParticulasTormenta(16);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarTormenta(minijuego, 0.0f, 6.2832f);
        float r = AzarTormenta(minijuego, 0.8f, 2.6f);

        EmitirParticulaTormenta(
            minijuego,
            { centro.x + std::cos(a) * 0.5f, NIVEL_NUBES_TORMENTA + 1.0f, centro.z + std::sin(a) * 0.5f },
            { std::cos(a) * r, AzarTormenta(minijuego, 2.0f, 5.0f), std::sin(a) * r },
            AzarTormenta(minijuego, 0.8f, 1.4f),
            AzarTormenta(minijuego, 0.35f, 0.7f),
            2.0f,
            1.4f,
            k % 2 == 0 ? Color{ 92, 98, 128, 255 } : Color{ 120, 126, 156, 255 },
            false
        );
    }

    EmitirChispasTormenta(minijuego, { centro.x, NIVEL_NUBES_TORMENTA + 1.2f, centro.z }, 8, 5.0f, COLOR_RAYO_TORMENTA);
}


static void EmitirConfetiTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    Vector3 centro
)
{
    const Color colores[6] =
    {
        { 255, 214, 84, 255 }, { 69, 206, 239, 255 }, { 239, 82, 147, 255 },
        { 150, 230, 130, 255 }, { 250, 246, 236, 255 }, { 255, 160, 48, 255 }
    };

    int cantidad = CantidadParticulasTormenta(100);

    for (int k = 0; k < cantidad; k++)
    {
        EmitirParticulaTormenta(
            minijuego,
            {
                centro.x + AzarTormenta(minijuego, -2.6f, 2.6f),
                AzarTormenta(minijuego, 3.6f, 5.8f),
                centro.z + AzarTormenta(minijuego, -2.0f, 2.0f)
            },
            {
                AzarTormenta(minijuego, -0.8f, 0.8f),
                AzarTormenta(minijuego, -0.4f, 1.2f),
                AzarTormenta(minijuego, -0.6f, 0.6f)
            },
            AzarTormenta(minijuego, 2.4f, 3.8f),
            AzarTormenta(minijuego, 0.07f, 0.12f),
            1.6f,
            1.2f,
            colores[k % 6],
            false
        );
    }
}


// Puntas de las bobinas Tesla (chispas que saltan solas).
static const Vector3 PUNTAS_BOBINAS_TORMENTA[3] =
{
    { -7.6f, 2.55f, -7.6f },
    { 7.6f, 2.55f, -7.6f },
    { 11.8f, 6.15f, -8.2f }
};


static void ActualizarVisualTormenta(
    MinijuegoTormentaMagnetica& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[]
)
{
    minijuego.tiempoVisual += deltaTime;

    for (int i = 0; i < MAX_PARTICULAS_TORMENTA_MAGNETICA; i++)
    {
        ParticulaTormentaMagnetica& p = minijuego.particulasEfecto[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f || p.posicion.y < NIVEL_NUBES_TORMENTA - 1.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        float freno = std::exp(-p.freno * deltaTime);
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        // Las chispas rebotan sobre la cubierta.
        bool sobreCubierta =
            std::fabs(p.posicion.x) < MEDIO_LADO_ARENA_MAGNETICA &&
            std::fabs(p.posicion.z) < MEDIO_LADO_ARENA_MAGNETICA;

        if (sobreCubierta && p.posicion.y < 0.03f && p.velocidad.y < 0.0f)
        {
            p.posicion.y = 0.03f;
            p.velocidad.y *= -0.35f;
        }
    }

    for (int i = 0; i < MAX_ONDAS_TORMENTA_MAGNETICA; i++)
    {
        OndaTormentaMagnetica& o = minijuego.ondas[i];
        if (!o.activa) continue;
        o.vida -= deltaTime;
        if (o.vida <= 0.0f) o.activa = false;
    }

    for (int i = 0; i < MAX_AVISOS_TORMENTA_MAGNETICA; i++)
    {
        AvisoTormentaMagnetica& aviso = minijuego.avisos[i];
        if (!aviso.activo) continue;
        aviso.vida -= deltaTime;
        if (aviso.vida <= 0.0f) aviso.activo = false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorTormentaMagnetica& estado = minijuego.estadosJugadores[i];

        estado.destelloGolpe -= deltaTime * 2.2f;
        if (estado.destelloGolpe < 0.0f) estado.destelloGolpe = 0.0f;

        if (estado.tiempoCaida < 0.0f || estado.tocoFondo)
        {
            continue;
        }

        estado.tiempoCaida += deltaTime;
        estado.velocidadCaida.y -= 16.0f * deltaTime;
        estado.posicionCaida.x += estado.velocidadCaida.x * deltaTime;
        estado.posicionCaida.y += estado.velocidadCaida.y * deltaTime;
        estado.posicionCaida.z += estado.velocidadCaida.z * deltaTime;

        if (estado.posicionCaida.y < NIVEL_NUBES_TORMENTA + 0.6f)
        {
            estado.tocoFondo = true;
            EmitirBocanadaNubesTormenta(minijuego, estado.posicionCaida);
        }
    }

    // Chispas sueltas de las bobinas Tesla (ambiente).
    if (CalidadEfectos() != CALIDAD_BAJA)
    {
        for (int b = 0; b < 3; b++)
        {
            if (AzarTormenta(minijuego, 0.0f, 1.0f) > deltaTime * 2.6f) continue;

            Vector3 punta = PUNTAS_BOBINAS_TORMENTA[b];
            float a = AzarTormenta(minijuego, 0.0f, 6.2832f);
            float radio = b == 2 ? 1.1f : 0.62f;
            punta.x += std::cos(a) * radio;
            punta.z += std::sin(a) * radio;
            EmitirChispasTormenta(minijuego, punta, 4, 2.4f, Color{ 190, 170, 255, 255 });
        }
    }

    // Confeti sobre el ultimo que quedo en la plataforma.
    if (minijuego.fase == FASE_MAGNETICA_TERMINADO && !minijuego.confetiLanzado)
    {
        minijuego.confetiLanzado = true;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                minijuego.resultado.participantes[i].participo &&
                !minijuego.estadosJugadores[i].eliminado
            )
            {
                EmitirConfetiTormenta(minijuego, jugadores[i].posicion);
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
            }
        }
    }
}


//==================================================
// PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Plataforma de pruebas en plena tormenta electrica, de noche: chapa de
// acero mojada, cobre de las bobinas, ceramica de los aisladores, franjas
// de peligro amarillas y un cielo de pizarra azul que se ilumina con cada
// relampago. El acento es el color del campo (celeste atrae, magenta
// repele), reservado para el nucleo, sus ondas y la tira del borde.

static const Color COLOR_CIELO_ALTO_TORMENTA = { 14, 16, 32, 255 };
static const Color COLOR_CIELO_BAJO_TORMENTA = { 64, 68, 102, 255 };
static const Color COLOR_NIEBLA_TORMENTA = { 52, 58, 86, 255 };
static const Color COLOR_NIEBLA_CLARA_TORMENTA = { 132, 140, 186, 255 };
static const Color COLOR_NUBE_TORMENTA = { 54, 58, 86, 255 };
static const Color COLOR_NUBE_CLARA_TORMENTA = { 96, 102, 140, 255 };
static const Color COLOR_NUBE_OSCURA_TORMENTA = { 34, 36, 56, 255 };
static const Color COLOR_ACERO_TORMENTA = { 104, 114, 132, 255 };
static const Color COLOR_ACERO_OSCURO_TORMENTA = { 56, 62, 78, 255 };
static const Color COLOR_PLACA_TORMENTA = { 86, 94, 112, 255 };
static const Color COLOR_JUNTA_TORMENTA = { 34, 38, 50, 255 };
static const Color COLOR_COBRE_TORMENTA = { 204, 122, 66, 255 };
static const Color COLOR_COBRE_OSCURO_TORMENTA = { 118, 64, 38, 255 };
static const Color COLOR_CERAMICA_TORMENTA = { 222, 218, 206, 255 };
static const Color COLOR_PELIGRO_TORMENTA = { 226, 176, 40, 255 };
static const Color COLOR_PELIGRO_OSCURO_TORMENTA = { 36, 36, 42, 255 };
static const Color COLOR_HORMIGON_TORMENTA = { 106, 110, 122, 255 };
static const Color COLOR_ALUMINIO_TORMENTA = { 182, 190, 206, 255 };
static const Color COLOR_ROCA_TORMENTA = { 40, 42, 60, 255 };
static const Color COLOR_VENTANA_TORMENTA = { 255, 204, 128, 255 };

// Ojo de referencia para la niebla (la camara de juego).
static const Vector3 OJO_NIEBLA_TORMENTA = { 0.0f, 8.4f, 14.2f };

// Fogonazo actual del relampago (0..1). Se fija al empezar Dibujar; solo
// aclara colores.
static float destelloTormenta = 0.0f;


static unsigned char CanalTormenta(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoTormenta(Color color, float factor)
{
    return Color{
        CanalTormenta((float)color.r * factor),
        CanalTormenta((float)color.g * factor),
        CanalTormenta((float)color.b * factor),
        color.a
    };
}


static Color MezclaTormenta(Color a, Color b, float t)
{
    t = Limitar01VisualTormenta(t);

    return Color{
        CanalTormenta((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalTormenta((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalTormenta((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalTormenta((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


// Caras de arriba con la luz fria del cielo (mas fuerte con el
// relampago); las laterales, sombra azulada.
static Color SombraTormenta(Color color, float factor, bool arriba)
{
    float d = destelloTormenta;

    if (arriba)
    {
        factor *= 1.0f + 0.25f * d;
        return Color{
            CanalTormenta((float)color.r * factor * 0.97f),
            CanalTormenta((float)color.g * factor * 1.0f),
            CanalTormenta((float)color.b * factor * 1.06f),
            color.a
        };
    }

    factor *= 1.0f + 0.15f * d;
    return Color{
        CanalTormenta((float)color.r * factor * 0.92f),
        CanalTormenta((float)color.g * factor * 0.95f),
        CanalTormenta((float)color.b * factor * 1.05f),
        color.a
    };
}


// Luz falsa por normal: arriba 1.0, frente 0.78, izquierda 0.68,
// derecha 0.60, fondo 0.50.
static float LuzNormalTormenta(Vector3 n)
{
    float f = 0.64f + 0.36f * n.y + 0.14f * n.z - 0.04f * n.x;
    if (f < 0.42f) f = 0.42f;
    if (f > 1.0f) f = 1.0f;
    return f;
}


// Lo lejano se funde con la niebla de lluvia azul pizarra.
static float CantidadNieblaTormenta(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_TORMENTA.x;
    float dy = p.y - OJO_NIEBLA_TORMENTA.y;
    float dz = p.z - OJO_NIEBLA_TORMENTA.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 18.0f) / 95.0f;
    return n < 0.0f ? 0.0f : (n > 0.92f ? 0.92f : n);
}


static Color ColorNieblaTormenta()
{
    return MezclaTormenta(COLOR_NIEBLA_TORMENTA, COLOR_NIEBLA_CLARA_TORMENTA, destelloTormenta * 0.55f);
}


static Color TinteTormenta(Color color, Vector3 p)
{
    Color niebla = ColorNieblaTormenta();
    niebla.a = color.a;
    return MezclaTormenta(color, niebla, CantidadNieblaTormenta(p));
}


static Vector3 SumaTormenta(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaTormenta(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaTormenta(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }

static Vector3 CruzTormenta(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

static Vector3 NormalizarTormenta(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 1.0e-5f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static void VerticeTormenta(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadTormenta(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTormenta(a, color);
    VerticeTormenta(b, color);
    VerticeTormenta(c, color);
    VerticeTormenta(d, color);
}


// Caja alineada con caras sombreadas (arriba 1.0, frente 0.78,
// izquierda 0.70, derecha 0.62, fondo 0.54).
static void CuboTormenta(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadTormenta({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraTormenta(color, 1.0f, true));
    QuadTormenta({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraTormenta(color, 0.78f, false));
    QuadTormenta({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraTormenta(color, 0.54f, false));
    QuadTormenta({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraTormenta(color, 0.62f, false));
    QuadTormenta({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraTormenta(color, 0.70f, false));
    QuadTormenta({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, SombraTormenta(color, 0.42f, false));
    rlEnd();
}


static void CuboNieblaTormenta(Vector3 c, Vector3 t, Color color)
{
    CuboTormenta(c, t, TinteTormenta(color, c));
}


// Caja orientada (ex, ey, ez unitarios y ortogonales), luz por normal.
static void CajaOrientadaTormenta(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Vector3 t, Color color)
{
    Vector3 hx = EscalaTormenta(ex, t.x * 0.5f);
    Vector3 hy = EscalaTormenta(ey, t.y * 0.5f);
    Vector3 hz = EscalaTormenta(ez, t.z * 0.5f);

    auto P = [&](float sx, float sy, float sz) -> Vector3
    {
        return {
            c.x + hx.x * sx + hy.x * sy + hz.x * sz,
            c.y + hx.y * sx + hy.y * sy + hz.y * sz,
            c.z + hx.z * sx + hy.z * sy + hz.z * sz
        };
    };

    auto Cara = [&](Vector3 n) -> Color
    {
        float f = LuzNormalTormenta(n);
        return SombraTormenta(color, f, n.y > 0.7f);
    };

    Vector3 menosX = EscalaTormenta(ex, -1.0f);
    Vector3 menosY = EscalaTormenta(ey, -1.0f);
    Vector3 menosZ = EscalaTormenta(ez, -1.0f);

    rlBegin(RL_QUADS);
    QuadTormenta(P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), P(-1, 1, -1), Cara(ey));
    QuadTormenta(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), Cara(menosY));
    QuadTormenta(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), Cara(ez));
    QuadTormenta(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), Cara(menosZ));
    QuadTormenta(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), Cara(ex));
    QuadTormenta(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), Cara(menosX));
    rlEnd();
}


// Viga de seccion rectangular de "a" a "b".
static void VigaTormenta(Vector3 a, Vector3 b, float ancho, float alto, Color color)
{
    Vector3 ez = NormalizarTormenta(RestaTormenta(b, a));
    Vector3 arriba = std::fabs(ez.y) > 0.95f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    Vector3 ex = NormalizarTormenta(CruzTormenta(arriba, ez));
    Vector3 ey = CruzTormenta(ez, ex);
    Vector3 d = RestaTormenta(b, a);
    float largo = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    Vector3 c = EscalaTormenta(SumaTormenta(a, b), 0.5f);
    CajaOrientadaTormenta(c, ex, ey, ez, { ancho, alto, largo }, color);
}


// Cilindro o tronco de cono de "a" (radio r0) a "b" (r1), luz por normal.
static void CilindroTormenta(Vector3 a, Vector3 b, float r0, float r1, int lados, Color color, bool tapas = true)
{
    Vector3 eje = NormalizarTormenta(RestaTormenta(b, a));
    Vector3 ref = std::fabs(eje.y) > 0.95f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    Vector3 u = NormalizarTormenta(CruzTormenta(ref, eje));
    Vector3 v = CruzTormenta(eje, u);
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaTormenta(EscalaTormenta(u, std::cos(a0)), EscalaTormenta(v, std::sin(a0)));
        Vector3 d1 = SumaTormenta(EscalaTormenta(u, std::cos(a1)), EscalaTormenta(v, std::sin(a1)));
        Vector3 n = SumaTormenta(EscalaTormenta(u, std::cos(am)), EscalaTormenta(v, std::sin(am)));
        Color cara = SombraTormenta(color, LuzNormalTormenta(n), false);

        QuadTormenta(
            SumaTormenta(a, EscalaTormenta(d0, r0)),
            SumaTormenta(a, EscalaTormenta(d1, r0)),
            SumaTormenta(b, EscalaTormenta(d1, r1)),
            SumaTormenta(b, EscalaTormenta(d0, r1)),
            cara
        );

        if (tapas && r1 > 0.001f)
        {
            Color tapa = SombraTormenta(color, LuzNormalTormenta(eje), eje.y > 0.7f);
            QuadTormenta(b, SumaTormenta(b, EscalaTormenta(d1, r1)), SumaTormenta(b, EscalaTormenta(d0, r1)), b, tapa);
        }

        if (tapas && r0 > 0.001f)
        {
            Color tapa = SombraTormenta(color, LuzNormalTormenta(EscalaTormenta(eje, -1.0f)), false);
            QuadTormenta(a, SumaTormenta(a, EscalaTormenta(d0, r0)), SumaTormenta(a, EscalaTormenta(d1, r0)), a, tapa);
        }
    }

    rlEnd();
}


// Corona plana horizontal (r0..r1) a la altura c.y.
static void AnilloTormenta(Vector3 c, float r0, float r1, Color color, int lados = 32)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadTormenta(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Toro en el plano (u, v) con luz por normal (bobinas, jaula del nucleo).
static void ToroTormenta(Vector3 c, Vector3 u, Vector3 v, float radio, float grosor, Color color, int lados = 18, int seccion = 7)
{
    Vector3 w = CruzTormenta(u, v);

    rlBegin(RL_QUADS);

    for (int i = 0; i < lados; i++)
    {
        float a0 = 6.2832f * (float)i / (float)lados;
        float a1 = 6.2832f * (float)(i + 1) / (float)lados;

        for (int j = 0; j < seccion; j++)
        {
            float b0 = 6.2832f * (float)j / (float)seccion;
            float b1 = 6.2832f * (float)(j + 1) / (float)seccion;
            float aa[4] = { a0, a1, a1, a0 };
            float bb[4] = { b0, b0, b1, b1 };

            for (int q = 0; q < 4; q++)
            {
                Vector3 radial = SumaTormenta(EscalaTormenta(u, std::cos(aa[q])), EscalaTormenta(v, std::sin(aa[q])));
                Vector3 n = SumaTormenta(EscalaTormenta(radial, std::cos(bb[q])), EscalaTormenta(w, std::sin(bb[q])));
                Vector3 p = SumaTormenta(c, SumaTormenta(EscalaTormenta(radial, radio), EscalaTormenta(n, grosor)));
                VerticeTormenta(p, SombraTormenta(color, LuzNormalTormenta(n), n.y > 0.7f));
            }
        }
    }

    rlEnd();
}


static void ToroHorizontalTormenta(Vector3 c, float radio, float grosor, Color color, int lados = 18)
{
    ToroTormenta(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, grosor, color, lados, 7);
}


// Esfera achatada (nubes, cupulas): arriba mas clara, abajo oscura, con
// niebla por vertice.
static void ElipsoideTormenta(Vector3 c, float rx, float ry, float rz, Color claro, Color oscuro, int anillos = 5, int lados = 10)
{
    rlBegin(RL_QUADS);

    for (int a = 0; a < anillos; a++)
    {
        float t0 = -1.5708f + 3.1416f * (float)a / (float)anillos;
        float t1 = -1.5708f + 3.1416f * (float)(a + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float p0 = 6.2832f * (float)k / (float)lados;
            float p1 = 6.2832f * (float)(k + 1) / (float)lados;
            float tt[4] = { t0, t0, t1, t1 };
            float pp[4] = { p0, p1, p1, p0 };

            for (int q = 0; q < 4; q++)
            {
                float ct = std::cos(tt[q]);
                float st = std::sin(tt[q]);
                float nx = ct * std::cos(pp[q]);
                float nz = ct * std::sin(pp[q]);
                Vector3 p = { c.x + nx * rx, c.y + st * ry, c.z + nz * rz };
                float luz = Limitar01VisualTormenta(0.5f + 0.5f * st + 0.15f * nz);
                Color cv = MezclaTormenta(oscuro, claro, luz);
                cv = SombraTormenta(cv, 1.0f, st > 0.3f);
                VerticeTormenta(p, TinteTormenta(cv, p));
            }
        }
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorTormenta(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloTormenta(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorTormenta(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano inclinado como la vista).
static void ResplandorFrenteTormenta(Vector3 c, float radio, Color color)
{
    ResplandorTormenta(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.92f, 0.40f }, radio, color);
}


// Cinta que mira a la camara entre dos puntos (rayos, arcos, lluvia).
static void CintaTormenta(Vector3 a, Vector3 b, float ancho, Color color)
{
    Vector3 dir = RestaTormenta(b, a);
    Vector3 medio = EscalaTormenta(SumaTormenta(a, b), 0.5f);
    Vector3 haciaOjo = RestaTormenta(OJO_NIEBLA_TORMENTA, medio);
    Vector3 lado = EscalaTormenta(NormalizarTormenta(CruzTormenta(dir, haciaOjo)), ancho * 0.5f);

    QuadTormenta(
        RestaTormenta(a, lado),
        SumaTormenta(a, lado),
        SumaTormenta(b, lado),
        RestaTormenta(b, lado),
        color
    );
}


// Rayo quebrado de "a" a "b" con ramas. Dibujar en la pasada aditiva.
static void RayoTormenta(Vector3 a, Vector3 b, int semilla, int segmentos, float desvio, float ancho, float intensidad, bool ramas = true)
{
    if (intensidad <= 0.01f) return;

    const int MAX_PUNTOS = 24;
    if (segmentos > MAX_PUNTOS - 1) segmentos = MAX_PUNTOS - 1;

    Vector3 puntos[MAX_PUNTOS];

    for (int i = 0; i <= segmentos; i++)
    {
        float u = (float)i / (float)segmentos;
        float envolvente = std::sin(u * 3.1416f);
        puntos[i] =
        {
            a.x + (b.x - a.x) * u + (Hash01Tormenta(semilla, i * 3) - 0.5f) * 2.0f * desvio * envolvente,
            a.y + (b.y - a.y) * u,
            a.z + (b.z - a.z) * u + (Hash01Tormenta(semilla, i * 3 + 1) - 0.5f) * 2.0f * desvio * envolvente
        };
    }

    unsigned char alfaHalo = CanalTormenta(70.0f * intensidad);
    unsigned char alfaNucleo = CanalTormenta(255.0f * intensidad);
    Color halo = { COLOR_RAYO_TORMENTA.r, COLOR_RAYO_TORMENTA.g, COLOR_RAYO_TORMENTA.b, alfaHalo };
    Color nucleo = { 236, 240, 255, alfaNucleo };

    rlBegin(RL_QUADS);

    for (int i = 0; i < segmentos; i++)
    {
        CintaTormenta(puntos[i], puntos[i + 1], ancho * 5.0f, halo);
        CintaTormenta(puntos[i], puntos[i + 1], ancho, nucleo);
    }

    if (ramas)
    {
        for (int r = 0; r < 2; r++)
        {
            int desde = 2 + (int)(Hash01Tormenta(semilla, 50 + r) * (float)(segmentos - 4));
            Vector3 p = puntos[desde];
            float largo = std::fabs(b.y - a.y) / (float)segmentos;
            float lado = Hash01Tormenta(semilla, 60 + r) > 0.5f ? 1.0f : -1.0f;

            for (int k = 0; k < 4; k++)
            {
                Vector3 q =
                {
                    p.x + lado * largo * (0.5f + Hash01Tormenta(semilla, 70 + r * 8 + k)),
                    p.y - largo * (0.7f + 0.5f * Hash01Tormenta(semilla, 80 + r * 8 + k)),
                    p.z + (Hash01Tormenta(semilla, 90 + r * 8 + k) - 0.5f) * largo
                };
                Color c = nucleo;
                c.a = CanalTormenta((float)alfaNucleo * (0.6f - 0.12f * (float)k));
                CintaTormenta(p, q, ancho * 0.6f, c);
                p = q;
            }
        }
    }

    rlEnd();
}


//==================================================
// RELAMPAGOS
//==================================================

struct RelampagoTormenta
{
    float intensidad = 0.0f;
    int semilla = 0;
    Vector3 arriba{};
    Vector3 abajo{};
};


// Parpadeo tipico: fogonazo, apagon, segundo fogonazo y cola.
static float ParpadeoRelampagoTormenta(float u, float duracion)
{
    if (u < 0.0f || u > duracion) return 0.0f;
    if (u < 0.05f) return 1.0f;
    if (u < 0.10f) return 0.3f;
    if (u < 0.17f) return 0.85f;
    return 0.5f * (duracion - u) / (duracion - 0.17f);
}


// Relampagos de fondo: uno cada ~3,6 s en un lugar distinto (todo sale
// del reloj visual, sin estado). Uno de cada cuatro cae en el pararrayos
// de la torre de la bobina gigante.
static RelampagoTormenta RelampagoFondoTormenta(float t)
{
    RelampagoTormenta r;
    const float PERIODO = 3.6f;
    int k = (int)std::floor(t / PERIODO);
    float inicio = (float)k * PERIODO + 0.4f + Hash01Tormenta(k, 1) * 2.4f;
    float u = t - inicio;

    r.intensidad = ParpadeoRelampagoTormenta(u, 0.5f);
    r.semilla = k * 31 + 7;

    if (Hash01Tormenta(k, 4) < 0.25f)
    {
        r.arriba = { 15.0f, 30.0f, -24.0f };
        r.abajo = { 11.8f, 7.9f, -8.2f };
    }
    else
    {
        float x = -70.0f + 140.0f * Hash01Tormenta(k, 2);
        float z = -60.0f - 70.0f * Hash01Tormenta(k, 3);
        r.arriba = { x + 6.0f, 34.0f, z - 10.0f };
        r.abajo = { x, NIVEL_NUBES_TORMENTA, z };
    }

    return r;
}


// Rayo que cae sobre el nucleo nuevo cada vez que el campo se invierte.
static RelampagoTormenta RelampagoCampoTormenta(const MinijuegoTormentaMagnetica& m)
{
    RelampagoTormenta r;
    if (m.fase == FASE_MAGNETICA_PREPARACION) return r;

    r.intensidad = ParpadeoRelampagoTormenta(m.tiempoDesdeCambio, 0.45f);
    r.semilla = m.cambiosCampo * 17 + 3;
    r.abajo = { m.posicionNucleo.x, m.posicionNucleo.y + 0.5f, m.posicionNucleo.z };
    r.arriba = { m.posicionNucleo.x + 3.0f, 24.0f, m.posicionNucleo.z - 10.0f };
    return r;
}


//==================================================
// CIELO Y HORIZONTE
//==================================================

// Cielo de tormenta nocturna (2D, antes del 3D): pizarra casi negra
// arriba que se aclara hacia el horizonte; el relampago lo enciende.
static void DibujarCieloTormenta(const Camera3D& vista, float t)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float d = destelloTormenta;

    float horizonte = GetWorldToScreen({ 0.0f, OJO_NIEBLA_TORMENTA.y, -3000.0f }, vista).y;
    if (horizonte < (float)alto * 0.04f) horizonte = (float)alto * 0.04f;
    if (horizonte > (float)alto * 0.6f) horizonte = (float)alto * 0.6f;

    Color alto0 = MezclaTormenta(COLOR_CIELO_ALTO_TORMENTA, Color{ 70, 76, 120, 255 }, d * 0.6f);
    Color bajo0 = MezclaTormenta(COLOR_CIELO_BAJO_TORMENTA, Color{ 170, 178, 220, 255 }, d * 0.6f);
    Color niebla = ColorNieblaTormenta();

    DrawRectangleGradientV(0, 0, ancho, (int)horizonte, alto0, bajo0);
    DrawRectangleGradientV(0, (int)horizonte, ancho, alto - (int)horizonte, bajo0, niebla);

    // Jirones de nube oscura que cruzan rapido por la franja de cielo.
    for (int k = 0; k < 7; k++)
    {
        float vel = 18.0f + 14.0f * Hash01Tormenta(k, 20);
        float x = std::fmod(Hash01Tormenta(k, 21) * (float)(ancho + 700) + t * vel, (float)(ancho + 700)) - 350.0f;
        float y = horizonte * (0.15f + 0.8f * Hash01Tormenta(k, 22));
        float largo = 180.0f + 220.0f * Hash01Tormenta(k, 23);
        Color c = MezclaTormenta(COLOR_NUBE_OSCURA_TORMENTA, COLOR_NUBE_CLARA_TORMENTA, d * 0.7f);
        DrawEllipse((int)x, (int)y, largo, 6.0f + 6.0f * Hash01Tormenta(k, 24), Fade(c, 0.75f));
    }
}


// Mar de nubes bajo la torre: malla con niebla por vertice, manchas
// claras y oscuras, y luz desde adentro donde cae un relampago.
static void DibujarMarNubesTormenta(float t, const RelampagoTormenta& fondo)
{
    const int CX = 18;
    const int CZ = 12;
    const float X0 = -230.0f, X1 = 230.0f;
    const float Z0 = -270.0f, Z1 = 30.0f;
    float y = NIVEL_NUBES_TORMENTA;

    auto Color0 = [&](int i, int j) -> Color
    {
        float x = X0 + (X1 - X0) * (float)i / (float)CX;
        float z = Z0 + (Z1 - Z0) * (float)j / (float)CZ;
        float mancha = Hash01Tormenta(i * 3 + 1, j * 5 + 2);
        Color c = MezclaTormenta(COLOR_NUBE_OSCURA_TORMENTA, COLOR_NUBE_TORMENTA, 0.35f + 0.65f * mancha);

        if (fondo.intensidad > 0.0f)
        {
            float dx = x - fondo.abajo.x;
            float dz = z - fondo.abajo.z;
            float cerca = 1.0f - std::sqrt(dx * dx + dz * dz) / 60.0f;
            if (cerca > 0.0f) c = MezclaTormenta(c, COLOR_NUBE_CLARA_TORMENTA, cerca * fondo.intensidad);
        }

        return SombraTormenta(c, 1.0f, true);
    };

    rlBegin(RL_QUADS);

    for (int j = 0; j < CZ; j++)
    {
        for (int i = 0; i < CX; i++)
        {
            float xa = X0 + (X1 - X0) * (float)i / (float)CX;
            float xb = X0 + (X1 - X0) * (float)(i + 1) / (float)CX;
            float za = Z0 + (Z1 - Z0) * (float)j / (float)CZ;
            float zb = Z0 + (Z1 - Z0) * (float)(j + 1) / (float)CZ;
            Vector3 a = { xa, y, zb }, b = { xb, y, zb }, c = { xb, y, za }, d = { xa, y, za };
            VerticeTormenta(a, TinteTormenta(Color0(i, j + 1), a));
            VerticeTormenta(b, TinteTormenta(Color0(i + 1, j + 1), b));
            VerticeTormenta(c, TinteTormenta(Color0(i + 1, j), c));
            VerticeTormenta(d, TinteTormenta(Color0(i, j), d));
        }
    }

    rlEnd();

    // Lomos de nube que derivan despacio con el viento.
    int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 44 : (CalidadDecoracion() == CALIDAD_MEDIA ? 22 : 0);

    for (int k = 0; k < cantidad; k++)
    {
        float deriva = std::fmod(Hash01Tormenta(k, 30) * 160.0f + t * (0.8f + 0.6f * Hash01Tormenta(k, 31)), 160.0f) - 80.0f;
        float z = 14.0f - 150.0f * Hash01Tormenta(k, 32) * Hash01Tormenta(k, 33) - 6.0f;
        float rx = 7.0f + 9.0f * Hash01Tormenta(k, 34);
        float ry = 1.6f + 2.0f * Hash01Tormenta(k, 35);
        float rz = 4.0f + 5.0f * Hash01Tormenta(k, 36);
        Color claro = COLOR_NUBE_CLARA_TORMENTA;

        if (fondo.intensidad > 0.0f)
        {
            float dx = deriva - fondo.abajo.x;
            float dz = z - fondo.abajo.z;
            float cerca = 1.0f - std::sqrt(dx * dx + dz * dz) / 50.0f;
            if (cerca > 0.0f) claro = MezclaTormenta(claro, Color{ 200, 206, 240, 255 }, cerca * fondo.intensidad);
        }

        ElipsoideTormenta({ deriva, y + 0.2f, z }, rx, ry, rz, claro, COLOR_NUBE_OSCURA_TORMENTA, 4, 10);
    }
}


// Cordillera que asoma sobre las nubes: picos facetados en dos planos.
// "niebla" fija cuanto se funde con el aire: el plano cercano queda
// oscuro y el lejano mas claro, como dos siluetas.
static void PicoTormenta(Vector3 base, float radio, float alto, Color color, int semilla, float niebla)
{
    const int LADOS = 6;
    float giro = Hash01Tormenta(semilla, 1) * 1.0f;
    Vector3 cima =
    {
        base.x + (Hash01Tormenta(semilla, 2) - 0.5f) * radio * 0.5f,
        base.y + alto,
        base.z + (Hash01Tormenta(semilla, 3) - 0.5f) * radio * 0.3f
    };

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = giro + 6.2832f * (float)k / (float)LADOS;
        float a1 = giro + 6.2832f * (float)(k + 1) / (float)LADOS;
        float r0 = radio * (0.75f + 0.35f * Hash01Tormenta(semilla, 10 + k));
        float r1 = radio * (0.75f + 0.35f * Hash01Tormenta(semilla, 10 + (k + 1) % LADOS));
        Vector3 p0 = { base.x + std::cos(a0) * r0, base.y, base.z + std::sin(a0) * r0 };
        Vector3 p1 = { base.x + std::cos(a1) * r1, base.y, base.z + std::sin(a1) * r1 };

        Vector3 n = NormalizarTormenta(CruzTormenta(RestaTormenta(p1, p0), RestaTormenta(cima, p0)));
        if (n.y < 0.0f) n = EscalaTormenta(n, -1.0f);
        Color cara = SombraTormenta(color, LuzNormalTormenta(n) * (0.9f + 0.2f * Hash01Tormenta(semilla, 20 + k)), false);

        VerticeTormenta(p0, MezclaTormenta(cara, ColorNieblaTormenta(), niebla));
        VerticeTormenta(p1, MezclaTormenta(cara, ColorNieblaTormenta(), niebla));
        VerticeTormenta(cima, MezclaTormenta(SombraTormenta(color, 1.1f, true), ColorNieblaTormenta(), niebla));
    }

    rlEnd();
}


static void DibujarHorizonteTormenta(const RelampagoTormenta& fondo)
{
    // Plano lejano: picos altos, casi fundidos con la niebla.
    for (int k = 0; k < 16; k++)
    {
        float x = -210.0f + 28.0f * (float)k + 10.0f * Hash01Tormenta(k, 40);
        float alto = 18.0f + 16.0f * Hash01Tormenta(k, 41);
        PicoTormenta({ x, NIVEL_NUBES_TORMENTA - 2.0f, -185.0f - 20.0f * Hash01Tormenta(k, 42) }, 16.0f + 8.0f * Hash01Tormenta(k, 43), alto, Color{ 58, 60, 86, 255 }, k + 100, 0.55f);
    }

    // Plano cercano: crestas mas oscuras y bajas.
    for (int k = 0; k < 14; k++)
    {
        float x = -150.0f + 23.0f * (float)k + 8.0f * Hash01Tormenta(k, 50);
        if (std::fabs(x) < 18.0f) continue;
        float alto = 10.0f + 12.0f * Hash01Tormenta(k, 51);
        PicoTormenta({ x, NIVEL_NUBES_TORMENTA - 2.0f, -112.0f - 14.0f * Hash01Tormenta(k, 52) }, 11.0f + 6.0f * Hash01Tormenta(k, 53), alto, COLOR_ROCA_TORMENTA, k + 200, 0.25f);
    }

    // Cumulonimbos lejanos: torres de nube que suben fuera de cuadro.
    for (int k = 0; k < 7; k++)
    {
        float x = -180.0f + 60.0f * (float)k + 20.0f * Hash01Tormenta(k, 60);
        float z = -230.0f - 20.0f * Hash01Tormenta(k, 61);
        Color claro = COLOR_NUBE_CLARA_TORMENTA;

        if (fondo.intensidad > 0.0f)
        {
            float cerca = 1.0f - std::fabs(x - fondo.abajo.x) / 90.0f;
            if (cerca > 0.0f) claro = MezclaTormenta(claro, Color{ 210, 214, 250, 255 }, cerca * fondo.intensidad);
        }

        for (int b = 0; b < 3; b++)
        {
            float r = (26.0f - 5.0f * (float)b) * (0.8f + 0.4f * Hash01Tormenta(k, 62 + b));
            ElipsoideTormenta({ x + (Hash01Tormenta(k, 70 + b) - 0.5f) * 14.0f, NIVEL_NUBES_TORMENTA + 6.0f + 16.0f * (float)b, z }, r, r * 0.8f, r * 0.6f, claro, COLOR_NUBE_OSCURA_TORMENTA, 5, 10);
        }
    }
}


//==================================================
// UTILERIA
//==================================================

// Bobina Tesla: pedestal, primario de cobre, secundario bobinado y toroide
// de aluminio arriba (centro del toroide en base.y + 3.05 * escala).
// MODELO FUTURO: bobina Tesla de laboratorio (pedestal, primario, toroide).
static void BobinaTeslaTormenta(Vector3 base, float s)
{
    CuboTormenta({ base.x, base.y + 0.22f * s, base.z }, { 1.3f * s, 0.44f * s, 1.3f * s }, COLOR_ACERO_OSCURO_TORMENTA);
    CuboTormenta({ base.x, base.y + 0.46f * s, base.z }, { 1.1f * s, 0.06f * s, 1.1f * s }, COLOR_PELIGRO_TORMENTA);

    // Cuatro aisladores bajo el primario.
    for (int k = 0; k < 4; k++)
    {
        float a = 0.785f + 1.5708f * (float)k;
        Vector3 p = { base.x + std::cos(a) * 0.42f * s, base.y + 0.49f * s, base.z + std::sin(a) * 0.42f * s };
        CilindroTormenta(p, { p.x, p.y + 0.16f * s, p.z }, 0.06f * s, 0.06f * s, 6, COLOR_CERAMICA_TORMENTA);
    }

    // Primario: espiral plana de cobre.
    float yPrimario = base.y + 0.66f * s;
    for (int r = 0; r < 3; r++)
    {
        float radio = (0.34f + 0.11f * (float)r) * s;
        ToroHorizontalTormenta({ base.x, yPrimario, base.z }, radio, 0.025f * s, r % 2 == 0 ? COLOR_COBRE_TORMENTA : COLOR_COBRE_OSCURO_TORMENTA, 16);
    }

    // Secundario: tubo con bandas de bobinado.
    float y0 = base.y + 0.62f * s;
    float y1 = base.y + 2.72f * s;
    const int BANDAS = 12;
    for (int b = 0; b < BANDAS; b++)
    {
        float ya = y0 + (y1 - y0) * (float)b / (float)BANDAS;
        float yb = y0 + (y1 - y0) * (float)(b + 1) / (float)BANDAS;
        Color c = b % 2 == 0 ? COLOR_COBRE_TORMENTA : TonoTormenta(COLOR_COBRE_TORMENTA, 0.78f);
        CilindroTormenta({ base.x, ya, base.z }, { base.x, yb, base.z }, 0.2f * s, 0.2f * s, 10, c, false);
    }

    CilindroTormenta({ base.x, y1, base.z }, { base.x, y1 + 0.16f * s, base.z }, 0.13f * s, 0.1f * s, 8, COLOR_ACERO_TORMENTA);
    ToroHorizontalTormenta({ base.x, base.y + 3.05f * s, base.z }, 0.62f * s, 0.2f * s, COLOR_ALUMINIO_TORMENTA, 20);
    CilindroTormenta({ base.x, base.y + 2.9f * s, base.z }, { base.x, base.y + 3.2f * s, base.z }, 0.42f * s, 0.42f * s, 12, COLOR_ALUMINIO_TORMENTA);
    CilindroTormenta({ base.x, base.y + 3.2f * s, base.z }, { base.x, base.y + 3.55f * s, base.z }, 0.05f * s, 0.0f, 6, COLOR_ACERO_TORMENTA);
}


// Pararrayos: mastil de celosia triangular con punta de cobre y corona.
// MODELO FUTURO: pararrayos de celosia con punta Franklin.
static void PararrayosTormenta(Vector3 base, float alto)
{
    const float R = 0.16f;
    Vector3 patas[3];

    for (int k = 0; k < 3; k++)
    {
        float a = 1.5708f + 2.0944f * (float)k;
        patas[k] = { base.x + std::cos(a) * R, base.y, base.z + std::sin(a) * R };
        CuboTormenta({ patas[k].x, base.y + alto * 0.5f, patas[k].z }, { 0.05f, alto, 0.05f }, COLOR_ACERO_TORMENTA);
    }

    // Travesanos en zigzag.
    int tramos = (int)(alto / 0.55f);
    for (int t = 0; t < tramos; t++)
    {
        float ya = base.y + 0.55f * (float)t;
        float yb = ya + 0.55f;
        for (int k = 0; k < 3; k++)
        {
            Vector3 a = patas[k];
            Vector3 b = patas[(k + 1) % 3];
            VigaTormenta({ a.x, ya, a.z }, { b.x, yb, b.z }, 0.03f, 0.03f, COLOR_ACERO_OSCURO_TORMENTA);
        }
    }

    float cima = base.y + alto;
    CilindroTormenta({ base.x, cima, base.z }, { base.x, cima + 0.12f, base.z }, 0.2f, 0.2f, 8, COLOR_ACERO_OSCURO_TORMENTA);
    CilindroTormenta({ base.x, cima + 0.12f, base.z }, { base.x, cima + 0.95f, base.z }, 0.05f, 0.02f, 6, COLOR_COBRE_TORMENTA);
    DrawSphereEx({ base.x, cima + 0.55f, base.z }, 0.11f, 6, 8, SombraTormenta(COLOR_COBRE_TORMENTA, 0.95f, true));

    for (int k = 0; k < 4; k++)
    {
        float a = 0.785f * (float)(2 * k + 1);
        Vector3 desde = { base.x, cima + 0.18f, base.z };
        Vector3 hasta = { base.x + std::cos(a) * 0.32f, cima + 0.42f, base.z + std::sin(a) * 0.32f };
        CilindroTormenta(desde, hasta, 0.02f, 0.01f, 4, COLOR_COBRE_TORMENTA, false);
    }
}


// Mastil con dos focos orientados hacia "objetivo" (luz con charco en la
// pasada aditiva).
// MODELO FUTURO: torre de focos de estadio con dos proyectores.
static void MastilFocosTormenta(Vector3 base, float alto, Vector3 objetivo)
{
    CilindroTormenta(base, { base.x, base.y + alto, base.z }, 0.11f, 0.08f, 8, COLOR_ACERO_TORMENTA);
    CuboTormenta({ base.x, base.y + 0.3f, base.z }, { 0.5f, 0.6f, 0.5f }, COLOR_ACERO_OSCURO_TORMENTA);

    float cima = base.y + alto;
    Vector3 hacia = NormalizarTormenta(RestaTormenta(objetivo, { base.x, cima, base.z }));
    Vector3 lado = NormalizarTormenta(CruzTormenta({ 0.0f, 1.0f, 0.0f }, hacia));
    Vector3 arriba = CruzTormenta(hacia, lado);

    CuboTormenta({ base.x, cima, base.z }, { 0.9f, 0.08f, 0.12f }, COLOR_ACERO_OSCURO_TORMENTA);

    for (int k = -1; k <= 1; k += 2)
    {
        Vector3 c = SumaTormenta({ base.x, cima + 0.12f, base.z }, EscalaTormenta(lado, 0.32f * (float)k));
        CajaOrientadaTormenta(c, lado, arriba, hacia, { 0.36f, 0.3f, 0.34f }, COLOR_ACERO_OSCURO_TORMENTA);
        Vector3 lente = SumaTormenta(c, EscalaTormenta(hacia, 0.175f));
        CajaOrientadaTormenta(lente, lado, arriba, hacia, { 0.3f, 0.24f, 0.01f }, Color{ 236, 242, 255, 255 });
    }
}


// Banco de condensadores: latas con aislador arriba y barra de cobre.
// MODELO FUTURO: banco de condensadores de alta tension.
static void CondensadoresTormenta(Vector3 base, Vector3 eje, int cantidad)
{
    CuboTormenta(
        { base.x + eje.x * 0.55f * (float)(cantidad - 1) * 0.5f, base.y + 0.06f, base.z + eje.z * 0.55f * (float)(cantidad - 1) * 0.5f },
        { std::fabs(eje.x) * 0.55f * (float)cantidad + 0.5f, 0.12f, std::fabs(eje.z) * 0.55f * (float)cantidad + 0.5f },
        COLOR_ACERO_OSCURO_TORMENTA
    );

    Vector3 anterior{};

    for (int k = 0; k < cantidad; k++)
    {
        Vector3 p = { base.x + eje.x * 0.55f * (float)k, base.y + 0.12f, base.z + eje.z * 0.55f * (float)k };
        CilindroTormenta(p, { p.x, p.y + 0.8f, p.z }, 0.2f, 0.2f, 10, Color{ 92, 112, 146, 255 });
        CilindroTormenta({ p.x, p.y + 0.8f, p.z }, { p.x, p.y + 0.86f, p.z }, 0.2f, 0.16f, 10, COLOR_ACERO_TORMENTA);

        for (int d = 0; d < 3; d++)
        {
            float y = p.y + 0.9f + 0.09f * (float)d;
            CilindroTormenta({ p.x, y, p.z }, { p.x, y + 0.05f, p.z }, 0.09f, 0.09f, 8, COLOR_CERAMICA_TORMENTA);
        }

        Vector3 borne = { p.x, p.y + 1.18f, p.z };
        if (k > 0) VigaTormenta(anterior, borne, 0.05f, 0.03f, COLOR_COBRE_TORMENTA);
        anterior = borne;
    }
}


// Columna de discos de ceramica (aislador de alta tension).
static void AisladorTormenta(Vector3 base, int discos, float escala)
{
    CilindroTormenta(base, { base.x, base.y + 0.07f * escala * (float)discos + 0.1f * escala, base.z }, 0.04f * escala, 0.04f * escala, 6, COLOR_ACERO_TORMENTA);

    for (int d = 0; d < discos; d++)
    {
        float y = base.y + 0.08f * escala + 0.07f * escala * (float)d;
        CilindroTormenta({ base.x, y, base.z }, { base.x, y + 0.035f * escala, base.z }, 0.13f * escala, 0.1f * escala, 10, COLOR_CERAMICA_TORMENTA);
    }
}


// Carrete de cable acostado.
static void CarreteCableTormenta(Vector3 base, float giro)
{
    Vector3 eje = { std::cos(giro), 0.0f, std::sin(giro) };
    Vector3 c = { base.x, base.y + 0.5f, base.z };
    Vector3 a = SumaTormenta(c, EscalaTormenta(eje, -0.3f));
    Vector3 b = SumaTormenta(c, EscalaTormenta(eje, 0.3f));

    CilindroTormenta(a, SumaTormenta(a, EscalaTormenta(eje, 0.07f)), 0.5f, 0.5f, 12, Color{ 126, 92, 60, 255 });
    CilindroTormenta(SumaTormenta(b, EscalaTormenta(eje, -0.07f)), b, 0.5f, 0.5f, 12, Color{ 126, 92, 60, 255 });
    CilindroTormenta(a, b, 0.36f, 0.36f, 12, Color{ 40, 40, 46, 255 }, false);
}


// Gabinete electrico con dial y cartel de peligro.
static void GabineteTormenta(Vector3 base, float giro)
{
    Vector3 ez = { std::sin(giro), 0.0f, std::cos(giro) };
    Vector3 ex = { std::cos(giro), 0.0f, -std::sin(giro) };
    Vector3 ey = { 0.0f, 1.0f, 0.0f };
    Vector3 c = { base.x, base.y + 0.55f, base.z };

    CajaOrientadaTormenta(c, ex, ey, ez, { 0.8f, 1.1f, 0.4f }, Color{ 128, 136, 120, 255 });
    CajaOrientadaTormenta({ c.x, c.y + 0.6f, c.z }, ex, ey, ez, { 0.9f, 0.08f, 0.5f }, COLOR_ACERO_OSCURO_TORMENTA);

    Vector3 frente = SumaTormenta(c, EscalaTormenta(ez, 0.205f));
    CajaOrientadaTormenta(SumaTormenta(frente, { 0.0f, 0.22f, 0.0f }), ex, ey, ez, { 0.3f, 0.3f, 0.02f }, Color{ 230, 230, 220, 255 });
    CajaOrientadaTormenta(SumaTormenta(frente, { 0.0f, -0.18f, 0.0f }), ex, ey, ez, { 0.36f, 0.3f, 0.02f }, COLOR_PELIGRO_TORMENTA);
}


// Baliza giratoria ambar (el haz va en la pasada aditiva).
static void BalizaTormenta(Vector3 base)
{
    CilindroTormenta(base, { base.x, base.y + 0.12f, base.z }, 0.14f, 0.14f, 8, COLOR_ACERO_OSCURO_TORMENTA);
    CilindroTormenta({ base.x, base.y + 0.12f, base.z }, { base.x, base.y + 0.34f, base.z }, 0.11f, 0.08f, 8, Color{ 255, 150, 50, 255 });
}


// Anemometro: tres cazoletas que giran y veleta.
static void AnemometroTormenta(Vector3 base, float alto, float t)
{
    CilindroTormenta(base, { base.x, base.y + alto, base.z }, 0.06f, 0.04f, 6, COLOR_ACERO_TORMENTA);
    float cima = base.y + alto;
    float giro = t * 7.0f;

    for (int k = 0; k < 3; k++)
    {
        float a = giro + 2.0944f * (float)k;
        Vector3 punta = { base.x + std::cos(a) * 0.38f, cima + 0.05f, base.z + std::sin(a) * 0.38f };
        VigaTormenta({ base.x, cima + 0.05f, base.z }, punta, 0.025f, 0.025f, COLOR_ACERO_TORMENTA);
        DrawSphereEx(punta, 0.08f, 5, 6, SombraTormenta(COLOR_ALUMINIO_TORMENTA, 0.9f, true));
    }

    float veleta = 2.4f + std::sin(t * 1.3f) * 0.3f;
    Vector3 cola = { base.x + std::cos(veleta) * 0.4f, cima - 0.25f, base.z + std::sin(veleta) * 0.4f };
    VigaTormenta({ base.x, cima - 0.25f, base.z }, cola, 0.03f, 0.18f, COLOR_PELIGRO_TORMENTA);
}


// Cable colgado entre dos puntos (catenaria aproximada).
static void CableTormenta(Vector3 a, Vector3 b, float comba, float grosor, Color color)
{
    const int TRAMOS = 10;
    Vector3 anterior = a;

    for (int k = 1; k <= TRAMOS; k++)
    {
        float u = (float)k / (float)TRAMOS;
        Vector3 p =
        {
            a.x + (b.x - a.x) * u,
            a.y + (b.y - a.y) * u - comba * 4.0f * u * (1.0f - u),
            a.z + (b.z - a.z) * u
        };
        VigaTormenta(anterior, p, grosor, grosor, color);
        anterior = p;
    }
}


// Baranda de tubo entre dos esquinas.
static void BarandaTormenta(Vector3 a, Vector3 b, float alto)
{
    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);
    int postes = (int)(largo / 0.9f) + 1;

    for (int k = 0; k <= postes; k++)
    {
        float u = (float)k / (float)postes;
        Vector3 p = { a.x + dx * u, a.y, a.z + dz * u };
        CuboTormenta({ p.x, p.y + alto * 0.5f, p.z }, { 0.05f, alto, 0.05f }, COLOR_PELIGRO_TORMENTA);
    }

    VigaTormenta({ a.x, a.y + alto, a.z }, { b.x, b.y + alto, b.z }, 0.05f, 0.05f, COLOR_PELIGRO_TORMENTA);
    VigaTormenta({ a.x, a.y + alto * 0.5f, a.z }, { b.x, b.y + alto * 0.5f, b.z }, 0.035f, 0.035f, COLOR_PELIGRO_TORMENTA);
}


// Franjas diagonales de peligro sobre un plano: "eje" corre a lo largo
// de la banda, "ancho" la cruza (con su largo).
static void FranjasPeligroTormenta(Vector3 origen, Vector3 eje, Vector3 ancho, float largo, float paso, float luz, bool arriba)
{
    Color oscuro = SombraTormenta(COLOR_PELIGRO_OSCURO_TORMENTA, luz, arriba);
    Color amarillo = SombraTormenta(COLOR_PELIGRO_TORMENTA, luz, arriba);
    float desfase = std::sqrt(ancho.x * ancho.x + ancho.y * ancho.y + ancho.z * ancho.z);

    auto P = [&](float s, float a) -> Vector3
    {
        return {
            origen.x + eje.x * s + ancho.x * a,
            origen.y + eje.y * s + ancho.y * a,
            origen.z + eje.z * s + ancho.z * a
        };
    };

    rlBegin(RL_QUADS);
    QuadTormenta(P(0.0f, 0.0f), P(largo, 0.0f), P(largo, 1.0f), P(0.0f, 1.0f), oscuro);

    for (float s = -desfase; s < largo; s += paso * 2.0f)
    {
        float a0 = s, a1 = s + paso;
        float b0 = s + desfase, b1 = s + paso + desfase;
        if (a1 < 0.0f && b1 < 0.0f) continue;

        auto Clamp = [&](float v) { return v < 0.0f ? 0.0f : (v > largo ? largo : v); };
        QuadTormenta(P(Clamp(a0), 0.0f), P(Clamp(a1), 0.0f), P(Clamp(b1), 1.0f), P(Clamp(b0), 1.0f), amarillo);
    }

    rlEnd();
}


//==================================================
// MEDIA DISTANCIA: TORRES
//==================================================

// Torre de control a la izquierda: fuste de hormigon que sale de las
// nubes, cabina con ventanales encendidos, antena con baliza roja.
static const Vector3 TORRE_CONTROL_TORMENTA = { -11.8f, 0.2f, -6.8f };

static void TorreControlTormenta()
{
    Vector3 b = TORRE_CONTROL_TORMENTA;

    CuboNieblaTormenta({ b.x, (NIVEL_NUBES_TORMENTA + b.y) * 0.5f, b.z }, { 2.4f, b.y - NIVEL_NUBES_TORMENTA, 2.4f }, COLOR_HORMIGON_TORMENTA);

    for (int k = 1; k < 5; k++)
    {
        float y = b.y - 3.2f * (float)k;
        CuboNieblaTormenta({ b.x, y, b.z }, { 2.6f, 0.22f, 2.6f }, TonoTormenta(COLOR_HORMIGON_TORMENTA, 0.85f));
    }

    // Escalera de servicio por la cara que mira al centro.
    for (int k = 0; k < 18; k++)
    {
        float y = b.y - 0.4f - 0.45f * (float)k;
        CuboTormenta({ b.x + 1.26f, y, b.z + 0.6f }, { 0.04f, 0.04f, 0.5f }, COLOR_ACERO_TORMENTA);
    }
    CuboTormenta({ b.x + 1.26f, b.y - 4.2f, b.z + 0.35f }, { 0.05f, 8.0f, 0.05f }, COLOR_ACERO_TORMENTA);
    CuboTormenta({ b.x + 1.26f, b.y - 4.2f, b.z + 0.85f }, { 0.05f, 8.0f, 0.05f }, COLOR_ACERO_TORMENTA);

    // Losa, cabina y techo.
    CuboTormenta({ b.x, b.y + 0.2f, b.z }, { 4.6f, 0.4f, 4.2f }, COLOR_ACERO_OSCURO_TORMENTA);
    FranjasPeligroTormenta({ b.x - 2.3f, b.y + 0.21f, b.z + 2.101f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, -0.18f, 0.0f }, 4.6f, 0.22f, 0.8f, false);

    float y0 = b.y + 0.4f;
    CuboTormenta({ b.x - 0.2f, y0 + 1.2f, b.z - 0.2f }, { 3.4f, 2.4f, 2.8f }, Color{ 74, 82, 100, 255 });

    // Ventanales (frente y lado que mira a la plataforma) con parteluces.
    Color vidrio = SombraTormenta(COLOR_VENTANA_TORMENTA, 0.9f, false);
    rlBegin(RL_QUADS);
    QuadTormenta({ b.x - 1.8f, y0 + 1.0f, b.z + 1.205f }, { b.x + 1.4f, y0 + 1.0f, b.z + 1.205f }, { b.x + 1.4f, y0 + 2.1f, b.z + 1.205f }, { b.x - 1.8f, y0 + 2.1f, b.z + 1.205f }, vidrio);
    QuadTormenta({ b.x + 1.505f, y0 + 1.0f, b.z + 1.0f }, { b.x + 1.505f, y0 + 1.0f, b.z - 1.4f }, { b.x + 1.505f, y0 + 2.1f, b.z - 1.4f }, { b.x + 1.505f, y0 + 2.1f, b.z + 1.0f }, TonoTormenta(vidrio, 0.85f));
    rlEnd();

    for (int k = 0; k < 5; k++)
    {
        float x = b.x - 1.8f + 0.8f * (float)k;
        CuboTormenta({ x, y0 + 1.55f, b.z + 1.22f }, { 0.07f, 1.2f, 0.04f }, COLOR_ACERO_OSCURO_TORMENTA);
    }
    for (int k = 0; k < 4; k++)
    {
        float z = b.z + 1.0f - 0.8f * (float)k;
        CuboTormenta({ b.x + 1.52f, y0 + 1.55f, z }, { 0.04f, 1.2f, 0.07f }, COLOR_ACERO_OSCURO_TORMENTA);
    }

    // Siluetas de los operadores mirando la prueba.
    CuboTormenta({ b.x + 0.2f, y0 + 1.25f, b.z + 0.9f }, { 0.35f, 0.7f, 0.25f }, Color{ 40, 36, 44, 255 });
    CuboTormenta({ b.x + 1.1f, y0 + 1.3f, b.z - 0.3f }, { 0.25f, 0.8f, 0.35f }, Color{ 40, 36, 44, 255 });

    CuboTormenta({ b.x - 0.2f, y0 + 2.5f, b.z - 0.2f }, { 4.0f, 0.22f, 3.4f }, COLOR_ACERO_OSCURO_TORMENTA);
    CuboTormenta({ b.x - 1.0f, y0 + 2.85f, b.z - 0.8f }, { 1.0f, 0.5f, 0.8f }, COLOR_ACERO_TORMENTA);

    // Antena con pararrayos y plato.
    CilindroTormenta({ b.x + 0.8f, y0 + 2.6f, b.z - 0.9f }, { b.x + 0.8f, y0 + 6.2f, b.z - 0.9f }, 0.07f, 0.04f, 6, COLOR_ACERO_TORMENTA);
    for (int k = 0; k < 3; k++)
    {
        float y = y0 + 3.6f + 0.8f * (float)k;
        CuboTormenta({ b.x + 0.8f, y, b.z - 0.9f }, { 0.7f - 0.15f * (float)k, 0.04f, 0.04f }, COLOR_ACERO_TORMENTA);
    }
    CilindroTormenta({ b.x - 1.4f, y0 + 3.0f, b.z + 0.3f }, { b.x - 1.15f, y0 + 3.25f, b.z + 0.65f }, 0.42f, 0.36f, 12, COLOR_ALUMINIO_TORMENTA);

    BarandaTormenta({ b.x - 2.2f, b.y + 0.4f, b.z + 2.0f }, { b.x + 2.2f, b.y + 0.4f, b.z + 2.0f }, 0.8f);
    BarandaTormenta({ b.x + 2.2f, b.y + 0.4f, b.z + 2.0f }, { b.x + 2.2f, b.y + 0.4f, b.z - 2.0f }, 0.8f);
}


// Torre de la bobina gigante a la derecha (su toroide es la punta de
// chispas PUNTAS_BOBINAS_TORMENTA[2]).
static const Vector3 TORRE_BOBINA_TORMENTA = { 11.8f, -0.2f, -8.2f };

static void TorreBobinaTormenta()
{
    Vector3 b = TORRE_BOBINA_TORMENTA;

    CuboNieblaTormenta({ b.x, (NIVEL_NUBES_TORMENTA + b.y - 0.4f) * 0.5f, b.z }, { 2.8f, b.y - 0.4f - NIVEL_NUBES_TORMENTA, 2.8f }, COLOR_HORMIGON_TORMENTA);

    for (int k = 1; k < 5; k++)
    {
        float y = b.y - 0.4f - 3.0f * (float)k;
        CuboNieblaTormenta({ b.x, y, b.z }, { 3.0f, 0.24f, 3.0f }, TonoTormenta(COLOR_HORMIGON_TORMENTA, 0.85f));
    }

    // Mensulas bajo la plataforma redonda.
    for (int k = 0; k < 4; k++)
    {
        float a = 0.785f + 1.5708f * (float)k;
        VigaTormenta({ b.x + std::cos(a) * 1.3f, b.y - 2.4f, b.z + std::sin(a) * 1.3f }, { b.x + std::cos(a) * 2.3f, b.y - 0.45f, b.z + std::sin(a) * 2.3f }, 0.22f, 0.22f, COLOR_ACERO_OSCURO_TORMENTA);
    }

    CilindroTormenta({ b.x, b.y - 0.4f, b.z }, { b.x, b.y, b.z }, 2.5f, 2.5f, 20, COLOR_ACERO_OSCURO_TORMENTA);
    FranjasPeligroTormenta({ b.x - 2.5f, b.y - 0.02f, b.z + 2.48f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, -0.2f, 0.0f }, 5.0f, 0.25f, 0.8f, false);

    BobinaTeslaTormenta(b, 2.08f);

    // Pararrayos sobre el toroide (blanco del relampago de fondo).
    float cimaToroide = b.y + 3.55f * 2.08f;
    CilindroTormenta({ b.x, cimaToroide, b.z }, { b.x, 7.9f, b.z }, 0.06f, 0.02f, 6, COLOR_COBRE_TORMENTA);

    for (int k = 0; k < 6; k++)
    {
        float a = 1.0472f * (float)k;
        AisladorTormenta({ b.x + std::cos(a) * 2.1f, b.y, b.z + std::sin(a) * 2.1f }, 4, 1.0f);
    }
}


// Torre de alta tension (celosia) que sale del mar de nubes.
static void TorreAltaTensionTormenta(Vector3 base, float alto, float giro, Vector3 brazos[2])
{
    Vector3 ex = { std::cos(giro), 0.0f, std::sin(giro) };
    Vector3 ez = { -std::sin(giro), 0.0f, std::cos(giro) };
    float abajo = 2.2f;
    float arriba = 0.45f;
    Color acero = TinteTormenta(COLOR_ACERO_OSCURO_TORMENTA, base);

    Vector3 patas0[4], patas1[4];
    for (int k = 0; k < 4; k++)
    {
        float sx = (k == 0 || k == 3) ? -1.0f : 1.0f;
        float sz = (k < 2) ? -1.0f : 1.0f;
        patas0[k] = SumaTormenta(base, SumaTormenta(EscalaTormenta(ex, sx * abajo), EscalaTormenta(ez, sz * abajo)));
        patas1[k] = SumaTormenta({ base.x, base.y + alto, base.z }, SumaTormenta(EscalaTormenta(ex, sx * arriba), EscalaTormenta(ez, sz * arriba)));
        VigaTormenta(patas0[k], patas1[k], 0.16f, 0.16f, acero);
    }

    // Arriostrado en X (lineas finas).
    int tramos = 6;
    for (int t = 0; t < tramos; t++)
    {
        float u0 = (float)t / (float)tramos;
        float u1 = (float)(t + 1) / (float)tramos;
        for (int k = 0; k < 4; k++)
        {
            Vector3 a0 = { patas0[k].x + (patas1[k].x - patas0[k].x) * u0, patas0[k].y + (patas1[k].y - patas0[k].y) * u0, patas0[k].z + (patas1[k].z - patas0[k].z) * u0 };
            int n = (k + 1) % 4;
            Vector3 b1 = { patas0[n].x + (patas1[n].x - patas0[n].x) * u1, patas0[n].y + (patas1[n].y - patas0[n].y) * u1, patas0[n].z + (patas1[n].z - patas0[n].z) * u1 };
            DrawLine3D(a0, b1, acero);
        }
    }

    // Dos crucetas con aisladores colgando.
    for (int c = 0; c < 2; c++)
    {
        float y = base.y + alto - 0.6f - 1.6f * (float)c;
        float largo = 2.6f - 0.5f * (float)c;
        Vector3 izq = SumaTormenta({ base.x, y, base.z }, EscalaTormenta(ex, -largo));
        Vector3 der = SumaTormenta({ base.x, y, base.z }, EscalaTormenta(ex, largo));
        VigaTormenta(izq, der, 0.14f, 0.2f, acero);
        CilindroTormenta({ izq.x, y - 0.6f, izq.z }, izq, 0.1f, 0.1f, 5, TinteTormenta(COLOR_CERAMICA_TORMENTA, base));
        CilindroTormenta({ der.x, y - 0.6f, der.z }, der, 0.1f, 0.1f, 5, TinteTormenta(COLOR_CERAMICA_TORMENTA, base));
        if (c == 0)
        {
            brazos[0] = { izq.x, y - 0.6f, izq.z };
            brazos[1] = { der.x, y - 0.6f, der.z };
        }
    }
}


// Radar meteorologico sobre su torre, el plato gira despacio.
static void RadarTormenta(Vector3 base, float t)
{
    CuboNieblaTormenta({ base.x, (NIVEL_NUBES_TORMENTA + base.y) * 0.5f, base.z }, { 1.8f, base.y - NIVEL_NUBES_TORMENTA, 1.8f }, COLOR_HORMIGON_TORMENTA);
    CuboNieblaTormenta({ base.x, base.y + 0.15f, base.z }, { 3.0f, 0.3f, 3.0f }, COLOR_ACERO_OSCURO_TORMENTA);
    CilindroTormenta({ base.x, base.y + 0.3f, base.z }, { base.x, base.y + 1.4f, base.z }, 0.2f, 0.15f, 8, TinteTormenta(COLOR_ACERO_TORMENTA, base));

    float giro = t * 0.7f;
    Vector3 mira = NormalizarTormenta({ std::cos(giro), 0.45f, std::sin(giro) });
    Vector3 centro = { base.x, base.y + 1.9f, base.z };
    Color plato = TinteTormenta(COLOR_ALUMINIO_TORMENTA, base);
    CilindroTormenta(SumaTormenta(centro, EscalaTormenta(mira, -0.25f)), SumaTormenta(centro, EscalaTormenta(mira, 0.15f)), 0.4f, 1.3f, 14, plato, false);
    CilindroTormenta(centro, SumaTormenta(centro, EscalaTormenta(mira, 0.9f)), 0.04f, 0.02f, 4, plato);
}


// Plataforma de pruebas vecina con su propia bobina (lejos, a la derecha).
static void PlataformaVecinaTormenta(Vector3 base)
{
    CuboNieblaTormenta({ base.x, (NIVEL_NUBES_TORMENTA + base.y) * 0.5f, base.z }, { 2.6f, base.y - NIVEL_NUBES_TORMENTA, 2.6f }, COLOR_HORMIGON_TORMENTA);
    CuboNieblaTormenta({ base.x, base.y + 0.2f, base.z }, { 7.0f, 0.4f, 7.0f }, COLOR_ACERO_OSCURO_TORMENTA);
    BobinaTeslaTormenta({ base.x, base.y + 0.4f, base.z }, 1.3f);
    PararrayosTormenta({ base.x - 2.8f, base.y + 0.4f, base.z - 2.8f }, 5.0f);
}


//==================================================
// LA PLATAFORMA DE PRUEBAS
//==================================================

static const Vector3 EMISORES_TORMENTA[7] =
{
    { 0.0f, 0.0f, 0.0f },
    { -3.25f, 0.0f, -2.80f }, { 3.25f, 0.0f, -2.80f },
    { -3.25f, 0.0f, 2.80f }, { 3.25f, 0.0f, 2.80f },
    { 0.0f, 0.0f, -3.75f }, { 0.0f, 0.0f, 3.75f }
};

// Mensulas de la plataforma: dos esquinas del fondo con bobinas Tesla,
// dos laterales con mastiles de focos y dos del fondo con instrumentos.
static const Vector3 MENSULAS_BOBINA_TORMENTA[2] = { { -7.6f, -0.8f, -7.6f }, { 7.6f, -0.8f, -7.6f } };
static const Vector3 MENSULAS_FOCOS_TORMENTA[2] = { { -7.0f, -0.7f, 2.4f }, { 7.0f, -0.7f, 2.4f } };
static const Vector3 OBJETIVOS_FOCOS_TORMENTA[2] = { { -2.2f, 0.0f, -0.6f }, { 2.2f, 0.0f, -0.6f } };
static const float ALTO_MASTIL_FOCOS_TORMENTA = 5.6f;


// Cubierta de chapa: placas de 1,15 con relieve de lagrimas, remaches,
// pistas de cobre entre los emisores, emisores y franja de peligro en el
// borde real de la arena (5,75).
static void DibujarCubiertaTormenta(Color colorCampo)
{
    const float L = MEDIO_LADO_ARENA_MAGNETICA;
    const int N = 10;
    const float P = 2.0f * L / (float)N;
    bool relieve = CalidadDecoracion() != CALIDAD_BAJA;

    // Cuerpo de la cubierta con su espesor.
    CuboTormenta({ 0.0f, -ESPESOR_CUBIERTA_TORMENTA * 0.5f - 0.002f, 0.0f }, { 2.0f * L, ESPESOR_CUBIERTA_TORMENTA, 2.0f * L }, COLOR_ACERO_OSCURO_TORMENTA);

    rlBegin(RL_QUADS);
    QuadTormenta({ -L, 0.0f, L }, { L, 0.0f, L }, { L, 0.0f, -L }, { -L, 0.0f, -L }, SombraTormenta(COLOR_JUNTA_TORMENTA, 1.0f, true));

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            float x0 = -L + P * (float)i + 0.03f;
            float x1 = -L + P * (float)(i + 1) - 0.03f;
            float z0 = -L + P * (float)j + 0.03f;
            float z1 = -L + P * (float)(j + 1) - 0.03f;
            float var = 0.9f + 0.18f * Hash01Tormenta(i * 7 + 3, j * 11 + 5);
            Color placa = TonoTormenta(COLOR_PLACA_TORMENTA, var);
            if (Hash01Tormenta(i + 40, j + 9) > 0.86f) placa = MezclaTormenta(placa, Color{ 110, 92, 80, 255 }, 0.3f);

            float y = 0.004f;
            QuadTormenta({ x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, { x0, y, z0 }, SombraTormenta(placa, 1.0f, true));

            // Bisel: filo claro al fondo e izquierda, sombra al frente y derecha.
            const float B = 0.05f;
            Color claro = SombraTormenta(placa, 1.18f, true);
            Color oscuro = SombraTormenta(placa, 0.72f, true);
            y = 0.005f;
            QuadTormenta({ x0, y, z0 + B }, { x1, y, z0 + B }, { x1, y, z0 }, { x0, y, z0 }, claro);
            QuadTormenta({ x0, y, z1 }, { x0 + B, y, z1 }, { x0 + B, y, z0 }, { x0, y, z0 }, claro);
            QuadTormenta({ x0, y, z1 }, { x1, y, z1 }, { x1, y, z1 - B }, { x0, y, z1 - B }, oscuro);
            QuadTormenta({ x1 - B, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, { x1 - B, y, z0 }, oscuro);

            // Remaches.
            Color remache = SombraTormenta(placa, 1.3f, true);
            const float R = 0.035f;
            float cx[2] = { x0 + 0.12f, x1 - 0.12f };
            float cz[2] = { z0 + 0.12f, z1 - 0.12f };
            y = 0.007f;
            for (int a = 0; a < 2; a++)
            {
                for (int b = 0; b < 2; b++)
                {
                    QuadTormenta({ cx[a] - R, y, cz[b] + R }, { cx[a] + R, y, cz[b] + R }, { cx[a] + R, y, cz[b] - R }, { cx[a] - R, y, cz[b] - R }, remache);
                }
            }

            // Chapa de lagrimas: rayitas alternadas a 45 grados.
            if (relieve)
            {
                Color lagrima = SombraTormenta(placa, 1.12f, true);
                for (int a = 0; a < 4; a++)
                {
                    for (int b = 0; b < 4; b++)
                    {
                        float px = x0 + 0.2f + (x1 - x0 - 0.4f) * ((float)a + 0.5f) / 4.0f;
                        float pz = z0 + 0.2f + (z1 - z0 - 0.4f) * ((float)b + 0.5f) / 4.0f;
                        float s = ((a + b) & 1) ? 1.0f : -1.0f;
                        float u = 0.055f, w = 0.014f;
                        QuadTormenta(
                            { px - u - w * s, 0.006f, pz + u * s - w },
                            { px + u - w * s, 0.006f, pz - u * s - w },
                            { px + u + w * s, 0.006f, pz - u * s + w },
                            { px - u + w * s, 0.006f, pz + u * s + w },
                            lagrima
                        );
                    }
                }
            }
        }
    }

    // Pistas de cobre: cruz central y laterales que unen los emisores.
    Color cobre = SombraTormenta(COLOR_COBRE_OSCURO_TORMENTA, 1.0f, true);
    Color cobreClaro = SombraTormenta(COLOR_COBRE_TORMENTA, 0.95f, true);
    float y = 0.009f;
    const float A = 0.07f;
    QuadTormenta({ -3.25f, y, A }, { 3.25f, y, A }, { 3.25f, y, -A }, { -3.25f, y, -A }, cobre);
    QuadTormenta({ -A, y, 3.75f }, { A, y, 3.75f }, { A, y, -3.75f }, { -A, y, -3.75f }, cobre);
    QuadTormenta({ -3.25f - A, y, 2.8f }, { -3.25f + A, y, 2.8f }, { -3.25f + A, y, -2.8f }, { -3.25f - A, y, -2.8f }, cobre);
    QuadTormenta({ 3.25f - A, y, 2.8f }, { 3.25f + A, y, 2.8f }, { 3.25f + A, y, -2.8f }, { 3.25f - A, y, -2.8f }, cobre);
    y = 0.010f;
    const float H = 0.022f;
    QuadTormenta({ -3.25f, y, H }, { 3.25f, y, H }, { 3.25f, y, -H }, { -3.25f, y, -H }, cobreClaro);
    QuadTormenta({ -H, y, 3.75f }, { H, y, 3.75f }, { H, y, -3.75f }, { -H, y, -3.75f }, cobreClaro);
    QuadTormenta({ -3.25f - H, y, 2.8f }, { -3.25f + H, y, 2.8f }, { -3.25f + H, y, -2.8f }, { -3.25f - H, y, -2.8f }, cobreClaro);
    QuadTormenta({ 3.25f - H, y, 2.8f }, { 3.25f + H, y, 2.8f }, { 3.25f + H, y, -2.8f }, { 3.25f - H, y, -2.8f }, cobreClaro);
    rlEnd();

    // Emisores: bobinas planas donde puede aparecer el nucleo.
    for (int e = 0; e < 7; e++)
    {
        Vector3 c = EMISORES_TORMENTA[e];
        AnilloTormenta({ c.x, 0.012f, c.z }, 0.0f, 0.86f, SombraTormenta(COLOR_ACERO_OSCURO_TORMENTA, 1.0f, true), 24);
        AnilloTormenta({ c.x, 0.014f, c.z }, 0.78f, 0.84f, SombraTormenta(COLOR_ACERO_TORMENTA, 1.1f, true), 24);

        for (int r = 0; r < 4; r++)
        {
            float r0 = 0.3f + 0.115f * (float)r;
            AnilloTormenta({ c.x, 0.015f, c.z }, r0, r0 + 0.07f, SombraTormenta(r % 2 == 0 ? COLOR_COBRE_TORMENTA : COLOR_COBRE_OSCURO_TORMENTA, 1.0f, true), 24);
        }

        AnilloTormenta({ c.x, 0.016f, c.z }, 0.0f, 0.18f, SombraTormenta(COLOR_ALUMINIO_TORMENTA, 0.8f, true), 12);
    }

    // Franja de peligro en el borde y tira de luz del campo por dentro.
    const float F = 0.38f;
    y = 0.012f;
    FranjasPeligroTormenta({ -L, y, -L }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, F }, 2.0f * L, 0.3f, 1.0f, true);
    FranjasPeligroTormenta({ L, y, L }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -F }, 2.0f * L, 0.3f, 1.0f, true);
    FranjasPeligroTormenta({ -L, y + 0.001f, L }, { 0.0f, 0.0f, -1.0f }, { F, 0.0f, 0.0f }, 2.0f * L, 0.3f, 1.0f, true);
    FranjasPeligroTormenta({ L, y + 0.001f, -L }, { 0.0f, 0.0f, 1.0f }, { -F, 0.0f, 0.0f }, 2.0f * L, 0.3f, 1.0f, true);

    float li = L - F - 0.02f;
    float le = L - F - 0.09f;
    y = 0.014f;
    rlBegin(RL_QUADS);
    QuadTormenta({ -li, y, -le }, { li, y, -le }, { li, y, -li }, { -li, y, -li }, colorCampo);
    QuadTormenta({ -li, y, li }, { li, y, li }, { li, y, le }, { -li, y, le }, colorCampo);
    QuadTormenta({ -li, y, le }, { -le, y, le }, { -le, y, -le }, { -li, y, -le }, colorCampo);
    QuadTormenta({ le, y, le }, { li, y, le }, { li, y, -le }, { le, y, -le }, colorCampo);
    rlEnd();

    // Canto del frente con franjas (el unico que ve la camara).
    FranjasPeligroTormenta({ -L, -0.02f, L + 0.003f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, -0.16f, 0.0f }, 2.0f * L, 0.3f, 0.82f, false);

    // Estructura bajo la cubierta: vigas perimetrales, puntales y fuste.
    for (int s = -1; s <= 1; s += 2)
    {
        CuboTormenta({ 0.0f, -0.75f, (float)s * (L - 0.5f) }, { 2.0f * L - 0.8f, 0.4f, 0.3f }, COLOR_ACERO_OSCURO_TORMENTA);
        CuboTormenta({ (float)s * (L - 0.5f), -0.75f, 0.0f }, { 0.3f, 0.4f, 2.0f * L - 0.8f }, COLOR_ACERO_OSCURO_TORMENTA);
    }

    for (int k = 0; k < 4; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sz = (k & 2) ? 1.0f : -1.0f;
        VigaTormenta({ sx * (L - 0.6f), -0.9f, sz * (L - 0.6f) }, { sx * 1.3f, -4.2f, sz * 1.3f }, 0.32f, 0.32f, COLOR_ACERO_OSCURO_TORMENTA);
    }

    CuboTormenta({ 0.0f, -4.3f, 0.0f }, { 3.8f, 0.4f, 3.8f }, COLOR_ACERO_OSCURO_TORMENTA);
    CuboNieblaTormenta({ 0.0f, (NIVEL_NUBES_TORMENTA - 4.5f) * 0.5f, 0.0f }, { 3.0f, -4.5f - NIVEL_NUBES_TORMENTA, 3.0f }, COLOR_HORMIGON_TORMENTA);
}


// Mensulas exteriores con su utileria (anillo cercano).
static void DibujarMensulasTormenta(float t, int paso)
{
    const float L = MEDIO_LADO_ARENA_MAGNETICA;

    // Esquinas del fondo: bobina Tesla, condensadores, aisladores.
    for (int s = 0; s < 2; s++)
    {
        Vector3 m = MENSULAS_BOBINA_TORMENTA[s];
        float sx = s == 0 ? -1.0f : 1.0f;

        CuboTormenta({ m.x, m.y - 0.15f, m.z }, { 3.0f, 0.3f, 3.0f }, COLOR_ACERO_OSCURO_TORMENTA);
        FranjasPeligroTormenta({ m.x - 1.5f, m.y - 0.02f, m.z + 1.503f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, -0.2f, 0.0f }, 3.0f, 0.2f, 0.8f, false);

        VigaTormenta({ sx * (L - 0.3f), -0.35f, -(L - 0.9f) }, { m.x - sx * 0.6f, m.y - 0.15f, m.z + 0.9f }, 0.25f, 0.25f, COLOR_ACERO_TORMENTA);
        VigaTormenta({ sx * (L - 0.9f), -0.35f, -(L - 0.3f) }, { m.x - sx * 0.9f, m.y - 0.15f, m.z + 0.6f }, 0.25f, 0.25f, COLOR_ACERO_TORMENTA);
        VigaTormenta({ sx * (L - 1.2f), -2.4f, -(L - 1.2f) }, { m.x, m.y - 0.3f, m.z }, 0.2f, 0.2f, COLOR_ACERO_OSCURO_TORMENTA);

        BobinaTeslaTormenta({ m.x, m.y, m.z }, 1.1f);

        BarandaTormenta({ m.x - 1.45f, m.y, m.z - 1.45f }, { m.x + 1.45f, m.y, m.z - 1.45f }, 0.75f);
        BarandaTormenta({ m.x + sx * 1.45f, m.y, m.z - 1.45f }, { m.x + sx * 1.45f, m.y, m.z + 1.45f }, 0.75f);

        if (paso == 1)
        {
            CondensadoresTormenta({ m.x + sx * 1.05f, m.y, m.z - 0.9f }, { 0.0f, 0.0f, 1.0f }, 3);
            AisladorTormenta({ m.x - sx * 0.95f, m.y, m.z - 1.0f }, 6, 1.3f);
        }

        BalizaTormenta({ m.x - sx * 1.3f, m.y, m.z + 1.3f });
    }

    // Laterales: mastiles de focos sobre mensulas cortas.
    for (int s = 0; s < 2; s++)
    {
        Vector3 m = MENSULAS_FOCOS_TORMENTA[s];
        float sx = s == 0 ? -1.0f : 1.0f;

        CuboTormenta({ m.x, m.y - 0.12f, m.z }, { 1.3f, 0.24f, 1.3f }, COLOR_ACERO_OSCURO_TORMENTA);
        VigaTormenta({ sx * (L - 0.2f), -0.4f, m.z }, { m.x, m.y - 0.12f, m.z }, 0.22f, 0.22f, COLOR_ACERO_TORMENTA);
        VigaTormenta({ sx * (L - 0.5f), -1.9f, m.z }, { m.x, m.y - 0.24f, m.z }, 0.16f, 0.16f, COLOR_ACERO_OSCURO_TORMENTA);
        MastilFocosTormenta(m, ALTO_MASTIL_FOCOS_TORMENTA, OBJETIVOS_FOCOS_TORMENTA[s]);
        GabineteTormenta({ m.x + sx * 0.25f, m.y, m.z + 0.35f }, sx * -1.5708f);
    }

    // Fondo: pararrayos alto a la izquierda, anemometro a la derecha,
    // carretes y gabinetes sobre una pasarela baja.
    // (Baja: lo que cae por el fondo pasa por delante, oculto por la cubierta.)
    const float YP = -2.4f;
    CuboTormenta({ 0.0f, YP - 0.11f, -L - 1.05f }, { 8.4f, 0.22f, 1.5f }, COLOR_ACERO_OSCURO_TORMENTA);
    for (int k = -1; k <= 1; k += 2)
    {
        VigaTormenta({ (float)k * 3.0f, -0.6f, -L + 0.4f }, { (float)k * 3.0f, YP - 0.11f, -L - 0.8f }, 0.2f, 0.2f, COLOR_ACERO_TORMENTA);
    }
    BarandaTormenta({ -4.2f, YP, -L - 1.75f }, { 4.2f, YP, -L - 1.75f }, 0.7f);

    PararrayosTormenta({ -3.2f, YP, -L - 1.1f }, 8.2f);
    AnemometroTormenta({ 3.2f, YP, -L - 1.1f }, 6.0f, t);

    if (paso == 1)
    {
        CarreteCableTormenta({ -1.2f, YP, -L - 1.1f }, 0.2f);
        GabineteTormenta({ 1.1f, YP, -L - 1.25f }, 0.0f);
        AisladorTormenta({ 0.2f, YP, -L - 1.3f }, 5, 1.2f);
    }

    // Cables: de las bobinas a las torres y colgando bajo la cubierta.
    Color cable = { 26, 26, 32, 255 };
    CableTormenta({ -8.9f, -0.6f, -8.4f }, { TORRE_CONTROL_TORMENTA.x + 1.0f, 0.1f, TORRE_CONTROL_TORMENTA.z - 0.6f }, 0.9f, 0.06f, cable);
    CableTormenta({ 8.9f, -0.6f, -8.4f }, { TORRE_BOBINA_TORMENTA.x - 2.3f, -0.3f, TORRE_BOBINA_TORMENTA.z + 0.6f }, 0.8f, 0.07f, cable);

    for (int s = -1; s <= 1; s += 2)
    {
        CableTormenta({ (float)s * (L + 0.05f), -0.6f, 2.0f }, { (float)s * (L + 0.05f), -0.6f, -6.0f }, 0.9f, 0.05f, cable);
        CableTormenta({ (float)s * (L + 0.1f), -0.5f, 5.4f }, { (float)s * (L + 0.1f), -0.5f, 2.8f }, 0.45f, 0.04f, Color{ 150, 60, 40, 255 });
    }
}


//==================================================
// NUCLEO, PINCHOS Y JUGADORES
//==================================================

// Escala de aparicion del nucleo tras cada cambio (rebote corto).
static float EscalaNucleoTormenta(const MinijuegoTormentaMagnetica& m)
{
    if (m.fase == FASE_MAGNETICA_PREPARACION) return 1.0f;
    float k = m.tiempoDesdeCambio / 0.35f;
    if (k >= 1.0f) return 1.0f;
    return 0.45f + 0.55f * k + 0.3f * std::sin(k * 3.1416f);
}


// Nucleo: esfera del color del campo dentro de una jaula de aros que
// gira, y seis puntas que señalan hacia adentro (atrae) o hacia afuera
// (repele).
// MODELO FUTURO: nucleo magnetico con jaula de aros y polos.
static void DibujarNucleoTormenta(const MinijuegoTormentaMagnetica& m, float t)
{
    Vector3 p = m.posicionNucleo;
    Color color = ColorCampoTormenta(m.campoAtrae);
    float escala = EscalaNucleoTormenta(m);
    float pulso = 1.0f + 0.08f * std::sin(t * 5.2f);
    float R = RADIO_NUCLEO_MAGNETICO * escala;

    DrawSphereEx(p, R * 0.6f * pulso, 10, 14, MezclaTormenta(color, WHITE, 0.3f));

    float sentido = m.campoAtrae ? 1.0f : -1.0f;

    for (int i = 0; i < 3; i++)
    {
        float a = t * (1.1f + 0.4f * (float)i) * sentido + 2.1f * (float)i;
        Vector3 n = NormalizarTormenta({ std::cos(a), 0.5f + 0.4f * (float)i, std::sin(a) });
        Vector3 u = NormalizarTormenta(CruzTormenta(n, { 0.0f, 0.0f, 1.0f }));
        Vector3 v = CruzTormenta(n, u);
        ToroTormenta(p, u, v, R * (1.0f + 0.1f * (float)i), 0.035f, COLOR_ALUMINIO_TORMENTA, 22, 5);
    }

    // Polos: conos que giran despacio alrededor del nucleo.
    for (int k = 0; k < 6; k++)
    {
        float a = t * 0.8f * sentido + 1.0472f * (float)k;
        Vector3 radial = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 cerca = SumaTormenta(p, EscalaTormenta(radial, R * 1.35f));
        Vector3 lejos = SumaTormenta(p, EscalaTormenta(radial, R * 1.75f));

        if (m.campoAtrae)
        {
            CilindroTormenta(lejos, cerca, 0.11f * escala, 0.0f, 6, color);
        }
        else
        {
            CilindroTormenta(cerca, lejos, 0.11f * escala, 0.0f, 6, color);
        }
    }
}


// Pincho: dardo de acero cargado que cae del cielo (la marca del suelo va
// con los translucidos).
// MODELO FUTURO: dardo pararrayos con aletas.
static void DibujarPinchoTormenta(const PinchoTormentaMagnetica& pincho)
{
    if (pincho.aviso > 0.0f) return;

    Vector3 p = pincho.posicion;
    CilindroTormenta({ p.x, p.y - 0.55f, p.z }, { p.x, p.y + 0.3f, p.z }, 0.0f, 0.28f, 8, Color{ 186, 194, 210, 255 });
    CilindroTormenta({ p.x, p.y + 0.3f, p.z }, { p.x, p.y + 0.48f, p.z }, 0.2f, 0.2f, 8, COLOR_COBRE_TORMENTA);
    CilindroTormenta({ p.x, p.y + 0.48f, p.z }, { p.x, p.y + 0.62f, p.z }, 0.12f, 0.06f, 8, COLOR_ACERO_OSCURO_TORMENTA);

    for (int k = 0; k < 3; k++)
    {
        float a = 2.0944f * (float)k;
        Vector3 ex = { std::cos(a), 0.0f, std::sin(a) };
        Vector3 ez = { -std::sin(a), 0.0f, std::cos(a) };
        CajaOrientadaTormenta({ p.x + ex.x * 0.26f, p.y + 0.5f, p.z + ex.z * 0.26f }, ex, { 0.0f, 1.0f, 0.0f }, ez, { 0.22f, 0.3f, 0.03f }, COLOR_PELIGRO_TORMENTA);
    }
}


static void DibujarAnilloJugadorTormenta(const JugadorPrueba& jugador, Color color, float t)
{
    if (jugador.cayendo || !JugadorSobreArenaMagnetica(jugador)) return;

    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (pies < -0.3f) return;
    float alto = pies > 0.0f ? pies : 0.0f;
    float escala = 1.0f - Limitar01VisualTormenta(alto / 4.0f) * 0.35f;
    float pulso = 1.0f + 0.04f * std::sin(t * 5.0f + jugador.posicion.x);

    Color relleno = color;
    relleno.a = 70;
    AnilloTormenta({ jugador.posicion.x, 0.03f, jugador.posicion.z }, 0.0f, 0.5f * escala, relleno, 20);
    AnilloTormenta({ jugador.posicion.x, 0.034f, jugador.posicion.z }, 0.5f * escala * pulso, 0.62f * escala * pulso, color, 24);
}


// Eliminados: caen girando hacia el mar de nubes.
static void DibujarCaidasTormenta(
    const MinijuegoTormentaMagnetica& m,
    const Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorTormentaMagnetica& e = m.estadosJugadores[i];

        if (!m.resultado.participantes[i].participo || e.tiempoCaida < 0.0f || e.tocoFondo)
        {
            continue;
        }

        Vector3 pies = { e.posicionCaida.x, e.posicionCaida.y - 0.7f, e.posicionCaida.z };
        DibujarModeloJugadorEnPosicion(pies, e.tiempoCaida * 220.0f, participantes[i].color);
    }
}


static void DibujarParticulasTormenta(const MinijuegoTormentaMagnetica& m)
{
    for (int i = 0; i < MAX_PARTICULAS_TORMENTA_MAGNETICA; i++)
    {
        const ParticulaTormentaMagnetica& p = m.particulasEfecto[i];

        if (!p.activa || p.brilla)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        float tam = p.tamano * (0.5f + 0.5f * vida);
        CuboTormenta(p.posicion, { tam, tam, tam }, p.color);
    }
}


// Anillos del campo en la chapa, marcas de los pinchos, anillos de los
// jugadores y gotas que rebotan.
static void DibujarTranslucidosTormenta(
    const MinijuegoTormentaMagnetica& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[]
)
{
    float t = m.tiempoVisual;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Lineas de campo: anillos que se cierran sobre el nucleo (atrae) o se
    // abren hacia el borde (repele).
    Color campo = ColorCampoTormenta(m.campoAtrae);
    Vector3 c = { m.posicionNucleo.x, 0.022f, m.posicionNucleo.z };

    for (int k = 0; k < 4; k++)
    {
        float u = std::fmod(t * 0.75f + (float)k * 0.25f, 1.0f);
        float r = m.campoAtrae ? 4.6f - 3.6f * u : 1.0f + 3.6f * u;
        Color anillo = campo;
        anillo.a = CanalTormenta(150.0f * std::sin(u * 3.1416f));
        AnilloTormenta(c, r, r + 0.09f, anillo, 40);
    }

    // Pinchos: marca que crece donde va a caer y sombra mientras cae.
    for (int i = 0; i < MAX_PINCHOS_TORMENTA_MAGNETICA; i++)
    {
        const PinchoTormentaMagnetica& p = m.pinchos[i];
        if (!p.activo) continue;

        Vector3 s = { p.posicion.x, 0.026f, p.posicion.z };

        if (p.aviso > 0.0f)
        {
            float tt = 1.0f - p.aviso / p.avisoTotal;
            float radio = 0.25f + 0.65f * tt;
            Color relleno = COLOR_AMBAR_TORMENTA;
            relleno.a = CanalTormenta(255.0f * (0.18f + 0.30f * tt));
            AnilloTormenta(s, 0.0f, radio, relleno, 24);

            Color borde = COLOR_AMBAR_TORMENTA;
            borde.a = 200;
            AnilloTormenta({ s.x, s.y + 0.002f, s.z }, 0.86f, 0.94f, borde, 28);
            AnilloTormenta({ s.x, s.y + 0.003f, s.z }, radio - 0.05f, radio, Color{ 255, 236, 200, 230 }, 24);

            rlBegin(RL_QUADS);
            for (int k = 0; k < 4; k++)
            {
                float a = 1.5708f * (float)k + 0.785f;
                Vector3 d = { std::cos(a), 0.0f, std::sin(a) };
                Vector3 l = { -d.z * 0.035f, 0.0f, d.x * 0.035f };
                Vector3 a0 = { s.x + d.x * 0.98f, s.y + 0.004f, s.z + d.z * 0.98f };
                Vector3 a1 = { s.x + d.x * 1.25f, s.y + 0.004f, s.z + d.z * 1.25f };
                QuadTormenta(RestaTormenta(a0, l), SumaTormenta(a0, l), SumaTormenta(a1, l), RestaTormenta(a1, l), borde);
            }
            rlEnd();
        }
        else
        {
            float h = Limitar01VisualTormenta(p.posicion.y / 5.6f);
            AnilloTormenta(s, 0.0f, 0.25f + 0.35f * (1.0f - h), Color{ 10, 10, 20, (unsigned char)(150.0f - 70.0f * h) }, 16);
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo || m.estadosJugadores[i].eliminado) continue;
        DibujarAnilloJugadorTormenta(jugadores[i], participantes[i].color, t);
    }

    // Gotas que rebotan en la chapa.
    if (CalidadEfectos() != CALIDAD_BAJA)
    {
        int gotas = (int)(36.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < gotas; k++)
        {
            float ciclo = 0.55f + 0.3f * Hash01Tormenta(k, 300);
            float fase = t / ciclo + Hash01Tormenta(k, 301);
            int vuelta = (int)std::floor(fase);
            float u = fase - (float)vuelta;
            float x = -5.4f + 10.8f * Hash01Tormenta(k * 13 + vuelta, 302);
            float z = -5.4f + 10.8f * Hash01Tormenta(k * 13 + vuelta, 303);
            Color gota = { 190, 206, 240, CanalTormenta(130.0f * (1.0f - u)) };
            float r = 0.04f + 0.2f * u;
            AnilloTormenta({ x, 0.03f, z }, r, r + 0.025f, gota, 10);
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Lluvia: trazos sesgados por el viento que caen a traves de la escena.
static void DibujarLluviaTormenta(float t)
{
    if (CalidadEfectos() == CALIDAD_BAJA) return;

    int cantidad = (int)(380.0f * FactorCalidadGrafica(CalidadParticulas()));
    float d = destelloTormenta;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlBegin(RL_LINES);

    for (int k = 0; k < cantidad; k++)
    {
        float vel = 17.0f + 6.0f * Hash01Tormenta(k, 310);
        float caida = std::fmod(t * vel + Hash01Tormenta(k, 311) * 16.0f, 16.0f);
        float y = 12.0f - caida;
        float x = -17.0f + 34.0f * Hash01Tormenta(k, 312) + caida * 0.18f;
        float z = -18.0f + 28.0f * Hash01Tormenta(k, 313);

        // La que cae sobre la cubierta muere en la chapa.
        if (y < 0.0f && std::fabs(x) < MEDIO_LADO_ARENA_MAGNETICA && std::fabs(z) < MEDIO_LADO_ARENA_MAGNETICA) continue;

        float largo = 0.55f + 0.25f * Hash01Tormenta(k, 314);
        unsigned char alfa = CanalTormenta(70.0f + 60.0f * d + 40.0f * Hash01Tormenta(k, 315));
        rlColor4ub(170, 186, 222, 0);
        rlVertex3f(x - largo * 0.18f, y + largo, z);
        rlColor4ub(196, 210, 240, alfa);
        rlVertex3f(x, y, z);
    }

    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// LUCES (pasada aditiva)
//==================================================

// Haz que mira a la camara: ancho y color distintos en cada punta.
static void HazTormenta(Vector3 a, Vector3 b, float anchoA, float anchoB, Color colorA, Color colorB)
{
    Vector3 dir = RestaTormenta(b, a);
    Vector3 medio = EscalaTormenta(SumaTormenta(a, b), 0.5f);
    Vector3 lado = NormalizarTormenta(CruzTormenta(dir, RestaTormenta(OJO_NIEBLA_TORMENTA, medio)));

    rlBegin(RL_QUADS);
    VerticeTormenta(RestaTormenta(a, EscalaTormenta(lado, anchoA * 0.5f)), colorA);
    VerticeTormenta(SumaTormenta(a, EscalaTormenta(lado, anchoA * 0.5f)), colorA);
    VerticeTormenta(SumaTormenta(b, EscalaTormenta(lado, anchoB * 0.5f)), colorB);
    VerticeTormenta(RestaTormenta(b, EscalaTormenta(lado, anchoB * 0.5f)), colorB);
    rlEnd();
}


static Color ConAlfaTormenta(Color c, float alfa)
{
    c.a = CanalTormenta(alfa);
    return c;
}


// Color de la tira del borde: el del campo, y destella antes del cambio.
static Color ColorBordeTormenta(const MinijuegoTormentaMagnetica& m)
{
    Color campo = ColorCampoTormenta(m.campoAtrae);

    if (m.fase == FASE_MAGNETICA_JUGANDO && m.tiempoHastaCambioCampo < 0.9f)
    {
        bool destello = std::fmod(m.tiempoVisual * 8.0f, 2.0f) < 1.0f;
        return destello ? WHITE : Color{ 255, 210, 60, 255 };
    }

    if (m.fase != FASE_MAGNETICA_PREPARACION && m.tiempoDesdeCambio < 0.35f) return WHITE;
    return campo;
}


static void DibujarLucesTormenta(
    const MinijuegoTormentaMagnetica& m,
    const JugadorPrueba jugadores[],
    const RelampagoTormenta& fondo,
    const RelampagoTormenta& rayoCampo
)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = m.tiempoVisual;
    const float L = MEDIO_LADO_ARENA_MAGNETICA;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Relampagos: el del fondo ilumina las nubes; el del campo cae sobre
    // el nucleo y blanquea la cubierta.
    if (fondo.intensidad > 0.0f)
    {
        RayoTormenta(fondo.arriba, fondo.abajo, fondo.semilla, 16, 5.0f, 0.45f, fondo.intensidad);
        ResplandorSueloTormenta({ fondo.abajo.x, NIVEL_NUBES_TORMENTA + 1.5f, fondo.abajo.z }, 34.0f, Color{ 170, 180, 255, CanalTormenta(90.0f * fondo.intensidad) });
    }

    if (rayoCampo.intensidad > 0.0f)
    {
        RayoTormenta(rayoCampo.arriba, rayoCampo.abajo, rayoCampo.semilla, 18, 1.1f, 0.12f, rayoCampo.intensidad);
        ResplandorSueloTormenta({ rayoCampo.abajo.x, 0.03f, rayoCampo.abajo.z }, 7.0f, Color{ 210, 220, 255, CanalTormenta(70.0f * rayoCampo.intensidad) });
        ResplandorFrenteTormenta(rayoCampo.abajo, 2.4f, Color{ 230, 236, 255, CanalTormenta(200.0f * rayoCampo.intensidad) });
    }

    // Relampagos dentro del mar de nubes (parpadeos sueltos).
    for (int k = 0; k < 3; k++)
    {
        int ventana = (int)std::floor(t * 1.7f + (float)k * 0.37f);
        float u = t * 1.7f + (float)k * 0.37f - (float)ventana;
        if (Hash01Tormenta(ventana, 400 + k) > 0.45f) continue;
        float f = ParpadeoRelampagoTormenta(u * 0.6f, 0.3f);
        if (f <= 0.0f) continue;
        Vector3 p =
        {
            -60.0f + 120.0f * Hash01Tormenta(ventana, 410 + k),
            NIVEL_NUBES_TORMENTA + 0.8f,
            -20.0f - 80.0f * Hash01Tormenta(ventana, 420 + k)
        };
        ResplandorSueloTormenta(p, 14.0f, Color{ 150, 160, 240, CanalTormenta(70.0f * f) });
    }

    // Nucleo: halo, charco del color del campo, columna sobre su emisor y
    // aro del emisor activo.
    Color campo = ColorCampoTormenta(m.campoAtrae);
    Vector3 n = m.posicionNucleo;
    float pulso = 0.85f + 0.15f * std::sin(t * 5.2f);
    float escala = EscalaNucleoTormenta(m);

    ResplandorFrenteTormenta(n, 2.3f * pulso * escala, ConAlfaTormenta(campo, 95.0f));
    ResplandorFrenteTormenta(n, 0.9f * escala, Color{ 255, 255, 255, 120 });
    ResplandorSueloTormenta({ n.x, 0.03f, n.z }, 4.2f * pulso, ConAlfaTormenta(campo, 30.0f));
    ResplandorSueloTormenta({ n.x, 0.032f, n.z }, 2.4f * pulso, ConAlfaTormenta(campo, 50.0f));
    // Reflejo estirado en la chapa mojada, hacia la camara.
    ResplandorTormenta({ n.x, 0.034f, n.z + 1.2f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 0.5f, ConAlfaTormenta(campo, 60.0f), 4.0f);
    HazTormenta({ n.x, 0.02f, n.z }, { n.x, n.y, n.z }, 0.7f, 0.3f, ConAlfaTormenta(campo, 80.0f), ConAlfaTormenta(campo, 20.0f));
    AnilloTormenta({ n.x, 0.03f, n.z }, 0.76f, 0.92f, ConAlfaTormenta(campo, 150.0f * pulso), 28);

    for (int k = 0; k < 6; k++)
    {
        float a = t * (m.campoAtrae ? 3.0f : -3.0f) + 1.0472f * (float)k;
        float y = n.y + 0.35f * std::sin(t * 2.0f + (float)k);
        Vector3 chispa = { n.x + std::cos(a) * 1.1f * escala, y, n.z + std::sin(a) * 1.1f * escala };
        ResplandorFrenteTormenta(chispa, 0.16f, ConAlfaTormenta(campo, 220.0f));
    }

    // Tira de luz del borde y LEDs del canto delantero.
    Color borde = ColorBordeTormenta(m);
    float li = L - 0.38f - 0.055f;
    float y = 0.03f;
    float g = 0.28f;
    Color b0 = ConAlfaTormenta(borde, 70.0f);
    Color b1 = ConAlfaTormenta(borde, 0.0f);

    rlBegin(RL_QUADS);
    // Hacia adentro de la tira (fondo, frente, izquierda, derecha).
    VerticeTormenta({ -li, y, -li + g }, b1); VerticeTormenta({ li, y, -li + g }, b1); VerticeTormenta({ li, y, -li }, b0); VerticeTormenta({ -li, y, -li }, b0);
    VerticeTormenta({ -li, y, li }, b0); VerticeTormenta({ li, y, li }, b0); VerticeTormenta({ li, y, li - g }, b1); VerticeTormenta({ -li, y, li - g }, b1);
    VerticeTormenta({ -li, y, li }, b0); VerticeTormenta({ -li + g, y, li }, b1); VerticeTormenta({ -li + g, y, -li }, b1); VerticeTormenta({ -li, y, -li }, b0);
    VerticeTormenta({ li - g, y, li }, b1); VerticeTormenta({ li, y, li }, b0); VerticeTormenta({ li, y, -li }, b0); VerticeTormenta({ li - g, y, -li }, b1);
    rlEnd();

    for (int k = 0; k < 19; k++)
    {
        float x = -L + 0.3f + 0.6f * (float)k;
        Vector3 led = { x, -0.33f, L + 0.02f };
        ResplandorFrenteTormenta(led, 0.22f, ConAlfaTormenta(borde, 150.0f));
        ResplandorFrenteTormenta(led, 0.07f, Color{ 255, 255, 255, 200 });
    }

    // Bobinas Tesla: corona violeta, arcos que saltan y charco en su base.
    for (int b = 0; b < 3; b++)
    {
        Vector3 punta = PUNTAS_BOBINAS_TORMENTA[b];
        float s = b == 2 ? 2.08f : 1.1f;
        float radio = 0.62f * s;
        float zumbido = 0.75f + 0.25f * std::sin(t * 31.0f + (float)b * 2.0f);

        ResplandorFrenteTormenta(punta, 1.6f * s * zumbido, Color{ 150, 130, 255, 60 });
        ResplandorFrenteTormenta(punta, 0.7f * s, Color{ 210, 200, 255, 70 });

        int cuadro = (int)std::floor(t * 14.0f);

        for (int a = 0; a < 3; a++)
        {
            int semilla = cuadro * 7 + b * 131 + a * 17;
            if (Hash01Tormenta(semilla, 5) < 0.25f) continue;

            float ang = 6.2832f * Hash01Tormenta(semilla, 1);
            Vector3 radial = { std::cos(ang), 0.0f, std::sin(ang) };
            Vector3 desde = SumaTormenta(punta, EscalaTormenta(radial, radio + 0.1f * s));
            float largo = (0.9f + 0.9f * Hash01Tormenta(semilla, 2)) * s;
            Vector3 hasta =
            {
                desde.x + radial.x * largo,
                desde.y + (Hash01Tormenta(semilla, 3) - 0.6f) * largo,
                desde.z + radial.z * largo
            };
            RayoTormenta(desde, hasta, semilla, 6, 0.28f * s, 0.035f * s, 0.85f, false);
        }

        float yBase = b == 2 ? TORRE_BOBINA_TORMENTA.y + 0.02f : MENSULAS_BOBINA_TORMENTA[b].y + 0.02f;
        ResplandorSueloTormenta({ punta.x, yBase, punta.z }, 2.4f * s, Color{ 150, 130, 255, CanalTormenta(34.0f * zumbido) });
    }

    // Focos: halo, cono y charco blanco frio sobre la cubierta mojada.
    for (int s = 0; s < 2; s++)
    {
        Vector3 base = MENSULAS_FOCOS_TORMENTA[s];
        Vector3 objetivo = OBJETIVOS_FOCOS_TORMENTA[s];
        Vector3 lampara = { base.x, base.y + ALTO_MASTIL_FOCOS_TORMENTA + 0.12f, base.z };
        float titileo = 0.95f + 0.05f * std::sin(t * 50.0f + (float)s);

        ResplandorFrenteTormenta(lampara, 0.9f, Color{ 220, 232, 255, CanalTormenta(130.0f * titileo) });
        ResplandorFrenteTormenta(lampara, 0.35f, Color{ 255, 255, 255, 220 });
        HazTormenta(lampara, objetivo, 0.5f, 3.6f, Color{ 210, 224, 255, 34 }, Color{ 210, 224, 255, 0 });
        ResplandorSueloTormenta({ objetivo.x, 0.035f, objetivo.z }, 3.2f, Color{ 214, 226, 255, 30 });
        ResplandorSueloTormenta({ objetivo.x, 0.036f, objetivo.z }, 1.8f, Color{ 214, 226, 255, 26 });
        ResplandorTormenta({ objetivo.x * 0.8f, 0.037f, objetivo.z + 2.4f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 0.6f, Color{ 220, 232, 255, 40 }, 4.5f);
    }

    // Balizas ambar de las mensulas: giran y barren con su haz.
    for (int s = 0; s < 2; s++)
    {
        Vector3 m0 = MENSULAS_BOBINA_TORMENTA[s];
        float sx = s == 0 ? -1.0f : 1.0f;
        Vector3 baliza = { m0.x - sx * 1.3f, m0.y + 0.25f, m0.z + 1.3f };
        float giro = t * 4.5f + (float)s * 1.7f;
        float frente = std::sin(giro);
        float brillo = 0.35f + 0.65f * frente * frente;

        ResplandorFrenteTormenta(baliza, 0.6f * brillo + 0.2f, Color{ 255, 150, 50, CanalTormenta(150.0f * brillo) });
        Vector3 hacia = { baliza.x + std::cos(giro) * 2.6f, baliza.y, baliza.z + std::sin(giro) * 2.6f };
        HazTormenta(baliza, hacia, 0.12f, 0.9f, Color{ 255, 160, 60, 60 }, Color{ 255, 160, 60, 0 });
        ResplandorSueloTormenta({ baliza.x, m0.y + 0.02f, baliza.z }, 1.3f, Color{ 255, 150, 50, CanalTormenta(40.0f * brillo) });
    }

    // Cabina de control: ventanales calidos y su charco en la losa.
    Vector3 tc = TORRE_CONTROL_TORMENTA;
    ResplandorFrenteTormenta({ tc.x - 0.2f, tc.y + 1.95f, tc.z + 1.4f }, 2.4f, Color{ 255, 190, 110, 40 });
    ResplandorSueloTormenta({ tc.x, tc.y + 0.42f, tc.z + 1.7f }, 2.0f, Color{ 255, 190, 110, 46 });

    // Luces rojas de balizamiento que parpadean.
    const Vector3 rojas[3] =
    {
        { tc.x + 0.8f, tc.y + 6.65f, tc.z - 0.9f },
        { TORRE_BOBINA_TORMENTA.x, 7.95f, TORRE_BOBINA_TORMENTA.z },
        { -20.0f, 4.2f, -28.0f }
    };
    for (int k = 0; k < 3; k++)
    {
        bool encendida = std::fmod(t + (float)k * 0.53f, 1.6f) < 0.4f;
        ResplandorFrenteTormenta(rojas[k], encendida ? 0.8f : 0.3f, Color{ 255, 60, 50, (unsigned char)(encendida ? 200 : 60) });
    }

    // Pinchos: haz guia mientras avisan; brillo y estela al caer.
    for (int i = 0; i < MAX_PINCHOS_TORMENTA_MAGNETICA; i++)
    {
        const PinchoTormentaMagnetica& p = m.pinchos[i];
        if (!p.activo) continue;

        if (p.aviso > 0.0f)
        {
            float tt = 1.0f - p.aviso / p.avisoTotal;
            HazTormenta({ p.posicion.x, 0.05f, p.posicion.z }, { p.posicion.x, 5.6f, p.posicion.z }, 0.5f, 0.1f, ConAlfaTormenta(COLOR_AMBAR_TORMENTA, 60.0f * tt), ConAlfaTormenta(COLOR_AMBAR_TORMENTA, 0.0f));
            ResplandorSueloTormenta({ p.posicion.x, 0.04f, p.posicion.z }, 1.2f, ConAlfaTormenta(COLOR_AMBAR_TORMENTA, 40.0f * tt));
        }
        else
        {
            Vector3 q = p.posicion;
            ResplandorFrenteTormenta(q, 0.8f, ConAlfaTormenta(COLOR_AMBAR_TORMENTA, 120.0f));
            HazTormenta({ q.x, q.y + 0.4f, q.z }, { q.x, q.y + 2.2f, q.z }, 0.35f, 0.0f, Color{ 255, 220, 160, 110 }, Color{ 255, 220, 160, 0 });
        }
    }

    // Ondas de los cambios de campo e impactos.
    for (int i = 0; i < MAX_ONDAS_TORMENTA_MAGNETICA; i++)
    {
        const OndaTormentaMagnetica& o = m.ondas[i];
        if (!o.activa) continue;

        float u = 1.0f - o.vida / o.vidaMaxima;
        float r = o.haciaAdentro ? o.radioMaximo * (1.0f - u) + 0.3f : o.radioMaximo * u;
        float alfa = o.haciaAdentro ? 220.0f * std::sin(u * 3.1416f) : 220.0f * (1.0f - u);
        AnilloTormenta({ o.posicion.x, 0.04f, o.posicion.z }, r, r + 0.22f, ConAlfaTormenta(o.color, alfa), 40);
    }

    // Chispas.
    for (int i = 0; i < MAX_PARTICULAS_TORMENTA_MAGNETICA; i++)
    {
        const ParticulaTormentaMagnetica& p = m.particulasEfecto[i];
        if (!p.activa || !p.brilla) continue;

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        ResplandorFrenteTormenta(p.posicion, p.tamano * 2.6f, ConAlfaTormenta(p.color, 200.0f * vida));
        ResplandorFrenteTormenta(p.posicion, p.tamano * 0.9f, Color{ 255, 255, 255, CanalTormenta(255.0f * vida) });
    }

    // Destello del pinchazo sobre cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorTormentaMagnetica& e = m.estadosJugadores[i];
        if (!m.resultado.participantes[i].participo || e.eliminado || e.destelloGolpe <= 0.0f) continue;

        Vector3 p = jugadores[i].posicion;
        ResplandorFrenteTormenta({ p.x, p.y + 0.2f, p.z + 0.3f }, 1.4f * e.destelloGolpe + 0.3f, Color{ 220, 230, 255, CanalTormenta(170.0f * e.destelloGolpe) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJAR
//==================================================

// Torres de alta tension del fondo con sus cables colgando.
static void DibujarTendidoTormenta()
{
    const Vector3 torres[5] =
    {
        { -46.0f, 0.0f, -38.0f }, { -28.0f, 0.0f, -45.0f }, { -6.0f, 0.0f, -50.0f },
        { 14.0f, 0.0f, -47.0f }, { 36.0f, 0.0f, -40.0f }
    };

    Vector3 brazos[5][2];

    for (int k = 0; k < 5; k++)
    {
        float alto = 20.0f + 2.5f * Hash01Tormenta(k, 500);
        float giro = 0.25f * (Hash01Tormenta(k, 501) - 0.5f);
        TorreAltaTensionTormenta({ torres[k].x, NIVEL_NUBES_TORMENTA, torres[k].z }, alto, giro, brazos[k]);
    }

    for (int k = 0; k < 4; k++)
    {
        for (int lado = 0; lado < 2; lado++)
        {
            Vector3 a = brazos[k][lado];
            Vector3 b = brazos[k + 1][lado];
            Color cable = TinteTormenta(Color{ 30, 30, 40, 255 }, a);
            Vector3 anterior = a;

            for (int s = 1; s <= 12; s++)
            {
                float u = (float)s / 12.0f;
                Vector3 p = { a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u - 2.2f * 4.0f * u * (1.0f - u), a.z + (b.z - a.z) * u };
                DrawLine3D(anterior, p, cable);
                anterior = p;
            }
        }
    }
}


// Jirones de nube baja que pasan bajo la cubierta y detras de ella.
static void DibujarNubesBajasTormenta(float t, int paso)
{
    for (int k = 0; k < 12; k += paso)
    {
        float vel = 0.9f + 0.8f * Hash01Tormenta(k, 520);
        float x = std::fmod(Hash01Tormenta(k, 521) * 70.0f + t * vel, 70.0f) - 35.0f;
        bool detras = k % 3 == 0;
        float y = detras ? 0.5f + 2.0f * Hash01Tormenta(k, 522) : -3.5f - 4.0f * Hash01Tormenta(k, 522);
        float z = detras ? -20.0f - 14.0f * Hash01Tormenta(k, 523) : -14.0f + 18.0f * Hash01Tormenta(k, 523);

        // Que no tapen la cubierta desde la camara.
        if (!detras && std::fabs(x) < 8.0f && z > -9.0f) z = -9.0f - 4.0f * Hash01Tormenta(k, 524);

        float rx = 4.0f + 4.0f * Hash01Tormenta(k, 525);
        Color claro = MezclaTormenta(COLOR_NUBE_TORMENTA, COLOR_NUBE_CLARA_TORMENTA, 0.6f);
        ElipsoideTormenta({ x, y, z }, rx, 0.8f + 0.6f * Hash01Tormenta(k, 526), rx * 0.5f, claro, COLOR_NUBE_OSCURA_TORMENTA, 4, 10);
        ElipsoideTormenta({ x + rx * 0.6f, y + 0.3f, z + 0.4f }, rx * 0.55f, 0.7f, rx * 0.35f, claro, COLOR_NUBE_OSCURA_TORMENTA, 4, 8);
    }
}


static void DibujarEscenarioTormenta(const MinijuegoTormentaMagnetica& m, const RelampagoTormenta& fondo)
{
    float t = m.tiempoVisual;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Horizonte y mar de nubes.
    DibujarMarNubesTormenta(t, fondo);

    if (decoracion)
    {
        DibujarHorizonteTormenta(fondo);

        // Media distancia: tendido electrico, radar, plataforma vecina y
        // las dos torres que flanquean la prueba.
        DibujarTendidoTormenta();
        RadarTormenta({ -20.0f, 1.6f, -28.0f }, t);
        PlataformaVecinaTormenta({ 24.0f, -1.0f, -30.0f });
        TorreControlTormenta();
        TorreBobinaTormenta();
        DibujarNubesBajasTormenta(t, paso);
    }

    // Arena: cubierta con su estructura.
    DibujarCubiertaTormenta(TonoTormenta(ColorBordeTormenta(m), 0.85f));

    if (decoracion)
    {
        // Anillo cercano: mensulas con bobinas, focos, pararrayos,
        // condensadores, aisladores, carretes, gabinetes y cables.
        DibujarMensulasTormenta(t, paso);
    }
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
    (void)cantidadMaxima;

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    const float t = tiempoVisual;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    RelampagoTormenta fondo = RelampagoFondoTormenta(t);
    RelampagoTormenta rayoCampo = RelampagoCampoTormenta(*this);
    destelloTormenta = fondo.intensidad * 0.55f;
    if (rayoCampo.intensidad > destelloTormenta) destelloTormenta = rayoCampo.intensidad;

    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    Camera3D vista = camara;

    if (fase == FASE_MAGNETICA_PREPARACION)
    {
        float u = Limitar01VisualTormenta(tiempoPreparacion / DURACION_PREPARACION_MAGNETICA);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 3.0f * suave;
        vista.position.z += 4.5f * suave;
        vista.target.y += 1.0f * suave;
    }

    ClearBackground(COLOR_NIEBLA_TORMENTA);
    DibujarCieloTormenta(vista, t);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarEscenarioTormenta(*this, fondo);
    DibujarNucleoTormenta(*this, t);

    for (int i = 0; i < MAX_PINCHOS_TORMENTA_MAGNETICA; i++)
    {
        if (pinchos[i].activo) DibujarPinchoTormenta(pinchos[i]);
    }

    DibujarTranslucidosTormenta(*this, jugadores, participantes);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || estadosJugadores[i].eliminado)
        {
            continue;
        }

        // Sacudida corta al recibir un pinchazo.
        const EstadoJugadorTormentaMagnetica& e = estadosJugadores[i];
        if (e.destelloGolpe > 0.0f)
        {
            JugadorPrueba sacudido = jugadores[i];
            sacudido.posicion.x += std::sin(t * 70.0f) * 0.06f * e.destelloGolpe;
            DibujarJugadorModeloCompartido(sacudido, participantes[i]);
        }
        else
        {
            DibujarJugadorModeloCompartido(jugadores[i], participantes[i]);
        }

        if (mostrarDebug && !jugadores[i].cayendo)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    if (mostrarDebug) DrawBoundingBox(CrearHitboxBloquePrueba(suelo), YELLOW);

    DibujarCaidasTormenta(*this, participantes);
    DibujarParticulasTierra(particulas, cantidadParticulas);
    DibujarParticulasTormenta(*this);
    DibujarLluviaTormenta(t);
    DibujarLucesTormenta(*this, jugadores, fondo, rayoCampo);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Fogonazo del relampago sobre toda la imagen.
    if (destelloTormenta > 0.01f && CalidadEfectos() != CALIDAD_BAJA)
    {
        DrawRectangle(0, 0, ancho, alto, Color{ 200, 210, 255, CanalTormenta(32.0f * destelloTormenta) });
    }

    // Rotulos J1..J4 sobre los que siguen en la plataforma.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado ||
            jugadores[i].cayendo ||
            !participantes[i].activo
        )
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.75f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // «¡J# AL VACÍO!» donde cayo cada eliminado.
    for (int i = 0; i < MAX_AVISOS_TORMENTA_MAGNETICA; i++)
    {
        const AvisoTormentaMagnetica& aviso = avisos[i];
        if (!aviso.activo) continue;

        float u = 1.0f - aviso.vida / DURACION_AVISO_TORMENTA;
        Vector3 p = { aviso.posicion.x, aviso.posicion.y + 0.6f + u * 1.2f, aviso.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);

        float limiteAbajo = (float)(alto - MargenBarraRondaMinijuegos()) - Px(70.0f);
        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.y > limiteAbajo) s.y = limiteAbajo;
        if (s.x < Px(150.0f)) s.x = Px(150.0f);
        if (s.x > (float)ancho - Px(150.0f)) s.x = (float)ancho - Px(150.0f);

        float alfa = aviso.vida < 0.45f ? aviso.vida / 0.45f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        char texto[32];
        std::snprintf(texto, sizeof(texto), "¡J%d AL VACÍO!", participantes[aviso.jugador].numeroJugador);
        DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, participantes[aviso.jugador].color, alfa);
    }

    // Anuncios del campo.
    if (fase == FASE_MAGNETICA_JUGANDO)
    {
        float yAnuncio = (float)alto * 0.2f;

        if (tiempoHastaCambioCampo < 0.9f)
        {
            float pulso = 0.8f + 0.2f * std::sin(t * 18.0f);
            DibujarTextoMundoContornoUI("¡CAMBIO DE CAMPO!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 255, 214, 80, 255 }, pulso);
        }
        else
        {
            float u = cambiosCampo > 0 ? tiempoDesdeCambio : tiempoJugado;
            float duracion = cambiosCampo > 0 ? 1.3f : 1.8f;

            if (u < duracion)
            {
                float alfa = u < 0.12f ? u / 0.12f : (u > duracion - 0.35f ? (duracion - u) / 0.35f : 1.0f);
                Color campo = MezclaTormenta(ColorCampoTormenta(campoAtrae), WHITE, 0.2f);
                DibujarTextoMundoContornoUI(campoAtrae ? "¡ATRACCIÓN!" : "¡REPULSIÓN!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, campo, alfa);
                DibujarTextoMundoContornoUI(
                    campoAtrae ? "EL NÚCLEO TE ARRASTRA HACIA ÉL" : "EL NÚCLEO TE EMPUJA HACIA EL BORDE",
                    (float)ancho * 0.5f,
                    yAnuncio + Px(40.0f),
                    UI_TEXTO,
                    UI_TEXTO_MUNDO,
                    alfa
                );
            }
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "TORMENTA MAGNÉTICA",
        "EL NÚCLEO ATRAE Y REPELE: RESISTÍ EL TIRÓN, ESQUIVÁ LOS PINCHOS Y NO CAIGAS AL VACÍO.",
        resultado
    );

    hud.fase = fase == FASE_MAGNETICA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_MAGNETICA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin limite de tiempo: el medidor muestra el campo y cuanto le queda.
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "CAMPO";
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%s", campoAtrae ? "ATRAE" : "REPELE");
    hud.progresoMedidor = fase == FASE_MAGNETICA_JUGANDO && duracionCampo > 0.0f
        ? Limitar01VisualTormenta(tiempoHastaCambioCampo / duracionCampo)
        : 1.0f;

    int vivos = ContarVivosMagnetica(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        const EstadoJugadorTormentaMagnetica& estado = estadosJugadores[i];
        j.eliminado = estado.eliminado;

        if (estado.eliminado)
        {
            int ms = estado.tiempoSobrevividoMs;
            std::snprintf(j.valor, sizeof(j.valor), "%d,%d S", ms / 1000, (ms % 1000) / 100);
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "CAYÓ A LOS %d,%d S", ms / 1000, (ms % 1000) / 100);
        }
        else
        {
            if (fase == FASE_MAGNETICA_JUGANDO && estado.destelloGolpe > 0.2f)
            {
                std::snprintf(j.estado, sizeof(j.estado), "¡PINCHAZO!");
            }

            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "EN PIE");
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "MOVÉ: %s  SALTÁ: %s\nGOLPE: %s",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (fase == FASE_MAGNETICA_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (vivos == 0)
        {
            hud.tituloFinal = "¡NADIE QUEDÓ EN LA PLATAFORMA!";
            hud.notaFinal = "CAYERON A LA VEZ: COMPARTEN PUESTO";
        }
        else if (vivos > 1)
        {
            hud.notaFinal = "SIGUEN EN PIE: COMPARTEN PUESTO";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoTormentaMagnetica::ObtenerResultado() const
{
    return resultado;
}
