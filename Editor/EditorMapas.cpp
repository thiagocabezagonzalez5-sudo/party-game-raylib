#include "Editor/EditorMapas.h"

#include "raygizmo.h"
#include "raymath.h"

#include <cmath>


static const char* RUTA_MAPA_FABRICA_67 =
    "Mapas/Minijuego67.map";


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
        modoGizmo =
            GIZMO_EDITOR_TRASLADAR;
    }

    if (IsKeyPressed(KEY_TWO))
    {
        modoGizmo =
            GIZMO_EDITOR_ROTAR;
    }

    if (IsKeyPressed(KEY_THREE))
    {
        modoGizmo =
            GIZMO_EDITOR_ESCALAR;
    }

    bool controlPresionado =
        IsKeyDown(KEY_LEFT_CONTROL) ||
        IsKeyDown(KEY_RIGHT_CONTROL);

    if (
        controlPresionado &&
        IsKeyPressed(KEY_S)
    )
    {
        ultimoGuardadoExitoso =
            mapa.Guardar();

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

        if (modeloCuboCargado)
        {
            modeloCubo.transform =
                GizmoToMatrix(
                    objeto.transform
                );

            DrawModel(
                modeloCubo,
                { 0.0f, 0.0f, 0.0f },
                1.0f,
                objeto.color
            );
        }

        if (i == indiceSeleccionado)
        {
            DrawBoundingBox(
                CrearCajaSeleccionEditor(objeto),
                YELLOW
            );
        }
    }

    if (
        indiceSeleccionado >= 0 &&
        indiceSeleccionado < mapa.cantidadObjetos
    )
    {
        ObjetoMapaEditor& seleccionado =
            mapa.objetos[indiceSeleccionado];

        bool transformando =
            DrawGizmo3D(
                ObtenerFlagsGizmo(modoGizmo),
                &seleccionado.transform
            );

        if (transformando)
        {
            cambiosSinGuardar = true;
        }
    }

    EndMode3D();

    DrawRectangle(
        16,
        16,
        360,
        252,
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
                "SELECCION: %s",
                seleccionado.nombre
            ),
            30,
            94,
            18,
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
        "WASD mover camara | Q/E altura | RMB mirar",
        30,
        218,
        14,
        LIGHTGRAY
    );

    DrawText(
        "CTRL+S guardar | CTRL+R recargar | CTRL+E volver",
        30,
        238,
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
            282,
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
