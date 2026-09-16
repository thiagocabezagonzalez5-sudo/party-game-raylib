#pragma once

#include "Editor/MapaEditor.h"


enum ModoGizmoEditor
{
    GIZMO_EDITOR_TRASLADAR = 0,
    GIZMO_EDITOR_ROTAR,
    GIZMO_EDITOR_ESCALAR
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

    void Inicializar();
    void Actualizar(float deltaTime);
    void Dibujar();
    void Descargar();
};
