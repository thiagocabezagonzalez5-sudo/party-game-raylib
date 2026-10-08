#include "Minigames/MinijuegoRacimoToxico.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


//==================================================
// RACIMO TOXICO
//==================================================
//
// Por turnos, cada jugador toma 1 o 2 frutos del racimo. Las bayas
// toxicas (posiciones fijas, visibles) quitan una vida. El fruto dorado
// hace que se salte el turno del siguiente jugador.
//==================================================


static const float DURACION_PREPARACION_RACIMO = 3.0f;
static const float DURACION_TURNO_RACIMO = 5.0f;
static const float DURACION_PAUSA_RACIMO = 1.3f;
static const float DURACION_PARTIDA_RACIMO = 75.0f;
static const float DURACION_VUELO_RACIMO = 0.9f;
static const float Z_BALSAS_RACIMO = 3.5f;
static const float SEPARACION_BALSAS_RACIMO = 5.0f;
static const float PI_RACIMO = 3.14159265f;
static const int POSICIONES_TOXICAS_RACIMO[3] = { 5, 11, 17 };


//==================================================
// LOGICA
//==================================================


static bool EsControlBotRacimo(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static int ContarVivosRacimo(const MinijuegoRacimoToxico& minijuego)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static int SiguienteVivoRacimo(const MinijuegoRacimoToxico& minijuego, int desde)
{
    for (int paso = 1; paso <= MAX_PARTICIPANTES; paso++)
    {
        int indice = (desde + paso) % MAX_PARTICIPANTES;

        if (
            minijuego.resultado.participantes[indice].participo &&
            minijuego.estadosJugadores[indice].vivo
        )
        {
            return indice;
        }
    }

    return desde;
}


static void ReponerRacimo(MinijuegoRacimoToxico& minijuego)
{
    for (int i = 0; i < TOTAL_FRUTAS_RACIMO; i++)
    {
        minijuego.tipoFruta[i] = FRUTA_RACIMO_NORMAL;
    }

    for (int posicion : POSICIONES_TOXICAS_RACIMO)
    {
        minijuego.tipoFruta[posicion] = FRUTA_RACIMO_TOXICA;
    }

    // Un fruto dorado aparece a veces (nunca sobre una baya).
    if (GetRandomValue(0, 99) < 65)
    {
        for (int intento = 0; intento < 32; intento++)
        {
            int indice = GetRandomValue(1, TOTAL_FRUTAS_RACIMO - 2);

            if (minijuego.tipoFruta[indice] == FRUTA_RACIMO_NORMAL)
            {
                minijuego.tipoFruta[indice] = FRUTA_RACIMO_DORADA;
                break;
            }
        }
    }

    minijuego.frente = 0;
}


static void IniciarTurnoRacimo(MinijuegoRacimoToxico& minijuego)
{
    minijuego.fase = FASE_RACIMO_TURNO;
    minijuego.tiempoTurno = DURACION_TURNO_RACIMO;
    minijuego.seleccion = 1;
    minijuego.tiempoBot = GetRandomValue(90, 180) / 100.0f;
    minijuego.accionPrevia = true;
}


static void FinalizarRacimo(MinijuegoRacimoToxico& minijuego)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int ganadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        const EstadoJugadorRacimoToxico& estado = minijuego.estadosJugadores[i];

        if (estado.vivo)
        {
            int posicion = 1;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    minijuego.resultado.participantes[j].participo &&
                    minijuego.estadosJugadores[j].vivo &&
                    minijuego.estadosJugadores[j].vidas > estado.vidas
                )
                {
                    posicion++;
                }
            }

            resultadoJugador.posicionFinal = posicion;
        }
        else
        {
            resultadoJugador.posicionFinal = estado.posicionEliminacion;
        }

        resultadoJugador.puntuacionMinijuego =
            estado.vidas * 100 + estado.frutasSeguras;

        if (resultadoJugador.posicionFinal == 1)
        {
            ganadores++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        ganadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_RACIMO_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static Vector3 PosicionFrutaRacimo(int indice, float tiempo)
{
    return Vector3{
        (indice % 2 == 0 ? -0.36f : 0.36f) + std::sin(tiempo * 1.5f + (float)indice) * 0.03f,
        1.5f + (float)indice * 0.42f,
        0.0f
    };
}


static void TomarFrutasRacimo(MinijuegoRacimoToxico& minijuego, int cantidad)
{
    int jugador = minijuego.turno;
    EstadoJugadorRacimoToxico& estado = minijuego.estadosJugadores[jugador];

    bool toxica = false;
    bool dorada = false;
    int tomadas = 0;

    for (int i = 0; i < MAX_VUELOS_RACIMO; i++)
    {
        minijuego.vuelos[i].activo = false;
    }

    for (int k = 0; k < cantidad && minijuego.frente < TOTAL_FRUTAS_RACIMO; k++)
    {
        int tipo = minijuego.tipoFruta[minijuego.frente];

        VueloFrutaRacimo& vuelo = minijuego.vuelos[k];
        vuelo.activo = true;
        vuelo.tipo = tipo;
        vuelo.jugador = jugador;
        vuelo.progreso = 0.0f;
        vuelo.origen = PosicionFrutaRacimo(minijuego.frente, minijuego.tiempoAnimacion);

        if (tipo == FRUTA_RACIMO_TOXICA)
            toxica = true;
        else if (tipo == FRUTA_RACIMO_DORADA)
            dorada = true;
        else
            estado.frutasSeguras++;

        minijuego.frente++;
        tomadas++;
    }

    minijuego.ultimoJugador = jugador;
    minijuego.ultimoEvento = EVENTO_RACIMO_SEGURO;
    minijuego.jugadorSaltado = -1;

    if (tomadas > 0)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RECOGER_OBJETO);
    }

    if (toxica)
    {
        int vivosAntes = ContarVivosRacimo(minijuego);
        estado.vidas--;
        estado.sacudida = 0.8f;

        if (estado.vidas <= 0)
        {
            estado.vidas = 0;
            estado.vivo = false;
            estado.posicionEliminacion = vivosAntes;
            minijuego.ultimoEvento = EVENTO_RACIMO_ELIMINADO;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
        }
        else
        {
            minijuego.ultimoEvento = EVENTO_RACIMO_TOXICA;
            ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        }
    }
    else if (dorada)
    {
        minijuego.ultimoEvento = EVENTO_RACIMO_DORADA;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
    }

    if (dorada && ContarVivosRacimo(minijuego) > 1)
    {
        minijuego.saltarSiguiente = true;
    }

    minijuego.fase = FASE_RACIMO_ANIMACION;
    minijuego.tiempoPausa = DURACION_PAUSA_RACIMO;
}


static int ElegirTomaBotRacimo(const MinijuegoRacimoToxico& minijuego)
{
    int restante = TOTAL_FRUTAS_RACIMO - minijuego.frente;

    if (restante <= 1)
    {
        return 1;
    }

    // Error aleatorio para que no sea perfecto.
    if (GetRandomValue(0, 99) < 25)
    {
        return GetRandomValue(1, 2);
    }

    int f = minijuego.frente;
    bool seguro1 = minijuego.tipoFruta[f] != FRUTA_RACIMO_TOXICA;
    bool seguro2 = seguro1 && minijuego.tipoFruta[f + 1] != FRUTA_RACIMO_TOXICA;

    if (!seguro1)
    {
        return 1;
    }

    // Dejar al siguiente frente a la baya.
    if (minijuego.tipoFruta[f + 1] == FRUTA_RACIMO_TOXICA)
    {
        return 1;
    }

    if (seguro2 && f + 2 < TOTAL_FRUTAS_RACIMO && minijuego.tipoFruta[f + 2] == FRUTA_RACIMO_TOXICA)
    {
        return 2;
    }

    if (minijuego.tipoFruta[f] == FRUTA_RACIMO_DORADA)
    {
        return 1;
    }

    if (seguro2 && minijuego.tipoFruta[f + 1] == FRUTA_RACIMO_DORADA)
    {
        return 2;
    }

    return seguro2 ? GetRandomValue(1, 2) : 1;
}


static void ActualizarTurnoRacimo(
    MinijuegoRacimoToxico& minijuego,
    float deltaTime,
    Participante participantes[]
)
{
    int restante = TOTAL_FRUTAS_RACIMO - minijuego.frente;
    int maximo = restante < 2 ? restante : 2;
    if (maximo < 1) maximo = 1;

    float antes = minijuego.tiempoTurno;
    minijuego.tiempoTurno -= deltaTime;
    ActualizarAudioAlertaTiempo(minijuego.audio, antes, minijuego.tiempoTurno, 2.0f);

    const Participante& participante = participantes[minijuego.turno];

    if (EsControlBotRacimo(participante))
    {
        minijuego.tiempoBot -= deltaTime;

        if (minijuego.tiempoBot <= 0.0f)
        {
            TomarFrutasRacimo(minijuego, ElegirTomaBotRacimo(minijuego));
            return;
        }
    }
    else
    {
        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participante);

        if (entrada.izquierda)
        {
            minijuego.seleccion = 1;
        }
        else if (entrada.derecha)
        {
            minijuego.seleccion = 2;
        }

        if (minijuego.seleccion > maximo)
        {
            minijuego.seleccion = maximo;
        }

        bool confirmar = entrada.golpear && !minijuego.accionPrevia;
        minijuego.accionPrevia = entrada.golpear;

        if (confirmar)
        {
            TomarFrutasRacimo(minijuego, minijuego.seleccion);
            return;
        }
    }

    if (minijuego.tiempoTurno <= 0.0f)
    {
        TomarFrutasRacimo(minijuego, 1);
    }
}


static void ActualizarAnimacionRacimo(MinijuegoRacimoToxico& minijuego, float deltaTime)
{
    minijuego.tiempoPausa -= deltaTime;

    if (minijuego.tiempoPausa > 0.0f)
    {
        return;
    }

    if (ContarVivosRacimo(minijuego) <= 1)
    {
        FinalizarRacimo(minijuego);
        return;
    }

    minijuego.racimoRepuesto = false;

    if (minijuego.frente >= TOTAL_FRUTAS_RACIMO)
    {
        ReponerRacimo(minijuego);
        minijuego.racimoRepuesto = true;
    }

    int siguiente = SiguienteVivoRacimo(minijuego, minijuego.turno);

    if (minijuego.saltarSiguiente)
    {
        minijuego.saltarSiguiente = false;
        minijuego.jugadorSaltado = siguiente;
        siguiente = SiguienteVivoRacimo(minijuego, siguiente);
    }

    minijuego.turno = siguiente;
    IniciarTurnoRacimo(minijuego);
}


static void ActualizarEfectosRacimo(MinijuegoRacimoToxico& minijuego, float deltaTime)
{
    for (int i = 0; i < MAX_VUELOS_RACIMO; i++)
    {
        VueloFrutaRacimo& vuelo = minijuego.vuelos[i];

        if (!vuelo.activo)
        {
            continue;
        }

        vuelo.progreso += deltaTime / DURACION_VUELO_RACIMO;

        if (vuelo.progreso >= 1.0f)
        {
            vuelo.activo = false;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float& sacudida = minijuego.estadosJugadores[i].sacudida;

        if (sacudida > 0.0f)
        {
            sacudida -= deltaTime;
            if (sacudida < 0.0f) sacudida = 0.0f;
        }
    }
}


//==================================================
// INTERFAZ
//==================================================


void MinijuegoRacimoToxico::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        posicionBalsaX[i] = 0.0f;
    }

    for (int i = 0; i < MAX_VUELOS_RACIMO; i++)
    {
        vuelos[i] = {};
    }

    camara.position = { 0.0f, 7.5f, 17.0f };
    camara.target = { 0.0f, 4.8f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_RACIMO_PREPARACION;
    turno = -1;
    seleccion = 1;
    ultimoJugador = -1;
    jugadorSaltado = -1;
    ultimoEvento = EVENTO_RACIMO_NINGUNO;
    saltarSiguiente = false;
    accionPrevia = false;
    racimoRepuesto = false;
    tiempoTurno = DURACION_TURNO_RACIMO;
    tiempoPausa = 0.0f;
    tiempoRestante = DURACION_PARTIDA_RACIMO;
    tiempoPreparacion = DURACION_PREPARACION_RACIMO;
    tiempoAnimacion = 0.0f;
    tiempoBot = 0.0f;

    ReponerRacimo(*this);
}


void MinijuegoRacimoToxico::Reiniciar(
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

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_RACIMO_TERMINADO;
        return;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
    int ranura = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        estadosJugadores[i].vivo = true;
        estadosJugadores[i].vidas = VIDAS_RACIMO;

        float x = ((float)ranura - ((float)cantidad - 1.0f) * 0.5f) * SEPARACION_BALSAS_RACIMO;
        posicionBalsaX[i] = x;
        ranura++;

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            Vector3{ x, 0.9f, Z_BALSAS_RACIMO }
        );
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
    }

    turno = indices[GetRandomValue(0, cantidad - 1)];
}


void MinijuegoRacimoToxico::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    (void)cantidadMaxima;

    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_RACIMO_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    ActualizarEfectosRacimo(*this, deltaTime);

    if (fase == FASE_RACIMO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            IniciarTurnoRacimo(*this);
        }

        return;
    }

    tiempoRestante -= deltaTime;

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarRacimo(*this);
        return;
    }

    if (fase == FASE_RACIMO_TURNO)
    {
        ActualizarTurnoRacimo(*this, deltaTime, participantes);
    }
    else if (fase == FASE_RACIMO_ANIMACION)
    {
        ActualizarAnimacionRacimo(*this, deltaTime);
    }
}


//==================================================
// VISUAL
//==================================================
//
// MODELO FUTURO: reemplazar por GLB el arbol podrido con su rama, la
// enredadera y los frutos (normal, baya toxica, dorado), las balsas, los
// troncos flotantes, las cabanas sobre pilotes y las luciernagas.
//==================================================


static Color ColorFrutaRacimo(int tipo)
{
    if (tipo == FRUTA_RACIMO_TOXICA) return Color{ 150, 50, 190, 255 };
    if (tipo == FRUTA_RACIMO_DORADA) return Color{ 255, 212, 50, 255 };
    return Color{ 176, 206, 74, 255 };
}


static void DibujarFrutaRacimo(Vector3 posicion, int tipo, float tiempo)
{
    DrawSphere(posicion, 0.2f, ColorFrutaRacimo(tipo));

    if (tipo == FRUTA_RACIMO_TOXICA)
    {
        float pulso = 0.3f + 0.2f * std::sin(tiempo * 6.0f + posicion.y);
        DrawSphere(posicion, 0.3f, Fade(Color{ 230, 60, 150, 255 }, pulso));
    }
    else if (tipo == FRUTA_RACIMO_DORADA)
    {
        DrawSphere(posicion, 0.34f, Fade(Color{ 255, 230, 100, 255 }, 0.32f));
    }
}


static void DibujarTroncoFlotanteRacimo(Vector3 a, Vector3 b)
{
    DrawCylinderEx(a, b, 0.32f, 0.3f, 8, Color{ 70, 56, 40, 255 });
    DrawCylinderEx(
        { a.x, a.y + 0.28f, a.z },
        { b.x, b.y + 0.28f, b.z },
        0.1f,
        0.1f,
        6,
        Color{ 60, 98, 52, 255 }
    );
}


static void DibujarCabanaRacimo(float x, float z)
{
    for (int i = 0; i < 4; i++)
    {
        float dx = (i % 2 == 0 ? -1.2f : 1.2f);
        float dz = (i < 2 ? -0.9f : 0.9f);
        DrawCylinder({ x + dx, 0.6f, z + dz }, 0.12f, 0.14f, 2.4f, 6, Color{ 54, 42, 32, 255 });
    }

    DrawCube({ x, 1.9f, z }, 3.2f, 0.18f, 2.4f, Color{ 76, 58, 42, 255 });
    DrawCube({ x, 2.8f, z }, 2.8f, 1.6f, 2.0f, Color{ 88, 70, 52, 255 });
    DrawCube({ x, 2.8f, z + 1.01f }, 0.7f, 0.7f, 0.04f, Color{ 255, 214, 120, 255 });
    DrawCylinder({ x, 3.6f, z }, 0.0f, 2.6f, 1.2f, 4, Color{ 58, 74, 44, 255 });
}


static void DibujarPantanoRacimo(float tiempo)
{
    DrawPlane({ 0.0f, -0.1f, -4.0f }, { 90.0f, 70.0f }, Color{ 28, 58, 40, 255 });
    DrawPlane({ 0.0f, -0.05f, -4.0f }, { 90.0f, 70.0f }, Fade(Color{ 70, 100, 50, 255 }, 0.35f));

    // Arbol podrido y rama de la que cuelga el racimo.
    DrawCylinder({ -4.8f, 5.5f, -1.0f }, 0.5f, 0.8f, 11.0f, 8, Color{ 58, 46, 36, 255 });
    DrawCylinderEx({ -4.6f, 10.4f, -1.0f }, { 0.0f, 11.3f, 0.0f }, 0.28f, 0.2f, 8, Color{ 62, 50, 38, 255 });
    DrawCylinderEx({ -4.8f, 8.5f, -1.0f }, { -7.5f, 10.5f, -1.5f }, 0.2f, 0.12f, 6, Color{ 62, 50, 38, 255 });

    // Lianas colgantes.
    for (int i = 0; i < 6; i++)
    {
        float x = -4.0f + 1.4f * (float)i;
        float largo = 2.0f + (float)((i * 5) % 4);
        DrawCylinderEx(
            { x * 0.8f, 11.0f, -1.2f },
            { x * 0.8f + std::sin(tiempo + (float)i) * 0.15f, 11.0f - largo, -1.2f },
            0.04f,
            0.03f,
            5,
            Color{ 44, 94, 48, 255 }
        );
    }

    DibujarTroncoFlotanteRacimo({ -9.0f, 0.1f, 0.0f }, { -6.0f, 0.1f, -1.0f });
    DibujarTroncoFlotanteRacimo({ 6.5f, 0.1f, -2.0f }, { 10.0f, 0.1f, -1.0f });
    DibujarTroncoFlotanteRacimo({ 2.0f, 0.1f, -5.0f }, { 5.0f, 0.1f, -6.0f });

    DibujarCabanaRacimo(-10.5f, -8.0f);
    DibujarCabanaRacimo(9.5f, -9.0f);

    for (int i = 0; i < 7; i++)
    {
        float x = -16.0f + 5.5f * (float)i;
        DrawCylinder({ x, 4.0f, -15.0f }, 0.5f, 0.9f, 8.0f, 7, Color{ 38, 48, 38, 255 });
        DrawSphere({ x, 8.5f, -15.0f }, 2.0f, Color{ 30, 62, 40, 255 });
    }
}


static void DibujarLuciernagasRacimo(float tiempo)
{
    for (int i = 0; i < 16; i++)
    {
        float fi = (float)i;
        Vector3 p =
        {
            -12.0f + (float)((i * 37) % 24) + std::sin(tiempo * 0.7f + fi) * 0.8f,
            0.9f + (float)((i * 7) % 40) / 5.0f + std::sin(tiempo * 1.1f + fi * 2.0f) * 0.4f,
            -7.0f + (float)((i * 13) % 12)
        };
        float brillo = 0.5f + 0.5f * std::sin(tiempo * 2.5f + fi * 1.7f);

        DrawSphere(p, 0.07f, Fade(Color{ 220, 255, 120, 255 }, 0.4f + 0.6f * brillo));
        DrawSphere(p, 0.18f, Fade(Color{ 210, 255, 110, 255 }, 0.12f * brillo));
    }
}


static void DibujarBalsaRacimo(float x, bool activa, bool viva, float sacudida, float tiempo)
{
    float hundimiento = viva ? 0.0f : -0.55f;
    float bamboleo = std::sin(tiempo * 2.0f + x) * 0.04f;
    float y = 0.08f + hundimiento + bamboleo;
    float temblor = sacudida > 0.0f ? std::sin(tiempo * 50.0f) * 0.12f : 0.0f;

    Color madera = viva ? Color{ 110, 80, 52, 255 } : Color{ 54, 44, 36, 255 };

    for (int i = 0; i < 4; i++)
    {
        DrawCube(
            { x + temblor, y, Z_BALSAS_RACIMO - 0.8f + 0.55f * (float)i },
            3.2f,
            0.24f,
            0.5f,
            i % 2 == 0 ? madera : Color{ (unsigned char)(madera.r * 0.85f), (unsigned char)(madera.g * 0.85f), (unsigned char)(madera.b * 0.85f), 255 }
        );
    }

    if (activa)
    {
        float pulso = 0.5f + 0.3f * std::sin(tiempo * 6.0f);
        DrawCylinder({ x, 0.02f, Z_BALSAS_RACIMO }, 2.3f, 2.3f, 0.06f, 20, Fade(YELLOW, pulso));
    }
}


void MinijuegoRacimoToxico::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 10, 22, 18, 255 });
    BeginMode3D(camara);

    DibujarPantanoRacimo(tiempoAnimacion);

    // Enredadera.
    DrawCylinder({ 0.0f, 6.0f, 0.0f }, 0.09f, 0.09f, 10.6f, 6, Color{ 52, 110, 56, 255 });

    for (int i = frente; i < TOTAL_FRUTAS_RACIMO; i++)
    {
        Vector3 p = PosicionFrutaRacimo(i, tiempoAnimacion);
        DrawLine3D({ 0.0f, p.y, 0.0f }, p, Color{ 52, 110, 56, 255 });
        DibujarFrutaRacimo(p, tipoFruta[i], tiempoAnimacion);

        if (i % 3 == 0)
        {
            DrawSphere({ p.x * 0.5f, p.y + 0.12f, -0.1f }, 0.14f, Color{ 38, 96, 46, 255 });
        }
    }

    if (fase == FASE_RACIMO_TURNO)
    {
        for (int k = 0; k < seleccion && frente + k < TOTAL_FRUTAS_RACIMO; k++)
        {
            DrawSphereWires(
                PosicionFrutaRacimo(frente + k, tiempoAnimacion),
                0.38f,
                8,
                8,
                YELLOW
            );
        }
    }

    // Balsas y jugadores.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorRacimoToxico& estado = estadosJugadores[i];

        DibujarBalsaRacimo(
            posicionBalsaX[i],
            fase == FASE_RACIMO_TURNO && i == turno,
            estado.vivo,
            estado.sacudida,
            tiempoAnimacion
        );

        if (!estado.vivo)
        {
            continue;
        }

        JugadorPrueba visual = jugadores[i];
        visual.posicion.x = posicionBalsaX[i] +
            (estado.sacudida > 0.0f ? std::sin(tiempoAnimacion * 50.0f) * 0.12f : 0.0f);
        visual.posicion.y = 0.2f + visual.tamano.y * 0.5f;
        visual.posicion.z = Z_BALSAS_RACIMO;

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;
        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }

        if (fase == FASE_RACIMO_TURNO && i == turno)
        {
            DrawSphere(
                { posicionBalsaX[i], 2.6f + std::sin(tiempoAnimacion * 5.0f) * 0.15f, Z_BALSAS_RACIMO },
                0.22f,
                YELLOW
            );
        }
    }

    // Frutos en vuelo hacia quien los tomo.
    for (int i = 0; i < MAX_VUELOS_RACIMO; i++)
    {
        const VueloFrutaRacimo& vuelo = vuelos[i];

        if (!vuelo.activo || vuelo.jugador < 0)
        {
            continue;
        }

        Vector3 destino = { posicionBalsaX[vuelo.jugador], 2.0f, Z_BALSAS_RACIMO };
        float t = vuelo.progreso;
        Vector3 p =
        {
            vuelo.origen.x + (destino.x - vuelo.origen.x) * t,
            vuelo.origen.y + (destino.y - vuelo.origen.y) * t + std::sin(t * PI_RACIMO) * 1.2f,
            vuelo.origen.z + (destino.z - vuelo.origen.z) * t
        };
        DibujarFrutaRacimo(p, vuelo.tipo, tiempoAnimacion);
    }

    DibujarLuciernagasRacimo(tiempoAnimacion);

    // Niebla baja sobre el agua (translucida, al final).
    DrawCube({ -6.0f, 0.5f, -3.0f }, 16.0f, 1.0f, 5.0f, Fade(Color{ 150, 190, 170, 255 }, 0.1f));
    DrawCube({ 7.0f, 0.4f, -5.0f }, 16.0f, 0.8f, 5.0f, Fade(Color{ 150, 190, 170, 255 }, 0.1f));

    EndMode3D();

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Vidas sobre cada balsa.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { posicionBalsaX[i], 0.0f, Z_BALSAS_RACIMO + 1.4f },
            camara
        );
        const char* etiqueta = TextFormat("J%d", participantes[i].numeroJugador);

        // Etiqueta y vidas se anclan sobre el borde inferior para no cortarse.
        int baseY = (int)pantalla.y;
        int maxBaseY = GetScreenHeight() - 62;
        if (baseY > maxBaseY) baseY = maxBaseY;

        DrawText(
            etiqueta,
            (int)pantalla.x - MeasureText(etiqueta, 22) / 2,
            baseY + 4,
            22,
            estadosJugadores[i].vivo ? participantes[i].color : GRAY
        );

        for (int v = 0; v < VIDAS_RACIMO; v++)
        {
            int cx = (int)pantalla.x - 14 + v * 28;
            int cy = baseY + 38;

            if (v < estadosJugadores[i].vidas)
                DrawCircle(cx, cy, 9.0f, Color{ 120, 220, 90, 255 });
            else
                DrawCircleLines(cx, cy, 9.0f, GRAY);
        }
    }

    DrawRectangle(18, 16, 700, 112, Fade(BLACK, 0.79f));
    DrawText("RACIMO TOXICO", 32, 24, 28, GOLD);

    if (turno >= 0 && fase != FASE_RACIMO_PREPARACION && fase != FASE_RACIMO_TERMINADO)
    {
        DrawText(
            TextFormat(
                "TURNO DE J%d%s - TOMA %d",
                participantes[turno].numeroJugador,
                participantes[turno].esBot ? " (BOT)" : "",
                seleccion
            ),
            32,
            58,
            20,
            participantes[turno].color
        );
    }

    DrawText("IZQ = 1 FRUTO  |  DER = 2 FRUTOS  |  ACCION (E / SHIFT DER / B) = CONFIRMAR", 32, 84, 15, RAYWHITE);
    DrawText("MORADO = BAYA TOXICA (-1 VIDA)  |  DORADO = SALTA AL SIGUIENTE", 32, 104, 15, RAYWHITE);

    if (fase == FASE_RACIMO_TURNO || fase == FASE_RACIMO_ANIMACION)
    {
        DrawText(
            TextFormat("TIEMPO %.0f", tiempoRestante),
            ancho - 190,
            24,
            23,
            tiempoRestante <= 10.0f ? RED : GOLD
        );
    }

    if (fase == FASE_RACIMO_TURNO)
    {
        DrawText(
            TextFormat("TURNO %.1f", tiempoTurno > 0.0f ? tiempoTurno : 0.0f),
            ancho - 190,
            54,
            20,
            tiempoTurno <= 2.0f ? RED : RAYWHITE
        );
    }

    const char* mensaje = nullptr;

    if (fase == FASE_RACIMO_ANIMACION && ultimoJugador >= 0)
    {
        int numero = participantes[ultimoJugador].numeroJugador;

        switch (ultimoEvento)
        {
        case EVENTO_RACIMO_TOXICA:
            mensaje = TextFormat("J%d TOMO UNA BAYA TOXICA", numero);
            break;
        case EVENTO_RACIMO_ELIMINADO:
            mensaje = TextFormat("J%d QUEDA ELIMINADO", numero);
            break;
        case EVENTO_RACIMO_DORADA:
            mensaje = TextFormat("J%d TOMO EL FRUTO DORADO: SE SALTA UN TURNO", numero);
            break;
        default:
            break;
        }
    }
    else if (fase == FASE_RACIMO_TURNO && racimoRepuesto)
    {
        mensaje = "EL RACIMO REBROTO";
    }

    if (mensaje != nullptr)
    {
        DrawText(mensaje, ancho / 2 - MeasureText(mensaje, 24) / 2, alto - 100, 24, YELLOW);
    }

    if (fase == FASE_RACIMO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            ancho / 2 - MeasureText(texto, 88) / 2,
            alto / 2 - 54,
            88,
            GOLD
        );
    }
    else if (
        fase == FASE_RACIMO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR)
        {
            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (
                    resultado.participantes[i].participo &&
                    resultado.participantes[i].posicionFinal == 1
                )
                {
                    titulo = TextFormat("GANA J%d", participantes[i].numeroJugador);
                }
            }
        }

        DrawRectangle(ancho / 2 - 290, alto / 2 - 92, 580, 184, Fade(BLACK, 0.91f));
        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, 34) / 2,
            alto / 2 - 51,
            34,
            GOLD
        );
        DrawText(
            TextoReinicioMinijuego(),
            ancho / 2 - MeasureText(TextoReinicioMinijuego(), 21) / 2,
            alto / 2 + 25,
            21,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoRacimoToxico::ObtenerResultado() const
{
    return resultado;
}
