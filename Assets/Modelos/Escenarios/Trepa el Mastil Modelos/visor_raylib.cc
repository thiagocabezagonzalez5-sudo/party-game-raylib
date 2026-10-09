#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"casco_barco","cubierta","barandilla","mastil","vela","franja_roja","franja_azul","franja_verde","franja_amarilla","cofa","jarcias","bandera_roja","bandera_azul","bandera_verde","bandera_amarilla","cofre","barril","cuervo","ala_cuervo","gaviota","isla_lejana","mar","espuma_ola","ancla","timon"};
static Model M[25]{};static std::string ROOT;
static void Part(int i,Vector3 p={0,0,0},Vector3 s={1,1,1}){DrawModelEx(M[i],p,{0,1,0},0,s,WHITE);}
static Camera3D Cam(int mode){
 if(mode==3)return {{0,6.4f,16},{0,5.8f,0},{0,1,0},54,CAMERA_PERSPECTIVE};
 if(mode==2)return {{13,8,20},{0,5.4f,0},{0,1,0},50,CAMERA_PERSPECTIVE};
 return {{0,6.4f,19},{0,5.8f,0},{0,1,0},54,CAMERA_PERSPECTIVE};
}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(21);Part(20,{-22,-4.5f,-42});
 for(int k=0;k<7;k++)Part(22,{-12+k*4.f,-4.36f,5+(k%2)*1.4f});
 for(int k=0;k<4;k++)Part(19,{-22+k*13.f,10+k*.8f,-13-k*3.f});
 rlPushMatrix();if(mode==2)rlRotatef(-5.2f,0,0,1);
 Part(0);Part(1);Part(2,{0,0,2.25f});Part(2,{0,0,-2.25f});
 Part(15,{-8.2f,0,.6f});Part(15,{8.2f,0,.6f});
 for(float x:{-7.7f,-7.f,7.1f})Part(16,{x,0,1});
 Part(23,{-8,-2.4f,2.4f});Part(24,{0,0,-1.8f});
 int count=mode==3?2:4;
 for(int i=0;i<count;i++){
  float x=(i-.5f*(count-1))*4.4f;
  Part(3,{x,0,0});Part(4,{x,0,0});Part(5+i,{x,0,0});
  Part(9,{x,0,0});Part(10,{x,0,0});Part(11+i,{x,0,0});
  DrawCube({x+.34f,i==1?5.1f:1.3f,.57f},.57f,1.25f,.5f,i==0?RED:(i==1?BLUE:(i==2?GREEN:GOLD)));
  if(mode==2&&i==2)Part(17,{x+.9f,6.8f,.75f});
 }
 rlPopMatrix();
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 d=Vector3Subtract(b.max,b.min);float r=fmaxf(d.x,fmaxf(d.y,d.z));
 Vector3 dir=Vector3Normalize({.7f,1.05f,1.6f});if(i==4||i==17||i==19)dir=Vector3Normalize({.3f,.35f,1.5f});
 if(i==0||i==1||i==21)dir=Vector3Normalize({.5f,1.8f,1.2f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.45f+1)),c,{0,1,0},42,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D c,int mode,bool hud){
 BeginDrawing();ClearBackground({113,176,223,255});BeginMode3D(c);Scene(mode);EndMode3D();
 if(hud){DrawRectangle(0,0,1280,68,{25,56,82,225});DrawText("TREPA EL MASTIL / Modelos GLB",20,8,24,RAYWHITE);DrawText("1 Cuatro mastiles | 2 Barco inclinado | 3 Dos mastiles",20,42,16,RAYWHITE);}EndDrawing();
}
static void Shot(const char* name,Camera3D c,int mode){Render(c,mode,false);TakeScreenshot((ROOT+"/Vistas/"+name+".png").c_str());}
int main(int argc,char**argv){
 ROOT=argc>2?argv[2]:GetWorkingDirectory();bool cap=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(cap?1600:1280,cap?1000:800,"Trepa el Mastil - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(ROOT.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<25;i++){
  std::string p=std::string("GLB/")+N[i]+".glb";if(!FileExists(p.c_str())){fprintf(stderr,"Falta %s\n",p.c_str());break;}
  M[i]=LoadModel(p.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=25){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(cap){Shot("Cuatro_mastiles",Cam(1),1);Shot("Barco_inclinado",Cam(2),2);Shot("Dos_mastiles",Cam(3),3);for(int i=0;i<25;i++)Shot(N[i],ModelCam(i),i+10);}
 else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;Render(Cam(mode),mode,true);}}
 for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
