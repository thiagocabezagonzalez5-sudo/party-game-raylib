#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

// Visor autonomo; respeta la camara y medidas de MinijuegoVetaCristal.cpp.
static const char* N[]={"suelo_mina","paredes_tunel","veta_pared","portico_madera",
 "poste_lateral","lampara_colgante","riel_central","vagoneta","geoda_pequena",
 "geoda_grande","geoda_agotada","gema_azul","gema_dorada","gema_violeta",
 "marca_geoda"};
static Model M[15]{};
static void Pieza(int i,Vector3 p,float escala=1,Color tint=WHITE){
 DrawModel(M[i],p,escala,tint);
}
static void Mina(float t,bool detalles=true){
 Pieza(0,{0,0,0});Pieza(1,{0,0,0});Pieza(3,{0,0,0});Pieza(6,{0,0,0});
 for(int x:{-1,1})for(int z:{-5,0,5}){
  Pieza(4,{x*11.7f,0,(float)z});
  Pieza(5,{x*11.f,3.7f,(float)z});
 }
 for(int i=0;i<16;i++){
  float x=-11.1f+(i*37%23);
  float y=.75f+(i*7%12)*.31f;
  Pieza(2,{x,y,-7.94f});
 }
 for(int x:{-1,1})for(int z:{-5,3,6}){
  // Vetas adicionales incrustadas en las paredes laterales.
  rlPushMatrix();rlTranslatef(x*11.97f,2.5f,(float)z);
  rlRotatef(x>0?-90:90,0,1,0);Pieza(2,{0,0,0});rlPopMatrix();
 }
 if(!detalles)return;
 static const float X[]={3.8f,3.8f,7.6f,7.6f,6.f};
 static const float Z[]={-3.6f,3.6f,-3.6f,3.6f,0.f};
 for(int lado:{-1,1})for(int k=0;k<5;k++){
  float x=lado*X[k],z=Z[k];
  int modelo=k==4?9:8;
  if(lado==1&&k==1)modelo=10;
  Pieza(modelo,{x,k==4?3.10f:2.85f,z},modelo==10&&k==4?1.385f:1);
  Pieza(14,{x,.045f,z},k==4?1.48f:1,k==4?Color{247,202,94,255}:Color{96,213,244,255});
 }
 Pieza(7,{0,0,2.8f+1.5f*std::sin(t)});
 for(int i=0;i<12;i++){
  int id=11+(i%7==0?2:i%4==0?1:0);
  float x=(i%2?-1:1)*(2.2f+(i*3%8));
  float z=-5.4f+(i*5%12);
  Pieza(id,{x,.32f+(i%3)*.06f,z});
 }
}
static Camera3D CamaraPieza(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 s=Vector3Subtract(b.max,b.min);
 float d=fmaxf(s.x,fmaxf(s.y,s.z));
 Vector3 dir=Vector3Normalize({1.12f,.84f,1.5f});
 if(i==1)dir=Vector3Normalize({.62f,.48f,1.6f});
 return {Vector3Add(centro,Vector3Scale(dir,d*1.9f+1.5f)),centro,{0,1,0},38,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float t,bool hud=false){
 BeginDrawing();ClearBackground({17,15,22,255});BeginMode3D(cam);
 if(modo==1)Mina(t);
 else if(modo==2){Mina(t);}
 else if(modo==3){Mina(t,false);}
 else if(modo>=4&&modo<19)Pieza(modo-4,{0,0,0});
 EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,61,{24,22,31,235});
  DrawText("VETA DE CRISTAL / Modelos GLB",18,8,23,RAYWHITE);
  DrawText("1 Mina completa | 2 Camara del juego | 3 Tunel | Flechas: girar",18,37,16,LIGHTGRAY);
 }
 EndDrawing();
}
static void Capturar(const char* nombre,Camera3D cam,int modo){
 Dibujar(cam,modo,1.3f);
 TakeScreenshot((std::string("Vistas/")+nombre+".png").c_str());
}
int main(int argc,char** argv){
 std::string carpeta=GetWorkingDirectory();
 bool capturar=argc>1&&std::string(argv[1])=="--capturar";
 bool verificar=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);
 InitWindow(capturar?1600:1280,capturar?1000:800,"Veta de Cristal - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,220);
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
  Capturar("Escena_general",{{27,21,25},{0,1,0},{0,1,0},55,CAMERA_PERSPECTIVE},1);
  Capturar("Camara_del_juego",{{0,18,14.5f},{0,.8f,.4f},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capturar("Tunel",{{18,11,18},{0,1,-1},{0,1,0},53,CAMERA_PERSPECTIVE},3);
  Capturar("Geodas_y_vagoneta",{{10,10,10},{0,1,0},{0,1,0},45,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<15;i++)Capturar(N[i],CamaraPieza(i),i+4);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.42f;
  while(!WindowShouldClose()&&(!verificar||frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   Camera3D cam={{29*std::sin(azimut),18,29*std::cos(azimut)},{0,.8f,.4f},{0,1,0},50,CAMERA_PERSPECTIVE};
   if(modo==2)cam={{0,18,14.5f},{0,.8f,.4f},{0,1,0},50,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
