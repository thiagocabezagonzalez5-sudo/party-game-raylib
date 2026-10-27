#include "Minigames/MinijuegoTanquesPlasma.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// TANQUES DE PLASMA - CANON DE CRISTAL ALIENIGENA
//==================================================
//
// Logica: todo ocurre en el plano XZ del suelo (Vector2.x = X, Vector2.y = Z).
// Visual: al final del archivo, separado de la logica (estado propio con
// semilla visual; nunca consume el GetRandomValue de los bots).
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


// Efectos solo visuales / de audio (definidos con el dibujo, al final).
static void ReiniciarVisualTanques(MinijuegoTanquesPlasma& minijuego);

static void ActualizarVisualTanques(
    MinijuegoTanquesPlasma& minijuego,
    const Participante participantes[],
    float deltaTime
);


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

    ReiniciarVisualTanques(*this);
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
    ActualizarVisualTanques(*this, participantes, deltaTime);

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
// VISUAL: ESTADO, PARTICULAS Y FEEDBACK
//==================================================
//
// MODELO FUTURO: tanques (casco, orugas, torreta y canon de plasma),
// racimos de cristal de cobertura, nave estrellada, capsula de escape,
// baliza alienigena, contenedores, hongos luminosos y rocas flotantes.
//==================================================

static const float DURACION_ANUNCIO_TANQUES = 1.3f;
static const float TIEMPO_EYECCION_TANQUES = 0.9f;

// Nave estrellada en el borde izquierdo del precipicio: centro, giro
// (grados en Y, la proa mira al abismo) y punto del motor que humea.
static const Vector3 NAVE_CENTRO_TANQUES = { -21.6f, 0.35f, -10.6f };
static const float NAVE_GIRO_TANQUES = 53.0f;
static const Vector3 NAVE_HUMO_TANQUES = { -24.1f, 1.9f, -7.3f };


static float Limitar01Tanques(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float SuaveTanques(float u)
{
    u = Limitar01Tanques(u);
    return u * u * (3.0f - 2.0f * u);
}


// Hash determinista para la decoracion (misma escena en cada partida).
static float Hash01Tanques(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarTanques(MinijuegoTanquesPlasma& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasTanques(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static bool EsHumanoTanques(const Participante& participante)
{
    return !(participante.esBot || !participante.conectado);
}


// Donde queda parado el piloto despues de saltar del tanque destruido.
static Vector2 DescansoPilotoTanques(const EstadoTanquePlasma& tanque)
{
    float lado = tanque.posicion.x > ARENA_X_MAX - 3.0f ? -1.0f : 1.0f;
    float fondo = tanque.posicion.y > ARENA_Z_MAX - 2.5f ? -1.0f : 1.0f;
    return { tanque.posicion.x + 1.35f * lado, tanque.posicion.y + 0.75f * fondo };
}


static void EmitirParticulaTanques(
    MinijuegoTanquesPlasma& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color,
    bool brillo,
    bool humo = false
)
{
    for (int i = 0; i < MAX_PARTICULAS_TANQUES_PLASMA; i++)
    {
        ParticulaTanquesPlasma& p = minijuego.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.humo = humo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.color = color;
        return;
    }
}


// Chispas que saltan en abanico desde un punto.
static void EmitirChispasTanques(
    MinijuegoTanquesPlasma& minijuego,
    Vector3 centro,
    Color color,
    int base,
    float rapidez,
    bool brillo
)
{
    int cantidad = CantidadParticulasTanques(base);

    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AzarTanques(minijuego, 0.0f, 6.2832f);
        float r = AzarTanques(minijuego, 0.35f, 1.0f) * rapidez;

        EmitirParticulaTanques(
            minijuego,
            centro,
            { std::cos(angulo) * r, AzarTanques(minijuego, 0.6f, 1.6f) * rapidez * 0.6f, std::sin(angulo) * r },
            AzarTanques(minijuego, 0.25f, 0.55f),
            AzarTanques(minijuego, 0.05f, 0.11f),
            9.0f,
            color,
            brillo
        );
    }
}


static void EmitirHumoTanques(MinijuegoTanquesPlasma& minijuego, Vector3 centro, int base, float tamano)
{
    int cantidad = CantidadParticulasTanques(base);

    for (int k = 0; k < cantidad; k++)
    {
        EmitirParticulaTanques(
            minijuego,
            {
                centro.x + AzarTanques(minijuego, -0.25f, 0.25f),
                centro.y + AzarTanques(minijuego, 0.0f, 0.2f),
                centro.z + AzarTanques(minijuego, -0.25f, 0.25f)
            },
            { AzarTanques(minijuego, -0.25f, 0.35f), AzarTanques(minijuego, 0.7f, 1.3f), AzarTanques(minijuego, -0.2f, 0.2f) },
            AzarTanques(minijuego, 1.2f, 2.0f),
            tamano * AzarTanques(minijuego, 0.8f, 1.2f),
            0.0f,
            Color{ 74, 62, 88, 255 },
            false,
            true
        );
    }
}


// Disparo: fogonazo en la boca del canon y un poco de humo.
static void EfectoDisparoTanques(MinijuegoTanquesPlasma& m, const Participante participantes[], int i)
{
    const EstadoTanquePlasma& t = m.tanques[i];
    Vector3 boca = { t.posicion.x + t.direccion.x * 1.45f, ALTURA_DISPARO, t.posicion.y + t.direccion.y * 1.45f };
    Color color = participantes[i].color;
    int cantidad = CantidadParticulasTanques(7);

    for (int k = 0; k < cantidad; k++)
    {
        float abre = AzarTanques(m, -0.7f, 0.7f);
        float r = AzarTanques(m, 2.0f, 5.0f);
        Vector2 d = Vector2Rotate(t.direccion, abre);

        EmitirParticulaTanques(
            m,
            boca,
            { d.x * r, AzarTanques(m, 0.3f, 1.6f), d.y * r },
            AzarTanques(m, 0.15f, 0.3f),
            AzarTanques(m, 0.06f, 0.1f),
            3.0f,
            k % 3 == 0 ? Color{ 255, 255, 255, 255 } : color,
            true
        );
    }

    EmitirHumoTanques(m, boca, 1, 0.22f);
}


// Explosion del tanque eliminado: bola de fuego, chatarra y humo.
static void EfectoEliminacionTanques(MinijuegoTanquesPlasma& m, const Participante participantes[], int i)
{
    const EstadoTanquePlasma& t = m.tanques[i];
    Vector3 c = { t.posicion.x, 0.7f, t.posicion.y };
    int fuego = CantidadParticulasTanques(26);

    for (int k = 0; k < fuego; k++)
    {
        float angulo = AzarTanques(m, 0.0f, 6.2832f);
        float r = AzarTanques(m, 0.5f, 3.2f);
        Color color = k % 3 == 0 ? Color{ 255, 236, 160, 255 } : (k % 3 == 1 ? Color{ 255, 140, 56, 255 } : participantes[i].color);

        EmitirParticulaTanques(
            m,
            c,
            { std::cos(angulo) * r, AzarTanques(m, 1.5f, 5.0f), std::sin(angulo) * r },
            AzarTanques(m, 0.35f, 0.75f),
            AzarTanques(m, 0.12f, 0.24f),
            2.5f,
            color,
            true
        );
    }

    int chatarra = CantidadParticulasTanques(14);
    Color oscuro = Color{ 52, 48, 60, 255 };

    for (int k = 0; k < chatarra; k++)
    {
        float angulo = AzarTanques(m, 0.0f, 6.2832f);
        float r = AzarTanques(m, 1.5f, 4.5f);

        EmitirParticulaTanques(
            m,
            c,
            { std::cos(angulo) * r, AzarTanques(m, 3.0f, 6.5f), std::sin(angulo) * r },
            AzarTanques(m, 1.0f, 1.6f),
            AzarTanques(m, 0.1f, 0.2f),
            12.0f,
            k % 2 == 0 ? oscuro : participantes[i].color,
            false
        );
    }

    EmitirHumoTanques(m, c, 8, 0.45f);
}


static void EmitirConfetiTanques(
    MinijuegoTanquesPlasma& minijuego,
    const Participante participantes[],
    int indice,
    int base
)
{
    const EstadoTanquePlasma& t = minijuego.tanques[indice];
    Vector2 p = t.eliminado ? DescansoPilotoTanques(t) : t.posicion;
    int cantidad = CantidadParticulasTanques(base);
    const Color colores[3] =
    {
        participantes[indice].color,
        Color{ 255, 214, 110, 255 },
        Color{ 236, 240, 255, 255 }
    };

    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AzarTanques(minijuego, 0.0f, 6.2832f);
        float rapidez = AzarTanques(minijuego, 0.6f, 2.2f);

        EmitirParticulaTanques(
            minijuego,
            { p.x, 2.6f, p.y },
            { std::cos(angulo) * rapidez, AzarTanques(minijuego, 2.5f, 4.8f), std::sin(angulo) * rapidez },
            AzarTanques(minijuego, 1.4f, 2.2f),
            AzarTanques(minijuego, 0.08f, 0.14f),
            4.0f,
            colores[k % 3],
            false
        );
    }
}


static void ReiniciarVisualTanques(MinijuegoTanquesPlasma& minijuego)
{
    minijuego.semillaVisual = 2246822519u;
    minijuego.tiempoAnuncio = -1.0f;
    minijuego.combateVisto = false;
    minijuego.finVisto = false;

    for (int i = 0; i < 3; i++)
    {
        minijuego.destelloCristal[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS_TANQUES_PLASMA; i++)
    {
        minijuego.particulas[i] = {};
    }
}


static void ActualizarVisualTanques(
    MinijuegoTanquesPlasma& minijuego,
    const Participante participantes[],
    float deltaTime
)
{
    MinijuegoTanquesPlasma& m = minijuego;
    float dt = deltaTime;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;

    // Arranque del combate: cartel «¡FUEGO!».
    if (m.fase == FASE_TANQUES_COMBATE && !m.combateVisto)
    {
        m.combateVisto = true;
        m.tiempoAnuncio = 0.0f;
    }

    if (m.tiempoAnuncio >= 0.0f)
    {
        m.tiempoAnuncio += dt;

        if (m.tiempoAnuncio > DURACION_ANUNCIO_TANQUES)
        {
            m.tiempoAnuncio = -1.0f;
        }
    }

    for (int k = 0; k < 3; k++)
    {
        m.destelloCristal[k] = m.destelloCristal[k] > dt ? m.destelloCristal[k] - dt : 0.0f;
    }

    // Impactos nuevos (los crea la logica con tiempo 0 el cuadro anterior).
    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < MAX_EFECTOS_TANQUES_PLASMA; i++)
    {
        const EfectoImpactoTanque& e = m.efectos[i];

        if (!e.activo || e.tiempo != 0.0f)
        {
            continue;
        }

        bool dueno = e.propietario >= 0 && e.propietario < MAX_PARTICIPANTES;
        Color color = dueno ? participantes[e.propietario].color : Color{ 236, 240, 255, 255 };
        Vector3 c = { e.posicion.x, ALTURA_DISPARO, e.posicion.y };

        if (e.tamano > 2.0f)
        {
            continue;   // eliminacion: la explosion la arma el tanque
        }

        if (e.tamano > 1.0f)
        {
            // Plasma contra un tanque: punto para quien disparo.
            EmitirChispasTanques(m, c, color, 12, 4.5f, true);
            EmitirChispasTanques(m, c, Color{ 255, 255, 255, 255 }, 5, 3.5f, true);

            if (dueno && EsHumanoTanques(participantes[e.propietario]))
            {
                ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
            }

            continue;
        }

        EmitirChispasTanques(m, c, color, 7, 3.2f, true);

        // Si pego en un racimo, salta polvo de cristal y el racimo destella.
        for (int k = 0; k < CANTIDAD_OBSTACULOS_TANQUES; k++)
        {
            if (Vector2Distance(e.posicion, centros[k]) < radios[k] + RADIO_PROYECTIL + 0.3f)
            {
                m.destelloCristal[k] = 0.35f;
                EmitirChispasTanques(m, c, Color{ 150, 244, 250, 255 }, 6, 2.6f, false);
            }
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoTanquePlasma& t = m.tanques[i];

        if (!t.posicionIniciada)
        {
            t.posicionIniciada = true;
            t.posicionVista = t.posicion;
            t.vidasVistas = t.vidas;
            t.recargaVista = t.recarga;
        }

        // Orugas: avanzan lo que se movio el tanque.
        float recorrido = Vector2Distance(t.posicion, t.posicionVista);
        if (recorrido > 3.0f) recorrido = 0.0f;
        t.posicionVista = t.posicion;

        float giroAntes = t.giroOrugas;
        t.giroOrugas += recorrido;
        bool avanza = recorrido > 0.0005f && !t.eliminado;
        t.marcha += ((avanza ? 1.0f : 0.0f) - t.marcha) * Limitar01Tanques(dt * 8.0f);

        // Polvo violeta detras de las orugas.
        if (decoracion && avanza && std::floor(t.giroOrugas / 0.55f) != std::floor(giroAntes / 0.55f))
        {
            float a = t.anguloCasco;
            Vector2 atras = { -std::cos(a), -std::sin(a) };
            Vector2 lado = { -std::sin(a), std::cos(a) };

            for (int s = -1; s <= 1; s += 2)
            {
                EmitirParticulaTanques(
                    m,
                    { t.posicion.x + atras.x * 0.85f + lado.x * 0.55f * (float)s, 0.12f, t.posicion.y + atras.y * 0.85f + lado.y * 0.55f * (float)s },
                    { atras.x * 0.6f + AzarTanques(m, -0.3f, 0.3f), AzarTanques(m, 0.4f, 0.9f), atras.y * 0.6f + AzarTanques(m, -0.3f, 0.3f) },
                    AzarTanques(m, 0.5f, 0.8f),
                    AzarTanques(m, 0.08f, 0.14f),
                    0.0f,
                    Color{ 128, 104, 152, 255 },
                    false,
                    true
                );
            }
        }

        t.retroceso = t.retroceso > dt * 5.0f ? t.retroceso - dt * 5.0f : 0.0f;
        t.fogonazo = t.fogonazo > dt * 8.0f ? t.fogonazo - dt * 8.0f : 0.0f;
        t.sacudida = t.sacudida > dt ? t.sacudida - dt : 0.0f;
        t.destelloGolpe = t.destelloGolpe > dt * 3.0f ? t.destelloGolpe - dt * 3.0f : 0.0f;

        // Disparo: la recarga salto hacia arriba.
        if (!t.eliminado && t.recarga > t.recargaVista + 0.05f)
        {
            t.retroceso = 1.0f;
            t.fogonazo = 1.0f;
            EfectoDisparoTanques(m, participantes, i);
        }

        t.recargaVista = t.recarga;

        // Golpe recibido: sacudida y destello blanco.
        if (t.vidas < t.vidasVistas)
        {
            t.vidasVistas = t.vidas;
            t.sacudida = 0.35f;
            t.destelloGolpe = 1.0f;

            if (!t.eliminado && EsHumanoTanques(participantes[i]))
            {
                ActivarTemblorCamaraGeneral(0.09f, 0.18f);
            }
        }

        if (t.eliminado && !t.eliminacionVista)
        {
            t.eliminacionVista = true;
            EfectoEliminacionTanques(m, participantes, i);
            ActivarTemblorCamaraGeneral(0.2f, 0.36f);
            ReproducirSonidoMinijuego(m.audio, SONIDO_EXPLOSION);
        }

        // La carcasa sigue humeando un rato.
        if (t.eliminado && t.tiempoMuerto < 7.0f && std::fmod(t.tiempoMuerto, 0.22f) < dt)
        {
            EmitirHumoTanques(m, { t.posicion.x, 0.8f, t.posicion.y }, 1, 0.3f + 0.2f * (1.0f - t.tiempoMuerto / 7.0f));
        }
    }

    // Humo de la nave estrellada.
    if (decoracion && std::fmod(m.tiempoAnimacion, 0.3f) < dt)
    {
        EmitirHumoTanques(m, NAVE_HUMO_TANQUES, 1, 0.55f);
    }

    // Fin: confeti sobre el o los ganadores, en tandas.
    if (m.fase == FASE_TANQUES_TERMINADO)
    {
        bool primeraVez = !m.finVisto;
        m.finVisto = true;

        if (primeraVez || std::fmod(m.tiempoAnimacion, 0.5f) < dt)
        {
            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (
                    m.resultado.participantes[i].participo &&
                    m.resultado.participantes[i].posicionFinal == 1
                )
                {
                    EmitirConfetiTanques(m, participantes, i, primeraVez ? 36 : 8);
                }
            }
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_TANQUES_PLASMA; i++)
    {
        ParticulaTanquesPlasma& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= dt;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float freno = 1.0f - Limitar01Tanques(dt * (p.humo ? 0.8f : 1.4f));
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.velocidad.y -= p.gravedad * dt;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        // Lo solido rebota apenas en la roca.
        if (!p.brillo && !p.humo && p.posicion.y < 0.04f && p.velocidad.y < 0.0f)
        {
            p.posicion.y = 0.04f;
            p.velocidad.y *= -0.25f;
        }
    }
}


//==================================================
// PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Canon de cristal en un planeta ajeno, al anochecer: roca violeta,
// bruma lila en el abismo y la luz fria de un planeta anillado que entra
// desde la izquierda. Los cristales de cobertura llevan el acento cian
// (lo que bloquea el plasma); la decoracion usa rosa y lila.

static const Color COLOR_NIEBLA_TANQUES = { 54, 32, 84, 255 };      // bruma del canon
static const Color COLOR_BRUMA_TANQUES = { 150, 92, 158, 255 };     // lejanias
static const Color COLOR_ROCA_TANQUES = { 108, 88, 134, 255 };      // losas de la arena
static const Color COLOR_ROCA_OSCURA_TANQUES = { 76, 58, 102, 255 };
static const Color COLOR_ROCA_CLARA_TANQUES = { 152, 128, 176, 255 };
static const Color COLOR_JUNTA_TANQUES = { 50, 36, 72, 255 };
static const Color COLOR_CORNISA_TANQUES = { 94, 72, 122, 255 };
static const Color COLOR_ESTRATO_TANQUES = { 132, 98, 150, 255 };
static const Color COLOR_ESTRATO_OSCURO_TANQUES = { 88, 62, 112, 255 };
static const Color COLOR_CRISTAL_TANQUES = { 104, 238, 246, 255 }; // acento: cobertura
static const Color COLOR_CRISTAL_ROSA_TANQUES = { 236, 120, 214, 255 };
static const Color COLOR_CRISTAL_LILA_TANQUES = { 172, 136, 252, 255 };
static const Color COLOR_CASCO_TANQUES = { 188, 192, 208, 255 };    // nave y capsula
static const Color COLOR_METAL_TANQUES = { 92, 96, 118, 255 };
static const Color COLOR_OXIDO_TANQUES = { 168, 104, 70, 255 };
static const Color COLOR_FUEGO_TANQUES = { 255, 150, 62, 255 };
static const Color COLOR_HONGO_TANQUES = { 255, 176, 128, 255 };    // bulbos luminosos
static const Color COLOR_ENERGIA_TANQUES = { 206, 176, 255, 255 };  // franja del borde

// Camara definitiva: alta y de frente, la arena entera a la vista y, por
// encima del borde del precipicio, el otro lado del canon y el cielo.
static const Vector3 OJO_TANQUES = { 0.0f, 22.0f, 19.0f };
static const Vector3 MIRA_TANQUES = { 0.0f, 0.0f, -1.0f };
static const float FOV_TANQUES = 48.0f;

static const float BORDE_TANQUES = 0.9f;        // grosor del bordillo
static const float LABIO_Z_TANQUES = -13.2f;    // borde del precipicio
static const float PARED_Z_TANQUES = -34.0f;    // pared del otro lado


static unsigned char CanalTanques(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoTanques(Color color, float factor)
{
    return Color{
        CanalTanques((float)color.r * factor),
        CanalTanques((float)color.g * factor),
        CanalTanques((float)color.b * factor),
        color.a
    };
}


static Color MezclaTanques(Color a, Color b, float t)
{
    t = Limitar01Tanques(t);

    return Color{
        CanalTanques((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalTanques((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalTanques((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalTanques((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color ConAlfaTanques(Color color, float alfa)
{
    color.a = CanalTanques(alfa);
    return color;
}


// Caras de arriba con la luz fria del planeta; las laterales, sombra lila.
static Color SombraTanques(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalTanques((float)color.r * factor * 0.97f),
            CanalTanques((float)color.g * factor * 1.02f),
            CanalTanques((float)color.b * factor * 1.06f),
            color.a
        };
    }

    return Color{
        CanalTanques((float)color.r * factor * 0.96f),
        CanalTanques((float)color.g * factor * 0.90f),
        CanalTanques((float)color.b * factor * 1.05f),
        color.a
    };
}


static float DistanciaOjoTanques(Vector3 p)
{
    float dx = p.x - OJO_TANQUES.x;
    float dy = p.y - OJO_TANQUES.y;
    float dz = p.z - OJO_TANQUES.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}


// Lo lejano del canon se hunde en la bruma violeta.
static Color TinteTanques(Color color, Vector3 p)
{
    float n = (DistanciaOjoTanques(p) - 34.0f) / 40.0f;
    Color niebla = COLOR_NIEBLA_TANQUES;
    niebla.a = color.a;
    return MezclaTanques(color, niebla, n < 0.0f ? 0.0f : (n > 0.55f ? 0.55f : n));
}


// El otro lado del canon y las mesetas se aclaran hacia el horizonte.
static Color TinteLejosTanques(Color color, Vector3 p)
{
    float n = (DistanciaOjoTanques(p) - 50.0f) / 120.0f;
    Color bruma = COLOR_BRUMA_TANQUES;
    bruma.a = color.a;
    return MezclaTanques(color, bruma, n < 0.0f ? 0.0f : (n > 0.78f ? 0.78f : n));
}


static void VerticeTanques(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadTanques(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTanques(a, color);
    VerticeTanques(b, color);
    VerticeTanques(c, color);
    VerticeTanques(d, color);
}


static void QuadNieblaTanques(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTanques(a, TinteTanques(color, a));
    VerticeTanques(b, TinteTanques(color, b));
    VerticeTanques(c, TinteTanques(color, c));
    VerticeTanques(d, TinteTanques(color, d));
}


static void QuadLejosTanques(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTanques(a, TinteLejosTanques(color, a));
    VerticeTanques(b, TinteLejosTanques(color, b));
    VerticeTanques(c, TinteLejosTanques(color, c));
    VerticeTanques(d, TinteLejosTanques(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.54). Respeta la matriz actual.
static void CuboTanques(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadTanques({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraTanques(color, 1.0f, true));
    QuadTanques({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraTanques(color, 0.78f, false));
    QuadTanques({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraTanques(color, 0.54f, false));
    QuadTanques({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraTanques(color, 0.62f, false));
    QuadTanques({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraTanques(color, 0.70f, false));
    QuadTanques({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, SombraTanques(color, 0.45f, false));
    rlEnd();
}


static void CuboNieblaTanques(Vector3 c, Vector3 t, Color color)
{
    CuboTanques(c, t, TinteTanques(color, c));
}


// Luz falsa de las caras laterales segun su orientacion (angulo en XZ:
// 0 = +X, 90 = +Z hacia la camara). El planeta alumbra desde la izquierda.
static float LuzLateralTanques(float angulo)
{
    return 0.68f + 0.12f * std::sin(angulo) - 0.11f * std::cos(angulo);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (r1).
static void PrismaTanques(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    float giro = 0.0f
)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = SombraTanques(color, 1.0f, true);
    Color tapaAbajo = SombraTanques(color, 0.45f, false);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        Color cara = SombraTanques(color, LuzLateralTanques(a0 + paso * 0.5f), false);

        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadTanques(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadTanques(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }

        if (r0 > 0.001f)
        {
            Vector3 centro = { base.x, base.y, base.z };
            QuadTanques(centro, { base.x + c0 * r0, base.y, base.z + s0 * r0 }, { base.x + c1 * r0, base.y, base.z + s1 * r0 }, centro, tapaAbajo);
        }
    }

    rlEnd();
}


// Cristal facetado a lo largo de un eje: cuerpo hexagonal y punta.
static void CristalTanques(
    Vector3 base,
    Vector3 eje,
    float radio,
    float largo,
    float punta,
    Color color,
    float giro = 0.0f
)
{
    eje = Vector3Normalize(eje);
    Vector3 referencia = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, referencia));
    Vector3 v = Vector3CrossProduct(eje, u);

    Vector3 tope = Vector3Add(base, Vector3Scale(eje, largo));
    Vector3 pico = Vector3Add(base, Vector3Scale(eje, largo + punta));
    const int LADOS = 6;
    float paso = 6.2832f / (float)LADOS;

    rlBegin(RL_QUADS);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        Vector3 d0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 d1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 medio = Vector3Normalize(Vector3Add(d0, d1));

        // Las facetas que miran al planeta (izquierda, arriba) brillan mas.
        float f = 0.74f - 0.16f * medio.x + 0.14f * medio.z + 0.16f * medio.y;
        if (k & 1) f *= 0.9f;
        Color cara = TonoTanques(color, f);
        Color caraPunta = TonoTanques(color, f + 0.18f);

        Vector3 b0 = Vector3Add(base, Vector3Scale(d0, radio));
        Vector3 b1 = Vector3Add(base, Vector3Scale(d1, radio));
        Vector3 t0 = Vector3Add(tope, Vector3Scale(d0, radio * 0.9f));
        Vector3 t1 = Vector3Add(tope, Vector3Scale(d1, radio * 0.9f));

        QuadTanques(b0, b1, t1, t0, cara);
        QuadTanques(t0, t1, pico, pico, caraPunta);
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloTanques(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadTanques(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorTanques(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
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


static void ResplandorSueloTanques(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorTanques(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano inclinado como la vista).
static void ResplandorFrenteTanques(Vector3 c, float radio, Color color)
{
    ResplandorTanques(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.65f, 0.76f }, radio, color);
}


//==================================================
// CIELO: PLANETA ANILLADO, LUNAS Y ESTRELLAS
//==================================================

// Planeta gigante en 2D: bandas, terminador y anillo inclinado.
static void DibujarPlanetaTanques(Vector2 centro, float radio, float t)
{
    // Halo del planeta.
    DrawCircleGradient(centro, radio * 1.9f, Fade(Color{ 120, 200, 220, 255 }, 0.16f), Fade(Color{ 90, 120, 200, 255 }, 0.0f));

    const float inclinacion = -0.32f;
    float ci = std::cos(inclinacion), si = std::sin(inclinacion);

    // Mitad trasera del anillo (detras del disco).
    for (int pasada = 0; pasada < 2; pasada++)
    {
        for (int banda = 0; banda < 3; banda++)
        {
            float r = radio * (1.45f + 0.16f * (float)banda);
            Color c = banda == 1 ? Color{ 236, 214, 186, 255 } : Color{ 196, 176, 170, 255 };
            float alfa = banda == 1 ? 0.75f : 0.5f;
            const int SEG = 48;

            for (int k = 0; k < SEG; k++)
            {
                float a0 = 6.2832f * (float)k / (float)SEG;
                float a1 = 6.2832f * (float)(k + 1) / (float)SEG;
                bool delante = std::sin(a0) > 0.0f;

                if (delante != (pasada == 1))
                {
                    continue;
                }

                Vector2 p0 = { std::cos(a0) * r, std::sin(a0) * r * 0.24f };
                Vector2 p1 = { std::cos(a1) * r, std::sin(a1) * r * 0.24f };
                Vector2 q0 = { centro.x + p0.x * ci - p0.y * si, centro.y + p0.x * si + p0.y * ci };
                Vector2 q1 = { centro.x + p1.x * ci - p1.y * si, centro.y + p1.x * si + p1.y * ci };
                DrawLineEx(q0, q1, radio * (banda == 1 ? 0.09f : 0.05f), Fade(c, alfa));
            }
        }

        if (pasada == 1)
        {
            break;
        }

        // Disco por filas: bandas de gas que se mueven apenas y el lado
        // derecho en sombra (la luz viene de la izquierda).
        int filas = (int)(radio * 2.0f);

        for (int f = 0; f < filas; f++)
        {
            float dy = -radio + (float)f + 0.5f;
            float media = std::sqrt(radio * radio - dy * dy);
            float lat = dy / radio;
            float banda = std::sin(lat * 9.0f + std::sin(lat * 4.0f + t * 0.05f) * 0.8f);
            Color base = banda > 0.35f
                ? Color{ 150, 214, 214, 255 }
                : (banda < -0.45f ? Color{ 214, 150, 120, 255 } : Color{ 120, 176, 200, 255 });
            Color luz = MezclaTanques(base, Color{ 250, 248, 255, 255 }, 0.18f);
            Color sombra = MezclaTanques(TonoTanques(base, 0.28f), Color{ 30, 18, 60, 255 }, 0.5f);

            DrawRectangleGradientH(
                (int)(centro.x - media),
                (int)(centro.y + dy),
                (int)(media * 2.0f) + 1,
                1,
                luz,
                sombra
            );
        }

        // Gran mancha de tormenta.
        DrawEllipse((int)(centro.x - radio * 0.28f), (int)(centro.y + radio * 0.3f), radio * 0.16f, radio * 0.08f, Fade(Color{ 236, 160, 120, 255 }, 0.8f));
    }
}


// Cielo (2D, antes del 3D): indigo arriba, resplandor magenta abajo,
// estrellas, nebulosa, el planeta anillado, dos lunas y estrellas fugaces.
static void DibujarCieloTanques(const Camera3D& vista, float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    float horizonte = GetWorldToScreen({ 0.0f, -10.0f, PARED_Z_TANQUES }, vista).y;
    if (horizonte < (float)alto * 0.10f) horizonte = (float)alto * 0.10f;
    if (horizonte > (float)alto * 0.50f) horizonte = (float)alto * 0.50f;

    int h1 = (int)(horizonte * 0.5f);
    int h2 = (int)horizonte;

    DrawRectangleGradientV(0, 0, ancho, h1, Color{ 10, 8, 34, 255 }, Color{ 40, 20, 78, 255 });
    DrawRectangleGradientV(0, h1, ancho, h2 - h1, Color{ 40, 20, 78, 255 }, Color{ 150, 82, 150, 255 });
    DrawRectangleGradientV(0, h2, ancho, alto - h2, Color{ 150, 82, 150, 255 }, COLOR_NIEBLA_TANQUES);

    if (!detalle)
    {
        return;
    }

    // Nebulosa: velos anchos de color que derivan muy despacio.
    for (int k = 0; k < 5; k++)
    {
        float x = std::fmod(Hash01Tanques(k, 40) * (float)ancho + t * (2.0f + 2.0f * Hash01Tanques(k, 41)), (float)ancho + 400.0f) - 200.0f;
        float y = horizonte * (0.15f + 0.5f * Hash01Tanques(k, 42));
        Color c = k % 2 == 0 ? Color{ 200, 90, 190, 255 } : Color{ 80, 150, 220, 255 };
        DrawEllipse((int)x, (int)y, 220.0f + 160.0f * Hash01Tanques(k, 43), 26.0f + 18.0f * Hash01Tanques(k, 44), Fade(c, 0.10f));
    }

    // Estrellas: mas densas arriba, titilan cada una a su ritmo.
    for (int k = 0; k < 110; k++)
    {
        float x = Hash01Tanques(k, 1) * (float)ancho;
        float y = Hash01Tanques(k, 2) * Hash01Tanques(k, 2) * horizonte;
        float brillo = 0.4f + 0.6f * (0.5f + 0.5f * std::sin(t * (1.3f + 2.2f * Hash01Tanques(k, 3)) + (float)k));
        float r = Hash01Tanques(k, 4) < 0.14f ? 1.8f : 1.1f;
        Color c = Hash01Tanques(k, 5) < 0.3f ? Color{ 255, 210, 240, 255 } : Color{ 220, 234, 255, 255 };
        DrawCircleV({ x, y }, r, Fade(c, brillo * (1.0f - y / (horizonte + 1.0f) * 0.7f)));
    }

    // Planeta anillado arriba a la izquierda (es la luz de la escena).
    float radioPlaneta = (float)alto * 0.13f;
    DibujarPlanetaTanques({ (float)ancho * 0.09f, (float)alto * 0.1f }, radioPlaneta, t);

    // Dos lunas que cruzan muy despacio.
    Vector2 luna1 = { (float)ancho * (0.80f + 0.004f * std::sin(t * 0.05f)), (float)alto * 0.165f };
    float r1 = (float)alto * 0.032f;
    DrawCircleGradient(luna1, r1 * 3.0f, Fade(Color{ 230, 200, 255, 255 }, 0.18f), Fade(Color{ 200, 160, 255, 255 }, 0.0f));
    DrawCircleV(luna1, r1, Color{ 226, 214, 240, 255 });
    DrawCircleV({ luna1.x + r1 * 0.3f, luna1.y + r1 * 0.05f }, r1 * 0.82f, Color{ 160, 140, 190, 255 });
    DrawCircleV({ luna1.x - r1 * 0.35f, luna1.y - r1 * 0.2f }, r1 * 0.18f, Color{ 196, 182, 216, 255 });

    Vector2 luna2 = { (float)ancho * (0.885f - 0.006f * std::sin(t * 0.04f)), (float)alto * 0.075f };
    float r2 = (float)alto * 0.017f;
    DrawCircleGradient(luna2, r2 * 2.6f, Fade(Color{ 255, 190, 150, 255 }, 0.16f), Fade(Color{ 255, 160, 120, 255 }, 0.0f));
    DrawCircleV(luna2, r2, Color{ 246, 196, 160, 255 });
    DrawCircleV({ luna2.x + r2 * 0.35f, luna2.y }, r2 * 0.75f, Color{ 190, 130, 110, 255 });

    // Estrellas fugaces cada pocos segundos.
    for (int k = 0; k < 2; k++)
    {
        float ciclo = 5.5f + 2.5f * (float)k;
        float u = std::fmod(t + (float)k * 2.7f, ciclo) / 0.7f;

        if (u > 1.0f)
        {
            continue;
        }

        int vuelta = (int)((t + (float)k * 2.7f) / ciclo);
        float x0 = (0.25f + 0.5f * Hash01Tanques(vuelta, 60 + k)) * (float)ancho;
        float y0 = (0.05f + 0.3f * Hash01Tanques(vuelta, 70 + k)) * horizonte;
        Vector2 cabeza = { x0 + u * 220.0f, y0 + u * 70.0f };
        Vector2 cola = { cabeza.x - 90.0f, cabeza.y - 29.0f };
        float alfa = std::sin(u * 3.1416f);
        DrawLineEx(cola, cabeza, 2.0f, Fade(Color{ 255, 240, 250, 255 }, alfa * 0.8f));
        DrawCircleV(cabeza, 2.2f, Fade(WHITE, alfa));
    }
}


//==================================================
// HORIZONTE Y ABISMO (el otro lado del canon)
//==================================================

// Altura del borde del otro lado del canon (ondulado, con muescas).
static float AlturaParedLejanaTanques(float x)
{
    return -10.0f + 1.8f * std::sin(x * 0.11f) + 1.2f * std::sin(x * 0.29f + 1.3f) + 0.9f * std::sin(x * 0.71f + 0.4f);
}


// Pared del otro lado: columnas con estratos y la meseta encima.
static void DibujarParedLejanaTanques(int paso)
{
    const float ANCHO_COLUMNA = 3.0f;
    float pasoX = ANCHO_COLUMNA * (float)paso;

    rlBegin(RL_QUADS);

    for (float x = -66.0f; x < 66.0f; x += pasoX)
    {
        float x1 = x + pasoX;
        int k = (int)std::floor(x / ANCHO_COLUMNA) + 40;
        float z0 = PARED_Z_TANQUES + (Hash01Tanques(k, 1) - 0.5f) * 1.2f;
        float y0 = AlturaParedLejanaTanques(x);
        float y1 = AlturaParedLejanaTanques(x1);
        float fondo = -36.0f;

        // Estratos: tres bandas de roca hacia abajo.
        float cortes[4] = { 0.0f, 2.2f + Hash01Tanques(k, 2), 5.0f + Hash01Tanques(k, 3) * 1.5f, 30.0f };
        Color bandas[3] = { COLOR_ESTRATO_TANQUES, COLOR_ESTRATO_OSCURO_TANQUES, COLOR_ROCA_OSCURA_TANQUES };

        for (int b = 0; b < 3; b++)
        {
            float a0 = y0 - cortes[b], a1 = y1 - cortes[b];
            float d0 = y0 - cortes[b + 1], d1 = y1 - cortes[b + 1];
            if (d0 < fondo) d0 = fondo;
            if (d1 < fondo) d1 = fondo;
            Color c = TonoTanques(bandas[b], 0.86f + 0.14f * Hash01Tanques(k, 4 + b));

            // Cada banda se oscurece hacia abajo: el abismo se traga la luz.
            Color arriba = MezclaTanques(TinteLejosTanques(c, { x, a0, z0 }), COLOR_NIEBLA_TANQUES, 0.15f * (float)b);
            Color abajo = MezclaTanques(TinteLejosTanques(c, { x, d0, z0 }), COLOR_NIEBLA_TANQUES, 0.3f + 0.3f * (float)b);
            VerticeTanques({ x, d0, z0 }, abajo);
            VerticeTanques({ x1, d1, z0 }, abajo);
            VerticeTanques({ x1, a1, z0 }, arriba);
            VerticeTanques({ x, a0, z0 }, arriba);
        }

        // Meseta: una tapa angosta que recibe la luz del planeta.
        QuadLejosTanques(
            { x, y0, z0 },
            { x1, y1, z0 },
            { x1, y1 + 0.2f, z0 - 2.5f },
            { x, y0 + 0.2f, z0 - 2.5f },
            TonoTanques(COLOR_ESTRATO_TANQUES, 1.05f + 0.1f * Hash01Tanques(k, 9))
        );
    }

    rlEnd();
}


// Siluetas de mesetas y agujas (dos profundidades).
static void DibujarMesetasTanques(float z, float x0, float x1, float yBase, float yTope, float variacion, Color color, int semilla, int paso)
{
    float x = x0;
    int k = 0;

    rlBegin(RL_QUADS);

    while (x < x1)
    {
        float ancho = (4.0f + 9.0f * Hash01Tanques(k, semilla)) * (float)paso;
        bool aguja = Hash01Tanques(k, semilla + 1) < 0.3f;
        float alto = yTope - variacion * Hash01Tanques(k, semilla + 2);
        float zk = z + (Hash01Tanques(k, semilla + 3) - 0.5f) * 8.0f;
        Color c = TonoTanques(color, 0.88f + 0.16f * Hash01Tanques(k, semilla + 4));

        if (aguja)
        {
            // Aguja fina que se afina hacia la punta.
            float medio = x + ancho * 0.5f;
            float a = ancho * 0.18f;
            QuadLejosTanques({ medio - a, yBase, zk }, { medio + a, yBase, zk }, { medio + a * 0.35f, alto + variacion * 0.6f, zk }, { medio - a * 0.35f, alto + variacion * 0.6f, zk }, c);
            QuadLejosTanques({ x, yBase, zk + 1.0f }, { x + ancho, yBase, zk + 1.0f }, { x + ancho * 0.8f, alto - variacion * 0.7f, zk + 1.0f }, { x + ancho * 0.2f, alto - variacion * 0.7f, zk + 1.0f }, TonoTanques(c, 0.92f));
        }
        else
        {
            // Meseta de techo plano con laderas inclinadas.
            float talud = ancho * 0.14f;
            QuadLejosTanques({ x, yBase, zk }, { x + ancho, yBase, zk }, { x + ancho - talud, alto, zk }, { x + talud, alto, zk }, c);
            QuadLejosTanques({ x + talud, alto, zk }, { x + ancho - talud, alto, zk }, { x + ancho - talud * 1.2f, alto + 0.3f, zk - 4.0f }, { x + talud * 1.2f, alto + 0.3f, zk - 4.0f }, MezclaTanques(c, COLOR_BRUMA_TANQUES, 0.25f));
        }

        x += ancho * (0.75f + 0.35f * Hash01Tanques(k, semilla + 5));
        k++;
    }

    rlEnd();
}


// Gran anillo de una nave antigua clavado del otro lado del canon.
static void DibujarAnilloNaveLejanaTanques()
{
    Vector3 centro = { 34.0f, -12.0f, PARED_Z_TANQUES - 6.0f };
    const int SEG = 22;
    float radio = 9.0f;

    for (int k = 0; k < SEG; k++)
    {
        // Falta un tramo: el anillo esta roto.
        if (k >= 7 && k <= 9)
        {
            continue;
        }

        float a = 3.1416f * 0.08f + 3.1416f * 0.84f * (float)k / (float)(SEG - 1);
        Vector3 p = { centro.x + std::cos(a) * radio, centro.y + std::sin(a) * radio, centro.z };
        Color c = TinteLejosTanques(k % 4 == 0 ? COLOR_METAL_TANQUES : TonoTanques(COLOR_CASCO_TANQUES, 0.7f), p);

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(a * RAD2DEG, 0.0f, 0.0f, 1.0f);
        CuboTanques({ 0.0f, 0.0f, 0.0f }, { 1.4f, 2.6f, 1.6f }, c);
        rlPopMatrix();
    }
}


// Rocas que flotan sobre el abismo (baja gravedad), con su cristal.
static Vector3 PosicionRocaFlotanteTanques(int k, float t)
{
    const Vector3 BASES[5] =
    {
        { -11.0f, -3.2f, -19.5f },
        { -3.0f, -5.0f, -24.0f },
        { 6.5f, -2.6f, -21.0f },
        { 15.5f, -4.2f, -18.2f },
        { 24.0f, -6.0f, -25.0f }
    };

    Vector3 p = BASES[k];
    p.y += std::sin(t * 0.55f + (float)k * 1.7f) * 0.35f;
    return p;
}


static void DibujarRocasFlotantesTanques(float t)
{
    for (int k = 0; k < 5; k++)
    {
        Vector3 p = PosicionRocaFlotanteTanques(k, t);
        float e = 0.7f + 0.5f * Hash01Tanques(k, 80);
        float giro = t * 0.08f * (k % 2 == 0 ? 1.0f : -1.0f) + (float)k;

        PrismaTanques({ p.x, p.y - 1.7f * e, p.z }, 0.05f, 1.15f * e, 1.7f * e, 6, TinteTanques(COLOR_ROCA_OSCURA_TANQUES, p), giro);
        PrismaTanques(p, 1.15f * e, 0.85f * e, 0.35f * e, 6, TinteTanques(COLOR_ROCA_TANQUES, p), giro);

        Color cristal = k % 2 == 0 ? COLOR_CRISTAL_ROSA_TANQUES : COLOR_CRISTAL_LILA_TANQUES;
        CristalTanques({ p.x, p.y + 0.3f * e, p.z }, { 0.15f, 1.0f, 0.1f }, 0.22f * e, 0.9f * e, 0.35f * e, cristal, giro);
        CristalTanques({ p.x + 0.35f * e, p.y + 0.3f * e, p.z + 0.1f }, { 0.6f, 1.0f, 0.2f }, 0.13f * e, 0.5f * e, 0.2f * e, cristal, giro);
    }
}


//==================================================
// TERRENO: CORNISA, PRECIPICIO Y PAREDES DEL CANON
//==================================================

// Borde del precipicio: zigzag fijo para cada columna.
static float LabioTanques(float x)
{
    int k = (int)std::floor(x / 2.0f) + 30;
    float u = x / 2.0f - std::floor(x / 2.0f);
    float a = LABIO_Z_TANQUES + (Hash01Tanques(k, 11) - 0.5f) * 0.9f;
    float b = LABIO_Z_TANQUES + (Hash01Tanques(k + 1, 11) - 0.5f) * 0.9f;
    return a + (b - a) * u;
}


// Altura del terreno fuera de la arena: cornisa con un leve desnivel.
static float AlturaCornisaTanques(float x, float z)
{
    float h = 0.06f * std::sin(x * 0.9f + z * 0.4f) + 0.05f * std::sin(z * 1.3f - x * 0.3f);
    float ax = std::fabs(x);

    // Las cornisas laterales suben hacia las paredes del canon.
    if (ax > 22.0f)
    {
        h += (ax - 22.0f) * 0.35f;
    }

    return h - 0.05f;
}


static void DibujarCornisaTanques(int paso)
{
    const float CELDA = 2.0f;
    float tam = CELDA * (float)(paso > 1 ? 2 : 1);
    float xMin = ARENA_X_MIN - BORDE_TANQUES;
    float xMax = ARENA_X_MAX + BORDE_TANQUES;
    float zMin = ARENA_Z_MIN - BORDE_TANQUES;
    float zMax = ARENA_Z_MAX + BORDE_TANQUES;

    rlBegin(RL_QUADS);

    for (float x = -34.0f; x < 34.0f; x += tam)
    {
        for (float z = -15.0f; z < 16.0f; z += tam)
        {
            float x1 = x + tam;
            float z1 = z + tam;

            // Bajo la arena y su bordillo no hace falta suelo.
            if (x >= xMin && x1 <= xMax && z >= zMin && z1 <= zMax)
            {
                continue;
            }

            // El fondo termina en el labio del precipicio.
            float l0 = LabioTanques(x);
            float l1 = LabioTanques(x1);

            if (z1 <= std::fmin(l0, l1) - 0.01f)
            {
                continue;
            }

            float za0 = z < l0 ? l0 : z;
            float za1 = z < l1 ? l1 : z;

            int k = (int)(x * 7.0f + z * 13.0f) + 500;
            Color c = TonoTanques(COLOR_CORNISA_TANQUES, 0.88f + 0.2f * Hash01Tanques(k, 3));

            QuadNieblaTanques(
                { x, AlturaCornisaTanques(x, z1), z1 },
                { x1, AlturaCornisaTanques(x1, z1), z1 },
                { x1, AlturaCornisaTanques(x1, za1), za1 },
                { x, AlturaCornisaTanques(x, za0), za0 },
                c
            );
        }
    }

    // Cara del precipicio (se asoma apenas bajo el labio) y la penumbra
    // del abismo hacia abajo.
    for (float x = -34.0f; x < 34.0f; x += 2.0f)
    {
        float l0 = LabioTanques(x);
        float l1 = LabioTanques(x + 2.0f);
        Color arriba = TonoTanques(COLOR_ROCA_OSCURA_TANQUES, 0.9f);
        Color abajo = COLOR_NIEBLA_TANQUES;

        VerticeTanques({ x, AlturaCornisaTanques(x, l0), l0 }, TinteTanques(arriba, { x, 0.0f, l0 }));
        VerticeTanques({ x, -16.0f, l0 - 2.5f }, abajo);
        VerticeTanques({ x + 2.0f, -16.0f, l1 - 2.5f }, abajo);
        VerticeTanques({ x + 2.0f, AlturaCornisaTanques(x + 2.0f, l1), l1 }, TinteTanques(arriba, { x + 2.0f, 0.0f, l1 }));
    }

    rlEnd();
}


static void DibujarRacimoTanques(Vector3 base, float e, Color color, int semilla);


// Paredes laterales del canon: pilares de roca estratificada que se
// solapan (cada uno con tres escalones que se afinan hacia arriba).
static void DibujarParedesLateralesTanques(int paso)
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;

        for (int k = 0; k < 11; k += paso)
        {
            float z = 13.0f - (float)k * 2.5f;
            float alto = 3.2f + 3.8f * Hash01Tanques(k, 30 + lado);

            // Hacia el fondo la pared baja en escalones y deja ver el cielo.
            if (z < -4.0f)
            {
                alto *= 0.5f;
            }

            float radio = (2.4f + 0.8f * Hash01Tanques(k, 34 + lado)) * (paso > 1 ? 1.3f : 1.0f);
            float xi = 23.4f + 0.9f * Hash01Tanques(k, 32 + lado);
            Vector3 c = { s * (xi + radio), AlturaCornisaTanques(xi * s, z) - 0.3f, z };
            float giro = (float)k * 1.3f;

            float h0 = alto * 0.45f;
            float h1 = alto * 0.33f;
            float h2 = alto * 0.22f;

            PrismaTanques(c, radio, radio * 0.92f, h0, 7, TinteTanques(COLOR_ESTRATO_OSCURO_TANQUES, c), giro);
            PrismaTanques({ c.x, c.y + h0, c.z }, radio * 0.86f, radio * 0.78f, h1, 7, TinteTanques(COLOR_ESTRATO_TANQUES, c), giro + 0.2f);
            PrismaTanques({ c.x, c.y + h0 + h1, c.z }, radio * 0.7f, radio * 0.55f, h2, 7, TinteTanques(TonoTanques(COLOR_ROCA_CLARA_TANQUES, 0.92f), c), giro + 0.5f);

            // Algunos pilares llevan cristales en la cima.
            if (Hash01Tanques(k, 36 + lado) < 0.4f)
            {
                Vector3 cima = { c.x - s * radio * 0.2f, c.y + alto, c.z };
                DibujarRacimoTanques(cima, 0.8f, k % 2 == 0 ? COLOR_CRISTAL_ROSA_TANQUES : COLOR_CRISTAL_LILA_TANQUES, 20 + k);
            }
        }
    }
}


//==================================================
// ARENA: LOSAS, BORDILLO, EMISORES Y RACIMOS DE COBERTURA
//==================================================

// Vertice de la grilla de losas, movido un poco para que las juntas no
// queden rectas (los del borde se quedan quietos).
static Vector3 VerticeLosaTanques(int i, int j)
{
    const int NX = 16;
    const int NZ = 9;
    float x = ARENA_X_MIN + (ARENA_X_MAX - ARENA_X_MIN) * (float)i / (float)NX;
    float z = ARENA_Z_MIN + (ARENA_Z_MAX - ARENA_Z_MIN) * (float)j / (float)NZ;

    if (i > 0 && i < NX)
    {
        x += (Hash01Tanques(i * 31 + j, 5) - 0.5f) * 0.55f;
    }

    if (j > 0 && j < NZ)
    {
        z += (Hash01Tanques(i * 17 + j, 6) - 0.5f) * 0.55f;
    }

    return { x, 0.03f, z };
}


static Vector3 HaciaTanques(Vector3 a, Vector3 centro, float f)
{
    return { centro.x + (a.x - centro.x) * f, a.y, centro.z + (a.z - centro.z) * f };
}


static void DibujarSueloArenaTanques(bool detalle)
{
    // Juntas: una lamina oscura bajo todas las losas.
    rlBegin(RL_QUADS);
    QuadNieblaTanques(
        { ARENA_X_MIN, 0.0f, ARENA_Z_MAX },
        { ARENA_X_MAX, 0.0f, ARENA_Z_MAX },
        { ARENA_X_MAX, 0.0f, ARENA_Z_MIN },
        { ARENA_X_MIN, 0.0f, ARENA_Z_MIN },
        COLOR_JUNTA_TANQUES
    );

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            Vector3 a = VerticeLosaTanques(i, j);
            Vector3 b = VerticeLosaTanques(i + 1, j);
            Vector3 c = VerticeLosaTanques(i + 1, j + 1);
            Vector3 d = VerticeLosaTanques(i, j + 1);
            Vector3 centro = { (a.x + b.x + c.x + d.x) * 0.25f, 0.03f, (a.z + b.z + c.z + d.z) * 0.25f };

            float h = Hash01Tanques(i, j + 200);
            Color base = h < 0.18f ? MezclaTanques(COLOR_ROCA_TANQUES, COLOR_CRISTAL_LILA_TANQUES, 0.12f) : COLOR_ROCA_TANQUES;
            Color color = TonoTanques(base, 0.9f + 0.18f * Hash01Tanques(i, j + 300));

            // Bisel: anillo algo mas oscuro y la cara central clara.
            Vector3 a1 = HaciaTanques(a, centro, 0.94f);
            Vector3 b1 = HaciaTanques(b, centro, 0.94f);
            Vector3 c1 = HaciaTanques(c, centro, 0.94f);
            Vector3 d1 = HaciaTanques(d, centro, 0.94f);
            QuadNieblaTanques(d1, c1, b1, a1, TonoTanques(color, 0.86f));

            Vector3 a2 = HaciaTanques(a, centro, 0.84f);
            Vector3 b2 = HaciaTanques(b, centro, 0.84f);
            Vector3 c2 = HaciaTanques(c, centro, 0.84f);
            Vector3 d2 = HaciaTanques(d, centro, 0.84f);
            a2.y = b2.y = c2.y = d2.y = 0.045f;
            QuadNieblaTanques(d2, c2, b2, a2, color);
        }
    }

    rlEnd();

    if (!detalle)
    {
        return;
    }

    // Grietas y vetas de cristal en algunas losas.
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            float h = Hash01Tanques(i + 50, j + 50);

            if (h > 0.12f)
            {
                continue;
            }

            Vector3 a = VerticeLosaTanques(i, j);
            Vector3 c = VerticeLosaTanques(i + 1, j + 1);
            Vector3 centro = { (a.x + c.x) * 0.5f, 0.05f, (a.z + c.z) * 0.5f };
            float giro = Hash01Tanques(i, j + 90) * 180.0f;
            bool veta = h < 0.05f;

            rlPushMatrix();
            rlTranslatef(centro.x, 0.0f, centro.z);
            rlRotatef(giro, 0.0f, 1.0f, 0.0f);
            Color c1 = veta ? TonoTanques(COLOR_CRISTAL_LILA_TANQUES, 0.8f) : MezclaTanques(COLOR_ROCA_TANQUES, COLOR_JUNTA_TANQUES, 0.55f);
            CuboTanques({ -0.25f, 0.05f, 0.0f }, { 0.8f, 0.012f, 0.05f }, c1);
            CuboTanques({ 0.3f, 0.05f, 0.12f }, { 0.45f, 0.012f, 0.04f }, c1);
            rlPopMatrix();
        }
    }
}


// Bordillo de roca con la franja de energia en el canto interior.
static void TramoBordilloTanques(Vector3 centro, Vector3 tamano, bool alongX, float lado, int k)
{
    Color roca = TonoTanques(COLOR_ROCA_OSCURA_TANQUES, 0.92f + 0.16f * Hash01Tanques(k, 21));
    CuboNieblaTanques(centro, tamano, roca);
    CuboNieblaTanques({ centro.x, centro.y + tamano.y * 0.5f + 0.03f, centro.z }, { tamano.x - 0.08f, 0.06f, tamano.z - 0.08f }, TonoTanques(COLOR_ROCA_CLARA_TANQUES, 0.86f));

    // Franja de energia sobre el canto que da a la arena.
    Vector3 f = centro;
    f.y = centro.y + tamano.y * 0.5f + 0.07f;

    if (alongX)
    {
        f.z += lado * (tamano.z * 0.5f - 0.06f);
        CuboTanques(f, { tamano.x - 0.2f, 0.035f, 0.06f }, COLOR_ENERGIA_TANQUES);
    }
    else
    {
        f.x += lado * (tamano.z * 0.5f - 0.06f);
        CuboTanques(f, { 0.06f, 0.035f, tamano.x - 0.2f }, COLOR_ENERGIA_TANQUES);
    }
}


static void DibujarBordilloTanques()
{
    float g = BORDE_TANQUES;

    // Lados largos (fondo y frente): el canto interior coincide con Z.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float zc = lado < 0 ? ARENA_Z_MIN - g * 0.5f : ARENA_Z_MAX + g * 0.5f;
        float x = ARENA_X_MIN - g;
        int k = 0;

        while (x < ARENA_X_MAX + g - 0.01f)
        {
            float largo = 1.7f + 0.8f * Hash01Tanques(k, 20 + lado);
            if (x + largo > ARENA_X_MAX + g) largo = ARENA_X_MAX + g - x;
            float alto = 0.42f + 0.14f * Hash01Tanques(k, 24 + lado);
            if (lado > 0) alto *= 0.75f;   // el del frente, mas bajo

            TramoBordilloTanques({ x + largo * 0.5f, alto * 0.5f, zc }, { largo - 0.05f, alto, g }, true, (float)-lado, k + lado * 100);
            x += largo;
            k++;
        }
    }

    // Lados cortos (izquierda y derecha).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float xc = lado < 0 ? ARENA_X_MIN - g * 0.5f : ARENA_X_MAX + g * 0.5f;
        float z = ARENA_Z_MIN;
        int k = 0;

        while (z < ARENA_Z_MAX - 0.01f)
        {
            float largo = 1.7f + 0.8f * Hash01Tanques(k, 27 + lado);
            if (z + largo > ARENA_Z_MAX) largo = ARENA_Z_MAX - z;
            float alto = 0.42f + 0.14f * Hash01Tanques(k, 29 + lado);

            // tamano.x = largo del tramo; tamano.z = grosor.
            Vector3 c = { xc, alto * 0.5f, z + largo * 0.5f };
            Color roca = TonoTanques(COLOR_ROCA_OSCURA_TANQUES, 0.92f + 0.16f * Hash01Tanques(k, 31 + lado));
            CuboNieblaTanques(c, { g, alto, largo - 0.05f }, roca);
            CuboNieblaTanques({ c.x, alto + 0.03f, c.z }, { g - 0.08f, 0.06f, largo - 0.13f }, TonoTanques(COLOR_ROCA_CLARA_TANQUES, 0.86f));
            CuboTanques({ c.x - (float)lado * (g * 0.5f - 0.06f), alto + 0.07f, c.z }, { 0.06f, 0.035f, largo - 0.2f }, COLOR_ENERGIA_TANQUES);

            z += largo;
            k++;
        }
    }
}


// Emisor de las esquinas: poste, orbe flotante y anillo que gira.
static Vector3 OrbeEmisorTanques(int k, float t)
{
    float sx = (k & 1) ? 1.0f : -1.0f;
    float sz = (k & 2) ? 1.0f : -1.0f;
    float y = 1.75f + std::sin(t * 1.8f + (float)k) * 0.08f;
    return { sx * (ARENA_X_MAX + BORDE_TANQUES * 0.5f), y, sz * (ARENA_Z_MAX + BORDE_TANQUES * 0.5f) };
}


static void DibujarEmisoresTanques(float t)
{
    for (int k = 0; k < 4; k++)
    {
        Vector3 orbe = OrbeEmisorTanques(k, t);
        Vector3 base = { orbe.x, 0.0f, orbe.z };

        PrismaTanques(base, 0.62f, 0.5f, 0.55f, 6, TinteTanques(COLOR_ROCA_OSCURA_TANQUES, base), 0.3f);
        PrismaTanques({ base.x, 0.55f, base.z }, 0.26f, 0.2f, 0.75f, 4, COLOR_METAL_TANQUES, 0.785f);
        PrismaTanques({ base.x, 1.3f, base.z }, 0.3f, 0.12f, 0.16f, 4, TonoTanques(COLOR_METAL_TANQUES, 1.3f), 0.785f);

        // Anillo partido en cuatro gajos que gira alrededor del orbe.
        float giro = t * 1.6f + (float)k;

        for (int g = 0; g < 4; g++)
        {
            float a = giro + 1.5708f * (float)g;
            CuboTanques({ orbe.x + std::cos(a) * 0.42f, orbe.y, orbe.z + std::sin(a) * 0.42f }, { 0.12f, 0.08f, 0.12f }, COLOR_ENERGIA_TANQUES);
        }

        DrawSphereEx(orbe, 0.17f, 6, 8, Color{ 236, 220, 255, 255 });
    }
}


// Racimos de cristal de cobertura: base de roca con el radio exacto de
// la colision y cristales que no salen de ese circulo.
static void DibujarCoberturasTanques(const MinijuegoTanquesPlasma& m, float t)
{
    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < CANTIDAD_OBSTACULOS_TANQUES; i++)
    {
        Vector3 c = { centros[i].x, 0.0f, centros[i].y };
        float r = radios[i];
        float e = r / 2.0f;
        float pulso = 0.94f + 0.08f * std::sin(t * 2.2f + (float)i * 2.0f);
        float golpe = m.destelloCristal[i] / 0.35f;
        Color cristal = MezclaTanques(TonoTanques(COLOR_CRISTAL_TANQUES, pulso), WHITE, golpe * 0.6f);
        Color cristalHondo = MezclaTanques(cristal, COLOR_CRISTAL_LILA_TANQUES, 0.35f);

        // Base: roca de dos escalones (el de abajo coincide con el radio).
        PrismaTanques(c, r, r * 0.94f, 0.32f, 14, COLOR_ROCA_OSCURA_TANQUES, 0.2f);
        PrismaTanques({ c.x, 0.32f, c.z }, r * 0.9f, r * 0.72f, 0.3f, 14, COLOR_CORNISA_TANQUES, 0.4f);

        // Cristal principal y una corona de cristales inclinados.
        CristalTanques({ c.x, 0.5f, c.z }, { 0.05f, 1.0f, -0.04f }, 0.5f * e, 2.3f * e, 0.8f * e, cristal, 0.3f * (float)i);

        const int CORONA = 6;

        for (int k = 0; k < CORONA; k++)
        {
            float a = 6.2832f * (float)k / (float)CORONA + (float)i * 0.7f;
            float d = r * (0.36f + 0.12f * Hash01Tanques(k, i + 60));
            Vector3 b = { c.x + std::cos(a) * d, 0.5f, c.z + std::sin(a) * d };
            Vector3 eje = { std::cos(a) * 0.55f, 1.0f, std::sin(a) * 0.55f };
            float largo = (0.9f + 0.9f * Hash01Tanques(k, i + 70)) * e;
            Color color = k % 3 == 0 ? cristalHondo : cristal;
            CristalTanques(b, eje, (0.22f + 0.08f * Hash01Tanques(k, i + 80)) * e, largo, 0.35f * e, color, a);
        }
    }
}


//==================================================
// UTILERIA: ROCAS, CRISTALES, RESTOS DE NAVES Y FLORA
//==================================================

static void DibujarRocaTanques(Vector3 base, float e, float giro, Color color)
{
    Color c = TinteTanques(color, base);
    PrismaTanques(base, 0.75f * e, 0.55f * e, 0.55f * e, 6, c, giro);
    PrismaTanques({ base.x, base.y + 0.55f * e, base.z }, 0.55f * e, 0.18f * e, 0.35f * e, 6, TonoTanques(c, 1.08f), giro + 0.4f);
    PrismaTanques({ base.x + 0.6f * e, base.y, base.z + 0.25f * e }, 0.35f * e, 0.2f * e, 0.35f * e, 5, TonoTanques(c, 0.9f), giro + 1.0f);
}


// Racimo decorativo de cristales (rosa o lila).
static void DibujarRacimoTanques(Vector3 base, float e, Color color, int semilla)
{
    Color c = TinteTanques(color, base);
    CristalTanques(base, { 0.1f, 1.0f, 0.05f }, 0.24f * e, 1.1f * e, 0.4f * e, c, (float)semilla);

    for (int k = 0; k < 4; k++)
    {
        float a = 1.5708f * (float)k + (float)semilla * 0.9f;
        Vector3 b = { base.x + std::cos(a) * 0.25f * e, base.y, base.z + std::sin(a) * 0.25f * e };
        float largo = (0.45f + 0.45f * Hash01Tanques(k, semilla)) * e;
        CristalTanques(b, { std::cos(a) * 0.7f, 1.0f, std::sin(a) * 0.7f }, 0.13f * e, largo, 0.22f * e, TonoTanques(c, 0.9f + 0.1f * (float)(k & 1)), a);
    }
}


// Plancha del casco de una nave, medio enterrada.
static void DibujarPlanchaTanques(Vector3 base, float giro, float inclinacion, float e)
{
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(inclinacion, 1.0f, 0.0f, 0.0f);
    Color casco = TinteTanques(COLOR_CASCO_TANQUES, base);
    CuboTanques({ 0.0f, 0.25f * e, 0.0f }, { 1.5f * e, 0.08f, 0.9f * e }, TonoTanques(casco, 0.9f));
    CuboTanques({ 0.0f, 0.3f * e, 0.0f }, { 1.5f * e, 0.02f, 0.16f * e }, TinteTanques(COLOR_OXIDO_TANQUES, base));
    CuboTanques({ -0.55f * e, 0.3f * e, 0.3f * e }, { 0.1f, 0.02f, 0.1f }, COLOR_METAL_TANQUES);
    CuboTanques({ 0.55f * e, 0.3f * e, -0.3f * e }, { 0.1f, 0.02f, 0.1f }, COLOR_METAL_TANQUES);
    rlPopMatrix();
}


// Contenedores de suministro caidos de la nave.
static void DibujarContenedoresTanques(Vector3 base, float giro)
{
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    for (int k = 0; k < 3; k++)
    {
        Vector3 p = { (float)k * 0.55f - 0.55f, 0.0f, (k == 1) ? 0.3f : 0.0f };
        float alto = k == 2 ? 0.5f : 0.75f;
        PrismaTanques(p, 0.24f, 0.24f, alto, 8, COLOR_METAL_TANQUES);
        PrismaTanques({ p.x, alto * 0.55f, p.z }, 0.25f, 0.25f, 0.1f, 8, COLOR_OXIDO_TANQUES);
        PrismaTanques({ p.x, alto, p.z }, 0.14f, 0.1f, 0.06f, 8, COLOR_HONGO_TANQUES);
    }

    // Uno volcado.
    rlTranslatef(0.2f, 0.22f, -0.55f);
    rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    PrismaTanques({ 0.0f, -0.35f, 0.0f }, 0.22f, 0.22f, 0.7f, 8, TonoTanques(COLOR_METAL_TANQUES, 0.9f));
    PrismaTanques({ 0.0f, -0.05f, 0.0f }, 0.23f, 0.23f, 0.1f, 8, COLOR_OXIDO_TANQUES);
    rlPopMatrix();
}


// Hongos-tallo luminosos que se mecen.
static void DibujarTallosTanques(Vector3 base, float t, int semilla, float e)
{
    for (int k = 0; k < 3; k++)
    {
        float a = (float)k * 2.1f + (float)semilla;
        Vector3 pie = { base.x + std::cos(a) * 0.35f * e, base.y, base.z + std::sin(a) * 0.3f * e };
        float alto = (1.0f + 0.7f * Hash01Tanques(k, semilla + 3)) * e;
        float vaiven = std::sin(t * 1.3f + (float)k * 1.7f + (float)semilla) * 0.12f * alto;
        Vector3 cima = { pie.x + vaiven, pie.y + alto, pie.z + vaiven * 0.4f };

        // Tallo en dos tramos curvados.
        Vector3 medio = { pie.x + vaiven * 0.35f, pie.y + alto * 0.5f, pie.z + vaiven * 0.15f };
        CuboTanques({ (pie.x + medio.x) * 0.5f, (pie.y + medio.y) * 0.5f, (pie.z + medio.z) * 0.5f }, { 0.07f * e, alto * 0.5f, 0.07f * e }, Color{ 92, 70, 110, 255 });
        CuboTanques({ (medio.x + cima.x) * 0.5f, (medio.y + cima.y) * 0.5f, (medio.z + cima.z) * 0.5f }, { 0.06f * e, alto * 0.5f, 0.06f * e }, Color{ 110, 84, 126, 255 });

        // Bulbo luminoso.
        float latido = 0.85f + 0.15f * std::sin(t * 2.6f + (float)k * 1.3f + (float)semilla);
        PrismaTanques({ cima.x, cima.y - 0.08f * e, cima.z }, 0.1f * e, 0.2f * e, 0.14f * e, 6, TonoTanques(COLOR_HONGO_TANQUES, latido));
        PrismaTanques({ cima.x, cima.y + 0.06f * e, cima.z }, 0.2f * e, 0.04f * e, 0.14f * e, 6, TonoTanques(COLOR_HONGO_TANQUES, latido * 1.05f));
    }
}


// Capsula de escape clavada en la cornisa, con su luz que parpadea.
static const Vector3 CAPSULA_TANQUES = { -2.4f, 0.0f, -11.7f };

static void DibujarCapsulaTanques()
{
    Vector3 b = CAPSULA_TANQUES;
    rlPushMatrix();
    rlTranslatef(b.x, b.y - 0.2f, b.z);
    rlRotatef(-28.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(15.0f, 1.0f, 0.0f, 0.0f);
    Color casco = TinteTanques(COLOR_CASCO_TANQUES, b);
    PrismaTanques({ 0.0f, 0.0f, 0.0f }, 0.45f, 0.62f, 0.5f, 10, casco);
    PrismaTanques({ 0.0f, 0.5f, 0.0f }, 0.62f, 0.55f, 0.45f, 10, casco);
    PrismaTanques({ 0.0f, 0.95f, 0.0f }, 0.55f, 0.18f, 0.4f, 10, TonoTanques(casco, 1.05f));
    PrismaTanques({ 0.0f, 0.48f, 0.0f }, 0.64f, 0.64f, 0.08f, 10, COLOR_OXIDO_TANQUES);
    CuboTanques({ 0.0f, 0.75f, 0.5f }, { 0.36f, 0.26f, 0.12f }, Color{ 60, 140, 170, 255 });
    CuboTanques({ 0.0f, 1.38f, 0.0f }, { 0.12f, 0.08f, 0.12f }, Color{ 255, 120, 90, 255 });
    rlPopMatrix();
}


// Baliza alienigena: obelisco con runas y anillos que giran.
static const Vector3 BALIZA_TANQUES = { 19.4f, 0.0f, -10.6f };

static void DibujarBalizaTanques(float t)
{
    Vector3 b = BALIZA_TANQUES;
    Color piedra = TinteTanques(Color{ 70, 60, 92, 255 }, b);
    PrismaTanques(b, 1.0f, 0.85f, 0.4f, 6, TinteTanques(COLOR_ROCA_OSCURA_TANQUES, b));
    PrismaTanques({ b.x, 0.4f, b.z }, 0.55f, 0.18f, 4.2f, 4, piedra, 0.785f);

    // Runas que laten hacia arriba.
    for (int k = 0; k < 5; k++)
    {
        float y = 0.9f + (float)k * 0.65f;
        float ancho = 0.55f - (float)k * 0.065f;
        float latido = 0.5f + 0.5f * std::sin(t * 3.0f - (float)k * 0.9f);
        Color runa = MezclaTanques(TonoTanques(COLOR_CRISTAL_LILA_TANQUES, 0.6f), COLOR_ENERGIA_TANQUES, latido);
        CuboTanques({ b.x, y, b.z + ancho * 0.72f }, { ancho * 0.7f, 0.06f, 0.03f }, runa);
    }

    // Anillos de gajos flotando a dos alturas, en sentidos opuestos.
    for (int anillo = 0; anillo < 2; anillo++)
    {
        float y = 2.4f + (float)anillo * 1.5f + std::sin(t * 1.2f + (float)anillo) * 0.1f;
        float radio = 1.1f - (float)anillo * 0.3f;
        float giro = t * (anillo == 0 ? 0.9f : -1.3f);

        for (int g = 0; g < 6; g++)
        {
            float a = giro + 1.0472f * (float)g;
            CuboTanques({ b.x + std::cos(a) * radio, y, b.z + std::sin(a) * radio }, { 0.26f, 0.07f, 0.26f }, g % 2 == 0 ? COLOR_ENERGIA_TANQUES : COLOR_METAL_TANQUES);
        }
    }
}


// Agujas de cristal gigantes de la esquina derecha (media distancia).
static void DibujarAgujasTanques(float t)
{
    struct Aguja { Vector3 base; Vector3 eje; float radio; float largo; int color; };
    const Aguja AGUJAS[7] =
    {
        { { 22.6f, -0.4f, -11.8f }, { -0.18f, 1.0f, 0.05f }, 0.85f, 7.8f, 0 },
        { { 23.8f, -0.4f, -10.2f }, { 0.25f, 1.0f, 0.2f }, 0.6f, 5.2f, 1 },
        { { 21.4f, -0.2f, -12.6f }, { -0.45f, 1.0f, -0.1f }, 0.5f, 4.6f, 2 },
        { { 24.6f, -0.4f, -12.8f }, { 0.1f, 1.0f, -0.25f }, 0.7f, 6.4f, 1 },
        { { 21.0f, 0.0f, -9.4f }, { -0.6f, 1.0f, 0.4f }, 0.3f, 2.2f, 0 },
        { { 26.0f, 0.4f, -8.6f }, { 0.4f, 1.0f, 0.3f }, 0.45f, 3.6f, 2 },
        { { 19.6f, -0.6f, -13.4f }, { -0.3f, 1.0f, -0.4f }, 0.42f, 3.4f, 1 }
    };

    for (int k = 0; k < 7; k++)
    {
        const Aguja& a = AGUJAS[k];
        Color base = a.color == 0 ? COLOR_CRISTAL_ROSA_TANQUES : (a.color == 1 ? COLOR_CRISTAL_LILA_TANQUES : Color{ 132, 196, 250, 255 });
        float latido = 0.92f + 0.08f * std::sin(t * 1.4f + (float)k * 1.1f);
        CristalTanques(a.base, a.eje, a.radio, a.largo, a.radio * 1.6f, TinteTanques(TonoTanques(base, latido), a.base), (float)k);
    }

    PrismaTanques({ 23.0f, -0.3f, -11.4f }, 2.6f, 1.8f, 0.7f, 7, TinteTanques(COLOR_ROCA_OSCURA_TANQUES, { 23.0f, 0.0f, -11.4f }));
}


// Nave estrellada: fuselaje con la proa sobre el abismo, ala rota,
// costillas al aire y el motor de cola que todavia arde.
static void DibujarNaveTanques()
{
    Vector3 c = NAVE_CENTRO_TANQUES;
    Color casco = TinteTanques(COLOR_CASCO_TANQUES, c);
    Color oscuro = TinteTanques(COLOR_METAL_TANQUES, c);
    Color oxido = TinteTanques(COLOR_OXIDO_TANQUES, c);

    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(NAVE_GIRO_TANQUES, 0.0f, 1.0f, 0.0f);
    rlRotatef(-12.0f, 0.0f, 0.0f, 1.0f);

    // Fuselaje (local +X = proa).
    CuboTanques({ -1.6f, 0.8f, 0.0f }, { 2.6f, 1.5f, 2.0f }, casco);
    CuboTanques({ 1.0f, 0.7f, 0.0f }, { 2.0f, 1.3f, 1.8f }, TonoTanques(casco, 0.95f));
    CuboTanques({ 2.6f, 0.55f, 0.0f }, { 1.4f, 0.95f, 1.3f }, casco);
    CuboTanques({ 3.6f, 0.45f, 0.0f }, { 0.8f, 0.6f, 0.8f }, TonoTanques(casco, 1.05f));
    CuboTanques({ 2.4f, 1.1f, 0.0f }, { 0.9f, 0.3f, 0.9f }, Color{ 48, 108, 138, 255 });   // cabina
    CuboTanques({ -1.6f, 0.82f, 0.0f }, { 2.62f, 0.2f, 2.02f }, oxido);                   // franja

    // Costillas a la vista donde se arranco el casco.
    for (int k = 0; k < 4; k++)
    {
        float x = 0.3f + (float)k * 0.42f;
        CuboTanques({ x, 1.55f, 0.0f }, { 0.1f, 0.12f, 1.8f }, oscuro);
        CuboTanques({ x, 1.3f, 0.86f }, { 0.1f, 0.5f, 0.1f }, oscuro);
    }

    // Motor de cola (tobera hacia -X).
    rlPushMatrix();
    rlTranslatef(-3.1f, 0.9f, 0.0f);
    rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    PrismaTanques({ 0.0f, -0.2f, 0.0f }, 0.75f, 0.7f, 0.6f, 10, oscuro);
    PrismaTanques({ 0.0f, 0.4f, 0.0f }, 0.62f, 0.8f, 0.3f, 10, TonoTanques(oscuro, 0.8f));
    PrismaTanques({ 0.0f, 0.62f, 0.0f }, 0.45f, 0.45f, 0.1f, 10, Color{ 255, 140, 60, 255 });
    rlPopMatrix();

    // Ala izquierda entera (hacia la camara) y alerón.
    CuboTanques({ -0.8f, 0.5f, 1.9f }, { 2.4f, 0.12f, 1.9f }, TonoTanques(casco, 0.92f));
    CuboTanques({ -1.5f, 0.52f, 2.6f }, { 0.9f, 0.04f, 0.5f }, oxido);
    CuboTanques({ -2.6f, 1.9f, 0.0f }, { 0.9f, 1.3f, 0.1f }, casco);

    // Ala derecha arrancada, clavada de canto.
    rlTranslatef(-0.4f, 0.6f, -1.9f);
    rlRotatef(58.0f, 1.0f, 0.0f, 0.0f);
    CuboTanques({ 0.0f, 0.0f, -0.8f }, { 2.0f, 0.12f, 1.7f }, TonoTanques(casco, 0.85f));
    CuboTanques({ 0.0f, 0.04f, -1.55f }, { 1.9f, 0.04f, 0.2f }, oxido);
    rlPopMatrix();
}


static void DibujarUtileriaTanques(float t, int paso)
{
    // Cornisa del fondo (entre la arena y el precipicio).
    DibujarRacimoTanques({ -12.4f, 0.0f, -11.2f }, 1.2f, COLOR_CRISTAL_ROSA_TANQUES, 1);
    DibujarRacimoTanques({ 3.4f, 0.0f, -11.9f }, 0.9f, COLOR_CRISTAL_LILA_TANQUES, 2);
    DibujarRacimoTanques({ 11.6f, 0.0f, -11.1f }, 1.1f, COLOR_CRISTAL_ROSA_TANQUES, 3);
    DibujarRocaTanques({ -8.2f, 0.0f, -11.4f }, 1.1f, 0.4f, COLOR_ROCA_TANQUES);
    DibujarRocaTanques({ 7.6f, 0.0f, -11.8f }, 0.9f, 1.4f, COLOR_ROCA_TANQUES);
    DibujarCapsulaTanques();
    DibujarContenedoresTanques({ 0.8f, 0.0f, -11.0f }, 20.0f);
    DibujarPlanchaTanques({ -15.0f, 0.0f, -11.3f }, 30.0f, 18.0f, 1.0f);
    DibujarTallosTanques({ -5.6f, 0.0f, -11.0f }, t, 1, 1.0f);
    DibujarTallosTanques({ 14.4f, 0.0f, -11.4f }, t, 2, 1.2f);

    if (paso == 1)
    {
        DibujarRacimoTanques({ -16.8f, 0.0f, -12.2f }, 0.8f, COLOR_CRISTAL_LILA_TANQUES, 4);
        DibujarRocaTanques({ 15.8f, 0.0f, -12.4f }, 0.6f, 2.0f, COLOR_ROCA_OSCURA_TANQUES);
        DibujarPlanchaTanques({ 9.4f, 0.0f, -10.6f }, -40.0f, -12.0f, 0.8f);
        DibujarRocaTanques({ -10.0f, 0.0f, -12.4f }, 0.5f, 3.0f, COLOR_ROCA_OSCURA_TANQUES);
    }

    // Cornisas laterales.
    DibujarRacimoTanques({ -18.6f, 0.0f, 1.6f }, 1.0f, COLOR_CRISTAL_LILA_TANQUES, 5);
    DibujarRocaTanques({ -19.4f, 0.0f, -3.0f }, 1.3f, 0.9f, COLOR_ROCA_TANQUES);
    DibujarTallosTanques({ -18.8f, 0.0f, -6.6f }, t, 3, 1.1f);
    DibujarContenedoresTanques({ -19.0f, 0.0f, 5.8f }, -60.0f);
    DibujarRacimoTanques({ 18.8f, 0.0f, -4.2f }, 1.2f, COLOR_CRISTAL_ROSA_TANQUES, 6);
    DibujarRocaTanques({ 19.6f, 0.0f, 1.4f }, 1.2f, 2.2f, COLOR_ROCA_TANQUES);
    DibujarTallosTanques({ 18.6f, 0.0f, -7.4f }, t, 4, 1.0f);
    DibujarPlanchaTanques({ 19.2f, 0.0f, 5.0f }, 70.0f, 14.0f, 1.1f);

    if (paso == 1)
    {
        DibujarRocaTanques({ -21.5f, 0.3f, -0.4f }, 0.8f, 0.2f, COLOR_ROCA_OSCURA_TANQUES);
        DibujarRacimoTanques({ 21.4f, 0.3f, 2.8f }, 0.9f, COLOR_CRISTAL_LILA_TANQUES, 7);
        DibujarTallosTanques({ -21.0f, 0.3f, 3.6f }, t, 5, 0.9f);
    }

    // Frente: solo piedras y cristales bajos (no tapan la arena).
    for (int k = 0; k < 9; k += paso)
    {
        float x = -15.0f + (float)k * 3.7f + Hash01Tanques(k, 90) * 1.2f;
        float z = 10.5f + Hash01Tanques(k, 91) * 0.9f;

        if (k % 3 == 1)
        {
            DibujarRacimoTanques({ x, 0.0f, z }, 0.35f, k % 2 == 0 ? COLOR_CRISTAL_ROSA_TANQUES : COLOR_CRISTAL_LILA_TANQUES, 10 + k);
        }
        else
        {
            DibujarRocaTanques({ x, 0.0f, z }, 0.32f + 0.12f * Hash01Tanques(k, 92), (float)k, COLOR_ROCA_TANQUES);
        }
    }

    DibujarBalizaTanques(t);
    DibujarAgujasTanques(t);
    DibujarNaveTanques();
}


//==================================================
// TANQUES, PILOTOS Y PLASMA
//==================================================

// Posicion dibujada del tanque (con la sacudida del golpe).
static Vector3 BaseDibujoTanque(const MinijuegoTanquesPlasma& m, int i)
{
    const EstadoTanquePlasma& t = m.tanques[i];
    float s = t.sacudida / 0.35f;
    return {
        t.posicion.x + std::sin(m.tiempoAnimacion * 70.0f) * 0.09f * s,
        0.0f,
        t.posicion.y + std::cos(m.tiempoAnimacion * 61.0f) * 0.06f * s
    };
}


// Casco y orugas en coordenadas locales (+X = frente).
static void CascoTanque(Color cuerpo, Color oscuro, float giroOrugas, int vidas, float t, bool destruido)
{
    Color oruga = destruido ? Color{ 34, 30, 38, 255 } : Color{ 46, 42, 58, 255 };
    Color taco = destruido ? Color{ 44, 40, 48, 255 } : Color{ 82, 78, 96, 255 };

    for (int s = -1; s <= 1; s += 2)
    {
        float z = 0.56f * (float)s;
        CuboTanques({ 0.0f, 0.22f, z }, { 1.78f, 0.4f, 0.36f }, oruga);

        // Tacos de la oruga que corren hacia atras al avanzar.
        float corrido = std::fmod(giroOrugas * 1.4f, 0.28f);

        for (int k = 0; k < 6; k++)
        {
            float x = 0.78f - ((float)k * 0.28f + corrido);
            if (x < -0.84f) x += 1.68f;
            CuboTanques({ x, 0.43f, z }, { 0.09f, 0.04f, 0.38f }, taco);
        }

        // Ruedas (cubos con buje) a la vista en el costado.
        for (int k = 0; k < 4; k++)
        {
            float x = -0.6f + (float)k * 0.4f;
            CuboTanques({ x, 0.2f, z + 0.19f * (float)s }, { 0.22f, 0.22f, 0.03f }, Color{ 70, 68, 84, 255 });
        }

        CuboTanques({ 0.0f, 0.46f, z }, { 1.84f, 0.05f, 0.44f }, oscuro);
    }

    // Cuerpo, glacis frontal y motor trasero.
    CuboTanques({ -0.05f, 0.56f, 0.0f }, { 1.5f, 0.32f, 0.82f }, cuerpo);
    CuboTanques({ 0.78f, 0.5f, 0.0f }, { 0.24f, 0.2f, 0.78f }, MezclaTanques(cuerpo, WHITE, destruido ? 0.0f : 0.25f));
    CuboTanques({ -0.74f, 0.62f, 0.0f }, { 0.28f, 0.26f, 0.7f }, Color{ 70, 72, 90, 255 });

    if (destruido)
    {
        return;
    }

    // Faros y las dos celdas de plasma de la cubierta (= vidas).
    CuboTanques({ 0.91f, 0.52f, 0.26f }, { 0.03f, 0.07f, 0.12f }, Color{ 230, 250, 255, 255 });
    CuboTanques({ 0.91f, 0.52f, -0.26f }, { 0.03f, 0.07f, 0.12f }, Color{ 230, 250, 255, 255 });

    for (int c = 0; c < 2; c++)
    {
        bool viva = c < vidas;
        float latido = 0.85f + 0.15f * std::sin(t * 5.0f + (float)c);
        Color celda = viva ? TonoTanques(Color{ 150, 250, 255, 255 }, latido) : Color{ 40, 38, 52, 255 };
        CuboTanques({ -0.46f, 0.76f, c == 0 ? 0.22f : -0.22f }, { 0.24f, 0.1f, 0.22f }, celda);
    }
}


// Torreta con el canon de plasma y el piloto asomado por la escotilla.
static void TorretaTanque(Color cuerpo, Color claro, Color colorJugador, float retroceso, float t, bool conPiloto)
{
    Color metal = Color{ 196, 200, 216, 255 };
    float r = retroceso * 0.22f;

    PrismaTanques({ 0.0f, 0.72f, 0.0f }, 0.5f, 0.47f, 0.08f, 10, Color{ 70, 72, 90, 255 });
    PrismaTanques({ 0.0f, 0.8f, 0.0f }, 0.47f, 0.36f, 0.3f, 8, claro, 0.39f);

    // Canon: tubo, tres bobinas del color del jugador y la boca.
    CuboTanques({ 0.78f - r, 0.95f, 0.0f }, { 1.0f, 0.14f, 0.14f }, metal);

    for (int k = 0; k < 3; k++)
    {
        float x = 0.5f + (float)k * 0.26f - r;
        float latido = 0.8f + 0.2f * std::sin(t * 8.0f - (float)k * 1.2f);
        CuboTanques({ x, 0.95f, 0.0f }, { 0.07f, 0.22f, 0.22f }, TonoTanques(colorJugador, latido));
    }

    CuboTanques({ 1.3f - r, 0.95f, 0.0f }, { 0.1f, 0.24f, 0.24f }, cuerpo);
    CuboTanques({ 1.36f - r, 0.95f, 0.0f }, { 0.03f, 0.12f, 0.12f }, MezclaTanques(colorJugador, WHITE, 0.5f));

    // Antena con punta luminosa.
    CuboTanques({ -0.3f, 1.3f, 0.26f }, { 0.03f, 0.7f, 0.03f }, Color{ 70, 72, 90, 255 });
    CuboTanques({ -0.3f, 1.66f, 0.26f }, { 0.07f, 0.07f, 0.07f }, MezclaTanques(colorJugador, WHITE, 0.3f));

    // Escotilla y piloto.
    PrismaTanques({ -0.1f, 1.1f, 0.0f }, 0.22f, 0.22f, 0.03f, 8, Color{ 40, 38, 52, 255 });

    if (conPiloto)
    {
        DibujarModeloJugadorEnPosicion({ -0.1f, 0.78f, 0.0f }, 90.0f, colorJugador, ESCALA_MODELO_JUGADOR_3D * 0.42f);
    }
}


static void DibujarTanqueTanques(const MinijuegoTanquesPlasma& m, int i, const Participante& participante)
{
    const EstadoTanquePlasma& t = m.tanques[i];
    Vector3 base = BaseDibujoTanque(m, i);
    float tiempo = m.tiempoAnimacion;

    if (t.eliminado)
    {
        // Carcasa chamuscada, ladeada.
        Color quemado = MezclaTanques(participante.color, Color{ 30, 26, 34, 255 }, 0.78f);
        rlPushMatrix();
        rlTranslatef(base.x, -0.06f, base.z);
        rlRotatef(-t.anguloCasco * RAD2DEG, 0.0f, 1.0f, 0.0f);
        rlRotatef(7.0f, 1.0f, 0.0f, 0.0f);
        CascoTanque(quemado, Color{ 26, 24, 30, 255 }, t.giroOrugas, 0, tiempo, true);
        rlPopMatrix();

        // La torreta salta por el aire y cae de costado junto al casco.
        float u = Limitar01Tanques(t.tiempoMuerto / 0.65f);
        float anguloTorreta = std::atan2(t.direccion.y, t.direccion.x);
        Vector2 caida = { -std::sin(anguloTorreta) * 1.0f - std::cos(anguloTorreta) * 0.6f, std::cos(anguloTorreta) * 1.0f - std::sin(anguloTorreta) * 0.6f };
        Vector3 p = { base.x + caida.x * u, std::sin(u * 3.1416f) * 2.2f - 0.55f * u, base.z + caida.y * u };

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(-anguloTorreta * RAD2DEG + 90.0f * u, 0.0f, 1.0f, 0.0f);
        rlRotatef(70.0f * u, 1.0f, 0.0f, 0.0f);
        TorretaTanque(quemado, TonoTanques(quemado, 1.2f), Color{ 60, 56, 64, 255 }, 0.0f, tiempo, false);
        rlPopMatrix();

        // El piloto sale despedido y queda parado al lado.
        Vector2 descanso = DescansoPilotoTanques(t);
        float e = Limitar01Tanques(t.tiempoMuerto / TIEMPO_EYECCION_TANQUES);
        Vector3 pies = {
            t.posicion.x + (descanso.x - t.posicion.x) * e,
            0.9f * (1.0f - e) + std::sin(e * 3.1416f) * 2.6f,
            t.posicion.y + (descanso.y - t.posicion.y) * e
        };
        float vuelta = e < 1.0f ? 540.0f * e : 0.0f;
        DibujarSombraRetroCircular({ pies.x, pies.y, pies.z }, 0.35f, 0.35f, 0.02f, 0.3f, 10);
        DibujarModeloJugadorEnPosicion(pies, vuelta, participante.color, ESCALA_MODELO_JUGADOR_3D * 0.62f);
        return;
    }

    Color color = participante.color;

    if (t.invulnerabilidad > 0.0f && ((int)(tiempo * 18.0f) % 2 == 0))
    {
        color = MezclaTanques(color, WHITE, 0.55f);
    }

    color = MezclaTanques(color, WHITE, t.destelloGolpe * 0.6f);
    Color claro = MezclaTanques(color, WHITE, 0.3f);
    Color oscuro = TonoTanques(color, 0.55f);
    float anguloTorreta = std::atan2(t.direccion.y, t.direccion.x);

    DibujarSombraRetroCircular({ base.x, 0.6f, base.z }, 1.05f, 0.95f, 0.02f, 0.34f, 12);

    // Leve cabeceo al andar.
    float cabeceo = std::sin(t.giroOrugas * 9.0f) * 1.2f * t.marcha;

    rlPushMatrix();
    rlTranslatef(base.x, 0.0f, base.z);
    rlRotatef(-t.anguloCasco * RAD2DEG, 0.0f, 1.0f, 0.0f);
    rlRotatef(cabeceo, 0.0f, 0.0f, 1.0f);
    CascoTanque(color, oscuro, t.giroOrugas, t.vidas, tiempo, false);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(base.x, 0.0f, base.z);
    rlRotatef(-anguloTorreta * RAD2DEG, 0.0f, 1.0f, 0.0f);
    TorretaTanque(color, claro, participante.color, t.retroceso, tiempo, true);
    rlPopMatrix();
}


// Bola de plasma: nucleo blanco (el halo y la estela van en la pasada aditiva).
static void DibujarProyectilesTanques(const MinijuegoTanquesPlasma& m, const Participante participantes[])
{
    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        const ProyectilTanquePlasma& p = m.proyectiles[i];

        if (!p.activo)
        {
            continue;
        }

        Color color = p.propietario >= 0 ? participantes[p.propietario].color : WHITE;
        Vector3 c = AMundoTanques(p.posicion, ALTURA_DISPARO);
        DrawSphereEx(c, RADIO_PROYECTIL, 6, 8, MezclaTanques(color, WHITE, 0.35f));
        DrawSphereEx(c, RADIO_PROYECTIL * 0.62f, 5, 6, WHITE);
    }
}


static void DibujarParticulasTanques(const MinijuegoTanquesPlasma& m)
{
    for (int i = 0; i < MAX_PARTICULAS_TANQUES_PLASMA; i++)
    {
        const ParticulaTanquesPlasma& p = m.particulas[i];

        if (!p.activa || p.brillo || p.humo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        float s = p.tamano * (0.4f + 0.6f * vida);
        CuboTanques(p.posicion, { s, s, s }, p.color);
    }
}


// Alfa normal: anillos de jugador, contorno de las coberturas,
// quemaduras de los tanques destruidos y humo.
static void DibujarTranslucidosTanques(const MinijuegoTanquesPlasma& m, const Participante participantes[])
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int k = 0; k < CANTIDAD_OBSTACULOS_TANQUES; k++)
    {
        AnilloTanques({ centros[k].x, 0.055f, centros[k].y }, radios[k] + 0.02f, radios[k] + 0.14f, ConAlfaTanques(COLOR_CRISTAL_TANQUES, 120.0f), 32);
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoTanquePlasma& t = m.tanques[i];
        Vector3 base = BaseDibujoTanque(m, i);

        if (t.eliminado)
        {
            ResplandorSueloTanques({ t.posicion.x, 0.06f, t.posicion.y }, 1.7f, Color{ 16, 12, 20, 200 });
            Vector2 d = DescansoPilotoTanques(t);

            if (t.tiempoMuerto > TIEMPO_EYECCION_TANQUES)
            {
                AnilloTanques({ d.x, 0.06f, d.y }, 0.42f, 0.52f, ConAlfaTanques(participantes[i].color, 170.0f), 24);
            }

            continue;
        }

        AnilloTanques({ base.x, 0.06f, base.z }, 1.0f, 1.14f, ConAlfaTanques(participantes[i].color, 200.0f), 32);
    }

    for (int i = 0; i < MAX_PARTICULAS_TANQUES_PLASMA; i++)
    {
        const ParticulaTanquesPlasma& p = m.particulas[i];

        if (!p.activa || !p.humo)
        {
            continue;
        }

        float u = 1.0f - p.vida / p.vidaMaxima;
        float alfa = 150.0f * std::sin(u * 3.1416f);
        ResplandorFrenteTanques(p.posicion, p.tamano * (1.0f + u * 2.2f), ConAlfaTanques(p.color, alfa));
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// LUCES FALSAS (pasada aditiva)
//==================================================

static void DibujarLucesTanques(const MinijuegoTanquesPlasma& m, const Participante participantes[], bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Resplandor que sube del fondo del abismo.
    if (decoracion)
    {
        float latido = 0.85f + 0.15f * std::sin(t * 0.7f);
        rlBegin(RL_QUADS);
        Color fondo = ConAlfaTanques(Color{ 90, 150, 220, 255 }, 46.0f * latido);
        Color cerca = ConAlfaTanques(Color{ 90, 200, 230, 255 }, 0.0f);
        VerticeTanques({ -70.0f, -9.0f, PARED_Z_TANQUES + 2.0f }, fondo);
        VerticeTanques({ 70.0f, -9.0f, PARED_Z_TANQUES + 2.0f }, fondo);
        VerticeTanques({ 70.0f, -4.0f, LABIO_Z_TANQUES - 2.0f }, cerca);
        VerticeTanques({ -70.0f, -4.0f, LABIO_Z_TANQUES - 2.0f }, cerca);
        rlEnd();

        // Cristales del otro lado que destellan.
        for (int k = 0; k < 14; k++)
        {
            float x = -60.0f + 120.0f * Hash01Tanques(k, 120);
            Vector3 p = { x, AlturaParedLejanaTanques(x) - 1.5f - 3.0f * Hash01Tanques(k, 121), PARED_Z_TANQUES + 0.6f };
            float brillo = 0.5f + 0.5f * std::sin(t * (0.8f + Hash01Tanques(k, 122)) + (float)k);
            Color c = k % 2 == 0 ? Color{ 230, 120, 220, 255 } : Color{ 110, 220, 250, 255 };
            ResplandorFrenteTanques(p, 0.8f + 0.6f * brillo, ConAlfaTanques(c, 90.0f * brillo));
        }

        // Rocas flotantes: su cristal brilla.
        for (int k = 0; k < 5; k++)
        {
            Vector3 p = PosicionRocaFlotanteTanques(k, t);
            p.y += 0.9f;
            ResplandorFrenteTanques(p, 1.1f, ConAlfaTanques(k % 2 == 0 ? COLOR_CRISTAL_ROSA_TANQUES : COLOR_CRISTAL_LILA_TANQUES, 60.0f));
        }
    }

    // Racimos de cobertura: halo, charco de luz cian y destello al golpe.
    Vector2 centros[CANTIDAD_OBSTACULOS_TANQUES]{};
    float radios[CANTIDAD_OBSTACULOS_TANQUES]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int k = 0; k < CANTIDAD_OBSTACULOS_TANQUES; k++)
    {
        float e = radios[k] / 2.0f;
        float latido = 0.8f + 0.2f * std::sin(t * 2.2f + (float)k * 2.0f);
        float golpe = m.destelloCristal[k] / 0.35f;
        Vector3 c = { centros[k].x, 0.07f, centros[k].y };

        ResplandorSueloTanques(c, radios[k] + 2.6f, ConAlfaTanques(COLOR_CRISTAL_TANQUES, (36.0f + 60.0f * golpe) * latido));
        ResplandorSueloTanques(c, radios[k] + 1.0f, ConAlfaTanques(COLOR_CRISTAL_TANQUES, (30.0f + 50.0f * golpe) * latido));
        ResplandorFrenteTanques({ c.x, 2.0f * e + 0.6f, c.z }, 2.2f * e, ConAlfaTanques(COLOR_CRISTAL_TANQUES, (40.0f + 120.0f * golpe) * latido));
    }

    // Emisores de las esquinas.
    for (int k = 0; k < 4; k++)
    {
        Vector3 orbe = OrbeEmisorTanques(k, t);
        float latido = 0.8f + 0.2f * std::sin(t * 3.0f + (float)k);
        ResplandorFrenteTanques(orbe, 0.9f, ConAlfaTanques(COLOR_ENERGIA_TANQUES, 120.0f * latido));
        ResplandorFrenteTanques(orbe, 0.4f, ConAlfaTanques(WHITE, 140.0f * latido));
        ResplandorSueloTanques({ orbe.x, 0.6f, orbe.z }, 2.2f, ConAlfaTanques(COLOR_ENERGIA_TANQUES, 34.0f * latido));
    }

    if (decoracion)
    {
        // Baliza: haz que sube al cielo y charco a sus pies.
        Vector3 b = BALIZA_TANQUES;
        float latido = 0.75f + 0.25f * std::sin(t * 2.0f);
        DrawCylinderEx({ b.x, 4.6f, b.z }, { b.x, 24.0f, b.z }, 0.35f, 1.2f, 10, ConAlfaTanques(COLOR_ENERGIA_TANQUES, 26.0f * latido));
        DrawCylinderEx({ b.x, 4.6f, b.z }, { b.x, 24.0f, b.z }, 0.12f, 0.3f, 8, ConAlfaTanques(WHITE, 40.0f * latido));
        ResplandorFrenteTanques({ b.x, 4.7f, b.z }, 1.2f, ConAlfaTanques(COLOR_ENERGIA_TANQUES, 150.0f * latido));
        ResplandorSueloTanques({ b.x, 0.08f, b.z }, 3.0f, ConAlfaTanques(COLOR_ENERGIA_TANQUES, 40.0f * latido));

        // Agujas gigantes: brillo interior.
        ResplandorFrenteTanques({ 22.8f, 4.0f, -11.4f }, 4.5f, ConAlfaTanques(COLOR_CRISTAL_ROSA_TANQUES, 34.0f));
        ResplandorSueloTanques({ 22.0f, 0.1f, -10.6f }, 5.0f, ConAlfaTanques(COLOR_CRISTAL_ROSA_TANQUES, 30.0f));

        // Motor de la nave: fuego que titila y su charco naranja.
        float titila = 0.75f + 0.25f * std::sin(t * 13.0f) * std::sin(t * 7.3f + 1.0f);
        Vector3 fuego = { NAVE_HUMO_TANQUES.x + 0.3f, NAVE_HUMO_TANQUES.y - 0.7f, NAVE_HUMO_TANQUES.z - 0.2f };
        ResplandorFrenteTanques(fuego, 1.4f * titila, ConAlfaTanques(COLOR_FUEGO_TANQUES, 170.0f));
        ResplandorFrenteTanques({ fuego.x, fuego.y + 0.4f, fuego.z }, 0.7f, ConAlfaTanques(Color{ 255, 230, 160, 255 }, 160.0f * titila));
        ResplandorSueloTanques({ fuego.x + 0.6f, 0.1f, fuego.z }, 4.0f, ConAlfaTanques(COLOR_FUEGO_TANQUES, 46.0f * titila));

        // Capsula: baliza roja que parpadea.
        float parpadeo = std::fmod(t, 1.2f) < 0.25f ? 1.0f : 0.15f;
        Vector3 cap = { CAPSULA_TANQUES.x + 0.62f, 1.05f, CAPSULA_TANQUES.z + 0.3f };
        ResplandorFrenteTanques(cap, 0.7f, ConAlfaTanques(Color{ 255, 90, 70, 255 }, 200.0f * parpadeo));
        ResplandorSueloTanques({ cap.x, 0.08f, cap.z }, 1.6f, ConAlfaTanques(Color{ 255, 90, 70, 255 }, 50.0f * parpadeo));

        // Bulbos de los tallos.
        const Vector3 TALLOS[4] = { { -5.6f, 1.4f, -11.0f }, { 14.4f, 1.6f, -11.4f }, { -18.8f, 1.5f, -6.6f }, { 18.6f, 1.4f, -7.4f } };

        for (int k = 0; k < 4; k++)
        {
            float latido2 = 0.8f + 0.2f * std::sin(t * 2.6f + (float)k);
            ResplandorFrenteTanques(TALLOS[k], 1.0f, ConAlfaTanques(COLOR_HONGO_TANQUES, 70.0f * latido2));
            ResplandorSueloTanques({ TALLOS[k].x, 0.08f, TALLOS[k].z }, 1.8f, ConAlfaTanques(COLOR_HONGO_TANQUES, 34.0f * latido2));
        }

        // Esporas luminosas que suben y derivan.
        int esporas = (int)(46.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < esporas; k++)
        {
            float vel = 0.25f + 0.3f * Hash01Tanques(k, 130);
            float u = std::fmod(t * vel * 0.18f + Hash01Tanques(k, 131), 1.0f);
            Vector3 p =
            {
                -24.0f + 48.0f * Hash01Tanques(k, 132) + std::sin(t * 0.6f + (float)k) * 0.6f,
                0.3f + u * 5.5f,
                -14.0f + 25.0f * Hash01Tanques(k, 133) + std::cos(t * 0.5f + (float)k) * 0.4f
            };
            float alfa = 150.0f * std::sin(u * 3.1416f);
            Color c = k % 3 == 0 ? COLOR_HONGO_TANQUES : (k % 3 == 1 ? COLOR_CRISTAL_LILA_TANQUES : Color{ 160, 240, 250, 255 });
            ResplandorFrenteTanques(p, 0.09f + 0.06f * Hash01Tanques(k, 134), ConAlfaTanques(c, alfa));
        }
    }

    // Tanques: fogonazo, escudo mientras son invulnerables y fuego de los
    // destruidos.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoTanquePlasma& tq = m.tanques[i];
        Color color = participantes[i].color;
        Vector3 base = BaseDibujoTanque(m, i);

        if (tq.eliminado)
        {
            float apaga = 1.0f - Limitar01Tanques(tq.tiempoMuerto / 8.0f);
            float titila = 0.7f + 0.3f * std::sin(t * 17.0f + (float)i) * std::sin(t * 9.0f);

            if (apaga > 0.0f)
            {
                ResplandorFrenteTanques({ base.x, 0.9f, base.z }, 1.2f * titila * apaga + 0.3f, ConAlfaTanques(COLOR_FUEGO_TANQUES, 150.0f * apaga));
                ResplandorSueloTanques({ base.x, 0.08f, base.z }, 2.6f, ConAlfaTanques(COLOR_FUEGO_TANQUES, 50.0f * apaga * titila));
            }

            // Onda de la explosion.
            if (tq.tiempoMuerto < 0.6f)
            {
                float u = tq.tiempoMuerto / 0.6f;
                AnilloTanques({ base.x, 0.1f, base.z }, 0.5f + u * 4.0f, 0.9f + u * 4.4f, ConAlfaTanques(Color{ 255, 190, 120, 255 }, 200.0f * (1.0f - u)), 32);
                ResplandorFrenteTanques({ base.x, 1.0f, base.z }, 2.5f + u * 2.0f, ConAlfaTanques(Color{ 255, 220, 160, 255 }, 230.0f * (1.0f - u)));
            }

            continue;
        }

        // Luz de faros sobre la roca.
        float a = tq.anguloCasco;
        Vector3 frente = { base.x + std::cos(a) * 1.7f, 0.08f, base.z + std::sin(a) * 1.7f };
        ResplandorSueloTanques(frente, 1.0f, ConAlfaTanques(Color{ 200, 240, 255, 255 }, 26.0f));

        if (tq.fogonazo > 0.0f)
        {
            Vector3 boca = { base.x + tq.direccion.x * 1.45f, ALTURA_DISPARO, base.z + tq.direccion.y * 1.45f };
            ResplandorFrenteTanques(boca, 0.9f * tq.fogonazo + 0.2f, ConAlfaTanques(color, 230.0f * tq.fogonazo));
            ResplandorFrenteTanques(boca, 0.45f * tq.fogonazo, ConAlfaTanques(WHITE, 230.0f * tq.fogonazo));
            ResplandorSueloTanques({ boca.x, 0.08f, boca.z }, 1.8f, ConAlfaTanques(color, 80.0f * tq.fogonazo));
        }

        if (tq.invulnerabilidad > 0.0f)
        {
            float u = tq.invulnerabilidad / 0.68f;
            AnilloTanques({ base.x, 0.5f, base.z }, 1.1f, 1.22f, ConAlfaTanques(WHITE, 120.0f * u), 32);
            ResplandorFrenteTanques({ base.x, 0.8f, base.z }, 1.6f, ConAlfaTanques(color, 70.0f * u));
        }
    }

    // Plasma en vuelo: halo, estela y luz sobre la roca.
    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        const ProyectilTanquePlasma& p = m.proyectiles[i];

        if (!p.activo)
        {
            continue;
        }

        Color color = p.propietario >= 0 ? participantes[p.propietario].color : WHITE;
        Vector2 d = Vector2Normalize(p.velocidad);
        Vector3 c = AMundoTanques(p.posicion, ALTURA_DISPARO);

        for (int k = 1; k <= 5; k++)
        {
            Vector2 q = Vector2Subtract(p.posicion, Vector2Scale(d, (float)k * 0.32f));
            ResplandorFrenteTanques(AMundoTanques(q, ALTURA_DISPARO), 0.42f - (float)k * 0.05f, ConAlfaTanques(color, 170.0f - (float)k * 28.0f));
        }

        ResplandorFrenteTanques(c, 0.95f, ConAlfaTanques(color, 200.0f));
        ResplandorFrenteTanques(c, 0.45f, ConAlfaTanques(WHITE, 170.0f));
        ResplandorSueloTanques({ c.x, 0.08f, c.z }, 1.5f, ConAlfaTanques(color, 70.0f));
    }

    // Impactos: onda en la roca y destello.
    for (int i = 0; i < MAX_EFECTOS_TANQUES_PLASMA; i++)
    {
        const EfectoImpactoTanque& e = m.efectos[i];

        if (!e.activo || e.tamano > 2.0f)
        {
            continue;
        }

        float u = Limitar01Tanques(e.tiempo / 0.5f);
        Color color = e.propietario >= 0 && e.propietario < MAX_PARTICIPANTES ? participantes[e.propietario].color : WHITE;
        Vector3 c = { e.posicion.x, 0.09f, e.posicion.y };
        float r = e.tamano * (0.4f + u * 1.6f);
        AnilloTanques(c, r, r + 0.18f * e.tamano, ConAlfaTanques(color, 220.0f * (1.0f - u)), 28);
        ResplandorFrenteTanques({ c.x, ALTURA_DISPARO, c.z }, e.tamano * (0.6f + u), ConAlfaTanques(MezclaTanques(color, WHITE, 0.4f), 230.0f * (1.0f - u)));
    }

    // Particulas luminosas (chispas, fuego, fogonazos).
    for (int i = 0; i < MAX_PARTICULAS_TANQUES_PLASMA; i++)
    {
        const ParticulaTanquesPlasma& p = m.particulas[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrenteTanques(p.posicion, p.tamano * (1.2f + 1.4f * vida), ConAlfaTanques(p.color, 230.0f * vida));
    }

    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// ESCENA COMPLETA, CAMARA Y DIBUJAR
//==================================================

static void DibujarEscenarioTanques(const MinijuegoTanquesPlasma& m)
{
    float t = m.tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Horizonte: dos profundidades de mesetas y el otro lado del canon.
    if (decoracion)
    {
        DibujarMesetasTanques(-118.0f, -150.0f, 150.0f, -72.0f, -40.0f, 14.0f, Color{ 126, 88, 152, 255 }, 140, paso);
        DibujarMesetasTanques(-66.0f, -95.0f, 95.0f, -40.0f, -19.0f, 9.0f, Color{ 82, 56, 110, 255 }, 150, paso);
        DibujarAnilloNaveLejanaTanques();
    }

    DibujarParedLejanaTanques(decoracion ? paso : 2);

    if (decoracion)
    {
        DibujarRocasFlotantesTanques(t);
    }

    // Cornisa, precipicio y paredes laterales.
    DibujarCornisaTanques(paso);

    if (decoracion)
    {
        DibujarParedesLateralesTanques(paso);
    }

    // Arena: losas, bordillo, emisores y coberturas (siempre).
    DibujarSueloArenaTanques(decoracion);
    DibujarBordilloTanques();
    DibujarEmisoresTanques(t);
    DibujarCoberturasTanques(m, t);

    if (decoracion)
    {
        DibujarUtileriaTanques(t, paso);
    }
}


static Camera3D CamaraTanques(const MinijuegoTanquesPlasma& m)
{
    Camera3D camara{};
    camara.position = OJO_TANQUES;
    camara.target = MIRA_TANQUES;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = FOV_TANQUES;
    camara.projection = CAMERA_PERSPECTIVE;

    // Entrada: durante la cuenta la camara baja desde lo alto del canon.
    if (m.fase == FASE_TANQUES_PREPARACION)
    {
        float u = Limitar01Tanques(m.tiempoPreparacion / DURACION_PREPARACION_TANQUES);
        float s = SuaveTanques(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        camara.position.x -= 5.0f * s;
        camara.position.y += 7.0f * s;
        camara.position.z += 6.0f * s;
        camara.target.z -= 2.0f * s;
    }

    return camara;
}


void MinijuegoTanquesPlasma::Dibujar(
    const Participante participantes[]
) const
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    Camera3D vista = CamaraTanques(*this);

    ClearBackground(COLOR_NIEBLA_TANQUES);
    DibujarCieloTanques(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarEscenarioTanques(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarTanqueTanques(*this, i, participantes[i]);
        }
    }

    DibujarProyectilesTanques(*this, participantes);
    DibujarParticulasTanques(*this);
    DibujarTranslucidosTanques(*this, participantes);

    if (efectos)
    {
        DibujarLucesTanques(*this, participantes, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre cada tanque (o sobre el piloto que salto).
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo)
        {
            continue;
        }

        const EstadoTanquePlasma& tq = tanques[i];
        Vector3 sobre = tq.eliminado
            ? Vector3{ DescansoPilotoTanques(tq).x, 1.55f, DescansoPilotoTanques(tq).y }
            : Vector3{ tq.posicion.x, 2.25f, tq.posicion.y };
        Vector2 p = GetWorldToScreen(sobre, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        op.alfa = tq.eliminado ? Limitar01Tanques((tq.tiempoMuerto - TIEMPO_EYECCION_TANQUES) * 3.0f) * 0.8f : 1.0f;

        if (op.alfa > 0.01f)
        {
            DibujarEtiquetaUI(rotulo, p, 0.0f, op);
        }

        // «¡FUERA!» sobre la carcasa recien destruida.
        if (tq.eliminado && tq.tiempoMuerto < 1.8f)
        {
            Vector2 c = GetWorldToScreen({ tq.posicion.x, 2.0f, tq.posicion.y }, vista);
            float u = tq.tiempoMuerto / 1.8f;
            float alfa = u < 0.1f ? u / 0.1f : (u > 0.75f ? (1.0f - u) / 0.25f : 1.0f);
            DibujarTextoMundoContornoUI("¡FUERA!", c.x, c.y - Px(20.0f) - u * Px(30.0f), UI_TEXTO_GRANDE, Color{ 255, 120, 90, 255 }, alfa);
        }
    }

    // Arranque del combate.
    if (tiempoAnuncio >= 0.0f && fase == FASE_TANQUES_COMBATE)
    {
        float u = tiempoAnuncio / DURACION_ANUNCIO_TANQUES;
        float alfa = u < 0.1f ? u / 0.1f : (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
        float y = (float)alto * 0.36f - u * Px(18.0f);
        DibujarTextoMundoContornoUI("¡FUEGO!", (float)ancho * 0.5f, y, UI_TEXTO_TITULO, Color{ 150, 244, 250, 255 }, alfa);
        DibujarTextoMundoContornoUI("ESCONDETE TRAS LOS CRISTALES", (float)ancho * 0.5f, y + Px(50.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "TANQUES DE PLASMA",
        "DISPARÁ PLASMA A LOS RIVALES Y CUBRITE DETRÁS DE LOS CRISTALES",
        resultado
    );

    hud.fase = fase == FASE_TANQUES_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_TANQUES_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = fase == FASE_TANQUES_PREPARACION ? DURACION_COMBATE_TANQUES : tiempoCombate;
    hud.tiempoTotal = DURACION_COMBATE_TANQUES;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        if (!h.visible) continue;

        const EstadoTanquePlasma& tq = tanques[i];

        std::snprintf(h.valor, sizeof(h.valor), "%d %s", tq.vidas, tq.vidas == 1 ? "VIDA" : "VIDAS");
        h.valorNumerico = tq.vidas * 100 + tq.impactosAcertados;
        h.progreso = (float)tq.vidas / 2.0f;
        h.eliminado = tq.eliminado;

        if (!tq.eliminado && tq.destelloGolpe > 0.0f)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡TOCADO!");
        }

        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d PTS", PuntuacionTanque(tq));
        std::snprintf(h.detalleDesempate, sizeof(h.detalleDesempate), "%d IMPACTO%s", tq.impactosAcertados, tq.impactosAcertados == 1 ? "" : "S");

        if (EsHumanoTanques(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "MOVÉ Y APUNTÁ: %s\nDISPARÁ: %s",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (fase == FASE_TANQUES_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        hud.finPorTiempo = ContarTanquesVivos(*this) > 1;
        hud.notaFinal = "VIDAS x100 + IMPACTOS x25";
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoTanquesPlasma::ObtenerResultado() const
{
    return resultado;
}
