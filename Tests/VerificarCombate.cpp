// Prueba del combate compartido de los minijuegos (sin pantalla): golpe con
// anticipacion, empuje (fuerza, direccion y paredes), aturdimiento sin
// control, recuperacion automatica, golpe fuerte que derriba, sin
// aturdimientos encadenados y poses de animacion que lee el dibujo.
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>

static int errores = 0;

static void Comprobar(bool condicion, const char* formato, ...)
{
    if (!condicion)
    {
        char texto[256];
        va_list argumentos;
        va_start(argumentos, formato);
        std::vsnprintf(texto, sizeof(texto), formato, argumentos);
        va_end(argumentos);
        std::fprintf(stderr, "FALLO: %s\n", texto);
        errores++;
    }
}

static const float DT = 1.0f / 60.0f;
static const int MAX_BLOQUES = 4;

struct Escena
{
    JugadorPrueba jugadores[MAX_PARTICIPANTES];
    Participante participantes[MAX_PARTICIPANTES];
    InputMinijuegoParticipante entradas[MAX_PARTICIPANTES];
    BloquePrueba bloques[MAX_BLOQUES];
    int cantidadBloques = 0;
    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];
    int cantidad = 2;
};

static Escena escena;

static void Preparar(int cantidad, const Vector3 posiciones[])
{
    escena = Escena{};
    escena.cantidad = cantidad;

    // Piso amplio con la cara superior en y = 0.
    AgregarBloquePrueba(escena.bloques, escena.cantidadBloques, MAX_BLOQUES, { 0.0f, -0.5f, 0.0f }, { 40.0f, 1.0f, 40.0f }, GRAY);

    for (int i = 0; i < cantidad; i++)
    {
        Participante& p = escena.participantes[i];
        p.activo = true;
        p.conectado = true;
        p.numeroJugador = i + 1;

        ConfigurarJugadorMinijuegoEstandar(escena.jugadores[i], posiciones[i]);
        escena.jugadores[i].direccionMirada = { 1.0f, 0.0f, 0.0f };
    }
}

static void Paso()
{
    for (int i = 0; i < escena.cantidad; i++)
    {
        ActualizarJugadorPruebaNormal(
            escena.jugadores[i],
            escena.entradas[i],
            escena.bloques,
            escena.cantidadBloques,
            escena.particulas,
            MAX_PARTICULAS_TIERRA,
            true,
            false,
            DT
        );
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        escena.jugadores,
        escena.participantes,
        escena.cantidad,
        escena.particulas,
        MAX_PARTICULAS_TIERRA
    );
}

static void Asentar()
{
    for (int f = 0; f < 20; f++) Paso();
}

// Aprieta golpe un frame y avanza hasta que el golpe queda activo.
static int GolpearYEsperarActivo(int atacante)
{
    escena.entradas[atacante].golpear = true;
    Paso();
    escena.entradas[atacante].golpear = false;

    int frames = 1;

    while (!escena.jugadores[atacante].golpeando && frames < 60)
    {
        Paso();
        frames++;
    }

    return frames;
}


static void ProbarAnticipacionYGolpeNormal()
{
    const Vector3 pos[2] = { { 0.0f, 0.7f, 0.0f }, { 1.0f, 0.7f, 0.0f } };
    Preparar(2, pos);
    Asentar();

    JugadorPrueba& a = escena.jugadores[0];
    JugadorPrueba& b = escena.jugadores[1];

    Comprobar(a.enSuelo && b.enSuelo, "Ambos jugadores apoyados en el piso");

    escena.entradas[0].golpear = true;
    Paso();
    escena.entradas[0].golpear = false;

    Comprobar(!a.golpeando, "El golpe no es activo en el frame del boton (anticipacion)");
    Comprobar(a.tiempoAnticipacionGolpe > 0.0f, "La anticipacion empezo");
    Comprobar(a.tiempoAnimacionGolpe > 0.0f, "La animacion del golpe empezo");
    Comprobar(b.tiempoAturdido <= 0.0f, "El rival no recibe el golpe durante la anticipacion");

    float xAntes = b.posicion.x;
    int frames = 1;

    while (!a.golpeando && frames < 60)
    {
        Paso();
        frames++;
    }

    float segundos = frames * DT;
    Comprobar(segundos >= DURACION_ANTICIPACION_GOLPE - 0.001f && segundos <= DURACION_ANTICIPACION_GOLPE + 2.0f * DT,
        "Golpe activo tras la anticipacion (%.3f s)", segundos);
    Comprobar(b.tiempoAturdido > 0.0f, "El rival queda aturdido al conectar");
    Comprobar(!b.derribado, "Golpe parado: tambalea, no derriba");
    Comprobar(std::fabs(b.duracionAturdido - DURACION_ATURDIDO_GOLPE) < 0.001f, "Duracion del aturdimiento leve");
    Comprobar(b.direccionGolpeRecibido.x > 0.9f, "Empuje hacia +X (lejos del atacante)");

    // Mientras esta aturdido intenta volver hacia el atacante: no puede.
    escena.entradas[1].izquierda = true;
    float ultimoX = b.posicion.x;
    bool retrocedio = false;
    int framesAturdido = 0;

    while (b.tiempoAturdido > 0.0f && framesAturdido < 200)
    {
        Paso();
        if (b.posicion.x < ultimoX - 0.0001f) retrocedio = true;
        ultimoX = b.posicion.x;
        framesAturdido++;
    }

    Comprobar(!retrocedio, "Sin control durante el aturdimiento (no avanza contra el empuje)");
    Comprobar(std::fabs(framesAturdido * DT - DURACION_ATURDIDO_GOLPE) < 0.05f,
        "Se recupera solo al terminar el aturdimiento (%.2f s)", framesAturdido * DT);
    Comprobar(b.tiempoInmunidad > 0.0f, "Inmunidad breve al recuperarse");

    float desplazamiento = b.posicion.x - xAntes;
    Comprobar(desplazamiento > 1.15f && desplazamiento < 2.0f,
        "Empuje del golpe normal contundente pero acotado (%.2f m)", desplazamiento);

    // Recupera el control.
    float xRecuperado = b.posicion.x;
    for (int f = 0; f < 15; f++) Paso();
    Comprobar(b.posicion.x < xRecuperado - 0.3f, "Vuelve a moverse tras recuperarse");
    escena.entradas[1].izquierda = false;

    // Poses del atacante durante la animacion.
    PosePersonajeBloque pose = CrearPoseJugadorPrueba(a);
    Comprobar(pose.golpe < 0.0f, "La animacion de golpe termino");
}


static void ProbarSinAturdimientoEncadenado()
{
    // A golpea a B; C intenta golpear a B mientras sigue aturdido.
    const Vector3 pos[3] = { { 0.0f, 0.7f, 0.0f }, { 1.0f, 0.7f, 0.0f }, { 2.1f, 0.7f, 0.0f } };
    Preparar(3, pos);
    escena.jugadores[2].direccionMirada = { -1.0f, 0.0f, 0.0f };
    Asentar();

    JugadorPrueba& b = escena.jugadores[1];

    GolpearYEsperarActivo(0);
    Comprobar(b.tiempoAturdido > 0.0f, "B aturdido por A");

    float restanteAntes = b.tiempoAturdido;
    GolpearYEsperarActivo(2);

    Comprobar(b.tiempoAturdido < restanteAntes, "El golpe de C no reinicia ni suma aturdimiento");
    Comprobar(!escena.jugadores[2].golpeYaConecto, "C no conecta sobre un rival aturdido");

    // Ground pound sobre un aturdido: no lo aplasta.
    escena.jugadores[2].impactoGolpeSuelo = true;
    escena.jugadores[2].posicion = b.posicion;
    escena.jugadores[2].posicion.x += 0.5f;
    ResolverGolpesSuelo(escena.jugadores, escena.participantes, 3, DURACION_APLASTADO_GROUND_POUND);
    Comprobar(!b.aplastado, "Un aturdido no puede quedar ademas aplastado");
}


static void ProbarGolpeFuerteYPose()
{
    const Vector3 pos[2] = { { -4.0f, 0.7f, 0.0f }, { 1.0f, 0.7f, 0.0f } };
    Preparar(2, pos);
    Asentar();

    JugadorPrueba& a = escena.jugadores[0];
    JugadorPrueba& b = escena.jugadores[1];

    // Corre hacia B y golpea antes de llegar.
    escena.entradas[0].derecha = true;
    int f = 0;
    while (b.posicion.x - a.posicion.x > 1.25f && f < 300) { Paso(); f++; }

    float xAntes = b.posicion.x;
    escena.entradas[0].golpear = true;
    Paso();
    escena.entradas[0].golpear = false;
    while (!a.golpeando && f < 400) { Paso(); f++; }
    escena.entradas[0].derecha = false;

    Comprobar(b.derribado, "Golpe corriendo: derriba");
    Comprobar(std::fabs(b.duracionAturdido - DURACION_ATURDIDO_DERRIBO) < 0.001f, "Duracion del derribo");

    // Pose: cae, queda acostado y se levanta.
    bool acostado = false;
    bool levantandose = false;
    float caidaAnterior = 0.0f;

    while (b.tiempoAturdido > 0.0f && f < 800)
    {
        Paso();
        f++;
        if (b.tiempoAturdido <= 0.0f) break;
        PosePersonajeBloque pose = CrearPoseJugadorPrueba(b);
        Comprobar(pose.aturdido, "Pose aturdida mientras dura");
        if (pose.caida >= 0.999f) acostado = true;
        if (acostado && pose.caida < caidaAnterior - 0.001f) levantandose = true;
        caidaAnterior = pose.caida;
    }

    Comprobar(acostado, "Queda acostado durante el derribo");
    Comprobar(levantandose, "Se incorpora al final del derribo");
    Comprobar(CrearPoseJugadorPrueba(b).caida <= 0.0f, "De pie al recuperar el control");

    float desplazamiento = b.posicion.x - xAntes;
    Comprobar(desplazamiento > 1.8f && desplazamiento < 2.8f,
        "Empuje del golpe fuerte mayor que el normal (%.2f m)", desplazamiento);
}


static void ProbarDireccionYParedes()
{
    // Rival en diagonal: el empuje combina la linea entre ambos y la
    // orientacion del atacante (mira hacia +X).
    const Vector3 pos[2] = { { 0.0f, 0.7f, 0.0f }, { 0.8f, 0.7f, 0.8f } };
    Preparar(2, pos);
    Asentar();

    JugadorPrueba& b = escena.jugadores[1];
    GolpearYEsperarActivo(0);

    Comprobar(b.tiempoAturdido > 0.0f, "Golpe en diagonal conecta");
    Comprobar(b.direccionGolpeRecibido.x > b.direccionGolpeRecibido.z && b.direccionGolpeRecibido.z > 0.2f,
        "Direccion entre la posicion relativa y la mirada (%.2f, %.2f)",
        b.direccionGolpeRecibido.x, b.direccionGolpeRecibido.z);

    // Pared fina detras del rival: no la atraviesa.
    const Vector3 pos2[2] = { { 0.0f, 0.7f, 0.0f }, { 1.0f, 0.7f, 0.0f } };
    Preparar(2, pos2);
    AgregarBloquePrueba(escena.bloques, escena.cantidadBloques, MAX_BLOQUES, { 2.0f, 1.0f, 0.0f }, { 0.15f, 2.0f, 6.0f }, GRAY);
    Asentar();

    JugadorPrueba& r = escena.jugadores[1];
    const float caraPared = 2.0f - 0.075f;
    bool atraveso = false;
    bool conecto = false;

    escena.entradas[0].golpear = true;
    for (int f = 0; f < 120; f++)
    {
        Paso();
        escena.entradas[0].golpear = false;
        if (r.tiempoAturdido > 0.0f) conecto = true;
        if (r.posicion.x + r.tamano.x * 0.5f > caraPared + 0.01f) atraveso = true;
    }

    Comprobar(conecto, "El golpe contra la pared conecto");
    Comprobar(!atraveso, "El empujado no atraviesa la pared");
    Comprobar(r.enSuelo && !r.cayendo, "Sigue apoyado tras chocar con la pared");
}


int main()
{
    SetTraceLogLevel(LOG_WARNING);

    ProbarAnticipacionYGolpeNormal();
    ProbarSinAturdimientoEncadenado();
    ProbarGolpeFuerteYPose();
    ProbarDireccionYParedes();

    std::printf("Verificacion Combate: %d errores\n", errores);
    return errores == 0 ? 0 : 1;
}
