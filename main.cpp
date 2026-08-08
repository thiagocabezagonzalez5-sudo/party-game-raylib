#include <raylib.h>

//==================================================
// PLATAFORMA
//==================================================

struct Plataforma
{
    Vector3 posicion;
    Vector3 tamano;
    Color color;


    float ObtenerAlturaSuperior() const
    {
        return posicion.y + tamano.y / 2.0f;
    }


    bool JugadorSuperpuestoXZ(
        Vector3 posicionJugador,
        float anchoJugador,
        float profundidadJugador
    ) const
    {
        float mitadJugadorX = anchoJugador / 2.0f;
        float mitadJugadorZ = profundidadJugador / 2.0f;

        float mitadPlataformaX = tamano.x / 2.0f;
        float mitadPlataformaZ = tamano.z / 2.0f;


        float jugadorMinX = posicionJugador.x - mitadJugadorX;
        float jugadorMaxX = posicionJugador.x + mitadJugadorX;

        float jugadorMinZ = posicionJugador.z - mitadJugadorZ;
        float jugadorMaxZ = posicionJugador.z + mitadJugadorZ;


        float plataformaMinX = posicion.x - mitadPlataformaX;
        float plataformaMaxX = posicion.x + mitadPlataformaX;

        float plataformaMinZ = posicion.z - mitadPlataformaZ;
        float plataformaMaxZ = posicion.z + mitadPlataformaZ;


        bool colisionX =
            jugadorMaxX > plataformaMinX &&
            jugadorMinX < plataformaMaxX;

        bool colisionZ =
            jugadorMaxZ > plataformaMinZ &&
            jugadorMinZ < plataformaMaxZ;


        return colisionX && colisionZ;
    }


    void Dibujar() const
    {
        DrawCube(
            posicion,
            tamano.x,
            tamano.y,
            tamano.z,
            color
        );

        DrawCubeWires(
            posicion,
            tamano.x,
            tamano.y,
            tamano.z,
            DARKGRAY
        );
    }
};


//==================================================
// JUGADOR
//==================================================

struct Jugador
{
    Vector3 posicion;

    float ancho = 1.0f;
    float alto = 1.0f;
    float profundidad = 1.0f;

    float velocidadMovimiento = 5.0f;
    float velocidadVertical = 0.0f;

    bool enSuelo = true;
    bool caido = false;


    //-----------------
    // CONSTANTES FISICAS
    //-----------------

    const float GRAVEDAD = 20.0f;
    const float FUERZA_SALTO = 8.0f;


    //-----------------------------------
    // COMPROBAR SI TIENE PISO DEBAJO
    //-----------------------------------

    bool TieneSoporte(
        Plataforma plataformas[],
        int cantidadPlataformas
    )
    {
        float piesJugador = posicion.y - alto / 2.0f;

        for (int i = 0; i < cantidadPlataformas; i++)
        {
            if (
                plataformas[i].JugadorSuperpuestoXZ(
                    posicion,
                    ancho,
                    profundidad
                )
                )
            {
                float alturaPlataforma =
                    plataformas[i].ObtenerAlturaSuperior();


                // Pequeño margen para evitar errores
                // con números decimales.
                if (
                    piesJugador >= alturaPlataforma - 0.05f &&
                    piesJugador <= alturaPlataforma + 0.05f
                    )
                {
                    return true;
                }
            }
        }

        return false;
    }


    //-----------------
    // REINICIAR
    //-----------------

    void Reiniciar()
    {
        posicion = Vector3{
            0.0f,
            0.5f,
            0.0f
        };

        velocidadVertical = 0.0f;

        enSuelo = true;

        caido = false;
    }


    //-----------------
    // ACTUALIZAR
    //-----------------

    void Actualizar(
        float deltaTime,
        Plataforma plataformas[],
        int cantidadPlataformas
    )
    {
        //-----------------
        // JUGADOR CAIDO
        //-----------------

        if (caido)
        {
            if (IsKeyPressed(KEY_R))
            {
                Reiniciar();
            }

            return;
        }


        //-----------------
        // MOVIMIENTO
        //-----------------

        if (IsKeyDown(KEY_W))
        {
            posicion.z -= velocidadMovimiento * deltaTime;
        }

        if (IsKeyDown(KEY_S))
        {
            posicion.z += velocidadMovimiento * deltaTime;
        }

        if (IsKeyDown(KEY_A))
        {
            posicion.x -= velocidadMovimiento * deltaTime;
        }

        if (IsKeyDown(KEY_D))
        {
            posicion.x += velocidadMovimiento * deltaTime;
        }


        //-----------------
        // COMPROBAR SOPORTE
        //-----------------

        if (enSuelo)
        {
            if (!TieneSoporte(
                plataformas,
                cantidadPlataformas
            ))
            {
                enSuelo = false;
            }
        }


        //-----------------
        // SALTO
        //-----------------

        if (
            IsKeyPressed(KEY_SPACE) &&
            enSuelo
            )
        {
            velocidadVertical = FUERZA_SALTO;

            enSuelo = false;
        }


        //-----------------
        // GUARDAR ALTURA ANTERIOR
        //-----------------

        float posicionYAnterior = posicion.y;


        //-----------------
        // GRAVEDAD
        //-----------------

        if (!enSuelo)
        {
            velocidadVertical -= GRAVEDAD * deltaTime;

            posicion.y += velocidadVertical * deltaTime;
        }


        //-----------------
        // ATERRIZAJE
        //-----------------

        if (
            !enSuelo &&
            velocidadVertical <= 0.0f
            )
        {
            float piesAnteriores =
                posicionYAnterior - alto / 2.0f;

            float piesActuales =
                posicion.y - alto / 2.0f;


            bool aterrizo = false;

            float mejorAltura = -10000.0f;


            for (
                int i = 0;
                i < cantidadPlataformas;
                i++
                )
            {
                if (
                    plataformas[i].JugadorSuperpuestoXZ(
                        posicion,
                        ancho,
                        profundidad
                    )
                    )
                {
                    float alturaPlataforma =
                        plataformas[i].ObtenerAlturaSuperior();


                    //-----------------------------------
                    // ¿CRUZO LA SUPERFICIE AL CAER?
                    //-----------------------------------

                    if (
                        piesAnteriores >= alturaPlataforma &&
                        piesActuales <= alturaPlataforma
                        )
                    {
                        // Si hubiese más de una plataforma
                        // debajo, elegimos la más alta.
                        if (alturaPlataforma > mejorAltura)
                        {
                            mejorAltura = alturaPlataforma;

                            aterrizo = true;
                        }
                    }
                }
            }


            //-----------------
            // APOYAR JUGADOR
            //-----------------

            if (aterrizo)
            {
                posicion.y =
                    mejorAltura + alto / 2.0f;

                velocidadVertical = 0.0f;

                enSuelo = true;
            }
        }


        //-----------------
        // CAIDA AL VACIO
        //-----------------

        if (posicion.y < -5.0f)
        {
            caido = true;
        }
    }


    //-----------------
    // DIBUJAR
    //-----------------

    void Dibujar() const
    {
        DrawCube(
            posicion,
            ancho,
            alto,
            profundidad,
            RED
        );

        DrawCubeWires(
            posicion,
            ancho,
            alto,
            profundidad,
            MAROON
        );
    }
};


//==================================================
// MAIN
//==================================================

int main()
{
    //-----------------
    // VENTANA
    //-----------------

    const int ANCHO = 1280;
    const int ALTO = 720;

    InitWindow(
        ANCHO,
        ALTO,
        "Party Game"
    );

    SetTargetFPS(60);


    //-----------------
    // PANTALLA COMPLETA
    //-----------------

    int anchoVentana = ANCHO;
    int altoVentana = ALTO;

    Vector2 posicionVentana =
        GetWindowPosition();

    bool pantallaCompleta = false;


    //-----------------
    // CONFIGURACION
    //-----------------

    bool mostrarConfiguracion = false;


    //-----------------
    // CAMARA
    //-----------------

    Camera3D camara = { 0 };

    camara.position =
        Vector3{ 0.0f, 10.0f, 10.0f };

    camara.target =
        Vector3{ 0.0f, 0.0f, 0.0f };

    camara.up =
        Vector3{ 0.0f, 1.0f, 0.0f };

    camara.fovy = 45.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;


    //==================================================
    // PLATAFORMAS
    //==================================================

    Plataforma plataformas[] =
    {
        //-----------------------------------
        // PLATAFORMA PRINCIPAL
        //-----------------------------------

        {
            Vector3{ 0.0f, -0.25f, 0.0f },
            Vector3{ 20.0f, 0.5f, 20.0f },
            LIGHTGRAY
        },


        //-----------------------------------
        // BLOQUE 1
        //-----------------------------------

        {
            Vector3{ -4.0f, 0.5f, -2.0f },
            Vector3{ 3.0f, 1.0f, 3.0f },
            SKYBLUE
        },


        //-----------------------------------
        // BLOQUE 2
        //-----------------------------------

        {
            Vector3{ 4.0f, 0.6f, 1.0f },
            Vector3{ 3.0f, 1.2f, 3.0f },
            GOLD
        }
    };


    const int CANTIDAD_PLATAFORMAS =
        sizeof(plataformas) /
        sizeof(plataformas[0]);


    //-----------------
    // JUGADOR
    //-----------------

    Jugador jugador;

    jugador.posicion =
        Vector3{ 0.0f, 0.5f, 0.0f };


    //==================================================
    // GAME LOOP
    //==================================================

    while (!WindowShouldClose())
    {
        //-----------------
        // DELTA TIME
        //-----------------

        float deltaTime = GetFrameTime();


        //==================================================
        // PANTALLA COMPLETA
        //==================================================

        if (IsKeyPressed(KEY_F11))
        {
            if (!pantallaCompleta)
            {
                anchoVentana =
                    GetScreenWidth();

                altoVentana =
                    GetScreenHeight();

                posicionVentana =
                    GetWindowPosition();


                int monitor =
                    GetCurrentMonitor();

                int anchoMonitor =
                    GetMonitorWidth(monitor);

                int altoMonitor =
                    GetMonitorHeight(monitor);


                SetWindowSize(
                    anchoMonitor,
                    altoMonitor
                );

                ToggleFullscreen();

                pantallaCompleta = true;
            }
            else
            {
                ToggleFullscreen();

                SetWindowSize(
                    anchoVentana,
                    altoVentana
                );

                SetWindowPosition(
                    (int)posicionVentana.x,
                    (int)posicionVentana.y
                );

                pantallaCompleta = false;
            }
        }


        //==================================================
        // CONFIGURACION
        //==================================================

        if (IsKeyPressed(KEY_TAB))
        {
            mostrarConfiguracion =
                !mostrarConfiguracion;
        }


        //==================================================
        // ACTUALIZAR JUGADOR
        //==================================================

        if (!mostrarConfiguracion)
        {
            jugador.Actualizar(
                deltaTime,
                plataformas,
                CANTIDAD_PLATAFORMAS
            );
        }


        //==================================================
        // DIBUJO
        //==================================================

        BeginDrawing();

        ClearBackground(RAYWHITE);


        //==================================================
        // MUNDO 3D
        //==================================================

        BeginMode3D(camara);


        //-----------------
        // PLATAFORMAS
        //-----------------

        for (
            int i = 0;
            i < CANTIDAD_PLATAFORMAS;
            i++
            )
        {
            plataformas[i].Dibujar();
        }


        //-----------------
        // JUGADOR
        //-----------------

        jugador.Dibujar();


        EndMode3D();


        //==================================================
        // INTERFAZ 2D
        //==================================================

        DrawText(
            "WASD - Mover",
            20,
            20,
            20,
            BLACK
        );

        DrawText(
            "ESPACIO - Saltar",
            20,
            50,
            20,
            BLACK
        );

        DrawText(
            "TAB - Configuracion",
            20,
            80,
            20,
            BLACK
        );

        DrawText(
            "F11 - Pantalla completa",
            20,
            110,
            20,
            BLACK
        );


        //==================================================
        // MENSAJE DE CAIDA
        //==================================================

        if (jugador.caido)
        {
            const char* titulo =
                "TE CAISTE";

            const char* mensaje =
                "Presiona R para reiniciar";


            const int TAMANO_TITULO = 40;
            const int TAMANO_MENSAJE = 25;


            int anchoTitulo =
                MeasureText(
                    titulo,
                    TAMANO_TITULO
                );

            int anchoMensaje =
                MeasureText(
                    mensaje,
                    TAMANO_MENSAJE
                );


            int centroX =
                GetScreenWidth() / 2;

            int centroY =
                GetScreenHeight() / 2;


            DrawRectangle(
                centroX - 250,
                centroY - 100,
                500,
                200,
                Fade(BLACK, 0.75f)
            );


            DrawText(
                titulo,
                centroX - anchoTitulo / 2,
                centroY - 50,
                TAMANO_TITULO,
                RED
            );


            DrawText(
                mensaje,
                centroX - anchoMensaje / 2,
                centroY + 20,
                TAMANO_MENSAJE,
                RAYWHITE
            );
        }


        //==================================================
        // MODAL CONFIGURACION
        //==================================================

        if (mostrarConfiguracion)
        {
            int anchoPantalla =
                GetScreenWidth();

            int altoPantalla =
                GetScreenHeight();


            //-----------------
            // FONDO OSCURO
            //-----------------

            DrawRectangle(
                0,
                0,
                anchoPantalla,
                altoPantalla,
                Fade(BLACK, 0.6f)
            );


            const int ANCHO_MODAL = 500;
            const int ALTO_MODAL = 300;


            int modalX =
                (anchoPantalla - ANCHO_MODAL) / 2;

            int modalY =
                (altoPantalla - ALTO_MODAL) / 2;


            //-----------------
            // FONDO MODAL
            //-----------------

            DrawRectangle(
                modalX,
                modalY,
                ANCHO_MODAL,
                ALTO_MODAL,
                RAYWHITE
            );


            //-----------------
            // BORDE
            //-----------------

            DrawRectangleLines(
                modalX,
                modalY,
                ANCHO_MODAL,
                ALTO_MODAL,
                DARKGRAY
            );


            //-----------------
            // TITULO
            //-----------------

            DrawText(
                "CONFIGURACION",
                modalX + 30,
                modalY + 30,
                30,
                BLACK
            );


            //-----------------
            // OPCIONES FUTURAS
            //-----------------

            DrawText(
                "Pantalla",
                modalX + 30,
                modalY + 100,
                20,
                DARKGRAY
            );

            DrawText(
                "Audio",
                modalX + 30,
                modalY + 140,
                20,
                DARKGRAY
            );

            DrawText(
                "Controles",
                modalX + 30,
                modalY + 180,
                20,
                DARKGRAY
            );


            DrawText(
                "TAB para volver",
                modalX + 30,
                modalY + 240,
                18,
                GRAY
            );
        }


        EndDrawing();
    }


    //==================================================
    // CERRAR
    //==================================================

    CloseWindow();

    return 0;
}