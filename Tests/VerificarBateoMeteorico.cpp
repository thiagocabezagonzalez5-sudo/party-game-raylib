// Prueba de integracion: ZonaPruebas real, contexto OpenGL y entradas simuladas.
// Los wrappers observan el dibujo sin reemplazar el renderizador por un visor.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <initializer_list>

static constexpr int N = CANTIDAD_MODELOS_BATEO_METEORICO_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoBateo
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoBateo dibujos[N][24]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().bateoMeteorico; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_BATEO_METEORICO_3D[i])==0) return i;
    return -1;
}
static void Registrar(int nivel, const char* formato, va_list args)
{
    char texto[2048]; std::vsnprintf(texto,sizeof(texto),formato,args);
    if (std::strstr(texto,"Modelo de escenario ausente")) avisos++;
    if (nivel >= LOG_WARNING) std::fprintf(stderr,"%s\n",texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int k) { return k==tecla || __real_IsKeyPressed(k); }
extern "C" bool __real_FileExists(const char*);
extern "C" bool __wrap_FileExists(const char* ruta)
{ return !(rutaAusente && std::strcmp(ruta,rutaAusente)==0) && __real_FileExists(ruta); }
extern "C" Model __real_LoadModel(const char*);
extern "C" Model __wrap_LoadModel(const char* ruta)
{ int i=Indice(ruta); if (i>=0) cargas[i]++; return __real_LoadModel(ruta); }
extern "C" void __real_UnloadModel(Model);
extern "C" void __wrap_UnloadModel(Model m)
{
    for (int i=0;i<N;i++) if (m.meshes && m.meshes==Recursos()[i].modelo.meshes) descargas[i]++;
    __real_UnloadModel(m);
}
extern "C" void __real_DrawModelEx(Model,Vector3,Vector3,float,Vector3,Color);
extern "C" void __wrap_DrawModelEx(Model m,Vector3 p,Vector3 eje,float angulo,Vector3 escala,Color tinte)
{
    for (int i=0;i<N;i++)
    {
        auto& r=Recursos()[i]; if (!r.cargado) continue;
        int malla=-1;
        for (int j=0;j<r.modelo.meshCount;j++) if (m.meshes==&r.modelo.meshes[j]) malla=j;
        if (malla<0) continue;
        int n=cantidades[i]++;
        Color color=r.materialColor<0 ? WHITE : m.materials[r.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
        Comprobar(n<24,"Limite de instancias");
        if (n<24) dibujos[i][n]={p,eje,escala,angulo,color,malla};
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global de materiales y vertices");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&m.transform,&identidad,sizeof(Matrix))==0,"Pivotes originales sin centrado");
        for (int j=0;j<m.materialCount && j<16;j++) if (j!=r.materialColor)
            Comprobar(ColorIsEqual(originales[i][j],m.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Materiales estaticos intactos");
        if (i==MODELO_BATEO_FAROL)
            Comprobar(m.meshCount==1 && m.meshMaterial==&r.modelo.meshMaterial[malla],"Farol comparte las mallas y materiales");
        break;
    }
    __real_DrawModelEx(m,p,eje,angulo,escala,tinte);
}
extern "C" void __real_DrawCube(Vector3,float,float,float,Color);
extern "C" void __wrap_DrawCube(Vector3 p,float x,float y,float z,Color c)
{ cubos++; __real_DrawCube(p,x,y,z,c); }
extern "C" void __real_DrawSphere(Vector3,float,Color);
extern "C" void __wrap_DrawSphere(Vector3 p,float r,Color c)
{ esferas++; __real_DrawSphere(p,r,c); }
extern "C" void __real_DrawCylinder(Vector3,float,float,float,int,Color);
extern "C" void __wrap_DrawCylinder(Vector3 p,float a,float b,float h,int l,Color c)
{ cilindros++; __real_DrawCylinder(p,a,b,h,l,c); }
extern "C" void __real_DrawCylinderEx(Vector3,Vector3,float,float,int,Color);
extern "C" void __wrap_DrawCylinderEx(Vector3 a,Vector3 b,float x,float y,int l,Color c)
{ segmentos++; __real_DrawCylinderEx(a,b,x,y,l,c); }
extern "C" void __real_DrawCircle3D(Vector3,float,Vector3,float,Color);
extern "C" void __wrap_DrawCircle3D(Vector3 p,float r,Vector3 eje,float a,Color c)
{ circulos++; __real_DrawCircle3D(p,r,eje,a,c); }
extern "C" void __real_DrawTriangle3D(Vector3,Vector3,Vector3,Color);
extern "C" void __wrap_DrawTriangle3D(Vector3 a,Vector3 b,Vector3 c,Color color)
{ triangulosSombra++; __real_DrawTriangle3D(a,b,c,color); }
static void GuardarMateriales()
{
    for (int i=0;i<N;i++) for (int j=0;j<Recursos()[i].modelo.materialCount && j<16;j++)
        originales[i][j]=Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
}
static void Dibujar(ZonaPruebas& zona,const char* captura=nullptr)
{
    std::memset(cantidades,0,sizeof(cantidades));
    cubos=esferas=cilindros=segmentos=circulos=triangulosSombra=0;
    auto& m=zona.gestorMinijuegos.minijuegoBateoMeteorico;
    unsigned char antes[sizeof(m)]; std::memcpy(antes,&m,sizeof(m));
    Vector3 posiciones[MAX_PARTICIPANTES], tamanos[MAX_PARTICIPANTES];
    for (int i=0;i<MAX_PARTICIPANTES;i++)
    { posiciones[i]=zona.jugadores[i].posicion; tamanos[i]=zona.jugadores[i].tamano; }
    BeginDrawing(); zona.Dibujar();
    if (captura)
    {
        rlDrawRenderBatchActive(); Image imagen=LoadImageFromScreen();
        Comprobar(ExportImage(imagen,captura),"Captura del juego integrado"); UnloadImage(imagen);
    }
    EndDrawing();
    Comprobar(std::memcmp(antes,&m,sizeof(m))==0,"El dibujo no cambia reglas, estado ni camara");
    for (int i=0;i<MAX_PARTICIPANTES;i++)
        Comprobar(Igual(posiciones[i],zona.jugadores[i].posicion) && Igual(tamanos[i],zona.jugadores[i].tamano),"Posiciones e hitboxes intactos");
    for (int i=0;i<N;i++) for (int j=0;j<Recursos()[i].modelo.materialCount && j<16;j++)
        Comprobar(ColorIsEqual(originales[i][j],Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Restaurar materiales despues del dibujo");
}
static void Avanzar(ZonaPruebas& zona,int frames)
{ for (int i=0;i<frames;i++) zona.Actualizar(1.0f/60); }
static void Pulsar(ZonaPruebas& zona,int k)
{ tecla=k; Avanzar(zona,1); tecla=KEY_NULL; }
static Vector3 Entrante(float x,LanzamientoBateo l,float reloj)
{
    float u=Clamp(reloj/l.duracion,0,(l.duracion+.45f)/l.duracion), f=std::pow(u,l.exponente);
    return {x+l.curva*std::sin(PI*Clamp(f,0,1)),fmaxf(.45f,5+(1.4f-5)*f),-12+11.3f*f};
}
static void ComprobarCarriles(const MinijuegoBateoMeteorico& m,int cantidad)
{
    for (int i=0;i<cantidad;i++)
    {
        float x=(i-(cantidad-1)*.5f)*6;
        Comprobar(Cerca(m.estadosJugadores[i].carrilX,x),"Carriles originales de 2/3/4 participantes");
        for (int pieza : {MODELO_BATEO_CARRIL,MODELO_BATEO_CAMPO})
        {
            Comprobar(cantidades[pieza]==cantidad && Igual(dibujos[pieza][i].posicion,{x,0,0}),"Pivote compartido de plataforma y campo");
            Comprobar(ColorIsEqual(dibujos[pieza][i].color,colores[i]),"Bordes segun jugador");
        }
        const auto& tubo=dibujos[MODELO_BATEO_CANON_TUBO][i];
        Vector3 direccion=Vector3Normalize({0,-3.6f,11.3f});
        Vector3 extremo=Vector3Add(tubo.posicion,Vector3Transform({0,2.2f,0},MatrixRotate(tubo.eje,tubo.angulo*DEG2RAD)));
        Comprobar(Igual(tubo.posicion,{x,4.7f,-12.4f}) && Igual(extremo,Vector3Add(tubo.posicion,Vector3Scale(direccion,2.2f))),"Canon orientado una sola vez desde su pivote");
        const auto& bate=dibujos[MODELO_BATEO_BATE][i];
        float swing=m.estadosJugadores[i].tiempoSwing;
        float angulo=swing>0 ? -60+Clamp((.26f-swing)/.12f,0,1)*140 : -70;
        Comprobar(Igual(bate.posicion,{x+.4f,1.05f,.7f}) && Cerca(bate.angulo,angulo) && ColorIsEqual(bate.color,colores[i]),"Swing y color del bate desde el estado");
        const auto& aro=dibujos[MODELO_BATEO_ARO][i];
        float pulso=.5f+.5f*std::sin(m.tiempoAnimacion*8);
        Comprobar(Igual(aro.posicion,{x,1.4f,-.7f}) && Cerca(aro.escala.x,(.55f+.06f*pulso)/.55f) && Cerca(aro.escala.z,1),"Aro centrado y pulso en su plano");
    }
    Comprobar(Igual(dibujos[MODELO_BATEO_OBSERVATORIO][0].posicion,{-36,0,-64}) &&
        Igual(dibujos[MODELO_BATEO_TELESCOPIO][0].posicion,{-36,0,-64}),"Cupula y telescopio comparten origen");
    for (int k=0;k<5;k++) for (int j=0;j<4;j++)
    {
        const auto& d=dibujos[MODELO_BATEO_FAROL][k*4+j];
        float p=.5f+.5f*std::sin(m.tiempoAnimacion*3+(k-2));
        float s=j==2 ? 1+p/6 : 1;
        Comprobar(Igual(d.posicion,{-15+k*6.0f,j==2 ? 1.97f*(1-s) : 0,3.2f}) && Cerca(d.escala.y,s),"Faroles fijos y bombillas con pivote propio");
        Comprobar(ColorIsEqual(d.color,{255,230,(unsigned char)(150+60*p),255}),"Parpadeo solo en BOMBILLAS");
    }
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Bateo Meteorico");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL); SetRandomSeed(719);
    static ZonaPruebas zona; Participante participantes[MAX_PARTICIPANTES]{};
    Mesh* mallas[N]{};
    for (int cantidad=2;cantidad<=4;cantidad++)
    {
        for (int i=0;i<MAX_PARTICIPANTES;i++)
        {
            participantes[i]={}; participantes[i].activo=i<cantidad;
            participantes[i].esBot=i>0; participantes[i].conectado=true;
            participantes[i].control=CONTROL_TECLADO_WASD; participantes[i].numeroJugador=i+1;
            participantes[i].color=colores[i];
        }
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=true;
        if (cantidad==2) for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"Carga diferida hasta activar Bateo");
        zona.CambiarMinijuego(MINIJUEGO_BATEO_METEORICO);
        auto& m=zona.gestorMinijuegos.minijuegoBateoMeteorico;
        for (int i=0;i<N;i++)
        {
            auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_BATEO_METEORICO_3D[i]);
            if (!r.cargado) { zona.Descargar(); CloseWindow(); return 2; }
            if (cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Instancias y rondas comparten recursos");
            for (int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Colores de vertices conservados");
        }
        GuardarMateriales(); Dibujar(zona,cantidad==4 ? "build/bateo-preparacion.png" : nullptr);
        // La ruta compartida del personaje ya dibuja sus propias sombras.
        // Se conserva esa base para medir solamente las del escenario.
        int triangulosJugadores=triangulosSombra;
        const int esperados[]={1,1,1,1,1,20,cantidad,cantidad,cantidad,cantidad,0,0,0,cantidad,cantidad};
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==esperados[i],"Piezas iniciales sin duplicados");
        if (cubos!=90 || esferas!=26 || cilindros!=cantidad || segmentos!=0)
            std::fprintf(stderr,"Primitivas iniciales: cubos=%d esferas=%d cilindros=%d segmentos=%d\n",cubos,esferas,cilindros,segmentos);
        Comprobar(cubos==90 && esferas==26 && cilindros==cantidad && segmentos==0,
            "Solo estrellas, luna y sombras originales de jugadores permanecen como primitivas");
        ComprobarCarriles(m,cantidad);
        for (int i=0;i<cantidad;i++)
        {
            int dorados=0,rojos=0;
            for (auto tipo:m.estadosJugadores[i].tipos) { dorados+=tipo==METEORITO_DORADO; rojos+=tipo==METEORITO_ROJO; }
            Comprobar(dorados==1 && rojos==1,"Distribucion original de tipos");
        }
        Avanzar(zona,182); Comprobar(m.fase==FASE_BATEO_JUGANDO,"Preparacion real completa");
        for (int ronda=0;ronda<5;ronda++)
        {
            Comprobar(m.ronda==ronda && m.etapa==ETAPA_BATEO_AVISO,"Cinco lanzamientos reales");
            TipoMeteoritoBateo tipo=ronda==1 ? METEORITO_DORADO : ronda==2 || ronda==3 ? METEORITO_ROJO : METEORITO_NORMAL;
            for (int i=0;i<cantidad;i++) m.estadosJugadores[i].tipos[ronda]=tipo;
            Dibujar(zona);
            int espera=0; while (m.etapa!=ETAPA_BATEO_VUELO && espera++<100) Avanzar(zona,1);
            Comprobar(m.etapa==ETAPA_BATEO_VUELO,"Aviso termina en vuelo");
            for (int i=0;i<cantidad;i++) { auto& e=m.estadosJugadores[i]; e.botGolpeara=false; }
            // Observacion de trayectoria curva y velocidad procedural antes del golpe.
            Avanzar(zona,15); Dibujar(zona,cantidad==4 && ronda==2 ? "build/bateo-rojo.png" : nullptr);
            int pieza=tipo==METEORITO_NORMAL ? MODELO_BATEO_METEORITO_NORMAL : tipo==METEORITO_DORADO ? MODELO_BATEO_METEORITO_DORADO : MODELO_BATEO_METEORITO_ROJO;
            Comprobar(cantidades[pieza]==cantidad,"Meteorito del tipo actual por carril");
            int sombras=0;
            for (int i=0;i<cantidad;i++)
                sombras+=(tipo==METEORITO_NORMAL ? .45f : .5f)>=.10f*EscalaUmbralSombraPropRetro();
            Comprobar(triangulosSombra==triangulosJugadores+sombras*LadosSombraPropRetro(),
                "Una sombra por meteorito; luces, halos y estelas sin sombras duplicadas");
            for (int i=0;i<cantidad;i++)
            {
                const auto& e=m.estadosJugadores[i];
                Comprobar(Igual(dibujos[pieza][i].posicion,Entrante(e.carrilX,m.lanzamientos[ronda],e.reloj)),"Modelo sigue trayectoria entrante real");
                float s=tipo==METEORITO_ROJO ? 1+.12f*(.5f+.5f*std::sin(m.tiempoAnimacion*14)) : 1;
                Comprobar(Cerca(dibujos[pieza][i].escala.x,s),"Pulso inestable conservado");
            }
            int puntosAntes=m.estadosJugadores[0].puntos;
            if (ronda!=3)
            {
                // Entrada humana, y en ronda 5 mando ausente con IA existente.
                auto& e=m.estadosJugadores[0]; e.reloj=m.lanzamientos[ronda].duracion-1.0f/60;
                if (ronda==4)
                {
                    participantes[0].control=CONTROL_GAMEPAD; participantes[0].indiceGamepad=-1;
                    e.botGolpeara=true; e.instanteGolpeBot=m.lanzamientos[ronda].duracion;
                    Avanzar(zona,1);
                }
                else Pulsar(zona,KEY_E);
                Comprobar(e.estado==(ronda==2 ? LANZAMIENTO_EXPLOTADO : LANZAMIENTO_GOLPEADO),"Golpe por input real o IA desconectada");
                Dibujar(zona,cantidad==4 && ronda==2 ? "build/bateo-explosion.png" : nullptr);
                ComprobarCarriles(m,cantidad);
                if (ronda==2) Comprobar(cantidades[pieza]==cantidad-1 && e.puntos==puntosAntes-50,"Explosion oculta superficie y resta 50");
                else
                {
                    Avanzar(zona,25); Dibujar(zona,cantidad==4 && ronda==1 ? "build/bateo-dorado.png" : nullptr);
                    float u=e.tiempoVuelo/e.duracionVuelo;
                    Vector3 p={e.puntoGolpe.x+e.desvio*u,1.4f+(3+.35f*e.distancia)*4*u*(1-u),e.puntoGolpe.z-e.distancia*u};
                    Comprobar(Igual(dibujos[pieza][0].posicion,p),"Modelo sigue arco golpeado");
                }
            }
            espera=0; while (m.etapa!=ETAPA_BATEO_PAUSA && espera++<500) Avanzar(zona,1);
            Comprobar(m.etapa==ETAPA_BATEO_PAUSA,"Lanzamiento llega a estado terminal");
            if (ronda==3) Comprobar(m.estadosJugadores[0].mensaje==MENSAJE_BATEO_EVITADO,"Rojo evitado sin golpear");
            else if (ronda!=2) Comprobar(m.estadosJugadores[0].puntos==puntosAntes+(ronda==1 ? 200 : 100),"Anillo 100 y dorado doble");
            Dibujar(zona);
            int pasadosVisibles=0;
            for (int i=0;i<cantidad;i++)
                pasadosVisibles+=m.estadosJugadores[i].estado==LANZAMIENTO_PASADO &&
                    m.estadosJugadores[i].reloj<=m.lanzamientos[ronda].duracion+.45f;
            Comprobar(cantidades[MODELO_BATEO_METEORITO_NORMAL]+cantidades[MODELO_BATEO_METEORITO_DORADO]+cantidades[MODELO_BATEO_METEORITO_ROJO]==pasadosVisibles,"Ocultar aterrizados/explotados y conservar pasada visible");
            if (ronda==4) Comprobar(m.estadosJugadores[1].mensaje==MENSAJE_BATEO_SIN_GOLPE,"Fallo conserva mensaje y puntaje");
            espera=0; while (m.etapa==ETAPA_BATEO_PAUSA && m.fase!=FASE_BATEO_TERMINADO && espera++<100) Avanzar(zona,1);
        }
        Comprobar(m.fase==FASE_BATEO_TERMINADO && m.estadosJugadores[0].puntos==350 && m.resultado.participantes[0].posicionFinal==1,"Final y ganador originales");
        Dibujar(zona,cantidad==4 ? "build/bateo-final.png" : nullptr);
        Pulsar(zona,KEY_R); Comprobar(m.fase==FASE_BATEO_PREPARACION && m.ronda==0,"R reinicia sin cargar recursos");
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Salida al selector/menu");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_BATEO_METEORICO); Pulsar(zona,KEY_R);
        Comprobar(m.tiempoPreparacion<3,"Ronda de tablero no reinicia con R");
        for (int i=0;i<N;i++) Comprobar(cargas[i]==1 && mallas[i]==Recursos()[i].modelo.meshes,"Reentrada, reinicio y tablero sin duplicar cargas");
    }
    // Una pieza ausente a la vez; el disco y las otras piezas quedan intactos.
    auto& m=zona.gestorMinijuegos.minijuegoBateoMeteorico;
    m.fase=FASE_BATEO_JUGANDO; m.etapa=ETAPA_BATEO_VUELO; m.ronda=0;
    for (int i=0;i<4;i++) { auto& e=m.estadosJugadores[i]; e.estado=LANZAMIENTO_EN_VUELO; e.tipoActual=(TipoMeteoritoBateo)(i%3); e.reloj=.4f; }
    Dibujar(zona); int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes));
    int baseCubos=cubos,baseEsferas=esferas,baseCilindros=cilindros,baseSegmentos=segmentos,baseCirculos=circulos;
    for (int ausente=0;ausente<N;ausente++)
    {
        DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]); rutaAusente=RUTAS_MODELOS_BATEO_METEORICO_3D[ausente];
        for (int j=0;j<3;j++) CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteBateoMeteoricoRetro3D());
        Dibujar(zona);
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback independiente por pieza");
        Comprobar(cubos>baseCubos || esferas>baseEsferas || cilindros>baseCilindros || segmentos>baseSegmentos || circulos>baseCirculos,"Primitivas sustituyen GLB fallido");
        Comprobar(Recursos()[ausente].cargaIntentada && !Recursos()[ausente].cargado,"Fallo cacheado sin reintento por frame");
        rutaAusente=nullptr; Recursos()[ausente]={};
        CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteBateoMeteoricoRetro3D()); GuardarMateriales();
    }
    Comprobar(avisos==N,"Un diagnostico por archivo ausente");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
    for (const auto& r:almacen.ultimoAsiento) Comprobar(!r.cargaIntentada,"No cargar Asiento");
    for (const auto& r:almacen.cajasPuerto) Comprobar(!r.cargaIntentada,"No cargar Puerto");
    for (const auto& r:almacen.laberintoJade) Comprobar(!r.cargaIntentada,"No cargar Jade");
    for (const auto& r:almacen.vetaCristal) Comprobar(!r.cargaIntentada,"No cargar Veta");
    for (const auto& r:almacen.capsulasBarajadas) Comprobar(!r.cargaIntentada,"No cargar Capsulas");
    zona.Descargar(); zona.Descargar();
    for (int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Bateo Meteorico 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
