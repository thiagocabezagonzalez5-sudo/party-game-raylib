#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <string>
static const char* N[]={"mar_frio","lago_helado","tablero_4x4","bloque_oculto","bloque_emparejado","cursor_seleccion","simbolo_0_esfera","simbolo_1_cubo","simbolo_2_cono","simbolo_3_cilindro","simbolo_4_rombo","simbolo_5_nieve","simbolo_6_copo","simbolo_7_aurora","tempano_jugador","iceberg","montana_nevada","cueva_hielo","pinguino","foca","aurora_verde","aurora_violeta","aurora_cian","fragmento_hielo"};
static Model M[24]{};static std::string ROOT;
static void Part(int i,Vector3 p={0,0,0},Vector3 s={1,1,1}){DrawModelEx(M[i],p,{0,1,0},0,s,WHITE);}
static Camera3D Cam(int mode){
 if(mode==3)return {{8,12,12},{0,0,-1},{0,1,0},51,CAMERA_PERSPECTIVE};
 return {{0,14,10},{0,0,0},{0,1,0},50,CAMERA_PERSPECTIVE};
}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(0);Part(1);Part(2);
 for(int i=0;i<9;i++)Part(20+i%3,{-24+6.f*i,7+(i%3)*1.4f,-26});
 for(int i=0;i<6;i++)Part(16,{-22+8.5f*i,-.5f,-20},{1,1+(i%3)*.12f,1});
 Part(17,{0,0,-11});Part(15,{-12,-.5f,-9});Part(15,{11,-.5f,-10},{1.2f,1.2f,1.2f});Part(15,{15,-.5f,1},{.7f,.7f,.7f});
 for(Vector3 p: {Vector3{-12.6f,2.4f,-8.4f},Vector3{-11.6f,1.2f,-7.4f},Vector3{11.8f,3.f,-9.2f}})Part(18,p);
 Part(19,{12.5f,-.4f,-3});
 int n=mode==3?2:4;
 for(int i=0;i<n;i++){
  float x=n==2?(i==0?-7.2f:7.2f):(i%2==0?-7.2f:7.2f);
  float z=n==2?0:(i<2?-4.8f:4.8f);
  Part(14,{x,0,z});DrawCube({x,.82f,z},.55f,1.3f,.55f,i==0?RED:i==1?BLUE:i==2?GREEN:GOLD);
 }
 for(int i=0;i<16;i++){
  float x=(i%4-1.5f)*1.9f,z=(i/4-1.5f)*1.9f;
  bool revealed=mode==2&&(i==2||i==7||i==9||i==12);
  bool matched=mode==2&&(i==2||i==9);
  Part(matched?4:3,{x,0,z});
  if(revealed)Part(6+(matched?7:(i==7?1:4)),{x,matched?.32f:.95f,z});
  if(mode==3&&(i==3||i==10))Part(23,{x+.4f,.9f,z});
 }
 Part(5,{(mode==2?-.95f:-2.85f),0,(mode==3?.95f:-2.85f)});
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 d=Vector3Subtract(b.max,b.min);float r=fmaxf(d.x,fmaxf(d.y,d.z));
 Vector3 dir=Vector3Normalize({.7f,1.1f,1.5f});if(i==17||i>=20&&i<=22)dir=Vector3Normalize({.25f,.5f,1.5f});
 if(i<=5||i==14)dir=Vector3Normalize({.5f,1.8f,1.3f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.45f+1)),c,{0,1,0},42,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D c,int mode,bool hud){
 BeginDrawing();ClearBackground({10,19,48,255});BeginMode3D(c);Scene(mode);EndMode3D();
 if(hud){DrawRectangle(0,0,1280,68,{22,45,75,235});DrawText("PAREJAS GLACIARES / Modelos GLB",20,8,24,RAYWHITE);DrawText("1 Tablero oculto | 2 Pareja aurora | 3 Crujido",20,42,16,RAYWHITE);}EndDrawing();
}
static void Shot(const char* name,Camera3D c,int mode){Render(c,mode,false);TakeScreenshot((ROOT+"/Vistas/"+name+".png").c_str());}
int main(int argc,char**argv){
 ROOT=argc>2?argv[2]:GetWorkingDirectory();bool cap=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(cap?1600:1280,cap?1000:800,"Parejas Glaciares - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(ROOT.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<24;i++){
  std::string p=std::string("GLB/")+N[i]+".glb";if(!FileExists(p.c_str())){fprintf(stderr,"Falta %s\n",p.c_str());break;}
  M[i]=LoadModel(p.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=24){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(cap){Shot("Tablero_oculto",Cam(1),1);Shot("Pareja_aurora",Cam(2),2);Shot("Crujido",Cam(3),3);for(int i=0;i<24;i++)Shot(N[i],ModelCam(i),i+10);}
 else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;Render(Cam(mode),mode,true);}}
 for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
