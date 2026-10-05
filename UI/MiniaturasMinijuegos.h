#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "raylib.h"


// Dibuja la miniatura representativa de un minijuego dentro de rect.
// Reserva los ultimos 36 px de alto para que quien la use dibuje
// el nombre debajo. La comparten SeleccionMinijuegos y RuletaMinijuegos.
// escalaMaxima limita cuanto puede agrandarse el dibujo en celdas grandes.
void DibujarMiniaturaMinijuego(
    IdMinijuego id,
    Rectangle rect,
    float escalaMaxima = 1.0f
);
