#include "Editor/EditorMapas.h"

#include "raygizmo.h"
#include "raymath.h"

#include <cmath>
#include <cstring>


static const char* RUTA_MAPA_FABRICA_67 =
    "Mapas/Minijuego67.map";


static bool EsMarcadorCamaraPos(
    const ObjetoMapaEditor& objeto
)
{
    return std::strcmp(objeto.nombre, "CamaraPos") == 0;
}


static bool TransformIgualEditor(
    const Transform& a,
    const Transform& b
)
{
    const float epsilon = 0.0001f;

    return
        std::fabs(a.translation.x - b.translation.x) < epsilon &&
        std::fabs(a.translation.y - b.translation.y) < epsilon &&
        std::fabs(a.translation.z - b.translation.z) < epsilon &&
        std::fabs(a.rotation.x - b.rotation.x) < epsilon &&
        std::fabs(a.rotation.y - b.rotation.y) < epsilon &&
        std::fabs(a.rotation.z - b.rotation.z) < epsilon &&
        std::fabs(a.rotation.w - b.rotation.w) < epsilon &&
        std::fabs(a.scale.x - b.scale.x) < epsilon &&
        std::fabs(a.scale.y - b.scale.y) < epsilon &&
        std::fabs(a.scale.z - b.scale.z) < epsilon;
}


static void RegistrarPasoDeshacer(
    EditorMapas& editor,
    int indiceObjeto,
    const Transform& transformAnterior
)
{
    if (
        indiceObjeto < 0 ||
        indiceObjeto >= editor.mapa.cantidadObjetos
    )
    {
        return;
    }

    if (
        editor.cantidadHistorialDeshacer >=
        MAX_HISTORIAL_DESHACER_EDITOR
    )
    {
        for (int i = 1; i < MAX_HISTORIAL_DESHACER_EDITOR; i++)
        {
            editor.historialDeshacer[i - 1] =
                editor.historialDeshacer[i];
        }

        editor.cantidadHistorialDeshacer =
            MAX_HISTORIAL_DESHACER_EDITOR - 1;
    }

    PasoDeshacerEditor& paso =
        editor.historialDeshacer[
            editor.cantidadHistorialDeshacer
        ];

    paso.indiceObjeto = indiceObjeto;
    paso.transformAnterior = transformAnterior;

    editor.cantidadHistorialDeshacer++;
}


static bool DeshacerUltimaAccion(
    EditorMapas& editor
)
{
    if (editor.cantidadHistorialDeshacer <= 0)
    {
        return false;
    }

    editor.cantidadHistorialDeshacer--;

    const PasoDeshacerEditor& paso =
        editor.historialDeshacer[
            editor.cantidadHistorialDeshacer
        ];

    if (
        paso.indiceObjeto < 0 ||
        paso.indiceObjeto >= editor.mapa.cantidadObjetos
    )
    {
        return false;
    }

    editor.mapa.objetos[paso.indiceObjeto].transform =
        paso.transformAnterior;

    editor.indiceSeleccionado = paso.indiceObjeto;
    editor.cambiosSinGuardar = true;

    return true;
}


static float DimensionEditor(
    float valor,
    float minimo
)
{
    float resultado = std::fabs(valor);

    if (resultado < minimo)
    {
        resultado = minimo;
    }

    return resultado;
}


static const char* ObtenerNombreTipoObjeto(
    TipoObjetoMapaEditor tipo
)
{
    switch (tipo)
    {
        case OBJETO_MAPA_EDITOR_CINTA_67:
            return "CINTA 67";

        case OBJETO_MAPA_EDITOR_MESA_67:
            return "MESA 67";

        case OBJETO_MAPA_EDITOR_CUBO:
        default:
            return "CUBO";
    }
}


static const ObjetoMapaEditor* BuscarObjetoMapa(
    const MapaEditor& mapa,
    const char* nombre
)
{
    if (nombre == nullptr)
    {
        return nullptr;
    }

    for (int i = 0; i < mapa.cantidadObjetos; i++)
    {
        if (std::strcmp(mapa.objetos[i].nombre, nombre) == 0)
        {
            return &mapa.objetos[i];
        }
    }

    return nullptr;
}


static BoundingBox CrearCajaSeleccionEditor(
    const ObjetoMapaEditor& objeto
)
{
    Vector3 mitad =
    {
        std::fabs(objeto.transform.scale.x) * 0.5f,
        std::fabs(objeto.transform.scale.y) * 0.5f,
        std::fabs(objeto.transform.scale.z) * 0.5f
    };

    if (EsMarcadorCamaraPos(objeto))
    {
        mitad = { 0.40f, 0.40f, 0.40f };
    }
    else if (objeto.tipo == OBJETO_MAPA_EDITOR_CINTA_67)
    {
        mitad.x += 0.45f;
        if (mitad.y < 1.15f) mitad.y = 1.15f;
        mitad.z += 0.18f;
    }
    else if (objeto.tipo == OBJETO_MAPA_EDITOR_MESA_67)
    {
        if (mitad.y < 0.60f) mitad.y = 0.60f;
    }

    if (mitad.x < 0.05f) mitad.x = 0.05f;
    if (mitad.y < 0.05f) mitad.y = 0.05f;
    if (mitad.z < 0.05f) mitad.z = 0.05f;

    return
    {
        Vector3Subtract(
            objeto.transform.translation,
            mitad
        ),
        Vector3Add(
            objeto.transform.translation,
            mitad
        )
    };
}


static int ObtenerFlagsGizmo(
    ModoGizmoEditor modo
)
{
    switch (modo)
    {
        case GIZMO_EDITOR_ROTAR:
            return GIZMO_ROTATE;

        case GIZMO_EDITOR_ESCALAR:
            return GIZMO_SCALE | GIZMO_LOCAL;

        case GIZMO_EDITOR_TRASLADAR:
        default:
            return GIZMO_TRANSLATE;
    }
}


static const char* ObtenerNombreModoGizmo(
    ModoGizmoEditor modo
)
{
    switch (modo)
    {
        case GIZMO_EDITOR_ROTAR:
            return "ROTAR";

        case GIZMO_EDITOR_ESCALAR:
            return "ESCALAR";

        case GIZMO_EDITOR_TRASLADAR:
        default:
            return "TRASLADAR";
    }
}


static void ActualizarDireccionCamara(
    EditorMapas& editor
)
{
    const float cosPitch =
        std::cos(editor.pitchCamara);

    Vector3 direccion =
    {
        std::sin(editor.yawCamara) * cosPitch,
        std::sin(editor.pitchCamara),
        std::cos(editor.yawCamara) * cosPitch
    };

    editor.camara.target =
        Vector3Add(
            editor.camara.position,
            Vector3Scale(direccion, 10.0f)
        );
}


static void ActualizarCamaraEditor(
    EditorMapas& editor,
    float deltaTime
)
{
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Vector2 deltaMouse =
            GetMouseDelta();

        editor.yawCamara -=
            deltaMouse.x * 0.0045f;

        editor.pitchCamara -=
            deltaMouse.y * 0.0045f;

        if (editor.pitchCamara > 1.45f)
        {
            editor.pitchCamara = 1.45f;
        }

        if (editor.pitchCamara < -1.45f)
        {
            editor.pitchCamara = -1.45f;
        }

        ActualizarDireccionCamara(editor);
    }

    Vector3 direccion =
        Vector3Normalize(
            Vector3Subtract(
                editor.camara.target,
                editor.camara.position
            )
        );

    float rueda = GetMouseWheelMove();

    if (std::fabs(rueda) > 0.001f)
    {
        Vector3 zoom =
            Vector3Scale(
                direccion,
                rueda * 1.60f
            );

        editor.camara.position =
            Vector3Add(
                editor.camara.position,
                zoom
            );

        editor.camara.target =
            Vector3Add(
                editor.camara.target,
                zoom
            );
    }

    Vector3 adelante =
    {
        direccion.x,
        0.0f,
        direccion.z
    };

    if (Vector3Length(adelante) < 0.001f)
    {
        adelante = { 0.0f, 0.0f, -1.0f };
    }
    else
    {
        adelante = Vector3Normalize(adelante);
    }

    Vector3 derecha =
        Vector3Normalize(
            Vector3CrossProduct(
                adelante,
                { 0.0f, 1.0f, 0.0f }
            )
        );

    float velocidad =
        IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT)
        ? 10.0f
        : 4.5f;

    Vector3 movimiento{};

    if (IsKeyDown(KEY_W))
    {
        movimiento =
            Vector3Add(movimiento, adelante);
    }

    if (IsKeyDown(KEY_S))
    {
        movimiento =
            Vector3Subtract(movimiento, adelante);
    }

    if (IsKeyDown(KEY_D))
    {
        movimiento =
            Vector3Add(movimiento, derecha);
    }

    if (IsKeyDown(KEY_A))
    {
        movimiento =
            Vector3Subtract(movimiento, derecha);
    }

    if (IsKeyDown(KEY_E))
    {
        movimiento.y += 1.0f;
    }

    if (IsKeyDown(KEY_Q))
    {
        movimiento.y -= 1.0f;
    }

    if (Vector3Length(movimiento) > 0.001f)
    {
        movimiento =
            Vector3Scale(
                Vector3Normalize(movimiento),
                velocidad * deltaTime
            );

        editor.camara.position =
            Vector3Add(
                editor.camara.position,
                movimiento
            );

        editor.camara.target =
            Vector3Add(
                editor.camara.target,
                movimiento
            );
    }
}


static void SeleccionarObjetoConMouse(
    EditorMapas& editor
)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return;
    }

    Ray rayo =
        GetScreenToWorldRay(
            GetMousePosition(),
            editor.camara
        );

    int mejorIndice = -1;
    float menorDistancia = 1000000.0f;

    for (int i = 0; i < editor.mapa.cantidadObjetos; i++)
    {
        BoundingBox caja =
            CrearCajaSeleccionEditor(
                editor.mapa.objetos[i]
            );

        RayCollision colision =
            GetRayCollisionBox(
                rayo,
                caja
            );

        if (
            colision.hit &&
            colision.distance < menorDistancia
        )
        {
            menorDistancia = colision.distance;
            mejorIndice = i;
        }
    }

    if (mejorIndice >= 0)
    {
        editor.indiceSeleccionado =
            mejorIndice;
    }
}


static void DibujarSegmentoNumeroEditor(
    Vector3 posicion,
    bool horizontal,
    float escala,
    Color color
)
{
    DrawCube(
        posicion,
        (horizontal ? 0.62f : 0.13f) * escala,
        (horizontal ? 0.13f : 0.52f) * escala,
        0.20f * escala,
        color
    );
}


static void DibujarNumero3DEditor(
    int numero,
    Vector3 centro,
    float escala,
    Color color
)
{
    const float xLado = 0.31f * escala;
    const float yExtremo = 0.55f * escala;
    const float yMedio = 0.275f * escala;

    bool segmentos[7]{};

    if (numero == 6)
    {
        segmentos[0] = true;
        segmentos[2] = true;
        segmentos[3] = true;
        segmentos[4] = true;
        segmentos[5] = true;
        segmentos[6] = true;
    }
    else
    {
        segmentos[0] = true;
        segmentos[1] = true;
        segmentos[2] = true;
    }

    if (segmentos[0]) DibujarSegmentoNumeroEditor({ centro.x, centro.y + yExtremo, centro.z }, true, escala, color);
    if (segmentos[1]) DibujarSegmentoNumeroEditor({ centro.x + xLado, centro.y + yMedio, centro.z }, false, escala, color);
    if (segmentos[2]) DibujarSegmentoNumeroEditor({ centro.x + xLado, centro.y - yMedio, centro.z }, false, escala, color);
    if (segmentos[3]) DibujarSegmentoNumeroEditor({ centro.x, centro.y - yExtremo, centro.z }, true, escala, color);
    if (segmentos[4]) DibujarSegmentoNumeroEditor({ centro.x - xLado, centro.y - yMedio, centro.z }, false, escala, color);
    if (segmentos[5]) DibujarSegmentoNumeroEditor({ centro.x - xLado, centro.y + yMedio, centro.z }, false, escala, color);
    if (segmentos[6]) DibujarSegmentoNumeroEditor(centro, true, escala, color);
}


static void DibujarCinta67Editor(
    const ObjetoMapaEditor& objeto
)
{
    Vector3 posicion = objeto.transform.translation;
    Vector3 escala = objeto.transform.scale;

    float largoTotal =
        DimensionEditor(escala.x, 0.80f);

    float largoUtil = largoTotal - 0.40f;

    if (largoUtil < 0.40f)
    {
        largoUtil = 0.40f;
    }

    float altoBase =
        DimensionEditor(escala.y, 0.05f);

    float anchoBase =
        DimensionEditor(escala.z, 0.20f);

    float inicioX =
        posicion.x - largoUtil / 2.0f;

    DrawCube(
        posicion,
        largoTotal,
        altoBase,
        anchoBase,
        objeto.color
    );

    DrawCube(
        { posicion.x, posicion.y + altoBase * 0.62f, posicion.z },
        largoUtil,
        0.08f,
        DimensionEditor(anchoBase - 0.20f, 0.10f),
        Color{ 88, 95, 105, 255 }
    );

    const int CANTIDAD_DIVISIONES = 12;
    float separacion =
        largoUtil / (float)CANTIDAD_DIVISIONES;

    for (int i = 0; i < CANTIDAD_DIVISIONES; i++)
    {
        float x =
            inicioX +
            separacion * ((float)i + 0.5f);

        DrawCube(
            { x, posicion.y + altoBase * 0.76f, posicion.z },
            0.06f,
            0.03f,
            DimensionEditor(anchoBase - 0.24f, 0.08f),
            Color{ 177, 184, 192, 255 }
        );
    }

    const float INICIO_ZONA = 0.755f;
    const float FIN_ZONA = 0.815f;

    float xInicio =
        inicioX + INICIO_ZONA * largoUtil;

    float xFin =
        inicioX + FIN_ZONA * largoUtil;

    DrawCube(
        { (xInicio + xFin) / 2.0f, posicion.y + altoBase * 0.89f, posicion.z },
        xFin - xInicio,
        0.035f,
        DimensionEditor(anchoBase - 0.15f, 0.08f),
        Fade(YELLOW, 0.58f)
    );

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube(
            {
                posicion.x,
                posicion.y + altoBase * 1.07f,
                posicion.z + lado * anchoBase * 0.50f
            },
            largoTotal + 0.05f,
            0.17f,
            0.10f,
            Color{ 39, 43, 49, 255 }
        );
    }

    DrawCube(
        { inicioX - 0.42f, posicion.y + 0.90f, posicion.z },
        0.65f,
        2.0f,
        anchoBase + 0.28f,
        Color{ 55, 60, 68, 255 }
    );

    DrawCube(
        { inicioX - 0.06f, posicion.y + 0.98f, posicion.z },
        0.07f,
        1.25f,
        DimensionEditor(anchoBase - 0.14f, 0.08f),
        Color{ 192, 120, 55, 255 }
    );
}


static void DibujarMesa67Editor(
    const ObjetoMapaEditor& objeto
)
{
    Vector3 posicion = objeto.transform.translation;
    Vector3 escala = objeto.transform.scale;

    float ancho =
        DimensionEditor(escala.x, 0.40f);

    float profundidad =
        DimensionEditor(escala.z, 0.40f);

    float altura =
        DimensionEditor(escala.y, 0.08f);

    float radioBase =
        (ancho + profundidad) * 0.25f;

    float factorRadio =
        radioBase / 0.88f;

    DrawCylinder(
        posicion,
        radioBase,
        radioBase,
        altura,
        28,
        objeto.color
    );

    DrawCylinder(
        {
            posicion.x,
            posicion.y + altura * 0.68f,
            posicion.z
        },
        radioBase * 0.82f,
        radioBase * 0.82f,
        altura * 0.45f,
        28,
        Color{ 49, 52, 58, 255 }
    );

    DrawCylinder(
        {
            posicion.x,
            posicion.y + altura * 1.05f,
            posicion.z
        },
        radioBase * 0.68f,
        radioBase * 0.68f,
        DimensionEditor(altura * 0.18f, 0.03f),
        28,
        Color{ 135, 98, 72, 255 }
    );

    float yNumeros =
        posicion.y + altura * 2.77f;

    DibujarNumero3DEditor(
        6,
        {
            posicion.x - 0.26f * factorRadio,
            yNumeros,
            posicion.z
        },
        0.37f * factorRadio,
        Fade(RAYWHITE, 0.35f)
    );

    DibujarNumero3DEditor(
        7,
        {
            posicion.x + 0.27f * factorRadio,
            yNumeros,
            posicion.z
        },
        0.37f * factorRadio,
        Fade(RAYWHITE, 0.35f)
    );
}


static void DibujarDecoracionFabrica67Editor(
    const MapaEditor& mapa
)
{
    const ObjetoMapaEditor* pared =
        BuscarObjetoMapa(mapa, "ParedFondo");

    if (pared != nullptr)
    {
        Vector3 posicion = pared->transform.translation;
        Vector3 escala = pared->transform.scale;

        float alto = DimensionEditor(escala.y, 0.20f);
        float largo = DimensionEditor(escala.x, 0.20f);

        DrawCube(
            {
                posicion.x,
                posicion.y + alto * 0.415f,
                posicion.z + 0.17f
            },
            largo * 0.953f,
            0.50f,
            0.10f,
            Color{ 160, 105, 70, 255 }
        );
    }

    const ObjetoMapaEditor* piso =
        BuscarObjetoMapa(mapa, "Piso");

    if (piso != nullptr)
    {
        Vector3 posicion = piso->transform.translation;
        Vector3 escala = piso->transform.scale;

        DrawCube(
            {
                posicion.x - 0.5f,
                posicion.y + DimensionEditor(escala.y, 0.05f) * 3.3f,
                posicion.z
            },
            DimensionEditor(escala.x, 0.20f) * 0.70f,
            0.05f,
            DimensionEditor(escala.z, 0.20f) * 0.245f,
            Fade(RAYWHITE, 0.12f)
        );
    }
}


static void DibujarObjetoMapaEditor(
    EditorMapas& editor,
    ObjetoMapaEditor& objeto
)
{
    if (EsMarcadorCamaraPos(objeto))
    {
        const Vector3 posicion = objeto.transform.translation;

        DrawCube(
            posicion,
            0.65f,
            0.65f,
            0.65f,
            objeto.color
        );

        DrawCubeWires(
            posicion,
            0.72f,
            0.72f,
            0.72f,
            RAYWHITE
        );

        return;
    }

    if (objeto.tipo == OBJETO_MAPA_EDITOR_CINTA_67)
    {
        DibujarCinta67Editor(objeto);
        return;
    }

    if (objeto.tipo == OBJETO_MAPA_EDITOR_MESA_67)
    {
        DibujarMesa67Editor(objeto);
        return;
    }

    if (!editor.modeloCuboCargado)
    {
        return;
    }

    editor.modeloCubo.transform =
        GizmoToMatrix(
            objeto.transform
        );

    DrawModel(
        editor.modeloCubo,
        { 0.0f, 0.0f, 0.0f },
        1.0f,
        objeto.color
    );
}


void EditorMapas::Inicializar()
{
    camara.position =
    {
        11.0f,
        9.0f,
        13.0f
    };

    camara.target =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy = 45.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    Vector3 direccion =
        Vector3Normalize(
            Vector3Subtract(
                camara.target,
                camara.position
            )
        );

    pitchCamara =
        std::asin(direccion.y);

    yawCamara =
        std::atan2(
            direccion.x,
            direccion.z
        );

    mapa.Cargar(
        RUTA_MAPA_FABRICA_67
    );

    if (mapa.cantidadObjetos > 0)
    {
        indiceSeleccionado = 0;
    }

    Mesh mallaCubo =
        GenMeshCube(
            1.0f,
            1.0f,
            1.0f
        );

    modeloCubo =
        LoadModelFromMesh(mallaCubo);

    modeloCuboCargado = true;

    SetGizmoSize(1.35f);
    SetGizmoLineWidth(2.5f);

    cambiosSinGuardar = false;
    tiempoMensaje = 0.0f;

    cantidadHistorialDeshacer = 0;
    gizmoEstabaActivo = false;
    indiceGizmoActivo = -1;
}


void EditorMapas::Actualizar(
    float deltaTime
)
{
    if (tiempoMensaje > 0.0f)
    {
        tiempoMensaje -= deltaTime;

        if (tiempoMensaje < 0.0f)
        {
            tiempoMensaje = 0.0f;
        }
    }

    if (IsKeyPressed(KEY_ONE))
    {
        modoGizmo = GIZMO_EDITOR_TRASLADAR;
    }

    if (IsKeyPressed(KEY_TWO))
    {
        modoGizmo = GIZMO_EDITOR_ROTAR;
    }

    if (IsKeyPressed(KEY_THREE))
    {
        modoGizmo = GIZMO_EDITOR_ESCALAR;
    }

    bool controlPresionado =
        IsKeyDown(KEY_LEFT_CONTROL) ||
        IsKeyDown(KEY_RIGHT_CONTROL);

    if (
        controlPresionado &&
        IsKeyPressed(KEY_Z) &&
        !gizmoEstabaActivo
    )
    {
        DeshacerUltimaAccion(*this);
    }

    if (
        controlPresionado &&
        IsKeyPressed(KEY_S)
    )
    {
        ultimoGuardadoExitoso = mapa.Guardar();
        tiempoMensaje = 2.0f;

        if (ultimoGuardadoExitoso)
        {
            cambiosSinGuardar = false;
        }
    }

    if (
        controlPresionado &&
        IsKeyPressed(KEY_R)
    )
    {
        bool cargado =
            mapa.Cargar(
                RUTA_MAPA_FABRICA_67
            );

        ultimoGuardadoExitoso = cargado;
        tiempoMensaje = 2.0f;
        cambiosSinGuardar = false;

        cantidadHistorialDeshacer = 0;
        gizmoEstabaActivo = false;
        indiceGizmoActivo = -1;

        if (mapa.cantidadObjetos > 0)
        {
            indiceSeleccionado = 0;
        }
        else
        {
            indiceSeleccionado = -1;
        }
    }

    ActualizarCamaraEditor(
        *this,
        deltaTime
    );

    SeleccionarObjetoConMouse(*this);
}


void EditorMapas::Dibujar()
{
    ClearBackground(
        Color{ 28, 31, 38, 255 }
    );

    BeginMode3D(camara);

    DrawGrid(
        40,
        1.0f
    );

    for (int i = 0; i < mapa.cantidadObjetos; i++)
    {
        ObjetoMapaEditor& objeto =
            mapa.objetos[i];

        DibujarObjetoMapaEditor(
            *this,
            objeto
        );

        if (i == indiceSeleccionado)
        {
            DrawBoundingBox(
                CrearCajaSeleccionEditor(objeto),
                YELLOW
            );
        }
    }

    DibujarDecoracionFabrica67Editor(mapa);

    if (
        indiceSeleccionado >= 0 &&
        indiceSeleccionado < mapa.cantidadObjetos
    )
    {
        ObjetoMapaEditor& seleccionado =
            mapa.objetos[indiceSeleccionado];

        Transform transformAntesFrame =
            seleccionado.transform;

        bool transformando =
            DrawGizmo3D(
                ObtenerFlagsGizmo(modoGizmo),
                &seleccionado.transform
            );

        if (
            transformando &&
            !gizmoEstabaActivo
        )
        {
            gizmoEstabaActivo = true;
            indiceGizmoActivo = indiceSeleccionado;
            transformInicioGizmo = transformAntesFrame;
        }

        if (transformando)
        {
            cambiosSinGuardar = true;
        }

        if (
            !transformando &&
            gizmoEstabaActivo
        )
        {
            if (
                indiceGizmoActivo >= 0 &&
                indiceGizmoActivo < mapa.cantidadObjetos
            )
            {
                const Transform& transformFinal =
                    mapa.objetos[
                        indiceGizmoActivo
                    ].transform;

                if (
                    !TransformIgualEditor(
                        transformInicioGizmo,
                        transformFinal
                    )
                )
                {
                    RegistrarPasoDeshacer(
                        *this,
                        indiceGizmoActivo,
                        transformInicioGizmo
                    );
                }
            }

            gizmoEstabaActivo = false;
            indiceGizmoActivo = -1;
        }
    }

    EndMode3D();

    DrawRectangle(
        16,
        16,
        420,
        335,
        Fade(BLACK, 0.78f)
    );

    DrawText(
        "EDITOR DE MAPAS - FABRICA 67",
        30,
        30,
        22,
        RAYWHITE
    );

    DrawText(
        TextFormat(
            "GIZMO: %s",
            ObtenerNombreModoGizmo(modoGizmo)
        ),
        30,
        64,
        18,
        ORANGE
    );

    if (
        indiceSeleccionado >= 0 &&
        indiceSeleccionado < mapa.cantidadObjetos
    )
    {
        const ObjetoMapaEditor& seleccionado =
            mapa.objetos[indiceSeleccionado];

        DrawText(
            TextFormat(
                "SELECCION: %s  [%s]",
                seleccionado.nombre,
                ObtenerNombreTipoObjeto(seleccionado.tipo)
            ),
            30,
            94,
            16,
            RAYWHITE
        );

        DrawText(
            TextFormat(
                "POS  %.2f  %.2f  %.2f",
                seleccionado.transform.translation.x,
                seleccionado.transform.translation.y,
                seleccionado.transform.translation.z
            ),
            30,
            120,
            16,
            LIGHTGRAY
        );

        if (EsMarcadorCamaraPos(seleccionado))
        {
            DrawText(
                TextFormat(
                    "ZOOM ORTO  %.2f",
                    std::fabs(seleccionado.transform.scale.x)
                ),
                30,
                144,
                16,
                LIGHTGRAY
            );
        }
        else
        {
            DrawText(
                TextFormat(
                    "ESC  %.2f  %.2f  %.2f",
                    seleccionado.transform.scale.x,
                    seleccionado.transform.scale.y,
                    seleccionado.transform.scale.z
                ),
                30,
                144,
                16,
                LIGHTGRAY
            );
        }
    }
    else
    {
        DrawText(
            "SELECCION: ninguna",
            30,
            94,
            18,
            LIGHTGRAY
        );
    }

    DrawText(
        "CLICK objeto: seleccionar",
        30,
        178,
        15,
        LIGHTGRAY
    );

    DrawText(
        "1 mover   2 rotar   3 escalar",
        30,
        198,
        15,
        LIGHTGRAY
    );

    DrawText(
        "WASD mover | Q/E altura | RMB mirar",
        30,
        218,
        14,
        LIGHTGRAY
    );

    DrawText(
        "RUEDA: zoom hacia donde mira la camara",
        30,
        238,
        14,
        LIGHTGRAY
    );

    DrawText(
        "CTRL+Z deshacer | CTRL+S guardar | CTRL+R recargar",
        30,
        258,
        14,
        LIGHTGRAY
    );

    DrawText(
        "CTRL+E volver | historial: 64 acciones",
        30,
        278,
        14,
        LIGHTGRAY
    );

    if (cambiosSinGuardar)
    {
        DrawText(
            "CAMBIOS SIN GUARDAR",
            GetScreenWidth() - 245,
            24,
            18,
            YELLOW
        );
    }

    if (tiempoMensaje > 0.0f)
    {
        DrawText(
            ultimoGuardadoExitoso
                ? "MAPA GUARDADO / CARGADO"
                : "NO SE PUDO GUARDAR / CARGAR",
            30,
            328,
            18,
            ultimoGuardadoExitoso
                ? LIME
                : RED
        );
    }
}


void EditorMapas::Descargar()
{
    if (modeloCuboCargado)
    {
        UnloadModel(modeloCubo);
        modeloCuboCargado = false;
    }
}
