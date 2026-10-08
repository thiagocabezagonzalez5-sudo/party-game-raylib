#include "Minigames/MinijuegoCargaInestable.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/Input.h"

#include <cmath>


//==================================================
// CARGA INESTABLE - FUNDICION / CALDERA DE VAPOR 3D
//==================================================
//
// Logica: los jugadores se numeran en orden alrededor de un anillo y la
// carga pasa al siguiente vivo (golpear = horario, saltar = antihorario).
// El fusible es aleatorio y oculto: el jugador solo ve la carga calentarse.
// Visual: funciones DibujarEscenarioCarga / DibujarCarga3D, separadas de
// la logica.
//==================================================


static const float DURACION_PREPARACION_CARGA = 2.5f;
static const float DURACION_EXPLOSION_CARGA = 1.35f;
static const float DURACION_VUELO_CARGA = 0.40f;
// Tras recibirla hay que esperar el vuelo mas un instante de agarre.
static const float BLOQUEO_RECEPCION_CARGA = 0.78f;
// Cada pase recalienta la carga y le quita mecha.
static const float RECALENTAMIENTO_PASE_CARGA = 0.2f;

// Anillo de plataformas.
static const float RADIO_ANILLO_CARGA = 6.6f;
static const float RADIO_PLATAFORMA_CARGA = 1.7f;
static const float ALTURA_PLATAFORMA_CARGA = 0.8f;


static int ContarVivosCarga(
    const MinijuegoCargaInestable& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int ElegirVivoAleatorioCarga(
    const MinijuegoCargaInestable& minijuego
)
{
    int vivos[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            vivos[cantidad] = i;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return -1;
    }

    return vivos[GetRandomValue(0, cantidad - 1)];
}


static int BuscarSiguienteVivoCarga(
    const MinijuegoCargaInestable& minijuego,
    int origen,
    int direccion
)
{
    int indice = origen;

    for (int intento = 0; intento < MAX_PARTICIPANTES; intento++)
    {
        indice += direccion;

        if (indice < 0) indice = MAX_PARTICIPANTES - 1;
        if (indice >= MAX_PARTICIPANTES) indice = 0;

        if (
            minijuego.resultado.participantes[indice].participo &&
            !minijuego.jugadores[indice].eliminado
        )
        {
            return indice;
        }
    }

    return origen;
}


// Posicion del participante en el anillo (0 = arriba, sentido horario).
static int OrdenCarga(
    const MinijuegoCargaInestable& minijuego,
    int indice
)
{
    int orden = 0;

    for (int i = 0; i < indice; i++)
    {
        if (minijuego.resultado.participantes[i].participo)
        {
            orden++;
        }
    }

    return orden;
}


static int CantidadActivosCarga(
    const MinijuegoCargaInestable& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.resultado.participantes[i].participo)
        {
            cantidad++;
        }
    }

    return cantidad < 1 ? 1 : cantidad;
}


static float AnguloPlataformaCarga(
    const MinijuegoCargaInestable& minijuego,
    int indice
)
{
    return
        -PI / 2.0f +
        OrdenCarga(minijuego, indice) * PI * 2.0f /
        CantidadActivosCarga(minijuego);
}


// Centro de la plataforma (a ras del suelo).
static Vector3 PosicionPlataformaCarga(
    const MinijuegoCargaInestable& minijuego,
    int indice
)
{
    float angulo = AnguloPlataformaCarga(minijuego, indice);

    return Vector3
    {
        std::cos(angulo) * RADIO_ANILLO_CARGA,
        0.0f,
        std::sin(angulo) * RADIO_ANILLO_CARGA
    };
}


// Donde flota la carga cuando un jugador la sostiene (entre el y la caldera).
static Vector3 PosicionCargaSostenida(
    const MinijuegoCargaInestable& minijuego,
    int indice,
    float tiempo
)
{
    Vector3 plataforma = PosicionPlataformaCarga(minijuego, indice);

    return Vector3
    {
        plataforma.x * 0.72f,
        2.3f + std::sin(tiempo * 6.0f) * 0.12f,
        plataforma.z * 0.72f
    };
}


static void CrearExplosionCarga(
    MinijuegoCargaInestable& minijuego,
    Vector3 centro
)
{
    int creadas = 0;

    for (int i = 0; i < MAX_PARTICULAS_CARGA && creadas < 60; i++)
    {
        ParticulaCarga& particula = minijuego.particulas[i];

        if (particula.activa)
        {
            continue;
        }

        float angulo = GetRandomValue(0, 628) / 100.0f;
        float velocidadHorizontal = GetRandomValue(200, 800) / 100.0f;
        int tipo = GetRandomValue(0, 3);

        particula.activa = true;
        particula.posicion = centro;
        particula.velocidad =
        {
            std::cos(angulo) * velocidadHorizontal,
            GetRandomValue(300, 900) / 100.0f,
            std::sin(angulo) * velocidadHorizontal
        };
        particula.vidaMaxima = GetRandomValue(60, 120) / 100.0f;
        particula.vida = particula.vidaMaxima;
        particula.color =
            tipo == 0 ? Color{ 255, 220, 90, 255 }
            : (tipo == 1 ? Color{ 255, 130, 40, 255 }
            : (tipo == 2 ? Color{ 255, 70, 50, 255 }
            : Color{ 120, 120, 130, 255 }));
        creadas++;
    }
}


static void PrepararCargaNueva(
    MinijuegoCargaInestable& minijuego,
    int portadorInicial
)
{
    minijuego.numeroRonda++;
    minijuego.portador = portadorInicial;
    minijuego.portadorExplosion = -1;
    minijuego.duracionCarga = GetRandomValue(590, 980) / 100.0f;
    minijuego.tiempoCarga = minijuego.duracionCarga;
    minijuego.bloqueoPase = 0.48f;
    minijuego.tiempoExplosion = 0.0f;
    minijuego.fase = FASE_CARGA_ACTIVA;
    minijuego.origenPase = -1;
    minijuego.progresoPase = 0.0f;
    minijuego.tiempoTick = 0.6f;

    if (portadorInicial >= 0)
    {
        minijuego.jugadores[portadorInicial].tiempoDecisionBot =
            GetRandomValue(32, 105) / 100.0f;
    }
}


static void PasarCarga(
    MinijuegoCargaInestable& minijuego,
    int direccion
)
{
    if (minijuego.portador < 0 || minijuego.bloqueoPase > 0.0f)
    {
        return;
    }

    int anterior = minijuego.portador;
    int siguiente = BuscarSiguienteVivoCarga(
        minijuego,
        anterior,
        direccion
    );

    if (siguiente == anterior)
    {
        return;
    }

    minijuego.jugadores[anterior].cantidadPases++;
    minijuego.portador = siguiente;
    minijuego.bloqueoPase = BLOQUEO_RECEPCION_CARGA;
    minijuego.tiempoCarga -= RECALENTAMIENTO_PASE_CARGA;
    minijuego.origenPase = anterior;
    minijuego.progresoPase = 0.0f;
    minijuego.jugadores[siguiente].tiempoDecisionBot =
        GetRandomValue(5, 70) / 100.0f +
        (GetRandomValue(1, 100) <= 12 ? GetRandomValue(50, 100) / 100.0f : 0.0f);

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);
}


static void FinalizarCargaInestable(
    MinijuegoCargaInestable& minijuego
)
{
    int ganador = ElegirVivoAleatorioCarga(minijuego);

    if (ganador >= 0)
    {
        minijuego.jugadores[ganador].posicionFinal = 1;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        resultadoJugador.posicionFinal =
            minijuego.jugadores[i].posicionFinal;

        resultadoJugador.puntuacionMinijuego =
            minijuego.jugadores[i].cantidadPases * 5 +
            (MAX_PARTICIPANTES + 1 - resultadoJugador.posicionFinal) * 20;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.fase = FASE_CARGA_TERMINADO;
    minijuego.portador = ganador;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


void MinijuegoCargaInestable::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_CARGA; i++)
    {
        particulas[i] = {};
    }

    fase = FASE_CARGA_PREPARACION;
    portador = -1;
    portadorExplosion = -1;
    numeroRonda = 0;
    origenPase = -1;
    progresoPase = 1.0f;
    tiempoTick = 0.0f;
    tiempoPreparacion = DURACION_PREPARACION_CARGA;
    tiempoCarga = 0.0f;
    duracionCarga = 0.0f;
    bloqueoPase = 0.0f;
    tiempoExplosion = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoCargaInestable::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    portador = ElegirVivoAleatorioCarga(*this);
}


void MinijuegoCargaInestable::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    progresoPase += deltaTime / DURACION_VUELO_CARGA;
    if (progresoPase > 1.0f) progresoPase = 1.0f;

    for (int i = 0; i < MAX_PARTICULAS_CARGA; i++)
    {
        ParticulaCarga& particula = particulas[i];

        if (!particula.activa)
        {
            continue;
        }

        particula.velocidad.y -= 9.0f * deltaTime;
        particula.posicion = Vector3Add(
            particula.posicion,
            Vector3Scale(particula.velocidad, deltaTime)
        );
        particula.vida -= deltaTime;

        if (particula.vida <= 0.0f || particula.posicion.y < 0.0f)
        {
            particula.activa = false;
        }
    }

    if (fase == FASE_CARGA_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CARGA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            PrepararCargaNueva(*this, portador);
        }

        return;
    }

    if (fase == FASE_CARGA_EXPLOSION)
    {
        tiempoExplosion -= deltaTime;

        if (tiempoExplosion <= 0.0f)
        {
            if (ContarVivosCarga(*this) <= 1)
            {
                FinalizarCargaInestable(*this);
            }
            else
            {
                PrepararCargaNueva(
                    *this,
                    ElegirVivoAleatorioCarga(*this)
                );
            }
        }

        return;
    }

    bloqueoPase -= deltaTime;
    if (bloqueoPase < 0.0f) bloqueoPase = 0.0f;

    tiempoCarga -= deltaTime;

    // Tic-tic cada vez mas rapido: no revela el tiempo exacto.
    tiempoTick -= deltaTime;

    if (tiempoTick <= 0.0f)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ALERTA_TIEMPO);
        tiempoTick = Clamp(tiempoCarga * 0.22f, 0.14f, 0.7f);
    }

    if (tiempoCarga <= 0.0f)
    {
        int vivosAntes = ContarVivosCarga(*this);

        if (portador >= 0)
        {
            jugadores[portador].eliminado = true;
            jugadores[portador].posicionFinal = vivosAntes;
            CrearExplosionCarga(
                *this,
                PosicionCargaSostenida(*this, portador, tiempoAnimacion)
            );
        }

        portadorExplosion = portador;
        portador = -1;
        fase = FASE_CARGA_EXPLOSION;
        tiempoExplosion = DURACION_EXPLOSION_CARGA;

        ReproducirSonidoMinijuego(audio, SONIDO_EXPLOSION);
        ReproducirSonidoMinijuego(audio, SONIDO_ELIMINADO);
        return;
    }

    if (portador < 0 || bloqueoPase > 0.0f)
    {
        return;
    }

    if (participantes[portador].esBot || !participantes[portador].conectado)
    {
        // Como un humano: reacciona mas rapido cuanto mas caliente esta.
        float calor = duracionCarga > 0.0f
            ? 1.0f - Clamp(tiempoCarga / duracionCarga, 0.0f, 1.0f)
            : 0.0f;
        jugadores[portador].tiempoDecisionBot -= deltaTime * (1.0f + 1.5f * calor);

        if (jugadores[portador].tiempoDecisionBot <= 0.0f)
        {
            int direccion = GetRandomValue(0, 1) == 0 ? -1 : 1;

            // Con tres o mas vivos no se la devuelve a quien se la acaba de dar.
            if (
                ContarVivosCarga(*this) >= 3 &&
                BuscarSiguienteVivoCarga(*this, portador, direccion) == origenPase
            )
            {
                direccion = -direccion;
            }

            PasarCarga(*this, direccion);
        }

        return;
    }

    InputMinijuegoParticipante entrada =
        LeerInputMinijuegoParticipante(participantes[portador]);

    if (entrada.golpear)
    {
        PasarCarga(*this, 1);
    }
    else if (entrada.saltar)
    {
        PasarCarga(*this, -1);
    }
}


//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB la caldera central, las plataformas
// y puentes, las chimeneas y tuberias, el muro de hornos, y la esfera de
// carga con su carcasa agrietada.
//==================================================


static Camera3D ObtenerCamaraCarga(float temblor)
{
    Camera3D camara{};
    camara.position = { temblor, 16.0f + temblor * 0.5f, 12.5f };
    camara.target = { 0.0f, 0.8f, 0.8f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


static void DibujarEscenarioCarga(float tiempo)
{
    // Suelo de la fundicion y piso del anillo.
    DrawCube({ 0.0f, -0.3f, 0.0f }, 46.0f, 0.4f, 34.0f, Color{ 28, 22, 26, 255 });
    DrawCylinder({ 0.0f, -0.1f, 0.0f }, 10.5f, 10.5f, 0.2f, 40, Color{ 48, 38, 44, 255 });
    DrawCircle3D({ 0.0f, 0.12f, 0.0f }, 10.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 255, 140, 60, 255 }, 0.55f));
    DrawCircle3D({ 0.0f, 0.12f, 0.0f }, 9.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 255, 140, 60, 255 }, 0.25f));

    // Caldera central con lava burbujeante.
    float brillo = 0.8f + 0.2f * std::sin(tiempo * 3.0f);
    DrawCylinder({ 0.0f, 0.0f, 0.0f }, 3.1f, 3.3f, 0.6f, 28, Color{ 70, 60, 68, 255 });
    DrawCylinder(
        { 0.0f, 0.02f, 0.0f },
        2.7f,
        2.7f,
        0.62f,
        28,
        Color{ (unsigned char)(255 * brillo), (unsigned char)(110 * brillo), 30, 255 }
    );

    for (int b = 0; b < 5; b++)
    {
        float fase = std::fmod(tiempo * 0.7f + b * 0.21f, 1.0f);
        DrawSphereEx(
            { std::cos(b * 1.9f) * 1.5f, 0.62f + fase * 0.2f, std::sin(b * 1.9f) * 1.5f },
            0.18f * (1.0f - fase * 0.5f),
            6,
            6,
            Fade(Color{ 255, 220, 120, 255 }, 1.0f - fase)
        );
    }

    // Vapor que sube de la caldera.
    for (int k = 0; k < 6; k++)
    {
        float f = std::fmod(tiempo * 0.35f + k * 0.17f, 1.0f);
        DrawSphereEx(
            { std::cos(k * 1.05f) * 1.7f, 1.0f + f * 4.2f, std::sin(k * 1.05f) * 1.7f },
            0.3f + f * 0.55f,
            6,
            6,
            Fade(Color{ 190, 190, 200, 255 }, 0.28f * (1.0f - f))
        );
    }

    // Muro de hornos al fondo, con bocas encendidas.
    DrawCube({ 0.0f, 4.0f, -14.0f }, 46.0f, 9.0f, 1.0f, Color{ 52, 40, 44, 255 });

    for (int h = 0; h < 7; h++)
    {
        float x = -15.0f + h * 5.0f;
        float parpadeo = 0.7f + 0.3f * std::sin(tiempo * 4.0f + h * 1.9f);
        DrawCube({ x, 2.0f, -13.4f }, 2.6f, 2.6f, 0.4f, Color{ 30, 24, 28, 255 });
        DrawCube(
            { x, 1.8f, -13.15f },
            2.0f,
            1.8f,
            0.2f,
            Color{ (unsigned char)(255 * parpadeo), (unsigned char)(120 * parpadeo), 30, 255 }
        );
        DrawCube({ x, 6.2f, -13.4f }, 3.0f, 0.5f, 0.3f, Color{ 80, 70, 76, 255 });
    }

    // Chimeneas laterales y tuberias con manometros.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = lado * 17.0f;
        DrawCylinder({ x, 0.0f, -9.0f }, 1.3f, 1.6f, 14.0f, 12, Color{ 62, 52, 58, 255 });
        DrawCylinder({ x, 14.0f, -9.0f }, 1.6f, 1.6f, 0.5f, 12, Color{ 90, 70, 60, 255 });

        for (int k = 0; k < 3; k++)
        {
            float f = std::fmod(tiempo * 0.3f + k * 0.33f + (lado > 0 ? 0.1f : 0.0f), 1.0f);
            DrawSphereEx(
                { x + std::sin(f * 5.0f + k) * 0.6f, 14.5f + f * 4.0f, -9.0f },
                0.8f + f * 1.0f,
                6,
                6,
                Fade(Color{ 150, 150, 160, 255 }, 0.35f * (1.0f - f))
            );
        }

        DrawCylinderEx({ lado * 12.0f, 0.5f, -12.0f }, { lado * 12.0f, 7.5f, -12.0f }, 0.35f, 0.35f, 8, Color{ 120, 90, 60, 255 });
        DrawCylinderEx({ lado * 12.0f, 7.5f, -12.0f }, { lado * 17.0f, 7.5f, -12.0f }, 0.35f, 0.35f, 8, Color{ 120, 90, 60, 255 });
        DrawSphereEx({ lado * 12.0f, 4.0f, -11.5f }, 0.4f, 8, 8, Color{ 230, 220, 190, 255 });
    }
}


static void DibujarPlataformasCarga(
    const MinijuegoCargaInestable& minijuego,
    const Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCargaInestable& estado = minijuego.jugadores[i];
        Vector3 centro = PosicionPlataformaCarga(minijuego, i);
        float angulo = AnguloPlataformaCarga(minijuego, i);
        Color color = participantes[i].color;
        float altura = estado.eliminado ? 0.5f : ALTURA_PLATAFORMA_CARGA;

        // Puente desde la caldera.
        float radioMedio = (3.0f + RADIO_ANILLO_CARGA - RADIO_PLATAFORMA_CARGA) * 0.5f;
        float largo = RADIO_ANILLO_CARGA - RADIO_PLATAFORMA_CARGA - 3.0f + 0.6f;

        rlPushMatrix();
        rlTranslatef(std::cos(angulo) * radioMedio, 0.55f, std::sin(angulo) * radioMedio);
        rlRotatef(-angulo * RAD2DEG, 0.0f, 1.0f, 0.0f);
        DrawCube({ 0.0f, 0.0f, 0.0f }, largo, 0.16f, 0.9f, Color{ 90, 84, 92, 255 });
        DrawCube({ 0.0f, 0.1f, 0.5f }, largo, 0.08f, 0.08f, Fade(Color{ 255, 150, 60, 255 }, 0.7f));
        DrawCube({ 0.0f, 0.1f, -0.5f }, largo, 0.08f, 0.08f, Fade(Color{ 255, 150, 60, 255 }, 0.7f));
        rlPopMatrix();

        // Plataforma: chamuscada si el jugador exploto.
        Color metal = estado.eliminado ? Color{ 30, 26, 28, 255 } : Color{ 96, 90, 100, 255 };
        DrawCylinder(centro, RADIO_PLATAFORMA_CARGA, RADIO_PLATAFORMA_CARGA + 0.2f, altura, 20, metal);
        DrawCircle3D(
            { centro.x, altura + 0.02f, centro.z },
            RADIO_PLATAFORMA_CARGA * 0.82f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            estado.eliminado ? Fade(RED, 0.5f) : color
        );
        DrawCircle3D(
            { centro.x, altura + 0.02f, centro.z },
            RADIO_PLATAFORMA_CARGA * 0.7f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            estado.eliminado ? Fade(ORANGE, 0.35f) : WHITE
        );

        // Jugador sobre la plataforma (aplastado si exploto).
        JugadorPrueba figura{};
        figura.posicion = { centro.x, altura + figura.tamano.y * 0.5f, centro.z };
        figura.direccionMirada = Vector3Normalize({ -centro.x, 0.0f, -centro.z });
        figura.enSuelo = true;
        figura.aplastado = estado.eliminado;

        Participante visual = participantes[i];
        visual.activo = true;
        visual.conectado = true;
        DibujarJugadorCuboPrueba(figura, visual);

        // Flechas animadas que indican hacia donde puede pasar el portador.
        if (minijuego.fase == FASE_CARGA_ACTIVA && i == minijuego.portador && minijuego.bloqueoPase <= 0.0f)
        {
            Vector3 tangente = { -std::sin(angulo), 0.0f, std::cos(angulo) };

            for (int sentido = -1; sentido <= 1; sentido += 2)
            {
                Color colorFlecha = sentido > 0 ? GOLD : VIOLET;

                for (int k = 0; k < 3; k++)
                {
                    float onda = std::fmod(minijuego.tiempoAnimacion * 2.5f - k * 0.33f + 3.0f, 1.0f);
                    float distancia = RADIO_PLATAFORMA_CARGA + 0.45f + k * 0.4f;
                    DrawSphereEx(
                        {
                            centro.x + tangente.x * sentido * distancia,
                            altura + 0.3f,
                            centro.z + tangente.z * sentido * distancia
                        },
                        0.17f - k * 0.025f,
                        6,
                        6,
                        Fade(colorFlecha, 0.35f + 0.65f * (1.0f - onda))
                    );
                }
            }
        }
    }
}


static void DibujarCarga3D(
    Vector3 posicion,
    float calor,
    float tiempo,
    bool inerte
)
{
    float pulso = std::sin(tiempo * (5.0f + calor * 14.0f));
    float radio = 0.45f + calor * 0.3f + pulso * 0.04f;
    Color nucleo = inerte
        ? Color{ 92, 211, 255, 255 }
        : ColorLerp(Color{ 92, 211, 255, 255 }, Color{ 255, 50, 40, 255 }, calor);

    DrawSphereEx(posicion, radio * 1.8f, 12, 12, Fade(nucleo, 0.18f + 0.1f * calor));
    DrawSphereEx(posicion, radio, 14, 14, nucleo);
    DrawSphereEx(posicion, radio * 0.5f, 8, 8, WHITE);

    // Aros metalicos que giran alrededor de la carga.
    rlPushMatrix();
    rlTranslatef(posicion.x, posicion.y, posicion.z);
    rlRotatef(tiempo * 90.0f, 0.0f, 1.0f, 0.0f);
    DrawCircle3D({ 0.0f, 0.0f, 0.0f }, radio * 1.25f, { 1.0f, 0.0f, 0.0f }, 90.0f, RAYWHITE);
    DrawCircle3D({ 0.0f, 0.0f, 0.0f }, radio * 1.25f, { 0.0f, 0.0f, 1.0f }, 90.0f, Fade(RAYWHITE, 0.7f));
    rlPopMatrix();

    // Sombra / marca en el suelo.
    DrawCircle3D({ posicion.x, 0.14f, posicion.z }, 0.6f + calor * 0.3f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(nucleo, 0.5f));
}


void MinijuegoCargaInestable::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 20, 12, 18, 255 });

    float progresoExplosion = 1.0f - Clamp(
        tiempoExplosion / DURACION_EXPLOSION_CARGA,
        0.0f,
        1.0f
    );
    float temblor = fase == FASE_CARGA_EXPLOSION
        ? std::sin(tiempoAnimacion * 70.0f) * 0.3f * (1.0f - progresoExplosion)
        : 0.0f;

    Camera3D camara = ObtenerCamaraCarga(temblor);
    BeginMode3D(camara);

    DibujarEscenarioCarga(tiempoAnimacion);
    DibujarPlataformasCarga(*this, participantes);

    // La carga: dormida en la caldera, en vuelo con arco o sostenida.
    float calor = duracionCarga > 0.0f
        ? 1.0f - Clamp(tiempoCarga / duracionCarga, 0.0f, 1.0f)
        : 0.0f;
    Vector3 posicionCarga{};
    bool dibujarCarga = false;

    if (fase == FASE_CARGA_PREPARACION)
    {
        posicionCarga = { 0.0f, 2.4f + std::sin(tiempoAnimacion * 3.0f) * 0.15f, 0.0f };
        dibujarCarga = true;
    }
    else if (fase == FASE_CARGA_ACTIVA && portador >= 0)
    {
        Vector3 destino = PosicionCargaSostenida(*this, portador, tiempoAnimacion);

        if (progresoPase < 1.0f)
        {
            Vector3 origen = origenPase >= 0
                ? PosicionCargaSostenida(*this, origenPase, tiempoAnimacion)
                : Vector3{ 0.0f, 1.4f, 0.0f };
            float t = progresoPase;
            posicionCarga = Vector3Lerp(origen, destino, t);
            posicionCarga.y += std::sin(t * PI) * 2.2f;
        }
        else
        {
            posicionCarga = destino;
        }

        dibujarCarga = true;
    }

    if (dibujarCarga)
    {
        DibujarCarga3D(posicionCarga, fase == FASE_CARGA_PREPARACION ? 0.0f : calor, tiempoAnimacion, fase == FASE_CARGA_PREPARACION);
    }

    // Explosion: onda expansiva y aros sobre la plataforma del jugador.
    if (fase == FASE_CARGA_EXPLOSION && portadorExplosion >= 0)
    {
        Vector3 centro = PosicionCargaSostenida(*this, portadorExplosion, 0.0f);
        float p = progresoExplosion;

        DrawSphereEx(centro, 0.8f + p * 4.0f, 14, 14, Fade(ORANGE, 0.5f * (1.0f - p)));
        DrawSphereEx(centro, 0.4f + p * 2.2f, 10, 10, Fade(Color{ 255, 240, 160, 255 }, 0.8f * (1.0f - p)));
        DrawCircle3D({ centro.x, 0.2f, centro.z }, 1.0f + p * 6.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(GOLD, 1.0f - p));
        DrawCircle3D({ centro.x, 0.2f, centro.z }, 0.6f + p * 4.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(RED, 1.0f - p));
    }

    for (int i = 0; i < MAX_PARTICULAS_CARGA; i++)
    {
        const ParticulaCarga& particula = particulas[i];

        if (!particula.activa)
        {
            continue;
        }

        DrawSphereEx(
            particula.posicion,
            0.08f + 0.12f * (particula.vida / particula.vidaMaxima),
            4,
            4,
            Fade(particula.color, particula.vida / particula.vidaMaxima)
        );
    }

    EndMode3D();

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Destello blanco al estallar.
    if (fase == FASE_CARGA_EXPLOSION && progresoExplosion < 0.3f)
    {
        DrawRectangle(0, 0, ancho, alto, Fade(WHITE, 0.5f * (1.0f - progresoExplosion / 0.3f)));
    }

    // Etiquetas de jugador sobre cada plataforma.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 centro = PosicionPlataformaCarga(*this, i);
        Vector2 pantalla = GetWorldToScreen({ centro.x, 3.0f, centro.z }, camara);
        const char* texto = jugadores[i].eliminado
            ? TextFormat("J%d  X", participantes[i].numeroJugador)
            : TextFormat("J%d", participantes[i].numeroJugador);

        DrawText(
            texto,
            (int)pantalla.x - MeasureText(texto, 22) / 2,
            (int)pantalla.y,
            22,
            jugadores[i].eliminado ? RED : participantes[i].color
        );
    }

    // Medidor de calor: da pista del peligro sin revelar el tiempo exacto.
    if (fase == FASE_CARGA_ACTIVA)
    {
        int barraX = ancho / 2 - 160;
        int barraY = alto - 46;
        DrawRectangle(barraX - 2, barraY - 2, 324, 20, Fade(BLACK, 0.7f));
        DrawRectangle(
            barraX,
            barraY,
            (int)(320.0f * calor),
            16,
            ColorLerp(Color{ 92, 211, 255, 255 }, RED, calor)
        );
        DrawText("CALOR DE LA CARGA", barraX, barraY - 24, 18, RAYWHITE);
    }

    DrawRectangle(18, 16, 540, 130, Fade(BLACK, 0.78f));
    DrawText("CARGA INESTABLE", 32, 28, 30, GOLD);

    if (fase == FASE_CARGA_PREPARACION)
    {
        DrawText("PREPARATE", 32, 68, 22, RAYWHITE);
        DrawText(
            TextFormat("EMPIEZA EN %.1f", tiempoPreparacion),
            32,
            100,
            18,
            LIGHTGRAY
        );

        const char* numero = TextFormat("%d", (int)std::ceil(tiempoPreparacion));
        DrawText(numero, ancho / 2 - MeasureText(numero, 120) / 2, alto / 2 - 60, 120, Fade(GOLD, 0.9f));
    }
    else if (fase == FASE_CARGA_ACTIVA)
    {
        DrawText("GOLPEAR: HORARIO   SALTAR: ANTIHORARIO", 32, 68, 18, RAYWHITE);
        DrawText(
            TextFormat("RONDA %d  -  PASALA ANTES DE QUE EXPLOTE", numeroRonda),
            32,
            94,
            17,
            LIGHTGRAY
        );
        DrawText(
            bloqueoPase > 0.0f
                ? "LA CARGA VUELA Y NO SE PUEDE PASAR AL INSTANTE..."
                : "CADA PASE TARDA EN LLEGAR Y RECALIENTA LA CARGA",
            32,
            120,
            16,
            bloqueoPase > 0.0f ? ORANGE : Color{ 150, 215, 255, 255 }
        );
    }
    else if (fase == FASE_CARGA_EXPLOSION)
    {
        DrawText("SOBRECARGA!", 32, 74, 26, RED);
    }
    else
    {
        DrawText("ULTIMO EN PIE", 32, 68, 22, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), 32, 100, 18, LIGHTGRAY);
    }

    if (fase == FASE_CARGA_TERMINADO && portador >= 0)
    {
        const char* titulo = TextFormat("GANADOR: J%d", participantes[portador].numeroJugador);
        DrawRectangle(ancho / 2 - 240, alto / 2 - 50, 480, 90, Fade(BLACK, 0.75f));
        DrawText(titulo, ancho / 2 - MeasureText(titulo, 44) / 2, alto / 2 - 30, 44, participantes[portador].color);
    }
}


const ResultadoMinijuego&
MinijuegoCargaInestable::ObtenerResultado() const
{
    return resultado;
}
