#include "Minigames/MinijuegoTanquesPlasma.h"

#include "raylib.h"
#include "raymath.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_TANQUES = 2.5f;
static const float DURACION_COMBATE_TANQUES = 45.0f;
static const float RADIO_TANQUE = 21.0f;
static const float RADIO_PROYECTIL = 7.0f;
static const float VELOCIDAD_TANQUE = 155.0f;
static const float VELOCIDAD_PROYECTIL = 430.0f;
static const float TIEMPO_RECARGA_TANQUE = 0.48f;


static Rectangle ObtenerArenaTanques()
{
    return Rectangle
    {
        62.0f,
        126.0f,
        GetScreenWidth() - 124.0f,
        GetScreenHeight() - 218.0f
    };
}


static void ObtenerObstaculosTanques(
    Vector2 centros[3],
    float radios[3]
)
{
    Rectangle arena = ObtenerArenaTanques();

    centros[0] =
    {
        arena.x + arena.width * 0.50f,
        arena.y + arena.height * 0.50f
    };
    centros[1] =
    {
        arena.x + arena.width * 0.27f,
        arena.y + arena.height * 0.32f
    };
    centros[2] =
    {
        arena.x + arena.width * 0.73f,
        arena.y + arena.height * 0.68f
    };

    radios[0] = 53.0f;
    radios[1] = 37.0f;
    radios[2] = 37.0f;
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
}


static void LimitarTanqueEnArena(
    EstadoTanquePlasma& tanque
)
{
    Rectangle arena = ObtenerArenaTanques();

    tanque.posicion.x = Clamp(
        tanque.posicion.x,
        arena.x + RADIO_TANQUE,
        arena.x + arena.width - RADIO_TANQUE
    );

    tanque.posicion.y = Clamp(
        tanque.posicion.y,
        arena.y + RADIO_TANQUE,
        arena.y + arena.height - RADIO_TANQUE
    );

    Vector2 centros[3]{};
    float radios[3]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < 3; i++)
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
            Vector2Scale(tanque.direccion, RADIO_TANQUE + 12.0f)
        );
        proyectil.velocidad = Vector2Scale(
            tanque.direccion,
            VELOCIDAD_PROYECTIL
        );
        proyectil.tiempoVida = 2.8f;
        tanque.recarga = TIEMPO_RECARGA_TANQUE;
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
    Vector2 centros[3]{};
    float radios[3]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < 3; i++)
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
    Rectangle arena = ObtenerArenaTanques();

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

        if (
            proyectil.tiempoVida <= 0.0f ||
            !CheckCollisionPointRec(proyectil.posicion, arena) ||
            ProyectilChocaObstaculo(proyectil.posicion)
        )
        {
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
                minijuego.proyectiles[i].activo = false;
                minijuego.proyectiles[j].activo = false;
                break;
            }
        }
    }
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

    Rectangle arena = ObtenerArenaTanques();
    Vector2 spawns[MAX_PARTICIPANTES] =
    {
        { arena.x + 78.0f, arena.y + 76.0f },
        { arena.x + arena.width - 78.0f, arena.y + arena.height - 76.0f },
        { arena.x + arena.width - 78.0f, arena.y + 76.0f },
        { arena.x + 78.0f, arena.y + arena.height - 76.0f }
    };

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        tanques[i].posicion = spawns[i];

        Vector2 haciaCentro = Vector2Subtract(
            {
                arena.x + arena.width * 0.5f,
                arena.y + arena.height * 0.5f
            },
            spawns[i]
        );
        tanques[i].direccion = Vector2Normalize(haciaCentro);
    }
}


void MinijuegoTanquesPlasma::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_TANQUES_TERMINADO)
    {
        return;
    }

    if (fase == FASE_TANQUES_PREPARACION)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            fase = FASE_TANQUES_COMBATE;
        }

        return;
    }

    tiempoCombate -= deltaTime;

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

        if (participantes[i].esBot)
        {
            int objetivo = BuscarObjetivoTanque(*this, i);

            if (objetivo >= 0)
            {
                Vector2 haciaObjetivo = Vector2Normalize(
                    Vector2Subtract(tanques[objetivo].posicion, tanque.posicion)
                );
                tanque.direccion = haciaObjetivo;

                float lado = std::sin(tiempoAnimacion * 1.45f + tanque.faseBot) >= 0.0f
                    ? 1.0f
                    : -1.0f;
                Vector2 lateral = { -haciaObjetivo.y * lado, haciaObjetivo.x * lado };
                movimiento = Vector2Add(
                    lateral,
                    Vector2Scale(haciaObjetivo, -0.18f)
                );
                disparar = tanque.recarga <= 0.0f;
            }
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
        }

        if (disparar)
        {
            DispararTanque(*this, i);
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


static void DibujarTanquePlasma(
    const EstadoTanquePlasma& tanque,
    const Participante& participante,
    float tiempoAnimacion
)
{
    if (tanque.eliminado)
    {
        DrawCircleV(tanque.posicion, 27.0f, Fade(BLACK, 0.45f));
        DrawLineEx(
            { tanque.posicion.x - 20.0f, tanque.posicion.y - 20.0f },
            { tanque.posicion.x + 20.0f, tanque.posicion.y + 20.0f },
            6.0f,
            RED
        );
        DrawLineEx(
            { tanque.posicion.x + 20.0f, tanque.posicion.y - 20.0f },
            { tanque.posicion.x - 20.0f, tanque.posicion.y + 20.0f },
            6.0f,
            RED
        );
        return;
    }

    bool ocultar =
        tanque.invulnerabilidad > 0.0f &&
        ((int)(tiempoAnimacion * 18.0f) % 2 == 0);

    if (ocultar)
    {
        return;
    }

    float angulo = std::atan2(tanque.direccion.y, tanque.direccion.x) * RAD2DEG;

    DrawCircleV(tanque.posicion, RADIO_TANQUE + 7.0f, Fade(BLACK, 0.42f));
    DrawRectanglePro(
        { tanque.posicion.x, tanque.posicion.y, 44.0f, 31.0f },
        { 22.0f, 15.5f },
        angulo,
        participante.color
    );
    DrawRectanglePro(
        {
            tanque.posicion.x + tanque.direccion.x * 19.0f,
            tanque.posicion.y + tanque.direccion.y * 19.0f,
            34.0f,
            8.0f
        },
        { 4.0f, 4.0f },
        angulo,
        RAYWHITE
    );
    DrawCircleV(tanque.posicion, 10.0f, ColorLerp(participante.color, RAYWHITE, 0.35f));
    DrawCircleLines((int)tanque.posicion.x, (int)tanque.posicion.y, RADIO_TANQUE, BLACK);

    for (int vida = 0; vida < tanque.vidas; vida++)
    {
        DrawRectangle(
            (int)tanque.posicion.x - 12 + vida * 15,
            (int)tanque.posicion.y - 39,
            11,
            6,
            LIME
        );
    }

    DrawText(
        TextFormat("J%d", participante.numeroJugador),
        (int)tanque.posicion.x - 12,
        (int)tanque.posicion.y + 29,
        15,
        RAYWHITE
    );
}


void MinijuegoTanquesPlasma::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 10, 20, 31, 255 });

    Rectangle arena = ObtenerArenaTanques();
    DrawRectangleRec(arena, Color{ 24, 43, 55, 255 });

    for (int x = (int)arena.x; x < arena.x + arena.width; x += 42)
    {
        DrawLine(x, (int)arena.y, x, (int)(arena.y + arena.height), Fade(SKYBLUE, 0.08f));
    }

    for (int y = (int)arena.y; y < arena.y + arena.height; y += 42)
    {
        DrawLine((int)arena.x, y, (int)(arena.x + arena.width), y, Fade(SKYBLUE, 0.08f));
    }

    DrawRectangleLinesEx(arena, 5.0f, Color{ 90, 210, 239, 255 });

    Vector2 centros[3]{};
    float radios[3]{};
    ObtenerObstaculosTanques(centros, radios);

    for (int i = 0; i < 3; i++)
    {
        DrawCircleV(centros[i], radios[i], Color{ 57, 65, 75, 255 });
        DrawCircleLines((int)centros[i].x, (int)centros[i].y, radios[i], Fade(RAYWHITE, 0.55f));
        DrawCircleLines((int)centros[i].x, (int)centros[i].y, radios[i] - 9.0f, Fade(SKYBLUE, 0.35f));
    }

    for (int i = 0; i < MAX_PROYECTILES_TANQUES_PLASMA; i++)
    {
        const ProyectilTanquePlasma& proyectil = proyectiles[i];
        if (!proyectil.activo) continue;

        Color color = participantes[proyectil.propietario].color;
        DrawCircleV(proyectil.posicion, 12.0f, Fade(color, 0.24f));
        DrawCircleV(proyectil.posicion, RADIO_PROYECTIL, color);
        DrawCircleLines((int)proyectil.posicion.x, (int)proyectil.posicion.y, RADIO_PROYECTIL, RAYWHITE);
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            DibujarTanquePlasma(tanques[i], participantes[i], tiempoAnimacion);
        }
    }

    DrawRectangle(18, 15, 598, 96, Fade(BLACK, 0.82f));
    DrawText("TANQUES DE PLASMA", 32, 27, 30, SKYBLUE);

    if (fase == FASE_TANQUES_PREPARACION)
    {
        DrawText(TextFormat("PREPARATE  %.1f", tiempoPreparacion), 32, 70, 21, RAYWHITE);
    }
    else if (fase == FASE_TANQUES_COMBATE)
    {
        DrawText(
            TextFormat("TIEMPO %.1f  |  MOVER/APUNTAR + GOLPEAR PARA DISPARAR", tiempoCombate),
            32,
            72,
            18,
            RAYWHITE
        );
    }
    else
    {
        DrawText("BATALLA TERMINADA  |  R PARA REINICIAR", 32, 72, 19, GOLD);
    }

    int panelX = GetScreenWidth() - 225;
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
