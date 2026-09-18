#pragma once

#include "Editor/MapaEditor.h"

static const int MAX_HISTORIAL_DESHACER_EDITOR = 64;

enum ModoGizmoEditor
{
    GIZMO_EDITOR_TRASLADAR = 0,
    GIZMO_EDITOR_ROTAR,
    GIZMO_EDITOR_ESCALAR
};

struct PasoDeshacerEditor
{
    int indiceObjeto = -1;
    Transform transformAnterior{};
};

struct EditorMapas
{
    MapaEditor mapa;

    Camera3D camara{};

    Model modeloCubo{};
    bool modeloCuboCargado = false;

    int indiceSeleccionado = -1;

    ModoGizmoEditor modoGizmo =
        GIZMO_EDITOR_TRASLADAR;

    float yawCamara = 0.0f;
    float pitchCamara = 0.0f;

    bool cambiosSinGuardar = false;

    float tiempoMensaje = 0.0f;
    bool ultimoGuardadoExitoso = false;

    PasoDeshacerEditor historialDeshacer[MAX_HISTORIAL_DESHACER_EDITOR]{};
    int cantidadHistorialDeshacer = 0;

    bool gizmoEstabaActivo = false;
    int indiceGizmoActivo = -1;
    Transform transformInicioGizmo{};

    void Inicializar();
    void Actualizar(float deltaTime);
    void Dibujar();
    void Descargar();
};
