#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"suelo_desierto","pared_arenisca","cisterna_oasis","agua_cisterna","colector","tubo_piedra","tubo_agua","codo_tuberia","columna_ruina","repisa_cantaros","plataforma_jugadores","compuerta_base","compuerta_hoja","cantaro_barro","cantaro_oro","cantaro_miraje","palmera_oasis","cactus","duna","sol"};
static Model M[20]{};static std::string ROOT;
static void Part(int i,Vector3 p={0,0,0},Vector3 s={1,1,1}){DrawModelEx(M[i],p,{0,1,0},0,s,WHITE);}
static float X(int n,int i){return (i-.5f*(n-1))*(n<=4?3.4f:(n==5?3.f:2.7f));}
static void Segment(int i,Vector3 a,Vector3 b){
 Vector3 d=Vector3Subtract(b,a);float len=Vector3Length(d);if(len<.001f)return;
 d=Vector3Scale(d,1.f/len);Vector3 axis=Vector3CrossProduct({0,1,0},d);
 float dot=fmaxf(-1.f,fminf(1.f,d.y)),angle=acosf(dot)*RAD2DEG;
 if(Vector3Length(axis)<.0001f)axis={1,0,0};
 DrawModelEx(M[i],a,axis,angle,{1,len,1},WHITE);
}
static Camera3D Cam(int mode){
 if(mode==3)return {{13,15,22},{0,8,-3},{0,1,0},48,CAMERA_PERSPECTIVE};
 return {{0,10.5f,27},{0,7.4f,-2},{0,1,0},50,CAMERA_PERSPECTIVE};
}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 int n=mode==2?6:4,stages=mode==2?7:3;
 ClearBackground({250,206,140,255});
 Part(19,{-26,22,-60});Part(0);Part(18,{-34,0,-14});Part(18,{36,-1,-18});
 Part(1,{0,0,-3.2f});Part(2,{0,15.2f,-3.2f});Part(3,{0,16.74f,-3.2f});
 Part(4,{0,13.7f,-2.3f},{n==4?.65f:1.f,1,1});
 Part(9,{0,0,-1.9f});Part(10,{0,0,3});
 for(int i=0;i<3;i++)Part(16,{i==0?-6.8f:(i==1?6.4f:-10.f),15.2f,-3.2f},{i==2?.8f:1,1,i==2?.8f:1});
 for(int s:{-1,1}){
  Part(8,{s*18.f,0,-1});Part(8,{s*21.f,0,1},{1,.55f,1});Part(17,{s*11.5f,0,4.5f});
 }
 for(int e=0;e<n;e++){
  float x=X(n,e);Part(11,{x,12.8f,-2.3f});Part(12,{x,13.0f+(mode==3?.9f:0),-1.8f});
  Part(e==2?14:(e==1?15:13),{x,1.8f,-1.9f});
  if(e<4)DrawCube({x,.7f,2.8f},.65f,1.4f,.65f,e==0?RED:e==1?BLUE:e==2?GREEN:GOLD);
  int lane=e;
  Vector3 prior={x,12.8f,-2.3f};
  for(int stage=0;stage<stages;stage++){
   int pair=stage%2;int next=lane;
   if(lane>=pair && lane+1<n && (lane-pair)%2==0)next=lane+1;
   else if(lane>pair && (lane-pair)%2==1)next=lane-1;
   float y=12.2f-(stage+1)*8.6f/stages;
   Vector3 end={X(n,next),y,-2.3f};
   Vector3 start=prior;
   for(int j=1;j<=4;j++){
    float t=j/4.f;
    Vector3 v={start.x+(end.x-start.x)*t,start.y+(end.y-start.y)*t,-2.3f+(next>lane?.35f:(next<lane?-.3f:0))*sinf(t*PI)};
    Segment(5,prior,v);if(mode==3&&stage<=stages/2)Segment(6,prior,v);
    prior=v;
   }
   lane=next;
  }
  Segment(5,prior,{X(n,lane),2.9f,-2.3f});
 }
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 v=Vector3Subtract(b.max,b.min);float r=fmaxf(v.x,fmaxf(v.y,v.z));
 Vector3 dir=Vector3Normalize({.75f,1.f,1.5f});if(i==1||i==19)dir=Vector3Normalize({.1f,.3f,1.5f});
 if(i==0||i==10)dir=Vector3Normalize({.5f,2.f,1.3f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.45f+1)),c,{0,1,0},42,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D c,int mode,bool hud){BeginDrawing();ClearBackground({250,206,140,255});BeginMode3D(c);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,68,{88,59,40,230});DrawText("TUBERIAS DESIERTO / Modelos GLB",20,8,24,RAYWHITE);DrawText("1 Primera ronda | 2 Seis entradas | 3 Agua en movimiento",20,42,16,RAYWHITE);}EndDrawing();}
static void Shot(const char* name,Camera3D c,int mode){Render(c,mode,false);TakeScreenshot((ROOT+"/Vistas/"+name+".png").c_str());}
int main(int argc,char**argv){
 ROOT=argc>2?argv[2]:GetWorkingDirectory();bool cap=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(cap?1600:1280,cap?1000:800,"Tuberias Desierto - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(ROOT.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<20;i++){
  std::string p=std::string("GLB/")+N[i]+".glb";if(!FileExists(p.c_str())){fprintf(stderr,"Falta %s\n",p.c_str());break;}
  M[i]=LoadModel(p.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=20){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(cap){Shot("Primera_ronda",Cam(1),1);Shot("Seis_entradas",Cam(2),2);Shot("Agua_en_movimiento",Cam(3),3);for(int i=0;i<20;i++)Shot(N[i],ModelCam(i),i+10);}
 else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;Render(Cam(mode),mode,true);}}
 for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
