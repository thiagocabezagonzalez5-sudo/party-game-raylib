#pragma once 

#include "raylib.h"

struct CamaraJuego {
    Camera3D camara;
    
    void Inicializar();

    void Actualizar();

    void Begin();

    void End();
};