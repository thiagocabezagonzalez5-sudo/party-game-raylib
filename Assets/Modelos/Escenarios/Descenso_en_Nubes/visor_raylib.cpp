#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* N[]={"planeador","planeador_frenado","anillo_blanco",
 "anillo_dorado","estrella","nube_blanca","nube_tormenta","flecha_viento",
 "banda_viento","mar_de_nubes","isla_principal","diana_aterrizaje",
 "isla_flotante","globo_azul","globo_rojo","molino_torre","aspas_molino",
 "ave","arcoiris"};
static Model M[19]{};
static Color COLORES[]={{235,82,80,255},{75,135,238,255},{88,204,120,255},{244,204,65,255}};
static void Pieza(int id,Vector3 p,float rot=0,float escala=1,Color tint=WHITE){
 DrawModelEx(M[id],p,{0,1,0},rot,{escala,escala,escala},tint);
}
static void Jugadores(float y,float t){
 for(int i=0;i<4;i++){
  float x=(i-1.5f)*2.15f;float z=i%2?1.6f:-1.4f;
  DrawCube({x,y,z},.6f,1.35f,.6f,COLORES[i]);
  DrawCube({x,y+.7f,z},.63f,.16f,.63f,{242,241,227,255});
  Pieza(i==1?1:0,{x,y,z},0,1,COLORES[i]);
  if(i==1)for(int j=0;j<3;j++)Pieza(4,{x+.65f*cosf(t*2+j*2.1f),y+1.0f,z+.65f*sinf(t*2+j*2.1f)});
 }
}
static void Molinos(){
 Pieza(15,{-5.2f,0,-3});
 Pieza(16,{-5.2f,4.15f,-2.12f});
 for(int i=0;i<2;i++){
  float x=i?-11.5f:11.5f,y=i?38.f:64.f,z=-6;
  Pieza(5,{x,y-.3f,z},0,1.6f);
  Pieza(15,{x,y,z},0,.65f);
  Pieza(16,{x,y+2.70f,z+.56f},0,.65f);
 }
}
static void Fondo(float t){
 for(int i=0;i<34;i++){
  float y=4.f+i*2.85f;
  float a=i*2.45f,r=9.f+(i*7%7);
  Pieza(5,{r*cosf(a),y,r*sinf(a)},0,.85f+(i%3)*.20f);
 }
 for(int i=0;i<7;i++){
  float y=12.f+i*13.f,x=(i%2?1.f:-1.f)*(10.5f+(i%3));
  Pieza(12,{x,y,-3.f+(i%4)*2.f});
 }
 for(int i=0;i<5;i++){
  float y=20.f+i*17.f+.4f*sinf(t+i);
  Pieza(i%2?14:13,{(i%2?-1.f:1.f)*9.5f,y,3.f-i});
 }
 for(int i=0;i<8;i++){
  float a=i*1.7f+t*.3f;
  Pieza(17,{8*cosf(a),6.f+i*12.f,8*sinf(a)},a*RAD2DEG);
 }
 Molinos();Pieza(18,{0,62,-16});
}
static void Descenso(float t){
 Fondo(t);
 Jugadores(62,t);
 for(int i=0;i<28;i++){
  float y=71.f-i*1.12f;
  float x=(i%2?-2.5f:2.4f)+1.1f*sinf(i*.81f);
  float z=(i%3-1)*1.6f;
  Pieza(i%9==0?3:2,{x,y,z});
 }
 for(int i=0;i<5;i++)Pieza(6,{(i%2?-1.f:1.f)*(3.2f+i*.3f),54.f+i*4.f,-1.8f+(i%3)*1.8f});
 Pieza(8,{0,68,0});
 for(int i=-2;i<=2;i++)Pieza(7,{i*2.1f,68,i%2?1.8f:-1.8f});
}
static void Aterrizaje(float t){
 Fondo(t);
 Pieza(9,{0,0,0});Pieza(10,{0,0,0});Pieza(11,{0,0,0});
 Jugadores(7,t);
 for(int i=0;i<14;i++){
  float x=(i%2?-2.8f:2.5f)+.8f*sinf(i*.9f);
  Pieza(i%7==0?3:2,{x,15.f-i*1.0f,(i%3-1)*1.4f});
 }
 Pieza(6,{3.8f,9.f,-1.2f});
 for(int i=0;i<4;i++)Pieza(17,{(i%2?-1.f:1.f)*8.0f,5.f+i*2,0});
}
static void Escena(int modo,float t){
 if(modo>=5&&modo<24){Pieza(modo-5,{0,0,0});return;}
 if(modo==3)Aterrizaje(t);else Descenso(t);
}
static Camera3D CamaraModelo(int id){
 BoundingBox b=GetModelBoundingBox(M[id]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 size=Vector3Subtract(b.max,b.min);
 float radius=fmaxf(size.x,fmaxf(size.y,size.z));
 Vector3 dir=Vector3Normalize({.9f,.85f,1.45f});
 if(id==0||id==1||id==2||id==3||id==11)dir=Vector3Normalize({.3f,1.1f,1.3f});
 if(id==18)dir=Vector3Normalize({.12f,.20f,1.7f});
 return {Vector3Add(centro,Vector3Scale(dir,radius*1.8f+1.8f)),centro,{0,1,0},37,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float t,bool hud=false){
 BeginDrawing();ClearBackground(modo==3?Color{140,205,245,255}:Color{83,151,203,255});
 BeginMode3D(cam);Escena(modo,t);EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,64,{32,76,112,235});
  DrawText("DESCENSO EN NUBES / Modelos GLB",18,8,23,RAYWHITE);
  DrawText("1 Descenso | 2 Camara del juego | 3 Aterrizaje | Flechas: girar",18,39,16,RAYWHITE);
 }
 EndDrawing();
}
static void Capturar(const char* nombre,Camera3D cam,int modo){
 Dibujar(cam,modo,1.2f);
 TakeScreenshot((std::string("Vistas/")+nombre+".png").c_str());
}
int main(int argc,char** argv){
 std::string carpeta=GetWorkingDirectory();
 bool capturar=argc>1&&std::string(argv[1])=="--capturar";
 bool verificar=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);
 InitWindow(capturar?1600:1280,capturar?1000:800,"Descenso en Nubes - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,240);
 int cargados=0;
 for(int i=0;i<19;i++){
  std::string ruta=std::string("GLB/")+N[i]+".glb";
  if(!FileExists(ruta.c_str())){std::fprintf(stderr,"Falta %s\n",ruta.c_str());break;}
  M[i]=LoadModel(ruta.c_str());cargados++;
  if(!M[i].meshCount)break;
  std::printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(cargados!=19){for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capturar){
  Capturar("Escena_general",{{19,80,25},{0,60,0},{0,1,0},55,CAMERA_PERSPECTIVE},1);
  Capturar("Camara_del_juego",{{0,75,7.5f},{0,59,.8f},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capturar("Aterrizaje",{{0,21,7.5f},{0,5,.8f},{0,1,0},50,CAMERA_PERSPECTIVE},3);
  Capturar("Arcoiris_y_nubes",{{25,73,28},{0,61,-3},{0,1,0},52,CAMERA_PERSPECTIVE},4);
  for(int i=0;i<19;i++)Capturar(N[i],CamaraModelo(i),i+5);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.7f;
  while(!WindowShouldClose()&&(!verificar||frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   Camera3D cam={{27*sinf(azimut),79,27*cosf(azimut)},{0,61,0},{0,1,0},53,CAMERA_PERSPECTIVE};
   if(modo==2)cam={{0,75,7.5f},{0,59,.8f},{0,1,0},50,CAMERA_PERSPECTIVE};
   if(modo==3)cam={{0,21,7.5f},{0,5,.8f},{0,1,0},50,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
