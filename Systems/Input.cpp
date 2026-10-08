#include "raylib.h"
#include "Systems/Input.h"

#include <cmath>


//==================================================
// ZONA MUERTA DEL STICK
//==================================================

static float zonaMuertaStick =
    ZONA_MUERTA_STICK_DEFECTO / 100.0f;


void EstablecerZonaMuertaStick(
    float fraccion
)
{
    const float minima = ZONA_MUERTA_STICK_MINIMA / 100.0f;
    const float maxima = ZONA_MUERTA_STICK_MAXIMA / 100.0f;

    zonaMuertaStick =
        fraccion < minima ? minima : (fraccion > maxima ? maxima : fraccion);
}


float ObtenerZonaMuertaStick()
{
    return zonaMuertaStick;
}


Vector2 LeerStickIzquierdoCrudo(
    int indiceGamepad
)
{
    if (indiceGamepad < 0 || !IsGamepadAvailable(indiceGamepad))
    {
        return Vector2{ 0.0f, 0.0f };
    }

    return Vector2{
        GetGamepadAxisMovement(indiceGamepad, GAMEPAD_AXIS_LEFT_X),
        GetGamepadAxisMovement(indiceGamepad, GAMEPAD_AXIS_LEFT_Y)
    };
}


Vector2 LeerStickIzquierdo(
    int indiceGamepad
)
{
    return FiltrarZonaMuertaStick(
        LeerStickIzquierdoCrudo(indiceGamepad)
    );
}


Vector2 FiltrarZonaMuertaStick(
    Vector2 eje
)
{
    float magnitud = std::sqrt(eje.x * eje.x + eje.y * eje.y);

    if (magnitud <= zonaMuertaStick)
    {
        return Vector2{ 0.0f, 0.0f };
    }

    // Reescala (zona .. 1) a (0 .. 1) conservando la direccion.
    float util = magnitud > 1.0f ? 1.0f : magnitud;
    float escala = (util - zonaMuertaStick) / (1.0f - zonaMuertaStick) / magnitud;

    return Vector2{ eje.x * escala, eje.y * escala };
}


// Umbral de direccion para entradas digitales (minijuegos, menus): con la
// direccion normalizada, 0.38 ~ sen(22.5 grados) divide el circulo en 8
// sectores, asi una diagonal activa los dos ejes y un eje casi puro solo uno.
static const float UMBRAL_DIRECCION_DIGITAL = 0.38f;



static InputJugador LeerInputTeclado(
    TipoControl control
)
{
    InputJugador input{};

    if (
        control == CONTROL_TECLADO_COMPLETO ||
        control == CONTROL_TECLADO_WASD
    )
    {
        if (IsKeyDown(KEY_W)) input.moverZ -= 1.0f;
        if (IsKeyDown(KEY_S)) input.moverZ += 1.0f;
        if (IsKeyDown(KEY_A)) input.moverX -= 1.0f;
        if (IsKeyDown(KEY_D)) input.moverX += 1.0f;
    }

    if (
        control == CONTROL_TECLADO_COMPLETO ||
        control == CONTROL_TECLADO_FLECHAS
    )
    {
        if (IsKeyDown(KEY_UP)) input.moverZ -= 1.0f;
        if (IsKeyDown(KEY_DOWN)) input.moverZ += 1.0f;
        if (IsKeyDown(KEY_LEFT)) input.moverX -= 1.0f;
        if (IsKeyDown(KEY_RIGHT)) input.moverX += 1.0f;
    }

    if (control == CONTROL_TECLADO_FLECHAS)
    {
        input.saltar = IsKeyPressed(KEY_ENTER);
        input.accion = IsKeyPressed(KEY_RIGHT_SHIFT);
    }
    else
    {
        input.saltar = IsKeyPressed(KEY_SPACE);
        input.accion = IsKeyPressed(KEY_E);
    }

    return input;
}


static InputJugador LeerInputGamepad(
    int indiceGamepad
)
{
    InputJugador input{};

    if (!IsGamepadAvailable(indiceGamepad))
    {
        return input;
    }

    Vector2 stick = LeerStickIzquierdo(indiceGamepad);

    input.moverX = stick.x;
    input.moverZ = stick.y;

    input.saltar = IsGamepadButtonPressed(
        indiceGamepad,
        GAMEPAD_BUTTON_RIGHT_FACE_DOWN
    );

    input.accion = IsGamepadButtonPressed(
        indiceGamepad,
        GAMEPAD_BUTTON_RIGHT_FACE_LEFT
    );

    return input;
}


void ActualizarConexionParticipante(
    Participante& participante
)
{
    if (participante.esBot)
    {
        participante.conectado = true;
        return;
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        participante.conectado =
            participante.indiceGamepad >= 0 &&
            IsGamepadAvailable(
                participante.indiceGamepad
            );

        return;
    }

    participante.conectado =
        participante.control == CONTROL_TECLADO_COMPLETO ||
        participante.control == CONTROL_TECLADO_WASD ||
        participante.control == CONTROL_TECLADO_FLECHAS;
}


void ActualizarConexionesParticipantes(
    Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        ActualizarConexionParticipante(
            participantes[i]
        );
    }
}


void ConfigurarControlesParticipantes(
    Participante participantes[],
    int cantidadMaxima,
    ModoTeclado modoTeclado
)
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    int cantidadTeclados =
        modoTeclado == TECLADO_DIVIDIDO
        ? 2
        : 1;

    for (int i = 0; i < limite; i++)
    {
        Participante& participante =
            participantes[i];

        participante.esBot = false;
        participante.indiceGamepad = -1;

        if (modoTeclado == TECLADO_COMPLETO && i == 0)
        {
            participante.control =
                CONTROL_TECLADO_COMPLETO;
        }
        else if (modoTeclado == TECLADO_DIVIDIDO && i == 0)
        {
            participante.control =
                CONTROL_TECLADO_WASD;
        }
        else if (modoTeclado == TECLADO_DIVIDIDO && i == 1)
        {
            participante.control =
                CONTROL_TECLADO_FLECHAS;
        }
        else
        {
            participante.control =
                CONTROL_GAMEPAD;

            participante.indiceGamepad =
                i - cantidadTeclados;
        }

        ActualizarConexionParticipante(
            participante
        );
    }
}


InputJugador LeerInputParticipante(
    const Participante& participante
)
{
    if (
        participante.esBot ||
        !participante.conectado
    )
    {
        return InputJugador{};
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        return LeerInputGamepad(
            participante.indiceGamepad
        );
    }

    if (
        participante.control == CONTROL_TECLADO_COMPLETO ||
        participante.control == CONTROL_TECLADO_WASD ||
        participante.control == CONTROL_TECLADO_FLECHAS
    )
    {
        return LeerInputTeclado(
            participante.control
        );
    }

    return InputJugador{};
}


InputSeleccionParticipante LeerInputSeleccionParticipante(
    const Participante& participante
)
{
    InputSeleccionParticipante entrada{};

    if (
        participante.esBot ||
        !participante.conectado
    )
    {
        return entrada;
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        int gamepad = participante.indiceGamepad;

        // Navegacion de menu: exige una inclinacion clara (stick ya
        // filtrado por la zona muerta).
        Vector2 stick = LeerStickIzquierdo(gamepad);

        entrada.izquierda =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_LEFT
            ) || stick.x < -0.40f;

        entrada.derecha =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_RIGHT
            ) || stick.x > 0.40f;

        entrada.arriba =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_UP
            ) || stick.y < -0.40f;

        entrada.abajo =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_DOWN
            ) || stick.y > 0.40f;

        entrada.confirmar =
            IsGamepadButtonPressed(
                gamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_DOWN
            );

        entrada.cancelar =
            IsGamepadButtonPressed(
                gamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
            );

        return entrada;
    }

    bool usaWASD =
        participante.control == CONTROL_TECLADO_COMPLETO ||
        participante.control == CONTROL_TECLADO_WASD;

    bool usaFlechas =
        participante.control == CONTROL_TECLADO_COMPLETO ||
        participante.control == CONTROL_TECLADO_FLECHAS;

    if (usaWASD)
    {
        entrada.izquierda = IsKeyPressed(KEY_A);
        entrada.derecha = IsKeyPressed(KEY_D);
        entrada.arriba = IsKeyPressed(KEY_W);
        entrada.abajo = IsKeyPressed(KEY_S);
    }

    if (usaFlechas)
    {
        entrada.izquierda =
            entrada.izquierda || IsKeyPressed(KEY_LEFT);

        entrada.derecha =
            entrada.derecha || IsKeyPressed(KEY_RIGHT);

        entrada.arriba =
            entrada.arriba || IsKeyPressed(KEY_UP);

        entrada.abajo =
            entrada.abajo || IsKeyPressed(KEY_DOWN);
    }

    if (participante.control == CONTROL_TECLADO_FLECHAS)
    {
        entrada.confirmar = IsKeyPressed(KEY_ENTER);
        entrada.cancelar = IsKeyPressed(KEY_RIGHT_SHIFT);
    }
    else if (usaWASD)
    {
        entrada.confirmar = IsKeyPressed(KEY_SPACE);
        entrada.cancelar = IsKeyPressed(KEY_BACKSPACE);
    }

    return entrada;
}


InputMinijuegoParticipante LeerInputMinijuegoParticipante(
    const Participante& participante
)
{
    InputMinijuegoParticipante entrada{};

    if (
        participante.esBot ||
        !participante.conectado
    )
    {
        return entrada;
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        int gamepad = participante.indiceGamepad;

        // Movimiento digital de 8 direcciones: fuera de la zona muerta se
        // usa la DIRECCION del stick (no la magnitud por eje).
        Vector2 stick = LeerStickIzquierdo(gamepad);
        float magnitud = std::sqrt(stick.x * stick.x + stick.y * stick.y);
        float dirX = magnitud > 0.0f ? stick.x / magnitud : 0.0f;
        float dirY = magnitud > 0.0f ? stick.y / magnitud : 0.0f;

        entrada.izquierda =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_LEFT
            ) || dirX < -UMBRAL_DIRECCION_DIGITAL;

        entrada.derecha =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_RIGHT
            ) || dirX > UMBRAL_DIRECCION_DIGITAL;

        entrada.adelante =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_UP
            ) || dirY < -UMBRAL_DIRECCION_DIGITAL;

        entrada.atras =
            IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_LEFT_FACE_DOWN
            ) || dirY > UMBRAL_DIRECCION_DIGITAL;

        entrada.saltar = IsGamepadButtonPressed(
            gamepad,
            GAMEPAD_BUTTON_RIGHT_FACE_DOWN
        );

        entrada.golpear = IsGamepadButtonPressed(
            gamepad,
            GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
        );

        return entrada;
    }

    bool usaWASD =
        participante.control == CONTROL_TECLADO_COMPLETO ||
        participante.control == CONTROL_TECLADO_WASD;

    bool usaFlechas =
        participante.control == CONTROL_TECLADO_COMPLETO ||
        participante.control == CONTROL_TECLADO_FLECHAS;

    if (usaWASD)
    {
        entrada.izquierda = IsKeyDown(KEY_A);
        entrada.derecha = IsKeyDown(KEY_D);
        entrada.adelante = IsKeyDown(KEY_W);
        entrada.atras = IsKeyDown(KEY_S);
    }

    if (usaFlechas)
    {
        entrada.izquierda =
            entrada.izquierda || IsKeyDown(KEY_LEFT);

        entrada.derecha =
            entrada.derecha || IsKeyDown(KEY_RIGHT);

        entrada.adelante =
            entrada.adelante || IsKeyDown(KEY_UP);

        entrada.atras =
            entrada.atras || IsKeyDown(KEY_DOWN);
    }

    if (participante.control == CONTROL_TECLADO_FLECHAS)
    {
        entrada.saltar = IsKeyPressed(KEY_ENTER);
        entrada.golpear = IsKeyPressed(KEY_RIGHT_SHIFT);
    }
    else if (usaWASD)
    {
        entrada.saltar = IsKeyPressed(KEY_SPACE);
        entrada.golpear = IsKeyPressed(KEY_E);
    }

    return entrada;
}


bool AccionDireccionalControlPresionada(
    const Participante& participante,
    AccionDireccionalControl accion
)
{
    if (
        participante.esBot ||
        !participante.conectado ||
        participante.control == CONTROL_NINGUNO
    )
    {
        return false;
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        int boton = GAMEPAD_BUTTON_UNKNOWN;

        switch (accion)
        {
            case CONTROL_DIRECCION_ARRIBA:
                boton = GAMEPAD_BUTTON_RIGHT_FACE_UP;
                break;

            case CONTROL_DIRECCION_ABAJO:
                boton = GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
                break;

            case CONTROL_DIRECCION_IZQUIERDA:
                boton = GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
                break;

            case CONTROL_DIRECCION_DERECHA:
                boton = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
                break;
        }

        return IsGamepadButtonPressed(
            participante.indiceGamepad,
            boton
        );
    }

    bool usaFlechas =
        participante.control == CONTROL_TECLADO_FLECHAS;

    switch (accion)
    {
        case CONTROL_DIRECCION_ARRIBA:
            return IsKeyPressed(usaFlechas ? KEY_UP : KEY_W);

        case CONTROL_DIRECCION_ABAJO:
            return IsKeyPressed(usaFlechas ? KEY_DOWN : KEY_S);

        case CONTROL_DIRECCION_IZQUIERDA:
            return IsKeyPressed(usaFlechas ? KEY_LEFT : KEY_A);

        case CONTROL_DIRECCION_DERECHA:
            return IsKeyPressed(usaFlechas ? KEY_RIGHT : KEY_D);
    }

    return false;
}


const char* ObtenerTextoAccionDireccionalControl(
    const Participante& participante,
    AccionDireccionalControl accion
)
{
    if (participante.esBot)
    {
        return "BOT";
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        switch (accion)
        {
            case CONTROL_DIRECCION_ARRIBA: return "Y";
            case CONTROL_DIRECCION_ABAJO: return "A";
            case CONTROL_DIRECCION_IZQUIERDA: return "X";
            case CONTROL_DIRECCION_DERECHA: return "B";
        }
    }

    if (participante.control == CONTROL_TECLADO_FLECHAS)
    {
        switch (accion)
        {
            case CONTROL_DIRECCION_ARRIBA: return "FLECHA ARRIBA";
            case CONTROL_DIRECCION_ABAJO: return "FLECHA ABAJO";
            case CONTROL_DIRECCION_IZQUIERDA: return "FLECHA IZQ";
            case CONTROL_DIRECCION_DERECHA: return "FLECHA DER";
        }
    }

    switch (accion)
    {
        case CONTROL_DIRECCION_ARRIBA: return "W";
        case CONTROL_DIRECCION_ABAJO: return "S";
        case CONTROL_DIRECCION_IZQUIERDA: return "A";
        case CONTROL_DIRECCION_DERECHA: return "D";
    }

    return "?";
}


const char* ObtenerTextoBotonPrincipal(
    const Participante& participante
)
{
    if (participante.esBot)
    {
        return "BOT";
    }

    if (participante.control == CONTROL_GAMEPAD)
    {
        return "A";
    }

    if (participante.control == CONTROL_TECLADO_FLECHAS)
    {
        return "ENTER";
    }

    return "ESPACIO";
}


const char* ObtenerNombreControlParticipante(
    const Participante& participante
)
{
    if (participante.esBot)
    {
        return "BOT";
    }

    switch (participante.control)
    {
        case CONTROL_TECLADO_COMPLETO:
            return "TECLADO";

        case CONTROL_TECLADO_WASD:
            return "TECLADO WASD";

        case CONTROL_TECLADO_FLECHAS:
            return "TECLADO FLECHAS";

        case CONTROL_GAMEPAD:
            return TextFormat(
                "MANDO %d",
                participante.indiceGamepad + 1
            );

        case CONTROL_NINGUNO:
            break;
    }

    return "SIN CONTROL";
}


InputJugador LeerInputJugador(
    int jugador
)
{
    Participante participante{};

    if (jugador == 0)
    {
        participante.control =
            CONTROL_TECLADO_FLECHAS;

        participante.conectado =
            true;
    }
    else
    {
        participante.control =
            CONTROL_GAMEPAD;

        participante.indiceGamepad =
            jugador - 1;

        ActualizarConexionParticipante(
            participante
        );
    }

    InputJugador input = LeerInputParticipante(
        participante
    );

    if (jugador == 0)
    {
        input.saltar =
            IsKeyPressed(KEY_SPACE);

        input.accion =
            IsKeyPressed(KEY_E);
    }

    return input;
}
