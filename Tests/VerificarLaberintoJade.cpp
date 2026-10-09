// Prueba de ZonaPruebas y del minijuego real. El linker observa matrices
// enviadas a OpenGL y simula teclas; no se usa el visor del paquete.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "rlgl.h"
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

static int errores = 0;
static int avisosAusente = 0;
static int teclaSimulada = KEY_NULL;
static bool inclinarTeclas = false;
static bool observar = false;
static ZonaPruebas zona;
static Color originales[CANTIDAD_MODELOS_LABERINTO_JADE_3D][16]{};

struct DibujoJade
{
    Vector3 posicion{};
    Vector3 escala{};
    float angulo = 0;
    Color color{};
    int jugador = -1;
};
static DibujoJade dibujos[CANTIDAD_MODELOS_LABERINTO_JADE_3D][512]{};
static int cantidades[CANTIDAD_MODELOS_LABERINTO_JADE_3D]{};
static int ubicacionMVP = -1;
static int enviosMVP = 0;
static Matrix esperadoMVP{};

static void Comprobar(bool condicion, const char* detalle)
{
    if (!condicion)
    {
        if (errores < 20) std::fprintf(stderr, "FALLO: %s\n", detalle);
        errores++;
    }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < 0.002f; }
static bool Igual(Vector3 a, Vector3 b)
{
    return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z);
}
static bool Igual(Matrix a, Matrix b)
{
    float16 va = MatrixToFloatV(a), vb = MatrixToFloatV(b);
    for (int i=0; i<16; i++) if (!Cerca(va.v[i],vb.v[i])) return false;
    return true;
}
static void RegistrarPrueba(int nivel, const char* formato, va_list argumentos)
{
    char texto[2048];
    std::vsnprintf(texto, sizeof(texto), formato, argumentos);
    if (std::strstr(texto,"Modelo de escenario ausente")) avisosAusente++;
    if (nivel >= LOG_WARNING) std::fprintf(stderr,"%s\n",texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int tecla)
{
    return tecla == teclaSimulada || __real_IsKeyPressed(tecla);
}
extern "C" bool __real_IsKeyDown(int);
extern "C" bool __wrap_IsKeyDown(int tecla)
{
    return (inclinarTeclas && (tecla == KEY_D || tecla == KEY_W)) || __real_IsKeyDown(tecla);
}
extern "C" void __real_rlSetUniformMatrix(int, Matrix);
extern "C" void __wrap_rlSetUniformMatrix(int ubicacion, Matrix matriz)
{
    if (ubicacionMVP >= 0 && ubicacion == ubicacionMVP)
    {
        enviosMVP++;
        Comprobar(Igual(matriz, esperadoMVP), "GPU recibe inclinacion/traslacion exactamente una vez");
    }
    __real_rlSetUniformMatrix(ubicacion, matriz);
}
extern "C" void __real_DrawModelEx(Model, Vector3, Vector3, float, Vector3, Color);
extern "C" void __wrap_DrawModelEx(Model modelo, Vector3 posicion, Vector3 eje,
    float angulo, Vector3 escala, Color tinte)
{
    const auto& recursos = ObtenerModelosEscenariosRetro3D().laberintoJade;
    int pieza = -1;
    for (int i=0; observar && i<CANTIDAD_MODELOS_LABERINTO_JADE_3D; i++)
        if (modelo.meshes == recursos[i].modelo.meshes) { pieza=i; break; }
    if (pieza < 0) { __real_DrawModelEx(modelo,posicion,eje,angulo,escala,tinte); return; }
    const auto& r = recursos[pieza];
    Matrix padre = rlGetMatrixTransform();
    const auto& m = zona.gestorMinijuegos.minijuegoLaberintoInclinado;
    int jugador = -1;
    if (pieza < MODELO_JADE_TEMPLO)
    {
        Vector3 origen = Vector3Transform({0,0,0},padre);
        for (int j=0; j<MAX_PARTICIPANTES; j++)
            if (m.resultado.participantes[j].participo &&
                Igual(origen,m.centrosTableros[m.estadosJugadores[j].tablero])) jugador=j;
        Comprobar(jugador >= 0, "Pieza local pertenece a un tablero");
        if (jugador >= 0)
        {
            const auto& e = m.estadosJugadores[jugador];
            // Rotar Z y luego X un punto de prueba, sin reutilizar la matriz del juego.
            float az=-e.inclinacionX*12*DEG2RAD, ax=e.inclinacionZ*12*DEG2RAD;
            Vector3 p={std::cos(az)-2*std::sin(az),std::sin(az)+2*std::cos(az),3};
            Vector3 esperado={origen.x+p.x,origen.y+p.y*std::cos(ax)-p.z*std::sin(ax),
                origen.z+p.y*std::sin(ax)+p.z*std::cos(ax)};
            Comprobar(Igual(Vector3Transform({1,2,3},padre),esperado),
                "Todas las piezas heredan la inclinacion original en ambos ejes");
        }
    }
    else Comprobar(Igual(padre,MatrixIdentity()), "Templo, columnas y antorchas permanecen en mundo");
    int n=cantidades[pieza]++;
    Comprobar(n<512,"Limite de instancias de la prueba");
    Color color = r.materialColor < 0 ? WHITE : modelo.materials[r.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
    if (n<512) dibujos[pieza][n]={posicion,escala,angulo,color,jugador};
    Comprobar(ColorIsEqual(tinte,WHITE), "No tenir todos los materiales del GLB");
    for (int j=0; j<modelo.materialCount && j<16; j++)
        if (j != r.materialColor)
            Comprobar(ColorIsEqual(originales[pieza][j],modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),
                "Materiales fijos intactos durante el dibujo");

    Matrix instancia=MatrixMultiply(MatrixMultiply(MatrixScale(escala.x,escala.y,escala.z),
        MatrixRotate(eje,angulo*DEG2RAD)),MatrixTranslate(posicion.x,posicion.y,posicion.z));
    Matrix mundo=MatrixMultiply(MatrixMultiply(modelo.transform,instancia),padre);
    esperadoMVP=MatrixMultiply(MatrixMultiply(mundo,rlGetMatrixModelview()),rlGetMatrixProjection());
    ubicacionMVP=modelo.materials[0].shader.locs[SHADER_LOC_MATRIX_MVP];
    enviosMVP=0;
    __real_DrawModelEx(modelo,posicion,eje,angulo,escala,tinte);
    Comprobar(enviosMVP == modelo.meshCount, "Observar el dibujo real de cada malla en GPU");
    ubicacionMVP=-1;
}

static void DibujarPrueba(const char* captura=nullptr)
{
    // TextFormat reutiliza sus buffers durante el HUD; conservar la ruta.
    char rutaCaptura[512]{};
    if (captura) std::snprintf(rutaCaptura,sizeof(rutaCaptura),"%s",captura);
    std::memset(cantidades,0,sizeof(cantidades));
    auto& m=zona.gestorMinijuegos.minijuegoLaberintoInclinado;
    EstadoJugadorLaberinto estados[MAX_PARTICIPANTES];
    TipoCeldaLaberinto celdas[FILAS_LABERINTO][COLUMNAS_LABERINTO];
    std::memcpy(estados,m.estadosJugadores,sizeof(estados));
    std::memcpy(celdas,m.celdas,sizeof(celdas));
    Camera3D camara=m.camara;
    BeginDrawing();
    zona.Dibujar();
    if (captura)
    {
        rlDrawRenderBatchActive();
        Image imagen=LoadImageFromScreen();
        Comprobar(ExportImage(imagen,rutaCaptura),"Captura del minijuego integrado");
        UnloadImage(imagen);
    }
    EndDrawing();
    Comprobar(std::memcmp(estados,m.estadosJugadores,sizeof(estados))==0 &&
        std::memcmp(celdas,m.celdas,sizeof(celdas))==0,"El dibujo conserva simulacion y colisiones por celdas");
    Comprobar(Igual(camara.position,m.camara.position) && Igual(camara.target,m.camara.target),"Camara conservada");
    const auto& recursos=ObtenerModelosEscenariosRetro3D().laberintoJade;
    for (int i=0; i<CANTIDAD_MODELOS_LABERINTO_JADE_3D; i++)
        for (int j=0; j<recursos[i].modelo.materialCount && j<16; j++)
            Comprobar(ColorIsEqual(originales[i][j],recursos[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),
                "Restaurar todos los materiales tras cada dibujo");
}
static void Avanzar(int frames)
{
    for (int i=0; i<frames; i++) zona.Actualizar(1.0f/60);
}
static void ColocarEsfera(EstadoJugadorLaberinto& e,int c,int r)
{
    e.x=c+0.5f; e.z=r+0.5f;
    e.velocidadX=e.velocidadZ=e.inclinacionX=e.inclinacionZ=0;
}

int main()
{
    SetTraceLogCallback(RegistrarPrueba);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280,800,"Verificacion integrada del Laberinto Jade");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL);
    SetRandomSeed(1234);
    Participante participantes[MAX_PARTICIPANTES]{};
    const Color colores[]={RED,BLUE,GREEN,YELLOW};
    const Color bandas[]={{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
    auto& almacen=ObtenerModelosEscenariosRetro3D();
    Mesh* mallas[CANTIDAD_MODELOS_LABERINTO_JADE_3D]{};
    for (int cantidad=2; cantidad<=4; cantidad++)
    {
        for (int j=0; j<MAX_PARTICIPANTES; j++)
        {
            participantes[j]={};
            participantes[j].activo=j<cantidad;
            participantes[j].conectado=true;
            participantes[j].control=j%2==0?CONTROL_TECLADO_WASD:CONTROL_TECLADO_FLECHAS;
            participantes[j].numeroJugador=j+1;
            participantes[j].color=colores[j];
        }
        zona.Inicializar(participantes,cantidad);
        if (cantidad==2)
            for (const auto& r:almacen.laberintoJade)
                Comprobar(!r.cargaIntentada,"No cargar Jade al arrancar");
        zona.modoCatalogo=true;
        zona.CambiarMinijuego(MINIJUEGO_LABERINTO_INCLINADO);
        auto& m=zona.gestorMinijuegos.minijuegoLaberintoInclinado;
        for (int i=0; i<CANTIDAD_MODELOS_LABERINTO_JADE_3D; i++)
        {
            const auto& r=almacen.laberintoJade[i];
            Comprobar(r.cargado,RUTAS_MODELOS_LABERINTO_JADE_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(mallas[i]==r.modelo.meshes,"Mallas compartidas al reentrar");
            Comprobar(r.modelo.materialCount<=16,"Limite de materiales de la prueba");
            for (int j=0; j<r.modelo.materialCount && j<16; j++) originales[i][j]=r.modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
            for (int j=0; j<r.modelo.meshCount; j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Conservar colores de vertices");
        }
        BoundingBox esfera=GetModelBoundingBox(almacen.laberintoJade[MODELO_JADE_ESFERA].modelo);
        Comprobar(Igual(esfera.min,{-0.28f,-0.28f,-0.28f}) && Igual(esfera.max,{0.28f,0.28f,0.28f}),"Radio y pivote central de esfera");
        observar=true;
        for (int diseno=0; diseno<CANTIDAD_DISENOS_LABERINTO; diseno++)
        {
            for (int intento=0; m.disenoActual!=diseno && intento<32; intento++)
                zona.gestorMinijuegos.ReiniciarActivo(zona.contextoMinijuego);
            Comprobar(m.disenoActual==diseno,"Ejecutar los tres disenos disponibles");
            Comprobar(m.cantidadTableros==cantidad && m.fase==FASE_LABERINTO_PREPARACION,"Tableros y preparacion");
            DibujarPrueba(cantidad==4?TextFormat("build/jade-diseno-%d.png",diseno):nullptr);
            int muros=0,agujeros=0,glifos=0;
            for (int r=0; r<11; r++) for (int c=0; c<11; c++)
            {
                if (m.celdas[r][c]==CELDA_LABERINTO_MURO) { muros++; if ((c*7+r*3)%5==0) glifos++; }
                if (m.celdas[r][c]==CELDA_LABERINTO_AGUJERO) agujeros++;
            }
            const int esperados[]={cantidad,cantidad,muros*cantidad,glifos*cantidad,agujeros*cantidad,
                cantidad,2*cantidad,cantidad,cantidad,10*cantidad,4*cantidad,1,4,6,6};
            for (int i=0; i<15; i++) Comprobar(cantidades[i]==esperados[i],"Instancias por tablero sin duplicar recursos");
            for (int i=0; i<cantidades[MODELO_JADE_MURO]; i++)
            {
                const auto& d=dibujos[MODELO_JADE_MURO][i];
                int c=(int)std::lround(d.posicion.x)+5,r=(int)std::lround(d.posicion.z)+5;
                Comprobar(c>=0 && c<11 && r>=0 && r<11 && m.celdas[r][c]==CELDA_LABERINTO_MURO && Cerca(d.posicion.y,0),
                    "Muros colocados segun celdas del diseno actual");
            }
            for (int i=0; i<cantidad; i++)
                Comprobar(ColorIsEqual(dibujos[MODELO_JADE_BANDA][i].color,bandas[i]),"Color de jugador solo en COLOR_DINAMICO");
            Avanzar(182);
            Comprobar(m.fase==FASE_LABERINTO_JUGANDO,"Terminar cuenta regresiva");
            inclinarTeclas=true;
            Avanzar(10);
            inclinarTeclas=false;
            auto& e=m.estadosJugadores[0];
            Comprobar(e.inclinacionX>0.3f && e.inclinacionZ<-0.3f,"Entrada real inclina ambos ejes");
            DibujarPrueba(cantidad==4?TextFormat("build/jade-inclinacion-%d.png",diseno):nullptr);
            participantes[1].esBot=true;
            Avanzar(30);
            Comprobar(std::fabs(m.estadosJugadores[1].objetivoX)+std::fabs(m.estadosJugadores[1].objetivoZ)>0.01f,"IA conserva control de la esfera");
            participantes[1].esBot=false;
            for (int k=0; k<2; k++)
            {
                ColocarEsfera(e,m.controlColumna[k],m.controlFila[k]);
                Avanzar(1);
                Comprobar(e.puntosControl==k+1,"Activacion de checkpoint en orden");
            }
            DibujarPrueba();
            Comprobar(ColorIsEqual(dibujos[MODELO_JADE_CHECKPOINT][0].color,{52,176,128,255}) &&
                ColorIsEqual(dibujos[MODELO_JADE_CHECKPOINT][1].color,{52,176,128,255}),"Checkpoints activos color jade");
            const auto& trampa=m.trampas[0];
            m.tiempoJuego=2.4f-trampa.desfase;
            DibujarPrueba(cantidad==4?TextFormat("build/jade-aviso-%d.png",diseno):nullptr);
            Comprobar(dibujos[MODELO_JADE_FLECHA][0].color.r==235,"Aviso rojo de trampa");
            Vector3 dir=Vector3Transform({1,0,0},MatrixRotateY(dibujos[MODELO_JADE_FLECHA][0].angulo*DEG2RAD));
            Comprobar(Igual(dir,{trampa.direccionX,0,trampa.direccionZ}),"Flecha conserva direccion transpuesta/girada");
            ColocarEsfera(e,(trampa.columnaInicio+trampa.columnaFin)/2,(trampa.filaInicio+trampa.filaFin)/2);
            m.tiempoJuego=3.25f-trampa.desfase;
            Avanzar(1);
            Comprobar(e.velocidadX*trampa.direccionX+e.velocidadZ*trampa.direccionZ>0.1f,"Pulso empuja esfera en direccion logica");
            DibujarPrueba(cantidad==4?TextFormat("build/jade-disparo-%d.png",diseno):nullptr);
            Comprobar(ColorIsEqual(dibujos[MODELO_JADE_FLECHA][0].color,{255,150,40,255}),"Trampa naranja durante disparo");
            for (int r=0,encontrado=0; r<11 && !encontrado; r++) for (int c=0; c<11; c++)
                if (m.celdas[r][c]==CELDA_LABERINTO_AGUJERO) { ColocarEsfera(e,c,r); encontrado=1; break; }
            Avanzar(1);
            Comprobar(e.tiempoCaida>0 && e.caidas==1,"Caida detectada por celdas");
            Avanzar(20);
            DibujarPrueba(cantidad==4?TextFormat("build/jade-caida-%d.png",diseno):nullptr);
            const auto& caida=dibujos[MODELO_JADE_ESFERA][0];
            Comprobar(Cerca(caida.escala.x,e.tiempoCaida/0.7f) && Igual(caida.escala,{caida.escala.x,caida.escala.x,caida.escala.x}),"Escala uniforme de caida");
            Comprobar(Igual(caida.posicion,{e.x-5.5f,0.28f*caida.escala.x+0.03f,e.z-5.5f}),"Posicion y altura de esfera durante caida");
            Avanzar(25);
            Comprobar(Cerca(e.x,e.puntoRespawnX) && Cerca(e.z,e.puntoRespawnZ) && e.tiempoCaida==0,"Respawn en ultimo checkpoint");
            if (cantidad==4 && diseno==2)
            {
                DibujarPrueba();
                int presentes[15]; std::memcpy(presentes,cantidades,sizeof(presentes));
                for (int ausente=0; ausente<15; ausente++)
                {
                    almacen.laberintoJade[ausente].cargado=false;
                    DibujarPrueba();
                    for (int i=0; i<15; i++) Comprobar(cantidades[i]==(i==ausente?0:presentes[i]),"Fallback independiente por pieza");
                    almacen.laberintoJade[ausente].cargado=true;
                }
            }
            for (int j=0; j<cantidad; j++) ColocarEsfera(m.estadosJugadores[j],m.metaColumna,m.metaFila);
            Avanzar(1);
            Comprobar(m.llegadas==cantidad && m.fase==FASE_LABERINTO_TERMINADO && ResultadoMinijuegoFinalizado(m.resultado),"Llegada y final reales");
            DibujarPrueba(cantidad==4?TextFormat("build/jade-final-%d.png",diseno):nullptr);
            teclaSimulada=KEY_R; Avanzar(1); teclaSimulada=KEY_NULL;
            Comprobar(m.fase==FASE_LABERINTO_PREPARACION,"R reinicia minijuego");
            for (int i=0; i<15; i++) Comprobar(mallas[i]==almacen.laberintoJade[i].modelo.meshes,"Reinicio conserva recursos");
        }
        teclaSimulada=KEY_ESCAPE; Avanzar(1); teclaSimulada=KEY_NULL;
        Comprobar(zona.volverAlMenu,"ESC regresa al catalogo");
        zona.Inicializar(participantes,cantidad);
        zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_LABERINTO_INCLINADO);
        teclaSimulada=KEY_R; Avanzar(1); teclaSimulada=KEY_ESCAPE; Avanzar(1); teclaSimulada=KEY_NULL;
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Partida oficial respeta bloqueo de R/ESC");
        DibujarPrueba();
        for (int i=0; i<15; i++) Comprobar(mallas[i]==almacen.laberintoJade[i].modelo.meshes,"Selector/tablero reutilizan modelos");
        for (const auto& r:almacen.cajasPuerto) Comprobar(!r.cargaIntentada,"No cargar puerto con Jade");
        for (const auto& r:almacen.ultimoAsiento) Comprobar(!r.cargaIntentada,"No cargar feria con Jade");
        Comprobar(!almacen.montanaLava.cargaIntentada,"No cargar otros escenarios");
    }
    RecursoModeloEscenarioRetro3D ausente;
    for (int i=0; i<5; i++) PrepararSlotModeloEscenarioRetro3D(ausente,"build/jade-inexistente.glb",0);
    Comprobar(avisosAusente==1 && !ausente.cargado,"Carga fallida registrada una sola vez");
    observar=false;
    zona.Descargar(); zona.Descargar();
    for (const auto& r:almacen.laberintoJade) Comprobar(!r.cargado && !r.cargaIntentada && !r.modelo.meshes,"Descarga simetrica antes de CloseWindow");
    DescargarModeloJugadorCompartido();
    CloseWindow();
    std::printf("Laberinto Jade, 3 disenos x 2/3/4 jugadores: %d errores\n",errores);
    return errores==0?0:1;
}
