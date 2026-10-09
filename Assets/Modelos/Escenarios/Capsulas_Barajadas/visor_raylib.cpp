#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* N[]={"sala_laboratorio","monitor","mostrador","tubo_ensayo",
 "baliza","mesa_acero","pasarela","brazo_base","brazo_segmento",
 "brazo_articulacion","brazo_pinza","capsula_cuerpo","capsula_banda",
 "capsula_tapa","nucleo","marcador"};
static Model M[16]{};
static const Color COLORES[]={{229,83,81,255},{91,161,233,255},
 {89,211,135,255},{237,201,82,255},{198,117,218,255}};

static void Pieza(int i,Vector3 p,Color color=WHITE,float escala=1){
 DrawModel(M[i],p,escala,color);
}
static void Entre(Vector3 a,Vector3 b){
 Vector3 d=Vector3Subtract(b,a);
 float largo=Vector3Length(d);
 if(largo<.001f)return;
 Vector3 u=Vector3Scale(d,1/largo);
 Vector3 eje=Vector3CrossProduct({0,1,0},u);
 float dot=fmaxf(-1.f,fminf(1.f,Vector3DotProduct({0,1,0},u)));
 float grados=acosf(dot)*RAD2DEG;
 if(Vector3Length(eje)<.001f)eje={1,0,0};
 DrawModelEx(M[8],a,eje,grados,{1,largo,1},WHITE);
}
static void Ambiente(){
 Pieza(0,{0,0,0});Pieza(2,{0,0,-6.4f});
 for(int k=0;k<4;k++)Pieza(1,{-7.5f+5.f*k,4.6f,-8.1f});
 for(int k=0;k<8;k++){
  Color t=COLORES[k%5];
  Pieza(3,{-8.4f+2.4f*k,1,-6.4f},t);
 }
 for(int lado:{-1,1})Pieza(4,{10.5f*lado,5.6f,-7.8f},{255,174,53,255});
}
static void Escena(int modo,float t){
 if(modo>=5&&modo<21){Pieza(modo-5,{0,0,0});return;}
 Ambiente();Pieza(5,{0,0,0});Pieza(6,{0,0,4.8f});
 Vector3 hombro={0,4.2f,-3.4f};
 Vector3 mano={1.3f+.5f*sinf(t),4.0f,0};
 Vector3 codo={(hombro.x+mano.x)*.5f,(hombro.y+mano.y)*.5f+1.3f,(hombro.z+mano.z)*.5f-.3f};
 Pieza(7,{0,0,-3.4f});Entre(hombro,codo);Pieza(9,codo);Entre(codo,mano);Pieza(10,mano);
 int cantidad=modo==3?3:5;
 for(int i=0;i<cantidad;i++){
  float sep=cantidad==3?2.8f:2.1f;
  float x=(i-(cantidad-1)*.5f)*sep;
  float z=modo==4&&i==0?sin(t)*.8f:0;
  Vector3 p={x,1.01f,z};
  Pieza(11,p);Pieza(12,p,COLORES[i]);
  Pieza(13,{x,1.01f+1.5f+(i==cantidad/2?1.05f:0),z},COLORES[i]);
  Pieza(15,{x,.31f,4.0f},COLORES[i],.70f);
 }
 float nX=(cantidad%2==0)?-1.25f:0;
 Pieza(14,{nX,1.01f+.54f,0},{190,255,222,255},1+.08f*sinf(t*5));
}
static Camera3D CamaraModelo(int id){
 BoundingBox b=GetModelBoundingBox(M[id]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 s=Vector3Subtract(b.max,b.min);
 float radio=fmaxf(s.x,fmaxf(s.y,s.z));
 Vector3 dir=Vector3Normalize({1.0f,.85f,1.55f});
 if(id==0)dir=Vector3Normalize({.35f,.52f,1.7f});
 return {Vector3Add(centro,Vector3Scale(dir,radio*1.9f+1.4f)),centro,{0,1,0},37,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float t,bool hud=false){
 BeginDrawing();ClearBackground({17,28,39,255});BeginMode3D(cam);
 Escena(modo,t);EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,60,{20,35,45,237});
  DrawText("CAPSULAS BARAJADAS / Modelos GLB",16,8,22,RAYWHITE);
  DrawText("1 Cinco capsulas | 2 Camara del juego | 3 Tres capsulas | 4 Movimiento | Flechas: girar",16,35,15,LIGHTGRAY);
 }
 EndDrawing();
}
static void Capturar(const char* nombre,Camera3D cam,int modo){
 Dibujar(cam,modo,1.4f);
 TakeScreenshot((std::string("Vistas/")+nombre+".png").c_str());
}
int main(int argc,char** argv){
 std::string carpeta=GetWorkingDirectory();
 bool capturar=argc>1&&std::string(argv[1])=="--capturar";
 bool verificar=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);
 InitWindow(capturar?1600:1280,capturar?1000:800,"Capsulas Barajadas - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,240);
 int cargados=0;
 for(int i=0;i<16;i++){
  std::string ruta=std::string("GLB/")+N[i]+".glb";
  if(!FileExists(ruta.c_str())){std::fprintf(stderr,"Falta %s\n",ruta.c_str());break;}
  M[i]=LoadModel(ruta.c_str());cargados++;
  if(!M[i].meshCount)break;
  std::printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(cargados!=16){for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capturar){
  Capturar("Escena_general",{{17,11,19},{0,2,-2},{0,1,0},52,CAMERA_PERSPECTIVE},1);
  Capturar("Camara_del_juego",{{0,6.6f,11.2f},{0,1,2},{0,1,0},48,CAMERA_PERSPECTIVE},2);
  Capturar("Tres_capsulas",{{0,6.6f,11.2f},{0,1,2},{0,1,0},48,CAMERA_PERSPECTIVE},3);
  Capturar("Mesa_y_brazo",{{8,7,9},{0,2,-1},{0,1,0},45,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<16;i++)Capturar(N[i],CamaraModelo(i),i+5);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.3f;
  while(!WindowShouldClose()&&(!verificar||frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyPressed(KEY_FOUR))modo=4;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   Camera3D cam={{16*sinf(azimut),7,16*cosf(azimut)},{0,1,0},{0,1,0},48,CAMERA_PERSPECTIVE};
   if(modo==2)cam={{0,6.6f,11.2f},{0,1,2},{0,1,0},48,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
