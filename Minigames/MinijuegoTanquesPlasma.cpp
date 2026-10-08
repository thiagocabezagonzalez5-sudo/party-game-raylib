#include "Minigames/MinijuegoTanquesPlasma.h"

#include "Minigames/EfectosVisualesMinijuegos.h"

#include "Minigames/AudioMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/Input.h"

#include <cmath>


//==================================================
// TANQUES DE PLASMA - ARENA 3D "DESIERTO DE CRISTAL"
//==================================================
//
// Logica: todo ocurre en el plano XZ del suelo (Vector2.x = X, Vector2.y = Z).
// Visual: funciones DibujarEscenarioTanques y DibujarTanque3D, separadas de
// la logica para poder reemplazarlas por modelos mas adelante.
//==================================================


static const float DURACION_PREPARACION_TANQUES = 2.5f;
static const float DURACION_COMBATE_TANQUES = 45.0f;
static const float RADIO_TANQUE = 0.8f;
static const float RADIO_PROYECTIL = 0.3f;
static const float VELOCIDAD_TANQUE = 6.0f;
static const float VELOCIDAD_PROYECTIL = 16.0f;
static const float TIEMPO_RECARGA_TANQUE = 0.48f;
static const float ALTURA_DISPARO = 0.95f;

// Arena rectangular en el plano XZ.
static const float ARENA_X_MIN = -16.0f;
static const float ARENA_X_MAX = 16.0f;
static const float ARENA_Z_MIN = -9.0f;
static const float ARENA_Z_MAX = 9.0f;

static const int CANTIDAD_OBSTACULOS_TANQUES = 3;


static Vector3 AMundoTanques(Vector2 posicion, float altura)
{
    return Vector3{ posicion.x, altura, posicion.y };
}


static Camera3D ObtenerCamaraTanques()
{
    Camera3D camara{};
    camara.position = { 0.0f, 21.0f, 14.5f };
    camara.target = { 0.0f, 0.0f, 0.8f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Coberturas circulares con colision (columnas de cristal).
static void ObtenerObstaculosTanques(
    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES],
    float radios[CANTIDAD_OBSTACULOS_TANQUES]
)
{
    centros[0] = { 0.0f, 0.0f };
    centros[1] = { -7.4f, -3.2f };
    centros[2] = { 7.4f, 3.2f };

    radios[0] = 2.0f;
    radios[1] = 1.4f;
    radios[2] = 1.4f;
}


static int ContarTanquesVivos(
    const MinijuegoTanquesPlasma& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.tanques[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int PuntuacionTanque(
    const EstadoTanquePlasma& tanque
)
{
    return tanque.vidas * 100 + tanque.impactosAcertados * 25;
}


static void CrearEfectoImpacto(
    MinijuegoTanquesPlasma& minijuego,
    Vector2 posicion,
    float tamano,
    int propietario
)
{
    for (int i = 0; i < MAX_EFECTOS_TANQUES_PLASMA; i++)
    {
        if (!minijuego.efectos[i].activo)
        {
            minijuego.efectos[i].activo = true;
            minijuego.efectos[i].posicion = posicion;
            minijuego.efectos[i].tiempo = 0.0f;
            minijuego.efectos[i].tamano = tamano;
            minijuego.efectos[i].propietario = propietario;
            return;
        }
    }
}


static void FinalizarTanques(
    MinijuegoTanquesPlasma& minijuego
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

        int puntuacion = PuntuacionTanque(minijuego.tanques[i]);
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                PuntuacionTanque(minijuego.tanques[j]) > puntuacion
            )
            {
                posicion++;
            }
        }

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
    minijuego.fase = FASE_TANQUES_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void LimitarTanqueEnArena(
    EstadoTanquePlasma& tanque
)
{
    tanque.posicion.x = Clamp(
        tanque.posicion.x,
        ARENA_X_MIN + RADIO_TANQUE,
        ARENA_X_MAX - RADIO_TANQUE
    );

    tanque.posicion.y = Clamp(
        tanque.posicion.y,
        ARENA_Z_MIN + RADIO_TANQUE,
        ARENA_Z_MAX - RADIO_TANQUE
    );

    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < CANTIDAD_OBSTACULOS_TANQUES; i++)
    {
        Vector2 diferencia = Vector2Subtract(tanque.posicion, centros[i]);
        float distancia = Vector2Length(diferencia);
        float minimo = RADIO_TANQUE + radios[i];

        if (distancia < minimo)
        {
            Vector2 normal = distancia > 0.001f
                ? Vector2Scale(diferencia, 1.0f / distancia)
                : Vector2{ 1.0f, 0.0f };

            tanque.posicion = Vector2Add(
                centros[i],
                Vector2Scale(normal, minimo)
            );
        }
    }
}


static int BuscarObjetivoTanque(
    const MinijuegoTanquesPlasma& minijuego,
    int propietario
)
{
    int objetivo = -1;
    float mejorDistancia = 100000000.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == propietario ||
            !minijuego.resultado.participantes[i].participo ||
            minijuego.tanques[i].eliminado
        )
        {
            continue;
        }

        float distancia = Vector2DistanceSqr(
            minijuego.tanques[propietario].posicion,
            minijuego.tanques[i].posicion
        );

        if (distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
            objetivo = i;
        }
    }

    return objetivo;
}


// Verdadero si una columna se interpone entre origen y destino.
static bool LineaBloqueadaPorObstaculo(Vector2 origen, Vector2 destino)
{
    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    Vector2 segmento = Vector2Subtract(destino, origen);
    float largo2 = Vector2LengthSqr(segmento);

    for (int i = 0; i < CANTIDAD_OBSTACULOS_TANQUES; i++)
    {
        float t = largo2 > 0.0001f
            ? Clamp(
                Vector2DotProduct(Vector2Subtract(centros[i], origen), segmento) / largo2,
                0.0f,
                1.0f
            )
            : 0.0f;
        Vector2 cercano = Vector2Add(origen, Vector2Scale(segmento, t));

        if (Vector2Distance(cercano, centros[i]) < radios[i] + RADIO_PROYECTIL)
        {
            return true;
        }
    }

    return false;
}


static void DispararTanque(
    MinijuegoTanquesPlasma& minijuego,
    int propietario
)
{
    EstadoTanquePlasma& tanque = minijuego.tanques[propietario];

    if (tanque.recarga > 0.0f || tanque.eliminado)
    {
        return;
    }

    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        ProyectilTanquePlasma& proyectil = minijuego.proyectiles[i];

        if (proyectil.activo)
        {
            continue;
        }

        proyectil.activo = true;
        proyectil.propietario = propietario;
        proyectil.posicion = Vector2Add(
            tanque.posicion,
            Vector2Scale(tanque.direccion, RADIO_TANQUE + 0.55f)
        );
        proyectil.velocidad = Vector2Scale(
            tanque.direccion,
            VELOCIDAD_PROYECTIL
        );
        proyectil.tiempoVida = 2.8f;
        tanque.recarga = TIEMPO_RECARGA_TANQUE;

        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_DISPARO);
        return;
    }
}


static void ResolverChoquesEntreTanques(
    MinijuegoTanquesPlasma& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !minijuego.resultado.participantes[i].participo ||
            minijuego.tanques[i].eliminado
        )
        {
            continue;
        }

        for (int j = i + 1; j < MAX_PARTICIPANTES; j++)
        {
            if (
                !minijuego.resultado.participantes[j].participo ||
                minijuego.tanques[j].eliminado
            )
            {
                continue;
            }

            Vector2 diferencia = Vector2Subtract(
                minijuego.tanques[j].posicion,
                minijuego.tanques[i].posicion
            );
            float distancia = Vector2Length(diferencia);
            float minimo = RADIO_TANQUE * 2.0f;

            if (distancia < minimo)
            {
                Vector2 normal = distancia > 0.001f
                    ? Vector2Scale(diferencia, 1.0f / distancia)
                    : Vector2{ 1.0f, 0.0f };
                float correccion = (minimo - distancia) * 0.5f;

                minijuego.tanques[i].posicion = Vector2Subtract(
                    minijuego.tanques[i].posicion,
                    Vector2Scale(normal, correccion)
                );
                minijuego.tanques[j].posicion = Vector2Add(
                    minijuego.tanques[j].posicion,
                    Vector2Scale(normal, correccion)
                );

                LimitarTanqueEnArena(minijuego.tanques[i]);
                LimitarTanqueEnArena(minijuego.tanques[j]);
            }
        }
    }
}


static bool ProyectilChocaObstaculo(Vector2 posicion)
{
    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < CANTIDAD_OBSTACULOS_TANQUES; i++)
    {
        if (CheckCollisionCircles(
            posicion,
            RADIO_PROYECTIL,
            centros[i],
            radios[i]
        ))
        {
            return true;
        }
    }

    return false;
}


static void ActualizarProyectilesTanques(
    MinijuegoTanquesPlasma& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        ProyectilTanquePlasma& proyectil = minijuego.proyectiles[i];

        if (!proyectil.activo)
        {
            continue;
        }

        proyectil.posicion = Vector2Add(
            proyectil.posicion,
            Vector2Scale(proyectil.velocidad, deltaTime)
        );
        proyectil.tiempoVida -= deltaTime;

        bool fueraArena =
            proyectil.posicion.x < ARENA_X_MIN ||
            proyectil.posicion.x > ARENA_X_MAX ||
            proyectil.posicion.y < ARENA_Z_MIN ||
            proyectil.posicion.y > ARENA_Z_MAX;

        if (
            proyectil.tiempoVida <= 0.0f ||
            fueraArena ||
            ProyectilChocaObstaculo(proyectil.posicion)
        )
        {
            if (proyectil.tiempoVida > 0.0f)
            {
                CrearEfectoImpacto(
                    minijuego,
                    proyectil.posicion,
                    0.6f,
                    proyectil.propietario
                );
            }

            proyectil.activo = false;
            continue;
        }

        for (int jugador = 0; jugador < MAX_PARTICIPANTES; jugador++)
        {
            EstadoTanquePlasma& tanque = minijuego.tanques[jugador];

            if (
                jugador == proyectil.propietario ||
                !minijuego.resultado.participantes[jugador].participo ||
                tanque.eliminado ||
                tanque.invulnerabilidad > 0.0f
            )
            {
                continue;
            }

            if (!CheckCollisionCircles(
                proyectil.posicion,
                RADIO_PROYECTIL,
                tanque.posicion,
                RADIO_TANQUE
            ))
            {
                continue;
            }

            proyectil.activo = false;
            tanque.vidas--;
            tanque.invulnerabilidad = 0.68f;

            if (
                proyectil.propietario >= 0 &&
                proyectil.propietario < MAX_PARTICIPANTES
            )
            {
                minijuego.tanques[proyectil.propietario].impactosAcertados++;
            }

            if (tanque.vidas <= 0)
            {
                tanque.vidas = 0;
                tanque.eliminado = true;
                CrearEfectoImpacto(minijuego, tanque.posicion, 2.2f, jugador);
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
            }
            else
            {
                CrearEfectoImpacto(
                    minijuego,
                    proyectil.posicion,
                    1.2f,
                    proyectil.propietario
                );
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
            }

            break;
        }
    }

    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        if (!minijuego.proyectiles[i].activo) continue;

        for (int j = i + 1; j < MAX_PROYECTILES_TANQUES_PLASMA; j++)
        {
            if (
                !minijuego.proyectiles[j].activo ||
                minijuego.proyectiles[i].propietario ==
                    minijuego.proyectiles[j].propietario
            )
            {
                continue;
            }

            if (CheckCollisionCircles(
                minijuego.proyectiles[i].posicion,
                RADIO_PROYECTIL,
                minijuego.proyectiles[j].posicion,
                RADIO_PROYECTIL
            ))
            {
                CrearEfectoImpacto(
                    minijuego,
                    minijuego.proyectiles[i].posicion,
                    0.9f,
                    -1
                );
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GOLPE);
                minijuego.proyectiles[i].activo = false;
                minijuego.proyectiles[j].activo = false;
                break;
            }
        }
    }
}


// IA: persigue al rival mas cercano rodeandolo, esquiva disparos que
// vienen de frente y solo dispara con la linea despejada. La punteria
// tiene un error pequeno que oscila para que no sea perfecta.
static void ActualizarBotTanque(
    const MinijuegoTanquesPlasma& minijuego,
    int indice,
    Vector2& movimiento,
    Vector2& direccion,
    bool& disparar
)
{
    const EstadoTanquePlasma& tanque = minijuego.tanques[indice];
    int objetivo = BuscarObjetivoTanque(minijuego, indice);

    if (objetivo < 0)
    {
        return;
    }

    float tiempo = minijuego.tiempoAnimacion;
    Vector2 haciaObjetivo = Vector2Subtract(
        minijuego.tanques[objetivo].posicion,
        tanque.posicion
    );
    float distancia = Vector2Length(haciaObjetivo);
    haciaObjetivo = distancia > 0.001f
        ? Vector2Scale(haciaObjetivo, 1.0f / distancia)
        : Vector2{ 1.0f, 0.0f };

    float error = std::sin(tiempo * 2.3f + tanque.faseBot) * 0.10f;
    direccion = Vector2Rotate(haciaObjetivo, error);

    float lado = std::sin(tiempo * 1.45f + tanque.faseBot) >= 0.0f
        ? 1.0f
        : -1.0f;
    Vector2 lateral = { -haciaObjetivo.y * lado, haciaObjetivo.x * lado };

    float radial = distancia > 10.0f ? 0.6f : (distancia < 4.5f ? -0.8f : -0.1f);
    bool despejado = !LineaBloqueadaPorObstaculo(
        tanque.posicion,
        minijuego.tanques[objetivo].posicion
    );

    if (!despejado)
    {
        radial = 0.7f;
    }

    movimiento = Vector2Add(lateral, Vector2Scale(haciaObjetivo, radial));

    // Esquiva: si un proyectil ajeno se acerca y su trayectoria roza al tanque.
    if (std::sin(tiempo * 3.1f + tanque.faseBot) > -0.5f)
    {
        for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
        {
            const ProyectilTanquePlasma& proyectil = minijuego.proyectiles[i];

            if (!proyectil.activo || proyectil.propietario == indice)
            {
                continue;
            }

            Vector2 relativo = Vector2Subtract(proyectil.posicion, tanque.posicion);
            Vector2 dirProyectil = Vector2Normalize(proyectil.velocidad);

            if (
                Vector2LengthSqr(relativo) > 36.0f ||
                Vector2DotProduct(dirProyectil, relativo) >= 0.0f
            )
            {
                continue;
            }

            float cruz = dirProyectil.x * relativo.y - dirProyectil.y * relativo.x;

            if (std::fabs(cruz) < 1.6f)
            {
                Vector2 perpendicular = { -dirProyectil.y, dirProyectil.x };
                float signo = cruz > 0.0f ? -1.0f : 1.0f;
                movimiento = Vector2Scale(perpendicular, signo);
                break;
            }
        }
    }

    disparar = tanque.recarga <= 0.0f && despejado;
}


void MinijuegoTanquesPlasma::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        tanques[i] = {};
        tanques[i].faseBot = i * 1.7f;
    }

    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        proyectiles[i] = {};
    }

    for (int i = 0; i < MAX_EFECTOS_TANQUES_PLASMA; i++)
    {
        efectos[i] = {};
    }

    fase = FASE_TANQUES_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_TANQUES;
    tiempoCombate = DURACION_COMBATE_TANQUES;
    tiempoAnimacion = 0.0f;
}


void MinijuegoTanquesPlasma::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    const float margenX = 2.8f;
    const float margenZ = 2.6f;
    Vector2 spawns[MAX_PARTICIPANTES] =
    {
        { ARENA_X_MIN + margenX, ARENA_Z_MIN + margenZ },
        { ARENA_X_MAX - margenX, ARENA_Z_MAX - margenZ },
        { ARENA_X_MAX - margenX, ARENA_Z_MIN + margenZ },
        { ARENA_X_MIN + margenX, ARENA_Z_MAX - margenZ }
    };

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        tanques[i].posicion = spawns[i];

        Vector2 haciaCentro = Vector2Normalize(
            Vector2Subtract({ 0.0f, 0.0f }, spawns[i])
        );
        tanques[i].direccion = haciaCentro;
        tanques[i].anguloCasco = std::atan2(haciaCentro.y, haciaCentro.x);
    }
}


void MinijuegoTanquesPlasma::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    for (int i = 0; i < MAX_EFECTOS_TANQUES_PLASMA; i++)
    {
        if (!efectos[i].activo) continue;

        efectos[i].tiempo += deltaTime;
        if (efectos[i].tiempo >= 0.5f) efectos[i].activo = false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (tanques[i].eliminado) tanques[i].tiempoMuerto += deltaTime;
    }

    if (fase == FASE_TANQUES_TERMINADO)
    {
        return;
    }

    if (fase == FASE_TANQUES_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_TANQUES_COMBATE;
        }

        return;
    }

    float combateAntes = tiempoCombate;
    tiempoCombate -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, combateAntes, tiempoCombate);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoTanquePlasma& tanque = tanques[i];

        if (!resultado.participantes[i].participo || tanque.eliminado)
        {
            continue;
        }

        tanque.recarga -= deltaTime;
        tanque.invulnerabilidad -= deltaTime;
        if (tanque.recarga < 0.0f) tanque.recarga = 0.0f;
        if (tanque.invulnerabilidad < 0.0f) tanque.invulnerabilidad = 0.0f;

        Vector2 movimiento{};
        bool disparar = false;

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            ActualizarBotTanque(*this, i, movimiento, tanque.direccion, disparar);
        }
        else
        {
            InputMinijuegoParticipante entrada =
                LeerInputMinijuegoParticipante(participantes[i]);

            movimiento.x =
                (entrada.derecha ? 1.0f : 0.0f) -
                (entrada.izquierda ? 1.0f : 0.0f);
            movimiento.y =
                (entrada.atras ? 1.0f : 0.0f) -
                (entrada.adelante ? 1.0f : 0.0f);
            disparar = entrada.golpear;

            if (Vector2LengthSqr(movimiento) > 0.01f)
            {
                tanque.direccion = Vector2Normalize(movimiento);
            }
        }

        if (Vector2LengthSqr(movimiento) > 0.01f)
        {
            movimiento = Vector2Normalize(movimiento);
            tanque.posicion = Vector2Add(
                tanque.posicion,
                Vector2Scale(movimiento, VELOCIDAD_TANQUE * deltaTime)
            );
            LimitarTanqueEnArena(tanque);

            // El casco gira suavemente hacia donde avanza el tanque.
            float objetivoAngulo = std::atan2(movimiento.y, movimiento.x);
            float diferencia = objetivoAngulo - tanque.anguloCasco;
            while (diferencia > PI) diferencia -= 2.0f * PI;
            while (diferencia < -PI) diferencia += 2.0f * PI;

            float giro = 9.0f * deltaTime;
            if (giro > 1.0f) giro = 1.0f;
            tanque.anguloCasco += diferencia * giro;
        }

        if (disparar)
        {
            DispararTanque(*this, i);

            // Los bots no disparan apenas termina la recarga: un jugador
            // humano tarda en reapuntar. Se suma una pausa variable.
            if (
                (participantes[i].esBot || !participantes[i].conectado) &&
                tanque.recarga > 0.0f
            )
            {
                tanque.recarga += 0.55f + (float)GetRandomValue(0, 60) / 100.0f;
            }
        }
    }

    ResolverChoquesEntreTanques(*this);
    ActualizarProyectilesTanques(*this, deltaTime);

    if (ContarTanquesVivos(*this) <= 1 || tiempoCombate <= 0.0f)
    {
        if (tiempoCombate < 0.0f) tiempoCombate = 0.0f;
        FinalizarTanques(*this);
    }
}


//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB los tanques (casco, orugas, torreta,
// canon), las columnas de cristal, los muros de la base, los cristales
// del desierto, los pilares de las esquinas y los proyectiles de plasma.
//==================================================


static void DibujarEscenarioTanques(float tiempo)
{
    // Suelo exterior (arena oscura) y plataforma de la arena.
    DrawCube({ 0.0f, -0.12f, 0.0f }, 80.0f, 0.1f, 50.0f, Color{ 38, 30, 52, 255 });
    DrawCube({ 0.0f, -0.04f, 0.0f }, 33.0f, 0.1f, 19.0f, Color{ 40, 58, 70, 255 });

    // Rejilla de baldosas luminosas.
    Color rejilla = Fade(SKYBLUE, 0.18f);

    for (int x = (int)ARENA_X_MIN; x <= (int)ARENA_X_MAX; x += 2)
    {
        DrawLine3D({ (float)x, 0.02f, ARENA_Z_MIN }, { (float)x, 0.02f, ARENA_Z_MAX }, rejilla);
    }

    for (int z = (int)ARENA_Z_MIN; z <= (int)ARENA_Z_MAX; z += 2)
    {
        DrawLine3D({ ARENA_X_MIN, 0.02f, (float)z }, { ARENA_X_MAX, 0.02f, (float)z }, rejilla);
    }

    // Muros perimetrales con franja de energia.
    Color muro = Color{ 70, 78, 96, 255 };
    Color energia = Color{ 90, 210, 239, 255 };
    float largoX = ARENA_X_MAX - ARENA_X_MIN + 1.6f;
    float largoZ = ARENA_Z_MAX - ARENA_Z_MIN + 1.6f;

    DrawCube({ 0.0f, 0.6f, ARENA_Z_MIN - 0.4f }, largoX, 1.2f, 0.8f, muro);
    DrawCube({ 0.0f, 0.6f, ARENA_Z_MAX + 0.4f }, largoX, 1.2f, 0.8f, muro);
    DrawCube({ ARENA_X_MIN - 0.4f, 0.6f, 0.0f }, 0.8f, 1.2f, largoZ, muro);
    DrawCube({ ARENA_X_MAX + 0.4f, 0.6f, 0.0f }, 0.8f, 1.2f, largoZ, muro);

    DrawCube({ 0.0f, 1.22f, ARENA_Z_MIN - 0.1f }, largoX, 0.08f, 0.12f, energia);
    DrawCube({ 0.0f, 1.22f, ARENA_Z_MAX + 0.1f }, largoX, 0.08f, 0.12f, energia);
    DrawCube({ ARENA_X_MIN - 0.1f, 1.22f, 0.0f }, 0.12f, 0.08f, largoZ, energia);
    DrawCube({ ARENA_X_MAX + 0.1f, 1.22f, 0.0f }, 0.12f, 0.08f, largoZ, energia);

    // Torres de esquina con luz pulsante.
    float pulso = 0.6f + 0.4f * std::sin(tiempo * 3.0f);
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            Vector3 base = { sx * (ARENA_X_MAX + 0.9f), 0.0f, sz * (ARENA_Z_MAX + 0.9f) };
            DrawCube({ base.x, 1.5f, base.z }, 1.3f, 3.0f, 1.3f, Color{ 55, 62, 80, 255 });
            DrawCubeWires({ base.x, 1.5f, base.z }, 1.3f, 3.0f, 1.3f, Fade(energia, 0.5f));
            DrawSphereEx({ base.x, 3.3f, base.z }, 0.35f, 8, 8, Fade(energia, pulso));
        }
    }

    // Cristales del desierto repartidos alrededor de la base (posiciones fijas).
    for (int i = 0; i < 26; i++)
    {
        float angulo = i * 2.399f;
        float radioX = 22.0f + std::fmod(i * 7.3f, 9.0f);
        float radioZ = 15.0f + std::fmod(i * 4.1f, 6.0f);
        float x = std::cos(angulo) * radioX;
        float z = std::sin(angulo) * radioZ;
        float altura = 1.6f + std::fmod(i * 1.37f, 3.2f);
        Color cristal = i % 3 == 0
            ? Color{ 150, 110, 230, 255 }
            : (i % 3 == 1 ? Color{ 100, 190, 230, 255 } : Color{ 220, 130, 200, 255 });

        DrawCylinder({ x, 0.0f, z }, 0.0f, 0.7f + altura * 0.12f, altura, 5, Fade(cristal, 0.9f));
        DrawCylinder({ x + 0.7f, 0.0f, z + 0.4f }, 0.0f, 0.4f, altura * 0.6f, 5, Fade(cristal, 0.75f));
    }

    // Columnas de cristal: son las coberturas con colision.
    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < CANTIDAD_OBSTACULOS_TANQUES; i++)
    {
        Vector3 c = { centros[i].x, 0.0f, centros[i].y };
        DrawCylinder(c, radios[i], radios[i], 1.3f, 16, Color{ 57, 65, 75, 255 });
        DrawCylinder({ c.x, 1.3f, c.z }, radios[i] * 0.78f, radios[i], 0.25f, 16, Color{ 80, 92, 108, 255 });
        DrawCylinder({ c.x, 1.55f, c.z }, 0.0f, radios[i] * 0.65f, radios[i] * 1.9f, 6, Fade(Color{ 120, 200, 240, 255 }, 0.85f));
        DrawCircle3D({ c.x, 0.05f, c.z }, radios[i] + 0.1f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(energia, 0.7f));
    }
}


static void DibujarTanque3D(
    const EstadoTanquePlasma& tanque,
    const Participante& participante,
    float tiempoAnimacion
)
{
    Vector3 base = AMundoTanques(tanque.posicion, 0.0f);

    if (tanque.eliminado)
    {
        // Carcasa humeante.
        DrawCube({ base.x, 0.25f, base.z }, 1.7f, 0.5f, 1.2f, Color{ 28, 28, 32, 255 });
        DrawCubeWires({ base.x, 0.25f, base.z }, 1.7f, 0.5f, 1.2f, Fade(RED, 0.7f));
        float humo = std::fmod(tanque.tiempoMuerto, 1.6f) / 1.6f;
        DrawSphereEx(
            { base.x, 0.6f + humo * 1.6f, base.z },
            0.3f + humo * 0.4f,
            6,
            6,
            Fade(DARKGRAY, 0.55f * (1.0f - humo))
        );
        return;
    }

    Color color = participante.color;

    if (
        tanque.invulnerabilidad > 0.0f &&
        ((int)(tiempoAnimacion * 18.0f) % 2 == 0)
    )
    {
        color = WHITE;
    }

    Color claro = ColorLerp(color, RAYWHITE, 0.35f);
    float anguloTorreta = std::atan2(tanque.direccion.y, tanque.direccion.x);

    // Sombra circular y aro de jugador sobre el suelo.
    DrawCylinder({ base.x, 0.02f, base.z }, 1.05f, 1.05f, 0.02f, 16, Fade(BLACK, 0.4f));
    DrawCircle3D({ base.x, 0.05f, base.z }, 1.15f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.85f));

    // Casco y orugas (el eje local +X es el frente).
    rlPushMatrix();
    rlTranslatef(base.x, 0.0f, base.z);
    rlRotatef(-tanque.anguloCasco * RAD2DEG, 0.0f, 1.0f, 0.0f);
    DrawCube({ 0.0f, 0.3f, 0.64f }, 1.8f, 0.5f, 0.34f, Color{ 40, 42, 48, 255 });
    DrawCube({ 0.0f, 0.3f, -0.64f }, 1.8f, 0.5f, 0.34f, Color{ 40, 42, 48, 255 });
    DrawCube({ 0.0f, 0.5f, 0.0f }, 1.55f, 0.42f, 1.0f, color);
    DrawCubeWires({ 0.0f, 0.5f, 0.0f }, 1.55f, 0.42f, 1.0f, BLACK);
    DrawCube({ 0.85f, 0.46f, 0.0f }, 0.3f, 0.26f, 0.85f, claro);
    rlPopMatrix();

    // Torreta y canon apuntan hacia donde mira el tanque.
    rlPushMatrix();
    rlTranslatef(base.x, 0.0f, base.z);
    rlRotatef(-anguloTorreta * RAD2DEG, 0.0f, 1.0f, 0.0f);
    DrawCylinder({ 0.0f, 0.7f, 0.0f }, 0.5f, 0.58f, 0.36f, 12, claro);
    DrawCube({ 0.9f, 0.88f, 0.0f }, 1.3f, 0.17f, 0.17f, RAYWHITE);

    if (tanque.recarga > TIEMPO_RECARGA_TANQUE - 0.12f)
    {
        DrawSphereEx({ 1.6f, 0.88f, 0.0f }, 0.22f, 6, 6, Fade(color, 0.85f));
    }

    rlPopMatrix();
}


static void DibujarProyectil3D(
    const ProyectilTanquePlasma& proyectil,
    Color color
)
{
    Vector2 direccion = Vector2Normalize(proyectil.velocidad);

    // Estela: esferas cada vez mas pequenas y transparentes.
    for (int k = 4; k >= 1; k--)
    {
        Vector2 punto = Vector2Subtract(proyectil.posicion, Vector2Scale(direccion, k * 0.42f));
        DrawSphereEx(
            AMundoTanques(punto, ALTURA_DISPARO),
            RADIO_PROYECTIL * (1.0f - k * 0.17f),
            6,
            6,
            Fade(color, 0.5f - k * 0.09f)
        );
    }

    Vector3 centro = AMundoTanques(proyectil.posicion, ALTURA_DISPARO);
    DrawSphereEx(centro, RADIO_PROYECTIL * 1.7f, 8, 8, Fade(color, 0.22f));
    DrawSphereEx(centro, RADIO_PROYECTIL, 8, 8, color);
    DrawSphereEx(centro, RADIO_PROYECTIL * 0.45f, 6, 6, WHITE);
    DrawCircle3D({ centro.x, 0.04f, centro.z }, 0.35f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.5f));
}


static void DibujarEfectoImpacto3D(
    const EfectoImpactoTanque& efecto,
    const Participante participantes[]
)
{
    float progreso = efecto.tiempo / 0.5f;
    Color color = efecto.propietario >= 0 && efecto.propietario < MAX_PARTICIPANTES
        ? participantes[efecto.propietario].color
        : WHITE;
    Vector3 centro = AMundoTanques(efecto.posicion, ALTURA_DISPARO * 0.7f);

    DrawSphereEx(
        centro,
        efecto.tamano * (0.25f + progreso * 0.9f),
        8,
        8,
        Fade(color, 0.6f * (1.0f - progreso))
    );
    DrawCircle3D(
        { centro.x, 0.06f, centro.z },
        efecto.tamano * (0.4f + progreso * 1.6f),
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Fade(WHITE, 1.0f - progreso)
    );
}


void MinijuegoTanquesPlasma::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 16, 12, 30, 255 });

    Camera3D camara = ObtenerCamaraTanques();
    BeginMode3D(camara);

    DibujarEscenarioTanques(tiempoAnimacion);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarTanque3D(tanques[i], participantes[i], tiempoAnimacion);
        }
    }

    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        const ProyectilTanquePlasma& proyectil = proyectiles[i];
        if (!proyectil.activo) continue;

        Color color = proyectil.propietario >= 0
            ? participantes[proyectil.propietario].color
            : WHITE;
        DibujarProyectil3D(proyectil, color);
    }

    for (int i = 0; i < MAX_EFECTOS_TANQUES_PLASMA; i++)
    {
        if (efectos[i].activo)
        {
            DibujarEfectoImpacto3D(efectos[i], participantes);
        }
    }

    EndMode3D();

    // Etiquetas y vidas flotando sobre cada tanque.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || tanques[i].eliminado)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            AMundoTanques(tanques[i].posicion, 1.9f),
            camara
        );

        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            (int)pantalla.x - 12,
            (int)pantalla.y - 20,
            18,
            RAYWHITE
        );

        for (int vida = 0; vida < tanques[i].vidas; vida++)
        {
            DrawRectangle((int)pantalla.x - 13 + vida * 15, (int)pantalla.y + 1, 11, 6, LIME);
        }
    }

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangle(18, 15, 598, 96, Fade(BLACK, 0.82f));
    DrawText("TANQUES DE PLASMA", 32, 27, 30, SKYBLUE);

    if (fase == FASE_TANQUES_PREPARACION)
    {
        DrawText(TextFormat("PREPARATE  %.1f", tiempoPreparacion), 32, 70, 21, RAYWHITE);

        const char* numero = TextFormat("%d", (int)std::ceil(tiempoPreparacion));
        DrawText(numero, ancho / 2 - MeasureText(numero, 120) / 2, alto / 2 - 60, 120, Fade(GOLD, 0.9f));

        const char* controles = "MOVER = DIRECCION   |   GOLPEAR = DISPARAR";
        DrawText(controles, ancho / 2 - MeasureText(controles, 22) / 2, alto - 60, 22, RAYWHITE);
    }
    else if (fase == FASE_TANQUES_COMBATE)
    {
        DrawText(
            TextFormat("TIEMPO %.1f  |  MOVER/APUNTAR + GOLPEAR PARA DISPARAR", tiempoCombate),
            32,
            72,
            18,
            tiempoCombate <= 5.0f ? ORANGE : RAYWHITE
        );
    }
    else
    {
        DrawText(TextFormat("BATALLA TERMINADA  %s", TextoReinicioMinijuego()), 32, 72, 19, GOLD);

        int ganador = -1;
        int cantidadPrimeros = 0;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                resultado.participantes[i].participo &&
                resultado.participantes[i].posicionFinal == 1
            )
            {
                ganador = i;
                cantidadPrimeros++;
            }
        }

        const char* titulo = cantidadPrimeros == 1
            ? TextFormat("GANADOR: J%d", participantes[ganador].numeroJugador)
            : "EMPATE";
        Color colorTitulo = cantidadPrimeros == 1 ? participantes[ganador].color : GOLD;

        DrawRectangle(ancho / 2 - 240, alto / 2 - 50, 480, 90, Fade(BLACK, 0.75f));
        DrawText(titulo, ancho / 2 - MeasureText(titulo, 44) / 2, alto / 2 - 30, 44, colorTitulo);
    }

    int panelX = ancho - 225;
    DrawRectangle(panelX - 12, 15, 219, 96, Fade(BLACK, 0.82f));

    int fila = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        int x = panelX + (fila % 2) * 100;
        int y = 28 + (fila / 2) * 36;
        Color color = tanques[i].eliminado ? DARKGRAY : participantes[i].color;

        DrawText(TextFormat("J%d", participantes[i].numeroJugador), x, y, 18, color);
        DrawText(TextFormat("%dHP", tanques[i].vidas), x + 34, y, 16, color);
        fila++;
    }
}


const ResultadoMinijuego&
MinijuegoTanquesPlasma::ObtenerResultado() const
{
    return resultado;
}
