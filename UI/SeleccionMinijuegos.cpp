#include "UI/SeleccionMinijuegos.h"

#include "Systems/Input.h"
#include "UI/MiniaturasMinijuegos.h"


static Rectangle ObtenerAreaCatalogo()
{
    return Rectangle
    {
        45.0f,
        150.0f,
        GetScreenWidth() - 460.0f,
        430.0f
    };
}


static Rectangle ObtenerCeldaCatalogo(
    int indice
)
{
    Rectangle area = ObtenerAreaCatalogo();

    const int columnas = 8;
    const int filas =
        (CANTIDAD_MINIJUEGOS + columnas - 1) /
        columnas;

    const float separacionX = 11.0f;
    const float separacionY = 10.0f;

    float anchoCelda =
        (area.width - separacionX * (columnas - 1)) /
        (float)columnas;

    float altoCelda =
        (area.height - separacionY * (filas - 1)) /
        (float)filas;

    int columna = indice % columnas;
    int fila = indice / columnas;

    return Rectangle
    {
        area.x + columna * (anchoCelda + separacionX),
        area.y + fila * (altoCelda + separacionY),
        anchoCelda,
        altoCelda
    };
}


static int MoverIndice(
    int actual,
    int deltaX,
    int deltaY
)
{
    const int columnas = 8;
    const int filas =
        (CANTIDAD_MINIJUEGOS + columnas - 1) /
        columnas;

    int columna = actual % columnas;
    int fila = actual / columnas;

    columna += deltaX;
    fila += deltaY;

    if (columna < 0) columna = columnas - 1;
    if (columna >= columnas) columna = 0;
    if (fila < 0) fila = filas - 1;
    if (fila >= filas) fila = 0;

    int candidato = fila * columnas + columna;

    if (candidato >= CANTIDAD_MINIJUEGOS)
    {
        candidato = CANTIDAD_MINIJUEGOS - 1;
    }

    return candidato;
}


void SeleccionMinijuegos::Inicializar()
{
    indiceSeleccionado = 0;
    indiceAnterior = -1;
    confirmado = false;
    volver = false;
    progresoPanel = 0.0f;
}


void SeleccionMinijuegos::Actualizar(
    float deltaTime,
    const Participante& jugadorUno
)
{
    confirmado = false;
    volver = false;

    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(jugadorUno);

    int nuevoIndice = indiceSeleccionado;

    if (entrada.izquierda)
        nuevoIndice = MoverIndice(nuevoIndice, -1, 0);

    if (entrada.derecha)
        nuevoIndice = MoverIndice(nuevoIndice, 1, 0);

    if (entrada.arriba)
        nuevoIndice = MoverIndice(nuevoIndice, 0, -1);

    if (entrada.abajo)
        nuevoIndice = MoverIndice(nuevoIndice, 0, 1);

    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        Rectangle celda = ObtenerCeldaCatalogo(i);

        if (CheckCollisionPointRec(mouse, celda))
        {
            nuevoIndice = i;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                confirmado = true;
            }
        }
    }

    if (nuevoIndice != indiceSeleccionado)
    {
        indiceAnterior = indiceSeleccionado;
        indiceSeleccionado = nuevoIndice;
        progresoPanel = 0.0f;
    }

    progresoPanel += deltaTime * 6.0f;
    if (progresoPanel > 1.0f) progresoPanel = 1.0f;

    if (entrada.confirmar)
    {
        confirmado = true;
    }

    if (entrada.cancelar || IsKeyPressed(KEY_ESCAPE))
    {
        volver = true;
    }
}


void SeleccionMinijuegos::Dibujar(
    const Participante& jugadorUno
) const
{
    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(Color{ 15, 17, 23, 255 }, 0.60f)
    );

    const char* titulo = "MINIJUEGOS";

    DrawText(
        titulo,
        GetScreenWidth() / 2 - MeasureText(titulo, 40) / 2,
        42,
        40,
        RAYWHITE
    );

    Rectangle area = ObtenerAreaCatalogo();

    DrawRectangle(
        (int)area.x - 14,
        (int)area.y - 18,
        (int)area.width + 28,
        (int)area.height + 36,
        Fade(BLACK, 0.58f)
    );

    DrawRectangleLinesEx(
        {
            area.x - 14,
            area.y - 18,
            area.width + 28,
            area.height + 36
        },
        3.0f,
        Fade(RAYWHITE, 0.70f)
    );

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        Rectangle celda = ObtenerCeldaCatalogo(i);
        DibujarMiniaturaMinijuego(
            ObtenerIdMinijuegoPorIndice(i),
            celda
        );

        DrawRectangle(
            (int)celda.x,
            (int)(celda.y + celda.height - 36.0f),
            (int)celda.width,
            36,
            Fade(BLACK, 0.72f)
        );

        int tamanoNombre = 11;
        while (
            tamanoNombre > 8 &&
            MeasureText(ObtenerDatosMinijuegoPorIndice(i).nombre, tamanoNombre) >
                celda.width - 6.0f
        )
        {
            tamanoNombre--;
        }

        int anchoNombre =
            MeasureText(ObtenerDatosMinijuegoPorIndice(i).nombre, tamanoNombre);

        DrawText(
            ObtenerDatosMinijuegoPorIndice(i).nombre,
            (int)(celda.x + celda.width / 2.0f - anchoNombre / 2.0f),
            (int)(celda.y + celda.height - 25.0f),
            tamanoNombre,
            RAYWHITE
        );

        DrawRectangleLinesEx(
            celda,
            i == indiceSeleccionado ? 5.0f : 1.5f,
            i == indiceSeleccionado
                ? RED
                : Fade(RAYWHITE, 0.55f)
        );
    }

    float suavizado =
        1.0f -
        (1.0f - progresoPanel) *
        (1.0f - progresoPanel);

    float anchoPanel = 350.0f;
    float xDestino = GetScreenWidth() - anchoPanel - 22.0f;
    float xPanel =
        GetScreenWidth() + 20.0f -
        (GetScreenWidth() + 20.0f - xDestino) * suavizado;

    Rectangle panel =
    {
        xPanel,
        130.0f,
        anchoPanel,
        330.0f
    };

    DrawRectangle(
        (int)panel.x,
        (int)panel.y,
        (int)panel.width,
        (int)panel.height,
        Fade(Color{ 20, 22, 29, 255 }, 0.94f)
    );

    DrawRectangleLinesEx(panel, 4.0f, RED);

    const DatosMinijuegoCatalogo& seleccionado =
        ObtenerDatosMinijuegoPorIndice(indiceSeleccionado);

    DrawText(
        seleccionado.nombre,
        (int)panel.x + 24,
        (int)panel.y + 34,
        25,
        RAYWHITE
    );

    DrawText(
        "DESCRIPCION",
        (int)panel.x + 24,
        (int)panel.y + 91,
        18,
        ORANGE
    );

    const char* texto = seleccionado.descripcion;
    int inicio = 0;
    int largo = (int)TextLength(texto);
    int y = (int)panel.y + 124;

    while (inicio < largo)
    {
        int fin = inicio;
        int ultimoEspacio = -1;

        while (fin < largo)
        {
            if (texto[fin] == ' ')
            {
                ultimoEspacio = fin;
            }

            char linea[128]{};
            int cantidad = fin - inicio + 1;
            if (cantidad > 126) cantidad = 126;

            for (int k = 0; k < cantidad; k++)
            {
                linea[k] = texto[inicio + k];
            }

            linea[cantidad] = '\0';

            if (MeasureText(linea, 18) > 296)
            {
                break;
            }

            fin++;
        }

        if (fin < largo && ultimoEspacio >= inicio)
        {
            fin = ultimoEspacio;
        }

        if (fin <= inicio)
        {
            fin = inicio + 1;
        }

        char lineaFinal[128]{};
        int cantidadFinal =
            fin == largo
                ? largo - inicio
                : fin - inicio;

        if (cantidadFinal > 126) cantidadFinal = 126;

        for (int k = 0; k < cantidadFinal; k++)
        {
            lineaFinal[k] = texto[inicio + k];
        }

        lineaFinal[cantidadFinal] = '\0';

        DrawText(
            lineaFinal,
            (int)panel.x + 24,
            y,
            18,
            LIGHTGRAY
        );

        y += 26;
        inicio = fin;

        while (inicio < largo && texto[inicio] == ' ')
        {
            inicio++;
        }
    }

    DrawText(
        TextFormat(
            "J1: %s",
            ObtenerNombreControlParticipante(jugadorUno)
        ),
        45,
        GetScreenHeight() - 82,
        19,
        jugadorUno.color
    );

    DrawText(
        "J1 ELIGE | MOVER CURSOR O MOUSE | CONFIRMAR PARA JUGAR | ESC VOLVER",
        45,
        GetScreenHeight() - 52,
        18,
        LIGHTGRAY
    );
}
