// Prueba del juego integrado: ZonaPruebas real, OpenGL y entradas simuladas.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>

static constexpr int N = CANTIDAD_MODELOS_PISOTON_PLAGAS_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0, primitivas = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{}, usos[N]{};
static const char* ausente = nullptr;
static Color originales[N][16]{};
static unsigned long long firmas[N]{};
struct InstanciaPisoton { Vector3 posicion{}, escala{}; float angulo = 0; Color color{}; int malla = -1; };
static InstanciaPisoton dibujos[N][128]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().pisotonPlagas; }
// El jardin se hornea con primitivas propias: del paquete solo se cargan las
// piezas jugables (brote, flor y su aro; madriguera, aviso, plagas y avispas).
static bool Jugable(int i)
{ return (i>=MODELO_PISOTON_BROTE && i<=MODELO_PISOTON_ARO_FLOR) || (i>=MODELO_PISOTON_MADRIGUERA && i<=MODELO_PISOTON_ALA); }
static void Comprobar(bool ok, const char* texto)
{ if (!ok) { if (errores < 40) std::fprintf(stderr,"FALLO: %s\n",texto); errores++; } }
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b) { return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{ for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_PISOTON_PLAGAS_3D[i])==0) return i; return -1; }
static unsigned long long Firma(Model modelo)
{
    unsigned long long h = 1469598103934665603ULL;
    for (int i=0;i<modelo.meshCount;i++)
    {
        const Mesh& m = modelo.meshes[i];
        for (const float* datos : {m.vertices,m.normals}) if (datos)
        {
            const auto* bytes=reinterpret_cast<const unsigned char*>(datos);
            for (int b=0;b<m.vertexCount*3*(int)sizeof(float);b++) h=(h^bytes[b])*1099511628211ULL;
        }
        if (m.colors) for (int b=0;b<m.vertexCount*4;b++) h=(h^m.colors[b])*1099511628211ULL;
    }
    return h;
}
static void Registrar(int nivel, const char* formato, va_list args)
{
    char texto[2048]; std::vsnprintf(texto,sizeof(texto),formato,args);
    if (std::strstr(texto,"Modelo de escenario ausente")) avisos++;
    if (nivel>=LOG_WARNING) std::fprintf(stderr,"%s\n",texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int k) { return k==tecla || __real_IsKeyPressed(k); }
extern "C" bool __real_IsKeyDown(int);
extern "C" bool __wrap_IsKeyDown(int k) { return k==tecla || __real_IsKeyDown(k); }
extern "C" bool __real_FileExists(const char*);
extern "C" bool __wrap_FileExists(const char* p)
{ return !(ausente && std::strcmp(p,ausente)==0) && __real_FileExists(p); }
extern "C" Model __real_LoadModel(const char*);
extern "C" Model __wrap_LoadModel(const char* p)
{ int i=Indice(p); if (i>=0) cargas[i]++; return __real_LoadModel(p); }
extern "C" void __real_UnloadModel(Model);
extern "C" void __wrap_UnloadModel(Model modelo)
{ for (int i=0;i<N;i++) if (modelo.meshes && modelo.meshes==Recursos()[i].modelo.meshes) descargas[i]++; __real_UnloadModel(modelo); }
extern "C" void __real_DrawModelEx(Model,Vector3,Vector3,float,Vector3,Color);
extern "C" void __wrap_DrawModelEx(Model modelo,Vector3 p,Vector3 eje,float giro,Vector3 escala,Color tinte)
{
    for (int i=0;i<N;i++) if (Recursos()[i].cargado)
    {
        auto& r=Recursos()[i]; int malla=-1;
        for (int j=0;j<r.modelo.meshCount;j++) if (modelo.meshes==&r.modelo.meshes[j]) malla=j;
        if (malla<0) continue;
        int n=cantidades[i]++; usos[i]++;
        int material=r.materialColor>=0 ? r.materialColor : modelo.meshMaterial[0];
        if (n<128) dibujos[i][n]={Vector3Transform(p,rlGetMatrixTransform()),escala,giro,
            modelo.materials[material].maps[MATERIAL_MAP_DIFFUSE].color,malla};
        bool opacidad=i==MODELO_PISOTON_ROCIO || i==MODELO_PISOTON_ARO_FLOR;
        Color rgb=tinte; rgb.a=255;
        Comprobar(ColorIsEqual(rgb,WHITE) && (opacidad || tinte.a==255),"Sin tinte RGB global; solo rocio y aro pulsan opacidad de instancia");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&modelo.transform,&identidad,sizeof(Matrix))==0,"Pivotes originales sin normalizacion");
        for (int j=0;j<modelo.materialCount;j++) if (j!=r.materialColor)
        {
            Comprobar(ColorIsEqual(modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color,originales[i][j]),
                "Material original durante cada dibujo; solo COLOR_DINAMICO recibe color de estado");
        }
        break;
    }
    __real_DrawModelEx(modelo,p,eje,giro,escala,tinte);
}
extern "C" void __real_DrawCube(Vector3,float,float,float,Color);
extern "C" void __wrap_DrawCube(Vector3 p,float x,float y,float z,Color c)
{ primitivas++; __real_DrawCube(p,x,y,z,c); }
extern "C" void __real_DrawSphere(Vector3,float,Color);
extern "C" void __wrap_DrawSphere(Vector3 p,float r,Color c)
{ primitivas++; __real_DrawSphere(p,r,c); }
extern "C" void __real_DrawCylinder(Vector3,float,float,float,int,Color);
extern "C" void __wrap_DrawCylinder(Vector3 p,float a,float b,float h,int n,Color c)
{ primitivas++; __real_DrawCylinder(p,a,b,h,n,c); }
extern "C" void __real_DrawCylinderEx(Vector3,Vector3,float,float,int,Color);
extern "C" void __wrap_DrawCylinderEx(Vector3 p,Vector3 q,float a,float b,int n,Color c)
{ primitivas++; __real_DrawCylinderEx(p,q,a,b,n,c); }
extern "C" void __real_DrawCircle3D(Vector3,float,Vector3,float,Color);
extern "C" void __wrap_DrawCircle3D(Vector3 p,float r,Vector3 eje,float giro,Color c)
{ primitivas++; __real_DrawCircle3D(p,r,eje,giro,c); }
extern "C" void __real_DrawLine3D(Vector3,Vector3,Color);
extern "C" void __wrap_DrawLine3D(Vector3 p,Vector3 q,Color c)
{ primitivas++; __real_DrawLine3D(p,q,c); }
extern "C" void __real_DrawPlane(Vector3,Vector2,Color);
extern "C" void __wrap_DrawPlane(Vector3 p,Vector2 s,Color c)
{ primitivas++; __real_DrawPlane(p,s,c); }
static void GuardarRecursos()
{
    for (int i=0;i<N;i++) if (Recursos()[i].cargado)
    {
        firmas[i]=Firma(Recursos()[i].modelo);
        for (int j=0;j<Recursos()[i].modelo.materialCount;j++) originales[i][j]=Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
    }
}
static void Dibujar(ZonaPruebas& zona, const char* captura=nullptr)
{
    auto& m=zona.gestorMinijuegos.minijuegoPisotonPlagas;
    unsigned char estado[sizeof(m)], jugadores[sizeof(zona.jugadores)];
    std::memcpy(estado,&m,sizeof(m)); std::memcpy(jugadores,zona.jugadores,sizeof(jugadores));
    std::memset(cantidades,0,sizeof(cantidades)); primitivas=0;
    BeginDrawing(); zona.Dibujar();
    if (captura) { rlDrawRenderBatchActive(); Image img=LoadImageFromScreen(); Comprobar(ExportImage(img,captura),"Captura del minijuego integrado"); UnloadImage(img); }
    EndDrawing();
    Comprobar(std::memcmp(estado,&m,sizeof(m))==0,"El dibujo conserva logica, tiempos, puntuacion y camara");
    Comprobar(std::memcmp(jugadores,zona.jugadores,sizeof(jugadores))==0,"Jugadores y hitboxes intactos");
    for (int i=0;i<N;i++) if (Recursos()[i].cargado)
    {
        Comprobar(firmas[i]==Firma(Recursos()[i].modelo),"Vertices, normales y colores originales restaurados tras cada instancia");
        for (int j=0;j<Recursos()[i].modelo.materialCount;j++)
            Comprobar(ColorIsEqual(originales[i][j],Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Material restaurado tras el dibujo");
    }
    int avispas=0;
    for (const auto& a:m.avispas) if (a.activa)
    {
        if (Recursos()[MODELO_PISOTON_AVISPA].cargado)
            Comprobar(Igual(dibujos[MODELO_PISOTON_AVISPA][avispas].posicion,{a.x,a.altura,a.z}),"Cuerpo sigue altura y movimiento reales de avispa");
        if (Recursos()[MODELO_PISOTON_ALA].cargado) for (int lado=-1;lado<=1;lado+=2)
        {
            const auto& d=dibujos[MODELO_PISOTON_ALA][avispas*2+(lado+1)/2];
            Comprobar(Igual(d.posicion,{a.x-a.dirZ*.22f*lado,a.altura+.17f+std::sin(m.tiempoAnimacion*55)*.12f,a.z+a.dirX*.22f*lado}),"Alas conservan aleteo, lado y altura sin doble transformacion");
            Comprobar(Cerca(d.angulo,-std::atan2(a.dirZ,a.dirX)*RAD2DEG+(lado<0 ? 180 : 0)),"Alas miran hacia fuera del cuerpo");
        }
        avispas++;
    }
    Comprobar(cantidades[MODELO_PISOTON_ALA]==(Recursos()[MODELO_PISOTON_ALA].cargado ? avispas*2 : 0),"Dos alas compartidas por avispa activa");
}
static void Avanzar(ZonaPruebas& zona, int frames)
{ for (int i=0;i<frames;i++) zona.Actualizar(1.0f/60); }
static void Preparar(ZonaPruebas& zona)
{ tecla=KEY_NULL; zona.CambiarMinijuego(MINIJUEGO_PISOTON_PLAGAS); Avanzar(zona,181); }
static void Aislar(ZonaPruebas& zona)
{
    Preparar(zona); auto& m=zona.gestorMinijuegos.minijuegoPisotonPlagas;
    for (auto& p:m.plagas) p={};
    for (auto& a:m.avispas) { a={}; a.retrasoAparicion=1000; }
    for (auto& h:m.madrigueras) h.aviso=0;
    for (auto& p:m.particulas) p={};
    m.tiempoFlor=m.proximaAparicion=1000;
    for (int i=0;i<4;i++) { auto& j=zona.jugadores[i]; j.posicion={i ? 6.0f : 0,.65f,i ? i*1.2f : 0}; j.velocidad={}; j.enSuelo=true; }
}
static PlagaPisoton Plaga(TipoPlagaPisoton tipo)
{
    PlagaPisoton p{}; p.activa=true; p.tipo=tipo; p.aparicion=p.escala=1;
    p.vidaMaxima=1000; p.aturdida=99; p.edad=.7f;
    for (int s=0;s<3;s++) { p.segVivo[s]=s==0 || tipo==PLAGA_PISOTON_ORUGA; p.segX[s]=s*.55f; }
    return p;
}
static void Pisoton(ZonaPruebas& zona)
{
    tecla=KEY_SPACE; Avanzar(zona,1); tecla=KEY_NULL; Avanzar(zona,16);
    tecla=KEY_SPACE; Avanzar(zona,1); tecla=KEY_NULL;
    Comprobar(zona.jugadores[0].preparandoGolpeSuelo || zona.jugadores[0].golpeSueloActivo,"Segundo salto activa ground pound real");
    auto& m=zona.gestorMinijuegos.minijuegoPisotonPlagas;
    for (int k=0;k<90 && m.puntos[0]==0;k++) Avanzar(zona,1);
}
static void EscenaCompleta(ZonaPruebas& zona, bool brote)
{
    Aislar(zona); auto& m=zona.gestorMinijuegos.minijuegoPisotonPlagas;
    m.tiempoAnimacion=1.37f; m.tiempoFlor=brote ? 4 : 10; m.florAbierta=brote ? 0 : 1;
    m.florX=2; m.florZ=-2;
    for (int i=0;i<3;i++) { auto& p=m.plagas[i]; p=Plaga(static_cast<TipoPlagaPisoton>(i)); p.aturdida=0; for (float& x:p.segX) x+=-3+i*3; }
    for (int i=0;i<2;i++) { auto& a=m.avispas[i]; a.activa=true; a.x=-2+i*4; a.z=2; a.dirX=.6f; a.dirZ=.8f; a.altura=.88f+i*.14f; }
    for (auto& h:m.madrigueras) h.aviso=.6f;
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING); SetRandomSeed(2107);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Pisoton de Plagas: prueba integrada");
    if (!IsWindowReady()) return 2;
    Participante p[4]{}; ZonaPruebas zona; zona.Inicializar(p,4); zona.modoCatalogo=true;
    auto& m=zona.gestorMinijuegos.minijuegoPisotonPlagas;
    p[0].activo=true; m.Reiniciar(zona.jugadores,p,4);
    Comprobar(m.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Un participante cancela la ronda");
    for (int i=0;i<N;i++) Comprobar(cargas[i]==0,"Catalogo y ronda invalida no cargan el paquete");
    for (int cantidad=2;cantidad<=4;cantidad++)
    {
        for (int i=0;i<4;i++) { p[i].activo=i<cantidad; p[i].conectado=true; p[i].esBot=false; p[i].numeroJugador=i+1; p[i].color=Color{(unsigned char)(80+i*40),120,220,255}; p[i].control=i==0 ? CONTROL_TECLADO_WASD : CONTROL_TECLADO_FLECHAS; }
        zona.cantidadParticipantes=cantidad; zona.CambiarMinijuego(MINIJUEGO_PISOTON_PLAGAS);
        for (int i=0;i<N;i++) Comprobar(Jugable(i) ? (Recursos()[i].cargado && cargas[i]==1) : (!Recursos()[i].cargado && cargas[i]==0),"Piezas jugables cargadas una sola vez entre rondas y participantes; decorado sin GLB");
        GuardarRecursos();
        char ruta[100]; std::snprintf(ruta,sizeof(ruta),"build/pisoton-preparacion-%d.png",cantidad); Dibujar(zona,ruta);
        Comprobar(cantidades[MODELO_PISOTON_MADRIGUERA]==6,"Piezas conservan cantidades originales");
        Avanzar(zona,181); Comprobar(m.fase==FASE_PISOTON_JUGANDO,"Cuenta regresiva inicia la partida");
        float x=zona.jugadores[0].posicion.x; tecla=KEY_D; Avanzar(zona,15); tecla=KEY_NULL;
        Comprobar(zona.jugadores[0].posicion.x>x,"Movimiento conserva controles reales");
        for (int tipo=0;tipo<3;tipo++)
        {
            Aislar(zona); m.plagas[0]=Plaga(static_cast<TipoPlagaPisoton>(tipo)); Pisoton(zona);
            Comprobar(!m.plagas[0].activa && m.puntos[0]==(tipo==0 ? 1 : tipo==1 ? 6 : 5),"Pisoton elimina plaga y conserva puntuacion original");
            Comprobar(m.doradas[0]==(tipo==2 ? 1 : 0),"Babosa conserva desempate por doradas");
            Dibujar(zona,cantidad==4 && tipo==2 ? "build/pisoton-impacto.png" : nullptr);
        }
        Aislar(zona); m.plagas[0]=Plaga(PLAGA_PISOTON_ORUGA); m.plagas[0].segX[1]=2; m.plagas[0].segX[2]=3; Pisoton(zona); Dibujar(zona);
        Comprobar(!m.plagas[0].segVivo[0] && m.plagas[0].segVivo[1] && m.puntos[0]==2,"Golpe parcial conserva segmentos restantes");
        Comprobar(cantidades[MODELO_PISOTON_ORUGA_CABEZA]==1 && Cerca(dibujos[MODELO_PISOTON_ORUGA_CABEZA][0].posicion.x,m.plagas[0].segX[1]),"La cabeza pasa al primer segmento vivo");
        Aislar(zona); m.plagas[0]=Plaga(PLAGA_PISOTON_ESCARABAJO); m.plagas[0].aturdida=0;
        zona.jugadores[0].posicion.y=1.05f; zona.jugadores[0].velocidad.y=-2; zona.jugadores[0].enSuelo=false;
        Avanzar(zona,1); Comprobar(m.plagas[0].aturdida>2 && m.puntos[0]==0 && zona.jugadores[0].velocidad.y>0,"Salto normal solo aturde y rebota"); Dibujar(zona);
        Aislar(zona); m.puntos[0]=5; auto& a=m.avispas[0]; a.activa=true; a.x=a.z=0; a.objetivo=0; a.tiempoObjetivo=1000;
        Avanzar(zona,1); Comprobar(m.puntos[0]==3 && m.inmunidadAvispa[0]>0 && zona.jugadores[0].tiempoRalentizado>0,"Picadura aplica penalizacion e inmunidad"); Dibujar(zona);
        Aislar(zona); m.avispas[0].activa=true; m.avispas[0].x=.8f; m.avispas[0].objetivo=0; m.avispas[0].tiempoObjetivo=1000; zona.jugadores[0].direccionMirada={1,0,0};
        tecla=KEY_E; Avanzar(zona,1); tecla=KEY_NULL;
        // El golpe queda activo tras la anticipacion (DURACION_ANTICIPACION_GOLPE).
        Avanzar(zona,5);
        Comprobar(m.avispas[0].huida>2 && m.puntos[0]==0,"Golpe real espanta avispa sin cambiar puntaje"); Dibujar(zona);
        Aislar(zona); m.florX=m.florZ=0; m.tiempoFlor=.001f; Avanzar(zona,1);
        Comprobar(m.puntos[0]==3 && m.florGanador==0 && m.florAbierta>0,"Flor premia cercania con reglas originales"); Dibujar(zona,cantidad==4 ? "build/pisoton-flor.png" : nullptr);
        Aislar(zona); m.madrigueras[0].aviso=.001f; Avanzar(zona,1);
        Comprobar(m.plagas[0].activa && m.madrigueras[0].aviso==0,"Aviso genera plaga desde madriguera real"); Dibujar(zona);
        m.plagas[0].escapando=true; m.plagas[0].escala=.2f; Avanzar(zona,60); Comprobar(!m.plagas[0].activa,"Escape retira plaga y modelo");
        EscenaCompleta(zona,false); Dibujar(zona,cantidad==4 ? "build/pisoton-escena.png" : nullptr);
        Comprobar(cantidades[MODELO_PISOTON_FLOR_ABIERTA]==4 && cantidades[MODELO_PISOTON_ARO_FLOR]==1,"Flor abierta usa tallo, petalos y centro del GLB");
        m.florAbierta=0; m.tiempoFlor=1; Dibujar(zona,cantidad==4 ? "build/pisoton-apertura.png" : nullptr);
        Comprobar(cantidades[MODELO_PISOTON_FLOR_ABIERTA]==4 && cantidades[MODELO_PISOTON_ARO_FLOR]==0,"Apertura gradual conserva indicador de aviso");
        m.tiempoFlor=4; Dibujar(zona,cantidad==4 ? "build/pisoton-brote.png" : nullptr);
        Comprobar(cantidades[MODELO_PISOTON_BROTE]==1 && cantidades[MODELO_PISOTON_FLOR_ABIERTA]==0,"Brote sigue crecimiento antes de los petalos");
        m.tiempoRestante=.001f; Avanzar(zona,1); Dibujar(zona,cantidad==4 ? "build/pisoton-final.png" : nullptr);
        Comprobar(m.fase==FASE_PISOTON_TERMINADO && m.resultado.estado==RESULTADO_MINIJUEGO_FINALIZADO,"Tiempo termina partida y presenta resultado");
        tecla=KEY_R; Avanzar(zona,1); tecla=KEY_NULL; Comprobar(m.fase==FASE_PISOTON_PREPARACION && m.puntos[0]==0,"R reinicia fase y puntuacion");
        zona.modoTablero=true; tecla=KEY_R; Avanzar(zona,1); tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Ronda oficial conserva proteccion de R y ESC");
        zona.modoTablero=false; tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL; Comprobar(zona.volverAlMenu,"ESC vuelve al selector"); zona.volverAlMenu=false;
        for (int i=0;i<cantidad;i++) p[i].esBot=true;
        Preparar(zona); Avanzar(zona,2800); Comprobar(m.fase==FASE_PISOTON_TERMINADO,"IA completa ronda con 2/3/4 participantes");
        int total=0; for (int puntos:m.puntos) total+=puntos; Comprobar(total>0,"Bots consiguen puntos con mecanicas reales");
    }
    for (auto& participante:p) participante.esBot=false;
    for (int pieza=0;pieza<N;pieza++) if (Jugable(pieza))
    {
        EscenaCompleta(zona,pieza==MODELO_PISOTON_BROTE); Dibujar(zona);
        int base=primitivas, previo[N]; std::memcpy(previo,cantidades,sizeof(previo));
        Comprobar(previo[pieza]>0,"Cada GLB reemplaza un elemento visible del minijuego");
        DescargarSlotModeloEscenarioRetro3D(Recursos()[pieza]); ausente=RUTAS_MODELOS_PISOTON_PLAGAS_3D[pieza]; int avisosAntes=avisos;
        CargarPiezasJugablesPisotonPlagas(); CargarPiezasJugablesPisotonPlagas(); CargarPiezasJugablesPisotonPlagas();
        Comprobar(avisos==avisosAntes+1,"Un diagnostico por archivo ausente");
        Dibujar(zona); Comprobar(cantidades[pieza]==0 && primitivas>base,"Fallback por pieza sin GLB superpuesto");
        for (int i=0;i<N;i++) if (i!=pieza) Comprobar(cantidades[i]==previo[i],"Fallo local conserva las otras piezas");
        Dibujar(zona); Comprobar(avisos==avisosAntes+1,"Sin repeticion del diagnostico por frame");
        ausente=nullptr; Recursos()[pieza]={}; CargarPiezasJugablesPisotonPlagas(); GuardarRecursos();
    }
    for (int i=0;i<N;i++) if (Jugable(i)) Comprobar(usos[i]>0,"Las piezas jugables se usan en el minijuego real");
    zona.Descargar(); zona.Descargar();
    for (int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    for (const auto& reposo:ObtenerModelosEscenariosRetro3D().animacionesPisoton)
        Comprobar(!reposo.vertices && !reposo.normales,"Reposos CPU de animacion liberados");
    CloseWindow();
    std::printf("Verificacion Pisoton de Plagas 2/3/4 participantes: %d errores\n",errores);
    return errores ? 1 : 0;
}
