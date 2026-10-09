#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

// Visor autonomo. La logica real sigue en MinijuegoLaberintoInclinado.cpp.
static const char* N[]={"losa_marco","banda_jugador","muro_bloque","glifo_muro",
 "agujero","marca_salida","checkpoint","altar","esfera_jade","flecha_trampa",
 "boquilla_trampa","templo_fondo","columna_templo","antorcha","llama"};
static Model M[15]{};
static const char* MAPA[11]={
 "###########","#S        #","######### #","#   o     #","#     o   #",
 "# #########","#A        #","######### #","#M   o   B#","#      o  #","###########"};
static const Color P[]={ {224,88,78,255},{84,147,238,255},{101,203,131,255},{243,207,83,255} };

static void Pieza(int i,Vector3 p,Color tint=WHITE,float escala=1){
 DrawModel(M[i],p,escala,tint);
}
static void Tablero(Vector3 origen,int jugador,float tiempo){
 rlPushMatrix();
 rlTranslatef(origen.x,origen.y,origen.z);
 rlRotatef(4*std::sin(tiempo+jugador),1,0,0);
 rlRotatef(3*std::cos(tiempo*.8f+jugador),0,0,1);
 Pieza(0,{0,0,0});
 Pieza(1,{0,-.30f,6.02f},P[jugador]);
 for(int r=0;r<11;r++)for(int c=0;c<11;c++){
  Vector3 p={(float)c-5,0,(float)r-5};
  char celda=MAPA[r][c];
  if(celda=='#'){
   Pieza(2,p);
   if((c*7+r*3)%5==0)Pieza(3,{p.x,.61f,p.z});
  }else if(celda=='o')Pieza(4,p);
  else if(celda=='S')Pieza(5,{p.x,.05f,p.z});
  else if(celda=='A'||celda=='B')Pieza(6,{p.x,.05f,p.z},celda=='A'?Color{234,183,74,255}:Color{71,205,143,255});
  else if(celda=='M')Pieza(7,p);
 }
 for(int c=3;c<=7;c++)for(int r:{1,6}){
  Pieza(9,{(float)c-5,.052f,(float)r-5},Color{220,96,65,255});
 }
 Pieza(10,{-2.5f,.02f,-4.5f});Pieza(10,{-2.5f,.02f,.5f});
 Pieza(8,{-3.5f,.31f,-4.5f});
 rlPopMatrix();
}
static void Entorno(float t){
 Pieza(11,{0,0,0});
 for(int x:{-23,23})for(int z:{-14,12})Pieza(12,{(float)x,0,(float)z});
 for(int x:{-15,-9,-3,3,9,15}){
  Pieza(13,{(float)x,2.4f,-21});
  Pieza(14,{(float)x,4.3f+.05f*std::sin(t*11+x),-20.9f});
 }
}
static void Escena(int modo,float t){
 if(modo==1){
  Entorno(t);
  Tablero({-7,0,-7},0,t);Tablero({7,0,-7},1,t);
  Tablero({-7,0,7},2,t);Tablero({7,0,7},3,t);
 }else if(modo==2){Entorno(t);Tablero({-7,0,0},0,t);Tablero({7,0,0},1,t);}
 else if(modo==3){DrawPlane({0,-1,0},{30,30},{38,34,31,255});Tablero({0,0,0},0,t);}
 else if(modo>=4&&modo<19){Pieza(modo-4,{0,0,0});}
}
static Camera3D CamaraPieza(int id){
 BoundingBox b=GetModelBoundingBox(M[id]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 s=Vector3Subtract(b.max,b.min);
 float d=fmaxf(s.x,fmaxf(s.y,s.z));
 Vector3 v=Vector3Normalize({.88f,.81f,1.45f});
 if(id==11)v=Vector3Normalize({.85f,.52f,1.35f});
 if(id==12)v=Vector3Normalize({.9f,.65f,1.8f});
 return {Vector3Add(centro,Vector3Scale(v,d*1.8f+1.4f)),centro,{0,1,0},37,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float tiempo,bool hud=false){
 BeginDrawing();ClearBackground({22,26,27,255});BeginMode3D(cam);
 Escena(modo,tiempo);EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,61,{23,30,34,235});
  DrawText("LABERINTO JADE / Modelos GLB",18,7,23,RAYWHITE);
  DrawText("1 Cuatro tableros | 2 Dos tableros | 3 Un tablero | Flechas: girar",18,36,16,LIGHTGRAY);
 }
 EndDrawing();
}
static void Capturar(const char* nombre,Camera3D cam,int modo){
 Dibujar(cam,modo,1.2f);
 std::string ruta=std::string("Vistas/")+nombre+".png";
 TakeScreenshot(ruta.c_str());
}
int main(int argc,char** argv){
 std::string carpeta=GetWorkingDirectory();
 bool capturar=argc>1&&std::string(argv[1])=="--capturar";
 bool verificar=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);
 InitWindow(capturar?1600:1280,capturar?1000:800,"Laberinto Jade - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,300);
 int cargados=0;
 for(int i=0;i<15;i++){
  std::string ruta=std::string("GLB/")+N[i]+".glb";
  if(!FileExists(ruta.c_str())){std::fprintf(stderr,"Falta %s\n",ruta.c_str());break;}
  M[i]=LoadModel(ruta.c_str());cargados++;
  if(!M[i].meshCount)break;
  std::printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(cargados!=15){for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capturar){
  Capturar("Escena_general",{{31,37,42},{0,1,-3},{0,1,0},51,CAMERA_PERSPECTIVE},1);
  Capturar("Dos_tableros",{{0,21,9},{0,0,.4f},{0,1,0},45,CAMERA_PERSPECTIVE},2);
  SetWindowSize(1280,720);
  for(int i=0;i<3;i++){BeginDrawing();ClearBackground(BLACK);EndDrawing();}
  Capturar("Camara_del_juego",{{0,32,13.5f},{0,0,.8f},{0,1,0},45,CAMERA_PERSPECTIVE},1);
  SetWindowSize(1600,1000);
  for(int i=0;i<3;i++){BeginDrawing();ClearBackground(BLACK);EndDrawing();}
  Capturar("Tablero_detalle",{{13,14,14},{0,0,0},{0,1,0},46,CAMERA_PERSPECTIVE},3);
  for(int i=0;i<15;i++)Capturar(N[i],CamaraPieza(i),i+4);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.5f;
  while(!WindowShouldClose()&&(!verificar||frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   float radio=modo==3?26:48;
   Camera3D cam={{radio*std::sin(azimut),modo==3?17.f:34.f,radio*std::cos(azimut)},
    {0,0,0},{0,1,0},45,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);
 CloseWindow();return 0;
}
