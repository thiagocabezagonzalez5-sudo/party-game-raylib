#pragma once

#include "Minigames/ColisionesMinijuegos.h"
#include "Minigames/UtilidadesMinijuegos.h"

#define SOMBRAS_RETRO_AUTOMATICAS
#include "Minigames/SombrasRetro.h"

#include <cmath>


//==================================================
// PERFIL ESTANDAR DEL JUGADOR
//==================================================

const float VELOCIDAD_JUGADOR_ESTANDAR = 5.0f;
const float FUERZA_SALTO_JUGADOR_ESTANDAR = 7.2f;
const float GRAVEDAD_JUGADOR_ESTANDAR = 18.0f;
const float DURACION_RESPAWN_JUGADOR_ESTANDAR = 1.2f;

const float COOLDOWN_GOLPE_JUGADOR_ESTANDAR = 0.80f;
const float DURACION_RALENTIZACION_GOLPE = 0.70f;
const float MULTIPLICADOR_VELOCIDAD_RALENTIZADO = 0.45f;

// Empuje del golpe. Es un impulso que se frena solo (factor 5.5/s en el
// movimiento normal), asi que recorre aprox. fuerza / 5.5: unos 1.5 m el
// golpe normal y 2.1 m el fuerte. Las colisiones con bloques se resuelven
// en subpasos de 0.3 m, por lo que no atraviesa paredes.
const float FUERZA_GOLPE_JUGADOR_ESTANDAR = 8.5f;
const float FUERZA_GOLPE_JUGADOR_FUERTE = 11.5f;

// Aturdimiento: el golpe normal tambalea, el fuerte (golpear corriendo)
// derriba y deja al rival acostado hasta que se levanta.
const float DURACION_ATURDIDO_GOLPE = 0.55f;
const float DURACION_ATURDIDO_DERRIBO = 1.05f;

// Fraccion de la velocidad maxima hacia adelante para que el golpe cuente
// como "corriendo".
const float FRACCION_VELOCIDAD_GOLPE_FUERTE = 0.60f;

// Preparacion mas corta: conserva la lectura del movimiento pero responde
// bastante antes. Durante toda la preparacion y la caida el jugador sigue
// completamente bloqueado en horizontal.
const float DURACION_PREPARACION_GROUND_POUND = 0.20f;
const float DURACION_APLASTADO_GROUND_POUND = 2.35f;
const float MULTIPLICADOR_APLASTADO_GROUND_POUND = 0.12f;


inline void ConfigurarJugadorMinijuegoEstandar(
    JugadorPrueba& jugador,
    Vector3 posicionSpawn
)
{
    jugador.posicionSpawn = posicionSpawn;
    jugador.tamano = { 0.8f, 1.4f, 0.8f };
    jugador.velocidadMovimiento = VELOCIDAD_JUGADOR_ESTANDAR;
    jugador.fuerzaSalto = FUERZA_SALTO_JUGADOR_ESTANDAR;
    jugador.gravedad = GRAVEDAD_JUGADOR_ESTANDAR;
    jugador.duracionRespawn = DURACION_RESPAWN_JUGADOR_ESTANDAR;

    ReiniciarJugadorPrueba(jugador);
}


//==================================================
// PLATAFORMAS MOVILES DESCENDENTES
//==================================================

inline bool HayBloquesDescendiendo(
    const BloquePrueba bloques[],
    int cantidadBloques
)
{
    if (bloques == nullptr)
    {
        return false;
    }

    for (int i = 0; i < cantidadBloques; i++)
    {
        if (
            bloques[i].activaColision &&
            bloques[i].cayendo
        )
        {
            return true;
        }
    }

    return false;
}


inline void AcompanharPlataformaDescendente(
    JugadorPrueba& jugador,
    BloquePrueba bloques[],
    int cantidadBloques,
    float deltaTime
)
{
    if (
        bloques == nullptr ||
        cantidadBloques <= 0 ||
        !jugador.enSuelo ||
        jugador.cayendo ||
        jugador.velocidad.y > 0.01f
    )
    {
        return;
    }

    const float mitadY = jugador.tamano.y / 2.0f;
    const float piesJugador = jugador.posicion.y - mitadY;

    int indiceSoporte = -1;
    float mejorDistancia = 1000000.0f;

    for (int i = 0; i < cantidadBloques; i++)
    {
        const BloquePrueba& bloque = bloques[i];

        if (!bloque.activaColision)
        {
            continue;
        }

        BoundingBox cajaBloque = CrearHitboxBloquePrueba(bloque);
        const float MARGEN_CENTRO = 0.035f;

        bool centroDentro =
            jugador.posicion.x >= cajaBloque.min.x - MARGEN_CENTRO &&
            jugador.posicion.x <= cajaBloque.max.x + MARGEN_CENTRO &&
            jugador.posicion.z >= cajaBloque.min.z - MARGEN_CENTRO &&
            jugador.posicion.z <= cajaBloque.max.z + MARGEN_CENTRO;

        if (!centroDentro)
        {
            continue;
        }

        float descensoEsteFrame =
            bloque.cayendo && bloque.velocidadCaida > 0.0f
            ? bloque.velocidadCaida * deltaTime
            : 0.0f;

        if (descensoEsteFrame < 0.0f)
        {
            descensoEsteFrame = 0.0f;
        }

        float separacion = piesJugador - cajaBloque.max.y;

        if (
            separacion < -0.075f ||
            separacion > descensoEsteFrame + 0.11f
        )
        {
            continue;
        }

        float dx = jugador.posicion.x - bloque.posicion.x;
        float dz = jugador.posicion.z - bloque.posicion.z;
        float distancia = dx * dx + dz * dz;

        bool mejorCandidato =
            distancia < mejorDistancia - 0.0005f;

        bool empateCasiExacto =
            std::fabs(distancia - mejorDistancia) <= 0.0005f;

        bool priorizarEstable =
            empateCasiExacto &&
            indiceSoporte >= 0 &&
            bloques[indiceSoporte].cayendo &&
            !bloque.cayendo;

        if (mejorCandidato || priorizarEstable)
        {
            indiceSoporte = i;
            mejorDistancia = distancia;
        }
    }

    if (indiceSoporte < 0)
    {
        return;
    }

    const BloquePrueba& soporte = bloques[indiceSoporte];

    if (
        !soporte.cayendo ||
        soporte.velocidadCaida <= 0.0f
    )
    {
        return;
    }

    BoundingBox cajaSoporte = CrearHitboxBloquePrueba(soporte);

    jugador.posicion.y = cajaSoporte.max.y + mitadY;
    jugador.velocidad.y = 0.0f;
    jugador.enSuelo = true;
}


//==================================================
// MOVIMIENTO NORMAL + RALENTIZACION
//==================================================

inline void BloquearMovimientoHorizontalGroundPound(
    JugadorPrueba& jugador,
    InputMinijuegoParticipante& entrada
)
{
    entrada.izquierda = false;
    entrada.derecha = false;
    entrada.adelante = false;
    entrada.atras = false;
    entrada.saltar = false;
    entrada.golpear = false;

    jugador.velocidad.x = 0.0f;
    jugador.velocidad.z = 0.0f;
    jugador.empuje.x = 0.0f;
    jugador.empuje.z = 0.0f;
}


inline void ActualizarJugadorPruebaNormal(
    JugadorPrueba& jugador,
    const InputMinijuegoParticipante& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    int cantidadParticulas,
    bool permitirSalto,
    bool respawnAutomatico,
    float deltaTime
)
{
    jugador.golpeSueloRecibido = false;

    bool hayPlataformasMoviles =
        HayBloquesDescendiendo(
            bloques,
            cantidadBloques
        );

    if (!hayPlataformasMoviles)
    {
        AcompanharPlataformaDescendente(
            jugador,
            bloques,
            cantidadBloques,
            deltaTime
        );
    }

    if (jugador.tiempoRalentizado > 0.0f)
    {
        jugador.tiempoRalentizado -= deltaTime;

        if (jugador.tiempoRalentizado <= 0.0f)
        {
            jugador.tiempoRalentizado = 0.0f;
            jugador.multiplicadorRalentizacion = 1.0f;
        }
    }

    InputMinijuegoParticipante entradaProcesada = entrada;

    if (
        permitirSalto &&
        !jugador.enSuelo &&
        entradaProcesada.saltar &&
        !jugador.golpeSueloActivo &&
        !jugador.preparandoGolpeSuelo
    )
    {
        jugador.preparandoGolpeSuelo = true;
        jugador.tiempoPreparacionGolpeSuelo =
            DURACION_PREPARACION_GROUND_POUND;
        jugador.golpeando = false;
        jugador.tiempoAnticipacionGolpe = 0.0f;
        jugador.tiempoAnimacionGolpe = 0.0f;
    }

    if (jugador.enSuelo && jugador.preparandoGolpeSuelo)
    {
        jugador.preparandoGolpeSuelo = false;
        jugador.tiempoPreparacionGolpeSuelo = 0.0f;
    }

    if (jugador.preparandoGolpeSuelo)
    {
        BloquearMovimientoHorizontalGroundPound(
            jugador,
            entradaProcesada
        );

        jugador.velocidad.y = 0.0f;
        jugador.tiempoPreparacionGolpeSuelo -= deltaTime;

        if (jugador.tiempoPreparacionGolpeSuelo <= 0.0f)
        {
            jugador.preparandoGolpeSuelo = false;
            jugador.tiempoPreparacionGolpeSuelo = 0.0f;
            jugador.golpeSueloActivo = true;
            jugador.velocidad.y = -28.0f;
        }
    }

    if (jugador.golpeSueloActivo)
    {
        BloquearMovimientoHorizontalGroundPound(
            jugador,
            entradaProcesada
        );
    }

    bool estabaAplastado = jugador.aplastado;
    float tiempoAplastadoRestante = jugador.tiempoAplastado;

    if (estabaAplastado)
    {
        jugador.aplastado = false;
        entradaProcesada.saltar = false;
        entradaProcesada.golpear = false;
    }

    float velocidadOriginal = jugador.velocidadMovimiento;

    if (jugador.tiempoRalentizado > 0.0f)
    {
        float factor = jugador.multiplicadorRalentizacion;
        if (factor < 0.05f) factor = 0.05f;
        if (factor > 1.0f) factor = 1.0f;
        jugador.velocidadMovimiento *= factor;
    }

    if (estabaAplastado)
    {
        jugador.velocidadMovimiento *=
            MULTIPLICADOR_APLASTADO_GROUND_POUND;
    }

    bool golpeandoAntes = jugador.golpeando;
    bool estabaCayendo = jugador.cayendo;
    bool estabaHaciendoGroundPound =
        jugador.preparandoGolpeSuelo ||
        jugador.golpeSueloActivo;
    Vector3 posicionAntesColision = jugador.posicion;

    ActualizarJugadorPrueba(
        jugador,
        entradaProcesada,
        bloques,
        cantidadBloques,
        particulas,
        cantidadParticulas,
        permitirSalto,
        false,
        respawnAutomatico,
        deltaTime
    );

    // La fisica base puede conservar inercia horizontal aun con las entradas
    // anuladas. Mientras dure el ground pound la eliminamos despues de cada
    // actualizacion para que el personaje permanezca realmente estatico.
    if (estabaHaciendoGroundPound || jugador.golpeSueloActivo)
    {
        jugador.velocidad.x = 0.0f;
        jugador.velocidad.z = 0.0f;
        jugador.empuje.x = 0.0f;
        jugador.empuje.z = 0.0f;
    }

    if (
        !hayPlataformasMoviles &&
        !estabaCayendo &&
        !jugador.cayendo
    )
    {
        CorregirMovimientoJugadorContraBloques(
            jugador,
            posicionAntesColision,
            bloques,
            cantidadBloques
        );
    }

    jugador.velocidadMovimiento = velocidadOriginal;

    if (estabaAplastado)
    {
        tiempoAplastadoRestante -= deltaTime;

        if (
            tiempoAplastadoRestante > 0.0f &&
            !jugador.cayendo
        )
        {
            jugador.aplastado = true;
            jugador.tiempoAplastado = tiempoAplastadoRestante;
        }
        else
        {
            jugador.aplastado = false;
            jugador.tiempoAplastado = 0.0f;

            if (!jugador.cayendo && jugador.tiempoInmunidad < 0.65f)
            {
                jugador.tiempoInmunidad = 0.65f;
            }
        }
    }

    if (!golpeandoAntes && jugador.golpeando)
    {
        jugador.cooldownGolpe = COOLDOWN_GOLPE_JUGADOR_ESTANDAR;
    }

    if (estabaCayendo && !jugador.cayendo)
    {
        jugador.tiempoRalentizado = 0.0f;
        jugador.multiplicadorRalentizacion = 1.0f;
        jugador.preparandoGolpeSuelo = false;
        jugador.tiempoPreparacionGolpeSuelo = 0.0f;
        jugador.golpeSueloRecibido = false;
    }
}


inline void ActualizarJugadorMinijuegoEstandar(
    JugadorPrueba& jugador,
    const Participante& participante,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    int cantidadParticulas,
    bool respawnAutomatico,
    float deltaTime
)
{
    if (!participante.activo)
    {
        return;
    }

    InputMinijuegoParticipante entrada{};

    if (participante.conectado)
    {
        entrada = LeerInputMinijuegoParticipante(participante);
    }

    ActualizarJugadorPruebaNormal(
        jugador,
        entrada,
        bloques,
        cantidadBloques,
        particulas,
        cantidadParticulas,
        true,
        respawnAutomatico,
        deltaTime
    );
}


//==================================================
// COLISION SOLIDA ENTRE JUGADORES SIN EMPUJE
//==================================================

inline void ResolverColisionesJugadoresSinEmpuje(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    const float MARGEN = 0.001f;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        JugadorPrueba& a = jugadores[i];

        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            a.cayendo
        )
        {
            continue;
        }

        for (int j = i + 1; j < cantidadMaxima; j++)
        {
            JugadorPrueba& b = jugadores[j];

            if (
                !participantes[j].activo ||
                !participantes[j].conectado ||
                b.cayendo
            )
            {
                continue;
            }

            BoundingBox cajaA = CrearHitboxJugadorPrueba(a);
            BoundingBox cajaB = CrearHitboxJugadorPrueba(b);

            bool seSolapan =
                cajaA.min.x < cajaB.max.x &&
                cajaA.max.x > cajaB.min.x &&
                cajaA.min.y < cajaB.max.y &&
                cajaA.max.y > cajaB.min.y &&
                cajaA.min.z < cajaB.max.z &&
                cajaA.max.z > cajaB.min.z;

            if (!seSolapan)
            {
                continue;
            }

            float solapeX =
                a.posicion.x < b.posicion.x
                ? cajaA.max.x - cajaB.min.x
                : cajaB.max.x - cajaA.min.x;

            float solapeZ =
                a.posicion.z < b.posicion.z
                ? cajaA.max.z - cajaB.min.z
                : cajaB.max.z - cajaA.min.z;

            // Se corrige sobre el eje de menor solape. "a" siempre tiene el
            // indice menor, asi que la resolucion NO puede depender de ese
            // orden: si solo uno avanza hacia el otro, se frena al que avanza
            // (no hay empuje); si avanzan los dos (o ninguno), la correccion
            // se reparte a partes iguales y ambos se frenan en ese eje.
            bool ejeX = solapeX < solapeZ;

            float posicionA = ejeX ? a.posicion.x : a.posicion.z;
            float posicionB = ejeX ? b.posicion.x : b.posicion.z;
            float velocidadA = ejeX ? a.velocidad.x : a.velocidad.z;
            float velocidadB = ejeX ? b.velocidad.x : b.velocidad.z;
            float solape = ejeX ? solapeX : solapeZ;

            // Signo hacia el que se separa "a" (alejandose de "b").
            float signoA = posicionA < posicionB ? -1.0f : 1.0f;

            bool aVaHaciaB = velocidadA * -signoA > 0.001f;
            bool bVaHaciaA = velocidadB * signoA > 0.001f;

            float correccion = solape + MARGEN;
            float correccionA = 0.0f;
            float correccionB = 0.0f;

            if (aVaHaciaB && !bVaHaciaA)
            {
                correccionA = correccion;
            }
            else if (bVaHaciaA && !aVaHaciaB)
            {
                correccionB = correccion;
            }
            else
            {
                correccionA = correccion * 0.5f;
                correccionB = correccion * 0.5f;
            }

            if (correccionA > 0.0f)
            {
                if (ejeX)
                {
                    a.posicion.x += signoA * correccionA;
                    a.velocidad.x = 0.0f;
                    a.empuje.x = 0.0f;
                }
                else
                {
                    a.posicion.z += signoA * correccionA;
                    a.velocidad.z = 0.0f;
                    a.empuje.z = 0.0f;
                }
            }

            if (correccionB > 0.0f)
            {
                if (ejeX)
                {
                    b.posicion.x -= signoA * correccionB;
                    b.velocidad.x = 0.0f;
                    b.empuje.x = 0.0f;
                }
                else
                {
                    b.posicion.z -= signoA * correccionB;
                    b.velocidad.z = 0.0f;
                    b.empuje.z = 0.0f;
                }
            }
        }
    }
}


//==================================================
// PARTICULAS DEL GOLPE HORIZONTAL
//==================================================

inline void CrearParticulasImpactoGolpe(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    Vector3 posicionImpacto
)
{
    if (particulas == nullptr || cantidadMaxima <= 0)
    {
        return;
    }

    const int CANTIDAD_CREAR = 18;
    int creadas = 0;

    for (int i = 0; i < cantidadMaxima && creadas < CANTIDAD_CREAR; i++)
    {
        ParticulaTierra& particula = particulas[i];
        if (particula.activa) continue;

        float direccionX = (float)GetRandomValue(-100, 100) / 100.0f;
        float direccionZ = (float)GetRandomValue(-100, 100) / 100.0f;
        float longitud = std::sqrt(direccionX * direccionX + direccionZ * direccionZ);

        if (longitud < 0.01f)
        {
            direccionX = 1.0f;
            direccionZ = 0.0f;
            longitud = 1.0f;
        }

        direccionX /= longitud;
        direccionZ /= longitud;

        float velocidadHorizontal = (float)GetRandomValue(18, 42) / 10.0f;

        particula.activa = true;
        particula.posicion =
        {
            posicionImpacto.x + (float)GetRandomValue(-12, 12) / 100.0f,
            posicionImpacto.y + (float)GetRandomValue(-8, 18) / 100.0f,
            posicionImpacto.z + (float)GetRandomValue(-12, 12) / 100.0f
        };
        particula.velocidad =
        {
            direccionX * velocidadHorizontal,
            (float)GetRandomValue(18, 45) / 10.0f,
            direccionZ * velocidadHorizontal
        };
        particula.vidaMaxima = (float)GetRandomValue(18, 34) / 100.0f;
        particula.vida = particula.vidaMaxima;
        particula.tamano = (float)GetRandomValue(7, 14) / 100.0f;
        particula.color = GetRandomValue(0, 1) == 0 ? YELLOW : GOLD;
        creadas++;
    }
}


//==================================================
// GOLPE HORIZONTAL
//==================================================

// Un jugador solo puede recibir un golpe si no esta ya en otro estado de
// impacto (aturdido, aplastado, cayendo) ni inmune.
inline bool PuedeRecibirGolpeJugador(
    const JugadorPrueba& objetivo
)
{
    return
        !objetivo.cayendo &&
        !objetivo.aplastado &&
        objetivo.tiempoAturdido <= 0.0f &&
        objetivo.tiempoInmunidad <= 0.0f;
}


// Golpe fuerte: el atacante venia corriendo hacia donde mira.
inline bool EsGolpeFuerteJugador(
    const JugadorPrueba& atacante
)
{
    float avance =
        atacante.velocidad.x * atacante.direccionMirada.x +
        atacante.velocidad.z * atacante.direccionMirada.z;

    float maxima =
        atacante.velocidadMovimiento > 1.0f
        ? atacante.velocidadMovimiento
        : 1.0f;

    return avance >= maxima * FRACCION_VELOCIDAD_GOLPE_FUERTE;
}


// Direccion del empuje: sobre todo la linea entre ambos (posicion) y en
// parte hacia donde mira el atacante (orientacion). Siempre horizontal.
inline Vector3 DireccionEmpujeGolpeJugador(
    const JugadorPrueba& atacante,
    float normalX,
    float normalZ
)
{
    float x = normalX * 0.6f + atacante.direccionMirada.x * 0.4f;
    float z = normalZ * 0.6f + atacante.direccionMirada.z * 0.4f;
    float largo = std::sqrt(x * x + z * z);

    if (largo < 0.001f)
    {
        return { normalX, 0.0f, normalZ };
    }

    return { x / largo, 0.0f, z / largo };
}


// Aplica el impacto completo a quien recibe el golpe: empuje, aturdimiento
// (tambaleo o derribo) y cancelacion de lo que estuviera haciendo. El
// empuje reemplaza al anterior para que no se acumulen impulsos.
inline void AplicarGolpeRecibidoJugador(
    JugadorPrueba& objetivo,
    Vector3 direccion,
    bool fuerte
)
{
    float fuerza =
        fuerte
        ? FUERZA_GOLPE_JUGADOR_FUERTE
        : FUERZA_GOLPE_JUGADOR_ESTANDAR;

    objetivo.empuje.x = direccion.x * fuerza;
    objetivo.empuje.z = direccion.z * fuerza;

    objetivo.tiempoAturdido =
        fuerte
        ? DURACION_ATURDIDO_DERRIBO
        : DURACION_ATURDIDO_GOLPE;

    objetivo.duracionAturdido = objetivo.tiempoAturdido;
    objetivo.derribado = fuerte;
    objetivo.direccionGolpeRecibido = direccion;

    objetivo.golpeando = false;
    objetivo.tiempoGolpe = 0.0f;
    objetivo.tiempoAnticipacionGolpe = 0.0f;
    objetivo.tiempoAnimacionGolpe = 0.0f;
    objetivo.preparandoGolpeSuelo = false;
    objetivo.tiempoPreparacionGolpeSuelo = 0.0f;

    objetivo.tiempoRalentizado = DURACION_RALENTIZACION_GOLPE;
    objetivo.multiplicadorRalentizacion =
        MULTIPLICADOR_VELOCIDAD_RALENTIZADO;

    if (fuerte)
    {
        ActivarTemblorCamaraGeneral(0.07f, 0.16f);
    }
}


inline void ResolverGolpesJugadoresConEfectos(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        JugadorPrueba& atacante = jugadores[i];

        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            atacante.cayendo ||
            atacante.aplastado ||
            !atacante.golpeando ||
            atacante.golpeYaConecto
        )
        {
            continue;
        }

        // Se golpea al objetivo valido MAS CERCANO del cono, no al primero
        // por indice: el numero de jugador no debe dar prioridad.
        int mejorObjetivo = -1;
        float mejorDistancia = 0.0f;
        float mejorNormalX = 0.0f;
        float mejorNormalZ = 0.0f;

        for (int j = 0; j < cantidadMaxima; j++)
        {
            if (i == j) continue;

            const JugadorPrueba& objetivo = jugadores[j];

            if (
                !participantes[j].activo ||
                !participantes[j].conectado ||
                !PuedeRecibirGolpeJugador(objetivo)
            )
            {
                continue;
            }

            float dx = objetivo.posicion.x - atacante.posicion.x;
            float dz = objetivo.posicion.z - atacante.posicion.z;
            float distancia = std::sqrt(dx * dx + dz * dz);

            if (distancia < 0.001f || distancia > 1.55f)
                continue;

            if (std::fabs(objetivo.posicion.y - atacante.posicion.y) > 1.0f)
                continue;

            float normalX = dx / distancia;
            float normalZ = dz / distancia;
            float frente =
                normalX * atacante.direccionMirada.x +
                normalZ * atacante.direccionMirada.z;

            if (frente < 0.25f)
                continue;

            if (mejorObjetivo < 0 || distancia < mejorDistancia)
            {
                mejorObjetivo = j;
                mejorDistancia = distancia;
                mejorNormalX = normalX;
                mejorNormalZ = normalZ;
            }
        }

        if (mejorObjetivo >= 0)
        {
            JugadorPrueba& objetivo = jugadores[mejorObjetivo];
            float normalX = mejorNormalX;
            float normalZ = mejorNormalZ;

            AplicarGolpeRecibidoJugador(
                objetivo,
                DireccionEmpujeGolpeJugador(atacante, normalX, normalZ),
                EsGolpeFuerteJugador(atacante)
            );

            Vector3 posicionImpacto =
            {
                atacante.posicion.x + normalX * 0.82f,
                (atacante.posicion.y + objetivo.posicion.y) / 2.0f + 0.12f,
                atacante.posicion.z + normalZ * 0.82f
            };

            CrearParticulasImpactoGolpe(
                particulas,
                cantidadParticulas,
                posicionImpacto
            );

            atacante.golpeYaConecto = true;
            ReproducirSonidoJugadorMinijuego(SONIDO_GOLPE);
        }
    }
}


inline bool ResolverInteraccionesJugadoresMinijuegoEstandar(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    bool huboGolpeSuelo = ResolverGolpesSuelo(
        jugadores,
        participantes,
        cantidadMaxima,
        DURACION_APLASTADO_GROUND_POUND
    );

    if (huboGolpeSuelo)
    {
        ActivarTemblorCamaraGeneral(0.18f, 0.28f);
    }

    ResolverGolpesJugadoresConEfectos(
        jugadores,
        participantes,
        cantidadMaxima,
        particulas,
        cantidadParticulas
    );

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantes,
        cantidadMaxima
    );

    return huboGolpeSuelo;
}
