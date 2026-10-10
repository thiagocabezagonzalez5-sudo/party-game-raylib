// ZonaPruebas y renderizador reales; entradas simuladas y observacion del dibujo.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>

static constexpr int N = CANTIDAD_MODELOS_RODILLOS_NEON_3D;
static int errores=0, tecla=KEY_NULL, avisos=0, primitivos=0;
static int cargas[N]{}, descargas[N]{}, dibujos[N]{}, usos[N]{};
static const char* ausente=nullptr;
static Color materiales[N][16]{};
static float tiempo=0, alturaBanda=0;
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().rodillosNeon; }
static bool Cerca(float a,float b) { return std::fabs(a-b)<.002f; }
static void Comprobar(bool ok,const char* texto)
{ if(!ok) { if(errores<30) std::fprintf(stderr,"FALLO: %s\n",texto); errores++; } }
static int Indice(const char* ruta)
{ for(int i=0;i<N;i++) if(std::strcmp(ruta,RUTAS_MODELOS_RODILLOS_NEON_3D[i])==0) return i; return -1; }
static void Registrar(int nivel,const char* formato,va_list args)
{
    char texto[2048]; std::vsnprintf(texto,sizeof(texto),formato,args);
    if(std::strstr(texto,"Modelo de escenario ausente")) avisos++;
    if(nivel>=LOG_WARNING) std::fprintf(stderr,"%s\n",texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int k) { return k==tecla || __real_IsKeyPressed(k); }
extern "C" bool __real_FileExists(const char*);
extern "C" bool __wrap_FileExists(const char* p)
{ return !(ausente && std::strcmp(p,ausente)==0) && __real_FileExists(p); }
extern "C" Model __real_LoadModel(const char*);
extern "C" Model __wrap_LoadModel(const char* p)
{ int i=Indice(p); if(i>=0) cargas[i]++; return __real_LoadModel(p); }
extern "C" void __real_UnloadModel(Model);
extern "C" void __wrap_UnloadModel(Model m)
{ for(int i=0;i<N;i++) if(m.meshes && m.meshes==Recursos()[i].modelo.meshes) descargas[i]++; __real_UnloadModel(m); }
struct Instancia { Vector3 p{},eje{},escala{}; float angulo=0; Matrix matriz{}; Color color{}; };
static Instancia instancias[N][128]{};
extern "C" void __real_DrawModelEx(Model,Vector3,Vector3,float,Vector3,Color);
extern "C" void __wrap_DrawModelEx(Model m,Vector3 p,Vector3 eje,float giro,Vector3 escala,Color tinte)
{
    for(int i=0;i<N;i++) if(Recursos()[i].cargado && m.meshes==Recursos()[i].modelo.meshes)
    {
        auto& r=Recursos()[i]; int n=dibujos[i]++; usos[i]++;
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global sobre materiales y vertices");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&m.transform,&identidad,sizeof(Matrix))==0,"Pivote importado sin centrado");
        int dinamico=r.materialColor;
        if(i>=MODELO_RODILLOS_TRIANGULO && i<=MODELO_RODILLOS_ESTRELLA) dinamico=m.meshMaterial[0];
        for(int j=0;j<m.materialCount;j++) if(j!=dinamico)
            Comprobar(ColorIsEqual(materiales[i][j],m.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Materiales estaticos originales");
        if(n<128) instancias[i][n]={Vector3Transform(p,rlGetMatrixTransform()),eje,escala,giro,
            rlGetMatrixTransform(),m.materials[dinamico>=0?dinamico:0].maps[MATERIAL_MAP_DIFFUSE].color};
        if(i==MODELO_RODILLOS_LINEA)
        {
            auto& a=ObtenerModelosEscenariosRetro3D().animacionesRodillos[3]; Mesh& mesh=m.meshes[1];
            for(int v=0;v<mesh.vertexCount;v++)
            {
                float y=a.vertices[v*3+1];
                Comprobar(Cerca(mesh.vertices[v*3+1],std::fabs(y)>.2f?std::copysign(alturaBanda+std::fabs(y)-.23f,y):y),"Ventana comodin mueve bordes sin deformar flechas");
                Comprobar(Cerca(mesh.vertices[v*3],a.vertices[v*3]) && Cerca(mesh.vertices[v*3+2],a.vertices[v*3+2]),"Linea conserva ancho, profundidad y pivote");
            }
        }
        if(i==MODELO_RODILLOS_LED_CIAN || i==MODELO_RODILLOS_LED_ROSA)
        {
            int ai=i==MODELO_RODILLOS_LED_CIAN?1:2;
            auto& a=ObtenerModelosEscenariosRetro3D().animacionesRodillos[ai];
            int columna=(int)std::lround((p.x+24)/6);
            for(int j=0;j<2;j++) for(int v=0;v<m.meshes[j+1].vertexCount;v+=36)
            {
                Mesh& mesh=m.meshes[j+1]; float y=0;
                for(int k=v;k<v+36;k++) y+=mesh.vertices[k*3+1];
                float led=std::round((y/36-1.2f)/1.25f);
                float pulso=.5f+.5f*std::sin(tiempo*3-led*.6f+(float)columna);
                int alpha=(int)(a.colores[j][v*4+3]*(.35f+.65f*pulso));
                Comprobar(mesh.colors[v*4+3]==alpha,"Pulsos de cada LED siguen columna y tiempo");
                Comprobar(std::memcmp(mesh.colors+v*4,a.colores[j]+v*4,3)==0,"RGB de vertices conservado");
            }
        }
        break;
    }
    __real_DrawModelEx(m,p,eje,giro,escala,tinte);
}
extern "C" void __real_DrawCube(Vector3,float,float,float,Color);
extern "C" void __wrap_DrawCube(Vector3 p,float x,float y,float z,Color c)
{ primitivos++; __real_DrawCube(p,x,y,z,c); }
extern "C" void __real_DrawSphere(Vector3,float,Color);
extern "C" void __wrap_DrawSphere(Vector3 p,float r,Color c)
{ primitivos++; __real_DrawSphere(p,r,c); }
extern "C" void __real_DrawCylinderEx(Vector3,Vector3,float,float,int,Color);
extern "C" void __wrap_DrawCylinderEx(Vector3 a,Vector3 b,float x,float y,int n,Color c)
{ primitivos++; __real_DrawCylinderEx(a,b,x,y,n,c); }
extern "C" void __real_DrawTriangle3D(Vector3,Vector3,Vector3,Color);
extern "C" void __wrap_DrawTriangle3D(Vector3 a,Vector3 b,Vector3 c,Color color)
{ primitivos++; __real_DrawTriangle3D(a,b,c,color); }
extern "C" void __real_DrawCubeWires(Vector3,float,float,float,Color);
extern "C" void __wrap_DrawCubeWires(Vector3 p,float x,float y,float z,Color c)
{ primitivos++; __real_DrawCubeWires(p,x,y,z,c); }
extern "C" void __real_DrawSphereWires(Vector3,float,int,int,Color);
extern "C" void __wrap_DrawSphereWires(Vector3 p,float r,int x,int y,Color c)
{ primitivos++; __real_DrawSphereWires(p,r,x,y,c); }
extern "C" void __real_DrawCylinderWires(Vector3,float,float,float,int,Color);
extern "C" void __wrap_DrawCylinderWires(Vector3 p,float a,float b,float h,int n,Color c)
{ primitivos++; __real_DrawCylinderWires(p,a,b,h,n,c); }
static void GuardarMateriales()
{ for(int i=0;i<N;i++) for(int j=0;j<Recursos()[i].modelo.materialCount;j++) materiales[i][j]=Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color; }
static void Dibujar(ZonaPruebas& zona,const char* captura=nullptr)
{
    auto& m=zona.gestorMinijuegos.minijuegoRodillosNeon;
    unsigned char estado[sizeof(m)]; std::memcpy(estado,&m,sizeof(m));
    unsigned char jugadores[sizeof(zona.jugadores)]; std::memcpy(jugadores,zona.jugadores,sizeof(jugadores));
    tiempo=m.tiempoAnimacion; primitivos=0; std::memset(dibujos,0,sizeof(dibujos));
    const float velocidades[]={100,130,160,190,230};
    float ventana=velocidades[m.ronda<5?m.ronda:4]*DEG2RAD*.03f;
    float minimo=.14f*(2*PI/10); if(ventana<minimo) ventana=minimo;
    alturaBanda=1.7f*std::sin(ventana);
    BeginDrawing(); zona.Dibujar();
    if(captura) { rlDrawRenderBatchActive(); Image img=LoadImageFromScreen(); Comprobar(ExportImage(img,captura),"Captura del juego integrado"); UnloadImage(img); }
    EndDrawing();
    int simbolos[5]{};
    const Color colores[]={{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
    for(int i=0;i<4;i++) if(m.resultado.participantes[i].participo)
    {
        const auto& e=m.estadosJugadores[i];
        if(Recursos()[MODELO_RODILLOS_MARCO].cargado)
            Comprobar(ColorIsEqual(instancias[MODELO_RODILLOS_MARCO][e.maquina].color,colores[i]),"Marco colorea solo material dinamico por participante");
        if(Recursos()[MODELO_RODILLOS_BOTON].cargado)
        {
            float pulso=.5f+.5f*std::sin(tiempo*5);
            bool pendiente=e.rodilloActual<3 && m.etapa==ETAPA_RODILLOS_GIRO;
            Color esperado=Fade(colores[i],pendiente?.6f+.4f*pulso:.35f);
            Comprobar(ColorIsEqual(instancias[MODELO_RODILLOS_BOTON][e.maquina].color,esperado),"Cupula refleja disponibilidad y pulso sin colorear base");
        }
        for(int r=0;r<3;r++) for(int k=0;k<10;k++)
        {
            const auto& rod=e.rodillos[r]; float angulo=rod.angulo+k*(2*PI/10);
            while(angulo>PI) angulo-=2*PI;
            while(angulo<=-PI) angulo+=2*PI;
            if(std::fabs(angulo)>1.45f) continue;
            int tipo=rod.tira[k]; int pieza=MODELO_RODILLOS_TRIANGULO+tipo;
            if(!Recursos()[pieza].cargado) continue;
            const auto& d=instancias[pieza][simbolos[tipo]++];
            Comprobar(Cerca(d.p.x,e.posicionX+(r-1)*1.05f) && Cerca(d.p.y,3+1.73f*std::sin(angulo)) &&
                Cerca(d.p.z,1.73f*std::cos(angulo)) && Cerca(d.angulo,-angulo*RAD2DEG),"Simbolo usa tira y angulo real sin doble transformacion");
            bool glitch=m.estadoGlitch==2 && m.glitchJugador==i && m.glitchRodillo==r && rod.estado==RODILLO_GIRANDO;
            int material=Recursos()[pieza].modelo.meshMaterial[0];
            Color esperado=materiales[pieza][material];
            if(glitch) esperado={255,(unsigned char)(60+120*(.5f+.5f*std::sin(tiempo*40+k))),220,255};
            Comprobar(ColorIsEqual(d.color,esperado),"Glitch recolorea solo cara del rodillo afectado");
        }
    }
    for(int tipo=0;tipo<5;tipo++) Comprobar(dibujos[MODELO_RODILLOS_TRIANGULO+tipo]==simbolos[tipo],"No dibujar simbolos de la cara posterior");
    for(int k=0;k<3;k++) if(Recursos()[MODELO_RODILLOS_HOLOGRAMA_CUBO+k].cargado)
    {
        const auto& d=instancias[MODELO_RODILLOS_HOLOGRAMA_CUBO+k][0];
        Vector3 y=Vector3Subtract(Vector3Transform({0,1,0},d.matriz),d.p);
        float giro=(tiempo*40+k*60)*DEG2RAD;
        Comprobar(Cerca(d.p.x,-12+12*k) && Cerca(d.p.y,9.5f+.5f*std::sin(tiempo*1.5f+k)) && Cerca(d.p.z,-6),"Flotacion y posicion original de hologramas");
        Comprobar(Cerca(y.x,std::sin(20*DEG2RAD)*std::sin(giro)) && Cerca(y.y,std::cos(20*DEG2RAD)) && Cerca(y.z,std::sin(20*DEG2RAD)*std::cos(giro)),"Holograma hereda giro e inclinacion exactamente una vez");
    }
    Comprobar(std::memcmp(estado,&m,sizeof(m))==0,"El dibujo conserva estado, camara y reglas");
    Comprobar(std::memcmp(jugadores,zona.jugadores,sizeof(jugadores))==0,"El dibujo conserva jugadores y hitboxes");
    for(int i=0;i<N;i++) if(Recursos()[i].cargado) for(int j=0;j<Recursos()[i].modelo.materialCount;j++)
        Comprobar(ColorIsEqual(materiales[i][j],Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Material restaurado tras cada instancia");
    const int piezas[]={MODELO_RODILLOS_SUELO,MODELO_RODILLOS_LED_CIAN,MODELO_RODILLOS_LED_ROSA,MODELO_RODILLOS_LINEA};
    for(int i=0;i<4;i++) if(Recursos()[piezas[i]].cargado)
    {
        auto& r=Recursos()[piezas[i]]; auto& a=ObtenerModelosEscenariosRetro3D().animacionesRodillos[i];
        if(i==3) Comprobar(std::memcmp(a.vertices,r.modelo.meshes[1].vertices,r.modelo.meshes[1].vertexCount*3*sizeof(float))==0,"Banda restaurada en reposo");
        else for(int j=0;j<2;j++) Comprobar(std::memcmp(a.colores[j],r.modelo.meshes[j+1].colors,r.modelo.meshes[j+1].vertexCount*4)==0,"Colores y alpha restaurados");
    }
}
static void Avanzar(ZonaPruebas& zona,int frames)
{ for(int i=0;i<frames;i++) zona.Actualizar(1.0f/60); }
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Rodillos Neon");
    if(!IsWindowReady()) return 2;
    Participante p[MAX_PARTICIPANTES]{};
    ZonaPruebas zona; zona.Inicializar(p,4); zona.modoCatalogo=true;
    auto& m=zona.gestorMinijuegos.minijuegoRodillosNeon;
    for(int i=0;i<N;i++) Comprobar(cargas[i]==0,"El catalogo no carga los escenarios");
    p[0].activo=true; m.Reiniciar(zona.jugadores,p,4);
    Comprobar(m.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Un participante cancela sin cargar recursos");
    for(int i=0;i<N;i++) Comprobar(cargas[i]==0,"Partida invalida no carga el paquete");
    for(int cantidad=2;cantidad<=4;cantidad++)
    {
        for(int i=0;i<4;i++) { p[i].activo=i<cantidad; p[i].conectado=true; p[i].esBot=i>0; p[i].numeroJugador=i+1; p[i].control=CONTROL_TECLADO_WASD; }
        zona.cantidadParticipantes=cantidad; zona.CambiarMinijuego(MINIJUEGO_RODILLOS_NEON);
        for(int i=0;i<N;i++) Comprobar(Recursos()[i].cargado && cargas[i]==1,"21 cargas compartidas entre participantes y reinicios");
        GuardarMateriales();
        char nombre[128]; std::snprintf(nombre,sizeof(nombre),"build/rodillos-preparacion-%d.png",cantidad); Dibujar(zona,nombre);
        Comprobar(dibujos[MODELO_RODILLOS_GABINETE]==cantidad && dibujos[MODELO_RODILLOS_TAMBOR]==3*cantidad && dibujos[MODELO_RODILLOS_LED_CIAN]==5 && dibujos[MODELO_RODILLOS_LED_ROSA]==4,"Cantidad de instancias compartidas");
        for(int i=0;i<cantidad;i++)
        {
            float x=(i-(cantidad-1)*.5f)*5.8f;
            Comprobar(Cerca(instancias[MODELO_RODILLOS_GABINETE][i].p.x,x),"Origen de maquina sin traslacion doble");
            for(int r=0;r<3;r++)
            {
                auto& d=instancias[MODELO_RODILLOS_TAMBOR][i*3+r];
                Comprobar(Cerca(d.p.x,x+(r-1)*1.05f) && Cerca(d.p.y,3) && Cerca(d.angulo,-m.estadosJugadores[i].rodillos[r].angulo*RAD2DEG),"Pivote y giro reales del tambor");
            }
        }
        Avanzar(zona,250); Comprobar(m.fase==FASE_RODILLOS_JUGANDO && m.etapa==ETAPA_RODILLOS_GIRO,"Preparacion y aviso originales");
        m.estadoGlitch=0; m.tiempoGlitch=0; Avanzar(zona,1);
        Comprobar(m.estadoGlitch==1 && m.glitchJugador>=0,"Fallo real elige jugador y muestra aviso");
        Dibujar(zona,cantidad==4?"build/rodillos-aviso.png":nullptr);
        Avanzar(zona,49); Comprobar(m.estadoGlitch==2,"Aviso pasa al efecto tras 0.8 segundos");
        Avanzar(zona,61); Comprobar(m.estadoGlitch==3,"Efecto termina despues de un segundo");
        float antes=m.estadosJugadores[0].rodillos[0].angulo;
        m.estadoGlitch=2; m.glitchJugador=0; m.glitchRodillo=0; m.glitchInvierte=true; m.tiempoGlitch=1;
        Avanzar(zona,1); Comprobar(m.estadosJugadores[0].rodillos[0].angulo>antes,"Glitch invierte el giro real"); Dibujar(zona,cantidad==4?"build/rodillos-glitch.png":nullptr);
        m.glitchInvierte=false; antes=m.estadosJugadores[0].rodillos[0].angulo; Avanzar(zona,1);
        Comprobar(Cerca(m.estadosJugadores[0].rodillos[0].angulo-antes,-100*DEG2RAD/30),"Glitch acelera solo su rodillo");
        m.estadoGlitch=3; m.estadosJugadores[0].rodillos[0].angulo=0; tecla=KEY_E; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(m.estadosJugadores[0].rodilloActual==1 && m.estadosJugadores[0].rodillos[0].estado==RODILLO_FRENANDO && m.estadosJugadores[0].rodillos[0].comodin,"Entrada humana, frenado y comodin");
        Dibujar(zona,cantidad==4?"build/rodillos-frenado.png":nullptr); Avanzar(zona,12);
        Comprobar(m.estadosJugadores[0].rodillos[0].estado==RODILLO_DETENIDO,"Freno llega a la linea sin cambiar tiempos");
        for(int i=0;i<cantidad;i++) p[i].esBot=true;
        for(int i=0;i<6000 && m.fase!=FASE_RODILLOS_TERMINADO;i++) zona.Actualizar(1.0f/60);
        Comprobar(m.fase==FASE_RODILLOS_TERMINADO && m.ronda==5 && m.resultado.estado==RESULTADO_MINIJUEGO_FINALIZADO,"IA completa cinco rondas y final");
        Dibujar(zona,cantidad==4?"build/rodillos-final.png":nullptr);
        tecla=KEY_R; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(m.fase==FASE_RODILLOS_PREPARACION && m.estadosJugadores[0].puntos==0,"Reinicio R limpia puntuacion sin recargar");
        zona.modoTablero=true; tecla=KEY_R; Avanzar(zona,1); tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Ronda oficial protege reinicio y salida");
        zona.modoTablero=false; tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(zona.volverAlMenu,"Salida al menu disponible en pruebas"); zona.volverAlMenu=false;
    }
    // Fixtures de simbolos con la evaluacion real: par, triple, estrella, nada.
    for(int caso=0;caso<5;caso++)
    {
        zona.CambiarMinijuego(MINIJUEGO_RODILLOS_NEON); m.fase=FASE_RODILLOS_JUGANDO; m.etapa=ETAPA_RODILLOS_GIRO;
        for(int i=0;i<4;i++)
        {
            auto& e=m.estadosJugadores[i]; e.rodilloActual=3;
            for(int r=0;r<3;r++) { auto& rod=e.rodillos[r]; rod.estado=RODILLO_DETENIDO; rod.comodin=caso==4; rod.simbolo=caso==0?(r==2?1:0):caso==1?2:caso==2?4:r; rod.angulo=0; }
        }
        Avanzar(zona,1); const int puntos[]={3,10,20,0,20};
        Comprobar(m.etapa==ETAPA_RODILLOS_RESULTADO && m.estadosJugadores[0].puntosRonda==puntos[caso],"Puntuacion real de pares, triples y comodines");
        Dibujar(zona,caso==4?"build/rodillos-comodines.png":nullptr);
    }
    // Fallo de cada archivo sin mover assets: un diagnostico y fallback local.
    for(int pieza=0;pieza<N;pieza++)
    {
        for(auto& e:m.estadosJugadores) for(auto& rod:e.rodillos) for(int& s:rod.tira) s=(pieza>=MODELO_RODILLOS_TRIANGULO && pieza<=MODELO_RODILLOS_ESTRELLA)?pieza-MODELO_RODILLOS_TRIANGULO:0;
        Dibujar(zona); int base=primitivos, previo[N]; std::memcpy(previo,dibujos,sizeof(previo));
        DescargarAnimacionesRodillosNeonRetro3D(); DescargarSlotModeloEscenarioRetro3D(Recursos()[pieza]);
        ausente=RUTAS_MODELOS_RODILLOS_NEON_3D[pieza]; int avisoAntes=avisos;
        CargarPaqueteRodillosNeonRetro3D(); CargarPaqueteRodillosNeonRetro3D(); CargarPaqueteRodillosNeonRetro3D();
        Comprobar(avisos==avisoAntes+1,"Diagnostico una sola vez por pieza ausente");
        Dibujar(zona); Comprobar(dibujos[pieza]==0 && primitivos>base,"Fallback visible por pieza sin GLB duplicado");
        for(int i=0;i<N;i++) if(i!=pieza) Comprobar(dibujos[i]==previo[i],"Fallo no oculta otras piezas");
        ausente=nullptr; DescargarAnimacionesRodillosNeonRetro3D(); Recursos()[pieza]={}; CargarPaqueteRodillosNeonRetro3D(); GuardarMateriales();
    }
    for(int i=0;i<N;i++) Comprobar(usos[i]>0,"Todos los modelos se dibujan en el juego integrado");
    zona.Descargar(); zona.Descargar();
    for(int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && Recursos()[i].modelo.meshes==nullptr,"Descarga simetrica antes de CloseWindow");
    for(auto& a:ObtenerModelosEscenariosRetro3D().animacionesRodillos) Comprobar(!a.vertices && !a.colores[0] && !a.colores[1],"Copias CPU liberadas");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Rodillos Neon 2/3/4 participantes: %d errores\n",errores);
    return errores?1:0;
}
