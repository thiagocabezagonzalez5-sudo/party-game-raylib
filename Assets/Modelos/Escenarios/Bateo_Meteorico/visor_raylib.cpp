#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* N[]={"cumbre","observatorio_cupula","telescopio",
 "observatorio_secundario","planeta_anillado","farol","carril_plataforma",
 "campo_puntaje","canon_base","canon_tubo","meteorito_normal",
 "meteorito_dorado","meteorito_rojo","bate","aro_punto_dulce"};
static Model M[15]{};
static Color COLOR[]={{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
static void Pieza(int i,Vector3 p,Color color=WHITE,float escala=1){
 DrawModel(M[i],p,escala,color);
}
static void Orientar(Model& modelo,Vector3 a,Vector3 b,float longitudVisual){
 Vector3 d=Vector3Subtract(b,a),u=Vector3Normalize(d);
 Vector3 eje=Vector3CrossProduct({0,1,0},u);
 float dot=fmaxf(-1.f,fminf(1.f,Vector3DotProduct({0,1,0},u)));
 if(Vector3Length(eje)<.001f)eje={1,0,0};
 DrawModelEx(modelo,a,eje,acosf(dot)*RAD2DEG,{1,longitudVisual,1},WHITE);
}
static void Fondo(float t){
 Pieza(0,{0,0,0});
 Pieza(1,{-36,0,-64});Pieza(2,{-36,0,-64});
 Pieza(3,{44,0,-76});Pieza(4,{34,34,-112});
 for(int i=-2;i<=2;i++)Pieza(5,{i*6.f-3,0,3.2f});
 for(int i=0;i<100;i++){
  float x=-108+(i*73%217),y=13+(i*29%59);
  DrawSphere({x,y,-124},.15f+(i%7)*.05f,{199,219,248,255});
 }
 DrawSphere({-62,52,-118},5,{225,228,240,255});
}
static void Carril(float x,int jugador,float t){
 Pieza(6,{x,0,0},COLOR[jugador]);Pieza(7,{x,0,0},COLOR[jugador]);
 Pieza(8,{x,0,-12.8f});
 Vector3 boca={x,5,-12},destino={x,1.4f,-.7f};
 Vector3 dir=Vector3Normalize(Vector3Subtract(destino,boca));
 Vector3 a={x,4.7f,-12.4f};
 Orientar(M[9],a,Vector3Add(a,dir),1);
 Pieza(14,{x,1.4f,-.7f});
 int tipo=jugador==1?11:jugador==3?12:10;
 Pieza(tipo,{x,.95f+(jugador%2)*.8f,-2.9f-jugador*1.0f});
 // Marcador temporal del jugador para contextualizar escala y posición.
 DrawCube({x,.8f,.7f},.75f,1.6f,.75f,COLOR[jugador]);
 DrawCube({x,1.55f,.7f},.65f,.24f,.65f,{230,230,231,255});
 DrawModelEx(M[13],{x+.4f,1.05f,.7f},{0,1,0},-70,{1,1,1},COLOR[jugador]);
}
static void Escena(int modo,float t){
 if(modo>=5&&modo<20){Pieza(modo-5,{0,0,0});return;}
 Fondo(t);
 int cantidad=modo==3?2:4;
 for(int i=0;i<cantidad;i++){
  float x=(i-(cantidad-1)*.5f)*6;
  Carril(x,i,t);
 }
}
static Camera3D CamaraModelo(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 s=Vector3Subtract(b.max,b.min);
 float r=fmaxf(s.x,fmaxf(s.y,s.z));
 Vector3 d=Vector3Normalize({1.1f,.8f,1.7f});
 if(i==2||i==4)d=Vector3Normalize({1.f,.55f,1.2f});
 return {Vector3Add(centro,Vector3Scale(d,r*1.7f+1.6f)),centro,{0,1,0},38,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float t,bool hud=false){
 BeginDrawing();ClearBackground({9,12,30,255});BeginMode3D(cam);
 Escena(modo,t);EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,62,{18,23,46,240});
  DrawText("BATEO METEORICO / Modelos GLB",18,8,22,RAYWHITE);
  DrawText("1 Cuatro carriles | 2 Camara del juego | 3 Dos carriles | Flechas: girar",18,37,16,LIGHTGRAY);
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
 InitWindow(capturar?1600:1280,capturar?1000:800,"Bateo Meteorico - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,450);
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
  Capturar("Escena_general",{{28,23,31},{0,4,-31},{0,1,0},57,CAMERA_PERSPECTIVE},1);
  Capturar("Camara_del_juego",{{0,8,13},{0,1.6f,-12},{0,1,0},58,CAMERA_PERSPECTIVE},2);
  Capturar("Dos_carriles",{{0,7,12},{0,.2f,-12},{0,1,0},50,CAMERA_PERSPECTIVE},3);
  Capturar("Observatorio_y_planeta",{{-6,26,-27},{0,17,-77},{0,1,0},54,CAMERA_PERSPECTIVE},4);
  for(int i=0;i<15;i++)Capturar(N[i],CamaraModelo(i),i+5);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.45f;
  while(!WindowShouldClose()&&(!verificar||frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   Camera3D cam={{26*sinf(azimut),11,26*cosf(azimut)},{0,1,-12},{0,1,0},55,CAMERA_PERSPECTIVE};
   if(modo==2)cam={{0,8,13},{0,1.6f,-12},{0,1,0},58,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
