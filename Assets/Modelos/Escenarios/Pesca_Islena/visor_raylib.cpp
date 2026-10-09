#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"oceano","isla_laguna","superficie_laguna","muelle_bambu","cana_pescar","corcho","palmera","barca","gaviota","volcan","humo_volcan","coral_1","coral_2","pez_pequeno","pez_mediano","pez_dorado","bota_vieja","cursor_lanzamiento"};
static Model M[18]{};static Color PLAYER[]={RED,BLUE,GREEN,YELLOW};
static void Part(int i,Vector3 p={0,0,0},float yaw=0,Vector3 size={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},yaw,size,tint);}
static Vector3 Dock(int i){return i==0?Vector3{0,0,10.5f}:i==1?Vector3{0,0,-10.5f}:i==2?Vector3{-10.5f,0,0}:Vector3{10.5f,0,0};}
static float Yaw(int i){return i==0?0:i==1?180:i==2?-90:90;}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(0);Part(1);
 for(int i=0;i<16;i++){
  float a=(i*11%16)*6.283185f/16,r=2.1f+(i*13%19)*.29f;
  Part(11+i%2,{r*cosf(a),-1.8f,r*sinf(a)},i*49.f);
 }
 for(int i=0;i<8;i++){
  float a=(22.5f+45*i)*DEG2RAD;
  Part(6,{13.2f*cosf(a),0,13.2f*sinf(a)},i*36.f);
 }
 Part(7,{-12,0,-11});Part(9,{0,0,-21});Part(10,{0,7,-21});
 for(int i=0;i<4;i++)Part(8,{11*cosf(i*1.6f),5.5f+i*.55f,11*sinf(i*1.6f)},i*70.f);
 for(int i=0;i<4;i++){
  Vector3 p=Dock(i);Part(3,p,Yaw(i));
  if(mode!=4){Part(4,{p.x,1.45f,p.z},Yaw(i));
   DrawCube({p.x,1.04f,p.z},.6f,1.2f,.6f,PLAYER[i]);
   float scale=i==0?-1:i==1?1:0,other=i==2?1:i==3?-1:0;
   Vector3 tip={p.x+other*1.4f,2.7f,p.z+scale*1.4f};
   Vector3 b={p.x*.4f,.12f,p.z*.4f};
   DrawLine3D(tip,b,WHITE);Part(5,b,0,{1,1,1},PLAYER[i]);
   Part(17,{p.x*.4f,.09f,p.z*.4f},0,{1,1,1},PLAYER[i]);
  }
 }
 if(mode!=4){
  for(int i=0;i<14;i++){
   int type=i<6?13:i<10?14:i<12?15:16;
   float a=i*2.399963f,r=1.15f+(i*7%17)*.31f;
   Part(type,{r*cosf(a),-.9f,r*sinf(a)},i*47.f);
  }
 }
 // Superficie translucida sobre el arrecife; raylib ordena este modelo despues.
 Part(2);
}
static Camera3D CamModel(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f),v=Vector3Subtract(b.max,b.min);
 float r=fmaxf(v.x,fmaxf(v.y,v.z));Vector3 dir=Vector3Normalize({.85f,.9f,1.25f});
 if(i==0||i==1||i==2)dir=Vector3Normalize({.4f,1.7f,1.4f});
 if(i>=13&&i<=16)dir=Vector3Normalize({.65f,.65f,1.5f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.6f+1.0f)),c,{0,1,0},37,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D c,int mode,bool hud){BeginDrawing();ClearBackground({139,208,220,255});BeginMode3D(c);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,64,{17,71,91,230});DrawText("PESCA ISLENA / Modelos GLB",20,8,23,RAYWHITE);DrawText("1 Escena | 2 Muelle | 3 Arrecife | 4 Paisaje | Flechas: girar",20,39,16,RAYWHITE);}EndDrawing();}
static void Capture(const char* n,Camera3D c,int mode){Render(c,mode,false);TakeScreenshot((std::string("Vistas/")+n+".png").c_str());}
int main(int argc,char** argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Pesca Islena - GLB");
 if(!IsWindowReady())return 1;ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,230);
 int loaded=0;for(int i=0;i<18;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());loaded++;if(!M[i].meshCount)break;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=18){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Escena_general",{{20,24,23},{0,0,0},{0,1,0},58,CAMERA_PERSPECTIVE},1);
  Capture("Camara_del_juego",{{0,24,6.7f},{0,0,1.7f},{0,1,0},64,CAMERA_PERSPECTIVE},1);
  Capture("Muelle_y_cana",{{7,11,19},{0,.1,9},{0,1,0},49,CAMERA_PERSPECTIVE},2);
  Capture("Arrecife",{{8,11,11},{0,-.5,0},{0,1,0},54,CAMERA_PERSPECTIVE},3);
  Capture("Paisaje",{{22,17,23},{0,2,-4},{0,1,0},57,CAMERA_PERSPECTIVE},4);
  for(int i=0;i<18;i++)Capture(N[i],CamModel(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;float az=.65f;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  if(IsKeyDown(KEY_LEFT))az-=GetFrameTime();if(IsKeyDown(KEY_RIGHT))az+=GetFrameTime();
  Camera3D cam={{22*sinf(az),20,22*cosf(az)},{0,0,0},{0,1,0},57,CAMERA_PERSPECTIVE};
  if(mode==1)cam={{0,24,6.7f},{0,0,1.7f},{0,1,0},64,CAMERA_PERSPECTIVE};
  Render(cam,mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
